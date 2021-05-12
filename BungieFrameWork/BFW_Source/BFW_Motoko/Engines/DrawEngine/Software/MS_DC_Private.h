/*
	FILE:	MS_DrawContext_Private.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 17, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MS_DRAWCONTEXT_PRIVATE_H
#define MS_DRAWCONTEXT_PRIVATE_H

#include "BFW.h"
#include "BFW_Motoko.h"

#if UUmPlatform == UUmPlatform_Mac

	typedef void (*MStBufferProcPtr)(
		struct MtDrawContext	*drawContext);
	
	typedef struct MStPlatformSpecific
	{
		
		UUtUns16			*frontImageBaseAddr;
		UUtInt32			frontImageRowBytes;
		
		void				*realImageBufferMemory;

		MStBufferProcPtr	zBufferClearProc;
		MStBufferProcPtr	imageBufferClearProc;
		MStBufferProcPtr	imageBufferCopyProc;
		MStBufferProcPtr	imageBufferCopyAndClearProc;
		
		void				*zBufferClearProcMemory;
		void				*imageBufferClearProcMemory;
		void				*imageBufferCopyProcMemory;
		void				*imageBufferCopyAndClearProcMemory;
		
		UUtUns32			imageClearValue0;
		UUtUns32			imageClearValue1;
	
		void				*realZBufferMemory;

		UUtUns32			zClearValue0;
		UUtUns32			zClearValue1;
		
	} MStPlatformSpecific;

#elif UUmPlatform == UUmPlatform_Win32

	typedef struct MStPlatformSpecific
	{
		LPDIRECTDRAW		dd;
	
		LPDIRECTDRAWSURFACE	frontBuffer;
		LPDIRECTDRAWSURFACE	backBuffer;
		LPDIRECTDRAWSURFACE zBuffer;
		UUtInt32			bpp;
		RECT				rect;
		DWORD				top;
		DWORD				left;
		
		UUtUns32			imageClearValue0;
		UUtUns32			imageClearValue1;
		
		UUtUns32			zClearValue0;
		UUtUns32			zClearValue1;

	} MStPlatformSpecific;


#else

	#error help me

#endif

typedef struct MStDrawContextPrivate
{
	long			width;
	long			height;

	M3tDrawContextType			contextType;
	
	/* Platform specific struct */
		MStPlatformSpecific		platformSpecific;
	
	/* Rasterization stuff */
		UUtUns16				*imageBufferBaseAddr;
		UUtUns16				*zBufferBaseAddr;
		UUtUns32				*pixelTouchBaseAddr;

		UUtUns16				imageBufferRowBytes;
		UUtUns16				zBufferRowBytes;
		UUtUns16				pixelTouchRowBytes;
		
	/* Array data */
		void*					statePtr[M3cDrawStatePtrType_NumTypes];
		UUtInt32				stateInt[M3cDrawStateIntType_NumTypes];
		
} MStDrawContextPrivate;

typedef struct MStTextureMapPrivate
{
	UUtUns16		nWidthBits;
	UUtUns16		nHeightBits;
	
} MStTextureMapPrivate;

#endif /* MS_DRAWCONTEXT_PRIVATE_H */
