/*
	FILE:	MS_DrawEngine_Method.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 13, 1997

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"

#include "Motoko_Manager.h"

#include "MS_DC_Private.h"

#include "MS_DrawEngine_Method.h"
#include "MS_DrawEngine_Platform.h"

#include "MS_DC_Method_State.h"
#include "MS_DC_Method_Frame.h"
#include "MS_DC_Method_LinePoint.h"
#include "MS_DC_Method_Triangle.h"
#include "MS_DC_Method_Quad.h"
#include "MS_DC_Method_Query.h"
#include "MS_DC_Method_SmallQuad.h"
#include "MS_DC_Method_Pent.h"
#include "MS_DC_Method_Bitmap.h"

#define MScSoftware_Version	(0x00010000)

static UUtError
MSrDrawEngine_Texture_TemplateHandler(
	TMtTemplateProc_Message	inMessage,
	TMtTemplateTag			inTheCausingTemplate,	// The template tag of the changing instance
	void*					inDataPtr)
{
	M3tTextureMap*			textureMap;
	MStTextureMapPrivate*	privateData;
	UUtUns16				widthNBits, heightNBits;

	textureMap = (M3tTextureMap*)(inDataPtr);
	privateData = (MStTextureMapPrivate*)(TMmInstance_GetDynamicData(inDataPtr));

	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_DisposePreProcess:
			break;

		case TMcTemplateProcMessage_LoadPostProcess:
		case TMcTemplateProcMessage_Update:

			if(textureMap->numLongs > 1)
			{
				widthNBits = heightNBits = 15;

				while(!(textureMap->width & (1 << widthNBits))) widthNBits--;
				while(!(textureMap->height & (1 << heightNBits))) heightNBits--;

				UUmAssert(textureMap->width == 1 << widthNBits);
				UUmAssert(textureMap->height == 1 << heightNBits);

				privateData->nWidthBits = widthNBits;
				privateData->nHeightBits = heightNBits;
			}
			break;

		default:
			UUmAssert(!"Unkown message");
	}

	return UUcError_None;

}

static void
MSrDrawEngine_Method_ContextPrivateDelete(
	M3tDrawContextPrivate*		inDrawContextPrivate)
{
	MStDrawContextPrivate*		contextPrivate = (MStDrawContextPrivate *)inDrawContextPrivate;

	MSrDrawEngine_Platform_DestroyDrawContextPrivate(contextPrivate);

	UUrMemory_Block_Delete(inDrawContextPrivate);
}

static UUtError
MSrDrawEngine_Method_ContextPrivateNew(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	M3tDrawContextPrivate*		*outDrawContextPrivate,
	UUtBool						inFullScreen,
	M3tDrawAPI*					outAPI)
{
	UUtError				errorCode;
	MStDrawContextPrivate*	newDrawContextPrivate;
	UUtUns16				i;

	*outDrawContextPrivate = NULL;
	*outAPI = M3cDrawAPI_Software;

	newDrawContextPrivate = UUrMemory_Block_New(sizeof(MStDrawContextPrivate));

	if(newDrawContextPrivate == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not allocated private draw context");
	}


	newDrawContextPrivate->contextType = inDrawContextDescriptor->type;

	if(inDrawContextDescriptor->type == M3cDrawContextType_OnScreen)
	{
		newDrawContextPrivate->screenFlags = inDrawContextDescriptor->drawContext.onScreen.flags;
	}
	else
	{
		newDrawContextPrivate->screenFlags = M3cDrawContextScreenFlags_None;
	}

	newDrawContextPrivate->zBufferBaseAddr = NULL;
	newDrawContextPrivate->imageBufferBaseAddr = NULL;

	/*
	 * Setup the platform specific data
	 */
		errorCode =
			MSrDrawEngine_Platform_SetupDrawContextPrivate(
				inDrawContextDescriptor,
				newDrawContextPrivate,
				inFullScreen);
		UUmError_ReturnOnError(errorCode);

	for(i = 0; i < M3cDrawStateIntType_NumTypes; i++)
	{
		newDrawContextPrivate->stateInt[i] = 0;
	}

	for(i = 0; i < M3cDrawStatePtrType_NumTypes; i++)
	{
		newDrawContextPrivate->statePtr[i] = NULL;
	}

	*outDrawContextPrivate = (M3tDrawContextPrivate*)newDrawContextPrivate;

	return UUcError_None;
}

