// ======================================================================
// Imp_Trigger.c
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
#include "Imp_Console.h"
#include "Imp_Furniture.h"

#include "EnvFileFormat.h"
#include "Oni_Object_Private.h"

// ======================================================================
// prototypes
// ======================================================================

// ======================================================================
// functions
// ======================================================================

static UUtError IMPiAddConsole( BFtFileRef* inSourceFileRef, GRtGroup* inGroup, char* inInstanceName)
{
	BFtFileRef				*file_ref;
	UUtError				error;
	MXtHeader				*header;
	OBJtFurnGeomArray		*geom_array;
	M3tGeometry				*screen_geom;
	char					name[BFcMaxFileNameLength];
	char					*textureName;
	char					default_texture[BFcMaxFileNameLength];
	UUtUns32				i;
	UUtUns32				num_geoms;
	UUtUns32				geom_index;
	UUtUns32				m;
	UUtUns32				is_alarm;
	OBJtConsoleClass		*console;
	GRtElementType			elementType;
	char					*model_path;
	UUtInt32				screen_node;
	MXtMarker				*action_marker;
	M3tVector3D				action_point;
	M3tVector3D				action_vector;
	char					*default_basemap;
	UUtUns32				gq_flags;

	// find the model name
	error = GRrGroup_GetElement( inGroup, "model", &elementType, &model_path );
	if(error != UUcError_None || elementType != GRcElementType_String)
	{
		Imp_PrintWarning("File \"%s\": Could not get model string",  (UUtUns32)BFrFileRef_GetLeafName(inSourceFileRef), 0, 0);
		IMPmError_ReturnOnError(error);
	}

	// init locals
	header			= NULL;
	geom_array		= NULL;
	num_geoms		= 0;
	screen_node		= -1;
	screen_geom		= NULL;
	action_marker	= NULL;

	error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, model_path, &file_ref);
	IMPmError_ReturnOnError(error);

	// look for a basemap
	error = GRrGroup_GetElement( inGroup, "basemap", &elementType, &default_basemap );
	if(error != UUcError_None || elementType != GRcElementType_String)
	{
		default_basemap = NULL;
	}

	// parse the .env file
	error = Imp_ParseEnvFile(file_ref, &header);
	IMPmError_ReturnOnError(error);

	// copy the file name into name
	UUrString_Copy(name, BFrFileRef_GetLeafName(file_ref), BFcMaxFileNameLength);
	UUrString_StripExtension(name);

	// calculate the number of geometries, grab the screen node and action marker
	for (i = 0; i < header->numNodes; i++)
	{
		if( UUmString_IsEqual_NoCase( header->nodes[i].name, "screen" ) )
		{
			UUmAssert( screen_node == -1 );
			UUmAssert( header->nodes[i].numMaterials == 1 );
			screen_node = i;
			continue;
		}
		if( !action_marker )
		{
			action_marker = Imp_EnvFile_GetMarker( &header->nodes[i], "console" );
		}
		num_geoms += header->nodes[i].numMaterials;
	}

	if( screen_node == -1 || !action_marker )
	{
		UUmAssert( UUcFalse );
		return UUcError_Generic;
	}

	// build the geometry array instance
	error = TMrConstruction_Instance_NewUnique( OBJcTemplate_FurnGeomArray, num_geoms, &geom_array );
	IMPmError_ReturnOnError(error);

	// initialize the geoms and get the flag and light data
	for (i = 0, geom_index = 0; i < header->numNodes; i++)
	{
		if( i == (UUtUns32) screen_node )		continue;
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
		if( i == (UUtUns32) screen_node )		continue;
		for (m = 0; m < header->nodes[i].numMaterials; m++, geom_index++)
		{
			M3tGeometry		*geometry;

			// build the geometry instance
			error = TMrConstruction_Instance_NewUnique( M3cTemplate_Geometry, 0, &geometry );
			IMPmError_ReturnOnError(error);

			geometry->animation = NULL;

			// put the tris and quads associated with material m into a geometry
			Imp_NodeMaterial_To_Geometry_ApplyMatrix(&header->nodes[i], (UUtUns16)m, geometry);

			// grab the texture basemap
			if( default_basemap )
				textureName = default_basemap;
			else
				textureName = header->nodes[i].materials[0].maps[MXcMapping_DI].name;

			// grab a placeholder to the texture
			geometry->baseMap = M3rTextureMap_GetPlaceholder_StripExtension_UpperCase(textureName);
			UUmAssert(geometry->baseMap);

			// set the geometry
			geom_array->furn_geom[geom_index].geometry = geometry;
		}
	}

	// create the screen
	if( screen_node != -1 )
	{
		error = TMrConstruction_Instance_NewUnique( M3cTemplate_Geometry, 0, &screen_geom );
		IMPmError_ReturnOnError(error);

		// get the GQ flags
		gq_flags = IMPrEnv_GetNodeFlags( &header->nodes[screen_node] );

		error = Imp_Node_To_Geometry_ApplyMatrix( &header->nodes[screen_node], screen_geom );
		UUmError_ReturnOnErrorMsg(error, "failed to build the geometry");

		// set a place holder for the default texture
		textureName = header->nodes[screen_node].materials[0].maps[MXcMapping_DI].name;

		screen_geom->baseMap = M3rTextureMap_GetPlaceholder_StripExtension_UpperCase("NONE");
		UUmAssert(screen_geom->baseMap);

		UUrString_Copy( default_texture, "NONE", BFcMaxFileNameLength );

		IMPmError_ReturnOnErrorMsg(error, "Could not create screen texture place holder");
	}

	// calculate action vector
	{
		const M3tPoint3D cPosYPoint = { 0.f, 1.f, 0.f };
		const M3tPoint3D cZeroPoint = { 0.f, 0.f, 0.f };
		M3tMatrix4x3		temp;
		M3tPoint3D			from,to;

		temp = action_marker->matrix;
		MUrMatrix_MultiplyPoint(&cZeroPoint, &temp, &from);
		MUrMatrix_MultiplyPoint(&cPosYPoint, &temp, &to);
		MUmVector_Subtract(action_vector, to, from);
		MUrNormalize(&action_vector);
		action_point.x = from.x + ( action_vector.x * 2);
		action_point.y = from.y + ( action_vector.y * 2);
		action_point.z = from.z + ( action_vector.z * 2);
	}

	// dispose of the file ref
	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;

	// build the console instance
	error = TMrConstruction_Instance_Renew( OBJcTemplate_ConsoleClass, inInstanceName, 0, &console );
	IMPmError_ReturnOnError(error);

	console->flags				= 0;
	console->screen_geometry	= screen_geom;
	console->geometry_array		= geom_array;
	console->action_position	= action_point;
	console->action_vector		= action_vector;
	console->screen_gq_flags	= gq_flags;

	// grab screen textures

	// inactive state
	error = GRrGroup_GetElement( inGroup, "inactive_screen", &elementType, &textureName );
	if(error != UUcError_None || elementType != GRcElementType_String)
	{
		UUrString_Copy( console->screen_inactive, default_texture, BFcMaxFileNameLength );
	}
	else
	{
		UUrString_Copy( console->screen_inactive, textureName, BFcMaxFileNameLength );
	}

	// active state
	error = GRrGroup_GetElement( inGroup, "active_screen", &elementType, &textureName );
	if(error != UUcError_None || elementType != GRcElementType_String)
	{
		UUrString_Copy( console->screen_active, default_texture, BFcMaxFileNameLength );
	}
	else
	{
		UUrString_Copy( console->screen_active, textureName, BFcMaxFileNameLength );
	}

	error = GRrGroup_GetElement( inGroup, "triggered_screen", &elementType, &textureName );
	if(error != UUcError_None || elementType != GRcElementType_String)
	{
		UUrString_Copy( console->screen_triggered, default_texture, BFcMaxFileNameLength );
	}
	else
	{
		UUrString_Copy( console->screen_triggered, textureName, BFcMaxFileNameLength );
	}

	// work out if the console is an alarm, and so prompts with a different message
	error = GRrGroup_GetUns32(inGroup, "alarmPrompt", &is_alarm);
	if ((error == UUcError_None) && (is_alarm)) {
		console->flags |= OBJcConsoleClassFlag_AlarmPrompt;
	}

	// delete the header
	Imp_EnvFile_Delete(header);

	return UUcError_None;
}

// ----------------------------------------------------------------------

UUtError Imp_AddConsole( BFtFileRef* inSourceFileRef, UUtUns32 inSourceFileModDate, GRtGroup* inGroup, char* inInstanceName )
{
	UUtError				error;

	error = IMPiAddConsole( inSourceFileRef, inGroup, inInstanceName );
	IMPmError_ReturnOnError(error);

	return UUcError_None;
}

