#pragma once
#ifndef ONI_AI2_ALERT_H
#define ONI_AI2_ALERT_H

/*
	FILE:	 Oni_AI2_Alert.h

	AUTHOR:	 Chris Butcher

	CREATED: April 19, 2000

	PURPOSE: Alert Status for Oni's AI

	Copyright (c) 2000

*/

#include "Oni_AI2_Knowledge.h"

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cAlert_MaxStartleDuration			180
#define AI2cAlert_StartleAimingInterpolation	18

typedef enum AI2tAlertStatus {
	AI2cAlertStatus_Lull		= 0,		// idle or slacking								walk_noaim
	AI2cAlertStatus_Low			= 1,		// on duty or heard interesting sound.			walk_noaim
	AI2cAlertStatus_Medium		= 2,		// heard danger sound							run_noaim
	AI2cAlertStatus_High		= 3,		// heard gunfire								run
	AI2cAlertStatus_Combat		= 4,		// in combat									run
	AI2cAlertStatus_Max			= 5
} AI2tAlertStatus;

extern const char AI2cAlertStatusName[][16];

// ------------------------------------------------------------------------------------
// -- function prototypes

// length of time to glance at a contact for
UUtUns32 AI2rAlert_GetGlanceDuration(AI2tContactType inContactType);

// new-knowledge notification callback for all non-alerted states (idle, guard, patrol)
void AI2rAlert_NotifyKnowledge(ONtCharacter *ioCharacter, AI2tKnowledgeEntry *inEntry,
								UUtBool inDegrade);

// change alert state - returns whether startled
UUtBool AI2rAlert_UpgradeStatus(ONtCharacter *ioCharacter, AI2tAlertStatus inStatus, AI2tKnowledgeEntry *inContact);
void AI2rAlert_DowngradeStatus(ONtCharacter *ioCharacter, AI2tAlertStatus inStatus, AI2tKnowledgeEntry *inContact,
							   UUtBool inLeavingCombat);

// check whether we need to degrade alert state
void AI2rAlert_Decay(ONtCharacter *ioCharacter);

// change alert status of all AIs
void AI2rAlert_DebugAlertAll(AI2tAlertStatus inAlert);

#endif // ONI_AI2_ALERT_H
