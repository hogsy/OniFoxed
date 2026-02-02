/*
	FILE:	Imp_Env2_Process.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 3, 1997

	PURPOSE:

	Copyright 1997

*/

#include <stdio.h>

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
#include "BFW_Math.h"

#include "Imp_Env2_Private.h"
#include "Imp_Common.h"
#include "Imp_Texture.h"
#include "Imp_AI_Script.h"


extern UUtWindow	gDebugWindow;
extern UUtBool		IMPgLightmaps;
UUtBool renderLightmap;

UUtMemory_Pool*		IMPgEnv_MemoryPool;
FILE*				IMPgEnv_DebugBSP = NULL;

UUtBool				IMPgCreateLightmapBMPs = UUcFalse;

extern char*			IMPg_Gunk_FileName;
extern char*			IMPg_Gunk_Objname;
extern UUtUns32			IMPg_Gunk_Flags;

UUtUns32 			IMPgEnv_VerifyAdjCount;

float	gOctTreeMinDim;


#define IMPcSAT_InsideBNVTolerance		2.0f
#define IMPcGhostFloorThreshold 6.0f
#define IMPcDoorPlaneThresh 6.0f

static void
iPrintSideIndices(
	FILE*				inFile,
	UUtUns16			inNumPlanes,
	IMPtEnv_BSP_Plane*	inPlanes)
{
	UUtUns16	i;

	for(i = 0; i < inNumPlanes; i++)
	{
		fprintf(inFile, "%02d ", inPlanes[i].ref);
	}

	fprintf(inFile, UUmNL);
}

static void
iPrintTabs(
	FILE*		inFile,
	UUtUns16	inNumTabs)
{
	UUtUns16	i;

	for(i = 0; i < inNumTabs; i++)
	{
		fprintf(inFile, "|\t");
	}
}

static IMPtEnv_BSP_AbsoluteSideFlag
IMPiEnv_Process_BSP_QuadAbsSide_Get(
	IMPtEnv_BuildData*	inBuildData,
	UUtUns32			inPlaneEquIndex,
	UUtUns32			inBSPQuadIndex)
{
	M3tQuad*		quadArray;
	M3tQuad*		curQuad;

	M3tPlaneEquation*	planeEqu;
	M3tPlaneEquation*	planeArray;
	UUtUns32			numPoints;
	float				a, b, c, d;
	float				result;

	M3tPoint3D*			pointArray;
	M3tPoint3D*			targetPoint;

	UUtUns16			numPos = 0;
	UUtUns16			numNeg = 0;
	UUtUns16			numEqual = 0;

	UUtUns16			i;


	planeArray = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);
	pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
	quadArray = AUrSharedQuadArray_GetList(inBuildData->bspQuadArray);
	numPoints = AUrSharedPointArray_GetNum(inBuildData->sharedPointArray);

	planeEqu = planeArray + (inPlaneEquIndex & 0x7FFFFFFF);

	a = planeEqu->a;
	b = planeEqu->b;
	c = planeEqu->c;
	d = planeEqu->d;

	if(inPlaneEquIndex & 0x80000000)
	{
		a = -a;
		b = -b;
		c = -c;
		d = -d;
	}

	curQuad = quadArray + inBSPQuadIndex;

	UUmAssert(curQuad->indices[0] != curQuad->indices[1]);
	UUmAssert(curQuad->indices[0] != curQuad->indices[2]);
	UUmAssert(curQuad->indices[0] != curQuad->indices[3]);
	UUmAssert(curQuad->indices[1] != curQuad->indices[2]);
	UUmAssert(curQuad->indices[1] != curQuad->indices[3]);
	UUmAssert(curQuad->indices[2] != curQuad->indices[3]);

	for(i = 0; i < 4; i++)
	{
		if(curQuad->indices[i] == UUcMaxUns32) continue;

		UUmAssert(curQuad->indices[i] < numPoints);

		targetPoint = pointArray + curQuad->indices[i];

		result = targetPoint->x * a + targetPoint->y * b + targetPoint->z * c + d;

		if(UUmFloat_CompareEqu(result, 0.0f)) numEqual++;

		else if(result < 0.0f) numPos++;
		else numNeg++;
	}

	if(numEqual == 4) return IMPcAbsSide_Postive;

	if(numNeg == 0) return IMPcAbsSide_Postive;
	if(numPos == 0) return IMPcAbsSide_Negative;
	return IMPcAbsSide_BothPosNeg;
}

static IMPtEnv_BSP_AbsoluteSideFlag
IMPiEnv_Process_BSP_PlaneAbsSide_Get(
	IMPtEnv_BuildData*	inBuildData,
	UUtUns32			inPlaneEquIndex,
	IMPtEnv_BSP_Plane*	inPlane)
{
	UUtUns16						quadItr;
	IMPtEnv_BSP_AbsoluteSideFlag	absSide;
	UUtUns16						numPos = 0;
	UUtUns16						numNeg = 0;

	for(quadItr = 0; quadItr < inPlane->numQuads; quadItr++)
	{
		absSide =
			IMPiEnv_Process_BSP_QuadAbsSide_Get(
				inBuildData,
				inPlaneEquIndex,
				inPlane->quads[quadItr].bspQuadIndex);

		if(absSide == IMPcAbsSide_BothPosNeg) return IMPcAbsSide_BothPosNeg;

		if(absSide == IMPcAbsSide_Postive) numPos++;
		else numNeg++;
	}

	if(numPos == 0) return IMPcAbsSide_Negative;
	if(numNeg == 0) return IMPcAbsSide_Postive;

	return IMPcAbsSide_BothPosNeg;
}

static IMPtEnv_BSP_Plane*
IMPiEnv_Process_BSP_ChooseDividingPlane(
	IMPtEnv_BuildData*	inBuildData,
	UUtUns32			inNumPlanes,
	IMPtEnv_BSP_Plane*	inPlanes)
{
	UUtInt32						posNegDiff;

	UUtInt32						minDiff = UUcMaxInt32;
	UUtUns32						minDiffPlaneIndex = UUcMaxUns32;
	UUtUns32						itr0, itr1;
	IMPtEnv_BSP_AbsoluteSideFlag	absSide;
	UUtUns32						numBoth = 0;

	for(itr0 = 0; itr0 < inNumPlanes; itr0++)
	{
		posNegDiff = 0;

		for(itr1 = 0; itr1 < inNumPlanes; itr1++)
		{
			if(itr0 == itr1) continue;

			absSide =
				IMPiEnv_Process_BSP_PlaneAbsSide_Get(
					inBuildData,
					inPlanes[itr0].planeEquIndex,
					inPlanes + itr1);

			if(absSide == IMPcAbsSide_Postive) posNegDiff++;
			else if(absSide == IMPcAbsSide_Negative) posNegDiff--;
			else numBoth++;
		}

		posNegDiff = abs(posNegDiff);

		if(posNegDiff < minDiff)
		{
			minDiff = posNegDiff;
			minDiffPlaneIndex = itr0;
		}
	}

	return inPlanes + minDiffPlaneIndex;
}

static UUtUns32	gUniqueNum = 0;

static UUtError
IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
	IMPtEnv_BuildData*				inBuildData,
	UUtUns32						inPlaneEquIndex,
	IMPtEnv_BSP_AbsoluteSideFlag	inSideToKeep,
	UUtUns32						inV0,
	UUtUns32						inV1,
	UUtUns32						inV2,
	UUtUns32						inV3,
	UUtUns32						inQuadRef,
	IMPtEnv_BSP_Plane				*ioPlane)
{
	UUtError	error;
	UUtBool		added = UUcFalse;
	//IMPtEnv_BSP_AbsoluteSideFlag	debugSide;

	if(inV0 == inV1)
	{
		if(inV3 != UUcMaxUns32)
		{
			added = UUcTrue;
			error =
				AUrSharedQuadArray_AddQuad(
					inBuildData->bspQuadArray,
					inV0,
					inV2,
					inV3,
					UUcMaxUns32,
					&ioPlane->quads[ioPlane->numQuads].bspQuadIndex);
			UUmError_ReturnOnError(error);
		}
	}
	else if(inV0 == inV2)
	{
		UUmAssert(inV3 == UUcMaxUns32 || inV2 == inV3);
	}
	else if(inV0 == inV3)
	{
		added = UUcTrue;
		error =
			AUrSharedQuadArray_AddQuad(
				inBuildData->bspQuadArray,
				inV0,
				inV1,
				inV2,
				UUcMaxUns32,
				&ioPlane->quads[ioPlane->numQuads].bspQuadIndex);
		UUmError_ReturnOnError(error);
	}
	else if(inV1 == inV2)
	{
		if(inV3 != UUcMaxUns32)
		{
			added = UUcTrue;
			error =
				AUrSharedQuadArray_AddQuad(
					inBuildData->bspQuadArray,
					inV0,
					inV1,
					inV3,
					UUcMaxUns32,
					&ioPlane->quads[ioPlane->numQuads].bspQuadIndex);
			UUmError_ReturnOnError(error);
		}
	}
	else if(inV1 == inV3)
	{
		added = UUcTrue;
		error =
			AUrSharedQuadArray_AddQuad(
				inBuildData->bspQuadArray,
				inV0,
				inV1,
				inV2,
				UUcMaxUns32,
				&ioPlane->quads[ioPlane->numQuads].bspQuadIndex);
		UUmError_ReturnOnError(error);
	}
	else if(inV2 == inV3)
	{
		added = UUcTrue;
		error =
			AUrSharedQuadArray_AddQuad(
				inBuildData->bspQuadArray,
				inV0,
				inV1,
				inV2,
				UUcMaxUns32,
				&ioPlane->quads[ioPlane->numQuads].bspQuadIndex);
		UUmError_ReturnOnError(error);
	}
	else
	{
		added = UUcTrue;
		error =
			AUrSharedQuadArray_AddQuad(
				inBuildData->bspQuadArray,
				inV0,
				inV1,
				inV2,
				inV3,
				&ioPlane->quads[ioPlane->numQuads].bspQuadIndex);
		UUmError_ReturnOnError(error);
	}

	if(added)
	{
		ioPlane->quads[ioPlane->numQuads].ref = inQuadRef;
		ioPlane->numQuads++;
	}

	return UUcError_None;
}

