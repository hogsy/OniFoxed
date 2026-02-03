// ======================================================================
// Imp_Turret.c
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
#include "Imp_Turret.h"
//#include "Imp_Trigger.h"

#include "Imp_Texture.h"
#include "Imp_Weapon.h"

#include "Imp_Common.h"
#include "Imp_Character.h"

#include "EnvFileFormat.h"

#include "Oni_Object_Private.h"

// ======================================================================
// functions
// ======================================================================

static void
IMPiTurret_GetLightData(
	MXtNode					*inNode,
	UUtUns32				inIndex,
	OBJtLSData				**ioLSData)
{
	UUtError				error;
	GRtGroup_Context		*groupContext;
	GRtGroup				*group;
	GRtElementType			groupType;
	GRtGroup				*ls_group;

	IMPtLS_LightType		light_type;
	IMPtLS_Distribution		light_distribution;
	OBJtLSData				*ls_data = NULL;

	// init
	groupContext = NULL;

	// make sure there is user data to process
	if ((inNode->userDataCount == 0) || (inNode->userData[0] == '\0'))
	{
		goto cleanup;
	}

	// create a GRtGroup from the user data
	error =
		GRrGroup_Context_NewFromString(
			inNode->userData,
			gPreferencesGroup,
			NULL,
			&groupContext,
			&group);
	if (error != UUcError_None) { goto cleanup; }

	// get the ls group from the user data group
	error =
		GRrGroup_GetElement(
			group,
			"ls",
			&groupType,
			&ls_group);
	if (error != UUcError_None) { goto cleanup; }

	// create the OBJtLSData instance
	error =
		TMrConstruction_Instance_Renew(
			OBJcTemplate_LSData,
			inNode->name,
			0,
			&ls_data);
	if (error != UUcError_None) { goto cleanup; }

	// init
	ls_data->index = inIndex;
	ls_data->light_flags = OBJcLightFlag_HasLight;

	// process the user data
	error =
		IMPrEnv_GetLSData(
			ls_group,
			ls_data->filter_color,
			&light_type,
			&light_distribution,
			&ls_data->light_intensity,
			&ls_data->beam_angle,
			&ls_data->field_angle);
	if (error != UUcError_None) { goto cleanup; }

	// set the light type flags
	if (light_type == IMPcLS_LightType_Area)
	{
		ls_data->light_flags |= OBJcLightFlag_Type_Area;
	}
	else if (light_type == IMPcLS_LightType_Linear)
	{
		ls_data->light_flags |= OBJcLightFlag_Type_Linear;
	}
	else if (light_type == IMPcLS_LightType_Point)
	{
		ls_data->light_flags |= OBJcLightFlag_Type_Point;
	}

	// set the light distribution flags
	if (light_distribution == IMPcLS_Distribution_Diffuse)
	{
		ls_data->light_flags |= OBJcLightFlag_Dist_Diffuse;
	}
	else if (light_distribution == IMPcLS_Distribution_Spot)
	{
		ls_data->light_flags |= OBJcLightFlag_Dist_Spot;
	}

cleanup:
	*ioLSData = ls_data;
	if (groupContext)
	{
		GRrGroup_Context_Delete(groupContext);
	}
}

UUtError Imp_Turret_ReadCombatBehavior(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName,
	OBJtTurretClass		*inTurret);

// ----------------------------------------------------------------------

