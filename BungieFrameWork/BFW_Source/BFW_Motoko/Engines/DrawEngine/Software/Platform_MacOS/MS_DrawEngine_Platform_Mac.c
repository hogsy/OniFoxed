/*
	FILE:	MS_DrawEngine_Platform_Mac.c

	AUTHOR:	Brent H. Pease

	CREATED: May 19, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"

#include "Motoko_Manager.h"

#include "MS_DC_Private.h"

#include "MS_DrawEngine_Platform.h"

#include "MS_Mac_CustomProc.h"
#include "MS_Mac_PPCAsm.h"

UUtError MSrDrawEngine_Platform_SetupDrawContextPrivate(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	MStDrawContextPrivate*		inDrawContextPrivate,
	UUtBool						inFullScreen)
{
	UUtError			errorCode = UUcError_None;

	MStPlatformSpecific	*platformSpecific;
	PixMap				*devicePixMap;
	UUtUns32			frontBufferBaseAddrAlignment;
	UUtUns32			frontBufferRowBytesAlignment;
	UUtUns32			rowBytesCacheLineAligned;
	M3tDrawContextScreenFlags	screenFlags;

	UUtUns16				drawEngineIndex;
	UUtUns16				displayIndex;
	UUtUns16				modeIndex;
	M3tDrawEngineCapList*	engineCapList;

	GDHandle	gDevice;

	platformSpecific = &inDrawContextPrivate->platformSpecific;

	platformSpecific->zBufferClearProcMemory			= NULL;
	platformSpecific->imageBufferClearProcMemory		= NULL;
	platformSpecific->imageBufferCopyProcMemory			= NULL;
	platformSpecific->imageBufferCopyAndClearProcMemory	= NULL;

	platformSpecific->realImageBufferMemory				= NULL;
	platformSpecific->realZBufferMemory					= NULL;

	screenFlags = M3cDrawContextScreenFlags_None;

	/*
	 * This is a total hack and needs a cleaner solution someday
	 */
		M3rManager_GetActiveDrawEngine(
			&drawEngineIndex,
			&displayIndex,
			&modeIndex);

		engineCapList = M3rDrawEngine_GetCapList();

		gDevice = (GDHandle)engineCapList->drawEngines[drawEngineIndex].displayDevices[displayIndex].platformDevice;

	if(inDrawContextDescriptor->type == M3cDrawContextType_OnScreen)
	{
		screenFlags = inDrawContextDescriptor->drawContext.onScreen.flags;

		devicePixMap = *(*gDevice)->gdPMap;

		inDrawContextPrivate->width = inDrawContextDescriptor->drawContext.onScreen.rect.right - inDrawContextDescriptor->drawContext.onScreen.rect.left;
		inDrawContextPrivate->height = inDrawContextDescriptor->drawContext.onScreen.rect.bottom - inDrawContextDescriptor->drawContext.onScreen.rect.top;

		if(screenFlags & M3cDrawContextScreenFlags_DoubleBuffer)
		{
			platformSpecific->frontImageRowBytes = devicePixMap->rowBytes & 0x3fff;
			platformSpecific->frontImageBaseAddr = (UUtUns16 *)(devicePixMap->baseAddr +
				inDrawContextDescriptor->drawContext.onScreen.rect.left * M3cDrawRGBBytesPerPixel +
				inDrawContextDescriptor->drawContext.onScreen.rect.top * platformSpecific->frontImageRowBytes);

			/*
			 * Get the front image 8 byte alignment
			 */
				frontBufferBaseAddrAlignment = (unsigned long)platformSpecific->frontImageBaseAddr & 0x07;
				frontBufferRowBytesAlignment = platformSpecific->frontImageRowBytes & 0x07;

			/*
			 * Compute the rowBytes thats aligned to a cache line
			 */
				rowBytesCacheLineAligned =
					(inDrawContextPrivate->width * M3cDrawRGBBytesPerPixel +
						UUcProcessor_CacheLineSize - 1) & ~UUcProcessor_CacheLineSize_Mask;

			rowBytesCacheLineAligned |= frontBufferRowBytesAlignment;

			/*
			 * Allocate the backBuffer memory including extra memory for alignment
			 */
				platformSpecific->realImageBufferMemory =
					UUrMemory_Block_New(inDrawContextPrivate->height * rowBytesCacheLineAligned +
						UUcProcessor_CacheLineSize);

				if(platformSpecific->realImageBufferMemory == NULL)
				{
					UUrError_Report(UUcError_OutOfMemory, "Could not allocate back buffer");
					return UUcError_OutOfMemory;
				}

			/*
			 * Start alignment process on a cache line boundary
			 */
				inDrawContextPrivate->imageBufferBaseAddr =
					(unsigned short *)(((unsigned long)platformSpecific->realImageBufferMemory +
						UUcProcessor_CacheLineSize - 1) & ~UUcProcessor_CacheLineSize_Mask);

			/*
			 * Add in the front image base addr alignment
			 */
				inDrawContextPrivate->imageBufferBaseAddr = (unsigned short *)(
					(unsigned long)inDrawContextPrivate->imageBufferBaseAddr |
						frontBufferBaseAddrAlignment);

			inDrawContextPrivate->imageBufferRowBytes = rowBytesCacheLineAligned;
		}
		else
		{
			inDrawContextPrivate->imageBufferRowBytes = devicePixMap->rowBytes & 0x3fff;
			inDrawContextPrivate->imageBufferBaseAddr = (UUtUns16 *)(devicePixMap->baseAddr +
				inDrawContextDescriptor->drawContext.onScreen.rect.left * M3cDrawRGBBytesPerPixel +
				inDrawContextDescriptor->drawContext.onScreen.rect.top * inDrawContextPrivate->imageBufferRowBytes);
		}

		#if !(defined(MSmPixelTouch) && MSmPixelTouch)

			if(screenFlags & M3cDrawContextScreenFlags_DoubleBuffer)
			{
				errorCode = MSrMacCustomProc_BuildImageBufferCopy(inDrawContextPrivate);
				UUmError_ReturnOnErrorMsg(errorCode, "Could not build image buffer copy proc");

				errorCode = MSrMacCustomProc_BuildImageBufferCopyAndClear(inDrawContextPrivate);
				UUmError_ReturnOnErrorMsg(errorCode, "Could not build image buffer copy and clear proc");
			}

		#endif
	}
	else
	{
		inDrawContextPrivate->width = inDrawContextDescriptor->drawContext.offScreen.inWidth;
		inDrawContextPrivate->height = inDrawContextDescriptor->drawContext.offScreen.inHeight;

		inDrawContextPrivate->imageBufferRowBytes = platformSpecific->frontImageRowBytes =
			(inDrawContextPrivate->width * sizeof(UUtUns16) + UUcProcessor_CacheLineSize - 1) &
				~UUcProcessor_CacheLineSize_Mask;

		inDrawContextPrivate->imageBufferBaseAddr = platformSpecific->realImageBufferMemory = platformSpecific->frontImageBaseAddr =
			UUrMemory_Block_New(inDrawContextPrivate->height * platformSpecific->frontImageRowBytes);
		if(platformSpecific->frontImageBaseAddr == NULL)
		{
			return UUcError_OutOfMemory;
		}

		inDrawContextDescriptor->drawContext.offScreen.outBaseAddr = inDrawContextPrivate->imageBufferBaseAddr;
		inDrawContextDescriptor->drawContext.offScreen.outRowBytes = inDrawContextPrivate->imageBufferRowBytes;
	}

	platformSpecific->imageClearValue0 = 0;
	platformSpecific->imageClearValue1 = 0;

	/*
	 * Allocate ZBuffer
	 */
		platformSpecific->realZBufferMemory =
			UUrMemory_Block_New(inDrawContextPrivate->width * inDrawContextPrivate->height * M3cDrawZBytesPerPixel +
				UUcProcessor_CacheLineSize);

		if(platformSpecific->realZBufferMemory == NULL)
		{
			UUrError_Report(UUcError_OutOfMemory, "Could not allocate z buffer");
			return UUcError_OutOfMemory;
		}

		inDrawContextPrivate->zBufferBaseAddr = (UUtUns16 *)(
			((unsigned long)platformSpecific->realZBufferMemory +
				UUcProcessor_CacheLineSize - 1) & ~UUcProcessor_CacheLineSize_Mask);

		inDrawContextPrivate->zBufferRowBytes = inDrawContextPrivate->width * M3cDrawZBytesPerPixel;

		platformSpecific->zClearValue0 = 0xFFFFFFFF;
		platformSpecific->zClearValue1 = 0xFFFFFFFF;

	#if !(defined(MSmPixelTouch) && MSmPixelTouch)

		errorCode = MSrMacCustomProc_BuildZBufferClear(inDrawContextPrivate);
		UUmError_ReturnOnErrorMsg(errorCode, "Could not build z buffer clear proc");

		errorCode = MSrMacCustomProc_BuildImageBufferClear(inDrawContextPrivate);
		UUmError_ReturnOnErrorMsg(errorCode, "Could not build image buffer clear proc");

		if(screenFlags & M3cDrawContextScreenFlags_DoubleBuffer)
		{
			MSrPPCAsm_CallProc(
				platformSpecific->imageBufferClearProc,
				inDrawContextPrivate,
				0);
			MSrPPCAsm_CallProc(
				platformSpecific->zBufferClearProc,
				inDrawContextPrivate,
				(long)&platformSpecific->zClearValue0);
		}

	#else

		inDrawContextPrivate->pixelTouchRowBytes = (inDrawContextPrivate->width + 7) >> 3;
		inDrawContextPrivate->pixelTouchRowBytes = (inDrawContextPrivate->pixelTouchRowBytes +
													UUcProcessor_CacheLineSize - 1) & ~UUcProcessor_CacheLineSize_Mask;
		inDrawContextPrivate->pixelTouchBaseAddr = (UUtUns32 *)UUrMemory_Block_New(
													inDrawContextPrivate->pixelTouchRowBytes * inDrawContextPrivate->height);

		if(inDrawContextPrivate->pixelTouchBaseAddr == NULL)
		{
			UUrError_Report(UUcErrorSeverity_Fatal_Continue, UUcError_OutOfMemory, "Could not allocate pixel touch buffer");
			return UUcError_OutOfMemory;
		}

	#endif

	return UUcError_None;
}

