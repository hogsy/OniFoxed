#pragma once
/*
	FILE:	BFW_Collision.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: May 10, 1998
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1998

*/
#ifndef BFW_COLLISION_H
#define BFW_COLLISION_H

#include "BFW.h"
#include "BFW_Motoko.h"

#define CLcEdgeFudge 0.1f

typedef tm_enum CLtQuadProjection
{
	CLcProjection_Unknown,
	CLcProjection_XY,
	CLcProjection_XZ,
	CLcProjection_YZ
	
} CLtQuadProjection;

typedef enum CLtCollisionType
{
	CLcCollision_None,
	CLcCollision_Face,
	CLcCollision_Edge
} CLtCollisionType;

UUtBool CLrSphere_Ray(
	const M3tPoint3D *inRayStart,
	const M3tVector3D *inRayDir,
	const M3tBoundingSphere *inSphere,
	float *outT,
	UUtBool *outIsExit);
	
UUtBool CLrQuad_Cylinder(
	const M3tPoint3D			*inPointArray,
	const M3tQuad		*inQuad,
	const M3tBoundingCylinder	*inCylinder);
	
UUtBool
CLrSphere_Line(
	const M3tPoint3D*			inStart,
	const M3tPoint3D*			inEnd,
	const M3tBoundingSphere*	inSphere);

// CLrSphere_Sphere is not 100% accurate yet, examine before use
UUtBool CLrSphere_Sphere(
	const M3tBoundingSphere		*inSphere,
	const M3tVector3D			*inVector,
	const M3tBoundingSphere		*inTargetSphere);

UUtBool
CLrBox_Line(
	const M3tBoundingBox_MinMax*	inBox,
	const M3tPoint3D*				inStartPoint,
	const M3tPoint3D*				inEndPoint);
	
UUtBool
CLrBox_Sphere(
	M3tBoundingBox_MinMax*	inBox,
	M3tBoundingSphere*		inSphere);
	

UUtBool
CLrQuad_PointInQuad(
	CLtQuadProjection			inProjection,
	const M3tPoint3D*			inPointArray,
	const M3tQuad*	inQuad,
	const M3tPoint3D*			inPoint);

UUtBool
CLrLine_Plane(
	M3tPoint3D *inLineA,
	M3tPoint3D *inLineB,
	M3tPlaneEquation *inPlane,
	M3tPoint3D *outIntersection);	// optional
		
UUtBool
CLrQuad_Line(
	CLtQuadProjection			inProjection,
	const M3tPoint3D*			inPointArray,
	const M3tPlaneEquation*		inPlaneArray,
	UUtUns32					inPlaneEquIndex,
	const M3tQuad*	inQuad,
	const M3tPoint3D*			inStartPoint,
	const M3tPoint3D*			inEndPoint,
	M3tPoint3D					*outIntersection);

CLtCollisionType
CLrQuad_Quad(
	CLtQuadProjection			inProjectionA,
	const M3tPoint3D*			inPointArrayA,
	const M3tPlaneEquation*		inPlaneArrayA,		// NULL if plane is unknown
	UUtUns32					inPlaneEquIndexA,	// Ignored if above is NULL
	const M3tQuad*	inQuadA,
	CLtQuadProjection			inProjectionB,
	const M3tPoint3D*			inPointArrayB,
	const M3tPlaneEquation*		inPlaneArrayB,		// NULL if plane is unknown
	UUtUns32					inPlaneEquIndexB,	// Ignored if above is NULL
	const M3tQuad*	inQuadB,
	M3tPoint3D					*outFirstContact,
	M3tPoint3D					*outIntersectionL,	// NULL if you don't care
	M3tPoint3D					*outIntersectionR,	// Ignored if above is null
	M3tPlaneEquation			*outPlaneA,			// return the plane of quad A or NULL if you don't care
	M3tPlaneEquation			*outPlaneB);
	
	
UUtBool
CLrQuad_SphereTrivialReject(
	CLtQuadProjection			inProjection,
	const M3tPoint3D*			inPointArray,
	const M3tPlaneEquation*		inPlaneArray,
	UUtUns32					inPlaneEquIndex,
	const M3tQuad*	inQuad,
	const M3tBoundingSphere*	inSphere);

UUtBool
CLrQuad_Sphere(
	CLtQuadProjection			inProjection,
	const M3tPoint3D*			inPointArray,
	const M3tPlaneEquation*		inPlaneArray,
	UUtUns32					inPlaneEquIndex,
	const M3tQuad*	inQuad,
	const M3tBoundingSphere*	inSphere,
	M3tPoint3D					*outCollisionPoint);	// optional

UUtBool
CLrQuad_Box(
	CLtQuadProjection				inProjection,
	const M3tPoint3D*				inPointArray,
	const M3tPlaneEquation*			inPlaneArray,
	UUtUns32						inPlaneEquIndex,
	const M3tQuad*		inQuad,
	const M3tBoundingBox_MinMax*	inBox);	

void 
CLrPlaneEquationFromQuad(
	const M3tQuad			*inQuad,
	const M3tPoint3D		*inPointArray,
	M3tPlaneEquation		*outPlaneEquation);

CLtQuadProjection
CLrQuad_FindProjection(
	const M3tPoint3D*	inPointArray,
	const M3tQuad*		inQuad);

UUtBool
CLrBox_Point(
	M3tBoundingBox *inBox,
	M3tPoint3D *inPoint);
	
UUtBool
CLrBox_Box(
	M3tBoundingBox *inBoxA,
	M3tBoundingBox *inBoxB);
	
UUtBool CLrHotdog_Sphere(
	M3tBoundingHotdog *inDog,
	M3tBoundingSphere *inSphere);

// CB: added 21 august 2000. intersects the line segment given by (inPoint, inDir) with an axis-aligned
// bounding box. note: does NOT return any collisions that occur beyond t of 1.0.
// outFace is 0 = -X, 1 = +X, 2 = -Y, 3 = +Y, 4 = -Z, 5 = +Z
UUtBool PHrCollision_Box_Ray(const M3tBoundingBox_MinMax *inBox, M3tPoint3D *inPoint, M3tVector3D *inDir,
							 UUtBool inStopAtOneT, float *outT, UUtUns32 *outFace);

void CLrInitialize(void);


#endif /* BFW_COLLISION_H */
