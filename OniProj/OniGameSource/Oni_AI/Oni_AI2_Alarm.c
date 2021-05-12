/*
	FILE:	Oni_AI2_Alarm.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: September 2, 2000
	
	PURPOSE: Alarm AI for Oni
	
	Copyright (c) 2000 Bungie Software

*/

#include "Oni_AI2.h"
#include "Oni_Character.h"
#include "Oni_Object.h"
#include "Oni_GameState.h"

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cAlarm_OffsetDistance			5.0f
#define AI2cAlarm_MaxRepathAttempts			3
#define AI2cAlarm_StandDelay				60

// ------------------------------------------------------------------------------------
// -- internal structures

typedef struct AI2tFindConsoleUserData {
	ONtCharacter *	character;
	float			max_distance;

	OBJtObject *	current_console;
	float			current_distance;

} AI2tFindConsoleUserData;

// ------------------------------------------------------------------------------------
// -- internal prototypes

// finish our alarm state
static void AI2iAlarm_Finish(ONtCharacter *ioCharacter, AI2tAlarmState *ioAlarmState,
							 AI2tKnowledgeEntry *inTarget);

// ------------------------------------------------------------------------------------
// -- state changing and update routines

// enter our alarm behavior
void AI2rAlarm_Enter(ONtCharacter *ioCharacter)
{
	AI2tAlarmState *alarm_state;
	ONtActionMarker *alarm_marker;
	AI2tKnowledgeEntry *entry;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Alarm);
	alarm_state = &ioCharacter->ai2State.currentState->state.alarm;

	// set up our knowledge notify function so that we are alerted by stuff
	ioCharacter->ai2State.knowledgeState.callback = &AI2rAlert_NotifyKnowledge;

	// zero all damage amounts stored in our knowledge base
	for (entry = ioCharacter->ai2State.knowledgeState.contacts; entry != NULL; entry = entry->nextptr) {
		entry->total_damage = 0;
	}

#if DEBUG_VERBOSE_ALARM
	COrConsole_Printf("alarm: enter alarm state");
#endif
	alarm_marker = ioCharacter->ai2State.alarmStatus.action_marker;
	UUmAssertReadPtr(alarm_marker, sizeof(ONtActionMarker));

	// build a point which we can stand at to activate the alarm console from
	MUmVector_Copy(alarm_state->alarm_dest, alarm_marker->position);
	alarm_state->alarm_dest.x -= AI2cAlarm_OffsetDistance * MUrSin(alarm_marker->direction);
	alarm_state->alarm_dest.z -= AI2cAlarm_OffsetDistance * MUrCos(alarm_marker->direction);
	alarm_state->stand_delay_timer = 0;

	// go to the alarm console
	AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_NoAim_Run);
	AI2rMovement_StopAiming(ioCharacter);
	AI2rMovement_StopGlancing(ioCharacter);
	AI2rMovement_FreeFacing(ioCharacter);
	AI2rPath_GoToPoint(ioCharacter, &alarm_state->alarm_dest, ONcActionMarker_MaxDistance - AI2cAlarm_OffsetDistance,
						UUcTrue, ioCharacter->ai2State.alarmStatus.action_marker->direction, NULL);
}

