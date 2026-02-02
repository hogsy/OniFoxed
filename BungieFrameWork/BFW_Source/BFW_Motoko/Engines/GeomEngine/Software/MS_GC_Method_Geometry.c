/*
	FILE:	MS_GC_Method_Geometry.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 19, 1997

	PURPOSE:

	Copyright 1997

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"
#include "BFW_BitVector.h"
#include "BFW_Console.h"

#include "MS_GC_Private.h"
#include "MS_GC_Method_Geometry.h"
#include "MS_GC_Method_State.h"
#include "MS_GC_Env_Clip.h"
#include "MS_Geom_Clip.h"
#include "MS_Geom_Transform.h"

// CB: needed so that we can test point visibility against characters
#include "Oni_Character.h"
#include "Oni_Aiming.h"

#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING

	extern UUtBool		gClipForce4D;
	extern UUtBool		gNo4DClip;

#endif

#include "Motoko_Manager.h"

UUtError
MSrGeomContext_Method_Geometry_BoundingBox_Draw(
	UUtUns32			inShade,
	M3tGeometry*		inGeometryObject)
{

	MStClipStatus			clipStatus;
	M3tPointScreen			*screenPoints;
	UUtUns8					*clipCodes;
	M3tPoint4D				*frustumPoints;
	M3tQuad					quadIndices;
	UUtUns32				itr, boxShades[4 + M3cExtraCoords];

	MSrTransform_UpdateMatrices();

	clipCodes = MSgGeomContextPrivate->objectVertexData.clipCodes;
	frustumPoints = MSgGeomContextPrivate->objectVertexData.frustumPoints;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;

	clipStatus =
		MSgGeomContextPrivate->activeFunctions->transformBoundingBoxToFrustumScreen(
			inGeometryObject,
			frustumPoints,
			screenPoints,
			clipCodes);

	M3rDraw_State_Push();

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		screenPoints);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Appearence,
		M3cDrawState_Appearence_Gouraud);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_Vertex);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenShadeArray_DC,
		boxShades);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		8);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		inShade);

	M3rDraw_State_Commit();

	for (itr = 0; itr < 8; itr++) {
		boxShades[itr] = inShade;
	}

	MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = 8;
	MSgGeomContextPrivate->objectVertexData.maxClipTextureCords = 8 + M3cExtraCoords;
	MSgGeomContextPrivate->objectVertexData.maxClipVertices = 8 + M3cExtraCoords;

	switch(clipStatus)
	{
		case MScClipStatus_TrivialReject:
			break;

		case MScClipStatus_TrivialAccept:
			// minZ side
				quadIndices.indices[0] = 0;
				quadIndices.indices[1] = 1;
				quadIndices.indices[2] = 3;
				quadIndices.indices[3] = 2;
				M3rDraw_Quad(&quadIndices);

			// maxZ side
				quadIndices.indices[0] = 4;
				quadIndices.indices[1] = 5;
				quadIndices.indices[2] = 7;
				quadIndices.indices[3] = 6;
				M3rDraw_Quad(&quadIndices);

			// minY side
				quadIndices.indices[0] = 0;
				quadIndices.indices[1] = 1;
				quadIndices.indices[2] = 5;
				quadIndices.indices[3] = 4;
				M3rDraw_Quad(&quadIndices);

			// maxY side
				quadIndices.indices[0] = 2;
				quadIndices.indices[1] = 3;
				quadIndices.indices[2] = 7;
				quadIndices.indices[3] = 6;
				M3rDraw_Quad(&quadIndices);

			// minX side
				quadIndices.indices[0] = 0;
				quadIndices.indices[1] = 2;
				quadIndices.indices[2] = 6;
				quadIndices.indices[3] = 4;
				M3rDraw_Quad(&quadIndices);

			// maxX side
				quadIndices.indices[0] = 1;
				quadIndices.indices[1] = 3;
				quadIndices.indices[2] = 7;
				quadIndices.indices[3] = 5;
				M3rDraw_Quad(&quadIndices);
			break;

		case MScClipStatus_NeedsClipping:
			MSgGeomContextPrivate->polyComputeVertexProc = MSrClip_ComputeVertex_GouraudFlat;

			// minZ
				MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = 8;
				if(clipCodes[0] | clipCodes[1] | clipCodes[3] | clipCodes[2])
				{
					MSrClip_Quad(0, 1, 3, 2, clipCodes[0], clipCodes[1], clipCodes[3], clipCodes[2], 0);
				}
				else
				{
					quadIndices.indices[0] = 0;
					quadIndices.indices[1] = 1;
					quadIndices.indices[2] = 3;
					quadIndices.indices[3] = 2;
					M3rDraw_Quad(&quadIndices);
				}

			// maxZ
				MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = 8;
				if(clipCodes[4] | clipCodes[5] | clipCodes[7] | clipCodes[6])
				{
					MSrClip_Quad(4, 5, 7, 6, clipCodes[4], clipCodes[5], clipCodes[7], clipCodes[6], 0);
				}
				else
				{
					quadIndices.indices[0] = 4;
					quadIndices.indices[1] = 5;
					quadIndices.indices[2] = 7;
					quadIndices.indices[3] = 6;
					M3rDraw_Quad(&quadIndices);
				}

			// minY
				MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = 8;
				if(clipCodes[0] | clipCodes[1] | clipCodes[5] | clipCodes[4])
				{
					MSrClip_Quad(0, 1, 5, 4, clipCodes[0], clipCodes[1], clipCodes[5], clipCodes[4], 0);
				}
				else
				{
					quadIndices.indices[0] = 0;
					quadIndices.indices[1] = 1;
					quadIndices.indices[2] = 5;
					quadIndices.indices[3] = 4;
					M3rDraw_Quad(&quadIndices);
				}

			// maxY
				MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = 8;
				if(clipCodes[2] | clipCodes[3] | clipCodes[7] | clipCodes[6])
				{
					MSrClip_Quad(2, 3, 7, 6, clipCodes[2], clipCodes[3], clipCodes[7], clipCodes[6], 0);
				}
				else
				{
					quadIndices.indices[0] = 2;
					quadIndices.indices[1] = 3;
					quadIndices.indices[2] = 7;
					quadIndices.indices[3] = 6;
					M3rDraw_Quad(&quadIndices);
				}

			// minX
				MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = 8;
				if(clipCodes[0] | clipCodes[2] | clipCodes[6] | clipCodes[4])
				{
					MSrClip_Quad(0, 2, 6, 4, clipCodes[0], clipCodes[2], clipCodes[6], clipCodes[4], 0);
				}
				else
				{
					quadIndices.indices[0] = 0;
					quadIndices.indices[1] = 2;
					quadIndices.indices[2] = 6;
					quadIndices.indices[3] = 4;
					M3rDraw_Quad(&quadIndices);
				}

			// maxX
				MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = 8;
				if(clipCodes[1] | clipCodes[3] | clipCodes[7] | clipCodes[5])
				{
					MSrClip_Quad(1, 3, 7, 5, clipCodes[1], clipCodes[3], clipCodes[7], clipCodes[5], 0);
				}
				else
				{
					quadIndices.indices[0] = 1;
					quadIndices.indices[1] = 3;
					quadIndices.indices[2] = 7;
					quadIndices.indices[3] = 5;
					M3rDraw_Quad(&quadIndices);
				}
			break;
	}

	M3rDraw_State_Pop();

	return UUcError_None;
}

static UUtError
MSiGeometry_Draw_BackFaceKeep_Clip_ShadeVertex(
	M3tGeometry*		inGeometryObject)
{
	M3tPoint3D*						worldPoints;
	M3tVector3D*					worldVertexNormals;
	UUtUns32*						screenVertexShades;
	M3tPointScreen*					screenPoints;
	UUtUns32						index0, index1, index2;
	UUtUns32						i;

	M3tPoint4D*						frustumPoints;
	UUtUns8*						clipCodes;
	UUtUns8							clipCode0, clipCode1, clipCode2;
	UUtUns32						numPoints;
	UUtUns32		numTris;
	UUtUns32*		curIndexPtr;

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_Vertex);

	numPoints = inGeometryObject->pointArray->numPoints;

	worldPoints = MSgGeomContextPrivate->worldPoints;
	worldVertexNormals = MSgGeomContextPrivate->worldVertexNormals;
	screenVertexShades = (UUtUns32*)MSgGeomContextPrivate->shades_vertex;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;
	clipCodes = MSgGeomContextPrivate->objectVertexData.clipCodes;
	frustumPoints = MSgGeomContextPrivate->objectVertexData.frustumPoints;

	/*
	 * Transform all points to screen space
	 */
		MSgGeomContextPrivate->activeFunctions->transformPointListToFrustumScreen(
			inGeometryObject,
			frustumPoints,
			screenPoints,
			clipCodes);

	/*
	 * Transform all points and vertex normals to world space
	 */
		MSgGeomContextPrivate->activeFunctions->transformPointListAndVertexNormalToWorld(
			inGeometryObject,
			worldPoints,
			worldVertexNormals);

	/*
	 * Shade the vertices
	 */
		if(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Appearance] == M3cGeomState_Appearance_Texture &&
			MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Fill] == M3cGeomState_Fill_Solid &&
			(inGeometryObject->geometryFlags & M3cGeometryFlag_ComputeSpecular))
		{

		}
		else
		{
			MSgGeomContextPrivate->activeFunctions->shadeVerticesGouraud(
				inGeometryObject,
				worldPoints,
				worldVertexNormals,
				screenVertexShades);
		}

	//
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		NULL);

	M3rDraw_State_Commit();

	numTris = inGeometryObject->triNormalIndexArray->numIndices;
	curIndexPtr = inGeometryObject->triStripArray->indices;

	for(i = 0; i < numTris; i++)
	{
		index2 = *curIndexPtr++;

		if(index2 & 0x80000000)
		{
			index0 = (index2 & 0x7FFFFFFF);
			index1 = *curIndexPtr++;
			index2 = *curIndexPtr++;
			clipCode0 = clipCodes[index0];
			clipCode1 = clipCodes[index1];
		}

		clipCode2 = clipCodes[index2];

		if(clipCode0 & clipCode1 & clipCode2)
		{
			// Do nothing
		}
		else if(clipCode0 | clipCode1 | clipCode2)
		{
			MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = numPoints;
			MSrClip_Triangle(
				index0,
				index1,
				index2,
				clipCode0,
				clipCode1,
				clipCode2,
				0);
		}
		else
		{
			M3tTri	curTri;

			curTri.indices[0] = index0;
			curTri.indices[1] = index1;
			curTri.indices[2] = index2;

			M3rDraw_Triangle(
				&curTri);
		}

		index0 = index1;
		index1 = index2;
		clipCode0 = clipCode1;
		clipCode1 = clipCode2;
	}

	return UUcError_None;
}

static UUtError
MSiGeometry_Draw_BackFaceKeep_Clip_ShadeFace(
	M3tGeometry*		inGeometryObject)
{
	M3tPoint3D*				worldPoints;
	M3tVector3D*			worldTriNormals;
	UUtUns32*				screenTriShades;
	M3tPointScreen*			screenPoints;
	M3tPoint4D*				frustumPoints;
	UUtUns32				index0, index1, index2;
	UUtUns32				i;
	UUtUns8*				clipCodes;
	UUtUns8					clipCode0, clipCode1, clipCode2;
	UUtUns32				numPoints;
	UUtUns32				numTris;
	UUtUns32*				curIndexPtr;

	M3rDraw_State_Push();

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_None);

	numPoints = inGeometryObject->pointArray->numPoints;
	numTris = inGeometryObject->triNormalIndexArray->numIndices;

	worldPoints = MSgGeomContextPrivate->worldPoints;
	worldTriNormals = MSgGeomContextPrivate->worldTriNormals;
	screenTriShades = (UUtUns32*)MSgGeomContextPrivate->shades_tris;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;
	clipCodes = MSgGeomContextPrivate->objectVertexData.clipCodes;
	frustumPoints = MSgGeomContextPrivate->objectVertexData.frustumPoints;

	MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = numPoints;

	MSgGeomContextPrivate->activeFunctions->transformPointListToFrustumScreen(
		inGeometryObject,
		frustumPoints,
		screenPoints,
		clipCodes);

	/*
	 * Transform all faces to world space
	 */
		MSgGeomContextPrivate->activeFunctions->transformFaceNormalToWorld(
			inGeometryObject,
			worldTriNormals);

	/*
	 * Shade the vertices
	 */
		if(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Appearance] == M3cGeomState_Appearance_Texture &&
			MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Fill] == M3cGeomState_Fill_Solid &&
			(inGeometryObject->geometryFlags & M3cGeometryFlag_ComputeSpecular))
		{

		}
		else
		{
			MSgGeomContextPrivate->activeFunctions->shadeFacesGouraud(
				inGeometryObject,
				worldPoints,
				numTris,
				worldTriNormals,
				screenTriShades);
		}

	//
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		NULL);

	curIndexPtr = inGeometryObject->triStripArray->indices;

	for(i = 0; i < numTris; i++)
	{
		index2 = *curIndexPtr++;

		if(index2 & 0x80000000)
		{
			index0 = (index2 & 0x7FFFFFFF);
			index1 = *curIndexPtr++;
			index2 = *curIndexPtr++;
			clipCode0 = clipCodes[index0];
			clipCode1 = clipCodes[index1];
		}

		clipCode2 = clipCodes[index2];

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_ConstantColor,
			*screenTriShades);

		M3rDraw_State_Commit();

		if(clipCode0 & clipCode1 & clipCode2)
		{
			// Do nothing
		}
		else if(clipCode0 | clipCode1 | clipCode2)
		{
			MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = numPoints;
			MSrClip_Triangle(
				index0,
				index1,
				index2,
				clipCode0,
				clipCode1,
				clipCode2,
				0);
		}
		else
		{
			M3tTri	curTri;

			curTri.indices[0] = index0;
			curTri.indices[1] = index1;
			curTri.indices[2] = index2;

			M3rDraw_Triangle(
				&curTri);
		}

		index0 = index1;
		index1 = index2;
		clipCode0 = clipCode1;
		clipCode1 = clipCode2;
		screenTriShades++;
	}

	M3rDraw_State_Pop();

	return UUcError_None;
}

