/*
	FILE:	Oni_AI2_Movement.c

	AUTHOR:	Michael Evans, Chris Butcher

	CREATED: November 15, 1999

	PURPOSE: Movement AI for Oni

	Copyright (c) Bungie Software, 1999

*/

#include "BFW.h"
#include "BFW_Timer.h"

#include "Oni_AI2_Movement.h"
#include "Oni_AI2.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"


// ------------------------------------------------------------------------------------
// -- constants

const char AI2cMovementModeName[AI2cMovementMode_Max][16]
	 = {"by_alert_level", "stop", "crouch", "creep", "walk_noaim", "walk", "run_noaim", "run"};

const char AI2cMovementDirectionName[AI2cMovementDirection_Max][16]
	 = {"forwards", "backwards", "left", "right", "stopped"};

// these angles are the angle that we must apply to our facing to get our movement direction
// therefore left = 1/4 turn counterclockwise = pi/2
const float AI2cMovementDirection_Offset[AI2cMovementDirection_Max] =
	{0, M3cPi, M3cHalfPi, M3c2Pi - M3cHalfPi, 0};

const AI2tMovementDirection AI2cMovementDirection_TurnLeft[AI2cMovementDirection_Max] =
	{AI2cMovementDirection_Sidestep_Right, AI2cMovementDirection_Sidestep_Left, AI2cMovementDirection_Backpedal,
	AI2cMovementDirection_Forward, AI2cMovementDirection_Stopped};

const AI2tMovementDirection AI2cMovementDirection_TurnRight[AI2cMovementDirection_Max] =
	{AI2cMovementDirection_Sidestep_Left, AI2cMovementDirection_Sidestep_Right, AI2cMovementDirection_Forward,
	AI2cMovementDirection_Backpedal, AI2cMovementDirection_Stopped};

const AI2tMovementDirection AI2cMovementDirection_TurnAround[AI2cMovementDirection_Max] =
	{AI2cMovementDirection_Backpedal, AI2cMovementDirection_Forward, AI2cMovementDirection_Sidestep_Right,
	AI2cMovementDirection_Sidestep_Left, AI2cMovementDirection_Stopped};

// the default movement modes for all of the alert states
// hurry	 = responding to a specific alert
// unhurried = just walking around
const AI2tMovementMode AI2cDefaultMovementMode_Hurry[AI2cAlertStatus_Max] =
	{AI2cMovementMode_NoAim_Walk, AI2cMovementMode_NoAim_Walk, AI2cMovementMode_NoAim_Run, AI2cMovementMode_Run, AI2cMovementMode_Run};
const AI2tMovementMode AI2cDefaultMovementMode_Unhurried[AI2cAlertStatus_Max] =
	{AI2cMovementMode_NoAim_Walk, AI2cMovementMode_NoAim_Walk, AI2cMovementMode_Walk, AI2cMovementMode_Walk, AI2cMovementMode_Walk};


// ------------------------------------------------------------------------------------
// -- movement order interfaces

void AI2rMovement_Initialize(ONtCharacter *ioCharacter)
{
	// stand still
	ioCharacter->movementOrders.force_aim = UUcFalse;
	ioCharacter->movementOrders.facingMode = AI2cFacingMode_None;
	ioCharacter->movementOrders.numModifiers = 0;
	ioCharacter->movementOrders.modifierAvoidAngle = AI2cAngle_None;

	// eyes forward
	ioCharacter->movementOrders.aimingMode = AI2cAimingMode_None;
	ioCharacter->movementOrders.glance_timer = 0;
	ioCharacter->movementOrders.glance_delay = 0;
	ioCharacter->movementOrders.curPathIgnoreChar = NULL;

	// our movement mode is defined by our alert state
	AI2rMovement_DisableSlowdown(ioCharacter, UUcFalse);
	AI2rMovement_ChangeMovementMode(ioCharacter, AI2cMovementMode_Default);
}

/*
 * aiming interfaces
 */

void AI2rMovement_StopAiming(ONtCharacter *ioCharacter)
{
	ioCharacter->movementOrders.aimingMode = AI2cAimingMode_None;
}

