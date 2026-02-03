/*
	FILE:	Oni_AI2_Path.c

	AUTHOR:	Chris Butcher

	CREATED: July 29, 2000

	PURPOSE: High-level Pathfinding AI for Oni

	Copyright (c) Bungie Software, 2000

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Timer.h"

#include "Oni_AI2_Movement.h"
#include "Oni_AI2_Path.h"
#include "Oni_AI2.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"

#if defined(PROFILE) && PROFILE
#define PROFILE_PATHFINDING			0
#else
#define PROFILE_PATHFINDING			0
#endif

// ------------------------------------------------------------------------------------
// -- fudge factors

#define AI2cPath_BNVLookaheadCount				4

#define AI2cNewDestination_Distance				15.0f
#define AI2cFollowCharacter_DestinationDist		12.0f
#define AI2cDestination_VerticalHeightIgnore	(ONcCharacterHeight * 1.5f)

#define AI2cNewPath_MaxReuseTime				90
#define AI2cNewPath_MaxReuseDist				20.0f

#define AI2cRepath_DecayFrames					300
#define AI2cRepath_FailCount					5
#define AI2cRepath_MinFrames					30

#define AI2cFailedPath_MinFrames				90

// ------------------------------------------------------------------------------------
// -- global variables

// debugging
UUtBool AI2gDrawPaths = UUcFalse;
UUtBool AI2gIgnorePathError = UUcFalse;

// ------------------------------------------------------------------------------------
// -- internal function prototypes

// pathfinding
static void AI2iPath_NewDestination(ONtCharacter *ioCharacter);
static void AI2iPath_ClearPath(ONtCharacter *ioCharacter);
static UUtBool AI2iPath_FindGlobalPath(ONtCharacter *ioCharacter, PHtGraph *inGraph);
static UUtBool AI2iPath_MakePath(ONtCharacter *ioCharacter, UUtBool inForce);
static UUtBool AI2iPath_CheckFailure(ONtCharacter *ioCharacter);
static UUtBool AI2iPath_NextNode(ONtCharacter *ioCharacter);
static UUtBool AI2iPath_NewNode(ONtCharacter *ioCharacter, UUtBool inReusePath);
static void AI2iPath_NoPathAvailable(ONtCharacter *ioCharacter);

// movement
static void AI2iPath_CheckDestination(ONtCharacter *ioCharacter);

// ------------------------------------------------------------------------------------
// -- global routines

void AI2rPath_Initialize(ONtCharacter *ioCharacter)
{
	// we have no destination
	ioCharacter->pathState.at_destination = UUcTrue;
	ioCharacter->pathState.at_finalpoint = UUcTrue;
	ioCharacter->pathState.destinationType = AI2cDestinationType_None;

	AI2iPath_NewDestination(ioCharacter);
	AI2iPath_ClearPath(ioCharacter);
}

/*
 * destination setting interface
 */

static void AI2iPath_NoPathAvailable(ONtCharacter *ioCharacter)
{
	ioCharacter->pathState.failed_path_time = ONrGameState_GetGameTime();
}

UUtBool AI2rPath_Repath(ONtCharacter *ioCharacter, UUtBool inForce)
{
	UUtBool success;

#if PROFILE_PATHFINDING
	UUrProfile_State_Set(UUcProfile_State_On);
#endif

	AI2rMovement_NewPathReceived(ioCharacter);

	ioCharacter->pathState.at_finalpoint = UUcFalse;
	AI2iPath_CheckDestination(ioCharacter);
	ioCharacter->pathState.distance_from_start_to_dest_squared = ioCharacter->pathState.distance_to_destination_squared;

	if (ioCharacter->pathState.at_finalpoint) {
		AI2iPath_ClearPath(ioCharacter);
		return UUcTrue;
	} else {
		if (ioCharacter->pathState.failed_path_time != (UUtUns32) -1) {
			UUtUns32 current_time = ONrGameState_GetGameTime();

			// don't attempt to remake failed paths too often
			if (current_time < ioCharacter->pathState.failed_path_time + AI2cFailedPath_MinFrames) {
				return UUcFalse;
			}
		}

		success = AI2iPath_MakePath(ioCharacter, inForce);
		if (!success) {
			AI2iPath_NoPathAvailable(ioCharacter);
		}

		return success;
	}

#if PROFILE_PATHFINDING
	UUrProfile_State_Set(UUcProfile_State_Off);
#endif
}

