/*
	FILE:	Oni_AI2_Maneuver.c

	AUTHOR:	Chris Butcher

	CREATED: April 20, 1999

	PURPOSE: Maneuvering code for Oni AI system

	Copyright (c) 2000

*/

#include "bfw_math.h"

#include "Oni_AI2.h"
#include "Oni_Character.h"
#include "Oni_Character_Animation.h"
#include "Oni_Aiming.h"

#define DEBUG_MELEE_ADVANCE			0
#define DEBUG_GUN_PICKUP			0

// ------------------------------------------------------------------------------------
// -- global constants and macros

#define AI2cManeuver_GunPickupRange					(0.75f * ONcPickupRadius)
#define AI2cManeuver_GunPickupPathfindRange			(0.5f * ONcPickupRadius)
#define AI2cManeuver_GunMinRunningPickupRange		5.0f
#define AI2cManeuver_GunMaxRunningPickupRange		70.0f
#define AI2cManeuver_GunRunningPickupOvershootRange	10.0f
#define AI2cManeuver_VerticalPickupRange			5.0f
#define AI2cManeuver_GunPickupDeltaAngle			(20.0f * M3cDegToRad)
#define AI2cManeuver_GunThwartFrames				40
#define AI2cManeuver_GunPossibleThwartFrames		180
#define AI2cManeuver_ExpectedReachGunRange			25.0f
#define AI2cManeuver_GunUnreachableVerticalFraction	0.8f

#define AI2cManeuver_Primary_LocalDistance			15.0f
#define AI2cManeuver_Primary_LocalStopWeight		PHcBorder3
#define AI2cManeuver_Primary_LocalEscapeDistance	3

#define AI2cManeuver_Primary_LocalAdvanceMaxTryDist	50.0f
#define AI2cManeuver_Primary_LocalAdvanceAngle		(60.0f * M3cDegToRad)
#define AI2cManeuver_Primary_LocalAdvanceSteps		4
#define AI2cManeuver_Primary_LocalAdvanceAngleStep	(AI2cManeuver_Primary_LocalAdvanceAngle / AI2cManeuver_Primary_LocalAdvanceSteps)

#define AI2cManeuver_Primary_LocalRetreatAngle		(90.0f * M3cDegToRad)
#define AI2cManeuver_Primary_LocalRetreatSteps		4
#define AI2cManeuver_Primary_LocalRetreatAngleStep	(AI2cManeuver_Primary_LocalRetreatAngle / AI2cManeuver_Primary_LocalRetreatSteps)

#define AI2cManeuver_Advance_AvoidAngle				(45.0f * M3cDegToRad)
#define AI2cManeuver_Advance_StopDist				4.0f
#define AI2cManeuver_Advance_AbsoluteMinDist		6.0f

#define AI2cManeuver_Melee_MaxRangeTechniqueFrames	60

#define AI2cDodge_MaxWeightRadius					10.0f
#define AI2cDodge_MaxWeight							0.8f
#define AI2cDodge_MinWeight							0.3f
#define AI2cDodge_ZeroWeight						-0.3f
#define AI2cDodge_ProjectileActiveTimer				45
#define AI2cDodge_FiringSpreadActiveTimer			20
#define AI2cDodge_DirectHitRadius					10.0f
#define AI2cDodge_LocalPathDistance					20.0f
#define AI2cDodge_LocalPathAngle					(35.0f * M3cDegToRad)
#define AI2cDodge_SpreadMaxWeightAtDist				0.6f
#define AI2cDodge_SpreadRangeZero					1.2f
#define AI2cDodge_MinimumAvoidWeight				0.5f
#define AI2cDodge_WeightMultiplier					3.0f

const float AI2cManeuver_Primary_LocalWeights[PHcMax]
	= {1.0f, 0.85f, 0.7f, 0.6f, 0.3f, 0.0f, 0.0f};
const AStPathPointType AI2cManeuver_Primary_PointTypeByWeight[PHcMax]
	= {AScPathPoint_Clear, AScPathPoint_Clear, AScPathPoint_Danger, AScPathPoint_Tight, AScPathPoint_Danger,
		AScPathPoint_Danger, AScPathPoint_Impassable};
const char *AI2cPrimaryMovementName[AI2cPrimaryMovement_Max]
	= {"hold", "advance", "retreat", "gun", "alarm", "melee", "getup"};

// ------------------------------------------------------------------------------------
// -- internal global variables

static UUtUns32 AI2gManeuver_PathfindingError;
static PHtNode *AI2gManeuver_PathfindingCurrentNode;


// ------------------------------------------------------------------------------------
// -- internal function prototypes

static UUtBool AI2iManeuver_PathfindAndCheckViability(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
										 AI2tPrimaryMovementType inManeuverType, float inWeightToBeat);
static UUtBool AI2iManeuver_PathfindingErrorHandler(ONtCharacter *inCharacter, UUtUns32 inErrorID,
													UUtUns32 inParam1, UUtUns32 inParam2,
													UUtUns32 inParam3, UUtUns32 inParam4);

static UUtBool AI2iManeuver_CheckAdvance(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									  AI2tManeuverState *ioManeuverState, float inWeightToBeat);
static UUtBool AI2iManeuver_CheckRetreat(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									  AI2tManeuverState *ioManeuverState, float inWeightToBeat);
static UUtBool AI2iManeuver_CheckGun(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									  AI2tManeuverState *ioManeuverState, float inWeightToBeat);
static UUtBool AI2iManeuver_CheckMelee(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									  AI2tManeuverState *ioManeuverState, float inWeightToBeat);

// ------------------------------------------------------------------------------------
// -- global functions

// clear our maneuver state
void AI2rManeuverState_Clear(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState)
{
	AI2tManeuverState *maneuver_state = &ioCombatState->maneuver;

	maneuver_state->primary_movement = AI2cPrimaryMovement_None;
	maneuver_state->gun_target = NULL;
	maneuver_state->gun_thwart_timer = 0;
	maneuver_state->gun_possible_thwart_timer = 0;

	maneuver_state->num_dodge_entries = 0;
	UUrMemory_Clear(maneuver_state->dodge_entries, sizeof(maneuver_state->dodge_entries));

	maneuver_state->guns_num_thwarted = 0;
	UUrMemory_Clear(maneuver_state->guns_thwarted, sizeof(maneuver_state->guns_thwarted));
}

// ------------------------------------------------------------------------------------
// -- primary movement control

// clear our primary movement weights
void AI2rManeuver_ClearPrimaryMovement(ONtCharacter *ioCharacter, struct AI2tCombatState *ioCombatState)
{
	// zero our movement weights
	UUrMemory_Clear(ioCombatState->maneuver.primary_movement_weights, AI2cPrimaryMovement_Max * sizeof(float));
	ioCombatState->maneuver.primary_movement_modifier_angle = AI2cAngle_None;
	ioCombatState->maneuver.primary_movement_modifier_weight = 0.0f;
}

// decide our primary movement based on the weight table
void AI2rManeuver_DecidePrimaryMovement(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	UUtBool viable;
	UUtUns32 itr, best_choice, second_choice;
	float highest_weight, second_weight, this_weight;

	// no movement yet... also clears movement modifiers from previous updates
	AI2rPath_Halt(ioCharacter);

	ioCombatState->maneuver.advance_failed = UUcFalse;
	ioCombatState->maneuver.viability_checked_bitmask = 0;
	do {
		// zero the best choice (and the second-best choice)
		best_choice = second_choice = AI2cPrimaryMovement_None;
		highest_weight = second_weight = 0;
		for (itr = 0; itr < AI2cPrimaryMovement_Max; itr++) {
			this_weight = ioCombatState->maneuver.primary_movement_weights[itr];

			if (this_weight > highest_weight) {
				second_weight = highest_weight;
				second_choice = best_choice;

				highest_weight = this_weight;
				best_choice = itr;

			} else if (this_weight > second_weight) {
				second_weight = this_weight;
				second_choice = itr;
			}
		}

		if (best_choice == AI2cPrimaryMovement_None) {
			// there are no positive weights! default movement: not doing anything in particular
			best_choice = AI2cPrimaryMovement_Hold;
			highest_weight = 1.0f;
			ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Hold] = 1.0f;
		}

		// check to see if this preferred option is more viable than the second choice
		viable = AI2iManeuver_PathfindAndCheckViability(ioCharacter, ioCombatState, (AI2tPrimaryMovementType)best_choice, second_weight);
		if (viable) {
			// do it!
			break;
		} else {
			// it is imperative that we actually reduce the weight below the second-best choice, otherwise
			// we can get stuck in an infinite loop here
			UUmAssert((ioCombatState->maneuver.primary_movement_weights[best_choice] < second_weight) || (second_weight == 0));
		}

		// otherwise keep looking
	} while (1);

	ioCombatState->maneuver.primary_movement = (AI2tPrimaryMovementType) best_choice;

	if (ioCombatState->maneuver.primary_movement == AI2cPrimaryMovement_FindAlarm) {
		// enter the alarm state!
		AI2rAlarm_Setup(ioCharacter, ioCombatState->maneuver.found_alarm);
	}
}

