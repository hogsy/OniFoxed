/*
	FILE:	BFW_Image_Scale_Box.c

	AUTHOR:	Brent H. Pease

	CREATED: Nov 27, 1998

	PURPOSE:

	Copyright 1998
*/

#include "BFW.h"
#include "BFW_Image.h"
#include "BFW_Image_Private.h"
#include "BFW_MathLib.h"

#define INCLUDE_UNTESTED_STUFF 0

#define UUmAssertInRange(ptr, start, length) UUmAssert((((UUtUns32)(ptr)) < (((UUtUns32)(start)) + ((UUtUns32)(length)))) && (((UUtUns32)(ptr)) >= ((UUtUns32)(start))))


static void
IMiImage_Scale_Box_Down_RGB888_PowerOfTwo(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inShiftAmount,
	void*			outDstData)
{
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	half_multiplier;
	UUtUns32	multiplier;
	UUtUns32	round_number;
	UUtUns32	dividing_shift;
	UUtUns32	cumr, cumg, cumb;
	UUtUns32	srcLength = sizeof(UUtUns32) * inSrcWidth * inSrcHeight;

	UUtUns32	dstWidth = inSrcWidth >> inShiftAmount;
	UUtUns32	dstHeight = inSrcHeight >> inShiftAmount;

	UUtUns32*	dstRowStartPtr;
	UUtUns32*	srcRowStartPtr;
	UUtUns32*	dstPtr;
	UUtUns32*	srcPtr;

	/* the pixmap needs to be shrunk in x and y */

	multiplier = 1 << inShiftAmount;
	dividing_shift = inShiftAmount * 2;
	round_number = (1 << dividing_shift) >> 1;
	half_multiplier = multiplier >> 1;

	/*
	 * We are going to traverse the destination map filtering each
	 * corresponding block in the source map down to a single pixel
	 */

	dstRowStartPtr = outDstData;

	for(desty = 0; desty < dstHeight; desty++)
	{
		dstPtr = dstRowStartPtr;

		for(destx = 0; destx < dstWidth; destx++)
		{
			cumr = cumg = cumb = round_number;	// so we round instead of going down

			//srcRowStartPtr = ((UUtUns32 *) inSrcData) + (desty * multiplier) * inSrcWidth + (destx * multiplier);
			srcRowStartPtr = ((UUtUns32 *) inSrcData) + (((desty * inSrcWidth) + destx) << inShiftAmount);

			for(srcy = 0; srcy < multiplier; srcy++)
			{
				srcPtr = srcRowStartPtr;

				for(srcx = 0; srcx < half_multiplier; srcx++)
				{
					UUtUns32	pixel1;
					UUtUns32	pixel2;

					UUmAssertInRange(srcPtr, inSrcData, srcLength);

					pixel1 = *srcPtr++;
					pixel2 = *srcPtr++;

					cumr += (pixel1 >> 16) & 0xFF;
					cumg += (pixel1 >> 8) & 0xFF;
					cumb += (pixel1 >> 0) & 0xFF;

					cumr += (pixel2 >> 16) & 0xFF;
					cumg += (pixel2 >> 8) & 0xFF;
					cumb += (pixel2 >> 0) & 0xFF;
				}

				srcRowStartPtr += inSrcWidth;
			}

			cumr = cumr >> dividing_shift;
			cumg = cumg >> dividing_shift;
			cumb = cumb >> dividing_shift;

			UUmAssert(cumr <= 0xff);
			UUmAssert(cumg <= 0xff);
			UUmAssert(cumb <= 0xff);

			*dstPtr++ = (cumr << 16) | (cumg << 8) | cumb;
		}

		dstRowStartPtr += dstWidth;
	}
}

static UUtError
IMiImage_Scale_Box_Down_RGB888(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	pixel;
	UUtUns32	cumr, cumg, cumb;
	UUtUns32	numPixels;
	UUtUns32	sx, sy, ex, ey;
	UUtUns32	srcLength = sizeof(UUtUns32) * inSrcWidth * inSrcHeight;

	UUtUns32*	dstRowStartPtr;
	UUtUns32*	srcRowStartPtr;
	UUtUns32*	dstPtr;
	UUtUns32*	srcPtr;

	/* the pixmap needs to be shrunk in x and y */

	{
		UUtUns32 width_multiplier = inSrcWidth / inDstWidth;
		UUtUns32 height_multiplier = inSrcHeight / inDstHeight;

		if (width_multiplier == height_multiplier)
		{
			if ((inSrcWidth == (width_multiplier * inDstWidth)) && (inSrcHeight == (height_multiplier * inDstHeight)))
			{
				UUtUns16 shift = 0;

				switch(width_multiplier)
				{
					case 2:   shift = 1; break;
					case 4:   shift = 2; break;
					case 8:   shift = 3; break;
					case 16:  shift = 4; break;
					case 32:  shift = 5; break;
					case 64:  shift = 6; break;
					case 128: shift = 7; break;
					case 256: shift = 8; break;
				}

				if (shift != 0)
				{
					IMiImage_Scale_Box_Down_RGB888_PowerOfTwo(inSrcWidth, inSrcHeight, inSrcData, shift, outDstData);
					return UUcError_None;
				}
			}
		}
	}

	/* compute the gradients to 16 bits of fractional precision */
	my = (inSrcHeight << 16) / inDstHeight;
	mx = (inSrcWidth << 16) / inDstWidth;

	/*
	 * We are going to traverse the destination map filtering each
	 * corresponding block in the source map down to a single pixel
	 */

	dstRowStartPtr = outDstData;

	for(desty = 0; desty < inDstHeight; desty++)
	{
		dstPtr = dstRowStartPtr;

		for(destx = 0; destx < inDstWidth; destx++)
		{
			/* compute the starting x, y and then the ending x, y in the source
			   pixmap space */
			sx = (destx * mx) >> 16;
			sy = (desty * my) >> 16;

			ex = (destx * mx + mx) >> 16;
			ey = (desty * my + my) >> 16;

			cumr = cumg = cumb = 0;

			srcRowStartPtr = (UUtUns32 *) inSrcData + sy * inSrcWidth + sx;

			numPixels = (ex - sx) * (ey - sy);

			UUmAssert(numPixels > 0);

			for(srcy = sy; srcy < ey; srcy++)
			{
				UUmAssert((srcy >= 0) && (srcy < inSrcHeight));
				srcPtr = srcRowStartPtr;

				for(srcx = sx; srcx < ex; srcx++)
				{
					UUmAssert((srcx >=0) && (srcx < inSrcWidth));
					UUmAssertInRange(srcPtr, inSrcData, srcLength);

					pixel = *srcPtr++;
					cumr += (pixel >> 16) & 0xFF;
					cumg += (pixel >> 8) & 0xFF;
					cumb += (pixel) & 0xFF;
				}

				srcRowStartPtr += inSrcWidth;
			}

			cumr = MUrFloat_Round_To_Int((float) cumr / numPixels);
			cumg = MUrFloat_Round_To_Int((float) cumg / numPixels);
			cumb = MUrFloat_Round_To_Int((float) cumb / numPixels);

			*dstPtr++ = (cumr << 16) | (cumg << 8) | cumb;
		}

		dstRowStartPtr += inDstWidth;
	}

	return UUcError_None;
}

static UUtError
IMiImage_Scale_Box_Down_RGB555(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	pixel;
	UUtUns32	cumr, cumg, cumb;
	UUtUns32	numPixels;
	UUtUns32	sx, sy, ex, ey;
	UUtUns32	srcLength = 2 * inSrcWidth * inSrcHeight;

	UUtUns16*	dstRowStartPtr;
	UUtUns16*	srcRowStartPtr;
	UUtUns16*	dstPtr;
	UUtUns16*	srcPtr;

	/* the pixmap needs to be shrunk in x and y */

	/* compute the gradients to 16 bits of fractional precision */
	my = (inSrcHeight << 16) / inDstHeight;
	mx = (inSrcWidth << 16) / inDstWidth;

	/*
	 * We are going to traverse the destination map filtering each
	 * corresponding block in the source map down to a single pixel
	 */

	dstRowStartPtr = outDstData;

	for(desty = 0; desty < inDstHeight; desty++)
	{
		dstPtr = dstRowStartPtr;

		for(destx = 0; destx < inDstWidth; destx++)
		{
			/* compute the starting x, y and then the ending x, y in the source
			   pixmap space */
			sx = (destx * mx) >> 16;
			sy = (desty * my) >> 16;

			ex = (destx * mx + mx) >> 16;
			ey = (desty * my + my) >> 16;

			cumr = cumg = cumb = 0;

			srcRowStartPtr = (UUtUns16*)inSrcData + sy * inSrcWidth + sx;

			numPixels = (ex - sx) * (ey - sy);

			UUmAssert(numPixels > 0);

			for(srcy = sy; srcy < ey; srcy++)
			{
				UUmAssert((srcy >= 0) && (srcy < inSrcHeight));
				srcPtr = srcRowStartPtr;

				for(srcx = sx; srcx < ex; srcx++)
				{
					UUmAssert((srcx >=0) && (srcx < inSrcWidth));
					UUmAssertInRange(srcPtr, inSrcData, srcLength);

					pixel = *srcPtr++;
					cumr += (pixel >> 10) & 0x1F;
					cumg += (pixel >> 5) & 0x1F;
					cumb += (pixel >> 0) & 0x1F;
				}

				srcRowStartPtr += inSrcWidth;
			}

			cumr = MUrUnsignedSmallFloat_To_Uns_Round((float) cumr / numPixels);
			cumg = MUrUnsignedSmallFloat_To_Uns_Round((float) cumg / numPixels);
			cumb = MUrUnsignedSmallFloat_To_Uns_Round((float) cumb / numPixels);

			*dstPtr++ = (UUtUns16)((1 << 15) | (cumr << 10) | (cumg << 5) | cumb);
		}

		dstRowStartPtr += inDstWidth;
	}

	return UUcError_None;
}

static UUtError
IMiImage_Scale_Box_Down_ARGB8888(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	pixel;
	UUtUns32	cuma, cumr, cumg, cumb;
	UUtUns32	a;
	UUtUns32	numPixels;
	UUtUns32	sx, sy, ex, ey;

	UUtUns32*	dstRowStartPtr;
	UUtUns32*	srcRowStartPtr;
	UUtUns32*	dstPtr;
	UUtUns32*	srcPtr;
	UUtUns32	inSrcLength = 4 * inSrcWidth * inSrcHeight;

	/* the pixmap needs to be shrunk in x and y */

	/* compute the gradients to 16 bits of fractional precision */
	my = ((inSrcHeight - 1) << 16) / inDstHeight;
	mx = ((inSrcWidth - 1) << 16) / inDstWidth;

	/*
	 * We are going to traverse the destination map filtering each
	 * corresponding block in the source map down to a single pixel
	 */

	dstRowStartPtr = outDstData;

	for(desty = 0; desty < inDstHeight; desty++)
	{
		dstPtr = dstRowStartPtr;

		for(destx = 0; destx < inDstWidth; destx++)
		{
			/* compute the starting x, y and then the ending x, y in the source
			   pixmap space */
			sx = (destx * mx) >> 16;
			sy = (desty * my) >> 16;

			ex = (destx * mx + mx) >> 16;
			ey = (desty * my + my) >> 16;

			cuma = cumr = cumg = cumb = 0;

			srcRowStartPtr = (UUtUns32*)inSrcData + sy * inSrcWidth + sx;

			numPixels = (ex - sx + 1) * (ey - sy + 1);

			/* Filtering is inclusive so we iterate at least once */
			for(srcy = sy; srcy <= ey; srcy++)
			{
				UUmAssert((srcy >= 0) && (srcy < inSrcHeight));

				srcPtr = srcRowStartPtr;

				for(srcx = sx; srcx <= ex; srcx++)
				{
					UUmAssert((srcx >=0) && (srcx < inSrcWidth));
					UUmAssertInRange(srcPtr, inSrcData, inSrcLength);

					pixel = *srcPtr++;

					a = (pixel >> 24) & 0xFF;

					cuma += a;
					cumr += a * ((pixel >> 16) & 0xFF);
					cumg += a * ((pixel >> 8) & 0xFF);
					cumb += a * ((pixel) & 0xFF);
				}

				srcRowStartPtr += inSrcWidth;
			}

			if (cuma > 0)
			{
				cumr =  MUrUnsignedSmallFloat_To_Uns_Round((float) cumr / cuma);
				cumg =  MUrUnsignedSmallFloat_To_Uns_Round((float) cumg / cuma);
				cumb =  MUrUnsignedSmallFloat_To_Uns_Round((float) cumb / cuma);
			}

			cuma = MUrUnsignedSmallFloat_To_Uns_Round((float) cuma / numPixels);

			UUmAssert(cuma <= 0xff);
			UUmAssert(cumr <= 0xff);
			UUmAssert(cumg <= 0xff);
			UUmAssert(cumb <= 0xff);

			*dstPtr++ = (cuma << 24) | (cumr << 16) | (cumg << 8) | cumb;
		}

		dstRowStartPtr += inDstWidth;
	}

	return UUcError_None;
}

static UUtError
IMiImage_Scale_Box_Down_ARGB1555(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	pixel;
	UUtUns32	cuma, cumr, cumg, cumb;
	UUtUns32	a;
	UUtUns32	numPixels;
	UUtUns32	sx, sy, ex, ey;
	UUtUns32	srcLength = 2 * inSrcWidth * inSrcHeight;

	UUtUns16*	dstRowStartPtr;
	UUtUns16*	srcRowStartPtr;
	UUtUns16*	dstPtr;
	UUtUns16*	srcPtr;

	// Not really sure what exactly to do with the alpha at the edges here...

	/* the pixmap needs to be shrunk in x and y */

	/* compute the gradients to 16 bits of fractional precision */
	my = (inSrcHeight  << 16) / inDstHeight;
	mx = (inSrcWidth << 16) / inDstWidth;

	/*
	 * We are going to traverse the destination map filtering each
	 * corresponding block in the source map down to a single pixel
	 */

	dstRowStartPtr = outDstData;

	for(desty = 0; desty < inDstHeight; desty++)
	{
		dstPtr = dstRowStartPtr;

		for(destx = 0; destx < inDstWidth; destx++)
		{
			/* compute the starting x, y and then the ending x, y in the source
			   pixmap space */
			sx = (destx * mx) >> 16;
			sy = (desty * my) >> 16;

			ex = (destx * mx + mx) >> 16;
			ey = (desty * my + my) >> 16;

			cuma = cumr = cumg = cumb = 0;

			srcRowStartPtr = (UUtUns16*)inSrcData + sy * inSrcWidth + sx;

			numPixels = (ex - sx) * (ey - sy);

			UUmAssert(numPixels > 0);

			/* Filtering is inclusive so we iterate at least once */
			for(srcy = sy; srcy < ey; srcy++)
			{
				UUmAssert((srcy >= 0) && (srcy < inSrcHeight));
				srcPtr = srcRowStartPtr;

				for(srcx = sx; srcx < ex; srcx++)
				{
					UUmAssert((srcx >=0) && (srcx < inSrcWidth));
					UUmAssertInRange(srcPtr, inSrcData, srcLength);

					pixel = *srcPtr++;
					a = (pixel >> 15) & 01;

					cuma += a;
					if(a)
					{
						cumr += (pixel >> 10) & 0x1F;
						cumg += (pixel >> 5) & 0x1F;
						cumb += (pixel) & 0x1F;
					}
				}

				srcRowStartPtr += inSrcWidth;
			}

			if (cuma > 0) {
				cumr = MUrUnsignedSmallFloat_To_Uns_Round((float) cumr / cuma);
				cumg = MUrUnsignedSmallFloat_To_Uns_Round((float) cumg / cuma);
				cumb = MUrUnsignedSmallFloat_To_Uns_Round((float) cumb / cuma);
			}

			cuma = MUrUnsignedSmallFloat_To_Uns_Round((float) cuma / numPixels);

			UUmAssert(cuma <= 0x1);
			UUmAssert(cumr <= 0x1f);
			UUmAssert(cumg <= 0x1f);
			UUmAssert(cumb <= 0x1f);

			*dstPtr++ = (UUtUns16)((cuma << 15) | (cumr << 10) | (cumg << 5) | cumb);
		}

		dstRowStartPtr += inDstWidth;
	}

	return UUcError_None;
}

