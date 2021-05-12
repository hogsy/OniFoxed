/*
	FILE:	MD_DC_Method_Triangle.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 18, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"

#include "BFW_Shared_TriRaster.h"

#include "MD_DC_Private.h"
#include "MD_DC_Method_Triangle.h"

// ======================================================================
// funcitons
// ======================================================================
// ----------------------------------------------------------------------
void 
MDrDrawContext_Method_TriGouraudInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2)
{
	HRESULT						d3dResult;
	MDtDrawContextPrivate		*drawContextPrivate;
	D3DTLVERTEX					vertex_list[3];
	M3tPointScreen				*screenPoints;
	
	// get access to the private context
	drawContextPrivate =
		(MDtDrawContextPrivate*)inDrawContext->privateContext;
	if (drawContextPrivate == NULL)
		return;
	
	// get the list of screen points
	screenPoints =
		(M3tPointScreen*)drawContextPrivate->statePtr[
			M3cDrawStatePtrType_ScreenPointArray];
	
	// set up the vertex_list
	vertex_list[0].sx = screenPoints[inVIndex0].x;
	vertex_list[0].sy = screenPoints[inVIndex0].y;
	vertex_list[0].sz = screenPoints[inVIndex0].z;
	vertex_list[0].rhw = screenPoints[inVIndex0].invW;
	vertex_list[0].color = RGB_MAKE(255,255,255);
	vertex_list[0].specular = RGB_MAKE(0,0,0);
	
	vertex_list[1].sx = screenPoints[inVIndex1].x;
	vertex_list[1].sy = screenPoints[inVIndex1].y;
	vertex_list[1].sz = screenPoints[inVIndex1].z;
	vertex_list[1].rhw = screenPoints[inVIndex1].invW;
	vertex_list[1].color = RGB_MAKE(255,255,255);
	vertex_list[1].specular = RGB_MAKE(0,0,0);
	
	vertex_list[2].sx = screenPoints[inVIndex2].x;
	vertex_list[2].sy = screenPoints[inVIndex2].y;
	vertex_list[2].sz = screenPoints[inVIndex2].z;
	vertex_list[2].rhw = screenPoints[inVIndex2].invW;
	vertex_list[2].color = RGB_MAKE(255,255,255);
	vertex_list[2].specular = RGB_MAKE(0,0,0);
	
	// draw the triangle
	d3dResult = 
		IDirect3DDevice2_DrawPrimitive(
			drawContextPrivate->d3d_device2,
			D3DPT_TRIANGLELIST,
			D3DVT_TLVERTEX,
			vertex_list,
			3,
			D3DDP_WAIT);
}

// ----------------------------------------------------------------------
void 
MDrDrawContext_Method_TriGouraudFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inFaceShade)
{
	HRESULT						d3dResult;
	MDtDrawContextPrivate		*drawContextPrivate;
	D3DTLVERTEX					vertex_list[3];
	M3tPointScreen				*screenPoints;
	
	// get access to the private context
	drawContextPrivate =
		(MDtDrawContextPrivate*)inDrawContext->privateContext;
	if (drawContextPrivate == NULL)
		return;
	
	// get the list of screen points
	screenPoints =
		(M3tPointScreen*)drawContextPrivate->statePtr[
			M3cDrawStatePtrType_ScreenPointArray];
	
	// set up the vertex_list
	vertex_list[0].sx = screenPoints[inVIndex0].x;
	vertex_list[0].sy = screenPoints[inVIndex0].y;
	vertex_list[0].sz = screenPoints[inVIndex0].z;
	vertex_list[0].rhw = screenPoints[inVIndex0].invW;
	vertex_list[0].color = RGB_MAKE(255,255,255);
	vertex_list[0].specular = RGB_MAKE(0,0,0);
	
	vertex_list[1].sx = screenPoints[inVIndex1].x;
	vertex_list[1].sy = screenPoints[inVIndex1].y;
	vertex_list[1].sz = screenPoints[inVIndex1].z;
	vertex_list[1].rhw = screenPoints[inVIndex1].invW;
	vertex_list[1].color = RGB_MAKE(255,255,255);
	vertex_list[1].specular = RGB_MAKE(0,0,0);
	
	vertex_list[2].sx = screenPoints[inVIndex2].x;
	vertex_list[2].sy = screenPoints[inVIndex2].y;
	vertex_list[2].sz = screenPoints[inVIndex2].z;
	vertex_list[2].rhw = screenPoints[inVIndex2].invW;
	vertex_list[2].color = RGB_MAKE(255,255,255);
	vertex_list[2].specular = RGB_MAKE(0,0,0);
	
	// draw the triangle
	d3dResult = 
		IDirect3DDevice2_DrawPrimitive(
			drawContextPrivate->d3d_device2,
			D3DPT_TRIANGLELIST,
			D3DVT_TLVERTEX,
			vertex_list,
			3,
			D3DDP_WAIT);
}

// ----------------------------------------------------------------------
void 
MDrDrawContext_Method_TriTextureInterpolate(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2)
{
	HRESULT						d3dResult;
	MDtDrawContextPrivate		*drawContextPrivate;
	D3DTLVERTEX					vertex_list[3];
	M3tPointScreen				*screenPoints;
	
	// get access to the private context
	drawContextPrivate =
		(MDtDrawContextPrivate*)inDrawContext->privateContext;
	if (drawContextPrivate == NULL)
		return;
	
	// get the list of screen points
	screenPoints =
		(M3tPointScreen*)drawContextPrivate->statePtr[
			M3cDrawStatePtrType_ScreenPointArray];
	
	// set up the vertex_list
	vertex_list[0].sx = screenPoints[inVIndex0].x;
	vertex_list[0].sy = screenPoints[inVIndex0].y;
	vertex_list[0].sz = screenPoints[inVIndex0].z;
	vertex_list[0].rhw = screenPoints[inVIndex0].invW;
	vertex_list[0].color = RGB_MAKE(255,255,255);
	vertex_list[0].specular = RGB_MAKE(0,0,0);
	
	vertex_list[1].sx = screenPoints[inVIndex1].x;
	vertex_list[1].sy = screenPoints[inVIndex1].y;
	vertex_list[1].sz = screenPoints[inVIndex1].z;
	vertex_list[1].rhw = screenPoints[inVIndex1].invW;
	vertex_list[1].color = RGB_MAKE(255,255,255);
	vertex_list[1].specular = RGB_MAKE(0,0,0);
	
	vertex_list[2].sx = screenPoints[inVIndex2].x;
	vertex_list[2].sy = screenPoints[inVIndex2].y;
	vertex_list[2].sz = screenPoints[inVIndex2].z;
	vertex_list[2].rhw = screenPoints[inVIndex2].invW;
	vertex_list[2].color = RGB_MAKE(255,255,255);
	vertex_list[2].specular = RGB_MAKE(0,0,0);
	
	// draw the triangle
	d3dResult = 
		IDirect3DDevice2_DrawPrimitive(
			drawContextPrivate->d3d_device2,
			D3DPT_TRIANGLELIST,
			D3DVT_TLVERTEX,
			vertex_list,
			3,
			D3DDP_WAIT);
}

// ----------------------------------------------------------------------
void
MDrDrawContext_Method_TriTextureFlat(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inFaceShade)
{
	HRESULT						result;
	MDtDrawContextPrivate		*drawContextPrivate;
	D3DTLVERTEX					vertex_list[3];
	M3tPointScreen				*screenPoints;
	M3tTextureCoord				*textureCoords;
	UUtUns8						color1, color2, color3;

	color1 = color2 = color3 = 255;
	
	// get access to the data
	drawContextPrivate =
		(MDtDrawContextPrivate*)inDrawContext->privateContext;
	if (drawContextPrivate == NULL)
		return;
	screenPoints =
		(M3tPointScreen*)drawContextPrivate->statePtr[
			M3cDrawStatePtrType_ScreenPointArray];
	textureCoords =
		(M3tTextureCoord*)drawContextPrivate->statePtr[
			M3cDrawStatePtrType_TextureCoordArray];

	// tell Direct3D to use the texture
	MDrUseTexture(
		drawContextPrivate,
		drawContextPrivate->statePtr[M3cDrawStatePtrType_BaseTextureMap]);
	
	// set up the vertex_list
	vertex_list[0].sx = screenPoints[inVIndex0].x;
	vertex_list[0].sy = screenPoints[inVIndex0].y;
	vertex_list[0].sz = screenPoints[inVIndex0].z;
	vertex_list[0].rhw = screenPoints[inVIndex0].invW;
	vertex_list[0].tu = textureCoords[inVIndex0].u;
	vertex_list[0].tv = textureCoords[inVIndex0].v;
	vertex_list[0].color = RGB_MAKE(color1, color2, color3);
	vertex_list[0].specular = RGB_MAKE(0,0,0);
	
	vertex_list[1].sx = screenPoints[inVIndex1].x;
	vertex_list[1].sy = screenPoints[inVIndex1].y;
	vertex_list[1].sz = screenPoints[inVIndex1].z;
	vertex_list[1].rhw = screenPoints[inVIndex1].invW;
	vertex_list[1].tu = textureCoords[inVIndex1].u;
	vertex_list[1].tv = textureCoords[inVIndex1].v;
	vertex_list[1].color = RGB_MAKE(color1, color2, color3);
	vertex_list[1].specular = RGB_MAKE(0,0,0);
	
	vertex_list[2].sx = screenPoints[inVIndex2].x;
	vertex_list[2].sy = screenPoints[inVIndex2].y;
	vertex_list[2].sz = screenPoints[inVIndex2].z;
	vertex_list[2].rhw = screenPoints[inVIndex2].invW;
	vertex_list[2].tu = textureCoords[inVIndex2].u;
	vertex_list[2].tv = textureCoords[inVIndex2].v;
	vertex_list[2].color = RGB_MAKE(color1, color2, color3);
	vertex_list[2].specular = RGB_MAKE(0,0,0);
	
	// draw the triangle
	result = 
		IDirect3DDevice2_DrawPrimitive(
			drawContextPrivate->d3d_device2,
			D3DPT_TRIANGLELIST,
			D3DVT_TLVERTEX,
			vertex_list,
			3,
			D3DDP_WAIT);
}

// ----------------------------------------------------------------------
void
MDrDrawContext_Method_TriTextureSplit(
	M3tDrawContext*	inDrawContext,
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1,
	UUtUns16		inVIndex2,
	UUtUns16		inBaseUVIndex0,
	UUtUns16		inBaseUVIndex1,
	UUtUns16		inBaseUVIndex2,
	UUtUns16		inLightUVIndex0,
	UUtUns16		inLightUVIndex1,
	UUtUns16		inLightUVIndex2)
{
	HRESULT						result;
	MDtDrawContextPrivate		*drawContextPrivate;
	D3DTLVERTEX					vertex_list[3];
	M3tPointScreen				*screenPoints;
	M3tTextureCoord				*textureCoords;
	UUtUns8						color1, color2, color3;
	
	color1 = color2 = color3 = 255;
	
	// get access to the data
	drawContextPrivate =
		(MDtDrawContextPrivate*)inDrawContext->privateContext;
	if (drawContextPrivate == NULL)
		return;
	screenPoints =
		(M3tPointScreen*)drawContextPrivate->statePtr[
			M3cDrawStatePtrType_ScreenPointArray];
	textureCoords =
		(M3tTextureCoord*)drawContextPrivate->statePtr[
			M3cDrawStatePtrType_BaseTextureMap];

	// tell Direct3D to use the texture
	MDrUseTexture(
		drawContextPrivate,
		drawContextPrivate->statePtr[M3cDrawStatePtrType_BaseTextureMap]);
	
	// set up the vertex_list
	vertex_list[0].sx = screenPoints[inVIndex0].x;
	vertex_list[0].sy = screenPoints[inVIndex0].y;
	vertex_list[0].sz = screenPoints[inVIndex0].z;
	vertex_list[0].rhw = screenPoints[inVIndex0].invW;
	vertex_list[0].tu = textureCoords[inBaseUVIndex0].u;
	vertex_list[0].tv = textureCoords[inBaseUVIndex0].v;
	vertex_list[0].color = RGB_MAKE(color1, color2, color3);
	vertex_list[0].specular = RGB_MAKE(0,0,0);
	
	vertex_list[1].sx = screenPoints[inVIndex1].x;
	vertex_list[1].sy = screenPoints[inVIndex1].y;
	vertex_list[1].sz = screenPoints[inVIndex1].z;
	vertex_list[1].rhw = screenPoints[inVIndex1].invW;
	vertex_list[1].tu = textureCoords[inBaseUVIndex1].u;
	vertex_list[1].tv = textureCoords[inBaseUVIndex1].v;
	vertex_list[1].color = RGB_MAKE(color1, color2, color3);
	vertex_list[1].specular = RGB_MAKE(0,0,0);
	
	vertex_list[2].sx = screenPoints[inVIndex2].x;
	vertex_list[2].sy = screenPoints[inVIndex2].y;
	vertex_list[2].sz = screenPoints[inVIndex2].z;
	vertex_list[2].rhw = screenPoints[inVIndex2].invW;
	vertex_list[2].tu = textureCoords[inBaseUVIndex2].u;
	vertex_list[2].tv = textureCoords[inBaseUVIndex2].v;
	vertex_list[2].color = RGB_MAKE(color1, color2, color3);
	vertex_list[2].specular = RGB_MAKE(0,0,0);
	
	// draw the triangle
	result = 
		IDirect3DDevice2_DrawPrimitive(
			drawContextPrivate->d3d_device2,
			D3DPT_TRIANGLELIST,
			D3DVT_TLVERTEX,
			vertex_list,
			3,
			D3DDP_WAIT);
}