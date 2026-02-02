/*
	FILE:	Motoko_SIMDCache_AltiVec.c

	AUTHOR:	Brent H. Pease

	CREATED: Aug 4, 1999

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997-1999

*/
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Platform.h"
#include "BFW_TemplateManager.h"
#include "BFW_Platform_AltiVec.h"

#include "Motoko_Manager.h"
#include "Motoko_Private.h"
#include "Motoko_SIMDCache.h"

#include "Motoko_SIMDCache_AltiVec.h"

#if UUmSIMD == UUmSIMD_AltiVec

extern UUtMemory_Heap*	M3gSIMD_MemoryHeap;

UUtUns32	counter = 0;

void
M3rSIMDCache_Platform_Convert(
	M3tGeometry*				inGeometry,
	M3tGeometry_SIMDCacheEntry*	inCacheEntry)
{

	AVrFloat_XYZ4ToXXXXYYYYZZZZ(
		inGeometry->pointArray->numPoints,
		(float*)inGeometry->pointArray->points,
		(float*)inCacheEntry->pointSIMD);

	AVrFloat_XYZ4ToXXXXYYYYZZZZ(
		inGeometry->pointArray->numPoints,
		(float*)inGeometry->vertexNormalArray->vectors,
		(float*)inCacheEntry->vertexNormalSIMD);

	AVrFloat_XYZ4ToXXXXYYYYZZZZ(
		inGeometry->triNormalArray->numVectors,
		(float*)inGeometry->triNormalArray->vectors,
		(float*)inCacheEntry->triNormalSIMD);

UUrMemory_Block_VerifyList();

}

#endif