static UUtBool AI2iManeuver_PathfindingErrorHandler(ONtCharacter *inCharacter, UUtUns32 inErrorID,
													UUtUns32 inParam1, UUtUns32 inParam2,
													UUtUns32 inParam3, UUtUns32 inParam4)
{
	// AI2_ERROR inside AI2_Movement.c passes in phNodeStart as parameter 3
	AI2gManeuver_PathfindingCurrentNode = (PHtNode *) inParam3;
	if (AI2gManeuver_PathfindingCurrentNode != NULL) {
		UUmAssertReadPtr(AI2gManeuver_PathfindingCurrentNode, sizeof(PHtNode));
	}

	AI2gManeuver_PathfindingError = inErrorID;

	// we handled this error, it doesn't need to be reported
	return UUcTrue;
}

static UUtBool AI2iManeuver_CheckAdvance(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									  AI2tManeuverState *ioManeuverState, float inWeightToBeat)
{
	AI2tPathfindingErrorHandler previous_handler;
	M3tPoint3D from_pt, destination, localmove_point, dest_vector;
	UUtUns8 localmove_weight;
	UUtBool localmove_success, localmove_escape, close_enough, checked_blockage, blocked;
	UUtError error = UUcError_None;
	ONtCharacter *prev_ignorechar;
	float dist_sq, urgency, move_direction, delta_dir;

	ioCombatState->maneuver.advance_failed = UUcTrue;
	checked_blockage = UUcFalse;

	if (!ioCombatState->targeting.valid_target_pt) {
		// we cannot advance, no target point
		AI2rPath_Halt(ioCharacter);
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance] = 0.0f;
		return UUcFalse;
	}

	// get our location and target location
	MUmVector_Copy(destination, ioCombatState->targeting.target_pt);
	ONrCharacter_GetPathfindingLocation(ioCharacter, &from_pt);
	MUmVector_Subtract(dest_vector, destination, from_pt);

	move_direction = MUrATan2(dest_vector.x, dest_vector.z);
	UUmTrig_ClipLow(move_direction);

	if ((ioManeuverState->dodge_max_weight > AI2cDodge_MinimumAvoidWeight) && (ioManeuverState->dodge_avoid_angle != AI2cAngle_None)) {
		// we are trying to avoid hazards at the moment, don't advance into them
		delta_dir = move_direction - ioManeuverState->dodge_avoid_angle;
		UUmTrig_ClipAbsPi(delta_dir);

		if ((float)fabs(delta_dir) < AI2cManeuver_Advance_AvoidAngle) {
			// we can't advance, because we're avoiding a hazard nearby
			ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance] = 0.0f;
			return UUcFalse;
		}
	}

	// urgency decreases as we get closer to the target
	dist_sq = MUmVector_GetLengthSquared(dest_vector);
	if (dist_sq > UUmSQR(ioManeuverState->advance_maxdist)) {
		urgency = 1.0f;

	} else if (dist_sq < UUmSQR(ioManeuverState->advance_mindist)) {
		urgency = 0.0f;

	} else {
		urgency = (MUrSqrt(dist_sq) - ioManeuverState->advance_mindist)
			/ (ioManeuverState->advance_maxdist - ioManeuverState->advance_mindist);
	}

#if DEBUG_MELEE_ADVANCE
	COrConsole_Printf("%d: attempting advance (weight %f), urgency is %f",
			ONrGameState_GetGameTime(), ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance], urgency);
#endif

	if (dist_sq < UUmSQR(AI2cManeuver_Primary_LocalAdvanceMaxTryDist)) {
		// we are close. first try local movement straight towards target.

		// set up pathfindIgnoreChar so that we don't view the target as an obstruction
		// NB: don't trash the previous value of pathfindIgnoreChar!
		prev_ignorechar = ioCharacter->movementOrders.curPathIgnoreChar;
		ioCharacter->movementOrders.curPathIgnoreChar = ioCombatState->target;

		error = AI2rTryLocalMovement(ioCharacter, PHcPathMode_CasualMovement, &from_pt, &destination,
									3, &localmove_success, &localmove_point,
									&localmove_weight, &localmove_escape);

		// restore pathfindIgnoreChar
		ioCharacter->movementOrders.curPathIgnoreChar = prev_ignorechar;
	} else {
		// we're too far away to try local movement initially; use pathfinding instead
		localmove_success = UUcFalse;
	}

	if (localmove_success && localmove_escape) {
		// this is an escape path, we must check to make sure that it's valid
		if (!checked_blockage) {
			blocked = AI2rManeuver_CheckBlockage(ioCharacter, &destination);
		}
		localmove_success = !blocked;
#if DEBUG_MELEE_ADVANCE
		COrConsole_Printf("%d: localmove escape, %s", ONrGameState_GetGameTime(), blocked ? "BLOCKED" : "NOTBLOCKED");
#endif
	}

	if (urgency > 0) {
		close_enough = UUcFalse;

	} else if (dist_sq < UUmSQR(AI2cManeuver_Advance_AbsoluteMinDist)) {
		close_enough = UUcTrue;
#if DEBUG_MELEE_ADVANCE
		COrConsole_Printf("%d: within absolute minimum distance", ONrGameState_GetGameTime());
#endif

	} else if (localmove_success) {
		close_enough = UUcTrue;
#if DEBUG_MELEE_ADVANCE
		COrConsole_Printf("%d: localmove ok", ONrGameState_GetGameTime());
#endif

	} else {
		// the pathfinding grid says that we are blocked, check to see if it's right.
		// if we aren't in fact blocked from the target, then we can stop, we're close
		// enough to the target
		if (!checked_blockage) {
			blocked = AI2rManeuver_CheckBlockage(ioCharacter, &destination);
		}
		close_enough = !blocked;
#if DEBUG_MELEE_ADVANCE
		COrConsole_Printf("%d: localmove fail, %s", ONrGameState_GetGameTime(), blocked ? "BLOCKED" : "NOTBLOCKED");
#endif
	}

	if (close_enough) {
		// don't try moving towards the target any further.
#if DEBUG_MELEE_ADVANCE
		COrConsole_Printf("%d: advance halted", ONrGameState_GetGameTime());
#endif
		AI2rPath_Halt(ioCharacter);
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance] = 0.0f;
		return UUcFalse;
	}

	// we must always have at least some urgency if we're trying to move
	ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance] *= UUmMax(urgency, 0.5f);

	if (ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance] < inWeightToBeat) {
		// we aren't close enough to be interested in trying to advance
#if DEBUG_MELEE_ADVANCE
		COrConsole_Printf("%d: advance does not have sufficient weight (%f < %f)",
						ONrGameState_GetGameTime(), ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance], inWeightToBeat);
#endif
		AI2rPath_Halt(ioCharacter);
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance] = 0.0f;
		return UUcFalse;
	}

	if ((error != UUcError_None) || (!localmove_success) || (localmove_weight >= PHcBorder3)) {
		// can't walk straight there. try to pathfind to them.
		AI2gManeuver_PathfindingError = AI2cError_Pathfinding_NoError;

		previous_handler = AI2rError_TemporaryPathfindingHandler(AI2iManeuver_PathfindingErrorHandler);
		AI2rPath_GoToPoint(ioCharacter, &destination, AI2cManeuver_Advance_StopDist, UUcFalse, AI2cAngle_None, ioCombatState->target);
		AI2rError_RestorePathfindingHandler(previous_handler);

		if (AI2gManeuver_PathfindingError == AI2cError_Pathfinding_NoError) {
			// we aren't interested in using pathfinding here unless we find a non-simple path... simple
			// paths can be better handled by our local movement code.
			if (!AI2rPath_SimplePath(ioCharacter)) {
				// we found a complex path!
#if DEBUG_MELEE_ADVANCE
				COrConsole_Printf("%d: pathfinding success", ONrGameState_GetGameTime());
#endif
				ioCombatState->maneuver.advance_failed = UUcFalse;
				return UUcTrue;
			}
		}

		// no path to target. but can we walk towards the target?
		error = AI2rFindLocalPath(ioCharacter, PHcPathMode_CasualMovement, NULL, NULL, NULL,
						AI2cManeuver_Primary_LocalDistance, move_direction,
						AI2cManeuver_Primary_LocalAdvanceAngleStep, AI2cManeuver_Primary_LocalAdvanceSteps,
						AI2cManeuver_Primary_LocalStopWeight, AI2cManeuver_Primary_LocalEscapeDistance,
						&localmove_success, &localmove_point, &localmove_weight, &localmove_escape);
		if (error != UUcError_None) {
			// unable to do local movement
#if DEBUG_MELEE_ADVANCE
			COrConsole_Printf("%d: pathfinding fail, findlocalpath fail, cannot advance", ONrGameState_GetGameTime());
#endif
			ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance] = 0.0f;
			return UUcFalse;
		}
	}

	// we are not using a complex high-level path, any advance from here on
	// will be handled purely using movement modifiers
	AI2rPath_Halt(ioCharacter);

	if (localmove_success) {
		UUmAssert((localmove_weight >= 0) && (localmove_weight < PHcMax));

		// urgency is modified by the weight of the path that we found
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance] *= AI2cManeuver_Primary_LocalWeights[localmove_weight];

		if (ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance] < inWeightToBeat) {
			// don't do the movement
#if DEBUG_MELEE_ADVANCE
			COrConsole_Printf("%d: localmove advance does not have sufficient weight (%f < %f)",
							ONrGameState_GetGameTime(), ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance], inWeightToBeat);
