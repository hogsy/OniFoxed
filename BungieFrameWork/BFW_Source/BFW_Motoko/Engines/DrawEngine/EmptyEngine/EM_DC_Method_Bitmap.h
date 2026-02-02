/*
	FILE:	EM_DC_Method_Bitmap.h

	AUTHOR:	Brent H. Pease

	CREATED: Nov 13, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef EM_DC_METHOD_BITMAP_H
#define EM_DC_METHOD_BITMAP_H

void
EDrDrawContext_Method_Sprite(
	const M3tPointScreen	*inPoints,				// tl, tr, bl, br
	const M3tTextureCoord	*inTextureCoords,		// tl, tr, bl, br
	UUtUns16				inShade,
	UUtUns16				inAlpha);

UUtError
EDrDrawContext_Method_ScreenCapture(
	const UUtRect *inRect,
	void *outBuffer);


UUtBool
EDrDrawContext_Method_PointVisible(
	const M3tPointScreen	*inPoint,
	float					inTolerance);

#endif /* EM_DC_METHOD_BITMAP_H */
