/*
	FILE:	BFW_Math.c

	AUTHOR:	Michael Evans

	CREATED: January 9, 1998

	PURPOSE:

	Copyright 1998

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "BFW_Collision.h"
#include "BFW_Timer.h"

#include "EulerAngles.h"
#include "Decompose.h"


#define MUcQuatDelta 1e-6     // error tolerance

const M3tQuaternion MUgZeroQuat = { 0.f, 0.f, 0.f, 1.f };
const M3tPoint3D MUgZeroPoint = { 0.f, 0.f, 0.f };
const M3tVector3D MUgZeroVector = { 0.f, 0.f, 0.f };

const float MUgSign256[256] = {
	0.000000f,	0.024541f,	0.049068f,	0.073565f,	0.098017f,	0.122411f,	0.146730f,	0.170962f,
	0.195090f,	0.219101f,	0.242980f,	0.266713f,	0.290285f,	0.313682f,	0.336890f,	0.359895f,
	0.382683f,	0.405241f,	0.427555f,	0.449611f,	0.471397f,	0.492898f,	0.514103f,	0.534998f,
	0.555570f,	0.575808f,	0.595699f,	0.615232f,	0.634393f,	0.653173f,	0.671559f,	0.689541f,
	0.707107f,	0.724247f,	0.740951f,	0.757209f,	0.773010f,	0.788346f,	0.803208f,	0.817585f,
	0.831470f,	0.844854f,	0.857729f,	0.870087f,	0.881921f,	0.893224f,	0.903989f,	0.914210f,
	0.923880f,	0.932993f,	0.941544f,	0.949528f,	0.956940f,	0.963776f,	0.970031f,	0.975702f,
	0.980785f,	0.985278f,	0.989177f,	0.992480f,	0.995185f,	0.997290f,	0.998795f,	0.999699f,
	1.000000f,	0.999699f,	0.998795f,	0.997290f,	0.995185f,	0.992480f,	0.989177f,	0.985278f,
	0.980785f,	0.975702f,	0.970031f,	0.963776f,	0.956940f,	0.949528f,	0.941544f,	0.932993f,
	0.923880f,	0.914210f,	0.903989f,	0.893224f,	0.881921f,	0.870087f,	0.857729f,	0.844854f,
	0.831470f,	0.817585f,	0.803207f,	0.788346f,	0.773010f,	0.757209f,	0.740951f,	0.724247f,
	0.707107f,	0.689540f,	0.671559f,	0.653173f,	0.634393f,	0.615232f,	0.595699f,	0.575808f,
	0.555570f,	0.534998f,	0.514103f,	0.492898f,	0.471397f,	0.449611f,	0.427555f,	0.405241f,
	0.382683f,	0.359895f,	0.336890f,	0.313682f,	0.290285f,	0.266713f,	0.242980f,	0.219101f,
	0.195090f,	0.170962f,	0.146730f,	0.122411f,	0.098017f,	0.073564f,	0.049068f,	0.024541f,
	-0.000000f,	-0.024541f,	-0.049068f,	-0.073565f,	-0.098017f,	-0.122411f,	-0.146731f,	-0.170962f,
	-0.195090f,	-0.219101f,	-0.242980f,	-0.266713f,	-0.290285f,	-0.313682f,	-0.336890f,	-0.359895f,
	-0.382684f,	-0.405241f,	-0.427555f,	-0.449611f,	-0.471397f,	-0.492898f,	-0.514103f,	-0.534998f,
	-0.555570f,	-0.575808f,	-0.595699f,	-0.615232f,	-0.634393f,	-0.653173f,	-0.671559f,	-0.689541f,
	-0.707107f,	-0.724247f,	-0.740951f,	-0.757209f,	-0.773011f,	-0.788346f,	-0.803208f,	-0.817585f,
	-0.831470f,	-0.844854f,	-0.857729f,	-0.870087f,	-0.881921f,	-0.893224f,	-0.903989f,	-0.914210f,
	-0.923880f,	-0.932993f,	-0.941544f,	-0.949528f,	-0.956940f,	-0.963776f,	-0.970031f,	-0.975702f,
	-0.980785f,	-0.985278f,	-0.989177f,	-0.992480f,	-0.995185f,	-0.997290f,	-0.998795f,	-0.999699f,
	-1.000000f,	-0.999699f,	-0.998795f,	-0.997290f,	-0.995185f,	-0.992480f,	-0.989176f,	-0.985278f,
	-0.980785f,	-0.975702f,	-0.970031f,	-0.963776f,	-0.956940f,	-0.949528f,	-0.941544f,	-0.932993f,
	-0.923879f,	-0.914210f,	-0.903989f,	-0.893224f,	-0.881921f,	-0.870087f,	-0.857729f,	-0.844853f,
	-0.831470f,	-0.817585f,	-0.803207f,	-0.788346f,	-0.773010f,	-0.757209f,	-0.740951f,	-0.724247f,
	-0.707107f,	-0.689540f,	-0.671559f,	-0.653173f,	-0.634393f,	-0.615231f,	-0.595699f,	-0.575808f,
	-0.555570f,	-0.534997f,	-0.514103f,	-0.492898f,	-0.471397f,	-0.449611f,	-0.427555f,	-0.405241f,
	-0.382683f,	-0.359895f,	-0.336890f,	-0.313682f,	-0.290285f,	-0.266713f,	-0.242980f,	-0.219101f,
	-0.195090f,	-0.170962f,	-0.146730f,	-0.122411f,	-0.098017f,	-0.073564f,	-0.049068f,	0.000000f
	};


static void MUrRandomMatrix(M3tMatrix4x3 *outMatrix)
{
	float x,y,z;

	x = -50 + UUmLocalRandomRange(0, 10000) * 100 * 0.00001f; // S.S. / 10000.f;
	y = -50 + UUmLocalRandomRange(0, 10000) * 100 * 0.00001f; // S.S. / 10000.f;
	z = -50 + UUmLocalRandomRange(0, 10000) * 100 * 0.00001f; // S.S. / 10000.f;

	MUrMatrix_BuildTranslate(x,y,z,outMatrix);

	x = UUmLocalRandomRange(0, 10000) * M3c2Pi * 0.00001f; // S.S. / 10000.f;
	y = UUmLocalRandomRange(0, 10000) * M3c2Pi * 0.00001f; // S.S. / 10000.f;
	z = UUmLocalRandomRange(0, 10000) * M3c2Pi * 0.00001f; // S.S. / 10000.f;

	MUrMatrix_RotateX(outMatrix, x);
	MUrMatrix_RotateY(outMatrix, y);
	MUrMatrix_RotateZ(outMatrix, z);

	return;
}

static void MUrRandomVector(M3tVector3D *ioVector)
{
	float x,y,z;


	x = -50 + UUmLocalRandomRange(0, 10000) * 100 * 0.00001f; // S.S. / 10000.f;
	y = -50 + UUmLocalRandomRange(0, 10000) * 100 * 0.00001f; // S.S. / 10000.f;
	z = -50 + UUmLocalRandomRange(0, 10000) * 100 * 0.00001f; // S.S. / 10000.f;

	ioVector->x = x;
	ioVector->y = y;
	ioVector->z = z;

	return;
}


static void BlowCache(void)
{
	static UUtUns32 *blow = NULL;
	UUtUns32 itr;

	if (NULL == blow) {
		blow = UUrMemory_Block_New(1024 * 1024 * 4);
	}

	for(itr = 0; itr < (1024 * 1024); itr++) {
		blow[itr] += 1;
	}

	return;
}

static void MUrTest(void)
{
	UUtInt64 time1 = 0;
	UUtInt64 time2 = 0;
	double f1,f2;
	UUtUns16 itr;
	float x = 0.5f;
	float y = 1.0f;
	float z = 1.5f;

	M3tMatrix4x3 matA, matB;
	MUtEuler eulA, eulB;
	M3tQuaternion quatA, quatB;

	// test euler vs rotation
	eulA.x = x;
	eulA.y = y;
	eulA.z = z;
	eulA.order = MUcEulerOrderXYZs;

	MUrEulerToMatrix(&eulA, &matA);

	MUrMatrix_Identity(&matB);
	MUrMatrix_RotateX(&matB, x);
	MUrMatrix_RotateY(&matB, y);
	MUrMatrix_RotateZ(&matB, z);

	UUmAssert(MUrMatrix_IsEqual(&matA, &matB));

	// convert to euler
	eulB = MUrMatrixToEuler(&matB, MUcEulerOrderXYZs);
	UUmAssert(MUrEuler_IsEqual(&eulA, &eulB));

	// convert to matricies
	quatA = MUrEulerToQuat(&eulA);
	MUrEulerToMatrix(&eulA, &matA);
	UUmAssert(MUrMatrix_IsEqual(&matA, &matB));

	// convert one to a quaternion via matrix the other via euler
	MUrMatrixToQuat(&matB, &quatB);

	eulA = MUrMatrixToEuler(&matA, MUcEulerOrderXYZs);
	quatA = MUrEulerToQuat(&eulA);
	UUmAssert(MUrQuat_IsEqual(&quatA, &quatB));

	MUrQuatToMatrix(&quatA, &matA);
	MUrQuatToMatrix(&quatB, &matB);
	UUmAssert(MUrMatrix_IsEqual(&matA, &matB));

	// convert to matrix and back
	MUrQuatToMatrix(&quatA, &matA);
	MUrMatrixToQuat(&matA, &quatA);
	UUmAssert(MUrQuat_IsEqual(&quatA, &quatB));

	// convert to euler and back
	eulA = MUrQuatToEuler(&quatA, MUcEulerOrderXYZs);
	quatA = MUrEulerToQuat(&eulA);
	UUmAssert(MUrQuat_IsEqual(&quatA, &quatB));

	for(itr = 0; itr < 50000; itr++)
	{
		M3tMatrix4x3 a;
		M3tMatrix4x3 r1,r2;
		M3tVector3D vec;

		MUrRandomMatrix(&a);
		r1 = a;
		r2 = a;
		MUrRandomVector(&vec);

		time1 -= UUrMachineTime_High();
		// old
		time1 += UUrMachineTime_High();

		time2 -= UUrMachineTime_High();
		// new
		time2 += UUrMachineTime_High();


		if (!MUrMatrix_IsEqual(&r1, &r2)) {
			 UUrDebuggerMessage("failure");
		}
	}

	f1 = (double) time1;
	f2 = (double) time2;

	f1 /= UUrMachineTime_High_Frequency();
	f2 /= UUrMachineTime_High_Frequency();

	UUrDebuggerMessage("old %f new %f", f1, f2);


	return;
}

void MUrInitialize(void)
{
	// MUrTest();
}

void MUrTerminate(void)
{
}

// returns parametric value u
float MUrLineSegment_ClosestPoint(
	const M3tPoint3D			*inLineP1,
	const M3tPoint3D			*inLineP2,
	const M3tPoint3D			*inPoint)
{
	float u;

	float x1 = inLineP1->x;
	float x2 = inLineP2->x;
	float x3 = inPoint->x;

	float y1 = inLineP1->y;
	float y2 = inLineP2->y;
	float y3 = inPoint->y;

	float z1 = inLineP1->z;
	float z2 = inLineP2->z;
	float z3 = inPoint->z;

	u = ((x3 - x1) * (x2 - x1) + (y3 - y1) * (y2 - y1) + (z3 - z1) * (z2 - z1)) /
		((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));

	// NOTE: this can be made more by calculating x2-x1, y2-y1, z2-z1 only once each

	return u;
}

//
void MUrLineSegment_ComputePoint(
	const M3tPoint3D			*inLineP1,
	const M3tPoint3D			*inLineP2,
	      float					inU,
	      M3tPoint3D			*outPoint)
{
	M3tPoint3D 					P2MinusP1;

	P2MinusP1.x = inLineP2->x - inLineP1->x;
	P2MinusP1.y = inLineP2->y - inLineP1->y;
	P2MinusP1.z = inLineP2->z - inLineP1->z;

	outPoint->x = inLineP1->x + inU * P2MinusP1.x;
	outPoint->y = inLineP1->y + inU * P2MinusP1.y;
	outPoint->z = inLineP1->z + inU * P2MinusP1.z;

	return;
}

void MUrLineSegment_ClosestPointToPoint(
	M3tPoint3D *inLineP1,
	M3tPoint3D *inLineP2,
	M3tPoint3D *inPoint,
	M3tPoint3D *outClosestPoint)
{
	// Returns closest point on line to point
	float u = MUrLineSegment_ClosestPoint(inLineP1,inLineP2,inPoint);

	if (u<0.0f) *outClosestPoint = *inLineP1;
	else if (u>1.0f) *outClosestPoint = *inLineP2;
	else MUrLineSegment_ComputePoint(inLineP1,inLineP2,u,outClosestPoint);
}

#if !INLINE_TRIG
float MUrSin(float f)
{
	float result;

	UUmAssertTrigRange(f);

	result = (float) sin(f);

	return result;
}

float MUrCos(float f)
{
	float result;

	UUmAssertTrigRange(f);

	result = (float) cos(f);

	return result;
}

float MUrTan(float f)
{
	float result;

	result = (float) tan(f);

	return result;
}

float MUrATan2(float x, float y)
{
	float atan2_result = (float) atan2(x, y);

	return atan2_result;
}

#endif

#if !INLINE_SQRT

float MUrSqrt(float f)
{
	float result = (float) sqrt(f);

	return result;
}

float MUrOneOverSqrt(float f)
{
	float result = (float) sqrt(f);

	result = 1.f / result;

	return result;
}

#endif

float MUrATan(float f)
{
	float result = (float) atan(f);

	return result;
}

float MUrPoint_Length(M3tPoint3D *inPoint)
{
	float length = (float) MUrSqrt(UUmSQR(inPoint->x) + UUmSQR(inPoint->y) + UUmSQR(inPoint->z));

	return length;
}

float MUrPoint_ManhattanLength(M3tPoint3D *inPoint)
{
	float length = (float)(fabs(inPoint->x) + fabs(inPoint->y) + fabs(inPoint->z));

	return length;
}

UUtBool MUrPoint_PointOnPlaneSloppy(M3tPoint3D *inPoint, M3tPlaneEquation *inPlane)
{
	// Returns whether the point is roughly in the plane

	if(UUmFloat_CompareEquSloppy(
		inPlane->a * inPoint->x +
		inPlane->b * inPoint->y +
		inPlane->c * inPoint->z +
		inPlane->d, 0.0f))

		return UUcTrue;

	return UUcFalse;
}

UUtBool MUrPoint_PointBehindPlaneSloppy(M3tPoint3D *inPoint, M3tPlaneEquation *inPlane)
{
	// Returns whether the point is roughly behind the plane

	if(UUmFloat_CompareLTSloppy(
		inPlane->a * inPoint->x +
		inPlane->b * inPoint->y +
		inPlane->c * inPoint->z +
		inPlane->d,0.0f))

		return UUcTrue;

	return UUcFalse;
}


UUtBool MUrPoint_PointOnPlane(M3tPoint3D *inPoint, M3tPlaneEquation *inPlane)
{
	// Returns whether the point is in the plane

	if(UUmFloat_CompareEqu(
		inPlane->a * inPoint->x +
		inPlane->b * inPoint->y +
		inPlane->c * inPoint->z +
		inPlane->d, 0.0f))

		return UUcTrue;

	return UUcFalse;
}

UUtBool MUrPoint_PointBehindPlane(M3tPoint3D *inPoint, M3tPlaneEquation *inPlane)
{
	// Returns whether the point is behind the plane

	if(UUmFloat_CompareLT(
		inPlane->a * inPoint->x +
		inPlane->b * inPoint->y +
		inPlane->c * inPoint->z +
		inPlane->d,0.0f))

		return UUcTrue;

	return UUcFalse;
}

UUtBool MUrPoint_PointOnOrBehindPlane(M3tPoint3D *inPoint, M3tPlaneEquation *inPlane)
{
	// Returns whether the point is behind or in the plane

	if(UUmFloat_CompareLE(
		inPlane->a * inPoint->x +
		inPlane->b * inPoint->y +
		inPlane->c * inPoint->z +
		inPlane->d, 0.0f))

		return UUcTrue;

	return UUcFalse;
}

void MUrVector_Increment_Unique(M3tVector3D *inDest, M3tVector3D *inSrc, M3tVector3D *inList, UUtUns16 *inCount)
{
	// Increments dest by src if src doesn't already exist in the list
	// defined by 'inList' and 'inCount'. inSrc is added to that list afterwards.

	UUtUns16 c;
	M3tVector3D *curV;

	for (c=0; c<*inCount; c++)
	{
		curV = &inList[c];
		if (UUmFloat_CompareEqu(curV->x,inSrc->x) &&
			UUmFloat_CompareEqu(curV->y,inSrc->y) &&
			UUmFloat_CompareEqu(curV->z,inSrc->z))
		{
			break;
		}
	}
	if (c==*inCount)
	{
		MUmVector_Increment(*inDest,*inSrc);
		inList[c] = *inSrc;
		(*inCount)++;
	}
}

void MUrVector_ProjectToPlane(
	const M3tPlaneEquation*		inPlaneEqu,
	const M3tVector3D*			inVector,
	M3tVector3D*				outVector)
{
	// Projects 'inVector' into the plane
	float				distance;

	UUmAssertReadPtr(inPlaneEqu, sizeof(M3tPlaneEquation));
	UUmAssertReadPtr(inVector, sizeof(M3tVector3D));
	UUmAssertWritePtr(outVector, sizeof(M3tVector3D));

	distance = MUrSqrt(UUmSQR(inPlaneEqu->a) + UUmSQR(inPlaneEqu->b) + UUmSQR(inPlaneEqu->c));
	UUmAssert((distance >= 0.95f) && (distance <= 1.05f));

	distance =
		(inPlaneEqu->a * inVector->x) +
		(inPlaneEqu->b * inVector->y) +
		(inPlaneEqu->c * inVector->z);

	outVector->x = inVector->x - (distance * inPlaneEqu->a);
	outVector->y = inVector->y - (distance * inPlaneEqu->b);
	outVector->z = inVector->z - (distance * inPlaneEqu->c);

	return;
}


float MUrVector_Length(M3tVector3D *inPoint)
{
	float length = MUrSqrt(UUmSQR(inPoint->x) + UUmSQR(inPoint->y) + UUmSQR(inPoint->z));

	return length;
}

void MUrPoint_RotateAboutAxis(
	const M3tPoint3D*	inPoint,
	const M3tVector3D*	inVector,
	float				inRadians,
	M3tPoint3D 			*outPoint)
{
	M3tMatrix4x3	matrix;

	MUrMatrix_BuildRotate(
		inRadians,
		inVector->x,
		inVector->y,
		inVector->z,
		&matrix);

	MUrMatrix_MultiplyPoint(
		inPoint,
		&matrix,
		outPoint);

	return;
}

void MUrPoint_RotateXAxis(const M3tPoint3D *inPoint, float radians, M3tPoint3D *outPoint)
{
	float radians2;
	float thesin, thecos;
	float thesin2, thecos2;
	float inX, inY, inZ;

	UUmAssertReadPtr(inPoint, sizeof(*inPoint));
	UUmAssertWritePtr(outPoint, sizeof(*outPoint));

	inX = inPoint->x;
	inY = inPoint->y;
	inZ = inPoint->z;

	radians2 = radians + M3cHalfPi;
	if (radians2 > M3c2Pi) {
		radians2 -= M3c2Pi;
	}

	thesin = MUrSin(radians);
	thecos = MUrCos(radians);

	thesin2 = MUrSin(radians2);
	thecos2 = MUrCos(radians2);

	outPoint->y = (thecos * inY) + (thecos2 * inZ);
	outPoint->z = (thesin * inY) + (thesin2 * inZ);
	outPoint->x = inPoint->x;

	return;
}

void MUrPoint_RotateYAxis(const M3tPoint3D *inPoint, float inRadians, M3tPoint3D *outPoint)
{
	float radians = inRadians;
	float radians2;
	float sin1, cos1;
	float sin2, cos2;
	float inX, inY, inZ;

	UUmAssertReadPtr(inPoint, sizeof(*inPoint));
	UUmAssertWritePtr(outPoint, sizeof(*outPoint));

	inX = inPoint->x;
	inY = inPoint->y;
	inZ = inPoint->z;

	radians = M3c2Pi - radians;
	if (radians > M3c2Pi) {
		radians -= M3c2Pi;
	}

	radians2 = radians + M3cHalfPi;
	if (radians2 > M3c2Pi) {
		radians2 -= M3c2Pi;
	}

	sin1 = -MUrSin(inRadians);
	cos2 = -sin1;

	cos1 = MUrCos(inRadians);
	sin2 = cos1;

	outPoint->x = (cos1 * inX) + (cos2 * inZ);
	outPoint->z = (sin1 * inX) + (sin2 * inZ);
	outPoint->y = inY;

	return;
}

void MUrPoint_RotateZAxis(const M3tPoint3D *inPoint, float radians, M3tPoint3D *outPoint)
{
	float radians2;
	float thesin, thecos;
	float thesin2, thecos2;
	float inX, inY, inZ;

	UUmAssertReadPtr(inPoint, sizeof(*inPoint));
	UUmAssertWritePtr(outPoint, sizeof(*outPoint));

	inX = inPoint->x;
	inY = inPoint->y;
	inZ = inPoint->z;

	radians2 = radians + M3cHalfPi;
	if (radians2 > M3c2Pi) {
		radians2 -= M3c2Pi;
	}

	thesin = MUrSin(radians);
	thecos = MUrCos(radians);

	thesin2 = MUrSin(radians2);
	thecos2 = MUrCos(radians2);

	outPoint->x = (thecos * inX) + (thecos2 * inY);
	outPoint->y = (thesin * inX) + (thesin2 * inY);
	outPoint->z = inZ;

	return;
}

void
MUrPoint_Align(
	const M3tVector3D*	inFrom,
	const M3tVector3D*	inTo,
	M3tPoint3D			*outPoint)
{
	M3tMatrix4x3	matrix;

	MUrMatrix_Alignment(
		inFrom,
		inTo,
		&matrix);

	MUrMatrix_MultiplyPoint(
		outPoint,
		&matrix,
		outPoint);

	return;
}

#ifndef USE_FAST_MATH
void MUrNormalize(M3tVector3D *ioVector)
{
	/* Normalizes 'ioVector' */

	float oneOverLength;
	float length;

	length = MUmVector_GetLength(*ioVector);

	if ((ioVector->x > -1e-20f) && (ioVector->x < 1e-20f) &&
		(ioVector->y > -1e-20f) && (ioVector->y < 1e-20f) &&
		(ioVector->z > -1e-20f) && (ioVector->z < 1e-20f))
	{
		ioVector->x *= 1e20f;
		ioVector->y *= 1e20f;
		ioVector->z *= 1e20f;
		length = MUmVector_GetLength(*ioVector);
	}

	oneOverLength = 1.0f / length;

	ioVector->x *= oneOverLength;
	ioVector->y *= oneOverLength;
	ioVector->z *= oneOverLength;

	UUmAssert(ioVector->x > -2.f);
	UUmAssert(ioVector->x <  2.f);

	UUmAssert(ioVector->y > -2.f);
	UUmAssert(ioVector->y <  2.f);

	UUmAssert(ioVector->z > -2.f);
	UUmAssert(ioVector->z <  2.f);


	return;
}
#endif

