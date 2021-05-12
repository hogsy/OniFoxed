/*
	FILE:	MG_DrawContext_Private.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 17, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MG_DRAWCONTEXT_PRIVATE_H
#define MG_DRAWCONTEXT_PRIVATE_H

#include "BFW.h"
#include "BFW_Motoko.h"


#define DYNAHEADER
#include "glide.h"
#include "lrar_cache.h"

typedef struct MGtDrawContextPrivate MGtDrawContextPrivate;

#include "rasterizer_3dfx.h"

#define MGcMaxElements	(128 * 1024)

#define MGcMaxAlpha 255.f

typedef enum MGtDrawState_TMU
{
	MGcDrawState_TMU_1,
	MGcDrawState_TMU_2,
	
	MGcDrawState_TMU_Num
	
} MGtDrawState_TMU;

// 3dfx tells you to do this; check manual for details (don't take it out)
#define SNAP_COORD(x) (((float)(x)) + (float)(3<<18))

#define MGc255Float (255.f)
#define MGcMax5Bits (31)
extern float MGg5BitsTo0_255FloatTable[32];

#if defined DEBUGGING && DEBUGGING

#define MGmAssertVertexXY(glide_vertex) \
	UUmAssert((glide_vertex)->x - (float)(3<<18) >= 0.0f);	\
	UUmAssert((glide_vertex)->y - (float)(3<<18) >= 0.0f);	\
	UUmAssert((glide_vertex)->x - (float)(3<<18) < (float)MGgDrawContextPrivate->width);	\
	UUmAssert((glide_vertex)->y - (float)(3<<18) < (float)MGgDrawContextPrivate->height);	\
	UUmBlankFunction

#define MGmAssertVertex_RGB(glide_vertex) \
	UUmAssert((glide_vertex)->r >= 0.f);	\
	UUmAssert((glide_vertex)->g >= 0.f);	\
	UUmAssert((glide_vertex)->b >= 0.f);	\
	UUmAssert((glide_vertex)->r <= 255.f);	\
	UUmAssert((glide_vertex)->g <= 255.f);	\
	UUmAssert((glide_vertex)->b <= 255.f);	\
	UUmBlankFunction

#else

#define MGmAssertVertexXY(glide_vertex)
#define MGmAssertVertex_RGB(glide_vertex)

#endif

#if MGcUseZBuffer

#define MGmConvertVertex_XYZ(m_vertex, glide_vertex)	\
	(glide_vertex)->x= SNAP_COORD((m_vertex)->x);	\
	(glide_vertex)->y= SNAP_COORD((m_vertex)->y);	\
	(glide_vertex)->ooz= 65535.f / (m_vertex)->z;	\
	(glide_vertex)->oow= (m_vertex)->invW;			\
	UUmBlankFunction 

#else

#define MGmConvertVertex_XYZ(m_vertex, glide_vertex)	\
	(glide_vertex)->x= SNAP_COORD((m_vertex)->x);	\
	(glide_vertex)->y= SNAP_COORD((m_vertex)->y);	\
	(glide_vertex)->oow= (m_vertex)->invW;			\
	UUmBlankFunction 

#endif

#if 0
#define MGmConvertVertex_RGB(m_color, glide_vertex)	\
	(glide_vertex)->a = 0.f; \
	(glide_vertex)->r = UUmPin((255.f / ((float) 0x1f) * (((m_color) & (0x1f << 10)) >> 10)), 0.f, 255.f); \
	(glide_vertex)->g = UUmPin((255.f / ((float) 0x1f) * (((m_color) & (0x1f <<  5)) >>  5)), 0.f, 255.f); \
	(glide_vertex)->b = UUmPin((255.f / ((float) 0x1f) * (((m_color) & (0x1f <<  0)) >>  0)), 0.f, 255.f); \
	UUmBlankFunction 
#else
#define MGmConvertVertex_RGB(m_color, glide_vertex)	\
	(glide_vertex)->a = 0.0f; \
	(glide_vertex)->r = (float)(((m_color) >> 16) & 0xFF); \
	(glide_vertex)->g = (float)(((m_color) >> 8) & 0xFF); \
	(glide_vertex)->b = (float)(((m_color) >> 0) & 0xFF); \
	UUmBlankFunction 
#endif

#define MGmConvertVertex_UV(u_scale, v_scale, glide_tmu, m_texture_coord, glide_vertex)	\
	(glide_vertex)->tmuvtx[glide_tmu].sow= (u_scale*(m_texture_coord)->u)*(glide_vertex)->oow;	\
	(glide_vertex)->tmuvtx[glide_tmu].tow= (v_scale*(m_texture_coord)->v)*(glide_vertex)->oow;	\
	UUmBlankFunction 

#define MGmConvertVertex_UV_LM(u_scale_base, v_scale_base, u_scale_light, v_scale_light, m_base_texture_coord, m_light_texture_coord, glide_vertex)	\
	MGmConvertVertex_UV(u_scale_base, v_scale_base, 0, m_base_texture_coord, glide_vertex); \
	MGmConvertVertex_UV(u_scale_light, v_scale_light, 1, m_light_texture_coord, glide_vertex); \
	
	
#define MGmConvertVertex_XYZUV(m_vertex, u_scale_base, v_scale_base, m_texture_coord, glide_vertex)	\
	MGmConvertVertex_XYZ(m_vertex, glide_vertex); \
	MGmConvertVertex_UV(u_scale_base, v_scale_base, 0, m_texture_coord, glide_vertex);	\
	UUmBlankFunction 

#define MGmConvertVertex_XYZUVRGB(m_vertex, u_scale_base, v_scale_base, m_texture_coord, m_color, glide_vertex)	\
	MGmConvertVertex_XYZUV(m_vertex, u_scale_base, v_scale_base, m_texture_coord, glide_vertex); \
	MGmConvertVertex_RGB(m_color, glide_vertex); \
	UUmBlankFunction 

#define MGmConvertVertex_XYZUVRGBEnvMap(m_vertex, u_scale_base, v_scale_base, m_texture_coord, u_scale_env, v_scale_env, m_env_texture_coord, m_color, glide_vertex)	\
	MGmConvertVertex_XYZUV(m_vertex, u_scale_base, v_scale_base, m_texture_coord, glide_vertex); \
	MGmConvertVertex_UV(u_scale_env, v_scale_env, 1, m_env_texture_coord, glide_vertex);	\
	MGmConvertVertex_RGB(m_color, glide_vertex); \
	UUmBlankFunction 

#define MGmConvertVertex_XYZRGB(m_vertex, m_color, glide_vertex) \
	MGmConvertVertex_XYZ(m_vertex, glide_vertex); \
	MGmConvertVertex_RGB(m_color, glide_vertex); \
	UUmBlankFunction 

#define	MGmSplatVertex_XYZ(index) \
	if(index >= numRealVertices)	\
	{																		\
		MGmConvertVertex_XYZ(												\
			screenPoints + index,											\
			vertexList + index);											\
	}																		\
	UUmBlankFunction
			
#define	MGmSplatVertex_XYZRGB(index) \
	if(index >= numRealVertices)	\
	{																		\
		MGmConvertVertex_XYZRGB(											\
			screenPoints + index,											\
			vertexShades[index],											\
			vertexList + index);											\
	}																		\
	UUmBlankFunction
			
#define	MGmSplatVertex_XYZUV(index) \
	if(index >= numRealVertices)	\
	{																		\
		MGmConvertVertex_XYZUV(												\
			screenPoints + index,											\
			u_scale,														\
			v_scale,														\
			textureCoords + index,											\
			vertexList + index);											\
	}																		\
	UUmBlankFunction

#define	MGmSplatVertex_XYZUVRGB(index) \
	if(index >= numRealVertices)	\
	{																		\
		MGmConvertVertex_XYZUVRGB(											\
			screenPoints + index,											\
			u_scale,														\
			v_scale,														\
			textureCoords + index,											\
			vertexShades[index],											\
			vertexList + index);											\
	}																		\
	UUmBlankFunction
			
#define	MGmSplatVertex_XYZUVRGBEnvMap(index) 								\
	if(index >= numRealVertices)											\
	{																		\
		MGmConvertVertex_XYZUVRGBEnvMap(									\
			screenPoints + index,											\
			u_scale,														\
			v_scale,														\
			textureCoords + index,											\
			u_scale_env,													\
			v_scale_env,													\
			envTextureCoords + index,										\
			vertexShades[index],											\
			vertexList + index);											\
	}																		\
	UUmBlankFunction
			


#define MGmGetScreenPoints(MGgDrawContextPrivate) ((M3tPointScreen*) ((MGgDrawContextPrivate)->statePtr[M3cDrawStatePtrType_ScreenPointArray]))
#define MGmGetVertexShades(MGgDrawContextPrivate) ((UUtUns32*) ((MGgDrawContextPrivate)->statePtr[M3cDrawStatePtrType_ScreenShadeArray_DC]))
#define MGmGetTextureCoords(MGgDrawContextPrivate) ((M3tTextureCoord*) ((MGgDrawContextPrivate)->statePtr[M3cDrawStatePtrType_TextureCoordArray]))
#define MGmGetEnvTextureCoords(MGgDrawContextPrivate) ((M3tTextureCoord*) ((MGgDrawContextPrivate)->statePtr[M3cDrawStatePtrType_EnvTextureCoordArray]))
#define MGmGetBaseMap(MGgDrawContextPrivate) ((MGgDrawContextPrivate)->statePtr[M3cDrawStatePtrType_BaseTextureMap])
#define MGmGetBaseMapPrivate(MGgDrawContextPrivate) ((MGtTextureMapPrivate*)(TMrTemplate_PrivateData_GetDataPtr(MGgDrawEngine_TexureMap_PrivateData, (MGgDrawContextPrivate)->statePtr[M3cDrawStatePtrType_BaseTextureMap])))
#define MGmGetEnvMapPrivate(MGgDrawContextPrivate) ((MGtTextureMapPrivate*)(TMrTemplate_PrivateData_GetDataPtr(MGgDrawEngine_TexureMap_PrivateData, (MGgDrawContextPrivate)->statePtr[M3cDrawStatePtrType_EnvTextureMap])))
#define MGmGetLightMap(MGgDrawContextPrivate) ((MGgDrawContextPrivate)->statePtr[M3cDrawStatePtrType_LightTextureMap])
#define MGmGetLightMapPrivate(MGgDrawContextPrivate) ((MGtTextureMapPrivate*)(TMrTemplate_PrivateData_GetDataPtr(MGgDrawEngine_TexureMap_PrivateData, (MGgDrawContextPrivate)->statePtr[M3cDrawStatePtrType_LightTextureMap])))

typedef void
(*MGtModeFunction)(
	void);

typedef void
(*MGtVertexCreateFunction)(
	void);

typedef void
(*MGtDrawTri)(
	M3tTri*	inTri);

typedef void
(*MGtDrawTriSplit)(
	M3tTriSplit*	inTri);

typedef void
(*MGtDrawQuad)(
	M3tQuad*	inQuad);

typedef void
(*MGtDrawQuadSplit)(
	M3tQuadSplit*	inQuad);

typedef void
(*MGtDrawPent)(
	M3tPent*	inPent);

typedef void
(*MGtDrawPentSplit)(
	M3tPentSplit*	inPent);

#if 0
enum
{
	GLIDE_SCREEN_WIDTH= MScScreenWidth,
	GLIDE_SCREEN_HEIGHT= MScScreenHeight,
	GLIDE_REFRESH= 67
};
#endif

typedef struct MGtStatePrivate
{
	GrVertex*	vertexList;
	UUtUns32*	vertexListBV;
	
} MGtStatePrivate;

struct MGtDrawContextPrivate
{
	long						width;
	long						height;
	
	M3tDrawContextType			contextType;
	
	GrVertex*					vertexList;
	//UUtUns32*					vertexListBV;
	
	const MGtTextureMapPrivate*		curBaseTexture;
	const UUtInt32*					stateInt;
	const void**					statePtr;
	
	UUtBool							clipping;
	
	//MGtTextureMapPrivate*		lastTexture;
};

extern MGtDrawContextPrivate*	MGgDrawContextPrivate;

extern UUtUns32				MGgFrameBufferBytes;
extern UUtInt32				MGgTextureBytesDownloaded;
extern TMtPrivateData*		MGgDrawEngine_TexureMap_PrivateData;
extern float MGgGamma;
extern UUtUns32 MGgBilinear;
extern UUtBool	MGgMipMapping;


#endif /* MG_DRAWCONTEXT_PRIVATE_H */