void MSrDrawEngine_Platform_DestroyDrawContextPrivate(
	MStDrawContextPrivate*	inDrawContextPrivate)
{
	MStPlatformSpecific	*platformSpecific;

	platformSpecific = &inDrawContextPrivate->platformSpecific;

	if(platformSpecific->realImageBufferMemory != NULL)
	{
		UUrMemory_Block_Delete(platformSpecific->realImageBufferMemory);
	}

	if(platformSpecific->realZBufferMemory != NULL)
	{
		UUrMemory_Block_Delete(platformSpecific->realZBufferMemory);
	}

	if(platformSpecific->zBufferClearProcMemory != NULL)
	{
		UUrMemory_Block_Delete(platformSpecific->zBufferClearProcMemory);
	}

	if(platformSpecific->imageBufferClearProcMemory != NULL)
	{
		UUrMemory_Block_Delete(platformSpecific->imageBufferClearProcMemory);
	}

	if(platformSpecific->imageBufferCopyProcMemory != NULL)
	{
		UUrMemory_Block_Delete(platformSpecific->imageBufferCopyProcMemory);
	}

	if(platformSpecific->imageBufferCopyAndClearProcMemory != NULL)
	{
		UUrMemory_Block_Delete(platformSpecific->imageBufferCopyAndClearProcMemory);
	}

	#if defined(MSmPixelTouch) && MSmPixelTouch

		if(inDrawContextPrivate->pixelTouchBaseAddr != NULL)
		{
			UUrMemory_Block_Delete(inDrawContextPrivate->pixelTouchBaseAddr);
		}

	#endif

}