static UUtError
MSiGeometry_Draw_BackFaceKeep_ClipAccept_ShadeVertex(
	M3tGeometry*		inGeometryObject)
{
	M3tPoint3D*				worldPoints;
	M3tVector3D*			worldVertexNormals;
	UUtUns32*				screenVertexShades;
	M3tPointScreen*			screenPoints;
	UUtUns32				i;
	UUtUns32				numPoints;
	UUtUns32*				curIndexPtr;
	UUtUns32				index0, index1, index2;
	UUtUns32				numTris;

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_Vertex);

	numPoints = inGeometryObject->pointArray->numPoints;

	worldPoints = MSgGeomContextPrivate->worldPoints;
	worldVertexNormals = MSgGeomContextPrivate->worldVertexNormals;
	screenVertexShades = (UUtUns32*)MSgGeomContextPrivate->shades_vertex;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;

	/*
	 * Transform all points to screen space
	 */
		MSgGeomContextPrivate->activeFunctions->transformPointListToScreen(
			inGeometryObject,
			screenPoints);

		if(0)
		{
			UUtUns32	itr;

			MSrTransform_Geom_PointListToScreen(
				inGeometryObject,
				MSgGeomContextPrivate->gqVertexData.screenPoints);

			for(itr = 0; itr < numPoints; itr++)
			{
				UUmAssert(UUmFloat_CompareEqu(screenPoints[itr].x, MSgGeomContextPrivate->gqVertexData.screenPoints[itr].x));
				UUmAssert(UUmFloat_CompareEqu(screenPoints[itr].y, MSgGeomContextPrivate->gqVertexData.screenPoints[itr].y));
				UUmAssert(UUmFloat_CompareEqu(screenPoints[itr].z, MSgGeomContextPrivate->gqVertexData.screenPoints[itr].z));
				UUmAssert(UUmFloat_CompareEqu(screenPoints[itr].invW, MSgGeomContextPrivate->gqVertexData.screenPoints[itr].invW));
			}
		}


	/*
	 * Transform all points and vertex normals to world space
	 */
		MSgGeomContextPrivate->activeFunctions->transformPointListAndVertexNormalToWorld(
			inGeometryObject,
			worldPoints,
			worldVertexNormals);

	/*
	 * Shade the vertices
	 */
		if(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Appearance] == M3cGeomState_Appearance_Texture &&
			MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Fill] == M3cGeomState_Fill_Solid &&
			(inGeometryObject->geometryFlags & M3cGeometryFlag_ComputeSpecular))
		{

		}
		else
		{
			MSgGeomContextPrivate->activeFunctions->shadeVerticesGouraud(
				inGeometryObject,
				worldPoints,
				worldVertexNormals,
				screenVertexShades);
		}

	//
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		NULL);

	M3rDraw_State_Commit();

	numTris = inGeometryObject->triNormalIndexArray->numIndices;
	curIndexPtr = inGeometryObject->triStripArray->indices;

	for(i = 0; i < numTris; i++)
	{
		index2 = *curIndexPtr++;

		if(index2 & 0x80000000)
		{
			index0 = (index2 & 0x7FFFFFFF);
			index1 = *curIndexPtr++;
			index2 = *curIndexPtr++;
		}

		{
			M3tTri	curTri;

			curTri.indices[0] = index0;
			curTri.indices[1] = index1;
			curTri.indices[2] = index2;

			M3rDraw_Triangle(
				&curTri);
		}

		index0 = index1;
		index1 = index2;
	}

	return UUcError_None;
}

static UUtError
MSiGeometry_Draw_BackFaceKeep_ClipAccept_ShadeFace(
	M3tGeometry*		inGeometryObject)
{
	M3tPoint3D*				worldPoints;
	M3tVector3D*			worldTriNormals;
	UUtUns32*				screenTriShades;
	M3tPointScreen*			screenPoints;
	UUtUns32				numTris;
	UUtUns32				i;
	UUtUns32				numPoints;
	UUtUns32*				curIndexPtr;
	UUtUns32				index0, index1, index2;

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_None);

	numPoints = inGeometryObject->pointArray->numPoints;
	numTris = inGeometryObject->triNormalIndexArray->numIndices;

	worldPoints = MSgGeomContextPrivate->worldPoints;
	worldTriNormals = MSgGeomContextPrivate->worldTriNormals;
	screenTriShades = (UUtUns32*)MSgGeomContextPrivate->shades_tris;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;

	/*
	 * Transform all points to screen space
	 */
		MSgGeomContextPrivate->activeFunctions->transformPointListToScreen(
			inGeometryObject,
			screenPoints);

	/*
	 * Transform all face normals to world space
	 */
		MSgGeomContextPrivate->activeFunctions->transformFaceNormalToWorld(
			inGeometryObject,
			worldTriNormals);

	/*
	 * Shade the vertices
	 */
	if(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Appearance] == M3cGeomState_Appearance_Texture &&
		MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Fill] == M3cGeomState_Fill_Solid &&
		(inGeometryObject->geometryFlags & M3cGeometryFlag_ComputeSpecular))
	{

	}
	else
	{
		MSgGeomContextPrivate->activeFunctions->shadeFacesGouraud(
			inGeometryObject,
			worldPoints,
			numTris,
			worldTriNormals,
			screenTriShades);
	}

	//
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		NULL);

	numTris = inGeometryObject->triNormalIndexArray->numIndices;
	curIndexPtr = inGeometryObject->triStripArray->indices;

	for(i = 0; i < numTris; i++)
	{
		index2 = *curIndexPtr++;

		if(index2 & 0x80000000)
		{
			index0 = (index2 & 0x7FFFFFFF);
			index1 = *curIndexPtr++;
			index2 = *curIndexPtr++;
		}

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_ConstantColor,
			*screenTriShades);

		M3rDraw_State_Commit();

		{
			M3tTri	curTri;

			curTri.indices[0] = index0;
			curTri.indices[1] = index1;
			curTri.indices[2] = index2;

			M3rDraw_Triangle(
				&curTri);
		}

		index0 = index1;
		index1 = index2;
		screenTriShades++;
	}

	return UUcError_None;
}

/* */
static UUtError
MSiGeometry_Draw_BackFaceRemove_Clip_ShadeVertex(
	M3tGeometry*		inGeometryObject)
{
	M3tVector3D*			worldViewVectors;
	M3tPoint3D*				worldPoints;
	M3tVector3D*			worldVertexNormals;
	UUtUns32*				screenVertexShades;
	M3tPointScreen*			screenPoints;
	UUtUns32				index0, index1, index2;
	UUtUns32				i;
	M3tVector3D*			worldTriNormals;

	UUtUns32				numTris = 0;

	M3tPoint4D*				frustumPoints;
	UUtUns8*				clipCodes;
	UUtUns8					clipCode0, clipCode1, clipCode2;
	UUtUns32				numPoints;

	UUtUns32*				activeTrisBV;
	UUtUns32*				activeVerticesBV;

	UUtUns32*				curIndexPtr;

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_Vertex);

	numPoints = inGeometryObject->pointArray->numPoints;
	worldPoints = MSgGeomContextPrivate->worldPoints;
	worldViewVectors = MSgGeomContextPrivate->worldViewVectors;
	worldTriNormals = MSgGeomContextPrivate->worldTriNormals;
	clipCodes = MSgGeomContextPrivate->objectVertexData.clipCodes;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;
	frustumPoints = MSgGeomContextPrivate->objectVertexData.frustumPoints;
	screenVertexShades = MSgGeomContextPrivate->shades_vertex;
	worldVertexNormals = MSgGeomContextPrivate->worldVertexNormals;


	// 0. init
	//		- active face list
	//		- active vertex list
	// 1. Transform all face normals to world
		MSgGeomContextPrivate->activeFunctions->transformFaceNormalToWorld(
			inGeometryObject,
			worldTriNormals);

		numTris = inGeometryObject->triNormalIndexArray->numIndices;
		activeTrisBV = MSgGeomContextPrivate->activeTrisBV;
		UUrBitVector_ClearBitAll(
			activeTrisBV,
			numTris);

		activeVerticesBV = MSgGeomContextPrivate->activeVerticesBV;
		UUrBitVector_ClearBitAll(
			activeVerticesBV,
			numPoints);

	// 2. Transform all points to world for backface test
		MSgGeomContextPrivate->activeFunctions->transformPointListAndVertexNormalToWorldComputeViewVector(
			inGeometryObject,
			worldPoints,
			worldViewVectors,
			worldVertexNormals);

	// 3. Do backface remove test
	//		- mark all front facing faces as active
	//		- mark all vertices belonging to front facing faces as active
		MSgGeomContextPrivate->activeFunctions->backfaceRemove(
			inGeometryObject,
			worldPoints,
			worldViewVectors,
			worldTriNormals,
			activeTrisBV,
			activeVerticesBV);

	// 3. Transform all active vertices to frustum and screen space
		MSgGeomContextPrivate->activeFunctions->transformPointListToFrustumScreenActive(
			inGeometryObject,
			activeVerticesBV,
			frustumPoints,
			screenPoints,
			clipCodes);

		if(0)
		{
				UUtUns32	itr;

				MSrTransform_Geom_PointListToFrustumScreen(
				inGeometryObject,
//				activeVerticesBV,
				MSgGeomContextPrivate->gqVertexData.frustumPoints,
				MSgGeomContextPrivate->gqVertexData.screenPoints,
				MSgGeomContextPrivate->gqVertexData.clipCodes);

				for(itr = 0; itr < numPoints; itr++)
				{
					//if(!UUrBitVector_TestBit(activeVerticesBV, itr)) continue;

					UUmAssert(clipCodes[itr] == MSgGeomContextPrivate->gqVertexData.clipCodes[itr]);

					UUmAssert(UUmFloat_CompareEqu(screenPoints[itr].x, MSgGeomContextPrivate->gqVertexData.screenPoints[itr].x));
					UUmAssert(UUmFloat_CompareEqu(screenPoints[itr].y, MSgGeomContextPrivate->gqVertexData.screenPoints[itr].y));
					UUmAssert(UUmFloat_CompareEqu(screenPoints[itr].z, MSgGeomContextPrivate->gqVertexData.screenPoints[itr].z));
					UUmAssert(UUmFloat_CompareEqu(screenPoints[itr].invW, MSgGeomContextPrivate->gqVertexData.screenPoints[itr].invW));
				}
		}

	// 4. Shade all active vertices
		MSgGeomContextPrivate->activeFunctions->shadeVerticesGouraudActive(
			inGeometryObject,
			worldPoints,
			worldVertexNormals,
			screenVertexShades,
			activeVerticesBV);

	//
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		activeVerticesBV);

	M3rDraw_State_Commit();

	// 5. Clip and draw all active faces
		curIndexPtr = inGeometryObject->triStripArray->indices;

		UUmBitVector_Loop_Begin(i, numTris, activeTrisBV)
		{
			index2 = *curIndexPtr++;

			if(index2 & 0x80000000)
			{
				index0 = (index2 & 0x7FFFFFFF);
				index1 = *curIndexPtr++;
				index2 = *curIndexPtr++;
				clipCode0 = clipCodes[index0];
				clipCode1 = clipCodes[index1];
			}

			clipCode2 = clipCodes[index2];

			UUmBitVector_Loop_Test
			{

				UUmAssert(UUrBitVector_TestBit(activeVerticesBV, index0));
				UUmAssert(UUrBitVector_TestBit(activeVerticesBV, index1));
				UUmAssert(UUrBitVector_TestBit(activeVerticesBV, index2));

				if(clipCode0 & clipCode1 & clipCode2)
				{
					// Do nothing
				}
				else if(clipCode0 | clipCode1 | clipCode2)
				{
					MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = numPoints;
					MSrClip_Triangle(
						index0,
						index1,
						index2,
						clipCode0,
						clipCode1,
						clipCode2,
						0);
				}
				else
				{
					M3tTri	curTri;

					curTri.indices[0] = index0;
					curTri.indices[1] = index1;
					curTri.indices[2] = index2;

					M3rDraw_Triangle(
						&curTri);
				}
			}

			index0 = index1;
			index1 = index2;
			clipCode0 = clipCode1;
			clipCode1 = clipCode2;
		}
		UUmBitVector_Loop_End

	return UUcError_None;
}

