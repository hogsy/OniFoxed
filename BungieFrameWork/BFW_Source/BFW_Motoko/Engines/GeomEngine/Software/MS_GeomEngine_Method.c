/*
	FILE:	MS_GeomEngine_Method.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 13, 1997
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_BitVector.h"
#include "BFW_Console.h"
#include "BFW_Platform_AltiVec.h"

#include "MS_GeomEngine_Method.h"

#include "Motoko_Manager.h"

#include "MS_GC_Private.h"
#include "MS_Geom_Clip.h"
#include "MS_GC_Method_Frame.h"
#include "MS_GC_Method_Camera.h"
#include "MS_GC_Method_Light.h"
#include "MS_GC_Method_State.h"
#include "MS_GC_Method_Geometry.h"
#include "MS_GC_Method_Env.h"
#include "MS_GC_Method_Pick.h"

#include "MS_Geom_Transform.h"
#include "MS_Geom_Shade.h"

#if UUmSIMD == UUmSIMD_AltiVec

	#include "MS_Geom_Transform_AltiVec.h"
	#include "MS_Geom_Shade_AltiVec.h"
	
	#include "BFW_Platform.h"

#endif

#define MScGeomVersion	(0x00010000)

MStGeomContextPrivate* MSgGeomContextPrivate = NULL;

M3tGeomContextMethods	MSgGeomContextMethods;

static UUtError
MSrTransformedVertexData_Init(
	MStTransformedVertexData*	inData)
{
	inData->arrayLength = 0xFFFF;
	inData->bitVector = NULL;
	inData->frustumPoints = NULL;
	inData->screenPoints = NULL;
	inData->clipCodes = NULL;
	
	inData->newClipTextureIndex = 0x5555;
	inData->newClipVertexIndex = 0x5555;
	
	inData->maxClipVertices = 0;
	inData->maxClipTextureCords = 0;
	
	return UUcError_None;
}

static void
MSrTransformedVertexData_Delete(
	MStTransformedVertexData*	inData)
{
	if(inData->bitVector != NULL) UUrBitVector_Dispose(inData->bitVector);
	if(inData->frustumPoints != NULL) UUrMemory_Block_Delete(inData->frustumPoints);
	if(inData->screenPoints != NULL) UUrMemory_Block_Delete(inData->screenPoints);
	if(inData->clipCodes != NULL) UUrMemory_Block_Delete(inData->clipCodes);
	
	MSrTransformedVertexData_Init(inData);
}

static UUtError
MSrTransformedVertexData_Alloc(
	MStTransformedVertexData*	inData,
	UUtUns32					inArrayLength)
{
	inData->arrayLength = inArrayLength;
	
	MSrTransformedVertexData_Delete(inData);
	
	inData->bitVector = UUrBitVector_New(inArrayLength);
	UUmError_ReturnOnNull(inData->bitVector);
	
	inData->frustumPoints = UUrMemory_Block_New(sizeof(M3tPoint4D) * inArrayLength);
	UUmError_ReturnOnNull(inData->frustumPoints);
	
	inData->screenPoints = UUrMemory_Block_New(sizeof(M3tPointScreen) * inArrayLength);
	UUmError_ReturnOnNull(inData->screenPoints);
	
	inData->clipCodes = UUrMemory_Block_New(sizeof(UUtUns8) * inArrayLength);
	UUmError_ReturnOnNull(inData->clipCodes);
	
	return UUcError_None;
}

static UUtError
MSrGeomEngine_Method_ContextPrivateNew(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	M3tGeomContextMethods*		*outGeomContextFuncs)
{
	UUtError				error;
	
	
	MSgGeomContextPrivate = UUrMemory_Block_NewClear(sizeof(MStGeomContextPrivate));
	
	if(MSgGeomContextPrivate == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not allocated private geom context");
	}
	
	/*
	 * Initialize the methods
	 */
		MSgGeomContextMethods.frameStart				= MSrGeomContext_Method_Frame_Start;
		MSgGeomContextMethods.frameEnd					= MSrGeomContext_Method_Frame_End;
		MSgGeomContextMethods.pickScreenToWorld			= MSrGeomContext_Method_Pick_ScreenToWorld;
		MSgGeomContextMethods.lightListAmbient			= MSrGeomContext_Method_LightList_Ambient;
		MSgGeomContextMethods.lightListDirectional		= MSrGeomContext_Method_LightList_Directional;
		MSgGeomContextMethods.lightListPoint			= MSrGeomContext_Method_LightList_Point;
		MSgGeomContextMethods.lightListCone				= MSrGeomContext_Method_LightList_Cone;
		MSgGeomContextMethods.geometryBoundingBoxDraw	= MSrGeomContext_Method_Geometry_BoundingBox_Draw;
		MSgGeomContextMethods.spriteDraw				= MSrGeomContext_Method_Sprite_Draw;
		MSgGeomContextMethods.contrailDraw				= MSrGeomContext_Method_Contrail_Draw;
		MSgGeomContextMethods.geometryPolyDraw			= MSrGeomContext_Method_Geometry_PolyDraw;
		MSgGeomContextMethods.geometryLineDraw			= MSrGeomContext_Method_Geometry_LineDraw;
		MSgGeomContextMethods.geometryPointDraw			= MSrGeomContext_Method_Geometry_PointDraw;
		MSgGeomContextMethods.envSetCamera				= MSrGeomContext_Method_Env_SetCamera;
		MSgGeomContextMethods.envDrawGQList				= MSrGeomContext_Method_Env_DrawGQList;
		MSgGeomContextMethods.decalDraw					= MSrGeomContext_Method_Decal_Draw;
		MSgGeomContextMethods.geometryDraw				= MSrGeomContext_Method_Geometry_Draw;
		MSgGeomContextMethods.pointVisible				= MSrGeomContext_Method_PointTestVisible;
		MSgGeomContextMethods.pointVisibleScale			= MSrGeomContext_Method_PointTestVisibleScale;
		MSgGeomContextMethods.spriteArrayDraw			= MSrGeomContext_Method_SpriteArray_Draw;
		MSgGeomContextMethods.skyboxDraw				= MSrGeomContext_Method_Skybox_Draw;
		MSgGeomContextMethods.skyboxCreate				= MSrGeomContext_Method_Skybox_Create;
		MSgGeomContextMethods.skyboxDestroy				= MSrGeomContext_Method_Skybox_Destroy;
				
		*outGeomContextFuncs = &MSgGeomContextMethods;
		
	/*
	 * Initialize temporary memory
	 */
		MSrTransformedVertexData_Init(&MSgGeomContextPrivate->objectVertexData);
		MSrTransformedVertexData_Init(&MSgGeomContextPrivate->gqVertexData);
		//MSrTransformedVertexData_Init(&MSgGeomContextPrivate->otVertexData);

		error = 
			MSrTransformedVertexData_Alloc(
				&MSgGeomContextPrivate->objectVertexData,
				M3cMaxObjVertices);
		UUmError_ReturnOnError(error);
		
		MSgGeomContextPrivate->worldTriNormals = UUrMemory_Block_New(sizeof(M3tVector3D) * M3cMaxObjTris);
		UUmError_ReturnOnNull(MSgGeomContextPrivate->worldTriNormals);
		MSgGeomContextPrivate->worldVertexNormals = UUrMemory_Block_New(sizeof(M3tPoint3D) * M3cMaxObjVertices);
		UUmError_ReturnOnNull(MSgGeomContextPrivate->worldVertexNormals);
		MSgGeomContextPrivate->worldPoints = UUrMemory_Block_New(sizeof(M3tPoint3D) * M3cMaxObjVertices);
		UUmError_ReturnOnNull(MSgGeomContextPrivate->worldPoints);
		
		MSgGeomContextPrivate->worldViewVectors = NULL;
		
		MSgGeomContextPrivate->shades_vertex = UUrMemory_Block_New(sizeof(M3tDiffuseRGB) * M3cMaxObjVertices);
		UUmError_ReturnOnNull(MSgGeomContextPrivate->shades_vertex);
		MSgGeomContextPrivate->shades_tris = UUrMemory_Block_New(sizeof(M3tDiffuseRGB) * M3cMaxObjVertices);
		UUmError_ReturnOnNull(MSgGeomContextPrivate->shades_tris);
		
		MSgGeomContextPrivate->activeVerticesBV = UUrBitVector_New(M3cMaxObjVertices);
		UUmError_ReturnOnNull(MSgGeomContextPrivate->activeVerticesBV);
		
		MSgGeomContextPrivate->activeTrisBV = UUrBitVector_New(MScMaxGQs);
		UUmError_ReturnOnNull(MSgGeomContextPrivate->activeTrisBV);

		MSgGeomContextPrivate->envMapCoords = UUrMemory_Block_New(sizeof(M3tTextureCoord) * M3cMaxObjVertices);
		UUmError_ReturnOnNull(MSgGeomContextPrivate->envMapCoords);
		
		
	/*
	 * Initialize the light data
	 */
		MSgGeomContextPrivate->light_Ambient.color.r = 1.0f;
		MSgGeomContextPrivate->light_Ambient.color.g = 1.0f;
		MSgGeomContextPrivate->light_Ambient.color.b = 1.0f;

 		MSgGeomContextPrivate->light_NumDirectionalLights = 0;

		//MSgGeomContextPrivate->light_NumPointLights = 0;
		//MSgGeomContextPrivate->light_PointList = NULL;

		//MSgGeomContextPrivate->light_NumConeLights = 0;
		//MSgGeomContextPrivate->light_ConeList = NULL;
	
	/*
	 * Initialize the scale values
	 */
		if(inDrawContextDescriptor->type == M3cDrawContextType_OnScreen)
		{
			MSgGeomContextPrivate->width = inDrawContextDescriptor->drawContext.onScreen.rect.right - inDrawContextDescriptor->drawContext.onScreen.rect.left;
			MSgGeomContextPrivate->height = inDrawContextDescriptor->drawContext.onScreen.rect.bottom - inDrawContextDescriptor->drawContext.onScreen.rect.top;
		}
		else
		{
			MSgGeomContextPrivate->width = inDrawContextDescriptor->drawContext.offScreen.inWidth;
			MSgGeomContextPrivate->height = inDrawContextDescriptor->drawContext.offScreen.inHeight;
		}
		
		MSgGeomContextPrivate->scaleX = ((float)MSgGeomContextPrivate->width - 0.0001f) * 0.5f; // S.S. / 2.0f;
		MSgGeomContextPrivate->scaleY = - ((float)MSgGeomContextPrivate->height - 0.0001f) * 0.5f; // S.S. / 2.0f;
	
	// Initialize the state
		#if 0
		MSgGeomContextPrivate->stateTOS = 0;
		MSgGeomContextPrivate->stateInt = MSgGeomContextPrivate->stateIntStack[0];
		
		MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Fill] = M3cGeomState_Fill_Solid;
		MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Shade] = M3cGeomState_Shade_Vertex;
		MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Appearance] = M3cGeomState_Appearance_Texture;
		MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Hint] = M3tGeomState_Hint_None;
		
		MSgGeomContextPrivate->stateDirty = UUcTrue;
		#endif
		
	MSgGeomContextPrivate->stateInt = NULL;

	MSgGeomContextPrivate->lineComputeVertexProc = MSrClip_ComputeVertex_LineFlat;
	
