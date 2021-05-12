#pragma once
#ifndef ONI_AI2_MOVEMENTSTUB_H
#define ONI_AI2_MOVEMENTSTUB_H

/*
	FILE:	Oni_AI2_MovementStub.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: July 29, 2000
	
	PURPOSE: Non-incarnated Movement AI for Oni
	
	Copyright (c) Bungie Software, 2000

*/

#include "BFW.h"
#include "Oni_AStar.h"
#include "Oni_AI.h"
#include "Oni_AI2_Script.h"
#include "Oni_Path.h"

// ------------------------------------------------------------------------------------
// -- movement stub definitions

/* current state of a non-incarnated AI's movement subsystem */
typedef struct AI2tMovementStub
{
	/*
	 * internal state
	 */

	// the path that we are currently following across a node
	UUtBool					path_active;
	M3tPoint3D				path_start;
	M3tPoint3D				path_end;
	float					path_distance;
	float					cur_distance;

	// simulated character state
	UUtUns32				findfloor_timer;
	UUtUns32				cur_floor_gq;
	float					cur_facing;
	M3tPoint3D				cur_point;

} AI2tMovementStub;

// ------------------------------------------------------------------------------------
// -- movement routines

/*
 * interface with high-level path subsystem
 */

// command routines
void	AI2rMovementStub_NewDestination(ONtCharacter *ioCharacter);
void	AI2rMovementStub_ClearPath(ONtCharacter *ioCharacter);
UUtBool AI2rMovementStub_AdvanceThroughGrid(ONtCharacter *ioCharacter);
void	AI2rMovementStub_Update(ONtCharacter *ioCharacter);
UUtBool AI2rMovementStub_DestinationFacing(ONtCharacter *ioCharacter);
UUtBool AI2rMovementStub_CheckFailure(ONtCharacter *ioCharacter);
UUtBool AI2rMovementStub_MakePath(ONtCharacter *ioCharacter);

// query functions
float	AI2rMovementStub_GetMoveDirection(ONtCharacter *ioCharacter);

/*
 * debugging routines
 */

// report in
void AI2rMovementStub_Report(ONtCharacter *ioCharacter, UUtBool inVerbose, AI2tReportFunction inFunction);

// debugging display
void AI2rMovementStub_RenderPath(ONtCharacter *ioCharacter);


// ------------------------------------------------------------------------------------
// -- unincarnated-specific movement-stub routines

void AI2rMovementStub_Initialize(ONtCharacter *ioCharacter, AI2tMovementState *inState);

#endif  // ONI_AI2_MOVEMENTSTUB_H