#endif
			return UUcFalse;
		}

		// work out the direction that this point is in
		MUmVector_Subtract(localmove_point, localmove_point, from_pt);
		move_direction = MUrATan2(localmove_point.x, localmove_point.z);
		UUmTrig_ClipLow(move_direction);

		// move in the direction that we found!
		ioManeuverState->primary_movement_modifier_angle = move_direction;
		ioManeuverState->primary_movement_modifier_weight = ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance];
		ioManeuverState->advance_failed = UUcFalse;
#if DEBUG_MELEE_ADVANCE
		COrConsole_Printf("%d: localmove success", ONrGameState_GetGameTime());
#endif
		return UUcTrue;

	} else {
		// unable to find a clear direction to move in that's near to this direction.
#if DEBUG_MELEE_ADVANCE
		COrConsole_Printf("%d: localmove fail, cannot advance", ONrGameState_GetGameTime());
#endif
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance] = 0.0f;
		return UUcFalse;
	}
}

static UUtBool AI2iManeuver_CheckRetreat(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									  AI2tManeuverState *ioManeuverState, float inWeightToBeat)
{
	M3tPoint3D from_pt, dest_vector, localmove_point;
	UUtUns8 localmove_weight;
	UUtBool localmove_success, localmove_escape;
	UUtError error;
	float distance, urgency, move_direction, weight;

	if (ioCombatState->target == NULL) {
		// don't retreat from corpses
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Retreat] = 0.0f;
		return UUcFalse;
	}

	// can we walk away from the target?
	ONrCharacter_GetPathfindingLocation(ioCharacter, &from_pt);
	MUmVector_Subtract(dest_vector, from_pt, ioCombatState->target->location);
	move_direction = MUrATan2(dest_vector.x, dest_vector.z);

	error = AI2rFindLocalPath(ioCharacter, PHcPathMode_CasualMovement, NULL, NULL, NULL, AI2cManeuver_Primary_LocalDistance, move_direction,
					AI2cManeuver_Primary_LocalRetreatAngleStep, AI2cManeuver_Primary_LocalRetreatSteps,
					AI2cManeuver_Primary_LocalStopWeight, AI2cManeuver_Primary_LocalEscapeDistance,
					&localmove_success, &localmove_point, &localmove_weight, &localmove_escape);
	if (error != UUcError_None) {
		// unable to do local movement
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Retreat] = 0.0f;
		return UUcFalse;
	}

	if (localmove_success) {
		UUmAssert((localmove_weight >= 0) && (localmove_weight < PHcMax));

		// urgency decreases as we get further from the target
		distance = MUmVector_GetLengthSquared(dest_vector);
		if (distance < UUmSQR(ioManeuverState->retreat_mindist)) {
			urgency = 1.0f;

		} else if (distance > UUmSQR(ioManeuverState->retreat_maxdist)) {
			// don't perform this maneuver.
			ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Retreat] = 0.0f;
			return UUcFalse;

		} else {
			distance = MUrSqrt(distance);
			urgency = (distance - ioManeuverState->retreat_maxdist)
				/ (ioManeuverState->retreat_mindist - ioManeuverState->retreat_maxdist);
		}

		// urgency is modified by the weight of the path that we found
		weight = urgency * AI2cManeuver_Primary_LocalWeights[localmove_weight];
		weight *= ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Retreat];

		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Retreat] = weight;

		if (weight < inWeightToBeat) {
			// don't do this movement
			return UUcFalse;
		}

		// work out the direction that this point is in
		MUmVector_Subtract(localmove_point, localmove_point, from_pt);
		move_direction = MUrATan2(localmove_point.x, localmove_point.z);
		UUmTrig_ClipLow(move_direction);

		// move in the direction that we found!
		AI2rPath_Halt(ioCharacter);
		ioManeuverState->primary_movement_modifier_angle = move_direction;
		ioManeuverState->primary_movement_modifier_weight = weight;
		return UUcTrue;
	} else {
		// unable to find a clear direction to move in that's near to this direction.
		AI2rPath_Halt(ioCharacter);
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Retreat] = 0.0f;
		return UUcFalse;
	}
}

static UUtBool AI2iManeuver_TryPathToGun(ONtCharacter *ioCharacter, AI2tManeuverState *ioManeuverState, float inWeightToBeat,
										 M3tPoint3D *inWeaponPosition, float *ioDistance)
{
	UUtError error;
	float urgency, weight;
	M3tPoint3D current_pt;
	AI2tPathfindingErrorHandler previous_handler;

	// how far away is the weapon?
	ONrCharacter_GetPathfindingLocation(ioCharacter, &current_pt);
	error = PHrPathDistance(ioCharacter, (UUtUns32) -1, &current_pt, inWeaponPosition, ioCharacter->currentPathnode, NULL, ioDistance);
	if (error != UUcError_None) {
		// no path or too far away
		return UUcFalse;
	}

	// try to pathfind to the weapon
	AI2gManeuver_PathfindingError = AI2cError_Pathfinding_NoError;
	previous_handler = AI2rError_TemporaryPathfindingHandler(AI2iManeuver_PathfindingErrorHandler);
	AI2rPath_GoToPoint(ioCharacter, inWeaponPosition, AI2cManeuver_GunPickupPathfindRange, UUcTrue, AI2cAngle_None, NULL);
	AI2rError_RestorePathfindingHandler(previous_handler);

	if (AI2gManeuver_PathfindingError != AI2cError_Pathfinding_NoError) {
		// can't pathfind to weapon
		return UUcFalse;
	}

	// urgency of going for weapon is scaled based on the distance away that it currently is
	urgency = (*ioDistance - ioManeuverState->gun_search_minradius)
		/ (ioManeuverState->gun_search_maxradius - ioManeuverState->gun_search_minradius);
	urgency = 1.0f - UUmMax(urgency, 0.0f);

	// update the weight and return
	weight = ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Gun] * urgency;
	ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Gun] = weight;

	if (weight < inWeightToBeat) {
		// don't do this, we have better options available
		AI2rPath_Halt(ioCharacter);
		return UUcFalse;
	} else {
		// this is a valid maneuver!
		return UUcTrue;
	}
}

