#pragma once
#ifndef ONI_AI2_MOVEMENTSTATE_H
#define ONI_AI2_MOVEMENTSTATE_H

/*
	FILE:	Oni_AI2_MovementState.h

	AUTHOR:	Chris Butcher

	CREATED: July 29, 2000

	PURPOSE: Incarnated Movement AI for Oni

	Copyright (c) Bungie Software, 2000

*/

#include "BFW.h"
#include "Oni_AStar.h"
#include "Oni_AI.h"
#include "Oni_AI2_Script.h"
#include "Oni_Path.h"

// ------------------------------------------------------------------------------------
// -- movement state definitions

/* collision handling */

enum {
	AI2cCollisionFlag_Hit			= (1 << 0),
	AI2cCollisionFlag_Miss			= (1 << 1),
	AI2cCollisionFlag_Serious		= (1 << 2)
};

typedef enum AI2tCollisionType {
	AI2cCollisionType_Wall			= 0,
	AI2cCollisionType_Physics		= 1,
	AI2cCollisionType_Character		= 2,

	AI2cCollisionType_Max			= 3
} AI2tCollisionType;

typedef struct AI2tCollisionWallData {
	UUtUns32			gq_index;
	UUtUns32			plane_index;
} AI2tCollisionWallData;

typedef struct AI2tCollisionIdentifier {
	AI2tCollisionType type;
	union {
		AI2tCollisionWallData	wall;
		PHtPhysicsContext *		physics_context;
		ONtCharacter *			character;
	} data;
} AI2tCollisionIdentifier;

typedef struct AI2tCollision {
	UUtUns32				flags;
	AI2tCollisionIdentifier	collision_id;

	M3tPoint3D				last_hit_point;			// our location at the last hit time
	M3tPoint3D				last_collision_point;	// the last point that we hit
	M3tPlaneEquation		collision_plane;

	UUtUns16				frame_counter;			// either hit frames or miss frames, depending on flags
	UUtUns16				total_hit_frames;
	float					direction;
	float					weight;
} AI2tCollision;

typedef struct AI2tBadnessValue {
	float				value;
	float				val_increment;
	float				angle;
	UUtUns32			next_index;
} AI2tBadnessValue;

#define AI2cMax_PathPoints					256
#define AI2cMovementState_MaxCollisions		16
#define AI2cMovementState_MaxBadnessValues	4 * AI2cMovementState_MaxCollisions

/* current state of an incarnated AI's movement subsystem */
typedef struct AI2tMovementState {
	// internal representation of executor state
	UUtBool					aiming_weapon;

	// current means of motion, and direction of motion
	AI2tMovementDirection	movementDirection;
	UUtBool					invalid_movement_direction;	// set if we try to sidestep and discover we don't have that anim
	UUtBool					no_walking_sidestep;
	UUtBool					no_running_sidestep;
	UUtBool					desire_forwards_if_no_sidestep;
	UUtBool					keepmoving;
	UUtUns32				keepmoving_timer;

	// current collisions with walls that we must avoid
	UUtUns32				numCollisions;
	UUtUns32				numSeriousCollisions;
	AI2tCollision			collision[AI2cMovementState_MaxCollisions];
	UUtBool					collision_stuck;

	// additional movement modifier information
	UUtUns32				numActiveModifiers;			// doesn't count those turned off by collision
	UUtBool					modifiers_require_decreasing;

	// information about our current path point
	AStPathPoint			*next_path_point;
	M3tVector3D				vector_to_point;
	float					distance_to_path_point_squared;

	// information about our current facing directions
	// may be AI2cDirection_None
	float					move_direction;				// persistent info used for pass-through waypoints
	float					last_real_move_direction;	// last move_direction that wasn't AI2cAngle_None
	float					turning_to_face_direction;	// only used for standing still and facing
	AI2tMovementDirection	current_direction;			// sidestepping, moving forwards, etc
	UUtBool					done_dest_facing;

	// information about any temporary slowdowns in our movement
	UUtBool					temporarily_slowed;
	AI2tMovementMode		temporary_mode;

	// information about whether we have temporarily stopped to turn on the spot
	UUtBool					turning_on_spot;

	// information about whether we are halted to prevent overshoot
	UUtBool					tried_overshoot;
	UUtUns16				overshoot_timer;

	// timer so that we don't try overturning corrections while we're changing direction
	UUtUns16				overturn_disable_timer;

	// path on the small pathfinding grid
	M3tPoint3D				grid_path_start;
	M3tPoint3D				grid_path_end;
	AStPathPoint			grid_path[AI2cMax_PathPoints];
	UUtUns16				grid_num_points;
	UUtUns16				grid_current_point;
	PHtRoomData	*			grid_room;
	PHtRoomData	*			grid_last_room;
	UUtBool					grid_failure;

#if TOOL_VERSION
	// debugging storage of our badness ring, for collision display
	UUtUns32				numBadnessValues;
	AI2tBadnessValue		badnesslist[AI2cMovementState_MaxBadnessValues];

	// debugging storage of our last activation point
	UUtBool					activation_valid, activation_frompath;
	M3tPoint3D				activation_rawpoint;
	M3tPoint3D				activation_position;
	M3tPoint3D				activation_pathsegment[2];
#endif

} AI2tMovementState;

