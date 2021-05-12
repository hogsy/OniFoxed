/*
	FILE:	MS_GC_Method_Matrix.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 19, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#if 0
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Console.h"

#include "MS_GC_Private.h"
#include "MS_GC_Method_Matrix.h"

static void MSiStackMultiply(const M3tMatrix4x3 *inMatrix)
{

	MUrMatrix_Multiply(
		MSgGeomContextPrivate->matrixStackTop,
		inMatrix,
		MSgGeomContextPrivate->matrixStackTop);

	MSgGeomContextPrivate->matrixStackDirty = UUcTrue;
}

UUtError
MSrGeomContext_Method_MatrixStack_Push(
	void)
{
	UUtInt32*				tosPtr;
	UUtInt32				tosValue;
	
	if(MSgGeomContextPrivate->matrix_TOS >= M3cModelMatrix_MaxDepth)
	{
		return UUcError_Generic;
	}
	
	tosPtr = &MSgGeomContextPrivate->matrix_TOS;
	
	tosValue = *tosPtr;
	
	(MSgGeomContextPrivate->matrixStack)[tosValue + 1] =
		(MSgGeomContextPrivate->matrixStack)[tosValue];

	*tosPtr += 1;	
	
	MSgGeomContextPrivate->matrixStackTop++;
	
	MSgGeomContextPrivate->matrixStackDirty = UUcTrue;

	MSmStackVerify();

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_MatrixStack_Pop(
	void)
{
	UUtInt32				*tosPtr;

	tosPtr = &MSgGeomContextPrivate->matrix_TOS;

	if(*tosPtr <= 0)
	{
		return UUcError_Generic;
	}
	else
	{
		*tosPtr -= 1;
	}
	
	MSgGeomContextPrivate->matrixStackTop--;
	
	MSgGeomContextPrivate->matrixStackDirty = UUcTrue;

	MSmStackVerify();
	
	return UUcError_None;
}

UUtError
MSrGeomContext_Method_MatrixStack_Get(
	M3tMatrix4x3*		outMatrix3)
{
	M3tMatrix4x3				*curModelMatrix = (M3tMatrix4x3 *) MSgGeomContextPrivate->matrixStackTop;

	UUmAssertWritePtr(outMatrix3, sizeof(M3tMatrix4x3));
	
	*outMatrix3 = *curModelMatrix;

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_MatrixStack_Identity(
	void)
{
	M3tMatrix4x3*			m;
	UUtInt32				tosValue;
	
	tosValue = MSgGeomContextPrivate->matrix_TOS;
	
	m = &(MSgGeomContextPrivate->matrixStack)[tosValue];

	// build identity matrix	
	m->m[0][0] = 1.0f;
	m->m[0][1] = 0.0f;
	m->m[0][2] = 0.0f;
	
	m->m[1][0] = 0.0f;
	m->m[1][1] = 1.0f;
	m->m[1][2] = 0.0f;
	
	m->m[2][0] = 0.0f;
	m->m[2][1] = 0.0f;
	m->m[2][2] = 1.0f;
	
	m->m[3][0] = 0.0f;
	m->m[3][1] = 0.0f;
	m->m[3][2] = 0.0f;
	
	MSgGeomContextPrivate->matrixStackDirty = UUcTrue;

	MSmStackVerify();
	
	return UUcError_None;
}

UUtError
MSrGeomContext_Method_MatrixStack_Clear(
	void)
{
	
	// remove everything from the stack and the final element is the identity
	MSgGeomContextPrivate->matrix_TOS = 0;
	
	MSgGeomContextPrivate->matrixStackTop = MSgGeomContextPrivate->matrixStack;

	MSrGeomContext_Method_MatrixStack_Identity();

	MSgGeomContextPrivate->matrixStackDirty = UUcTrue;

	MSmStackVerify();
	
	return UUcError_None;
}

UUtError
MSrGeomContext_Method_MatrixStack_UniformScale(
	float			inScale)
{
	M3tMatrix4x3 *topOfStack = MSgGeomContextPrivate->matrixStackTop;

	MUrMatrixStack_Scale(topOfStack, inScale);

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_MatrixStack_Rotate(
	float			inRadians,
	float			inX,
	float			inY,
	float			inZ)
{
	M3tMatrix4x3				matrix;

	UUmAssert(inX > -1e9f && inX < 1e9f);
	UUmAssert(inY > -1e9f && inY < 1e9f);
	UUmAssert(inZ > -1e9f && inZ < 1e9f);
	UUmAssertTrigRange(inRadians);
	
	MUrMatrix_BuildRotate(
		inRadians,
		inX,
		inY,
		inZ,
		&matrix);
	
	MSmMatrixVerify(&matrix);
	MSiStackMultiply(&matrix);
	MSmStackVerify();

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_MatrixStack_Translate(
	float			inX,
	float			inY,
	float			inZ)
{
	M3tMatrix4x3 *topOfStack = MSgGeomContextPrivate->matrixStackTop;

	M3tVector3D translate;

	UUmAssert(inX > -1e9f && inX < 1e9f);
	UUmAssert(inY > -1e9f && inY < 1e9f);
	UUmAssert(inZ > -1e9f && inZ < 1e9f);

	translate.x = inX;
	translate.y = inY;
	translate.z = inZ;

	MUrMatrixStack_Translate(topOfStack, &translate);

	return UUcError_None;
}

UUtError
MSrGeomContext_Method_MatrixStack_Quaternion(
	const M3tQuaternion*	inQuaternion)
{
	M3tMatrix4x3 *topOfStack = MSgGeomContextPrivate->matrixStackTop;

	MUrMatrixStack_Quaternion(topOfStack, inQuaternion);

	return UUcError_None;
}


UUtError
MSrGeomContext_Method_MatrixStack_Multiply(
	const M3tMatrix4x3*	inMatrix)
{
	const M3tMatrix4x3	*matrix = (M3tMatrix4x3 *) inMatrix;

	MSmMatrixVerify(matrix);
	MSiStackMultiply(matrix);
	MSmStackVerify();

	return UUcError_None;
}

void MSrGeomContext_Method_MatrixToQuat(
	const M3tMatrix4x3 *inMatrix, 
	M3tQuaternion *outQuat)
{
	M3tMatrix4x3 *privateInMatrix = (M3tMatrix4x3 *) inMatrix;

	MUrMatrixToQuat(privateInMatrix, outQuat);
}

void
MSrGeomContext_Method_QuatToMatrix(
	const M3tQuaternion *inQuat, 
	M3tMatrix4x3 *outMatrix)
{
	M3tMatrix4x3 *privateOutMatrix = (M3tMatrix4x3 *) outMatrix;

	MUrQuatToMatrix(inQuat, privateOutMatrix);
}

void 
MSrGeomContext_Method_Matrix_GetTranslation(
	const M3tMatrix4x3 *inMatrix, 
	M3tPoint3D *outTranslation)
{
	M3tMatrix4x3 *privateInMatrix = (M3tMatrix4x3 *) inMatrix;

	*outTranslation = MUrMatrix_GetTranslation(privateInMatrix);
}

void
MSrGeomContext_Method_Matrix_Multiply (
	const M3tMatrix4x3	*inMatrixA, 
	const M3tMatrix4x3	*inMatrixB, 
	M3tMatrix4x3			*outResult)
{
	M3tMatrix4x3 *pInA = (M3tMatrix4x3 *) inMatrixA;
	M3tMatrix4x3 *pInB = (M3tMatrix4x3 *) inMatrixA;
	M3tMatrix4x3 *pOut = (M3tMatrix4x3 *) outResult;

	MUrMatrix_Multiply(pInA, pInB, pOut);
}

void
MSrGeomContext_Method_Matrix_MultiplyPoints (
	const M3tMatrix4x3	 *inMatrix, 
	const UUtUns16	      inNumPoints,
	const M3tPoint3D	 *inPoints, 
	M3tPoint3D			 *outPoints)
{
	M3tMatrix4x3 *privateInMatrix = (M3tMatrix4x3 *) inMatrix;

//	UUmAssert(((unsigned long)inPoints & UUcProcessor_CacheLineSize_Mask) == 0);
//	UUmAssert(((unsigned long)outPoints & UUcProcessor_CacheLineSize_Mask) == 0);

	MUrMatrix_MultiplyPoints(inNumPoints, privateInMatrix, inPoints, outPoints);
}

void MSrGeomContext_Method_Matrix_Alignment(
	const M3tVector3D *inFrom,
	const M3tVector3D *inTo,
	M3tMatrix4x3 *outMatrix)
{
	MUrMatrix_Alignment(inFrom, inTo, (M3tMatrix4x3 *) outMatrix);
}
#endif