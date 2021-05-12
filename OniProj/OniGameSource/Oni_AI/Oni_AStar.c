// ======================================================================
// Oni_AStar.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "Oni_AStar.h"
#include "Oni_AI.h"
//#include <math.h>
#include "bfw_math.h"
#include <stdlib.h>
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"
#include "BFW_Object.h"
#include "BFW_Path.h"
#include "BFW_Physics.h"
#include "BFW_Console.h"
#include "BFW_MathLib.h"
#include "BFW_Timer.h"

#define DEBUG_VERBOSE_ASTAR			0
#define ASTAR_USE_HEAP				1

// ======================================================================
// defines
// ======================================================================
#define AScMaxNodes					10240
#define AScNumDirLists				4

#define NO_REVISITING_NODES			0

#define AScMaxStartImpassableRadius 3.0f
#define AScMaxDestImpassableRadius  3

// ======================================================================
// enums
// ======================================================================
enum
{
	AScStatusFlag_Open		= 0x0001,
	AScStatusFlag_Closed	= 0x0002,
	AScStatusFlag_FinalPath	= 0x0004,
	AScStatusMask			= 0x0007,
	AScStatusFlag_Impassable= 0x0008,	// may be set if our start point was impassable
	AScStatusFlag_Obstructed= 0x0010,	// set if the impassability is caused by an obstruction
	AScStatusFlag_Stairs	= 0x0020	// set if we're inside the area covered by a stair outlet
};

// ======================================================================
// typedefs
// ======================================================================

typedef struct AStGridPoint
{
	UUtUns8		x;
	UUtUns8		y;

} AStGridPoint;

// heap callback functions
typedef UUtInt32 (*AStHeap_CompareFunc)(UUtUns32 inUserData, UUtUns16 inA, UUtUns16 inB);
typedef void (*AStHeap_NotifyLocation)(UUtUns32 inUserData, UUtUns16 inElement, UUtUns16 inHeapLocation);

struct AStPathNode
{
	UUtUns16				flags;
#if ASTAR_USE_HEAP
	UUtUns16				heapindex;	// index in the node heap
#else
	AStPathNode				*next;		// next node in list
	AStPathNode				*prev;		// prev node in list
#endif

	UUtUns16				adjacent;	// during pathfinding, points to previous node in the path;
										// after path is found, points to next node in the path
	AStGridPoint			loc;		// location in the grid
	
	float					f;			// g + estimated cost to reach finish
	float					g;			// actual cost to reach this node
};

typedef struct AStHeap
{
	UUtUns32				heap_size;
	UUtUns32				heap_userdata;
	AStHeap_CompareFunc		func_compare;
	AStHeap_NotifyLocation	func_notify;

	UUtUns32				heap_count;
	UUtUns16				*heap;
} AStHeap;

#define AScIndexArrayMaxUsedCount	256

struct AStPath
{
#if ASTAR_USE_HEAP
	AStHeap					open_heap;		// open heap of nodes
#else
	AStPathNode				*open_list;		// open list of nodes
#endif

	PHtNode					*roomnode;
	const PHtRoomData		*room;			// room to generate path in

	UUtUns16				*index_array;	// stores the index in the node array
											// of the node that lies at a particular location
	
	AStGridPoint			start;			// start location in room
	AStGridPoint			end;			// end location in room
	AStGridPoint			reachable_end;	// end location of path
	//AKtGQ					*endingGunk;	// Nonnull if the endpoint is on a PRT
	M3tPoint3D				worldStart;		// Worldspace startpoint of local path
	M3tPoint3D				worldEnd;		// Worldspace endpoint of local path
	ONtCharacter			*owner;			// The character who is using this path
	UUtBool					allow_stairs;	// whether we are trying to get to some stairs

#if TOOL_VERSION
	UUtBool					show_debug;		// write points into global debugging array
	UUtBool					show_evaluation;// write grid locations into global debugging array
#endif

	AStPathNode				*path_start;	// first node in the found path
	AStPathNode				*path_end;		// last node in the found path
	UUtBool					finished;		// true if a path has been found
	AStPathNode				*node_array;	// an array of nodes
	UUtUns16				num_allocs;		// num nodes alloced so far
	
	UUtBool					params_set;		// true if required parameters are set
	
	UUtUns16				dir_index;		// index of ASgDir to use

	UUtUns32				*obstruction_bv;// bitvector that indicates whether obstructions
											// in the grid have already been tested and discarded

	UUtUns32				index_array_used_count;
	UUtUns32				index_array_used[AScIndexArrayMaxUsedCount];

	// spheretree that stores the character's end position
	UUtBool					spheretree_built;
	UUtBool					spheretree_set;
	PHtSphereTree			sphereTree[1];
	PHtSphereTree			sphereTree_leaves[ONcSubSphereCount];
	PHtSphereTree			sphereTree_midnodes[ONcSubSphereCount_Mid];
};

// ======================================================================
// globals
// ======================================================================
AStPath						*ASgAstar_Path;

static UUtUns16				ASgDir[AScNumDirLists][8] = 
{
	{0, 1, 2, 3, 4, 5, 6, 7},
	{7, 6, 5, 4, 3, 2, 1, 0},
	{0, 2, 4, 6, 1, 3, 5, 7},			
	{1, 3, 5, 7, 0, 2, 4, 6}
};

static UUtUns8				ASgOrthogonalDirs[8][2] =
{
	{0xFF,	0xFF},
	{0,		2	},
	{0xFF,	0xFF},
	{2,		4	},
	{0xFF,	0xFF},
	{4,		6	},
	{0xFF,	0xFF},
	{6,		0	}
};

static AStPathPointType		ASgPassabilityType[PHcMax] =
{AScPathPoint_Clear, AScPathPoint_Clear, AScPathPoint_Clear, AScPathPoint_Danger, AScPathPoint_Tight,
 AScPathPoint_Danger, AScPathPoint_Danger, AScPathPoint_Danger, AScPathPoint_Impassable};

#if TOOL_VERSION
// TEMPORARY DEBUGGING - buffer of points found for paths
#define AScDebugPath_MaxPoints		4096
typedef struct AStDebugPathPoint {
	UUtBool valid;
	M3tPoint3D point;
} AStDebugPathPoint;

static UUtBool ASgDebugPath_Initialized = UUcFalse;
static UUtUns32 ASgDebugPath_StartIndex, ASgDebugPath_NumUsed, ASgDebugPath_NumPaths;
static AStDebugPathPoint ASgDebugPath[AScDebugPath_MaxPoints];

// TEMPORARY DEBUGGING - buffer of grid points considered in the last A* path generation
typedef struct AStDebugGridPoint {
	IMtShade shade;
	M3tPoint3D point;
} AStDebugGridPoint;

static UUtUns32 ASgDebugGrid_NumPoints = 0;
static AStDebugGridPoint ASgDebugGridPoints[AScMaxNodes];
#endif

#if PERFORMANCE_TIMER
UUtPerformanceTimer *ASgPath_Generate_Timer = NULL;
#endif

// ======================================================================
// prototypes
// ======================================================================
static void
ASiList_CalcWaypoints(
	AStPath					*inAstar,
	UUtUns16				inMaxWaypoints,
	UUtUns16				*outNumWaypoints,
	AStPathPoint			*outWaypoints,
	UUtBool					inCarefulPath);

#if TOOL_VERSION
static void
ASiBuildDebugPath(
	AStPath					*ioAstar,
	UUtUns16				inNumWaypoints,
	AStPathPoint			*inWaypoints);

static void
ASiBuildDebugGridEvaluation(
	AStPath					*ioAstar);
#endif

// ----------------------------------------------------------------------
static UUtBool
ASiCollisionTest(
	AStPath					*inAstar,
	UUtBool					inStartTest, 
	AStGridPoint			*inPoint);

static UUtBool 
ASiDetermineEndPoint(
	AStPath					*inAstar);

static UUtBool 
ASiTryEndPoint(
	AStPath					*inAstar,
	UUtInt16				inX,
	UUtInt16				inY);

#if ASTAR_USE_HEAP
// ----------------------------------------------------------------------
static UUtInt32
ASiAstarHeap_CompareFunc(
	UUtUns32				inUserData,
	UUtUns16				inElementA,
	UUtUns16				inElementB);

static void
ASiAstarHeap_NotifyLocation(
	UUtUns32				inUserData,
	UUtUns16				inElement, 
	UUtUns16				inHeapLocation);

static void
ASiHeap_Insert(
	AStHeap					*ioHeap,
	UUtUns16				inElement);

static UUtUns16
ASiHeap_PopFirstElement(
	AStHeap					*ioHeap);

static void
ASiHeap_ReorderElement(
	AStHeap					*ioHeap,
	UUtUns16				inHeapLocation);

