/*
	FILE:	OG_GC_Method_Matrix.h

	AUTHOR:	Brent H. Pease

	CREATED: June 10, 1999

	PURPOSE:

	Copyright 1997

*/
#ifndef OG_GC_METHOD_MATRIX_H
#define OG_GC_METHOD_MATRIX_H
#error
#if 0
UUtError
OGrGeomContext_Method_MatrixStack_Push(
	void);

UUtError
OGrGeomContext_Method_MatrixStack_Pop(
	void);

UUtError
OGrGeomContext_Method_MatrixStack_Get(
	M3tMatrix4x3*		outMatrix3);

UUtError
OGrGeomContext_Method_MatrixStack_Identity(
	void);

UUtError
OGrGeomContext_Method_MatrixStack_Clear(
	void);

UUtError
OGrGeomContext_Method_MatrixStack_UniformScale(
	float			inScale);

UUtError
OGrGeomContext_Method_MatrixStack_Rotate(
	float			inRadians,
	float			inX,
	float			inY,
	float			inZ);

UUtError
OGrGeomContext_Method_MatrixStack_Translate(
	float			inX,
	float			inY,
	float			inZ);

UUtError
OGrGeomContext_Method_MatrixStack_Quaternion(
	const M3tQuaternion*	inQuaternion);

UUtError
OGrGeomContext_Method_MatrixStack_Multiply(
	const M3tMatrix4x3*	inMatrix);

void
OGrGeomContext_Method_MatrixToQuat(
	const M3tMatrix4x3 *inMatrix,
	M3tQuaternion *outQuat);

void
OGrGeomContext_Method_QuatToMatrix(
	const M3tQuaternion *inQuat,
	M3tMatrix4x3 *outMatrix);

void
OGrGeomContext_Method_Matrix_GetTranslation(
	const M3tMatrix4x3 *inMatrix,
	M3tPoint3D *outTranslation);

void
OGrGeomContext_Method_Matrix_Multiply (
	const M3tMatrix4x3	*inMatrixA,
	const M3tMatrix4x3	*inMatrixB,
	M3tMatrix4x3			*outResult);

void
OGrGeomContext_Method_Matrix_MultiplyPoints (
	const M3tMatrix4x3	 *inMatrix,
	const UUtUns16		  inNumPoints,
	const M3tPoint3D	 *inPoint,
	M3tPoint3D			 *outPoint);

void
OGrGeomContext_Method_Matrix_Alignment(
	const M3tVector3D *inFrom,
	const M3tVector3D *inTo,
	M3tMatrix4x3 *outMatrix);
#endif

#endif



