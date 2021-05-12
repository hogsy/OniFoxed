/*
	Oni_Path.c
	
	This file contains all enemy pathfinding related code
	
	Author: Quinn Dunki
	c1998 Bungie
*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Akira.h"
#include "BFW_Console.h"
#include "BFW_Motoko.h"
#include "BFW_Mathlib.h"
#include "BFW_AppUtilities.h"
#include "BFW_Timer.h"

#include "Oni_Character.h"
#include "ONI_GameStatePrivate.h"
#include "Oni.h"
#include "Oni_Path.h"
#include "Oni_Camera.h"
#include "Oni_AStar.h"
#include "Oni_AI.h"

#include "lrar_cache.h"
#include "bfw_cseries.h"


/******************* Globals ****************************************/
#define PHcGhostQuadTouchDist				0.1
#define PHcConnectionDualDist				0.1
#define PHcMaxBlockages						16
#define PHcObstructionSize					(1.0f * PHcSquareSize)
#define PHcPathfindingObstructionMaxColors	4
#define PHcCheckBlockageTolerance			1e-03f

UUtInt32 PHgGridVisible;		// Flag for turning the grid on and off
UUtBool PHgRenderDynamicGrid = UUcFalse;

#if TOOL_VERSION
// CB: debugging! shows connections among BNVs in the environment
UUtUns32		PHgDebugGraph_NumConnections = 0, PHgDebugGraph_MaxConnections = 0, PHgDebugGraph_DistanceLimit = 0;
PHtConnection ** PHgDebugGraph_Connections;
PHtConnection *PHgDebugConnection;
#endif

// coordinates for highlighting the current debugging square
UUtUns16 PHgDebugSquareX = (UUtUns16) -1, PHgDebugSquareY = (UUtUns16) -1;

const IMtShade PHgPathfindingWeightColor[PHcMax]
	= {IMcShade_White, 0xFF80BF80, 0xFF8080BF, 0xFF5050CF, IMcShade_Green, 0xFF3030DF, IMcShade_Blue, 0xFF408040, IMcShade_Orange, IMcShade_Red};

const IMtShade PHgPathfindingObstructionColorTable[PHcPathfindingObstructionMaxColors]
	= {IMcShade_White, IMcShade_Yellow, IMcShade_LightBlue, 0xFFFF00FF};

/******************* Internal Prototypes ****************************/

static void PHiLRAR_NewProc(void *inUserData, short inBlockIndex);
static void PHiLRAR_PurgeProc(void *inUserData);
static UUtBool PHrBuildConnection(PHtGraph *ioGraph, AKtEnvironment *inEnvironment, AKtBNVNode *inBNVSrc, PHtNode *inNodeSrc,
							   UUtUns32 inSideIndex, UUtUns32 inAdjacencyIndex);
static PHtConnection *PHrAddConnection(PHtNode *inNodeSrc, PHtNode *inNodeDest, AKtGQ_General *gq, AKtEnvironment *inEnvironment,
									   UUtUns32 inPoint0Index, UUtBool inObstructed0, UUtUns32 inPoint1Index, UUtBool inObstructed1);
static void PHrRemoveConnection(PHtConnection *ioConnection, UUtBool inRemoveDependencies);
static UUtUns32 PHrFindBlockages(AKtEnvironment *inEnvironment, PHtConnection *inConnection,
								 UUtUns32 inMaxBlockages, float *inBlockageArray, UUtBool *outTotallyBlocked);
static void PHiPropagateShorterPath(PHtNode *inNode, float inDeltaWeight, UUtInt32 inDeltaPath);

