// G4 headers
#include "G4Poisson.hh"
#include "Randomize.hh"

#include <CCDB/Calibration.h>
#include <CCDB/Model/Assignment.h>
#include <CCDB/CalibrationGenerator.h>
using namespace ccdb;

// gemc headers
#include "condor_hitprocess.h"

// CLHEP units
#include "CLHEP/Units/PhysicalConstants.h"
using namespace CLHEP;

static condorConstants initializeCondorConstants(int runno, string digiVariation = "default", string digiSnapshotTime = "no", bool 
accountForHardwareStatus = false) {

  condorConstants ftc;
		
	// do not initialize at the beginning, only after the end of the first event,
	// with the proper run number coming from options or run table
	if (runno == -1) return ftc;
	string timestamp = "";
	if(digiSnapshotTime != "no") {
		timestamp = ":"+digiSnapshotTime;
	}
	
	ftc.runNo = runno;
	ftc.date = "2015-11-29";
	if (getenv("CCDB_CONNECTION") != NULL)
		ftc.connection = (string) getenv("CCDB_CONNECTION");
	else
		ftc.connection = "mysql://clas12reader@clasdb.jlab.org/clas12";
	
		
	ftc.dEdxMIP = 1.956; // muons in polyvinyltoluene
	ftc.pmtPEYld = 500;
	//	ftc.tdcLSB        = 42.5532;// counts per ns (23.5 ps LSB)
	
  ftc.thick=2;
  ftc.dEMIP = ftc.thick * ftc.dEdxMIP;
	

  ftc.attlen =  200;//dummy value
	ftc.veff = 16;
  ftc.countsForMIP=1000;
  ftc.twlk[0] = 40;
  ftc.twlk[1]=0.5;
  ftc.twlk[2]=0;
  ftc.twlk[3] = 40;
  ftc.twlk[4]=0.5;
  ftc.twlk[5]=0;
  
  ftc.toff_LR=0;
  ftc.toff_RFpad = 0;
  ftc.toff_P2P = 0;

  ftc.tdcconv= 0.02345;
  
  ftc.tres=0.1;
  
  ftc.pedestal=101;
  ftc.pedestal_sigm=2;
	
	

	/*// filling translation table
	for (int lay =0; lay<21; lay++){
	  for(int paddle=0; paddle<ftc.npaddles[lay]; paddle++){
	    for(int pmt=0; pmt<4; pmt++){
	      
	      int crate = lay*210+paddle*10+pmt;//dummy values
	      int slot = lay*210+paddle*10+pmt;
	      int channel = lay*210+paddle*10+pmt;
	      
	      int sector = 0;
	      
	      
	    // order is important as we could have duplicate entries w/o it
	      ftc.TT.addHardwareItem({sector, lay, paddle, pmt}, Hardware(crate, slot, channel));
	    }
	  }
        }
        cout << "  > Data loaded in translation table " << ftc.TT.getName() << endl;
        
	*/
	
	//int isec, ilay;
	
	//vector<vector<double> > data;
	
	return ftc;
	
}



