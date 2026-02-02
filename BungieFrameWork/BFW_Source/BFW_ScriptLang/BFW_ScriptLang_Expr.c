/*
	FILE:	BFW_ScriptLang_Expr.c

	AUTHOR:	Brent H. Pease

	CREATED: Oct 29, 1999

	PURPOSE:

	Copyright 1999

*/

#include "BFW.h"
#include "BFW_ScriptLang.h"
#include "BFW_ScriptLang_Private.h"
#include "BFW_ScriptLang_Expr.h"
#include "BFW_ScriptLang_Parse.h"
#include "BFW_ScriptLang_Error.h"
#include "BFW_ScriptLang_Eval.h"
#include "BFW_ScriptLang_Database.h"

static SLtToken*
SLiExpr_OpStack_Pop(
	SLtContext*			inContext)
{
	SLtExpr_OpStack*	opStack;

	UUmAssertReadPtr(inContext->curFuncState->curParenStack, sizeof(SLtExpr_OpStack));

	opStack = inContext->curFuncState->curParenStack;

	if(opStack->tos > 0)
	{
		return opStack->stack[--opStack->tos];
	}

	return NULL;
}

static UUtError
SLiExpr_OpStack_Push(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken)
{
	SLtExpr_OpStack*	opStack;

	UUmAssertReadPtr(inContext->curFuncState->curParenStack, sizeof(SLtExpr_OpStack));

	opStack = inContext->curFuncState->curParenStack;

	if(opStack->tos >= SLcExprOpStack_MaxDepth)
	{
		SLrScript_Error_Semantic(inErrorContext, "expression too complex");
		return UUcError_Generic;
	}

	opStack->stack[opStack->tos++] = inToken;

	return UUcError_None;
}

static SLtToken*
SLiExpr_OpStack_TOS(
	SLtContext*			inContext)
{
	SLtExpr_OpStack*	opStack;

	UUmAssertReadPtr(inContext->curFuncState->curParenStack, sizeof(SLtExpr_OpStack));

	opStack = inContext->curFuncState->curParenStack;

	if(opStack->tos > 0)
	{
		return opStack->stack[opStack->tos - 1];
	}

	return NULL;
}

static UUtError
SLiExpr_UnaryOp_TypeCheck(
	SLtToken*		inToken,
	SLtType	inType)
{

	return UUcError_None;
}

static UUtError
SLiExpr_BinaryOp_TypeCheck(
	SLtToken*		inToken,
	SLtType	inLeftType,
	SLtType	inRightType)
{

	return UUcError_None;
}

