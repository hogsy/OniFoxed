#pragma once
#ifndef ONI_AI2_Patrol_H
#define ONI_AI2_Patrol_H

/*
	FILE:	Oni_AI2_Patrol.h

	AUTHOR:	Michael Evans

	CREATED: November 15, 1999

	PURPOSE: Patrol AI for Oni

	Copyright (c) 1999

*/

#include "BFW_ScriptLang.h"

#include "Oni_GameState.h"
#include "Oni_AI2_Movement.h"
#include "Oni_AI2_Guard.h"
#include "Oni_AI2_Targeting.h"

// ------------------------------------------------------------------------------------
// -- waypoint definitions - required for the Patrol Path objects

typedef enum AI2tWaypointType
{
	AI2cWaypointType_MoveToFlag			= 0,
	AI2cWaypointType_Stop				= 1,
	AI2cWaypointType_Pause				= 2,
	AI2cWaypointType_LookAtFlag			= 3,
	AI2cWaypointType_LookAtPoint		= 4,
	AI2cWaypointType_MoveAndFaceFlag	= 5,
	AI2cWaypointType_Loop				= 6,
	AI2cWaypointType_MovementMode		= 7,
	AI2cWaypointType_MoveToPoint		= 8,
	AI2cWaypointType_LookInDirection	= 9,
	AI2cWaypointType_MoveThroughFlag	= 10,
	AI2cWaypointType_MoveThroughPoint	= 11,
	AI2cWaypointType_StopLooking		= 12,
	AI2cWaypointType_FreeFacing			= 13,
	AI2cWaypointType_GlanceAtFlag		= 14,
	AI2cWaypointType_MoveNearFlag		= 15,
	AI2cWaypointType_LoopToWaypoint		= 16,
	AI2cWaypointType_Scan				= 17,
	AI2cWaypointType_StopScanning		= 18,
	AI2cWaypointType_Guard				= 19,
	AI2cWaypointType_TriggerScript		= 20,
	AI2cWaypointType_PlayScript			= 21,
	AI2cWaypointType_LockIntoPatrol		= 22,
	AI2cWaypointType_ShootAtFlag		= 23,
	AI2cWaypointType_Max				= 24
} AI2tWaypointType;


typedef struct AI2tWaypointData_MoveToFlag
{
	UUtInt16 flag;
} AI2tWaypointData_MoveToFlag;

typedef struct AI2tWaypointData_Stop
{
	UUtInt32 pad;
} AI2tWaypointData_Stop;

typedef struct AI2tWaypointData_Pause
{
	UUtInt32 count;
} AI2tWaypointData_Pause;

typedef struct AI2tWaypointData_LookAtFlag
{
	UUtInt16 lookFlag;
} AI2tWaypointData_LookAtFlag;

typedef struct AI2tWaypointData_LookAtPoint
{
	M3tPoint3D lookPoint;
} AI2tWaypointData_LookAtPoint;

typedef struct AI2tWaypointData_MoveAndFaceFlag
{
	UUtInt16 faceFlag;
} AI2tWaypointData_MoveAndFaceFlag;

typedef struct AI2tWaypointData_Loop
{
	UUtInt32 pad;
} AI2tWaypointData_Loop;

typedef struct AI2tWaypointData_MovementMode
{
	UUtUns32 newMode;
} AI2tWaypointData_MovementMode;

typedef struct AI2tWaypointData_MoveToPoint
{
	M3tPoint3D point;
} AI2tWaypointData_MoveToPoint;

typedef struct AI2tWaypointData_LookInDirection
{
	UUtUns32 facing;		// AI2tMovementDirection
} AI2tWaypointData_LookInDirection;

typedef struct AI2tWaypointData_MoveThroughFlag
{
	UUtInt16 flag;
	float required_distance;
} AI2tWaypointData_MoveThroughFlag;

typedef struct AI2tWaypointData_MoveThroughPoint
{
	M3tPoint3D point;
	float required_distance;
} AI2tWaypointData_MoveThroughPoint;

typedef struct AI2tWaypointData_GlanceAtFlag
{
	UUtInt16 glanceFlag;
	UUtInt32 glanceFrames;
} AI2tWaypointData_GlanceAtFlag;

typedef struct AI2tWaypointData_MoveNearFlag
{
	UUtInt16 flag;
	float required_distance;
} AI2tWaypointData_MoveNearFlag;

typedef struct AI2tWaypointData_LoopToWaypoint
{
	UUtInt32 waypoint_index;
} AI2tWaypointData_LoopToWaypoint;

typedef struct AI2tWaypointData_Scan
{
	UUtInt16 scan_frames;
	float angle;
} AI2tWaypointData_Scan;

typedef struct AI2tWaypointData_Guard
{
	UUtInt16 flag;
	UUtInt16 guard_frames;
	float max_angle;
} AI2tWaypointData_Guard;

typedef struct AI2tWaypointData_TriggerScript
{
	UUtInt16 script_number;
} AI2tWaypointData_TriggerScript;

typedef struct AI2tWaypointData_PlayScript
{
	UUtInt16 script_number;
} AI2tWaypointData_PlayScript;

typedef struct AI2tWaypointData_LockIntoPatrol
{
	UUtBool lock;
} AI2tWaypointData_LockIntoPatrol;

typedef struct AI2tWaypointData_ShootAtFlag
{
	UUtInt16 flag;
	UUtInt16 shoot_frames;
	float additional_inaccuracy;
} AI2tWaypointData_ShootAtFlag;

