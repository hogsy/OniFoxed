/*
 *
 * RASTERIZER_3DFX.C
 *
 */
 
#include "bfw_cseries.h"

#include "lrar_cache.h"

#define DYNAHEADER_CREATE_STORAGE
#include "glide.h"

#include "rasterizer_3dfx.h"
#include "BFW.h"
#include "BFW_Console.h"
#include "BFW_FileFormat.h"

#include "Motoko_Manager.h"

#include "MG_DC_Private.h"

/* ---------- private prototypes */

float MGgGamma = 1.3f;
static float MGgOldGamma = 1.3f;

UUtUns32 MGgSlowLightmaps;
UUtUns32 MGgFilteringOverrideMode;
UUtUns32 MGgBilinear;
UUtBool	MGgMipMapping;
void *MGgDecompressBuffer;

UUtBool MGgDoubleBuffer = 1;
UUtBool MGgBufferClear = 1;

static void UUcExternal_Call dispose_glide_atexit(void);


//static void set_glide_color(pixel32 color);
//static void set_glide_mode(short mode);
//static void set_glide_fog(rgb_color *color, real density);

/* ---------- globals */

static UUtBool gGlideAvailable = UUcFalse;

enum
{
	MAXIMUM_CACHED_TEXTURES_PER_TMU= 2048,
	MAXIMUM_HARDWARE_FORMAT_CACHE_SIZE= 64*1024 // only contains GrTexInfo
};

tGlobals_3dfx	globals_3dfx;
UUtUns32		MGgFrameBufferBytes = 0;

static void display_hardware_info(GrHwConfiguration *hwconfig)
{
	UUtUns16 boardItr;

	UUrStartupMessage("num 3dfx cards %d (sli counts as one card, note we will select card#0)", hwconfig->num_sst);

	for(boardItr = 0; boardItr < hwconfig->num_sst; boardItr++) {
		UUtUns16 texelFxItr;
		GrVoodooConfig_t *curBoard;

		UUrStartupMessage("3dfx board %d", boardItr);
		UUrStartupMessage("  type %d (0 = voodoo)", boardItr, hwconfig->SSTs[boardItr].type);

		curBoard = &hwconfig->SSTs[boardItr].sstBoard.VoodooConfig;

		UUrStartupMessage("  ram %d", curBoard->fbRam);
		UUrStartupMessage("  rev %d", curBoard->fbiRev);
		UUrStartupMessage("  sli %d", curBoard->sliDetect);
		UUrStartupMessage("  num tmu %d", curBoard->nTexelfx);

		for(texelFxItr = 0; texelFxItr < curBoard->nTexelfx; texelFxItr++) {
			GrTMUConfig_t *curTMU = curBoard->tmuConfig + texelFxItr;
			UUrStartupMessage("  tmu %d", texelFxItr);

			UUrStartupMessage("    rev %d", curTMU->tmuRev);
			UUrStartupMessage("    ram %d", curTMU->tmuRam);
		}
	}

	return;
}

