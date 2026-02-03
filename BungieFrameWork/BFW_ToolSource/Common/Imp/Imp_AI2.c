/*
	FILE:	Imp_AI2.c

	AUTHOR:	 Chris Butcher

	CREATED: April 11, 2000

	PURPOSE: AI data importer

	Copyright (c) 2000 Bungie Software
*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_Group.h"
#include "BFW_TM_Construction.h"
#include "BFW_AppUtilities.h"

#include "Imp_Common.h"
#include "Imp_AI2.h"
#include "Imp_Character.h"
#include "Oni_Character.h"
#include "Oni_AI2.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

AUtFlagElement	IMPgAI2BehaviorFlags[] =
{
	{
		"never-startle",
		AI2cBehaviorFlag_NeverStartle
	},
	{
		"can-dodge",
		AI2cBehaviorFlag_DodgeMasterEnable
	},
	{
		"can-dodge-firing",
		AI2cBehaviorFlag_DodgeWhileFiring
	},
	{
		"stop-firing-to-dodge",
		AI2cBehaviorFlag_DodgeStopFiring
	},
	{
		"running-pickup",
		AI2cBehaviorFlag_RunningPickup
	},
	{
		NULL,
		0
	}
};

enum {
	cVocalization_Type,
	cVocalization_Sound,
	cVocalization_Chance,

	cVocalization_ArrayLength
};

UUtError Imp_AI2_ReadClassBehavior(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName,
	ONtCharacterClass	*inClass)
{
	UUtError error;
	char keyname[64];
	float dazed_delay;
	UUtUns32 itr, type_index, num_items;
	AI2tShootingSkill *skill, *default_skill;
	GRtElementArray *groupFlags, *vocalization_array, *sub_array;
	GRtElementType	groupType, elemType;
	void *element;

	/*
	 * flags
	 */
	error = GRrGroup_GetElement(inGroup, "flags", &groupType, &groupFlags);
	if (error == UUcError_None) {
		error = AUrFlags_ParseFromGroupArray(
			IMPgAI2BehaviorFlags,
			groupType,
			groupFlags,
			&inClass->ai2_behavior.flags);
		UUmError_ReturnOnError(error);

		if (inClass->ai2_behavior.flags & AI2cBehaviorFlag_DodgeWhileFiring) {
			// the 'dodge' flag is necessary in addition to the 'dodge while firing' flag, so
			// ensure that it's always set even if the designer didn't specify it.
			inClass->ai2_behavior.flags |= AI2cBehaviorFlag_DodgeMasterEnable;

			// dodge-while-firing takes precedence over stop-firing-to-dodge
			inClass->ai2_behavior.flags &= ~AI2cBehaviorFlag_DodgeStopFiring;

		} else if (inClass->ai2_behavior.flags & AI2cBehaviorFlag_DodgeStopFiring) {
			// the 'dodge' flag is necessary in addition to the 'stop firing to dodge' flag, so
			// ensure that it's always set even if the designer didn't specify it.
			inClass->ai2_behavior.flags |= AI2cBehaviorFlag_DodgeMasterEnable;
		}
	} else {
		inClass->ai2_behavior.flags = 0;
	}

	/*
	 * movement
	 */
	error = GRrGroup_GetFloat(inGroup, "turning_nimbleness", &inClass->ai2_behavior.turning_nimbleness);
	if (error != UUcError_None)	inClass->ai2_behavior.turning_nimbleness = 1.0f;

	error = GRrGroup_GetFloat(inGroup, "daze_delay_min", &dazed_delay);
	if (error == UUcError_None)	{
		inClass->ai2_behavior.dazed_minframes =
			(UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(UUcFramesPerSecond * dazed_delay);
	} else {
		inClass->ai2_behavior.dazed_minframes = 60;
	}

	error = GRrGroup_GetFloat(inGroup, "daze_delay_max", &dazed_delay);
	if (error == UUcError_None)	{
		inClass->ai2_behavior.dazed_maxframes =
			(UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(UUcFramesPerSecond * dazed_delay);
	} else {
		inClass->ai2_behavior.dazed_maxframes = 100;
	}

	// dodging behavior
	error = GRrGroup_GetFloat(inGroup, "dodge_weight_scale", &inClass->ai2_behavior.dodge_weight_scale);
	if (error != UUcError_None)	inClass->ai2_behavior.dodge_weight_scale = 1.0f;

	error = GRrGroup_GetFloat(inGroup, "dodge_time_scale", &inClass->ai2_behavior.dodge_time_scale);
	if (error != UUcError_None)	inClass->ai2_behavior.dodge_time_scale = 1.0f;

	error = GRrGroup_GetUns32(inGroup, "dodge_react_frames", &inClass->ai2_behavior.dodge_react_frames);
	if (error != UUcError_None)	inClass->ai2_behavior.dodge_react_frames = 0;

	/*
	 * default IDs for other kinds of behavior objects
	 */

	error = GRrGroup_GetUns16(inGroup, "default_combat_id", &inClass->ai2_behavior.default_combat_id);
	if (error != UUcError_None)	inClass->ai2_behavior.default_combat_id = 0;

	error = GRrGroup_GetUns16(inGroup, "default_melee_id", &inClass->ai2_behavior.default_melee_id);
	if (error != UUcError_None)	inClass->ai2_behavior.default_melee_id = 0;


	/*
	 * combat parameters
	 */

	error = GRrGroup_GetUns32(inGroup, "gun_pickup_percentage", &inClass->ai2_behavior.combat_parameters.gun_pickup_percentage);
	if (error != UUcError_None)	inClass->ai2_behavior.combat_parameters.gun_pickup_percentage = 100;

	error = GRrGroup_GetUns32(inGroup, "gun_runningpickup_percentage", &inClass->ai2_behavior.combat_parameters.gun_runningpickup_percentage);
	if (error != UUcError_None)	inClass->ai2_behavior.combat_parameters.gun_runningpickup_percentage = 0;

	  /*
	   * targeting parameters
	   */

	    /*
		 * initial deliberately-missed shots
		 */

#if TARGETING_MISS_DECAY
	error = GRrGroup_GetFloat(inGroup, "startle_miss_angle_min",
		&inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle_min);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle_min = 4.5f;
	inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle_min *= M3cDegToRad;

	error = GRrGroup_GetFloat(inGroup, "startle_miss_angle_max",
		&inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle_max);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle_max = 30.0f;
	inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle_max *= M3cDegToRad;

	error = GRrGroup_GetFloat(inGroup, "startle_miss_decay",
		&inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_decay);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_decay = 0.5f;
#else
	error = GRrGroup_GetFloat(inGroup, "startle_miss_angle",
		&inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle = 30.0f;

	inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle =
		UUmPin(inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle, 0.f, 80.0f);
	inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle =
		MUrSin(inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_angle * M3cDegToRad);

	error = GRrGroup_GetFloat(inGroup, "startle_miss_distance",
		&inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_distance);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.targeting_params.startle_miss_distance = 120.0f;
#endif
	    /*
		 * enemy movement prediction
		 */

	error = GRrGroup_GetFloat(inGroup, "predict_amount",
		&inClass->ai2_behavior.combat_parameters.targeting_params.predict_amount);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.targeting_params.predict_amount = 1.0f;

	error = GRrGroup_GetUns32(inGroup, "predict_positiondelayframes",
		&inClass->ai2_behavior.combat_parameters.targeting_params.predict_positiondelayframes);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.targeting_params.predict_positiondelayframes = 0;

	error = GRrGroup_GetUns32(inGroup, "predict_predictdelayframes",
		&inClass->ai2_behavior.combat_parameters.targeting_params.predict_delayframes);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.targeting_params.predict_delayframes = 5;

	error = GRrGroup_GetUns32(inGroup, "predict_velocityframes",
		&inClass->ai2_behavior.combat_parameters.targeting_params.predict_velocityframes);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.targeting_params.predict_velocityframes = 15;

	error = GRrGroup_GetUns32(inGroup, "predict_trendframes",
		&inClass->ai2_behavior.combat_parameters.targeting_params.predict_trendframes);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.targeting_params.predict_trendframes = 60;

	  /*
	   * shooting skill
	   */

	// each weapon has an index assigned to it which is read from the weapon ins file... this is
	// used to index an entry in the shooting skill table.
	//
	// wp0 are the default values that are copied to every other weapon index if the specific
	// weapon's values are not found
	default_skill = inClass->ai2_behavior.combat_parameters.shooting_skill;
	for (itr = 0, skill = default_skill; itr < AI2cCombat_MaxWeapons; itr++, skill++) {

		sprintf(keyname, "wp%d_recoil_compensation", itr);
		error = GRrGroup_GetFloat(inGroup, keyname, &skill->recoil_compensation);
		if (error != UUcError_None)	{
			if (skill == default_skill) {
				skill->recoil_compensation = 0.3f;
			} else {
				skill->recoil_compensation = default_skill->recoil_compensation;
			}
		}

		// best_aiming_angle is read as degrees, stored as sin(theta)
		sprintf(keyname, "wp%d_best_aiming_angle", itr);
		error = GRrGroup_GetFloat(inGroup, keyname, &skill->best_aiming_angle);
		if (error == UUcError_None)	{
			skill->best_aiming_angle = MUrSin(skill->best_aiming_angle * M3cDegToRad);
		} else {
			if (skill == default_skill) {
				skill->best_aiming_angle = 0.5f;
				skill->best_aiming_angle = MUrSin(skill->best_aiming_angle * M3cDegToRad);
			} else {
				skill->best_aiming_angle = default_skill->best_aiming_angle;
			}
		}

		sprintf(keyname, "wp%d_shot_error", itr);
		error = GRrGroup_GetFloat(inGroup, keyname, &skill->shot_error);
		if (error != UUcError_None)	{
			if (skill == default_skill) {
				skill->shot_error = 0.0f;
			} else {
				skill->shot_error = default_skill->shot_error;
			}
		}

		sprintf(keyname, "wp%d_shot_decay", itr);
		error = GRrGroup_GetFloat(inGroup, keyname, &skill->shot_decay);
		if (error != UUcError_None)	{
			if (skill == default_skill) {
				skill->shot_decay = 0.0f;
			} else {
				skill->shot_decay = default_skill->shot_decay;
			}
		}

		sprintf(keyname, "wp%d_inaccuracy", itr);
		error = GRrGroup_GetFloat(inGroup, keyname, &skill->gun_inaccuracy);
		if (error != UUcError_None)	{
			if (skill == default_skill) {
				skill->gun_inaccuracy = 1.0f;
			} else {
				skill->gun_inaccuracy = default_skill->gun_inaccuracy;
			}
		}

		sprintf(keyname, "wp%d_delay_min", itr);
		error = GRrGroup_GetUns16(inGroup, keyname, &skill->delay_min);
		if (error != UUcError_None)	{
			if (skill == default_skill) {
				skill->delay_min = 0;
			} else {
				skill->delay_min = default_skill->delay_min;
			}
		}

		sprintf(keyname, "wp%d_delay_max", itr);
		error = GRrGroup_GetUns16(inGroup, keyname, &skill->delay_max);
		if (error != UUcError_None)	{
			if (skill == default_skill) {
				skill->delay_max = 0;
			} else {
				skill->delay_max = default_skill->delay_max;
			}
		}
	}

	  /*
	   * delay frames until transition out of combat state
	   */

	error = GRrGroup_GetUns32(inGroup, "dead_makesure_delay", &inClass->ai2_behavior.combat_parameters.dead_makesure_delay);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.dead_makesure_delay = 90;

	error = GRrGroup_GetUns32(inGroup, "investigate_body_delay", &inClass->ai2_behavior.combat_parameters.investigate_body_delay);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.investigate_body_delay = 240;

	error = GRrGroup_GetUns32(inGroup, "lost_contact_delay", &inClass->ai2_behavior.combat_parameters.lost_contact_delay);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.lost_contact_delay = 180;

	error = GRrGroup_GetUns32(inGroup, "dead_taunt_percentage", &inClass->ai2_behavior.combat_parameters.dead_taunt_percentage);
	if (error != UUcError_None)
		inClass->ai2_behavior.combat_parameters.dead_taunt_percentage = 40;

	/*
	 * vocalization table
	 */

	// clear the vocalization table
	UUrMemory_Clear(&inClass->ai2_behavior.vocalizations, sizeof(inClass->ai2_behavior.vocalizations));

	error = GRrGroup_GetElement(inGroup, "vocalizations", &elemType, &element);
	if (error == UUcError_None) {
		if (elemType != GRcElementType_Array) {
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "'vocalizations' field is not an array!");
		}

		// read each vocalization from the array
		vocalization_array = (GRtElementArray *) element;
		num_items = GRrGroup_Array_GetLength(vocalization_array);

		for (itr = 0; itr < num_items; itr++) {
			// get the sub-array
			error =	GRrGroup_Array_GetElement(vocalization_array, itr, &elemType, &element);
			IMPmError_ReturnOnError(error);
			if (elemType != GRcElementType_Array) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "vocalization table: elements of array must themselves be arrays");
			}

			sub_array = (GRtElementArray *) element;
			if (GRrGroup_Array_GetLength(sub_array) != cVocalization_ArrayLength) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "vocalization table: array has incorrect number of elements: [type, sound, %% chance]");
			}

			// read the animation type
			error =	GRrGroup_Array_GetElement(sub_array, cVocalization_Type, &elemType, &element);
			IMPmError_ReturnOnError(error);
			if (elemType != GRcElementType_String) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "vocalization table: entry 1 must be a string (vocalization type)");
			}

			// work out which index in the array this corresponds to
			for (type_index = 0; type_index < AI2cVocalization_Max; type_index++) {
				if (UUmString_IsEqual(AI2cVocalizationTypeName[type_index], (const char *) element)) {
					break;
				}
			}
			if (type_index >= AI2cVocalization_Max) {
				Imp_PrintMessage(IMPcMsg_Important, "vocalization table: unknown type %s. valid vocalization types are:\n",
								(const char *) element);
				for (type_index = 0; type_index < AI2cVocalization_Max; type_index++) {
					Imp_PrintMessage(IMPcMsg_Important, "  %s\n", AI2cVocalizationTypeName[type_index]);
				}
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "vocalization table: unknown type");
			}

			// read the sound name
			error =	GRrGroup_Array_GetElement(sub_array, cVocalization_Sound, &elemType, &element);
			IMPmError_ReturnOnError(error);
			if (elemType != GRcElementType_String) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "vocalization table: entry 2 must be a string (sound name)");
			}
			UUrString_Copy(inClass->ai2_behavior.vocalizations.sound_name[type_index], (const char *) element, BFcMaxFileNameLength);

			// read the chance of playing
			error =	GRrGroup_Array_GetElement(sub_array, cVocalization_Chance, &elemType, &element);
			IMPmError_ReturnOnError(error);
			if (elemType != GRcElementType_String) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "vocalization table: entry 3 must be a number (play chance)");
			}
			error = UUrString_To_Uns8((const char *) element, &inClass->ai2_behavior.vocalizations.sound_chance[type_index]);
			IMPmError_ReturnOnErrorMsg(error, "vocalization table: could not read sound play chance");
		}
	}

	return UUcError_None;
}

