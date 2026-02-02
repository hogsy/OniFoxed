/*
	FILE:	MS_Geom_Transform_AltiVec.h

	AUTHOR:	Brent H. Pease

	CREATED: May 21, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/
#ifndef MS_GEOM_TRANFORM_ALTIVEC_H
#define MS_GEOM_TRANFORM_ALTIVEC_H

void
MSrTransform_Geom_PointListToScreen_AltiVec(
	M3tGeometry*			inGeometry,
	M3tPointScreen			*outResultScreenPoints);

void
MSrTransform_Geom_PointListToScreen_ActiveVertices_AltiVec(
	M3tGeometry*			inGeometry,
	UUtUns32*				inActiveVerticesBV,
	M3tPointScreen			*outResultScreenPoints);

void
MSrTransform_Geom_PointListToFrustumScreen_AltiVec(
	M3tGeometry*			inGeometry,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList);

void
MSrTransform_Geom_PointListLocalToFrustumScreen_ActiveVertices_AltiVec(
	M3tGeometry*			inGeometry,
	const UUtUns32*			inActiveVerticesBV,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList);

void
MSrTransform_Geom_PointListAndVertexNormalToWorld_AltiVec(
	M3tGeometry*			inGeometry,
	M3tPoint3D				*outResultWorldPoints,
	M3tVector3D				*outResultWorldVertexNormals);

void
MSrTransform_Geom_PointListAndVertexNormalToWorldComputeViewVector_AltiVec(
	M3tGeometry*			inGeometry,
	M3tPoint3D				*outResultWorldPoints,
	M3tVector3D				*outResultWorldViewVectors,
	M3tVector3D				*outResultWorldVertexNormals);

void
MSrTransform_Geom_FaceNormalToWorld_AltiVec(
	M3tGeometry*			inGeometry,
	M3tVector3D				*outResultWorldTriNormals);

void
MSrTransform_EnvPointListToFrustumScreen_ActiveVertices_AltiVec(
	const UUtUns32*			inActiveVerticesBV,
	M3tPoint4D				*outFrustumPoints,
	M3tPointScreen			*outScreenPoints,
	UUtUns8					*outClipCodeList);

void
MSrBackface_Remove_AltiVec(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	M3tVector3D*			inWorldViewVectors,
	M3tVector3D*			inWorldTriNormals,
	UUtUns32*				outActiveTrisBV,
	UUtUns32*				outActiveVerticesBV);

#endif /* MS_GEOM_TRANFORM_ALTIVEC_H */
