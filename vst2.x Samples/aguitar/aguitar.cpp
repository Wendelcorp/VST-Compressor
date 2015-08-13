//
//								aguitar.cpp
//

#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef __aguitar__
#include "aguitar.h"
#endif

#define	ATTACK_MS		10
#define RELEASE_MS		250
#define ATTACK_RATE		(1.0 / (ATTACK_MS / 1000.0))
#define RELEASE_RATE 	(1.0 / (RELEASE_MS / 1000.0))
#define ATTACK_DELTA	(ATTACK_RATE / SAMPLE_RATE)
#define RELEASE_DELTA	(RELEASE_RATE / SAMPLE_RATE)


//----------------------------------------------------------------------------- 
AGuitarProgram::AGuitarProgram ()
{
	// default Program Values
	paramThreshhold = 0.05f;
	paramFilterGain = 0.5f;
	paramDistortionGain = 0.7f;
	paramOverdrive = 0.8f;
	strcpy (name, "Init");
}

//-----------------------------------------------------------------------------
AGuitar::AGuitar (audioMasterCallback audioMaster)
	: AudioEffectX (audioMaster, kNumPrograms, kNumParams)
{
	// init
	
	programs = new AGuitarProgram[numPrograms];
	//setThreshhold(0.3f);
	//setFilterGain(0.5f);
	//setDistortiongain(0.7f);
	//setOverdrive(0.8f);

	if (programs)
		setProgram (0);

	setNumInputs (2);	// fake stereo input
	setNumOutputs (2);	// fake stereo output

	_rectifiedValue = _rectifiedFilteredValue = _rectifiedX = 0.0;

	setUniqueID ('a67R');	// this should be unique, use the Steinberg web page for plugin Id registration

	resume ();		// flush buffer
}

//------------------------------------------------------------------------
AGuitar::~AGuitar ()
{
	if (programs)
		delete[] programs;
}

//------------------------------------------------------------------------
void AGuitar::setProgram (VstInt32 program)
{
	AGuitarProgram* ap = &programs[program];

	curProgram = program;
	setParameter (kThreshhold, ap->paramThreshhold);	
	setParameter (kFilterGain, ap->paramFilterGain);	
	setParameter (kDistortionGain, ap->paramDistortionGain);	
	setParameter (kOverdrive, ap->paramOverdrive);	
}

//------------------------------------------------------------------------
void AGuitar::setThreshhold (float v)
{
	_paramThreshhold = v;
	programs[curProgram].paramThreshhold = v;
	_threshhold = v;
}

//------------------------------------------------------------------------
void AGuitar::setFilterGain(float v)
{
	_paramFilterGain = v;
	programs[curProgram].paramFilterGain= v;
	_filterGain = v;
}
//------------------------------------------------------------------------
void AGuitar::setDistortionGain(float v)
{
	_paramDistortionGain = v;
	programs[curProgram].paramDistortionGain = v;
	_distortionGain = v;
}
//------------------------------------------------------------------------
void AGuitar::setOverdrive(float v)
{
	_paramOverdrive = v;
	programs[curProgram].paramOverdrive = v;
	_overdrive = v * 4.0f;
}


//------------------------------------------------------------------------
void AGuitar::setProgramName (char *name)
{
	strcpy (programs[curProgram].name, name);
}

//------------------------------------------------------------------------
void AGuitar::getProgramName (char *name)
{
	if (!strcmp (programs[curProgram].name, "Init"))
		sprintf (name, "%s %d", programs[curProgram].name, curProgram + 1);
	else
		strcpy (name, programs[curProgram].name);
}

//-----------------------------------------------------------------------------------------
bool AGuitar::getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text)
{
	if (index < kNumPrograms)
	{
		strcpy (text, programs[index].name);
		return true;
	}
	return false;
}

//------------------------------------------------------------------------
void AGuitar::resume ()
{
	AudioEffectX::resume ();
}

//------------------------------------------------------------------------
void AGuitar::setParameter (VstInt32 index, float value)
{
	AGuitarProgram* ap = &programs[curProgram];

	switch (index)
	{
		case kThreshhold:    
			setThreshhold(value);					
			break;
		case kFilterGain:	
			setFilterGain(value);					
			break;
		case kDistortionGain:
			setDistortionGain(value);
			break;
		case kOverdrive:
			setOverdrive(value);
			break;
	}
}

