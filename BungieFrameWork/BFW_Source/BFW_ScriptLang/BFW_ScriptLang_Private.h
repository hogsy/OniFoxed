#pragma once
/*
	FILE:	BFW_ScriptLang_Private.h

	AUTHOR:	Brent H. Pease

	CREATED: Oct 29, 1999

	PURPOSE:

	Copyright 1999

*/
#ifndef BFW_SCRIPTLANG_PRIVATE
#define BFW_SCRIPTLANG_PRIVATE

#define SLcContext_ParseStack_MaxDepth		(64)
#define SLcContext_ExprStack_MaxDepth		(16)
#define SLcContext_StatementStack_MaxDepth	(16)
#define SLcContext_FuncState_MaxDepth		(6)

#define	SLcMaxScopeLevel				(8)

#define	SLcMaxLegalValues	(8)
#define SLcScript_MaxNumParamLists	(12)

#define SLcCompletionList_Size	(5 * 1024)

typedef enum SLtToken_Code
{
	SLcToken_Assign,
	SLcToken_Plus,
	SLcToken_Minus,
	SLcToken_LogicalAnd,
	SLcToken_LogicalOr,
	SLcToken_LogicalNot,
	SLcToken_Cmp_EQ,
	SLcToken_Cmp_NE,
	SLcToken_Cmp_LT,
	SLcToken_Cmp_GT,
	SLcToken_Cmp_LE,
	SLcToken_Cmp_GE,

	SLcToken_NumOperators,

	SLcToken_LeftParen = SLcToken_NumOperators,
	SLcToken_RightParen,
	SLcToken_LeftCurley,
	SLcToken_RightCurley,
	SLcToken_LeftBracket,
	SLcToken_RightBracket,
	SLcToken_SemiColon,
	SLcToken_Colon,
	SLcToken_Comma,

	SLcToken_Bar,

	SLcToken_Identifier,
	SLcToken_Constant_Int,
	SLcToken_Constant_Bool,
	SLcToken_Constant_Float,
	SLcToken_Constant_String,

	SLcToken_Schedule,
	SLcToken_Iterate,
	SLcToken_Repeat,
	SLcToken_Return,
	SLcToken_String,
	SLcToken_Every,
	SLcToken_False,
	SLcToken_Float,
	SLcToken_Using,
	SLcToken_Sleep,
	SLcToken_Bool,
	SLcToken_Else,
	SLcToken_Fork,
	SLcToken_Func,
	SLcToken_Over,
	SLcToken_True,
	SLcToken_Void,
	SLcToken_For,
	SLcToken_Int,
	SLcToken_Var,
	SLcToken_At,
	SLcToken_If,

	SLcToken_EOF

} SLtToken_Code;

typedef struct SLtToken SLtToken;

typedef enum SLtToken_Flags
{
	SLcTokenFlag_None				= 0,
	SLcTokenFlag_PrecedesNewline	= (1 << 0)

} SLtToken_Flags;

struct SLtToken
{
	UUtUns16		flags;
	UUtUns16		line;
	UUtUns16		token;
	const char*		lexem;
};


typedef enum SLtSymbol_Kind
{
	SLcSymbolKind_Var,
	SLcSymbolKind_VarAddr,
	SLcSymbolKind_Func_Script,
	SLcSymbolKind_Func_Command,
	SLcSymbolKind_Iterator

} SLtSymbol_Kind;

typedef struct SLtSymbol_Var
{
	SLtType	type;

	SLtValue val;

} SLtSymbol_Var;

typedef struct SLtSymbol_VarAddr
{
	SLtReadWrite		readWrite;
	SLtType				type;
	void*				valAddr;
	SLtNotificationProc	noticeProc;
	char*				desc;

} SLtSymbol_VarAddr;

typedef struct SLtParameterOption
{
	SLtParameter_Formal	formalParam;

	UUtBool					repeats;

	UUtUns16				numLegalValues;
	SLtValue		legalValue[SLcMaxLegalValues];

} SLtParameterOption;

typedef struct SLtParameterList
{
	UUtBool				hasRepeating;
	UUtUns16			numParams;
	SLtParameterOption	params[SLcScript_MaxNumParams];

} SLtParameterList;

