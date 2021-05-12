#pragma once

/*
	Oni_Path.h
	
	This file contains all pathfinding header stuff
	
	Author: Quinn Dunki, Michael Evans
	Copyright (c) Bungie Software 1998, 2000
*/

#ifndef __ONI_PATH_H__
#define __ONI_PATH_H__

#include "BFW.h"
#include "BFW_Path.h"
#include "BFW_Object.h"
#include "Oni_GameState.h"
#include "lrar_cache.h"

typedef struct PHtNode PHtNode;
typedef struct PHtConnection PHtConnection;
#define PHcMaxPathLength 200		/* Maxiumum length of a graph path (in nodes) */

// dynamic data such as location of characters, objects, etc ages three times every second
#define PHcDynamicDataAgeTime	20

// 256k grid cache (a typical pathfinding grid is 100x100x3 bytes = 30k)
#define PHcGridCacheSize		256 * 1024

// max number of obstructing characters or objects in a BNV
#define PHcMaxObstructions		32

// bitflags for obstructions
#define PHcObstructionTypeMask			0xc000
#define PHcObstructionIndexMask			0x3fff
#define PHcObstructionType_Object		(0 << 14)
#define PHcObstructionType_Door			(1 << 14)
#define PHcObstructionType_Character	(2 << 14)

extern UUtUns16 PHgDebugX[2];
extern UUtUns16 PHgDebugY[2];
extern PHtConnection *PHgDebugConnection;

extern UUtBool PHgRenderDynamicGrid;
extern UUtUns16 PHgDebugSquareX, PHgDebugSquareY;

extern const IMtShade PHgPathfindingWeightColor[PHcMax];

// CB: bitmasks and flags for PHtConnection->flags
#define PHcConnectionMask_Point0Shift		16
#define PHcConnectionMask_Point1Shift		18
#define PHcConnectionMask_Point0Index		(((UUtUns32) 3) << PHcConnectionMask_Point0Shift)
#define PHcConnectionMask_Point1Index		(((UUtUns32) 3) << PHcConnectionMask_Point1Shift)
#define PHmConnection_Point0Index(flags)	((flags & PHcConnectionMask_Point0Index) >> PHcConnectionMask_Point0Shift)
#define PHmConnection_Point1Index(flags)	((flags & PHcConnectionMask_Point1Index) >> PHcConnectionMask_Point1Shift)

#define PHmConnection_Point0(env,conn)		(env->pointArray->points + conn->gunk->m3Quad.vertexIndices.indices[PHmConnection_Point0Index(conn->flags)])
#define PHmConnection_Point1(env,conn)		(env->pointArray->points + conn->gunk->m3Quad.vertexIndices.indices[PHmConnection_Point1Index(conn->flags)])

// CB: index computation in global weight array. note: must have i < j to use this, due to symmetry
#define PHmNodeWeightIndex(node,i,j)		(node->weight_baseindex + i * node->num_connections_out + j)

enum {
	PHcConnectionFlag_Impassable		= (1 << 0),
	PHcConnectionFlag_Stairs			= (1 << 1),
	PHcConnectionFlag_ObstructedPoint0	= (1 << 2),
	PHcConnectionFlag_ObstructedPoint1	= (1 << 3),
	PHcConnectionFlag_Invalid			= (1 << 4)
};

// different modes of checking to see whether we can pass through
// a given space... from least permissive to most permissive (e.g.
// a door is not considered clear space as far as checking to see
// whether we can appear there, but we can move through it)
typedef enum {
	PHcPathMode_CheckClearSpace			= 0,
	PHcPathMode_CasualMovement			= 1,
	PHcPathMode_DirectedMovement		= 2
} PHtPathMode;

enum {	// Path status types
	PHcPath_Undef,
	PHcPath_Open,
	PHcPath_Closed
};

// Digraph pathfinding data types
struct PHtConnection
{
	UUtUns32 flags;
	
	PHtNode *from;
	PHtNode *to;							// Who we connect (note that connections are ONE WAY)
	UUtUns16 from_connectionindex;			// index of this connection in from->out_connections
	UUtUns16 to_connectionindex;			// index of this connection in to->in_connections

	AKtGQ_General *gunk;					// Gunk quad that this connection represents
	M3tPoint3D connection_midpoint;
	float connection_width;
	
	UUtBool door_side;
	UUtUns32 last_gridindex;				// unique grid construction index for the last pass we were added to
	struct OBJtObject *door_link;			// the door that blocks this connection

