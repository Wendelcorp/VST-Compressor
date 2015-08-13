//
//								apitch.h
//

#ifndef __apitch__
#define __apitch__

#include "public.sdk/source/vst2.x/audioeffectx.h"

#define	SAMPLE_RATE	44100
#define BOTTOM_FREQ 100
#define	BSZ 8192		// must be about 1/5 of a second at given sample rate
#define ROUND(n)		((int)((double)(n)+0.5))
#define SEMITONE		1.0594631
#define	XFADE			((int)((12 * SAMPLE_RATE) / 1000))
#define PIN(n,min,max)  ((n) > (max) ? (max) : ((n) < (min) ? (min) : (n)))

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
	kFinePitch = 0,
	kCoarsePitch,
	kFeedback,
	kDelay,
	kMixMode,

	kNumParams
};

enum
{
	kMixMono,
	kMixWetOnly,
	kMixWetLeft,
	kMixWetRight,
	kMixWetLeftish,
	kMixWetRightish,
	kMixStereo
};
#define	NUM_MIX_MODES	7
#define	NUM_PITCHES 25

class APitch;

//------------------------------------------------------------------------
class APitchProgram
{
friend class APitch;
public:
	APitchProgram ();
	~APitchProgram () {}

private:	
	float paramFinePitch;
	float paramCoarsePitch;
	float paramFeedback;
	float paramDelay;
	float paramMixMode;
	char name[24];
};

//------------------------------------------------------------------------
class APitch : public AudioEffectX
{
public:
	APitch (audioMasterCallback audioMaster);
	~APitch ();

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
	void setFine (float v);
	void setCoarse (float v);
	void setFeedback (float v);
	void setDelay (float v);
	void setMixMode (float v);
	void setSweep(void);
	void initFadeLookup(void);

	APitchProgram* programs;
	
	float _paramFinePitch;		// 0.0-1.0 passed in
	float _paramCoarsePitch;	// ditto
	float _paramFeedback;		// ditto
	float _paramDelay;			// ditto
	float _paramMixMode;		// ditto
	double _sweepRate;			// actual calc'd sweep rate
	double _feedback;			// 0.0 to 1.0
	double _feedbackPhase;		// -1.0 to 1.0
	int _sweepSamples;			// sweep width in # of samples
	int	   _delaySamples;		// number of samples to run behind filling pointer
	int	  _mixMode;				// mapped to supported mix modes
	double *_buf;				// stored sound
	int	   _fp;					// fill/write pointer
	double _outval;				// most recent output value (for feedback)
	double _fadeIn[XFADE];		// lookup table for cross-fade values
	double _fadeOut[XFADE];		// lookup table for cross-fade values
	int _sweepSteps;			// number of steps before the sweep resets
	double _sweepInc;			// calculated by desired pitch deviation
	bool _increasing;			// flag for pitch increasing/decreasing

	// output mixing
	double _mixLeftWet;
	double _mixLeftDry;
	double _mixRightWet;
	double _mixRightDry;
};

#endif
