//
//								apitch.cpp
//

#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef __apitch__
#include "apitch.h"
#endif

#ifndef __sdeditor__
#include "editor/sdeditor.h"
#endif

//
//								APitchProgram 
//
APitchProgram::APitchProgram ()
{
	// default Program Values
	paramFinePitch = 0.5f;
	paramCoarsePitch = 0.48f;
	paramFeedback = 0.0f;
	paramDelay = 0.0f;
	paramMixMode = 0.0f;
	strcpy (name, "Init");
}

//
//								APitch 
//
APitch::APitch(audioMasterCallback audioMaster)
	: AudioEffectX (audioMaster, kNumPrograms, kNumParams)
{
	// init
	programs = new APitchProgram[numPrograms];

	// make sure all instance vars are init'd to some known value
	_paramFinePitch = 0.5f;
	_paramCoarsePitch = 0.48f;
	_paramFeedback = 0.0f;
	_paramDelay = 0.0f;
	_paramMixMode = 0.0f;
	_sweepRate = 0.2;
	_feedback = 0.0;
	_feedbackPhase = 1.0;
	_sweepSamples = 0;
	_delaySamples = 22;
	_fadeLookup = NULL;
	_sweepSteps = 0;
	_sweepInc = 0.0;
	setSweep();
	_mixMode = 0;
	_fp = 0;

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

	setUniqueID ('qr7e');	// this should be unique, use the Steinberg web page for plugin Id registration

	// create the editor
	editor = new SDEditor (this);

	// allocate the buffer
	_buf = new double[BSZ];

	// init cross fade lookup tables
	initFadeLookup();

	resume ();		// flush buffer
}

//
//								~APitch 
//
APitch::~APitch ()
{
	if (programs)
		delete[] programs;
	if( _buf )
		delete[] _buf;
}

//
//								setProgram
//
void APitch::setProgram (VstInt32 program)
{
	APitchProgram* ap = &programs[program];

	curProgram = program;
	setParameter (kFinePitch, ap->paramFinePitch);	
	setParameter (kCoarsePitch, ap->paramCoarsePitch);
	setParameter (kFeedback, ap->paramFeedback);
	setParameter (kDelay, ap->paramDelay);
	setParameter (kMixMode, ap->paramMixMode);
}

//
//								setFine
//
//	Sets the fine pitch shift amount
//
void APitch::setFine(float finePitch)
{
	_paramFinePitch = finePitch;
	programs[curProgram].paramFinePitch = finePitch;

	// finish setup
	setSweep();
}


//
//								setCoarse
//
//	Maps 0.0-1.0 input to calculated width in samples from 0ms to 10ms
//
void APitch::setCoarse (float v)
{
	_paramCoarsePitch = v;
	programs[curProgram].paramCoarsePitch = v;

	// finish setup
	setSweep();
}

//
//								setDelay
//
//	Map to the expect 0-100ms range.
//
void APitch::setDelay (float v)
{
	_paramDelay = v;
	programs[curProgram].paramDelay = v;

	// convert incoming float to int sample count for up to 100ms of delay
	_delaySamples = (int)(0.1 * SAMPLE_RATE * v);

	// pin to a minimum value so our sweep never collides with the filling pointer
	_delaySamples = _delaySamples < 2 ? 2 : _delaySamples;

	// finish setup
	setSweep();
}

