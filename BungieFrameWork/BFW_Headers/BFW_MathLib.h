#pragma once
/*
	FILE:	BFW_MathLib.h
	
	AUTHOR:	Michael Evans
	
	CREATED: January 9, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#ifndef __BFW_MATHUTIL_H__
#define __BFW_MATHUTIL_H__

#include "bfw_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "BFW.h"
#include "BFW_Motoko.h"

#define UUcFloat_NumFractinalBits	(23)
#define UUcFloat_FractionMask	0x007fffff	// bits 9-31 [23]
#define UUcFloat_NormalizedBit	0x00800000
#define UUcFloat_ExponentMask	0x7f800000	// bits 1- 8 [8]
#define UUcFloat_SignBit		0x80000000	// bit     0 [1]
	
#define UUcDouble_FractionMask	((UUtUns64) 0x000fffffffffffff)	// bits 12-63
#define UUcDouble_NormalizedBit	((UUtUns64) 0x0010000000000000) 
#define UUcDouble_ExponentMask	((UUtUns64) 0x7ff0000000000000) // bits  1-11
#define UUcDouble_SignBit		((UUtUns64) 0x8000000000000000) // bit      0

typedef union MUtInternal_Cast
{
	float		float_4byte;
	UUtUns32	unsigned_4byte;
	UUtInt32	signed_4byte;
} MUtInternal_Cast;

// returns the bits of a float packed into a unsigned 32 int
// 2.f != 2, 2.f = some nasty hex number
UUtUns32 MUrFloat_To_Uns_Bits(float f);
float MUrUnsBits_To_Float(UUtUns32 bits);
/* make sure your floats never equal or exceed 1<<23 (8,388,608.0) or are negative */
/* this function rounds instead of truncating */
UUtInt32 MUrUnsignedSmallFloat_To_Int_Round(float f);
/* make sure your floats never equal or exceed 1<<23 (8,388,608.0) or are negative */
// this function rounds instead of truncating
UUtUns32 MUrUnsignedSmallFloat_To_Uns_Round(float f);
// f >= 0, f < (1 << (23 - shiftLeftValue))
// shiftLeftValue 0 .. 22
// returns ROUND(float * 2 ^ (shiftLeftValue))
UUtInt32 MUrUnsignedSmallFloat_ShiftLeft_To_Int_Round(float f, UUtInt32 shiftLeftValue);
// f >= 0, f < (1 << (23 - shiftLeftValue))
// shiftLeftValue 0 .. 22
// returns ROUND(float * 2 ^ (shiftLeftValue))
UUtUns32 MUrUnsignedSmallFloat_ShiftLeft_To_Uns_Round(float f, UUtInt32 shiftLeftValue);
UUtInt32 MUrFloat_Round_To_Int(float f);
UUtUns8 MUrFloat0ToLessThan1_ToUns8(float f);

//
// Types
//

typedef struct MUtEuler
{
	float x;
	float y;
	float z;

	UUtInt32 order;
} MUtEuler;

void MUrMinMaxBBox_CreateFromSphere(
		const M3tMatrix4x3 *inMatrix,
		const M3tBoundingSphere *inSphere, 
		M3tBoundingBox_MinMax *outBBox);

void MUrMinMaxBBox_ExpandBySphere(
		const M3tMatrix4x3 *inMatrix,
		const M3tBoundingSphere *inSphere, 
		M3tBoundingBox_MinMax *ioBBox);

void MUrMinMaxBBox_To_Sphere(
		const M3tBoundingBox_MinMax *inBBox,
		M3tBoundingSphere *outSphere);

void MUrCylinder_CreateFromSphere(
		const M3tMatrix4x3 *inMatrix,
		const M3tBoundingSphere *inSphere, 
		M3tBoundingCylinder *outCylinder);

void MUrCylinder_ExpandBySphere(
		const M3tMatrix4x3 *inMatrix,
		const M3tBoundingSphere *inSphere, 
		M3tBoundingCylinder *ioCylinder);

void MUrInitialize(void);
void MUrTerminate(void);


UUtUns8 MUrSolveQuadratic(float a, float b, float c, float *outSolutions);

/*
 * floating point versions of sin, cos, sqrt etc
 *
 *
 */

#if defined(DEBUGGING) && DEBUGGING
#define INLINE_TRIG 0
#define INLINE_SQRT 0
#else
#define INLINE_TRIG 1
#define INLINE_SQRT 1
#endif

#if INLINE_TRIG
#define MUrSin(f) ((float) sin(f))
#define MUrCos(f) ((float) cos(f))
#define MUrTan(f) ((float) tan(f))
#define MUrATan2(x,y) ((float) atan2(x,y))
#else
float MUrSin(float f);
float MUrCos(float f);
float MUrTan(float f);
float MUrATan2(float x, float y);
#endif

#if INLINE_SQRT
#define MUrSqrt(f) ((float) sqrt((f)))
#define MUrOneOverSqrt(f) ((float) 1.0 / (float) sqrt(f))
#else
float MUrSqrt(float f);
float MUrOneOverSqrt(float f);
#endif