	PHtConnection *prev_out, *next_out;		// linked list ptrs for from->out_connections
	PHtConnection *prev_in, *next_in;		// linked list ptrs for to->in_connections
	UUtUns16 blockage_baseindex;			// indices into PHtGraph->blockage_array
	UUtUns16 num_blockages;

#if TOOL_VERSION
	UUtUns32 traverse_id, traverse_dist;	// debugging - for traversing the graph and displaying it
	M3tPoint3D traverse_pts[3];
#endif
};

struct PHtNode
{
	/*
	 * properties of this pathfinding node
	 */
	UUtUns32 num_connections_out;
	PHtConnection *connections_out;			// List of paths branching from us
	UUtUns32 num_connections_in;
	PHtConnection *connections_in;			// List of paths that lead to us
	AKtBNVNode *location;					// Corresponding BNV
	UUtUns32 weight_baseindex;				// index into PHtGraph->weight_storage
	
	/*
	 * cached room grids - both static and dynamic
	 */
	UUtUns32 gridX, gridY, array_size;
	UUtBool grids_allocated, no_static_grid;
	PHtSquare *static_grid;
	PHtDynamicSquare *dynamic_grid;
	short cache_block_index;
	UUtUns32 calculate_time;
	UUtUns16 obstruction[PHcMaxObstructions];

	/*
	 * search-time variables
	 */
	UUtUns16 pathStatus;					// Open or closed?
	UUtUns16 pathDepth;						// depth here from start node
	PHtNode *next;							// for storing linked lists while searching
	float g,h,f;							// heuristic estimates
	float weight_multiplier;				// used for sound propagation through doors

	PHtNode *predecessor;					// how we got to this node (NULL if initial node)
	PHtConnection *predecessor_conn;
	
#if TOOL_VERSION
	UUtUns32 traverse_dist;		// debugging - for traversing the graph and displaying it
#endif
};

struct PHtGraph
{
	PHtNode *nodes;					// Array of graph nodes
	UUtUns32 numNodes;
	AKtEnvironment *env;			// The Akira environment we represent
	
	PHtNode *open;					// Shortest determined nodes
	PHtNode *closed;				// Retired Dijkstra nodes
	
	lrar_cache *grid_cache;			// lrar cache for pathfinding grids
	UUtUns8 *grid_storage;			// memory storage for pathfinding grids

	UUtUns32 num_weights;			// memory storage for node weights
	float *weight_storage;

	UUtMemory_Array *blockage_array;// blocked sections of ghost quads
};


// Construction/destruction
UUtError PHrBuildGraph(PHtGraph *ioGraph, AKtEnvironment *inEnv);
void PHrDisposeGraph(PHtGraph *ioGraph);
void PHrVerifyGraph(PHtGraph *ioGraph, UUtBool inCheckIndices);
void PHrResetNode(PHtNode *ioNode);
void PHrConnectStairs(PHtGraph *ioGraph, PHtNode *ioNode, AKtEnvironment *inEnv);

// Pathfinding
UUtBool PHrGeneratePath(
	PHtGraph *ioGraph,
	ONtCharacter *inCharacter,		// used for door-opening tests, can be NULL
	UUtUns32 inSoundType,			// (UUtUns32) -1 if not a sound test
	PHtNode *inA,
	PHtNode *inB,
	M3tPoint3D *inStart,
	M3tPoint3D *inEnd,
	float *ioDistance,				// NULL for no distance calc, otherwise read = max distance, write = actual distance
	PHtConnection **outConnections,	// either NULL or a pointer to an array of pointers PHcMaxPathLength-1 long
	UUtUns32 *outNumNodes);

// grid caching

void PHrPrepareRoomForPath(PHtNode *inNode, const PHtRoomData *inRoom);

UUtBool PHrSquareIsPassable(ONtCharacter *inCharacter, PHtPathMode inPathMode,
							PHtNode *inNode, const PHtRoomData *inRoom, UUtUns32 *inObstructionBV,
							UUtUns16 inX, UUtUns16 inY, UUtBool *outObstruction, UUtUns8 *outWeight);

float PHrEstimateDistance(PHtNode *inNodeA, M3tPoint3D *inPointA, PHtNode *inNodeB, M3tPoint3D *inPointB);
PHtNode *PHrPopCheapest(PHtNode **inHead);
PHtConnection *PHrFindConnection(PHtNode *inA, PHtNode *inB, M3tPoint3D *inPointB);

