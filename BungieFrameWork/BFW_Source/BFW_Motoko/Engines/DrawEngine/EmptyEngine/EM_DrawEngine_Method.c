/*
	FILE:	EM_DrawEngine_Method.c

	AUTHOR:	Kevin Armstrong, Michael Evans

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
#include "BFW_BitVector.h"
#include "BFW_Console.h"

#include "Motoko_Manager.h"

#include "EM_DC_Private.h"

#include "EM_DrawEngine_Method.h"
#include "EM_DrawEngine_Platform.h"

#include "EM_DC_Method_State.h"
#include "EM_DC_Method_Frame.h"
#include "EM_DC_Method_LinePoint.h"
#include "EM_DC_Method_Triangle.h"
#include "EM_DC_Method_Quad.h"
#include "EM_DC_Method_Pent.h"
#include "EM_DC_Method_Bitmap.h"
#include "EM_DC_Method_Query.h"

#define EMcSoftware_Version	(0x00010000)

M3tDrawContextMethods	EMgDrawContextMethods;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
static void
EDrDrawEngine_Method_ContextPrivateDelete(
	void)
{

}

// ----------------------------------------------------------------------
static UUtError
EDrDrawEngine_Method_ContextPrivateNew(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	M3tDrawContextMethods*		*outDrawContextFuncs,
	UUtBool						inFullScreen,
	M3tDrawAPI*					outAPI)
{
	*outAPI = M3cDrawAPI_Empty;

	EMgDrawContextMethods.frameStart = NULL;
	EMgDrawContextMethods.frameEnd = NULL;
	EMgDrawContextMethods.frameSync = NULL;
	EMgDrawContextMethods.triangle = NULL;
	EMgDrawContextMethods.quad = NULL;
	EMgDrawContextMethods.pent = NULL;
	EMgDrawContextMethods.line = NULL;
	EMgDrawContextMethods.point = NULL;
	EMgDrawContextMethods.sprite = NULL;
	EMgDrawContextMethods.screenCapture = NULL;
	EMgDrawContextMethods.pointVisible = NULL;
	EMgDrawContextMethods.textureFormatAvailable = NULL;

	return UUcError_None;
}


static UUtError
EDrDrawEngine_Method_Texture_Init(
	M3tTextureMap*				inTextureMap)
{
	//EDtTextureMapPrivate*	privateData	 = (EDtTextureMapPrivate*)M3rManager_Texture_GetEnginePrivate(inTextureMap);

	return UUcError_None;
}

static UUtError
EDrDrawEngine_Method_Texture_Update(
	M3tTextureMap*				inTextureMap)
{
	//EDtTextureMapPrivate *private_texture = (EDtTextureMapPrivate*) (M3rManager_Texture_GetEnginePrivate(inTextureMap));

	return UUcError_None;
}

static UUtError
EDrDrawEngine_Method_Texture_Load(
	M3tTextureMap*				inTextureMap)
{

	return UUcError_None;
}

static UUtError
EDrDrawEngine_Method_Texture_Unload(
	M3tTextureMap*				inTextureMap)
{
	return UUcError_None;
}

static UUtError
EDrDrawEngine_Method_Texture_Delete(
	M3tTextureMap*				inTextureMap)
{
	//EDtTextureMapPrivate *private_texture = (EDtTextureMapPrivate*) (M3rManager_Texture_GetEnginePrivate(inTextureMap));

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
EDrDrawEngine_Initialize(
	void)
{
	UUtError error;

	M3tDrawEngineCaps		drawEngineCaps;
	M3tDrawEngineMethods	drawEngineMethods;


	drawEngineMethods.contextPrivateNew = EDrDrawEngine_Method_ContextPrivateNew;
	drawEngineMethods.contextPrivateDelete = EDrDrawEngine_Method_ContextPrivateDelete;

	drawEngineCaps.engineFlags = M3cDrawEngineFlag_3DOnly;

	strncpy(drawEngineCaps.engineName, M3cDrawEngine_RAVE, M3cMaxNameLen);
	drawEngineCaps.engineDriver[0] = 0;

	drawEngineCaps.engineVersion = EMcSoftware_Version;

	// XXX - Someday make this more real
	drawEngineCaps.numDisplayDevices = 1;
	drawEngineCaps.displayDevices[0].numDisplayModes = 1;
	drawEngineCaps.displayDevices[0].displayModes[0].width = 0;
	drawEngineCaps.displayDevices[0].displayModes[0].height = 0;
	drawEngineCaps.displayDevices[0].displayModes[0].bitDepth = 0;

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

// ----------------------------------------------------------------------
void
EDrDrawEngine_Terminate(
	void)
{

}

// ----------------------------------------------------------------------
UUtError
EDrDrawContext_Method_Frame_Start(
	UUtUns32			inGameTime)
{

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
EDrDrawContext_Method_Frame_End(
	void)
{

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
EDrDrawContext_Method_Frame_Sync(
	void)
{
	return UUcError_None;
}

