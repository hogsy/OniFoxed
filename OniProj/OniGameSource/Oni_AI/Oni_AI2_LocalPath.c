/*
	FILE:	Oni_AI2_LocalPath.c

	AUTHOR:	Chris Butcher

	CREATED: May 08, 1999

	PURPOSE: Local-grid pathfinding for Oni AI

	Copyright (c) 2000

*/

#include "BFW_Path.h"

#include "Oni_AI2.h"
#include "Oni_Character.h"
#include "Oni_Path.h"

// ------------------------------------------------------------------------------------
// -- structure definitions

// a structure that stores data which is shared between successive calls to AI2rDoLocalPath.
// used to reduce stack overhead and provide spatial coherence information
typedef struct AI2tLocalPathData {
	AKtEnvironment	*environment;
	PHtGraph		*graph;
	ONtCharacter	*owner;
	PHtPathMode		path_mode;

	M3tPoint3D		start_point, end_point;
	UUtInt16		start_x, start_y;

	UUtUns8			max_weight;
	UUtUns8			escape_distance;

	AKtBNVNode		*start_aknode;
	PHtNode			*start_node;

	UUtUns32		*startroom_obstruction_bitvector;
	UUtUns32		*other_obstruction_bitvector;

	UUtUns32		nodes_crossed;
	AKtBNVNode		*bnv_hint;			// hint used between successive calls
} AI2tLocalPathData;

enum {
	AI2cLocalPath_Segment_None = 0,
	AI2cLocalPath_Segment_NegX = 1,
	AI2cLocalPath_Segment_PosX = 2,
	AI2cLocalPath_Segment_NegY = 3,
	AI2cLocalPath_Segment_PosY = 4
};

// ------------------------------------------------------------------------------------
// -- external globals

#if TOOL_VERSION
UUtBool		AI2gDebugLocalPath_Stored = UUcFalse;
UUtUns32	AI2gDebugLocalPath_LineCount = 20;
M3tPoint3D	AI2gDebugLocalPath_Point;
UUtBool		AI2gDebugLocalPath_Success[AI2_LOCALPATH_LINEMAX];
M3tPoint3D	AI2gDebugLocalPath_EndPoint[AI2_LOCALPATH_LINEMAX];
UUtUns8		AI2gDebugLocalPath_Weight[AI2_LOCALPATH_LINEMAX];
#endif

// ------------------------------------------------------------------------------------
// -- internal globals

static AI2tLocalPathData AI2gLocalPath;

// ------------------------------------------------------------------------------------
// -- internal function prototypes

static UUtBool AI2rDoLocalMovement(UUtUns8 *outWeight, UUtBool *outEscape, M3tPoint3D *outPoint);

// ------------------------------------------------------------------------------------
// -- routines

// set up for local path finding
UUtError AI2rLocalPath_Initialize(void)
{
	AI2gLocalPath.startroom_obstruction_bitvector	= UUrBitVector_New(2 * PHcMaxObstructions);
	AI2gLocalPath.other_obstruction_bitvector		= UUrBitVector_New(2 * PHcMaxObstructions);

	if ((AI2gLocalPath.startroom_obstruction_bitvector	== NULL) ||
		(AI2gLocalPath.other_obstruction_bitvector		== NULL))
		return UUcError_OutOfMemory;
	else
		return UUcError_None;
}

// dispose local path finding data structures
void AI2rLocalPath_Terminate(void)
{
	if (AI2gLocalPath.startroom_obstruction_bitvector)
		UUrBitVector_Dispose(AI2gLocalPath.startroom_obstruction_bitvector);

	if (AI2gLocalPath.other_obstruction_bitvector)
		UUrBitVector_Dispose(AI2gLocalPath.other_obstruction_bitvector);

	AI2gLocalPath.startroom_obstruction_bitvector = NULL;
	AI2gLocalPath.other_obstruction_bitvector = NULL;
}

