#include "VSTCompressor.h"

//Function to create an instance of the effect
AudioEffect* createEffectInstance(audioMasterCallback audioMaster) {
	return new VSTCompressor(audioMaster);
}

// Constructor
VSTCompressor::VSTCompressor(audioMasterCallback audioMaster)
	: AudioEffectX(audioMaster, 1, NUM_PARAMS)	// 1 program, 0 parameters
{
	setNumInputs(2);		// stereo in
	setNumOutputs(2);		// stereo out
	setUniqueID('Comp');	// identify
	canProcessReplacing();	// supports replacing output
	canDoubleReplacing();	// supports double precision processing

	//fGain = 1.f;			// default to 0 dB
	// Parameters
	ratio = 10.0;
	thresh = 0.05;
	makeup = 1.6;
	cntrl1 = 1.0;
	cntrl2 = 1.0;
	knee = 0.0;

	vst_strncpy(programName, "Default", kVstMaxProgNameLen);	// default program name
}


// Default destructor
VSTCompressor::~VSTCompressor() {
}

// Set Program Name Function
void VSTCompressor::setProgramName(char* name)
{
	vst_strncpy(programName, name, kVstMaxProgNameLen);
}

// Get Program Name function
void VSTCompressor::getProgramName(char* name)
{
	vst_strncpy(name, programName, kVstMaxProgNameLen);
}

// Proccessing function
void VSTCompressor::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* in1 = inputs[1];
	float* in2 = inputs[0];
	float* out1 = outputs[1];
	float* out2 = outputs[0];

	for (int i = 0; i < sampleFrames; i++)
	{
		
		// Set control voltages
		if (in1[i] > thresh){
			cntrl1 = thresh + ((in1[i] - thresh)/ratio);		// New gain equals threshold plus difference between input and threshold divided by the ratio
		}
		else { cntrl1 = in1[i]; }


		if (in2[i] > thresh){
			cntrl2 = thresh + ((in2[i] - thresh) / ratio);
		}
		else { cntrl2 = in2[i]; }


		// develop output mix
		out1[i] = (-1) * cntrl1 * makeup;
		out2[i] = (-1) * cntrl2 * makeup;
		/*
		// Increment inputs and outputs to next frame
		*in1++;
		*in2++;
		*out1++;
		*out2++;
		*/
	}
}


/*


VSTCompressor::VSTCompressor(audioMasterCallback audioMaster) :
AudioEffectX(audioMaster, 0, NUM_PARAMS) {
	*ratio = 10.0;
	*thresh = 0.5;
	*makeup = 0.0;
	*cntrl1 = 1.0;
	*cntrl2 = 1.0;
	*knee = 0.0;

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
*/