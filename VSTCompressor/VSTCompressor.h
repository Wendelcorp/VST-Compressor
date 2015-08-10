#include "audioeffectx.h"

#define NUM_PARAMS 0

class VSTCompressor : public AudioEffectX {
public:
	VSTCompressor(audioMasterCallback audioMaster);
	~VSTCompressor();

	void processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames);
};