static UUtError
IMiImage_Scale_Box_Down_ARGB4444(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	pixel;
	UUtUns32	cuma, cumr, cumg, cumb;
	UUtUns32	a;
	UUtUns32	numPixels;
	UUtUns32	sx, sy, ex, ey;
	UUtUns32	inSrcLength = 2 * inSrcWidth * inSrcHeight;

	UUtUns16*	dstRowStartPtr;
	UUtUns16*	srcRowStartPtr;
	UUtUns16*	dstPtr;
	UUtUns16*	srcPtr;

	// Not really sure what exactly to do with the alpha at the edges here...

	/* the pixmap needs to be shrunk in x and y */

	/* compute the gradients to 16 bits of fractional precision */
	my = (inSrcHeight << 16) / inDstHeight;
	mx = (inSrcWidth << 16) / inDstWidth;

	/*
	 * We are going to traverse the destination map filtering each
	 * corresponding block in the source map down to a single pixel
	 */

	dstRowStartPtr = outDstData;

	for(desty = 0; desty < inDstHeight; desty++)
	{
		dstPtr = dstRowStartPtr;

		for(destx = 0; destx < inDstWidth; destx++)
		{
			/* compute the starting x, y and then the ending x, y in the source
			   pixmap space */
			sx = (destx * mx) >> 16;
			sy = (desty * my) >> 16;

			ex = (destx * mx + mx) >> 16;
			ey = (desty * my + my) >> 16;

			cuma = cumr = cumg = cumb = 0;

			srcRowStartPtr = (UUtUns16*)inSrcData + sy * inSrcWidth + sx;

			numPixels = (ex - sx) * (ey - sy);

			UUmAssert(numPixels > 0);

			for(srcy = sy; srcy < ey; srcy++)
			{
				UUmAssert((srcy >= 0) && (srcy < inSrcHeight));

				srcPtr = srcRowStartPtr;

				for(srcx = sx; srcx < ex; srcx++)
				{
					UUmAssert((srcx >=0) && (srcx < inSrcWidth));
					UUmAssertInRange(srcPtr, inSrcData, inSrcLength);

					pixel = *srcPtr++;

					a = (pixel >> 12) & 0xF;

					cuma += a;
					cumr += a * ((pixel >> 8) & 0xF);
					cumg += a * ((pixel >> 4) & 0xF);
					cumb += a * ((pixel) & 0xF);
				}

				srcRowStartPtr += inSrcWidth;
			}

			if (cuma > 0)
			{
				cumr = MUrUnsignedSmallFloat_To_Uns_Round((float) cumr / cuma);
				cumg = MUrUnsignedSmallFloat_To_Uns_Round((float) cumg / cuma);
				cumb = MUrUnsignedSmallFloat_To_Uns_Round((float) cumb / cuma);
			}

			cuma = MUrUnsignedSmallFloat_To_Uns_Round((float) cuma / numPixels);

			UUmAssert(cuma <= 0xf);
			UUmAssert(cumr <= 0xf);
			UUmAssert(cumg <= 0xf);
			UUmAssert(cumb <= 0xf);

			*dstPtr++ = (UUtUns16)((cuma << 12) | (cumr << 8) | (cumg << 4) | cumb);
		}

		dstRowStartPtr += inDstWidth;
	}

	return UUcError_None;
}

static UUtError
IMiImage_Scale_Box_Down_ARGB8888_IndependantAlpha(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	pixel;
	UUtUns32	cuma, cumr, cumg, cumb;
	UUtUns32	a;
	UUtUns32	numPixels;
	UUtUns32	sx, sy, ex, ey;

	UUtUns32*	dstRowStartPtr;
	UUtUns32*	srcRowStartPtr;
	UUtUns32*	dstPtr;
	UUtUns32*	srcPtr;
	UUtUns32	inSrcLength = 4 * inSrcWidth * inSrcHeight;

	/* the pixmap needs to be shrunk in x and y */

	/* compute the gradients to 16 bits of fractional precision */
	my = ((inSrcHeight - 1) << 16) / inDstHeight;
	mx = ((inSrcWidth - 1) << 16) / inDstWidth;

	/*
	 * We are going to traverse the destination map filtering each
	 * corresponding block in the source map down to a single pixel
	 */

	dstRowStartPtr = outDstData;

	for(desty = 0; desty < inDstHeight; desty++)
	{
		dstPtr = dstRowStartPtr;

		for(destx = 0; destx < inDstWidth; destx++)
		{
			/* compute the starting x, y and then the ending x, y in the source
			   pixmap space */
			sx = (destx * mx) >> 16;
			sy = (desty * my) >> 16;

			ex = (destx * mx + mx) >> 16;
			ey = (desty * my + my) >> 16;

			cuma = cumr = cumg = cumb = 0;

			srcRowStartPtr = (UUtUns32*)inSrcData + sy * inSrcWidth + sx;

			numPixels = (ex - sx + 1) * (ey - sy + 1);

			/* Filtering is inclusive so we iterate at least once */
			for(srcy = sy; srcy <= ey; srcy++)
			{
				UUmAssert((srcy >= 0) && (srcy < inSrcHeight));

				srcPtr = srcRowStartPtr;

				for(srcx = sx; srcx <= ex; srcx++)
				{
					UUmAssert((srcx >=0) && (srcx < inSrcWidth));
					UUmAssertInRange(srcPtr, inSrcData, inSrcLength);

					pixel = *srcPtr++;

					a = (pixel >> 24) & 0xFF;

					cuma += a;
					cumr += ((pixel >> 16) & 0xFF);
					cumg += ((pixel >> 8) & 0xFF);
					cumb += ((pixel) & 0xFF);
				}

				srcRowStartPtr += inSrcWidth;
			}

			cumr = MUrUnsignedSmallFloat_To_Uns_Round((float) cumr / numPixels);
			cumg = MUrUnsignedSmallFloat_To_Uns_Round((float) cumg / numPixels);
			cumb = MUrUnsignedSmallFloat_To_Uns_Round((float) cumb / numPixels);

			cuma = MUrUnsignedSmallFloat_To_Uns_Round((float) cuma / numPixels);

			UUmAssert(cuma <= 0xff);
			UUmAssert(cumr <= 0xff);
			UUmAssert(cumg <= 0xff);
			UUmAssert(cumb <= 0xff);

			*dstPtr++ = (cuma << 24) | (cumr << 16) | (cumg << 8) | cumb;
		}

		dstRowStartPtr += inDstWidth;
	}

	return UUcError_None;
}

static UUtError
IMiImage_Scale_Box_Down_ARGB1555_IndependantAlpha(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	pixel;
	UUtUns32	cuma, cumr, cumg, cumb;
	UUtUns32	a;
	UUtUns32	numPixels;
	UUtUns32	sx, sy, ex, ey;
	UUtUns32	srcLength = 2 * inSrcWidth * inSrcHeight;

	UUtUns16*	dstRowStartPtr;
	UUtUns16*	srcRowStartPtr;
	UUtUns16*	dstPtr;
	UUtUns16*	srcPtr;

	// Not really sure what exactly to do with the alpha at the edges here...

	/* the pixmap needs to be shrunk in x and y */

	/* compute the gradients to 16 bits of fractional precision */
	my = (inSrcHeight  << 16) / inDstHeight;
	mx = (inSrcWidth << 16) / inDstWidth;

	/*
	 * We are going to traverse the destination map filtering each
	 * corresponding block in the source map down to a single pixel
	 */

	dstRowStartPtr = outDstData;

	for(desty = 0; desty < inDstHeight; desty++)
	{
		dstPtr = dstRowStartPtr;

		for(destx = 0; destx < inDstWidth; destx++)
		{
			/* compute the starting x, y and then the ending x, y in the source
			   pixmap space */
			sx = (destx * mx) >> 16;
			sy = (desty * my) >> 16;

			ex = (destx * mx + mx) >> 16;
			ey = (desty * my + my) >> 16;

			cuma = cumr = cumg = cumb = 0;

			srcRowStartPtr = (UUtUns16*)inSrcData + sy * inSrcWidth + sx;

			numPixels = (ex - sx) * (ey - sy);

			UUmAssert(numPixels > 0);

			/* Filtering is inclusive so we iterate at least once */
			for(srcy = sy; srcy < ey; srcy++)
			{
				UUmAssert((srcy >= 0) && (srcy < inSrcHeight));
				srcPtr = srcRowStartPtr;

				for(srcx = sx; srcx < ex; srcx++)
				{
					UUmAssert((srcx >=0) && (srcx < inSrcWidth));
					UUmAssertInRange(srcPtr, inSrcData, srcLength);

					pixel = *srcPtr++;
					a = (pixel >> 15) & 01;

					cuma += a;
					cumr += (pixel >> 10) & 0x1F;
					cumg += (pixel >> 5) & 0x1F;
					cumb += (pixel) & 0x1F;
				}

				srcRowStartPtr += inSrcWidth;
			}

			cumr = MUrUnsignedSmallFloat_To_Uns_Round((float) cumr / numPixels);
			cumg = MUrUnsignedSmallFloat_To_Uns_Round((float) cumg / numPixels);
			cumb = MUrUnsignedSmallFloat_To_Uns_Round((float) cumb / numPixels);

			cuma = MUrUnsignedSmallFloat_To_Uns_Round((float) cuma / numPixels);

			UUmAssert(cuma <= 0x1);
			UUmAssert(cumr <= 0x1f);
			UUmAssert(cumg <= 0x1f);
			UUmAssert(cumb <= 0x1f);

			*dstPtr++ = (UUtUns16)((cuma << 15) | (cumr << 10) | (cumg << 5) | cumb);
		}

		dstRowStartPtr += inDstWidth;
	}

	return UUcError_None;
}

static UUtError
IMiImage_Scale_Box_Down_ARGB4444_IndependantAlpha(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	pixel;
	UUtUns32	cuma, cumr, cumg, cumb;
	UUtUns32	a;
	UUtUns32	numPixels;
	UUtUns32	sx, sy, ex, ey;
	UUtUns32	inSrcLength = 2 * inSrcWidth * inSrcHeight;

	UUtUns16*	dstRowStartPtr;
	UUtUns16*	srcRowStartPtr;
	UUtUns16*	dstPtr;
	UUtUns16*	srcPtr;

	// Not really sure what exactly to do with the alpha at the edges here...

	/* the pixmap needs to be shrunk in x and y */

	/* compute the gradients to 16 bits of fractional precision */
	my = (inSrcHeight << 16) / inDstHeight;
	mx = (inSrcWidth << 16) / inDstWidth;

	/*
	 * We are going to traverse the destination map filtering each
	 * corresponding block in the source map down to a single pixel
	 */

	dstRowStartPtr = outDstData;

	for(desty = 0; desty < inDstHeight; desty++)
	{
		dstPtr = dstRowStartPtr;

		for(destx = 0; destx < inDstWidth; destx++)
		{
			/* compute the starting x, y and then the ending x, y in the source
			   pixmap space */
			sx = (destx * mx) >> 16;
			sy = (desty * my) >> 16;

			ex = (destx * mx + mx) >> 16;
			ey = (desty * my + my) >> 16;

			cuma = cumr = cumg = cumb = 0;

			srcRowStartPtr = (UUtUns16*)inSrcData + sy * inSrcWidth + sx;

			numPixels = (ex - sx) * (ey - sy);

			UUmAssert(numPixels > 0);

			for(srcy = sy; srcy < ey; srcy++)
			{
				UUmAssert((srcy >= 0) && (srcy < inSrcHeight));

				srcPtr = srcRowStartPtr;

				for(srcx = sx; srcx < ex; srcx++)
				{
					UUmAssert((srcx >=0) && (srcx < inSrcWidth));
					UUmAssertInRange(srcPtr, inSrcData, inSrcLength);

					pixel = *srcPtr++;

					a = (pixel >> 12) & 0xF;

					cuma += a;
					cumr += ((pixel >> 8) & 0xF);
					cumg += ((pixel >> 4) & 0xF);
					cumb += ((pixel) & 0xF);
				}

				srcRowStartPtr += inSrcWidth;
			}

			cumr = MUrUnsignedSmallFloat_To_Uns_Round((float) cumr / numPixels);
			cumg = MUrUnsignedSmallFloat_To_Uns_Round((float) cumg / numPixels);
			cumb = MUrUnsignedSmallFloat_To_Uns_Round((float) cumb / numPixels);

			cuma = MUrUnsignedSmallFloat_To_Uns_Round((float) cuma / numPixels);

			UUmAssert(cuma <= 0xf);
			UUmAssert(cumr <= 0xf);
			UUmAssert(cumg <= 0xf);
			UUmAssert(cumb <= 0xf);

			*dstPtr++ = (UUtUns16)((cuma << 12) | (cumr << 8) | (cumg << 4) | cumb);
		}

		dstRowStartPtr += inDstWidth;
	}

	return UUcError_None;
}

