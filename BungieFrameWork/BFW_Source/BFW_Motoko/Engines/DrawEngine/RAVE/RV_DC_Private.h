/*
	FILE:	RV_DrawContext_Private.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 17, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef RV_DRAWCONTEXT_PRIVATE_H
#define RV_DRAWCONTEXT_PRIVATE_H

#include "BFW.h"
#include "BFW_Motoko.h"

#include <RAVE.h>
#include "ATIRave.h"

enum
{
	RVcEnv_LightmapMode_None,
	RVcEnv_LightmapMode_On,
	RVcEnv_LightmapMode_Only
};

typedef struct RVtTextureMapPrivate
{

	UUtUns32 			raveFlags;
	TQAImagePixelType	pixelType;
	TQAImage			image[9];

	void*				convertedData;

	TQATexture*			raveTexture;

} RVtTextureMapPrivate;

#define RVcStateStack_MaxDepth 3

typedef struct RVtDrawContextPrivate
{
	TQAEngine*			activeRaveEngine;
	TQADrawContext*		activeRaveContext;
	RaveExtFuncs*		atiExtFuncs;

	UUtUns16			width;
	UUtUns16			height;

	UUtUns8				stateFlags;

	UUtUns16					stateTOS;
	void*						statePtrStack[RVcStateStack_MaxDepth][M3cDrawStatePtrType_NumTypes];
	UUtInt32					stateIntStack[RVcStateStack_MaxDepth][M3cDrawStateIntType_NumTypes];

	void**						statePtr;
	UUtInt32*					stateInt;

} RVtDrawContextPrivate;

#define	RVmVertex_ConvertTexture(raveVertex, point, textCoord)	\
			do { \
				(raveVertex).x = (point).x;	\
				(raveVertex).y = (point).y;	\
				(raveVertex).z = (point).z;	\
				(raveVertex).invW = (point).invW;	\
				(raveVertex).uOverW = (textCoord).u * (point).invW; \
				(raveVertex).vOverW = (textCoord).v * (point).invW; \
			} while (0)

extern RVtDrawContextPrivate	RVgDrawContextPrivate;
extern TQADrawContext*	RVgActiveRaveContext;


#endif /* RV_DRAWCONTEXT_PRIVATE_H */
