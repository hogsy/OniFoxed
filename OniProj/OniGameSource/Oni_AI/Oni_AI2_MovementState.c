/*
	FILE:	Oni_AI2_MovementState.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: July 29, 2000
	
	PURPOSE: Incarnated Movement AI for Oni
	
	Copyright (c) Bungie Software, 2000

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Timer.h"

#include "Oni_AI2.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"


// ------------------------------------------------------------------------------------
// -- fudge factors

#define AI2cPathPoint_Exact_Hit							(0.75f * PHcSquareSize)
#define AI2cPathPoint_Tight_Hit							(2.0f  * PHcSquareSize)
#define AI2cPathPoint_Free_Hit							(3.0f  * PHcSquareSize)

#define AI2cPathPoint_WalkSidewaysAllowDist				15.0f
#define AI2cPathPoint_OverrideTurnDistance				(1.25f * PHcSquareSize)
#define AI2cPathPoint_KeepMovingFrames					60			// keep moving beyond a don't-stop point for x frames
#define AI2cOverturnDisableFrames						40

#define AI2cTurningRadius_SafetyMargin					0.9f
#define AI2cStoppingDistance_SafetyMargin				0.9f
#define AI2cStoppingDistance_HaltFrames					40

#define AI2cFacingDistance_ReturnFromSlowDown			(  5.0f * M3cDegToRad)
#define AI2cFacingDistance_CollisionForceStationaryTurn	( 40.0f * M3cDegToRad)
#define AI2cFacingDistance_WalkingForceStationaryTurn	( 80.0f * M3cDegToRad)
#define AI2cFacingDistance_RunningForceStationaryTurn	(120.0f * M3cDegToRad)

// look range is greater than aim range because our aiming grids in fact only
// let us aim 90 degrees out of our way. but if it just has to *appear* like
// we're facing in the right direction, this will do fine
#define AI2cMovement_StandingGlanceRange				(115.f * M3cDegToRad)
#define AI2cMovement_MovingLookRange					(115.f * M3cDegToRad)
#define AI2cMovement_MovingAimRange						( 85.f * M3cDegToRad)
#define AI2cMovement_StandingLookRange					( 40.f * M3cDegToRad)
#define AI2cMovement_PanicAimRange						( 25.f * M3cDegToRad)
#define AI2cMovement_NeutralInteractionAimRange			( 10.f * M3cDegToRad)
#define AI2cMovement_MeleeAimRange						( 10.f * M3cDegToRad)
#define AI2cMovement_AnimParticleAimRange				(  4.f * M3cDegToRad)
#define AI2cMovement_DestFacingTolerance				(  5.f * M3cDegToRad)

#define AI2cMovement_Collision_ParallelTolerance		0.2f
#define AI2cMovement_Collision_ComfortDistance			10.0f
#define AI2cMovement_Collision_IgnoreFrames				3
#define AI2cMovement_Collision_SeriousFrames			10
#define AI2cMovement_Collision_MaxUrgencyFrames			30
#define AI2cMovement_Collision_MaxUrgencyWeight			0.5f
#define AI2cMovement_Collision_ErrorFrames				30
#define AI2cMovement_Collision_InsideForgetDistance		5.0f
#define AI2cMovement_Collision_StartForgetDistance		10.0f
#define AI2cMovement_Collision_TotallyForgetDistance	20.0f
#define AI2cMovement_Collision_StartForgetFrames		30
#define AI2cMovement_Collision_TotallyForgetFrames		90
#define AI2cMovement_Collision_MaxVerticalComponent		0.9f
#define AI2cMovement_Collision_DisableAngle				(30.0f * M3cDegToRad)
#define AI2cMovement_Collision_DisableMaxAngle			(90.0f * M3cDegToRad)
#define AI2cMovement_Collision_BadnessAngle				(30.0f * M3cDegToRad)
#define AI2cMovement_Collision_BadnessMaxAngle			(90.0f * M3cDegToRad)
#define AI2cMovement_Collision_HeuristicTheta			0.2f

#define AI2cMovement_Avoid_DisableAngle					(60.0f * M3cDegToRad)
#define AI2cMovement_Avoid_DisableMaxAngle				(100.0f * M3cDegToRad)

#define AI2cMovement_FellFromPath_Range					0.5f
#define AI2cMovement_FellFromPath_Dist					(3.5f * PHcSquareSize)

#define AI2cMovementState_ObstructionMinDistance		5.0f
#define AI2cMovementState_ObstructionCheckDistance		15.0f
#define AI2cMovementState_ObstructionCheckAttempts		6

#define AI2cMovementState_Reuse_MinDist					4.0f
#define AI2cMovementState_Reuse_MaxDist					25.0f

// ------------------------------------------------------------------------------------
// -- internal globals

// temporary array for badness value computation
static UUtUns32 AI2gMovementState_NumBadnessValues;
static AI2tBadnessValue AI2gMovementState_BadnessValues[AI2cMovementState_MaxBadnessValues];

// temporary character for error reporting
static ONtCharacter *AI2gMovementState_BadnessCharacter;

// ------------------------------------------------------------------------------------
// -- internal function prototypes

// collision avoidance
static void AI2iMovementState_AgeCollisions(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
static void AI2iMovementState_ClearCollisionFlags(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
static void AI2iMovementState_ResetBadness(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues);
static void AI2iMovementState_AddBadnessFromCollision(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues,
													  AI2tCollision *inCollision);
static AI2tBadnessValue *AI2iMovementState_CreateBadnessValue(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues,
															  AI2tBadnessValue *inStartValue, float inEndAngle,
															  float inStartIncrement, float inEndIncrement);
static AI2tBadnessValue *AI2iMovementState_NewBadnessValue(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues,
														   AI2tBadnessValue *inInsertPtr);
static void AI2iMovementState_AddBadnessIncrements(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues);
static float AI2iMovementState_LeastBadDirection(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues,
												 float inDirection);

// movement
static void AI2iMovementState_ClearTemporaryFlags(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
static void AI2iMovementState_Update_Facing(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, AStPathPoint *next_path_point);
static void AI2iMovementState_Calculate_Movement(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
static float AI2iMovementState_GetPointHitDistance(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
													AStPathPoint *inPathPoint);
static UUtBool AI2iMovementState_FinalPathPoint(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
static UUtBool AI2iMovementState_StopAtPathPoint(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
													float *outRadius);
static AI2tMovementDirection AI2iMovementState_FindMovementDirection(float inDesiredFacing, float inMoveDirection,
																	UUtBool inDisableSidestep, UUtBool inDisableBackwards);
static UUtBool AI2iMovementState_CheckPathPointHit(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
static PHtNode *AI2iMovementState_GetCurrentNode(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
static UUtBool AI2iMovementState_SetupPath(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
											PHtNode *inNode, UUtBool inActiveTransition);
static void AI2iMovementState_FindCurrentPoint(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
											  AI2tMovementStub *inStub);
static UUtBool AI2iMovementState_FindClearPoint(ONtCharacter *ioCharacter, M3tPoint3D *ioPoint);


// ------------------------------------------------------------------------------------
// -- high-level control functions

static PHtNode *AI2iMovementState_GetCurrentNode(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	if (ioCharacter->pathState.path_current_node == 0) {
		return ioCharacter->pathState.path_start_node;
	} else {
		return ioCharacter->pathState.path_connections[ioCharacter->pathState.path_current_node - 1]->to;
	}
}

void AI2rMovementState_Initialize(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
								  AI2tMovementStub *inStub)
{	
	UUtBool moved;
	M3tPoint3D position;

#if TOOL_VERSION
	ioMovementState->activation_valid = UUcFalse;
	ioMovementState->activation_frompath = UUcFalse;
#endif

	if ((inStub == NULL) || (!inStub->path_active)) {
		// stand still
		ioMovementState->movementDirection = AI2cMovementDirection_Stopped;
		ioMovementState->current_direction = AI2cMovementDirection_Stopped;
		ioMovementState->move_direction = AI2cAngle_None;
		ioMovementState->last_real_move_direction = AI2cAngle_None;

		position = ioCharacter->location;
		moved = AI2iMovementState_FindClearPoint(ioCharacter, &position);
		if (moved) {
			ONrCharacter_Teleport(ioCharacter, &position, UUcFalse);
		}

		AI2rMovementState_ClearPath(ioCharacter, ioMovementState);
	} else {
		PHtNode *current_node;

		// follow the path
		ioMovementState->movementDirection = AI2cMovementDirection_Forward;
		ioMovementState->current_direction = AI2cMovementDirection_Forward;
		ioMovementState->move_direction = inStub->cur_facing;
		ioMovementState->last_real_move_direction = inStub->cur_facing;

		ioMovementState->grid_path_start = inStub->path_start;
		ioMovementState->grid_path_end = inStub->path_end;

		current_node = AI2iMovementState_GetCurrentNode(ioCharacter, ioMovementState);
		if (AI2iMovementState_SetupPath(ioCharacter, ioMovementState, current_node, UUcTrue)) {
			AI2iMovementState_FindCurrentPoint(ioCharacter, ioMovementState, inStub);
		}
	}

	ioMovementState->keepmoving = UUcFalse;
	ioMovementState->turning_to_face_direction = AI2cAngle_None;
	ioMovementState->done_dest_facing = (ioCharacter->movementOrders.facingMode == AI2cFacingMode_FaceAtDestination);

	// no modifiers or collisions
	ioMovementState->numActiveModifiers = 0;
	ioMovementState->modifiers_require_decreasing = UUcFalse;
	ioMovementState->numCollisions = 0;
#if TOOL_VERSION
	ioMovementState->numBadnessValues = 0;
#endif

	// as far as we know, we have sidestep animations
	ioMovementState->invalid_movement_direction = UUcFalse;
	ioMovementState->no_walking_sidestep = UUcFalse;
	ioMovementState->no_running_sidestep = UUcFalse;
	ioMovementState->desire_forwards_if_no_sidestep = UUcTrue;

	AI2rMovementState_ResetAimingVarient(ioCharacter, ioMovementState);
}

static UUtBool AI2iMovementState_FindClearPoint(ONtCharacter *ioCharacter, M3tPoint3D *ioPoint)
{
	UUtError error;
	PHtGraph *graph;
	AKtBNVNode *cur_bnv;
	PHtNode *cur_pathnode;
	M3tPoint3D localpath_point;
	ONtCharacter *obstruction_character;
	OBJtObject *obstruction_door;
	UUtBool localpath_success, localpath_escape;
	UUtUns8 localpath_weight, obstruction;
	UUtUns16 obstruction_val, width;
	PHtDynamicSquare *grid;
	UUtInt16 x, y;

	// get the input position's location in the pathfinding graph
	cur_bnv = AKrNodeFromPoint(ioPoint);
	if (cur_bnv == NULL) {
		return UUcFalse;
	}
	graph = ONrGameState_GetGraph();
	cur_pathnode = PHrAkiraNodeToGraphNode(cur_bnv, graph);
	if (cur_pathnode == NULL) {
		return UUcFalse;
	}

	// check to see if the current point is obstructed
	PHrPrepareRoomForPath(cur_pathnode, &cur_bnv->roomData);
	PHrWorldToGridSpaceDangerous(&x, &y, ioPoint, &cur_bnv->roomData);
	x = UUmPin(x, 0, (UUtInt16) (cur_bnv->roomData.gridX - 1));
	y = UUmPin(y, 0, (UUtInt16) (cur_bnv->roomData.gridY - 1));
	width = (UUtUns16) cur_pathnode->gridX;
	grid = cur_pathnode->dynamic_grid;
	if (!cur_pathnode->grids_allocated || (grid == NULL)) {
		return UUcFalse;
	}

	obstruction = GetObstruction(x, y);

	if (obstruction == 0) {
		return UUcFalse;
	}
	UUmAssert((obstruction > 0) && (obstruction < PHcMaxObstructions));
	obstruction_val = cur_pathnode->obstruction[obstruction];

	if (PHrIgnoreObstruction(ioCharacter, PHcPathMode_CheckClearSpace, obstruction_val)) {
		return UUcFalse;
	}

	if (PHrObstructionIsCharacter(obstruction_val, &obstruction_character)) {
		if (MUrPoint_Distance_SquaredXZ(ioPoint, &obstruction_character->actual_position) > UUmSQR(6.0f)) {
			// we are sufficiently far away from this character's actual position that we don't need to move
			return UUcFalse;
		}

	} else if (PHrObstructionIsDoor(obstruction_val, &obstruction_door)) {
		if (!OBJrDoor_CharacterCanOpen(ioCharacter, obstruction_door, OBJcDoor_EitherSide)) {
			// we can't open this door, don't risk appearing on the wrong side of it
			return UUcFalse;
		}
	}

	// we are standing inside an obstruction! try to find a clear point.
	error = AI2rFindNearbyPoint(ioCharacter, PHcPathMode_CheckClearSpace, ioPoint, 
								AI2cMovementState_ObstructionCheckDistance, AI2cMovementState_ObstructionMinDistance,
								ioCharacter->facing, M3cPi / AI2cMovementState_ObstructionCheckAttempts, AI2cMovementState_ObstructionCheckAttempts,
								PHcDanger, 5, &localpath_success, &localpath_point, &localpath_weight, &localpath_escape);
	if ((error == UUcError_None) && (localpath_success)) {
		// move out of the obstruction
		ioPoint->x = localpath_point.x;
		ioPoint->z = localpath_point.z;
		return UUcTrue;
	}

	return UUcFalse;
}

static void AI2iMovementState_FindCurrentPoint(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
											  AI2tMovementStub *inStub)
{
	float pt_distance[AI2cMax_PathPoints], distance, target_distance, interp;
	UUtUns32 itr;
	M3tPoint3D position;

#if TOOL_VERSION
	ioMovementState->activation_valid = UUcTrue;
	ioMovementState->activation_frompath = UUcFalse;
#endif

	if (inStub->path_distance < PHcSquareSize) {
		// not worth finding exact location
		position = ioCharacter->actual_position;
		goto exit;
	}

	if (ioMovementState->grid_num_points == 0) {
		// go to the start of this path, and stand still.
		position = inStub->path_start;
		goto exit;

	} else {
		// calculate total distance along this path
		distance = 0;
		for (itr = 0; itr + 1 < ioMovementState->grid_num_points; itr++) {
			pt_distance[itr] = MUmVector_GetDistance(ioMovementState->grid_path[itr].point, ioMovementState->grid_path[itr + 1].point);
			distance += pt_distance[itr];
		}

		target_distance = distance * inStub->cur_distance / inStub->path_distance;
		for (itr = 0; itr + 1 < ioMovementState->grid_num_points; itr++) {
			if (target_distance > pt_distance[itr]) {
				target_distance -= pt_distance[itr];
			} else {
				// we want to be somewhere on this path segment
				ioMovementState->grid_current_point = (UUtUns16) (itr + 1);
				ioMovementState->next_path_point = ioMovementState->grid_path + ioMovementState->grid_current_point;

				// warp us to this path point
				interp = target_distance / pt_distance[itr];
				UUmAssert((interp >= 0) && (interp <= 1));

				MUmVector_ScaleCopy(position, 1.0f - interp, ioMovementState->grid_path[itr].point);
				MUmVector_ScaleIncrement(position, interp, ioMovementState->grid_path[itr + 1].point);

#if TOOL_VERSION
				// store this activation point for debugging display
				ioMovementState->activation_frompath = UUcTrue;
				ioMovementState->activation_pathsegment[0] = ioMovementState->grid_path[itr].point;
				ioMovementState->activation_pathsegment[1] = ioMovementState->grid_path[itr + 1].point;
#endif
				goto exit;
			}
		}

		UUmAssert(itr + 1 < ioMovementState->grid_num_points);
	}

exit:
	position.y = ioCharacter->location.y;
#if TOOL_VERSION
	ioMovementState->activation_rawpoint = position;
#endif

	AI2iMovementState_FindClearPoint(ioCharacter, &position);

	ONrCharacter_Teleport(ioCharacter, &position, UUcFalse);
	ONrCharacter_SnapToFloor(ioCharacter);
#if TOOL_VERSION
	ioMovementState->activation_position = ioCharacter->actual_position;
#endif

	return;
}

// ------------------------------------------------------------------------------------
// -- interface functions to the rest of the AI system

/*
 * movement interfaces
 */

