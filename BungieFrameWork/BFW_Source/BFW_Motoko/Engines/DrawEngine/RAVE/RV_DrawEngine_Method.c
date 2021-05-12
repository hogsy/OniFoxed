/*
	FILE:	RV_DrawEngine_Method.c
	
	AUTHOR:	Kevin Armstrong, Michael Evans
	
	CREATED: January 5, 1998
	
	PURPOSE: 
	
	Copyright 1997 - 1998

*/

// ======================================================================
// includes
// ======================================================================
#include <RAVE.h>
#include "ATIRave.h"

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"
#include "BFW_BitVector.h"
#include "BFW_Console.h"

#include "Motoko_Manager.h"

#include "RV_DC_Private.h"

#include "RV_DrawEngine_Method.h"
#include "RV_DrawEngine_Platform.h"

#include "RV_DC_Method_State.h"
#include "RV_DC_Method_Frame.h"
#include "RV_DC_Method_LinePoint.h"
#include "RV_DC_Method_Triangle.h"
#include "RV_DC_Method_Quad.h"
#include "RV_DC_Method_Pent.h"
#include "RV_DC_Method_Bitmap.h"
#include "RV_DC_Method_Query.h"

#define RVcVersion	(0x00010000)

#define RVcMaxDisplayDevices	(8)
#define RVcMaxEngines			(8)

typedef struct RVtDisplayDevice
{
	TQADevice	raveDevice;
	
} RVtDisplayDevice;

typedef struct RVtEngine
{
	TQAEngine*			raveEngine;
	
	UUtUns16			numDisplayDevices;
	RVtDisplayDevice	displayDevices[RVcMaxDisplayDevices];
	
} RVtEngine;

UUtUns16			RVgEngine_Num = 0;
RVtEngine			RVgEngine_List[RVcMaxDisplayDevices];
UUtUns16			RVgEngine_ActiveIndex;

RVtDrawContextPrivate	RVgDrawContextPrivate;

// ----------------------------------------------------------------------
extern UUtUns16				M3gActiveDrawEngine;
extern UUtUns16				M3gActiveDisplayDevice;
extern UUtUns16				M3gActiveDisplayMode;

M3tDrawContextMethods	RVgDrawContextMethods;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------

static void
RViDrawEngine_AddDisplayDevice(
	TQADevice*	inRaveDevice,
	TQAEngine*	inRaveEngine)
{
	UUtUns16			engineItr;
	RVtEngine*			targetEngine = NULL;
	RVtDisplayDevice*	targetDisplayDevice;
	
	for(engineItr = 0; engineItr < RVgEngine_Num; engineItr++)
	{
		if(RVgEngine_List[engineItr].raveEngine == inRaveEngine)
		{
			targetEngine = RVgEngine_List + engineItr;
			break;
		}
	}
	
	if(targetEngine == NULL)
	{
		targetEngine = RVgEngine_List + RVgEngine_Num++;
		targetEngine->numDisplayDevices = 0;
		targetEngine->raveEngine = inRaveEngine;
	}
	
	targetDisplayDevice = targetEngine->displayDevices + targetEngine->numDisplayDevices++;
	
	targetDisplayDevice->raveDevice = *inRaveDevice;
}

static void
RViDrawEngine_DisplayDeviceList_Build(
	void)
{
	GDHandle	curDevice;
	TQADevice	raveDevice;
	TQAEngine*	curRaveEngine;
	
	curDevice = GetDeviceList();
	
	while(curDevice != NULL)
	{
		raveDevice.deviceType = kQADeviceGDevice;
		raveDevice.device.gDevice = curDevice;
		
		// loop through all the engines that support this display device
		curRaveEngine = QADeviceGetFirstEngine(&raveDevice);
		
		while(curRaveEngine != NULL)
		{
			RViDrawEngine_AddDisplayDevice(
				&raveDevice,
				curRaveEngine);
			
			curRaveEngine = QADeviceGetNextEngine(&raveDevice, curRaveEngine);
		}
		
		curDevice = GetNextDevice(curDevice);
	}
}

// ----------------------------------------------------------------------
static void
RVrDrawEngine_Method_ContextPrivateDelete(
	void)
{
	QADrawContextDelete(RVgDrawContextPrivate.activeRaveContext);
	RVgDrawContextPrivate.activeRaveContext = NULL;
	RVgDrawContextPrivate.atiExtFuncs = NULL;
}

