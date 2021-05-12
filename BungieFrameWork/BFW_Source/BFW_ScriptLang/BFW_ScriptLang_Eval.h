#pragma once
/*
	FILE:	BFW_ScriptLang_Eval.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/
#ifndef BFW_SCRIPTLANG_EVAL
#define BFW_SCRIPTLANG_EVAL

UUtBool
SLrExpr_GetResult(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtType		*outType,
	SLtValue	*outVal);

SLtExpr*
SLrExpr_Pop(
	SLtContext*	inContext);

void
SLrExpr_PopRemove(
	SLtContext*	inContext);

void
SLrExpr_Push_Const(
	SLtContext*			inContext,
	SLtType		inType,
	SLtValue	inVal);

void
SLrExpr_Push_Ident(
	SLtContext*			inContext,
	const char*			inName);

UUtError
SLrExpr_Eval(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtExpr*			inExpr,
	SLtType		*outType,
	SLtValue	*outVal);

UUtBool
SLrStatement_Evaluate(
	SLtContext*	inContext);

void
SLrStatement_Level_Push(
	SLtContext*	inContext,
	UUtBool		inEval);
	
void
SLrStatement_Level_Pop(
	SLtContext*	inContext);

void
SLrStatement_Level_Invert(
	SLtContext*	inContext);
	
UUtError
SLrExprListToParamList(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtParameter_Actual		*outParamList);

UUtBool
SLrExpr_CompareValues_Eq(
	SLtType		inType,
	SLtValue	inValA,
	SLtValue	inValB);

void
SLrExpr_MakeFromConstant(
	SLtExpr*	ioExpr,
	SLtToken*	inToken);

void
SLrExpr_MakeStringConstant(
	SLtExpr*	ioExpr,
	const char*	inString);

UUtInt32
SLrExpr_ValAndType_To_Int(
	SLtType		inType,
	SLtValue	inVal);
	

#endif /* BFW_SCRIPTLANG_EVAL */
