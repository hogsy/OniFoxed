/*
	FILE:	Oni_AI2_Patrol.c

	AUTHOR:	Michael Evans, Chris Butcher

	CREATED: November 15, 1999

	PURPOSE: Patrol AI for Oni

	Copyright (c) 1999

*/

#include "bfw_math.h"

#include "BFW_ScriptLang.h"

#include "Oni_AI2.h"
#include "Oni_AI2_Patrol.h"
#include "Oni_Character.h"
#include "Oni_Object.h"
#include "Oni_GameStatePrivate.h"

// ------------------------------------------------------------------------------------
// -- internal constants

#define AI2cPatrol_ReturnToPathDistance		40.0f
#define AI2cPatrol_FailThreshold			4

#define AI2cPatrol_RecalcDirectionAngle		(3.0f * M3cDegToRad)
#define AI2cPatrol_ScanLookDistance			100.0f
#define AI2cPatrol_ScanAngleFrames			60
#define AI2cPatrol_ScanAngleFramesDelta		40

#define AI2cPatrol_GuardDirFrames			240
#define AI2cPatrol_GuardDirFramesDelta		240
#define AI2cPatrol_GuardAngleFrames			90
#define AI2cPatrol_GuardAngleFramesDelta	60
#define AI2cPatrol_GuardAngleRange			(45.0f * M3cDegToRad)

// ------------------------------------------------------------------------------------
// -- internal function prototypes

// execute a waypoint command
static void AI2iPatrol_DoWaypoint(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState,
									UUtUns32 inWaypointIndex, UUtBool inFirstTime, UUtBool inFailureRetry);

// is a waypoint done?
static UUtBool AI2iPatrol_CurrentWaypointFulfilled(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState);

// go to the next waypoint
UUtBool AI2iPatrol_AdvanceWaypoint(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState);

// is a waypoint a movement waypoint?
static UUtBool AI2iPatrol_WaypointIsMovement(AI2tWaypoint *waypoint, UUtInt16 *outFlagNum);

// find the nearest waypoint to start at
static void AI2iPatrol_FindNearestWaypoint(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState);

// ensure that our targeting has the correct weapon parameters
static void AI2iPatrol_CheckTargetingWeapon(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState);

// set up to start or stop shooting
static void AI2iPatrol_Shooting_Setup(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState,
										M3tPoint3D *inTarget, float inInaccuracy);
static void AI2iPatrol_Shooting_Stop(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState);

// fire our weapon, or stop firing
static void AI2iPatrol_Trigger(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState, UUtBool inFire);

// ------------------------------------------------------------------------------------
// -- update and state change functions

void AI2rPatrol_Reset(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState)
{
	ioPatrolState->pause_timer = 0;
	ioPatrolState->current_waypoint = 0;
	ioPatrolState->current_movement_waypoint = (UUtUns32) -1;
	ioPatrolState->scanning = UUcFalse;
	ioPatrolState->guarding = UUcFalse;
	ioPatrolState->started_guarding = UUcFalse;
	ioPatrolState->scanguard_timer = 0;
	ioPatrolState->running_script = NULL;
	ioPatrolState->locked_in = UUcFalse;
	ioPatrolState->pathfinding_failed = UUcFalse;
	ioPatrolState->targeting_setup = UUcFalse;
	ioPatrolState->run_targeting = UUcFalse;
	ioPatrolState->targeting_timer = 0;
}

void AI2rPatrol_Enter(ONtCharacter *ioCharacter)
{
	AI2tPatrolState *patrol_state;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Patrol);
	patrol_state = &ioCharacter->ai2State.currentState->state.patrol;

	patrol_state->done_glance_waypoint = (UUtUns32) -1;

	// set up our knowledge notify function so that we are alerted by stuff
	if (ioCharacter->ai2State.flags & AI2cFlag_NonCombatant) {
		ioCharacter->ai2State.knowledgeState.callback = &AI2rNonCombatant_NotifyKnowledge;
	} else {
		ioCharacter->ai2State.knowledgeState.callback = &AI2rAlert_NotifyKnowledge;
	}

	if (ioCharacter->ai2State.currentState->begun) {
		// we have already started our patrol at some previous time.
		// don't start from the beginning, use the current point of the path
		patrol_state->running_script = NULL;
		if ((patrol_state->current_waypoint < 0) ||
			(patrol_state->current_waypoint >= patrol_state->path.num_waypoints)) {
			// this is an invalid current waypoint!
			AI2_ERROR(AI2cError, AI2cSubsystem_Patrol, AI2cError_Patrol_InvalidEntry, ioCharacter,
				      patrol_state->current_waypoint, patrol_state->path.num_waypoints, patrol_state, 0);

			AI2rPatrol_Reset(ioCharacter, patrol_state);
		}

		if (patrol_state->path.flags & AI2cPatrolPathFlag_ReturnToNearest) {
			AI2iPatrol_FindNearestWaypoint(ioCharacter, patrol_state);
		}

	} else {
		// start the character in its patrol
		AI2rPatrol_Reset(ioCharacter, patrol_state);
		patrol_state->return_to_prev_point = UUcFalse;
		AI2_ERROR(AI2cStatus, AI2cSubsystem_Patrol, AI2cError_Patrol_BeginPatrol, ioCharacter,
			      patrol_state, 0, 0, 0);
	}

	// we aren't yet on our path
	patrol_state->on_path = UUcFalse;

	AI2iPatrol_DoWaypoint(ioCharacter, patrol_state, patrol_state->current_waypoint, UUcTrue, UUcFalse);
}

static void AI2iPatrol_AbortScript(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState)
{
	UUtError error;

	if (ioPatrolState->running_script == NULL)
		return;

	// this should access our reference pointer and null it out
	error = SLrScheduler_Remove(ioPatrolState->running_script);
	if (error != UUcError_None) {
		// manually remove our reference pointer
		SLrScheduler_ForceDelete(ioPatrolState->running_script);
	}
}

// exit a patrol state - called when the character goes out of "patrol mode".
void AI2rPatrol_Exit(ONtCharacter *ioCharacter)
{
	AI2tPatrolState *patrol_state;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Patrol);
	patrol_state = &ioCharacter->ai2State.currentState->state.patrol;

	// stop any current shooting
	patrol_state->targeting_timer = 0;
	if (patrol_state->run_targeting) {
		AI2iPatrol_Shooting_Stop(ioCharacter, patrol_state);
	}

	// stop any current movement
	patrol_state->pause_timer = 0;
	AI2rPath_Halt(ioCharacter);
	AI2rMovement_StopGlancing(ioCharacter);
	AI2rMovement_StopAiming(ioCharacter);
	AI2rMovement_FreeFacing(ioCharacter);

	// record where we were when we stopped patrolling
	patrol_state->return_to_prev_point = UUcTrue;
	ONrCharacter_GetPathfindingLocation(ioCharacter, &patrol_state->prev_point);

	// if we were running a script as part of our patrol, abort it
	AI2iPatrol_AbortScript(ioCharacter, patrol_state);

	// if we were running a film as part of our patrol, stop it
	ONrFilm_Stop(ioCharacter);
}

