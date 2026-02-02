// ======================================================================
// Imp_DialogList.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"
#include "BFW_DialogManager.h"

#include "Imp_Common.h"
#include "Imp_DialogList.h"

// ======================================================================
// globals
// ======================================================================
static char*	gDialogListCompileTime = __DATE__" "__TIME__;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtError
Imp_AddDialogList(
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

	// check to see if the dialogs need to be built
	bool_result =
		TMrConstruction_Instance_CheckExists(
			VMcTemplate_View,
			inInstanceName,
			&create_date);
	if (bool_result)
	{
		compile_date = UUrConvertStrToSecsSince1900(gDialogListCompileTime);

		build_instance = (UUtBool)(create_date < inSourceFileModDate ||
									create_date < compile_date);
	}
	else
	{
		build_instance = UUcTrue;
	}

	if (build_instance)
	{
		GRtElementType		element_type;
		GRtElementArray		*dialog_list_array;
		DMtDialogList		*dialog_list;
		UUtUns16			num_dialogs;
		UUtUns16			i;

		// get the dialog list array
		error =
			GRrGroup_GetElement(
				inGroup,
				"dialoglist",
				&element_type,
				&dialog_list_array);
		if ((error != UUcError_None) && (element_type != GRcElementType_Array))
		{
			IMPmError_ReturnOnErrorMsg(error, "Unable to get element array");
		}

		// get the number of elements in the dialog list
		num_dialogs = (UUtUns16)GRrGroup_Array_GetLength(dialog_list_array);

		// create an dialog list template instance
		error =
			TMrConstruction_Instance_Renew(
				DMcTemplate_DialogList,
				inInstanceName,
				num_dialogs,
				&dialog_list);
		IMPmError_ReturnOnErrorMsg(error, "Could not create a dialog list template");

		// fill in the dialog list
		for (i = 0; i < num_dialogs; i++)
		{
			char 			*dialog_name;
			TMtPlaceHolder	dialog_ref;

			// get the name of the dialog instance
			error =
				GRrGroup_Array_GetElement(
					dialog_list_array,
					i,
					&element_type,
					&dialog_name);
			if ((error != UUcError_None) && (element_type != GRcElementType_String))
			{
				IMPmError_ReturnOnErrorMsg(error, "Unable to get dialog instance name");
			}

			// get a place holder for the dialog
			error =
				TMrConstruction_Instance_GetPlaceHolder(
					VMcTemplate_View,
					dialog_name,
					&dialog_ref);
			IMPmError_ReturnOnErrorMsg(error, "Could not get dialog placeholder");

			// save the dialog ref
			dialog_list->dialogs[i].dialog_ref = (void*)dialog_ref;
		}
	}

	return UUcError_None;
}
