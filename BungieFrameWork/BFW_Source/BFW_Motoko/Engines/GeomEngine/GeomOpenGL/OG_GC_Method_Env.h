/*
	FILE:	OG_GC_Method_Env.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 10, 1999
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef OG_GC_METHOD_ENV_H
#define OG_GC_METHOD_ENV_H

UUtError
OGrGeomContext_Method_Env_SetCamera(
	M3tGeomCamera*			inCamera);		// If null use active camera in geom context
			
UUtError
OGrGeomContext_Method_Env_DrawGQList(
	UUtUns32	inNumGQs,
	UUtUns32*	inGQIndices,
	UUtBool	inTransparentList);

UUtBool
OGrGeomContext_Method_Env_Check(
	UUtUns32*			inBBoxVertexIndices);

void
OGrGeomContext_Method_Env_Block(
	UUtUns32*			inBBoxVertexIndices,
	UUtUns32			inGQIndex);

void
OGrVis_Block_Frustum_Tri(
	UUtUns32		inVertex0Index,
	UUtUns32		inVertex1Index,
	UUtUns32		inVertex2Index);

void
OGrVis_Block_Frustum_Quad(
	UUtUns32		inVertex0Index,
	UUtUns32		inVertex1Index,
	UUtUns32		inVertex2Index,
	UUtUns32		inVertex3Index);

#endif /* OG_GC_METHOD_ENV_H */