/* ---------- code */
void initialize_3dfx(
	GrScreenResolution_t	inScreenResolution)
{
	GrHwConfiguration hwconfig;
	unsigned long minimum_address, maximum_address;
	UUtBool success;

	UUrStartupMessage("initializing 3dfx...");

	UUmAssert(gGlideAvailable);
	
	success = grSstQueryHardware(&hwconfig);
	UUmAssert(success);
	UUrStartupMessage("3dfx hardware query gave us %d...", success);

	UUrStartupMessage("selecting 3dfx display device...");
	grSstSelect(0);
	
	globals_3dfx.numTMU = hwconfig.SSTs[0].sstBoard.VoodooConfig.nTexelfx;
	globals_3dfx.numTMU = UUmMin(globals_3dfx.numTMU, 2);	// only handle 2 tmus

	UUrStartupMessage("opening 3dfx display device, res = %d (if we hang here, you may need to reinstall 3dfx)...", inScreenResolution);

	success= grSstWinOpen(0, inScreenResolution, GR_REFRESH_60Hz, GR_COLORFORMAT_ARGB,
		GR_ORIGIN_UPPER_LEFT, 2, 1); // two page-flipping buffers and a depth buffer
	UUmAssert(success);

	UUrStartupMessage("3dfx display device open returned %d...", success);

	minimum_address = grTexMinAddress(GR_TMU0);
	maximum_address = grTexMaxAddress(GR_TMU0);
	globals_3dfx.texture_memory_cache_tmu0= 
		lrar_new(	"3dfx texture memory cache 0", 
					minimum_address,
					maximum_address, 
					MAXIMUM_CACHED_TEXTURES_PER_TMU, 
					4, 
					21,
					(lrar_new_block_proc) NULL, (lrar_purge_block_proc) NULL);
	UUmAssert(globals_3dfx.texture_memory_cache_tmu0);

	if (globals_3dfx.numTMU >= 2) {
		minimum_address = grTexMinAddress(GR_TMU1);
		maximum_address = grTexMaxAddress(GR_TMU1);
		globals_3dfx.texture_memory_cache_tmu1 = 
			lrar_new(	"3dfx texture memory cache 1", 
						minimum_address,
						maximum_address, 
						MAXIMUM_CACHED_TEXTURES_PER_TMU, 
						4, 
						21,
						(lrar_new_block_proc) NULL, (lrar_purge_block_proc) NULL);
		UUmAssert(globals_3dfx.texture_memory_cache_tmu1);
	}

	globals_3dfx.currentColorCombine = MGcColorCombine_Invalid;
	globals_3dfx.currentAlphaCombine = MGcAlphaCombine_Invalid;
	globals_3dfx.currentZWriteMode = MGcZWrite_Invalid;
	globals_3dfx.currentTextureCombine = MGcTextureCombine_Invalid;
	globals_3dfx.currentMipMapMode_tmu0 = MGcMipMapMode_Invalid;
	globals_3dfx.currentMipMapMode_tmu1 = MGcMipMapMode_Invalid;
	globals_3dfx.current_texture_tmu0 = (GrTexInfo *) NULL;
	globals_3dfx.current_texture_tmu1 = (GrTexInfo *) NULL;
	globals_3dfx.clampS_tmu0 = GR_TEXTURECLAMP_WRAP;
	globals_3dfx.clampT_tmu0 = GR_TEXTURECLAMP_WRAP;
	globals_3dfx.clampS_tmu1 = GR_TEXTURECLAMP_WRAP;
	globals_3dfx.clampT_tmu1 = GR_TEXTURECLAMP_WRAP;
	
	grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
	grTexMipMapMode(GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE);
	grTexLodBiasValue(GR_TMU0, 0.5f);		// per texture would be ideal (.5 if only 1 texture)
	grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
	grTexCombine(GR_TMU0, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
					GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);

	if (globals_3dfx.numTMU >= 2) {
		grTexClampMode(GR_TMU1, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
		grTexMipMapMode(GR_TMU0, GR_MIPMAP_DISABLE, FXFALSE);
		grTexLodBiasValue(GR_TMU0, 0.0f);		// per texture would be ideal
		grTexLodBiasValue(GR_TMU1, 0.0f);		// per texture would be ideal
		grTexFilterMode(GR_TMU1, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
	}

	grHints( GR_HINT_ALLOW_MIPMAP_DITHER, 1);
	
	// clear the backbuffer to green, swap it to front
	#if MGcUseZBuffer
		grBufferClear(0x007F007F, 0, GR_ZDEPTHVALUE_FARTHEST);
	#else
		grBufferClear(0x007F007F, 0, GR_WDEPTHVALUE_FARTHEST);
	#endif

	//grGlideShamelessPlug(FXTRUE);
	grBufferSwap(0);
	grRenderBuffer(GR_BUFFER_FRONTBUFFER);

	// no fog
	grFogMode(GR_FOG_DISABLE);

	// depth buffer
#if MGcUseZBuffer
	grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
	grDepthBufferFunction(GR_CMP_GEQUAL);
#else
	grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
	grDepthBufferFunction(GR_CMP_LEQUAL);
#endif
	grDepthMask(FXTRUE);


	grGlideShamelessPlug(FXFALSE);

	erase_backbuffer_3dfx();
	start_rasterizing_3dfx();
	stop_rasterizing_3dfx();
	display_backbuffer_3dfx();

	grBufferSwap(1);
	grRenderBuffer(GR_BUFFER_BACKBUFFER);

	while (grBufferNumPending()>0) ;
	
	if(MGgDecompressBuffer != NULL)
	{
		UUrMemory_Block_Delete(MGgDecompressBuffer);
	}
	
	MGgDecompressBuffer = UUrMemory_Block_New((256 * 256 * 2 * 3) / 2);
	UUmAssert(NULL != MGgDecompressBuffer);

	UUrStartupMessage("finished opening 3dfx...");

	return;
}

void terminate_3dfx(void)
{
	if (grGlideShutdown != NULL)
	{
		grGlideShutdown();
	}

	if (NULL != MGgDecompressBuffer) {
		UUrMemory_Block_Delete(MGgDecompressBuffer);
	}

	return;
}

void dispose_3dfx(
	void)
{
	if (globals_3dfx.texture_memory_cache_tmu0 != NULL)
	{
		lrar_dispose(globals_3dfx.texture_memory_cache_tmu0);
	}
	
	if (globals_3dfx.texture_memory_cache_tmu1 != NULL)
	{
		lrar_dispose(globals_3dfx.texture_memory_cache_tmu1);
	}

	grSstWinClose();

	return;
}

static void FX_CALL glide_error(const char *string, FxBool fatal)
{
	UUrEnterDebugger("%s %s", fatal ? "fatal" : "not-fatal", string);
}

UUtBool available_3dfx(
	void)
{
	static UUtBool first_time= UUcTrue;
	static UUtBool has_3dfx= UUcFalse;
	static UUtBool has_glide = UUcFalse;
	UUtBool success;

	UUrStartupMessage("testing for presence of 3dfx..");

	globals_3dfx.texture_memory_cache_tmu0 = NULL;
	globals_3dfx.texture_memory_cache_tmu1 = NULL;

	if (first_time)
	{
#if UUmPlatform == UUmPlatform_Win32
		UUtBool	has_glide = GetProcAddresses();
#elif UUmPlatform == UUmPlatform_Mac
		UUtBool has_glide = grSstQueryBoards != 0;
#endif

		if (has_glide)
		{
			GrHwConfiguration hwconfig;

			grGlideInit(); // can only call this once
			UUrAtExit_Register(dispose_glide_atexit);

			grErrorSetCallback(glide_error);

			if (grSstQueryBoards(&hwconfig) && hwconfig.num_sst>0)
			{
				UUrStartupMessage("detected 3dfx.. (%d boards)", hwconfig.num_sst);

				has_3dfx= UUcTrue;
			}
			else 
			{
				UUrStartupMessage("did not detect 3dfx hardware");
			}
		}
		else
		{
			UUrStartupMessage("did not detect glide (3dfx driver software)");
		}

		if (has_3dfx)
		{
			GrHwConfiguration hwconfig;

			success = grSstQueryHardware(&hwconfig);
			UUmAssert(success);
			UUrStartupMessage("3dfx hardware query gave us %d...", success);

			display_hardware_info(&hwconfig);
			MGgFrameBufferBytes = 1024 * 1024 * hwconfig.SSTs[0].sstBoard.VoodooConfig.fbRam;
		}

		first_time= UUcFalse;
	}

	gGlideAvailable = has_3dfx;
	
	return has_3dfx;
}

void start_rasterizing_3dfx(
	void)
{
	return;
}

void stop_rasterizing_3dfx(
	void)
{
	return;
}

void erase_backbuffer_3dfx(
	void)
{
	MGtZWriteMode oldMode = globals_3dfx.currentZWriteMode;
	globals_3dfx.currentZWriteMode = MGcZWrite_Invalid;

	MGrSet_ZWrite(MGcZWrite_On);

#if MGcUseZBuffer
	grBufferClear(0, 0, GR_ZDEPTHVALUE_FARTHEST);
#else
	grBufferClear(0, 0, GR_WDEPTHVALUE_FARTHEST);
#endif

	MGrSet_ZWrite(oldMode);
	
	return;
}

void lock_backbuffer_3dfx(
	void)
{
#if 0
	GrLfbInfo_t info;

	info.size= sizeof(GrLfbInfo_t);
	if (grLfbLock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER, GR_LFBWRITEMODE_565,
		GR_ORIGIN_UPPER_LEFT, FXFALSE, &info))
	{
		short bytes_per_row= (short) info.strideInBytes;
		void *base_address= (byte *) info.lfbPtr + bounds->top*bytes_per_row +
			bounds->left*get_encoding_byte_depth(_16bit_565_encoding);

		initialize_bitmap(bitmap, rectangle2d_width(bounds), rectangle2d_height(bounds),
			bytes_per_row, _16bit_565_encoding, 0, base_address);
	}
#endif

	return;	
}

void unlock_backbuffer_3dfx(
	void)
{
	grLfbUnlock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER);
		
	return;
}

