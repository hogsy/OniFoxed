/*
	FILE:	BFW_Collision.c

	AUTHOR:	Brent H. Pease

	CREATED: May 10, 1998

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1998

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Motoko.h"

#include "BFW_Console.h"
#include "BFW_Collision.h"
#include "BFW_MathLib.h"
#include "BFW_Akira.h"
#include "BFW_AppUtilities.h"
#include "BFW_Object.h"
#include "BFW_MathLib.h"
#include "BFW_Timer.h"

#if PERFORMANCE_TIMER
UUtPerformanceTimer *CLgQuad_Line_Timer = NULL;
UUtPerformanceTimer *CLgQuad_PointInQuad_Timer = NULL;
#endif

void CLrInitialize(void)
{
#if PERFORMANCE_TIMER
	CLgQuad_Line_Timer = UUrPerformanceTimer_New("CL_Quad_Line");
	CLgQuad_PointInQuad_Timer = UUrPerformanceTimer_New("CL_Quad_PointInQuad");
#endif

	return;
}

void CLrPlaneEquationFromQuad(
	const M3tQuad			*inQuad,
	const M3tPoint3D		*inPointArray,
	M3tPlaneEquation		*outPlaneEquation)
{
	M3tVector3D normal;

	MUrVector_NormalFromPoints(
		&inPointArray[inQuad->indices[0]],
		&inPointArray[inQuad->indices[1]],
		&inPointArray[inQuad->indices[2]],
		&normal);

	UUmAssert(normal.x >= -1.0f && normal.x <= 1.0f);
	UUmAssert(normal.y >= -1.0f && normal.y <= 1.0f);
	UUmAssert(normal.z >= -1.0f && normal.z <= 1.0f);
	UUmAssert(!AKmNoIntersection(normal));

	outPlaneEquation->a = normal.x;
	outPlaneEquation->b = normal.y;
	outPlaneEquation->c = normal.z;
	outPlaneEquation->d = -(	outPlaneEquation->a * inPointArray[inQuad->indices[0]].x +
								outPlaneEquation->b * inPointArray[inQuad->indices[0]].y +
								outPlaneEquation->c * inPointArray[inQuad->indices[0]].z);

	return;
}

#define CLm2DPointInQuad(ut, vt, u0, v0, u1, v1, u2, v2, u3, v3) \
		(UUmFloat_CompareLE(((u0) - (ut)) * ((v1) - (vt)) - ((v0) - (vt)) * ((u1) - (ut)), 0.0f) &&	\
		UUmFloat_CompareLE(((u1) - (ut)) * ((v2) - (vt)) - ((v1) - (vt)) * ((u2) - (ut)), 0.0f) &&	\
		UUmFloat_CompareLE(((u2) - (ut)) * ((v3) - (vt)) - ((v2) - (vt)) * ((u3) - (ut)), 0.0f) &&	\
		UUmFloat_CompareLE(((u3) - (ut)) * ((v0) - (vt)) - ((v3) - (vt)) * ((u0) - (ut)), 0.0f))

UUtBool CLrSphere_Ray(
	const M3tPoint3D *inRayStart,
	const M3tVector3D *inRayDir,
	const M3tBoundingSphere *inSphere,
	float *outT,
	UUtBool *outIsExit)
{
	float disc, a, b, c, t;
	M3tVector3D relative_pos;

	UUmAssert(inSphere);

	MUmVector_Subtract(relative_pos, *inRayStart, inSphere->center);
	a = MUmVector_GetLengthSquared(*inRayDir);
	b = 2*MUrVector_DotProduct(&relative_pos, inRayDir);
	c = MUmVector_GetLengthSquared(relative_pos) - UUmSQR(inSphere->radius);

	disc = UUmSQR(b) - 4*a*c;
	if (disc < 0)
		return UUcFalse;

	if (c < 0) {
		// we are inside the sphere, use the greater of the two t-values
		t = (-b + MUrSqrt(disc)) / (2*a);
		*outT = t;
		*outIsExit = UUcTrue;
		return UUcTrue;
	} else {
		// we are outside the sphere, use the lesser of the two t-values
		t = (-b - MUrSqrt(disc)) / (2*a);
		if (t >= 0) {
			// return the intersection
			*outT = t;
			*outIsExit = UUcFalse;
			return UUcTrue;
		} else {
			// intersections are behind ray origin
			return UUcFalse;
		}
	}
}

UUtBool
CLrSphere_Line(
	const M3tPoint3D*			inStart,
	const M3tPoint3D*			inEnd,
	const M3tBoundingSphere*	inSphere)
{
	float radiusSquared = UUmSQR(inSphere->radius);
	float distanceSquared;
	float u;

	// ****** TRIVIAL REJECT ? ******

	// step 1: test start
	distanceSquared = 0;
	distanceSquared += UUmSQR(inStart->x - inSphere->center.x);
	distanceSquared += UUmSQR(inStart->y - inSphere->center.y);
	distanceSquared += UUmSQR(inStart->z - inSphere->center.z);

	if (distanceSquared < radiusSquared) {
		return UUcTrue;
	}

	// step 2: test end
	distanceSquared = 0;
	distanceSquared += UUmSQR(inEnd->x - inSphere->center.x);
	distanceSquared += UUmSQR(inEnd->y - inSphere->center.y);
	distanceSquared += UUmSQR(inEnd->z - inSphere->center.z);

	if (distanceSquared < radiusSquared) {
		return UUcTrue;
	}

	// step 3: test rest of the line
	u = MUrLineSegment_ClosestPoint(inStart, inEnd, &inSphere->center);

	if ((u > 0) && (u < 1.f)) {
		M3tPoint3D closestPoint;

		MUrLineSegment_ComputePoint(inStart, inEnd, u, &closestPoint);

		distanceSquared = 0;
		distanceSquared += UUmSQR(closestPoint.x - inSphere->center.x);
		distanceSquared += UUmSQR(closestPoint.y - inSphere->center.y);
		distanceSquared += UUmSQR(closestPoint.z - inSphere->center.z);

		if (distanceSquared < radiusSquared) {
			return UUcTrue;
		}
	}

	return UUcFalse;
}

UUtBool CLrHotdog_Sphere(
	M3tBoundingHotdog *inDog,
	M3tBoundingSphere *inSphere)
{
	// intersects hotdog and point
	float u = MUrLineSegment_ClosestPoint(&inDog->start,&inDog->end,&inSphere->center);
	M3tPoint3D close;

	if (u<0.0f)
	{
		if (MUrPoint_Distance(&inSphere->center,&inDog->start) < inDog->radius+inSphere->radius)
		{
			return UUcTrue;
		}
		return UUcFalse;
	}
	if (u>1.0f)
	{
		if (MUrPoint_Distance(&inSphere->center,&inDog->end) < inDog->radius+inSphere->radius)
		{
			return UUcTrue;
		}
		return UUcFalse;
	}

	MUrLineSegment_ComputePoint(&inDog->start,&inDog->end,u,&close);
	if (MUrPoint_Distance(&close,&inSphere->center) < inSphere->radius+inDog->radius)
	{
		return UUcTrue;
	}

	return UUcFalse;
}

static UUtBool CLrPoint_In_Cylinder(
	const M3tPoint3D			*inPoint,
	const M3tBoundingCylinder	*inCylinder)
{
	UUtBool inside = UUcFalse;

	if ((inPoint->y >= inCylinder->bottom) && (inPoint->y <= inCylinder->top))
	{
		float distance_squared =
			UUmSQR(inPoint->x - inCylinder->circle.center.x) +
			UUmSQR(inPoint->z - inCylinder->circle.center.z);

		if (distance_squared < UUmSQR(inCylinder->circle.radius))
		{
			inside = UUcTrue;
		}
	}

	return inside;
}

static void iCircle_LineSegment_Intersections(
	const M3tPoint3D *inP1,
	const M3tPoint3D *inP2,
	const M3tBoundingCircle *circle,
	UUtUns8 *outCount,
	M3tPoint3D *outIntersections)
{
    // Given a ray defined as:
	// x = x1 + (x2 - x1)*t = x1 + i*t
	// z = z1 + (z2 - z1)*t = z1 + k*t
	//
    //and a sphere defined as:
	//
	//(x - l)**2 + (z - n)**2 = r**2
	//
	//Substituting in gives the quadratic equation:
	//
	//a*t**2 + b*t + c = 0
	//
	//where:
	//
	//a = i**2 + j**2 + k**2
	//b = 2*i*(x1 - l) + 2*k*(z1 - n)
	//c = (x1-l)**2 + (z1-n)**2 - r**2;

	float a,b,c;		// quadratic equation
	float x1,z1;		// parameteric line start
	float i,k;			// parametric line delta
	float l,n;			// center points of the circle
	float r;			// radius of the circle

	UUtUns8 itr;
	UUtUns8 num_parametric_solutions;
	float parametric_solutions[2];

	// line variables
	x1 = inP1->x;
	z1 = inP1->z;

	i = inP2->x - inP1->x;
	k = inP2->z - inP1->z;

	// sphere variables
	l = circle->center.x;
	n = circle->center.z;
	r = circle->radius;

	// clear out count
	*outCount = 0;

	// try trivial reject
	{
		float line_min_x, line_max_x;
		float line_min_z, line_max_z;

		if (inP1->x < inP2->x) {
			line_min_x = inP1->x;
			line_max_x = inP2->x;
		}
		else {
			line_min_x = inP2->x;
			line_max_x = inP1->x;
		}

		if (inP1->z < inP2->z) {
			line_min_z = inP1->z;
			line_max_z = inP2->z;
		}
		else {
			line_min_z = inP2->z;
			line_max_z = inP1->z;
		}

		if (line_min_x > (circle->center.x + r)) {
			goto exit;
		}

		if (line_max_x < (circle->center.x - r)) {
			goto exit;
		}

		if (line_min_z > (circle->center.z + r)) {
			goto exit;
		}

		if (line_max_z < (circle->center.z - r)) {
			goto exit;
		}

	}

	// quadratic equation values
	a = UUmSQR(i) + UUmSQR(k);
	b = 2 * i * (x1 - l) + 2 * k * (z1 - n);
	c = UUmSQR(x1 - l) + UUmSQR(z1 - n) - UUmSQR(r);

	num_parametric_solutions = MUrSolveQuadratic(a, b, c, parametric_solutions);

	// turn the list of parametric line solutions into actual line segment solutions
	for(itr = 0; itr < num_parametric_solutions; itr++) {
		float t = parametric_solutions[itr];
		float y1 = inP1->y;
		float j = inP2->y - inP1->y;

		if ((t >= 0) && (t <= 1))
		{
			outIntersections[*outCount].x = x1 + i * t;
			outIntersections[*outCount].y = y1 + j * t;
			outIntersections[*outCount].z = z1 + k * t;
			*outCount += 1;
		}
	}

exit:
	return;
}


UUtBool CLrQuad_Cylinder(
	const M3tPoint3D			*inPointArray,
	const M3tQuad		*inQuad,
	const M3tBoundingCylinder	*inCylinder)
{
	UUtBool intersect = UUcTrue;
	UUtUns16 itr;
	const M3tPoint3D *quad_3d[4];
	M3tPoint3D circle_center_3d;

	for(itr = 0; itr < 4; itr++) {
		quad_3d[itr] = inPointArray + inQuad->indices[itr];
	}

	// if a point of the quad is in the cylinder, then the quad is in
	for(itr = 0; itr < 4; itr++) {
		if (CLrPoint_In_Cylinder(quad_3d[itr], inCylinder)) {
			goto exit;
		}
	}

	circle_center_3d.x = inCylinder->circle.center.x;
	circle_center_3d.z = inCylinder->circle.center.z;

	// next test to see if the quad surrounds the circle
	if (CLrQuad_PointInQuad(CLcProjection_XZ, inPointArray, inQuad, &circle_center_3d))
	{
		// circle and quad intersect, now is the hard version finding out if the
		// cylinder intersects

		// punt
		goto exit;
	}

	// next test all the points of interesection with the lines
	// and the circle
	for(itr = 0; itr < 4; itr++) {
		UUtUns8 intersection_count;
		M3tPoint3D intersections[2];

		iCircle_LineSegment_Intersections(quad_3d[itr], quad_3d[(itr + 1) % 4], &inCylinder->circle, &intersection_count, intersections);

		switch(intersection_count)
		{
			case 2:
				if ((intersections[1].y >= inCylinder->bottom) &&
					(intersections[1].y <= inCylinder->top))
				{
					goto exit;
				}
			case 1:
				if ((intersections[0].y >= inCylinder->bottom) &&
					(intersections[0].y <= inCylinder->top))
				{
					goto exit;
				}
		}
	}

	intersect = UUcFalse;

exit:
	return intersect;
}


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
	M3tPoint3D					*outFirstContact,	// Point of first contact for edge collisions (NULL if you don't care)
	M3tPoint3D					*outIntersectionL,	// NULL if you don't care
	M3tPoint3D					*outIntersectionR,	// Ignored if above is null
	M3tPlaneEquation			*outPlaneA,			// return the plane of quad A or NULL if you don't care
	M3tPlaneEquation			*outPlaneB)			// return the plane of quad B or NULL if you don't care
{
	// Intersects A with B. The points of intersection returned are the endpoints of the line
	// created by the quad intersection
	// IMPORTANT: Edge cases are not distinguished unless you request the intersection line returned.
	// If you do not request the line of intersection, edges are returned as face collisions. In an edge
	// case, the line of intersection returned is the edge on B that collided. This may not be what you
	// expect, but deal with it. It's what's most useful for physics.

	M3tPlaneEquation planeA,planeB;
	UUtUns32 left,right;
	UUtUns32 foundL=0xFFFFFFFF;
	UUtUns32 foundR=0xFFFFFFFF;
	const M3tPoint3D *L;
	const M3tPoint3D *R;
	M3tPoint3D interL,interR,inter;
	const M3tPoint3D *edgeBL;
	const M3tPoint3D *edgeBR;
	UUtBool wantLine,foundLeft,foundRight;

	wantLine = outIntersectionL ? UUcTrue : UUcFalse;
	foundLeft = foundRight = UUcFalse;

	// Start by finding the planes of the two quads if we don't know them
	if (!inPlaneArrayA)
	{
		CLrPlaneEquationFromQuad(inQuadA, inPointArrayA, &planeA);
	}
	else
	{
		AKmPlaneEqu_GetComponents(inPlaneEquIndexA,(M3tPlaneEquation *)inPlaneArrayA,planeA.a,planeA.b,planeA.c,planeA.d);
	}

	if (!inPlaneArrayB)
	{
		CLrPlaneEquationFromQuad(inQuadB, inPointArrayB, &planeB);
	}
	else
	{
		AKmPlaneEqu_GetComponents(inPlaneEquIndexB,(M3tPlaneEquation *)inPlaneArrayB,planeB.a,planeB.b,planeB.c,planeB.d);
	}

	// Next, check all the edges of A against quad B
	left = 0;
	right = 1;
	do
	{
		UUmAssert(left>=0 && left<4);
		UUmAssert(right>=0 && right<4);
		L = &inPointArrayA[inQuadA->indices[left]];
		R = &inPointArrayA[inQuadA->indices[right]];
		if (CLrQuad_Line(inProjectionB,inPointArrayB,&planeB,0,inQuadB,L,R,&inter))
		{
			if (wantLine)
			{
				if (!AKmNoIntersection(inter))
				{
					if (!foundLeft)
					{
						interL = inter;
						foundLeft = UUcTrue;
						foundL = 0;
					}
					else
					{
						//UUmAssert(!foundRight);
						interR = inter;
						foundRight = UUcTrue;
						foundR = 0;
					}
				}
				//else UUmAssert(!"Failure");
			}
			else
			{
				foundLeft = UUcTrue;
				break;
			}
		}
		left = right;
		right = (right+1) % 4;
	} while (right != 1);

	// If we haven't found enough intersections yet, then check edges of B against A
	if ((wantLine && (!foundLeft || !foundRight)) ||
		(!wantLine && !foundLeft))
	{
		left = 0;
		right = 1;
		do
		{
			UUmAssert(left>=0 && left<4);
			UUmAssert(right>=0 && right<4);
			L = &inPointArrayB[inQuadB->indices[left]];
			R = &inPointArrayB[inQuadB->indices[right]];
			if (CLrQuad_Line(inProjectionA,inPointArrayA,&planeA,0,inQuadA,L,R,&inter))
			{
				if (wantLine)
				{
					if (!AKmNoIntersection(inter))
					{
						if (!foundLeft)
						{
							interL = inter;
							foundLeft = UUcTrue;
							foundL = 1;
						}
						else
						{
							//UUmAssert(!foundRight);
							interR = inter;
							edgeBL = L;
							edgeBR = R;
							foundRight = UUcTrue;
							foundR = 1;
						}
					}
					//else UUmAssert(!"Failure");
				}
				else
				{
					foundLeft = UUcTrue;
					break;
				}
			}
			left = right;
			right = (right+1) % 4;
		} while (right != 1);
	}

	// Return results
	if (foundLeft || foundRight)
	{
		if (wantLine)
		{
			//UUmAssert(foundLeft && foundRight);
			if (!foundLeft || !foundRight)
			{
				// Ambiguous solution - ignore it
				//interR = interL;
				//foundR = foundL+1;
				return CLcCollision_None;
			}
		}

		// Calculate point of initial contact
		/*switch(outEdgeCaseList[v])
		{
			case CLcCollision_Face:
				// Collision point is center of collision line of integral quad
				// Not sure if this is a good idea...
				MUmVector_Add(AKgCollisionList[passes].collisionPoint,outPoints[v][0],outPoints[v][1]);
				MUmVector_Scale(AKgCollisionList[passes].collisionPoint,0.5f);
				break;

			case CLcCollision_Edge:
				// Collision is point of contact of integral quad on destination edge
				AKgCollisionList[passes].collisionPoint = outPoints[v][1];
				break;
		}*/

		if (outPlaneA) *outPlaneA = planeA;
		if (outPlaneB) *outPlaneB = planeB;
		if (foundL == foundR)
		{
			if (outIntersectionL)
			{
				*outIntersectionL = interL;
				*outIntersectionR = interR;
			}
			if (outFirstContact)
			{
				// Collision point is center of intersection line
				// Not sure if this is a good idea...
				MUmVector_Add(*outFirstContact,interL,interR);
				MUmVector_Scale(*outFirstContact,0.5f);
			}
			return CLcCollision_Face;
		}
		else
		{
			if (outIntersectionL)
			{
				*outIntersectionL = *edgeBL;
				*outIntersectionR = *edgeBR;
			}
			if (outFirstContact) *outFirstContact = interR;
			return CLcCollision_Edge;
		}
	}

	return CLcCollision_None;
}

