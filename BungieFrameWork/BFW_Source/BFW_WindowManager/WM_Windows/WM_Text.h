// ======================================================================
// WM_Text.h
// ======================================================================
#ifndef WM_TEXT_H
#define WM_TEXT_H

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
	WMcTextStyle_None			= (0x0000 << 16),
	WMcTextStyle_HLeft			= (0x0001 << 16),
	WMcTextStyle_HCenter		= (0x0002 << 16),
	WMcTextStyle_HRight			= (0x0004 << 16),
	WMcTextStyle_VTop			= (0x0008 << 16),
	WMcTextStyle_VCenter		= (0x0010 << 16),
	WMcTextStyle_VBottom		= (0x0020 << 16),
	WMcTextStyle_SingleLine		= (0x0040 << 16),
	WMcTextStyle_OwnerDraw		= (0x0080 << 16),

	WMcTextStyle_Basic			= (WMcTextStyle_HLeft | WMcTextStyle_VTop),
	WMcTextStyle_Standard		= (WMcTextStyle_HLeft | WMcTextStyle_VCenter)

};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtText;

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrText_Initialize(
	void);

UUtBool
WMrText_SetShade(
	WMtWindow				*inWindow,
	IMtShade				inShade);

// ======================================================================
#endif /* WM_TEXT_H */
