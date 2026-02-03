// ======================================================================
// Imp_PartSpec.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"
#include "WM_PartSpecification.h"

#include "Imp_Common.h"
#include "Imp_Texture.h"
#include "Imp_PartSpec.h"

// ======================================================================
// typedefs
// ======================================================================
typedef struct tParts
{
	char					*name;
	UUtUns16				row;
	UUtUns16				column;

} tParts;

// ======================================================================
// globals
// ======================================================================
static char*	gPartSpecCompileTime = __DATE__" "__TIME__;

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
IMPiAddPartSpecToUI(
	GRtGroup			*inGroup,
	char				*inPartName,
	PStPartSpec			**outPartSpec)
{
	UUtError			error;
	char				*partspec_name;
	TMtPlaceHolder		partspec_ref;

	error = GRrGroup_GetString(inGroup, inPartName, &partspec_name);
	IMPmError_ReturnOnError(error);

	error =
		TMrConstruction_Instance_GetPlaceHolder(
			PScTemplate_PartSpecification,
			partspec_name,
			&partspec_ref);
	IMPmError_ReturnOnErrorMsg(error, "Could not get partspec placeholder");

	*outPartSpec = (PStPartSpec*)partspec_ref;

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

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
Imp_AddPartSpec(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;


	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(PScTemplate_PartSpecification, inInstanceName);

	if (build_instance)
	{
		GRtElementType			element_type;
		GRtElementArray			*part_spec_array;
		UUtUns16				num_parts;
		UUtUns16				i;
		PStPartSpec				*part_spec;
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
				PScTemplate_PartSpecification,
				inInstanceName,
				0,
				&part_spec);
		IMPmError_ReturnOnErrorMsg(error, "Could not create a part specification instance");

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
Imp_AddPartSpecList(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;


	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(PScTemplate_PartSpecList, inInstanceName);

	if (build_instance)
	{
		GRtElementType			element_type;
		GRtElementArray			*partspecdesc_array;
		UUtUns32				num_partspecdescs;
		UUtUns32				i;
		PStPartSpecList			*partspec_list;

		// get the array of partspec descriptions
		error =
			GRrGroup_GetElement(
				inGroup,
				"list",
				&element_type,
				&partspecdesc_array);
		if ((error != UUcError_None) || (element_type != GRcElementType_Array))
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get part spec desc list array");
		}

		// get the number of partspec descriptions in the array
		num_partspecdescs = GRrGroup_Array_GetLength(partspecdesc_array);

		if (num_partspecdescs == 0)
		{
			return UUcError_None;
		}

		// create a partspeclist instance
		error =
			TMrConstruction_Instance_Renew(
				PScTemplate_PartSpecList,
				inInstanceName,
				num_partspecdescs,
				&partspec_list);
		IMPmError_ReturnOnErrorMsg(error, "Could not create a part spec list instance");

		for (i = 0; i < num_partspecdescs; i++)
		{
			GRtGroup				*partspecdesc_group;
			UUtUns32				partspectype;
			char					*partspec;
			TMtPlaceHolder			partspec_ref;

			// get a partspec desc group from the array
			error =
				GRrGroup_Array_GetElement(
					partspecdesc_array,
					i,
					&element_type,
					&partspecdesc_group);
			if ((error != UUcError_None) || (element_type != GRcElementType_Group))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get part spec desc group from array");
			}

			// process the elements of the partspec description
			error =
				GRrGroup_GetUns32(
					partspecdesc_group,
					"partspectype",
					&partspectype);
			IMPmError_ReturnOnErrorMsg(error, "Unable to get partspectype");

			error =
				GRrGroup_GetString(
					partspecdesc_group,
					"partspec",
					&partspec);
			IMPmError_ReturnOnErrorMsg(error, "Unable to get partspec");

			// save the information
			partspec_list->partspec_descs[i].partspec_type = partspectype;
			error =
				TMrConstruction_Instance_GetPlaceHolder(
					PScTemplate_PartSpecification,
					partspec,
					&partspec_ref);
			IMPmError_ReturnOnErrorMsg(error, "Could not get partspec placeholder");

			partspec_list->partspec_descs[i].partspec = (void*)partspec_ref;
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddPartSpecUI(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;


	// check to see if the dialogs need to be built
	build_instance = !TMrConstruction_Instance_CheckExists(PScTemplate_PartSpecUI, inInstanceName);

	if (build_instance)
	{
		PStPartSpecUI			*partspec_ui;

		// create a partspec_ui instance
		error =
			TMrConstruction_Instance_Renew(
				PScTemplate_PartSpecUI,
				inInstanceName,
				0,
				&partspec_ui);
		IMPmError_ReturnOnErrorMsg(error, "Could not create a part spec ui instance");

		error = IMPiAddPartSpecToUI(inGroup, "background", &partspec_ui->background);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "border", &partspec_ui->border);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "title", &partspec_ui->title);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "grow", &partspec_ui->grow);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "close_idle", &partspec_ui->close_idle);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "close_pressed", &partspec_ui->close_pressed);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "zoom_idle", &partspec_ui->zoom_idle);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "zoom_pressed", &partspec_ui->zoom_pressed);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "flatten_idle", &partspec_ui->flatten_idle);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "flatten_pressed", &partspec_ui->flatten_pressed);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "text_caret", &partspec_ui->text_caret);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "outline", &partspec_ui->outline);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "button_off", &partspec_ui->button_off);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "default_button", &partspec_ui->default_button);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "button_on", &partspec_ui->button_on);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "checkbox_on", &partspec_ui->checkbox_on);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "checkbox_off", &partspec_ui->checkbox_off);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "editfield", &partspec_ui->editfield);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "ef_hasfocus", &partspec_ui->ef_hasfocus);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "hilite", &partspec_ui->hilite);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "divider", &partspec_ui->divider);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "check", &partspec_ui->check);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "popup_menu", &partspec_ui->popup_menu);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "progressbar_track", &partspec_ui->progressbar_track);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "progressbar_fill", &partspec_ui->progressbar_fill);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "radio_button_on", &partspec_ui->radiobutton_on);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "radio_button_off", &partspec_ui->radiobutton_off);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "up_arrow", &partspec_ui->up_arrow);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "up_arrow_pressed", &partspec_ui->up_arrow_pressed);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "dn_arrow", &partspec_ui->dn_arrow);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "dn_arrow_pressed", &partspec_ui->dn_arrow_pressed);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "lt_arrow", &partspec_ui->lt_arrow);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "lt_arrow_pressed", &partspec_ui->lt_arrow_pressed);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "rt_arrow", &partspec_ui->rt_arrow);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "rt_arrow_pressed", &partspec_ui->rt_arrow_pressed);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "sb_thumb", &partspec_ui->sb_thumb);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "sb_v_track", &partspec_ui->sb_v_track);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "sb_h_track", &partspec_ui->sb_h_track);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "sl_thumb", &partspec_ui->sl_thumb);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "sl_track", &partspec_ui->sl_track);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "tab_active", &partspec_ui->tab_active);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "tab_inactive", &partspec_ui->tab_inactive);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "file", &partspec_ui->file);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");

		error = IMPiAddPartSpecToUI(inGroup, "folder", &partspec_ui->folder);
		IMPmError_ReturnOnErrorMsg(error, "could not get partspec");
	}

	return UUcError_None;
}