static UUtError
MSiGeometry_Draw_BackFaceRemove_Clip_ShadeFace(
	M3tGeometry*		inGeometryObject)
{

	MSiGeometry_Draw_BackFaceKeep_Clip_ShadeFace(inGeometryObject);

	return UUcError_None;
}

static void MSiGeometry_Draw_Triangles_In_BitVector(UUtUns32 *activeTrisBV, UUtUns32 *activeVerticesBV, UUtUns32 numTris, UUtUns32 *indicies);

static UUtError
MSiGeometry_Draw_BackFaceRemove_ClipAccept_ShadeVertex(
	M3tGeometry*		inGeometryObject)
{
	M3tPoint3D*				worldPoints;
	M3tVector3D*			worldViewVectors;
	M3tVector3D*			worldVertexNormals;
	UUtUns32*				screenVertexShades;
	M3tPointScreen*			screenPoints;
	M3tVector3D*			worldTriNormals;

	UUtUns32				numTris = 0;
	UUtUns32				numPoints;

	UUtUns32*				activeTrisBV;
	UUtUns32*				activeVerticesBV;

	UUtUns32*				curIndexPtr;

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_Vertex);

	numPoints = inGeometryObject->pointArray->numPoints;
	worldPoints = MSgGeomContextPrivate->worldPoints;
	worldViewVectors = MSgGeomContextPrivate->worldViewVectors;
	worldTriNormals = MSgGeomContextPrivate->worldTriNormals;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;
	screenVertexShades = MSgGeomContextPrivate->shades_vertex;
	worldVertexNormals = MSgGeomContextPrivate->worldVertexNormals;

	// 0. init
	//		- active face list
	//		- active vertex list
	// 1. Transform all face normals to world
		MSgGeomContextPrivate->activeFunctions->transformFaceNormalToWorld(
			inGeometryObject,
			worldTriNormals);

		numTris = inGeometryObject->triNormalIndexArray->numIndices;
		activeTrisBV = MSgGeomContextPrivate->activeTrisBV;
		UUrBitVector_ClearBitAll(
			activeTrisBV,
			numTris);

		activeVerticesBV = MSgGeomContextPrivate->activeVerticesBV;
		UUrBitVector_ClearBitAll(
			activeVerticesBV,
			numPoints);

	// 2. Transform all points to world and normals for backface test
		MSgGeomContextPrivate->activeFunctions->transformPointListAndVertexNormalToWorldComputeViewVector(
			inGeometryObject,
			worldPoints,
			worldViewVectors,
			worldVertexNormals);

#if TOOL_VERSION
		if(0)
		{
			UUtUns32	itr;
			MStDebugEnvMap*	newEnvMap;
			float*			pXXXX;
			float*			pYYYY;
			float*			pZZZZ;
			float*			nXXXX;
			float*			nYYYY;
			float*			nZZZZ;
			M3tVector3D*	curViewVector = worldViewVectors;

			pXXXX = (float*)worldPoints;
			pYYYY = (float*)worldPoints + 4;
			pZZZZ = (float*)worldPoints + 8;
			nXXXX = (float*)worldVertexNormals;
			nYYYY = (float*)worldVertexNormals + 4;
			nZZZZ = (float*)worldVertexNormals + 8;

			for(itr = 0; itr < (numPoints >> 2); itr++)
			{
				newEnvMap = MSgGeomContextPrivate->debugEnvMap + MSgGeomContextPrivate->numDebugEnvMap++;
				newEnvMap->origin.x = pXXXX[0];
				newEnvMap->origin.y = pYYYY[0];
				newEnvMap->origin.z = pZZZZ[0];
				newEnvMap->normal.x = newEnvMap->origin.x + nXXXX[0];
				newEnvMap->normal.y = newEnvMap->origin.y + nYYYY[0];
				newEnvMap->normal.z = newEnvMap->origin.z + nZZZZ[0];
				newEnvMap->view.x = newEnvMap->origin.x + curViewVector->x;
				newEnvMap->view.y = newEnvMap->origin.y + curViewVector->y;
				newEnvMap->view.z = newEnvMap->origin.z + curViewVector->z;
				curViewVector++;

				newEnvMap = MSgGeomContextPrivate->debugEnvMap + MSgGeomContextPrivate->numDebugEnvMap++;
				newEnvMap->origin.x = pXXXX[1];
				newEnvMap->origin.y = pYYYY[1];
				newEnvMap->origin.z = pZZZZ[1];
				newEnvMap->normal.x = newEnvMap->origin.x + nXXXX[1];
				newEnvMap->normal.y = newEnvMap->origin.y + nYYYY[1];
				newEnvMap->normal.z = newEnvMap->origin.z + nZZZZ[1];
				newEnvMap->view.x = newEnvMap->origin.x + curViewVector->x;
				newEnvMap->view.y = newEnvMap->origin.y + curViewVector->y;
				newEnvMap->view.z = newEnvMap->origin.z + curViewVector->z;
				curViewVector++;

				newEnvMap = MSgGeomContextPrivate->debugEnvMap + MSgGeomContextPrivate->numDebugEnvMap++;
				newEnvMap->origin.x = pXXXX[2];
				newEnvMap->origin.y = pYYYY[2];
				newEnvMap->origin.z = pZZZZ[2];
				newEnvMap->normal.x = newEnvMap->origin.x + nXXXX[2];
				newEnvMap->normal.y = newEnvMap->origin.y + nYYYY[2];
				newEnvMap->normal.z = newEnvMap->origin.z + nZZZZ[2];
				newEnvMap->view.x = newEnvMap->origin.x + curViewVector->x;
				newEnvMap->view.y = newEnvMap->origin.y + curViewVector->y;
				newEnvMap->view.z = newEnvMap->origin.z + curViewVector->z;
				curViewVector++;

				newEnvMap = MSgGeomContextPrivate->debugEnvMap + MSgGeomContextPrivate->numDebugEnvMap++;
				newEnvMap->origin.x = pXXXX[3];
				newEnvMap->origin.y = pYYYY[3];
				newEnvMap->origin.z = pZZZZ[3];
				newEnvMap->normal.x = newEnvMap->origin.x + nXXXX[3];
				newEnvMap->normal.y = newEnvMap->origin.y + nYYYY[3];
				newEnvMap->normal.z = newEnvMap->origin.z + nZZZZ[3];
				newEnvMap->view.x = newEnvMap->origin.x + curViewVector->x;
				newEnvMap->view.y = newEnvMap->origin.y + curViewVector->y;
				newEnvMap->view.z = newEnvMap->origin.z + curViewVector->z;
				curViewVector++;

				pXXXX += 12;
				pYYYY += 12;
				pZZZZ += 12;
				nXXXX += 12;
				nYYYY += 12;
				nZZZZ += 12;
			}
		}
#endif

	// 3. Do backface remove test
	//		- mark all front facing faces as active
	//		- mark all vertices belonging to front facing faces as active
		MSgGeomContextPrivate->activeFunctions->backfaceRemove(
			inGeometryObject,
			worldPoints,
			worldViewVectors,
			worldTriNormals,
			activeTrisBV,
			activeVerticesBV);

	// 4. Transform all active vertices to frustum and screen space
		MSgGeomContextPrivate->activeFunctions->transformPointListToScreenActive(
			inGeometryObject,
			activeVerticesBV,
			screenPoints);

	// 5. Shade all active vertices
		MSgGeomContextPrivate->activeFunctions->shadeVerticesGouraudActive(
			inGeometryObject,
			worldPoints,
			worldVertexNormals,
			screenVertexShades,
			activeVerticesBV);


	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		activeVerticesBV);

	M3rDraw_State_Commit();

	// 6. Clip and draw all active faces
        curIndexPtr = inGeometryObject->triStripArray->indices;

#if 1
        MSiGeometry_Draw_Triangles_In_BitVector(activeTrisBV, activeVerticesBV, numTris, curIndexPtr);
#else
{
	UUtUns32				index0, index1, index2;
	UUtUns32				i;

		static struct {
                    unsigned int tris;
                    unsigned int strips;
                    unsigned int calls;
                } MSgCounters;

                MSgCounters.calls++;
                MSgCounters.tris += numTris;

		UUmBitVector_Loop_Begin(i, numTris, activeTrisBV)
		{
			index2 = *curIndexPtr++;

			if(index2 & 0x80000000)
			{
                                MSgCounters.strips++;

				index0 = (index2 & 0x7FFFFFFF);
				index1 = *curIndexPtr++;
				index2 = *curIndexPtr++;
			}

			UUmBitVector_Loop_Test
			{
				UUmAssert(UUrBitVector_TestBit(activeVerticesBV, index0));
				UUmAssert(UUrBitVector_TestBit(activeVerticesBV, index1));
				UUmAssert(UUrBitVector_TestBit(activeVerticesBV, index2));

				{
					M3tTri	curTri;

					curTri.indices[0] = index0;
					curTri.indices[1] = index1;
					curTri.indices[2] = index2;

					M3rDraw_Triangle(&curTri);
				}
			}

			index0 = index1;
			index1 = index2;
		}
		UUmBitVector_Loop_End
}
#endif

	return UUcError_None;
}

static void MSiGeometry_Draw_Triangles_In_BitVector(UUtUns32 *activeTrisBV, UUtUns32 *activeVerticesBV, UUtUns32 numTris, UUtUns32 *indicies)
{
	UUtUns32				index0, index1, index2, i;
//        M3rDraw_Triangle_Func triFunc;

//        triFunc = M3rDraw_GetTriangleFunction();

        UUmBitVector_Loop_Begin(i, numTris, activeTrisBV)
        {
                index2 = *indicies++;

                if(index2 & 0x80000000)
                {
                        index0 = (index2 & 0x7FFFFFFF);
                        index1 = *indicies++;
                        index2 = *indicies++;
                }

                UUmBitVector_Loop_Test
                {
                        UUmAssert(UUrBitVector_TestBit(activeVerticesBV, index0));
                        UUmAssert(UUrBitVector_TestBit(activeVerticesBV, index1));
                        UUmAssert(UUrBitVector_TestBit(activeVerticesBV, index2));

                        {
                                M3tTri	curTri;

                                curTri.indices[0] = index0;
                                curTri.indices[1] = index1;
                                curTri.indices[2] = index2;

//                                triFunc(&curTri);
                                M3rDraw_Triangle(&curTri);
                        }
                }

                index0 = index1;
                index1 = index2;
        }
        UUmBitVector_Loop_End
}