void AI2rPath_GoToPoint(ONtCharacter *ioCharacter, const M3tPoint3D *inPoint, float inDistance,
						UUtBool inStopAtPoint, float inStopFacing, ONtCharacter *inIgnoreChar)
{
	// we can't stop exactly on a point - this is as close as we can get
	inDistance = UUmMax(inDistance, AI2cMoveToPoint_DefaultDistance);

	ioCharacter->pathState.destinationLocation = *inPoint;
	ioCharacter->pathState.destinationType = AI2cDestinationType_Point;
	ioCharacter->pathState.destinationData.point_data.required_distance = inDistance;
	ioCharacter->pathState.destinationData.point_data.stop_at_point = inStopAtPoint;
	ioCharacter->pathState.destinationData.point_data.final_facing = inStopFacing;
	AI2rMovement_IgnoreCharacter(ioCharacter, inIgnoreChar);

	AI2rPath_Repath(ioCharacter, UUcFalse);
}

void AI2rPath_FollowCharacter(ONtCharacter *ioCharacter, ONtCharacter *inFollow)
{
	if (inFollow == NULL) {
		AI2rPath_Halt(ioCharacter);
		return;
	}

	UUmAssertReadPtr(inFollow, sizeof(ONtCharacter));
	ioCharacter->pathState.destinationLocation = inFollow->location;
	ioCharacter->pathState.destinationType = AI2cDestinationType_FollowCharacter;
	ioCharacter->pathState.destinationData.followcharacter_data.character = inFollow;
	AI2rMovement_IgnoreCharacter(ioCharacter, inFollow);

	AI2rPath_Repath(ioCharacter, UUcFalse);
}

void AI2rPath_Halt(ONtCharacter *ioCharacter)
{
	// CB: we do not actually halt pathfinding here... this function clears our pathfinding
	// ORDERS which is a very different thing. pathfinding will be halted in AI2rPath_Update
	// if there are no alternative orders given this frame.
	ioCharacter->pathState.destinationType = AI2cDestinationType_None;
	AI2rMovement_Halt(ioCharacter);
}

// ------------------------------------------------------------------------------------
// -- update routines

// update for one tick
void AI2rPath_Update(ONtCharacter *ioCharacter)
{
#if PROFILE_PATHFINDING
	UUrProfile_State_Set(UUcProfile_State_On);
#endif

	// handle debugging chump
	if (UUmString_IsEqual(ioCharacter->player_name, "chump")) {
		if (AI2gChumpStop) {
			AI2rPath_Halt(ioCharacter);
		} else if (ioCharacter->pathState.at_destination) {
			// we're meant to be following someone!
			AI2rPath_FollowCharacter(ioCharacter, ONrGameState_GetPlayerCharacter());
		}
	}

	if (ioCharacter->pathState.destinationType == AI2cDestinationType_None) {
		// we are halted, stop our pathfinding
		AI2rPath_Repath(ioCharacter, UUcTrue);
	} else {
		// check to see whether we've received a new destination this frame
		if ((ioCharacter->pathState.at_destination != ioCharacter->pathState.prev_at_destination) ||
			(MUmVector_GetDistanceSquared(ioCharacter->pathState.destinationLocation, ioCharacter->pathState.prev_destination) >
					UUmSQR(AI2cNewDestination_Distance))) {
			AI2iPath_NewDestination(ioCharacter);
		}
	}

	ioCharacter->pathState.prev_at_destination	= ioCharacter->pathState.at_destination;
	ioCharacter->pathState.prev_destination		= ioCharacter->pathState.destinationLocation;

	// check to see if we want to open a door
	if (ioCharacter->pathState.door_to_open != NULL) {
		if (OBJrDoor_CharacterProximity(ioCharacter->pathState.door_to_open, ioCharacter)) {
			OBJrDoor_CharacterOpen(ioCharacter, ioCharacter->pathState.door_to_open);
		}
	}

	AI2iPath_CheckDestination(ioCharacter);
	AI2iPath_CheckFailure(ioCharacter);

	if (!ioCharacter->pathState.at_finalpoint) {
		while (AI2rMovement_AdvanceThroughGrid(ioCharacter)) {
			if (!AI2iPath_NextNode(ioCharacter)) {
				// we have hit the end of our path
				break;
			}
		}
	}

#if PROFILE_PATHFINDING
	UUrProfile_State_Set(UUcProfile_State_Off);
#endif
}

// ------------------------------------------------------------------------------------
// -- pathfinding routines

static void AI2iPath_NewDestination(ONtCharacter *ioCharacter)
{
	// clear our pathfinding error information
	ioCharacter->pathState.forced_repath_count = 0;
	ioCharacter->pathState.forced_repath_timer = 0;
	ioCharacter->pathState.forced_repath_decaytimer = 0;
	ioCharacter->pathState.failed_path_time = (UUtUns32) -1;

	AI2rMovement_NewDestination(ioCharacter);
}