// debugging - cast a halo of lines into the local environment
void AI2rLocalPath_DebugLines(ONtCharacter *inCharacter)
{
#if TOOL_VERSION
	UUtError error;
	UUtUns32 itr;
	float direction;
	UUtBool escape_path;
	M3tVector3D dir_vector, endpoint;

	AI2gDebugLocalPath_LineCount = UUmMin(AI2gDebugLocalPath_LineCount, AI2_LOCALPATH_LINEMAX);
	AI2gDebugLocalPath_Stored = UUcTrue;

	if (AI2gDebug_LocalPathfinding) {
		// try pathfinding in this direction
		MUmVector_Copy(AI2gDebugLocalPath_Point, inCharacter->location);
		direction = inCharacter->facing + inCharacter->facingModifier;

		error = AI2rFindLocalPath(inCharacter, PHcPathMode_CasualMovement, NULL, NULL, NULL,
								20.0f, direction, M3cPi / 16.0f, 4, PHcDanger, 3,
								&AI2gDebugLocalPath_Success[0],
								&AI2gDebugLocalPath_EndPoint[0],
								&AI2gDebugLocalPath_Weight[0], &escape_path);

		if (error != UUcError_None) {
			// BNV error - we cannot pathfind from this point
			AI2gDebugLocalPath_Stored = UUcFalse;
			return;
		}

	} else if (AI2gDebug_LocalMovement) {

		MUmVector_Copy(AI2gDebugLocalPath_Point, inCharacter->location);
		for (itr = 0; itr < AI2gDebugLocalPath_LineCount; itr++) {
			direction = itr * M3c2Pi / AI2gDebugLocalPath_LineCount;

			MUmVector_Set(dir_vector, MUrSin(direction), 0, MUrCos(direction));
			MUmVector_Copy(endpoint, AI2gDebugLocalPath_Point);
			MUmVector_ScaleIncrement(endpoint, 40.0f, dir_vector);

			error = AI2rTryLocalMovement(inCharacter, PHcPathMode_CasualMovement, &AI2gDebugLocalPath_Point, &endpoint, 3,
										&AI2gDebugLocalPath_Success[itr],
										&AI2gDebugLocalPath_EndPoint[itr],
										&AI2gDebugLocalPath_Weight[itr], &escape_path);

			if (error != UUcError_None) {
				// BNV error - we cannot pathfind from this point
				AI2gDebugLocalPath_Stored = UUcFalse;
				return;
			}
		}
	}
#endif
}

