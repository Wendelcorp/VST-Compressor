#include "audioeffectx.h"

#define NUM_PARAMS 0

class VSTCompressor : public AudioEffectX {
public:
	VSTCompressor(audioMasterCallback audioMaster);
	~VSTCompressor();

	void processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames);

protected:
	float* ratio;			// Compression ratio
	float* thresh;			// Threshold
	float* makeup;			// Make-up gain
	// Store control gains for L/R channels
	float* cntrl1;
	float* cntrl2;
	float* knee;		// Knee width


};
