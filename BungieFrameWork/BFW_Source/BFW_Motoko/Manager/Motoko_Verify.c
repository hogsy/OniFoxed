/*
	FILE:	Motoko_Verify.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: May 5, 1997
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997-1999

*/
#include "BFW.h"
#include "BFW_Motoko.h"
#include "Motoko_Verify.h"


#if defined(DEBUGGING) && DEBUGGING
UUtError
M3rVerify_Geometry(
	const M3tGeometry *inGeometry)
{
	const char *instanceName = TMrInstance_GetInstanceName(inGeometry);
	UUtUns32	numPoints = inGeometry->pointArray->numPoints;
	UUtUns32	itr;
	UUtError	error;

	// verify the points
	for(itr = 0; itr < numPoints; itr++)
	{
		error = M3rVerify_Point3D(inGeometry->pointArray->points + itr);
		UUmAssert(UUcError_None == error);
	}		
	
	if (instanceName != NULL)
	{
		int set_breakpoint_here_if_you_like = 0;
	}

	// verify the tri strips

		for(itr = 0; itr < inGeometry->triStripArray->numIndices; itr++)
		{
			UUmAssert((inGeometry->triStripArray->indices[itr] & 0x7FFF) < numPoints);
		}

	// verify the tri normal indices
		
		for(itr = 0; itr < inGeometry->triNormalIndexArray->numIndices; itr++)
		{
			UUmAssert(inGeometry->triNormalIndexArray->indices[itr] < 
				inGeometry->triNormalArray->numVectors);
		}

	return UUcError_None;
}

UUtError
M3rVerify_Point3D(
	const M3tPoint3D *inPoint)
{
	UUtError result;
	UUtBool validPoint;

	validPoint = UUcTrue;
	validPoint &= (inPoint->x > -1e9f) && (inPoint->x < 1e9f);
	validPoint &= (inPoint->y > -1e9f) && (inPoint->y < 1e9f);
	validPoint &= (inPoint->z > -1e9f) && (inPoint->z < 1e9f);

	if (validPoint) {
		result = UUcError_None;
	}
	else {
		result = UUcError_Generic;
	}

	return result;
}

UUtError
M3rVerify_PointScreen(
	const M3tPointScreen *inPoint)
{
	UUtError result;
	UUtBool validPoint;

	validPoint = UUcTrue;
	validPoint &= (inPoint->x > -1e9f) && (inPoint->x < 1e9f);
	validPoint &= (inPoint->y > -1e9f) && (inPoint->y < 1e9f);
	validPoint &= (inPoint->z > -1e9f) && (inPoint->z < 1e9f);
	validPoint &= (inPoint->invW > -1e9f) && (inPoint->invW < 1e9f);

	if (validPoint) {
		result = UUcError_None;
	}
	else {
		result = UUcError_Generic;
	}

	return result;
}

void MSrMatrixVerify(const M3tMatrix4x3 *m)
{
	UUtUns32 i,j;

	UUmAssertReadPtr(m, sizeof(*m));

	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < 3; j++)
		{
			UUmAssert(m->m[i][j] > -1e9f && m->m[i][j] < 1e9f);
		}
	}
}

void MSrMatrix3x3Verify(const M3tMatrix3x3 *m)
{
	UUtUns32 i,j;

	UUmAssertReadPtr(m, sizeof(*m));

	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 3; j++)
		{
			UUmAssert(m->m[i][j] > -1e9f && m->m[i][j] < 1e9f);
		}
	}
}

void MSrStackVerify_Debug(void)
{
	M3tMatrix4x3*	m;
	
	M3rMatrixStack_Get(&m);

	MSmMatrixVerify(m);
}

#endif

