// ======================================================================
// WM_Cursor.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "WM_Cursor.h"
#include "WM_PartSpecification.h"

// ======================================================================
// defines
// ======================================================================
#define WMcCursor_Width			32
#define WMcCursor_Height		32

#define WMcCursor_Center_X		16
#define WMcCursor_Center_Y		16

// ======================================================================
// globals
// ======================================================================
static UUtBool					WMgCursor_Visible;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
WMrCursor_Draw(
	WMtCursor				*inCursor,
	IMtPoint2D				*inDestination)
{
	M3tPointScreen			dest;
	
	if (WMgCursor_Visible == UUcFalse) { return; }
	
	dest.x = (float)(inDestination->x - WMcCursor_Center_X);
	dest.y = (float)(inDestination->y - WMcCursor_Center_Y);
	dest.z = 0.5f;
	dest.invW = 1.0f / 0.5f;
	
	PSrPartSpec_Draw(
		inCursor->cursor_partspec,
		PScPart_MiddleMiddle,
		&dest,
		WMcCursor_Width,
		WMcCursor_Height,
		M3cMaxAlpha);
}

// ----------------------------------------------------------------------
WMtCursor*
WMrCursor_Get(
	WMtCursorType			inCursorType)
{
	UUtError				error;
	WMtCursorList			*cursor_list;
	UUtUns32				i;
	WMtCursor				*cursor;
	
	error =
		TMrInstance_GetDataPtr(
			WMcTemplate_CursorList,
			"cursor_list",
			&cursor_list);
	if (error != UUcError_None)
	{
		return NULL;
	}
	
	cursor = NULL;
	
	for (i = 0; i < cursor_list->num_cursors; i++)
	{
		if (cursor_list->cursors[i].cursor_type == inCursorType)
		{
			cursor = &cursor_list->cursors[i];
			break;
		}
	}
	
	return cursor;
}

// ----------------------------------------------------------------------
void
WMrCursor_SetVisible(
	UUtBool					inIsVisible)
{
	WMgCursor_Visible = inIsVisible;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrCursor_RegisterTemplates(
	void)
{
	UUtError				error;
	
	error =
		TMrTemplate_Register(
			WMcTemplate_CursorList,
			sizeof(WMtCursorList),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
