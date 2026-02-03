/*
	FILE:	MS_DC_Method_LinePoint.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 13, 1997

	PURPOSE:

	Copyright 1997

*/

#include <stdlib.h>

#include "BFW.h"
#include "BFW_Motoko.h"

#include "BFW_Shared_TriRaster.h"

#include "MS_DC_Private.h"
#include "MS_DC_Method_LinePoint.h"

void
MSrDrawContext_Method_Point(
	M3tDrawContext*	inDrawContext,
	M3tPointScreen*	invCoord,
	UUtUns16		inVShade)
{
	UUtInt32	x, y;
	UUtUns32	z;
	UUtUns32	targetZ;
	UUtUns16	*targetZPtr, *targetRGBPtr;
	MStDrawContextPrivate	*drawContextPrivate = (MStDrawContextPrivate *)inDrawContext->privateContext;

	UUmAssert(invCoord->y >= 0.0 && invCoord->y < (float)drawContextPrivate->height);
	UUmAssert(invCoord->x >= 0.0 && invCoord->x < (float)drawContextPrivate->width);
	UUmAssert(invCoord->z >= 0.0 && invCoord->z <= 1.0);

	x = (UUtInt32)invCoord->x;
	y = (UUtInt32)invCoord->y;
	z = (UUtInt32)invCoord->z * MSmULongScale;

	//y = drawContext->height - y;

	targetZPtr = (UUtUns16 *)((char *)drawContextPrivate->zBufferBaseAddr +
				y * drawContextPrivate->zBufferRowBytes +
				x * 2);

	targetZ = *targetZPtr << 16;

	if(z < targetZ)
	{
		*targetZPtr = (UUtUns16)(z >> 16);

		targetRGBPtr = (UUtUns16 *)((char *)drawContextPrivate->imageBufferBaseAddr +
				y * drawContextPrivate->imageBufferRowBytes +
				x * 2);

		*targetRGBPtr = inVShade;

	}
}