typedef struct SLtSymbol_Func_Script
{
	SLtType			returnType;
	SLtToken*				startToken;
	UUtUns16				numParams;
	SLtParameter_Formal	paramList[SLcScript_MaxNumParams];

} SLtSymbol_Func_Script;

typedef struct SLtSymbol_Func_Command
{
	SLtEngineCommand		command;
	const char*				desc;
	const char*				paramSpec;
	UUtBool					checkParams;
	SLtType			returnType;

	UUtUns16				numParamListOptions;
	SLtParameterList		paramListOptions[SLcScript_MaxNumParamLists];

} SLtSymbol_Func_Command;

typedef struct SLtSymbol_Iterator
{
	SLtScript_Iterator_GetFirst	getFirst;
	SLtScript_Iterator_GetNext	getNext;
	SLtType				type;

} SLtSymbol_Iterator;


typedef struct SLtSymbol SLtSymbol;

struct SLtSymbol
{
	const char*		name;
	const char*		fileName;
	UUtUns16		line;
	UUtUns16		scopeLevel;
	SLtSymbol_Kind	kind;

	union
	{
		SLtSymbol_Var			var;
		SLtSymbol_VarAddr		varAddr;
		SLtSymbol_Func_Script	funcScript;
		SLtSymbol_Func_Command	funcCommand;
		SLtSymbol_Iterator		iterator;

	} u;

	SLtSymbol*	prev;
	SLtSymbol*	next;

};

typedef enum SLtExpr_Kind
{
	SLcExprKind_Constant,
	SLcExprKind_Identifier

} SLtExpr_Kind;

typedef struct SLtExpr_Ident
{
	const char*	name;

} SLtExpr_Ident;

typedef struct SLtExpr_Const
{
	SLtValue	val;

} SLtExpr_Const;

typedef struct SLtExpr
{
	SLtExpr_Kind	kind;
	SLtType	type;

	union
	{
		SLtExpr_Ident	ident;
		SLtExpr_Const	constant;
	} u;

} SLtExpr;

#define SLcExprOpStack_MaxDepth			(4)
#define SLcContext_ParenStack_MaxDepth	(4)

typedef struct SLtExpr_OpStack
{
	UUtUns16	tos;
	SLtToken*	stack[SLcExprOpStack_MaxDepth];

} SLtExpr_OpStack;

typedef struct SLtContext_FuncState
{
	UUtUns16		parseTOS;
	UUtUns8			parseStack[SLcContext_ParseStack_MaxDepth];

	UUtUns16		exprTOS;
	SLtExpr			exprStack[SLcContext_ExprStack_MaxDepth];
	UUtUns16		parenLevel;

	UUtUns16			parenTOS;
	SLtExpr_OpStack		parenStack[SLcContext_ParenStack_MaxDepth];
	SLtExpr_OpStack*	curParenStack;

	SLtSymbol*		symbolList[SLcMaxScopeLevel];
	UUtUns16		scopeLevel;

	SLtToken*		curToken;

	UUtUns16		statementLevel;
	UUtBool			statementEval[SLcContext_StatementStack_MaxDepth];
	UUtUns16		statement_TargetLevel;

	const char*		funcName;
	SLtSymbol*		symbol;

	UUtUns16		numParams;
	SLtExpr			paramExprs[SLcScript_MaxNumParams];
	SLtParameter_Actual	params[SLcScript_MaxNumParams];

	const char*		funcIdent;

} SLtContext_FuncState;

struct SLtContext
{
	UUtUns16				funcTOS;
	SLtContext_FuncState	funcStack[SLcContext_FuncState_MaxDepth];

	SLtContext_FuncState*	curFuncState;

	// common state that is shared
	SLtType		returnType;
	SLtValue	returnValue;

	UUtUns32			ticksTillCompletion;
	UUtBool				stalled;

	// whereabouts the rest of the engine has a pointer to us stored
	struct SLtContext **referenceptr;

};

extern UUtMemory_Heap*		SLgDatabaseHeap;
extern UUtMemory_Pool*		SLgPermanentMem;
extern UUtMemory_String*	SLgLexemMemory;
extern UUtMemory_String*	SLgPermStringMemory;
extern BFtDebugFile*		SLgDebugFile;


#endif /* BFW_SCRIPTLANG_PRIVATE */
