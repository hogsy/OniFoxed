/*
	FILE:	Motoko_Manager.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: May 5, 1997
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997-1999

*/

#include "bfw_math.h"
//#include <math.h>

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_TemplateManager.h"
#include "BFW_Console.h"
#include "BFW_MathLib.h"
#include "BFW_BitVector.h"
#include "BFW_Platform.h"

#include "Motoko_Manager.h"
#include "Motoko_Private.h"
#include "Motoko_Verify.h"
#include "Motoko_Sort.h"

#include "MS_GeomEngine_Method.h"
#include "MS_DrawEngine_Method.h"
#include "MG_DrawEngine_Method.h"

#ifndef __ONADA__
#include "GL_DrawEngine_Method.h"
#include "OG_GeomEngine_Method.h"
#else
extern UUtBool gl_draw_engine_initialize(void); // from gl_engine.c
#endif

#include "Oni.h"

#if UUmPlatform == UUmPlatform_Win32
//	#include "MD_DrawEngine_Method.h"
#else
#include "RV_DrawEngine_Method.h"
#endif

#if UUmPlatform == UUmPlatform_Mac
#include "RV_DrawEngine_Method.h"
#endif

M3tManagerDrawContext	M3gManagerDrawContext= {0};
M3tManagerGeomContext	M3gManagerGeomContext= {0};

UUtUns32*	M3gMyIckyBV = NULL;

UUtUns16				M3gNumDrawEngines = 0;
M3tManagerDrawEngine	M3gDrawEngineList[M3cMaxNumEngines];

UUtUns16				M3gNumGeomEngines = 0;
M3tManagerGeomEngine	M3gGeomEngineList[M3cMaxNumEngines];

UUtUns16				M3gActiveDrawEngine = 0xFFFF;
UUtUns16				M3gActiveGeomEngine = 0xFFFF;

UUtUns16				M3gActiveDisplayDevice = 0xFFFF;
UUtUns16				M3gActiveDisplayMode = 0xFFFF;

//UUtUns32				M3gMaxTexturePrivateSize = 0;

UUtBool					M3gFullScreen;

//M3tManagerTextureData*	M3gTexture_Loaded_Head = NULL;
//M3tManagerTextureData*	M3gTexture_Loaded_Tail = NULL;

M3tGeomContextMethods*	M3gGeomContext = NULL;

UUtUns32				M3gNumPoints = 0;
UUtUns32				M3gNumObjs = 0;

UUtBool					M3gSIMDPresent;

TMtPrivateData*			M3gTemplate_Geometry_PrivateData = NULL;
TMtPrivateData*			M3gTemplate_TextureMapBig_PrivateData = NULL;

UUtBool					M3gResolutionSwitch = UUcTrue;

static UUtError
M3iGeometryProcHandler(
	TMtTemplateProc_Message	inMessage,
	void*					inInstancePtr,
	void*					inPrivateData)
{
	UUtError				error = UUcError_None;
	M3tGeometry*			geometry = (M3tGeometry *) inInstancePtr;

	switch(inMessage)
	{
		case TMcTemplateProcMessage_LoadPostProcess:		
			M3gNumPoints += geometry->pointArray->numPoints;
			M3gNumObjs++;

		case TMcTemplateProcMessage_Update:
			#if defined(DEBUGGING) && DEBUGGING
			
				error = M3rVerify_Geometry(geometry);
			
			#endif
			
			geometry->geometryFlags |= M3cGeometryFlag_RemoveBackface;
			
			if(geometry->baseMap != NULL)
			{
				switch(geometry->baseMap->texelType)
				{
					case IMcPixelType_ARGB4444:
					case IMcPixelType_ARGB1555:
					case IMcPixelType_A8:
					case IMcPixelType_A4I4:
					case IMcPixelType_ARGB8888:
						geometry->geometryFlags &= ~M3cGeometryFlag_RemoveBackface;
						break;
				}
			}
			
		break;

		case TMcTemplateProcMessage_NewPostProcess:
			break;
		
		case TMcTemplateProcMessage_PrepareForUse:
			break;
	}

	return error;
}