static UUtError
MSiGeometry_Draw_BackFaceRemove_ClipAccept_ShadeFace(
	M3tGeometry*		inGeometryObject)
{

	MSiGeometry_Draw_BackFaceKeep_ClipAccept_ShadeFace(inGeometryObject);

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_Geometry_Draw(
	M3tGeometry*		inGeometryObject)
{
	MStClipStatus					clipStatus;
	M3tPointScreen*					screenPoints;
	UUtUns8*						clipCodes;
	M3tPoint4D*						frustumPoints;
	UUtError						error;

	MSgGeomContextPrivate->numDebugEnvMap = 0;

	UUmAssertReadPtr(MSgGeomContextPrivate, sizeof(MStGeomContextPrivate));
	UUmAssertReadPtr(inGeometryObject, sizeof(M3tGeometry));

	clipCodes = MSgGeomContextPrivate->objectVertexData.clipCodes;
	frustumPoints = MSgGeomContextPrivate->objectVertexData.frustumPoints;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;

	UUmAssert(inGeometryObject->pointArray->numPoints < M3cMaxObjVertices);
	UUmAssert((inGeometryObject->triNormalArray == NULL) || inGeometryObject->triNormalArray->numVectors < M3cMaxObjTris);

//	TMrInstance_PrepareForUse(inGeometryObject);

	MSrTransform_UpdateMatrices();

	clipStatus =
		MSgGeomContextPrivate->activeFunctions->transformBoundingBoxClipStatus(
			inGeometryObject);

	if(clipStatus == MScClipStatus_TrivialReject)
	{
		return UUcError_None;
	}

	#if defined(DEBUGGING_CLIPPING) && DEBUGGING_CLIPPING

		if(gNo4DClip && clipStatus == MScClipStatus_Needs4DClipping)
		{
			return UUcError_None;
		}

		if(clipStatus == MScClipStatus_Needs3DClipping && gClipForce4D)
		{
			clipStatus = MScClipStatus_Needs4DClipping;
		}

	#endif

	M3rDraw_State_Push();

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		inGeometryObject->pointArray->numPoints);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		screenPoints);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenShadeArray_DC,
		MSgGeomContextPrivate->shades_vertex);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Clipping,
		clipStatus != MScClipStatus_TrivialAccept);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_VertexFormat,
		M3cDrawStateVertex_Unified);

	#if 0
		*((UUtUns32*)&MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Appearance]) = M3cGeomState_Appearance_Gouraud;
		*((UUtUns32*)&MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Shade]) = M3cGeomState_Shade_Face;
	#endif

	// set up clip data
		//MSgGeomContextPrivate->curClipData = &MSgGeomContextPrivate->objectVertexData;

		MSgGeomContextPrivate->objectVertexData.newClipTextureIndex = 0x5555; // garbage
		MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = 0x5555; // garbage - will be set later
		MSgGeomContextPrivate->objectVertexData.maxClipVertices =
			MSgGeomContextPrivate->objectVertexData.newClipVertexIndex + M3cExtraCoords;
		MSgGeomContextPrivate->objectVertexData.maxClipTextureCords = 0;

	if(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Appearance] == M3cGeomState_Appearance_Texture)
	{

		UUmAssert(inGeometryObject->baseMap != NULL);


		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_TextureCoordArray,
			inGeometryObject->texCoordArray->textureCoords);

		MSgGeomContextPrivate->objectVertexData.textureCoords = inGeometryObject->texCoordArray->textureCoords;

		UUmAssert(inGeometryObject->texCoordArray->numTextureCoords == inGeometryObject->pointArray->numPoints);

		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_BaseTextureMap,
			inGeometryObject->baseMap);

		if(inGeometryObject->baseMap->flags & M3cTextureFlags_ReceivesEnvMap)
		{

			M3rDraw_State_SetInt(
				M3cDrawStateIntType_Appearence,
				M3cDrawState_Appearence_Texture_Lit_EnvMap);

			M3rDraw_State_SetPtr(
				M3cDrawStatePtrType_EnvTextureCoordArray,
				MSgGeomContextPrivate->envMapCoords);

			if(inGeometryObject->baseMap->envMap != NULL)
			{
				M3rDraw_State_SetPtr(
					M3cDrawStatePtrType_EnvTextureMap,
					inGeometryObject->baseMap->envMap);
			}
			else
			{
				UUmAssert(M3rDraw_State_GetPtr(M3cDrawStatePtrType_EnvTextureMap) != NULL);
			}
			MSgGeomContextPrivate->polyComputeVertexProc =
				MSrClip_ComputeVertex_TextureEnvInterpolate;
		}
		else
		{
			M3rDraw_State_SetInt(
				M3cDrawStateIntType_Appearence,
				M3cDrawState_Appearence_Texture_Lit);
			M3rDraw_State_SetPtr(
				M3cDrawStatePtrType_EnvTextureCoordArray,
				NULL);
			M3rDraw_State_SetPtr(
				M3cDrawStatePtrType_EnvTextureMap,
				NULL);
			MSgGeomContextPrivate->polyComputeVertexProc =
				MSrClip_ComputeVertex_TextureInterpolate;
		}
	}
	else
	{
		M3rDraw_State_SetInt(
			M3cDrawStateIntType_Appearence,
			M3cDrawState_Appearence_Gouraud);
		M3rDraw_State_SetInt(
			M3cDrawStateIntType_ConstantColor,
			IMcShade_Red);
		MSgGeomContextPrivate->polyComputeVertexProc =
			MSrClip_ComputeVertex_GouraudInterpolate;
	}

	//Backfacing: On/Off
	//Clipping: Triv, 3D, 4D
	//Shade: Vert, Face
	error = UUcError_None;

	if(1 && (inGeometryObject->geometryFlags & M3cGeometryFlag_RemoveBackface))
	{
		if(clipStatus == MScClipStatus_TrivialAccept)
		{
			if(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Shade] == M3cGeomState_Shade_Vertex)
			{
				error = MSiGeometry_Draw_BackFaceRemove_ClipAccept_ShadeVertex(
							inGeometryObject);
			}
			else
			{
				error = MSiGeometry_Draw_BackFaceRemove_ClipAccept_ShadeFace(
							inGeometryObject);
			}
		}
		else
		{
			if(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Shade] == M3cGeomState_Shade_Vertex)
			{
				error = MSiGeometry_Draw_BackFaceRemove_Clip_ShadeVertex(
							inGeometryObject);
			}
			else
			{
				error = MSiGeometry_Draw_BackFaceRemove_Clip_ShadeFace(
							inGeometryObject);
			}
		}
	}
	else
	{
		if(clipStatus == MScClipStatus_TrivialAccept)
		{
			if(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Shade] == M3cGeomState_Shade_Vertex)
			{
				error = MSiGeometry_Draw_BackFaceKeep_ClipAccept_ShadeVertex(
							inGeometryObject);
			}
			else
			{
				error = MSiGeometry_Draw_BackFaceKeep_ClipAccept_ShadeFace(
							inGeometryObject);
			}
		}
		else
		{
			if(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Shade] == M3cGeomState_Shade_Vertex)
			{
				error = MSiGeometry_Draw_BackFaceKeep_Clip_ShadeVertex(
							inGeometryObject);
			}
			else
			{
				error = MSiGeometry_Draw_BackFaceKeep_Clip_ShadeFace(
							inGeometryObject);
			}
		}
	}

	M3rDraw_State_Pop();

	#if 0 && TOOL_VERSION
	{
		UUtUns32	dItr;

		M3rMatrixStack_Push();
		M3rMatrixStack_Identity();

		for(dItr = 0; dItr < MSgGeomContextPrivate->numDebugEnvMap; dItr++)
		{
			MSgGeomContextPrivate->submitPoints[0] = MSgGeomContextPrivate->debugEnvMap[dItr].origin;
			MSgGeomContextPrivate->submitPoints[1] = MSgGeomContextPrivate->debugEnvMap[dItr].normal;

			MSrGeomContext_Method_Geometry_LineDraw(
				2,
				MSgGeomContextPrivate->submitPoints,
				IMcShade_White);

			#if 0
			MSgGeomContextPrivate->submitPoints[0] = MSgGeomContextPrivate->debugEnvMap[dItr].origin;
			MSgGeomContextPrivate->submitPoints[1] = MSgGeomContextPrivate->debugEnvMap[dItr].reflection;

			MSrGeomContext_Method_Geometry_LineDraw(
				2,
				MSgGeomContextPrivate->submitPoints,
				IMcShade_Red);
			#endif

			MSgGeomContextPrivate->submitPoints[0] = MSgGeomContextPrivate->debugEnvMap[dItr].origin;
			MSgGeomContextPrivate->submitPoints[1] = MSgGeomContextPrivate->debugEnvMap[dItr].view;

			MSrGeomContext_Method_Geometry_LineDraw(
				2,
				MSgGeomContextPrivate->submitPoints,
				IMcShade_Green);
		}
		M3rMatrixStack_Pop();

	}
	#endif

	return error;
}

UUtError
MSrGeomContext_Method_Geometry_PolyDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade)
{
	M3tPointScreen*					screenPoints;
	M3tPoint4D*						frustumPoints;
	UUtUns8*						clipCodes;
	UUtUns32						i;
	M3tTri							newTri;

	frustumPoints = MSgGeomContextPrivate->objectVertexData.frustumPoints;
	clipCodes = MSgGeomContextPrivate->objectVertexData.clipCodes;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;

	M3rDraw_State_Push();

	// set up clip data
		//MSgGeomContextPrivate->curClipData = &MSgGeomContextPrivate->objectVertexData;

	MSrTransform_UpdateMatrices();

	MSrTransform_PointListToFrustumScreen(
		inNumPoints,
		inPoints,
		frustumPoints,
		screenPoints,
		clipCodes);

	MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = inNumPoints;
	MSgGeomContextPrivate->objectVertexData.maxClipVertices = inNumPoints + M3cExtraCoords;

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		screenPoints);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Appearence,
		M3cDrawState_Appearence_Gouraud);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_None);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		inShade);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		inNumPoints);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		NULL);

	M3rDraw_State_Commit();

	for(i = 1; i < inNumPoints - 1; i++)
	{
		if(clipCodes[0] | clipCodes[i] | clipCodes[i+1])
		{
			MSrClip_Triangle(
					0,
				i,
				(i+1),
				clipCodes[0],
				clipCodes[i],
				clipCodes[i+1],
				0);
		}
		else
		{
			newTri.indices[0] = 0;
			newTri.indices[1] = i;
			newTri.indices[2] = (i+1);

			M3rDraw_Triangle(&newTri);
		}
	}

	M3rDraw_State_Pop();

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_Geometry_LineDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade)
{
	M3tPointScreen*					screenPoints;
	M3tPoint4D*						frustumPoints;
	UUtUns8*						clipCodes;
	UUtUns32						i;

	frustumPoints = MSgGeomContextPrivate->objectVertexData.frustumPoints;
	clipCodes = MSgGeomContextPrivate->objectVertexData.clipCodes;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;

	M3rDraw_State_Push();

	// set up clip data
		//MSgGeomContextPrivate->curClipData = &MSgGeomContextPrivate->objectVertexData;

	MSrTransform_UpdateMatrices();

	MSrTransform_PointListToFrustumScreen(
		inNumPoints,
		inPoints,
		frustumPoints,
		screenPoints,
		clipCodes);

	MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = inNumPoints;

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		screenPoints);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		inShade);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Appearence,
		M3cDrawState_Appearence_Gouraud);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		0);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		NULL);

	M3rDraw_State_Commit();

	for(i = 0; i < inNumPoints - 1; i++)
	{
		if(clipCodes[i] | clipCodes[i+1])
		{
			MSrClip_Line(
					i,
				(i+1),
				clipCodes[i],
				clipCodes[i+1],
				0);
		}
		else
		{
			M3rDraw_Line(i, (i+1));
		}
	}

	M3rDraw_State_Pop();

	return UUcError_None;
}

#if 0
UUtError
MSrGeomContext_Method_Geometry_LineDraw2D(
	UUtUns32			inNumPoints,
	M3tPointScreen*		inPoints,
	UUtUns32			inShade)
{
	UUtUns32 i;
	float w,h;

	M3rDraw_State_Push();

	w = (float)M3rDraw_GetWidth();
	h = (float)M3rDraw_GetHeight();
	MSrTransform_UpdateMatrices();

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		inPoints);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		inShade);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		0);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		NULL);

	M3rDraw_State_Commit();

	for(i = 0; i < inNumPoints - 1; i++)
	{
		// Do braindead clipping
		if (inPoints[i].x < 0.0f) inPoints[i].x = 0.0f;
		else if (inPoints[i].x > w) inPoints[i].x = w;
		if (inPoints[i].y < 0.0f) inPoints[i].y = 0.0f;
		else if (inPoints[i].y > h) inPoints[i].y = h;

		M3rDraw_Line(i, (i+1));
	}

	M3rDraw_State_Pop();

	return UUcError_None;
}
#endif

UUtError
MSrGeomContext_Method_Geometry_PointDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade)
{
	M3tPointScreen*					screenPoints;
	M3tPoint4D*						frustumPoints;
	UUtUns8*						clipCodes;
	UUtUns32						i;

	frustumPoints = MSgGeomContextPrivate->objectVertexData.frustumPoints;
	clipCodes = MSgGeomContextPrivate->objectVertexData.clipCodes;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;

	M3rDraw_State_Push();

	// set up clip data
		//MSgGeomContextPrivate->curClipData = &MSgGeomContextPrivate->objectVertexData;

	MSrTransform_UpdateMatrices();

	MSrTransform_PointListToFrustumScreen(
		inNumPoints,
		inPoints,
		frustumPoints,
		screenPoints,
		clipCodes);

	MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = inNumPoints;

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		screenPoints);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		inShade);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		0);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		NULL);

	M3rDraw_State_Commit();

	for(i = 0; i < inNumPoints; i++)
	{
		if(clipCodes[i])
		{
		}
		else
		{
			M3rDraw_Point(screenPoints + i);
		}
	}

	M3rDraw_State_Pop();

	return UUcError_None;
}