// ----------------------------------------------------------------------
static UUtError
RVrDrawEngine_Method_ContextPrivateNew(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	M3tDrawContextMethods*		*outDrawContextFuncs,
	UUtBool						inFullScreen,
	M3tDrawAPI*					outAPI)
{
	TQAError			raveError;
	TQAEngine*			targetRaveEngine;
	M3tDrawEngineCaps*	targetEngineCaps;
	UUtUns32			targetEngineIndex;
	TQARect				raveRect;
	UUtUns16			itr;
	
	UUmAssert(RVgDrawContextPrivate.activeRaveContext == NULL);
	UUmAssert(inDrawContextDescriptor->type == M3cDrawContextType_OnScreen);
	
	*outAPI = M3cDrawAPI_RAVE;

	// find the right engine
		targetEngineCaps = M3rDrawEngine_GetCaps(M3gActiveDrawEngine);
		UUmAssertReadPtr(targetEngineCaps, sizeof(M3tDrawEngineCaps));
		
		targetEngineIndex = (UUtUns32)targetEngineCaps->enginePrivate;
		targetRaveEngine = RVgEngine_List[targetEngineIndex].raveEngine;
		
		RVgEngine_ActiveIndex = targetEngineIndex;
		RVgDrawContextPrivate.activeRaveEngine = targetRaveEngine;
	
	// Setup the context parameters
		raveRect.top = inDrawContextDescriptor->drawContext.onScreen.rect.top;
		raveRect.left = inDrawContextDescriptor->drawContext.onScreen.rect.left;
		raveRect.bottom = inDrawContextDescriptor->drawContext.onScreen.rect.bottom;
		raveRect.right = inDrawContextDescriptor->drawContext.onScreen.rect.right;
	
	// setup the width and height
		RVgDrawContextPrivate.width = raveRect.right - raveRect.left;
		RVgDrawContextPrivate.height = raveRect.bottom - raveRect.top;
	
	// create the context
		raveError = 
			QADrawContextNew(
				&RVgEngine_List[targetEngineIndex].displayDevices[M3gActiveDisplayDevice].raveDevice,
				&raveRect,
				NULL,		// never clip
				targetRaveEngine,
				kQAContext_DoubleBuffer,
				&RVgActiveRaveContext);
		if(raveError != kQANoErr)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not create RAVE context");
		}
	
	// init the function parameters
		RVgDrawContextMethods.frameStart = RVrDrawContext_Method_Frame_Start;
		RVgDrawContextMethods.frameEnd = RVrDrawContext_Method_Frame_End;
		RVgDrawContextMethods.frameSync = RVrDrawContext_Method_Frame_Sync;
		RVgDrawContextMethods.triangle = NULL;
		RVgDrawContextMethods.quad = NULL;
		RVgDrawContextMethods.pent = NULL;
		RVgDrawContextMethods.line = NULL;
		RVgDrawContextMethods.point = RVrDrawContext_Method_Point;
		RVgDrawContextMethods.sprite = RVrDrawContext_Method_Sprite;
		RVgDrawContextMethods.screenCapture = RVrDrawContext_Method_ScreenCapture;
		RVgDrawContextMethods.textureFormatAvailable = RVrDrawContext_TextureFormatAvailable;
	
	// initialize the state
		RVgDrawContextPrivate.stateTOS = 0;
		RVgDrawContextPrivate.statePtr = RVgDrawContextPrivate.statePtrStack[0];
		RVgDrawContextPrivate.stateInt = RVgDrawContextPrivate.stateIntStack[0];
	
		for(itr = 0; itr < M3cDrawStateIntType_NumTypes; itr++)
		{
			RVgDrawContextPrivate.stateInt[itr] = 0;
		}
		
		for(itr = 0; itr < M3cDrawStatePtrType_NumTypes; itr++)
		{
			RVgDrawContextPrivate.statePtr[itr] = NULL;
		}
		
		RVgDrawContextPrivate.stateInt[M3cDrawStateIntType_Appearence] = M3cDrawState_Appearence_Gouraud;
		RVgDrawContextPrivate.stateInt[M3cDrawStateIntType_Interpolation] = M3cDrawState_Interpolation_None;
		RVgDrawContextPrivate.stateInt[M3cDrawStateIntType_Fill] = M3cDrawState_Fill_Solid;

		//RVgDrawContextPrivate->vertexList = MGgDrawContextPrivate->vertexListStack[0];
		//RVgDrawContextPrivate->vertexBitVector = MGgDrawContextPrivate->vertexBVStack[0];
		RVgDrawContextPrivate.stateFlags = 0xFF;
		
	
	return UUcError_None;
}

static UUtError
RVrDrawEngine_Method_Texture_Init(
	M3tTextureMap*				inTextureMap)
{
	//RVtTextureMapPrivate*	privateData	 = (RVtTextureMapPrivate*)M3rManager_Texture_GetEnginePrivate(inTextureMap);
	
	//privateData->convertedData = NULL;
	//privateData->raveTexture = NULL;
		
	return UUcError_None;
}