static void AI2iPath_ClearPath(ONtCharacter *ioCharacter)
{
	ioCharacter->pathState.pathDestinationType = AI2cDestinationType_None;
	ioCharacter->pathState.at_finalpoint = UUcTrue;
	ioCharacter->pathState.path_num_nodes = 0;
	ioCharacter->pathState.door_to_open = NULL;
	ioCharacter->pathState.moving_onto_stairs = UUcFalse;
	ioCharacter->pathState.last_pathfinding_error = AI2cError_Pathfinding_NoError;

	AI2rMovement_ClearPath(ioCharacter);
}

void AI2rPath_FindGhostWaypoint(ONtCharacter *ioCharacter, M3tPoint3D *inStartPoint, UUtUns32 inCurrentNode, M3tPoint3D *outPoint)
{
	UUtUns32 current_node_index;
	M3tPoint3D target_pt;
	AKtEnvironment *environment = ONrGameState_GetEnvironment();

	// work out the first approximation to our target point
	UUmAssert(ioCharacter->pathState.path_num_nodes >= 2);
	current_node_index = inCurrentNode + AI2cPath_BNVLookaheadCount;

	if (current_node_index > (UUtUns32) (ioCharacter->pathState.path_num_nodes - 2)) {
		// we start our lookahead at the actual destination point
		current_node_index = ioCharacter->pathState.path_num_nodes - 1;
		target_pt = ioCharacter->pathState.path_end_location;
	} else {
		// we start our lookahead a certain number of BNVs ahead in our path... get the closest point on the
		// connection's ghost quad.
		PHrWaypointFromGunk(environment, ioCharacter->pathState.path_connections[current_node_index]->gunk,
							&target_pt, inStartPoint);
	}

	while (current_node_index > inCurrentNode) {
		current_node_index--;

		// project the target point onto the current node's connection's ghost quad
		PHrProjectOntoConnection(environment, ioCharacter->pathState.path_connections[current_node_index],
								inStartPoint, &target_pt);
	}

	// use the target point that was computed
	*outPoint = target_pt;
}

// generate a path through the pathfinding graph
static UUtBool AI2iPath_FindGlobalPath(ONtCharacter *ioCharacter, PHtGraph *inGraph)
{
	UUtBool					gotpath;

	if (ioCharacter->pathState.path_start_node == ioCharacter->pathState.path_end_node) {
		// the path is entirely within one node
		ioCharacter->pathState.path_num_nodes = 1;

	} else {
		// we must find a path from start node to end node through the pathfinding graph.
		if (ioCharacter->pathState.path_start_node->num_connections_out == 0) {
			ioCharacter->pathState.last_pathfinding_error = AI2cError_Pathfinding_NoConnectionsFromStart;
			goto large_grid_path_failed;
		}
		if (ioCharacter->pathState.path_end_node->num_connections_in == 0) {
			ioCharacter->pathState.last_pathfinding_error = AI2cError_Pathfinding_NoConnectionsToDest;
			goto large_grid_path_failed;
		}

		// generate the path
		gotpath = PHrGeneratePath(inGraph, ioCharacter, (UUtUns32) -1,
						ioCharacter->pathState.path_start_node, ioCharacter->pathState.path_end_node,
						&ioCharacter->pathState.path_start_location, &ioCharacter->pathState.path_end_location, NULL,
						ioCharacter->pathState.path_connections, &ioCharacter->pathState.path_num_nodes);

		if (!gotpath) {
			ioCharacter->pathState.last_pathfinding_error = AI2cError_Pathfinding_NoPath;
			goto large_grid_path_failed;
		}
	}

	// set up to follow this path
	ioCharacter->pathState.path_current_node = 0;
	ioCharacter->pathState.last_pathfinding_error = AI2cError_Pathfinding_NoError;

	return UUcTrue;

large_grid_path_failed:
	AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, ioCharacter->pathState.last_pathfinding_error, ioCharacter,
				&ioCharacter->pathState.path_start_location, &ioCharacter->pathState.path_end_location,
				ioCharacter->pathState.path_start_node, ioCharacter->pathState.path_end_node);

	return UUcFalse;
}