void MUrLineSegment_ComputePoint(
	const M3tPoint3D			*inLineP1,
	const M3tPoint3D			*inLineP2,
	      float					inU,
	      M3tPoint3D			*outPoint);

float MUrLineSegment_ClosestPoint(
	const M3tPoint3D			*inLineP1,
	const M3tPoint3D			*inLineP2,
	const M3tPoint3D			*inPoint);

void MUrLineSegment_ClosestPointToPoint(
	M3tPoint3D *inLineP1,
	M3tPoint3D *inLineP2,
	M3tPoint3D *inPoint,
	M3tPoint3D *outClosestPoint);

float MUrATan(float f);
float MUrACos(float f);
float MUrASin(float inFloat);

void MUrPoint_RotateAboutAxis(
	const M3tPoint3D*	inPoint,
	const M3tVector3D*	inVector,
	float				inRadians,
	M3tPoint3D 			*outPoint);
void MUrPoint_RotateXAxis(const M3tPoint3D *inPoint, float radians, M3tPoint3D *outPoint);
void MUrPoint_RotateYAxis(const M3tPoint3D *inPoint, float radians, M3tPoint3D *outPoint);
void MUrPoint_RotateZAxis(const M3tPoint3D *inPoint, float radians, M3tPoint3D *outPoint);

void
MUrPoint_Align(
	const M3tVector3D*	inFrom,
	const M3tVector3D*	inTo,
	M3tPoint3D			*outPoint);

void MUrPoint2D_Add(M3tPoint2D *result, const M3tPoint2D *inPointA, const M3tPoint2D *inPointB);
void MUrPoint2D_Subtract(M3tPoint2D *result, const M3tPoint2D *inPointA, const M3tPoint2D *inPointB);

/*
 * macros for basic math
 */

#define MUmAssertPlane(plane, value) \
		UUmAssert((plane).a > (-(value))); \
		UUmAssert((plane).b > (-(value))); \
		UUmAssert((plane).c > (-(value))); \
		UUmAssert((plane).d > (-(value))); \
		UUmAssert((plane).a < ( (value))); \
		UUmAssert((plane).b < ( (value))); \
		UUmAssert((plane).c < ( (value))); \
		UUmAssert((plane).d < ( (value))); 

#define MUmAssertVector(vec, value)	\
	UUmAssert((vec).x > (-(value))); \
	UUmAssert((vec).y > (-(value))); \
	UUmAssert((vec).z > (-(value))); \
	UUmAssert((vec).x < (value)); \
	UUmAssert((vec).y < (value)); \
	UUmAssert((vec).z < (value)); 

#define MUmVector_IsZero(vec) \
(	UUmFloat_CompareEqu((vec).x, 0) && \
	UUmFloat_CompareEqu((vec).y, 0) && \
	UUmFloat_CompareEqu((vec).z, 0))

#define MUmVector_Copy(dst, src)		\
do {	(dst).x = (src).x;	\
		(dst).y = (src).y;	\
		(dst).z = (src).z; } while (0)

#ifndef USE_FAST_MATH
#define MUmVector_Add(dst, srca, srcb)	\
do {	(dst).x = (srca).x + (srcb).x;	\
		(dst).y = (srca).y + (srcb).y;	\
		(dst).z = (srca).z + (srcb).z; } while (0)
#else
#define MUmVector_Add(r, a, b) \
	fast_add_vect((D3DVECTOR *)(&(r)), (D3DVECTOR *)(&(a)), (D3DVECTOR *)(&(b)))
#endif

#ifndef USE_FAST_MATH
#define MUmVector_Subtract(dst, srca, srcb)	\
do {	(dst).x = (srca).x - (srcb).x;	\
		(dst).y = (srca).y - (srcb).y;	\
		(dst).z = (srca).z - (srcb).z; } while (0)
#else
#define MUmVector_Subtract(r, a, b)	\
	fast_sub_vect((D3DVECTOR *)(&(r)), (D3DVECTOR *)(&(a)), (D3DVECTOR *)(&(b)))
#endif

#define MUmVector_Decrement(dst, src)	\
do {	(dst).x -= (src).x;	\
		(dst).y -= (src).y;	\
		(dst).z -= (src).z; } while (0)

#define MUmVector_Increment(dst, src)	\
do {	(dst).x += (src).x;	\
		(dst).y += (src).y;	\
		(dst).z += (src).z; } while (0)

#define MUmVector_IncrementMultiplied(dst, src, amt)	\
do {	(dst).x += (amt) * (src).x;	\
		(dst).y += (amt) * (src).y;	\
		(dst).z += (amt) * (src).z; } while (0)

#define MUmVector_Set(vec,a,b,c)	\
do {	(vec).x=(a); \
		(vec).y=(b); \
		(vec).z=(c); } while (0)

#ifndef USE_FAST_MATH	
#define MUmVector_Scale(vec,s)	\
do {	(vec).x*=(s); \
		(vec).y*=(s); \
		(vec).z*=(s); } while (0)
#else
#define MUmVector_Scale(vec,s)	\
	fast_scale_vect((D3DVECTOR *)(&(vec)), (D3DVECTOR *)(&(vec)), (s))