/******************* Construction / Destruction *********************/
UUtError PHrBuildGraph(PHtGraph *ioGraph, AKtEnvironment *inEnv)
{
	/****************
	* Initializes the new graph 'ioGraph' based on 'inEnv'
	*/
	
	UUtError error;
	UUtBool totally_blocked;
	UUtUns16 block_start, block_end;
	UUtUns32 i,j,c,s,a,nodeCount,connCount, weight_index, num_blockages, *arrayptr;
	PHtNode *node;
	PHtConnection *conn, *conn2;
	AKtBNVNode_Side *side;
	float weight, blockages[PHcMaxBlockages * 2];
	
	if (!inEnv) return UUcError_Generic;
	//printf("Building pathfinding graph...");
	
	ioGraph->env = inEnv;
	
	// Allocate memory for the graph
	nodeCount = 0;
	for (c=0; c<inEnv->bnvNodeArray->numNodes; c++)
	{
		// Figure how many leaf nodes there are
		if (inEnv->bnvNodeArray->nodes[c].flags & AKcBNV_Flag_Room)
			nodeCount++;
	}

	ioGraph->numNodes = nodeCount;
	if (ioGraph->numNodes > 0) {
		ioGraph->nodes = UUrMemory_Block_New(nodeCount * sizeof(PHtNode));
		UUmError_ReturnOnNull(ioGraph->nodes);
	} else {
		ioGraph->nodes = NULL;
	}
	
	// initialize the lrar cache which is used to allocate the pathfinding grids
	ioGraph->grid_cache = lrar_new("pathfinding grid cache", 0, PHcGridCacheSize, (UUtUns16) nodeCount,
									2, NONE, PHiLRAR_NewProc, PHiLRAR_PurgeProc);
	ioGraph->grid_storage = UUrMemory_Block_New(PHcGridCacheSize);
	if (!ioGraph->grid_storage)
		return UUcError_OutOfMemory;

	// Create a graph node for each room
	nodeCount = 0;
	for (c=0; c<inEnv->bnvNodeArray->numNodes; c++)
	{
		if (inEnv->bnvNodeArray->nodes[c].flags & AKcBNV_Flag_Room)
		{
			PHrAddNode(ioGraph,&inEnv->bnvNodeArray->nodes[c],nodeCount);
			nodeCount++;
		}
	}
	
	// Create all the connections between nodes
	connCount = 0;
	for (c=0; c<ioGraph->numNodes; c++)
	{
		node = &ioGraph->nodes[c];
		
		// For each room, check all the sides
		for (s=node->location->sideStartIndex; s<node->location->sideEndIndex; s++)
		{
			side = &inEnv->bnvSideArray->sides[s];
			
			for (a=side->adjacencyStartIndex; a<side->adjacencyEndIndex; a++)
			{
				if (PHrBuildConnection(ioGraph, inEnv, node->location, node, s, a)) {
					connCount++;
				}
			}
		}
	}

	PHrVerifyGraph(ioGraph, UUcFalse);

	// determine how many weights we will need to store for the pathfinding graph
	ioGraph->num_weights = 0;
	for (c = 0, node = ioGraph->nodes; c < ioGraph->numNodes; c++, node++) {
		node->weight_baseindex = ioGraph->num_weights;
		ioGraph->num_weights += node->num_connections_in * node->num_connections_out;
	}
	if (ioGraph->num_weights > 0) {
		ioGraph->weight_storage = UUrMemory_Block_New(ioGraph->num_weights * sizeof(float));
		UUmError_ReturnOnNull(ioGraph->weight_storage);
	} else {
		ioGraph->weight_storage = NULL;
	}

	// build pathfinding weights
	for (c = 0, node = ioGraph->nodes; c < ioGraph->numNodes; c++, node++) {
		// set up all connection indices for this node
		for (i = 0, conn = node->connections_in; conn != NULL; conn = conn->next_in, i++) {
			conn->to_connectionindex = (UUtUns16) i;
		}
		for (i = 0, conn = node->connections_out; conn != NULL; conn = conn->next_out, i++) {
			conn->from_connectionindex = (UUtUns16) i;
		}

		// loop over all pairs of connections that cross this node
		for (i = 0, conn = node->connections_in; conn != NULL; conn = conn->next_in, i++) {
			for (j = 0, conn2 = node->connections_out; conn2 != NULL; conn2 = conn2->next_out, j++) {

				weight = MUmVector_GetDistance(conn->connection_midpoint, conn2->connection_midpoint);
				if (conn->from != conn2->to) {
					if (weight < 1e-03) {
						// the connections' midpoints are on top of one another!
						UUmAssert(conn->to == node);
						UUmAssert(conn2->from == node);
						UUrDebuggerMessage("PHrBuildGraph: detected superposed connections between BNVs %d -> %d and %d -> %d, removing; details follow:\n",
											conn->from->location->index, conn->to->location->index, 
											conn2->from->location->index, conn2->to->location->index);

						UUrDebuggerMessage("  FromBNV (%d): [%f,%f,%f]-[%f,%f,%f]\n", conn->from->location->index,
							conn->from->location->roomData.origin.x, conn->from->location->roomData.origin.y, conn->from->location->roomData.origin.z,
							conn->from->location->roomData.antiOrigin.x, conn->from->location->roomData.antiOrigin.y, conn->from->location->roomData.antiOrigin.z);
						UUrDebuggerMessage("  CurBNV (%d): [%f,%f,%f]-[%f,%f,%f]\n", node->location->index,
							node->location->roomData.origin.x, node->location->roomData.origin.y, node->location->roomData.origin.z,
							node->location->roomData.antiOrigin.x, node->location->roomData.antiOrigin.y, node->location->roomData.antiOrigin.z);
						UUrDebuggerMessage("  ToBNV (%d): [%f,%f,%f]-[%f,%f,%f]\n", conn2->to->location->index,
							conn2->to->location->roomData.origin.x, conn2->to->location->roomData.origin.y, conn2->to->location->roomData.origin.z,
							conn2->to->location->roomData.antiOrigin.x, conn2->to->location->roomData.antiOrigin.y, conn2->to->location->roomData.antiOrigin.z);
						UUrDebuggerMessage("  *** problem connection (widths %f, %f) lies at [%f,%f,%f]\n\n", conn->connection_width, conn2->connection_width,
							conn->connection_midpoint.x, conn->connection_midpoint.y, conn->connection_midpoint.z);

						// FIXME: do we want to do anything here? are zero weights still a problem?
					}
				}

				UUmAssert((i < node->num_connections_in) && (j < node->num_connections_out));
				weight_index = PHmNodeWeightIndex(node, conn->to_connectionindex, conn2->from_connectionindex);
				ioGraph->weight_storage[weight_index] = weight;
			}
			UUmAssert(j == node->num_connections_out);
		}
		UUmAssert(i == node->num_connections_in);
	}

	PHrVerifyGraph(ioGraph, UUcTrue);

#if defined(DEBUGGING) && DEBUGGING
	for (i = 0; i < ioGraph->num_weights; i++) {
		// check that there is no uninitialised memory
		UUmAssert(fabs(ioGraph->weight_storage[i]) < 20000.0);
	}
#endif

	// set up the blockages
	ioGraph->blockage_array = UUrMemory_Array_New(sizeof(UUtUns32), 10 * sizeof(UUtUns32), 0, 0);
	UUmError_ReturnOnNull(ioGraph->blockage_array);

	for (c = 0, node = ioGraph->nodes; c < ioGraph->numNodes; c++, node++) {
		for (conn = node->connections_out; conn != NULL; conn = conn->next_out) {
			// skip connections that have already had their blockages set up
			if (conn->num_blockages != (UUtUns16) -1)
				continue;

			num_blockages = PHrFindBlockages(inEnv, conn, PHcMaxBlockages, blockages, &totally_blocked);

			conn->num_blockages = (UUtUns16) num_blockages;
			conn->blockage_baseindex = (UUtUns16) UUrMemory_Array_GetUsedElems(ioGraph->blockage_array);
			if (totally_blocked) {
				conn->flags |= PHcConnectionFlag_Impassable;
			}

			error = UUrMemory_Array_SetUsedElems(ioGraph->blockage_array, conn->blockage_baseindex + num_blockages, NULL);
			UUmError_ReturnOnError(error);

			arrayptr = (UUtUns32 *) (UUrMemory_Array_GetMemory(ioGraph->blockage_array)) + conn->blockage_baseindex;
			for (i = 0; i < num_blockages; i++) {
				block_start = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(blockages[2*i + 0] * UUcMaxUns16);
				block_end	= (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(blockages[2*i + 1] * UUcMaxUns16);
				*arrayptr++ = (((UUtUns32) block_start) << 16) + block_end;
			}

			// find our reverse connection
			for (conn2 = node->connections_in; conn2 != NULL; conn2 = conn2->next_in) {
				if (conn2->gunk == conn->gunk) {
					break;
				}
			}

			if (conn2 != NULL) {
				// check that we've found the right connection
				if ((conn->from != conn2->to) || (conn->to != conn2->from)) {
					UUrDebuggerMessage("PHrBuildGraph: ghost quad %d [%f, %f, %f] used for multiple connections %d -> %d and %d -> %d!\n",
						conn->gunk - inEnv->gqGeneralArray->gqGeneral, conn->connection_midpoint.x, conn->connection_midpoint.y, conn->connection_midpoint.z,
						conn->from->location->index, conn->to->location->index,
						conn2->from->location->index, conn2->to->location->index);
				}

				if (conn2->num_blockages == (UUtUns16) -1) {
					// copy the blockage information
					conn2->num_blockages = conn->num_blockages;
					conn2->blockage_baseindex = conn->blockage_baseindex;
					if (totally_blocked) {
						conn2->flags |= PHcConnectionFlag_Impassable;
					}
				} else {
					UUrDebuggerMessage("PHrBuildGraph: connection through ghost quad %d already has blockage info!\n",
						conn->gunk - inEnv->gqGeneralArray->gqGeneral, conn->connection_midpoint.x, conn->connection_midpoint.y, conn->connection_midpoint.z);
				}
			}
		}
	}

	ioGraph->open = ioGraph->closed = NULL;
	//printf("done. %d nodes and %d connections."UUmNL,ioGraph->numNodes,connCount);
	//PHrDisplayGraph(ioGraph); getchar();
	
	return UUcError_None;
}

void PHrVerifyGraph(PHtGraph *ioGraph, UUtBool inCheckIndices)
{
	PHtNode *node;
	UUtUns32 c, i;
	PHtConnection *conn, *prevconn;

	for (c = 0, node = ioGraph->nodes; c < ioGraph->numNodes; c++, node++) {
		prevconn = NULL;
		for (i = 0, conn = node->connections_in; conn != NULL; conn = conn->next_in, i++) {
			UUmAssert(conn->prev_in == prevconn);
			UUmAssert(conn->to == node);
			UUmAssert(conn->from != node);
			UUmAssert((conn->from >= ioGraph->nodes) && (conn->from < ioGraph->nodes + ioGraph->numNodes));
			if (inCheckIndices) {
				UUmAssert(conn->to_connectionindex == i);
			}
			prevconn = conn;
		}
		UUmAssert(i == node->num_connections_in);

		prevconn = NULL;
		for (i = 0, conn = node->connections_out; conn != NULL; conn = conn->next_out, i++) {
			UUmAssert(conn->prev_out == prevconn);
			UUmAssert(conn->from == node);
			UUmAssert(conn->to != node);
			UUmAssert((conn->to >= ioGraph->nodes) && (conn->to < ioGraph->nodes + ioGraph->numNodes));
			if (inCheckIndices) {
				UUmAssert(conn->from_connectionindex == i);
			}
			prevconn = conn;
		}
		UUmAssert(i == node->num_connections_out);
	}
}

void PHrDisposeGraph(PHtGraph *ioGraph)
{
	/******************
	* Releases all the memory used by 'ioGraph'
	*/
	
	UUtUns16 c;
	PHtConnection *conn, *next;
	PHtNode *node;
	
	for (c = 0, node = ioGraph->nodes; c < ioGraph->numNodes; c++, node++)
	{
		// remove all outgoing connections
		for (conn = node->connections_out; conn != NULL; conn = next) {
			next = conn->next_out;

			PHrRemoveConnection(conn, UUcFalse);
			UUrMemory_Block_Delete(conn);
		}
		
		// remove all incoming connections
		for (conn = node->connections_in; conn != NULL; conn = next) {
			next = conn->next_in;

			PHrRemoveConnection(conn, UUcFalse);
			UUrMemory_Block_Delete(conn);
		}

		UUmAssert(node->num_connections_in == 0);
		UUmAssert(node->num_connections_out == 0);
		UUmAssert(node->connections_in == NULL);
		UUmAssert(node->connections_out == NULL);
	}

	if (ioGraph->nodes != NULL) {
		UUrMemory_Block_Delete(ioGraph->nodes);
		ioGraph->nodes = NULL;
	}
	
	if (ioGraph->weight_storage != NULL) {
		UUrMemory_Block_Delete(ioGraph->weight_storage);
		ioGraph->weight_storage = NULL;
	}

	if (ioGraph->grid_storage != NULL) {
		UUrMemory_Block_Delete(ioGraph->grid_storage);
		ioGraph->grid_storage = NULL;
	}

	if (ioGraph->grid_cache != NULL) {
		lrar_dispose(ioGraph->grid_cache);
		ioGraph->grid_cache = NULL;
	}

	if (ioGraph->blockage_array != NULL) {
		UUrMemory_Array_Delete(ioGraph->blockage_array);
		ioGraph->blockage_array = NULL;
	}
}

void PHrResetNode(PHtNode *ioNode)
{
	/*****************
	* Resets 'ioNode' to a default state
	*/
	
	ioNode->g = ioNode->h = ioNode->f = 0.0f;
	ioNode->weight_multiplier = 1.0f;
	ioNode->next = NULL;
	ioNode->predecessor = NULL;
	ioNode->predecessor_conn = NULL;
	ioNode->pathStatus = PHcPath_Undef;
	ioNode->pathDepth = (UUtUns16) -1;
}


/******************* Pathfinding ********************/

// generate a path through the BNV pathfinding graph - used for character pathfinding,
// sound propagation and distance estimation. uses an A* algorithm based on euclidean
// distance through midpoints of ghost quads... will **not** produce optimal output
// distances in the straight-line geometry situation if BNV boundaries are crossed
UUtBool PHrGeneratePath(
	PHtGraph *ioGraph,
	ONtCharacter *inCharacter,		// used for door-opening tests, can be NULL
	UUtUns32 inSoundType,			// (UUtUns32) -1 if not a sound test
	PHtNode *inStartNode,
	PHtNode *inEndNode,
	M3tPoint3D *inStartPoint,
	M3tPoint3D *inEndPoint,
	float *ioDistance,				// NULL for no distance calc, otherwise read = max distance, write = actual distance
	PHtConnection **outConnections,	// either NULL or a pointer to an array of pointers PHcMaxPathLength-1 long
	UUtUns32 *outNumNodes)
{	
	UUtUns16 index_in, index_out;
	UUtUns32 reset_itr, itr, weight_index;
	UUtInt32 delta_path;
	PHtConnection *con;
	float weight, new_f, new_g, new_h, delta_f, new_weight_multiplier;
	UUtBool passable;

	// prepare all nodes for the start of the search
	for (reset_itr = 0; reset_itr < ioGraph->numNodes; reset_itr++) {
		PHrResetNode(&ioGraph->nodes[reset_itr]);
	}

	ioGraph->open = NULL;
	ioGraph->closed = NULL;

	inStartNode->pathDepth = 0;
	inStartNode->predecessor = NULL;
	inStartNode->predecessor_conn = NULL;

	inStartNode->g = 0.0f;
	inStartNode->h = PHrEstimateDistance(inStartNode, inStartPoint, inEndNode, inEndPoint);
	inStartNode->f = inStartNode->g + inStartNode->h;
	inStartNode->weight_multiplier = 1.0f;

	if ((ioDistance != NULL) && (*ioDistance > 0)) {
		// check that we aren't so far away that no path can succeed
		if (inStartNode->f > *ioDistance) {
			return UUcFalse;
		}
	}

	PHrAppendToList(inStartNode, &ioGraph->open);
	inStartNode->pathStatus = PHcPath_Open;

	while (ioGraph->open)
	{
		PHtNode *cheap;
		PHtNode *check;

		cheap = PHrPopCheapest(&ioGraph->open);

		if (cheap == inEndNode) {
			if (outConnections != NULL) {
				// store the path that we found
				UUtUns32 numNodes = cheap->pathDepth + 1;
				PHtNode *node;

				UUmAssert(numNodes < PHcMaxPathLength);
				
				for (itr = numNodes - 1, node = cheap; itr > 0; itr--, node = node->predecessor) {
					UUmAssert(node != NULL);
					UUmAssert(node->predecessor_conn != NULL);

					outConnections[itr - 1] = node->predecessor_conn;
				}
				UUmAssertWritePtr(outNumNodes, sizeof(UUtUns32));
				*outNumNodes = numNodes;

				UUmAssert(outConnections[0]->from == inStartNode);
				UUmAssert(outConnections[numNodes - 2]->to == inEndNode);
			}

			if (ioDistance != NULL) {
				// store the distance of the path that we found
				*ioDistance = cheap->g;
				if (cheap->predecessor_conn == NULL) {
					// found a straight-line path
					*ioDistance = cheap->weight_multiplier * MUmVector_GetDistance(*inStartPoint, *inEndPoint);
				} else {
					*ioDistance = cheap->g + cheap->weight_multiplier
							* MUmVector_GetDistance(cheap->predecessor_conn->connection_midpoint, *inEndPoint);
				}
			}

			return UUcTrue;
		}
		
		for (itr = 0, con = cheap->connections_out; con != NULL; con = con->next_out, itr++) {
			if (con->to == cheap->predecessor) {
				// don't check the node we came in from
				continue;
			}

			if ((inCharacter != NULL) && (con->flags & PHcConnectionFlag_Impassable)) {
				// physical objects (i.e. characters) can't pass through this connection
				continue;
			}

			if (cheap->predecessor == NULL) {
				// straight-line distance to the connection
				weight = MUmVector_GetDistance(*inStartPoint, con->connection_midpoint);
			} else {
				UUmAssertReadPtr(cheap->predecessor_conn, sizeof(PHtConnection));

				index_in = cheap->predecessor_conn->to_connectionindex;
				UUmAssert((index_in >= 0) && (index_in < cheap->num_connections_in));

				index_out = con->from_connectionindex;
				UUmAssert((index_out >= 0) && (index_out < cheap->num_connections_out));

				weight_index = PHmNodeWeightIndex(cheap, index_in, index_out);

				UUmAssert((weight_index >= 0) && (weight_index < ioGraph->num_weights));
				weight = ioGraph->weight_storage[weight_index];
			}

			// sanity check
			UUmAssert((weight >= 0) && (weight < 20000.0f));

			new_weight_multiplier = 1.0f;
			if (con->door_link != NULL) {
				if (inCharacter != NULL) {
					// if we can't pass through this door, then skip this connection
					if (!OBJrDoor_CharacterCanOpen(inCharacter, con->door_link, con->door_side)) {
						continue;
					}
				} else {
					// get the class of this door
					OBJtOSD_Door *door = (OBJtOSD_Door *) con->door_link->object_data;

					if ((door->door_class != NULL) && (door->state != OBJcDoorState_Open)) {
						// can the sound travel through this door?
						switch (door->door_class->sound_allow) {
							case AI2cSoundAllow_All:
								passable = UUcTrue;
								break;

							case AI2cSoundAllow_Shouting:
								passable = ((inSoundType == AI2cContactType_Sound_Danger) || (inSoundType == AI2cContactType_Sound_Melee) ||
											(inSoundType == AI2cContactType_Sound_Gunshot));
								break;

							case AI2cSoundAllow_Gunfire:
								passable = (inSoundType == AI2cContactType_Sound_Gunshot);
								break;

							case AI2cSoundAllow_None:
								passable = UUcFalse;
								break;

							default:
								UUmAssert(!"PHrGeneratePath: unknown sound-allow type in door class");
								passable = UUcFalse;
								break;
						}
						if (!passable) {
							// sound cannot travel through this connection
							continue;
						}

						// apply the muffle fraction to the weight multiplier which increases the path distance both through
						// this connection and on any future connections in this path
						if (door->door_class->sound_muffle_fraction > 0.99f) {
							// sound cannot travel through this connection
							continue;

						} else if (door->door_class->sound_muffle_fraction > 0) {
							// sounds are muffled beyond this connection
							new_weight_multiplier = 1.0f / (1.0f - door->door_class->sound_muffle_fraction);
						}
					}
				}
			}

			// calculate how good this path is
			check = con->to;
			new_g = cheap->g + weight * cheap->weight_multiplier;
			new_h = cheap->weight_multiplier * PHrEstimateDistance(check, &con->connection_midpoint, inEndNode, inEndPoint);
			new_f = new_g + new_h;
			
			// we must have an admissible heuristic in order to be A*, where h >= sum(g) to goal, and
			// so f must never decrease by traversing a connection
/*			if (new_f < cheap->f - 0.01f) {
				UUmAssert(!"PHrGeneratePath: heuristic is non-admissible, A* cannot succeed");
			}*/

			if ((ioDistance != NULL) && (*ioDistance > 0)) {
				// check that we aren't exceeding the max distance required for path to succeed
				if (new_f > *ioDistance) {
					continue;
				}
			}

			if ((check->pathStatus == PHcPath_Undef) || (new_f < check->f))
			{
				delta_f = new_f - check->f;
				delta_path = (cheap->pathDepth + 1) - check->pathDepth;

				check->pathDepth = cheap->pathDepth + 1;
				check->predecessor = cheap;
				check->predecessor_conn = con;
				check->f = new_f;
				check->g = new_g;
				check->h = new_h;
				check->weight_multiplier = cheap->weight_multiplier * new_weight_multiplier;

				// we have found a better path to this node than previously existed
				if (check->pathStatus == PHcPath_Closed) {
					PHiPropagateShorterPath(check, delta_f, delta_path);
					PHrRemoveFromList(check,&ioGraph->closed);
				}

				if (check->pathStatus != PHcPath_Open) {
					PHrAppendToList(check,&ioGraph->open);
					check->pathStatus = PHcPath_Open;
				}

#if defined(DEBUGGING) && DEBUGGING
				{
					PHtNode *testnode;
					PHtConnection *testconn;

					// extended sanity checking
					for (testconn = check->connections_out; testconn != NULL; testconn = testconn->next_out) {
						UUmAssert(testconn->from == check);
						testnode = testconn->to;
						if ((testnode->pathStatus != PHcPath_Undef) && (testnode->predecessor == check)) {
							UUmAssert(testnode->pathDepth == check->pathDepth + 1);
						}
					}

					for (testnode = check; testnode->predecessor != NULL; testnode = testnode->predecessor) {
						UUmAssert(testnode->pathDepth > 0);
						UUmAssert(testnode->pathDepth == testnode->predecessor->pathDepth + 1);
						UUmAssert(testnode->predecessor_conn->from == testnode->predecessor);
						UUmAssert(testnode->predecessor_conn->to == testnode);
					}
					UUmAssert(testnode == inStartNode);
					UUmAssert(testnode->pathDepth == 0);
					UUmAssert(testnode->predecessor_conn == NULL);
				}
#endif
			}
		}
		UUmAssert(itr == cheap->num_connections_out);

		PHrAppendToList(cheap,&ioGraph->closed);
		cheap->pathStatus = PHcPath_Closed;
	}
	
	return UUcFalse;		
}

static void PHiPropagateShorterPath(PHtNode *inNode, float inDeltaWeight, UUtInt32 inDeltaPath)
{
	PHtConnection *connection;
	PHtNode *node;

	UUmAssert(inNode->pathStatus != PHcPath_Undef);
	UUmAssert(inDeltaWeight < 0);

	for (connection = inNode->connections_out; connection != NULL; connection = connection->next_out) {
		UUmAssert(connection->from == inNode);
		node = connection->to;

		// look for any nodes for which the closest path was through the node that has been changed
		if (node->pathStatus == PHcPath_Undef)
			continue;

		if (node->predecessor_conn != connection)
			continue;

		// apply the shorter path
		UUmAssert(node->predecessor == inNode);
		node->f += inDeltaWeight;
		node->g += inDeltaWeight;
		UUmAssert((node->f > node->g) && (node->g > 0));

		UUmAssert(node->pathDepth > -inDeltaPath);
		node->pathDepth = (UUtUns16) (node->pathDepth + inDeltaPath);
		UUmAssert(node->pathDepth == inNode->pathDepth + 1);

		PHiPropagateShorterPath(node, inDeltaWeight, inDeltaPath);
	}
}

AKtBNVNode *PHrFindRoomParent(const PHtRoomData *inRoom, const AKtEnvironment *inEnv)
{
	return &inEnv->bnvNodeArray->nodes[inRoom->parent];
}

float PHrEstimateDistance(PHtNode *inNodeA, M3tPoint3D *inPointA, PHtNode *inNodeB, M3tPoint3D *inPointB)
{
	return MUmVector_GetDistance(*inPointA, *inPointB);
}

PHtNode *PHrPopCheapest(PHtNode **inHead)
{
	/**************************
	* Finds the node in the list with the lowest Dijkstra F value,
	* removes it from the list, and returns it to the caller
	*/
	
	PHtNode *head = *inHead;
	PHtNode *lowN;
	float low = UUcFloat_Max;
	
	if (!head) return NULL;
	
	// Find cheapest
	do
	{
		if (head->f < low) {
			low = head->f;
			lowN = head;
		}

		head = head->next;
	} while(head != NULL);
	
	PHrRemoveFromList(lowN,inHead);
	
	return lowN;
}

static PHtConnection *PHiClosestConnection(PHtConnection **inConnectionList, UUtUns16 inConnectionCount, M3tPoint3D *inPoint)
{
	/************
	* Returns the closest connection from a list to a given point
	*/
	
	PHtConnection *closest_connection=NULL;
	PHtConnection *connection;
	float shortest_distance_squared = UUcFloat_Max;
	M3tPoint3D *p0, *p1;
	UUtUns32 i;
	AKtEnvironment *environment = ONrGameState_GetEnvironment();
	
	for (i=0; i < inConnectionCount; i++)
	{
		float distance_squared;

		connection = inConnectionList[i];

		p0 = PHmConnection_Point0(environment, connection);
		p1 = PHmConnection_Point1(environment, connection);
	
		// find the distance to closest point to this connection
		// this is actually kinda wrong since you might be near the middle
		distance_squared = UUmMin(MUrPoint_Distance_Squared(inPoint, p0), MUrPoint_Distance_Squared(inPoint, p1));

		if (distance_squared < shortest_distance_squared) {
			shortest_distance_squared = distance_squared;
			closest_connection = connection;
		}
	}
	
	UUmAssert(closest_connection != NULL);

	return closest_connection;
}


PHtConnection *PHrFindConnection(PHtNode *inA, PHtNode *inB, M3tPoint3D *inPointB)
{
	/************************
	* Returns the first available edge connection between A and B,
	* assuming we wish to get to pointB in B.
	*/
	
	#define PHcMaxConnections 32
	PHtConnection *conlist[PHcMaxConnections],*con = inA->connections_out;
	UUtUns16 concount = 0;
	
	while (con)
	{
		UUmAssert(con->from == inA);
		if (con->to == inB)
		{
			conlist[concount] = con;
			concount++;
			UUmAssert(concount<PHcMaxConnections);
		}
		con = con->next_out;
	}
	
	if (concount)
	{
		// Found a list of connections from A to B. Return best choice
		return PHiClosestConnection(conlist,concount,inPointB);
	}
	
	return NULL;
}



/******************* Utilities *********************/

PHtRoomData *PHrRoomFromPoint(AKtEnvironment *inEnv, M3tPoint3D *inPoint)
{
	/******************
	* Returns the room data containing 'inPoint'.
	*
	* IMPORTANT: A point can be in more than one room at once (even though
	* it can only exist in one node at a time). Therefore this routine may not
	* return what you expect. It returns the first room it finds containing your
	* point, which is not necessarily the only room that contains it.
	*/
	
	AKtBNVNode *node;
	PHtRoomData *room;
	
	node = AKrNodeFromPoint(inPoint);

	if (NULL == node) {
		room = NULL;
	}
	else {
		room = &node->roomData;
	}

	return room;
}

UUtBool PHrPointInRoom(AKtEnvironment *inEnv, PHtRoomData *inRoom, M3tPoint3D *inPoint)
{
	/***********************
	 * Returns true if the point is in the room.
	 * The "room" in this context is the union of the 2 spaces
	 * created by the volume node and the 3D extrusion of the room
	 * pathfinding grid.
	 */
	
	UUtInt16 gridX,gridY;
	M3tPoint3D checkPoint;
	AKtBNVNode *parent = PHrFindRoomParent(inRoom,inEnv);
	UUmAssertReadPtr(inRoom, sizeof(PHtRoomData));

	// CB: this assertion is a hindrance because I want to call it from the importer
	// before compressed_grid has actually been calculated
	
	PHrWorldToGridSpaceDangerous(&gridX,&gridY,inPoint,inRoom);
	
	if ((gridX >= 0) && ((UUtUns32) gridX < inRoom->gridX) &&
		(gridY >= 0) && ((UUtUns32) gridY < inRoom->gridY))
	{
		// For stairs, that's close enough
		if (parent->flags & AKcBNV_Flag_Stairs) return UUcTrue;
		
		// CB: this function is used only for debugging purposes, and in the
		// importer for determining which quads need to be rasterized into the
		// pathfinding grid. it's now pretty well obsolete so the use of
		// AIcBNVTestFudge is okay

		// X and Z are okay, now what about the Y?
		checkPoint = *inPoint;
		checkPoint.y += AIcBNVTestFudge;
		return AKrPointInNodeVertically(&checkPoint,inEnv,parent);
	}
	return UUcFalse;
}

void PHrAddCharacterToGrid(UUtUns32 inIndex, ONtCharacter *inCharacter, PHtDynamicSquare *inGrid,
						   UUtUns16 inGridX, UUtUns16 inGridY, PHtNode *inNode, const PHtRoomData *inRoom)
{
	/************************
	* Makes 'inCharacter's presence known in the grid
	* described by the inGrid parms. 'inGrid' is assumed
	* to be in the same coordinate space as 'inRoom'
	*/
	
	UUtUns8 obstruction;
	UUtInt16 gx,gy;
	UUtUns16 width = inGridX,height = inGridY;
	PHtDynamicSquare *grid = inGrid;

	if (&inCharacter->currentNode->roomData != inRoom) return;

	PHrWorldToGridSpaceDangerous(&gx,&gy,&inCharacter->location,inRoom);

	// reject character if they do not overlap the grid at all
	if ((gx < -2) || (gx >= width + 2) || (gy < -2) || (gy >= height + 2))
		return;

	// make an entry for this character in the obstruction table of the room
	// entry 0 holds the number of entries (since '0' in the grid means no
	// obstruction, we can't use entry 0 anyway)
	obstruction = (UUtUns8) inNode->obstruction[0];
	if (obstruction >= PHcMaxObstructions) {
		COrConsole_Printf("### PHrAddCharacterToGrid: too many obstructions in one BNV");
		return;
	}
	inNode->obstruction[obstruction] = (UUtUns16) ((inIndex & PHcObstructionIndexMask) | PHcObstructionType_Character);
	inNode->obstruction[0] += 1;

	/*		 aaa
			bEEEc
			bFFFc
			bGGGc
			 ddd
	*/

	PHrPutObstruction(grid,height,width,gx-1,gy-1,obstruction);	// E
	PHrPutObstruction(grid,height,width,gx,gy-1,obstruction);
	PHrPutObstruction(grid,height,width,gx+1,gy-1,obstruction);
	
	PHrPutObstruction(grid,height,width,gx-1,gy,obstruction);	// F
	PHrPutObstruction(grid,height,width,gx,gy,obstruction);
	PHrPutObstruction(grid,height,width,gx+1,gy,obstruction);
	
	PHrPutObstruction(grid,height,width,gx-1,gy+1,obstruction);	// F
	PHrPutObstruction(grid,height,width,gx,gy+1,obstruction);
	PHrPutObstruction(grid,height,width,gx+1,gy+1,obstruction);
	
	PHrPutObstruction(grid,height,width,gx-1,gy-2,obstruction);	// a
	PHrPutObstruction(grid,height,width,gx,gy-2,obstruction);
	PHrPutObstruction(grid,height,width,gx+1,gy-2,obstruction);
	
	PHrPutObstruction(grid,height,width,gx-2,gy-1,obstruction);	// b
	PHrPutObstruction(grid,height,width,gx-2,gy,obstruction);
	PHrPutObstruction(grid,height,width,gx-2,gy+1,obstruction);
	
	PHrPutObstruction(grid,height,width,gx+2,gy-1,obstruction);	// c
	PHrPutObstruction(grid,height,width,gx+2,gy,obstruction);
	PHrPutObstruction(grid,height,width,gx+2,gy+1,obstruction);
	
	PHrPutObstruction(grid,height,width,gx-1,gy+2,obstruction);	// d
	PHrPutObstruction(grid,height,width,gx,gy+2,obstruction);
	PHrPutObstruction(grid,height,width,gx+1,gy+2,obstruction);
}


void PHrAddObjectToGrid(UUtUns32 inIndex, OBtObject *inObject, PHtDynamicSquare *inGrid,
						UUtUns16 inGridX, UUtUns16 inGridY, PHtNode *inNode, const PHtRoomData *inRoom)
{
	/************************
	* Makes 'inObject's presence known in the grid
	* described by the inGrid parms. 'inGrid' is assumed
	* to be in the same coordinate space as 'inRoom'
	*
	* Rasterizes a bounding box into the grid
	*/

	UUtInt32 radius;
	UUtInt32 h;
	UUtInt32 v;
	PHtDynamicSquare *grid = inGrid;
	UUtUns16 width = inGridX,height = inGridY;
	UUtInt16 gx,gy;
	UUtUns8 obstruction;

	PHrWorldToGridSpaceDangerous(&gx,&gy,&inObject->physics->position,inRoom);

	// _ftol called here
	if (0 && inObject->physics->flags & PHcFlags_Physics_FaceCollision)
	{
		// Geometric coverage
	}
	else
	{
		// Spherical ground coverage
		radius = MUrUnsignedSmallFloat_To_Int_Round(inObject->physics->sphereTree->sphere.radius / inRoom->squareSize) + 1;

		// reject object if it does not overlap the grid at all
		if ((gx < -radius) || (gx >= width + radius) || (gy < -radius) || (gy >= height + radius))
			return;

		// make an entry for this character in the obstruction table of the room
		// entry 0 holds the number of entries (since '0' in the grid means no
		// obstruction, we can't use entry 0 anyway)
		obstruction = (UUtUns8) inNode->obstruction[0];
		if (obstruction >= PHcMaxObstructions) {
			COrConsole_Printf("### PHrAddObjectToGrid: too many obstructions in one BNV");
			return;
		}
		inNode->obstruction[obstruction] = (UUtUns16) ((inIndex & PHcObstructionIndexMask) | PHcObstructionType_Object);
		inNode->obstruction[0] += 1;

		for (v = radius; v <= radius; v++)
		{
			for (h= -radius; h <= radius; h++)
			{
				PHrPutObstruction(grid,height,width,h+gx,v+gy,obstruction);
			}
		}
	}

	return;
}

void PHrAddDoorToGrid(UUtUns32 inIndex, M3tPoint3D *inPoint0, M3tPoint3D *inPoint1, M3tPoint3D *inPoint2, M3tPoint3D *inPoint3,
					  PHtDynamicSquare *inGrid, UUtUns16 inGridX, UUtUns16 inGridY, PHtNode *inNode, const PHtRoomData *inRoom)
{
	/************************
	* Makes a door's presence known in the grid
	* described by the inGrid parms. 'inGrid' is assumed
	* to be in the same coordinate space as 'inRoom'
	*
	* Rasterizes a line into the grid, or two lines if inPoint2 and inPoint3 are non-NULL.
	*/

	PHtDynamicSquare *grid = inGrid;
	UUtUns32 line;
	UUtUns16 width = inGridX,height = inGridY;
	UUtInt16 p0x, p0y, p1x, p1y;
	M3tPoint3D *p0, *p1;
	UUtUns8 obstruction = UUcMaxUns8;

	for (line = 0; line < 2; line++) {
		if (line == 0) {
			p0 = inPoint0;
			p1 = inPoint1;
		} else {
			p0 = inPoint2;
			p1 = inPoint3;
		}
		if ((p0 == NULL) || (p1 == NULL)) {
			// we do not have the necessary information to calculate this line
			continue;
		}

		// determine the line's location in the grid's space
		PHrWorldToGridSpaceDangerous(&p0x, &p0y, p0, inRoom);
		PHrWorldToGridSpaceDangerous(&p1x, &p1y, p1, inRoom);

		// reject the line if it does not overlap the grid at all
		if (((p0x < 0) && (p1x < 0)) ||
			((p0x >= width) && (p1x >= width)) ||
			((p0y < 0) && (p1y < 0)) ||
			((p0y >= height) && (p1y >= height))) {
			continue;
		}
	
		// get an obstruction index for this object
		if (obstruction == UUcMaxUns8) {
			// make an entry for this door in the obstruction table of the room
			// entry 0 holds the number of entries (since '0' in the grid means no
			// obstruction, we can't use entry 0 anyway)
			obstruction = (UUtUns8) inNode->obstruction[0];
			if (obstruction >= PHcMaxObstructions) {
				COrConsole_Printf("### PHrAddDoorToGrid: too many obstructions in one BNV");
				return;
			}
			inNode->obstruction[obstruction] = (UUtUns16) ((inIndex & PHcObstructionIndexMask) | PHcObstructionType_Door);
			inNode->obstruction[0] += 1;
		}

		// draw this obstruction into the grid
		PHrDynamicBresenham2(p0x, p0y, p1x, p1y, grid, width, height, obstruction, UUcFalse);
	}

	return;
}

	
UUtUns32 PHrAkiraNodeToIndex(AKtBNVNode *inNode, AKtEnvironment *inEnv)
{
	/***************************
	* Returns the absolute Akira index corrosponding to the
	* Akira node 'inNode'
	*/
	
	UUtUns32 node_index =  inNode->index;

	
	return node_index;
}

void PHrAppendToList(PHtNode *inElem, PHtNode **inHead)
{
	/*******************
	* Appends 'inElem' to 'inHead'
	*/
	
	PHtNode *head = *inHead;
	
	if (!head)
	{
		*inHead = inElem;
	}
	else
	{
		while (head->next) head = head->next;
		head->next = inElem;
	}	
	inElem->next = NULL;
}

void PHrRemoveFromList(PHtNode *inVictim, PHtNode **inHead)
{
	/********************
	* Removes 'inVictim' from 'inHead'
	*/
	
	PHtNode *head;
	
	head = *inHead;
	if (head == inVictim) *inHead = head->next;
	else do
	{
		if (head->next == inVictim)
		{
			head->next = ((PHtNode *)(head->next))->next;
		}
		else head = head->next;
	} while (head);
	
	inVictim->pathStatus = PHcPath_Undef;
}

PHtNode *PHrAkiraNodeToGraphNode(AKtBNVNode *inAkiraNode, PHtGraph *inGraph)
{
	if ((inAkiraNode == NULL) || (inAkiraNode->pathnodeIndex == (UUtUns32) -1)) {
		return NULL;
	} else {
		return inGraph->nodes + inAkiraNode->pathnodeIndex;
	}
}

void PHrAddNode(PHtGraph *ioGraph, AKtBNVNode *inRoom, UUtUns32 inSlot)
{
	/*************************
	* Creates a new node for 'inRoom' in 'ioGraph' at 'inSlot'.
	*/
	
	PHtNode *node = &ioGraph->nodes[inSlot];	
	inRoom->pathnodeIndex = inSlot;

	node->num_connections_in = 0;
	node->num_connections_out = 0;
	node->connections_in = NULL;
	node->connections_out = NULL;
	node->location = inRoom;

	// the node's pathfinding data has not yet been retrieved
	node->gridX = node->gridY = 0;
	node->grids_allocated = UUcFalse;

	PHrResetNode(node);
}


static UUtBool PHrBuildConnection(PHtGraph *ioGraph, AKtEnvironment *inEnvironment,
								  AKtBNVNode *inBNVSrc, PHtNode *inNodeSrc,
								  UUtUns32 inSideIndex, UUtUns32 inAdjacencyIndex)
{
	AKtAdjacency *adjacency, *test_adjacency;
	AKtGQ_General *gq, *test_gq;
	AKtBNVNode *dest_bnv;
	AKtBNVNode_Side *src_side;
	PHtNode *dest_node;
	PHtConnection *conn;
	UUtUns32 itr, p0_index, p1_index, side_itr, adj_itr;
	M3tPoint3D *point_array, *p0, *p1, *cur_point, *test_p0, *test_p1, src_midpoint;
	UUtBool p0_obstructed, p1_obstructed;

	// find the GQ for this side
	adjacency = &inEnvironment->adjacencyArray->adjacencies[inAdjacencyIndex];
	gq = &inEnvironment->gqGeneralArray->gqGeneral[adjacency->adjacentGQIndex];

	if ((gq->flags & (AKcGQ_Flag_Ghost | AKcGQ_Flag_SAT_Up | AKcGQ_Flag_SAT_Down)) == 0) {
		// this adjacency is not created by a ghost quad, do nothing
		return UUcFalse;
	}

	// find the lowest points on this gq
	point_array = inEnvironment->pointArray->points;
	p0_index = 0;
	p0 = point_array + gq->m3Quad.vertexIndices.indices[0];
	p1_index = (UUtUns32) -1;
	p1 = NULL;

	for(itr = 1; itr < 4; itr++) {
		cur_point = point_array + gq->m3Quad.vertexIndices.indices[itr];

		if (cur_point->y < p0->y) {
			p0 = cur_point;
			p0_index = itr;
		}
	}

	for(itr = 0; itr < 4; itr++) {
		if (itr == p0_index)
			continue;

		cur_point = point_array + gq->m3Quad.vertexIndices.indices[itr];

		if ((p1 == NULL) || (cur_point->y < p1->y)) {
			p1 = cur_point;
			p1_index = itr;
		}
	}
	
	UUmAssert((p0_index >= 0) && (p0_index < 4));
	UUmAssert((p1_index >= 0) && (p1_index < 4));

	p0_obstructed = p1_obstructed = UUcTrue;

	// check all adjacencies out of the source BNV to see whether there are other adjacencies
	// abutting the ends of this connection's edge
	for (side_itr = inBNVSrc->sideStartIndex; side_itr < inBNVSrc->sideEndIndex; side_itr++) {
		src_side = inEnvironment->bnvSideArray->sides + side_itr;

		for (adj_itr = src_side->adjacencyStartIndex; adj_itr < src_side->adjacencyEndIndex; adj_itr++) {
			// don't check the adjacency that we are processing
			if (adj_itr == inAdjacencyIndex)
				continue;

			// get the two points that make up this adjacency's edge. note that we can't use PHmConnection_Point
			// because we are still building the pathfinding graph.
			test_adjacency = inEnvironment->adjacencyArray->adjacencies + adj_itr;
			test_gq = inEnvironment->gqGeneralArray->gqGeneral + test_adjacency->adjacentGQIndex;
			AUrQuad_LowestPoints(&test_gq->m3Quad.vertexIndices, inEnvironment->pointArray->points, &test_p0, &test_p1);

			if ((p0_obstructed) &&
				((MUmVector_GetDistanceSquared(*test_p0, *p0) < UUmSQR(PHcGhostQuadTouchDist)) ||
				 (MUmVector_GetDistanceSquared(*test_p1, *p0) < UUmSQR(PHcGhostQuadTouchDist)))) {
				p0_obstructed = UUcFalse;
			}

			if ((p1_obstructed) &&
				((MUmVector_GetDistanceSquared(*test_p0, *p1) < UUmSQR(PHcGhostQuadTouchDist)) ||
				 (MUmVector_GetDistanceSquared(*test_p1, *p1) < UUmSQR(PHcGhostQuadTouchDist)))) {
				p1_obstructed = UUcFalse;
			}

			if ((!p0_obstructed) && (!p1_obstructed))
				break;
		}

		if ((!p0_obstructed) && (!p1_obstructed))
			break;
	}

	dest_bnv = &inEnvironment->bnvNodeArray->nodes[adjacency->adjacentBNVIndex];
	dest_node = PHrAkiraNodeToGraphNode(dest_bnv, ioGraph);

	/*
	 * create the connection
	 */
	conn = UUrMemory_Block_New(sizeof(PHtConnection));
	if (conn == NULL) {
		return UUcFalse;
	}
	
	conn->flags = 0;
	if ((inNodeSrc->location->flags & AKcBNV_Flag_Stairs_Standard) ||
		(dest_node->location->flags & AKcBNV_Flag_Stairs_Standard)) {
		// stair connections are always marked as obstructed at their edges so we don't fall off
		conn->flags |= PHcConnectionFlag_Stairs | PHcConnectionFlag_ObstructedPoint0 | PHcConnectionFlag_ObstructedPoint1;
	} else {
		if (p0_obstructed) {
			conn->flags |= PHcConnectionFlag_ObstructedPoint0;
		}
		if (p1_obstructed) {
			conn->flags |= PHcConnectionFlag_ObstructedPoint1;
		}
	}

	// store the indices of the ghost quad's base points
	conn->flags |= (p0_index << PHcConnectionMask_Point0Shift);
	conn->flags |= (p1_index << PHcConnectionMask_Point1Shift);
	MUmVector_Add(conn->connection_midpoint, *p0, *p1);
	MUmVector_Scale(conn->connection_midpoint, 0.5);
	conn->connection_width = MUrSqrt(UUmSQR(p1->x - p0->x) + UUmSQR(p1->z - p0->z));

	conn->from = inNodeSrc;
	conn->to = dest_node;
	conn->gunk = gq;
	conn->from_connectionindex		= (UUtUns16) -1;		// will be set up in a separate pass
	conn->to_connectionindex		= (UUtUns16) -1;
	conn->door_link = NULL;
	conn->num_blockages = (UUtUns16) -1;
	conn->blockage_baseindex = (UUtUns16) -1;

	// add this connection to the from node's list
	if (inNodeSrc->connections_out != NULL) {
		inNodeSrc->connections_out->prev_out = conn;
	}
	conn->next_out = inNodeSrc->connections_out;
	inNodeSrc->connections_out = conn;
	conn->prev_out = NULL;
	inNodeSrc->num_connections_out++;

	// add this connection to the to node's list
	if (dest_node->connections_in != NULL) {
		dest_node->connections_in->prev_in = conn;
	}
	conn->next_in = dest_node->connections_in;
	dest_node->connections_in = conn;
	conn->prev_in = NULL;
	dest_node->num_connections_in++;

	// find out if there is a door attached to this connection
	MUmVector_Add(src_midpoint, inBNVSrc->roomData.origin, inBNVSrc->roomData.antiOrigin);
	MUmVector_Scale(src_midpoint, 0.5);
	OBJrDoor_MakeConnectionLink(conn, &src_midpoint);

	return UUcTrue;
}

static void PHrRemoveConnection(PHtConnection *ioConnection, UUtBool inRemoveDependencies)
{
	UUmAssertReadPtr(ioConnection, sizeof(PHtConnection));

	if (inRemoveDependencies) {
		if (ioConnection->door_link != NULL) {
			// remove any door connection to this part of the pathfinding graph
			OBJtOSD_Door *door_osd = (OBJtOSD_Door *) (ioConnection->door_link->object_data);

			if (door_osd->pathfinding_connection[0] == ioConnection) {
				door_osd->pathfinding_connection[0] = NULL;
			}
			if (door_osd->pathfinding_connection[1] == ioConnection) {
				door_osd->pathfinding_connection[1] = NULL;
			}

			ioConnection->door_link = NULL;
		}
	}

	if (ioConnection->from_connectionindex != (UUtUns16) -1) {
		// remove this connection from the from node's list
		UUmAssertReadPtr(ioConnection->from, sizeof(PHtNode));
		UUmAssert(ioConnection->from->num_connections_out > 0);

		if (ioConnection->prev_out == NULL) {
			UUmAssert(ioConnection->from->connections_out == ioConnection);
			ioConnection->from->connections_out = ioConnection->next_out;
		} else {
			UUmAssert(ioConnection->prev_out->next_out == ioConnection);
			ioConnection->prev_out->next_out = ioConnection->next_out;
		}
		if (ioConnection->next_out != NULL) {
			UUmAssert(ioConnection->next_out->prev_out == ioConnection);
			ioConnection->next_out->prev_out = ioConnection->prev_out;
		}
		ioConnection->from->num_connections_out--;
		ioConnection->next_out = ioConnection->prev_out = NULL;
		ioConnection->from_connectionindex = (UUtUns16) -1;
	}
	ioConnection->from = NULL;

	if (ioConnection->to_connectionindex != (UUtUns16) -1) {
		// remove this connection from the to node's list
		UUmAssertReadPtr(ioConnection->to, sizeof(PHtNode));
		UUmAssert(ioConnection->to->num_connections_in > 0);

		if (ioConnection->prev_in == NULL) {
			UUmAssert(ioConnection->to->connections_in == ioConnection);
			ioConnection->to->connections_in = ioConnection->next_in;
		} else {
			UUmAssert(ioConnection->prev_in->next_in == ioConnection);
			ioConnection->prev_in->next_in = ioConnection->next_in;
		}
		if (ioConnection->next_in != NULL) {
			UUmAssert(ioConnection->next_in->prev_in == ioConnection);
			ioConnection->next_in->prev_in = ioConnection->prev_in;
		}
		ioConnection->to->num_connections_in--;
		ioConnection->next_in = ioConnection->prev_in = NULL;
		ioConnection->to_connectionindex = (UUtUns16) -1;
	}
	ioConnection->to = NULL;
}

/******************* Miscellaneous *********************/

void PHrDisplayGraph(PHtGraph *inGraph)
{
	/***********
	* Outputs an ASCII version of the pathfinding graph
	*/
	
	UUtUns32 i;
	PHtConnection *conn;
	
	for (i=0; i < inGraph->numNodes; i++)
	{
		fprintf(stderr,"BNV %d:\tfrom: ",i);

		conn = inGraph->nodes[i].connections_in;
		while (conn)
		{
			fprintf(stderr,"%d, ",((PHtNode *)conn->from)->location->index);
			conn = conn->next_in;
		}

		fprintf(stderr,"\to: ");
		conn = inGraph->nodes[i].connections_out;
		while (conn)
		{
			fprintf(stderr,"%d, ",((PHtNode *)conn->to)->location->index);
			conn = conn->next_out;
		}
		fprintf(stderr,UUmNL);
	}
}

void PHrRenderGrid(PHtNode *inNode, PHtRoomData *inRoom, IMtPoint2D *inErrStart, IMtPoint2D *inErrEnd)
{
	/**********
	* Renders the pathfinding grid in 3D for debugging (optionally displaying overhangs)
	*/

	M3tPoint3D line_points[3];
	UUtUns16 p,r,width;
	UUtUns32 current_time = UUrMachineTime_Sixtieths();
	UUtUns8 obstruction;
	M3tPoint3D c;
	PHtSquare chk;
	float h, line_y;
	PHtSquare *grid;
	
	if (inRoom->debug_render_time == current_time) {
		// don't draw the same grid twice
		return;
	}
	inRoom->debug_render_time = current_time;

	if (NULL == inRoom->compressed_grid) return;

	if (PHgRenderDynamicGrid) {
		// we must calculate the dynamic grid if we need to
		PHrPrepareRoomForPath(inNode, inRoom);
	}

	grid = UUrMemory_Block_New(inRoom->gridX * inRoom->gridY * sizeof(PHtSquare));

	if (NULL == grid) {
		return;
	}
	
	h = inRoom->squareSize / 2.0f;
	width = (short) inRoom->gridX;
	line_y = inRoom->origin.y + 1.0f;

	PHrGrid_Decompress(inRoom->compressed_grid, inRoom->compressed_grid_size, grid);
	
	for (p=0; p < inRoom->gridX; p++)					//	|  |  |  |  |
	{
		PHrGridToWorldSpace(p,0,NULL,&c,inRoom);
		line_points[0].x = c.x-h;
		line_points[0].y = line_y;
		line_points[0].z = c.z-h;
		PHrGridToWorldSpace(p,(UUtUns16)inRoom->gridY-1,NULL,&c,inRoom);
		line_points[1].x = c.x-h;
		line_points[1].y = line_y;
		line_points[1].z = c.z+h;
		M3rGeom_Line_Light(line_points,line_points+1,IMcShade_Gray50);
	}
	for (p=0; p<inRoom->gridY; p++)					//	---------------
	{
		PHrGridToWorldSpace(0,p,NULL,&c,inRoom);
		line_points[0].x = c.x-h;
		line_points[0].y = line_y;
		line_points[0].z = c.z-h;
		PHrGridToWorldSpace((UUtUns16)inRoom->gridX-1,p,NULL,&c,inRoom);
		line_points[1].x = c.x+h;
		line_points[1].y = line_y;
		line_points[1].z = c.z-h;
		M3rGeom_Line_Light(line_points,line_points+1,IMcShade_Gray50);
	}
	PHrGridToWorldSpace(0,(UUtUns16)inRoom->gridY-1,NULL,&c,inRoom);	//	_______________|
	line_points[0].x = c.x-h;	line_points[0].y = line_y;	line_points[0].z = c.z+h;
	PHrGridToWorldSpace((UUtUns16)inRoom->gridX-1,(UUtUns16)inRoom->gridY-1,NULL,&c,inRoom);
	line_points[1].x = c.x+h;	line_points[1].y = line_y;	line_points[1].z = c.z+h;
	PHrGridToWorldSpace((UUtUns16)inRoom->gridX-1,0,NULL,&c,inRoom);
	line_points[2].x = c.x+h;	line_points[2].y = line_y;	line_points[2].z = c.z-h;
	M3rGeom_Line_Light(line_points,line_points+1,IMcShade_Gray50);
	M3rGeom_Line_Light(line_points+1,line_points+2,IMcShade_Gray50);
	
	for (p=0; p<inRoom->gridY; p++)
	{
		for (r=0; r<inRoom->gridX; r++)
		{
			UUtUns32 shade;

			chk = GetCell(r,p);
			PHrGridToWorldSpace(r,p,NULL,&c,inRoom);

			if (((inErrStart != NULL) && (r == inErrStart->x) && (p == inErrStart->y)) ||
				((inErrEnd   != NULL) && (r == inErrEnd  ->x) && (p == inErrEnd  ->y))) {
				// draw a big pink X above the square
				line_points[0].x = c.x-h;	line_points[0].z = c.z-h;	line_points[0].y = line_y + 2.0f;
				line_points[1].x = c.x+h;	line_points[1].z = c.z+h;	line_points[1].y = line_y + 2.0f;
				M3rGeom_Line_Light(line_points,line_points+1,IMcShade_Pink);
				line_points[0].x = c.x+h;	line_points[0].z = c.z-h;	line_points[0].y = line_y + 2.0f;
				line_points[1].x = c.x-h;	line_points[1].z = c.z+h;	line_points[1].y = line_y + 2.0f;
				M3rGeom_Line_Light(line_points,line_points+1,IMcShade_Pink);				
			}

			if ((r == PHgDebugSquareX) && (p == PHgDebugSquareY)) {
				// this is the currently selected square for debugging display, outline it in yellow
				line_points[0].x = c.x-h;	line_points[0].z = c.z-h;	line_points[0].y = line_y + 0.5f;
				line_points[1].x = c.x-h;	line_points[1].z = c.z+h;	line_points[1].y = line_y + 0.5f;
				M3rGeom_Line_Light(&line_points[0], &line_points[1], IMcShade_Yellow);

				line_points[0].x = c.x+h;	line_points[0].z = c.z+h;
				M3rGeom_Line_Light(&line_points[0], &line_points[1], IMcShade_Yellow);

				line_points[1].x = c.x+h;	line_points[1].z = c.z-h;
				M3rGeom_Line_Light(&line_points[0], &line_points[1], IMcShade_Yellow);

				line_points[0].x = c.x-h;	line_points[0].z = c.z-h;
				M3rGeom_Line_Light(&line_points[0], &line_points[1], IMcShade_Yellow);
			}

			if (PHgRenderDynamicGrid && (inNode->grids_allocated) && (inNode->dynamic_grid != NULL)) {
				obstruction = inNode->dynamic_grid[r + p * width].obstruction;
				if (obstruction != 0) {
					// there is an obstruction here in the dynamic grid, draw a diamond
					shade = PHgPathfindingObstructionColorTable[obstruction % PHcPathfindingObstructionMaxColors];

					line_points[0].x = c.x;		line_points[0].z = c.z+h;	line_points[0].y = line_y;
					line_points[1].x = c.x+h;	line_points[1].z = c.z;		line_points[1].y = line_y;
					M3rGeom_Line_Light(&line_points[0], &line_points[1], shade);

					line_points[0].x = c.x;		line_points[0].z = c.z-h;
					M3rGeom_Line_Light(&line_points[0], &line_points[1], shade);

					line_points[1].x = c.x-h;	line_points[1].z = c.z;
					M3rGeom_Line_Light(&line_points[0], &line_points[1], shade);

					line_points[0].x = c.x;		line_points[0].z = c.z+h;
					M3rGeom_Line_Light(&line_points[0], &line_points[1], shade);
				}
			}

			if (chk.weight == PHcClear) {
				continue;
			} else if ((chk.weight < 0) || (chk.weight >= PHcMax)) {
				shade = IMcShade_Black;
			} else {
				shade = PHgPathfindingWeightColor[chk.weight];
			}

			// Scale size of X 
			h = inRoom->squareSize/2.0f;
			h = UUmPin(h,0.0f,inRoom->squareSize/2.0f);

			line_points[0].x = c.x-h;	line_points[0].z = c.z-h;	line_points[0].y = line_y;
			line_points[1].x = c.x+h;	line_points[1].z = c.z+h;	line_points[1].y = line_y;
			M3rGeom_Line_Light(line_points,line_points+1,shade);
			line_points[0].x = c.x+h;	line_points[0].z = c.z-h;	line_points[0].y = line_y;
			line_points[1].x = c.x-h;	line_points[1].z = c.z+h;	line_points[1].y = line_y;
			M3rGeom_Line_Light(line_points,line_points+1,shade);
		}
	}


	UUrMemory_Block_Delete(grid);
}

#if TOOL_VERSION
/************************* Debugging *****************************/

// find all connections that are accessible from a node and store them in PHgDebugGraph_Connections
void PHrDebugGraph_TraverseConnections(PHtGraph *ioGraph, PHtNode *inStartNode, PHtNode *inOtherNode,
									   UUtUns32 inDistanceLimit)
{
	PHtConnection *con;
	PHtNode *node;
	UUtUns32 reset_itr;
	AKtEnvironment *env;
	static UUtUns32 traverse_id = 1;

	traverse_id++;
	PHgDebugGraph_NumConnections = 0;
	PHgDebugGraph_DistanceLimit = inDistanceLimit;
	ioGraph->open = NULL;
	env = ONrGameState_GetEnvironment();

	// none of the nodes are currently in the accessible set
	for (reset_itr = 0; reset_itr < ioGraph->numNodes; reset_itr++) 
	{
		ioGraph->nodes[reset_itr].traverse_dist = (UUtUns32) -1;
		ioGraph->nodes[reset_itr].pathStatus = PHcPath_Undef;
	}

	inStartNode->traverse_dist = 0;
	PHrAppendToList(inStartNode, &ioGraph->open);
	inStartNode->pathStatus = PHcPath_Open;
	
	while ((node = ioGraph->open) || (inOtherNode != NULL))
	{
		if (node == NULL) {
			// we have exhausted all connections from the start node.
			// try the other node.
			node = inOtherNode;
			inOtherNode = NULL;
			node->traverse_dist = 0;
		} else {
			// get this node from the list
			PHrRemoveFromList(node,&ioGraph->open);
		}
		node->pathStatus = PHcPath_Open;

		for (con = node->connections_out; con != NULL; con = con->next_out) {
			if (PHgDebugGraph_NumConnections >= PHgDebugGraph_MaxConnections) {
				// enlarge the buffer
				PHgDebugGraph_MaxConnections += 50;
				PHgDebugGraph_Connections = (PHtConnection **) UUrMemory_Block_Realloc(PHgDebugGraph_Connections,
																	PHgDebugGraph_MaxConnections * sizeof(PHtConnection *));
			}

			if (con->traverse_id != traverse_id) {
				PHgDebugGraph_Connections[PHgDebugGraph_NumConnections++] = con;
				con->traverse_id = traverse_id;
				con->traverse_dist = node->traverse_dist;

				MUmVector_ScaleCopy(con->traverse_pts[0], 0.5f, con->from->location->roomData.origin);
				MUmVector_ScaleIncrement(con->traverse_pts[0], 0.5f, con->from->location->roomData.antiOrigin);
				con->traverse_pts[0].y = con->from->location->roomData.origin.y + 5.0f;

				MUmVector_ScaleCopy(con->traverse_pts[2], 0.5f, con->to->location->roomData.origin);
				MUmVector_ScaleIncrement(con->traverse_pts[2], 0.5f, con->to->location->roomData.antiOrigin);
				con->traverse_pts[2].y = con->to->location->roomData.origin.y + 5.0f;

				PHrWaypointFromGunk(env, con->gunk, &con->traverse_pts[1], &con->traverse_pts[0]);
			}

			if ((con->to->pathStatus != PHcPath_Open) && (con->traverse_dist < inDistanceLimit)) {
				con->to->pathStatus = PHcPath_Open;
				con->to->traverse_dist = con->traverse_dist + 1;
				PHrAppendToList(con->to, &ioGraph->open);
			}
		}
	}
	
	return;
}

// render the connections in the debug graph
void PHrDebugGraph_Render(void)
{
	UUtUns32 itr, itr2, val, range, blockage_val, *blockage_array;
	float b0, b1;
	PHtConnection *con;
	IMtShade shade;
	M3tPoint3D *p0, *p1, l0, l1;
	AKtEnvironment *environment = ONrGameState_GetEnvironment();

	for (itr = 0; itr < PHgDebugGraph_NumConnections; itr++) {
		con = PHgDebugGraph_Connections[itr];
		UUmAssert((con->traverse_dist >= 0) && (con->traverse_dist <= PHgDebugGraph_DistanceLimit));
		val = (3 * 256 * con->traverse_dist) / PHgDebugGraph_DistanceLimit;

		if (con->flags & PHcConnectionFlag_Impassable) {
			shade = IMcShade_Purple;

		} else if (con->flags & PHcConnectionFlag_Stairs) {
			shade = IMcShade_Blue;

		} else if (con->door_link != NULL) {
			shade = IMcShade_Green;

		} else {
			range = val >> 8;
			val = 255 - (val & 0xFF);

			if (range == 0) {
				shade = 0xFFFFFF00 | val;
			} else if (range == 1) {
				shade = 0xFFFF0000 | (val << 8);
			} else {
				shade = 0xFF000000 | (val << 16);
			}
		}

		M3rGeom_Line_Light(&con->traverse_pts[0], &con->traverse_pts[1], shade);
		M3rGeom_Line_Light(&con->traverse_pts[1], &con->traverse_pts[2], shade);

		// get the two base points of the connection
		p0 = PHmConnection_Point0(environment, con);
		p1 = PHmConnection_Point1(environment, con);
		if ((con->flags & PHcConnectionFlag_ObstructedPoint0) == 0) {
			l0 = *p0;
			l0.y = con->traverse_pts[1].y;
			M3rGeom_Line_Light(&con->traverse_pts[1], &l0, shade);
		}
		if ((con->flags & PHcConnectionFlag_ObstructedPoint1) == 0) {
			l1 = *p1;
			l1.y = con->traverse_pts[1].y;
			M3rGeom_Line_Light(&con->traverse_pts[1], &l1, shade);
		}

		// draw obstructions
		blockage_array = UUrMemory_Array_GetMemory(ONrGameState_GetGraph()->blockage_array);
		for (itr2 = 0; itr2 < con->num_blockages; itr2++) {
			blockage_val = blockage_array[itr2 + con->blockage_baseindex];

			// get the points of this blockage as t-values along the connection's base
			b0 = ((float) ((blockage_val & 0xFFFF0000) >> 16)) / UUcMaxUns16;
			b1 = ((float)  (blockage_val & 0x0000FFFF))        / UUcMaxUns16;

			// construct worldspace points for the blockage
			MUmVector_ScaleCopy(l0, 1.0f - b0, *p0);
			MUmVector_ScaleIncrement(l0, b0, *p1);
			MUmVector_ScaleCopy(l1, 1.0f - b1, *p0);
			MUmVector_ScaleIncrement(l1, b1, *p1);
			l0.y = l1.y = con->traverse_pts[1].y + 3.0f;

			// draw the obstruction in HOT PINK
			M3rGeom_Line_Light(&l0, &l1, IMcShade_Pink);
		}
	}
}

// delete the debugging traversal buffer
void PHrDebugGraph_Terminate(void)
{
	if (PHgDebugGraph_Connections != NULL) {
		UUrMemory_Block_Delete(PHgDebugGraph_Connections);
		PHgDebugGraph_Connections = NULL;
		PHgDebugGraph_NumConnections = 0;
	}
}
#endif

// flushes the dynamic pathfinding grid for a room
void PHrFlushDynamicGrid(PHtNode *inNode)
{
	inNode->calculate_time = 0;
}

// add the door stored in a connection into the dynamic grid
static UUtBool PHiAddDoorConnectionToGrid(PHtConnection *inConnection, UUtUns32 inGridIndex, PHtNode *inNode, const PHtRoomData *inRoom)
{
	UUtUns32 num_door_points, door_id;
	M3tPoint3D door_points[4];

	if ((inConnection->door_link == NULL) || (inConnection->last_gridindex == inGridIndex))
		return UUcFalse;

	// determine the endpoints of the area that is obstructed by the door
	num_door_points = OBJrDoor_ComputeObstruction(inConnection->door_link, &door_id, door_points);

	if (num_door_points >= 4) {
		PHrAddDoorToGrid(door_id, &door_points[0], &door_points[1], &door_points[2], &door_points[3],
						inNode->dynamic_grid, (UUtUns16) inNode->gridX, (UUtUns16) inNode->gridY, inNode, inRoom);
	} else if (num_door_points >= 2) {
		PHrAddDoorToGrid(door_id, &door_points[0], &door_points[1], NULL, NULL,
						inNode->dynamic_grid, (UUtUns16) inNode->gridX, (UUtUns16) inNode->gridY, inNode, inRoom);
	}

	inConnection->last_gridindex = inGridIndex;
	return UUcTrue;
}

// decompresses and/or calculates the pathfinding grids for a room if necessary
void PHrPrepareRoomForPath(PHtNode *inNode, const PHtRoomData *inRoom)
{
	UUtUns32		numObjects, size, current_time, itr;
	OBtObject		*object;
	UUtBool			recalculate_dynamic_grid = UUcFalse;
	PHtGraph		*graph = ONrGameState_GetGraph();
	PHtConnection	*connection;

	UUmAssertReadPtr(inRoom, sizeof(PHtRoomData));
	UUmAssert((inRoom->gridX > UUcMinUns16) && (inRoom->gridX < UUcMaxUns16));	// casting later
	UUmAssert((inRoom->gridY > UUcMinUns16) && (inRoom->gridY < UUcMaxUns16));	// casting later
	
	current_time = ONrGameState_GetGameTime();

	if (!inNode->grids_allocated) {
		// allocate space for the room data
		inNode->gridX = inRoom->gridX;
		inNode->gridY = inRoom->gridY;

		size = inNode->gridX * inNode->gridY;
		if (size & 0x03) {
			size = (size & ~0x03) + 0x04;
		}
		inNode->array_size = size;
		inNode->no_static_grid = (inRoom->compressed_grid == NULL);

		if (inNode->no_static_grid) {
			lrar_allocate(graph->grid_cache, size, "", (void *) inNode);	// one array for dynamic data

			UUmAssertReadPtr(inNode->dynamic_grid, size);
			inNode->static_grid = NULL;
		} else {
			lrar_allocate(graph->grid_cache, size * 2, "", (void *) inNode);	// 2 arrays; static and dynamic (same size)

			UUmAssertReadPtr(inNode->static_grid, size);
			UUmAssertReadPtr(inNode->dynamic_grid, size);

			//decompress the static room data into the static grid
			PHrGrid_Decompress(inRoom->compressed_grid, inRoom->compressed_grid_size, inNode->static_grid);
		}

		recalculate_dynamic_grid = UUcTrue;
	}

	// how long has it been since the last time the dynamic data here was rebuilt?
	if (inNode->calculate_time == 0) {
		recalculate_dynamic_grid = UUcTrue;
	} else {
		recalculate_dynamic_grid = recalculate_dynamic_grid ||
			(current_time >= inNode->calculate_time + PHcDynamicDataAgeTime);
	}

	if (recalculate_dynamic_grid) {
		ONtCharacter **active_character_list = ONrGameState_ActiveCharacterList_Get();
		UUtUns32 active_character_count = ONrGameState_ActiveCharacterList_Count();
		UUtUns32 active_character_itr;
		UUtUns32 object_index;
		static UUtUns32 unique_gridpreparation_index = 0;
	
		UUmAssertWritePtr(inNode->dynamic_grid, inNode->array_size);
		UUrMemory_Clear(inNode->dynamic_grid, inNode->array_size);

		unique_gridpreparation_index++;

		// no obstructions yet - next will be inserted in index 1 because index 0 is a count
		inNode->obstruction[0] = 1;

		// Add the characters to the grid
		for(active_character_itr = 0; active_character_itr < active_character_count; active_character_itr++)
		{
			ONtCharacter *current_character = active_character_list[active_character_itr];
			ONtCharacterIndexType index = ONrCharacter_GetIndex(current_character);

			if (current_character->flags & (ONcCharacterFlag_Dead_3_Cosmetic | ONcCharacterFlag_Teleporting)) {
				// we don't have to pathfind around this character
				continue;
			}

			PHrAddCharacterToGrid(index, current_character, inNode->dynamic_grid, (UUtUns16) inNode->gridX, (UUtUns16) inNode->gridY, inNode, inRoom);
		}
		
		// Add the objects to the grid
		numObjects = ONgGameState->objects->numObjects;
		for (object_index = 0; object_index < numObjects; object_index++)
		{
			object = ONgGameState->objects->object_list + object_index;

			if (!(object->flags & OBcFlags_InUse)) continue;
			
			// Include animated objects, but not small projectiles
			if (object->physics->flags & PHcFlags_Physics_Projectile) continue;

			// Don't include hidden objects or door objects (which are handled separately)
			if (object->flags & (OBcFlags_Invisible | OBcFlags_IsDoor)) continue;

			PHrAddObjectToGrid(object_index, object, inNode->dynamic_grid, (UUtUns16) inNode->gridX, (UUtUns16) inNode->gridY, inNode, inRoom);
		}

		// Add the doors to the grid
		connection = inNode->connections_out;
		for (itr = 0; itr < inNode->num_connections_out; itr++, connection = connection->next_out) {
			if (connection == NULL) {
				UUmAssert(!"PHrPrepareRoomForPath: mismatch indices/list in node's out connections");
				break;
			}
			PHiAddDoorConnectionToGrid(connection, unique_gridpreparation_index, inNode, inRoom);
		}
		connection = inNode->connections_in;
		for (itr = 0; itr < inNode->num_connections_in; itr++, connection = connection->next_in) {
			if (connection == NULL) {
				UUmAssert(!"PHrPrepareRoomForPath: mismatch indices/list in node's in connections");
				break;
			}
			PHiAddDoorConnectionToGrid(connection, unique_gridpreparation_index, inNode, inRoom);
		}

		inNode->calculate_time = current_time;
	}

	return;
}

static void PHiLRAR_NewProc(void *inUserData, short inBlockIndex)
{
	PHtGraph *graph = ONrGameState_GetGraph();
	PHtNode *node = (PHtNode *) inUserData;
	UUtUns8 *mem_base;
	UUmAssertReadPtr(node, sizeof(PHtNode));

	mem_base = graph->grid_storage + lrar_block_address(graph->grid_cache, inBlockIndex);

	if (node->no_static_grid) {
		node->static_grid = NULL;
		node->dynamic_grid = (PHtDynamicSquare *) mem_base;
	} else {
		node->static_grid = (PHtSquare *) mem_base;
		node->dynamic_grid = (PHtDynamicSquare *) (mem_base + node->array_size);
	}
	node->cache_block_index = inBlockIndex;

	node->grids_allocated = UUcTrue;
	node->calculate_time = 0;
}

static void PHiLRAR_PurgeProc(void *inUserData)
{
	PHtNode *node = (PHtNode *) inUserData;

	node->static_grid = NULL;
	node->dynamic_grid = NULL;
	node->cache_block_index = NONE;
	node->grids_allocated = UUcFalse;
}

UUtBool PHrObstructionIsCharacter(UUtUns16 inObstruction, ONtCharacter **outCharacter)
{
	ONtCharacter *character;
	UUtUns32 character_index;

	if ((inObstruction & PHcObstructionTypeMask) != PHcObstructionType_Character)
		return UUcFalse;
	
	character_index = inObstruction & PHcObstructionIndexMask;
	UUmAssert((character_index  >= 0) && (character_index  < ONcMaxCharacters));
	character = &ONgGameState->characters[character_index ];

	if ((character->flags & ONcCharacterFlag_InUse) == 0)
		return UUcFalse;

	if (outCharacter != NULL) {
		*outCharacter = character;
	}
	return UUcTrue;
}

UUtBool PHrObstructionIsDoor(UUtUns16 inObstruction, OBJtObject **outDoor)
{
	OBJtObject *door_object;

	if ((inObstruction & PHcObstructionTypeMask) != PHcObstructionType_Door)
		return UUcFalse;

	door_object = OBJrDoor_FindDoorWithID(inObstruction & PHcObstructionIndexMask);
	if (door_object == NULL) {
		return UUcFalse;
	} else {
		if (outDoor != NULL) {
			*outDoor = door_object;
		}
		return UUcTrue;
	}
}

UUtBool PHrIgnoreObstruction(ONtCharacter *inCharacter, PHtPathMode inPathMode, UUtUns16 inObstruction)
{
	UUtBool ignore = UUcFalse;

	switch(inObstruction & PHcObstructionTypeMask) {
		case PHcObstructionType_Character:
		{
			ONtCharacter *character;
			
			inObstruction &= PHcObstructionIndexMask;
			UUmAssert((inObstruction >= 0) && (inObstruction < ONcMaxCharacters));
			character = &ONgGameState->characters[inObstruction];
			
			// this obstruction is a character - we may ignore it in some cases
			if (character == inCharacter) {
				ignore = UUcTrue;
				
			} else if (inPathMode == PHcPathMode_CheckClearSpace) {
				ignore = UUcFalse;

			} else if (inCharacter->flags & ONcCharacterFlag_AIMovement) {
				ignore = AI2rTryToPathfindThrough(inCharacter, character);
			}
			break;
		}

		case PHcObstructionType_Object:
		{
			OBtObject *object;
			
			inObstruction &= PHcObstructionIndexMask;
			UUmAssert((inObstruction >= 0) && (inObstruction < ONgGameState->objects->numObjects));
			object = ONgGameState->objects->object_list + inObstruction;
			
			/*
			// this is an object - all objects are marked as impassable
			// EXCEPT at present all doors. [this may change in future when AIs
			// can only open some doors]
		
			if (object->flags & OBcFlags_InUse) {
				if (object->flags & OBcFlags_IsDoor) {
					UUmAssertReadPtr(object->owner, sizeof(OBJtObject));
					ignore = OBJrDoor_CharacterCanOpen(inCharacter, (OBJtObject *) object->owner, OBJcDoor_EitherSide);
				} else {
					ignore = UUcFalse;
				}
			}
			else {
				ignore = UUcTrue;
			}*/
			ignore = ((object->flags & OBcFlags_InUse) == 0) || (object->flags & OBcFlags_Invisible);
			break;
		}

		case PHcObstructionType_Door:
		{
			if (inPathMode == PHcPathMode_CheckClearSpace) {
				ignore = UUcFalse;

			} else {
				OBJtObject *door_object;

				door_object = OBJrDoor_FindDoorWithID(inObstruction & PHcObstructionIndexMask);
				if (door_object == NULL) {
					ignore = UUcTrue;

				} else {
					if (OBJrDoor_IsOpen(door_object) || !OBJrDoor_CharacterCanOpen(inCharacter, door_object, OBJcDoor_EitherSide)) {
						// either this door is already open, or we can't open it further; either way we can't
						// change the space that it physically occupies
						ignore = UUcFalse;

					} else if ((inPathMode == PHcPathMode_CasualMovement) && OBJrDoor_OpensManually(door_object)) {
						// we can't open this door by just wandering into it
						ignore = UUcFalse;

					} else {
						// we can open this door, it's not an obstruction
						ignore = UUcTrue;
					}
				}
			}
			break;
		}
	}

	return ignore;
}

UUtBool PHrSquareIsPassable(ONtCharacter *inCharacter, PHtPathMode inPathMode,
							PHtNode *inNode, const PHtRoomData *inRoom, UUtUns32 *inObstructionBV,
							UUtUns16 inX, UUtUns16 inY, UUtBool *outObstruction, UUtUns8 *outWeight)
{
	UUtUns32 index;
	UUtUns8 obstruction, weight;

	if (outObstruction) {
		*outObstruction = UUcFalse;
	}

	// is the new loc valid?
	if (((inX < 0) || (inX >= (UUtInt16)inRoom->gridX)) ||
		((inY < 0) || (inY >= (UUtInt16)inRoom->gridY))) {
		if (outWeight)
			*outWeight = PHcImpassable;
		return UUcFalse;
	}

	// calculate the grid index
	index = inX + (inY * inRoom->gridX);
	
	if (inNode->no_static_grid) {
		weight = PHcClear;
	} else {
		// decide if we can go there
		UUmAssertReadPtr(inNode->static_grid, inNode->array_size);
		weight = inNode->static_grid[index].weight;
		if (weight == PHcImpassable) {
			if (outWeight) {
				*outWeight = weight;
			}
			return UUcFalse;
		}
	}
	
	UUmAssertReadPtr(inNode->dynamic_grid, inNode->array_size);
	obstruction = inNode->dynamic_grid[index].obstruction;
	if (obstruction) {
		UUmAssert((obstruction > 0) && (obstruction < PHcMaxObstructions));

		// have we already tested this obstruction?
		if (UUrBitVector_TestBit(inObstructionBV, 2 * obstruction)) {
			// get the stored value of whether the obstruction can be ignored.
			// if not, then return impassable
			if (!UUrBitVector_TestBit(inObstructionBV, 2*obstruction + 1)) {
				if (outWeight) {
					*outWeight = PHcImpassable;
				}
				if (outObstruction) {
					*outObstruction = UUcTrue;
				}
				return UUcFalse;
			}

		} else {
			UUtBool ignore;

			// perform the test - can we safely ignore this obstruction?
			ignore = PHrIgnoreObstruction(inCharacter, inPathMode, inNode->obstruction[obstruction]);
			
			// flag that we have tested this obstruction already
			UUrBitVector_SetBit(inObstructionBV, 2*obstruction);

			if (ignore) {
				// store that this obstruction can be ignored
				UUrBitVector_SetBit(inObstructionBV, 2*obstruction + 1);
			} else {
				// this square is obstructed
				if (outWeight) {
					*outWeight = PHcImpassable;
				}
				if (outObstruction) {
					*outObstruction = UUcTrue;
				}
				return UUcFalse;
			}
		}
	}

	// this square is passable
	if (outWeight)
		*outWeight = weight;

	return UUcTrue;
}

// draws a bresenham line through the grid, and returns the highest
// weight found by said line. stops and returns upon encountering
// Danger or Impassable.
//
// NB: this is a NON-DIAGONAL bresenham line so that we don't slip
// through diagonally drawn lines in the pathfinding grid!
UUtBool PHrLocalPathWeight(
	ONtCharacter *inCharacter,
	PHtPathMode inPathMode,
	PHtNode *inNode,
	PHtRoomData *inRoom,
	UUtUns32 *inObstructionBV,
	UUtUns8 stop_weight,
	UUtUns32 passthrough_squares,
	UUtInt32 x1,
	UUtInt32 y1,
	UUtInt32 x2,
	UUtInt32 y2,
	UUtBool *escape_path,
	UUtUns8 *lowest_weight,
	UUtUns8 *highest_weight,
	UUtInt32 *last_x,
	UUtInt32 *last_y)
{
	UUtUns32 width = inRoom->gridX;
	UUtUns32 height = inRoom->gridY;
	UUtInt32 ax,ay,dx,dy,x,y,e,xdir,ydir;
	UUtUns8 thiscell, highest = PHcClear, lowest = PHcImpassable;
	UUtBool first_square = UUcTrue, prev_obstruction, obstruction;

	UUmAssert((x1 >= 0) && (x1 < (UUtInt32) width) && (y1 >= 0) && (y1 < (UUtInt32) height));
	UUmAssert((x2 >= 0) && (x2 < (UUtInt32) width) && (y2 >= 0) && (y2 < (UUtInt32) height));

	dx = x2-x1;
	dy = y2-y1;
	ax = UUmABS(x2-x1);
	ay = UUmABS(y2-y1);
	
	if (dx < 0) {
		xdir = -1;
	}
	else if (dx==0) {
		xdir = 0;
	}
	else {
		xdir = 1;
	}

	if (dy<0) {
		ydir = -1;
	}
	else if (dy==0) {
		ydir=0;
	}
	else {
		ydir = 1;
	}
	
	x = x1;
	y = y1;
	
	*escape_path = UUcFalse;
	prev_obstruction = UUcFalse;
	*last_x = x;
	*last_y = y;

	if (ax > ay)
	{
		e = (ay-ax)/2;
		while (1)
		{
			if (!PHrSquareIsPassable(inCharacter, inPathMode, inNode, inRoom, inObstructionBV,
									(UUtUns16) x, (UUtUns16) y, &obstruction, &thiscell))
				thiscell = PHcImpassable;

			if (thiscell < lowest) {
				lowest = thiscell;
			}

			if (thiscell > highest) {
				highest = thiscell;
			}

			if (thiscell >= stop_weight) {
				// we are currently looking at a stop cell.
				//
				// fail the path for a number of reasons:
				//   - if we have previously been below the stop threshold, and we are now entering
				//     an area above the stop threshold.
				//
				//   - if we are at the destination and it's impassable
				//
				//   - if we have exceeded our allowable number of passthrough squares
				//
				//   - if we are trying to go from an obstructed impassability to a real impassability
				if ((lowest < stop_weight) || (passthrough_squares == 0) || ((x == x2) && (y == y2)) ||
					(prev_obstruction && (thiscell >= PHcImpassable) && !obstruction)) {
					*highest_weight = highest;
					*lowest_weight = lowest;
					return UUcFalse;
				}
				
				if (first_square) {
					// we are actually standing on an 'impassable' square. this is an escape path.
					*escape_path = UUcTrue;
				}

				passthrough_squares--;
			} else {
				// this is a valid cell
				*last_x = x;
				*last_y = y;

				if ((x == x2) && (y == y2)) {
					*highest_weight = highest;
					*lowest_weight = lowest;
					return UUcTrue;
				}
			}
			
			if (e>=0)
			{
				y+=ydir;
				e-=ax;
			}
			else
			{
				x+=xdir;
				e+=ay;
			}

			first_square = UUcFalse;
			prev_obstruction = obstruction;
		}
	}
	else
	{
		e = (ax-ay)/2;
		while (1)
		{
			if (!PHrSquareIsPassable(inCharacter, inPathMode, inNode, inRoom, inObstructionBV,
									(UUtUns16) x, (UUtUns16) y, &obstruction, &thiscell))
				thiscell = PHcImpassable;

			if (thiscell < lowest) {
				lowest = thiscell;
			}

			if (thiscell > highest) {
				highest = thiscell;
			}

			if (thiscell >= stop_weight) {
				// we are currently looking at a stop cell.
				//
				// fail the path for a number of reasons:
				//   - if we have previously been below the stop threshold, and we are now entering
				//     an area above the stop threshold.
				//
				//   - if we are at the destination and it's impassable
				//
				//   - if we have exceeded our allowable number of passthrough squares
				if ((lowest < stop_weight) || (passthrough_squares == 0) || ((x == x2) && (y == y2)) ||
					(prev_obstruction && (thiscell >= PHcImpassable) && !obstruction)) {
					*highest_weight = highest;
					*lowest_weight = lowest;
					return UUcFalse;
				}
				
				if (first_square) {
					// we are actually standing on an 'impassable' square. this is an escape path.
					*escape_path = UUcTrue;
				}

				passthrough_squares--;
			} else {
				// this is a valid cell
				*last_x = x;
				*last_y = y;

				if ((x == x2) && (y == y2)) {
					*highest_weight = highest;
					*lowest_weight = lowest;
					return UUcTrue;
				}
			}

			if (e>=0)
			{
				x+=xdir;
				e-=ay;
			}
			else
			{
				y+=ydir;
				e+=ax;
			}

			first_square = UUcFalse;
			prev_obstruction = obstruction;
		}
	}

	UUmAssert(!"never get here");
}

static void PHiAddBlockage(float inT, float inSize, UUtUns32 inMaxBlockages,
						   UUtUns32 *ioNumBlockages, float *ioBlockageArray)
{
	UUtUns32 itr;
	const float t_eps = 0.01f;
	float t_low, t_hi;

	t_low = UUmPin(inT - inSize, 0.0f, 1.0f);
	t_hi  = UUmPin(inT + inSize, 0.0f, 1.0f);
	if (t_low >= t_hi) {
		// this blockage is empty
		return;
	}

	for (itr = 0; itr < *ioNumBlockages; itr++) {
		if (ioBlockageArray[2*itr + 1] > t_low - t_eps) {
			// the end of this blockage comes after our beginning... we should add to it, or before it
			break;
		}
	}

	if ((itr == *ioNumBlockages) || (ioBlockageArray[2*itr + 0] > t_hi + t_eps)) {
		// we must come before this blockage... create a new entry
		if (*ioNumBlockages >= inMaxBlockages) {
			UUrDebuggerMessage("PHiAddBlockage: overflowed max blockages (%d)\n", inMaxBlockages);
			return;
		}

		if (itr < *ioNumBlockages) {
			// move the rest of the blockages along the array
			UUrMemory_MoveOverlap(ioBlockageArray + 2*itr, ioBlockageArray + 2*(itr + 1),
									(*ioNumBlockages - itr) * 2 * sizeof(float));
		}
		ioBlockageArray[2*itr + 0] = t_low;
		ioBlockageArray[2*itr + 1] = t_hi;
		*ioNumBlockages += 1;
	} else {
		// we have found a blockage that we overlap, expand it
		if (ioBlockageArray[2*itr + 0] > t_low) {
			ioBlockageArray[2*itr + 0] = t_low;

			// sanity check
			UUmAssert((itr == 0) || (ioBlockageArray[2*itr + 0] > ioBlockageArray[2*(itr - 1) + 1]));
		}
		if (ioBlockageArray[2*itr + 1] < t_hi) {
			ioBlockageArray[2*itr + 1] = t_hi;

			if ((itr + 1 < *ioNumBlockages) && (ioBlockageArray[2*itr + 1] > ioBlockageArray[2*(itr + 1) + 0] - t_eps)) {
				// the blockage which we expanded now overlaps the next one higher, merge them together
				ioBlockageArray[2*itr + 1] = ioBlockageArray[2*(itr + 1) + 1];

				if (itr + 2 < *ioNumBlockages) {
					// move the rest of the blockages down
					UUrMemory_MoveOverlap(ioBlockageArray + 2*(itr + 2), ioBlockageArray + 2*(itr + 1),
											(*ioNumBlockages - (itr + 2)) * 2 * sizeof(float));
				}
				*ioNumBlockages -= 1;
			}
		}
	}

#if defined(DEBUGGING) && DEBUGGING
	// sanity check the blockage array
	UUmAssert(*ioNumBlockages < inMaxBlockages);
	for (itr = 0; itr < *ioNumBlockages; itr++) {
		UUmAssert(ioBlockageArray[2*itr + 1] >= ioBlockageArray[2*itr + 0]);
		if (itr > 0) {
			UUmAssert(ioBlockageArray[2*itr + 0] > ioBlockageArray[2*(itr - 1) + 1]);
		}
	}
#endif
}

static UUtBool PHiSquareIsBlocked(PHtSquare *inSquare)
{
	switch(inSquare->weight) {
		case PHcBorder4:
		case PHcDanger:
		case PHcImpassable:
			return UUcTrue;
	}

	return UUcFalse;
}

static void PHiCheckForBlockages(PHtNode *inNode, M3tPoint3D *inPoint0, M3tPoint3D *inPoint1,
								 UUtUns32 inMaxBlockages, UUtUns32 *ioNumBlockages, float *ioBlockageArray)
{
	PHtRoomData *room = &inNode->location->roomData;
	UUtUns16 p0_x, p0_y, p1_x, p1_y;
	float distsq, start_t, cur_t, p_dx, p_dz, x_dt, y_dt, t_size, grid_height, t_offset_x, t_offset_y, t_offset, edge_t;
	M3tPoint3D start_gridpt, start_pt, end_pt, edge_vector;
	UUtUns32 width = room->gridX;
	UUtInt16 start_x, start_y, end_x, end_y;
	UUtInt32 ax,ay,dx,dy,x,y,e,xdir,ydir;
	PHtSquare *grid, this_cell;

	if (room->compressed_grid == NULL) {
		// no obstructions, hence no blockages
		return;
	}

	if (!PHrWorldInGridSpace(inPoint0, room)) {
		UUrDebuggerMessage("PHrBuildGraph: connection [%f,%f,%f]-[%f,%f,%f] point 0 is not in room (BNV %d)\n",
							inPoint0->x, inPoint0->y, inPoint0->z, inPoint1->x, inPoint1->y, inPoint1->z, inNode->location->index);
		return;
	}
	if (!PHrWorldInGridSpace(inPoint1, room)) {
		UUrDebuggerMessage("PHrBuildGraph: connection [%f,%f,%f]-[%f,%f,%f] point 1 is not in room (BNV %d)\n",
							inPoint0->x, inPoint0->y, inPoint0->z, inPoint1->x, inPoint1->y, inPoint1->z, inNode->location->index);
		return;
	}

	// find the endpoints in grid-space
	PHrWorldToGridSpace(&p0_x, &p0_y, inPoint0, room);
	PHrWorldToGridSpace(&p1_x, &p1_y, inPoint1, room);

	// decompress the grid into a temporary buffer
	grid = UUrMemory_Block_New(room->gridX * room->gridY * sizeof(PHtSquare));
	if (grid == NULL) {
		return;
	}
	PHrGrid_Decompress(room->compressed_grid, room->compressed_grid_size, grid);

	// figure out how far we should offset the start point of the grid line
	dx = p1_x - p0_x;
	dy = p1_y - p0_y;
	ax = UUmABS(dx);
	ay = UUmABS(dy);
	MUmVector_Subtract(edge_vector, *inPoint1, *inPoint0);
	
	// offset the start point away from the end point by a max of 2 squares
	MUmVector_Copy(start_pt, *inPoint0);
	if (ax != 0) {
		t_offset_x = -2.0f / ax;
		if (dx > 0) {
			// don't offset over grid X = 0
			edge_t = - ((             - room->gox  * room->squareSize) + (inPoint0->x - room->origin.x)) / (inPoint1->x - inPoint0->x);
			t_offset_x = UUmMax(t_offset_x, edge_t);
		} else {
			// don't offset over grid X = gridX
			edge_t = - (((room->gridX - room->gox) * room->squareSize) - (inPoint0->x - room->origin.x)) / (inPoint1->x - inPoint0->x);
			t_offset_x = UUmMax(t_offset_x, edge_t);
		}
	}
	if (ay == 0) {
		if (ax == 0) {
			// the connection is zero-length, only check the cell that it starts in
			if ((p0_x >= 0) && (p0_x < room->gridX) && (p0_y >= 0) && (p0_y < room->gridY)) {
				this_cell = GetCell(p0_x, p0_y);
				if (PHiSquareIsBlocked(&this_cell)) {
					// this square is blocked
					PHiAddBlockage(0, 1.0f, inMaxBlockages, ioNumBlockages, ioBlockageArray);
				}
			}

			goto exit;
		} else {
			t_offset = t_offset_x;
		}
	} else {
		t_offset_y = -2.0f / ay;
		if (dy > 0) {
			// don't offset over grid Y = 0
			edge_t = - ((             - room->goy  * room->squareSize) + (inPoint0->z - room->origin.z)) / (inPoint1->z - inPoint0->z);
			t_offset_y = UUmMax(t_offset_y, edge_t);
		} else {
			// don't offset over grid Y = gridY
			edge_t = - (((room->gridY - room->goy) * room->squareSize) - (inPoint0->z - room->origin.z)) / (inPoint1->z - inPoint0->z);
			t_offset_y = UUmMax(t_offset_y, edge_t);
		}

		if (ax == 0) {
			t_offset = t_offset_y;
		} else {
			t_offset = UUmMax(t_offset_x, t_offset_y);
		}
	}
	if (t_offset < 0) {
		MUmVector_ScaleIncrement(start_pt, t_offset, edge_vector);
	}
	PHrWorldToGridSpaceDangerous(&start_x, &start_y, &start_pt, room);

	// offset the end point away from the start point by a max of 2 squares
	MUmVector_Copy(end_pt, *inPoint1);
	if (ax != 0) {
		t_offset_x = 2.0f / ax;
		if (dx < 0) {
			// don't offset over grid X = 0
			edge_t = ((             - room->gox  * room->squareSize) + (inPoint1->x - room->origin.x)) / (inPoint1->x - inPoint0->x);
			t_offset_x = UUmMin(t_offset_x, edge_t);
		} else {
			// don't offset over grid X = gridX
			edge_t = (((room->gridX - room->gox) * room->squareSize) - (inPoint1->x - room->origin.x)) / (inPoint1->x - inPoint0->x);
			t_offset_x = UUmMin(t_offset_x, edge_t);
		}
	}
	if (ay == 0) {
		if (ax == 0) {
			t_offset = 0;
		} else {
			t_offset = t_offset_x;
		}
	} else {
		t_offset_y = 2.0f / ay;
		if (dy < 0) {
			// don't offset over grid Y = 0
			edge_t = ((             - room->goy  * room->squareSize) + (inPoint1->z - room->origin.z)) / (inPoint1->z - inPoint0->z);
			t_offset_y = UUmMin(t_offset_y, edge_t);
		} else {
			// don't offset over grid Y = gridY
			edge_t = (((room->gridY - room->goy) * room->squareSize) - (inPoint1->z - room->origin.z)) / (inPoint1->z - inPoint0->z);
			t_offset_y = UUmMin(t_offset_y, edge_t);
		}

		if (ax == 0) {
			t_offset = t_offset_y;
		} else {
			t_offset = UUmMin(t_offset_x, t_offset_y);
		}
	}
	if (t_offset > 0) {
		MUmVector_ScaleIncrement(end_pt, t_offset, edge_vector);
	}
	PHrWorldToGridSpaceDangerous(&end_x, &end_y, &end_pt, room);

	// snap the start and end points to the edges of the grid (safety check)
	start_x = UUmPin(start_x, 0, ((UUtInt16) room->gridX) - 1);
	start_y = UUmPin(start_y, 0, ((UUtInt16) room->gridY) - 1);
	end_x   = UUmPin(end_x,   0, ((UUtInt16) room->gridX) - 1);
	end_y   = UUmPin(end_y,   0, ((UUtInt16) room->gridY) - 1);

	// determine the t-values of the start point and a single X or Y step
	p_dx = inPoint1->x - inPoint0->x;
	p_dz = inPoint1->z - inPoint0->z;
	distsq = UUmSQR(p_dx) + UUmSQR(p_dz);

	// we know that the points can't be directly on top of each other, because if they
	// were ax and ay would have been both zero and we would have prematurely exited.
	UUmAssert(distsq > UUmSQR(PHcCheckBlockageTolerance));

	grid_height = 0;
	PHrGridToWorldSpace(start_x, start_y, &grid_height, &start_gridpt, room);
	start_t = ((start_gridpt.x - inPoint0->x) * p_dx + (start_gridpt.z - inPoint0->z) * p_dz) / distsq;
	x_dt = PHcSquareSize * p_dx / distsq;
	y_dt = PHcSquareSize * p_dz / distsq;
	t_size = PHcObstructionSize / MUrSqrt(distsq);

	// traverse the grid along the computed line segment in a non-diagonal fashion, looking for impassable squares
	dx = end_x - start_x;
	dy = end_y - start_y;
	ax = UUmABS(dx);
	ay = UUmABS(dy);
	
	if (dx < 0) {
		xdir = -1;
	} else if (dx == 0) {
		xdir = 0;
	} else {
		xdir = 1;
	}

	if (dy < 0) {
		ydir = -1;
	} else if (dy == 0) {
		ydir = 0;
	} else {
		ydir = 1;
	}
	
	x = start_x;
	y = start_y;
	cur_t = start_t;
	
	if (ax > ay) {
		e = (ay - ax) / 2;
		while (1) {
			this_cell = GetCell(x, y);
			if (PHiSquareIsBlocked(&this_cell)) {
				// this square is blocked
				PHiAddBlockage(cur_t, t_size, inMaxBlockages, ioNumBlockages, ioBlockageArray);
			}

			if ((x == end_x) && (y == end_y)) {
				goto exit;
			}
			
			if (e>=0) {
				y += ydir;
				e -= ax;
				cur_t += ydir * y_dt;
			} else {
				x += xdir;
				e += ay;
				cur_t += xdir * x_dt;
			}
		}
	} else {
		e = (ax - ay) / 2;
		while (1) {
			this_cell = GetCell(x, y);
			if (PHiSquareIsBlocked(&this_cell)) {
				// this square is blocked
				PHiAddBlockage(cur_t, t_size, inMaxBlockages, ioNumBlockages, ioBlockageArray);
			}

			if ((x == end_x) && (y == end_y)) {
				goto exit;
			}
			
			if (e >= 0) {
				x += xdir;
				e -= ay;
				cur_t += xdir * x_dt;
			} else {
				y += ydir;
				e += ax;
				cur_t += ydir * y_dt;
			}
		}
	}

exit:
	UUrMemory_Block_Delete(grid);
}

static UUtUns32 PHrFindBlockages(AKtEnvironment *inEnvironment, PHtConnection *inConnection, 
								 UUtUns32 inMaxBlockages, float *inBlockageArray, UUtBool *outTotallyBlocked)
{
	UUtUns32 num_blockages;
	M3tPoint3D *p0, *p1;

	num_blockages = 0;
	p0 = PHmConnection_Point0(inEnvironment, inConnection);
	p1 = PHmConnection_Point1(inEnvironment, inConnection);

	// check for impassable parts of each grid
	PHiCheckForBlockages(inConnection->from, p0, p1, inMaxBlockages, &num_blockages, inBlockageArray);
	PHiCheckForBlockages(inConnection->to,   p0, p1, inMaxBlockages, &num_blockages, inBlockageArray);

	if ((num_blockages == 1) && (inBlockageArray[0] < 4.0f / inConnection->connection_width) &&
		(inBlockageArray[1] > 1.0f - (4.0f / inConnection->connection_width))) {
		*outTotallyBlocked = UUcTrue;
	} else {
		*outTotallyBlocked = UUcFalse;
	}

	return num_blockages;
}

void PHrProjectOntoConnection(AKtEnvironment *inEnvironment, PHtConnection *inConnection,
							  M3tPoint3D *inStartPoint, M3tPoint3D *ioTargetPoint)
{
	M3tPoint3D *p0, *p1, new_pt;
	float perp_x, perp_z, target_dist, from_dist, interp_sum, pt_val, interp0, interp1;
	float min_bound, max_bound, t_val, blockage_min, blockage_max, min_t, max_t;
	UUtBool recompute_pt;
	UUtUns32 itr, *arrayptr;

	p0 = PHmConnection_Point0(inEnvironment, inConnection);
	p1 = PHmConnection_Point1(inEnvironment, inConnection);

	// calculate a 2D perpendicular vector to this ghost quad
	perp_x = p0->z - p1->z;
	perp_z = p1->x - p0->x;

	// work out the locations of the current and target points relative to this quad
	target_dist = (ioTargetPoint->x - p0->x) * perp_x + (ioTargetPoint->z - p0->z) * perp_z;
	from_dist	= (inStartPoint->x  - p0->x) * perp_x + (inStartPoint->z  - p0->z) * perp_z;
	interp_sum = target_dist - from_dist;
	if (fabs(interp_sum) < 1e-03) {
		// the target - from line is parallel to the ghost quad, just pick a new target point that is
		// at the midpoint of the ghost quad
		MUmVector_Add(*ioTargetPoint, *p0, *p1);
		MUmVector_Scale(*ioTargetPoint, 0.5f);
	} else {
		// get the point where the target - from line intersects the ghost quad (in 2D)
		MUmVector_ScaleCopy(new_pt, -from_dist / interp_sum, *ioTargetPoint);
		MUmVector_ScaleIncrement(new_pt, target_dist / interp_sum, *inStartPoint);

		// work out whereabouts the new point is along the quad's length
		pt_val = ((new_pt.x - p0->x) * (p1->x - p0->x) + (new_pt.z - p0->z) * (p1->z - p0->z))
						/ inConnection->connection_width;

		// whereabouts along this quad can we actually pathfind through?
		min_bound = (inConnection->flags & PHcConnectionFlag_ObstructedPoint0) ? PHcComfortDistance : 0;
		max_bound = (inConnection->flags & PHcConnectionFlag_ObstructedPoint1) ? PHcComfortDistance : 0;
		max_bound = inConnection->connection_width - max_bound;

		if (max_bound < min_bound) {
			// there is nowhere in the connection where we can actually walk freely, because it is
			// obstructed and so narrow that the obstruction zones overlap. snap the waypoint to the
			// center of the connection.
			MUmVector_Add(*ioTargetPoint, *p0, *p1);
			MUmVector_Scale(*ioTargetPoint, 0.5f);

		} else {
			min_t = min_bound / inConnection->connection_width;
			max_t = max_bound / inConnection->connection_width;

			recompute_pt = UUcFalse;
			if (pt_val < min_bound) {
				pt_val = min_bound;
				recompute_pt = UUcTrue;

			} else if (pt_val > max_bound) {
				pt_val = max_bound;
				recompute_pt = UUcTrue;
			}

			if (inConnection->num_blockages > 0) {
				// check pt_val against the blockages of this ghost quad
				t_val = pt_val / inConnection->connection_width;

				arrayptr = (UUtUns32 *) UUrMemory_Array_GetMemory(ONrGameState_GetGraph()->blockage_array)
									+ inConnection->blockage_baseindex;
				for (itr = 0; itr < inConnection->num_blockages; itr++, arrayptr++) {
					blockage_min = ((float) ((*arrayptr & 0xFFFF0000) >> 16)) / UUcMaxUns16;
					blockage_max = ((float) (*arrayptr & 0x0000FFFF))		  / UUcMaxUns16;

					if ((t_val < blockage_min) || (t_val > blockage_max))
						continue;

					if (blockage_min - min_t < 0.01f) {
						// we must snap to the high side of the blockage
						t_val = blockage_max;
					} else if (max_t - blockage_max < 0.01f) {
						// we must snap to the low side of the blockage
						t_val = blockage_min;
					} else if (t_val > (blockage_min + blockage_max) / 2) {
						t_val = blockage_max;
					} else {
						t_val = blockage_min;
					}
					pt_val = t_val * inConnection->connection_width;
					recompute_pt = UUcTrue;
					break;
				}
			}

			if (recompute_pt) {
				interp1 = (pt_val / inConnection->connection_width);
				interp0 = 1.0f - (pt_val / inConnection->connection_width);

				MUmVector_ScaleCopy(new_pt, interp0, *p0);
				MUmVector_ScaleIncrement(new_pt, interp1, *p1);
			}
			MUmVector_Copy(*ioTargetPoint, new_pt);
		}
	}
}

float PHrGetConnectionHeight(PHtConnection *inConnection)
{
	AKtEnvironment *environment = ONrGameState_GetEnvironment();
	M3tPoint3D *p0, *p1;

	p0 = PHmConnection_Point0(environment, inConnection);
	p1 = PHmConnection_Point1(environment, inConnection);

	return (p0->y + p1->y) / 2;
}

void PHrNotifyDoorStateChange(PHtConnection *inConnection)
{
	UUmAssertReadPtr(inConnection, sizeof(PHtConnection));

	// flag that the dynamic grid for the two nodes that this door connects
	// must be recalculated before being used
	if (inConnection->to != NULL) {
		inConnection->to->calculate_time = 0;
	}
	if (inConnection->from != NULL) {
		inConnection->from->calculate_time = 0;
	}
}

// find a distance approximation from one point to another through the pathfinding graph
//
// inCharacter: optional (may be NULL), used for determining whether doors are passable
// inSoundType: (UUtUns32) -1 for non-sound, otherwise adds sound propagation muffle values
// inFromNode and inToNode: if NULL, will be calculated
// ioDistance: value passed in is maximum distance to look (leave zero if no max distance)
UUtError PHrPathDistance(ONtCharacter *inCharacter, UUtUns32 inSoundType,
						 M3tPoint3D *inFrom, M3tPoint3D *inTo,
						PHtNode *inFromNode, PHtNode *inToNode, float *ioDistance)
{
	AKtEnvironment			*env;
	PHtGraph				*graph;
	AKtBNVNode				*bnv_node;
	UUtBool					gotpath;
	
	env = ONrGameState_GetEnvironment();
	graph = ONrGameState_GetGraph();
	
	// set up the starting point
	if (inFromNode == NULL) {
		bnv_node = AKrNodeFromPoint(inFrom);
		if (bnv_node == NULL) {
			return UUcError_Generic;
		}

		inFromNode = PHrAkiraNodeToGraphNode(bnv_node, graph);
		if (inFromNode == NULL) {
			return UUcError_Generic;
		}
	}

	// set up the end point
	if (inToNode == NULL) {
		bnv_node = AKrNodeFromPoint(inTo);
		if (bnv_node == NULL) {
			return UUcError_Generic;
		}

		inToNode = PHrAkiraNodeToGraphNode(bnv_node, graph);
		if (inToNode == NULL) {
			return UUcError_Generic;
		}
	}

	// generate the path
	gotpath = PHrGeneratePath(graph, inCharacter, inSoundType, inFromNode, inToNode, inFrom, inTo, ioDistance, NULL, NULL);
	if (!gotpath) {
		return UUcError_Generic;
	}

	return UUcError_None;
}