static UUtError
MSiSprite_Draw_Unoriented(
	M3tTextureMap*		inTextureMap,
	M3tPoint3D*			inPoint,
	float				inHorizSize,
	float				inVertSize,
	UUtUns32			inShade,
	UUtUns32			inAlpha,
	float				inXOffset,
	float				inXShorten,
	float				inXChop)
{
	float oldAmount, newAmount, amount, orig_xd, xd, yd;

	UUtUns8 screenPointBuf[sizeof(M3tPointScreen) + (2 * UUcProcessor_CacheLineSize)];
	M3tPointScreen *screenPoint = UUrAlignMemory(screenPointBuf);

	UUtUns8 frustumPointBuf[sizeof(M3tPoint4D) + (2 * UUcProcessor_CacheLineSize)];
	M3tPoint4D *frustumPoint = UUrAlignMemory(frustumPointBuf);

	UUtUns8 inPointBuf[sizeof(M3tPoint3D) + (2 * UUcProcessor_CacheLineSize)];
	M3tPoint3D *inPointAligned = UUrAlignMemory(inPointBuf);

	M3tPointScreen spriteScreenPoints[2];
	M3tTextureCoord spriteTextureCoords[2];
	M3tTextureCoord spriteTextureFinal[4];

	UUtUns8	clipCode;

	UUtUns16 drawWidth = M3rDraw_GetWidth();
	UUtUns16 drawHeight = M3rDraw_GetHeight();

	float xScale = drawWidth * 0.78125f; // S.S. (500.f / 640.f);
	float yScale = drawHeight * 1.041666667f; // S.S. (500.f / 480.f);

	float preClipWidth, preClipHeight;

	UUmAssertReadPtr(inTextureMap, sizeof(*inTextureMap));
	UUmAssertReadPtr(inPoint, sizeof(*inPoint));

	MSrTransform_UpdateMatrices();

	*inPointAligned = *inPoint;

	MSrTransform_PointListToFrustumScreen(
		1,
		inPointAligned,
		frustumPoint,
		screenPoint,
		&clipCode);

	orig_xd = xScale * inHorizSize * screenPoint->invW;
	yd = yScale * inVertSize  * screenPoint->invW;

	xd = orig_xd * (1.0f - inXShorten);

	spriteTextureCoords[0].u = inXShorten * 0.5f; // S.S. / 2;
	spriteTextureCoords[1].u = 1.f - (inXShorten * 0.5f /* S.S. / 2*/);

	spriteTextureCoords[0].v = 0.f;
	spriteTextureCoords[1].v = 1.f;

	spriteScreenPoints[0] = *screenPoint;
	spriteScreenPoints[1] = *screenPoint;

	if (inXOffset != 0.f) {
		spriteScreenPoints[0].x = spriteScreenPoints[1].x = screenPoint->x + xd * inXOffset;
	}

	spriteScreenPoints[0].x -= xd;	spriteScreenPoints[1].x += xd;
	spriteScreenPoints[0].y -= yd;	spriteScreenPoints[1].y += yd;

	if (inXChop != 0.0f) {
		// remove a certain amount from the front of the sprite
		spriteScreenPoints[1].x -= 2 * inXChop * orig_xd;
		if (spriteScreenPoints[1].x <= spriteScreenPoints[0].x)
			return UUcError_None;

		spriteTextureCoords[1].u -= inXChop;
	}

	// test for trivial reject
	if ((spriteScreenPoints[0].x >= drawWidth) ||
		(spriteScreenPoints[0].y >= drawHeight) ||
		(spriteScreenPoints[1].x <= 0) ||
		(spriteScreenPoints[1].y <= 0) ||
		(frustumPoint->z <= MSgGeomContextPrivate->activeCamera->zNear) ||
		(frustumPoint->z >= MSgGeomContextPrivate->activeCamera->zFar))
	{
		return UUcError_None;
	}

	preClipWidth = spriteScreenPoints[1].x - spriteScreenPoints[0].x;
	preClipHeight = spriteScreenPoints[1].y - spriteScreenPoints[0].y;

	if (spriteScreenPoints[0].x < 0) {
		oldAmount = preClipWidth;
		newAmount = preClipWidth - (0 - spriteScreenPoints[0].x);
		spriteScreenPoints[0].x = 0.f;

		amount = 1.f - (newAmount / oldAmount);

		spriteTextureCoords[0].u = amount;
	}

	if (spriteScreenPoints[0].y < 0) {
		oldAmount = preClipHeight;
		newAmount = preClipHeight - (0 - spriteScreenPoints[0].y);
		spriteScreenPoints[0].y = 0.f;

		amount = 1.f - (newAmount / oldAmount);

		spriteTextureCoords[0].v = amount;
	}

	if (spriteScreenPoints[1].x >= drawWidth) {
		oldAmount = preClipWidth;
		newAmount = preClipWidth - (spriteScreenPoints[1].x - drawWidth);
		spriteScreenPoints[1].x = drawWidth;

		amount = 1.f - (newAmount / oldAmount);

		spriteTextureCoords[1].u -= amount;
	}

	if (spriteScreenPoints[1].y >= drawHeight) {
		oldAmount = preClipHeight;
		newAmount = preClipHeight - (spriteScreenPoints[1].y - drawHeight);
		spriteScreenPoints[1].y = drawHeight;

		amount = 1.f - (newAmount / oldAmount);

		spriteTextureCoords[1].v -= amount;
	}

	spriteTextureFinal[0].u = spriteTextureCoords[0].u;	// tl
	spriteTextureFinal[0].v = spriteTextureCoords[0].v;
	spriteTextureFinal[1].u = spriteTextureCoords[1].u;	// tr
	spriteTextureFinal[1].v = spriteTextureCoords[0].v;
	spriteTextureFinal[2].u = spriteTextureCoords[0].u;	// bl
	spriteTextureFinal[2].v = spriteTextureCoords[1].v;
	spriteTextureFinal[3].u = spriteTextureCoords[1].u;	// br
	spriteTextureFinal[3].v = spriteTextureCoords[1].v;

	M3rDraw_State_Push();

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_BaseTextureMap,
		inTextureMap);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		0);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		NULL);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		inShade);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Alpha,
		inAlpha);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_None);

	M3rDraw_State_Commit();

	M3rDraw_Sprite(
		spriteScreenPoints,
		spriteTextureFinal);

	M3rDraw_State_Pop();

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_Sprite_Draw(
	M3tTextureMap*		inTextureMap,
	M3tPoint3D*			inPoint,
	float				inHorizSize,
	float				inVertSize,
	UUtUns32			inShade,
	UUtUns16			inAlpha,
	float				inRotation,
	M3tVector3D*		inDirection,
	M3tMatrix3x3*		inOrientation,
	float				inXOffset,
	float				inXShorten,
	float				inXChop)
{
	M3tGeomState_SpriteMode sprite_mode = MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_SpriteMode];
	M3tVector3D sprite_X, sprite_Y;
	M3tVector3D *cameraPos, *cameraFwd, *cameraUp, *cameraRight, cameraDir, particleDir;
	float spriteorient_x, spriteorient_y, d;
	float x_0, x_1, u_0, u_1;

	if ((sprite_mode == M3cGeomState_SpriteMode_Normal) && (inRotation == 0)) {
		// CB: this is an unrotated, unoriented sprite and so may be drawn along
		// screen X and Y. this requires only 2 points instead of 4.
		return MSiSprite_Draw_Unoriented(inTextureMap, inPoint, inHorizSize, inVertSize, inShade, inAlpha, inXOffset, inXShorten, inXChop);

	}
	MSrTransform_UpdateMatrices();

	// get the active camera's position and orientation
	cameraPos	= &MSgGeomContextPrivate->activeCamera->cameraLocation;
	cameraFwd	= &MSgGeomContextPrivate->activeCamera->viewVector;
	cameraUp	= &MSgGeomContextPrivate->activeCamera->upVector;
	cameraRight	= &MSgGeomContextPrivate->activeCamera->crossVector;

	UUmAssert(MUmVector_IsNormalized(*cameraFwd));
	UUmAssert(MUmVector_IsNormalized(*cameraUp));
	UUmAssert(MUmVector_IsNormalized(*cameraRight));
	UUmAssert(UUmFloat_CompareEqu(MUrVector_DotProduct(cameraUp,  cameraFwd  ), 0));
	UUmAssert(UUmFloat_CompareEqu(MUrVector_DotProduct(cameraUp,  cameraRight), 0));
	UUmAssert(UUmFloat_CompareEqu(MUrVector_DotProduct(cameraFwd, cameraRight), 0));

	// determine the sprite's orientation in space
	switch(sprite_mode) {
	case M3cGeomState_SpriteMode_Normal:
		sprite_X = *cameraRight;
		sprite_Y = *cameraUp;
		break;

	case M3cGeomState_SpriteMode_Rotated:
		// CB: this is a rotated sprite that faces the screen. calculate the
		// orientation of the sprite.

		if (inOrientation != NULL) {
			// oriented towards particle's +Y
			MUrMatrix_GetCol((M3tMatrix4x3 *) inOrientation, 1, &particleDir);
//			UUmAssert(MUmVector_IsNormalized(particleDir));
			spriteorient_x = MUrVector_DotProduct(cameraRight, &particleDir);
			spriteorient_y = MUrVector_DotProduct(cameraUp, &particleDir);

		} else if (inDirection != NULL) {
			// oriented towards particle's direction
			spriteorient_x = MUrVector_DotProduct(cameraRight, inDirection);
			spriteorient_y = MUrVector_DotProduct(cameraUp, inDirection);
		} else {
			COrConsole_Printf("### oriented particle supplied no orientation or direction");
			spriteorient_x = 1.0f;
			spriteorient_y = 0.0f;
		}

		d = UUmSQR(spriteorient_x) + UUmSQR(spriteorient_y);
		if (d < 1e-08) {
			spriteorient_x = 1.0f;
			spriteorient_y = 0.0f;
		} else {
			d = MUrSqrt(d);

			spriteorient_x /= d;
			spriteorient_y /= d;
		}

		// the sprite faces the camera
		sprite_X.x = spriteorient_x * cameraRight->x + spriteorient_y * cameraUp->x;
		sprite_X.y = spriteorient_x * cameraRight->y + spriteorient_y * cameraUp->y;
		sprite_X.z = spriteorient_x * cameraRight->z + spriteorient_y * cameraUp->z;

		sprite_Y.x = spriteorient_y * cameraRight->x - spriteorient_x * cameraUp->x;
		sprite_Y.y = spriteorient_y * cameraRight->y - spriteorient_x * cameraUp->y;
		sprite_Y.z = spriteorient_y * cameraRight->z - spriteorient_x * cameraUp->z;
		break;

	case M3cGeomState_SpriteMode_Billboard:
		// CB: this sprite is oriented along its direction but perpendicular to the camera
		if (inOrientation != NULL) {
			MUrMatrix_GetCol((M3tMatrix4x3 *) inOrientation, 1, &sprite_X);
		} else if (inDirection != NULL) {
			// oriented towards particle's direction
			d = MUmVector_GetLengthSquared(*inDirection);

			if (d < 1e-08) {
				MUmVector_Set(sprite_X, 0, 1, 0);	// FIXME: we might want to use last valid direction
			} else {
				d = MUrSqrt(d);

				MUmVector_Copy(sprite_X, *inDirection);
				MUmVector_Scale(sprite_X, 1.0f / d);
			}
		} else {
			UUmAssert(!"oriented particle supplied no orientation or direction");
		}

		MUmVector_Subtract(cameraDir, *inPoint, *cameraPos);
		MUrVector_CrossProduct(&sprite_X, &cameraDir, &sprite_Y);
		MUmVector_Normalize(sprite_Y);
		break;

	case M3cGeomState_SpriteMode_Arrow:
		// CB: this sprite is oriented along its direction and upvector
		UUmAssert(inOrientation != NULL);
		MUrMatrix_GetCol((M3tMatrix4x3 *) inOrientation, 1, &sprite_X);
		MUrMatrix_GetCol((M3tMatrix4x3 *) inOrientation, 2, &sprite_Y);
		break;

	case M3cGeomState_SpriteMode_Discus:
		// CB: this sprite is oriented along its direction and rightvector
		UUmAssert(inOrientation != NULL);
		MUrMatrix_GetCol((M3tMatrix4x3 *) inOrientation, 1, &sprite_X);
		MUrMatrix_GetCol((M3tMatrix4x3 *) inOrientation, 0, &sprite_Y);
		break;

	case M3cGeomState_SpriteMode_Flat:
		// CB: this sprite is oriented perpendicular to its direction
		UUmAssert(inOrientation != NULL);
		MUrMatrix_GetCol((M3tMatrix4x3 *) inOrientation, 0, &sprite_X);
		MUrMatrix_GetCol((M3tMatrix4x3 *) inOrientation, 2, &sprite_Y);
		break;

	default:
		UUmAssert(!"unknown sprite mode");
		break;
	}

