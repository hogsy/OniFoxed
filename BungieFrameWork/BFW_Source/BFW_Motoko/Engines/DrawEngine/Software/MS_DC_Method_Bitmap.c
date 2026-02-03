/*
	FILE:	MS_DC_Method_Bitmap.c

	AUTHOR:	Brent H. Pease

	CREATED: Nov 13, 1997

	PURPOSE:

	Copyright 1997

*/

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"

#include "MS_DC_Private.h"
#include "MS_DC_Method_Bitmap.h"

// ======================================================================
// defines
// ======================================================================
// bits and shifts
#define cAlphaBits4444			0x0000F000
#define cRedBits4444			0x00000F00
#define cGreenBits4444			0x000000F0
#define cBlueBits4444			0x0000000F

#define cAlphaShift4444			12
#define cRedShift4444			8
#define cGreenShift4444			4
#define cBlueShift4444			0

#define cAlphaBit1555			0x00008000
#define cRedBits555				0x00007C00
#define cGreenBits555			0x000003E0
#define cBlueBits555			0x0000001F

#define cRedBits565				0x0000F800
#define cGreenBits565			0x000007E0
#define cBlueBits565			0x0000001F

#define cRedShift555			10
#define cGreenShift555			5
#define cBlueShift555			0

#define cRedShift565			11
#define cGreenShift565			5
#define cBlueShift565			0

// the mixers are designed to take a pixel from 4444 or 555 color and blend it with a
// 555 or 565 color to produce a 555 or 565 color

// low level mixers
#define Red4444(s,i)			((((s & cRedBits4444) >> (cRedShift4444 - 1)) * i) >> 5)
#define Green4444(s,i)			((((s & cGreenBits4444) >> (cGreenShift4444 - 1)) * i) >> 5)
#define Blue4444(s,i)			((((s & cBlueBits4444) << 1) * i) >> 5)

#define Red555(s,i)				((((s & cRedBits555) >> cRedShift555) * i) >> 5)
#define Green555(s,i)			((((s & cGreenBits555) >> cGreenShift555) * i) >> 5)
#define Blue555(s,i)			((((s & cBlueBits555) >> cBlueShift555) * i) >> 5)

#define Red565(s,i)				((((s & cRedBits565) >> cRedShift565) * i) >> 5)
#define Green565(s,i)			((((s & cGreenBits565) >> cGreenShift565) * i) >> 5)
#define Blue565(s,i)			((((s & cBlueBits565) >> cBlueShift565) * i) >> 5)

// mid level mixers
#if 0 //UUmPlatform == UUmPlatform_Win32

	#define MixRed4444(s,d,i)	(((Red4444(s, i) + Red565(d,(31-i))) & 0x001F) << cRedShift565)
	#define MixGreen4444(s,d,i)	(((((Green4444(s, i) << 1) | 1) + Green565(d,(31-i))) & 0x001F) << cGreenShift565)
	#define MixBlue4444(s,d,i)	(((Blue4444(s, i) + Blue565(d,(31-i))) & 0x001F) << cBlueShift565)

	#define MixRed555(s,d,i)	(((Red555(s, i) + Red565(d,(31-i))) & 0x001F) << cRedShift565)
	#define MixGreen555(s,d,i)	(((((Green555(s, i) << 1) | 1) + Green565(d,(31-i))) & 0x003F) << cGreenShift565)
	#define MixBlue555(s,d,i)	(((Blue555(s, i) + Blue565(d,(31-i))) & 0x001F) << cBlueShift565)

#else

	#define MixRed4444(s,d,i)	(((Red4444(s, i) + Red555(d,(31-i))) & 0x001F) << cRedShift555)
	#define MixGreen4444(s,d,i)	(((Green4444(s, i) + Green555(d,(31-i))) & 0x001F) << cGreenShift555)
	#define MixBlue4444(s,d,i)	(((Blue4444(s, i) + Blue555(d,(31-i))) & 0x001F) << cBlueShift555)

	#define MixRed555(s,d,i)	(((Red555(s, i) + Red555(d,(31-i))) & 0x001F) << cRedShift555)
	#define MixGreen555(s,d,i)	(((Green555(s, i) + Green555(d,(31-i))) & 0x001F) << cGreenShift555)
	#define MixBlue555(s,d,i)	(((Blue555(s, i) + Blue555(d,(31-i))) & 0x001F) << cBlueShift555)

#endif

// hi level mixers
#define Mix4444(s,d,i)			(MixRed4444(s,d,i) | MixGreen4444(s,d,i) | MixBlue4444(s,d,i))
#define Mix555(s,d,i)			(MixRed555(s,d,i) | MixGreen555(s,d,i) | MixBlue555(s,d,i))

#define CalcAlpha(a4,a5)		(((((a4 & cAlphaBits4444) >> (cAlphaShift4444 - 1)) | 1) * a5) >> 5)

