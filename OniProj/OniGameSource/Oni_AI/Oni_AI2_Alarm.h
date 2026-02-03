#pragma once
#ifndef ONI_AI2_ALARM_H
#define ONI_AI2_ALARM_H

/*
	FILE:	Oni_AI2_Alarm.h

	AUTHOR:	Chris Butcher

	CREATED: September 2, 2000

	PURPOSE: Alarm AI for Oni

	Copyright (c) 2000 Bungie Software

*/

#define DEBUG_VERBOSE_ALARM			0

// ------------------------------------------------------------------------------------
// -- alarm state structures

// current state of a character running to an alarm
typedef struct AI2tAlarmState
{
	M3tPoint3D				alarm_dest;
	UUtUns32				stand_delay_timer;
} AI2tAlarmState;

// persistent alarm state stored for all characters
typedef struct AI2tPersistentAlarmState
{
//	AI2tKnowledgeEntry *	alarm_knowledge;
	struct ONtActionMarker *action_marker;
	struct ONtActionMarker *last_action_marker;
	UUtUns32				repath_attempts;
	UUtUns32				fight_timer;
} AI2tPersistentAlarmState;

// settings for alarm behavior
typedef struct AI2tAlarmSettings
{
	float					find_distance;
	float					ignore_enemy_dist;
	float					chase_enemy_dist;
	UUtUns32				damage_threshold;
	UUtUns32				fight_timer;

} AI2tAlarmSettings;

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cAlarmSettings_DefaultFindDistance		300.0f
#define AI2cAlarmSettings_DefaultIgnoreEnemy		80.0f
#define AI2cAlarmSettings_DefaultChaseEnemy			0.0f
#define AI2cAlarmSettings_DefaultDamageThreshold	30
#define AI2cAlarmSettings_DefaultFightTimer			360

// ------------------------------------------------------------------------------------
// -- state changing and update routines

// set up an alarm state
void AI2rAlarm_Enter(ONtCharacter *ioCharacter);

// called every tick while running for an alarm
void AI2rAlarm_Update(ONtCharacter *ioCharacter);

// exit an alarm state
void AI2rAlarm_Exit(ONtCharacter *ioCharacter);

// set up an alarm behavior
UUtBool AI2rAlarm_Setup(ONtCharacter *ioCharacter, struct ONtActionMarker *inActionMarker);

// stop an alarm behavior
void AI2rAlarm_Stop(ONtCharacter *ioCharacter, UUtBool inSuccess);

// ------------------------------------------------------------------------------------
// -- global alarm behavior functions

// we were just knocked down
void AI2rAlarm_NotifyKnockdown(ONtCharacter *ioCharacter);

// find the closest alarm console that we could run to
struct ONtActionMarker *AI2rAlarm_FindAlarmConsole(ONtCharacter *ioCharacter);

// check that an alarm console is valid for us to use
UUtBool AI2rAlarm_IsValidConsole(ONtCharacter *ioCharacter, struct OBJtOSD_Console *inConsole);
UUtBool AI2rAlarm_IsValidAlarmConsole(ONtCharacter *ioCharacter, struct OBJtOSD_Console *inConsole);

// ------------------------------------------------------------------------------------
// -- debugging functions

// report in
void AI2rAlarm_Report(ONtCharacter *ioCharacter, AI2tAlarmState *ioAlarmState,
					   UUtBool inVerbose, AI2tReportFunction inFunction);

#endif // ONI_AI2_ALARM_H
