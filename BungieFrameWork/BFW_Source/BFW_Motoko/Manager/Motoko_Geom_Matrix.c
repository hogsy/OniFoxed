/*
	FILE:	Motoko_Geom_Matrix.c

	AUTHOR:	Brent H. Pease

	CREATED: April 2, 1997

	PURPOSE: Interface to the Motoko 3D engine

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "BFW_Shared_Math.h"

#include "Motoko_Manager.h"

typedef struct M3tManager_MatrixGlobals
{

	/* Model matrix stack */
		UUtInt32				matrix_TOS;
		M3tMatrix4x3*			matrixStack;
		M3tMatrix4x3*			matrixStackTop;

} M3tManager_MatrixGlobals;

M3tManager_MatrixGlobals	M3gManager_MatrixGlobals;

UUtError
M3rMatrixStack_Push(
	void)
{
	if(M3gManager_MatrixGlobals.matrix_TOS >= M3cModelMatrix_MaxDepth)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Stack overflow");
	}

	M3gManager_MatrixGlobals.matrix_TOS++;
	M3gManager_MatrixGlobals.matrixStackTop++;

	*M3gManager_MatrixGlobals.matrixStackTop = *(M3gManager_MatrixGlobals.matrixStackTop - 1);

	MSmStackVerify();

	return UUcError_None;
}

UUtError
M3rMatrixStack_Get(
	M3tMatrix4x3*		*outMatrix)
{
	*outMatrix = M3gManager_MatrixGlobals.matrixStackTop;

	return UUcError_None;
}

UUtError
M3rMatrixStack_Pop(
	void)
{
	if(M3gManager_MatrixGlobals.matrix_TOS <= 0)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Stack underflow");
	}
	M3gManager_MatrixGlobals.matrix_TOS--;
	M3gManager_MatrixGlobals.matrixStackTop--;

	MSmStackVerify();

	return UUcError_None;
}

void
M3rMatrixStack_Identity(
	void)
{
	M3tMatrix4x3*	matrix = M3gManager_MatrixGlobals.matrixStackTop;

	matrix->m[0][0] = 1.0f;
	matrix->m[0][1] = 0.0f;
	matrix->m[0][2] = 0.0f;

	matrix->m[1][0] = 0.0f;
	matrix->m[1][1] = 1.0f;
	matrix->m[1][2] = 0.0f;

	matrix->m[2][0] = 0.0f;
	matrix->m[2][1] = 0.0f;
	matrix->m[2][2] = 1.0f;

	matrix->m[3][0] = 0.0f;
	matrix->m[3][1] = 0.0f;
	matrix->m[3][2] = 0.0f;

	MSmStackVerify();

}

void
M3rMatrixStack_Clear(
	void)
{
	M3gManager_MatrixGlobals.matrix_TOS = 0;
	M3gManager_MatrixGlobals.matrixStackTop = M3gManager_MatrixGlobals.matrixStack;
	M3rMatrixStack_Identity();
}

void
M3rMatrixStack_Rotate(
	float				inRadians,
	float				inX,
	float				inY,
	float				inZ)
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
	MUrMatrix_Multiply(
		M3gManager_MatrixGlobals.matrixStackTop,
		&matrix,
		M3gManager_MatrixGlobals.matrixStackTop);
	MSmStackVerify();
}

void
M3rMatrixStack_UniformScale(
	float				inScale)
{
	MUrMatrixStack_Scale(M3gManager_MatrixGlobals.matrixStackTop, inScale);
}

void
M3rMatrixStack_Translate(
	float				inX,
	float				inY,
	float				inZ)
{
	M3tVector3D translate;

	UUmAssert(inY > -1e9f && inY < 1e9f);
	UUmAssert(inZ > -1e9f && inZ < 1e9f);

	translate.x = inX;
	translate.y = inY;
	translate.z = inZ;

	MUrMatrixStack_Translate(M3gManager_MatrixGlobals.matrixStackTop, &translate);
}

void
M3rMatrixStack_Quaternion(
	const M3tQuaternion*	inQuaternion)
{
	MUrMatrixStack_Quaternion(M3gManager_MatrixGlobals.matrixStackTop, inQuaternion);
}

void
M3rMatrixStack_Multiply(
	const M3tMatrix4x3*		inMatrix)
{
	MSmMatrixVerify(inMatrix);
	MUrMatrix_Multiply(
		M3gManager_MatrixGlobals.matrixStackTop,
		inMatrix,
		M3gManager_MatrixGlobals.matrixStackTop);
	MSmStackVerify();
}

UUtError
M3rManager_Matrix_Initialize(
	void)
{
	M3gManager_MatrixGlobals.matrix_TOS = -1;
	M3gManager_MatrixGlobals.matrixStack = UUrMemory_Block_New(sizeof(M3tMatrix4x4) * M3cModelMatrix_MaxDepth);
	UUmError_ReturnOnNull(M3gManager_MatrixGlobals.matrixStack);

	M3gManager_MatrixGlobals.matrixStackTop = M3gManager_MatrixGlobals.matrixStack;

	return UUcError_None;
}

void
M3rManager_Matrix_Terminate(
	void)
{
	if(M3gManager_MatrixGlobals.matrixStack != NULL)
	{
		UUrMemory_Block_Delete(M3gManager_MatrixGlobals.matrixStack);
	}
}