#if TOOL_VERSION
	error = 
		M3rTextureMap_New(
			16,
			16,
			IMcPixelType_RGB555,
			M3cTexture_AllocMemory,
			0,
			"flash texture",
			&MSgGeomContextPrivate->flashTexture);
	UUmError_ReturnOnError(error);
	
	M3rTextureMap_Fill_Shade(
			MSgGeomContextPrivate->flashTexture,
			IMcShade_Red,
			NULL);
#else
	// CB: this debugging texture is not allocated in the ship version
	MSgGeomContextPrivate->flashTexture = NULL;
#endif

#if TOOL_VERSION
	MSgGeomContextPrivate->debugEnvMap = UUrMemory_Block_New(sizeof(MStDebugEnvMap) * M3cMaxObjVertices);
	UUmError_ReturnOnNull(MSgGeomContextPrivate->debugEnvMap);
#else
	// CB: this debugging array is not allocated in the ship version
	MSgGeomContextPrivate->debugEnvMap = NULL;
#endif

	// initialize the processing functions
		MSgGeomContextPrivate->sisdFunctions.transformPointListToScreen =					MSrTransform_Geom_PointListToScreen;
		MSgGeomContextPrivate->sisdFunctions.transformPointListToScreenActive =				MSrTransform_Geom_PointListToScreen_ActiveVertices;
		MSgGeomContextPrivate->sisdFunctions.transformPointListToFrustumScreen =			MSrTransform_Geom_PointListToFrustumScreen;
		MSgGeomContextPrivate->sisdFunctions.transformPointListToFrustumScreenActive =		MSrTransform_Geom_PointListLocalToFrustumScreen_ActiveVertices;
		MSgGeomContextPrivate->sisdFunctions.transformEnvPointListToFrustumScreenActive =	MSrTransform_EnvPointListToFrustumScreen_ActiveVertices;
		MSgGeomContextPrivate->sisdFunctions.transformPointListAndVertexNormalToWorld = 	MSrTransform_Geom_PointListAndVertexNormalToWorld;
		MSgGeomContextPrivate->sisdFunctions.transformPointListAndVertexNormalToWorldComputeViewVector =	MSrTransform_Geom_PointListAndVertexNormalToWorldComputeViewVector;
		MSgGeomContextPrivate->sisdFunctions.transformFaceNormalToWorld =					MSrTransform_Geom_FaceNormalToWorld;
		MSgGeomContextPrivate->sisdFunctions.transformBoundingBoxToFrustumScreen =			MSrTransform_Geom_BoundingBoxToFrustumScreen;
		MSgGeomContextPrivate->sisdFunctions.transformBoundingBoxClipStatus =				MSrTransform_Geom_BoundingBoxClipStatus;
		MSgGeomContextPrivate->sisdFunctions.shadeVerticesGouraud =							MSrShade_Vertices_Gouraud;
		MSgGeomContextPrivate->sisdFunctions.shadeVerticesGouraudActive =					MSrShade_Vertices_GouraudActive;
		MSgGeomContextPrivate->sisdFunctions.shadeFacesGouraud =							MSrShade_Faces_Gouraud;
		MSgGeomContextPrivate->sisdFunctions.backfaceRemove =								MSrBackface_Remove;
		
	#if UUmSIMD == UUmSIMD_AltiVec
	
		MSgGeomContextPrivate->simdFunctions.transformPointListToScreen =					MSrTransform_Geom_PointListToScreen_AltiVec;
		MSgGeomContextPrivate->simdFunctions.transformPointListToScreenActive =				MSrTransform_Geom_PointListToScreen_ActiveVertices_AltiVec;
		MSgGeomContextPrivate->simdFunctions.transformPointListToFrustumScreen =			MSrTransform_Geom_PointListToFrustumScreen_AltiVec;
		MSgGeomContextPrivate->simdFunctions.transformPointListToFrustumScreenActive =		MSrTransform_Geom_PointListLocalToFrustumScreen_ActiveVertices_AltiVec;
		MSgGeomContextPrivate->simdFunctions.transformEnvPointListToFrustumScreenActive =	MSrTransform_EnvPointListToFrustumScreen_ActiveVertices_AltiVec;
		MSgGeomContextPrivate->simdFunctions.transformPointListAndVertexNormalToWorld = 	MSrTransform_Geom_PointListAndVertexNormalToWorld_AltiVec;
		MSgGeomContextPrivate->simdFunctions.transformPointListAndVertexNormalToWorldComputeViewVector =	MSrTransform_Geom_PointListAndVertexNormalToWorldComputeViewVector_AltiVec;
		MSgGeomContextPrivate->simdFunctions.transformFaceNormalToWorld =					MSrTransform_Geom_FaceNormalToWorld_AltiVec;
		MSgGeomContextPrivate->simdFunctions.transformBoundingBoxToFrustumScreen =			MSrTransform_Geom_BoundingBoxToFrustumScreen;
		MSgGeomContextPrivate->simdFunctions.transformBoundingBoxClipStatus =				MSrTransform_Geom_BoundingBoxClipStatus;
		MSgGeomContextPrivate->simdFunctions.shadeVerticesGouraud =							MSrShade_Vertices_Gouraud_Directional_AltiVec;
		MSgGeomContextPrivate->simdFunctions.shadeVerticesGouraudActive =					MSrShade_Vertices_Gouraud_Directional_Active_AltiVec;
		MSgGeomContextPrivate->simdFunctions.shadeFacesGouraud =							MSrShade_Faces_Gouraud;
		MSgGeomContextPrivate->simdFunctions.backfaceRemove =								MSrBackface_Remove_AltiVec;

	#endif
	
	#if UUmSIMD != UUmSIMD_None
		
		if(UUrPlatform_SIMD_IsPresent())
		{
			MSgGeomContextPrivate->worldViewVectors = UUrMemory_Block_New(sizeof(M3tVector3D) * M3cMaxObjVertices);
			UUmError_ReturnOnNull(MSgGeomContextPrivate->worldViewVectors);
		}
		
	#endif
	
	#if UUmSIMD == UUmSIMD_AltiVec
	
		if(1 && UUrPlatform_SIMD_IsPresent())
		{
			MSgGeomContextPrivate->activeFunctions = &MSgGeomContextPrivate->simdFunctions;
		}
		else
	#endif
	
	{
		MSgGeomContextPrivate->activeFunctions = &MSgGeomContextPrivate->sisdFunctions;
	}
	
	//
	#if UUmSIMD != UUmSIMD_None
	
		MSgGeomContextPrivate->envPointSIMD = NULL;
	
	#endif
	
	return UUcError_None;
}

