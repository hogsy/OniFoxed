
/*
	FILE:	BFW_Akira_Collision.c

	AUTHOR:	Brent H. Pease

	CREATED: October 12, 1998

	PURPOSE: environment engine

	Copyright (c) Bungie Software 1997, 2000

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_Console.h"
#include "BFW_BitVector.h"
#include "BFW_AppUtilities.h"
#include "BFW_Collision.h"
#include "BFW_Shared_Math.h"
#include "BFW_BitVector.h"
#include "BFW_Object.h"
#include "BFW_Timer.h"
#include "Oni_GameState.h"

//UUtPerformanceTimer *AKg_EndFrame_Timer = NULL;
//UUtPerformanceTimer *AKg_BBox_Visible_Timer = NULL;


#include "Akira_Private.h"

#define AKcMaxNumCheckedNodes	(4096)

// CB: changed from 512 to 2048 for mad bomber explosion
#define AKcMaxSphereStack		(2048)

#define DEBUG_COLLISION 0
#define DEBUG_COLLISION_TRAVERSAL 0

#define AKcMaxNumCheckedGQs	(AKcMaxNumCollisions * 3)

UUtBool gEnvironmentCollision = UUcTrue;

AKtCollision	AKgCollisionList[AKcMaxNumCollisions];
AKtCollision	AKgTempCollisionList[AKcMaxNumCollisions];
UUtUns16		AKgCollisionListRemap[AKcMaxNumCollisions];
UUtUns16		AKgNumCollisions;
static AUtDict	*AKgNodeTestedDict = NULL;
static AUtDict	*AKgLocalNodeTestedDict = NULL;
static AUtDict	*AKgGQTestedDict = NULL;
//extern UUtUns32 gTickNum;

UUtError AKrCollision_LevelBegin(AKtEnvironment *inEnvironment)
{
	UUtError error = UUcError_None;
	UUtUns32 numGQs = inEnvironment->gqGeneralArray->numGQs;
	UUtUns32 numLeafNodes = inEnvironment->octTree->leafNodeArray->numNodes;

	AKgNodeTestedDict = AUrDict_New(AKcMaxNumCheckedNodes, numLeafNodes);
	UUmError_ReturnOnNull(AKgNodeTestedDict);

	AKgGQTestedDict = AUrDict_New(AKcMaxNumCheckedGQs, numGQs);
	UUmError_ReturnOnNull(AKgGQTestedDict);

	AKgLocalNodeTestedDict = AUrDict_New(AKcMaxNumCheckedGQs, numLeafNodes);
	UUmError_ReturnOnNull(AKgLocalNodeTestedDict);

	return UUcError_None;
}

void AKrCollision_LevelEnd(void)
{
	if (NULL != AKgNodeTestedDict) {
		AUrDict_Dispose(AKgNodeTestedDict);
		AKgNodeTestedDict = NULL;
	}

	if (NULL != AKgGQTestedDict) {
		AUrDict_Dispose(AKgGQTestedDict);
		AKgGQTestedDict = NULL;
	}

	if (NULL != AKgLocalNodeTestedDict) {
		AUrDict_Dispose(AKgLocalNodeTestedDict);
		AKgLocalNodeTestedDict = NULL;
	}

	return;
}

static UUtBool AKiSortCompare(UUtUns16 elem1, UUtUns16 elem2)
{
	UUtBool result;
	AKtCollision *col1 = AKgTempCollisionList + elem1;
	AKtCollision *col2 = AKgTempCollisionList + elem2;

	result = col1->compare_distance > col2->compare_distance;

	return result;
}

static void AKiSortCollisions(UUtUns16 inMaxCollisions)
{
	UUtUns16 itr;

	if ((AKgNumCollisions < 2) || (0 == inMaxCollisions)) {
		UUrMemory_MoveFast(AKgTempCollisionList, AKgCollisionList, sizeof(AKtCollision) * UUmMin(AKgNumCollisions, inMaxCollisions));
	}
	else if (1 == inMaxCollisions) {
		UUtUns16 min_index;
		UUtUns32 min_distance;

		min_index = 0;
		min_distance = AKgTempCollisionList[0].compare_distance;

		for(itr = 1; itr < AKgNumCollisions; itr++)
		{
			if (AKgTempCollisionList[itr].compare_distance < min_distance) {
				min_index = itr;
				min_distance = AKgTempCollisionList[itr].compare_distance;
			}
		}

		AKgCollisionList[0] = AKgTempCollisionList[min_index];
	}
	else {
		UUmAssert(AKcMaxNumCollisions <= UUcMaxUns8);

		for(itr = 0; itr < AKgNumCollisions; itr++)
		{
			AKgCollisionListRemap[itr] = itr;
		}

		AUrQSort_16(AKgCollisionListRemap, AKgNumCollisions, AKiSortCompare);

		for(itr = 0; itr < AKgNumCollisions; itr++)
		{
			UUtUns16 fromIndex = AKgCollisionListRemap[itr];

			AKgCollisionList[itr] = AKgTempCollisionList[fromIndex];
		}
	}

	return;
}

UUtBool AKrLineOfSight(
	M3tPoint3D *inPointA,
	M3tPoint3D *inPointB)
{
	/*****************
	* Detects line of sight between two points. If
	* LOS is blocked, we return the point at which it was blocked
	* in the collisionPoint field of the first Akira collision.
	*/

	M3tVector3D sightline;
	AKtEnvironment *env;
	OBtObjectList *oblist;
	UUtUns16 obcount,i;
	const M3tBoundingSphere *sphere;
	env = ONrGameState_GetEnvironment();
	oblist = ONrGameState_GetObjectList();
	obcount = oblist->numObjects;

	// Check for environment blockage
	MUmVector_Subtract(sightline,*inPointB,*inPointA);
	if (AKrCollision_Point(env, inPointA, &sightline, AKcGQ_Flag_LOS_CanSee_Skip_AI, 1))
	{
		return UUcFalse;
	}

	// Check for objects
	for (i=0; i<obcount; i++)
	{
		const OBtObject *curObject;

		curObject = oblist->object_list + i;

		if (!(curObject->flags & OBcFlags_InUse)) continue;
		if (curObject->physics->flags & PHcFlags_Physics_Projectile) continue;

		// checking sight against spheres, which is relatively close
		sphere = &(curObject->physics->sphereTree[0].sphere);

		// collide against sphere
		if (CLrSphere_Line(inPointA,inPointB,sphere))
		{
			// This is fubar. Look away.
			AKgCollisionList[0].collisionPoint = *inPointB;
			return UUcFalse;
		}
	}

	return UUcTrue;
}

static UUtUns32
AKrFindOctTreeNodeIndex_Integer(
	AKtOctTree_InteriorNode*	inInteriorNodeArray,
	float						inPointX,
	float						inPointY,
	float						inPointZ,
	AKtOctNodeIndexCache*		ioCache)
{
	UUtUns32					curOctant;
	AKtOctTree_InteriorNode*	curIntNode;
	UUtUns32					curNodeIndex = 0;
	UUtUns32					itrs = 0;

	float						scale = 1024.f * 64.f;

	UUtInt32	in_x = MUrFloat_Round_To_Int(inPointX * scale);
	UUtInt32	in_y = MUrFloat_Round_To_Int(inPointY * scale);
	UUtInt32	in_z = MUrFloat_Round_To_Int(inPointZ * scale);
	UUtUns32	cur_dim = (UUtUns32) (AKcMaxHalfOTDim * scale * 0.5f);

	UUtInt32	cur_center_x = 0;
	UUtInt32	cur_center_y = 0;
	UUtInt32	cur_center_z = 0;

	UUmAssert(pow(2, AKcMaxTreeDepth) == scale);

	while(1)
	{
		// Grab a pointer to the current node
		curIntNode = inInteriorNodeArray + curNodeIndex;

		// We want to find which octant the end point lies in
		curOctant = 0;

		if (in_x > cur_center_x) {
			cur_center_x += cur_dim;
			curOctant |= AKcOctTree_SideBV_X;
		}
		else {
			cur_center_x -= cur_dim;
		}

		if (in_y > cur_center_y) {
			cur_center_y += cur_dim;
			curOctant |= AKcOctTree_SideBV_Y;
		}
		else {
			cur_center_y -= cur_dim;
		}

		if (in_z > cur_center_z) {
			cur_center_z += cur_dim;
			curOctant |= AKcOctTree_SideBV_Z;
		}
		else {
			cur_center_z -= cur_dim;
		}

		// Find the child
		curNodeIndex = curIntNode->children[curOctant];

		// if we are at a leaf node then break
		if (AKmOctTree_IsLeafNode(curNodeIndex)) break;

		if (++itrs >= AKcMaxTreeDepth) {
			UUmAssert("OT corrupted");
		}

		cur_dim = cur_dim >> 1;
	}

	return curNodeIndex;
}

