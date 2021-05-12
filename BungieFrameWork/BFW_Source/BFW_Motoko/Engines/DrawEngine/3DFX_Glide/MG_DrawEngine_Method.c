/*
	FILE:	MG_DrawEngine_Method.c
	
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

#include "MG_DC_Private.h"

#include "MG_DrawEngine_Method.h"
#include "MG_DrawEngine_Platform.h"

#include "MG_DC_Method_State.h"
#include "MG_DC_Method_Frame.h"
#include "MG_DC_Method_LinePoint.h"
#include "MG_DC_Method_Bitmap.h"
#include "MG_DC_Method_Query.h"
#include "MG_DC_CreateVertex.h"

#include "MG_DrawEngine_Method_Ptrs.h"

#include "rasterizer_3dfx.h"

#include "bfw_cseries.h"

#define MGcSoftware_Version	(0x00010000)

// MGcTexInfo_TableSize should be the same value as M3cNumTextureTypes but keep seperate to generate errors.
// since the table needs to grow when M3cNumTextureTypes changes

// this array is in order i.e. the left column is 0..6 it is direct looked up
static const MGtTexelTypeInfo MGgTexInfoTable[] = 
{
	{	IMcPixelType_ARGB4444,	MGcTextureValid,	MGcTextureExpansion_No,		MGcTextureAlpha_Yes,	GR_TEXFMT_ARGB_4444 },
	{	IMcPixelType_RGB555,	MGcTextureValid,	MGcTextureExpansion_No,		MGcTextureAlpha_No,		GR_TEXFMT_ARGB_1555 },
	{	IMcPixelType_ARGB1555,	MGcTextureValid,	MGcTextureExpansion_No,		MGcTextureAlpha_Yes,	GR_TEXFMT_ARGB_1555 },
	{	IMcPixelType_I8,		MGcTextureValid,	MGcTextureExpansion_No,		MGcTextureAlpha_No,		GR_TEXFMT_INTENSITY_8 },
	{	IMcPixelType_I1,		MGcTextureInvalid,	MGcTextureExpansion_No,		MGcTextureAlpha_No,		GR_TEXFMT_INTENSITY_8 },
	{	IMcPixelType_A8,		MGcTextureValid,	MGcTextureExpansion_No,		MGcTextureAlpha_Yes,	GR_TEXFMT_ALPHA_8 },
	{	IMcPixelType_A4I4,		MGcTextureValid,	MGcTextureExpansion_No,		MGcTextureAlpha_Yes,	GR_TEXFMT_ALPHA_INTENSITY_44 },
	{	IMcPixelType_ARGB8888,	MGcTextureInvalid,	MGcTextureExpansion_No,		MGcTextureAlpha_Yes,	0 },
	{	IMcPixelType_RGB888,	MGcTextureInvalid,	MGcTextureExpansion_No,		MGcTextureAlpha_Yes,	0 },
	{	IMcPixelType_DXT1,		MGcTextureValid,	MGcTextureExpansion_Yes,	MGcTextureAlpha_No,		GR_TEXFMT_RGB_565 },
	{	IMcPixelType_RGB_Bytes,	MGcTextureInvalid,	MGcTextureExpansion_No,		MGcTextureAlpha_No,		0 },
	{	IMcPixelType_RGBA_Bytes,MGcTextureInvalid,	MGcTextureExpansion_No,		MGcTextureAlpha_No,		0 },
	{	IMcPixelType_RGBA5551,	MGcTextureInvalid,	MGcTextureExpansion_No,		MGcTextureAlpha_Yes,	0 },
	{	IMcPixelType_RGBA4444,	MGcTextureInvalid,	MGcTextureExpansion_No,		MGcTextureAlpha_Yes,	0 },
	{	IMcPixelType_RGB565,	MGcTextureValid,	MGcTextureExpansion_No,		MGcTextureAlpha_No,		GR_TEXFMT_RGB_565 },
	{	IMcPixelType_ABGR1555,	MGcTextureInvalid,	MGcTextureExpansion_No,		MGcTextureAlpha_Yes,	GR_TEXFMT_ARGB_1555 }
};

typedef struct MGtPixelFormat
{
	GrScreenResolution_t	glideID;
	UUtUns16				width;
	UUtUns16				height;
} MGtPixelFormat;

#ifndef GR_RESOLUTION_1024x768
#define GR_RESOLUTION_1024x768  0xC
#endif

#ifndef GR_RESOLUTION_1280x1024
#define GR_RESOLUTION_1280x1024 0xD
#endif

#ifndef GR_RESOLUTION_1600x1200
#define GR_RESOLUTION_1600x1200 0xE
#endif

#ifndef GR_RESOLUTION_400x300
#define GR_RESOLUTION_400x300   0xF
#endif

static const MGtPixelFormat MGgPixelFormatTable[] =
{
//	{GR_RESOLUTION_320x200,		320,	200},
//	{GR_RESOLUTION_320x240,		320,	240},
//	{GR_RESOLUTION_400x256,		400,	256},
//	{GR_RESOLUTION_512x384,		512,	384},
//	{GR_RESOLUTION_640x350,		640,	350},
//	{GR_RESOLUTION_640x400,		640,	400},
	{GR_RESOLUTION_640x480,		640,	480},
	{GR_RESOLUTION_800x600,		800,	600},
	{GR_RESOLUTION_960x720,		960,	720},
	{GR_RESOLUTION_856x480,		856,	480},
//	{GR_RESOLUTION_512x256,		512,	256},
	{GR_RESOLUTION_1024x768,	1024,	768},
	{GR_RESOLUTION_1280x1024,	1280,	1024},
	{GR_RESOLUTION_1600x1200,	1600,	1200},
//	{GR_RESOLUTION_400x300,		400,	300},
	{GR_RESOLUTION_NONE,	0, 0},
};



GrScreenResolution_t	MGgMotokoToGlideScreenRes[M3cMaxDisplayModes];

UUtUns16	MGgSizeofPixelInfoTable = sizeof(MGgTexInfoTable) / sizeof(MGtTexelTypeInfo);

MGtDrawContextPrivate*	MGgDrawContextPrivate = NULL;

M3tDrawEngineCaps		MGgDrawEngineCaps;
M3tDrawEngineMethods	MGgDrawEngineMethods;
M3tDrawContextMethods	MGgDrawContextMethods;

UUtInt32				MGgTextureBytesDownloaded;

TMtPrivateData*			MGgDrawEngine_TexureMap_PrivateData = NULL;

static void MGiTextureMap_UpdateTMUInfo(MGtTextureMapPrivate *ioPrivate);

const MGtTexelTypeInfo *MGrTexelType_GetInfo(IMtPixelType inTexelType)
{
	const MGtTexelTypeInfo *texInfo = NULL;

	UUmAssert(MGgSizeofPixelInfoTable == IMcNumPixelTypes);
	UUmAssert(inTexelType <= IMcNumPixelTypes);
	UUmAssert(inTexelType >= 0);
	
	texInfo = MGgTexInfoTable + inTexelType;

	UUmAssert(texInfo->texelType == inTexelType);

	return texInfo;
}

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------


static void
MGiTextureMap_BuildPrivate(
	M3tTextureMap*			inTextureMap,
	MGtTextureMapPrivate*	outPrivate)
{
	const MGtTexelTypeInfo *texelTypeInfo = NULL;
	short glide_level_of_detail_max;
	short glide_level_of_detail_min;
	short glide_aspect_ratio;
	UUtUns16 widthNBits, heightNBits;
	
	UUmAssertReadPtr(inTextureMap, sizeof(*inTextureMap));
	UUmAssertWritePtr(outPrivate, sizeof(*outPrivate));
	UUmAssert(0 == (inTextureMap->flags & M3cTextureFlags_Offscreen));

	UUmAssert((inTextureMap->width >= 1) && (inTextureMap->width <= 256));
	UUmAssert((inTextureMap->height >= 1) && (inTextureMap->height <= 256));

	widthNBits = heightNBits = 15;
	
	while(!(inTextureMap->width & (1 << widthNBits))) widthNBits--;
	while(!(inTextureMap->height & (1 << heightNBits))) heightNBits--;
	
	UUmAssert(inTextureMap->width == 1 << widthNBits);
	UUmAssert(inTextureMap->height == 1 << heightNBits);

	if (widthNBits>heightNBits)
	{
		outPrivate->u_scale= 256.f;
		outPrivate->v_scale= (float) (256>>(widthNBits-heightNBits));
	}
	else
	{
		outPrivate->u_scale= (float) (256>>(heightNBits-widthNBits));
		outPrivate->v_scale= 256.f;
	}

	texelTypeInfo = MGrTexelType_GetInfo(inTextureMap->texelType);

	UUmAssert(texelTypeInfo->textureValid == MGcTextureValid);

	switch (UUmMax(inTextureMap->width, inTextureMap->height))
	{
		case 256: glide_level_of_detail_max= GR_LOD_256; break;
		case 128: glide_level_of_detail_max= GR_LOD_128; break;
		case 64: glide_level_of_detail_max= GR_LOD_64; break;
		case 32: glide_level_of_detail_max= GR_LOD_32; break;
		case 16: glide_level_of_detail_max= GR_LOD_16; break;
		case 8: glide_level_of_detail_max= GR_LOD_8; break;
		case 4: glide_level_of_detail_max= GR_LOD_4; break;
		case 2: glide_level_of_detail_max= GR_LOD_2; break;
		case 1: glide_level_of_detail_max= GR_LOD_1; break;
		default: UUmAssert(0); break;
	}

	switch ((8*inTextureMap->width)/inTextureMap->height)
	{
		case 64: glide_aspect_ratio= GR_ASPECT_8x1; break;
		case 32: glide_aspect_ratio= GR_ASPECT_4x1; break;
		case 16: glide_aspect_ratio= GR_ASPECT_2x1; break;
		case 8: glide_aspect_ratio= GR_ASPECT_1x1; break;
		case 4: glide_aspect_ratio= GR_ASPECT_1x2; break;
		case 2: glide_aspect_ratio= GR_ASPECT_1x4; break;
		case 1: glide_aspect_ratio= GR_ASPECT_1x8; break;
		default: UUmAssert(0); break;
	}

	switch(glide_aspect_ratio)
	{
		case GR_ASPECT_8x1:
		case GR_ASPECT_1x8:
			glide_level_of_detail_min = GR_LOD_8; 
		break;

		case GR_ASPECT_4x1:
		case GR_ASPECT_1x4:
			glide_level_of_detail_min = GR_LOD_4;
		break;

		case GR_ASPECT_2x1:
		case GR_ASPECT_1x2:
			glide_level_of_detail_min = GR_LOD_2;
		break;

		case GR_ASPECT_1x1:
			glide_level_of_detail_min = GR_LOD_1;
		break;

	}

	if (inTextureMap->flags & M3cTextureFlags_HasMipMap) {
		outPrivate->hardware_format.smallLod= glide_level_of_detail_min;		// NOTE: for mipmapping add case here:
	}
	else {
		outPrivate->hardware_format.smallLod= glide_level_of_detail_max;		// NOTE: for mipmapping add case here:
	}

	outPrivate->texture = inTextureMap;
	outPrivate->hardware_format.format= texelTypeInfo->grTextureFormat;
	outPrivate->hardware_format.largeLod= glide_level_of_detail_max;
	outPrivate->hardware_format.aspectRatio= glide_aspect_ratio;

	if (MGcTextureExpansion_Yes == texelTypeInfo->textureExpansion) {
		outPrivate->hardware_format.data= MGgDecompressBuffer;
	}
	else {
		outPrivate->hardware_format.data= inTextureMap->pixels;
	}
	outPrivate->flags = 0;
	outPrivate->flags |= (texelTypeInfo->hasAlpha == MGcTextureAlpha_Yes) ? MGcTextureFlag_HasAlpha : 0;
	outPrivate->flags |= (inTextureMap->flags & M3cTextureFlags_Trilinear) ? MGcTextureFlag_MipMapDither : 0;
	outPrivate->flags |= (inTextureMap->flags & M3cTextureFlags_ClampHoriz) ? MGcTextureFlag_ClampS : 0;
	outPrivate->flags |= (inTextureMap->flags & M3cTextureFlags_ClampVert) ? MGcTextureFlag_ClampT : 0;
	outPrivate->flags |= (inTextureMap->flags & M3cTextureFlags_HasMipMap) ? MGcTextureFlag_HasMipMap: 0;
	outPrivate->flags |= (MGcTextureExpansion_Yes == texelTypeInfo->textureExpansion) ? MGcTextureFlag_Expansion: 0;
	outPrivate->width = inTextureMap->width;
	outPrivate->height = inTextureMap->height;
		
	return;
}

static void MGiTextureMap_UpdateTMUInfo(MGtTextureMapPrivate *ioPrivate)
{
	long size;

	if (ioPrivate->hardware_block_index_tmu0 != NONE) {
		size = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &ioPrivate->hardware_format);
		ioPrivate->hardware_block_dirty_tmu0 = UUcTrue;

		if (size != ioPrivate->hardware_block_size_tmu0) {
			lrar_deallocate(globals_3dfx.texture_memory_cache_tmu0, ioPrivate->hardware_block_index_tmu0);
		}
	}

	if (ioPrivate->hardware_block_index_tmu0_even != NONE) {
		size = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_EVEN, &ioPrivate->hardware_format);
		ioPrivate->hardware_block_dirty_tmu0_even = UUcTrue;

		if (size != ioPrivate->hardware_block_size_tmu0_even) {
			lrar_deallocate(globals_3dfx.texture_memory_cache_tmu0, ioPrivate->hardware_block_index_tmu0_even);
		}
	}

	if (ioPrivate->hardware_block_index_tmu1 != NONE) {
		size = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &ioPrivate->hardware_format);
		ioPrivate->hardware_block_dirty_tmu1 = UUcTrue;

		if (size != ioPrivate->hardware_block_size_tmu1) {
			lrar_deallocate(globals_3dfx.texture_memory_cache_tmu1, ioPrivate->hardware_block_index_tmu1);
		}
	}

	if (ioPrivate->hardware_block_index_tmu1_odd != NONE) {
		size = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_ODD, &ioPrivate->hardware_format);
		ioPrivate->hardware_block_dirty_tmu1_odd = UUcTrue;

		if (size != ioPrivate->hardware_block_size_tmu1_odd) {
			lrar_deallocate(globals_3dfx.texture_memory_cache_tmu1, ioPrivate->hardware_block_index_tmu1_odd);
		}
	}
}

static void MGiTextureMap_ClearTMUInfo(MGtTextureMapPrivate *outPrivate)
{
	UUmAssertWritePtr(outPrivate, sizeof(*outPrivate));

	outPrivate->hardware_block_index_tmu0= NONE;
	outPrivate->hardware_block_index_tmu0_even = NONE;
	outPrivate->hardware_block_index_tmu1= NONE;
	outPrivate->hardware_block_index_tmu1_odd = NONE;

#if 0	// these values are valid only for uploaded 
	outPrivate->hardware_block_dirty_tmu0 = UUcTrue;
	outPrivate->hardware_block_dirty_tmu0_even = UUcTrue;
	outPrivate->hardware_block_dirty_tmu1 = UUcTrue;
	outPrivate->hardware_block_dirty_tmu1_odd = UUcTrue;

	outPrivate->hardware_block_size_tmu0 = 0;
	outPrivate->hardware_block_size_tmu0_even = 0;
	outPrivate->hardware_block_size_tmu1 = 0;
	outPrivate->hardware_block_size_tmu1_odd = 0;
#endif

	return;
}

static UUtError
MGrDrawEngine_Method_Texture_Init(
	M3tTextureMap*				inTextureMap,
	MGtTextureMapPrivate*		inTextureMapPrivate)
{
	UUmAssertReadPtr(inTextureMap, sizeof(*inTextureMap));
	UUmAssertReadPtr(inTextureMapPrivate, sizeof(*inTextureMapPrivate));

	// always set this, even if this is an invalid texture (or offscreen for that matter)
	MGiTextureMap_ClearTMUInfo(inTextureMapPrivate);
	
	// is this an offscreen texture
	if (inTextureMap->flags & M3cTextureFlags_Offscreen) return UUcError_None;

	// build the private data (does not include the TMU data)
	MGiTextureMap_BuildPrivate(inTextureMap, inTextureMapPrivate);
	

	return UUcError_None;
}

static UUtError
MGrDrawEngine_Method_Texture_Update(
	M3tTextureMap*				inTextureMap,
	MGtTextureMapPrivate*		inTextureMapPrivate)
{
	UUmAssertReadPtr(inTextureMap, sizeof(*inTextureMap));
	UUmAssertReadPtr(inTextureMapPrivate, sizeof(*inTextureMapPrivate));

	// is this an offscreen texture
	if (inTextureMap->flags & M3cTextureFlags_Offscreen) return UUcError_None;

	// build the private data (does not include the TMU data)
	MGiTextureMap_BuildPrivate(inTextureMap, inTextureMapPrivate);


	// update the tmu info based on the changes to privateData
	MGiTextureMap_UpdateTMUInfo(inTextureMapPrivate);

	set_current_glide_texture(GR_TMU0, NULL, MGcTextureUploadMode_Invalid);
	set_current_glide_texture(GR_TMU1, NULL, MGcTextureUploadMode_Invalid);

	return UUcError_None;
}

static UUtError
MGrDrawEngine_Method_Texture_Delete(
	M3tTextureMap*				inTextureMap,
	MGtTextureMapPrivate*		inTextureMapPrivate)
{
	UUmAssertReadPtr(inTextureMap, sizeof(*inTextureMap));
	UUmAssertReadPtr(inTextureMapPrivate, sizeof(*inTextureMapPrivate));

	// is this an offscreen texture
	if (inTextureMap->flags & M3cTextureFlags_Offscreen) return UUcError_None;
	
	if ((globals_3dfx.texture_memory_cache_tmu0 != NULL) && (inTextureMapPrivate->hardware_block_index_tmu0 != NONE))
	{
		lrar_deallocate(globals_3dfx.texture_memory_cache_tmu0, inTextureMapPrivate->hardware_block_index_tmu0);
	}

	if ((globals_3dfx.texture_memory_cache_tmu1 != NULL) && (inTextureMapPrivate->hardware_block_index_tmu1 != NONE))
	{
		lrar_deallocate(globals_3dfx.texture_memory_cache_tmu1, inTextureMapPrivate->hardware_block_index_tmu1);
	}
	
	return UUcError_None;
}

static UUtError
MGiDrawEngine_TextureMap_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void*					inInstancePtr,
	void*					inPrivateData)
{
	M3tTextureMap *inTexture = (M3tTextureMap*)inInstancePtr;
	UUtError	error;

{	
	void
	MGrDrawContext_VerifyCaches(
		void);
	MGrDrawContext_VerifyCaches();
}
	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
			inTexture->flags |= M3cTextureFlags_Offscreen;
			inTexture->debugName[0] = '\0';

			error = MGrDrawEngine_Method_Texture_Init(inTexture, (MGtTextureMapPrivate*)inPrivateData);
			UUmError_ReturnOnError(error);
			break;
			
		case TMcTemplateProcMessage_LoadPostProcess:
			M3rTextureMap_Prepare(inTexture);

			error = MGrDrawEngine_Method_Texture_Init(inTexture, (MGtTextureMapPrivate*)inPrivateData);
			UUmError_ReturnOnError(error);
			break;
			
		case TMcTemplateProcMessage_DisposePreProcess:
			error = MGrDrawEngine_Method_Texture_Delete(inTexture, (MGtTextureMapPrivate*)inPrivateData);
			UUmError_ReturnOnError(error);
			break;
			
		case TMcTemplateProcMessage_Update:
			error = MGrDrawEngine_Method_Texture_Update(inTexture, (MGtTextureMapPrivate*)inPrivateData);
			UUmError_ReturnOnError(error);
			break;
			
		case TMcTemplateProcMessage_PrepareForUse:
			break;
	}
	
{	
	void
	MGrDrawContext_VerifyCaches(
		void);
	MGrDrawContext_VerifyCaches();
}
	return UUcError_None;
}
	


// ----------------------------------------------------------------------
static void
MGrDrawEngine_Method_ContextPrivateDelete(
	void)
{
	UUmAssertReadPtr(MGgDrawEngine_TexureMap_PrivateData, sizeof(void*));
	
	TMrTemplate_PrivateData_Delete(MGgDrawEngine_TexureMap_PrivateData);
	MGgDrawEngine_TexureMap_PrivateData = NULL;
	
	// grsstclose
	dispose_3dfx();
	
	UUrMemory_Block_Delete(MGgDrawContextPrivate);
	MGgDrawContextPrivate = NULL;
}

// ----------------------------------------------------------------------
static UUtError
MGrDrawEngine_Method_ContextPrivateNew(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	M3tDrawContextMethods*		*outDrawContextFuncs,
	UUtBool						inFullScreen,
	M3tDrawAPI*					outAPI)
{
	UUtError				error;
	UUtError				errorCode;
	UUtUns16				activeDrawEngine;
	UUtUns16				activeDevice;
	UUtUns16				activeMode;
	M3tDrawEngineCaps*		drawEngineCaps;
	
	UUmAssert(MGgDrawContextPrivate == NULL);
	
	MGgDrawContextPrivate = UUrMemory_Block_New(sizeof(MGtDrawContextPrivate));
	
	if(MGgDrawContextPrivate == NULL)
	{
		UUrError_Report(UUcError_OutOfMemory, "Could not allocated private draw context");
		return UUcError_OutOfMemory;
	}
	
	/*
	 * Get the active draw engine info
	 */
	 	
		M3rManager_GetActiveDrawEngine(
			&activeDrawEngine,
			&activeDevice,
			&activeMode);
	
	 	drawEngineCaps = M3rDrawEngine_GetCaps(activeDrawEngine);

		*outAPI = M3cDrawAPI_Glide;

	/*
	 * initialize methods
	 */
		MGgDrawContextMethods.frameStart = MGrDrawContext_Method_Frame_Start;
		MGgDrawContextMethods.frameEnd = MGrDrawContext_Method_Frame_End;
		MGgDrawContextMethods.frameSync = MGrDrawContext_Method_Frame_Sync;
		MGgDrawContextMethods.triangle = NULL;
		MGgDrawContextMethods.quad = NULL;
		MGgDrawContextMethods.pent = NULL;
		MGgDrawContextMethods.line = NULL;
		MGgDrawContextMethods.point = MGrPoint;
		MGgDrawContextMethods.triSprite = MGrDrawContext_Method_TriSprite;
		MGgDrawContextMethods.sprite = MGrDrawContext_Method_Sprite;
		MGgDrawContextMethods.screenCapture = MGrDrawContext_Method_ScreenCapture;
		MGgDrawContextMethods.pointVisible = MGrDrawContext_Method_PointVisible;
		MGgDrawContextMethods.textureFormatAvailable = MGrDrawContext_TextureFormatAvailable;
		
		*outDrawContextFuncs = &MGgDrawContextMethods;
		
	MGgDrawContextPrivate->contextType = inDrawContextDescriptor->type;
	
	MGgDrawContextPrivate->width= drawEngineCaps[activeDrawEngine].displayDevices[activeDevice].displayModes[activeMode].width;
	MGgDrawContextPrivate->height= drawEngineCaps[activeDrawEngine].displayDevices[activeDevice].displayModes[activeMode].height;
		
	initialize_3dfx(
		MGgMotokoToGlideScreenRes[activeMode]);
	
	// Init some state
		MGgDrawContextPrivate->vertexList = NULL;
		MGgDrawContextPrivate->curBaseTexture = NULL;
	
	// create the texture private data
		error = 
			TMrTemplate_PrivateData_New(
				M3cTemplate_TextureMap,
				sizeof(MGtTextureMapPrivate),
				MGiDrawEngine_TextureMap_ProcHandler,
				&MGgDrawEngine_TexureMap_PrivateData);
		UUmError_ReturnOnError(error);

	
	return UUcError_None;
	