// find a point that is close to another point and can reach it through the local environment, in a specified direction
UUtError AI2rFindNearbyPoint(ONtCharacter *ioCharacter, PHtPathMode inPathMode, M3tPoint3D *inStartPoint,
							float inDistance, float inMinDistance, float inDirection, float inSideVary,
							UUtUns32 inSideAttempts, UUtUns8 inMaxWeight, UUtUns8 inEscapeDistance, UUtBool *outSuccess,
							M3tPoint3D *outPoint, UUtUns8 *outWeight, UUtBool *outEscape)
{
	M3tPoint3D				cur_stop_point, found_stop_point;
	float					found_distsq, cur_distsq, current_direction;
	UUtUns32				attempt, side;
	UUtUns8					cur_weight, found_weight;
	UUtBool					found, cur_path_ok, cur_escape_path, found_escape_path;

	AI2gLocalPath.owner = ioCharacter;
	AI2gLocalPath.path_mode = inPathMode;
	AI2gLocalPath.max_weight = inMaxWeight;

	AI2gLocalPath.start_point = *inStartPoint;
	AI2gLocalPath.graph = ONrGameState_GetGraph();
	AI2gLocalPath.environment = ONrGameState_GetEnvironment();

	// find the starting BNV, and its pathfinding data
	AI2gLocalPath.start_aknode = AKrNodeFromPoint(&AI2gLocalPath.start_point);
	if (AI2gLocalPath.start_aknode == NULL) {
		AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, AI2cError_Pathfinding_NoBNVAtStart,
			  AI2gLocalPath.owner, &AI2gLocalPath.start_point, &AI2gLocalPath.start_point, NULL, NULL);

		return UUcError_Generic;
	}
	AI2gLocalPath.start_node = PHrAkiraNodeToGraphNode(AI2gLocalPath.start_aknode, AI2gLocalPath.graph);
	if (AI2gLocalPath.start_node == NULL) {
		AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, AI2cError_Pathfinding_NoBNVAtStart,
			  AI2gLocalPath.owner, &AI2gLocalPath.start_point, &AI2gLocalPath.start_point, NULL, NULL);

		return UUcError_Generic;
	}

	// find where the start point is in grid space
	PHrWorldToGridSpaceDangerous(&AI2gLocalPath.start_x, &AI2gLocalPath.start_y,
						&AI2gLocalPath.start_point, &AI2gLocalPath.start_aknode->roomData);
	AI2gLocalPath.start_x = UUmPin(AI2gLocalPath.start_x, 0, (UUtInt16) (AI2gLocalPath.start_aknode->roomData.gridX - 1));
	AI2gLocalPath.start_y = UUmPin(AI2gLocalPath.start_y, 0, (UUtInt16) (AI2gLocalPath.start_aknode->roomData.gridY - 1));
	AI2gLocalPath.bnv_hint = NULL;
	AI2gLocalPath.escape_distance = inEscapeDistance;

	found = UUcFalse;
	attempt = 0;
	while (1) {
		if (attempt == 0) {
			current_direction = inDirection;
		} else {
			current_direction = inDirection + attempt * inSideVary * ((side == 0) ? +1 : -1);
		}
		UUmTrig_Clip(current_direction);

		MUmVector_Set(AI2gLocalPath.end_point, MUrSin(current_direction), 0, MUrCos(current_direction));
		MUmVector_Scale(AI2gLocalPath.end_point, inDistance);
		MUmVector_Add(AI2gLocalPath.end_point, AI2gLocalPath.end_point, AI2gLocalPath.start_point);

		// try the local path in this direction
		AI2gLocalPath.nodes_crossed = 0;
		cur_path_ok = AI2rDoLocalMovement(&cur_weight, &cur_escape_path, &cur_stop_point);

		if (cur_path_ok) {
			// we have found a viable direction
			*outPoint = AI2gLocalPath.end_point;
			*outWeight = cur_weight;
			*outEscape = cur_escape_path;
			*outSuccess = UUcTrue;
			return UUcError_None;
		}

		cur_distsq = MUmVector_GetDistanceSquared(cur_stop_point, AI2gLocalPath.start_point);
		if (!found || (cur_distsq > found_distsq)) {
			// this path is longer than our current best one. store its values.
			found_weight = cur_weight;
			found_escape_path = cur_escape_path;
			found_distsq = cur_distsq;
			found_stop_point = cur_stop_point;
			found = UUcTrue;
		}

		// keep looking for a better direction
		if ((attempt == 0) || (side == 1)) {
			attempt++;
			side = 0;

			if (attempt > inSideAttempts) {
				if (found && (cur_distsq > UUmSQR(inMinDistance))) {
					// we have found a path that is acceptable, return it
					*outPoint = found_stop_point;
					*outWeight = found_weight;
					*outEscape = found_escape_path;
					*outSuccess = UUcFalse;
					return UUcError_None;
				} else {
					// we have not found any path that was acceptable
					return UUcError_Generic;
				}
			}
		} else {
			side = 1;
		}
	}
}