void AI2rAlarm_Update(ONtCharacter *ioCharacter)
{
	float distance_sq;
	AI2tPersistentAlarmState *persistent_state;
	AI2tAlarmState *alarm_state;
	AI2tKnowledgeEntry *entry;
	UUtBool can_press_alarm;

	UUmAssert(ioCharacter->charType == ONcChar_AI2);
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Alarm);
	alarm_state = &ioCharacter->ai2State.currentState->state.alarm;
	persistent_state = &ioCharacter->ai2State.alarmStatus;

	if (ioCharacter->console_marker != NULL) {
		// we are performing the console action, wait for it to finish
#if DEBUG_VERBOSE_ALARM
		COrConsole_Printf("alarm: wait for console action");
#endif
		AI2rPath_Halt(ioCharacter);
		return;
	}

	if (persistent_state->action_marker == NULL) {
		// no console for us to go to! there has been an error
#if DEBUG_VERBOSE_ALARM
		COrConsole_Printf("alarm: ERROR: no action marker");
#endif
		AI2iAlarm_Finish(ioCharacter, alarm_state, NULL);
		return;
	}

	// check to see if we want to fall out of our alarm behavior and attack someone
	for (entry = ioCharacter->ai2State.knowledgeState.contacts; entry != NULL; entry = entry->nextptr) {
		if ((entry->priority < AI2cContactPriority_Hostile_Threat) || (entry->strength < AI2cContactStrength_Definite)) {
			// only consider attacking definite threats
			continue;
		}

		distance_sq = MUrPoint_Distance_Squared(&entry->last_location, &ioCharacter->actual_position);
		if (distance_sq > UUmSQR(ioCharacter->ai2State.alarmSettings.ignore_enemy_dist)) {
			// ignore all characters further away than a certain distance
			continue;

		} else if ((distance_sq > UUmSQR(ioCharacter->ai2State.alarmSettings.chase_enemy_dist)) &&
					(entry->total_damage < ioCharacter->ai2State.alarmSettings.damage_threshold)) {
			// for characters who are further away than our chase distance, only go after them if
			// they have dealt a significant amount of damage to us
			continue;
		}

		// attack this character
		AI2iAlarm_Finish(ioCharacter, alarm_state, entry);
	}
	
	if (!ioCharacter->pathState.at_finalpoint) {
		// keep running to alarm!

		if (ONrCharacter_IsFallen(ioCharacter)) {
			// we need to get up from lying on the ground, we have been knocked down.
			// note that if we get knocked down AI2rAlarm_NotifyKnockdown will be called,
			// which may have dropped us out of the alarm state, so we'll only ever get here
			// if we want to keep going for the alarm.
#if DEBUG_VERBOSE_ALARM
			COrConsole_Printf("alarm: get up (continue running)");
#endif
			AI2rExecutor_MoveOverride(ioCharacter, LIc_BitMask_Forward);
		}

		return;
	}

	if (ONrCharacter_IsStanding(ioCharacter)) {
		can_press_alarm = ONrGameState_TryActionMarker(ioCharacter, persistent_state->action_marker, UUcTrue);
		if (can_press_alarm) {
			// we can use the alarm! we're done here
#if DEBUG_VERBOSE_ALARM
			COrConsole_Printf("alarm: beginning console action");
#endif
			ONrCharacter_ConsoleAction_Begin(ioCharacter, persistent_state->action_marker);
			AI2rPath_Halt(ioCharacter);
			return;
		} else {
			// maybe we aren't facing in the right direction yet?
			if (!ioCharacter->pathState.at_destination) {
				// we will be able to use the console when we reach our destination facing
#if DEBUG_VERBOSE_ALARM
				COrConsole_Printf("alarm: do facing");
#endif
				return;
			}
		}
	} else {
		// wait until we are stationary before we try to use the console
		AI2rPath_Halt(ioCharacter);
		if (alarm_state->stand_delay_timer == 0) {
			alarm_state->stand_delay_timer = AI2cAlarm_StandDelay;
		}
		alarm_state->stand_delay_timer--;
		if (alarm_state->stand_delay_timer == 0) {
			// we've delayed for our timer and we still aren't standing, something is wrong.
		} else {
#if DEBUG_VERBOSE_ALARM
			COrConsole_Printf("alarm: waiting until stationary (timer %d)", alarm_state->stand_delay_timer);
#endif
			return;
		}
	}

	// for some reason, we can't use the alarm console despite the fact that we are at
	// our destination
	if (ONrCharacter_IsFallen(ioCharacter)) {
		// we need to get up from lying on the ground, maybe we have been knocked down.
		// get up away from the console (backwards).
#if DEBUG_VERBOSE_ALARM
		COrConsole_Printf("alarm: get up (at console)");
#endif
		AI2rExecutor_MoveOverride(ioCharacter, LIc_BitMask_Backward);
		return;
	}

	// we can't hit the console from here, try getting there again
	if (persistent_state->repath_attempts >= AI2cAlarm_MaxRepathAttempts) {
		// give up on this alarm
#if DEBUG_VERBOSE_ALARM
		COrConsole_Printf("alarm: FAIL: cannot reach alarm (tried %d repaths)", persistent_state->repath_attempts);
#endif
		AI2rAlarm_Stop(ioCharacter, UUcFalse);
		return;
	}