void display_backbuffer_3dfx(
	void)
{
	while (grBufferNumPending()>1) ;

	if (MGgGamma != MGgOldGamma) {
		grGammaCorrectionValue(MGgGamma);
	}
	
	if (MGgDoubleBuffer) {
		grBufferSwap(1);
		grRenderBuffer(GR_BUFFER_BACKBUFFER);
	}
	else {
		grRenderBuffer(GR_BUFFER_FRONTBUFFER);
	}
	
	if (MGgBilinear) {
		grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
		if (globals_3dfx.numTMU >= 2) {
			grTexFilterMode(GR_TMU1, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
		}
	}
	else {
		grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);
		if (globals_3dfx.numTMU >= 2) {
			grTexFilterMode(GR_TMU1, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);
		}
	}

	//lrar_flush(globals_3dfx.texture_memory_cache_tmu0);
	//lrar_flush(globals_3dfx.texture_memory_cache_tmu1);

	return;
}

/* ---------- private code */

static void UUcExternal_Call dispose_glide_atexit(
	void)
{
	grGlideShutdown();
	
	return;
}

static void
myGRTexDownloadMipMap(	GrChipID_t tmu,
						FxU32      startAddress,
						FxU32      evenOdd,
						GrTexInfo  *info,
						long		size)
{	
	MGgTextureBytesDownloaded += size;

	grTexDownloadMipMap(tmu, startAddress, evenOdd, info);
}