//	UUmAssert(MUmVector_IsNormalized(sprite_X));
//	UUmAssert(MUmVector_IsNormalized(sprite_Y));

	// rotate sprite's axes
	if (inRotation != 0.0f) {
		UUtInt16 pi_multiples;
		float theta, costheta, sintheta;
		M3tVector3D temp_x;

		// convert theta into radians between 0 and 2pi
		theta = inRotation * 0.01745329252f; // S.S. M3c2Pi / 360.0f;
		pi_multiples = (UUtInt16)(theta * 0.1591549431f); // S.S. (theta / M3c2Pi);
		if (theta < 0)
			theta = theta + M3c2Pi * (1 - pi_multiples);
		else
			theta = theta + M3c2Pi * (  - pi_multiples);

		costheta = MUrCos(theta);
		sintheta = MUrSqrt(1.0f - UUmSQR(costheta));
		if (theta > M3cPi)
			sintheta = -sintheta;

		temp_x.x = sprite_X.x * costheta - sprite_Y.x * sintheta;
		temp_x.y = sprite_X.y * costheta - sprite_Y.y * sintheta;
		temp_x.z = sprite_X.z * costheta - sprite_Y.z * sintheta;

		sprite_Y.x = sprite_X.x * sintheta + sprite_Y.x * costheta;
		sprite_Y.y = sprite_X.y * sintheta + sprite_Y.y * costheta;
		sprite_Y.z = sprite_X.z * sintheta + sprite_Y.z * costheta;

		MUmVector_Copy(sprite_X, temp_x);
	}


	{
		/*
		 * draw the sprite at inPoint along sprite_X and sprite_Y
		 */

		M3tPointScreen *screenPoint = MSgGeomContextPrivate->objectVertexData.screenPoints;
		M3tPoint4D *frustumPoint = MSgGeomContextPrivate->objectVertexData.frustumPoints;
		M3tPoint3D *worldPoint = MSgGeomContextPrivate->worldPoints;
		M3tTextureCoord spriteTextureCoords[4 + M3cExtraCoords];

		UUtUns8	clipCode[4];
		MStClipStatus clipStatus;

		M3tQuad quadIndices;

		UUmAssertReadPtr(inTextureMap, sizeof(*inTextureMap));
		UUmAssertReadPtr(inPoint, sizeof(*inPoint));

		MSrTransform_UpdateMatrices();


		MUmVector_Scale(sprite_X, inHorizSize);
		MUmVector_Scale(sprite_Y, inVertSize);

		// use inXOffset and inXShorten to work out the location and texture coordinates for
		// the left and right sides of the sprite
		//
		// inXOffset changes whereabouts inPoint is located. +1 = left edge of sprite, 0 = middle of sprite, -1 = right edge
		//
		// inXShorten removes that proportion of the sprite from each side of inPoint
		//
		// e.g.
		//
		// |----x----|              inXOffset = 0   inXShorten = 0
		//
		//   |--x--|                inXOffset = 0   inXShorten = 0.5
		//
		//      x-------|           inXOffset = +1  inXShorten = 0
		//
		//      x----|              inXOffset = +1  inXShorten = 0.5

		x_0 = (1.0f - inXShorten) * (inXOffset - 1.0f);		u_0 = 0.0f - 0.5f * inXShorten * (inXOffset - 1.0f);
		x_1 = (1.0f - inXShorten) * (inXOffset + 1.0f);		u_1 = 1.0f - 0.5f * inXShorten * (inXOffset + 1.0f);

		if (inXChop != 0.0f) {
			// remove a certain amount from the front of the sprite
			x_1 -= 2.0f * inXChop;
			if (x_1 <= x_0)
				return UUcError_None;

			u_1 -= 1.0f * inXChop;
		}

		// make the sprite's world points and store in the global geomcontext's world point array
		worldPoint[0].x = inPoint->x + x_0 * sprite_X.x - sprite_Y.x;
		worldPoint[0].y = inPoint->y + x_0 * sprite_X.y - sprite_Y.y;
		worldPoint[0].z = inPoint->z + x_0 * sprite_X.z - sprite_Y.z;

		worldPoint[1].x = inPoint->x + x_1 * sprite_X.x - sprite_Y.x;
		worldPoint[1].y = inPoint->y + x_1 * sprite_X.y - sprite_Y.y;
		worldPoint[1].z = inPoint->z + x_1 * sprite_X.z - sprite_Y.z;

		worldPoint[2].x = inPoint->x + x_0 * sprite_X.x + sprite_Y.x;
		worldPoint[2].y = inPoint->y + x_0 * sprite_X.y + sprite_Y.y;
		worldPoint[2].z = inPoint->z + x_0 * sprite_X.z + sprite_Y.z;

		worldPoint[3].x = inPoint->x + x_1 * sprite_X.x + sprite_Y.x;
		worldPoint[3].y = inPoint->y + x_1 * sprite_X.y + sprite_Y.y;
		worldPoint[3].z = inPoint->z + x_1 * sprite_X.z + sprite_Y.z;

		// transform these points into the global transformed object data arrays
		clipStatus = MSrTransform_PointListToFrustumScreen(4, worldPoint, frustumPoint, screenPoint, clipCode);

		if (clipStatus == MScClipStatus_TrivialReject)
			return UUcError_None;

		spriteTextureCoords[0].u = u_0;
		spriteTextureCoords[0].v = 0.f;
		spriteTextureCoords[1].u = u_1;
		spriteTextureCoords[1].v = 0.f;
		spriteTextureCoords[2].u = u_0;
		spriteTextureCoords[2].v = 1.f;
		spriteTextureCoords[3].u = u_1;
		spriteTextureCoords[3].v = 1.f;

		M3rDraw_State_Push();

		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_ScreenPointArray,
			screenPoint);

		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_BaseTextureMap,
			inTextureMap);

		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_TextureCoordArray,
			spriteTextureCoords);
		MSgGeomContextPrivate->objectVertexData.textureCoords = spriteTextureCoords;

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_NumRealVertices,
			4);

		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_VertexBitVector,
			NULL);

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_Interpolation,
			M3cDrawState_Interpolation_None);

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_ConstantColor,
			inShade);

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_Alpha,
			inAlpha);

		M3rDraw_State_Commit();

		if (clipStatus == MScClipStatus_NeedsClipping) {
			MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = 4;
			MSgGeomContextPrivate->objectVertexData.maxClipTextureCords = 4 + M3cExtraCoords;
			MSgGeomContextPrivate->objectVertexData.maxClipVertices = 4 + M3cExtraCoords;

			MSgGeomContextPrivate->polyComputeVertexProc = MSrClip_ComputeVertex_TextureFlat;

			MSrClip_Quad(0, 1, 3, 2, clipCode[0], clipCode[1], clipCode[3], clipCode[2], 0);
		} else {
			quadIndices.indices[0] = 0;
			quadIndices.indices[1] = 1;
			quadIndices.indices[2] = 3;
			quadIndices.indices[3] = 2;
			M3rDraw_Quad(&quadIndices);
		}

		M3rDraw_State_Pop();

		// we allocated contrailTextureCoords off the stack, so after our exit from here
		// it will be invalid. nothing should be using MSgGeomContextPrivate->objectVertexData.textureCoords
		// anyway, but make absolutely sure that they won't
		MSgGeomContextPrivate->objectVertexData.textureCoords = NULL;
	}

	return UUcError_None;
}

extern UUtBool					M3gDraw_Sorting;

#define							M3gDraw_SpriteArray_MaxSprites				1000
M3tPointScreen					M3gDraw_SpriteArray_ScreenPoints[2 * M3gDraw_SpriteArray_MaxSprites];
M3tTextureCoord					M3gDraw_SpriteArray_TextureCoords[4 * M3gDraw_SpriteArray_MaxSprites];
UUtUns32						M3gDraw_SpriteArray_Colors[M3gDraw_SpriteArray_MaxSprites];

UUtError
MSrGeomContext_Method_SpriteArray_Draw(
	M3tSpriteArray		*inSpriteArray )
{
	M3tGeomState_SpriteMode	sprite_mode = MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_SpriteMode];
//	M3tVector3D				*cameraPos, *cameraFwd, *cameraUp, *cameraRight;
	float					oldAmount, newAmount, xd, yd;

	UUtUns8					screenPointBuf[sizeof(M3tPointScreen) * 4 + (2 * UUcProcessor_CacheLineSize)];
	M3tPointScreen			*screenPoint = UUrAlignMemory(screenPointBuf);

	UUtUns8					frustumPointBuf[sizeof(M3tPoint4D) + (2 * UUcProcessor_CacheLineSize)];
	M3tPoint4D				*frustumPoint = UUrAlignMemory(frustumPointBuf);

	UUtUns8					inPointBuf[sizeof(M3tPoint3D) * 8 + (2 * UUcProcessor_CacheLineSize)];
	M3tPoint3D				*world_points = UUrAlignMemory(inPointBuf);

	//M3tPointScreen			spriteScreenPoints[2 * 8];
	M3tTextureCoord			spriteTextureCoords[2];
	UUtUns32				j;
	UUtUns8					clipCode;

	UUtUns16				drawWidth = M3rDraw_GetWidth();
	UUtUns16				drawHeight = M3rDraw_GetHeight();

	float					xScale = drawWidth * 0.78125f; // S.S. (500.f / 640.f);
	float					yScale = drawHeight * 1.04166667f; // S.S. (500.f / 480.f);
	UUtUns32				i;
	UUtBool					old_sorting;
	float					preClipWidth, preClipHeight;
	M3tSpriteInstance*		instance;

	MSrTransform_UpdateMatrices();
/*
	// get the active camera's position and orientation
	cameraPos	= &MSgGeomContextPrivate->activeCamera->cameraLocation;
	cameraFwd	= &MSgGeomContextPrivate->activeCamera->viewVector;
	cameraUp	= &MSgGeomContextPrivate->activeCamera->upVector;
	cameraRight	= &MSgGeomContextPrivate->activeCamera->crossVector;
	UUmAssert(MUmVector_IsNormalized(*cameraFwd));
	UUmAssert(MUmVector_IsNormalized(*cameraUp));
	UUmAssert(MUmVector_IsNormalized(*cameraRight));
	UUmAssert(UUmFloat_CompareEqu(MUrVector_DotProduct(cameraUp,  cameraFwd  ), 0));
	UUmAssert(UUmFloat_CompareEqu(MUrVector_DotProduct(cameraUp,  cameraRight), 0));
	UUmAssert(UUmFloat_CompareEqu(MUrVector_DotProduct(cameraFwd, cameraRight), 0));
*/
	old_sorting						= M3gDraw_Sorting;

	M3gDraw_Sorting					= UUcFalse;

	j = 0;
	for( i = 0; i < inSpriteArray->sprite_count; i++ )
	{
		instance						= &inSpriteArray->sprites[i];
		*world_points					= instance->position;

		MSrTransform_PointListToFrustumScreen( 1, world_points, frustumPoint, screenPoint, &clipCode );

		if( (frustumPoint->z <= MSgGeomContextPrivate->activeCamera->zNear) || (frustumPoint->z >= MSgGeomContextPrivate->activeCamera->zFar))
			continue;

		xd											= xScale * instance->scale * screenPoint->invW;
		yd											= yScale * instance->scale * screenPoint->invW;

		M3gDraw_SpriteArray_ScreenPoints[j*2+0].x			= screenPoint->x - xd;
		if( M3gDraw_SpriteArray_ScreenPoints[j*2+0].x >= drawWidth )				continue;

		M3gDraw_SpriteArray_ScreenPoints[j*2+0].y			= screenPoint->y - yd;
		if( M3gDraw_SpriteArray_ScreenPoints[j*2+0].y >= drawHeight )				continue;

		M3gDraw_SpriteArray_ScreenPoints[j*2+1].x			= screenPoint->x + xd;
		if( M3gDraw_SpriteArray_ScreenPoints[j*2+1].x <= 0 )						continue;

		M3gDraw_SpriteArray_ScreenPoints[j*2+1].y			= screenPoint->y + yd;
		if( M3gDraw_SpriteArray_ScreenPoints[j*2+1].y <= 0 )						continue;

		M3gDraw_SpriteArray_ScreenPoints[j*2+0].z			= screenPoint->z;
		M3gDraw_SpriteArray_ScreenPoints[j*2+0].invW		= screenPoint->invW;

		M3gDraw_SpriteArray_ScreenPoints[j*2+1].z			= screenPoint->z;
		M3gDraw_SpriteArray_ScreenPoints[j*2+1].invW		= screenPoint->invW;

		preClipWidth					= M3gDraw_SpriteArray_ScreenPoints[j*2+1].x - M3gDraw_SpriteArray_ScreenPoints[j*2].x;
		preClipHeight					= M3gDraw_SpriteArray_ScreenPoints[j*2+1].y - M3gDraw_SpriteArray_ScreenPoints[j*2].y;

		spriteTextureCoords[0].u		= 0.f;
		spriteTextureCoords[1].u		= 1.f;
		spriteTextureCoords[0].v		= 0.f;
		spriteTextureCoords[1].v		= 1.f;

		// FIXME: clipping is whacked...
		if( M3gDraw_SpriteArray_ScreenPoints[j*2+0].x < 0 )
		{
			oldAmount					= preClipWidth;
			newAmount					= preClipWidth + M3gDraw_SpriteArray_ScreenPoints[j*2+0].x;
			M3gDraw_SpriteArray_ScreenPoints[j*2+0].x		= 0.f;
			spriteTextureCoords[0].u	= 1.f - (newAmount / oldAmount);
		}

		if( M3gDraw_SpriteArray_ScreenPoints[j*2+0].y < 0 )
		{
			oldAmount					= preClipHeight;
			newAmount					= preClipHeight + M3gDraw_SpriteArray_ScreenPoints[j*2+0].y;
			M3gDraw_SpriteArray_ScreenPoints[j*2+0].y		= 0.f;
			spriteTextureCoords[0].v	= 1.f - (newAmount / oldAmount);
		}

		if( M3gDraw_SpriteArray_ScreenPoints[j*2+1].x >= drawWidth )
		{
			oldAmount					= preClipWidth;
			newAmount					= preClipWidth - (M3gDraw_SpriteArray_ScreenPoints[j*2+1].x - drawWidth);
			M3gDraw_SpriteArray_ScreenPoints[j*2+1].x = drawWidth;
			spriteTextureCoords[1].u	-= 1.f - (newAmount / oldAmount);
		}

		if( M3gDraw_SpriteArray_ScreenPoints[j*2+1].y >= drawHeight )
		{
			oldAmount					= preClipHeight;
			newAmount					= preClipHeight - (M3gDraw_SpriteArray_ScreenPoints[j*2+1].y - drawHeight);
			M3gDraw_SpriteArray_ScreenPoints[j*2+1].y = drawHeight;
			spriteTextureCoords[1].v	-= 1.f - (newAmount / oldAmount);
		}

		M3gDraw_SpriteArray_TextureCoords[j*4+0].u		= spriteTextureCoords[0].u;	// tl
		M3gDraw_SpriteArray_TextureCoords[j*4+0].v		= spriteTextureCoords[0].v;
		M3gDraw_SpriteArray_TextureCoords[j*4+1].u		= spriteTextureCoords[1].u;	// tr
		M3gDraw_SpriteArray_TextureCoords[j*4+1].v		= spriteTextureCoords[0].v;
		M3gDraw_SpriteArray_TextureCoords[j*4+2].u		= spriteTextureCoords[0].u;	// bl
		M3gDraw_SpriteArray_TextureCoords[j*4+2].v		= spriteTextureCoords[1].v;
		M3gDraw_SpriteArray_TextureCoords[j*4+3].u		= spriteTextureCoords[1].u;	// br
		M3gDraw_SpriteArray_TextureCoords[j*4+3].v		= spriteTextureCoords[1].v;

		M3gDraw_SpriteArray_Colors[j]						= (instance->alpha << 24) | 0xffffff;

		j++;
	}

	//M3rDraw_State_Commit();
	if( j )
	{
		M3rDraw_State_Push();

			M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap,inSpriteArray->texture_map);

			M3rDraw_State_SetInt(M3cDrawStateIntType_NumRealVertices,0);

			M3rDraw_State_SetPtr(M3cDrawStatePtrType_VertexBitVector,NULL);

			M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, M3cMaxAlpha);

			M3rDraw_State_SetInt(M3cDrawStateIntType_Interpolation,M3cDrawState_Interpolation_None);

			M3rDraw_State_Commit();

			M3rDraw_SpriteArray( M3gDraw_SpriteArray_ScreenPoints, M3gDraw_SpriteArray_TextureCoords, M3gDraw_SpriteArray_Colors, j );

		M3rDraw_State_Pop();
	}

	M3gDraw_Sorting	= old_sorting;

	return UUcError_None;
}