static void
MSrGeomEngine_Method_ContextPrivateDelete(
	void)
{
	
	MSrTransformedVertexData_Delete(&MSgGeomContextPrivate->objectVertexData);
	MSrTransformedVertexData_Delete(&MSgGeomContextPrivate->gqVertexData);
	//MSrTransformedVertexData_Delete(&MSgGeomContextPrivate->otVertexData);
	
	UUrMemory_Block_Delete(MSgGeomContextPrivate->worldTriNormals);
	UUrMemory_Block_Delete(MSgGeomContextPrivate->worldVertexNormals);
	UUrMemory_Block_Delete(MSgGeomContextPrivate->worldPoints);
	UUrMemory_Block_Delete(MSgGeomContextPrivate->shades_vertex);
	UUrMemory_Block_Delete(MSgGeomContextPrivate->shades_tris);
	
	UUrMemory_Block_Delete(MSgGeomContextPrivate->envMapCoords);
	
	if(MSgGeomContextPrivate->worldViewVectors != NULL)
	{
		UUrMemory_Block_Delete(MSgGeomContextPrivate->worldViewVectors);
	}
	
	UUrBitVector_Dispose(MSgGeomContextPrivate->activeVerticesBV);
	UUrBitVector_Dispose(MSgGeomContextPrivate->activeTrisBV);

	if (MSgGeomContextPrivate->debugEnvMap != NULL) {
		UUrMemory_Block_Delete(MSgGeomContextPrivate->debugEnvMap);
		MSgGeomContextPrivate->debugEnvMap = NULL;
	}	

	#if UUmSIMD != UUmSIMD_None
		if(MSgGeomContextPrivate->envPointSIMD != NULL)
		{
			UUrMemory_Block_Delete(MSgGeomContextPrivate->envPointSIMD);
		}
	#endif
		
	UUrMemory_Block_Delete(MSgGeomContextPrivate);
	
	MSgGeomContextPrivate = NULL;
}

