#pragma once
/*
	FILE:	BFW_ScriptLang_Expr.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/
#ifndef BFW_SCRIPTLANG_EXPR_H
#define BFW_SCRIPTLANG_EXPR_H

UUtError
SLrExpr_Parse_LeftParen(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext);
	
UUtError
SLrExpr_Parse_RightParen(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext);
	
UUtError
SLrExpr_Parse_Op_Unary(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken);
	
UUtError
SLrExpr_Parse_Identifier(
	SLtContext*	inContext,
	SLtToken*	inToken);
	
UUtError
SLrExpr_Parse_Op_Constant(
	SLtContext*	inContext,
	SLtToken*	inToken);
	
UUtError
SLrExpr_Parse_Op_Binary(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken);

UUtError
SLrExpr_Parse_Final(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext);	

UUtError
SLrExpr_Parse_FunctionCall_Ident(
	SLtContext*	inContext,
	SLtToken*	inToken);	
	
UUtError
SLrExpr_Parse_Param(
	SLtContext*	inContext);	

UUtError
SLrExpr_Parse_Param_Old(
	SLtContext*	inContext,
	SLtToken*	inToken);

UUtError
SLrExpr_Parse_FunctionCall_Make(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext);	

UUtError
SLrExpr_ConvertValue(
	SLtErrorContext*	inErrorContext,
	SLtType		inFromType,
	SLtType		inToType,
	SLtValue	*ioValue);
	

#endif /* BFW_SCRIPTLANG_EXPR_H */