UUtBool
CLrQuad_Line(
	CLtQuadProjection			inProjection,
	const M3tPoint3D*			inPointArray,
	const M3tPlaneEquation*		inPlaneArray,		// NULL if plane is unknown
	UUtUns32					inPlaneEquIndex,	// Ignored if above is NULL
	const M3tQuad*				inQuad,
	const M3tPoint3D*			inStartPoint,
	const M3tPoint3D*			inEndPoint,
	M3tPoint3D					*outIntersection)	// NULL if you don't care
{
	float		Xi, Yi, Zi;
	float		Xf, Yf, Zf;
	float		Xm, Ym, Zm;
	float		denom;
	float		t;
	M3tPoint3D	intersection;
	float		num1;
	M3tPlaneEquation plane;
	UUtBool		intersected = UUcTrue;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(CLgQuad_Line_Timer);
#endif

	// Find the plane of the quad
	if (!inPlaneArray) {
		CLrPlaneEquationFromQuad(inQuad, inPointArray, &plane);
	}
	else {
		AKmPlaneEqu_GetComponents(inPlaneEquIndex,(M3tPlaneEquation *)inPlaneArray,plane.a,plane.b,plane.c,plane.d);
	}

	// Now check for collision
	Xi = inStartPoint->x;
	Yi = inStartPoint->y;
	Zi = inStartPoint->z;

	Xf = inEndPoint->x;
	Yf = inEndPoint->y;
	Zf = inEndPoint->z;

	// if both points are on the plane then the line is on the plane
	if (UUmFloat_CompareEqu(plane.a * Xi + plane.b * Yi + plane.c * Zi + plane.d, 0.0f) &&
		UUmFloat_CompareEqu(plane.a * Xf + plane.b * Yf + plane.c * Zf + plane.d, 0.0f))
	{
		float	Ui0, Ui1, Ui2, Ui3;
		float	Vi0, Vi1, Vi2, Vi3;
		float	Um0, Um1, Um2, Um3;
		float	Vm0, Vm1, Vm2, Vm3;

		float	Uit, Vit;
		float	Umt, Vmt;

		float	tt, tn;

		const M3tPoint3D*	curPoint;

		// XXX - This code could be speed up a bit

		if (inProjection == CLcProjection_Unknown) {
			inProjection = CLrQuad_FindProjection(inPointArray, inQuad);
			if (inProjection == CLcProjection_Unknown) {
				// degenerate quad
				goto no_intersection;
			}
		}

		if (CLrQuad_PointInQuad(inProjection, inPointArray, inQuad, inEndPoint)) {
			intersection = *inEndPoint;
			goto has_intersection;
		}

		// now we need to see if the line intersects with the edges of the quad
		if (CLcProjection_XY == inProjection) {
			curPoint = inPointArray + inQuad->indices[0];
			Ui0 = curPoint->x;
			Vi0 = curPoint->y;

			curPoint = inPointArray + inQuad->indices[1];
			Ui1 = curPoint->x;
			Vi1 = curPoint->y;
			Um0 = (Ui1 - Ui0);
			Vm0 = (Vi1 - Vi0);

			curPoint = inPointArray + inQuad->indices[2];
			Ui2 = curPoint->x;
			Vi2 = curPoint->y;
			Um1 = (Ui2 - Ui1);
			Vm1 = (Vi2 - Vi1);

			curPoint = inPointArray + inQuad->indices[3];
			Ui3 = curPoint->x;
			Vi3 = curPoint->y;
			Um2 = (Ui3 - Ui2);
			Vm2 = (Vi3 - Vi2);

			Um3 = (Ui0 - Ui3);
			Vm3 = (Vi0 - Vi3);

			Uit = Xi;
			Vit = Yi;

			Umt = (Xf - Xi);
			Vmt = (Yf - Yi);
		}
		else if (CLcProjection_XZ == inProjection) {
			curPoint = inPointArray + inQuad->indices[0];
			Ui0 = curPoint->x;
			Vi0 = curPoint->z;

			curPoint = inPointArray + inQuad->indices[1];
			Ui1 = curPoint->x;
			Vi1 = curPoint->z;
			Um0 = (Ui1 - Ui0);
			Vm0 = (Vi1 - Vi0);

			curPoint = inPointArray + inQuad->indices[2];
			Ui2 = curPoint->x;
			Vi2 = curPoint->z;
			Um1 = (Ui2 - Ui1);
			Vm1 = (Vi2 - Vi1);

			curPoint = inPointArray + inQuad->indices[3];
			Ui3 = curPoint->x;
			Vi3 = curPoint->z;
			Um2 = (Ui3 - Ui2);
			Vm2 = (Vi3 - Vi2);

			Um3 = (Ui0 - Ui3);
			Vm3 = (Vi0 - Vi3);

			Uit = Xi;
			Vit = Zi;

			Umt = (Xf - Xi);
			Vmt = (Zf - Zi);
		}
		else {
			curPoint = inPointArray + inQuad->indices[0];
			Ui0 = curPoint->y;
			Vi0 = curPoint->z;

			curPoint = inPointArray + inQuad->indices[1];
			Ui1 = curPoint->y;
			Vi1 = curPoint->z;
			Um0 = (Ui1 - Ui0);
			Vm0 = (Vi1 - Vi0);

			curPoint = inPointArray + inQuad->indices[2];
			Ui2 = curPoint->y;
			Vi2 = curPoint->z;
			Um1 = (Ui2 - Ui1);
			Vm1 = (Vi2 - Vi1);

			curPoint = inPointArray + inQuad->indices[3];
			Ui3 = curPoint->y;
			Vi3 = curPoint->z;
			Um2 = (Ui3 - Ui2);
			Vm2 = (Vi3 - Vi2);

			Um3 = (Ui0 - Ui3);
			Vm3 = (Vi0 - Vi3);

			Uit = Yi;
			Vit = Zi;

			Umt = (Yf - Yi);
			Vmt = (Zf - Zi);
		}

		/*
			Uin + tn * Umn = Uit + tt * Umt
			Vin + tn * Vmn = Vit + tt * Vmt

			tn = (Uit + tt * Umt - Uin) / Umn
			tn = (Vit + tt * Vmt - Vin) / Vmn

			(Uit + tt * Umt - Uin) / Umn = (Vit + tt * Vmt - Vin) / Vmn

			(Uit + tt * Umt - Uin) * Vmn = (Vit + tt * Vmt - Vin) * Umn

			Uit * Vmn + tt * Umt * Vmn - Uin * Vmn = Vit * Umn + tt * Vmt * Umn - Vin * Umn

			tt * Umt * Vmn - tt * Vmt * Umn = Vit * Umn - Vin * Umn - Uit * Vmn + Uin * Vmn

			tt * (Umt * Vmn - Vmt * Umn) = Vit * Umn - Vin * Umn - Uit * Vmn + Uin * Vmn

			tt = (Vit * Umn - Vin * Umn - Uit * Vmn + Uin * Vmn) / (Umt * Vmn - Vmt * Umn)
		*/

		tt = (Vit * Um0 - Vi0 * Um0 - Uit * Vm0 + Ui0 * Vm0) / (Umt * Vm0 - Vmt * Um0);
		if (tt >= 0.0f && tt <= 1.0) {
			if (UUmFloat_CompareEqu(Um0, 0.0f)) {
				tn = (Vit + tt * Vmt - Vi0) / Vm0;
			}
			else {
				tn = (Uit + tt * Umt - Ui0) / Um0;
			}

			if (tn >= 0.0f && tn <= 1.0f) {
				goto calc_plane_intersection;
			}
		}

		tt = (Vit * Um1 - Vi1 * Um1 - Uit * Vm1 + Ui1 * Vm1) / (Umt * Vm1 - Vmt * Um1);
		if (tt >= 0.0f && tt <= 1.0) {
			if (UUmFloat_CompareEqu(Um1, 0.0f)) {
				tn = (Vit + tt * Vmt - Vi1) / Vm1;
			}
			else {
				tn = (Uit + tt * Umt - Ui1) / Um1;
			}

			if (tn >= 0.0f && tn <= 1.0f) {
				goto calc_plane_intersection;
			}
		}

		tt = (Vit * Um2 - Vi2 * Um2 - Uit * Vm2 + Ui2 * Vm2) / (Umt * Vm2 - Vmt * Um2);
		if (tt >= 0.0f && tt <= 1.0) {
			if (UUmFloat_CompareEqu(Um2, 0.0f)) {
				tn = (Vit + tt * Vmt - Vi2) / Vm2;
			}
			else {
				tn = (Uit + tt * Umt - Ui2) / Um2;
			}

			if (tn >= 0.0f && tn <= 1.0f) {
				goto calc_plane_intersection;
			}
		}

		tt = (Vit * Um3 - Vi3 * Um3 - Uit * Vm3 + Ui3 * Vm3) / (Umt * Vm3 - Vmt * Um3);
		if (tt >= 0.0f && tt <= 1.0) {
			if (UUmFloat_CompareEqu(Um3, 0.0f)) {
				tn = (Vit + tt * Vmt - Vi3) / Vm3;
			}
			else {
				tn = (Uit + tt * Umt - Ui3) / Um3;
			}

			if (tn >= 0.0f && tn <= 1.0f) {
				goto calc_plane_intersection;
			}
		}

		goto no_intersection;

calc_plane_intersection:
		Xm = (Xf - Xi);
		Ym = (Yf - Yi);
		Zm = (Zf - Zi);

		intersection.x = Xm * tn + Xi;
		intersection.y = Ym * tn + Yi;
		intersection.z = Zm * tn + Zi;

		goto has_intersection;
	}
	else {
		Xm = (Xf - Xi);
		Ym = (Yf - Yi);
		Zm = (Zf - Zi);

		denom = plane.a * Xm + plane.b * Ym + plane.c * Zm;

		if (UUmFloat_CompareEqu(denom,0.0f)) {
			goto no_intersection;
		}

		num1 = -(plane.a * Xi + plane.b * Yi + plane.c * Zi + plane.d);

		if (UUmFloat_CompareGT((float)fabs(num1),(float)fabs(denom)) ||
			((UUmFloat_CompareLT(denom,0.0f)) && (UUmFloat_CompareGT(num1,0.0f))) ||
			((UUmFloat_CompareGT(denom,0.0f)) && (UUmFloat_CompareLT(num1,0.0f))))
		{
			goto no_intersection;
		}

		t = num1 / denom;

		//UUmAssert(UUmFloat_CompareGESloppy(t,0.0f) && UUmFloat_CompareLESloppy(t,1.0f));

		intersection.x = Xm * t + Xi;
		intersection.y = Ym * t + Yi;
		intersection.z = Zm * t + Zi;

		if (CLrQuad_PointInQuad(inProjection, inPointArray, inQuad, &intersection)) {
			UUmAssert(intersection.x > -10000.0f && intersection.x < 10000.0f);
			goto has_intersection;
		}
	}

no_intersection:
	intersected = UUcFalse;

has_intersection:
	if (intersected && (NULL !=outIntersection)) {
		*outIntersection = intersection;
	}

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(CLgQuad_Line_Timer);
#endif

	return intersected;
}

