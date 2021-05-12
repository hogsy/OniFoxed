/*
	FILE:	EM_DC_Method_Bitmap.c
	
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

#include "EM_DC_Private.h"
#include "EM_DC_Method_Bitmap.h"
#include "EM_DC_Method_Quad.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
EDrDrawContext_Method_Sprite(
	const M3tPointScreen	*inPoints,				// tl, tr, bl, br
	const M3tTextureCoord	*inTextureCoords,		// tl, tr, bl, br
	UUtUns16				inShade,
	UUtUns16				inAlpha)
{

}

UUtError
EDrDrawContext_Method_ScreenCapture(
	const UUtRect *inRect,
	void *outBuffer)
{

	return UUcError_None;
}


UUtBool
EDrDrawContext_Method_PointVisible(
	const M3tPointScreen	*inPoint,
	float					inTolerance)
{
	return UUcTrue;
}
