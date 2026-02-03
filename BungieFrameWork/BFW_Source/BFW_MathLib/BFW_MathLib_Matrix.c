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
#include "EulerAngles.h"
#include "Decompose.h"

const M3tMatrix4x3 MUgIdentityMatrix =
	{	1, 0, 0,
		0, 1, 0,
		0, 0, 1,
		0, 0, 0 };

void MUrMatrixToQuat(const M3tMatrix4x3 *inMatrix, M3tQuaternion *outQuat)
{

  float		tr, s;
  float		q[4];
  UUtInt32	i, j, k;

  UUtInt32 nxt[3] = {1, 2, 0};

  tr = inMatrix->m[0][0] + inMatrix->m[1][1] + inMatrix->m[2][2];

  // check the diagonal

  if (tr > 0.0)
  {
    s = MUrSqrt (tr + 1.f);

    outQuat->w = s * 0.5f; // S.S. / 2.f;

	s = 0.5f / s;

    outQuat->x = (inMatrix->m[2][1] - inMatrix->m[1][2]) * s;
    outQuat->y = (inMatrix->m[0][2] - inMatrix->m[2][0]) * s;
    outQuat->z = (inMatrix->m[1][0] - inMatrix->m[0][1]) * s;

  }
  else {

	  // diagonal is negative

	  i = 0;

      if (inMatrix->m[1][1] > inMatrix->m[0][0]) i = 1;
	  if (inMatrix->m[2][2] > inMatrix->m[i][i]) i = 2;

	  j = nxt[i];
      k = nxt[j];

      s = MUrSqrt ((inMatrix->m[i][i] - (inMatrix->m[j][j] + inMatrix->m[k][k])) + 1.f);

	  q[i] = s * 0.5f;

      if (s != 0.0) s = 0.5f / s;

	  q[3] = (inMatrix->m[k][j] - inMatrix->m[j][k]) * s;
      q[j] = (inMatrix->m[j][i] + inMatrix->m[i][j]) * s;
      q[k] = (inMatrix->m[k][i] + inMatrix->m[i][k]) * s;

	  outQuat->x = q[0];
	  outQuat->y = q[1];
	  outQuat->z = q[2];
	  outQuat->w = q[3];
  }

  MUrQuat_Normalize(outQuat);
  MUmQuat_VerifyUnit(outQuat);
}

void MUrMatrix_GetRow(const M3tMatrix4x3 *inMatrix, UUtUns8 inRow, M3tPoint3D *outRow)
{
	UUmAssert(inRow >= 0);
	UUmAssert(inRow <= 3);

	outRow->x = inMatrix->m[0][inRow];
	outRow->y = inMatrix->m[1][inRow];
	outRow->z = inMatrix->m[2][inRow];
}

void MUrMatrix_GetCol(const M3tMatrix4x3 *inMatrix, UUtUns8 inCol, M3tPoint3D *outCol)
{
	UUmAssert(inCol >= 0);
	UUmAssert(inCol <= 3);

	outCol->x = inMatrix->m[inCol][0];
	outCol->y = inMatrix->m[inCol][1];
	outCol->z = inMatrix->m[inCol][2];
}