float MUrPointsXYAngle(float inFromX, float inFromY, float inToX, float inToY)
{
	float x = inToX - inFromX;
	float y = inToY - inFromY;
	float len = MUrSqrt((x * x) + (y * y));

	float result;

	x /= len;

	result = (float) MUrACos(x);

	if (y < 0)
	{
		result = M3c2Pi - result;
	}

	return result;
}

float MUrAngleBetweenVectors(float inAx, float inAy, float inBx, float inBy)
{
	/* Returns angle between A and B in radians */

	float dot,absA,absB,denom;
	float result;
	float dotOverDenom;

	dot = inAx*inBx + inAy*inBy;
	absA = MUrSqrt(inAx*inAx + inAy*inAy);
	absB = MUrSqrt(inBx*inBx + inBy*inBy);
	denom = absA * absB;
	dotOverDenom = dot / denom;

	UUmAssert((denom > 0) && (denom < 1e9) && (denom > -1e9));
	UUmAssert((dot < 1e9) && (dot > -1e9));
	UUmAssert(denom != 0);

	// never should be too far off
	UUmAssert(dotOverDenom > -1.1f);
	UUmAssert(dotOverDenom < 1.1f);

	// valid range of acos is 0..1
	dotOverDenom = UUmPin(dotOverDenom, -1.f, 1.f);

	result = (float) MUrACos(dotOverDenom);

	UUmAssert(result >= 0);
	UUmAssert(result <= M3cPi);

	return result;
}

UUtBool MUrPoint_Distance_Less(const M3tPoint3D *inA, const M3tPoint3D *inB, float amount)
{
	UUtBool result;

	result = MUrPoint_Distance_Squared(inA, inB) < UUmSQR(amount);

	return result;
}

UUtBool MUrPoint_Distance_LessOrEqual(const M3tPoint3D *inA, const M3tPoint3D *inB, float amount)
{
	UUtBool result;

	result = MUrPoint_Distance_Squared(inA, inB) <= UUmSQR(amount);

	return result;
}