UUtBool
CLrLine_Plane(
	M3tPoint3D *inLineA,
	M3tPoint3D *inLineB,
	M3tPlaneEquation *inPlane,
	M3tPoint3D *outIntersection)	// optional
{
	float Xm,Ym,Zm,denom,t;
	M3tPoint3D newPoint;

	Xm = (inLineB->x - inLineA->x);
	Ym = (inLineB->y - inLineA->y);
	Zm = (inLineB->z - inLineA->z);

	denom = inPlane->a * Xm +inPlane-> b * Ym + inPlane->c * Zm;

	if (denom == 0.0f)
	{
		return UUcFalse;
	}

	t = -(inPlane->a * inLineA->x + inPlane->b * inLineA->y + inPlane->c * inLineA->z + inPlane->d) / denom;

	if (t < 0.0f || t > 1.0f)
	{
		return UUcFalse;
	}

	newPoint.x = Xm * t + inLineA->x;
	newPoint.y = Ym * t + inLineA->y;
	newPoint.z = Zm * t + inLineA->z;

	if (outIntersection) *outIntersection = newPoint;
	return UUcTrue;
}

UUtBool
CLrQuad_SphereTrivialReject(
	CLtQuadProjection			inProjection,
	const M3tPoint3D*			inPointArray,
	const M3tPlaneEquation*		inPlaneArray,
	UUtUns32					inPlaneEquIndex,
	const M3tQuad*	inQuad,
	const M3tBoundingSphere*	inSphere)
{
	UUtUns16 trivialItr;
	float distanceSquared;
	UUtBool trivialReject;

	M3tPoint3D bestPoint = inPointArray[inQuad->indices[0]];

	for(trivialItr = 1; trivialItr < 4; trivialItr++)
	{
		const M3tPoint3D *curTrivialPoint = inPointArray + inQuad->indices[trivialItr];

		float bestDiffX = inSphere->center.x - bestPoint.x;
		float curDiffX = inSphere->center.x - curTrivialPoint->x;
		float bestDiffY = inSphere->center.y - bestPoint.y;
		float curDiffY = inSphere->center.y - curTrivialPoint->y;
		float bestDiffZ = inSphere->center.z - bestPoint.z;
		float curDiffZ = inSphere->center.z - curTrivialPoint->z;

		// they encompass actual x so set to acutal x
		if (((bestDiffX > 0) && (curDiffX < 0)) || ((bestDiffX < 0) && (curDiffX > 0)))
		{
			bestPoint.x = inSphere->center.x;
		}
		else if (fabs(curDiffX) < fabs(bestDiffX))
		{
			bestPoint.x = curTrivialPoint->x;
		}

		// they encompass actual y so set to acutal y
		if (((bestDiffY > 0) && (curDiffY < 0)) || ((bestDiffY < 0) && (curDiffY > 0)))
		{
			bestPoint.y = inSphere->center.y;
		}
		else if (fabs(curDiffY) < fabs(bestDiffY))
		{
			bestPoint.y = curTrivialPoint->y;
		}

		// they encompass actual z so set to acutal z
		if (((bestDiffZ > 0) && (curDiffZ < 0)) || ((bestDiffZ < 0) && (curDiffZ > 0)))
		{
			bestPoint.z = inSphere->center.z;
		}
		else if (fabs(curDiffZ) < fabs(bestDiffZ))
		{
			bestPoint.z = curTrivialPoint->z;
		}
	}

	distanceSquared =	UUmSQR(bestPoint.x - inSphere->center.x) +
						UUmSQR(bestPoint.y - inSphere->center.y) +
						UUmSQR(bestPoint.z - inSphere->center.z);

	trivialReject = distanceSquared > UUmSQR(inSphere->radius);

	return trivialReject;
}