void MGrSet_TexClampMode(GrChipID_t tmu, GrTextureClampMode_t clampS, GrTextureClampMode_t clampT)
{
	GrTextureClampMode_t *globalClampS;
	GrTextureClampMode_t *globalClampT;

	switch(tmu)
	{
		case 0:
			globalClampS = &globals_3dfx.clampS_tmu0;
			globalClampT = &globals_3dfx.clampT_tmu0;
		break;

		case 1:
			globalClampS = &globals_3dfx.clampS_tmu1;
			globalClampT = &globals_3dfx.clampT_tmu1;
		break;

		default:
		UUmAssert(!"invalid case");
	}

	if ((*globalClampS == clampS) && (*globalClampT == clampT)) {
		return;
	}

	grTexClampMode(tmu, clampS, clampT);

	*globalClampS = clampS;
	*globalClampS = clampT;

	return;
}

void
set_current_glide_texture(
	GrChipID_t				inTMU,
	MGtTextureMapPrivate*	inTexture,
	MGtTextureUploadMode	mode)
{	
	GrTexInfo**			current_texture_ptr; 
	GrTexInfo*			hardware_format; 
	short*				hardware_block_index;
	lrar_cache*	lrar_cache;
	short*				current_block_index;
	UUtBool*			hardware_block_dirty;
	long*				hardware_block_size;
	MGtMipMapMode		mipMapMode;
	FxU32				evenOdd;

	static MGtTextureMapPrivate	*last_texture_tmu0 = (void *) -1;
	static MGtTextureMapPrivate	*last_texture_tmu1 = (void *) -1;
	static MGtTextureUploadMode	last_mode_tmu0 = MGcTextureUploadMode_Invalid;
	static MGtTextureUploadMode	last_mode_tmu1 = MGcTextureUploadMode_Invalid;

	if (NULL != inTexture) {
		M3rTextureMap_Prepare(inTexture->texture);
	}

	if (GR_TMU0 == inTMU)
	{
		if ((last_texture_tmu0 == inTexture) && (last_mode_tmu0 == mode)) {
			return;
		}

		last_texture_tmu0 = inTexture;
		last_mode_tmu0 = mode;
	}
	else if (GR_TMU1 == inTMU)
	{
		if ((last_texture_tmu1 == inTexture) && (last_mode_tmu1 == mode)) {
			return;
		}

		last_texture_tmu1 = inTexture;
		last_mode_tmu1 = mode;
	}

	if (NULL == inTexture)
	{
		if (GR_TMU0 == inTMU)
		{
			globals_3dfx.current_texture_tmu0 = NULL;
		}
		else if (GR_TMU1 == inTMU)
		{
			globals_3dfx.current_texture_tmu1 = NULL;
		}
		else
		{
			UUmAssert(0);
		}

		return;
	}

	if (MGcTextureUploadMode_Trilinear == mode) {
		mipMapMode = MGcMipMapMode_Trilinear;
	}
	else if (MGcFilteringOverride_NoFiltering == MGgFilteringOverrideMode)
	{
		mipMapMode = MGcMipMapMode_NoMipmapping;
	}
	else if (MGcFilteringOverride_Bilinear == MGgFilteringOverrideMode)
	{
		mipMapMode = MGcMipMapMode_MipMapping;
	}
	else if (MGcFilteringOverride_Dither == MGgFilteringOverrideMode)
	{
		mipMapMode = MGcMipMapMode_Dither;
	}
	else {
		mipMapMode = (inTexture->flags & MGcTextureFlag_MipMapDither) ?
						MGcMipMapMode_Dither :
						((inTexture->flags & MGcTextureFlag_HasMipMap) ? MGcMipMapMode_MipMapping : MGcMipMapMode_NoMipmapping);
	}

	hardware_format = &(inTexture->hardware_format);

	switch(inTMU)
	{
		case GR_TMU0:
			if (MGcTextureUploadMode_Trilinear == mode) {
				hardware_block_index = &(inTexture->hardware_block_index_tmu0_even);
				hardware_block_size = &(inTexture->hardware_block_size_tmu0_even);
				hardware_block_dirty = &(inTexture->hardware_block_dirty_tmu0_even);
				evenOdd = GR_MIPMAPLEVELMASK_EVEN;
			}
			else {
				hardware_block_index = &(inTexture->hardware_block_index_tmu0);
				hardware_block_size = &(inTexture->hardware_block_size_tmu0);
				hardware_block_dirty = &(inTexture->hardware_block_dirty_tmu0);
				evenOdd = GR_MIPMAPLEVELMASK_BOTH;
			}

			current_block_index = &globals_3dfx.current_blockindex_tmu0;
			lrar_cache = globals_3dfx.texture_memory_cache_tmu0;
			current_texture_ptr = &(globals_3dfx.current_texture_tmu0);
			break;
		
		case GR_TMU1:
			if (MGcTextureUploadMode_Trilinear == mode) {
				hardware_block_index = &(inTexture->hardware_block_index_tmu1_odd);
				hardware_block_size = &(inTexture->hardware_block_size_tmu1_odd);
				hardware_block_dirty = &(inTexture->hardware_block_dirty_tmu1_odd);
				evenOdd = GR_MIPMAPLEVELMASK_ODD;
			}
			else {
				hardware_block_index = &(inTexture->hardware_block_index_tmu1);
				hardware_block_size = &(inTexture->hardware_block_size_tmu1);
				hardware_block_dirty = &(inTexture->hardware_block_dirty_tmu1);
				evenOdd = GR_MIPMAPLEVELMASK_BOTH;
			}

			current_block_index = &globals_3dfx.current_blockindex_tmu1;
			lrar_cache = globals_3dfx.texture_memory_cache_tmu1;
			current_texture_ptr = &(globals_3dfx.current_texture_tmu1);
			break;

		default:
			UUmAssert(0);
	}

	if ((*current_texture_ptr == hardware_format) && ((*current_block_index) == (*hardware_block_index)))
	{
		unsigned long hardware_address= lrar_block_address(lrar_cache,*hardware_block_index);

		if (GR_TMU0 == inTMU) {
			UUmAssert(globals_3dfx.hardware_address_tmu0 == hardware_address);
			UUmAssert(globals_3dfx.hardware_format_tmu0.smallLod == hardware_format->smallLod);
			UUmAssert(globals_3dfx.hardware_format_tmu0.largeLod == hardware_format->largeLod);
			UUmAssert(globals_3dfx.hardware_format_tmu0.aspectRatio == hardware_format->aspectRatio);
			UUmAssert(globals_3dfx.hardware_format_tmu0.format == hardware_format->format);
			UUmAssert(globals_3dfx.hardware_format_tmu0.data == hardware_format->data);
			UUmAssert(globals_3dfx.evenOdd_tmu0 == evenOdd);
		}
		else {
			UUmAssert(globals_3dfx.hardware_address_tmu1 == hardware_address);
			UUmAssert(globals_3dfx.hardware_format_tmu1.smallLod == hardware_format->smallLod);
			UUmAssert(globals_3dfx.hardware_format_tmu1.largeLod == hardware_format->largeLod);
			UUmAssert(globals_3dfx.hardware_format_tmu1.aspectRatio == hardware_format->aspectRatio);
			UUmAssert(globals_3dfx.hardware_format_tmu1.format == hardware_format->format);
			UUmAssert(globals_3dfx.hardware_format_tmu1.data == hardware_format->data);
			UUmAssert(globals_3dfx.evenOdd_tmu1 == evenOdd);
		}

		// grTexSource(inTMU, hardware_address, evenOdd, hardware_format);
	}
	else
	{
		long size = 0;
		UUtBool texture_changed= *hardware_block_dirty;
		GrTextureClampMode_t clampS = (inTexture->flags & MGcTextureFlag_ClampS) ? GR_TEXTURECLAMP_CLAMP : GR_TEXTURECLAMP_WRAP;
		GrTextureClampMode_t clampT = (inTexture->flags & MGcTextureFlag_ClampT) ? GR_TEXTURECLAMP_CLAMP : GR_TEXTURECLAMP_WRAP;

		// certain parameters are based on the texture loaded up to the card
		MGrSet_MipMapMode(inTMU, mipMapMode);
		MGrSet_TexClampMode(inTMU, clampS, clampT);

		// if we don't own a block on the card, get one
		if (NONE == *hardware_block_index)
		{
			size = grTexTextureMemRequired(evenOdd, hardware_format);
			*hardware_block_size = size;

			lrar_allocate(lrar_cache, size, inTexture->texture->debugName, hardware_block_index);
			texture_changed= UUcTrue;
		}
		else if (texture_changed)
		{
			size = grTexTextureMemRequired(evenOdd, hardware_format);
		}

		// if we own a block on the card, make us the current texture
		if (*hardware_block_index!=NONE)
		{
			unsigned long hardware_address= lrar_block_address(lrar_cache,*hardware_block_index);

			if (GR_TMU0 == inTMU) {
				globals_3dfx.hardware_address_tmu0 = hardware_address;
				globals_3dfx.hardware_format_tmu0 = *hardware_format;
				globals_3dfx.evenOdd_tmu0 = evenOdd; 
			}
			else {
				globals_3dfx.hardware_address_tmu1 = hardware_address;
				globals_3dfx.hardware_format_tmu1 = *hardware_format;
				globals_3dfx.evenOdd_tmu1 = evenOdd; 
			}

			if (texture_changed) {
				char *thisMap;
				UUtUns32 thisMapSize;

				// for compressed textures,
				// decompress the texture here
				if (inTexture->flags & MGcTextureFlag_Expansion)
				{
					IMtMipMap mipMap;

					if (inTexture->hardware_format.smallLod != inTexture->hardware_format.largeLod)
					{
						mipMap = IMcHasMipMap;
					}
					else
					{
						mipMap = IMcNoMipMap;
					}


					IMrImage_ConvertPixelType(
						IMcDitherMode_Off, 
						inTexture->width,
						inTexture->height,
						mipMap,
						inTexture->texture->texelType,
						inTexture->texture->pixels,
						IMcPixelType_RGB565,
						MGgDecompressBuffer);
				}

				myGRTexDownloadMipMap(inTMU, hardware_address, evenOdd, hardware_format, size);
				
				thisMap = hardware_format->data;
				thisMapSize = inTexture->width * inTexture->height * 2;

				*hardware_block_dirty = UUcFalse;
			}

			if (MGgMipMapping) 
			{
				grTexSource(inTMU, hardware_address, evenOdd, hardware_format);
			}
			else
			{
				GrTexInfo new_hardware_format = *hardware_format;

				new_hardware_format.smallLod = new_hardware_format.largeLod;

				grTexSource(inTMU, hardware_address, evenOdd, &new_hardware_format);
			}

			*current_texture_ptr = hardware_format;
			UUmAssert(*current_texture_ptr != NULL);

			*current_block_index = *hardware_block_index;
		}
	}

	return;
}

