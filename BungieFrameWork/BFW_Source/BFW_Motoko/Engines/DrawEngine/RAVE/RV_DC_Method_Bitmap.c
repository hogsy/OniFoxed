/*
	FILE:	MG_DC_Method_Bitmap.c

	AUTHOR:	Brent H. Pease, Michael Evans

	CREATED: Nov 13, 1997

	PURPOSE:

	Copyright 1997, 1998

*/

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"

#include "Motoko_Manager.h"

#include "RV_DC_Private.h"
#include "RV_DC_Method_Bitmap.h"
#include "RV_DC_Method_Quad.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
RVrDrawContext_Method_Sprite(
	const M3tPointScreen	*inPoints,				// tl, tr, bl, br
	const M3tTextureCoord	*inTextureCoords)		// tl, tr, bl, br
{
	TQAVTexture	raveVertices[4];

	UUmAssert(RVgActiveRaveContext != NULL);

	RVmVertex_ConvertTexture(raveVertices[0], inPoints[0], inTextureCoords[0]);
	RVmVertex_ConvertTexture(raveVertices[1], inPoints[1], inTextureCoords[1]);
	RVmVertex_ConvertTexture(raveVertices[2], inPoints[2], inTextureCoords[2]);
	RVmVertex_ConvertTexture(raveVertices[3], inPoints[3], inTextureCoords[3]);

	QADrawTriTexture(
		RVgActiveRaveContext,
		raveVertices + 0,
		raveVertices + 1,
		raveVertices + 2,
		kQATriFlags_None);

	QADrawTriTexture(
		RVgActiveRaveContext,
		raveVertices + 0,
		raveVertices + 2,
		raveVertices + 3,
		kQATriFlags_None);

}

UUtError
RVrDrawContext_Method_ScreenCapture(
	const UUtRect *inRect,
	void *outBuffer)
{

	return UUcError_None;
}