// perform death shutdown - called when the character dies while in "patrol mode".
void AI2rPatrol_NotifyDeath(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Patrol);
	AI2iPatrol_AbortScript(ioCharacter, &ioCharacter->ai2State.currentState->state.patrol);
}

// pause a patrol for some number of frames
void AI2rPatrol_Pause(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState,
					  UUtUns32 inPauseFrames)
{
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Patrol);
	ioPatrolState->pause_timer = UUmMax(ioPatrolState->pause_timer, inPauseFrames);
}

UUtBool AI2iPatrol_AdvanceWaypoint(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState)
{
	ioPatrolState->current_waypoint++;
	ioPatrolState->pathfinding_fail_count = 0;
	ioPatrolState->done_glance_waypoint = (UUtUns32) -1;

	// have we hit the end of our path?
	if (ioPatrolState->current_waypoint >= ioPatrolState->path.num_waypoints) {
		AI2rPatrol_Finish(ioCharacter, ioPatrolState);
		return UUcTrue;
	} else {
		AI2iPatrol_DoWaypoint(ioCharacter, ioPatrolState, ioPatrolState->current_waypoint, UUcTrue, UUcFalse);
		if (ioCharacter->ai2State.currentGoal != AI2cGoal_Patrol) {
			return UUcTrue;
		}
		return UUcFalse;
	}
}

void AI2rPatrol_Update(ONtCharacter *ioCharacter)
{
	AI2tPatrolState *patrol_state;
	UUtBool fail_waypoint;
	float move_dir;
	UUtUns32 start_waypoint;

	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Patrol);
	UUmAssert(ioCharacter->ai2State.currentState->begun);
	patrol_state = &ioCharacter->ai2State.currentState->state.patrol;

	UUmAssert(patrol_state->path.num_waypoints > 0);

	if (!patrol_state->locked_in) {
		// non-active modes decay slowly in alert status
		AI2rAlert_Decay(ioCharacter);
	}

	if (patrol_state->running_script != NULL) {
		if (SLrScheduler_Executing(patrol_state->running_script)) {
			// we must wait for the script to finish
			return;
		} else {
			// there has been a scripting-system error and our context
			// has not been deleted, even though it is not running
			SLrScheduler_ForceDelete(patrol_state->running_script);
		}
	}

	if (patrol_state->pause_timer) {
		patrol_state->pause_timer--;

		if (patrol_state->pause_timer) {
			// pause for a while
			AI2rPath_Halt(ioCharacter);
			return;
		} else {
			// continue patrolling
			AI2iPatrol_DoWaypoint(ioCharacter, patrol_state, patrol_state->current_waypoint, UUcFalse, UUcFalse);
		}
	}

	if (patrol_state->targeting_timer) {
		UUmAssert(patrol_state->run_targeting);
		patrol_state->targeting_timer--;

		if (patrol_state->targeting_timer == 0) {
			AI2iPatrol_Shooting_Stop(ioCharacter, patrol_state);
		}
	}

	if (patrol_state->scanguard_timer > 0) {
		UUmAssert((patrol_state->scanning) || (patrol_state->guarding));
		patrol_state->scanguard_timer--;

		if (patrol_state->scanguard_timer == 0) {
			if (patrol_state->scanning) {
				// stop scanning
				patrol_state->scanning = UUcFalse;
				AI2rMovement_StopAiming(ioCharacter);

			} else if (patrol_state->guarding) {
				// stop guarding
				patrol_state->guarding = UUcFalse;
				AI2rMovement_StopAiming(ioCharacter);
			}

			if (AI2iPatrol_AdvanceWaypoint(ioCharacter, patrol_state)) {
				// finished our patrol.
				return;
			}
		}
	}

	fail_waypoint = UUcFalse;
	if (patrol_state->pathfinding_failed) {
		// we had a pathfinding error last frame
		patrol_state->pathfinding_failed = UUcFalse;
		patrol_state->pathfinding_fail_count++;
		if (patrol_state->pathfinding_fail_count >= AI2cPatrol_FailThreshold) {
			// give up on this waypoint
			AI2_ERROR(AI2cError, AI2cSubsystem_Patrol, AI2cError_Patrol_FailedWaypoint, ioCharacter,
					&patrol_state->path, patrol_state->current_waypoint, patrol_state->path.num_waypoints, AI2cPatrol_FailThreshold);
			fail_waypoint = UUcTrue;
		} else if (patrol_state->current_movement_waypoint != (UUtUns32) -1) {
			// try again to reach the current waypoint that we were moving to
			AI2iPatrol_DoWaypoint(ioCharacter, patrol_state, patrol_state->current_movement_waypoint, UUcTrue, UUcTrue);
		}
	}

	start_waypoint = patrol_state->current_waypoint;
	while (fail_waypoint || AI2iPatrol_CurrentWaypointFulfilled(ioCharacter, patrol_state)) {
		fail_waypoint = UUcFalse;
		AI2iPatrol_AdvanceWaypoint(ioCharacter, patrol_state);
		if (ioCharacter->ai2State.currentGoal != AI2cGoal_Patrol) {
			// we have transitioned to another goal, abort the patrol update
			return;
		}

		if (patrol_state->current_waypoint == start_waypoint) {
			COrConsole_Printf("### %s: patrol %s is looping endlessly, aborting", ioCharacter->player_name, patrol_state->path.name);
			AI2rPatrol_Finish(ioCharacter, patrol_state);
			return;
		}
	}

	if (!ioCharacter->pathState.at_destination) {
		if (ONrCharacter_IsFallen(ioCharacter)) {
			// we need to get up from lying on the ground, we have been knocked down.
			AI2rExecutor_MoveOverride(ioCharacter, LIc_BitMask_Forward);
		}
	}

	if (patrol_state->scanning) {
		move_dir = AI2rMovement_GetMoveDirection(ioCharacter);

		if ((move_dir != AI2cAngle_None) && ((float)fabs(move_dir - patrol_state->scan_state.dir_state.cur_direction_angle) > AI2cPatrol_RecalcDirectionAngle)) {
			// update the direction to match our current direction
			patrol_state->scan_state.dir_state.has_cur_direction = UUcTrue;
			patrol_state->scan_state.dir_state.cur_direction_isexit = UUcFalse;
			MUmVector_Set(patrol_state->scan_state.dir_state.cur_direction,
							AI2cPatrol_ScanLookDistance * MUrSin(move_dir), 0, AI2cPatrol_ScanLookDistance * MUrCos(move_dir));
			patrol_state->scan_state.dir_state.cur_direction_angle = move_dir;

			// recalc the scan angle and look along this vector
			AI2rGuard_RecalcScanDir(ioCharacter, &patrol_state->scan_state);
			AI2rMovement_LookInDirection(ioCharacter, &patrol_state->scan_state.facing_vector);
		}

		// scan the area
		AI2rGuard_UpdateScan(ioCharacter, &patrol_state->scan_state);

	} else if (patrol_state->guarding) {
		// scan the area
		AI2rGuard_UpdateScan(ioCharacter, &patrol_state->scan_state);
	}

	if (patrol_state->run_targeting) {
		// fire at our target
		AI2iPatrol_CheckTargetingWeapon(ioCharacter, patrol_state);
		AI2rTargeting_Update(&patrol_state->targeting, UUcTrue, UUcTrue, UUcTrue, NULL);
		AI2rTargeting_Execute(&patrol_state->targeting, UUcTrue, UUcTrue, UUcTrue);
	}
}