void MGrSet_MipMapMode(GrChipID_t inTMU, MGtMipMapMode inMode)
{
	MGtMipMapMode *currentMipMapMode;

	switch(inTMU)
	{
		case GR_TMU0: currentMipMapMode = &(globals_3dfx.currentMipMapMode_tmu0); break;
		case GR_TMU1: currentMipMapMode = &(globals_3dfx.currentMipMapMode_tmu1); break;
		default: UUmAssert(0); return;
	}

	if ((*currentMipMapMode) == inMode) {
		return;
	}

	*currentMipMapMode = inMode;

	switch(inMode)
	{
		case MGcMipMapMode_Invalid:
		break;

		case MGcMipMapMode_NoMipmapping:
			grTexMipMapMode(inTMU, GR_MIPMAP_DISABLE, FXFALSE);		
		break;

		case MGcMipMapMode_MipMapping:
			grTexMipMapMode(inTMU, GR_MIPMAP_NEAREST, FXFALSE);
		break;

		case MGcMipMapMode_Dither:
			grTexMipMapMode(inTMU, GR_MIPMAP_NEAREST_DITHER, FXFALSE);
		break;

		case MGcMipMapMode_Trilinear:
			grTexMipMapMode(inTMU, GR_MIPMAP_NEAREST, FXTRUE);
		break;

		default:
			UUmAssert(!"invalid default case");
		break;
	}

	return;
}