#endif

#define MUmVector_ScaleCopy(dst,s,src)	\
do {	(dst).x = (src).x * (s); \
		(dst).y = (src).y * (s); \
		(dst).z = (src).z * (s); } while (0)

#define MUmVector_ScaleIncrement(dst,s,src)	\
do {	(dst).x += (src).x * (s); \
		(dst).y += (src).y * (s); \
		(dst).z += (src).z * (s); } while (0)

#define MUmVector_Negate(vec) \
do {	(vec).x = -(vec).x; \
		(vec).y = -(vec).y; \
		(vec).z = -(vec).z; } while (0)

#ifndef USE_FAST_MATH
#define MUmVector_GetLength(vector)		\
	MUrSqrt(UUmSQR((vector).x) + UUmSQR((vector).y) + UUmSQR((vector).z))
#else
#define MUmVector_GetLength(vector)		\
	fast_mag_vect((D3DVECTOR *)(&(vector)))
#endif

#define MUmVector_GetLengthSquared(vector)		\
	(UUmSQR((vector).x) + UUmSQR((vector).y) + UUmSQR((vector).z))

#define MUmVector_GetManhattanLength(vector)		\
	(UUmFabs((vector).x) + UUmFabs((vector).y) + UUmFabs((vector).z))

#define MUmVector_GetDistance(vA, vB)		\
	(MUrSqrt(UUmSQR((vA).x - (vB).x) + UUmSQR((vA).y - (vB).y) + UUmSQR((vA).z - (vB).z)))

#define MUmVector_GetManhattanDistance(vA, vB)	\
	(UUmFabs((vA).x - (vB).x) + UUmFabs((vA).y - (vB).y) + UUmFabs((vA).z - (vB).z))

#define MUmVector_GetDistanceSquared(vA, vB)		\
	(UUmSQR((vA).x - (vB).x) + UUmSQR((vA).y - (vB).y) + UUmSQR((vA).z - (vB).z))

#ifndef USE_FAST_MATH
#define MUmVector_Normalize(vector)	\
do {	float one_over_length = MUrOneOverSqrt(MUmVector_GetLengthSquared(vector)); \
		MUmVector_Scale(vector, one_over_length); } while (0)
#else
#define MUmVector_Normalize(vector)	\
	fast_norm_vect((D3DVECTOR *)(&(vector)), (D3DVECTOR *)(&(vector)))
#endif

UUtBool MUrVector_Normalize_ZeroTest(M3tVector3D *inV);

#define MUmVector_IsNormalized(vector)	\
		(((UUmSQR((vector).x) + UUmSQR((vector).y) + UUmSQR((vector).z)) > .99f)  && \
		((UUmSQR((vector).x) + UUmSQR((vector).y) + UUmSQR((vector).z)) < 1.01f))
	
#define MUmPoint_Compare(pointA, pointB) \
	(UUmFloat_CompareEqu((pointA).x, (pointB).x) && \
	 UUmFloat_CompareEqu((pointA).y, (pointB).y) && \
	 UUmFloat_CompareEqu((pointA).z, (pointB).z))

#define MUmPoint_CompareZero(pointA) \
	(UUmFloat_CompareEqu((pointA).x, 0) && \
	 UUmFloat_CompareEqu((pointA).y, 0) && \
	 UUmFloat_CompareEqu((pointA).z, 0))

#define MUmVector3D_Swap(vector) \
	UUrSwap_4Byte(&((vector).x)); \
	UUrSwap_4Byte(&((vector).y)); \
	UUrSwap_4Byte(&((vector).z));

#define MUmPoint3D_Swap(point) \
	UUrSwap_4Byte(&((point).x)); \
	UUrSwap_4Byte(&((point).y)); \
	UUrSwap_4Byte(&((point).z));

UUtBool
MUrAngleLess180(
	M3tVector3D*	inVectA,
	M3tVector3D*	inVectB);

void MUrVector_PushBack(M3tVector3D *outPushBack, const M3tVector3D *inDelta, const M3tVector3D *inNormal);
float MUrPointsXYAngle(float inFromX, float inFromY, float inToX, float inToY);

void MUrVector_ProjectToPlane(
	const M3tPlaneEquation*		inPlaneEqu,
	const M3tVector3D*			inVector,
	M3tVector3D*				outVector);

UUtBool MUrPoint_Distance_Less(const M3tPoint3D *inA, const M3tPoint3D *inB, float amount);
UUtBool MUrPoint_Distance_LessOrEqual(const M3tPoint3D *inA, const M3tPoint3D *inB, float amount);

static UUcInline UUtBool MUrPoint_Distance_Greater(const M3tPoint3D *inA, const M3tPoint3D *inB, float amount)
{
	return !MUrPoint_Distance_LessOrEqual(inA, inB, amount);
}

static UUcInline UUtBool MUrPoint_Distance_GreaterOrEqual(const M3tPoint3D *inA, const M3tPoint3D *inB, float amount)
{
	return !MUrPoint_Distance_Less(inA, inB, amount);
}

