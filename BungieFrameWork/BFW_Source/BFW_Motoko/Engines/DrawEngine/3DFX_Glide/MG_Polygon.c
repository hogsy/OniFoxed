/*
	FILE:	MG_Polygon.c
	
	AUTHOR:	Michael Evans
	
	CREATED: July 5, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/
#if 0
#include "MG_Polygon.h"

#include "BFW.h"
#include "BFW_Motoko.h"

#include "Motoko_Manager.h"

#include "BFW_Shared_TriRaster.h"

#include "MG_DC_Private.h"
#include "MG_DC_Method_Triangle.h"
#include "MG_DC_Method_Quad.h"
#include "rasterizer_3dfx.h"

void MGrDrawPolygon_VertexList_TextureUnlit(int nverts, const GrVertex vlist[], MGtTextureMapPrivate *inTexture)
{

	grDrawPolygonVertexList(nverts, vlist);
}

void MGrDrawPolygon_VertexList_TextureInterpolate(int nverts, const GrVertex vlist[], MGtTextureMapPrivate *inTexture)
{
	MGtAlphaCombine	textureAlpha = (inTexture->flags & MGcTextureFlag_HasAlpha) ? MGcAlphaCombine_TextureAlpha : MGcAlphaCombine_NoAlpha;
	UUtBool trilinear = (2 == globals_3dfx.numTMU) && (inTexture->flags & MGcTextureFlag_HasMipMap) && ((MGgFilteringOverrideMode == MGcFilteringOverride_Dont) || (MGgFilteringOverrideMode == MGcFilteringOverride_Trilinear));

	MGrSet_ColorCombine(MGcColorCombine_TextureGouraud);
	MGrSet_AlphaCombine(textureAlpha);

	if (trilinear) {
		MGrSet_TextureCombine(MGcTextureCombine_Trilinear);
		set_current_glide_texture(GR_TMU0, inTexture, MGcTextureUploadMode_Trilinear);
		set_current_glide_texture(GR_TMU1, inTexture, MGcTextureUploadMode_Trilinear);
	}
	else {
		MGrSet_TextureCombine(MGcTextureCombine_None);
		set_current_glide_texture(GR_TMU0, inTexture, MGcTextureUploadMode_Normal);
	}

	grDrawPolygonVertexList(nverts, vlist);
}

void MGrDrawPolygon_VertexList_TextureFlat(int nverts, const GrVertex vlist[], MGtTextureMapPrivate *inTexture)
{
	MGtAlphaCombine	textureAlpha = (inTexture->flags & MGcTextureFlag_HasAlpha) ? MGcAlphaCombine_TextureAlpha : MGcAlphaCombine_NoAlpha;
	UUtBool trilinear = (2 == globals_3dfx.numTMU) && (inTexture->flags & MGcTextureFlag_HasMipMap) && ((MGgFilteringOverrideMode == MGcFilteringOverride_Dont) || (MGgFilteringOverrideMode == MGcFilteringOverride_Trilinear));

	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);
	MGrSet_AlphaCombine(textureAlpha);

	if (trilinear) {
		MGrSet_TextureCombine(MGcTextureCombine_Trilinear);
		set_current_glide_texture(GR_TMU0, inTexture, MGcTextureUploadMode_Trilinear);
		set_current_glide_texture(GR_TMU1, inTexture, MGcTextureUploadMode_Trilinear);
	}
	else {
		MGrSet_TextureCombine(MGcTextureCombine_None);
		set_current_glide_texture(GR_TMU0, inTexture, MGcTextureUploadMode_Normal);
	}

	grDrawPolygonVertexList(nverts, vlist);
}

void MGrDrawTriangle_TextureInterpolate(
	const GrVertex*			inVertex0,
	const GrVertex*			inVertex1,
	const GrVertex*			inVertex2,
	MGtTextureMapPrivate*	inTexture)
{
	MGtAlphaCombine	textureAlpha = (inTexture->flags & MGcTextureFlag_HasAlpha) ? MGcAlphaCombine_TextureAlpha : MGcAlphaCombine_NoAlpha;
	UUtBool trilinear = (2 == globals_3dfx.numTMU) && (inTexture->flags & MGcTextureFlag_HasMipMap) && ((MGgFilteringOverrideMode == MGcFilteringOverride_Dont) || (MGgFilteringOverrideMode == MGcFilteringOverride_Trilinear));

	MGrSet_ColorCombine(MGcColorCombine_TextureGouraud);
	MGrSet_AlphaCombine(textureAlpha);

	if (trilinear) {
		MGrSet_TextureCombine(MGcTextureCombine_Trilinear);
		set_current_glide_texture(GR_TMU0, inTexture, MGcTextureUploadMode_Trilinear);
		set_current_glide_texture(GR_TMU1, inTexture, MGcTextureUploadMode_Trilinear);
	}
	else {
		MGrSet_TextureCombine(MGcTextureCombine_None);
		set_current_glide_texture(GR_TMU0, inTexture, MGcTextureUploadMode_Normal);
	}

	grDrawTriangle(inVertex0, inVertex1, inVertex2);
}

#endif
