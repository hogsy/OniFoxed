/*
	FILE:	MG_DC_Method_Triangle.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 18, 1997

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_BitVector.h"

#include "Motoko_Manager.h"

#include "BFW_Shared_TriRaster.h"

#include "MG_DC_Private.h"
#include "MG_DC_Method_Triangle.h"
#include "MG_DC_Method_State.h"
#include "rasterizer_3dfx.h"
#include "MG_Polygon.h"

#define DYNAHEADER
#include "glide.h"

void
MGrTriangle_Point(
	M3tTri*		inTri)
{
	M3tPointScreen*	screenPoints = MGmGetScreenPoints(MGgDrawContextPrivate);
	UUtUns32*		vertexShades = MGmGetVertexShades(MGgDrawContextPrivate);
	GrVertex*		vertexList;

	vertexList = MGgDrawContextPrivate->vertexList;
	UUmAssert(vertexList != NULL);

	// Someday move this up
	MGrSet_ColorCombine(MGcColorCombine_ConstantColor);
	MGrSet_AlphaCombine(MGcAlphaCombine_NoAlpha);

	vertexList[inTri->indices[0]].oow *= 1.01f;
	grDrawPoint(vertexList + inTri->indices[0]);

	vertexList[inTri->indices[1]].oow *= 1.01f;
	grDrawPoint(vertexList + inTri->indices[1]);

	vertexList[inTri->indices[2]].oow *= 1.01f;
	grDrawPoint(vertexList + inTri->indices[2]);
}

void
MGrTriangle_Line_InterpNone(
	M3tTri*		inTri)
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

	for(vertexItr = 0; vertexItr < 3; vertexItr++)
	{
		vertexList[inTri->indices[vertexItr]].oow *= 1.01f;
		vertexList[inTri->indices[(vertexItr + 1) % 3]].oow *= 1.01f;
		grDrawLine(vertexList + inTri->indices[vertexItr], vertexList +inTri->indices[(vertexItr + 1) % 3]);
	}
}

void
MGrTriangle_Line_InterpVertex(
	M3tTri*		inTri)
{
	MGrTriangle_Line_InterpNone(inTri);
}

void
MGrTriangle_Solid_Gouraud_InterpNone(
	M3tTri*		inTri)
{
	GrVertex*		vertexList;

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

		MGmSplatVertex_XYZ(inTri->indices[0]);
		MGmSplatVertex_XYZ(inTri->indices[1]);
		MGmSplatVertex_XYZ(inTri->indices[2]);
	}

	MGrDrawTriangle(
		vertexList + inTri->indices[0],
		vertexList + inTri->indices[1],
		vertexList + inTri->indices[2]);
}

void
MGrTriangle_Solid_Gouraud_InterpVertex(
	M3tTri*		inTri)
{
	GrVertex*		vertexList;

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

		MGmSplatVertex_XYZRGB(inTri->indices[0]);
		MGmSplatVertex_XYZRGB(inTri->indices[1]);
		MGmSplatVertex_XYZRGB(inTri->indices[2]);
	}

	MGrDrawTriangle(
		vertexList + inTri->indices[0],
		vertexList + inTri->indices[1],
		vertexList + inTri->indices[2]);
}

void
MGrTriangle_Solid_TextureLit_InterpNone(
	M3tTri*		inTri)
{
	GrVertex*				vertexList;

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

		MGmSplatVertex_XYZUV(inTri->indices[0]);
		MGmSplatVertex_XYZUV(inTri->indices[1]);
		MGmSplatVertex_XYZUV(inTri->indices[2]);
	}

	MGrDrawTriangle(
		vertexList + inTri->indices[0],
		vertexList + inTri->indices[1],
		vertexList + inTri->indices[2]);
}

#undef MGmAssertVertex_XYZ

void
MGrTriangle_Solid_TextureLit_InterpVertex(
	M3tTri*		inTri)
{
	GrVertex*				vertexList;

	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Interpolation] ==
				M3cDrawState_Interpolation_Vertex);

	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Appearence] ==
				M3cDrawState_Appearence_Texture_Lit);

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

		MGmSplatVertex_XYZUVRGB(inTri->indices[0]);
		MGmSplatVertex_XYZUVRGB(inTri->indices[1]);
		MGmSplatVertex_XYZUVRGB(inTri->indices[2]);
	}

	MGmAssertVertex_RGB(vertexList + inTri->indices[0]);
	MGmAssertVertex_RGB(vertexList + inTri->indices[1]);
	MGmAssertVertex_RGB(vertexList + inTri->indices[2]);

	MGrDrawTriangle(
		vertexList + inTri->indices[0],
		vertexList + inTri->indices[1],
		vertexList + inTri->indices[2]);

}

void
MGrTriangle_Solid_TextureLit_EnvMap_InterpVertex(
	M3tTri*		inTri)
{
	GrVertex*				vertexList;

	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Interpolation] ==
				M3cDrawState_Interpolation_Vertex);

	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Appearence] ==
				M3cDrawState_Appearence_Texture_Lit_EnvMap);

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

		MGmSplatVertex_XYZUVRGBEnvMap(inTri->indices[0]);
		MGmSplatVertex_XYZUVRGBEnvMap(inTri->indices[1]);
		MGmSplatVertex_XYZUVRGBEnvMap(inTri->indices[2]);
	}

	MGmAssertVertex_RGB(vertexList + inTri->indices[0]);
	MGmAssertVertex_RGB(vertexList + inTri->indices[1]);
	MGmAssertVertex_RGB(vertexList + inTri->indices[2]);

	MGrDrawTriangle(
		vertexList + inTri->indices[0],
		vertexList + inTri->indices[1],
		vertexList + inTri->indices[2]);

}

void
MGrTriangle_Solid_TextureUnlit(
	M3tTri*		inTri)
{
	// XXX - Handle me correctly someday
	MGrTriangle_Solid_TextureLit_InterpNone(inTri);
}

// Trisplit functions

void
MGrTriSplit_Solid_TextureLit_InterpNone_LMOff(
	M3tTriSplit*		inTri)
{

}

void
MGrTriSplit_Solid_TextureLit_InterpVertex_LMOff(
	M3tTriSplit*		inTri)
{

}

// Unlit

void
MGrTriSplit_Solid_TextureUnlit_LMOff(
	M3tTriSplit*		inTri)
{
	MGtTextureMapPrivate*	baseMapPrivate = MGmGetBaseMapPrivate(MGgDrawContextPrivate);
	M3tTextureCoord*		textureCoords = MGmGetTextureCoords(MGgDrawContextPrivate);
	GrVertex*				vertexList;
	float					u_scale, v_scale;

	UUmAssertReadPtr(baseMapPrivate, sizeof(*baseMapPrivate));
	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Interpolation] ==
				M3cDrawState_Interpolation_None);

	UUmAssert(MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_Appearence] ==
				M3cDrawState_Appearence_Texture_Unlit);

	vertexList = MGgDrawContextPrivate->vertexList;
	UUmAssert(vertexList != NULL);


	if(MGgDrawContextPrivate->clipping)
	{
		M3tPointScreen*			screenPoints = MGmGetScreenPoints(MGgDrawContextPrivate);
		UUtUns16	numRealVertices = (UUtUns16)MGgDrawContextPrivate->stateInt[M3cDrawStateIntType_NumRealVertices];

		UUmAssert(numRealVertices < MGcMaxElements);

		MGmSplatVertex_XYZ(inTri->vertexIndices.indices[0]);
		MGmSplatVertex_XYZ(inTri->vertexIndices.indices[1]);
		MGmSplatVertex_XYZ(inTri->vertexIndices.indices[2]);
	}


	u_scale = baseMapPrivate->u_scale;
	v_scale = baseMapPrivate->v_scale;

	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inTri->baseUVIndices.indices[0],
		vertexList + inTri->vertexIndices.indices[0]);

	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inTri->baseUVIndices.indices[1],
		vertexList + inTri->vertexIndices.indices[1]);

	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inTri->baseUVIndices.indices[2],
		vertexList + inTri->vertexIndices.indices[2]);

	MGrDrawTriangle(
		vertexList + inTri->vertexIndices.indices[0],
		vertexList + inTri->vertexIndices.indices[1],
		vertexList + inTri->vertexIndices.indices[2]);

}