static UUtError
IMPiEnv_Process_BSP_Quad_AddSplit(
	IMPtEnv_BuildData*				inBuildData,
	IMPtEnv_BSP_AbsoluteSideFlag	inSideToKeep,
	UUtUns32						inPlaneEquIndex,
	IMPtEnv_BSP_Plane				*ioPlane,
	UUtUns32						inQuadToSplit,
	UUtUns32						inQuadRef)
{
	UUtError			error;

	M3tPoint3D*			pointArray;
	M3tPoint3D*			targetPoint;

	M3tQuad*		quadArray;
	M3tQuad*		targetQuad;

	M3tPlaneEquation*	planeEqu;
	M3tPlaneEquation*	planeArray;

	UUtUns32			curPointIndex;

	UUtUns16			clipCode = 0;	// bit vector, 1 means out of bounds

	UUtUns32			n0, n1;

	float				a, b, c, d;
	float				result;

	gUniqueNum++;

	planeArray = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);
	pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
	quadArray = AUrSharedQuadArray_GetList(inBuildData->bspQuadArray);
	targetQuad = AUrSharedQuadArray_GetList(inBuildData->bspQuadArray) + inQuadToSplit;

	UUmAssert(pointArray != NULL);

	planeEqu = planeArray + (inPlaneEquIndex & 0x7FFFFFFF);

	a = planeEqu->a;
	b = planeEqu->b;
	c = planeEqu->c;
	d = planeEqu->d;

	if(inPlaneEquIndex & 0x80000000)
	{
		a = -a;
		b = -b;
		c = -c;
		d = -d;
	}


	for(curPointIndex = 0; curPointIndex < 4; curPointIndex++)
	{
		if(targetQuad->indices[curPointIndex] == UUcMaxUns32) continue;

		targetPoint = pointArray + targetQuad->indices[curPointIndex];

		result = targetPoint->x * a + targetPoint->y * b + targetPoint->z * c + d;

		if(UUmFloat_CompareEqu(result, 0.0f)) continue;

		if(inSideToKeep == IMPcAbsSide_Postive)
		{
			if(result > 0.0f)
			{
				clipCode |= 1 << curPointIndex;
			}
		}
		else
		{
			if(result < 0.0f)
			{
				clipCode |= 1 << curPointIndex;
			}
		}
	}

	if(targetQuad->indices[3] == UUcMaxUns32)
	{
		// CLIP A TRIANGLE

		/*
			only a few possibilities here...

			1 is out, 0 is in

				v2	v1	v0
				===========
			0	0	0	0	<- Not Possible
			1	0	0	1
			2	0	1	0
			3	0	1	1
			4	1	0	0
			5	1	0	1
			6	1	1	0
			7	1	1	1	<- Not Possible
		*/

		switch(clipCode)
		{
			case 0x0:
			case 0x7:
				UUmAssert(!"Illegal case");
				break;

			case 0x1:
				// 0 is out, 1, 2 are in
				//	  \	   *0
				//     \  /|
				//	    \/ |
				//	  n1*\ |
				//	   /  \|
				//	  /    *n0
				//	 /     |\
				// 2*------*1\

				// Need to compute v1 -> v0 intersection, n0
				// Need to compute v2 -> v0 intersection, n1
				// New quad is (1, 2, n1, n0)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[1],
						pointArray + targetQuad->indices[0],
						&n0);
				UUmError_ReturnOnError(error);

				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[2],
						pointArray + targetQuad->indices[0],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[1],
						targetQuad->indices[2],
						n1,
						n0,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0x2:
				// 1 is out, 0, 2 are in
				//	  \	   *1
				//     \  /|
				//	    \/ |
				//	  n1*\ |
				//	   /  \|
				//	  /    *n0
				//	 /     |\
				// 0*------*2\

				// Need to compute v2 -> v1 intersection, n0
				// Need to compute v0 -> v1 intersection, n1
				// New quad is (2, 0, n1, n0)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[2],
						pointArray + targetQuad->indices[1],
						&n0);
				UUmError_ReturnOnError(error);

				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[0],
						pointArray + targetQuad->indices[1],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[2],
						targetQuad->indices[0],
						n1,
						n0,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0x3:
				// 0, 1 are out, 2 is in
				//	  \	   *2
				//     \  /|
				//	    \/ |
				//	  n1*\ |
				//	   /  \|
				//	  /    *n0
				//	 /     |\
				// 1*------*0\

				// Need to compute v2 -> v0 intersection, n0
				// Need to compute v2 -> v1 intersection, n1
				// New quad is (n0, n1, 2)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[2],
						pointArray + targetQuad->indices[0],
						&n0);
				UUmError_ReturnOnError(error);

				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[2],
						pointArray + targetQuad->indices[1],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						n0,
						n1,
						targetQuad->indices[2],
						UUcMaxUns32,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0x4:
				// 2 is out, 0, 1 are in
				//	  \	   *2
				//     \  /|
				//	    \/ |
				//	  n1*\ |
				//	   /  \|
				//	  /    *n0
				//	 /     |\
				// 1*------*0\

				// Need to compute v0 -> v2 intersection, n0
				// Need to compute v1 -> v2 intersection, n1
				// New quad is (0, 1, n1, n0)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[0],
						pointArray + targetQuad->indices[2],
						&n0);
				UUmError_ReturnOnError(error);

				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[1],
						pointArray + targetQuad->indices[2],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[0],
						targetQuad->indices[1],
						n1,
						n0,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0x5:
				// 0, 2 are out, 1 is in
				//	  \	   *1
				//     \  /|
				//	    \/ |
				//	  n1*\ |
				//	   /  \|
				//	  /    *n0
				//	 /     |\
				// 0*------*2\

				// Need to compute v1 -> v2 intersection, n0
				// Need to compute v1 -> v0 intersection, n1
				// New quad is (1, n0, n1)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[1],
						pointArray + targetQuad->indices[2],
						&n0);
				UUmError_ReturnOnError(error);

				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[1],
						pointArray + targetQuad->indices[0],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[1],
						n0,
						n1,
						UUcMaxUns32,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);

				#if defined(DEBUGGING) && DEBUGGING
				{
					IMPtEnv_BSP_AbsoluteSideFlag debugSide;

					debugSide =
						IMPiEnv_Process_BSP_QuadAbsSide_Get(
							inBuildData,
							inPlaneEquIndex,
							ioPlane->quads[ioPlane->numQuads - 1].bspQuadIndex);

					UUmAssert(debugSide == inSideToKeep);
				}
				#endif
				break;

			case 0x6:
				// 1, 2 are out, 0 is in
				//	  \	   *0
				//     \  /|
				//	    \/ |
				//	  n1*\ |
				//	   /  \|
				//	  /    *n0
				//	 /     |\
				// 2*------*1\

				// Need to compute v0 -> v1 intersection, n0
				// Need to compute v0 -> v2 intersection, n1
				// New quad is (0, n0, n1)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[0],
						pointArray + targetQuad->indices[1],
						&n0);
				UUmError_ReturnOnError(error);

				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[0],
						pointArray + targetQuad->indices[2],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[0],
						n0,
						n1,
						UUcMaxUns32,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			default:
				UUmAssert(0);

		}

	}
	else
	{
		// CLIP A QUAD

		/*
			only a few possibilities here...

			1 is out, 0 is in

				v3	v2	v1	v0
				==============
			0	0	0	0	0	<- Not Possible
			1	0	0	0	1
			2	0	0	1	0
			3	0	0	1	1
			4	0	1	0	0
			5	0	1	0	1	<- Not Possible
			6	0	1	1	0
			7	0	1	1	1
			8	1	0	0	0
			9	1	0	0	1
			A	1	0	1	0	<- Not Possible
			B	1	0	1	1
			C	1	1	0	0
			D	1	1	0	1
			E	1	1	1	0
			F	1	1	1	1	<- Not Possible
		*/

		switch(clipCode)
		{
			case 0x0:
			case 0x5:
			case 0xA:
			case 0xF:
				UUmAssert(!"Illegal case");
				break;

			case 0x1:
				// 0 is out, 1, 2, 3 is in
				//		\
				// 3*----n1--*0
				//	|     \  |
				//	|      \ |
				//	|       \|
				//	|        n0
				//	|        |\
				// 2*--------*1\

				// Need to compute v1 -> v0 intersection, n0
				// Need to compute v3 -> v0 intersection, n1
				// New quads are (1, 2, n0) and (2, 3, n1, n0)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[1],
						pointArray + targetQuad->indices[0],
						&n0);
				UUmError_ReturnOnError(error);

				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[3],
						pointArray + targetQuad->indices[0],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[1],
						targetQuad->indices[2],
						n0,
						UUcMaxUns32,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);

				quadArray = AUrSharedQuadArray_GetList(inBuildData->bspQuadArray);
				targetQuad = quadArray + inQuadToSplit;
				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[2],
						targetQuad->indices[3],
						n1,
						n0,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0x2:
				// 1 is out, 0, 2, 3 is in
				//		\
				// 0*----n1--*1
				//	|     \  |
				//	|      \ |
				//	|       \|
				//	|        n0
				//	|        |\
				// 3*--------*2\

				// Need to compute v2 -> v1 intersection, n0
				// Need to compute v0 -> v1 intersection, n1
				// New quads are (2, 3, n0) and (3, 0, n1, n0)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[2],
						pointArray + targetQuad->indices[1],
						&n0);
				UUmError_ReturnOnError(error);
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[0],
						pointArray + targetQuad->indices[1],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[2],
						targetQuad->indices[3],
						n0,
						UUcMaxUns32,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);

				quadArray = AUrSharedQuadArray_GetList(inBuildData->bspQuadArray);
				targetQuad = quadArray + inQuadToSplit;

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[3],
						targetQuad->indices[0],
						n1,
						n0,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0x3:
				// 0, 1 is out, 2, 3 is in
				//		 |
				// 3*----n1--*0
				//	|    |   |
				//	|    |   |
				//	|    |   |
				//	|    |   |
				//	|    |   |
				// 2*----n0--*1
				//		 |

				// Need to compute v2 -> v1 intersection, n0
				// Need to compute v3 -> v0 intersection, n1
				// New quads are (2, 3, n1, n0)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[2],
						pointArray + targetQuad->indices[1],
						&n0);
				UUmError_ReturnOnError(error);
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[3],
						pointArray + targetQuad->indices[0],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[2],
						targetQuad->indices[3],
						n1,
						n0,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0x4:
				// 2 is out, 0, 1, 3 is in
				//		\
				// 1*----n1--*2
				//	|     \  |
				//	|      \ |
				//	|       \|
				//	|        n0
				//	|        |\
				// 0*--------*3\

				// Need to compute v3 -> v2 intersection, n0
				// Need to compute v1 -> v2 intersection, n1
				// New quads are (3, 0, n0) and (0, 1, n1, n0)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[3],
						pointArray + targetQuad->indices[2],
						&n0);
				UUmError_ReturnOnError(error);
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[1],
						pointArray + targetQuad->indices[2],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[3],
						targetQuad->indices[0],
						n0,
						UUcMaxUns32,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);

				quadArray = AUrSharedQuadArray_GetList(inBuildData->bspQuadArray);
				targetQuad = quadArray + inQuadToSplit;

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[0],
						targetQuad->indices[1],
						n1,
						n0,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0x6:
				// 1, 2 is out, 0, 3 is in
				//		 |
				// 0*----n1--*1
				//	|    |   |
				//	|    |   |
				//	|    |   |
				//	|    |   |
				//	|    |   |
				// 3*----n0--*2
				//		 |

				// Need to compute v3 -> v2 intersection, n0
				// Need to compute v0 -> v1 intersection, n1
				// New quads are (3, 0, n1, n0)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[3],
						pointArray + targetQuad->indices[2],
						&n0);
				UUmError_ReturnOnError(error);
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[0],
						pointArray + targetQuad->indices[1],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[3],
						targetQuad->indices[0],
						n1,
						n0,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0x7:
				// 0, 1, 2 is out, 3 is in
				//
				// 0*--------*1
				//	|        |
				// \|        |
				//  n0       |
				//	|\       |
				//	| \      |
				// 3*--n1----*2
				//      \

				// Need to compute v3 -> v0 intersection, n0
				// Need to compute v3 -> v2 intersection, n1
				// New quads are (3, n0, n1)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[3],
						pointArray + targetQuad->indices[0],
						&n0);
				UUmError_ReturnOnError(error);
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[3],
						pointArray + targetQuad->indices[2],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[3],
						n0,
						n1,
						UUcMaxUns32,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0x8:
				// 3 is out, 0, 1, 2 is in
				//		\
				// 2*----n1--*3
				//	|     \  |
				//	|      \ |
				//	|       \|
				//	|        n0
				//	|        |\
				// 1*--------*0\

				// Need to compute v0 -> v3 intersection, n0
				// Need to compute v2 -> v3 intersection, n1
				// New quads are (0, 1, n0) and (1, 2, n1, n0)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[0],
						pointArray + targetQuad->indices[3],
						&n0);
				UUmError_ReturnOnError(error);
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[2],
						pointArray + targetQuad->indices[3],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					AUrSharedQuadArray_AddQuad(
						inBuildData->bspQuadArray,
						targetQuad->indices[0],
						targetQuad->indices[1],
						n0,
						UUcMaxUns32,
						&ioPlane->quads[ioPlane->numQuads].bspQuadIndex);
				UUmError_ReturnOnError(error);

				quadArray = AUrSharedQuadArray_GetList(inBuildData->bspQuadArray);
				targetQuad = quadArray + inQuadToSplit;

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[1],
						targetQuad->indices[2],
						n1,
						n0,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0x9:
				// 0, 3 is out, 1, 2 is in
				//		 |
				// 2*----n1--*3
				//	|    |   |
				//	|    |   |
				//	|    |   |
				//	|    |   |
				//	|    |   |
				// 1*----n0--*0
				//		 |

				// Need to compute v1 -> v0 intersection, n0
				// Need to compute v2 -> v3 intersection, n1
				// New quads are (1, 2, n1, n0)
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[1],
						pointArray + targetQuad->indices[0],
						&n0);
				UUmError_ReturnOnError(error);
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[2],
						pointArray + targetQuad->indices[3],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[1],
						targetQuad->indices[2],
						n1,
						n0,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0xB:
				// 0, 1, 3 is out, 2 is in
				//
				// 3*--------*0
				//	|        |
				// \|        |
				//  n0       |
				//	|\       |
				//	| \      |
				// 2*--n1----*1
				//      \

				// Need to compute v2 -> v3 intersection, n0
				// Need to compute v2 -> v1 intersection, n1
				// New quads are (2, n0, n1)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[2],
						pointArray + targetQuad->indices[3],
						&n0);
				UUmError_ReturnOnError(error);
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[2],
						pointArray + targetQuad->indices[1],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[2],
						n0,
						n1,
						UUcMaxUns32,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0xC:
				// 2, 3 is out, 0, 1 is in
				//		 |
				// 1*----n1--*2
				//	|    |   |
				//	|    |   |
				//	|    |   |
				//	|    |   |
				//	|    |   |
				// 0*----n0--*3
				//		 |

				// Need to compute v0 -> v3 intersection, n0
				// Need to compute v1 -> v2 intersection, n1
				// New quads are (0, 1, n1, n0)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[0],
						pointArray + targetQuad->indices[3],
						&n0);
				UUmError_ReturnOnError(error);
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[1],
						pointArray + targetQuad->indices[2],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[0],
						targetQuad->indices[1],
						n1,
						n0,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0xD:
				// 0, 2, 3 is out,1 is in
				//
				// 2*--------*3
				//	|        |
				// \|        |
				//  n0       |
				//	|\       |
				//	| \      |
				// 1*--n1----*0
				//      \

				// Need to compute v1 -> v2 intersection, n0
				// Need to compute v1 -> v0 intersection, n1
				// New quads are (1, n0, n1)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[1],
						pointArray + targetQuad->indices[2],
						&n0);
				UUmError_ReturnOnError(error);
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[1],
						pointArray + targetQuad->indices[0],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[1],
						n0,
						n1,
						UUcMaxUns32,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			case 0xE:
				// 1, 2, 3 is out, 0 is in
				//
				// 1*--------*2
				//	|        |
				// \|        |
				//  n0       |
				//	|\       |
				//	| \      |
				// 0*--n1----*3
				//      \

				// Need to compute v0 -> v1 intersection, n0
				// Need to compute v0 -> v3 intersection, n1
				// New quads are (0, n0, n1)
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[0],
						pointArray + targetQuad->indices[1],
						&n0);
				UUmError_ReturnOnError(error);
				pointArray = AUrSharedPointArray_GetList(inBuildData->bspPointArray);
				error =
					AUrSharedPointArray_InterpolatePoint(
						inBuildData->bspPointArray,
						a, b, c, d,
						pointArray + targetQuad->indices[0],
						pointArray + targetQuad->indices[3],
						&n1);
				UUmError_ReturnOnError(error);

				error =
					IMPiEnv_Process_BSP_Quad_AddSplit_AddQuad(
						inBuildData,
						inPlaneEquIndex,
						inSideToKeep,
						targetQuad->indices[0],
						n0,
						n1,
						UUcMaxUns32,
						inQuadRef,
						ioPlane);
				UUmError_ReturnOnError(error);
				break;

			default:
				UUmAssert(!"Illegal case");
		}
	}

	return UUcError_None;
}