#if INCLUDE_UNTESTED_STUFF
static UUtError
IMiImage_Scale_Box_Up_RGB888(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	/* THIS IS HIGHLY *UN*OPTIMIZED */
	/* the pixmap needs to be expanded in x and y */
	UUtUns32	rUp, gUp, bUp;
	UUtUns32	rUpLeft, gUpLeft, bUpLeft;
	UUtUns32	rUpRight, gUpRight, bUpRight;
	UUtUns32	rDown, gDown, bDown;
	UUtUns32	rDownLeft, gDownLeft, bDownLeft;
	UUtUns32	rDownRight, gDownRight, bDownRight;
	UUtUns32	rLeft, gLeft, bLeft;
	UUtUns32	rRight, gRight, bRight;
	UUtUns32	rCenter, gCenter, bCenter;
	UUtUns32	texelUp, texelDown, texelRight;
	UUtUns32	texelLeft, texelUpLeft, texelUpRight, texelDownLeft, texelDownRight;
	UUtUns32	rFinal, gFinal, bFinal;
	UUtUns32	blocky, blockx;
	UUtUns32	rUL, gUL, bUL;
	UUtUns32	rLL, gLL, bLL;
	UUtUns32	rUR, gUR, bUR;
	UUtUns32	rLR, gLR, bLR;
	UUtUns32	mrUp, mgUp, mbUp;
	UUtUns32	mrDown, mgDown, mbDown;
	UUtUns32	rUpInterp, gUpInterp, bUpInterp;
	UUtUns32	rDownInterp, gDownInterp, bDownInterp;
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	texel;
	UUtUns32	sx, sy, ex, ey;

	UUmAssert_Untested();

	/* compute the gradients to 16 bits of fractional precision */
	my = (inDstHeight << 16) / inSrcHeight;
	mx = (inDstWidth << 16) / inSrcWidth;

	/* We are going to traverse the source map expanding each pixel
	   into a block of pixels in the destination map */

	for(srcy = 0; srcy < inSrcHeight; srcy++)
	{
		for(srcx = 0; srcx < inSrcWidth; srcx++)
		{
			/* compute the starting x, y and then the ending x, y in the destination
			   pixmap space */
			sx = (srcx * mx) >> 16;
			sy = (srcy * my) >> 16;

			ex = (srcx * mx + mx) >> 16;
			ey = (srcy * my + my) >> 16;

			texel = *((UUtUns32*)inSrcData + inSrcWidth * srcy + srcx);
			rCenter = ((texel >> 16) & 0xFF) << 16;
			gCenter = ((texel >> 8) & 0xFF) << 16;
			bCenter = ((texel >> 0) & 0xFF) << 16;

			/* Read in the surrounding pixels */
			if(srcy > 0)
			{
				texelUp = *((UUtUns32*)inSrcData + inSrcWidth * (srcy-1) + srcx);

				if(srcx > 0)
				{
					texelUpLeft = *((UUtUns32*)inSrcData + inSrcWidth * (srcy-1) + (srcx-1));
				}
				else
				{
					texelUpLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelUpRight = *((UUtUns32*)inSrcData + inSrcWidth * (srcy-1) + (srcx+1));
				}
				else
				{
					texelUpRight = texel;
				}
			}
			else
			{
				texelUpRight = texelUpLeft = texelUp = texel;
			}
			rUp = ((texelUp >> 16) & 0xFF) << 16;
			gUp = ((texelUp >> 8) & 0xFF) << 16;
			bUp = ((texelUp >> 0) & 0xFF) << 16;
			rUpLeft = ((texelUpLeft >> 16) & 0xFF) << 16;
			gUpLeft = ((texelUpLeft >> 8) & 0xFF) << 16;
			bUpLeft = ((texelUpLeft >> 0) & 0xFF) << 16;
			rUpRight = ((texelUpRight >> 16) & 0xFF) << 16;
			gUpRight = ((texelUpRight >> 8) & 0xFF) << 16;
			bUpRight = ((texelUpRight >> 0) & 0xFF) << 16;

			if(srcy + 1 < inSrcHeight)
			{
				texelDown = *((UUtUns32*)inSrcData + inSrcWidth * (srcy+1) + srcx);

				if(srcx > 0)
				{
					texelDownLeft = *((UUtUns32*)inSrcData + inSrcWidth * (srcy+1) + (srcx-1));
				}
				else
				{
					texelDownLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelDownRight = *((UUtUns32*)inSrcData + inSrcWidth * (srcy+1) + (srcx+1));
				}
				else
				{
					texelDownRight = texel;
				}
			}
			else
			{
				texelDownRight = texelDownLeft = texelDown = texel;
			}

			rDown = ((texelDown >> 16) & 0xFF) << 16;
			gDown = ((texelDown >> 8) & 0xFF) << 16;
			bDown = ((texelDown >> 0) & 0xFF) << 16;
			rDownLeft = ((texelDownLeft >> 16) & 0xFF) << 16;
			gDownLeft = ((texelDownLeft >> 8) & 0xFF) << 16;
			bDownLeft = ((texelDownLeft >> 0) & 0xFF) << 16;
			rDownRight = ((texelDownRight >> 16) & 0xFF) << 16;
			gDownRight = ((texelDownRight >> 8) & 0xFF) << 16;
			bDownRight = ((texelDownRight >> 0) & 0xFF) << 16;

			if(srcx > 0)
			{
				texelLeft = *((UUtUns32*)inSrcData + inSrcWidth * srcy + (srcx-1));
			}
			else
			{
				texelLeft = texel;
			}
			rLeft = ((texelLeft >> 16) & 0xFF) << 16;
			gLeft = ((texelLeft >> 8) & 0xFF) << 16;
			bLeft = ((texelLeft >> 0) & 0xFF) << 16;

			if(srcx + 1 < inSrcWidth)
			{
				texelRight = *((UUtUns32*)inSrcData + inSrcWidth * srcy + (srcx+1));
			}
			else
			{
				texelRight = texel;
			}

			rRight = ((texelRight >> 16) & 0xFF) << 16;
			gRight = ((texelRight >> 8) & 0xFF) << 16;
			bRight = ((texelRight >> 0) & 0xFF) << 16;

			rUL = (rCenter + rUp + rLeft + rUpLeft) / 4;
			gUL = (gCenter + gUp + gLeft + gUpLeft) / 4;
			bUL = (bCenter + bUp + bLeft + bUpLeft) / 4;
			rUR = (rCenter + rUp + rRight + rUpRight) / 4;
			gUR = (gCenter + gUp + gRight + gUpRight) / 4;
			bUR = (bCenter + bUp + bRight + bUpRight) / 4;
			rLL = (rCenter + rDown + rLeft + rDownLeft) / 4;
			gLL = (gCenter + gDown + gLeft + gDownLeft) / 4;
			bLL = (bCenter + bDown + bLeft + bDownLeft) / 4;
			rLR = (rCenter + rDown + rRight + rDownRight) / 4;
			gLR = (gCenter + gDown + gRight + gDownRight) / 4;
			bLR = (bCenter + bDown + bRight + bDownRight) / 4;

			mrUp = (rUR - rUL) / (ex - sx);
			mgUp = (gUR - gUL) / (ex - sx);
			mbUp = (bUR - bUL) / (ex - sx);
			mrDown = (rLR - rLL) / (ex - sx);
			mgDown = (gLR - gLL) / (ex - sx);
			mbDown = (bLR - bLL) / (ex - sx);

			/* loop through each pixel in the dest block */
			for(desty = sy; desty < ey; desty++)
			{
				for(destx = sx; destx < ex; destx++)
				{
					blocky = desty - sy;
					blockx = destx - sx;

					/* interpolate along both horizontal lines */
					rUpInterp = mrUp * blockx + rUL;
					gUpInterp = mgUp * blockx + gUL;
					bUpInterp = mbUp * blockx + bUL;
					rDownInterp = mrDown * blockx + rLL;
					gDownInterp = mgDown * blockx + gLL;
					bDownInterp = mbDown * blockx + bLL;

					/* Now interpolate along the vertical line */
					rFinal = (((rDownInterp - rUpInterp) * blocky) / (ey - sy)) + rUpInterp;
					rFinal >>= 16;
					gFinal = (((gDownInterp - gUpInterp) * blocky) / (ey - sy)) + gUpInterp;
					gFinal >>= 16;
					bFinal = (((bDownInterp - bUpInterp) * blocky) / (ey - sy)) + bUpInterp;
					bFinal >>= 16;

					texel =
						(rFinal << 16) |
						(gFinal << 8) |
						(bFinal << 0);

					*((UUtUns32*)outDstData + inDstWidth * desty + destx) = texel;
				}
			}
		}
	}

	return UUcError_None;
}
#endif

#if INCLUDE_UNTESTED_STUFF
static UUtError
IMiImage_Scale_Box_Up_RGB555(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	/* THIS IS HIGHLY *UN*OPTIMIZED */
	/* the pixmap needs to be expanded in x and y */
	UUtUns32	rUp, gUp, bUp;
	UUtUns32	rUpLeft, gUpLeft, bUpLeft;
	UUtUns32	rUpRight, gUpRight, bUpRight;
	UUtUns32	rDown, gDown, bDown;
	UUtUns32	rDownLeft, gDownLeft, bDownLeft;
	UUtUns32	rDownRight, gDownRight, bDownRight;
	UUtUns32	rLeft, gLeft, bLeft;
	UUtUns32	rRight, gRight, bRight;
	UUtUns32	rCenter, gCenter, bCenter;
	UUtUns32	texelUp, texelDown, texelRight;
	UUtUns32	texelLeft, texelUpLeft, texelUpRight, texelDownLeft, texelDownRight;
	UUtUns32	rFinal, gFinal, bFinal;
	UUtUns32	blocky, blockx;
	UUtUns32	rUL, gUL, bUL;
	UUtUns32	rLL, gLL, bLL;
	UUtUns32	rUR, gUR, bUR;
	UUtUns32	rLR, gLR, bLR;
	UUtUns32	mrUp, mgUp, mbUp;
	UUtUns32	mrDown, mgDown, mbDown;
	UUtUns32	rUpInterp, gUpInterp, bUpInterp;
	UUtUns32	rDownInterp, gDownInterp, bDownInterp;
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	texel;
	UUtUns32	sx, sy, ex, ey;

	UUmAssert_Untested();

	/* compute the gradients to 16 bits of fractional precision */
	my = (inDstHeight << 16) / inSrcHeight;
	mx = (inDstWidth << 16) / inSrcWidth;

	/* We are going to traverse the source map expanding each pixel
	   into a block of pixels in the destination map */

	for(srcy = 0; srcy < inSrcHeight; srcy++)
	{
		for(srcx = 0; srcx < inSrcWidth; srcx++)
		{
			/* compute the starting x, y and then the ending x, y in the destination
			   pixmap space */
			sx = (srcx * mx) >> 16;
			sy = (srcy * my) >> 16;

			ex = (srcx * mx + mx) >> 16;
			ey = (srcy * my + my) >> 16;

			texel = *((UUtUns16*)inSrcData + inSrcWidth * srcy + srcx);
			rCenter = ((texel >> 10) & 0x1F) << 16;
			gCenter = ((texel >> 5) & 0x1F) << 16;
			bCenter = ((texel >> 0) & 0x1F) << 16;

			/* Read in the surrounding pixels */
			if(srcy > 0)
			{
				texelUp = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + srcx);

				if(srcx > 0)
				{
					texelUpLeft = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + (srcx-1));
				}
				else
				{
					texelUpLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelUpRight = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + (srcx+1));
				}
				else
				{
					texelUpRight = texel;
				}
			}
			else
			{
				texelUpRight = texelUpLeft = texelUp = texel;
			}
			rUp = ((texelUp >> 10) & 0x1F) << 16;
			gUp = ((texelUp >> 5) & 0x1F) << 16;
			bUp = ((texelUp >> 0) & 0x1F) << 16;
			rUpLeft = ((texelUpLeft >> 10) & 0x1F) << 16;
			gUpLeft = ((texelUpLeft >> 5) & 0x1F) << 16;
			bUpLeft = ((texelUpLeft >> 0) & 0x1F) << 16;
			rUpRight = ((texelUpRight >> 10) & 0x1F) << 16;
			gUpRight = ((texelUpRight >> 5) & 0x1F) << 16;
			bUpRight = ((texelUpRight >> 0) & 0x1F) << 16;

			if(srcy + 1 < inSrcHeight)
			{
				texelDown = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + srcx);

				if(srcx > 0)
				{
					texelDownLeft = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + (srcx-1));
				}
				else
				{
					texelDownLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelDownRight = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + (srcx+1));
				}
				else
				{
					texelDownRight = texel;
				}
			}
			else
			{
				texelDownRight = texelDownLeft = texelDown = texel;
			}

			rDown = ((texelDown >> 10) & 0x1F) << 16;
			gDown = ((texelDown >> 5) & 0x1F) << 16;
			bDown = ((texelDown >> 0) & 0x1F) << 16;
			rDownLeft = ((texelDownLeft >> 10) & 0x1F) << 16;
			gDownLeft = ((texelDownLeft >> 5) & 0x1F) << 16;
			bDownLeft = ((texelDownLeft >> 0) & 0x1F) << 16;
			rDownRight = ((texelDownRight >> 10) & 0x1F) << 16;
			gDownRight = ((texelDownRight >> 5) & 0x1F) << 16;
			bDownRight = ((texelDownRight >> 0) & 0x1F) << 16;

			if(srcx > 0)
			{
				texelLeft = *((UUtUns16*)inSrcData + inSrcWidth * srcy + (srcx-1));
			}
			else
			{
				texelLeft = texel;
			}
			rLeft = ((texelLeft >> 10) & 0x1F) << 16;
			gLeft = ((texelLeft >> 5) & 0x1F) << 16;
			bLeft = ((texelLeft >> 0) & 0x1F) << 16;

			if(srcx + 1 < inSrcWidth)
			{
				texelRight = *((UUtUns16*)inSrcData + inSrcWidth * srcy + (srcx+1));
			}
			else
			{
				texelRight = texel;
			}

			rRight = ((texelRight >> 10) & 0x1F) << 16;
			gRight = ((texelRight >> 5) & 0x1F) << 16;
			bRight = ((texelRight >> 0) & 0x1F) << 16;

			rUL = (rCenter + rUp + rLeft + rUpLeft) / 4;
			gUL = (gCenter + gUp + gLeft + gUpLeft) / 4;
			bUL = (bCenter + bUp + bLeft + bUpLeft) / 4;
			rUR = (rCenter + rUp + rRight + rUpRight) / 4;
			gUR = (gCenter + gUp + gRight + gUpRight) / 4;
			bUR = (bCenter + bUp + bRight + bUpRight) / 4;
			rLL = (rCenter + rDown + rLeft + rDownLeft) / 4;
			gLL = (gCenter + gDown + gLeft + gDownLeft) / 4;
			bLL = (bCenter + bDown + bLeft + bDownLeft) / 4;
			rLR = (rCenter + rDown + rRight + rDownRight) / 4;
			gLR = (gCenter + gDown + gRight + gDownRight) / 4;
			bLR = (bCenter + bDown + bRight + bDownRight) / 4;

			mrUp = (rUR - rUL) / (ex - sx);
			mgUp = (gUR - gUL) / (ex - sx);
			mbUp = (bUR - bUL) / (ex - sx);
			mrDown = (rLR - rLL) / (ex - sx);
			mgDown = (gLR - gLL) / (ex - sx);
			mbDown = (bLR - bLL) / (ex - sx);

			/* loop through each pixel in the dest block */
			for(desty = sy; desty < ey; desty++)
			{
				for(destx = sx; destx < ex; destx++)
				{
					blocky = desty - sy;
					blockx = destx - sx;

					/* interpolate along both horizontal lines */
					rUpInterp = mrUp * blockx + rUL;
					gUpInterp = mgUp * blockx + gUL;
					bUpInterp = mbUp * blockx + bUL;
					rDownInterp = mrDown * blockx + rLL;
					gDownInterp = mgDown * blockx + gLL;
					bDownInterp = mbDown * blockx + bLL;

					/* Now interpolate along the vertical line */
					rFinal = (((rDownInterp - rUpInterp) * blocky) / (ey - sy)) + rUpInterp;
					rFinal >>= 16;
					gFinal = (((gDownInterp - gUpInterp) * blocky) / (ey - sy)) + gUpInterp;
					gFinal >>= 16;
					bFinal = (((bDownInterp - bUpInterp) * blocky) / (ey - sy)) + bUpInterp;
					bFinal >>= 16;

					texel =
						(rFinal << 10) |
						(gFinal << 5) |
						(bFinal << 0);

					*((UUtUns16*)outDstData + inDstWidth * desty + destx) = (UUtUns16)texel;
				}
			}
		}
	}

	return UUcError_None;
}
#endif

