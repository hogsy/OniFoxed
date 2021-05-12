// ======================================================================
// Imp_View.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"
#include "BFW_ViewManager.h"
#include "BFW_DialogManager.h"
#include "BFW_FileFormat.h"

#include "Imp_Common.h"
#include "Imp_View.h"
#include "Imp_Texture.h"

#include "VM_View_Box.h"
#include "VM_View_Button.h"
#include "VM_View_CheckBox.h"
#include "VM_View_Dialog.h"
#include "VM_View_EditField.h"
#include "VM_View_ListBox.h"
#include "VM_View_Picture.h"
#include "VM_View_RadioButton.h"
#include "VM_View_RadioGroup.h"
#include "VM_View_Scrollbar.h"
#include "VM_View_Slider.h"
#include "VM_View_Tab.h"
#include "VM_View_Text.h"

// ======================================================================
// typedef
// ======================================================================
typedef UUtError
(*IMPiViewProc)(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData);

typedef struct tViewProc
{
	char					*name;
	UUtUns16				type;
	IMPiViewProc			view_proc;
	
} tViewProc;

typedef struct tParts
{
	char					*name;
	UUtUns16				row;
	UUtUns16				column;
	
} tParts;

// ======================================================================
// globals
// ======================================================================
static char*	gViewCompileTime = __DATE__" "__TIME__;

static AUtFlagElement	IMPgDialogFlags[] =
{
	{ "none",						DMcDialogFlag_None },
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
	{ "view_can_focus",				VMcViewFlag_CanFocus },
	{ NULL,							0 }
};

static AUtFlagElement	IMPgCommonFlags[] =
{
	{ "none", 						VMcCommonFlag_None },
	{ "text_hleft",					VMcCommonFlag_Text_HLeft },
	{ "text_hcenter",				VMcCommonFlag_Text_HCenter },
	{ "text_hright",				VMcCommonFlag_Text_HRight },
	{ "text_vcenter",				VMcCommonFlag_Text_VCenter },
	{ "text_small",					VMcCommonFlag_Text_Small },
	{ "text_bold",					VMcCommonFlag_Text_Bold },
	{ "text_italic",				VMcCommonFlag_Text_Italic },
	{ NULL,							0 }
};

static AUtFlagElement	IMPgAnimationFlags[] =
{
	{ "none",						VMcAnimationFlag_None },
	{ "animation_random",			VMcAnimationFlag_Random },
	{ "animation_pingpong",			VMcAnimationFlag_PingPong },
	{ NULL,							0 }
};