// finish a patrol
void AI2rPatrol_Finish(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState)
{
	// if we're at the end of our patrol, reset back to the first waypoint
	if (ioPatrolState->current_waypoint >= ioPatrolState->path.num_waypoints)
		ioPatrolState->current_waypoint = 0;

	if (ioCharacter->ai2State.currentState == &ioCharacter->ai2State.jobState) {
		// this was our job, transition to an idle state
		AI2rExitState(ioCharacter);
		ioCharacter->ai2State.currentGoal = AI2cGoal_Idle;
		ioCharacter->ai2State.currentState = &ioCharacter->ai2State.currentStateBuffer;
		ioCharacter->ai2State.currentState->begun = UUcFalse;
		AI2rEnterState(ioCharacter);
	} else {
		// return to our job
		AI2rReturnToJob(ioCharacter, UUcTrue, UUcFalse);
	}
}

UUtBool AI2rPatrol_AtJobLocation(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState)
{
	if (ioPatrolState->return_to_prev_point) {
		// we aren't at our patrol path yet
		return UUcFalse;
	}

	return ioPatrolState->on_path;
}

static void AI2iPatrol_FindNearestWaypoint(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState)
{
	UUtUns32 itr, closest;
	UUtInt16 flag_id;
	UUtError error;
	UUtBool found;
	ONtFlag flag;
	AI2tWaypoint *waypoint;
	M3tVector3D current_pt;
	float closest_distance, current_distance;

	closest = (UUtUns32) -1;
	closest_distance = 1e09;
	ONrCharacter_GetPathfindingLocation(ioCharacter, &current_pt);

	// search for movement waypoints that we can get to, and pick the closest one
	for (itr = 0, waypoint = ioPatrolState->path.waypoints; itr < ioPatrolState->path.num_waypoints; itr++, waypoint++) {
		if (!AI2iPatrol_WaypointIsMovement(waypoint, &flag_id))
			continue;

		found = ONrLevel_Flag_ID_To_Flag(flag_id, &flag);
		if (!found)
			continue;

		// how far is it to this flag?
		current_distance = 0;
		error = PHrPathDistance(ioCharacter, (UUtUns32) -1, &current_pt, &flag.location,
								ioCharacter->currentPathnode, NULL, &current_distance);
		if (error != UUcError_None)
			continue;

		if (current_distance < closest_distance) {
			closest_distance = current_distance;
			closest = itr;
		}
	}

	if (closest != (UUtUns32) -1) {
		// we've found a waypoint to return to
		ioPatrolState->current_waypoint = closest;
		ioPatrolState->return_to_prev_point = UUcFalse;
	}
}

// handle pathfinding errors
UUtBool AI2rPatrol_PathfindingHandler(ONtCharacter *inCharacter, UUtUns32 inErrorID,
										UUtUns32 inParam1, UUtUns32 inParam2,
										UUtUns32 inParam3, UUtUns32 inParam4)
{
	// note that we had a pathfinding error - will be handled in update loop
	UUmAssert(inCharacter->ai2State.currentGoal == AI2cGoal_Patrol);
	inCharacter->ai2State.currentState->state.patrol.pathfinding_failed = UUcTrue;

	// error is NOT handled - so it still gets reported
	return UUcFalse;
}

// ------------------------------------------------------------------------------------
// -- scripting interface functions

// find a patrol point in the level
UUtError AI2rPatrol_FindByName(const char *inPatrolName, AI2tPatrolPath *outPath)
{
	OBJtOSD_All search_param;
	OBJtObject *found_object;
	UUtError error;

	UUrString_Copy(search_param.osd.patrolpath_osd.name, inPatrolName, SLcScript_MaxNameLength);
	found_object = OBJrObjectType_Search(OBJcType_PatrolPath, OBJcSearch_PatrolPathName, &search_param);

	if (found_object != NULL) {
		if (outPath == NULL) {
			// this was just a check to see that it existed, and it does
			return UUcError_None;
		} else {
			error = OBJrPatrolPath_ReadPathData(found_object, outPath);
			return error;
		}
	} else {
		return UUcError_Generic;
	}
}

// ------------------------------------------------------------------------------------
// -- shooting functions

// ensure that our targeting has the correct weapon parameters
static void AI2iPatrol_CheckTargetingWeapon(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState)
{
	AI2tWeaponParameters *weapon_params;
	M3tMatrix4x3 *weapon_matrix;
	WPtWeaponClass *weapon_class;

	if (ioPatrolState->current_weapon != ioCharacter->inventory.weapons[0]) {
		// get the parameters for this new weapon
		if (ioCharacter->inventory.weapons[0] == NULL) {
			weapon_params = NULL;
			weapon_matrix = NULL;
		} else {
			weapon_class = WPrGetClass(ioCharacter->inventory.weapons[0]);
			weapon_params = &weapon_class->ai_parameters;
			weapon_matrix = ONrCharacter_GetMatrix(ioCharacter, ONcWeapon_Index);
		}

		AI2rTargeting_ChangeWeapon(&ioPatrolState->targeting, weapon_params, weapon_matrix);
		ioPatrolState->current_weapon = ioCharacter->inventory.weapons[0];
	}
}