#if INCLUDE_UNTESTED_STUFF
static UUtError
IMiImage_Scale_Box_Up_ARGB8888(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	/* THIS IS HIGHLY *UN*OPTIMIZED */
	/* the pixmap needs to be expanded in x and y */
	UUtUns32	aUp, rUp, gUp, bUp;
	UUtUns32	aUpLeft, rUpLeft, gUpLeft, bUpLeft;
	UUtUns32	aUpRight, rUpRight, gUpRight, bUpRight;
	UUtUns32	aDown, rDown, gDown, bDown;
	UUtUns32	aDownLeft, rDownLeft, gDownLeft, bDownLeft;
	UUtUns32	aDownRight, rDownRight, gDownRight, bDownRight;
	UUtUns32	aLeft, rLeft, gLeft, bLeft;
	UUtUns32	aRight, rRight, gRight, bRight;
	UUtUns32	aCenter, rCenter, gCenter, bCenter;
	UUtUns32	texelUp, texelDown, texelRight;
	UUtUns32	texelLeft, texelUpLeft, texelUpRight, texelDownLeft, texelDownRight;
	UUtUns32	aFinal, rFinal, gFinal, bFinal;
	UUtUns32	blocky, blockx;
	UUtUns32	aUL, rUL, gUL, bUL;
	UUtUns32	aLL, rLL, gLL, bLL;
	UUtUns32	aUR, rUR, gUR, bUR;
	UUtUns32	aLR, rLR, gLR, bLR;
	UUtUns32	maUp, mrUp, mgUp, mbUp;
	UUtUns32	maDown, mrDown, mgDown, mbDown;
	UUtUns32	aUpInterp, rUpInterp, gUpInterp, bUpInterp;
	UUtUns32	aDownInterp, rDownInterp, gDownInterp, bDownInterp;
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	texel;
	UUtUns32	sx, sy, ex, ey;

	UUmAssert_Untested();

	// Not really sure I am handling alpha entirely correct here...

	/* compute the gradients to 16 bits of fractional precision */
	my = (inDstHeight << 16) / inSrcHeight;
	mx = (inDstWidth << 16) / inSrcWidth;

	/* We are going to traverse the source map expanding each pixel
	   into a block of pixels in the destination map */

	for(srcy = 0; srcy < inSrcHeight; srcy++)
	{
		for(srcx = 0; srcx < inSrcWidth; srcx++)
		{
			/* compute the starting x, y and then the ending x, y in the destination
			   pixmap space */
			sx = (srcx * mx) >> 16;
			sy = (srcy * my) >> 16;

			ex = (srcx * mx + mx) >> 16;
			ey = (srcy * my + my) >> 16;

			texel = *((UUtUns32*)inSrcData + inSrcWidth * srcy + srcx);
			aCenter = ((texel >> 24) & 0xFF) << 16;
			rCenter = ((texel >> 16) & 0xFF) << 16;
			gCenter = ((texel >> 8) & 0xFF) << 16;
			bCenter = ((texel >> 0) & 0xFF) << 16;

			/* Read in the surrounding pixels */
			if(srcy > 0)
			{
				texelUp = *((UUtUns32*)inSrcData + inSrcWidth * (srcy-1) + srcx);

				if(srcx > 0)
				{
					texelUpLeft = *((UUtUns32*)inSrcData + inSrcWidth * (srcy-1) + (srcx-1));
				}
				else
				{
					texelUpLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelUpRight = *((UUtUns32*)inSrcData + inSrcWidth * (srcy-1) + (srcx+1));
				}
				else
				{
					texelUpRight = texel;
				}
			}
			else
			{
				texelUpRight = texelUpLeft = texelUp = texel;
			}

			aUp = ((texelUp >> 24) & 0xFF) << 16;
			if(aUp > 0)
			{
				rUp = ((texelUp >> 16) & 0xFF) << 16;
				gUp = ((texelUp >> 8) & 0xFF) << 16;
				bUp = ((texelUp >> 0) & 0xFF) << 16;
			}
			else
			{
				rUp = rCenter;
				gUp = gCenter;
				bUp = bCenter;
			}

			aUpLeft = ((texelUpLeft >> 24) & 0xFF) << 16;
			if(aUpLeft > 0)
			{
				rUpLeft = ((texelUpLeft >> 16) & 0xFF) << 16;
				gUpLeft = ((texelUpLeft >> 8) & 0xFF) << 16;
				bUpLeft = ((texelUpLeft >> 0) & 0xFF) << 16;
			}
			else
			{
				rUpLeft = rCenter;
				gUpLeft = gCenter;
				bUpLeft = bCenter;
			}

			aUpRight = ((texelUpRight >> 24) & 0xFF) << 16;
			if(aUpRight > 0)
			{
				rUpRight = ((texelUpRight >> 16) & 0xFF) << 16;
				gUpRight = ((texelUpRight >> 8) & 0xFF) << 16;
				bUpRight = ((texelUpRight >> 0) & 0xFF) << 16;
			}
			else
			{
				rUpRight = rCenter;
				gUpRight = gCenter;
				bUpRight = bCenter;
			}

			if(srcy + 1 < inSrcHeight)
			{
				texelDown = *((UUtUns32*)inSrcData + inSrcWidth * (srcy+1) + srcx);

				if(srcx > 0)
				{
					texelDownLeft = *((UUtUns32*)inSrcData + inSrcWidth * (srcy+1) + (srcx-1));
				}
				else
				{
					texelDownLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelDownRight = *((UUtUns32*)inSrcData + inSrcWidth * (srcy+1) + (srcx+1));
				}
				else
				{
					texelDownRight = texel;
				}
			}
			else
			{
				texelDownRight = texelDownLeft = texelDown = texel;
			}

			aDown = ((texelDown >> 24) & 0xFF) << 16;
			if(aDown > 0)
			{
				rDown = ((texelDown >> 16) & 0xFF) << 16;
				gDown = ((texelDown >> 8) & 0xFF) << 16;
				bDown = ((texelDown >> 0) & 0xFF) << 16;
			}
			else
			{
				rDown = rCenter;
				gDown = gCenter;
				bDown = bCenter;
			}

			aDownLeft = ((texelDownLeft >> 24) & 0xFF) << 16;
			if(aDownLeft > 0)
			{
				rDownLeft = ((texelDownLeft >> 16) & 0xFF) << 16;
				gDownLeft = ((texelDownLeft >> 8) & 0xFF) << 16;
				bDownLeft = ((texelDownLeft >> 0) & 0xFF) << 16;
			}
			else
			{
				rDownLeft = rCenter;
				gDownLeft = gCenter;
				bDownLeft = bCenter;
			}

			aDownRight = ((texelDownRight >> 24) & 0xFF) << 16;
			if(aDownRight > 0)
			{
				rDownRight = ((texelDownRight >> 16) & 0xFF) << 16;
				gDownRight = ((texelDownRight >> 8) & 0xFF) << 16;
				bDownRight = ((texelDownRight >> 0) & 0xFF) << 16;
			}
			else
			{
				rDownRight = rCenter;
				gDownRight = gCenter;
				bDownRight = bCenter;
			}

			if(srcx > 0)
			{
				texelLeft = *((UUtUns32*)inSrcData + inSrcWidth * srcy + (srcx-1));
			}
			else
			{
				texelLeft = texel;
			}

			aLeft = ((texelLeft >> 24) & 0xFF) << 16;
			if(aLeft > 0)
			{
				rLeft = ((texelLeft >> 16) & 0xFF) << 16;
				gLeft = ((texelLeft >> 8) & 0xFF) << 16;
				bLeft = ((texelLeft >> 0) & 0xFF) << 16;
			}
			else
			{
				rLeft = rCenter;
				gLeft = gCenter;
				bLeft = bCenter;
			}

			if(srcx + 1 < inSrcWidth)
			{
				texelRight = *((UUtUns32*)inSrcData + inSrcWidth * srcy + (srcx+1));
			}
			else
			{
				texelRight = texel;
			}

			aRight = ((texelRight >> 24) & 0xFF) << 16;
			if(aRight > 0)
			{
				rRight = ((texelRight >> 16) & 0xFF) << 16;
				gRight = ((texelRight >> 8) & 0xFF) << 16;
				bRight = ((texelRight >> 0) & 0xFF) << 16;
			}
			else
			{
				rRight = rCenter;
				gRight = gCenter;
				bRight = bCenter;
			}

			aUL = (aCenter + aUp + aLeft + aUpLeft) / 4;
			rUL = (rCenter + rUp + rLeft + rUpLeft) / 4;
			gUL = (gCenter + gUp + gLeft + gUpLeft) / 4;
			bUL = (bCenter + bUp + bLeft + bUpLeft) / 4;

			aUR = (aCenter + aUp + aRight + aUpRight) / 4;
			rUR = (rCenter + rUp + rRight + rUpRight) / 4;
			gUR = (gCenter + gUp + gRight + gUpRight) / 4;
			bUR = (bCenter + bUp + bRight + bUpRight) / 4;

			aLL = (aCenter + aDown + aLeft + aDownLeft) / 4;
			rLL = (rCenter + rDown + rLeft + rDownLeft) / 4;
			gLL = (gCenter + gDown + gLeft + gDownLeft) / 4;
			bLL = (bCenter + bDown + bLeft + bDownLeft) / 4;

			aLR = (aCenter + aDown + aRight + aDownRight) / 4;
			rLR = (rCenter + rDown + rRight + rDownRight) / 4;
			gLR = (gCenter + gDown + gRight + gDownRight) / 4;
			bLR = (bCenter + bDown + bRight + bDownRight) / 4;

			maUp = (aUR - aUL) / (ex - sx);
			mrUp = (rUR - rUL) / (ex - sx);
			mgUp = (gUR - gUL) / (ex - sx);
			mbUp = (bUR - bUL) / (ex - sx);

			maDown = (aLR - aLL) / (ex - sx);
			mrDown = (rLR - rLL) / (ex - sx);
			mgDown = (gLR - gLL) / (ex - sx);
			mbDown = (bLR - bLL) / (ex - sx);

			/* loop through each pixel in the dest block */
			for(desty = sy; desty < ey; desty++)
			{
				for(destx = sx; destx < ex; destx++)
				{
					blocky = desty - sy;
					blockx = destx - sx;

					/* interpolate along both horizontal lines */
					aUpInterp = maUp * blockx + aUL;
					rUpInterp = mrUp * blockx + rUL;
					gUpInterp = mgUp * blockx + gUL;
					bUpInterp = mbUp * blockx + bUL;

					aDownInterp = maDown * blockx + aLL;
					rDownInterp = mrDown * blockx + rLL;
					gDownInterp = mgDown * blockx + gLL;
					bDownInterp = mbDown * blockx + bLL;

					/* Now interpolate along the vertical line */
					aFinal = (((aDownInterp - aUpInterp) * blocky) / (ey - sy)) + aUpInterp;
					aFinal >>= 16;
					rFinal = (((rDownInterp - rUpInterp) * blocky) / (ey - sy)) + rUpInterp;
					rFinal >>= 16;
					gFinal = (((gDownInterp - gUpInterp) * blocky) / (ey - sy)) + gUpInterp;
					gFinal >>= 16;
					bFinal = (((bDownInterp - bUpInterp) * blocky) / (ey - sy)) + bUpInterp;
					bFinal >>= 16;

					texel =
						(aFinal << 24) |
						(rFinal << 16) |
						(gFinal << 8) |
						(bFinal << 0);

					*((UUtUns32*)outDstData + inDstWidth * desty + destx) = texel;
				}
			}
		}
	}

	return UUcError_None;
}
#endif

