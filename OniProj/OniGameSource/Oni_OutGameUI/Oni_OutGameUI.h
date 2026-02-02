// ======================================================================
// Oni_OutGameUI.h
// ======================================================================
#pragma once

#ifndef ONI_OUTGAMEUI_H
#define ONI_OUTGAMEUI_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// defines
// ======================================================================
#define ONcOutGameUIName			"psui_oniUI"

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
ONrOutGameUI_MainMenu_Display(
	void);

UUtUns32
ONrOutGameUI_NewGame_Display(
	void);

UUtUns32
ONrOutGameUI_LoadGame_Display(
	void);

UUtUns32
ONrOutGameUI_Options_Display(
	void);

UUtUns32
ONrOutGameUI_QuitYesNo_Display(
	void);

// ======================================================================
#endif /* ONI_OUTGAMEUI_H */
