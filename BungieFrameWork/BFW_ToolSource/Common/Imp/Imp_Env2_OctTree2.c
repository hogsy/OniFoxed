/*
	FILE:	Imp_Env2_OctTree2.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 3, 1997

	PURPOSE:

	Copyright 1997

*/

#include <stdio.h>
#include <math.h>

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_Motoko.h"
#include "BFW_AppUtilities.h"
#include "BFW_TM_Construction.h"
#include "BFW_MathLib.h"
#include "BFW_LSSolution.h"
#include "BFW_Collision.h"
#include "BFW_Shared_TriRaster.h"
#include "BFW_FileFormat.h"
#include "BFW_BitVector.h"
#include "BFW_Timer.h"

#include "Imp_Env2_Private.h"
#include "Imp_Common.h"

UUtUns32	gGQ_NumRemapped;
UUtUns32*	gGQ_RemapArray;
UUtUns32*	gGQ_RemapArray_CreateGQList;

UUtUns32*	IMPgOctree2_StackBase;
UUtUns32*	IMPgOctree2_StackMax;

extern float			AKgIntToFloatDim[];
extern float			AKgIntToFloatXYZ[];

// This is used to convert from an oct tree to a quad tree
// gOct2QuadTree[axis][side][itr]
UUtUns32	gOct2QuadTree[3][2][4] =
{
	{
		{0, 2, 4, 6},	//uv0 -> 000 010 100 110
		{1, 3, 5, 7}	//uv1 -> 001 011 101 111
	},
	{
		{0, 1, 4, 5},	//u0v -> 000 001 100 101
		{2, 3, 6, 7}	//u1v -> 010 011 110 111
	},
	{
		{0, 1, 2, 3},	//0uv -> 000 001 010 011
		{4, 5, 6, 7}	//1uv -> 100 101 110 111
	}
};

static UUtError
IMPiEnv_QuadTree_Build(
	IMPtEnv_BuildData*		inBuildData,
	IMPtEnv_OTData*			inOTData,
	UUtUns32*				inOct2QuadTree,
	UUtUns32				inOTNodeIndex,
	UUtUns32				*outQuadTreeIndex)
{
	UUtError					error;
	UUtUns32					newQTNodeIndex;
	AKtQuadTree_Node*			qtNode;
	UUtUns32					itr;
	AKtOctTree_InteriorNode*	otInteriorNode;

	if(AKmOctTree_IsLeafNode(inOTNodeIndex))
	{
		*outQuadTreeIndex = inOTNodeIndex;

		return UUcError_None;
	}

	// create a new interior quad tree node
		if(inOTData->nextQTNodeIndex >= IMPcEnv_MaxQTNodes)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "out of interior nodes");
		}

		newQTNodeIndex = inOTData->nextQTNodeIndex++;

		qtNode = inOTData->qtNodes + newQTNodeIndex;

		otInteriorNode = inOTData->interiorNodes + inOTNodeIndex;

		for(itr = 0; itr < 4; itr++)
		{
			error =
				IMPiEnv_QuadTree_Build(
					inBuildData,
					inOTData,
					inOct2QuadTree,
					otInteriorNode->children[inOct2QuadTree[itr]],
					&qtNode->children[itr]);
			UUmError_ReturnOnError(error);
		}

		*outQuadTreeIndex = newQTNodeIndex;

	return UUcError_None;
}

static void
IMPiEnv_OctTree_Init(
	IMPtEnv_OTData*	inOTData)
{
	UUtUns32		i;

	inOTData->maxDepth = 0;

	for(i = 0; i < IMPcEnv_OT_MaxGQsPerNode; i++)
	{
		inOTData->gqsPerNodeDist[i] = 0;
	}

	for(i = 0; i < IMPcEnv_OT_MaxBNVsPerNode; i++)
	{
		inOTData->bnvsPerNodeDist[i] = 0;
	}

	for(i = 0; i < IMPcEnv_OT_NumLeafNodeDims; i++)
	{
		inOTData->leafNodeSizeDist[i] = 0;
	}

	inOTData->nextLeafNodeIndex = 0;
	inOTData->nextInteriorNodeIndex = 0;

	inOTData->nextGQIndex = 0;

	inOTData->nextQTNodeIndex = 0;
	inOTData->nextBNVIndex = 0;
}

static void
IMPiEnv_OctTree_PrintStats(
	FILE*			inFile,
	IMPtEnv_OTData*	inOTData)
{
	UUtUns32		i;

	fprintf(inFile, "num interior nodes: %d"UUmNL, inOTData->nextInteriorNodeIndex);
	fprintf(inFile, "num leaf nodes: %d"UUmNL, inOTData->nextLeafNodeIndex);
	fprintf(inFile, "max depth: %d"UUmNL, inOTData->maxDepth);
	fprintf(inFile, "distribution"UUmNL);

	for(i = 0; i < IMPcEnv_OT_MaxGQsPerNode; i++)
	{
		fprintf(inFile, "GQsPerNode[%d] = %d"UUmNL, i, inOTData->gqsPerNodeDist[i]);
	}

	for(i = 0; i < IMPcEnv_OT_MaxBNVsPerNode; i++)
	{
		fprintf(inFile, "BNVsPerNode[%d] = %d"UUmNL, i, inOTData->bnvsPerNodeDist[i]);
	}

	for(i = 0; i < IMPcEnv_OT_NumLeafNodeDims; i++)
	{
		fprintf(inFile, "leafNodeSizeDist[%d] = %d"UUmNL, i, inOTData->leafNodeSizeDist[i]);
	}

}

static void
IMPiEnv_OctTree_FindIntersectionGQs(
	IMPtEnv_BuildData*		inBuildData,
	UUtUns32				inGQIndexCount,
	const UUtUns32*			inGQIndexArray,
	M3tBoundingBox_MinMax*	inBBox,
	UUtUns32				*outNewGQIndexArray,
	UUtUns32				*outNewGQIndexCount)
{
	UUtUns32			index;
	UUtUns32			curGQIndex;
	IMPtEnv_GQ*			curGQ;
	M3tPoint3D*			pointArray;
	M3tPlaneEquation*	planeArray;
	UUtUns32			numIndices = 0;

	UUmAssert(outNewGQIndexArray != NULL);

	pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	planeArray = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);

	for(index = 0;
		index < inGQIndexCount;
		index++)
	{
		curGQIndex = inGQIndexArray[index];
		curGQ = inBuildData->gqList + curGQIndex;

		if(curGQ->flags & AKcGQ_Flag_NoTextureMask)
		{
			continue;
		}

		if(CLrQuad_Box(
			curGQ->projection,
			pointArray,
			planeArray,
			curGQ->planeEquIndex,
			&curGQ->visibleQuad,
			inBBox) == UUcTrue)
		{
			UUmAssert(numIndices < inBuildData->maxGQs);

			outNewGQIndexArray[numIndices++] = curGQIndex;
		}
	}

	*outNewGQIndexCount = numIndices;
}

static UUtBool
IMPiEnv_OctTree_Build_BNVInfo_CheckQuad(
	IMPtEnv_BuildData*		inBuildData,
	IMPtEnv_BNV*			inBNV,
	M3tPoint3D*				inPointList,
	M3tQuad*				inQuad)
{
	UUtUns32					curSideIndex;
	IMPtEnv_BNV_Side*			curSide;
	UUtUns32					curQuadIndex;
	M3tPoint3D*					pointArray;
	M3tQuad						targetQuad;
	M3tQuad*				curQuad;
	M3tQuad*				quadArray;

	pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	quadArray = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);

	for(curSideIndex = 0, curSide = inBNV->sides;
		curSideIndex < inBNV->numSides;
		curSideIndex++, curSide++)
	{
		for(curQuadIndex = 0; curQuadIndex < curSide->numBNVQuads; curQuadIndex++)
		{
			curQuad = quadArray + curSide->bnvQuadList[curQuadIndex];

			targetQuad.indices[0] = (curQuad->indices[0]);
			targetQuad.indices[1] = (curQuad->indices[1]);
			targetQuad.indices[2] = (curQuad->indices[2]);
			targetQuad.indices[3] = (curQuad->indices[3]);

			if(CLrQuad_Quad(
				CLcProjection_Unknown,	// projection A
				inPointList,			// point array A
				NULL,					// plane array A
				0xFFFFFFFF,				// plane index A
				inQuad,					// quad A
				CLcProjection_Unknown,	// projection B
				pointArray,				// point array B
				NULL,					// plane array B
				0xFFFFFFFF,				// plane index B
				&targetQuad,			// quad B
				NULL,					// first contact
				NULL,					// intersection L
				NULL,					// intersection R
				NULL,					// plane A
				NULL					// plane B
			) != CLcCollision_None) return UUcTrue;
		}
	}

	return UUcFalse;
}

