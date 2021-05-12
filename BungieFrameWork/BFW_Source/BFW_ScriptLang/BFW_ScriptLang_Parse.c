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
#include "BFW_ScriptLang_Error.h"
#include "BFW_ScriptLang_Eval.h"
#include "BFW_ScriptLang_Expr.h"
#include "BFW_ScriptLang_Database.h"
#include "BFW_ScriptLang_Scheduler.h"

enum
{
	SLcParseState_Global,
	SLcParseState_DataDef,
	SLcParseState_DataDef2,
	SLcParseState_DataDef3,
	SLcParseState_TypeSpec,
	SLcParseState_FuncDef,
	SLcParseState_FuncDef2,
	SLcParseState_FuncDef3,
	SLcParseState_FuncDef4,
	SLcParseState_FuncDef_FormalParamList,
	SLcParseState_FuncDef_FormalParamList2,
	SLcParseState_FuncDef_FormalParamList_Finished,
	SLcParseState_Statements,
	SLcParseState_Statements2,
	SLcParseState_Statement,
	SLcParseState_Statement_Iterating,
	SLcParseState_Statement_If_ReturnFromExpr,
	SLcParseState_Statement_If_ReturnFromExpr2,
	SLcParseState_Statement_If_ReturnFromExpr3,
	SLcParseState_Statement_Return_ReturnFromExpr,
	SLcParseState_Statement_Sleep_ReturnFromExpr,
	SLcParseState_Statement_ReturnFromCurley,
	SLcParseState_Statement_Expr_ReturnFromExpr,
	SLcParseState_Statement_Schdl_ReturnFromFuncCall,
	SLcParseState_Statement_Schdl_At,
	SLcParseState_Statement_Schdl_Repeat,
	SLcParseState_Statement_Schdl_Repeat2,
	SLcParseState_Expression,
	SLcParseState_Expression2,
	SLcParseState_Expression3,
	SLcParseState_Expression4,
	SLcParseState_Expression_Finish,
	SLcParseState_Expression_FunctionCall,
	SLcParseState_Expression_FunctionCall_Finish,
	SLcParseState_Expression_FunctionCall_ParamList_Old,
	SLcParseState_Expression_FunctionCall_ParamList_New,
	SLcParseState_Expression_FunctionCall_ParamList_New2,
	SLcParseState_DefaultReturn,
	SLcParseState_Statement_Fork_ReturnFromFuncCall
};

static const char *SLiParseStateToString(UUtUns8 inParseState)
{
	const char *result;

	typedef struct parse_state_to_string_table
	{
		UUtUns8 parse_state;
		const char *string;
	} parse_state_to_string_table;

	parse_state_to_string_table table[] = 
	{
		{ SLcParseState_Global, "SLcParseState_Global" },
		{ SLcParseState_DataDef, "SLcParseState_DataDef" },
		{ SLcParseState_DataDef2, "SLcParseState_DataDef2" },
		{ SLcParseState_DataDef3, "SLcParseState_DataDef3" },
		{ SLcParseState_TypeSpec, "SLcParseState_TypeSpec" },
		{ SLcParseState_FuncDef, "SLcParseState_FuncDef" },
		{ SLcParseState_FuncDef2, "SLcParseState_FuncDef2" },
		{ SLcParseState_FuncDef3, "SLcParseState_FuncDef3" },
		{ SLcParseState_FuncDef4, "SLcParseState_FuncDef4" },
		{ SLcParseState_FuncDef_FormalParamList, "SLcParseState_FuncDef_FormalParamList" },
		{ SLcParseState_FuncDef_FormalParamList2, "SLcParseState_FuncDef_FormalParamList2" },
		{ SLcParseState_FuncDef_FormalParamList_Finished, "SLcParseState_FuncDef_FormalParamList_Finished" },
		{ SLcParseState_Statements, "SLcParseState_Statements" },
		{ SLcParseState_Statements2, "SLcParseState_Statements2" },
		{ SLcParseState_Statement, "SLcParseState_Statement" },
		{ SLcParseState_Statement_Iterating, "SLcParseState_Statement_Iterating" },
		{ SLcParseState_Statement_If_ReturnFromExpr, "SLcParseState_Statement_If_ReturnFromExpr" },
		{ SLcParseState_Statement_If_ReturnFromExpr2, "SLcParseState_Statement_If_ReturnFromExpr2" },
		{ SLcParseState_Statement_If_ReturnFromExpr3, "SLcParseState_Statement_If_ReturnFromExpr3" },
		{ SLcParseState_Statement_Return_ReturnFromExpr, "SLcParseState_Statement_Return_ReturnFromExpr" },
		{ SLcParseState_Statement_Sleep_ReturnFromExpr, "SLcParseState_Statement_Sleep_ReturnFromExpr" },
		{ SLcParseState_Statement_ReturnFromCurley, "SLcParseState_Statement_ReturnFromCurley" },
		{ SLcParseState_Statement_Expr_ReturnFromExpr, "SLcParseState_Statement_Expr_ReturnFromExpr" },
		{ SLcParseState_Statement_Schdl_ReturnFromFuncCall, "SLcParseState_Statement_Schdl_ReturnFromFuncCall" },
		{ SLcParseState_Statement_Schdl_At, "SLcParseState_Statement_Schdl_At" },
		{ SLcParseState_Statement_Schdl_Repeat, "SLcParseState_Statement_Schdl_Repeat" },
		{ SLcParseState_Statement_Schdl_Repeat2, "SLcParseState_Statement_Schdl_Repeat2" },
		{ SLcParseState_Expression, "SLcParseState_Expression" },
		{ SLcParseState_Expression2, "SLcParseState_Expression2" },
		{ SLcParseState_Expression3, "SLcParseState_Expression3" },
		{ SLcParseState_Expression4, "SLcParseState_Expression4" },
		{ SLcParseState_Expression_Finish, "SLcParseState_Expression_Finish" },
		{ SLcParseState_Expression_FunctionCall, "SLcParseState_Expression_FunctionCall" },
		{ SLcParseState_Expression_FunctionCall_Finish, "SLcParseState_Expression_FunctionCall_Finish" },
		{ SLcParseState_Expression_FunctionCall_ParamList_Old, "SLcParseState_Expression_FunctionCall_ParamList_Old" },
		{ SLcParseState_Expression_FunctionCall_ParamList_New, "SLcParseState_Expression_FunctionCall_ParamList_New" },
		{ SLcParseState_Expression_FunctionCall_ParamList_New2, "SLcParseState_Expression_FunctionCall_ParamList_New2" },
		{ SLcParseState_DefaultReturn, "SLcParseState_DefaultReturn" },
		{ SLcParseState_Statement_Fork_ReturnFromFuncCall, "SLcParseState_Statement_Fork_ReturnFromFuncCal" },
		{ 0, NULL }
	};

	if (inParseState < (sizeof(table) / sizeof(parse_state_to_string_table)))
	{
		UUmAssert(table[inParseState].parse_state == inParseState);

		result = table[inParseState].string;
	}

	return result;
}

