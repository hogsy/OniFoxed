/*
	FILE:	MS_DC_Method_Pent.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 18, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef MS_DC_METHOD_PENT_H
#define MS_DC_METHOD_PENT_H

void
MSrDrawContext_Method_PentGouraudInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inVIndex4);

void
MSrDrawContext_Method_PentGouraudFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inVIndex4,
	UUtUns16		inFaceShade);

void
MSrDrawContext_Method_PentTextureInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inVIndex4);

void
MSrDrawContext_Method_PentTextureFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inVIndex4,
	UUtUns16		inFaceShade);

void
MSrDrawContext_Method_PentTextureSplit(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inVIndex3,
	UUtUns16		inVIndex4,
	UUtUns16		inBaseUVIndex0,
	UUtUns16		inBaseUVIndex1,
	UUtUns16		inBaseUVIndex2,
	UUtUns16		inBaseUVIndex3,
	UUtUns16		inBaseUVIndex4,
	UUtUns16		inLightUVIndex0,
	UUtUns16		inLightUVIndex1,
	UUtUns16		inLightUVIndex2,
	UUtUns16		inLightUVIndex3,
	UUtUns16		inLightUVIndex4);

#endif /* MS_DC_METHOD_PENT_H */