void AI2rMovement_LookAtPoint(ONtCharacter *ioCharacter, const M3tPoint3D *inPoint)
{
	ioCharacter->movementOrders.aimingMode = AI2cAimingMode_LookAtPoint;
	ioCharacter->movementOrders.aimingData.lookAtPoint.point = *inPoint;
}

void AI2rMovement_LookInDirection(ONtCharacter *ioCharacter, const M3tPoint3D *inDirection)
{
	ioCharacter->movementOrders.aimingMode = AI2cAimingMode_LookInDirection;
	ioCharacter->movementOrders.aimingData.lookInDirection.vector = *inDirection;
}

void AI2rMovement_LookAtCharacter(ONtCharacter *ioCharacter, ONtCharacter *inTarget)
{
	ioCharacter->movementOrders.aimingMode = AI2cAimingMode_LookAtCharacter;
	ioCharacter->movementOrders.aimingData.lookAtCharacter.character = inTarget;
}

void AI2rMovement_DontForceAim(ONtCharacter *ioCharacter)
{
	ioCharacter->movementOrders.force_aim = UUcFalse;

	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_ResetAimingVarient(ioCharacter, &active_character->movement_state);
	}
}

void AI2rMovement_Force_AimWithWeapon(ONtCharacter *ioCharacter, UUtBool inAimWithWeapon)
{
	ioCharacter->movementOrders.force_aim = UUcTrue;
	ioCharacter->movementOrders.force_withweapon = inAimWithWeapon;

	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_ResetAimingVarient(ioCharacter, &active_character->movement_state);
	}
}

/*
 * glancing interfaces
 */

void AI2rMovement_StopGlancing(ONtCharacter *ioCharacter)
{
	ioCharacter->movementOrders.glance_timer = 0;
	ioCharacter->movementOrders.glance_delay = 0;
}

void AI2rMovement_GlanceAtPoint(ONtCharacter *ioCharacter, const M3tPoint3D *inPoint, UUtUns32 inTimer, UUtBool inDanger, UUtUns32 inDelay)
{
	ioCharacter->movementOrders.glance_timer = inTimer;
	ioCharacter->movementOrders.glance_delay = inDelay;
	ioCharacter->movementOrders.glance_danger = inDanger;
	ioCharacter->movementOrders.glanceAimingMode = AI2cAimingMode_LookAtPoint;
	ioCharacter->movementOrders.glanceAimingData.lookAtPoint.point = *inPoint;
}

void AI2rMovement_GlanceInDirection(ONtCharacter *ioCharacter, const M3tPoint3D *inDirection, UUtUns32 inTimer, UUtBool inDanger, UUtUns32 inDelay)
{
	ioCharacter->movementOrders.glance_timer = inTimer;
	ioCharacter->movementOrders.glance_delay = inDelay;
	ioCharacter->movementOrders.glance_danger = inDanger;
	ioCharacter->movementOrders.glanceAimingMode = AI2cAimingMode_LookInDirection;
	ioCharacter->movementOrders.glanceAimingData.lookInDirection.vector = *inDirection;
}

void AI2rMovement_GlanceAtCharacter(ONtCharacter *ioCharacter, ONtCharacter *inTarget, UUtUns32 inTimer, UUtBool inDanger, UUtUns32 inDelay)
{
	ioCharacter->movementOrders.glance_timer = inTimer;
	ioCharacter->movementOrders.glance_delay = inDelay;
	ioCharacter->movementOrders.glance_danger = inDanger;
	ioCharacter->movementOrders.glanceAimingMode = AI2cAimingMode_LookAtCharacter;
	ioCharacter->movementOrders.glanceAimingData.lookAtCharacter.character = inTarget;
}

/*
 * facing interfaces
 */

void AI2rMovement_LockFacing(ONtCharacter *ioCharacter, AI2tMovementDirection inDirection)
{
	ioCharacter->movementOrders.facingMode = AI2cFacingMode_PathRelative;
	ioCharacter->movementOrders.facingData.pathRelative.direction = inDirection;
}

void AI2rMovement_FreeFacing(ONtCharacter *ioCharacter)
{
	ioCharacter->movementOrders.facingMode = AI2cFacingMode_None;
}

/*
 * pathfinding-ignore-character interfaces
 */