UUtError
MSrGeomContext_Method_Contrail_Draw(
			M3tTextureMap*		inTextureMap,
			float				inV0,
			float				inV1,
			M3tContrailData*	inPoint0,
			M3tContrailData*	inPoint1)
{
	M3tPointScreen *screenPoint = MSgGeomContextPrivate->objectVertexData.screenPoints;
	M3tPoint4D *frustumPoint = MSgGeomContextPrivate->objectVertexData.frustumPoints;
	M3tPoint3D *worldPoint = MSgGeomContextPrivate->worldPoints;
	M3tTextureCoord contrailTextureCoords[4 + M3cExtraCoords];
	UUtUns32 contrailShades[4 + M3cExtraCoords];
	UUtUns8	clipCode[4];
	MStClipStatus clipStatus;
	M3tQuad quadIndices;

	UUmAssertReadPtr(inTextureMap, sizeof(*inTextureMap));
	UUmAssertReadPtr(inPoint0, sizeof(*inPoint0));
	UUmAssertReadPtr(inPoint1, sizeof(*inPoint1));

	MSrTransform_UpdateMatrices();

	// make the contrail's world points and store in the global geomcontext's world point array
	worldPoint[0].x = inPoint0->position.x + inPoint0->width.x;
	worldPoint[0].y = inPoint0->position.y + inPoint0->width.y;
	worldPoint[0].z = inPoint0->position.z + inPoint0->width.z;

	worldPoint[1].x = inPoint0->position.x - inPoint0->width.x;
	worldPoint[1].y = inPoint0->position.y - inPoint0->width.y;
	worldPoint[1].z = inPoint0->position.z - inPoint0->width.z;

	worldPoint[2].x = inPoint1->position.x + inPoint1->width.x;
	worldPoint[2].y = inPoint1->position.y + inPoint1->width.y;
	worldPoint[2].z = inPoint1->position.z + inPoint1->width.z;

	worldPoint[3].x = inPoint1->position.x - inPoint1->width.x;
	worldPoint[3].y = inPoint1->position.y - inPoint1->width.y;
	worldPoint[3].z = inPoint1->position.z - inPoint1->width.z;

	// transform these points into the global transformed object data arrays
	clipStatus = MSrTransform_PointListToFrustumScreen(4, worldPoint, frustumPoint, screenPoint, clipCode);

	if (clipStatus == MScClipStatus_TrivialReject)
		return UUcError_None;

	contrailTextureCoords[0].u = 0.f;
	contrailTextureCoords[0].v = inV0;
	contrailTextureCoords[1].u = 1.f;
	contrailTextureCoords[1].v = inV0;
	contrailTextureCoords[2].u = 0.f;
	contrailTextureCoords[2].v = inV1;
	contrailTextureCoords[3].u = 1.f;
	contrailTextureCoords[3].v = inV1;

	M3rDraw_State_Push();

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		screenPoint);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_BaseTextureMap,
		inTextureMap);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_TextureCoordArray,
		contrailTextureCoords);
	MSgGeomContextPrivate->objectVertexData.textureCoords = contrailTextureCoords;

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		4);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_VertexBitVector,
		NULL);

	if (inPoint0->tint == inPoint1->tint) {
		M3rDraw_State_SetInt(
			M3cDrawStateIntType_Interpolation,
			M3cDrawState_Interpolation_None);

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_ConstantColor,
			inPoint0->tint);

	} else {
		contrailShades[0] = inPoint0->tint;
		contrailShades[1] = inPoint0->tint;
		contrailShades[2] = inPoint1->tint;
		contrailShades[3] = inPoint1->tint;

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_Interpolation,
			M3cDrawState_Interpolation_Vertex);

		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_ScreenShadeArray_DC,
			contrailShades);
	}

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Alpha,
		inPoint0->alpha);

	M3rDraw_State_Commit();

	if (clipStatus == MScClipStatus_NeedsClipping) {
		MSgGeomContextPrivate->objectVertexData.newClipVertexIndex = 4;
		MSgGeomContextPrivate->objectVertexData.maxClipTextureCords = 4 + M3cExtraCoords;
		MSgGeomContextPrivate->objectVertexData.maxClipVertices = 4 + M3cExtraCoords;

		MSgGeomContextPrivate->polyComputeVertexProc = MSrClip_ComputeVertex_TextureFlat;

		MSrClip_Quad(0, 1, 3, 2, clipCode[0], clipCode[1], clipCode[3], clipCode[2], 0);
	} else {
		quadIndices.indices[0] = 0;
		quadIndices.indices[1] = 1;
		quadIndices.indices[2] = 3;
		quadIndices.indices[3] = 2;
		M3rDraw_Quad(&quadIndices);
	}

	// we allocated contrailTextureCoords off the stack, so after our exit from here
	// it will be invalid. nothing should be using MSgGeomContextPrivate->objectVertexData.textureCoords
	// anyway, but make absolutely sure that they won't
	MSgGeomContextPrivate->objectVertexData.textureCoords = NULL;

	M3rDraw_State_Pop();

	return UUcError_None;
}

UUtBool
MSrGeomContext_Method_PointTestVisible(
		M3tPoint3D*			inPoint,
		float				inTolerance)
{
	if (0 && M3rDraw_SupportPointVisible()) {
		// transformation buffers
		static UUtUns8 screenBuf[sizeof(M3tPointScreen) + (2 * UUcProcessor_CacheLineSize)];
		static UUtUns8 frustumBuf[sizeof(M3tPoint4D) + (2 * UUcProcessor_CacheLineSize)];
		static UUtUns8 worldBuf[sizeof(M3tPoint3D) + (2 * UUcProcessor_CacheLineSize)];
		static M3tPointScreen *screenPoint = NULL;
		static M3tPoint4D *frustumPoint = NULL;
		static M3tPoint3D *worldPoint = NULL;

		MStClipStatus clip_status;
		UUtUns8 clip_code;

		if (screenPoint == NULL) {
			// set up the memory-aligned pointers
			screenPoint		= UUrAlignMemory(screenBuf);
			frustumPoint	= UUrAlignMemory(frustumBuf);
			worldPoint		= UUrAlignMemory(worldBuf);
		}

		UUmAssertReadPtr(inPoint, sizeof(*inPoint));
		*worldPoint = *inPoint;

		// move the sample point towards the camera by a world-distance equal to tolerance
		MUmVector_ScaleIncrement(*worldPoint, -inTolerance, MSgGeomContextPrivate->activeCamera->viewVector);

		MSrTransform_UpdateMatrices();
		clip_status = MSrTransform_PointListToFrustumScreen(1, worldPoint, frustumPoint, screenPoint, &clip_code);

		if (clip_status != MScClipStatus_TrivialAccept)
			return UUcFalse;

		return M3rDraw_PointVisible(screenPoint, inTolerance);
	} else {
		// we cannot directly test a point's visibility
		M3tVector3D vector_to_pt, camera_test_pt;
		float z_near_tolerance = UUmMax(MSgGeomContextPrivate->activeCamera->zNear, 5.0f);

		MUmVector_Copy(camera_test_pt, MSgGeomContextPrivate->activeCamera->cameraLocation);
		MUmVector_ScaleIncrement(camera_test_pt, z_near_tolerance, MSgGeomContextPrivate->activeCamera->viewVector);

		MUmVector_Subtract(vector_to_pt, *inPoint, camera_test_pt);
		MUmVector_ScaleIncrement(vector_to_pt, -inTolerance, MSgGeomContextPrivate->activeCamera->viewVector);

		if (AKrCollision_Point(MSgGeomContextPrivate->environment, &camera_test_pt, &vector_to_pt, AKcGQ_Flag_LOS_CanSee_Skip_Player, 0)) {
			// our line of sight is obstructed by the environment
			return UUcFalse;

		} else if (AMrRayToCharacter((UUtUns16) -1, &camera_test_pt, &vector_to_pt, UUcTrue, NULL, NULL, NULL)) {
			// our line of sight is obstructed by a character
			return UUcFalse;

		} else {
			float test_distance;
			M3tVector3D unit_vector;

			test_distance = MUmVector_GetLength(vector_to_pt);
			if (test_distance < 1e-03f) {
				// the points are coincident, we can see clearly
				return UUcTrue;
			} else {
				MUmVector_ScaleCopy(unit_vector, 1.0f / test_distance, vector_to_pt);

				if (AMrRayToObject(&camera_test_pt, &unit_vector, test_distance, NULL, NULL)) {
					// our line of sight is obstructed by an animating object
					return UUcFalse;

				} else {
					return UUcTrue;
				}
			}
		}
	}
}


float
MSrGeomContext_Method_PointTestVisibleScale(
		M3tPoint3D*			inPoint,
		M3tPoint2D*			inTestOffsets,
		UUtUns32			inTestOffsetsCount )
{
	// transformation buffers
	static UUtUns8 screenBuf[sizeof(M3tPointScreen) + (2 * UUcProcessor_CacheLineSize)];
	static UUtUns8 frustumBuf[sizeof(M3tPoint4D) + (2 * UUcProcessor_CacheLineSize)];
	static UUtUns8 worldBuf[sizeof(M3tPoint3D) + (2 * UUcProcessor_CacheLineSize)];
	static M3tPointScreen *screenPoint = NULL;
	static M3tPoint4D *frustumPoint = NULL;
	static M3tPoint3D *worldPoint = NULL;
	UUtUns32				visible_count;
	MStClipStatus			clip_status;
	UUtUns8					clip_code;
	UUtUns32				i;
	UUtUns16				drawWidth		= M3rDraw_GetWidth();
	UUtUns16				drawHeight		= M3rDraw_GetHeight();

	UUmAssert( inTestOffsetsCount && inPoint && inTestOffsets );

	if (screenPoint == NULL)
	{
		// set up the memory-aligned pointers
		screenPoint		= UUrAlignMemory(screenBuf);
		frustumPoint	= UUrAlignMemory(frustumBuf);
		worldPoint		= UUrAlignMemory(worldBuf);
	}

	UUmAssertReadPtr(inPoint, sizeof(*inPoint));
	*worldPoint = *inPoint;

	// move the sample point towards the camera by a world-distance equal to tolerance
	MUmVector_ScaleIncrement(*worldPoint, 0, MSgGeomContextPrivate->activeCamera->viewVector);

	MSrTransform_UpdateMatrices();

	visible_count = 0;

	for( i = 0; i < inTestOffsetsCount; i++ )
	{
		clip_status		= MSrTransform_PointListToFrustumScreen(1, worldPoint, frustumPoint, screenPoint, &clip_code);
		screenPoint->x	+= inTestOffsets[i].x;
		screenPoint->y	+= inTestOffsets[i].z;
		if( screenPoint->y < 0 || screenPoint->y >= drawHeight || screenPoint->x < 0 || screenPoint->x >= drawWidth )
			continue;
		visible_count += M3rDraw_PointVisible(screenPoint, 0);
	}
	return (float) visible_count / (float) inTestOffsetsCount;
}

#define		MScMinUV		(0.0f)
#define		MScMaxUV		(1.0f)