void
MSrDrawContext_Method_Line_Interpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1)
{
#if 0
	float					fabsMajorDelta, minorDelta;
	float					majorStart, majorEnd, minorStart;
	long					majorStartSnapped, majorEndSnapped;
	float					snapCorrection;
	float					invLength;
	float					tempStart, tempEnd, dTemp;
	unsigned long			z, dZ;
	long					rScaled, dRScaled;
	long					gScaled, dGScaled;
	long					bScaled, dBScaled;
	long					minorScaled, dMinorScaled;
	long					rgbMinorIncrement, rgbMajorIncrement;
	long					zMinorIncrement, zMajorIncrement;
	long					rgbPixelBytes, zPixelBytes;

	long					rgbRowBytes;
	long					zRowBytes;
	float					x0, y0, x1, y1;
	float					dx, dy, fabs_dx, fabs_dy;

	M3tPointScreen			*vCoordStart, *vCoordEnd;
	UUtUns16				vShadeStart, vShadeEnd;

	long					major, minor;
	char					*rgbAddress;
	char					*zAddress;
	unsigned long			zPixel;

	const long				fractMask = MSmFractOne - 1;
	const float				charScale = (MSmFractOne * MSmUCharScale);
	MStDrawContextPrivate	*drawContextPrivate = (MStDrawContextPrivate *)inDrawContext->privateContext;

	float					height = (float) drawContextPrivate->height;
	float					width = (float) drawContextPrivate->width;

	M3tPointScreen*			screenPoints;
	M3tPointScreen*			vCoord0;
	M3tPointScreen*			vCoord1;

	UUtUns16*			vertexShades;
	UUtUns16			vShade0;
	UUtUns16			vShade1;

	screenPoints = drawContextPrivate->arrayData[M3cDrawArrayType_ScreenPoint];
	vertexShades = (UUtUns16*)drawContextPrivate->arrayData[M3cDrawArrayType_ScreenShade_DC];

	vCoord0 = screenPoints + inVIndex0;
	vCoord1 = screenPoints + inVIndex1;

	vShade0 = vertexShades[inVIndex0];
	vShade1 = vertexShades[inVIndex1];

	x0 = vCoord0->x;
	x1 = vCoord1->x;
	y0 = vCoord0->y;
	y1 = vCoord1->y;

	/*
	 *
	 * GME - 6/18
	 * It appears that this functions requires clipped lines, I am adding assertions
	 *
	 */

	UUmAssert(x0 >= 0.0);
	UUmAssert(x1 >= 0.0);
	UUmAssert(y0 >= 0.0);
	UUmAssert(y1 >= 0.0);

	UUmAssert(x0 < width);
	UUmAssert(x1 < width);
	UUmAssert(y0 < height);
	UUmAssert(y1 < height);

	dx = x1 - x0;
	dy = y1 - y0;

	rgbPixelBytes = M3cDrawRGBBytesPerPixel;
	zPixelBytes = M3cDrawZBytesPerPixel;

	fabs_dx = (float)UUmFabs (dx);
	fabs_dy = (float)UUmFabs (dy);

	rgbRowBytes = drawContextPrivate->imageBufferRowBytes;
	zRowBytes = drawContextPrivate->zBufferRowBytes;

	if (fabs_dx > fabs_dy)
	{
		/*
		 * Δx is larger, so the major axis will be X.
		 */

		rgbMajorIncrement = rgbPixelBytes;
		zMajorIncrement = zPixelBytes;
		rgbMinorIncrement = rgbRowBytes;
		zMinorIncrement = zRowBytes;

		if (dx > 0.0F)
		{
			vCoordStart = vCoord0;
			vShadeStart = vShade0;
			vCoordEnd = vCoord1;
			vShadeEnd = vShade1;
			majorStart = x0;
			majorEnd = x1;
			minorStart = y0;
			minorDelta = dy;
		}
		else
		{
			vCoordStart = vCoord1;
			vShadeStart = vShade1;
			vCoordEnd = vCoord0;
			vShadeEnd = vShade0;
			majorStart = x1;
			majorEnd = x0;
			minorStart = y1;
			minorDelta = -dy;
		}
		fabsMajorDelta = fabs_dx;
	}
	else
	{
		/*
		 * Δy is larger, so the major axis will be Y.
		 */

		rgbMajorIncrement = rgbRowBytes;
		zMajorIncrement = zRowBytes;
		rgbMinorIncrement = rgbPixelBytes;
		zMinorIncrement = zPixelBytes;

		if (dy > 0.0F)
		{
			vCoordStart = vCoord0;
			vShadeStart = vShade0;
			vCoordEnd = vCoord1;
			vShadeEnd = vShade1;
			majorStart = y0;
			majorEnd = y1;
			minorStart = x0;
			minorDelta = dx;
		}
		else
		{
			vCoordStart = vCoord1;
			vShadeStart = vShade1;
			vCoordEnd = vCoord0;
			vShadeEnd = vShade0;
			majorStart = y1;
			majorEnd = y0;
			minorStart = x1;
			minorDelta = -dx;
		}
		fabsMajorDelta = fabs_dy;
	}

	/*
	 * Snap major axis endpoints, including a 0.5 pixel subpixel coverage rule.
	 */

	majorStartSnapped = (UUtInt32)(majorStart + 0.5F);

	/* Avoid Divide by zero error */
	if(fabsMajorDelta == 0.0F)
	{
		return;
	}

	invLength = 1.0F / fabsMajorDelta;
	snapCorrection = (majorStartSnapped + 0.5F) - majorStart;
	tempEnd = majorEnd - 0.5F;
	if (tempEnd < 0.0F)
	{
		/*
		 * majorEnd is less than 0.5, so this line doesn't reach the
		 * first pixel. Return. Note that this test isn't done for optimization
		 * reasons -- if we continue, the cast of fTemp to an int will produce
		 * 0, which isn't correct.
		 */

		return;
	}
	majorEndSnapped = (UUtInt32)tempEnd;

	/*
	 * Convert Z to fixed point, compute forward difference, and compute sub-pixel corrected
	 * initial value. This isn't really necessary if kQAZFunction_None is set, but testing
	 * for that isn't really worth while.
	 */

	{
		const float	zScale = (float)MSmULongScale;
		const float	zOffset = (float)MSmULongOffset;

		tempStart = vCoordStart->z * zScale + zOffset;
		tempEnd = vCoordEnd->z * zScale + zOffset;
		dTemp = (tempEnd - tempStart) * invLength;
		z = (UUtUns32)(tempStart + snapCorrection * dTemp);
		dZ = (long) dTemp;

	}

	tempStart = vShadeStart->r * charScale;
	tempEnd = vShadeEnd->r * charScale;
	dTemp = (tempEnd - tempStart) * invLength;
	rScaled = (UUtInt32)tempStart;
	dRScaled = (UUtInt32)dTemp;

	tempStart = vShadeStart->g * charScale;
	tempEnd = vShadeEnd->g * charScale;
	dTemp = (tempEnd - tempStart) * invLength;
	gScaled = (UUtInt32)tempStart;
	dGScaled = (UUtInt32)dTemp;

	tempStart = vShadeStart->b * charScale;
	tempEnd = vShadeEnd->b * charScale;
	dTemp = (tempEnd - tempStart) * invLength;
	bScaled = (UUtInt32)tempStart;
	dBScaled = (UUtInt32)dTemp;

	/*
	 * Determine the initial minor axis value, the minor axis delta per major axis pixel,
	 * and whether the minor axis will increment or decrement as we move along the major axis.
	 */

	minorScaled = (UUtInt32)(minorStart * (float)MSmFractOne);
	dMinorScaled = (UUtInt32)(minorDelta * invLength * (float)MSmFractOne);

	if (minorDelta < 0.0F)
	{
		/*
		 * Minor axis decrements, so negate the rgb/zMinorIncrement values.
		 */

		rgbMinorIncrement = -rgbMinorIncrement;
		zMinorIncrement = -zMinorIncrement;
	}

	major = majorStartSnapped;
	minor = minorScaled >> MSmFractBits;
	minorScaled &= fractMask;

	rgbAddress = (char *)drawContextPrivate->imageBufferBaseAddr
			+ major * rgbMajorIncrement + minor * abs (rgbMinorIncrement);

	zAddress = (char *)drawContextPrivate->zBufferBaseAddr
			+ major * zMajorIncrement + minor * abs (zMinorIncrement);

	while (major++ <= majorEndSnapped)
	{
		zPixel = *((unsigned short *) zAddress);
		zPixel = zPixel | (zPixel << 16);

		if (z < zPixel)
		{
			long	rDithered, gDithered, bDithered;

			rDithered = rScaled >> MSmFractBits;
			gDithered = gScaled >> MSmFractBits;
			bDithered = bScaled >> MSmFractBits;

			*((UUtUns16 *) zAddress) = (UUtUns16)(z >> 16);

			*((UUtUns16 *) rgbAddress) = (UUtUns16)(
					  ((rScaled >> (MSmFractBits + 3)) << 10)
					| ((gScaled >> (MSmFractBits + 3)) << 5)
					|  (bScaled >> (MSmFractBits + 3)));
		}

		zAddress += zMajorIncrement;
		z += dZ;
		minorScaled += dMinorScaled;
		rgbAddress += rgbMajorIncrement;
		rScaled += dRScaled;
		gScaled += dGScaled;
		bScaled += dBScaled;
		if (minorScaled & ~fractMask)
		{
			rgbAddress += rgbMinorIncrement;
			minorScaled &= fractMask;
			zAddress += zMinorIncrement;
		}
	}
#endif

}

