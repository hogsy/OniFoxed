/*
	FILE:	MS_DC_Method_SmallQuad.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 18, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef MS_DC_METHOD_SMALLQUAD_H
#define MS_DC_METHOD_SMALLQUAD_H

void
MSrDrawContext_Method_SmallQuadGouraudInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices);

void
MSrDrawContext_Method_SmallQuadGouraudFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices,
	UUtUns16		inFaceShade);

void
MSrDrawContext_Method_SmallQuadTextureInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices);

void
MSrDrawContext_Method_SmallQuadTextureFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns32		inIndices,
	UUtUns16		inFaceShade);

#endif /* MS_DC_METHOD_SMALLQUAD_H */