map<string, double> condor_HitProcess::integrateDgt(MHit* aHit, int hitn) {
	map<string, double> dgtz;
	
	// hit ids
	vector<identifier> identity = aHit->GetId();
	
	int group_x = identity[0].id;
	int group_z  = identity[1].id; // 1-1A, 2-1B, 3-2B
	int panel_x = identity[2].id;
  int panel_y = identity[3].id;
  int panel_z = identity[4].id;
	int pmt    = identity[5].id; // 0=> Left PMT, 1=> Right PMT. A better name would be pmtSide
	
	// TDC conversion factors
	double tdcconv = ftc.tdcconv;

	if(aHit->isBackgroundHit == 1) {
		
		// background hit has all the energy in the first step. Time is also first step
		double totEdep = aHit->GetEdep()[0];
		double stepTime = aHit->GetTime()[0];
		double adc = totEdep * ftc.countsForMIP / ftc.dEMIP ; // no gain as that comes from data already
		double tdc = stepTime/tdcconv;
		
		dgtz["hitn"]       = hitn;
		dgtz["group_x"]    = group_x;
		dgtz["group_z"]    = group_z;
    dgtz["panel_x"]    = panel_x;
    dgtz["panel_y"]    = panel_y;
    dgtz["panel_z"]    = panel_z;
		dgtz["ADC_order"] = pmt;
		dgtz["ADC_ADC"]   = (int) adc;
		dgtz["ADC_time"]  = (tdc*24.0/1000);
		dgtz["ADC_ped"]   = 0;

		dgtz["TDC_order"] = pmt + 2;
		dgtz["TDC_TDC"]   = (int) tdc;

		return dgtz;
	}
	
	
	trueInfos tInfos(aHit);
	
	// Get the paddle half-length
	double length = aHit->GetDetector().dimensions[0];
	
	// Distances from left, right
	//	double dLeft  = length + tInfos.lx;
	//	double dRight = length - tInfos.lx;
	
	double d = length + (1 - 2 * pmt) * tInfos.lx;
	
	// attenuation length
	//	double attlenL = ftc.attlen[sector-1][panel-1][0][paddle-1];
	//	double attlenR = ftc.attlen[sector-1][panel-1][1][paddle-1];
	
	double attlen = ftc.attlen;
	double attlen_otherside = ftc.attlen;
	// attenuation factor
	//	double attLeft  = exp(-dLeft/cm/attlenL);
	//	double attRight = exp(-dRight/cm/attlenR);
	
	double att = exp(-d / cm / attlen);
	
	// Gain factors to simulate FCAL PMT gain matching algorithm.
	// Each L,R PMT pair has HV adjusted so geometeric mean sqrt(L*R)
	// is independent of counter length, which compensates for
	// the factor exp(-L/2/attlen) where L=full length of bar.
	//	double gainLeft  = sqrt(attLeft*attRight);
	//	double gainRight = gainLeft;
	
	double gain = sqrt(exp(-d / cm / attlen) * exp(-(2 * length - d) / cm / attlen_otherside));
	
	// Attenuated light at PMT
	//	double eneL = tInfos.eTot*attLeft;
	//	double eneR = tInfos.eTot*attRight;
	
	double ene = tInfos.eTot*att;
	
	// giving geantinos some energies
	if (aHit->GetPID() == 0) {
		double gmomentum = aHit->GetMom().mag() / GeV;
		
		ene = gmomentum*att;
		
	}
	
	double adc = 0;
	double tdc = 0;


	// Fluctuate the light measured by the PMT with
	// Poisson distribution for emitted photoelectrons
	// Treat L and R separately, in case nphe=0
	
		
	
	double nphe = G4Poisson(ene * ftc.pmtPEYld);
	ene = nphe / ftc.pmtPEYld;
	
	if (ene > 0) {
		adc = ene * ftc.countsForMIP / ftc.dEMIP / gain;
    //cout << "debug adc = " << adc << endl;
    //cout << "gain = " << gain << endl;
    //cout << "ene = " << ene << endl;
    //cout << "ftc.countsForMIP =" << ftc.countsForMIP << endl;
    //cout << "ftc.dEMIP = " << ftc.dEMIP << endl;
		double A = ftc.twlk[3 * pmt + 0];
		double B = ftc.twlk[3 * pmt + 1];
		//double            C = ftc.twlk[sector-1][panel-1][2][paddle-1];
		
		double timeWalk  = A / pow(adc, B);
//		double timeWalkU = A / pow(adcu, B);
		
		//		double tU = tInfos.time + d/ftc.veff[sector-1][panel-1][pmt][paddle-1]/cm + (1. - 2. * pmt)*ftc.toff_LR[sector-1][panel-1][paddle-1]/2.
		//		- ftc.toff_RFpad[sector-1][panel-1][paddle-1]
		//		- ftc.toff_P2P[sector-1][panel-1][paddle-1];
		
		double tU = tInfos.time + d/ftc.veff/cm + (1. - 2. * pmt)*ftc.toff_LR/2.
		- ftc.toff_RFpad;
		
		// cout << " FCAL Unsmeared Time before p2p subtraction: " << tU << endl;
		
		tU = tU - ftc.toff_P2P;
		
		// cout << " FCAL Unsmeared Time after p2p subtraction: " << tU << endl;
		
//		tdcu = (tU + timeWalkU) / tdcconv;
		tdc  = G4RandGauss::shoot(tU+ timeWalk, sqrt(2) * ftc.tres) / tdcconv;
		
	}
	
	
	// Status flags
//	if(accountForHardwareStatus) {
//		switch (ftc.status[sector - 1][panel - 1][pmt][paddle - 1]) {
//		case 0:
//			break;
//		case 1:
//			adc = 0;
//			break;
//		case 2:
//			tdc = 0;
//      break;
//		case 3:
//			adc = tdc = 0;
//			break;
//
//		case 5:
//			break;
//
//		default:
//			cout << " > Unknown CONDOR status: " << ftc.status[sector - 1][panel - 1][0][paddle - 1] << " for sector " << sector << ",  panel " << panel << ", paddle " << paddle << " left " << endl;
//		}
//	}
	
	//	cout << " > FCAL status: " << ftc.status[sector-1][panel-1][0][paddle-1] << " for sector " << sector << ",  panel " << panel << ", paddle " << paddle << " left: " << adcl << endl;
	//	cout << " > FCAL status: " << ftc.status[sector-1][panel-1][1][paddle-1] << " for sector " << sector << ",  panel " << panel << ", paddle " << paddle << " right:  " << adcr << endl;
	
	dgtz["group_x"]    = group_x;
	dgtz["group_z"]    = group_z;
  dgtz["panel_x"]    = panel_x;
  dgtz["panel_y"]    = panel_y;
  dgtz["panel_z"]    = panel_z;
	dgtz["ADC_order"] = pmt;
	dgtz["ADC_ADC"]   = (int) adc;
	dgtz["ADC_time"]  = (tdc*tdcconv);
	dgtz["ADC_ped"]   = 0;

	dgtz["TDC_order"] = pmt + 2;
	dgtz["TDC_TDC"]   = (int) tdc;


	// decide if write an hit or not
	writeHit = true;
	// define conditions to reject hit
	if (rejectHitConditions) {
		writeHit = false;
	}
	
	return dgtz;
}