static UUtError
RVrDrawEngine_Method_Texture_Load(
	M3tTextureMap*				inTextureMap)
{
	//TQAError				raveError;
	//RVtTextureMapPrivate*	privateData = (RVtTextureMapPrivate*) (M3rManager_Texture_GetEnginePrivate(inTextureMap));
	
	//UUmAssert(privateData->raveTexture == NULL);
	//UUmAssert(RVgDrawContextPrivate.activeRaveEngine != NULL);
	
	#if 0
	raveError = 
		QATextureNew(
			RVgDrawContextPrivate.activeRaveEngine,
			privateData->raveFlags,
			privateData->pixelType,
			privateData->image,
			&privateData->raveTexture);
	if(raveError != kQANoErr)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not create rave texture");
	}
	#endif
	
	return UUcError_None;
}

static UUtError
RVrDrawEngine_Method_Texture_Unload(
	M3tTextureMap*				inTextureMap)
{
	//RVtTextureMapPrivate*	privateData = (RVtTextureMapPrivate*) (M3rManager_Texture_GetEnginePrivate(inTextureMap));

#if 0
	UUmAssert(privateData->raveTexture != NULL);

	QATextureDelete(RVgDrawContextPrivate.activeRaveEngine, privateData->raveTexture);
	
	privateData->raveTexture = NULL;
#endif

	return UUcError_None;
}

static UUtError
RVrDrawEngine_Method_Texture_Update(
	M3tTextureMap*				inTextureMap)
{
#if 0
	UUtError				error;
	RVtTextureMapPrivate*	privateData = (RVtTextureMapPrivate*) (M3rManager_Texture_GetEnginePrivate(inTextureMap));
	IMtPixel				emptyPixel = {0};
	
	// XXX - need to handle I8 case
	
	switch(inTextureMap->texelType)
	{
		case IMcPixelType_ARGB4444:
			privateData->pixelType = (TQAImagePixelType)kQATIPixel_ARGB4444;
			privateData->image[0].rowBytes = sizeof(UUtUns16) * inTextureMap->width;
			privateData->image[0].pixmap = inTextureMap->data;
			break;
			
		case IMcPixelType_RGB555:
			privateData->pixelType = kQAPixel_RGB16;
			privateData->image[0].rowBytes = sizeof(UUtUns16) * inTextureMap->width;
			privateData->image[0].pixmap = inTextureMap->data;
			break;
			
		case IMcPixelType_ARGB1555:
			privateData->pixelType = kQAPixel_ARGB16;
			privateData->image[0].rowBytes = sizeof(UUtUns16) * inTextureMap->width;
			privateData->image[0].pixmap = inTextureMap->data;
			break;
			
		case IMcPixelType_I8:
			// need to convert to RGB16 - perhaps this is supported through ATI RAVE
			// XXX - Assert this is not mipmapped
			
			privateData->convertedData =
				UUrMemory_Block_New(inTextureMap->width * inTextureMap->height * sizeof(UUtUns16));
			UUmError_ReturnOnNull(privateData->convertedData);
			
			error = 
				IMrImage_ConvertPixelType(
					IMcDitherMode_On,
					inTextureMap->width,
					inTextureMap->height,
					IMcNoMipMap,
					IMcPixelType_I8,
					inTextureMap->data,
					IMcPixelType_RGB555,
					privateData->convertedData);
			UUmError_ReturnOnError(error);
			
			privateData->pixelType = kQAPixel_RGB16;
			privateData->image[0].rowBytes = sizeof(UUtUns16) * inTextureMap->width;
			privateData->image[0].pixmap = privateData->convertedData;
			break;
			
		case IMcPixelType_I1:
			UUmAssert(0);
			break;
			
		case IMcPixelType_A8:
			UUmAssert(0);
			break;
			
		case IMcPixelType_A4I4:
			UUmAssert(0);
			break;
			
		case IMcPixelType_ARGB8888:
			privateData->pixelType = kQAPixel_ARGB32;
			privateData->image[0].rowBytes = sizeof(UUtUns32) * inTextureMap->width;
			privateData->image[0].pixmap = inTextureMap->data;
			break;
			
		case IMcPixelType_RGB888:
			privateData->pixelType = kQAPixel_RGB32;
			privateData->image[0].rowBytes = sizeof(UUtUns32) * inTextureMap->width;
			privateData->image[0].pixmap = inTextureMap->data;
			break;
		
		default:
			UUmAssert(!"Illegal pixel type");
	}
	
	// create TQAImage
	privateData->image[0].width = inTextureMap->width;
	privateData->image[0].height = inTextureMap->width;
	
	// compute the rave texture flags
	privateData->raveFlags = kQATexture_NoCompression;
	
	if(inTextureMap->flags & M3cTextureFlags_HasMipMap)
	{
		// privateData->raveFlags |= kQATexture_Mipmap;
		// eventually build mipmaps
	}
	
	if(privateData->raveTexture != NULL)
	{
		if(RVgDrawContextPrivate.atiExtFuncs != NULL)
		{
			// eventually use the ati extended functions
			// XXX - what if pixel type changes???
		}
		else
		{
			RVrDrawEngine_Method_Texture_Unload(inTextureMap);
			RVrDrawEngine_Method_Texture_Load(inTextureMap);
		}
	}
#endif	
	return UUcError_None;
}