void AI2rMovement_IgnoreCharacter(ONtCharacter *ioCharacter, ONtCharacter *inIgnoreChar)
{
	ioCharacter->movementOrders.curPathIgnoreChar = inIgnoreChar;
}

UUtBool AI2rMovement_ShouldIgnoreObstruction(ONtCharacter *ioCharacter, ONtCharacter *inObstruction)
{
	return (inObstruction == ioCharacter->movementOrders.curPathIgnoreChar);
}

/*
 * general movement interfaces
 */

void AI2rMovement_DisableSlowdown(ONtCharacter *ioCharacter, UUtBool inDisabled)
{
	ioCharacter->movementOrders.slowdownDisabled = inDisabled;
}

void AI2rMovement_ChangeMovementMode(ONtCharacter *ioCharacter, AI2tMovementMode inNewMode)
{
	ioCharacter->movementOrders.movementMode = inNewMode;

	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_NewMovementMode(ioCharacter, &active_character->movement_state);
	}
}

void AI2rMovement_NotifyAlertChange(ONtCharacter *ioCharacter)
{
	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_NotifyAlertChange(ioCharacter, &active_character->movement_state);
	}
}

UUtError AI2rMovement_MovementModifier(ONtCharacter *ioCharacter, float inDirection, float inWeight)
{
	if (ioCharacter->movementOrders.numModifiers >= AI2cMovement_MaxModifiers) {
		AI2_ERROR(AI2cError, AI2cSubsystem_Movement, AI2cError_Movement_MaxModifiers, ioCharacter,
					ioCharacter->movementOrders.numModifiers, &inDirection, &inWeight, 0);

		return UUcError_Generic;
	}

	ioCharacter->movementOrders.modifier[ioCharacter->movementOrders.numModifiers].direction = inDirection;
	ioCharacter->movementOrders.modifier[ioCharacter->movementOrders.numModifiers].full_weight = inWeight;
	ioCharacter->movementOrders.modifier[ioCharacter->movementOrders.numModifiers].decreased_weight = inWeight;
	MUmVector_Set(ioCharacter->movementOrders.modifier[ioCharacter->movementOrders.numModifiers].unitvector, MUrSin(inDirection), 0, MUrCos(inDirection));
	ioCharacter->movementOrders.numModifiers++;

	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_NewMovementModifier(ioCharacter, &active_character->movement_state, ioCharacter->movementOrders.numModifiers - 1);
	}

	return UUcError_None;
}

void AI2rMovement_MovementModifierAvoidAngle(ONtCharacter *ioCharacter, float inDirection)
{
	ioCharacter->movementOrders.modifierAvoidAngle = inDirection;

	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_NewAvoidAngle(ioCharacter, &active_character->movement_state);
	}
}

void AI2rMovement_ClearMovementModifiers(ONtCharacter *ioCharacter)
{
	ioCharacter->movementOrders.numModifiers = 0;
	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		active_character->movement_state.numActiveModifiers = 0;
	}
}

/*
 * query interfaces
 */

UUtBool AI2rMovement_HasMovementModifiers(ONtCharacter *ioCharacter)
{
	return (ioCharacter->movementOrders.numModifiers > 0);
}

UUtBool AI2rMovement_IsKeepingMoving(ONtCharacter *ioCharacter)
{
	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		return AI2rMovementState_IsKeepingMoving(ioCharacter, &active_character->movement_state);
	} else {
		return UUcFalse;
	}
}

float AI2rMovement_GetMoveDirection(ONtCharacter *ioCharacter)
{
	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		return AI2rMovementState_GetMoveDirection(ioCharacter, &active_character->movement_state);
	} else {
		return AI2rMovementStub_GetMoveDirection(ioCharacter);
	}
}

UUtBool AI2rMovement_SimplePath(ONtCharacter *ioCharacter)
{
	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		return AI2rMovementState_SimplePath(ioCharacter, &active_character->movement_state);
	} else {
		return UUcTrue;
	}
}

// ------------------------------------------------------------------------------------
// -- command interface

void AI2rMovement_Halt(ONtCharacter *ioCharacter)
{
	ioCharacter->movementOrders.numModifiers = 0;
	ioCharacter->movementOrders.modifierAvoidAngle = AI2cAngle_None;
	ioCharacter->movementOrders.curPathIgnoreChar = NULL;

	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_Halt(ioCharacter, &active_character->movement_state);
	}
}

