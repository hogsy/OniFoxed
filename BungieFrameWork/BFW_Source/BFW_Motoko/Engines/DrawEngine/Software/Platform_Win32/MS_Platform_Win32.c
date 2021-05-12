/*
	FILE:	MS_Platform_Mac.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: May 19, 1997
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997

*/

// ======================================================================
// includes
// ======================================================================
#include "BFW_Motoko.h"

#include "MS_DC_Private.h"
#include "MS_DC_Method_Frame.h"
#include "MS_DrawEngine_Platform.h"

#include "Motoko_Manager.h"
#include "Motoko_Platform_Win32.h"

// ======================================================================
// globals
// ======================================================================
static UUtBool					MSgOS_NT_4;

// ======================================================================
// prototypes
// ======================================================================
UUtError
MSiInitBuffers(
	MStDrawContextPrivate		*inDrawContextPrivate);
	
static BOOL PASCAL
MSiDeviceCallback(
	LPGUID						inGuid,
	LPSTR						inDescription,
	LPSTR						inName,
	LPVOID						inData);

static HRESULT PASCAL
MSiModeCallback(
	LPDDSURFACEDESC				inSurfaceDesc,
	LPVOID						inData);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void MSiPlatform_Win32_ClearZBuffer(
	MStDrawContextPrivate		*drawContextPrivate)
{
	UUtInt32	numDoubles;
	double		*dPtr;
	double		clearValue;
	
	numDoubles = drawContextPrivate->height * drawContextPrivate->zBufferRowBytes / sizeof(double);
	dPtr = (double *)drawContextPrivate->zBufferBaseAddr;
	
	clearValue = *(double *)&drawContextPrivate->platformSpecific.zClearValue0;
	
	while(numDoubles-- > 0)
	{
		*dPtr++ = clearValue;
	}
}

// ----------------------------------------------------------------------
static void MSiPlatform_Win32_ClearImageBuffer(
	MStDrawContextPrivate		*drawContextPrivate)
{
	UUtInt32	numDoubles;
	double		*dPtr;
	double		clearValue;
	
	numDoubles = drawContextPrivate->height * drawContextPrivate->imageBufferRowBytes / sizeof(double);
	dPtr = (double *)drawContextPrivate->imageBufferBaseAddr;
	
	clearValue = *(double *)&drawContextPrivate->platformSpecific.imageClearValue0;
	
	while(numDoubles-- > 0)
	{
		*dPtr++ = clearValue;
	}
}

