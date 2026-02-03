/*
	FILE:	OG_GC_Method_Geometry.h

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE:

	Copyright 1997

*/
#ifndef OG_GC_METHOD_GEOMETRY_H
#define OG_GC_METHOD_GEOMETRY_H

UUtError
OGrGeomContext_Method_Geometry_BoundingBox_Draw(
	UUtUns32			inRGB555,
	M3tGeometry*		inGeometryObject);

UUtError
OGrGeomContext_Method_Sprite_Draw(
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
OGrGeomContext_Method_Contrail_Draw(
			M3tTextureMap*		inTextureMap,
			float				inV0,
			float				inV1,
			M3tContrailData*	inPoint0,
			M3tContrailData*	inPoint1);

UUtError
OGrGeomContext_Method_Geometry_Draw(
	M3tGeometry*		inGeometryObject);

UUtError
OGrGeomContext_Method_Geometry_PolyDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade);

UUtError
OGrGeomContext_Method_Geometry_LineDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade);

UUtError
OGrGeomContext_Method_Geometry_PointDraw(
	UUtUns32			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns32			inShade);

UUtBool
OGrGeomContext_Method_PointVisible(
	M3tPoint3D*			inPoint,
	float				inTolerance);


#endif /* OG_GC_METHOD_GEOMETRY_H */
