/*
	FILE:	EM_DrawContext_Private.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 17, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef EM_DRAWCONTEXT_PRIVATE_H
#define EM_DRAWCONTEXT_PRIVATE_H

#include "BFW.h"
#include "BFW_Motoko.h"

enum
{
	RVcEnv_LightmapMode_None,
	RVcEnv_LightmapMode_On,
	RVcEnv_LightmapMode_Only
};

typedef struct EDtTextureMapPrivate
{

	UUtInt16 flags;
	UUtInt16 width, height;

} EDtTextureMapPrivate;

#define MGcStateStack_MaxDepth 3

typedef struct EDtDrawContextPrivate
{
	UUtInt32	width;
	UUtInt32	height;

	UUtUns16					stateTOS;
	void*						statePtrStack[MGcStateStack_MaxDepth][M3cDrawStatePtrType_NumTypes];
	UUtInt32					stateIntStack[MGcStateStack_MaxDepth][M3cDrawStateIntType_NumTypes];

	void**						statePtr;
	UUtInt32*					stateInt;

} EDtDrawContextPrivate;

#endif /* EM_DRAWCONTEXT_PRIVATE_H */
