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
#include "Imp_GameSettings.h"
#include "Imp_Furniture.h"

#include "EnvFileFormat.h"

#include "oni.h"
#include "Oni_Object.h"
#include "Oni_Object_Private.h"
#include "Oni_Weapon.h"

// ======================================================================
// functions
// ======================================================================

static void Imp_GetDifficultyMultipliers(GRtGroup *inGroup, const char *inName, float *outMultipliers)
{
	UUtError				error;	
	GRtElementType			element_type;
	GRtElementArray			*multiplier_array;
	void					*element;
	UUtUns32				itr, array_length;
	

	UUmAssertWritePtr(outMultipliers, ONcDifficultyLevel_Count * sizeof(float));

	// set up default multipliers
	for (itr = 0; itr < ONcDifficultyLevel_Count; itr++) {
		outMultipliers[itr] = 1.0f;
	}

	error = GRrGroup_GetElement(inGroup, inName, &element_type, &element);
	if ((error == UUcError_None) && (element_type == GRcElementType_Array)) {
		multiplier_array = (GRtElementArray *) element;

		array_length = GRrGroup_Array_GetLength(multiplier_array);
		array_length = UUmMin(array_length, ONcDifficultyLevel_Count);

		for (itr = 0; itr < array_length; itr++) {
			error =	GRrGroup_Array_GetElement(multiplier_array, itr, &element_type, &element);

			if ((error == UUcError_None) && (element_type == GRcElementType_String)) {
				UUrString_To_Float((const char *) element, &outMultipliers[itr]);
			}			
		}
	}
}