static UUtBool
IMPiEnv_OctTree_Build_BNVInfo_Check(
	IMPtEnv_BuildData*		inBuildData,
	M3tBoundingBox_MinMax*	inBBox,
	IMPtEnv_BNV*			inBNV)
{
	M3tPoint3D			points[8];
	M3tQuad				quad;
	UUtUns32			curSideIndex;
	IMPtEnv_BNV_Side*	curSide;
	UUtUns32			curQuadIndex;
	UUtUns32			curVertexIndex;

	M3tQuad*		quadArray;
	M3tPoint3D*			pointArray;
	M3tQuad*		curQuad;
	M3tPoint3D*			curPoint;

	float				minX, minY, minZ;
	float				maxX, maxY, maxZ;

	minX = inBBox->minPoint.x;
	minY = inBBox->minPoint.y;
	minZ = inBBox->minPoint.z;
	maxX = inBBox->maxPoint.x;
	maxY = inBBox->maxPoint.y;
	maxZ = inBBox->maxPoint.z;

	// First check if any ot vertices are in the BNV's bsp tree
		points[0].x = minX; points[0].y = minY; points[0].z = minZ;
		if(IMPiEnv_Process_BSP_PointInBSP(inBuildData, inBNV, &points[0], 0) == UUcTrue) return UUcTrue;

		points[1].x = maxX; points[1].y = minY; points[1].z = minZ;
		if(IMPiEnv_Process_BSP_PointInBSP(inBuildData, inBNV, &points[1], 0) == UUcTrue) return UUcTrue;

		points[2].x = minX; points[2].y = maxY; points[2].z = minZ;
		if(IMPiEnv_Process_BSP_PointInBSP(inBuildData, inBNV, &points[2], 0) == UUcTrue) return UUcTrue;

		points[3].x = maxX; points[3].y = maxY; points[3].z = minZ;
		if(IMPiEnv_Process_BSP_PointInBSP(inBuildData, inBNV, &points[3], 0) == UUcTrue) return UUcTrue;

		points[4].x = minX; points[4].y = minY; points[4].z = maxZ;
		if(IMPiEnv_Process_BSP_PointInBSP(inBuildData, inBNV, &points[4], 0) == UUcTrue) return UUcTrue;

		points[5].x = maxX; points[5].y = minY; points[5].z = maxZ;
		if(IMPiEnv_Process_BSP_PointInBSP(inBuildData, inBNV, &points[5], 0) == UUcTrue) return UUcTrue;

		points[6].x = minX; points[6].y = maxY; points[6].z = maxZ;
		if(IMPiEnv_Process_BSP_PointInBSP(inBuildData, inBNV, &points[6], 0) == UUcTrue) return UUcTrue;

		points[7].x = maxX; points[7].y = maxY; points[7].z = maxZ;
		if(IMPiEnv_Process_BSP_PointInBSP(inBuildData, inBNV, &points[7], 0) == UUcTrue) return UUcTrue;

	quadArray = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);
	pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);

	// Next see if any BNV vertices are within the ot node
		for(curSideIndex = 0, curSide = inBNV->sides;
			curSideIndex < inBNV->numSides;
			curSideIndex++, curSide++)
		{
			for(curQuadIndex = 0; curQuadIndex < curSide->numBNVQuads; curQuadIndex++)
			{
				curQuad = quadArray + curSide->bnvQuadList[curQuadIndex];

				for(curVertexIndex = 0; curVertexIndex < 4; curVertexIndex++)
				{
					curPoint = pointArray + curQuad->indices[curVertexIndex];

					if(minX <= curPoint->x && curPoint->x <= maxX &&
						minY <= curPoint->y && curPoint->y <= maxY &&
						minZ <= curPoint->z && curPoint->z <= maxZ)
					{
						return UUcTrue;
					}
				}
			}
		}

	// minX plane
	quad.indices[0] = 0; quad.indices[1] = 2; quad.indices[2] = 6; quad.indices[3] = 4;
	if(IMPiEnv_OctTree_Build_BNVInfo_CheckQuad(
		inBuildData,
		inBNV,
		points,
		&quad) == UUcTrue) return UUcTrue;

	// maxX plane
	quad.indices[0] = 1; quad.indices[1] = 3; quad.indices[2] = 7; quad.indices[3] = 5;
	if(IMPiEnv_OctTree_Build_BNVInfo_CheckQuad(
		inBuildData,
		inBNV,
		points,
		&quad) == UUcTrue) return UUcTrue;

	// minY plane
	quad.indices[0] = 0; quad.indices[1] = 1; quad.indices[2] = 5; quad.indices[3] = 4;
	if(IMPiEnv_OctTree_Build_BNVInfo_CheckQuad(
		inBuildData,
		inBNV,
		points,
		&quad) == UUcTrue) return UUcTrue;

	// maxY plane
	quad.indices[0] = 2; quad.indices[1] = 3; quad.indices[2] = 7; quad.indices[3] = 6;
	if(IMPiEnv_OctTree_Build_BNVInfo_CheckQuad(
		inBuildData,
		inBNV,
		points,
		&quad) == UUcTrue) return UUcTrue;

	// minZ plane
	quad.indices[0] = 0; quad.indices[1] = 1; quad.indices[2] = 3; quad.indices[3] = 2;
	if(IMPiEnv_OctTree_Build_BNVInfo_CheckQuad(
		inBuildData,
		inBNV,
		points,
		&quad) == UUcTrue) return UUcTrue;

	// maxZ plane
	quad.indices[0] = 4; quad.indices[1] = 5; quad.indices[2] = 7; quad.indices[3] = 6;
	if(IMPiEnv_OctTree_Build_BNVInfo_CheckQuad(
		inBuildData,
		inBNV,
		points,
		&quad) == UUcTrue) return UUcTrue;

	return UUcFalse;
}

static void
IMPiEnv_OctTree_FindIntersectionBNVs(
	IMPtEnv_BuildData*		inBuildData,
	UUtUns32				inBNVIndexCount,
	const UUtUns32*			inBNVIndexArray,
	M3tBoundingBox_MinMax*	inBBox,
	UUtUns32				*outNewBNVIndexArray,
	UUtUns32				*outNewBNVIndexCount)
{
	UUtUns32			index;
	UUtUns32			curBNVIndex;
	IMPtEnv_BNV*		curBNV;
	M3tPoint3D*			pointArray;
	M3tPlaneEquation*	planeArray;
	UUtUns32			numIndices = 0;

	UUmAssert(outNewBNVIndexArray != NULL);

	pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	planeArray = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);

	for(index = 0;
		index < inBNVIndexCount;
		index++)
	{
		curBNVIndex = inBNVIndexArray[index];
		curBNV = inBuildData->bnvList + curBNVIndex;

		if(IMPiEnv_OctTree_Build_BNVInfo_Check(
			inBuildData,
			inBBox,
			curBNV) == UUcTrue)
		{
			UUmAssert(numIndices < inBuildData->numBNVs);

			outNewBNVIndexArray[numIndices++] = curBNVIndex;
		}
	}

	*outNewBNVIndexCount = numIndices;
}

static UUtBool
IMPiEnv_OctTree_NodeContainsNode(
	IMPtEnv_OTData*	inOTData,
	UUtUns32		inCurNodeIndex,
	UUtUns32		inDesiredNode)
{
	AKtOctTree_InteriorNode*	interiorNode;
	UUtUns32					itr;

	if(inCurNodeIndex == inDesiredNode) return UUcTrue;

	if(AKmOctTree_IsLeafNode(inCurNodeIndex)) return UUcFalse;

	interiorNode = inOTData->interiorNodes + inCurNodeIndex;

	for(itr = 0; itr < 8; itr++)
	{
		if(IMPiEnv_OctTree_NodeContainsNode(
			inOTData,
			interiorNode->children[itr],
			inDesiredNode))
		{
			return UUcTrue;
		}
	}

	return UUcFalse;
}

