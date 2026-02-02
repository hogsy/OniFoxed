// ======================================================================
// DM_DialogCursor.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_DialogManager.h"
#include "DM_DialogCursor.h"
#include "BFW_Motoko.h"
#include "BFW_ViewUtilities.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
DCrCursor_Animate(
	DCtCursor				*inCursor,
	UUtUns32				inTime)
{
	DCtCursor_PrivateData	*private_data;

	UUmAssert(inCursor);

	// get a pointer to the private data
	private_data = (DCtCursor_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Cursor_PrivateData, inCursor);
	if (private_data == NULL) return;

	// make sure it is time to animate
	if (private_data->next_animation > inTime) return;

	// update next animation time
	private_data->next_animation = inTime + (UUtUns32)inCursor->animation_rate;

	// go to next cursor in the list
	private_data->current_cursor++;
	if (private_data->current_cursor >= inCursor->num_cursors)
		private_data->current_cursor = 0;
}

// ----------------------------------------------------------------------
void
DCrCursor_Draw(
	DCtCursor				*inCursor,
	IMtPoint2D				*inDestination,
	float					inLayer)
{
	DCtCursor_PrivateData	*private_data;
	M3tPointScreen			destination;

	// get a pointer to the private data
	private_data = (DCtCursor_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Cursor_PrivateData, inCursor);
	if (private_data == NULL) return;

	// update the destination
	destination.x		= (float)inDestination->x - DCcCursorCenter_X;
	destination.y		= (float)inDestination->y - DCcCursorCenter_Y;
	destination.z		= inLayer;
	destination.invW	= 1.0f / destination.z;

	// draw the cursor
	VUrDrawTextureRef(
		inCursor->cursors[private_data->current_cursor].texture_ref,
		&destination,
		-1,
		-1,
		VUcAlpha_Enabled);
}

// ----------------------------------------------------------------------
UUtError
DCrCursor_Load(
	DCtCursorType			inCursorType,
	DCtCursor				**outCursor)
{
	UUtError				error;
	DCtCursorTypeList		*cursor_type_list;
	DCtCursor				*cursor;
	UUtUns16				i;
	DCtCursor_PrivateData	*private_data;

	UUmAssert(outCursor);

	// clear the outCursor
	*outCursor = NULL;

	// get a pointer to the cursor type list
	error =
		TMrInstance_GetDataPtr(
			DCcTemplate_CursorTypeList,
			"cursor_type_list",
			&cursor_type_list);
	UUmError_ReturnOnError(error);

	// find the cursor in the cursor type list
	for (i = 0; i < cursor_type_list->num_cursor_type_pairs; i++)
	{
		if (cursor_type_list->cursor_type_pair[i].cursor_type == inCursorType)
		{
			// get a pointer to the cursor
			error =
				TMrInstance_GetDataPtr(
					DCcTemplate_Cursor,
					cursor_type_list->cursor_type_pair[i].cursor_name,
					&cursor);
			UUmError_ReturnOnError(error);

			// get a pointer to the private data
			private_data = (DCtCursor_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Cursor_PrivateData, cursor);
			if (private_data == NULL) return UUcError_Generic;

			private_data->current_cursor = 0;

			// return the cursor
			*outCursor = cursor;

			return UUcError_None;
		}
	}

	return UUcError_Generic;
}

// ----------------------------------------------------------------------
UUtError
DCrCursor_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void*					inInstancePtr,
	void*					inPrivateData)
{
	DCtCursor				*cursor;
	DCtCursor_PrivateData	*private_data;

	// get a pointer to the dialog data
	cursor = (DCtCursor*)inInstancePtr;

	// get a pointer to the private data
	private_data = (DCtCursor_PrivateData*)inPrivateData;
	if (private_data == NULL) return UUcError_Generic;

	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initialize the private_data
			private_data->current_cursor = 0;
			private_data->next_animation = 0;
		break;

		case TMcTemplateProcMessage_DisposePreProcess:
		break;

		case TMcTemplateProcMessage_Update:
		break;

		default:
			UUmAssert(!"Illegal message");
		break;
	}

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
DCrInitialize(
	void)
{
	UUtError				error;

	// install the private data and/or procs for cursors
	error =
		TMrTemplate_PrivateData_New(
			DCcTemplate_Cursor,
			sizeof(DCtCursor_PrivateData),
			DCrCursor_ProcHandler,
			&DMgTemplate_Cursor_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install dialog proc handler");

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
DCrTerminate(
	void)
{
}