UUtBool
CLrQuad_Sphere(
	CLtQuadProjection			inProjection,
	const M3tPoint3D*			inPointArray,
	const M3tPlaneEquation*		inPlaneArray,
	UUtUns32					inPlaneEquIndex,
	const M3tQuad*	inQuad,
	const M3tBoundingSphere*	inSphere,
	M3tPoint3D					*outCollisionPoint)	// optional
{
	/*
		XXX - This algorithm is not quite right. We need to test the min distance to
		each edge of the quad also

		Pcs = the point at the sphere center
		dist = distance from Pcs to the plane
		Pop = orthogonal projection of the center onto the plane J

		Jn = plane normal taken from a, b, c of plane equation
		Jd = d component of plane equation

		dist = (Jn . Pcs) + Jd

		Pop = Pcs - dist(Jn)

	*/

	float				dist;
	float				a, b, c, d;
	float				cX, cY, cZ;
	M3tPlaneEquation	planeEqu;
	M3tPoint3D			Pop;
	float				r2, d2;
	float				t;
	UUtUns16			curPointIndex;
	const M3tPoint3D*	curPoint;

	r2 = inSphere->radius;
	r2 *= r2;

	cX = inSphere->center.x;
	cY = inSphere->center.y;
	cZ = inSphere->center.z;

	// Start by finding the plane of the quad if we don't know it
	if (!inPlaneArray)
	{
		CLrPlaneEquationFromQuad(inQuad, inPointArray, &planeEqu);
	}
	else
	{
		AKmPlaneEqu_GetComponents(inPlaneEquIndex,(M3tPlaneEquation *)inPlaneArray,planeEqu.a,planeEqu.b,planeEqu.c,planeEqu.d);
	}

	a = planeEqu.a;
	b = planeEqu.b;
	c = planeEqu.c;
	d = planeEqu.d;

	// First check if we are far away from the plane

		dist = a * cX + b * cY + c * cZ + d;

		if ((float)fabs(dist) > inSphere->radius)
		{
			return UUcFalse;
		}

	// Now check if we are close to any of the corners
		for(curPointIndex = 0; curPointIndex < 4; curPointIndex++)
		{
			curPoint = inPointArray + inQuad->indices[curPointIndex];

			d2 = 0.0f;

			t = curPoint->x - cX;
			t *= t;
			d2 += t;

			t = curPoint->y - cY;
			t *= t;
			d2 += t;

			t = curPoint->z - cZ;
			t *= t;
			d2 += t;

			// if the distance^2 is less then the radius^2 then we are penetrating the sphere
			if (d2 < r2)
			{
				if (outCollisionPoint)
				{
					outCollisionPoint->x = cX - dist * a;
					outCollisionPoint->y = cY - dist * b;
					outCollisionPoint->z = cZ - dist * c;
				}
				return UUcTrue;
			}
		}

	Pop.x = cX - dist * a;
	Pop.y = cY - dist * b;
	Pop.z = cZ - dist * c;

	if (CLrQuad_PointInQuad(inProjection, inPointArray, inQuad, &Pop))
	{
		if (outCollisionPoint) *outCollisionPoint = Pop;
		return UUcTrue;
	}

	// Okay, those all failed. Now comes the expensive part. Check the
	// minimum distance from the sphere center to each of the quad edges
	{
		const M3tPoint3D *startPoint,*endPoint;
		UUtUns16 st=0,ed=1;

		// Intersect each edge with the sphere
		do
		{
			startPoint = &inPointArray[inQuad->indices[st]];
			endPoint = &inPointArray[inQuad->indices[ed]];
			if (CLrSphere_Line(startPoint,endPoint,inSphere))
			{
				if (outCollisionPoint) *outCollisionPoint = Pop;
				return UUcTrue;
			}

			st++;
			ed = (ed+1)%4;
		} while (ed!=1);
	}

	return UUcFalse;
}


#define CLr_Quad_FindProject_HandleCaseMacro(ax,ay,az,bx,by,bz) \
sameX = UUmFloat_CompareEqu(ax, bx); \
sameY = UUmFloat_CompareEqu(ay, by); \
sameZ = UUmFloat_CompareEqu(az, bz); \
if (!(sameX & sameY & sameZ)) { \
not_xy_plane = not_xy_plane | (sameX & sameY);	\
not_xz_plane = not_xz_plane | (sameX & sameZ);	\
not_yz_plane = not_yz_plane | (sameY & sameZ);	\
} \
do {} while (0)

#define CLr_Quad_FindProject_MinMaxMacro(a,b,min,max) \
if (a > b) { max = a; min = b; } \
else { max = b; min = a; } \
do {} while (0)


static CLtQuadProjection
CLrQuad_FindProjection_Slow(
	const M3tPoint3D*	inPointArray,
	const M3tQuad*		inQuad)
{

	UUtBool				xyPlane = UUcTrue;
	UUtBool				xzPlane = UUcTrue;
	UUtBool				yzPlane = UUcTrue;
	UUtUns16			i,j;
	const M3tPoint3D*	curPoint;
	UUtUns16			curPointIndex;
	float				minX, minY, minZ;
	float				maxX, maxY, maxZ;
	float				dX, dY, dZ;


	// Find a legit 2D plane to project quad onto
	// A legit 2D plane does not collapse any of the points into the same point
	// or onto a flat line

	for(i = 0; i < 4; i++)
	{
		for(j = i+1; j < 4; j++)
		{
			const M3tPoint3D*		targetPoint0;
			const M3tPoint3D*		targetPoint1;
			UUtBool sameX, sameY, sameZ;
			UUtUns32 targetPointIndex0, targetPointIndex1;

			if (i == j) {
				continue;
			}

			targetPointIndex0 = inQuad->indices[i];
			targetPointIndex1 = inQuad->indices[j];

			if (targetPointIndex0 == targetPointIndex1) {
				continue;
			}

			targetPoint0 = inPointArray + targetPointIndex0;
			targetPoint1 = inPointArray + targetPointIndex1;

			sameX = UUmFloat_CompareEqu(targetPoint0->x, targetPoint1->x);
			sameY = UUmFloat_CompareEqu(targetPoint0->y, targetPoint1->y);
			sameZ = UUmFloat_CompareEqu(targetPoint0->z, targetPoint1->z);

			if (sameX && sameY && sameZ)
			{
				continue;
			}

			if (sameX && sameY)
				{
					xyPlane = UUcFalse;
				}

			if (sameX && sameZ)
				{
					xzPlane = UUcFalse;
				}

			if (sameY && sameZ)
				{
					yzPlane = UUcFalse;
				}
			}
		}

	minX = minY = minZ = 1e9;
	maxX = maxY = maxZ = -1e9;
	for(curPointIndex = 0; curPointIndex < 4; curPointIndex++)
	{
		curPoint = inPointArray + inQuad->indices[curPointIndex];

		if (minX > curPoint->x) minX = curPoint->x;
		if (maxX < curPoint->x) maxX = curPoint->x;

		if (minY > curPoint->y) minY = curPoint->y;
		if (maxY < curPoint->y) maxY = curPoint->y;

		if (minZ > curPoint->z) minZ = curPoint->z;
		if (maxZ < curPoint->z) maxZ = curPoint->z;

	}
	dX = maxX - minX;
	dY = maxY - minY;
	dZ = maxZ - minZ;

	if (UUmFloat_CompareGT(dX, 0.0f) && UUmFloat_CompareGT(dY, 0.0f) && xyPlane)
	{
		return CLcProjection_XY;
	}
	else if (UUmFloat_CompareGT(dX, 0.0f) && UUmFloat_CompareGT(dZ, 0.0f) && xzPlane)
	{
		return CLcProjection_XZ;
	}
	else if (UUmFloat_CompareGT(dY, 0.0f) && UUmFloat_CompareGT(dZ, 0.0f) && yzPlane)
	{
		return CLcProjection_YZ;
	}
	else
	{
		return CLcProjection_Unknown;
	}
}

