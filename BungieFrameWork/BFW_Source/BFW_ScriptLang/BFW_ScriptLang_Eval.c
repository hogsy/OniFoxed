/*
	FILE:	BFW_ScriptLang_Eval.c

	AUTHOR:	Brent H. Pease

	CREATED: Oct 29, 1999

	PURPOSE:

	Copyright 1999

*/

#include "BFW.h"
#include "BFW_TemplateManager.h"

#include "BFW_ScriptLang.h"
#include "BFW_ScriptLang_Private.h"
#include "BFW_ScriptLang_Token.h"
#include "BFW_ScriptLang_Context.h"
#include "BFW_ScriptLang_Parse.h"
#include "BFW_ScriptLang_Database.h"
#include "BFW_ScriptLang_Eval.h"
#include "BFW_ScriptLang_Error.h"
#include "BFW_Console.h"

SLtExpr* SLrExpr_Pop(SLtContext *inContext)
{
	SLtExpr *result;
	UUmAssert(inContext->curFuncState->exprTOS > 0);

	if (inContext->curFuncState->exprTOS > 0) {
		inContext->curFuncState->exprTOS--;

		result = inContext->curFuncState->exprStack + inContext->curFuncState->exprTOS;
	}
	else {
		COrConsole_Printf("failed to pop expression stack");
		result = NULL;
	}

	return result;
}

void
SLrExpr_PopRemove(
	SLtContext*	inContext)
{
	if(inContext->curFuncState->exprTOS == 0) return;

	inContext->curFuncState->exprTOS--;
}

void
SLrExpr_MakeFromConstant(
	SLtExpr*	ioExpr,
	SLtToken*	inToken)
{
	ioExpr->kind = SLcExprKind_Constant;
	SLrParse_ConstTokenToTypeAndVal(
		inToken,
		&ioExpr->type,
		&ioExpr->u.constant.val);
}

void
SLrExpr_MakeStringConstant(
	SLtExpr*	ioExpr,
	const char*	inString)
{
	ioExpr->kind = SLcExprKind_Constant;
	ioExpr->type = SLcType_String;
	ioExpr->u.constant.val.str = (char*)inString;
}

static void SLrExpr_MakeRoom(SLtContext *inContext)
{
	if (SLcContext_ExprStack_MaxDepth == inContext->curFuncState->exprTOS) {
		UUrMemory_MoveOverlap(
			inContext->curFuncState->exprStack + 1,
			inContext->curFuncState->exprStack + 0,
			sizeof(SLtExpr) * (SLcContext_ExprStack_MaxDepth - 1));

		inContext->curFuncState->exprTOS--;
	}

	return;
}

void
SLrExpr_Push_Const(
	SLtContext*			inContext,
	SLtType		inType,
	SLtValue	inVal)
{
	SLrExpr_MakeRoom(inContext);

	UUmAssert(inContext->curFuncState->exprTOS < SLcContext_ExprStack_MaxDepth);


	inContext->curFuncState->exprStack[inContext->curFuncState->exprTOS].kind = SLcExprKind_Constant;
	inContext->curFuncState->exprStack[inContext->curFuncState->exprTOS].type = inType;
	inContext->curFuncState->exprStack[inContext->curFuncState->exprTOS].u.constant.val = inVal;

	inContext->curFuncState->exprTOS++;
}

void
SLrExpr_Push_Ident(
	SLtContext*			inContext,
	const char*			inName)
{
	SLrExpr_MakeRoom(inContext);

	UUmAssert(inContext->curFuncState->exprTOS < SLcContext_ExprStack_MaxDepth);

	inContext->curFuncState->exprStack[inContext->curFuncState->exprTOS].kind = SLcExprKind_Identifier;
	inContext->curFuncState->exprStack[inContext->curFuncState->exprTOS].type = (SLtType)-1;
	inContext->curFuncState->exprStack[inContext->curFuncState->exprTOS].u.ident.name = inName;

	inContext->curFuncState->exprTOS++;
}

