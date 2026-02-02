// ======================================================================
// WM_Scrollbar.h
// ======================================================================
#ifndef WM_SCROLLBAR_H
#define WM_SCROLLBAR_H

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
	WMcScrollbarStyle_None				= (0x0000 << 16),
	WMcScrollbarStyle_Vertical			= (0x0001 << 16),
	WMcScrollbarStyle_Horizontal		= (0x0002 << 16),
	WMcScrollbarStyle_HasDoubleArrows	= (0x0004 << 16),	/* not supported yet */
	WMcScrollbarStyle_ArrowsAtBothEnds	= (0x0008 << 16)	/* not supported yet */

};

enum
{
	SBcNotify_LineUp,
	SBcNotify_LineDown,
	SBcNotify_PageUp,
	SBcNotify_PageDown,
	SBcNotify_ThumbPosition

};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtScrollbar;

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrScrollbar_Initialize(
	void);

UUtInt32
WMrScrollbar_GetPosition(
	WMtScrollbar			*inScrollbar);

void
WMrScrollbar_GetRange(
	WMtScrollbar			*inScrollbar,
	UUtInt32				*outMin,
	UUtInt32				*outMax,
	UUtInt32				*outRangeVisible);

void
WMrScrollbar_SetPosition(
	WMtScrollbar			*inScrollbar,
	UUtInt32				inPosition);

void
WMrScrollbar_SetRange(
	WMtScrollbar			*inScrollbar,
	UUtInt32				inMin,
	UUtInt32				inMax,
	UUtInt32				inRangeVisible);

// ======================================================================
#endif /* WM_SCROLLBAR_H */