#if DEBUG_VERBOSE_ALARM
	COrConsole_Printf("alarm: trying repath");
#endif
	AI2rPath_GoToPoint(ioCharacter, &alarm_state->alarm_dest, ONcActionMarker_MaxDistance - AI2cAlarm_OffsetDistance,
						UUcTrue, persistent_state->action_marker->direction, NULL);
	persistent_state->repath_attempts++;
}

// exit from our alarm behavior
void AI2rAlarm_Exit(ONtCharacter *ioCharacter)
{
#if DEBUG_VERBOSE_ALARM
	COrConsole_Printf("alarm: exit alarm state");
#endif
	AI2rPath_Halt(ioCharacter);
	AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Default);
}

// set up an alarm behavior
UUtBool AI2rAlarm_Setup(ONtCharacter *ioCharacter, ONtActionMarker *inActionMarker)
{
	UUmAssert(ioCharacter->charType == ONcChar_AI2);

	if (inActionMarker == NULL) {
		// this is a request to continue our existing alarm behavior
		if (ioCharacter->ai2State.alarmStatus.action_marker == NULL) {
			// we don't have one. do nothing.
			return UUcFalse;
		} else {
			inActionMarker = ioCharacter->ai2State.alarmStatus.action_marker;
		}

	} else if (ioCharacter->ai2State.alarmStatus.action_marker != inActionMarker) {
		// we are going for a new alarm
		if (ioCharacter->ai2State.alarmStatus.action_marker != NULL) {
			// clear our previous alarm behavior
			AI2rAlarm_Stop(ioCharacter, UUcFalse);
		}
		ioCharacter->ai2State.alarmStatus.action_marker = inActionMarker;
	}

	ioCharacter->ai2State.alarmStatus.fight_timer = 0;

	// only clear the repath_attempts field if we are trying for a new alarm console...
	// this should prevent endlessly trying to reach an unreachable console
	if (inActionMarker != ioCharacter->ai2State.alarmStatus.last_action_marker) {
		ioCharacter->ai2State.alarmStatus.repath_attempts = 0;
		ioCharacter->ai2State.alarmStatus.last_action_marker = inActionMarker;
	}

	// begin our alarm behavior
	AI2rExitState(ioCharacter);

	ioCharacter->ai2State.currentGoal = AI2cGoal_Alarm;
	ioCharacter->ai2State.currentState = &(ioCharacter->ai2State.currentStateBuffer);
	ioCharacter->ai2State.currentState->begun = UUcFalse;

	AI2rEnterState(ioCharacter);

	return UUcTrue;
}

// stop an alarm behavior
void AI2rAlarm_Stop(ONtCharacter *ioCharacter, UUtBool inSuccess)
{
	UUmAssert(ioCharacter->charType == ONcChar_AI2);
	
#if DEBUG_VERBOSE_ALARM
	COrConsole_Printf("alarm: stop");
#endif
	if (ioCharacter->ai2State.currentGoal == AI2cGoal_Alarm) {
		// stop trying to go to this alarm
#if DEBUG_VERBOSE_ALARM
		COrConsole_Printf("alarm: returning to job");
#endif
		AI2iAlarm_Finish(ioCharacter, &ioCharacter->ai2State.currentState->state.alarm, NULL);
	}

	UUmAssert(ioCharacter->ai2State.currentGoal != AI2cGoal_Alarm);

	// forget about this alarm
	ioCharacter->ai2State.alarmStatus.action_marker = NULL;
//	ioCharacter->ai2State.alarmStatus.alarm_knowledge = NULL;
	ioCharacter->ai2State.alarmStatus.fight_timer = 0;
}