float MUrPoint_Distance_Squared(const M3tPoint3D *inA, const M3tPoint3D *inB)
{
	/* Returns the distance between 'A' and 'B' */

	M3tPoint3D delta;
	float distanceSquared;

	UUmAssertReadPtr(inA, sizeof(M3tPoint3D));
	UUmAssertReadPtr(inB, sizeof(M3tPoint3D));

	delta.x = inB->x - inA->x;
	delta.y = inB->y - inA->y;
	delta.z = inB->z - inA->z;

	distanceSquared = UUmSQR(delta.x) + UUmSQR(delta.y) + UUmSQR(delta.z);

	return distanceSquared;
}

float MUrPoint_Distance_SquaredXZ(const M3tPoint3D *inA, const M3tPoint3D *inB)
{
	/* Returns the distance between 'A' and 'B' in the XZ plane */

	M3tPoint3D delta;
	float distanceSquared;

	UUmAssertReadPtr(inA, sizeof(M3tPoint3D));
	UUmAssertReadPtr(inB, sizeof(M3tPoint3D));

	delta.x = inB->x - inA->x;
	delta.z = inB->z - inA->z;

	distanceSquared = UUmSQR(delta.x) + UUmSQR(delta.z);

	return distanceSquared;
}

float MUrPoint_Distance(const M3tPoint3D *inA, const M3tPoint3D *inB)
{
	/* Returns the distance between 'A' and 'B' */

	M3tPoint3D delta;
	float distance;

	UUmAssertReadPtr(inA, sizeof(M3tPoint3D));
	UUmAssertReadPtr(inB, sizeof(M3tPoint3D));

	delta.x = inB->x - inA->x;
	delta.y = inB->y - inA->y;
	delta.z = inB->z - inA->z;

	distance = MUrPoint_Length(&delta);

	return distance;
}

float MUrPoint_ManhattanDistance(const M3tPoint3D *inA, const M3tPoint3D *inB)
{
	/* Returns the distance between 'A' and 'B' */

	M3tPoint3D delta;
	float distance;

	UUmAssertReadPtr(inA, sizeof(M3tPoint3D));
	UUmAssertReadPtr(inB, sizeof(M3tPoint3D));

	delta.x = inB->x - inA->x;
	delta.y = inB->y - inA->y;
	delta.z = inB->z - inA->z;

	distance = MUrPoint_ManhattanLength(&delta);

	return distance;
}

float MUrPoint_ManhattanDistanceXZ(const M3tPoint3D *inA, const M3tPoint3D *inB)
{
	/* Returns the distance between 'A' and 'B' in the XZ plane */

	M3tPoint3D delta;
	float distance;

	UUmAssertReadPtr(inA, sizeof(M3tPoint3D));
	UUmAssertReadPtr(inB, sizeof(M3tPoint3D));

	delta.x = inB->x - inA->x;
	delta.y = 0;
	delta.z = inB->z - inA->z;

	distance = MUrPoint_ManhattanLength(&delta);

	return distance;
}

float MUrPoint_ManhattanDistanceToQuad(
	M3tPoint3D *inPoint,
	M3tPoint3D *inPointArray,
	M3tQuad *inQuad)
{
	// Returns the average of the manhattan distances from the point to
	// each vertex of the quad

	float accum = 0.0;
	UUtUns16 c;

	for (c=0; c<4; c++)
	{
		accum += MUrPoint_ManhattanDistance((const M3tPoint3D *)inPoint,(const M3tPoint3D *)
					(&inPointArray[inQuad->indices[c]]));
	}

	return accum/4.0f;
}

float MUrPoint_DistanceToQuad(
	M3tPoint3D *inPoint,
	M3tPoint3D *inPointArray,
	M3tQuad *inQuad)
{
	// Returns the average of the manhattan distances from the point to
	// each vertex of the quad

	float accum = 0.0;
	UUtUns16 c;

	for (c=0; c<4; c++)
	{
		accum += MUrPoint_Distance((const M3tPoint3D *)inPoint,(const M3tPoint3D *)
					(&inPointArray[inQuad->indices[c]]));
	}

	return accum/4.0f;
}

UUtBool
MUrAngleLess180(
	M3tVector3D*	inVectA,
	M3tVector3D*	inVectB)
{
	UUmAssertReadPtr(inVectA, sizeof(M3tVector3D));
	UUmAssertReadPtr(inVectB, sizeof(M3tVector3D));

	if((inVectA->x != 0.0f || inVectA->y != 0.0f) &&
		(inVectB->x != 0.0f || inVectB->y != 0.0f))
	{
		// Throw away Z
		return ((inVectA->x * inVectB->y - inVectB->x * inVectA->y) < 0.0F);
	}
	else if((inVectA->x != 0.0f || inVectA->z != 0.0f) &&
		(inVectB->x != 0.0f || inVectB->z != 0.0f))
	{
		// Throw away Y
		return ((inVectA->x * inVectB->z - inVectB->x * inVectA->z) < 0.0F);
	}
	else if((inVectA->z != 0.0f || inVectA->y != 0.0f) &&
		(inVectB->z != 0.0f || inVectB->y != 0.0f))
	{
		// Throw away X
		return ((inVectA->z * inVectB->y - inVectB->z * inVectA->y) < 0.0F);
	}
	else
	{
		UUmAssert(!"doh!");
	}

	return UUcFalse;
}

///////////////////////////////////////////////////////////////////////////////
// Function:	EulerToQuaternion
// Purpose:		Convert a set of Euler angles to a Quaternion
// Arguments:	A rotation set of 3 angles, a quaternion to set
// Discussion:  As the order of rotations is important I am
//				using the Quantum Mechanics convention of (X,Y,Z)
//				a Yaw-Pitch-Roll (Y,X,Z) system would have to be
//				adjusted.  It is more efficient this way though.
///////////////////////////////////////////////////////////////////////////////
void MUrXYZEulerToQuat(float x, float y, float z, M3tQuaternion *quat)
{
/// Local Variables ///////////////////////////////////////////////////////////
	float tx,ty,tz,cx,cy,cz,sx,sy,sz,cc,cs,sc,ss;

	// GET THE HALF ANGLES
	tx = x * 0.5f;
	ty = y * 0.5f;
	tz = z * 0.5f;

	cx = MUrCos(tx);
	cy = MUrCos(ty);
	cz = MUrCos(tz);
	sx = MUrSin(tx);
	sy = MUrSin(ty);
	sz = MUrSin(tz);

	cc = cx * cz;
	cs = cx * sz;
	sc = sx * cz;
	ss = sx * sz;

	quat->x = (cy * sc) - (sy * cs);
	quat->y = (cy * ss) + (sy * cc);
	quat->z = (cy * cs) - (sy * sc);
	quat->w = (cy * cc) + (sy * ss);

	MUmQuat_VerifyUnit(quat);

	return;
}

/*SDOC***********************************************************************

  Name:		gluEulerToQuat

  Action:   Converts representation of a rotation from Euler angles to
			quaternion representation

  Params:   float (roll), float (pitch), float (yaw), M3tQuaternion* (quat)

  Returns:  nothing

  Comments: remember:	roll  - rotation around X axis
						pitch - rotation around Y axis
						yaw   - rotation around Z axis

			rotations are performed in the following order:
					yaw(z) -> pitch(y) -> roll(x)

			Qfinal = Qyaw Qpitch Qroll

***********************************************************************EDOC*/
void MUrZYXEulerToQuat(float x, float y, float z, M3tQuaternion *quat)
{
	float cr, cp, cy, sr, sp, sy, cpcy, spsy;

	cr = MUrCos(x/2);
	cp = MUrCos(y/2);
	cy = MUrCos(z/2);

	sr = MUrSin(x/2);
	sp = MUrSin(y/2);
	sy = MUrSin(z/2);

	cpcy = cp * cy;
	spsy = sp * sy;

	quat->w = cr * cpcy + sr * spsy;
	quat->x = sr * cpcy - cr * spsy;
	quat->y = cr * sp * cy + sr * cp * sy;
	quat->z = cr * cp * sy - sr * sp * cy;

	return;
}

/*SDOC***********************************************************************

  Name:		MUrQuat_Slerp

  Action:	Smoothly (spherically, shortest path on a quaternion sphere)
			interpolates between two UNIT quaternion positions

  Params:   GLQUAT (first and second quaternion), float (interpolation
			parameter [0..1]), M3tQuaternion (resulting quaternion; inbetween)

  Returns:  nothing

  Comments: Most of this code is optimized for speed and not for readability

			As t goes from 0 to 1, qt goes from p to q.
		slerp(p,q,t) = (p*sin((1-t)*omega) + q*sin(t*omega)) / sin(omega)

***********************************************************************EDOC*/
void MUrQuat_Slerp(const M3tQuaternion * from, const M3tQuaternion * to, float t,
															M3tQuaternion * res)
{
    float           to1[4];
    float          omega, cosom, sinom;
    float          scale0, scale1;

	UUmAssert(!"slerp should not be called at the moment");

	MUmQuat_VerifyUnit(from);
	MUmQuat_VerifyUnit(to);

    // calc cosine
    cosom = from->x * to->x + from->y * to->y + from->z * to->z
			   + from->w * to->w;

    // adjust signs (if necessary)
    if ( cosom < 0.0 )
	{
		cosom = -cosom;

		to1[0] = - to->x;
		to1[1] = - to->y;
		to1[2] = - to->z;
		to1[3] = - to->w;

    } else  {

		to1[0] = to->x;
		to1[1] = to->y;
		to1[2] = to->z;
		to1[3] = to->w;

    }

    // calculate coefficients

    if ( (1.0 - cosom) > MUcQuatDelta )
	{
            // standard case (slerp)
            omega = MUrACos(cosom);
            sinom = MUrSin(omega);
            scale0 = MUrSin((1.f - t) * omega) / sinom;
            scale1 = MUrSin(t * omega) / sinom;

    } else {
			// "from" and "to" quaternions are very close
			//  ... so we can do a linear interpolation

            scale0 = 1.f - t;
            scale1 = t;
    }

	// calculate final values
	res->x = (scale0 * from->x + scale1 * to1[0]);
	res->y = (scale0 * from->y + scale1 * to1[1]);
	res->z = (scale0 * from->z + scale1 * to1[2]);
	res->w = (scale0 * from->w + scale1 * to1[3]);

	MUmQuat_VerifyUnit(res);

	return;
}


/*SDOC***********************************************************************

  Name:		MUrQuat_Lerp

  Action:   Linearly interpolates between two quaternion positions

  Params:   GLQUAT (first and second quaternion), float (interpolation
			parameter [0..1]), M3tQuaternion (resulting quaternion; inbetween)

  Returns:  nothing

  Comments: fast but not as nearly as smooth as Slerp

***********************************************************************EDOC*/
void MUrQuat_Lerp(const M3tQuaternion *from, const M3tQuaternion *to, float t, M3tQuaternion *res)
{
    float           to1[4];
    double          cosom;
    double          scale0, scale1;

	MUmQuat_VerifyUnit(from);
	MUmQuat_VerifyUnit(to);

    // calc cosine
    cosom = from->x * to->x + from->y * to->y + from->z * to->z
			   + from->w * to->w;

    // adjust signs (if necessary)
    if ( cosom < 0.0 )
	{
		to1[0] = - to->x;
		to1[1] = - to->y;
		to1[2] = - to->z;
		to1[3] = - to->w;

    } else  {

		to1[0] = to->x;
		to1[1] = to->y;
		to1[2] = to->z;
		to1[3] = to->w;

    }


	// interpolate linearly
    scale0 = 1.0 - t;
    scale1 = t;

	// calculate final values
	res->x = (float) (scale0 * from->x + scale1 * to1[0]);
	res->y = (float) (scale0 * from->y + scale1 * to1[1]);
	res->z = (float) (scale0 * from->z + scale1 * to1[2]);
	res->w = (float) (scale0 * from->w + scale1 * to1[3]);

	// ? is there a faster way to make this work then normalizing ?
	MUrQuat_Normalize(res);
	MUmQuat_VerifyUnit(res);

	return;
}



/*SDOC***********************************************************************

  Name:		MUrQuat_Normalize

  Action:   Normalizes quaternion (i.e. w^2 + x^2 + y^2 + z^2 = 1)

  Params:   M3tQuaternion* (quaternion)

  Returns:  nothing

  Comments: none

***********************************************************************EDOC*/
#ifndef USE_FAST_MATH
void MUrQuat_Normalize(M3tQuaternion *quat)
{
    float	dist, square;

	square = quat->x * quat->x + quat->y * quat->y + quat->z * quat->z
		+ quat->w * quat->w;

	if (square > 0.0)
	{
		dist = MUrOneOverSqrt(square);

		quat->x *= dist;
		quat->y *= dist;
		quat->z *= dist;
		quat->w *= dist;
	}
	else {
		quat->x = 0.f;
		quat->y = 0.f;
		quat->z = 0.f;
		quat->w = 1.f;
	}

	return;
}
#endif