static CLtQuadProjection
CLrQuad_FindProjection_Fast(
	const M3tPoint3D*	inPointArray,
	const M3tQuad*		inQuad)
{
	CLtQuadProjection projection;
	UUtBool			not_xy_plane, not_xz_plane, not_yz_plane;
	float			minX0, minY0, minZ0;
	float			maxX0, maxY0, maxZ0;
	float			minX1, minY1, minZ1;
	float			maxX1, maxY1, maxZ1;

	UUtUns32			index0, index1, index2, index3;
	const M3tPoint3D	*point0, *point1, *point2, *point3;
	float			x0,x1,x2,x3;
	float			y0,y1,y2,y3;
	float			z0,z1,z2,z3;
	UUtBool			sameX, sameY, sameZ;

	index0 = inQuad->indices[0];
	index1 = inQuad->indices[1];
	index2 = inQuad->indices[2];
	index3 = inQuad->indices[3];

	point0 = inPointArray + index0;
	point1 = inPointArray + index1;
	point2 = inPointArray + index2;
	point3 = inPointArray + index3;


// piplining vs small register tradeoff ?
#if 0
	x0 = point0->x;	y0 = point0->y;	z0 = point0->z;
	x1 = point1->x;	y1 = point1->y;	z1 = point1->z;
	x2 = point2->x;	y2 = point2->y;	z2 = point2->z;
	x3 = point3->x;	y3 = point3->y;	z3 = point3->z;
#else
	x0 = point0->x;	x1 = point1->x;	x2 = point2->x;	x3 = point3->x;
	y0 = point0->y;	y1 = point1->y;	y2 = point2->y; y3 = point3->y;
	z0 = point0->z;	z1 = point1->z;	z2 = point2->z;	z3 = point3->z;
#endif

	CLr_Quad_FindProject_MinMaxMacro(x0, x1, minX0, maxX0);
	CLr_Quad_FindProject_MinMaxMacro(x2, x3, minX1, maxX1);

	CLr_Quad_FindProject_MinMaxMacro(y0, y1, minY0, maxY0);
	CLr_Quad_FindProject_MinMaxMacro(y2, y3, minY1, maxY1);

	CLr_Quad_FindProject_MinMaxMacro(z0, z1, minZ0, maxZ0);
	CLr_Quad_FindProject_MinMaxMacro(z2, z3, minZ1, maxZ1);

	minX0 = UUmMin(minX0, minX1);
	maxX0 = UUmMax(maxX0, maxX1);

	minY0 = UUmMin(minY0, minY1);
	maxY0 = UUmMax(maxY0, maxY1);

	minZ0 = UUmMin(minZ0, minZ1);
	maxZ0 = UUmMax(maxZ0, maxZ1);

	{
		float dX = maxX0 - minX0;
		float dY = maxY0 - minY0;
		float dZ = maxZ0 - minZ0;

		UUtBool global_same_x = !UUmFloat_CompareGT(dX, 0.f);
		UUtBool global_same_y = !UUmFloat_CompareGT(dY, 0.f);
		UUtBool global_same_z = !UUmFloat_CompareGT(dZ, 0.f);

		// noting in the above code might make this faster, legibility
		// binary or should be slightly faster here
		not_xy_plane = global_same_x | global_same_y;
		not_xz_plane = global_same_x | global_same_z;
		not_yz_plane = global_same_y | global_same_z;
	}


#if 0	// old non binary search method
	minX = UUmMin(minX, x0); maxX = UUmMax(maxX, x0);
	minX = UUmMin(minX, x1); maxX = UUmMax(maxX, x1);
	minX = UUmMin(minX, x2); maxX = UUmMax(maxX, x2);
	minX = UUmMin(minX, x3); maxX = UUmMax(maxX, x3);

	minY = UUmMin(minY, y0); maxY = UUmMax(maxY, y0);
	minY = UUmMin(minY, y1); maxY = UUmMax(maxY, y1);
	minY = UUmMin(minY, y2); maxY = UUmMax(maxY, y2);
	minY = UUmMin(minY, y3); maxY = UUmMax(maxY, y3);

	minZ = UUmMin(minZ, z0); maxZ = UUmMax(maxZ, z0);
	minZ = UUmMin(minZ, z1); maxZ = UUmMax(maxZ, z1);
	minZ = UUmMin(minZ, z2); maxZ = UUmMax(maxZ, z2);
	minZ = UUmMin(minZ, z3); maxZ = UUmMax(maxZ, z3);
#endif

	// (0,1) (0,2) (0,3)
	CLr_Quad_FindProject_HandleCaseMacro(x0, y0, z0, x1, y1, z1);
	CLr_Quad_FindProject_HandleCaseMacro(x0, y0, z0, x2, y2, z2);
	CLr_Quad_FindProject_HandleCaseMacro(x0, y0, z0, x3, y3, z3);

	// (1,2) (1,3)
	CLr_Quad_FindProject_HandleCaseMacro(x1, y1, z1, x2, y2, z2);
	CLr_Quad_FindProject_HandleCaseMacro(x1, y1, z1, x3, y3, z3);

	// (2,3)
	CLr_Quad_FindProject_HandleCaseMacro(x2, y2, z2, x3, y3, z3);

	if (!not_xy_plane)
	{
		projection = CLcProjection_XY;
	}
	else if (!not_xz_plane)
	{
		projection = CLcProjection_XZ;
	}
	else if (!not_yz_plane)
	{
		projection = CLcProjection_YZ;
	}
	else
	{
		projection = CLcProjection_Unknown;
	}

	return projection;
}

CLtQuadProjection
CLrQuad_FindProjection(
	const M3tPoint3D*	inPointArray,
	const M3tQuad*		inQuad)
{
	CLtQuadProjection projection_fast = CLrQuad_FindProjection_Fast(inPointArray, inQuad);

#if defined(DEBUGGING) && DEBUGGING
	{
		CLtQuadProjection projection_slow = CLrQuad_FindProjection_Slow(inPointArray, inQuad);
		UUmAssert(projection_slow == projection_fast);
	}
#endif

	return projection_fast;
}

static UUcInline UUtBool CLrQuad_PointInQuad_Internal(
	CLtQuadProjection	inProjection,
	const M3tPoint3D*	inPointArray,
	const M3tQuad*		inQuad,
	const M3tPoint3D*	inPoint)
{
	float			tr0, tr1, tr2, tr3;
	float			u0, u1, u2, u3, ut;
	float			v0, v1, v2, v3, vt;
	float			w0, w1, w2, w3, wt;

	const M3tPoint3D*		curPoint;

	UUtBool			result;

	if (inProjection == CLcProjection_Unknown)
	{
		inProjection = CLrQuad_FindProjection(inPointArray, inQuad);
		if (inProjection == CLcProjection_Unknown) {
			// degenerate quad
			return UUcFalse;
		}
	}

	if (inProjection == CLcProjection_XY)
	{
		ut = inPoint->x;
		vt = inPoint->y;
		wt = inPoint->z;

		curPoint = inPointArray + inQuad->indices[0];
		u0 = curPoint->x;
		v0 = curPoint->y;
		w0 = curPoint->z;
		curPoint = inPointArray + inQuad->indices[1];
		u1 = curPoint->x;
		v1 = curPoint->y;
		w1 = curPoint->z;
		curPoint = inPointArray + inQuad->indices[2];
		u2 = curPoint->x;
		v2 = curPoint->y;
		w2 = curPoint->z;
		curPoint = inPointArray + inQuad->indices[3];
		u3 = curPoint->x;
		v3 = curPoint->y;
		w3 = curPoint->z;

		if (UUmFloat_CompareLTSloppy(wt, w0) &&
			UUmFloat_CompareLTSloppy(wt, w1) &&
			UUmFloat_CompareLTSloppy(wt, w2) &&
			UUmFloat_CompareLTSloppy(wt, w3)) return UUcFalse;
		if (UUmFloat_CompareGTSloppy(wt, w0) &&
			UUmFloat_CompareGTSloppy(wt, w1) &&
			UUmFloat_CompareGTSloppy(wt, w2) &&
			UUmFloat_CompareGTSloppy(wt, w3)) return UUcFalse;
	}
	else if (inProjection == CLcProjection_XZ)
	{
		ut = inPoint->x;
		vt = inPoint->z;
		wt = inPoint->y;

		curPoint = inPointArray + inQuad->indices[0];
		u0 = curPoint->x;
		v0 = curPoint->z;
		w0 = curPoint->y;
		curPoint = inPointArray + inQuad->indices[1];
		u1 = curPoint->x;
		v1 = curPoint->z;
		w1 = curPoint->y;
		curPoint = inPointArray + inQuad->indices[2];
		u2 = curPoint->x;
		v2 = curPoint->z;
		w2 = curPoint->y;
		curPoint = inPointArray + inQuad->indices[3];
		u3 = curPoint->x;
		v3 = curPoint->z;
		w3 = curPoint->y;

		if (UUmFloat_CompareLTSloppy(wt, w0) &&
			UUmFloat_CompareLTSloppy(wt, w1) &&
			UUmFloat_CompareLTSloppy(wt, w2) &&
			UUmFloat_CompareLTSloppy(wt, w3)) return UUcFalse;
		if (UUmFloat_CompareGTSloppy(wt, w0) &&
			UUmFloat_CompareGTSloppy(wt, w1) &&
			UUmFloat_CompareGTSloppy(wt, w2) &&
			UUmFloat_CompareGTSloppy(wt, w3)) return UUcFalse;
	}
	else
	{
		ut = inPoint->y;
		vt = inPoint->z;
		wt = inPoint->x;

		curPoint = inPointArray + inQuad->indices[0];
		u0 = curPoint->y;
		v0 = curPoint->z;
		w0 = curPoint->x;
		curPoint = inPointArray + inQuad->indices[1];
		u1 = curPoint->y;
		v1 = curPoint->z;
		w1 = curPoint->x;
		curPoint = inPointArray + inQuad->indices[2];
		u2 = curPoint->y;
		v2 = curPoint->z;
		w2 = curPoint->x;
		curPoint = inPointArray + inQuad->indices[3];
		u3 = curPoint->y;
		v3 = curPoint->z;
		w3 = curPoint->x;

		if (UUmFloat_CompareLTSloppy(wt, w0) &&
			UUmFloat_CompareLTSloppy(wt, w1) &&
			UUmFloat_CompareLTSloppy(wt, w2) &&
			UUmFloat_CompareLTSloppy(wt, w3)) return UUcFalse;
		if (UUmFloat_CompareGTSloppy(wt, w0) &&
			UUmFloat_CompareGTSloppy(wt, w1) &&
			UUmFloat_CompareGTSloppy(wt, w2) &&
			UUmFloat_CompareGTSloppy(wt, w3)) return UUcFalse;
	}

	/*
		If the point is in the quad we have something like this

	                     *0
	                    /|\
	                   / | \
	                  /  |  \
	                 /   |   \
	                /    |    \
	              3*-----*t----*1
	                \    |    /
	                 \   |   /
	                  \  |  /
	                   \ | /
	                    \|/
	                     *2
	  either:

	  |t0 x t1| <= 0 &&
	  |t1 x t2| <= 0 &&
	  |t2 x t3| <= 0 &&
	  |t3 x t0| <= 0

	  or

	  |t0 x t1| >= 0 &&
	  |t1 x t2| >= 0 &&
	  |t2 x t3| >= 0 &&
	  |t3 x t0| >= 0

	  (It depends on the orientation of the vertices)

	  if the above conditions are not met the point is outside the quad

	*/

	tr0 = (u0 - ut) * (v1 - vt) - (v0 - vt) * (u1 - ut);
	tr1 = (u1 - ut) * (v2 - vt) - (v1 - vt) * (u2 - ut);
	tr2 = (u2 - ut) * (v3 - vt) - (v2 - vt) * (u3 - ut);
	tr3 = (u3 - ut) * (v0 - vt) - (v3 - vt) * (u0 - ut);

	result =
		UUmFloat_CompareGE(tr0, 0.0f) &&
		UUmFloat_CompareGE(tr1, 0.0f) &&
		UUmFloat_CompareGE(tr2, 0.0f) &&
		UUmFloat_CompareGE(tr3, 0.0f);

	if (result == UUcTrue)
	{
		return UUcTrue;
	}

	result =
		UUmFloat_CompareLE(tr0, 0.0f) &&
		UUmFloat_CompareLE(tr1, 0.0f) &&
		UUmFloat_CompareLE(tr2, 0.0f) &&
		UUmFloat_CompareLE(tr3, 0.0f);

	return result;
}