static UUtBool AI2iManeuver_CheckGun(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									  AI2tManeuverState *ioManeuverState, float inWeightToBeat)
{
	const TRtAnimation *found_animation;
	const WPtWeaponClass *weapon_class;
	M3tPoint3D destination;
	UUtUns32 weapon_iterator, itr;
	WPtWeapon *weapon;
	float distance;
	UUtBool path_to_gun, running_pickup, ignore_gun, thwarted, possibly_thwarted;
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);

	UUmAssertReadPtr(active_character, sizeof(*active_character));

	if (ioManeuverState->gun_target != NULL) {
		UUmAssertReadPtr(ioManeuverState->gun_target, sizeof(UUtUns32));

		// we are already going for a gun. check that it's still available for pickup.
		if (WPrCanBePickedUp(ioManeuverState->gun_target)) {
			if (ONrCharacter_IsFallen(ioCharacter)) {
				// get up - we can't pick up guns while fallen
				AI2rExecutor_MoveOverride(ioCharacter, LIc_BitMask_Forward);
				return UUcTrue;
			}

			// how far away is the gun that we are currently going for?
			WPrGetPickupPosition(ioManeuverState->gun_target, &destination);
			distance = MUmVector_GetDistance(ioCharacter->location, destination);

			running_pickup = ioManeuverState->gun_running_pickup;
			running_pickup = running_pickup && TRrAnimation_TestFlag(active_character->animation, ONcAnimFlag_Can_Pickup);
			running_pickup = running_pickup && !TRrAnimation_TestFlag(active_character->animation, ONcAnimFlag_NoAIPickup);

			if (running_pickup) {
				// we are performing a running pickup, don't change our animation but keep aiming us towards the gun
				AI2rPath_Halt(ioCharacter);
				WPrGetPickupPosition(ioManeuverState->gun_target, &destination);
				AI2rMovement_LookAtPoint(ioCharacter, &destination);
				return UUcTrue;

			} else if (distance < AI2cManeuver_GunPickupRange) {
				if (ioCharacter->pickup_target != ioManeuverState->gun_target) {
					// stop on top of the weapon, and start a pickup animation
					AI2rPath_Halt(ioCharacter);

					weapon_class = WPrGetClass(ioManeuverState->gun_target);
					found_animation = ONrCharacter_DoAnimation(ioCharacter, active_character, ONcAnimPriority_Appropriate,
						(weapon_class->flags & WPcWeaponClassFlag_TwoHanded)
						? ONcAnimType_Pickup_Rifle : ONcAnimType_Pickup_Pistol);
					if (found_animation) {
						// we have started our pickup animation!
						ioCharacter->pickup_target = ioManeuverState->gun_target;

						if (ioCharacter->inventory.weapons[0]) {
							// drop our current (empty) weapon
							ONrCharacter_DropWeapon(ioCharacter,UUcFalse,WPcPrimarySlot,UUcFalse);
						}
					}
				}

				// continue performing the movement
				return UUcTrue;
			} else {
				if (!ioManeuverState->gun_running_pickup) {
					// we can't pick up this weapon yet, we are too far away
					ioCharacter->pickup_target = NULL;
				}

				thwarted = (ioCharacter->pathState.at_destination) || (active_character->movement_state.collision_stuck);
				possibly_thwarted = (MUrPoint_Distance_SquaredXZ(&ioCharacter->location, &destination) < UUmSQR(AI2cManeuver_ExpectedReachGunRange));

				if (possibly_thwarted &&
					(UUmFabs(destination.y - ioCharacter->location.y) > AI2cManeuver_GunUnreachableVerticalFraction * distance)) {
					// this gun is potentially unreachable - it is too far above or below us.
					thwarted = UUcTrue;
				}

				if (thwarted) {
					ioManeuverState->gun_thwart_timer++;
				} else {
					ioManeuverState->gun_thwart_timer = 0;

					if (possibly_thwarted) {
						ioManeuverState->gun_possible_thwart_timer++;
					} else {
						ioManeuverState->gun_possible_thwart_timer = 0;
					}
				}

				// determine our behavior
				thwarted = UUcFalse;
				if (ioManeuverState->gun_thwart_timer >= AI2cManeuver_GunThwartFrames) {
					if (gDebugGunBehavior) {
						COrConsole_Printf("%s: thwarted! mark weapon as uncollectable!", ioCharacter->player_name);
					}
					thwarted = UUcTrue;

				} else if (ioManeuverState->gun_possible_thwart_timer >= AI2cManeuver_GunPossibleThwartFrames) {
					if (gDebugGunBehavior) {
						COrConsole_Printf("%s: possibly thwarted, move on. mark weapon as uncollectable!", ioCharacter->player_name);
					}
					thwarted = UUcTrue;
				}

				// check to see if we should abort our action
				if (thwarted) {
					// try to find this gun in our existing thwarted array
					for (itr = 0; itr < ioManeuverState->guns_num_thwarted; itr++) {
						if (ioManeuverState->guns_thwarted[itr] == ioManeuverState->gun_target) {
							break;
						}
					}
					if (itr < ioManeuverState->guns_num_thwarted) {
						// the gun already exists in the array
					} else {
						// move all guns down in the array
						for (itr = 0; itr < ioManeuverState->guns_num_thwarted; itr++) {
							if (itr < AI2cManeuver_MaxThwartedGuns - 1) {
								ioManeuverState->guns_thwarted[itr + 1] = ioManeuverState->guns_thwarted[itr];
							}
						}
						ioManeuverState->guns_thwarted[0] = ioManeuverState->gun_target;
						ioManeuverState->guns_num_thwarted = UUmMax(ioManeuverState->guns_num_thwarted + 1, AI2cManeuver_MaxThwartedGuns);
					}

					// abort our gun pickup attempt.
					ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Gun] = 0.0f;
					ioManeuverState->gun_target = NULL;
					return UUcFalse;
				}

				distance = ioManeuverState->gun_search_maxradius;
				path_to_gun = AI2iManeuver_TryPathToGun(ioCharacter, ioManeuverState, inWeightToBeat, &destination, &distance);
				if (path_to_gun) {
					// perform this maneuver!
					return UUcTrue;
				} else {
					// abort our gun pickup attempt.
					ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Gun] = 0.0f;
					ioManeuverState->gun_target = NULL;
					return UUcFalse;
				}
			}
		}
	}

	if (!ioCombatState->guncheck_set) {
		UUtUns16 random_chance;

		// check to see if we can go for a gun
		random_chance = (UUtUns16) ((ioCombatState->combat_parameters->gun_pickup_percentage * UUcMaxUns16) / 100);
		ioCombatState->guncheck_enable = (UUrRandom() < random_chance);
		ioCombatState->guncheck_set = UUcTrue;

		if (gDebugGunBehavior) {
			COrConsole_Printf("%s: go for gun chance %02d%%: %s", ioCharacter->player_name,
				ioCombatState->combat_parameters->gun_pickup_percentage, ioCombatState->guncheck_enable ? "YES" : "NO");
		}
	}
	if (!ioCombatState->guncheck_enable) {
		// we can't go for a gun, don't even try
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Gun] = 0.0f;
		return UUcFalse;
	}

	// look for nearby weapons
	weapon_iterator = WPrLocateNearbyWeapons(&ioCharacter->location, ioManeuverState->gun_search_maxradius);

	distance = ioManeuverState->gun_search_maxradius;
	ioManeuverState->gun_target = NULL;
	while ((weapon = WPrClosestNearbyWeapon(&weapon_iterator)) != NULL) {
		UUmAssertReadPtr(weapon, sizeof(UUtUns32));

		if (ioCharacter->flags & ONcCharacterFlag_InfiniteAmmo) {
			// we are a special-case character, only go for guns which don't have to be reloaded
			// to prevent wacky infinite-ammo problems
			if ((WPrGetClass(weapon)->flags & WPcWeaponClassFlag_Unreloadable) == 0) {
				continue;
			}

		} else {
			// we are not a special-case character, don't go for huge guns
			if (WPrGetClass(weapon)->flags & WPcWeaponClassFlag_Unstorable) {
				continue;
			}

			// don't go for empty guns if we don't have the ammo to reload them
			if ((!WPrHasAmmo(weapon)) && (WPrPowerup_Count(ioCharacter, WPrAmmoType(weapon)) == 0)) {
				continue;
			}
		}

		// don't try for guns that we have been thwarted trying to get to before
		ignore_gun = UUcFalse;
		for (itr = 0; itr < ioManeuverState->guns_num_thwarted; itr++) {
			if (weapon == ioManeuverState->guns_thwarted[itr]) {
				ignore_gun = UUcTrue;
				break;
			}
		}
		if (ignore_gun) {
			if (gDebugGunBehavior) {
				COrConsole_Printf("%s: ignoring weapon that we previously tried to pick up", ioCharacter->player_name);
			}
			continue;
		}

		WPrGetPickupPosition(weapon, &destination);

		path_to_gun = AI2iManeuver_TryPathToGun(ioCharacter, ioManeuverState, inWeightToBeat, &destination, &distance);
		if (path_to_gun) {
			// this is an acceptable option... keep looking for a shorter path though
			ioManeuverState->gun_target = weapon;
		}
	}

	if (ioManeuverState->gun_target == NULL) {
		// no weapons are available nearby that we can pathfind to. give up.
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Gun] = 0.0f;
	}

	if (ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Gun] < inWeightToBeat) {
		AI2rPath_Halt(ioCharacter);
		return UUcFalse;
	} else {
		// we have found a path to a nearby weapon, try to pick it up
		return UUcTrue;
	}
}

static UUtBool AI2iManeuver_CheckMelee(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
									  AI2tManeuverState *ioManeuverState, float inWeightToBeat)
{
	AI2tMeleeState *melee_state = &ioCombatState->melee;
	ONtCharacter *prev_ignore_char;
	float dist_sq;

	if (ioCharacter->ai2State.meleeProfile == NULL) {
		// we can't melee. do _anything_ else by preference.
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Melee] = 0.0f;
		return UUcFalse;
	}

	if (ioCombatState->target == NULL) {
		// can't melee dead characters
		ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Melee] = 0.0f;
		return UUcFalse;
	} else {
		melee_state->target = ioCombatState->target;
	}

	if (ioCombatState->try_advance_timer > AI2cCombat_UpdateFrames) {
		ioCombatState->try_advance_timer -= AI2cCombat_UpdateFrames;
	} else {
		ioCombatState->try_advance_timer = 0;
	}

	if (ioCombatState->try_advance_timer == 0) {
		dist_sq = MUmVector_GetDistanceSquared(ioCharacter->location, ioCombatState->target->location);
		if (dist_sq < UUmSQR(AI2cMelee_MaxRange)) {
			// run the melee manager for this tick - NB: we must not be obstructed by the target...
			// the check in AI2rTryToPathfindThrough triggers off the final primary_movement
			// value and will not fire the first time we call this
			prev_ignore_char = ioCharacter->movementOrders.curPathIgnoreChar;
			AI2rMovement_IgnoreCharacter(ioCharacter, ioCombatState->target);
			AI2rMelee_TestLocalEnvironment(ioCharacter, melee_state);
			AI2rMovement_IgnoreCharacter(ioCharacter, prev_ignore_char);

			if (!melee_state->localpath_blocked) {
				// we can successfully enter into melee
				return UUcTrue;
			}
		}
	}

	// we can't go into melee. find something else to do.
	AI2rPath_Halt(ioCharacter);
	ioCombatState->run_targeting = UUcTrue;
	ioCombatState->aim_weapon = UUcFalse;
	ioCombatState->no_technique_timer = AI2cManeuver_Melee_MaxRangeTechniqueFrames;

	// FIXME: eventually, change between advance / retreat / hide depending on whether we are
	// being shot at, and whether we can pathfind to the target?
	ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Melee] = 0.0f;
	ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Advance] = 1.0f;
	ioManeuverState->advance_maxdist = 20.0f;
	ioManeuverState->advance_mindist = 15.0f;
	ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Hold]
		= UUmMax(0.2f, ioManeuverState->primary_movement_weights[AI2cPrimaryMovement_Hold]);
	return UUcFalse;
}