//
//								setSweep
//
//	Sets up sweep based on pitch delta and delay as they're interrelated
//	Assumes _paramFinePitch, _paramCoarsePitch, and _delaySamples have all been set by
//	setFine, setCoarse, and setDelay
//
void APitch::setSweep()
{
	// calc the total pitch delta
	double semiTones = 12.0 - ROUND(_paramCoarsePitch * NUM_PITCHES);
	double fine = (2.0 * _paramFinePitch) - 1.0;
	double pitchDelta = pow(STEP,semiTones + fine);

	// see if we're increasing or decreasing
	_increasing = pitchDelta >= 1.0;

	// calc the # of samples in the sweep, 15ms minimum, scaled up to 50ms for an octave
	double absDelta = pitchDelta >= 1.0 ? pitchDelta : 1.0 pitchDelta;
	_sweepSamples = (int)((0.015 + 0.035 * absDelta) * SWEEP_RATE);
	int min = (int)(SWEEP_RATE * 0.015);
	int max = (int)(SWEEP_RATE * 0.05);
	_sweepSamples = PIN(_sweepSamples,min,max);

	// fix up the pitchDelta to become the _sweepInc
	_sweepInc = pitchDelta - 1.0;

	// assign initial pointers, step values
	int sweepStart = _increasing ? _delaySamples + _sweepSamples : _delaySamples;


	// init store and read ptrs to known value, chanA active 1st
    fp = ep3 = ep4 = xfade_cnt = 0;
    sweep.l = 0;
    if(sweep_up)
        ep1 = ep2 = min_sweep;
    else
        ep1 = ep2 = max_sweep;
    active_cnt = active;
    blendA = 1.0;
    blendB = 0.0;
    fadeA = fade_out;
    fadeB = fade_in;
    chanA = TRUE;
}

//
//								initFadeLookup
//
//	Inits the cross-fade lookup table with a fixed fade time XFADE
//
void APitch::initFadeLookup()
{
	// fill the table with sin values for interval 0 - PI yielding 0-1 and back range
	for( int i = 0; i < XFADE; i++ )
	{
		double theta = (double)i * M_PI / (double)XFADE;
		double v = cos(theta);
		_fadeIn[i] = v;
		v = sin(theta);
		_fadeOut[i] = v;
	}
}

//
//								setFeedback
//
void APitch::setFeedback(float v)
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
void APitch::setMixMode (float v)
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
	case kMixWetOnly:
		_mixLeftWet = _mixRightWet = 1.0f;
		_mixLeftDry = _mixRightDry = 0.0f;
		_feedbackPhase = 1.0;
		break;
	case kMixWetLeft:
		_mixLeftWet = _mixRightDry = 1.0;
		_mixRightWet = _mixLeftDry = 0.0;
		_feedbackPhase = 1.0;
		break;
	case kMixWetRight:
		_mixLeftWet = _mixRightDry = 0.0;
		_mixRightWet = _mixLeftDry = 1.0;
		_feedbackPhase = 1.0;
		break;
	case kMixWetLeftish:
		_mixLeftWet = _mixRightDry = 0.87;
		_mixRightWet = _mixLeftDry = 0.13;
		_feedbackPhase = 1.0;
		break;
	case kMixWetRightish:
		_mixLeftWet = _mixRightDry = 0.13;
		_mixRightWet = _mixLeftDry = 0.87;
		_feedbackPhase = 1.0;
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
void APitch::setProgramName (char *name)
{
	strcpy (programs[curProgram].name, name);
}

//
//								getProgramName
//
void APitch::getProgramName (char *name)
{
	if (!strcmp (programs[curProgram].name, "Init"))
		sprintf (name, "%s %d", programs[curProgram].name, curProgram + 1);
	else
		strcpy (name, programs[curProgram].name);
}

//
//								getProgramNameIndexed
//
bool APitch::getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text)
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
void APitch::resume ()
{
	AudioEffectX::resume ();
}

//
//								setParameter
//
void APitch::setParameter (VstInt32 index, float value)
{
	APitchProgram* ap = &programs[curProgram];

	switch (index)
	{
		case kFinePitch:    setFine(value);				break;
		case kCoarsePitch:   setCoarse(value);			break;
		case kFeedback: setFeedback(value);				break;
		case kDelay:	setDelay(value);				break;
		case kMixMode: setMixMode(value);				break;
	}
}

//
//								getParameter
//
float APitch::getParameter (VstInt32 index)
{
	float v = 0;

	switch (index)
	{
		case kFinePitch:    v = _paramFinePitch;	break;
		case kCoarsePitch:	v = _paramCoarsePitch;	break;
		case kFeedback: v = _paramFeedback;			break;
		case kDelay:	v = _paramDelay;			break;
		case kMixMode:	v = _paramMixMode;			break;
	}
	return v;
}