#else		// ASTAR_USE_HEAP
// ----------------------------------------------------------------------
static UUtBool
ASiList_IsEmpty(
	AStPathNode				*inList);

static void
ASiList_InsertNode(
	AStPathNode				**inList,
	AStPathNode				*inNode);

static AStPathNode*
ASiList_RemoveFirstNode(
	AStPathNode 			**inList);

static void
ASiList_RemoveNode_Fast(
	AStPathNode				**inList,
	AStPathNode				*inNode);

#endif		// ASTAR_USE_HEAP

// ----------------------------------------------------------------------
static UUtBool
ASiPassable(
	AStPath					*inAstar,
	AStPathNode				*inNode,
	UUtUns16				inDir,
	UUtBool					inAllowDiagonals,
	AStGridPoint			*outNewLoc,
	float					*outNewCost,
	UUtUns16				*outNewFlags);

static float
ASiDistEstimate(
	AStPathNode				*inNode,
	AStGridPoint			*inEnd);

// ----------------------------------------------------------------------
static UUtError
iDetectLoop(
	UUtUns32				inNodeArraySize,
	AStPathNode				*inNodeArray, 
	AStPathNode				*inNode);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------

static UUcInline void ASrIndexArray_Use(AStPath *inPath, UUtUns32 inIndex)
{
	if (inPath->index_array_used_count < AScIndexArrayMaxUsedCount) {
		inPath->index_array_used[inPath->index_array_used_count] = inIndex;
	}

	inPath->index_array_used_count++;
}


