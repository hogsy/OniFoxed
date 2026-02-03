/*
	FILE:	Motoko_Private.h

	AUTHOR:	Brent H. Pease

	CREATED: May 13, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/
#ifndef MOTOKO_PRIVATE_H
#define MOTOKO_PRIVATE_H

#define M3cStateStack_MaxDepth				8

// CB: maximum texture size is 256 x 256, mip-mapped (+50%)
#define M3cTexture_TemporaryStorageSize		((3 * 256 * 256 * sizeof(UUtUns32)) / 2);

typedef struct M3tManagerDrawContext
{
	M3tDrawContextMethods*	drawFuncs;	// This is the context presented to the app

	UUtUns16				width;
	UUtUns16				height;
	UUtUns32*				bitVector;

	M3tPointScreen*			pointArray;
	M3tTextureCoord*		baseUVArray;
	UUtUns32*				shadeArray;

	M3tDrawAPI				apiIndex;

} M3tManagerDrawContext;

typedef struct M3tManagerGeomContext
{
	M3tGeomContextMethods*	geomContext;

} M3tManagerGeomContext;

typedef struct M3tManagerTextureData
{
	M3tTextureMap*			temporaryTexture;

	UUtUns32				temporarySize;
	void*					temporaryStorage;
} M3tManagerTextureData;

typedef struct M3tManagerDrawEngine
{
	M3tDrawEngineCaps		caps;
	M3tDrawEngineMethods	methods;

} M3tManagerDrawEngine;

typedef struct M3tManagerGeomEngine
{
	M3tGeomEngineCaps		caps;
	M3tGeomEngineMethods	methods;

} M3tManagerGeomEngine;

extern M3tManagerDrawContext	M3gManagerDrawContext;
extern M3tManagerGeomContext	M3gManagerGeomContext;
extern M3tManagerDrawEngine		M3gDrawEngineList[];
//extern M3tManagerTextureData*	M3gTexture_Loaded_Head;
//extern M3tManagerTextureData*	M3gTexture_Loaded_Tail;
extern UUtUns16					M3gActiveDrawEngine;
extern UUtUns16					M3gActiveDisplayDevice;
extern UUtUns16					M3gActiveDisplayMode;
extern M3tManagerGeomEngine		M3gGeomEngineList[];
extern UUtUns16					M3gActiveGeomEngine;
extern UUtBool					M3gDraw_Sorting;
extern UUtBool					M3gDraw_Vertex_Unified;
extern M3tDrawContext_Counters	M3gDrawContext_Counters;
extern TMtPrivateData*			M3gTemplate_TextureMap_PrivateData;
extern TMtPrivateData*			M3gTemplate_TextureMapBig_PrivateData;


UUtError
M3rDraw_State_Initialize(
	void);

void
M3rDraw_State_Terminate(
	void);

UUtError
M3rGeom_State_Initialize(
	void);

void
M3rGeom_State_Terminate(
	void);

UUtError
M3rManager_Texture_Initialize(
	void);

void
M3rManager_Texture_Terminate(
	void);

#endif /* MOTOKO_PRIVATE_H */