// find a path through the local environment, in a specified direction
UUtError AI2rFindLocalPath(ONtCharacter *ioCharacter, PHtPathMode inPathMode, M3tPoint3D *inStartPoint,
						   AKtBNVNode *inStartNode, PHtNode *inStartPathNode,
						   float inDistance, float inDirection, float inSideVary,
						   UUtUns32 inSideAttempts, UUtUns8 inMaxWeight, UUtUns8 inEscapeDistance,
						   UUtBool *outSuccess, M3tPoint3D *outPoint, UUtUns8 *outWeight, UUtBool *outEscape)
{
	M3tPoint3D				stop_point;
	float					current_direction;
	UUtUns32				attempt, side;
	UUtUns8					found_weight;
	UUtBool					path_ok, escape_path;

	AI2gLocalPath.owner = ioCharacter;
	AI2gLocalPath.path_mode = inPathMode;
	AI2gLocalPath.max_weight = inMaxWeight;
	AI2gLocalPath.graph = ONrGameState_GetGraph();
	AI2gLocalPath.environment = ONrGameState_GetEnvironment();

	if (inStartPoint == NULL) {
		// use the character's current position
		ONrCharacter_GetPathfindingLocation(ioCharacter, &AI2gLocalPath.start_point);

		AI2gLocalPath.start_aknode = ioCharacter->currentNode;
		if (AI2gLocalPath.start_aknode == NULL) {
			AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, AI2cError_Pathfinding_NoBNVAtStart,
				  AI2gLocalPath.owner, &AI2gLocalPath.start_point, &AI2gLocalPath.start_point, NULL, NULL);

			return UUcError_Generic;
		}
		AI2gLocalPath.start_node = ioCharacter->currentPathnode;
		if (AI2gLocalPath.start_node == NULL) {
			AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, AI2cError_Pathfinding_NoBNVAtStart,
				  AI2gLocalPath.owner, &AI2gLocalPath.start_point, &AI2gLocalPath.start_point, NULL, NULL);

			return UUcError_Generic;
		}
	} else {
		// use the supplied starting point
		AI2gLocalPath.start_point = *inStartPoint;

		AI2gLocalPath.start_aknode = inStartNode;
		if (AI2gLocalPath.start_aknode == NULL) {
			AI2gLocalPath.start_aknode = AKrNodeFromPoint(&AI2gLocalPath.start_point);
		}
		if (AI2gLocalPath.start_aknode == NULL) {
			AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, AI2cError_Pathfinding_NoBNVAtStart,
				  AI2gLocalPath.owner, &AI2gLocalPath.start_point, &AI2gLocalPath.start_point, NULL, NULL);

			return UUcError_Generic;
		}

		AI2gLocalPath.start_node = inStartPathNode;
		if (AI2gLocalPath.start_node == NULL) {
			AI2gLocalPath.start_node = PHrAkiraNodeToGraphNode(AI2gLocalPath.start_aknode, AI2gLocalPath.graph);
		}
		if (AI2gLocalPath.start_node == NULL) {
			AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, AI2cError_Pathfinding_NoBNVAtStart,
				  AI2gLocalPath.owner, &AI2gLocalPath.start_point, &AI2gLocalPath.start_point, NULL, NULL);

			return UUcError_Generic;
		}
	}

	// find where the start point is in grid space
	PHrWorldToGridSpaceDangerous(&AI2gLocalPath.start_x, &AI2gLocalPath.start_y,
						&AI2gLocalPath.start_point, &AI2gLocalPath.start_aknode->roomData);
	AI2gLocalPath.start_x = UUmPin(AI2gLocalPath.start_x, 0, (UUtInt16) (AI2gLocalPath.start_aknode->roomData.gridX - 1));
	AI2gLocalPath.start_y = UUmPin(AI2gLocalPath.start_y, 0, (UUtInt16) (AI2gLocalPath.start_aknode->roomData.gridY - 1));
	AI2gLocalPath.bnv_hint = NULL;
	AI2gLocalPath.escape_distance = inEscapeDistance;

	attempt = 0;
	while (1) {
		if (attempt == 0) {
			current_direction = inDirection;
		} else {
			current_direction = inDirection + attempt * inSideVary * ((side == 0) ? +1 : -1);
		}
		UUmTrig_Clip(current_direction);

		MUmVector_Set(AI2gLocalPath.end_point, MUrSin(current_direction), 0, MUrCos(current_direction));
		MUmVector_Scale(AI2gLocalPath.end_point, inDistance);
		MUmVector_Add(AI2gLocalPath.end_point, AI2gLocalPath.end_point, AI2gLocalPath.start_point);

		// try the local path in this direction
		AI2gLocalPath.nodes_crossed = 0;
		path_ok = AI2rDoLocalMovement(&found_weight, &escape_path, &stop_point);

		if (path_ok) {
			// we have found a viable direction
			MUmVector_Copy(*outPoint, AI2gLocalPath.end_point);
			*outWeight = found_weight;
			*outEscape = escape_path;
			*outSuccess = UUcTrue;
			return UUcError_None;

		} else if (attempt == 0) {
			// we haven't found a path, but store the values that we found
			// on our first attempt as fallback return values if we fail
			// to find any better
			MUmVector_Copy(*outPoint, stop_point);
			*outWeight = found_weight;
			*outEscape = escape_path;
		}

		// keep looking for a better direction
		if ((attempt == 0) || (side == 1)) {
			attempt++;
			side = 0;

			if (attempt > inSideAttempts) {
				// we haven't found any better. return the values stored
				// by our initial attempt.
				*outSuccess = UUcFalse;
				return UUcError_None;
			}
		} else {
			side = 1;
		}
	}
}