UUtError
ASrInitialize(
	void)
{
	ASgAstar_Path = ASrPath_New();
	if (!ASgAstar_Path) return UUcError_OutOfMemory;

#if PERFORMANCE_TIMER
	ASgPath_Generate_Timer = UUrPerformanceTimer_New("AS_Path_Generate");
#endif
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ASrTerminate(
	void)
{
	ASrPath_Delete(&ASgAstar_Path);
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
AStPath*
ASrPath_New(
	void)
{
	AStPath			*astar;
	
	astar = (AStPath *) UUrMemory_Block_NewClear(sizeof(AStPath));
	if (astar == NULL) {
		goto fail;
	}
	
	// initialize astar
	astar->room				= NULL;
	astar->path_start		= NULL;
	astar->path_end			= NULL;
	astar->finished			= UUcFalse;
	astar->node_array		= UUrMemory_Block_New(sizeof(AStPathNode) * AScMaxNodes);
	if (astar->node_array == NULL) {
		goto fail;
	}

	astar->num_allocs		= 0;
	astar->params_set		= UUcFalse;
	astar->index_array		= UUrMemory_Block_New(sizeof(UUtUns16) * UUmSQR(PHcMaxGridSize));
	if (astar->index_array == NULL) {
		goto fail;
	}
		
	astar->dir_index = 0;
	
	astar->obstruction_bv = UUrBitVector_New(2 * PHcMaxObstructions);
	if (astar->obstruction_bv == NULL) {
		goto fail;
	}

	UUrMemory_Set8(astar->index_array, 0xFF, sizeof(UUtUns16) * PHcMaxGridSize * PHcMaxGridSize);
	astar->index_array_used_count = 0;

	astar->spheretree_set	= UUcFalse;
	astar->spheretree_built	= UUcFalse;

#if ASTAR_USE_HEAP
	astar->open_heap.heap_size		= AScMaxNodes;
	astar->open_heap.heap_count		= 1;			// note: element 0 is always unused
	astar->open_heap.heap_userdata	= (UUtUns32) astar;
	astar->open_heap.func_compare	= ASiAstarHeap_CompareFunc;
	astar->open_heap.func_notify	= ASiAstarHeap_NotifyLocation;
	astar->open_heap.heap			= UUrMemory_Block_New(sizeof(UUtUns16) * astar->open_heap.heap_size);
	if (astar->open_heap.heap == NULL) {
		goto fail;
	}
#else
	astar->open_list		= NULL;
#endif

	return astar;

fail:
	ASrPath_Delete(&astar);
	return NULL;
}

// ----------------------------------------------------------------------
void
ASrPath_Delete(
	AStPath					**ioAstar)
{
	if (*ioAstar)
	{
		if ((*ioAstar)->node_array) {
			UUrMemory_Block_Delete((*ioAstar)->node_array);
			(*ioAstar)->node_array = NULL;
		}
		
		if ((*ioAstar)->index_array) {
			UUrMemory_Block_Delete((*ioAstar)->index_array);
			(*ioAstar)->index_array = NULL;
		}
		
		if ((*ioAstar)->obstruction_bv) {
			UUrBitVector_Dispose((*ioAstar)->obstruction_bv);
			(*ioAstar)->obstruction_bv = NULL;
		}
		
#if ASTAR_USE_HEAP
		if ((*ioAstar)->open_heap.heap) {
			UUrMemory_Block_Delete((*ioAstar)->open_heap.heap);
			(*ioAstar)->open_heap.heap = NULL;
		}
#endif

		UUrMemory_Block_Delete(*ioAstar);
		*ioAstar = NULL;
	}
}

// ----------------------------------------------------------------------
UUtBool
ASrPath_Generate(
	AStPath					*ioAstar,
	UUtUns16				inMaxWaypoints,
	UUtUns16				*outNumWaypoints,
	AStPathPoint			*outWaypoints,
	ONtCharacter			*inWalker,
	UUtBool					inAllowDiagonals,
	UUtBool					inCarefulPath)
{
	AStPathNode				*path, *node;
	UUtUns16				i, dir;
	UUtUns8					weight;
	UUtBool					newly_allocated_node, obstructed, path_available[8];
	UUtUns16				new_flags[8];
	UUtUns16				node_index, location_index;
	AStGridPoint			new_loc[8];
	float					new_cost[8];
	UUtBool					result = UUcFalse;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(ASgPath_Generate_Timer);
#endif
	
	UUmAssert(ioAstar);
	UUmAssert(ioAstar->params_set != UUcFalse);

	// no waypoints to return yet
	*outNumWaypoints = 0;

	// no obstructions have been tested or discarded yet
	UUrBitVector_ClearBitAll(ioAstar->obstruction_bv, PHcMaxObstructions);

	// find whereabouts we can actually get to in this room
	if (!ASiDetermineEndPoint(ioAstar)) {
		// there is nowhere passable near the endpoint that we can get to, a path is not
		// possible to it.
		result = UUcFalse;
		goto exit;
	}

#if ASTAR_USE_HEAP
	// note that element 0 of the heap is always unused to make the equations easier
	ioAstar->open_heap.heap_count = 1;
#else
	ioAstar->open_list = NULL;
#endif

	// create a new path
	path = &ioAstar->node_array[ioAstar->num_allocs];
	UUmAssert(path != NULL);
	
	ioAstar->num_allocs++;

	UUrMemory_Clear(path, sizeof(*path));
	path->adjacent	= (UUtUns16) -1;
	path->f			= ASiDistEstimate(path, &ioAstar->end);
	path->loc		= ioAstar->start;

	// if our start point is impassable then we can walk through impassable grid points
	// until we find some clear space
	if (!PHrSquareIsPassable(ioAstar->owner, PHcPathMode_DirectedMovement,
							ioAstar->roomnode, ioAstar->room, ioAstar->obstruction_bv,
							ioAstar->start.x, ioAstar->start.y, &obstructed, &weight)) {
		path->flags |= AScStatusFlag_Impassable;
	}
	if (obstructed) {
		path->flags |= AScStatusFlag_Obstructed;
	}
	if (weight == PHcStairs) {
		path->flags |= AScStatusFlag_Stairs;
	}

	// if the start and end are the same node, then this path has been found
	if ((path->loc.x == ioAstar->reachable_end.x) && (path->loc.y == ioAstar->reachable_end.y))
	{
		// node is now the last node in the found path
		ioAstar->path_end = path;
		ioAstar->finished = UUcTrue;
		
		// calculate the generated path
		ASiList_CalcWaypoints(
			ioAstar,
			inMaxWaypoints,
			outNumWaypoints,
			outWaypoints,
			inCarefulPath);

#if TOOL_VERSION
		if (ioAstar->show_debug) {
			ASiBuildDebugPath(ioAstar,
							  *outNumWaypoints,
							  outWaypoints);
		}
#endif

#if DEBUG_VERBOSE_ASTAR
		COrConsole_Printf_Color(UUcTrue, 0xFFFF0000, 0xFF800000, "%s: A* on %d [%dx%d]: 0 nodes examined, TRIVIAL SUCCESS",
							inWalker->player_name, ioAstar->roomnode->location->index,
							ioAstar->room->gridX, ioAstar->room->gridY);
#endif
		result = UUcTrue;
		goto exit;
	}

	// calculate the location of this node in the grid
	location_index = path->loc.x + (path->loc.y * (UUtUns16) ioAstar->room->gridX);
	ioAstar->index_array[location_index] = 0;
	ASrIndexArray_Use(ioAstar, location_index);

#if ASTAR_USE_HEAP
	// insert the path onto the open-node heap
	ASiHeap_Insert(&ioAstar->open_heap, path - ioAstar->node_array);
#else
	// insert the path into the open_list list
	ASiList_InsertNode(&ioAstar->open_list, path);
#endif
	path->flags |= AScStatusFlag_Open;

	// generate a path
	while (1)
	{
#if ASTAR_USE_HEAP
		node_index = ASiHeap_PopFirstElement(&ioAstar->open_heap);
		if (node_index == (UUtUns16) -1) {
			// no further nodes in the open heap
			break;
		}
		UUmAssert((node_index >= 0) && (node_index < ioAstar->num_allocs));
		path = &ioAstar->node_array[node_index];
		UUmAssert((path->flags & AScStatusMask) == AScStatusFlag_Open);
#else
		if (ASiList_IsEmpty(ioAstar->open_list)) {
			// no further nodes on the open list
			break;
		}

		// get the lowest cost node out of the open list
		path = ASiList_RemoveFirstNode(&ioAstar->open_list);
#endif
		
		// try all of the grid locations around the current node, see if they
		// are possible, and calculate the location and cost if they are.
		for (i = 0; i < 8; i++) {
			path_available[i] = ASiPassable(ioAstar, path, i, inAllowDiagonals, &new_loc[i], &new_cost[i], &new_flags[i]);
		}

		// loop over all diagonal directions
		for (i = 1; i < 8; i += 2) {
			if (!path_available[i])
				continue;

			// this is a diagonal path that is marked as available. this
			// path is NOT available if both of the orthogonal directions that
			// border it are unavailable... this stops us from taking diagonal
			// shortcuts through diagonally rasterized walls, but lets us form
			// diagonal paths across clear areas.
			UUmAssert((ASgOrthogonalDirs[i][0] >= 0) && (ASgOrthogonalDirs[i][0] < 8) &&
					  (ASgOrthogonalDirs[i][1] >= 0) && (ASgOrthogonalDirs[i][1] < 8));
			if (!path_available[ASgOrthogonalDirs[i][0]] && !path_available[ASgOrthogonalDirs[i][1]]) {
				path_available[i] = UUcFalse;
			}
		}

		// we alternate between a number of different direction ordering methods so that
		// we don't insert the children in the same order each time we consider a node.
		// CB: not really sure why this is necessary, it's ordered by weight anyway... ?
		ioAstar->dir_index = (ioAstar->dir_index + 1) % AScNumDirLists;
		
		// insert all children of the current node
		for (i = 0; i < 8; i++)
		{
			dir = ASgDir[ioAstar->dir_index][i];

			if (!path_available[dir])
				continue;

			// the path is not available if the cost is greater than the current cost
			location_index = new_loc[dir].x + (new_loc[dir].y * (UUtUns16) ioAstar->room->gridX);
			node_index = ioAstar->index_array[location_index];
			if (node_index == (UUtUns16) -1) {
				// this node hasn't been allocated yet, it is not part of a path.
				// we will allocate it right now and set it up.
				if (ioAstar->num_allocs >= AScMaxNodes)
				{
					// allocated all that we can
					UUrDebuggerMessage("ASrPath_Generate: overflowed AScMaxNodes (%d) trying to pathfind [%d,%d] - [%d,%d] on BNV %d grid %dx%d. ABORTING.\n",
									AScMaxNodes, ioAstar->start.x, ioAstar->start.y, ioAstar->end.x, ioAstar->end.y,
									ioAstar->roomnode->location->index, ioAstar->room->gridX, ioAstar->room->gridY);

#if DEBUG_VERBOSE_ASTAR
					COrConsole_Printf_Color(UUcTrue, 0xFFFF0000, 0xFF800000, "%s: A* on %d [%dx%d]: %d nodes examined, OVERFLOW, FAIL",
										inWalker->player_name, ioAstar->roomnode->location->index,
										ioAstar->room->gridX, ioAstar->room->gridY, ioAstar->num_allocs);
#endif
					result = UUcFalse;
					goto exit;
				}

				// allocate the node and set up its entry in the cost array
				node_index = ioAstar->num_allocs++;
				node = &ioAstar->node_array[node_index];	

				ioAstar->index_array[location_index] = node_index;
				ASrIndexArray_Use(ioAstar, location_index);

				// set up the node
#if ASTAR_USE_HEAP
				node->heapindex = (UUtUns16) -1;
#else
				node->prev = NULL;
				node->next = NULL;
#endif
				newly_allocated_node = UUcTrue;
			} else {
				// this node has been visited before by another path.

#if NO_REVISITING_NODES
				// discard the potential new path to the node. this generates a worse result, but it is faster.
				continue;
#endif

				// get a pointer to the node
				node = &ioAstar->node_array[node_index];				
				if (new_cost[dir] >= node->g) {
					// the new path is longer than the existing path, don't use it
					continue;
				}

#if !ASTAR_USE_HEAP
				// we have a shorter path; remove the node from the list that it currently
				// exists on, and reuse it.
				if ((node->flags & AScStatusMask) == AScStatusFlag_Open) {
					ASiList_RemoveNode_Fast(&ioAstar->open_list, node);
				}
#endif

				newly_allocated_node = UUcFalse;
			}

			// set the new properties of the node
			if (newly_allocated_node) {
				// set the node's location - IMPORTANT: this must come before ASiDistEstimate
				node->loc		= new_loc[dir];
			} else {
				// we are reusing a node, it must be at the correct location already
				UUmAssert((node->loc.x == new_loc[dir].x) && (node->loc.y == new_loc[dir].y));
			}
			node->adjacent		= path - ioAstar->node_array;
			node->g				= new_cost[dir];
			node->f				= node->g + ASiDistEstimate(node, &ioAstar->reachable_end);
			node->flags			= new_flags[dir];

			if (newly_allocated_node) {
				// check to see if we found the destination
				if ((node->loc.x == ioAstar->reachable_end.x) && (node->loc.y == ioAstar->reachable_end.y))
				{
					// node is now the last node in the found path
					ioAstar->path_end = node;
					ioAstar->finished = UUcTrue;
					
					// calculate the generated path
					ASiList_CalcWaypoints(
						ioAstar,
						inMaxWaypoints,
						outNumWaypoints,
						outWaypoints,
						inCarefulPath);

#if TOOL_VERSION
					if (ioAstar->show_debug) {
						ASiBuildDebugPath(ioAstar,
										  *outNumWaypoints,
										  outWaypoints);
					}
#endif

#if DEBUG_VERBOSE_ASTAR
					COrConsole_Printf_Color(UUcTrue, 0xFFFF0000, 0xFF800000, "%s: A* on %d [%dx%d]: %d nodes examined, SUCCESS",
										inWalker->player_name, ioAstar->roomnode->location->index,
										ioAstar->room->gridX, ioAstar->room->gridY, ioAstar->num_allocs);
#endif
					result = UUcTrue;
					goto exit;
				}
			}

			UUmAssert(UUcError_None == iDetectLoop(ioAstar->num_allocs, ioAstar->node_array, node));
		
#if ASTAR_USE_HEAP
			if (newly_allocated_node) {
				ASiHeap_Insert(&ioAstar->open_heap, (UUtUns16) node_index);
			} else {
				ASiHeap_ReorderElement(&ioAstar->open_heap, node->heapindex);
			}
#else
			// put the node into the open list
			ASiList_InsertNode(&ioAstar->open_list, node);
#endif
			node->flags |= AScStatusFlag_Open;
		}

		// put path onto the closed list
		path->flags &= ~AScStatusMask;
		path->flags |= AScStatusFlag_Closed;
	}

#if DEBUG_VERBOSE_ASTAR
	COrConsole_Printf_Color(UUcTrue, 0xFFFF0000, 0xFF800000, "%s: A* on %d [%dx%d]: %d nodes examined, NO PATH, FAIL",
						inWalker->player_name, ioAstar->roomnode->location->index,
						ioAstar->room->gridX, ioAstar->room->gridY, ioAstar->num_allocs);
#endif
	result = UUcFalse;

exit:

#if TOOL_VERSION
	if (ioAstar->show_evaluation) {
		ASiBuildDebugGridEvaluation(ioAstar);
	}
#endif

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(ASgPath_Generate_Timer);
#endif
	
	return result;
}

// ----------------------------------------------------------------------
UUtBool
ASrPath_SetParams(
	AStPath					*ioAstar,
	PHtNode					*inNode,
	const PHtRoomData		*inRoom,
	const M3tPoint3D		*inStart,
	const M3tPoint3D		*inEnd,
	ONtCharacter			*inCharacter,
	UUtBool					inAllowStairs,
	UUtBool					inShowDebug,
	UUtBool					inShowEvaluation)
{
	IMtPoint2D start;
	IMtPoint2D end;

	UUmAssert(ioAstar && inRoom && inStart && inEnd);
	
	ioAstar->roomnode = inNode;
	ioAstar->room = inRoom;
	ioAstar->owner = inCharacter;
	ioAstar->worldStart = *inStart;
	ioAstar->worldEnd = *inEnd;

	PHrWorldToGridSpace(&start.x, &start.y, inStart, inRoom);
	PHrWorldToGridSpace(&end.x, &end.y, inEnd, inRoom);

	ioAstar->start.x = (UUtUns8) UUmPin(start.x, 0, PHcMaxGridSize - 1);
	ioAstar->start.y = (UUtUns8) UUmPin(start.y, 0, PHcMaxGridSize - 1);
	ioAstar->end.x = (UUtUns8) UUmPin(end.x, 0, PHcMaxGridSize - 1);
	ioAstar->end.y = (UUtUns8) UUmPin(end.y, 0, PHcMaxGridSize - 1);

	PHrPrepareRoomForPath(inNode,inRoom);

	// set the grid to 0xFFFF
	if (ioAstar->index_array_used_count > AScIndexArrayMaxUsedCount) {
		UUrMemory_Set8(ioAstar->index_array, 0xFF, sizeof(UUtUns16) * PHcMaxGridSize * PHcMaxGridSize);
	}
	else {
		UUtUns32 itr;
		UUtUns32 count = ioAstar->index_array_used_count;

		for(itr = 0; itr < count; itr++)
		{
			ioAstar->index_array[ioAstar->index_array_used[itr]] = (UUtUns16) -1;
		}
	}

	ioAstar->index_array_used_count = 0;
	
	// clear the allocated nodes from the node_array
	ioAstar->num_allocs	= 0;

	// no path has been found
	ioAstar->path_end = NULL;
	ioAstar->finished = UUcFalse;
	
	// the spheretree at the end point will need to be generated
	ioAstar->spheretree_set = UUcFalse;
	
	// don't allow the stair region unless we're trying to go there
	ioAstar->allow_stairs = inAllowStairs;

#if TOOL_VERSION
	ioAstar->show_debug = inShowDebug;
	ioAstar->show_evaluation = inShowEvaluation;
#endif

	// the params are set now
	ioAstar->params_set	= UUcTrue;

	return UUcTrue;
}


UUtError ASrGetWorldGridValue(
	M3tPoint3D *inPoint,
	ONtCharacter *inViewer,
	PHtSquare *outValue)
{
	/*****************
	* Returns whether there's a big X in the path grid
	* at 'inPoint'
	*/
	
	AKtBNVNode *node;
	PHtNode *path_node;
	UUtUns16 gx,gy;
	UUtUns32 width;
	PHtSquare *grid;
	
	node = AKrNodeFromPoint(inPoint);
	if (!node) return UUcError_Generic;

	if (NULL == node->roomData.compressed_grid) {
		outValue->weight = PHcClear;
		return UUcError_None;
	}
	
	path_node = PHrAkiraNodeToGraphNode(node, ONrGameState_GetGraph());
	if (path_node == NULL) {
		return UUcError_Generic;
	}

	PHrPrepareRoomForPath(path_node,&node->roomData);
	grid = ASgAstar_Path->roomnode->static_grid;
	if (grid == NULL) {
		outValue->weight = PHcClear;
	} else {
		width = ASgAstar_Path->roomnode->gridX;
		
		PHrWorldToGridSpace(&gx,&gy,inPoint,&node->roomData);
		*outValue = GetCell(gx,gy);
	}
	return UUcError_None;
}
					
// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ASiList_CalcWaypoints(
	AStPath					*inAstar,
	UUtUns16				inMaxWaypoints,
	UUtUns16				*outNumWaypoints,
	AStPathPoint			*outWaypoints,
	UUtBool					inCarefulPath)
{
	PHtSquare				*grid, square_val;
	AStPathNode				*current_node, *parent_node, *child_node;
	float					altitude;
	AStGridPoint			pointbuf[AI2cMax_PathPoints], cur_point;
	UUtUns32				itr, num_waypoints;
	UUtInt32				width, dummy_stop_x, dummy_stop_y;
	UUtUns16				prev_adjacent, next_adjacent;
	UUtUns8					square_weight, low_weight, high_weight;
	UUtBool					is_stairs, path_ok, escape_path;

	// these values used by GetCell macro
	grid = ASgAstar_Path->roomnode->static_grid;
	width = ASgAstar_Path->roomnode->gridX;
	
	num_waypoints = 1;
	pointbuf[0] = inAstar->start;

	/*
	 * first: reverse the path's adjacencies and find the start node
	 */

	current_node = inAstar->path_end;
	prev_adjacent = (UUtUns16) -1;
	next_adjacent = current_node->adjacent;
	
	while (next_adjacent != (UUtUns16) -1) {
		UUmAssert((next_adjacent >= 0) && (next_adjacent < inAstar->num_allocs));

		// set the current node to point down the path found
		current_node->adjacent = prev_adjacent;

		// move to the parent node of this node
		prev_adjacent = current_node - inAstar->node_array;
		current_node = &inAstar->node_array[next_adjacent];
		next_adjacent = current_node->adjacent;
	}
	current_node->adjacent = prev_adjacent;

	// current_node now points to the beginning node in the path
	inAstar->path_start = current_node;

	/*
	 * next: go through the nodes and generate waypoints
	 */

	// stair BNVs only generate a single path point at the end
	is_stairs = (((PHrFindRoomParent(inAstar->room, ONgGameState->level->environment))->flags &
					AKcBNV_Flag_Stairs_Standard) > 0);
	if (!is_stairs) {

		parent_node = NULL;
		while (current_node->adjacent != (UUtUns16) -1)
		{
			UUmAssert((current_node->adjacent >= 0) && (current_node->adjacent < inAstar->num_allocs));
			child_node = &inAstar->node_array[current_node->adjacent];

#if TOOL_VERSION
			if (inAstar->show_evaluation) {
				// TEMPORARY DEBUGGING - this node gets drawn in a different color
				current_node->flags &= ~AScStatusMask;
				current_node->flags |= AScStatusFlag_FinalPath;
			}
#endif

			if (num_waypoints >= AI2cMax_PathPoints - 2) {
				// we can't add any more waypoints to the path
				break;
			}

			// note the first node is purposely ignored, we use the start instead
			if (parent_node != NULL)
			{
				UUtInt16	diff_x1;
				UUtInt16	diff_y1;
				
				UUtInt16	diff_x2;
				UUtInt16	diff_y2;

				diff_x1 = parent_node->loc.x - current_node->loc.x;
				diff_y1 = parent_node->loc.y - current_node->loc.y;
				
				diff_x2 = current_node->loc.x - child_node->loc.x;
				diff_y2 = current_node->loc.y - child_node->loc.y;
				
				if ((diff_x1 != diff_x2) || (diff_y1 != diff_y2)) {
					pointbuf[num_waypoints++] = current_node->loc;
				}
			}
			
			// go to the next child in the list
			parent_node = current_node;
			current_node = child_node;
		}
	}
	
	if ((inAstar->reachable_end.x != inAstar->end.x) || (inAstar->reachable_end.y != inAstar->end.y)) {
		// add a waypoint for the reachable end (where we actually were able to pathfind to)
		UUmAssert(num_waypoints < AI2cMax_PathPoints);
		pointbuf[num_waypoints++] = inAstar->reachable_end;
	}

	// the last waypoint is the destination
	UUmAssert(num_waypoints < AI2cMax_PathPoints);
	pointbuf[num_waypoints++] = inAstar->end;

	/*
	 * second: check each waypoint to see whether it could be safely removed from the path
	 */

	*outNumWaypoints = 0;
	cur_point = inAstar->start;
	for (itr = 0; itr < num_waypoints; itr++) {
		if ((itr > 0) && (itr + 1 < num_waypoints)) {
			// check this point to see if we can add it
			if ((*outNumWaypoints + 1) >= inMaxWaypoints) {
				// we can't add this point to the path and still have room for the end point,
				// we must skip it.
				continue;
			}

			if (inCarefulPath) {
				// don't ever skip path points based on "clear space"
			} else {
				// check to see if we can move from cur_point safely to the next point past this one
				path_ok = PHrLocalPathWeight(inAstar->owner, PHcPathMode_DirectedMovement, inAstar->roomnode,
											(PHtRoomData *) inAstar->room, inAstar->obstruction_bv,
											PHcBorder2, 3, cur_point.x, cur_point.y, pointbuf[itr + 1].x, pointbuf[itr + 1].y,
											&escape_path, &low_weight, &high_weight, &dummy_stop_x, &dummy_stop_y);
				if ((path_ok) && (!escape_path) && (high_weight < PHcBorder2)) {
					continue;
				}
			}
		}

		// add this point to the path
		UUmAssert(*outNumWaypoints < inMaxWaypoints);
		if (itr < num_waypoints - 1) {
			// this is now the point that we check local-paths from
			cur_point = pointbuf[itr];

			// find its worldspace location
			altitude = inAstar->room->origin.y;
			PHrGridToWorldSpace(cur_point.x, cur_point.y, &altitude,
								&(outWaypoints[(*outNumWaypoints)].point), inAstar->room);
		} else {
			// this point is our destination
			outWaypoints[(*outNumWaypoints)].point = inAstar->worldEnd;
		}
		
		// get the weight at this point
		if (grid != NULL) {
			square_val = GetCell(pointbuf[itr].x, pointbuf[itr].y);
			square_weight = square_val.weight;
			UUmAssert((square_weight >= 0) && (square_weight < PHcMax));
		} else {
			square_weight = PHcClear;
		}

		// set the passability of this point
		outWaypoints[(*outNumWaypoints)].type = ASgPassabilityType[square_weight];
		(*outNumWaypoints) += 1;
	}
}

#if TOOL_VERSION
// ----------------------------------------------------------------------
static void
ASiBuildDebugPath(
	AStPath					*ioAstar,
	UUtUns16				inNumWaypoints,
	AStPathPoint			*inWaypoints)
{
	AStPathNode *node, *prev_node;
	UUtUns32 num_points, itr, index;
	AStDebugPathPoint *point;
	float altitude;

	if (!ASgDebugPath_Initialized) {
		ASgDebugPath_StartIndex = ASgDebugPath_NumUsed = ASgDebugPath_NumPaths = 0;
		UUrMemory_Clear(ASgDebugPath, AScDebugPath_MaxPoints * sizeof(AStDebugPathPoint));

		ASgDebugPath_Initialized = UUcTrue;
	}

	// calculate how many points we will need for the raw path
	num_points = 0;
	prev_node = NULL;
	node = ioAstar->path_start;

	while (node != NULL) {
		num_points++;

		if (node->adjacent == (UUtUns16) -1) {
			node = NULL;
		} else {
			UUmAssert((node->adjacent >= 0) && (node->adjacent < ioAstar->num_allocs));
			node = &ioAstar->node_array[node->adjacent];
		}
	};

	while (ASgDebugPath_NumUsed + num_points + 1 + inNumWaypoints + 1 > AScDebugPath_MaxPoints) {
		// there isn't enough room for us to just write into the empty space in the buffer
		if (ASgDebugPath_NumUsed == 0) {
			// we exceed the buffer's length and cannot store this path
			UUmAssert(!"ASiBuildDebugPath: path is too long for AScDebugPath_MaxPoints");
			return;
		}

		UUmAssert(ASgDebugPath[ASgDebugPath_StartIndex].valid);
		
		// advance the start index along the buffer until we reach the end of the first path
		UUmAssert(ASgDebugPath_NumPaths > 0);
		ASgDebugPath_NumPaths--;
		while ((ASgDebugPath[ASgDebugPath_StartIndex].valid) && (ASgDebugPath_NumUsed > 0)) {
			ASgDebugPath_StartIndex = (ASgDebugPath_StartIndex + 1) % AScDebugPath_MaxPoints;
			ASgDebugPath_NumUsed--;
		}

		if (ASgDebugPath_NumUsed > 0) {
			// we hit the end of the first path
			ASgDebugPath_StartIndex = (ASgDebugPath_StartIndex + 1) % AScDebugPath_MaxPoints;
			ASgDebugPath_NumUsed--;

			if (ASgDebugPath_NumUsed > 0) {
				// we should be at the start of the next path now
				UUmAssert(ASgDebugPath[ASgDebugPath_StartIndex].valid);
			}
		}

		if (ASgDebugPath_NumUsed == 0) {
			// we have had to delete the entire buffer
			ASgDebugPath_StartIndex = 0;
			ASgDebugPath_NumPaths = 0;
			UUrMemory_Clear(ASgDebugPath, AScDebugPath_MaxPoints * sizeof(AStDebugPathPoint));
		}
	}

	// store the points along the raw A* path in the debugpath buffer
	index = (ASgDebugPath_StartIndex + ASgDebugPath_NumUsed) % AScDebugPath_MaxPoints;
	node = ioAstar->path_start;
	for (itr = 0; itr < num_points; itr++) {
		point = &ASgDebugPath[index];
		point->valid = UUcTrue;

		altitude = ioAstar->room->origin.y;
		PHrGridToWorldSpace(node->loc.x, node->loc.y, &altitude, &(point->point), ioAstar->room);

		UUmAssert((node->adjacent >= 0) && (node->adjacent < ioAstar->num_allocs));
		node = &ioAstar->node_array[node->adjacent];

		index = (index + 1) % AScDebugPath_MaxPoints;
		ASgDebugPath_NumUsed++;
	}

	// write a terminating node into the buffer
	point->valid = UUcFalse;
	ASgDebugPath_NumUsed++;
	ASgDebugPath_NumPaths++;

	// store the points along the processed waypoint path in the debugpath buffer
	for (itr = 0; itr < inNumWaypoints; itr++) {
		point = &ASgDebugPath[index];
		point->valid = UUcTrue;
		point->point = inWaypoints[itr].point;

		index = (index + 1) % AScDebugPath_MaxPoints;
		ASgDebugPath_NumUsed++;
	}

	// write a terminating node into the buffer
	point->valid = UUcFalse;
	ASgDebugPath_NumUsed++;
	ASgDebugPath_NumPaths++;

	UUmAssert(ASgDebugPath_NumUsed < AScDebugPath_MaxPoints);
}

// ----------------------------------------------------------------------
static void
ASiBuildDebugGridEvaluation(
	AStPath					*ioAstar)
{
	AStDebugGridPoint *point;
	AStPathNode *node;
	float altitude;
	UUtUns32 itr;

	altitude = ioAstar->room->origin.y;

	ASgDebugGrid_NumPoints = 0;
	for (itr = 0, node = ioAstar->node_array; itr < (UUtUns32) ioAstar->num_allocs; itr++, node++) {
		point = &ASgDebugGridPoints[ASgDebugGrid_NumPoints++];

		PHrGridToWorldSpace(node->loc.x, node->loc.y, &altitude, &(point->point), ioAstar->room);
		switch(node->flags & AScStatusMask) {
			case AScStatusFlag_Open:
				point->shade = IMcShade_Pink;
			break;

			case AScStatusFlag_Closed:
				point->shade = IMcShade_Purple;
			break;

			case AScStatusFlag_FinalPath:
				point->shade = IMcShade_Yellow;
			break;

			default:
				point->shade = IMcShade_Green;
			break;
		}
	}
}

// ----------------------------------------------------------------------
void
ASrDisplayDebuggingInfo(
	void)
{
	if (ASgDebugPath_Initialized && (ASgDebugPath_NumUsed > 0)) {
		UUtBool first_point, is_processed;
		UUtUns32 points_remaining, path_itr, index, component;
		AStDebugPathPoint *point;
		M3tPoint3D p0, p1, last_point, first_unprocessed_point;
		IMtShade path_shade, point_shade;

		points_remaining = ASgDebugPath_NumUsed;
		path_itr = 0;
		index = ASgDebugPath_StartIndex;
		do {
			first_point = UUcTrue;
			is_processed = ((path_itr % 2) == 1);

			if (ASgDebugPath_NumPaths <= 1) {
				component = 255;
			} else {
				component = (path_itr * 255) / (ASgDebugPath_NumPaths - 1);
			}

			if (is_processed) {
				path_shade  = 0xFF800000 | ((128 + component/2) << 16) | ((128 + component/2) << 8);			// darkred -> orange
			} else {
				path_shade  = 0xFF000000 | (component << 16) | (128 + component/2);								// darkblue -> purple
			}
			point_shade = 0xFF808000 | ((128 + component/2) << 16) | ((128 + component/2) << 8) | component;	// darkyellow -> white
			do {
				point = &ASgDebugPath[index];
				if (!point->valid) {
					if ((!first_point) && (!is_processed)) {
						// draw a big O at the end point
						p0 = p1 = last_point;
						p0.y += 6.0f; p1.y += 6.0f;
						p0.x += 3.0f; p1.z += 3.0f;
						M3rGeom_Line_Light(&p0, &p1, point_shade);
						p0.x -= 6.0f;
						M3rGeom_Line_Light(&p0, &p1, point_shade);
						p1.z -= 6.0f;
						M3rGeom_Line_Light(&p0, &p1, point_shade);
						p0.x += 6.0f;
						M3rGeom_Line_Light(&p0, &p1, point_shade);
					}
					break;
				}

				if (is_processed) {
					// draw a small star at the current point
					p0 = p1 = point->point;
					p0.y += 3.0f; p1.y += 3.0f;
					p0.x += 2.0f; p1.x -= 2.0f;
					M3rGeom_Line_Light(&p0, &p1, path_shade);
					p0.x -= 2.0f; p1.x += 2.0f;
					p0.y += 2.0f; p1.y -= 2.0f;
					M3rGeom_Line_Light(&p0, &p1, path_shade);
					p0.y -= 2.0f; p1.y += 2.0f;
					p0.z += 2.0f; p1.z -= 2.0f;
					M3rGeom_Line_Light(&p0, &p1, path_shade);
				}

				if (first_point) {
					if (is_processed) {
						last_point = first_unprocessed_point;
						first_point = UUcFalse;
					} else {
						// draw a big X at the start point
						p0 = p1 = point->point;
						p0.y += 6.0f; p1.y += 6.0f;
						p0.x -= 3.0f; p0.z -= 3.0f;
						p1.x += 3.0f; p1.z += 3.0f;
						M3rGeom_Line_Light(&p0, &p1, point_shade);
						p0.x += 6.0f; p1.x -= 6.0f;
						M3rGeom_Line_Light(&p0, &p1, point_shade);

						first_unprocessed_point = point->point;
					}
				}
				if (!first_point) {
					// draw a line from the last point to the current point
					p0 = last_point; p1 = point->point;
					if (is_processed) {
						p0.y += 3.0f; p1.y += 3.0f;
					} else {
						p0.y += 6.0f; p1.y += 6.0f;
					}
					M3rGeom_Line_Light(&p0, &p1, path_shade);
				}

				first_point = UUcFalse;
				last_point = point->point;
				index = (index + 1) % AScDebugPath_MaxPoints;
				points_remaining--;
			} while (points_remaining > 0);

			while ((points_remaining > 0) && (!point->valid)) {
				// advance over the invalid point to the start of
				// the next path
				index = (index + 1) % AScDebugPath_MaxPoints;
				point = &ASgDebugPath[index];
				points_remaining--;
			}
			path_itr++;
		} while (points_remaining > 0);
	}

	if (ASgDebugGrid_NumPoints > 0) {
		UUtUns32 itr;
		M3tPoint3D p0, p1;
		AStDebugGridPoint *point;

		for (itr = 0, point = ASgDebugGridPoints; itr < ASgDebugGrid_NumPoints; itr++, point++) {
			p0 = p1 = point->point;

			p0.y += 3.0f; p1.y += 3.0f;
			p0.x -= 2.0f; p1.x += 2.0f;
			M3rGeom_Line_Light(&p0, &p1, point->shade);

			p0.x += 2.0f; p1.x -= 2.0f;
			p0.z -= 2.0f; p1.z += 2.0f;
			M3rGeom_Line_Light(&p0, &p1, point->shade);
		}
	}
}
#endif

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static float
ASiDistEstimate(
	 AStPathNode			*inNode,
	AStGridPoint			*inEnd)
{
	UUtInt16					diff_x;
	UUtInt16					diff_y;
#if 0
	if (0)	// max(dx, dy)
	{
		// estimate the distance from this point to the desired destination
		diff_x = fabs(inEnd->x - inNode->loc.x);
		diff_y = fabs(inEnd->y - inNode->loc.y);
		
		return (diff_x > diff_y ? diff_x : diff_y);
	}
	
	if (0)  // Euclidian
	{
		// estimate the distance from this point to the desired destination
		diff_x = inEnd->x - inNode->loc.x;
		diff_y = inEnd->y - inNode->loc.y;
		
		return (sqrt((diff_x * diff_x) + (diff_y * diff_y)));
	}
	
	if (1)  // Manhattan
	{
#endif
		// estimate the distance from this point to the desired destination
		diff_x = abs(inEnd->x - inNode->loc.x);
		diff_y = abs(inEnd->y - inNode->loc.y);
		
		return (float)(diff_x + diff_y);
#if 0
	}
#endif
}

// ----------------------------------------------------------------------
static UUtBool
ASiPassable(
	 AStPath				*inAstar,
	 AStPathNode			*inNode,
	UUtUns16					inDir,
	UUtBool						inAllowDiagonals,
	AStGridPoint				*outNewLoc,
	float						*outNewCost,
	UUtUns16					*outNewFlags)
{
	UUtUns8						x;
	UUtUns8			 			y;
	UUtBool						obstructed;
	float						grid_cost;
	UUtUns8						weight;
	UUtUns16					new_flags;
	AStPathNode					*parent;

	if (!inAllowDiagonals && (inDir & 0x01)) {
		// odd-numbered directions are diagonal, reject them
		return UUcFalse;
	}

	switch (inDir)
	{
		case 0:		// up
			x = inNode->loc.x;
			y = inNode->loc.y - 1;
		break;
		
		case 1:		// up & right
			x = inNode->loc.x + 1;
			y = inNode->loc.y - 1;
		break;
		
		case 2:		// right
			x = inNode->loc.x + 1;
			y = inNode->loc.y;
		break;
		
		case 3:		// dn & right
			x = inNode->loc.x + 1;
			y = inNode->loc.y + 1;
		break;
		
		case 4:		// dn
			x = inNode->loc.x;
			y = inNode->loc.y + 1;
		break;
		
		case 5:		// dn & left
			x = inNode->loc.x - 1;
			y = inNode->loc.y + 1;
		break;
		
		case 6:		// left
			x = inNode->loc.x - 1;
			y = inNode->loc.y;
		break;
		
		case 7:		// up & left
			x = inNode->loc.x - 1;
			y = inNode->loc.y - 1;
		break;
		
		default:
			UUmAssert(!"ASiPassable: unknown direction (not in 0..7)");
		break;
	}

	// make sure this isn't the parent
	if (inNode->adjacent != (UUtUns16) -1)
	{
		UUmAssert((inNode->adjacent >= 0) && (inNode->adjacent < inAstar->num_allocs));
		parent = &inAstar->node_array[inNode->adjacent];

		if ((parent->loc.x == x) && (parent->loc.y == y))
		{
			return UUcFalse;
		}
	}
	
	// set the new location
	outNewLoc->x = x;
	outNewLoc->y = y;
	
	// check to see that the square is passable for us
	new_flags = 0;
	if (!PHrSquareIsPassable(inAstar->owner, PHcPathMode_DirectedMovement,
							inAstar->roomnode, inAstar->room, 
							inAstar->obstruction_bv, x, y, &obstructed, &weight)) {
		new_flags |= AScStatusFlag_Impassable;
	}

	if (obstructed) {
		new_flags |= AScStatusFlag_Obstructed;
	}

	if (weight == PHcStairs) {
		new_flags |= AScStatusFlag_Stairs;
	}

	if (new_flags & AScStatusFlag_Impassable) {
		if ((inNode->flags & AScStatusFlag_Impassable) == 0) {
			// we can only walk to an impassable node if we were already on an impassable node
			return UUcFalse;
		} else {
			// don't walk too far along impassable nodes
			if (inNode->g >= AScMaxStartImpassableRadius) {
				return UUcFalse;
			}

			// don't walk from a mere obstruction into a REAL impassability
			if (((new_flags & AScStatusFlag_Obstructed) == 0) && (inNode->flags & AScStatusFlag_Obstructed)) {
				return UUcFalse;
			}
		}
	} else {
		if (inNode->flags & AScStatusFlag_Impassable) {
			// we are walking out of impassable space into clear space; check we're not trying to do it through a wall.
			if (ASiCollisionTest(inAstar, UUcTrue, outNewLoc)) {
				return UUcFalse;
			}
		}

		if ((new_flags & AScStatusFlag_Stairs) && ((inNode->flags & AScStatusFlag_Stairs) == 0) && (!inAstar->allow_stairs)) {
			// we are trying to walk from somewhere that is not in a stair region to somewhere
			// that is in a stair region, and our path does not allow us to go onto stairs.
			return UUcFalse;
		}
	}

	// calculate the grid cost
	UUmAssert((weight >= 0) && (weight < PHcMax));
	grid_cost = PHcPathWeight[weight];
	
	// diagonal or orthogonal?
	if ((inNode->loc.x != outNewLoc->x) && (inNode->loc.y != outNewLoc->y))
	{
		// movement is diagonal
		*outNewCost = (1.4f * grid_cost) + inNode->g;
	}
	else
	{
		// movement is orthogonal
		*outNewCost = (1.0f * grid_cost) + inNode->g;
	}
	
	*outNewFlags = new_flags;

	return UUcTrue;
}

const PHtRoomData *ASrGetCurrentRoom(void)
{
	/*********************
	* Returns current room
	*/
	
	return ASgAstar_Path->room;
}

void ASrGetLocalStart(IMtPoint2D *outPoint)
{
	// Returns local start grid square
	outPoint->x = ASgAstar_Path->start.x;
	outPoint->y = ASgAstar_Path->start.y;
}

void ASrGetLocalEnd(IMtPoint2D *outPoint)
{
	// Returns local end grid square
	outPoint->x = ASgAstar_Path->end.x;
	outPoint->y = ASgAstar_Path->end.y;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError iDetectLoop(UUtUns32 inNodeArraySize, AStPathNode *inNodeArray, AStPathNode *inNode)
{
	#define cMaxNodes 1000
	AStPathNode *nodeTable[cMaxNodes] = { NULL };
	AStPathNode *current_node = inNode;
	UUtUns16 depth;
	UUtUns16 itr;
	
	depth = 0;
	
	while(current_node != NULL)
	{
		nodeTable[depth] = current_node;
		
		if (current_node->adjacent == (UUtUns16) -1) {
			current_node = NULL;
		} else {
			UUmAssert((current_node->adjacent >= 0) && (current_node->adjacent < inNodeArraySize));
			current_node = &inNodeArray[current_node->adjacent];
		}

		for(itr = 0; itr < depth; itr++) {
			if (nodeTable[depth] == nodeTable[itr]) {
				return UUcError_Generic;
			}
		}

		depth++;

		if (cMaxNodes == depth) {
			break;
		}
	}

	return UUcError_None;
}

void ASrDisplayWorkingGrid(void)
{
	/**********
	* Displays 'grid' for debugging
	*/
	
	UUtInt32 c,d,width,height;
	char ch;
	PHtSquare *grid,cell;
	
	grid = ASgAstar_Path->roomnode->static_grid;

	if (grid == NULL)
		return;

	width = ASgAstar_Path->roomnode->gridX;
	height = ASgAstar_Path->roomnode->gridY;
	
	for (c=0; c<height; c++)
	{
		for (d=0; d<width; d++)
		{
			cell = GetCell(d,c);
			if ((cell.weight < 0) || (cell.weight >= PHcMax)) {
				ch = '?';
			} else {
				ch = PHcPrintChar[cell.weight];
			}
			
			// Override with start and end points, if appropriate
			if (c==ASgAstar_Path->start.x && d==ASgAstar_Path->start.y) ch = 'S';
			else if (c==ASgAstar_Path->end.x && d==ASgAstar_Path->end.y) ch = 'E';
			
			printf("%c", ch);
		}
		printf(""UUmNL);
	}
	
	getchar();
}

void ASrDebugGetGridPoints(AStPath *inPath, IMtPoint2D *outStart, IMtPoint2D *outEnd)
{
	UUmAssertReadPtr(inPath, sizeof(AStPath));

	outStart->x = inPath->start.x;
	outStart->y = inPath->start.y;
	outEnd->x = inPath->end.x;
	outEnd->y = inPath->end.y;
}


static UUtBool ASiCollisionTest(AStPath *inAstar, UUtBool inStartTest, AStGridPoint *inPoint)
{
	ONtActiveCharacter *active_character;
	PHtSphereTree *tree;
	UUtUns32 itr;
	M3tVector3D cur_pos, delta_pos, movement;

	PHrGridToWorldSpace(inPoint->x, inPoint->y, NULL, &cur_pos, inAstar->room);

	active_character = ONrGetActiveCharacter(inAstar->owner);
	if (active_character == NULL) {
		// no collisions
		return UUcFalse;
	}

	if (inStartTest) {
		// use owning character's spheretree - NB: this assumes that all A* paths are generated from a
		// character's current position!
		tree = active_character->sphereTree;

		movement.x = cur_pos.x - inAstar->worldStart.x;
		movement.y = 0;
		movement.z = cur_pos.z - inAstar->worldStart.z;
	} else {
		if (!inAstar->spheretree_built) {
			// build and link up a sphere-tree for the character. NB - this assumes that all characters
			// have the same number of subspheres in their sphere-tree
			PHrSphereTree_New(inAstar->sphereTree, NULL);

			for(itr = 0; itr < ONcSubSphereCount_Mid; itr++) {
				PHrSphereTree_New(inAstar->sphereTree_midnodes + itr, inAstar->sphereTree);
			}

			for(itr = 0; itr < ONcSubSphereCount; itr++) {
				PHrSphereTree_New(inAstar->sphereTree_leaves + itr, inAstar->sphereTree_midnodes + (itr / 2));
			}
			inAstar->spheretree_built = UUcTrue;
		}

		if (!inAstar->spheretree_set) {
			// copy the sphere-tree for the character, and set the position as if the character was
			// standing at the endpoint.
			MUmVector_Subtract(delta_pos, inAstar->worldEnd, inAstar->worldStart);

			inAstar->sphereTree[0].sphere = active_character->sphereTree[0].sphere;
			MUmVector_Add(inAstar->sphereTree[0].sphere.center, delta_pos, inAstar->sphereTree[0].sphere.center);

			for(itr = 0; itr < ONcSubSphereCount_Mid; itr++) {
				inAstar->sphereTree_midnodes[itr].sphere = active_character->midNodes[itr].sphere;
				MUmVector_Add(inAstar->sphereTree_midnodes[itr].sphere.center, delta_pos, inAstar->sphereTree_midnodes[itr].sphere.center);
			}

			for(itr = 0; itr < ONcSubSphereCount; itr++) {
				inAstar->sphereTree_leaves[itr].sphere = active_character->leafNodes[itr].sphere;
				MUmVector_Add(inAstar->sphereTree_leaves[itr].sphere.center, delta_pos, inAstar->sphereTree_leaves[itr].sphere.center);
			}
			inAstar->spheretree_set = UUcTrue;
		}

		tree = inAstar->sphereTree;

		movement.x = cur_pos.x - inAstar->worldEnd.x;
		movement.y = 0;
		movement.z = cur_pos.z - inAstar->worldEnd.z;
	}

	return AI2rManeuver_TrySphereTreeCollision(tree, &movement);
}

// determine whereabouts in the grid we can actually get to that's near the destination
static UUtBool ASiDetermineEndPoint(AStPath *inAstar)
{
	UUtUns16 radius;
	UUtInt16 v0, v1, test_v;

	// check to see if the endpoint is in fact passable - if so, don't bother with this
	if (PHrSquareIsPassable(inAstar->owner, PHcPathMode_DirectedMovement, 
							inAstar->roomnode, inAstar->room, inAstar->obstruction_bv,
							inAstar->end.x, inAstar->end.y, NULL, NULL)) {
		inAstar->reachable_end = inAstar->end;
		return UUcTrue;
	}

	for (radius = 1; radius <= AScMaxDestImpassableRadius; radius++) {
		// try + and -x
		v0 = inAstar->end.x - radius; 
		v1 = inAstar->end.x + radius;
		for (test_v = inAstar->end.y - radius; test_v <= inAstar->end.y + radius; test_v++) {
			if (ASiTryEndPoint(inAstar, v0, test_v)) {
				return UUcTrue;
			}
			
			if (ASiTryEndPoint(inAstar, v1, test_v)) {
				return UUcTrue;
			}
		}

		// try + and -y
		v0 = inAstar->end.y - radius; 
		v1 = inAstar->end.y + radius;
		for (test_v = inAstar->end.x - (radius - 1); test_v <= inAstar->end.x + (radius - 1); test_v++) {
			if (ASiTryEndPoint(inAstar, test_v, v0)) {
				return UUcTrue;
			}

			if (ASiTryEndPoint(inAstar, test_v, v1)) {
				return UUcTrue;
			}
		}
	}

	return UUcFalse;
}

static UUtBool ASiTryEndPoint(AStPath *inAstar, UUtInt16 inX, UUtInt16 inY)
{
	AStGridPoint test_point;

	// must stay on the grid
	if (((inX < 0) || (inX >= (UUtInt16)inAstar->room->gridX)) ||
		((inY < 0) || (inY >= (UUtInt16)inAstar->room->gridY))) {
		return UUcFalse;
	}

	// can only go to passable squares
	if (!PHrSquareIsPassable(inAstar->owner, PHcPathMode_DirectedMovement, 
							inAstar->roomnode, inAstar->room, 
							inAstar->obstruction_bv, inX, inY, NULL, NULL)) {
		return UUcFalse;
	}

	test_point.x = (UUtUns8) inX;
	test_point.y = (UUtUns8) inY;

	// can only go to squares from which the end point is reachable
	if (ASiCollisionTest(inAstar, UUcFalse, &test_point)) {
		return UUcFalse;
	}

	inAstar->reachable_end = test_point;

	return UUcTrue;
}

#if ASTAR_USE_HEAP
// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ASiHeap_BubbleUp(
	AStHeap					*ioHeap,
	UUtUns16				inHeapLocation)
{
	UUtUns16 element, parent_element, parent_location;

	UUmAssert((inHeapLocation >= 1) && (inHeapLocation < ioHeap->heap_count));

	element = ioHeap->heap[inHeapLocation];
	while (inHeapLocation > 1) {
		parent_location = inHeapLocation >> 1;
		parent_element = ioHeap->heap[parent_location];
		if (ioHeap->func_compare(ioHeap->heap_userdata, element, parent_element) <= 0) {
			// this element is a lower (or equal) priority to its parent, don't swap them
			break;
		}

		// swap the parent down to this element's location
		ioHeap->heap[inHeapLocation] = parent_element;
		ioHeap->func_notify(ioHeap->heap_userdata, parent_element, inHeapLocation);

		// keep looking further up the heap
		inHeapLocation = parent_location;
	}

	// this element has found its position
	ioHeap->heap[inHeapLocation] = element;
	ioHeap->func_notify(ioHeap->heap_userdata, element, inHeapLocation);
}

static void
ASiHeap_BubbleDown(
	AStHeap					*ioHeap,
	UUtUns16				inHeapLocation)
{
	UUtUns16 element, child_element, otherchild_element;
	UUtUns16 child_location, otherchild_location;

	UUmAssert((inHeapLocation >= 1) && (inHeapLocation < ioHeap->heap_count));

	element = ioHeap->heap[inHeapLocation];
	while (1) {
		child_location = inHeapLocation << 1;
		if (child_location >= ioHeap->heap_count) {
			// this element has no children, it cannot bubble any further down the heap
			break;
		}
		child_element = ioHeap->heap[child_location];

		otherchild_location = child_location + 1;
		if (otherchild_location < ioHeap->heap_count) {
			// this element has two children, we must compare it to the greater of the two
			otherchild_element = ioHeap->heap[otherchild_location];
			if (ioHeap->func_compare(ioHeap->heap_userdata, otherchild_element, child_element) > 0) {
				// the other child has a higher priority, use it instead
				child_element = otherchild_element;
				child_location = otherchild_location;
			}
		}

		if (ioHeap->func_compare(ioHeap->heap_userdata, element, child_element) >= 0) {
			// this element is of a higher (or equal) priority to its child, don't swap them
			break;
		}

		// swap the element down to its child's location
		ioHeap->heap[inHeapLocation] = child_element;
		ioHeap->func_notify(ioHeap->heap_userdata, child_element, inHeapLocation);

		// keep looking further down the heap
		inHeapLocation = child_location;
	}

	// this element has found its position
	ioHeap->heap[inHeapLocation] = element;
	ioHeap->func_notify(ioHeap->heap_userdata, element, inHeapLocation);
}

// ----------------------------------------------------------------------
static UUtUns16
ASiHeap_PopFirstElement(
	AStHeap					*ioHeap)
{
	if (ioHeap->heap_count > 1) {
		UUtUns16 first_element = ioHeap->heap[1];

		ioHeap->heap_count--;
		if (ioHeap->heap_count > 1) {
			// move the last element in the heap up to the top of the
			// heap, and then bubble it down to where it belongs
			ioHeap->heap[1] = ioHeap->heap[ioHeap->heap_count];
			ASiHeap_BubbleDown(ioHeap, 1);
		}

		return first_element;
	} else {
		// the heap is empty
		UUmAssert(ioHeap->heap_count == 1);
		return (UUtUns16) -1;
	}
}

static void
ASiHeap_Insert(
	AStHeap					*ioHeap,
	UUtUns16				inElement)
{
	if (ioHeap->heap_count >= ioHeap->heap_size) {
		UUmAssert(!"ASiHeap_Insert: heap overflow");
		return;
	} else {
		UUtUns16 new_location = (UUtUns16) ioHeap->heap_count++;

		ioHeap->heap[new_location] = inElement;
		ASiHeap_BubbleUp(ioHeap, new_location);
	}
}

static void
ASiHeap_ReorderElement(
	AStHeap					*ioHeap,
	UUtUns16				inHeapLocation)
{
	UUmAssert((inHeapLocation >= 1) && (inHeapLocation < ioHeap->heap_count));

	if (inHeapLocation > 1) {
		UUtUns16 parent_location;

		// check to see whether we need to bubble up
		parent_location = inHeapLocation >> 1;
		if (ioHeap->func_compare(ioHeap->heap_userdata, ioHeap->heap[inHeapLocation], 
									ioHeap->heap[parent_location]) > 0) {
			// this element is a higher priority than its parent
			ASiHeap_BubbleUp(ioHeap, inHeapLocation);
			return;
		}
	}

	// try bubbling this element down the heap
	ASiHeap_BubbleDown(ioHeap, inHeapLocation);
}

// ----------------------------------------------------------------------
static UUtInt32
ASiAstarHeap_CompareFunc(
	UUtUns32				inUserData,
	UUtUns16				inElementA,
	UUtUns16				inElementB)
{
	AStPath *astar = (AStPath *) inUserData;
	float fa, fb;

	UUmAssert((inElementA >= 0) && (inElementA < astar->num_allocs));
	UUmAssert((inElementB >= 0) && (inElementB < astar->num_allocs));
	fa = astar->node_array[inElementA].f;
	fb = astar->node_array[inElementB].f;

	if (fa < fb) {
		return +1;
	} else if (fa > fb) {
		return -1;
	} else {
		return 0;
	}
}

static void
ASiAstarHeap_NotifyLocation(
	UUtUns32				inUserData,
	UUtUns16				inElement, 
	UUtUns16				inHeapLocation)
{
	AStPath *astar = (AStPath *) inUserData;

	UUmAssert((inElement >= 0) && (inElement < astar->num_allocs));
	astar->node_array[inElement].heapindex = inHeapLocation;
}

#else
// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ASiList_InsertNode(
	 AStPathNode				**inList,
	 AStPathNode				*inNode)
{
	if (*inList == NULL)
	{
		// insert at the begining of the list
		inNode->prev = NULL;
		inNode->next = NULL;
		*inList = inNode;
	}
	else
	{
		 AStPathNode			*temp;
		 AStPathNode			*prev;

		temp = *inList;
		prev = NULL;
		
		while (temp)
		{
			if (temp->f >= inNode->f)
			{
				// insert the node before temp
				inNode->next	= temp;
				inNode->prev	= temp->prev;
				temp->prev		= inNode;
				
				if (inNode->prev)
				{
					// set the previous node's next to point to inNode
					inNode->prev->next = inNode;
				}
				else
				{
					// insert before the first node in the list
					*inList = inNode;
				}
				break;
			}
			else
			{
				// move to next node
				prev = temp;
				temp = temp->next;
			}
		}
		
		if (temp == NULL)
		{
			// insert at the end of the list
			if (prev)
			{
				// there are nodes before this one
				inNode->next	= NULL;
				inNode->prev	= prev;
				prev->next		= inNode;
			}
			else
			{
				// this case should be handled by the first if statement
				// this one is at the beginning of the list
				inNode->next	= NULL;
				inNode->prev	= NULL;
				*inList			= inNode;
			}
		}
	}
}

