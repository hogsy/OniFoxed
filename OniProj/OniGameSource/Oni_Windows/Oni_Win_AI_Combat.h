#pragma once
/*
	FILE:	Oni_Win_Combat.h
	
	AUTHOR:	Michael Evans
	
	CREATED: May 5, 2000
	
	PURPOSE: AI Combat Windows
	
	Copyright (c) Bungie Software 2000

*/

#ifndef ONI_WIN_AI_COMBAT_H
#define ONI_WIN_AI_COMBAT_H

#include "BFW.h"
#include "BFW_WindowManager.h"

UUtUns16 OWrChooseCombat(UUtUns16 inCombatID);

UUtBool OWrEditCombat_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);


#endif // ONI_WIN_AI_COMBAT_H