//failure:
	
	MGrDrawEngine_Method_ContextPrivateDelete();
	
	return errorCode;
}

static void
MGrDrawEngine_Method_Texture_ResetAll(
	void)
{
}

// ----------------------------------------------------------------------
static UUtError
MGrDrawEngine_Method_PrivateState_New(
	void*	inState_Private)
{
	MGtStatePrivate*	statePrivate = (MGtStatePrivate*)inState_Private;
	
	statePrivate->vertexList = NULL;
	statePrivate->vertexListBV = NULL;
	
	statePrivate->vertexList = UUrMemory_Block_New(MGcMaxElements * sizeof(GrVertex));
	UUmError_ReturnOnNull(statePrivate->vertexList);
	
	statePrivate->vertexListBV = UUrBitVector_New(MGcMaxElements);
	UUmError_ReturnOnNull(statePrivate->vertexListBV);
	
	return UUcError_None;
}

// This lets the engine delete a new private state structure
static void 
MGrDrawEngine_Method_PrivateState_Delete(
	void*	inState_Private)
{
	MGtStatePrivate*	statePrivate = (MGtStatePrivate*)inState_Private;
	
	if(statePrivate->vertexList != NULL)
	{
		UUrMemory_Block_Delete(statePrivate->vertexList);
	}
	
	if(statePrivate->vertexListBV != NULL)
	{
		UUrBitVector_Dispose(statePrivate->vertexListBV);
	}
	
	statePrivate->vertexList = NULL;
	statePrivate->vertexListBV = NULL;
}

