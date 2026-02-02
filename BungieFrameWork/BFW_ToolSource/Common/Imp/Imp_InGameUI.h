// ======================================================================
// Imp_InGameUI.h
// ======================================================================
#pragma once

#ifndef IMP_INGAMEUI_H
#define IMP_INGAMEUI_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// prototypes
// ======================================================================
UUtError
Imp_AddDiaryPage(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddHelpPage(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddItemPage(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddObjectivePage(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddWeaponPage(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ----------------------------------------------------------------------
UUtError
Imp_AddEnemyScannerUI(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ----------------------------------------------------------------------
UUtError
Imp_AddKeyIcons(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ----------------------------------------------------------------------
UUtError
Imp_AddTextConsole(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ----------------------------------------------------------------------
UUtError
Imp_AddHUDHelp(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

// ======================================================================
#endif /* IMP_INGAMEUI_H */
