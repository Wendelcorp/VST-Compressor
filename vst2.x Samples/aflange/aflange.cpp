//
//								aflange.cpp
//

#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef __aflange__
#include "aflange.h"
#endif

#ifndef __sdeditor__
#include "editor/sdeditor.h"
#endif

//
//								AFlangeProgram 
//
AFlangeProgram::AFlangeProgram ()
{
	// default Program Values
	paramSweepRate = 0.0f;
	paramWidth = 0.3f;
	paramFeedback = 0.0f;
	paramDelay = 0.2f;
	paramMixMode = 0.0f;
	strcpy (name, "Init");
}

//
//								AFlange 
//
AFlange::AFlange(audioMasterCallback audioMaster)
	: AudioEffectX (audioMaster, kNumPrograms, kNumParams)
{
	// init
	programs = new AFlangeProgram[numPrograms];

	// make sure all instance vars are init'd to some known value
	_paramSweepRate = 0.2f;
	_paramWidth = 0.3f;
	_paramFeedback = 0.0f;
	_paramDelay = 0.2f;
	_paramMixMode = 0.0f;
	_sweepRate = 0.2;
	_feedback = 0.0;
	_feedbackPhase = 1.0;
	_sweepSamples = 0;
	_delaySamples = 22;
	setSweep();
	_mixMode = 0;
	_fp = 0;
	_sweep = 0.0;
	_outval = 0.0f;
	_mixLeftWet =
	_mixLeftDry =
	_mixRightWet =
	_mixRightDry = 0.5f;

	// set initial program (defaults)
	if (programs)
		setProgram (0);

	setNumInputs (2);	// pseudo-mono input
	setNumOutputs (2);	// pseudo-mono output

	setUniqueID ('mm91');	// this should be unique, use the Steinberg web page for plugin Id registration

	// create the editor
	editor = new SDEditor (this);

	// allocate the buffer
	_buf = new double[BSZ];

	resume ();		// flush buffer
}

//
//								~AFlange 
//
AFlange::~AFlange ()
{
	if (programs)
		delete[] programs;
	if( _buf )
		delete[] _buf;
}

//
//								setProgram
//
void AFlange::setProgram (VstInt32 program)
{
	AFlangeProgram* ap = &programs[program];

	curProgram = program;
	setParameter (kRate, ap->paramSweepRate);	
	setParameter (kWidth, ap->paramWidth);
	setParameter (kFeedback, ap->paramFeedback);
	setParameter (kDelay, ap->paramDelay);
	setParameter (kMixMode, ap->paramMixMode);
}

//
//								setRate
//
void AFlange::setRate (float rate)
{
	_paramSweepRate = rate;
	programs[curProgram].paramSweepRate = rate;

	// map into param onto desired sweep range with log curve
	_sweepRate = pow(10.0,(double)_paramSweepRate);
	_sweepRate  -= 1.0;
	_sweepRate  *= 1.1f;
	_sweepRate  += 0.1f;

	// finish setup
	setSweep();
}


//
//								setWidth
//
//	Maps 0.0-1.0 input to calculated width in samples from 0ms to 10ms
//
void AFlange::setWidth (float v)
{
	_paramWidth = v;
	programs[curProgram].paramWidth = v;

	// map so that we can spec between 0ms and 50ms
	_sweepSamples = ROUND(v * 0.01 * SAMPLE_RATE);

	// finish setup
	setSweep();
}

//
//								setDelay
//
//	Expects input from 0.0, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875
//
void AFlange::setDelay (float v)
{
	_paramDelay = v;
	programs[curProgram].paramDelay = v;

	// convert incoming float to int for switch
	int iv = ROUND(v * NUM_DELAYS);
	switch(iv)
	{
	case 0:	// 0 ms
		_delaySamples = 1;
		break;
	case 1:	// .1 ms
		_delaySamples = 4;
		break;
	case 2:	// .2 ms
		_delaySamples = 9;
		break;
	case 3:	// .5 ms
		_delaySamples = 22;
		break;
	case 4:	// 1 ms
	default:
		_delaySamples = 44;
		break;
	case 5:	// 2 ms
		_delaySamples = 88;
		break;
	case 6:	// 5 ms
		_delaySamples = 220;
		break;
	case 7:	// 10 ms
		_delaySamples = 441;
		break;
	}

	// finish setup
	setSweep();
}

//
//								setSweep
//
//	Sets up sweep based on rate, depth, and delay as they're all interrelated
//	Assumes _sweepRate, _sweepSamples, and _delaySamples have all been set by
//	setRate, setWidth, and setDelay
//
void AFlange::setSweep()
{
	// calc # of samples per second we'll need to move to achieve spec'd sweep rate
	_step = (double)(_sweepSamples * 2.0 * _sweepRate) / (double)SAMPLE_RATE;
	if( _step <= 1.0 )
	{
		printf( "_step out of range: %f\n", _step );
	}

	// calc min and max sweep now
	_minSweepSamples = _delaySamples;
	_maxSweepSamples = _delaySamples + _sweepSamples;

	// set intial sweep pointer to midrange
	_sweep = (_minSweepSamples + _maxSweepSamples) / 2;
}


//
//								setFeedback
//
void AFlange::setFeedback(float v)
{
	_paramFeedback = v;
	programs[curProgram].paramFeedback = v;
	_feedback = v;
}