static void AI2iPatrol_Shooting_Setup(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState,
										M3tPoint3D *inTarget, float inInaccuracy)
{
	// characters must be active in order to be shooting
	ONrForceActiveCharacter(ioCharacter);

	if (!ioPatrolState->targeting_setup) {
		AI2tTargetingOwner targeting_owner;

		// set up our targeting
		targeting_owner.type = AI2cTargetingOwnerType_Character;
		targeting_owner.owner.character = ioCharacter;
		AI2rTargeting_Initialize(targeting_owner, &ioPatrolState->targeting, &AI2gPatrolAI_TargetingCallbacks, NULL, NULL,
				&ioCharacter->characterClass->ai2_behavior.combat_parameters.targeting_params,
				ioCharacter->characterClass->ai2_behavior.combat_parameters.shooting_skill);
		ioPatrolState->targeting_setup = UUcTrue;
	}

	// set up our current weapon state
	ioPatrolState->current_weapon = NULL;
	AI2iPatrol_CheckTargetingWeapon(ioCharacter, ioPatrolState);

	// set up our targeting parameters
	ioPatrolState->targeting.target_pt = *inTarget;
	ioPatrolState->targeting.valid_target_pt = UUcTrue;
	ioPatrolState->targeting_inaccuracy = inInaccuracy;
	ioPatrolState->trigger_pressed = UUcFalse;

	ioPatrolState->run_targeting = UUcTrue;
}

static void AI2iPatrol_Shooting_Stop(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState)
{
	// stop our targeting
	AI2iPatrol_Trigger(ioCharacter, ioPatrolState, UUcFalse);
	ioPatrolState->run_targeting = UUcFalse;

	if (ioPatrolState->targeting_setup) {
		AI2rTargeting_Terminate(&ioPatrolState->targeting);
		ioPatrolState->targeting_setup = UUcFalse;
	}
}

static void AI2iPatrol_Trigger(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState, UUtBool inTrigger)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	// we should never be made inactive while patrolling
	UUmAssertReadPtr(active_character, sizeof(ONtActiveCharacter));
	if (active_character == NULL) {
		// fail safe
		return;
	}

	if (ONrCharacter_IsVictimAnimation(ioCharacter) || ONrCharacter_IsPickingUp(ioCharacter)) {
		// don't fire our weapon in these cases
		inTrigger = UUcFalse;
	}

	if (ioCharacter->inventory.weapons[0] == NULL) {
		inTrigger = UUcFalse;
	}

	if (inTrigger) {
		WPtWeaponClass *weapon_class = WPrGetClass(ioCharacter->inventory.weapons[0]);

		if (weapon_class->flags & WPcWeaponClassFlag_AISingleShot) {
			if (WPrMustWaitToFire(ioCharacter->inventory.weapons[0])) {
				// don't press trigger while we wait for chamber time
				inTrigger = UUcFalse;
			}
		}
	}

	if (inTrigger) {
		ioPatrolState->trigger_pressed = UUcTrue;
		active_character->pleaseFire = UUcTrue;
	} else {
		// release our triggers
		if (ioPatrolState->trigger_pressed) {
			active_character->pleaseReleaseTrigger = UUcTrue;
			ioPatrolState->trigger_pressed = UUcFalse;
		}
		active_character->pleaseFire = UUcFalse;
	}
}

void AI2rPatrolAI_TargetingCallback_Fire(AI2tTargetingState *inTargetingState, UUtBool inFire)
{
	ONtCharacter *character;

	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Character);
	character = inTargetingState->owner.owner.character;

	UUmAssert(character->charType == ONcChar_AI2);
	UUmAssert(character->ai2State.currentGoal == AI2cGoal_Patrol);

	AI2iPatrol_Trigger(character, &character->ai2State.currentState->state.patrol, inFire);
}

// ------------------------------------------------------------------------------------
// -- waypoint functions

// is a waypoint a movement one?
static UUtBool AI2iPatrol_WaypointIsMovement(AI2tWaypoint *waypoint, UUtInt16 *outFlagNum)
{
	switch(waypoint->type)
	{
		case AI2cWaypointType_LookInDirection:
		case AI2cWaypointType_Stop:
		case AI2cWaypointType_Pause:
		case AI2cWaypointType_LookAtPoint:
		case AI2cWaypointType_Loop:
		case AI2cWaypointType_LoopToWaypoint:
		case AI2cWaypointType_MovementMode:
		case AI2cWaypointType_StopLooking:
		case AI2cWaypointType_FreeFacing:
		case AI2cWaypointType_Scan:
		case AI2cWaypointType_GlanceAtFlag:
		case AI2cWaypointType_StopScanning:
		case AI2cWaypointType_PlayScript:
		case AI2cWaypointType_TriggerScript:
		case AI2cWaypointType_LockIntoPatrol:
		case AI2cWaypointType_ShootAtFlag:
			return UUcFalse;

		case AI2cWaypointType_MoveToFlag:
			if (outFlagNum) {
				*outFlagNum = waypoint->data.moveToFlag.flag;
			}
			return UUcTrue;

		case AI2cWaypointType_LookAtFlag:
			if (outFlagNum) {
				*outFlagNum = waypoint->data.lookAtFlag.lookFlag;
			}
			return UUcTrue;

		case AI2cWaypointType_MoveAndFaceFlag:
			if (outFlagNum) {
				*outFlagNum = waypoint->data.moveAndFaceFlag.faceFlag;
			}
			return UUcTrue;

		case AI2cWaypointType_MoveThroughFlag:
			if (outFlagNum) {
				*outFlagNum = waypoint->data.moveToFlag.flag;
			}
			return UUcTrue;

		case AI2cWaypointType_Guard:
			if (outFlagNum) {
				*outFlagNum = waypoint->data.guard.flag;
			}
			return UUcTrue;

		case AI2cWaypointType_MoveNearFlag:
			if (outFlagNum) {
				*outFlagNum = waypoint->data.moveNearFlag.flag;
			}
			return UUcTrue;

		default:
			UUmAssert(!"AI2iPatrol_WaypointIsMovement: unknown waypoint type");
			return UUcFalse;
	}
}

