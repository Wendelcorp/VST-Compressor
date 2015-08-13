//
//								aflange.h
//

#ifndef __aflange__
#define __aflange__

#include "public.sdk/source/vst2.x/audioeffectx.h"

#define	SAMPLE_RATE	44100
#define BOTTOM_FREQ 100
#define M_PI 3.141592653589793
#define M_PI_2 (M_PI/2.0)
#define	BSZ 8192		// must be about 1/5 of a second at given sample rate
#define ROUND(n)		((int)((double)(n)+0.5))

//
//	MODF - vaguely related to the library routine modf(), this macro breaks a double into
//	integer and fractional components i and f respectively.
//
//	n - input number, a double
//	i - integer portion, an integer (the input number integer portion should fit)
//	f - fractional portion, a double
//
#define	MODF(n,i,f) ((i) = (int)(n), (f) = (n) - (double)(i))


enum
{
	// Global
	kNumPrograms = 16,

	// Parameters Tags
	kRate = 0,
	kWidth,
	kFeedback,
	kDelay,
	kMixMode,

	kNumParams
};

enum
{
	kMixMono,
	kMixMonoMinus,
	kMixStereo
};
#define	NUM_MIX_MODES	3
#define	NUM_DELAYS	8

class AFlange;

//------------------------------------------------------------------------
class AFlangeProgram
{
friend class AFlange;
public:
	AFlangeProgram ();
	~AFlangeProgram () {}

private:	
	float paramSweepRate;
	float paramWidth;
	float paramFeedback;
	float paramDelay;
	float paramMixMode;
	char name[24];
};

//------------------------------------------------------------------------
class AFlange : public AudioEffectX
{
public:
	AFlange (audioMasterCallback audioMaster);
	~AFlange ();

	//---from AudioEffect-----------------------
	virtual void processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames);

	virtual void setProgram (VstInt32 program);
	virtual void setProgramName (char* name);
	virtual void getProgramName (char* name);
	virtual bool getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text);
	
	virtual void setParameter (VstInt32 index, float value);
	virtual float getParameter (VstInt32 index);
	virtual void getParameterLabel (VstInt32 index, char* label);
	virtual void getParameterDisplay (VstInt32 index, char* text);
	virtual void getParameterName (VstInt32 index, char* text);

	virtual void resume ();

	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion () { return 1000; }
	
	virtual VstPlugCategory getPlugCategory () { return kPlugCategEffect; }

protected:
	void setRate (float v);
	void setWidth (float v);
	void setFeedback (float v);
	void setDelay (float v);
	void setMixMode (float v);
	void setSweep(void);

	AFlangeProgram* programs;
	
	float _paramSweepRate;		// 0.0-1.0 passed in
	float _paramWidth;			// ditto
	float _paramFeedback;		// ditto
	float _paramDelay;			// ditto
	float _paramMixMode;		// ditto
	double _sweepRate;			// actual calc'd sweep rate
	double _feedback;			// 0.0 to 1.0
	double _feedbackPhase;		// -1.0 to 1.0
	int _sweepSamples;			// sweep width in # of samples
	int	   _delaySamples;		// number of samples to run behind filling pointer
	double _minSweepSamples;	// lower bound of calculated sweep range, calc'd by setSweep from rate, width, delay
	double _maxSweepSamples;	// upper bound, ditto
	int	  _mixMode;				// mapped to supported mix modes
	double *_buf;				// stored sound
	int	   _fp;					// fill/write pointer
	double _step;				// amount to step the sweep each sample
	double _sweep;				// current value of sweep in steps behind fill pointer
	double _outval;				// most recent output value (for feedback)

	// output mixing
	double _mixLeftWet;
	double _mixLeftDry;
	double _mixRightWet;
	double _mixRightDry;
};

#endif
