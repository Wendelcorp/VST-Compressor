#include "VSTCompressor.h"

AudioEffect* createEffectInstance(audioMasterCallback audioMaster) {
	return new VSTCompressor(audioMaster);
}

VSTCompressor::VSTCompressor(audioMasterCallback audioMaster) :
AudioEffectX(audioMaster, 0, NUM_PARAMS) {
}

VSTCompressor::~VSTCompressor() {
}

void VSTCompressor::processReplacing(float **inputs, float **outputs,
	VstInt32 sampleFrames) {
	// Real processing goes here
}