// DO NOT FUCKING TURN THIS INTO SOME FUCKING TYPED CASTED WHILE LOOP SHIT
static UUtError
MSrDrawEngine_Method_ContextMetaHandler(
	M3tDrawContext*				inDrawContext,
	M3tDrawContextMethodType	inMethodType,
	M3tDrawContextMethod		*outMethod)  // Would not want to "hoop" anyone
{
	MStDrawContextPrivate	*drawContextPrivate = (MStDrawContextPrivate *)inDrawContext->privateContext;

	switch(inMethodType)
	{
		case M3cDrawContextMethodType_Frame_Start:
			outMethod->frameStart = MSrDrawContext_Method_Frame_Start;
			break;

		case M3cDrawContextMethodType_Frame_End:
			outMethod->frameEnd = MSrDrawContext_Method_Frame_End;
			break;

		case M3cDrawContextMethodType_Frame_Sync:
			outMethod->frameSync = MSrDrawContext_Method_Frame_Sync;
			break;

		case M3cDrawContextMethodType_State_SetInt:
			outMethod->stateSetInt = MSrDrawContext_Method_State_SetInt;
			break;

		case M3cDrawContextMethodType_State_GetInt:
			outMethod->stateGetInt = MSrDrawContext_Method_State_GetInt;
			break;

		case M3cDrawContextMethodType_State_SetPtr:
			outMethod->stateSetPtr = MSrDrawContext_Method_State_SetPtr;
			break;

		case M3cDrawContextMethodType_State_GetPtr:
			outMethod->stateGetPtr = MSrDrawContext_Method_State_GetPtr;
			break;

		case M3cDrawContextMethodType_TriInterpolate:
			switch(drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					outMethod->triInterpolate = MSrDrawContext_Method_TriGouraudInterpolate;
					break;

				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					outMethod->triInterpolate = MSrDrawContext_Method_TriTextureInterpolate;
					break;

				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;

		case M3cDrawContextMethodType_TriFlat:
			switch(drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					outMethod->triFlat = MSrDrawContext_Method_TriGouraudFlat;
					break;

				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					outMethod->triFlat = MSrDrawContext_Method_TriTextureFlat;
					break;

				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;

		case M3cDrawContextMethodType_TriSplit:
			outMethod->triSplit = MSrDrawContext_Method_TriTextureSplit;
			break;

		case M3cDrawContextMethodType_QuadInterpolate:
			switch(drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					outMethod->quadInterpolate = MSrDrawContext_Method_QuadGouraudInterpolate;
					break;

				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					outMethod->quadInterpolate = MSrDrawContext_Method_QuadTextureInterpolate;
					break;

				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;

		case M3cDrawContextMethodType_QuadFlat:
			switch(drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					outMethod->quadFlat = MSrDrawContext_Method_QuadGouraudFlat;
					break;

				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					outMethod->quadFlat = MSrDrawContext_Method_QuadTextureFlat;
					break;

				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;

		case M3cDrawContextMethodType_QuadSplit:
			outMethod->quadSplit = MSrDrawContext_Method_QuadTextureSplit;
			break;

		case M3cDrawContextMethodType_SmallQuadInterpolate:
			switch(drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					outMethod->smallQuadInterpolate = MSrDrawContext_Method_SmallQuadGouraudInterpolate;
					break;

				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					outMethod->smallQuadInterpolate = MSrDrawContext_Method_SmallQuadTextureInterpolate;
					break;

				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;

		case M3cDrawContextMethodType_SmallQuadFlat:
			switch(drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					outMethod->smallQuadFlat = MSrDrawContext_Method_SmallQuadGouraudFlat;
					break;

				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					outMethod->smallQuadFlat = MSrDrawContext_Method_SmallQuadTextureFlat;
					break;

				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;

		case M3cDrawContextMethodType_PentInterpolate:
			switch(drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					outMethod->pentInterpolate = MSrDrawContext_Method_PentGouraudInterpolate;
					break;

				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					outMethod->pentInterpolate = MSrDrawContext_Method_PentTextureInterpolate;
					break;

				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;

		case M3cDrawContextMethodType_PentFlat:
			switch(drawContextPrivate->stateInt[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					outMethod->pentFlat = MSrDrawContext_Method_PentGouraudFlat;
					break;

				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Unlit:
					outMethod->pentFlat = MSrDrawContext_Method_PentTextureFlat;
					break;

				default:
					UUmAssert(!"Unkown appearence type");
			}
			break;

		case M3cDrawContextMethodType_PentSplit:
			outMethod->pentSplit = MSrDrawContext_Method_PentTextureSplit;
			break;

		case M3cDrawContextMethodType_Line_Interpolate:
			outMethod->lineInterpolate = MSrDrawContext_Method_Line_Interpolate;
			break;

		case M3cDrawContextMethodType_Line_Flat:
			outMethod->lineFlat = MSrDrawContext_Method_Line_Flat;
			break;

		case M3cDrawContextMethodType_Point:
			outMethod->point = MSrDrawContext_Method_Point;
			break;

		case M3cDrawContextMethodType_Bitmap:
			outMethod->bitmap = MSrDrawContext_Method_Bitmap;
			break;

		case M3cDrawContextMethodType_TextureFormatAvailable:
			outMethod->textureFormatAvailable = MSrDrawContext_TextureFormatAvailable;
			break;

		case M3cDrawContextMethodType_GetWidth:
			outMethod->getWidth = MSrDrawContext_GetWidth;
			break;

		case M3cDrawContextMethodType_GetHeight:
			outMethod->getHeight = MSrDrawContext_GetHeight;
			break;

		default:
			UUmAssert(!"There is some smelly shit going on here...");
	}

	return UUcError_None;
}

static UUtError
MSrDrawEngine_Method_Texture_Init(
	M3tTextureMap*				inTextureMap)
{
	MStTextureMapPrivate*	privateData	 = (MStTextureMapPrivate*)M3rManager_Texture_GetEnginePrivate(inTextureMap);
	UUtUns16				widthNBits, heightNBits;

	UUmAssert(privateData != NULL);

	if(inTextureMap->numLongs > 1 && inTextureMap->width > 0 && inTextureMap->height > 0)
	{
		widthNBits = heightNBits = 15;

		while(!(inTextureMap->width & (1 << widthNBits))) widthNBits--;
		while(!(inTextureMap->height & (1 << heightNBits))) heightNBits--;

		UUmAssert(inTextureMap->width == 1 << widthNBits);
		UUmAssert(inTextureMap->height == 1 << heightNBits);

		privateData->nWidthBits = widthNBits;
		privateData->nHeightBits = heightNBits;
	}

	return UUcError_None;
}

static UUtError
MSrDrawEngine_Method_Texture_Load(
	M3tTextureMap*				inTextureMap)
{

	return UUcError_None;
}

static UUtError
MSrDrawEngine_Method_Texture_Unload(
	M3tTextureMap*				inTextureMap)
{

	return UUcError_None;
}

static UUtError
MSrDrawEngine_Method_Texture_Delete(
	M3tTextureMap*				inTextureMap)
{

	return UUcError_None;
}

void
MSrDrawEngine_Initialize(
	void)
{
	UUtError error;

	M3tDrawEngineCaps		drawEngineCaps;
	M3tDrawEngineMethods	drawEngineMethods;

	drawEngineMethods.contextPrivateNew = MSrDrawEngine_Method_ContextPrivateNew;
	drawEngineMethods.contextPrivateDelete = MSrDrawEngine_Method_ContextPrivateDelete;
	drawEngineMethods.contextMetaHandler = MSrDrawEngine_Method_ContextMetaHandler;

	drawEngineMethods.textureInit = MSrDrawEngine_Method_Texture_Init;
	drawEngineMethods.textureLoad = MSrDrawEngine_Method_Texture_Load;
	drawEngineMethods.textureUnload = MSrDrawEngine_Method_Texture_Unload;
	drawEngineMethods.textureDelete = MSrDrawEngine_Method_Texture_Delete;
	drawEngineMethods.textureUpdate = MSrDrawEngine_Method_Texture_Init;

	drawEngineCaps.engineFlags = M3cDrawEngineFlag_CanHandleOffScreen;

	strncpy(drawEngineCaps.engineName, M3cDrawEngine_Software, M3cMaxNameLen);
	drawEngineCaps.engineDriver[0] = 0;

	drawEngineCaps.engineVersion = MScSoftware_Version;

	error = MSrDrawEngine_Platform_SetupEngineCaps(&drawEngineCaps);
	if(error != UUcError_None)
	{
		UUrError_Report(UUcError_Generic, "Could not setup engine caps");
		return;
	}

	error =
		M3rManager_Register_DrawEngine(
			sizeof(MStTextureMapPrivate),
			&drawEngineCaps,
			&drawEngineMethods);
	if(error != UUcError_None)
	{
		UUrError_Report(UUcError_Generic, "Could not register engine");
		return;
	}
}

void
MSrDrawEngine_Terminate(
	void)
{

}
