#pragma once

/*
	FILE:	Oni_GameSettings.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: September 25, 2000
	
	PURPOSE: Global game settings for Oni
	
	Copyright Bungie Software, 2000

*/
#ifndef ONI_GAMESETTINGS_H
#define ONI_GAMESETTINGS_H

#include "BFW.h"
#include "BFW_SoundSystem2.h"

// ------------------------------------------------------------------------------------
// -- game sounds

enum {
	ONcEventSound_DoorAction_Success	= 0,
	ONcEventSound_DoorAction_Fail		= 1,
	ONcEventSound_Door_Lock				= 2,
	ONcEventSound_Door_Unlock			= 3,
	ONcEventSound_UseHypo				= 4,
	ONcEventSound_HealthFull			= 5,
	ONcEventSound_InventoryFail			= 6,
	ONcEventSound_ReceiveAmmo			= 7,
	ONcEventSound_ReceiveCell			= 8,
	ONcEventSound_ReceiveHypo			= 9,
	ONcEventSound_ReceiveLSI			= 10,
	ONcEventSound_CompassNew			= 11,
	ONcEventSound_ObjectiveNew			= 12,
	ONcEventSound_ObjectivePrompt		= 13,
	ONcEventSound_ObjectiveComplete		= 14,
	ONcEventSound_Autosave				= 15,
	ONcEventSound_TimerTick				= 16,
	ONcEventSound_TimerSuccess			= 17,

	ONcEventSound_Max					= 18
};

extern const char *ONcEventSoundName[ONcEventSound_Max];

enum {
	ONcConditionSound_HealthLow			= 0,
	ONcConditionSound_HealthOver		= 1,
	ONcConditionSound_Shield			= 2,
	ONcConditionSound_Invis				= 3,
	ONcConditionSound_TimerCritical		= 4,

	ONcConditionSound_Max				= 5
};

extern const char *ONcConditionSoundName[ONcConditionSound_Max];

// ------------------------------------------------------------------------------------
// -- game settings

#define ONcHealthColor_MaxPoints		16
#define ONcAutoPromptMessages_Max		24

typedef tm_struct ONtAutoPrompt
{
	char			event_name[32];
	UUtUns16		start_level;
	UUtUns16		end_level;
	char			message_name[32];

} ONtAutoPrompt;

#define ONcTemplate_GameSettings		UUm4CharToUns32('O', 'N', 'G', 'S')
typedef tm_template('O', 'N', 'G', 'S', "Oni Game Settings")
ONtGameSettings
{
	float				boosted_health;
	float				hypo_amount;
	float				hypo_boost_amount;
	float				overhypo_base_damage;
	float				overhypo_max_damage;

	UUtUns32			healthcolor_numpoints;
	float				healthcolor_values[16];			// must match ONcHealthColor_MaxPoints
	UUtUns32			healthcolor_colors[16];

	// instance names for powerup geometries and glow textures
	char				powerup_geom[7][128];			// must match WPcPowerup_NumTypes
	char				powerup_glow[7][128];
	float				powerup_glowsize[7][2];

	// sound names
	char				event_soundname[18][32];		// must match ONcEventSound_Max
	char				condition_soundname[5][32];		// must match ONcConditionSound_Max

	// difficulty settings
	float				notice_multipliers[3];
	float				blocking_multipliers[3];
	float				dodge_multipliers[3];
	float				inaccuracy_multipliers[3];
	float				enemy_hp_multipliers[3];
	float				player_hp_multipliers[3];

	// autoprompt messages
	UUtUns32			num_autoprompts;
	ONtAutoPrompt		autoprompts[16];				// must match ONcAutoPromptMessages_Max

} ONtGameSettings;

typedef struct ONtGameSettingsRuntime {
	// sound pointers
	SStImpulse *	event_sound[ONcEventSound_Max];
	SStAmbient *	condition_sound[ONcConditionSound_Max];

} ONtGameSettingsRuntime;

extern ONtGameSettings			*ONgGameSettings;
extern ONtGameSettingsRuntime	ONgGameSettingsRuntime;

// ------------------------------------------------------------------------------------
// -- function prototypes

UUtError ONrGameSettings_Initialize(void);
void ONrGameSettings_CalculateSoundPointers(void);

#endif /* ONI_GAMESETTINGS_H */
