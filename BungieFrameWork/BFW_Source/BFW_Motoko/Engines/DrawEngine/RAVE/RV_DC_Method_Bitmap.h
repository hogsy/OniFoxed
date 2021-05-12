/*
	FILE:	RV_DC_Method_Bitmap.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Nov 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef RV_DC_METHOD_BITMAP_H
#define RV_DC_METHOD_BITMAP_H

void
RVrDrawContext_Method_Sprite(
	const M3tPointScreen	*inPoints,				// tl, tr, bl, br
	const M3tTextureCoord	*inTextureCoords);		// tl, tr, bl, br

UUtError RVrDrawContext_Method_ScreenCapture(
	const UUtRect *inRect, 
	void *outBuffer);

#endif /* RV_DC_METHOD_BITMAP_H */
