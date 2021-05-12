#pragma once
/*
	FILE:	BFW_ScriptLang_Parse.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/
#ifndef BFW_SCRIPTLANG_PARSE_H
#define BFW_SCRIPTLANG_PARSE_H


typedef enum SLtParse_Mode
{
	SLcParseMode_AddToDatabase,
	SLcParseMode_InitialExecution,
	SLcParseMode_ContinueExecution
	
} SLtParse_Mode;

UUtError
SLrParse_FuncStack_Push(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	UUtUns16			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	const char*			inFuncName);

UUtError
SLrScript_Parse(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtParse_Mode		inMode);

UUtError
SLrParse_TokenToType(
	SLtToken_Code	inTokenCode,
	SLtType	*outVariableType);
	
UUtError
SLrParse_ConstTokenToTypeAndVal(
	SLtToken*			inToken,
	SLtType		*outType,
	SLtValue	*outVal);

UUtError
SLrParse_FuncStack_Push_Console(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken);

UUtBool
SLrScript_Parse_TokenStarts_Operator_Unary(
	SLtToken*	inCurToken);
	
UUtBool
SLrScript_Parse_TokenStarts_Operator_Binary(
	SLtToken*	inCurToken);

UUtBool
SLrScript_Parse_TokenStarts_Constant(
	SLtToken*	inCurToken);
	
#endif