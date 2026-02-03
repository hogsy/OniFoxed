/*
	FILE:	MD_DC_Method_Triangle.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 18, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef MD_DC_METHOD_TRIANGLE_H
#define MD_DC_METHOD_TRIANGLE_H

void
MDrDrawContext_Method_TriGouraudInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2);

void
MDrDrawContext_Method_TriGouraudFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inFaceShade);

void
MDrDrawContext_Method_TriTextureInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2);

void
MDrDrawContext_Method_TriTextureFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inFaceShade);

void
MDrDrawContext_Method_TriTextureSplit(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inBaseUVIndex0,
	UUtUns16		inBaseUVIndex1,
	UUtUns16		inBaseUVIndex2,
	UUtUns16		inLightUVIndex0,
	UUtUns16		inLightUVIndex1,
	UUtUns16		inLightUVIndex2);

#endif /* MD_DC_METHOD_TRIANGLE_H */