// sector = identity[0].id;
// panel  = identity[1].id; // 1-1A, 2-1B, 3-2B
// paddle = identity[2].id;
// pmt    = identity[3].id; // 0=> Left PMT, 1=> Right PMT. A better name would be pmtSide

vector<identifier> condor_HitProcess::processID(vector<identifier> id, G4Step* aStep, detector Detector) {
  //cout << "condor_HitProcess::processID" << endl;
	vector<identifier> yid = id;
	yid[0].id_sharing = 1; // group x
	yid[1].id_sharing = 1; // group y
	yid[2].id_sharing = 1; // panel x
	yid[3].id_sharing = 1; // panel y
  yid[4].id_sharing = 1; // panel z
  yid[5].id_sharing = 1; // side, left or right
	
	if (yid[5].id != 0) {
		cout << "*****WARNING***** in condor_HitProcess :: processID, identifier PTT of the original hit should be 0 " << endl;
		cout << "yid[3].id = " << yid[5].id << endl;
	}
	
	// Now we want to have similar identifiers, but the only difference be id PMT to be 1, instead of 0
	identifier this_id = yid[0];
	yid.push_back(this_id);
	this_id = yid[1];
	yid.push_back(this_id);
	this_id = yid[2];
	yid.push_back(this_id);
	this_id = yid[3];
  yid.push_back(this_id);
  this_id = yid[4];
  yid.push_back(this_id);
  this_id = yid[5];
	this_id.id = 1;
	yid.push_back(this_id);
	
	return yid;
}

// - electronicNoise: returns a vector of hits generated / by electronics.

