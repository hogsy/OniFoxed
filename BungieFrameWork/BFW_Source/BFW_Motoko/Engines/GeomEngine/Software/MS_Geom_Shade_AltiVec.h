/*
	FILE:	MS_Geom_Shade_AltiVec.h

	AUTHOR:	Brent H. Pease

	CREATED: Aug. 4, 1999

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/
#ifndef MS_GEOM_SHADE_ALTIVEC_H
#define MS_GEOM_SHADE_ALTIVEC_H

void
MSrShade_Vertices_Gouraud_Directional_AltiVec(
	M3tGeometry*	inGeometry,
	M3tPoint3D*		inWorldPoints,
	M3tVector3D*	inWorldVertexNormals,
	UUtUns32		*outShades);

void
MSrShade_Vertices_Gouraud_Directional_Active_AltiVec(
	M3tGeometry*	inGeometry,
	M3tPoint3D*		inWorldPoints,
	M3tVector3D*	inWorldVertexNormals,
	UUtUns32		*outShades,
	UUtUns32*		inActiveVerticesBV);

#endif /* MS_GEOM_SHADE_ALTIVEC_H */