#if defined(DEBUGGING) && DEBUGGING
#define MEASURE_CACHE			1
#else
#define MEASURE_CACHE			0
#endif

void AKrCollision_InitializeSpatialCache(
	AKtOctNodeIndexCache*		outCache)
{
	outCache->node_index = AKcOctNodeIndex_Uncached;
}

static UUtUns32
AKrFindOctTreeNodeIndex_Float(
	AKtOctTree_InteriorNode*	inInteriorNodeArray,
	float						inPointX,
	float						inPointY,
	float						inPointZ,
	AKtOctNodeIndexCache*		ioCache)
{
	UUtUns32					curOctant;
	AKtOctTree_InteriorNode*	curIntNode;
	float						curDim = AKcMaxHalfOTDim;
	UUtUns32					curNodeIndex = 0;
	float						curCenterPointX = 0;
	float						curCenterPointY = 0;
	float						curCenterPointZ = 0;
	UUtUns32					itrs = 0;
	UUtUns32					leaf;

#if MEASURE_CACHE
	static UUtUns32				nodecache_hit = 0, nodecache_miss = 0;
#endif

	if ((ioCache != NULL) && (ioCache->node_index != AKcOctNodeIndex_Uncached)) {
		float d = ioCache->node_dim;

		// we have a previous node that we should check first
		if ((fabs(inPointX - ioCache->node_center.x) < ioCache->node_dim) &&
			(fabs(inPointY - ioCache->node_center.y) < ioCache->node_dim) &&
			(fabs(inPointZ - ioCache->node_center.z) < ioCache->node_dim)) {

			// we are still inside the previous node
#if defined(DEBUGGING) && DEBUGGING
			{
				AKtOctNodeIndexCache temp_cache;
				UUtUns32 temp_index;

				// sanity check!
				AKrCollision_InitializeSpatialCache(&temp_cache);
				temp_index = AKrFindOctTreeNodeIndex_Float(inInteriorNodeArray, inPointX, inPointY, inPointZ, &temp_cache);
				UUmAssert(temp_index == ioCache->node_index);
			}
#endif

#if MEASURE_CACHE
			nodecache_hit++;
#endif
			return ioCache->node_index;
		} else {
#if MEASURE_CACHE
			nodecache_miss++;
#endif
		}
	}

	while(1)
	{
		// Grab a pointer to the current node
		curIntNode = inInteriorNodeArray + curNodeIndex;

		// We want to find which octant the end point lies in
		curOctant = 0;

		if (inPointX > curCenterPointX) {
			curOctant |= AKcOctTree_SideBV_X;
		}

		if (inPointY > curCenterPointY) {
			curOctant |= AKcOctTree_SideBV_Y;
		}

		if (inPointZ > curCenterPointZ) {
			curOctant |= AKcOctTree_SideBV_Z;
		}

		// Find the child
		curNodeIndex = curIntNode->children[curOctant];

		leaf = AKmOctTree_IsLeafNode(curNodeIndex);

		if ((!leaf) || (ioCache != NULL)) {
			// Need to go further down the tree so reduce the curDim and update the center point
			curDim *= 0.5f;
			curCenterPointX += (curOctant & AKcOctTree_SideBV_X) ? curDim : -curDim;
			curCenterPointY += (curOctant & AKcOctTree_SideBV_Y) ? curDim : -curDim;
			curCenterPointZ += (curOctant & AKcOctTree_SideBV_Z) ? curDim : -curDim;
		}

		// if we are at a leaf node then break
		if (leaf) break;

		if (++itrs >= AKcMaxTreeDepth) {
			UUmAssert("OT corrupted");
		}
	}

	if (ioCache != NULL) {
		// store the leafnode that we found
		ioCache->node_index = curNodeIndex;
		ioCache->node_dim = curDim;
		ioCache->node_center.x = curCenterPointX;
		ioCache->node_center.y = curCenterPointY;
		ioCache->node_center.z = curCenterPointZ;
	}

	return curNodeIndex;
}

UUtUns32 AKrFindOctTreeNodeIndex(
	AKtOctTree_InteriorNode*	inInteriorNodeArray,
	float						inPointX,
	float						inPointY,
	float						inPointZ,
	AKtOctNodeIndexCache*		ioCache)
{
//	UUtUns32 integer_node;
	UUtUns32 float_node;

	float_node = AKrFindOctTreeNodeIndex_Float(inInteriorNodeArray, inPointX, inPointY, inPointZ, ioCache);

	// initial integer version of find oct tree node index
	// integer_node = AKrFindOctTreeNodeIndex_Integer(inInteriorNodeArray, inPointX, inPointY, inPointZ, ioCache);
	// UUmAssert(float_node == integer_node);

	return float_node;
}



#define DEBUG_COLLISION2			0
#define DEBUG_COLLISION2_TRAVERSAL	0

