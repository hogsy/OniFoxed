/*
	FILE:	BFW_LSSolution.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Jan 1, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#ifndef _BFW_LSSOLUTION_H_
#define _BFW_LSSOLUTION_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "BFW.h"
#include "BFW_Motoko.h"
#include "IMP_Env2_Private.h"


//#define LScMaxPoints	(1024 * 128)
//#define LScMaxFaces		(LScMaxPoints * 2)
#define LScMaxDataSize	(50 * 1024 * 1024)

#define LScPrepVersion			"1"
#define LScPrepVersion_Length	(8)

#define LScPrepEndToken			"et"

enum
{
	LScFlag_None		= 0,
	LScFlag_Daylight	= (1 << 0),
	LScFlag_Exterior	= (1 << 1),
	LScFlag_RGB			= (1 << 2),
	LScFlag_Irradiance	= (1 << 3)

};

typedef struct LStVertex
{
	UUtUns32	pointIndex;
	float		r, g, b;
	
} LStVertex;

typedef struct LStFace
{
	UUtUns32		isQuad;
	LStVertex		vertices[4];

} LStFace;

typedef struct LStPatch
{
	UUtUns32	faceStartIndex;
	UUtUns32	faceEndIndex;

	M3tBoundingBox_MinMax bbox;
	M3tPlaneEquation plane_equation;
} LStPatch;

typedef struct LStPoint
{
	M3tPoint3D	location;
	M3tVector3D normal;
	UUtUns32	shade;
	UUtUns32	used;
} LStPoint;


typedef struct LStData
{
	UUtUns32		num_points;
	LStPoint		*the_point_list;

	M3tBoundingBox_MinMax bbox;
	
	float			contrast;
	float			brightness;
	
	char			prepVersion[LScPrepVersion_Length];
	
	UUtUns32		flags;
	
} LStData;

UUtError
LSrData_WriteBinary(
	BFtFileRef*		inFileRef,
	LStData*		inData);

UUtError
LSrData_ReadBinary(
	BFtFileRef*		inFileRef,
	LStData*		*outNewData);

UUtUns32
LSrData_GetSize(
	LStData*		inData);

#if UUmPlatform == UUmPlatform_Win32

	UUtError
	LSrData_CreateFromLSFile(
		const char*		inLSFilePath,
		LStData*	*outNewData);
	
	UUtError
	LSrPreperationFile_Create(
		struct IMPtEnv_BuildData*	inBuildData,
		const char*						inGQFileName,
		const char*						inLPFilePath);
		
#endif

void
LSrData_Delete(
	LStData*	inData);


#ifdef __cplusplus
}
#endif

#endif /* _BFW_LSSOLUTION_H_ */
