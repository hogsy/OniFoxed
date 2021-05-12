#pragma once
/*
	FILE:	BFW_DebuggerSymbols.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: July 12, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/
#include "BFW.h"

#define DScNameBufferSize				(256)

#define DScSourceSymDB_Version				"ssdb version 0.01"

#define	DScSourceSymDB_VersionStrLength		(32)

typedef enum DStSourceSymDB_Platform
{
	DScSourceSymDB_Platform_MacOS,
	DScSourceSymDB_Platform_Win32,
	
	DScPlatform_Num
	
} DStSourceSymDB_Platform;

typedef struct DStSourceSymDB_BinOffset2SourceFileMap
{
	UUtUns32	owningResourceIndex;
	UUtUns32	owningFunctionIndex;
	UUtUns32	owningSourceFileIndex;
	UUtUns32	owningScope;
	
	UUtUns32	binOffsetLow;			// resource relative
	UUtUns32	binOffsetHigh;			// resource relative
	
	UUtUns32	sourceFileLineLow;
	UUtUns32	sourceFileLineHigh;
	
} DStSourceSymDB_BinOffset2SourceFileMap;

typedef struct DStSourceSymDB_SourceFile
{
	char*		sourceFileLeafName;
	UUtUns32	modDate;
	
	UUtUns32	functionIndexLow;
	UUtUns32	functionIndexHigh;
	
} DStSourceSymDB_SourceFile;

typedef struct DStSourceSymDB_Scope
{
	UUtUns32	scopingDepth;
	
	UUtUns32	mappingIndexLow;	// indexes into resource mappings
	UUtUns32	mappingIndexHigh;	// indexes into resource mappings
	
	UUtUns32	parentScopeIndex;
	UUtUns32	parentFunctionIndex;
	UUtUns32	parentSourceFileIndex;
	
} DStSourceSymDB_Scope;

typedef struct DStSourceSymDB_Function
{
	char*		functionName;
	
	UUtUns32	owningResourceIndex;
	UUtUns32	owningSourceFileIndex;
	
	UUtUns32	mappingIndexLow;	// indexes into resource mappings
	UUtUns32	mappingIndexHigh;	// indexes into resource mappings
	
	UUtUns32	scopeIndexLow;
	UUtUns32	scopeIndexHigh;
	
} DStSourceSymDB_Function;

typedef struct DStSourceSymDB_Resource
{
	UUtUns32	mappingIndexLow;
	UUtUns32	mappingIndexHigh;
	
} DStSourceSymDB_Resource;

typedef struct DStSourceSymDB_Header
{
	char						version[DScSourceSymDB_VersionStrLength];
	char*						executableLeafName;
	DStSourceSymDB_Platform		platform;
	
	UUtUns32					rootFunctionIndex;
	
	UUtUns32					numResources;
	DStSourceSymDB_Resource*	resources;
	
	UUtUns32					numSourceFiles;
	DStSourceSymDB_SourceFile*	sourceFiles;
	
	UUtUns32					numFunctions;
	DStSourceSymDB_Function*	functions;
	
	UUtUns32					numScopes;
	DStSourceSymDB_Scope*		scope;
	
	UUtUns32					nameLength;
	char*						names;
	
	UUtUns32								numMappings;
	DStSourceSymDB_BinOffset2SourceFileMap*	mappings;

} DStSourceSymDB_Header;


UUtError
DSrProgramCounter_GetFileAndLine(
	UUtUns32	inProgramCounter,
	char		*outFileBuffer,
	char		*outFunctionNameBuffer,
	UUtUns32	*outLine);

UUtError
DSrInitialize(
	void);

void
DSrTerminate(
	void);