// pathfind in a new node
static UUtBool AI2iPath_NewNode(ONtCharacter *ioCharacter, UUtBool inReusePath)
{
	if (ioCharacter->pathState.path_current_node + 1 < ioCharacter->pathState.path_num_nodes) {
		PHtNode *to_node;

		// check to see if we have to open a manual door to get to the next grid
		ioCharacter->pathState.door_to_open = ioCharacter->pathState.path_connections[ioCharacter->pathState.path_current_node]->door_link;

		// check to see if we're trying to move onto a stair node
		to_node = ioCharacter->pathState.path_connections[ioCharacter->pathState.path_current_node]->to;
		ioCharacter->pathState.moving_onto_stairs = ((to_node->location->flags & AKcBNV_Flag_Stairs_Standard) > 0);
	} else {
		ioCharacter->pathState.door_to_open = NULL;
		ioCharacter->pathState.moving_onto_stairs = UUcFalse;
	}

	if (AI2rMovement_MakePath(ioCharacter, inReusePath)) {
		// success!
		ioCharacter->pathState.at_finalpoint = UUcFalse;
		return UUcTrue;
	} else {
		// error making grid path
		AI2iPath_NoPathAvailable(ioCharacter);
		ioCharacter->pathState.at_finalpoint = UUcTrue;
		return UUcFalse;
	}
}

// we hit the end of our grid, advance to the next node in our path.
static UUtBool AI2iPath_NextNode(ONtCharacter *ioCharacter)
{
	UUtBool success;

	ioCharacter->pathState.path_current_node += 1;

	if (ioCharacter->pathState.path_current_node >= ioCharacter->pathState.path_num_nodes) {
		// we hit the end of our path
		ioCharacter->pathState.path_current_node = 0;
		ioCharacter->pathState.path_num_nodes = 0;
		ioCharacter->pathState.door_to_open = NULL;
		ioCharacter->pathState.at_finalpoint = UUcTrue;
		AI2rMovement_ClearPath(ioCharacter);
		return UUcFalse;
	}

	success = AI2iPath_NewNode(ioCharacter, UUcFalse);

	// FIXME: handle failure cases

	return UUcTrue;
}

// construct a path
static UUtBool AI2iPath_MakePath(ONtCharacter *ioCharacter, UUtBool inForce)
{
	PHtGraph				*graph;
	AKtBNVNode				*akNodeEnd;
	M3tPoint3D				locationStart, locationEnd;
	PHtNode					*nodeStart, *nodeEnd;
	UUtUns32				patherror = AI2cError_Pathfinding_NoError;
	UUtBool					reuse_path;
	UUtUns32				current_time;

	graph = ONrGameState_GetGraph();
	current_time = ONrGameState_GetGameTime();

	if (ioCharacter->pathState.destinationType == AI2cDestinationType_FollowCharacter) {
		// our destination continually changes, we are following someone!
		ONrCharacter_GetPathfindingLocation(ioCharacter->pathState.destinationData.followcharacter_data.character,
											&ioCharacter->pathState.destinationLocation);
	}

	// determine whether we can reuse an existing path
	reuse_path = !inForce;
	reuse_path = reuse_path && (ioCharacter->pathState.pathDestinationType != AI2cDestinationType_None);
	reuse_path = reuse_path && (!ioCharacter->pathState.at_destination);
	reuse_path = reuse_path && (ioCharacter->pathState.path_num_nodes > 0);
	reuse_path = reuse_path && (ioCharacter->pathState.pathComputeTime + AI2cNewPath_MaxReuseTime > current_time);
	reuse_path = reuse_path && (MUmVector_GetDistanceSquared(ioCharacter->pathState.pathDestinationLocation,
															ioCharacter->pathState.destinationLocation) < UUmSQR(AI2cNewPath_MaxReuseDist));

#if DEBUG_PATH_REUSE
	if ((ioCharacter->pathState.pathDestinationType == AI2cDestinationType_None) || (ioCharacter->pathState.at_destination) ||
		(ioCharacter->pathState.path_num_nodes == 0)) {
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: no reuse; no path", current_time, ioCharacter->player_name);

	} else if (!reuse_path) {
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: no reuse; path was at %d dist %f (tol %f)",
								current_time, ioCharacter->player_name, ioCharacter->pathState.pathComputeTime,
								MUmVector_GetDistance(ioCharacter->pathState.pathDestinationLocation, ioCharacter->pathState.destinationLocation),
								AI2cNewPath_MaxReuseDist);

	} else {
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: initial path reuse success", current_time, ioCharacter->player_name);
	}