UUtBool
AKrCollision_Point_SpatialCache(
	const AKtEnvironment*	inEnvironment,
	const M3tPoint3D*		ioPoint,
	M3tVector3D*			inVector,
	UUtUns32				inSkipFlags,
	UUtUns16				inNumCollisions,
	AKtOctNodeIndexCache*	ioStartCache,
	AKtOctNodeIndexCache*	ioEndCache)
{
	UUtUns32*					octTreeGQIndices;
	M3tPoint3D*					pointArray;
	M3tPlaneEquation*			planeEquArray;
	AKtOctTree_InteriorNode*	interiorNodeArray;
	AKtOctTree_LeafNode*		leafNodeArray;
	AKtQuadTree_Node*			qtInteriorNodeArray;
	AKtQuadTree_Node*			curQTIntNode;

	UUtUns32					curGQIndIndex;

	AKtGQ_General*				gqGeneralArray;
	AKtGQ_Collision*			gqCollisionArray;

	AKtGQ_Collision*			curGQCollision;

	UUtUns32					xSideFlag;
	UUtUns32					ySideFlag;
	UUtUns32					zSideFlag;
	UUtUns32					sideCrossing;

	UUtUns32					endNodeIndex;
	UUtUns32					curNodeIndex;
	AKtOctTree_LeafNode*		curOTLeafNode;
	UUtUns32					curOctant;

	M3tPoint3D					endPoint;

	float						startPointX;
	float						startPointY;
	float						startPointZ;

	float						endPointX;
	float						endPointY;
	float						endPointZ;

	float						vX;
	float						vY;
	float						vZ;

	float						invVX;
	float						invVY;
	float						invVZ;

	float						xPlane;
	float						yPlane;
	float						zPlane;
	float						d;

	float						curCenterPointU;
	float						curCenterPointV;

	float						minX;
	float						minY;
	float						minZ;
	float						maxX, maxY, maxZ;


	float						curHalfDim;
	float						curFullDim;
	float						t1, t2;

	float						curDistance;


	UUtUns32					length;


	#if defined(DEBUG_COLLISION2_TRAVERSAL) && DEBUG_COLLISION2_TRAVERSAL
		M3tBoundingBox_MinMax	bBox;
		M3tPoint3D				start, end;
		UUtBool					in;
	#endif

	UUmAssertReadPtr(inEnvironment, sizeof(AKtEnvironment));
	UUmAssertReadPtr(ioPoint, sizeof(M3tPoint3D));
	UUmAssertReadPtr(inVector, sizeof(M3tVector3D));

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(AKg_Collision_Point_Timer);
#endif

	AUrDict_Clear(AKgGQTestedDict);

	AKgNumCollisions = 0;

	pointArray = inEnvironment->pointArray->points;
	planeEquArray = inEnvironment->planeArray->planes;
	interiorNodeArray = inEnvironment->octTree->interiorNodeArray->nodes;
	leafNodeArray = inEnvironment->octTree->leafNodeArray->nodes;
	qtInteriorNodeArray = inEnvironment->octTree->qtNodeArray->nodes;
	gqGeneralArray = inEnvironment->gqGeneralArray->gqGeneral;
	gqCollisionArray = inEnvironment->gqCollisionArray->gqCollision;
	octTreeGQIndices = inEnvironment->octTree->gqIndices->indices;

	vX = inVector->x;
	vY = inVector->y;
	vZ = inVector->z;

	if (vX != 0.0f) {
		invVX = 1.0f / vX;
	}

	if (vY != 0.0f) {
		invVY = 1.0f / vY;
	}

	if (vZ != 0.0f) {
		invVZ = 1.0f / vZ;
	}

	startPointX = ioPoint->x;
	startPointY = ioPoint->y;
	startPointZ = ioPoint->z;

	// Compute the side flags
	xSideFlag = AKcOctTree_Side_NegX + (vX > 0.0f);
	ySideFlag = AKcOctTree_Side_NegY + (vY > 0.0f);
	zSideFlag = AKcOctTree_Side_NegZ + (vZ > 0.0f);

	endPoint.x = endPointX = startPointX + vX;
	endPoint.y = endPointY = startPointY + vY;
	endPoint.z = endPointZ = startPointZ + vZ;

	curHalfDim = AKcMaxHalfOTDim;

	curNodeIndex = 0;

	// CB: this could perhaps be more efficient if we never found the end octree node directly but
	// instead just looked at our t-val along the ray and compared it to 1.0...

	#if defined(DEBUG_COLLISION2) && DEBUG_COLLISION2

		UUrDebuggerMessage("finding start node...");

	#endif

	// Find the start and finish octree nodes
	curNodeIndex = AKrFindOctTreeNodeIndex(interiorNodeArray, startPointX, startPointY, startPointZ, ioStartCache);
	endNodeIndex = AKrFindOctTreeNodeIndex(interiorNodeArray, endPointX, endPointY, endPointZ, ioEndCache);

	curHalfDim = AKcMaxHalfOTDim;
	curOctant = 0xFFFFFFFF;

	// go into the start node

	while(1)
	{
		UUtUns32 oct_tree_leaf_index = AKmOctTree_GetLeafIndex(curNodeIndex);
		UUtUns32 startIndirectIndex;
		UUmAssert(AKmOctTree_IsLeafNode(curNodeIndex));

		if (0xFFFFFFFF == curNodeIndex) goto finished;

		UUmAssert(oct_tree_leaf_index < inEnvironment->octTree->leafNodeArray->numNodes);

		curOTLeafNode = leafNodeArray + oct_tree_leaf_index;

		#if defined(DEBUG_COLLISION2_TRAVERSAL) && DEBUG_COLLISION2_TRAVERSAL

			bBox.minPoint.x = curOTLeafNode->sidePlane[AKcOctTree_Side_NegX];
			bBox.minPoint.y = curOTLeafNode->sidePlane[AKcOctTree_Side_NegY];
			bBox.minPoint.z = curOTLeafNode->sidePlane[AKcOctTree_Side_NegZ];
			bBox.maxPoint.x = curOTLeafNode->sidePlane[AKcOctTree_Side_PosX];
			bBox.maxPoint.y = curOTLeafNode->sidePlane[AKcOctTree_Side_PosY];
			bBox.maxPoint.z = curOTLeafNode->sidePlane[AKcOctTree_Side_PosZ];

			start.x = startPointX;
			start.y = startPointY;
			start.z = startPointZ;
			end.x = endPointX;
			end.y = endPointY;
			end.z = endPointZ;

			in = CLrBox_Line(&bBox, &start, &end);

			UUmAssert(in);

		#endif

		// Do something with curOTLeafNode here

		if (0 == curOTLeafNode->gqIndirectIndex_Encode ) goto skip;

		AKmOctTree_DecodeGQIndIndices(curOTLeafNode->gqIndirectIndex_Encode, startIndirectIndex, length);

		for(curGQIndIndex = startIndirectIndex;
			curGQIndIndex < startIndirectIndex + length;
			curGQIndIndex++)
		{
			UUtUns32 curGQIndex;
			AKtGQ_General *curGQGeneral;
			M3tPoint3D intersection;

			curGQIndex = octTreeGQIndices[curGQIndIndex];
			curGQGeneral = gqGeneralArray + curGQIndex;

			UUmAssert(curGQIndex < inEnvironment->gqGeneralArray->numGQs);

			if (curGQGeneral->flags & inSkipFlags) { continue; }

			if (AUrDict_TestAndAdd(AKgGQTestedDict, curGQIndex)) {
				continue;
			}

			curGQCollision = gqCollisionArray + curGQIndex;

			if (
				CLrQuad_Line(
					(CLtQuadProjection)AKmGetQuadProjection(curGQGeneral->flags),
					pointArray,
					planeEquArray,
					curGQCollision->planeEquIndex,
					&curGQGeneral->m3Quad.vertexIndices,
					ioPoint,
					&endPoint,
					&intersection))
			{
				if (0 == inNumCollisions) {
					AKgNumCollisions = 1;
					goto finished;
				}

				curDistance = (float)fabs(intersection.x - startPointX);
				curDistance += (float)fabs(intersection.y - startPointY);
				curDistance += (float)fabs(intersection.z - startPointZ);

				if (AKgNumCollisions >= AKcMaxNumCollisions) goto finished;

				AKgTempCollisionList[AKgNumCollisions].float_distance = curDistance;
				AKgTempCollisionList[AKgNumCollisions].compare_distance = MUrUnsignedSmallFloat_ShiftLeft_To_Uns_Round(curDistance, 8);
				AKgTempCollisionList[AKgNumCollisions].gqIndex = curGQIndex;
				AKgTempCollisionList[AKgNumCollisions].collisionPoint = intersection;
				AKgNumCollisions++;
			}
		}

		skip:

		if ((inNumCollisions != 0) && (AKgNumCollisions >= inNumCollisions)) goto finished;

		// If we are finished then bail
		if (endNodeIndex == curNodeIndex) goto finished;

		{
			UUtUns32					dim_Encode;

			dim_Encode = curOTLeafNode->dim_Encode;
			curFullDim = AKgIntToFloatDim[(dim_Encode >> AKcOctTree_Shift_Dim) & AKcOctTree_Mask_Dim];
			maxX = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_X) & AKcOctTree_Mask_X];
			maxY = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_Y) & AKcOctTree_Mask_Y];
			maxZ = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_Z) & AKcOctTree_Mask_Z];
		}

		minX = maxX - curFullDim;
		minY = maxY - curFullDim;
		minZ = maxZ - curFullDim;

		// Find out which plane I cross
		// project onto XY plane
		xPlane = (xSideFlag == AKcOctTree_Side_PosX) ? maxX : minX;
		yPlane = (ySideFlag == AKcOctTree_Side_PosY) ? maxY : minY;
		zPlane = (zSideFlag == AKcOctTree_Side_PosZ) ? maxZ : minZ;

		t1 = vY * (xPlane - startPointX);
		t2 = vX * (yPlane - startPointY);

		if (fabs(t1) > fabs(t2)) {
			// We cross the y axis - we can eliminate the x side
			// project onto YZ plane

			t1 = vZ * (yPlane - startPointY);
			t2 = vY * (zPlane - startPointZ);

			if (fabs(t1) > fabs(t2)) {
				sideCrossing = zSideFlag;
			}
			else {
				sideCrossing = ySideFlag;
			}
		}
		else {
			// We cross the x axis - we can eliminate the y side
			// project onto XZ plane

			t1 = (vZ) * (xPlane - startPointX);
			t2 = (vX) * (zPlane - startPointZ);

			if (fabs(t1) > fabs(t2)) {
				sideCrossing = zSideFlag;
			}
			else {
				sideCrossing = xSideFlag;
			}
		}

		#if defined(DEBUG_COLLISION) && DEBUG_COLLISION

			UUrDebuggerMessage("sideCrossing: %d", sideCrossing);

		#endif

		// Lets move to the next node

		curNodeIndex = curOTLeafNode->adjInfo[sideCrossing];

		if (!AKmOctTree_IsLeafNode(curNodeIndex))
		{
			float uc, vc;

			// need to traverse quad tree

			curHalfDim = curFullDim * 0.5f;

			// First compute the intersection point
			switch(sideCrossing)
			{
				case AKcOctTree_Side_NegX:
				case AKcOctTree_Side_PosX:
					curCenterPointU = minY + curHalfDim;
					curCenterPointV = minZ + curHalfDim;
					d = invVX * (xPlane - startPointX);
					uc = startPointY + vY * d;
					vc = startPointZ + vZ * d;
					break;

				case AKcOctTree_Side_NegY:
				case AKcOctTree_Side_PosY:
					curCenterPointU = minX + curHalfDim;
					curCenterPointV = minZ + curHalfDim;
					d = invVY * (yPlane - startPointY);
					uc = startPointX + vX * d;
					vc = startPointZ + vZ * d;
					break;

				case AKcOctTree_Side_NegZ:
				case AKcOctTree_Side_PosZ:
					curCenterPointU = minX + curHalfDim;
					curCenterPointV = minY + curHalfDim;
					d = invVZ * (zPlane - startPointZ);
					uc = startPointX + vX * d;
					vc = startPointY + vY * d;
					break;

				default:
					UUmAssert(0);
			}

			// next traverse the tree
			while(1)
			{
				curQTIntNode = qtInteriorNodeArray + curNodeIndex;

				curOctant = 0;
				if (uc > curCenterPointU) curOctant |= AKcQuadTree_SideBV_U;
				if (vc > curCenterPointV) curOctant |= AKcQuadTree_SideBV_V;

				curNodeIndex = curQTIntNode->children[curOctant];

				if (AKmOctTree_IsLeafNode(curNodeIndex)) break;
				curHalfDim *= 0.5f;
				curCenterPointU += (curOctant & AKcQuadTree_SideBV_U) ? curHalfDim : -curHalfDim;
				curCenterPointV += (curOctant & AKcQuadTree_SideBV_V) ? curHalfDim : -curHalfDim;
			}
		}
	}

