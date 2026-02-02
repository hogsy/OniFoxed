#pragma once

/*
	FILE:	BFW_Group.h

	AUTHOR:	Brent H. Pease

	CREATED: Dec 17, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef BFW_GROUP_H
#define BFW_GROUP_H

#include "BFW.h"
#include "BFW_FileManager.h"

#define GRcMaxNumArray	512

typedef struct	GRtGroup_Context	GRtGroup_Context;
typedef struct	GRtGroup			GRtGroup;
typedef struct	GRtElementArray		GRtElementArray;

typedef enum GRtElementType
{
	GRcElementType_String,
	GRcElementType_Group,
	GRcElementType_Array

} GRtElementType;

UUtError
GRrGroup_Context_New(
	GRtGroup_Context	**outGroupContext);

UUtError
GRrGroup_Context_NewFromFileRef(
	BFtFileRef*			inFileRef,
	GRtGroup*			inPreferenceGroup,
	GRtGroup_Context	*inContext,
	GRtGroup_Context*	*outGroupContext,
	GRtGroup*			*outGroup);

UUtError
GRrGroup_Context_NewFromPartialFile(
	BFtTextFile*		inFile,
	GRtGroup*			inPreferenceGroup,
	GRtGroup_Context	*inContext,
	UUtUns16			inLines,
	GRtGroup_Context*	*outGroupContext,
	GRtGroup*			*outGroup);

UUtError
GRrGroup_Context_NewFromString(
	const char*			inString,
	GRtGroup*			inPreferenceGroup,
	GRtGroup_Context	*inContext,
	GRtGroup_Context*	*outGroupContext,
	GRtGroup*			*outGroup);

void
GRrGroup_Context_Delete(
	GRtGroup_Context*	inGroupContext);

void
GRrGroup_SetRecursive(
	GRtGroup*	inGroup,
	GRtGroup*	inRecursive);

UUtError
GRrGroup_GetElement(
	GRtGroup*		inGroup,
	const char*		inVarName,
	GRtElementType	*outElementType,
	void*			*outDataPtr);		// Either a GRtGroup, GRtElementArray, or char*

UUtError
GRrGroup_GetString(
	GRtGroup*		inGroup,
	const char*		inVarName,
	char*			*outString);

UUtError
GRrGroup_GetFloat(
	GRtGroup*		inGroup,
	const char*		inVarName,
	float			*outNum);

UUtError
GRrGroup_GetBool(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtBool			*outBool);

UUtError
GRrGroup_GetInt8(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtInt8			*outNum);

UUtError
GRrGroup_GetInt16(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtInt16		*outNum);

UUtError
GRrGroup_GetInt32(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtInt32		*outNum);

UUtError
GRrGroup_GetUns8(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtUns8			*outNum);

UUtError
GRrGroup_GetUns16(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtUns16		*outNum);

UUtError
GRrGroup_GetUns32(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtUns32		*outNum);

UUtError
GRrGroup_GetOSType(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtUns32		*outOSType);

UUtUns32
GRrGroup_GetNumVariables(
	const GRtGroup*		inGroup);

const char *
GRrGroup_GetVarName(
	const GRtGroup*		inGroup,
	UUtUns32			inVarNumber);

UUtError
GRrGroup_Array_GetElement(
	GRtElementArray*	inElementArray,
	UUtUns32			inIndex,
	GRtElementType		*outElementType,
	void*				*outDataPtr);		// Either a GRtGroup, GRtElementArray, or char*

UUtUns32
GRrGroup_Array_GetLength(
	GRtElementArray*	inElementArray);


#endif