/*SDOC***********************************************************************

  Name:		MUrQuat_GetValue

  Action:   Disassembles quaternion to an axis and an angle

  Params:   M3tQuaternion* (quaternion), float* (x, y, z - axis), float (angle)

  Returns:  nothing

  Comments: NOTE: vector has been split into x, y, z so that you do not have
			to change your vector library (i.e. greater portability)

			NOTE2: angle is in RADIANS

***********************************************************************EDOC*/
void MUrQuat_GetValue(M3tQuaternion *quat, float *x, float *y,
											float *z, float *radians)
{
    float len;
    float tx, ty, tz;

	// cache variables
	tx = quat->x;
	ty = quat->y;
	tz = quat->z;

	len = tx * tx + ty * ty + tz * tz;

    if (len > MUcQuatDelta)
	{
		*x = tx * (1.f / len);
		*y = ty * (1.f / len);
		*z = tz * (1.f / len);
	    *radians = (float)(2.0 * MUrACos(quat->w));
    }
    else {
		*x = 0.0;
		*y = 0.0;
		*z = 1.0;
	    *radians = 0.0;
    }

	return;
}


/*SDOC***********************************************************************

  Name:		MUrQuat_SetValue

  Action:   Assembles quaternion from an axis and an angle

  Params:   M3tQuaternion* (quaternion), float (x, y, z - axis), float (angle)

  Returns:  nothing

  Comments: NOTE: vector has been split into x, y, z so that you do not have
			to change your vector library (i.e. greater portability)

			NOTE2: angle has to be in RADIANS

***********************************************************************EDOC*/
void MUrQuat_SetValue(M3tQuaternion *quat, float x, float y, float z, float angle)
{
	float theSin = MUrSin(angle * 0.5f /* S.S. / 2.0f*/);
	float theCos = MUrCos(angle * 0.5f /* S.S. / 2.0f*/);
	float temp, dist;

	// normalize
	temp = x*x + y*y + z*z;

    dist = (float)(MUrOneOverSqrt(temp)) * theSin;

    x *= dist;
    y *= dist;
    z *= dist;

	quat->x = x;
	quat->y = y;
	quat->z = z;

	quat->w = theCos;

	return;
}



/*SDOC***********************************************************************

  Name:		MUrQuat_ScaleAngle

  Action:   Scales the rotation angle of a quaternion

  Params:   M3tQuaternion* (quaternion), float (scale value)

  Returns:  nothing

  Comments: none

***********************************************************************EDOC*/
void MUrQuat_ScaleAngle(M3tQuaternion * quat, float scale)
{
    float x, y, z;	// axis
    float angle;		// and angle

	MUrQuat_GetValue(quat, &x, &y, &z, &angle);

    MUrQuat_SetValue(quat, x, y, z, (angle * scale));

	return;
}



/*SDOC***********************************************************************

  Name:		MUrQuat_Inverse

  Action:   Inverts quaternion's rotation ( q^(-1) )

  Params:   M3tQuaternion* (quaternion)

  Returns:  nothing

  Comments: none
Returns the inverse of the quaternion (1/q).  check conjugate
***********************************************************************EDOC*/
void MUrQuat_Inverse(const M3tQuaternion *quat, M3tQuaternion *res)
{
	M3tMatrix4x3 tempMatrix;
	M3tMatrix4x3 inverseMatrix;

	UUmAssertReadPtr(quat, sizeof(*quat));
	UUmAssertWritePtr(res, sizeof(*res));

	MUmQuat_VerifyUnit(quat);

	MUrQuatToMatrix(quat, &tempMatrix);
	MUrMatrix_Inverse(&tempMatrix, &inverseMatrix);
	MUrMatrixToQuat(&inverseMatrix, res);

	MUmQuat_VerifyUnit(res);

	return;
}

void MUrQuat_SetFromTwoVectors(const M3tVector3D *inFrom, const M3tVector3D *inTo, M3tQuaternion *outQuat)
{
	M3tVector3D from = *inFrom;
	M3tVector3D to = *inTo;

	MUmVector_Normalize(from);
	MUmVector_Normalize(to);

	MUrQuat_SetFromAx(from.x, from.y, from.z, to.x, to.y, to.z, outQuat);

	return;
}

/*SDOC***********************************************************************

  Name:		MUrQuat_SetFromAx

  Action:   Constructs quaternion to rotate from one direction vector to
			another

  Params:   fromX, fromY, fromZ - fromVector
			toX, toY, toZ - toVector, M3tQuaternion* (resulting quaternion)

  Returns:  nothing

  Comments: Two vectors have to be UNIT vectors (so make sure you normalize
			them before calling this function
			Some parts are heavily optimized so readability is not so great :(
***********************************************************************EDOC*/
void MUrQuat_SetFromAx(float fromX,float fromY, float fromZ,
						 float toX,float toY, float toZ, M3tQuaternion *outQuat)

{
    float tx, ty, tz, temp, dist;
    float cost, len, ss;

	UUmAssert((UUmSQR(fromX) + UUmSQR(fromY) + UUmSQR(fromZ)) > 0.99);
	UUmAssert((UUmSQR(fromX) + UUmSQR(fromY) + UUmSQR(fromZ)) < 1.01);
	UUmAssert((UUmSQR(toX) + UUmSQR(toY) + UUmSQR(toZ)) > 0.99);
	UUmAssert((UUmSQR(toX) + UUmSQR(toY) + UUmSQR(toZ)) < 1.01);
	UUmAssertWritePtr(outQuat, sizeof(M3tQuaternion));

	// get dot product of two vectors
    cost = toX * fromX + toY * fromY + toZ * fromZ;

    // check if parallel
    if (cost > 0.99999f) {
		outQuat->x = outQuat->y = outQuat->z = 0.0f;
		outQuat->w = 1.0f;
    }
	else if (cost < -0.99999f) {		// check if opposite
			// check if we can use cross product of from vector with [1, 0, 0]
		tx = 0.0;
		ty = toX;
		tz = -toY;

		len = MUrSqrt(ty*ty + tz*tz);

		if (len < MUcQuatDelta)
		{
			// nope! we need cross product of from vector with [0, 1, 0]
			tx = -toZ;
			ty = 0.0;
			tz = toX;
		}

		// normalize
		temp = tx*tx + ty*ty + tz*tz;

		dist = MUrOneOverSqrt(temp);

		tx *= dist;
		ty *= dist;
		tz *= dist;

		outQuat->x = tx;
		outQuat->y = ty;
		outQuat->z = tz;
		outQuat->w = 0.0;
	}
	else {
		// ... else we can just cross two vectors

		tx = toY * fromZ - toZ * fromY;
		ty = toZ * fromX - toX * fromZ;
		tz = toX * fromY - toY * fromX;

		temp = tx*tx + ty*ty + tz*tz;

		dist = MUrOneOverSqrt(temp);

		tx *= dist;
		ty *= dist;
		tz *= dist;


		// we have to use half-angle formulae (sin^2 t = ( 1 - cos (2t) ) /2)

		ss = MUrSqrt(0.5f * (1.0f - cost));

		tx *= ss;
		ty *= ss;
		tz *= ss;

		// scale the axis to get the normalized quaternion
		outQuat->x = tx;
		outQuat->y = ty;
		outQuat->z = tz;

		// cos^2 t = ( 1 + cos (2t) ) / 2
		// w part is cosine of half the rotation angle
		outQuat->w = MUrSqrt(0.5f * (1.0f + cost));
	}

	MUmQuat_VerifyUnit(outQuat);

	return;
}


/*SDOC***********************************************************************

  Name:		MUrQuat_Mul

  Action:   Multiplies two quaternions

  Params:   M3tQuaternion ( q1 * q2 = res)

  Returns:  nothing

  Comments: NOTE: multiplication is not commutative

***********************************************************************EDOC*/
#ifndef USE_FAST_MATH
void MUrQuat_Mul(const M3tQuaternion* q1, const M3tQuaternion* q2, M3tQuaternion* res)
{
#if 0
	float A,B,C,D,E,F,G,H;

	A = (q1->w + q1->x) * (q2->w + q2->x);  // wW + xW + wX + xX
	B = (q1->z - q1->y) * (q2->y - q2->z);  // zY - yY - zZ + yZ
	C = (q1->x - q1->w) * (q2->y - q2->z);	// xY - wY - xZ + wZ
	D = (q1->y + q1->z) * (q2->x - q2->w);
	E = (q1->x + q1->z) * (q2->x + q2->y);
	F = (q1->x - q1->z) * (q2->x - q2->y);
	G = (q1->w + q1->y) * (q2->w - q2->z);
	H = (q1->w - q1->y) * (q2->w + q2->z);

	res->w =  B + (- E - F + G + H) * 0.5f;
	res->x =  A - (  E + F + G + H) * 0.5f;
	res->y = -C + (  E - F + G - H) * 0.5f;
	res->z = -D + (  E - F - G + H) * 0.5f;

	MUrQuat_Normalize(res);
#else
	float x,y,z,w;

	MUmQuat_VerifyUnit(q1);
	MUmQuat_VerifyUnit(q2);

	x = q1->w * q2->x + q1->x * q2->w + q1->y * q2->z - q1->z * q2->y;
	y = q1->w * q2->y + q1->y * q2->w + q1->z * q2->x - q1->x * q2->z;
	z = q1->w * q2->z + q1->z * q2->w + q1->x * q2->y - q1->y * q2->x;
	w = q1->w * q2->w - q1->x * q2->x - q1->y * q2->y - q1->z * q2->z;

	res->x = x;
	res->y = y;
	res->z = z;
	res->w = w;
#endif

	MUmQuat_VerifyUnit(res);
}
#endif

/*SDOC***********************************************************************

  Name:		MUrQuat_Add

  Action:   Adds two quaternions

  Params:   M3tQuaternion* (q1 + q2 = res)

  Returns:  nothing

  Comments: none

***********************************************************************EDOC*/
#ifndef USE_FAST_MATH
void MUrQuat_Add(const M3tQuaternion* q1, const M3tQuaternion* q2, M3tQuaternion* res)
{
	res->x = q1->x + q2->x;
	res->y = q1->y + q2->y;
	res->z = q1->z + q2->z;
	res->w = q1->w + q2->w;

	// make sure the resulting quaternion is a unit quat.
	MUrQuat_Normalize(res);
}
#endif

/*SDOC***********************************************************************

  Name:		MUrQuat_Sub

  Action:   Subtracts two quaternions

  Params:   M3tQuaternion* (q1 - q2 = res)

  Returns:  nothing

  Comments: none

***********************************************************************EDOC*/
#ifndef USE_FAST_MATH
void MUrQuat_Sub(const M3tQuaternion* q1, const M3tQuaternion* q2, M3tQuaternion* res)
{
	res->x = q1->x - q2->x;
	res->y = q1->y - q2->y;
	res->z = q1->z - q2->z;
	res->w = q1->w - q2->w;

	// make sure the resulting quaternion is a unit quat.
	MUrQuat_Normalize(res);
}
#endif

/*SDOC***********************************************************************

  Name:		MUrQuat_Div

  Action:   Divide two quaternions

  Params:   M3tQuaternion* (q1 / q2 = res)

  Returns:  nothing

  Comments: none

***********************************************************************EDOC*/
void MUrQuat_Div(const M3tQuaternion* q1, const M3tQuaternion* q2, M3tQuaternion* res)
#if 1
{
	M3tQuaternion q2_inverse;

	MUmQuat_VerifyUnit(q1);
	MUmQuat_VerifyUnit(q2);

	MUrQuat_Inverse(q2, &q2_inverse);
	MUrQuat_Mul(&q2_inverse, q1, res);

	MUmQuat_VerifyUnit(res);
}
#elif
{
	M3tQuaternion q, r, s;

	MUrQuat_Copy(q2, &q);

	// invert vector
    q.x = -q.x;
    q.y = -q.y;
    q.z = -q.z;

	MUrQuat_Mul(q1, &q, &r);
	MUrQuat_Mul(&q, &q, &s);

	res->x = r.x / s.w;
	res->y = r.y / s.w;
	res->z = r.z / s.w;
	res->w = r.w / s.w;

	MUrQuat_Normalize(res);
	MUmQuat_VerifyUnit(res);
}
#endif

