// ======================================================================
// WM_CheckBox.h
// ======================================================================
#ifndef WM_CHECKBOX_H
#define WM_CHECKBOX_H

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
	WMcCheckBoxStyle_None		= (0x0000 << 16),
	WMcCheckBoxStyle_HasTitle	= (0x0001 << 16),
	WMcCheckBoxStyle_HasIcon	= (0x0002 << 16),	/* not supported yet */
	
	WMcCheckBoxStyle_TextCheckBox	= (WMcCheckBoxStyle_HasTitle),
	WMcCheckBoxStyle_IconCheckBox	= (WMcCheckBoxStyle_HasIcon)
		
};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtCheckBox;

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrCheckBox_Initialize(
	void);

void 
WMrCheckBox_SetCheck(WMtCheckBox *inCheckBox, UUtBool inValue);

UUtBool
WMrCheckBox_GetCheck(WMtCheckBox *inCheckBox);

// ======================================================================
#endif /* WM_CHECKBOX_H */