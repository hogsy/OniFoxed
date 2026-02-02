#pragma once
#ifndef ONI_AI2_GUARD_H
#define ONI_AI2_GUARD_H

/*
	FILE:	Oni_AI2_Guard.h

	AUTHOR:	Chris Butcher

	CREATED: June 22, 2000

	PURPOSE: Guard AI for Oni

	Copyright (c) 2000

*/

// ------------------------------------------------------------------------------------
// -- structures

typedef struct AI2tGuardDirState
{
	float		face_direction;
	float		initial_direction;

	UUtBool		has_cur_direction;
	UUtBool		cur_direction_isexit;
	// FIXME: current exit index
	float		cur_direction_angle;

	M3tVector3D	cur_direction;

	// FIXME: current room, list of exits and visibility status

} AI2tGuardDirState;

typedef struct AI2tScanState
{
	UUtBool		stay_facing;
	float		stay_facing_direction;
	float		stay_facing_range;

	AI2tGuardDirState dir_state;

	float		cur_angle;
	float		angle_range;
	M3tVector3D facing_vector;

	UUtUns32	direction_minframes, direction_deltaframes;
	UUtUns32	angle_minframes, angle_deltaframes;

	UUtUns32	direction_timer;
	UUtUns32	angle_timer;

} AI2tScanState;

typedef struct AI2tGuardState
{
	UUtInt32	flag;
} AI2tGuardState;

// ------------------------------------------------------------------------------------
// -- scanning functions

void AI2rGuard_ResetScanState(AI2tScanState *ioScanState, float inInitialDirection);

void AI2rGuard_NewDirection(ONtCharacter *ioCharacter, AI2tGuardDirState *ioDirState, float inDeltaDir);

void AI2rGuard_UpdateScan(ONtCharacter *ioCharacter, AI2tScanState *ioScanState);

void AI2rGuard_RecalcScanDir(ONtCharacter *ioCharacter, AI2tScanState *ioScanState);

#endif // ONI_AI2_GUARD_H