static UUtBool AI2iManeuver_PathfindAndCheckViability(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState,
										 AI2tPrimaryMovementType inManeuverType, float inWeightToBeat)
{
	AI2tManeuverState *maneuver_state = &ioCombatState->maneuver;
	ONtActiveCharacter *active_character;

	switch (inManeuverType) {
	case AI2cPrimaryMovement_Hold:
		// this is always equally viable - don't modify weights.
		AI2rPath_Halt(ioCharacter);
		return UUcTrue;

	case AI2cPrimaryMovement_Advance:
		return AI2iManeuver_CheckAdvance(ioCharacter, ioCombatState, maneuver_state, inWeightToBeat);

	case AI2cPrimaryMovement_Retreat:
		// we are thinking about retreating, don't go into melee (this stops us wavering and charging
		// back towards enemy whenever we encounter an obstruction)
		ioCombatState->maneuver.primary_movement_weights[AI2cPrimaryMovement_Melee] = 0.0f;

		return AI2iManeuver_CheckRetreat(ioCharacter, ioCombatState, maneuver_state, inWeightToBeat);

	case AI2cPrimaryMovement_Gun:
		return AI2iManeuver_CheckGun(ioCharacter, ioCombatState, maneuver_state, inWeightToBeat);

	case AI2cPrimaryMovement_GetUp:
		active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssertReadPtr(active_character, sizeof(*active_character));

		if (ONrAnimState_IsFallen(active_character->nextAnimState)) {
			// we need to send a temporary keypress directly through to the executor
			// so that we will get up rather than lying on the ground aiming!
			AI2rExecutor_MoveOverride(ioCharacter, LIc_BitMask_Forward);
			return UUcTrue;

		} else {
			// we will get up because we are already running some kind of get-up animation.
			maneuver_state->primary_movement_weights[AI2cPrimaryMovement_GetUp] = 0.0f;
			return UUcFalse;
		}

	case AI2cPrimaryMovement_FindAlarm:
		if (maneuver_state->alarm_console == NULL) {
			// try to find a nearby alarm console
			maneuver_state->found_alarm = AI2rAlarm_FindAlarmConsole(ioCharacter);
			if (maneuver_state->found_alarm != NULL) {
				return UUcTrue;
			}

		} else {
			if (AI2rAlarm_IsValidAlarmConsole(ioCharacter, maneuver_state->alarm_console)) {
				// we can go to the specified console
				maneuver_state->found_alarm = maneuver_state->alarm_console->action_marker;
				return UUcTrue;
			}
		}

		// no valid alarm console found
		maneuver_state->primary_movement_weights[AI2cPrimaryMovement_FindAlarm] = 0.0f;
		return UUcFalse;

	case AI2cPrimaryMovement_Melee:
		return AI2iManeuver_CheckMelee(ioCharacter, ioCombatState, maneuver_state, inWeightToBeat);

	default:
		UUmAssert(!"AI2iManeuver_PathfindAndCheckViability: unknown maneuver type");
		if ((inManeuverType >= 0) && (inManeuverType < AI2cPrimaryMovement_Max)) {
			maneuver_state->primary_movement_weights[inManeuverType] = 0.0f;
		}
		return UUcFalse;
	}
}

// ------------------------------------------------------------------------------------
// -- secondary movement control

// determine whether there is a blockage between us and some point that prevents us from moving straight there.
// intended for usage in situations where the pathfinding grid says there is a blockage... does not take into
// account any characters.
UUtBool AI2rManeuver_CheckBlockage(ONtCharacter *ioCharacter, M3tPoint3D *inTargetPoint)
{
	M3tVector3D ray_from, ray_dir, ray_unitdir;
	AKtEnvironment *environment;
	float test_dist;
	ONtActiveCharacter *active_character;

	ONrCharacter_GetPelvisPosition(ioCharacter, &ray_from);
	MUmVector_Subtract(ray_dir, *inTargetPoint, ray_from);

	// check to see if we're clear of geometry blockages
	environment = ONrGameState_GetEnvironment();
	if (AKrCollision_Point(environment, &ray_from, &ray_dir, AKcGQ_Flag_Chr_Col_Skip, 0)) {
		return UUcTrue;
	}

	// we must normalize the ray's direction in order to use AMrRayToObject
	test_dist = MUmVector_GetLength(ray_dir);
	if (test_dist > 1e-03f) {
		MUmVector_ScaleCopy(ray_unitdir, 1.0f / test_dist, ray_dir);
		if (AMrRayToObject(&ray_from, &ray_unitdir, test_dist, NULL, NULL)) {
			return UUcTrue;
		}
	}

	// check to see that our collision would succeed
	active_character = ONrGetActiveCharacter(ioCharacter);
	if ((active_character != NULL) && AI2rManeuver_TrySphereTreeCollision(active_character->sphereTree, &ray_dir)) {
		return UUcTrue;
	}

	// no blockages
	return UUcFalse;
}

// determine whether there are any characters that might block us
UUtBool AI2rManeuver_CheckInterveningCharacters(ONtCharacter *ioCharacter, M3tPoint3D *inTargetPoint)
{
	UUtUns32 itr, partIndex, active_character_count;
	ONtCharacter **active_character_list, *curCharacter;
	M3tVector3D ray_dir;
	M3tPoint3D ray_from, intersect_point;
	UUtBool intersection;
	float check_radius;

	ONrCharacter_GetPelvisPosition(ioCharacter, &ray_from);
	MUmVector_Subtract(ray_dir, *inTargetPoint, ray_from);

	active_character_list = ONrGameState_ActiveCharacterList_Get();
	active_character_count = ONrGameState_ActiveCharacterList_Count();
	check_radius = ONrCharacter_GetLeafSphereRadius(ioCharacter) + 3.0f;

	for(itr = 0; itr < active_character_count; itr++)
	{
		curCharacter = active_character_list[itr];
		UUmAssert(curCharacter->flags & ONcCharacterFlag_InUse);
		if ((curCharacter == ioCharacter) || (curCharacter->flags & (ONcCharacterFlag_Dead_3_Cosmetic | ONcCharacterFlag_Teleporting))) {
			continue;
		}

		// check the ray against the character's body parts
		intersection = ONrCharacter_Collide_SphereRay(curCharacter, &ray_from, &ray_dir, check_radius,
												&partIndex, NULL, &intersect_point, NULL);
		if (intersection) {
			// there is an intervening character, we cannot move here
			return UUcTrue;
		}
	}

	return UUcFalse;
}

// try colliding with a spheretree against the environment
UUtBool AI2rManeuver_TrySphereTreeCollision(PHtSphereTree *inSphereTree, M3tVector3D *inMovementVector)
{
	AKtEnvironment *environment = ONrGameState_GetEnvironment();
	UUtUns32 itr;

	// find all quads that might affect this movement
	AKrCollision_Sphere(environment, &inSphereTree->sphere, inMovementVector, AKcGQ_Flag_Chr_Col_Skip, AKcMaxNumCollisions);

	for(itr = 0; itr < AKgNumCollisions; itr++)
	{
		// check to see whether this quad would block our movement
		AKtCollision *curCollision = AKgCollisionList + itr;
		AKtGQ_General *gqGeneral = environment->gqGeneralArray->gqGeneral + curCollision->gqIndex;
		AKtGQ_Collision *gqCollision = environment->gqCollisionArray->gqCollision + curCollision->gqIndex;
		M3tVector3D to_plane_vec, endpoint;
		M3tPoint3D collision_point;
		M3tPlaneEquation plane;

		AKmPlaneEqu_GetComponents(
			gqCollision->planeEquIndex,
			environment->planeArray->planes,
			plane.a,
			plane.b,
			plane.c,
			plane.d);

		if ((float)fabs(plane.b) > ONcMinFloorNormalY) {
			// this is a floor or ceiling quad and isn't considered for our purposes
			continue;
		}

		if (inMovementVector->x * plane.a + inMovementVector->y * plane.b + inMovementVector->z * plane.c > 0) {
			// this plane faces away from our direction of movement
			if (gqGeneral->flags & AKcGQ_Flag_2Sided) {
				// flip the normal
				plane.a *= -1;
				plane.b *= -1;
				plane.c *= -1;
				plane.d *= -1;
			} else {
				// this gq does not need to be considered for collision
				continue;
			}
		}

		MUmVector_Add(endpoint, inSphereTree->sphere.center, *inMovementVector);
		if (endpoint.x * plane.a + endpoint.y * plane.b + endpoint.z * plane.c + plane.d > 0) {
			// our sphere does not intersect the plane along its movement vector; while we may not
			// be able to move all the way to the endpoint, there isn't a blockage.
			continue;
		}

		MUmVector_Subtract(to_plane_vec, curCollision->collisionPoint, inSphereTree->sphere.center);
		if (to_plane_vec.x * plane.a + to_plane_vec.y * plane.b + to_plane_vec.z * plane.c > 0) {
			// this collision occurs on the back side of the sphere, ignore it
			continue;
		}

		if (!PHrCollision_Quad_SphereTreeVector(environment, curCollision->gqIndex, inSphereTree, inMovementVector, &collision_point)) {
			// we do not actually intersect this quad (this check comes last because it's expensive)
			continue;
		}

		// this is a valid wall collision
		return UUcTrue;
	}

	return UUcFalse;
}

