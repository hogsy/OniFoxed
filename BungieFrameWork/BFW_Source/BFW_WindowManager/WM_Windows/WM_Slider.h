// ======================================================================
// WM_Slider.h
// ======================================================================
#ifndef WM_SLIDER_H
#define WM_SLIDER_H

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
	WMcSliderStyle_None			= (0x0000 << 16)

};

enum
{
	SLcNotify_NewPosition		= 1

};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtSlider;

// ======================================================================
// prototypes
// ======================================================================
UUtInt32
WMrSlider_GetPosition(
	WMtSlider				*inSlider);

void
WMrSlider_GetRange(
	WMtSlider				*inSlider,
	UUtInt32				*outMin,
	UUtInt32				*outMax);

void
WMrSlider_SetPosition(
	WMtSlider				*inSlider,
	UUtInt32				inPosition);

void
WMrSlider_SetRange(
	WMtSlider				*inSlider,
	UUtInt32				inMin,
	UUtInt32				inMax);

// ----------------------------------------------------------------------
UUtError
WMrSlider_Initialize(
	void);

// ======================================================================
#endif /* WM_SLIDER_H */
