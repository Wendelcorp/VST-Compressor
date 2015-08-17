#include "audioeffectx.h"

#ifndef __VST Compressor__
#define __VST Compressor__
#define NUM_PARAMS 0


class VSTCompressor : public AudioEffectX {
public:
	// Constructor and destructor
	VSTCompressor(audioMasterCallback audioMaster);
	~VSTCompressor();


	// Processing
	void processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames);

	// Program
	virtual void setProgramName(char* name);
	virtual void getProgramName(char* name);

protected:
	float* ratio;			// Compression ratio
	float* thresh;			// Threshold
	float* makeup;			// Make-up gain
	// Store control gains for L/R channels
	float* cntrl1;
	float* cntrl2;
	float* knee;		// Knee width

	// Program name
	char programName[kVstMaxProgNameLen + 1];
};

#endif