//------------------------------------------------------------------------
float AGuitar::getParameter (VstInt32 index)
{
	float v = 0;

	switch (index)
	{
		case kThreshhold :    
			v = _paramThreshhold;	
			break;
		case kFilterGain:
			v = _paramFilterGain;
			break;
		case kDistortionGain:
			v = _paramDistortionGain;
			break;
		case kOverdrive:
			v = _paramOverdrive;
			break;
	}
	return v;
}

//------------------------------------------------------------------------
void AGuitar::getParameterName (VstInt32 index, char *label)
{
	switch (index)
	{
		case kThreshhold:    
			strcpy (label, "Threshhold");		
			break;
		case kFilterGain:
			strcpy (label, "Filter Gain");
			break;
		case kDistortionGain:
			strcpy (label, "Distortion Gain");
			break;
		case kOverdrive:
			strcpy (label, "Overdrive");
			break;
	}
}

//------------------------------------------------------------------------
void AGuitar::getParameterDisplay (VstInt32 index, char *text)
{
	char buf[64];
	switch (index)
	{
		case kThreshhold:    
			sprintf( buf, "%2.1f", _threshhold);
			strcpy( text, buf );
			break;
		case kFilterGain:
			sprintf( buf, "%2.1f", _filterGain);
			strcpy( text, buf );
			break;
		case kDistortionGain:
			sprintf( buf, "%2.1f", _distortionGain );
			strcpy( text, buf);
			break;
		case kOverdrive:
			sprintf( buf, "%2.1f", _overdrive );
			break;
	}
}

//------------------------------------------------------------------------
void AGuitar::getParameterLabel (VstInt32 index, char *label)
{
	switch (index)
	{
		case kThreshhold:    
		case kFilterGain:
		case kDistortionGain:
		case kOverdrive:
			strcpy (label, "v");	
			break;
	}
}

//------------------------------------------------------------------------
bool AGuitar::getEffectName (char* name)
{
	strcpy (name, "Guitar Processor");
	return true;
}

//------------------------------------------------------------------------
bool AGuitar::getProductString (char* text)
{
	strcpy (text, "Guitar Processor");
	return true;
}

//------------------------------------------------------------------------
bool AGuitar::getVendorString (char* text)
{
	strcpy (text, "DarkVapor");
	return true;
}

//---------------------------------------------------------------------------
void AGuitar::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* in1 = inputs[0];
	float* in2 = inputs[1];
	float* out1 = outputs[0];
	float* out2 = outputs[1];
	double inval,absinval,filterOut,distortionOut;

	for( int i = 0; i < sampleFrames; i++ )
	{
		// rectify
		inval = absinval = in1[i] + in2[i];
		if( inval < 0 )
		{
			absinval = -inval;
		}
		if( absinval > _rectifiedValue )
		{
			_rectifiedValue = absinval;

			// filter it at about 20hz
			_rectifiedFilteredValue = 0.0028 * _rectifiedValue + 0.99715 * _rectifiedX;
			_rectifiedX = _rectifiedFilteredValue;
		}

		// hipass
		//filterOut = inval + _y1 * 0.75;
		//_y1 = filterOut;
		filterOut = inval;

		//// distortion
		//distortionOut = filterOut * 40.0f;
		//if(distortionOut >= 1.0f)
		//{
		//	distortionOut = 2.0f - 1.0f/distortionOut;
		//}
		//else if(distortionOut < -1.0f)
		//{
		//	distortionOut = -2.0f - 1.0f/distortionOut;
		//}
		//distortionOut /= 2.0f;

		//// blend
		//float distortionBias = _rectifiedValue - _paramThreshhold;
		//distortionBias *= 1.0f;
		//if( distortionBias < 0.0f )
		//{
		//	distortionBias = 0.0f;
		//}
		//if( distortionBias > 0.999f )
		//{
		//	distortionBias = 0.999f;
		//}
		//float filterBias = (1.0f - distortionBias) * 20.0f;

		double gain = 1.0;
		if( _rectifiedFilteredValue > _paramThreshhold )
		{
			gain = _paramThreshhold / _rectifiedFilteredValue;
		}
		filterOut *= gain;
		filterOut /= _paramThreshhold;
		filterOut /= 4.0;

		out1[i] = out2[i] = (float)filterOut;

		// output
		//out1[i] = out2[i] = distortionOut * distortionBias + filterOut * filterBias;

		// decay rectifier
		double delta = RELEASE_DELTA;
		_rectifiedValue -= delta;
		if( _rectifiedValue < 0.0 )
		{
			_rectifiedValue = 0.0;
		}
	}
}