finished:

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(AKg_Collision_Point_Timer);
#endif

	// sort the collisions
	if (AKgNumCollisions > 0) {
		AKiSortCollisions(inNumCollisions);
	}

	return (UUtBool)(AKgNumCollisions > 0);
}

UUtBool
AKrCollision_Point(
	const AKtEnvironment*	inEnvironment,
	const M3tPoint3D*		ioPoint,
	M3tVector3D*			inVector,
	UUtUns32				inSkipFlags,
	UUtUns16				inNumCollisions)
{
	static AKtOctNodeIndexCache global_temporary_cache;

	// we have no information about where this point is in the octree... however we can
	// try to use the start point's node as a cache for the end point
	AKrCollision_InitializeSpatialCache(&global_temporary_cache);
	return AKrCollision_Point_SpatialCache(inEnvironment, ioPoint, inVector, inSkipFlags,
							inNumCollisions, &global_temporary_cache, &global_temporary_cache);
}

#define BRENTS_CHEESY_COL_PROFILE	0

#if BRENTS_CHEESY_COL_PROFILE

	UUtUns32	gCol_Sphere_Func_Num			= 0;
	UUtUns32	gCol_Sphere_Func_Time			= 0;

	UUtUns32	gCol_Sphere_FindingLeaf_Time	= 0;
	UUtUns32	gCol_Sphere_FindingGQs_Time		= 0;

	UUtUns32	gCol_Sphere_GQChecks			= 0;
	UUtUns32	gCol_Sphere_GQHits				= 0;
	UUtUns32	gCol_Sphere_QuadSphereVector_Time	= 0;

	UUtUns32	gCol_Sphere_LeafNodes_Num			= 0;
	UUtUns32	gCol_Sphere_SphereChecks_Num		= 0;

#endif