// change movement mode
void AI2rMovementState_NewMovementMode(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	AI2tMovementMode new_mode, slow_mode;
	
	new_mode = ioCharacter->movementOrders.movementMode;

	if (ioMovementState->temporarily_slowed) {
		// our movement mode has been modified temporarily. work out what our slowed movement mode should
		// be for this new movement mode.
		slow_mode = new_mode;
		if (slow_mode == AI2cMovementMode_Default) {
			slow_mode = AI2rMovement_Default(ioCharacter);
		}

		// could we slow down without stopping if we had to?
		if (slow_mode >= AI2cMovementMode_NoAim_Run) {
			slow_mode = (AI2tMovementMode)(slow_mode - (AI2cMovementMode_Run - AI2cMovementMode_Walk));
		}

		ioMovementState->temporary_mode = slow_mode;
	}

	AI2rMovementState_ResetAimingVarient(ioCharacter, ioMovementState);
}

// if a sidestep is not found, would we prefer to move forwards or backwards? true = forwards, false = backwards
UUtBool AI2rMovementState_ForwardMovementPreference(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	return ioMovementState->desire_forwards_if_no_sidestep;
}

// sidestep not found, note this and don't try to use it in future
void AI2rMovementState_SidestepNotFound(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, UUtBool inWalking)
{
	if (inWalking) {
		ioMovementState->no_walking_sidestep = UUcTrue;
	} else {
		ioMovementState->no_running_sidestep = UUcTrue;
	}

	if ((ioMovementState->current_direction == AI2cMovementDirection_Sidestep_Left) ||
		(ioMovementState->current_direction == AI2cMovementDirection_Sidestep_Right)) {
		ioMovementState->invalid_movement_direction = UUcTrue;
	}
}


// notify that the character's alert status has changed
void AI2rMovementState_NotifyAlertChange(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	if (ioCharacter->movementOrders.movementMode == AI2cMovementMode_Default)
		AI2rMovementState_ResetAimingVarient(ioCharacter, ioMovementState);
}

// reset the character's aiming variant
void AI2rMovementState_ResetAimingVarient(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	AI2tMovementMode mode = ioCharacter->movementOrders.movementMode;

	if (mode == AI2cMovementMode_Default)
		mode = AI2rMovement_Default(ioCharacter);

	if (ioCharacter->movementOrders.force_aim) {
		// force either aiming or looking depending on the parameters we were given
		// by a higher level of AI code
		ioMovementState->aiming_weapon = ioCharacter->movementOrders.force_withweapon;
	} else {
		// determine aiming/looking status based on movement mode
		switch(mode) {
			
		case AI2cMovementMode_NoAim_Walk:
		case AI2cMovementMode_NoAim_Run:
			ioMovementState->aiming_weapon = UUcFalse;
			break;
			
		case AI2cMovementMode_Creep:
		case AI2cMovementMode_Walk:
		case AI2cMovementMode_Run:
			ioMovementState->aiming_weapon = UUcTrue;
			break;
			
		default:
			UUmAssert(!"AI2rMovementState_ChangeMovementMode: unknown movement mode");
			break;
		}
	}

	ONrCharacter_DisableWeaponVarient(ioCharacter, !ioMovementState->aiming_weapon);
}

// ------------------------------------------------------------------------------------
// -- query interface to high-level pathfinding system

UUtBool AI2rMovementState_IsKeepingMoving(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	return ioMovementState->keepmoving;
}

float AI2rMovementState_GetMoveDirection(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	return ioMovementState->move_direction;
}

UUtBool AI2rMovementState_SimplePath(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	if (ioMovementState->grid_current_point + 1 < ioMovementState->grid_num_points) {
		// we have points to hit after our current point, this is not a simple path
		return UUcFalse;
	}

	return UUcTrue;
}

// ------------------------------------------------------------------------------------
// -- command interface to high-level pathfinding system

// halt (clear all movement)
void AI2rMovementState_Halt(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	ioMovementState->numActiveModifiers = 0;
}

// clear values that need to be reset upon receiving a new path
// this must be called BEFORE check-destination is called for the new path
void AI2rMovementState_NewPathReceived(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	ioMovementState->done_dest_facing = UUcFalse;
	ioMovementState->keepmoving = UUcFalse;
}

// clear values that apply only to our current path point
static void AI2iMovementState_ClearTemporaryFlags(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	ioMovementState->temporarily_slowed = UUcFalse;
	ioMovementState->tried_overshoot = UUcFalse;
	ioMovementState->overshoot_timer = 0;
	ioMovementState->overturn_disable_timer = 0;
	ioMovementState->turning_on_spot = UUcFalse;
}

// clear our pathfinding error information
void AI2rMovementState_NewDestination(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	ioMovementState->numCollisions = 0;
	ioMovementState->numSeriousCollisions = 0;
	ioMovementState->grid_failure = UUcFalse;
#if TOOL_VERSION
	ioMovementState->numBadnessValues = 0;
#endif
}

// reset our path
void AI2rMovementState_ClearPath(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	ioMovementState->grid_last_room = NULL;
	ioMovementState->grid_room = NULL;
	ioMovementState->grid_num_points = 0;
	ioMovementState->grid_current_point = 0;
	ioMovementState->grid_failure = UUcFalse;
	ioMovementState->next_path_point = NULL;
	ioMovementState->collision_stuck = UUcFalse;

	AI2iMovementState_ClearTemporaryFlags(ioCharacter, ioMovementState);
}

static UUtBool AI2iMovementState_CheckPathPointHit(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	float hit_distance;

	if (ioMovementState->next_path_point == NULL) {
		UUmAssert(!"AI2iMovementState_CheckPathPointHit: no next path point!");
		ioMovementState->grid_current_point = 1;
		ioMovementState->next_path_point = ioMovementState->grid_path + ioMovementState->grid_current_point;
		return UUcFalse;
	}

	// update our location relative to our current path point
	MUmVector_Subtract(ioMovementState->vector_to_point, ioMovementState->next_path_point->point, ioCharacter->actual_position);
	ioMovementState->distance_to_path_point_squared = UUmSQR(ioMovementState->vector_to_point.x) + UUmSQR(ioMovementState->vector_to_point.z);

	// how close do we have to get to this point before we can move to the next one?
	hit_distance = AI2iMovementState_GetPointHitDistance(ioCharacter, ioMovementState, ioMovementState->next_path_point);
	if (ioMovementState->distance_to_path_point_squared > UUmSQR(hit_distance)) {
		// not there yet
		return UUcFalse;
	} else {
		// we have hit this point
		return UUcTrue;
	}
}

UUtBool AI2rMovementState_AdvanceThroughGrid(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	// check to see if we are actually still moving
	if ((ioCharacter->pathState.at_finalpoint) || (ioMovementState->grid_num_points == 0)) {
		ioMovementState->grid_num_points = 0;
		ioMovementState->grid_current_point = 0;
		ioMovementState->next_path_point = NULL;
		return UUcFalse;
	}

	while (AI2iMovementState_CheckPathPointHit(ioCharacter, ioMovementState)) {	
		ioMovementState->grid_current_point++;

		if (ioMovementState->grid_current_point >= ioMovementState->grid_num_points) {
			// we hit the end of our grid path.
			ioMovementState->grid_failure = UUcFalse;
			return UUcTrue;

		} else {
			// move to the next point
			AI2iMovementState_ClearTemporaryFlags(ioCharacter, ioMovementState);
			ioMovementState->next_path_point++;
			UUmAssert(ioMovementState->next_path_point == ioMovementState->grid_path + ioMovementState->grid_current_point);
		}
	}

	// not yet done
	return UUcFalse;
}

// update for one game tick
void AI2rMovementState_Update(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	if (ioMovementState->overturn_disable_timer > 0) {
		ioMovementState->overturn_disable_timer -= 1;
	}

	if (ioMovementState->overshoot_timer > 0) {
		ioMovementState->overshoot_timer -= 1;
	}

	// age any wall collisions and recompute their weights
	AI2iMovementState_AgeCollisions(ioCharacter, ioMovementState);

	// calculate how we should be moving this frame
	AI2iMovementState_Calculate_Movement(ioCharacter, ioMovementState);

	// clear our flags in preparation for the next frame's collisions
	AI2iMovementState_ClearCollisionFlags(ioCharacter, ioMovementState);
}

UUtBool AI2rMovementState_DestinationFacing(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	float facing_angle, delta_facing;

	if (ioMovementState->done_dest_facing) {
		// check our destination-facing
		if (ioCharacter->movementOrders.facingMode == AI2cFacingMode_FaceAtDestination) {
			// delay until we reach the right facing
			delta_facing = ioCharacter->facing - ioCharacter->movementOrders.facingData.faceAtDestination.angle;
			UUmTrig_ClipAbsPi(delta_facing);

			return ((float)fabs(delta_facing) < AI2cMovement_DestFacingTolerance);
		} else {
			return UUcTrue;
		}
	}

	// we are at the destination - whatever the outcome, we don't need to check this again
	UUmAssert(ioCharacter->movementOrders.facingMode != AI2cFacingMode_FaceAtDestination);
	ioMovementState->done_dest_facing = UUcTrue;

	if (ioCharacter->pathState.destinationType != AI2cDestinationType_Point)
		return UUcFalse;

	facing_angle = ioCharacter->pathState.destinationData.point_data.final_facing;
	if (facing_angle == AI2cAngle_None)
		return UUcFalse;

	// now that we are at the destination, set up our desired facing
	ioCharacter->movementOrders.facingMode = AI2cFacingMode_FaceAtDestination;
	ioCharacter->movementOrders.facingData.faceAtDestination.angle = facing_angle;
	return UUcFalse;
}