// check to see if we can perform an escape move
const TRtAnimation *AI2rManeuver_TryEscapeMove(ONtCharacter *ioCharacter, AI2tMovementDirection inDirection, UUtBool inRequirePickup, M3tPoint3D *outPoint)
{
	ONtActiveCharacter *active_character = ONrForceActiveCharacter(ioCharacter);
	const TRtAnimation *animation = NULL;
	TRtAnimType try_anim_type;
	M3tPoint3D localmove_pt, localmove_endpt;
	UUtBool localmove_success, localmove_escape;
	UUtUns8 localmove_weight;
	TRtPosition *position_pt;
	float sintheta, costheta;
	UUtError error;

	if (active_character == NULL) {
		return NULL;
	}

	try_anim_type = ONcAnimType_None;
	if (inDirection == AI2cMovementDirection_Stopped) {
		inDirection = active_character->movement_state.current_direction;
	}

	switch(inDirection) {
		case AI2cMovementDirection_Forward:
			try_anim_type = ONcAnimType_Crouch_Forward;
		break;

		case AI2cMovementDirection_Backpedal:
			try_anim_type = ONcAnimType_Crouch_Back;
		break;

		case AI2cMovementDirection_Sidestep_Left:
			try_anim_type = ONcAnimType_Crouch_Left;
		break;

		case AI2cMovementDirection_Sidestep_Right:
			try_anim_type = ONcAnimType_Crouch_Right;
		break;
	}

	if (try_anim_type != ONcAnimType_None) {
		// look for an escape-move animation from our current state that can pick up
		animation = TRrCollection_Lookup(ioCharacter->characterClass->animations, try_anim_type,
										active_character->nextAnimState, active_character->animVarient);
		if ((animation != NULL) && inRequirePickup) {
			if ((!TRrAnimation_TestFlag(animation, ONcAnimFlag_Can_Pickup)) ||
				(TRrAnimation_TestFlag(animation, ONcAnimFlag_NoAIPickup))) {
				// we are looking for pickup animations, and this animation is not acceptable.
				animation = NULL;
			}
		}
	}

	if (animation == NULL) {
		// look for a sliding animation from our current state that can pick up
		animation = TRrCollection_Lookup(ioCharacter->characterClass->animations, ONcAnimType_Slide,
										active_character->nextAnimState, active_character->animVarient);
		if ((animation != NULL) && inRequirePickup) {
			if ((!TRrAnimation_TestFlag(animation, ONcAnimFlag_Can_Pickup)) ||
				(TRrAnimation_TestFlag(animation, ONcAnimFlag_NoAIPickup))) {
				// we are looking for pickup animations, and this animation is not acceptable.
				animation = NULL;
			}
		}
	}

	if (animation == NULL) {
		return NULL;
	}

	// get the endpoint of the escape move
	sintheta = MUrSin(ioCharacter->facing);
	costheta = MUrCos(ioCharacter->facing);
	position_pt = TRrAnimation_GetPositionPoint(animation, TRrAnimation_GetDuration(animation) - 1);
	localmove_pt.x = (costheta * position_pt->location_x + sintheta * position_pt->location_y) * TRcPositionGranularity;
	localmove_pt.z = (costheta * position_pt->location_y - sintheta * position_pt->location_x) * TRcPositionGranularity;
	localmove_pt.y = 0;
	MUmVector_Add(localmove_pt, localmove_pt, ioCharacter->actual_position);

	// check to see if the animation is safe with regards to our local environment
	error = AI2rTryLocalMovement(ioCharacter, PHcPathMode_CasualMovement, &ioCharacter->actual_position, &localmove_pt,
								3, &localmove_success, &localmove_endpt,
								&localmove_weight, &localmove_escape);
	if ((error != UUcError_None) || (!localmove_success)) {
		// local movement is impossible
		return NULL;
	}

	if (localmove_escape) {
		// we must check that this escape path isn't impassable
		if (AI2rManeuver_CheckBlockage(ioCharacter, &localmove_pt)) {
			return NULL;
		}
	}

	// the escape move is valid!
	if (outPoint != NULL) {
		*outPoint = localmove_pt;
	}
	return animation;
}

// check to see if we can pick up a gun with an escape move
UUtBool AI2rManeuver_TryRunningGunPickup(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	const TRtAnimation *animation;
	M3tPoint3D endpoint;
	M3tVector3D gun_pos, delta_gun_pos;
	AI2tMovementDirection pickup_direction;
	float gun_dist_sq, gun_angle, relative_gun_angle, endpoint_dist;
	ONtActiveCharacter *active_character;

	if (ioCombatState->maneuver.primary_movement != AI2cPrimaryMovement_Gun)
		return UUcFalse;

	if (ioCombatState->maneuver.gun_target == NULL)
		return UUcFalse;

	// check the gun is in range
	WPrGetPickupPosition(ioCombatState->maneuver.gun_target, &gun_pos);
	MUmVector_Subtract(delta_gun_pos, gun_pos, ioCharacter->actual_position);
	gun_dist_sq = UUmSQR(delta_gun_pos.x) + UUmSQR(delta_gun_pos.z);

	active_character = ONrForceActiveCharacter(ioCharacter);
	if (active_character == NULL) {
#if DEBUG_GUN_PICKUP
		COrConsole_Printf("%s: guncheck can't make active");
#endif
		return UUcFalse;
	}

	if (!ONrCharacter_IsInactiveUpright(ioCharacter)) {
		if (ONrAnim_IsPickup(active_character->curAnimType)) {
			// we're already playing the stationary pickup animation
			return UUcFalse;

		} else if (TRrAnimation_TestFlag(active_character->animation, ONcAnimFlag_Can_Pickup) &&
					!TRrAnimation_TestFlag(active_character->animation, ONcAnimFlag_NoAIPickup)) {
			// we're already running a direct-pickup-capable animation
			if (gun_dist_sq < UUmSQR(ONcPickupRadius)) {
#if DEBUG_GUN_PICKUP
				COrConsole_Printf("%s: guncheck picked up weapon", ioCharacter->player_name);
#endif
				ONrCharacter_PickupWeapon(ioCharacter, ioCombatState->maneuver.gun_target, UUcFalse);
			} else {
#if DEBUG_GUN_PICKUP
				COrConsole_Printf("%s: guncheck running pickup-capable anim but outside range", ioCharacter->player_name);
#endif
			}
			return UUcTrue;

		} else {
#if DEBUG_GUN_PICKUP
			COrConsole_Printf("%s: guncheck is not a capable anim type", ioCharacter->player_name);
#endif
			return UUcFalse;
		}
	}

	if (fabs(delta_gun_pos.y) > AI2cManeuver_VerticalPickupRange) {
#if DEBUG_GUN_PICKUP
		COrConsole_Printf("%s: guncheck is outside vertical range (abs(%f) > %f)",
						ioCharacter->player_name, delta_gun_pos.y, AI2cManeuver_VerticalPickupRange);
#endif
		return UUcFalse;
	}

	if (gun_dist_sq > UUmSQR(AI2cManeuver_GunMaxRunningPickupRange)) {
#if DEBUG_GUN_PICKUP
		COrConsole_Printf("%s: guncheck is too far (%f > max %f)",
						ioCharacter->player_name, MUrSqrt(gun_dist_sq), AI2cManeuver_GunMaxRunningPickupRange);
#endif
		return UUcFalse;
	}

	// check we aren't so close that we would miss the gun if we did an escape move
	if (gun_dist_sq < UUmSQR(AI2cManeuver_GunMinRunningPickupRange)) {
#if DEBUG_GUN_PICKUP
		COrConsole_Printf("%s: guncheck is too close (%f < min %f)",
							ioCharacter->player_name, MUrSqrt(gun_dist_sq), ONcPickupRadius);
#endif
		return UUcFalse;
	}

	// get the direction to the gun
	gun_angle = MUrATan2(delta_gun_pos.x, delta_gun_pos.z);
	UUmTrig_ClipLow(gun_angle);

	relative_gun_angle = gun_angle - ioCharacter->facing;
	UUmTrig_ClipAbsPi(relative_gun_angle);

	if (fabs(relative_gun_angle) < AI2cManeuver_GunPickupDeltaAngle) {
		pickup_direction = AI2cMovementDirection_Forward;

	} else if (fabs(relative_gun_angle - M3cHalfPi) < AI2cManeuver_GunPickupDeltaAngle) {
		pickup_direction = AI2cMovementDirection_Sidestep_Left;

	} else if (fabs(relative_gun_angle + M3cHalfPi) < AI2cManeuver_GunPickupDeltaAngle) {
		pickup_direction = AI2cMovementDirection_Sidestep_Right;

	} else if (fabs(relative_gun_angle) > M3cPi - AI2cManeuver_GunPickupDeltaAngle) {
		pickup_direction = AI2cMovementDirection_Backpedal;

	} else {
		// the gun is not exactly in a direction from us, don't try running a pickup animation
#if DEBUG_GUN_PICKUP
		COrConsole_Printf("%s: guncheck direction %f is not along an axis", ioCharacter->player_name, relative_gun_angle * M3cRadToDeg);
#endif
		return UUcFalse;
	}

	// try an escape move in the desired direction
	animation = AI2rManeuver_TryEscapeMove(ioCharacter, pickup_direction, UUcTrue, &endpoint);
	if (animation == NULL) {
#if DEBUG_GUN_PICKUP
		COrConsole_Printf("%s: guncheck found no escape move in direction %s", ioCharacter->player_name, AI2cMovementDirectionName[pickup_direction]);
#endif
		return UUcFalse;
	}

	// how far away is this endpoint along the gun angle?
	endpoint_dist = MUrSin(gun_angle) * (endpoint.x - ioCharacter->actual_position.x) +
					MUrCos(gun_angle) * (endpoint.z - ioCharacter->actual_position.z);
	if (endpoint_dist < MUrSqrt(gun_dist_sq) + AI2cManeuver_GunRunningPickupOvershootRange) {
		// this escape move doesn't bring us close enough to the gun, but it will in the future
#if DEBUG_GUN_PICKUP
		COrConsole_Printf("%s: guncheck escape move dist %f isn't beyond dist %f + pickup %f",
					ioCharacter->player_name, endpoint_dist, MUrSqrt(gun_dist_sq), AI2cManeuver_GunMinRunningPickupRange);
#endif
		return UUcTrue;
	}

	// apply the escape move
#if DEBUG_GUN_PICKUP
	COrConsole_Printf("%s: guncheck set animation %s", ioCharacter->player_name, TMrInstance_GetInstanceName(animation));
#endif
	ioCharacter->pickup_target = ioCombatState->maneuver.gun_target;
	ONrCharacter_SetAnimation_External(ioCharacter, active_character->nextAnimState, animation, 6);

	if (ioCharacter->inventory.weapons[0]) {
		// drop our current (empty) weapon
		ONrCharacter_DropWeapon(ioCharacter, UUcFalse, WPcPrimarySlot, UUcFalse);
	}

	return UUcTrue;
}