float MUrPoint_Distance(const M3tPoint3D *inA, const M3tPoint3D *inB);
float MUrPoint_Distance_Squared(const M3tPoint3D *inA, const M3tPoint3D *inB);

float MUrPoint_Distance_SquaredXZ(const M3tPoint3D *inA, const M3tPoint3D *inB);
float MUrPoint_ManhattanDistance(const M3tPoint3D *inA, const M3tPoint3D *inB);
float MUrPoint_Length(M3tPoint3D *inPoint);
float MUrPoint_ManhattanLength(M3tPoint3D *inPoint);
UUtBool MUrPoint_PointOnPlane(M3tPoint3D *inPoint, M3tPlaneEquation *inPlane);
UUtBool MUrPoint_PointBehindPlane(M3tPoint3D *inPoint, M3tPlaneEquation *inPlane);
UUtBool MUrPoint_PointOnOrBehindPlane(M3tPoint3D *inPoint, M3tPlaneEquation *inPlane);
UUtBool MUrPoint_PointOnPlaneSloppy(M3tPoint3D *inPoint, M3tPlaneEquation *inPlane);
UUtBool MUrPoint_PointBehindPlaneSloppy(M3tPoint3D *inPoint, M3tPlaneEquation *inPlane);
float MUrVector_Length(M3tVector3D *inPoint);
float MUrAngleBetweenVectors(float inAx, float inAy, float inBx, float inBy);
float MUrAngleBetweenVectors3D(const M3tVector3D *inVector1, const M3tVector3D *inVector2);
float MUrAngleBetweenVectors3D_0To2Pi(const M3tVector3D *inVector1, const M3tVector3D *inVector2);
void MUrVector_Increment_Unique(M3tVector3D *inDest, M3tVector3D *inSrc, M3tVector3D *inList, UUtUns16 *inCount);
void MUrVector_ProjectVector(M3tVector3D *inA, M3tVector3D *inB, M3tVector3D *outV);

float MUrPoint_ManhattanDistanceXZ(const M3tPoint3D *inA, const M3tPoint3D *inB);

float MUrPoint_DistanceToQuad(
	M3tPoint3D	*inPoint,
	M3tPoint3D	*inPointArray,
	M3tQuad		*inQuad);
float MUrPoint_ManhattanDistanceToQuad(
	M3tPoint3D	*inPoint,
	M3tPoint3D	*inPointArray,
	M3tQuad		*inQuad);
	
void MUrVector_PlaneFromEdges(
	M3tPoint3D *inEdge1a,
	M3tPoint3D *inEdge1b,
	M3tPoint3D *inEdge2a,
	M3tPoint3D *inEdge2b,
	M3tPlaneEquation *outPlane);

float
MUrTriangle_Area(
	M3tPoint3D*	inPoint0,
	M3tPoint3D*	inPoint1,
	M3tPoint3D*	inPoint2);
	
/*
 * 
 */

void MUrMatrix_From_Point_And_Vectors(
	M3tMatrix4x3 *inMatrix,
	const M3tPoint3D *inPoint,
	const M3tVector3D *inForward,
	const M3tVector3D *inUp);

UUtBool MUrMatrix_IsEqual(
	const M3tMatrix4x3 *inMatrixA,
	const M3tMatrix4x3 *inMatrixB);

UUtBool MUrEuler_IsEqual(
	const MUtEuler *inEulerA,
	const MUtEuler *inEulerB);

UUtBool MUrQuat_IsEqual(
	const M3tQuaternion *inQuatA,
	const M3tQuaternion *inQuatB);

UUtBool MUrPoint_IsEqual(
	const M3tPoint3D *inPointA,
	const M3tPoint3D *inPointB);

UUtBool MUrVector_IsEqual(
	const M3tVector3D *inPointA,
	const M3tVector3D *inPointB);

UUtBool MUrPlaneEquation_IsEqual(
	const M3tPlaneEquation *inPlaneEquationA,
	const M3tPlaneEquation *inPlaneEquationB);

// IsClose returns true for planes that have opposite facing
UUtBool MUrPlaneEquation_IsClose(
	const M3tPlaneEquation *inPlaneEquationA,
	const M3tPlaneEquation *inPlaneEquationB);

typedef struct {
    M3tVector3D		translation;		
    M3tQuaternion	rotation;
    M3tQuaternion	stretchRotation;	
    M3tVector3D		stretch;		
    float			sign;				
} MUtAffineParts;

// strips scale and translation
void MUrMatrix_To_RotationMatrix(const M3tMatrix4x3 *inMatrix, M3tMatrix4x3 *outMatrix);

void MUrMatrix_StripScale(const M3tMatrix4x3 *inMatrix, M3tMatrix4x3 *outMatrix);
UUtError MUrMatrix_GetUniformScale(M3tMatrix4x3 *inMatrix, float *outScale);
void MUrMatrix_To_ScaleMatrix(const M3tMatrix4x3 *inMatrix, M3tMatrix4x3 *outMatrix);
UUtBool MUrMatrix_Has_NonUniformScale(const M3tMatrix4x3 *inMatrix);