// ----------------------------------------------------------------------
static UUtBool
ASiList_IsEmpty(
	 AStPathNode			*inList)
{
	if (inList == NULL)
		return UUcTrue;
	else
		return UUcFalse;
}

// ----------------------------------------------------------------------
static  AStPathNode*
ASiList_RemoveFirstNode(
	 AStPathNode				**inList)
{
	 AStPathNode				*return_node;
	
	if (*inList)
	{
		// save the first node in the list
		return_node = *inList;
		
		// point list to next node in the list
		*inList = (*inList)->next;
		
		// update the prev pointer for the new first node in the list
		if (*inList)
		{
			(*inList)->prev = NULL;
		}
		
		// the return_node doesn't point to anything anymore
		return_node->next = NULL;
	}
	else
	{
		return_node = NULL;
	}
	
	// return the first node in the list
	return return_node;
}

// ----------------------------------------------------------------------
static void
ASiList_RemoveNode_Fast(
	 AStPathNode			**inList,
	 AStPathNode			*inNode)
{
	// remove this node from the list
	
	// update the next pointer of the prev node in the list
	if (inNode->prev)
	{
		inNode->prev->next = inNode->next;
	}
	else
	{
		// this is the start of the list
		*inList = inNode->next;
	}
	
	// update the prev pointer of the next node in the list
	if (inNode->next)
	{
		inNode->next->prev = inNode->prev;
	}
	
	// inNode no longer points to anything
	inNode->prev = inNode->next = NULL;
}
#endif
