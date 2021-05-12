/*
	FILE:	Oni_AI2_Executor.c
	
	AUTHOR:	Michael Evans, Chris Butcher
	
	CREATED: November 15, 1999
	
	PURPOSE: Low Level AI for Oni
	
	Copyright (c) 1999

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Totoro.h"

#include "Oni_AI2_Executor.h"
#include "Oni_GameState.h"
#include "Oni_Character.h"
#include "Oni_Character_Animation.h"

void AI2rExecutor_Initialize(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	executor->aiming = UUcFalse;
	executor->aiming_stopping = UUcFalse;
	executor->aim_reached_target = UUcTrue;
	executor->aiming_with_weapon = UUcFalse;
	executor->movement_direction = AI2cMovementDirection_Stopped;
	executor->movement_mode = AI2cMovementMode_Walk;
	executor->facing = ioCharacter->facing;
	executor->aiming_speed_scale = 1.0f;
	executor->turn_override = 0;
	executor->move_override = UUcFalse;
	executor->attack_override = UUcFalse;
	executor->throw_override = UUcFalse;
	executor->facing_override = AI2cAngle_None;
	executor->actual_movement_mode = AI2cMovementMode_Walk;
	executor->last_actual_facing = AI2cAngle_None;
}

// get the maximum amount a character can turn per frame
float AI2rExecutor_GetMaxDeltaFacingPerFrame(ONtCharacterClass *inClass, AI2tMovementMode inMovementMode, UUtBool inDanger)
{
	float delta = AI2cExecutor_MaxDeltaFacingPerFrame;

	delta *= inClass->ai2_behavior.turning_nimbleness;

	if ((!inDanger) && ((inMovementMode == AI2cMovementMode_Creep) ||
						(inMovementMode == AI2cMovementMode_NoAim_Walk) ||
						(inMovementMode == AI2cMovementMode_Walk))) {
		delta *= AI2cExecutor_FacingSpeedScaleWhenWalking;
	}

	return delta;
}

void AI2rExecutor_Update(ONtCharacter *ioCharacter)
{
	ONtInputState input;
	AI2tMovementMode current_movement_mode;
	LItButtonBits keys;
	UUtBool disable_turn = UUcFalse;
	float facing_goal, delta_facing_goal, new_facing;
	float max_turn;
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;

	executor = &active_character->executor_state;

	if (executor->move_override) {
		// our movement is being controlled by an override (e.g. by melee manager)
		keys = executor->move_override_keys;

		if (keys & LIc_BitMask_Walk) {
			executor->actual_movement_mode = (executor->aiming) ? AI2cMovementMode_Walk : AI2cMovementMode_NoAim_Walk;

		} else if (keys & LIc_BitMask_Crouch) {
			executor->actual_movement_mode = AI2cMovementMode_Creep;
			if (!ONrAnimState_IsCrouch(active_character->nextAnimState)) {
				// disable turning until we are actually starting to crouch
				disable_turn = UUcTrue;
			}

		} else {
			executor->actual_movement_mode = (executor->aiming) ? AI2cMovementMode_Run : AI2cMovementMode_NoAim_Run;
		}
	} else {
		// use our standard parameters
		current_movement_mode = executor->movement_mode;
		if (current_movement_mode == AI2cMovementMode_Default) {
			// this should never happen - movement layer should catch it. but do the right thing anyway.
			current_movement_mode = AI2rMovement_Default(ioCharacter);
		}
		executor->actual_movement_mode = current_movement_mode;

		// determine our movement based on the parameters set up by the
		// movement layer
		switch(executor->movement_direction)
		{
			case AI2cMovementDirection_Stopped:
				keys = 0;
			break;

			case AI2cMovementDirection_Forward:
				keys = LIc_BitMask_Forward;
			break;

			case AI2cMovementDirection_Backpedal:
				keys = LIc_BitMask_Backward;
			break;

			case AI2cMovementDirection_Sidestep_Left:
				keys = LIc_BitMask_StepLeft;
			break;

			case AI2cMovementDirection_Sidestep_Right:
				keys = LIc_BitMask_StepRight;
			break;

			default:
				UUmAssert(!"invalid case in executor update");
		}

		switch(current_movement_mode)
		{
			case AI2cMovementMode_Creep:
				keys |= LIc_BitMask_Crouch;
			break;

			case AI2cMovementMode_NoAim_Walk:
			case AI2cMovementMode_Walk:
				keys |= LIc_BitMask_Walk;
			break;

			case AI2cMovementMode_NoAim_Run:
			case AI2cMovementMode_Run:
			break;

			case AI2cMovementMode_Default:
			default:
				UUmAssert(!"invalid movement case");
			break;
		}
	}

	if (disable_turn || ONrCharacter_IsUnableToTurn(ioCharacter)) {
		// can't turn (dead, being thrown, etc)
		active_character->pleaseTurnLeft = UUcFalse;
		active_character->pleaseTurnRight = UUcFalse;

	} else if (keys & LIc_BitMask_TurnLeft) {
		// we are being forced to turn left
		active_character->pleaseTurnLeft  = UUcTrue;
		active_character->pleaseTurnRight = UUcFalse;

	} else if (keys & LIc_BitMask_TurnRight) {
		// we are being forced to turn right
		active_character->pleaseTurnLeft  = UUcFalse;
		active_character->pleaseTurnRight = UUcTrue;

	} else {
		// turn the character to face in the desired direction
		facing_goal = executor->facing_override;
		if (facing_goal == AI2cAngle_None)
			facing_goal = executor->facing;

		facing_goal += ioCharacter->facingModifier;
		UUmTrig_Clip(facing_goal);
		
		delta_facing_goal = facing_goal - ioCharacter->facing;
		UUmTrig_Clip(delta_facing_goal);
		
		max_turn = AI2rExecutor_GetMaxDeltaFacingPerFrame(ioCharacter->characterClass, current_movement_mode, executor->turn_danger);
		if (executor->turn_override) {
#if AI2_ERROR_REPORT
			// this is only useful if we're logging errors
			AI2_ERROR(AI2cStatus, AI2cSubsystem_Executor, AI2cError_Executor_TurnOverride,
				ioCharacter, &(ioCharacter->facing), &facing_goal, 0, 0);
#endif

			max_turn = executor->turn_override;
			executor->turn_override = 0.0f;
		}
		
		delta_facing_goal = (delta_facing_goal > M3cPi) ? (delta_facing_goal - M3c2Pi) : delta_facing_goal;
		delta_facing_goal = UUmPin(delta_facing_goal, -max_turn, max_turn);
		
		active_character->pleaseTurnLeft = active_character->pleaseTurnRight = UUcFalse;
		if (delta_facing_goal < -0.001f) {
			active_character->pleaseTurnLeft = UUcTrue;

		} else if (delta_facing_goal > 0.001f) {
			active_character->pleaseTurnRight = UUcTrue;

		} else if (ioCharacter->movementOrders.facingMode == AI2cFacingMode_FaceAtDestination) {
			// we're at our desired facing, just face forward from now on
			AI2rMovement_FreeFacing(ioCharacter);
		}
		
		new_facing = delta_facing_goal + ioCharacter->facing;
		UUmTrig_Clip(new_facing);
		
		ONrCharacter_SetFacing(ioCharacter, new_facing); 
	}

	// movement and facing overrides persist for only a single tick
	// (attack overrides persist until cleared)
	executor->move_override = UUcFalse;
	executor->facing_override = AI2cAngle_None;

	input.buttonIsDown = keys;
	input.buttonWentDown = keys & ~active_character->inputOld;

	if (executor->attack_override) {
		if ((ioCharacter->charType != ONcChar_AI2) || (ioCharacter->ai2State.currentGoal != AI2cGoal_Combat) ||
			(ioCharacter->ai2State.currentState->state.combat.maneuver.primary_movement != AI2cPrimaryMovement_Melee)) {
			// we are not in melee - this attack override is no longer valid
			AI2rExecutor_ClearAttackOverride(ioCharacter);
		} else {
			// check combo types - if we don't want one, clear it.
			if (executor->attack_override_requiredcombotype == ONcAnimType_None) {
				active_character->lastAttack = ONcAnimType_None;
			}
			
			input.buttonIsDown |= executor->attack_override_keys_isdown;
			input.buttonWentDown |= executor->attack_override_keys_wentdown;
		}
	}

	// certain kinds of buttons require that wentDown is set in order for us to get the
	// desired effect
	if (input.buttonIsDown & LIc_BitMask_Jump)
		input.buttonWentDown |= LIc_BitMask_Jump;

	if (input.buttonIsDown & LIc_BitMask_Crouch)
		input.buttonWentDown |= LIc_BitMask_Crouch;

	if (input.buttonIsDown & LIc_BitMask_Action)
		input.buttonWentDown |= LIc_BitMask_Action;

	input.buttonIsUp = ~input.buttonIsDown;
	input.buttonWentUp = input.buttonIsUp & active_character->inputOld;

	input.buttonLast = 0;

	input.turnLR = 0.f;
	input.turnUD = 0.f;

	ONrCharacter_UpdateInput(ioCharacter, &input);

	AI2rExecutor_UpdateAim(ioCharacter);

	return;
}

void AI2rExecutor_TurnOverride(ONtCharacter *ioCharacter, float override)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	executor->turn_override = override;
}

void AI2rExecutor_Move(ONtCharacter *ioCharacter, float inFacing, AI2tMovementMode inMovementMode,
					   AI2tMovementDirection inDirection, UUtBool turn_danger)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	UUmAssertTrigRange(inFacing);

	executor->turn_danger = turn_danger;
	executor->facing = inFacing;
	executor->movement_mode = inMovementMode;
	executor->movement_direction = inDirection;

	return;
}

AI2tMovementMode AI2rExecutor_ActualMovementMode(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return AI2cMovementMode_Run;
	executor = &active_character->executor_state;

	return executor->actual_movement_mode;
}

void AI2rExecutor_MoveOverride(ONtCharacter *ioCharacter, LItButtonBits inKeys)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	executor->move_override = UUcTrue;
	executor->move_override_keys = inKeys;
}

void AI2rExecutor_FacingOverride(ONtCharacter *ioCharacter, float inFacing, UUtBool inDanger)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	executor->facing_override = inFacing;
	executor->turn_danger = inDanger;
}

void AI2rExecutor_AttackOverride(ONtCharacter *ioCharacter, LItButtonBits inKeysIsDown, LItButtonBits inKeysWentDown,
								 UUtUns16 inDesiredAnimType, UUtUns16 inRequiredComboType)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	executor->attack_override = UUcTrue;
	executor->attack_override_desiredanimtype = inDesiredAnimType;
	executor->attack_override_requiredcombotype = inRequiredComboType;
	executor->attack_override_keys_isdown = inKeysIsDown;
	executor->attack_override_keys_wentdown = inKeysWentDown;
}

void AI2rExecutor_ThrowOverride(ONtCharacter *ioCharacter, ONtCharacter *inTarget, TRtAnimation *inThrow, TRtAnimation *inTargetThrow)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;

	executor = &active_character->executor_state;

	UUmAssert(inThrow != NULL);
	UUmAssert(inTargetThrow != NULL);
	executor->throw_override = UUcTrue;
	executor->throw_target = inTarget;
	executor->throw_anim = inThrow;
	executor->throw_targetanim = inTargetThrow;

	// a throw is performed through a dummy attack override using the punch animtype, which is
	// diverted by AI2rRemapThrow (called from RemapAnimationHook)
	AI2rExecutor_AttackOverride(ioCharacter, LIc_BitMask_Punch, LIc_BitMask_Punch, ONcAnimType_Punch, ONcAnimType_None);
}

void AI2rExecutor_ClearAttackOverride(ONtCharacter *ioCharacter)
{
	UUtUns16 anim_type;
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	if (!executor->throw_override) {
		// check that we performed the attack that we wanted
		anim_type = TRrAnimation_GetType(active_character->animation);
		if (anim_type != executor->attack_override_desiredanimtype) {
			// we've just performed an attack, all right, but not the one we were expecting!
			if (ioCharacter->charType == ONcChar_AI2) {
				AI2_ERROR(AI2cWarning, AI2cSubsystem_Executor, AI2cError_Executor_UnexpectedAttack, ioCharacter,
							executor->attack_override_desiredanimtype, anim_type, active_character->animation, 0);
			}
		}
	}

	executor->attack_override = UUcFalse;
	executor->throw_override = UUcFalse;
}

UUtBool AI2rExecutor_HasAttackOverride(ONtCharacter *ioCharacter, UUtUns16 *outAnimType)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return UUcFalse;

	executor = &active_character->executor_state;

	if (executor->attack_override) {
		if (outAnimType != NULL) {
			*outAnimType = executor->attack_override_desiredanimtype;
		}
		return UUcTrue;
	} else {
		return UUcFalse;
	}
}

UUtBool AI2rExecutor_HasThrowOverride(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return UUcFalse;

	return active_character->executor_state.throw_override;
}

UUtBool AI2rExecutor_TryingToThrow(ONtCharacter *ioCharacter, ONtCharacter **outTarget,
								   const TRtAnimation **outThrow, const TRtAnimation **outTargetThrow)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return UUcFalse;

	executor = &active_character->executor_state;
	if (!executor->throw_override) {
		return UUcFalse;
	}

	*outTarget = executor->throw_target;
	*outThrow = executor->throw_anim;
	*outTargetThrow = executor->throw_targetanim;
	return UUcTrue;
}

void AI2rExecutor_AimStop(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	if (executor->aiming) {
		// stop aiming (interpolate back to midpoint)
		executor->aiming_stopping = UUcTrue;
	}
}

void AI2rExecutor_AimTarget(ONtCharacter *ioCharacter, M3tPoint3D *inLocation, UUtBool inAimWeapon)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	executor->aiming = UUcTrue;
	executor->aiming_at_target = UUcTrue;
	executor->aiming_with_weapon = inAimWeapon;
	MUmVector_Copy(executor->aim_vector, *inLocation);
}

void AI2rExecutor_AimDirection(ONtCharacter *ioCharacter, M3tVector3D *inDirection, UUtBool inAimWeapon)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	executor->aiming = UUcTrue;
	executor->aiming_at_target = UUcFalse;
	executor->aiming_with_weapon = inAimWeapon;
	MUmVector_Copy(executor->aim_vector, *inDirection);
}

UUtBool AI2rExecutor_AimReachedTarget(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return UUcTrue;
	executor = &active_character->executor_state;

	return executor->aim_reached_target;
}

void AI2rExecutor_SetAimingSpeed(ONtCharacter *ioCharacter, float inAimingSpeedScale)
{
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	executor->aiming_speed_scale = inAimingSpeedScale;
}

void AI2rExecutor_SetCurrentState(ONtCharacter *ioCharacter)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(ioCharacter);
	AI2tExecutorState *executor;

	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	if (active_character->isAiming) {
		// we must take the current state of our aiming, because otherwise our aiming
		// interpolation might be incorrect at the end of a playback. we are playing a film,
		// so we'll be using aim_vector. make sure that aimingTarget is updated so that when we
		// switch back to using aim_target we don't have garbage in there.
		executor->aiming = UUcTrue;
		executor->aim_vector = active_character->aimingVector;

		MUmVector_Copy(active_character->aimingTarget, ioCharacter->location);
		active_character->aimingTarget.y += ONcCharacterHeight;
		MUmVector_Add(active_character->aimingTarget, active_character->aimingTarget, active_character->aimingVector);
	} else {
		executor->aiming = UUcFalse;
	}
}

void AI2rExecutor_UpdateAim(ONtCharacter *ioCharacter)
{
	M3tVector3D desired_aiming_vector, charfrom_vector;
	float cur_theta, cur_phi, desired_theta, desired_phi, aim_horiz, aim_length;
	float delta_theta, delta_phi, rotate, rotate_frac, rotate_max, cos_phi;
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;

	executor = &active_character->executor_state;

	if (executor->aiming) {
		MUmVector_Copy(charfrom_vector, ioCharacter->location);
		charfrom_vector.y += ONcCharacterHeight;

		if (active_character->isAiming) {
			// get the last aiming location
			cur_theta = active_character->aimingVectorLR;
			if (executor->last_actual_facing != AI2cAngle_None) {
				// adjust theta for any changes in facing since the last frame
				cur_theta += (ioCharacter->facing - executor->last_actual_facing);
				UUmTrig_Clip(cur_theta);
			}

			cur_phi = active_character->aimingVectorUD;
			UUmTrig_ClipAbsPi(cur_phi);

			UUmAssertTrigRange(cur_theta);
			UUmAssert((float)fabs(cur_phi) <= M3cPi);
		} else {
			// current aiming is the character's facing
			cur_theta = ioCharacter->facing;
			cur_phi = 0.0f;
		}

		if (executor->aiming_stopping) {
			// we are trying to stop aiming - i.e. returning to the center
			desired_theta = ioCharacter->facing;
			desired_phi = 0.0f;
			aim_length = 100.0f;
		} else {
			// calculate the desired aiming vector
			if (executor->aiming_at_target) {
				MUmVector_Subtract(desired_aiming_vector, executor->aim_vector, charfrom_vector);
			} else {
				MUmVector_Copy(desired_aiming_vector, executor->aim_vector);
			}

			desired_theta = MUrATan2(desired_aiming_vector.x, desired_aiming_vector.z);
			UUmTrig_ClipLow(desired_theta);
			aim_horiz = UUmSQR(desired_aiming_vector.x) + UUmSQR(desired_aiming_vector.z);
			desired_phi = MUrATan2(desired_aiming_vector.y, MUrSqrt(aim_horiz));
			aim_length = MUrSqrt(aim_horiz + UUmSQR(desired_aiming_vector.y));

			// work out how far this is from our current facing
			delta_theta = desired_theta - ioCharacter->facing;
			UUmTrig_ClipAbsPi(delta_theta);
			if (fabs(delta_theta) > AI2cExecutor_AbsoluteMaxAimingAngle) {
				// we cannot aim around this far, stop aiming
				desired_theta = ioCharacter->facing;
				desired_phi = 0.0f;
				aim_length = 100.0f;
				MUmVector_ScaleCopy(desired_aiming_vector, aim_length, ioCharacter->facingVector);
			}
		}

		// work out how far we have to rotate
		// FIXME: use quaternions here for better results?
		delta_theta = desired_theta - cur_theta;
		UUmTrig_ClipAbsPi(delta_theta);

		delta_phi = desired_phi - cur_phi;
		rotate = MUrSqrt(UUmSQR(delta_theta) + UUmSQR(delta_phi));

		// can we rotate this much?
		rotate_max = AI2cExecutor_MaxDeltaAimingPerFrame * executor->aiming_speed_scale;
		if ((ioCharacter->inventory.weapons[0] != NULL) && (executor->aiming_with_weapon)) {
			rotate_max *= WPrGetClass(ioCharacter->inventory.weapons[0])->aimingSpeed;
		}

		if (rotate <= rotate_max) {
			executor->aim_reached_target = UUcTrue;
			if (executor->aiming_stopping) {
				// we have returned to the center!
				executor->aiming_stopping = UUcFalse;
				executor->aiming = UUcFalse;
				active_character->isAiming = UUcFalse;
			} else {
				// we can point directly at the target
				MUmVector_Add(active_character->aimingTarget, desired_aiming_vector, charfrom_vector);
				active_character->isAiming = UUcTrue;
			}
		} else {
			// we must interpolate between the desired and current rotations
			executor->aim_reached_target = UUcFalse;
			rotate_frac = rotate_max / rotate;

			desired_theta = cur_theta + rotate_frac * delta_theta;
			desired_phi = cur_phi + rotate_frac * delta_phi;

			UUmTrig_Clip(desired_theta);
			UUmTrig_Clip(desired_phi);

			// calculate the new aiming vector
			cos_phi = MUrCos(desired_phi);
			desired_aiming_vector.x = aim_length * MUrSin(desired_theta) * cos_phi;
			desired_aiming_vector.y = aim_length * MUrSin(desired_phi);
			desired_aiming_vector.z = aim_length * MUrCos(desired_theta) * cos_phi;
	
			// set the character's aiming vector
			MUmVector_Add(active_character->aimingTarget, desired_aiming_vector, charfrom_vector);
			active_character->isAiming = UUcTrue;
		}
	} else {
		active_character->isAiming = UUcFalse;
		executor->aim_reached_target = UUcTrue;
	}

	executor->last_actual_facing = ioCharacter->facing;
}

// report in
void AI2rExecutor_Report(ONtCharacter *ioCharacter, UUtBool inVerbose, AI2tReportFunction inFunction)
{
	char reportbuf[256];
	ONtActiveCharacter *active_character;
	AI2tExecutorState *executor;

	UUmAssert(ioCharacter->flags & ONcCharacterFlag_AIMovement);
	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL)
		return;
	executor = &active_character->executor_state;

	if (executor->movement_direction == AI2cMovementDirection_Stopped) {
		if (executor->actual_movement_mode == AI2cMovementMode_Creep) {
			strcpy(reportbuf, "  crouching");
		} else {
			strcpy(reportbuf, "  standing");
		}
	} else {
		switch(executor->actual_movement_mode)
		{
			case AI2cMovementMode_Creep:
				sprintf(reportbuf, "  creeping %s", AI2cMovementDirectionName[executor->movement_direction]);
			break;

			case AI2cMovementMode_NoAim_Walk:
			case AI2cMovementMode_Walk:
				sprintf(reportbuf, "  walking %s", AI2cMovementDirectionName[executor->movement_direction]);
			break;

			case AI2cMovementMode_NoAim_Run:
			case AI2cMovementMode_Run:
				sprintf(reportbuf, "  running %s", AI2cMovementDirectionName[executor->movement_direction]);
			break;

			case AI2cMovementMode_Default:
			default:
				sprintf(reportbuf, "  <invalid movement mode %d>", executor->actual_movement_mode);
			break;
		}
	}
	inFunction(reportbuf);

	if (executor->aiming) {
		if (executor->aiming_at_target) {
			sprintf(reportbuf, "  %sing at %f %f %f", executor->aiming_with_weapon ? "aim" : "look",
					executor->aim_vector.x, executor->aim_vector.y, executor->aim_vector.z);
		} else {
			sprintf(reportbuf, "  %sing in direction %f %f %f", executor->aiming_with_weapon ? "aim" : "look",
					executor->aim_vector.x, executor->aim_vector.y, executor->aim_vector.z);
		}
		if (executor->aiming_stopping) {
			strcat(reportbuf, " (returning to stop)");
		}
	} else {
		sprintf(reportbuf, "  looking ahead");
	}
	inFunction(reportbuf);
}