void MUrMatrix_DecomposeAffine(const M3tMatrix4x3 *inMatrix, MUtAffineParts *outParts);
void MUrMatrix_Lerp(const M3tMatrix4x3 *inFrom, const M3tMatrix4x3 *inTo, float t, M3tMatrix4x3 *outResult);
void MUrMatrixToQuat(const M3tMatrix4x3 *inMatrix, M3tQuaternion *outQuat);
void MUrQuatToMatrix(const M3tQuaternion *inQuat, M3tMatrix4x3 *outMatrix);

static UUcInline M3tPoint3D MUrMatrix_GetXAxis(const M3tMatrix4x3 *inMatrix)
{
	M3tPoint3D translation;

	translation.x = inMatrix->m[0][0];
	translation.y = inMatrix->m[0][1];
	translation.z = inMatrix->m[0][2];

	return translation;
}

static UUcInline M3tPoint3D MUrMatrix_GetYAxis(const M3tMatrix4x3 *inMatrix)
{
	M3tPoint3D x_axis;

	x_axis.x = inMatrix->m[1][0];
	x_axis.y = inMatrix->m[1][1];
	x_axis.z = inMatrix->m[1][2];

	return x_axis;
}

static UUcInline M3tPoint3D MUrMatrix_GetZAxis(const M3tMatrix4x3 *inMatrix)
{
	M3tPoint3D y_axis;

	y_axis.x = inMatrix->m[2][0];
	y_axis.y = inMatrix->m[2][1];
	y_axis.z = inMatrix->m[2][2];

	return y_axis;
}

static UUcInline M3tPoint3D MUrMatrix_GetTranslation(const M3tMatrix4x3 *inMatrix)
{
	M3tPoint3D z_axis;

	z_axis.x = inMatrix->m[3][0];
	z_axis.y = inMatrix->m[3][1];
	z_axis.z = inMatrix->m[3][2];

	return z_axis;
}
static UUcInline void MUrMatrix_SetXAxis(M3tMatrix4x3 *inMatrix, const M3tPoint3D *inXAxis)
{
	inMatrix->m[0][0] = inXAxis->x;
	inMatrix->m[0][1] = inXAxis->y;
	inMatrix->m[0][2] = inXAxis->z;

	return;
}

static UUcInline void MUrMatrix_SetYAxis(M3tMatrix4x3 *inMatrix, const M3tPoint3D *inYAxis)
{
	inMatrix->m[1][0] = inYAxis->x;
	inMatrix->m[1][1] = inYAxis->y;
	inMatrix->m[1][2] = inYAxis->z;

	return;
}

static UUcInline void MUrMatrix_SetZAxis(M3tMatrix4x3 *inMatrix, const M3tPoint3D *inZAxis)
{
	inMatrix->m[2][0] = inZAxis->x;
	inMatrix->m[2][1] = inZAxis->y;
	inMatrix->m[2][2] = inZAxis->z;

	return;
}

static UUcInline void MUrMatrix_SetTranslation(M3tMatrix4x3 *inMatrix, const M3tPoint3D *inTranslation)
{
	inMatrix->m[3][0] = inTranslation->x;
	inMatrix->m[3][1] = inTranslation->y;
	inMatrix->m[3][2] = inTranslation->z;

	return;
}

void MUrMatrix_GetRow(const M3tMatrix4x3 *inMatrix, UUtUns8 inRow, M3tPoint3D *outRow);
void MUrMatrix_GetCol(const M3tMatrix4x3 *inMatrix, UUtUns8 inCol, M3tPoint3D *outCol);
void MUrMatrix_Inverse(const M3tMatrix4x3 *inV, M3tMatrix4x3 *outV);
void MUrMatrix_Adjoint(const M3tMatrix4x3 *inV, M3tMatrix4x3 *outV);
void MUrMatrix_Identity(M3tMatrix4x3 *ioMatrix);
float MUrMatrix_Determinant(const M3tMatrix4x3 *m);
void MUrMatrix_RotateX(M3tMatrix4x3 *matrix, float angle); 
void MUrMatrix_RotateY(M3tMatrix4x3 *matrix, float angle); 
void MUrMatrix_RotateZ(M3tMatrix4x3 *matrix, float angle); 
void MUrMatrix3x3_Rotate(M3tMatrix3x3 *matrix, float inX, float inY, float inZ, float angle); 
void MUrMatrix3x3_RotateX(M3tMatrix3x3 *matrix, float angle); 
void MUrMatrix3x3_RotateY(M3tMatrix3x3 *matrix, float angle); 
void MUrMatrix3x3_RotateZ(M3tMatrix3x3 *matrix, float angle); 
void MUrMatrix_Translate(
	M3tMatrix4x3 		*ioMatrix,
	const M3tVector3D	*inTranslate);

float MUr3x3_Determinant(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3 );
float MUr2x2_Determinant(float a, float b, float c, float d);

#if defined(DEBUGGING) && DEBUGGING
void MUrMatrixVerify(const M3tMatrix4x3 *inMatrix);
#else
#define MUrMatrixVerify(inMatrix) UUmBlankFunction
#endif