//
//								setMixMode
//
//	Expects input 0.0, 0.2, 0.4, 0.6, 0.8 and maps to the five supported mix modes
//
void AFlange::setMixMode (float v)
{
	_paramMixMode = v;
	programs[curProgram].paramMixMode = v;
	_mixMode = (int)(v * NUM_MIX_MODES);
	switch(_mixMode)
	{
	case kMixMono:
	default:
		_mixLeftWet = _mixRightWet = 0.5f;
		_mixLeftDry = _mixRightDry = 0.5f;
		_feedbackPhase = 1.0;
		break;
	case kMixMonoMinus:
		_mixLeftWet = _mixRightWet = 0.5f;
		_mixLeftDry = _mixRightDry = -0.5f;
		_feedbackPhase = -1.0;
		break;
	case kMixStereo:
		_mixLeftWet = 0.5f;
		_mixLeftDry = 0.5f;
		_mixRightWet = -0.5f;
		_mixRightDry = 0.5f;
		break;
	}
}



//
//								setProgramName
//
void AFlange::setProgramName (char *name)
{
	strcpy (programs[curProgram].name, name);
}

//
//								getProgramName
//
void AFlange::getProgramName (char *name)
{
	if (!strcmp (programs[curProgram].name, "Init"))
		sprintf (name, "%s %d", programs[curProgram].name, curProgram + 1);
	else
		strcpy (name, programs[curProgram].name);
}

//
//								getProgramNameIndexed
//
bool AFlange::getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text)
{
	if (index < kNumPrograms)
	{
		strcpy (text, programs[index].name);
		return true;
	}
	return false;
}

//
//								resume
//
void AFlange::resume ()
{
	AudioEffectX::resume ();
}

//
//								setParameter
//
void AFlange::setParameter (VstInt32 index, float value)
{
	AFlangeProgram* ap = &programs[curProgram];

	switch (index)
	{
		case kRate:    setRate(value);					break;
		case kWidth:   setWidth(value);					break;
		case kFeedback: setFeedback(value);				break;
		case kDelay:	setDelay(value);				break;
		case kMixMode: setMixMode(value);				break;
	}
}

//
//								getParameter
//
float AFlange::getParameter (VstInt32 index)
{
	float v = 0;

	switch (index)
	{
		case kRate :    v = _paramSweepRate;	break;
		case kWidth:	v = _paramWidth;		break;
		case kFeedback: v = _paramFeedback;		break;
		case kDelay:	v = _paramDelay;		break;
		case kMixMode:	v = _paramMixMode;		break;
	}
	return v;
}

//
//								getParameterName
//
void AFlange::getParameterName (VstInt32 index, char *label)
{
	switch (index)
	{
		case kRate:    strcpy (label, "Rate");			break;
		case kWidth:   strcpy (label, "Width");			break;
		case kFeedback: strcpy(label, "Feedback");		break;
		case kDelay:	strcpy (label, "Delay");		break;
		case kMixMode: strcpy (label, "Mix Mode");		break;
	}
}

//
//								getParameterDisplay
//
void AFlange::getParameterDisplay (VstInt32 index, char *text)
{
	char buf[64];
	switch (index)
	{
	case kRate:    
		sprintf( buf, "%2.1f", _sweepRate );
		strcpy( text, buf );
		break;
	case kWidth:
		sprintf( buf, "%2.0f", _paramWidth * 100.0 );
		strcpy( text, buf );
		break;
	case kFeedback:
		sprintf( buf, "%2.0f", _feedback * 100.0 );
		strcpy( text,buf );
		break;
	case kDelay:
		sprintf( buf, "%d", _paramDelay);
		break;
	case kMixMode:
		switch(_mixMode)
		{
		case kMixMono:
			strcpy( text, "mono" );
			break;
		case kMixMonoMinus:
			strcpy( text, "mono-" );
			break;
		case kMixStereo:
			strcpy( text, "stereo" );
			break;
		}
		break;
	}
}

//
//								getParameterLabel
//
void AFlange::getParameterLabel (VstInt32 index, char *label)
{
	switch (index)
	{
		case kRate:		strcpy (label, "Hz");		break;
		case kWidth:	strcpy( label, "%");		break;
		case kFeedback:	strcpy( label, "%");		break;
		case kDelay:	strcpy( label, "");			break;
		case kMixMode:	strcpy( label, "");			break;
	}
}

//
//								getEffectName
//
bool AFlange::getEffectName (char* name)
{
	strcpy (name, "Flange");
	return true;
}

//
//								getProductString
//
bool AFlange::getProductString (char* text)
{
	strcpy (text, "Flange");
	return true;
}

//
//								getVendorString
//
bool AFlange::getVendorString (char* text)
{
	strcpy (text, "DarkVapor");
	return true;
}

//
//								processReplacing
//
void AFlange::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* in1 = inputs[0];
	float* in2 = inputs[1];
	float* out1 = outputs[0];
	float* out2 = outputs[1];

	for( int i = 0; i < sampleFrames; i++ )
	{
		// assemble mono input value and store it in circle queue
		float inval = (in1[i] + in2[i]) / 2.0f;
		double inmix = inval + _feedback * _feedbackPhase * _outval;

		_buf[_fp] = inmix;
		_fp = (_fp + 1) & (BSZ-1);

		// build the two emptying pointers and do linear interpolation
		int ep1, ep2;
		double w1, w2;
		double ep = _fp - _sweep;
		MODF(ep, ep1, w2);
		ep1 &= (BSZ-1);
		ep2 = ep1 + 1;
		ep2 &= (BSZ-1);
		w1 = 1.0 - w2;
		_outval = _buf[ep1] * w1 + _buf[ep2] * w2;

		// develop output mix
		out1[i] = (float)(_mixLeftDry * inval + _mixLeftWet * _outval);
		out2[i] = (float)(_mixRightDry * inval + _mixRightWet * _outval);

		// increment the sweep
		_sweep += _step;
		if( _sweep >= _maxSweepSamples || _sweep <= _minSweepSamples )
		{
			_step = -_step;
		}

	}

}
