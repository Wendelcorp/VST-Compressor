//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/11/13 09:08:28 $
//
// Category     : VST 2.x SDK Samples
// Filename     : sdeditor.cpp
// Created by   : Steinberg Media Technologies
// Description  : Simple Surround Delay plugin with Editor using VSTGUI
//
// © 2006, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#ifndef __sdeditor__
#include "sdeditor.h"
#endif

#ifndef __adelay__
#include "../apitch.h"
#endif

#include <stdio.h>

//-----------------------------------------------------------------------------
// resource id's
enum {
	// bitmaps
	kBackgroundId = 128,
	kFaderBodyId,
	kFaderHandleId,
	
	// fader positions
	kFaderX = 56,
	kFaderY = 61,
	kFaderInc = 68,

	// multi-select positions
	kSelectX = 110,
	kSelectY = 240,
	kSelectWidth = 80,
	kSelectHeight = 20,
	kSelectInc = 30,

	kDisplayX = 10,
	kDisplayY = 184,
	kDisplayXWidth = 30,
	kDisplayHeight = 14
};

//-----------------------------------------------------------------------------
// prototype string convert float -> percent
void percentStringConvert (float value, char* string);
void percentStringConvert (float value, char* string)
{
	 sprintf (string, "%d%%", (int)(100 * value + 0.5f));
}


//
//								SDEditor
//
SDEditor::SDEditor (AudioEffect *effect)
 : AEffGUIEditor (effect) 
{
	finePitchFader    = 0;
	delayFader = 0;
	feedbackFader   = 0;
	coarsePitchOption = 0;
	mixOption = 0;

	// load the background bitmap
	// we don't need to load all bitmaps, this could be done when open is called
	hBackground = new CBitmap (kBackgroundId);

	// init the size of the plugin
	rect.left   = 0;
	rect.top    = 0;
	rect.right  = (short)hBackground->getWidth ();
	rect.bottom = (short)hBackground->getHeight ();
}

//
//								~SDEditor
//
SDEditor::~SDEditor ()
{
	// free the background bitmap
	if (hBackground)
		hBackground->forget ();
	hBackground = 0;
}