static UUtError
MSrGeomEngine_Method_ContextSetEnvironment(
	struct AKtEnvironment*		inEnvironment)
{
	UUtError	error;
	UUtUns32	numFloats;

	numFloats;
	
	if (NULL != inEnvironment) 
	{
		MSgGeomContextPrivate->environment = inEnvironment;
		
		error = 
			MSrTransformedVertexData_Alloc(
				&MSgGeomContextPrivate->gqVertexData,
				inEnvironment->pointArray->numPoints + M3cExtraCoords);
		UUmError_ReturnOnError(error);
				
		MSgGeomContextPrivate->gqVertexData.textureCoords = inEnvironment->textureCoordArray->textureCoords;
		MSgGeomContextPrivate->gqVertexData.worldPoints = inEnvironment->pointArray->points;
		
		#if UUmSIMD != UUmSIMD_None
		{
			if(UUrPlatform_SIMD_IsPresent())
			{
				if(MSgGeomContextPrivate->envPointSIMD != NULL)
				{
					UUrMemory_Block_Delete(MSgGeomContextPrivate->envPointSIMD);
				}
				
				numFloats = (inEnvironment->pointArray->numPoints + 3) & ~0x3;
				numFloats *= 3; 
				
				MSgGeomContextPrivate->envPointSIMD = UUrMemory_Block_New(numFloats * sizeof(float));
				UUmError_ReturnOnNull(MSgGeomContextPrivate->envPointSIMD);
				
				AVrFloat_XYZ4ToXXXXYYYYZZZZ(
					inEnvironment->pointArray->numPoints,
					(float*)inEnvironment->pointArray->points,
					MSgGeomContextPrivate->envPointSIMD);
			}
		}
		#endif
		
	}
	else 
	{
		MSgGeomContextPrivate->environment = NULL;
		
		MSrTransformedVertexData_Delete(&MSgGeomContextPrivate->gqVertexData);
		
		MSgGeomContextPrivate->gqVertexData.textureCoords = NULL;
		MSgGeomContextPrivate->gqVertexData.worldPoints = NULL;

		#if UUmSIMD != UUmSIMD_None
			if(MSgGeomContextPrivate->envPointSIMD != NULL)
			{
				UUrMemory_Block_Delete(MSgGeomContextPrivate->envPointSIMD);
				MSgGeomContextPrivate->envPointSIMD = NULL;
			}
		#endif
	}
	
