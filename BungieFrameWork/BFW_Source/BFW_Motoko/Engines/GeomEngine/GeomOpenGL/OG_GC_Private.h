/*
	FILE:	OG_GC_Private.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 10, 1999
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef OG_GC_PRIVATE_H
#define OG_GC_PRIVATE_H

#include "BFW_Akira.h"
#include "BFW_Shared_Math.h"
#include "BFW_Shared_Clip.h"
#include "BFW_MathLib.h"


typedef struct OGtGeomContextPrivate	OGtGeomContextPrivate;

#define OGcTempQuads_Num	8
#define OGcTempVertices_Num	(OGcTempQuads_Num * 4)

#define OGcSpriteHorizWorldScale	(1.85f)
#define OGcSpriteVertWorldScale		(1.85f)

struct OGtGeomContextPrivate
{
	M3tGeomContextMethods	contextMethods;
	
	struct TMtCache_Simple*		geometryCache;
	
	AKtEnvironment*			environment;
	UUtUns32*				evil_stateInt;
	
	M3tPoint3D			tempPoints[OGcTempVertices_Num];
	M3tTextureCoord		tempBaseUVs[OGcTempVertices_Num];
	M3tTextureCoord		tempLightUVs[OGcTempVertices_Num];
	M3tTextureMap*		tempBaseMaps[OGcTempQuads_Num];
	M3tTextureMap*		tempLightMaps[OGcTempQuads_Num];
	M3tTextureMap*		tempCrudMaps[OGcTempQuads_Num];
	
};

extern OGtGeomContextPrivate	OGgGeomContextPrivate;

#endif /* OG_GC_PRIVATE_H */