char *SLiExprToString(SLtExpr *inExpr)
{
	static char buffer[512];
	char *kind_string;

	switch(inExpr->kind)
	{
		case SLcExprKind_Constant:
			kind_string = "constant";
		break;

		case SLcExprKind_Identifier:
			kind_string = "identifier";
		break;

		default:
			kind_string = "unknown kind";
		break;
	}

	if (SLcExprKind_Constant == inExpr->kind) {
		switch(inExpr->type)
		{
			case SLcType_Int32:
				sprintf(buffer, "constant int32 = %d", inExpr->u.constant.val.i);
			break;

			case SLcType_String:
				sprintf(buffer, "constant string = %s", inExpr->u.constant.val.str);
			break;

			case SLcType_Float:
				sprintf(buffer, "constant float = %f", inExpr->u.constant.val.f);
			break;

			case SLcType_Bool:
				sprintf(buffer, "constant bool = %d", inExpr->u.constant.val.b);
			break;

			case SLcType_Void:
				sprintf(buffer, "constant void");
			break;
		}
	}
	else if (SLcExprKind_Identifier == inExpr->kind) {
		sprintf(buffer, "identifier %s", inExpr->u.ident.name);
	}
	else {
		sprintf(buffer, "unknown");
	}

	return buffer;
}



static void
SLiContext_Dump_Info(SLtContext *inContext, SLtToken *curToken)
{
#if SUPPORT_DEBUG_FILES
	static BFtDebugFile *stream = NULL;
	UUtInt32 itr; 

	if (NULL == stream) { 
		stream = BFrDebugFile_Open("hard_core_script_debug.txt");
	}

	if (NULL != stream) {
		if (inContext->curFuncState->curToken != NULL) {
			BFrDebugFile_Printf(stream, "line #%d lexem %s\n", inContext->curFuncState->curToken->line, inContext->curFuncState->curToken->lexem);
		}

		BFrDebugFile_Printf(stream, "line #%d lexem %s\n", curToken->line, curToken->lexem);
		BFrDebugFile_Printf(stream, "expr stack %d\n", inContext->curFuncState->exprTOS);

		for(itr = inContext->curFuncState->exprTOS - 1; itr > 0; itr--)
		{
			BFrDebugFile_Printf(stream, "\t%s\n", SLiExprToString(inContext->curFuncState->exprStack + itr));
		}
				

		BFrDebugFile_Printf(stream, "parse stack %d\n", inContext->curFuncState->parseTOS);
		BFrDebugFile_Printf(stream, "-------------------------\n");

		for(itr = inContext->curFuncState->parseTOS - 1; itr > 0; itr--)
		{
			BFrDebugFile_Printf(stream, "\t%s\n", SLiParseStateToString(inContext->curFuncState->parseStack[itr]));
		}

		BFrDebugFile_Printf(stream, "\n\n");
	}
#endif

	return;

}


#if 0
static UUtError
SLiScript_Parse_Statement(
	SLtToken*					*ioCurToken,
	SLtParse_Block_Statement*	inParseBlock_Statement,
	SLtParse_Block_Expression*	inParseBlock_Expr);
	
static UUtError
SLiScript_Parse_Statements(
	SLtToken*					*ioCurToken,
	SLtParse_Block_Statement*	inParseBlock_Statement,
	SLtParse_Block_Expression*	inParseBlock_Expr);

static UUtError
SLiScript_Parse_Expression(
	SLtToken*					*ioCurToken,
	SLtParse_Block_Expression*	inParseBlock_Expr);

#endif

static UUtBool
SLiScript_Parse_TokenStarts_DataDefinition(
	SLtToken*				inCurToken)
{
	if(inCurToken->token == SLcToken_Var)
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

static UUtBool
SLiScript_Parse_TokenStarts_FuncDefinition(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_Func)
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

UUtBool
SLrScript_Parse_TokenStarts_Constant(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_Constant_Int ||
		inCurToken->token == SLcToken_Constant_Float ||
		inCurToken->token == SLcToken_Constant_Bool ||
		inCurToken->token == SLcToken_Constant_String
		)
	{
		return UUcTrue;
	}

	return UUcFalse;
}

static UUtBool
SLiScript_Parse_TokenStarts_TypeSpec(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_String ||
		inCurToken->token == SLcToken_Float ||
		inCurToken->token == SLcToken_Int ||
		inCurToken->token == SLcToken_Bool ||
		inCurToken->token == SLcToken_Void
		)
	{
		return UUcTrue;
	}

	return UUcFalse;
}

static UUtBool
SLiScript_Parse_TokenStarts_Expression(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_LeftParen ||
		inCurToken->token == SLcToken_Identifier ||
		inCurToken->token == SLcToken_LogicalNot ||
		SLrScript_Parse_TokenStarts_Constant(inCurToken)
		)
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

UUtBool
SLrScript_Parse_TokenStarts_Operator_Unary(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_LogicalNot)
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

UUtBool
SLrScript_Parse_TokenStarts_Operator_Binary(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_Plus ||
		inCurToken->token == SLcToken_Minus ||
		inCurToken->token == SLcToken_Cmp_EQ ||
		inCurToken->token == SLcToken_Cmp_NE ||
		inCurToken->token == SLcToken_Cmp_LT ||
		inCurToken->token == SLcToken_Cmp_GT ||
		inCurToken->token == SLcToken_Cmp_LE ||
		inCurToken->token == SLcToken_Cmp_GE ||
		inCurToken->token == SLcToken_LogicalAnd ||
		inCurToken->token == SLcToken_LogicalOr ||
		inCurToken->token == SLcToken_Assign
		)
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

static UUtBool
SLiScript_Parse_TokenStarts_Operator_Arith(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_Plus ||
		inCurToken->token == SLcToken_Minus
		)
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

static UUtBool
SLiScript_Parse_TokenStarts_Operator_Cmp(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_Cmp_EQ ||
		inCurToken->token == SLcToken_Cmp_NE ||
		inCurToken->token == SLcToken_Cmp_LT ||
		inCurToken->token == SLcToken_Cmp_GT ||
		inCurToken->token == SLcToken_Cmp_LE ||
		inCurToken->token == SLcToken_Cmp_GE
		)
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

static UUtBool
SLiScript_Parse_TokenStarts_Operator_Logical(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_LogicalAnd ||
		inCurToken->token == SLcToken_LogicalOr
		)
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

static UUtBool
SLiScript_Parse_TokenStarts_Operator_Assign(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_Assign)
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

static UUtBool
SLiScript_Parse_TokenStarts_OldParameter(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_Identifier ||
		SLrScript_Parse_TokenStarts_Constant(inCurToken))
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