#if INCLUDE_UNTESTED_STUFF
static UUtError
IMiImage_Scale_Box_Up_ARGB1555(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	/* THIS IS HIGHLY *UN*OPTIMIZED */
	/* the pixmap needs to be expanded in x and y */
	UUtUns32	aUp, rUp, gUp, bUp;
	UUtUns32	aUpLeft, rUpLeft, gUpLeft, bUpLeft;
	UUtUns32	aUpRight, rUpRight, gUpRight, bUpRight;
	UUtUns32	aDown, rDown, gDown, bDown;
	UUtUns32	aDownLeft, rDownLeft, gDownLeft, bDownLeft;
	UUtUns32	aDownRight, rDownRight, gDownRight, bDownRight;
	UUtUns32	aLeft, rLeft, gLeft, bLeft;
	UUtUns32	aRight, rRight, gRight, bRight;
	UUtUns32	aCenter, rCenter, gCenter, bCenter;
	UUtUns32	texelUp, texelDown, texelRight;
	UUtUns32	texelLeft, texelUpLeft, texelUpRight, texelDownLeft, texelDownRight;
	UUtUns32	aFinal, rFinal, gFinal, bFinal;
	UUtUns32	blocky, blockx;
	UUtUns32	aUL, rUL, gUL, bUL;
	UUtUns32	aLL, rLL, gLL, bLL;
	UUtUns32	aUR, rUR, gUR, bUR;
	UUtUns32	aLR, rLR, gLR, bLR;
	UUtUns32	maUp, mrUp, mgUp, mbUp;
	UUtUns32	maDown, mrDown, mgDown, mbDown;
	UUtUns32	aUpInterp, rUpInterp, gUpInterp, bUpInterp;
	UUtUns32	aDownInterp, rDownInterp, gDownInterp, bDownInterp;
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	texel;
	UUtUns32	sx, sy, ex, ey;

	UUmAssert_Untested();

	// Not really sure I am handling alpha entirely correct here...

	/* compute the gradients to 16 bits of fractional precision */
	my = (inDstHeight << 16) / inSrcHeight;
	mx = (inDstWidth << 16) / inSrcWidth;

	/* We are going to traverse the source map expanding each pixel
	   into a block of pixels in the destination map */

	for(srcy = 0; srcy < inSrcHeight; srcy++)
	{
		for(srcx = 0; srcx < inSrcWidth; srcx++)
		{
			/* compute the starting x, y and then the ending x, y in the destination
			   pixmap space */
			sx = (srcx * mx) >> 16;
			sy = (srcy * my) >> 16;

			ex = (srcx * mx + mx) >> 16;
			ey = (srcy * my + my) >> 16;

			texel = *((UUtUns16*)inSrcData + inSrcWidth * srcy + srcx);
			aCenter = ((texel >> 15) & 0x01) << 16;
			rCenter = ((texel >> 10) & 0x1F) << 16;
			gCenter = ((texel >> 5) & 0x1F) << 16;
			bCenter = ((texel >> 0) & 0x1F) << 16;

			/* Read in the surrounding pixels */
			if(srcy > 0)
			{
				texelUp = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + srcx);

				if(srcx > 0)
				{
					texelUpLeft = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + (srcx-1));
				}
				else
				{
					texelUpLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelUpRight = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + (srcx+1));
				}
				else
				{
					texelUpRight = texel;
				}
			}
			else
			{
				texelUpRight = texelUpLeft = texelUp = texel;
			}

			aUp = ((texelUp >> 15) & 0x01) << 16;
			if(aUp > 0)
			{
				rUp = ((texelUp >> 10) & 0x1F) << 16;
				gUp = ((texelUp >> 5) & 0x1F) << 16;
				bUp = ((texelUp >> 0) & 0x1F) << 16;
			}
			else
			{
				rUp = rCenter;
				gUp = gCenter;
				bUp = bCenter;
			}

			aUpLeft = ((texelUpLeft >> 15) & 0x01) << 16;
			if(aUpLeft > 0)
			{
				rUpLeft = ((texelUpLeft >> 10) & 0x1F) << 16;
				gUpLeft = ((texelUpLeft >> 5) & 0x1F) << 16;
				bUpLeft = ((texelUpLeft >> 0) & 0x1F) << 16;
			}
			else
			{
				rUpLeft = rCenter;
				gUpLeft = gCenter;
				bUpLeft = bCenter;
			}

			aUpRight = ((texelUpRight >> 15) & 0x01) << 16;
			if(aUpRight > 0)
			{
				rUpRight = ((texelUpRight >> 10) & 0x1F) << 16;
				gUpRight = ((texelUpRight >> 5) & 0x1F) << 16;
				bUpRight = ((texelUpRight >> 0) & 0x1F) << 16;
			}
			else
			{
				rUpRight = rCenter;
				gUpRight = gCenter;
				bUpRight = bCenter;
			}

			if(srcy + 1 < inSrcHeight)
			{
				texelDown = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + srcx);

				if(srcx > 0)
				{
					texelDownLeft = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + (srcx-1));
				}
				else
				{
					texelDownLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelDownRight = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + (srcx+1));
				}
				else
				{
					texelDownRight = texel;
				}
			}
			else
			{
				texelDownRight = texelDownLeft = texelDown = texel;
			}

			aDown = ((texelDown >> 15) & 0x01) << 16;
			if(aDown > 0)
			{
				rDown = ((texelDown >> 10) & 0x1F) << 16;
				gDown = ((texelDown >> 5) & 0x1F) << 16;
				bDown = ((texelDown >> 0) & 0x1F) << 16;
			}
			else
			{
				rDown = rCenter;
				gDown = gCenter;
				bDown = bCenter;
			}

			aDownLeft = ((texelDownLeft >> 15) & 0x01) << 16;
			if(aDownLeft > 0)
			{
				rDownLeft = ((texelDownLeft >> 10) & 0x1F) << 16;
				gDownLeft = ((texelDownLeft >> 5) & 0x1F) << 16;
				bDownLeft = ((texelDownLeft >> 0) & 0x1F) << 16;
			}
			else
			{
				rDownLeft = rCenter;
				gDownLeft = gCenter;
				bDownLeft = bCenter;
			}

			aDownRight = ((texelDownRight >> 15) & 0x01) << 16;
			if(aDownRight > 0)
			{
				rDownRight = ((texelDownRight >> 10) & 0x1F) << 16;
				gDownRight = ((texelDownRight >> 5) & 0x1F) << 16;
				bDownRight = ((texelDownRight >> 0) & 0x1F) << 16;
			}
			else
			{
				rDownRight = rCenter;
				gDownRight = gCenter;
				bDownRight = bCenter;
			}

			if(srcx > 0)
			{
				texelLeft = *((UUtUns16*)inSrcData + inSrcWidth * srcy + (srcx-1));
			}
			else
			{
				texelLeft = texel;
			}

			aLeft = ((texelLeft >> 15) & 0x01) << 16;
			if(aLeft > 0)
			{
				rLeft = ((texelLeft >> 10) & 0x1F) << 16;
				gLeft = ((texelLeft >> 5) & 0x1F) << 16;
				bLeft = ((texelLeft >> 0) & 0x1F) << 16;
			}
			else
			{
				rLeft = rCenter;
				gLeft = gCenter;
				bLeft = bCenter;
			}

			if(srcx + 1 < inSrcWidth)
			{
				texelRight = *((UUtUns16*)inSrcData + inSrcWidth * srcy + (srcx+1));
			}
			else
			{
				texelRight = texel;
			}

			aRight = ((texelRight >> 15) & 0x01) << 16;
			if(aRight > 0)
			{
				rRight = ((texelRight >> 10) & 0x1F) << 16;
				gRight = ((texelRight >> 5) & 0x1F) << 16;
				bRight = ((texelRight >> 0) & 0x1F) << 16;
			}
			else
			{
				rRight = rCenter;
				gRight = gCenter;
				bRight = bCenter;
			}

			aUL = (aCenter + aUp + aLeft + aUpLeft) / 4;
			rUL = (rCenter + rUp + rLeft + rUpLeft) / 4;
			gUL = (gCenter + gUp + gLeft + gUpLeft) / 4;
			bUL = (bCenter + bUp + bLeft + bUpLeft) / 4;

			aUR = (aCenter + aUp + aRight + aUpRight) / 4;
			rUR = (rCenter + rUp + rRight + rUpRight) / 4;
			gUR = (gCenter + gUp + gRight + gUpRight) / 4;
			bUR = (bCenter + bUp + bRight + bUpRight) / 4;

			aLL = (aCenter + aDown + aLeft + aDownLeft) / 4;
			rLL = (rCenter + rDown + rLeft + rDownLeft) / 4;
			gLL = (gCenter + gDown + gLeft + gDownLeft) / 4;
			bLL = (bCenter + bDown + bLeft + bDownLeft) / 4;

			aLR = (aCenter + aDown + aRight + aDownRight) / 4;
			rLR = (rCenter + rDown + rRight + rDownRight) / 4;
			gLR = (gCenter + gDown + gRight + gDownRight) / 4;
			bLR = (bCenter + bDown + bRight + bDownRight) / 4;

			maUp = (aUR - aUL) / (ex - sx);
			mrUp = (rUR - rUL) / (ex - sx);
			mgUp = (gUR - gUL) / (ex - sx);
			mbUp = (bUR - bUL) / (ex - sx);

			maDown = (aLR - aLL) / (ex - sx);
			mrDown = (rLR - rLL) / (ex - sx);
			mgDown = (gLR - gLL) / (ex - sx);
			mbDown = (bLR - bLL) / (ex - sx);

			/* loop through each pixel in the dest block */
			for(desty = sy; desty < ey; desty++)
			{
				for(destx = sx; destx < ex; destx++)
				{
					blocky = desty - sy;
					blockx = destx - sx;

					/* interpolate along both horizontal lines */
					aUpInterp = maUp * blockx + aUL;
					rUpInterp = mrUp * blockx + rUL;
					gUpInterp = mgUp * blockx + gUL;
					bUpInterp = mbUp * blockx + bUL;

					aDownInterp = maDown * blockx + aLL;
					rDownInterp = mrDown * blockx + rLL;
					gDownInterp = mgDown * blockx + gLL;
					bDownInterp = mbDown * blockx + bLL;

					/* Now interpolate along the vertical line */
					aFinal = (((aDownInterp - aUpInterp) * blocky) / (ey - sy)) + aUpInterp;
					aFinal >>= 16;
					rFinal = (((rDownInterp - rUpInterp) * blocky) / (ey - sy)) + rUpInterp;
					rFinal >>= 16;
					gFinal = (((gDownInterp - gUpInterp) * blocky) / (ey - sy)) + gUpInterp;
					gFinal >>= 16;
					bFinal = (((bDownInterp - bUpInterp) * blocky) / (ey - sy)) + bUpInterp;
					bFinal >>= 16;

					texel =
						(aFinal << 15) |
						(rFinal << 10) |
						(gFinal << 5) |
						(bFinal << 0);

					*((UUtUns16*)outDstData + inDstWidth * desty + destx) = (UUtUns16)texel;
				}
			}
		}
	}

	return UUcError_None;
}
#endif

