/*
 * (c) Copyright 2018 CORSIKA Project, corsika-project@lists.kit.edu
 *
 * This software is distributed under the terms of the GNU General Public
 * Licence version 3 (GPL Version 3). See file LICENSE for a full version of
 * the license.
 */

/* clang-format off */
// InteractionCounter used boost/histogram, which
// fails if boost/type_traits have been included before. Thus, we have
// to include it first...
#include <corsika/framework/process/InteractionCounter.hpp>
/* clang-format on */
#include <corsika/framework/process/ProcessSequence.hpp>
#include <corsika/framework/process/SwitchProcessSequence.hpp>
#include <corsika/framework/process/InteractionCounter.hpp>
#include <corsika/framework/geometry/Plane.hpp>
#include <corsika/framework/geometry/Sphere.hpp>
#include <corsika/framework/geometry/PhysicalGeometry.hpp>
#include <corsika/framework/core/Logging.hpp>
#include <corsika/framework/core/PhysicalUnits.hpp>
#include <corsika/framework/core/Cascade.hpp>
#include <corsika/framework/core/EnergyMomentumOperations.hpp>
#include <corsika/framework/utility/SaveBoostHistogram.hpp>
#include <corsika/framework/utility/CorsikaFenv.hpp>
#include <corsika/framework/random/RNGManager.hpp>

#include <corsika/output/OutputManager.hpp>
#include <corsika/modules/writers/SubWriter.hpp>
#include <corsika/modules/writers/EnergyLossWriter.hpp>
#include <corsika/modules/writers/LongitudinalWriter.hpp>

#include <corsika/media/Environment.hpp>
#include <corsika/media/FlatExponential.hpp>
#include <corsika/media/GeomagneticModel.hpp>
#include <corsika/media/HomogeneousMedium.hpp>
#include <corsika/media/IMagneticFieldModel.hpp>
#include <corsika/media/LayeredSphericalAtmosphereBuilder.hpp>
#include <corsika/media/NuclearComposition.hpp>
#include <corsika/media/MediumPropertyModel.hpp>
#include <corsika/media/UniformMagneticField.hpp>
#include <corsika/media/ShowerAxis.hpp>
#include <corsika/media/CORSIKA7Atmospheres.hpp>

#include <corsika/modules/BetheBlochPDG.hpp>
#include <corsika/modules/LongitudinalProfile.hpp>
#include <corsika/modules/ObservationPlane.hpp>
#include <corsika/modules/OnShellCheck.hpp>
#include <corsika/modules/StackInspector.hpp>
#include <corsika/modules/TrackWriter.hpp>
#include <corsika/modules/ParticleCut.hpp>
#include <corsika/modules/Pythia8.hpp>
#include <corsika/modules/Sibyll.hpp>
#include <corsika/modules/Epos.hpp>
#include <corsika/modules/UrQMD.hpp>
#include <corsika/modules/PROPOSAL.hpp>
#include <corsika/modules/QGSJetII.hpp>

#include <corsika/setup/SetupStack.hpp>
#include <corsika/setup/SetupTrajectory.hpp>

#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>

#include <iomanip>
#include <limits>
#include <string_view>

/*
  NOTE, WARNING, ATTENTION

  The .../Random.hpppp implement the hooks of external modules to the C8 random
  number generator. It has to occur excatly ONCE per linked
  executable. If you include the header below multiple times and
  link this togehter, it will fail.
 */
#include <corsika/modules/Random.hpp>

using namespace corsika;
using namespace std;

using EnvironmentInterface = IMediumPropertyModel<IMagneticFieldModel<IMediumModel>>;
using EnvType = Environment<EnvironmentInterface>;

using Particle = setup::Stack<EnvType>::particle_type;

void registerRandomStreams(int seed) {
  RNGManager<>::getInstance().registerRandomStream("cascade");
  RNGManager<>::getInstance().registerRandomStream("qgsjet");
  RNGManager<>::getInstance().registerRandomStream("sibyll");
  RNGManager<>::getInstance().registerRandomStream("epos");
  RNGManager<>::getInstance().registerRandomStream("pythia");
  RNGManager<>::getInstance().registerRandomStream("urqmd");
  RNGManager<>::getInstance().registerRandomStream("proposal");
  if (seed == 0) {
    std::random_device rd;
    seed = rd();
    CORSIKA_LOG_INFO("random seed (auto) {} ", seed);
  } else {
    CORSIKA_LOG_INFO("random seed {} ", seed);
  }
  RNGManager<>::getInstance().setSeed(seed);
}

template <typename T>
using MyExtraEnv = MediumPropertyModel<UniformMagneticField<T>>;

