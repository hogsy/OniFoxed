/*
	FILE:	Imp_Weapon.c
	
	AUTHOR:	Michael Evans
	
	CREATED: 3/18/1998
	
	PURPOSE: 
	
	Copyright 1998 Bungie Software

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_AppUtilities.h"

#include "Imp_Weapon.h"
#include "Imp_Model.h"
#include "Imp_Common.h"
#include "Imp_ParseEnvFile.h"
#include "Imp_Character.h"

#include "Oni_Character.h"
#include "ONI_Weapon.h"

#define IMPcBarrelLineLen 6


AUtFlagElement	IMPgWeaponClassFlags[] = 
{
	{
		"big",
		WPcWeaponClassFlag_Big
	},
	{
		"unstorable",
		WPcWeaponClassFlag_Unstorable
	},
	{
		"energy",
		WPcWeaponClassFlag_Energy
	},
	{
		"autofire",
		WPcWeaponClassFlag_Autofire
	},
	{
		"ai_stun",
		WPcWeaponClassFlag_AIStun
	},
	{
		"ai_knockdown",
		WPcWeaponClassFlag_AIKnockdown
	},
	{
		"ai_singleshot",
		WPcWeaponClassFlag_AISingleShot
	},
	{
		"no_reload",
		WPcWeaponClassFlag_Unreloadable
	},
	{
		"walk_only",
		WPcWeaponClassFlag_WalkOnly
	},
	{
		"restart_each_shot",
		WPcWeaponClassFlag_RestartEachShot
	},
	{
		"dont_stop_while_firing",
		WPcWeaponClassFlag_DontStopWhileFiring
	},
	{
		"laser",
		WPcWeaponClassFlag_HasLaser
	},
	{
		"no_cursor_scale", 
		WPcWeaponClassFlag_NoScale
	},
	{
		"half_cursor_scale", 
		WPcWeaponClassFlag_HalfScale
	},
	{
		"dont-clip-sight",
		WPcWeaponClassFlag_DontClipSight
	},
	{
		"dont_clip_sight",
		WPcWeaponClassFlag_DontClipSight
	},
	{
		"dontclipsight",
		WPcWeaponClassFlag_DontClipSight
	},
	{
		"dontfadeout",
		WPcWeaponClassFlag_DontFadeOut
	},
	{
		"dont_fade_out",
		WPcWeaponClassFlag_DontFadeOut
	},
	{
		"dont-fade-out",
		WPcWeaponClassFlag_DontFadeOut
	},
	{
		"use-ammo-continuously",
		WPcWeaponClassFlag_UseAmmoContinuously
	},
	{
		NULL,
		0
	}
};

AUtFlagElement	IMPgWeaponTargetingFlags[] = 
{
	{
		"no-wild-shots",
		WPcWeaponTargetingFlag_NoWildShots
	},
	{
		NULL,
		0
	}
};

// import a weapon sprite block

static void ImportWeaponSprite(GRtGroup *inGroup, const char *inPrefix, WPtSprite *ioSprite)
{
	char variable_name[256];

	M3tTextureMap	*texture;
	char			*shade_string;
	float			scale;
	UUtError		error;

	sprintf(variable_name, "%stexture", inPrefix);
	texture = M3rGroup_GetTextureMap(inGroup, variable_name);
	if (NULL != texture) {
		ioSprite->texture = texture;
	}

	sprintf(variable_name, "%scolor", inPrefix);
	error = GRrGroup_GetString(inGroup, variable_name, &shade_string);
	if (UUcError_None == error) {
		sscanf( shade_string, "%x", &ioSprite->shade);
	}

	sprintf(variable_name, "%sscale", inPrefix);
	error = GRrGroup_GetFloat(inGroup, variable_name, &scale);
	if (UUcError_None == error) {
		ioSprite->scale = scale;
	}

	return;
}

UUtError
Imp_AddWeapon(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError		error;
	WPtWeaponClass *weapon;
	UUtUns32		store;
	char			*modelPath;
	char			*icon_name;
	char			*hud_empty_name;
	char			*hud_fill_name;
	char			*textureName;
	char			*soundName;
	GRtElementArray *groupFlags;
	GRtElementType	groupType;
	const M3tPoint3D cPosYPoint = { 0.f, 1.f, 0.f };
	const M3tPoint3D cPosZPoint = { 0.f, 0.f, 1.f };
	const M3tPoint3D cZeroPoint = { 0.f, 0.f, 0.f };
	M3tPoint3D		from,to;
	BFtFileRef		*modelFileRef;
	MXtHeader		*header;
	M3tMatrix4x3		temp;
	MXtMarker		*muzzle = NULL;
	MXtMarker		*ejection = NULL;
	MXtMarker		*clip = NULL;
	MXtMarker		*sight = NULL;
	MXtMarker		*second_hand;
	UUtBool			twoHanded = UUcFalse;
	UUtInt32		tempint;
	float			tempfloat;
	
	if (TMrConstruction_Instance_CheckExists(WPcTemplate_WeaponClass, inInstanceName))	{
		return UUcError_None;
	}

	Imp_PrintMessage(IMPcMsg_Cosmetic, "\tbuilding..."UUmNL);

	// Create our instance
	error = TMrConstruction_Instance_Renew(
			WPcTemplate_WeaponClass,
			inInstanceName,
			0,				// variable length array whoha
			&weapon);
	IMPmError_ReturnOnError(error);
	UUrMemory_Clear(weapon,sizeof(WPtWeaponClass));

	// Gun Geometry
	error = GRrGroup_GetString(inGroup, "model", &modelPath);
	IMPmError_ReturnOnError(error);

	error = BFrFileRef_DuplicateAndReplaceName(inSourceFile, modelPath, &modelFileRef);
	IMPmError_ReturnOnError(error);

	error = Imp_ParseEnvFile(modelFileRef, &header);
	IMPmError_ReturnOnError(error);

	BFrFileRef_Dispose(modelFileRef);

	if (header->numNodes > 1) {
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Model file had > 1 node!");
	}

	error = TMrConstruction_Instance_NewUnique(
		M3cTemplate_Geometry,
		0,
		&(weapon->geometry));
	IMPmError_ReturnOnError(error);

	error = Imp_Node_To_Geometry_ApplyMatrix(header->nodes + 0, weapon->geometry);
	UUmError_ReturnOnErrorMsg(error, "failed to build the geometry");


	// second hand
	second_hand = Imp_EnvFile_GetMarker(header->nodes + 0, "lefthand");
	if (second_hand)
	{
		temp = second_hand->matrix;

		MUrMatrix_MultiplyPoint(&cZeroPoint, &temp, &from);
		MUrMatrix_MultiplyPoint(&cPosZPoint, &temp, &to);
		MUmVector_Subtract(weapon->secondHandVector, to, from);
		MUrNormalize(&weapon->secondHandVector);

		MUrMatrix_MultiplyPoint(&cZeroPoint, &temp, &from);
		MUrMatrix_MultiplyPoint(&cPosYPoint, &temp, &to);
		MUmVector_Subtract(weapon->secondHandUpVector, to, from);
		MUrNormalize(&weapon->secondHandUpVector);

		MUrMatrix_MultiplyPoint(&cZeroPoint, &temp, &from);
		weapon->secondHandPosition = from;

		twoHanded = UUcTrue;
	}

	{
		// CB: read particle attachment points
		GRtElementArray*		attachmentArray;
		GRtGroup*				attachmentGroup;
		UUtUns16				itr;
		UUtUns32				found_shooters;
		WPtParticleAttachment *	attachment;
		GRtElementType			elementType;
		float					axial_dot;
		char *					tempstring;
		char					errmsg[128];
		MXtMarker *				marker;
		M3tVector3D				marker_pos, marker_z, attach_x, attach_y, attach_z;
		static M3tVector3D		weapon_orient_dir = {1, 0, 0};
		M3tQuaternion			ai_shooter_inverse_quat;
		AI2tWeaponParameters	*parameters;
		
		// set up defaults
		weapon->attachment_count = 0;
		weapon->shooter_count = 0;

		error = GRrGroup_GetElement(inGroup, "attachments", &elementType, (void *) &attachmentArray);
		if(error == UUcError_None)
		{
			if(elementType != GRcElementType_Array)
			{
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "attachments must be an array");
			}
			
			weapon->attachment_count = (UUtUns16) GRrGroup_Array_GetLength(attachmentArray);
			found_shooters = 0;
			
			for(itr = 0, attachment = weapon->attachment; itr < weapon->attachment_count; itr++, attachment++)
			{
				error = GRrGroup_Array_GetElement(attachmentArray, itr,
												  &elementType, &attachmentGroup);
				if (error != UUcError_None) {
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
				if (marker) {
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
				} else {
					sprintf(errmsg, "couldn't find marker %s on the weapon geometry", tempstring);
					UUmError_ReturnOnErrorMsg(UUcError_Generic, errmsg);
				}
				

				// find the particle class
				error = GRrGroup_GetString(attachmentGroup, "particle_class", &tempstring);
				UUmError_ReturnOnErrorMsg(error, "every attachment entry must have a 'particle_class'");
				
				if ((strlen(tempstring) < 1) || (strlen(tempstring) > P3cParticleClassNameLength)) {
					sprintf(errmsg, "particle class name '%s' is not within range 1-%d chars", tempstring, P3cParticleClassNameLength);
					UUmError_ReturnOnErrorMsg(UUcError_Generic, errmsg);
				}
				strcpy(attachment->classname, tempstring);
				attachment->classptr = NULL;	// will be set up at runtime


				// find the index of this attachment
				error = GRrGroup_GetInt32(attachmentGroup, "index", &tempint);
				if (error == UUcError_None) {
					attachment->shooter_index = (UUtUns16) tempint;
				} else {
					attachment->shooter_index = (UUtUns16) -1;
				}

				error = GRrGroup_GetUns16(attachmentGroup, "start_delay", &attachment->start_delay);
				if (error != UUcError_None) {
					attachment->start_delay = 0;
				}

				// get the shooter properties (if they're there)
				attachment->ammo_used = attachment->chamber_time = 0;

				error = GRrGroup_GetInt32(attachmentGroup, "ammo_used", &tempint);
				if (error == UUcError_None) {
					attachment->ammo_used = (UUtUns16) tempint;
					if (attachment->ammo_used > 0) {
						// this attachment is a shooter. it must have an index.
						if (attachment->shooter_index == (UUtUns16) -1) {
							attachment->shooter_index = weapon->shooter_count;
						}

						weapon->shooter_count++;

						error = GRrGroup_GetFloat(attachmentGroup, "rof", &tempfloat);
						if (error != UUcError_None) {
							sprintf(errmsg, "attachment %d uses ammo but has no rof", itr);
							UUmError_ReturnOnErrorMsg(error, errmsg);
						}
						attachment->chamber_time = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(UUcFramesPerSecond / tempfloat);

						error = GRrGroup_GetFloat(attachmentGroup, "cheat_rof", &tempfloat);
						if (error != UUcError_None) {
							attachment->cheat_chamber_time = attachment->chamber_time;
						} else {
							attachment->cheat_chamber_time = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(UUcFramesPerSecond / tempfloat);
						}

						error = GRrGroup_GetUns16(attachmentGroup, "minframes", &attachment->minfire_time);
						if (error != UUcError_None) {
							attachment->minfire_time = 0;
						}

						if (found_shooters == 0) {
							parameters = &weapon->ai_parameters;
						} else if (found_shooters == 1) {
							parameters = &weapon->ai_parameters_alt;
						} else {
							parameters = NULL;
						}

						if (parameters != NULL) {
							// get the Y axis of the attachment and place in shooterdir_gunspace
							MUrMatrix_GetCol(&attachment->matrix, 1, &parameters->shooterdir_gunspace);

							// get the translational component of this matrix and remove the component of this offset
							// that lies parallel to the Y axis of the attachment. this gives us the perpendicular offset
							// which is all that we need to consider when aiming the axis at something. we have to remove
							// the parallel offset because otherwise we have real problems shooting something that is
							// closer than the end of the barrel (which can happen due to us putting our weapon into
							// someone's chest)
							parameters->shooter_perpoffset_gunspace = MUrMatrix_GetTranslation(&attachment->matrix);
							axial_dot = MUrVector_DotProduct(&parameters->shooter_perpoffset_gunspace, &parameters->shooterdir_gunspace);
							MUmVector_ScaleIncrement(parameters->shooter_perpoffset_gunspace, -axial_dot, parameters->shooterdir_gunspace);

							// set ai_shooter_inversematrix to be a matrix which takes the shooter's direction
							// and points it along weapon_orient_dir (the vector along the weapon which is pointed
							// at the aiming target).
							MUrQuat_SetFromTwoVectors(&parameters->shooterdir_gunspace, &weapon_orient_dir, &ai_shooter_inverse_quat);
							MUrQuatToMatrix(&ai_shooter_inverse_quat, &parameters->shooter_inversematrix);

							found_shooters++;
						}
					}

				}
			}

			if (found_shooters < 0) 
			{
				MUrMatrix_Identity(&weapon->ai_parameters_alt.shooter_inversematrix);
				MUmVector_Set(weapon->ai_parameters.shooterdir_gunspace, 1, 0, 0);
				MUmVector_Set(weapon->ai_parameters.shooter_perpoffset_gunspace, 0, 0, 0);
				Imp_PrintWarning("Imp_AddWeapon: weapon '%s' has no shooters!", inInstanceName);
			}
		}
	}

	// get the name of the texture
	error = GRrGroup_GetString(inGroup, "baseMap", &textureName);
	IMPmError_ReturnOnErrorMsg(error, "missing baseMap tag");
	if (UUcError_None == error)
	{
		char	mungedTextureName[BFcMaxFileNameLength];
		
		UUrString_Copy(mungedTextureName, textureName, BFcMaxFileNameLength);
		UUrString_StripExtension(mungedTextureName);

		weapon->geometry->baseMap = M3rTextureMap_GetPlaceholder(mungedTextureName);
	}

	// maximum sight distance	
	error = GRrGroup_GetFloat(inGroup, "maximum_sight_distance", &weapon->maximum_sight_distance);
	if (UUcError_None != error) {
		weapon->maximum_sight_distance = 5000.f;
	}

	// sighting.......
	weapon->cursor.texture = NULL;
	weapon->cursor.shade = IMcShade_White;
	weapon->cursor.scale = 1.f;

	ImportWeaponSprite(inGroup, "sight_", &weapon->cursor);

	weapon->cursor_targeted = weapon->cursor;

	ImportWeaponSprite(inGroup, "sight_targeted_", &weapon->cursor_targeted);

	weapon->tunnel.texture = NULL;
	weapon->tunnel.shade = IMcShade_White;
	weapon->tunnel.scale = 1.f;

	ImportWeaponSprite(inGroup, "tunnel_", &weapon->tunnel);

	error = GRrGroup_GetUns32(inGroup, "tunnel_count", &weapon->tunnel_count);
	if (UUcError_None != error) {
		weapon->tunnel_count = 0;
	}

	error = GRrGroup_GetFloat(inGroup, "tunnel_spacing", &weapon->tunnel_spacing);
	if (UUcError_None != error) {
		weapon->tunnel_spacing = 20.f;
	}

	// get the laser color
	{
		char *laser_color;

		error = GRrGroup_GetString(inGroup, "laser_color", &laser_color);

		if (error == UUcError_None ) {
			sscanf(laser_color, "%x", &weapon->laser_color);
		}
		else {
			weapon->laser_color	= 0xffff0000;
		}
	}

	// get the sight node
	sight = Imp_EnvFile_GetMarker(header->nodes, "laser");
	if (sight)
	{
		temp = sight->matrix;
		MUrMatrix_MultiplyPoint(&cZeroPoint, &temp, &from);
		MUrMatrix_MultiplyPoint(&cPosYPoint, &temp, &to);
		MUmVector_Subtract(weapon->sight.vector, to, from);
		MUrNormalize(&weapon->sight.vector);
		MUmVector_Copy(weapon->sight.position, from);
	}

	// flags
	error = GRrGroup_GetElement(inGroup, "flags", &groupType, &groupFlags);
	if (error != UUcError_None) weapon->flags = 0;
	else
	{
		error = AUrFlags_ParseFromGroupArray(
			IMPgWeaponClassFlags,
			groupType,
			groupFlags,
			&store);
		UUmError_ReturnOnError(error);
		weapon->flags = store;
	}

	if (twoHanded) {
		weapon->flags |= WPcWeaponClassFlag_TwoHanded;
	}

	// chamber time stop threshole, for use with flag dont_stop_while_firing
	error = GRrGroup_GetUns16(inGroup, "stopChamberThreshold", &(weapon->stopChamberThreshold));
	if (error != UUcError_None) weapon->stopChamberThreshold = 0;

	// max ammo
	error = GRrGroup_GetUns16(inGroup, "max_ammo", &(weapon->max_ammo));
	IMPmError_ReturnOnErrorMsg(error, "weapon must have a 'max_ammo' string");

	// reload time
	error = GRrGroup_GetUns16(inGroup, "reloadTime", &(weapon->reloadTime));
	IMPmError_ReturnOnErrorMsg(error, "missing reloadTime");

	// reload delay time - time after firing before we can fire again
	error = GRrGroup_GetUns16(inGroup, "reloadDelayTime", &(weapon->reload_delay_time));
	if (error != UUcError_None) weapon->reload_delay_time = 0;

	// free time
	error = GRrGroup_GetUns16(inGroup, "freeTime", &(weapon->freeTime));
	if (error != UUcError_None) weapon->freeTime = 0;
	
	// name
	UUrString_Copy(weapon->name,inInstanceName,WPcMaxWeaponName);

	// delete the file in memory
	Imp_EnvFile_Delete(header);

	error = Imp_GetAnimType(inGroup, "recoilAnimType", &(weapon->recoilAnimType));
	IMPmError_ReturnOnErrorMsg(error, "missing recoilAnimType");

	error = Imp_GetAnimType(inGroup, "reloadAnimType", &(weapon->reloadAnimType));
	IMPmError_ReturnOnErrorMsg(error, "missing reloadlAnimType");

	error = GRrGroup_GetFloat(inGroup, "aimingSpeed", &(weapon->aimingSpeed));
	IMPmError_ReturnOnErrorMsg(error, "missing aimingSpeed");

	error = GRrGroup_GetFloat(inGroup, "recoil", &(weapon->private_recoil_info.base));
	IMPmError_ReturnOnErrorMsg(error, "missing recoil");
	weapon->private_recoil_info.base *= (M3c2Pi / 360.f);	// turn it into radians

	error = GRrGroup_GetFloat(inGroup, "recoilMax", &(weapon->private_recoil_info.max));
	IMPmError_ReturnOnErrorMsg(error, "missing recoilMax");
	weapon->private_recoil_info.max *= (M3c2Pi / 360.f);	// turn it into radians

	error = GRrGroup_GetFloat(inGroup, "recoilFactor", &(weapon->private_recoil_info.factor));
	IMPmError_ReturnOnErrorMsg(error, "missing recoilFactor");
	weapon->private_recoil_info.factor = UUmPin(weapon->private_recoil_info.factor, 1.05f, 20);

	error = GRrGroup_GetFloat(inGroup, "recoilReturnSpeed", &(weapon->private_recoil_info.returnSpeed));
	IMPmError_ReturnOnErrorMsg(error, "missing recoilReturnSpeed");
	weapon->private_recoil_info.returnSpeed *= (M3c2Pi / 360.f);	// turn it into radians

	error = GRrGroup_GetFloat(inGroup, "recoilFiringReturnSpeed", &(weapon->private_recoil_info.firingReturnSpeed));
	if (error != UUcError_None) {
		weapon->private_recoil_info.firingReturnSpeed = weapon->private_recoil_info.returnSpeed;
	} else {
		weapon->private_recoil_info.firingReturnSpeed *= (M3c2Pi / 360.f);	// turn it into radians
	}

	weapon->private_recoil_info.base /= (1 + 1 / (weapon->private_recoil_info.factor - 1));

	{
		UUtBool recoil_direct = UUcFalse;

		error = GRrGroup_GetBool(inGroup, "recoilDirect", &recoil_direct);
		if ((error == UUcError_None) && (recoil_direct)) {
			weapon->flags |= WPcWeaponClassFlag_RecoilDirect;
		}
	}
	
	// okay if icon is NULL
	error = GRrGroup_GetString(inGroup, "icon", &icon_name);
	if (error == UUcError_None)
	{
		weapon->icon = M3rTextureMap_GetPlaceholder(icon_name);
	}
	else
	{
		weapon->icon = NULL;
	}

	// okay if hud_empty is NULL
	error = GRrGroup_GetString(inGroup, "HUD_empty", &hud_empty_name);
	if (error == UUcError_None)
	{
		// create a placeholder
		TMrConstruction_Instance_GetPlaceHolder(
			M3cTemplate_TextureMap,
			hud_empty_name,
			(TMtPlaceHolder*)&weapon->hud_empty);
	}
	else
	{
		weapon->hud_empty = NULL;
	}

	// okay if hud_fill is NULL
	error = GRrGroup_GetString(inGroup, "HUD_fill", &hud_fill_name);
	if (error == UUcError_None)
	{
		// create a placeholder
		TMrConstruction_Instance_GetPlaceHolder(
			M3cTemplate_TextureMap,
			hud_fill_name,
			(TMtPlaceHolder*)&weapon->hud_fill);
	}
	else
	{
		weapon->hud_fill = NULL;
	}

	// read the glow texture
	error = GRrGroup_GetString(inGroup, "glow_texture", &textureName);
	if (error == UUcError_None)
	{
		// create a placeholder
		TMrConstruction_Instance_GetPlaceHolder(
			M3cTemplate_TextureMap,
			textureName,
			(TMtPlaceHolder*)&weapon->glow_texture);
	}
	else
	{
		weapon->glow_texture = NULL;
	}

	// read the loaded glow texture
	error = GRrGroup_GetString(inGroup, "loaded_glow_texture", &textureName);
	if (error == UUcError_None)
	{
		// create a placeholder
		TMrConstruction_Instance_GetPlaceHolder(
			M3cTemplate_TextureMap,
			textureName,
			(TMtPlaceHolder*)&weapon->loaded_glow_texture);
	}
	else
	{
		weapon->loaded_glow_texture = NULL;
	}

	// read the glow texture scales
	error = GRrGroup_GetFloat(inGroup, "glow_xscale", &weapon->glow_scale[0]);
	if (error != UUcError_None) {
		weapon->glow_scale[0] = 1.0f;
	}
	error = GRrGroup_GetFloat(inGroup, "glow_yscale", &weapon->glow_scale[1]);
	if (error != UUcError_None) {
		weapon->glow_scale[1] = 1.0f;
	}

	// read the empty sound name
	error = GRrGroup_GetString(inGroup, "empty_sound", &soundName);
	if (error != UUcError_None) {
		soundName = "";
	}
	UUrString_Copy(weapon->empty_soundname, soundName, SScMaxNameLength);
	weapon->empty_sound = NULL;			// set up at runtime

	// read the weapon's AI parameters - this is split off into a separate function because it is called
	// by turrets too
	error = Imp_Weapon_ReadAIParameters(inSourceFile, inSourceFileModDate, inGroup, inInstanceName, UUcFalse, &weapon->ai_parameters);
	UUmError_ReturnOnError(error);

	if (weapon->shooter_count > 1) {
		// read the AI parameters for the second shooter on this weapon
		error = Imp_Weapon_ReadAIParameters(inSourceFile, inSourceFileModDate, inGroup, inInstanceName, UUcTrue, &weapon->ai_parameters_alt);
		UUmError_ReturnOnError(error);

		weapon->flags |= WPcWeaponClassFlag_HasAlternate;
	} else {
		UUrMemory_Clear(&weapon->ai_parameters_alt, sizeof(weapon->ai_parameters_alt));
	}

	// calculate the weapon's pickup position
	MUmVector_Add(weapon->pickup_offset, weapon->geometry->pointArray->minmax_boundingBox.minPoint,
										 weapon->geometry->pointArray->minmax_boundingBox.maxPoint);
	MUmVector_Scale(weapon->pickup_offset, 0.5f);

	// read the weapon's Y offset used for display
	error = GRrGroup_GetFloat(inGroup, "display_yoffset", &weapon->display_yoffset);
	if (error != UUcError_None) {
		weapon->display_yoffset = 0.0f;
	}

	return UUcError_None;
}

UUtError Imp_Weapon_ReadAIParameters(
	BFtFileRef*				inSourceFile,
	UUtUns32				inSourceFileModDate,
	GRtGroup*				inGroup,
	char*					inInstanceName,
	UUtBool					inAlternateParameters,
	AI2tWeaponParameters	*inParameters)
{
	UUtError	error;
	void		*element;
	GRtElementType element_type;

	
	// NB: ai_parameters.shooterdir_gunspace and shooter_inversematrix must be set up separately to this
	// call!


	// get the estimated speed of projectiles for target leading
	error = GRrGroup_GetFloat(inGroup, inAlternateParameters ? "ai_prediction_speed_alt" : "ai_prediction_speed", &(inParameters->prediction_speed));
	if (error != UUcError_None) inParameters->prediction_speed = 1200.0f;	// roughly pistol bullet speed

	// get the hit-estimation radii for AI targeting both normally, and when in the middle
	// of a burst (burst radius is optional)
	error = GRrGroup_GetFloat(inGroup, inAlternateParameters ? "ai_aim_radius_alt" : "ai_aim_radius", &(inParameters->aim_radius));
	if (error != UUcError_None) inParameters->aim_radius = 2.5f;
	error = GRrGroup_GetFloat(inGroup, inAlternateParameters ? "ai_burst_radius_alt" : "ai_burst_radius", &(inParameters->burst_radius));
	if (error != UUcError_None) inParameters->burst_radius = 0;

	// get the max number of shots to deliberately miss with when startled
	error = GRrGroup_GetUns16(inGroup, inAlternateParameters ? "max_startle_misses_alt" : "max_startle_misses", &(inParameters->max_startle_misses));
	if (error != UUcError_None) inParameters->max_startle_misses = 5;

	// get the weapon's index in the AI shooting skill table
	error = GRrGroup_GetUns16(inGroup, inAlternateParameters ? "ai_shootskill_index_alt" : "ai_shootskill_index", &(inParameters->shootskill_index));
	if (error != UUcError_None) {
		// this isn't necessarily a problem as this may be a turret's AI parameters
		inParameters->shootskill_index = 0;
	}

	// get the volume of gunshot events emitted from this weapon... this is the distance at which AI can hear
	// gunfire
	error = GRrGroup_GetFloat(inGroup, inAlternateParameters ? "ai_volume_alt" : "ai_volume", &(inParameters->detection_volume));
	if (error != UUcError_None) inParameters->detection_volume = 800.0f;

	// get the minimum safe distance that AI will fire this gun at
	error = GRrGroup_GetFloat(inGroup, inAlternateParameters ? "ai_min_safe_distance_alt" : "ai_min_safe_distance", &(inParameters->min_safe_distance));
	if (error != UUcError_None) inParameters->min_safe_distance = 0;

	// get the maximum range that AI will fire this gun at
	error = GRrGroup_GetFloat(inGroup, inAlternateParameters ? "ai_max_range_alt" : "ai_max_range", &(inParameters->max_range));
	if (error != UUcError_None) inParameters->max_range = 0;

	// how long will the AI go into melee for, if it stuns or knocks down an enemy using this weapon?
	error = GRrGroup_GetUns32(inGroup, inAlternateParameters ? "ai_melee_timer_alt" : "ai_melee_timer", &(inParameters->melee_assist_timer));
	if (error != UUcError_None) inParameters->melee_assist_timer = 0;

	// ballistic parameters
	error = GRrGroup_GetFloat(inGroup, inAlternateParameters ? "ai_ballistic_speed_alt" : "ai_ballistic_speed", &(inParameters->ballistic_speed));
	if (error != UUcError_None) inParameters->ballistic_speed = 0.0f;

	error = GRrGroup_GetFloat(inGroup, inAlternateParameters ? "ai_ballistic_gravity_alt" : "ai_ballistic_gravity", &(inParameters->ballistic_gravity));
	if (error != UUcError_None) inParameters->ballistic_gravity = 0.0f;

	// get the dodge pattern parameters
	error = GRrGroup_GetFloat(inGroup, inAlternateParameters ? "ai_dodge_length_alt" : "ai_dodge_length", &(inParameters->dodge_pattern_length));
	if (error != UUcError_None) inParameters->dodge_pattern_length = 0.0f;

	error = GRrGroup_GetFloat(inGroup, inAlternateParameters ? "ai_dodge_baserad_alt" : "ai_dodge_baserad", &(inParameters->dodge_pattern_base));
	if (error != UUcError_None) inParameters->dodge_pattern_base = 0.0f;

	error = GRrGroup_GetFloat(inGroup, inAlternateParameters ? "ai_dodge_angle_alt" : "ai_dodge_angle", &(inParameters->dodge_pattern_angle));
	if (error != UUcError_None) inParameters->dodge_pattern_angle = 0.0f;
	inParameters->dodge_pattern_angle *= M3cDegToRad;

	// targeting flags
	inParameters->targeting_flags = 0;
	error = GRrGroup_GetElement(inGroup, inAlternateParameters ? "ai_targeting_flags_alt" : "ai_targeting_flags", &element_type, &element);
	if (error == UUcError_None) {
		AUrFlags_ParseFromGroupArray(
			IMPgWeaponTargetingFlags,
			element_type,
			element,
			&inParameters->targeting_flags);
	}

	return UUcError_None;
}