void MUrMatrix_Multiply(
			const M3tMatrix4x3	*inMatrixA, 
			const M3tMatrix4x3	*inMatrixB, 
			M3tMatrix4x3		*outResult);
void MUrMatrix3x3_Multiply(
			const M3tMatrix3x3	*inMatrixA, 
			const M3tMatrix3x3	*inMatrixB, 
			M3tMatrix3x3		*outResult);
// transpose a matrix
void MUrMatrix3x3_Transpose(
			const M3tMatrix3x3	*inMatrix, 
			M3tMatrix3x3		*outResult);

void MUrMatrix_MultiplyNormal(
	const M3tVector3D	*inNormal,
	const M3tMatrix4x3	*inMatrix,
	M3tVector3D			*outNormal);

void MUrMatrix_MultiplyNormals(
	UUtUns32			inNumPoints,
	const M3tMatrix4x3	*inMatrix,
	const M3tVector3D	*inNormals,
	M3tVector3D			*outNormals);

void MUrMatrix_MultiplyPoint(
	const M3tPoint3D	 *inPoint, 
	const M3tMatrix4x3	 *inMatrix, 
	M3tPoint3D			 *outPoint);

void MUrMatrix3x3_MultiplyPoint(
	const M3tPoint3D	 *inPoint, 
	const M3tMatrix3x3	 *inMatrix, 
	M3tPoint3D			 *outPoint);

void MUrMatrix3x3_TransposeMultiplyPoint(
	const M3tPoint3D	 *inPoint, 
	const M3tMatrix3x3	 *inMatrix, 
	M3tPoint3D			 *outPoint);

void MUrMatrix_MultiplyPoints(
	UUtUns32			inNumPoints,
	const M3tMatrix4x3	*inMatrix, 
	const M3tPoint3D	*inPoints, 
	M3tPoint3D			*outPoints);

void MUrMatrix_DualAlignment(
	const M3tVector3D *src1,
	const M3tVector3D *src2,
	const M3tVector3D *dst1,
	const M3tVector3D *dst2,
	M3tMatrix4x3 *outMatrix);

void MUrMatrix_Alignment(
	const M3tVector3D *from,
	const M3tVector3D *to,
	M3tMatrix4x3 *outMatrix);

/*
 * matrix stack functions
 *
 *
 */

void MUrMatrixStack_Matrix(
	M3tMatrix4x3 *topOfStack,
	const M3tMatrix4x3 *inMatrix);

void MUrMatrixStack_Translate(
	M3tMatrix4x3 *topOfStack,
	const M3tVector3D *inTranslate);

void MUrMatrixStack_Scale(
	M3tMatrix4x3 *topOfStack,
	float inScale);

void MUrMatrixStack_Quaternion(
	M3tMatrix4x3 *topOfStack,
	const M3tQuaternion *inQuaternion);

void MUrMatrixStack_Euler(
	M3tMatrix4x3 *topOfStack,
	const MUtEuler *inEuler);

void MUrMatrixStack_RotateX(M3tMatrix4x3 *topOfStack, float angle); 
void MUrMatrixStack_RotateY(M3tMatrix4x3 *topOfStack, float angle); 
void MUrMatrixStack_RotateZ(M3tMatrix4x3 *topOfStack, float angle); 


/* 
 * euler functions
 *
 * see EulerAngles.h or GraphicsGems IV pp 222
 *
 * include EulerAngles.h for constants for EulerFrom
 * 
 */

MUtEuler Euler_(float ai, float aj, float ah, int order);
//#ifndef USE_FAST_MATH
void MUrXYZEulerToQuat(float x, float y, float z, M3tQuaternion *quat);
/*#else
#define MUrXYZEulerToQuat(x, y, z, quat) \
	{ \
		float rot[3]= {(x), (y), (z)}; \
		fast_euler2quat((D3DRMQUATERNION *)(quat), rot); \
	}
#endif*/
void MUrZYXEulerToQuat(float x, float y, float z, M3tQuaternion *quat);

// MUcEulerOrderXYZs is (rotate x) * (rotate y) * (rotate z)
M3tQuaternion MUrEulerToQuat(const MUtEuler *ea);
void MUrXYXsEulerToQuat(float x, float y, float z, M3tQuaternion *outQuat);
void MUrXYZrEulerToQuat(float x, float y, float z, M3tQuaternion *outQuat);
void MUrEulerToMatrix(const MUtEuler *inEuler, M3tMatrix4x3 *inMatrix);
MUtEuler MUrMatrixToEuler(const M3tMatrix4x3 *M, int order);
MUtEuler MUrQuatToEuler(const M3tQuaternion *q, int order);


/*
 * quaternion functions from external source
 */

void MUrQuat_SetFromTwoVectors(const M3tVector3D *from, const M3tVector3D *to, M3tQuaternion *outQuat);
void MUrQuat_SetFromAx(float x1,float y1, float z1, float x2,float y2, float z2, M3tQuaternion *quat);
void MUrQuat_SetValue(M3tQuaternion *quat, float x, float y, float z, float angle);
void MUrQuat_GetValue(M3tQuaternion *quat, float *x, float *y, float *z, float *radians);