UUtBool
AKrCollision_Sphere(
	const AKtEnvironment*		inEnvironment,
	const M3tBoundingSphere*	ioSphere,
	M3tVector3D*				inVector,
	UUtUns32					inSkipFlags,

	UUtUns16					inNumCollisions)	// in: max number of quads to return, out: actual number
{
#if BRENTS_CHEESY_COL_PROFILE

	UUtInt64	funcStart;
	UUtInt64	funcQuadSphereVectorStart;
	UUtInt64	curTime;
	UUtInt64	findGQStart;

#endif

	UUtUns32*					octTreeGQIndices;
	M3tPlaneEquation*			planeEquArray;
	AKtOctTree_InteriorNode*	interiorNodeArray;
	AKtOctTree_LeafNode*		leafNodeArray;
	AKtQuadTree_Node*			qtInteriorNodeArray;
	AKtGQ_Collision*			gqCollisionArray;
	AKtGQ_General*				gqGeneral;

	AKtOctTree_LeafNode*		curLeafNode;
	AKtQuadTree_Node*			curQTIntNode;

	float						xi, yi, zi;
	float						xm, ym, zm;
	float						curt, inct;
	float						r, r2;
	float						t, dmin;

	UUtUns32					curNodeIndex;
	UUtUns32					curLeafIndex;

	UUtUns32					leafTOS;
	UUtUns32					leafStack[AKcMaxSphereStack];

	float						minX, minY, minZ;
	float						maxX, maxY, maxZ;
	float						curX, curY, curZ;

	UUtUns32					dim_Encode;
	//float						curHalfDim;
	float						curFullDim;

	UUtBool						done;

	UUtUns32					itr;

	UUtUns32					curGQIndIndex;

	UUtUns32					startIndirectIndex;
	UUtUns32					length;
	UUtUns32					curGQIndex;
	float						curDistance;
	M3tPoint3D					intersection;


	M3tPoint3D					endPoint;
	float						vectorLength;

	UUmAssertReadPtr(inEnvironment, sizeof(*inEnvironment));
	UUmAssertReadPtr(ioSphere, sizeof(*ioSphere));
	UUmAssertReadPtr(inVector, sizeof(*inVector));
	UUmAssert(ioSphere->radius >= 0);

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(AKg_Collision_Sphere_Timer);
#endif

	#if BRENTS_CHEESY_COL_PROFILE

		gCol_Sphere_Func_Num++;
		funcStart = UUrMachineTime_High();

	#endif

	AKgNumCollisions = 0;
	AUrDict_Clear(AKgNodeTestedDict);

	planeEquArray = inEnvironment->planeArray->planes;
	interiorNodeArray = inEnvironment->octTree->interiorNodeArray->nodes;
	leafNodeArray = inEnvironment->octTree->leafNodeArray->nodes;
	qtInteriorNodeArray = inEnvironment->octTree->qtNodeArray->nodes;
	gqCollisionArray = inEnvironment->gqCollisionArray->gqCollision;
	gqGeneral = inEnvironment->gqGeneralArray->gqGeneral;
	octTreeGQIndices = inEnvironment->octTree->gqIndices->indices;

	xi = ioSphere->center.x;
	yi = ioSphere->center.y;
	zi = ioSphere->center.z;

	xm = inVector->x;
	ym = inVector->y;
	zm = inVector->z;

	endPoint.x = xi + xm;
	endPoint.y = yi + ym;
	endPoint.z = zi + zm;

	r = ioSphere->radius;

	vectorLength = MUrSqrt(xm * xm + ym * ym + zm * zm);

	if (vectorLength < r * 0.125f)
	{

		r += vectorLength * 0.5f;

		xi += xm * 0.5f;
		yi += ym * 0.5f;
		zi += zm * 0.5f;

		r2 = r * r;

		// go into the start node
		curNodeIndex = AKrFindOctTreeNodeIndex(interiorNodeArray, xi, yi, zi, NULL);

		// now add every adjacent sibling that is within the sphere centered around curXYZ
		leafTOS = 1;
		leafStack[0] = curNodeIndex;

		while(leafTOS > 0)
		{
			curNodeIndex = leafStack[--leafTOS];

			if (curNodeIndex == 0xFFFFFFFF) continue;

			if (!AKmOctTree_IsLeafNode(curNodeIndex))
			{
				curQTIntNode = qtInteriorNodeArray + curNodeIndex;

				for(itr = 0; itr < 4; itr++)
				{
					if (leafTOS >= AKcMaxSphereStack)
					{
						UUmAssert(!"leaf overrun");
						goto bail;
					}
					leafStack[leafTOS++] = curQTIntNode->children[itr];
				}

				continue;
			}

			curLeafIndex = AKmOctTree_GetLeafIndex(curNodeIndex);

			if (AUrDict_Test(AKgNodeTestedDict, curLeafIndex) == UUcTrue) continue;

			#if BRENTS_CHEESY_COL_PROFILE

				gCol_Sphere_SphereChecks_Num++;

			#endif

			curLeafNode = leafNodeArray + curLeafIndex;

			dim_Encode = curLeafNode->dim_Encode;
			curFullDim = AKgIntToFloatDim[(dim_Encode >> AKcOctTree_Shift_Dim) & AKcOctTree_Mask_Dim];
			maxX = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_X) & AKcOctTree_Mask_X];
			maxY = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_Y) & AKcOctTree_Mask_Y];
			maxZ = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_Z) & AKcOctTree_Mask_Z];
			minX = maxX - curFullDim;
			minY = maxY - curFullDim;
			minZ = maxZ - curFullDim;

			dmin = 0.0f;

			if (xi < minX) {
				t = xi - minX;
				t *= t;
				dmin += t;
			}
			else if (xi > maxX) {
				t = xi - maxX;
				t *= t;
				dmin += t;
			}

			if (yi < minY) {
				t = yi - minY;
				t *= t;
				dmin += t;
			}
			else if (yi > maxY) {
				t = yi - maxY;
				t *= t;
				dmin += t;
			}

			if (zi < minZ) {
				t = zi - minZ;
				t *= t;
				dmin += t;
			}
			else if (zi > maxZ) {
				t = zi - maxZ;
				t *= t;
				dmin += t;
			}

			// if we are within the sphere then push all adjacent quads that have not been visited onto the stack
			if (dmin <= r2)
			{
				AUrDict_TestAndAdd(AKgNodeTestedDict, curLeafIndex);

				for(itr = 0; itr < 6; itr++)
				{
					if (leafTOS >= AKcMaxSphereStack)
					{
						UUmAssert(!"leaf overrun");
						goto bail;
					}
					leafStack[leafTOS++] = curLeafNode->adjInfo[itr];
				}
			}
		}
	}
	else
	{
		inct = r * 1.0f / vectorLength;
		r2 = r * r;

		curt = 0.0f;
		done = UUcFalse;

		while(!done)
		{

			if (curt >= 1.0f)
			{
				curt = 1.0f;
				done = UUcTrue;
			}

			curX = xi + curt * xm;
			curY = yi + curt * ym;
			curZ = zi + curt * zm;

			AUrDict_Clear(AKgLocalNodeTestedDict);

			// go into the start node
			curNodeIndex = AKrFindOctTreeNodeIndex(interiorNodeArray, curX, curY, curZ, NULL);

			// now add every adjacent sibling that is within the sphere centered around curXYZ
			leafTOS = 1;
			leafStack[0] = curNodeIndex;

			while(leafTOS > 0)
			{
				curNodeIndex = leafStack[--leafTOS];

				if (0xFFFFFFFF == curNodeIndex) continue;

				if (!AKmOctTree_IsLeafNode(curNodeIndex)) {
					curQTIntNode = qtInteriorNodeArray + curNodeIndex;

					for(itr = 0; itr < 4; itr++)
					{
						if (leafTOS >= AKcMaxSphereStack) {
							UUmAssert(!"leaf overrun");
							goto bail;
						}
						leafStack[leafTOS++] = curQTIntNode->children[itr];
					}

					continue;
				}

				curLeafIndex = AKmOctTree_GetLeafIndex(curNodeIndex);

				if (AUrDict_Test(AKgLocalNodeTestedDict, curLeafIndex) == UUcTrue) continue;

				#if BRENTS_CHEESY_COL_PROFILE

					gCol_Sphere_SphereChecks_Num++;

				#endif

				curLeafNode = leafNodeArray + curLeafIndex;

				dim_Encode = curLeafNode->dim_Encode;
				curFullDim = AKgIntToFloatDim[(dim_Encode >> AKcOctTree_Shift_Dim) & AKcOctTree_Mask_Dim];
				maxX = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_X) & AKcOctTree_Mask_X];
				maxY = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_Y) & AKcOctTree_Mask_Y];
				maxZ = AKgIntToFloatXYZ[(dim_Encode >> AKcOctTree_Shift_Z) & AKcOctTree_Mask_Z];
				minX = maxX - curFullDim;
				minY = maxY - curFullDim;
				minZ = maxZ - curFullDim;

				dmin = 0.0f;
				if (curX < minX) {
					t = curX - minX;
					t *= t;
					dmin += t;
				}
				else if (curX > maxX) {
					t = curX - maxX;
					t *= t;
					dmin += t;
				}

				if (curY < minY) {
					t = curY - minY;
					t *= t;
					dmin += t;
				}
				else if (curY > maxY) {
					t = curY - maxY;
					t *= t;
					dmin += t;
				}

				if (curZ < minZ) {
					t = curZ - minZ;
					t *= t;
					dmin += t;
				}
				else if (curZ > maxZ) {
					t = curZ - maxZ;
					t *= t;
					dmin += t;
				}

				// if we are within the sphere then push all adjacent quads that have not been visited onto the stack
				if (dmin <= r2)
				{
					AUrDict_TestAndAdd(AKgNodeTestedDict, curLeafIndex);
					AUrDict_TestAndAdd(AKgLocalNodeTestedDict, curLeafIndex);

					for(itr = 0; itr < 6; itr++)
					{
						if (leafTOS >= AKcMaxSphereStack)
						{
							UUmAssert(!"leaf overrun");
							goto bail;
						}
						leafStack[leafTOS++] = curLeafNode->adjInfo[itr];
					}
				}
			}

			curt += inct;
		}
	}

