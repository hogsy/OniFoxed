/*
	FILE:	MS_GC_Method_AltiVec.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Aug. 4, 1999
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997

*/
#if 0

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"

#include "Motoko_Manager.h"

#include "MS_GC_Method_AltiVec.h"
#include "MS_GC_Private.h"
#include "MS_GC_Method_Geometry.h"
#include "MS_GC_Method_State.h"
#include "MS_Geom_Clip.h"
#include "MS_Geom_Shade_AltiVec.h"
#include "MS_Geom_Transform.h"
#include "MS_Geom_Transform_AltiVec.h"

#if UUmSIMD == UUmSIMD_AltiVec


static void
MSiAltiVec_Geometry_Draw_ClipAccept(
	M3tGeometry*		inGeometryObject)
{
	UUtInt16				numPoints;
	M3tGeometry_Private*	geomPrivate = (M3tGeometry_Private*)TMrInstance_GetDynamicData(inGeometryObject);
	M3tPointScreen*			screenPoints;
	M3tVector3D*			worldVertexNormals;
	UUtUns32*				screenVertexShades;
	UUtUns16				i;
	UUtUns16				index0, index1, index2, index3;
	
	UUmAssert(geomPrivate->simdCacheEntryIndex != 0xFFFF);
	UUmAssertReadPtr(geomPrivate->pointSIMD, sizeof(float));
	UUmAssertReadPtr(geomPrivate->vertexNormalSIMD, sizeof(float));
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_Vertex);
	
	numPoints = inGeometryObject->pointArray->numPoints;
	screenPoints = MSgGeomContextPrivate->objectVertexData.screenPoints;
	worldVertexNormals = MSgGeomContextPrivate->worldVertexNormals;
	screenVertexShades = (UUtUns32*)MSgGeomContextPrivate->shades_vertex;
	
	// Transform all points to screen space
		MSrTransform_ToScreen_AltiVec(
			numPoints,
			MSgGeomContextPrivate->matrix_localToFrustum,
			geomPrivate->pointSIMD,
			screenPoints,
			MSgGeomContextPrivate->scaleX,
			MSgGeomContextPrivate->scaleY);
		
	// Transform all normals to world space
		MSrTransform_Normal_AltiVec(
			numPoints,
			MSgGeomContextPrivate->matrixStackTop,
			geomPrivate->vertexNormalSIMD,
			worldVertexNormals);
	
	// Shade all points
		MSrShade_Vertices_Gouraud_Directional_AltiVec(
			inGeometryObject,
 			(float*)worldVertexNormals,
			screenVertexShades);
	
	// draw the tris and quads
		if(inGeometryObject->triArray != NULL)
		{
			M3tTri*					curTri;
			UUtUns16				numTris;

			numTris = inGeometryObject->triArray->numTris;

			curTri = inGeometryObject->triArray->tris;
			
			for(i = 0; i < numTris; i++)
			{
				index0 = curTri->indices[0];
				index1 = curTri->indices[1];
				index2 = curTri->indices[2];
				
				M3rDraw_Triangle(
					curTri);

					curTri++;
			}
		}
		
		if(inGeometryObject->quadArray != NULL)
		{
			M3tQuad*				curQuad;
			UUtUns16				numQuads;

			numQuads = inGeometryObject->quadArray->numQuads;
			
			curQuad = inGeometryObject->quadArray->quads;
			
			for(i = 0; i < numQuads; i++)
			{
				index0 = curQuad->indices[0];
				index1 = curQuad->indices[1];
				index2 = curQuad->indices[2];
				index3 = curQuad->indices[3];
				
				M3rDraw_Quad(
					curQuad);
				
				curQuad++;
			}
		}

}	

UUtError
MSrGeomContext_Method_AltiVec_Geometry_Draw(
	M3tGeometry*		inGeometryObject)
{
	MStClipStatus					clipStatus;
	
	TMrInstance_PrepareForUse(inGeometryObject);
	
	MSrTransform_UpdateMatrices();

	clipStatus =
		MSrTransform_DetermineClipStatus(
			&inGeometryObject->boundingBox,
			MSgGeomContextPrivate->matrix_localToFrustum);
	
	if(clipStatus == MScClipStatus_TrivialReject)
	{
		return UUcError_None;
	}
	
	M3rDraw_State_Push();
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_NumRealVertices,
		inGeometryObject->pointArray->numPoints);
		
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenPointArray,
		MSgGeomContextPrivate->objectVertexData.screenPoints);
		
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_ScreenShadeArray_DC,
		MSgGeomContextPrivate->shades_vertex);

	if(MSgGeomContextPrivate->stateInt[M3cGeomStateIntType_Appearance] == M3cGeomState_Appearance_Texture)
	{
		UUmAssert(inGeometryObject->baseMap != NULL);

		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_TextureCoordArray,
			inGeometryObject->texCoordArray->textureCoords);
		
		MSgGeomContextPrivate->objectVertexData.textureCoords = inGeometryObject->texCoordArray->textureCoords;
		
		UUmAssert(inGeometryObject->texCoordArray->numTextureCoords == inGeometryObject->pointArray->numPoints);
		
		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_BaseTextureMap,
			inGeometryObject->baseMap);
	}
	else
	{

	}

	M3rDraw_State_Commit();

	if(clipStatus == MScClipStatus_TrivialAccept)
	{
		MSiAltiVec_Geometry_Draw_ClipAccept(inGeometryObject);
	}
	else
	{
	
	}
	
	M3rDraw_State_Pop();

	return UUcError_None;
}


#endif

#endif
