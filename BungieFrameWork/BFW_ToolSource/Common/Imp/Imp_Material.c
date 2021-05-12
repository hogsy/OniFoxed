// ======================================================================
// Imp_Material.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"
#include "BFW_Materials.h"

#include "Imp_Common.h"
#include "Imp_Material.h"


// ======================================================================
// globals
// ======================================================================
static char*		gMaterialsCompileTime = __DATE__" "__TIME__;
static MAtMaterial *gBaseMaterial = NULL;
static MAtImpact *	gBaseImpact = NULL;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtError
Imp_AddMaterial(
	BFtFileRef			*inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup			*inGroup,
	char				*inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	
	// check to see if the material needs to be built
	build_instance = !TMrConstruction_Instance_CheckExists(MAcTemplate_Material, inInstanceName);
	
	if (build_instance)
	{
		char					*parent_name;
		MAtMaterial				*material;
		
		// get the name of the parent
		error = GRrGroup_GetString(inGroup, "parent", &parent_name);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get parent name");
		
		// build an instance of the material
		error =
			TMrConstruction_Instance_Renew(
				MAcTemplate_Material,
				inInstanceName,
				0,
				&material);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create material instance");
		
		// the real id will be set at runtime
		material->id = MAcInvalidID;

		// set the parent
		if (UUrString_Compare_NoCase(parent_name, "None") != 0)
		{
			// get an instance placeholder for the parent
			error =
				TMrConstruction_Instance_GetPlaceHolder(
					MAcTemplate_Material,
					parent_name,
					(TMtPlaceHolder*)&(material->parent));
			UUmError_ReturnOnErrorMsg(error, "could not set material's parent placeholder");
		}
		else
		{
			material->parent = NULL;

			// there must only be one base material
			if (gBaseMaterial != NULL) {
				char errmsg[128];

				sprintf(errmsg, "cannot have more than one base material (both %s and %s have parent 'None')",
						inInstanceName, TMrInstance_GetInstanceName(gBaseMaterial));
				UUmError_ReturnOnErrorMsg(UUcError_Generic, errmsg);
			} else {
				gBaseMaterial = material;
			}
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddImpact(
	BFtFileRef			*inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup			*inGroup,
	char				*inInstanceName)
{
	UUtError			error;

	UUtBool				build_instance;
	
	
	// check to see if the impact needs to be built
	build_instance = !TMrConstruction_Instance_CheckExists(MAcTemplate_Impact, inInstanceName);
	
	if (build_instance)
	{
		char					*parent_name;
		MAtImpact				*impact;
		
		// get the name of the parent
		error = GRrGroup_GetString(inGroup, "parent", &parent_name);
		IMPmError_ReturnOnErrorMsg(error, "Unable to get parent name");
		
		// build an instance of the impact
		error =
			TMrConstruction_Instance_Renew(
				MAcTemplate_Impact,
				inInstanceName,
				0,
				&impact);
		IMPmError_ReturnOnErrorMsg(error, "Unable to create impact instance");
		
		// the real id will be set at runtime
		impact->id = MAcInvalidID;

		// set the parent
		if (UUrString_Compare_NoCase(parent_name, "None") != 0)
		{
			// get an instance placeholder for the parent
			error =
				TMrConstruction_Instance_GetPlaceHolder(
					MAcTemplate_Impact,
					parent_name,
					(TMtPlaceHolder*)&(impact->parent));
			UUmError_ReturnOnErrorMsg(error, "could not set impact's parent placeholder");
		}
		else
		{
			impact->parent = NULL;

			// there must only be one base impact
			if (gBaseImpact != NULL) {
				char errmsg[128];

				sprintf(errmsg, "cannot have more than one base impact (both %s and %s have parent 'None')",
						inInstanceName, TMrInstance_GetInstanceName(gBaseImpact));
				UUmError_ReturnOnErrorMsg(UUcError_Generic, errmsg);
			} else {
				gBaseImpact = impact;
			}
		}
	}
	
	return UUcError_None;
}