void
MSrDrawContext_Method_Line_Flat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inShade)
{
	float					fabsMajorDelta, minorDelta;
	float					majorStart, majorEnd, minorStart;
	long					majorStartSnapped, majorEndSnapped;
	float					snapCorrection;
	float					invLength;
	float					tempStart, tempEnd, dTemp;
	unsigned long			z, dZ;
	long					minorScaled, dMinorScaled;
	long					zMinorIncrement, zMajorIncrement;
	long					rgbPixelBytes, zPixelBytes;

	long					rgbMinorIncrement, rgbMajorIncrement;
	long					rgbRowBytes;
	long					zRowBytes;
	float					x0, y0, x1, y1;
	float					dx, dy, fabs_dx, fabs_dy;

	M3tPointScreen			*vCoordStart, *vCoordEnd;

	long					major, minor;
	char					*rgbAddress;
	char					*zAddress;
	unsigned long			zPixel;

	const long				fractMask = MSmFractOne - 1;
	MStDrawContextPrivate	*drawContextPrivate = (MStDrawContextPrivate *)inDrawContext->privateContext;


	float					height = (float) drawContextPrivate->height;
	float					width = (float) drawContextPrivate->width;

	M3tPointScreen*			screenPoints;
	M3tPointScreen*			vCoord0;
	M3tPointScreen*			vCoord1;

	screenPoints = drawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];

	vCoord0 = screenPoints + inVIndex0;
	vCoord1 = screenPoints + inVIndex1;

	x0 = vCoord0->x;
	x1 = vCoord1->x;
	y0 = vCoord0->y;
	y1 = vCoord1->y;

	/*
	 *
	 * GME - 6/18
	 * It appears that this functions requires clipped lines, I am adding assertions
	 *
	 */

	UUmAssert(x0 >= 0.0);
	UUmAssert(x1 >= 0.0);
	UUmAssert(y0 >= 0.0);
	UUmAssert(y1 >= 0.0);
	UUmAssert(vCoord0->z >= 0.0);
	UUmAssert(vCoord1->z >= 0.0);
	UUmAssert(vCoord0->z <= 1.0);
	UUmAssert(vCoord1->z <= 1.0);

	UUmAssert(x0 < width);
	UUmAssert(x1 < width);
	UUmAssert(y0 < height);
	UUmAssert(y1 < height);

	// this is just some simple clipping, not really correct in the long run -GME
	if ((x0 < 0) || (x1 < 0) || (y0 < 0) || (y1 < 0) || (x0 >= width) || (x1 >= width) || (y0 >= height) || (y1 >= width))
	{
		return;
	}

	dx = x1 - x0;
	dy = y1 - y0;

	rgbPixelBytes = M3cDrawRGBBytesPerPixel;
	zPixelBytes = M3cDrawZBytesPerPixel;

	fabs_dx = (float)UUmFabs (dx);
	fabs_dy = (float)UUmFabs (dy);

	rgbRowBytes = drawContextPrivate->imageBufferRowBytes;
	zRowBytes = drawContextPrivate->zBufferRowBytes;

	if (fabs_dx > fabs_dy)
	{
		/*
		 * Δx is larger, so the major axis will be X.
		 */

		rgbMajorIncrement = rgbPixelBytes;
		zMajorIncrement = zPixelBytes;
		rgbMinorIncrement = rgbRowBytes;
		zMinorIncrement = zRowBytes;

		if (dx > 0.0F)
		{
			vCoordStart = vCoord0;
			vCoordEnd = vCoord1;
			majorStart = x0;
			majorEnd = x1;
			minorStart = y0;
			minorDelta = dy;
		}
		else
		{
			vCoordStart = vCoord1;
			vCoordEnd = vCoord0;
			majorStart = x1;
			majorEnd = x0;
			minorStart = y1;
			minorDelta = -dy;
		}
		fabsMajorDelta = fabs_dx;
	}
	else
	{
		/*
		 * Δy is larger, so the major axis will be Y.
		 */

		rgbMajorIncrement = rgbRowBytes;
		zMajorIncrement = zRowBytes;
		rgbMinorIncrement = rgbPixelBytes;
		zMinorIncrement = zPixelBytes;

		if (dy > 0.0F)
		{
			vCoordStart = vCoord0;
			vCoordEnd = vCoord1;
			majorStart = y0;
			majorEnd = y1;
			minorStart = x0;
			minorDelta = dx;
		}
		else
		{
			vCoordStart = vCoord1;
			vCoordEnd = vCoord0;
			majorStart = y1;
			majorEnd = y0;
			minorStart = x1;
			minorDelta = -dx;
		}
		fabsMajorDelta = fabs_dy;
	}

	/*
	 * Snap major axis endpoints, including a 0.5 pixel subpixel coverage rule.
	 */

	majorStartSnapped = (UUtInt32)(majorStart + 0.5F);

	/* Avoid Divide by zero error */
	if(fabsMajorDelta == 0.0F)
	{
		return;
	}

	invLength = 1.0F / fabsMajorDelta;
	snapCorrection = (majorStartSnapped + 0.5F) - majorStart;
	tempEnd = majorEnd - 0.5F;
	if (tempEnd < 0.0F)
	{
		/*
		 * majorEnd is less than 0.5, so this line doesn't reach the
		 * first pixel. Return. Note that this test isn't done for optimization
		 * reasons -- if we continue, the cast of fTemp to an int will produce
		 * 0, which isn't correct.
		 */

		return;
	}
	majorEndSnapped = (UUtInt32)tempEnd;

	/*
	 * Convert Z to fixed point, compute forward difference, and compute sub-pixel corrected
	 * initial value. This isn't really necessary if kQAZFunction_None is set, but testing
	 * for that isn't really worth while.
	 */

	{
		const float	zScale = (float)MSmULongScale;
		const float	zOffset = (float)MSmULongOffset;

		tempStart = vCoordStart->z * zScale + zOffset;
		tempEnd = vCoordEnd->z * zScale + zOffset;
		dTemp = (tempEnd - tempStart) * invLength;
		z = (UUtUns32)(tempStart + snapCorrection * dTemp);
		dZ = (long) dTemp;

	}

	/*
	 * Determine the initial minor axis value, the minor axis delta per major axis pixel,
	 * and whether the minor axis will increment or decrement as we move along the major axis.
	 */

	minorScaled = (UUtInt32)(minorStart * (float)MSmFractOne);
	dMinorScaled = (UUtInt32)(minorDelta * invLength * (float)MSmFractOne);

	if (minorDelta < 0.0F)
	{
		/*
		 * Minor axis decrements, so negate the rgb/zMinorIncrement values.
		 */

		rgbMinorIncrement = -rgbMinorIncrement;
		zMinorIncrement = -zMinorIncrement;
	}

	major = majorStartSnapped;
	minor = minorScaled >> MSmFractBits;
	minorScaled &= fractMask;

	rgbAddress = (char *)drawContextPrivate->imageBufferBaseAddr
			+ major * rgbMajorIncrement + minor * abs (rgbMinorIncrement);

	zAddress = (char *)drawContextPrivate->zBufferBaseAddr
			+ major * zMajorIncrement + minor * abs (zMinorIncrement);

	while (major++ <= majorEndSnapped)
	{
		zPixel = *((unsigned short *) zAddress);
		zPixel = zPixel | (zPixel << 16);

		if (z < zPixel)
		{

			*((UUtUns16 *) zAddress) = (UUtUns16)(z >> 16);

			*((UUtUns16 *) rgbAddress) = inShade;
		}

		zAddress += zMajorIncrement;
		z += dZ;
		minorScaled += dMinorScaled;
		rgbAddress += rgbMajorIncrement;
		if (minorScaled & ~fractMask)
		{
			rgbAddress += rgbMinorIncrement;
			minorScaled &= fractMask;
			zAddress += zMinorIncrement;
		}
	}
}

