/*
	FILE:	BFW_ScriptLang.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_AppUtilities.h"
#include "BFW_Console.h"

#include "BFW_ScriptLang.h"
#include "BFW_ScriptLang_Private.h"
#include "BFW_ScriptLang_Token.h"
#include "BFW_ScriptLang_Context.h"
#include "BFW_ScriptLang_Parse.h"
#include "BFW_ScriptLang_Eval.h"
#include "BFW_ScriptLang_Database.h"
#include "BFW_ScriptLang_Error.h"
#include "BFW_ScriptLang_Scheduler.h"

#define SLcLexem_TableSize		(32 * 1024)
#define SLcDatabase_HeapSize	(1 * 1024 * 1024)
#define SLcPermanent_PoolSize	(50 * 1024)

UUtMemory_String*	SLgLexemMemory = NULL;
UUtMemory_String*	SLgPermStringMemory = NULL;
UUtMemory_Heap*		SLgDatabaseHeap = NULL;

UUtMemory_Pool*		SLgPermanentMem = NULL;

BFtDebugFile*		SLgDebugFile = NULL;

const char *SLcTypeName[SLcType_Max] = {"integer", "string", "float", "bool", "void"};

UUtError 
SLrScript_CommandTable_Register_Void(SLtRegisterVoidFunctionTable *inTable)
{
	SLtRegisterVoidFunctionTable *current_entry;

	for(current_entry = inTable; current_entry->name != NULL; current_entry++)
	{
		UUtError error;

		UUmAssert(NULL != current_entry->name);
		UUmAssert(NULL != current_entry->description);
		UUmAssert(NULL != current_entry->function);

		error = SLrScript_Command_Register_Void(current_entry->name, current_entry->description, current_entry->parameters, current_entry->function);
		UUmError_ReturnOnErrorMsg(error, "failed to register command");
	}

	return UUcError_None;
}

UUtError SLrGlobalVariable_Register_Int32_Table(SLtRegisterInt32Table *inTable)
{
	SLtRegisterInt32Table *current_entry;

	for(current_entry = inTable; current_entry->name != NULL; current_entry++)
	{
		UUtError error;

		UUmAssert(NULL != current_entry->name);
		UUmAssert(NULL != current_entry->description);
		UUmAssert(NULL != current_entry->variableAddress);

		error = SLrGlobalVariable_Register_Int32(current_entry->name, current_entry->description, current_entry->variableAddress);
		UUmError_ReturnOnErrorMsg(error, "failed to register variable");
	}

	return UUcError_None;
}

UUtError SLrGlobalVariable_Register_Float_Table(SLtRegisterFloatTable *inTable)
{
	SLtRegisterFloatTable *current_entry;

	for(current_entry = inTable; current_entry->name != NULL; current_entry++)
	{
		UUtError error;

		UUmAssert(NULL != current_entry->name);
		UUmAssert(NULL != current_entry->description);
		UUmAssert(NULL != current_entry->variableAddress);

		error = SLrGlobalVariable_Register_Float(current_entry->name, current_entry->description, current_entry->variableAddress);
		UUmError_ReturnOnErrorMsg(error, "failed to register variable");
	}

	return UUcError_None;
}

UUtError SLrGlobalVariable_Register_Bool_Table(SLtRegisterBoolTable *inTable)
{
	SLtRegisterBoolTable *current_entry;

	for(current_entry = inTable; current_entry->name != NULL; current_entry++)
	{
		UUtError error;

		UUmAssert(NULL != current_entry->name);
		UUmAssert(NULL != current_entry->description);
		UUmAssert(NULL != current_entry->variableAddress);

		error = SLrGlobalVariable_Register_Bool(current_entry->name, current_entry->description, current_entry->variableAddress);
		UUmError_ReturnOnErrorMsg(error, "failed to register variable");
	}

	return UUcError_None;
}

UUtError SLrGlobalVariable_Register_String_Table(SLtRegisterStringTable *inTable)
{
	SLtRegisterStringTable *current_entry;

	for(current_entry = inTable; current_entry->name != NULL; current_entry++)
	{
		UUtError error;

		UUmAssert(NULL != current_entry->name);
		UUmAssert(NULL != current_entry->description);
		UUmAssert(NULL != current_entry->variableAddress);

		error = SLrGlobalVariable_Register_String(current_entry->name, current_entry->description, current_entry->variableAddress);
		UUmError_ReturnOnErrorMsg(error, "failed to register variable");
	}

	return UUcError_None;
}

UUtError
SLrScript_Command_Register_ReturnType(
	const char*				inCommandName,
	const char*				inDescription,
	const char*				inParameterSpecification,
	SLtType			inReturnType,
	SLtEngineCommand		inCommandFunc)
{
	return
		SLrScript_Database_FunctionEngine_Add(
			inCommandName,
			inDescription,
			inParameterSpecification,
			inReturnType,
			inCommandFunc);
}

UUtError
SLrScript_Command_Register_Void(
	const char*				inCommandName,
	const char*				inDescription,
	const char*				inParameterSpecification,
	SLtEngineCommand		inCommandFunc)
{
	return
		SLrScript_Database_FunctionEngine_Add(
			inCommandName,
			inDescription,
			inParameterSpecification,
			SLcType_Void,
			inCommandFunc);
}

UUtError
SLrScript_Iterator_Register(
	const char*					inIteratorName,
	SLtType				inVariableType,
	SLtScript_Iterator_GetFirst	inGetFirstFunc,
	SLtScript_Iterator_GetNext	inGetNextFunc)
{
	return SLrScript_Database_Iterator_Add(inIteratorName, inVariableType, inGetFirstFunc, inGetNextFunc);
}

UUtError	// error code can probably be ignore - i will decide later
SLrScript_Schedule(
	const char*			inName,
	UUtUns16			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	SLtParameter_Actual		*ioReturnValue,
	UUtUns32			inTimeDelta,		// if 0 script is executed immediately
	UUtUns32			inNumberOfTimes,	// if 0 script executes every game tick(not recommended)
	SLtContext **		ioReferencePtr)		// used for getting a reference to track currently-running scripts
{
	UUtError	error;
	SLtErrorContext				errorContext;
	
	errorContext.fileName = "(called from engine)";
	errorContext.funcName = UUrMemory_String_GetStr(SLgLexemMemory, inName);
	errorContext.line = 0;

	error = 
		SLrScheduler_Execute(
			inName,
			&errorContext,
			inParameterListLength,
			inParameterList,
			ioReturnValue,
			inTimeDelta,
			inNumberOfTimes,
			ioReferencePtr);
	if(error != UUcError_None) return error;
	
	return UUcError_None;	
}

UUtError	// error code can probably be ignore - i will decide later
SLrScript_ExecuteOnce(
	const char*				inName,
	UUtUns16				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	SLtParameter_Actual		*ioReturnValue,	
	SLtContext **			ioReferencePtr)	// used for getting a reference to track currently-running scripts
{
	UUtError	error;
	SLtErrorContext				errorContext;
	SLtParameter_Actual			local_buffer[SLcScript_MaxNumParams];

	UUrMemory_MoveFast(inParameterList, local_buffer, sizeof(SLtParameter_Actual) * inParameterListLength);
	
	errorContext.fileName = "(called from engine)";
	errorContext.funcName = UUrMemory_String_GetStr(SLgLexemMemory, inName);
	errorContext.line = 0;

	error = 
		SLrScheduler_Execute(
			inName,
			&errorContext,
			inParameterListLength,
			local_buffer,
			ioReturnValue,
			0,
			1,
			ioReferencePtr);
	if(error != UUcError_None) return error;
	
	return UUcError_None;	
}

void
SLrScript_Database_Reset(
	void)
{
	UUrMemory_String_Reset(SLgLexemMemory);
	SLrScript_Database_Internal_Reset();
}

UUtError
SLrScript_Database_Add(
	const char*	inFileName,
	const char*	inText)
{
	UUtError	error;
	UUtUns16	numTokens;
	SLtToken*	tokens;
	SLtToken*	curToken;
	SLtContext*	context;
	SLtErrorContext	errorContext;
	
	UUrMemory_String_GetStr(SLgLexemMemory, inFileName);
	errorContext.fileName = UUrMemory_String_GetStr(SLgLexemMemory, inFileName);
	
	error = SLrScript_TextToToken(SLgLexemMemory, inFileName, inText, &numTokens, &tokens);
	UUmError_ReturnOnError(error);
	
	curToken = tokens;
	
	context = SLrContext_New(NULL);
	UUmError_ReturnOnNull(context);
	
	context->curFuncState = context->funcStack;
	context->curFuncState->parseTOS = 0;
	context->curFuncState->curToken = tokens;
	context->funcTOS = 1;
	
	error = SLrScript_Parse(context, &errorContext, SLcParseMode_AddToDatabase);
	if(error != UUcError_None) return error;
	
	SLrContext_Delete(context);
	
	return UUcError_None;
}

UUtError
SLrGlobalVariable_Register(					// use to register a variable that is to be exposed to scripts
	const char*			inName,
	const char*			inDescription,
	SLtReadWrite		inReadWrite,
	SLtType				inType,
	SLtValue*			inVariableAddress,
	SLtNotificationProc	inNotificationProc)
{
	UUtError		error;
	SLtErrorContext	errorContext;
	
	errorContext.fileName = "(called from engine)";
	errorContext.funcName = "(called from engine)";
	errorContext.line = 0;
	
	error = 
		SLrScript_Database_Var_Engine_Add(
			inName,
			inDescription,
			&errorContext,
			inReadWrite,
			inType,
			inVariableAddress,
			inNotificationProc);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

UUtError
SLrGlobalVariable_Register_Int32(
	const char*	inName,
	const char*	inDescription,
	UUtInt32*	inVariableAddress)
{
	return SLrGlobalVariable_Register(
				inName,
				inDescription,
				SLcReadWrite_ReadWrite,
				SLcType_Int32,
				(SLtValue*)inVariableAddress,
				NULL);
}

UUtError
SLrGlobalVariable_Register_Float(
	const char*	inName,
	const char*	inDescription,
	float*		inVariableAddress)
{
	return SLrGlobalVariable_Register(
				inName,
				inDescription,
				SLcReadWrite_ReadWrite,
				SLcType_Float,
				(SLtValue*)inVariableAddress,
				NULL);
}

UUtError
SLrGlobalVariable_Register_Bool(
	const char*	inName,
	const char*	inDescription,
	UUtBool*	inVariableAddress)
{
	return SLrGlobalVariable_Register(
				inName,
				inDescription,
				SLcReadWrite_ReadWrite,
				SLcType_Bool,
				(SLtValue*)inVariableAddress,
				NULL);
}

UUtError
SLrGlobalVariable_Register_String(
	const char*	inName,
	const char*	inDescription,
	char*		inVariableAddress)
{
	return SLrGlobalVariable_Register(
				inName,
				inDescription,
				SLcReadWrite_ReadWrite,
				SLcType_String,
				(SLtValue*)inVariableAddress,
				NULL);
}

UUtError
SLrGlobalVariable_GetValue(					// use to get a value from a global var declared in a script
	const char*	inName,
	SLtType		*outType,
	SLtValue	*outValue)
{

	return UUcError_None;
}

UUtError
SLrScript_Update(
	UUtUns32	inGameTime)
{
	SLrScheduler_Update(inGameTime);
	
	return UUcError_None;
}

static UUtBool
SLiScript_ConsoleHook(
	const char*	inPrefix,
	UUtUns32	inRefCon,
	const char*	inCommandLine)
{
	UUtError		error;
	SLtErrorContext	errorContext;
	UUtUns16		numTokens;
	SLtToken*		tokens;
	SLtContext*		context;
	
	errorContext.fileName = "(called from console)";
	errorContext.funcName = "(called from console)";
	errorContext.line = 0;

	error =
		SLrScript_TextToToken(
			SLgLexemMemory,
			errorContext.fileName,
			inCommandLine,
			&numTokens,
			&tokens);
	if(error != UUcError_None) return UUcFalse;
	
	if(numTokens == 0) return UUcTrue;
	
	context = SLrContext_New(NULL);
	if(context == NULL) return UUcFalse;
	
	error = SLrParse_FuncStack_Push_Console(context, &errorContext, tokens);
	if(error != UUcError_None) goto done;
	
	error = 
		SLrScript_Parse(
			context,
			&errorContext,
			SLcParseMode_InitialExecution);
	
	switch(context->returnType)
	{
		case SLcType_Int32:
			COrConsole_Printf("int32: %d", context->returnValue.i);
			break;
			
		case SLcType_Bool:
			COrConsole_Printf("bool: %d", context->returnValue.b);
			break;
			
		case SLcType_Float:
			COrConsole_Printf("float: %f", context->returnValue.f);
			break;
			
		case SLcType_String:
			COrConsole_Printf("string: %s", context->returnValue.str);
			break;
	}
	
	if ((context->ticksTillCompletion == 0) && (!context->stalled)) {
		SLrContext_Delete(context);
	}

done:
	
	return error == UUcError_None ? UUcTrue : UUcFalse;	
}

UUtError
SLrScript_Initialize(
	void)
{
	UUtError	error;
	
	SLgLexemMemory = UUrMemory_String_New(SLcLexem_TableSize);
	UUmError_ReturnOnNull(SLgLexemMemory);
	
	SLgPermStringMemory = UUrMemory_String_New(SLcLexem_TableSize);
	UUmError_ReturnOnNull(SLgPermStringMemory);
	
	SLgDatabaseHeap = UUrMemory_Heap_New(SLcDatabase_HeapSize, UUcFalse);
	UUmError_ReturnOnNull(SLgDatabaseHeap);
	
	SLgPermanentMem = UUrMemory_Pool_New(SLcPermanent_PoolSize, UUcFalse);
	UUmError_ReturnOnNull(SLgPermanentMem);
	
	error = SLrScheduler_Initialize();
	UUmError_ReturnOnError(error);
	
	error = 
		SLrScript_Command_Register_Void(
			"dump_docs",
			"Shows all registered variables and commands",
			"",
			SLrDatabase_Command_DumpAll);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new command.");
	
	error = COrConsole_AddHook("", 0, SLiScript_ConsoleHook);
	UUmError_ReturnOnError(error);
	
	SLgDebugFile = BFrDebugFile_Open("script_debug.txt");
	UUmAssert(SLgDebugFile);
	
	return UUcError_None;
}

void
SLrScript_Terminate(
	void)
{
	SLrScheduler_Terminate();
	
	UUrMemory_String_Delete(SLgPermStringMemory);
	UUrMemory_String_Delete(SLgLexemMemory);
	UUrMemory_Heap_Delete(SLgDatabaseHeap);
	UUrMemory_Pool_Delete(SLgPermanentMem);
	
	BFrDebugFile_Close(SLgDebugFile);

	return;
}

void
SLrScript_ConsoleCompletionList_Get(
	UUtUns32		*outNumNames,
	char			***outList)
{
	SLrScript_Database_ConsoleCompletionList_Get(outNumNames, outList);

	return;
}

void SLrScript_LevelBegin(
	void)
{
	SLrScheduler_Initialize();
}

void SLrScript_LevelEnd(
	void)
{
}