/*SDOC***********************************************************************

  Name:		MUrQuat_Copy

  Action:   copies q1 into q2

  Params:   M3tQuaternion* (q1 and q2)

  Returns:  nothing

  Comments: none

***********************************************************************EDOC*/
void MUrQuat_Copy(const M3tQuaternion* q1, M3tQuaternion* q2)
{
	q2->x = q1->x;
	q2->y = q1->y;
	q2->z = q1->z;
	q2->w = q1->w;
}



/*SDOC***********************************************************************

  Name:		MUrQuat_Square

  Action:   Square quaternion

  Params:   M3tQuaternion* (q1 * q1 = res)

  Returns:  nothing

  Comments: none

***********************************************************************EDOC*/
void MUrQuat_Square(const M3tQuaternion* q1, M3tQuaternion* res)
{
	float  tt;

	UUmAssertReadPtr(q1, sizeof(*q1));
	UUmAssertWritePtr(res, sizeof(*res));
	MUmQuat_VerifyUnit(q1);

	tt = 2 * q1->w;
	res->x = tt * q1->x;
	res->y = tt * q1->y;
	res->z = tt * q1->z;
	res->w = (q1->w * q1->w - q1->x * q1->x - q1->y * q1->y - q1->z * q1->z);

	MUmQuat_VerifyUnit(res);

	return;
}


/*SDOC***********************************************************************

  Name:		MUrQuat_Sqrt

  Action:   Find square root of a quaternion

  Params:   M3tQuaternion* (MUrSqrt(q1) = res)

  Returns:  nothing

  Comments: none

***********************************************************************EDOC*/
void MUrQuat_Sqrt(const M3tQuaternion* q1, M3tQuaternion* res)
{
	float  length, m, r1, r2;
	M3tQuaternion r;

	length = MUrSqrt (q1->w * q1->w + q1->x * q1->x + q1->y * q1->y);
	if (length != 0.0)
		length = 1.0f / length;
	else length = 1.0f;

	r.x = q1->x * length;
	r.y = q1->z * length;
	r.z = 0.0f;
	r.w = q1->w * length;

	m = MUrOneOverSqrt(r.w * r.w + r.x * r.x);
	r1 = MUrSqrt ((1.0f + r.y) * 0.5f);
	r2 = MUrSqrt ((1.0f - r.y) * 0.5f);

	res->x = MUrSqrt (length) * r2 * r.x * m;
	res->y = MUrSqrt (length) * r1;
	res->z = q1->z;
	res->w = MUrSqrt (length) * r1 * r.w * m;

}


/*SDOC***********************************************************************

  Name:		MUrQuat_Dot

  Action:   Computes the dot product of two unit quaternions

  Params:   M3tQuaternion (first and second quaternion)

  Returns:  (float) Dot product

  Comments: Quaternion has to be normalized (i.e. it's a unit quaternion)

***********************************************************************EDOC*/
float MUrQuat_Dot(const M3tQuaternion* q1, const M3tQuaternion* q2)
{
  return (float)(q1->w * q2->w + q1->x * q2->x + q1->y * q2->y+q1->z*q2->z);
}


/*SDOC***********************************************************************

  Name:		MUrQuat_Length

  Action:   Calculates the length of a quaternion

  Params:   M3tQuaternion* (quaternion)

  Returns:  float (length)

  Comments: none

***********************************************************************EDOC*/
float MUrQuat_Length(M3tQuaternion* q1)
{
  return MUrSqrt (q1->w * q1->w + q1->x * q1->x + q1->y * q1->y + q1->z * q1->z);
}


/*SDOC***********************************************************************

  Name:		MUrQuat_Negate

  Action:   Negates vector part of a quaternion

  Params:   M3tQuaternion (source and destination quaternion)

  Returns:  nothing

  Comments: Source quaternion does NOT have to be normalized

***********************************************************************EDOC*/
void MUrQuat_Negate(M3tQuaternion* q1, M3tQuaternion* q2)
{
	MUrQuat_Copy(q1, q2);

	MUrQuat_Normalize(q2);
	q2->x = -q2->x;
	q2->y = -q2->y;
	q2->z = -q2->z;
}

/*SDOC***********************************************************************

  Name:		MUrQuat_Exp

  Action:   Calculates exponent of a quaternion

  Params:   M3tQuaternion* (Source and destination quaternion)

  Returns:  nothing

  Comments: none

***********************************************************************EDOC*/
void MUrQuat_Exp(M3tQuaternion* q1, M3tQuaternion* q2)
{
	float  len1, len2;

	len1 = (float) MUrSqrt (q1->x * q1->x + q1->y * q1->y + q1->z * q1->z);
	if (len1 > 0.0)
		len2 = (float)sin(len1) / len1;
	else
		len2 = 1.0;

	q2->x = q1->x * len2;
	q2->y = q1->y * len2;
	q2->z = q1->z * len2;
	q2->w = MUrCos (len1);
}


/*SDOC***********************************************************************

  Name:		MUrQuat_Log

  Action:   Calculates natural logarithm of a quaternion

  Params:   M3tQuaternion* (Source and destination quaternion)

  Returns:  nothing

  Comments: none

***********************************************************************EDOC*/
void MUrQuat_Log(M3tQuaternion* q1, M3tQuaternion* q2)
{
	float  length;

	length = MUrSqrt (q1->x * q1->x + q1->y * q1->y + q1->z * q1->z);

	//make sure we do not divide by 0
	if (q1->w != 0.0)
		length = MUrATan (length / q1->w);
	else length = M3cHalfPi;

	q2->w = 0.0f;
	q2->x = q1->x * length;
	q2->y = q1->y * length;
	q2->z = q1->z * length;
}

void
MUrQuatToMatrix(
	const M3tQuaternion	*inQuaternion,
	M3tMatrix4x3		*outMatrix)
{
	float s,xs,ys,zs,wx,wy,wz,xx,xy,xz,yy,yz,zz;

	s = 2.f / (UUmSQR(inQuaternion->x) + UUmSQR(inQuaternion->y) + UUmSQR(inQuaternion->z) + UUmSQR(inQuaternion->w));

	xs = inQuaternion->x * s;
	ys = inQuaternion->y * s;
	zs = inQuaternion->z * s;

	wx = inQuaternion->w * xs;
	wy = inQuaternion->w * ys;
	wz = inQuaternion->w * zs;

	xx = inQuaternion->x * xs;
	xy = inQuaternion->x * ys;
	xz = inQuaternion->x * zs;

	yy = inQuaternion->y * ys;
	yz = inQuaternion->y * zs;
	zz = inQuaternion->z * zs;

	outMatrix->m[0][0] = 1.f - (yy + zz);
	outMatrix->m[0][1] = xy - wz;
	outMatrix->m[0][2] = xz + wy;

	outMatrix->m[1][0] = xy + wz;
	outMatrix->m[1][1] = 1.f - (xx + zz);
	outMatrix->m[1][2] = yz - wx;

	outMatrix->m[2][0] = xz - wy;
	outMatrix->m[2][1] = yz + wx;
	outMatrix->m[2][2] = 1.f - (xx + yy);

	outMatrix->m[3][0] = 0;
	outMatrix->m[3][1] = 0;
	outMatrix->m[3][2] = 0;

	return;
}

void MUrMatrix_MultiplyPoint(
	const M3tPoint3D	 *inPoint,
	const M3tMatrix4x3	 *inMatrix,
	M3tPoint3D			 *outPoint)
{
	float iX = inPoint->x;
	float iY = inPoint->y;
	float iZ = inPoint->z;

	outPoint->x = inMatrix->m[0][0] * iX + inMatrix->m[1][0] * iY + inMatrix->m[2][0] * iZ + inMatrix->m[3][0];
	outPoint->y = inMatrix->m[0][1] * iX + inMatrix->m[1][1] * iY + inMatrix->m[2][1] * iZ + inMatrix->m[3][1];
	outPoint->z = inMatrix->m[0][2] * iX + inMatrix->m[1][2] * iY + inMatrix->m[2][2] * iZ + inMatrix->m[3][2];

	return;
}

void MUrMatrix3x3_MultiplyPoint(
	const M3tPoint3D	 *inPoint,
	const M3tMatrix3x3	 *inMatrix,
	M3tPoint3D			 *outPoint)
{
	float iX = inPoint->x;
	float iY = inPoint->y;
	float iZ = inPoint->z;

	outPoint->x = inMatrix->m[0][0] * iX + inMatrix->m[1][0] * iY + inMatrix->m[2][0] * iZ;
	outPoint->y = inMatrix->m[0][1] * iX + inMatrix->m[1][1] * iY + inMatrix->m[2][1] * iZ;
	outPoint->z = inMatrix->m[0][2] * iX + inMatrix->m[1][2] * iY + inMatrix->m[2][2] * iZ;

	return;
}

void MUrMatrix3x3_TransposeMultiplyPoint(
	const M3tPoint3D	 *inPoint,
	const M3tMatrix3x3	 *inMatrix,
	M3tPoint3D			 *outPoint)
{
	float iX = inPoint->x;
	float iY = inPoint->y;
	float iZ = inPoint->z;

	outPoint->x = inMatrix->m[0][0] * iX + inMatrix->m[0][1] * iY + inMatrix->m[0][2] * iZ;
	outPoint->y = inMatrix->m[1][0] * iX + inMatrix->m[1][1] * iY + inMatrix->m[1][2] * iZ;
	outPoint->z = inMatrix->m[2][0] * iX + inMatrix->m[2][1] * iY + inMatrix->m[2][2] * iZ;

	return;
}

void MUrMatrix_MultiplyNormal(
	const M3tVector3D	 *inNormal,
	const M3tMatrix4x3	 *inMatrix,
	M3tVector3D			 *outNormal)
{
	float iX;
	float iY;
	float iZ;

	UUmAssertReadPtr(inNormal, sizeof(*inNormal));
	UUmAssertReadPtr(inMatrix, sizeof(*inMatrix));
	UUmAssertWritePtr(outNormal, sizeof(*outNormal));

	iX = inNormal->x;
	iY = inNormal->y;
	iZ = inNormal->z;

	outNormal->x = inMatrix->m[0][0] * iX + inMatrix->m[1][0] * iY + inMatrix->m[2][0] * iZ;
	outNormal->y = inMatrix->m[0][1] * iX + inMatrix->m[1][1] * iY + inMatrix->m[2][1] * iZ;
	outNormal->z = inMatrix->m[0][2] * iX + inMatrix->m[1][2] * iY + inMatrix->m[2][2] * iZ;

	MUrNormalize(outNormal);

	return;
}

void MUrMatrix3x3_MultiplyNormal(
	const M3tVector3D	 *inNormal,
	const M3tMatrix3x3	 *inMatrix,
	M3tVector3D			 *outNormal)
{
	float iX;
	float iY;
	float iZ;

	UUmAssertReadPtr(inNormal, sizeof(*inNormal));
	UUmAssertReadPtr(inMatrix, sizeof(*inMatrix));
	UUmAssertWritePtr(outNormal, sizeof(*outNormal));

	iX = inNormal->x;
	iY = inNormal->y;
	iZ = inNormal->z;

	outNormal->x = inMatrix->m[0][0] * iX + inMatrix->m[1][0] * iY + inMatrix->m[2][0] * iZ;
	outNormal->y = inMatrix->m[0][1] * iX + inMatrix->m[1][1] * iY + inMatrix->m[2][1] * iZ;
	outNormal->z = inMatrix->m[0][2] * iX + inMatrix->m[1][2] * iY + inMatrix->m[2][2] * iZ;

	MUrNormalize(outNormal);

	return;
}

void MUrMatrix_MultiplyNormals(
	UUtUns32			inNumNormals,
	const M3tMatrix4x3	*inMatrix,
	const M3tVector3D	*inNormals,
	M3tVector3D			*outNormals)
{
	UUtUns32 itr;

	for(itr = 0; itr < inNumNormals; itr++) {
		MUrMatrix_MultiplyNormal(inNormals + itr, inMatrix, outNormals + itr);
	}

	return;
}

void MUrMatrix3x3_MultiplyNormals(
	UUtUns32			inNumNormals,
	const M3tMatrix3x3	*inMatrix,
	const M3tVector3D	*inNormals,
	M3tVector3D			*outNormals)
{
	UUtUns32 itr;

	for(itr = 0; itr < inNumNormals; itr++) {
		MUrMatrix3x3_MultiplyNormal(inNormals + itr, inMatrix, outNormals + itr);
	}

	return;
}

void MUrMatrix_MultiplyPoints(
	UUtUns32			inNumPoints,
	const M3tMatrix4x3	*inMatrix,
	const M3tPoint3D	*inPoints,
	M3tPoint3D			*outPoints)
{
	UUtUns32 itr;
	const M3tPoint3D	*curInPoint = inPoints;
	M3tPoint3D			*curOutPoint = outPoints;

	for(itr = 0; itr < inNumPoints; itr++)
	{
		MUrMatrix_MultiplyPoint(curInPoint, inMatrix, curOutPoint);

		curInPoint++;
		curOutPoint++;
	}
}

