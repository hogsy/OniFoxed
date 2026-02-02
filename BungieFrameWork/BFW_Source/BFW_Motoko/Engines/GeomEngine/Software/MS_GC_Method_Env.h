/*
	FILE:	MS_GC_Method_Env.h

	AUTHOR:	Brent H. Pease

	CREATED: Jan 14, 1998

	PURPOSE:

	Copyright 1997

*/
#ifndef MS_GC_METHOD_ENV_H
#define MS_GC_METHOD_ENV_H

UUtError
MSrGeomContext_Method_Env_SetCamera(
	M3tGeomCamera*			inCamera);		// If null use active camera in geom context

UUtError
MSrGeomContext_Method_Env_DrawGQList(
	UUtUns32	inNumGQs,
	UUtUns32*	inGQIndices,
	UUtBool		inTransparentList);

UUtBool
MSrGeomContext_Method_Env_Check(
	UUtUns32*			inBBoxVertexIndices);

void
MSrGeomContext_Method_Env_Block(
	UUtUns32*			inBBoxVertexIndices,
	UUtUns32			inGQIndex);

void
MSrVis_Block_Frustum_Tri(
	UUtUns32		inVertex0Index,
	UUtUns32		inVertex1Index,
	UUtUns32		inVertex2Index);

void
MSrVis_Block_Frustum_Quad(
	UUtUns32		inVertex0Index,
	UUtUns32		inVertex1Index,
	UUtUns32		inVertex2Index,
	UUtUns32		inVertex3Index);

#endif /* MS_GC_METHOD_ENV_H */
