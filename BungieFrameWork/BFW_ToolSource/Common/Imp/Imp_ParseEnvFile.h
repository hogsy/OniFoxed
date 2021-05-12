/*
	FILE:	Imp_ParseEnvFile.h
	
	AUTHOR:	Brent H. Pease, Michael Evans
	
	CREATED: Nov 17, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#include "BFW.h"
#include "EnvFileFormat.h"

UUtError
Imp_ConvertENVFile(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_ParseEnvFile(
	BFtFileRef*			inFileRef,
	MXtHeader*			*outHeader);

void
Imp_EnvFile_Delete(
	MXtHeader*			inHeader);

UUtError
Imp_ParseEvaFile(
	BFtFileRef*			inFileRef,
	AXtHeader*			*outHeader);

void
Imp_EvaFile_Delete(
	AXtHeader*			inHeader);

MXtMarker *
Imp_EnvFile_GetMarker(
	MXtNode				*inNode,
	const char			*inMarkerName);

UUtError
Imp_WriteTextEnvFile(
	BFtFileRef*			inFileRef,
	const MXtHeader*	inHeader);