static void
IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
	IMPtEnv_OTData*				inOTData,
	IMPtEnv_OTExtraNodeInfo*	inNodeExtraInfo,
	UUtUns32					inNodeIndex,
	UUtUns32					inSide)
{
	UUtUns32	adjIndex;
	UUtUns32	mirrorSide;
	UUtUns32	mirrorSideTable[] =
				{
					AKcOctTree_Side_PosX,
					AKcOctTree_Side_NegX,
					AKcOctTree_Side_PosY,
					AKcOctTree_Side_NegY,
					AKcOctTree_Side_PosZ,
					AKcOctTree_Side_NegZ
				};
	IMPtEnv_OTExtraNodeInfo*	adjExtraNodeInfo;
	UUtBool						nodeContaintsNode;

	adjIndex = inNodeExtraInfo->adjacent[inSide];

	if(adjIndex == 0xFFFFFFFF) return;

	if(AKmOctTree_IsLeafNode(adjIndex))
	{
		adjExtraNodeInfo = inOTData->leafNodeExtras + AKmOctTree_GetLeafIndex(adjIndex);
	}
	else
	{
		adjExtraNodeInfo = inOTData->interiorNodeExtras + adjIndex;
	}

	switch(inSide)
	{
		case AKcOctTree_Side_NegX:
			UUmAssert(inNodeExtraInfo->minXIndex == adjExtraNodeInfo->maxXIndex);
			break;
		case AKcOctTree_Side_PosX:
			UUmAssert(inNodeExtraInfo->maxXIndex == adjExtraNodeInfo->minXIndex);
			break;
		case AKcOctTree_Side_NegY:
			UUmAssert(inNodeExtraInfo->minYIndex == adjExtraNodeInfo->maxYIndex);
			break;
		case AKcOctTree_Side_PosY:
			UUmAssert(inNodeExtraInfo->maxYIndex == adjExtraNodeInfo->minYIndex);
			break;
		case AKcOctTree_Side_NegZ:
			UUmAssert(inNodeExtraInfo->minZIndex == adjExtraNodeInfo->maxZIndex);
			break;
		case AKcOctTree_Side_PosZ:
			UUmAssert(inNodeExtraInfo->maxZIndex == adjExtraNodeInfo->minZIndex);
			break;
	}

	mirrorSide = mirrorSideTable[inSide];

	UUmAssert(inNodeExtraInfo->depth >= adjExtraNodeInfo->depth);

	nodeContaintsNode =
		IMPiEnv_OctTree_NodeContainsNode(
			inOTData,
			adjExtraNodeInfo->adjacent[mirrorSide],
			inNodeIndex);
	UUmAssert(nodeContaintsNode);
}

static void
IMPiEnv_OctTree_Verify_SideRecursive(
	IMPtEnv_OTData*			inOTData,
	UUtUns32				inCurLeafNodeIndex,
	UUtUns32				inTargetAdjInfo,
	UUtUns32				inTargetAdjsCurSideIndex,
	float					inCurSidesValue)
{
	UUtUns32				itr;
	AKtQuadTree_Node*		curQuadNode;
	AKtOctTree_LeafNode*	targetLeafNode;
	float					curDim;
	UUtUns32				dimIndex;
	UUtUns32				maxXIndex;
	UUtUns32				maxYIndex;
	UUtUns32				maxZIndex;
	float					maxX, maxY, maxZ;
	float					minX, minY, minZ;
	UUtUns32				targetLeafNodeIndex;

	if(inTargetAdjInfo == 0xFFFFFFFF) return;

	if(AKmOctTree_IsLeafNode(inTargetAdjInfo))
	{
		targetLeafNodeIndex = AKmOctTree_GetLeafIndex(inTargetAdjInfo);
		targetLeafNode = inOTData->leafNodes + targetLeafNodeIndex;

		if(AKmOctTree_IsLeafNode(targetLeafNode->adjInfo[inTargetAdjsCurSideIndex]))
		{
			UUmAssert(targetLeafNode->adjInfo[inTargetAdjsCurSideIndex] == AKmOctTree_MakeLeafIndex(inCurLeafNodeIndex));
		}

		dimIndex = ((targetLeafNode->dim_Encode >> AKcOctTree_Shift_Dim) & AKcOctTree_Mask_Dim);
		UUmAssert(dimIndex < 9);
		curDim = AKgIntToFloatDim[dimIndex];

		maxXIndex = ((targetLeafNode->dim_Encode >> AKcOctTree_Shift_X) & AKcOctTree_Mask_X);
		UUmAssert(maxXIndex < 512);
		maxX = AKgIntToFloatXYZ[maxXIndex];

		maxYIndex = ((targetLeafNode->dim_Encode >> AKcOctTree_Shift_Y) & AKcOctTree_Mask_Y);
		UUmAssert(maxYIndex < 512);
		maxY = AKgIntToFloatXYZ[maxYIndex];

		maxZIndex = ((targetLeafNode->dim_Encode >> AKcOctTree_Shift_Z) & AKcOctTree_Mask_Z);
		UUmAssert(maxZIndex < 512);
		maxZ = AKgIntToFloatXYZ[maxZIndex];

		minX = maxX - curDim;
		minY = maxY - curDim;
		minZ = maxZ - curDim;

		switch(inTargetAdjsCurSideIndex)
		{
			case AKcOctTree_Side_NegX:
				UUmAssert(inCurSidesValue == minX);
				break;
			case AKcOctTree_Side_PosX:
				UUmAssert(inCurSidesValue == maxX);
				break;
			case AKcOctTree_Side_NegY:
				UUmAssert(inCurSidesValue == minY);
				break;
			case AKcOctTree_Side_PosY:
				UUmAssert(inCurSidesValue == maxY);
				break;
			case AKcOctTree_Side_NegZ:
				UUmAssert(inCurSidesValue == minZ);
				break;
			case AKcOctTree_Side_PosZ:
				UUmAssert(inCurSidesValue == maxZ);
				break;
		}
	}
	else
	{
		curQuadNode = inOTData->qtNodes + inTargetAdjInfo;

		for(itr = 0; itr < 4; itr++)
		{
			IMPiEnv_OctTree_Verify_SideRecursive(
				inOTData,
				inCurLeafNodeIndex,
				curQuadNode->children[itr],
				inTargetAdjsCurSideIndex,
				inCurSidesValue);
		}
	}
}

static void
IMPiEnv_OctTree_Verify_SideAdjInfo_IMeanItThisTime_Really(
	IMPtEnv_OTData*	inOTData,
	UUtUns32		inCurNodeIndex,
	UUtUns32		inTargetLeafNodeIndex)
{

}

static void
IMPiEnv_OctTree_Verify_SideAdjInfo_IMeanItThisTime(
	IMPtEnv_OTData*	inOTData,
	UUtUns32		inAdjInfo,
	UUtUns32		inTargetLeafNodeIndex,
	UUtUns32		inCorrectLeafIndex)
{
	UUtUns32	theFuckingIndex;

	if(AKmOctTree_IsLeafNode(inAdjInfo))
	{
		theFuckingIndex = AKmOctTree_GetLeafIndex(inAdjInfo);

		UUmAssert(theFuckingIndex == inCorrectLeafIndex);
	}
	else
	{
		// first start at the top of the tree and find the target leaf node
		IMPiEnv_OctTree_Verify_SideAdjInfo_IMeanItThisTime_Really(
			inOTData,
			0,
			inTargetLeafNodeIndex);

		// compute the difference between the target leaf node level and the current level
		// this is the number of levels in the quad tree

	}
}

static void
IMPiEnv_OctTree_Verify_SideAdjInfo(
	IMPtEnv_OTData*	inOTData,
	UUtUns32		inAdjInfo,
	UUtUns32		inCorrectLeafIndex,
	UUtUns32		inSideIndex)
{
	UUtUns32					targetLeafIndex;
	AKtOctTree_LeafNode*		targetLeafNode;
	UUtUns32					targetAdjLeafIndex;
	UUtUns32					itr;
	AKtQuadTree_Node*			curQuadTreeNode;

	if(inAdjInfo == 0xFFFFFFFF) return;

	if(AKmOctTree_IsLeafNode(inAdjInfo))
	{
		targetLeafIndex = AKmOctTree_GetLeafIndex(inAdjInfo);
		targetLeafNode = inOTData->leafNodes + targetLeafIndex;

		targetAdjLeafIndex = targetLeafNode->adjInfo[inSideIndex];

		IMPiEnv_OctTree_Verify_SideAdjInfo_IMeanItThisTime(
			inOTData,
			targetAdjLeafIndex,
			targetLeafIndex,
			inCorrectLeafIndex);
	}
	else
	{
		curQuadTreeNode = inOTData->qtNodes + inAdjInfo;

		for(itr = 0; itr < 4; itr++)
		{
			IMPiEnv_OctTree_Verify_SideAdjInfo(
				inOTData,
				curQuadTreeNode->children[itr],
				inCorrectLeafIndex,
				inSideIndex);
		}
	}
}