UUtError MSrGeomContext_Method_Skybox_Create(
			M3tSkyboxData		*inSkybox,
			M3tTextureMap**		inTextures )
{
	M3tSkyboxData			*sky;
	UUtInt32				i;
	float					sky_distance = 5.f;

	UUmAssert( inSkybox );

	if( !inSkybox )
		return UUcError_None;

	sky = inSkybox;

	for( i = 0; i < 5; i++ )
	{
		sky->textures[i] = inTextures[i];
	}

	sky->cube_points[0].x	= -sky_distance;
	sky->cube_points[0].y	=  sky_distance;
	sky->cube_points[0].z	= -sky_distance;

	sky->cube_points[1].x	=  sky_distance;
	sky->cube_points[1].y	=  sky_distance;
	sky->cube_points[1].z	= -sky_distance;

	sky->cube_points[2].x	=  sky_distance;
	sky->cube_points[2].y	=  sky_distance;
	sky->cube_points[2].z	=  sky_distance;

	sky->cube_points[3].x	= -sky_distance;
	sky->cube_points[3].y	=  sky_distance;
	sky->cube_points[3].z	=  sky_distance;

	sky->cube_points[4].x	= -sky_distance;
	sky->cube_points[4].y	= -sky_distance;
	sky->cube_points[4].z	= -sky_distance;

	sky->cube_points[5].x	=  sky_distance;
	sky->cube_points[5].y	= -sky_distance;
	sky->cube_points[5].z	= -sky_distance;

	sky->cube_points[6].x	=  sky_distance;
	sky->cube_points[6].y	= -sky_distance;
	sky->cube_points[6].z	=  sky_distance;

	sky->cube_points[7].x	= -sky_distance;
	sky->cube_points[7].y	= -sky_distance;
	sky->cube_points[7].z	=  sky_distance;

	sky->indices[M3cSkyTop][0]			= 3;
	sky->indices[M3cSkyTop][1]			= 2;
	sky->indices[M3cSkyTop][2]			= 1;
	sky->indices[M3cSkyTop][3]			= 0;

	sky->indices[M3cSkyLeft][0]		= 3;
	sky->indices[M3cSkyLeft][1]		= 0;
	sky->indices[M3cSkyLeft][2]		= 4;
	sky->indices[M3cSkyLeft][3]		= 7;

	sky->indices[M3cSkyRight][0]		= 1;
	sky->indices[M3cSkyRight][1]		= 2;
	sky->indices[M3cSkyRight][2]		= 6;
	sky->indices[M3cSkyRight][3]		= 5;

	sky->indices[M3cSkyFront][0]		= 0;
	sky->indices[M3cSkyFront][1]		= 1;
	sky->indices[M3cSkyFront][2]		= 5;
	sky->indices[M3cSkyFront][3]		= 4;

	sky->indices[M3cSkyBack][0]		= 2;
	sky->indices[M3cSkyBack][1]		= 3;
	sky->indices[M3cSkyBack][2]		= 7;
	sky->indices[M3cSkyBack][3]		= 6;

	sky->texture_coords[0].u			= MScMinUV;
	sky->texture_coords[0].v			= MScMinUV;
	sky->texture_coords[1].u			= MScMaxUV;
	sky->texture_coords[1].v			= MScMinUV;
	sky->texture_coords[2].u			= MScMaxUV;
	sky->texture_coords[2].v			= MScMaxUV;
	sky->texture_coords[3].u			= MScMinUV;
	sky->texture_coords[3].v			= MScMaxUV;

	return UUcError_None;
}

UUtError MSrGeomContext_Method_Skybox_Destroy(
			M3tSkyboxData		*inSkybox )
{
	return UUcError_None;
}

UUtError MSrGeomContext_Method_Skybox_Draw( M3tSkyboxData		*inSkybox )
{
	static M3tQuad			quad	= {{0,1,2,3}};
	UUtUns8					point_buf[sizeof(M3tPoint3D) * M3cMaxSkyboxVerts + (2 * UUcProcessor_CacheLineSize)];
	M3tPoint3D				*aligned_points;
	M3tPointScreen			*screen_points;
	M3tPoint4D				*frustum_points;
	UUtUns8					*clip_codes;
	MStClipStatus			clip_status;
	UUtUns32				i;
	M3tSkyboxData			*sky;

	UUmAssert( inSkybox );

	if( !inSkybox )
		return UUcError_None;

	sky = inSkybox;

	// setup matricies and camera
	MSrTransform_UpdateMatrices();

	// grab buffers
	aligned_points					= UUrAlignMemory(point_buf);
	clip_codes						= MSgGeomContextPrivate->objectVertexData.clipCodes;
	frustum_points					= MSgGeomContextPrivate->objectVertexData.frustumPoints;
	screen_points					= MSgGeomContextPrivate->objectVertexData.screenPoints;

	M3rDraw_State_Push();
	{
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_VertexBitVector,	NULL);
		M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha,				M3cMaxAlpha);
		M3rDraw_State_SetInt(M3cDrawStateIntType_Interpolation,		M3cDrawState_Interpolation_None);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor,		0xffffffff);
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_ScreenPointArray,	screen_points );
		M3rDraw_State_SetInt(M3cDrawStateIntType_NumRealVertices,	4);
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_TextureCoordArray,	sky->texture_coords );
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_VertexBitVector,	NULL);
		MSgGeomContextPrivate->objectVertexData.textureCoords		= sky->texture_coords;
		M3rDraw_State_Commit();
		M3rDraw_Commit_Alpha();

		for( i = 0; i < 5; i++ )
		{
			if( sky->textures[i] )
			{
				M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, sky->textures[i]);
				M3rDraw_State_Commit();
				M3gDraw_Sorting			= UUcFalse;

				aligned_points[0]		= sky->cube_points[sky->indices[i][0]];
				aligned_points[1]		= sky->cube_points[sky->indices[i][1]];
				aligned_points[2]		= sky->cube_points[sky->indices[i][2]];
				aligned_points[3]		= sky->cube_points[sky->indices[i][3]];

				clip_status = MSrTransform_PointListToFrustumScreen( 4, aligned_points, frustum_points, screen_points, clip_codes );

				if( clip_status == MScClipStatus_TrivialReject )		continue;
				else if (clip_status == MScClipStatus_NeedsClipping)
				{
					MSgGeomContextPrivate->objectVertexData.newClipVertexIndex		= 4;
					MSgGeomContextPrivate->objectVertexData.maxClipTextureCords		= M3cMaxSkyboxVerts;
					MSgGeomContextPrivate->objectVertexData.maxClipVertices			= M3cMaxSkyboxVerts;
					MSgGeomContextPrivate->polyComputeVertexProc					= MSrClip_ComputeVertex_TextureFlat;
					MSrClip_Quad(0, 1, 2, 3, clip_codes[0], clip_codes[1], clip_codes[2], clip_codes[3], 0);
				}
				else
				{
					M3rDraw_Quad(&quad);
				}
			}
		}
	}
	M3rDraw_State_Pop();
	return UUcError_None;
}


UUtError
MSrGeomContext_Method_Decal_Draw(
	M3tDecalHeader*		inDecal,
	UUtUns16			inAlpha)
{
	M3tTextureCoord			*coords;
	M3tPointScreen			*screen_points;
	M3tPoint4D				*frustum_points;

	M3tPointScreen			*orig_screen_points;
	M3tPoint4D				*orig_frustum_points;
	M3tTextureCoord			*orig_texture_coords;

	UUtUns8					*decal_ptr;
	M3tPoint3D				*points;
	UUtUns16				*indices;
	M3tPoint3D				*aligned_points;
	UUtUns8					*clip_codes;
	MStClipStatus			clip_status;

	UUtUns32				i;
	UUtUns32				tri_count, vert_count, index_count;
	M3tTextureCoord			texture_coords[100];
	M3tTriSplit				tri;
	UUtUns8					point_buf[sizeof(M3tPoint3D) * M3cMaxSkyboxVerts + UUcProcessor_CacheLineSize];
	UUtUns32				*shades;
	UUtUns32				alpha;

	MSrTransform_UpdateMatrices();

	UUmAssertReadPtr(MSgGeomContextPrivate, sizeof(MSgGeomContextPrivate));

	aligned_points			= UUrAlignMemory(point_buf);
	clip_codes				= MSgGeomContextPrivate->objectVertexData.clipCodes;
	frustum_points			= MSgGeomContextPrivate->objectVertexData.frustumPoints;
	screen_points			= MSgGeomContextPrivate->objectVertexData.screenPoints;

//	if(MSgGeomContextPrivate->environment->lightMapArray != NULL)
//		lightMapArray		= MSgGeomContextPrivate->environment->lightMapArray->maps;
//	else
//		lightMapArray		= NULL;

	// number of indices must be four-byte aligned
	vert_count = inDecal->vertex_count;
	tri_count = inDecal->triangle_count;
	index_count = 3 * tri_count;
	if (index_count & 0x03) {
		index_count &= ~(0x03);
		index_count += 4;
	}

	// compute offsets into the decal buffer
	decal_ptr	= (UUtUns8 *)			inDecal;			decal_ptr += sizeof(M3tDecalHeader);
	points		= (M3tPoint3D *)		decal_ptr;			decal_ptr += vert_count * sizeof(M3tVector3D);
	coords		= (M3tTextureCoord*)	decal_ptr;			decal_ptr += vert_count * sizeof(M3tTextureCoord);
	indices		= (UUtUns16 *)			decal_ptr;			decal_ptr += index_count * sizeof(UUtUns16);
	shades		= (UUtUns32 *)			decal_ptr;			decal_ptr += vert_count * sizeof(UUtUns32);

	alpha = (inDecal->alpha * inAlpha * 2) >> 8;
	alpha = UUmMin(alpha, M3cMaxAlpha);

	M3rDraw_State_Push();
	{
		M3rDraw_State_SetInt(M3cDrawStateIntType_VertexFormat,		M3cDrawStateVertex_Split );
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_VertexBitVector,	NULL);
		M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha,				alpha);
		M3rDraw_State_SetInt(M3cDrawStateIntType_Interpolation,		M3cDrawState_Interpolation_None);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor,		inDecal->tint);
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_ScreenPointArray,	screen_points );
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_VertexBitVector,	NULL);
		M3rDraw_State_SetInt(M3cDrawStateIntType_NumRealVertices,	vert_count);
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap,	inDecal->texture);
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_TextureCoordArray,	texture_coords);
		MSgGeomContextPrivate->objectVertexData.textureCoords		= texture_coords;
		MSgGeomContextPrivate->objectVertexData.newClipVertexIndex	= 4;

		orig_screen_points											= MSgGeomContextPrivate->gqVertexData.screenPoints;
		orig_frustum_points											= MSgGeomContextPrivate->gqVertexData.frustumPoints;
		orig_texture_coords											= MSgGeomContextPrivate->gqVertexData.textureCoords;

		MSgGeomContextPrivate->gqVertexData.frustumPoints			= frustum_points;
		MSgGeomContextPrivate->gqVertexData.screenPoints			= screen_points;
		MSgGeomContextPrivate->gqVertexData.textureCoords			= texture_coords;

		M3rDraw_State_Commit();

		for( i = 0; i < tri_count; i++ )
		{
			UUtUns32		index0	= indices[i*3+0];
			UUtUns32		index1	= indices[i*3+1];
			UUtUns32		index2	= indices[i*3+2];

			aligned_points[0]	= points[index0];
			aligned_points[1]	= points[index1];
			aligned_points[2]	= points[index2];

			texture_coords[0]	= coords[index0];
			texture_coords[1]	= coords[index1];
			texture_coords[2]	= coords[index2];

			clip_status			= MSrTransform_PointListToFrustumScreen( 3, aligned_points, frustum_points, screen_points, clip_codes );

			if( clip_status == MScClipStatus_TrivialReject ) continue;
			if( clip_codes[0] || clip_codes[1] || clip_codes[2] )
			{
				MSgGeomContextPrivate->gqVertexData.newClipTextureIndex = 4;
				MSgGeomContextPrivate->gqVertexData.newClipVertexIndex = 4;

				tri.vertexIndices.indices[0]	= 0;
				tri.vertexIndices.indices[1]	= 1;
				tri.vertexIndices.indices[2]	= 2;
				tri.baseUVIndices.indices[0]	= 0;
				tri.baseUVIndices.indices[1]	= 1;
				tri.baseUVIndices.indices[2]	= 2;
				tri.shades[0]					= shades[index0];
				tri.shades[1]					= shades[index1];
				tri.shades[2]					= shades[index2];

				MSrEnv_Clip_Tri( &tri, clip_codes[0], clip_codes[1], clip_codes[2], 0 );
				//MSrClip_Triangle( 0, 1, 2, clip_codes[0], clip_codes[1], clip_codes[2], 0);
			}
			else
			{
				tri.vertexIndices.indices[0]	= 0;
				tri.vertexIndices.indices[1]	= 1;
				tri.vertexIndices.indices[2]	= 2;
				tri.baseUVIndices.indices[0]	= 0;
				tri.baseUVIndices.indices[1]	= 1;
				tri.baseUVIndices.indices[2]	= 2;
				tri.shades[0]					= shades[index0];
				tri.shades[1]					= shades[index1];
				tri.shades[2]					= shades[index2];


				M3rDraw_Triangle( &tri );
			}
		}
	}

	M3rDraw_State_Pop();

	MSgGeomContextPrivate->gqVertexData.frustumPoints			= orig_frustum_points;
	MSgGeomContextPrivate->gqVertexData.screenPoints			= orig_screen_points;
	MSgGeomContextPrivate->gqVertexData.textureCoords			= orig_texture_coords;

	return UUcError_None;
}