// ----------------------------------------------------------------------
UUtError
MSrDrawEngine_Platform_SetupDrawContextPrivate(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	MStDrawContextPrivate*		inDrawContextPrivate,
	UUtBool						inFullScreen)
{
	MStPlatformSpecific 			*platformSpecific = &inDrawContextPrivate->platformSpecific;
	HRESULT							result = DD_OK;
	UUtError						error;
	UUtUns32						win_flags;
	M3tDrawContextDescriptorOnScreen	*onScreen;
	OSVERSIONINFO					info;
	
	M3tDrawEngineCaps				*current_draw_engine_caps;
	M3tDrawEngineCapList			*draw_engine_caps_list;
	UUtUns16						activeDrawEngine;
	UUtUns16						activeDevice;
	UUtUns16						activeMode;

	// get a pointer to onScreen
	onScreen = &inDrawContextDescriptor->drawContext.onScreen;
	
	// set use MSgOS_NT_4 to false
	MSgOS_NT_4 = UUcFalse;
	
	// find out which OS the game is running under
	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&info))
	{
		// Are we running under NT4
		if (info.dwPlatformId == VER_PLATFORM_WIN32_NT)
			MSgOS_NT_4 = UUcTrue;
	}
	
	// ----------------------------------------
	// Create the direct draw object
	// ----------------------------------------
	
	// get a pointer tothe draw engine caps list
	draw_engine_caps_list = M3rDrawEngine_GetCapList();
	
	UUmAssert(draw_engine_caps_list);
	
	// get the index of the active draw engine
	M3rManager_GetActiveDrawEngine(&activeDrawEngine, &activeDevice, &activeMode);
	
	// get a pointer to the current draw engine's caps
	current_draw_engine_caps = &draw_engine_caps_list->drawEngines[activeDrawEngine];
	
	// create the DirectDraw object
	if (MSgOS_NT_4)
	{
		// running under NT 4.0
		result =
			DirectDrawCreate(
				NULL,
				&platformSpecific->dd,
				NULL);	
	}
	else
	{
		// not running under NT 4.0
		result =
			DirectDrawCreate(
				(LPGUID)current_draw_engine_caps->displayDevices[activeDevice].platformDevice,
				&platformSpecific->dd,
				NULL);	
	}
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	// ----------------------------------------
	// Setup the connection to the window
	// ----------------------------------------
	
	// set the flags for the window
	if (inFullScreen == UUcTrue)
	{
		win_flags = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN;
	}
	else
	{
		win_flags = DDSCL_NOWINDOWCHANGES | DDSCL_NORMAL;
	}
	
	// set the cooperative level
	result =
		IDirectDraw_SetCooperativeLevel(
			platformSpecific->dd,
			inDrawContextDescriptor->drawContext.onScreen.window,
			win_flags);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	// ----------------------------------------
	// setup the buffers and whatnots
	// ----------------------------------------
	platformSpecific->imageClearValue0 = 0;
	platformSpecific->imageClearValue1 = 0;

	
	if (inDrawContextDescriptor->type == M3cDrawContextType_OnScreen)
	{
		// set dimension and location data
		inDrawContextPrivate->width =
			inDrawContextDescriptor->drawContext.onScreen.rect.right -
			inDrawContextDescriptor->drawContext.onScreen.rect.left;
		inDrawContextPrivate->height =
			inDrawContextDescriptor->drawContext.onScreen.rect.bottom -
			inDrawContextDescriptor->drawContext.onScreen.rect.top;

		platformSpecific->left =
			inDrawContextDescriptor->drawContext.onScreen.rect.left;
		platformSpecific->top =
			inDrawContextDescriptor->drawContext.onScreen.rect.top;

		platformSpecific->rect.left = 0;
		platformSpecific->rect.top = 0;
		platformSpecific->rect.right = inDrawContextPrivate->width;
		platformSpecific->rect.bottom = inDrawContextPrivate->height;

		// Initialzie the buffers
		error = MSiInitBuffers(inDrawContextPrivate);
		UUmError_ReturnOnErrorMsg(error, "Unable to init the buffers.");
	}
	else
	{
		UUmAssert(!"Must be onscreen");
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void MSrDrawEngine_Platform_DestroyDrawContextPrivate(
	MStDrawContextPrivate		*inDrawContextPrivate)
{
	if (inDrawContextPrivate)
	{
		RELEASE(inDrawContextPrivate->platformSpecific.zBuffer);
		RELEASE(inDrawContextPrivate->platformSpecific.backBuffer);
		RELEASE(inDrawContextPrivate->platformSpecific.frontBuffer);
		RELEASE(inDrawContextPrivate->platformSpecific.dd);
	}
}

// ----------------------------------------------------------------------
UUtError
MSrDrawContext_Method_Frame_Start(
	M3tDrawContext				*inDrawContext)
{
	MStDrawContextPrivate		*drawContextPrivate = (MStDrawContextPrivate*)inDrawContext->privateContext;
	MStPlatformSpecific			*platformSpecific = &drawContextPrivate->platformSpecific;
	DDSURFACEDESC				ddsd;
//	DDBLTFX						dd_blt_fx;
	HRESULT						ddResult = DD_OK;
	
	// use the blitter to do a color fill on the backbuffer
/*	dd_blt_fx.dwSize = sizeof(DDBLTFX);
	dd_blt_fx.dwFillColor = platformSpecific->imageClearValue0;
	ddResult = IDirectDrawSurface_Blt(
		platformSpecific->backBuffer,
		NULL,
		NULL,
		NULL,
		DDBLT_COLORFILL | DDBLT_WAIT,
		&dd_blt_fx);
	if (ddResult != DD_OK)
	{
		UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(ddResult));
	}
	

	// use the blitter to do a color fill on the backbuffer
	dd_blt_fx.dwSize = sizeof(DDBLTFX);
	dd_blt_fx.dwFillColor = platformSpecific->zClearValue0;
	ddResult = IDirectDrawSurface_Blt(
		platformSpecific->zBuffer,
		NULL,
		NULL,
		NULL,
		DDBLT_COLORFILL | DDBLT_WAIT,
		&dd_blt_fx);
	if (ddResult != DD_OK)
	{
		UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(ddResult));
	}*/
	
	if(drawContextPrivate->screenFlags & M3cDrawContextScreenFlags_DoubleBuffer)
	{
		// lock the backBuffer
		UUrMemory_Clear(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		
		ddResult = IDirectDrawSurface_Lock(
			platformSpecific->backBuffer,
			NULL,
			&ddsd,
			0,
			NULL);
		if (ddResult != DD_OK)
		{
			UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(ddResult));
		}
	
		
		drawContextPrivate->imageBufferBaseAddr = ddsd.lpSurface;
		drawContextPrivate->imageBufferRowBytes = (UUtUns16)ddsd.lPitch;
		
		// lock the zBuffer
		UUrMemory_Clear(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		
		ddResult = IDirectDrawSurface_Lock(
			platformSpecific->zBuffer,
			NULL,
			&ddsd,
			0,
			NULL);
		if (ddResult != DD_OK)
		{
			UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(ddResult));
		}
		
		drawContextPrivate->zBufferBaseAddr = ddsd.lpSurface;
		drawContextPrivate->zBufferRowBytes = (UUtUns16)ddsd.lPitch;
		
		// clear the imageBuffer
		MSiPlatform_Win32_ClearImageBuffer(drawContextPrivate);
		
		// clear the zBuffer
		MSiPlatform_Win32_ClearZBuffer(drawContextPrivate);
	}
	else
	{
		UUmAssert(!"Implement me");
	}
			
	return UUcError_None;
}
	
// ----------------------------------------------------------------------
UUtError
MSrDrawContext_Method_Frame_End(
	M3tDrawContext		*inDrawContext)
{
	MStDrawContextPrivate*	drawContextPrivate = (MStDrawContextPrivate*)inDrawContext->privateContext;
	MStPlatformSpecific*	platformSpecific = &drawContextPrivate->platformSpecific;

	HRESULT					ddResult = DD_OK;
	
	if(drawContextPrivate->screenFlags & M3cDrawContextScreenFlags_DoubleBuffer)
	{
		// unlock the zBuffer
		ddResult =
			IDirectDrawSurface_Unlock(
				platformSpecific->zBuffer,
				NULL);
		if (ddResult != DD_OK)
		{
			UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(ddResult));
		}

		// unlock the backBuffer
		ddResult =
			IDirectDrawSurface_Unlock(
				platformSpecific->backBuffer,
				NULL);
		if (ddResult != DD_OK)
		{
			UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(ddResult));
		}
		
		ddResult =
			IDirectDrawSurface_BltFast(
				platformSpecific->frontBuffer,
				platformSpecific->top,
				platformSpecific->left,
				platformSpecific->backBuffer,
				&platformSpecific->rect,
				DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT); /* XXX - Eventually get rid of the wait */
		if (ddResult != DD_OK)
		{
			UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(ddResult));
		}
	}
	else
	{
		UUmAssert(!"Implement me");
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
MSrDrawContext_Method_Frame_Sync(
	M3tDrawContext		*drawContext)
{
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
MSrDrawEngine_Platform_SetupEngineCaps(
	M3tDrawEngineCaps	*ioDrawEngineCaps)
{
	HRESULT				result;

	ioDrawEngineCaps->numDisplayDevices = 0;
	
	// enumerate the direct draw devices
	result = DirectDrawEnumerate(MSiDeviceCallback, ioDrawEngineCaps);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
	}
	
	return UUcError_None;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
MSiInitBuffers(
	MStDrawContextPrivate		*inDrawContextPrivate)
{
	DDSURFACEDESC				ddsd;
	HRESULT						result;
	
	// setup the direct draw surface description for the front buffer
	UUrMemory_Clear(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize					= sizeof(DDSURFACEDESC);
	ddsd.dwFlags				= DDSD_CAPS;
	ddsd.ddsCaps.dwCaps			= DDSCAPS_PRIMARYSURFACE;
	
	// create the frontBuffer
	result =
		IDirectDraw_CreateSurface(
			inDrawContextPrivate->platformSpecific.dd,
			&ddsd,
			&inDrawContextPrivate->platformSpecific.frontBuffer,
			NULL);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
		
	inDrawContextPrivate->platformSpecific.bpp = 16;

	// setup the direct draw surface description for the back buffer
	UUrMemory_Clear(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize					= sizeof(DDSURFACEDESC);
	ddsd.dwFlags				= DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps			= DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwHeight				= inDrawContextPrivate->height;
	ddsd.dwWidth				= inDrawContextPrivate->width;

	// create the backBuffer
	result =
		IDirectDraw_CreateSurface(
			inDrawContextPrivate->platformSpecific.dd,
			&ddsd,
			&inDrawContextPrivate->platformSpecific.backBuffer,
			NULL);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	// create the zbuffer
	UUrMemory_Clear(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize				= sizeof(DDSURFACEDESC);
//	ddsd.dwFlags			= DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_ZBUFFERBITDEPTH;
//	ddsd.ddsCaps.dwCaps		= DDSCAPS_ZBUFFER | DDSCAPS_SYSTEMMEMORY;
	ddsd.dwFlags			= DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps		= DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth			= inDrawContextPrivate->width;
	ddsd.dwHeight			= inDrawContextPrivate->height;
	ddsd.dwZBufferBitDepth	= inDrawContextPrivate->platformSpecific.bpp;
	
	result =
		IDirectDraw_CreateSurface(
			inDrawContextPrivate->platformSpecific.dd,
			&ddsd,
			&inDrawContextPrivate->platformSpecific.zBuffer,
			NULL);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	inDrawContextPrivate->zBufferBaseAddr = ddsd.lpSurface;
	inDrawContextPrivate->zBufferRowBytes = (UUtUns16)ddsd.lPitch;
	
	inDrawContextPrivate->platformSpecific.zClearValue0 = 0xFFFFFFFF;
	inDrawContextPrivate->platformSpecific.zClearValue1 = 0xFFFFFFFF;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static BOOL PASCAL
MSiDeviceCallback(
	LPGUID						inGuid,
	LPSTR						inDescription,
	LPSTR						inName,
	LPVOID						inData)
{
	M3tDrawEngineCaps			*draw_engine_caps;
	UUtUns32					index;
	LPDIRECTDRAW				dd;
	HRESULT						result;
	
	// if inData is null, something went terribly wrong
	if (inData == NULL)
	{
		return DDENUMRET_OK;
	}
	
	// get a pointer to the draw engine caps
	draw_engine_caps = (M3tDrawEngineCaps*)inData;
	
	// get an index into the display devices array
	index = draw_engine_caps->numDisplayDevices;
	if (index >= M3cMaxDisplayDevices)
	{
		return DDENUMRET_CANCEL;
	}
	
	// initialize the display device
	draw_engine_caps->displayDevices[index].platformDevice = (LPVOID)inGuid;
	draw_engine_caps->displayDevices[index].numDisplayModes = 0;
		
	// create the DirectDraw object
	result = DirectDrawCreate(inGuid, &dd, NULL);
	if (result != DD_OK)
	{
		UUrError_Report(
			UUcError_DirectDraw,
			M3rPlatform_GetErrorMsg(result));
		goto cleanup;
	}

	// enumerate the display modes
	result =
		IDirectDraw_EnumDisplayModes(
			dd,
			0,
			NULL,
			&draw_engine_caps->displayDevices[index],
			MSiModeCallback);
	if (result != DD_OK)
	{
		UUrError_Report(
			UUcError_DirectDraw,
			M3rPlatform_GetErrorMsg(result));
		goto cleanup;
	}

	// increment the number of display devices
	draw_engine_caps->numDisplayDevices++;
	

cleanup:
	// cleanup before leaving
	if (dd)
	{
		IDirectDraw_Release(dd);
		dd = NULL;
	}
	
	return DDENUMRET_OK;	
}

// ----------------------------------------------------------------------
static HRESULT PASCAL
MSiModeCallback(
	LPDDSURFACEDESC				inSurfaceDesc,
	LPVOID						inData)
{
	M3tDisplayDevice			*display_device;
	UUtUns16					index;
	
	// if inData is NULL, something went terribly wrong
	if (inData == NULL)
	{
		return DDENUMRET_OK;
	}
	
	// if the surface description is null something went wrong
	if (inSurfaceDesc == NULL)
	{
		return DDENUMRET_CANCEL;
	}
	
	// double check structure size
	if (inSurfaceDesc->dwSize != sizeof(DDSURFACEDESC))
	{
		return DDENUMRET_CANCEL;
	}
	
	// only use surfaces that have 16 bit pixels
	if (inSurfaceDesc->ddpfPixelFormat.dwRGBBitCount != 16)
	{
		return DDENUMRET_OK;
	}
	
	// get a pointer to the display device
	display_device = (M3tDisplayDevice*)inData;
	
	// get an index into the display modes array
	index = display_device->numDisplayModes;
	if (index >= M3cMaxDisplayModes)
	{
		return DDENUMRET_CANCEL;
	}
	
	// fill in info about the display mode
	display_device->displayModes[index].width = (UUtUns16)inSurfaceDesc->dwWidth;
	display_device->displayModes[index].height = (UUtUns16)inSurfaceDesc->dwHeight;
	display_device->displayModes[index].bitDepth =
		(UUtUns16)inSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;
	display_device->displayModes[index].pad = 0;
	
	// increment the number of display devices
	display_device->numDisplayModes++;
	
	return DDENUMRET_OK;
}