UUtBool AI2rMovementState_CheckFailure(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	AStPathPoint *last_point, *next_point;
	UUtBool in_last_room = UUcFalse;
	UUtBool in_current_room = UUcFalse;
	float path_x, path_z, path_dist_sq, delta_x, delta_z, dist;

	/*
	 * if our collision has gotten wedged against a wall, there is a problem
	 */

	if (ioMovementState->collision_stuck) {
#if AI_VERBOSE_PATHFAILURE
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%s: pathfinding error, collision stuck", ioCharacter->player_name);
#endif
		ioCharacter->pathState.last_pathfinding_error = AI2cError_Pathfinding_CollisionStuck;
		AI2rPath_PathError(ioCharacter);
		return UUcTrue;
	}


	/*
	 * if the AI is in an unexpected room, there is a problem
	 */

	if (NULL != ioMovementState->grid_last_room) {
		in_last_room = PHrPointInRoom(ONgGameState->level->environment, ioMovementState->grid_last_room, &ioCharacter->location);
	}
	if (NULL != ioMovementState->grid_room) {
		in_current_room = PHrPointInRoom(ONgGameState->level->environment, ioMovementState->grid_room, &ioCharacter->location);
	}

	if (!in_last_room && !in_current_room) {
#if AI_VERBOSE_PATHFAILURE
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%s: pathfinding error, outside expected rooms", ioCharacter->player_name);
#endif
		ioCharacter->pathState.last_pathfinding_error = AI2cError_Pathfinding_FellFromRoom;
		AI2rPath_PathError(ioCharacter);
		return UUcTrue;
	}



	/*
	 * if the AI is no longer following its A* path, there is a problem
	 */

	if ((ioMovementState->grid_num_points > 0) && (ioMovementState->grid_current_point > 0) &&
		(ioMovementState->next_path_point != NULL)) {
		UUmAssert(ioMovementState->next_path_point == ioMovementState->grid_path + ioMovementState->grid_current_point);
		next_point = ioMovementState->next_path_point;
		last_point = ioMovementState->next_path_point - 1;

		path_x = next_point->point.x - last_point->point.x;
		path_z = next_point->point.z - last_point->point.z;
		delta_x = ioCharacter->location.x - last_point->point.x;
		delta_z = ioCharacter->location.z - last_point->point.z;
		path_dist_sq = UUmSQR(path_x) + UUmSQR(path_z);
		if (path_dist_sq > 1e-03f) {
			dist = (delta_x * path_x + delta_z * path_z) / path_dist_sq;
			if ((dist < -AI2cMovement_FellFromPath_Range) || (dist > 1.0f + AI2cMovement_FellFromPath_Range)) {
				// we are outside the path in range
				ioCharacter->pathState.last_pathfinding_error = AI2cError_Pathfinding_FellFromPath;
				AI2rPath_PathError(ioCharacter);
				return UUcTrue;
			}

			dist = (delta_x * path_z - delta_z * path_x) / MUrSqrt(path_dist_sq);
			if ((float)fabs(dist) > AI2cMovement_FellFromPath_Dist) {
				// we are outside the path in perpendicular distance
				ioCharacter->pathState.last_pathfinding_error = AI2cError_Pathfinding_FellFromPath;
				AI2rPath_PathError(ioCharacter);
				return UUcTrue;
			}
		}
	}

	// no errors encountered
	return UUcFalse;
}

static UUtBool AI2iMovementState_SetupPath(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
										   PHtNode *inNode, UUtBool inActiveTransition)
{
	UUtBool success;
	PHtRoomData *room;

	if (inNode == NULL) {
		ioCharacter->pathState.last_pathfinding_error = AI2cError_Pathfinding_NoBNVAtStart;
		AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, ioCharacter->pathState.last_pathfinding_error, ioCharacter,
					&ioCharacter->pathState.path_start_location, &ioCharacter->pathState.path_end_location,
					ioCharacter->pathState.path_start_node, ioCharacter->pathState.path_end_node);
		return UUcFalse;
	}

	room = &inNode->location->roomData;
	if (room != ioMovementState->grid_room) {
		ioMovementState->grid_last_room = ioMovementState->grid_room;
		ioMovementState->grid_room = room;
	}

	PHrWorldPinToGridSpace(&ioMovementState->grid_path_start, room);
	PHrWorldPinToGridSpace(&ioMovementState->grid_path_end, room);
	UUmAssert(PHrWorldInGridSpace(&ioMovementState->grid_path_start, room));
	UUmAssert(PHrWorldInGridSpace(&ioMovementState->grid_path_end, room));
	success = ASrPath_SetParams(ASgAstar_Path, inNode, room, &ioMovementState->grid_path_start, &ioMovementState->grid_path_end,
								ioCharacter, ioCharacter->pathState.moving_onto_stairs, (inActiveTransition && AI2gDebug_ShowActivationPaths),
								AI2gDebug_ShowAstarEvaluation);
	
	if (success) {
		success = ASrPath_Generate(ASgAstar_Path, AI2cMax_PathPoints, &ioMovementState->grid_num_points,
									ioMovementState->grid_path, ioCharacter, UUcTrue, (inActiveTransition || ioMovementState->grid_failure));
	}

	if (!success) {
		AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, AI2cError_Pathfinding_AStarFailed, ioCharacter,
			&ioMovementState->grid_path_start, &ioMovementState->grid_path_end, room, inNode);
		AI2rMovementState_ClearPath(ioCharacter, ioMovementState);
		return UUcFalse;
	}

	UUmAssert(ioMovementState->grid_num_points >= 2);

	// we skip the first point, which is the start (where we are now)
	ioMovementState->grid_current_point = 1;
	ioMovementState->next_path_point = &ioMovementState->grid_path[ioMovementState->grid_current_point];
	return UUcTrue;
}

static UUtBool AI2iMovementState_AdjustPathPoint(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, 
												 UUtUns32 inPointIndex, M3tPoint3D *inNewPoint)
{
	UUtBool success;
	float dist_sq;
	M3tPoint3D *cur_point;

	UUmAssert((inPointIndex >= 0) && (inPointIndex < ioMovementState->grid_num_points));
	cur_point = &ioMovementState->grid_path[inPointIndex].point;
	dist_sq = MUmVector_GetDistanceSquared(*cur_point, *inNewPoint);

	if (dist_sq < UUmSQR(AI2cMovementState_Reuse_MinDist)) {
		// we are so close that we can always fudge the point by this much
#if DEBUG_PATH_REUSE
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: adjustpt close (%f < %f), success",
								ONrGameState_GetGameTime(), ioCharacter->player_name, MUrSqrt(dist_sq), AI2cMovementState_Reuse_MinDist);
#endif
		success = UUcTrue;

	} else if (dist_sq > UUmSQR(AI2cMovementState_Reuse_MaxDist)) {
		// we are too far away to even attempt this adjustment
#if DEBUG_PATH_REUSE
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: adjustpt too far (%f > %f), fail",
								ONrGameState_GetGameTime(), ioCharacter->player_name, MUrSqrt(dist_sq), AI2cMovementState_Reuse_MaxDist);
#endif
		success = UUcFalse;

	} else {
		IMtPoint2D old_point, new_point;

		// check to see if we can make this adjustment according to the pathfinding grid
		PHrWorldToGridSpaceDangerous(&old_point.x, &old_point.y, cur_point, ioMovementState->grid_room);
		PHrWorldToGridSpaceDangerous(&new_point.x, &new_point.y, inNewPoint, ioMovementState->grid_room);

		if ((old_point.x < 0) || (old_point.x >= (UUtInt16) ioMovementState->grid_room->gridX) ||
			(old_point.y < 0) || (old_point.y >= (UUtInt16) ioMovementState->grid_room->gridY) ||
			(new_point.x < 0) || (new_point.x >= (UUtInt16) ioMovementState->grid_room->gridX) ||
			(new_point.y < 0) || (new_point.y >= (UUtInt16) ioMovementState->grid_room->gridY)) {
			// one or more of the points lies off the grid; we cannot make this adjustment
#if DEBUG_PATH_REUSE
			COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: adjustpt off grid (%d,%d)->(%d,%d) on %dx%d, fail",
									ONrGameState_GetGameTime(), ioCharacter->player_name, 
									old_point.x, old_point.y, new_point.x, new_point.y, 
									ioMovementState->grid_room->gridX, ioMovementState->grid_room->gridY);
#endif
			success = UUcFalse;
		} else {
			UUtUns32 obstruction_bv[(2 * PHcMaxObstructions) / 32];
			UUtInt32 dummy_stop_x, dummy_stop_y;
			UUtUns8 low_weight, high_weight;
			UUtBool escape_path, path_is_clear;
			PHtNode *cur_pathnode;

			UUrMemory_Clear(obstruction_bv, sizeof(obstruction_bv));
			cur_pathnode = AI2iMovementState_GetCurrentNode(ioCharacter, ioMovementState);

			path_is_clear = PHrLocalPathWeight(ioCharacter, PHcPathMode_DirectedMovement, cur_pathnode,
												ioMovementState->grid_room, obstruction_bv,
												PHcBorder2, 3, old_point.x, old_point.y, new_point.x, new_point.y,
												&escape_path, &low_weight, &high_weight, &dummy_stop_x, &dummy_stop_y);		
			success = (path_is_clear) && (!escape_path);
#if DEBUG_PATH_REUSE
			if (success) {
				COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: adjustpt localpath succeeded",
										ONrGameState_GetGameTime(), ioCharacter->player_name);
			} else {
				COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: adjustpt localpath failed (path %s low %d high %d escape %s)",
										ONrGameState_GetGameTime(), ioCharacter->player_name, (path_is_clear) ? "Y" : "N",
										low_weight, high_weight, (escape_path) ? "Y" : "N");
			}
#endif
		}
	}

	return success;
}

UUtBool AI2rMovementState_MakePath(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, UUtBool inReusePath)
{
	M3tPoint3D current_point;
	PHtNode *node;

	if (ioCharacter->pathState.path_num_nodes == 0) {
		AI2rMovementState_ClearPath(ioCharacter, ioMovementState);
		return UUcTrue;
	}

	ONrCharacter_GetPathfindingLocation(ioCharacter, &current_point);

	if (ioCharacter->pathState.path_current_node + 1 == ioCharacter->pathState.path_num_nodes) {
		// we are going to the destination
		node = ioCharacter->pathState.path_end_node;
		ioMovementState->grid_path_end = ioCharacter->pathState.path_end_location;
	} else {
		// find the point on the exit ghost quad which we are going to
		node = ioCharacter->pathState.path_connections[ioCharacter->pathState.path_current_node]->from;
		AI2rPath_FindGhostWaypoint(ioCharacter, &current_point, ioCharacter->pathState.path_current_node, &ioMovementState->grid_path_end);
	}

	if (ioCharacter->pathState.path_current_node == 0) {
		// we start from the character's current location
		MUmVector_Copy(ioMovementState->grid_path_start, current_point);
	} else {
		// because the character's current location might be a little way outside the BNV, get the closest point
		// on the ghost quad to their location
		PHrWaypointFromGunk(ONgLevel->environment, ioCharacter->pathState.path_connections[ioCharacter->pathState.path_current_node - 1]->gunk,
								&ioMovementState->grid_path_start, &current_point);
	}

#if DEBUG_PATH_REUSE
	if (!inReusePath) {
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: not allowed to reuse grid path",
									ONrGameState_GetGameTime(), ioCharacter->player_name);

	} else if (ioMovementState->grid_num_points == 0) {
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: no grid path to reuse",
									ONrGameState_GetGameTime(), ioCharacter->player_name);

	} else {
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: trying to reuse grid path of %d points",
									ONrGameState_GetGameTime(), ioCharacter->player_name, ioMovementState->grid_num_points);
	}
#endif

	inReusePath = inReusePath && (ioMovementState->grid_num_points > 0);
	inReusePath = inReusePath && AI2iMovementState_AdjustPathPoint(ioCharacter, ioMovementState, 
																	0, &ioMovementState->grid_path_start);
	inReusePath = inReusePath && AI2iMovementState_AdjustPathPoint(ioCharacter, ioMovementState, 
																	ioMovementState->grid_num_points - 1, &ioMovementState->grid_path_end);

	if (inReusePath) {
		// we previously calculated a path that we can reuse
#if DEBUG_PATH_REUSE
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: grid path reuse succeeded",
								ONrGameState_GetGameTime(), ioCharacter->player_name);
#endif
		return UUcTrue;
	} else {
		// we must recalculate
#if DEBUG_PATH_REUSE
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: grid path reuse failed, recalculating",
								ONrGameState_GetGameTime(), ioCharacter->player_name);
#endif
		return AI2iMovementState_SetupPath(ioCharacter, ioMovementState, node, UUcFalse);
	}
}

// ------------------------------------------------------------------------------------
// -- movement routines