static UUtError
IMPiEnv_Process_BSP_Plane_Split(
	IMPtEnv_BuildData*				inBuildData,
	IMPtEnv_BSP_AbsoluteSideFlag	inSideToKeep,
	UUtUns32						inSplittingPlaneEquIndex,
	IMPtEnv_BSP_Plane*				inPlaneToSplit,
	IMPtEnv_BSP_Plane				*outResultPlane)
{
	UUtError						error;
	UUtUns16						quadItr;
	IMPtEnv_BSP_AbsoluteSideFlag	quadAbsSide;

	outResultPlane->planeEquIndex = inPlaneToSplit->planeEquIndex;
	outResultPlane->ref = inPlaneToSplit->ref;
	outResultPlane->numQuads = 0;

	for(quadItr = 0; quadItr < inPlaneToSplit->numQuads; quadItr++)
	{
		quadAbsSide =
			IMPiEnv_Process_BSP_QuadAbsSide_Get(
				inBuildData,
				inSplittingPlaneEquIndex,
				inPlaneToSplit->quads[quadItr].bspQuadIndex);

		if(quadAbsSide == IMPcAbsSide_BothPosNeg)
		{

			if(outResultPlane->numQuads + 1 >= IMPcEnv_MaxQuadsPerPlane)
			{
				UUmError_ReturnOnErrorMsg(UUcError_Generic,"Too many quads on a plane");
			}


			error =
				IMPiEnv_Process_BSP_Quad_AddSplit(
					inBuildData,
					inSideToKeep,
					inSplittingPlaneEquIndex,
					outResultPlane,
					inPlaneToSplit->quads[quadItr].bspQuadIndex,
					inPlaneToSplit->quads[quadItr].ref);
			UUmError_ReturnOnError(error);

		}
		else if(quadAbsSide == inSideToKeep)
		{
			if(outResultPlane->numQuads >= IMPcEnv_MaxQuadsPerPlane)
			{
				UUmError_ReturnOnErrorMsg(UUcError_Generic,"Too many quads on a plane");
			}

			outResultPlane->quads[outResultPlane->numQuads++] = inPlaneToSplit->quads[quadItr];
		}
	}

	return UUcError_None;
}

static IMPtEnv_BSP_Plane	IMPgEnv_NewPlanes[IMPcEnv_MaxNewAlphaBSPPlanes];

static UUtError
IMPiEnv_Process_BSP_Plane_BuildNewList(
	IMPtEnv_BuildData*				inBuildData,
	IMPtEnv_BSP_AbsoluteSideFlag	inSideToKeep,
	IMPtEnv_BSP_Plane*				inDividingPlane,
	UUtUns32						inNumPlanes,
	IMPtEnv_BSP_Plane*				inPlanes,
	UUtUns32						*outNumNewPlanes,
	IMPtEnv_BSP_Plane*				*outPlaneList)
{
	UUtError						error;
	UUtUns32						planeItr;
	IMPtEnv_BSP_Plane*				curPlane;
	UUtUns32						planeEquIndex;
	UUtUns32						numNewPlanes = 0;
	IMPtEnv_BSP_AbsoluteSideFlag	absPlaneSide;
	IMPtEnv_BSP_Plane*				newAllocatedList;

	planeEquIndex = inDividingPlane->planeEquIndex;

	for(planeItr = 0; planeItr < inNumPlanes; planeItr++)
	{
		curPlane = inPlanes + planeItr;

		if(curPlane == inDividingPlane)
		{
			if(inSideToKeep == IMPcAbsSide_Postive && inDividingPlane->numQuads > 0)
			{
				IMPgEnv_NewPlanes[numNewPlanes++] = *curPlane;
			}
			continue;
		}

		absPlaneSide =
			IMPiEnv_Process_BSP_PlaneAbsSide_Get(
				inBuildData,
				planeEquIndex,
				curPlane);

		if(absPlaneSide == IMPcAbsSide_BothPosNeg)
		{
			error =
				IMPiEnv_Process_BSP_Plane_Split(
					inBuildData,
					inSideToKeep,
					planeEquIndex,
					curPlane,
					IMPgEnv_NewPlanes + numNewPlanes);
			UUmError_ReturnOnError(error);

			if(IMPgEnv_NewPlanes[numNewPlanes].numQuads > 0) numNewPlanes++;
		}
		else if(absPlaneSide == inSideToKeep)
		{
			IMPgEnv_NewPlanes[numNewPlanes++] = *curPlane;
		}
	}

	#if 0
	for(planeItr = 0; planeItr < numNewPlanes; planeItr++)
	{
		absPlaneSide =
			IMPiEnv_Process_BSP_PlaneAbsSide_Get(
				inBuildData,
				inDividingPlane->planeEquIndex,
				IMPgEnv_NewPlanes + planeItr);
		UUmAssert(absPlaneSide == inSideToKeep);
	}
	#endif

	// allocate and create new plane list

	newAllocatedList =
		UUrMemory_Pool_Block_New(
			IMPgEnv_MemoryPool,
			numNewPlanes * sizeof(IMPtEnv_BSP_Plane));
	UUmError_ReturnOnNull(newAllocatedList);

	UUrMemory_MoveFast(IMPgEnv_NewPlanes, newAllocatedList, numNewPlanes * sizeof(IMPtEnv_BSP_Plane));

	*outNumNewPlanes = numNewPlanes;
	*outPlaneList = newAllocatedList;

	return UUcError_None;
}

static UUtError
IMPiEnv_Process_BSP_Build_Recursive(
	IMPtEnv_BuildData*	inBuildData,
	UUtUns32			inLevel,
	UUtUns32			inNumPlanes,
	IMPtEnv_BSP_Plane*	inPlanes,
	UUtUns32			*outNewIndex)
{
	UUtError			error;
	IMPtEnv_BSP_Plane*	divPlane;
	UUtUns32			numNewPosPlanes;
	UUtUns32			numNewNegPlanes;
	IMPtEnv_BSP_Plane*	newPosPlanes;
	IMPtEnv_BSP_Plane*	newNegPlanes;

	IMPtEnv_BSP_Node*	newNode;
	UUtUns32			length = 0;

	if ((0 == inNumPlanes) && (NULL == outNewIndex)) {
		return UUcError_None;
	}

	// find dividing plane
		divPlane =
			IMPiEnv_Process_BSP_ChooseDividingPlane(
				inBuildData,
				inNumPlanes,
				inPlanes);

		UUmAssert(divPlane->numQuads > 0);

	// create the node
		if(inBuildData->numBSPNodes >= IMPcEnv_MaxNumBSPNodes)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Ran out of BSP nodes");
		}

		newNode = inBuildData->bspNodes + inBuildData->numBSPNodes++;

		if((inBuildData->numBSPNodes % 50) == 0) Imp_PrintMessage(IMPcMsg_Important,".");

		newNode->splittingPlane = divPlane;
		newNode->splittingQuad = divPlane->quads[--divPlane->numQuads];

		if(outNewIndex != NULL) *outNewIndex = inBuildData->numBSPNodes - 1;

	// Find all the GQs on the positive div plane
		IMPiEnv_Process_BSP_Plane_BuildNewList(
			inBuildData,
			IMPcAbsSide_Postive,
			divPlane,
			inNumPlanes,
			inPlanes,
			&numNewPosPlanes,
			&newPosPlanes);

		if(numNewPosPlanes == 0)
		{
			newNode->posNodeIndex = 0xFFFFFFFF;
		}
		else
		{
			error =
				IMPiEnv_Process_BSP_Build_Recursive(
					inBuildData,
					inLevel + 1,
					numNewPosPlanes,
					newPosPlanes,
					&newNode->posNodeIndex);
			UUmError_ReturnOnError(error);
		}

	// Find all the GQs on the neg div plane
		IMPiEnv_Process_BSP_Plane_BuildNewList(
			inBuildData,
			IMPcAbsSide_Negative,
			divPlane,
			inNumPlanes,
			inPlanes,
			&numNewNegPlanes,
			&newNegPlanes);

		if(numNewNegPlanes == 0)
		{
			newNode->negNodeIndex = 0xFFFFFFFF;
		}
		else
		{
			error =
				IMPiEnv_Process_BSP_Build_Recursive(
					inBuildData,
					inLevel + 1,
					numNewNegPlanes,
					newNegPlanes,
					&newNode->negNodeIndex);
			UUmError_ReturnOnError(error);
		}

	UUmAssert((numNewPosPlanes + numNewNegPlanes) >= (inNumPlanes - 1));

	return UUcError_None;
}

static UUtError
IMPiEnv_Process_BSP_BNV_Build(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_BNV*		inBNV)
{
	UUtError				error;

	IMPtEnv_BSP_Plane*		bspPlaneList;
	IMPtEnv_BSP_Plane*		curBSPPlane;

	UUtUns16				curBNVSideIndex;
	IMPtEnv_BNV_Side*		curBNVSide;

	M3tPoint3D*				pointArray;

	UUtUns32				curQuadIndex;

	M3tQuad			newQuad;

	UUtUns32				curPointIndex;
	M3tPoint3D*				targetPoint;

	M3tQuad*			curQuad;
	M3tQuad*			quadArray;
	UUtUns16				nodeItr;

	inBuildData->numBSPNodes = 0;

	pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	quadArray = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);

	UUrMemory_Pool_Reset(IMPgEnv_MemoryPool);
	AUrSharedPointArray_Reset(inBuildData->bspPointArray);
	AUrSharedQuadArray_Reset(inBuildData->bspQuadArray);

	bspPlaneList = UUrMemory_Pool_Block_New(IMPgEnv_MemoryPool, inBNV->numSides * sizeof(IMPtEnv_BSP_Plane));
	UUmError_ReturnOnNull(bspPlaneList);

	for(curBNVSideIndex = 0, curBNVSide = inBNV->sides, curBSPPlane = bspPlaneList;
		curBNVSideIndex < inBNV->numSides;
		curBNVSideIndex++, curBNVSide++, curBSPPlane++)
	{
		curBSPPlane->numQuads = curBNVSide->numBNVQuads;
		curBSPPlane->planeEquIndex = curBNVSide->planeEquIndex;
		curBSPPlane->ref = curBNVSideIndex;

		for(curQuadIndex = 0; curQuadIndex < curBNVSide->numBNVQuads; curQuadIndex++)
		{
			curQuad = quadArray + curBNVSide->bnvQuadList[curQuadIndex];

			for(curPointIndex = 0; curPointIndex < 4; curPointIndex++)
			{
				targetPoint = pointArray + curQuad->indices[curPointIndex];

				error =
					AUrSharedPointArray_AddPoint(
						inBuildData->bspPointArray,
						targetPoint->x,
						targetPoint->y,
						targetPoint->z,
						&newQuad.indices[curPointIndex]);
				UUmError_ReturnOnError(error);
			}

			UUmAssert(newQuad.indices[0] != newQuad.indices[1]);
			UUmAssert(newQuad.indices[0] != newQuad.indices[2]);
			UUmAssert(newQuad.indices[0] != newQuad.indices[3]);
			UUmAssert(newQuad.indices[1] != newQuad.indices[2]);
			UUmAssert(newQuad.indices[1] != newQuad.indices[3]);
			UUmAssert(newQuad.indices[2] != newQuad.indices[3]);

			error =
				AUrSharedQuadArray_AddQuad(
					inBuildData->bspQuadArray,
					newQuad.indices[0],
					newQuad.indices[1],
					newQuad.indices[2],
					newQuad.indices[3],
					&curBSPPlane->quads[curQuadIndex].bspQuadIndex);
			UUmError_ReturnOnError(error);

			curBSPPlane->quads[curQuadIndex].ref = 0xE1117E;
		}
	}

	error =
		IMPiEnv_Process_BSP_Build_Recursive(
			inBuildData,
			0,
			inBNV->numSides,
			bspPlaneList,
			NULL);
	UUmError_ReturnOnError(error);

	// convert to BNV bsp nodes
	if(inBuildData->numBSPNodes > IMPcEnv_MaxNumBNVBSPNodes)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "too many bnv bsp nodes");
	}

	inBNV->numNodes = inBuildData->numBSPNodes;

	for(nodeItr = 0; nodeItr < inBuildData->numBSPNodes; nodeItr++)
	{
		inBNV->bspNodes[nodeItr].planeEquIndex = inBuildData->bspNodes[nodeItr].splittingPlane->planeEquIndex;
		inBNV->bspNodes[nodeItr].posNodeIndex = inBuildData->bspNodes[nodeItr].posNodeIndex;
		inBNV->bspNodes[nodeItr].negNodeIndex = inBuildData->bspNodes[nodeItr].negNodeIndex;
	}

	return UUcError_None;
}

