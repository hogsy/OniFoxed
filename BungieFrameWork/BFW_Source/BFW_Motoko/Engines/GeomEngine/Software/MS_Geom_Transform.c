/*
	FILE:	MS_Geom_Transform.c

	AUTHOR:	Brent H. Pease

	CREATED: May 21, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_BitVector.h"

#include "BFW_Shared_Math.h"

#include "MS_GC_Private.h"
#include "MS_Geom_Transform.h"
#include "MS_GC_Method_Camera.h"

MStClipStatus
MSrTransform_PointListToFrustumScreen(
	UUtUns32			inNumVertices,
	const M3tPoint3D*	inPointList,
	M3tPoint4D			*outFrustumPoints,
	M3tPointScreen		*outScreenPoints,
	UUtUns8				*outClipCodeList)
{
	MStClipStatus result;
	UUtInt16	i;
	float		m00, m10, m20, m30;
	float		m01, m11, m21, m31;
	float		m02, m12, m22, m32;
	float		m03, m13, m23, m33;

	float		iX, iY, iZ;
	float		hX, hY, hZ, hW;
	float		invW, negW;
	float		scaleX, scaleY;

	const M3tPoint3D*		curLocalPoint = inPointList;
	M3tPointScreen*	curScreenPoint = outScreenPoints;
	M3tPoint4D*		curFrustumPoint = outFrustumPoints;

	UUtUns8*		curClipCodePtr = outClipCodeList;
	UUtUns8			curClipCodeValue;

	UUtUns8			clipCodeOR = 0;
	UUtUns8			clipCodeAND = (UUtUns8) ~0;
	UUtBool			needs4DClipping = UUcFalse;

	UUmAssert(((unsigned long)inPointList & UUcProcessor_CacheLineSize_Mask) == 0);
	UUmAssert(((unsigned long)outScreenPoints & UUcProcessor_CacheLineSize_Mask) == 0);

	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;

	MSmTransform_Matrix4x4ToRegisters(
		MSgGeomContextPrivate->matrix_localToFrustum,
		m00, m10, m20, m30,
		m01, m11, m21, m31,
		m02, m12, m22, m32,
		m03, m13, m23, m33);

	for(i = (UUtInt16) ((inNumVertices + 1) >> 1); i-- > 0;)
	{
		UUrProcessor_ZeroCacheLine(curScreenPoint, 0);
		UUrProcessor_ZeroCacheLine(curFrustumPoint, 0);

		iX = curLocalPoint->x;
		iY = curLocalPoint->y;
		iZ = curLocalPoint->z;

		MSmTransform_Local2FrustumInvW(
			iX, iY, iZ,
			hX, hY, hZ, hW, invW,
			m00, m10, m20, m30,
			m01, m11, m21, m31,
			m02, m12, m22, m32,
			m03, m13, m23, m33);

		curFrustumPoint->x = hX;
		curFrustumPoint->y = hY;
		curFrustumPoint->z = hZ;
		curFrustumPoint->w = hW;
		negW = -hW;

		MSiVerifyPoint4D(curFrustumPoint);

		MSmTransform_FrustumClipTest(hX, hY, hZ, hW, negW, curClipCodeValue, needs4DClipping);

		// XXX - Might need another case where we don't do this for out of bounds verts
		MSmTransform_Frustum2Screen(
			curScreenPoint->x,
			curScreenPoint->y,
			curScreenPoint->z,
			curScreenPoint->invW,
			hX, hY, hZ, invW, scaleX, scaleY);

		clipCodeOR |= curClipCodeValue;
		clipCodeAND &= curClipCodeValue;

		*curClipCodePtr = curClipCodeValue;

		if(i == 0 && (inNumVertices & 0x01))
		{
			break;
		}

		curLocalPoint++;
		curScreenPoint++;
		curClipCodePtr++;
		curFrustumPoint++;

		iX = curLocalPoint->x;
		iY = curLocalPoint->y;
		iZ = curLocalPoint->z;

		MSmTransform_Local2FrustumInvW(
			iX, iY, iZ,
			hX, hY, hZ, hW, invW,
			m00, m10, m20, m30,
			m01, m11, m21, m31,
			m02, m12, m22, m32,
			m03, m13, m23, m33);

		curFrustumPoint->x = hX;
		curFrustumPoint->y = hY;
		curFrustumPoint->z = hZ;
		curFrustumPoint->w = hW;
		negW = -hW;

		MSiVerifyPoint4D(curFrustumPoint);

		MSmTransform_FrustumClipTest(hX, hY, hZ, hW, negW, curClipCodeValue, needs4DClipping);

		// XXX - Might need another case where we don't do this for out of bounds verts
		MSmTransform_Frustum2Screen(
			curScreenPoint->x,
			curScreenPoint->y,
			curScreenPoint->z,
			curScreenPoint->invW,
			hX, hY, hZ, invW, scaleX, scaleY);

		clipCodeOR |= curClipCodeValue;
		clipCodeAND &= curClipCodeValue;

		*curClipCodePtr = curClipCodeValue;

		curLocalPoint++;
		curScreenPoint++;
		curClipCodePtr++;
		curFrustumPoint++;
	}

	if (clipCodeAND != 0)
	{
		result = MScClipStatus_TrivialReject;
	}
	else if (clipCodeOR != 0)
	{
		result = MScClipStatus_NeedsClipping;
	}
	else
	{
		result = MScClipStatus_TrivialAccept;
	}

	return result;
}

void
MSrTransform_UpdateMatrices(
	void)
{
	M3tMatrix4x4	localToView;

	M3rCamera_GetActive((M3tGeomCamera**)&MSgGeomContextPrivate->activeCamera);

	M3rManager_Camera_UpdateMatrices(MSgGeomContextPrivate->activeCamera);

	M3rMatrixStack_Get(&MSgGeomContextPrivate->matrix_localToWorld);

	MUrMath_Matrix4x4Multiply4x3(
		&MSgGeomContextPrivate->activeCamera->matrix_worldToView,
		MSgGeomContextPrivate->matrix_localToWorld,
		&localToView);

	MUrMath_Matrix4x4Multiply(
		&MSgGeomContextPrivate->activeCamera->matrix_viewToFrustum,
		&localToView,
		&MSgGeomContextPrivate->matrix_localToFrustum);
}

void
MSrTransform_Geom_PointListToScreen(
	M3tGeometry*			inGeometry,
	M3tPointScreen			*outResultScreenPoints)
{
	UUtUns16	i;
	float		m00, m10, m20, m30;
	float		m01, m11, m21, m31;
	float		m02, m12, m22, m32;
	float		m03, m13, m23, m33;

	float		iX, iY, iZ;
	float		hX, hY, hZ, hW;
	float		invW;
	float		scaleX, scaleY;

	M3tPoint3D*		curLocalPoint = inGeometry->pointArray->points;
	M3tPointScreen*	curScreenPoint = outResultScreenPoints;

	UUtUns32	numPoints = inGeometry->pointArray->numPoints;

	UUmAssert(inGeometry->pointArray->numPoints < M3cMaxObjVertices);
	UUmAssertReadPtr(inGeometry, sizeof(inGeometry));
	UUmAssertReadPtr(outResultScreenPoints, sizeof(outResultScreenPoints));

	UUmAssert(((unsigned long)curLocalPoint & UUcProcessor_CacheLineSize_Mask) == 0);
	UUmAssert(((unsigned long)outResultScreenPoints & UUcProcessor_CacheLineSize_Mask) == 0);

	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;

	MSmTransform_Matrix4x4ToRegisters(
		MSgGeomContextPrivate->matrix_localToFrustum,
		m00, m10, m20, m30,
		m01, m11, m21, m31,
		m02, m12, m22, m32,
		m03, m13, m23, m33);

	for(i = (UUtUns16) ((numPoints + 1) >> 1); i-- > 0;)
	{
		UUrProcessor_ZeroCacheLine(curScreenPoint, 0);

		iX = curLocalPoint->x;
		iY = curLocalPoint->y;
		iZ = curLocalPoint->z;

		MSmTransform_Local2FrustumInvW(
			iX, iY, iZ,
			hX, hY, hZ, hW, invW,
			m00, m10, m20, m30,
			m01, m11, m21, m31,
			m02, m12, m22, m32,
			m03, m13, m23, m33);

		MSmTransform_Frustum2Screen(
			curScreenPoint->x,
			curScreenPoint->y,
			curScreenPoint->z,
			curScreenPoint->invW,
			hX, hY, hZ, invW, scaleX, scaleY);

		MSiVerifyPointScreen(curScreenPoint);

		if(i == 0 && (numPoints & 0x01))
		{
			break;
		}

		curLocalPoint++;
		curScreenPoint++;

		iX = curLocalPoint->x;
		iY = curLocalPoint->y;
		iZ = curLocalPoint->z;

		MSmTransform_Local2FrustumInvW(
			iX, iY, iZ,
			hX, hY, hZ, hW, invW,
			m00, m10, m20, m30,
			m01, m11, m21, m31,
			m02, m12, m22, m32,
			m03, m13, m23, m33);

		MSmTransform_Frustum2Screen(
			curScreenPoint->x,
			curScreenPoint->y,
			curScreenPoint->z,
			curScreenPoint->invW,
			hX, hY, hZ, invW, scaleX, scaleY);

		MSiVerifyPointScreen(curScreenPoint);

		curLocalPoint++;
		curScreenPoint++;
	}
}


void
MSrTransform_Geom_PointListToScreen_ActiveVertices(
	M3tGeometry*			inGeometry,
	UUtUns32*				inActiveVerticesBV,
	M3tPointScreen			*outResultScreenPoints)
{
	UUtUns32	i;
	float		m00, m10, m20, m30;
	float		m01, m11, m21, m31;
	float		m02, m12, m22, m32;
	float		m03, m13, m23, m33;

	float		iX, iY, iZ;
	float		hX, hY, hZ, hW;
	float		invW;
	float		scaleX, scaleY;

	M3tPoint3D*		curLocalPoint = inGeometry->pointArray->points;
	M3tPointScreen*	curScreenPoint = outResultScreenPoints;

	UUtUns32		numPoints = inGeometry->pointArray->numPoints;

	UUmAssert(inGeometry->pointArray->numPoints < M3cMaxObjVertices);
	UUmAssertReadPtr(inGeometry, sizeof(inGeometry));
	UUmAssertReadPtr(outResultScreenPoints, sizeof(outResultScreenPoints));
	UUmAssertReadPtr(inActiveVerticesBV, sizeof(inActiveVerticesBV));

	UUmAssert(((unsigned long)curLocalPoint & UUcProcessor_CacheLineSize_Mask) == 0);
	UUmAssert(((unsigned long)curScreenPoint & UUcProcessor_CacheLineSize_Mask) == 0);

	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;

	MSmTransform_Matrix4x4ToRegisters(
		MSgGeomContextPrivate->matrix_localToFrustum,
		m00, m10, m20, m30,
		m01, m11, m21, m31,
		m02, m12, m22, m32,
		m03, m13, m23, m33);

	UUmBitVector_Loop_Begin(i, numPoints, inActiveVerticesBV)
	{
		if(!(i & 1))
		{
			UUrProcessor_ZeroCacheLine(curScreenPoint, 0);
		}

		UUmBitVector_Loop_Test
		{
			iX = curLocalPoint->x;
			iY = curLocalPoint->y;
			iZ = curLocalPoint->z;

			MSmTransform_Local2FrustumInvW(
				iX, iY, iZ,
				hX, hY, hZ, hW, invW,
				m00, m10, m20, m30,
				m01, m11, m21, m31,
				m02, m12, m22, m32,
				m03, m13, m23, m33);

			MSmTransform_Frustum2Screen(
				curScreenPoint->x,
				curScreenPoint->y,
				curScreenPoint->z,
				curScreenPoint->invW,
				hX, hY, hZ, invW, scaleX, scaleY);

			MSiVerifyPointScreen(curScreenPoint);
		}

		curLocalPoint++;
		curScreenPoint++;
	}
	UUmBitVector_Loop_End
}


void
MSrTransform_Geom_PointListToFrustumScreen(
	M3tGeometry*			inGeometry,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList)
{
	MSrTransform_PointListToFrustumScreen(
		inGeometry->pointArray->numPoints,
		inGeometry->pointArray->points,
		outFrustumPoints,
		outScreenPoints,
		outClipCodeList);
}


void
MSrTransform_Geom_PointListLocalToFrustumScreen_ActiveVertices(
	M3tGeometry*			inGeometry,
	const UUtUns32*			inActiveVerticesBV,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList)
{
	UUtUns32	i;
	float		m00, m10, m20, m30;
	float		m01, m11, m21, m31;
	float		m02, m12, m22, m32;
	float		m03, m13, m23, m33;

	float		iX, iY, iZ;
	float		hX, hY, hZ, hW;
	float		invW, negW;
	float		scaleX, scaleY;

	const M3tPoint3D*	curLocalPoint = inGeometry->pointArray->points;
	M3tPointScreen*		curScreenPoint = outScreenPoints;
	M3tPoint4D*			curFrustumPoint = outFrustumPoints;

	UUtUns8*			curClipCodePtr = outClipCodeList;
	UUtUns8				curClipCodeValue;

	UUtBool				needs4DClipping = UUcFalse;

	UUtUns32			numPoints = inGeometry->pointArray->numPoints;

	UUmAssert(((unsigned long)curLocalPoint & UUcProcessor_CacheLineSize_Mask) == 0);
	UUmAssert(((unsigned long)curScreenPoint & UUcProcessor_CacheLineSize_Mask) == 0);

	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;

	MSmTransform_Matrix4x4ToRegisters(
		MSgGeomContextPrivate->matrix_localToFrustum,
		m00, m10, m20, m30,
		m01, m11, m21, m31,
		m02, m12, m22, m32,
		m03, m13, m23, m33);

	UUmBitVector_Loop_Begin(i, numPoints, inActiveVerticesBV)
	{
		if(!(i & 1))
		{
			UUrProcessor_ZeroCacheLine(curScreenPoint, 0);
			UUrProcessor_ZeroCacheLine(curFrustumPoint, 0);
		}

		UUmBitVector_Loop_Test
		{
			iX = curLocalPoint->x;
			iY = curLocalPoint->y;
			iZ = curLocalPoint->z;

			MSmTransform_Local2FrustumInvW(
				iX, iY, iZ,
				hX, hY, hZ, hW, invW,
				m00, m10, m20, m30,
				m01, m11, m21, m31,
				m02, m12, m22, m32,
				m03, m13, m23, m33);

			curFrustumPoint->x = hX;
			curFrustumPoint->y = hY;
			curFrustumPoint->z = hZ;
			curFrustumPoint->w = hW;
			negW = -hW;

			MSiVerifyPoint4D(curFrustumPoint);

			MSmTransform_FrustumClipTest(hX, hY, hZ, hW, negW, curClipCodeValue, needs4DClipping);

			MSmTransform_Frustum2Screen(
				curScreenPoint->x,
				curScreenPoint->y,
				curScreenPoint->z,
				curScreenPoint->invW,
				hX, hY, hZ, invW, scaleX, scaleY);

			*curClipCodePtr = curClipCodeValue;
		}

		curLocalPoint++;
		curScreenPoint++;
		curClipCodePtr++;
		curFrustumPoint++;
	}
	UUmBitVector_Loop_End


}


void
MSrTransform_Geom_PointListAndVertexNormalToWorld(
	M3tGeometry*			inGeometry,
	M3tPoint3D				*outResultWorldPoints,
	M3tVector3D				*outResultWorldVertexNormals)
{
	UUtInt32	i;
	UUtInt32	j;
	UUtUns32	block8;

	float		m00, m10, m20, m30;
	float		m01, m11, m21, m31;
	float		m02, m12, m22, m32;

	float		iX, iY, iZ;

	M3tPoint3D*		curPoint = inGeometry->pointArray->points;
	M3tPoint3D*		curWorldPoint = outResultWorldPoints;
	M3tVector3D*	curVertexNormal = inGeometry->vertexNormalArray->vectors;
	M3tVector3D*	curWorldVertexNormal = outResultWorldVertexNormals;

	M3tMatrix4x3*	matrix3;

	UUtUns32		numPoints = inGeometry->pointArray->numPoints;

	UUmAssert(((unsigned long)curPoint & UUcProcessor_CacheLineSize_Mask) == 0);
	UUmAssert(((unsigned long)curWorldPoint & UUcProcessor_CacheLineSize_Mask) == 0);
	UUmAssert(((unsigned long)curVertexNormal & UUcProcessor_CacheLineSize_Mask) == 0);
	UUmAssert(((unsigned long)curWorldVertexNormal & UUcProcessor_CacheLineSize_Mask) == 0);

	matrix3 = MSgGeomContextPrivate->matrix_localToWorld;
	UUmAssertReadPtr(matrix3, sizeof(M3tMatrix4x3));

	m00 = matrix3->m[0][0];
	m01 = matrix3->m[0][1];
	m02 = matrix3->m[0][2];

	m10 = matrix3->m[1][0];
	m11 = matrix3->m[1][1];
	m12 = matrix3->m[1][2];

	m20 = matrix3->m[2][0];
	m21 = matrix3->m[2][1];
	m22 = matrix3->m[2][2];

	m30 = matrix3->m[3][0];
	m31 = matrix3->m[3][1];
	m32 = matrix3->m[3][2];

	block8 = (numPoints + 7) >> 3;

	for(i = block8; i-- > 0;)
	{
		UUrProcessor_ZeroCacheLine((char*)curWorldPoint, 0);
		UUrProcessor_ZeroCacheLine((char*)curWorldPoint + UUcProcessor_CacheLineSize, 0);
		UUrProcessor_ZeroCacheLine((char*)curWorldPoint + UUcProcessor_CacheLineSize * 2, 0);
		UUrProcessor_ZeroCacheLine((char*)curWorldVertexNormal, 0);
		UUrProcessor_ZeroCacheLine((char*)curWorldVertexNormal + UUcProcessor_CacheLineSize, 0);
		UUrProcessor_ZeroCacheLine((char*)curWorldVertexNormal + UUcProcessor_CacheLineSize * 2, 0);

		for(j = 0; j < 8; j++)
		{
			iX = curPoint->x;
			iY = curPoint->y;
			iZ = curPoint->z;

			curWorldPoint->x = m00 * iX + m10 * iY + m20 * iZ + m30;
			curWorldPoint->y = m01 * iX + m11 * iY + m21 * iZ + m31;
			curWorldPoint->z = m02 * iX + m12 * iY + m22 * iZ + m32;

			curPoint++;
			curWorldPoint++;

			iX = curVertexNormal->x;
			iY = curVertexNormal->y;
			iZ = curVertexNormal->z;

			curWorldVertexNormal->x = m00 * iX + m10 * iY + m20 * iZ;
			curWorldVertexNormal->y = m01 * iX + m11 * iY + m21 * iZ;
			curWorldVertexNormal->z = m02 * iX + m12 * iY + m22 * iZ;

			curVertexNormal++;
			curWorldVertexNormal++;
		}
	}

	UUrProcessor_ZeroCacheLine(curWorldPoint, 0);
	UUrProcessor_ZeroCacheLine(curWorldVertexNormal, 0);

	for(i = numPoints - (block8 * 8); i-- > 0;)
	{
		iX = curPoint->x;
		iY = curPoint->y;
		iZ = curPoint->z;

		curWorldPoint->x = m00 * iX + m10 * iY + m20 * iZ + m30;
		curWorldPoint->y = m01 * iX + m11 * iY + m21 * iZ + m31;
		curWorldPoint->z = m02 * iX + m12 * iY + m22 * iZ + m32;

		curPoint++;
		curWorldPoint++;

		iX = curVertexNormal->x;
		iY = curVertexNormal->y;
		iZ = curVertexNormal->z;

		curWorldVertexNormal->x = m00 * iX + m10 * iY + m20 * iZ;
		curWorldVertexNormal->y = m01 * iX + m11 * iY + m21 * iZ;
		curWorldVertexNormal->z = m02 * iX + m12 * iY + m22 * iZ;

		curVertexNormal++;
		curWorldVertexNormal++;
	}
}

void
MSrTransform_Geom_PointListAndVertexNormalToWorldComputeViewVector(
	M3tGeometry*			inGeometry,
	M3tPoint3D				*outResultWorldPoints,
	M3tVector3D				*outResultViewVectors,
	M3tVector3D				*outResultWorldVertexNormals)
{
	MSrTransform_Geom_PointListAndVertexNormalToWorld(
		inGeometry,
		outResultWorldPoints,
		outResultWorldVertexNormals);
}

void
MSrTransform_Geom_FaceNormalToWorld(
	M3tGeometry*			inGeometry,
	M3tVector3D				*outResultWorldTriNormals)
{
	UUtInt32	i;
	UUtInt32	j;
	UUtUns32	block8;

	float		m00, m10, m20, m30;
	float		m01, m11, m21, m31;
	float		m02, m12, m22, m32;

	float		iX, iY, iZ;

	M3tVector3D		*curVertexNormal;
	M3tVector3D		*curWorldVertexNormal;

	M3tMatrix4x3*	matrix3;

	UUtUns32		numVectors;

	matrix3 = MSgGeomContextPrivate->matrix_localToWorld;
	UUmAssertReadPtr(matrix3, sizeof(M3tMatrix4x3));

	m00 = matrix3->m[0][0];
	m01 = matrix3->m[0][1];
	m02 = matrix3->m[0][2];

	m10 = matrix3->m[1][0];
	m11 = matrix3->m[1][1];
	m12 = matrix3->m[1][2];

	m20 = matrix3->m[2][0];
	m21 = matrix3->m[2][1];
	m22 = matrix3->m[2][2];

	m30 = matrix3->m[3][0];
	m31 = matrix3->m[3][1];
	m32 = matrix3->m[3][2];

	curVertexNormal = inGeometry->triNormalArray->vectors;
	curWorldVertexNormal = outResultWorldTriNormals;

	UUmAssert(((unsigned long)curVertexNormal & UUcProcessor_CacheLineSize_Mask) == 0);
	UUmAssert(((unsigned long)curWorldVertexNormal & UUcProcessor_CacheLineSize_Mask) == 0);

	numVectors = inGeometry->triNormalArray->numVectors;
	block8 = (numVectors + 7) >> 3;

	for(i = block8; i-- > 0;)
	{
		UUrProcessor_ZeroCacheLine((char*)curWorldVertexNormal, 0);
		UUrProcessor_ZeroCacheLine((char*)curWorldVertexNormal + UUcProcessor_CacheLineSize, 0);
		UUrProcessor_ZeroCacheLine((char*)curWorldVertexNormal + UUcProcessor_CacheLineSize * 2, 0);

		for(j = 0; j < 4; j++)
		{
			iX = curVertexNormal->x;
			iY = curVertexNormal->y;
			iZ = curVertexNormal->z;

			curWorldVertexNormal->x = m00 * iX + m10 * iY + m20 * iZ;
			curWorldVertexNormal->y = m01 * iX + m11 * iY + m21 * iZ;
			curWorldVertexNormal->z = m02 * iX + m12 * iY + m22 * iZ;

			curVertexNormal++;
			curWorldVertexNormal++;

			iX = curVertexNormal->x;
			iY = curVertexNormal->y;
			iZ = curVertexNormal->z;

			curWorldVertexNormal->x = m00 * iX + m10 * iY + m20 * iZ;
			curWorldVertexNormal->y = m01 * iX + m11 * iY + m21 * iZ;
			curWorldVertexNormal->z = m02 * iX + m12 * iY + m22 * iZ;

			curVertexNormal++;
			curWorldVertexNormal++;
		}
	}

	UUrProcessor_ZeroCacheLine(curWorldVertexNormal, 0);

	for(i = numVectors - (block8 * 8); i-- > 0;)
	{
		iX = curVertexNormal->x;
		iY = curVertexNormal->y;
		iZ = curVertexNormal->z;

		curWorldVertexNormal->x = m00 * iX + m10 * iY + m20 * iZ;
		curWorldVertexNormal->y = m01 * iX + m11 * iY + m21 * iZ;
		curWorldVertexNormal->z = m02 * iX + m12 * iY + m22 * iZ;

		curVertexNormal++;
		curWorldVertexNormal++;
	}
}

MStClipStatus
MSrTransform_Geom_BoundingBoxToFrustumScreen(
	M3tGeometry*			inGeometry,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList)
{

	M3rMinMaxBBox_To_BBox(&inGeometry->pointArray->minmax_boundingBox, (M3tBoundingBox*)MSgGeomContextPrivate->worldPoints);

	return MSrTransform_PointListToFrustumScreen(
			8,
			MSgGeomContextPrivate->worldPoints,
			outFrustumPoints,
			outScreenPoints,
			outClipCodeList);
}


MStClipStatus
MSrTransform_Geom_BoundingBoxClipStatus(
	M3tGeometry*			inGeometry)
{
	MStClipStatus result;
	UUtInt16	i;
	float		m00, m10, m20, m30;
	float		m01, m11, m21, m31;
	float		m02, m12, m22, m32;
	float		m03, m13, m23, m33;

	float		iX, iY, iZ;
	float		hX, hY, hZ, hW;
	float		negW;

	M3tPoint3D*		curLocalPoint;
	M3tBoundingBox	bbox;

	UUtUns16		curClipCodeValue;

	UUtUns16		clipCodeOR = 0;
	UUtUns16		clipCodeAND = (UUtUns16) ~0;
	UUtBool			needs4DClipping = UUcFalse;

	M3rMinMaxBBox_To_BBox(&inGeometry->pointArray->minmax_boundingBox, &bbox);
	curLocalPoint = bbox.localPoints;

	MSmTransform_Matrix4x4ToRegisters(
		MSgGeomContextPrivate->matrix_localToFrustum,
		m00, m10, m20, m30,
		m01, m11, m21, m31,
		m02, m12, m22, m32,
		m03, m13, m23, m33);

	for(i = 8; i-- > 0;)
	{
		iX = curLocalPoint->x;
		iY = curLocalPoint->y;
		iZ = curLocalPoint->z;

		MSmTransform_Local2FrustumNegW(
			iX, iY, iZ,
			hX, hY, hZ, hW, negW,
			m00, m10, m20, m30,
			m01, m11, m21, m31,
			m02, m12, m22, m32,
			m03, m13, m23, m33);

		MSmTransform_FrustumClipTest(hX, hY, hZ, hW, negW, curClipCodeValue, needs4DClipping);

		clipCodeOR |= curClipCodeValue;
		clipCodeAND &= curClipCodeValue;

		curLocalPoint++;
	}

	if (clipCodeAND != 0)
	{
		result = MScClipStatus_TrivialReject;
	}
	else if (clipCodeOR != 0)
	{
		result = MScClipStatus_NeedsClipping;
	}
	else
	{
		result = MScClipStatus_TrivialAccept;
	}

	return result;
}

void
MSrTransform_EnvPointListToFrustumScreen_ActiveVertices(
	const UUtUns32*			inActiveVerticesBV,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList)
{
	UUtUns32	i;
	float		m00, m10, m20, m30;
	float		m01, m11, m21, m31;
	float		m02, m12, m22, m32;
	float		m03, m13, m23, m33;

	float		iX, iY, iZ;
	float		hX, hY, hZ, hW;
	float		invW, negW;
	float		scaleX, scaleY;

	const M3tPoint3D*	worldPoints = MSgGeomContextPrivate->environment->pointArray->points;
	M3tPointScreen*		screenPoints = outScreenPoints;
	M3tPoint4D*			frustumPoints = outFrustumPoints;

	UUtUns8*			clipCodePtrs = outClipCodeList;
	UUtUns32			curClipCodeValue;

	UUtUns32			clipCodeOR = 0;
	UUtUns32			clipCodeAND = ~0;
	UUtBool				needs4DClipping = UUcFalse;

	UUtUns32			numPoints = MSgGeomContextPrivate->environment->pointArray->numPoints;

	scaleX = MSgGeomContextPrivate->scaleX;
	scaleY = MSgGeomContextPrivate->scaleY;

	MSmTransform_Matrix4x4ToRegisters(
		MSgGeomContextPrivate->matrix_localToFrustum,
		m00, m10, m20, m30,
		m01, m11, m21, m31,
		m02, m12, m22, m32,
		m03, m13, m23, m33);


	UUmBitVector_ForEachBitSetDo(i, inActiveVerticesBV, numPoints)
	{
		iX = worldPoints[i].x;
		iY = worldPoints[i].y;
		iZ = worldPoints[i].z;

		MSmTransform_Local2FrustumInvW(
			iX, iY, iZ,
			hX, hY, hZ, hW, invW,
			m00, m10, m20, m30,
			m01, m11, m21, m31,
			m02, m12, m22, m32,
			m03, m13, m23, m33);

		frustumPoints[i].x = hX;
		frustumPoints[i].y = hY;
		frustumPoints[i].z = hZ;
		frustumPoints[i].w = hW;
		negW = -hW;

		MSiVerifyPoint4D(frustumPoints + i);

		MSmTransform_FrustumClipTest(hX, hY, hZ, hW, negW, curClipCodeValue, needs4DClipping);

		MSmTransform_Frustum2Screen(
			screenPoints[i].x,
			screenPoints[i].y,
			screenPoints[i].z,
			screenPoints[i].invW,
			hX, hY, hZ, invW, scaleX, scaleY);

		clipCodePtrs[i] = (UUtUns8) curClipCodeValue;
	}
}

static const float M3gBackfaceRemoveThreshold = -0.1f;

void
MSrBackface_Remove(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	M3tVector3D*			inWorldViewVectors,
	M3tVector3D*			inWorldTriNormals,
	UUtUns32*				outActiveTrisBV,
	UUtUns32*				outActiveVerticesBV)
{
	M3tPoint3D*				curPoint;
	float					viewVectorX, viewVectorY, viewVectorZ;
	M3tVector3D*			curNormal;
	float					clX, clY, clZ;
	UUtUns32				i;
	UUtUns32				index0, index1, index2;
	UUtUns32				numTris;
	UUtUns32*				curIndexPtr;
	UUtUns32*				triNormalIndices;

	//clX = MSgGeomContextPrivate->visCamera->cameraLocation.x;
	//clY = MSgGeomContextPrivate->visCamera->cameraLocation.y;
	//clZ = MSgGeomContextPrivate->visCamera->cameraLocation.z;
	clX = MSgGeomContextPrivate->activeCamera->cameraLocation.x;
	clY = MSgGeomContextPrivate->activeCamera->cameraLocation.y;
	clZ = MSgGeomContextPrivate->activeCamera->cameraLocation.z;

	numTris = (UUtUns16)inGeometry->triNormalIndexArray->numIndices;
	triNormalIndices = inGeometry->triNormalIndexArray->indices;
	curIndexPtr = inGeometry->triStripArray->indices;

	for(i = 0; i < numTris; i++)
	{
		index2 = *curIndexPtr++;

		if(index2 & 0x80000000)
		{
			index0 = (index2 & 0x7FFFFFFF);
			index1 = *curIndexPtr++;
			index2 = *curIndexPtr++;
		}

		curPoint = inWorldPoints + index0;
		curNormal = inWorldTriNormals + triNormalIndices[i];

		viewVectorX = clX - curPoint->x;
		viewVectorY = clY - curPoint->y;
		viewVectorZ = clZ - curPoint->z;

		if(viewVectorX * curNormal->x +
			viewVectorY * curNormal->y +
			viewVectorZ * curNormal->z >= 0)
		{
			UUrBitVector_SetBit(
				outActiveTrisBV,
				i);

			UUrBitVector_SetBit(
				outActiveVerticesBV,
				index0);
			UUrBitVector_SetBit(
				outActiveVerticesBV,
				index1);
			UUrBitVector_SetBit(
				outActiveVerticesBV,
				index2);
		}

		index0 = index1;
		index1 = index2;

	}
}

