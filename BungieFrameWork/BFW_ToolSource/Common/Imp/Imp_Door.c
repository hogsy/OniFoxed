// ======================================================================
// Imp_Door.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================

#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"

#include "Imp_Common.h"
#include "Imp_ParseEnvFile.h"
#include "Imp_Env2_Private.h"
#include "Imp_Model.h"
#include "Imp_Texture.h"
#include "Imp_Door.h"
#include "Imp_Furniture.h"

#include "EnvFileFormat.h"

#include "Oni_Object.h"
#include "Oni_Object_Private.h"

// ======================================================================
// flags
// ======================================================================

static AUtFlagElement	IMPgDoorFlags[] =
{
	{ "none",						WMcWindowFlag_None },
	{ "visible",					WMcWindowFlag_Visible },
	{ "disabled",					WMcWindowFlag_Disabled },
	{ "mouse_transparent",			WMcWindowFlag_MouseTransparent },
	{ NULL,							0 }
};
// ======================================================================
// prototypes
// ======================================================================

// ======================================================================
// functions
// ======================================================================
static UUtError IMPiDoor_ImportFurnitureGeometryArray( BFtFileRef* inSourceFileRef, OBJtFurnGeomArray **ioGeomArray )
{
	UUtError				error;	
	MXtHeader				*header;
	OBJtFurnGeomArray		*geom_array;
	char					name[BFcMaxFileNameLength];
	char					*textureName;
	UUtUns32				i;
	UUtUns32				num_geoms;
	UUtUns32				geom_index;
	UUtUns32				m;

	UUmAssert( inSourceFileRef && ioGeomArray );

	// parse the .env file
	error = Imp_ParseEnvFile(inSourceFileRef, &header);
	IMPmError_ReturnOnError(error);
		
	// copy the file name into name
	UUrString_Copy(name, BFrFileRef_GetLeafName(inSourceFileRef), BFcMaxFileNameLength);
	UUrString_StripExtension(name);
	
	num_geoms = 0;
	
	// calculate the number of geometries, grab the screen node and action marker
	for( i = 0; i < header->numNodes; i++ )
	{
		num_geoms += header->nodes[i].numMaterials;
	}

	// build the geometry array instance
	error = TMrConstruction_Instance_NewUnique( OBJcTemplate_FurnGeomArray, num_geoms, &geom_array );
	IMPmError_ReturnOnError(error);

	// initialize the geoms and get the flag and light data
	for (i = 0, geom_index = 0; i < header->numNodes; i++)
	{
		for (m = 0; m < header->nodes[i].numMaterials; m++, geom_index++)
		{
			// initialize the furn_geom
			geom_array->furn_geom[geom_index].gq_flags = AKcGQ_Flag_None;
			geom_array->furn_geom[geom_index].geometry = NULL;
			geom_array->furn_geom[geom_index].ls_data = NULL;
			
			// get the GQ flags
			geom_array->furn_geom[geom_index].gq_flags = IMPrEnv_GetNodeFlags(&header->nodes[i]);
			
			// get the light data
			IMPrFurniture_GetLightData( &header->nodes[i], i, &geom_array->furn_geom[geom_index]);
		}
	}
		
	for (i = 0, geom_index = 0; i < header->numNodes; i++)
	{
		for (m = 0; m < header->nodes[i].numMaterials; m++, geom_index++)
		{
			M3tGeometry		*geometry;
			
			// build the geometry instance
			error = TMrConstruction_Instance_NewUnique( M3cTemplate_Geometry, 0, &geometry );
			IMPmError_ReturnOnError(error);
			
			geometry->animation = NULL;
			
			// put the tris and quads associated with material m into a geometry
			Imp_NodeMaterial_To_Geometry(&header->nodes[i], (UUtUns16)m, geometry);
			
			textureName = header->nodes[i].materials[0].maps[MXcMapping_DI].name;
			
			geometry->baseMap = M3rTextureMap_GetPlaceholder_StripExtension_UpperCase(textureName);
			UUmAssert(geometry->baseMap);

			// set the geometry
			geom_array->furn_geom[geom_index].geometry = geometry;
		}
	}


	*ioGeomArray		= geom_array;

	// delete the header
	Imp_EnvFile_Delete(header);

	return UUcError_None;
}

