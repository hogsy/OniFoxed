/*
RASTERIZER_3DFX.H
*/
#ifndef __RASTERIZER_3DFX_H__
#define __RASTERIZER_3DFX_H__

#define DYNAHEADER
#include "glide.h"

//otherwise uses WBUFFER (need to scale z values for better dynamic range)
#define MGcUseZBuffer 0

typedef struct MGtTextureMapPrivate	MGtTextureMapPrivate;

typedef enum {
	MGcColorCombine_Invalid = 0x100,
	MGcColorCombine_ConstantColor = 0x200,
	MGcColorCombine_Gouraud = 0x300,
	MGcColorCombine_TextureFlat = 0x400,
	MGcColorCombine_TextureGouraud = 0x500,
	MGcColorCombine_UnlitTexture = 0x600
} MGtColorCombine;

typedef enum {
	MGcAlphaCombine_Invalid = 0x1000,
	MGcAlphaCombine_Anti_Alias = 0x2000,
	MGcAlphaCombine_NoAlpha = 0x3000,
	MGcAlphaCombine_TextureAlpha = 0x4000,
	MGcAlphaCombine_TextureTimesConstantAlpha = 0x5000,
	MGcAlphaCombine_ConstantAlpha = 0x6000,
	MGcAlphaCombine_Multipass = 0x7000,
	MGcAlphaCombine_Additive = 0x8000
} MGtAlphaCombine;

typedef enum {
	MGcTextureCombine_Invalid = 0x10000,
	MGcTextureCombine_None = 0x20000,
	MGcTextureCombine_Multiply = 0x30000,
	MGcTextureCombine_Trilinear = 0x40000,
	MGcTextureCombine_AlphaBlend = 0x80000,
	MGcTextureCombine_EnvMap = 0x100000
	
} MGtTextureCombine;

typedef enum {
	MGcMipMapMode_Invalid = 0x100000,
	MGcMipMapMode_NoMipmapping = 0x200000,
	MGcMipMapMode_MipMapping = 0x300000,
	MGcMipMapMode_Dither = 0x400000,
	MGcMipMapMode_Trilinear = 0x500000
} MGtMipMapMode;

typedef enum {
	MGcTextureUploadMode_Invalid = 0x1000000,
	MGcTextureUploadMode_Normal = 0x2000000,
	MGcTextureUploadMode_Trilinear = 0x3000000
} MGtTextureUploadMode;

typedef enum {
	MGcZWrite_Invalid = 0x10,
	MGcZWrite_On = 0x20,
	MGcZWrite_Off = 0x30
	
} MGtZWriteMode;


enum {
	MGcFilteringOverride_Dont = 0,
	MGcFilteringOverride_NoFiltering = 1,
	MGcFilteringOverride_Bilinear = 2,
	MGcFilteringOverride_Dither = 3,
	MGcFilteringOverride_Trilinear = 4
};

enum {
	MGcTextureFlag_HasAlpha			= 0x01,
	MGcTextureFlag_MipMapDither		= 0x02,
	MGcTextureFlag_ClampS			= 0x04,
	MGcTextureFlag_ClampT			= 0x08,
	MGcTextureFlag_HasMipMap		= 0x10,
	MGcTextureFlag_Expansion		= 0x20
};


typedef enum
{
	MGcTextureInvalid,
	MGcTextureValid
} MGtTextureValid;

typedef enum
{
	MGcTextureAlpha_Yes,
	MGcTextureAlpha_No
} MGcTextureAlpha;

typedef enum
{
	MGcTextureExpansion_No,
	MGcTextureExpansion_Yes
} MGcTextureExpansion;
#include "MG_DC_Private.h"

typedef struct MGtTexelTypeInfo
{
	IMtPixelType		texelType;
	MGtTextureValid		textureValid;
	MGcTextureExpansion	textureExpansion;
	MGcTextureAlpha		hasAlpha;
	GrTextureFormat_t	grTextureFormat;
} MGtTexelTypeInfo;

struct MGtTextureMapPrivate
{
	float u_scale, v_scale;

	M3tTextureMap *texture;
	GrTexInfo hardware_format; // glide texture format