// execute a waypoint command
static void AI2iPatrol_DoWaypoint(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState,
									UUtUns32 inWaypointIndex, UUtBool inFirstTime, UUtBool inFailureRetry)
{
	UUtBool waypoint_specifies_path = UUcFalse, found;
	AI2tWaypoint *waypoint;
	SLtContext **contextptr = NULL;
	ONtFlag flag;
	UUtUns32 new_waypoint_index;
	UUtInt16 script_number;
	char scriptname[32];
	SLtParameter_Actual script_param;

	if (!inFailureRetry) {
		// we are moving to a new waypoint, reset our pathfinding-failure variables
		ioPatrolState->pathfinding_failed = UUcFalse;
		ioPatrolState->pathfinding_fail_count = 0;
	}

	if (ioPatrolState->return_to_prev_point) {
		// return to roughly where we were when we last stopped patrolling.
		AI2rPath_GoToPoint(ioCharacter, &ioPatrolState->prev_point, AI2cPatrol_ReturnToPathDistance, UUcFalse, AI2cAngle_None, NULL);
		return;
	}

	UUmAssert((inWaypointIndex >= 0) && (inWaypointIndex < ioPatrolState->path.num_waypoints));
	waypoint = &ioPatrolState->path.waypoints[inWaypointIndex];

	AI2_ERROR(AI2cStatus, AI2cSubsystem_Patrol, AI2cError_Patrol_AtWaypoint, ioCharacter,
			inWaypointIndex, waypoint, ioPatrolState, 0);

	if (AI2iPatrol_WaypointIsMovement(waypoint, NULL)) {
		// store the last movement waypoint that we hit
		ioPatrolState->current_movement_waypoint = inWaypointIndex;
	}

	switch(waypoint->type)
	{
		case AI2cWaypointType_LookInDirection:
			AI2rMovement_LockFacing(ioCharacter, (AI2tMovementDirection)waypoint->data.lookInDirection.facing);
		break;

		case AI2cWaypointType_MoveToFlag:
			found = ONrLevel_Flag_ID_To_Flag(waypoint->data.moveToFlag.flag, &flag);

			if (!found) {
				AI2_ERROR(AI2cError, AI2cSubsystem_Patrol, AI2cError_Patrol_NoSuchFlag, ioCharacter,
					      waypoint->data.moveToFlag.flag, inWaypointIndex, ioPatrolState, 0);
			} else {
				AI2rPath_GoToPoint(ioCharacter, &flag.location, 0, UUcTrue, AI2cAngle_None, NULL);
			}
		break;

		case AI2cWaypointType_Stop:
			AI2rPatrol_Finish(ioCharacter, ioPatrolState);
		break;

		case AI2cWaypointType_Pause:
			// don't pause if we're just coming out of a pause state
			if (inFirstTime) {
				ioPatrolState->pause_timer = waypoint->data.pause.count;
				AI2rPath_Halt(ioCharacter);
			}
		break;

		case AI2cWaypointType_LookAtFlag:
			found = ONrLevel_Flag_ID_To_Flag(waypoint->data.lookAtFlag.lookFlag, &flag);

			if (!found) {
				AI2_ERROR(AI2cError, AI2cSubsystem_Patrol, AI2cError_Patrol_NoSuchFlag, ioCharacter,
					      waypoint->data.lookAtFlag.lookFlag, inWaypointIndex, ioPatrolState, 0);
			} else {
				AI2rMovement_LookAtPoint(ioCharacter, &flag.location);
			}
		break;

		case AI2cWaypointType_LookAtPoint:
			AI2rMovement_LookAtPoint(ioCharacter, &waypoint->data.lookAtPoint.lookPoint);
		break;

		case AI2cWaypointType_MoveAndFaceFlag:
			found = ONrLevel_Flag_ID_To_Flag(waypoint->data.moveAndFaceFlag.faceFlag, &flag);

			if (!found) {
				AI2_ERROR(AI2cError, AI2cSubsystem_Patrol, AI2cError_Patrol_NoSuchFlag, ioCharacter,
					      waypoint->data.moveAndFaceFlag.faceFlag, inWaypointIndex,
						  ioPatrolState, 0);
			} else {
				AI2rPath_GoToPoint(ioCharacter, &flag.location, 0, UUcTrue, flag.rotation, NULL);
			}
		break;

		case AI2cWaypointType_Loop:
			ioPatrolState->current_waypoint = 0;
			AI2iPatrol_DoWaypoint(ioCharacter, ioPatrolState, ioPatrolState->current_waypoint, UUcTrue, UUcFalse);
		break;

		case AI2cWaypointType_LoopToWaypoint:
			new_waypoint_index = waypoint->data.loopToWaypoint.waypoint_index;
			new_waypoint_index = UUmPin(new_waypoint_index, 0, ioPatrolState->path.num_waypoints - 1);
			if (new_waypoint_index == ioPatrolState->current_waypoint) {
				new_waypoint_index = (ioPatrolState->current_waypoint + 1) % ioPatrolState->path.num_waypoints;
			}
			if (new_waypoint_index != ioPatrolState->current_waypoint) {
				// snap to the new waypoint
				ioPatrolState->current_waypoint = new_waypoint_index;
				AI2iPatrol_DoWaypoint(ioCharacter, ioPatrolState, ioPatrolState->current_waypoint, UUcTrue, UUcFalse);
			}
		break;

		case AI2cWaypointType_MovementMode:
			AI2rMovement_ChangeMovementMode(ioCharacter, (AI2tMovementMode)waypoint->data.movementMode.newMode);
		break;

		case AI2cWaypointType_MoveToPoint:
			AI2rPath_GoToPoint(ioCharacter, &waypoint->data.moveToPoint.point, 0, UUcTrue, AI2cAngle_None, NULL);
			ioPatrolState->current_movement_waypoint = inWaypointIndex;
		break;

		case AI2cWaypointType_MoveThroughFlag:
			found = ONrLevel_Flag_ID_To_Flag(waypoint->data.moveToFlag.flag, &flag);

			if (!found) {
				AI2_ERROR(AI2cError, AI2cSubsystem_Patrol, AI2cError_Patrol_NoSuchFlag, ioCharacter,
					      waypoint->data.moveToFlag.flag, inWaypointIndex,
						  ioPatrolState, 0);
			} else {
				AI2rPath_GoToPoint(ioCharacter, &flag.location, waypoint->data.moveThroughFlag.required_distance, UUcFalse, AI2cAngle_None, NULL);
			}
		break;

		case AI2cWaypointType_MoveThroughPoint:
			AI2rPath_GoToPoint(ioCharacter, &waypoint->data.moveThroughPoint.point,
							waypoint->data.moveThroughPoint.required_distance, UUcFalse, AI2cAngle_None, NULL);
			ioPatrolState->current_movement_waypoint = inWaypointIndex;
		break;

		case AI2cWaypointType_StopLooking:
			AI2rMovement_StopAiming(ioCharacter);
		break;

		case AI2cWaypointType_FreeFacing:
			AI2rMovement_FreeFacing(ioCharacter);
		break;

		case AI2cWaypointType_Scan:
		case AI2cWaypointType_GlanceAtFlag:
			// these are taken care of in CurrentWaypointFulfilled!
		break;

		case AI2cWaypointType_StopScanning:
			if (ioPatrolState->scanning) {
				ioPatrolState->scanning = UUcFalse;
				ioPatrolState->scanguard_timer = 0;
				AI2rMovement_StopAiming(ioCharacter);
			}
		break;

		case AI2cWaypointType_Guard:
			found = ONrLevel_Flag_ID_To_Flag(waypoint->data.guard.flag, &flag);

			if (!found) {
				AI2_ERROR(AI2cError, AI2cSubsystem_Patrol, AI2cError_Patrol_NoSuchFlag, ioCharacter,
					      waypoint->data.guard.flag, inWaypointIndex, ioPatrolState, 0);
			} else {
				AI2rPath_GoToPoint(ioCharacter, &flag.location, 0, UUcTrue, flag.rotation, NULL);
			}

			// we have not yet started our guarding scan (we will when we reach the flag)
			ioPatrolState->started_guarding = UUcFalse;
			ioPatrolState->guarding = UUcFalse;
			ioPatrolState->scanguard_timer = 0;
		break;

		case AI2cWaypointType_MoveNearFlag:
			found = ONrLevel_Flag_ID_To_Flag(waypoint->data.moveNearFlag.flag, &flag);

			if (!found) {
				AI2_ERROR(AI2cError, AI2cSubsystem_Patrol, AI2cError_Patrol_NoSuchFlag, ioCharacter,
					      waypoint->data.moveNearFlag.flag, inWaypointIndex,
						  ioPatrolState, 0);
			} else {
				AI2rPath_GoToPoint(ioCharacter, &flag.location, waypoint->data.moveNearFlag.required_distance, UUcTrue, AI2cAngle_None, NULL);
			}
		break;

		case AI2cWaypointType_PlayScript:
			// store a pointer to the context so that we can block while it executes
			contextptr = (SLtContext**)&ioPatrolState->running_script;

		case AI2cWaypointType_TriggerScript:
			// get the name of the script to run
			script_number = (waypoint->type == AI2cWaypointType_PlayScript) ? waypoint->data.triggerScript.script_number
																			: waypoint->data.playScript.script_number;
			sprintf(scriptname, "patrolscript%04d", script_number);
			script_param.type = SLcType_String;
			script_param.val.str = ioCharacter->player_name;

			SLrScript_ExecuteOnce(scriptname, 1, &script_param, NULL, contextptr);
		break;

		case AI2cWaypointType_LockIntoPatrol:
			ioPatrolState->locked_in = waypoint->data.lockIntoPatrol.lock;

			if ((!ioPatrolState->locked_in) && (ioCharacter->ai2State.knowledgeState.num_contacts > 0)) {
				// we're releasing the lock - run our knowledge notification function again on our
				// highest-priority contact, so we can work out whether to pursue or go into combat with
				AI2rAlert_NotifyKnowledge(ioCharacter, ioCharacter->ai2State.knowledgeState.contacts, UUcFalse);
				if (ioCharacter->ai2State.currentGoal != AI2cGoal_Patrol) {
					return;
				}
			}
		break;

		case AI2cWaypointType_ShootAtFlag:
			found = ONrLevel_Flag_ID_To_Flag(waypoint->data.shootAtFlag.flag, &flag);

			if (!found) {
				AI2_ERROR(AI2cError, AI2cSubsystem_Patrol, AI2cError_Patrol_NoSuchFlag, ioCharacter,
					      waypoint->data.shootAtFlag.flag, inWaypointIndex,
						  ioPatrolState, 0);
			} else {
				// set up our targeting
				AI2iPatrol_Shooting_Setup(ioCharacter, ioPatrolState, &flag.location, waypoint->data.shootAtFlag.additional_inaccuracy);
				ioPatrolState->targeting_timer = waypoint->data.shootAtFlag.shoot_frames;
			}
		break;

		default:
			UUmAssert(!"AI2iPatrol_DoWaypoint: unknown waypoint type");
		break;
	}
}

