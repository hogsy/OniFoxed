/*
	FILE:	MD_DrawEngine_Method.c
	
	AUTHOR:	Kevin Armstrong
	
	CREATED: January 5, 1998
	
	PURPOSE: 
	
	Copyright 1997 - 1998

*/

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"

#include "Motoko_Manager.h"

#include "Motoko_Platform_Win32.h"

#include "MD_DC_Private.h"

#include "MD_DrawEngine_Method.h"
#include "MD_DrawEngine_Platform.h"

#include "MD_DC_Method_State.h"
#include "MD_DC_Method_Frame.h"
#include "MD_DC_Method_LinePoint.h"
#include "MD_DC_Method_Triangle.h"
#include "MD_DC_Method_Quad.h"
#include "MD_DC_Method_Query.h"
#include "MD_DC_Method_SmallQuad.h"
#include "MD_DC_Method_Pent.h"
#include "MD_DC_Method_Bitmap.h"

// ======================================================================
// defines
// ======================================================================
#define MDcD3D_Version (0x00010000)

#define MDcMaxD3DDevices	4
#define MDcMaxTextureDesc	8
#define MDcMaxDescChars		256

// ======================================================================
// typedef
// ======================================================================
typedef struct MDtD3D_device_info
{
	GUID					d3d_guid;
	
	char					device_desc[MDcMaxDescChars];
	
	D3DDEVICEDESC			HAL_desc;
	D3DDEVICEDESC			HEL_desc;
	
	UUtUns16				num_texture_formats;
	DDSURFACEDESC			texture_format[MDcMaxTextureDesc];
	
} MDtD3D_device_info;

typedef struct MDtPlatform
{
	UUtBool					primary;
	GUID					dd_guid;

	UUtUns16				num_d3d_devices;
	MDtD3D_device_info		d3d_devices[MDcMaxD3DDevices];
	
} MDtPlatform;

// ======================================================================
// globals
// ======================================================================

// ======================================================================
// prototypes
// ======================================================================
UUtError
MDiInitBuffers(
	MDtDrawContextPrivate		*inDrawContextPrivate,
	M3tDisplayDevice			*inDisplayDevice,
	UUtUns32					inD3DDevice);
	
UUtError
MDiInitD3D(
	MDtDrawContextPrivate		*inDrawContextPrivate,
	M3tDisplayDevice			*inDisplayDevice,
	UUtUns32					inD3DDevice);

UUtError
MDrDrawContext_Method_Frame_Start(
	M3tDrawContext				*inDrawContext);

UUtError
MDrDrawContext_Method_Frame_End(
	M3tDrawContext				*inDrawContext);
	
UUtError
MDrDrawContext_Method_Frame_Sync(
	M3tDrawContext				*drawContext);

UUtError
MDiDrawEngine_SetupEngineCaps(
	M3tDrawEngineCaps	*ioDrawEngineCaps);

static BOOL PASCAL
MDiDD_DriverCallback(
	LPGUID						inGuid,
	LPSTR						inDescription,
	LPSTR						inName,
	LPVOID						inData);

static HRESULT PASCAL
MDiDD_ModeCallback(
	LPDDSURFACEDESC				inSurfaceDesc,
	LPVOID						inData);

static HRESULT PASCAL
MDiD3D_DeviceCallback(
	LPGUID						inGuid,
	LPSTR						inName,
	LPSTR						inDescription,
	LPD3DDEVICEDESC				inHALDesc,
	LPD3DDEVICEDESC				inHELDesc,
	LPVOID						inData);

static HRESULT PASCAL
MDiD3D_TextureFormatCallback(
	LPDDSURFACEDESC				inTextureFormat,
	LPVOID						inData);

UUtInt32
MDiFlagsToBitDepth(
	UUtUns32					inFlags);

UUtBool
MDiIsHardware(
	MDtD3D_device_info			*inD3DDeviceInfo);