static UUtBool
SLiScript_Parse_TokenStarts_OldFunctionCall(
	SLtToken*	inCurToken,
	UUtBool		inExecuting)
{
	if(inCurToken[0].token == SLcToken_Identifier &&
		(!inExecuting || SLrDatabase_IsFunctionCall(inCurToken[0].lexem)) &&
		((inCurToken->flags & SLcTokenFlag_PrecedesNewline) ||
			SLiScript_Parse_TokenStarts_OldParameter(inCurToken + 1)))
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

static UUtBool
SLiScript_Parse_TokenStarts_FunctionCall(
	SLtToken*	inCurToken,
	UUtBool		inExecuting)
{
	if(
		inCurToken[0].token == SLcToken_Identifier &&
		inCurToken[1].token == SLcToken_LeftParen
		)
	{
		return UUcTrue;
	}
	
	if(SLiScript_Parse_TokenStarts_OldFunctionCall(inCurToken, inExecuting)) return UUcTrue;
	
	return UUcFalse;
}

static UUtBool
SLiScript_Parse_TokenStarts_Statement(
	SLtToken*	inCurToken)
{
	if(inCurToken->token == SLcToken_LeftCurley ||
		SLiScript_Parse_TokenStarts_DataDefinition(inCurToken) ||
		SLiScript_Parse_TokenStarts_Expression(inCurToken) ||
		inCurToken->token == SLcToken_Schedule ||
		inCurToken->token == SLcToken_Sleep ||
		inCurToken->token == SLcToken_Return ||
		inCurToken->token == SLcToken_Iterate ||
		inCurToken->token == SLcToken_Fork ||
		inCurToken->token == SLcToken_If
		)
	{
		return UUcTrue;
	}
		
	return UUcFalse;	
}

static UUtError
SLiScript_Parse_VerifyToken(
	SLtErrorContext*	inErrorContext,
	SLtToken*			inCurToken,
	SLtToken_Code		inDesiredToken)
{
	if(inCurToken->token != inDesiredToken)
	{
		inErrorContext->line = inCurToken->line;
		SLrScript_Error_Parse_IllegalToken(
			inErrorContext,
			inCurToken,
			inDesiredToken);
		
		return UUcError_Generic;
	}
	
	return UUcError_None;
}

static void
SLiParse_Stack_Push(
	SLtContext*	inContext,
	UUtUns8		inNewState)
{
	UUmAssert(inContext->curFuncState->parseTOS < SLcContext_ParseStack_MaxDepth);
	
	inContext->curFuncState->parseStack[inContext->curFuncState->parseTOS++] = inNewState;
}

static UUtUns8
SLiParse_Stack_Pop(
	SLtContext*	inContext)
{
	if (inContext->curFuncState->parseTOS == 0) {
		UUmAssert(0);
		return SLcParseState_DefaultReturn;
	} else {	
		return inContext->curFuncState->parseStack[--inContext->curFuncState->parseTOS];
	}
}

UUtError
SLrParse_FuncStack_Push_Console(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken)
{
	SLtContext_FuncState*	newState;
	
	newState = inContext->funcStack + inContext->funcTOS++;
	inContext->curFuncState = newState;
	
	UUrMemory_Clear(newState, sizeof(*newState));
	
	newState->funcName = "(console)";
	newState->symbol = NULL;
	
	newState->curToken = inToken;
	
	newState->scopeLevel = 1;
	
	SLiParse_Stack_Push(inContext, SLcParseState_DefaultReturn);
	SLiParse_Stack_Push(inContext, SLcParseState_Statements);
	SLrStatement_Level_Push(inContext, UUcTrue);

	return UUcError_None;
}
	

UUtError
SLrParse_FuncStack_Push(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	UUtUns16			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	const char*			inFuncName)
{
	UUtError				error;
	SLtContext_FuncState*	newState;
	SLtSymbol*				funcSymbol;
	UUtUns16				itr;
	
	BFrDebugFile_Printf(SLgDebugFile, "pushing context \"%s\"", inFuncName);
	SLrDebugMessage_LogParamList(inParameterListLength, inParameterList);
	
	error = SLrScript_Database_Symbol_Get(inContext, inFuncName, &funcSymbol);
	if(error != UUcError_None)
	{
		SLrScript_Error_Semantic(inErrorContext, "Could not find function \"%s\"", inFuncName);
		return UUcError_Generic;
	}
	
	if(inContext->funcTOS >= SLcContext_FuncState_MaxDepth)
	{
		UUmAssert(0);
		return UUcError_Generic;
	}
	
	if(funcSymbol->kind == SLcSymbolKind_Func_Script)
	{
		newState = inContext->funcStack + inContext->funcTOS++;
		inContext->curFuncState = newState;
		
		UUrMemory_Clear(newState, sizeof(*newState));
		
		newState->funcName = inFuncName;
		newState->symbol = funcSymbol;
		newState->scopeLevel = 1;
			
		error = 
			SLrScript_ParameterCheckAndPromote(
				inErrorContext,
				inFuncName,
				&funcSymbol->u.funcScript,
				&inParameterListLength,
				inParameterList);
		if(error != UUcError_None) return error;
								
		// enter all the parameters
		for(itr = 0; itr < inParameterListLength; itr++)
		{
			error = 
				SLrScript_Database_Var_Add(
					inContext,
					funcSymbol->u.funcScript.paramList[itr].name,
					inErrorContext,
					inParameterList[itr].type,
					inParameterList[itr].val);
			UUmError_ReturnOnError(error);
		}

		newState->curToken = funcSymbol->u.funcScript.startToken;
		
		SLiParse_Stack_Push(inContext, SLcParseState_DefaultReturn);
		SLiParse_Stack_Push(inContext, SLcParseState_Statements);
		SLrStatement_Level_Push(inContext, UUcTrue);
	}
	else if(funcSymbol->kind == SLcSymbolKind_Func_Command)
	{
		SLtParameter_Actual	resultParameter;
		
		if(funcSymbol->u.funcCommand.numParamListOptions > 0)
		{
			error = 
				SLrCommand_ParameterCheckAndPromote(
					inErrorContext,
					inFuncName,
					&funcSymbol->u.funcCommand,
					&inParameterListLength,
					inParameterList);
			if(error != UUcError_None) return error;
		}

		inContext->ticksTillCompletion = 0;
		inContext->stalled = UUcFalse;
		resultParameter.type = SLcType_Void;
		resultParameter.val.i = 0;
		
		error = 
			funcSymbol->u.funcCommand.command(
				inErrorContext,
				inParameterListLength,
				inParameterList,
				&inContext->ticksTillCompletion,
				&inContext->stalled,
				&resultParameter);
		if(error != UUcError_None)
		{
			SLrScript_ReportError(inErrorContext, "command \"%s\" return an error", inFuncName);
		}

		if (resultParameter.type != SLcType_Void) {
			SLrExpr_Push_Const(inContext, resultParameter.type, resultParameter.val);
		}

//		COrConsole_Printf("slr %s %d | %d", inFuncName, inContext->stalled, inContext->ticksTillCompletion);
		
		if ((inContext->ticksTillCompletion > 0) || (inContext->stalled))
		{
			// we are only here if we are sleeping or stalled
			
			// schedule this task to be executed later
			error = 
				SLrScheduler_Schedule(
					inContext,
					inErrorContext,
					inContext->stalled ? 1 : inContext->ticksTillCompletion);
			if(error != UUcError_None) return error;
			
			if(inContext->stalled)
			{
				newState = inContext->funcStack + inContext->funcTOS++;
				inContext->curFuncState = newState;
				
				UUrMemory_Clear(newState, sizeof(*newState));
				
				newState->funcName = inFuncName;
				newState->symbol = funcSymbol;

				// copy over the parameters
				newState->numParams = inParameterListLength;
				UUrMemory_MoveFast(inParameterList, newState->params, inParameterListLength * sizeof(*inParameterList));
			}			
		}
	}
	else
	{
		SLrScript_Error_Semantic(inErrorContext, "Could not find function \"%s\"", inFuncName);
		return UUcError_Generic;
	}
	
	return UUcError_None;
}

static UUtError 
SLrParse_FuncStack_Pop(
	SLtContext*	inContext)
{
	if (inContext->funcTOS == 0) {
		UUmAssert(0);
		return UUcError_Generic;
	} else {	
		inContext->funcTOS--;
		inContext->curFuncState = inContext->funcStack + inContext->funcTOS - 1;
		return UUcError_None;
	}
}

UUtError
SLrScript_Parse(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtParse_Mode		inMode)
{
	UUtError			error;
	SLtToken*			curToken;
	UUtBool				globalScope = UUcFalse;
	SLtToken*			identToken;
	SLtType		typeSpec;
	SLtType		resultType;
	SLtValue	resultVal;
	UUtBool				eval = UUcFalse;

	UUtUns16				funcDefParamIndex;
	SLtParameter_Formal	funcDefParams[SLcScript_MaxNumParams];
	SLtToken*				funcDefStartToken;
	SLtType			funcDefReturnType;
	SLtToken*				funcDefIdentToken;
	SLtParameter_Actual			paramList[SLcScript_MaxNumParams];
	SLtToken*				iteratorSavedToken;
	SLtSymbol*				iteratorSymbol;
	SLtSymbol*				iteratorVarSymbol;
	SLtToken*				stalledToken;	
	SLtContext_FuncState*	curFuncState;
	
	inContext->ticksTillCompletion = 0;

	curFuncState = inContext->curFuncState;

	inContext->returnType = SLcType_Void;
	inContext->returnValue.i = 0;

	if(inMode == SLcParseMode_AddToDatabase)
	{
		
		SLiParse_Stack_Push(inContext, SLcParseState_Global);
		inErrorContext->funcName = "(global scope)";
	}
	else if(inMode == SLcParseMode_InitialExecution)
	{
		
	}
	else if(inMode == SLcParseMode_ContinueExecution)
	{
		// check for stalled function call
		if (inContext->stalled) {		
			UUmAssert(curFuncState->symbol->kind == SLcSymbolKind_Func_Command);

			inContext->ticksTillCompletion = 0;
			inContext->stalled = UUcFalse;
			
			error = 
				curFuncState->symbol->u.funcCommand.command(
					inErrorContext,
					curFuncState->numParams,
					curFuncState->params,
					&inContext->ticksTillCompletion,
					&inContext->stalled,
					NULL);
			if (error != UUcError_None) return error;
			
			if ((inContext->ticksTillCompletion > 0) || (inContext->stalled))
			{
				// schedule this task to be executed later
				error = 
					SLrScheduler_Schedule(
						inContext,
						inErrorContext,
						inContext->stalled ? 1 : inContext->ticksTillCompletion);
				if(error != UUcError_None) return error;
				return UUcError_None;
			}
			else
			{
				error = SLrParse_FuncStack_Pop(inContext);
				if (error != UUcError_None) {
					return error;
				}
			}
			
		}
	}
	else
	{
		UUmAssert(!"Illegal mode");
	}
	
	
startOver:	
	curFuncState = inContext->curFuncState;
	curToken = curFuncState->curToken;

	while(1)
	{
		SLiContext_Dump_Info(inContext, curToken);

		inErrorContext->line = curToken->line;
		curFuncState = inContext->curFuncState;
		inErrorContext->funcName = curFuncState->funcIdent;
		
		switch(SLiParse_Stack_Pop(inContext))
		{
			case SLcParseState_Global:
				UUmAssert(inMode == SLcParseMode_AddToDatabase);
				
				globalScope = UUcTrue;
				
				SLiParse_Stack_Push(inContext, SLcParseState_Global);
				if(SLiScript_Parse_TokenStarts_DataDefinition(curToken))
				{
					SLiParse_Stack_Push(inContext, SLcParseState_DataDef);
				}
				else if(SLiScript_Parse_TokenStarts_FuncDefinition(curToken))
				{
					SLiParse_Stack_Push(inContext, SLcParseState_FuncDef);
				}
				else if(curToken->token == SLcToken_EOF)
				{
					goto done;
				}
				else
				{
					SLrScript_Error_Parse(
						inErrorContext,
						curToken,
						"expected either a data definition or a function definition");
					
					return UUcError_Generic;
				}
				break;
				
			case SLcParseState_DataDef:
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_Var);
				if(error != UUcError_None) return error;
				curToken++;
				
				SLiParse_Stack_Push(inContext, SLcParseState_DataDef2);
				SLiParse_Stack_Push(inContext, SLcParseState_TypeSpec);
				break;
			
			case SLcParseState_DataDef2:
				identToken = curToken;

				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_Identifier);
				if(error != UUcError_None) return error;
				curToken++;
				
				SLiParse_Stack_Push(inContext, SLcParseState_DataDef3);
				
				// check for an initialization
				if(curToken->token == SLcToken_Assign)
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Expression);
				}
				eval = inMode != SLcParseMode_AddToDatabase || globalScope;
				break;
			
			case SLcParseState_DataDef3:
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_SemiColon);
				if(error != UUcError_None) return error;
				curToken++;
				
				inErrorContext->line = curToken->line;
				
				if(eval == UUcFalse ||
					SLrExpr_GetResult(
						inContext,
						inErrorContext,
						&resultType,
						&resultVal) == UUcFalse)
				{
					resultVal.i = 0;
					resultType = typeSpec;
				}
				
				error = SLrExpr_ConvertValue(inErrorContext, resultType, typeSpec, &resultVal);
				if(error != UUcError_None) return error;
				
				if(globalScope || inMode != SLcParseMode_AddToDatabase)
				{
					SLrScript_Database_Var_Add(
						inMode == SLcParseMode_AddToDatabase ? NULL : inContext,
						identToken->lexem,
						inErrorContext,
						typeSpec,
						resultVal);
				}
				eval = inMode != SLcParseMode_AddToDatabase;
				break;
				
			case SLcParseState_TypeSpec:
				switch(curToken->token)
				{
					case SLcToken_String:
						typeSpec = SLcType_String;
						break;
					
					case SLcToken_Float:
						typeSpec = SLcType_Float;
						break;
					
					case SLcToken_Int:
						typeSpec = SLcType_Int32;
						break;
					
					case SLcToken_Void:
						typeSpec = SLcType_Void;
						break;
					
					case SLcToken_Bool:
						typeSpec = SLcType_Bool;
						break;
					
					default:
						SLrScript_Error_Parse(
							inErrorContext,
							curToken,
							"expected a type specification here");
						
						return UUcError_Generic;
				}
				curToken++;
				break;
			
			case SLcParseState_FuncDef:
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_Func);
				if(error != UUcError_None) return error;
				curToken++;
				
				SLiParse_Stack_Push(inContext, SLcParseState_FuncDef2);
				
				typeSpec = SLcType_Void;
				
				if(SLiScript_Parse_TokenStarts_TypeSpec(curToken))
				{
					SLiParse_Stack_Push(inContext, SLcParseState_TypeSpec);
				}
				break;
				
			case SLcParseState_FuncDef2:
				funcDefIdentToken = curToken;
				funcDefReturnType = typeSpec;
				
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_Identifier);
				if(error != UUcError_None) return error;
				curToken++;
				
				funcDefParamIndex = 0;
				SLiParse_Stack_Push(inContext, SLcParseState_FuncDef3);
				
				if(curToken->token == SLcToken_LeftParen)
				{
					curToken++;

					if(curToken->token == SLcToken_RightParen)
					{
						curToken++;
					}
					else
					{
						SLiParse_Stack_Push(inContext, SLcParseState_FuncDef_FormalParamList);
					}
				}
				break;

			case SLcParseState_FuncDef3:
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_LeftCurley);
				if(error != UUcError_None) return error;
				curToken++;
				
				funcDefStartToken = curToken;
				globalScope = UUcFalse;
				
				SLiParse_Stack_Push(inContext, SLcParseState_FuncDef4);
				SLiParse_Stack_Push(inContext, SLcParseState_Statements);
				break;

			case SLcParseState_FuncDef4:
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_RightCurley);
				if(error != UUcError_None) return error;
				curToken++;
				
				if(inMode == SLcParseMode_AddToDatabase)
				{
					inErrorContext->line = curToken->line;
					error =
						SLrScript_Database_FunctionScript_Add(
							funcDefIdentToken->lexem,
							inErrorContext,
							funcDefReturnType,
							funcDefParamIndex,
							funcDefParams,
							curToken - funcDefStartToken,
							funcDefStartToken);
					if(error != UUcError_None) return error;
				}
				else
				{
					SLrStatement_Level_Pop(inContext);
				}
				break;
							
			case SLcParseState_FuncDef_FormalParamList:
				SLiParse_Stack_Push(inContext, SLcParseState_FuncDef_FormalParamList_Finished);
				if(curToken->token == SLcToken_Void)
				{
					curToken++;
				}
				else
				{
					SLiParse_Stack_Push(inContext, SLcParseState_FuncDef_FormalParamList2);
					SLiParse_Stack_Push(inContext, SLcParseState_TypeSpec);
				}
				break;
				
			case SLcParseState_FuncDef_FormalParamList2:
				funcDefParams[funcDefParamIndex].name = curToken->lexem;
				funcDefParams[funcDefParamIndex].type = typeSpec;
				funcDefParamIndex++;
				
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_Identifier);
				if(error != UUcError_None) return error;
				curToken++;
				
				if(curToken->token == SLcToken_Comma)
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_FuncDef_FormalParamList2);
					SLiParse_Stack_Push(inContext, SLcParseState_TypeSpec);
				}
				break;
			
			case SLcParseState_FuncDef_FormalParamList_Finished:
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_RightParen);
				if(error != UUcError_None) return error;
				curToken++;
				break;
			
			case SLcParseState_Statements:
				SLiParse_Stack_Push(inContext, SLcParseState_Statements2);
				SLiParse_Stack_Push(inContext, SLcParseState_Statement);
				break;
							
			case SLcParseState_Statements2:
				if(SLiScript_Parse_TokenStarts_Statement(curToken))
				{
					SLiParse_Stack_Push(inContext, SLcParseState_Statements);
				}
				break;
							
			case SLcParseState_Statement:
				
				if(curToken->token == SLcToken_LeftCurley)
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Statement_ReturnFromCurley);
					SLiParse_Stack_Push(inContext, SLcParseState_Statements);
					
					if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
					{
						SLrScript_Database_Scope_Enter(inContext);
					}
				}
				else if(SLiScript_Parse_TokenStarts_DataDefinition(curToken))
				{
					SLiParse_Stack_Push(inContext, SLcParseState_DataDef);
				}
				else if(SLiScript_Parse_TokenStarts_Expression(curToken))
				{
					SLiParse_Stack_Push(inContext, SLcParseState_Statement_Expr_ReturnFromExpr);
					SLiParse_Stack_Push(inContext, SLcParseState_Expression);
					inContext->ticksTillCompletion = 0;
					inContext->stalled = UUcFalse;
					stalledToken = curToken;
					UUmAssert(curFuncState->parenTOS == 0);
					
				}
				else if(curToken->token == SLcToken_Schedule)
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Statement_Schdl_ReturnFromFuncCall);
					SLiParse_Stack_Push(inContext, SLcParseState_Expression_FunctionCall);
					
				}
				else if(curToken->token == SLcToken_Fork)
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Statement_Fork_ReturnFromFuncCall);
					SLiParse_Stack_Push(inContext, SLcParseState_Expression_FunctionCall);
					
				}
				else if(curToken->token == SLcToken_Sleep)
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Statement_Sleep_ReturnFromExpr);
					SLiParse_Stack_Push(inContext, SLcParseState_Expression);
				}
				else if(curToken->token == SLcToken_Return)
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Statement_Return_ReturnFromExpr);
					if(SLiScript_Parse_TokenStarts_Expression(curToken))
					{
						SLiParse_Stack_Push(inContext, SLcParseState_Expression);
					}
				}
				else if(curToken->token == SLcToken_If)
				{
					curToken++;
					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_LeftParen);
					if(error != UUcError_None) return error;
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Statement_If_ReturnFromExpr);
					SLiParse_Stack_Push(inContext, SLcParseState_Expression);
				}
				else if(curToken->token == SLcToken_Iterate)
				{
					curToken++;
					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_Over);
					if(error != UUcError_None) return error;
					curToken++;

					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_Identifier);
					if(error != UUcError_None) return error;
					
					if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
					{
						error = 
							SLrScript_Database_Symbol_Get(
								inContext,
								curToken->lexem,
								&iteratorSymbol);
						if(error != UUcError_None)
						{
							SLrScript_Error_Semantic(inErrorContext, "iterator \"%s\" does not exist", curToken->lexem);
							return UUcError_Generic;
						}
						
					}
					
					curToken++;
					
					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_Using);
					if(error != UUcError_None) return error;
					curToken++;
					
					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_Identifier);
					if(error != UUcError_None) return error;
					
					if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
					{
						error = 
							SLrScript_Database_Symbol_Get(
								inContext,
								curToken->lexem,
								&iteratorVarSymbol);
						if(error != UUcError_None)
						{
							SLrScript_Error_Semantic(inErrorContext, "iterator variable \"%s\" does not exist", curToken->lexem);
							return UUcError_Generic;
						}
						
						if(iteratorSymbol->u.iterator.type != iteratorVarSymbol->u.var.type)
						{
							SLrScript_Error_Semantic(inErrorContext, "iterator variable \"%s\" types do not match", curToken->lexem);
							return UUcError_Generic;
						}
	
						if(iteratorSymbol->u.iterator.getFirst(&iteratorVarSymbol->u.var.val))
						{
							iteratorSavedToken = curToken + 1;
							SLiParse_Stack_Push(inContext, SLcParseState_Statement_Iterating);
						}
					}
					curToken++;
					
					SLiParse_Stack_Push(inContext, SLcParseState_Statement);
				}
				else
				{
					// empty statement do nothing
				}
				break;
			
			case SLcParseState_Statement_Iterating:
				UUmAssert(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext));
				
				if(iteratorSymbol->u.iterator.getNext(&iteratorVarSymbol->u.var.val))
				{
					curToken = iteratorSavedToken;
					SLiParse_Stack_Push(inContext, SLcParseState_Statement_Iterating);
					SLiParse_Stack_Push(inContext, SLcParseState_Statement);
				}
				break;
				
			
			case SLcParseState_Statement_If_ReturnFromExpr:
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_RightParen);
				if(error != UUcError_None) return error;
				curToken++;
				
				SLiParse_Stack_Push(inContext, SLcParseState_Statement_If_ReturnFromExpr3);
				SLiParse_Stack_Push(inContext, SLcParseState_Statement_If_ReturnFromExpr2);
				SLiParse_Stack_Push(inContext, SLcParseState_Statement);
				
				if(inMode != SLcParseMode_AddToDatabase)
				{
					if(SLrStatement_Evaluate(inContext) == UUcFalse)
					{
						SLrStatement_Level_Push(inContext, UUcFalse);
					}
					else
					{
						inErrorContext->line = curToken->line;
						if(SLrExpr_GetResult(inContext, inErrorContext, &resultType, &resultVal) == UUcFalse)
						{
							UUmAssert(0);
						}
						
						if(resultType != SLcType_Int32 && resultType != SLcType_Bool)
						{
							SLrScript_Error_Semantic(inErrorContext, "if needs an integer or boolean");
							return UUcError_Generic;
						}
						
						SLrStatement_Level_Push(inContext, resultVal.i == 0 ? UUcFalse : UUcTrue);
					}
				}
				break;
			
			case SLcParseState_Statement_If_ReturnFromExpr2:
				if(curToken->token == SLcToken_Else)
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Statement);
					if(inMode != SLcParseMode_AddToDatabase)
					{
						SLrStatement_Level_Invert(inContext);
					}
				}
				break;
			
			case SLcParseState_Statement_If_ReturnFromExpr3:
				if(inMode != SLcParseMode_AddToDatabase)
				{
					SLrStatement_Level_Pop(inContext);
				}
				break;
			
			case SLcParseState_Statement_Return_ReturnFromExpr:
				if(!(curToken[-1].flags & SLcTokenFlag_PrecedesNewline))
				{
					// require ;
					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_SemiColon);
					if(error != UUcError_None) return error;
					curToken++;
				}
				else if(curToken->token == SLcToken_SemiColon)
				{
					// ; is optional
					curToken++;
				}
				
				// pop the expr and put it in the result slot
				if(SLrExpr_GetResult(
					inContext,
					inErrorContext,
					&inContext->returnType,
					&inContext->returnValue) == UUcFalse)
				{
					inContext->returnType = SLcType_Void;
					inContext->returnValue.i = 0;
				}
				
				if(inMode != SLcParseMode_AddToDatabase) goto done;
				
				break;
				
			case SLcParseState_Statement_Sleep_ReturnFromExpr:
				if(!(curToken[-1].flags & SLcTokenFlag_PrecedesNewline))
				{
					// require ;
					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_SemiColon);
					if(error != UUcError_None) return error;
					curToken++;
				}
				else if(curToken->token == SLcToken_SemiColon)
				{
					// ; is optional
					curToken++;
				}
				
				if(inMode == SLcParseMode_AddToDatabase)
				{
					if(funcDefReturnType != SLcType_Void)
					{
						inErrorContext->line = curToken->line;
						SLrScript_Error_Semantic(inErrorContext, "Can not have a sleep statement in a function that returns a value");
						return UUcError_Generic;
					}
				}
				else if(SLrStatement_Evaluate(inContext))
				{
					curFuncState->curToken = curToken;
					
					// schedule this context to be slept
					if(SLrExpr_GetResult(
						inContext,
						inErrorContext,
						&resultType,
						&resultVal) == UUcFalse)
					{
						UUmAssert(0);
						return UUcError_Generic;
					}
					UUmAssert(resultType == SLcType_Int32);
					
					error = 
						SLrScheduler_Schedule(
							inContext,
							inErrorContext,
							resultVal.i);
					if(error != UUcError_None) return error;
					
					return UUcError_None;
				}
				break;
				
			case SLcParseState_Statement_ReturnFromCurley:
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_RightCurley);
				if(error != UUcError_None) return error;
				curToken++;
				if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
				{
					SLrScript_Database_Scope_Leave(inContext);
				}
				break;
				
			case SLcParseState_Statement_Expr_ReturnFromExpr:
				if(!(curToken[-1].flags & SLcTokenFlag_PrecedesNewline))
				{
					// require ;
					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_SemiColon);
					if(error != UUcError_None) return error;
					curToken++;
				}
				else if(curToken->token == SLcToken_SemiColon)
				{
					// ; is optional
					curToken++;
				}
					
				if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
				{
					if(SLrExpr_GetResult(
						inContext,
						inErrorContext,
						&inContext->returnType,
						&inContext->returnValue) == UUcFalse)
					{
						inContext->returnType = SLcType_Void;
					}
					
					if(curFuncState->parenTOS != 0)
					{
						SLrScript_Error_Semantic(inErrorContext, "Missing some )'s");
						return UUcError_Generic;
					}
				}
				break;
				
			case SLcParseState_Statement_Schdl_ReturnFromFuncCall:
				if(curToken->token == SLcToken_At)
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Statement_Schdl_At);
					SLiParse_Stack_Push(inContext, SLcParseState_Expression);
				}
				else if(curToken->token == SLcToken_Repeat)
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Statement_Schdl_Repeat);
					SLiParse_Stack_Push(inContext, SLcParseState_Expression);
				}
				else
				{
					if(!(curToken[-1].flags & SLcTokenFlag_PrecedesNewline))
					{
						// require ;
						error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_SemiColon);
						if(error != UUcError_None) return error;
						curToken++;
					}
					else if(curToken->token == SLcToken_SemiColon)
					{
						// ; is optional
						curToken++;
					}
				}
				break;
				
			case SLcParseState_Statement_Schdl_At:
				if(!(curToken[-1].flags & SLcTokenFlag_PrecedesNewline))
				{
					// require ;
					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_SemiColon);
					if(error != UUcError_None) return error;
					curToken++;
				}
				else if(curToken->token == SLcToken_SemiColon)
				{
					// ; is optional
					curToken++;
				}
				
				if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
				{
					inErrorContext->line = curToken->line;
					if(SLrExpr_GetResult(
						inContext,
						inErrorContext,
						&resultType,
						&resultVal) == UUcFalse)
					{
						UUmAssert(0);
						return UUcError_Generic;
					}
					
					if(resultType != SLcType_Int32)
					{
						SLrScript_Error_Semantic(inErrorContext, "schedule at needs an integer");
						return UUcError_Generic;
					}
					
					error = 
						SLrExprListToParamList(
							inContext,
							inErrorContext,
							paramList);
					if(error != UUcError_None) return error;
					
					inErrorContext->line = curToken->line;
					error =
						SLrScript_Schedule(
							curFuncState->funcIdent,
							curFuncState->numParams,
							paramList,
							NULL,
							resultVal.i,
							1,
							NULL);				// CB: no way of tracking sub-functions, but parent will still be hanging around
					if(error != UUcError_None) return error;
				}
				break;

			case SLcParseState_Statement_Schdl_Repeat:
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_Every);
				if(error != UUcError_None) return error;
				curToken++;
				SLiParse_Stack_Push(inContext, SLcParseState_Statement_Schdl_Repeat2);
				SLiParse_Stack_Push(inContext, SLcParseState_Expression);
				break;

			case SLcParseState_Statement_Schdl_Repeat2:
				if(!(curToken[-1].flags & SLcTokenFlag_PrecedesNewline))
				{
					// require ;
					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_SemiColon);
					if(error != UUcError_None) return error;
					curToken++;
				}
				else if(curToken->token == SLcToken_SemiColon)
				{
					// ; is optional
					curToken++;
				}
				
				if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
				{
					UUtUns32	numberOfTimes;
					UUtUns32	timeDelta;
					
					inErrorContext->line = curToken->line;
					if(SLrExpr_GetResult(
						inContext,
						inErrorContext,
						&resultType,
						&resultVal) == UUcFalse)
					{
						UUmAssert(0);
						return UUcError_Generic;
					}
					
					if(resultType != SLcType_Int32)
					{
						SLrScript_Error_Semantic(inErrorContext, "schedule at needs an integer");
						return UUcError_Generic;
					}
					
					timeDelta = resultVal.i;
					
					if(SLrExpr_GetResult(
						inContext,
						inErrorContext,
						&resultType,
						&resultVal) == UUcFalse)
					{
						UUmAssert(0);
						return UUcError_Generic;
					}
					
					if(resultType != SLcType_Int32)
					{
						SLrScript_Error_Semantic(inErrorContext, "schedule at needs an integer");
						return UUcError_Generic;
					}
					
					numberOfTimes = resultVal.i;
					
					error = 
						SLrExprListToParamList(
							inContext,
							inErrorContext,
							paramList);
					if(error != UUcError_None) return error;
					
					inErrorContext->line = curToken->line;
					error =
						SLrScript_Schedule(
							curFuncState->funcIdent,
							curFuncState->numParams,
							paramList,
							NULL,
							timeDelta,
							numberOfTimes,
							NULL);				// CB: no way of tracking sub-functions, but parent will still be hanging around
					if(error != UUcError_None) return error;
				}
				break;

			case SLcParseState_Expression:
				SLiParse_Stack_Push(inContext, SLcParseState_Expression_Finish);
				SLiParse_Stack_Push(inContext, SLcParseState_Expression2);
				if(eval || (inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext)))
				{
					error = SLrExpr_Parse_LeftParen(inContext, inErrorContext);
					if(error != UUcError_None) return error;
				}
				break;
				
			case SLcParseState_Expression2:
				if(curToken->token == SLcToken_LeftParen)
				{
					if(eval || (inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext)))
					{
						error = SLrExpr_Parse_LeftParen(inContext, inErrorContext);
						if(error != UUcError_None) return error;
					}
					
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Expression3);
					SLiParse_Stack_Push(inContext, SLcParseState_Expression2);
				}
				else if(SLrScript_Parse_TokenStarts_Operator_Unary(curToken))
				{
					if(eval || (inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext)))
					{
						error = SLrExpr_Parse_Op_Unary(inContext, inErrorContext, curToken);
						if(error != UUcError_None) return error;
					}
					
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Expression2);
				}
				else if(curToken->token == SLcToken_Identifier)
				{
					if(SLiScript_Parse_TokenStarts_FunctionCall(curToken, inMode != SLcParseMode_AddToDatabase))
					{
						SLiParse_Stack_Push(inContext, SLcParseState_Expression4);
						SLiParse_Stack_Push(inContext, SLcParseState_Expression_FunctionCall_Finish);
						SLiParse_Stack_Push(inContext, SLcParseState_Expression_FunctionCall);
					}
					else
					{
						if(eval || (inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext)))
						{
							error = SLrExpr_Parse_Identifier(inContext, curToken);
							if(error != UUcError_None) return error;
						}
						
						curToken++;
						SLiParse_Stack_Push(inContext, SLcParseState_Expression4);
					}
				}
				else if(SLrScript_Parse_TokenStarts_Constant(curToken))
				{
					if(eval || (inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext)))
					{
						error = SLrExpr_Parse_Op_Constant(inContext, curToken);
						if(error != UUcError_None) return error;
					}
					
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Expression4);
				}
				break;
			
			case SLcParseState_Expression4:
				if(SLrScript_Parse_TokenStarts_Operator_Binary(curToken))
				{
					if(eval || (inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext)))
					{
						error = SLrExpr_Parse_Op_Binary(inContext, inErrorContext, curToken);
						if(error != UUcError_None) return error;
					}
					
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Expression2);
				}
				break;
			
			case SLcParseState_Expression_Finish:
				if(eval || (inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext)))
				{
					error = SLrExpr_Parse_RightParen(inContext, inErrorContext);
					if(error != UUcError_None) return error;
					
					error = SLrExpr_Parse_Final(inContext, inErrorContext);
					if(error != UUcError_None) return error;
				}
				break;
				
			case SLcParseState_Expression_FunctionCall:
				UUmAssert(curToken->token == SLcToken_Identifier);
				if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
				{
					error = SLrExpr_Parse_FunctionCall_Ident(inContext, curToken);
					if(error != UUcError_None) return error;
				}
				
				if(SLiScript_Parse_TokenStarts_OldFunctionCall(curToken, inMode != SLcParseMode_AddToDatabase))
				{
					if(!(curToken->flags & SLcTokenFlag_PrecedesNewline))
					{
						SLiParse_Stack_Push(inContext, SLcParseState_Expression_FunctionCall_ParamList_Old);
					}
					curToken++;
				}
				else
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Expression_FunctionCall_ParamList_New);
					UUmAssert(curToken->token == SLcToken_LeftParen);
					curToken++;
				}
				break;
			
			case SLcParseState_Expression3:
				error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_RightParen);
				if(error != UUcError_None) return error;
				
				if(eval || (inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext)))
				{
					error = SLrExpr_Parse_RightParen(inContext, inErrorContext);
					if(error != UUcError_None) return error;
				}
				
				curToken++;
				SLiParse_Stack_Push(inContext, SLcParseState_Expression4);
				break;
				
			case SLcParseState_Expression_FunctionCall_ParamList_New:
				if(curToken->token == SLcToken_RightParen)
				{
					curToken++;
				}
				else
				{
					SLiParse_Stack_Push(inContext, SLcParseState_Expression_FunctionCall_ParamList_New2);
					SLiParse_Stack_Push(inContext, SLcParseState_Expression);
				}
				break;

			case SLcParseState_Expression_FunctionCall_ParamList_New2:
				if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
				{
					error = SLrExpr_Parse_Param(inContext);
					if(error != UUcError_None) return error;
				}
				
				if(curToken->token == SLcToken_Comma)
				{
					curToken++;
					SLiParse_Stack_Push(inContext, SLcParseState_Expression_FunctionCall_ParamList_New);
				}
				else
				{
					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_RightParen);
					if(error != UUcError_None) return error;
					curToken++;
				}
				break;

			case SLcParseState_Expression_FunctionCall_ParamList_Old:
				if(SLrScript_Parse_TokenStarts_Constant(curToken) ||
					(curToken->token == SLcToken_Identifier))
				{
					if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
					{
						error = SLrExpr_Parse_Param_Old(inContext, curToken);
						if(error != UUcError_None) return error;
					}
				}
				else
				{
					SLrScript_Error_Semantic(inErrorContext, "Illegal token in old style function");
					return UUcError_Generic;
				}
				if(!(curToken->flags & SLcTokenFlag_PrecedesNewline))
				{
					SLiParse_Stack_Push(inContext, SLcParseState_Expression_FunctionCall_ParamList_Old);
				}
				curToken++;
				break;
				
			case SLcParseState_Expression_FunctionCall_Finish:
				if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
				{
					curFuncState->curToken = curToken;
					
					error = SLrExpr_Parse_FunctionCall_Make(inContext, inErrorContext);
					if(error != UUcError_None) return error;
					
					if(inContext->ticksTillCompletion > 0 || inContext->stalled)
					{
						// we need to sleep for a while
						
						return UUcError_None;
					}
					
					if(inContext->returnType != SLcType_Void)
					{
						SLrExpr_Push_Const(
							inContext,
							inContext->returnType,
							inContext->returnValue);
					}
					
					curToken = inContext->curFuncState->curToken;
					
				}
				break;
				
			case SLcParseState_DefaultReturn:
				//inContext->returnType = SLcType_Void;
				//inContext->returnValue.i = 0;
				goto done;
			
			case SLcParseState_Statement_Fork_ReturnFromFuncCall:
				if(!(curToken[-1].flags & SLcTokenFlag_PrecedesNewline))
				{
					// require ;
					error = SLiScript_Parse_VerifyToken(inErrorContext, curToken, SLcToken_SemiColon);
					if(error != UUcError_None) return error;
					curToken++;
				}
				else if(curToken->token == SLcToken_SemiColon)
				{
					// ; is optional
					curToken++;
				}
				
				if(inMode != SLcParseMode_AddToDatabase && SLrStatement_Evaluate(inContext))
				{
					error = 
						SLrExprListToParamList(
							inContext,
							inErrorContext,
							paramList);
					if(error != UUcError_None) return error;

					error =
						SLrScript_ExecuteOnce(
							curFuncState->funcIdent,
							curFuncState->numParams,
							paramList,
							NULL, NULL);
					if(error != UUcError_None) return error;
				}
				break;
			
			
			
			default:
				UUmAssert(0);
		}
	}

