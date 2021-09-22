/*
	FILE:	Oni_Script.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Nov 13, 1999
	
	PURPOSE:
	
	Copyright 1997, 1998

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_ScriptLang.h"
#include "BFW_Console.h"

#include "Oni_GameState.h"
#include "Oni_Character.h"
#include "Oni_Script.h"
#include "Oni_AI_Setup.h"
#include "Oni_Level.h"

#define ONcScript_StaticParameterSize		32 * 1024

char *ONgScript_LevelName;

typedef struct ONtScript_StaticParameter {
	UUtUns32	length;
	char		string[1];
} ONtScript_StaticParameter;

// static memory for storing script parameters
UUtBool ONgScript_StaticParameterBaseOverflowed;
UUtUns32 ONgScript_NextStaticParameterOffset;
char ONgScript_StaticParameterBase[ONcScript_StaticParameterSize];

static UUtError
ONiScript_LoadDirectory(
	BFtFileRef*	inDirectoryRef)
{
	UUtError			error;
	BFtFileIterator*	fileIterator;
	UUtUns32			length;
	char*				textFile;
	BFtFileRef			curFileRef;

	UUrMemory_Clear(&curFileRef, sizeof(curFileRef));
	
	error =
		BFrDirectory_FileIterator_New(
			inDirectoryRef,
			NULL,
			".bsl",
			&fileIterator);
	if(error != UUcError_None) goto abort;
	
	while(1)
	{
		error = BFrDirectory_FileIterator_Next(fileIterator, &curFileRef);
		if(error != UUcError_None) break;
		
		error = BFrFileRef_LoadIntoMemory(&curFileRef, &length, &textFile);
		if(error != UUcError_None) goto abort;
		
		error =	SLrScript_Database_Add(BFrFileRef_GetLeafName(&curFileRef), textFile);
		if(error != UUcError_None) goto abort;
				
		UUrMemory_Block_Delete(textFile);
	}
	
	BFrDirectory_FileIterator_Delete(fileIterator);
	
	return UUcError_None;

abort:
	COrConsole_Printf("### error loading script file \"%s\"", BFrFileRef_GetLeafName(&curFileRef));

	return error;
}

static UUtError
ONiScript_Reload(
	SLtErrorContext			*inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError	error;
	BFtFileRef	levelRef;
	char		buffer[256];
	
	SLrScript_Database_Reset();

	levelRef = TMgDataFolderRef;

	// find current level directory
	sprintf(buffer, "IGMD%c%s", BFcPathSeparator, ONgScript_LevelName);

	error = BFrFileRef_AppendName(&levelRef, buffer);
	if(error != UUcError_None)
	{
		COrConsole_Printf("Could not find \"%s\" IGMD directory", ONgScript_LevelName);
		return UUcError_None;
	}
	
	// load all script files
	error = ONiScript_LoadDirectory(&levelRef);
	if(error != UUcError_None)
	{
		COrConsole_Printf("Could not load \"%s\" IGMD directory", ONgScript_LevelName);
		return UUcError_None;
	}
	
	return UUcError_None;
}

UUtError
ONrScript_Initialize(
	void)
{
	UUtError	error;
	
	error =
		SLrScript_Command_Register_Void(
			"script_reload",
			"reload scripts for a level",
			"",
			ONiScript_Reload);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

void
ONrScript_Terminate(
	void)
{


}

UUtError
ONrScript_LevelBegin(
	char*	inLevelName)
{
	SLrScript_LevelBegin();

	ONgScript_LevelName = inLevelName;
	
	ONgScript_StaticParameterBaseOverflowed = UUcFalse;
	ONgScript_NextStaticParameterOffset = 0;
	UUrMemory_Clear(&ONgScript_StaticParameterBase, ONcScript_StaticParameterSize);

	ONiScript_Reload(NULL, 0, NULL, NULL, NULL, NULL);

	return UUcError_None;
}

void
ONrScript_LevelEnd(
	void)
{
	SLrScript_LevelEnd();
}

static char *
ONiScript_StaticParameter(
	const char *inParameter)
{
	UUtUns32 length = strlen(inParameter);
	ONtScript_StaticParameter *parameter;

	// string must have room for trailing terminator, and be a whole number of words long
	length = (length + 1 + 0x03) & ~0x03;
	if (ONgScript_NextStaticParameterOffset + sizeof(UUtUns32) + length > ONcScript_StaticParameterSize) {
		// there is not enough room in the static array
		return NULL;
	}

	// copy the parameter into the array
	parameter = (ONtScript_StaticParameter *) (ONgScript_StaticParameterBase + ONgScript_NextStaticParameterOffset);
	parameter->length = length;
	UUrString_Copy(parameter->string, inParameter, parameter->length);

	// set up for the next parameter
	ONgScript_NextStaticParameterOffset += sizeof(UUtUns32) + parameter->length;
	return parameter->string;
}

// CB: this routine is used to ensure that we are passing in a static pointer for the
// string parameter to the script system. this is important because the script system
// may take the pointer that we give it and store it for an arbitrary length of time.
//
// the best solution would be for the scripting system internals to handle allocation
// and deletion of dynamic string parameters but we don't have time to do this.
// instead we will have to hope that we never exceed 1024 unique parameters passed into
// the scripting system per level.              -- friday 13th october, 2000
UUtError
ONrScript_ExecuteSimple(
	const char *inScriptName,
	const char *inParameterName)
{
	SLtParameter_Actual script_param;

	script_param.type = SLcType_String;
	script_param.val.str = ONiScript_StaticParameter(inParameterName);

	if (script_param.val.str == NULL) {
		if (!ONgScript_StaticParameterBaseOverflowed) {
			COrConsole_Printf("FATAL SCRIPTING ERROR: exceeded maximum scripting parameter base size of %d", ONcScript_StaticParameterSize);
			ONgScript_StaticParameterBaseOverflowed = UUcTrue;
		}
		return UUcError_Generic;
	}

	SLrScript_ExecuteOnce(inScriptName, 1, &script_param, NULL, NULL);
	return UUcError_None;
}

UUtError
ONrScript_Schedule(
	const char *inScriptName,
	const char *inParameterName,
	UUtUns32 inDelayTime,
	UUtUns32 inRepeatCount)
{
	SLtParameter_Actual script_param;

	script_param.type = SLcType_String;
	script_param.val.str = ONiScript_StaticParameter(inParameterName);

	if (script_param.val.str == NULL) {
		if (!ONgScript_StaticParameterBaseOverflowed) {
			COrConsole_Printf("FATAL SCRIPTING ERROR: exceeded maximum scripting parameter base size of %d", ONcScript_StaticParameterSize);
			ONgScript_StaticParameterBaseOverflowed = UUcTrue;
		}
		return UUcError_Generic;
	}

	SLrScript_Schedule(inScriptName, 1, &script_param, NULL, inDelayTime, inRepeatCount, NULL);
	return UUcError_None;
}