static void
IMPiEnv_OctTree_Verify_SideAdjInfoRecursive(
	IMPtEnv_OTData*	inOTData,
	UUtUns32		inCurIndex,
	UUtUns32		inOctant)
{
	UUtUns32					leafIndex;
	UUtUns32					itr;
	AKtOctTree_InteriorNode*	curInteriorNode;
	AKtOctTree_LeafNode*		curLeafNode;

	if(AKmOctTree_IsLeafNode(inCurIndex))
	{
		leafIndex = AKmOctTree_GetLeafIndex(inCurIndex);

		curLeafNode = inOTData->leafNodes + leafIndex;

		IMPiEnv_OctTree_Verify_SideAdjInfo(
			inOTData,
			curLeafNode->adjInfo[AKcOctTree_Side_NegX],
			leafIndex,
			AKcOctTree_Side_PosX);
		IMPiEnv_OctTree_Verify_SideAdjInfo(
			inOTData,
			curLeafNode->adjInfo[AKcOctTree_Side_PosX],
			leafIndex,
			AKcOctTree_Side_NegX);
		IMPiEnv_OctTree_Verify_SideAdjInfo(
			inOTData,
			curLeafNode->adjInfo[AKcOctTree_Side_NegY],
			leafIndex,
			AKcOctTree_Side_PosY);
		IMPiEnv_OctTree_Verify_SideAdjInfo(
			inOTData,
			curLeafNode->adjInfo[AKcOctTree_Side_PosY],
			leafIndex,
			AKcOctTree_Side_NegY);
		IMPiEnv_OctTree_Verify_SideAdjInfo(
			inOTData,
			curLeafNode->adjInfo[AKcOctTree_Side_NegZ],
			leafIndex,
			AKcOctTree_Side_PosZ);
		IMPiEnv_OctTree_Verify_SideAdjInfo(
			inOTData,
			curLeafNode->adjInfo[AKcOctTree_Side_PosZ],
			leafIndex,
			AKcOctTree_Side_NegZ);
	}
	else
	{
		curInteriorNode = inOTData->interiorNodes + inCurIndex;

		for(itr = 0; itr < 8; itr++)
		{
			IMPiEnv_OctTree_Verify_SideAdjInfoRecursive(
				inOTData,
				curInteriorNode->children[itr],
				itr);
		}
	}
}

static void
IMPiEnv_OctTree_Verify(
	IMPtEnv_OTData*	inOTData)
{
	UUtUns32				itr;
	AKtOctTree_LeafNode*	curLeafNode;
	float					maxX, maxY, maxZ;
	float					minX, minY, minZ;
	float					curDim;
	UUtUns32				dimIndex;
	UUtUns32				maxXIndex;
	UUtUns32				maxYIndex;
	UUtUns32				maxZIndex;

	// Second verify that indices match up between adjacent nodes
		IMPiEnv_OctTree_Verify_SideAdjInfoRecursive(
			inOTData,
			0,
			0xFFFFFFFF);

	// first check that all side values match
	for(itr = 0, curLeafNode = inOTData->leafNodes;
		itr < inOTData->nextLeafNodeIndex;
		itr++, curLeafNode++)
	{
		dimIndex = ((curLeafNode->dim_Encode >> AKcOctTree_Shift_Dim) & AKcOctTree_Mask_Dim);
		UUmAssert(dimIndex < 9);
		curDim = AKgIntToFloatDim[dimIndex];

		maxXIndex = ((curLeafNode->dim_Encode >> AKcOctTree_Shift_X) & AKcOctTree_Mask_X);
		UUmAssert(maxXIndex < 512);
		maxX = AKgIntToFloatXYZ[maxXIndex];

		maxYIndex = ((curLeafNode->dim_Encode >> AKcOctTree_Shift_Y) & AKcOctTree_Mask_Y);
		UUmAssert(maxYIndex < 512);
		maxY = AKgIntToFloatXYZ[maxYIndex];

		maxZIndex = ((curLeafNode->dim_Encode >> AKcOctTree_Shift_Z) & AKcOctTree_Mask_Z);
		UUmAssert(maxZIndex < 512);
		maxZ = AKgIntToFloatXYZ[maxZIndex];

		minX = maxX - curDim;
		minY = maxY - curDim;
		minZ = maxZ - curDim;

		// Verify current node's negX side with adj nodes posX side
			IMPiEnv_OctTree_Verify_SideRecursive(
				inOTData,
				itr,
				curLeafNode->adjInfo[AKcOctTree_Side_NegX],
				AKcOctTree_Side_PosX,
				minX);
		// Verify this posX side with adj negX side
			IMPiEnv_OctTree_Verify_SideRecursive(
				inOTData,
				itr,
				curLeafNode->adjInfo[AKcOctTree_Side_PosX],
				AKcOctTree_Side_NegX,
				maxX);

		// Verify this negY side with adj posY side
			IMPiEnv_OctTree_Verify_SideRecursive(
				inOTData,
				itr,
				curLeafNode->adjInfo[AKcOctTree_Side_NegY],
				AKcOctTree_Side_PosY,
				minY);
		// Verify this posY side with adj negY side
			IMPiEnv_OctTree_Verify_SideRecursive(
				inOTData,
				itr,
				curLeafNode->adjInfo[AKcOctTree_Side_PosY],
				AKcOctTree_Side_NegY,
				maxY);

		// Verify this negZ side with adj posZ side
			IMPiEnv_OctTree_Verify_SideRecursive(
				inOTData,
				itr,
				curLeafNode->adjInfo[AKcOctTree_Side_NegZ],
				AKcOctTree_Side_PosZ,
				minZ);
		// Verify this posZ side with adj negZ side
			IMPiEnv_OctTree_Verify_SideRecursive(
				inOTData,
				itr,
				curLeafNode->adjInfo[AKcOctTree_Side_PosZ],
				AKcOctTree_Side_NegZ,
				maxZ);
	}

}

UUtUns32 oct_tree_bnv_memory_saved = 0;
UUtUns32 oct_tree_gq_memory_saved = 0;

static void	IMPiEnv_OctTree_LeafNode_CreateGQList(
	AKtOctTree_LeafNode		*ioLeafNode,
	IMPtEnv_OTExtraNodeInfo *inNodeExtraInfo,
	IMPtEnv_BuildData*		inBuildData,
	IMPtEnv_OTData*			inOTData,
	UUtUns32				inGQIndexCount,
	const UUtUns32*			inGQIndexArray)
{
	UUtInt32 itr;
	UUtInt32 count_start;
	UUtInt32 count_end;
	UUtInt32 gq_index_count = inGQIndexCount;

	inOTData->leafNodeSizeDist[inNodeExtraInfo->dimIndex]++;

	ioLeafNode->dim_Encode = inNodeExtraInfo->dimIndex << AKcOctTree_Shift_Dim;

	ioLeafNode->dim_Encode |= inNodeExtraInfo->maxXIndex << AKcOctTree_Shift_X;
	ioLeafNode->dim_Encode |= inNodeExtraInfo->maxYIndex << AKcOctTree_Shift_Y;
	ioLeafNode->dim_Encode |= inNodeExtraInfo->maxZIndex << AKcOctTree_Shift_Z;

	// Update the gq stuff
	inOTData->gqsPerNodeDist[gq_index_count]++;

	if(gq_index_count == 0) {
		ioLeafNode->gqIndirectIndex_Encode = 0;
	}
	else {
		UUmAssert(inOTData->nextGQIndex <= AKcOctTree_GQInd_Start_Mask);
		UUmAssert(gq_index_count <= AKcOctTree_GQInd_Len_Mask);

		ioLeafNode->gqIndirectIndex_Encode = (inOTData->nextGQIndex << AKcOctTree_GQInd_Start_Shift) | gq_index_count;
	}

	if(gq_index_count > 0)
	{
		for(itr = 0; itr < gq_index_count; itr++)
		{
			if(gGQ_RemapArray[inGQIndexArray[itr]] == 0xFFFFFFFF) {
				gGQ_RemapArray[inGQIndexArray[itr]] = gGQ_NumRemapped++;
			}

			gGQ_RemapArray_CreateGQList[itr] = gGQ_RemapArray[inGQIndexArray[itr]];
		}
	}

	// ok we want to look for a string of numbers which matches gGQ_RemapArray_CreateGQList
	// or create a new string @ inOTData->gqIndexArray[inOTData->nextGQIndex + itr] = gGQ_RemapArray[inGQIndexArray[itr]];

	// scan for a list identical to what we want

	if (0 == gq_index_count) {
		return;
	}

	count_start = ((UUtInt32)inOTData->nextGQIndex) - ((UUtInt32) gq_index_count);
	count_end = (count_start > 100) ? (count_start - 100) : 0;

	for(itr = count_start; itr >= count_end; itr--)
	{
		UUtBool is_equal;
		UUtUns32 *test_memory;

		UUmAssert(itr >= 0);
		UUmAssert(((UUtUns32) itr) < inOTData->nextGQIndex);

		test_memory = inOTData->gqIndexArray + itr;

		is_equal = UUrMemory_IsEqual(test_memory, gGQ_RemapArray_CreateGQList, gq_index_count * sizeof(UUtUns32));

		if (is_equal) {
			UUmAssert(gq_index_count <= AKcOctTree_GQInd_Len_Mask);
			UUmAssert(itr <= AKcOctTree_GQInd_Start_Mask);

			ioLeafNode->gqIndirectIndex_Encode = (itr << AKcOctTree_GQInd_Start_Shift) | gq_index_count;
			oct_tree_gq_memory_saved += sizeof(UUtUns32) * gq_index_count;

			return;
		}
	}

	UUrMemory_MoveFast(gGQ_RemapArray_CreateGQList, inOTData->gqIndexArray + inOTData->nextGQIndex, gq_index_count * sizeof(UUtUns32));
	inOTData->nextGQIndex += gq_index_count;

#if 0
	{
		// CB: debugging
		UUtUns32 x, y, z, size;

		size	= (ioLeafNode->dim_Encode >> AKcOctTree_Shift_Dim) & AKcOctTree_Mask_Dim;
		x		= (ioLeafNode->dim_Encode >> AKcOctTree_Shift_X  ) & AKcOctTree_Mask_X;
		y		= (ioLeafNode->dim_Encode >> AKcOctTree_Shift_Y  ) & AKcOctTree_Mask_Y;
		z		= (ioLeafNode->dim_Encode >> AKcOctTree_Shift_Z  ) & AKcOctTree_Mask_Z;

		Imp_PrintMessage(IMPcMsg_Important,"(%d %d %d) leaf size %d: add %d GQ: total %d"UUmNL, x, y, z, size, gq_index_count, inOTData->nextGQIndex);
	}
#endif
}


