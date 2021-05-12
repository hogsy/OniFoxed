/*
	FILE:	MG_DC_Method_Quad.c
	
	AUTHOR:	Brent H. Pease, Michael Evans
	
	CREATED: Sept 18, 1997
	
	PURPOSE: 
	
	Copyright 1997, 1998

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_BitVector.h"

#include "Motoko_Manager.h"

#include "BFW_Shared_TriRaster.h"

#include "MG_DC_Private.h"
#include "MG_DC_Method_Triangle.h"
#include "MG_DC_Method_State.h"
#include "MG_DC_Method_Quad.h"
#include "rasterizer_3dfx.h"

void 
MGrQuad_Point(
	M3tQuad*		inQuad)
{
	M3tPointScreen*	screenPoints = MGmGetScreenPoints(MGgDrawContextPrivate);
	UUtUns32*		vertexShades = MGmGetVertexShades(MGgDrawContextPrivate);
	GrVertex*		vertexList;
	
	vertexList = MGgDrawContextPrivate->vertexList;
	UUmAssert(vertexList != NULL);

	// Someday move this up
	MGrSet_ColorCombine(MGcColorCombine_ConstantColor);
	MGrSet_AlphaCombine(MGcAlphaCombine_NoAlpha);

	vertexList[inQuad->indices[0]].oow *= 1.01f;
	grDrawPoint(vertexList + inQuad->indices[0]);

	vertexList[inQuad->indices[1]].oow *= 1.01f;
	grDrawPoint(vertexList + inQuad->indices[1]);

	vertexList[inQuad->indices[2]].oow *= 1.01f;
	grDrawPoint(vertexList + inQuad->indices[2]);

	vertexList[inQuad->indices[3]].oow *= 1.01f;
	grDrawPoint(vertexList + inQuad->indices[3]);
}

void
MGrQuad_Line_InterpNone(
	M3tQuad*		inQuad)
{
	M3tPointScreen*	screenPoints = MGmGetScreenPoints(MGgDrawContextPrivate);
	UUtUns32*		vertexShades = MGmGetVertexShades(MGgDrawContextPrivate);
	GrVertex*		vertexList;
	
	UUtUns16		vertexItr;
	
	vertexList = MGgDrawContextPrivate->vertexList;
	UUmAssert(vertexList != NULL);

	// Someday move this up
	MGrSet_ColorCombine(MGcColorCombine_ConstantColor);
	MGrSet_AlphaCombine(MGcAlphaCombine_NoAlpha);

	for(vertexItr = 0; vertexItr < 4; vertexItr++)
	{
		vertexList[inQuad->indices[vertexItr]].oow *= 1.01f;
		vertexList[inQuad->indices[(vertexItr + 1) % 4]].oow *= 1.01f;
		grDrawLine(vertexList + inQuad->indices[vertexItr], vertexList + inQuad->indices[(vertexItr + 1) % 4]);
	}
}

void
MGrQuad_Line_InterpVertex(
	M3tQuad*		inQuad)
{
	MGrQuad_Line_InterpNone(inQuad);
}

void
MGrQuad_Solid_Gouraud_InterpNone(
	M3tQuad*		inQuad)
{
	GrVertex*		vertexList;
	int				iList[4];

	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Interpolation] ==
				M3cDrawState_Interpolation_None);
	
	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Appearence] ==
				M3cDrawState_Appearence_Gouraud);
	
	vertexList = MGgDrawContextPrivate->vertexList;
	UUmAssert(vertexList != NULL);
	
	if(MGgDrawContextPrivate->clipping)
	{
		M3tPointScreen*	screenPoints = MGmGetScreenPoints(MGgDrawContextPrivate);
		UUtUns16		numRealVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
		UUmAssert(numRealVertices < MGcMaxElements);
		
		MGmSplatVertex_XYZ(inQuad->indices[0]);
		MGmSplatVertex_XYZ(inQuad->indices[1]);
		MGmSplatVertex_XYZ(inQuad->indices[2]);
		MGmSplatVertex_XYZ(inQuad->indices[3]);
	}
	
	iList[0] = inQuad->indices[0];
	iList[1] = inQuad->indices[1];
	iList[2] = inQuad->indices[2];
	iList[3] = inQuad->indices[3];

	MGrDrawPolygon(
		4,
		iList,
		vertexList);

}

void
MGrQuad_Solid_Gouraud_InterpVertex(
	M3tQuad*		inQuad)
{
	GrVertex*		vertexList;
	int				iList[4];
	
	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Interpolation] ==
				M3cDrawState_Interpolation_Vertex);
	
	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Appearence] ==
				M3cDrawState_Appearence_Gouraud);

	vertexList = MGgDrawContextPrivate->vertexList;
	UUmAssert(vertexList != NULL);
	
	if(MGgDrawContextPrivate->clipping)
	{
		UUtUns16		numRealVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
		M3tPointScreen*	screenPoints = MGmGetScreenPoints(MGgDrawContextPrivate);
		UUtUns32*		vertexShades = MGmGetVertexShades(MGgDrawContextPrivate);

		UUmAssert(numRealVertices < MGcMaxElements);
		
		MGmSplatVertex_XYZRGB(inQuad->indices[0]);
		MGmSplatVertex_XYZRGB(inQuad->indices[1]);
		MGmSplatVertex_XYZRGB(inQuad->indices[2]);
		MGmSplatVertex_XYZRGB(inQuad->indices[3]);
	}
			
	iList[0] = inQuad->indices[0];
	iList[1] = inQuad->indices[1];
	iList[2] = inQuad->indices[2];
	iList[3] = inQuad->indices[3];

	MGrDrawPolygon(
		4,
		iList,
		vertexList);
}

void
MGrQuad_Solid_TextureLit_InterpNone(
	M3tQuad*		inQuad)
{
	GrVertex*		vertexList;
	int				iList[4];
	
	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Interpolation] ==
				M3cDrawState_Interpolation_None);
	
	//UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Appearence] ==
	//			M3cDrawState_Appearence_Texture_Lit);
	
	vertexList = MGgDrawContextPrivate->vertexList;
	UUmAssert(vertexList != NULL);
	
	if(MGgDrawContextPrivate->clipping)
	{
		UUtUns16				numRealVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
		M3tTextureCoord*		textureCoords = MGmGetTextureCoords(MGgDrawContextPrivate);
		M3tPointScreen*			screenPoints = MGmGetScreenPoints(MGgDrawContextPrivate);
		MGtTextureMapPrivate*	baseMapPrivate = MGmGetBaseMapPrivate(MGgDrawContextPrivate);
		float					u_scale, v_scale;
		
		UUmAssert(numRealVertices < MGcMaxElements);
		UUmAssertReadPtr(baseMapPrivate, sizeof(*baseMapPrivate));
		
		u_scale = baseMapPrivate->u_scale;
		v_scale = baseMapPrivate->v_scale;
		
		MGmSplatVertex_XYZUV(inQuad->indices[0]);
		MGmSplatVertex_XYZUV(inQuad->indices[1]);
		MGmSplatVertex_XYZUV(inQuad->indices[2]);
		MGmSplatVertex_XYZUV(inQuad->indices[3]);
	}
	
	iList[0] = inQuad->indices[0];
	iList[1] = inQuad->indices[1];
	iList[2] = inQuad->indices[2];
	iList[3] = inQuad->indices[3];

	MGrDrawPolygon(
		4,
		iList,
		vertexList);
}

void
MGrQuad_Solid_TextureLit_InterpVertex(
	M3tQuad*		inQuad)
{
	GrVertex*		vertexList;
	int				iList[4];
	
	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Interpolation] ==
				M3cDrawState_Interpolation_Vertex);
	
	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Appearence] ==
				M3cDrawState_Appearence_Texture_Lit);
	
	UUmAssert(globals_3dfx.currentColorCombine == MGcColorCombine_TextureGouraud);
	
	vertexList = MGgDrawContextPrivate->vertexList;
	UUmAssert(vertexList != NULL);
	
	if(MGgDrawContextPrivate->clipping)
	{
		M3tPointScreen*			screenPoints = MGmGetScreenPoints(MGgDrawContextPrivate);
		UUtUns32*				vertexShades = MGmGetVertexShades(MGgDrawContextPrivate);
		MGtTextureMapPrivate*	baseMapPrivate = MGmGetBaseMapPrivate(MGgDrawContextPrivate);
		M3tTextureCoord*		textureCoords = MGmGetTextureCoords(MGgDrawContextPrivate);
		UUtUns16				numRealVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
		float					u_scale, v_scale;

		UUmAssertReadPtr(baseMapPrivate, sizeof(*baseMapPrivate));
		UUmAssert(numRealVertices < MGcMaxElements);
		
		u_scale = baseMapPrivate->u_scale;
		v_scale = baseMapPrivate->v_scale;
		
		MGmSplatVertex_XYZUVRGB(inQuad->indices[0]);
		MGmSplatVertex_XYZUVRGB(inQuad->indices[1]);
		MGmSplatVertex_XYZUVRGB(inQuad->indices[2]);
		MGmSplatVertex_XYZUVRGB(inQuad->indices[3]);
	}
	
	MGmAssertVertex_RGB(vertexList + inQuad->indices[0]);
	MGmAssertVertex_RGB(vertexList + inQuad->indices[1]);
	MGmAssertVertex_RGB(vertexList + inQuad->indices[2]);
	MGmAssertVertex_RGB(vertexList + inQuad->indices[3]);

	iList[0] = inQuad->indices[0];
	iList[1] = inQuad->indices[1];
	iList[2] = inQuad->indices[2];
	iList[3] = inQuad->indices[3];

	MGrDrawPolygon(
		4,
		iList,
		vertexList);
}

void
MGrQuad_Solid_TextureLit_EnvMap_InterpVertex(
	M3tQuad*		inQuad)
{
	GrVertex*		vertexList;
	int				iList[4];
	
	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Interpolation] ==
				M3cDrawState_Interpolation_Vertex);
	
	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Appearence] ==
				M3cDrawState_Appearence_Texture_Lit_EnvMap);
	
	UUmAssert(globals_3dfx.currentColorCombine == MGcColorCombine_TextureGouraud);
	
	vertexList = MGgDrawContextPrivate->vertexList;
	UUmAssert(vertexList != NULL);
	
	if(MGgDrawContextPrivate->clipping)
	{
		M3tPointScreen*			screenPoints = MGmGetScreenPoints(MGgDrawContextPrivate);
		UUtUns32*				vertexShades = MGmGetVertexShades(MGgDrawContextPrivate);
		MGtTextureMapPrivate*	baseMapPrivate = MGmGetBaseMapPrivate(MGgDrawContextPrivate);
		MGtTextureMapPrivate*	envMapPrivate = MGmGetEnvMapPrivate(MGgDrawContextPrivate);
		M3tTextureCoord*		textureCoords = MGmGetTextureCoords(MGgDrawContextPrivate);
		M3tTextureCoord*		envTextureCoords = MGmGetEnvTextureCoords(MGgDrawContextPrivate);
		UUtUns16				numRealVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
		float					u_scale, v_scale;
		float					u_scale_env, v_scale_env;

		UUmAssertReadPtr(baseMapPrivate, sizeof(*baseMapPrivate));
		UUmAssert(numRealVertices < MGcMaxElements);
		
		u_scale = baseMapPrivate->u_scale;
		v_scale = baseMapPrivate->v_scale;
		u_scale_env = envMapPrivate->u_scale;
		v_scale_env = envMapPrivate->v_scale;
		
		MGmSplatVertex_XYZUVRGBEnvMap(inQuad->indices[0]);
		MGmSplatVertex_XYZUVRGBEnvMap(inQuad->indices[1]);
		MGmSplatVertex_XYZUVRGBEnvMap(inQuad->indices[2]);
		MGmSplatVertex_XYZUVRGBEnvMap(inQuad->indices[3]);
	}
	
	MGmAssertVertex_RGB(vertexList + inQuad->indices[0]);
	MGmAssertVertex_RGB(vertexList + inQuad->indices[1]);
	MGmAssertVertex_RGB(vertexList + inQuad->indices[2]);
	MGmAssertVertex_RGB(vertexList + inQuad->indices[3]);

	iList[0] = inQuad->indices[0];
	iList[1] = inQuad->indices[1];
	iList[2] = inQuad->indices[2];
	iList[3] = inQuad->indices[3];

	MGrDrawPolygon(
		4,
		iList,
		vertexList);
}

void
MGrQuad_Solid_TextureUnlit(
	M3tQuad*		inQuad)
{
	MGrQuad_Solid_TextureLit_InterpNone(inQuad);
}

// Trisplit functions

void
MGrQuadSplit_Solid_TextureLit_InterpNone_LMOff(
	M3tQuadSplit*		inQuad)
{

}

void
MGrQuadSplit_Solid_TextureLit_InterpVertex_LMOff(
	M3tQuadSplit*		inQuad)
{

}

// Unlit
void
MGrQuadSplit_Solid_TextureUnlit_LMOff(
	M3tQuadSplit*		inQuad)
{
	MGtTextureMapPrivate*	baseMapPrivate = MGmGetBaseMapPrivate(MGgDrawContextPrivate);
	M3tTextureCoord*		textureCoords = MGmGetTextureCoords(MGgDrawContextPrivate);
	GrVertex*				vertexList;
	float					u_scale, v_scale;
	int						iList[4];
	
	UUmAssertReadPtr(baseMapPrivate, sizeof(*baseMapPrivate));
	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Interpolation] ==
				M3cDrawState_Interpolation_None);
	
	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Appearence] ==
				M3cDrawState_Appearence_Texture_Unlit);
	
	vertexList = MGgDrawContextPrivate->vertexList;
	UUmAssert(vertexList != NULL);
	
	if(MGgDrawContextPrivate->clipping)
	{
		UUtUns16	numRealVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];
		M3tPointScreen*			screenPoints = MGmGetScreenPoints(MGgDrawContextPrivate);
		
		UUmAssert(numRealVertices < MGcMaxElements);
	
		MGmSplatVertex_XYZ(inQuad->vertexIndices.indices[0]);
		MGmSplatVertex_XYZ(inQuad->vertexIndices.indices[1]);
		MGmSplatVertex_XYZ(inQuad->vertexIndices.indices[2]);
		MGmSplatVertex_XYZ(inQuad->vertexIndices.indices[3]);
	}
	
	u_scale = baseMapPrivate->u_scale;
	v_scale = baseMapPrivate->v_scale;

	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inQuad->baseUVIndices.indices[0],
		vertexList + inQuad->vertexIndices.indices[0]);
		
	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inQuad->baseUVIndices.indices[1],
		vertexList + inQuad->vertexIndices.indices[1]);
		
	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inQuad->baseUVIndices.indices[2],
		vertexList + inQuad->vertexIndices.indices[2]);
		
	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inQuad->baseUVIndices.indices[3],
		vertexList + inQuad->vertexIndices.indices[3]);
		
	iList[0] = inQuad->vertexIndices.indices[0];
	iList[1] = inQuad->vertexIndices.indices[1];
	iList[2] = inQuad->vertexIndices.indices[2];
	iList[3] = inQuad->vertexIndices.indices[3];

	MGrDrawPolygon(
		4,
		iList,
		vertexList);
	
	return;
}
