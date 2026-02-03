/*
	FILE:	MS_DC_Platform_Mac.c

	AUTHOR:	Brent H. Pease

	CREATED: May 19, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"

#include "MS_DC_Private.h"
#include "MS_DC_Method_Frame.h"

#include "MS_Mac_CustomProc.h"
#include "MS_Mac_PPCAsm.h"

UUtError
MSrDrawContext_Method_Frame_Start(
	M3tDrawContext		*inDrawContext)
{
	MStDrawContextPrivate*	drawContextPrivate = (MStDrawContextPrivate*)inDrawContext->privateContext;
	MStPlatformSpecific*	platformSpecific;

	platformSpecific = &drawContextPrivate->platformSpecific;

	#if defined(MSmPixelTouch) && MSmPixelTouch

	{
		UUtUns16 x, y;
		double	*p;

		double	zero;
		UUtUns32	clearValue[2];

		clearValue[0] = 0;
		clearValue[1] = 0;

		zero = *(double *)(clearValue);

		for(y = 0; y < drawContextPrivate->height; y++)
		{

			p = (double *)((char *)drawContextPrivate->pixelTouchBaseAddr + drawContextPrivate->pixelTouchRowBytes * y);

			for(x = 0; x < drawContextPrivate->pixelTouchRowBytes >> 5; x++)
			{
				UUrProcessor_ZeroCacheLine(p, 0);

				*p++ = zero;
				*p++ = zero;
				*p++ = zero;
				*p++ = zero;
			}
		}
	}

	#else

		if(!(drawContextPrivate->screenFlags & M3cDrawContextScreenFlags_DoubleBuffer))
		{
			MSrPPCAsm_CallProc(
				platformSpecific->zBufferClearProc,
				drawContextPrivate,
				(long)&platformSpecific->zClearValue0);

			MSrPPCAsm_CallProc(
				platformSpecific->imageBufferClearProc,
				drawContextPrivate,
				(long)&platformSpecific->imageClearValue0);
		}

	#endif

	return UUcError_None;
}

UUtError
MSrDrawContext_Method_Frame_End(
	M3tDrawContext		*inDrawContext)
{
	MStDrawContextPrivate*	drawContextPrivate = (MStDrawContextPrivate*)inDrawContext->privateContext;
	MStPlatformSpecific	*platformSpecific;

	platformSpecific = &drawContextPrivate->platformSpecific;

	#if defined(MSmPixelTouch) && MSmPixelTouch

	{
		UUtUns16	x, y;
		UUtUns16*	curPixelTouch;
		UUtUns16	pixelTouchVal;
		UUtUns16*	curSrcImagePtr;
		UUtUns16*	curDstImagePtr;
		double		imageClearValue;
		UUtInt16	i;

		imageClearValue = *(double *)&drawContextPrivate->platformSpecific.imageClearValue0;

		for(y = 0; y < drawContextPrivate->height; y++)
		{
			curPixelTouch = (UUtUns16 *)((char *)drawContextPrivate->pixelTouchBaseAddr + drawContextPrivate->pixelTouchRowBytes * y);
			curSrcImagePtr = (UUtUns16 *)((char *)drawContextPrivate->imageBufferBaseAddr + drawContextPrivate->imageBufferRowBytes * y);
			curDstImagePtr = (UUtUns16 *)((char *)platformSpecific->frontImageBaseAddr + platformSpecific->frontImageRowBytes * y);

			for(x = 0; x < drawContextPrivate->width >> 4; x++)
			{
				pixelTouchVal = *curPixelTouch++;

				if(pixelTouchVal == 0)
				{
					*(double *)(curDstImagePtr) = imageClearValue;
					*(double *)(curDstImagePtr + 4) = imageClearValue;
					*(double *)(curDstImagePtr + 8) = imageClearValue;
					*(double *)(curDstImagePtr + 12) = imageClearValue;
					curDstImagePtr += 16;
					curSrcImagePtr += 16;
				}
				else if(pixelTouchVal == 0xFFFF)
				{
					*(double *)(curDstImagePtr) = *(double *)(curSrcImagePtr);
					*(double *)(curDstImagePtr + 4) = *(double *)(curSrcImagePtr + 4);
					*(double *)(curDstImagePtr + 8) = *(double *)(curSrcImagePtr + 8);
					*(double *)(curDstImagePtr + 12) = *(double *)(curSrcImagePtr + 12);
					curDstImagePtr += 16;
					curSrcImagePtr += 16;
				}
				else
				{
					for(i = 16; i-- > 0;)
					{
						if(pixelTouchVal & (1 << i))
						{
							*curDstImagePtr = *curSrcImagePtr;
						}
						else
						{
							*curDstImagePtr = 0; //XXX - get real clear value someday
						}
						curDstImagePtr++;
						curSrcImagePtr++;
					}
				}
			}

			//XXX - someday handle extra pixels
		}

	}

	#else

		if(drawContextPrivate->screenFlags & M3cDrawContextScreenFlags_DoubleBuffer)
		{
			MSrPPCAsm_CallProc(
				platformSpecific->imageBufferCopyAndClearProc,
				drawContextPrivate,
				(long)&platformSpecific->imageClearValue0);

			MSrPPCAsm_CallProc(
				platformSpecific->zBufferClearProc,
				drawContextPrivate,
				(long)&platformSpecific->zClearValue0);

		}

	#endif

	return UUcError_None;
}

UUtError
MSrDrawContext_Method_Frame_Sync(
	M3tDrawContext		*drawContext)
{

	return UUcError_None;
}