static void IMPiEnv_OctTree_LeafNode_CreateBNVList(
	AKtOctTree_LeafNode		*ioLeafNode,
	IMPtEnv_BuildData*		inBuildData,
	IMPtEnv_OTData*			inOTData,
	UUtUns32				inBNVIndexCount,
	const UUtUns32*			inBNVIndexArray)
{
	UUtInt32 itr;
	UUtInt32 count;
	UUtInt32 bnv_index_scan = 0xffffffff;

	// Update the bnv stuff
	inOTData->bnvsPerNodeDist[inBNVIndexCount]++;


	if(inBNVIndexCount == 0) {
		ioLeafNode->bnvIndirectIndex_Encode = 0;

		return;
	}

	count = ((UUtInt32)inOTData->nextBNVIndex) - ((UUtInt32) inBNVIndexCount);

	// scan for a list identical to what we want
	for(itr = count; itr >= 0; itr--)
	{
		UUtBool is_equal;
		UUtUns32 *test_memory;

		UUmAssert(itr >= 0);
		UUmAssert(((UUtUns32) itr) < inOTData->nextBNVIndex);

		test_memory = inOTData->bnv_index_array + itr;

		is_equal = UUrMemory_IsEqual(test_memory, inBNVIndexArray, inBNVIndexCount * sizeof(UUtUns32));

		if (is_equal) {
			UUmAssert(inBNVIndexCount <= 0xff);
			UUmAssert(itr <= 0xffffff);

			ioLeafNode->bnvIndirectIndex_Encode = (itr << 8) | inBNVIndexCount;
			oct_tree_bnv_memory_saved += sizeof(UUtUns32) * inBNVIndexCount;

			return;
		}
	}

	UUmAssert(inBNVIndexCount <= 0xff);
	UUmAssert(inOTData->nextBNVIndex <= 0xffffff);

	ioLeafNode->bnvIndirectIndex_Encode = (inOTData->nextBNVIndex << 8) | inBNVIndexCount;

	if(inBNVIndexCount > 0) {
		UUmAssert((inOTData->nextBNVIndex + inBNVIndexCount) < IMPcEnv_MaxBNVIndexArray);

		UUrMemory_MoveFast(
			inBNVIndexArray,
			inOTData->bnv_index_array + inOTData->nextBNVIndex,
			inBNVIndexCount * sizeof(UUtUns32));
	}

	count = inOTData->nextBNVIndex + inBNVIndexCount;
	for(itr = inOTData->nextBNVIndex;
		itr < count;
		itr++)
	{
		UUmAssert(inOTData->bnv_index_array[itr] < inBuildData->numBNVs);
	}

	inOTData->nextBNVIndex += inBNVIndexCount;

	return;
}

