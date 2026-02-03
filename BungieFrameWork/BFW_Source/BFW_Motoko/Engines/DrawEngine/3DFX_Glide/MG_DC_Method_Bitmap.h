/*
	FILE:	MG_DC_Method_Bitmap.h

	AUTHOR:	Brent H. Pease

	CREATED: Nov 13, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef MG_DC_METHOD_BITMAP_H
#define MG_DC_METHOD_BITMAP_H

void
MGrDrawContext_Method_TriSprite(
	const M3tPointScreen	*inPoints,				// points[3]
	const M3tTextureCoord	*inTextureCoords);		// UVs[3]

void
MGrDrawContext_Method_Sprite(
	const M3tPointScreen	*inPoints,				// tl, tr, bl, br
	const M3tTextureCoord	*inTextureCoords);		// tl, tr, bl, br

UUtError MGrDrawContext_Method_ScreenCapture(
	const UUtRect			*inRect,
	void					*outBuffer);

UUtBool MGrDrawContext_Method_PointVisible(
	const M3tPointScreen	*inPoint,
	float					inTolerance);
#endif /* MG_DC_METHOD_BITMAP_H */
