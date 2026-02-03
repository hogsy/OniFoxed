/*
	FILE:	MS_Geom_Transform.h

	AUTHOR:	Brent H. Pease

	CREATED: May 21, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/
#ifndef MS_GEOM_TRANSFORM_H
#define MS_GEOM_TRANSFORM_H

#include "BFW_MathLib.h"

MStClipStatus
MSrTransform_PointListToFrustumScreen(
	UUtUns32			inNumVertices,
	const M3tPoint3D*	inPointList,
	M3tPoint4D*			outFrustumPoints,
	M3tPointScreen*		outScreenPoints,
	UUtUns8*			outClipCodeList);

void
MSrTransform_UpdateMatrices(
	void);

void
MSrTransform_Geom_PointListToScreen(
	M3tGeometry*			inGeometry,
	M3tPointScreen			*outResultScreenPoints);

void
MSrTransform_Geom_PointListToScreen_ActiveVertices(
	M3tGeometry*			inGeometry,
	UUtUns32*				inActiveVerticesBV,
	M3tPointScreen			*outResultScreenPoints);

void
MSrTransform_Geom_PointListToFrustumScreen(
	M3tGeometry*			inGeometry,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList);

void
MSrTransform_Geom_PointListLocalToFrustumScreen_ActiveVertices(
	M3tGeometry*			inGeometry,
	const UUtUns32*			inActiveVerticesBV,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList);

void
MSrTransform_Geom_PointListAndVertexNormalToWorld(
	M3tGeometry*			inGeometry,
	M3tPoint3D				*outResultWorldPoints,
	M3tVector3D				*outResultWorldVertexNormals);

void
MSrTransform_Geom_PointListAndVertexNormalToWorldComputeViewVector(
	M3tGeometry*			inGeometry,
	M3tPoint3D				*outResultWorldPoints,
	M3tVector3D				*outResultViewVectors,
	M3tVector3D				*outResultWorldVertexNormals);

void
MSrTransform_Geom_FaceNormalToWorld(
	M3tGeometry*			inGeometry,
	M3tVector3D				*outResultWorldTriNormals);

MStClipStatus
MSrTransform_Geom_BoundingBoxToFrustumScreen(
	M3tGeometry*			inGeometry,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList);

MStClipStatus
MSrTransform_Geom_BoundingBoxClipStatus(
	M3tGeometry*			inGeometry);

void
MSrTransform_EnvPointListToFrustumScreen_ActiveVertices(
	const UUtUns32*			inActiveVerticesBV,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList);

void
MSrBackface_Remove(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	M3tVector3D*			inWorldViewVectors,
	M3tVector3D*			inWorldTriNormals,
	UUtUns32*				outActiveTrisBV,
	UUtUns32*				outActiveVerticesBV);

#endif /* MS_GEOM_TRANSFORM_H */