UUtBool
IMPiEnv_Process_BSP_PointInBSP(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_BNV*		inBNV,
	M3tPoint3D*			inPoint,
	float				inTolerance)
{
	AKtBNV_BSPNode*		curNode;

	M3tPlaneEquation*	planeEqu;
	M3tPlaneEquation*	planeArray;

	float				a, b, c, d, threshold;
	UUtUns32			curNodeIndex;
	static UUtBool		invalid_bnv_once = UUcTrue;

	UUmAssert(inBNV->numNodes > 0);
	UUmAssert(inBNV->numSides > 0);

	if (((0 == inBNV->numNodes) || (0 == inBNV->numSides)) && invalid_bnv_once) {
		Imp_PrintWarning("BNV file %s object %s had %d nodes and %d sides, it should have atleast one of each.  You will only get this error once per importer session.",
			inBNV->fileName,
			inBNV->objName,
			inBNV->numNodes,
			inBNV->numSides);

		invalid_bnv_once = UUcFalse;
	}

	planeArray = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);

	curNodeIndex = 0;

	while(1)
	{
		UUmAssert(curNodeIndex < inBNV->numNodes);
		curNode = inBNV->bspNodes + curNodeIndex;

		UUmAssert((curNode->planeEquIndex & 0x7FFFFFFF) < AUrSharedPlaneEquArray_GetNum(inBuildData->sharedPlaneEquArray));
		planeEqu = planeArray + (curNode->planeEquIndex & 0x7FFFFFFF);

		UUmAssertReadPtr(planeEqu, sizeof(*planeEqu));

		a = planeEqu->a;
		b = planeEqu->b;
		c = planeEqu->c;
		d = planeEqu->d;

		if(curNode->planeEquIndex & 0x80000000)
		{
			a = -a;
			b = -b;
			c = -c;
			d = -d;
		}

		threshold = (curNode->negNodeIndex == 0xFFFFFFFF) ? inTolerance : 0.0f;

		if(UUmFloat_CompareLE(inPoint->x * a +
								inPoint->y * b +
								inPoint->z * c + d, threshold))
		{
			if(0xFFFFFFFF == curNode->posNodeIndex)
			{
				return UUcTrue;
			}
			else
			{
				curNodeIndex = curNode->posNodeIndex;
			}
		}
		else
		{
			if(0xFFFFFFFF == curNode->negNodeIndex)
			{
				return UUcFalse;
			}
			else
			{
				curNodeIndex = curNode->negNodeIndex;
			}
		}
	}
}

static UUtError
IMPiEnv_Process_Alpha_BSP_Build(
	IMPtEnv_BuildData*	inBuildData,
	UUtUns32			inNumGQs,
	UUtUns32*			inGQIndices)
{
	UUtError				error;
	IMPtEnv_GQ*				curGQ;
	M3tPoint3D*				pointArray;
	M3tQuad			newQuad;
	UUtUns32				curPointIndex;
	M3tPoint3D*				targetPoint;
	UUtUns32				gqItr;
	UUtUns32				planeItr;
	IMPtEnv_BSP_Quad*		curPlaneQuad;

	UUtUns32				numPlanes = 0;
	IMPtEnv_BSP_Plane*		planes;
	UUtUns16				nodeItr;
	UUtInt64				time = UUrMachineTime_High();

	//return UUcError_None;

	Imp_PrintMessage(IMPcMsg_Important,"Building Alpha BSP tree (%d gqs)...", inNumGQs);

	inBuildData->numBSPNodes = 0;

	pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);

	UUrMemory_Pool_Reset(IMPgEnv_MemoryPool);
	AUrSharedPointArray_Reset(inBuildData->bspPointArray);
	AUrSharedQuadArray_Reset(inBuildData->bspQuadArray);

	planes =
		UUrMemory_Pool_Block_New(
			IMPgEnv_MemoryPool,
			inNumGQs * sizeof(IMPtEnv_BSP_Plane));
	if (NULL == planes) {
		Imp_PrintWarning("failed to allocate %d bytes", inNumGQs * sizeof(IMPtEnv_BSP_Plane));
		return UUcError_OutOfMemory;
	}

	for(gqItr = 0; gqItr < inNumGQs; gqItr++)
	{
		curGQ = inBuildData->gqList + inGQIndices[gqItr];

		// check to see if an existing plane exists for this gq
		for(planeItr = 0; planeItr < numPlanes; planeItr++)
		{
			if ((planes[planeItr].planeEquIndex & 0x7FFFFFFF) == (curGQ->planeEquIndex & 0x7FFFFFFF)) {
				break;
			}
		}

		// alloc new plane if needed
		if(planeItr >= numPlanes)
		{
			planes[numPlanes].planeEquIndex = curGQ->planeEquIndex;
			planes[numPlanes].numQuads = 0;
			planes[numPlanes].ref = 0x7175;	// this is not used
			planeItr = numPlanes++;
		}

		if(planes[planeItr].numQuads >= (IMPcEnv_MaxQuadsPerPlane / 2))
		{
			UUmAssert(0);
			planes[numPlanes].planeEquIndex = curGQ->planeEquIndex;
			planes[numPlanes].numQuads = 0;
			planes[numPlanes].ref = 0x7175;	// this is not used
			planeItr = numPlanes++;
		}

		// add this gq to this plane
		curPlaneQuad = planes[planeItr].quads + planes[planeItr].numQuads++;

		curPlaneQuad->ref = inGQIndices[gqItr];

		for(curPointIndex = 0; curPointIndex < 4; curPointIndex++)
		{
			targetPoint = pointArray + curGQ->visibleQuad.indices[curPointIndex];

			error =
				AUrSharedPointArray_AddPoint(
					inBuildData->bspPointArray,
					targetPoint->x,
					targetPoint->y,
					targetPoint->z,
					&newQuad.indices[curPointIndex]);
			UUmError_ReturnOnError(error);

		}

		if(newQuad.indices[2] == newQuad.indices[3])
		{
			error =
				AUrSharedQuadArray_AddQuad(
					inBuildData->bspQuadArray,
					newQuad.indices[0],
					newQuad.indices[1],
					newQuad.indices[2],
					UUcMaxUns32,
					&curPlaneQuad->bspQuadIndex);
			UUmError_ReturnOnError(error);
		}
		else
		{
			error =
				AUrSharedQuadArray_AddQuad(
					inBuildData->bspQuadArray,
					newQuad.indices[0],
					newQuad.indices[1],
					newQuad.indices[2],
					newQuad.indices[3],
					&curPlaneQuad->bspQuadIndex);
			UUmError_ReturnOnError(error);
		}
	}

	error =
		IMPiEnv_Process_BSP_Build_Recursive(
			inBuildData,
			0,
			numPlanes,
			planes,
			NULL);
	UUmError_ReturnOnError(error);

	// convert the bsp nodes into alpha bsp nodes
	{
		inBuildData->numAlphaBSPNodes = inBuildData->numBSPNodes;

		for(nodeItr = 0; nodeItr < inBuildData->numBSPNodes; nodeItr++)
		{
			inBuildData->alphaBSPNodes[nodeItr].gqIndex = inBuildData->bspNodes[nodeItr].splittingQuad.ref;
			inBuildData->alphaBSPNodes[nodeItr].planeEquIndex = inBuildData->bspNodes[nodeItr].splittingPlane->planeEquIndex;
			inBuildData->alphaBSPNodes[nodeItr].posNodeIndex = inBuildData->bspNodes[nodeItr].posNodeIndex;
			inBuildData->alphaBSPNodes[nodeItr].negNodeIndex = inBuildData->bspNodes[nodeItr].negNodeIndex;
		}
	}

	fprintf(IMPgEnv_StatsFile, "**Alpha BSP stats"UUmNL);
	fprintf(IMPgEnv_StatsFile,"\tnodes = %d\n", inBuildData->numAlphaBSPNodes);

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	return UUcError_None;
}

UUtBool
IMPiEnv_Process_BNV_ContainsBNV(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_BNV*		inParentBNV,
	IMPtEnv_BNV*		inChildBNV)
{
	UUtUns32			curQuadIndex, i;
	UUtUns32			curSideIndex;
	UUtUns32			pointCount = 0;
	IMPtEnv_BNV_Side*	curSide;

	M3tQuad*		quadList;
	M3tQuad*		curQuad;
	M3tPoint3D*			pointList;
	M3tPoint3D			centerPoint = {0,0,0};
	M3tPlaneEquation*	planeEquList;

	quadList = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);
	pointList = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	planeEquList = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);

	for(curSideIndex = 0, curSide = inChildBNV->sides;
		curSideIndex < inChildBNV->numSides;
		curSideIndex++, curSide++)
	{
		for(curQuadIndex = 0; curQuadIndex < curSide->numBNVQuads; curQuadIndex++)
		{
			curQuad = quadList + curSide->bnvQuadList[curQuadIndex];

			for(i = 0; i < 4; i++)
			{
				if (inChildBNV->flags & AKcBNV_Flag_Stairs)
				{
					// For stairs, average the points and determine parents by center point
					MUmVector_Increment(centerPoint,*(pointList + curQuad->indices[i]));
					pointCount++;
				}

				else if(IMPiEnv_Process_BSP_PointInBSP(
					inBuildData,
					inParentBNV,
					pointList + curQuad->indices[i], 0) == UUcFalse)
				{
					return UUcFalse;
				}
			}
		}
	}

	if (inChildBNV->flags & AKcBNV_Flag_Stairs)
	{
		MUmVector_Scale(centerPoint,(1.0f / (float)pointCount));
		if(IMPiEnv_Process_BSP_PointInBSP(
			inBuildData,
			inParentBNV,
			&centerPoint, 0) == UUcFalse) return UUcFalse;
	}

	return UUcTrue;
}

static void
IMPiEnv_Process_BNV_FindParents(
	IMPtEnv_BuildData*	inBuildData)
{
	IMPtEnv_BNV*	curChildBNV;
	IMPtEnv_BNV*	curParentBNV;

	UUtUns16	curChildIndex, curParentIndex;

	float		parentVolume;
	UUtUns32	parentIndex;

	UUtBool		isLeaf;
	UUtInt64	time = UUrMachineTime_High();

	Imp_PrintMessage(IMPcMsg_Important, "determining BNV parents");

	for(
		curChildIndex = 0, curChildBNV = inBuildData->bnvList;
		curChildIndex < inBuildData->numBNVs;
		curChildIndex++, curChildBNV++)
	{
		if((curChildIndex % 10) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		parentIndex = 0xFFFFFFFF;
		parentVolume = UUcFloat_Max;
		isLeaf = UUcTrue;

		// Idea here is to find the tightest fit enclosing BNV
		for(
			curParentIndex = 0, curParentBNV = inBuildData->bnvList;
			curParentIndex < inBuildData->numBNVs;
			curParentIndex++, curParentBNV++)
		{
			if(curChildIndex != curParentIndex)
			{
				if(IMPiEnv_Process_BNV_ContainsBNV(inBuildData, curParentBNV, curChildBNV))
				{
					if (!(curChildBNV->flags & AKcBNV_Flag_Stairs))
					{
						/// XXX preventing Power Plant from imping - Add this again someday? UUmAssert(curChildBNV->volume <= curParentBNV->volume);
					}

					if(curParentBNV->volume < parentVolume)
					{
						parentVolume = curParentBNV->volume;
						parentIndex = curParentIndex;
					}
				}

				if(IMPiEnv_Process_BNV_ContainsBNV(inBuildData, curChildBNV, curParentBNV))
				{
					//isLeaf = UUcFalse;
				}
			}
		}

		curChildBNV->isLeaf = isLeaf;

		if(parentIndex == 0xFFFFFFFF)
		{
			curChildBNV->parent = 0xFFFFFFFF;
		}
		else
		{
			curChildBNV->parent = parentIndex;
		}
	}

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));
}

static void
IMPiEnv_Process_BNV_AnalyzeParents_Recursive(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_BNV*		inBNV,
	UUtUns16			inParent,
	UUtUns16			inLevel)
{
	UUtUns16		curBNVIndex;
	IMPtEnv_BNV*	curBNV;
	UUtBool			foundChild;

	foundChild = UUcFalse;

	for(curBNVIndex = 0, curBNV = inBuildData->bnvList;
		curBNVIndex < inBuildData->numBNVs;
		curBNVIndex++, curBNV++)
	{
		if(curBNV->parent == inParent)
		{
			UUmAssert(0xFFFFFFFF == curBNV->level);

			foundChild = UUcTrue;

			curBNV->level = inLevel;
			IMPiEnv_Process_BNV_AnalyzeParents_Recursive(
				inBuildData,
				curBNV,
				curBNVIndex,
				inLevel+1);
		}
	}

	if(foundChild == UUcFalse)
	{
		UUmAssert(inBNV->isLeaf == UUcTrue);
	}
	else
	{
		//UUmAssert(inBNV->isLeaf == UUcFalse);
	}

	if(inBNV->isLeaf == UUcTrue)
	{
		//UUmAssert(foundChild == UUcFalse);
	}
}

