/*
	FILE:	MG_DC_Method_Pent.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 18, 1997

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_BitVector.h"

#include "Motoko_Manager.h"

#include "MG_DC_Private.h"
#include "MG_DC_Method_Triangle.h"
#include "MG_DC_Method_State.h"
#include "MG_DC_Method_Pent.h"

#include "rasterizer_3dfx.h"


void
MGrPent_Point(
	M3tPent*		inPent)
{
	M3tPointScreen*	screenPoints = MGmGetScreenPoints(MGgDrawContextPrivate);
	UUtUns32*		vertexShades = MGmGetVertexShades(MGgDrawContextPrivate);
	GrVertex*		vertexList;

	vertexList = MGgDrawContextPrivate->vertexList;
	UUmAssert(vertexList != NULL);

	// Someday move this up
	MGrSet_ColorCombine(MGcColorCombine_ConstantColor);
	MGrSet_AlphaCombine(MGcAlphaCombine_NoAlpha);

	vertexList[inPent->indices[0]].oow *= 1.01f;
	grDrawPoint(vertexList + inPent->indices[0]);

	vertexList[inPent->indices[1]].oow *= 1.01f;
	grDrawPoint(vertexList + inPent->indices[1]);

	vertexList[inPent->indices[2]].oow *= 1.01f;
	grDrawPoint(vertexList + inPent->indices[2]);

	vertexList[inPent->indices[3]].oow *= 1.01f;
	grDrawPoint(vertexList + inPent->indices[3]);

	vertexList[inPent->indices[4]].oow *= 1.01f;
	grDrawPoint(vertexList + inPent->indices[4]);
}

void
MGrPent_Line_InterpNone(
	M3tPent*		inPent)
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

	for(vertexItr = 0; vertexItr < 5; vertexItr++)
	{
		vertexList[inPent->indices[vertexItr]].oow *= 1.01f;
		vertexList[inPent->indices[(vertexItr + 1) % 5]].oow *= 1.01f;
		grDrawLine(vertexList + inPent->indices[vertexItr], vertexList + inPent->indices[(vertexItr + 1) % 5]);
	}
}

void
MGrPent_Line_InterpVertex(
	M3tPent*		inPent)
{
	MGrPent_Line_InterpNone(inPent);
}

void
MGrPent_Solid_Gouraud_InterpNone(
	M3tPent*		inPent)
{
	GrVertex*		vertexList;
	int				iList[5];

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

		MGmSplatVertex_XYZ(inPent->indices[0]);
		MGmSplatVertex_XYZ(inPent->indices[1]);
		MGmSplatVertex_XYZ(inPent->indices[2]);
		MGmSplatVertex_XYZ(inPent->indices[3]);
		MGmSplatVertex_XYZ(inPent->indices[4]);
	}

	iList[0] = inPent->indices[0];
	iList[1] = inPent->indices[1];
	iList[2] = inPent->indices[2];
	iList[3] = inPent->indices[3];
	iList[4] = inPent->indices[4];

	MGrDrawPolygon(
		5,
		iList,
		vertexList);

}

void
MGrPent_Solid_Gouraud_InterpVertex(
	M3tPent*		inPent)
{
	GrVertex*		vertexList;
	int				iList[5];

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

		MGmSplatVertex_XYZRGB(inPent->indices[0]);
		MGmSplatVertex_XYZRGB(inPent->indices[1]);
		MGmSplatVertex_XYZRGB(inPent->indices[2]);
		MGmSplatVertex_XYZRGB(inPent->indices[3]);
		MGmSplatVertex_XYZRGB(inPent->indices[4]);
	}

	iList[0] = inPent->indices[0];
	iList[1] = inPent->indices[1];
	iList[2] = inPent->indices[2];
	iList[3] = inPent->indices[3];
	iList[4] = inPent->indices[4];

	MGrDrawPolygon(
		5,
		iList,
		vertexList);
}

void
MGrPent_Solid_TextureLit_InterpNone(
	M3tPent*		inPent)
{
	GrVertex*		vertexList;
	int				iList[5];

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

		MGmSplatVertex_XYZUV(inPent->indices[0]);
		MGmSplatVertex_XYZUV(inPent->indices[1]);
		MGmSplatVertex_XYZUV(inPent->indices[2]);
		MGmSplatVertex_XYZUV(inPent->indices[3]);
		MGmSplatVertex_XYZUV(inPent->indices[4]);
	}

	iList[0] = inPent->indices[0];
	iList[1] = inPent->indices[1];
	iList[2] = inPent->indices[2];
	iList[3] = inPent->indices[3];
	iList[4] = inPent->indices[4];

	MGrDrawPolygon(
		5,
		iList,
		vertexList);
}

void
MGrPent_Solid_TextureLit_InterpVertex(
	M3tPent*		inPent)
{
	GrVertex*		vertexList;
	int				iList[5];

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

		MGmSplatVertex_XYZUVRGB(inPent->indices[0]);
		MGmSplatVertex_XYZUVRGB(inPent->indices[1]);
		MGmSplatVertex_XYZUVRGB(inPent->indices[2]);
		MGmSplatVertex_XYZUVRGB(inPent->indices[3]);
		MGmSplatVertex_XYZUVRGB(inPent->indices[4]);
	}

	MGmAssertVertex_RGB(vertexList + inPent->indices[0]);
	MGmAssertVertex_RGB(vertexList + inPent->indices[1]);
	MGmAssertVertex_RGB(vertexList + inPent->indices[2]);
	MGmAssertVertex_RGB(vertexList + inPent->indices[3]);
	MGmAssertVertex_RGB(vertexList + inPent->indices[4]);

	iList[0] = inPent->indices[0];
	iList[1] = inPent->indices[1];
	iList[2] = inPent->indices[2];
	iList[3] = inPent->indices[3];
	iList[4] = inPent->indices[4];

	MGrDrawPolygon(
		5,
		iList,
		vertexList);
}

void
MGrPent_Solid_TextureLit_EnvMap_InterpVertex(
	M3tPent*		inPent)
{
	GrVertex*		vertexList;
	int				iList[5];

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

		MGmSplatVertex_XYZUVRGBEnvMap(inPent->indices[0]);
		MGmSplatVertex_XYZUVRGBEnvMap(inPent->indices[1]);
		MGmSplatVertex_XYZUVRGBEnvMap(inPent->indices[2]);
		MGmSplatVertex_XYZUVRGBEnvMap(inPent->indices[3]);
		MGmSplatVertex_XYZUVRGBEnvMap(inPent->indices[4]);
	}

	MGmAssertVertex_RGB(vertexList + inPent->indices[0]);
	MGmAssertVertex_RGB(vertexList + inPent->indices[1]);
	MGmAssertVertex_RGB(vertexList + inPent->indices[2]);
	MGmAssertVertex_RGB(vertexList + inPent->indices[3]);
	MGmAssertVertex_RGB(vertexList + inPent->indices[4]);

	iList[0] = inPent->indices[0];
	iList[1] = inPent->indices[1];
	iList[2] = inPent->indices[2];
	iList[3] = inPent->indices[3];
	iList[4] = inPent->indices[4];

	MGrDrawPolygon(
		5,
		iList,
		vertexList);
}

void
MGrPent_Solid_TextureUnlit(
	M3tPent*		inPent)
{
	MGrPent_Solid_TextureLit_InterpNone(inPent);
}

// Trisplit functions
void
MGrPentSplit_Solid_TextureLit_InterpNone_LMOff(
	M3tPentSplit*		inPent)
{

}
void
MGrPentSplit_Solid_TextureLit_InterpVertex_LMOff(
	M3tPentSplit*		inPent)
{

}

// Unlit

void
MGrPentSplit_Solid_TextureUnlit_LMOff(
	M3tPentSplit*		inPent)
{
	MGtTextureMapPrivate*	baseMapPrivate = MGmGetBaseMapPrivate(MGgDrawContextPrivate);
	M3tTextureCoord*		textureCoords = MGmGetTextureCoords(MGgDrawContextPrivate);
	GrVertex*				vertexList;
	float					u_scale, v_scale;
	int						iList[5];

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

		MGmSplatVertex_XYZ(inPent->vertexIndices.indices[0]);
		MGmSplatVertex_XYZ(inPent->vertexIndices.indices[1]);
		MGmSplatVertex_XYZ(inPent->vertexIndices.indices[2]);
		MGmSplatVertex_XYZ(inPent->vertexIndices.indices[3]);
		MGmSplatVertex_XYZ(inPent->vertexIndices.indices[4]);
	}

	u_scale = baseMapPrivate->u_scale;
	v_scale = baseMapPrivate->v_scale;

	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inPent->baseUVIndices.indices[0],
		vertexList + inPent->vertexIndices.indices[0]);

	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inPent->baseUVIndices.indices[1],
		vertexList + inPent->vertexIndices.indices[1]);

	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inPent->baseUVIndices.indices[2],
		vertexList + inPent->vertexIndices.indices[2]);

	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inPent->baseUVIndices.indices[3],
		vertexList + inPent->vertexIndices.indices[3]);

	MGmConvertVertex_UV(
		u_scale, v_scale,
		0,
		textureCoords + inPent->baseUVIndices.indices[4],
		vertexList + inPent->vertexIndices.indices[4]);

	iList[0] = inPent->vertexIndices.indices[0];
	iList[1] = inPent->vertexIndices.indices[1];
	iList[2] = inPent->vertexIndices.indices[2];
	iList[3] = inPent->vertexIndices.indices[3];
	iList[4] = inPent->vertexIndices.indices[4];

	MGrDrawPolygon(
		5,
		iList,
		vertexList);
}