void MGrSet_AlphaCombine(MGtAlphaCombine inMode) 
{
	if (inMode == globals_3dfx.currentAlphaCombine) {
		return;
	}

	globals_3dfx.currentAlphaCombine = inMode;


	switch(inMode)
	{
		case MGcAlphaCombine_Anti_Alias:
			grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_ITERATED, FXFALSE);
			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ZERO, GR_BLEND_ZERO);
		break;

		case MGcAlphaCombine_NoAlpha:
			grAlphaCombine(GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_NONE, FXFALSE);
			grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ONE, GR_BLEND_ZERO);
		break;

		case MGcAlphaCombine_TextureAlpha:
			grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ZERO, GR_BLEND_ZERO);
		break;

		case MGcAlphaCombine_TextureTimesConstantAlpha:
			// alpha = f * alpha_other
			// alpha = (alpha.texture / 255) * constant alpha
			grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_TEXTURE_ALPHA, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_CONSTANT, FXFALSE);
			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ZERO, GR_BLEND_ZERO);
		break;

		case MGcAlphaCombine_ConstantAlpha:
			// alpha = local
			// alpha = constant_alpha
			grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_NONE, FXFALSE);
			grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ZERO, GR_BLEND_ZERO);
		break;

		case MGcAlphaCombine_Multipass:
			grAlphaCombine(GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_NONE, FXFALSE);
			grAlphaBlendFunction( GR_BLEND_DST_COLOR, GR_BLEND_ZERO, GR_BLEND_ONE, GR_BLEND_ZERO );
		break;

		case MGcAlphaCombine_Additive:
			grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_TEXTURE_ALPHA, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_CONSTANT, FXFALSE);
			grAlphaBlendFunction( GR_BLEND_SRC_ALPHA, GR_BLEND_ONE, GR_BLEND_ONE, GR_BLEND_ZERO );
		break;
	}
}