static UUtError
IMiImage_Scale_Box_Up_ARGB4444(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	/* THIS IS HIGHLY *UN*OPTIMIZED */
	/* the pixmap needs to be expanded in x and y */
	UUtUns32	aUp, rUp, gUp, bUp;
	UUtUns32	aUpLeft, rUpLeft, gUpLeft, bUpLeft;
	UUtUns32	aUpRight, rUpRight, gUpRight, bUpRight;
	UUtUns32	aDown, rDown, gDown, bDown;
	UUtUns32	aDownLeft, rDownLeft, gDownLeft, bDownLeft;
	UUtUns32	aDownRight, rDownRight, gDownRight, bDownRight;
	UUtUns32	aLeft, rLeft, gLeft, bLeft;
	UUtUns32	aRight, rRight, gRight, bRight;
	UUtUns32	aCenter, rCenter, gCenter, bCenter;
	UUtUns32	texelUp, texelDown, texelRight;
	UUtUns32	texelLeft, texelUpLeft, texelUpRight, texelDownLeft, texelDownRight;
	UUtUns32	aFinal, rFinal, gFinal, bFinal;
	UUtUns32	blocky, blockx;
	UUtUns32	aUL, rUL, gUL, bUL;
	UUtUns32	aLL, rLL, gLL, bLL;
	UUtUns32	aUR, rUR, gUR, bUR;
	UUtUns32	aLR, rLR, gLR, bLR;
	UUtUns32	maUp, mrUp, mgUp, mbUp;
	UUtUns32	maDown, mrDown, mgDown, mbDown;
	UUtUns32	aUpInterp, rUpInterp, gUpInterp, bUpInterp;
	UUtUns32	aDownInterp, rDownInterp, gDownInterp, bDownInterp;
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	texel;
	UUtUns32	sx, sy, ex, ey;

	//UUmAssert_Untested();

	// Not really sure I am handling alpha entirely correct here...

	/* compute the gradients to 16 bits of fractional precision */
	my = (inDstHeight << 16) / inSrcHeight;
	mx = (inDstWidth << 16) / inSrcWidth;

	/* We are going to traverse the source map expanding each pixel
	   into a block of pixels in the destination map */

	for(srcy = 0; srcy < inSrcHeight; srcy++)
	{
		for(srcx = 0; srcx < inSrcWidth; srcx++)
		{
			/* compute the starting x, y and then the ending x, y in the destination
			   pixmap space */
			sx = (srcx * mx) >> 16;
			sy = (srcy * my) >> 16;

			ex = (srcx * mx + mx) >> 16;
			ey = (srcy * my + my) >> 16;

			texel = *((UUtUns16*)inSrcData + inSrcWidth * srcy + srcx);
			aCenter = ((texel >> 12) & 0xF) << 16;
			rCenter = ((texel >> 8) & 0xF) << 16;
			gCenter = ((texel >> 4) & 0xF) << 16;
			bCenter = ((texel >> 0) & 0xF) << 16;

			/* Read in the surrounding pixels */
			if(srcy > 0)
			{
				texelUp = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + srcx);

				if(srcx > 0)
				{
					texelUpLeft = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + (srcx-1));
				}
				else
				{
					texelUpLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelUpRight = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + (srcx+1));
				}
				else
				{
					texelUpRight = texel;
				}
			}
			else
			{
				texelUpRight = texelUpLeft = texelUp = texel;
			}

			aUp = ((texelUp >> 12) & 0xF) << 16;
			if(aUp > 0)
			{
				rUp = ((texelUp >> 8) & 0xF) << 16;
				gUp = ((texelUp >> 4) & 0xF) << 16;
				bUp = ((texelUp >> 0) & 0xF) << 16;
			}
			else
			{
				rUp = rCenter;
				gUp = gCenter;
				bUp = bCenter;
			}

			aUpLeft = ((texelUpLeft >> 12) & 0xF) << 16;
			if(aUpLeft > 0)
			{
				rUpLeft = ((texelUpLeft >> 8) & 0xF) << 16;
				gUpLeft = ((texelUpLeft >> 4) & 0xF) << 16;
				bUpLeft = ((texelUpLeft >> 0) & 0xF) << 16;
			}
			else
			{
				rUpLeft = rCenter;
				gUpLeft = gCenter;
				bUpLeft = bCenter;
			}

			aUpRight = ((texelUpRight >> 12) & 0xFF) << 16;
			if(aUpRight > 0)
			{
				rUpRight = ((texelUpRight >> 8) & 0xFF) << 16;
				gUpRight = ((texelUpRight >> 4) & 0xFF) << 16;
				bUpRight = ((texelUpRight >> 0) & 0xFF) << 16;
			}
			else
			{
				rUpRight = rCenter;
				gUpRight = gCenter;
				bUpRight = bCenter;
			}

			if(srcy + 1 < inSrcHeight)
			{
				texelDown = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + srcx);

				if(srcx > 0)
				{
					texelDownLeft = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + (srcx-1));
				}
				else
				{
					texelDownLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelDownRight = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + (srcx+1));
				}
				else
				{
					texelDownRight = texel;
				}
			}
			else
			{
				texelDownRight = texelDownLeft = texelDown = texel;
			}

			aDown = ((texelDown >> 12) & 0xF) << 16;
			if(aDown > 0)
			{
				rDown = ((texelDown >> 8) & 0xF) << 16;
				gDown = ((texelDown >> 4) & 0xF) << 16;
				bDown = ((texelDown >> 0) & 0xF) << 16;
			}
			else
			{
				rDown = rCenter;
				gDown = gCenter;
				bDown = bCenter;
			}

			aDownLeft = ((texelDownLeft >> 12) & 0xF) << 16;
			if(aDownLeft > 0)
			{
				rDownLeft = ((texelDownLeft >> 8) & 0xF) << 16;
				gDownLeft = ((texelDownLeft >> 4) & 0xF) << 16;
				bDownLeft = ((texelDownLeft >> 0) & 0xF) << 16;
			}
			else
			{
				rDownLeft = rCenter;
				gDownLeft = gCenter;
				bDownLeft = bCenter;
			}

			aDownRight = ((texelDownRight >> 12) & 0xF) << 16;
			if(aDownRight > 0)
			{
				rDownRight = ((texelDownRight >> 8) & 0xF) << 16;
				gDownRight = ((texelDownRight >> 4) & 0xF) << 16;
				bDownRight = ((texelDownRight >> 0) & 0xF) << 16;
			}
			else
			{
				rDownRight = rCenter;
				gDownRight = gCenter;
				bDownRight = bCenter;
			}

			if(srcx > 0)
			{
				texelLeft = *((UUtUns16*)inSrcData + inSrcWidth * srcy + (srcx-1));
			}
			else
			{
				texelLeft = texel;
			}

			aLeft = ((texelLeft >> 12) & 0xF) << 16;
			if(aLeft > 0)
			{
				rLeft = ((texelLeft >> 8) & 0xF) << 16;
				gLeft = ((texelLeft >> 4) & 0xF) << 16;
				bLeft = ((texelLeft >> 0) & 0xF) << 16;
			}
			else
			{
				rLeft = rCenter;
				gLeft = gCenter;
				bLeft = bCenter;
			}

			if(srcx + 1 < inSrcWidth)
			{
				texelRight = *((UUtUns16*)inSrcData + inSrcWidth * srcy + (srcx+1));
			}
			else
			{
				texelRight = texel;
			}

			aRight = ((texelRight >> 12) & 0xF) << 16;
			if(aRight > 0)
			{
				rRight = ((texelRight >> 8) & 0xF) << 16;
				gRight = ((texelRight >> 4) & 0xF) << 16;
				bRight = ((texelRight >> 0) & 0xF) << 16;
			}
			else
			{
				rRight = rCenter;
				gRight = gCenter;
				bRight = bCenter;
			}

			aUL = (aCenter + aUp + aLeft + aUpLeft) / 4;
			rUL = (rCenter + rUp + rLeft + rUpLeft) / 4;
			gUL = (gCenter + gUp + gLeft + gUpLeft) / 4;
			bUL = (bCenter + bUp + bLeft + bUpLeft) / 4;

			aUR = (aCenter + aUp + aRight + aUpRight) / 4;
			rUR = (rCenter + rUp + rRight + rUpRight) / 4;
			gUR = (gCenter + gUp + gRight + gUpRight) / 4;
			bUR = (bCenter + bUp + bRight + bUpRight) / 4;

			aLL = (aCenter + aDown + aLeft + aDownLeft) / 4;
			rLL = (rCenter + rDown + rLeft + rDownLeft) / 4;
			gLL = (gCenter + gDown + gLeft + gDownLeft) / 4;
			bLL = (bCenter + bDown + bLeft + bDownLeft) / 4;

			aLR = (aCenter + aDown + aRight + aDownRight) / 4;
			rLR = (rCenter + rDown + rRight + rDownRight) / 4;
			gLR = (gCenter + gDown + gRight + gDownRight) / 4;
			bLR = (bCenter + bDown + bRight + bDownRight) / 4;

			maUp = (aUR - aUL) / (ex - sx);
			mrUp = (rUR - rUL) / (ex - sx);
			mgUp = (gUR - gUL) / (ex - sx);
			mbUp = (bUR - bUL) / (ex - sx);

			maDown = (aLR - aLL) / (ex - sx);
			mrDown = (rLR - rLL) / (ex - sx);
			mgDown = (gLR - gLL) / (ex - sx);
			mbDown = (bLR - bLL) / (ex - sx);

			/* loop through each pixel in the dest block */
			for(desty = sy; desty < ey; desty++)
			{
				for(destx = sx; destx < ex; destx++)
				{
					blocky = desty - sy;
					blockx = destx - sx;

					/* interpolate along both horizontal lines */
					aUpInterp = maUp * blockx + aUL;
					rUpInterp = mrUp * blockx + rUL;
					gUpInterp = mgUp * blockx + gUL;
					bUpInterp = mbUp * blockx + bUL;

					aDownInterp = maDown * blockx + aLL;
					rDownInterp = mrDown * blockx + rLL;
					gDownInterp = mgDown * blockx + gLL;
					bDownInterp = mbDown * blockx + bLL;

					/* Now interpolate along the vertical line */
					aFinal = (((aDownInterp - aUpInterp) * blocky) / (ey - sy)) + aUpInterp;
					aFinal >>= 16;
					rFinal = (((rDownInterp - rUpInterp) * blocky) / (ey - sy)) + rUpInterp;
					rFinal >>= 16;
					gFinal = (((gDownInterp - gUpInterp) * blocky) / (ey - sy)) + gUpInterp;
					gFinal >>= 16;
					bFinal = (((bDownInterp - bUpInterp) * blocky) / (ey - sy)) + bUpInterp;
					bFinal >>= 16;

					texel =
						(aFinal << 12) |
						(rFinal << 8) |
						(gFinal << 4) |
						(bFinal << 0);

					*((UUtUns16*)outDstData + inDstWidth * desty + destx) = (UUtUns16)texel;
				}
			}
		}
	}

	return UUcError_None;
}

#if INCLUDE_UNTESTED_STUFF
static UUtError
IMiImage_Scale_Box_Up_ARGB8888_IndependantAlpha(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	/* THIS IS HIGHLY *UN*OPTIMIZED */
	/* the pixmap needs to be expanded in x and y */
	UUtUns32	aUp, rUp, gUp, bUp;
	UUtUns32	aUpLeft, rUpLeft, gUpLeft, bUpLeft;
	UUtUns32	aUpRight, rUpRight, gUpRight, bUpRight;
	UUtUns32	aDown, rDown, gDown, bDown;
	UUtUns32	aDownLeft, rDownLeft, gDownLeft, bDownLeft;
	UUtUns32	aDownRight, rDownRight, gDownRight, bDownRight;
	UUtUns32	aLeft, rLeft, gLeft, bLeft;
	UUtUns32	aRight, rRight, gRight, bRight;
	UUtUns32	aCenter, rCenter, gCenter, bCenter;
	UUtUns32	texelUp, texelDown, texelRight;
	UUtUns32	texelLeft, texelUpLeft, texelUpRight, texelDownLeft, texelDownRight;
	UUtUns32	aFinal, rFinal, gFinal, bFinal;
	UUtUns32	blocky, blockx;
	UUtUns32	aUL, rUL, gUL, bUL;
	UUtUns32	aLL, rLL, gLL, bLL;
	UUtUns32	aUR, rUR, gUR, bUR;
	UUtUns32	aLR, rLR, gLR, bLR;
	UUtUns32	maUp, mrUp, mgUp, mbUp;
	UUtUns32	maDown, mrDown, mgDown, mbDown;
	UUtUns32	aUpInterp, rUpInterp, gUpInterp, bUpInterp;
	UUtUns32	aDownInterp, rDownInterp, gDownInterp, bDownInterp;
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	texel;
	UUtUns32	sx, sy, ex, ey;

	UUmAssert_Untested();

	// Not really sure I am handling alpha entirely correct here...

	/* compute the gradients to 16 bits of fractional precision */
	my = (inDstHeight << 16) / inSrcHeight;
	mx = (inDstWidth << 16) / inSrcWidth;

	/* We are going to traverse the source map expanding each pixel
	   into a block of pixels in the destination map */

	for(srcy = 0; srcy < inSrcHeight; srcy++)
	{
		for(srcx = 0; srcx < inSrcWidth; srcx++)
		{
			/* compute the starting x, y and then the ending x, y in the destination
			   pixmap space */
			sx = (srcx * mx) >> 16;
			sy = (srcy * my) >> 16;

			ex = (srcx * mx + mx) >> 16;
			ey = (srcy * my + my) >> 16;

			texel = *((UUtUns32*)inSrcData + inSrcWidth * srcy + srcx);
			aCenter = ((texel >> 24) & 0xFF) << 16;
			rCenter = ((texel >> 16) & 0xFF) << 16;
			gCenter = ((texel >> 8) & 0xFF) << 16;
			bCenter = ((texel >> 0) & 0xFF) << 16;

			/* Read in the surrounding pixels */
			if(srcy > 0)
			{
				texelUp = *((UUtUns32*)inSrcData + inSrcWidth * (srcy-1) + srcx);

				if(srcx > 0)
				{
					texelUpLeft = *((UUtUns32*)inSrcData + inSrcWidth * (srcy-1) + (srcx-1));
				}
				else
				{
					texelUpLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelUpRight = *((UUtUns32*)inSrcData + inSrcWidth * (srcy-1) + (srcx+1));
				}
				else
				{
					texelUpRight = texel;
				}
			}
			else
			{
				texelUpRight = texelUpLeft = texelUp = texel;
			}

			aUp = ((texelUp >> 24) & 0xFF) << 16;
			rUp = ((texelUp >> 16) & 0xFF) << 16;
			gUp = ((texelUp >> 8) & 0xFF) << 16;
			bUp = ((texelUp >> 0) & 0xFF) << 16;

			aUpLeft = ((texelUpLeft >> 24) & 0xFF) << 16;
			rUpLeft = ((texelUpLeft >> 16) & 0xFF) << 16;
			gUpLeft = ((texelUpLeft >> 8) & 0xFF) << 16;
			bUpLeft = ((texelUpLeft >> 0) & 0xFF) << 16;

			aUpRight = ((texelUpRight >> 24) & 0xFF) << 16;
			rUpRight = ((texelUpRight >> 16) & 0xFF) << 16;
			gUpRight = ((texelUpRight >> 8) & 0xFF) << 16;
			bUpRight = ((texelUpRight >> 0) & 0xFF) << 16;

			if(srcy + 1 < inSrcHeight)
			{
				texelDown = *((UUtUns32*)inSrcData + inSrcWidth * (srcy+1) + srcx);

				if(srcx > 0)
				{
					texelDownLeft = *((UUtUns32*)inSrcData + inSrcWidth * (srcy+1) + (srcx-1));
				}
				else
				{
					texelDownLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelDownRight = *((UUtUns32*)inSrcData + inSrcWidth * (srcy+1) + (srcx+1));
				}
				else
				{
					texelDownRight = texel;
				}
			}
			else
			{
				texelDownRight = texelDownLeft = texelDown = texel;
			}

			aDown = ((texelDown >> 24) & 0xFF) << 16;
			if(aDown > 0)
			{
				rDown = ((texelDown >> 16) & 0xFF) << 16;
				gDown = ((texelDown >> 8) & 0xFF) << 16;
				bDown = ((texelDown >> 0) & 0xFF) << 16;
			}
			else
			{
				rDown = rCenter;
				gDown = gCenter;
				bDown = bCenter;
			}

			aDownLeft = ((texelDownLeft >> 24) & 0xFF) << 16;
			if(aDownLeft > 0)
			{
				rDownLeft = ((texelDownLeft >> 16) & 0xFF) << 16;
				gDownLeft = ((texelDownLeft >> 8) & 0xFF) << 16;
				bDownLeft = ((texelDownLeft >> 0) & 0xFF) << 16;
			}
			else
			{
				rDownLeft = rCenter;
				gDownLeft = gCenter;
				bDownLeft = bCenter;
			}

			aDownRight = ((texelDownRight >> 24) & 0xFF) << 16;
			if(aDownRight > 0)
			{
				rDownRight = ((texelDownRight >> 16) & 0xFF) << 16;
				gDownRight = ((texelDownRight >> 8) & 0xFF) << 16;
				bDownRight = ((texelDownRight >> 0) & 0xFF) << 16;
			}
			else
			{
				rDownRight = rCenter;
				gDownRight = gCenter;
				bDownRight = bCenter;
			}

			if(srcx > 0)
			{
				texelLeft = *((UUtUns32*)inSrcData + inSrcWidth * srcy + (srcx-1));
			}
			else
			{
				texelLeft = texel;
			}

			aLeft = ((texelLeft >> 24) & 0xFF) << 16;
			rLeft = ((texelLeft >> 16) & 0xFF) << 16;
			gLeft = ((texelLeft >> 8) & 0xFF) << 16;
			bLeft = ((texelLeft >> 0) & 0xFF) << 16;

			if(srcx + 1 < inSrcWidth)
			{
				texelRight = *((UUtUns32*)inSrcData + inSrcWidth * srcy + (srcx+1));
			}
			else
			{
				texelRight = texel;
			}

			aRight = ((texelRight >> 24) & 0xFF) << 16;
			rRight = ((texelRight >> 16) & 0xFF) << 16;
			gRight = ((texelRight >> 8) & 0xFF) << 16;
			bRight = ((texelRight >> 0) & 0xFF) << 16;

			aUL = (aCenter + aUp + aLeft + aUpLeft) / 4;
			rUL = (rCenter + rUp + rLeft + rUpLeft) / 4;
			gUL = (gCenter + gUp + gLeft + gUpLeft) / 4;
			bUL = (bCenter + bUp + bLeft + bUpLeft) / 4;

			aUR = (aCenter + aUp + aRight + aUpRight) / 4;
			rUR = (rCenter + rUp + rRight + rUpRight) / 4;
			gUR = (gCenter + gUp + gRight + gUpRight) / 4;
			bUR = (bCenter + bUp + bRight + bUpRight) / 4;

			aLL = (aCenter + aDown + aLeft + aDownLeft) / 4;
			rLL = (rCenter + rDown + rLeft + rDownLeft) / 4;
			gLL = (gCenter + gDown + gLeft + gDownLeft) / 4;
			bLL = (bCenter + bDown + bLeft + bDownLeft) / 4;

			aLR = (aCenter + aDown + aRight + aDownRight) / 4;
			rLR = (rCenter + rDown + rRight + rDownRight) / 4;
			gLR = (gCenter + gDown + gRight + gDownRight) / 4;
			bLR = (bCenter + bDown + bRight + bDownRight) / 4;

			maUp = (aUR - aUL) / (ex - sx);
			mrUp = (rUR - rUL) / (ex - sx);
			mgUp = (gUR - gUL) / (ex - sx);
			mbUp = (bUR - bUL) / (ex - sx);

			maDown = (aLR - aLL) / (ex - sx);
			mrDown = (rLR - rLL) / (ex - sx);
			mgDown = (gLR - gLL) / (ex - sx);
			mbDown = (bLR - bLL) / (ex - sx);

			/* loop through each pixel in the dest block */
			for(desty = sy; desty < ey; desty++)
			{
				for(destx = sx; destx < ex; destx++)
				{
					blocky = desty - sy;
					blockx = destx - sx;

					/* interpolate along both horizontal lines */
					aUpInterp = maUp * blockx + aUL;
					rUpInterp = mrUp * blockx + rUL;
					gUpInterp = mgUp * blockx + gUL;
					bUpInterp = mbUp * blockx + bUL;

					aDownInterp = maDown * blockx + aLL;
					rDownInterp = mrDown * blockx + rLL;
					gDownInterp = mgDown * blockx + gLL;
					bDownInterp = mbDown * blockx + bLL;

					/* Now interpolate along the vertical line */
					aFinal = (((aDownInterp - aUpInterp) * blocky) / (ey - sy)) + aUpInterp;
					aFinal >>= 16;
					rFinal = (((rDownInterp - rUpInterp) * blocky) / (ey - sy)) + rUpInterp;
					rFinal >>= 16;
					gFinal = (((gDownInterp - gUpInterp) * blocky) / (ey - sy)) + gUpInterp;
					gFinal >>= 16;
					bFinal = (((bDownInterp - bUpInterp) * blocky) / (ey - sy)) + bUpInterp;
					bFinal >>= 16;

					texel =
						(aFinal << 24) |
						(rFinal << 16) |
						(gFinal << 8) |
						(bFinal << 0);

					*((UUtUns32*)outDstData + inDstWidth * desty + destx) = texel;
				}
			}
		}
	}

	return UUcError_None;
}
#endif

