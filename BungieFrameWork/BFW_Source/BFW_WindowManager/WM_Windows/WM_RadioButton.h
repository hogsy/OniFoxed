// ======================================================================
// WM_RadioButton.h
// ======================================================================
#ifndef WM_RADIOBUTTON_H
#define WM_RADIOBUTTON_H

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
	WMcRadioButtonStyle_None		= (0x0000 << 16),
	WMcRadioButtonStyle_HasTitle	= (0x0001 << 16),
	WMcRadioButtonStyle_HasIcon		= (0x0002 << 16),	/* not supported yet */

	WMcRadioButtonStyle_TextRadioButton		= (WMcRadioButtonStyle_HasTitle),
	WMcRadioButtonStyle_IconRadioButton		= (WMcRadioButtonStyle_HasIcon)

};

enum
{
	WMcRadioButton_Off				= UUcFalse,
	WMcRadioButton_On				= UUcTrue
};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtRadioButton;

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrRadioButton_Initialize(
	void);

// ======================================================================
#endif /* WM_RADIOBUTTON_H */