bail:

	#if BRENTS_CHEESY_COL_PROFILE

		findGQStart = UUrMachineTime_High();
		gCol_Sphere_FindingLeaf_Time += findGQStart - funcStart;

	#endif

	AUrDict_Clear(AKgGQTestedDict);

	#if BRENTS_CHEESY_COL_PROFILE

		gCol_Sphere_LeafNodes_Num += AKgNodeTestedDict->numPages;

	#endif

	// Now traverse all the intersected leaf nodes
	for(itr = 0; itr < AKgNodeTestedDict->numPages; itr++)
	{
		curLeafIndex = AKgNodeTestedDict->pages[itr];
		curLeafNode = leafNodeArray + curLeafIndex;

		//AUrDict_TestAndAdd(AKgDebugLeafNodes, curLeafIndex);

		if (curLeafNode->gqIndirectIndex_Encode == 0) continue;

		AKmOctTree_DecodeGQIndIndices(curLeafNode->gqIndirectIndex_Encode, startIndirectIndex, length);

		for(curGQIndIndex = startIndirectIndex;
			curGQIndIndex < startIndirectIndex + length;
			curGQIndIndex++)
		{
			curGQIndex = octTreeGQIndices[curGQIndIndex];
			UUmAssert(curGQIndex < inEnvironment->gqGeneralArray->numGQs);

			if (gqGeneral[curGQIndex].flags & inSkipFlags) { continue; }

			// check this gq if it is not in our dictionary

			if (!AUrDict_TestAndAdd(AKgGQTestedDict, curGQIndex))
			{
				UUtBool touchedThisGQ;

				#if BRENTS_CHEESY_COL_PROFILE

					gCol_Sphere_GQChecks++;
					funcQuadSphereVectorStart = UUrMachineTime_High();

				#endif

				touchedThisGQ =
					AKrGQ_SphereVector(
						inEnvironment,
						curGQIndex,
						ioSphere,
						inVector,
						&intersection);

				#if BRENTS_CHEESY_COL_PROFILE

					gCol_Sphere_QuadSphereVector_Time += UUrMachineTime_High() - funcQuadSphereVectorStart;

				#endif

				if (touchedThisGQ)
				{
					#if BRENTS_CHEESY_COL_PROFILE

						gCol_Sphere_GQHits++;

					#endif

					//inEnvironment->gqGeneralArray->gqGeneral[curGQIndex].flags |= AKcGQ_Flag_Draw_Flash;

					if (inNumCollisions == 0)
					{
						AKgNumCollisions = 1;
						goto finished;
					}

					if (AKgNumCollisions >= AKcMaxNumCollisions) goto finished;

					curDistance = MUrPoint_Distance(&intersection,&ioSphere->center);

					AKgTempCollisionList[AKgNumCollisions].float_distance = curDistance;
					AKgTempCollisionList[AKgNumCollisions].compare_distance = MUrUnsignedSmallFloat_ShiftLeft_To_Uns_Round(curDistance, 8);
					AKgTempCollisionList[AKgNumCollisions].gqIndex = curGQIndex;
					AKgTempCollisionList[AKgNumCollisions].collisionPoint = intersection;
					AKgTempCollisionList[AKgNumCollisions].collisionType = CLcCollision_Face;
					AKmPlaneEqu_GetComponents(
						gqCollisionArray[curGQIndex].planeEquIndex,planeEquArray,
						AKgTempCollisionList[AKgNumCollisions].plane.a,
						AKgTempCollisionList[AKgNumCollisions].plane.b,
						AKgTempCollisionList[AKgNumCollisions].plane.c,
						AKgTempCollisionList[AKgNumCollisions].plane.d);
					AKgNumCollisions++;
				}
			}
		}
	}

finished:

	#if BRENTS_CHEESY_COL_PROFILE

		//ggCol_Sphere_MainWhile_Time += UUrMachineTime_High() - funcWhileLoopStart;

	#endif

	// sort the collisions
	if (AKgNumCollisions > 0) {
		AKiSortCollisions(inNumCollisions);
	}

	#if BRENTS_CHEESY_COL_PROFILE

		curTime = UUrMachineTime_High();

		gCol_Sphere_Func_Time += curTime - funcStart;
		gCol_Sphere_FindingGQs_Time += curTime - findGQStart;

	#endif

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(AKg_Collision_Sphere_Timer);
#endif

	return (UUtBool)(AKgNumCollisions > 0);
}

#define BRENTS_CHEESY_SPHEREVECTOR_PROFILE	0

#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

	UUtUns32	gSphereVec_Func_Num		= 0;
	UUtUns32	gSphereVec_Func_Time	= 0;

	UUtUns32	gSphereVec_TrivReject_SameSide	= 0;
	UUtUns32	gSphereVec_TrivReject_Box	= 0;

	UUtUns32	gSphereVec_Accept_PointInQuad_Start	= 0;
	UUtUns32	gSphereVec_Accept_PointInQuad_End	= 0;

	UUtUns32	gSphereVec_Accept_QuadLine	= 0;

	UUtUns32	gSphereVec_Accept_VertexInSphere_Start	= 0;
	UUtUns32	gSphereVec_Accept_VertexInSphere_End	= 0;

	UUtUns32	gSphereVec_Accept_DistEdge	= 0;

	UUtUns32	gSphereVec_Accept_Edge_Start = 0;
	UUtUns32	gSphereVec_Accept_Edge_End = 0;

	UUtUns32	gSphereVec_Accept_Edge_Edge = 0;

	UUtUns32	gSphereVec_Reject_Total = 0;
	UUtUns32	gSphereVec_Accept_Total = 0;

#endif