// check to see whether a specified path through the local environment is viable
UUtError AI2rTryLocalMovement(ONtCharacter *ioCharacter, PHtPathMode inPathMode, M3tPoint3D *inStart, M3tPoint3D *inEnd, UUtUns8 inEscapeDistance,
							   UUtBool *outMoveOK, M3tPoint3D *outStopPoint, UUtUns8 *outWeight, UUtBool *outEscape)
{
	AI2gLocalPath.owner = ioCharacter;
	AI2gLocalPath.path_mode = inPathMode;

	MUmVector_Copy(AI2gLocalPath.start_point,	*inStart);
	MUmVector_Copy(AI2gLocalPath.end_point,		*inEnd);
	AI2gLocalPath.max_weight = PHcDanger;

	AI2gLocalPath.graph = ONrGameState_GetGraph();
	AI2gLocalPath.environment = ONrGameState_GetEnvironment();

	// find the BNV where we currently are
	AI2gLocalPath.start_aknode = AKrNodeFromPoint(&AI2gLocalPath.start_point);
	if (AI2gLocalPath.start_aknode == NULL) {
		AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, AI2cError_Pathfinding_NoBNVAtStart,
			  AI2gLocalPath.owner, &AI2gLocalPath.start_point, &AI2gLocalPath.end_point, NULL, NULL);

		*outMoveOK = UUcFalse;
		return UUcError_Generic;
	}

	// find the pathfinding data for this BNV
	AI2gLocalPath.start_node = PHrAkiraNodeToGraphNode(AI2gLocalPath.start_aknode, AI2gLocalPath.graph);
	if (AI2gLocalPath.start_node == NULL) {
		AI2_ERROR(AI2cWarning, AI2cSubsystem_Pathfinding, AI2cError_Pathfinding_NoBNVAtStart,
			  AI2gLocalPath.owner, &AI2gLocalPath.start_point, &AI2gLocalPath.end_point, NULL, NULL);

		*outMoveOK = UUcFalse;
		return UUcError_Generic;
	}

	// find where the start point is in grid space
	PHrWorldToGridSpaceDangerous(&AI2gLocalPath.start_x, &AI2gLocalPath.start_y,
						&AI2gLocalPath.start_point, &AI2gLocalPath.start_aknode->roomData);
	AI2gLocalPath.start_x = UUmPin(AI2gLocalPath.start_x, 0, (UUtInt16) (AI2gLocalPath.start_aknode->roomData.gridX - 1));
	AI2gLocalPath.start_y = UUmPin(AI2gLocalPath.start_y, 0, (UUtInt16) (AI2gLocalPath.start_aknode->roomData.gridY - 1));
	AI2gLocalPath.bnv_hint = NULL;
	AI2gLocalPath.escape_distance = inEscapeDistance;

	// try the local path in this direction
	AI2gLocalPath.nodes_crossed = 0;
	*outMoveOK = AI2rDoLocalMovement(outWeight, outEscape, outStopPoint);

	return UUcError_None;
}