// get our hostile contact
static AI2tKnowledgeEntry *AI2iAlarm_HostileCharacter(ONtCharacter *ioCharacter, AI2tAlarmState *ioAlarmState)
{
	AI2tKnowledgeEntry *entry, *best_entry;
	float dist_sq, best_dist_sq;

/*	if (inWantAlarm && (ioCharacter->ai2State.alarmStatus.alarm_knowledge != NULL)) {
		// we were sent into the alarm state by a specific hostile character, return them
		return ioCharacter->ai2State.alarmStatus.alarm_knowledge;
	}*/

	best_entry = NULL;
	for (entry = ioCharacter->ai2State.knowledgeState.contacts; entry != NULL; entry = entry->nextptr) {
		if ((entry->priority < AI2cContactPriority_Hostile_Threat) || (entry->strength <= AI2cContactStrength_Forgotten)) {
			continue;
		}

		dist_sq = MUrPoint_Distance_Squared(&entry->last_location, &ioCharacter->actual_position);
		if (best_entry == NULL) {
			best_entry = entry;
			best_dist_sq = dist_sq;

		} else if (entry->strength < best_entry->strength) {
			return best_entry;

		} else if (dist_sq < best_dist_sq) {
			best_entry = entry;
			best_dist_sq = dist_sq;
		}
	}

	return best_entry;
}

// finish our alarm state
static void AI2iAlarm_Finish(ONtCharacter *ioCharacter, AI2tAlarmState *ioAlarmState,
							 AI2tKnowledgeEntry *inTarget)
{
	if (inTarget == NULL) {
		// get the nearest hostile character to us
		inTarget = AI2iAlarm_HostileCharacter(ioCharacter, ioAlarmState);
	} else {
		// this is a deliberate falling-out of alarm behavior in order to attack a specific
		// character... set our fight timer so that we will return to our alarm behavior after
		// a set number of seconds
		ioCharacter->ai2State.alarmStatus.fight_timer = ioCharacter->ai2State.alarmSettings.fight_timer;
	}

	if (inTarget == NULL) {
		// go back to our job
		AI2rReturnToJob(ioCharacter, UUcTrue, UUcTrue);
	} else {
		// transition into the pursuit state and follow target.
		AI2rExitState(ioCharacter);

		ioCharacter->ai2State.currentGoal = AI2cGoal_Pursuit;
		ioCharacter->ai2State.currentStateBuffer.state.pursuit.target = inTarget;
		ioCharacter->ai2State.currentState = &(ioCharacter->ai2State.currentStateBuffer);
		ioCharacter->ai2State.currentState->begun = UUcFalse;

		AI2rEnterState(ioCharacter);
	}
}

// ------------------------------------------------------------------------------------
// -- global alarm behavior functions

// we were just knocked down
void AI2rAlarm_NotifyKnockdown(ONtCharacter *ioCharacter)
{
	AI2tKnowledgeEntry *entry;
	AI2tAlarmState *alarm_state;
	float distance_squared;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Alarm);

	if ((ioCharacter->ai2State.combatSettings.flags & OBJcCombatFlags_Alarm_AttackIfKnockedDown) == 0) {
		// continue following our alarm behavior
		return;
	}

	alarm_state = &ioCharacter->ai2State.currentState->state.alarm;

	// find the nearest hostile character
	entry = AI2iAlarm_HostileCharacter(ioCharacter, alarm_state);
	if ((entry == NULL) || (entry->enemy == NULL) || (entry->strength != AI2cContactStrength_Definite)) {
		// don't go after tentative contacts
		return;
	}

	distance_squared = MUrPoint_Distance_Squared(&ioCharacter->actual_position, &entry->enemy->actual_position);
	if (distance_squared > UUmSQR(ioCharacter->ai2State.alarmSettings.ignore_enemy_dist)) {
		// ignore this character, they are too far away
		return;
	}

	// pause our alarm behavior and attack this character
	AI2iAlarm_Finish(ioCharacter, alarm_state, entry);
}