static void AI2iMovementState_Calculate_Movement(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	AI2tMovementMode movement_mode, slow_mode;
	AI2tMovementDirection actual_movement_direction, new_movement_direction;
	AI2tMovementModifier *modifier;
	AI2tCollision *collision;
	AI2tAimingMode aim_mode;
	AI2tAimingData *aim_data;
	ONtCharacter *look_character;
	float delta_x, delta_z, distance_to_stop_in;
	float facing_direction, aim_direction, desired_facing_direction, delta_aim, aim_range;
	float facing_distance, starting_dir, standingturn_angle;
	float point_hit_distance, stopping_distance, stopatpoint_radius;
	float theta, costheta, sintheta, distance_fwd, distance_perp, delta_movedir;
	float turning_rate, movement_rate, turning_radius, required_turning_radius, override_amount;
	UUtBool can_change_facing, can_slow_down, check_overturn, turn_danger, disable_sidestep, glancing, currently_aiming;
	UUtBool will_overturn, will_overshoot, check_overshoot, have_weapon, have_twohanded_weapon, disable_walk_backwards, aiming_particle;
	UUtBool in_melee = UUcFalse, panicked = UUcFalse;
	UUtUns32 itr, itr2;
	TRtAnimType cur_anim_type;
	M3tVector3D aim_point, move_vector;

	can_slow_down = UUcFalse;
	if (ioMovementState->temporarily_slowed) {
		// our movement mode has been modified temporarily
		movement_mode = ioMovementState->temporary_mode;
	} else {
		// moving at our normal speed
		movement_mode = ioCharacter->movementOrders.movementMode;

		if (movement_mode == AI2cMovementMode_Default) {
			movement_mode = AI2rMovement_Default(ioCharacter);
		}

		// could we slow down without stopping if we had to?
		if ((!ioCharacter->movementOrders.slowdownDisabled) && (movement_mode >= AI2cMovementMode_NoAim_Run)) {
			can_slow_down = UUcTrue;
			slow_mode = (AI2tMovementMode)(movement_mode - (AI2cMovementMode_Run - AI2cMovementMode_Walk));
		}
	}

	// where are we going?
	if (ioCharacter->pathState.at_finalpoint) {

		if (ioMovementState->keepmoving) {
			// our current direction and move direction will be unchanged from what they were last update.

			// check the keep-moving timer so that we don't just walk off into the ether forever
			if (ioMovementState->keepmoving_timer > 0)
				ioMovementState->keepmoving_timer--;

			if (ioMovementState->keepmoving_timer == 0) {
				// while we were told not to stop at the destination, we've walked for a time beyond it
				// and encountered no further commands.
				ioMovementState->keepmoving = UUcFalse;
				AI2rPath_Halt(ioCharacter);
			}
		} else if (AI2rPath_StopAtDest(ioCharacter, &point_hit_distance)) {
			// stop at the destination
			ioMovementState->current_direction = AI2cMovementDirection_Stopped;
			ioMovementState->move_direction = AI2cAngle_None;

		} else {
			// don't stop at the destination - this must be our first time through. set the keepmoving timer
			ioMovementState->keepmoving = UUcTrue;
			ioMovementState->keepmoving_timer = AI2cPathPoint_KeepMovingFrames;
		}
		ioMovementState->grid_current_point = 0;
		ioMovementState->grid_num_points = 0;
		ioMovementState->next_path_point = NULL;

	} else if (ioMovementState->next_path_point != NULL) {

		UUmAssert(ioMovementState->grid_num_points > 0);
		UUmAssert(ioMovementState->grid_current_point >= 0);
		UUmAssert(ioMovementState->grid_current_point < ioMovementState->grid_num_points);
		UUmAssertReadPtr(ioMovementState->next_path_point, sizeof(AStPathPoint));
		
		// we have somewhere to go, and a way to get there
		ioMovementState->move_direction = MUrATan2(ioMovementState->vector_to_point.x, 
													ioMovementState->vector_to_point.z);
		UUmTrig_ClipLow(ioMovementState->move_direction);

		if (ioMovementState->current_direction == AI2cMovementDirection_Stopped) {
			ioMovementState->current_direction = AI2cMovementDirection_Forward;
		}

		if (ioCharacter->movementOrders.facingMode == AI2cFacingMode_FaceAtDestination) {
			// we're moving - no longer do we have to face towards the angle that we were
			// given at our previous destination. look straight ahead unless otherwise ordered.
			ioCharacter->movementOrders.facingMode = AI2cFacingMode_PathRelative;
			ioCharacter->movementOrders.facingData.pathRelative.direction = AI2cMovementDirection_Forward;
		}

	} else {
		// we are not at our destination and we have no way of getting there. movement modifiers
		// and other layers of code must take over now.
		ioMovementState->current_direction = AI2cMovementDirection_Stopped;
		ioMovementState->move_direction = AI2cAngle_None;
		ioMovementState->grid_current_point = 0;
		ioMovementState->grid_num_points = 0;
		ioMovementState->next_path_point = NULL;
	}


	/* apply movement modifiers as necessary */
	ioMovementState->numActiveModifiers = 0;
	if (ioCharacter->movementOrders.numModifiers > 0) {

		// work out the current movement direction
		if (ioMovementState->move_direction == AI2cAngle_None) {
			MUmVector_Set(move_vector, 0, 0, 0);
		} else {
			MUmVector_Set(move_vector, MUrSin(ioMovementState->move_direction), 0, MUrCos(ioMovementState->move_direction));
		}

		for (itr = 0, modifier = ioCharacter->movementOrders.modifier; itr < ioCharacter->movementOrders.numModifiers; itr++, modifier++) {

			if (ioMovementState->modifiers_require_decreasing) {
				// recalculate the decreased weight of these modifiers
				modifier->decreased_weight = modifier->full_weight;

				if (ioMovementState->numSeriousCollisions > 0) {
					// we must check all modifiers to see if they are disabled or reduced by a serious collision nearby
					// this flag is only set if a serious collision is created or removed, or if a new modifier is added
					// while there are serious collisions active

					// check to see if this modifier is disabled or reduced by a serious collision nearby
					for (itr2 = 0, collision = ioMovementState->collision; itr2 < ioMovementState->numCollisions; itr2++, collision++) {
						if (((collision->flags & AI2cCollisionFlag_Hit) == 0) ||
							((collision->flags & AI2cCollisionFlag_Serious) == 0))
							continue;

						delta_movedir = modifier->direction - collision->direction;
						UUmTrig_ClipAbsPi(delta_movedir);
						delta_movedir = (float)fabs(delta_movedir);

						if (delta_movedir < AI2cMovement_Collision_DisableAngle) {
							// this modifier is disabled
							modifier->decreased_weight = 0;
							break;

						} else if (delta_movedir > AI2cMovement_Collision_DisableMaxAngle) {
							// this modifier is unaffected by this collision, keep looking
							continue;

						} else {
							// reduce the weight of the modifier
							modifier->decreased_weight *= (AI2cMovement_Collision_DisableAngle - delta_movedir) /
								(AI2cMovement_Collision_DisableAngle - AI2cMovement_Collision_DisableMaxAngle);
						}
					}
				}

				if (ioCharacter->movementOrders.modifierAvoidAngle != AI2cAngle_None) {
					// check to see if this modifer is disabled or reduced by something that we are trying to avoid
					delta_movedir = modifier->direction - ioCharacter->movementOrders.modifierAvoidAngle;
					UUmTrig_ClipAbsPi(delta_movedir);
					delta_movedir = (float)fabs(delta_movedir);

					if (delta_movedir < AI2cMovement_Avoid_DisableAngle) {
						// this modifier is disabled
						modifier->decreased_weight = 0;
						break;

					} else if (delta_movedir < AI2cMovement_Avoid_DisableMaxAngle) {
						// reduce the weight of the modifier
						modifier->decreased_weight *= (AI2cMovement_Avoid_DisableAngle - delta_movedir) /
							(AI2cMovement_Avoid_DisableAngle - AI2cMovement_Avoid_DisableMaxAngle);
					}
				}
			} else {
				// use the already-calculated decreased_weight
			}

			if (modifier->decreased_weight > 0) {
				ioMovementState->numActiveModifiers++;

				// apply the movement modifier to our movement direction
				MUmVector_ScaleIncrement(move_vector, modifier->decreased_weight, modifier->unitvector);
			}
		}

		// calculate what direction we should move in
		if (ioMovementState->numActiveModifiers > 0) {
			ioMovementState->move_direction = MUrATan2(move_vector.x, move_vector.z);
			UUmTrig_ClipLow(ioMovementState->move_direction);
		}
	}

	// we are done with modifier updating unless the set of serious collisions changes
	ioMovementState->modifiers_require_decreasing = UUcFalse;

	// we need to move away from collisions if
	//		a) we are trying to move
	//		b) we have modifiers that are being suppressed by the collisions
	if ((ioMovementState->numCollisions > 0) && ((ioMovementState->move_direction != AI2cAngle_None) ||
												 (ioCharacter->movementOrders.numModifiers > ioMovementState->numActiveModifiers))) {
		AI2gMovementState_BadnessCharacter = ioCharacter;

		/* determine which directions we should be moving in by building the badness structure */
		AI2iMovementState_ResetBadness(&AI2gMovementState_NumBadnessValues, AI2cMovementState_MaxBadnessValues, 
										AI2gMovementState_BadnessValues);
		for (itr = 0, collision = ioMovementState->collision; itr < ioMovementState->numCollisions; itr++, collision++) {
			AI2iMovementState_AddBadnessFromCollision(&AI2gMovementState_NumBadnessValues, AI2cMovementState_MaxBadnessValues,
														AI2gMovementState_BadnessValues, collision);
		}

#if TOOL_VERSION
		/* copy the current badness ring into the character for storage */
		ioMovementState->numBadnessValues = AI2gMovementState_NumBadnessValues;
		UUrMemory_MoveFast(AI2gMovementState_BadnessValues, ioMovementState->badnesslist, 
							AI2cMovementState_MaxBadnessValues * sizeof(AI2tBadnessValue));
#endif

		/* traverse the badness ring, looking for the best direction to move in */
		starting_dir = ioMovementState->move_direction;
		if (starting_dir == AI2cAngle_None) {
			starting_dir = ioMovementState->last_real_move_direction;
			if (starting_dir == AI2cAngle_None) {
				starting_dir = ioCharacter->facing;
			}
		}

		ioMovementState->move_direction = AI2iMovementState_LeastBadDirection(&AI2gMovementState_NumBadnessValues, AI2cMovementState_MaxBadnessValues, 
																				AI2gMovementState_BadnessValues, starting_dir);

		AI2gMovementState_BadnessCharacter = NULL;
	}


	/* final movement direction calculation */
	if (ioMovementState->move_direction == AI2cAngle_None) {
		// if we've been told to stop, make sure we do
		ioMovementState->current_direction = AI2cMovementDirection_Stopped;

	} else if (ioMovementState->current_direction == AI2cMovementDirection_Stopped) {
		// if we've been told to move, don't stand still
		ioMovementState->current_direction = AI2cMovementDirection_Forward;

		// store our current desired move direction for future reference, since it is valid
		ioMovementState->last_real_move_direction = ioMovementState->move_direction;
	}

	/* calculate which direction we would normally face in, without any aiming orders */
	can_change_facing = UUcTrue;
	if (ioMovementState->move_direction == AI2cAngle_None) {
		// we are standing still and can face however we want to
		switch(ioCharacter->movementOrders.facingMode) {
		case AI2cFacingMode_None:
		case AI2cFacingMode_PathRelative:
			facing_direction = ioCharacter->facing;
			break;

		case AI2cFacingMode_Angle:
			facing_direction = ioCharacter->movementOrders.facingData.angle.angle;
			break;

		case AI2cFacingMode_FaceAtDestination:
			can_change_facing = UUcFalse;
			facing_direction = ioCharacter->movementOrders.facingData.faceAtDestination.angle;
			break;

		default:
			UUmAssert(!"AI2rMovementState_Calculate_Movement: unknown facing mode");
		}
	} else {
		// we aren't standing still, so unset this field
		ioMovementState->turning_to_face_direction = AI2cAngle_None;
				
		// we are moving and face in a fixed direction based on our current movement direction.
		// however, we may change this movement direction, **UNLESS** we are locked to a particular
		// direction by a PathRelative facing command.
		if (ioCharacter->movementOrders.facingMode == AI2cFacingMode_PathRelative) {
			can_change_facing = UUcFalse;
			ioMovementState->current_direction = ioCharacter->movementOrders.facingData.pathRelative.direction;
		}
		facing_direction = ioMovementState->move_direction -
					AI2cMovementDirection_Offset[ioMovementState->current_direction];
		UUmTrig_ClipLow(facing_direction);
	}


	/* are we in melee or panicked? (this affects our facing decisions) */
	if (ioCharacter->charType == ONcChar_AI2) {
		if (ioCharacter->ai2State.currentGoal == AI2cGoal_Panic) {
			panicked = UUcTrue;
		} else if (ioCharacter->ai2State.currentGoal == AI2cGoal_Combat) {
			in_melee = (ioCharacter->ai2State.currentState->state.combat.maneuver.primary_movement == AI2cPrimaryMovement_Melee);
		}
	}

	/* looking or aiming? */
	if ((ioCharacter->inventory.weapons[0] != NULL) && ((movement_mode != AI2cMovementMode_NoAim_Run) && 
										  (movement_mode != AI2cMovementMode_NoAim_Walk))) {
		currently_aiming = UUcTrue;
	} else {
		currently_aiming = UUcFalse;
	}

	/* calculate which direction we want to aim in */
	if (ioCharacter->movementOrders.glance_timer && (ioCharacter->movementOrders.glance_delay == 0)) {
		glancing = UUcTrue;
		aim_mode = ioCharacter->movementOrders.glanceAimingMode;
		aim_data = &ioCharacter->movementOrders.glanceAimingData;
		turn_danger = ioCharacter->movementOrders.glance_danger;
	} else {
		glancing = UUcFalse;
		aim_mode = ioCharacter->movementOrders.aimingMode;
		aim_data = &ioCharacter->movementOrders.aimingData;
		turn_danger = UUcFalse;
	}

	if ((ioCharacter->charType == ONcChar_AI2) && (!AI2cGoalIsCasual[ioCharacter->ai2State.currentGoal])) {
		// turn much faster when in combat
		turn_danger = UUcTrue;
	}

	switch(aim_mode) {
	case AI2cAimingMode_None:
		aim_direction = AI2cAngle_None;
		AI2rExecutor_AimStop(ioCharacter);
		break;

	case AI2cAimingMode_LookAtPoint:
		delta_x = aim_data->lookAtPoint.point.x - ioCharacter->location.x;
		delta_z = aim_data->lookAtPoint.point.z - ioCharacter->location.z;

		aim_direction = MUrATan2(delta_x, delta_z);
		UUmTrig_ClipLow(aim_direction);

		AI2rExecutor_AimTarget(ioCharacter, &aim_data->lookAtPoint.point, ioMovementState->aiming_weapon);
		break;

	case AI2cAimingMode_LookInDirection:
		aim_direction = MUrATan2(aim_data->lookInDirection.vector.x, 
								aim_data->lookInDirection.vector.z);
		UUmTrig_ClipLow(aim_direction);

		AI2rExecutor_AimDirection(ioCharacter, &aim_data->lookInDirection.vector,
										ioMovementState->aiming_weapon);
		break;

	case AI2cAimingMode_LookAtCharacter:
		look_character = aim_data->lookAtCharacter.character;
		if ((look_character == NULL) || ((look_character->flags & ONcCharacterFlag_InUse) == 0)) {
			// no character to look at (?)
			aim_direction = facing_direction;
			AI2rExecutor_AimStop(ioCharacter);
		} else {
			aim_point = look_character->location;
			aim_point.y += look_character->heightThisFrame;
			delta_x = look_character->location.x - ioCharacter->location.x;
			delta_z = look_character->location.z - ioCharacter->location.z;

			aim_direction = MUrATan2(delta_x, delta_z);
			UUmTrig_ClipLow(aim_direction);

			AI2rExecutor_AimTarget(ioCharacter, &aim_point, ioMovementState->aiming_weapon);
		}
		break;

	default:
		UUmAssert(!"AI2rMovementState_Calculate_Movement: unknown aiming mode");
	}


	// check to see if we want to face in a different direction.
	// don't change facing if we are very close to the destination
	if (can_change_facing) {
		desired_facing_direction = AI2cAngle_None;

		// work out whether we can walk sideways
		switch (AI2rExecutor_ActualMovementMode(ioCharacter)) {
			case AI2cMovementMode_Creep:
			case AI2cMovementMode_NoAim_Walk:
			case AI2cMovementMode_Walk:
				disable_sidestep = ioMovementState->no_walking_sidestep;
			break;
			
			case AI2cMovementMode_NoAim_Run:
			case AI2cMovementMode_Run:
				disable_sidestep = ioMovementState->no_running_sidestep;
			break;

			default:
				UUmAssert(!"invalid actual-movement-mode from executor");
			break;
		}

		// if we have no sidestep animations then we must rely on forwards and backwards. but otherwise,
		// don't use backwards to reach path points, because it looks bad.
		disable_walk_backwards = !disable_sidestep;

		if (ioMovementState->move_direction == AI2cAngle_None) {
			// we are stationary and face in our aiming direction
			desired_facing_direction = aim_direction;
			disable_walk_backwards = UUcFalse;

		} else if (glancing) {
			// never change our movement direction while moving just to glance
			desired_facing_direction = ioMovementState->move_direction;

		} else if (ioCharacter->pathState.destinationType == AI2cDestinationType_None) {
			// we are moving but have no destination, we are controlled completely by
			// movement modifiers. face in our aiming direction.
			desired_facing_direction = aim_direction;
			disable_walk_backwards = UUcFalse;

		} else if (ioCharacter->pathState.distance_from_start_to_dest_squared > UUmSQR(AI2cPathPoint_WalkSidewaysAllowDist)) {
			// we are travelling a long way, face along our path unless we're looking at a character
			desired_facing_direction = (aim_mode == AI2cAimingMode_LookAtCharacter) ? aim_direction : ioMovementState->move_direction;

		} else if ((ioCharacter->pathState.destinationType == AI2cDestinationType_Point) &&
					(ioCharacter->pathState.destinationData.point_data.final_facing != AI2cAngle_None)) {
			// face to our destination point's facing
			desired_facing_direction = ioCharacter->pathState.destinationData.point_data.final_facing;

		} else {
			// face in our current direction
			desired_facing_direction = ioCharacter->facing;
		}

		if (disable_sidestep) {
			// work out whether forwards or backwards would be better
			delta_aim = desired_facing_direction - ioMovementState->move_direction;
			UUmTrig_ClipAbsPi(delta_aim);
			ioMovementState->desire_forwards_if_no_sidestep = ((float)fabs(delta_aim) < M3cHalfPi);
		}

		if (desired_facing_direction == AI2cAngle_None) {
			if (ioMovementState->move_direction != AI2cAngle_None) {
				// we'd rather be walking forward, given the opportunity
				ioMovementState->current_direction = AI2cMovementDirection_Forward;
			}
		} else {
			/* determine whether we need to change facing direction in order to keep aiming where we are */
			
			if (ioMovementState->move_direction == AI2cAngle_None) {
				// we aren't trying to move, so unset this field
				ioMovementState->turning_on_spot = UUcFalse;
				
				if (ioMovementState->turning_to_face_direction != AI2cAngle_None) {
					// if we're already turning to a particular direction, then stay
					// with those orders if we can
					facing_direction = ioMovementState->turning_to_face_direction;
				}
				
				// how well will our current facing direction satisfy our aiming?
				delta_aim = desired_facing_direction - facing_direction;
				UUmTrig_Clip(delta_aim);
				
				cur_anim_type = ONrCharacter_GetAnimType(ioCharacter);
				switch(cur_anim_type) {
					case ONcAnimType_Muro_Thunderbolt:
					case ONcAnimType_Ninja_Fireball:
						aiming_particle = UUcTrue;
					break;

					default:
						aiming_particle = UUcFalse;
					break;
				}

				if (aiming_particle) {
					// facing range is very small so that we aim the particles from our animation
					// in the right direction
					aim_range = AI2cMovement_AnimParticleAimRange;

				} else if (in_melee) {
					// facing range is deliberately made small so that we face directly at target
					// and can block if necessary
					aim_range = AI2cMovement_MeleeAimRange;

				} else if (panicked) {
					// keep a close watch on our attacker
					aim_range = AI2cMovement_PanicAimRange;

				} else if ((ioCharacter->charType == ONcChar_AI2) && (ioCharacter->ai2State.currentGoal == AI2cGoal_Neutral)) {
					// we are engaged in a neutral interaction, it's important that we face exactly at the target so that our
					// animations make sense
					aim_range = AI2cMovement_NeutralInteractionAimRange;

				} else if (glancing && !currently_aiming) {
					// we can turn our head further without turning our body, because we aren't holding a gun
					aim_range = AI2cMovement_StandingGlanceRange;

				} else if ((ioCharacter->charType == ONcChar_AI2) && (ioCharacter->ai2State.currentGoal == AI2cGoal_Pursuit)) {
					// we aren't really stationary, we are kinda moving, so increase our aim
					// range accordingly
					aim_range = AI2cMovement_MovingAimRange;

				} else {
					aim_range = AI2cMovement_StandingLookRange;
				}
				
				// if this isn't satisfied, turn to where we're aiming.
				if ((delta_aim > aim_range) && (delta_aim < M3c2Pi - aim_range)) {
					facing_direction = desired_facing_direction;
					ioMovementState->turning_to_face_direction = facing_direction;
				}
			} else {
				// calculate which direction we want to aim in relative to our currently planned facing direction
				delta_aim = desired_facing_direction - facing_direction;
				UUmTrig_ClipAbsPi(delta_aim);
				
				if (in_melee && !disable_sidestep) {
					// facing range is deliberately made small so that we face directly at target
					// and can block if necessary
					aim_range = AI2cMovement_MeleeAimRange;
				} else if (currently_aiming) {
					// we have to aim at someone - don't let them get too far out of our cone of view
					aim_range = AI2cMovement_MovingAimRange;
				} else {
					// we are just looking, we can turn our head more
					aim_range = AI2cMovement_MovingLookRange;
				}

				if ((ioMovementState->invalid_movement_direction) || ((float)fabs(delta_aim) > aim_range)) {
					// we are trying to aim at a target that is outside our field of view, or
					// we don't have the required animation to move in our current direction.

					new_movement_direction = AI2iMovementState_FindMovementDirection(desired_facing_direction,
								ioMovementState->move_direction, disable_sidestep, disable_walk_backwards);
/*					COrConsole_Printf("try new direction (facing %f desired %f -> delta %f, invalid %s), found %s from (desire %f, move %f) [was %s]",
						facing_direction, desired_facing_direction, delta_aim,
						(ioMovementState->invalid_movement_direction) ? "yes" : "no", AI2cMovementDirectionName[new_movement_direction], 
						desired_facing_direction, ioMovementState->move_direction, AI2cMovementDirectionName[ioMovementState->current_direction]);*/

					if (new_movement_direction != ioMovementState->current_direction) {
						// we've changed direction, so disable overturn correction for a while
						ioMovementState->overturn_disable_timer = AI2cOverturnDisableFrames;
						ioMovementState->current_direction = new_movement_direction;
					}

					ioMovementState->invalid_movement_direction = UUcFalse;
				}
				
				// update our facing direction for this new movement direction
				facing_direction = ioMovementState->move_direction -
					AI2cMovementDirection_Offset[ioMovementState->current_direction];
				UUmTrig_ClipLow(facing_direction);
			}
		}
	}

	// don't move if we are turning on the spot
	if (ioMovementState->turning_on_spot) {
		actual_movement_direction = AI2cMovementDirection_Stopped;
	} else {
		actual_movement_direction = ioMovementState->current_direction;
	}

	// stop if we would be moving in the wrong direction
	if (ioMovementState->move_direction != AI2cAngle_None) {

		facing_distance = ioCharacter->facing - (facing_direction + ioCharacter->facingModifier);
		if (facing_distance > M3cPi) {
			facing_distance -= M3c2Pi;
		} else if (facing_distance < -M3cPi) {
			facing_distance += M3c2Pi;
		}
		facing_distance = (float)fabs(facing_distance);

		if (ioMovementState->numCollisions > 0) {
			standingturn_angle = AI2cFacingDistance_CollisionForceStationaryTurn;

		} else if (movement_mode >= AI2cMovementMode_NoAim_Run) {
			standingturn_angle = AI2cFacingDistance_RunningForceStationaryTurn;

		} else  {
			standingturn_angle = AI2cFacingDistance_WalkingForceStationaryTurn;
		}

		if (facing_distance > standingturn_angle) {
			// don't run, we are heading in the wrong direction
			actual_movement_direction = AI2cMovementDirection_Stopped;

		} else if (facing_distance < AI2cFacingDistance_ReturnFromSlowDown) {
			// we are close to where we have to be facing. If we are slowed down or
			// turning on the spot, we can stop now.
			ioMovementState->temporarily_slowed = UUcFalse;
			ioMovementState->turning_on_spot = UUcFalse;
		}

		// if we're moving, make sure that we don't run in circles around the point that we're aiming for,
		// or overshoot it. see if we need to slow down or stop.
		if ((actual_movement_direction != AI2cMovementDirection_Stopped) &&
			(ioMovementState->next_path_point != NULL)) {

			// work out if we need to check overturn
			check_overturn = (ioMovementState->overturn_disable_timer == 0);

			// from our facing, determine vectors that are forward and perpendicular to direction of travel
			//
			// theta = facing, adjusted for sidestepping or backpedalling to give direction of travel
			// forward = (sin, 0, cos)
			// right = (cos, 0, -sin)
			//
			// use these to determine our forward and perpendicular distance to the path point.
			theta = ioCharacter->facing - AI2cMovementDirection_Offset[ioMovementState->current_direction];
			UUmTrig_ClipLow(theta);
			
			costheta = MUrCos(theta); sintheta = MUrSin(theta);
			
			distance_fwd  = (float)fabs(sintheta * ioMovementState->vector_to_point.x
				+ costheta * ioMovementState->vector_to_point.z);
			distance_perp = (float)fabs(costheta * ioMovementState->vector_to_point.x
				- sintheta * ioMovementState->vector_to_point.z);
			
			// only execute this code if it's relevant
			will_overturn = UUcFalse;
			if (check_overturn) {
				// how closely must we hit this point?
				point_hit_distance = AI2iMovementState_GetPointHitDistance(ioCharacter, ioMovementState, ioMovementState->next_path_point);
								
				// what turning radius is required to hit this point with the desired precision?
				if (distance_perp - point_hit_distance > 0.1f) {
					distance_perp -= point_hit_distance;
					
					// trigonometry tells us that r = (perp^2 + fwd^2) / 2 * perp
					required_turning_radius = (UUmSQR(distance_perp) + UUmSQR(distance_fwd)) / (2.0f * distance_perp);
				} else {
					// we are already on target to hit the point within the point hit distance, don't worry about it
					required_turning_radius = 1000.0f;
				}
				
				// what is our current turning radius?
				if ((ioCharacter->inventory.weapons[0] == NULL) || (!currently_aiming)) {
					have_weapon = UUcFalse;
					have_twohanded_weapon = UUcFalse;
				} else {
					have_weapon = UUcTrue;
					have_twohanded_weapon = (WPrGetClass(ioCharacter->inventory.weapons[0])->flags & WPcWeaponClassFlag_TwoHanded) > 0;
				}
				turning_rate = AI2rExecutor_GetMaxDeltaFacingPerFrame(ioCharacter->characterClass, movement_mode, turn_danger);
				movement_rate = ONrCharacterClass_MovementSpeed(ioCharacter->characterClass, ioMovementState->current_direction,
																movement_mode, have_weapon, have_twohanded_weapon);
				turning_radius = movement_rate / turning_rate;
				
				// can we make it to the point?
				will_overturn = (turning_radius > AI2cTurningRadius_SafetyMargin * required_turning_radius);				
			}
			
			// work out if we need to check overshoot
			will_overshoot = UUcFalse;
			check_overshoot = AI2iMovementState_StopAtPathPoint(ioCharacter, ioMovementState, &stopatpoint_radius);
			if (check_overshoot) {
				if (ioMovementState->tried_overshoot) {
					// can't pause again for this flag
					check_overshoot = UUcFalse;

				} else if (distance_fwd < distance_perp) {
					// don't bother checking overshoot unless we are pointing roughly towards the target. this is important
					// as otherwise we might be roughly perpendicular to it -> distance_fwd is almost zero and we might
					// get a false overshoot.
					check_overshoot = UUcFalse;

				} else if (!ONrCharacter_IsKeepingMoving(ioCharacter)) {
					// we aren't playing an animation that ends in a moving state... thus we are either already
					// stopped or stopping.
					check_overshoot = UUcFalse;
				}

				if (check_overshoot) {
					// what is our current stopping distance?
					stopping_distance = ONrCharacterClass_StoppingDistance(ioCharacter->characterClass, ioMovementState->current_direction,
																			movement_mode);
					distance_to_stop_in = AI2cStoppingDistance_SafetyMargin * (distance_fwd + stopatpoint_radius);
			
					will_overshoot = (stopping_distance > distance_to_stop_in);
				}
			}
						
			if ((will_overturn || will_overshoot) && can_slow_down) {
				// could we make it to the point if walking?
				if (check_overturn) {
					turning_rate = AI2rExecutor_GetMaxDeltaFacingPerFrame(ioCharacter->characterClass, slow_mode, turn_danger);
					movement_rate = ONrCharacterClass_MovementSpeed(ioCharacter->characterClass, ioMovementState->current_direction,
						slow_mode, have_weapon, have_twohanded_weapon);
					turning_radius = movement_rate / turning_rate;
					will_overturn = (turning_radius > AI2cTurningRadius_SafetyMargin * required_turning_radius);
				}
				
				if (check_overshoot) {
					// what is our current stopping distance?
					stopping_distance = ONrCharacterClass_StoppingDistance(ioCharacter->characterClass, ioMovementState->current_direction, slow_mode);			
					will_overshoot = (stopping_distance > distance_to_stop_in);
				}
				
				if (!(will_overturn || will_overshoot)) {
					// slow to a walking pace until we reach desired facing, or hit this point successfully
					ioMovementState->temporarily_slowed = UUcTrue;
					ioMovementState->temporary_mode = movement_mode = slow_mode;
				}
			}
			
			if (will_overturn) {
				// stop and turn on the spot until we reach 'close enough' to the desired facing
				ioMovementState->turning_on_spot = UUcTrue;
				actual_movement_direction = AI2cMovementDirection_Stopped;
			} else if (will_overshoot) {
				// stop so we don't overshoot
				ioMovementState->tried_overshoot = UUcTrue;
				ioMovementState->overshoot_timer = AI2cStoppingDistance_HaltFrames;
			}

			// work out if we might need to override our normal turning radius
			if (!ioMovementState->turning_on_spot) {
				override_amount = 1.0f - (ioMovementState->distance_to_path_point_squared / UUmSQR(AI2cPathPoint_OverrideTurnDistance));
				if (override_amount > 0) {
					// we are very close to the point, and we're still moving.
					// override our normal turning speed in order to make absolutely sure that we hit this point.
					UUmAssert(override_amount <= 1.0f);
					AI2rExecutor_TurnOverride(ioCharacter, override_amount * M3cHalfPi);
				}
			}
		}
	
		// stop so we don't overshoot
		if (ioMovementState->overshoot_timer) {
			actual_movement_direction = AI2cMovementDirection_Stopped;
		}
	}

	// commit our choices
	AI2rExecutor_Move(ioCharacter, facing_direction, movement_mode, actual_movement_direction, turn_danger);
}