UUtBool
AKrGQ_SphereVector(
	const AKtEnvironment*		inEnvironment,
	UUtUns32					inGQIndex,
	const M3tBoundingSphere*	inSphere,
	const M3tVector3D*			inVector,
	M3tPoint3D					*outIntersection)	// NULL if you don't care
{
#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

	UUtInt64	funcStart;

#endif

	const AKtGQ_Collision	*gqCollision;
	const AKtGQ_General		*gqGeneral;
	const M3tPoint3D		*pointArray;
	const M3tPlaneEquation	*planeArray;
	const M3tQuad			*quad;

	UUtBool					result;

	float					startDist, endDist;
	M3tPoint3D				Pop;
	M3tPoint3D				endPoint;

	UUtUns32	i;

	float sphere_center_x;
	float sphere_center_y;
	float sphere_center_z;

	float vector_x;
	float vector_y;
	float vector_z;

	float vector_length_squared;

	float px, py, pz;

	float distSquared;
	float rSquared;
	float t;
	float one_over_vector_length_squared;

	float radius;

	float minX, minY, minZ;
	float maxX, maxY, maxZ;

	float vector_dot_sphere_center;
	const M3tPlaneEquation *planeEqu;


#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(AKg_GQ_SphereVector_Timer);
#endif


	//UUtBool	wouldHaveBailed = UUcFalse;

	UUmAssertReadPtr(inEnvironment, sizeof(AKtEnvironment));
	UUmAssertReadPtr(inVector, sizeof(M3tVector3D));

	gqCollision = inEnvironment->gqCollisionArray->gqCollision + inGQIndex;
	gqGeneral = inEnvironment->gqGeneralArray->gqGeneral + inGQIndex;
	pointArray = inEnvironment->pointArray->points;
	planeArray = inEnvironment->planeArray->planes;
	quad = &gqGeneral->m3Quad.vertexIndices;


	#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

		gSphereVec_Func_Num++;
		funcStart = UUrMachineTime_High();

	#endif


#if 0
	CLrQuad_Line(
		AKmGetQuadProjection(gqGeneral->flags),
		pointArray, planeArray,
		gqCollision->planeEquIndex,
		&gqGeneral->m3Quad.vertexIndices,
		start_point,
		end_point,
#endif

	sphere_center_x = inSphere->center.x;
	sphere_center_y = inSphere->center.y;
	sphere_center_z = inSphere->center.z;

	endPoint.x = sphere_center_x + inVector->x;
	endPoint.y = sphere_center_y + inVector->y;
	endPoint.z = sphere_center_z + inVector->z;

	radius = inSphere->radius;

	// First check if we are far away from the plane
	{
		planeEqu = planeArray + (gqCollision->planeEquIndex & 0x7FFFFFFF);

		// check for the start and end point within radius distance of the quad
		startDist = planeEqu->a * sphere_center_x + planeEqu->b * sphere_center_y + planeEqu->c * sphere_center_z + planeEqu->d;
		endDist = planeEqu->a * endPoint.x + planeEqu->b * endPoint.y + planeEqu->c * endPoint.z + planeEqu->d;
	}

	// to intersect we must cross the plane or intersect at the start or end of the line segment
	if (((startDist > radius) && (endDist > radius)) || ((startDist < -radius) && (endDist < -radius))) {
		#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE
			gSphereVec_TrivReject_SameSide++;
		#endif

		result = UUcFalse;
		goto exit;
	}

	// check min max box
	minX = gqCollision->bBox.minPoint.x;
	maxX = gqCollision->bBox.maxPoint.x;
	minY = gqCollision->bBox.minPoint.y;
	maxY = gqCollision->bBox.maxPoint.y;
	minZ = gqCollision->bBox.minPoint.z;
	maxZ = gqCollision->bBox.maxPoint.z;

	UUmAssert(minX <= maxX);
	UUmAssert(minY <= maxY);
	UUmAssert(minZ <= maxZ);

	minX -= radius + 0.001f;
	maxX += radius + 0.001f;
	minY -= radius + 0.001f;
	maxY += radius + 0.001f;
	minZ -= radius + 0.001f;
	maxZ += radius + 0.001f;

	if (
		(sphere_center_x < minX && endPoint.x < minX) ||
		(sphere_center_x > maxX && endPoint.x > maxX) ||
		(sphere_center_y < minY && endPoint.y < minY) ||
		(sphere_center_y > maxY && endPoint.y > maxY) ||
		(sphere_center_z < minZ && endPoint.z < minZ) ||
		(sphere_center_z > maxZ && endPoint.z > maxZ)
		)
	{
		#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

			gSphereVec_TrivReject_Box++;

		#endif

		//wouldHaveBailed = UUcTrue;

		result = UUcFalse;
		goto exit;
	}

	if ((float)fabs(startDist) <= radius) {
		Pop.x = sphere_center_x - startDist * planeEqu->a;
		Pop.y = sphere_center_y - startDist * planeEqu->b;
		Pop.z = sphere_center_z - startDist * planeEqu->c;

		if (CLrQuad_PointInQuad((CLtQuadProjection)AKmGetQuadProjection(gqGeneral->flags), pointArray, quad, &Pop)) {
			if (outIntersection != NULL) {
				*outIntersection = Pop;
			}

			#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

				gSphereVec_Accept_PointInQuad_Start++;

			#endif

			//UUmAssert(wouldHaveBailed == UUcFalse);
			result = UUcTrue;
			goto exit;
		}
	}

	if ((float)fabs(endDist) <= radius)
	{
		Pop.x = endPoint.x - endDist * planeEqu->a;
		Pop.y = endPoint.y - endDist * planeEqu->b;
		Pop.z = endPoint.z - endDist * planeEqu->c;

		if (CLrQuad_PointInQuad((CLtQuadProjection)AKmGetQuadProjection(gqGeneral->flags), pointArray, quad, &Pop)) {
			if (outIntersection != NULL) {
				*outIntersection = Pop;
			}

			#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

				gSphereVec_Accept_PointInQuad_End++;

			#endif

			//UUmAssert(wouldHaveBailed == UUcFalse);
			result = UUcTrue;
			goto exit;
		}
	}

	// Calculate some useful constants
	vector_x = inVector->x;
	vector_y = inVector->y;
	vector_z = inVector->z;

	vector_length_squared = vector_x * vector_x + vector_y * vector_y + vector_z * vector_z;
	vector_dot_sphere_center = vector_x * sphere_center_x + vector_y * sphere_center_y + vector_z * sphere_center_z;

	one_over_vector_length_squared = 1.0f / vector_length_squared;

	rSquared = radius * radius;

	// Next check the distance of each vertex to the line of the sphere vector
	for(i = 0; i < 4; i++)
	{
		const M3tPoint3D*		pointC;
		float Cx, Cy, Cz;

		pointC = pointArray + quad->indices[i];
		Cx = pointC->x;
		Cy = pointC->y;
		Cz = pointC->z;

		distSquared = UUmSQR(Cx - sphere_center_x);
		distSquared += UUmSQR(Cy - sphere_center_y);
		distSquared += UUmSQR(Cz - sphere_center_z);

		if (distSquared <= rSquared)
		{
			#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

				gSphereVec_Accept_VertexInSphere_Start++;

			#endif


			if (outIntersection != NULL)
			{
				*outIntersection = *pointC;
			}

			//UUmAssert(wouldHaveBailed == UUcFalse);
			result = UUcTrue;
			goto exit;
		}

		distSquared = UUmSQR(Cx - endPoint.x);
		distSquared += UUmSQR(Cy - endPoint.y);
		distSquared += UUmSQR(Cz - endPoint.z);

		if (distSquared <= rSquared)
		{
			#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

				gSphereVec_Accept_VertexInSphere_End++;

			#endif

			//UUmAssert(wouldHaveBailed == UUcFalse);
			if (outIntersection != NULL)
			{
				*outIntersection = *pointC;
			}

			result = UUcTrue;
			goto exit;
		}

		t = -(vector_dot_sphere_center - vector_x * Cx - vector_y * Cy - vector_z * Cz) * one_over_vector_length_squared;
		if (t >= 0.0f && t <= 1.0f)
		{
			distSquared = UUmSQR(sphere_center_x + vector_x * t - Cx);
			distSquared += UUmSQR(sphere_center_y + vector_y * t - Cy);
			distSquared += UUmSQR(sphere_center_z + vector_z * t - Cz);

			if (distSquared <= rSquared)
			{
				if (outIntersection != NULL)
				{
					*outIntersection = *pointC;
				}

				#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

					gSphereVec_Accept_DistEdge++;

				#endif

				//UUmAssert(wouldHaveBailed == UUcFalse);
				result = UUcTrue;
				goto exit;
			}
		}
	}

	// Next check the distance of the closest point between each quad
	// edge and the cylinder axis to see if its less then the radius

	for(i = 0; i < 4; i++)
	{
		const M3tPoint3D*		pointC;
		const M3tPoint3D*		pointD;

		float Cx, Cy, Cz;
		float CDx, CDy, CDz;

		float one_over_length_of_cd_squared;
		float CD_dot_C;

		pointC = pointArray + quad->indices[i];
		pointD = pointArray + quad->indices[(i + 1) % 4];

		Cx = pointC->x;
		Cy = pointC->y;
		Cz = pointC->z;

		CDx = pointD->x - Cx;
		CDy = pointD->y - Cy;
		CDz = pointD->z - Cz;

		one_over_length_of_cd_squared = 1.0f / (CDx * CDx + CDy * CDy + CDz * CDz);
		CD_dot_C = CDx * Cx + CDy * Cy + CDz * Cz;

		// Check the distance from the startPoint to the edge
		t = -(CD_dot_C - CDx * sphere_center_x - CDy * sphere_center_y - CDz * sphere_center_z) * one_over_length_of_cd_squared;
		if (t >= 0.0f && t <= 1.0f)
		{
			px = Cx + CDx * t;
			distSquared = UUmSQR(px - sphere_center_x);

			py = Cy + CDy * t;
			distSquared += UUmSQR(py - sphere_center_y);

			pz = Cz + CDz * t;
			distSquared += UUmSQR(pz - sphere_center_z);

			if (distSquared <= rSquared) {
				if (outIntersection != NULL) {
					outIntersection->x = px;
					outIntersection->y = py;
					outIntersection->z = pz;
				}

				#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

					gSphereVec_Accept_Edge_Start++;

				#endif

				//UUmAssert(wouldHaveBailed == UUcFalse);
				result = UUcTrue;
				goto exit;
			}
		}

		// Check the distance from the endPoint to the edge
		t = -(CD_dot_C - CDx * endPoint.x - CDy * endPoint.y - CDz * endPoint.z)  * one_over_length_of_cd_squared;
		if (t >= 0.0f && t <= 1.0f)
		{
			px = Cx + CDx * t;
			distSquared = UUmSQR(px - endPoint.x);

			py = Cy + CDy * t;
			distSquared += UUmSQR(py - endPoint.y);

			pz = Cz + CDz * t;
			distSquared += UUmSQR(pz - endPoint.z);

			if (distSquared <= rSquared)
			{
				if (outIntersection != NULL)
				{
					outIntersection->x = px;
					outIntersection->y = py;
					outIntersection->z = pz;
				}

				#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

					gSphereVec_Accept_Edge_End++;

				#endif

				//UUmAssert(wouldHaveBailed == UUcFalse);
				result = UUcTrue;
				goto exit;
			}
		}

		// This is from mathematica
		t = -(vector_dot_sphere_center - sphere_center_x * CDx - sphere_center_y * CDy - sphere_center_z * CDz - vector_x * Cx + CDx * Cx -
					vector_y * Cy + CDy * Cy - vector_z * Cz + CDz * Cz) /
    		(vector_length_squared - 2 * vector_x * CDx + CDx * CDx - 2 * vector_y * CDy + CDy * CDy -
    				2 * vector_z * CDz + CDz * CDz);

		if (t >= 0.0f && t <= 1.0f) {
    		px = (Cx + CDx * t);
    		py = (Cy + CDy * t);
    		pz = (Cz + CDz * t);

    		distSquared = UUmSQR((sphere_center_x + vector_x * t) - px);
    		distSquared += UUmSQR((sphere_center_y + vector_y * t) - py);
			distSquared += UUmSQR((sphere_center_z + vector_z * t) - pz);

			if (distSquared <= rSquared) {
				if (outIntersection != NULL) {
    				outIntersection->x = px;
    				outIntersection->y = py;
    				outIntersection->z = pz;
    			}

				#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

					gSphereVec_Accept_Edge_Edge++;

				#endif

				//UUmAssert(wouldHaveBailed == UUcFalse);
    			result = UUcTrue;
				goto exit;
    		}
    	}
	}

	// next try line quad for cylinder center
	if (CLrQuad_Line(
			(CLtQuadProjection)AKmGetQuadProjection(gqGeneral->flags),
			pointArray,
			inEnvironment->planeArray->planes,
			gqCollision->planeEquIndex,
			quad,
			&inSphere->center,
			&endPoint,
			outIntersection))
	{
		#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

			gSphereVec_Accept_QuadLine++;

		#endif

		//UUmAssert(wouldHaveBailed == UUcFalse);
		result = UUcTrue;
		goto exit;
	}


	result = UUcFalse;

exit:

	//if (wouldHaveBailed) UUmAssert(result == UUcFalse);

	#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE

		gSphereVec_Func_Time += UUrMachineTime_High() - funcStart;

		if (result) {
			gSphereVec_Accept_Total++;
		}
		else {
			gSphereVec_Reject_Total++;
		}

	#endif

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(AKg_GQ_SphereVector_Timer);
#endif

	return result;
}

void
AKrEnvironment_Collision_PrintStats(
	void)
{
#if SUPPORT_DEBUG_FILES
	BFtDebugFile*	statsFile = NULL;
#if BRENTS_CHEESY_COL_PROFILE
	statsFile = BFrDebugFile_Open("collisionStats.txt");

	if (statsFile == NULL) return;

	BFrDebugFile_Printf(statsFile, "time per func: %f"UUmNL, (float)gCol_Sphere_Func_Time / (float)gCol_Sphere_Func_Num);
	BFrDebugFile_Printf(statsFile, "time in findLeafNode per total: %f"UUmNL, (float)gCol_Sphere_FindingLeaf_Time / (float)gCol_Sphere_Func_Time);
	BFrDebugFile_Printf(statsFile, "time in findGQ per total: %f"UUmNL, (float)gCol_Sphere_FindingGQs_Time / (float)gCol_Sphere_Func_Time);
	BFrDebugFile_Printf(statsFile, "time in QSV per total: %f"UUmNL, (float)gCol_Sphere_QuadSphereVector_Time / (float)gCol_Sphere_Func_Time);
	BFrDebugFile_Printf(statsFile, UUmNL);

	BFrDebugFile_Printf(statsFile, "SphereChecks per func: %f"UUmNL, (float)gCol_Sphere_SphereChecks_Num / (float)gCol_Sphere_Func_Num);
	BFrDebugFile_Printf(statsFile, "LeafNodes per func: %f"UUmNL, (float)gCol_Sphere_LeafNodes_Num / (float)gCol_Sphere_Func_Num);
	BFrDebugFile_Printf(statsFile, UUmNL);

	BFrDebugFile_Printf(statsFile, "GQs checked per func: %f"UUmNL, (float)gCol_Sphere_GQChecks / (float)gCol_Sphere_Func_Num);
	BFrDebugFile_Printf(statsFile, "GQs hit per func: %f"UUmNL, (float)gCol_Sphere_GQHits / (float)gCol_Sphere_Func_Num);
	BFrDebugFile_Printf(statsFile, "GQs hit ration: %f"UUmNL, (float)gCol_Sphere_GQHits / (float)gCol_Sphere_GQChecks);
	BFrDebugFile_Printf(statsFile, UUmNL);

	BFrDebugFile_Close(statsFile);
#endif

#if BRENTS_CHEESY_SPHEREVECTOR_PROFILE
	statsFile = BFrDebugFile_Open("sphereVectorStats.txt");

	if (statsFile == NULL) return;

	BFrDebugFile_Printf(statsFile, "funcNum\t%d"UUmNL, gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "funcTime\t%d"UUmNL, gSphereVec_Func_Time);
	BFrDebugFile_Printf(statsFile, "time per func\t%f"UUmNL, (float)gSphereVec_Func_Time / (float)gSphereVec_Func_Num);

	BFrDebugFile_Printf(statsFile, "Reject_Total\t%d"UUmNL, gSphereVec_Reject_Total);
	BFrDebugFile_Printf(statsFile, "Accept_Total\t%d"UUmNL, gSphereVec_Accept_Total);
	BFrDebugFile_Printf(statsFile, "TrivReject SameSide\t%d"UUmNL, gSphereVec_TrivReject_SameSide);
	BFrDebugFile_Printf(statsFile, "TrivReject Box\t%d"UUmNL, gSphereVec_TrivReject_Box);
	BFrDebugFile_Printf(statsFile, "Accept_PointInQuad_Start\t%d"UUmNL, gSphereVec_Accept_PointInQuad_Start);
	BFrDebugFile_Printf(statsFile, "Accept_PointInQuad_End\t%d"UUmNL, gSphereVec_Accept_PointInQuad_End);
	BFrDebugFile_Printf(statsFile, "Accept_QuadLine\t%d"UUmNL, gSphereVec_Accept_QuadLine);
	BFrDebugFile_Printf(statsFile, "Accept_VertexInSphere_Start\t%d"UUmNL, gSphereVec_Accept_VertexInSphere_Start);
	BFrDebugFile_Printf(statsFile, "Accept_VertexInSphere_End\t%d"UUmNL, gSphereVec_Accept_VertexInSphere_End);
	BFrDebugFile_Printf(statsFile, "Accept_DistEdge\t%d"UUmNL, gSphereVec_Accept_DistEdge);
	BFrDebugFile_Printf(statsFile, "Accept_Edge_Start\t%d"UUmNL, gSphereVec_Accept_Edge_Start);
	BFrDebugFile_Printf(statsFile, "Accept_Edge_End\t%d"UUmNL, gSphereVec_Accept_Edge_End);
	BFrDebugFile_Printf(statsFile, "Accept_Edge_Edge\t%d"UUmNL, gSphereVec_Accept_Edge_Edge);

	BFrDebugFile_Printf(statsFile, "Reject_Total per func\t%f"UUmNL, (float)gSphereVec_Reject_Total / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "Accept_Total per func\t%f"UUmNL, (float)gSphereVec_Accept_Total / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "TrivReject_SameSide per func\t%f"UUmNL, (float)gSphereVec_TrivReject_SameSide / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "TrivReject_Box per func\t%f"UUmNL, (float)gSphereVec_TrivReject_Box / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "Accept_PointInQuad_Start per func\t%f"UUmNL, (float)gSphereVec_Accept_PointInQuad_Start / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "Accept_PointInQuad_End per func\t%f"UUmNL, (float)gSphereVec_Accept_PointInQuad_End / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "Accept_QuadLine per func\t%f"UUmNL, (float)gSphereVec_Accept_QuadLine / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "Accept_VertexInSphere_Start per func\t%f"UUmNL, (float)gSphereVec_Accept_VertexInSphere_Start / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "Accept_VertexInSphere_End per func\t%f"UUmNL, (float)gSphereVec_Accept_VertexInSphere_End / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "Accept_DistEdge per func\t%f"UUmNL, (float)gSphereVec_Accept_DistEdge / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "Accept_Edge_Start per func\t%f"UUmNL, (float)gSphereVec_Accept_Edge_Start / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "Accept_Edge_End per func\t%f"UUmNL, (float)gSphereVec_Accept_Edge_End / (float)gSphereVec_Func_Num);
	BFrDebugFile_Printf(statsFile, "Accept_Edge_Edge per func\t%f"UUmNL, (float)gSphereVec_Accept_Edge_Edge / (float)gSphereVec_Func_Num);

	BFrDebugFile_Close(statsFile);
#endif
#endif
}

