/*
	FILE:	BFW_Image_Private.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Nov 27, 1998
	
	PURPOSE: 
	
	Copyright 1998
*/
#ifndef BFW_IMAGE_PRIVATE_H
#define BFW_IMAGE_PRIVATE_H

UUtError
IMrImage_Scale_Box(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	IMtPixelType	inSrcPixelType,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData);

UUtError
IMrImage_Scale_Box_IndependantAlpha(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	IMtPixelType	inSrcPixelType,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData);

// 0 is considered zero intensity - 255 is full intensity
// values outside this range are clamped to 0 and 255
typedef struct IMtDitherPixel
{
	UUtInt16	a, r, g, b;
	
} IMtDitherPixel;

void
IMrImage_Dither_Row_ARGB_to_ARGB1555(
	UUtUns16		inWidth,
	IMtDitherPixel*	inSrcCurRow,
	IMtDitherPixel*	inSrcNextRow,
	UUtUns16*		outDstRow);

void
IMrImage_Dither_Row_ARGB_to_ARGB4444(
	UUtUns16		inWidth,
	IMtDitherPixel*	inSrcCurRow,
	IMtDitherPixel*	inSrcNextRow,
	UUtUns16*		outDstRow);

void
IMrImage_Dither_Row_RGB_to_ARGB1555(
	UUtUns16		inWidth,
	IMtDitherPixel*	inSrcCurRow,
	IMtDitherPixel*	inSrcNextRow,
	UUtUns16*		outDstRow);

void
IMrImage_Dither_Row_RGB_to_ARGB4444(
	UUtUns16		inWidth,
	IMtDitherPixel*	inSrcCurRow,
	IMtDitherPixel*	inSrcNextRow,
	UUtUns16*		outDstRow);

#endif /* BFW_IMAGE_PRIVATE_H */