#endif

	// calculate the path nodes at the start and end of this path
	locationStart	= ioCharacter->actual_position;
	locationEnd		= ioCharacter->pathState.destinationLocation;
	locationEnd.y	+= PHcWorldCoord_YOffset;

	// get the pathfinding nodes for these points
	nodeStart		= ioCharacter->currentPathnode;
	akNodeEnd		= AKrNodeFromPoint(&locationEnd);
	if (akNodeEnd == NULL) {
		nodeEnd		= NULL;
	} else {
		nodeEnd		= PHrAkiraNodeToGraphNode(akNodeEnd, graph);
	}

	if (nodeStart == NULL) {
		patherror = AI2cError_Pathfinding_NoBNVAtStart;

	} else if (nodeEnd == NULL) {
		patherror = AI2cError_Pathfinding_NoBNVAtDest;
	}

	if (patherror != AI2cError_Pathfinding_NoError) {
		// failure - we cannot find one of more of the pathfinding nodes and cannot make a path
		ioCharacter->pathState.last_pathfinding_error	= patherror;
		ioCharacter->pathState.path_start_location		= locationStart;
		ioCharacter->pathState.path_end_location		= locationEnd;
		ioCharacter->pathState.path_start_node			= nodeStart;
		ioCharacter->pathState.path_end_node			= nodeEnd;

		AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, ioCharacter->pathState.last_pathfinding_error, ioCharacter,
					&ioCharacter->pathState.path_start_location, &ioCharacter->pathState.path_end_location,
					ioCharacter->pathState.path_start_node, ioCharacter->pathState.path_end_node);
		return UUcFalse;
	}

#if DEBUG_PATH_REUSE
	if (reuse_path) {
		if ((nodeStart != ioCharacter->pathState.path_start_node) || (nodeEnd != ioCharacter->pathState.path_end_node)) {
			COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: no reuse; pathnode mismatch cur %d/%d != new %d/%d",
									current_time, ioCharacter->player_name,
									(ioCharacter->pathState.path_start_node == NULL) ? -1 : (ioCharacter->pathState.path_start_node->location->index),
									(ioCharacter->pathState.path_end_node   == NULL) ? -1 : (ioCharacter->pathState.path_end_node  ->location->index),
									(nodeStart                              == NULL) ? -1 : (nodeStart                             ->location->index),
									(nodeEnd                                == NULL) ? -1 : (nodeEnd                               ->location->index));
		}
	}
#endif

	// check to see whether these nodes match our existing path's nodes
	reuse_path = reuse_path && ((nodeStart == ioCharacter->pathState.path_start_node) && (nodeEnd == ioCharacter->pathState.path_end_node));

	// we use the new path start and end locations regardless of whether we end up reusing the
	// path or not.
	ioCharacter->pathState.path_start_location			= locationStart;
	ioCharacter->pathState.path_end_location			= locationEnd;

	if (!reuse_path) {
		UUtBool success;

		// we must construct a new global path through the pathfinding graph
		AI2iPath_ClearPath(ioCharacter);

		ioCharacter->pathState.pathComputeTime = current_time;
		ioCharacter->pathState.pathDestinationType		= ioCharacter->pathState.destinationType;
		ioCharacter->pathState.pathDestinationData		= ioCharacter->pathState.destinationData;
		ioCharacter->pathState.pathDestinationLocation	= ioCharacter->pathState.destinationLocation;
		ioCharacter->pathState.path_start_node			= nodeStart;
		ioCharacter->pathState.path_end_node			= nodeEnd;

		success = AI2iPath_FindGlobalPath(ioCharacter, graph);
		if (!success) {
			return UUcFalse;
		}
	} else {
#if DEBUG_PATH_REUSE
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%d %s: global path reuse success", current_time, ioCharacter->player_name);
#endif
	}

	return AI2iPath_NewNode(ioCharacter, reuse_path);
}

// there is a problem with following the path that we constructed
void AI2rPath_PathError(ONtCharacter *ioCharacter)
{
	UUtBool success;

#if PROFILE_PATHFINDING
	UUrProfile_State_Set(UUcProfile_State_On);
#endif

	ioCharacter->pathState.forced_repath_count++;

#if AI_VERBOSE_PATHFAILURE
	COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%s: path error (count %d)", ioCharacter->player_name, ioCharacter->pathState.forced_repath_count);
#endif

	if (ioCharacter->pathState.forced_repath_count >= AI2cRepath_FailCount) {
		// we cannot successfully make this path. throw an error and hope that our high-level logic catches it.
#if AI_VERBOSE_PATHFAILURE
		COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%s: FAILING PATH", ioCharacter->player_name);
#endif
		UUmAssert((ioCharacter->pathState.last_pathfinding_error > AI2cError_Pathfinding_NoError) &&
				  (ioCharacter->pathState.last_pathfinding_error < AI2cError_Pathfinding_Max));
		AI2_ERROR(AI2cError, AI2cSubsystem_Pathfinding, ioCharacter->pathState.last_pathfinding_error, ioCharacter,
					&ioCharacter->pathState.path_start_location, &ioCharacter->pathState.path_end_location,
					ioCharacter->pathState.path_start_node, ioCharacter->pathState.path_end_node);
		AI2rPath_Halt(ioCharacter);
	} else {
		// remake our path
		success = AI2iPath_MakePath(ioCharacter, UUcTrue);
		if (!success) {
			AI2iPath_NoPathAvailable(ioCharacter);
		}
	}

	// make sure that we don't repath too often
	ioCharacter->pathState.forced_repath_timer = AI2cRepath_MinFrames;
	ioCharacter->pathState.forced_repath_decaytimer = AI2cRepath_DecayFrames;

#if PROFILE_PATHFINDING
	UUrProfile_State_Set(UUcProfile_State_Off);
#endif
}