#if INCLUDE_UNTESTED_STUFF
static UUtError
IMiImage_Scale_Box_Up_ARGB1555_IndependantAlpha(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	/* THIS IS HIGHLY *UN*OPTIMIZED */
	/* the pixmap needs to be expanded in x and y */
	UUtUns32	aUp, rUp, gUp, bUp;
	UUtUns32	aUpLeft, rUpLeft, gUpLeft, bUpLeft;
	UUtUns32	aUpRight, rUpRight, gUpRight, bUpRight;
	UUtUns32	aDown, rDown, gDown, bDown;
	UUtUns32	aDownLeft, rDownLeft, gDownLeft, bDownLeft;
	UUtUns32	aDownRight, rDownRight, gDownRight, bDownRight;
	UUtUns32	aLeft, rLeft, gLeft, bLeft;
	UUtUns32	aRight, rRight, gRight, bRight;
	UUtUns32	aCenter, rCenter, gCenter, bCenter;
	UUtUns32	texelUp, texelDown, texelRight;
	UUtUns32	texelLeft, texelUpLeft, texelUpRight, texelDownLeft, texelDownRight;
	UUtUns32	aFinal, rFinal, gFinal, bFinal;
	UUtUns32	blocky, blockx;
	UUtUns32	aUL, rUL, gUL, bUL;
	UUtUns32	aLL, rLL, gLL, bLL;
	UUtUns32	aUR, rUR, gUR, bUR;
	UUtUns32	aLR, rLR, gLR, bLR;
	UUtUns32	maUp, mrUp, mgUp, mbUp;
	UUtUns32	maDown, mrDown, mgDown, mbDown;
	UUtUns32	aUpInterp, rUpInterp, gUpInterp, bUpInterp;
	UUtUns32	aDownInterp, rDownInterp, gDownInterp, bDownInterp;
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	texel;
	UUtUns32	sx, sy, ex, ey;

	UUmAssert_Untested();

	// Not really sure I am handling alpha entirely correct here...

	/* compute the gradients to 16 bits of fractional precision */
	my = (inDstHeight << 16) / inSrcHeight;
	mx = (inDstWidth << 16) / inSrcWidth;

	/* We are going to traverse the source map expanding each pixel
	   into a block of pixels in the destination map */

	for(srcy = 0; srcy < inSrcHeight; srcy++)
	{
		for(srcx = 0; srcx < inSrcWidth; srcx++)
		{
			/* compute the starting x, y and then the ending x, y in the destination
			   pixmap space */
			sx = (srcx * mx) >> 16;
			sy = (srcy * my) >> 16;

			ex = (srcx * mx + mx) >> 16;
			ey = (srcy * my + my) >> 16;

			texel = *((UUtUns16*)inSrcData + inSrcWidth * srcy + srcx);
			aCenter = ((texel >> 15) & 0x01) << 16;
			rCenter = ((texel >> 10) & 0x1F) << 16;
			gCenter = ((texel >> 5) & 0x1F) << 16;
			bCenter = ((texel >> 0) & 0x1F) << 16;

			/* Read in the surrounding pixels */
			if(srcy > 0)
			{
				texelUp = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + srcx);

				if(srcx > 0)
				{
					texelUpLeft = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + (srcx-1));
				}
				else
				{
					texelUpLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelUpRight = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + (srcx+1));
				}
				else
				{
					texelUpRight = texel;
				}
			}
			else
			{
				texelUpRight = texelUpLeft = texelUp = texel;
			}

			aUp = ((texelUp >> 15) & 0x01) << 16;
			rUp = ((texelUp >> 10) & 0x1F) << 16;
			gUp = ((texelUp >> 5) & 0x1F) << 16;
			bUp = ((texelUp >> 0) & 0x1F) << 16;

			aUpLeft = ((texelUpLeft >> 15) & 0x01) << 16;
			rUpLeft = ((texelUpLeft >> 10) & 0x1F) << 16;
			gUpLeft = ((texelUpLeft >> 5) & 0x1F) << 16;
			bUpLeft = ((texelUpLeft >> 0) & 0x1F) << 16;

			aUpRight = ((texelUpRight >> 15) & 0x01) << 16;
			rUpRight = ((texelUpRight >> 10) & 0x1F) << 16;
			gUpRight = ((texelUpRight >> 5) & 0x1F) << 16;
			bUpRight = ((texelUpRight >> 0) & 0x1F) << 16;

			if(srcy + 1 < inSrcHeight)
			{
				texelDown = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + srcx);

				if(srcx > 0)
				{
					texelDownLeft = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + (srcx-1));
				}
				else
				{
					texelDownLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelDownRight = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + (srcx+1));
				}
				else
				{
					texelDownRight = texel;
				}
			}
			else
			{
				texelDownRight = texelDownLeft = texelDown = texel;
			}

			aDown = ((texelDown >> 15) & 0x01) << 16;
			rDown = ((texelDown >> 10) & 0x1F) << 16;
			gDown = ((texelDown >> 5) & 0x1F) << 16;
			bDown = ((texelDown >> 0) & 0x1F) << 16;

			aDownLeft = ((texelDownLeft >> 15) & 0x01) << 16;
			rDownLeft = ((texelDownLeft >> 10) & 0x1F) << 16;
			gDownLeft = ((texelDownLeft >> 5) & 0x1F) << 16;
			bDownLeft = ((texelDownLeft >> 0) & 0x1F) << 16;

			aDownRight = ((texelDownRight >> 15) & 0x01) << 16;
			rDownRight = ((texelDownRight >> 10) & 0x1F) << 16;
			gDownRight = ((texelDownRight >> 5) & 0x1F) << 16;
			bDownRight = ((texelDownRight >> 0) & 0x1F) << 16;

			if(srcx > 0)
			{
				texelLeft = *((UUtUns16*)inSrcData + inSrcWidth * srcy + (srcx-1));
			}
			else
			{
				texelLeft = texel;
			}

			aLeft = ((texelLeft >> 15) & 0x01) << 16;
			rLeft = ((texelLeft >> 10) & 0x1F) << 16;
			gLeft = ((texelLeft >> 5) & 0x1F) << 16;
			bLeft = ((texelLeft >> 0) & 0x1F) << 16;

			if(srcx + 1 < inSrcWidth)
			{
				texelRight = *((UUtUns16*)inSrcData + inSrcWidth * srcy + (srcx+1));
			}
			else
			{
				texelRight = texel;
			}

			aRight = ((texelRight >> 15) & 0x01) << 16;
			rRight = ((texelRight >> 10) & 0x1F) << 16;
			gRight = ((texelRight >> 5) & 0x1F) << 16;
			bRight = ((texelRight >> 0) & 0x1F) << 16;

			aUL = (aCenter + aUp + aLeft + aUpLeft) / 4;
			rUL = (rCenter + rUp + rLeft + rUpLeft) / 4;
			gUL = (gCenter + gUp + gLeft + gUpLeft) / 4;
			bUL = (bCenter + bUp + bLeft + bUpLeft) / 4;

			aUR = (aCenter + aUp + aRight + aUpRight) / 4;
			rUR = (rCenter + rUp + rRight + rUpRight) / 4;
			gUR = (gCenter + gUp + gRight + gUpRight) / 4;
			bUR = (bCenter + bUp + bRight + bUpRight) / 4;

			aLL = (aCenter + aDown + aLeft + aDownLeft) / 4;
			rLL = (rCenter + rDown + rLeft + rDownLeft) / 4;
			gLL = (gCenter + gDown + gLeft + gDownLeft) / 4;
			bLL = (bCenter + bDown + bLeft + bDownLeft) / 4;

			aLR = (aCenter + aDown + aRight + aDownRight) / 4;
			rLR = (rCenter + rDown + rRight + rDownRight) / 4;
			gLR = (gCenter + gDown + gRight + gDownRight) / 4;
			bLR = (bCenter + bDown + bRight + bDownRight) / 4;

			maUp = (aUR - aUL) / (ex - sx);
			mrUp = (rUR - rUL) / (ex - sx);
			mgUp = (gUR - gUL) / (ex - sx);
			mbUp = (bUR - bUL) / (ex - sx);

			maDown = (aLR - aLL) / (ex - sx);
			mrDown = (rLR - rLL) / (ex - sx);
			mgDown = (gLR - gLL) / (ex - sx);
			mbDown = (bLR - bLL) / (ex - sx);

			/* loop through each pixel in the dest block */
			for(desty = sy; desty < ey; desty++)
			{
				for(destx = sx; destx < ex; destx++)
				{
					blocky = desty - sy;
					blockx = destx - sx;

					/* interpolate along both horizontal lines */
					aUpInterp = maUp * blockx + aUL;
					rUpInterp = mrUp * blockx + rUL;
					gUpInterp = mgUp * blockx + gUL;
					bUpInterp = mbUp * blockx + bUL;

					aDownInterp = maDown * blockx + aLL;
					rDownInterp = mrDown * blockx + rLL;
					gDownInterp = mgDown * blockx + gLL;
					bDownInterp = mbDown * blockx + bLL;

					/* Now interpolate along the vertical line */
					aFinal = (((aDownInterp - aUpInterp) * blocky) / (ey - sy)) + aUpInterp;
					aFinal >>= 16;
					rFinal = (((rDownInterp - rUpInterp) * blocky) / (ey - sy)) + rUpInterp;
					rFinal >>= 16;
					gFinal = (((gDownInterp - gUpInterp) * blocky) / (ey - sy)) + gUpInterp;
					gFinal >>= 16;
					bFinal = (((bDownInterp - bUpInterp) * blocky) / (ey - sy)) + bUpInterp;
					bFinal >>= 16;

					texel =
						(aFinal << 15) |
						(rFinal << 10) |
						(gFinal << 5) |
						(bFinal << 0);

					*((UUtUns16*)outDstData + inDstWidth * desty + destx) = (UUtUns16)texel;
				}
			}
		}
	}

	return UUcError_None;
}
#endif