#ifndef USE_FAST_MATH
void
MUrVector_CrossProduct(
	const M3tVector3D*		inVectorA,
	const M3tVector3D*		inVectorB,
	M3tVector3D*			inResult)
{
	UUmAssert(NULL != inVectorA);
	UUmAssert(NULL != inVectorB);
	UUmAssert(NULL != inResult);

	inResult->x = inVectorA->y * inVectorB->z - inVectorA->z * inVectorB->y;
	inResult->y = inVectorA->z * inVectorB->x - inVectorA->x * inVectorB->z;
	inResult->z = inVectorA->x * inVectorB->y - inVectorA->y * inVectorB->x;
}

float
MUrVector_DotProduct(
	const M3tVector3D*		inVectorA,
	const M3tVector3D*		inVectorB)
{
	float result;

	UUmAssert(NULL != inVectorA);
	UUmAssert(NULL != inVectorB);

	result = inVectorA->x * inVectorB->x + inVectorA->y * inVectorB->y + inVectorA->z * inVectorB->z;

	return result;
}
#endif

void
MUrVector_NormalFromPoints(
	const M3tPoint3D*	inPoint0,
	const M3tPoint3D*	inPoint1,
	const M3tPoint3D*	inPoint2,
	M3tVector3D			*outNormal)
{
	// Returns normal of (0,0,0) if points were collinear
	M3tVector3D	vector01, vector02;
	float		length;

	UUmAssert(NULL != inPoint0);
	UUmAssert(NULL != inPoint1);
	UUmAssert(NULL != inPoint2);
	UUmAssert(NULL != outNormal);

	vector01.x = inPoint1->x - inPoint0->x;
	vector01.y = inPoint1->y - inPoint0->y;
	vector01.z = inPoint1->z - inPoint0->z;

	vector02.x = inPoint2->x - inPoint0->x;
	vector02.y = inPoint2->y - inPoint0->y;
	vector02.z = inPoint2->z - inPoint0->z;

	MUrVector_CrossProduct(
		&vector01,
		&vector02,
		outNormal);

	length = outNormal->x * outNormal->x + outNormal->y * outNormal->y + outNormal->z * outNormal->z;

	if (length==0.0f) {
		outNormal->x = 1.f;
		outNormal->y = 0.f;
		outNormal->z = 0.f;

		return;
	}

	length = MUrOneOverSqrt(length);

	outNormal->x *= length;	// Negatives here account for counterclockwise point ordering
	outNormal->y *= length;
	outNormal->z *= length;

	UUmAssert(MUmVector_IsNormalized(*outNormal));

	return;
}

void MUrVector_PlaneFromEdges(
	M3tPoint3D *inEdge1a,
	M3tPoint3D *inEdge1b,
	M3tPoint3D *inEdge2a,
	M3tPoint3D *inEdge2b,
	M3tPlaneEquation *outPlane)
{
	// Computes a plane equation from two edges. The d value
	// of the plane is computed with the SECOND edge. This can be
	// significant of the two edges are not intersecting.
	M3tVector3D vecA,vecB,normal;

	// Determine the normal of the plane of the two lines of the collision
	MUmVector_Subtract(vecA,*inEdge1a,*inEdge1b);
	MUmVector_Subtract(vecB,*inEdge2b,*inEdge2a);
	MUrVector_CrossProduct(&vecA,&vecB,&normal);
	MUrNormalize(&normal);

	// Now extract the plane equation at the point of impact
	outPlane->a = normal.x;
	outPlane->b = normal.y;
	outPlane->c = normal.z;
	outPlane->d = -(normal.x*inEdge2a->x + normal.y*inEdge2a->y + normal.z*inEdge2a->z);
}


void MUrVector_SetLength(M3tVector3D *ioVector, float inNewMagnitude)
{
	/**************
	* Adjusts 'ioVector' so that its magnitude is 'm',
	* but leaves its direction unchanged
	*/

	float oldLength, scaleAmt;

	oldLength = MUrVector_Length(ioVector);
	scaleAmt = inNewMagnitude / oldLength;
	MUmVector_Scale(*ioVector, scaleAmt);

	return;
}

void MUrVector_PushBack(
	M3tVector3D *outPushBack,
	const M3tVector3D *inDelta,
	const M3tVector3D *inNormal)
{
#if 0
	// This works just as well as slow 4 stage process below, but I didn't want
	// to mess with it in case I didn't know what I was doing...
	M3tVector3D projection,projector;

	projector = *inDelta;
	MUmVector_Negate(projector);
	MUrVector_ProjectVector(&projector,(M3tVector3D *)inNormal,&projection);
	MUmVector_Negate(projection);
	*outPushBack = projection;

	if (UUmFloat_CompareEquSloppy(inDelta->x,outPushBack->x)) outPushBack->x = inDelta->x;
	if (UUmFloat_CompareEquSloppy(inDelta->y,outPushBack->y)) outPushBack->y = inDelta->y;
	if (UUmFloat_CompareEquSloppy(inDelta->z,outPushBack->z)) outPushBack->z = inDelta->z;

/*	if ((inDelta->x < 0 && -outPushBack->x < inDelta->x) ||
		(inDelta->x > 0 && -outPushBack->x > inDelta->x)) outPushBack->x = -inDelta->x;
	if ((inDelta->y < 0 && -outPushBack->y < inDelta->y) ||
		(inDelta->y > 0 && -outPushBack->y > inDelta->y)) outPushBack->y = -inDelta->y;
	if ((inDelta->z < 0 && -outPushBack->z < inDelta->z) ||
		(inDelta->z > 0 && -outPushBack->z > inDelta->z)) outPushBack->z = -inDelta->z;
*/
/*	if (outPushBack->x < 0.0f) outPushBack->x -= 0.01f;
	if (outPushBack->y < 0.0f) outPushBack->y -= 0.01f;
	if (outPushBack->z < 0.0f) outPushBack->z -= 0.01f;*/

/*	if (outPushBack->x < 0.0f && outPushBack->x > -0.1f) outPushBack->x = -0.1f;
	if (outPushBack->y < 0.0f && outPushBack->y > -0.1f) outPushBack->y = -0.1f;
	if (outPushBack->z < 0.0f && outPushBack->z > -0.1f) outPushBack->z = -0.1f;*/

#else

#if 0
	// CB: I am replacing this code with a far more efficient section (below) because
	// this function is actually showing up on my profiles. it shouldn't. - 19 october 2000
	const M3tVector3D positiveXVector = { 1, 0, 0 };
	M3tMatrix4x3 alignmentMatrix;
	M3tMatrix4x3 inverseAlignmentMatrix;
	M3tPoint3D inDeltaAlignmentSpace;
	M3tPoint3D pushBackAlignmentSpace;

	// processs:
	// 1. build an alignment matrix to put inNormal to (1,0,0)
	// 2. multiply in delta by that alignment matrix
	// 3. create a pushback delta in that alignment space (easy just subtract off the x)
	// 4. take that pushback and multiply it by the alignment matrix tht goes back

	MUrMatrix_Alignment(inNormal, &positiveXVector, &alignmentMatrix);
	MUrMatrix_Alignment(&positiveXVector, inNormal, &inverseAlignmentMatrix);

	MUrMatrix_MultiplyPoints(1, &alignmentMatrix, (const M3tPoint3D *) inDelta, &inDeltaAlignmentSpace);

	if (inDeltaAlignmentSpace.x < 0) {
		pushBackAlignmentSpace.x = inDeltaAlignmentSpace.x;
		pushBackAlignmentSpace.y = 0;
		pushBackAlignmentSpace.z = 0;

		//if (pushBackAlignmentSpace.x > -0.01f) pushBackAlignmentSpace.x = -0.01f;
		MUrMatrix_MultiplyPoints(1, &inverseAlignmentMatrix, &pushBackAlignmentSpace, outPushBack);
	}
	else {
		outPushBack->x = 0;
		outPushBack->y = 0;
		outPushBack->z = 0;
	}

	//if (UUmFloat_CompareEquSloppy(inDelta->x,outPushBack->x)) outPushBack->x = inDelta->x;
	//if (UUmFloat_CompareEquSloppy(inDelta->y,outPushBack->y)) outPushBack->y = inDelta->y;
	//if (UUmFloat_CompareEquSloppy(inDelta->z,outPushBack->z)) outPushBack->z = inDelta->z;
#else
	float pushback_dot;

	UUmAssert(MUmVector_IsNormalized(*inNormal));
	pushback_dot = MUrVector_DotProduct(inDelta, inNormal);

	if (pushback_dot < 0) {
		MUmVector_ScaleCopy(*outPushBack, pushback_dot, *inNormal);
	} else {
		*outPushBack = MUgZeroVector;
	}
#endif
#endif
	return;
}


void MUrVector_ProjectVector(M3tVector3D *inA, M3tVector3D *inB, M3tVector3D *outV)
{
	/******************
	* Projects A on to B
	*/

	float magsquared,dotovermagsq;

	magsquared = MUrVector_Length(inB);
	magsquared *= magsquared;
	UUmAssert(magsquared);

	*outV = *inB;
	dotovermagsq = MUrVector_DotProduct(inA,inB)/magsquared;
	MUmVector_Scale(*outV,dotovermagsq);
}

float
MUrTriangle_Area(
	M3tPoint3D*	inPoint0,
	M3tPoint3D*	inPoint1,
	M3tPoint3D*	inPoint2)
{

	float		length01, temp, height;
	M3tPoint3D	closestPointOn01;

	length01 = inPoint1->x - inPoint0->x;
	length01 *= length01;

	temp = inPoint1->y - inPoint0->y;
	length01 += temp * temp;

	temp = inPoint1->z - inPoint0->z;
	length01 += temp * temp;

	length01 = MUrSqrt(length01);

	MUrLineSegment_ClosestPointToPoint(
		inPoint0,
		inPoint1,
		inPoint2,
		&closestPointOn01);

	height = closestPointOn01.x - inPoint2->x;
	height *= height;

	temp = closestPointOn01.y - inPoint2->y;
	height += temp * temp;

	temp = closestPointOn01.z - inPoint2->z;
	height += temp * temp;

	height = MUrSqrt(height);

	return length01 * height * 0.5f;
}

float MUrAngleBetweenVectors3D(const M3tVector3D *inVector1, const M3tVector3D *inVector2)
{
	/* Returns angle between A and B in radians */
	float dotProduct;
	float result;

	UUmAssert(MUmVector_IsNormalized(*inVector1));
	UUmAssert(MUmVector_IsNormalized(*inVector2));

	dotProduct = MUrVector_DotProduct(inVector1, inVector2);

	// never should be to far off
	UUmAssert((dotProduct > -1.1f) && (dotProduct < 1.1f));

	// valid range of acos is strictly 0..1
	dotProduct = UUmPin(dotProduct, -1.f, 1.f);
	result = MUrACos(dotProduct);

	UUmAssertTrigRange(result);

	return result;
}

float MUrAngleBetweenVectors3D_0To2Pi(const M3tVector3D *inVector1, const M3tVector3D *inVector2)
{
	/* Returns angle between A and B in radians */
	float dotProduct;
	float result;
	M3tVector3D	alignedV2;
	M3tVector3D	upX;
	M3tMatrix4x3	alignUpXMatrix;

	UUmAssert(MUmVector_IsNormalized(*inVector1));
	UUmAssert(MUmVector_IsNormalized(*inVector2));

	upX.x = 1.0f;
	upX.y = 0.0f;
	upX.z = 0.0f;

	MUrMatrix_Alignment(
		inVector1,
		&upX,
		&alignUpXMatrix);

	MUrMatrix_MultiplyPoint(
		(const M3tPoint3D*)inVector2,
		&alignUpXMatrix,
		(M3tPoint3D*)&alignedV2);

	dotProduct = MUrVector_DotProduct(&upX, &alignedV2);

	// never should be to far off
	UUmAssert((dotProduct > -1.1f) && (dotProduct < 1.1f));

	// valid range of acos is strictly 0..1
	dotProduct = UUmPin(dotProduct, -1.f, 1.f);
	result = MUrACos(dotProduct);

	UUmAssertTrigRange(result);

	if(alignedV2.y < 0.0f) result = M3cPi * 2.0f - result;

	return result;
}


float MUrACos(float inFloat)
{
	float result;

	UUmAssert((inFloat >= -1.f) && (inFloat <= 1.f));

	result = (float) acos(inFloat);

#if UUmCompiler	== UUmCompiler_MWerks
	result = UUmPin(result, 0, M3cPi);
	UUmAssert((result >= 0) && (result <= M3cPi));
#else
	UUmAssert((result >= 0) && (result <= M3cPi));
#endif

	return result;
}