UUtBool CLrQuad_PointInQuad(
	CLtQuadProjection	inProjection,
	const M3tPoint3D*	inPointArray,
	const M3tQuad*		inQuad,
	const M3tPoint3D*	inPoint)
{
	UUtBool result;

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Enter(CLgQuad_PointInQuad_Timer);
#endif

	result = CLrQuad_PointInQuad_Internal(inProjection, inPointArray, inQuad, inPoint);

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Exit(CLgQuad_PointInQuad_Timer);
#endif

	return result;
}

UUtBool
CLrQuad_Box(
	CLtQuadProjection				inProjection,
	const M3tPoint3D*				inPointArray,
	const M3tPlaneEquation*			inPlaneArray,
	UUtUns32						inPlaneEquIndex,
	const M3tQuad*		inQuad,
	const M3tBoundingBox_MinMax*	inBBox)
{
	const M3tPoint3D*	curPoint;
	M3tPoint3D	startPoint;
	M3tPoint3D	endPoint;
	UUtUns16	curPointIndex;

	// First check if any of the vertices are within the box
		for(curPointIndex = 0; curPointIndex < 4; curPointIndex++)
		{
			curPoint = inPointArray + inQuad->indices[curPointIndex];

			if (UUmFloat_CompareGE(curPoint->x, inBBox->minPoint.x) &&
				UUmFloat_CompareLE(curPoint->x, inBBox->maxPoint.x) &&
				UUmFloat_CompareGE(curPoint->y, inBBox->minPoint.y) &&
				UUmFloat_CompareLE(curPoint->y, inBBox->maxPoint.y) &&
				UUmFloat_CompareGE(curPoint->z, inBBox->minPoint.z) &&
				UUmFloat_CompareLE(curPoint->z, inBBox->maxPoint.z))
			{
				return UUcTrue;
			}
		}

	if (inProjection == CLcProjection_Unknown)
	{
		inProjection = CLrQuad_FindProjection(inPointArray, inQuad);
		if (inProjection == CLcProjection_Unknown) {
			// degenerate quad
			return UUcFalse;
		}
	}

	// Now check for the line intersection of each box edge with the quad

	/*
		0 - min, 1 - max

		v	X	Y	Z
		==	=========
		0	0	0	0
		1	0	0	1
		2	0	1	0
		3	0	1	1
		4	1	0	0
		5	1	0	1
		6	1	1	0
		7	1	1	1

		Edges
		=====
		0 -> 1
		0 -> 2
		0 -> 4
		1 -> 3
		1 -> 5
		2 -> 3
		2 -> 6
		3 -> 7
		4 -> 5
		4 -> 6
		5 -> 7
		6 -> 7
	*/

	// 0 -> 1
		startPoint.x = inBBox->minPoint.x;
		startPoint.y = inBBox->minPoint.y;
		startPoint.z = inBBox->minPoint.z;

		endPoint.x = inBBox->minPoint.x;
		endPoint.y = inBBox->minPoint.y;
		endPoint.z = inBBox->maxPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))

		{
			return UUcTrue;
		}

	// 0 -> 2
		startPoint.x = inBBox->minPoint.x;
		startPoint.y = inBBox->minPoint.y;
		startPoint.z = inBBox->minPoint.z;

		endPoint.x = inBBox->minPoint.x;
		endPoint.y = inBBox->maxPoint.y;
		endPoint.z = inBBox->minPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))

		{
			return UUcTrue;
		}

	// 0 -> 4
		startPoint.x = inBBox->minPoint.x;
		startPoint.y = inBBox->minPoint.y;
		startPoint.z = inBBox->minPoint.z;

		endPoint.x = inBBox->maxPoint.x;
		endPoint.y = inBBox->minPoint.y;
		endPoint.z = inBBox->minPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))

		{
			return UUcTrue;
		}

	// 1 -> 3
		startPoint.x = inBBox->minPoint.x;
		startPoint.y = inBBox->minPoint.y;
		startPoint.z = inBBox->maxPoint.z;

		endPoint.x = inBBox->minPoint.x;
		endPoint.y = inBBox->maxPoint.y;
		endPoint.z = inBBox->maxPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))

		{
			return UUcTrue;
		}

	// 1 -> 5
		startPoint.x = inBBox->minPoint.x;
		startPoint.y = inBBox->minPoint.y;
		startPoint.z = inBBox->maxPoint.z;

		endPoint.x = inBBox->maxPoint.x;
		endPoint.y = inBBox->minPoint.y;
		endPoint.z = inBBox->maxPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))

		{
			return UUcTrue;
		}

	// 2 -> 3
		startPoint.x = inBBox->minPoint.x;
		startPoint.y = inBBox->maxPoint.y;
		startPoint.z = inBBox->minPoint.z;

		endPoint.x = inBBox->minPoint.x;
		endPoint.y = inBBox->maxPoint.y;
		endPoint.z = inBBox->maxPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))

		{
			return UUcTrue;
		}

	// 2 -> 6
		startPoint.x = inBBox->minPoint.x;
		startPoint.y = inBBox->maxPoint.y;
		startPoint.z = inBBox->minPoint.z;

		endPoint.x = inBBox->maxPoint.x;
		endPoint.y = inBBox->maxPoint.y;
		endPoint.z = inBBox->minPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))

		{
			return UUcTrue;
		}

	// 3 -> 7
		startPoint.x = inBBox->minPoint.x;
		startPoint.y = inBBox->maxPoint.y;
		startPoint.z = inBBox->maxPoint.z;

		endPoint.x = inBBox->maxPoint.x;
		endPoint.y = inBBox->maxPoint.y;
		endPoint.z = inBBox->maxPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))

		{
			return UUcTrue;
		}

	// 4 -> 5
		startPoint.x = inBBox->maxPoint.x;
		startPoint.y = inBBox->minPoint.y;
		startPoint.z = inBBox->minPoint.z;

		endPoint.x = inBBox->maxPoint.x;
		endPoint.y = inBBox->minPoint.y;
		endPoint.z = inBBox->maxPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))

		{
			return UUcTrue;
		}

	// 4 -> 6
		startPoint.x = inBBox->maxPoint.x;
		startPoint.y = inBBox->minPoint.y;
		startPoint.z = inBBox->minPoint.z;

		endPoint.x = inBBox->maxPoint.x;
		endPoint.y = inBBox->maxPoint.y;
		endPoint.z = inBBox->minPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))

		{
			return UUcTrue;
		}

	// 5 -> 7
		startPoint.x = inBBox->maxPoint.x;
		startPoint.y = inBBox->minPoint.y;
		startPoint.z = inBBox->maxPoint.z;

		endPoint.x = inBBox->maxPoint.x;
		endPoint.y = inBBox->maxPoint.y;
		endPoint.z = inBBox->maxPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))

		{
			return UUcTrue;
		}

	// 6 -> 7
		startPoint.x = inBBox->maxPoint.x;
		startPoint.y = inBBox->maxPoint.y;
		startPoint.z = inBBox->minPoint.z;

		endPoint.x = inBBox->maxPoint.x;
		endPoint.y = inBBox->maxPoint.y;
		endPoint.z = inBBox->maxPoint.z;

		if (CLrQuad_Line(
			inProjection,
			inPointArray,
			inPlaneArray,
			inPlaneEquIndex,
			inQuad,
			&startPoint,
			&endPoint,
			NULL))
		{
			return UUcTrue;
		}

	// now we need to loop through each edge of the quad to see if it penetrates the box
		for(curPointIndex = 0; curPointIndex < 4; curPointIndex++)
		{
			if (CLrBox_Line(
				inBBox,
				inPointArray + inQuad->indices[curPointIndex],
				inPointArray + inQuad->indices[(curPointIndex + 1) % 4]) == UUcTrue)
			{
				return UUcTrue;
			}

		}

	return UUcFalse;
}

UUtBool
CLrBox_Point(
	M3tBoundingBox *inBox,
	M3tPoint3D *inPoint)
{
	if (inPoint->x>=inBox->localPoints[0].x &&
		inPoint->x<=inBox->localPoints[1].x &&
		inPoint->y>=inBox->localPoints[0].y &&
		inPoint->y<=inBox->localPoints[4].y &&
		inPoint->z>=inBox->localPoints[2].z &&
		inPoint->z<=inBox->localPoints[4].z)
	{
		return UUcTrue;
	}

	return UUcFalse;
}

UUtBool
CLrBox_Box(
	M3tBoundingBox *inBoxA,
	M3tBoundingBox *inBoxB)
{
	// Lame intersection, not totally correct. For debugging of same size boxes only.
	UUtUns16 indexA;

	for (indexA=0; indexA<8; indexA++)
	{
		if (CLrBox_Point(inBoxB,&inBoxA->localPoints[indexA])) UUmAssert(0);//return UUcTrue;
	}
	for (indexA=0; indexA<8; indexA++)
	{
		if (CLrBox_Point(inBoxA,&inBoxB->localPoints[indexA])) UUmAssert(0);//return UUcTrue;
	}

	return UUcFalse;
}