UUtError
Imp_AddTurret(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	char				*modelPath;
	char				*textureName;
	char				*soundName;
	const M3tPoint3D	cPosYPoint = { 0.f, 1.f, 0.f };
	const M3tPoint3D	cPosZPoint = { 0.f, 0.f, 1.f };
	const M3tPoint3D	cZeroPoint = { 0.f, 0.f, 0.f };
	BFtFileRef			*modelFileRef;
	MXtHeader			*header;
	MXtMarker			*muzzle = NULL;
	MXtMarker			*ejection = NULL;
	MXtMarker			*clip = NULL;
	MXtMarker			*sight = NULL;
	UUtBool				twoHanded = UUcFalse;
	MXtMarker			*marker;
	UUtError			error;
	M3tVector3D			marker_pos, marker_z, attach_x, attach_y, attach_z;
	float				tempfloat, axial_dot;
	OBJtTurretClass		*turret;

	// build the turret instance
	error = TMrConstruction_Instance_Renew( OBJcTemplate_TurretClass, inInstanceName, 0, &turret);
	IMPmError_ReturnOnError(error);

	// base Geometry
	{
		error = GRrGroup_GetString(inGroup, "base", &modelPath);
		IMPmError_ReturnOnError(error);

		error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, modelPath, &modelFileRef);
		IMPmError_ReturnOnError(error);

		error = Imp_ParseEnvFile(modelFileRef, &header);
		IMPmError_ReturnOnError(error);

		BFrFileRef_Dispose(modelFileRef);

		if (header->numNodes > 1)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Turret base model had > 1 node!");
		}

		error = TMrConstruction_Instance_NewUnique( M3cTemplate_Geometry, 0, &(turret->base_geometry));
		IMPmError_ReturnOnError(error);

		// get the GQ flags
		turret->base_gq_flags = IMPrEnv_GetNodeFlags( header->nodes );

		error = Imp_Node_To_Geometry_ApplyMatrix(header->nodes, turret->base_geometry);
		UUmError_ReturnOnErrorMsg(error, "failed to build the geometry");

		// get the light data
		IMPiTurret_GetLightData( &header->nodes[0], 0, &turret->base_ls_data );

		// get the texture name
		textureName = header->nodes[0].materials[0].maps[MXcMapping_DI].name;

		// grab a placeholder to the texture
		turret->base_geometry->baseMap = M3rTextureMap_GetPlaceholder_StripExtension_UpperCase(textureName);
		UUmAssert(turret->base_geometry->baseMap);

		// read turret attachment point
		{
			marker = Imp_EnvFile_GetMarker( header->nodes, "turret" );
			turret->turret_position = MUrMatrix_GetTranslation( &marker->matrix);
		}

		// delete env file
		Imp_EnvFile_Delete(header);
	}

	// turret Geometry
	{
		error = GRrGroup_GetString(inGroup, "turret", &modelPath);
		IMPmError_ReturnOnError(error);

		error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, modelPath, &modelFileRef);
		IMPmError_ReturnOnError(error);

		error = Imp_ParseEnvFile(modelFileRef, &header);
		IMPmError_ReturnOnError(error);

		BFrFileRef_Dispose(modelFileRef);

		if (header->numNodes > 1)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Turret turret model file had > 1 node!");
		}

		error = TMrConstruction_Instance_NewUnique( M3cTemplate_Geometry, 0, &(turret->turret_geometry));
		IMPmError_ReturnOnError(error);

		// get the GQ flags
		turret->turret_gq_flags = IMPrEnv_GetNodeFlags( header->nodes );

		error = Imp_Node_To_Geometry_ApplyMatrix(header->nodes, turret->turret_geometry);
		UUmError_ReturnOnErrorMsg(error, "failed to build the geometry");

		// get the texture name
		textureName = header->nodes[0].materials[0].maps[MXcMapping_DI].name;

		// grab a placeholder to the texture
		turret->turret_geometry->baseMap = M3rTextureMap_GetPlaceholder_StripExtension_UpperCase(textureName);
		UUmAssert(turret->turret_geometry->baseMap);


		// grab the texture
		//error = Imp_AddLocalTexture( inSourceFileRef, textureName, &turret->turret_geometry->baseMap );
		//IMPmError_ReturnOnErrorMsg(error, "Could not create texture place holder");

		// read barrel attachment point
		{
			marker = Imp_EnvFile_GetMarker( header->nodes, "gun" );
			turret->barrel_position = MUrMatrix_GetTranslation( &marker->matrix );
		}

		// delete env file
		Imp_EnvFile_Delete(header);
	}

	// barrel Geometry
	{
		error = GRrGroup_GetString(inGroup, "gun", &modelPath);
		IMPmError_ReturnOnError(error);

		error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, modelPath, &modelFileRef);
		IMPmError_ReturnOnError(error);

		error = Imp_ParseEnvFile(modelFileRef, &header);
		IMPmError_ReturnOnError(error);

		BFrFileRef_Dispose(modelFileRef);

		if (header->numNodes > 1)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Turret barrel model file had > 1 node!");
		}

		error = TMrConstruction_Instance_NewUnique( M3cTemplate_Geometry, 0, &(turret->barrel_geometry));
		IMPmError_ReturnOnError(error);

		// get the GQ flags
		turret->barrel_gq_flags = IMPrEnv_GetNodeFlags( header->nodes );

		error = Imp_Node_To_Geometry_ApplyMatrix(header->nodes, turret->barrel_geometry);
		UUmError_ReturnOnErrorMsg(error, "failed to build the geometry");

		// get the texture name
		textureName = header->nodes[0].materials[0].maps[MXcMapping_DI].name;

		// grab a placeholder to the texture
		turret->barrel_geometry->baseMap = M3rTextureMap_GetPlaceholder_StripExtension_UpperCase(textureName);
		UUmAssert(turret->barrel_geometry->baseMap);

		// read particle attachment points
		{
			GRtElementArray*		attachmentArray;
			UUtBool					found_shooter;
			GRtGroup*				attachmentGroup;
			UUtUns16				itr;
			OBJtTurretParticleAttachment	*attachment;
			GRtElementType			elementType;
			UUtInt32				tempint;
			char *					tempstring;
			char					errmsg[128];
			//static M3tVector3D		weapon_orient_dir = {1, 0, 0};			// WEAPON
			static M3tVector3D		weapon_orient_dir = {0, 0, 1};		// TURRET
			M3tQuaternion			ai_shooter_inverse_quat;

			// set up defaults
			turret->attachment_count = 0;
			turret->shooter_count = 0;
			turret->max_ammo = 0;

			error = GRrGroup_GetElement(inGroup, "attachments", &elementType, (void *) &attachmentArray);
			if(error == UUcError_None)
			{
				if(elementType != GRcElementType_Array)
				{
					UUmError_ReturnOnErrorMsg(UUcError_Generic, "attachments must be an array");
				}

				turret->attachment_count = (UUtUns16) GRrGroup_Array_GetLength(attachmentArray);
				found_shooter = UUcFalse;

				for(itr = 0, attachment = turret->attachment; itr < turret->attachment_count; itr++, attachment++)
				{
					error = GRrGroup_Array_GetElement(attachmentArray, itr, &elementType, &attachmentGroup);
					if (error != UUcError_None)
					{
						sprintf(errmsg, "could not get attachment #%d from group", itr);
						UUmError_ReturnOnErrorMsg(error, errmsg);
					}

					if(elementType != GRcElementType_Group)
					{
						UUmError_ReturnOnErrorMsg(UUcError_Generic, "every element of 'attachments' array must be a group");
					}

					// find the marker on the weapon that this is attached to
					error = GRrGroup_GetString(attachmentGroup, "marker", &tempstring);
					UUmError_ReturnOnErrorMsg(error, "every attachment entry must have a 'marker'");

					marker = Imp_EnvFile_GetMarker(header->nodes + 0, tempstring);
					if (marker)
					{
						// it's important that the attachment matrix should be a rigid body transformation matrix
						// i.e. no scale or shear. so we do this orthonormal basis construction thang.
						MUrMatrix_GetCol(&marker->matrix, 1, &attach_y);
						MUmVector_Normalize(attach_y);

						MUrMatrix_GetCol(&marker->matrix, 2, &marker_z);
						MUrVector_CrossProduct(&attach_y, &marker_z, &attach_x);
						MUmVector_Normalize(attach_x);

						MUrVector_CrossProduct(&attach_x, &attach_y, &attach_z);
						MUmVector_Normalize(attach_z);

						MUrMatrix3x3_FormOrthonormalBasis(&attach_x, &attach_y, &attach_z,
														(M3tMatrix3x3 *) &attachment->matrix);

						marker_pos = MUrMatrix_GetTranslation(&marker->matrix);
						MUrMatrix_SetTranslation(&attachment->matrix, &marker_pos);
					}
					else
					{
						sprintf(errmsg, "couldn't find marker %s on the weapon geometry", tempstring);
						UUmError_ReturnOnErrorMsg(UUcError_Generic, errmsg);
					}

					// find the particle class
					error = GRrGroup_GetString(attachmentGroup, "particle_class", &tempstring);
					UUmError_ReturnOnErrorMsg(error, "every attachment entry must have a 'particle_class'");

					if ((strlen(tempstring) < 1) || (strlen(tempstring) > P3cParticleClassNameLength))
					{
						sprintf(errmsg, "particle class name '%s' is not within range 1-%d chars", tempstring, P3cParticleClassNameLength);
						UUmError_ReturnOnErrorMsg(UUcError_Generic, errmsg);
					}
					strcpy(attachment->particle_class_name, tempstring);
					attachment->particle_class = NULL;	// will be set up at runtime

					// find the index of this attachment
					error = GRrGroup_GetInt32(attachmentGroup, "index", &tempint);
					if (error == UUcError_None)
					{
						attachment->shooter_index = (UUtUns16) tempint;
					}
					else
					{
						attachment->shooter_index = (UUtUns16) -1;
					}

					// get the shooter properties (if they're there...)
					error = GRrGroup_GetFloat(attachmentGroup, "rof", &tempfloat);
					if (error == UUcError_None)
					{
						attachment->chamber_time = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(UUcFramesPerSecond / tempfloat);
						// this attachment is a shooter. it must have an index.
						if (attachment->shooter_index == (UUtUns16) -1)
						{
							attachment->shooter_index = turret->shooter_count;
						}

						turret->shooter_count++;
						if (!found_shooter)
						{
							// this is the first shooter attached to this weapon.
							//
							// get the Y axis of the attachment and place in shooterdir_gunspace
							MUrMatrix_GetCol(&attachment->matrix, 1, &turret->ai_params.shooterdir_gunspace);

							// get the translational component of this matrix and remove the component of this offset
							// that lies parallel to the Y axis of the attachment. this gives us the perpendicular offset
							// which is all that we need to consider when aiming the axis at something. we have to remove
							// the parallel offset because otherwise we have real problems shooting something that is
							// closer than the end of the barrel (which can happen due to us putting our weapon into
							// someone's chest)
							turret->ai_params.shooter_perpoffset_gunspace = MUrMatrix_GetTranslation(&attachment->matrix);
							axial_dot = MUrVector_DotProduct(&turret->ai_params.shooter_perpoffset_gunspace, &turret->ai_params.shooterdir_gunspace);
							MUmVector_ScaleIncrement(turret->ai_params.shooter_perpoffset_gunspace, -axial_dot, turret->ai_params.shooterdir_gunspace);

							// set ai_shooter_inversematrix to be a matrix which takes the shooter's direction
							// and points it along weapon_orient_dir (the vector along the weapon which is pointed
							// at the aiming target).
							MUrQuat_SetFromTwoVectors(&turret->ai_params.shooterdir_gunspace, &weapon_orient_dir, &ai_shooter_inverse_quat);
							MUrQuatToMatrix(&ai_shooter_inverse_quat, &turret->ai_params.shooter_inversematrix);

							found_shooter = UUcTrue;
						}
					}
				}
				if (!found_shooter)
				{
					MUrMatrix_Identity(&turret->ai_params.shooter_inversematrix);
					MUmVector_Set(turret->ai_params.shooterdir_gunspace, 1, 0, 0);
					MUmVector_Set(turret->ai_params.shooter_perpoffset_gunspace, 0, 0, 0);
					Imp_PrintWarning("Imp_AddTurret: turret '%s' has no shooters!", inInstanceName);
				}
			}
			// delete env file
			Imp_EnvFile_Delete(header);
		}
	}

	// flags
	turret->flags = 0;

	// grab default params

	error = GRrGroup_GetFloat(inGroup, "max_vert_angle",	 &tempfloat);
	if( error == UUcError_None )
		turret->max_vert_angle	= tempfloat * M3cDegToRad;
	else
		turret->max_vert_angle	= M3c2Pi;

	error = GRrGroup_GetFloat(inGroup, "min_vert_angle",	 &tempfloat);
	if( error == UUcError_None )
		turret->min_vert_angle	= tempfloat * M3cDegToRad;
	else
		turret->min_vert_angle	= 0;

	error = GRrGroup_GetFloat(inGroup, "max_vert_speed",	 &tempfloat);
	if( error == UUcError_None )
		turret->max_vert_speed	= tempfloat * UUcDegPerSecToRadPerFrame;
	else
		turret->max_vert_speed	= 45.0f * UUcDegPerSecToRadPerFrame;

	error = GRrGroup_GetFloat(inGroup, "max_horiz_angle",	 &tempfloat);
	if( error == UUcError_None )
		turret->max_horiz_angle	= tempfloat * M3cDegToRad;
	else
		turret->max_horiz_angle	= M3c2Pi;

	error = GRrGroup_GetFloat(inGroup, "min_horiz_angle",	 &tempfloat);
	if( error == UUcError_None )
		turret->min_horiz_angle	= tempfloat * M3cDegToRad;
	else
		turret->min_horiz_angle	= 0;

	error = GRrGroup_GetFloat(inGroup, "max_horiz_speed",	 &tempfloat);
	if( error == UUcError_None )
		turret->max_horiz_speed	= tempfloat * UUcDegPerSecToRadPerFrame;
	else
		turret->max_horiz_speed	= 45.0f * UUcDegPerSecToRadPerFrame;


	error = GRrGroup_GetUns32(inGroup, "timeout", &turret->timeout);
	if (error != UUcError_None) {
		turret->timeout = 900;
	}

	// free time
	error = GRrGroup_GetUns16(inGroup, "freeTime", &(turret->freeTime));
	if (error != UUcError_None) turret->freeTime = 0;

	error = Imp_Turret_ReadCombatBehavior(
		inSourceFileRef,
		inSourceFileModDate,
		inGroup,
		inInstanceName,
		turret);
	if (error != UUcError_None)
		IMPmError_ReturnOnErrorMsg(error, "error reading turret AI params");

	// name
	UUrString_Copy(turret->name,inInstanceName,WPcMaxWeaponName);

	// import all textures in this directory...
	{
		BFtFileRef				*texture_dir;

		// create the texture directory file ref
		error = BFrFileRef_GetParentDirectory( inSourceFileRef, &texture_dir);
		//error =  BFrFileRef_DuplicateAndReplaceName( inSourceFileRef, ".", &texture_dir);
		IMPmError_ReturnOnError(error);

		// import all local textures
		error = Imp_AddLocalTextures( texture_dir );
		IMPmError_ReturnOnError(error);

		BFrFileRef_Dispose(texture_dir);
	}

	// get the active sound
	error = GRrGroup_GetString(inGroup, "activeSound", &soundName);
	if (error != UUcError_None) {
		soundName = "turret_active";
	}
	UUrString_Copy(turret->active_soundname, soundName, SScMaxNameLength);
	turret->active_sound = NULL;

	return UUcError_None;
}



