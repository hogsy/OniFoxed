/*
	FILE:	MS_Geom_Shade.h

	AUTHOR:	Brent H. Pease

	CREATED: May 21, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/
#ifndef MS_GEOM_SHADE_H
#define MS_GEOM_SHADE_H

void
MSrShade_Vertices_Gouraud(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	M3tVector3D*			inWorldVertexNormals,
	UUtUns32*				inResultScreenShades);

void
MSrShade_Vertices_GouraudActive(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	M3tVector3D*			inWorldVertexNormals,
	UUtUns32*				inResultScreenShades,
	UUtUns32*				inActiveVerticesBV);

void
MSrShade_Faces_Gouraud(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	UUtUns32				inNumFaces,
	M3tVector3D*			inWorldFaceNormals,
	UUtUns32*				inResultFaceScreenShades);

#endif /* MS_GEOM_SHADE_H */