// determine dodge-type secondary movement
void AI2rManeuver_FindDodge(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	AI2tManeuverState *maneuver_state = &ioCombatState->maneuver;
	UUtBool entry_deleted;
	AI2tDodgeEntry *entry;
	M3tVector3D dodge_dir_accum;
	UUtUns32 itr, num_entries;

	num_entries = 0;
	ioCombatState->maneuver.dodge_max_weight = 0;
	ioCombatState->maneuver.dodge_avoid_angle = AI2cAngle_None;
	MUmVector_Set(dodge_dir_accum, 0, 0, 0);

	// update our dodge table with any nearby projectiles and firing spreads
	AI2rKnowledge_FindNearbyProjectiles(ioCharacter, maneuver_state);
	AI2rKnowledge_FindNearbyFiringSpreads(ioCharacter, maneuver_state);

	// age our dodge table, and build movement modifiers for currently active dodges
	for (itr = 0, entry = maneuver_state->dodge_entries; itr < maneuver_state->num_dodge_entries; itr++, entry++) {
		entry_deleted = UUcFalse;

		if (entry->cause_type == AI2cDodgeCauseType_None) {
			// this entry is already deleted
			entry_deleted = UUcTrue;

		} else if (entry->active_timer <= AI2cCombat_UpdateFrames) {
			// this entry is now going inactive, mark it as such
			entry->active_timer = 0;
			entry->cause_type = AI2cDodgeCauseType_None;
			entry_deleted = UUcTrue;

		} else {
			// this entry is still active, but decrement its timer
			entry->active_timer -= AI2cCombat_UpdateFrames;
			entry->total_frames += AI2cCombat_UpdateFrames;
		}

		if (entry_deleted) {
			if (itr + 1 == maneuver_state->num_dodge_entries) {
				maneuver_state->num_dodge_entries--;
			}
			continue;
		}

		if (entry->total_frames < ioCharacter->characterClass->ai2_behavior.dodge_react_frames) {
			// we can't react to this hazard yet, it hasn't been around for long enough
			continue;
		}

		// work out the amount that we want to dodge in this direction (depends on timer and the actual weight
		// of the danger)
		if (entry->max_active_timer == 0) {
			entry->current_weight = 0;
		} else {
			entry->current_weight = (entry->weight * entry->active_timer) / entry->max_active_timer;
			if (entry->current_weight > ioCombatState->maneuver.dodge_max_weight) {
				ioCombatState->maneuver.dodge_max_weight = entry->current_weight;
				ioCombatState->maneuver.dodge_avoid_angle = entry->avoid_angle;
			}
		}

		// add the contribution from this danger into our dodge direction
		MUmVector_ScaleIncrement(dodge_dir_accum, entry->current_weight, entry->dodge_dir);
		num_entries++;
	}

	if (num_entries == 0) {
		ioCombatState->maneuver.dodge_max_weight = 0;
		return;
	}

	// calculate the angle that we want to dodge in (a combination of all our danger sources)
	ioCombatState->maneuver.dodge_accum_angle = MUrATan2(dodge_dir_accum.x, dodge_dir_accum.z);
	UUmTrig_ClipLow(ioCombatState->maneuver.dodge_accum_angle);
}

// apply dodge movement as a movement modifier
void AI2rManeuver_ApplyDodge(ONtCharacter *ioCharacter, AI2tCombatState *ioCombatState)
{
	UUtBool localpath_success, localpath_escape;
	M3tPoint3D localpath_point;
	UUtUns8 localpath_weight;
	float real_dodge_angle;
	UUtError error;

	if (ioCombatState->maneuver.dodge_max_weight < 1e-03f) {
		// nothing to dodge
		return;
	}

	error = AI2rFindLocalPath(ioCharacter, PHcPathMode_CasualMovement, NULL, NULL, NULL, AI2cDodge_LocalPathDistance,
								ioCombatState->maneuver.dodge_accum_angle, AI2cDodge_LocalPathAngle / 4, 4,
								PHcBorder3, 3, &localpath_success, &localpath_point, &localpath_weight, &localpath_escape);
	if ((error == UUcError_None) && (localpath_success)) {
		// calculate the direction that we found to move in
		real_dodge_angle = MUrATan2(localpath_point.x - ioCharacter->actual_position.x, localpath_point.z - ioCharacter->actual_position.z);
		UUmTrig_ClipLow(real_dodge_angle);

		// add a movement modifier in this direction
		AI2rMovement_MovementModifier(ioCharacter, real_dodge_angle, AI2cDodge_WeightMultiplier * ioCombatState->maneuver.dodge_max_weight);
	}

	// apply our avoid-angle
	if ((ioCombatState->maneuver.dodge_max_weight > AI2cDodge_MinimumAvoidWeight) &&
		(ioCombatState->maneuver.dodge_avoid_angle != AI2cAngle_None)) {
		AI2rMovement_MovementModifierAvoidAngle(ioCharacter, ioCombatState->maneuver.dodge_avoid_angle);
	} else {
		AI2rMovement_MovementModifierAvoidAngle(ioCharacter, AI2cAngle_None);
	}
}

