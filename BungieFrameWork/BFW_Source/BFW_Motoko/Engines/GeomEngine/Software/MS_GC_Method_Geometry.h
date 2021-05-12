/*
	FILE:	MS_GC_Method_Geometry.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 19, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MS_GC_METHOD_GEOMETRY_H
#define MS_GC_METHOD_GEOMETRY_H

UUtError
MSrGeomContext_Method_Geometry_BoundingBox_Draw(
	UUtUns32			inShade,
	M3tGeometry*		inGeometryObject);

UUtError
MSrGeomContext_Method_Sprite_Draw(
	M3tTextureMap*		inTextureMap,
	M3tPoint3D*			inPoint,
	float				inHorizSize,
	float				inVertSize,
	UUtUns32			inShade,
	UUtUns16			inAlpha,
	float				inRotation,
	M3tVector3D*		inDirection,
	M3tMatrix3x3*		inOrientation,
	float				inXOffset,
	float				inXShorten,
	float				inXChop);	

UUtError
MSrGeomContext_Method_SpriteArray_Draw(
	M3tSpriteArray		*inSpriteArray );

UUtError
MSrGeomContext_Method_Contrail_Draw(
			M3tTextureMap*		inTextureMap,
			float				inV0,
			float				inV1,
			M3tContrailData*	inPoint0,
			M3tContrailData*	inPoint1);

UUtError
MSrGeomContext_Method_Geometry_Draw(
	M3tGeometry*		inGeometryObject);
			
UUtError
MSrGeomContext_Method_Geometry_PolyDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade);
			
UUtError
MSrGeomContext_Method_Geometry_LineDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade);

UUtError
MSrGeomContext_Method_Geometry_PointDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade);

UUtBool
MSrGeomContext_Method_PointTestVisible(
	M3tPoint3D*			inPoint,
	float				inTolerance);

float
MSrGeomContext_Method_PointTestVisibleScale(
	M3tPoint3D*			inPoint,
	M3tPoint2D*			inTestOffsets,
	UUtUns32			inTestOffsetsCount );

UUtError 
MSrGeomContext_Method_Skybox_Draw( 
	M3tSkyboxData		*inSkybox );

UUtError MSrGeomContext_Method_Skybox_Create(
	M3tSkyboxData		*inSkybox,
	M3tTextureMap**		inTextures );

UUtError MSrGeomContext_Method_Skybox_Destroy(
	M3tSkyboxData		*inSkybox );

UUtError
MSrGeomContext_Method_Decal_Draw(
	M3tDecalHeader*		inDecal,
	UUtUns16			inAlpha);
			

#endif /* MS_GC_METHOD_GEOMETRY_H */