//
//								getParameterName
//
void APitch::getParameterName (VstInt32 index, char *label)
{
	switch (index)
	{
		case kFinePitch:    strcpy (label, "Fine Pitch");			break;
		case kCoarsePitch:   strcpy (label, "Coarse Pitch");		break;
		case kFeedback: strcpy(label, "Feedback");					break;
		case kDelay:	strcpy (label, "Delay");					break;
		case kMixMode: strcpy (label, "Mix Mode");					break;
	}
}

//
//								getParameterDisplay
//
void APitch::getParameterDisplay (VstInt32 index, char *text)
{
	char buf[64];
	switch (index)
	{
	case kFinePitch:    
		sprintf( buf, "%f", _paramFinePitch);
		strcpy( text, buf );
		break;
	case kCoarsePitch:
		sprintf( buf, "%f", _paramCoarsePitch);
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
		case kMixWetOnly:
			strcpy( text, "mono wet only" );
			break;
		case kMixWetLeft:
			strcpy( text, "wet left" );
			break;
		case kMixWetRight:
			strcpy( text, "wet right" );
			break;
		case kMixWetLeftish:
			strcpy( text, "wet part left" );
			break;
		case kMixWetRightish:
			strcpy( text, "wet part right" );
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
void APitch::getParameterLabel (VstInt32 index, char *label)
{
	switch (index)
	{
		case kFinePitch:		strcpy (label, "");			break;
		case kCoarsePitch:	strcpy( label, "semi-tones");	break;
		case kFeedback:	strcpy( label, "%");				break;
		case kDelay:	strcpy( label, "");					break;
		case kMixMode:	strcpy( label, "");					break;
	}
}

//
//								getEffectName
//
bool APitch::getEffectName (char* name)
{
	strcpy (name, "Pitch");
	return true;
}

//
//								getProductString
//
bool APitch::getProductString (char* text)
{
	strcpy (text, "Pitch");
	return true;
}

//
//								getVendorString
//
bool APitch::getVendorString (char* text)
{
	strcpy (text, "DarkVapor");
	return true;
}

//
//								processReplacing
//
void APitch::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* in1 = inputs[0];
	float* in2 = inputs[1];
	float* out1 = outputs[0];
	float* out2 = outputs[1];

	// iterate thru sample frames
	for( int i = 0; i < sampleFrames; i++ )
	{
		// assemble mono input value and store it in circle queue
		float inval = (in1[i] + in2[i]) / 2.0f;
		double inmix = inval + _feedback * _feedbackPhase * _outval;

		_buf[_fp] = inmix;
		_fp = (_fp + 1) & (BSZ-1);

		// iterate thru the taps
		_outval = 0.0;
		for( int itap = 0; itap < NUM_TAPS; itap++ )
		{
			Tap *tap = &_taps[itap];

			// build the two emptying pointers and do linear interpolation
			int ep1, ep2;
			double w1, w2;
			double ep = _fp - tap->SweepValue;
			MODF(ep, ep1, w2);
			ep1 &= (BSZ-1);
			ep2 = ep1 + 1;
			ep2 &= (BSZ-1);
			w1 = 1.0 - w2;
			double tapout = _buf[ep1] * w1 + _buf[ep2] * w2;

			tapout *= _fadeLookup[tap->Step];
			_outval += tapout;

			// step the sweep
			tap->SweepValue += _sweepInc;
			tap->Step++;
			if( tap->Step >= _sweepSteps )
			{
				tap->Step = 0;
				tap->SweepValue = _increasing? _delaySamples + _sweepSamples : _delaySamples;
			}
		}
		_outval /= NUM_TAPS;

		// develop output mix
		out1[i] = (float)(_mixLeftDry * inval + _mixLeftWet * _outval);
		out2[i] = (float)(_mixRightDry * inval + _mixRightWet * _outval);

	} // end of loop iterating thru sample frames
}
