// ======================================================================
// Imp_UnitViewer.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"

#include "Oni_UnitViewer.h"
#include "Oni_Character.h"
#include "Oni_Weapon.h"

#include "Imp_Common.h"
#include "Imp_UnitViewer.h"

// ======================================================================
// globals
// ======================================================================
static char*	gUVCompileTime = __DATE__" "__TIME__;

static AUtFlagElement	IMPgDataTypeFlags[] =
{
	{ "character",					UVcDataType_Character },
	{ "weapon",						UVcDataType_Weapon },
	{ "url",						UVcDataType_URL },
	{ NULL,							0 }
};

// ======================================================================
// functions
// ======================================================================
UUtError
Imp_AddDataList(
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
		compile_date = UUrConvertStrToSecsSince1900(gUVCompileTime);

		build_instance = (UUtBool)(create_date < inSourceFileModDate ||
									create_date < compile_date);
	}
	else
	{
		build_instance = UUcTrue;
	}

	if (build_instance)
	{
		UUtUns32			i;
		UUtUns32			num_groups;
		char				*type;

		GRtElementType		element_type;
		GRtElementArray		*data_array;
		UVtDataList			*data_list;

		// get the data array
		error =
			GRrGroup_GetElement(
				inGroup,
				"data",
				&element_type,
				&data_array);
		if ((error != UUcError_None) || (element_type != GRcElementType_Array))
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Can't process the data");
		}

		// get the number of data groups in the data array
		num_groups = GRrGroup_Array_GetLength(data_array);

		// create an view template instance
		error =
			TMrConstruction_Instance_Renew(
				UVcTemplate_DataList,
				inInstanceName,
				num_groups,
				&data_list);
		IMPmError_ReturnOnErrorMsg(error, "Could not create the data list template");

		// get the data type
		error =
			GRrGroup_GetString(
				inGroup,
				"type",
				&type);
		IMPmError_ReturnOnErrorMsg(error, "Data list type not found in _ins.txt");

		error =
			AUrFlags_ParseFromString(
				type,
				UUcFalse,
				IMPgDataTypeFlags,
				&data_list->data_type);
		IMPmError_ReturnOnErrorMsg(error, "Unknown data list type");

		// process the elements of the data array
		for (i = 0; i < num_groups; i++)
		{
			GRtGroup			*data_group;
			char				*name;
			char				*template_name;

			// ------------------------------
			// get the data group at index i
			// ------------------------------
			error =
				GRrGroup_Array_GetElement(
					data_array,
					i,
					&element_type,
					&data_group);
			if ((error != UUcError_None) || (element_type != GRcElementType_Group))
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not get the data group");
			}

			// ------------------------------
			// get the template ref
			// ------------------------------
			error =
				GRrGroup_GetString(
					data_group,
					"template",
					&template_name);
			IMPmError_ReturnOnErrorMsg(error, "Unable to get template name");

			UUrString_Copy(
				data_list->data[i].instance_name,
				template_name,
				UVcMaxInstNameLength);

			// ------------------------------
			// get the name
			// ------------------------------
			error =
				GRrGroup_GetString(
					data_group,
					"name",
					&name);

			UUrString_Copy(
				data_list->data[i].name,
				name,
				UVcMaxDataNameLength);
		}
	}

	return UUcError_None;
}