void AI2rMovement_NewPathReceived(ONtCharacter *ioCharacter)
{
	if (ioCharacter->movementOrders.facingMode == AI2cFacingMode_FaceAtDestination) {
		ioCharacter->movementOrders.facingMode = AI2cFacingMode_None;
	}

	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_NewPathReceived(ioCharacter, &active_character->movement_state);
	}
}

void AI2rMovement_NewDestination(ONtCharacter *ioCharacter)
{
	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_NewDestination(ioCharacter, &active_character->movement_state);
	} else {
		AI2rMovementStub_NewDestination(ioCharacter);
	}
}

void AI2rMovement_ClearPath(ONtCharacter *ioCharacter)
{
	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_ClearPath(ioCharacter, &active_character->movement_state);
	} else {
		AI2rMovementStub_ClearPath(ioCharacter);
	}
}

UUtBool AI2rMovement_AdvanceThroughGrid(ONtCharacter *ioCharacter)
{
	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		return AI2rMovementState_AdvanceThroughGrid(ioCharacter, &active_character->movement_state);
	} else {
		return AI2rMovementStub_AdvanceThroughGrid(ioCharacter);
	}
}

void AI2rMovement_Update(ONtCharacter *ioCharacter)
{
	if (ioCharacter->movementOrders.glance_delay > 0) {
		ioCharacter->movementOrders.glance_delay -= 1;
	} else {
		if (ioCharacter->movementOrders.glance_timer > 0) {
			// note: glance_timer only decrements once the aim has in fact reached the desired direction,
			// if we are a present character
			if ((!ONrCharacter_IsActive(ioCharacter)) || (AI2rExecutor_AimReachedTarget(ioCharacter))) {
				ioCharacter->movementOrders.glance_timer -= 1;
			}
		}
	}

	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_Update(ioCharacter, &active_character->movement_state);
		AI2rExecutor_Update(ioCharacter);
	} else {
		AI2rMovementStub_Update(ioCharacter);

		if (ioCharacter->movementStub.path_active) {
			// directly warp the character to the newly calculated position
			ioCharacter->facing = ioCharacter->movementStub.cur_facing;
			ONrCharacter_Teleport(ioCharacter, &ioCharacter->movementStub.cur_point, UUcFalse);
		}
	}
}

UUtBool AI2rMovement_DestinationFacing(ONtCharacter *ioCharacter)
{
	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		return AI2rMovementState_DestinationFacing(ioCharacter, &active_character->movement_state);
	} else {
		return AI2rMovementStub_DestinationFacing(ioCharacter);
	}
}

UUtBool AI2rMovement_CheckFailure(ONtCharacter *ioCharacter)
{
	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		return AI2rMovementState_CheckFailure(ioCharacter, &active_character->movement_state);
	} else {
		return AI2rMovementStub_CheckFailure(ioCharacter);
	}
}

UUtBool AI2rMovement_MakePath(ONtCharacter *ioCharacter, UUtBool inReusePath)
{
	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		return AI2rMovementState_MakePath(ioCharacter, &active_character->movement_state, inReusePath);
	} else {
		return AI2rMovementStub_MakePath(ioCharacter);
	}
}