static UUtError
SLiExpr_Evaluate(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken)
{
	UUtError			error;
	SLtType		resultType = SLcType_Void;
	SLtValue	resultVal;

	if(SLrScript_Parse_TokenStarts_Operator_Binary(inToken))
	{
		SLtExpr*			rightSide;
		SLtExpr*			leftSide;
		SLtType		rightType;
		SLtValue	rightVal;
		SLtType		leftType;
		SLtValue	leftVal;
		UUtInt32			leftInt;
		UUtInt32			rightInt;

		rightSide = SLrExpr_Pop(inContext);
		leftSide = SLrExpr_Pop(inContext);

		inErrorContext->line = inToken->line;
		error = SLrExpr_Eval(inContext, inErrorContext, rightSide, &rightType, &rightVal);
		if(error != UUcError_None) return error;

		inErrorContext->line = inToken->line;
		error = SLrExpr_Eval(inContext, inErrorContext, leftSide, &leftType, &leftVal);
		if(error != UUcError_None) return error;

		error = SLiExpr_BinaryOp_TypeCheck(inToken, leftType, rightType);
		if(error != UUcError_None) return error;

		switch(inToken->token)
		{
			case SLcToken_Assign:
				if(leftSide->kind != SLcExprKind_Identifier)
				{
					SLrScript_Error_Semantic(inErrorContext, "The left side of an assignment must be an identifier");
					return UUcError_Generic;
				}

				SLrExpr_ConvertValue(inErrorContext, rightType, leftType, &rightVal);

				error =
					SLrScript_Database_Var_SetValue(
						inErrorContext,
						inContext,
						leftSide->u.ident.name,
						rightVal);
				if(error != UUcError_None) return error;

				resultType = rightType;
				resultVal = rightVal;
				break;

			case SLcToken_Plus:
				resultType = leftType;
				if(leftType == SLcType_Int32)
				{
					resultVal.i = leftVal.i + rightVal.i;
				}
				else
				{
					resultVal.f = leftVal.f + rightVal.f;
				}
				break;

			case SLcToken_Minus:
				resultType = leftType;
				if(leftType == SLcType_Int32)
				{
					resultVal.i = leftVal.i - rightVal.i;
				}
				else
				{
					resultVal.f = leftVal.f - rightVal.f;
				}
				break;

			case SLcToken_LogicalAnd:
				resultType = SLcType_Bool;
				leftInt = SLrExpr_ValAndType_To_Int(leftType, leftVal);
				rightInt = SLrExpr_ValAndType_To_Int(rightType, rightVal);
				resultVal.b = leftInt && rightInt;
				break;

			case SLcToken_LogicalOr:
				resultType = SLcType_Bool;
				leftInt = SLrExpr_ValAndType_To_Int(leftType, leftVal);
				rightInt = SLrExpr_ValAndType_To_Int(rightType, rightVal);
				resultVal.b = leftInt || rightInt;
				break;

			case SLcToken_Cmp_EQ:
				resultType = SLcType_Int32;
				if(leftType == SLcType_Int32 || leftType == SLcType_Bool)
				{
					resultVal.i = leftVal.i == rightVal.i;
				}
				else
				{
					resultVal.i = leftVal.f == rightVal.f;
				}
				break;

			case SLcToken_Cmp_NE:
				resultType = SLcType_Int32;
				if(leftType == SLcType_Int32 || leftType == SLcType_Bool)
				{
					resultVal.i = leftVal.i != rightVal.i;
				}
				else
				{
					resultVal.i = leftVal.f != rightVal.f;
				}
				break;

			case SLcToken_Cmp_LT:
				resultType = SLcType_Int32;
				if(leftType == SLcType_Int32 || leftType == SLcType_Bool)
				{
					resultVal.i = leftVal.i < rightVal.i;
				}
				else
				{
					resultVal.i = leftVal.f < rightVal.f;
				}
				break;

			case SLcToken_Cmp_GT:
				resultType = SLcType_Int32;
				if(leftType == SLcType_Int32 || leftType == SLcType_Bool)
				{
					resultVal.i = leftVal.i > rightVal.i;
				}
				else
				{
					resultVal.i = leftVal.f > rightVal.f;
				}
				break;

			case SLcToken_Cmp_LE:
				resultType = SLcType_Int32;
				if(leftType == SLcType_Int32 || leftType == SLcType_Bool)
				{
					resultVal.i = leftVal.i <= rightVal.i;
				}
				else
				{
					resultVal.i = leftVal.f <= rightVal.f;
				}
				break;

			case SLcToken_Cmp_GE:
				resultType = SLcType_Int32;
				if(leftType == SLcType_Int32 || leftType == SLcType_Bool)
				{
					resultVal.i = leftVal.i >= rightVal.i;
				}
				else
				{
					resultVal.i = leftVal.f >= rightVal.f;
				}
				break;

			default:
				UUmAssert(0);
		}
	}
	else if(SLrScript_Parse_TokenStarts_Operator_Unary(inToken))
	{
		SLtExpr*			expr;
		SLtType		exprType;
		SLtValue	exprVal;

		expr = SLrExpr_Pop(inContext);

		inErrorContext->line = inToken->line;
		error = SLrExpr_Eval(inContext, inErrorContext, expr, &exprType, &exprVal);
		if(error != UUcError_None) return error;

		error = SLiExpr_UnaryOp_TypeCheck(inToken, exprType);
		if(error != UUcError_None) return error;

		switch(inToken->token)
		{
			case SLcToken_LogicalNot:
				resultType = SLcType_Bool;
				switch(exprType)
				{
					case SLcType_Int32:
						resultVal.b = !exprVal.i;
						break;
					case SLcType_Bool:
						resultVal.b = !exprVal.b;
						break;

					default:
						UUmAssert(0);
				}
				break;

			default:
				UUmAssert(0);
		}
	}
	else
	{
		UUmAssert(0);
	}

	SLrExpr_Push_Const(inContext, resultType, resultVal);

	return UUcError_None;
}