void MGrSet_ColorCombine(MGtColorCombine inMode)
{
	if (inMode == globals_3dfx.currentColorCombine) {
		return;
	}

	globals_3dfx.currentColorCombine = inMode;

	switch(inMode)
	{
		case MGcColorCombine_ConstantColor:
			// color = local
			// color = constant

			grColorCombine(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_NONE, FXFALSE);
		break;

		case MGcColorCombine_Gouraud:
			// color = f * OTHER
			// color = LOCAL/255 * OTHER
			// color = iterated/255 * texture

			grColorCombine(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_NONE, FXFALSE);
		break;

		case MGcColorCombine_TextureFlat:
			// color = f * OTHER
			// color = LOCAL/255 * OTHER
			// color = constant/255 * texture

			grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_CONSTANT, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
		break;

		case MGcColorCombine_TextureGouraud:
			// color = f * OTHER
			// color = LOCAL/255 * OTHER
			// color = iterated/255 * texture

			grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
		break;

		case MGcColorCombine_UnlitTexture:
			// color = f * OTHER
			// color = 1 * texture
			// color = texture

			grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE, GR_COMBINE_LOCAL_NONE, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
		break;
	}
}

void MGrSet_TextureCombine(MGtTextureCombine inMode)
{
	if (inMode == globals_3dfx.currentTextureCombine) {
		return;
	}
	
	if (1 == globals_3dfx.numTMU) {
		return;
	}

	globals_3dfx.currentTextureCombine = inMode;

	switch(inMode)
	{
		case MGcTextureCombine_Invalid:
		break;

		case MGcTextureCombine_None:
			grHints( GR_HINT_STWHINT, 0);
			grTexCombine(GR_TMU0, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);

			// turn off TMU1 if it is not needed, improves performance
           grTexCombine( GR_TMU1, GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_ZERO, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );
		break;

		case MGcTextureCombine_Multiply:
			grHints( GR_HINT_STWHINT, GR_STWHINT_ST_DIFF_TMU1);
			grTexCombine(GR_TMU0, GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, 
					GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, FXFALSE, FXFALSE);

			grTexCombine( GR_TMU1, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );
		break;

		case MGcTextureCombine_AlphaBlend:
			grHints( GR_HINT_STWHINT, GR_STWHINT_ST_DIFF_TMU1);
			//grHints( GR_HINT_STWHINT, 0);
			grTexCombine( GR_TMU0, GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL, GR_COMBINE_FACTOR_OTHER_ALPHA,
				GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );

			grTexCombine( GR_TMU1, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );
		break;

		case MGcTextureCombine_Trilinear:
			grHints( GR_HINT_STWHINT, 0);
			grTexCombine( GR_TMU0, GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL, GR_COMBINE_FACTOR_LOD_FRACTION,
				GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL, GR_COMBINE_FACTOR_LOD_FRACTION, FXFALSE, FXFALSE );

			grTexCombine( GR_TMU1, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );
		break;
		
		case MGcTextureCombine_EnvMap:
			grHints( GR_HINT_STWHINT, GR_STWHINT_ST_DIFF_TMU1);
			
			grTexCombine( GR_TMU0, GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL, GR_COMBINE_FACTOR_LOCAL_ALPHA,
				GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );

			grTexCombine( GR_TMU1, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE,
				GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE );
			break;
			
		default:
			UUmAssert(0);
		break;
	}

	return;
}