// is a waypoint done?
static UUtBool AI2iPatrol_CurrentWaypointFulfilled(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState)
{
	AI2tWaypoint *current_waypoint;
	ONtFlag flag;
	float angle;
	UUtBool found;

	if (ioPatrolState->pause_timer > 0) {
		return UUcFalse;
	}

	if (ioPatrolState->pathfinding_failed) {
		// this will be handled next time through the update loop
		return UUcFalse;
	}

	if (ioPatrolState->return_to_prev_point) {
		// we are returning to where we were when we last stopped patrolling.
		if (ioCharacter->pathState.at_destination) {
			ioPatrolState->return_to_prev_point = UUcFalse;

			// set up our current waypoint
			AI2iPatrol_DoWaypoint(ioCharacter, ioPatrolState, ioPatrolState->current_waypoint, UUcTrue, UUcFalse);

			// fall through to checking to see if this waypoint is fulfilled
		} else {
			return UUcFalse;
		}
	}

	current_waypoint = &ioPatrolState->path.waypoints[ioPatrolState->current_waypoint];

	switch(current_waypoint->type)
	{
		case AI2cWaypointType_MoveAndFaceFlag:
		case AI2cWaypointType_MoveThroughFlag:
		case AI2cWaypointType_MoveThroughPoint:
		case AI2cWaypointType_MoveToFlag:
		case AI2cWaypointType_MoveToPoint:
		case AI2cWaypointType_MoveNearFlag:
			// have we stopped moving?
			if (ioCharacter->pathState.at_destination) {
				ioPatrolState->on_path = UUcTrue;
				return UUcTrue;
			}

			// are we close enough?
			if ((ioCharacter->pathState.destinationType == AI2cDestinationType_Point) &&
				(ioCharacter->pathState.destinationData.point_data.final_facing == AI2cAngle_None)) {
				if (ioCharacter->pathState.distance_to_destination_squared < UUmSQR(ioCharacter->pathState.destinationData.point_data.required_distance)) {
					ioPatrolState->on_path = UUcTrue;
					return UUcTrue;
				}
			}

			return UUcFalse;

		case AI2cWaypointType_GlanceAtFlag:
		case AI2cWaypointType_Scan:
			// these moves are special cases because they can't execute while there is
			// another glance or scan going
			if (ioPatrolState->done_glance_waypoint == ioPatrolState->current_waypoint) {
				// if we are stopped, wait until we are done glancing / scanning before we move on
				if (ioPatrolState->stationary_glance) {
					return ((ioCharacter->movementOrders.glance_timer == 0) && (!ioPatrolState->scanning));
				} else {
					// we've started executing the glance, good enough - we will glance while moving
					return UUcTrue;
				}
			}

			if ((ioCharacter->movementOrders.glance_timer > 0) || (ioPatrolState->scanning)) {
				// we can't glance until after our previous glance has finished
				return UUcFalse;
			}

			if (current_waypoint->type == AI2cWaypointType_GlanceAtFlag) {
				// find the flag to glance at
				found = ONrLevel_Flag_ID_To_Flag(current_waypoint->data.glanceAtFlag.glanceFlag, &flag);
				if (!found) {
					// can't do this glance
					AI2_ERROR(AI2cError, AI2cSubsystem_Patrol, AI2cError_Patrol_NoSuchFlag, ioCharacter,
							  current_waypoint->data.glanceAtFlag.glanceFlag, ioPatrolState->current_waypoint,
							  ioPatrolState, 0);
					return UUcTrue;
				}

				// execute the glance
				AI2rMovement_GlanceAtPoint(ioCharacter, &flag.location, current_waypoint->data.glanceAtFlag.glanceFrames, UUcFalse, 0);
			} else {
				// start scanning
				AI2rGuard_ResetScanState(&ioPatrolState->scan_state, ioCharacter->facing);

				ioPatrolState->scanning = UUcTrue;
				ioPatrolState->guarding = UUcFalse;
				ioPatrolState->scanguard_timer = current_waypoint->data.scan.scan_frames;
				ioPatrolState->scan_state.dir_state.face_direction = AI2cAngle_None;		// face along direction of movement
				ioPatrolState->scan_state.direction_minframes = 10000;
				ioPatrolState->scan_state.direction_deltaframes = 0;

				ioPatrolState->scan_state.angle_minframes = AI2cPatrol_ScanAngleFrames;
				ioPatrolState->scan_state.angle_deltaframes = AI2cPatrol_ScanAngleFramesDelta;
				ioPatrolState->scan_state.angle_range = current_waypoint->data.scan.angle * M3cDegToRad;
			}

			// note that we are now busy glancing or scanning
			ioPatrolState->done_glance_waypoint = ioPatrolState->current_waypoint;
			if (ioCharacter->pathState.at_destination && AI2rPath_StopAtDest(ioCharacter, NULL)) {
				// we execute a stationary glance (i.e. wait for the glance or scan to finish before continuing) if
				// we are at our destination, and we are stopped at this destination.
				AI2rPath_Halt(ioCharacter);
				ioPatrolState->stationary_glance = UUcTrue;
				return UUcFalse;
			} else {
				// just queue up the glance, and away we go
				ioPatrolState->stationary_glance = UUcFalse;
				return UUcTrue;
			}

		case AI2cWaypointType_Guard:
			if (!ioCharacter->pathState.at_destination) {
				// wait until we reach our flag and orient correctly
				return UUcFalse;
			}

			ioPatrolState->on_path = UUcTrue;
			if (!ioPatrolState->started_guarding) {
				// set up the scanning state for our guarding
				AI2rGuard_ResetScanState(&ioPatrolState->scan_state, ioCharacter->facing);

				ioPatrolState->scanning = UUcFalse;
				ioPatrolState->guarding = UUcTrue;
				ioPatrolState->started_guarding = UUcTrue;
				ioPatrolState->scanguard_timer = current_waypoint->data.guard.guard_frames;

				ioPatrolState->scan_state.stay_facing = UUcTrue;
				if (ioCharacter->movementOrders.facingMode == AI2cFacingMode_FaceAtDestination) {
					ioPatrolState->scan_state.stay_facing_direction = ioCharacter->movementOrders.facingData.faceAtDestination.angle;
				} else {
					ioPatrolState->scan_state.stay_facing_direction = ioCharacter->facing;
				}
				angle = current_waypoint->data.guard.max_angle * M3cDegToRad;
				ioPatrolState->scan_state.stay_facing_range = UUmPin(angle, 0, M3cPi);
				ioPatrolState->scan_state.direction_minframes = AI2cPatrol_GuardDirFrames;
				ioPatrolState->scan_state.direction_deltaframes = AI2cPatrol_GuardDirFramesDelta;

				ioPatrolState->scan_state.angle_minframes = AI2cPatrol_GuardAngleFrames;
				ioPatrolState->scan_state.angle_deltaframes = AI2cPatrol_GuardAngleFramesDelta;
				ioPatrolState->scan_state.angle_range = AI2cPatrol_GuardAngleRange;
			}

			// exit this waypoint when our timer runs out (if ever) and we stop guarding
			return !ioPatrolState->guarding;

		case AI2cWaypointType_PlayScript:
			// we are done once the script is finished and de-schedules itself and deletes our reference pointer
			return (ioPatrolState->running_script == NULL);

		case AI2cWaypointType_ShootAtFlag:
			// wait for our timer to run out
			return (ioPatrolState->targeting_timer == 0);

		// instantaneous commands that do not block
		case AI2cWaypointType_Pause:
		case AI2cWaypointType_LookInDirection:
		case AI2cWaypointType_MovementMode:
		case AI2cWaypointType_Loop:
		case AI2cWaypointType_LookAtPoint:
		case AI2cWaypointType_LookAtFlag:
		case AI2cWaypointType_StopLooking:
		case AI2cWaypointType_StopScanning:
		case AI2cWaypointType_FreeFacing:
		case AI2cWaypointType_Stop:
		case AI2cWaypointType_TriggerScript:
		case AI2cWaypointType_LockIntoPatrol:
			return UUcTrue;

		default:
			UUmAssert(!"AI2iPatrol_CurrentWaypointFulfilled: unknown waypoint type!");
			return UUcTrue;
	}
}