extern const M3tPoint3D MUgZeroPoint;
extern const M3tVector3D MUgZeroVector;
extern const M3tQuaternion MUgZeroQuat;
extern const M3tMatrix4x3 MUgIdentityMatrix;
extern const float MUgSign256[];

#define MUrQuat_Clear(inQuat) { *inQuat = MUgZeroQuat; }
#ifndef USE_FAST_MATH
void MUrQuat_Add(const M3tQuaternion* q1, const M3tQuaternion* q2, M3tQuaternion* res);
#else
#define MUrQuat_Add(q1, q2, res) \
	fast_add_quat((D3DRMQUATERNION *)(res), (D3DRMQUATERNION *)(q1), (D3DRMQUATERNION *)(q2))
#endif
void MUrQuat_Copy(const M3tQuaternion* q1, M3tQuaternion* q2);
void MUrQuat_Div(const M3tQuaternion* q1, const M3tQuaternion* q2, M3tQuaternion* res);
void MUrQuat_Div(const M3tQuaternion* q1, const M3tQuaternion* q2, M3tQuaternion* res);
float MUrQuat_Dot(const M3tQuaternion* q1, const M3tQuaternion* q2);
void MUrQuat_Exp(M3tQuaternion* q1, M3tQuaternion* q2);

#define MUmVector_Verify(vector) { UUmAssert((vector).x >= -1e9); UUmAssert((vector).y >= -1e9); UUmAssert((vector).z >= -1e9); UUmAssert((vector).x <= 1e9); UUmAssert((vector).y <= 1e9); UUmAssert((vector).z <= 1e9); }
#define MUmQuat_VerifyUnit(quat) { float size;  UUmAssertReadPtr(quat, sizeof(M3tQuaternion)); size = UUmSQR((quat)->x) + UUmSQR((quat)->y) + UUmSQR((quat)->z) + UUmSQR((quat)->w); UUmAssert((size > 0.98f) && (size < 1.02f)); }

#define MUmQuaternion_Swap(quat) \
	UUrSwap_4Byte(&((quat).x)); \
	UUrSwap_4Byte(&((quat).y)); \
	UUrSwap_4Byte(&((quat).z)); \
	UUrSwap_4Byte(&((quat).w));

#define MUmQuat_Compare(quatA,quatB) \
	(UUmFloat_CompareEqu(quatA.x,quatB.x) && \
	UUmFloat_CompareEqu(quatA.y,quatB.y) && \
	UUmFloat_CompareEqu(quatA.z,quatB.z) && \
	UUmFloat_CompareEqu(quatA.w,quatB.w))
	
void MUrQuat_Inverse(const M3tQuaternion *quat, M3tQuaternion *res);
float MUrQuat_Length(M3tQuaternion* q1);
void MUrQuat_Lerp(const M3tQuaternion *from, const M3tQuaternion *to, float t, M3tQuaternion *res);
void MUrQuat_Log(M3tQuaternion* q1, M3tQuaternion* q2);
#ifndef USE_FAST_MATH
void MUrQuat_Mul(const M3tQuaternion* q1, const M3tQuaternion* q2, M3tQuaternion* res);
#else
#define MUrQuat_Mul(q1, q2, res) \
	fast_mult_quat((D3DRMQUATERNION *)(res), (D3DRMQUATERNION *)(q1), (D3DRMQUATERNION *)(q2))
#endif
void MUrQuat_Negate(M3tQuaternion* q1, M3tQuaternion* q2);
#ifndef USE_FAST_MATH
void MUrQuat_Normalize(M3tQuaternion* quat);
#else
#define MUrQuat_Normalize(quat) \
	fast_norm_quat((D3DRMQUATERNION *)(quat), (D3DRMQUATERNION *)(quat))
#endif
void MUrQuat_ScaleAngle(M3tQuaternion * quat, float scale);
//#ifndef USE_FAST_MATH
void MUrQuat_Slerp(const M3tQuaternion * from, const M3tQuaternion * to, float t, M3tQuaternion * res);
/*#else
#define MUrQuat_Slerp(from, to, t, res) \
	fast_slerp_quat((D3DRMQUATERNION *)(res), (D3DRMQUATERNION *)(from), (D3DRMQUATERNION *)(to), (t))
#endif*/
void MUrQuat_Sqrt(const M3tQuaternion* q1, M3tQuaternion* res);
void MUrQuat_Square(const M3tQuaternion* q1, M3tQuaternion* res);
#ifndef USE_FAST_MATH
void MUrQuat_Sub(const M3tQuaternion* q1, const M3tQuaternion* q2, M3tQuaternion* res);
#else
#define MUrQuat_Sub(q1, q2, res) \
	fast_sub_quat((D3DRMQUATERNION *)(res), (D3DRMQUATERNION *)(q1), (D3DRMQUATERNION *)(q2))
#endif
#ifndef USE_FAST_MATH
void MUrNormalize(M3tVector3D *ioVector);
#else
#define MUrNormalize(v) fast_norm_vect((D3DVECTOR *)(v), (D3DVECTOR *)(v))
#endif

void MUrPoint3D_Lerp(const M3tPoint3D *inFrom, const M3tPoint3D *inTo, float t, M3tPoint3D *outResult);