// try a path through the local environment
//
// sets outWeight and outEscape always - outPoint only if stopped before the path is complete
static UUtBool AI2rDoLocalMovement(UUtUns8 *outWeight, UUtBool *outEscape, M3tPoint3D *outPoint)
{
	UUtUns32 *obstruction_bv, segment;
	UUtInt32 lastvalid_x, lastvalid_y;
	UUtInt16 w, h, end_x, end_y, cur_x, cur_y, segmentend_x, segmentend_y;
	UUtUns8 current_weight, low_weight, high_weight;
	UUtBool path_ok, segment_escape_path;
	M3tPoint3D world_point, test_point;
	AKtBNVNode *current_bnv, *new_bnv;
	PHtNode *current_node;
	PHtRoomData *current_room;
	float nudge_val;

	current_bnv = AI2gLocalPath.start_aknode;
	current_node = AI2gLocalPath.start_node;
	current_room = &AI2gLocalPath.start_aknode->roomData;
	cur_x = AI2gLocalPath.start_x;
	cur_y = AI2gLocalPath.start_y;
	current_weight = PHcClear;
	*outEscape = UUcFalse;

	do {
		w = (UUtInt16) current_room->gridX;
		h = (UUtInt16) current_room->gridY;

		UUmAssert((cur_x >= 0) && (cur_x < w) && (cur_y >= 0) && (cur_y < h));

		// calculate the endpoint of this attempted path in gridspace for the current room
		PHrWorldToGridSpaceDangerous(&end_x, &end_y, &AI2gLocalPath.end_point, current_room);

		// if the endpoint lies outside our gridspace, chop the local path up into several
		// sections and check them separately
		segment = AI2cLocalPath_Segment_None;
		if (end_x < 0) {
			segment = AI2cLocalPath_Segment_NegX;
			segmentend_x = 0;
			segmentend_y = cur_y + (UUtInt16) MUrFloat_Round_To_Int((end_y - cur_y) *
															((float) (0 - cur_x)) / (end_x - cur_x));
		} else if (end_x >= w) {
			segment = AI2cLocalPath_Segment_PosX;
			segmentend_x = w - 1;
			segmentend_y = cur_y + (UUtInt16) MUrFloat_Round_To_Int((end_y - cur_y) *
															((float) ((w - 1) - cur_x)) / (end_x - cur_x));
		} else {
			segmentend_x = end_x;
			segmentend_y = end_y;
		}

		if (segmentend_y < 0) {
			segment = AI2cLocalPath_Segment_NegY;
			segmentend_x = cur_x + (UUtInt16) MUrFloat_Round_To_Int((segmentend_x - cur_x) *
															((float) (0 - cur_y)) / (segmentend_y - cur_y));
			segmentend_y = 0;

		} else if (segmentend_y >= h) {
			segment = AI2cLocalPath_Segment_PosY;
			segmentend_x = cur_x + (UUtInt16) MUrFloat_Round_To_Int((segmentend_x - cur_x) *
															((float) ((h - 1) - cur_y)) / (segmentend_y - cur_y));
			segmentend_y = h - 1;
		}

		// we have now clipped the line to our gridspace bounds
		UUmAssert((segmentend_x >= 0) && (segmentend_x < w) && (segmentend_y >= 0) && (segmentend_y < h));

		if (AI2gLocalPath.nodes_crossed == 0) {
			// use the obstruction bitvector that is specifically allocated for our starting room
			obstruction_bv = AI2gLocalPath.startroom_obstruction_bitvector;
		} else {
			// use the general-purpose obstruction bitvector, but clear it first
			obstruction_bv = AI2gLocalPath.other_obstruction_bitvector;
			UUrBitVector_ClearBitAll(obstruction_bv, 2 * PHcMaxObstructions);
		}

		// check to see if this line segment is traversable in our grid
		PHrPrepareRoomForPath(current_node, current_room);
		path_ok = PHrLocalPathWeight(AI2gLocalPath.owner, AI2gLocalPath.path_mode, current_node, current_room, obstruction_bv,
									AI2gLocalPath.max_weight, AI2gLocalPath.escape_distance, cur_x, cur_y, segmentend_x, segmentend_y,
									&segment_escape_path, &low_weight, &high_weight, &lastvalid_x, &lastvalid_y);

		if (!path_ok) {
			// stop the traversal, we cannot move in this direction. calculate how far we *did* get.
			UUmAssert((lastvalid_x >= 0) && (lastvalid_x < w) && (lastvalid_y >= 0) && (lastvalid_y < h));
			PHrGridToWorldSpace((UUtUns16) lastvalid_x, (UUtUns16) lastvalid_y, NULL, outPoint, current_room);

			*outWeight = high_weight;
			return UUcFalse;
		}

		if (segment_escape_path) {
			// determine weight based on where we can get to
			current_weight = UUmMax(low_weight, current_weight);
			*outEscape = UUcTrue;
		} else {
			// determine weight base on where we would end up
			current_weight = UUmMax(high_weight, current_weight);
		}

		if (segment == AI2cLocalPath_Segment_None) {
			// we have successfully traversed our way all the way to the end point
			*outWeight = current_weight;
			MUmVector_Copy(*outPoint, AI2gLocalPath.end_point);
			return UUcTrue;
		}

		// we have traversed until we hit the edge of our pathfinding grid. work out where this is
		// in worldspace
		PHrGridToWorldSpace(segmentend_x, segmentend_y, NULL, &world_point, current_room);

		current_bnv = NULL;
		if ((AI2gLocalPath.nodes_crossed == 0) && (AI2gLocalPath.bnv_hint != NULL)) {
			// this is our first crossing. use spatial locality to check whether world_point
			// lies inside the last first-crossing node that we found (which was stored in bnv_hint)
			PHrWorldToGridSpaceDangerous(&cur_x, &cur_y, &world_point, &AI2gLocalPath.bnv_hint->roomData);

			if ((cur_x < 0) || (cur_x >= (UUtInt16) current_room->gridX) ||
				(cur_y < 0) || (cur_y >= (UUtInt16) current_room->gridY)) {
				// this BNV doesn't overlap our new point. we need to search all nearby BNVs instead.
			} else {
				current_bnv = AI2gLocalPath.bnv_hint;
			}
		}

		if (current_bnv == NULL) {
			// nudge this point just a little off the grid. we must move it half a square from the
			// center of the square to the edge, and then another AKcBNV_OutsideAcceptDistance to be sure that it's
			// outside when tested against the BNV
			MUmVector_Copy(test_point, world_point);
			nudge_val = (current_room->squareSize * 0.5f + 1.5f * AKcBNV_OutsideAcceptDistance);
			switch(segment) {
			case AI2cLocalPath_Segment_NegX:
				test_point.x -= nudge_val;
				break;

			case AI2cLocalPath_Segment_PosX:
				test_point.x += nudge_val;
				break;

			case AI2cLocalPath_Segment_NegY:
				test_point.z -= nudge_val;
				break;

			case AI2cLocalPath_Segment_PosY:
				test_point.z += nudge_val;
				break;

			default:
				UUmAssert(!"AI2rDoLocalMovement: unknown segmentation boundary");
				break;
			}

			// find which BNV this point lies in
			new_bnv = AKrNodeFromPoint(&test_point);
			if ((new_bnv == NULL) || (new_bnv == current_bnv)) {
				// after nudging our test point outside the BNV, we are still apparently inside it
				// this means that there must be no BNVs that actually enclose the test point
			} else {
				current_bnv = new_bnv;
				current_room = &current_bnv->roomData;

				if (0 == (current_bnv->flags & AKcBNV_Flag_NonAI)) {
					// work out whereabouts we currently are in the grid space of the new grid
					PHrWorldToGridSpaceDangerous(&cur_x, &cur_y, &test_point, current_room);

					if ((cur_x < 0) || (cur_x >= (UUtInt16) current_room->gridX) ||
						(cur_y < 0) || (cur_y >= (UUtInt16) current_room->gridY)) {
						// we aren't actually in the new space... i.e. we are slightly off the edge
						// of the BNV's extents but because there were no BNVs that actually contained us,
						// it was returned instead of NULL.
						current_bnv = NULL;
					} else {
						current_node = PHrAkiraNodeToGraphNode(current_bnv, AI2gLocalPath.graph);
					}
				} else {
					current_bnv = NULL;
				}
			}
		}

		if (current_bnv == NULL) {
			// we cannot move in this direction because there is no AI-navigable BNV there!
			MUmVector_Copy(*outPoint, world_point);
			*outWeight = current_weight;
			return UUcFalse;
		}

		if (AI2gLocalPath.nodes_crossed == 0) {
			// store the BNV that we found in bnv_hint so that we will test it next time
			// instead of having to search all nearby BNVs
			AI2gLocalPath.bnv_hint = current_bnv;
		}

		AI2gLocalPath.nodes_crossed++;
	} while (1);
}