done:
	
	if(inContext->funcTOS == 1) return UUcError_None;
	
	error = SLrParse_FuncStack_Pop(inContext);
	if (error != UUcError_None) {
		return error;
	}
	
	// if the result is not NULL then put the result on this stack
	if(inContext->returnType != SLcType_Void)
	{
		SLrExpr_Push_Const(
			inContext,
			inContext->returnType,
			inContext->returnValue);
	}
	
	goto startOver;
	
	
	return UUcError_None;
}

UUtError
SLrParse_TokenToType(
	SLtToken_Code	inTokenCode,
	SLtType	*outVariableType)
{
	switch(inTokenCode)
	{
		case SLcToken_Float:
			*outVariableType = SLcType_Float;
			return UUcError_None;
			
		case SLcToken_String:
			*outVariableType = SLcType_String;
			return UUcError_None;
		
		case SLcToken_Void:
			*outVariableType = SLcType_Void;
			return UUcError_None;
		
		case SLcToken_Int:
			*outVariableType = SLcType_Int32;
			return UUcError_None;
		
		case SLcToken_Bool:
			*outVariableType = SLcType_Bool;
			return UUcError_None;
	}
	
	return UUcError_Generic;
}

UUtError
SLrParse_ConstTokenToTypeAndVal(
	SLtToken*			inToken,
	SLtType		*outType,
	SLtValue	*outVal)
{
	switch(inToken->token)
	{
		case SLcToken_Constant_Int:
			*outType = SLcType_Int32;
			break;
			
		case SLcToken_Constant_Float:
			*outType = SLcType_Float;
			break;
			
		case SLcToken_Constant_String:
			*outType = SLcType_String;
			break;
		
		case SLcToken_Constant_Bool:
			*outType = SLcType_Bool;
			break;
		
		default:
			UUmAssert(0);
			return UUcError_Generic;
	}
	
	switch(*outType)
	{
		case SLcType_Int32:
			UUrString_To_Int32(inToken->lexem, &outVal->i);
			break;
		
		case SLcType_String:
			outVal->str = (char*)inToken->lexem;
			break;
		
		case SLcType_Float:
			UUrString_To_Float(inToken->lexem, &outVal->f);
			break;
		
		case SLcType_Bool:
			outVal->b = inToken->lexem[0] == '0' ? UUcFalse : UUcTrue;
			break;
		
		default:
			UUmAssert(0);
			return UUcError_Generic;
	}
	
	return UUcError_None;
}

