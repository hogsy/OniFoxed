/*
	FILE:	Oni_GameSettings.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: September 25, 2000
	
	PURPOSE: Global game settings for Oni
	
	Copyright Bungie Software, 2000

*/

#include "BFW.h"
#include "Oni_GameSettings.h"
#include "Oni_GameState.h"
#include "Oni_Weapon.h"

// ------------------------------------------------------------------------------------
// -- constants

const char *ONcEventSoundName[ONcEventSound_Max]
= {"door_success", "door_fail", "door_lock", "door_unlock", "use_hypo", "health_full", "inventory_fail",
	"receive_ammo", "receive_cell", "receive_hypo", "receive_lsi", "compass", "objective_new", "objective_prompt", "objective_complete",
	"autosave", "timer_tick", "timer_success"};

const char *ONcConditionSoundName[ONcConditionSound_Max]
= {"health_low", "health_over", "shield", "invis", "timer_critical"};

// ------------------------------------------------------------------------------------
// -- external globals

ONtGameSettings	*ONgGameSettings = NULL;
ONtGameSettingsRuntime ONgGameSettingsRuntime;

// ------------------------------------------------------------------------------------
// -- initialization and setup

UUtError ONrGameSettings_Initialize(void)
{
	UUtError					error;
	
	UUrMemory_Clear(&ONgGameSettingsRuntime, sizeof(ONgGameSettingsRuntime));

	error = TMrInstance_GetDataPtr( ONcTemplate_GameSettings, "game_settings", &ONgGameSettings );
	UUmError_ReturnOnErrorMsg(error, "Could not access game_settings template");

	WPgHypoStrength		= (UUtInt32)(100.0f * ONgGameSettings->hypo_amount);

	return UUcError_None;
}

// turn sound names into sound pointers - called from OSrUpdateSoundPointers()
void ONrGameSettings_CalculateSoundPointers(void)
{
	UUtUns32					itr;

	for (itr = 0; itr < ONcEventSound_Max; itr++) {
		ONgGameSettingsRuntime.event_sound[itr] = SSrImpulse_GetByName(ONgGameSettings->event_soundname[itr]);
	}

	for (itr = 0; itr < ONcConditionSound_Max; itr++) {
		ONgGameSettingsRuntime.condition_sound[itr] = SSrAmbient_GetByName(ONgGameSettings->condition_soundname[itr]);
	}
}

