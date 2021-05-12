// ======================================================================
// WM_Cursor.h
// ======================================================================
#ifndef WM_CURSOR_H
#define WM_CURSOR_H

#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"

// ======================================================================
// enums
// ======================================================================
typedef enum WMtCursorType
{
	WMcCursorType_None,
	WMcCursorType_Arrow			

} WMtCursorType;

// ======================================================================
// typedefs
// ======================================================================
typedef tm_struct WMtCursor
{
	UUtInt32					cursor_type;
	tm_templateref				cursor_partspec;
	
} WMtCursor;

#define WMcTemplate_CursorList				UUm4CharToUns32('W', 'M', 'C', 'L')
typedef tm_template('W', 'M', 'C', 'L', "WM Cursor List")
WMtCursorList
{
	tm_pad							pad[20];
	
	tm_varindex UUtUns32			num_cursors;
	tm_vararray WMtCursor			cursors[1];
	
} WMtCursorList;

// ======================================================================
// prototypes
// ======================================================================
void
WMrCursor_Draw(
	WMtCursor				*inCursor,
	IMtPoint2D				*inDestination);
	
WMtCursor*
WMrCursor_Get(
	WMtCursorType			inCursorType);

void
WMrCursor_SetVisible(
	UUtBool					inIsVisible);
	
// ----------------------------------------------------------------------
UUtError
WMrCursor_RegisterTemplates(
	void);

// ======================================================================
#endif /* WM_CURSOR_H */