static UUtError
IMPiEnv_OctTree_Build_Recursive(
	IMPtEnv_BuildData*		inBuildData,
	IMPtEnv_OTData*			inOTData,
	UUtUns32*				inTopOfStack,
	UUtUns32				inGQIndexCount,
	const UUtUns32*			inGQIndexArray,
	UUtUns32				inBNVIndexCount,
	const UUtUns32*			inBNVIndexArray,
	M3tBoundingBox_MinMax*	inBoundingBox,
	UUtUns32				inLevel,
	UUtUns32				inParent,
	UUtUns32				inOctant,
	UUtUns32				*outChildIndex)
{
	UUtError					error;

	UUtBool						nodeIsLeaf;
	IMPtEnv_OTExtraNodeInfo*	nodeExtraInfo;
	UUtUns32					nodeIndex;
	float						nodeDim;

	AKtOctTree_LeafNode*		leafNode;
	AKtOctTree_InteriorNode*	interiorNode;

	M3tBoundingBox_MinMax		childBBox;
	UUtUns32					childGQIndexCount;
	UUtUns32*					childGQIndexArray;
	UUtUns32					childGQTotal = 0;
	UUtUns32					childOctantItr;

	UUtUns32					childBNVTotal = 0;
	UUtUns32					childBNVIndexCount;
	UUtUns32*					childBNVIndexArray;

	static UUtUns32				statusDotCounter = 0;

	float						nodeCenterX, nodeCenterY, nodeCenterZ;
	UUtUns32					*top_of_stack = inTopOfStack;

	UUmAssert((top_of_stack >= IMPgOctree2_StackBase) && (top_of_stack < IMPgOctree2_StackMax));

	// Print out the cheesy status dots
	if(statusDotCounter++ % 2000 == 0) Imp_PrintMessage(IMPcMsg_Important,".");

	// Check for maximum tree depth
	if(inLevel > inOTData->maxDepth)
	{
		inOTData->maxDepth = inLevel;
	}

	// Get this node's dimension
	nodeDim = inBoundingBox->maxPoint.x - inBoundingBox->minPoint.x;

	UUmAssert(nodeDim > 0.0f);
	UUmAssert(nodeDim == (inBoundingBox->maxPoint.y - inBoundingBox->minPoint.y));
	UUmAssert(nodeDim == (inBoundingBox->maxPoint.z - inBoundingBox->minPoint.z));

	// Do some sanity checks
	if(nodeDim < AKcMinOTDim)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Too complex for oct tree params");
	}

	UUmAssert(inLevel < IMPcEnv_OT_MaxOctTreeDepth);

	// Decide if this node is an interior node or a leaf node
	nodeIsLeaf = (nodeDim == AKcMinOTDim) || (inGQIndexCount < IMPcEnv_OT_MinGQsPerNode);
	nodeIsLeaf = nodeIsLeaf && (nodeDim <= AKcMaxHalfOTDim);

	if(nodeIsLeaf)
	{
		if(inGQIndexCount >= IMPcEnv_OT_MaxGQsPerNode) {
			Imp_PrintWarning("oct tree node had too many gqs (%d / %d)", inGQIndexCount, IMPcEnv_OT_MaxGQsPerNode);
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Environment too complex for oct tree, too many GQs");
		}

		if(inBNVIndexCount >= IMPcEnv_OT_MaxBNVsPerNode)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Environment too complex for oct tree, too many BNVs");
		}

		if(inOTData->nextLeafNodeIndex >= IMPcEnv_MaxLeafNodes)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Too many parent nodes");
		}
		nodeIndex = inOTData->nextLeafNodeIndex++;
		nodeExtraInfo = inOTData->leafNodeExtras + nodeIndex;
		leafNode = inOTData->leafNodes + nodeIndex;
	}
	else
	{
		if(inOTData->nextInteriorNodeIndex >= IMPcEnv_MaxInteriorNodes)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Too many parent nodes");
		}
		nodeIndex = inOTData->nextInteriorNodeIndex++;
		nodeExtraInfo = inOTData->interiorNodeExtras + nodeIndex;
		interiorNode = inOTData->interiorNodes + nodeIndex;
	}

	// create the node extra info
	{
		float	tf1 = (float)(log(nodeDim) / log(2.0));
		float	tf2 = (float)(log(AKcMinOTDim) / log(2.0));

		nodeExtraInfo->dimIndex = (UUtUns32) (tf1 - tf2);
	}

	//nodeExtraInfo->dimIndex = ((log(nodeDim) - log(AKcMinOTDim))/log(2.0));

	nodeExtraInfo->minXIndex = (UUtUns16) (inBoundingBox->minPoint.x / AKcMinOTDim + 255.0f);
	nodeExtraInfo->minYIndex = (UUtUns16) (inBoundingBox->minPoint.y / AKcMinOTDim + 255.0f);
	nodeExtraInfo->minZIndex = (UUtUns16) (inBoundingBox->minPoint.z / AKcMinOTDim + 255.0f);
	nodeExtraInfo->maxXIndex = (UUtUns16) (inBoundingBox->maxPoint.x / AKcMinOTDim + 255.0f);
	nodeExtraInfo->maxYIndex = (UUtUns16) (inBoundingBox->maxPoint.y / AKcMinOTDim + 255.0f);
	nodeExtraInfo->maxZIndex = (UUtUns16) (inBoundingBox->maxPoint.z / AKcMinOTDim + 255.0f);
	UUmAssert(nodeExtraInfo->minXIndex < 512);
	UUmAssert(nodeExtraInfo->minYIndex < 512);
	UUmAssert(nodeExtraInfo->minZIndex < 512);
	UUmAssert(nodeExtraInfo->maxXIndex < 512);
	UUmAssert(nodeExtraInfo->maxYIndex < 512);
	UUmAssert(nodeExtraInfo->maxZIndex < 512);
	UUmAssert(nodeExtraInfo->maxXIndex >= 0);
	UUmAssert(nodeExtraInfo->maxYIndex >= 0);
	UUmAssert(nodeExtraInfo->maxZIndex >= 0);

	nodeExtraInfo->depth = inLevel;
	nodeExtraInfo->parent = inParent;
	nodeExtraInfo->myOctant = inOctant;

	// if we are in a leaf node then create it
	if(nodeIsLeaf)
	{
		UUmAssert(nodeExtraInfo->dimIndex < 9);
		if(inOTData->nextGQIndex + inGQIndexCount > IMPcEnv_MaxGQIndexArray)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Too many gq indices.");
		}

		UUmAssert(outChildIndex != NULL);
		*outChildIndex = AKmOctTree_MakeLeafIndex(nodeIndex);

		if (inGQIndexCount > AKcOctTree_GQInd_Len_Mask) {
			// CB: we can store at most AKcOctTree_GQInd_Len_Mask GQs in a single leaf node due to the bitmask packing. warn about this
			// and truncate the GQ list so that we don't corrupt other fields in the bitmask.
			Imp_PrintWarning("WARNING: octree leafnode at (%f %f %f)-(%f %f %f) has too many GQs in it (%d > %d)",
							inBoundingBox->minPoint.x, inBoundingBox->minPoint.y, inBoundingBox->minPoint.z,
							inBoundingBox->maxPoint.x, inBoundingBox->maxPoint.y, inBoundingBox->maxPoint.z, inGQIndexCount, AKcOctTree_GQInd_Len_Mask);
			inGQIndexCount = 0xff;
		}

		IMPiEnv_OctTree_LeafNode_CreateGQList(leafNode, nodeExtraInfo, inBuildData, inOTData, inGQIndexCount, inGQIndexArray);
		IMPiEnv_OctTree_LeafNode_CreateBNVList(leafNode, inBuildData, inOTData, inBNVIndexCount, inBNVIndexArray);

		return UUcError_None;
	}

	// we are in an interior node
	if(outChildIndex != NULL)
	{
		*outChildIndex = nodeIndex;
	}

	childGQIndexArray = top_of_stack;
	top_of_stack += inGQIndexCount;

	childBNVIndexArray = top_of_stack;
	top_of_stack += inBNVIndexCount;

	UUmAssert((top_of_stack >= IMPgOctree2_StackBase) && (top_of_stack < IMPgOctree2_StackMax));

	nodeCenterX = (inBoundingBox->minPoint.x + inBoundingBox->maxPoint.x) * 0.5f;
	nodeCenterY = (inBoundingBox->minPoint.y + inBoundingBox->maxPoint.y) * 0.5f;
	nodeCenterZ = (inBoundingBox->minPoint.z + inBoundingBox->maxPoint.z) * 0.5f;

	// First find all the leaf nodes
	for(childOctantItr = 0; childOctantItr < 8; childOctantItr++)
	{
		childBBox.minPoint.x = (childOctantItr & AKcOctTree_SideBV_X) ? nodeCenterX : inBoundingBox->minPoint.x;
		childBBox.minPoint.y = (childOctantItr & AKcOctTree_SideBV_Y) ? nodeCenterY : inBoundingBox->minPoint.y;
		childBBox.minPoint.z = (childOctantItr & AKcOctTree_SideBV_Z) ? nodeCenterZ : inBoundingBox->minPoint.z;

		childBBox.maxPoint.x = (childOctantItr & AKcOctTree_SideBV_X) ? inBoundingBox->maxPoint.x : nodeCenterX;
		childBBox.maxPoint.y = (childOctantItr & AKcOctTree_SideBV_Y) ? inBoundingBox->maxPoint.y : nodeCenterY;
		childBBox.maxPoint.z = (childOctantItr & AKcOctTree_SideBV_Z) ? inBoundingBox->maxPoint.z : nodeCenterZ;

		IMPiEnv_OctTree_FindIntersectionGQs(
			inBuildData,
			inGQIndexCount,
			inGQIndexArray,
			&childBBox,
			childGQIndexArray,
			&childGQIndexCount);

		childGQTotal += childGQIndexCount;

		IMPiEnv_OctTree_FindIntersectionBNVs(
			inBuildData,
			inBNVIndexCount,
			inBNVIndexArray,
			&childBBox,
			childBNVIndexArray,
			&childBNVIndexCount);

		childBNVTotal += childBNVIndexCount;

		error =
			IMPiEnv_OctTree_Build_Recursive(
				inBuildData,
				inOTData,
				top_of_stack,
				childGQIndexCount,
				childGQIndexArray,
				childBNVIndexCount,
				childBNVIndexArray,
				&childBBox,
				inLevel + 1,
				nodeIndex,
				childOctantItr,
				&interiorNode->children[childOctantItr]);
		UUmError_ReturnOnError(error);
	}

	UUmAssert(childGQTotal >= inGQIndexCount);
	UUmAssert(childBNVTotal >= inBNVIndexCount);

	return UUcError_None;
}