//
//								open
//
bool SDEditor::open (void *ptr)
{
	// !!! always call this !!!
	AEffGUIEditor::open (ptr);
	
	//--load some bitmaps
	CBitmap* hFaderBody   = new CBitmap (kFaderBodyId);
	CBitmap* hFaderHandle = new CBitmap (kFaderHandleId);

	//--init background frame-----------------------------------------------
	// We use a local CFrame object so that calls to setParameter won't call into objects which may not exist yet. 
	// If all GUI objects are created we assign our class member to this one. See bottom of this method.
	CRect size (0, 0, hBackground->getWidth (), hBackground->getHeight ());
	CFrame* lFrame = new CFrame (size, ptr, this);
	lFrame->setBackground (hBackground);

	//--init the faders------------------------------------------------
	int minPos = kFaderY;
	int maxPos = kFaderY + hFaderBody->getHeight () - hFaderHandle->getHeight () - 1;
	CPoint point (0, 0);
	CPoint offset (1, 0);

	// fine pitch
	size (kFaderX, kFaderY,
          kFaderX + hFaderBody->getWidth (), kFaderY + hFaderBody->getHeight ());
	finePitchFader = new CVerticalSlider (size, this, kFinePitch, minPos, maxPos, hFaderHandle, hFaderBody, point);
	finePitchFader->setOffsetHandle (offset);
	finePitchFader->setValue (effect->getParameter (kFinePitch));
	lFrame->addView (finePitchFader);

	// delay
	size.offset (kFaderInc + hFaderBody->getWidth (), 0);
	delayFader = new CVerticalSlider (size, this, kDelay, minPos, maxPos, hFaderHandle, hFaderBody, point);
	delayFader->setOffsetHandle (offset);
	delayFader->setValue (effect->getParameter (kDelay));
	lFrame->addView (delayFader);

	// feedback
	size.offset (kFaderInc + hFaderBody->getWidth (), 0);
	feedbackFader = new CVerticalSlider (size, this, kFeedback, minPos, maxPos, hFaderHandle, hFaderBody, point);
	feedbackFader->setOffsetHandle (offset);
	feedbackFader->setValue (effect->getParameter (kFeedback));
	lFrame->addView (feedbackFader);

	// Coarse pitch shift in steps
	size (kSelectX, kSelectY,
          kSelectX + kSelectWidth, kSelectY + kSelectHeight);
	coarsePitchOption = new COptionMenu(size, this, kCoarsePitch);
	coarsePitchOption->addEntry("+12");
	coarsePitchOption->addEntry("+11");
	coarsePitchOption->addEntry("+10");
	coarsePitchOption->addEntry("+9");
	coarsePitchOption->addEntry("+8");
	coarsePitchOption->addEntry("+7");
	coarsePitchOption->addEntry("+6");
	coarsePitchOption->addEntry("+5");
	coarsePitchOption->addEntry("+4");
	coarsePitchOption->addEntry("+3");
	coarsePitchOption->addEntry("+2");
	coarsePitchOption->addEntry("+1");
	coarsePitchOption->addEntry("0");
	coarsePitchOption->addEntry("-1");
	coarsePitchOption->addEntry("-2");
	coarsePitchOption->addEntry("-3");
	coarsePitchOption->addEntry("-4");
	coarsePitchOption->addEntry("-5");
	coarsePitchOption->addEntry("-6");
	coarsePitchOption->addEntry("-7");
	coarsePitchOption->addEntry("-8");
	coarsePitchOption->addEntry("-9");
	coarsePitchOption->addEntry("-10");
	coarsePitchOption->addEntry("-11");
	coarsePitchOption->addEntry("-12");
	coarsePitchOption->setCurrent(ROUND((effect->getParameter(kCoarsePitch)*NUM_PITCHES)));
	lFrame->addView (coarsePitchOption);

	// Mix mode
	size.offset( 0, kSelectInc );
	mixOption = new COptionMenu(size, this, kMixMode);
	mixOption->addEntry("mono");
	mixOption->addEntry("wet only");
	mixOption->addEntry("wet left");
	mixOption->addEntry("wet right");
	mixOption->addEntry("wet part left");
	mixOption->addEntry("wet part right");
	mixOption->addEntry("stereo");
	mixOption->setCurrent((int)(effect->getParameter(kMixMode)*NUM_MIX_MODES));
	lFrame->addView (mixOption);

	// Note : in the constructor of a CBitmap, the number of references is set to 1.
	// Then, each time the bitmap is used (for hinstance in a vertical slider), this
	// number is incremented.
	// As a consequence, everything happens as if the constructor by itself was adding
	// a reference. That's why we need til here a call to forget ().
	// You mustn't call delete here directly, because the bitmap is used by some CControls...
	// These "rules" apply to the other VSTGUI objects too.
	hFaderBody->forget ();
	hFaderHandle->forget ();

	frame = lFrame;
	return true;
}

//
//								close
//
void SDEditor::close ()
{
	delete frame;
	frame = 0;
}

//
//								setParameter
//
void SDEditor::setParameter (VstInt32 index, float value)
{
	if (frame == 0)
		return;

	// called from ADelayEdit
	switch (index)
	{
	case kFinePitch:
		if (finePitchFader)
			finePitchFader->setValue (effect->getParameter (index));
		break;

	case kDelay:
		if (delayFader)
			delayFader->setValue (effect->getParameter (index));
		break;

	case kFeedback:
		if(feedbackFader)
			feedbackFader->setValue (effect->getParameter (index));
		break;

	case kCoarsePitch:
		if (coarsePitchOption)
			coarsePitchOption->setValue (effect->getParameter (index)* NUM_PITCHES);
		break;
	
	case kMixMode:
		if (mixOption)
			mixOption->setValue (effect->getParameter (index));
		break;
	}
}

//
//								valueChanged
//
void SDEditor::valueChanged (CDrawContext* context, CControl* control)
{
	long tag = control->getTag ();
	switch (tag)
	{
	case kFinePitch:
	case kDelay:
	case kFeedback:
		effect->setParameterAutomated (tag, control->getValue ());
		control->setDirty ();
		break;

	case kCoarsePitch:
		effect->setParameterAutomated (tag, control->getValue () / NUM_PITCHES);
		control->setDirty ();
		break;

	case kMixMode:
		effect->setParameterAutomated (tag, control->getValue ()/NUM_MIX_MODES);
		control->setDirty ();
		break;
	}
}