UUtError
MSrDrawEngine_Platform_SetupEngineCaps(
	M3tDrawEngineCaps	*ioDrawEngineCaps)
{
	GDHandle			curGDevice;
	M3tDisplayDevice*	curDisplayDevice;

	ioDrawEngineCaps->numDisplayDevices = 0;

	for(curGDevice = GetDeviceList();
		curGDevice != NULL;
		curGDevice = GetNextDevice(curGDevice))
	{
		if(ioDrawEngineCaps->numDisplayDevices >= M3cMaxDisplayDevices)
		{
			break;
		}

		if((*(*curGDevice)->gdPMap)->pixelSize != 16)
		{
			continue;
		}

		curDisplayDevice = ioDrawEngineCaps->displayDevices + ioDrawEngineCaps->numDisplayDevices++;

		// XXX - Make this real
		curDisplayDevice->platformDevice = (void*)curGDevice;
		curDisplayDevice->numDisplayModes = 1;
		curDisplayDevice->displayModes[0].width = MSgResX;
		curDisplayDevice->displayModes[0].height = MSgResY;
		curDisplayDevice->displayModes[0].bitDepth = 16;

	}

	if(ioDrawEngineCaps->numDisplayDevices == 0)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find a monitor with bit depth 16");
	}

	return UUcError_None;
}