void MGrSet_TextureMode(
	M3tTextureMap*	inTexture)
{
	MGtTextureMapPrivate*	texturePrivate = TMrTemplate_PrivateData_GetDataPtr(MGgDrawEngine_TexureMap_PrivateData, inTexture);
	MGtAlphaCombine	textureAlpha;
	UUtBool			trilinear;
	UUtBool			envmapping;
	UUtBool			blend_additive;
	
	UUmAssertReadPtr(texturePrivate, sizeof(MGtTextureMapPrivate));

	MGgDrawContextPrivate->curBaseTexture = texturePrivate;
	
	textureAlpha =
				(texturePrivate->flags & MGcTextureFlag_HasAlpha) ?
					MGcAlphaCombine_TextureTimesConstantAlpha :
					MGcAlphaCombine_ConstantAlpha;
	#if 0
	trilinear =
				(2 == globals_3dfx.numTMU) &&
				(texturePrivate->flags & MGcTextureFlag_HasMipMap) &&
				((MGgFilteringOverrideMode == MGcFilteringOverride_Dont) ||
					(MGgFilteringOverrideMode == MGcFilteringOverride_Trilinear));					
	#endif
	
	trilinear = 0;
	
	envmapping = (inTexture->flags & M3cTextureFlags_ReceivesEnvMap) != 0;
	blend_additive = (inTexture->flags & M3cTextureFlags_Blend_Additive) != 0;
	
	if(blend_additive)
	{
		textureAlpha = MGcAlphaCombine_Additive;
	}
	else if(envmapping)
	{
		textureAlpha = MGcAlphaCombine_ConstantAlpha;
	}
	
	MGrSet_AlphaCombine(textureAlpha);
	
	if (trilinear)
	{
		MGrSet_TextureCombine(MGcTextureCombine_Trilinear);
		set_current_glide_texture(GR_TMU0, texturePrivate, MGcTextureUploadMode_Trilinear);
		set_current_glide_texture(GR_TMU1, texturePrivate, MGcTextureUploadMode_Trilinear);
	}
	else if(envmapping)
	{
		MGtTextureMapPrivate*	envMapPrivate = MGmGetEnvMapPrivate(MGgDrawContextPrivate);
		
//		UUmAssertReadPtr(envMapPrivate, sizeof(*envMapPrivate));
		if (envMapPrivate)
		{
			MGrSet_TextureCombine(MGcTextureCombine_EnvMap);
			set_current_glide_texture(GR_TMU0, texturePrivate, MGcTextureUploadMode_Normal);
			set_current_glide_texture(GR_TMU1, envMapPrivate, MGcTextureUploadMode_Normal);
		}
		else
		{
			MGrSet_TextureCombine(MGcTextureCombine_None);
			set_current_glide_texture(GR_TMU0, texturePrivate, MGcTextureUploadMode_Normal);
		}
	}
	else
	{
		MGrSet_TextureCombine(MGcTextureCombine_None);
		set_current_glide_texture(GR_TMU0, texturePrivate, MGcTextureUploadMode_Normal);
	}
	
}

void
MGrDrawPolygon(
	int				nverts,
	const int		ilist[],
	const GrVertex	vlist[])
{
	#if 0//defined(DEBUGGING) && DEBUGGING
	{
		int	i;
		
		for(i = 0; i < nverts; i++)
		{
			MGmAssertVertexXY(vlist + ilist[i]);
		}
	}
	#endif
	
	grDrawPolygon(nverts, ilist, vlist);
}

void
MGrDrawTriangle(
	const GrVertex* a,
	const GrVertex* b,
	const GrVertex* c)
{
	//MGmAssertVertexXY(a);
	//MGmAssertVertexXY(b);
	//MGmAssertVertexXY(c);

	grDrawTriangle(a, b, c);
}

void MGrSet_ZWrite(MGtZWriteMode inMode)
{
	if (inMode == globals_3dfx.currentZWriteMode) {
		return;
	}

	globals_3dfx.currentZWriteMode = inMode;

	switch(inMode)
	{
		case MGcZWrite_Invalid:
		break;

		case MGcZWrite_On:
			grDepthMask(FXTRUE);
		break;

		case MGcZWrite_Off:
			grDepthMask(FXFALSE);
		break;

		default:
			UUmAssert(!"invalid grDepthMask");
		break;
	}

	return;
}

