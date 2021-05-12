// ======================================================================
// WM_Button.h
// ======================================================================
#ifndef WM_BUTTON_H
#define WM_BUTTON_H

#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcButtonStyle_None				= (0x0000 << 16),
	WMcButtonStyle_HasBackground	= (0x0001 << 16),
	WMcButtonStyle_HasTitle			= (0x0002 << 16),
	WMcButtonStyle_HasIcon			= (0x0004 << 16),	/* not supported yet */
	WMcButtonStyle_Toggle			= (0x0008 << 16),
	WMcButtonStyle_Default			= (0x0010 << 16),
	
	WMcButtonStyle_PushButton			= (WMcButtonStyle_HasBackground | WMcButtonStyle_HasTitle),
	WMcButtonStyle_DefaultPushButton	= (WMcButtonStyle_HasBackground | WMcButtonStyle_HasTitle | WMcButtonStyle_Default),
	WMcButtonStyle_IconButton			= (WMcButtonStyle_HasBackground | WMcButtonStyle_HasIcon),
	WMcButtonStyle_ToggleButton			= (WMcButtonStyle_PushButton | WMcButtonStyle_Toggle),
	WMcButtonStyle_ToggleIconButton		= (WMcButtonStyle_IconButton | WMcButtonStyle_Toggle)
};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtButton;

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrButton_Initialize(
	void);

UUtBool
WMrButton_IsHighlighted(
	WMtWindow *inWindow);

// ======================================================================
#endif /* WM_BUTTON_H */