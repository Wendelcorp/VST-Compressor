//
//								apitchmain.cpp
//

#ifndef __apitch__
#include "apitch.h"
#endif

//-------------------------------------------------------------------------------------------------------
AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return new APitch (audioMaster);
}