int main(int argc, char** argv) {

  // the main command line description
  CLI::App app{"Simulate standard (downgoing) showers with CORSIKA 8."};

  // some options that we want to fill in
  int A, Z, nevent = 0;

  // the following section adds the options to the parser

  // we start by definining a sub-group for the primary ID
  auto opt_Z = app.add_option("-Z", Z, "Atomic number for primary")
                   ->check(CLI::Range(0, 26))
                   ->group("Primary");
  auto opt_A = app.add_option("-A", A, "Atomic mass number for primary")
                   ->needs(opt_Z)
                   ->check(CLI::Range(1, 58))
                   ->group("Primary");
  app.add_option("-p,--pdg", "PDG code for primary.")
      ->excludes(opt_A)
      ->excludes(opt_Z)
      ->group("Primary");
  // the remainding options
  app.add_option("-E,--energy", "Primary energy in GeV")
      ->required()
      ->check(CLI::PositiveNumber)
      ->group("Primary");
  app.add_option("-z,--zenith", "Primary zenith angle (deg)")
      ->default_val(0.)
      ->check(CLI::Range(0., 90.))
      ->group("Primary");
  app.add_option("-a,--azimuth", "Primary azimuth angle (deg)")
      ->default_val(0.)
      ->check(CLI::Range(0., 360.))
      ->group("Primary");
  app.add_option("-N,--nevent", nevent, "The number of events/showers to run.")
      ->default_val(1)
      ->check(CLI::PositiveNumber)
      ->group("Library/Output");
  app.add_option("-f,--filename", "Filename for output library.")
      ->required()
      ->default_val("corsika_library")
      ->check(CLI::NonexistentPath)
      ->group("Library/Output");
  app.add_option("-s,--seed", "The random number seed.")
      ->default_val(0)
      ->check(CLI::NonNegativeNumber)
      ->group("Misc.");
  bool force_interaction = false;
  app.add_flag("--force-interaction", force_interaction,
               "Force the location of the first interaction.")
      ->group("Misc.");
  app.add_option("-v,--verbosity", "Verbosity level: warn, info, debug, trace.")
      ->default_val("info")
      ->check(CLI::IsMember({"warn", "info", "debug", "trace"}))
      ->group("Misc.");

  double emcutgev;
  app.add_option("--emcut", emcutgev, "EM charged particle energy [GeV].")
      ->default_val(50)
      ->check(CLI::PositiveNumber)
      ->group("Cuts");

  double hadcutgev;
  app.add_option("--hadcut", hadcutgev, "hadron energy cut [GeV].")
      ->default_val(50)
      ->check(CLI::PositiveNumber)
      ->group("Cuts");
  
  // parse the command line options into the variables
  CLI11_PARSE(app, argc, argv);

  if (app.count("--verbosity")) {
    std::string_view const loglevel = app["--verbosity"]->as<std::string_view>();
    if (loglevel == "warn") {
      logging::set_level(logging::level::warn);
    } else if (loglevel == "info") {
      logging::set_level(logging::level::info);
    } else if (loglevel == "debug") {
      logging::set_level(logging::level::debug);
    } else if (loglevel == "trace") {
#ifndef _C8_DEBUG_
      CORSIKA_LOG_ERROR("trace log level requires a Debug build.");
      return 1;
#endif
      logging::set_level(logging::level::trace);
    }
  }

  // check that we got either PDG or A/Z
  // this can be done with option_groups but the ordering
  // gets all messed up
  if (app.count("--pdg") == 0) {
    if ((app.count("-A") == 0) || (app.count("-Z") == 0)) {
      CORSIKA_LOG_ERROR("If --pdg is not provided, then both -A and -Z are required.");
      return 1;
    }
  }

  // initialize random number sequence(s)
  registerRandomStreams(app["--seed"]->as<int>());

  /* === START: SETUP ENVIRONMENT AND ROOT COORDINATE SYSTEM === */
  EnvType env;
  CoordinateSystemPtr const& rootCS = env.getCoordinateSystem();
  Point const center{rootCS, 0_m, 0_m, 0_m};
  GeomagneticModel wmm(center, corsika_data("GeoMag/WMM.COF"));

  // build a Linsley US Standard atmosphere into `env`
  create_5layer_atmosphere<EnvironmentInterface, MyExtraEnv>(
      env, AtmosphereId::LinsleyUSStd, center, Medium::AirDry1Atm,
      wmm.getField(2022.5, 10_km, 49, 8.4));

  /* === END: SETUP ENVIRONMENT AND ROOT COORDINATE SYSTEM === */

  ofstream atmout("earth.dat");
  for (LengthType h = 0_m; h < 110_km; h += 100_m) {
    Point const ptest{rootCS, 0_m, 0_m, constants::EarthRadius::Mean + h};
    auto rho =
        env.getUniverse()->getContainingNode(ptest)->getModelProperties().getMassDensity(
            ptest);
    atmout << h / 1_m << " " << rho / 1_kg * cube(1_m) << "\n";
  }
  atmout.close();

  /* === START: CONSTRUCT PRIMARY PARTICLE === */

  // parse the primary ID as a PDG or A/Z code
  Code beamCode;

  // check if we want to use a PDG code instead
  if (app.count("--pdg") > 0) {
    beamCode = convert_from_PDG(PDGCode(app["--pdg"]->as<int>()));
  } else {
    // check manually for proton and neutrons
    if ((A == 1) && (Z == 1))
      beamCode = Code::Proton;
    else if ((A == 1) && (Z == 0))
      beamCode = Code::Neutron;
    else
      beamCode = get_nucleus_code(A, Z);
  }
  HEPEnergyType mass = get_mass(beamCode);

  // particle energy
  HEPEnergyType const E0 = 1_GeV * app["--energy"]->as<double>();

  // direction of the shower in (theta, phi) space
  auto const thetaRad = app["--zenith"]->as<double>() / 180. * M_PI;
  auto const phiRad = app["--azimuth"]->as<double>() / 180. * M_PI;

  // convert Elab to Plab
  HEPMomentumType P0 = calculate_momentum(E0, mass);

  // convert the momentum to the zenith and azimuth angle of the primary
  auto const [px, py, pz] =
      std::make_tuple(P0 * sin(thetaRad) * cos(phiRad), P0 * sin(thetaRad) * sin(phiRad),
                      -P0 * cos(thetaRad));
  auto plab = MomentumVector(rootCS, {px, py, pz});
  /* === END: CONSTRUCT PRIMARY PARTICLE === */

  /* === START: CONSTRUCT GEOMETRY === */
  auto const observationHeight = 5.3_km + constants::EarthRadius::Mean;
  auto const injectionHeight = 111.75_km + constants::EarthRadius::Mean;
  auto const t = -observationHeight * cos(thetaRad) +
                 sqrt(-static_pow<2>(sin(thetaRad) * observationHeight) +
                      static_pow<2>(injectionHeight));
  Point const showerCore{rootCS, 0_m, 0_m, observationHeight};
  Point const injectionPos =
      showerCore + DirectionVector{rootCS,
                                   {-sin(thetaRad) * cos(phiRad),
                                    -sin(thetaRad) * sin(phiRad), cos(thetaRad)}} *
                       t;

  // we make the axis much longer than the inj-core distance since the
  // profile will go beyond the core, depending on zenith angle
  ShowerAxis const showerAxis{injectionPos, (showerCore - injectionPos) * 1.2, env};
  /* === END: CONSTRUCT GEOMETRY === */

  // create the output manager that we then register outputs with
  OutputManager output(app["--filename"]->as<std::string>());

  // register energy losses as output
  EnergyLossWriter dEdX{showerAxis, 10_g / square(1_cm), 200};
  output.add("energyloss", dEdX);

  //// create a track writer and register it with the output manager
  //TrackWriter tracks;
  //output.add("tracks", tracks);

  corsika::sibyll::Interaction sibyll{env};
  InteractionCounter sibyllCounted{sibyll};

  corsika::pythia8::Decay decayPythia;

  // use sibyll decay routine for decays of particles unknown to pythia
  corsika::sibyll::Decay decaySibyll{{
      Code::N1440Plus,
      Code::N1440MinusBar,
      Code::N1440_0,
      Code::N1440_0Bar,
      Code::N1710Plus,
      Code::N1710MinusBar,
      Code::N1710_0,
      Code::N1710_0Bar,

      Code::Pi1300Plus,
      Code::Pi1300Minus,
      Code::Pi1300_0,

      Code::KStar0_1430_0,
      Code::KStar0_1430_0Bar,
      Code::KStar0_1430_Plus,
      Code::KStar0_1430_MinusBar,
  }};

  // decaySibyll.printDecayConfig();

  HEPEnergyType const emcut = emcutgev*(1_GeV);
  HEPEnergyType const hadcut = hadcutgev*(1_GeV);
  ParticleCut<SubWriter<decltype(dEdX)>> cut(emcut, emcut, hadcut, hadcut, true, dEdX);

  // energy threshold for high energy hadronic model. Affects LE/HE switch for hadron
  // interactions and the hadronic photon model in proposal
  HEPEnergyType heHadronModelThreshold = 63.1_GeV;
  corsika::proposal::Interaction emCascade(env, sibyll.getHadronInteractionModel(),
                                           heHadronModelThreshold);
  // NOT available for PROPOSAL due to interface trouble:
  // InteractionCounter emCascadeCounted(emCascade);
  // corsika::proposal::ContinuousProcess<SubWriter<decltype(dEdX)>> emContinuous(env);
  BetheBlochPDG<SubWriter<decltype(dEdX)>> emContinuous{dEdX};

  LongitudinalWriter profile{showerAxis, 200, 10_g / square(1_cm)};
  output.add("profile", profile);
  LongitudinalProfile<SubWriter<decltype(profile)>> longprof{profile};

  corsika::urqmd::UrQMD urqmd;
  InteractionCounter urqmdCounted(urqmd);
  StackInspector<setup::Stack<EnvType>> stackInspect(10000, false, E0);

  // assemble all processes into an ordered process list
  struct EnergySwitch {
    HEPEnergyType cutE_;
    EnergySwitch(HEPEnergyType cutE)
        : cutE_(cutE) {}
    bool operator()(const Particle& p) const { return (p.getKineticEnergy() < cutE_); }
  };
  auto hadronSequence =
      make_select(EnergySwitch(heHadronModelThreshold), urqmdCounted, sibyllCounted);
  auto decaySequence = make_sequence(decayPythia, decaySibyll);

  // create a track writer and register it with the output manager
  TrackWriter trackWriter;
  output.add("tracks", trackWriter);
  //TrackWriter trackWriter{tracks};

  // observation plane
  Plane const obsPlane(showerCore, DirectionVector(rootCS, {0., 0., 1.}));
  ObservationPlane<setup::Tracking, ParticleWriterParquet> observationLevel{
      obsPlane, DirectionVector(rootCS, {1., 0., 0.})};
  // register ground particle output
  output.add("particles", observationLevel);

  // assemble the final process sequence
  
  auto sequence = make_sequence(stackInspect, hadronSequence, decaySequence, cut,
                                emCascade, emContinuous, trackWriter,
                                observationLevel, longprof);
  /* === END: SETUP PROCESS LIST === */
  
  // create the cascade object using the default stack and tracking implementation
  setup::Tracking tracking;
  setup::Stack<EnvType> stack;
  Cascade EAS(env, tracking, sequence, output, stack);
  
  // print our primary parameters all in one place
  if (app["--pdg"]->count() > 0) {
    CORSIKA_LOG_INFO("Primary PDG ID: {}", app["--pdg"]->as<int>());
  } else {
    CORSIKA_LOG_INFO("Primary Z/A: {}/{}", Z, A);
  }
  CORSIKA_LOG_INFO("Primary Energy: {}", E0);
  CORSIKA_LOG_INFO("Primary Momentum: {}", P0);
  CORSIKA_LOG_INFO("Point of Injection: {}", injectionPos.getCoordinates());
  CORSIKA_LOG_INFO("Shower Axis Length: {}", (showerCore - injectionPos).getNorm() * 1.2);

  // trigger the output manager to open the library for writing
  output.startOfLibrary();

  // loop over each shower
  for (int i_shower = 1; i_shower < nevent + 1; i_shower++) {

    CORSIKA_LOG_INFO("Shower {} / {} ", i_shower, nevent);

    // directory for outputs
    string const outdir(app["--filename"]->as<std::string>());
    string const labHist_file = outdir + "/inthist_lab_" + to_string(i_shower) + ".npz";
    string const cMSHist_file = outdir + "/inthist_cms_" + to_string(i_shower) + ".npz";

    // setup particle stack, and add primary particle
    stack.clear();

    // add the desired particle to the stack
    stack.addParticle(std::make_tuple(
        beamCode, calculate_kinetic_energy(plab.getNorm(), get_mass(beamCode)),
        plab.normalized(), injectionPos, 0_ns));

    // if we want to fix the first location of the shower
    if (force_interaction) {
      CORSIKA_LOG_INFO("Fixing first interaction at injection point.");
      EAS.forceInteraction();
    }

    // run the shower
    EAS.run();

    HEPEnergyType const Efinal =
        dEdX.getEnergyLost() + observationLevel.getEnergyGround();

    CORSIKA_LOG_INFO(
        "total energy budget (GeV): {} (dEdX={} ground={}), "
        "relative difference (%): {}",
        Efinal / 1_GeV, dEdX.getEnergyLost() / 1_GeV,
        observationLevel.getEnergyGround() / 1_GeV, (Efinal / E0 - 1) * 100);

    // auto const hists = heModelCounted.getHistogram() + urqmdCounted.getHistogram();
    auto const hists = sibyllCounted.getHistogram() + urqmdCounted.getHistogram();

    save_hist(hists.labHist(), labHist_file, true);
    save_hist(hists.CMSHist(), cMSHist_file, true);
  }

  // and finalize the output on disk
  output.endOfLibrary();
}