// ----------------------------------------------------------------------
static UUtError
M3iBigTextureInit(
	M3tTextureMap_Big		*inTextureMap)
{
	// calculate the number of sub_textures needed
	inTextureMap->num_x = inTextureMap->width / M3cTextureMap_MaxWidth;
	inTextureMap->num_y = inTextureMap->height / M3cTextureMap_MaxHeight;
	
	if ((inTextureMap->width & (M3cTextureMap_MaxWidth - 1)) > 0) inTextureMap->num_x++;
	if ((inTextureMap->height & (M3cTextureMap_MaxHeight - 1)) > 0) inTextureMap->num_y++;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
M3iBigTextureUpdate(
	M3tTextureMap_Big		*inTextureMap)
{
	UUtUns32					i;

	// go through the sub_textures and update each one
	for (i = 0; i < inTextureMap->num_textures; i++)
	{
		TMrInstance_Update(inTextureMap->textures[i]);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
M3iBigTextureProcHandler(
	TMtTemplateProc_Message	inMessage,
	void*					inInstancePtr,
	void*					inPrivateData)
{
	UUtError				error;
	M3tTextureMap_Big		*texture_map;
	
	UUmAssert(inInstancePtr);
	
	// get a pointer to the texture map
	texture_map = (M3tTextureMap_Big*)inInstancePtr;
	
	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		break;
					
		case TMcTemplateProcMessage_LoadPostProcess:
			error = M3iBigTextureInit(texture_map);
			UUmError_ReturnOnError(error);
		break;
			
		case TMcTemplateProcMessage_DisposePreProcess:
		break;
			
		case TMcTemplateProcMessage_Update:
			error = M3iBigTextureUpdate(texture_map);
			UUmError_ReturnOnError(error);
		break;
		
		default:
			UUmAssert(!"Illegal message");
		break;
	}
	
	return UUcError_None;
}

static void
M3rCacheProc_Load(
	void*		inInstanceData,
	UUtUns32	inAddress)
{

}

static void
M3rCacheProc_Unload(
	void*		inInstanceData,
	UUtUns32	inAddress)
{

}

static UUtUns32
M3rCacheProc_ComputeSize(
	void*		inInstanceData)
{
	
	return 0;
}

UUtError 
M3rInitialize(
	void)
{
	UUtError	error;
	
	UUmAssert(M3cDrawStateIntType_NumTypes <= 32); // make sure we can fit in a UUtUns32

	UUrStartupMessage("initializing 3D display system..");
	
	/*
	 * Register our templates
	 */
		error = M3rRegisterTemplates();
		UUmError_ReturnOnError(error);
	
	M3gNumDrawEngines = 0;
	M3gNumGeomEngines = 0;
	
	error = M3rSort_Initialize();
	UUmError_ReturnOnError(error);
	
	M3gSIMDPresent = UUrPlatform_SIMD_IsPresent();
	
		
	UUrStartupMessage("initializing geometry engines...");
	
	UUrStartupMessage("initializing draw engines...");

/* good-bye, Glide!	
	if (!ONgCommandLine.useOpenGL)	
	{
		MGrDrawEngine_Initialize();
	}

	if (!ONgCommandLine.useGlide) */
	{
	#ifndef __ONADA__
		GLrDrawEngine_Initialize();
	#else
		if (!gl_draw_engine_initialize())
		{
			return 1;
		}
	#endif
	}

	//RVrDrawEngine_Initialize();

	//MSrDrawEngine_Initialize();
	
	MSrGeomEngine_Initialize();

#ifndef __ONADA__
	OGrGeomEngine_Initialize();
#endif
	
#if UUmPlatform == UUmPlatform_Win32

	//MDrDrawEngine_Initialize();
	
#endif
	
	error = M3rManager_Matrix_Initialize();
	UUmError_ReturnOnError(error);
	
	error = M3rManager_Texture_Initialize();
	UUmError_ReturnOnError(error);
	
	error = 
		TMrTemplate_PrivateData_New(
			M3cTemplate_TextureMap_Big,
			0,
			M3iBigTextureProcHandler,
			&M3gTemplate_TextureMapBig_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install big texture proc handler");
	
	error = 
		TMrTemplate_PrivateData_New(
			M3cTemplate_Geometry,
			0,
			M3iGeometryProcHandler,
			&M3gTemplate_Geometry_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install geomety private data");
		
	return UUcError_None;
}

void 
M3rTerminate(
	void)
{

	#if UUmPlatform == UUmPlatform_Win32
		//MDrDrawEngine_Terminate();
	#endif
	
	//GLrDrawEngine_Terminate();
	//MGrDrawEngine_Terminate();
	//MSrDrawEngine_Terminate();
	MSrGeomEngine_Terminate();
#ifndef __ONADA__
	OGrGeomEngine_Terminate();
#endif
	M3rManager_Matrix_Terminate();
	M3rManager_Texture_Terminate();

	M3rSort_Terminate();

	{
		extern void gl_draw_engine_terminate(void); // gl_engine.c

		gl_draw_engine_terminate();
	}

	return;
}

/*
 * Specify the active engine, display device, and mode
 */
UUtError
M3rDrawEngine_MakeActive(
	UUtUns16				inDrawEngineIndex,		// Index into the engine caps list
	UUtUns16				inDisplayDeviceIndex,	// The display to use
	UUtBool					inFullScreen,
	UUtUns16				inDisplayModeIndex)		// Only relevent for full screen mode
{
//	UUmAssert(M3gGeomContext == NULL); -S.S. resolution changing caused this assert to fire
	
	UUmAssert(inDrawEngineIndex < M3gNumDrawEngines);
	
	M3gActiveDrawEngine = inDrawEngineIndex;
	M3gActiveDisplayDevice = inDisplayDeviceIndex;
	M3gActiveDisplayMode = inDisplayModeIndex;
	
	M3gFullScreen = inFullScreen;
	
	return UUcError_None;
}

/*
 * Service query functions
 */

UUtError
M3rGeomEngine_MakeActive(
	UUtUns16				inGeomEngineIndex)
{
	// UUmAssert(M3gGeomContext == NULL); -S.S. resolution swicthing code hits this
	
	UUmAssert(inGeomEngineIndex < M3gNumGeomEngines);

	M3gActiveGeomEngine = inGeomEngineIndex;
	
	return UUcError_None;
}

void
M3rManager_GetActiveDrawEngine(
	UUtUns16	*outActiveDrawEngie,
	UUtUns16	*outActiveDevice,
	UUtUns16	*outActiveMode)
{
	UUmAssertWritePtr(outActiveDrawEngie, sizeof(*outActiveDrawEngie));
	UUmAssertWritePtr(outActiveDevice, sizeof(*outActiveDevice));
	UUmAssertWritePtr(outActiveMode, sizeof(*outActiveMode));

	*outActiveDrawEngie = M3gActiveDrawEngine;
	*outActiveDevice = M3gActiveDisplayDevice;
	*outActiveMode = M3gActiveDisplayMode;

	return;
}

M3tDrawAPI
M3rDrawContext_GetAPI(
	void)
{
	return M3gManagerDrawContext.apiIndex;
}

UUtError
M3rDrawContext_New(
	M3tDrawContextDescriptor*	inDrawContextDescriptor)
{
	UUtError					error;

	UUmAssert(M3gActiveDrawEngine != 0xFFFF);
	
	error = M3rDraw_State_Initialize();
	UUmError_ReturnOnError(error);
	
	error =
		M3gDrawEngineList[M3gActiveDrawEngine].methods.contextPrivateNew(
			inDrawContextDescriptor,
			&M3gManagerDrawContext.drawFuncs,
			M3gFullScreen,
			&M3gManagerDrawContext.apiIndex);
	UUmError_ReturnOnErrorMsg(error, "Could not create draw context private");
	
	error = 
		TMrTemplate_CallProc(
			M3cTemplate_TextureMap,
			TMcTemplateProcMessage_LoadPostProcess);
	UUmError_ReturnOnError(error);
		
	M3gManagerDrawContext.bitVector = NULL;
	
	// initially we are not sorting
	M3gDraw_Sorting = UUcFalse;

	return UUcError_None;
}

void
M3rDrawContext_Delete(
	void)
{

	TMrTemplate_CallProc(
		M3cTemplate_TextureMap,
		TMcTemplateProcMessage_DisposePreProcess);
	
	M3rDraw_State_Terminate();
	
	M3gDrawEngineList[M3gActiveDrawEngine].methods.contextPrivateDelete();
	
	if(M3gManagerDrawContext.bitVector != NULL)
	{
		UUrBitVector_Dispose(M3gManagerDrawContext.bitVector);
	}
}

void
M3rDrawContext_ResetTextures(
	void)
{
	M3gDrawEngineList[M3gActiveDrawEngine].methods.textureResetAll();
}

UUtError 
M3rGeomContext_New(
	M3tDrawContextDescriptor*	inDrawContextDescriptor)
{
	UUtError					error;
			
	// check to make sure that the active draw engine is compatable with this geom engine		
	if(((1 << M3gActiveDrawEngine) & M3gGeomEngineList[M3gActiveGeomEngine].caps.compatibleDrawEngineBV) == 0)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "active draw and geom engines are not compatable");
	}
	
	error =
		M3rDrawContext_New(
			inDrawContextDescriptor);
	UUmError_ReturnOnErrorMsg(error, "Could not setup draw context");
	
	error =
		M3gGeomEngineList[M3gActiveGeomEngine].methods.contextPrivateNew(
			inDrawContextDescriptor,
			&M3gManagerGeomContext.geomContext);
	UUmError_ReturnOnErrorMsg(error, "Could not setup draw context private");
	
	UUmAssert(M3gManagerGeomContext.geomContext != NULL);
	if(M3gManagerGeomContext.geomContext == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not setup draw context private");
	}
	
	// Initialize the camera
		M3rManager_Camera_Initialize();
	
	error = M3rGeom_State_Initialize();
	UUmError_ReturnOnError(error);
		
	M3gGeomContext = M3gManagerGeomContext.geomContext;

	M3rMatrixStack_Clear();
		
	return UUcError_None;
}

void
M3rGeomContext_Delete(
	void)
{
	
	M3rGeom_State_Terminate();
			
	M3gGeomEngineList[M3gActiveGeomEngine].methods.contextPrivateDelete();
	
	M3rDrawContext_Delete();
	
	M3gGeomContext = NULL;
}

UUtError 
M3rManager_Register_DrawEngine(
	M3tDrawEngineCaps*			inDrawEngineCaps,
	M3tDrawEngineMethods*		inDrawEngineMethods)
{
	 M3gDrawEngineList[M3gNumDrawEngines].methods = *inDrawEngineMethods;
	 M3gDrawEngineList[M3gNumDrawEngines].caps = *inDrawEngineCaps;
	 
	 M3gNumDrawEngines++;
	 
	 return UUcError_None;
}

UUtError 
M3rManager_Register_GeomEngine(
	M3tGeomEngineCaps*			inGeomEngineCaps,
	M3tGeomEngineMethods*		inGeomEngineMethods)
{
	 M3gGeomEngineList[M3gNumGeomEngines].methods = *inGeomEngineMethods;
	 M3gGeomEngineList[M3gNumGeomEngines].caps = *inGeomEngineCaps;
	 
	 M3gNumGeomEngines++;

	 return UUcError_None;
}

UUtUns16
M3rDrawEngine_GetNumber(
	void)
{
	return M3gNumDrawEngines;
}

M3tDrawEngineCaps*
M3rDrawEngine_GetCaps(
	UUtUns16	inDrawEngineIndex)
{
	UUmAssert(inDrawEngineIndex < M3gNumDrawEngines);
	
	return &M3gDrawEngineList[inDrawEngineIndex].caps;
}

UUtUns16
M3rGeomEngine_GetNumber(
	void)
{
	return M3gNumGeomEngines;
}

M3tGeomEngineCaps*
M3rGeomEngine_GetCaps(
	UUtUns16	inGeomEngineIndex)
{
	UUmAssert(inGeomEngineIndex < M3gNumGeomEngines);
	
	return &M3gGeomEngineList[inGeomEngineIndex].caps;
}


void 
M3rGeometry_MultiplyAndDraw(
	M3tGeometry *inGeometryObject,
	const M3tMatrix4x3 *inMatrix)
{
	M3rMatrixStack_Push();
	
	M3rMatrixStack_Multiply(inMatrix);
	M3rGeometry_Draw(inGeometryObject);

	M3rMatrixStack_Pop();

	return;
}

// S.S.
UUtBool M3rSinglePassMultitexturingAvailable(
	void)
{
	return M3gManagerDrawContext.drawFuncs->supportSinglePassMultitexture();
}
