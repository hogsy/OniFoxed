#pragma once
/*
	FILE:	Oni_Win_AI.h

	AUTHOR:	Michael Evans

	CREATED: April 11, 2000

	PURPOSE: AI Windows

	Copyright (c) Bungie Software 2000

*/
#ifndef ONI_WIN_AI_H
#define ONI_WIN_AI_H

#include "BFW.h"
#include "WM_Menu.h"
#include "WM_PopupMenu.h"
#include "Oni_AI2_Patrol.h"

extern UUtBool			OWgShowPatrolPath;
extern UUtBool			OWgCurrentPatrolPathValid;
extern UUtUns16			OWgCurrentPatrolPathID;
extern AI2tPatrolPath	*OWgCurrentPatrolPath;

void OWrAI_Menu_HandleMenuCommand(WMtMenu *inMenu, UUtUns16 menu_item_id);
void OWrAI_Menu_Init(WMtMenu *inMenu);

UUtBool
OWrPath_Edit_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrProp_Char_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrAI_DropAndAddFlag(
	void);

UUtInt16 OWrChooseFlag(UUtInt16 inFlagID);
UUtUns16 OWrChoosePath(UUtUns16	inPathID);

UUtError OWrAI_Initialize(void);
UUtError OWrAI_LevelBegin(void);
void OWrAI_FindCurrentPath(void);

void OWrAI_Display(void);

void OWrProp_BuildWeaponMenu(WMtPopupMenu *inPopupMenu, char *inCurrentClass);

#endif // ONI_WIN_AI_H