// ------------------------------------------------------------------------------------
// -- debugging functions

// report in
void AI2rPatrol_Report(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState,
					   UUtBool inVerbose, AI2tReportFunction inFunction)
{
	char reportbuf[256];

	if ((ioPatrolState->current_waypoint < 0) || (ioPatrolState->current_waypoint >= ioPatrolState->path.num_waypoints)) {
		strcpy(reportbuf, "  <error - outside path range>");
	} else {
		AI2rPatrol_DescribeWaypoint(&ioPatrolState->path.waypoints[ioPatrolState->current_waypoint], reportbuf);
	}

	inFunction(reportbuf);
}

// print out a patrol path
void AI2rPatrol_DescribePath(AI2tPatrolPath *inPath, char *outString)
{
	sprintf(outString, "** path: %s, %d waypoints\n", inPath->name, inPath->num_waypoints);
}

void AI2rPatrol_DescribeWaypoint(AI2tWaypoint *inWaypoint, char *outString)
{
	const char *string;

	switch(inWaypoint->type) {
	case AI2cWaypointType_MoveToFlag:
		sprintf(outString, "  movetoflag %d", inWaypoint->data.moveToFlag.flag);
		break;

	case AI2cWaypointType_Stop:
		sprintf(outString, "  stop");
		break;

	case AI2cWaypointType_Pause:
		sprintf(outString, "  pause %d frames", inWaypoint->data.pause.count);
		break;

	case AI2cWaypointType_LookAtFlag:
		sprintf(outString, "  lookatflag %d", inWaypoint->data.lookAtFlag.lookFlag);
		break;

	case AI2cWaypointType_LookAtPoint:
		sprintf(outString, "  lookatpoint %f %f %f",
							inWaypoint->data.lookAtPoint.lookPoint.x,
							inWaypoint->data.lookAtPoint.lookPoint.y,
							inWaypoint->data.lookAtPoint.lookPoint.z);
		break;

	case AI2cWaypointType_MoveAndFaceFlag:
		sprintf(outString, "  moveandfaceflag %d", inWaypoint->data.moveAndFaceFlag.faceFlag);
		break;

	case AI2cWaypointType_Loop:
		sprintf(outString, "  loop");
		break;

	case AI2cWaypointType_MovementMode:
		if ((inWaypoint->data.movementMode.newMode < 0) ||
			(inWaypoint->data.movementMode.newMode >= AI2cMovementMode_Max)) {
			string = "<error>";
		} else {
			string = AI2cMovementModeName[inWaypoint->data.movementMode.newMode];
		}

		sprintf(outString, "  movementmode %s", string);
		break;

	case AI2cWaypointType_MoveToPoint:
		sprintf(outString, "  movetopoint %f %f %f",
							inWaypoint->data.moveToPoint.point.x,
							inWaypoint->data.moveToPoint.point.y,
							inWaypoint->data.moveToPoint.point.z);
		break;

	case AI2cWaypointType_LookInDirection:
		if ((inWaypoint->data.lookInDirection.facing < 0) ||
			(inWaypoint->data.lookInDirection.facing >= AI2cMovementDirection_Max)) {
			string = "<error>";
		} else {
			string = AI2cMovementDirectionName[inWaypoint->data.lookInDirection.facing];
		}

		sprintf(outString, "  lock-facing %s", string);
		break;

	case AI2cWaypointType_FreeFacing:
		sprintf(outString, "  free-facing");
		break;

	case AI2cWaypointType_MoveThroughFlag:
		sprintf(outString, "  movethroughflag %d %f",
				inWaypoint->data.moveThroughFlag.flag, inWaypoint->data.moveThroughFlag.required_distance);
		break;

	case AI2cWaypointType_MoveThroughPoint:
		sprintf(outString, "  movethroughpoint %f %f %f",
							inWaypoint->data.moveThroughPoint.point.x,
							inWaypoint->data.moveThroughPoint.point.y,
							inWaypoint->data.moveThroughPoint.point.z,
							inWaypoint->data.moveThroughPoint.required_distance);
		break;

	case AI2cWaypointType_StopLooking:
		sprintf(outString, "  stoplooking");
		break;

	case AI2cWaypointType_GlanceAtFlag:
		sprintf(outString, "  glance-at-flag %d for %d frames", inWaypoint->data.glanceAtFlag.glanceFlag,
			inWaypoint->data.glanceAtFlag.glanceFrames);
		break;

	case AI2cWaypointType_MoveNearFlag:
		sprintf(outString, "  movenearflag %d %f",
				inWaypoint->data.moveNearFlag.flag, inWaypoint->data.moveNearFlag.required_distance);
		break;

	default:
		sprintf(outString, "  unknown waypoint type %d", inWaypoint->type);
		break;
	}
}

