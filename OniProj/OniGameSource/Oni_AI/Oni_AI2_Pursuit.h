#pragma once
#ifndef ONI_AI2_Pursuit_H
#define ONI_AI2_Pursuit_H

/*
	FILE:	Oni_AI2_Pursuit.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: June 21, 2000
	
	PURPOSE: Pursuit AI for Oni
	
	Copyright (c) 2000

*/

#include "Oni_GameState.h"
#include "Oni_AI2_Knowledge.h"
#include "Oni_AI2_Guard.h"

// ------------------------------------------------------------------------------------
// -- constants

typedef enum AI2tPursuitMode {
	AI2cPursuitMode_None		= 0,
	AI2cPursuitMode_Forget		= 1,
	AI2cPursuitMode_GoTo		= 2,
	AI2cPursuitMode_Wait		= 3,
	AI2cPursuitMode_Look		= 4,
	AI2cPursuitMode_Move		= 5,
	AI2cPursuitMode_Hunt		= 6,
	AI2cPursuitMode_Glance		= 7,

	AI2cPursuitMode_Max			= 8
} AI2tPursuitMode;

typedef enum AI2tPursuitLostBehavior {
	AI2cPursuitLost_Return		= 0,
	AI2cPursuitLost_KeepLooking	= 1,
	AI2cPursuitLost_Alarm		= 2,

	AI2cPursuitLost_Max			= 3
} AI2tPursuitLostBehavior;

extern const char *AI2cPursuitModeName[];
extern const char *AI2cPursuitLostName[];

// ------------------------------------------------------------------------------------
// -- structures

// settings for a character's pursuit behavior, read from
// the character object
typedef struct AI2tPursuitSettings {
	AI2tPursuitMode				strong_unseen;
	AI2tPursuitMode				weak_unseen;
	AI2tPursuitMode				strong_seen;
	AI2tPursuitMode				weak_seen;

	AI2tPursuitLostBehavior		pursue_lost;
} AI2tPursuitSettings;

// the state of a pursuing AI
typedef struct AI2tPursuitState {
	// global state
	AI2tKnowledgeEntry	*target;
	AI2tPursuitMode		current_mode;
	UUtBool				certain_of_enemy;
	UUtBool				pursue_danger;
	UUtBool				been_close_to_contact;
	UUtUns32			say_timer;

	// state changing
	UUtUns32			pause_timer;
	UUtUns32			delay_timer;

	// GoTo state
	UUtUns32			last_los_check;
	UUtUns32			los_successful_checks;
	UUtBool				running_to_contact;

	// direction scanning
	UUtBool				scanning_area;
	AI2tScanState		scan_state;

} AI2tPursuitState;

// ------------------------------------------------------------------------------------
// -- update and state change functions

// set up a patrol state - called when the character goes into "patrol mode".
void AI2rPursuit_Enter(ONtCharacter *ioCharacter);

// called every tick while patrolling
void AI2rPursuit_Update(ONtCharacter *ioCharacter);

// exit a patrol state - called when the character goes out of "patrol mode".
void AI2rPursuit_Exit(ONtCharacter *ioCharacter);

// pause a patrol for some number of frames
void AI2rPursuit_Pause(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState, UUtUns32 inPauseFrames);

// is the character hurrying?
UUtBool AI2rPursuit_IsHurrying(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState);

// an entry in our knowledge base has just been removed, forcibly delete any references we had to it
UUtBool AI2rPursuit_NotifyKnowledgeRemoved(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState,
										   AI2tKnowledgeEntry *inEntry);

// ------------------------------------------------------------------------------------
// -- debugging functions

// report in
void AI2rPursuit_Report(ONtCharacter *ioCharacter, AI2tPursuitState *ioPursuitState,
					   UUtBool inVerbose, AI2tReportFunction inFunction);

#endif // ONI_AI2_Pursuit_H
