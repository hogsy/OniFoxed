/*
	FILE:	EM_GC_Method_Geometry.c

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"
#include "BFW_BitVector.h"

#include "EM_GC_Private.h"
#include "EM_GC_Method_Geometry.h"
#include "EM_GC_Method_State.h"

UUtError
EGrGeomContext_Method_Geometry_BoundingBox_Draw(
	UUtUns16			inRGB555,
	M3tGeometry*		inGeometryObject)
{

	return UUcError_None;
}

UUtError
EGrGeomContext_Method_Geometry_Draw(
	M3tGeometry*		inGeometryObject)
{

	return UUcError_None;
}

UUtError
EGrGeomContext_Method_Geometry_PolyDraw(
	UUtUns16			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns16			inShade)
{

	return UUcError_None;
}

UUtError
EGrGeomContext_Method_Geometry_LineDraw(
	UUtUns16			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns16			inShade)
{

	return UUcError_None;
}

UUtError
EGrGeomContext_Method_Geometry_LineDraw2D(
	UUtUns16			inNumPoints,
	M3tPointScreen*		inPoints,
	UUtUns16			inShade)
{

	return UUcError_None;
}

UUtError
EGrGeomContext_Method_Geometry_PointDraw(
	UUtUns16			inNumPoints,
	M3tPoint3D*			inPoints,
	UUtUns16			inShade)
{

	return UUcError_None;
}

UUtError
EGrGeomContext_Method_Sprite_Draw(
	M3tTextureMap*		inTextureMap,
	M3tPoint3D*			inPoint,
	float				inSize,
	UUtUns16			inShade,
	UUtUns16			inAlpha,
	float				inRotation,
	M3tVector3D*		inDirection,
	M3tMatrix3x3*		inOrientation)
{

	return UUcError_None;
}