static UUtError
IMPiEnv_Process_BNV_AnalyzeParents(
	IMPtEnv_BuildData*	inBuildData)
{
	UUtUns16		curBNVIndex;
	IMPtEnv_BNV*	curBNV;
//	UUtBool			isRoom;
//	UUtUns16		curChildIndex;
//	IMPtEnv_BNV*	curChildBNV;
	UUtUns16*		bnvInsertionArray;
	UUtInt64		time = UUrMachineTime_High();

	Imp_PrintMessage(IMPcMsg_Important, "analyzing BNV parents");

	bnvInsertionArray = UUrMemory_Block_NewClear(sizeof(UUtUns16) * inBuildData->numBNVs);
	UUmError_ReturnOnNull(bnvInsertionArray);

	// compute the levels
	for(curBNVIndex = 0, curBNV = inBuildData->bnvList;
		curBNVIndex < inBuildData->numBNVs;
		curBNVIndex++, curBNV++)
	{
		if((curBNVIndex % 30) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		if(0xFFFFFFFF == curBNV->parent)
		{
			curBNV->level = 0;

			IMPiEnv_Process_BNV_AnalyzeParents_Recursive(inBuildData, curBNV, curBNVIndex, 1);
		}
	}

	// Compute the child and next pointers
	for(curBNVIndex = 0, curBNV = inBuildData->bnvList;
		curBNVIndex < inBuildData->numBNVs;
		curBNVIndex++, curBNV++)
	{
		if((curBNVIndex % 30) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		if(0xFFFFFFFF == curBNV->parent)
		{
			// Do nothing
		}
		else if(bnvInsertionArray[curBNV->parent] == 0)
		{
			inBuildData->bnvList[curBNV->parent].child = curBNVIndex;
			bnvInsertionArray[curBNV->parent] = curBNVIndex;
		}
		else
		{
			inBuildData->bnvList[bnvInsertionArray[curBNV->parent]].next = curBNVIndex;
			bnvInsertionArray[curBNV->parent] = curBNVIndex;
		}
	}

	// Do one more pass for room flag and error reporting
	for(curBNVIndex = 0, curBNV = inBuildData->bnvList;
		curBNVIndex < inBuildData->numBNVs;
		curBNVIndex++, curBNV++)
	{
		if((curBNVIndex % 30) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		if(curBNV->isLeaf == UUcTrue || 1)
		{
			curBNV->flags |= AKcBNV_Flag_Room;
		}
	}

	UUrMemory_Block_Delete(bnvInsertionArray);

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	return UUcError_None;
}

static UUtBool
IMPiEnv_Process_BNV_QuadLiesOnSide(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_BNV_Side*	inBNVSide,
	UUtUns32			inQuadIndex,
	UUtUns32			inPlaneEquIndex)
{
	UUtUns16			curPointIndex;
	UUtUns16			curQuadIndex;

	M3tPoint3D*			pointArray;
	M3tQuad*		quadArray;

	M3tPoint3D*			curPoint;
	M3tPoint3D			centerPoint;

	float				minX, minY, minZ;
	float				maxX, maxY, maxZ;

	pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	quadArray = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);

	if(AKmPlaneEqu_GetIndex(inBNVSide->planeEquIndex) != AKmPlaneEqu_GetIndex(inPlaneEquIndex))
	{
		return UUcFalse;
	}

	// Compute center of this quad
	curPoint = pointArray +	quadArray[inQuadIndex].indices[0];

	minX = maxX = curPoint->x;
	minY = maxY = curPoint->y;
	minZ = maxZ = curPoint->z;

	for(curPointIndex = 1; curPointIndex < 4; curPointIndex++)
	{
		curPoint = pointArray +	quadArray[inQuadIndex].indices[curPointIndex];

		UUmMinMax(curPoint->x, minX, maxX);
		UUmMinMax(curPoint->y, minY, maxY);
		UUmMinMax(curPoint->z, minZ, maxZ);
	}

	centerPoint.x = (minX + maxX) * 0.5f;
	centerPoint.y = (minY + maxY) * 0.5f;
	centerPoint.z = (minZ + maxZ) * 0.5f;

	for(curQuadIndex = 0; curQuadIndex < inBNVSide->numBNVQuads; curQuadIndex++)
	{
		if(CLrQuad_PointInQuad(
			CLcProjection_Unknown,
			pointArray,
			quadArray + inBNVSide->bnvQuadList[curQuadIndex],
			&centerPoint) == UUcTrue)
		{
			return UUcTrue;
		}
	}

	return UUcFalse;
}

static UUtError
IMPrEnv_Process_BNV_BuildBSP(
	IMPtEnv_BuildData*	inBuildData)
{
	UUtUns16			curBNVIndex;
	IMPtEnv_BNV*		curBNV;
	UUtInt64			time = UUrMachineTime_High();


	Imp_PrintMessage(IMPcMsg_Important, UUmNL"computing BNV BSP trees");

	for(curBNVIndex = 0, curBNV = inBuildData->bnvList;
		curBNVIndex < inBuildData->numBNVs;
		curBNVIndex++, curBNV++)
	{
		if((curBNVIndex % 10) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		IMPiEnv_Process_BSP_BNV_Build(inBuildData, curBNV);

	}

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	return UUcError_None;
}

static UUtBool
IsGhostQuadOnThisSide(
	IMPtEnv_BuildData *inBuildData,
	IMPtEnv_BNV_Side *curSide,
	IMPtEnv_GQ *current_ghost_quad)
{
	// RULE #1 the planes must be almost coplanar (normal facing doesn't matter)
	// RULE #2 each vertex of the ghost quad must be on this BNV side

	M3tPlaneEquation *planes = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);
	M3tPoint3D *sharedPoints = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	M3tQuad*		sharedQuads = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);
	UUtUns32 curVertexIndex;
	UUtBool is_ghost_quad_on_this_side = UUcFalse;

	// RULE #1 the planes must be almost coplanar (normal facing doesn't matter)
	if ((current_ghost_quad->planeEquIndex & 0x7FFFFFFF) != (curSide->planeEquIndex & 0x7FFFFFFF)) {
		M3tPlaneEquation *p1 = planes + (curSide->planeEquIndex & 0x7FFFFFFF);	// ignore facing bit
		M3tPlaneEquation *p2 = planes + (current_ghost_quad->planeEquIndex & 0x7FFFFFFF);	// ignore facing bit

		if (!MUrPlaneEquation_IsClose(p1, p2)) {
			goto exit;
		}
	}

	// RULE #2 each vertex of the ghost quad must be on this BNV
	for(curVertexIndex = 0; curVertexIndex < 4; curVertexIndex++)
	{
		M3tPoint3D *ghost_quad_vertex = sharedPoints + current_ghost_quad->visibleQuad.indices[curVertexIndex];
		UUtBool ghost_quad_vertex_lies_on_this_side = UUcFalse;
		UUtUns32 curQuadIndex;

		// this ghost quad vertex must lie on one of the faces that makes up the plane of this bnv
		for(curQuadIndex = 0; curQuadIndex < curSide->numBNVQuads; curQuadIndex++)
		{
			M3tQuad *current_bnv_face_quad = sharedQuads + curSide->bnvQuadList[curQuadIndex];

			if (CLrQuad_PointInQuad(CLcProjection_Unknown, sharedPoints, current_bnv_face_quad, ghost_quad_vertex)) {
				ghost_quad_vertex_lies_on_this_side = UUcTrue;
				break;
			}
		}

		if (!ghost_quad_vertex_lies_on_this_side) {
			goto exit;
		}
	}

	is_ghost_quad_on_this_side = UUcTrue;

exit:
	return is_ghost_quad_on_this_side;
}

static void FindSpecialQuadsOnThisSide(IMPtEnv_BuildData *inBuildData, IMPtEnv_BNV *curBNV, IMPtEnv_BNV_Side *curSide, UUtUns32 *special_quad_indicies, UUtUns32 num_special_quads)
{
	UUtUns32 special_quad_itr;
	M3tPoint3D *sharedPoints = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);

	for(special_quad_itr = 0; special_quad_itr < num_special_quads; special_quad_itr++)
	{
		UUtUns32 current_ghost_index = special_quad_indicies[special_quad_itr];
		IMPtEnv_GQ *current_ghost_quad = inBuildData->gqList + current_ghost_index;

		// only test quads near us
		if (!AUrDict_Test(inBuildData->fixed_oct_tree_temp_list, current_ghost_index)) {
			continue;
		}

		UUmAssert((current_ghost_quad->flags & (AKcGQ_Flag_Ghost | AKcGQ_Flag_SAT_Up | AKcGQ_Flag_SAT_Down)));

		if (!IsGhostQuadOnThisSide(inBuildData, curSide, current_ghost_quad)) {
			continue;
		}

		// if this is not a stair BNV then this must be close to the floor
		if (!(curBNV->flags & AKcBNV_Flag_Stairs)) {
			M3tPoint3D *lowest_point;

			AUrQuad_LowestPoints(&current_ghost_quad->visibleQuad,sharedPoints,&lowest_point,NULL);

			if ((float)fabs(lowest_point->y - curBNV->minY) > IMPcGhostFloorThreshold) {
				continue;
			}
		}

		// if we get here then current_ghost_quad is on this side
		if(curSide->numGQGhostIndices > IMPcEnv_MaxQuadsPerSide) {
			Imp_PrintWarning("Exceeded max number of GQ ghost things FILE %s LINE %d",__FILE__, __LINE__);
		}

		curSide->gqGhostIndices[curSide->numGQGhostIndices++] = current_ghost_index;
	}

	return;
}


// ******* ADD ADDITIONAL BNV STUFF HERE XYZZY
static void IMP_BNV_Compute_BoundingBox_And_Volume(IMPtEnv_BuildData *inBuildData, IMPtEnv_BNV *curBNV)
{
	M3tPoint3D *sharedPoints = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	M3tQuad	*sharedQuads = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);
	UUtUns32 curSideIndex;
	IMPtEnv_BNV_Side*	curSide;

	curBNV->minX = curBNV->maxX = sharedPoints[sharedQuads[curBNV->sides->bnvQuadList[0]].indices[0]].x;
	curBNV->minY = curBNV->maxY = sharedPoints[sharedQuads[curBNV->sides->bnvQuadList[0]].indices[0]].y;
	curBNV->minZ = curBNV->maxZ = sharedPoints[sharedQuads[curBNV->sides->bnvQuadList[0]].indices[0]].z;

	for(curSideIndex = 0, curSide = curBNV->sides;
		curSideIndex < curBNV->numSides;
		curSideIndex++, curSide++)
	{
		UUtUns32 curQuadIndex;

		for(curQuadIndex = 0; curQuadIndex < curSide->numBNVQuads; curQuadIndex++)
		{
			UUtUns32 curPointIndex;

			for(curPointIndex = 0; curPointIndex < 4; curPointIndex++)
			{
				const M3tPoint3D *curPoint = sharedPoints + sharedQuads[curSide->bnvQuadList[curQuadIndex]].indices[curPointIndex];

				UUmMinMax(curPoint->x, curBNV->minX, curBNV->maxX);
				UUmMinMax(curPoint->y, curBNV->minY, curBNV->maxY);
				UUmMinMax(curPoint->z, curBNV->minZ, curBNV->maxZ);
			}
		}
	}

	// compute the volume
	curBNV->volume = (curBNV->maxX - curBNV->minX) * (curBNV->maxY - curBNV->minY) * (curBNV->maxZ - curBNV->minZ);

	return;
}