	return UUcError_None;
}

// This lets the engine allocate a new private state structure
static UUtError
MSrGeomEngine_Method_PrivateState_New(
	void*	inState_Private)
{
	
	return UUcError_None;
}

// This lets the engine delete a new private state structure
static void 
MSrGeomEngine_Method_PrivateState_Delete(
	void*	inState_Private)
{

}

// This lets the engine update the state according to the update flags
static UUtError
MSrGeomEngine_Method_State_Update(
	void*			inState_Private,
	UUtUns16		inState_IntFlags,
	const UUtInt32*	inState_Int)
{
	MSgGeomContextPrivate->stateInt = inState_Int;
	
	if(inState_IntFlags & (1 << M3cGeomStateIntType_Shade))
	{
		switch(inState_Int[M3cGeomStateIntType_Shade])
		{
			case M3cGeomState_Shade_Vertex:
				switch(inState_Int[M3cGeomStateIntType_Appearance])
				{
					case M3cGeomState_Appearance_Gouraud:
						MSgGeomContextPrivate->polyComputeVertexProc = 
							MSrClip_ComputeVertex_GouraudInterpolate;
						break;
					
					case M3cGeomState_Appearance_Texture:
						MSgGeomContextPrivate->polyComputeVertexProc = 
							MSrClip_ComputeVertex_TextureInterpolate;
						break;
					
					default:
						UUmAssert(0);
						break;
				}
				break;

			case M3cGeomState_Shade_Face:
				switch(inState_Int[M3cGeomStateIntType_Appearance])
				{
					case M3cGeomState_Appearance_Gouraud:
						MSgGeomContextPrivate->polyComputeVertexProc = 
							MSrClip_ComputeVertex_GouraudFlat;
						break;
					
					case M3cGeomState_Appearance_Texture:
						MSgGeomContextPrivate->polyComputeVertexProc = 
							MSrClip_ComputeVertex_TextureFlat;
						break;
					
					default:
						UUmAssert(0);
						break;
				}
				break;
			
			default:
				UUmAssert(0);
				break;
		}
	}

	return UUcError_None;
}

