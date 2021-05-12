/*
	FILE:	MS_DC_Method_Bitmap.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Nov 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MS_DC_METHOD_BITMAP_H
#define MS_DC_METHOD_BITMAP_H

void
MSrDrawContext_Method_Bitmap(
	M3tDrawContext			*inDrawContext,
	M3tTextureMap			*inBitmap,
	M3tPointScreen			*inDestPoint,
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	UUtUns16				inShade,
	UUtUns16				inAlpha);			/* 0 - 31 */

#endif /* MS_DC_METHOD_BITMAP_H */