static UUcInline UUtBool AI2iMovementState_MovementDirectionDisabled(AI2tMovementDirection inDirection,
										UUtBool inDisableSidestep, UUtBool inDisableBackwards)
{
	switch(inDirection) {
	case AI2cMovementDirection_Forward:
		return UUcFalse;

	case AI2cMovementDirection_Backpedal:
		return inDisableBackwards;

	case AI2cMovementDirection_Sidestep_Left:
	case AI2cMovementDirection_Sidestep_Right:
		return inDisableSidestep;

	default:
		UUmAssert(!"AI2iMovementState_MovementDirectionDisabled: bad movement direction");
		return UUcTrue;
	}
}

static AI2tMovementDirection AI2iMovementState_FindMovementDirection(float inDesiredFacing, float inMoveDirection,
																	UUtBool inDisableSidestep, UUtBool inDisableBackwards)
{
	float delta, abs_delta;

	delta = inDesiredFacing - inMoveDirection;
	UUmTrig_ClipAbsPi(delta);
	abs_delta = (float)fabs(delta);

	if (inDisableSidestep) {
		if (inDisableBackwards) {
			// we can only move forwards
			return AI2cMovementDirection_Forward;
		}

		if (abs_delta < M3cHalfPi) {
			return AI2cMovementDirection_Forward;
		} else {
			return AI2cMovementDirection_Backpedal;
		}
	} else {
		// we must consider the possibility of sidestepping
		if (abs_delta < M3cQuarterPi) {
			// we want to look in the quadrant that is directly in front of us
			return AI2cMovementDirection_Forward;

		} else if ((abs_delta > M3cHalfPi + M3cQuarterPi) && (!inDisableBackwards)) {
			// we want to look in the quadrant that is directly behind us
			return AI2cMovementDirection_Backpedal;

		} else if (delta > 0) {
			return AI2cMovementDirection_Sidestep_Right;

		} else {
			return AI2cMovementDirection_Sidestep_Left;
		}
	}
}