static UUtError
IMiImage_Scale_Box_Up_ARGB4444_IndependantAlpha(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	/* THIS IS HIGHLY *UN*OPTIMIZED */
	/* the pixmap needs to be expanded in x and y */
	UUtUns32	aUp, rUp, gUp, bUp;
	UUtUns32	aUpLeft, rUpLeft, gUpLeft, bUpLeft;
	UUtUns32	aUpRight, rUpRight, gUpRight, bUpRight;
	UUtUns32	aDown, rDown, gDown, bDown;
	UUtUns32	aDownLeft, rDownLeft, gDownLeft, bDownLeft;
	UUtUns32	aDownRight, rDownRight, gDownRight, bDownRight;
	UUtUns32	aLeft, rLeft, gLeft, bLeft;
	UUtUns32	aRight, rRight, gRight, bRight;
	UUtUns32	aCenter, rCenter, gCenter, bCenter;
	UUtUns32	texelUp, texelDown, texelRight;
	UUtUns32	texelLeft, texelUpLeft, texelUpRight, texelDownLeft, texelDownRight;
	UUtUns32	aFinal, rFinal, gFinal, bFinal;
	UUtUns32	blocky, blockx;
	UUtUns32	aUL, rUL, gUL, bUL;
	UUtUns32	aLL, rLL, gLL, bLL;
	UUtUns32	aUR, rUR, gUR, bUR;
	UUtUns32	aLR, rLR, gLR, bLR;
	UUtUns32	maUp, mrUp, mgUp, mbUp;
	UUtUns32	maDown, mrDown, mgDown, mbDown;
	UUtUns32	aUpInterp, rUpInterp, gUpInterp, bUpInterp;
	UUtUns32	aDownInterp, rDownInterp, gDownInterp, bDownInterp;
	UUtUns32	destx, desty, srcx, srcy;
	UUtUns32	my, mx;
	UUtUns32	texel;
	UUtUns32	sx, sy, ex, ey;

	//UUmAssert_Untested();

	// Not really sure I am handling alpha entirely correct here...

	/* compute the gradients to 16 bits of fractional precision */
	my = (inDstHeight << 16) / inSrcHeight;
	mx = (inDstWidth << 16) / inSrcWidth;

	/* We are going to traverse the source map expanding each pixel
	   into a block of pixels in the destination map */

	for(srcy = 0; srcy < inSrcHeight; srcy++)
	{
		for(srcx = 0; srcx < inSrcWidth; srcx++)
		{
			/* compute the starting x, y and then the ending x, y in the destination
			   pixmap space */
			sx = (srcx * mx) >> 16;
			sy = (srcy * my) >> 16;

			ex = (srcx * mx + mx) >> 16;
			ey = (srcy * my + my) >> 16;

			texel = *((UUtUns16*)inSrcData + inSrcWidth * srcy + srcx);
			aCenter = ((texel >> 12) & 0xF) << 16;
			rCenter = ((texel >> 8) & 0xF) << 16;
			gCenter = ((texel >> 4) & 0xF) << 16;
			bCenter = ((texel >> 0) & 0xF) << 16;

			/* Read in the surrounding pixels */
			if(srcy > 0)
			{
				texelUp = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + srcx);

				if(srcx > 0)
				{
					texelUpLeft = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + (srcx-1));
				}
				else
				{
					texelUpLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelUpRight = *((UUtUns16*)inSrcData + inSrcWidth * (srcy-1) + (srcx+1));
				}
				else
				{
					texelUpRight = texel;
				}
			}
			else
			{
				texelUpRight = texelUpLeft = texelUp = texel;
			}

			aUp = ((texelUp >> 12) & 0xF) << 16;
			rUp = ((texelUp >> 8) & 0xF) << 16;
			gUp = ((texelUp >> 4) & 0xF) << 16;
			bUp = ((texelUp >> 0) & 0xF) << 16;

			aUpLeft = ((texelUpLeft >> 12) & 0xF) << 16;
			rUpLeft = ((texelUpLeft >> 8) & 0xF) << 16;
			gUpLeft = ((texelUpLeft >> 4) & 0xF) << 16;
			bUpLeft = ((texelUpLeft >> 0) & 0xF) << 16;

			aUpRight = ((texelUpRight >> 12) & 0xFF) << 16;
			rUpRight = ((texelUpRight >> 8) & 0xFF) << 16;
			gUpRight = ((texelUpRight >> 4) & 0xFF) << 16;
			bUpRight = ((texelUpRight >> 0) & 0xFF) << 16;

			if(srcy + 1 < inSrcHeight)
			{
				texelDown = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + srcx);

				if(srcx > 0)
				{
					texelDownLeft = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + (srcx-1));
				}
				else
				{
					texelDownLeft = texel;
				}

				if(srcx + 1 < inSrcWidth)
				{
					texelDownRight = *((UUtUns16*)inSrcData + inSrcWidth * (srcy+1) + (srcx+1));
				}
				else
				{
					texelDownRight = texel;
				}
			}
			else
			{
				texelDownRight = texelDownLeft = texelDown = texel;
			}

			aDown = ((texelDown >> 12) & 0xF) << 16;
			rDown = ((texelDown >> 8) & 0xF) << 16;
			gDown = ((texelDown >> 4) & 0xF) << 16;
			bDown = ((texelDown >> 0) & 0xF) << 16;

			aDownLeft = ((texelDownLeft >> 12) & 0xF) << 16;
			rDownLeft = ((texelDownLeft >> 8) & 0xF) << 16;
			gDownLeft = ((texelDownLeft >> 4) & 0xF) << 16;
			bDownLeft = ((texelDownLeft >> 0) & 0xF) << 16;

			aDownRight = ((texelDownRight >> 12) & 0xF) << 16;
			rDownRight = ((texelDownRight >> 8) & 0xF) << 16;
			gDownRight = ((texelDownRight >> 4) & 0xF) << 16;
			bDownRight = ((texelDownRight >> 0) & 0xF) << 16;

			if(srcx > 0)
			{
				texelLeft = *((UUtUns16*)inSrcData + inSrcWidth * srcy + (srcx-1));
			}
			else
			{
				texelLeft = texel;
			}

			aLeft = ((texelLeft >> 12) & 0xF) << 16;
			rLeft = ((texelLeft >> 8) & 0xF) << 16;
			gLeft = ((texelLeft >> 4) & 0xF) << 16;
			bLeft = ((texelLeft >> 0) & 0xF) << 16;

			if(srcx + 1 < inSrcWidth)
			{
				texelRight = *((UUtUns16*)inSrcData + inSrcWidth * srcy + (srcx+1));
			}
			else
			{
				texelRight = texel;
			}

			aRight = ((texelRight >> 12) & 0xF) << 16;
			rRight = ((texelRight >> 8) & 0xF) << 16;
			gRight = ((texelRight >> 4) & 0xF) << 16;
			bRight = ((texelRight >> 0) & 0xF) << 16;

			aUL = (aCenter + aUp + aLeft + aUpLeft) / 4;
			rUL = (rCenter + rUp + rLeft + rUpLeft) / 4;
			gUL = (gCenter + gUp + gLeft + gUpLeft) / 4;
			bUL = (bCenter + bUp + bLeft + bUpLeft) / 4;

			aUR = (aCenter + aUp + aRight + aUpRight) / 4;
			rUR = (rCenter + rUp + rRight + rUpRight) / 4;
			gUR = (gCenter + gUp + gRight + gUpRight) / 4;
			bUR = (bCenter + bUp + bRight + bUpRight) / 4;

			aLL = (aCenter + aDown + aLeft + aDownLeft) / 4;
			rLL = (rCenter + rDown + rLeft + rDownLeft) / 4;
			gLL = (gCenter + gDown + gLeft + gDownLeft) / 4;
			bLL = (bCenter + bDown + bLeft + bDownLeft) / 4;

			aLR = (aCenter + aDown + aRight + aDownRight) / 4;
			rLR = (rCenter + rDown + rRight + rDownRight) / 4;
			gLR = (gCenter + gDown + gRight + gDownRight) / 4;
			bLR = (bCenter + bDown + bRight + bDownRight) / 4;

			maUp = (aUR - aUL) / (ex - sx);
			mrUp = (rUR - rUL) / (ex - sx);
			mgUp = (gUR - gUL) / (ex - sx);
			mbUp = (bUR - bUL) / (ex - sx);

			maDown = (aLR - aLL) / (ex - sx);
			mrDown = (rLR - rLL) / (ex - sx);
			mgDown = (gLR - gLL) / (ex - sx);
			mbDown = (bLR - bLL) / (ex - sx);

			/* loop through each pixel in the dest block */
			for(desty = sy; desty < ey; desty++)
			{
				for(destx = sx; destx < ex; destx++)
				{
					blocky = desty - sy;
					blockx = destx - sx;

					/* interpolate along both horizontal lines */
					aUpInterp = maUp * blockx + aUL;
					rUpInterp = mrUp * blockx + rUL;
					gUpInterp = mgUp * blockx + gUL;
					bUpInterp = mbUp * blockx + bUL;

					aDownInterp = maDown * blockx + aLL;
					rDownInterp = mrDown * blockx + rLL;
					gDownInterp = mgDown * blockx + gLL;
					bDownInterp = mbDown * blockx + bLL;

					/* Now interpolate along the vertical line */
					aFinal = (((aDownInterp - aUpInterp) * blocky) / (ey - sy)) + aUpInterp;
					aFinal >>= 16;
					rFinal = (((rDownInterp - rUpInterp) * blocky) / (ey - sy)) + rUpInterp;
					rFinal >>= 16;
					gFinal = (((gDownInterp - gUpInterp) * blocky) / (ey - sy)) + gUpInterp;
					gFinal >>= 16;
					bFinal = (((bDownInterp - bUpInterp) * blocky) / (ey - sy)) + bUpInterp;
					bFinal >>= 16;

					texel =
						(aFinal << 12) |
						(rFinal << 8) |
						(gFinal << 4) |
						(bFinal << 0);

					*((UUtUns16*)outDstData + inDstWidth * desty + destx) = (UUtUns16)texel;
				}
			}
		}
	}

	return UUcError_None;
}

UUtError
IMrImage_Scale_Box(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	IMtPixelType	inSrcPixelType,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	UUmAssertReadPtr(inSrcData, IMrImage_ComputeSize(inSrcPixelType, IMcNoMipMap, inSrcWidth, inSrcHeight));
	UUmAssertWritePtr(outDstData, IMrImage_ComputeSize(inSrcPixelType, IMcNoMipMap, inDstWidth, inDstHeight));

	UUmAssert(0 != inSrcWidth);
	UUmAssert(0 != inSrcHeight);
	UUmAssert(0 != inDstWidth);
	UUmAssert(0 != inDstHeight);

	if ((0 == inSrcWidth) || (0 == inSrcHeight) || (0 == inDstWidth) || (0 == inDstHeight)) {
		return UUcError_None;
	}

	UUrMemory_Block_VerifyList();

	if(inSrcWidth == inDstWidth && inSrcHeight == inDstHeight)
	{

		if(inSrcData == outDstData) return UUcError_None;

		UUrMemory_MoveFast(
			inSrcData,
			outDstData,
			IMrImage_ComputeRowBytes(inSrcPixelType, inSrcWidth) * inSrcHeight);
	}
	else if(inDstWidth <= inSrcWidth && inDstHeight <= inSrcHeight)
	{
		switch(inSrcPixelType)
		{
			case IMcPixelType_ARGB4444:
				return
					IMiImage_Scale_Box_Down_ARGB4444(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);

			case IMcPixelType_RGB555:
				return
					IMiImage_Scale_Box_Down_RGB555(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);

			case IMcPixelType_ARGB1555:
				return
					IMiImage_Scale_Box_Down_ARGB1555(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);

			case IMcPixelType_I8:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_I1:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_A8:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_A4I4:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_ARGB8888:
				return
					IMiImage_Scale_Box_Down_ARGB8888(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);

			case IMcPixelType_RGB888:
				return
					IMiImage_Scale_Box_Down_RGB888(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);
			default:
				UUmError_ReturnOnError(UUcError_Generic);
		}
	}
	else if(inDstWidth >= inSrcWidth && inDstHeight >= inSrcHeight)
	{
		UUmAssert(!"not using scale up");

		switch(inSrcPixelType)
		{
			case IMcPixelType_ARGB4444:
				return
					IMiImage_Scale_Box_Up_ARGB4444(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);

#if INCLUDE_UNTESTED_STUFF
			case IMcPixelType_RGB555:
				return
					IMiImage_Scale_Box_Up_RGB555(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);
#endif

#if INCLUDE_UNTESTED_STUFF
			case IMcPixelType_ARGB1555:
				return
					IMiImage_Scale_Box_Up_ARGB1555(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);
#endif

			case IMcPixelType_I8:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_I1:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_A8:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_A4I4:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

#if INCLUDE_UNTESTED_STUFF
			case IMcPixelType_ARGB8888:
				return
					IMiImage_Scale_Box_Up_ARGB8888(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);
#endif

#if INCLUDE_UNTESTED_STUFF
			case IMcPixelType_RGB888:
				return
					IMiImage_Scale_Box_Up_RGB888(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);
#endif
			default:
				UUmError_ReturnOnError(UUcError_Generic);
		}
	}
	else
	{
		/* expand source to bigger dimension then scale to destination */

		void	*tempMemory;
		UUtUns16	tempDimension;

		tempDimension = inSrcWidth > inSrcHeight ? inSrcWidth : inSrcHeight;

		tempMemory = UUrMemory_Block_New(
						IMrImage_ComputeRowBytes(inSrcPixelType, tempDimension) * tempDimension);
		UUmError_ReturnOnNull(tempMemory);

		IMrImage_Scale_Box(
			inSrcWidth,
			inSrcHeight,
			inSrcPixelType,
			inSrcData,
			tempDimension,
			tempDimension,
			tempMemory);


		IMrImage_Scale_Box(
			tempDimension,
			tempDimension,
			inSrcPixelType,
			tempMemory,
			inDstWidth,
			inDstHeight,
			outDstData);

		UUrMemory_Block_Delete(tempMemory);
	}

	UUrMemory_Block_VerifyList();

	return UUcError_None;
}

UUtError
IMrImage_Scale_Box_IndependantAlpha(
	UUtUns16		inSrcWidth,
	UUtUns16		inSrcHeight,
	IMtPixelType	inSrcPixelType,
	void*			inSrcData,
	UUtUns16		inDstWidth,
	UUtUns16		inDstHeight,
	void*			outDstData)
{
	UUrMemory_Block_VerifyList();

	if(inSrcWidth == inDstWidth && inSrcHeight == inDstHeight)
	{

		if(inSrcData == outDstData) return UUcError_None;

		UUrMemory_MoveFast(
			inSrcData,
			outDstData,
			IMrImage_ComputeRowBytes(inSrcPixelType, inSrcWidth) * inSrcHeight);
	}
	else if(inDstWidth < inSrcWidth && inDstHeight < inSrcHeight)
	{
		switch(inSrcPixelType)
		{
			case IMcPixelType_ARGB4444:
				return
					IMiImage_Scale_Box_Down_ARGB4444_IndependantAlpha(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);

			case IMcPixelType_RGB555:
				return
					IMiImage_Scale_Box_Down_RGB555(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);

			case IMcPixelType_ARGB1555:
				return
					IMiImage_Scale_Box_Down_ARGB1555_IndependantAlpha(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);

			case IMcPixelType_I8:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_I1:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_A8:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_A4I4:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_ARGB8888:
				return
					IMiImage_Scale_Box_Down_ARGB8888_IndependantAlpha(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);

			case IMcPixelType_RGB888:
				return
					IMiImage_Scale_Box_Down_RGB888(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);
			default:
				UUmError_ReturnOnError(UUcError_Generic);
		}
	}
	else if(inDstWidth >= inSrcWidth && inDstHeight >= inSrcHeight)
	{
		switch(inSrcPixelType)
		{
			case IMcPixelType_ARGB4444:
				return
					IMiImage_Scale_Box_Up_ARGB4444_IndependantAlpha(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);

#if INCLUDE_UNTESTED_STUFF
			case IMcPixelType_RGB555:
				return
					IMiImage_Scale_Box_Up_RGB555(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);
#endif

#if INCLUDE_UNTESTED_STUFF
			case IMcPixelType_ARGB1555:
				return
					IMiImage_Scale_Box_Up_ARGB1555_IndependantAlpha(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);
#endif

			case IMcPixelType_I8:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_I1:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_A8:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

			case IMcPixelType_A4I4:
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Not implemented");

#if INCLUDE_UNTESTED_STUFF
			case IMcPixelType_ARGB8888:
				return
					IMiImage_Scale_Box_Up_ARGB8888_IndependantAlpha(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);
#endif

#if INCLUDE_UNTESTED_STUFF
			case IMcPixelType_RGB888:
				return
					IMiImage_Scale_Box_Up_RGB888(
						inSrcWidth,
						inSrcHeight,
						inSrcData,
						inDstWidth,
						inDstHeight,
						outDstData);
#endif
			default:
				UUmError_ReturnOnError(UUcError_Generic);
		}
	}
	else
	{
		/* expand source to bigger dimension then scale to destination */

		void	*tempMemory;
		UUtUns16	tempDimension;

		tempDimension = inSrcWidth > inSrcHeight ? inSrcWidth : inSrcHeight;

		tempMemory = UUrMemory_Block_New(
						IMrImage_ComputeRowBytes(inSrcPixelType, tempDimension) * tempDimension);
		UUmError_ReturnOnNull(tempMemory);

		IMrImage_Scale_Box(
			inSrcWidth,
			inSrcHeight,
			inSrcPixelType,
			inSrcData,
			tempDimension,
			tempDimension,
			tempMemory);


		IMrImage_Scale_Box(
			tempDimension,
			tempDimension,
			inSrcPixelType,
			tempMemory,
			inDstWidth,
			inDstHeight,
			outDstData);

		UUrMemory_Block_Delete(tempMemory);
	}

	UUrMemory_Block_VerifyList();

	return UUcError_None;
}
