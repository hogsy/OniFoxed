/*
	FILE:	MS_Geom_Shade_AltiVec.c

	AUTHOR:	Brent H. Pease

	CREATED: Aug. 4, 1999

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"
#include "BFW_Platform_AltiVec.h"
#include "BFW_BitVector.h"

#include "Motoko_Manager.h"

#include "MS_GC_Private.h"
#include "MS_Geom_Shade_AltiVec.h"

#if UUmSIMD == UUmSIMD_AltiVec

// THIS IS WRITTEN EXCLUSIVELY FOR THE DEMO
// It assume 2 directional lights
void
MSrShade_Vertices_Gouraud_Directional_AltiVec(
	M3tGeometry*	inGeometry,
	M3tPoint3D*		inWorldPoints,
	M3tVector3D*	inWorldVertexNormals,
	UUtUns32*		outShades)
{
	UUtInt16	itr;
	UUtUns16	numVertices;

	vector float*	curNormal;
	vector float	nXXXX, nYYYY, nZZZZ;

	vector float	rrrr, gggg, bbbb;

	vector float	splatAmbientR, splatAmbientG, splatAmbientB;

	vector float	splatDir0X, splatDir0Y, splatDir0Z;
	vector float	splatDir0R, splatDir0G, splatDir0B;
	vector float	splatDir1X, splatDir1Y, splatDir1Z;
	vector float	splatDir1R, splatDir1G, splatDir1B;

	const vector float	splatZero = AVcSplatZero;
	const vector float	oneMinusEpsilon = AVcSplatOneMinusEpsilon;

	vector float	dot;
	vector bool int	selectVec;

	vector float	vecR, vecG, vecB;

	vector unsigned long	dRRRR, dGGGG, dBBBB;

	UUtUns32	blockDescNormal, blockDescShades;
	UUtInt32	numItrs;

	// 0  0  0  R1 | 0  0  0  R2 | 0  0  0  R3 | 0  0  0  R4  <- 0
	// 0  1  2  3  | 4  5  6  7  | 8  9  A  B  | C  D  E  F

	// 0  0  0  G1 | 0  0  0  G2 | 0  0  0  G3 | 0  0  0  G4  <- 1
	// 0  1  2  3  | 4  5  6  7  | 8  9  A  B  | C  D  E  F

	// 0  R1 G1 0  | 0  R2 G2 0  | 0  R3 G3 0  | 0  R4 G4 0   <- result

	// 00 03 13 00 | 00 07 17 00 | 00 0B 1B 00 | 00 0F 1F 00
	const vector unsigned char perm0 = (vector unsigned char)
		(0x00, 0x03, 0x13, 0x00,  0x00, 0x07, 0x17, 0x00,  0x00, 0x0B, 0x1B, 0x00,  0x00, 0x0F, 0x1F, 0x00);

	// 0  R1 G1 0  | 0  R2 G2 0  | 0  R3 G3 0  | 0  R4 G4 0  <- 0
	// 0  1  2  3  | 4  5  6  7  | 8  9  A  B  | C  D  E  F

	// 0  0  0  B1 | 0  0  0  B2 | 0  0  0  B3 | 0  0  0  B4 <- 1
	// 0  1  2  3  | 4  5  6  7  | 8  9  A  B  | C  D  E  F

	// 0  R1 G1 B1 | 0  R2 G2 B2 | 0  R3 G3 B3 | 0  R4 G4 B4  <- result

	// 00 01 02 13 | 04 05 06 17 | 08 08 0A 1B | 0C 0D 0E 1D
	const vector unsigned char perm1 = (vector unsigned char)
		(0x00, 0x01, 0x02, 0x13,  0x04, 0x05, 0x06, 0x17,  0x08, 0x09, 0x0A, 0x1B,  0x0C, 0x0D, 0x0E, 0x1F);

	vector unsigned long*	shadeVector;

	numVertices = inGeometry->pointArray->numPoints;

	if(inGeometry->geometryFlags & M3cGeometryFlag_SelfIlluminent)
	{
		for(itr = 0; itr < numVertices; itr++)
		{
			*outShades++ = 0x7FFF;
		}
		return;
	}

	numItrs = (numVertices + 3) >> 2;

	blockDescNormal = AVmBuildBlockDST(3, numItrs, 3);
	blockDescShades = AVmBuildBlockDST(1, numItrs, 1);

	vec_dst((vector float *)inWorldVertexNormals, blockDescNormal, 0);
	vec_dstst((vector unsigned long *)outShades, blockDescShades, 1);

	UUmAssert(MSgGeomContextPrivate->light_NumDirectionalLights == 2);

	*(float*)&splatAmbientR = MSgGeomContextPrivate->light_Ambient.color.r;
	*(float*)&splatAmbientG = MSgGeomContextPrivate->light_Ambient.color.g;
	*(float*)&splatAmbientB = MSgGeomContextPrivate->light_Ambient.color.b;
	splatAmbientR = vec_splat(splatAmbientR, 0);
	splatAmbientG = vec_splat(splatAmbientG, 0);
	splatAmbientB = vec_splat(splatAmbientB, 0);

	*(float*)&splatDir0X = -MSgGeomContextPrivate->light_DirectionalList[0].normalizedDirection.x;
	*(float*)&splatDir0Y = -MSgGeomContextPrivate->light_DirectionalList[0].normalizedDirection.y;
	*(float*)&splatDir0Z = -MSgGeomContextPrivate->light_DirectionalList[0].normalizedDirection.z;
	*(float*)&splatDir0R = MSgGeomContextPrivate->light_DirectionalList[0].color.r;
	*(float*)&splatDir0G = MSgGeomContextPrivate->light_DirectionalList[0].color.g;
	*(float*)&splatDir0B = MSgGeomContextPrivate->light_DirectionalList[0].color.b;
	splatDir0X = vec_splat(splatDir0X, 0);
	splatDir0Y = vec_splat(splatDir0Y, 0);
	splatDir0Z = vec_splat(splatDir0Z, 0);
	splatDir0R = vec_splat(splatDir0R, 0);
	splatDir0G = vec_splat(splatDir0G, 0);
	splatDir0B = vec_splat(splatDir0B, 0);

	*(float*)&splatDir1X = -MSgGeomContextPrivate->light_DirectionalList[1].normalizedDirection.x;
	*(float*)&splatDir1Y = -MSgGeomContextPrivate->light_DirectionalList[1].normalizedDirection.y;
	*(float*)&splatDir1Z = -MSgGeomContextPrivate->light_DirectionalList[1].normalizedDirection.z;
	*(float*)&splatDir1R = MSgGeomContextPrivate->light_DirectionalList[1].color.r;
	*(float*)&splatDir1G = MSgGeomContextPrivate->light_DirectionalList[1].color.g;
	*(float*)&splatDir1B = MSgGeomContextPrivate->light_DirectionalList[1].color.b;
	splatDir1X = vec_splat(splatDir1X, 0);
	splatDir1Y = vec_splat(splatDir1Y, 0);
	splatDir1Z = vec_splat(splatDir1Z, 0);
	splatDir1R = vec_splat(splatDir1R, 0);
	splatDir1G = vec_splat(splatDir1G, 0);
	splatDir1B = vec_splat(splatDir1B, 0);

	curNormal = (vector float*)inWorldVertexNormals;
	shadeVector = (vector unsigned long*)outShades;

	for(itr = 0; itr < numItrs; itr++, curNormal += 3, shadeVector++)
	{
		if((itr & 0x3) == 0) UUrProcessor_ZeroCacheLine(shadeVector, 0);

		nXXXX = curNormal[0];
		nYYYY = curNormal[1];
		nZZZZ = curNormal[2];

		rrrr = splatAmbientR;
		gggg = splatAmbientG;
		bbbb = splatAmbientB;

		// do dir light 0
			dot = vec_madd(nXXXX, splatDir0X, splatZero);
			dot = vec_madd(nYYYY, splatDir0Y, dot);
			dot = vec_madd(nZZZZ, splatDir0Z, dot);

			selectVec = vec_cmpgt(dot, splatZero);

			vecR = vec_madd(dot, splatDir0R, splatZero);
			vecG = vec_madd(dot, splatDir0G, splatZero);
			vecB = vec_madd(dot, splatDir0B, splatZero);

			vecR = vec_sel(splatZero, vecR, selectVec);
			vecG = vec_sel(splatZero, vecG, selectVec);
			vecB = vec_sel(splatZero, vecB, selectVec);

			rrrr = vec_add(rrrr, vecR);
			gggg = vec_add(gggg, vecG);
			bbbb = vec_add(bbbb, vecB);

		// do dir light 1
			dot = vec_madd(nXXXX, splatDir1X, splatZero);
			dot = vec_madd(nYYYY, splatDir1Y, dot);
			dot = vec_madd(nZZZZ, splatDir1Z, dot);

			selectVec = vec_cmpgt(dot, splatZero);

			vecR = vec_madd(dot, splatDir1R, splatZero);
			vecG = vec_madd(dot, splatDir1G, splatZero);
			vecB = vec_madd(dot, splatDir1B, splatZero);

			vecR = vec_sel(splatZero, vecR, selectVec);
			vecG = vec_sel(splatZero, vecG, selectVec);
			vecB = vec_sel(splatZero, vecB, selectVec);

			rrrr = vec_add(rrrr, vecR);
			gggg = vec_add(gggg, vecG);
			bbbb = vec_add(bbbb, vecB);

		// cap at one
			rrrr = vec_min(rrrr, oneMinusEpsilon);
			gggg = vec_min(gggg, oneMinusEpsilon);
			bbbb = vec_min(bbbb, oneMinusEpsilon);

		// convert to int
			dRRRR = vec_ctu(rrrr, 8);
			dGGGG = vec_ctu(gggg, 8);
			dBBBB = vec_ctu(bbbb, 8);

		// permute into 0RGB 0RGB 0RGB 0RGB
			dRRRR = vec_perm(dRRRR, dGGGG, perm0);
			dRRRR = vec_perm(dRRRR, dBBBB, perm1);

		*shadeVector = dRRRR;
	}

	//vec_dssall();
}

void
MSrShade_Vertices_Gouraud_Directional_Active_AltiVec(
	M3tGeometry*	inGeometry,
	M3tPoint3D*		inWorldPoints,
	M3tVector3D*	inWorldVertexNormals,
	UUtUns32		*outShades,
	UUtUns32*		inActiveVerticesBV)
{
	UUtInt16	itr;
	UUtUns16	numVertices;

	vector float*	curNormal;
	vector float	nXXXX, nYYYY, nZZZZ;

	vector float	rrrr, gggg, bbbb;

	vector float	splatAmbientR, splatAmbientG, splatAmbientB;

	vector float	splatDir0X, splatDir0Y, splatDir0Z;
	vector float	splatDir0R, splatDir0G, splatDir0B;
	vector float	splatDir1X, splatDir1Y, splatDir1Z;
	vector float	splatDir1R, splatDir1G, splatDir1B;

	const vector float	splatZero = AVcSplatZero;
	const vector float	oneMinusEpsilon = AVcSplatOneMinusEpsilon;

	vector float	dot;
	vector bool int	selectVec;

	vector float	vecR, vecG, vecB;

	vector unsigned long	dRRRR, dGGGG, dBBBB;

	UUtUns32	blockDescNormal, blockDescShades;
	UUtInt32	numItrs;

	// 0  0  0  R1 | 0  0  0  R2 | 0  0  0  R3 | 0  0  0  R4  <- 0
	// 0  1  2  3  | 4  5  6  7  | 8  9  A  B  | C  D  E  F

	// 0  0  0  G1 | 0  0  0  G2 | 0  0  0  G3 | 0  0  0  G4  <- 1
	// 0  1  2  3  | 4  5  6  7  | 8  9  A  B  | C  D  E  F

	// 0  R1 G1 0  | 0  R2 G2 0  | 0  R3 G3 0  | 0  R4 G4 0   <- result

	// 00 03 13 00 | 00 07 17 00 | 00 0B 1B 00 | 00 0F 1F 00
	const vector unsigned char perm0 = (vector unsigned char)
		(0x00, 0x03, 0x13, 0x00,  0x00, 0x07, 0x17, 0x00,  0x00, 0x0B, 0x1B, 0x00,  0x00, 0x0F, 0x1F, 0x00);

	// 0  R1 G1 0  | 0  R2 G2 0  | 0  R3 G3 0  | 0  R4 G4 0  <- 0
	// 0  1  2  3  | 4  5  6  7  | 8  9  A  B  | C  D  E  F

	// 0  0  0  B1 | 0  0  0  B2 | 0  0  0  B3 | 0  0  0  B4 <- 1
	// 0  1  2  3  | 4  5  6  7  | 8  9  A  B  | C  D  E  F

	// 0  R1 G1 B1 | 0  R2 G2 B2 | 0  R3 G3 B3 | 0  R4 G4 B4  <- result

	// 00 01 02 13 | 04 05 06 17 | 08 08 0A 1B | 0C 0D 0E 1D
	const vector unsigned char perm1 = (vector unsigned char)
		(0x00, 0x01, 0x02, 0x13,  0x04, 0x05, 0x06, 0x17,  0x08, 0x09, 0x0A, 0x1B,  0x0C, 0x0D, 0x0E, 0x1F);

	vector unsigned long*	shadeVector;

	numVertices = inGeometry->pointArray->numPoints;

	if(inGeometry->geometryFlags & M3cGeometryFlag_SelfIlluminent)
	{
		for(itr = 0; itr < numVertices; itr++)
		{
			*outShades++ = 0x7FFF;
		}
		return;
	}

	numItrs = (numVertices + 3) >> 2;

	blockDescNormal = AVmBuildBlockDST(3, numItrs, 3);
	blockDescShades = AVmBuildBlockDST(1, numItrs, 1);

	vec_dst((vector float *)inWorldVertexNormals, blockDescNormal, 0);
	vec_dstst((vector unsigned long *)outShades, blockDescShades, 1);

	UUmAssert(MSgGeomContextPrivate->light_NumDirectionalLights == 2);

	*(float*)&splatAmbientR = MSgGeomContextPrivate->light_Ambient.color.r;
	*(float*)&splatAmbientG = MSgGeomContextPrivate->light_Ambient.color.g;
	*(float*)&splatAmbientB = MSgGeomContextPrivate->light_Ambient.color.b;
	splatAmbientR = vec_splat(splatAmbientR, 0);
	splatAmbientG = vec_splat(splatAmbientG, 0);
	splatAmbientB = vec_splat(splatAmbientB, 0);

	*(float*)&splatDir0X = -MSgGeomContextPrivate->light_DirectionalList[0].normalizedDirection.x;
	*(float*)&splatDir0Y = -MSgGeomContextPrivate->light_DirectionalList[0].normalizedDirection.y;
	*(float*)&splatDir0Z = -MSgGeomContextPrivate->light_DirectionalList[0].normalizedDirection.z;
	*(float*)&splatDir0R = MSgGeomContextPrivate->light_DirectionalList[0].color.r;
	*(float*)&splatDir0G = MSgGeomContextPrivate->light_DirectionalList[0].color.g;
	*(float*)&splatDir0B = MSgGeomContextPrivate->light_DirectionalList[0].color.b;
	splatDir0X = vec_splat(splatDir0X, 0);
	splatDir0Y = vec_splat(splatDir0Y, 0);
	splatDir0Z = vec_splat(splatDir0Z, 0);
	splatDir0R = vec_splat(splatDir0R, 0);
	splatDir0G = vec_splat(splatDir0G, 0);
	splatDir0B = vec_splat(splatDir0B, 0);

	*(float*)&splatDir1X = -MSgGeomContextPrivate->light_DirectionalList[1].normalizedDirection.x;
	*(float*)&splatDir1Y = -MSgGeomContextPrivate->light_DirectionalList[1].normalizedDirection.y;
	*(float*)&splatDir1Z = -MSgGeomContextPrivate->light_DirectionalList[1].normalizedDirection.z;
	*(float*)&splatDir1R = MSgGeomContextPrivate->light_DirectionalList[1].color.r;
	*(float*)&splatDir1G = MSgGeomContextPrivate->light_DirectionalList[1].color.g;
	*(float*)&splatDir1B = MSgGeomContextPrivate->light_DirectionalList[1].color.b;
	splatDir1X = vec_splat(splatDir1X, 0);
	splatDir1Y = vec_splat(splatDir1Y, 0);
	splatDir1Z = vec_splat(splatDir1Z, 0);
	splatDir1R = vec_splat(splatDir1R, 0);
	splatDir1G = vec_splat(splatDir1G, 0);
	splatDir1B = vec_splat(splatDir1B, 0);

	curNormal = (vector float*)inWorldVertexNormals;
	shadeVector = (vector unsigned long*)outShades;

	for(itr = 0; itr < numItrs; itr++, curNormal += 3, shadeVector++)
	{
		if((itr & 0x3) == 0) UUrProcessor_ZeroCacheLine(shadeVector, 0);

		if(!UUrBitVector_TestBitRange(inActiveVerticesBV, itr * 4, itr * 4 + 3)) continue;

		nXXXX = curNormal[0];
		nYYYY = curNormal[1];
		nZZZZ = curNormal[2];

		rrrr = splatAmbientR;
		gggg = splatAmbientG;
		bbbb = splatAmbientB;

		// do dir light 0
			dot = vec_madd(nXXXX, splatDir0X, splatZero);
			dot = vec_madd(nYYYY, splatDir0Y, dot);
			dot = vec_madd(nZZZZ, splatDir0Z, dot);

			selectVec = vec_cmpgt(dot, splatZero);

			vecR = vec_madd(dot, splatDir0R, splatZero);
			vecG = vec_madd(dot, splatDir0G, splatZero);
			vecB = vec_madd(dot, splatDir0B, splatZero);

			vecR = vec_sel(splatZero, vecR, selectVec);
			vecG = vec_sel(splatZero, vecG, selectVec);
			vecB = vec_sel(splatZero, vecB, selectVec);

			rrrr = vec_add(rrrr, vecR);
			gggg = vec_add(gggg, vecG);
			bbbb = vec_add(bbbb, vecB);

		// do dir light 1
			dot = vec_madd(nXXXX, splatDir1X, splatZero);
			dot = vec_madd(nYYYY, splatDir1Y, dot);
			dot = vec_madd(nZZZZ, splatDir1Z, dot);

			selectVec = vec_cmpgt(dot, splatZero);

			vecR = vec_madd(dot, splatDir1R, splatZero);
			vecG = vec_madd(dot, splatDir1G, splatZero);
			vecB = vec_madd(dot, splatDir1B, splatZero);

			vecR = vec_sel(splatZero, vecR, selectVec);
			vecG = vec_sel(splatZero, vecG, selectVec);
			vecB = vec_sel(splatZero, vecB, selectVec);

			rrrr = vec_add(rrrr, vecR);
			gggg = vec_add(gggg, vecG);
			bbbb = vec_add(bbbb, vecB);

		// cap at one
			rrrr = vec_min(rrrr, oneMinusEpsilon);
			gggg = vec_min(gggg, oneMinusEpsilon);
			bbbb = vec_min(bbbb, oneMinusEpsilon);

		// convert to int
			dRRRR = vec_ctu(rrrr, 8);
			dGGGG = vec_ctu(gggg, 8);
			dBBBB = vec_ctu(bbbb, 8);

		// permute into 0RGB 0RGB 0RGB 0RGB
			dRRRR = vec_perm(dRRRR, dGGGG, perm0);
			dRRRR = vec_perm(dRRRR, dBBBB, perm1);

		*shadeVector = dRRRR;
	}

	//vec_dssall();
}


#endif
