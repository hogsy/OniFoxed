/*
	FILE:	Oni_AI2_Guard.c

	AUTHOR:	Chris Butcher

	CREATED: June 22, 2000

	PURPOSE: Guard AI for Oni

	Copyright (c) 2000

*/

#include "bfw_math.h"

#include "Oni_AI2.h"
#include "Oni_Character.h"

#define DEBUG_VERBOSE_GUARD			0

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cGuard_MinimumAngleDelta		(40.0f * M3cDegToRad)
#define AI2cGuard_ClearDistance			30.0f
#define AI2cGuard_MaxClearAttempts		6

// ------------------------------------------------------------------------------------
// -- utility functions

void AI2rGuard_ResetScanState(AI2tScanState *ioScanState, float inInitialDirection)
{
	ioScanState->dir_state.has_cur_direction = UUcFalse;
	ioScanState->dir_state.initial_direction = inInitialDirection;

	ioScanState->direction_timer = 0;
	ioScanState->angle_timer = 0;
	ioScanState->cur_angle = 0;
	ioScanState->stay_facing = UUcFalse;

#if DEBUG_VERBOSE_GUARD
	COrConsole_Printf("scan state reset");
#endif
}

void AI2rGuard_NewDirection(ONtCharacter *ioCharacter, AI2tGuardDirState *ioDirState, float inDeltaDir)
{
	UUtUns32 attempts = 0;
	AKtEnvironment *environment = ONrGameState_GetEnvironment();
	float try_direction, avoid_direction, delta_theta;
	M3tPoint3D eye_point;
	UUtBool try_facing = UUcFalse;

	ONrCharacter_GetEyePosition(ioCharacter, &eye_point);

	// FIXME: determine whether we want to look at an exit or just in a random direction

	// FIXME: get the list of exits from our current BNV, and classify each as visible w.r.t. this move direction
	// and also perform LOS

	// we have to pick a random direction; try not to pick the same direction as last time
	if (ioDirState->has_cur_direction) {
		if (ioDirState->cur_direction_isexit) {
			UUmAssert(0);
		} else {
			avoid_direction = ioDirState->cur_direction_angle;
		}
	} else {
		// this is our first direction! just use the character's facing
		avoid_direction = AI2cAngle_None;
		try_facing = UUcTrue;
	}

	while (1) {
		if (try_facing) {
			try_direction = (ioDirState->initial_direction == AI2cAngle_None)
					? ioCharacter->facing : ioDirState->initial_direction;
			try_facing = UUcFalse;		// only face this way once
		} else {
			// find a random direction to look in
			if (ioDirState->face_direction == AI2cAngle_None) {
				try_direction = UUmRandomFloat * M3c2Pi;
			} else {
				UUmAssert(inDeltaDir != AI2cAngle_None);
				try_direction = ioDirState->face_direction + UUmRandomRangeFloat(-inDeltaDir, +inDeltaDir);
			}
		}

		if (attempts < AI2cGuard_MaxClearAttempts) {
			// don't use directions that are too close to where we're already looking
			delta_theta = try_direction - avoid_direction;
			UUmTrig_ClipAbsPi(delta_theta);
			if ((float)fabs(delta_theta) < AI2cGuard_MinimumAngleDelta) {
				attempts++;
				continue;
			}
		}

		// form this look vector
		UUmTrig_Clip(try_direction);
		MUmVector_Set(ioDirState->cur_direction, AI2cGuard_ClearDistance * MUrSin(try_direction), 0,
												 AI2cGuard_ClearDistance * MUrCos(try_direction));

		if ((attempts >= AI2cGuard_MaxClearAttempts) ||
			(!AKrCollision_Point(environment, &eye_point, &ioDirState->cur_direction, AKcGQ_Flag_LOS_CanSee_Skip_AI, 0))) {
			// return this direction
			ioDirState->has_cur_direction = UUcTrue;
			ioDirState->cur_direction_isexit = UUcFalse;
			ioDirState->cur_direction_angle = try_direction;
			return;
		}

		// increment the number of attempts to avoid getting stuck
		attempts++;
	}
}

void AI2rGuard_UpdateScan(ONtCharacter *ioCharacter, AI2tScanState *ioScanState)
{
	UUtBool changed = UUcFalse;

	if ((!ioScanState->dir_state.has_cur_direction) || (ioScanState->direction_timer == 0)) {
		// find a new direction to look in
		if (ioScanState->stay_facing) {
			if (ioScanState->stay_facing_direction == AI2cAngle_None) {
				ioScanState->dir_state.face_direction = ioCharacter->facing;
			} else {
				ioScanState->dir_state.face_direction = ioScanState->stay_facing_direction;
			}
		} else {
			ioScanState->dir_state.face_direction = AI2cAngle_None;
		}
		AI2rGuard_NewDirection(ioCharacter, &ioScanState->dir_state, (ioScanState->stay_facing) ? ioScanState->stay_facing_range : AI2cAngle_None);
		changed = UUcTrue;

		if (!ioScanState->dir_state.has_cur_direction) {
			// error - just look straight ahead
			MUmVector_Set(ioScanState->dir_state.cur_direction, MUrSin(ioCharacter->facing), 0, MUrCos(ioCharacter->facing));
		}

		// reset both timers
		ioScanState->direction_timer = UUmRandomRange(ioScanState->direction_minframes, ioScanState->direction_deltaframes);
		ioScanState->angle_timer = UUmRandomRange(ioScanState->angle_minframes, ioScanState->angle_deltaframes);
	} else {
		ioScanState->direction_timer--;
	}

	if (ioScanState->angle_range > 0) {
		if (ioScanState->angle_timer == 0) {
			// find a new angle to look in
			ioScanState->cur_angle = ioScanState->angle_range * (2 * UUmRandomFloat - 1);
			ioScanState->angle_timer = UUmRandomRange(ioScanState->angle_minframes, ioScanState->angle_deltaframes);
			changed = UUcTrue;
		} else {
			ioScanState->angle_timer--;
		}
	}

	if (changed) {
		if (ioScanState->angle_range == 0) {
			// just look directly in the danger direction
			MUmVector_Copy(ioScanState->facing_vector, ioScanState->dir_state.cur_direction);
		} else {
			AI2rGuard_RecalcScanDir(ioCharacter, ioScanState);
		}
	}

	// look in this direction
	AI2rMovement_LookInDirection(ioCharacter, &ioScanState->facing_vector);
}

void AI2rGuard_RecalcScanDir(ONtCharacter *ioCharacter, AI2tScanState *ioScanState)
{
	float theta, horiz_dist;

	// perturb the danger direction horizontally by our current offset angle
	horiz_dist = MUrSqrt(UUmSQR(ioScanState->dir_state.cur_direction.x) +
						UUmSQR(ioScanState->dir_state.cur_direction.z));
	theta = MUrATan2(ioScanState->dir_state.cur_direction.x, ioScanState->dir_state.cur_direction.z);
	UUmTrig_ClipLow(theta);

	theta += ioScanState->cur_angle;
	UUmTrig_Clip(theta);

	ioScanState->facing_vector.x = horiz_dist * MUrSin(theta);
	ioScanState->facing_vector.z = horiz_dist * MUrCos(theta);
	ioScanState->facing_vector.y = ioScanState->dir_state.cur_direction.y;
}