static UUtBool AI2iPath_CheckFailure(ONtCharacter *ioCharacter)
{
	if (ioCharacter->pathState.at_finalpoint)
		return UUcFalse;

	if (ioCharacter->pathState.path_num_nodes == 0) {
		// we don't have a path to check!
		return UUcFalse;
	}

	if (ioCharacter->pathState.forced_repath_timer > 0) {
		ioCharacter->pathState.forced_repath_timer--;

		if (ioCharacter->pathState.forced_repath_timer > 0) {
			// cannot repath yet, don't bother checking
			return UUcFalse;
		}
	}

	if (AI2rMovement_CheckFailure(ioCharacter)) {
		// there is a problem
		return UUcTrue;
	}

	// our path is okay, decay our error count
	if (ioCharacter->pathState.forced_repath_decaytimer > 0) {
		ioCharacter->pathState.forced_repath_decaytimer--;

		if (ioCharacter->pathState.forced_repath_decaytimer == 0) {
			// decay one of the repath counts
#if AI_VERBOSE_PATHFAILURE
			COrConsole_Printf_Color(UUcTrue, 0xFF700D6F, 0xFF300030, "%s: decaying error count to %d",
									ioCharacter->player_name, ioCharacter->pathState.forced_repath_count - 1);
#endif
			UUmAssert(ioCharacter->pathState.forced_repath_count > 0);
			ioCharacter->pathState.forced_repath_count--;

			if (ioCharacter->pathState.forced_repath_count > 0) {
				// reset the decay timer so that the next one will decay
				ioCharacter->pathState.forced_repath_decaytimer = AI2cRepath_DecayFrames;
			}
		}
	}

	return UUcFalse;
}

UUtBool AI2rPath_SimplePath(ONtCharacter *ioCharacter)
{
	if (ioCharacter->pathState.at_finalpoint) {
		return UUcTrue;
	}

	if (ioCharacter->pathState.path_num_nodes > 1) {
		return UUcFalse;
	}

	return AI2rMovement_SimplePath(ioCharacter);
}

UUtBool AI2rPath_StopAtDest(ONtCharacter *ioCharacter, float *outDestRadius)
{
	if (ioCharacter->pathState.destinationType == AI2cDestinationType_None) {
		if (AI2rMovement_HasMovementModifiers(ioCharacter)) {
			// we are being completely controlled by our movement modifiers -
			// never stop moving
			if (outDestRadius != NULL) {
				*outDestRadius = 0;
			}
			return UUcFalse;
		}
	}

	if (ioCharacter->pathState.at_finalpoint) {
		if (AI2rMovement_IsKeepingMoving(ioCharacter)) {
			// keep moving!
			if (outDestRadius != NULL) {
				*outDestRadius = 0;
			}
			return UUcFalse;
		}
	}

	switch (ioCharacter->pathState.destinationType) {
		case AI2cDestinationType_Point:
			if (outDestRadius != NULL) {
				*outDestRadius = ioCharacter->pathState.destinationData.point_data.required_distance;
			}
			return ioCharacter->pathState.destinationData.point_data.stop_at_point;

		case AI2cDestinationType_FollowCharacter:
			if (outDestRadius != NULL) {
				*outDestRadius = AI2cFollowCharacter_DestinationDist;
			}
			return UUcTrue;

		case AI2cDestinationType_None:
		default:
			if (outDestRadius != NULL) {
				*outDestRadius = 0;
			}
			return UUcTrue;
	}
}

