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
#include "BFW_MathLib.h"
#include "BFW_Console.h"

#include "Motoko_Manager.h"

#include "MG_DC_Private.h"
#include "MG_DC_Method_Bitmap.h"
#include "MG_DC_Method_Quad.h"

#include "glide.h"
#include "rasterizer_3dfx.h"

// this table converts from Motoko alpha to glide vertex alpha
float MGg5BitsTo0_255FloatTable[32] = 
{
	( 0.f * (MGc255Float / ((float) MGcMax5Bits))), ( 1.f * (MGc255Float / ((float) MGcMax5Bits))), ( 2.f * (MGc255Float / ((float) MGcMax5Bits))), ( 3.f * (MGc255Float / ((float) MGcMax5Bits))),
	( 4.f * (MGc255Float / ((float) MGcMax5Bits))), ( 5.f * (MGc255Float / ((float) MGcMax5Bits))), ( 6.f * (MGc255Float / ((float) MGcMax5Bits))), ( 7.f * (MGc255Float / ((float) MGcMax5Bits))),

	( 8.f * (MGc255Float / ((float) MGcMax5Bits))), ( 9.f * (MGc255Float / ((float) MGcMax5Bits))), (10.f * (MGc255Float / ((float) MGcMax5Bits))), (11.f * (MGc255Float / ((float) MGcMax5Bits))),
	(12.f * (MGc255Float / ((float) MGcMax5Bits))), (13.f * (MGc255Float / ((float) MGcMax5Bits))), (14.f * (MGc255Float / ((float) MGcMax5Bits))), (15.f * (MGc255Float / ((float) MGcMax5Bits))),

	(16.f * (MGc255Float / ((float) MGcMax5Bits))), (17.f * (MGc255Float / ((float) MGcMax5Bits))), (18.f * (MGc255Float / ((float) MGcMax5Bits))), (19.f * (MGc255Float / ((float) MGcMax5Bits))),
	(20.f * (MGc255Float / ((float) MGcMax5Bits))), (21.f * (MGc255Float / ((float) MGcMax5Bits))), (22.f * (MGc255Float / ((float) MGcMax5Bits))), (23.f * (MGc255Float / ((float) MGcMax5Bits))),

	(24.f * (MGc255Float / ((float) MGcMax5Bits))), (25.f * (MGc255Float / ((float) MGcMax5Bits))), (26.f * (MGc255Float / ((float) MGcMax5Bits))), (27.f * (MGc255Float / ((float) MGcMax5Bits))),
	(28.f * (MGc255Float / ((float) MGcMax5Bits))), (29.f * (MGc255Float / ((float) MGcMax5Bits))), (30.f * (MGc255Float / ((float) MGcMax5Bits))), (31.f * (MGc255Float / ((float) MGcMax5Bits))),
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------


void
MGrDrawContext_Method_TriSprite(
	const M3tPointScreen	*inPoints,				// points[3]
	const M3tTextureCoord	*inTextureCoords)		// UVs[3]
{
	const MGtTextureMapPrivate	*textureMapPrivate = MGgDrawContextPrivate->curBaseTexture;

	//
	// glide meshing rules state: non horizontal lines: left edge in
	// horizontal lines, small y in (top for upper left origin)
	//

	float y0 = SNAP_COORD(inPoints[0].y);
	float x0 = SNAP_COORD(inPoints[0].x);
	float ooz0 = 65535.f / inPoints[0].z;
	float oow0 = inPoints[0].invW;
	float y1 = SNAP_COORD(inPoints[1].y);
	float x1 = SNAP_COORD(inPoints[1].x);
	float ooz1 = 65535.f / inPoints[1].z;
	float oow1 = inPoints[1].invW;
	float y2 = SNAP_COORD(inPoints[2].y);
	float x2 = SNAP_COORD(inPoints[2].x);
	float ooz2 = 65535.f / inPoints[2].z;
	float oow2 = inPoints[2].invW;

	GrVertex glide_vertices[3];	
	
	UUmAssert(textureMapPrivate != NULL);
	
	glide_vertices[0].x= x0;
	glide_vertices[0].y= y0;
	glide_vertices[0].ooz= ooz0;
	glide_vertices[0].oow= oow0;
	glide_vertices[0].tmuvtx[0].sow= textureMapPrivate->u_scale*inTextureCoords[0].u*oow0;
	glide_vertices[0].tmuvtx[0].tow= textureMapPrivate->v_scale*inTextureCoords[0].v*oow0;

	glide_vertices[1].x= x1;
	glide_vertices[1].y= y1;
	glide_vertices[1].ooz= ooz1;
	glide_vertices[1].oow= oow1;
	glide_vertices[1].tmuvtx[0].sow= textureMapPrivate->u_scale*inTextureCoords[1].u*oow1;
	glide_vertices[1].tmuvtx[0].tow= textureMapPrivate->v_scale*inTextureCoords[1].v*oow1;

	glide_vertices[2].x= x2;
	glide_vertices[2].y= y2;
	glide_vertices[2].ooz= ooz2;
	glide_vertices[2].oow= oow2;
	glide_vertices[2].tmuvtx[0].sow= textureMapPrivate->u_scale*inTextureCoords[2].u*oow2;
	glide_vertices[2].tmuvtx[0].tow= textureMapPrivate->v_scale*inTextureCoords[2].v*oow2;

	grDrawPlanarPolygonVertexList(3, glide_vertices);
	
	return;
}

void
MGrDrawContext_Method_Sprite(
	const M3tPointScreen	*inPoints,				// tl, br
	const M3tTextureCoord	*inTextureCoords)		// tl, tr, bl, br
{
	const MGtTextureMapPrivate	*textureMapPrivate = MGgDrawContextPrivate->curBaseTexture;
	float ooz = 65535.f / inPoints[0].z;
	float oow = inPoints[0].invW;

	//
	// glide meshing rules state: non horizontal lines: left edge in
	// horizontal lines, small y in (top for upper left origin)
	//

	float y0 = SNAP_COORD(inPoints[0].y);
	float x0 = SNAP_COORD(inPoints[0].x);
	float y1 = SNAP_COORD(inPoints[1].y);
	float x1 = SNAP_COORD(inPoints[1].x);

	GrVertex glide_vertices[4];	
	
	UUmAssert(textureMapPrivate != NULL);
	
	// upper left
	glide_vertices[1].x= x0;
	glide_vertices[1].y= y0;
	glide_vertices[1].ooz= ooz;
	glide_vertices[1].oow= oow;
	glide_vertices[1].tmuvtx[0].sow= textureMapPrivate->u_scale*inTextureCoords[0].u*oow;
	glide_vertices[1].tmuvtx[0].tow= textureMapPrivate->v_scale*inTextureCoords[0].v*oow;

	// upper right
	glide_vertices[2].x= x1;
	glide_vertices[2].y= y0;
	glide_vertices[2].ooz= ooz;
	glide_vertices[2].oow= oow;
	glide_vertices[2].tmuvtx[0].sow= textureMapPrivate->u_scale*inTextureCoords[1].u*oow;
	glide_vertices[2].tmuvtx[0].tow= textureMapPrivate->v_scale*inTextureCoords[1].v*oow;

	// lower left
	glide_vertices[0].x= x0;
	glide_vertices[0].y= y1;
	glide_vertices[0].ooz= ooz;
	glide_vertices[0].oow= oow;
	glide_vertices[0].tmuvtx[0].sow= textureMapPrivate->u_scale*inTextureCoords[2].u*oow;
	glide_vertices[0].tmuvtx[0].tow= textureMapPrivate->v_scale*inTextureCoords[2].v*oow;

	// lower right
	glide_vertices[3].x= x1;
	glide_vertices[3].y= y1;
	glide_vertices[3].ooz= ooz;
	glide_vertices[3].oow= oow;
	glide_vertices[3].tmuvtx[0].sow= textureMapPrivate->u_scale*inTextureCoords[3].u*oow;
	glide_vertices[3].tmuvtx[0].tow= textureMapPrivate->v_scale*inTextureCoords[3].v*oow;

	grDrawPlanarPolygonVertexList(4, glide_vertices);
	
	return;
}


UUtError
MGrDrawContext_Method_ScreenCapture(
	const UUtRect  *inRect,
	void *outBuffer)
{
	UUtError error;
	FxBool success;
	FxU32 width = inRect->right - inRect->left + 1;
	FxU32 height= inRect->bottom - inRect->top + 1;

	success = grLfbReadRegion(
		GR_BUFFER_FRONTBUFFER,
		inRect->left,
		inRect->top,
		width,
		height,
		width * 2,
		outBuffer);

	UUmAssert(success);

	if (success) {
		error = UUcError_None;
	}
	else {
		error = UUcError_Generic;
	}

	return UUcError_None;
}

UUtBool 
MGrDrawContext_Method_PointVisible(
	const M3tPointScreen	*inPoint,
	float					inTolerance)
{
	FxBool returnval;
	GrLfbInfo_t buffer_info;
	FxU16 x = (FxU16) MUrUnsignedSmallFloat_To_Uns_Round(inPoint->x);
	FxU16 y = (FxU16) MUrUnsignedSmallFloat_To_Uns_Round(inPoint->y);
	FxU16 *readptr, exponent, mantissa, depth_val;
	float decoded_invw;

	// lock the depth buffer for reads
	UUrMemory_Clear(&buffer_info, sizeof(buffer_info));
	buffer_info.size = sizeof(buffer_info);
	returnval = grLfbLock(GR_LFB_READ_ONLY | GR_LFB_IDLE, GR_BUFFER_AUXBUFFER, GR_LFBWRITEMODE_ANY,
							GR_ORIGIN_ANY, FXFALSE, &buffer_info);
	if (returnval != FXTRUE) {
		COrConsole_Printf("### cannot lock glide buffer for reading");
		return UUcTrue;
	}

	if (buffer_info.origin == GR_ORIGIN_LOWER_LEFT) {
		y = MGgDrawContextPrivate->height - y;
	}

	readptr = ((FxU16 *) buffer_info.lfbPtr) + y * buffer_info.strideInBytes / 2 + x;
	depth_val = *readptr;

	grLfbUnlock(GR_LFB_READ_ONLY, GR_BUFFER_AUXBUFFER);

	// depth value is a 16-bit floating point number with no sign bit, 4 bits of exponent and 12 bits
	// of mantissa
	exponent = (depth_val & 0xF000) >> 12;
	mantissa = (depth_val & 0x0FFF);
	decoded_invw = 1.0f + mantissa / 4096.0f;		// mantissa / 2^12
	decoded_invw = 1.0f / (decoded_invw * (1 << exponent));

	// visible if the point isn't behind the distance stored in the depth buffer
	return (decoded_invw < inPoint->invW);
}