vector<MHit*> condor_HitProcess::electronicNoise() {
  //cout << "condor_HitProcess::electronicNoise" << endl;
	vector<MHit*> noiseHits;
	
	// first, identify the cells that would have electronic noise
	// then instantiate hit with energy E, time T, identifier IDF:
	//
	// MHit* thisNoiseHit = new MHit(E, T, IDF, pid);
	
	// push to noiseHits collection:
	// noiseHits.push_back(thisNoiseHit)
	
	return noiseHits;
}


map< string, vector <int> > condor_HitProcess::multiDgt(MHit* aHit, int hitn) {
	map< string, vector <int> > MH;
	//cout << "condor_HitProcess::multiDgt" << endl;
	return MH;
}

// - charge: returns charge/time digitized information / step

map< int, vector <double> > condor_HitProcess::chargeTime(MHit* aHit, int hitn) {
	map< int, vector <double> > CT;
	//cout << "condor_HitProcess::chargeTime" << endl;
	vector<double> hitNumbers;
	vector<double> stepIndex;
	vector<double> chargeAtElectronics;
	vector<double> timeAtElectronics;
	vector<double> identifiers;
	vector<double> hardware;
	hitNumbers.push_back(hitn);
	
	// getting identifiers
	vector<identifier> identity = aHit->GetId();
	
	int group_x = identity[0].id;
	int group_z = identity[1].id;
	int panel_x = identity[2].id;
  int panel_y = identity[3].id;
  int panel_z = identity[4].id;
	int pmt = identity[5].id; // 0=> Left PMT, 1=> Right PMT. A better name would be pmtSide
	
	identifiers.push_back(group_x); // sector
	identifiers.push_back(group_z); // panel, 1a, 1b, 2a
	identifiers.push_back(panel_x); // paddle number
  identifiers.push_back(panel_y); // paddle number
  identifiers.push_back(panel_z); // paddle number
	identifiers.push_back(pmt); // the pmt side: 0=> Left, 1=>Right
	
	// getting hardware
  //Hardware thisHardware = ftc.TT.getHardware({sector, panel, paddle, pmt});
	//hardware.push_back(thisHardware.getCrate());
	//hardware.push_back(thisHardware.getSlot());
	//hardware.push_back(thisHardware.getChannel());
  
  //making up crate and slot numbers.
  hardware.push_back(group_x*14+group_z);
  hardware.push_back(panel_y);
  hardware.push_back(panel_x+100*panel_z);
  
  
	// Adding pedestal mean and sigma into the hardware as well
	// All of these variables start from 1, therefore -1 is subtracted, e.g. sector-1
	hardware.push_back(ftc.pedestal);
	hardware.push_back(ftc.pedestal_sigm);
	
	// attenuation length
	double attlen = ftc.attlen;
	double attlen_otherside = ftc.attlen;
	
	trueInfos tInfos(aHit);
	
	// Get the paddle half-length
	double length = aHit->GetDetector().dimensions[0];
	
	// Vector of positions of the hit in each step
	vector<G4ThreeVector> Lpos = aHit->GetLPos();
	
	// Vector of Edep and time of the hit in each step
	vector<G4double> Edep = aHit->GetEdep();
	vector<G4double> time = aHit->GetTime();
	
	for (unsigned int s = 0; s < tInfos.nsteps; s++) {
		// Distances from left, right
		//	double dLeft  = length + tInfos.lx;
		//	double dRight = length - tInfos.lx;
		
		double d = length + (1 - 2 * pmt) * Lpos[s].x();
		//double d = length + (1 - 2 * pmt) * tInfos.lx;
		
		// attenuation factor
		//	double attLeft  = exp(-dLeft/cm/attlenL);
		//	double attRight = exp(-dRight/cm/attlenR);
		
		double att = exp(-d / cm / attlen);
		
		// Gain factors to simulate FCAL PMT gain matching algorithm.
		// Each L,R PMT pair has HV adjusted so geometeric mean sqrt(L*R)
		// is independent of counter length, which compensates for
		// the factor exp(-L/2/attlen) where L=full length of bar.
		//	double gainLeft  = sqrt(attLeft*attRight);
		//	double gainRight = gainLeft;
		
		double gain = sqrt(exp(-d / cm /attlen ) * exp(-(2 * length - d) / cm / attlen_otherside));
		
		// Attenuated light at PMT
		//	double eneL = tInfos.eTot*attLeft;
		//	double eneR = tInfos.eTot*attRight;
		
		double ene = Edep[s] * att;
		
		// giving geantinos some energies
		if (aHit->GetPID() == 0) {
			double gmomentum = aHit->GetMom().mag() / GeV;
			//		eneL = gmomentum*attLeft;
			//		eneR = gmomentum*attRight;
			
			ene = gmomentum*att;
		}
		
		double adc = 0;
		
		// Fluctuate the light measured by the PMT with
		// Poisson distribution for emitted photoelectrons
		// Treat L and R separately, in case nphe=0
		
		double nphe = G4Poisson(ene * ftc.pmtPEYld);
		ene = nphe / ftc.pmtPEYld;
		
		if (ene > 0) {
			adc = ene * ftc.countsForMIP / ftc.dEMIP / gain;
			double A = ftc.twlk[3 * pmt + 0];
			double B = ftc.twlk[3 * pmt + 1];
			//double            C = ftc.twlk[sector-1][panel-1][2][paddle-1];
			double timeWalk = A / pow(adc, B);
			
			double stepTimeU = time[s] + d/ftc.veff/cm + (1. - 2. * pmt)*ftc.toff_LR/2.
			- ftc.toff_RFpad
			- ftc.toff_P2P
			+ timeWalk;
			
			double stepTime = G4RandGauss::shoot(stepTimeU, sqrt(2) * ftc.tres);
			
			stepIndex.push_back(s); // Since it is going to be only one hit, i.e. only one step
			chargeAtElectronics.push_back(adc);
			timeAtElectronics.push_back(stepTime);
		}
		
	}
	
	//	// Status flags
	//	switch (ftc.status[sector-1][panel-1][pmt][paddle-1])
	//	{
	//		case 0:
	//			break;
	//		case 1:
	//			adc = 0;
	//			break;
	//		case 2:
	//			tdc = 0;
	//			break;
	//		case 3:
	//			adc = tdc = 0;
	//			break;
	//
	//		case 5:
	//			break;
	//
	//		default:
	//			cout << " > Unknown FCAL status: " << ftc.status[sector-1][panel-1][0][paddle-1] << " for sector " << sector << ",  panel " << panel << ", paddle " << paddle << " left " << endl;
	//	}
	
	
	CT[0] = hitNumbers;
	CT[1] = stepIndex;
	CT[2] = chargeAtElectronics;
	CT[3] = timeAtElectronics;
	CT[4] = identifiers;
	CT[5] = hardware;
	
	return CT;
}

// - voltage: returns a voltage value for a given time. The inputs are:
// charge value (coming from chargeAtElectronics)
// time (coming from timeAtElectronics)

double condor_HitProcess::voltage(double charge, double time, double forTime) {
  //cout << "condor_HitProcess::voltage" << endl;
	//	return 0.0;
	return PulseShape(forTime, ftc.vpar, charge, time);
}

void condor_HitProcess::initWithRunNumber(int runno)
{
  //cout << "condor_HitProcess::initWithRunNumber" << endl;
	string digiVariation    = gemcOpt.optMap["DIGITIZATION_VARIATION"].args;
	string digiSnapshotTime = gemcOpt.optMap["DIGITIZATION_TIMESTAMP"].args;
	
	if (ftc.runNo != runno) {
		cout << " > Initializing " << HCname << " digitization for run number " << runno << endl;
		ftc = initializeCondorConstants(runno, digiVariation, digiSnapshotTime, accountForHardwareStatus);
		ftc.runNo = runno;
	}
}

// this static function will be loaded first thing by the executable
condorConstants condor_HitProcess::ftc = initializeCondorConstants(-1);




