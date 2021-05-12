#pragma once
/*
	FILE:	BFW_ScriptLang_Database.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/
#ifndef BFW_SCRIPTLANG_DATABASE_H
#define BFW_SCRIPTLANG_DATABASE_H

void
SLrScript_Database_Internal_Reset(
	void);

UUtError
SLrScript_Database_Symbol_Get(
	SLtContext*		inContext,
	const char*		inName,
	SLtSymbol*		*outSymbol);

UUtError
SLrScript_Database_FunctionScript_Add(
	const char*					inName,
	SLtErrorContext*			inErrorContext,
	SLtType						inReturnType,
	UUtUns16					inNumParams,
	SLtParameter_Formal*		inParamList,
	UUtUns32					inNumTokens,
	SLtToken*					inStartToken);

UUtError
SLrScript_Database_FunctionEngine_Add(
	const char*			inName,
	const char*			inDesc,
	const char*			inParameterSpecification,
	SLtType				inReturnType,
	SLtEngineCommand	inCommand);

UUtError
SLrScript_Database_Var_Add(
	SLtContext*			inContext,
	const char*			inName,
	SLtErrorContext*	inErrorContext,
	SLtType		inType,
	SLtValue	inValue);

UUtError
SLrScript_Database_Var_Engine_Add(
	const char*			inName,
	const char*			inDescription,
	SLtErrorContext*	inErrorContext,
	SLtReadWrite		inReadWrite,
	SLtType				inType,
	SLtValue*			inValueAddr,
	SLtNotificationProc	inNotificationProc);

UUtError
SLrScript_Database_Var_GetValue(
	SLtErrorContext*	inErrorContext,
	SLtContext*			inContext,
	const char*			inName,
	SLtType				*outType,
	SLtValue			*outValue);

UUtError
SLrScript_Database_Var_SetValue(
	SLtErrorContext*	inErrorContext,
	SLtContext*			inContext,
	const char*			inName,
	SLtValue			inValue);

UUtError
SLrScript_Database_Iterator_Add(
	const char*					inIteratorName,
	SLtType						inVariableType,
	SLtScript_Iterator_GetFirst	inGetFirstFunc,
	SLtScript_Iterator_GetNext	inGetNextFunc);


void
SLrScript_Database_Scope_Enter(
	SLtContext*			inContext);

void
SLrScript_Database_Scope_Leave(
	SLtContext*			inContext);

UUtError
SLrScript_ParameterCheckAndPromote(
	SLtErrorContext*		inErrorContext,
	const char*				inScriptName,
	SLtSymbol_Func_Script*	inScript,
	UUtUns16*				ioGivenParams_Num,
	SLtParameter_Actual*	ioGivenParams_List);

UUtError
SLrCommand_ParameterCheckAndPromote(
	SLtErrorContext*		inErrorContext,
	const char*				inCommandName,
	SLtSymbol_Func_Command*	inCommand,
	UUtUns16*				ioGivenParams_Num,
	SLtParameter_Actual*	ioGivenParams_List);

void
SLrScript_Database_ConsoleCompletionList_Get(
	UUtUns32		*outNumNames,
	char			***outList);

UUtError
SLrDatabase_Command_DumpAll(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue);

UUtBool
SLrDatabase_IsFunctionCall(
	const char*	inName);

#endif /* BFW_SCRIPTLANG_DATABASE_H */