// This lets the engine update the state according to the update flags
static UUtError
MGrDrawEngine_Method_State_Update(
	void*			inState_Private,
	UUtUns32		inState_IntFlags,
	const UUtInt32*	inState_Int,
	UUtInt32		inState_PtrFlags,
	const void**	inState_Ptr)
{
	MGtStatePrivate*			statePrivate = (MGtStatePrivate*)inState_Private;
	UUtUns16					fill;
	UUtUns16					appearance;
	UUtUns16					interpolation;
	UUtUns16					tmuMode;
	M3tDrawStateVertexFormat	vertexMode;
	
	MGgDrawContextPrivate->vertexList = statePrivate->vertexList;
	MGgDrawContextPrivate->stateInt = inState_Int;
	MGgDrawContextPrivate->statePtr = inState_Ptr;
	
	if(inState_IntFlags & (1 << M3cDrawStateIntType_ZCompare))
	{
		if(inState_Int[M3cDrawStateIntType_ZCompare] == M3cDrawState_ZCompare_On)
		{
			#if MGcUseZBuffer
				grDepthBufferFunction(GR_CMP_GEQUAL);
			#else
				grDepthBufferFunction(GR_CMP_LEQUAL);
			#endif
		}
		else
		{
			grDepthBufferFunction(GR_CMP_ALWAYS);
		}
	}
	
	if(inState_IntFlags & (1 << M3cDrawStateIntType_ZWrite))
	{
		if(inState_Int[M3cDrawStateIntType_ZWrite] == M3cDrawState_ZWrite_On)
		{
			MGrSet_ZWrite(MGcZWrite_On);
		}
		else
		{
			MGrSet_ZWrite(MGcZWrite_Off);
		}
	}
	
	if((inState_Int[M3cDrawStateIntType_Appearence] != M3cDrawState_Appearence_Gouraud) && 
		(inState_Int[M3cDrawStateIntType_Fill] == M3cDrawState_Fill_Solid))
	{
		if(inState_PtrFlags & (1 << M3cDrawStatePtrType_BaseTextureMap))
		{
			M3tTextureMap* textureMap = (M3tTextureMap*)inState_Ptr[M3cDrawStatePtrType_BaseTextureMap];
			
			if(textureMap != NULL)
			{
				MGrSet_TextureMode(textureMap);
			}
			else
			{
				MGgDrawContextPrivate->curBaseTexture = NULL;
			}
		}
	}
	else
	{
		MGgDrawContextPrivate->curBaseTexture = NULL;
	}
	
	if(inState_IntFlags & ((1 << M3cDrawStateIntType_ConstantColor) | (1 << M3cDrawStateIntType_Alpha)))
	{
		UUtUns32 	alpha = inState_Int[M3cDrawStateIntType_Alpha];
		UUtUns32	glideColor; 

		UUmAssert(alpha <= 0xff);
				
		glideColor = 
			(alpha & 0xff) << 24 | 
			(inState_Int[M3cDrawStateIntType_ConstantColor] & 0x00ffffff);

		grConstantColorValue(glideColor);
	}
	
	fill = (UUtUns16)inState_Int[M3cDrawStateIntType_Fill];
	appearance = (UUtUns16)inState_Int[M3cDrawStateIntType_Appearence];
	interpolation = (UUtUns16)inState_Int[M3cDrawStateIntType_Interpolation];
	tmuMode = (UUtUns16)globals_3dfx.numTMU - 1;
	vertexMode = (M3tDrawStateVertexFormat)inState_Int[M3cDrawStateIntType_VertexFormat];
	
	if(inState_IntFlags & (
			(1 << M3cDrawStateIntType_Appearence) |
			(1 << M3cDrawStateIntType_Interpolation) |
			(1 << M3cDrawStateIntType_Fill) |
			(1 << M3cDrawStateIntType_VertexFormat)))
	{
		MGtModeFunction	modeFunction;

		MGgDrawContextMethods.line		= MGgLineFuncs[interpolation];
		
		if(vertexMode == M3cDrawStateVertex_Unified)
		{
			MGgDrawContextMethods.triangle	= (M3tDrawContextMethod_Triangle)MGgTriangleFuncs_VertexUnified[fill][appearance][interpolation];
			MGgDrawContextMethods.quad		= (M3tDrawContextMethod_Quad)MGgQuadFuncs_VertexUnified[fill][appearance][interpolation];
			MGgDrawContextMethods.pent		= (M3tDrawContextMethod_Pent)MGgPentFuncs_VertexUnified[fill][appearance][interpolation];

			modeFunction = MGgModeFuncs_VertexUnified[fill][appearance][interpolation];
		}
		else if(vertexMode == M3cDrawStateVertex_Split)
		{			
			MGgDrawContextMethods.triangle	= (M3tDrawContextMethod_Triangle)MGgTriangleFuncs_VertexSplit[fill][appearance][interpolation][tmuMode];
			MGgDrawContextMethods.quad		= (M3tDrawContextMethod_Quad)MGgQuadFuncs_VertexSplit[fill][appearance][interpolation][tmuMode];
			MGgDrawContextMethods.pent		= (M3tDrawContextMethod_Pent)MGgPentFuncs_VertexSplit[fill][appearance][interpolation][tmuMode];

			modeFunction = MGgModeFuncs_VertexSplit[fill][appearance][interpolation][tmuMode];
		}
		else
		{
			UUmAssert(!"Illegal vertex mode");
		}
		
		modeFunction();
		
	}
	
	if(interpolation == M3cDrawState_Interpolation_Vertex && appearance == M3cDrawState_Appearence_Texture_Lit)
	{
		UUmAssert(globals_3dfx.currentColorCombine == MGcColorCombine_TextureGouraud);
	}
	
	if(inState_PtrFlags & (1 << M3cDrawStatePtrType_ScreenPointArray))
	{
		if(inState_Int[M3cDrawStateIntType_NumRealVertices] > 0)
		{
			MGtVertexCreateFunction vertexCreateFunction;
			
			if(inState_Int[M3cDrawStateIntType_VertexFormat] == M3cDrawStateVertex_Unified)
			{
				vertexCreateFunction = MGgVertexCreateFuncs_Unified[appearance][interpolation];
			}
			else
			{
				vertexCreateFunction = MGgVertexCreateFuncs_Split[appearance][interpolation][tmuMode];
			}
			
			vertexCreateFunction();
		}
	}
	
	MGgDrawContextPrivate->clipping = (UUtBool)inState_Int[M3cDrawStateIntType_Clipping];
	
	MGgBufferClear = (UUtBool) inState_Int[M3cDrawStateIntType_BufferClear];
	MGgDoubleBuffer = (UUtBool) inState_Int[M3cDrawStateIntType_DoubleBuffer];

	return UUcError_None;
}



