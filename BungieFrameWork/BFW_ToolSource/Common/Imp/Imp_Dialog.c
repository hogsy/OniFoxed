// ======================================================================
// Imp_Dialog.c
// ======================================================================

// ======================================================================
// Includes
// ======================================================================
#include "BFW.h"
#include "BFW_DialogManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_ViewManager.h"

#include "Imp_Common.h"
#include "Imp_Dialog.h"
#include "Imp_Texture.h"
#include "Imp_View.h"

// ======================================================================
// defines
// ======================================================================
#define cMaxTextureNameLength		32
#define cMaxDialogFlags				32

// ======================================================================
// typedef
// ======================================================================
typedef UUtError
(*IMPrViewProc)(
	GRtGroup				*inGroup,
	TMtPlaceHolder			*outViewData);

typedef struct tViewProc
{
	char					name[32];
	IMPrViewProc			view_proc;
} tViewProc;

// ======================================================================
// globals
// ======================================================================
static char*	gDialogCompileTime = __DATE__" "__TIME__;

static AUtFlagElement	IMPgDialogFlags[] =
{
	{ "none", 						DMcDialogFlag_None },
	{ "dialog_centered",			DMcDialogFlag_Centered },
	{ "dialog_show_cursor",			DMcDialogFlag_ShowCursor },
	{ NULL,							0 }
};

static AUtFlagElement	IMPgViewFlags[] =
{
	{ "none", 						VMcViewFlag_None },
	{ "view_visible",				VMcViewFlag_Visible },
	{ "view_enabled",				VMcViewFlag_Enabled },
	{ "view_active",				VMcViewFlag_Active },
	{ NULL,							0 }
};