UUtError
SLrExpr_Eval(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtExpr*			inExpr,
	SLtType		*outType,
	SLtValue	*outVal)
{
	UUtError	error;
	SLtSymbol*	symbol;

	if (inExpr == NULL) {
		return UUcError_Generic;
	}

	switch(inExpr->kind)
	{
		case SLcExprKind_Constant:
			*outType = inExpr->type;
			*outVal = inExpr->u.constant.val;
			break;

		case SLcExprKind_Identifier:
			error =
				SLrScript_Database_Symbol_Get(
					inContext,
					inExpr->u.ident.name,
					&symbol);
			if(error != UUcError_None)
			{
				*outType = SLcType_String;
				outVal->str = (char*)inExpr->u.ident.name;
			}
			else
			{
				error =
					SLrScript_Database_Var_GetValue(
						inErrorContext,
						inContext,
						inExpr->u.ident.name,
						outType,
						outVal);
				if(error != UUcError_None) return error;
			}
			break;

		default:
			UUmAssert(0);
	}

	return UUcError_None;
}

UUtBool
SLrStatement_Evaluate(
	SLtContext*	inContext)
{
	UUmAssert(inContext->curFuncState->statementLevel > 0);

	return inContext->curFuncState->statementEval[inContext->curFuncState->statementLevel - 1];
}

void
SLrStatement_Level_Push(
	SLtContext*	inContext,
	UUtBool		inEval)
{
	if(inContext->curFuncState->statementLevel > 0 && SLrStatement_Evaluate(inContext)) inContext->curFuncState->statement_TargetLevel = inContext->curFuncState->statementLevel + 1;

	inContext->curFuncState->statementEval[inContext->curFuncState->statementLevel++] = inEval;
}

void
SLrStatement_Level_Pop(
	SLtContext*	inContext)
{
	UUmAssert(inContext->curFuncState->statementLevel > 0);

	inContext->curFuncState->statementLevel--;
}

void
SLrStatement_Level_Invert(
	SLtContext*	inContext)
{
	if(inContext->curFuncState->statement_TargetLevel == inContext->curFuncState->statementLevel)
	{
		inContext->curFuncState->statementEval[inContext->curFuncState->statementLevel - 1] = !inContext->curFuncState->statementEval[inContext->curFuncState->statementLevel - 1];
	}
}

UUtBool
SLrExpr_GetResult(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtType		*outType,
	SLtValue	*outVal)
{
	UUtError	error;
	SLtExpr*	expr;

	if(inContext->curFuncState->exprTOS == 0) return UUcFalse;

	expr = SLrExpr_Pop(inContext);

	if (NULL == expr) {
		return UUcFalse;
	}

	error = SLrExpr_Eval(inContext, inErrorContext, expr, outType, outVal);

	if(error != UUcError_None) {
		return UUcFalse;
	}

	return UUcTrue;
}

UUtError
SLrExprListToParamList(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtParameter_Actual		*outParamList)
{
	UUtError			error;
	UUtUns16			itr;
	SLtType		type;
	SLtValue	val;
	SLtContext_FuncState*	curFuncState;

	curFuncState = inContext->curFuncState;

	// CB: this is done so that we don't trash over statically allocated fixed-length buffers
	curFuncState->numParams = UUmMin(curFuncState->numParams, SLcScript_MaxNumParams);

	for(itr = 0; itr < curFuncState->numParams; itr++)
	{
		error =
			SLrExpr_Eval(
				inContext,
				inErrorContext,
				curFuncState->paramExprs + itr,
				&type,
				&val);
		if(error != UUcError_None) return error;

		outParamList[itr].type = type;
		outParamList[itr].val = val;
	}

	return UUcError_None;
}

UUtBool
SLrExpr_CompareValues_Eq(
	SLtType		inType,
	SLtValue	inValA,
	SLtValue	inValB)
{
	switch(inType)
	{
		case SLcType_Int32:
			return inValA.i == inValB.i ? UUcTrue : UUcFalse;
			break;

		case SLcType_Float:
			return inValA.f == inValB.f ? UUcTrue : UUcFalse;
			break;

		case SLcType_String:
			return !strcmp(inValA.str, inValB.str) ? UUcTrue : UUcFalse;
			break;

		case SLcType_Bool:
			return inValA.b == inValB.b ? UUcTrue : UUcFalse;
			break;

	}

	UUmAssert(0);
	return UUcFalse;
}

UUtInt32
SLrExpr_ValAndType_To_Int(
	SLtType		inType,
	SLtValue	inVal)
{
	if(inType == SLcType_Int32)
	{
		return inVal.i;
	}
	else if(inType == SLcType_Bool)
	{
		return inVal.b;
	}

	UUmAssert(0);

	return 0;
}