UUtError Imp_AddGameSettings( BFtFileRef* inSourceFileRef, UUtUns32 inSourceFileModDate, GRtGroup* inGroup, char* inInstanceName )
{
	UUtError				error;	
	ONtGameSettings			*settings;
	GRtElementType			element_type;
	GRtElementArray			*sub_array, *main_array;
	void					*element;
	char					*string, keyname[64];
	float					temp_float;
	UUtUns32				itr, r, g, b;

	enum {
		cHealthColor_Value,
		cHealthColor_Red,
		cHealthColor_Green,
		cHealthColor_Blue,

		cHealthColor__ArrayLength
	};

	enum {
		cAutoPrompt_Event,
		cAutoPrompt_StartLevel,
		cAutoPrompt_EndLevel,
		cAutoPrompt_Message,

		cAutoPrompt_ArrayLength
	};

	// build the instance
	error = TMrConstruction_Instance_Renew( ONcTemplate_GameSettings, inInstanceName, 0, (void**) &settings );
	IMPmError_ReturnOnError(error);

	error = GRrGroup_GetFloat( inGroup, "boosted_health", &temp_float );
	if(error == UUcError_None )
		settings->boosted_health	= temp_float + 1.0f;
	else
		settings->boosted_health	= 1.0;

	error = GRrGroup_GetFloat( inGroup, "hypo_amount", &temp_float );
	if(error == UUcError_None )
		settings->hypo_amount		= temp_float;
	else
		settings->hypo_amount		= 0.25;

	error = GRrGroup_GetFloat( inGroup, "hypo_boost_amount", &temp_float );
	if(error == UUcError_None )
		settings->hypo_boost_amount	= temp_float;
	else
		settings->hypo_boost_amount	= 1.0;

	error = GRrGroup_GetFloat( inGroup, "overhypo_base_damage", &temp_float );
	if(error == UUcError_None )
		settings->overhypo_base_damage	= temp_float;
	else
		settings->overhypo_base_damage	= 1.0f;

	error = GRrGroup_GetFloat( inGroup, "overhypo_max_damage", &temp_float );
	if(error == UUcError_None )
		settings->overhypo_max_damage	= temp_float;
	else
		settings->overhypo_max_damage	= 1.5f;

	error = GRrGroup_GetElement(inGroup, "health_colors", &element_type, &element);
	if ((error != UUcError_None) || (element_type != GRcElementType_Array)) {
		settings->healthcolor_numpoints = 2;
		settings->healthcolor_values[0] = 0.0f;		settings->healthcolor_colors[0] = IMcShade_Red;
		settings->healthcolor_values[1] = 1.0f;		settings->healthcolor_colors[1] = IMcShade_Green;
	} else {
		main_array = (GRtElementArray *) element;
		settings->healthcolor_numpoints = UUmMin(GRrGroup_Array_GetLength(main_array), ONcHealthColor_MaxPoints);

		for (itr = 0; itr < settings->healthcolor_numpoints; itr++) {
			// get the sub-array
			error =	GRrGroup_Array_GetElement(main_array, itr, &element_type, &element);
			IMPmError_ReturnOnError(error);
			if (element_type != GRcElementType_Array) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "game settings: elements of health color array must themselves be arrays");
			}
			sub_array = (GRtElementArray *) element;

			// get the value of this point on the ramp
			error =	GRrGroup_Array_GetElement(sub_array, cHealthColor_Value, &element_type, &element);
			if ((error != UUcError_None) || (element_type != GRcElementType_String) || 
				(UUrString_To_Float((const char *) element, &temp_float) != UUcError_None)) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "game settings: health color values must be numbers");
			}			
			settings->healthcolor_values[itr] = temp_float / 100.0f;

			// get the color of this point
			error =	GRrGroup_Array_GetElement(sub_array, cHealthColor_Red, &element_type, &element);
			if ((error != UUcError_None) || (element_type != GRcElementType_String) || 
				(UUrString_To_Uns32((const char *) element, &r) != UUcError_None)) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "game settings: health color red values must be numbers");
			}			
			error =	GRrGroup_Array_GetElement(sub_array, cHealthColor_Green, &element_type, &element);
			if ((error != UUcError_None) || (element_type != GRcElementType_String) || 
				(UUrString_To_Uns32((const char *) element, &g) != UUcError_None)) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "game settings: health color green values must be numbers");
			}			
			error =	GRrGroup_Array_GetElement(sub_array, cHealthColor_Blue, &element_type, &element);
			if ((error != UUcError_None) || (element_type != GRcElementType_String) || 
				(UUrString_To_Uns32((const char *) element, &b) != UUcError_None)) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "game settings: health color blue values must be numbers");
			}			

			settings->healthcolor_colors[itr] = 0xFF000000 | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
		}
	}

	for (itr = 0; itr < WPcPowerup_NumTypes; itr++) {
		// get the powerup geometry
		sprintf(keyname, "powerup_%s", WPgPowerupName[itr]);
		error = GRrGroup_GetString(inGroup, keyname, &string);
		if (error != UUcError_None) {
			string = "hypo";
		}
		UUrString_Copy(settings->powerup_geom[itr], string, 128);

		// get the glow texture
		sprintf(keyname, "glowtex_%s", WPgPowerupName[itr]);
		error = GRrGroup_GetString(inGroup, keyname, &string);
		if (error != UUcError_None) {
			string = "lensflare01";
		}
		UUrString_Copy(settings->powerup_glow[itr], string, 128);

		// get the glow texture size
		sprintf(keyname, "glowsize_x_%s", WPgPowerupName[itr]);
		error = GRrGroup_GetFloat(inGroup, keyname, &settings->powerup_glowsize[itr][0]);
		if (error != UUcError_None) {
			settings->powerup_glowsize[itr][0] = WPcPowerupDefaultGlowSize;
		}

		sprintf(keyname, "glowsize_y_%s", WPgPowerupName[itr]);
		error = GRrGroup_GetFloat(inGroup, keyname, &settings->powerup_glowsize[itr][1]);
		if (error != UUcError_None) {
			settings->powerup_glowsize[itr][1] = WPcPowerupDefaultGlowSize;
		}
	}

	for (itr = 0; itr < ONcEventSound_Max; itr++) {
		sprintf(keyname, "eventsound_%s", ONcEventSoundName[itr]);
		error = GRrGroup_GetString(inGroup, keyname, &string);
		if (error != UUcError_None) {
			string = "";
		}

		UUrString_Copy(settings->event_soundname[itr], string, 32);
	}

	for (itr = 0; itr < ONcConditionSound_Max; itr++) {
		sprintf(keyname, "conditionsound_%s", ONcConditionSoundName[itr]);
		error = GRrGroup_GetString(inGroup, keyname, &string);
		if (error != UUcError_None) {
			string = "";
		}

		UUrString_Copy(settings->condition_soundname[itr], string, 32);
	}

	Imp_GetDifficultyMultipliers(inGroup, "notice_multipliers", settings->notice_multipliers);
	Imp_GetDifficultyMultipliers(inGroup, "blocking_multipliers", settings->blocking_multipliers);
	Imp_GetDifficultyMultipliers(inGroup, "dodge_multipliers", settings->dodge_multipliers);
	Imp_GetDifficultyMultipliers(inGroup, "inaccuracy_multipliers", settings->inaccuracy_multipliers);
	Imp_GetDifficultyMultipliers(inGroup, "enemy_hp_multipliers", settings->enemy_hp_multipliers);
	Imp_GetDifficultyMultipliers(inGroup, "player_hp_multipliers", settings->player_hp_multipliers);

	/*
	 * autoprompt messages
	 */

	settings->num_autoprompts = 0;
	error = GRrGroup_GetElement(inGroup, "autoprompts", &element_type, &element);
	if ((error == UUcError_None) && (element_type == GRcElementType_Array)) {
		main_array = (GRtElementArray *) element;
		settings->num_autoprompts = UUmMin(GRrGroup_Array_GetLength(main_array), ONcAutoPromptMessages_Max);

		for (itr = 0; itr < settings->num_autoprompts; itr++) {
			// get the sub-array
			error =	GRrGroup_Array_GetElement(main_array, itr, &element_type, &element);
			IMPmError_ReturnOnError(error);
			if (element_type != GRcElementType_Array) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "game settings: elements of autoprompt array must themselves be arrays");
			}
			sub_array = (GRtElementArray *) element;

			// get the event of this prompt
			error =	GRrGroup_Array_GetElement(sub_array, cAutoPrompt_Event, &element_type, &element);
			if ((error != UUcError_None) || (element_type != GRcElementType_String)) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "game settings: autoprompt event types must be strings");
			}
			UUrString_Copy(settings->autoprompts[itr].event_name, (const char *) element, 32);

			// get the start and end levels for which this prompt will be displayed
			error =	GRrGroup_Array_GetElement(sub_array, cAutoPrompt_StartLevel, &element_type, &element);
			if ((error != UUcError_None) || (element_type != GRcElementType_String) || 
				(UUrString_To_Uns16((const char *) element, &settings->autoprompts[itr].start_level) != UUcError_None)) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "game settings: autoprompt start level values must be numbers");
			}
			error =	GRrGroup_Array_GetElement(sub_array, cAutoPrompt_EndLevel, &element_type, &element);
			if ((error != UUcError_None) || (element_type != GRcElementType_String) || 
				(UUrString_To_Uns16((const char *) element, &settings->autoprompts[itr].end_level) != UUcError_None)) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "game settings: autoprompt end level values must be numbers");
			}

			// get the message for this prompt
			error =	GRrGroup_Array_GetElement(sub_array, cAutoPrompt_Message, &element_type, &element);
			if ((error != UUcError_None) || (element_type != GRcElementType_String)) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "game settings: autoprompt message name must be a string");
			}
			UUrString_Copy(settings->autoprompts[itr].message_name, (const char *) element, 32);
		}
	}
	

	return UUcError_None;
}