static UUtError
IMPiEnv_OctTree_Build_AdjInfo_ForSide(
	IMPtEnv_BuildData*			inBuildData,
	IMPtEnv_OTData*				inOTData,
	IMPtEnv_OTExtraNodeInfo*	inNodeExtraInfo,
	UUtUns32					inNodeIndex,
	UUtUns32					inSide,			// AKcOctTree_Side_*
	UUtUns32					inAxis,			// AKcOctTree_SideAxis_*
	UUtUns32					inPosNeg)		// 0 or 1
{
	UUtError				error;
	UUtUns32				adjInfoIndex;
	AKtOctTree_LeafNode*	leafNode;

	adjInfoIndex = inNodeExtraInfo->adjacent[inSide];

	leafNode = inOTData->leafNodes + inNodeIndex;

	if(AKmOctTree_IsLeafNode(adjInfoIndex))
	{
		leafNode->adjInfo[inSide] = adjInfoIndex;

		return UUcError_None;
	}

	error =
		IMPiEnv_QuadTree_Build(
			inBuildData,
			inOTData,
			gOct2QuadTree[inAxis][!inPosNeg],
			adjInfoIndex,
			&leafNode->adjInfo[inSide]);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

static UUtError
IMPiEnv_OctTree_Build_AdjInfo(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_OTData*		inOTData)
{
	UUtError					error;
	IMPtEnv_OTExtraNodeInfo*	curExtraNodeInfo;
	UUtUns32					nodeIndexItr;

	for(nodeIndexItr = 0, curExtraNodeInfo = inOTData->leafNodeExtras;
		nodeIndexItr < inOTData->nextLeafNodeIndex;
		nodeIndexItr++, curExtraNodeInfo++)
	{
		// Compute info for neg x side
		error =
			IMPiEnv_OctTree_Build_AdjInfo_ForSide(
				inBuildData,
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_NegX,
				AKcOctTree_SideAxis_X,
				0);
		UUmError_ReturnOnError(error);
		// Compute info for pos x side
		error =
			IMPiEnv_OctTree_Build_AdjInfo_ForSide(
				inBuildData,
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_PosX,
				AKcOctTree_SideAxis_X,
				1);
		UUmError_ReturnOnError(error);

		// Compute info for neg y side
		error =
			IMPiEnv_OctTree_Build_AdjInfo_ForSide(
				inBuildData,
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_NegY,
				AKcOctTree_SideAxis_Y,
				0);
		UUmError_ReturnOnError(error);
		// Compute info for pos y side
		error =
			IMPiEnv_OctTree_Build_AdjInfo_ForSide(
				inBuildData,
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_PosY,
				AKcOctTree_SideAxis_Y,
				1);
		UUmError_ReturnOnError(error);

		// Compute info for neg z side
		error =
			IMPiEnv_OctTree_Build_AdjInfo_ForSide(
				inBuildData,
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_NegZ,
				AKcOctTree_SideAxis_Z,
				0);
		UUmError_ReturnOnError(error);
		// Compute info for pos z side
		error =
			IMPiEnv_OctTree_Build_AdjInfo_ForSide(
				inBuildData,
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_PosZ,
				AKcOctTree_SideAxis_Z,
				1);
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

static void
IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
	IMPtEnv_OTData*				inOTData,
	IMPtEnv_OTExtraNodeInfo*	inNodeExtraInfo,
	UUtUns32					inNodeIndex,
	UUtUns32					inSide,			// AKcOctTree_Side_*
	UUtUns32					inAxis,			// AKcOctTree_SideAxis_*
	UUtUns32					inPosNeg)		// 0 or 1
{
	IMPtEnv_OTExtraNodeInfo*	curNodeExtraInfo = inNodeExtraInfo;
	UUtUns32					curNodeIndex = inNodeIndex;
	UUtUns32					potentialNextIndex;
	UUtUns32					curOctant;
	UUtUns32					octantStack[16];
	UUtInt16					octantTOS = 0;

	if(AKmOctTree_IsLeafNode(curNodeIndex))
	{
		UUmAssert(AKmOctTree_GetLeafIndex(curNodeIndex) < inOTData->nextLeafNodeIndex);
	}
	else
	{
		UUmAssert(curNodeIndex < inOTData->nextInteriorNodeIndex);
	}

	// go up until we can move into a sibling
		while(1)
		{
			if(curNodeExtraInfo->myOctant == 0xFFFFFFFF)
			{
				inNodeExtraInfo->adjacent[inSide] = 0xFFFFFFFF;
				return;
			}

			octantStack[octantTOS++] = curNodeExtraInfo->myOctant;

			if(inPosNeg)
			{
				if(!(curNodeExtraInfo->myOctant & inAxis)) break;
			}
			else
			{
				if(curNodeExtraInfo->myOctant & inAxis) break;
			}

			curNodeIndex = curNodeExtraInfo->parent;

			UUmAssert(curNodeIndex < inOTData->nextInteriorNodeIndex);

			curNodeExtraInfo = inOTData->interiorNodeExtras + curNodeIndex;
		}

	UUmAssert(curNodeExtraInfo->myOctant != 0xFFFFFFFF);

	UUmAssert(inNodeExtraInfo->depth >= curNodeExtraInfo->depth);

	curNodeIndex = curNodeExtraInfo->parent;

	while(octantTOS > 0)
	{
		curOctant = octantStack[--octantTOS];

		UUmAssert(curOctant < 8);

		curOctant = (curOctant & ~inAxis) | (~curOctant & inAxis);

		potentialNextIndex = inOTData->interiorNodes[curNodeIndex].children[curOctant];

		UUmAssert(potentialNextIndex != 0xFFFFFFFF);

		curNodeIndex = potentialNextIndex;

		if(AKmOctTree_IsLeafNode(curNodeIndex)) break;
	}

	if(!AKmOctTree_IsLeafNode(curNodeIndex))
	{
		curNodeExtraInfo = inOTData->interiorNodeExtras + curNodeIndex;
	}
	else
	{
		curNodeExtraInfo = inOTData->leafNodeExtras + AKmOctTree_GetLeafIndex(curNodeIndex);
	}
	UUmAssert(inNodeExtraInfo->depth >= curNodeExtraInfo->depth);

	inNodeExtraInfo->adjacent[inSide] = curNodeIndex;
}

static void
IMPiEnv_OctTree_Build_ExtraInfo(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_OTData*		inOTData)
{
	IMPtEnv_OTExtraNodeInfo*	curExtraNodeInfo;
	UUtUns32					nodeIndexItr;

	for(nodeIndexItr = 0, curExtraNodeInfo = inOTData->interiorNodeExtras;
		nodeIndexItr < inOTData->nextInteriorNodeIndex;
		nodeIndexItr++, curExtraNodeInfo++)
	{
		// Compute info for neg x side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_NegX,
				AKcOctTree_SideBV_X,
				0);
		// Compute info for pos x side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_PosX,
				AKcOctTree_SideBV_X,
				1);

		// Compute info for neg y side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_NegY,
				AKcOctTree_SideBV_Y,
				0);
		// Compute info for pos y side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_PosY,
				AKcOctTree_SideBV_Y,
				1);

		// Compute info for neg z side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_NegZ,
				AKcOctTree_SideBV_Z,
				0);
		// Compute info for pos z side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_PosZ,
				AKcOctTree_SideBV_Z,
				1);
	}

	for(nodeIndexItr = 0, curExtraNodeInfo = inOTData->leafNodeExtras;
		nodeIndexItr < inOTData->nextLeafNodeIndex;
		nodeIndexItr++, curExtraNodeInfo++)
	{
		// Compute info for neg x side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_NegX,
				AKcOctTree_SideBV_X,
				0);
		// Compute info for pos x side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_PosX,
				AKcOctTree_SideBV_X,
				1);

		// Compute info for neg y side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_NegY,
				AKcOctTree_SideBV_Y,
				0);
		// Compute info for pos y side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_PosY,
				AKcOctTree_SideBV_Y,
				1);

		// Compute info for neg z side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_NegZ,
				AKcOctTree_SideBV_Z,
				0);
		// Compute info for pos z side
			IMPiEnv_OctTree_Build_ExtraInfo_ForSide(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_PosZ,
				AKcOctTree_SideBV_Z,
				1);
	}

	for(nodeIndexItr = 0, curExtraNodeInfo = inOTData->interiorNodeExtras;
		nodeIndexItr < inOTData->nextInteriorNodeIndex;
		nodeIndexItr++, curExtraNodeInfo++)
	{
		if(curExtraNodeInfo->parent != 0xFFFFFFFF)
		{
			UUmAssert(inOTData->interiorNodes[curExtraNodeInfo->parent].children[curExtraNodeInfo->myOctant] == nodeIndexItr);
			UUmAssert(inOTData->interiorNodeExtras[curExtraNodeInfo->parent].depth == curExtraNodeInfo->depth - 1);
			UUmAssert(inOTData->interiorNodeExtras[curExtraNodeInfo->parent].dimIndex == curExtraNodeInfo->dimIndex + 1);
		}

		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_NegX);
		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_PosX);

		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_NegY);
		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_PosY);

		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_NegZ);
		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				nodeIndexItr,
				AKcOctTree_Side_PosZ);
	}

	for(nodeIndexItr = 0, curExtraNodeInfo = inOTData->leafNodeExtras;
		nodeIndexItr < inOTData->nextLeafNodeIndex;
		nodeIndexItr++, curExtraNodeInfo++)
	{
		if(curExtraNodeInfo->parent != 0xFFFFFFFF)
		{
			UUmAssert(inOTData->interiorNodes[curExtraNodeInfo->parent].children[curExtraNodeInfo->myOctant] ==
				AKmOctTree_MakeLeafIndex(nodeIndexItr));
			UUmAssert(inOTData->interiorNodeExtras[curExtraNodeInfo->parent].depth == curExtraNodeInfo->depth - 1);
			UUmAssert(inOTData->interiorNodeExtras[curExtraNodeInfo->parent].dimIndex == curExtraNodeInfo->dimIndex + 1);
		}

		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_NegX);
		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_PosX);

		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_NegY);
		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_PosY);

		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_NegZ);
		// Verify
			IMPiEnv_OctTree_Verify_ExtraInfo_Adjacent(
				inOTData,
				curExtraNodeInfo,
				AKmOctTree_MakeLeafIndex(nodeIndexItr),
				AKcOctTree_Side_PosZ);
	}
}