UUtError
IMPrEnv_Process_BNV_ComputeProperties(
	IMPtEnv_BuildData*	inBuildData)
{
	UUtUns16			curBNVIndex, curSideIndex;
	IMPtEnv_BNV*		curBNV;
	IMPtEnv_BNV_Side*	curSide;

	M3tPoint3D*			curPoint;
	M3tPoint3D*			sharedPoints;
	M3tQuad*		sharedQuads;

	UUtUns32			curGQIndex;
	UUtUns32			curVertexIndex;

	IMPtEnv_GQ*			curGQ;


	M3tPlaneEquation*	planes;

	UUtInt64			time;

	UUtUns32			num_special_quads;
	UUtUns32			special_quad_indicies[2000];

	UUtUns32			non_ai_bnvs = 0;

	time = UUrMachineTime_High();
	Imp_PrintMessage(IMPcMsg_Important, "finding ghost quads and stair alignment quads");

	sharedQuads = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);
	sharedPoints = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	planes = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);

	num_special_quads = 0;
	for(curGQIndex = 0, curGQ = inBuildData->gqList; curGQIndex < inBuildData->numGQs; curGQIndex++, curGQ++)
	{
		if (curGQ->flags & (AKcGQ_Flag_Ghost | AKcGQ_Flag_SAT_Up | AKcGQ_Flag_SAT_Down)) {
			if (curGQ->flags & AKcGQ_Flag_Triangle) {
				M3tPoint3D *point = &sharedPoints[curGQ->visibleQuad.indices[0]];

				Imp_PrintWarning("triangular ghost quad (gq %d) at [%f, %f, %f], file %s obj %s",
								curGQIndex, point->x, point->y, point->z, curGQ->fileName, curGQ->objName);
			}
			special_quad_indicies[num_special_quads] = curGQIndex;
			num_special_quads++;
		}

		if (num_special_quads > 2000) {
			Imp_PrintWarning("to many ghost quads & stair alignment quads");
			return UUcError_Generic;
		}
	}

	Imp_PrintMessage(IMPcMsg_Important, " (%d)", num_special_quads);

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	time = UUrMachineTime_High();
	Imp_PrintMessage(IMPcMsg_Important, "computing BNV properties");

	for(curBNVIndex = 0, curBNV = inBuildData->bnvList;
		curBNVIndex < inBuildData->numBNVs;
		curBNVIndex++, curBNV++)
	{
		if((curBNVIndex % 10) == 0) {
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		// Check for BNVs less then 5 sides
		if(curBNV->numSides < 5) {
			Imp_PrintWarning("BNV %d has less then 5 sides File: %s, Obj: %s", curBNVIndex, curBNV->fileName, curBNV->objName);
		}

		// Compute bounding box
		IMP_BNV_Compute_BoundingBox_And_Volume(inBuildData, curBNV);

		// test the oct tree
		IMPrFixedOctTree_Test(
			inBuildData,
			curBNV->minX - 2.f,
			curBNV->maxX + 2.f,
			curBNV->minY - 2.f,
			curBNV->maxY + 2.f,
			curBNV->minZ - 2.f,
			curBNV->maxZ + 2.f);

		// we are going to build up the gq ghost indices for each side of this BNV
		for(curSideIndex = 0; curSideIndex < curBNV->numSides; curSideIndex++)
		{
			// RULE #1 must be an AI BNV

			curSide = curBNV->sides + curSideIndex;

			if (curBNV->flags & AKcBNV_Flag_NonAI) {
				non_ai_bnvs += 1;
				continue;
			}

			FindSpecialQuadsOnThisSide(inBuildData, curBNV, curSide, special_quad_indicies, num_special_quads);
		}

#if defined(DEBUGGING) && DEBUGGING
		// CB: this test makes sure that a ghost quad isn't mistakenly added to multiple sides
		for (curSideIndex = 0; curSideIndex < curBNV->numSides; curSideIndex++) {
			curSide = curBNV->sides + curSideIndex;

			for (curGQIndex=0; curGQIndex<curSide->numGQGhostIndices; curGQIndex++) {
				UUtUns32 check_gq = curSide->gqGhostIndices[curGQIndex];
				UUtUns32 curSideIndex2, curGQIndex2;

				for (curSideIndex2 = 0; curSideIndex2 < curSideIndex; curSideIndex2++) {
					IMPtEnv_BNV_Side *curSide2 = curBNV->sides + curSideIndex2;

					for (curGQIndex2 = 0; curGQIndex2 < curSide2->numGQGhostIndices; curGQIndex2++) {
						UUtUns32 other_gq = curSide2->gqGhostIndices[curGQIndex2];

						if (check_gq == other_gq) {
							Imp_PrintMessage(IMPcMsg_Important, "WARNING: BNV %d has the same special quad on multiple sides! (%d, %d and %d, %d)",
								curBNVIndex, curSideIndex, curGQIndex, curSideIndex2, curGQIndex2);
						}
					}
				}
			}
		}
#endif

		// find every gq that has a vertex inside this BNV
		for(curGQIndex = 0;	curGQIndex < inBuildData->numGQs; curGQIndex++)
		{
			if (!AUrDict_Test(inBuildData->fixed_oct_tree_temp_list, curGQIndex)) {
				continue;
			}

			curGQ = inBuildData->gqList + curGQIndex;

			for(curVertexIndex = 0; curVertexIndex < 4; curVertexIndex++)
			{
				curPoint = sharedPoints + curGQ->visibleQuad.indices[curVertexIndex];

				if (IMPiEnv_Process_BSP_PointInBSP(inBuildData, curBNV, curPoint, 0)) {

					if (curBNV->numGQs >= IMPcEnv_MaxGQsPerBNV) {
						Imp_PrintWarning("Exceeded IMPcEnv_MaxGQsPerBNV in FILE: \"%s\" OBJ: \"%s\" max is %d",
							curBNV->fileName,
							curBNV->objName,
							IMPcEnv_MaxGQsPerBNV);
					}

					curBNV->gqList[curBNV->numGQs++] = curGQIndex;

					break;
				}
			}
		}

		// Compute plane of stairs between bottoms of SATs for standard stairs
		if (curBNV->flags & AKcBNV_Flag_Stairs_Standard) {
			M3tPoint3D *a, *b, *up0,*up1,*dn0,*dn1, *ph0, *ph1, *ph2, *ph3;
			M3tVector3D n;
			float h0, h1, h2, h3;
			UUtUns32 upsat_number = 0, downsat_number = 0;
			IMPtEnv_GQ *upsat, *downsat;

			up0 = up1 = dn0 = dn1 = NULL;
			upsat = downsat = NULL;
			for(curSideIndex = 0, curSide = curBNV->sides;
				curSideIndex < curBNV->numSides;
				curSideIndex++, curSide++)
			{
				for (curGQIndex=0; curGQIndex<curSide->numGQGhostIndices; curGQIndex++)
				{
					curGQ = inBuildData->gqList + curSide->gqGhostIndices[curGQIndex];
					AUrQuad_LowestPoints(&curGQ->visibleQuad,sharedPoints,&a,&b);

					if (curGQ->flags & AKcGQ_Flag_SAT_Down) {
						if ((dn0 == a) && (dn1 == b)) {
							// don't consider a quad if it's already our current down SAT
						} else {
							downsat_number++;

							if ((dn0 == NULL) || (a->y > dn0->y)) {
								// take the highest of the down SATs
								downsat = curGQ;
								dn0 = a;
								dn1 = b;
								AUrQuad_HighestPoints(&curGQ->visibleQuad,sharedPoints,&ph0,&ph1);
							}
						}
					}

					if (curGQ->flags & AKcGQ_Flag_SAT_Up) {
						if ((up0 == a) && (up1 == b)) {
							// don't consider a quad if it's already our current down SAT
						} else {
							upsat_number++;

							if ((up0 == NULL) || (a->y < up0->y)) {
								// take the lowest of the up SATs
								upsat = curGQ;
								up0 = a;
								up1 = b;
								AUrQuad_HighestPoints(&curGQ->visibleQuad,sharedPoints,&ph2,&ph3);
							}
						}
					}
				}
			}

			if ((upsat_number != 1) || (downsat_number != 1)) {
				Imp_PrintMessage(IMPcMsg_Important, "file: %s, obj:%s - stair BNV doesn't have correct SAT count (%d up, %d down)",
								curBNV->fileName, curBNV->objName, upsat_number, downsat_number);

				if ((upsat_number == 0) || (downsat_number == 0)) {
					// can't create this stair BNV's plane and height information, bail
					UUrMemory_Clear(&curBNV->stairPlane,sizeof(M3tPlaneEquation));
					curBNV->stairHeight = 0;
					continue;
				}
			}

			UUmAssert((upsat != NULL) && (downsat != NULL));
			upsat->stairBNVIndex = curBNVIndex;
			downsat->stairBNVIndex = curBNVIndex;

			MUrVector_NormalFromPoints(dn0,dn1,up0,&n);

			if (n.y < 0.0f) {
				MUmVector_Negate(n);
			}

			curBNV->stairPlane.a = n.x;
			curBNV->stairPlane.b = n.y;
			curBNV->stairPlane.c = n.z;
			curBNV->stairPlane.d = -(n.x*dn0->x + n.y*dn0->y + n.z*dn0->z);

			// calculate the heights of the tops of the ghost quads above this plane
			h0 = curBNV->stairPlane.a * ph0->x + curBNV->stairPlane.b * ph0->y + curBNV->stairPlane.c * ph0->z + curBNV->stairPlane.d;
			h1 = curBNV->stairPlane.a * ph1->x + curBNV->stairPlane.b * ph1->y + curBNV->stairPlane.c * ph1->z + curBNV->stairPlane.d;
			h2 = curBNV->stairPlane.a * ph2->x + curBNV->stairPlane.b * ph2->y + curBNV->stairPlane.c * ph2->z + curBNV->stairPlane.d;
			h3 = curBNV->stairPlane.a * ph3->x + curBNV->stairPlane.b * ph3->y + curBNV->stairPlane.c * ph3->z + curBNV->stairPlane.d;

			curBNV->stairHeight = UUmMax4(h0, h1, h2, h3);
		}
		else {
			UUrMemory_Clear(&curBNV->stairPlane,sizeof(M3tPlaneEquation));
			curBNV->stairHeight = 0;
		}
	}

	Imp_PrintMessage(IMPcMsg_Important, " (non ai %d/%d)", non_ai_bnvs, inBuildData->numBNVs);

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	return UUcError_None;
}

void
IMPrEnv_Process_BNV_ComputeAdjacency(
	IMPtEnv_BuildData*	inBuildData)
{
	// Create the adjacency data for each node - Warning! Icky O(n^2) algorithm! Ew! Eek!

	UUtUns32 curBNVIndex,curSideIndex,curGQIndex;
	UUtUns32 curBNVIndex2,curSideIndex2,curGQIndex2;
	IMPtEnv_BNV *curBNV,*curBNV2;
	IMPtEnv_BNV_Side *curSide,*curSide2;
	IMPtEnv_GQ *curGQ;
	UUtInt64 time = UUrMachineTime_High();

	//#define DEBUG_ADJ
	Imp_PrintMessage(IMPcMsg_Important,"Finding adjacencies...");
	IMPgEnv_VerifyAdjCount = 0;

	for(curBNVIndex = 0; curBNVIndex < inBuildData->numBNVs; curBNVIndex++)
	{
		if((curBNVIndex % 20) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		// Setup
		#ifdef DEBUG_ADJ
		Imp_PrintMessage(IMPcMsg_Important,"- Adjacency for node %d -"UUmNL,curBNVIndex);
		#endif
		curBNV = &inBuildData->bnvList[curBNVIndex];

		// Cycle through all the sides of the node
		for (curSideIndex = 0; curSideIndex < curBNV->numSides; curSideIndex++)
		{
			// Cycle through all ghosts of the side
			curSide = &curBNV->sides[curSideIndex];
			curSide->numAdjacencies = 0;
			if (!(curBNV->flags & AKcBNV_Flag_Room)) continue;	// Tree node sides are done now
			if (curBNV->flags & AKcBNV_Flag_NonAI) continue;

			for (curGQIndex=0; curGQIndex<curSide->numGQGhostIndices; curGQIndex++)
			{
				// Now check all the other nodes for matching ghosts
				for(curBNVIndex2 = 0; curBNVIndex2 < inBuildData->numBNVs; curBNVIndex2++)
				{
					if (curBNVIndex2 == curBNVIndex) continue;
					curBNV2 = &inBuildData->bnvList[curBNVIndex2];
					if (!(curBNV2->flags & AKcBNV_Flag_Room)) continue;	// Only room nodes!
					if ((curBNV2->flags & AKcBNV_Flag_NonAI)) continue;	// Skip non AI

					// Handle normal adjacencies
					for (curSideIndex2 = 0; curSideIndex2 < curBNV2->numSides; curSideIndex2++)
					{
						curSide2 = &curBNV2->sides[curSideIndex2];
						for (curGQIndex2=0; curGQIndex2<curSide2->numGQGhostIndices; curGQIndex2++)
						{
							if (curSide2->gqGhostIndices[curGQIndex2] == curSide->gqGhostIndices[curGQIndex])
							{
								// Found a shared ghost- make a note of it
								Imp_Env_AddAdjacency(curSide,curBNV,curSide2,curBNVIndex2,curSide2->gqGhostIndices[curGQIndex2],0,inBuildData);

								#ifdef DEBUG_ADJ
								Imp_PrintMessage(IMPcMsg_Important,"\tNode %d, sharing ghost #%d"UUmNL,curBNVIndex2,curSide2->gqGhostIndices[curGQIndex2]);
								#endif
							}
						}
					}
				}
			}
		}
	}

	// Check for stair adjacencies
	for(curBNVIndex = 0; curBNVIndex < inBuildData->numBNVs; curBNVIndex++)
	{
		if((curBNVIndex % 20) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		curBNV = &inBuildData->bnvList[curBNVIndex];
		if (!(curBNV->flags & AKcBNV_Flag_Room)) continue;	// Only room nodes from here on!
		if (curBNV->flags & AKcBNV_Flag_Stairs) continue;	// Skip stairs at this stage
		if (curBNV->flags & AKcBNV_Flag_NonAI) continue;	// Skip non AI

		for(curBNVIndex2 = 0; curBNVIndex2 < inBuildData->numBNVs; curBNVIndex2++)
		{
			if (curBNVIndex2 == curBNVIndex) continue;
			curBNV2 = &inBuildData->bnvList[curBNVIndex2];
			if (!(curBNV2->flags & AKcBNV_Flag_Room)) continue;	// Only room nodes!
			if (!(curBNV2->flags & AKcBNV_Flag_Stairs)) continue;	// Only stairs!
			if (curBNV->flags & AKcBNV_Flag_NonAI) continue;	// Skip non AI

			for (curSideIndex2 = 0; curSideIndex2 < curBNV2->numSides; curSideIndex2++)
			{
				curSide2 = &curBNV2->sides[curSideIndex2];
				for (curGQIndex2=0; curGQIndex2<curSide2->numGQGhostIndices; curGQIndex2++)
				{
					M3tPoint3D centroid;
					UUtUns32 curGQGlobalIndex;

					curGQGlobalIndex = curSide2->gqGhostIndices[curGQIndex2];
					curGQ = &inBuildData->gqList[curGQGlobalIndex];
					if (!(curGQ->flags & (AKcGQ_Flag_SAT_Up | AKcGQ_Flag_SAT_Down))) continue;

					AUrQuad_ComputeCenter(
						AUrSharedPointArray_GetList(inBuildData->sharedPointArray),
						&curGQ->visibleQuad,
						&centroid.x,
						&centroid.y,
						&centroid.z);

					if (Imp_Env_PointInNode(&centroid,inBuildData,curBNV))
					{
						M3tPoint3D *lowPoint;
						UUtUns32 adj_index, fwd_conn_toside;
						UUtBool found_fwd_conn, found_back_conn;

						// Potential SAT- make sure SAT touches floor
						AUrQuad_LowestPoints(&curGQ->visibleQuad,AUrSharedPointArray_GetList(inBuildData->sharedPointArray),&lowPoint,NULL);
						if ((float)fabs(lowPoint->y - curBNV->origin.y) > IMPcGhostFloorThreshold)
							continue;

						// CB: look to see if this SAT already has an adjacency in the non-stair BNV. if it does,
						// then don't add it again
						found_fwd_conn = UUcFalse;
						fwd_conn_toside = 0;	// default side to add back conn to if we don't find an adjacency
						for (curSideIndex = 0; (!found_fwd_conn) && (curSideIndex < curBNV->numSides); curSideIndex++) {
							curSide = &curBNV->sides[curSideIndex];

							for (adj_index = 0; adj_index < curSide->numAdjacencies; adj_index++) {
								if (curSide->adjacencyList[adj_index].adjacentGQIndex == curGQGlobalIndex) {
									found_fwd_conn = UUcTrue;
									fwd_conn_toside = curSideIndex;
									break;
								}
							}
						}

						if (!found_fwd_conn) {
							// the BNV does not already have an adjacency for the SAT, therefore the stair lets out into
							// the middle of the BNV. just add the adjacency to side 0 of the BNV that's being let out
							// into.
							Imp_Env_AddAdjacency(&curBNV->sides[0],curBNV,curSide2,curBNVIndex2,curGQGlobalIndex,0,inBuildData);
						}

						// CB: look to see if this SAT already has an adjacency in the stair BNV. if it does,
						// then don't add it again. we only have to look on the side that we found the ghost quad
						// on (curSide2).
						found_back_conn = UUcFalse;
						for (adj_index = 0; adj_index < curSide2->numAdjacencies; adj_index++) {
							if (curSide2->adjacencyList[adj_index].adjacentGQIndex == curGQGlobalIndex) {
								found_back_conn = UUcTrue;
								break;
							}
						}

						if (!found_back_conn) {
							// add an adjacency from the stair BNV to the BNV that it connects to.
							Imp_Env_AddAdjacency(curSide2,curBNV2,&curBNV->sides[fwd_conn_toside],curBNVIndex,curGQGlobalIndex,0,inBuildData);
						}

						#ifdef DEBUG_ADJ
						Imp_PrintMessage(IMPcMsg_Important,"\tStairs %d sharing SAT #%d"UUmNL,curBNVIndex2,curSide2->gqGhostIndices[curGQIndex2]);
						#endif
					}
				}
			}
		}
	}

#if defined(DEBUGGING) && DEBUGGING
	// CB: this test makes sure that no BNVs have the same adjacency on multiple sides
	for(curBNVIndex = 0; curBNVIndex < inBuildData->numBNVs; curBNVIndex++)
	{
		UUtUns32 curAdjIndex, curAdjIndex2;

		curBNV = &inBuildData->bnvList[curBNVIndex];

		for (curSideIndex = 0; curSideIndex < curBNV->numSides; curSideIndex++) {
			curSide = curBNV->sides + curSideIndex;

			for (curAdjIndex = 0; curAdjIndex < curSide->numAdjacencies; curAdjIndex++) {
				UUtUns32 check_gq = curSide->adjacencyList[curAdjIndex].adjacentGQIndex;

				for (curSideIndex2 = 0; curSideIndex2 < curSideIndex; curSideIndex2++) {
					curSide2 = curBNV->sides + curSideIndex2;

					for (curAdjIndex2 = 0; curAdjIndex2 < curSide2->numAdjacencies; curAdjIndex2++) {
						UUtUns32 other_gq = curSide2->adjacencyList[curAdjIndex2].adjacentGQIndex;

						if (check_gq == other_gq) {
							Imp_PrintMessage(IMPcMsg_Important, "WARNING: BNV %d has the same adjacency on multiple sides! (%d, %d and %d, %d)",
								curBNVIndex, curSideIndex, curAdjIndex, curSideIndex2, curAdjIndex2);
						}
					}
				}
			}
		}
	}
#endif

	Imp_PrintMessage(IMPcMsg_Important,"  %d",IMPgEnv_VerifyAdjCount);
	#ifdef DEBUG_ADJ
	getchar();
	#endif

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));
}


