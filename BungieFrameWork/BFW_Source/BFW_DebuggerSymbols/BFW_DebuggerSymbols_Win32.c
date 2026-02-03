/*
	FILE:	BFW_DebuggerSymbols_Win32.c

	AUTHOR:	Brent H. Pease

	CREATED: July 12, 1999

	PURPOSE:

	Copyright 1999

*/

#include "BFW.h"
#include "BFW_DebuggerSymbols.h"

UUtError
DSrInitialize(
	void)
{
	return UUcError_None;
}

void
DSrTerminate(
	void)
{
}

UUtError
DSrProgramCounter_GetFileAndLine(
	UUtUns32	inProgramCounter,
	char		*outFile,
	char		*outFunctionName,
	UUtUns32	*outLine)
{

	return UUcError_Generic;
}