static UUtError
RVrDrawEngine_Method_Texture_Delete(
	M3tTextureMap*				inTextureMap)
{
#if 0
	RVtTextureMapPrivate *privateData = (RVtTextureMapPrivate*) (M3rManager_Texture_GetEnginePrivate(inTextureMap));
	
	if(privateData->convertedData != NULL)
	{
		UUrMemory_Block_Delete(privateData->convertedData);
		privateData->convertedData = NULL;
	}
	
	if(privateData->raveTexture != NULL)
	{
		QATextureDelete(RVgDrawContextPrivate.activeRaveEngine, privateData->raveTexture);
	}
#endif

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
RVrDrawEngine_Initialize(
	void)
{
	UUtError error;
	
	M3tDrawEngineCaps		drawEngineCaps;
	M3tDrawEngineMethods	drawEngineMethods;
	
	UUtUns16				engineItr;
	char					buffer[1024];
	UUtUns32				nameLength;
	TQAError				raveError;
	
	RViDrawEngine_DisplayDeviceList_Build();
	
	if(RVgEngine_Num == 0) return;
	
	drawEngineMethods.contextPrivateNew = RVrDrawEngine_Method_ContextPrivateNew;
	drawEngineMethods.contextPrivateDelete = RVrDrawEngine_Method_ContextPrivateDelete;
	
	for(engineItr = 0; engineItr < RVgEngine_Num; engineItr++)
	{
		drawEngineCaps.engineFlags = M3cDrawEngineFlag_None;
		drawEngineCaps.enginePrivate = (void*)engineItr;
		
		UUrString_Copy(drawEngineCaps.engineName, M3cDrawEngine_RAVE, M3cMaxNameLen);
		
		raveError = 
			QAEngineGestalt(
				RVgEngine_List[engineItr].raveEngine,
				kQAGestalt_ASCIINameLength,
				&nameLength);
		if(raveError != kQANoErr)
		{
			UUrError_Report(UUcError_Generic, "could not get name length");
			return;
		}
		
		if(nameLength >= 1024)
		{
			UUrError_Report(UUcError_Generic, "name length too long");
			return;
		}
		
		raveError = 
			QAEngineGestalt(
				RVgEngine_List[engineItr].raveEngine,
				kQAGestalt_ASCIIName,
				buffer);
		if(raveError != kQANoErr)
		{
			UUrError_Report(UUcError_Generic, "Could get name length");
			return;
		}
		
		UUrString_Copy(drawEngineCaps.engineDriver, buffer, M3cMaxNameLen);
		
		drawEngineCaps.engineVersion = RVcVersion;
		
		// XXX - Someday make this more real
		drawEngineCaps.numDisplayDevices = 1;
		drawEngineCaps.displayDevices[0].numDisplayModes = 1;
		drawEngineCaps.displayDevices[0].displayModes[0].width = 640;
		drawEngineCaps.displayDevices[0].displayModes[0].height = 480;
		drawEngineCaps.displayDevices[0].displayModes[0].bitDepth = 16;
		
		error =
			M3rManager_Register_DrawEngine(
				&drawEngineCaps,
				&drawEngineMethods);
		if(error != UUcError_None)
		{
			UUrError_Report(UUcError_Generic, "Could not setup engine caps");
			return;
		}
	}

	// initialize a few fields
		RVgDrawContextPrivate.activeRaveEngine = NULL;
		RVgDrawContextPrivate.activeRaveContext = NULL;
		RVgDrawContextPrivate.atiExtFuncs = NULL;
}

// ----------------------------------------------------------------------
void
RVrDrawEngine_Terminate(
	void)
{

}

// ----------------------------------------------------------------------
UUtError
RVrDrawContext_Method_Frame_Start(
	UUtUns32			inGameTime)
{
	QARenderStart(RVgActiveRaveContext, NULL, NULL);

	return UUcError_None;
}
	
// ----------------------------------------------------------------------
UUtError
RVrDrawContext_Method_Frame_End(
	UUtUns32	*outTextureBytesDownloaded)
{
	QARenderEnd(RVgActiveRaveContext, NULL);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
RVrDrawContext_Method_Frame_Sync(
	void)
{
	return UUcError_None;
}