void Imp_Env_AddAdjacency(
	IMPtEnv_BNV_Side *inSrcSide,
	IMPtEnv_BNV *inSrcBNV,
	IMPtEnv_BNV_Side *inDestSide,
	UUtUns32 inDestBNVIndex,
	UUtUns32 inConnectingGQIndex,
	UUtUns16 inFlags,
	IMPtEnv_BuildData *inBuildData)
{
	/***************
	* Adds an adjacency from inSrcSide to inDestSide (which is part of inBNVIndex)
	*/

	AKtAdjacency *adj = &inSrcSide->adjacencyList[inSrcSide->numAdjacencies];

	adj->adjacentBNVIndex = inDestBNVIndex;
	adj->adjacentGQIndex = inConnectingGQIndex;
	adj->adjacencyFlags = inFlags;

	inBuildData->gqList[inConnectingGQIndex].ghostUsed = UUcTrue;

	inSrcSide->numAdjacencies++;
	IMPgEnv_VerifyAdjCount++;

	if (inSrcSide->numAdjacencies > IMPcEnv_MaxAdjacencies)
	{
		inSrcSide->numAdjacencies--;
		IMPrEnv_LogError("Adjacency overflow, file %s, object %s"UUmNL,inSrcBNV->fileName,inSrcBNV->objName);
	}
}

static UUtError
IMPiEnv_Process_Stairs(
	IMPtEnv_BuildData*	inBuildData)
{
	// This function strikes me as highly obsolete, but I'm not totally sure what it does.

	UUtUns16			curBNVIndex;
	IMPtEnv_BNV*		curBNV;
	M3tPlaneEquation	*planeArray;
	UUtUns32			curGQIndex;
	IMPtEnv_GQ*			curGQ;

	M3tQuad*	quadArray;

	UUtUns32			curPointIndex;

	M3tPoint3D*			pointArray;
	M3tPoint3D*			curPoint;

	UUtBool				gqIsEnclosed;

	UUtUns16			numStairGQs;
	UUtInt64			time = UUrMachineTime_High();

	Imp_PrintMessage(IMPcMsg_Important, "Finding and processing stairs");

	pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	quadArray = AUrSharedQuadArray_GetList(inBuildData->sharedBNVQuadArray);
	planeArray = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);

	for(curBNVIndex = 0, curBNV = inBuildData->bnvList;
		curBNVIndex < inBuildData->numBNVs;
		curBNVIndex++, curBNV++)
	{
		if((curBNVIndex % 30) == 0) {
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		if(!(curBNV->flags & AKcBNV_Flag_Stairs)) {
			continue;
		}

		// Next find the enclosed stairs
		numStairGQs = 0;
		for(curGQIndex = 0, curGQ = inBuildData->gqList; curGQIndex < inBuildData->numGQs; curGQIndex++, curGQ++)
		{
			gqIsEnclosed = UUcTrue;

			for(curPointIndex = 0; curPointIndex < 4; curPointIndex++)
			{
				curPoint = pointArray + curGQ->visibleQuad.indices[curPointIndex];

				if(IMPiEnv_Process_BSP_PointInBSP(
					inBuildData,
					curBNV,
					curPoint, 0) == UUcFalse)
				{
					gqIsEnclosed = UUcFalse;
					goto gqDone;
				}
			}

gqDone:
			if(gqIsEnclosed)
			{
				// This may be a stair- is it pointing up or down?
				if ((float)fabs(planeArray[curGQ->planeEquIndex & 0x7FFFFFFF].b) > PHcFlatNormal)
				{
					curGQ->flags |= AKcGQ_Flag_Stairs;
					curGQ->used = UUcTrue;
					numStairGQs++;
					//curBNV->stairGQIndices[curBNV->numStairGQs++] = curGQIndex;
				}
			}
		}

		if(numStairGQs == 0)
		{
			IMPrEnv_LogError(
				"BNV index: %d, file: %s, Obj: %s -> Could not find any stairs",
				curBNVIndex,
				curBNV->fileName,
				curBNV->objName);
			curBNV->error = UUcTrue;
		}
	}

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	return UUcError_None;
}

static UUtError
IMPrEnv_Process_GQVis(
	IMPtEnv_BuildData*	inBuildData)
{
	UUtError		error;

	UUtUns32		curGQIndex;
	IMPtEnv_GQ*		curGQ;
	IMPtEnv_GQ*		targetGQ;

	M3tPoint3D*		sharedPoints;

	UUtUns32*			gqTakenBV;

	IMPtEnv_GQPatch*	newPatch;

	UUtUns32			nextIndIndex;

	UUtUns32			newIndIndexIndex;

	UUtUns32			curPatchIndex;
	IMPtEnv_GQPatch*	curPatch;

	UUtUns32			curIndIndex;
	UUtUns32			nextGQIndex;
	UUtUns32			curEdgeItr;


	AUtEdge*			edgeList;
	AUtEdge*			targetEdge;

	UUtUns32			targetGQIndex;
	UUtUns32			curQuadItr;
	UUtUns32			numEdges;
	float				totalGQArea = 0.0f;
	UUtInt64			time = UUrMachineTime_High();

	Imp_PrintMessage(IMPcMsg_Important, "Computing gq patches for visibility...");

	inBuildData->numPatches = 0;
	inBuildData->numPatchIndIndices = 0;

	sharedPoints = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);

	gqTakenBV = UUrBitVector_New(inBuildData->numGQs);
	UUmError_ReturnOnNull(gqTakenBV);

	UUrBitVector_ClearBitAll(gqTakenBV, inBuildData->numGQs);

	Imp_PrintMessage(IMPcMsg_Cosmetic, UUmNL "\tbuilding gq edge list");
	for(curGQIndex = 0, curGQ = inBuildData->gqList;
		curGQIndex < inBuildData->numGQs;
		curGQIndex++, curGQ++)
	{
		if((curGQIndex % 5000) == 0) {
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		if(curGQ->flags & AKcGQ_Flag_NoTextureMask)
		{
			UUrBitVector_SetBit(gqTakenBV, curGQIndex);
			continue;
		}

		if(curGQ->flags & AKcGQ_Flag_Triangle)
		{
			error =
				AUrSharedEdgeArray_AddPoly(
					inBuildData->edgeArray,
					3,
					curGQ->visibleQuad.indices,
					curGQIndex,
					curGQ->edgeIndices.indices);
			UUmError_ReturnOnError(error);
		}
		else
		{
			error =
				AUrSharedEdgeArray_AddPoly(
					inBuildData->edgeArray,
					4,
					curGQ->visibleQuad.indices,
					curGQIndex,
					curGQ->edgeIndices.indices);
			UUmError_ReturnOnError(error);
		}
	}

	Imp_PrintMessage(IMPcMsg_Cosmetic, UUmNL"\tbuilding patch list");

	edgeList = AUrSharedEdgeArray_GetList(inBuildData->edgeArray);

	// Build a list of patches. each patch contains a list of gqs that share an edge
	for(curGQIndex = 0, curGQ = inBuildData->gqList;
		curGQIndex < inBuildData->numGQs;
		curGQIndex++, curGQ++)
	{
		// look for a gq that has not been taken
		if(UUrBitVector_TestAndSetBit(gqTakenBV, curGQIndex)) continue;

		// we now have a gq that has not been taken

		// get a new patch
			if(inBuildData->numPatches >= IMPcEnv_MaxPatches)
			{
				UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Not enough patch memory");
			}

			newPatch = inBuildData->patches + inBuildData->numPatches++;

		if((inBuildData->numPatches % 200) == 0) Imp_PrintMessage(IMPcMsg_Important,".");

		// get a new ind index
			if(inBuildData->numPatchIndIndices >= IMPcEnv_MaxGQIndexArray)
			{
				UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Not enough patch indIndex memory");
			}
			newIndIndexIndex = inBuildData->numPatchIndIndices++;

		// start the new patch
			nextIndIndex = newPatch->gqStartIndirectIndex = newIndIndexIndex;
			newPatch->gqEndIndirectIndex = newIndIndexIndex + 1;

			inBuildData->patchIndIndices[newIndIndexIndex] = curGQIndex;

		// loop through all the gqs in the patch so far and look for adjacencies
		// stop when no new gqs are added
		while(nextIndIndex < newPatch->gqEndIndirectIndex)
		{
			targetGQIndex = inBuildData->patchIndIndices[nextIndIndex];

			targetGQ = inBuildData->gqList + targetGQIndex;

			numEdges = (targetGQ->flags & AKcGQ_Flag_Triangle) ? 3 : 4;

			for(curEdgeItr = 0; curEdgeItr < numEdges; curEdgeItr++)
			{
				targetEdge = edgeList + targetGQ->edgeIndices.indices[curEdgeItr];

				for(curQuadItr = 0; curQuadItr < AUcMaxQuadsPerEdge; curQuadItr++)
				{
					if(targetEdge->quadIndices[curQuadItr] == targetGQIndex) continue;
					if(targetEdge->quadIndices[curQuadItr] == UUcMaxUns32) break;

					nextGQIndex = targetEdge->quadIndices[curQuadItr];

/*					if(curQuadItr < 2)
					{
						targetGQ->adjGQIndices[curEdgeItr] = nextGQIndex;
					}*/

					if(!UUrBitVector_TestAndSetBit(gqTakenBV, nextGQIndex))
					{
						if(inBuildData->numPatchIndIndices >= IMPcEnv_MaxGQIndexArray)
						{
							UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Not enough patch indIndex memory");
						}
						newIndIndexIndex = inBuildData->numPatchIndIndices++;

						newPatch->gqEndIndirectIndex = newIndIndexIndex + 1;

						inBuildData->patchIndIndices[newIndIndexIndex] = nextGQIndex;
					}
				}
			}

			nextIndIndex++;
		}
	}

	Imp_PrintMessage(IMPcMsg_Cosmetic, UUmNL"\tanalyzing patch list"UUmNL);

	for(curPatchIndex = 0, curPatch = inBuildData->patches;
		curPatchIndex < inBuildData->numPatches;
		curPatchIndex++, curPatch++)
	{
		float				totalArea;

		totalArea = 0;

		for(curIndIndex = curPatch->gqStartIndirectIndex;
			curIndIndex < curPatch->gqEndIndirectIndex;
			curIndIndex++)
		{
			targetGQ = inBuildData->gqList + inBuildData->patchIndIndices[curIndIndex];

			totalArea +=
				MUrTriangle_Area(
					sharedPoints + targetGQ->visibleQuad.indices[0],
					sharedPoints + targetGQ->visibleQuad.indices[1],
					sharedPoints + targetGQ->visibleQuad.indices[2]) +
				MUrTriangle_Area(
					sharedPoints + targetGQ->visibleQuad.indices[0],
					sharedPoints + targetGQ->visibleQuad.indices[2],
					sharedPoints + targetGQ->visibleQuad.indices[3]);
		}

		totalGQArea += totalArea;
	}

	inBuildData->totalGQArea = totalGQArea;

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	UUrBitVector_Dispose(gqTakenBV);

	return UUcError_None;
}