static UUtBool
CLrBox_Line_Slower(
	const M3tBoundingBox_MinMax*	inBox,
	const M3tPoint3D*				inStartPoint,
	const M3tPoint3D*				inEndPoint)
{
	float		Xi, Yi, Zi;
	float		Xm, Ym, Zm;
	float		Xmi, Ymi, Zmi;

	float		Xf, Yf, Zf;

	float		ut, vt;

	float		minX, minY, minZ;
	float		maxX, maxY, maxZ;
	float		t;

	//return UUcTrue;

	minX = inBox->minPoint.x;
	minY = inBox->minPoint.y;
	minZ = inBox->minPoint.z;

	maxX = inBox->maxPoint.x;
	maxY = inBox->maxPoint.y;
	maxZ = inBox->maxPoint.z;

	Xi = inStartPoint->x;
	Yi = inStartPoint->y;
	Zi = inStartPoint->z;

	// If either of the points are in the box then duh.
		if (minX <= Xi && Xi <= maxX &&
			minY <= Yi && Yi <= maxY &&
			minZ <= Zi && Zi <= maxZ)
		{
			return UUcTrue;
		}

		Xf = inEndPoint->x;
		Yf = inEndPoint->y;
		Zf = inEndPoint->z;

		if (minX <= Xf && Xf <= maxX &&
			minY <= Yf && Yf <= maxY &&
			minZ <= Zf && Zf <= maxZ)
		{
			return UUcTrue;
		}

	// Check for trivial reject
		if (Xi < minX && Xf < minX ||
			Yi < minY && Yf < minY ||
			Zi < minZ && Zf < minZ ||
			Xi > maxX && Xf > maxX ||
			Yi > maxY && Yf > maxY ||
			Zi > maxZ && Zf > maxZ)
		{
			return UUcFalse;
		}

	Xm = (inEndPoint->x - Xi);
	Ym = (inEndPoint->y - Yi);
	Zm = (inEndPoint->z - Zi);

	/*
		0 - min, 1 - max

		v	X	Y	Z
		==	=========
		0	0	0	0
		1	0	0	1
		2	0	1	0
		3	0	1	1
		4	1	0	0
		5	1	0	1
		6	1	1	0
		7	1	1	1

		      Y
		      ^
		      |
		      |
		      |
		      |
		      *-------> X
		     /
		    /
		   /
		 Z<
		     2*-------------*6
		     /|            /|
		    / |           / |
		   /  |          /  |
		  /   |         /   |
		3*----+--------*7   |
		 |    '        |    |
		 |   0*- - - - +----*4
		 |   '         |   /
		 |  '          |  /
		 | '           | /
		 |'            |/
		1*-------------*5

		quads		plane equ
		=====		=======
		0 1 3 2		-1, 0, 0, 0.x
		4 6 7 5		1, 0, 0, -4.x
		0 4 5 1		0, -1, 0, 0.y
		2 3 7 6		0, 1, 0, -2.y
		0 2 6 4		0, 0, -1, 0.z
		1 5 7 3		0, 0, 1, -1.z
	*/

	if (Xm != 0.0f)
	{
		// Quad 0 1 3 2
			Xmi = 1.f / Xm;
			// t = -(-1 * Xi + minX) / -Xm
			t = (minX - Xi) * Xmi;
			if (t >= 0.0f && t <= 1.0f)
			{
				ut = Ym * t + Yi;
				vt = Zm * t + Zi;

				if (minY <= ut && ut <= maxY &&
					minZ <= vt && vt <= maxZ)
				{
					UUmAssert((minY - Yi) * Xm <= Ym * (minX - Xi));
					UUmAssert(Ym * (minX - Xi) <= (maxY - Yi) * Xm);

					UUmAssert((minZ - Zi) * Xm <= Zm * (minX - Xi));
					UUmAssert(Zm * (minX - Xi) <= (maxZ - Zi) * Xm);

					return UUcTrue;
				}
			}

		// Quad 4 6 7 5

			// t = -(1 * Xi - maxX) / Xm
			t = (maxX - Xi) * Xmi;
			if (t >= 0.0f && t <= 1.0f)
			{
				ut = Ym * t + Yi;
				vt = Zm * t + Zi;

				if (minY <= ut && ut <= maxY &&
					minZ <= vt && vt <= maxZ)
				{
					UUmAssert((minY - Yi) * Xm <= Ym * (maxX - Xi));
					UUmAssert(Ym * (maxX - Xi) <= (maxY - Yi) * Xm);

					UUmAssert((minZ - Zi) * Xm <= Zm * (maxX - Xi));
					UUmAssert(Zm * (maxX - Xi) <= (maxZ - Zi) * Xm);

					return UUcTrue;
				}
			}
	}

	if (Ym != 0.0f)
	{
		// Quad 0 4 5 1

			Ymi = 1.f / Ym;
			// t = -(-1 * Yi + minY) / -Ym
			t = (minY - Yi) * Ymi;
			if (t >= 0.0f && t <= 1.0f)
			{
				ut = Xm * t + Xi;
				vt = Zm * t + Zi;

				if (minX <= ut && ut <= maxX &&
					minZ <= vt && vt <= maxZ)
				{
					if (Ym > 0.0f)
					{
						UUmAssert((minX - Xi) * Ym <= Xm * (minY - Yi));
						UUmAssert(Xm * (minY - Yi) <= (maxX - Xi) * Ym);

						UUmAssert((minZ - Zi) * Ym <= Zm * (minY - Yi));
						UUmAssert(Zm * (minY - Yi) <= (maxZ - Zi) * Ym);
					}
					else
					{
						UUmAssert((minX - Xi) * Ym >= Xm * (minY - Yi));
						UUmAssert(Xm * (minY - Yi) >= (maxX - Xi) * Ym);

						UUmAssert((minZ - Zi) * Ym >= Zm * (minY - Yi));
						UUmAssert(Zm * (minY - Yi) >= (maxZ - Zi) * Ym);
					}

					return UUcTrue;
				}
			}

		// Quad 2 3 7 6

			// t = -(1 * Xi - maxX) / Xm
			t = (maxY - Yi) * Ymi;
			if (t >= 0.0f && t <= 1.0f)
			{
				ut = Xm * t + Xi;
				vt = Zm * t + Zi;

				if (minX <= ut && ut <= maxX &&
					minZ <= vt && vt <= maxZ)
				{
					return UUcTrue;
				}
			}
	}

	if (Zm != 0.0f)
	{
		// Quad 0 2 6 4

			Zmi = 1.f / Zm;
			// t = -(-1 * Zi + minZ) / -Zm
			t = (minZ - Zi) * Zmi;
			if (t >= 0.0f && t <= 1.0f)
			{
				ut = Xm * t + Xi;
				vt = Ym * t + Yi;

				if (minX <= ut && ut <= maxX &&
					minY <= vt && vt <= maxY)
				{
					return UUcTrue;
				}
			}

		// Quad 1 5 7 3

			// t = -(1 * Zi - maxZ) / Zm
			t = (maxZ - Zi) * Zmi;
			if (t >= 0.0f && t <= 1.0f)
			{
				ut = Xm * t + Xi;
				vt = Ym * t + Yi;

				if (minX <= ut && ut <= maxX &&
					minY <= vt && vt <= maxY)
				{
					return UUcTrue;
				}
			}
	}

	return UUcFalse;
}

UUtBool
CLrBox_Line(
	const M3tBoundingBox_MinMax*	inBox,
	const M3tPoint3D*				inStartPoint,
	const M3tPoint3D*				inEndPoint)
{
	float		Xi, Yi, Zi;
	float		Xm, Ym, Zm;

	float		Xf, Yf, Zf;

	float		minX, minY, minZ;
	float		maxX, maxY, maxZ;
	float		t, t2;

	//return UUcTrue;

	minX = inBox->minPoint.x;
	minY = inBox->minPoint.y;
	minZ = inBox->minPoint.z;

	maxX = inBox->maxPoint.x;
	maxY = inBox->maxPoint.y;
	maxZ = inBox->maxPoint.z;

	Xi = inStartPoint->x;
	Yi = inStartPoint->y;
	Zi = inStartPoint->z;

	// If either of the points are in the box then duh.
		if (minX <= Xi && Xi <= maxX &&
			minY <= Yi && Yi <= maxY &&
			minZ <= Zi && Zi <= maxZ)
		{
			return UUcTrue;
		}

		Xf = inEndPoint->x;
		Yf = inEndPoint->y;
		Zf = inEndPoint->z;

		if (minX <= Xf && Xf <= maxX &&
			minY <= Yf && Yf <= maxY &&
			minZ <= Zf && Zf <= maxZ)
		{
			return UUcTrue;
		}

	// Check for trivial reject
		if (Xi < minX && Xf < minX ||
			Yi < minY && Yf < minY ||
			Zi < minZ && Zf < minZ ||
			Xi > maxX && Xf > maxX ||
			Yi > maxY && Yf > maxY ||
			Zi > maxZ && Zf > maxZ)
		{
			return UUcFalse;
		}

	/*
		The following crap would take a long time to explain
		so you will just have to take my word for it that its
		completely broken and you have no hope of fixing it.
	*/

	Xm = (Xf - Xi);
	Ym = (Yf - Yi);
	Zm = (Zf - Zi);

	minY -= Yi;
	maxY -= Yi;

	minX -= Xi;
	maxX -= Xi;

	minZ -= Zi;
	maxZ -= Zi;

	if (Xm > 0.0f)
	{
		t = Ym * minX;
		t2 = Zm * minX;

		if (minY * Xm <= t && t <= maxY * Xm &&
			minZ * Xm <= t2 && t2 <= maxZ * Xm)
		{
			return UUcTrue;
		}

		t = Ym * maxX;
		t2 = Zm * maxX;

		if (minY * Xm <= t && t <= maxY * Xm &&
			minZ * Xm <= t2 && t2 <= maxZ * Xm)
		{
			return UUcTrue;
		}
	}
	else if (Xm < 0.0f)
	{
		t = Ym * minX;
		t2 = Zm * minX;

		if (minY * Xm >= t && t >= maxY * Xm &&
			minZ * Xm >= t2 && t2 >= maxZ * Xm)
		{
			return UUcTrue;
		}

		t = Ym * maxX;
		t2 = Zm * maxX;

		if (minY * Xm >= t && t >= maxY * Xm &&
			minZ * Xm >= t2 && t2 >= maxZ * Xm)
		{
			return UUcTrue;
		}
	}

	if (Ym > 0.0f)
	{
		t = Xm * minY;
		t2 = Zm * minY;

		if (minX * Ym <= t && t <= maxX * Ym &&
			minZ * Ym <= t2 && t2 <= maxZ * Ym)
		{
			return UUcTrue;
		}

		t = Xm * maxY;
		t2 = Zm * maxY;

		if (minX * Ym <= t && t <= maxX * Ym &&
			minZ * Ym <= t2 && t2 <= maxZ * Ym)
		{
			return UUcTrue;
		}
	}
	else if (Ym < 0.0f)
	{
		t = Xm * minY;
		t2 = Zm * minY;

		if (minX * Ym >= t && t >= maxX * Ym &&
			minZ * Ym >= t2 && t2 >= maxZ * Ym)
		{
			return UUcTrue;
		}

		t = Xm * maxY;
		t2 = Zm * maxY;

		if (minX * Ym >= t && t >= maxX * Ym &&
			minZ * Ym >= t2 && t2 >= maxZ * Ym)
		{
			return UUcTrue;
		}
	}

	if (Zm > 0.0f)
	{
		t = Xm * minZ;
		t2 = Ym * minZ;

		if (minX * Zm <= t && t <= maxX * Zm &&
			minY * Zm <= t2 && t2 <= maxY * Zm)
		{
			return UUcTrue;
		}

		t = Xm * maxZ;
		t2 = Ym * maxZ;

		if (minX * Zm <= t && t <= maxX * Zm &&
			minY * Zm <= t2 && t2 <= maxY * Zm)
		{
			return UUcTrue;
		}
	}
	else if (Zm < 0.0f)
	{
		t = Xm * minZ;
		t2 = Ym * minZ;

		if (minX * Zm >= t && t >= maxX * Zm &&
			minY * Zm >= t2 && t2 >= maxY * Zm)
		{
			return UUcTrue;
		}

		t = Xm * maxZ;
		t2 = Ym * maxZ;

		if (minX * Zm >= t && t >= maxX * Zm &&
			minY * Zm >= t2 && t2 >= maxY * Zm)
		{
			return UUcTrue;
		}
	}

	return UUcFalse;
}