void AI2rMovement_Report(ONtCharacter *ioCharacter, UUtBool inVerbose, AI2tReportFunction inFunction)
{
	char reportbuf[256], namebuf[128];
	ONtCharacter *look_character;

	sprintf(reportbuf, "  movement mode %s", AI2cMovementModeName[ioCharacter->movementOrders.movementMode]);
	inFunction(reportbuf);

	switch(ioCharacter->movementOrders.facingMode) {
		case AI2cFacingMode_PathRelative:
			sprintf(reportbuf, "  facing locked %s", AI2cMovementDirectionName[ioCharacter->movementOrders.facingData.pathRelative.direction]);
			break;

		case AI2cFacingMode_Angle:
			sprintf(reportbuf, "  facing in angle %f (deg)", ioCharacter->movementOrders.facingData.angle.angle * M3cRadToDeg);
			break;

		case AI2cFacingMode_FaceAtDestination:
			sprintf(reportbuf, "  facing-at-destination %f", ioCharacter->movementOrders.facingData.faceAtDestination.angle);
			break;

		default:
			sprintf(reportbuf, "  <unknown facing state %d>", ioCharacter->movementOrders.facingMode);
			break;
	}
	inFunction(reportbuf);

	switch(ioCharacter->movementOrders.aimingMode) {
		case AI2cAimingMode_None:
			sprintf(reportbuf, "  not aiming");
			break;

		case AI2cAimingMode_LookAtPoint:
			sprintf(reportbuf, "  aiming at %f, %f, %f",
					ioCharacter->movementOrders.aimingData.lookAtPoint.point.x,
					ioCharacter->movementOrders.aimingData.lookAtPoint.point.y,
					ioCharacter->movementOrders.aimingData.lookAtPoint.point.z);
			break;

		case AI2cAimingMode_LookInDirection:
			sprintf(reportbuf, "  aiming in direction %f, %f, %f",
					ioCharacter->movementOrders.aimingData.lookInDirection.vector.x,
					ioCharacter->movementOrders.aimingData.lookInDirection.vector.y,
					ioCharacter->movementOrders.aimingData.lookInDirection.vector.z);
			break;

		case AI2cAimingMode_LookAtCharacter:
			look_character = ioCharacter->movementOrders.aimingData.lookAtCharacter.character;
			if ((look_character == NULL) || ((look_character->flags & ONcCharacterFlag_InUse) == 0)) {
				strcpy(namebuf, "<nobody>");
			} else {
				strcpy(namebuf, look_character->player_name);
			}

			sprintf(reportbuf, "  aiming at %s", namebuf);
			break;

		default:
			sprintf(reportbuf, "  <unknown aiming state>");
			break;
	}

	if (ioCharacter->movementOrders.force_aim) {
		strcat(reportbuf, (ioCharacter->movementOrders.force_withweapon) ? " (forced to aim weapon)" : " (forced not to aim weapon)");
	}
	inFunction(reportbuf);

	if (ioCharacter->movementOrders.glance_timer) {
		switch(ioCharacter->movementOrders.glanceAimingMode) {
			case AI2cAimingMode_None:
				sprintf(reportbuf, "  <error: glancing(%d delay %d) but glancing mode is NONE>",
						ioCharacter->movementOrders.glance_timer, ioCharacter->movementOrders.glance_delay);
				break;

			case AI2cAimingMode_LookAtPoint:
				sprintf(reportbuf, "  glancing(%d delay %d) at %f, %f, %f",
					ioCharacter->movementOrders.glance_timer, ioCharacter->movementOrders.glance_delay,
					ioCharacter->movementOrders.glanceAimingData.lookAtPoint.point.x,
					ioCharacter->movementOrders.glanceAimingData.lookAtPoint.point.y,
					ioCharacter->movementOrders.glanceAimingData.lookAtPoint.point.z);
				break;

			case AI2cAimingMode_LookInDirection:
				sprintf(reportbuf, "  glancing(%d delay %d) in direction %f, %f, %f",
					ioCharacter->movementOrders.glance_timer, ioCharacter->movementOrders.glance_delay,
					ioCharacter->movementOrders.glanceAimingData.lookInDirection.vector.x,
					ioCharacter->movementOrders.glanceAimingData.lookInDirection.vector.y,
					ioCharacter->movementOrders.glanceAimingData.lookInDirection.vector.z);
				break;

			case AI2cAimingMode_LookAtCharacter:
				look_character = ioCharacter->movementOrders.glanceAimingData.lookAtCharacter.character;
				if ((look_character == NULL) || ((look_character->flags & ONcCharacterFlag_InUse) == 0)) {
					strcpy(namebuf, "<nobody>");
				} else {
					strcpy(namebuf, look_character->player_name);
				}

				sprintf(reportbuf, "  glancing(%d delay %d) at %s",
						ioCharacter->movementOrders.glance_timer, ioCharacter->movementOrders.glance_delay, namebuf);
				break;

			default:
				sprintf(reportbuf, "  <unknown glancing state: %d>", ioCharacter->movementOrders.glanceAimingMode);
				break;
		}
		inFunction(reportbuf);
	}

	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_Report(ioCharacter, &active_character->movement_state, inVerbose, inFunction);
	} else {
		AI2rMovementStub_Report(ioCharacter, inVerbose, inFunction);
	}
}