// ----------------------------------------------------------------------
void
MGrDrawEngine_Initialize(
	void)
{
	UUtError error;
	
	UUtUns16				curDisplayModeIndex;
	UUtUns16				itr;
	
	// available_3dfx() loads the glide DLL and must be called before
	// initialize_3dfx().
	if (!available_3dfx())
	{
		return;
	}

	MGgGamma = 1.3f;
	MGgFilteringOverrideMode = 0;
	MGgSlowLightmaps = 0;
	MGgBilinear = 1;
	MGgMipMapping = 1;

	MGgDrawEngineMethods.contextPrivateNew = MGrDrawEngine_Method_ContextPrivateNew;
	MGgDrawEngineMethods.contextPrivateDelete = MGrDrawEngine_Method_ContextPrivateDelete;
	MGgDrawEngineMethods.textureResetAll = MGrDrawEngine_Method_Texture_ResetAll;
	
	MGgDrawEngineMethods.privateStateSize = sizeof(MGtStatePrivate);
	MGgDrawEngineMethods.privateStateNew = MGrDrawEngine_Method_PrivateState_New;
	MGgDrawEngineMethods.privateStateDelete = MGrDrawEngine_Method_PrivateState_Delete;
	MGgDrawEngineMethods.privateStateUpdate = MGrDrawEngine_Method_State_Update;
	
	MGgDrawEngineCaps.engineFlags = M3cDrawEngineFlag_3DOnly;
	
	UUrString_Copy(MGgDrawEngineCaps.engineName, M3cDrawEngine_3DFX_Glide2x, M3cMaxNameLen);
	MGgDrawEngineCaps.engineDriver[0] = 0;
	
	MGgDrawEngineCaps.engineVersion = MGcSoftware_Version;
	
	MGgDrawEngineCaps.numDisplayDevices = 1;
	MGgDrawEngineCaps.displayDevices[0].numDisplayModes = 0;
	
	for(itr = 0;
		MGgPixelFormatTable[itr].glideID != GR_RESOLUTION_NONE && itr < M3cMaxDisplayModes;
		itr++)
	{
		if((UUtUns32)(MGgPixelFormatTable[itr].width * MGgPixelFormatTable[itr].height) <
			MGgFrameBufferBytes)
		{
			curDisplayModeIndex = MGgDrawEngineCaps.displayDevices[0].numDisplayModes++;
			MGgDrawEngineCaps.displayDevices[0].displayModes[curDisplayModeIndex].width = 
				MGgPixelFormatTable[itr].width;
			MGgDrawEngineCaps.displayDevices[0].displayModes[curDisplayModeIndex].height = 
				MGgPixelFormatTable[itr].height;
			MGgDrawEngineCaps.displayDevices[0].displayModes[curDisplayModeIndex].bitDepth = 16;
			MGgMotokoToGlideScreenRes[curDisplayModeIndex] = MGgPixelFormatTable[itr].glideID;
		}
	}
	
	error =
		M3rManager_Register_DrawEngine(
			&MGgDrawEngineCaps,
			&MGgDrawEngineMethods);
	if(error != UUcError_None)
	{
		UUrError_Report(UUcError_Generic, "Could not setup engine caps");
		return;
	}

}