	// block indicies in the lrar_cache on the card
	short hardware_block_index_tmu0;	
	short hardware_block_index_tmu0_even;
	short hardware_block_index_tmu1;
	short hardware_block_index_tmu1_odd;

	long hardware_block_size_tmu0;
	long hardware_block_size_tmu0_even;
	long hardware_block_size_tmu1;
	long hardware_block_size_tmu1_odd;

	UUtBool hardware_block_dirty_tmu0;	
	UUtBool hardware_block_dirty_tmu0_even;
	UUtBool hardware_block_dirty_tmu1;
	UUtBool hardware_block_dirty_tmu1_odd;

	short flags;					
	short width, height;
};

typedef struct 
{
	GrTexInfo *current_texture_tmu0;
	GrTexInfo *current_texture_tmu1;
	short current_blockindex_tmu0;
	short current_blockindex_tmu1;
	UUtUns16 numTMU;
	struct lrar_cache *texture_memory_cache_tmu0;
	struct lrar_cache *texture_memory_cache_tmu1;
	MGtTextureCombine	currentTextureCombine;
	MGtColorCombine		currentColorCombine;
	MGtAlphaCombine		currentAlphaCombine;
	MGtZWriteMode		currentZWriteMode;
	MGtMipMapMode		currentMipMapMode_tmu0;
	MGtMipMapMode		currentMipMapMode_tmu1;
	GrTextureClampMode_t clampS_tmu0;
	GrTextureClampMode_t clampT_tmu0;
	GrTextureClampMode_t clampS_tmu1;
	GrTextureClampMode_t clampT_tmu1;
	unsigned long		hardware_address_tmu0;
	GrTexInfo			hardware_format_tmu0;
	FxU32				evenOdd_tmu0;
	unsigned long		hardware_address_tmu1;
	GrTexInfo			hardware_format_tmu1;
	FxU32				evenOdd_tmu1;
} tGlobals_3dfx;

extern tGlobals_3dfx globals_3dfx;
extern UUtUns32 MGgSlowLightmaps;
extern UUtUns32 MGgFilteringOverrideMode;
extern void *MGgDecompressBuffer;
extern UUtBool MGgDoubleBuffer;
extern UUtBool MGgBufferClear;


/* ---------- prototypes/RASTERIZER_3DFX.C */

UUtBool available_3dfx(void);
short enumerate_rasterizer_parameters_3dfx(struct rasterizer_parameters *parameters);

void
initialize_3dfx(
	GrScreenResolution_t	inScreenResolution);

void terminate_3dfx(void);

void dispose_3dfx(void);

void start_rasterizing_3dfx(void);
void stop_rasterizing_3dfx(void);

void erase_backbuffer_3dfx(void);
void lock_backbuffer_3dfx(void);
void unlock_backbuffer_3dfx(void);
void display_backbuffer_3dfx(void);

void draw_surface_3dfx(struct view_data *view, struct render_surface *surface);

void set_current_glide_texture(GrChipID_t inTMU, MGtTextureMapPrivate *texture, MGtTextureUploadMode mode);

// get info about a texture format
const MGtTexelTypeInfo *MGrTexelType_GetInfo(IMtPixelType inTexelType);

// setup glide
void MGrSet_ZWrite(MGtZWriteMode inMode);
void MGrSet_ColorCombine(MGtColorCombine inMode);
void MGrSet_AlphaCombine(MGtAlphaCombine inMode);
void MGrSet_TextureCombine(MGtTextureCombine inMode);
void MGrSet_MipMapMode(GrChipID_t tmu, MGtMipMapMode inMode);
void MGrSet_TexClampMode(GrChipID_t tmu, GrTextureClampMode_t clampS, GrTextureClampMode_t clampT);

void MGrSet_TextureMode(
	M3tTextureMap*	inTexture);

void MGrDrawPolygon( int nverts, const int ilist[], const GrVertex vlist[] );

void MGrDrawTriangle(const GrVertex* a, const GrVertex* b, const GrVertex* c); 

/* ---------- prototypes/RASTERIZER_3DFX_WINDOWS.C */

UUtBool load_glide_procedures(void);

#endif