UUtBool
CLrBox_Sphere(
	M3tBoundingBox_MinMax*	inBox,
	M3tBoundingSphere*		inSphere)
{
	float	r2 = inSphere->radius;
	float	dmin = 0.0f;
	float	t;

	float	cX, cY, cZ;
	float	minX, minY, minZ;
	float	maxX, maxY, maxZ;

	r2 *= r2;

	cX = inSphere->center.x;
	cY = inSphere->center.y;
	cZ = inSphere->center.z;

	minX = inBox->minPoint.x;
	minY = inBox->minPoint.y;
	minZ = inBox->minPoint.z;

	maxX = inBox->maxPoint.x;
	maxY = inBox->maxPoint.y;
	maxZ = inBox->maxPoint.z;

	if (minX <= cX && cX <= maxX &&
		minY <= cY && cY <= maxY &&
		minZ <= cZ && cZ <= maxZ)
	{
		return UUcTrue;
	}

	// Don't ask me - I got it from Gems I

	if (cX < minX)
	{
		t = cX - minX;
		t *= t;
		dmin += t;
	}
	else if (cX > maxX)
	{
		t = cX - maxX;
		t *= t;
		dmin += t;
	}

	if (cY < minY)
	{
		t = cY - minY;
		t *= t;
		dmin += t;
	}
	else if (cY > maxY)
	{
		t = cY - maxY;
		t *= t;
		dmin += t;
	}

	if (cZ < minZ)
	{
		t = cZ - minZ;
		t *= t;
		dmin += t;
	}
	else if (cZ > maxZ)
	{
		t = cZ - maxZ;
		t *= t;
		dmin += t;
	}

	return (UUtBool)(dmin <= r2);
}

UUtBool CLrSphere_Sphere(
	const M3tBoundingSphere		*inSphere,
	const M3tVector3D			*inVector,
	const M3tBoundingSphere		*inTargetSphere)
{
	float radius_squared;
	float distance_squared;
	M3tPoint3D end;
	UUtBool intersect;

	MUmVector_Add(end, inSphere->center, *inVector);

	intersect = CLrSphere_Line(&inSphere->center, &end, inTargetSphere);
	if (intersect) { goto exit; }

	radius_squared = UUmSQR(inSphere->radius + inTargetSphere->radius);

	distance_squared = MUmVector_GetDistanceSquared(inSphere->center, inTargetSphere->center);
	intersect = (distance_squared < radius_squared);
	if (intersect) { goto exit; }

	distance_squared = MUmVector_GetDistanceSquared(end, inTargetSphere->center);
	intersect = (distance_squared < radius_squared);
	if (intersect) { goto exit; }

exit:
	return intersect;
}

UUtBool PHrCollision_Volume_Ray(
	const M3tBoundingVolume *inA,
	const M3tPoint3D *inPoint,
	const M3tVector3D *inV,
	M3tPlaneEquation *outPlane,	// optional
	M3tPoint3D *outPoint)		// optional
{
	/**********
	* Intersects a point that moves 'inV' with the given bounding volume
	*/

	UUtBool collide;
	UUtBool foundCollision = UUcFalse;
	UUtUns32 i;
	M3tPlaneEquation *collisionPlane = NULL;
	float dot,smallestDot=-1e9f,d;
	M3tVector3D normal,normalV;
	M3tPoint3D endPoint,checkPoint,collisionPoint;

	normalV = *inV;
	d = MUmVector_GetLength(normalV);

	if (d!=0.0f) {
		MUmVector_Scale(normalV,1.0f/d);
	}
	else {
		UUmAssert(!foundCollision);
		goto exit;
	}
	MUmVector_Add(endPoint,*inPoint,*inV);

	for (i=0; i<M3cNumBoundingFaces; i++)
	{
		collide = CLrQuad_Line(
			(CLtQuadProjection)inA->curProjections[i],
			inA->worldPoints,
			inA->curPlanes + i,
			0,
			inA->faces + i,
			inPoint,
			&endPoint,
			&checkPoint);

		if (collide)
		{
			// Keep side facing us most directly
			AKmPlaneEqu_GetNormal(inA->curPlanes[i],normal);
			dot = MUrAngleBetweenVectors3D(&normalV,&normal);
			if (UUmFloat_CompareGT(dot,smallestDot))
			{
				smallestDot = dot;
				collisionPlane = (M3tPlaneEquation *)(inA->curPlanes + i);
				collisionPoint = checkPoint;
				foundCollision = UUcTrue;
			}
		}
	}

	if (foundCollision) {
		if (outPlane != NULL) {
			*outPlane = *collisionPlane;
		}

		if (outPoint != NULL) {
			*outPoint = collisionPoint;
		}
	}

exit:
	return foundCollision;
}

UUtBool PHrCollision_Volume_Point_Inside(
	const M3tBoundingVolume *inA,
	const M3tPoint3D *inPoint)
{
	UUtBool inside = UUcTrue;
	UUtInt32 i;

	for(i = 0; i < M3cNumBoundingFaces; i++)
	{
		M3tPlaneEquation *plane = NULL;
		float result;

		plane = (M3tPlaneEquation *) (inA->curPlanes + i);

		result = inPoint->x * plane->a + inPoint->y * plane->b + inPoint->z * plane->c + plane->d;

		if (result > 0) {
			inside = UUcFalse;
			break;
		}
	}

	return inside;
}

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
#pragma global_optimizer off
#endif

UUtBool PHrCollision_Box_Ray(const M3tBoundingBox_MinMax *inBox, M3tPoint3D *inPoint, M3tVector3D *inDir,
							 UUtBool inStopAtOneT, float *outT, UUtUns32 *outFace)
{
	float p[3], d[3];
	float boxmin[3], boxmax[3];
	float tmin[3], tmax[3];
	float t_entry, t_exit;
	UUtUns32 face;

#define COMPUTE_AXIS(ax, i) {							\
	p[i] = inPoint->ax;									\
	d[i] = inDir->ax;									\
	boxmin[i] = inBox->minPoint.ax;						\
	boxmax[i] = inBox->maxPoint.ax;						\
	if (d[i] > 1e-03f) {								\
		if (p[i] > boxmax[i]) {							\
			return UUcFalse;							\
		} else {										\
			tmin[i] = (boxmin[i] - p[i]) / d[i];		\
			tmax[i] = (boxmax[i] - p[i]) / d[i];		\
		}												\
	} else if (d[i] < -1e-03f) {						\
		if (p[i] < boxmin[i]) {							\
			return UUcFalse;							\
		} else {										\
			tmin[i] = (boxmax[i] - p[i]) / d[i];		\
			tmax[i] = (boxmin[i] - p[i]) / d[i];		\
		}												\
	} else {											\
		if (p[i] < boxmin[i]) {							\
			return UUcFalse;							\
		} else if (p[i] > boxmax[i]) {					\
			return UUcFalse;							\
		} else {										\
			tmin[i] = -1e09;							\
			tmax[i] = +1e09;							\
		}												\
	}													\
}

	COMPUTE_AXIS(x, 0);
	COMPUTE_AXIS(y, 1);
	COMPUTE_AXIS(z, 2);

	if (tmin[0] > tmin[1]) {
		if (tmin[0] > tmin[2]) {
			face = 0;
		} else {
			face = 2;
		}
	} else {
		if (tmin[1] > tmin[2]) {
			face = 1;
		} else {
			face = 2;
		}
	}
	t_entry = tmin[face];
	if ((inStopAtOneT) && (t_entry > 1.0f)) {
		return UUcFalse;
	} else if (t_entry < 0.0f) {
		t_entry = 0.0f;
	}

	if (tmax[0] < tmax[1]) {
		if (tmax[0] < tmax[2]) {
			t_exit = tmax[0];
		} else {
			t_exit = tmax[2];
		}
	} else {
		if (tmax[1] < tmax[2]) {
			t_exit = tmax[1];
		} else {
			t_exit = tmax[2];
		}
	}
	if (t_exit < t_entry) {
		return UUcFalse;
	}

	*outT = t_entry;
	if (d[face] > 0) {
		*outFace = 2 * face;
	} else {
		*outFace = 2 * face + 1;
	}
	return UUcTrue;
}

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
#pragma global_optimizer reset
#endif