static tParts			IMPgParts[] =
{
	{ "lt",	0,	0 },
	{ "lm",	0,	1 },
	{ "lb",	0,	2 },
	{ "mt",	1,	0 },
	{ "mm",	1,	1 },
	{ "mb",	1,	2 },
	{ "rt",	2,	0 },
	{ "rm",	2,	1 },
	{ "rb",	2,	2 },
	{ NULL,	0,	0 }
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
IMPiProcessViewFlags(
	GRtGroup			*inGroup,
	AUtFlagElement		*inFlagList,
	char				*flag_name,
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
			flag_name,
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
	*outFlags = (UUtUns16)flags;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiProcessLocation(
	GRtGroup			*inGroup,
	char				*location_name,
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
			location_name,
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
static UUtError
IMPiProcessPartSpecification(
	BFtFileRef			*inSourceFile,
	GRtGroup			*inGroup,
	char				*inPartSpecificationName,
	VMtPartSpec **outPartSpecificationRef)
{
	UUtError			error;
	GRtElementType		element_type;
	void				*part_specification;
	char				*instance_name;
	TMtPlaceHolder		part_spec_ref;
	UUtUns32			sourceModDate;
	
	*outPartSpecificationRef = NULL;
	
	// get the element
	// it is okay if it doesn't exist
	error =
		GRrGroup_GetElement(
			inGroup,
			inPartSpecificationName,
			&element_type,
			&part_specification);
	if (error != UUcError_None) return UUcError_None;
	
	if (element_type == GRcElementType_String)
	{
		instance_name = (char*)part_specification;
	}
	else if (element_type == GRcElementType_Group)
	{
		// get the instance name
		error =
			GRrGroup_GetString(
				part_specification,
				"instance",
				&instance_name);
		IMPmError_ReturnOnErrorMsg(error, "Could not get instance name");
		
		// get the mod date of this file
		error =
			BFrFileRef_GetModTime(
				inSourceFile,
				&sourceModDate);
		IMPmError_ReturnOnErrorMsg(error, "Could not get instance name");

		// process the group
		error =
			Imp_AddPartSpecification(
				inSourceFile,
				sourceModDate,
				part_specification,
				instance_name);
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
	
	// get a place holder for the 
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_PartSpecification,
			instance_name,
			&part_spec_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get view placeholder for dialog");
	
	*outPartSpecificationRef = (void*)part_spec_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiProcessSingleTexture(
	BFtFileRef			*inSourceFile,
	GRtGroup			*inGroup,
	char				*inTextureListName,
	TMtPlaceHolder		*outTextureRef)
{
	UUtError			error;
	char				*texture_file_name;
	GRtElementType		element_type;
	BFtFileRef			*texture_file_ref;
	const char			*texture_name;
	TMtPlaceHolder		texture_ref;
	
	error =
		GRrGroup_GetElement(
			inGroup,
			inTextureListName,
			&element_type,
			&texture_file_name);
	if ((error != UUcError_None) && (element_type != GRcElementType_String))
	{
		if (error != GRcError_ElementNotFound)
		{
			Imp_PrintWarning("Unable to process the texture");
		}
		
		return error;
	}

	// create a new file ref using the file name
	error =
		BFrFileRef_DuplicateAndReplaceName(
			inSourceFile,
			texture_file_name,
			&texture_file_ref);
	IMPmError_ReturnOnErrorMsg(error, "texture file was not found");
	
	// set the textures name
	texture_name = BFrFileRef_GetLeafName(texture_file_ref);
	
	// process the texture map file
	error =
		Imp_ProcessTexture_File(
			texture_file_ref,
			texture_name,
			&texture_ref);
	IMPmError_ReturnOnErrorMsg(error, "unable to process the texture");
	
	BFrFileRef_Dispose(texture_file_ref);
	
	// save the texture ref
	*outTextureRef = texture_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiProcessTextures(
	BFtFileRef*			inSourceFile,
	GRtGroup			*inGroup,
	VMtTextureList		**inTextureList,
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
			VMcTemplate_TextureList,
			num_textures,
			inTextureList);
	IMPmError_ReturnOnError(error);
	
	// process the textures
	for (i = 0; i < num_textures; i++)
	{
		char			*texture_file;
		BFtFileRef		*texture_file_ref;
		const char			*texture_name;
		TMtPlaceHolder	texture_ref;
		FFtFileInfo		format;
		
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
		
		// look into the texture
		error = FFrPeek_2D(texture_file_ref, &format);
		IMPmError_ReturnOnErrorMsg(error, "could not get info about the file.");
		
		if ((format.width > M3cTextureMap_MaxWidth) ||
			(format.height > M3cTextureMap_MaxHeight))
		{
			// process the texture map file
			error =	Imp_ProcessTexture_Big_File(texture_file_ref, texture_name, &texture_ref);
			IMPmError_ReturnOnErrorMsg(error, "Could not import the texture file.");
		}
		else
		{
			// process the texture map file
			error =	Imp_ProcessTexture_File(texture_file_ref, texture_name, &texture_ref);
			IMPmError_ReturnOnErrorMsg(error, "Could not import the texture file.");
		}
		
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
static UUtError
IMPiView_Process_Box(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_Box				*box;
	TMtPlaceHolder			box_ref;
	
	// create the dialog template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_Box,
			inInstanceName,
			0,
			&box);
	IMPmError_ReturnOnErrorMsg(error, "Could not create a template");
	
	// initialize the fields
	box->box								= NULL;

	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"box",
			&box->box);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}

	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_Box,
			inInstanceName,
			&box_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = box_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_Button(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_Button			*button;
	TMtPlaceHolder			button_ref;
	UUtUns16				common_flags;
	UUtUns16				animation_flags;
	char					*button_title;
	
	// create the template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_Button,
			inInstanceName,
			0,
			&button);
	IMPmError_ReturnOnErrorMsg(error, "Could not create the template");
	
	// read the title
	error =
		GRrGroup_GetString(
			inGroup,
			"title",
			&button_title);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Unable to get view type");
	}
	else if (error == GRcError_ElementNotFound)
	{
		button->title[0] = '\0';
	}
	else
	{
		// only if the title exists does the text need to be copied, the 
		// common flags read, and the text offset location processed
		
		// copy the title text
		UUrString_Copy(button->title, button_title, VMcMaxTitleLength);
		
		// read the flags
		error =
			IMPiProcessViewFlags(
				inGroup,
				IMPgCommonFlags,
				"common_flags",
				&common_flags);
		IMPmError_ReturnOnErrorMsg(error, "Unable to process button flags");

		// get the text location offset
		error =
			IMPiProcessLocation(
				inGroup,
				"text_location_offset",
				&button->text_location_offset);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get the location");
	}
	
	// read the animation rate
	error =
		GRrGroup_GetUns16(
			inGroup,
			"anim_rate",
			&button->animation_rate);
	if (error != UUcError_None)
		button->animation_rate = VMcView_Button_DefaultAnimationRate;
	
	// read the animation flags
	error =
		IMPiProcessViewFlags(
			inGroup,
			IMPgAnimationFlags,
			"animation_flags",
			&animation_flags);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
		IMPmError_ReturnOnErrorMsg(error, "Unable to process button flags");
	
	button->flags = common_flags | animation_flags;
	
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"idle",
			&button->idle);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
		
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"mouse_over",
			&button->mouse_over);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
	
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"pressed",
			&button->pressed);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
	
	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_Button,
			inInstanceName,
			&button_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = button_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_CheckBox(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_CheckBox		*checkbox;
	TMtPlaceHolder			checkbox_ref;
	char					*checkbox_title;
	
	// create the dialog template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_CheckBox,
			inInstanceName,
			0,
			&checkbox);
	IMPmError_ReturnOnErrorMsg(error, "Could not create a template");
	
	// initialize the fields
	checkbox->flags						= VMcCommonFlag_None;
	checkbox->reserved					= 0;
	checkbox->title_location_offset.x	= 0;
	checkbox->title_location_offset.y	= 0;
	checkbox->title[0]					= 0;

	// read the flags
	error =
		IMPiProcessViewFlags(
			inGroup,
			IMPgCommonFlags,
			"common_flags",
			&checkbox->flags);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get the flags");
	
	// get the title location offset
	error =
		IMPiProcessLocation(
			inGroup,
			"title_location_offset",
			&checkbox->title_location_offset);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get the location");
		
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"outline",
			&checkbox->outline);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
		
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"cb_off",
			&checkbox->off);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
		
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"cb_on",
			&checkbox->on);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
		
	// read the title
	error =
		GRrGroup_GetString(
			inGroup,
			"title",
			&checkbox_title);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get string");
	
	UUrString_Copy(checkbox->title, checkbox_title, VMcMaxTitleLength);

	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_CheckBox,
			inInstanceName,
			&checkbox_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = checkbox_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_Dialog(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	DMtDialogData			*dialog;
	TMtPlaceHolder			dialog_ref;
	
	// create the dialog template
	error =
		TMrConstruction_Instance_Renew(
			DMcTemplate_DialogData,
			inInstanceName,
			0,
			&dialog);
	IMPmError_ReturnOnErrorMsg(error, "Could not create a dialog template");
	
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"partspec",
			&dialog->partspec);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
		
	// read the background textures
	error =
		IMPiProcessTextures(
			inSourceFile,
			inGroup,
			&dialog->b_textures,
			"b_list");
	IMPmError_ReturnOnErrorMsg(error, "Could not get the background textures");
	
	// read the flags
	error =
		IMPiProcessViewFlags(
			inGroup,
			IMPgDialogFlags,
			"dialog_flags",
			&dialog->flags);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get view flags");
	
	// initialize the fields
	dialog->reserved = 0;
	
	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			DMcTemplate_DialogData,
			inInstanceName,
			&dialog_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the dialog");
	
	*outViewData = dialog_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_EditField(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_EditField		*editfield;
	TMtPlaceHolder			editfield_ref;
	UUtUns16				temp;
	
	// create the template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_EditField,
			inInstanceName,
			0,
			&editfield);
	IMPmError_ReturnOnErrorMsg(error, "Could not create the template");
	
	// initialize the text
	editfield->flags = 0;
	editfield->max_chars = DMcControl_EditField_MaxChars;
	editfield->text_location_offset.x = 0;
	editfield->text_location_offset.y = 0;
	
	// read the flags
	error =
		IMPiProcessViewFlags(
			inGroup,
			IMPgCommonFlags,
			"common_flags",
			&editfield->flags);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get flags");
	
	// read the max_chars
	error =
		GRrGroup_GetUns16(
			inGroup,
			"max_chars",
			&temp);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Unable to get max_chars");
	}
	else
	{
		editfield->max_chars = temp;
	}
	
	// get the text location offset
	error =
		IMPiProcessLocation(
			inGroup,
			"text_location_offset",
			&editfield->text_location_offset);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get the location");
		
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"outline",
			&editfield->outline);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
		
	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_EditField,
			inInstanceName,
			&editfield_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = editfield_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_ListBox(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_ListBox			*listbox;
	TMtPlaceHolder			listbox_ref;
	
	// create the template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_ListBox,
			inInstanceName,
			0,
			&listbox);
	IMPmError_ReturnOnErrorMsg(error, "Could not create the template");
	
	// initialize the fields
	listbox->backborder			= NULL;
	listbox->flags				= VMcCommonFlag_None;
	listbox->reserved			= 0;

	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"backborder",
			&listbox->backborder);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}

	// read the flags
	error =
		IMPiProcessViewFlags(
			inGroup,
			IMPgCommonFlags,
			"common_flags",
			&listbox->flags);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get the flags");
	
	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_ListBox,
			inInstanceName,
			&listbox_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = listbox_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_Picture(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_Picture			*picture;
	TMtPlaceHolder			picture_ref;
	
	// create the template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_Picture,
			inInstanceName,
			0,
			&picture);
	IMPmError_ReturnOnErrorMsg(error, "Could not create the template");
	
	// read the background textures
	error =
		IMPiProcessTextures(
			inSourceFile,
			inGroup,
			&picture->b_textures,
			"b_list");
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not get the background textures");
	}
	
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"partspec",
			&picture->partspec);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
	
	if ((picture->b_textures == NULL) &&
		(picture->partspec == NULL))
	{
		IMPmError_ReturnOnErrorMsg(error, "Picture is not complete");
	}
	
	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_Picture,
			inInstanceName,
			&picture_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = picture_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_RadioButton(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_RadioButton		*radiobutton;
	TMtPlaceHolder			radiobutton_ref;
	char					*radiobutton_title;
	
	// create the dialog template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_RadioButton,
			inInstanceName,
			0,
			&radiobutton);
	IMPmError_ReturnOnErrorMsg(error, "Could not create a template");
	
	// initialize the fields
	radiobutton->flags						= VMcCommonFlag_None;
	radiobutton->reserved					= 0;
	radiobutton->title[0]					= 0;

	// read the flags
	error =
		IMPiProcessViewFlags(
			inGroup,
			IMPgCommonFlags,
			"common_flags",
			&radiobutton->flags);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get the flags");
	
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"rb_off",
			&radiobutton->off);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
		
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"rb_on",
			&radiobutton->on);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}

	// read the title
	error =
		GRrGroup_GetString(
			inGroup,
			"title",
			&radiobutton_title);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get string");
	
	UUrString_Copy(radiobutton->title, radiobutton_title, VMcMaxTitleLength);

	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_RadioButton,
			inInstanceName,
			&radiobutton_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = radiobutton_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_RadioGroup(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_RadioGroup		*radiogroup;
	TMtPlaceHolder			radiogroup_ref;
	
	// create the dialog template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_RadioGroup,
			inInstanceName,
			0,
			&radiogroup);
	IMPmError_ReturnOnErrorMsg(error, "Could not create a template");
	
	// initialize the fields
	radiogroup->outline						= NULL;

	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"outline",
			&radiogroup->outline);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}

	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_RadioGroup,
			inInstanceName,
			&radiogroup_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = radiogroup_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_Scrollbar(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_Scrollbar		*scrollbar;
	TMtPlaceHolder			scrollbar_ref;
	
	// create the dialog template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_Scrollbar,
			inInstanceName,
			0,
			&scrollbar);
	IMPmError_ReturnOnErrorMsg(error, "Could not create a template");
	
	// initialize the fields
	scrollbar->vertical_scrollbar	= NULL;
	scrollbar->vertical_thumb		= NULL;
	scrollbar->horizontal_scrollbar	= NULL;
	scrollbar->horizontal_thumb		= NULL;
		
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"vertical_scrollbar",
			&scrollbar->vertical_scrollbar);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}

	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"vertical_thumb",
			&scrollbar->vertical_thumb);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
	
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"horizontal_scrollbar",
			&scrollbar->horizontal_scrollbar);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}

	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"horizontal_thumb",
			&scrollbar->horizontal_thumb);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
	
	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_Scrollbar,
			inInstanceName,
			&scrollbar_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = scrollbar_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_Slider(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_Slider			*slider;
	TMtPlaceHolder			slider_ref;
	char					*title;
	
	// create the dialog template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_Slider,
			inInstanceName,
			0,
			&slider);
	IMPmError_ReturnOnErrorMsg(error, "Could not create a template");
	
	// initialize the fields
	slider->flags			= 0;
	slider->reserved		= 0;
	slider->outline			= NULL;
	slider->title[0]		= '\0';
	
	// read the title
	error =
		GRrGroup_GetString(
			inGroup,
			"title",
			&title);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get view type");
	
	UUrString_Copy(slider->title, title, VMcMaxTitleLength);
	
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"outline",
			&slider->outline);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
	
	// read the flags
	error =
		IMPiProcessViewFlags(
			inGroup,
			IMPgCommonFlags,
			"common_flags",
			&slider->flags);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get flags");

	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_Slider,
			inInstanceName,
			&slider_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = slider_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_Tab(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_Tab				*tab;
	TMtPlaceHolder			tab_ref;
	
	// create the template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_Tab,
			inInstanceName,
			0,
			&tab);
	IMPmError_ReturnOnErrorMsg(error, "Could not create the template");
	
	tab->tab_button = NULL;
	tab->outline = NULL;
	
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"tab_button",
			&tab->tab_button);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
	
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"outline",
			&tab->outline);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
	
	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_Tab,
			inInstanceName,
			&tab_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = tab_ref;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_TabGroup(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	*outViewData = 0;
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiView_Process_Text(
	BFtFileRef				*inSourceFile,
	GRtGroup				*inGroup,
	char					*inInstanceName,
	TMtPlaceHolder			*outViewData)
{
	UUtError				error;
	VMtView_Text			*text;
	TMtPlaceHolder			text_ref;
	char					*string;
	
	// create the template
	error =
		TMrConstruction_Instance_Renew(
			VMcTemplate_View_Text,
			inInstanceName,
			0,
			&text);
	IMPmError_ReturnOnErrorMsg(error, "Could not create the template");
	
	// initialize the text
	text->flags = 0;
	text->text_location_offset.x = 0;
	text->text_location_offset.y = 0;
	text->outline = NULL;
	text->string[0] = '\0';
	
	// read the flags
	error =
		IMPiProcessViewFlags(
			inGroup,
			IMPgCommonFlags,
			"common_flags",
			&text->flags);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get view flags");
	
	// get the text location offset
	error =
		IMPiProcessLocation(
			inGroup,
			"text_location_offset",
			&text->text_location_offset);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get the location");
	
	// get the string
	error =
		GRrGroup_GetString(
			inGroup,
			"string",
			&string);
	IMPmError_ReturnOnErrorMsg(error, "Unable to get the string");
	
	UUrString_Copy(text->string, string, VMcView_Text_MaxStringLength);
	
	// process the part specifications
	error =
		IMPiProcessPartSpecification(
			inSourceFile,
			inGroup,
			"outline",
			&text->outline);
	if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
	{
		IMPmError_ReturnOnErrorMsg(error, "Could not process part specification");
	}
	
	// get a placeholder
	error = 
		TMrConstruction_Instance_GetPlaceHolder(
			VMcTemplate_View_Text,
			inInstanceName,
			&text_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get placeholder for the template");
	
	*outViewData = text_ref;
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
static tViewProc		IMPgViewProc[] =
{
	{ "box",			VMcViewType_Box,			IMPiView_Process_Box },
	{ "button", 		VMcViewType_Button,			IMPiView_Process_Button },
	{ "checkbox",		VMcViewType_CheckBox,		IMPiView_Process_CheckBox },
	{ "dialog",			VMcViewType_Dialog,			IMPiView_Process_Dialog },
	{ "editfield",		VMcViewType_EditField,		IMPiView_Process_EditField },
	{ "listbox",		VMcViewType_ListBox,		IMPiView_Process_ListBox },
	{ "picture",		VMcViewType_Picture,		IMPiView_Process_Picture },
	{ "radiobutton",	VMcViewType_RadioButton,	IMPiView_Process_RadioButton },
	{ "radiogroup",		VMcViewType_RadioGroup,		IMPiView_Process_RadioGroup },
	{ "scrollbar",		VMcViewType_Scrollbar,		IMPiView_Process_Scrollbar },
	{ "slider",			VMcViewType_Slider,			IMPiView_Process_Slider },
	{ "tab",			VMcViewType_Tab,			IMPiView_Process_Tab },
	{ "tabgroup",		VMcViewType_TabGroup,		IMPiView_Process_TabGroup },
	{ "text",			VMcViewType_Text,			IMPiView_Process_Text },
	{ NULL,				VMcViewType_None,			NULL }
};


// ----------------------------------------------------------------------
UUtError
Imp_AddPartSpecification(
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
		compile_date = UUrConvertStrToSecsSince1900(gViewCompileTime);
		
		build_instance = (UUtBool)(create_date < inSourceFileModDate ||
									create_date < compile_date);
	}
	else
	{
		build_instance = UUcTrue;
	}
	
	if (build_instance)
	{
		GRtElementType			element_type;
		GRtElementArray			*part_spec_array;
		UUtUns16				num_parts;
		UUtUns16				i;
		VMtPartSpec				*part_spec;
		TMtPlaceHolder			texture;
		
		// get the texture
		error =
			IMPiProcessSingleTexture(
				inSourceFile,
				inGroup,
				"texture_map",
				&texture);
		if ((error != UUcError_None) && (error != GRcError_ElementNotFound))
		{
			IMPmError_ReturnOnErrorMsg(error, "Unable to process the colors texture");
		}

		// create an part specification template instance
		error =
			TMrConstruction_Instance_Renew(
				VMcTemplate_PartSpecification,
				inInstanceName,
				0,
				&part_spec);
		IMPmError_ReturnOnErrorMsg(error, "Could not create a part specification template");
		
		// set the texture
		part_spec->texture = (void*)texture;
		
		// clear the part matrices
		UUrMemory_Clear(part_spec->part_matrix_tl, sizeof(part_spec->part_matrix_tl));
		UUrMemory_Clear(part_spec->part_matrix_br, sizeof(part_spec->part_matrix_br));
		
		// get the parts specs
		error =
			GRrGroup_GetElement(
				inGroup,
				"part_specs",
				&element_type,
				&part_spec_array);
		if ((error != UUcError_None) || (element_type != GRcElementType_Array))
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get part specs array");
		}
		
		num_parts = (UUtUns16)GRrGroup_Array_GetLength(part_spec_array);
		
		// process the part specs
		for (i = 0; i < num_parts; i++)
		{
			GRtElementArray			*part_array;
			char					*name;
			char					*temp;
			UUtInt32				left;
			UUtInt32				top;
			UUtInt32				right;
			UUtInt32				bottom;
			tParts					*parts;
			
			// get the element i from the part spec array
			error =
				GRrGroup_Array_GetElement(
					part_spec_array,
					i,
					&element_type,
					&part_array);
			if ((error != UUcError_None) || (element_type != GRcElementType_Array))
			{	
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get part spec from array");
			}
			
			if (GRrGroup_Array_GetLength(part_array) != 5)
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "the part array in not correct");
			}
			
			// ------------------------------
			// process the elements of the part
			// ------------------------------
			
			// get the part name
			error =
				GRrGroup_Array_GetElement(
					part_array,
					0,
					&element_type,
					&name);
			if ((error != UUcError_None) || (element_type != GRcElementType_String))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "the part array in not correct");
			}
			
			// get the left coordinate of the part
			left = 0;
			error =
				GRrGroup_Array_GetElement(
					part_array,
					1,
					&element_type,
					&temp);
			if ((error != UUcError_None) || (element_type != GRcElementType_String))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "the part array in not correct");
			}

			sscanf(temp, "%d", &left);
			if ((left < 0) || (left > M3cTextureMap_MaxWidth))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "the part array in not correct");
			}
			
			// get the top coordinate of the part
			top = 0;
			error =
				GRrGroup_Array_GetElement(
					part_array,
					2,
					&element_type,
					&temp);
			if ((error != UUcError_None) || (element_type != GRcElementType_String))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "the part array in not correct");
			}

			sscanf(temp, "%d", &top);
			if ((top < 0) || (top > M3cTextureMap_MaxWidth))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "the part array in not correct");
			}
			
			// get the right coordinate of the part
			right = 0;
			error =
				GRrGroup_Array_GetElement(
					part_array,
					3,
					&element_type,
					&temp);
			if ((error != UUcError_None) || (element_type != GRcElementType_String))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "the part array in not correct");
			}

			sscanf(temp, "%d", &right);
			if ((right < 0) || (right > M3cTextureMap_MaxWidth))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "the part array in not correct");
			}
			
			// get the bottom coordinate of the part
			bottom = 0;
			error =
				GRrGroup_Array_GetElement(
					part_array,
					4,
					&element_type,
					&temp);
			if ((error != UUcError_None) || (element_type != GRcElementType_String))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "the part array in not correct");
			}

			sscanf(temp, "%d", &bottom);
			if ((bottom < 0) || (bottom > M3cTextureMap_MaxWidth))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "the part array in not correct");
			}
			
			// stick the elements into the template
			for (parts = IMPgParts; parts->name != NULL; parts++)
			{
				if (strcmp(name, parts->name) == 0)
				{
					part_spec->part_matrix_tl[parts->row][parts->column].x = (UUtUns16)left;
					part_spec->part_matrix_tl[parts->row][parts->column].y = (UUtUns16)top;
					part_spec->part_matrix_br[parts->row][parts->column].x = (UUtUns16)right;
					part_spec->part_matrix_br[parts->row][parts->column].y = (UUtUns16)bottom;
					break;
				}
			}
		}
	}
	
	return UUcError_None;
}

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
		compile_date = UUrConvertStrToSecsSince1900(gViewCompileTime);
		
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
			IMPiProcessViewFlags(
				inGroup,
				IMPgViewFlags,
				"view_flags",
				&flags);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get view flags");
		
		// get the location
		error =
			IMPiProcessLocation(
				inGroup,
				"location",
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
		view_data = 0;
		for (viewproc = IMPgViewProc;
			 viewproc->name != NULL;
			 viewproc++)
		{
			if (!strcmp(type_name, viewproc->name))
			{
				viewproc->view_proc(inSourceFile, inGroup, inInstanceName, &view_data);
				type = viewproc->type;
				break;
			}
		}
				
		// get the views array
		error =
			GRrGroup_GetElement(
				inGroup,
				"child_views",
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
				IMPmError_ReturnOnErrorMsg(error, "Could not process view");
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