UUtUns8	SLgExpr_PrecedenceTable[SLcToken_NumOperators] =
{
/* =	*/	0,
/* +	*/	20,
/* -	*/	20,
/* and	*/	5,
/* or	*/	5,
/* !	*/	255,
/* eq	*/	10,
/* ne	*/	10,
/* <	*/	10,
/* >	*/	10,
/* <=	*/	10,
/* >=	*/	10
};

static UUtBool
SLiExpr_Op_Higher(
	SLtToken*	inTokenA,
	SLtToken*	inTokenB)
{
	return SLgExpr_PrecedenceTable[inTokenA->token] >= SLgExpr_PrecedenceTable[inTokenB->token];
}

UUtError
SLrExpr_Parse_LeftParen(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext)
{
	SLtContext_FuncState*	curFuncState;

	curFuncState = inContext->curFuncState;

	if(curFuncState->parenTOS >= SLcContext_ParenStack_MaxDepth)
	{
		SLrScript_Error_Semantic(inErrorContext, "expression too complex");
		return UUcError_Generic;
	}

	curFuncState->curParenStack = curFuncState->parenStack + curFuncState->parenTOS;

	curFuncState->parenTOS++;

	return UUcError_None;
}

UUtError
SLrExpr_Parse_RightParen(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext)
{
	SLtContext_FuncState*	curFuncState;
	SLtToken*				op;

	curFuncState = inContext->curFuncState;

	if(curFuncState->parenTOS == 0)
	{
		SLrScript_Error_Semantic(inErrorContext, "Too many )'s");
		return UUcError_Generic;
	}

	while(1)
	{
		// pop the operator off the operator stack
			op = SLiExpr_OpStack_Pop(inContext);

			if(op == NULL) break;

		// evaluate the operator and push the result on the stack
			SLiExpr_Evaluate(inContext, inErrorContext, op);
	}

	curFuncState->parenTOS--;

	curFuncState->curParenStack = curFuncState->parenStack + curFuncState->parenTOS - 1;

	return UUcError_None;
}

UUtError
SLrExpr_Parse_Op_Unary(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken)
{
	UUtError	error;

	error = SLiExpr_OpStack_Push(inContext, inErrorContext, inToken);
	if(error != UUcError_None) return error;

	return UUcError_None;
}

UUtError
SLrExpr_Parse_Identifier(
	SLtContext*	inContext,
	SLtToken*	inToken)
{
	SLrExpr_Push_Ident(inContext, inToken->lexem);

	return UUcError_None;
}

UUtError
SLrExpr_Parse_Op_Constant(
	SLtContext*	inContext,
	SLtToken*	inToken)
{
	UUtError			error;
	SLtType		type;
	SLtValue	val;

	error = SLrParse_ConstTokenToTypeAndVal(inToken, &type, &val);
	if(error != UUcError_None) return error;

	SLrExpr_Push_Const(inContext, type, val);

	return UUcError_None;
}

UUtError
SLrExpr_Parse_Op_Binary(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken)
{
	UUtError	error;
	SLtToken*	op;

	// check to see if there is a higher precedence operator on the stack
		op = SLiExpr_OpStack_TOS(inContext);

	// if so evaluate it
		if(op != NULL && SLiExpr_Op_Higher(op, inToken))
		{
			SLiExpr_OpStack_Pop(inContext);
			SLiExpr_Evaluate(inContext, inErrorContext, op);
		}

	// push this operator on the stack
		error = SLiExpr_OpStack_Push(inContext, inErrorContext, inToken);
		if(error != UUcError_None) return error;

	return UUcError_None;
}