// ----------------------------------------------------------------------
void
MGrDrawEngine_Terminate(
	void)
{
	terminate_3dfx();
}

// ----------------------------------------------------------------------
UUtError
MGrDrawContext_Method_Frame_Start(
	UUtUns32			inGameTime)
{

	//MGgDrawContextPrivate->stateFlags = 0xFF;
	
	if (MGgBufferClear) {
		erase_backbuffer_3dfx();
	}

	start_rasterizing_3dfx();
	
	MGgTextureBytesDownloaded = 0;
	
	return UUcError_None;
}
	
// ----------------------------------------------------------------------
UUtError
MGrDrawContext_Method_Frame_End(
	UUtUns32	*outTextureBytesDownloaded)
{
	
	*outTextureBytesDownloaded = MGgTextureBytesDownloaded;

	stop_rasterizing_3dfx();
	display_backbuffer_3dfx();
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
MGrDrawContext_Method_Frame_Sync(
	void)
{
	return UUcError_None;
}

void
MGrDrawContext_VerifyCaches(
	void);
void
MGrDrawContext_VerifyCaches(
	void)
{
#ifdef DEBUG
	verify_lrar_cache(globals_3dfx.texture_memory_cache_tmu0);
	verify_lrar_cache(globals_3dfx.texture_memory_cache_tmu1);
#endif
}