// add a waypoint to a patrol path
//
// NB: because this writes into the OBJtObject that represents that patrol path, this is a PERSISTENT change
UUtError AI2rPatrol_AddWaypoint(OBJtObject *ioObject, UUtUns32 inPosition, AI2tWaypoint *inWaypointData)
{
	AI2tPatrolPath path;
	UUtError error;

	if (ioObject->object_type != OBJcType_PatrolPath) {
		return UUcError_Generic;
	}

	error = OBJrPatrolPath_ReadPathData(ioObject, &path);
	UUmError_ReturnOnError(error);

	// find a valid position to insert at
	if (inPosition < 0) {
		inPosition = 0;
	} else if (inPosition > path.num_waypoints) {
		inPosition = path.num_waypoints;
	}

	if (path.num_waypoints >= AI2cMax_WayPoints) {
		return UUcError_Generic;
	}

	path.num_waypoints++;
	path.waypoints[inPosition] = *inWaypointData;

	error = OBJrPatrolPath_WritePathData(ioObject, &path);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// remove a waypoint from a patrol path
//
// NB: because this writes into the OBJtObject that represents that patrol path, this is a PERSISTENT change
UUtError AI2rPatrol_RemoveWaypoint(OBJtObject *ioObject, UUtUns32 inPosition)
{
	UUtUns32 itr;
	AI2tPatrolPath path;
	UUtError error;

	if (ioObject->object_type != OBJcType_PatrolPath) {
		return UUcError_Generic;
	}

	error = OBJrPatrolPath_ReadPathData(ioObject, &path);
	UUmError_ReturnOnError(error);

	if (path.num_waypoints == 0)
		return UUcError_Generic;

	if (inPosition == UUcMaxUns32) {
		// remove the last waypoint
		inPosition = path.num_waypoints - 1;

	} else if ((inPosition < 0) || (inPosition >= path.num_waypoints)) {
		// if we don't specify a correct position, don't do anything
		return UUcError_Generic;
	}

	// delete, and copy waypoint information down if necessary
	path.num_waypoints--;
	for (itr = path.num_waypoints; itr > inPosition; itr++) {
		path.waypoints[itr - 1] = path.waypoints[itr];
	}

	error = OBJrPatrolPath_WritePathData(ioObject, &path);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// get the currently selected patrol path by the editor
OBJtObject *AI2rDebug_GetSelectedPath(void)
{
	OBJtObject *object;

	object = OBJrSelectedObjects_GetSelectedObject(0);

	if ((object != NULL) && (object->object_type == OBJcType_PatrolPath)) {
		return object;
	} else {
		return NULL;
	}
}

