#pragma once
#ifndef ONI_AI2_EXECUTOR_H
#define ONI_AI2_EXECUTOR_H

/*
	FILE:	Oni_AI2_Executor.h
	
	AUTHOR:	Michael Evans, Chris Butcher
	
	CREATED: November 15, 1999
	
	PURPOSE: Low Level AI for Oni
	
	Copyright (c) 1999

*/

#include "BFW.h"
#include "Oni_GameState.h"
#include "Oni_AI2_Movement.h"
#include "BFW_Totoro_Private.h"

// absolute maximum aiming angle
#define AI2cExecutor_AbsoluteMaxAimingAngle			(130.0f * M3cDegToRad)

// 100 frames for a complete turn. weighted by class turn speed.
#define	AI2cExecutor_MaxDeltaFacingPerFrame			M3c2Pi / 100

// characters are half as fast to turn when walking or creeping
#define AI2cExecutor_FacingSpeedScaleWhenWalking	0.5f

// 60 frames for a complete turn. weighted by weapon aiming speed.
#define	AI2cExecutor_MaxDeltaAimingPerFrame			M3c2Pi / 60

typedef struct AI2tExecutorState
{
	UUtBool aiming, aiming_stopping, aiming_at_target, aiming_with_weapon;
	M3tPoint3D aim_vector;		// position or direction depending on value of aiming_at_target
	AI2tMovementMode movement_mode;
	AI2tMovementDirection movement_direction;
	AI2tMovementMode actual_movement_mode;
	float facing;
	float turn_override;
	float last_actual_facing;
	UUtBool turn_danger;		// forces a fast turn (e.g. when glancing in direction of a gunshot, or in melee)
	float aiming_speed_scale;	// used to slow aiming while actually in combat and shooting
	UUtBool aim_reached_target;

	// overridden movement by higher levels of the system
	UUtBool move_override;
	LItButtonBits move_override_keys;

	float facing_override;

	UUtBool attack_override;
	UUtUns16 attack_override_desiredanimtype;
	UUtUns16 attack_override_requiredcombotype;
	LItButtonBits attack_override_keys_isdown;
	LItButtonBits attack_override_keys_wentdown;

	UUtBool throw_override;
	ONtCharacter *throw_target;
	TRtAnimation *throw_anim;
	TRtAnimation *throw_targetanim;

} AI2tExecutorState;

void AI2rExecutor_Update(ONtCharacter *ioCharacter);
void AI2rExecutor_TurnOverride(ONtCharacter *ioCharacter, float override);
void AI2rExecutor_UpdateAim(ONtCharacter *ioCharacter);
void AI2rExecutor_Initialize(ONtCharacter *ioCharacter);
void AI2rExecutor_SetCurrentState(ONtCharacter *ioCharacter);

// manipulate the character's movements
void AI2rExecutor_Move(ONtCharacter *ioCharacter, float inFacing, AI2tMovementMode inMovementMode,
						AI2tMovementDirection inDirection, UUtBool turn_danger);
void AI2rExecutor_AimStop(ONtCharacter *ioCharacter);
void AI2rExecutor_AimTarget(ONtCharacter *ioCharacter, M3tPoint3D *inLocation, UUtBool inAimWeapon);
void AI2rExecutor_AimDirection(ONtCharacter *ioCharacter, M3tVector3D *inDirection, UUtBool inAimWeapon);
float AI2rExecutor_GetMaxDeltaFacingPerFrame(ONtCharacterClass *inClass, AI2tMovementMode inMovementMode, UUtBool turn_danger);
void AI2rExecutor_SetAimingSpeed(ONtCharacter *ioCharacter, float inAimingSpeedScale);

// override commands issued through the above routines
void AI2rExecutor_MoveOverride(ONtCharacter *ioCharacter, LItButtonBits inKeys);
void AI2rExecutor_FacingOverride(ONtCharacter *ioCharacter, float inFacing, UUtBool inDanger);
void AI2rExecutor_AttackOverride(ONtCharacter *ioCharacter, LItButtonBits inKeysIsDown, LItButtonBits inKeysWentDown,
								 UUtUns16 inDesiredAnimType, UUtUns16 inRequiredComboType);
void AI2rExecutor_ThrowOverride(ONtCharacter *ioCharacter, ONtCharacter *inTarget,
								TRtAnimation *inThrow, TRtAnimation *inTargetThrow);
void AI2rExecutor_ClearAttackOverride(ONtCharacter *ioCharacter);

// query functions
UUtBool AI2rExecutor_AimReachedTarget(ONtCharacter *ioCharacter);
UUtBool AI2rExecutor_HasAttackOverride(ONtCharacter *ioCharacter, UUtUns16 *outAnimType);
UUtBool AI2rExecutor_HasThrowOverride(ONtCharacter *ioCharacter);
UUtBool AI2rExecutor_TryingToThrow(ONtCharacter *ioCharacter, ONtCharacter **outTarget,
								   const TRtAnimation **outThrow, const TRtAnimation **outTargetThrow);
AI2tMovementMode AI2rExecutor_ActualMovementMode(ONtCharacter *ioCharacter);

// stop the character - called by movement layer when not moving
static void UUcInline AI2rExecutor_Stop(ONtCharacter *ioCharacter, float inFacing, AI2tMovementMode inMovementMode) {
	AI2rExecutor_Move(ioCharacter, inFacing, inMovementMode, AI2cMovementDirection_Stopped, UUcFalse);
}

// report in
void AI2rExecutor_Report(ONtCharacter *ioCharacter, UUtBool inVerbose, AI2tReportFunction inFunction);

#endif  // ONI_AI2_EXECUTOR_H