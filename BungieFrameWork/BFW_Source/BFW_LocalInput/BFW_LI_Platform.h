// ======================================================================
// BFW_LI_Platform.h
// ======================================================================
#ifndef BFW_LI_PLATFORM_H
#define BFW_LI_PLATFORM_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_LocalInput.h"

// ======================================================================
// prototypes
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
LIrPlatform_InputEvent_InterpretModifiers(
	LItInputEventType	inEventType,
	UUtUns32			inModifiers);

void
LIrPlatform_InputEvent_GetMouse(
	LItMode				inMode,
	LItInputEvent		*outInputEvent);
	
// ----------------------------------------------------------------------
void
LIrPlatform_Mode_Set(
	LItMode				inMode);

// ----------------------------------------------------------------------
UUtError
LIrPlatform_Initialize(
	UUtAppInstance		inInstance,
	UUtWindow			inWindow);

void
LIrPlatform_PollInputForAction(
	LItAction			*outAction);
	
void
LIrPlatform_Terminate(
	void);

UUtBool
LIrPlatform_Update(
	LItMode				inMode);

UUtBool
LIrPlatform_TestKey(
	LItKeyCode			inKeyCode,
	LItMode				inMode);

// ======================================================================
#endif /* BFW_LI_PLATFORM_H */
