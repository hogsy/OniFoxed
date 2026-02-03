/*
	FILE:	MG_DC_Method_LinePoint.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 13, 1997

	PURPOSE:

	Copyright 1997

*/

#include <stdlib.h>

#include "BFW.h"
#include "BFW_Motoko.h"

#include "BFW_Shared_TriRaster.h"

#include "MG_DC_Private.h"
#include "MG_DC_Method_LinePoint.h"
#include "MG_DC_Method_State.h"
#include "rasterizer_3dfx.h"

#define cAntiAliasedLines 0

void
MGrPoint(
	M3tPointScreen*	invCoord)
{
	GrVertex	glide_vertex;

	MGmConvertVertex_XYZ(invCoord, &glide_vertex);

	glide_vertex.oow *= 1.01f;

	MGrSet_ColorCombine(MGcColorCombine_ConstantColor);
	MGrSet_AlphaCombine(MGcAlphaCombine_NoAlpha);
	grDrawPoint(&glide_vertex);
}

void
MGrLine_InterpVertex(
	UUtUns32		inVIndex0,
	UUtUns32		inVIndex1)
{
	MGrLine_InterpNone(
		inVIndex0,
		inVIndex1);
}

void
MGrLine_InterpNone(
	UUtUns32		inVIndex0,
	UUtUns32		inVIndex1)
{
	M3tPointScreen				*screenPoints;

	GrVertex glide_vertices[2];

	screenPoints =
		(M3tPointScreen*)MGgDrawContextPrivate->statePtr[
			M3cDrawStatePtrType_ScreenPointArray];

	MGmConvertVertex_XYZ(screenPoints + inVIndex0, glide_vertices + 0);
	MGmConvertVertex_XYZ(screenPoints + inVIndex1, glide_vertices + 1);

	glide_vertices[0].oow *= 1.01f;
	glide_vertices[1].oow *= 1.01f;

#if cAntiAliasedLines
	glide_vertices[0].a = 255.f;
	glide_vertices[1].a = 255.f;
	MGrSet_ColorCombine(MGcColorCombine_ConstantColor);
	MGrSet_AlphaCombine(MGcAlphaCombine_Anti_Alias);
	grAADrawLine(glide_vertices + 0, glide_vertices + 1);
#else
	MGrSet_ColorCombine(MGcColorCombine_ConstantColor);
	MGrSet_AlphaCombine(MGcAlphaCombine_NoAlpha);
	grDrawLine(glide_vertices + 0, glide_vertices + 1);
#endif


	return;
}

