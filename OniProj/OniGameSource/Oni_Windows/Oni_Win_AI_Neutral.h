#pragma once
/*
	FILE:	Oni_Win_Neutral.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: July 13, 2000
	
	PURPOSE: AI Neutral Windows
	
	Copyright (c) Bungie Software 2000

*/

#ifndef ONI_WIN_AI_NEUTRAL_H
#define ONI_WIN_AI_NEUTRAL_H

#include "BFW.h"
#include "BFW_WindowManager.h"

UUtUns16 OWrChooseNeutral(UUtUns16 inNeutralID);

UUtBool OWrEditNeutral_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);


#endif // ONI_WIN_AI_NEUTRAL_H