UUtError
SLrExpr_Parse_Final(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext)
{

	return UUcError_None;
}

UUtError
SLrExpr_Parse_FunctionCall_Ident(
	SLtContext*	inContext,
	SLtToken*	inToken)
{
	SLtContext_FuncState*	curFuncState;

	curFuncState = inContext->curFuncState;

	curFuncState->funcIdent = inToken->lexem;
	curFuncState->numParams = 0;

	return UUcError_None;
}

UUtError
SLrExpr_Parse_Param(
	SLtContext*	inContext)
{
	SLtExpr*				expr;
	SLtContext_FuncState*	curFuncState;

	curFuncState = inContext->curFuncState;

	// pop it off the stack

	expr = SLrExpr_Pop(inContext);

	curFuncState->paramExprs[curFuncState->numParams++] = *expr;

	return UUcError_None;
}

UUtError
SLrExpr_Parse_Param_Old(
	SLtContext*	inContext,
	SLtToken*	inToken)
{
	SLtContext_FuncState*	curFuncState;

	curFuncState = inContext->curFuncState;

	if(SLrScript_Parse_TokenStarts_Constant(inToken))
	{
		SLrExpr_MakeFromConstant(curFuncState->paramExprs + curFuncState->numParams, inToken);
	}
	else
	{
		SLrExpr_MakeStringConstant(curFuncState->paramExprs + curFuncState->numParams, inToken->lexem);
	}
	curFuncState->numParams++;

	return UUcError_None;
}

UUtError
SLrExpr_Parse_FunctionCall_Make(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext)
{
	UUtError				error;
	SLtContext_FuncState*	curFuncState;
	SLtParameter_Actual			paramList[SLcScript_MaxNumParams];

	curFuncState = inContext->curFuncState;

	inContext->returnType = SLcType_Void;

	error =
		SLrExprListToParamList(
			inContext,
			inErrorContext,
			paramList);
	if(error != UUcError_None) return error;

	error =
		SLrParse_FuncStack_Push(
			inContext,
			inErrorContext,
			curFuncState->numParams,
			paramList,
			curFuncState->funcIdent);
	if(error != UUcError_None) return error;

	return UUcError_None;
}

UUtError
SLrExpr_ConvertValue(
	SLtErrorContext*	inErrorContext,
	SLtType		inFromType,
	SLtType		inToType,
	SLtValue	*ioValue)
{
	UUtBool	illegalConvertion = UUcFalse;

	switch(inFromType)
	{
		case SLcType_Int32:
			switch(inToType)
			{
				case SLcType_Int32:
					break;

				case SLcType_Float:
					ioValue->f = (float)ioValue->i;
					break;

				case SLcType_Bool:
					ioValue->b = (ioValue->i == 0) ? UUcFalse : UUcTrue;
					break;

				default:
					illegalConvertion = UUcTrue;
			}
			break;

		case SLcType_Float:
			switch(inToType)
			{
				case SLcType_Int32:
					ioValue->i = (UUtInt32)ioValue->f;
					break;

				case SLcType_Float:
					break;

				case SLcType_Bool:
					ioValue->b = (UUtBool)ioValue->f;
					break;

				default:
					illegalConvertion = UUcTrue;
			}
			break;

		case SLcType_Bool:
			switch(inToType)
			{
				case SLcType_Int32:
					ioValue->i = (UUtUns32)ioValue->b;
					break;

				case SLcType_Float:
					ioValue->f = (float)ioValue->b;
					break;

				case SLcType_Bool:
					break;

				default:
					illegalConvertion = UUcTrue;
			}
			break;

		case SLcType_String:
			switch(inToType)
			{
				case SLcType_String:
					break;

				default:
					illegalConvertion = UUcTrue;
			}
			break;

		default:
			illegalConvertion = UUcTrue;
	}

	if(illegalConvertion)
	{
		SLrScript_Error_Semantic(inErrorContext, "Illegal type convertion");
		return UUcError_Generic;
	}

	return UUcError_None;
}
