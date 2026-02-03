/*
	FILE:	MG_Polygon.h

	AUTHOR:	Michael Evans

	CREATED: July 5, 1998

	PURPOSE:

	Copyright 1998

*/

#ifndef MG_POLYGON_H
#define MG_POLYGON_H

#if 0

#include "BFW.h"
#include "BFW_Motoko.h"

#include "Motoko_Manager.h"

#include "BFW_Shared_TriRaster.h"

#include "MG_DC_Private.h"
#include "MG_DC_Method_Triangle.h"
#include "MG_DC_Method_Quad.h"
#include "rasterizer_3dfx.h"

void MGrDrawPolygon_VertexList_TextureUnlit(
		int nverts,
		const GrVertex vlist[],
		MGtTextureMapPrivate *inTexture);

void MGrDrawPolygon_VertexList_TextureInterpolate(
		int nverts,
		const GrVertex vlist[],
		MGtTextureMapPrivate *inTexture);

void MGrDrawPolygon_VertexList_TextureFlat(
		int nverts,
		const GrVertex vlist[],
		MGtTextureMapPrivate *inTexture);

void MGrDrawTriangle_TextureInterpolate(
	const GrVertex*			inVertex0,
	const GrVertex*			inVertex1,
	const GrVertex*			inVertex2,
	MGtTextureMapPrivate*	inTexture);

#endif

#endif // MG_POLYGON_H