UUtBool AI2rAlarm_IsValidConsole(ONtCharacter *ioCharacter, OBJtOSD_Console *inConsole)
{
	if ((inConsole->flags & OBJcConsoleFlag_Active) == 0) {
		return UUcFalse;
	}

	if (inConsole->flags & OBJcConsoleFlag_Triggered) {
		return UUcFalse;
	}

	return UUcTrue;
}

UUtBool AI2rAlarm_IsValidAlarmConsole(ONtCharacter *ioCharacter, OBJtOSD_Console *inConsole)
{
	if ((inConsole->flags & OBJcConsoleFlag_IsAlarm) == 0) {
		return UUcFalse;
	}

	return AI2rAlarm_IsValidConsole(ioCharacter, inConsole);
}

static UUtBool AI2iAlarm_FindConsoleEnum(OBJtObject *inObject, UUtUns32 inUserData)
{
	AI2tFindConsoleUserData *user_data = (AI2tFindConsoleUserData *) inUserData;
	OBJtOSD_Console *console_osd;
	float this_distance;
	UUtError error;

	UUmAssertReadPtr(user_data, sizeof(AI2tFindConsoleUserData));

	UUmAssert(inObject->object_type == OBJcType_Console);
	console_osd = (OBJtOSD_Console *) inObject->object_data;

	if (!AI2rAlarm_IsValidAlarmConsole(user_data->character, console_osd)) {
		// cannot use this console
		return UUcTrue;
	}

	this_distance = user_data->max_distance;
	error = PHrPathDistance(user_data->character, (UUtUns32) -1, &user_data->character->actual_position, &console_osd->action_marker->position,
							user_data->character->currentPathnode, NULL, &this_distance);
	if (error != UUcError_None) {
		// no path or too far away
		return UUcTrue;
	}

	if ((user_data->current_console == NULL) || (this_distance < user_data->current_distance)) {
		// store this console
		user_data->current_console = inObject;
		user_data->current_distance = this_distance;
	}

	return UUcTrue;
}

// find the nearest alarm console to a character
ONtActionMarker *AI2rAlarm_FindAlarmConsole(ONtCharacter *ioCharacter)
{
	AI2tFindConsoleUserData user_data;

	user_data.character = ioCharacter;
	user_data.max_distance = ioCharacter->ai2State.alarmSettings.find_distance;
	user_data.current_console = NULL;
	OBJrObjectType_EnumerateObjects(OBJcType_Console, AI2iAlarm_FindConsoleEnum, (UUtUns32) &user_data);

	if (user_data.current_console == NULL) {
		return NULL;
	} else {
		UUmAssert(user_data.current_console->object_type == OBJcType_Console);
		return ((OBJtOSD_Console *) user_data.current_console->object_data)->action_marker;
	}
}

// ------------------------------------------------------------------------------------
// -- debugging functions

// report in
void AI2rAlarm_Report(ONtCharacter *ioCharacter, AI2tAlarmState *ioAlarmState,
					   UUtBool inVerbose, AI2tReportFunction inFunction)
{
	char reportbuf[256];
	OBJtObject *object;
	OBJtOSD_Console *console_osd;

	UUmAssertReadPtr(ioCharacter->ai2State.alarmStatus.action_marker, sizeof(ONtActionMarker));

	object = (OBJtObject *) ioCharacter->ai2State.alarmStatus.action_marker->object;
	UUmAssertReadPtr(object, sizeof(OBJtObject));

	console_osd = (OBJtOSD_Console *) object->object_data;
	UUmAssertReadPtr(console_osd, sizeof(OBJtOSD_Console));

	sprintf(reportbuf, "  trying to reach console %d at (%f %f %f) facing %f, currently %d repath attempts",
			console_osd->id,
			ioCharacter->ai2State.alarmStatus.action_marker->position.x,
			ioCharacter->ai2State.alarmStatus.action_marker->position.y,
			ioCharacter->ai2State.alarmStatus.action_marker->position.z,
			ioCharacter->ai2State.alarmStatus.action_marker->direction,
			ioCharacter->ai2State.alarmStatus.repath_attempts);
	inFunction(reportbuf);
}