static UUtError IMPiAddDoor( BFtFileRef* inSourceFileRef, GRtGroup* inGroup, char* inInstanceName)
{
	BFtFileRef				*file_ref;
	UUtError				error;	
	OBJtDoorClass			*door;
	GRtElementType			elementType;
	char					*model_path, *string;
	GRtElementType			element_type;
	GRtElementArray			*flags_array;
	UUtUns32				flags;

	// build the door instance
	error = TMrConstruction_Instance_Renew( OBJcTemplate_DoorClass, inInstanceName, 0, &door );
	IMPmError_ReturnOnError(error);

	door->geometry_array[0]	= NULL;
	door->geometry_array[1]	= NULL;
	door->animation			= NULL;

	// parse the flags
	error = GRrGroup_GetElement( inGroup, "flags", &element_type, &flags_array);
	if( error != UUcError_None )
	{
		flags = 0;
	}
	else
	{
		error = AUrFlags_ParseFromGroupArray( IMPgDoorFlags, element_type, flags_array, &flags );
		IMPmError_ReturnOnErrorMsg(error, "Unable to process flags");
	}

	// animation
	error = GRrGroup_GetString( inGroup, "animation", &model_path );
	if(error != UUcError_None )
	{
		UUmAssert( UUcFalse );
	}
	else
	{
		error = TMrConstruction_Instance_GetPlaceHolder( OBcTemplate_Animation, model_path, (TMtPlaceHolder*)&door->animation );
		IMPmError_ReturnOnErrorMsg(error, "Could not create place holder for animation");
	}

	// find the first model name
	error = GRrGroup_GetElement( inGroup, "model", &elementType, &model_path );
	if(error != UUcError_None || elementType != GRcElementType_String)
	{
		Imp_PrintWarning("File \"%s\": Could not get model string",  (UUtUns32)BFrFileRef_GetLeafName(inSourceFileRef), 0, 0);
		IMPmError_ReturnOnError(error);
	}

	// grab the first model
	error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, model_path, &file_ref);
	IMPmError_ReturnOnError(error);

	error = IMPiDoor_ImportFurnitureGeometryArray( file_ref, &door->geometry_array[0] );
	IMPmError_ReturnOnError(error);

	// dispose of the file ref
	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;

	// find the second model name
	error = GRrGroup_GetElement( inGroup, "model2", &elementType, &model_path );
	if(error == UUcError_None && elementType == GRcElementType_String)
	{
		// grab the second model
		error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, model_path, &file_ref);
		IMPmError_ReturnOnError(error);

		error = IMPiDoor_ImportFurnitureGeometryArray( file_ref, &door->geometry_array[1] );
		IMPmError_ReturnOnError(error);

		// dispose of the file ref
		BFrFileRef_Dispose(file_ref);
		file_ref = NULL;
	}

	/*
	 * read sound propagation values
	 */

	error = GRrGroup_GetFloat(inGroup, "muffle_fraction", &door->sound_muffle_fraction);
	if (error != UUcError_None) {
		// default value
		door->sound_muffle_fraction = 0.5f;
	}

	error = GRrGroup_GetString(inGroup, "allow_sounds", &string);
	if (error == UUcError_None) {
		if (UUmString_IsEqual(string, "all")) {
			door->sound_allow = AI2cSoundAllow_All;

		} else if (UUmString_IsEqual(string, "shouting")) {
			door->sound_allow = AI2cSoundAllow_Shouting;

		} else if (UUmString_IsEqual(string, "gunfire")) {
			door->sound_allow = AI2cSoundAllow_Gunfire;

		} else if (UUmString_IsEqual(string, "none")) {
			door->sound_allow = AI2cSoundAllow_None;

		} else {
			error = UUcError_Generic;
		}
	}
	if (error != UUcError_None) {
		// default value
		door->sound_allow = AI2cSoundAllow_Gunfire;
	}

	/*
	 * read AI sound values
	 */

	error = GRrGroup_GetString(inGroup, "ai_soundtype", &string);
	if (error == UUcError_None) {
		if (UUmString_IsEqual(string, "unimportant")) {
			door->ai_soundtype = AI2cContactType_Sound_Ignore;

		} else if (UUmString_IsEqual(string, "interesting")) {
			door->ai_soundtype = AI2cContactType_Sound_Interesting;

		} else if (UUmString_IsEqual(string, "danger")) {
			door->ai_soundtype = AI2cContactType_Sound_Danger;

		} else if (UUmString_IsEqual(string, "melee")) {
			door->ai_soundtype = AI2cContactType_Sound_Melee;

		} else if (UUmString_IsEqual(string, "gunfire")) {
			door->ai_soundtype = AI2cContactType_Sound_Gunshot;

		} else {
			error = UUcError_Generic;
		}
	}
	if (error == UUcError_None) {
		door->ai_soundradius = 100.0f;
		GRrGroup_GetFloat(inGroup, "ai_soundradius", &door->ai_soundradius);
	} else {
		// default values
		door->ai_soundtype = (UUtUns32) -1;
		door->ai_soundradius = 100.0f;
	}

	/*
	 * read open and close impulse sound names
	 */

	error = GRrGroup_GetString(inGroup, "open_sound", &string);
	if (error != UUcError_None) {
		string = "";
	}
	UUrString_Copy(door->open_soundname, string, BFcMaxFileNameLength);

	error = GRrGroup_GetString(inGroup, "close_sound", &string);
	if (error != UUcError_None) {
		string = "";
	}
	UUrString_Copy(door->close_soundname, string, BFcMaxFileNameLength);

	// these pointers will be set up at runtime
	door->open_sound = NULL;
	door->close_sound = NULL;

	return UUcError_None;
}


// ----------------------------------------------------------------------

UUtError Imp_AddDoor( BFtFileRef* inSourceFileRef, UUtUns32 inSourceFileModDate, GRtGroup* inGroup, char* inInstanceName )
{
	UUtError				error;

	error = IMPiAddDoor( inSourceFileRef, inGroup, inInstanceName );
	IMPmError_ReturnOnError(error);
	
	return UUcError_None;
}

