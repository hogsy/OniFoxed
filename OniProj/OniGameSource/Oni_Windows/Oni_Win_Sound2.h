// ======================================================================
// Oni_Win_Sound2.h
// ======================================================================
#pragma once

#ifndef ONI_WIN_SOUND2_H
#define ONI_WIN_SOUND2_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_SoundSystem2.h"
#include "BFW_WindowManager.h"

#include "Oni_Sound2.h"
#include "Oni_Windows.h"

// ======================================================================
// enums
// ======================================================================
typedef enum OWtSMType
{
	OWcSMType_Ambient,
	OWcSMType_Group,
	OWcSMType_Impulse
	
} OWtSMType;

typedef enum OWtSelectResult
{
	OWcSelectResult_Cancel,
	OWcSelectResult_Select,
	OWcSelectResult_None
	
} OWtSelectResult;

// ======================================================================
// prototypes
// ======================================================================
void
OWrSound2_Terminate(
	void);
	
// ----------------------------------------------------------------------
UUtError
OWrImpulseProperties_Display(
	WMtDialog					*inParentDialog,
	SStImpulse					*inImpulse);
	
// ----------------------------------------------------------------------
UUtError
OWrSM_Display(
	OWtSMType					inType);

// ----------------------------------------------------------------------
OWtSelectResult
OWrSelect_AmbientSound(
	SStAmbient					**outAmbient);
	
OWtSelectResult
OWrSelect_SoundGroup(
	SStGroup					**outGroup);
	
OWtSelectResult
OWrSelect_ImpulseSound(
	SStImpulse					**outImpulse);
	
// ----------------------------------------------------------------------
UUtError
OWrAmbientProperties_Display(
	WMtDialog					*inParentDialog,
	SStAmbient					*inAmbient);
	
UUtError
OWrSoundToAnim_Display(
	void);
	
// ----------------------------------------------------------------------
UUtError
OWrCreateDialogue_Display(
	void);
	
UUtError
OWrCreateImpulse_Display(
	void);

UUtError
OWrCreateGroup_Display(
	void);

void
OWrViewAnimation_Display(
	void);

// ----------------------------------------------------------------------
UUtError
OWrMinMax_Fixer(
	void);

UUtError
OWrSpeech_Fixer(
	void);
	
// ======================================================================
#endif /* ONI_WIN_SOUND2_H */