float MUrASin(float inFloat)
{
	float result;

	UUmAssert((inFloat >= -1.f) && (inFloat <= 1.f));

	result = (float) asin(inFloat);

	return result;
}

void MUrMinMaxBBox_CreateFromSphere(
		const M3tMatrix4x3 *inMatrix,
		const M3tBoundingSphere *inSphere,
		M3tBoundingBox_MinMax *outBBox)
{
	M3tPoint3D center;

	UUmAssertWritePtr(outBBox, sizeof(*outBBox));

	if (NULL == inMatrix)
	{
		center = inSphere->center;
	}
	else
	{
		UUmAssertReadPtr(inMatrix, sizeof(*inMatrix));
		MUrMatrix_MultiplyPoints(1, inMatrix, &inSphere->center, &center);
	}

	outBBox->minPoint.x = center.x - inSphere->radius;
	outBBox->maxPoint.x = center.x + inSphere->radius;

	outBBox->minPoint.y = center.y - inSphere->radius;
	outBBox->maxPoint.y = center.y + inSphere->radius;

	outBBox->minPoint.z = center.z - inSphere->radius;
	outBBox->maxPoint.z = center.z + inSphere->radius;

	return;
}


void MUrMinMaxBBox_ExpandBySphere(
		const M3tMatrix4x3 *inMatrix,
		const M3tBoundingSphere *inSphere,
		M3tBoundingBox_MinMax *outBBox)
{
	M3tPoint3D center;

	UUmAssertReadPtr(inSphere, sizeof(*inSphere));
	UUmAssertWritePtr(outBBox, sizeof(*outBBox));

	if (NULL == inMatrix) {
		center = inSphere->center;
	}
	else {
		UUmAssertReadPtr(inMatrix, sizeof(*inMatrix));
		MUrMatrix_MultiplyPoints(1, inMatrix, &inSphere->center, &center);
	}

	outBBox->minPoint.x = UUmMin(center.x - inSphere->radius, outBBox->minPoint.x);
	outBBox->maxPoint.x = UUmMax(center.x + inSphere->radius, outBBox->maxPoint.x);

	outBBox->minPoint.y = UUmMin(center.y - inSphere->radius, outBBox->minPoint.y);
	outBBox->maxPoint.y = UUmMax(center.y + inSphere->radius, outBBox->maxPoint.y);

	outBBox->minPoint.z = UUmMin(center.z - inSphere->radius, outBBox->minPoint.z);
	outBBox->maxPoint.z = UUmMax(center.z + inSphere->radius, outBBox->maxPoint.z);

	return;
}

void MUrMinMaxBBox_To_Sphere(
	const M3tBoundingBox_MinMax *inBBox,
	M3tBoundingSphere *outSphere)
{
#define cLargerThanRoot2 1.414214f // 1.414213562373

	UUmAssertReadPtr(inBBox, sizeof(*inBBox));
	UUmAssertWritePtr(outSphere, sizeof(*outSphere));

	outSphere->center.x = inBBox->minPoint.x + (inBBox->maxPoint.x - inBBox->minPoint.x) / 2;
	outSphere->center.y = inBBox->minPoint.y + (inBBox->maxPoint.y - inBBox->minPoint.y) / 2;
	outSphere->center.z = inBBox->minPoint.z + (inBBox->maxPoint.z - inBBox->minPoint.z) / 2;

	outSphere->radius = (cLargerThanRoot2 * 0.5f /* S.S. / 2.f*/) *
							UUmMax3(inBBox->maxPoint.x - inBBox->minPoint.x,
									inBBox->maxPoint.y - inBBox->minPoint.y,
									inBBox->maxPoint.z - inBBox->minPoint.z);
	return;
}

void MUrCylinder_CreateFromSphere(
		const M3tMatrix4x3 *inMatrix,
		const M3tBoundingSphere *inSphere,
		M3tBoundingCylinder *outCylinder)
{
	M3tPoint3D center;

	UUmAssertReadPtr(inMatrix, sizeof(*inMatrix));
	UUmAssertReadPtr(inSphere, sizeof(*inSphere));
	UUmAssertWritePtr(outCylinder, sizeof(*outCylinder));

	MUrMatrix_MultiplyPoints(1, inMatrix, &inSphere->center, &center);

	outCylinder->circle.center.x = center.x;
	outCylinder->circle.center.z = center.z;
	outCylinder->circle.radius = inSphere->radius;

	outCylinder->top = center.y + inSphere->radius;
	outCylinder->bottom = center.y - inSphere->radius;

	return;
}

void MUrPoint2D_Add(M3tPoint2D *result, const M3tPoint2D *inPointA, const M3tPoint2D *inPointB)
{
	result->x = inPointA->x + inPointB->x;
	result->z = inPointA->z + inPointB->z;

	return;
}

void MUrPoint2D_Subtract(M3tPoint2D *result, const M3tPoint2D *inPointA, const M3tPoint2D *inPointB)
{
	result->x = inPointA->x - inPointB->x;
	result->z = inPointA->z - inPointB->z;

	return;
}

static float MUrPoint2D_Distance_Squared(const M3tPoint2D *inPointA, const M3tPoint2D *inPointB)
{
	float distance_squared;

	distance_squared = UUmSQR(inPointA->x - inPointB->x) + UUmSQR(inPointA->z - inPointB->z);

	return distance_squared;
}

static float MUrPoint2D_Distance(const M3tPoint2D *inPointA, const M3tPoint2D *inPointB)
{
	float distance_squared;
	float distance;

	distance_squared = UUmSQR(inPointA->x - inPointB->x) + UUmSQR(inPointA->z - inPointB->z);
	distance = MUrSqrt(distance_squared);

	return distance;
}

static float MUrLineSegment2D_ExpandByPoint(float cur_distance_squared, M3tPoint2D *l1, M3tPoint2D *l2, M3tPoint2D *newPoint)
{
	float l1_new_distance_squared = MUrPoint2D_Distance_Squared(l1, newPoint);
	float l2_new_distance_squared = MUrPoint2D_Distance_Squared(l2, newPoint);

	if ((cur_distance_squared >= l1_new_distance_squared) && (cur_distance_squared >= l2_new_distance_squared)) {
		return cur_distance_squared;
	}
	else if (l1_new_distance_squared >= l2_new_distance_squared) {
		*l2 = *newPoint;

		return l1_new_distance_squared;
	}
	else {
		*l1 = *newPoint;

		return l2_new_distance_squared;
	}
}

static void MUrCircle_ExpandByCircle(M3tBoundingCircle *ioCircleA, const M3tBoundingCircle *inCircleB)
{
	float length;
	M3tPoint2D unit_vector;
	M3tPoint2D vector_ra;
	M3tPoint2D vector_rb;
	M3tPoint2D a_minus_vra;
	M3tPoint2D b_minus_vrb;
	M3tPoint2D a_plus_vra;
	M3tPoint2D b_plus_vrb;

	M3tPoint2D l1, l2;
	float l_distance_squared;

	M3tPoint2D new_center;

	// step 1 build unit_vector
	MUrPoint2D_Subtract(&unit_vector, &ioCircleA->center, &inCircleB->center);
	length = UUmSQR(unit_vector.x) + UUmSQR(unit_vector.z);

	if (0 == length) {
		ioCircleA->radius = UUmMax(ioCircleA->radius, inCircleB->radius);
		return;
	}

	length = MUrSqrt(length);

	unit_vector.x /= length;
	unit_vector.z /= length;

	// build vector_ra & vector_rb
	vector_ra.x = unit_vector.x * ioCircleA->radius;
	vector_ra.z = unit_vector.z * ioCircleA->radius;

	vector_rb.x = unit_vector.x * inCircleB->radius;
	vector_rb.z = unit_vector.z * inCircleB->radius;

	// build the four line segment points
	MUrPoint2D_Subtract(&a_minus_vra, &ioCircleA->center, &vector_ra);
	MUrPoint2D_Subtract(&b_minus_vrb, &inCircleB->center, &vector_rb);
	MUrPoint2D_Add(&a_plus_vra, &ioCircleA->center, &vector_ra);
	MUrPoint2D_Add(&b_plus_vrb, &inCircleB->center, &vector_rb);

	// ok now pick or line segment points
	l1 = a_minus_vra;
	l2 = a_plus_vra;
	l_distance_squared = MUrPoint2D_Distance_Squared(&l1, &l2);

	l_distance_squared = MUrLineSegment2D_ExpandByPoint(l_distance_squared, &l1, &l2, &b_minus_vrb);
	l_distance_squared = MUrLineSegment2D_ExpandByPoint(l_distance_squared, &l1, &l2, &b_plus_vrb);

	// center is in the middle of those two
	new_center.x = (l1.x + l2.x) * 0.5f; // S.S. / 2.f;
	new_center.z = (l1.z + l2.z) * 0.5f; // S.S. / 2.f;

	ioCircleA->center = new_center;
	ioCircleA->radius = MUrSqrt(l_distance_squared) * 0.5f; // S.S. / 2.f;

	return;
}

void MUrCylinder_ExpandBySphere(
		const M3tMatrix4x3 *inMatrix,
		const M3tBoundingSphere *inSphere,
		M3tBoundingCylinder *ioCylinder)
{
	M3tPoint3D center;
	M3tBoundingCircle sphere_circle;

	UUmAssertReadPtr(inMatrix, sizeof(*inMatrix));
	UUmAssertReadPtr(inSphere, sizeof(*inSphere));
	UUmAssertWritePtr(ioCylinder, sizeof(*ioCylinder));

	MUrMatrix_MultiplyPoints(1, inMatrix, &inSphere->center, &center);

	// step 1 grow top and bottom
	ioCylinder->top = UUmMax(center.y + inSphere->radius, ioCylinder->top);
	ioCylinder->bottom = UUmMin(center.y - inSphere->radius, ioCylinder->bottom);

	// step 2 grow radius & move center

	// draw line segment from center of one 2-d circle to the other
	// grow segment to hit the edges of each circle.
	// new radius is in the middle of that segment
	// with radius = length of segment / 2

	sphere_circle.center.x = center.x;
	sphere_circle.center.z = center.z;
	sphere_circle.radius = inSphere->radius;

	MUrCircle_ExpandByCircle(&ioCylinder->circle, &sphere_circle);

	return;
}

UUtUns8 MUrSolveQuadratic(float a, float b, float c, float *outSolutions)
{
	float	discriminant = b * b - 4 * a * c;
	UUtUns8	num_solutions;

	UUmAssertReadPtr(outSolutions, sizeof(float) * 2);

	if (discriminant < 0) {
		num_solutions = 0;
	}
	else {
		float one_over_two_a = 0.5f / a;

		if (UUmFloat_CompareEqu(discriminant, 0)) {
			outSolutions[0] = -b * one_over_two_a;

			num_solutions = 1;
		}
		else {
			discriminant = MUrSqrt(discriminant);

			outSolutions[0] = (-b + discriminant) * one_over_two_a;
			outSolutions[1] = (-b - discriminant) * one_over_two_a;

			num_solutions = 2;
		}
	}

	return num_solutions;
}

UUtBool MUrEuler_IsEqual(
	const MUtEuler *inEulerA,
	const MUtEuler *inEulerB)
{
	UUtBool equal;

	if (inEulerA->order != inEulerB->order) {
		return UUcFalse;
	}

	equal = UUmFloat_CompareEqu(inEulerA->x, inEulerB->x);
	if (!equal) {
		return UUcFalse;
	}

	equal = UUmFloat_CompareEqu(inEulerA->y, inEulerB->y);
	if (!equal) {
		return UUcFalse;
	}

	equal = UUmFloat_CompareEqu(inEulerA->z, inEulerB->z);
	if (!equal) {
		return UUcFalse;
	}

	return UUcTrue;
}


static UUtBool MUrPlaneEquation_IsClose_Internal(
	const M3tPlaneEquation *inPlaneEquationA,
	const M3tPlaneEquation *inPlaneEquationB)
{
	UUtBool is_close;

	is_close = UUmFloat_CompareEquSloppy(inPlaneEquationA->a, inPlaneEquationB->a);
	is_close &= UUmFloat_CompareEquSloppy(inPlaneEquationA->b, inPlaneEquationB->b);
	is_close &= UUmFloat_CompareEquSloppy(inPlaneEquationA->c, inPlaneEquationB->c);
	is_close &= UUmFloat_CompareEquSloppy(inPlaneEquationA->d, inPlaneEquationB->d);

	return is_close;
}

