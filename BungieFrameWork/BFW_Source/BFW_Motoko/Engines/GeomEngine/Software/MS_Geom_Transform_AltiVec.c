/*
	FILE:	MS_Geom_Transform_AltiVec.c

	AUTHOR:	Brent H. Pease

	CREATED: May 21, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_BitVector.h"
#include "BFW_Platform_AltiVec.h"

#include "BFW_Shared_Math.h"

#include "MS_GC_Private.h"
#include "MS_GC_Method_Camera.h"

#include "Motoko_Manager.h"
#include "MS_Geom_Transform_AltiVec.h"

#if UUmSIMD == UUmSIMD_AltiVec

void
MSrTransform_Geom_PointListToScreen_AltiVec(
	M3tGeometry*			inGeometry,
	M3tPointScreen			*outResultScreenPoints)
{
	M3tGeometry_Private*	geomPrivate = (M3tGeometry_Private*)TMrTemplate_PrivateData_GetDataPtr(M3gTemplate_Geometry_PrivateData, inGeometry);
	UUtInt32	itr;

	vector float*	mPtr;
	vector float*	curPoint = (vector float*)geomPrivate->pointSIMD;
	vector float*	curScreen = (vector float*)outResultScreenPoints;

	vector float	tempRow0, tempRow1, tempRow2, tempRow3;


	vector float	splatm00, splatm10, splatm20, splatm30;
	vector float	splatm01, splatm11, splatm21, splatm31;
	vector float	splatm02, splatm12, splatm22, splatm32;
	vector float	splatm03, splatm13, splatm23, splatm33;

	vector float	xxxx, yyyy, zzzz;

	vector float	hXXXX, hYYYY, hZZZZ, hWWWW;
	vector float	t1, t2;

	vector float	splatScaleX, splatScaleY;

	const vector float	zero = AVcSplatZero;
	const vector float	one = AVcSplatOne;
	const vector float	negOne = AVcSplatNegOne;
	const vector unsigned char	prl2Seq4x4_s0p0 = AVcPrl2Seq4x4_s0p0;
	const vector unsigned char	prl2Seq4x4_s0p1 = AVcPrl2Seq4x4_s0p1;
	const vector unsigned char	prl2Seq4x4_s1p0 = AVcPrl2Seq4x4_s1p0;
	const vector unsigned char	prl2Seq4x4_s1p1 = AVcPrl2Seq4x4_s1p1;

	UUtUns32	blockDescPoint, blockDescScreen;
	UUtInt32	numItrs;

	UUmAssert(((UUtUns32)&MSgGeomContextPrivate->matrix_localToFrustum & 0xF) == 0);
	UUmAssert(((UUtUns32)curPoint & 0xF) == 0);
	UUmAssert(((UUtUns32)curScreen & 0xF) == 0);

	numItrs = (inGeometry->pointArray->numPoints + 3) >> 2;

	blockDescPoint = AVmBuildBlockDST(3, numItrs, 3);
	blockDescScreen = AVmBuildBlockDST(4, numItrs, 4);

	vec_dst((vector float *)curPoint, blockDescPoint, 0);
	vec_dstst((vector float *)curScreen, blockDescScreen, 1);

	mPtr = (vector float*)MSgGeomContextPrivate->matrix_localToFrustum.m;

	tempRow0 = mPtr[0];
	tempRow1 = mPtr[1];
	tempRow2 = mPtr[2];
	tempRow3 = mPtr[3];

	*(float *)&splatScaleX = MSgGeomContextPrivate->scaleX;
	*(float *)&splatScaleY = MSgGeomContextPrivate->scaleY;
	splatScaleX = vec_splat(splatScaleX, 0);
	splatScaleY = vec_splat(splatScaleY, 0);

	splatm00 = vec_splat(tempRow0, 0);
	splatm01 = vec_splat(tempRow0, 1);
	splatm02 = vec_splat(tempRow0, 2);
	splatm03 = vec_splat(tempRow0, 3);

	splatm10 = vec_splat(tempRow1, 0);
	splatm11 = vec_splat(tempRow1, 1);
	splatm12 = vec_splat(tempRow1, 2);
	splatm13 = vec_splat(tempRow1, 3);

	splatm20 = vec_splat(tempRow2, 0);
	splatm21 = vec_splat(tempRow2, 1);
	splatm22 = vec_splat(tempRow2, 2);
	splatm23 = vec_splat(tempRow2, 3);

	splatm30 = vec_splat(tempRow3, 0);
	splatm31 = vec_splat(tempRow3, 1);
	splatm32 = vec_splat(tempRow3, 2);
	splatm33 = vec_splat(tempRow3, 3);

	for(itr = 0;
		itr < numItrs;
		itr++, curPoint += 3, curScreen += 4)
	{
		if((itr & 1) == 0) UUrProcessor_ZeroCacheLine(curScreen, 0);

		xxxx = curPoint[0];
		yyyy = curPoint[1];
		zzzz = curPoint[2];

		hXXXX = vec_madd(splatm00, xxxx, splatm30);
		hXXXX = vec_madd(splatm10, yyyy, hXXXX);
		hXXXX = vec_madd(splatm20, zzzz, hXXXX);

		hYYYY = vec_madd(splatm01, xxxx, splatm31);
		hYYYY = vec_madd(splatm11, yyyy, hYYYY);
		hYYYY = vec_madd(splatm21, zzzz, hYYYY);

		hZZZZ = vec_madd(splatm02, xxxx, splatm32);
		hZZZZ = vec_madd(splatm12, yyyy, hZZZZ);
		hZZZZ = vec_madd(splatm22, zzzz, hZZZZ);

		hWWWW = vec_madd(splatm03, xxxx, splatm33);
		hWWWW = vec_madd(splatm13, yyyy, hWWWW);
		hWWWW = vec_madd(splatm23, zzzz, hWWWW);

		t1 = vec_re(hWWWW);
		t2 = vec_nmsub(t1, hWWWW, one);
		hWWWW = vec_madd(t1, t2, t1);

		// hWWWW is now inv hWWWW

		hXXXX = vec_madd(hXXXX, hWWWW, one);
		hXXXX = vec_madd(hXXXX, splatScaleX, zero);

		hYYYY = vec_madd(hYYYY, hWWWW, negOne);
		hYYYY = vec_madd(hYYYY, splatScaleY, zero);

		hZZZZ = vec_madd(hZZZZ, hWWWW, zero);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p0);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p0);
		xxxx = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		yyyy = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p1);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p1);
		zzzz = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		hWWWW = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		curScreen[0] = xxxx;
		curScreen[1] = yyyy;
		curScreen[2] = zzzz;
		curScreen[3] = hWWWW;
	}

	//vec_dssall();
}

void
MSrTransform_Geom_PointListToScreen_ActiveVertices_AltiVec(
	M3tGeometry*			inGeometry,
	UUtUns32*				inActiveVerticesBV,
	M3tPointScreen			*outResultScreenPoints)
{
	M3tGeometry_Private*	geomPrivate = (M3tGeometry_Private*)TMrTemplate_PrivateData_GetDataPtr(M3gTemplate_Geometry_PrivateData, inGeometry);
	UUtInt32	itr;

	vector float*	mPtr;
	vector float*	curPoint = (vector float*)geomPrivate->pointSIMD;
	vector float*	curScreen = (vector float*)outResultScreenPoints;

	vector float	tempRow0, tempRow1, tempRow2, tempRow3;


	vector float	splatm00, splatm10, splatm20, splatm30;
	vector float	splatm01, splatm11, splatm21, splatm31;
	vector float	splatm02, splatm12, splatm22, splatm32;
	vector float	splatm03, splatm13, splatm23, splatm33;

	vector float	xxxx, yyyy, zzzz;

	vector float	hXXXX, hYYYY, hZZZZ, hWWWW;
	vector float	t1, t2;

	vector float	splatScaleX, splatScaleY;

	const vector float	zero = AVcSplatZero;
	const vector float	one = AVcSplatOne;
	const vector float	negOne = AVcSplatNegOne;
	const vector unsigned char	prl2Seq4x4_s0p0 = AVcPrl2Seq4x4_s0p0;
	const vector unsigned char	prl2Seq4x4_s0p1 = AVcPrl2Seq4x4_s0p1;
	const vector unsigned char	prl2Seq4x4_s1p0 = AVcPrl2Seq4x4_s1p0;
	const vector unsigned char	prl2Seq4x4_s1p1 = AVcPrl2Seq4x4_s1p1;

	UUtUns32	blockDescPoint, blockDescScreen;
	UUtInt32	numItrs;

	UUmAssert(((UUtUns32)&MSgGeomContextPrivate->matrix_localToFrustum & 0xF) == 0);
	UUmAssert(((UUtUns32)curPoint & 0xF) == 0);
	UUmAssert(((UUtUns32)curScreen & 0xF) == 0);

	numItrs = (inGeometry->pointArray->numPoints + 3) >> 2;

	blockDescPoint = AVmBuildBlockDST(3, numItrs, 3);
	blockDescScreen = AVmBuildBlockDST(4, numItrs, 4);

	vec_dst((vector float *)curPoint, blockDescPoint, 0);
	vec_dstst((vector float *)curScreen, blockDescScreen, 1);

	mPtr = (vector float*)MSgGeomContextPrivate->matrix_localToFrustum.m;

	tempRow0 = mPtr[0];
	tempRow1 = mPtr[1];
	tempRow2 = mPtr[2];
	tempRow3 = mPtr[3];

	*(float *)&splatScaleX = MSgGeomContextPrivate->scaleX;
	*(float *)&splatScaleY = MSgGeomContextPrivate->scaleY;
	splatScaleX = vec_splat(splatScaleX, 0);
	splatScaleY = vec_splat(splatScaleY, 0);

	splatm00 = vec_splat(tempRow0, 0);
	splatm01 = vec_splat(tempRow0, 1);
	splatm02 = vec_splat(tempRow0, 2);
	splatm03 = vec_splat(tempRow0, 3);

	splatm10 = vec_splat(tempRow1, 0);
	splatm11 = vec_splat(tempRow1, 1);
	splatm12 = vec_splat(tempRow1, 2);
	splatm13 = vec_splat(tempRow1, 3);

	splatm20 = vec_splat(tempRow2, 0);
	splatm21 = vec_splat(tempRow2, 1);
	splatm22 = vec_splat(tempRow2, 2);
	splatm23 = vec_splat(tempRow2, 3);

	splatm30 = vec_splat(tempRow3, 0);
	splatm31 = vec_splat(tempRow3, 1);
	splatm32 = vec_splat(tempRow3, 2);
	splatm33 = vec_splat(tempRow3, 3);

	for(itr = 0;
		itr < numItrs;
		itr++, curPoint += 3, curScreen += 4)
	{
		if((itr & 1) == 0) UUrProcessor_ZeroCacheLine(curScreen, 0);

		if(!UUrBitVector_TestBitRange(inActiveVerticesBV, itr * 4, itr * 4 + 3)) continue;

		xxxx = curPoint[0];
		yyyy = curPoint[1];
		zzzz = curPoint[2];

		hXXXX = vec_madd(splatm00, xxxx, splatm30);
		hXXXX = vec_madd(splatm10, yyyy, hXXXX);
		hXXXX = vec_madd(splatm20, zzzz, hXXXX);

		hYYYY = vec_madd(splatm01, xxxx, splatm31);
		hYYYY = vec_madd(splatm11, yyyy, hYYYY);
		hYYYY = vec_madd(splatm21, zzzz, hYYYY);

		hZZZZ = vec_madd(splatm02, xxxx, splatm32);
		hZZZZ = vec_madd(splatm12, yyyy, hZZZZ);
		hZZZZ = vec_madd(splatm22, zzzz, hZZZZ);

		hWWWW = vec_madd(splatm03, xxxx, splatm33);
		hWWWW = vec_madd(splatm13, yyyy, hWWWW);
		hWWWW = vec_madd(splatm23, zzzz, hWWWW);

		t1 = vec_re(hWWWW);
		t2 = vec_nmsub(t1, hWWWW, one);
		hWWWW = vec_madd(t1, t2, t1);

		// hWWWW is now inv hWWWW

		hXXXX = vec_madd(hXXXX, hWWWW, one);
		hXXXX = vec_madd(hXXXX, splatScaleX, zero);

		hYYYY = vec_madd(hYYYY, hWWWW, negOne);
		hYYYY = vec_madd(hYYYY, splatScaleY, zero);

		hZZZZ = vec_madd(hZZZZ, hWWWW, zero);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p0);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p0);
		xxxx = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		yyyy = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p1);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p1);
		zzzz = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		hWWWW = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		curScreen[0] = xxxx;
		curScreen[1] = yyyy;
		curScreen[2] = zzzz;
		curScreen[3] = hWWWW;
	}

	//vec_dssall();
}

void
MSrTransform_Geom_PointListToFrustumScreen_AltiVec(
	M3tGeometry*			inGeometry,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList)
{
	M3tGeometry_Private*	geomPrivate = (M3tGeometry_Private*)TMrTemplate_PrivateData_GetDataPtr(M3gTemplate_Geometry_PrivateData, inGeometry);
	UUtInt32	itr;

	vector float*	mPtr;
	vector float*	curPoint = (vector float*)geomPrivate->pointSIMD;
	vector float*	curScreen = (vector float*)outScreenPoints;
	vector float*	curFrustum = (vector float*)outFrustumPoints;

	vector float	tempRow0, tempRow1, tempRow2, tempRow3;


	vector float	splatm00, splatm10, splatm20, splatm30;
	vector float	splatm01, splatm11, splatm21, splatm31;
	vector float	splatm02, splatm12, splatm22, splatm32;
	vector float	splatm03, splatm13, splatm23, splatm33;

	vector float	xxxx, yyyy, zzzz, wwww;

	vector float	hXXXX, hYYYY, hZZZZ, hWWWW;
	vector float	t1, t2;

	vector float	splatScaleX, splatScaleY;
	vector float	negwwww;

	vector unsigned int	clipCodeXXXXPos, clipCodeYYYYPos, clipCodeZZZZPos;
	vector unsigned int	clipCodeXXXXNeg, clipCodeYYYYNeg, clipCodeZZZZNeg;
	vector unsigned int	clipCodeCum;
	vector unsigned int clipCode0123;
	vector unsigned char clipCode_lvsr;
	vector unsigned int mask;

	const vector float	one = AVcSplatOne;
	const vector float	negOne = AVcSplatNegOne;
	const vector unsigned char	prl2Seq4x4_s0p0 = AVcPrl2Seq4x4_s0p0;
	const vector unsigned char	prl2Seq4x4_s0p1 = AVcPrl2Seq4x4_s0p1;
	const vector unsigned char	prl2Seq4x4_s1p0 = AVcPrl2Seq4x4_s1p0;
	const vector unsigned char	prl2Seq4x4_s1p1 = AVcPrl2Seq4x4_s1p1;
	const vector unsigned int	clipCodeX_sr = (const vector unsigned int)(29, 29, 29, 29);
	const vector unsigned int	clipCodeY_sr = (const vector unsigned int)(27, 27, 27, 27);
	const vector unsigned int	clipCodeZ_sr = (const vector unsigned int)(25, 25, 25, 25);
	const vector unsigned int	clipCodeY_mask = (const vector unsigned int)(3 << 2, 3 << 2, 3 << 2, 3 << 2);
	const vector unsigned int	clipCodeZ_mask = (const vector unsigned int)(3 << 4, 3 << 4, 3 << 4, 3 << 4);
	const vector unsigned char	clipCode_perm = (const vector unsigned char)(3, 7, 11, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	const vector unsigned int splat_zero = (const vector unsigned int)(0, 0, 0, 0);
	const vector unsigned int splat_int_one = (const vector unsigned int)(1, 1, 1, 1);
	const vector unsigned int splat_int_allone = (const vector unsigned int)(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);

	UUtUns32	blockDescPoint, blockDescScreen;
	UUtInt32	numItrs;

	UUtUns32*	clipCodePtr = (UUtUns32*)outClipCodeList;

	UUmAssert(((UUtUns32)&MSgGeomContextPrivate->matrix_localToFrustum & 0xF) == 0);
	UUmAssert(((UUtUns32)curPoint & 0xF) == 0);
	UUmAssert(((UUtUns32)curScreen & 0xF) == 0);
	UUmAssert(((UUtUns32)clipCodePtr & 0xF) == 0);

	numItrs = (inGeometry->pointArray->numPoints + 3) >> 2;

	blockDescPoint = AVmBuildBlockDST(3, numItrs, 3);
	blockDescScreen = AVmBuildBlockDST(4, numItrs, 4);

	vec_dst((vector float *)curPoint, blockDescPoint, 0);
	vec_dstst((vector float *)curScreen, blockDescScreen, 1);

	mPtr = (vector float*)MSgGeomContextPrivate->matrix_localToFrustum.m;

	tempRow0 = mPtr[0];
	tempRow1 = mPtr[1];
	tempRow2 = mPtr[2];
	tempRow3 = mPtr[3];

	*(float *)&splatScaleX = MSgGeomContextPrivate->scaleX;
	*(float *)&splatScaleY = MSgGeomContextPrivate->scaleY;
	splatScaleX = vec_splat(splatScaleX, 0);
	splatScaleY = vec_splat(splatScaleY, 0);

	splatm00 = vec_splat(tempRow0, 0);
	splatm01 = vec_splat(tempRow0, 1);
	splatm02 = vec_splat(tempRow0, 2);
	splatm03 = vec_splat(tempRow0, 3);

	splatm10 = vec_splat(tempRow1, 0);
	splatm11 = vec_splat(tempRow1, 1);
	splatm12 = vec_splat(tempRow1, 2);
	splatm13 = vec_splat(tempRow1, 3);

	splatm20 = vec_splat(tempRow2, 0);
	splatm21 = vec_splat(tempRow2, 1);
	splatm22 = vec_splat(tempRow2, 2);
	splatm23 = vec_splat(tempRow2, 3);

	splatm30 = vec_splat(tempRow3, 0);
	splatm31 = vec_splat(tempRow3, 1);
	splatm32 = vec_splat(tempRow3, 2);
	splatm33 = vec_splat(tempRow3, 3);

	clipCodeCum = vec_or(splat_zero, splat_zero);

	for(itr = 0;
		itr < numItrs;
		itr++, curPoint += 3, curFrustum += 4, curScreen += 4, clipCodePtr++)
	{
		if((itr & 1) == 0)
		{
			UUrProcessor_ZeroCacheLine(curScreen, 0);
			UUrProcessor_ZeroCacheLine(curFrustum, 0);
		}

		// store the clip codes if needed
		if((itr % 4) == 0)
		{
			if(itr != 0)
			{
				*(vector unsigned int*)(clipCodePtr - 1) = clipCodeCum;
			}
			clipCodeCum = vec_or(splat_zero, splat_zero);
		}

		xxxx = curPoint[0];
		yyyy = curPoint[1];
		zzzz = curPoint[2];

		hXXXX = vec_madd(splatm00, xxxx, splatm30);
		hXXXX = vec_madd(splatm10, yyyy, hXXXX);
		hXXXX = vec_madd(splatm20, zzzz, hXXXX);

		hYYYY = vec_madd(splatm01, xxxx, splatm31);
		hYYYY = vec_madd(splatm11, yyyy, hYYYY);
		hYYYY = vec_madd(splatm21, zzzz, hYYYY);

		hZZZZ = vec_madd(splatm02, xxxx, splatm32);
		hZZZZ = vec_madd(splatm12, yyyy, hZZZZ);
		hZZZZ = vec_madd(splatm22, zzzz, hZZZZ);

		hWWWW = vec_madd(splatm03, xxxx, splatm33);
		hWWWW = vec_madd(splatm13, yyyy, hWWWW);
		hWWWW = vec_madd(splatm23, zzzz, hWWWW);

		negwwww = vec_sub((const vector float)splat_zero, hWWWW);

		// compute the clip codes
			clipCodeXXXXPos = (vector unsigned int)vec_cmpgt(hXXXX, hWWWW);
			clipCodeXXXXNeg = (vector unsigned int)vec_cmplt(hXXXX, negwwww);
			clipCodeYYYYPos = (vector unsigned int)vec_cmpgt(hYYYY, hWWWW);
			clipCodeYYYYNeg = (vector unsigned int)vec_cmplt(hYYYY, negwwww);
			clipCodeZZZZPos = (vector unsigned int)vec_cmpgt(hZZZZ, hWWWW);
			clipCodeZZZZNeg = (vector unsigned int)vec_cmplt(hZZZZ, negwwww);

			mask = vec_or(splat_int_one, splat_int_one);
			clipCode0123 = vec_and(clipCodeXXXXPos, mask);
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeXXXXNeg, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeYYYYPos, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeYYYYNeg, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeZZZZPos, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeZZZZNeg, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_perm(clipCode0123, clipCode0123, clipCode_perm);

			// we need to shift clipCode0123 based on clipCodePtr
			clipCode_lvsr = vec_lvsr(0, clipCodePtr);
			clipCode0123 = vec_perm(clipCode0123, clipCode0123, clipCode_lvsr);

			// or in clipCode0123 into clipCodeCum
			clipCodeCum = vec_or(clipCodeCum, clipCode0123);

		// store the frustum point
		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p0);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p0);
		xxxx = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		yyyy = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p1);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p1);
		zzzz = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		wwww = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		curFrustum[0] = xxxx;
		curFrustum[1] = yyyy;
		curFrustum[2] = zzzz;
		curFrustum[3] = wwww;

		t1 = vec_re(hWWWW);
		t2 = vec_nmsub(t1, hWWWW, one);
		hWWWW = vec_madd(t1, t2, t1);

		// hWWWW is now inv hWWWW

		hXXXX = vec_madd(hXXXX, hWWWW, one);
		hXXXX = vec_madd(hXXXX, splatScaleX, (const vector float)splat_zero);

		hYYYY = vec_madd(hYYYY, hWWWW, negOne);
		hYYYY = vec_madd(hYYYY, splatScaleY, (const vector float)splat_zero);

		hZZZZ = vec_madd(hZZZZ, hWWWW, (const vector float)splat_zero);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p0);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p0);
		xxxx = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		yyyy = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p1);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p1);
		zzzz = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		hWWWW = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		curScreen[0] = xxxx;
		curScreen[1] = yyyy;
		curScreen[2] = zzzz;
		curScreen[3] = hWWWW;
	}

	*(vector unsigned int*)(clipCodePtr-1) = clipCodeCum;

	//vec_dssall();
}

void
MSrTransform_Geom_PointListLocalToFrustumScreen_ActiveVertices_AltiVec(
	M3tGeometry*			inGeometry,
	const UUtUns32*			inActiveVerticesBV,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList)
{
	M3tGeometry_Private*	geomPrivate = (M3tGeometry_Private*)TMrTemplate_PrivateData_GetDataPtr(M3gTemplate_Geometry_PrivateData, inGeometry);
	UUtInt32	itr;

	vector float*	mPtr;
	vector float*	curPoint = (vector float*)geomPrivate->pointSIMD;
	vector float*	curScreen = (vector float*)outScreenPoints;
	vector float*	curFrustum = (vector float*)outFrustumPoints;

	vector float	tempRow0, tempRow1, tempRow2, tempRow3;


	vector float	splatm00, splatm10, splatm20, splatm30;
	vector float	splatm01, splatm11, splatm21, splatm31;
	vector float	splatm02, splatm12, splatm22, splatm32;
	vector float	splatm03, splatm13, splatm23, splatm33;

	vector float	xxxx, yyyy, zzzz, wwww;

	vector float	hXXXX, hYYYY, hZZZZ, hWWWW;
	vector float	t1, t2;

	vector float	splatScaleX, splatScaleY;
	vector float	negwwww;

	vector unsigned int	clipCodeXXXXPos, clipCodeYYYYPos, clipCodeZZZZPos;
	vector unsigned int	clipCodeXXXXNeg, clipCodeYYYYNeg, clipCodeZZZZNeg;
	vector unsigned int	clipCodeCum;
	vector unsigned int clipCode0123;
	vector unsigned char clipCode_lvsr;
	vector unsigned int mask;

	const vector float	one = AVcSplatOne;
	const vector float	negOne = AVcSplatNegOne;
	const vector unsigned char	prl2Seq4x4_s0p0 = AVcPrl2Seq4x4_s0p0;
	const vector unsigned char	prl2Seq4x4_s0p1 = AVcPrl2Seq4x4_s0p1;
	const vector unsigned char	prl2Seq4x4_s1p0 = AVcPrl2Seq4x4_s1p0;
	const vector unsigned char	prl2Seq4x4_s1p1 = AVcPrl2Seq4x4_s1p1;
	const vector unsigned int	clipCodeX_sr = (const vector unsigned int)(29, 29, 29, 29);
	const vector unsigned int	clipCodeY_sr = (const vector unsigned int)(27, 27, 27, 27);
	const vector unsigned int	clipCodeZ_sr = (const vector unsigned int)(25, 25, 25, 25);
	const vector unsigned int	clipCodeY_mask = (const vector unsigned int)(3 << 2, 3 << 2, 3 << 2, 3 << 2);
	const vector unsigned int	clipCodeZ_mask = (const vector unsigned int)(3 << 4, 3 << 4, 3 << 4, 3 << 4);
	const vector unsigned char	clipCode_perm = (const vector unsigned char)(3, 7, 11, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	const vector unsigned int splat_zero = (const vector unsigned int)(0, 0, 0, 0);
	const vector unsigned int splat_int_one = (const vector unsigned int)(1, 1, 1, 1);
	const vector unsigned int splat_int_allone = (const vector unsigned int)(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);

	UUtUns32	blockDescPoint, blockDescScreen;
	UUtInt32	numItrs;

	UUtUns32*	clipCodePtr = (UUtUns32*)outClipCodeList;

	UUmAssert(((UUtUns32)&MSgGeomContextPrivate->matrix_localToFrustum & 0xF) == 0);
	UUmAssert(((UUtUns32)curPoint & 0xF) == 0);
	UUmAssert(((UUtUns32)curScreen & 0xF) == 0);
	UUmAssert(((UUtUns32)clipCodePtr & 0xF) == 0);

	numItrs = (inGeometry->pointArray->numPoints + 3) >> 2;

	blockDescPoint = AVmBuildBlockDST(3, numItrs, 3);
	blockDescScreen = AVmBuildBlockDST(4, numItrs, 4);

	vec_dst((vector float *)curPoint, blockDescPoint, 0);
	vec_dstst((vector float *)curScreen, blockDescScreen, 1);

	mPtr = (vector float*)MSgGeomContextPrivate->matrix_localToFrustum.m;

	tempRow0 = mPtr[0];
	tempRow1 = mPtr[1];
	tempRow2 = mPtr[2];
	tempRow3 = mPtr[3];

	*(float *)&splatScaleX = MSgGeomContextPrivate->scaleX;
	*(float *)&splatScaleY = MSgGeomContextPrivate->scaleY;
	splatScaleX = vec_splat(splatScaleX, 0);
	splatScaleY = vec_splat(splatScaleY, 0);

	splatm00 = vec_splat(tempRow0, 0);
	splatm01 = vec_splat(tempRow0, 1);
	splatm02 = vec_splat(tempRow0, 2);
	splatm03 = vec_splat(tempRow0, 3);

	splatm10 = vec_splat(tempRow1, 0);
	splatm11 = vec_splat(tempRow1, 1);
	splatm12 = vec_splat(tempRow1, 2);
	splatm13 = vec_splat(tempRow1, 3);

	splatm20 = vec_splat(tempRow2, 0);
	splatm21 = vec_splat(tempRow2, 1);
	splatm22 = vec_splat(tempRow2, 2);
	splatm23 = vec_splat(tempRow2, 3);

	splatm30 = vec_splat(tempRow3, 0);
	splatm31 = vec_splat(tempRow3, 1);
	splatm32 = vec_splat(tempRow3, 2);
	splatm33 = vec_splat(tempRow3, 3);

	clipCodeCum = vec_or(splat_zero, splat_zero);

	for(itr = 0;
		itr < numItrs;
		itr++, curPoint += 3, curFrustum += 4, curScreen += 4, clipCodePtr++)
	{
		if((itr & 1) == 0)
		{
			UUrProcessor_ZeroCacheLine(curScreen, 0);
			UUrProcessor_ZeroCacheLine(curFrustum, 0);
		}

		// store the clip codes if needed
		if((itr % 4) == 0)
		{
			if(itr != 0)
			{
				*(vector unsigned int*)(clipCodePtr - 1) = clipCodeCum;
			}
			clipCodeCum = vec_or(splat_zero, splat_zero);
		}

		if(!UUrBitVector_TestBitRange(inActiveVerticesBV, itr * 4, itr * 4 + 3)) continue;

		xxxx = curPoint[0];
		yyyy = curPoint[1];
		zzzz = curPoint[2];

		hXXXX = vec_madd(splatm00, xxxx, splatm30);
		hXXXX = vec_madd(splatm10, yyyy, hXXXX);
		hXXXX = vec_madd(splatm20, zzzz, hXXXX);

		hYYYY = vec_madd(splatm01, xxxx, splatm31);
		hYYYY = vec_madd(splatm11, yyyy, hYYYY);
		hYYYY = vec_madd(splatm21, zzzz, hYYYY);

		hZZZZ = vec_madd(splatm02, xxxx, splatm32);
		hZZZZ = vec_madd(splatm12, yyyy, hZZZZ);
		hZZZZ = vec_madd(splatm22, zzzz, hZZZZ);

		hWWWW = vec_madd(splatm03, xxxx, splatm33);
		hWWWW = vec_madd(splatm13, yyyy, hWWWW);
		hWWWW = vec_madd(splatm23, zzzz, hWWWW);

		negwwww = vec_sub((const vector float)splat_zero, hWWWW);

		// compute the clip codes
			clipCodeXXXXPos = (vector unsigned int)vec_cmpgt(hXXXX, hWWWW);
			clipCodeXXXXNeg = (vector unsigned int)vec_cmplt(hXXXX, negwwww);
			clipCodeYYYYPos = (vector unsigned int)vec_cmpgt(hYYYY, hWWWW);
			clipCodeYYYYNeg = (vector unsigned int)vec_cmplt(hYYYY, negwwww);
			clipCodeZZZZPos = (vector unsigned int)vec_cmpgt(hZZZZ, hWWWW);
			clipCodeZZZZNeg = (vector unsigned int)vec_cmplt(hZZZZ, negwwww);

			mask = vec_or(splat_int_one, splat_int_one);
			clipCode0123 = vec_and(clipCodeXXXXPos, mask);
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeXXXXNeg, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeYYYYPos, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeYYYYNeg, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeZZZZPos, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeZZZZNeg, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_perm(clipCode0123, clipCode0123, clipCode_perm);

			// we need to shift clipCode0123 based on clipCodePtr
			clipCode_lvsr = vec_lvsr(0, clipCodePtr);
			clipCode0123 = vec_perm(clipCode0123, clipCode0123, clipCode_lvsr);

			// or in clipCode0123 into clipCodeCum
			clipCodeCum = vec_or(clipCodeCum, clipCode0123);

		// store the frustum point
		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p0);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p0);
		xxxx = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		yyyy = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p1);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p1);
		zzzz = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		wwww = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		curFrustum[0] = xxxx;
		curFrustum[1] = yyyy;
		curFrustum[2] = zzzz;
		curFrustum[3] = wwww;

		t1 = vec_re(hWWWW);
		t2 = vec_nmsub(t1, hWWWW, one);
		hWWWW = vec_madd(t1, t2, t1);

		// hWWWW is now inv hWWWW

		hXXXX = vec_madd(hXXXX, hWWWW, one);
		hXXXX = vec_madd(hXXXX, splatScaleX, (const vector float)splat_zero);

		hYYYY = vec_madd(hYYYY, hWWWW, negOne);
		hYYYY = vec_madd(hYYYY, splatScaleY, (const vector float)splat_zero);

		hZZZZ = vec_madd(hZZZZ, hWWWW, (const vector float)splat_zero);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p0);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p0);
		xxxx = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		yyyy = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p1);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p1);
		zzzz = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		hWWWW = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		curScreen[0] = xxxx;
		curScreen[1] = yyyy;
		curScreen[2] = zzzz;
		curScreen[3] = hWWWW;
	}

	*(vector unsigned int*)(clipCodePtr-1) = clipCodeCum;

	//vec_dssall();
}

void
MSrTransform_Geom_PointListAndVertexNormalToWorld_AltiVec(
	M3tGeometry*			inGeometry,
	M3tPoint3D				*outResultWorldPoints,
	M3tVector3D				*outResultWorldVertexNormals)
{
	M3tGeometry_Private*	geomPrivate = (M3tGeometry_Private*)TMrTemplate_PrivateData_GetDataPtr(M3gTemplate_Geometry_PrivateData, inGeometry);
	UUtInt32				itr;

	vector float*	mPtr;
	vector float*	curNormal = (vector float*)geomPrivate->vertexNormalSIMD;
	vector float*	curPoint = (vector float*)geomPrivate->pointSIMD;
	vector float*	xfrmNormal = (vector float*)outResultWorldVertexNormals;
	vector float*	xfrmPoint = (vector float*)outResultWorldPoints;

	vector float	splatm00, splatm10, splatm20, splatm30;
	vector float	splatm01, splatm11, splatm21, splatm31;
	vector float	splatm02, splatm12, splatm22, splatm32;

	vector float	tempRow0, tempRow1, tempRow2;

	vector float	nxxxx, nyyyy, nzzzz;
	vector float	pxxxx, pyyyy, pzzzz;

	vector float	tfXXXX, tfYYYY, tfZZZZ;

	vector float	zero = AVcSplatZero;

	UUtUns32	blockDesc;
	UUtInt32	numItrs;

	mPtr = (vector float*)MSgGeomContextPrivate->matrix_localToWorld->m;
	UUmAssert(((UUtUns32)curNormal & 0xF) == 0);
	UUmAssert(((UUtUns32)curPoint & 0xF) == 0);
	UUmAssert(((UUtUns32)xfrmNormal & 0xF) == 0);
	UUmAssert(((UUtUns32)xfrmPoint & 0xF) == 0);

	numItrs = (inGeometry->pointArray->numPoints + 3) >> 2;

	blockDesc = AVmBuildBlockDST(3, numItrs, 3);
	vec_dst((vector float *)curNormal, blockDesc, 0);
	vec_dst((vector float *)curPoint, blockDesc, 1);
	vec_dstst((vector float *)xfrmNormal, blockDesc, 2);
	vec_dstst((vector float *)xfrmPoint, blockDesc, 3);


	tempRow0 = mPtr[0];
	tempRow1 = mPtr[1];
	tempRow2 = mPtr[2];

	splatm00 = vec_splat(tempRow0, 0);
	splatm01 = vec_splat(tempRow0, 1);
	splatm02 = vec_splat(tempRow0, 2);
	splatm10 = vec_splat(tempRow0, 3);
	splatm11 = vec_splat(tempRow1, 0);
	splatm12 = vec_splat(tempRow1, 1);
	splatm20 = vec_splat(tempRow1, 2);
	splatm21 = vec_splat(tempRow1, 3);
	splatm22 = vec_splat(tempRow2, 0);
	splatm30 = vec_splat(tempRow2, 1);
	splatm31 = vec_splat(tempRow2, 2);
	splatm32 = vec_splat(tempRow2, 3);

	for(itr = 0;
		itr < numItrs;
		itr++, curNormal += 3, xfrmNormal += 3, curPoint += 3, xfrmPoint += 3)
	{
		#if 0
		if((itr % 3) == 0)
		{
			UUrProcessor_ZeroCacheLine((char*)xfrmNormal, 0);
			UUrProcessor_ZeroCacheLine((char*)xfrmNormal + UUcProcessor_CacheLineSize, 0);
			UUrProcessor_ZeroCacheLine((char*)xfrmNormal + UUcProcessor_CacheLineSize * 2, 0);
			UUrProcessor_ZeroCacheLine((char*)xfrmPoint, 0);
			UUrProcessor_ZeroCacheLine((char*)xfrmPoint + UUcProcessor_CacheLineSize, 0);
			UUrProcessor_ZeroCacheLine((char*)xfrmPoint + UUcProcessor_CacheLineSize * 2, 0);
		}
		#endif

		nxxxx = curNormal[0];
		nyyyy = curNormal[1];
		nzzzz = curNormal[2];

		tfXXXX = vec_madd(splatm00, nxxxx, zero);
		tfXXXX = vec_madd(splatm10, nyyyy, tfXXXX);
		tfXXXX = vec_madd(splatm20, nzzzz, tfXXXX);

		tfYYYY = vec_madd(splatm01, nxxxx, zero);
		tfYYYY = vec_madd(splatm11, nyyyy, tfYYYY);
		tfYYYY = vec_madd(splatm21, nzzzz, tfYYYY);

		tfZZZZ = vec_madd(splatm02, nxxxx, zero);
		tfZZZZ = vec_madd(splatm12, nyyyy, tfZZZZ);
		tfZZZZ = vec_madd(splatm22, nzzzz, tfZZZZ);

		xfrmNormal[0] = tfXXXX;
		xfrmNormal[1] = tfYYYY;
		xfrmNormal[2] = tfZZZZ;

		pxxxx = curPoint[0];
		pyyyy = curPoint[1];
		pzzzz = curPoint[2];

		tfXXXX = vec_madd(splatm00, pxxxx, splatm30);
		tfXXXX = vec_madd(splatm10, pyyyy, tfXXXX);
		tfXXXX = vec_madd(splatm20, pzzzz, tfXXXX);

		tfYYYY = vec_madd(splatm01, pxxxx, splatm31);
		tfYYYY = vec_madd(splatm11, pyyyy, tfYYYY);
		tfYYYY = vec_madd(splatm21, pzzzz, tfYYYY);

		tfZZZZ = vec_madd(splatm02, pxxxx, splatm32);
		tfZZZZ = vec_madd(splatm12, pyyyy, tfZZZZ);
		tfZZZZ = vec_madd(splatm22, pzzzz, tfZZZZ);

		xfrmPoint[0] = tfXXXX;
		xfrmPoint[1] = tfYYYY;
		xfrmPoint[2] = tfZZZZ;
	}

	//vec_dssall();
}

void
MSrTransform_Geom_PointListAndVertexNormalToWorldComputeViewVector_AltiVec(
	M3tGeometry*			inGeometry,
	M3tPoint3D				*outResultWorldPoints,
	M3tVector3D				*outResultWorldViewVectors,
	M3tVector3D				*outResultWorldVertexNormals)
{
	M3tGeometry_Private*	geomPrivate = (M3tGeometry_Private*)TMrTemplate_PrivateData_GetDataPtr(M3gTemplate_Geometry_PrivateData, inGeometry);
	UUtInt32				itr;

	vector float*	mPtr;
	vector float*	curNormal = (vector float*)geomPrivate->vertexNormalSIMD;
	vector float*	curPoint = (vector float*)geomPrivate->pointSIMD;
	vector float*	xfrmNormal = (vector float*)outResultWorldVertexNormals;
	vector float*	xfrmPoint = (vector float*)outResultWorldPoints;
	vector float*	curViewVector = (vector float*)outResultWorldViewVectors;

	vector float	splatm00, splatm10, splatm20, splatm30;
	vector float	splatm01, splatm11, splatm21, splatm31;
	vector float	splatm02, splatm12, splatm22, splatm32;

	vector float	tempRow0, tempRow1, tempRow2;

	vector float	nxxxx, nyyyy, nzzzz;
	vector float	pxxxx, pyyyy, pzzzz;

	vector float	tfXXXX, tfYYYY, tfZZZZ;

	vector float	zero = AVcSplatZero;

	vector float	splat_clX, splat_clY, splat_clZ;
	vector float	viewVecXXXX, viewVecYYYY, viewVecZZZZ, len0123;
	vector float	t0, t1, t2;

	const vector unsigned char	perm_s0p0 = AVcPrl2Seq4x3_s0p0;
	const vector unsigned char	perm_s0p1 = AVcPrl2Seq4x3_s0p1;
	const vector unsigned char	perm_s0p2 = AVcPrl2Seq4x3_s0p2;
	const vector unsigned char	perm_s1p0 = AVcPrl2Seq4x3_s1p0;
	const vector unsigned char	perm_s1p1 = AVcPrl2Seq4x3_s1p1;
	const vector unsigned char	perm_s1p2 = AVcPrl2Seq4x3_s1p2;

	const vector float splat_zero = (const vector float)((const vector unsigned int)(0, 0, 0, 0));
	const vector float splat_onehalf = (const vector float)(0.5f, 0.5f, 0.5f, 0.5f);
	const vector float splat_one = (const vector float)(1.0f, 1.0f, 1.0f, 1.0f);

	UUtUns32	blockDesc;
	UUtInt32	numItrs;

	UUmAssertReadPtr(curViewVector, sizeof(*curViewVector));

	mPtr = (vector float*)MSgGeomContextPrivate->matrix_localToWorld->m;
	UUmAssert(((UUtUns32)curNormal & 0xF) == 0);
	UUmAssert(((UUtUns32)curPoint & 0xF) == 0);
	UUmAssert(((UUtUns32)xfrmNormal & 0xF) == 0);
	UUmAssert(((UUtUns32)xfrmPoint & 0xF) == 0);
	UUmAssert(((UUtUns32)curViewVector & 0xF) == 0);

	numItrs = (inGeometry->pointArray->numPoints + 3) >> 2;

	blockDesc = AVmBuildBlockDST(3, numItrs, 3);
	vec_dst((vector float *)curNormal, blockDesc, 0);
	vec_dst((vector float *)curPoint, blockDesc, 1);
	vec_dstst((vector float *)xfrmNormal, blockDesc, 2);
	vec_dstst((vector float *)xfrmPoint, blockDesc, 3);


	tempRow0 = mPtr[0];
	tempRow1 = mPtr[1];
	tempRow2 = mPtr[2];

	splatm00 = vec_splat(tempRow0, 0);
	splatm01 = vec_splat(tempRow0, 1);
	splatm02 = vec_splat(tempRow0, 2);
	splatm10 = vec_splat(tempRow0, 3);
	splatm11 = vec_splat(tempRow1, 0);
	splatm12 = vec_splat(tempRow1, 1);
	splatm20 = vec_splat(tempRow1, 2);
	splatm21 = vec_splat(tempRow1, 3);
	splatm22 = vec_splat(tempRow2, 0);
	splatm30 = vec_splat(tempRow2, 1);
	splatm31 = vec_splat(tempRow2, 2);
	splatm32 = vec_splat(tempRow2, 3);

	//*(float*)&splat_clX = MSgGeomContextPrivate->activeCamera->cameraLocation.x;
	//*(float*)&splat_clY = MSgGeomContextPrivate->activeCamera->cameraLocation.y;
	//*(float*)&splat_clZ = MSgGeomContextPrivate->activeCamera->cameraLocation.z;
	*(float*)&splat_clX = MSgGeomContextPrivate->visCamera->cameraLocation.x;
	*(float*)&splat_clY = MSgGeomContextPrivate->visCamera->cameraLocation.y;
	*(float*)&splat_clZ = MSgGeomContextPrivate->visCamera->cameraLocation.z;

	splat_clX = vec_splat(splat_clX, 0);
	splat_clY = vec_splat(splat_clY, 0);
	splat_clZ = vec_splat(splat_clZ, 0);

	for(itr = 0;
		itr < numItrs;
		itr++, curNormal += 3, xfrmNormal += 3, curPoint += 3, xfrmPoint += 3, curViewVector += 3)
	{
		#if 0
		if((itr % 3) == 0)
		{
			UUrProcessor_ZeroCacheLine((char*)xfrmNormal, 0);
			UUrProcessor_ZeroCacheLine((char*)xfrmNormal + UUcProcessor_CacheLineSize, 0);
			UUrProcessor_ZeroCacheLine((char*)xfrmNormal + UUcProcessor_CacheLineSize * 2, 0);
			UUrProcessor_ZeroCacheLine((char*)xfrmPoint, 0);
			UUrProcessor_ZeroCacheLine((char*)xfrmPoint + UUcProcessor_CacheLineSize, 0);
			UUrProcessor_ZeroCacheLine((char*)xfrmPoint + UUcProcessor_CacheLineSize * 2, 0);
		}
		#endif

		nxxxx = curNormal[0];
		nyyyy = curNormal[1];
		nzzzz = curNormal[2];

		tfXXXX = vec_madd(splatm00, nxxxx, zero);
		tfXXXX = vec_madd(splatm10, nyyyy, tfXXXX);
		tfXXXX = vec_madd(splatm20, nzzzz, tfXXXX);

		tfYYYY = vec_madd(splatm01, nxxxx, zero);
		tfYYYY = vec_madd(splatm11, nyyyy, tfYYYY);
		tfYYYY = vec_madd(splatm21, nzzzz, tfYYYY);

		tfZZZZ = vec_madd(splatm02, nxxxx, zero);
		tfZZZZ = vec_madd(splatm12, nyyyy, tfZZZZ);
		tfZZZZ = vec_madd(splatm22, nzzzz, tfZZZZ);

		xfrmNormal[0] = tfXXXX;
		xfrmNormal[1] = tfYYYY;
		xfrmNormal[2] = tfZZZZ;

		pxxxx = curPoint[0];
		pyyyy = curPoint[1];
		pzzzz = curPoint[2];

		tfXXXX = vec_madd(splatm00, pxxxx, splatm30);
		tfXXXX = vec_madd(splatm10, pyyyy, tfXXXX);
		tfXXXX = vec_madd(splatm20, pzzzz, tfXXXX);

		tfYYYY = vec_madd(splatm01, pxxxx, splatm31);
		tfYYYY = vec_madd(splatm11, pyyyy, tfYYYY);
		tfYYYY = vec_madd(splatm21, pzzzz, tfYYYY);

		tfZZZZ = vec_madd(splatm02, pxxxx, splatm32);
		tfZZZZ = vec_madd(splatm12, pyyyy, tfZZZZ);
		tfZZZZ = vec_madd(splatm22, pzzzz, tfZZZZ);

		xfrmPoint[0] = tfXXXX;
		xfrmPoint[1] = tfYYYY;
		xfrmPoint[2] = tfZZZZ;

		// compute normalized view vector
		viewVecXXXX = vec_sub(splat_clX, tfXXXX);
		viewVecYYYY = vec_sub(splat_clY, tfYYYY);
		viewVecZZZZ = vec_sub(splat_clZ, tfZZZZ);

		len0123 = vec_madd(viewVecXXXX, viewVecXXXX, splat_zero);
		len0123 = vec_madd(viewVecYYYY, viewVecYYYY, len0123);
		len0123 = vec_madd(viewVecZZZZ, viewVecZZZZ, len0123);

		len0123 = vec_rsqrte(len0123);
		if(0)
		{
	 		vector float	y0, t0, t1;

			y0 = vec_rsqrte(len0123);
			t0 = vec_madd(y0, y0, splat_zero);
			t1 = vec_madd(y0, splat_onehalf, splat_zero);
			t0 = vec_nmsub(len0123, t0, splat_one);
			len0123 = vec_madd(t0, t1, y0);
		}

		viewVecXXXX = vec_madd(viewVecXXXX, len0123, splat_zero);
		viewVecYYYY = vec_madd(viewVecYYYY, len0123, splat_zero);
		viewVecZZZZ = vec_madd(viewVecZZZZ, len0123, splat_zero);

		// switch to xyzxyzxyzxyz format
		t0 = vec_perm(viewVecXXXX, viewVecZZZZ, perm_s0p0);
		t1 = vec_perm(viewVecXXXX, viewVecYYYY, perm_s0p1);
		t2 = vec_perm(viewVecYYYY, viewVecZZZZ, perm_s0p2);

		viewVecXXXX = vec_perm(t0, t2, perm_s1p0);
		viewVecYYYY = vec_perm(t0, t1, perm_s1p1);
		viewVecZZZZ = vec_perm(t1, t2, perm_s1p2);

		curViewVector[0] = viewVecXXXX;
		curViewVector[1] = viewVecYYYY;
		curViewVector[2] = viewVecZZZZ;
	}

	//vec_dssall();
}

void
MSrTransform_Geom_FaceNormalToWorld_AltiVec(
	M3tGeometry*			inGeometry,
	M3tVector3D				*outResultWorldTriNormals)
{
	M3tGeometry_Private*	geomPrivate = (M3tGeometry_Private*)TMrTemplate_PrivateData_GetDataPtr(M3gTemplate_Geometry_PrivateData, inGeometry);
	UUtInt32				itr;
	UUtInt32				numItrs;

	vector float*	mPtr;
	vector float*	curNormal;
	vector float*	xfrmNormal;

	vector float	splatm00, splatm10, splatm20;
	vector float	splatm01, splatm11, splatm21;
	vector float	splatm02, splatm12, splatm22;

	vector float	tempRow0, tempRow1, tempRow2;
	vector float	nxxxx, nyyyy, nzzzz;
	vector float	tfXXXX, tfYYYY, tfZZZZ;
	vector float	zero = AVcSplatZero;

	vector float	t0, t1, t2;

	const vector unsigned char	perm_s0p0 = AVcPrl2Seq4x3_s0p0;
	const vector unsigned char	perm_s0p1 = AVcPrl2Seq4x3_s0p1;
	const vector unsigned char	perm_s0p2 = AVcPrl2Seq4x3_s0p2;
	const vector unsigned char	perm_s1p0 = AVcPrl2Seq4x3_s1p0;
	const vector unsigned char	perm_s1p1 = AVcPrl2Seq4x3_s1p1;
	const vector unsigned char	perm_s1p2 = AVcPrl2Seq4x3_s1p2;

	mPtr = (vector float*)MSgGeomContextPrivate->matrix_localToWorld->m;

	UUmAssert(((UUtUns32)mPtr & 0xF) == 0);

	tempRow0 = mPtr[0];
	tempRow1 = mPtr[1];
	tempRow2 = mPtr[2];

	splatm00 = vec_splat(tempRow0, 0);
	splatm01 = vec_splat(tempRow0, 1);
	splatm02 = vec_splat(tempRow0, 2);
	splatm10 = vec_splat(tempRow0, 3);
	splatm11 = vec_splat(tempRow1, 0);
	splatm12 = vec_splat(tempRow1, 1);
	splatm20 = vec_splat(tempRow1, 2);
	splatm21 = vec_splat(tempRow1, 3);
	splatm22 = vec_splat(tempRow2, 0);

	numItrs = (inGeometry->triNormalArray->numVectors + 3) >> 2;

	curNormal = (vector float*)geomPrivate->triNormalSIMD;
	xfrmNormal = (vector float*)outResultWorldTriNormals;

	UUmAssert(((UUtUns32)curNormal & 0xF) == 0);
	UUmAssert(((UUtUns32)xfrmNormal & 0xF) == 0);

	for(itr = 0;
		itr < numItrs;
		itr++, curNormal += 3, xfrmNormal += 3)
	{
		nxxxx = curNormal[0];
		nyyyy = curNormal[1];
		nzzzz = curNormal[2];

		tfXXXX = vec_madd(splatm00, nxxxx, zero);
		tfXXXX = vec_madd(splatm10, nyyyy, tfXXXX);
		tfXXXX = vec_madd(splatm20, nzzzz, tfXXXX);

		tfYYYY = vec_madd(splatm01, nxxxx, zero);
		tfYYYY = vec_madd(splatm11, nyyyy, tfYYYY);
		tfYYYY = vec_madd(splatm21, nzzzz, tfYYYY);

		tfZZZZ = vec_madd(splatm02, nxxxx, zero);
		tfZZZZ = vec_madd(splatm12, nyyyy, tfZZZZ);
		tfZZZZ = vec_madd(splatm22, nzzzz, tfZZZZ);

		t0 = vec_perm(tfXXXX, tfZZZZ, perm_s0p0);
		t1 = vec_perm(tfXXXX, tfYYYY, perm_s0p1);
		t2 = vec_perm(tfYYYY, tfZZZZ, perm_s0p2);

		tfXXXX = vec_perm(t0, t2, perm_s1p0);
		tfYYYY = vec_perm(t0, t1, perm_s1p1);
		tfZZZZ = vec_perm(t1, t2, perm_s1p2);

		xfrmNormal[0] = tfXXXX;
		xfrmNormal[1] = tfYYYY;
		xfrmNormal[2] = tfZZZZ;
	}

}

void
MSrTransform_EnvPointListToFrustumScreen_ActiveVertices_AltiVec(
	const UUtUns32*			inActiveVerticesBV,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList)
{
	UUtInt32		itr;

	vector float*	mPtr;
	vector float*	curPoint = (vector float*)MSgGeomContextPrivate->envPointSIMD;
	vector float*	curScreen = (vector float*)outScreenPoints;
	vector float*	curFrustum = (vector float*)outFrustumPoints;

	vector float	tempRow0, tempRow1, tempRow2, tempRow3;


	vector float	splatm00, splatm10, splatm20, splatm30;
	vector float	splatm01, splatm11, splatm21, splatm31;
	vector float	splatm02, splatm12, splatm22, splatm32;
	vector float	splatm03, splatm13, splatm23, splatm33;

	vector float	xxxx, yyyy, zzzz, wwww;

	vector float	hXXXX, hYYYY, hZZZZ, hWWWW;
	vector float	t1, t2;

	vector float	splatScaleX, splatScaleY;
	vector float	negwwww;

	vector unsigned int	clipCodeXXXXPos, clipCodeYYYYPos, clipCodeZZZZPos;
	vector unsigned int	clipCodeXXXXNeg, clipCodeYYYYNeg, clipCodeZZZZNeg;
	vector unsigned int	clipCodeCum;
	vector unsigned int clipCode0123;
	vector unsigned char clipCode_lvsr;
	vector unsigned int mask;

	const vector float	one = AVcSplatOne;
	const vector float	negOne = AVcSplatNegOne;
	const vector unsigned char	prl2Seq4x4_s0p0 = AVcPrl2Seq4x4_s0p0;
	const vector unsigned char	prl2Seq4x4_s0p1 = AVcPrl2Seq4x4_s0p1;
	const vector unsigned char	prl2Seq4x4_s1p0 = AVcPrl2Seq4x4_s1p0;
	const vector unsigned char	prl2Seq4x4_s1p1 = AVcPrl2Seq4x4_s1p1;
	const vector unsigned int	clipCodeX_sr = (const vector unsigned int)(29, 29, 29, 29);
	const vector unsigned int	clipCodeY_sr = (const vector unsigned int)(27, 27, 27, 27);
	const vector unsigned int	clipCodeZ_sr = (const vector unsigned int)(25, 25, 25, 25);
	const vector unsigned int	clipCodeY_mask = (const vector unsigned int)(3 << 2, 3 << 2, 3 << 2, 3 << 2);
	const vector unsigned int	clipCodeZ_mask = (const vector unsigned int)(3 << 4, 3 << 4, 3 << 4, 3 << 4);
	const vector unsigned char	clipCode_perm = (const vector unsigned char)(3, 7, 11, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	const vector unsigned int splat_zero = (const vector unsigned int)(0, 0, 0, 0);
	const vector unsigned int splat_int_one = (const vector unsigned int)(1, 1, 1, 1);
	const vector unsigned int splat_int_allone = (const vector unsigned int)(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);

	UUtUns32	blockDescPoint, blockDescScreen;
	UUtInt32	numItrs;

	UUtUns32*	clipCodePtr = (UUtUns32*)outClipCodeList;

	UUmAssert(((UUtUns32)&MSgGeomContextPrivate->matrix_localToFrustum & 0xF) == 0);
	UUmAssert(((UUtUns32)curPoint & 0xF) == 0);
	UUmAssert(((UUtUns32)curScreen & 0xF) == 0);
	UUmAssert(((UUtUns32)clipCodePtr & 0xF) == 0);

	numItrs = (MSgGeomContextPrivate->environment->pointArray->numPoints + 3) >> 2;

	blockDescPoint = AVmBuildBlockDST(3, numItrs, 3);
	blockDescScreen = AVmBuildBlockDST(4, numItrs, 4);

	vec_dst((vector float *)curPoint, blockDescPoint, 0);
	vec_dstst((vector float *)curScreen, blockDescScreen, 1);

	mPtr = (vector float*)MSgGeomContextPrivate->matrix_localToFrustum.m;

	tempRow0 = mPtr[0];
	tempRow1 = mPtr[1];
	tempRow2 = mPtr[2];
	tempRow3 = mPtr[3];

	*(float *)&splatScaleX = MSgGeomContextPrivate->scaleX;
	*(float *)&splatScaleY = MSgGeomContextPrivate->scaleY;
	splatScaleX = vec_splat(splatScaleX, 0);
	splatScaleY = vec_splat(splatScaleY, 0);

	splatm00 = vec_splat(tempRow0, 0);
	splatm01 = vec_splat(tempRow0, 1);
	splatm02 = vec_splat(tempRow0, 2);
	splatm03 = vec_splat(tempRow0, 3);

	splatm10 = vec_splat(tempRow1, 0);
	splatm11 = vec_splat(tempRow1, 1);
	splatm12 = vec_splat(tempRow1, 2);
	splatm13 = vec_splat(tempRow1, 3);

	splatm20 = vec_splat(tempRow2, 0);
	splatm21 = vec_splat(tempRow2, 1);
	splatm22 = vec_splat(tempRow2, 2);
	splatm23 = vec_splat(tempRow2, 3);

	splatm30 = vec_splat(tempRow3, 0);
	splatm31 = vec_splat(tempRow3, 1);
	splatm32 = vec_splat(tempRow3, 2);
	splatm33 = vec_splat(tempRow3, 3);

	clipCodeCum = vec_or(splat_zero, splat_zero);

	for(itr = 0;
		itr < numItrs;
		itr++, curPoint += 3, curFrustum += 4, curScreen += 4, clipCodePtr++)
	{
		if((itr & 1) == 0)
		{
			UUrProcessor_ZeroCacheLine(curScreen, 0);
			UUrProcessor_ZeroCacheLine(curFrustum, 0);
		}

		// store the clip codes if needed
		if((itr % 4) == 0)
		{
			if(itr != 0)
			{
				*(vector unsigned int*)(clipCodePtr - 1) = clipCodeCum;
			}
			clipCodeCum = vec_or(splat_zero, splat_zero);
		}

		if(!UUrBitVector_TestBitRange(inActiveVerticesBV, itr * 4, itr * 4 + 3)) continue;

		xxxx = curPoint[0];
		yyyy = curPoint[1];
		zzzz = curPoint[2];

		hXXXX = vec_madd(splatm00, xxxx, splatm30);
		hXXXX = vec_madd(splatm10, yyyy, hXXXX);
		hXXXX = vec_madd(splatm20, zzzz, hXXXX);

		hYYYY = vec_madd(splatm01, xxxx, splatm31);
		hYYYY = vec_madd(splatm11, yyyy, hYYYY);
		hYYYY = vec_madd(splatm21, zzzz, hYYYY);

		hZZZZ = vec_madd(splatm02, xxxx, splatm32);
		hZZZZ = vec_madd(splatm12, yyyy, hZZZZ);
		hZZZZ = vec_madd(splatm22, zzzz, hZZZZ);

		hWWWW = vec_madd(splatm03, xxxx, splatm33);
		hWWWW = vec_madd(splatm13, yyyy, hWWWW);
		hWWWW = vec_madd(splatm23, zzzz, hWWWW);

		negwwww = vec_sub((const vector float)splat_zero, hWWWW);

		// compute the clip codes
			clipCodeXXXXPos = (vector unsigned int)vec_cmpgt(hXXXX, hWWWW);
			clipCodeXXXXNeg = (vector unsigned int)vec_cmplt(hXXXX, negwwww);
			clipCodeYYYYPos = (vector unsigned int)vec_cmpgt(hYYYY, hWWWW);
			clipCodeYYYYNeg = (vector unsigned int)vec_cmplt(hYYYY, negwwww);
			clipCodeZZZZPos = (vector unsigned int)vec_cmpgt(hZZZZ, hWWWW);
			clipCodeZZZZNeg = (vector unsigned int)vec_cmplt(hZZZZ, negwwww);

			mask = vec_or(splat_int_one, splat_int_one);
			clipCode0123 = vec_and(clipCodeXXXXPos, mask);
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeXXXXNeg, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeYYYYPos, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeYYYYNeg, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeZZZZPos, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_or(clipCode0123, vec_and(clipCodeZZZZNeg, mask));
			mask = vec_sl(mask, splat_int_one);

			clipCode0123 = vec_perm(clipCode0123, clipCode0123, clipCode_perm);

			// we need to shift clipCode0123 based on clipCodePtr
			clipCode_lvsr = vec_lvsr(0, clipCodePtr);
			clipCode0123 = vec_perm(clipCode0123, clipCode0123, clipCode_lvsr);

			// or in clipCode0123 into clipCodeCum
			clipCodeCum = vec_or(clipCodeCum, clipCode0123);

		// store the frustum point
		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p0);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p0);
		xxxx = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		yyyy = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p1);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p1);
		zzzz = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		wwww = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		curFrustum[0] = xxxx;
		curFrustum[1] = yyyy;
		curFrustum[2] = zzzz;
		curFrustum[3] = wwww;

		t1 = vec_re(hWWWW);
		t2 = vec_nmsub(t1, hWWWW, one);
		hWWWW = vec_madd(t1, t2, t1);

		// hWWWW is now inv hWWWW

		hXXXX = vec_madd(hXXXX, hWWWW, one);
		hXXXX = vec_madd(hXXXX, splatScaleX, (const vector float)splat_zero);

		hYYYY = vec_madd(hYYYY, hWWWW, negOne);
		hYYYY = vec_madd(hYYYY, splatScaleY, (const vector float)splat_zero);

		hZZZZ = vec_madd(hZZZZ, hWWWW, (const vector float)splat_zero);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p0);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p0);
		xxxx = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		yyyy = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		t1 = vec_perm(hXXXX, hYYYY, prl2Seq4x4_s0p1);
		t2 = vec_perm(hZZZZ, hWWWW, prl2Seq4x4_s0p1);
		zzzz = vec_perm(t1, t2, prl2Seq4x4_s1p0);
		hWWWW = vec_perm(t1, t2, prl2Seq4x4_s1p1);

		curScreen[0] = xxxx;
		curScreen[1] = yyyy;
		curScreen[2] = zzzz;
		curScreen[3] = hWWWW;
	}

	*(vector unsigned int*)(clipCodePtr-1) = clipCodeCum;

	//vec_dssall();
}

static const float M3gBackfaceRemoveThreshold = -0.1f;

void
MSrBackface_Remove_AltiVec(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	M3tVector3D*			inWorldViewVectors,
	M3tVector3D*			inWorldTriNormals,
	UUtUns32*				outActiveTrisBV,
	UUtUns32*				outActiveVerticesBV)
{
	UUrBitVector_SetBitAll(outActiveTrisBV, inGeometry->triNormalIndexArray->numIndices);
	UUrBitVector_SetBitAll(outActiveVerticesBV, inGeometry->pointArray->numPoints);

#if 0
	M3tVector3D*			curViewVector;
	M3tVector3D*			curNormal;
	UUtUns16				i;
	M3tTri*					curTri;
	M3tQuad*				curQuad;
	UUtUns16				index0, index1, index2, index3;

	if(inGeometry->triArray != NULL)
	{
		for(i = 0, curTri = inGeometry->triArray->tris,
				curNormal = inWorldTriNormals;
			i < inGeometry->triArray->numTris;
			i++, curTri++, curNormal++)
		{
			index0 = curTri->indices[0];
			curViewVector = inWorldViewVectors + index0;

			if(curViewVector->x * curNormal->x +
				curViewVector->y * curNormal->y +
				curViewVector->z * curNormal->z < M3gBackfaceRemoveThreshold)
			{
				continue;
			}

			UUrBitVector_SetBit(
				outActiveTrisBV,
				i);

			index1 = curTri->indices[1];
			index2 = curTri->indices[2];

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
	}

	if(inGeometry->quadArray != NULL)
	{
		for(i = 0, curQuad = inGeometry->quadArray->quads,
				curNormal = inWorldQuadNormals;
			i < inGeometry->quadArray->numQuads;
			i++, curQuad++, curNormal++)
		{
			index0 = curQuad->indices[0];
			curViewVector = inWorldViewVectors + index0;

			if(curViewVector->x * curNormal->x +
				curViewVector->y * curNormal->y +
				curViewVector->z * curNormal->z < M3gBackfaceRemoveThreshold)
			{
				continue;
			}

			UUrBitVector_SetBit(
				outActiveQuadsBV,
				i);

			index1 = curQuad->indices[1];
			index2 = curQuad->indices[2];
			index3 = curQuad->indices[3];

			UUrBitVector_SetBit(
				outActiveVerticesBV,
				index0);
			UUrBitVector_SetBit(
				outActiveVerticesBV,
				index1);
			UUrBitVector_SetBit(
				outActiveVerticesBV,
				index2);
			UUrBitVector_SetBit(
				outActiveVerticesBV,
				index3);
		}
	}
#endif
}


#endif
