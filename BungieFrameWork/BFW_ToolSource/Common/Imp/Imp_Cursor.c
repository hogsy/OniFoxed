// ======================================================================
// Imp_Cursor.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"

//#include "DM_DialogCursor.h"
#include "WM_Cursor.h"
#include "WM_PartSpecification.h"

#include "Imp_Cursor.h"
#include "Imp_Common.h"
//#include "Imp_Dialog.h"
#include "Imp_Texture.h"

// ======================================================================
// globals
// ======================================================================
static char*	gCursorCompileTime = __DATE__" "__TIME__;

static AUtFlagElement	IMPgArrowTypes[] =
{
	{ "none", 						WMcCursorType_None },
	{ "arrow", 						WMcCursorType_Arrow },
	{ NULL,							0 }
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
/*UUtError
Imp_AddCursor(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;
	UUtUns16			num_cursors;
	GRtElementArray		*cursor_list_array;
	GRtElementType		element_type;
	UUtUns16			i;
	DCtCursor			*cursor;
	UUtUns16			temp;

	UUtBool				bool_result;
	UUtBool				build_instance;

	UUtUns32			create_date;
	UUtUns32			compile_date;

	// check to see if the dialogs need to be built
	bool_result =
		TMrConstruction_Instance_CheckExists(
			VMcTemplate_View,
			inInstanceName,
			&create_date);
	if (bool_result)
	{
		compile_date = UUrConvertStrToSecsSince1900(gCursorCompileTime);

		build_instance = (UUtBool)(create_date < inSourceFileModDate ||
									create_date < compile_date);
	}
	else
	{
		build_instance = UUcTrue;
	}

	if (!build_instance) return UUcError_None;

	// get a pointer to the cursor list array
	error =
		GRrGroup_GetElement(
			inGroup,
			"cursor_list",
			&element_type,
			&cursor_list_array);
	if ((error != UUcError_None) || (element_type != GRcElementType_Array))
	{
		Imp_PrintWarning("Could not get the cursor list");
		return UUcError_Generic;
	}

	// get the number of cursors in the cursor list array
	num_cursors = (UUtUns16)GRrGroup_Array_GetLength(cursor_list_array);

	// create a new cursor list template
	error =
		TMrConstruction_Instance_Renew(
			DCcTemplate_Cursor,
			inInstanceName,
			num_cursors,
			&cursor);
	IMPmError_ReturnOnError(error);

	// get the animation rate
	error =
		GRrGroup_GetUns16(
			inGroup,
			"anim_rate",
			&temp);
	if (error != UUcError_None)
		cursor->animation_rate = DCcCursor_DefaultAnimationRate;
	else
		cursor->animation_rate = temp;

	// process the cursors
	for (i = 0; i < num_cursors; i++)
	{
		char			*cursor_file_name;
		BFtFileRef		*cursor_file_ref;
		const char		*cursor_name;
		TMtPlaceHolder	cursor_ref;

		// get the cursor filename of cursor_list_array[i]
		error =
			GRrGroup_Array_GetElement(
				cursor_list_array,
				i,
				&element_type,
				&cursor_file_name);
		if ((error != UUcError_None) || (element_type != GRcElementType_String))
		{
			Imp_PrintWarning("Could not get the cursor file name");
			return UUcError_Generic;
		}

		// create a new file ref using the file name
		error =
			BFrFileRef_DuplicateAndReplaceName(
				inSourceFile,
				cursor_file_name,
				&cursor_file_ref);
		IMPmError_ReturnOnErrorMsg(error, "cursor file was not found");

		// set the cursors name
		cursor_name = BFrFileRef_GetLeafName(cursor_file_ref);

		// process the texture map file
		error =
			Imp_ProcessTexture_File(
				cursor_file_ref,
				cursor_name,
				&cursor_ref);

		// save the cursor ref
		cursor->cursors[i].texture_ref = (void*)cursor_ref;

		// dispose of the file ref
		BFrFileRef_Dispose(cursor_file_ref);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddCursorTypeList(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				bool_result;
	UUtBool				build_instance;

	UUtUns32			create_date;
	UUtUns32			compile_date;

	// check to see if the cursor type list needs to be built
	bool_result =
		TMrConstruction_Instance_CheckExists(
			DCcTemplate_CursorTypeList,
			inInstanceName,
			&create_date);
	if (bool_result)
	{
		compile_date = UUrConvertStrToSecsSince1900(gCursorCompileTime);

		build_instance = (UUtBool)(create_date < inSourceFileModDate ||
									create_date < compile_date);
	}
	else
	{
		build_instance = UUcTrue;
	}

	// build the cursor type list if necessary
	if (build_instance)
	{
		DCtCursorTypeList		*cursor_type_list;
		GRtElementArray			*cursor_type_array;
		GRtElementType			element_type;
		UUtUns16				i;
		UUtUns16				num_pairs;
		DCtCursorType			cursor_type;
		char					*cursor_name;
		GRtGroup				*pair;

		// get a pointer to the cursor_type_array
		error =
			GRrGroup_GetElement(
				inGroup,
				"cursor_type_list",
				&element_type,
				&cursor_type_array);
		if ((error != UUcError_None) || (element_type != GRcElementType_Array))
		{
			Imp_PrintWarning("Could not get the cursor type array");
			return UUcError_Generic;
		}

		// get the number of pairs in the cursor_type_array
		num_pairs = (UUtUns16)GRrGroup_Array_GetLength(cursor_type_array);

		// create a new cursor type list template
		error =
			TMrConstruction_Instance_Renew(
				DCcTemplate_CursorTypeList,
				inInstanceName,
				num_pairs,
				&cursor_type_list);
		IMPmError_ReturnOnError(error);

		// process the pairs
		for (i = 0; i < num_pairs; i++)
		{
			// get pair[i]
			error =
				GRrGroup_Array_GetElement(
					cursor_type_array,
					i,
					&element_type,
					&pair);
			if ((error != UUcError_None) || (element_type != GRcElementType_Group))
			{
				Imp_PrintWarning("Could not get the cursor pair");
				return UUcError_Generic;
			}

			// get the type of the pair
			error = GRrGroup_GetUns16(pair, "cursor_type", &cursor_type);
			IMPmError_ReturnOnError(error);

			cursor_type_list->cursor_type_pair[i].cursor_type = cursor_type;

			// get the name of the pair
			error = GRrGroup_GetString(pair, "cursor_name", &cursor_name);
			IMPmError_ReturnOnError(error);

			UUrString_Copy(
				cursor_type_list->cursor_type_pair[i].cursor_name,
				cursor_name,
				DCcMaxCursorNameLength);
		}
	}

	return UUcError_None;
}*/

// ----------------------------------------------------------------------
UUtError
Imp_AddCursorList(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;
	UUtBool				build_instance;

	build_instance = !TMrConstruction_Instance_CheckExists(WMcTemplate_CursorList,inInstanceName);

	// build the cursor list if necessary
	if (build_instance)
	{
		GRtElementArray			*cursor_list_array;
		GRtElementType			element_type;
		UUtUns32				num_elements;
		UUtUns32				i;
		WMtCursorList			*cursor_list;

		// get the cursor list array
		error =
			GRrGroup_GetElement(
				inGroup,
				"cursor_list",
				&element_type,
				&cursor_list_array);
		if ((error != UUcError_None) || (element_type != GRcElementType_Array))
		{
			Imp_PrintWarning("Could not get the cursor list array");
			return UUcError_Generic;
		}

		// get the number of elements in the cursor list array
		num_elements = GRrGroup_Array_GetLength(cursor_list_array);

		if (num_elements == 0)
		{
			return UUcError_None;
		}

		// create a new cursor type list template
		error =
			TMrConstruction_Instance_Renew(
				WMcTemplate_CursorList,
				inInstanceName,
				num_elements,
				&cursor_list);
		IMPmError_ReturnOnError(error);

		for (i = 0; i < num_elements; i++)
		{
			GRtGroup				*cursor_desc_group;
			char					*cursor_type;
			char					*cursor_partspec_name;
			TMtPlaceHolder			cursor_partspec;

			// get a group from the array
			error =
				GRrGroup_Array_GetElement(
					cursor_list_array,
					i,
					&element_type,
					&cursor_desc_group);
			if ((error != UUcError_None) || (element_type != GRcElementType_Group))
			{
				Imp_PrintWarning("Could not get the cursor desc group");
				return UUcError_Generic;
			}

			// get the cursor type string
			error =
				GRrGroup_GetString(
					cursor_desc_group,
					"type",
					&cursor_type);
			IMPmError_ReturnOnError(error);

			// interpret the cursor type string
			error =
				AUrFlags_ParseFromGroupArray(
					IMPgArrowTypes,
					GRcElementType_String,
					cursor_type,
					(UUtUns32*)&cursor_list->cursors[i].cursor_type);
			IMPmError_ReturnOnErrorMsg(error, "Unable to process button flags");

			// get the cursor_partspec
			error =
				GRrGroup_GetString(
					cursor_desc_group,
					"partspec",
					&cursor_partspec_name);
			IMPmError_ReturnOnError(error);

			error =
				TMrConstruction_Instance_GetPlaceHolder(
					PScTemplate_PartSpecification,
					cursor_partspec_name,
					&cursor_partspec);
			IMPmError_ReturnOnErrorMsg(error, "Could not get partspec placeholder");

			// save the partspec
			cursor_list->cursors[i].cursor_partspec = (void*)cursor_partspec;
		}
	}

	return UUcError_None;
}
