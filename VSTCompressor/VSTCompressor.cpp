#include "VSTCompressor.h"

AudioEffect* createEffectInstance(audioMasterCallback audioMaster) {
	return new VSTCompressor(audioMaster);
}

VSTCompressor::VSTCompressor(audioMasterCallback audioMaster) :
AudioEffectX(audioMaster, 0, NUM_PARAMS) {
	*ratio = 10.0;
	*thresh = 0.5;
	*makeup = 0.0;
	*cntrl1 = 1.0;
	*cntrl2 = 1.0;
	*knee = 0.0;

}

VSTCompressor::~VSTCompressor() {
}

void VSTCompressor::processReplacing(float **inputs, float **outputs,
	VstInt32 sampleFrames) {
	// Real processing goes here

	// Setup inputs/outputs
	float* in1 = inputs[0];
	float* in2 = inputs[1];
	float* out1 = outputs[0];
	float* out2 = outputs[1];

	// set control voltage for channel1
	if (*in1 > *thresh){
		*cntrl1 = *thresh * ((*in1 - *thresh) / (*ratio));
	}

	else { *cntrl1 = 1.0; }

	// Control voltage for channel 2
	if (*in2 > *thresh){
		*cntrl2 = *thresh * ((*in2 - *thresh) / (*ratio));
	}

	else { *cntrl2 = 1.0; }

	



	// set new outputs
	*out1 = (*in1) * (*cntrl1);
	*out2 = (*in2) * (*cntrl2);
}