static UUtBool AI2iMovementState_FinalPathPoint(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	if (ioMovementState->grid_current_point < ioMovementState->grid_num_points - 1)
		return UUcFalse;
	
	return (ioCharacter->pathState.path_current_node == ioCharacter->pathState.path_num_nodes - 1);
}

static UUtBool AI2iMovementState_StopAtPathPoint(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, float *outRadius)
{
	// only consider stopping at destination
	if (!AI2iMovementState_FinalPathPoint(ioCharacter, ioMovementState))
		return UUcFalse;
	
	return AI2rPath_StopAtDest(ioCharacter, outRadius);
}
												
static float AI2iMovementState_GetPointHitDistance(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, AStPathPoint *inPathPoint)
{
	float stopping_tolerance, hit_tolerance;

	// if we've previously failed a grid path on this grid, be very careful
	if (ioMovementState->grid_failure) {
		hit_tolerance = AI2cPathPoint_Exact_Hit;
	} else {
		// calculate how closely we have to hit this point
		switch(inPathPoint->type) {
		case AScPathPoint_Clear:
			// empty space, no obstacles nearby
			hit_tolerance = AI2cPathPoint_Free_Hit;
			break;

		case AScPathPoint_Tight:
			// there are obstacles or danger nearby, follow path
			hit_tolerance = AI2cPathPoint_Tight_Hit;
			break;

		case AScPathPoint_Danger:
		case AScPathPoint_Impassable:
			// no margin for error, there is danger very close
			hit_tolerance = AI2cPathPoint_Exact_Hit;
			break;

		default:
			UUmAssert(!"AI2iMovementState_GetPointHitDistance: unknown path point type!");
			hit_tolerance = AI2cPathPoint_Exact_Hit;
			break;
		}
	}

	if (AI2iMovementState_FinalPathPoint(ioCharacter, ioMovementState)) {
		// make sure we try to hit within our destination tolerance!
		if (AI2rPath_StopAtDest(ioCharacter, &stopping_tolerance)) {
			hit_tolerance = UUmMin(hit_tolerance, stopping_tolerance);
		}
		hit_tolerance = UUmMax(hit_tolerance, AI2cPathPoint_Exact_Hit);
	}

	return hit_tolerance;
}

// ------------------------------------------------------------------------------------
// -- collision avoidance

static void AI2iMovementState_ResetBadness(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues)
{
	UUmAssert(inMaxNumValues >= 2);

	*ioNumValues = 2;

	// no badness anywhere
	ioValues[0].angle = 0;
	ioValues[0].value = 0;
	ioValues[0].next_index = 1;

	ioValues[1].angle = M3c2Pi;
	ioValues[1].value = 0;
	ioValues[1].next_index = 0;
}

static void AI2iMovementState_AddBadnessFromCollision(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues,
													  AI2tCollision *inCollision)
{
	float badness_begin, badness_begincap, badness_endcap, badness_end;
	float badness;
	AI2tBadnessValue *ptr;

	if (UUmFloat_CompareEqu(inCollision->weight, 0))
		return;

	// work out the angles at which our badness is determined
	badness_begin = inCollision->direction - AI2cMovement_Collision_BadnessMaxAngle;
	UUmTrig_ClipLow(badness_begin);

	badness_begincap = inCollision->direction - AI2cMovement_Collision_BadnessAngle;
	UUmTrig_ClipLow(badness_begincap);

	badness_endcap = inCollision->direction + AI2cMovement_Collision_BadnessAngle;
	UUmTrig_ClipHigh(badness_endcap);

	badness_end = inCollision->direction + AI2cMovement_Collision_BadnessMaxAngle;
	UUmTrig_ClipHigh(badness_end);

	badness = inCollision->weight;

	// write the badness graph into the list
	ptr = ioValues;
	ptr = AI2iMovementState_CreateBadnessValue(ioNumValues, inMaxNumValues, ioValues, ptr, badness_begin, 0, 0);
	ptr = AI2iMovementState_CreateBadnessValue(ioNumValues, inMaxNumValues, ioValues, ptr, badness_begincap, 0, badness);
	ptr = AI2iMovementState_CreateBadnessValue(ioNumValues, inMaxNumValues, ioValues, ptr, badness_endcap, badness, badness);
	ptr = AI2iMovementState_CreateBadnessValue(ioNumValues, inMaxNumValues, ioValues, ptr, badness_end, badness, 0);
	AI2iMovementState_AddBadnessIncrements(ioNumValues, inMaxNumValues, ioValues);
}

static AI2tBadnessValue *AI2iMovementState_CreateBadnessValue(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues,
															  AI2tBadnessValue *inStartValue, float inEndAngle,
																float inStartIncrement, float inEndIncrement)
{
	AI2tBadnessValue *ptr, *nextptr, *newptr;
	float incr_delta, start_angle, angle_range, angle_remaining, interp;
	UUtBool wrap_before_comparing;

	ptr = inStartValue;
	UUmAssertReadPtr(ptr, sizeof(AI2tBadnessValue));

	start_angle = ptr->angle;
	wrap_before_comparing = (start_angle > inEndAngle);

	if (wrap_before_comparing) {
		angle_range = M3c2Pi + inEndAngle - start_angle;
	} else {
		angle_range = inEndAngle - start_angle;
	}
	incr_delta = inEndIncrement - inStartIncrement;

	do {
		if (UUmFloat_CompareEqu(ptr->angle, inEndAngle)) {
			// there is already a point with the desired angle. stop.
			ptr->val_increment = inEndIncrement;

			return ptr;
		}

		if (wrap_before_comparing) {
			angle_remaining = M3c2Pi + inEndAngle - ptr->angle;
		} else {
			angle_remaining = inEndAngle - ptr->angle;
		}

		// increment the badness value at this point by the correct amount - linear interpolation
		// between the endpoints of the piecewise linear function
		ptr->val_increment = inEndIncrement - incr_delta * angle_remaining / angle_range;

		UUmAssert((ptr->next_index >= 0) && (ptr->next_index < inMaxNumValues));
		nextptr = &ioValues[ptr->next_index];

		if (wrap_before_comparing) {
			if (nextptr->angle < inEndAngle) {
				// we have wrapped around and can start really comparing
				wrap_before_comparing = UUcFalse;
			}
		} else {
			if (nextptr->angle > inEndAngle) {
				// the desired end point lies within this interval. create it!
				newptr = AI2iMovementState_NewBadnessValue(ioNumValues, inMaxNumValues, ioValues, ptr);
				if (newptr == NULL) {
					return ptr;
				}

				// work out what the current value at this point is according to the pair of
				// points that presently bracket it
				UUmAssert((inEndAngle >= ptr->angle) && (inEndAngle <= nextptr->angle));
				interp = (inEndAngle - ptr->angle) / (nextptr->angle - ptr->angle);
				newptr->value = ptr->value + interp * (nextptr->value - ptr->value);

				// set up the point according to our input parameters
				newptr->angle = inEndAngle;
				newptr->val_increment = inEndIncrement;

				return newptr;
			}
		}

		ptr = nextptr;

	} while (ptr != inStartValue);

	// we have wrapped all the way around the badness list!
	UUmAssert(ptr != inStartValue);
	return ptr;
}