/*
 * 
 */
	void MUrVector_SetLength(M3tVector3D *ioVector, float inNewMagnitude);

#ifndef USE_FAST_MATH
	void
	MUrVector_CrossProduct(
		const M3tVector3D*		inVectorA,
		const M3tVector3D*		inVectorB,
		M3tVector3D*			inResult);
#else
	#define MUrVector_CrossProduct(a, b, r) \
	fast_cross_vect((D3DVECTOR *)(r), (D3DVECTOR *)(a), (D3DVECTOR *)(b))
#endif

#ifndef USE_FAST_MATH
	float
	MUrVector_DotProduct(
		const M3tVector3D*		inVectorA,
		const M3tVector3D*		inVectorB);
#else
	#define MUrVector_DotProduct(a, b) \
	fast_dot_vect((D3DVECTOR *)(a), (D3DVECTOR *)(b))
#endif

	void
	MUrVector_NormalFromPoints(
		const M3tPoint3D*	inPoint0,
		const M3tPoint3D*	inPoint1,
		const M3tPoint3D*	inPoint2,
		M3tVector3D			*outNormal);
	
/*
 * 
 */

	void
	MUrMatrix_BuildRotateX(
		float			inRadians,
		M3tMatrix4x3		*outMatrix);

	void
	MUrMatrix_BuildRotateY(
		float			inRadians,
		M3tMatrix4x3		*outMatrix);

	void
	MUrMatrix_BuildRotateZ(
		float			inRadians,
		M3tMatrix4x3		*outMatrix);

	void
	MUrMatrix3x3_BuildRotateX(
		float			inRadians,
		M3tMatrix3x3		*outMatrix);

	void
	MUrMatrix3x3_BuildRotateY(
		float			inRadians,
		M3tMatrix3x3		*outMatrix);

	void
	MUrMatrix3x3_BuildRotateZ(
		float			inRadians,
		M3tMatrix3x3		*outMatrix);

	void
	MUrMatrix3x3_BuildRotate(
		float			inRadians,
		float			inX,
		float			inY,
		float			inZ,
		M3tMatrix3x3	*outMatrix);

	void
	MUrMatrix_BuildRotate(
		float			inRadians,
		float			inX,
		float			inY,
		float			inZ,
		M3tMatrix4x3		*outMatrix);
	
	void
	MUrMatrix_BuildTranslate(
		float			inX,
		float			inY,
		float			inZ,
		M3tMatrix4x3		*outMatrix);

	void
	MUrMatrix_BuildUniformScale(
		float			inScale,
		M3tMatrix4x3		*outMatrix);

	void 
	MUrMatrix_BuildScale(
		M3tPoint3D		*inScale,
		M3tMatrix4x3		*outMatrix);

	void MUrMatrix_FromMax(
		M3tMatrix4x3 *ioMatrix);

// Matrix3x3 operations
	void
	MUrMatrix3x3_MultiplyNormal(
		const M3tVector3D	 *inNormal, 
		const M3tMatrix3x3	 *inMatrix, 
		M3tVector3D			 *outNormal);
	
	void
	MUrMatrix3x3_MultiplyNormals(
		UUtUns32			inNumNormals,
		const M3tMatrix3x3	*inMatrix,
		const M3tVector3D	*inNormals,
		M3tVector3D			*outNormals);
	
	void
	MUrMatrix3x3_ExtractFrom4x3(
		M3tMatrix3x3		*outMatrix3x3,
		const M3tMatrix4x3	*inMatrix4x3);
	
	void
	MUrMatrix4x3_Multiply3x3(
		const M3tMatrix4x3*		inMatrix4x3,
		const M3tMatrix3x3*		inMatrix3x3,
		M3tMatrix4x3			*outMatrix4x3);
	
	void
	MUrMatrix4x3_BuildFrom3x3(
		M3tMatrix4x3			*outMatrix4x3,
		const M3tMatrix3x3*		inMatrix3x3);

void MUrMatrix3x3_Identity(M3tMatrix3x3 *ioMatrix);

void MUrMatrix3x3_FormOrthonormalBasis(M3tVector3D *inX, M3tVector3D *inY, M3tVector3D *inZ,
														M3tMatrix3x3 *outMatrix);

/*
 * Matrix4x4 operations
 */
	void
	MUrMath_Matrix4x4Multiply(
		M3tMatrix4x4*	inMatrixA,
		M3tMatrix4x4*	inMatrixB,
		M3tMatrix4x4	*outResult);

	void
	MUrMath_Matrix4x4Multiply4x3(
		M3tMatrix4x4*	inMatrixA,
		M3tMatrix4x3*	inMatrixB,
		M3tMatrix4x4	*outResult);



void
MUrPointVector_FlipAboutPlane(
	M3tPoint3D*			inPoint,
	M3tVector3D*		inVector,
	M3tPlaneEquation*	inPlane,
	M3tPoint3D			*outReflectedPoint,
	M3tVector3D			*outReflectedVector);


#ifdef __cplusplus
}
#endif

#endif __BFW_MATHUTIL_H__