static UUtError IMPrEnv_Process_GQDoors( IMPtEnv_BuildData*	inBuildData )
{
	UUtUns32			curGQIndex;
	IMPtEnv_GQ*			curGQ;
	UUtInt64			time = UUrMachineTime_High();
//	UUtUns32			new_index;
//	UUtError			error;

	Imp_PrintMessage(IMPcMsg_Important, "Processing door GQs...");

	inBuildData->door_count	= 0;

	for(curGQIndex = 0, curGQ = inBuildData->gqList; curGQIndex < inBuildData->numGQs; curGQIndex++, curGQ++ )
	{
		if((curGQIndex % 5000) == 0)			Imp_PrintMessage(IMPcMsg_Important, ".");

		if(curGQ->flags & AKcGQ_Flag_Door)
		{
			inBuildData->door_gq_indexes[inBuildData->door_count++] = curGQIndex;

			// setup the quads with default door fram properties

			UUmAssert( inBuildData->door_frame_texture != 0xFFFF );

			curGQ->flags |= AKcGQ_Flag_No_Collision | AKcGQ_Flag_DrawBothSides | AKcGQ_Flag_Transparent;

			curGQ->textureMapIndex = inBuildData->door_frame_texture;

			UUmAssert( inBuildData->door_count <= IMPcEnv_MaxNumDoors );
		}
	}

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, "   %d" UUmNL "total time = %f" UUmNL UUmNL, inBuildData->door_count, UUrMachineTime_High_To_Seconds(time));

	return UUcError_None;
}


UUtBool
Imp_Env_PointInNode(
	M3tPoint3D*			inPoint,
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_BNV*		inNode)
{
	return IMPiEnv_Process_BSP_PointInBSP(inBuildData,inNode,inPoint, 0);
}

static UUtError
IMPiEnv_Process_Flags(
	IMPtEnv_BuildData *inBuildData)
{
	UUtUns16 i;

	// bla bla bla, here
	for (i=0; i<inBuildData->numFlags; i++)
	{
	}

	return UUcError_None;
}

static IMPtEnv_Object *IMPiFindDoorOnQuad(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_GQ *inGQ)
{
	// Returns whether there is a door on this quad. Assumes inGQ is a ghost or SAT.
	UUtUns32 i;
	IMPtEnv_Object *object;
	M3tPlaneEquation plane,*planeList;

	planeList = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);

	for (i=0; i<inBuildData->numObjects; i++)
	{
		object = inBuildData->objectList + i;
		AKmPlaneEqu_GetComponents(inGQ->planeEquIndex,planeList,plane.a,plane.b,plane.c,plane.d);

		if (AUrPlane_Distance_Squared(&plane,&object->position,NULL) < UUmSQR(IMPcDoorPlaneThresh)) {
			return object;
		}
	}

	return NULL;
}

static UUtError
IMPiCreateQuadRemapFile(
	IMPtEnv_BuildData*	inBuildData)
{
	char			filename[64];
	BFtFile			*outFile;
	BFtFileRef		*fileRef;
	UUtError		error;
	UUtInt64		time = UUrMachineTime_High();


	// Create the quad remap file
	sprintf(filename,"%s%d.tmp",IMPcRemapFilePrefix,IMPgCurLevel);

	error = BFrFileRef_MakeFromName(filename,&fileRef);
	UUmError_ReturnOnErrorMsg(error,"Unable to create quad remap fileref");

	BFrFile_Delete(fileRef);

	error = BFrFile_Create(fileRef);
	UUmError_ReturnOnErrorMsg(error,"Unable to create quad remap file");

	error = BFrFile_Open(fileRef,"rw",&outFile);
	UUmError_ReturnOnErrorMsg(error,"Unable to open quad remap file for write");

	error = BFrFile_Write(outFile,sizeof(UUtUns32),&IMPgAI_QuadRemapCount);
	UUmError_ReturnOnErrorMsg(error,"Unable to write quad remap count");

	error = BFrFile_Write(outFile,sizeof(UUtUns32) * IMPgAI_QuadRemapCount * 2,IMPgAI_QuadRefRemap);
	UUmError_ReturnOnErrorMsg(error,"Unable to write quad remaps");

	BFrFile_Close(outFile);

	BFrFileRef_Dispose(fileRef);

	return UUcError_None;
}

static UUtError
IMPrEnv_Process_GQMinMax(
	IMPtEnv_BuildData*	inBuildData)
{
	UUtUns32		curGQIndex;
	IMPtEnv_GQ*		curGQ;

	IMPtEnv_Object	*object;
	UUtInt64		time = UUrMachineTime_High();

	M3tPoint3D*			sharedPoints;
	M3tPlaneEquation*	planeArray = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);


	// Calculate bounds and various properties for GQs
	Imp_PrintMessage(IMPcMsg_Important, "computing gq min max / properties");

	sharedPoints = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);

	for(curGQIndex = 0, curGQ = inBuildData->gqList;
		curGQIndex < inBuildData->numGQs;
		curGQIndex++, curGQ++)
	{
		float			minX, minY, minZ;
		float			maxX, maxY, maxZ;

		const M3tPoint3D*	curPoint;
		UUtUns8				i;

		{
			const float floor_angle = 0.5f;
			M3tPlaneEquation*	planeEqu = planeArray + (curGQ->planeEquIndex & 0x7FFFFFFF);

			if ((float)fabs(planeEqu->b) > floor_angle) {
				curGQ->flags |= AKcGQ_Flag_FloorCeiling;
			}
			else {
				curGQ->flags |= AKcGQ_Flag_Wall;
			}
		}


		if ((curGQIndex % 5000) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		// Check for doors on this quad
		if (curGQ->flags & AKcGQ_Flag_PathfindingMask) {
			object = IMPiFindDoorOnQuad(inBuildData,curGQ);

			if (object) {
				curGQ->flags |= AKcGQ_Flag_Door;
				object->door_gq = curGQIndex;
			}
		}

		// Remap indices for scripting
		if (curGQ->scriptID != 0xFFFF)
		{
			IMPrScript_AddQuadRemap(
				curGQIndex,
				curGQ->scriptID,
				inBuildData->environmentGroup);
		}

		// Quad extrema
		AUrQuad_HighestPoints(&curGQ->visibleQuad,sharedPoints,&curGQ->hi1,&curGQ->hi2);
		AUrQuad_LowestPoints(&curGQ->visibleQuad,sharedPoints,&curGQ->lo1,&curGQ->lo2);

		curPoint = sharedPoints + curGQ->visibleQuad.indices[0];

		minX = maxX = curPoint->x;
		minY = maxY = curPoint->y;
		minZ = maxZ = curPoint->z;

		for(i = 1; i < 4; i++)
		{
			float curX, curY, curZ;

			curPoint = sharedPoints + curGQ->visibleQuad.indices[i];

			curX = curPoint->x;
			curY = curPoint->y;
			curZ = curPoint->z;

			UUmMinMax(curX, minX, maxX);
			UUmMinMax(curY, minY, maxY);
			UUmMinMax(curZ, minZ, maxZ);
		}

		curGQ->bBox.minPoint.x = minX;
		curGQ->bBox.minPoint.y = minY;
		curGQ->bBox.minPoint.z = minZ;
		curGQ->bBox.maxPoint.x = maxX;
		curGQ->bBox.maxPoint.y = maxY;
		curGQ->bBox.maxPoint.z = maxZ;
	}

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));


	return UUcError_None;
}

static UUtError
IMPiEnv_Process_CalculateOrigins(
	IMPtEnv_BuildData*	inBuildData)
{
	UUtUns16	curBNVIndex;
	UUtInt64	time = UUrMachineTime_High();

	Imp_PrintMessage(IMPcMsg_Important, "calculating BNV origins");

	for(curBNVIndex = 0; curBNVIndex < inBuildData->numBNVs; curBNVIndex++)
	{
		IMPtEnv_BNV *curNode = &inBuildData->bnvList[curBNVIndex];

		if((curBNVIndex % 10) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		if (!(curNode->flags & AKcBNV_Flag_Room)) continue;	// Only room nodes!
		IMPiCalculateNodeOrigins(inBuildData,curNode,&curNode->origin,&curNode->antiOrigin);
	}

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	return UUcError_None;
}

UUtError
IMPrEnv_Process(
	IMPtEnv_BuildData*	inBuildData,
	BFtFileRef*			inSourceFileRef)
{
	UUtError	error;
	UUtUns32	gqItr;
	IMPtEnv_GQ*	curGQ;

	// 1. Initialize stuff

	error = IMPrEnv_Process_BNV_BuildBSP(inBuildData);
	UUmError_ReturnOnError(error);

	error = IMPrEnv_Process_OctTrees2(inBuildData);
	UUmError_ReturnOnError(error);

	// identify and setup the door frame quads
	IMPrEnv_Process_GQDoors(inBuildData);

	// loop through the gqs and set the proper flags based on the texture map
	for(gqItr = 0, curGQ = inBuildData->gqList; gqItr < inBuildData->numGQs; gqItr++, curGQ++)
	{
		if(curGQ->textureMapIndex != 0xFFFF)
		{
			if(IMrPixelType_HasAlpha(inBuildData->envTextureList[curGQ->textureMapIndex].flags.pixelType))
			{
				curGQ->flags |= (AKcGQ_Flag_DrawBothSides | AKcGQ_Flag_Transparent);
			}
		}
		if (curGQ->flags & AKcGQ_Flag_Transparent)
		{
			if (inBuildData->numAlphaQuads < IMPcEnv_MaxAlphaQuads)
			{
				inBuildData->alphaQuads[inBuildData->numAlphaQuads++] = gqItr;
			}
			else
			{
				Imp_PrintMessage(IMPcMsg_Important, UUmNL"TOO MANY ALPHA QUADS"UUmNL);
			}
		}
	}

	if(IMPgConstructing)
	{
		error = IMPrEnv_Process_GQMinMax(inBuildData);
		UUmError_ReturnOnError(error);

		#if 1
		// 11. build the alpha quad bsp
			IMPiEnv_Process_Alpha_BSP_Build(
				inBuildData,
				inBuildData->numAlphaQuads,
				inBuildData->alphaQuads);
		#endif

		// create the fixed oct tree
		{
			UUtInt64 fixed_oct_tree_time = UUrMachineTime_High();

			IMPrFixedOctTree_Create(inBuildData);

			fixed_oct_tree_time = UUrMachineTime_High() - fixed_oct_tree_time;
			Imp_PrintMessage(IMPcMsg_Important, UUmNL "time to build fixed node oct tree = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(fixed_oct_tree_time));
		}


		// 2. Compute the BNV volume node tree
		error = IMPrEnv_Process_BNV_ComputeProperties(inBuildData);
		UUmError_ReturnOnError(error);

		IMPiEnv_Process_BNV_FindParents(inBuildData);
		IMPiEnv_Process_BNV_AnalyzeParents(inBuildData);

		// 3. Process the stair GQs and BNVs
		error = IMPiEnv_Process_Stairs(inBuildData);
		UUmError_ReturnOnError(error);

		// 4. Calculate origins
		error = IMPiEnv_Process_CalculateOrigins(inBuildData);
		UUmError_ReturnOnError(error);

		// 5. Find adjacencies
		IMPrEnv_Process_BNV_ComputeAdjacency(inBuildData);

	}

	// 6. Look for GQs that should be ignored
	IMPrEnv_Process_GQVis(inBuildData);


	if(IMPgConstructing)
	{
		IMPiCreateQuadRemapFile(inBuildData);
	}

	// 9. lightmaps
	error = IMPrEnv_Process_LightMap(inBuildData, inSourceFileRef);
	UUmError_ReturnOnError(error);

	if(IMPgConstructing)
	{
		// 10. Process the marker flags and objects
		Imp_PrintMessage(IMPcMsg_Important,"Processing flags..."UUmNL);

		error = IMPiEnv_Process_Flags(inBuildData);
		UUmError_ReturnOnError(error);

		Imp_PrintMessage(IMPcMsg_Important, "."UUmNL);
		// 12. Clean up
	}

	return UUcError_None;

}