static void AI2iMovementState_AddBadnessIncrements(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues)
{
	UUtUns32 itr;
	AI2tBadnessValue *ptr;

	for (itr = 0, ptr = ioValues; itr < *ioNumValues; itr++, ptr++) {
		ptr->value += ptr->val_increment;
		ptr->val_increment = 0;
	}
}

static AI2tBadnessValue *AI2iMovementState_NewBadnessValue(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues, 
														   AI2tBadnessValue *inInsertPtr)
{
	AI2tBadnessValue *newptr;
	UUtUns32 next_index;

	if (*ioNumValues >= inMaxNumValues) {
		// exceeded fixed-size array
		AI2_ERROR(AI2cBug, AI2cSubsystem_Movement, AI2cError_Movement_MaxBadnessValues,
				AI2gMovementState_BadnessCharacter, *ioNumValues, inMaxNumValues, 0, 0);
		return NULL;
	}

	next_index = inInsertPtr->next_index;

	UUmAssertReadPtr(inInsertPtr, sizeof(AI2tBadnessValue));
	UUmAssert((next_index >= 0) && (next_index < *ioNumValues));

	inInsertPtr->next_index = (*ioNumValues)++;
	newptr = &ioValues[inInsertPtr->next_index];

	newptr->next_index = next_index;
	newptr->val_increment = 0;

	return newptr;
}

static float AI2iMovementState_LeastBadDirection(UUtUns32 *ioNumValues, UUtUns32 inMaxNumValues, AI2tBadnessValue *ioValues,
												 float inDirection)
{
	AI2tBadnessValue *ptr, *nextptr;
	UUtUns32 next_index;
	float delta_theta, this_value, least_bad_value, least_bad_angle;

	least_bad_angle = 0;
	least_bad_value = 1e09;
	ptr = ioValues;

	// insert a point into the badness ring exactly where the current direction is - just so that we have a point
	// to check at
	AI2iMovementState_CreateBadnessValue(ioNumValues, inMaxNumValues, ioValues, ptr, inDirection, 0, 0);

	while (1) {
		next_index = ptr->next_index;
		if (next_index == 0)
			break;

		UUmAssert((next_index >= 0) && (next_index < *ioNumValues));
		nextptr = &ioValues[next_index];

		// compute the heuristic for the current point on the badness ring
		delta_theta = ptr->angle - inDirection;
		UUmTrig_ClipAbsPi(delta_theta);
		this_value = ptr->value + AI2cMovement_Collision_HeuristicTheta * (float)fabs(delta_theta);
		if (this_value < least_bad_value) {
			least_bad_value = this_value;
			least_bad_angle = ptr->angle;
		}

		ptr = nextptr;
	}

	return least_bad_angle;
}

void AI2iMovementState_AgeCollisions(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	UUtUns32 itr, prev_serious, cur_serious;
	M3tPoint3D current_pt, delta_pos;
	float forget_dist, cyl_dist, cyl_radius, current_dir, forget_dist_weight, forget_time_weight, urgency;
	AI2tCollision *collision;

	ONrCharacter_GetPathfindingLocation(ioCharacter, &current_pt);

	ioMovementState->numSeriousCollisions = 0;
	for (itr = 0, collision = ioMovementState->collision; itr < ioMovementState->numCollisions; ) {
		// if the hit flag wasn't set during the character movement phase last frame, set the miss flag
		// and start the timer
		if ((collision->flags & (AI2cCollisionFlag_Hit | AI2cCollisionFlag_Miss)) == 0) {
			collision->flags |= AI2cCollisionFlag_Miss;
			collision->frame_counter = 0;
		}
		collision->frame_counter += 1;

		// store the value of the serious flag and clear it
		prev_serious = (collision->flags & AI2cCollisionFlag_Serious);

		if (collision->flags & AI2cCollisionFlag_Hit) {
			// don't forget about this collision, we're still hitting the wall
			forget_dist_weight = 1.0f;
			forget_time_weight = 1.0f;
		} else {
			// get distance away from the last hit point
			MUmVector_Subtract(delta_pos, current_pt, collision->last_hit_point);
			forget_dist = MUmVector_GetLengthSquared(delta_pos);

			if (forget_dist > UUmSQR(AI2cMovement_Collision_TotallyForgetDistance)) {
				// forget about this collision, we have moved a long distance
				collision->flags &= ~(AI2cCollisionFlag_Serious | AI2cCollisionFlag_Hit | AI2cCollisionFlag_Miss);
				goto endloop;

			} else if (forget_dist > UUmSQR(AI2cMovement_Collision_StartForgetDistance)) {
				// we have moved away from the last point that we collided at; start to forget about this collision
				forget_dist_weight = (AI2cMovement_Collision_TotallyForgetDistance - MUrSqrt(forget_dist)) /
									 (AI2cMovement_Collision_TotallyForgetDistance - AI2cMovement_Collision_StartForgetDistance);

			} else {
				// we are still close to the last location of our collision
				forget_dist_weight = 1.0f;
			}

			if (collision->frame_counter >= AI2cMovement_Collision_TotallyForgetFrames) {
				// forget about this collision, it has been a long time
				collision->flags &= ~(AI2cCollisionFlag_Serious | AI2cCollisionFlag_Hit | AI2cCollisionFlag_Miss);
				goto endloop;

			} else if (collision->frame_counter >= AI2cMovement_Collision_StartForgetFrames) {
				// it has been a while since we last collided; start to forget about this collision
				forget_time_weight = (AI2cMovement_Collision_TotallyForgetFrames - (float) collision->frame_counter) /
									 (AI2cMovement_Collision_TotallyForgetFrames - AI2cMovement_Collision_StartForgetFrames);

			} else {
				// it hasn't been that long since our collision
				forget_time_weight = 1.0f;
			}
		}

		if (collision->total_hit_frames <= AI2cMovement_Collision_IgnoreFrames) {
			// this collision is not yet important enough to worry about
			urgency = 0;
		} else {
			if (collision->flags & AI2cCollisionFlag_Hit) {
				collision->flags |= AI2cCollisionFlag_Serious;
			}

			if (collision->total_hit_frames < AI2cMovement_Collision_MaxUrgencyFrames) {
				// this collision has some urgency
				urgency = (AI2cMovement_Collision_MaxUrgencyWeight *
							(collision->total_hit_frames - AI2cMovement_Collision_IgnoreFrames)) /
							(AI2cMovement_Collision_MaxUrgencyFrames - AI2cMovement_Collision_IgnoreFrames);

			} else {
				// this collision has a lot of urgency
				urgency = AI2cMovement_Collision_MaxUrgencyWeight;

				if (collision->flags & AI2cCollisionFlag_Hit) {
					collision->flags |= AI2cCollisionFlag_Serious;

					if (collision->total_hit_frames >= AI2cMovement_Collision_ErrorFrames) {
						// we have bumped against this wall a lot... our pathfinding has critically failed.
						// throw a pathfinding error next time we are in CheckFailure()
						ioMovementState->collision_stuck = UUcTrue;
					}
				}
			}
		}

		collision->weight = urgency * forget_dist_weight * forget_time_weight;

		if ((collision->weight > 0) && ((collision->flags & AI2cCollisionFlag_Hit) == 0)) {
			// how far are we currently perpendicularly away from the wall? note that we only use our XZ movement
			// to check, so this approximates cylindrical distance
			cyl_dist = current_pt.x * collision->collision_plane.a + collision->last_collision_point.y * collision->collision_plane.b
					+ current_pt.z * collision->collision_plane.c + collision->collision_plane.d;
			cyl_radius = ONrCharacter_GetLeafSphereRadius(ioCharacter);

			if (cyl_dist < -AI2cMovement_Collision_InsideForgetDistance) {
				// we are actually 'inside' the wall. it is obviously no longer relevant since we got here!
				// delete this collision and move another element down into its place if appropriate.
				collision->flags &= ~(AI2cCollisionFlag_Serious | AI2cCollisionFlag_Hit | AI2cCollisionFlag_Miss);
				goto endloop;
			} else {
				if (cyl_dist > 0) {
					// we are touching the wall but still outside it.
					cyl_dist = (cyl_dist - cyl_radius) / AI2cMovement_Collision_ComfortDistance;

					if (cyl_dist > 1.0f) {
						// we are sufficiently far away that this collision doesn't matter any more
						collision->weight = 0;
					} else if (cyl_dist > 0) {
						// we are not actually touching the wall... however we are still near it.
						current_dir = ioMovementState->move_direction;
						if (current_dir == AI2cAngle_None) {
							// we are currently stationary. since we're not actually touching this wall, ignore it
							collision->weight = 0;

						} else if (MUrSin(current_dir) * collision->collision_plane.a + MUrCos(current_dir) * collision->collision_plane.c >
									AI2cMovement_Collision_ParallelTolerance) {
							// we are currently heading away from this wall. ignore it.
							collision->weight = 0;

						} else {
							// we are heading parallel to or away from the wall. keep this collision, but reduce its
							// weight depending on how far away we are.
							collision->weight *= 1.0f - cyl_dist;
						}
					} else {
						// we are so close that we are touching the wall. don't reduce weight by anything.
					}
				} else {
					// we are slightly inside the wall but not sufficiently far to discard it completely!
					collision->weight *= 1.0f + cyl_dist / AI2cMovement_Collision_InsideForgetDistance;
				}
			}
		}

endloop:
		if ((collision->flags & (AI2cCollisionFlag_Hit | AI2cCollisionFlag_Miss)) == 0) {
			// this collision must be deleted; move another collision down to take its place if possible
			if (itr < ioMovementState->numCollisions - 1) {
				*collision = ioMovementState->collision[ioMovementState->numCollisions - 1];
			}
			ioMovementState->numCollisions--;
		} else {
			cur_serious = collision->flags & AI2cCollisionFlag_Serious;
			if (cur_serious) {
				ioMovementState->numSeriousCollisions++;
			}
			if (cur_serious != prev_serious) {
				// the set of serious collisions has changed. we must re-update all modifiers
				ioMovementState->modifiers_require_decreasing = UUcTrue;
			}

			// move on to the next collision.
			itr++;
			collision++;
		}
	}
}

void AI2iMovementState_ClearCollisionFlags(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	UUtUns32 itr;
	AI2tCollision *collision;

	// clear hit flags; leave miss flags so that wall-collision code knows to clear frame-counter if they're set
	for (itr = 0, collision = ioMovementState->collision; itr < ioMovementState->numCollisions; itr++, collision++) {
		collision->flags &= ~AI2cCollisionFlag_Hit;
	}
}

// a movement modifier has been added
void AI2rMovementState_NewMovementModifier(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, UUtUns32 inNewIndex)
{
	ioMovementState->numActiveModifiers++;
	if ((ioMovementState->numSeriousCollisions > 0) || (ioCharacter->movementOrders.modifierAvoidAngle != AI2cAngle_None)) {
		// there is a new modifier that has not yet been decreased by collisions
		ioMovementState->modifiers_require_decreasing = UUcTrue;
	}
}

void AI2rMovementState_NewAvoidAngle(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	// movement modifiers will need to be changed to handle this avoid angle
	ioMovementState->modifiers_require_decreasing = UUcTrue;
}

// check to see if a particular collision should be ignored
static UUtBool AI2iMovementState_RejectCollision(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
											   M3tPoint3D *inPoint, M3tPlaneEquation *inPlane)
{
	M3tPoint3D char_pt;

	// reject collisions if stopped
	if ((ioMovementState->current_direction == AI2cMovementDirection_Stopped) ||
		(ioMovementState->move_direction == AI2cAngle_None)) {
		return UUcTrue;
	}

	// reject collisions with very horizontal planes (i.e. low ceilings) ... this is a stopgap measure to fix problems
	// with characters that are placed with their heads in the ceiling
	if ((float)fabs(inPlane->b) > AI2cMovement_Collision_MaxVerticalComponent) {
		return UUcTrue;
	}

	// reject collisions that face away from where we are currently
	ONrCharacter_GetPathfindingLocation(ioCharacter, &char_pt);
	if (char_pt.x * inPlane->a + char_pt.y * inPlane->b + char_pt.z * inPlane->c + inPlane->d < 0) {
		return UUcTrue;
	}

	return UUcFalse;
}

