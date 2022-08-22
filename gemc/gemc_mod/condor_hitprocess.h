#ifndef CONDOR_HITPROCESS_H
#define CONDOR_HITPROCESS_H 1

// gemc headers
#include "HitProcess.h"


class condorConstants
{
public:
	
	// Database parameters
	int    runNo;
	string date;
	string connection;
	char   database[80];
	
	
	
	// tdc_conc: tdc conversion factors
  double tdcconv;
	
	// veff: effective velocity
	double veff;
	
	// attlen: attenuation length
	double attlen;
	
	// countsForMIP: Desired ADC channel for MIP peak calibration
	double countsForMIP;
	
	// twlk: Time walk correction, 3 constants each for L and R
	double twlk[6];
	
	// toff_LR and tof_P2P: time offsets for Left-Right and Paddle-to-Paddle
	double toff_LR;
	double toff_RFpad;
	double toff_P2P;
	
	// ======== FADC Pedestals and sigmas ===========
	double pedestal = {};
	double pedestal_sigm= {};
	
	// tres: Gaussian sigma for smearing time resolution
	// indexes are sector/layer/paddle
	double tres;

	int    npaddles;  // Number of paddles for Panel 1A, 1B and 2.
	double    thick;     // Thickness of paddles (cm) for Panel 1A, 1B and 2.
	double dEdxMIP;      // Nominal MIP specific energy loss (MeV/gm/cm2)
	double dEMIP;     // Nominal MIP energy loss (MeV) for Panel 1A, 1B and 2.
	
	double pmtPEYld;      // Photoelectron yield (p.e./MeV)
	double pmtQE;         // Quantum efficiency of PMT
	double pmtDynodeGain; // PMT dynode gain
	double pmtDynodeK;    // PMT dynode secondary emission statistics factor: K=0 (Poisson) K=1 (exponential)
	double pmtFactor;     // Contribution to FWHM from PMT statistical fluctuations.
								 //	double tdcLSB;        // Conversion from ns to TDC channel.
	
	//	voltage signal parameters, using double gaussian + delay (function DGauss, need documentation for it)
	double vpar[4];
	
	// translation table
	TranslationTable TT;
	
	
};


// Class definition
/// \class condor_HitProcess
/// <b> Forward Time of Flight Hit Process Routine</b>\n\n

class condor_HitProcess : public HitProcess
{
public:
	
	~condor_HitProcess(){;}
	
	// constants initialized with initWithRunNumber
	static condorConstants ftc;
	
	void initWithRunNumber(int runno);
	
	// - integrateDgt: returns digitized information integrated over the hit
	map<string, double> integrateDgt(MHit*, int);
	
	// - multiDgt: returns multiple digitized information / hit
	map< string, vector <int> > multiDgt(MHit*, int);
	
	// - charge: returns charge/time digitized information / step
	virtual map< int, vector <double> > chargeTime(MHit*, int);
	
	// - voltage: returns a voltage value for a given time. The input are charge value, time
	virtual double voltage(double, double, double);
	
	// The pure virtual method processID returns a (new) identifier
	// containing hit sharing information
	vector<identifier> processID(vector<identifier>, G4Step*, detector);
	
	// creates the HitProcess
	static HitProcess *createHitClass() {return new condor_HitProcess;}
	
	// - electronicNoise: returns a vector of hits generated / by electronics.
	vector<MHit*> electronicNoise();
	
};



#endif