static tViewProc		IMPgViewProc[] =
{
	{ "dialog",			IMPrView_Process_Dialog },
	{ "button", 		IMPrView_Process_Button },
	{ "checkbox",		IMPrView_Process_CheckBox },
	{ "EditField",		IMPrView_Process_EditField },
	{ "Picture",		IMPrView_Process_Picture },
	{ "Scrollbar",		IMPrView_Process_Scrollbar },
	{ "Slider",			IMPrView_Process_Slider },
	{ "Tab",			IMPrView_Process_Tab },
	{ "Text",			IMPrView_Process_Text },
	{ NULL,				NULL }
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
/*static UUtError
iProcessControls(
	GRtGroup			*inGroup,
	DMtDialog			*inDialog)
{
	UUtError			error;
	UUtUns16			num_controls;
	GRtElementArray		*control_list;
	GRtElementType		element_type;
	UUtUns16			i;

	// get pointer to control list
	error =
		GRrGroup_GetElement(
			inGroup,
			"controlList",
			&element_type,
			&control_list);
	if ((error != UUcError_None) || (element_type != GRcElementType_Array))
	{
		Imp_PrintWarning("Could not get the control list");
		return error;
	}

	// get the number of controls in the control_list
	num_controls = (UUtUns16)GRrGroup_Array_GetLength(control_list);

	// create a new control list template
	error =
		TMrConstruction_Instance_NewUnique(
			DMcTemplate_ControlList,
			num_controls,
			&inDialog->dialog_controls);
	IMPmError_ReturnOnError(error);

	// process the controls
	for (i = 0; i < num_controls; i++)
	{
		DMtControl			*control;
		GRtGroup			*control_group;
		char				*control_name;

		// get the control element
		error =
			GRrGroup_Array_GetElement(
				control_list,
				i,
				&element_type,
				&control_group);
		if ((error != UUcError_None) || (element_type != GRcElementType_Group))
		{
			Imp_PrintWarning("Could not get the control group");
			return error;
		}

		// create an instance of this control
		error =
			TMrConstruction_Instance_NewUnique(
				DMcTemplate_Control,
				0,
				&control);
		UUmError_ReturnOnError(error);

		// initialize the fields
		control->flags = 0;

		// get the width
		error =
			GRrGroup_GetUns16(
				control_group,
				"width",
				&control->width);
		IMPmError_ReturnOnError(error);

		// get the height
		error =
			GRrGroup_GetUns16(
				control_group,
				"height",
				&control->height);
		IMPmError_ReturnOnError(error);

		// get the controls location
		error = Imp_ProcessLocation(control_group, &control->location);
		if (error != UUcError_None)
		{
			Imp_PrintWarning("Could not process the control's location");
			return error;
		}

		// get the controls id number
		error =
			GRrGroup_GetUns16(
				control_group,
				"control_id",
				&control->id);
		if (error != UUcError_None)
		{
			Imp_PrintWarning("Could not get the controls id number");
			return error;
		}

		// get the controls name
		error =
			GRrGroup_GetString(
				control_group,
				"control_name",
				&control_name);
		IMPmError_ReturnOnErrorMsg(error, "Cound not get the controls name");

		// process the control flags
		error =	Imp_ProcessDialogFlags(control_group, IMPgControlFlags, &control->flags);
		IMPmError_ReturnOnErrorMsg(error, "Error processing the flags.");

		// what type of control are we dealing with
		if (strncmp(control_name, "button", strlen("button")) == 0)
		{
			error =
				TMrConstruction_Instance_GetPlaceHolder(
					DMcTemplate_Control_Button,
					control_name,
					(TMtPlaceHolder*)&control->control_ref);
		}
		else if (strncmp(control_name, "edit", strlen("edit")) == 0)
		{
			error =
				TMrConstruction_Instance_GetPlaceHolder(
					DMcTemplate_Control_EditField,
					control_name,
					(TMtPlaceHolder*)&control->control_ref);
		}
		else if (strncmp(control_name, "picture", strlen("picture")) == 0)
		{
			error =
				TMrConstruction_Instance_GetPlaceHolder(
					DMcTemplate_Control_Picture,
					control_name,
					(TMtPlaceHolder*)&control->control_ref);
		}
		else if (strncmp(control_name, "text", strlen("text")) == 0)
		{
			error =
				TMrConstruction_Instance_GetPlaceHolder(
					DMcTemplate_Control_Text,
					control_name,
					(TMtPlaceHolder*)&control->control_ref);
		}
		else
		{
			error = UUcError_Generic;
		}

		if (error != UUcError_None)
		{
			Imp_PrintWarning("Could not get the control's control_ref");
			return error;
		}

		// set the control
		inDialog->dialog_controls->controls[i] = control;
	}

	return UUcError_None;
}*/

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
Imp_ProcessDialogFlags(
	GRtGroup			*inGroup,
	AUtFlagElement		*inFlagList,
	UUtUns16			*outFlags)
{
	UUtError			error;
	GRtElementType		element_type;
	GRtElementArray		*flag_array;
	UUtUns32			flags;

	// init the flags
	*outFlags = 0;
	flags = 0;

	// get the flag string
	error =
		GRrGroup_GetElement(
			inGroup,
			"flags",
			&element_type,
			&flag_array);
	if (error != UUcError_None) return error;
	if (element_type != GRcElementType_Array)
	{
		IMPmError_ReturnOnErrorMsg(error, "Error getting flags");
	}

	// process the flags
	error =
		AUrFlags_ParseFromGroupArray(
			inFlagList,
			element_type,
			flag_array,
			&flags);
	if ((error != UUcError_None) && (error != AUcError_FlagNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Unable to parse the flags.");
	}

	// set the outgoing string
	*outFlags = flags;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_ProcessLocation(
	GRtGroup			*inGroup,
	IMtPoint2D			*outLocation)
{
	UUtError			error;
	GRtElementType		element_type;
	GRtGroup			*location;

	// initialize outLocation
	outLocation->x = 0;
	outLocation->y = 0;

	// get the location group
	error =
		GRrGroup_GetElement(
			inGroup,
			"location",
			&element_type,
			&location);
	if (error != UUcError_None)
	{
		return error;
	}
	if (element_type != GRcElementType_Group)
	{
		Imp_PrintWarning("Could not get location");
		return UUcError_Generic;
	}

	// get the location x
	error = GRrGroup_GetUns16(location, "x", (UUtUns16*)&outLocation->x);
	IMPmError_ReturnOnError(error);

	// get the location y
	error = GRrGroup_GetUns16(location, "y", (UUtUns16*)&outLocation->y);
	IMPmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_ProcessTextures(
	BFtFileRef*			inSourceFile,
	GRtGroup			*inGroup,
	DMtTextureList		**inTextureList,
	char				*inTextureListName)
{
	UUtError			error;
	UUtUns16			num_textures;
	GRtElementArray		*texture_list;
	GRtElementType		element_type;
	UUtUns16			i;

	// get pointer to texture list
	// it is okay for the list not to exist
	error =
		GRrGroup_GetElement(
			inGroup,
			inTextureListName,
			&element_type,
			&texture_list);
	if (error != UUcError_None)
		return UUcError_None;

	if (element_type != GRcElementType_Array)
	{
		Imp_PrintWarning("Could not get the texture list");
		return UUcError_Generic;
	}

	// get the number of textures in the texture_list
	num_textures = (UUtUns16)GRrGroup_Array_GetLength(texture_list);

	// create a new texture list template
	error =
		TMrConstruction_Instance_NewUnique(
			DMcTemplate_TextureList,
			num_textures,
			inTextureList);
	IMPmError_ReturnOnError(error);

	// process the textures
	for (i = 0; i < num_textures; i++)
	{
		char			*texture_file;
		BFtFileRef		*texture_file_ref;
		char			*texture_name;
		TMtPlaceHolder	texture_ref;

		// get the name of texture[i]
		error =
			GRrGroup_Array_GetElement(
				texture_list,
				i,
				&element_type,
				&texture_file);
		if ((error != UUcError_None) || (element_type != GRcElementType_String))
		{
			Imp_PrintWarning("Could not get the texture file name");
			return UUcError_Generic;
		}

		// create a new file ref using the texture's file name
		error =
			BFrFileRef_DuplicateAndReplaceName(
				inSourceFile,
				texture_file,
				&texture_file_ref);
		IMPmError_ReturnOnErrorMsg(error, "texture file was not found");

		// set the textures name
		texture_name = BFrFileRef_GetLeafName(texture_file_ref);

		// process the texture map file
		error =	Imp_ProcessTexture_Big_File(texture_file_ref, texture_name, &texture_ref);
		IMPmError_ReturnOnErrorMsg(error, "Could not import the texture file.");

		// save the texture ref
		(*inTextureList)->textures[i].texture_ref = (void*)texture_ref;

		// dispose of the file ref
		BFrFileRef_Dispose(texture_file_ref);
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
Imp_AddView(
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
		compile_date = UUrConvertStrToSecsSince1900(gDialogCompileTime);

		build_instance = (UUtBool)(create_date < inSourceFileModDate ||
									create_date < compile_date);
	}
	else
	{
		build_instance = UUcTrue;
	}

	if (build_instance)
	{
		UUtUns16					id;
		UUtUns16					type;
		char						*type_name;
		UUtUns16					flags;
		IMtPoint2D					location;
		UUtInt16					width;
		UUtInt16					height;
		GRtElementType				element_type;
		GRtElementArray				*views_array;
		UUtUns16					num_child_views;
		VMtView						*view;
		TMtPlaceHolder				view_data;
		UUtUns16					i;
		tViewProc					*viewproc;

		// get the id
		error =
			GRrGroup_GetUns16(
				inGroup,
				"id",
				&id);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get view id");

		// get the type name
		error =
			GRrGroup_GetString(
				inGroup,
				"type",
				&type_name);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get view type");

		// get the flags
		error =
			Imp_ProcessDialogFlags(
				inGroup,
				IMPgViewFlags,
				&flags);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get view flags");

		// get the location
		error =
			Imp_ProcessLocation(
				inGroup,
				&location);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get view location");

		// get the width
		error =
			GRrGroup_GetInt16(
				inGroup,
				"width",
				&width);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get view width");

		// get the height
		error =
			GRrGroup_GetInt16(
				inGroup,
				"height",
				&height);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get view height");

		// get the view data
		view_data = NULL;
		for (viewproc = IMPgViewProc;
			 viewproc->name != NULL;
			 viewproc++)
		{
			if (!strncmp(type_name, viewproc->name, strlen(viewproc->name)))
			{
				viewproc->view_proc(inGroup, &view_data);
			}
		}

		// get the views array
		error =
			GRrGroup_GetElement(
				inGroup,
				"view_list",
				&element_type,
				&views_array);
		if ((error != UUcError_None) && (element_type != GRcElementType_Array))
		{
			if (error != GRcError_ElementNotFound)
			{
				IMPmError_ReturnOnErrorMsg(
					error,
					"Unable to get view views array");
			}
			else
			{
				views_array = NULL;
			}
		}

		// get the number of elements in the views array
		if (views_array)
			num_child_views = (UUtUns16)GRrGroup_Array_GetLength(views_array);
		else
			num_child_views = 0;

		// create an view template instance
		error =
			TMrConstruction_Instance_Renew(
				VMcTemplate_View,
				inInstanceName,
				num_child_views,
				&view);
		IMPmError_ReturnOnErrorMsg(error, "Could not create a view template");

		// set the fields of view
		view->id			= id;
		view->type			= type;
		view->flags			= flags;
		view->location		= location;
		view->width			= width;
		view->height		= height;
		view->view_data		= (TMtPlaceHolder*)view_data;

		// copy the views from the views_array
		for (i = 0; i < num_child_views; i++)
		{
			void			*view_description;
			TMtPlaceHolder	view_ref;
			char			*instance_name;

			// get the name of the view
			error =
				GRrGroup_Array_GetElement(
					views_array,
					i,
					&element_type,
					&view_description);
			IMPmError_ReturnOnErrorMsg(error, "Could not get view ref name");

			if (element_type == GRcElementType_String)
			{
				instance_name = view_description;
			}
			else if (element_type == GRcElementType_Group)
			{
				// get the instance name
				error =
					GRrGroup_GetString(
						view_description,
						"instance",
						&instance_name);
				IMPmError_ReturnOnErrorMsg(error, "Could not get instance name");

				// process the group
				error =
					Imp_AddView(
						inSourceFile,
						inSourceFileModDate,
						view_description,
						instance_name);
				IMPmError_ReturnOnErrorMsg(error, "Could not get instance name");
			}

			// get a place holder for the view
			error =
				TMrConstruction_Instance_GetPlaceHolder(
					VMcTemplate_View,
					instance_name,
					&view_ref);
			IMPmError_ReturnOnErrorMsg(error, "Could not get view placeholder for dialog");

			view->child_views[i].view_ref = (void*)view_ref;
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddDialogData(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
/*	UUtError			error;

	UUtBool				bool_result;
	UUtBool				build_instance;

	UUtUns32			create_date;
	UUtUns32			compile_date;

	DMtDialogData		*dialogdata;

	// check to see if the dialogs need to be built
	bool_result =
		TMrConstruction_Instance_CheckExists(
			DMcTemplate_DialogData,
			inInstanceName,
			&create_date);
	if (bool_result)
	{
		compile_date = UUrConvertStrToSecsSince1900(gDialogCompileTime);

		build_instance = (UUtBool)(create_date < inSourceFileModDate ||
									create_date < compile_date);
	}
	else
	{
		build_instance = UUcTrue;
	}

	// build the dialogs if necessary
	if (build_instance)
	{
		// create a new dialog template
		error =
			TMrConstruction_Instance_Renew(
				DMcTemplate_DialogData,
				inInstanceName,
				0,
				&dialogdata);
		IMPmError_ReturnOnErrorMsg(error, "Could not create a dialogdata template");

		// get the animation rate
		error =
			GRrGroup_GetUns16(
				inGroup,
				"animation_rate",
				&dialogdata->animation_rate);
		if (error != UUcError_None)
		{
			if (error != GRcError_ElementNotFound)
			{
				IMPmError_ReturnOnErrorMsg(
					error,
					"Unable to process animation rate for dialog data");
			}
			else
			{
				dialogdata->animation_rate = 0;
			}
		}

		// import the background
		error =
			Imp_ProcessTextures(
				inSourceFile,
				inGroup,
				&dialogdata->dialog_textures,
				"textures");
		IMPmError_ReturnOnErrorMsg(error, "Could not get dialogdata textures");

		// import the location
		error =
			Imp_ProcessDialogFlags(
				inGroup,
				IMPgDialogFlags,
				&dialogdata->flags);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get the dialogdata location");
	}*/

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddDialog(
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

	DMtDialog			*dialog;

	// check to see if the dialogs need to be built
	bool_result =
		TMrConstruction_Instance_CheckExists(
			DMcTemplate_Dialog,
			inInstanceName,
			&create_date);
	if (bool_result)
	{
		compile_date = UUrConvertStrToSecsSince1900(gDialogCompileTime);

		build_instance = (UUtBool)(create_date < inSourceFileModDate ||
									create_date < compile_date);
	}
	else
	{
		build_instance = UUcTrue;
	}

	// build the dialogs if necessary
	if (build_instance)
	{
		char			*view_name;

		// create a new dialog template
		error =
			TMrConstruction_Instance_Renew(
				DMcTemplate_Dialog,
				inInstanceName,
				0,
				&dialog);
		IMPmError_ReturnOnErrorMsg(error, "Could not create a dialog template");

		// get the name of the view
		error =
			GRrGroup_GetString(
				inGroup,
				"view",
				&view_name);
		IMPmError_ReturnOnErrorMsg(error, "Could not find a view in the dialog ins file");

		// get a place holder for the view
		error =
			TMrConstruction_Instance_GetPlaceHolder(
				VMcTemplate_View,
				view_name,
				(TMtPlaceHolder*)&dialog->view_ref);
		IMPmError_ReturnOnErrorMsg(error, "Could not get view placeholder for dialog");
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddDialogTypeList(
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
			DMcTemplate_DialogTypeList,
			inInstanceName,
			&create_date);
	if (bool_result)
	{
		compile_date = UUrConvertStrToSecsSince1900(gDialogCompileTime);

		build_instance = (UUtBool)(create_date < inSourceFileModDate ||
									create_date < compile_date);
	}
	else
	{
		build_instance = UUcTrue;
	}

	// build the dialogs if necessary
	if (build_instance)
	{
		DMtDialogTypeList		*dialog_type_list;
		GRtElementArray			*dialog_type_array;
		GRtElementType			element_type;
		UUtUns16				i;
		UUtUns16				num_pairs;
		DMtDialogType			dialog_type;
		char					*dialog_name;
		GRtGroup				*pair;

		// get a pointer to the dialog_type_array
		error =
			GRrGroup_GetElement(
				inGroup,
				"dialog_type_list",
				&element_type,
				&dialog_type_array);
		if ((error != UUcError_None) || (element_type != GRcElementType_Array))
		{
			Imp_PrintWarning("Could not get the dialog type array");
			return UUcError_Generic;
		}

		// get the number of pairs in the dialog_type_array
		num_pairs = (UUtUns16)GRrGroup_Array_GetLength(dialog_type_array);

		// create a new dialog template
		error =
			TMrConstruction_Instance_Renew(
				DMcTemplate_DialogTypeList,
				inInstanceName,
				num_pairs,
				&dialog_type_list);
		IMPmError_ReturnOnError(error);

		// process the pairs
		for (i = 0; i < num_pairs; i++)
		{
			UUtUns16			j;

			// get pair[i]
			error =
				GRrGroup_Array_GetElement(
					dialog_type_array,
					i,
					&element_type,
					&pair);
			if ((error != UUcError_None) || (element_type != GRcElementType_Group))
			{
				Imp_PrintWarning("Could not get the pair");
				return UUcError_Generic;
			}

			// get the type of the pair
			error = GRrGroup_GetUns16(pair, "dialog_type", &dialog_type);
			IMPmError_ReturnOnError(error);

			// check for duplicates
			for (j = 0; j < i; j++)
			{
				if (dialog_type_list->dialog_type_pair[j].dialog_type == dialog_type)
				{
					Imp_PrintWarning("WARNING: Two dialogs have the same dialog type.");
				}
			}

			dialog_type_list->dialog_type_pair[i].dialog_type = dialog_type;

			// get the name of the pair
			error = GRrGroup_GetString(pair, "dialog_name", &dialog_name);
			IMPmError_ReturnOnError(error);

			UUrString_Copy(
				dialog_type_list->dialog_type_pair[i].dialog_name,
				dialog_name,
				DMcMaxDialogNameLength);
		}
	}

	return UUcError_None;
}