// ------------------------------------------------------------------------------------
// -- movement routines

/*
 * interface with movement control subsystem
 */

// movement modifiers
void AI2rMovementState_NewMovementModifier(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, UUtUns32 inNewIndex);
void AI2rMovementState_NewAvoidAngle(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);

// change movement mode
void AI2rMovementState_NewMovementMode(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
void AI2rMovementState_NotifyAlertChange(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
void AI2rMovementState_ResetAimingVarient(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);

// query functions
UUtBool AI2rMovementState_IsKeepingMoving(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
float AI2rMovementState_GetMoveDirection(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
UUtBool AI2rMovementState_SimplePath(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);

/*
 * interface with high-level path subsystem
 */

// command routines
void	AI2rMovementState_NewPathReceived(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
void	AI2rMovementState_Halt(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
void	AI2rMovementState_NewDestination(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
void	AI2rMovementState_ClearPath(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
UUtBool AI2rMovementState_AdvanceThroughGrid(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
void	AI2rMovementState_Update(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
UUtBool AI2rMovementState_DestinationFacing(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
UUtBool AI2rMovementState_CheckFailure(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
UUtBool AI2rMovementState_MakePath(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, UUtBool inReusePath);

/*
 * debugging routines
 */

// report in
void AI2rMovementState_Report(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, UUtBool inVerbose, AI2tReportFunction inFunction);

// debugging display
void AI2rMovementState_RenderPath(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
void AI2rMovementState_RenderCollision(M3tPoint3D *inPoint, UUtUns32 inNumCollisions, AI2tCollision *inCollisions,
									   UUtUns32 inNumBadnessValues, AI2tBadnessValue *inBadnessValues);


// ------------------------------------------------------------------------------------
// -- incarnation-specific movement-state routines

struct AI2tMovementStub;
void AI2rMovementState_Initialize(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState,
								  struct AI2tMovementStub *inStub);
UUtError AI2rMovementState_NotifyWallCollision(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, UUtUns32 inGQIndex, UUtUns32 inPlaneIndex,
											   M3tPoint3D *inPoint, M3tPlaneEquation *inPlane);
UUtError AI2rMovementState_NotifyPhysicsCollision(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, PHtPhysicsContext *inContext,
											   M3tPoint3D *inPoint, M3tPlaneEquation *inPlane);
UUtError AI2rMovementState_NotifyCharacterCollision(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, ONtCharacter *inCollidingCharacter,
											   M3tPoint3D *inPoint, M3tPlaneEquation *inPlane);
UUtBool AI2rMovementState_ForwardMovementPreference(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState);
void AI2rMovementState_SidestepNotFound(ONtCharacter *ioCharacter, AI2tMovementState *ioMovementState, UUtBool inWalking);

#endif  // ONI_AI2_MOVEMENTSTATE_H