static void
MSrGeomEngine_Method_Camera_View_Update(
	void)
{

}

static void
MSrGeomEngine_Method_Camera_Static_Update(
	void)
{

}

void
MSrGeomEngine_Initialize(
	void)
{
	UUtError				error;
	M3tGeomEngineMethods	geomEngineMethods;
	M3tGeomEngineCaps		geomEngineCaps;
	
	geomEngineMethods.contextPrivateNew = MSrGeomEngine_Method_ContextPrivateNew;
	geomEngineMethods.contextPrivateDelete = MSrGeomEngine_Method_ContextPrivateDelete;
	geomEngineMethods.contextSetEnvironment = MSrGeomEngine_Method_ContextSetEnvironment;
	
	geomEngineMethods.privateStateSize = 0;
	geomEngineMethods.privateStateNew = MSrGeomEngine_Method_PrivateState_New;
	geomEngineMethods.privateStateDelete = MSrGeomEngine_Method_PrivateState_Delete;
	geomEngineMethods.privateStateUpdate = MSrGeomEngine_Method_State_Update;
	
	geomEngineMethods.cameraViewUpdate = MSrGeomEngine_Method_Camera_View_Update;
	geomEngineMethods.cameraStaticUpdate = MSrGeomEngine_Method_Camera_Static_Update;
	
	geomEngineCaps.engineFlags = M3tGeomEngineFlag_None;
	
	UUrString_Copy(geomEngineCaps.engineName, M3cGeomEngine_Software, M3cMaxNameLen);
	geomEngineCaps.engineDriver[0] = 0;
	
	geomEngineCaps.engineVersion = MScGeomVersion;
	
	geomEngineCaps.compatibleDrawEngineBV = 0xFFFF;
	
	error = M3rManager_Register_GeomEngine(&geomEngineCaps, &geomEngineMethods);
		
	if(error != UUcError_None)
	{
		UUrError_Report(error, "Could not register motoko software rasterizer engine");
	}
}

void
MSrGeomEngine_Terminate(
	void)
{

}