void AI2rMovement_RenderPath(ONtCharacter *ioCharacter)
{
	M3tVector3D line_start, line_end;
	AI2tMovementModifier *modifier;
	float sintheta, costheta;
	UUtUns32 itr;

	if (ioCharacter->movementOrders.modifierAvoidAngle != AI2cAngle_None) {
		sintheta = MUrSin(ioCharacter->movementOrders.modifierAvoidAngle);
		costheta = MUrCos(ioCharacter->movementOrders.modifierAvoidAngle);

		MUmVector_Copy(line_start, ioCharacter->actual_position);
		line_start.x -= 5.0f * costheta;
		line_start.z += 5.0f * sintheta;
		MUmVector_Copy(line_end, line_start);
		line_end.x += 30.0f * sintheta;
		line_end.z += 30.0f * costheta;
		M3rGeom_Line_Light(&line_start, &line_end, IMcShade_LightBlue);

		MUmVector_Copy(line_start, ioCharacter->actual_position);
		line_start.x += 5.0f * costheta;
		line_start.z -= 5.0f * sintheta;
		MUmVector_Copy(line_end, line_start);
		line_end.x += 30.0f * sintheta;
		line_end.z += 30.0f * costheta;
		M3rGeom_Line_Light(&line_start, &line_end, IMcShade_LightBlue);
	}

	// draw our movement modifiers
	line_start = ioCharacter->actual_position;

	for (itr = 0, modifier = ioCharacter->movementOrders.modifier; itr < ioCharacter->movementOrders.numModifiers; itr++, modifier++) {

		MUmVector_Set(line_end, MUrSin(modifier->direction), 0, MUrCos(modifier->direction));
		MUmVector_Scale(line_end, 20.0f * modifier->decreased_weight);
		MUmVector_Add(line_end, line_end, line_start);

		M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Green);
	}

	if ((!ioCharacter->pathState.at_destination) && (ioCharacter->movementOrders.facingMode == AI2cFacingMode_FaceAtDestination)) {
		// we are just turning to face our destination facing.
		MUmVector_Copy(line_start, ioCharacter->pathState.destinationLocation);
		line_start.y += 6.0f;

		MUmVector_Set(line_end, 20.0f * MUrSin(ioCharacter->movementOrders.facingData.faceAtDestination.angle), 0,
								20.0f * MUrCos(ioCharacter->movementOrders.facingData.faceAtDestination.angle));
		MUmVector_Add(line_end, line_end, line_start);
		M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Blue);
	}

	if (ONrCharacter_IsActive(ioCharacter)) {
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
		UUmAssert(active_character != NULL);

		AI2rMovementState_RenderPath(ioCharacter, &active_character->movement_state);
	} else {
		AI2rMovementStub_RenderPath(ioCharacter);
	}
}

// ------------------------------------------------------------------------------------
// -- global movement functions

// get the default movement speed for a character based on what their alert state is
// and what they're doing
AI2tMovementMode AI2rMovement_Default(ONtCharacter *ioCharacter)
{
	AI2tAlertStatus current_alert_status;
	const AI2tMovementMode *mode_array;

	UUmAssertReadPtr(ioCharacter, sizeof(ONtCharacter));
	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);

	if (ioCharacter->charType != ONcChar_AI2) {
		// player being moved by AI system
		return AI2cMovementMode_Run;
	}

	mode_array = AI2rIsHurrying(ioCharacter) ? AI2cDefaultMovementMode_Hurry : AI2cDefaultMovementMode_Unhurried;

	current_alert_status = ioCharacter->ai2State.alertStatus;
	if ((ioCharacter->ai2State.alertStatus < 0) || (ioCharacter->ai2State.alertStatus >= AI2cAlertStatus_Max)) {
		AI2_ERROR(AI2cBug, AI2cSubsystem_Movement, AI2cError_Movement_InvalidAlertStatus, ioCharacter,
				ioCharacter->ai2State.alertStatus, 0, 0, 0);
		return mode_array[AI2cAlertStatus_Lull];
	} else {
		return mode_array[ioCharacter->ai2State.alertStatus];
	}
}

