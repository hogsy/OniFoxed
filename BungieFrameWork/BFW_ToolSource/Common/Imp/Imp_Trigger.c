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
#include "Imp_Trigger.h"
#include "Imp_Texture.h"

#include "EnvFileFormat.h"

#include "Oni_Object_Private.h"

UUtError Imp_ProcessTexture_Dir( BFtFileRef *inFileRef, const char *inInstanceName, TMtPlaceHolder *outTextureRef);

// ======================================================================
// functions
// ======================================================================


// ----------------------------------------------------------------------
UUtError Imp_AddTriggerEmitter( BFtFileRef* inSourceFileRef, UUtUns32 inSourceFileModDate, GRtGroup* inGroup, char* inInstanceName)
{
	MXtMarker					*laser = NULL;
	UUtError					error;
	M3tMatrix4x3				temp;

	M3tPoint3D					from,to;
	MXtHeader					*header;
	BFtFileRef					*model_file;
	OBJtTriggerEmitterClass		*emitter;
	char						*temp_name;

	// build the trigger emitter instance
	error = TMrConstruction_Instance_Renew( OBJcTemplate_TriggerEmitterClass, inInstanceName, 0, &emitter);
	IMPmError_ReturnOnError(error);

	error = GRrGroup_GetString(inGroup, "model", &temp_name);
	IMPmError_ReturnOnError(error);

	error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, temp_name, &model_file);
	IMPmError_ReturnOnError(error);

	error = Imp_ParseEnvFile(model_file, &header);
	IMPmError_ReturnOnError(error);

	// dispose of the file ref
	BFrFileRef_Dispose( model_file );

	if (header->numNodes > 1) 
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Emitter file had > 1 node!");
	}
	
	// build the geometry instance
	error = TMrConstruction_Instance_NewUnique( M3cTemplate_Geometry, 0, &emitter->geometry );
	IMPmError_ReturnOnError(error);

	emitter->geometry->animation = NULL;

	// grab the laser sight
	laser = Imp_EnvFile_GetMarker(header->nodes + 0, "laser");
	if (laser)
	{
		const M3tPoint3D cPosYPoint = { 0.f, 1.f, 0.f };
		const M3tPoint3D cPosZPoint = { 0.f, 0.f, 1.f };
		const M3tPoint3D cZeroPoint = { 0.f, 0.f, 0.f };
		temp = laser->matrix;
		MUrMatrix_MultiplyPoint(&cZeroPoint, &temp, &from);
		MUrMatrix_MultiplyPoint(&cPosYPoint, &temp, &to);
		MUmVector_Subtract(emitter->emit_vector, to, from);
		MUrNormalize(&emitter->emit_vector);
		MUmVector_Copy(emitter->emit_position, from);
	}		
	else
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Trigger emitter did not have a 'Laser' marker!");
	}

	// get the GQ flags
	emitter->gq_flags = IMPrEnv_GetNodeFlags( header->nodes );
		
	// put the tris and quads associated with material m into a geometry
	Imp_NodeMaterial_To_Geometry_ApplyMatrix( header->nodes, (UUtUns16)0, emitter->geometry );
					
	// get the texture name
	temp_name = header->nodes[0].materials[0].maps[MXcMapping_DI].name;

	emitter->geometry->baseMap = M3rTextureMap_GetPlaceholder_StripExtension_UpperCase(temp_name);
	UUmAssert(emitter->geometry->baseMap);
		
	// delete the header
	Imp_EnvFile_Delete(header);

	return UUcError_None;
}
// ----------------------------------------------------------------------
UUtError Imp_AddTrigger( BFtFileRef* inSourceFileRef, UUtUns32 inSourceFileModDate, GRtGroup* inGroup, char* inInstanceName)
{
	UUtError				error;
	char					*temp_name;
	char					*soundName;
	OBJtTriggerClass		*trigger;
	float					temp_float;

	// build the trigger instance
	error = TMrConstruction_Instance_Renew( OBJcTemplate_TriggerClass, inInstanceName, 0, &trigger );
	IMPmError_ReturnOnError(error);

	// base Geometry
	{
		BFtFileRef		*model_file;
		MXtHeader		*header;

		error = GRrGroup_GetString(inGroup, "base", &temp_name);
		IMPmError_ReturnOnError(error);

		error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, temp_name, &model_file);
		IMPmError_ReturnOnError(error);

		error = Imp_ParseEnvFile(model_file, &header);
		IMPmError_ReturnOnError(error);

		BFrFileRef_Dispose(model_file);

		if (header->numNodes > 1) 
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Trigger base model had > 1 node!");
		}

		error = TMrConstruction_Instance_NewUnique( M3cTemplate_Geometry, 0, &trigger->base_geometry );
		IMPmError_ReturnOnError(error);

		// get the GQ flags
		trigger->base_gq_flags = IMPrEnv_GetNodeFlags( header->nodes );
		
		error = Imp_Node_To_Geometry_ApplyMatrix( header->nodes, trigger->base_geometry );
		UUmError_ReturnOnErrorMsg(error, "failed to build the geometry");
				
		// get the light data
		//IMPiTurret_GetLightData( &header->nodes[0], 0, &trigger->base_ls_data );
		
		// get the texture name
		temp_name = header->nodes[0].materials[0].maps[MXcMapping_DI].name;

		// grab a placeholder to the texture
		trigger->base_geometry->baseMap = M3rTextureMap_GetPlaceholder_StripExtension_UpperCase(temp_name);
		UUmAssert(trigger->base_geometry->baseMap);

		// delete env file
		Imp_EnvFile_Delete(header);
	}
	
	// emitter 
	{
		error = GRrGroup_GetString(inGroup, "emitter", &temp_name);
		IMPmError_ReturnOnErrorMsg(error, "Trigger has no emitter");

		error = TMrConstruction_Instance_GetPlaceHolder( OBJcTemplate_TriggerEmitterClass, temp_name, (TMtPlaceHolder*)&trigger->emitter );
		IMPmError_ReturnOnErrorMsg(error, "Could not create place holder for emitter");
	}

	// animation
	{
		error = GRrGroup_GetString(inGroup, "animation", &temp_name);
		if(error != UUcError_None )
		{
			trigger->animation = NULL;
		}
		else
		{
			error = TMrConstruction_Instance_GetPlaceHolder( OBcTemplate_Animation, temp_name, (TMtPlaceHolder*)&trigger->animation );
			IMPmError_ReturnOnErrorMsg(error, "Could not create place holder for animation");
		}
	}

	// set default properties
	trigger->start_offset		= 0.0;
	trigger->anim_scale			= 1.0;
	trigger->time_on			= 0;
	trigger->time_off			= 0;
	trigger->color				= 0x7FFF0000;
	
	// read properties
	error = GRrGroup_GetFloat( inGroup, "start_offset", &temp_float );
	if(error == UUcError_None )
		trigger->start_offset	= temp_float;

	error = GRrGroup_GetFloat( inGroup, "anim_scale", &temp_float );
	if(error == UUcError_None )
		trigger->anim_scale		= temp_float;

	error = GRrGroup_GetFloat( inGroup, "time_on", &temp_float );
	if(error == UUcError_None )
		trigger->time_on		= (UUtUns16) ( temp_float * UUcFramesPerSecond );

	error = GRrGroup_GetFloat( inGroup, "time_off", &temp_float );
	if(error == UUcError_None )
		trigger->time_off		= (UUtUns16) ( temp_float * UUcFramesPerSecond );

	error = GRrGroup_GetString( inGroup, "color", &temp_name );
	if(error == UUcError_None )
		sscanf( temp_name, "%x", &trigger->color );


	// import all textures in this directory...
	{
		BFtFileRef				*texture_dir;

		// create the texture directory file ref
		error = BFrFileRef_GetParentDirectory( inSourceFileRef, &texture_dir);

		//error =  BFrFileRef_DuplicateAndReplaceName( inSourceFileRef, "", &texture_dir);
		IMPmError_ReturnOnError(error);

		// import all local textures
		error = Imp_AddLocalTextures( texture_dir );
		IMPmError_ReturnOnError(error);

		BFrFileRef_Dispose(texture_dir);
	}

	// get the active sound
	error = GRrGroup_GetString(inGroup, "activeSound", &soundName);
	if (error != UUcError_None) {
		soundName = "trigger_active";
	}
	UUrString_Copy(trigger->active_soundname, soundName, SScMaxNameLength);

	// get the trigger sound
	error = GRrGroup_GetString(inGroup, "triggerSound", &soundName);
	if (error != UUcError_None) {
		soundName = "trigger_hit";
	}
	UUrString_Copy(trigger->trigger_soundname, soundName, SScMaxNameLength);

	trigger->active_sound = NULL;
	trigger->trigger_sound = NULL;

	return UUcError_None;	
}