// Utilities
AKtBNVNode *PHrFindRoomParent(const PHtRoomData *inRoom, const AKtEnvironment *inEnv);

UUtBool PHrPointInRoom(AKtEnvironment *inEnv, PHtRoomData *inRoom, M3tPoint3D *inPoint);
PHtRoomData *PHrRoomFromPoint(AKtEnvironment *inEnv, M3tPoint3D *inPoint);
UUtUns32 PHrAkiraNodeToIndex(AKtBNVNode *inNode, AKtEnvironment *inEnv);
void PHrAppendToList(PHtNode *inElem, PHtNode **inHead);
void PHrRemoveFromList(PHtNode *inVictim, PHtNode **inHead);
void PHrAddNode(PHtGraph *ioGraph, AKtBNVNode *inRoom, UUtUns32 inSlot);
PHtNode *PHrAkiraNodeToGraphNode(AKtBNVNode *inAkiraNode, PHtGraph *inGraph);
void PHrAddCharacterToGrid(UUtUns32 inIndex, ONtCharacter *inCharacter, PHtDynamicSquare *inGrid,
						   UUtUns16 inGridX, UUtUns16 inGridY, PHtNode *inNode, const PHtRoomData *inRoom);
void PHrAddObjectToGrid(UUtUns32 inIndex, OBtObject *inObject, PHtDynamicSquare *inGrid,
						UUtUns16 inGridX, UUtUns16 inGridY, PHtNode *inNode, const PHtRoomData *inRoom);
void PHrProjectOntoConnection(AKtEnvironment *inEnvironment, PHtConnection *inConnection,
							  M3tPoint3D *inStartPoint, M3tPoint3D *ioTargetPoint);
float PHrGetConnectionHeight(PHtConnection *inConnection);
void PHrNotifyDoorStateChange(PHtConnection *inConnection);
void PHrFlushDynamicGrid(PHtNode *inNode);

UUtBool PHrIgnoreObstruction(ONtCharacter *inCharacter, PHtPathMode inPathMode, UUtUns16 inObstruction);
UUtBool PHrObstructionIsCharacter(UUtUns16 inObstruction, ONtCharacter **outCharacter);
UUtBool PHrObstructionIsDoor(UUtUns16 inObstruction, struct OBJtObject **outDoor);

// Misc
void PHrRenderGrid(PHtNode *inNode, PHtRoomData *inRoom, IMtPoint2D *inErrStart, IMtPoint2D *inErrEnd);
void PHrDisplayGraph(PHtGraph *inGraph);

#if TOOL_VERSION
// DEBUGGING: generate and display grid
void PHrDebugGraph_TraverseConnections(PHtGraph *ioGraph, PHtNode *inStartNode, PHtNode *inOtherNode,
										UUtUns32 inDistanceLimit);
void PHrDebugGraph_Render(void);
void PHrDebugGraph_Terminate(void);
#endif

// perform a straight-line search through the pathfinding grid. stops at impassable or danger,
// returns false and sets stop_x and stop_y. if path is successful, returns true. always sets highest weight.
UUtBool PHrLocalPathWeight(ONtCharacter *inCharacter, PHtPathMode inPathMode, PHtNode *inNode, PHtRoomData *inRoom, UUtUns32 *inObstructionBV,
				UUtUns8 stop_weight, UUtUns32 passthrough_squares, UUtInt32 x1, UUtInt32 y1, UUtInt32 x2, UUtInt32 y2,
				UUtBool *escape_path, UUtUns8 *lowest_weight, UUtUns8 *highest_weight, UUtInt32 *last_x, UUtInt32 *last_y);

// find a distance approximation from one point to another through the pathfinding graph
//
// inCharacter: optional (may be NULL), used for determining whether doors are passable
// inSoundType: (UUtUns32) -1 for non-sound, otherwise adds sound propagation muffle values
// inFromNode and inToNode: if NULL, will be calculated
// ioDistance: value passed in is maximum distance to look (leave zero if no max distance)
UUtError PHrPathDistance(ONtCharacter *inCharacter, UUtUns32 inSoundType,
						 M3tPoint3D *inFrom, M3tPoint3D *inTo,
						PHtNode *inFromNode, PHtNode *inToNode, float *ioDistance);
#endif