static UUtError
IMPiEnv_OctTree_Build(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_OTData*		inOTData)
{
	UUtError		error;
	UUtUns32		i;

	UUtUns32*		initialGQIndexArray;
	UUtUns32*		initialBNVIndexArray;
	UUtUns32		oct_tree_stack_size;
	UUtUns32*		initial_stack;
	UUtUns32*		top_of_stack;

	M3tBoundingBox_MinMax	bBox;

	IMPiEnv_OctTree_Init(inOTData);

	oct_tree_stack_size = (inBuildData->numGQs + inBuildData->numBNVs) * (IMPcEnv_OT_MaxOctTreeDepth + 1) * sizeof(UUtUns32);
	Imp_PrintMessage(IMPcMsg_Important, " (stack size %d) "UUmNL, oct_tree_stack_size);

	initial_stack = UUrMemory_Block_New(oct_tree_stack_size);
	UUmError_ReturnOnNullMsg(initial_stack, "could not create oct tree build stack");

	IMPgOctree2_StackBase = initial_stack;
	IMPgOctree2_StackMax = initial_stack + oct_tree_stack_size / sizeof(UUtUns32);

	top_of_stack = initial_stack;

	initialGQIndexArray = top_of_stack;
	top_of_stack += inBuildData->numGQs;

	for(i = 0; i < inBuildData->numGQs; i++)
	{
		initialGQIndexArray[i] = i;
	}

	initialBNVIndexArray = top_of_stack;
	top_of_stack += inBuildData->numBNVs;

	for(i = 0; i < inBuildData->numBNVs; i++)
	{
		initialBNVIndexArray[i] = i;
	}

	bBox.minPoint.x = -AKcMaxHalfOTDim;
	bBox.minPoint.y = -AKcMaxHalfOTDim;
	bBox.minPoint.z = -AKcMaxHalfOTDim;
	bBox.maxPoint.x = AKcMaxHalfOTDim;
	bBox.maxPoint.y = AKcMaxHalfOTDim;
	bBox.maxPoint.z = AKcMaxHalfOTDim;


	error =
		IMPiEnv_OctTree_Build_Recursive(
			inBuildData,
			inOTData,
			top_of_stack,
			inBuildData->numGQs,
			initialGQIndexArray,
			inBuildData->numBNVs,
			initialBNVIndexArray,
			&bBox,
			0,
			0xFFFFFFFF,
			0xFFFFFFFF,
			NULL);

	Imp_PrintMessage(IMPcMsg_Important, ""UUmNL);
	Imp_PrintMessage(IMPcMsg_Important, "compaction saved %d/%d -> %d/%d = %d/%d (bnv/gq),bytes"UUmNL,
		oct_tree_bnv_memory_saved + (inOTData->nextBNVIndex * 2), oct_tree_gq_memory_saved + (inOTData->nextGQIndex * 2),
		inOTData->nextBNVIndex * 2, inOTData->nextGQIndex * 2,
		oct_tree_bnv_memory_saved, oct_tree_gq_memory_saved);

	UUrMemory_Block_Delete(initial_stack);

	IMPiEnv_OctTree_Build_ExtraInfo(
		inBuildData,
		inOTData);

	error =
		IMPiEnv_OctTree_Build_AdjInfo(
			inBuildData,
			inOTData);
	UUmError_ReturnOnError(error);


	IMPiEnv_OctTree_Verify(inOTData);

	return error;
}


UUtError
IMPrEnv_Process_OctTrees2(
	IMPtEnv_BuildData*	inBuildData)
{
	UUtError		error;
	M3tPoint3D*		pointArray;
	float			globalMinX, globalMinY, globalMinZ;
	float			globalMaxX, globalMaxY, globalMaxZ;

	UUtUns32		curPointIndex;
	M3tPoint3D*		curPoint;
	UUtUns32		numPoints;
	UUtInt64		time;

	IMPtEnv_GQ*		newGQList;
	UUtUns32		itr;
	UUtUns32 		point_NumRemapped;
	UUtUns32*		point_RemapArray;
	IMPtEnv_GQ*		curGQ;
	UUtUns32		vertexItr;

	M3tQuad*	curQuad;
	M3tQuad*	quadList;
	UUtUns32		numQuads;

	gGQ_NumRemapped = 0;
	gGQ_RemapArray = UUrMemory_Block_New(inBuildData->numGQs * sizeof(UUtUns32));
	UUmError_ReturnOnNull(gGQ_RemapArray);

	gGQ_RemapArray_CreateGQList = UUrMemory_Block_New(inBuildData->numGQs * sizeof(UUtUns32));
	UUmError_ReturnOnNull(gGQ_RemapArray_CreateGQList);

	UUrMemory_Set32(gGQ_RemapArray, 0xFFFFFFFF, inBuildData->numGQs * sizeof(UUtUns32));

	newGQList = UUrMemory_Block_New(inBuildData->numGQs * sizeof(IMPtEnv_GQ));
	UUmError_ReturnOnNull(newGQList);

	globalMinX = globalMinY = globalMinZ = 1e9;
	globalMaxX = globalMaxY = globalMaxZ = -1e9;

	pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	numPoints = AUrSharedPointArray_GetNum(inBuildData->sharedPointArray);

	// First find the global min max
		for(curPointIndex = 0, curPoint = pointArray;
			curPointIndex < numPoints;
			curPointIndex++, curPoint++)
		{
			if(curPoint->x < globalMinX) globalMinX = curPoint->x;
			if(curPoint->y < globalMinY) globalMinY = curPoint->y;
			if(curPoint->z < globalMinZ) globalMinZ = curPoint->z;
			if(curPoint->x > globalMaxX) globalMaxX = curPoint->x;
			if(curPoint->y > globalMaxY) globalMaxY = curPoint->y;
			if(curPoint->z > globalMaxZ) globalMaxZ = curPoint->z;
		}


	if(globalMinX < (-AKcMaxHalfOTDim + AKcMinOTDim) ||
		globalMinY < (-AKcMaxHalfOTDim + AKcMinOTDim) ||
		globalMinY < (-AKcMaxHalfOTDim + AKcMinOTDim) ||
		globalMaxX > AKcMaxHalfOTDim ||
		globalMaxY > AKcMaxHalfOTDim ||
		globalMaxZ > AKcMaxHalfOTDim)
	{
		IMPrEnv_LogError("Exceeded max environment dimension - octree bounds (%f %f %f) - (%f %f %f)",
						globalMinX, globalMinY, globalMinZ, globalMaxX, globalMaxY, globalMaxZ);
	}

	// Compute Oct Tree for collision detection
	time = UUrMachineTime_High();

		Imp_PrintMessage(IMPcMsg_Important, "computing oct tree");
		error =
			IMPiEnv_OctTree_Build(
				inBuildData,
				&inBuildData->otData);
		UUmError_ReturnOnError(error);

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	// Fill the rest of the remap array
		for(itr = 0; itr < inBuildData->numGQs; itr++)
		{
			if(gGQ_RemapArray[itr] == 0xFFFFFFFF)
			{
				gGQ_RemapArray[itr] = gGQ_NumRemapped++;
			}
		}

		UUmAssert(gGQ_NumRemapped == inBuildData->numGQs);

		for(itr = 0; itr < inBuildData->numGQs; itr++)
		{
			newGQList[gGQ_RemapArray[itr]] = inBuildData->gqList[itr];
		}

		UUrMemory_Block_Delete(gGQ_RemapArray);
		UUrMemory_Block_Delete(gGQ_RemapArray_CreateGQList);
		UUrMemory_Block_Delete(inBuildData->gqList);

		inBuildData->gqList = newGQList;

		AUrSharedElemArray_Resort((AUtSharedElemArray*)inBuildData->sharedPointArray);

		point_NumRemapped = 0;
		point_RemapArray =
			UUrMemory_Block_New(
				AUrSharedPointArray_GetNum(inBuildData->sharedPointArray) * sizeof(UUtUns32));
		UUmError_ReturnOnNull(point_RemapArray);

		UUrMemory_Set32(point_RemapArray, 0xFFFFFFFF, AUrSharedPointArray_GetNum(inBuildData->sharedPointArray) * sizeof(UUtUns32));

		for(itr = 0, curGQ = inBuildData->gqList;
			itr < inBuildData->numGQs;
			itr++, curGQ++)
		{
			for(vertexItr = 0; vertexItr < 4; vertexItr++)
			{
				if(point_RemapArray[curGQ->visibleQuad.indices[vertexItr]] == 0xFFFFFFFF)
				{
					point_RemapArray[curGQ->visibleQuad.indices[vertexItr]] = point_NumRemapped++;
				}
				curGQ->visibleQuad.indices[vertexItr] = point_RemapArray[curGQ->visibleQuad.indices[vertexItr]];
			}
		}

		quadList = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);
		numQuads = AUrSharedQuadArray_GetNum(inBuildData->sharedBNVQuadArray);

		for(itr = 0, curQuad = quadList;
			itr < numQuads;
			itr++, curQuad++)
		{
			for(vertexItr = 0; vertexItr < 4; vertexItr++)
			{
				if(point_RemapArray[curQuad->indices[vertexItr]] == 0xFFFFFFFF)
				{
					point_RemapArray[curQuad->indices[vertexItr]] = point_NumRemapped++;
				}
				curQuad->indices[vertexItr] = point_RemapArray[curQuad->indices[vertexItr]];
			}
		}

		for(itr = 0;
			itr < AUrSharedPointArray_GetNum(inBuildData->sharedPointArray);
			itr++)
		{
			if(point_RemapArray[itr] == 0xFFFFFFFF)
			{
				point_RemapArray[itr] = point_NumRemapped++;
			}
		}

		UUmAssert(point_NumRemapped == AUrSharedPointArray_GetNum(inBuildData->sharedPointArray));

		error = AUrSharedPointArray_Reorder(inBuildData->sharedPointArray, point_RemapArray);
		UUmError_ReturnOnError(error);

		UUrMemory_Block_Delete(point_RemapArray);

		AUrSharedQuadArray_Resort(inBuildData->sharedBNVQuadArray);

		pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);


	fprintf(IMPgEnv_StatsFile, "**ot stats"UUmNL);
	IMPiEnv_OctTree_PrintStats(IMPgEnv_StatsFile, &inBuildData->otData);

	return UUcError_None;
}