// find if we have a collision with a particular object
static AI2tCollision *AI2iMovementState_FindCollision(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, AI2tCollisionIdentifier *inCollisionID)
{
	UUtUns32 itr;
	UUtBool found = UUcFalse;
	AI2tCollision *collision;

	for (itr = 0, collision = ioMovementState->collision; itr < ioMovementState->numCollisions; itr++, collision++) {
		if (collision->collision_id.type != inCollisionID->type)
			continue;

		switch(collision->collision_id.type) {
			case AI2cCollisionType_Wall:
				found = (collision->collision_id.data.wall.plane_index == inCollisionID->data.wall.plane_index);
			break;

			case AI2cCollisionType_Physics:
				found = (collision->collision_id.data.physics_context == inCollisionID->data.physics_context);
			break;

			case AI2cCollisionType_Character:
				found = (collision->collision_id.data.character == inCollisionID->data.character);
			break;

			default:
				UUmAssert(!"AI2iMovementState_MakeCollision: unknown collision type found");
				found = UUcFalse;
			break;
		}

		if (found)
			break;
	}

	if (found) {
		return collision;
	} else {
		return NULL;
	}	
}

// find an existing collision or create a new collision if we don't already have one
static AI2tCollision *AI2iMovementState_MakeCollision(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
											   AI2tCollisionIdentifier *inCollisionID, M3tPoint3D *inPoint, M3tPlaneEquation *inPlane)
{
	AI2tCollision *collision;

	collision = AI2iMovementState_FindCollision(ioCharacter, ioMovementState, inCollisionID);

	if (collision == NULL) {
		// we need to add a new collision for this object
		if (ioMovementState->numCollisions >= AI2cMovementState_MaxCollisions) {
			// our collision array is full
			// FIXME: overwrite a less-important collision here?
			return NULL;
		}

		collision = &ioMovementState->collision[ioMovementState->numCollisions++];

		collision->flags = 0;
		collision->frame_counter = 0;
		collision->total_hit_frames = 0;
		collision->collision_id = *inCollisionID;
		collision->collision_plane = *inPlane;
		collision->weight = 0;	// set up when we age wall collisions

		// the direction to the collision is the inverse of the normal
		collision->direction = MUrATan2(-inPlane->a, -inPlane->c);
		UUmTrig_ClipLow(collision->direction);	
		
		AI2_ERROR(AI2cStatus, AI2cSubsystem_Movement, AI2cError_Movement_Collision, ioCharacter,
				inCollisionID, inPoint, inPlane, &collision->direction);
	} else {
		// we found an existing collision with this object
		if (collision->flags & AI2cCollisionFlag_Miss) {
			collision->flags &= ~AI2cCollisionFlag_Miss;
			collision->frame_counter = 0;
		}
	}

	collision->flags |= AI2cCollisionFlag_Hit;
	collision->last_hit_point = ioCharacter->actual_position;
	MUmVector_Copy(collision->last_collision_point, *inPoint);
	collision->total_hit_frames++;

	return collision;
}

// notify the AI of a wall collision
UUtError AI2rMovementState_NotifyWallCollision(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
											   UUtUns32 inGQIndex, UUtUns32 inPlaneIndex, M3tPoint3D *inPoint,
											   M3tPlaneEquation *inPlane)
{
	AI2tCollisionIdentifier collision_id;
	AI2tCollision *collision;

	if (AI2iMovementState_RejectCollision(ioCharacter, ioMovementState, inPoint, inPlane)) {
		return UUcError_None;
	}

	collision_id.type = AI2cCollisionType_Wall;
	collision_id.data.wall.gq_index		= inGQIndex;
	collision_id.data.wall.plane_index	= inPlaneIndex;

	// get the collision for this object (existing or newly created)
	collision = AI2iMovementState_MakeCollision(ioCharacter, ioMovementState, &collision_id, inPoint, inPlane);
	if (collision == NULL) {
		AI2_ERROR(AI2cError, AI2cSubsystem_Movement, AI2cError_Movement_MaxCollisions, ioCharacter,
					ioMovementState->numCollisions, &collision_id, inPoint, inPlane);
		return UUcError_Generic;
	}

	// store the gq index for this collision (it may change even if we found an existing collision)
	UUmAssert(collision->collision_id.type == AI2cCollisionType_Wall);
	collision->collision_id.data.wall.gq_index = inGQIndex;

	return UUcError_None;
}

UUtError AI2rMovementState_NotifyPhysicsCollision(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, PHtPhysicsContext *inContext,
											   M3tPoint3D *inPoint, M3tPlaneEquation *inPlane)
{
	AI2tCollisionIdentifier collision_id;
	AI2tCollision *collision;

	if (AI2iMovementState_RejectCollision(ioCharacter, ioMovementState, inPoint, inPlane)) {
		return UUcError_None;
	}

	collision_id.type = AI2cCollisionType_Physics;
	collision_id.data.physics_context = inContext;

	// get the collision for this object (existing or newly created)
	collision = AI2iMovementState_MakeCollision(ioCharacter, ioMovementState, &collision_id, inPoint, inPlane);
	if (collision == NULL) {
		AI2_ERROR(AI2cError, AI2cSubsystem_Movement, AI2cError_Movement_MaxCollisions, ioCharacter,
					ioMovementState->numCollisions, &collision_id, inPoint, inPlane);
		return UUcError_Generic;
	}

	return UUcError_None;
}

UUtError AI2rMovementState_NotifyCharacterCollision(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, ONtCharacter *inCollidingCharacter,
											   M3tPoint3D *inPoint, M3tPlaneEquation *inPlane)
{
	AI2tCollisionIdentifier collision_id;
	AI2tCollision *collision;

	if (AI2rTryToPathfindThrough(ioCharacter, inCollidingCharacter)) {
		return UUcError_None;
	}

	if (AI2iMovementState_RejectCollision(ioCharacter, ioMovementState, inPoint, inPlane)) {
		return UUcError_None;
	}

	collision_id.type = AI2cCollisionType_Physics;
	collision_id.data.character = inCollidingCharacter;

	// get the collision for this object (existing or newly created)
	collision = AI2iMovementState_MakeCollision(ioCharacter, ioMovementState, &collision_id, inPoint, inPlane);
	if (collision == NULL) {
		AI2_ERROR(AI2cError, AI2cSubsystem_Movement, AI2cError_Movement_MaxCollisions, ioCharacter,
					ioMovementState->numCollisions, &collision_id, inPoint, inPlane);
		return UUcError_Generic;
	}

	return UUcError_None;
}

// ------------------------------------------------------------------------------------
// -- debugging routines

// report in
void AI2rMovementState_Report(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
							  UUtBool inVerbose, AI2tReportFunction inFunction)
{
	char reportbuf[256], tempbuf[256];
	AI2tCollision *collision;
	UUtUns32 itr;

	if (inVerbose && (ioMovementState->numCollisions > 0)) {
		sprintf(reportbuf, "  currently tracking %d wall collisions:", ioMovementState->numCollisions);
		inFunction(reportbuf);

		for (itr = 0, collision = ioMovementState->collision; itr < ioMovementState->numCollisions; itr++, collision++) {
			sprintf(reportbuf, "    %-4s (%2d) (total hit %2d%s) plane (%f %f %f / %f), point (%f %f %f), direction %f deg, weight %f ",
					(collision->flags & AI2cCollisionFlag_Hit) ? "HIT" : (collision->flags & AI2cCollisionFlag_Miss) ? "MISS" : "NONE",
					collision->frame_counter, collision->total_hit_frames, (collision->flags & AI2cCollisionFlag_Serious) ? ", SERIOUS" : "",
					collision->collision_plane.a, collision->collision_plane.b, collision->collision_plane.c, collision->collision_plane.d,
					collision->last_collision_point.x, collision->last_collision_point.y, collision->last_collision_point.z,
					collision->direction * M3cRadToDeg, collision->weight);
			switch(collision->collision_id.type) {
				case AI2cCollisionType_Wall:
					sprintf(tempbuf, "[gq %d plane %d]", collision->collision_id.data.wall.gq_index, collision->collision_id.data.wall.plane_index);
				break;

				case AI2cCollisionType_Physics:
					sprintf(tempbuf, "[context type %d]", collision->collision_id.data.physics_context->callback->type);
				break;

				case AI2cCollisionType_Character:
					sprintf(tempbuf, "[character %s]", collision->collision_id.data.character->player_name);
				break;

				default:
					sprintf(tempbuf, "[unknown %d]", collision->collision_id.type);
				break;
			}
			strcat(reportbuf, tempbuf);
			inFunction(reportbuf);
		}
	}

	sprintf(reportbuf, "  current movement: direction %s, angle %f (deg)", AI2cMovementDirectionName[ioMovementState->movementDirection],
		ioMovementState->move_direction * M3cRadToDeg);
	inFunction(reportbuf);
}

#define AI2cMovementState_Render_BadnessAngle		(5.0f * M3cDegToRad)

// draw collision debugging info
void AI2rMovementState_RenderCollision(M3tPoint3D *inPoint, UUtUns32 inNumCollisions, AI2tCollision *inCollisions,
									   UUtUns32 inNumBadnessValues, AI2tBadnessValue *inBadnessValues)
{
	UUtUns32 itr, next_index;
	M3tVector3D line_start, line_end, center_pt;
	AI2tCollision *collision;
	AI2tBadnessValue *ptr, *nextptr;
	float radius, angle, value;
	UUtBool exit;

	MUmVector_Copy(center_pt, *inPoint);
	MUmVector_Copy(line_start, center_pt);
	for (itr = 0, collision = inCollisions; itr < inNumCollisions; itr++, collision++) {
		MUmVector_Set(line_end, MUrSin(collision->direction), 0, MUrCos(collision->direction));
		MUmVector_Scale(line_end, 20.0f * collision->weight);
		MUmVector_Add(line_end, line_end, line_start);

		M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Yellow);
	}

	if ((inNumCollisions > 0) && (inNumBadnessValues > 0)) {
		// draw the badness array
		ptr = inBadnessValues;
		radius = 30.0f;

		do {
			next_index = ptr->next_index;
			UUmAssert((next_index >= 0) && (next_index < inNumBadnessValues));
			nextptr = &inBadnessValues[next_index];

			// draw an arc from ptr to nextptr
			angle = ptr->angle;
			MUmVector_Set(line_end, MUrSin(angle), 0, MUrCos(angle));
			MUmVector_Scale(line_end, radius * ptr->value);
			MUmVector_Add(line_end, line_end, center_pt);
			exit = UUcFalse;

			do {
				angle += AI2cMovementState_Render_BadnessAngle;
				
				if (angle > nextptr->angle) {
					angle = nextptr->angle;
					value = nextptr->value;
					exit = UUcTrue;
				} else {
					value = ptr->value + (nextptr->value - ptr->value) *
						(angle - ptr->angle) / (nextptr->angle - ptr->angle);
				}

				MUmVector_Copy(line_start, line_end);
				MUmVector_Set(line_end, MUrSin(angle), 0, MUrCos(angle));
				MUmVector_Scale(line_end, radius * value);
				MUmVector_Add(line_end, line_end, center_pt);
				M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Red);
			} while (!exit);
			
			ptr = nextptr;

		} while (ptr != inBadnessValues);
	}
}

// draw the character's path
void AI2rMovementState_RenderPath(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState)
{
	M3tPoint3D line_start, line_end, top_point;
	IMtShade jack_shade;
	float radius;
	UUtUns32 itr;

#if TOOL_VERSION
	// draw our wall collisions
	AI2rMovementState_RenderCollision(&ioCharacter->actual_position, ioMovementState->numCollisions, ioMovementState->collision, 
										ioMovementState->numBadnessValues, ioMovementState->badnesslist);
#endif

	if (ioCharacter->pathState.at_finalpoint)
		return;

	// draw little markers at each small grid node in the current room
	jack_shade = IMcShade_White;
	for (itr = ioMovementState->grid_current_point; itr < ioMovementState->grid_num_points; itr++) {
		MUmVector_Copy(top_point, ioMovementState->grid_path[itr].point);
		top_point.y += 10.0f;
		M3rGeom_Line_Light(&ioMovementState->grid_path[itr].point, &top_point, jack_shade);

		MUmVector_Copy(line_start, top_point);
		MUmVector_Copy(line_end, top_point);
		line_start.x -= 5.0f;
		line_end.x += 5.0f;
		M3rGeom_Line_Light(&line_start, &line_end, jack_shade);

		MUmVector_Copy(line_start, top_point);
		MUmVector_Copy(line_end, top_point);
		line_start.z -= 5.0f;
		line_end.z += 5.0f;
		M3rGeom_Line_Light(&line_start, &line_end, jack_shade);

		// get the radius to hit this point with
		radius = AI2iMovementState_GetPointHitDistance(ioCharacter, ioMovementState, &ioMovementState->grid_path[itr]);

		MUmVector_Copy(line_start, top_point);
		MUmVector_Copy(line_end, top_point);
		line_start.y -= 8.0f;
		line_end.y -= 8.0f;

		line_start.x += radius;
		line_end.z += radius;
		M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Blue);

		line_start.x -= 2 * radius;		
		M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Blue);

		line_end.z -= 2 * radius;
		M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Blue);

		line_start.x += 2 * radius;	
		M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Blue);

		jack_shade = IMcShade_Red;
	}
}

