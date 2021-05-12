/*
	FILE:	MD_DC_Method_Bitmap.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Nov 13, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"

#include "MD_DC_Private.h"
#include "MD_DC_Method_Bitmap.h"
#include "MD_DC_Method_Quad.h"

// ======================================================================
// defines
// ======================================================================

// ======================================================================
// funcitons
// ======================================================================
// ----------------------------------------------------------------------
void 
MDrDrawContext_Method_Bitmap(
	M3tDrawContext			*inDrawContext,
	M3tTextureMap			*inBitmap,
	M3tPointScreen			*inDestPoint,
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	UUtUns16				inShade,
	UUtUns16				inAlpha)			// 0 - 31
{
	MDtDrawContextPrivate	*drawContextPrivate;
	MDtTextureMapPrivate	*textureMapPrivate;
	M3tPointScreen			vc[4];
	UUtUns16				vs[4];
	M3tTextureCoord			vt[4];
	float					max_u;
	float					max_v;
	
	// get access to the private context
	drawContextPrivate =
		(MDtDrawContextPrivate*)inDrawContext->privateContext;
	if (drawContextPrivate == NULL)
		return;
	
	// get access to the private texturemap data
	textureMapPrivate =
		(MDtTextureMapPrivate*)(TMmInstance_GetDynamicData(inBitmap));

	vs[0] = vs[1] = vs[2] = vs[3] = 0xFFFF;
	
	max_u = (float)inWidth / (float)inBitmap->width;
	max_v = (float)inHeight / (float)inBitmap->height;
	
	vc[0].x = inDestPoint->x;
	vc[0].y = inDestPoint->y;
	vc[0].z = inDestPoint->z;
	vc[0].invW = inDestPoint->invW;
	vt[0].u = 0.0;
	vt[0].v = 0.0;
	
	vc[1].x = inDestPoint->x + inWidth;
	vc[1].y = inDestPoint->y;
	vc[1].z = inDestPoint->z;
	vc[1].invW = inDestPoint->invW;
	vt[1].u = max_u;
	vt[1].v = 0.0;
	
	vc[2].x = inDestPoint->x + inWidth;
	vc[2].y = inDestPoint->y + inHeight;
	vc[2].z = inDestPoint->z;
	vc[2].invW = inDestPoint->invW;
	vt[2].u = max_u;
	vt[2].v = max_v;
	
	vc[3].x = inDestPoint->x;
	vc[3].y = inDestPoint->y + inHeight;
	vc[3].z = inDestPoint->z;
	vc[3].invW = inDestPoint->invW;
	vt[3].u = 0.0;
	vt[3].v = max_v;
	
	M3rDraw_State_SetPtr(inDrawContext, M3cDrawStatePtrType_ScreenPointArray,  vc);
	M3rDraw_State_SetPtr(inDrawContext, M3cDrawStatePtrType_ScreenShadeArray_DC, vs);
	M3rDraw_State_SetPtr(inDrawContext, M3cDrawStatePtrType_TextureCoordArray, vt);
	M3rDraw_State_SetPtr(inDrawContext, M3cDrawStatePtrType_BaseTextureMap, inBitmap);
	
	MDrDrawContext_Method_QuadTextureFlat(inDrawContext, 0, 1, 2, 3, 0);
}