UUtUns32
MDiGetD3DDeviceIndex(
	void);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError MDrDrawEngine_SetupDrawContextPrivate(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	MDtDrawContextPrivate*		inDrawContextPrivate,
	UUtBool						inFullScreen)
{
	HRESULT							result = DD_OK;
	UUtError						error;
	UUtUns32						win_flags;
	M3tDrawContextDescriptorOnScreen	*onScreen;

	MDtPlatform						*platform_data;
	M3tDrawEngineCaps				*current_draw_engine_caps;
	M3tDrawEngineCapList			*draw_engine_caps_list;
	UUtUns16						activeDrawEngine;
	UUtUns16						activeDevice;
	UUtUns16						activeMode;
	UUtUns32						d3d_device_index;

	// get a pointer to onScreen
	onScreen = &inDrawContextDescriptor->drawContext.onScreen;
		
	// ----------------------------------------
	// Create the direct draw object
	// ----------------------------------------

	// get a pointer to the draw engine caps list
	draw_engine_caps_list = M3rDrawEngine_GetCapList();
	
	UUmAssert(draw_engine_caps_list);
	
	// get the index of the active draw engine
	M3rManager_GetActiveDrawEngine(&activeDrawEngine, &activeDevice, &activeMode);
	
	// get a pointer to the current draw engine's caps
	current_draw_engine_caps = &draw_engine_caps_list->drawEngines[activeDrawEngine];
	
	// get a pointer to the platform_data
	platform_data = 
		(MDtPlatform*)current_draw_engine_caps->displayDevices[activeDevice].platformDevice;
	
	// create the DirectDraw object
	if (platform_data->primary)
	{
		result =
			DirectDrawCreate(
				NULL,
				&inDrawContextPrivate->dd,
				NULL);
	}
	else
	{
		result =
			DirectDrawCreate(
				&platform_data->dd_guid,
				&inDrawContextPrivate->dd,
				NULL);
	}
	if (result != DD_OK)
	{
		UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
	}
	
	// get the DirectDraw2 object
	result =
		IDirectDraw_QueryInterface(
			inDrawContextPrivate->dd,
			&IID_IDirectDraw2,
			(void**)&inDrawContextPrivate->dd2);
	if (result != DD_OK)
	{
		UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
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
		IDirectDraw2_SetCooperativeLevel(
			inDrawContextPrivate->dd2,
			inDrawContextDescriptor->drawContext.onScreen.window,
			win_flags);
	if (result != DD_OK)
	{
		UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
	}
	
	// ----------------------------------------
	// create the Direct3D interface
	// ----------------------------------------
	
	// get the d3d2 interface from DirectDraw
	result =
		IDirectDraw2_QueryInterface(
			inDrawContextPrivate->dd2,
			&IID_IDirect3D2,
			(void**)&inDrawContextPrivate->d3d2);
	if (result != DD_OK)
	{
		UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
	}

	// ----------------------------------------
	// setup the buffers and whatnots
	// ----------------------------------------
	
	// get the d3d_device index
	d3d_device_index = MDiGetD3DDeviceIndex();
	
	if (inDrawContextDescriptor->type == M3cDrawContextType_OnScreen)
	{
		// XXX - this may need to change to better reflect the activeMode
		// set dimension and location data
		inDrawContextPrivate->width =
			inDrawContextDescriptor->drawContext.onScreen.rect.right -
			inDrawContextDescriptor->drawContext.onScreen.rect.left;
		inDrawContextPrivate->height =
			inDrawContextDescriptor->drawContext.onScreen.rect.bottom -
			inDrawContextDescriptor->drawContext.onScreen.rect.top;

		inDrawContextPrivate->left =
			inDrawContextDescriptor->drawContext.onScreen.rect.left;
		inDrawContextPrivate->top =
			inDrawContextDescriptor->drawContext.onScreen.rect.top;

		inDrawContextPrivate->rect.left = 0;
		inDrawContextPrivate->rect.top = 0;
		inDrawContextPrivate->rect.right = inDrawContextPrivate->width;
		inDrawContextPrivate->rect.bottom = inDrawContextPrivate->height;

		// Initialize the buffers
		error =
			MDiInitBuffers(
				inDrawContextPrivate,
				&current_draw_engine_caps->displayDevices[activeDevice],
				d3d_device_index);
		UUmError_ReturnOnErrorMsg(error, "Unable to init the buffers.");
	
		// Initialize Direct3D
		error =
			MDiInitD3D(
				inDrawContextPrivate,
				&current_draw_engine_caps->displayDevices[activeDevice],
				d3d_device_index);
		UUmError_ReturnOnErrorMsg(error, "Unable to init the buffers.");
	}
	else
	{
		UUmAssert(!"Must be onscreen");
	}

	// ----------------------------------------
	// Set the display mode
	// ----------------------------------------
	
	result =
		IDirectDraw2_SetDisplayMode(
			inDrawContextPrivate->dd2,
			inDrawContextPrivate->width,
			inDrawContextPrivate->height,
			inDrawContextPrivate->zbpp,
			60,
			0);
	UUmError_ReturnOnErrorMsg(error, "Unable to set the display mode.");
	
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void MDrDrawEngine_DestroyDrawContextPrivate(
	MDtDrawContextPrivate		*inDrawContextPrivate)
{
	if (inDrawContextPrivate)
	{
		RELEASE(inDrawContextPrivate->zBuffer);
		RELEASE(inDrawContextPrivate->backBuffer);
		RELEASE(inDrawContextPrivate->frontBuffer);
		RELEASE(inDrawContextPrivate->d3d_viewport2);
		RELEASE(inDrawContextPrivate->d3d_device2);
		RELEASE(inDrawContextPrivate->d3d2);
		RELEASE(inDrawContextPrivate->dd2);
		RELEASE(inDrawContextPrivate->dd);
	}
}

// ----------------------------------------------------------------------
static void
MDrDrawEngine_Method_ContextPrivateDelete(
	M3tDrawContextPrivate		*inDrawContextPrivate)
{
	MDtDrawContextPrivate		*contextPrivate;
	
	contextPrivate = (MDtDrawContextPrivate *)inDrawContextPrivate;

	MDrDrawEngine_DestroyDrawContextPrivate(contextPrivate);
	
	UUrMemory_Block_Delete(inDrawContextPrivate);
}

// ----------------------------------------------------------------------
static UUtError
MDrDrawEngine_Method_ContextPrivateNew(
	M3tDrawContextDescriptor	*inDrawContextDescriptor,
	M3tDrawContextPrivate		**outDrawContextPrivate,
	UUtBool						inFullScreen,
	M3tDrawAPI*					outAPI)
{
	UUtError					errorCode;
	MDtDrawContextPrivate		*newDrawContextPrivate;
	
	*outDrawContextPrivate = NULL;
	*outAPI = M3cDrawAPI_D3D;

	// allocate memory for the Direct3D draw context private data
	newDrawContextPrivate = UUrMemory_Block_New(sizeof(MDtDrawContextPrivate));
	if(newDrawContextPrivate == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not allocate private draw context");
	}
	
	// clear the structure
	UUrMemory_Clear(newDrawContextPrivate, sizeof(MDtDrawContextPrivate));
	
	// set the contextType
	newDrawContextPrivate->contextType = inDrawContextDescriptor->type;
	
	// set the screenFlags
	if (inDrawContextDescriptor->type == M3cDrawContextType_OnScreen)
	{
		newDrawContextPrivate->screenFlags =
			inDrawContextDescriptor->drawContext.onScreen.flags;
	}
	else
	{
		newDrawContextPrivate->screenFlags = M3cDrawContextScreenFlags_None;
	}
	
	// Setup the platform specific data
	errorCode =
		MDrDrawEngine_SetupDrawContextPrivate(
			inDrawContextDescriptor,
			newDrawContextPrivate,
			inFullScreen);
	if (errorCode != UUcError_None)
	{
		goto failure;
	}
	
	// return the new draw context
	*outDrawContextPrivate = (M3tDrawContextPrivate*)newDrawContextPrivate;
	
	return UUcError_None;
	
failure:
	
	MDrDrawEngine_Method_ContextPrivateDelete(
		(M3tDrawContextPrivate *)newDrawContextPrivate);
	
	return errorCode;
}

// ----------------------------------------------------------------------
static UUtError
MDrDrawEngine_Method_ContextMetaHandler(
	M3tDrawContext*				inDrawContext,
	M3tDrawContextMethodType	inMethodType,
	M3tDrawContextMethod*		inMethod)
{
	MDtDrawContextPrivate	*drawContextPrivate;
	
	// get a pointer shortcut
	drawContextPrivate = (MDtDrawContextPrivate *)inDrawContext->privateContext;
	
	switch (inMethodType)
	{
		case M3cDrawContextMethodType_Frame_Start:
			inMethod->frameStart = MDrDrawContext_Method_Frame_Start;
			break;
			
		case M3cDrawContextMethodType_Frame_End:
			inMethod->frameEnd = MDrDrawContext_Method_Frame_End;
			break;
			
		case M3cDrawContextMethodType_Frame_Sync:
			inMethod->frameSync = MDrDrawContext_Method_Frame_Sync;
			break;
			
		case M3cDrawContextMethodType_State_SetInt:
			inMethod->stateSetInt = MDrDrawContext_Method_State_SetInt;
			break;
			
		case M3cDrawContextMethodType_State_GetInt:
			inMethod->stateGetInt = MDrDrawContext_Method_State_GetInt;
			break;
			
		case M3cDrawContextMethodType_State_SetPtr:
			inMethod->stateSetPtr = MDrDrawContext_Method_State_SetPtr;
			break;
			
		case M3cDrawContextMethodType_State_GetPtr:
			inMethod->stateGetPtr = MDrDrawContext_Method_State_GetPtr;
			break;
			
		case M3cDrawContextMethodType_TriInterpolate:
			switch (drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					inMethod->triInterpolate = MDrDrawContext_Method_TriGouraudInterpolate;
					break;
					
				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					inMethod->triInterpolate = MDrDrawContext_Method_TriTextureInterpolate;
					break;
					
				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;
			
		case M3cDrawContextMethodType_TriFlat:
			switch (drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					inMethod->triFlat = MDrDrawContext_Method_TriGouraudFlat;
					break;
					
				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					inMethod->triFlat = MDrDrawContext_Method_TriTextureFlat;
					break;
					
				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;
			
		case M3cDrawContextMethodType_TriSplit:
			inMethod->triSplit = MDrDrawContext_Method_TriTextureSplit;
			break;
			
		case M3cDrawContextMethodType_QuadInterpolate:
			switch (drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					inMethod->quadInterpolate = MDrDrawContext_Method_QuadGouraudInterpolate;
					break;
					
				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					inMethod->quadInterpolate = MDrDrawContext_Method_QuadTextureInterpolate;
					break;
					
				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;
			
		case M3cDrawContextMethodType_QuadFlat:
			switch (drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					inMethod->quadFlat = MDrDrawContext_Method_QuadGouraudFlat;
					break;
					
				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					inMethod->quadFlat = MDrDrawContext_Method_QuadTextureFlat;
					break;
					
				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;
			
		case M3cDrawContextMethodType_QuadSplit:
			inMethod->quadSplit = MDrDrawContext_Method_QuadTextureSplit;
			break;
			
		case M3cDrawContextMethodType_SmallQuadInterpolate:
			switch (drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					inMethod->smallQuadInterpolate = MDrDrawContext_Method_SmallQuadGouraudInterpolate;
					break;
				
				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					inMethod->smallQuadInterpolate = MDrDrawContext_Method_SmallQuadTextureInterpolate;
					break;
					
				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;
			
		case M3cDrawContextMethodType_SmallQuadFlat:
			switch (drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					inMethod->smallQuadFlat = MDrDrawContext_Method_SmallQuadGouraudFlat;
					break;
					
				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					inMethod->smallQuadFlat = MDrDrawContext_Method_SmallQuadTextureFlat;
					break;
					
				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;
			
		case M3cDrawContextMethodType_PentInterpolate:
			switch (drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					inMethod->pentInterpolate = MDrDrawContext_Method_PentGouraudInterpolate;
					break;
					
				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					inMethod->pentInterpolate = MDrDrawContext_Method_PentTextureInterpolate;
					break;
					
				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;
			
		case M3cDrawContextMethodType_PentFlat:
			switch (drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					inMethod->pentFlat = MDrDrawContext_Method_PentGouraudFlat;
					break;
					
				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					inMethod->pentFlat = MDrDrawContext_Method_PentTextureFlat;
					break;
					
				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;
			
		case M3cDrawContextMethodType_PentSplit:
			inMethod->pentSplit = MDrDrawContext_Method_PentTextureSplit;
			break;
			
		case M3cDrawContextMethodType_Line_Interpolate:
			inMethod->lineInterpolate = MDrDrawContext_Method_Line_Interpolate;
			break;
			
		case M3cDrawContextMethodType_Line_Flat:
			inMethod->lineFlat = MDrDrawContext_Method_Line_Flat;
			break;
			
		case M3cDrawContextMethodType_Point:
			inMethod->point = MDrDrawContext_Method_Point;
			break;
		
		case M3cDrawContextMethodType_Bitmap:
			inMethod->bitmap = MDrDrawContext_Method_Bitmap;
			break;
		
		case M3cDrawContextMethodType_TextureFormatAvailable:
			inMethod->textureFormatAvailable = MDrDrawContext_TextureFormatAvailable;
			break;
		
		case M3cDrawContextMethodType_GetWidth:
			inMethod->getWidth = MDrDrawContext_GetWidth;
			break;
		
		case M3cDrawContextMethodType_GetHeight:
			inMethod->getHeight = MDrDrawContext_GetHeight;
			break;
		
		default:
			UUmDebugStr("MDrDrawEngine_Method_ContextMetaHandler: invalid case");
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
MDrTextureFormatAvailable(
	LPDDPIXELFORMAT				inDDPixelFormat)
{
	LPDDPIXELFORMAT					ddpf;
	MDtPlatform						*platform_data;
	M3tDrawEngineCaps				*current_draw_engine_caps;
	M3tDrawEngineCapList			*draw_engine_caps_list;
	UUtUns16						activeDrawEngine;
	UUtUns16						activeDevice;
	UUtUns16						activeMode;
	UUtUns32						d3d_device_index;
	UUtUns32						i;
	
	// get a pointer to the draw engine caps list
	draw_engine_caps_list = M3rDrawEngine_GetCapList();
	
	UUmAssert(draw_engine_caps_list);
	
	// get the index of the active draw engine
	M3rManager_GetActiveDrawEngine(&activeDrawEngine, &activeDevice, &activeMode);
	
	// get a pointer to the current draw engine's caps
	current_draw_engine_caps = &draw_engine_caps_list->drawEngines[activeDrawEngine];
	
	// get a pointer to the platform_data
	platform_data = 
		(MDtPlatform*)current_draw_engine_caps->displayDevices[activeDevice].platformDevice;

	// get the d3d_device index
	d3d_device_index = MDiGetD3DDeviceIndex();
	
	for (i = 0; i < platform_data->d3d_devices[d3d_device_index].num_texture_formats; i++)
	{
		// get a pointer to the pixel format
		ddpf = &platform_data->d3d_devices[d3d_device_index].texture_format[i].ddpfPixelFormat;
	
		// check to see that the important parts are the same
		if ((ddpf->dwFlags & DDPF_RGB) == (inDDPixelFormat->dwFlags & DDPF_RGB))
		{
			if ((ddpf->dwRGBBitCount == inDDPixelFormat->dwRGBBitCount) &&
				(ddpf->dwRBitMask == inDDPixelFormat->dwRBitMask) &&
				(ddpf->dwGBitMask == inDDPixelFormat->dwGBitMask) &&
				(ddpf->dwBBitMask == inDDPixelFormat->dwBBitMask))
			{
				if (inDDPixelFormat->dwFlags & DDPF_ALPHAPIXELS)
				{
					if ((ddpf->dwRGBAlphaBitMask == inDDPixelFormat->dwRGBAlphaBitMask))
						return UUcTrue;
				}
				else
				{
					return UUcTrue;
				}
			}
		}
	}
	
	return UUcFalse;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
MDrDrawEngine_Method_Texture_Init(
	M3tTextureMap*				inTextureMap)
{
	MDtTextureMapPrivate*	privateData;
		
	// get a pointer to the texturemaps private data
	privateData =
		(MDtTextureMapPrivate*)M3rManager_Texture_GetEnginePrivate(
			inTextureMap);
	
	UUmAssert(privateData != NULL);
	
	if(inTextureMap->numLongs > 1 && inTextureMap->width > 0 && inTextureMap->height > 0)
	{
		// clear the variables
		privateData->flags = 0;
		privateData->texture_handle = 0;
		privateData->memory_surface = NULL;
		privateData->device_surface = NULL;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
MDrDrawEngine_Method_Texture_Load(
	M3tTextureMap*				inTextureMap)
{
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
MDrDrawEngine_Method_Texture_Unload(
	M3tTextureMap*				inTextureMap)
{
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
MDrDrawEngine_Method_Texture_Delete(
	M3tTextureMap*				inTextureMap)
{
	MDtTextureMapPrivate*	privateData;
		
	// get a pointer to the texturemaps private data
	privateData =
		(MDtTextureMapPrivate*)M3rManager_Texture_GetEnginePrivate(
			inTextureMap);
	
	UUmAssert(privateData != NULL);
	
	if (privateData->memory_surface)
	{
		IDirectDrawSurface3_Release(privateData->memory_surface);
		privateData->memory_surface = NULL;
	}
	
	if (privateData->device_surface)
	{
		IDirectDrawSurface3_Release(privateData->device_surface);
		privateData->device_surface = NULL;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
MDrDrawEngine_Method_Texture_Update(
	M3tTextureMap*				inTextureMap)
{
	
	return UUcError_None;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
MDrDrawEngine_Initialize(
	void)
{
	UUtError				error;
	OSVERSIONINFO			info;
	M3tDrawEngineCaps		drawEngineCaps;
	M3tDrawEngineMethods	drawEngineMethods;
	
	// find out which OS the game is running under
	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&info))
	{
		// don't make initialize the draw engine if this is NT4
		if (info.dwPlatformId == VER_PLATFORM_WIN32_NT)
			return;
	}

	// set the function pointers
	drawEngineMethods.contextPrivateNew = MDrDrawEngine_Method_ContextPrivateNew;
	drawEngineMethods.contextPrivateDelete = MDrDrawEngine_Method_ContextPrivateDelete;
	drawEngineMethods.contextMetaHandler = MDrDrawEngine_Method_ContextMetaHandler;
	
	drawEngineMethods.textureInit = MDrDrawEngine_Method_Texture_Init;
	drawEngineMethods.textureLoad = MDrDrawEngine_Method_Texture_Load;
	drawEngineMethods.textureUnload = MDrDrawEngine_Method_Texture_Unload;
	drawEngineMethods.textureDelete = MDrDrawEngine_Method_Texture_Delete;
	drawEngineMethods.textureUpdate = MDrDrawEngine_Method_Texture_Update;
	
	// Setup engine caps
	UUrMemory_Clear(&drawEngineCaps, sizeof(M3tDrawEngineCaps));
	
	drawEngineCaps.engineFlags = M3cDrawEngineFlag_3DOnly;
	
	UUrString_Copy(drawEngineCaps.engineName, M3cDrawEngine_D3D, M3cMaxNameLen);
	drawEngineCaps.engineDriver[0] = 0;
	
	drawEngineCaps.engineVersion = MDcD3D_Version;

	// Setup engine caps
	error = MDiDrawEngine_SetupEngineCaps(&drawEngineCaps);
	if(error != UUcError_None)
	{
		UUrError_Report(UUcError_Generic, "Could not setup engine caps");
		return;
	}
		
	// register the Direct3D draw engine
	error =
		M3rManager_Register_DrawEngine(
			sizeof(MDtTextureMapPrivate),
			&drawEngineCaps,
			&drawEngineMethods);
	if(error != UUcError_None)
	{
		UUrError_Report(error, "Could not register Motoko Direct3D rasterizer engine");
		return;
	}
}

// ----------------------------------------------------------------------
void
MDrDrawEngine_Terminate(
	void)
{
}


// ----------------------------------------------------------------------
void
MDrUseTexture(
	MDtDrawContextPrivate		*inDrawContextPrivate,
	M3tTextureMap				*inTextureMap)
{
	MDtTextureMapPrivate		*textureMapPrivate;
	HRESULT						result;

	UUmAssert(inDrawContextPrivate);
	UUmAssert(inTextureMap);
	
	textureMapPrivate =
		(MDtTextureMapPrivate*)(TMmInstance_GetDynamicData(inTextureMap));

	if (inDrawContextPrivate->d3d_device2)
	{
		// be sure that the memory surface hasn't been created yet
		if (textureMapPrivate->memory_surface == NULL)
		{
			DDSURFACEDESC			ddsd;
			LPDIRECTDRAWSURFACE		temp_surface;
			LPDIRECT3DTEXTURE2		texture;
			
			// --------------------------------------------------
			// setup the surface description
			UUrMemory_Clear(&ddsd, sizeof(DDSURFACEDESC));
			ddsd.dwSize				= sizeof(DDSURFACEDESC);
			ddsd.dwFlags			=	DDSD_CAPS |
										DDSD_HEIGHT |
										DDSD_WIDTH |
										DDSD_PIXELFORMAT;
			ddsd.ddsCaps.dwCaps		= 	DDSCAPS_TEXTURE;
			ddsd.dwWidth 			= inTextureMap->width;
			ddsd.dwHeight			= inTextureMap->height;
			ddsd.lPitch				= inTextureMap->rowBytes;
			
			ddsd.ddpfPixelFormat.dwSize				= sizeof(DDPIXELFORMAT);
			ddsd.ddpfPixelFormat.dwFlags			= DDPF_RGB;
			
			switch (inTextureMap->texelType)
			{
				case M3cTextureType_ARGB4444:
					ddsd.ddpfPixelFormat.dwRGBBitCount		= 16;
					ddsd.ddpfPixelFormat.dwFlags			|= DDPF_ALPHAPIXELS;
					ddsd.ddpfPixelFormat.dwRBitMask			= 0x00000F00;
					ddsd.ddpfPixelFormat.dwGBitMask			= 0x000000F0;
					ddsd.ddpfPixelFormat.dwBBitMask			= 0x0000000F;
					ddsd.ddpfPixelFormat.dwRGBAlphaBitMask	= 0x0000F000;
					break;
					
				case M3cTextureType_RGB555:
					ddsd.ddpfPixelFormat.dwRGBBitCount		= 16;
					ddsd.ddpfPixelFormat.dwRBitMask			= 0x00007C00;
					ddsd.ddpfPixelFormat.dwGBitMask			= 0x000003E0;
					ddsd.ddpfPixelFormat.dwBBitMask			= 0x0000001F;
					ddsd.ddpfPixelFormat.dwRGBAlphaBitMask	= 0x00000000;
					break;
					
				case M3cTextureType_ARGB1555:
					ddsd.ddpfPixelFormat.dwRGBBitCount		= 16;
					ddsd.ddpfPixelFormat.dwFlags			|= DDPF_ALPHAPIXELS;
					ddsd.ddpfPixelFormat.dwRBitMask			= 0x00007C00;
					ddsd.ddpfPixelFormat.dwGBitMask			= 0x000003E0;
					ddsd.ddpfPixelFormat.dwBBitMask			= 0x0000001F;
					ddsd.ddpfPixelFormat.dwRGBAlphaBitMask	= 0x00008000;
					break;
					
				case M3cTextureType_ARGB8888:
					ddsd.ddpfPixelFormat.dwRGBBitCount		= 32;
					ddsd.ddpfPixelFormat.dwFlags			|= DDPF_ALPHAPIXELS;
					ddsd.ddpfPixelFormat.dwRBitMask			= 0x00FF0000;
					ddsd.ddpfPixelFormat.dwGBitMask			= 0x0000FF00;
					ddsd.ddpfPixelFormat.dwBBitMask			= 0x000000FF;
					ddsd.ddpfPixelFormat.dwRGBAlphaBitMask	= 0xFF000000;
					break;
				
				default:
					UUmAssert(!"Unknown texture format.");
			}
			
			if (inDrawContextPrivate->isHardware)
			{
				ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD;
			}
			else
			{
				ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
			}
			
			// create the device surface
			result =
				IDirectDraw2_CreateSurface(
					inDrawContextPrivate->dd2,
					&ddsd,
					&temp_surface,
					NULL);
			if (result != D3D_OK)
			{
				UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
				return;
			}
			
			// get the DirectDrawSurface3 interface
			result =
				IDirectDrawSurface_QueryInterface(
					temp_surface,
					&IID_IDirectDrawSurface3,
					(void**)&textureMapPrivate->device_surface);
			
			// --------------------------------------------------
			// create the memory surface -- this one will actually hold the
			// data and it does not get lost.
			if (inDrawContextPrivate->isHardware)
			{
				ddsd.ddsCaps.dwCaps 	= DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE;
				
				// create the memory surface
				result =
					IDirectDraw2_CreateSurface(
						inDrawContextPrivate->dd2,
						&ddsd,
						&temp_surface,
						NULL);
				if (result != D3D_OK)
				{
					UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
					return;
				}
				
				// get an interface to the DirectDrawSurface3
				result =
					IDirectDrawSurface_QueryInterface(
						temp_surface,
						&IID_IDirectDrawSurface3,
						(void**)&textureMapPrivate->memory_surface);
				if (result != D3D_OK)
				{
					UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
					return;
				}
			}
			else
			{
				// when dealing with a SW rasterizer we don't need to make
				// two surfaces
				textureMapPrivate->memory_surface = textureMapPrivate->device_surface;
				IDirectDrawSurface3_AddRef(textureMapPrivate->device_surface);
			}
			
			// set the pointer to the data
			UUrMemory_Clear(&ddsd, sizeof(DDSURFACEDESC));
			ddsd.dwSize				= sizeof(DDSURFACEDESC);
			ddsd.dwFlags			= DDSD_LPSURFACE;
			ddsd.lpSurface			= (void*)inTextureMap->data;
			result =
				IDirectDrawSurface3_SetSurfaceDesc(
					textureMapPrivate->memory_surface,
					&ddsd,
					0);
			if (result != D3D_OK)
			{
				UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
				return;
			}
			
			// --------------------------------------------------
			// query the surface for a texture interface
			result =
				IDirectDrawSurface3_QueryInterface(
					textureMapPrivate->device_surface,
					&IID_IDirect3DTexture2,
					(void**)&texture);
			if (result != D3D_OK)
			{
				UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
				return;
			}
			
			// get a handle to the texture
			result =
				IDirect3DTexture2_GetHandle(
					texture,
					inDrawContextPrivate->d3d_device2,
					&textureMapPrivate->texture_handle);
			if (result != D3D_OK)
			{
				UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
				return;
			}
			
			RELEASE(texture);
		}
		
		// --------------------------------------------------
		// load the texture
		result =
			IDirect3DDevice2_SetRenderState(
				inDrawContextPrivate->d3d_device2,
				D3DRENDERSTATE_TEXTUREHANDLE,
				textureMapPrivate->texture_handle);
		if (result != D3D_OK)
		{
			UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
			return;
		}
	}
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================

// ----------------------------------------------------------------------
static UUtBool				states_set;

UUtError
MDrDrawContext_Method_Frame_Start(
	M3tDrawContext				*inDrawContext)
{
	MDtDrawContextPrivate		*drawContextPrivate;
	HRESULT						result = DD_OK;
	D3DRECT						d3d_rect;
	DDBLTFX						dd_blt_fx;
	
	// get shortcut pointers
	drawContextPrivate = (MDtDrawContextPrivate*)inDrawContext->privateContext;
	
	// use the blitter to do a color fill on the backbuffer
	dd_blt_fx.dwSize = sizeof(DDBLTFX);
	dd_blt_fx.dwFillColor = RGB_MAKE(0,0,255);
	result = IDirectDrawSurface_Blt(
		drawContextPrivate->backBuffer,
		NULL,
		NULL,
		NULL,
		DDBLT_COLORFILL,
		&dd_blt_fx);
	if (result != DD_OK)
	{
		UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
	}

	// clear the z buffer
	d3d_rect.x1 = drawContextPrivate->rect.left;
	d3d_rect.y1 = drawContextPrivate->rect.top;
	d3d_rect.x2 = drawContextPrivate->rect.right;
	d3d_rect.y2 = drawContextPrivate->rect.bottom;
	
	result =
		IDirect3DViewport2_Clear(
			drawContextPrivate->d3d_viewport2,
			1,
			&d3d_rect,
			D3DCLEAR_ZBUFFER);
	if (result != D3D_OK)
	{
		UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
	}

	// begin the Direct3D scene
	result = IDirect3DDevice2_BeginScene(drawContextPrivate->d3d_device2);

// XXX probably shouldn't set these each time
	if (states_set == UUcFalse)
	{
		// Turn off specular highlights
		result =
			IDirect3DDevice2_SetRenderState(
				drawContextPrivate->d3d_device2,
				D3DRENDERSTATE_SPECULARENABLE,
				FALSE);
		if (result != D3D_OK)
		{
			UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		}
		
		// turn on z-buffering
		result =
			IDirect3DDevice2_SetRenderState(
				drawContextPrivate->d3d_device2,
				D3DRENDERSTATE_ZENABLE,
				TRUE);
		if (result != D3D_OK)
		{
			UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		}
		
		// turn off culling
		result =
			IDirect3DDevice2_SetRenderState(
				drawContextPrivate->d3d_device2,
				D3DRENDERSTATE_CULLMODE,
				D3DCULL_NONE);
		if (result != D3D_OK)
		{
			UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		}
		
		// turn on perspective correct texture mapping
		result =
			IDirect3DDevice2_SetRenderState(
				drawContextPrivate->d3d_device2,
				D3DRENDERSTATE_TEXTUREPERSPECTIVE,
				TRUE);
		if (result != D3D_OK)
		{
			UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		}

		// turn on perspective blending
		result =
			IDirect3DDevice2_SetRenderState(
				drawContextPrivate->d3d_device2,
				D3DRENDERSTATE_ALPHABLENDENABLE,
				TRUE);
		if (result != D3D_OK)
		{
			UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		}
		
		// turn on some ambient light
		result =
			IDirect3DDevice2_SetLightState(
				drawContextPrivate->d3d_device2,
				D3DLIGHTSTATE_AMBIENT,
				RGBA_MAKE(10, 10, 10, 10));
		if (result != D3D_OK)
		{
			UUmError_ReturnOnErrorMsg(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		}

		states_set = UUcTrue;
	}
	
	return UUcError_None;
}
	
// ----------------------------------------------------------------------
UUtError
MDrDrawContext_Method_Frame_End(
	M3tDrawContext				*inDrawContext)
{
	MDtDrawContextPrivate		*drawContextPrivate;
	HRESULT						result = DD_OK;

	drawContextPrivate = (MDtDrawContextPrivate*)inDrawContext->privateContext;
	
	// end the Direct3D scene
	result = IDirect3DDevice2_EndScene(drawContextPrivate->d3d_device2);
	
	// draw the rendered scene to the screen
	if(drawContextPrivate->screenFlags & M3cDrawContextScreenFlags_DoubleBuffer)
	{
		result =
			IDirectDrawSurface_BltFast(
				drawContextPrivate->frontBuffer,
				drawContextPrivate->top,
				drawContextPrivate->left,
				drawContextPrivate->backBuffer,
				&drawContextPrivate->rect,
				DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT); /* XXX - Eventually get rid of the wait */
		
		if(result != DD_OK)
		{
			UUrError_Report(UUcError_Generic, M3rPlatform_GetErrorMsg(result));
			return UUcError_Generic;
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
MDrDrawContext_Method_Frame_Sync(
	M3tDrawContext		*drawContext)
{
	return UUcError_None;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
MDiInitBuffers(
	MDtDrawContextPrivate		*inDrawContextPrivate,
	M3tDisplayDevice			*inDisplayDevice,
	UUtUns32					inD3DDevice)
{
	DDSURFACEDESC				ddsd;
	LPD3DDEVICEDESC				d3d_device_desc;
	MDtPlatform					*platform_data;
//	DDSCAPS						ddscaps;
	HRESULT						result;
	UUtUns32					mem_location;
	
	// setup the direct draw surface description for the front buffer
	UUrMemory_Clear(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize					= sizeof(DDSURFACEDESC);
	ddsd.dwFlags				= DDSD_CAPS;
	ddsd.ddsCaps.dwCaps			= DDSCAPS_PRIMARYSURFACE;
	
	// create the frontBuffer
	result =
		IDirectDraw2_CreateSurface(
			inDrawContextPrivate->dd2,
			&ddsd,
			&inDrawContextPrivate->frontBuffer,
			NULL);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	// get a pointer to the platform data
	platform_data = (MDtPlatform*)inDisplayDevice->platformDevice;
	
	// get the D3D device description
	if (MDiIsHardware(&platform_data->d3d_devices[inD3DDevice]))
	{
		// hardware device
		inDrawContextPrivate->isHardware = UUcTrue;
		d3d_device_desc = &platform_data->d3d_devices[inD3DDevice].HAL_desc;
		mem_location = DDSCAPS_VIDEOMEMORY;
	}
	else
	{
		// software device
		inDrawContextPrivate->isHardware = UUcFalse;
		d3d_device_desc = &platform_data->d3d_devices[inD3DDevice].HEL_desc;
		mem_location = DDSCAPS_SYSTEMMEMORY;
	}

	inDrawContextPrivate->zbpp =
		MDiFlagsToBitDepth(d3d_device_desc->dwDeviceZBufferBitDepth);

	// setup the direct draw surface description for the back buffer
	UUrMemory_Clear(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize					= sizeof(DDSURFACEDESC);
	ddsd.dwFlags				= DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps			= DDSCAPS_3DDEVICE | mem_location;
	ddsd.dwHeight				= inDrawContextPrivate->height;
	ddsd.dwWidth				= inDrawContextPrivate->width;

	// create the backBuffer
	result =
		IDirectDraw2_CreateSurface(
			inDrawContextPrivate->dd2,
			&ddsd,
			&inDrawContextPrivate->backBuffer,
			NULL);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	// create the zbuffer
	UUrMemory_Clear(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize				= sizeof(DDSURFACEDESC);
	ddsd.dwFlags			= DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT| DDSD_ZBUFFERBITDEPTH;
	ddsd.ddsCaps.dwCaps		= DDSCAPS_ZBUFFER | mem_location;
	ddsd.dwWidth			= inDrawContextPrivate->width;
	ddsd.dwHeight			= inDrawContextPrivate->height;
	ddsd.dwZBufferBitDepth	= inDrawContextPrivate->zbpp;
	
	result =
		IDirectDraw2_CreateSurface(
			inDrawContextPrivate->dd2,
			&ddsd,
			&inDrawContextPrivate->zBuffer,
			NULL);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	// attach the zBuffer to the rendering surface
	result =
		IDirectDrawSurface2_AddAttachedSurface(
			inDrawContextPrivate->backBuffer,
			inDrawContextPrivate->zBuffer);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
MDiInitD3D(
	MDtDrawContextPrivate		*inDrawContextPrivate,
	M3tDisplayDevice			*inDisplayDevice,
	UUtUns32					inD3DDevice)
{
	HRESULT						result;
	D3DVIEWPORT2				viewport_data;
	MDtPlatform					*platform_data;
	
	// get a pointer to the platform data
	platform_data = (MDtPlatform*)inDisplayDevice->platformDevice;
	
	// create the D3D Device
	result =
		IDirect3D2_CreateDevice(
			inDrawContextPrivate->d3d2,
			&platform_data->d3d_devices[inD3DDevice].d3d_guid,
			inDrawContextPrivate->backBuffer,
			&inDrawContextPrivate->d3d_device2);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	// Create the viewport
	result =
		IDirect3D2_CreateViewport(
			inDrawContextPrivate->d3d2,
			&inDrawContextPrivate->d3d_viewport2,
			NULL);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	// Attach viewport to D3D device
	result =
		IDirect3DDevice_AddViewport(
			inDrawContextPrivate->d3d_device2,
			inDrawContextPrivate->d3d_viewport2);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	// configure the viewport
	UUrMemory_Clear(&viewport_data, sizeof(D3DVIEWPORT2));
	
	viewport_data.dwSize		= sizeof(D3DVIEWPORT2);
	viewport_data.dwX			= inDrawContextPrivate->left;
	viewport_data.dwY			= inDrawContextPrivate->top;
	viewport_data.dwWidth		= inDrawContextPrivate->width;
	viewport_data.dwHeight		= inDrawContextPrivate->height;
	viewport_data.dvClipWidth	= (float)inDrawContextPrivate->width;
	viewport_data.dvClipHeight	= (float)inDrawContextPrivate->height;
	viewport_data.dvClipX		= 0.0;
	viewport_data.dvClipY		= 0.0;
	viewport_data.dvMinZ		= 0.0;
	viewport_data.dvMaxZ		= 1.0;
	
	result =
		IDirect3DViewport2_SetViewport2(
			inDrawContextPrivate->d3d_viewport2,
			&viewport_data);
	if (result != D3D_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	// make the device use the viewport
	result =
		IDirect3DDevice2_SetCurrentViewport(
			inDrawContextPrivate->d3d_device2,
			inDrawContextPrivate->d3d_viewport2);
	if (result != D3D_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}
	
	// load the texture formats for the device
	result =
		IDirect3DDevice2_EnumTextureFormats(
			inDrawContextPrivate->d3d_device2,
			MDiD3D_TextureFormatCallback,
			&platform_data->d3d_devices[inD3DDevice]);
	if (result != D3D_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		return UUcError_DirectDraw;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
MDiDrawEngine_SetupEngineCaps(
	M3tDrawEngineCaps	*ioDrawEngineCaps)
{
	HRESULT				result;

	ioDrawEngineCaps->numDisplayDevices = 0;
	
	// enumerate the direct draw devices
	result = DirectDrawEnumerate(MDiDD_DriverCallback, ioDrawEngineCaps);
	if (result != DD_OK)
	{
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static BOOL PASCAL
MDiDD_DriverCallback(
	LPGUID						inGuid,
	LPSTR						inDescription,
	LPSTR						inName,
	LPVOID						inData)
{
	M3tDrawEngineCaps			*draw_engine_caps;
	UUtUns32					index;
	LPDIRECTDRAW				dd;
	LPDIRECT3D2					d3d2;
	MDtPlatform					*platform_data;
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
	
	// allocate the platform_data
	platform_data = (MDtPlatform*)UUrMemory_Block_New(sizeof(MDtPlatform));
	if (platform_data == NULL)
	{
		return DDENUMRET_OK;
	}
	
	// clear the memory
	UUrMemory_Clear(platform_data, sizeof(MDtPlatform));
	
	// save guid
	if (inGuid)
	{
		platform_data->dd_guid = *inGuid;
		platform_data->primary = UUcFalse;
	}
	else
	{
		platform_data->primary = UUcTrue;
	}
	
	// initialize the display device
	draw_engine_caps->displayDevices[index].platformDevice =
		(M3tPlatformDevice)platform_data;
	draw_engine_caps->displayDevices[index].numDisplayModes = 0;
	
	// create the DirectDraw object
	result = DirectDrawCreate(inGuid, &dd, NULL);
	if (result != DD_OK)
	{
		UUrMemory_Block_Delete(platform_data);
		draw_engine_caps->displayDevices[index].platformDevice = NULL;
		
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		goto cleanup;
	}
	
	// try to get a Direct3D interface
	result = IDirectDraw_QueryInterface(dd, &IID_IDirect3D2, &d3d2);
	if (result != DD_OK)
	{
		UUrMemory_Block_Delete(platform_data);
		// No d3d interface is available
		goto cleanup;
	}

	// enumerate the display modes
	result =
		IDirectDraw_EnumDisplayModes(
			dd,
			0,
			NULL,
			&draw_engine_caps->displayDevices[index],
			MDiDD_ModeCallback);
	if (result != DD_OK)
	{
		UUrMemory_Block_Delete(platform_data);
		draw_engine_caps->displayDevices[index].platformDevice = NULL;
		
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		goto cleanup;
	}
	
	// enumerate the d3d devices
	result =
		IDirect3D2_EnumDevices(
			d3d2,
			MDiD3D_DeviceCallback,
			&draw_engine_caps->displayDevices[index]);
	if (result != D3D_OK)
	{
		UUrMemory_Block_Delete(platform_data);
		draw_engine_caps->displayDevices[index].platformDevice = NULL;
		
		UUrError_Report(UUcError_DirectDraw, M3rPlatform_GetErrorMsg(result));
		goto cleanup;
	}

	// increment the number of display devices
	draw_engine_caps->numDisplayDevices++;

cleanup:
	// cleanup before leaving
	if (d3d2)
	{
		IDirect3D2_Release(d3d2);
		d3d2 = NULL;
	}
	
	if (dd)
	{
		IDirectDraw_Release(dd);
		dd = NULL;
	}
	
	return DDENUMRET_OK;	
}

// ----------------------------------------------------------------------
static HRESULT PASCAL
MDiDD_ModeCallback(
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

// ----------------------------------------------------------------------
static HRESULT PASCAL
MDiD3D_DeviceCallback(
	LPGUID						inGuid,
	LPSTR						inName,
	LPSTR						inDescription,
	LPD3DDEVICEDESC				inHALDesc,
	LPD3DDEVICEDESC				inHELDesc,
	LPVOID						inData)
{
	M3tDisplayDevice			*display_device;
	MDtPlatform					*platform_data;
	UUtUns16					index;
	
	// if inData is NULL, something went terribly wrong
	if (inData == NULL)
	{
		return DDENUMRET_OK;
	}
	
	// get a pointer to the display device
	display_device = (M3tDisplayDevice*)inData;
	
	// get a pointer to the platform data
	platform_data = (MDtPlatform*)display_device->platformDevice;
	if (platform_data == NULL)
	{
		return DDENUMRET_OK;
	}
	
	// get an index into the d3d device array
	index = platform_data->num_d3d_devices;
	if (index >= MDcMaxD3DDevices)
	{
		return DDENUMRET_OK;
	}
	
	// record the info
	platform_data->d3d_devices[index].d3d_guid = *inGuid;
	UUrString_Copy(platform_data->d3d_devices[index].device_desc, inDescription, MDcMaxDescChars);
	platform_data->d3d_devices[index].HAL_desc = *inHALDesc;
	platform_data->d3d_devices[index].HEL_desc = *inHELDesc;
		
	// increment the number of d3d devices
	platform_data->num_d3d_devices++;
	
	return DDENUMRET_OK;
}

// ----------------------------------------------------------------------
static HRESULT PASCAL
MDiD3D_TextureFormatCallback(
	LPDDSURFACEDESC				inTextureFormat,
	LPVOID						inData)
{
	MDtD3D_device_info			*d3d_device_info;
	UUtUns16					index;
	
	// get a pointer to the d3d device info
	d3d_device_info = (MDtD3D_device_info*)inData;
	if (d3d_device_info == NULL)
	{
		return DDENUMRET_OK;
	}
	
	// get an index into the texture format array
	index = d3d_device_info->num_texture_formats;
	if (index >= MDcMaxTextureDesc)
	{
		return DDENUMRET_OK;
	}
	
	// record the info
	d3d_device_info->texture_format[index] = *inTextureFormat;
	
	// increment the number of texture formats
	d3d_device_info->num_texture_formats++;
	
	return DDENUMRET_OK;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtInt32
MDiFlagsToBitDepth(
	UUtUns32					inFlags)
{
	if (inFlags & DDBD_1)
		return 1;
	else if (inFlags & DDBD_2)
		return 2;
	else if (inFlags & DDBD_4)
		return 4;
	else if (inFlags & DDBD_8)
		return 8;
	else if (inFlags & DDBD_16)
		return 16;
	else if (inFlags & DDBD_24)
		return 24;
	else if (inFlags & DDBD_32)
		return 32;
	else
		return 0;
}

// ----------------------------------------------------------------------
UUtBool
MDiIsHardware(
	MDtD3D_device_info			*inD3DDeviceInfo)
{
	UUtBool isHardware = (inD3DDeviceInfo->HAL_desc.dcmColorModel) ? UUcTrue : UUcFalse;

	return isHardware;
}

// ----------------------------------------------------------------------
UUtUns32
MDiGetD3DDeviceIndex(
	void)
{
	// XXX - must find a better mechanism for this
	return 1;
}