static void AI2iPath_CheckDestination(ONtCharacter *ioCharacter)
{
	M3tVector3D distance;
	float radius;

	if (ioCharacter->pathState.destinationType == AI2cDestinationType_None) {
		// we have no path and are therefore at our destination
		ioCharacter->pathState.at_finalpoint = UUcTrue;
		ioCharacter->pathState.at_destination = UUcTrue;
		return;
	}

	MUmVector_Subtract(distance, ioCharacter->pathState.destinationLocation, ioCharacter->actual_position);

	// don't count vertical distance - but also remember that there may be multiple floors above each other
	if ((float)fabs(distance.y) < AI2cDestination_VerticalHeightIgnore)
		distance.y = 0;

	ioCharacter->pathState.distance_to_destination_squared = MUmVector_GetLengthSquared(distance);

	// how close do we have to get?
	radius = AI2cMoveToPoint_DefaultDistance;
	if (ioCharacter->pathState.destinationType == AI2cDestinationType_Point) {
		radius = UUmMax(radius, ioCharacter->pathState.destinationData.point_data.required_distance);
	}

	if (ioCharacter->pathState.distance_to_destination_squared < UUmSQR(radius)) {
		ioCharacter->pathState.at_finalpoint = UUcTrue;
	}

	if (ioCharacter->pathState.at_finalpoint) {
		// check to see whether we're facing correctly at our destination
		ioCharacter->pathState.at_destination = AI2rMovement_DestinationFacing(ioCharacter);
	} else {
		// keep moving, not at destination yet
		ioCharacter->pathState.at_destination = UUcFalse;
	}
}

// ------------------------------------------------------------------------------------
// -- debugging routines

// report in
void AI2rPath_Report(ONtCharacter *ioCharacter, UUtBool inVerbose, AI2tReportFunction inFunction)
{
	UUtUns32 itr;
	char reportbuf[256], tempbuf[128];
	PHtNode *node;

	switch(ioCharacter->pathState.destinationType) {
		case AI2cDestinationType_None:
			strcpy(reportbuf, "  no destination");
			break;

		case AI2cDestinationType_Point:
			sprintf(reportbuf, "  destination: point %f %f %f, tolerance %f",
					ioCharacter->pathState.destinationLocation.x,
					ioCharacter->pathState.destinationLocation.y,
					ioCharacter->pathState.destinationLocation.z,
					ioCharacter->pathState.destinationData.point_data.required_distance);
			strcat(reportbuf, (ioCharacter->pathState.destinationData.point_data.stop_at_point) ? " (stop)" : " (keep moving)");
			if (ioCharacter->pathState.destinationData.point_data.final_facing != AI2cAngle_None) {
				sprintf(tempbuf, " (orient to angle %f deg)", ioCharacter->pathState.destinationData.point_data.final_facing * M3cRadToDeg);
				strcat(reportbuf, tempbuf);
			}
			break;

		case AI2cDestinationType_FollowCharacter:
			sprintf(reportbuf, "  following %s (currently going to %f %f %f)", ioCharacter->pathState.destinationData.followcharacter_data.character->player_name,
					ioCharacter->pathState.destinationLocation.x,
					ioCharacter->pathState.destinationLocation.y,
					ioCharacter->pathState.destinationLocation.z);
			break;

		default:
			sprintf(reportbuf, "  unknown destination type %d", ioCharacter->pathState.destinationType);
			break;
	}
	inFunction(reportbuf);

	if (ioCharacter->pathState.at_destination) {
		sprintf(reportbuf, "  at destination");
		inFunction(reportbuf);

	} else if (ioCharacter->pathState.at_finalpoint) {
		sprintf(reportbuf, "  at final point, orienting");
		inFunction(reportbuf);

	} else {
		sprintf(reportbuf, "  path from %f %f %f (BNV %d) to %f %f %f (BNV %d), currently in %d of %d nodes",
				ioCharacter->pathState.path_start_location.x, ioCharacter->pathState.path_start_location.y, ioCharacter->pathState.path_start_location.z,
				(ioCharacter->pathState.path_start_node == NULL) ? -1 : ioCharacter->pathState.path_start_node->location->index,
				ioCharacter->pathState.path_end_location.x, ioCharacter->pathState.path_end_location.y, ioCharacter->pathState.path_end_location.z,
				(ioCharacter->pathState.path_end_node == NULL) ? -1 : ioCharacter->pathState.path_end_node->location->index,
				ioCharacter->pathState.path_current_node, ioCharacter->pathState.path_num_nodes);
		inFunction(reportbuf);

		if (inVerbose && (ioCharacter->pathState.path_num_nodes > 0)) {
			strcpy(reportbuf, "  path is: ");
			for (itr = 0; itr < ioCharacter->pathState.path_num_nodes; itr++) {
				if (itr < ioCharacter->pathState.path_num_nodes - 1) {
					node = ioCharacter->pathState.path_connections[itr]->from;
				} else {
					node = ioCharacter->pathState.path_connections[itr - 1]->to;
				}
				sprintf(tempbuf, "%d%s%s", (node == NULL) ? -1 : node->location->index,
					(itr == ioCharacter->pathState.path_current_node) ? " (current)" : "",
					(itr < ioCharacter->pathState.path_num_nodes - 1) ? " -> " : "");
				strcat(reportbuf, tempbuf);
			}
			inFunction(reportbuf);
		}
	}

	if (inVerbose) {
		sprintf(reportbuf, "  repaths: %d", ioCharacter->pathState.forced_repath_count);
		if (ioCharacter->pathState.forced_repath_count > 0) {
			sprintf(tempbuf, " (decay-timer %d)", ioCharacter->pathState.forced_repath_decaytimer);
			strcat(reportbuf, tempbuf);
		}
		if (ioCharacter->pathState.forced_repath_timer > 0) {
			sprintf(tempbuf, " (can't repath again for %d frames)", ioCharacter->pathState.forced_repath_timer);
		}
		inFunction(reportbuf);

		if (ioCharacter->pathState.last_pathfinding_error != AI2cError_Pathfinding_NoError) {
			// throw the error again, and make sure that we don't think it's a real error
			inFunction("  last pathfinding error was:");
			AI2gIgnorePathError = UUcTrue;
			AI2_ERROR(AI2cError, AI2cSubsystem_Pathfinding, ioCharacter->pathState.last_pathfinding_error, ioCharacter,
						&ioCharacter->pathState.path_start_location, &ioCharacter->pathState.path_end_location,
						ioCharacter->pathState.path_start_node, ioCharacter->pathState.path_end_node);
			AI2gIgnorePathError = UUcFalse;
		}
	}
}

