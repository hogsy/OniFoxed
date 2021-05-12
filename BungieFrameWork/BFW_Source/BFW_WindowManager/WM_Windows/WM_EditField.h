// ======================================================================
// WM_EditField.h
// ======================================================================
#ifndef WM_EDITFIELD_H
#define WM_EDITFIELD_H

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
	WMcEditFieldStyle_None			= (0x0000 << 16),
	WMcEditFieldStyle_NumbersOnly	= (0x0001 << 16)
	
};

enum
{
	EFcNotify_ContentChanged		= 10
};

// ======================================================================
// typedefs
// ======================================================================
typedef WMtWindow				WMtEditField;

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrEditField_Initialize(
	void);

UUtInt32
WMrEditField_GetInt32(WMtEditField *inEditField);

float
WMrEditField_GetFloat(WMtEditField *inEditField);

void
WMrEditField_SetInt32(WMtEditField *inEditField, UUtInt32 inNumber);

void
WMrEditField_SetFloat(WMtEditField *inEditField, float inFloat);

void
WMrEditField_SetText(WMtEditField *inEditField, const char *inString);

void
WMrEditField_GetText(WMtEditField *inEditField, char *outString, UUtUns32 inStringLength);

// ======================================================================
#endif /* WM_EDITFIELD_H */