typedef union AI2tWaypointData
{
	AI2tWaypointData_MoveToFlag moveToFlag;
	AI2tWaypointData_Stop stop;
	AI2tWaypointData_Pause pause;
	AI2tWaypointData_LookAtFlag lookAtFlag;
	AI2tWaypointData_LookAtPoint lookAtPoint;
	AI2tWaypointData_MoveAndFaceFlag moveAndFaceFlag;
	AI2tWaypointData_Loop loop;
	AI2tWaypointData_MovementMode movementMode;
	AI2tWaypointData_MoveToPoint moveToPoint;
	AI2tWaypointData_LookInDirection lookInDirection;
	AI2tWaypointData_MoveThroughPoint moveThroughPoint;
	AI2tWaypointData_MoveThroughFlag moveThroughFlag;
	AI2tWaypointData_GlanceAtFlag glanceAtFlag;
	AI2tWaypointData_MoveNearFlag moveNearFlag;
	AI2tWaypointData_LoopToWaypoint loopToWaypoint;
	AI2tWaypointData_Scan scan;
	AI2tWaypointData_Guard guard;
	AI2tWaypointData_TriggerScript triggerScript;
	AI2tWaypointData_PlayScript playScript;
	AI2tWaypointData_LockIntoPatrol lockIntoPatrol;
	AI2tWaypointData_ShootAtFlag shootAtFlag;
} AI2tWaypointData;

typedef struct AI2tWaypoint AI2tWaypoint;

struct AI2tWaypoint
{
	AI2tWaypointType type;
	AI2tWaypointData data;
};

// ------------------------------------------------------------------------------------
// -- patrol object structure
enum {
	AI2cPatrolPathFlag_ReturnToNearest		= (1 << 0)
};

#define AI2cMax_WayPoints	64
typedef struct AI2tPatrolPath {
	char			name[SLcScript_MaxNameLength];
	UUtUns16		id_number;
	UUtUns16		flags;

	UUtUns32		num_waypoints;
	AI2tWaypoint	waypoints[AI2cMax_WayPoints];
} AI2tPatrolPath;


// the state of a patrolling AI
typedef struct AI2tPatrolState {
	AI2tPatrolPath  path;

	// path position
	UUtUns32		current_waypoint;
	UUtUns32		current_movement_waypoint;
	UUtUns32		pause_timer;
	UUtUns32		done_glance_waypoint;

	// path state
	UUtBool			locked_in;
	UUtBool			stationary_glance;
	UUtBool			on_path;
	UUtBool			return_to_prev_point;
	UUtBool			pathfinding_failed;
	UUtUns32		pathfinding_fail_count;
	M3tPoint3D		prev_point;

	// scan / guard behavior
	UUtBool			scanning;
	UUtBool			guarding;
	UUtBool			started_guarding;
	UUtUns32		scanguard_timer;
	AI2tScanState	scan_state;

	// playback
	void *			running_script;

	// shooting
	UUtBool			targeting_setup;
	UUtBool			run_targeting;
	UUtBool			trigger_pressed;
	float			targeting_inaccuracy;
	UUtUns32		targeting_timer;
	WPtWeapon *		current_weapon;
	AI2tTargetingState targeting;

} AI2tPatrolState;

// ------------------------------------------------------------------------------------
// -- update and state change functions

// set up a patrol state - called when the character goes into "patrol mode".
void AI2rPatrol_Enter(ONtCharacter *ioCharacter);

// called every tick while patrolling
void AI2rPatrol_Update(ONtCharacter *ioCharacter);

// exit a patrol state - called when the character goes out of "patrol mode".
void AI2rPatrol_Exit(ONtCharacter *ioCharacter);

// perform death shutdown - called when the character dies while in "patrol mode".
void AI2rPatrol_NotifyDeath(ONtCharacter *ioCharacter);

// initialize for a new patrol
void AI2rPatrol_Reset(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState);

// handle pathfinding errors
UUtBool AI2rPatrol_PathfindingHandler(ONtCharacter *inCharacter, UUtUns32 inErrorID,
										UUtUns32 inParam1, UUtUns32 inParam2,
										UUtUns32 inParam3, UUtUns32 inParam4);
// pause a patrol for some number of frames
void AI2rPatrol_Pause(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState, UUtUns32 inPauseFrames);

// are we at a point that could be considered a job location?
UUtBool AI2rPatrol_AtJobLocation(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState);

// finish a patrol
void AI2rPatrol_Finish(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState);

// ------------------------------------------------------------------------------------
// -- scripting interface functions

// find a patrol path by name
UUtError AI2rPatrol_FindByName(const char *inPatrolName, AI2tPatrolPath *outPath);

// ------------------------------------------------------------------------------------
// -- shooting interface functions

void AI2rPatrolAI_TargetingCallback_Fire(AI2tTargetingState *inTargetingState, UUtBool inFire);

// ------------------------------------------------------------------------------------
// -- debugging functions

// report in
void AI2rPatrol_Report(ONtCharacter *ioCharacter, AI2tPatrolState *ioPatrolState,
					   UUtBool inVerbose, AI2tReportFunction inFunction);

// print out a patrol path
void AI2rPatrol_DescribePath(AI2tPatrolPath *inPath, char *outString);
void AI2rPatrol_DescribeWaypoint(AI2tWaypoint *inWaypoint, char *outString);

// edit a patrol path
UUtError AI2rPatrol_AddWaypoint(struct OBJtObject *ioObject, UUtUns32 inPosition, AI2tWaypoint *inWaypointData);
UUtError AI2rPatrol_RemoveWaypoint(struct OBJtObject *ioObject, UUtUns32 inPosition);

// get the currently selected patrol path by the editor
struct OBJtObject *AI2rDebug_GetSelectedPath(void);

#endif // ONI_AI2_Patrol_H
