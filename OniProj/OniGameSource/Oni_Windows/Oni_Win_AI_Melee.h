#pragma once
#ifndef ONI_WIN_AI_MELEE_H
#define ONI_WIN_AI_MELEE_H

/*
	FILE:	Oni_Win_AI_Melee.h

	AUTHOR:	Chris Butcher

	CREATED: May 20, 2000

	PURPOSE: AI Melee Profile Editor

	Copyright (c) Bungie Software 2000

*/

#include "BFW.h"
#include "BFW_WindowManager.h"

UUtUns16 OWrChooseMelee(UUtUns16 inProfileID);

UUtBool OWrEditMelee_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);


#endif // ONI_WIN_AI_MELEE_H
