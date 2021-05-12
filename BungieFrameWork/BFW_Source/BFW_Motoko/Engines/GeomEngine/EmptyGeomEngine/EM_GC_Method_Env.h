/*
	FILE:	EM_GC_Method_Env.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 10, 1999
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef EM_GC_METHOD_ENV_H
#define EM_GC_METHOD_ENV_H

UUtError
EGrGeomContext_Method_Env_SetCamera(
	M3tGeomCamera*			inCamera);		// If null use active camera in geom context
			
UUtError
EGrGeomContext_Method_Env_DrawGQBV(
	UUtUns32*			inGQBV,
	UUtUns16			inLightmapMode);

UUtBool
EGrGeomContext_Method_Env_Check(
	UUtUns16*			inBBoxVertexIndices);

void
EGrGeomContext_Method_Env_Block(
	UUtUns16*			inBBoxVertexIndices,
	UUtUns16			inGQIndex);

void
EGrVis_Block_Frustum_Tri(
	UUtUns16		inVertex0Index,
	UUtUns16		inVertex1Index,
	UUtUns16		inVertex2Index);

void
EGrVis_Block_Frustum_Quad(
	UUtUns16		inVertex0Index,
	UUtUns16		inVertex1Index,
	UUtUns16		inVertex2Index,
	UUtUns16		inVertex3Index);

#endif /* EM_GC_METHOD_ENV_H */