UUtError Imp_Turret_ReadCombatBehavior(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName,
	OBJtTurretClass			*inTurret)
{
	UUtError				error;
	AI2tShootingSkill		*skill;

	error = Imp_Weapon_ReadAIParameters(
		inSourceFile,
		inSourceFileModDate,
		inGroup,
		inInstanceName,
		UUcFalse,
		&inTurret->ai_params );
	if (error != UUcError_None)
		IMPmError_ReturnOnErrorMsg(error, "error reading turret AI params");


	/*
	 * combat parameters
	 */

	  /*
	   * targeting parameters
	   */

	    /*
		 * initial deliberately-missed shots
		 */

#if TARGETING_MISS_DECAY
	error = GRrGroup_GetFloat(inGroup, "startle_miss_angle_min", &inTurret->targeting_params.startle_miss_angle_min);
	if (error != UUcError_None)
		inTurret->targeting_params.startle_miss_angle_min = 4.5f;
	inTurret->targeting_params.startle_miss_angle_min *= M3cDegToRad;

	error = GRrGroup_GetFloat(inGroup, "startle_miss_angle_max", &inTurret->targeting_params.startle_miss_angle_max);
	if (error != UUcError_None)
		inTurret->targeting_params.startle_miss_angle_max = 30.0f;
	inTurret->targeting_params.startle_miss_angle_max *= M3cDegToRad;

	error = GRrGroup_GetFloat(inGroup, "startle_miss_decay", &inTurret->targeting_params.startle_miss_decay);
	if (error != UUcError_None)
		inTurret->targeting_params.startle_miss_decay = 0.5f;
#else
	error = GRrGroup_GetFloat(inGroup, "startle_miss_angle", &inTurret->targeting_params.startle_miss_angle);
	if (error != UUcError_None)
		inTurret->targeting_params.startle_miss_angle = 30.0f;

	inTurret->targeting_params.startle_miss_angle = UUmPin(inTurret->targeting_params.startle_miss_angle, 0.f, 80.0f);
	inTurret->targeting_params.startle_miss_angle = MUrSin(inTurret->targeting_params.startle_miss_angle * M3cDegToRad);

	error = GRrGroup_GetFloat(inGroup, "startle_miss_distance", &inTurret->targeting_params.startle_miss_distance);
	if (error != UUcError_None)
		inTurret->targeting_params.startle_miss_distance = 120.0f;
#endif
	    /*
		 * enemy movement prediction
		 */

	error = GRrGroup_GetFloat(inGroup, "predict_amount", &inTurret->targeting_params.predict_amount);
	if (error != UUcError_None)
		inTurret->targeting_params.predict_amount = 1.0f;

	error = GRrGroup_GetUns32(inGroup, "predict_delayframes", &inTurret->targeting_params.predict_delayframes);
	if (error != UUcError_None)
		inTurret->targeting_params.predict_delayframes = 5;

	error = GRrGroup_GetUns32(inGroup, "predict_velocityframes",  &inTurret->targeting_params.predict_velocityframes);
	if (error != UUcError_None)
		inTurret->targeting_params.predict_velocityframes = 15;

	error = GRrGroup_GetUns32(inGroup, "predict_trendframes",  &inTurret->targeting_params.predict_trendframes);
	if (error != UUcError_None)
		inTurret->targeting_params.predict_trendframes = 60;


	error = GRrGroup_GetUns32(inGroup, "predict_positiondelayframes", &inTurret->targeting_params.predict_positiondelayframes);
	if (error != UUcError_None)	{
		inTurret->targeting_params.predict_positiondelayframes = 0;
	}


	  /*
	   * shooting skill
	   */

	// each weapon has an index assigned to it which is read from the weapon ins file... this is
	// used to index an entry in the shooting skill table

	skill = &inTurret->shooting_skill;

	error = GRrGroup_GetFloat(inGroup, "skill_recoil_compensation", &skill->recoil_compensation);
	if (error != UUcError_None)
		skill->recoil_compensation = 0.3f;

	// best_aiming_angle is read as degrees, stored as sin(theta)
	error = GRrGroup_GetFloat(inGroup, "skill_best_aiming_angle", &skill->best_aiming_angle);
	if (error != UUcError_None)
		skill->best_aiming_angle = 0.5f;
	skill->best_aiming_angle = MUrSin(skill->best_aiming_angle * M3cDegToRad);

	error = GRrGroup_GetFloat(inGroup, "skill_shot_error", &skill->shot_error);
	if (error != UUcError_None)
		skill->shot_error = 0.0f;

	error = GRrGroup_GetFloat(inGroup, "skill_shot_decay", &skill->shot_decay);
	if (error != UUcError_None)
		skill->shot_decay = 0.5f;

	error = GRrGroup_GetFloat(inGroup, "skill_inaccuracy", &skill->gun_inaccuracy);
	if (error != UUcError_None)
		skill->gun_inaccuracy = 1.0f;

	error = GRrGroup_GetUns16(inGroup, "skill_delay_min", &skill->delay_min);
	if (error != UUcError_None)
		skill->delay_min = 0;

	error = GRrGroup_GetUns16(inGroup, "skill_delay_max", &skill->delay_max);
	if (error != UUcError_None)
		skill->delay_max = 0;

	return UUcError_None;
}