void MUrMatrix_Multiply(
			const M3tMatrix4x3	*inMatrixA,
			const M3tMatrix4x3	*inMatrixB,
			M3tMatrix4x3			*outResult)
{
	float	a00, a10, a20, a30;
	float	a01, a11, a21, a31;
	float	a02, a12, a22, a32;

	float	b0, b1, b2;
	float	d0, d1, d2;

	a00 = inMatrixA->m[0][0];
	a01 = inMatrixA->m[0][1];
	a02 = inMatrixA->m[0][2];

	a10 = inMatrixA->m[1][0];
	a11 = inMatrixA->m[1][1];
	a12 = inMatrixA->m[1][2];

	a20 = inMatrixA->m[2][0];
	a21 = inMatrixA->m[2][1];
	a22 = inMatrixA->m[2][2];

	a30 = inMatrixA->m[3][0];
	a31 = inMatrixA->m[3][1];
	a32 = inMatrixA->m[3][2];

	b0 = inMatrixB->m[0][0];
	b1 = inMatrixB->m[0][1];
	b2 = inMatrixB->m[0][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;

	outResult->m[0][0] = d0;
	outResult->m[0][1] = d1;
	outResult->m[0][2] = d2;

	b0 = inMatrixB->m[1][0];
	b1 = inMatrixB->m[1][1];
	b2 = inMatrixB->m[1][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;

	outResult->m[1][0] = d0;
	outResult->m[1][1] = d1;
	outResult->m[1][2] = d2;

	b0 = inMatrixB->m[2][0];
	b1 = inMatrixB->m[2][1];
	b2 = inMatrixB->m[2][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;

	outResult->m[2][0] = d0;
	outResult->m[2][1] = d1;
	outResult->m[2][2] = d2;

	b0 = inMatrixB->m[3][0];
	b1 = inMatrixB->m[3][1];
	b2 = inMatrixB->m[3][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2 + a30;
	d1 = a01 * b0 + a11 * b1 + a21 * b2 + a31;
	d2 = a02 * b0 + a12 * b1 + a22 * b2 + a32;

	outResult->m[3][0] = d0;
	outResult->m[3][1] = d1;
	outResult->m[3][2] = d2;
}

void MUrMatrix3x3_Multiply(
			const M3tMatrix3x3	*inMatrixA,
			const M3tMatrix3x3	*inMatrixB,
			M3tMatrix3x3		*outResult)
{
	float	a00, a10, a20;
	float	a01, a11, a21;
	float	a02, a12, a22;

	float	b0, b1, b2;
	float	d0, d1, d2;

	a00 = inMatrixA->m[0][0];
	a01 = inMatrixA->m[0][1];
	a02 = inMatrixA->m[0][2];

	a10 = inMatrixA->m[1][0];
	a11 = inMatrixA->m[1][1];
	a12 = inMatrixA->m[1][2];

	a20 = inMatrixA->m[2][0];
	a21 = inMatrixA->m[2][1];
	a22 = inMatrixA->m[2][2];

	b0 = inMatrixB->m[0][0];
	b1 = inMatrixB->m[0][1];
	b2 = inMatrixB->m[0][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;

	outResult->m[0][0] = d0;
	outResult->m[0][1] = d1;
	outResult->m[0][2] = d2;

	b0 = inMatrixB->m[1][0];
	b1 = inMatrixB->m[1][1];
	b2 = inMatrixB->m[1][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;

	outResult->m[1][0] = d0;
	outResult->m[1][1] = d1;
	outResult->m[1][2] = d2;

	b0 = inMatrixB->m[2][0];
	b1 = inMatrixB->m[2][1];
	b2 = inMatrixB->m[2][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;

	outResult->m[2][0] = d0;
	outResult->m[2][1] = d1;
	outResult->m[2][2] = d2;
}

void MUrMatrix3x3_Transpose(
			const M3tMatrix3x3	*inMatrix,
			M3tMatrix3x3		*outResult)
{
	outResult->m[0][0] = inMatrix->m[0][0];
	outResult->m[0][1] = inMatrix->m[1][0];
	outResult->m[0][2] = inMatrix->m[2][0];

	outResult->m[1][0] = inMatrix->m[0][1];
	outResult->m[1][1] = inMatrix->m[1][1];
	outResult->m[1][2] = inMatrix->m[2][1];

	outResult->m[2][0] = inMatrix->m[0][2];
	outResult->m[2][1] = inMatrix->m[1][2];
	outResult->m[2][2] = inMatrix->m[2][2];
}


#if 0
{
	M3tMatrix4x3 result;

	result.m[0][0] = inA->m[0][0] * inB->m[0][0] + inA->m[1][0] * inB->m[0][1] + inA->m[2][0] * inB->m[0][2];
	result.m[0][1] = inA->m[0][1] * inB->m[0][0] + inA->m[1][1] * inB->m[0][1] + inA->m[2][1] * inB->m[0][2];
	result.m[0][2] = inA->m[0][2] * inB->m[0][0] + inA->m[1][2] * inB->m[0][1] + inA->m[2][2] * inB->m[0][2];

	result.m[1][0] = inA->m[0][0] * inB->m[1][0] + inA->m[1][0] * inB->m[1][1] + inA->m[2][0] * inB->m[1][2];
	result.m[1][1] = inA->m[0][1] * inB->m[1][0] + inA->m[1][1] * inB->m[1][1] + inA->m[2][1] * inB->m[1][2];
	result.m[1][2] = inA->m[0][2] * inB->m[1][0] + inA->m[1][2] * inB->m[1][1] + inA->m[2][2] * inB->m[1][2];

	result.m[2][0] = inA->m[0][0] * inB->m[2][0] + inA->m[1][0] * inB->m[2][1] + inA->m[2][0] * inB->m[2][2];
	result.m[2][1] = inA->m[0][1] * inB->m[2][0] + inA->m[1][1] * inB->m[2][1] + inA->m[2][1] * inB->m[2][2];
	result.m[2][2] = inA->m[0][2] * inB->m[2][0] + inA->m[1][2] * inB->m[2][1] + inA->m[2][2] * inB->m[2][2];

	result.m[3][0] = inA->m[0][0] * inB->m[3][0] + inA->m[1][0] * inB->m[3][1] + inA->m[2][0] * inB->m[3][2] + inA->m[3][0];
	result.m[3][1] = inA->m[0][1] * inB->m[3][0] + inA->m[1][1] * inB->m[3][1] + inA->m[2][1] * inB->m[3][2] + inA->m[3][1];
	result.m[3][2] = inA->m[0][2] * inB->m[3][0] + inA->m[1][2] * inB->m[3][1] + inA->m[2][2] * inB->m[3][2] + inA->m[3][2];

	UUrMemory_MoveFast(&result, outResult, sizeof(*outResult));

	return;
}
#endif

void
MUrMatrix_BuildRotateX(
	float			inRadians,
	M3tMatrix4x3		*outMatrix)
{
	float					c;
	float					s;

	UUmAssert(NULL != outMatrix);

	c = MUrCos(inRadians);
	s = MUrSin(inRadians);

	outMatrix->m[0][0] = 1.f;
	outMatrix->m[0][1] = 0;
	outMatrix->m[0][2] = 0;

	outMatrix->m[1][0] = 0;
	outMatrix->m[1][1] = c;
	outMatrix->m[1][2] = s;

	outMatrix->m[2][0] = 0;
	outMatrix->m[2][1] = -s;
	outMatrix->m[2][2] = c;

	outMatrix->m[3][0] = 0.0f;
	outMatrix->m[3][1] = 0.0f;
	outMatrix->m[3][2] = 0.0f;
}

void MUrMatrix_FromMax(
	M3tMatrix4x3 *ioMatrix)
{
	// Converts a matrix from MAX's coordinate system to ours
	M3tMatrix4x3 m1;

	MUrMatrix_BuildRotateX(M3cHalfPi,&m1);
	MUrMatrix_Multiply(&m1,ioMatrix,ioMatrix);
}

void
MUrMatrix_BuildRotateY(
	float			inRadians,
	M3tMatrix4x3		*outMatrix)
{
	const float	inX = 0.0f;
	const float	inY = 1.0f;
	const float	inZ = 0.0f;

	float					c;
	float					s;

	UUmAssert(NULL != outMatrix);

	c = MUrCos(inRadians);
	s = MUrSin(inRadians);

	outMatrix->m[0][0] = c;
	outMatrix->m[0][1] = 0;
	outMatrix->m[0][2] = -s;

	outMatrix->m[1][0] = 0;
	outMatrix->m[1][1] = 1;
	outMatrix->m[1][2] = 0;

	outMatrix->m[2][0] = s;
	outMatrix->m[2][1] = 0;
	outMatrix->m[2][2] = c;

	outMatrix->m[3][0] = 0.0f;
	outMatrix->m[3][1] = 0.0f;
	outMatrix->m[3][2] = 0.0f;

}

void
MUrMatrix_BuildRotateZ(
	float			inRadians,
	M3tMatrix4x3		*outMatrix)
{
	const float	inX = 0.0f;
	const float	inY = 0.0f;
	const float	inZ = 1.0f;

	float					c;
	float					s;

	UUmAssert(NULL != outMatrix);

	c = MUrCos(inRadians);
	s = MUrSin(inRadians);

	outMatrix->m[0][0] = c;
	outMatrix->m[0][1] = s;
	outMatrix->m[0][2] = 0;

	outMatrix->m[1][0] = -s;
	outMatrix->m[1][1] = c;
	outMatrix->m[1][2] = 0;

	outMatrix->m[2][0] = 0;
	outMatrix->m[2][1] = 0;
	outMatrix->m[2][2] = 1;

	outMatrix->m[3][0] = 0.0f;
	outMatrix->m[3][1] = 0.0f;
	outMatrix->m[3][2] = 0.0f;
}

void
MUrMatrix3x3_BuildRotateX(
	float			inRadians,
	M3tMatrix3x3	*outMatrix)
{
	float					c;
	float					s;

	UUmAssert(NULL != outMatrix);

	c = MUrCos(inRadians);
	s = MUrSin(inRadians);

	outMatrix->m[0][0] = 1.f;
	outMatrix->m[0][1] = 0;
	outMatrix->m[0][2] = 0;

	outMatrix->m[1][0] = 0;
	outMatrix->m[1][1] = c;
	outMatrix->m[1][2] = s;

	outMatrix->m[2][0] = 0;
	outMatrix->m[2][1] = -s;
	outMatrix->m[2][2] = c;
}

void
MUrMatrix3x3_BuildRotateY(
	float			inRadians,
	M3tMatrix3x3	*outMatrix)
{
	const float	inX = 0.0f;
	const float	inY = 1.0f;
	const float	inZ = 0.0f;

	float					c;
	float					s;

	UUmAssert(NULL != outMatrix);

	c = MUrCos(inRadians);
	s = MUrSin(inRadians);

	outMatrix->m[0][0] = c;
	outMatrix->m[0][1] = 0;
	outMatrix->m[0][2] = -s;

	outMatrix->m[1][0] = 0;
	outMatrix->m[1][1] = 1;
	outMatrix->m[1][2] = 0;

	outMatrix->m[2][0] = s;
	outMatrix->m[2][1] = 0;
	outMatrix->m[2][2] = c;

}

void
MUrMatrix3x3_BuildRotateZ(
	float			inRadians,
	M3tMatrix3x3	*outMatrix)
{
	const float	inX = 0.0f;
	const float	inY = 0.0f;
	const float	inZ = 1.0f;

	float					c;
	float					s;

	UUmAssert(NULL != outMatrix);

	c = MUrCos(inRadians);
	s = MUrSin(inRadians);

	outMatrix->m[0][0] = c;
	outMatrix->m[0][1] = s;
	outMatrix->m[0][2] = 0;

	outMatrix->m[1][0] = -s;
	outMatrix->m[1][1] = c;
	outMatrix->m[1][2] = 0;

	outMatrix->m[2][0] = 0;
	outMatrix->m[2][1] = 0;
	outMatrix->m[2][2] = 1;
}

void
MUrMatrix3x3_BuildRotate(
	float			inRadians,
	float			inX,
	float			inY,
	float			inZ,
	M3tMatrix3x3	*outMatrix)
{
	float					c;
	float					s;

	UUmAssert(NULL != outMatrix);

	c = MUrCos(inRadians);
	s = MUrSin(inRadians);

	outMatrix->m[0][0] = inX * inX * (1 - c) + c;
	outMatrix->m[0][1] = inY * inX * (1 - c) + inZ * s;
	outMatrix->m[0][2] = inX * inZ * (1 - c) - inY * s;

	outMatrix->m[1][0] = inX * inY * (1 - c) - inZ * s;
	outMatrix->m[1][1] = inY * inY * (1 - c) + c;
	outMatrix->m[1][2] = inY * inZ * (1 - c) + inX * s;

	outMatrix->m[2][0] = inX * inZ * (1 - c) + inY * s;
	outMatrix->m[2][1] = inY * inZ * (1 - c) - inX * s;
	outMatrix->m[2][2] = inZ * inZ * (1 - c) + c;
}

void
MUrMatrix_BuildRotate(
	float			inRadians,
	float			inX,
	float			inY,
	float			inZ,
	M3tMatrix4x3	*outMatrix)
{
	float					c;
	float					s;

	UUmAssert(NULL != outMatrix);

	c = MUrCos(inRadians);
	s = MUrSin(inRadians);

	outMatrix->m[0][0] = inX * inX * (1 - c) + c;
	outMatrix->m[0][1] = inY * inX * (1 - c) + inZ * s;
	outMatrix->m[0][2] = inX * inZ * (1 - c) - inY * s;

	outMatrix->m[1][0] = inX * inY * (1 - c) - inZ * s;
	outMatrix->m[1][1] = inY * inY * (1 - c) + c;
	outMatrix->m[1][2] = inY * inZ * (1 - c) + inX * s;

	outMatrix->m[2][0] = inX * inZ * (1 - c) + inY * s;
	outMatrix->m[2][1] = inY * inZ * (1 - c) - inX * s;
	outMatrix->m[2][2] = inZ * inZ * (1 - c) + c;

	outMatrix->m[3][0] = 0.0f;
	outMatrix->m[3][1] = 0.0f;
	outMatrix->m[3][2] = 0.0f;
}

void
MUrMatrix_BuildTranslate(
	float			inX,
	float			inY,
	float			inZ,
	M3tMatrix4x3		*outMatrix)
{
	UUmAssert(NULL != outMatrix);

	outMatrix->m[0][0] = 1.0f;
	outMatrix->m[0][1] = 0.0f;
	outMatrix->m[0][2] = 0.0f;

	outMatrix->m[1][0] = 0.0f;
	outMatrix->m[1][1] = 1.0f;
	outMatrix->m[1][2] = 0.0f;

	outMatrix->m[2][0] = 0.0f;
	outMatrix->m[2][1] = 0.0f;
	outMatrix->m[2][2] = 1.0f;

	outMatrix->m[3][0] = inX;
	outMatrix->m[3][1] = inY;
	outMatrix->m[3][2] = inZ;
}

void
MUrMatrix_BuildUniformScale(
	float			inScale,
	M3tMatrix4x3		*outMatrix)
{
	UUmAssert(NULL != outMatrix);

	outMatrix->m[0][0] = inScale;
	outMatrix->m[0][1] = 0.0f;
	outMatrix->m[0][2] = 0.0f;

	outMatrix->m[1][0] = 0.0f;
	outMatrix->m[1][1] = inScale;
	outMatrix->m[1][2] = 0.0f;

	outMatrix->m[2][0] = 0.0f;
	outMatrix->m[2][1] = 0.0f;
	outMatrix->m[2][2] = inScale;

	outMatrix->m[3][0] = 0.0f;
	outMatrix->m[3][1] = 0.0f;
	outMatrix->m[3][2] = 0.0f;

	return;
}

void
MUrMatrix_BuildScale(
	M3tPoint3D		*inScale,
	M3tMatrix4x3		*outMatrix)
{
	UUmAssert(NULL != outMatrix);

	outMatrix->m[0][0] = inScale->x;
	outMatrix->m[0][1] = 0.0f;
	outMatrix->m[0][2] = 0.0f;

	outMatrix->m[1][0] = 0.0f;
	outMatrix->m[1][1] = inScale->y;
	outMatrix->m[1][2] = 0.0f;

	outMatrix->m[2][0] = 0.0f;
	outMatrix->m[2][1] = 0.0f;
	outMatrix->m[2][2] = inScale->z;

	outMatrix->m[3][0] = 0.0f;
	outMatrix->m[3][1] = 0.0f;
	outMatrix->m[3][2] = 0.0f;

	return;
}


void MUrMatrix_Alignment(
	const M3tVector3D *from,
	const M3tVector3D *to,
	M3tMatrix4x3 *outMatrix)
{
	M3tQuaternion quat;

	UUmAssertReadPtr(from, sizeof(M3tVector3D));
	UUmAssertReadPtr(to, sizeof(M3tVector3D));
	UUmAssertWritePtr(outMatrix, sizeof(M3tMatrix4x3));

	MUrQuat_SetFromAx(from->x, from->y, from->z, to->x, to->y, to->z, &quat);
	MUrQuatToMatrix(&quat, outMatrix);
}

void MUrMatrix_Inverse(const M3tMatrix4x3 *inM, M3tMatrix4x3 *outM)
{
    int i, j;
    float det;

	UUmAssertReadPtr(inM, sizeof(*inM));
	UUmAssertWritePtr(outM, sizeof(*outM));

    /* calculate the adjoint matrix */
	MUrMatrix_Adjoint(inM, outM);

    /*  calculate the 4x4 determinant
     *  if the determinant is zero,
     *  then the inverse matrix is not unique.
     */

    det = MUrMatrix_Determinant( inM );

	UUmAssert( fabs( det ) >= UUcEpsilon);	// ("Non-singular matrix, no inverse!"UUmNL)

    /* scale the adjoint matrix to get the inverse */

    for (i=0; i<4; i++)
	{
        for(j=0; j<3; j++)
		{
			outM->m[i][j] = outM->m[i][j] / det;
		}
	}
}


/*
 *   MUrMatrix_Adjoint( original_matrix, inverse_matrix )
 *
 *     calculate the MUrMatrix_Adjoint of a 4x4 matrix
 *
 *      Let  a   denote the minor determinant of matrix A obtained by
 *           ij
 *
 *      deleting the ith row and jth column from A.
 *
 *                    i+j
 *     Let  b   = (-1)    a
 *          ij            ji
 *
 *    The matrix B = (b  ) is the MUrMatrix_Adjoint of A
 *                     ij
 */

void MUrMatrix_Adjoint(const M3tMatrix4x3 *inM, M3tMatrix4x3 *outM)
{
    float a1, a2, a3, a4, b1, b2, b3, b4;
    float c1, c2, c3, c4, d1, d2, d3, d4;

    /* assign to individual variable names to aid  */
    /* selecting correct values  */

	a1 = inM->m[0][0];
	b1 = inM->m[0][1];
	c1 = inM->m[0][2];
//	d1 = inM->m[0][3];
	d1 = 0.f;

	a2 = inM->m[1][0];
	b2 = inM->m[1][1];
	c2 = inM->m[1][2];
//	d2 = inM->m[1][3];
	d2 = 0.f;

	a3 = inM->m[2][0];
	b3 = inM->m[2][1];
	c3 = inM->m[2][2];
//	d3 = inM->m[2][3];
	d3 = 0.f;

	a4 = inM->m[3][0];
	b4 = inM->m[3][1];
	c4 = inM->m[3][2];
//	d4 = inM->m[3][3];
	d4 = 1.f;


    /* row column labeling reversed since we transpose rows & columns */

    outM->m[0][0]  =   MUr3x3_Determinant( b2, b3, b4, c2, c3, c4, d2, d3, d4);
    outM->m[1][0]  = - MUr3x3_Determinant( a2, a3, a4, c2, c3, c4, d2, d3, d4);
    outM->m[2][0]  =   MUr3x3_Determinant( a2, a3, a4, b2, b3, b4, d2, d3, d4);
    outM->m[3][0]  = - MUr3x3_Determinant( a2, a3, a4, b2, b3, b4, c2, c3, c4);

    outM->m[0][1]  = - MUr3x3_Determinant( b1, b3, b4, c1, c3, c4, d1, d3, d4);
    outM->m[1][1]  =   MUr3x3_Determinant( a1, a3, a4, c1, c3, c4, d1, d3, d4);
    outM->m[2][1]  = - MUr3x3_Determinant( a1, a3, a4, b1, b3, b4, d1, d3, d4);
    outM->m[3][1]  =   MUr3x3_Determinant( a1, a3, a4, b1, b3, b4, c1, c3, c4);

    outM->m[0][2]  =   MUr3x3_Determinant( b1, b2, b4, c1, c2, c4, d1, d2, d4);
    outM->m[1][2]  = - MUr3x3_Determinant( a1, a2, a4, c1, c2, c4, d1, d2, d4);
    outM->m[2][2]  =   MUr3x3_Determinant( a1, a2, a4, b1, b2, b4, d1, d2, d4);
    outM->m[3][2]  = - MUr3x3_Determinant( a1, a2, a4, b1, b2, b4, c1, c2, c4);

//    outM->m[0][3]  = - MUr3x3_Determinant( b1, b2, b3, c1, c2, c3, d1, d2, d3);
//    outM->m[1][3]  =   MUr3x3_Determinant( a1, a2, a3, c1, c2, c3, d1, d2, d3);
//    outM->m[2][3]  = - MUr3x3_Determinant( a1, a2, a3, b1, b2, b3, d1, d2, d3);
//    outM->m[3][3]  =   MUr3x3_Determinant( a1, a2, a3, b1, b2, b3, c1, c2, c3);
}

/*
 * float = MUrMatrix_Determinant( matrix )
 *
 * calculate the determinant of a 4x4 matrix.
 */
float MUrMatrix_Determinant(const M3tMatrix4x3 *inMatrix)
{
    float ans;
    float a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

    /* assign to individual variable names to aid selecting */
	/*  correct elements */

	a1 = inMatrix->m[0][0];
	b1 = inMatrix->m[0][1];
	c1 = inMatrix->m[0][2];
	// d1 = inMatrix->m[0][3];
	d1 = 0.f;

	a2 = inMatrix->m[1][0];
	b2 = inMatrix->m[1][1];
	c2 = inMatrix->m[1][2];
	//d2 = inMatrix->m[1][3];
	d2 = 0.f;

	a3 = inMatrix->m[2][0];
	b3 = inMatrix->m[2][1];
	c3 = inMatrix->m[2][2];
	//d3 = inMatrix->m[2][3];
	d3 = 0.f;

	a4 = inMatrix->m[3][0];
	b4 = inMatrix->m[3][1];
	c4 = inMatrix->m[3][2];
	//d4 = inMatrix->m[3][3];
	d4 = 1.f;

    ans = a1 * MUr3x3_Determinant( b2, b3, b4, c2, c3, c4, d2, d3, d4)
        - b1 * MUr3x3_Determinant( a2, a3, a4, c2, c3, c4, d2, d3, d4)
        + c1 * MUr3x3_Determinant( a2, a3, a4, b2, b3, b4, d2, d3, d4)
        - d1 * MUr3x3_Determinant( a2, a3, a4, b2, b3, b4, c2, c3, c4);

    return ans;
}



/*
 * float = MUr3x3_Determinant(  a1, a2, a3, b1, b2, b3, c1, c2, c3 )
 *
 * calculate the determinant of a 3x3 matrix
 * in the form
 *
 *     | a1,  b1,  c1 |
 *     | a2,  b2,  c2 |
 *     | a3,  b3,  c3 |
 */

float MUr3x3_Determinant(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3 )
{
    float ans;

    ans = a1 * MUr2x2_Determinant( b2, b3, c2, c3 )
        - b1 * MUr2x2_Determinant( a2, a3, c2, c3 )
        + c1 * MUr2x2_Determinant( a2, a3, b2, b3 );
    return ans;
}

/*
 * float = MUr2x2_Determinant( float a, float b, float c, float d )
 *
 * calculate the determinant of a 2x2 matrix.
 */

float MUr2x2_Determinant(float a, float b, float c, float d)
{
    float ans;
    ans = a * d - b * c;
    return ans;
}


void MUrMatrixStack_RotateX(M3tMatrix4x3 *topOfStack, float angle)
{
	M3tMatrix4x3 rotate;

	MUrMatrix_BuildRotateX(angle, &rotate);

	MUrMatrix_Multiply(topOfStack, &rotate, topOfStack);
}

void MUrMatrixStack_RotateY(M3tMatrix4x3 *topOfStack, float angle)
{
	M3tMatrix4x3 rotate;

	MUrMatrix_BuildRotateY(angle, &rotate);

	MUrMatrix_Multiply(topOfStack, &rotate, topOfStack);
}

void MUrMatrixStack_RotateZ(M3tMatrix4x3 *topOfStack, float angle)
{
	M3tMatrix4x3 rotate;

	MUrMatrix_BuildRotateZ(angle, &rotate);

	MUrMatrix_Multiply(topOfStack, &rotate, topOfStack);
}

void MUrMatrixStack_Matrix(
	M3tMatrix4x3 *topOfStack,
	const M3tMatrix4x3 *inMatrix)
{
	MUrMatrixVerify(inMatrix);
	MUrMatrixVerify(topOfStack);

	MUrMatrix_Multiply(
		topOfStack,
		inMatrix,
		topOfStack);

	MUrMatrixVerify(topOfStack);
}

void MUrMatrixStack_Translate(
	M3tMatrix4x3 *ioTOS,
	const M3tVector3D *inTranslate)
{
	float x = inTranslate->x;
	float y = inTranslate->y;
	float z = inTranslate->z;

	float r00 = ioTOS->m[0][0];
	float r01 = ioTOS->m[0][1];
	float r02 = ioTOS->m[0][2];

	float r10 = ioTOS->m[1][0];
	float r11 = ioTOS->m[1][1];
	float r12 = ioTOS->m[1][2];

	float r20 = ioTOS->m[2][0];
	float r21 = ioTOS->m[2][1];
	float r22 = ioTOS->m[2][2];

	float r30 = ioTOS->m[3][0];
	float r31 = ioTOS->m[3][1];
	float r32 = ioTOS->m[3][2];

	r30 += r00 * x + r10 * y + r20 * z;
	r31 += r01 * x + r11 * y + r21 * z;
	r32 += r02 * x + r12 * y + r22 * z;

	ioTOS->m[3][0] = r30;
	ioTOS->m[3][1] = r31;
	ioTOS->m[3][2] = r32;

	return;
}

void MUrMatrixStack_Scale(
	M3tMatrix4x3 *topOfStack,
	float inScale)
{
	M3tMatrix4x3 matrix;

	MUrMatrix_BuildUniformScale(inScale, &matrix);

	MUrMatrixVerify(topOfStack);

	MUrMatrix_Multiply(
		topOfStack,
		&matrix,
		topOfStack);

	MUrMatrixVerify(topOfStack);
}

void MUrMatrixStack_Quaternion(
	M3tMatrix4x3 *topOfStack,
	const M3tQuaternion *inQuaternion)
{
	M3tMatrix4x3 matrix;

	MUrQuatToMatrix(inQuaternion, &matrix);

#if defined(DEBUGGING) && DEBUGGING
	if (1)
	{
		M3tQuaternion testQuat, testQuat2;
		MUtEuler testEuler;
		M3tMatrix4x3 testMatrix;

		MUrMatrixToQuat(&matrix, &testQuat);
		MUrQuatToMatrix(&testQuat, &testMatrix);

		UUmAssert(MUrQuat_IsEqual(inQuaternion, &testQuat));

		testEuler = MUrQuatToEuler(&testQuat, MUcEulerOrderXYXs);
		testQuat = MUrEulerToQuat(&testEuler);
		MUrXYXsEulerToQuat(testEuler.x, testEuler.y, testEuler.z, &testQuat2);

		UUmAssert(MUrQuat_IsEqual(inQuaternion, &testQuat));
		UUmAssert(MUrQuat_IsEqual(&testQuat2, &testQuat));
	}
#endif

	MUrMatrixVerify(topOfStack);

	MUrMatrix_Multiply(
		topOfStack,
		&matrix,
		topOfStack);

	MUrMatrixVerify(topOfStack);
}

void MUrMatrixStack_Euler(
	M3tMatrix4x3 *topOfStack,
	const MUtEuler *inEuler)
{
	M3tMatrix4x3 eulerMatrix;

	MUrEulerToMatrix(inEuler, &eulerMatrix);

	MUrMatrixVerify(topOfStack);

	MUrMatrix_Multiply(
		topOfStack,
		&eulerMatrix,
		topOfStack);

	MUrMatrixVerify(topOfStack);
}

void MUrMatrix_Identity(M3tMatrix4x3 *ioMatrix)
{
	UUmAssertReadPtr(ioMatrix, sizeof(*ioMatrix));

	ioMatrix->m[0][0] = 1.0f;
	ioMatrix->m[0][1] = 0.0f;
	ioMatrix->m[0][2] = 0.0f;

	ioMatrix->m[1][0] = 0.0f;
	ioMatrix->m[1][1] = 1.0f;
	ioMatrix->m[1][2] = 0.0f;

	ioMatrix->m[2][0] = 0.0f;
	ioMatrix->m[2][1] = 0.0f;
	ioMatrix->m[2][2] = 1.0f;

	ioMatrix->m[3][0] = 0.0f;
	ioMatrix->m[3][1] = 0.0f;
	ioMatrix->m[3][2] = 0.0f;

	return;
}

void MUrMatrix3x3_Identity(M3tMatrix3x3 *ioMatrix)
{
	UUmAssertReadPtr(ioMatrix, sizeof(*ioMatrix));

	ioMatrix->m[0][0] = 1.0f;
	ioMatrix->m[0][1] = 0.0f;
	ioMatrix->m[0][2] = 0.0f;

	ioMatrix->m[1][0] = 0.0f;
	ioMatrix->m[1][1] = 1.0f;
	ioMatrix->m[1][2] = 0.0f;

	ioMatrix->m[2][0] = 0.0f;
	ioMatrix->m[2][1] = 0.0f;
	ioMatrix->m[2][2] = 1.0f;

	return;
}

void MUrMatrix_Translate(
	M3tMatrix4x3 		*ioMatrix,
	const M3tVector3D	*inTranslate)
{
	float x = inTranslate->x;
	float y = inTranslate->y;
	float z = inTranslate->z;

	float r00 = ioMatrix->m[0][0];
	float r01 = ioMatrix->m[0][1];
	float r02 = ioMatrix->m[0][2];

	float r10 = ioMatrix->m[1][0];
	float r11 = ioMatrix->m[1][1];
	float r12 = ioMatrix->m[1][2];

	float r20 = ioMatrix->m[2][0];
	float r21 = ioMatrix->m[2][1];
	float r22 = ioMatrix->m[2][2];

	float r30 = ioMatrix->m[3][0];
	float r31 = ioMatrix->m[3][1];
	float r32 = ioMatrix->m[3][2];

	r30 += r00 * x + r10 * y + r20 * z;
	r31 += r01 * x + r11 * y + r21 * z;
	r32 += r02 * x + r12 * y + r22 * z;

	ioMatrix->m[3][0] = r30;
	ioMatrix->m[3][1] = r31;
	ioMatrix->m[3][2] = r32;
}

void MUrMatrix_RotateX(M3tMatrix4x3 *matrix, float angle)
{
	M3tMatrix4x3 rotateMatrix;

	MUrMatrix_BuildRotateX(angle, &rotateMatrix);

	MUrMatrix_Multiply(&rotateMatrix, matrix, matrix);
}

void MUrMatrix_RotateY(M3tMatrix4x3 *matrix, float angle)
{
	M3tMatrix4x3 rotateMatrix;

	MUrMatrix_BuildRotateY(angle, &rotateMatrix);

	MUrMatrix_Multiply(&rotateMatrix, matrix, matrix);
}

void MUrMatrix_RotateZ(M3tMatrix4x3 *matrix, float angle)
{
	M3tMatrix4x3 rotateMatrix;

	MUrMatrix_BuildRotateZ(angle, &rotateMatrix);

	MUrMatrix_Multiply(&rotateMatrix, matrix, matrix);
}

void MUrMatrix3x3_Rotate(M3tMatrix3x3 *matrix, float inX, float inY, float inZ, float angle)
{
	M3tMatrix3x3 rotateMatrix;

	MUrMatrix3x3_BuildRotate(angle, inX, inY, inZ, &rotateMatrix);

	MUrMatrix3x3_Multiply(&rotateMatrix, matrix, matrix);
}

void MUrMatrix3x3_RotateX(M3tMatrix3x3 *matrix, float angle)
{
	M3tMatrix3x3 rotateMatrix;

	MUrMatrix3x3_BuildRotateX(angle, &rotateMatrix);

	MUrMatrix3x3_Multiply(&rotateMatrix, matrix, matrix);
}

void MUrMatrix3x3_RotateY(M3tMatrix3x3 *matrix, float angle)
{
	M3tMatrix3x3 rotateMatrix;

	MUrMatrix3x3_BuildRotateY(angle, &rotateMatrix);

	MUrMatrix3x3_Multiply(&rotateMatrix, matrix, matrix);
}

void MUrMatrix3x3_RotateZ(M3tMatrix3x3 *matrix, float angle)
{
	M3tMatrix3x3 rotateMatrix;

	MUrMatrix3x3_BuildRotateZ(angle, &rotateMatrix);

	MUrMatrix3x3_Multiply(&rotateMatrix, matrix, matrix);
}

#if defined(DEBUGGING) && DEBUGGING
void MUrMatrixVerify(const M3tMatrix4x3 *inMatrix)
{
	UUtUns8 i,j;

	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < 3; j++)
		{
			UUmAssert(inMatrix->m[i][j] > -1e9f && inMatrix->m[i][j] < 1e9f);
		}
	}
}
#endif

UUtBool MUrMatrix_IsEqual(
	const M3tMatrix4x3 *inMatrixA,
	const M3tMatrix4x3 *inMatrixB)
{
	UUtBool equal;
	UUtUns8 i,j;

	for(i = 0; i < 4; i++) {
		for(j = 0; j < 3; j++)
		{
			equal = UUmFloat_CompareEqu(inMatrixA->m[i][j], inMatrixB->m[i][j]);

			if (!equal) {
				return UUcFalse;
			}
		}
	}

	return UUcTrue;
}

void MUrMatrix_DecomposeAffine(const M3tMatrix4x3 *inMatrix, MUtAffineParts *outParts)
{
	HMatrix A;
	AffineParts parts;

	UUmAssertReadPtr(inMatrix, sizeof(*inMatrix));
	UUmAssertWritePtr(outParts, sizeof(*outParts));

	A[0][0] = inMatrix->m[0][0];
	A[1][0] = inMatrix->m[0][1]; //inMatrix->m[0][1];
	A[2][0] = inMatrix->m[0][2]; //inMatrix->m[0][2];
	A[3][0] = 0.f;

	A[0][1] = inMatrix->m[1][0]; //inMatrix->m[1][0];
	A[1][1] = inMatrix->m[1][1];
	A[2][1] = inMatrix->m[1][2]; //inMatrix->m[1][2];
	A[3][1] = 0.f;

	A[0][2] = inMatrix->m[2][0]; //inMatrix->m[2][0];
	A[1][2] = inMatrix->m[2][1]; //inMatrix->m[2][1];
	A[2][2] = inMatrix->m[2][2];
	A[3][2] = 0.f;

//	outTranslation->x = inMatrix->m[3][0];
//	outTranslation->y = inMatrix->m[3][1];
//	outTranslation->z = inMatrix->m[3][2];

	A[0][3] = inMatrix->m[3][0]; //inMatrix->m[3][0];
	A[1][3] = inMatrix->m[3][1]; //inMatrix->m[3][1];
	A[2][3] = inMatrix->m[3][2]; //inMatrix->m[3][2];
	A[3][3] = 1.f;

	decomp_affine(A, &parts);

	outParts->translation.x = parts.t.x;
	outParts->translation.y = parts.t.y;
	outParts->translation.z = parts.t.z;

	outParts->rotation.x = parts.q.x;
	outParts->rotation.y = parts.q.y;
	outParts->rotation.z = parts.q.z;
	outParts->rotation.w = -parts.q.w;		// shoemake appears to have

	outParts->stretchRotation.x = parts.u.x;
	outParts->stretchRotation.y = parts.u.y;
	outParts->stretchRotation.z = parts.u.z;
	outParts->stretchRotation.w = -parts.u.w;

	outParts->stretch.x = parts.k.x;
	outParts->stretch.y = parts.k.y;
	outParts->stretch.z = parts.k.z;

	outParts->sign = parts.f;

	return;
}

void MUrMatrix_To_RotationMatrix(const M3tMatrix4x3 *inMatrix, M3tMatrix4x3 *outMatrix)
{
	MUtAffineParts	parts;

	MUrMatrix_DecomposeAffine(inMatrix, &parts);

	// rotate
	MUrQuatToMatrix(&parts.rotation, outMatrix);

	return;
}

void MUrMatrix_StripScale(const M3tMatrix4x3 *inMatrix, M3tMatrix4x3 *outMatrix)
{
	MUtAffineParts	parts;
	M3tMatrix4x3		translateTM;
	M3tMatrix4x3		rotateTM;

	MUrMatrix_DecomposeAffine(inMatrix, &parts);

	// translate
	MUrMatrix_BuildTranslate(
		parts.translation.x,
		parts.translation.y,
		parts.translation.z,
		&translateTM);

	// rotate
	MUrQuatToMatrix(&parts.rotation, &rotateTM);

	// this should just be original matrix
	// outMatrix = rotateTM * translateTM;
	MUrMatrix_Multiply(&rotateTM, &translateTM, outMatrix);

	return;
}

UUtError MUrMatrix_GetUniformScale(M3tMatrix4x3 *inMatrix, float *outScale)
{
	UUtBool uniformScale;
	MUtAffineParts parts;

	MUrMatrix_DecomposeAffine(inMatrix, &parts);

	uniformScale =
		UUmFloat_CompareEquSloppy(parts.stretch.x, parts.stretch.y) &&
		UUmFloat_CompareEquSloppy(parts.stretch.x, parts.stretch.z) &&
		UUmFloat_CompareEquSloppy(parts.stretch.x, parts.stretch.z);

	if (!uniformScale) {
		return UUcError_Generic;
	}

	*outScale = parts.stretch.x;

	return UUcError_None;
}

void MUrMatrix_To_ScaleMatrix(const M3tMatrix4x3 *inMatrix, M3tMatrix4x3 *outMatrix)
{
	M3tQuaternion	invQuat;

	MUtAffineParts	parts;

	M3tMatrix4x3		stretchTM;
	M3tMatrix4x3		rotTM;
	M3tMatrix4x3		invRotTM;

	M3tMatrix4x3		temp;

	MUrMatrix_DecomposeAffine(inMatrix, &parts);

	MUrQuat_Inverse(&parts.stretchRotation, &invQuat);
	MUrQuatToMatrix(&parts.stretchRotation, &rotTM);
	MUrQuatToMatrix(&invQuat, &invRotTM);
	MUrMatrix_BuildScale(&parts.stretch, &stretchTM);

	MUrMatrix_Multiply(&rotTM, &stretchTM, &temp);
	MUrMatrix_Multiply(&temp, &invRotTM, outMatrix);

	return;
}

static UUtBool MUrMatrix_HasNonUniformScale(const M3tMatrix4x3 *inMatrix)
{
	MUtAffineParts	parts;
	UUtBool			hasNonUniformScale;

	MUrMatrix_DecomposeAffine(inMatrix, &parts);

	hasNonUniformScale =
		(!UUmFloat_CompareEqu(parts.stretch.x, parts.stretch.y)) ||
		(!UUmFloat_CompareEqu(parts.stretch.x, parts.stretch.z)) ||
		(!UUmFloat_CompareEqu(parts.stretch.y, parts.stretch.z));

	return hasNonUniformScale;
}

void
MUrMatrix3x3_ExtractFrom4x3(
	M3tMatrix3x3		*outMatrix3x3,
	const M3tMatrix4x3	*inMatrix4x3)
{
	outMatrix3x3->m[0][0] = inMatrix4x3->m[0][0];
	outMatrix3x3->m[0][1] = inMatrix4x3->m[0][1];
	outMatrix3x3->m[0][2] = inMatrix4x3->m[0][2];
	outMatrix3x3->m[1][0] = inMatrix4x3->m[1][0];
	outMatrix3x3->m[1][1] = inMatrix4x3->m[1][1];
	outMatrix3x3->m[1][2] = inMatrix4x3->m[1][2];
	outMatrix3x3->m[2][0] = inMatrix4x3->m[2][0];
	outMatrix3x3->m[2][1] = inMatrix4x3->m[2][1];
	outMatrix3x3->m[2][2] = inMatrix4x3->m[2][2];
}

void
MUrMatrix4x3_Multiply3x3(
	const M3tMatrix4x3*		inMatrix4x3,
	const M3tMatrix3x3*		inMatrix3x3,
	M3tMatrix4x3			*outMatrix4x3)
{
	float	a00, a10, a20, a30;
	float	a01, a11, a21, a31;
	float	a02, a12, a22, a32;

	float	b0, b1, b2;
	float	d0, d1, d2;

	a00 = inMatrix4x3->m[0][0];
	a01 = inMatrix4x3->m[0][1];
	a02 = inMatrix4x3->m[0][2];

	a10 = inMatrix4x3->m[1][0];
	a11 = inMatrix4x3->m[1][1];
	a12 = inMatrix4x3->m[1][2];

	a20 = inMatrix4x3->m[2][0];
	a21 = inMatrix4x3->m[2][1];
	a22 = inMatrix4x3->m[2][2];

	a30 = inMatrix4x3->m[3][0];
	a31 = inMatrix4x3->m[3][1];
	a32 = inMatrix4x3->m[3][2];

	b0 = inMatrix3x3->m[0][0];
	b1 = inMatrix3x3->m[0][1];
	b2 = inMatrix3x3->m[0][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;

	outMatrix4x3->m[0][0] = d0;
	outMatrix4x3->m[0][1] = d1;
	outMatrix4x3->m[0][2] = d2;

	b0 = inMatrix3x3->m[1][0];
	b1 = inMatrix3x3->m[1][1];
	b2 = inMatrix3x3->m[1][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;

	outMatrix4x3->m[1][0] = d0;
	outMatrix4x3->m[1][1] = d1;
	outMatrix4x3->m[1][2] = d2;

	b0 = inMatrix3x3->m[2][0];
	b1 = inMatrix3x3->m[2][1];
	b2 = inMatrix3x3->m[2][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;

	outMatrix4x3->m[2][0] = d0;
	outMatrix4x3->m[2][1] = d1;
	outMatrix4x3->m[2][2] = d2;

	d0 = a30;
	d1 = a31;
	d2 = a32;

	outMatrix4x3->m[3][0] = d0;
	outMatrix4x3->m[3][1] = d1;
	outMatrix4x3->m[3][2] = d2;
}

void
MUrMatrix4x3_BuildFrom3x3(
	M3tMatrix4x3			*outMatrix4x3,
	const M3tMatrix3x3*		inMatrix3x3)
{
	outMatrix4x3->m[0][0] = inMatrix3x3->m[0][0];
	outMatrix4x3->m[0][1] = inMatrix3x3->m[0][1];
	outMatrix4x3->m[0][2] = inMatrix3x3->m[0][2];

	outMatrix4x3->m[1][0] = inMatrix3x3->m[1][0];
	outMatrix4x3->m[1][1] = inMatrix3x3->m[1][1];
	outMatrix4x3->m[1][2] = inMatrix3x3->m[1][2];

	outMatrix4x3->m[2][0] = inMatrix3x3->m[2][0];
	outMatrix4x3->m[2][1] = inMatrix3x3->m[2][1];
	outMatrix4x3->m[2][2] = inMatrix3x3->m[2][2];

	outMatrix4x3->m[3][0] = 0.0f;
	outMatrix4x3->m[3][1] = 0.0f;
	outMatrix4x3->m[3][2] = 0.0f;
}

void MUrMatrix_DualAlignment(
	const M3tVector3D *src1,
	const M3tVector3D *src2,
	const M3tVector3D *dst1,
	const M3tVector3D *dst2,
	M3tMatrix4x3 *outMatrix)
{
	M3tMatrix4x3 alignment_matrix1;
	M3tMatrix4x3 alignment_matrix2;
	M3tVector3D positive_z = { 0, 0, 1.f };
	M3tVector3D src2_aligned1;
	M3tVector3D src2_aligned2;
	M3tVector3D dst2_aligned2;
	float angle_src;
	float angle_dst;
	float angle_delta;
	M3tMatrix4x3 rotate_matrix;

	M3tVector3D verify1;
	M3tVector3D verify2;

	// build a matrix that aligns src1 to dst1, call this align 1
	// find the rotation such that src2 aligned by align1 rotated around dst1 reaches dst2
	// get two points in the x,y plane: src2 and dst2, find angle between them

	MUrMatrix_Alignment(src1, dst1, &alignment_matrix1);
	MUrMatrix_Alignment(dst1, &positive_z, &alignment_matrix2);

	MUrMatrix_MultiplyPoint(src2, &alignment_matrix1, &src2_aligned1);

	MUrMatrix_MultiplyPoint(&src2_aligned1, &alignment_matrix2, &src2_aligned2);
	MUrMatrix_MultiplyPoint(dst2, &alignment_matrix2, &dst2_aligned2);

	angle_src = MUrATan2(src2_aligned2.x, src2_aligned2.y);
	angle_dst = MUrATan2(dst2_aligned2.x, dst2_aligned2.y);
	angle_delta = angle_dst - angle_src;
	UUmTrig_Clip(angle_delta);

	MUrMatrix_BuildRotate(angle_delta, dst2->x, dst2->y, dst2->z, &rotate_matrix);

	// ok, we have a 2 stage process, alignment matrix, then rotate matrix
	MUrMatrix_Multiply(&alignment_matrix1, &rotate_matrix, outMatrix);

	// verify
	MUrMatrix_MultiplyPoint(src1, outMatrix, &verify1);
	MUrMatrix_MultiplyPoint(src2, outMatrix, &verify2);

	UUmAssert(MUrVector_IsEqual(&verify1, dst1));
	UUmAssert(MUrVector_IsEqual(&verify2, dst2));

	return;
}

void
MUrMath_Matrix4x4Multiply(
	M3tMatrix4x4*	inMatrixA,
	M3tMatrix4x4*	inMatrixB,
	M3tMatrix4x4	*outResult)
{
	float	a00, a10, a20, a30;
	float	a01, a11, a21, a31;
	float	a02, a12, a22, a32;
	float	a03, a13, a23, a33;

	float	b0, b1, b2, b3;
	float	d0, d1, d2, d3;

	a00 = inMatrixA->m[0][0];
	a01 = inMatrixA->m[0][1];
	a02 = inMatrixA->m[0][2];
	a03 = inMatrixA->m[0][3];

	a10 = inMatrixA->m[1][0];
	a11 = inMatrixA->m[1][1];
	a12 = inMatrixA->m[1][2];
	a13 = inMatrixA->m[1][3];

	a20 = inMatrixA->m[2][0];
	a21 = inMatrixA->m[2][1];
	a22 = inMatrixA->m[2][2];
	a23 = inMatrixA->m[2][3];

	a30 = inMatrixA->m[3][0];
	a31 = inMatrixA->m[3][1];
	a32 = inMatrixA->m[3][2];
	a33 = inMatrixA->m[3][3];

	b0 = inMatrixB->m[0][0];
	b1 = inMatrixB->m[0][1];
	b2 = inMatrixB->m[0][2];
	b3 = inMatrixB->m[0][3];

	d0 = a00 * b0 + a10 * b1 + a20 * b2 + a30 * b3;
	d1 = a01 * b0 + a11 * b1 + a21 * b2 + a31 * b3;
	d2 = a02 * b0 + a12 * b1 + a22 * b2 + a32 * b3;
	d3 = a03 * b0 + a13 * b1 + a23 * b2 + a33 * b3;

	outResult->m[0][0] = d0;
	outResult->m[0][1] = d1;
	outResult->m[0][2] = d2;
	outResult->m[0][3] = d3;

	b0 = inMatrixB->m[1][0];
	b1 = inMatrixB->m[1][1];
	b2 = inMatrixB->m[1][2];
	b3 = inMatrixB->m[1][3];

	d0 = a00 * b0 + a10 * b1 + a20 * b2 + a30 * b3;
	d1 = a01 * b0 + a11 * b1 + a21 * b2 + a31 * b3;
	d2 = a02 * b0 + a12 * b1 + a22 * b2 + a32 * b3;
	d3 = a03 * b0 + a13 * b1 + a23 * b2 + a33 * b3;

	outResult->m[1][0] = d0;
	outResult->m[1][1] = d1;
	outResult->m[1][2] = d2;
	outResult->m[1][3] = d3;

	b0 = inMatrixB->m[2][0];
	b1 = inMatrixB->m[2][1];
	b2 = inMatrixB->m[2][2];
	b3 = inMatrixB->m[2][3];

	d0 = a00 * b0 + a10 * b1 + a20 * b2 + a30 * b3;
	d1 = a01 * b0 + a11 * b1 + a21 * b2 + a31 * b3;
	d2 = a02 * b0 + a12 * b1 + a22 * b2 + a32 * b3;
	d3 = a03 * b0 + a13 * b1 + a23 * b2 + a33 * b3;

	outResult->m[2][0] = d0;
	outResult->m[2][1] = d1;
	outResult->m[2][2] = d2;
	outResult->m[2][3] = d3;

	b0 = inMatrixB->m[3][0];
	b1 = inMatrixB->m[3][1];
	b2 = inMatrixB->m[3][2];
	b3 = inMatrixB->m[3][3];

	d0 = a00 * b0 + a10 * b1 + a20 * b2 + a30 * b3;
	d1 = a01 * b0 + a11 * b1 + a21 * b2 + a31 * b3;
	d2 = a02 * b0 + a12 * b1 + a22 * b2 + a32 * b3;
	d3 = a03 * b0 + a13 * b1 + a23 * b2 + a33 * b3;

	outResult->m[3][0] = d0;
	outResult->m[3][1] = d1;
	outResult->m[3][2] = d2;
	outResult->m[3][3] = d3;
}

void
MUrMath_Matrix4x4Multiply4x3(
	M3tMatrix4x4*	inMatrixA,
	M3tMatrix4x3*	inMatrixB,
	M3tMatrix4x4	*outResult)
{
	float	a00, a10, a20, a30;
	float	a01, a11, a21, a31;
	float	a02, a12, a22, a32;
	float	a03, a13, a23, a33;

	float	b0, b1, b2;
	float	d0, d1, d2, d3;

	a00 = inMatrixA->m[0][0];
	a01 = inMatrixA->m[0][1];
	a02 = inMatrixA->m[0][2];
	a03 = inMatrixA->m[0][3];

	a10 = inMatrixA->m[1][0];
	a11 = inMatrixA->m[1][1];
	a12 = inMatrixA->m[1][2];
	a13 = inMatrixA->m[1][3];

	a20 = inMatrixA->m[2][0];
	a21 = inMatrixA->m[2][1];
	a22 = inMatrixA->m[2][2];
	a23 = inMatrixA->m[2][3];

	a30 = inMatrixA->m[3][0];
	a31 = inMatrixA->m[3][1];
	a32 = inMatrixA->m[3][2];
	a33 = inMatrixA->m[3][3];

	b0 = inMatrixB->m[0][0];
	b1 = inMatrixB->m[0][1];
	b2 = inMatrixB->m[0][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;
	d3 = a03 * b0 + a13 * b1 + a23 * b2;

	outResult->m[0][0] = d0;
	outResult->m[0][1] = d1;
	outResult->m[0][2] = d2;
	outResult->m[0][3] = d3;

	b0 = inMatrixB->m[1][0];
	b1 = inMatrixB->m[1][1];
	b2 = inMatrixB->m[1][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;
	d3 = a03 * b0 + a13 * b1 + a23 * b2;

	outResult->m[1][0] = d0;
	outResult->m[1][1] = d1;
	outResult->m[1][2] = d2;
	outResult->m[1][3] = d3;

	b0 = inMatrixB->m[2][0];
	b1 = inMatrixB->m[2][1];
	b2 = inMatrixB->m[2][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2;
	d1 = a01 * b0 + a11 * b1 + a21 * b2;
	d2 = a02 * b0 + a12 * b1 + a22 * b2;
	d3 = a03 * b0 + a13 * b1 + a23 * b2;

	outResult->m[2][0] = d0;
	outResult->m[2][1] = d1;
	outResult->m[2][2] = d2;
	outResult->m[2][3] = d3;

	b0 = inMatrixB->m[3][0];
	b1 = inMatrixB->m[3][1];
	b2 = inMatrixB->m[3][2];

	d0 = a00 * b0 + a10 * b1 + a20 * b2 + a30;
	d1 = a01 * b0 + a11 * b1 + a21 * b2 + a31;
	d2 = a02 * b0 + a12 * b1 + a22 * b2 + a32;
	d3 = a03 * b0 + a13 * b1 + a23 * b2 + a33;

	outResult->m[3][0] = d0;
	outResult->m[3][1] = d1;
	outResult->m[3][2] = d2;
	outResult->m[3][3] = d3;
}

void MUrMatrix_From_Point_And_Vectors(
	M3tMatrix4x3 *inMatrix,
	const M3tPoint3D *inPoint,
	const M3tVector3D *inForward,
	const M3tVector3D *inUp)
{
	M3tVector3D left;

	MUrVector_CrossProduct(inUp, inForward, &left);

	inMatrix->m[0][0]= inForward->x;	inMatrix->m[0][1]= inForward->y;	inMatrix->m[0][2]= inForward->z;
	inMatrix->m[1][0]= left.x;			inMatrix->m[1][1]= left.y;			inMatrix->m[1][2]= left.z;
	inMatrix->m[2][0]= inUp->x;			inMatrix->m[2][1]= inUp->y;			inMatrix->m[2][2]= inUp->z;
	inMatrix->m[3][0]= inPoint->x;		inMatrix->m[3][1]= inPoint->y;		inMatrix->m[3][2]= inPoint->z;

	return;
}


void MUrMatrix_Lerp(const M3tMatrix4x3 *inFrom, const M3tMatrix4x3 *inTo, float t, M3tMatrix4x3 *outResult)
{
	MUtAffineParts inFromParts;
	MUtAffineParts inToParts;
	MUtAffineParts lerpParts;
	M3tMatrix4x3 rotateTM;
	M3tMatrix4x3 translateTM;

	MUrMatrix_DecomposeAffine(inFrom, &inFromParts);
	MUrMatrix_DecomposeAffine(inTo, &inToParts);

	MUrPoint3D_Lerp(&inFromParts.translation, &inToParts.translation, t, &lerpParts.translation);
	MUrPoint3D_Lerp(&inFromParts.stretch, &inToParts.stretch, t, &lerpParts.stretch);
	MUrQuat_Lerp(&inFromParts.rotation, &inToParts.rotation, t, &lerpParts.rotation);

	lerpParts.stretchRotation = inFromParts.stretchRotation;
	lerpParts.sign = inFromParts.sign;
	UUmAssert(inFromParts.sign == inToParts.sign);

	// outMatrix = (stretchRotateTM * stretchTM * invStretchRotateTM) * rotateTM * translateTM;

	// translate
	MUrMatrix_BuildTranslate(
		lerpParts.translation.x,
		lerpParts.translation.y,
		lerpParts.translation.z,
		&translateTM);

	// rotate
	MUrQuatToMatrix(&lerpParts.rotation, &rotateTM);

	// this should just be original matrix
	// outMatrix = rotateTM * translateTM;
	MUrMatrix_Multiply(&translateTM, &rotateTM, outResult);

	return;
}