UUtBool MUrPlaneEquation_IsClose(
	const M3tPlaneEquation *inPlaneEquationA,
	const M3tPlaneEquation *inPlaneEquationB)
{
	M3tPlaneEquation negPlaneEquationA;
	UUtBool is_close;

	is_close = MUrPlaneEquation_IsClose_Internal(inPlaneEquationA, inPlaneEquationB);

	if (!is_close) {
		negPlaneEquationA.a = -inPlaneEquationA->a;
		negPlaneEquationA.b = -inPlaneEquationA->b;
		negPlaneEquationA.c = -inPlaneEquationA->c;
		negPlaneEquationA.d = -inPlaneEquationA->d;

		is_close = MUrPlaneEquation_IsClose_Internal(&negPlaneEquationA, inPlaneEquationB);
	}

	return is_close;
}

static UUtBool MUrQuat_IsEqual_Internal(
	const M3tQuaternion *inQuatA,
	const M3tQuaternion *inQuatB)
{
	UUtBool equal;

	equal = UUmFloat_CompareEqu(inQuatA->x, inQuatB->x);
	if (!equal) {
		return UUcFalse;
	}

	equal = UUmFloat_CompareEqu(inQuatA->y, inQuatB->y);
	if (!equal) {
		return UUcFalse;
	}

	equal = UUmFloat_CompareEqu(inQuatA->z, inQuatB->z);
	if (!equal) {
		return UUcFalse;
	}

	equal = UUmFloat_CompareEqu(inQuatA->w, inQuatB->w);
	if (!equal) {
		return UUcFalse;
	}

	return UUcTrue;
}

UUtBool MUrQuat_IsEqual(
	const M3tQuaternion *inQuatA,
	const M3tQuaternion *inQuatB)
{
	UUtBool equal;

	equal = MUrQuat_IsEqual_Internal(inQuatA, inQuatB);
	if (!equal) {
		M3tQuaternion inverseA;

		inverseA.x = -inQuatA->x;
		inverseA.y = -inQuatA->y;
		inverseA.z = -inQuatA->z;
		inverseA.w = -inQuatA->w;

		equal = MUrQuat_IsEqual_Internal(&inverseA, inQuatB);
	}

	return equal;
}

void MUrPoint3D_Lerp(const M3tPoint3D *inFrom, const M3tPoint3D *inTo, float t, M3tPoint3D *outResult)
{
	float scale0 = 1.f - t;
	float scale1 = t;

	UUmAssert((t >= 0.f) && (t <= 1.f));

	outResult->x = inFrom->x * scale0 + inTo->x * scale1;
	outResult->y = inFrom->y * scale0 + inTo->y * scale1;
	outResult->z = inFrom->z * scale0 + inTo->z * scale1;

	return;
}

#if UUmCompiler	== UUmCompiler_VisC
UUtInt32 MUrFloat_Round_To_Int(float inFloatNumber)
{
	UUtInt32 i;

	_asm {
	  fld	inFloatNumber;
	  fistp	i;
	}

	return i;
}
#else
UUtInt32 MUrFloat_Round_To_Int(float inFloatNumber)
{
	UUtInt32 integerNumber;
//	#define MUrFloat_Round_To_Int(x) (((x) < 0.0) ? (float)((UUtInt32)(x - 0.5)) : (float)((UUtInt32)(x + 0.5)))

	if (inFloatNumber < 0)
	{
		integerNumber = inFloatNumber - 0.5f;
	}
	else
	{
		integerNumber = inFloatNumber + 0.5f;
	}

	return integerNumber;
}
#endif

UUtBool MUrPoint_IsEqual(
	const M3tPoint3D *inPointA,
	const M3tPoint3D *inPointB)
{
	UUtBool equal;

	equal = UUmFloat_CompareEqu(inPointA->x, inPointB->x);
	if (!equal) {
		return UUcFalse;
	}

	equal = UUmFloat_CompareEqu(inPointA->y, inPointB->y);
	if (!equal) {
		return UUcFalse;
	}

	equal = UUmFloat_CompareEqu(inPointA->z, inPointB->z);
	if (!equal) {
		return UUcFalse;
	}

	return UUcTrue;
}

UUtBool MUrVector_IsEqual(
	const M3tVector3D *inVectorA,
	const M3tVector3D *inVectorB)
{
	UUtBool equal;

	equal = UUmFloat_CompareEqu(inVectorA->x, inVectorB->x);
	if (!equal) {
		return UUcFalse;
	}

	equal = UUmFloat_CompareEqu(inVectorA->y, inVectorB->y);
	if (!equal) {
		return UUcFalse;
	}

	equal = UUmFloat_CompareEqu(inVectorA->z, inVectorB->z);
	if (!equal) {
		return UUcFalse;
	}

	return UUcTrue;
}

void
MUrPointVector_FlipAboutPlane(
	M3tPoint3D*			inPoint,
	M3tVector3D*		inVector,
	M3tPlaneEquation*	inPlane,
	M3tPoint3D			*outReflectedPoint,
	M3tVector3D			*outReflectedVector)
{
	float	plane_a, plane_b, plane_c, plane_d;
	float	vec_x, vec_y, vec_z;
	float	dist_point_to_plane;
	float	refl_point_x, refl_point_y, refl_point_z;
	float	point_x, point_z, point_y;
	float	denom;
	float	inter_point_x, inter_point_y, inter_point_z;
	float	refl_vec_x, refl_vec_y, refl_vec_z;
	float	temp;
	float	t;

	plane_a = inPlane->a;
	plane_b = inPlane->b;
	plane_c = inPlane->c;
	plane_d = inPlane->d;

	vec_x = inVector->x;
	vec_y = inVector->y;
	vec_z = inVector->z;

	point_x = inPoint->x;
	point_y = inPoint->y;
	point_z = inPoint->z;

	// compute reflection point
		dist_point_to_plane = plane_a * point_x + plane_b * point_y + plane_c * point_z + plane_d;

		refl_point_x = point_x - dist_point_to_plane * plane_a * 2.0f;
		refl_point_y = point_y - dist_point_to_plane * plane_b * 2.0f;
		refl_point_z = point_z - dist_point_to_plane * plane_c * 2.0f;

	// compute intersection point
		denom = plane_a * vec_x + plane_b * vec_y + plane_c * vec_z;

		if(denom == 0.0f)
		{
			*outReflectedPoint = *inPoint;
			*outReflectedVector = *inVector;
			return;
		}

		t = -(plane_a * point_x + plane_b * point_y + plane_c * point_z + plane_d) / denom;

		inter_point_x = vec_x * t + point_x;
		inter_point_y = vec_y * t + point_y;
		inter_point_z = vec_z * t + point_z;

	#if 0
	// compute reflection vector which is V - N(2(N.V))
		temp = 2.0f * denom;
		refl_vec_x = vec_x - plane_a * temp;
		refl_vec_y = vec_y - plane_b * temp;
		refl_vec_z = vec_z - plane_c * temp;

	#else
		refl_vec_x = inter_point_x - refl_point_x;
		refl_vec_y = inter_point_y - refl_point_y;
		refl_vec_z = inter_point_z - refl_point_z;

		temp = MUrOneOverSqrt(refl_vec_x * refl_vec_x + refl_vec_y * refl_vec_y + refl_vec_z * refl_vec_z);

		refl_vec_x *= temp;
		refl_vec_y *= temp;
		refl_vec_z *= temp;
	#endif

	outReflectedPoint->x = refl_point_x;
	outReflectedPoint->y = refl_point_y;
	outReflectedPoint->z = refl_point_z;

	outReflectedVector->x = refl_vec_x;
	outReflectedVector->y = refl_vec_y;
	outReflectedVector->z = refl_vec_z;

}

/* these used to be inlined */

// returns the bits of a float packed into a unsigned 32 int
// 2.f != 2, 2.f = some nasty hex number
UUtUns32 MUrFloat_To_Uns_Bits(float f)
{
	MUtInternal_Cast number;

	number.float_4byte = f;

	return number.unsigned_4byte;
}

float MUrUnsBits_To_Float(UUtUns32 bits)
{
	MUtInternal_Cast number;

	number.unsigned_4byte = bits;

	return number.float_4byte;
}

/* make sure your floats never equal or exceed 1<<23 (8,388,608.0) or are negative */
/* this function rounds instead of truncating */
UUtInt32 MUrUnsignedSmallFloat_To_Int_Round(float f)
{
	MUtInternal_Cast number;

	UUmAssert((f >= 0) && (f < UUcFloat_NormalizedBit));

	f += UUcFloat_NormalizedBit;

	number.float_4byte = f;

	return number.signed_4byte & UUcFloat_FractionMask;
}

/* make sure your floats never equal or exceed 1<<23 (8,388,608.0) or are negative */
// this function rounds instead of truncating
UUtUns32 MUrUnsignedSmallFloat_To_Uns_Round(float f)
{
	MUtInternal_Cast number;

	UUmAssert((f >= 0) && (f < UUcFloat_NormalizedBit));

	f += UUcFloat_NormalizedBit;

	number.float_4byte = f;

	return number.unsigned_4byte & UUcFloat_FractionMask;
}

// f >= 0, f < (1 << (23 - shiftLeftValue))
// shiftLeftValue 0 .. 22
// returns ROUND(float * 2 ^ (shiftLeftValue))
UUtInt32 MUrUnsignedSmallFloat_ShiftLeft_To_Int_Round(float f, UUtInt32 shiftLeftValue)
{
	MUtInternal_Cast number;

	UUmAssert((shiftLeftValue >= 0) && (shiftLeftValue < 23));
	UUmAssert((f >= 0) && (f < (1 << (23 - shiftLeftValue))) );

	f += (1 << (23 - shiftLeftValue));

	number.float_4byte = f;

	return number.signed_4byte & UUcFloat_FractionMask;
}

// f >= 0, f < (1 << (23 - shiftLeftValue))
// shiftLeftValue 0 .. 22
// returns ROUND(float * 2 ^ (shiftLeftValue))
UUtUns32 MUrUnsignedSmallFloat_ShiftLeft_To_Uns_Round(float f, UUtInt32 shiftLeftValue)
{
	MUtInternal_Cast number;

	UUmAssert((shiftLeftValue >= 0) && (shiftLeftValue < 23));
	UUmAssert((f >= 0) && (f < (1 << (23 - shiftLeftValue))) );

	f += (1 << (23 - shiftLeftValue));

	number.float_4byte = f;

	return number.unsigned_4byte & UUcFloat_FractionMask;
}

UUtUns8
MUrFloat0ToLessThan1_ToUns8(
	float f)
{
	MUtInternal_Cast number;

	UUmAssert((f >= 0) && (f <= 1.0));

	f += (UUcFloat_NormalizedBit >> 8);

	number.float_4byte = f;

	return (UUtUns8) (number.unsigned_4byte & UUcFloat_FractionMask);
}

UUtBool MUrVector_Normalize_ZeroTest(
	M3tVector3D *inV)
{
	float d = MUmVector_GetLengthSquared(*inV);

	if (UUmFloat_TightCompareZero(d))
		return UUcTrue;

	d = MUrOneOverSqrt(d);
	MUmVector_Scale(*inV, d);
	return UUcFalse;
}

void MUrMatrix3x3_FormOrthonormalBasis(M3tVector3D *inX, M3tVector3D *inY, M3tVector3D *inZ,
														M3tMatrix3x3 *outMatrix)
{
#if DEBUGGING && (DEBUGGING)
	// check normalization, orthogonality and right-handedness of this basis
	{
		float temp_dot;
		M3tVector3D calc_z;

		UUmAssert(MUmVector_IsNormalized(*inX));
		UUmAssert(MUmVector_IsNormalized(*inY));
		UUmAssert(MUmVector_IsNormalized(*inZ));

		temp_dot = MUrVector_DotProduct(inX, inY);
		UUmAssert(UUmFloat_LooseCompareZero(temp_dot));

		temp_dot = MUrVector_DotProduct(inX, inZ);
		UUmAssert(UUmFloat_LooseCompareZero(temp_dot));

		temp_dot = MUrVector_DotProduct(inY, inZ);
		UUmAssert(UUmFloat_LooseCompareZero(temp_dot));

		MUrVector_CrossProduct(inX, inY, &calc_z);
		temp_dot = MUrVector_DotProduct(inZ, &calc_z);
		UUmAssert(UUmFloat_LooseCompareZero(temp_dot - 1.0f));
	}
#endif

	outMatrix->m[0][0] = inX->x;
	outMatrix->m[0][1] = inX->y;
	outMatrix->m[0][2] = inX->z;

	outMatrix->m[1][0] = inY->x;
	outMatrix->m[1][1] = inY->y;
	outMatrix->m[1][2] = inY->z;

	outMatrix->m[2][0] = inZ->x;
	outMatrix->m[2][1] = inZ->y;
	outMatrix->m[2][2] = inZ->z;

	return;
}