// based on a given danger, work out how we should dodge
void AI2iManeuver_MakeDodgeFromDanger(ONtCharacter *ioCharacter, AI2tManeuverState *ioManeuverState, AI2tDodgeEntry *ioEntry)
{
	UUtBool use_previous = UUcFalse, direct_hit, path0_ok, path1_ok, localpath_escape;
	float dist_to_danger;
	M3tVector3D localmove_point;
	M3tPoint3D localpath_endpoint;
	UUtUns8 localpath_weight;
	UUtError error;

	// calculate whether the danger is very near at hand
	dist_to_danger = MUmVector_GetLength(ioEntry->danger_dir);
	direct_hit = (dist_to_danger < AI2cDodge_DirectHitRadius);

	// store the direction that we shouldn't go in
	if (direct_hit) {
		ioEntry->avoid_angle = MUrATan2(-ioEntry->incoming_dir.x, -ioEntry->incoming_dir.z);
	} else {
		ioEntry->avoid_angle = MUrATan2(ioEntry->danger_dir.x, ioEntry->danger_dir.z);
	}
	UUmTrig_ClipLow(ioEntry->avoid_angle);

	if (ioEntry->already_dodging) {
		if (direct_hit) {
			use_previous = UUcTrue;
		} else {
			// only use the previous dodge if it's still away from the danger
			use_previous = (MUrVector_DotProduct(&ioEntry->danger_dir, &ioEntry->dodge_dir) < 0);
		}
	}

	if (use_previous) {
		// just use the same dodge direction as last time
		return;
	}

	if (direct_hit) {
		// pick a random direction that's horizontal, and perpendicular to the incoming direction
		ioEntry->dodge_dir.x = ioEntry->incoming_dir.z;
		ioEntry->dodge_dir.y = 0;
		ioEntry->dodge_dir.z = -ioEntry->incoming_dir.x;
		if (MUrVector_Normalize_ZeroTest(&ioEntry->dodge_dir)) {
			ioEntry->dodge_dir.x = 1;
			ioEntry->dodge_dir.y = 0;
			ioEntry->dodge_dir.z = 0;
		} else {
			// look in both perpendicular directions to see if one is blocked
			localmove_point.x = ioCharacter->actual_position.x + AI2cDodge_LocalPathDistance * ioEntry->dodge_dir.x;
			localmove_point.y = ioCharacter->actual_position.y;
			localmove_point.z = ioCharacter->actual_position.z + AI2cDodge_LocalPathDistance * ioEntry->dodge_dir.z;
			error = AI2rTryLocalMovement(ioCharacter, PHcPathMode_CasualMovement, &ioCharacter->actual_position, &localmove_point, 3,
										&path0_ok, &localpath_endpoint, &localpath_weight, &localpath_escape);
			if (error != UUcError_None) {
				path0_ok = UUcFalse;
			}

			localmove_point.x = ioCharacter->actual_position.x - AI2cDodge_LocalPathDistance * ioEntry->dodge_dir.x;
			localmove_point.y = ioCharacter->actual_position.y;
			localmove_point.z = ioCharacter->actual_position.z - AI2cDodge_LocalPathDistance * ioEntry->dodge_dir.z;
			error = AI2rTryLocalMovement(ioCharacter, PHcPathMode_CasualMovement, &ioCharacter->actual_position, &localmove_point, 3,
										&path1_ok, &localpath_endpoint, &localpath_weight, &localpath_escape);
			if (error != UUcError_None) {
				path1_ok = UUcFalse;
			}

			if (path0_ok && !path1_ok) {
				// leave the dodge direction as it is
			} else if (path1_ok && !path0_ok) {
				// force the reversed dodge direction
				MUmVector_Negate(ioEntry->dodge_dir);
			} else {
				// randomly decide to use one of the two dodge directions
				if (UUrRandom() < (UUcMaxUns16 / 2)) {
					MUmVector_Negate(ioEntry->dodge_dir);
				}
			}
		}
	} else {
		// move directly away from the danger
		ioEntry->dodge_dir.x = -ioEntry->danger_dir.x;
		ioEntry->dodge_dir.y = 0;
		ioEntry->dodge_dir.z = -ioEntry->danger_dir.z;
	}

	ioEntry->already_dodging = UUcTrue;
}

// update a dodge-type maneuver table of projectiles
void AI2rManeuver_NotifyNearbyProjectile(ONtCharacter *ioCharacter, AI2tManeuverState *ioManeuverState,
										 AI2tDodgeProjectile *inProjectile, M3tVector3D *inIncomingDirection,
										 float inClosestDistance, M3tVector3D *inDirectionToTrajectory)
{
	AI2tDodgeEntry *entry, *overwrite_entry;
	float weight;
	UUtUns32 itr, timer;
	UUtBool found;

	// calculate the weight for this movement modifier
	weight = (inProjectile->dodge_radius - inClosestDistance) / AI2cDodge_MaxWeightRadius;
	if (weight > AI2cDodge_MaxWeight) {
		weight = AI2cDodge_MaxWeight;

	} else if (weight < AI2cDodge_ZeroWeight) {
		// ignore this entry
		return;

	} else if (weight < AI2cDodge_MinWeight) {
		weight = AI2cDodge_MinWeight;
	}

	weight -= (1.0f - ioCharacter->characterClass->ai2_behavior.dodge_weight_scale);
	if (weight < 0) {
		return;
	}

	// find an entry in our dodge table to store this in
	found = UUcFalse;
	overwrite_entry = NULL;
	for (itr = 0, entry = ioManeuverState->dodge_entries; itr < ioManeuverState->num_dodge_entries; itr++, entry++) {
		if ((entry->cause_type == AI2cDodgeCauseType_Projectile) && (entry->cause.projectile.projectile == inProjectile)) {
			// this is an existing entry for this projectile!
			found = UUcTrue;
			break;

		} else if (entry->cause_type == AI2cDodgeCauseType_None) {
			// this is a free entry
			overwrite_entry = entry;
		}
	}

	if (!found) {
		// we failed to find an existing entry for this projectile
		if (overwrite_entry == NULL) {
			// our table is full as well
			if (ioManeuverState->num_dodge_entries < AI2cDodgeEntries_Max) {
				entry = &ioManeuverState->dodge_entries[ioManeuverState->num_dodge_entries++];
			} else {
				// we can't add this entry
				return;
			}
		} else {
			// overwrite this entry
			entry = overwrite_entry;
		}

		// clear persistent fields in this entry
		entry->already_dodging = UUcFalse;
		entry->total_frames = 0;
	}

	timer = MUrUnsignedSmallFloat_To_Uns_Round(AI2cDodge_ProjectileActiveTimer * ioCharacter->characterClass->ai2_behavior.dodge_time_scale);
	entry->active_timer = timer;
	entry->max_active_timer = timer;
	entry->weight = weight;
	entry->cause_type = AI2cDodgeCauseType_Projectile;
	entry->cause.projectile.projectile = inProjectile;
	entry->incoming_dir = *inIncomingDirection;
	entry->danger_dir.x = inDirectionToTrajectory->x;
	entry->danger_dir.y = 0;
	entry->danger_dir.z = inDirectionToTrajectory->z;

	AI2iManeuver_MakeDodgeFromDanger(ioCharacter, ioManeuverState, entry);
}

// update a dodge-type maneuver table of firing spreads
void AI2rManeuver_NotifyFiringSpread(ONtCharacter *ioCharacter, AI2tManeuverState *ioManeuverState,
									 AI2tDodgeFiringSpread *inSpread, M3tVector3D *inIncomingDirection,
									 float inSpreadSizeAtThisRange, float inDistanceToCenter,
									 M3tVector3D *inDirectionToCenter)
{
	AI2tDodgeEntry *entry, *overwrite_entry;
	float weight, maxweight_dist, interp_down;
	UUtUns32 itr, timer;
	UUtBool found;

	// calculate the weight for this movement modifier
	maxweight_dist = AI2cDodge_SpreadMaxWeightAtDist * inSpreadSizeAtThisRange;
	interp_down = (inDistanceToCenter - maxweight_dist) / (inSpreadSizeAtThisRange - maxweight_dist);
	if (interp_down < 0) {
		weight = AI2cDodge_MaxWeight;

	} else if (interp_down < 1) {
		weight = AI2cDodge_MaxWeight + interp_down * (AI2cDodge_MinWeight - AI2cDodge_MaxWeight);

	} else if (inDistanceToCenter > AI2cDodge_SpreadRangeZero * inSpreadSizeAtThisRange) {
		// we are too far away, don't bother with this spread
		return;

	} else {
		weight = AI2cDodge_MinWeight;
	}

	weight -= (1.0f - ioCharacter->characterClass->ai2_behavior.dodge_weight_scale);
	if (weight < 0) {
		return;
	}

	// find an entry in our dodge table to store this in
	found = UUcFalse;
	overwrite_entry = NULL;
	for (itr = 0, entry = ioManeuverState->dodge_entries; itr < ioManeuverState->num_dodge_entries; itr++, entry++) {
		if ((entry->cause_type == AI2cDodgeCauseType_FiringSpread) && (entry->cause.spread.spread == inSpread)) {
			// this is an existing entry for this firing spread!
			found = UUcTrue;
			break;

		} else if (entry->cause_type == AI2cDodgeCauseType_None) {
			// this is a free entry
			overwrite_entry = entry;
		}
	}

	if (!found) {
		// we failed to find an existing entry for this firing spread
		if (overwrite_entry == NULL) {
			// our table is full as well
			if (ioManeuverState->num_dodge_entries < AI2cDodgeEntries_Max) {
				entry = &ioManeuverState->dodge_entries[ioManeuverState->num_dodge_entries++];
			} else {
				// we can't add this entry
				return;
			}
		} else {
			// overwrite this entry
			entry = overwrite_entry;
		}

		// clear persistent fields in this entry
		entry->already_dodging = UUcFalse;
		entry->total_frames = 0;
	}

	timer = MUrUnsignedSmallFloat_To_Uns_Round(AI2cDodge_FiringSpreadActiveTimer * ioCharacter->characterClass->ai2_behavior.dodge_time_scale);
	entry->active_timer = timer;
	entry->max_active_timer = timer;
	entry->weight = weight;
	entry->cause_type = AI2cDodgeCauseType_FiringSpread;
	entry->cause.spread.spread = inSpread;
	entry->incoming_dir = *inIncomingDirection;
	entry->danger_dir.x = inDirectionToCenter->x;
	entry->danger_dir.y = 0;
	entry->danger_dir.z = inDirectionToCenter->z;

	AI2iManeuver_MakeDodgeFromDanger(ioCharacter, ioManeuverState, entry);
}