// ======================================================================
// funcitons
// ======================================================================
// ----------------------------------------------------------------------
void
MSrDrawContext_Method_Bitmap(
	M3tDrawContext			*inDrawContext,
	M3tTextureMap			*inBitmap,
	M3tPointScreen			*inDestPoint,
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	UUtUns16				inShade,
	UUtUns16				inAlpha)			/* 0 - 31 */
{
	UUtUns16				i, j;

	UUtUns16				alpha;

	UUtUns16				*src_texels, *src_texel;
	UUtUns32				src_rowtexels;
	UUtInt16				src_x, src_y;
	UUtUns16				src;

	UUtUns32				*src_32_texels, *src_32_texel;
	UUtUns32				src_32_rowtexels;

	UUtUns16				*dst_texels, *dst_texel;
	UUtUns32				dst_rowtexels;
	UUtInt16				dst_x, dst_y;

	UUtInt16				draw_width, draw_height;
	UUtInt16				clip_left, clip_top, clip_right, clip_bottom;

	MStDrawContextPrivate	*drawContextPrivate;

	// get access to the private context
	drawContextPrivate = (MStDrawContextPrivate *)inDrawContext->privateContext;
	if (drawContextPrivate->imageBufferBaseAddr == NULL)
		return;

	// set the clip bounds
	clip_left = 0;
	clip_top = 0;
	clip_right = (UUtUns16)drawContextPrivate->width;
	clip_bottom = (UUtUns16)drawContextPrivate->height;

	// if nothing is going to be drawn, exit now
	if (clip_left >= clip_right)
		return;
	if (clip_top >= clip_bottom)
		return;

	// set dst_x and dst_y
	dst_x = (UUtInt16)inDestPoint->x;
	dst_y = (UUtInt16)inDestPoint->y;

	// set src_x and src_y
	src_x = 0;
	src_y = 0;

	// set draw_width and draw_height
	draw_width = inWidth;//inBitmap->width;
	draw_height = inHeight;//inBitmap->height;

	if ((dst_x + draw_width) > clip_right)
		draw_width = clip_right - dst_x;
	if (dst_x < clip_left)
	{
		src_x = clip_left - dst_x;
		dst_x = clip_left;
		draw_width -= src_x;
	}
	if ((dst_y + draw_height) > clip_bottom)
		draw_height = clip_bottom - dst_y;
	if (dst_y < clip_top)
	{
		src_y = clip_top - dst_y;
		dst_y = clip_top;
		draw_height -= src_y;
	}

	// get the rowpixels and a pointer to the dst pixels
	dst_rowtexels = drawContextPrivate->imageBufferRowBytes >> 1;
	dst_texels =
		drawContextPrivate->imageBufferBaseAddr +
		dst_x +
		(dst_y * dst_rowtexels);


	switch (inBitmap->texelType)
	{
		case M3cTextureType_ARGB4444:
		case M3cTextureType_RGB555:
		case M3cTextureType_ARGB1555:
			// get a pointer to the source pixels
			src_rowtexels = inBitmap->rowBytes >> 1;
			src_texels =
				((UUtUns16*)inBitmap->data) +
				src_x +
				(src_y * src_rowtexels);

			// draw the 16bit bitmaps
			switch (inBitmap->texelType)
			{
				case M3cTextureType_ARGB4444:
					for (i = 0; i < draw_height; i++)
					{
						dst_texel = dst_texels;
						src_texel = src_texels;

						for (j = 0; j < draw_width; j++)
						{
							alpha = CalcAlpha(*src_texel, inAlpha);
							*dst_texel = Mix4444(*src_texel, *dst_texel, alpha);
							dst_texel++;
							src_texel++;
						}
						dst_texels += dst_rowtexels;
						src_texels += src_rowtexels;
					}
					break;

				case M3cTextureType_RGB555:
					for (i = 0; i < draw_height; i++)
					{
						dst_texel = dst_texels;
						src_texel = src_texels;

						for (j = 0; j < draw_width; j++)
						{
							*dst_texel = Mix555(*src_texel, *dst_texel, inAlpha);
							dst_texel++;
							src_texel++;
						}
						dst_texels += dst_rowtexels;
						src_texels += src_rowtexels;
					}
					break;

				case M3cTextureType_ARGB1555:
					for (i = 0; i < draw_height; i++)
					{
						dst_texel = dst_texels;
						src_texel = src_texels;

						for (j = 0; j < draw_width; j++)
						{
							if (*src_texel & cAlphaBit1555)
								*dst_texel = Mix555(*src_texel, *dst_texel, inAlpha);
							dst_texel++;
							src_texel++;
						}
						dst_texels += dst_rowtexels;
						src_texels += src_rowtexels;
					}
					break;
			}
			break;

		case M3cTextureType_ARGB8888:
			// get a pointer to the source pixels
			src_32_rowtexels = inBitmap->rowBytes >> 2;
			src_32_texels =
				((UUtUns32*)inBitmap->data) +
				src_x +
				(src_y * src_32_rowtexels);

			for (i = 0; i < draw_height; i++)
			{
				dst_texel = dst_texels;
				src_32_texel = src_32_texels;

				for (j = 0; j < draw_width; j++)
				{
					src = (UUtUns16)M3mARGB32_to_16(*src_32_texel);
					alpha = CalcAlpha(src, inAlpha);
					*dst_texel = Mix4444(src, *dst_texel, alpha);
					dst_texel++;
					src_32_texel++;
				}
				dst_texels += dst_rowtexels;
				src_32_texels += src_32_rowtexels;
			}
			break;

		default:
			UUmAssert(!"Unknown TexelType");
			break;
	}
}