// draw the character's path
void AI2rPath_RenderPath(ONtCharacter *ioCharacter)
{
	M3tPoint3D line_start, line_end, top_point;
	IMtShade shade;
	float angle, radius, costheta, sintheta;
	UUtUns32 itr;

	if (ioCharacter->pathState.at_destination)
		return;

	if (ioCharacter->pathState.destinationType == AI2cDestinationType_Point) {
		// draw our target point
		MUmVector_Copy(top_point, ioCharacter->pathState.destinationLocation);
		top_point.y += 10.0f;
		M3rGeom_Line_Light(&ioCharacter->pathState.destinationLocation, &top_point, IMcShade_Yellow);

		// yellow if we have to stop there, orange if we just pass through
		shade = (ioCharacter->pathState.destinationData.point_data.stop_at_point) ? IMcShade_Yellow : IMcShade_Orange;

		// draw a little arrow if we have to face when we get there
		angle = ioCharacter->pathState.destinationData.point_data.final_facing;
		if (angle != AI2cAngle_None) {
			costheta = MUrCos(angle);
			sintheta = MUrSin(angle);

			MUmVector_Copy(line_start, top_point);
			MUmVector_Copy(line_end, top_point);

			line_start.x += sintheta * 6.0f;
			line_start.z += costheta * 6.0f;
			M3rGeom_Line_Light(&line_start, &line_end, shade);

			line_start.x += -costheta * 1.5f - sintheta * 2.0f;
			line_start.z +=  sintheta * 1.5f - costheta * 2.0f;
			M3rGeom_Line_Light(&line_start, &line_end, shade);

			line_start.x += costheta * 3.0f;
			line_start.z += -sintheta * 3.0f;
			M3rGeom_Line_Light(&line_start, &line_end, shade);
		}

		radius = ioCharacter->pathState.destinationData.point_data.required_distance;

		// draw a diamond around the point indicating radius
		MUmVector_Copy(line_start, top_point);
		MUmVector_Copy(line_end, top_point);
		line_start.x += radius;
		line_end.z += radius;
		M3rGeom_Line_Light(&line_start, &line_end, shade);

		line_start.x -= 2 * radius;
		M3rGeom_Line_Light(&line_start, &line_end, shade);

		line_end.z -= 2 * radius;
		M3rGeom_Line_Light(&line_start, &line_end, shade);

		line_start.x += 2 * radius;
		M3rGeom_Line_Light(&line_start, &line_end, shade);
	}

	// draw a line through each gunk connection from large nodes to the next
	line_start = ioCharacter->actual_position;
	for (itr = ioCharacter->pathState.path_current_node; itr + 1 < ioCharacter->pathState.path_num_nodes; itr++) {
		AI2rPath_FindGhostWaypoint(ioCharacter, &line_start, itr, &line_end);

		M3rGeom_Line_Light(&line_start, &line_end, IMcShade_Orange);
		MUmVector_Copy(line_start, line_end);
	}
	M3rGeom_Line_Light(&line_start, &ioCharacter->pathState.destinationLocation, IMcShade_Orange);
}
