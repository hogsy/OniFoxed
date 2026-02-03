#pragma once
#ifndef ONI_AI2_PATH_H
#define ONI_AI2_PATH_H

/*
	FILE:	Oni_AI2_Path.h

	AUTHOR:	Chris Butcher

	CREATED: July 29, 2000

	PURPOSE: High-level Pathfinding AI for Oni

	Copyright (c) Bungie Software, 2000

*/

#include "BFW.h"
#include "Oni_AStar.h"
#include "Oni_AI.h"
#include "Oni_AI2_Script.h"
#include "Oni_Path.h"

#define AI_VERBOSE_PATHFAILURE			0
#define DEBUG_PATH_REUSE				0

// ------------------------------------------------------------------------------------
// -- structures and definitions

/* our current movement orders */
typedef enum AI2tDestinationType
{
	AI2cDestinationType_None			= 0,
	AI2cDestinationType_Point			= 1,
	AI2cDestinationType_FollowCharacter	= 2,
	AI2cDestinationType_Max				= 3
} AI2tDestinationType;

typedef struct AI2tDestinationData_Point {
	float				required_distance;	// may be zero for 'hit as exactly as possible'
	UUtBool				stop_at_point;
	float				final_facing;		// only used if stop_at_point. may be AI2cAngle_None for 'no facing'
} AI2tDestinationData_Point;

typedef struct AI2tDestinationData_FollowCharacter {
	ONtCharacter *		character;
} AI2tDestinationData_FollowCharacter;

typedef union AI2tDestinationData {
	AI2tDestinationData_Point			point_data;
	AI2tDestinationData_FollowCharacter	followcharacter_data;
} AI2tDestinationData;

/* current state of an AI's high-level path */
typedef struct AI2tPathState {
	/*
	 * goal settings
	 */

	// our current destination
	AI2tDestinationType		destinationType;
	AI2tDestinationData		destinationData;
	M3tPoint3D				destinationLocation;

	/*
	 * internal state
	 */

	// our currently computed path
	UUtUns32				pathComputeTime;
	AI2tDestinationType		pathDestinationType;
	AI2tDestinationData		pathDestinationData;
	M3tPoint3D				pathDestinationLocation;

	// current movement information
	UUtBool					at_destination;
	UUtBool					at_finalpoint;
	UUtBool					prev_at_destination;
	M3tPoint3D				prev_destination;
	float					distance_to_destination_squared;
	float					distance_from_start_to_dest_squared;
	struct OBJtObject *		door_to_open;
	UUtBool					moving_onto_stairs;

	// High-level path
	UUtUns32				path_num_nodes;
	UUtUns32				path_current_node;
	M3tPoint3D				path_start_location;
	M3tPoint3D				path_end_location;
	PHtNode	*				path_start_node;
	PHtNode	*				path_end_node;
	PHtConnection *			path_connections[PHcMaxPathLength-1];

	// error handling
	UUtUns32				last_pathfinding_error;
	UUtUns32				forced_repath_timer;
	UUtUns32				forced_repath_count;
	UUtUns32				forced_repath_decaytimer;
	UUtUns32				failed_path_time;

} AI2tPathState;

// ------------------------------------------------------------------------------------
// -- pathing routines

// global routines
void AI2rPath_Initialize(ONtCharacter *ioCharacter);
void AI2rPath_Update(ONtCharacter *ioCharacter);

// destination setting
void AI2rPath_GoToPoint(ONtCharacter *ioCharacter, const M3tPoint3D *inPoint, float inDistance, UUtBool inStopAtPoint,
						float inStopFacing, ONtCharacter *inIgnoreChar);
void AI2rPath_FollowCharacter(ONtCharacter *ioCharacter, ONtCharacter *inFollow);
void AI2rPath_Halt(ONtCharacter *ioCharacter);
void AI2rPath_Pause(ONtCharacter *ioCharacter, UUtUns32 inPause);
UUtBool AI2rPath_Repath(ONtCharacter *ioCharacter, UUtBool inForce);

// interface with movement subsystem
void AI2rPath_PathError(ONtCharacter *ioCharacter);
void AI2rPath_FindGhostWaypoint(ONtCharacter *ioCharacter, M3tPoint3D *inStartPoint, UUtUns32 inCurrentNode, M3tPoint3D *outPoint);
UUtBool AI2rPath_StopAtDest(ONtCharacter *ioCharacter, float *outDestRadius);
UUtBool AI2rPath_SimplePath(ONtCharacter *ioCharacter);

// ------------------------------------------------------------------------------------
// -- debugging routines

extern UUtBool AI2gDrawPaths;
extern UUtBool AI2gIgnorePathError;

// draw the character's path
void AI2rPath_RenderPath(ONtCharacter *ioCharacter);

// report in
void AI2rPath_Report(ONtCharacter *ioCharacter, UUtBool inVerbose, AI2tReportFunction inFunction);

#endif  // ONI_AI2_PATH_H
