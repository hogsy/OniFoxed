/*
	FILE:	BFW_ScriptLang_Database.c

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
#include "BFW_ScriptLang_Database.h"
#include "BFW_ScriptLang_Error.h"
#include "BFW_ScriptLang_Parse.h"
#include "BFW_ScriptLang_Eval.h"

SLtSymbol*	SLgGlobalSymbolList;
SLtSymbol*	SLgPermanentSymbolList = NULL;

UUtBool		SLgPermanentSymbolList_Dirty = UUcTrue;

// CB: changed from 512 to 1024 -- october 11, 2000
#define	SLcCompletionList_MaxLength (1024)

UUtUns32		SLgCompletionList_Length = 0;
static const char*	SLgCompletionList[SLcCompletionList_MaxLength];

static SLtSymbol*
SLiSymbol_Find(
	SLtContext*		inContext,
	const char*		inName,
	SLtSymbol_Kind	inKind)
{
	SLtSymbol*	curSymbol;
	UUtInt16	curLevel;

	if(inContext != NULL)
	{
		for(curLevel = inContext->curFuncState->scopeLevel + 1; curLevel-- > 0;)
		{
			for(curSymbol = inContext->curFuncState->symbolList[curLevel]; curSymbol; curSymbol = curSymbol->next)
			{
				if(!strcmp(curSymbol->name, inName) && curSymbol->kind == inKind) return curSymbol;
			}
		}
	}

	for(curSymbol = SLgGlobalSymbolList; curSymbol; curSymbol = curSymbol->next)
	{
		if(!strcmp(curSymbol->name, inName) && curSymbol->kind == inKind) return curSymbol;
	}

	for(curSymbol = SLgPermanentSymbolList; curSymbol; curSymbol = curSymbol->next)
	{
		if(!strcmp(curSymbol->name, inName) && curSymbol->kind == inKind) return curSymbol;
	}

	return NULL;
}

static SLtSymbol*
SLiSymbol_New_AddToScope(
	SLtContext*			inContext,
	const char*			inName,
	SLtErrorContext*	inErrorContext,
	SLtSymbol_Kind		inKind)
{
	SLtSymbol*		symbol;
	UUtUns16		scopeLevel;
	UUtBool			makePerm;

    if (inContext)
		scopeLevel = inContext->curFuncState->scopeLevel;
    else
		scopeLevel = 0;
    makePerm = (inKind == SLcSymbolKind_Func_Command || inKind == SLcSymbolKind_Iterator || inKind == SLcSymbolKind_VarAddr);

	// first make sure that this name does not exist at this level
	symbol = SLiSymbol_Find(inContext, inName, inKind);
	if(symbol != NULL && symbol->scopeLevel == scopeLevel)
	{
		// error duplicate symbol
		SLrScript_Error_Semantic(inErrorContext, "Identifer \"%s\" already declared", inName);

		return NULL;
	}

	if(makePerm == UUcTrue)
	{
		SLgPermanentSymbolList_Dirty = UUcTrue;

		symbol = UUrMemory_Pool_Block_New(SLgPermanentMem, sizeof(SLtSymbol));
		if(symbol == NULL) return NULL;
	}
	else
	{
		symbol = UUrMemory_Heap_Block_New(SLgDatabaseHeap, sizeof(SLtSymbol));
		if(symbol == NULL) return NULL;
	}

	symbol->name = inName;
	if(symbol->name == NULL) return NULL;

	symbol->fileName = inErrorContext->fileName;
	if(symbol->fileName == NULL) return NULL;

	symbol->kind = inKind;
	symbol->scopeLevel = scopeLevel;
	symbol->line = inErrorContext->line;

	if(inContext == NULL)
	{

		if(makePerm == UUcTrue)
		{
			symbol->next = SLgPermanentSymbolList;
			symbol->prev = NULL;
			if(SLgPermanentSymbolList != NULL)
			{
				SLgPermanentSymbolList->prev = symbol;
			}

			SLgPermanentSymbolList = symbol;
		}
		else
		{
			symbol->next = SLgGlobalSymbolList;
			symbol->prev = NULL;
			if(SLgGlobalSymbolList != NULL)
			{
				SLgGlobalSymbolList->prev = symbol;
			}

			SLgGlobalSymbolList = symbol;
		}
	}
	else
	{
		symbol->next = inContext->curFuncState->symbolList[scopeLevel];
		symbol->prev = NULL;

		if(inContext->curFuncState->symbolList[scopeLevel] != NULL)
		{
			inContext->curFuncState->symbolList[scopeLevel]->prev = symbol;
		}

		inContext->curFuncState->symbolList[scopeLevel] = symbol;
	}

	return symbol;
}

static void
SLiSymbol_Delete(
	SLtContext*	inContext,
	SLtSymbol*	inSymbol)
{

	if(inSymbol->prev == NULL)
	{
		if(inContext == NULL)
		{
			SLgGlobalSymbolList = inSymbol->next;
		}
		else
		{
			inContext->curFuncState->symbolList[inContext->curFuncState->scopeLevel] = inSymbol->next;
		}
	}
	else
	{
		inSymbol->prev->next = inSymbol->next;
	}

	if(inSymbol->next != NULL)
	{
		inSymbol->next->prev = inSymbol->prev;
	}

	UUrMemory_Heap_Block_Delete(SLgDatabaseHeap, inSymbol);
}

void
SLrScript_Database_Internal_Reset(
	void)
{
	UUrMemory_Heap_Reset(SLgDatabaseHeap);

	SLgGlobalSymbolList = NULL;
}

UUtError
SLrScript_Database_FunctionScript_Add(
	const char*				inName,
	SLtErrorContext*		inErrorContext,
	SLtType			inReturnType,
	UUtUns16				inNumParams,
	SLtParameter_Formal*	inParamList,
	UUtUns32				inNumTokens,
	SLtToken*				inStartToken)
{
	SLtSymbol*					newSymbol;
	SLtToken*					newTokens;

	if(inNumParams > SLcScript_MaxNumParams)
	{
		SLrScript_Error_Semantic(inErrorContext, "Too many parameters, talk to brent");
		return UUcError_Generic;
	}

	newSymbol = SLiSymbol_New_AddToScope(NULL, inName, inErrorContext, SLcSymbolKind_Func_Script);
	if(newSymbol == NULL) return UUcError_Generic;	// don't let semantic errors cause asserts

	newTokens = UUrMemory_Heap_Block_New(SLgDatabaseHeap, sizeof(SLtToken) * inNumTokens);
	UUmError_ReturnOnNull(newTokens);

	UUrMemory_MoveFast(inStartToken, newTokens, sizeof(SLtToken) * inNumTokens);

	UUrMemory_MoveFast(inParamList, newSymbol->u.funcScript.paramList, sizeof(*inParamList) * inNumParams);

	newSymbol->u.funcScript.returnType = inReturnType;
	newSymbol->u.funcScript.startToken = newTokens;
	newSymbol->u.funcScript.numParams = inNumParams;

	return UUcError_None;
}


typedef enum SLtParamElemType
{
	SLcParamElemType_Formal,
	SLcParamElemType_Group

} SLtParamElemType;

typedef enum SLtGroupType
{
	SLcGroupType_And,
	SLcGroupType_Or

} SLtGroupType;

typedef struct SLtParamElem SLtParamElem;

typedef struct SLtParam_Group
{
	SLtGroupType	type;

	SLtParamElem*	a;
	SLtParamElem*	b;

} SLtParam_Group;

struct SLtParamElem
{
	SLtParamElemType	type;

	union
	{
		SLtParameterOption	paramOpt;
		SLtParam_Group		group;
	} u;
};

#define SLcDatabase_MaxParamElems	(128)
UUtUns16		SLgDatabase_NumParamElems;
SLtParamElem	SLgDatabase_TempParamElems[SLcDatabase_MaxParamElems];

static SLtParamElem*
SLiParamGroup_New(
	void)
{
	SLtParamElem*	newParamGroup;

	if(SLgDatabase_NumParamElems > SLcDatabase_MaxParamElems) return NULL;

	newParamGroup = SLgDatabase_TempParamElems + SLgDatabase_NumParamElems++;

	UUrMemory_Clear(newParamGroup, sizeof(*newParamGroup));

	newParamGroup->type = SLcParamElemType_Group;

	return newParamGroup;
}

static SLtParamElem*
SLiParamFormal_New(
	void)
{
	SLtParamElem*	newParamGroup;

	if(SLgDatabase_NumParamElems > SLcDatabase_MaxParamElems) return NULL;

	newParamGroup = SLgDatabase_TempParamElems + SLgDatabase_NumParamElems++;

	UUrMemory_Clear(newParamGroup, sizeof(*newParamGroup));

	newParamGroup->type = SLcParamElemType_Formal;

	return newParamGroup;
}

static UUtError
SLiParameterSpec_BuildFormal(
	SLtToken*			*ioCurToken,
	SLtParamElem*		*outFormal)
{
	UUtError			error;
	SLtType		type;
	SLtParamElem*		newFormal;
	SLtToken*			curToken = *ioCurToken;

	newFormal = SLiParamFormal_New();
	UUmError_ReturnOnNull(newFormal);

	*outFormal = newFormal;

	newFormal->u.paramOpt.formalParam.name = curToken->lexem;

	curToken++;

	if(curToken->token != SLcToken_Colon)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Ilegal param spec, colon expected");
	}
	curToken++;

	error = SLrParse_TokenToType((SLtToken_Code)curToken->token, &newFormal->u.paramOpt.formalParam.type);
	UUmError_ReturnOnErrorMsg(error, "Illegal type spec");

	curToken++;

	if(curToken->token == SLcToken_LeftCurley)
	{
		// parse legal value
		curToken++;

		while(1)
		{
			// parse value
			error =
				SLrParse_ConstTokenToTypeAndVal(
					curToken,
					&type,
					&newFormal->u.paramOpt.legalValue[newFormal->u.paramOpt.numLegalValues]);
			UUmError_ReturnOnErrorMsg(error, "Illegal type");
			newFormal->u.paramOpt.numLegalValues++;

			if(type != newFormal->u.paramOpt.formalParam.type)
			{
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Illegal type");
			}

			curToken++;

			if (curToken->token == SLcToken_RightCurley)
				break;

			if (curToken->token == SLcToken_Bar)
			{
				if (newFormal->u.paramOpt.numLegalValues >= SLcMaxLegalValues) {
					UUmError_ReturnOnErrorMsg(UUcError_Generic, "Number of values exceeds SLcMaxLegalValue");
				}
			}
			else
			{
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Ilegal param spec, expected | in legal values");
			}
			curToken++;
		}
		curToken++;
	}

	if(curToken->token == SLcToken_Plus)
	{
		curToken++;
		newFormal->u.paramOpt.repeats = UUcTrue;
	}
	else
	{
		newFormal->u.paramOpt.repeats = UUcFalse;
	}

	*ioCurToken = curToken;

	return UUcError_None;
}

static UUtError
SLiParameterSpec_BuildGroup_Recursive(
	SLtToken*			*ioCurToken,
	SLtParamElem*		*outGroup)
{
	UUtError			error;
	SLtToken*			curToken = *ioCurToken;
	SLtParamElem*		newGroup;
	SLtParamElem*		leftElem;

	if(curToken->token == SLcToken_RightBracket)
	{
		*outGroup = NULL;
		goto done;
	}

	// parse left side
	if(curToken->token == SLcToken_LeftBracket)
	{
		curToken++;
		error = SLiParameterSpec_BuildGroup_Recursive(&curToken, &leftElem);
		UUmError_ReturnOnError(error);

		if(curToken->token != SLcToken_RightBracket)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "expecting a ]");
		}
		curToken++;
	}
	else if(curToken->token == SLcToken_Identifier)
	{
		error = SLiParameterSpec_BuildFormal(&curToken, &leftElem);
		UUmError_ReturnOnError(error);
	}

	if(curToken->token == SLcToken_EOF || curToken->token == SLcToken_RightBracket)
	{
		*outGroup = leftElem;
		goto done;
	}

	newGroup = SLiParamGroup_New();
	UUmError_ReturnOnNull(newGroup);

	*outGroup = newGroup;

	if(curToken->token == SLcToken_Bar)
	{
		curToken++;
		newGroup->u.group.type = SLcGroupType_Or;
	}
	else
	{
		newGroup->u.group.type = SLcGroupType_And;
	}

	newGroup->u.group.a = leftElem;

	#if 0
	if(curToken->token == SLcToken_EOF || curToken->token == SLcToken_RightBracket)
	{
		*outGroup = leftElem;
		goto done;
	}
	#endif

	error = SLiParameterSpec_BuildGroup_Recursive(&curToken, &newGroup->u.group.b);
	UUmError_ReturnOnError(error);

done:

	*ioCurToken = curToken;

	return UUcError_None;
}

#define SLcDatabase_MaxParamListOptions	(8)

UUtUns16			SLgDatabase_NumListOptions;
SLtParameterList	SLgDatabase_ParamListOptions[SLcDatabase_MaxParamListOptions];

static UUtError
SLiParameterSpec_Options_Build(
	SLtParamElem*		inParamElem,
	UUtUns16			*outNumParamLists,
	SLtParameterList	*outParamLists)
{
	UUtError			error;
	UUtUns16			numLeftLists;
	SLtParameterList	leftLists[SLcScript_MaxNumParamLists];
	UUtUns16			numRightLists;
	SLtParameterList	rightLists[SLcScript_MaxNumParamLists];
	UUtUns16			itrLeft;
	UUtUns16			itrRight;
	UUtUns16			curListIndex;

	//UUrMemory_Clear(leftLists, SLcScript_MaxNumParamLists * sizeof(*leftLists));
	//UUrMemory_Clear(rightLists, SLcScript_MaxNumParamLists * sizeof(*rightLists));

	if(inParamElem == NULL)
	{
		*outNumParamLists = 0;
		return UUcError_None;
	}

	if(inParamElem->type == SLcParamElemType_Formal)
	{
		*outNumParamLists = 1;
		outParamLists[0].numParams = 1;
		outParamLists[0].params[0] = inParamElem->u.paramOpt;
		outParamLists[0].hasRepeating = inParamElem->u.paramOpt.repeats;
	}
	else if(inParamElem->type == SLcParamElemType_Group)
	{
		// build left list
		error =
			SLiParameterSpec_Options_Build(
				inParamElem->u.group.a,
				&numLeftLists,
				leftLists);
		UUmError_ReturnOnError(error);

		// build right list
		error =
			SLiParameterSpec_Options_Build(
				inParamElem->u.group.b,
				&numRightLists,
				rightLists);
		UUmError_ReturnOnError(error);

		if(inParamElem->u.group.type == SLcGroupType_Or)
		{
			// concate the left and right sides
			*outNumParamLists = numLeftLists + numRightLists;
			if(*outNumParamLists >= SLcScript_MaxNumParamLists)
			{
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "too many possible opts");
			}
			UUrMemory_MoveFast(
				leftLists,
				outParamLists,
				numLeftLists * sizeof(*outParamLists));
			UUrMemory_MoveFast(
				rightLists,
				outParamLists + numLeftLists,
				numRightLists * sizeof(*outParamLists));
			if(numLeftLists == 0 || numRightLists == 0)
			{
				outParamLists[numLeftLists + numRightLists].numParams = 0;
				outParamLists[numLeftLists + numRightLists].hasRepeating = UUcFalse;
				(*outNumParamLists)++;
			}
		}
		else if(inParamElem->u.group.type == SLcGroupType_And)
		{
			UUmAssert(numLeftLists > 0 && numRightLists > 0);

			// for each left list add all the right lists
			*outNumParamLists = numLeftLists * numRightLists;
			if(*outNumParamLists >= SLcScript_MaxNumParamLists)
			{
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "too many possible opts");
			}

			curListIndex = 0;
			for(itrLeft = 0; itrLeft < numLeftLists; itrLeft++)
			{
				for(itrRight = 0; itrRight < numRightLists; itrRight++)
				{
					outParamLists[curListIndex].numParams = leftLists[itrLeft].numParams + rightLists[itrRight].numParams;

					if(leftLists[itrLeft].hasRepeating || rightLists[itrRight].hasRepeating)
					{
						outParamLists[curListIndex].hasRepeating = UUcTrue;
					}
					else
					{
						outParamLists[curListIndex].hasRepeating = UUcFalse;
					}

					UUrMemory_MoveFast(
						leftLists[itrLeft].params,
						outParamLists[curListIndex].params,
						leftLists[itrLeft].numParams * sizeof(SLtParameterOption));

					UUrMemory_MoveFast(
						rightLists[itrRight].params,
						outParamLists[curListIndex].params + leftLists[itrLeft].numParams,
						rightLists[itrRight].numParams * sizeof(SLtParameterOption));

					curListIndex++;
				}
			}
		}
		else
		{
			UUmAssert(0);
		}
	}
	else
	{
		UUmAssert(0);
	}

	return UUcError_None;
}

static UUtError
SLiParameterSpec_Build(
	const char*			inParameterSpec,
	UUtUns16			*outNumParamListOptions,
	SLtParameterList	*outParamListOptions)
{
	UUtError			error;
	SLtToken*			tokens;
	UUtUns16			numTokens;
	SLtParamElem*		paramGroup;

	SLgDatabase_NumParamElems = 0;
	SLgDatabase_NumListOptions = 0;

	error =
		SLrScript_TextToToken(
			SLgPermStringMemory,
			"(internal)",
			inParameterSpec,
			&numTokens,
			&tokens);
	UUmError_ReturnOnError(error);

	error =
		SLiParameterSpec_BuildGroup_Recursive(
			&tokens,
			&paramGroup);
	UUmError_ReturnOnError(error);

	SLiParameterSpec_Options_Build(
		paramGroup,
		outNumParamListOptions,
		outParamListOptions);

	return UUcError_None;
}

UUtError
SLrScript_Database_FunctionEngine_Add(
	const char*			inName,
	const char*			inDesc,
	const char*			inParameterSpecification,
	SLtType		inReturnType,
	SLtEngineCommand	inCommand)
{
	UUtError		error;
	SLtSymbol*		newSymbol;
	SLtErrorContext	errorContext;

	errorContext.funcName = inName;
	errorContext.fileName = "(engine)";
	errorContext.line = 0;

	newSymbol = SLiSymbol_New_AddToScope(NULL, inName, &errorContext, SLcSymbolKind_Func_Command);
	if(newSymbol == NULL) return UUcError_Generic;	// don't let semantic errors cause asserts

	newSymbol->u.funcCommand.command = inCommand;
	newSymbol->u.funcCommand.desc = inDesc;
	newSymbol->u.funcCommand.paramSpec = inParameterSpecification;

	if(inParameterSpecification != NULL)
	{
		if(inParameterSpecification[0] == 0)
		{
			newSymbol->u.funcCommand.numParamListOptions = 1;
			newSymbol->u.funcCommand.paramListOptions[0].numParams = 0;
		}
		else
		{
			error =
				SLiParameterSpec_Build(
					inParameterSpecification,
					&newSymbol->u.funcCommand.numParamListOptions,
					newSymbol->u.funcCommand.paramListOptions);
			UUmError_ReturnOnError(error);
		}
	}
	else
	{
		newSymbol->u.funcCommand.numParamListOptions = 0;
	}

	return UUcError_None;
}

UUtError
SLrScript_Database_Var_Add(
	SLtContext*			inContext,
	const char*			inName,
	SLtErrorContext*	inErrorContext,
	SLtType		inType,
	SLtValue	inValue)
{
	SLtSymbol*	newSymbol;

	newSymbol = SLiSymbol_New_AddToScope(inContext, inName, inErrorContext, SLcSymbolKind_Var);
	if(newSymbol == NULL) return UUcError_None;	// don't let semantic errors cause asserts

	newSymbol->u.var.type = inType;
	newSymbol->u.var.val = inValue;

	return UUcError_None;
}

UUtError
SLrScript_Database_Var_Engine_Add(
	const char*			inName,
	const char*			inDescription,
	SLtErrorContext*	inErrorContext,
	SLtReadWrite		inReadWrite,
	SLtType		inType,
	SLtValue*	inValueAddr,
	SLtNotificationProc	inNotificationProc)
{
	SLtSymbol*	newSymbol;

	newSymbol = SLiSymbol_New_AddToScope(NULL, inName, inErrorContext, SLcSymbolKind_VarAddr);
	if(newSymbol == NULL) return UUcError_None;	// don't let semantic errors cause asserts

	newSymbol->u.varAddr.type = inType;
	newSymbol->u.varAddr.valAddr = inValueAddr;
	newSymbol->u.varAddr.readWrite = inReadWrite;
	newSymbol->u.varAddr.noticeProc = inNotificationProc;

	return UUcError_None;
}

UUtError
SLrScript_Database_Var_GetValue(
	SLtErrorContext*	inErrorContext,
	SLtContext*			inContext,
	const char*			inName,
	SLtType				*outType,
	SLtValue			*outvalue)
{
	UUtError			error;
	SLtSymbol*			symbol;
	SLtValue	returnVal;

	returnVal.i = 0;

	error = SLrScript_Database_Symbol_Get(inContext, inName, &symbol);

	if(error != UUcError_None)
	{
		SLrScript_Error_Semantic(inErrorContext, "could not find variable name \"%s\"", inName);
		return error;
	}

	if(symbol->kind != SLcSymbolKind_Var && symbol->kind != SLcSymbolKind_VarAddr)
	{
		SLrScript_Error_Semantic(inErrorContext, "symbol \"%s\" is not a variable (function most likely)", inName);
		return UUcError_Generic;
	}

	if(symbol->kind == SLcSymbolKind_Var)
	{
		*outType = symbol->u.var.type;
		*outvalue = symbol->u.var.val;
	}
	else
	{
		*outType = symbol->u.varAddr.type;

		switch(symbol->u.varAddr.type)
		{
			case SLcType_Int32:
				UUmAssertReadPtr(symbol->u.varAddr.valAddr, sizeof(UUtInt32));
				outvalue->i = *(UUtInt32*)symbol->u.varAddr.valAddr;
				break;
			case SLcType_Float:
				UUmAssertReadPtr(symbol->u.varAddr.valAddr, sizeof(float));
				outvalue->f = *(float*)symbol->u.varAddr.valAddr;
				break;
			case SLcType_Bool:
				UUmAssertReadPtr(symbol->u.varAddr.valAddr, sizeof(UUtBool));
				outvalue->b = *(UUtBool*)symbol->u.varAddr.valAddr;
				break;
			case SLcType_String:
				UUmAssertReadPtr(symbol->u.varAddr.valAddr, sizeof(char));
				outvalue->str = (char*)symbol->u.varAddr.valAddr;
				break;

			default:
				UUmAssert(0);
		}
	}

	return UUcError_None;
}

UUtError
SLrScript_Database_Var_SetValue(
	SLtErrorContext*	inErrorContext,
	SLtContext*			inContext,
	const char*			inName,
	SLtValue	inValue)
{
	UUtError			error;
	SLtSymbol*			symbol;

	error = SLrScript_Database_Symbol_Get(inContext, inName, &symbol);

	if(error != UUcError_None)
	{
		SLrScript_Error_Semantic(inErrorContext, "could not find variable name \"%s\"", inName);
		return error;
	}

	if(symbol->kind != SLcSymbolKind_Var && symbol->kind != SLcSymbolKind_VarAddr)
	{
		SLrScript_Error_Semantic(inErrorContext, "symbol \"%s\" is not a variable", inName);
		return UUcError_Generic;
	}

	if(symbol->kind == SLcSymbolKind_Var)
	{
		symbol->u.var.val = inValue;
	}
	else
	{
		if(symbol->u.varAddr.readWrite == SLcReadWrite_ReadOnly)
		{
			SLrScript_Error_Semantic(inErrorContext, "symbol \"%s\" is read only", inName);
			return UUcError_Generic;
		}

		switch(symbol->u.varAddr.type)
		{
			case SLcType_Int32:
				*(UUtInt32*)symbol->u.varAddr.valAddr = inValue.i;
				break;

			case SLcType_Float:
				*(float*)symbol->u.varAddr.valAddr = inValue.f;
				break;

			case SLcType_Bool:
				*(UUtBool*)symbol->u.varAddr.valAddr = inValue.b;
				break;

			case SLcType_String:
				UUrString_Copy(symbol->u.varAddr.valAddr, inValue.str, SLcScript_String_MaxLength);
				break;

			default:
				UUmAssert(0);
		}

		if(symbol->u.varAddr.noticeProc != NULL)
		{
			symbol->u.varAddr.noticeProc();
		}
	}

	return UUcError_None;
}

UUtError
SLrScript_Database_Iterator_Add(
	const char*					inIteratorName,
	SLtType				inVariableType,
	SLtScript_Iterator_GetFirst	inGetFirstFunc,
	SLtScript_Iterator_GetNext	inGetNextFunc)
{
	SLtSymbol*	newSymbol;
	SLtErrorContext	errorContext;

	errorContext.funcName = inIteratorName;
	errorContext.fileName = "(engine)";
	errorContext.line = 0;

	newSymbol = SLiSymbol_New_AddToScope(NULL, inIteratorName, &errorContext, SLcSymbolKind_Iterator);
	if(newSymbol == NULL) return UUcError_None;	// don't let semantic errors cause asserts

	newSymbol->u.iterator.type = inVariableType;
	newSymbol->u.iterator.getFirst = inGetFirstFunc;
	newSymbol->u.iterator.getNext = inGetNextFunc;

	return UUcError_None;
}

void
SLrScript_Database_Scope_Enter(
	SLtContext*			inContext)
{
	inContext->curFuncState->scopeLevel++;
}

void
SLrScript_Database_Scope_Leave(
	SLtContext*			inContext)
{
	SLtSymbol*	curSymbol;
	SLtSymbol*	nextSymbol;

	UUmAssert(inContext->curFuncState->scopeLevel > 0);

	curSymbol = inContext->curFuncState->symbolList[inContext->curFuncState->scopeLevel];

	while(curSymbol)
	{
		nextSymbol = curSymbol->next;

		SLiSymbol_Delete(inContext, curSymbol);

		curSymbol = nextSymbol;
	}

	inContext->curFuncState->symbolList[inContext->curFuncState->scopeLevel] = NULL;

	inContext->curFuncState->scopeLevel--;
}

UUtError
SLrScript_Database_Symbol_Get(
	SLtContext*		inContext,
	const char*		inName,
	SLtSymbol*		*outSymbol)
{
	SLtSymbol*	curSymbol;
	UUtInt16	curLevel;

	*outSymbol = NULL;
	if (inName == NULL) {
		return UUcError_Generic;
	}

	if(inContext != NULL && inContext->curFuncState != NULL)
	{
		for(curLevel = inContext->curFuncState->scopeLevel + 1; curLevel-- > 0;)
		{
			for(curSymbol = inContext->curFuncState->symbolList[curLevel]; curSymbol; curSymbol = curSymbol->next)
			{
				if(!strcmp(curSymbol->name, inName))
				{
					*outSymbol = curSymbol;
					return UUcError_None;
				}
			}
		}
	}

	for(curSymbol = SLgGlobalSymbolList; curSymbol; curSymbol = curSymbol->next)
	{
		if(!strcmp(curSymbol->name, inName))
		{
			*outSymbol = curSymbol;
			return UUcError_None;
		}
	}

	for(curSymbol = SLgPermanentSymbolList; curSymbol; curSymbol = curSymbol->next)
	{
		if(!strcmp(curSymbol->name, inName))
		{
			*outSymbol = curSymbol;
			return UUcError_None;
		}
	}

	return UUcError_Generic;
}

// possible kinds of type-cast results
enum {
	SLcParameter_Match,
	SLcParameter_Promote,
	SLcParameter_Mismatch
};

static void
SLiParameterDefault(
	SLtType					inType,
	SLtParameter_Actual *	outParameter)
{
	outParameter->type = inType;

	switch(inType) {
		case SLcType_Int32:
			outParameter->val.i = 0;
			return;

		case SLcType_String:
			outParameter->val.str = "";
			return;

		case SLcType_Float:
			outParameter->val.f = 0.0f;
			return;

		case SLcType_Bool:
			outParameter->val.b = UUcFalse;
			return;
	}
}

static UUtUns32
SLiParameterCast(
	SLtType					inFormalType,
	SLtParameter_Actual *	ioParameter)
{
	if (ioParameter->type == inFormalType) {
		return SLcParameter_Match;
	}

	switch(inFormalType) {
		case SLcType_Int32:
			if (ioParameter->type == SLcType_Bool) {
				ioParameter->type = SLcType_Int32;
				ioParameter->val.i = (ioParameter->val.b) ? 1 : 0;
				return SLcParameter_Promote;
			} else {
				return SLcParameter_Mismatch;
			}

		case SLcType_String:
			// FIXME: we should eventually be able to promote float, bool or int to string once we work
			// out how to find storage for the string.
			return SLcParameter_Mismatch;

		case SLcType_Float:
			if (ioParameter->type == SLcType_Int32) {
				ioParameter->type = SLcType_Float;
				ioParameter->val.f = (float) ioParameter->val.i;
				return SLcParameter_Promote;
			} else {
				return SLcParameter_Mismatch;
			}

		case SLcType_Bool:
			if (ioParameter->type == SLcType_Int32) {
				ioParameter->type = SLcType_Bool;
				ioParameter->val.b = (ioParameter->val.i == 0) ? UUcFalse : UUcTrue;
				return SLcParameter_Promote;
			} else {
				return SLcParameter_Mismatch;
			}

		default:
			return SLcParameter_Mismatch;
	}
}

UUtError
SLrScript_ParameterCheckAndPromote(
	SLtErrorContext*		inErrorContext,
	const char*				inScriptName,
	SLtSymbol_Func_Script*	inScript,
	UUtUns16*				ioGivenParams_Num,
	SLtParameter_Actual*	ioGivenParams_List)
{
	UUtUns16			itr;
	SLtParameter_Formal*formalParam;
	UUtUns32			paramcheck;

	if (*ioGivenParams_Num > SLcScript_MaxNumParams) {
		SLrScript_Error_Semantic(inErrorContext, "\"%s\": call exceeds max script parameters (%d)", inScriptName, SLcScript_MaxNumParams);
		return UUcError_Generic;
	}
	if (inScript->numParams > SLcScript_MaxNumParams) {
		SLrScript_Error_Semantic(inErrorContext, "\"%s\": parameter list length exceeds max (%d)", inScriptName, SLcScript_MaxNumParams);
		return UUcError_Generic;
	}

	// compare the parameter lists
	for (itr = 0, formalParam = inScript->paramList, itr = 0; itr < inScript->numParams; itr++, formalParam++) {

		if (itr >= *ioGivenParams_Num) {
			// no input parameter exists to match this formal parameter; we must add a default value
			SLiParameterDefault(formalParam->type, &ioGivenParams_List[itr]);
			continue;
		}

		// attempt to match the input parameter to the formal parameter
		paramcheck = SLiParameterCast(formalParam->type, &ioGivenParams_List[itr]);
		if (paramcheck == SLcParameter_Mismatch) {
			char parambuf[256];

			switch(ioGivenParams_List[itr].type) {
				case SLcType_Int32:
					sprintf(parambuf, "%s (%d)", SLcTypeName[SLcType_Int32], ioGivenParams_List[itr].val.i);
					break;

				case SLcType_Float:
					sprintf(parambuf, "%s (%f)", SLcTypeName[SLcType_Float], ioGivenParams_List[itr].val.f);
					break;

				case SLcType_Bool:
					sprintf(parambuf, "%s (%s)", SLcTypeName[SLcType_Bool], ioGivenParams_List[itr].val.b ? "true" : "false");
					break;

				case SLcType_String:
					sprintf(parambuf, "%s (%s)", SLcTypeName[SLcType_String], ioGivenParams_List[itr].val.str);
					break;

				default:
					sprintf(parambuf, "unknown-val-type %d", ioGivenParams_List[itr].type);
					break;
			}

			// we can't promote the given data type to match this formal parameter, bail out
			SLrScript_Error_Semantic(inErrorContext, "\"%s\": can't convert parameter %d from %s to %s %s", inScriptName,
									itr + 1, parambuf, SLcTypeName[formalParam->type], formalParam->name);
			return UUcError_Generic;
		}
	}

	// we must have expanded or contracted the parameter list by the right amount by now, set its length
	*ioGivenParams_Num = inScript->numParams;
	return UUcError_None;
}

UUtError
SLrCommand_ParameterCheckAndPromote(
	SLtErrorContext*		inErrorContext,
	const char*				inCommandName,
	SLtSymbol_Func_Command*	inCommand,
	UUtUns16*				ioGivenParams_Num,
	SLtParameter_Actual*	ioGivenParams_List)
{
	UUtUns16			itrList;
	UUtUns16			itrFormal;
	UUtUns16			itrGiven;
	UUtUns16			itrLegalValue;
	SLtParameterList*	curParamList;
	SLtParameterList*	choiceParamList;
	SLtParameterOption*	curParamOpt;
	UUtUns32			paramcheck;
	UUtBool				matched_parameter, current_fail, new_choice;
	UUtUns16			choice_discarded, choice_added, choice_promoted;
	UUtUns16			current_discarded, current_added, current_promoted;
	SLtParameter_Actual	current_parameters[SLcScript_MaxNumParams], final_parameters[SLcScript_MaxNumParams];

	if (*ioGivenParams_Num > SLcScript_MaxNumParams) {
		SLrScript_Error_Semantic(inErrorContext, "\"%s\": exceeded max command parameters (%d)", inCommandName, SLcScript_MaxNumParams);
		return UUcError_Generic;
	}

	// no successful choices found yet
	choiceParamList = NULL;

	for(itrList = 0, curParamList = inCommand->paramListOptions; itrList < inCommand->numParamListOptions; itrList++, curParamList++) {
		// assess the current candidate parameter list
		current_fail = UUcFalse;
		current_discarded = current_added = current_promoted = 0;

		for(itrFormal = 0, curParamOpt = curParamList->params, itrGiven = 0; itrFormal < curParamList->numParams; itrFormal++, curParamOpt++) {
			matched_parameter = UUcFalse;
			do {
				// attempt to match the input parameter to the formal parameter

				if (itrGiven >= *ioGivenParams_Num) {
					// no input parameter exists to match this formal parameter; we must add a default value
					current_added++;
					matched_parameter = UUcTrue;
					break;
				}

				current_parameters[itrGiven] = ioGivenParams_List[itrGiven];
				paramcheck = SLiParameterCast(curParamOpt->formalParam.type, &current_parameters[itrGiven]);
				if (paramcheck == SLcParameter_Mismatch) {
					// we can't promote the given data type to match this formal parameter, bail out
					break;

				} else if (paramcheck == SLcParameter_Promote) {
					if (matched_parameter) {
						// we've already matched this formal parameter at least, don't promote further given
						// parameters just to continue matching the repeated parameter
						break;
					} else {
						// we have to promote to match this parameter, record this
						current_promoted++;
					}
				}

				if (curParamOpt->numLegalValues > 0) {
					// try to find a legal value that matches
					for(itrLegalValue = 0; itrLegalValue < curParamOpt->numLegalValues; itrLegalValue++) {
						if(SLrExpr_CompareValues_Eq(curParamOpt->formalParam.type, curParamOpt->legalValue[itrLegalValue], current_parameters[itrGiven].val)) {
							break;
						}
					}

					if (itrLegalValue >= curParamOpt->numLegalValues) {
						// no legal values, we can't match these parameters
						break;
					}
				}

				// the parameter matches!
				matched_parameter = UUcTrue;
				itrGiven++;

			} while (curParamOpt->repeats);

			if (!matched_parameter) {
				// couldn't find anything to match this parameter, fail the current parameter list
				current_fail = UUcTrue;
				break;
			}
		}

		// if we didn't end up processing any surplus parameters, record so
		current_discarded += *ioGivenParams_Num - itrGiven;

		// if this parameter list is a better match than the previous, use it
		if (current_fail) {
			new_choice = UUcFalse;

		} else if (choiceParamList == NULL) {
			new_choice = UUcTrue;

		} else if (current_discarded < choice_discarded) {
			new_choice = UUcTrue;

		} else if (current_discarded > choice_discarded) {
			new_choice = UUcFalse;

		} else if (current_added < choice_added) {
			new_choice = UUcTrue;

		} else if (current_added > choice_added) {
			new_choice = UUcFalse;

		} else if (current_promoted < choice_promoted) {
			new_choice = UUcTrue;

		} else if (current_promoted > choice_promoted) {
			new_choice = UUcFalse;

		} else {
			// use the first choice if it matched identically to this one
			new_choice = UUcFalse;
		}

		if (new_choice) {
			choiceParamList = curParamList;
			choice_discarded = current_discarded;
			choice_added = current_added;
			choice_promoted = current_promoted;
			UUrMemory_MoveFast(current_parameters, final_parameters, *ioGivenParams_Num * sizeof(SLtParameter_Actual));
		}
	}

	if (choiceParamList == NULL) {
		// the given set of parameters didn't match any of the formal parameter lists
		SLrScript_Error_Semantic(inErrorContext, "\"%s\": parameter list does not match: %s", inCommandName, inCommand->paramSpec);
		return UUcError_Generic;
	} else {
		if (choice_added > 0) {
			UUmAssert(choice_discarded == 0);

			// we had to add default parameters to fill up the formal parameter list
			for (itrList = choiceParamList->numParams - choice_added, itrFormal = 0; itrFormal < choice_added; itrFormal++, itrList++) {
				SLiParameterDefault(choiceParamList->params[itrList].formalParam.type, &final_parameters[itrList]);
			}
		}

		// we must have expanded or contracted the parameter list by the right amount by now, set its length
		*ioGivenParams_Num = choiceParamList->numParams;

		// copy the promoted / added to / discarded from parameter list
		UUrMemory_MoveFast(final_parameters, ioGivenParams_List, *ioGivenParams_Num * sizeof(SLtParameter_Actual));

		return UUcError_None;
	}
}

void
SLrScript_Database_ConsoleCompletionList_Get(
	UUtUns32		*outNumNames,
	char**	*outList)
{
	if(SLgPermanentSymbolList_Dirty)
	{
		UUtError				error;
		SLtSymbol*				curSymbol;
		AUtSharedStringArray*	sortedStringArray;
		UUtUns32				numEntries;
		UUtUns32*				sortedIndexList;
		AUtSharedString*		list;
		UUtUns32				itr;

		sortedStringArray = AUrSharedStringArray_New();

		for(curSymbol = SLgPermanentSymbolList; curSymbol; curSymbol = curSymbol->next)
		{
			UUmAssert(NULL != curSymbol->name);

			error =
				AUrSharedStringArray_AddString(
					sortedStringArray,
					curSymbol->name,
					(UUtInt32)curSymbol,
					NULL);
			UUmAssert(UUcError_None == error);
		}

		for(curSymbol = SLgGlobalSymbolList; curSymbol; curSymbol = curSymbol->next)
		{
			UUmAssert(NULL != curSymbol->name);

			error = AUrSharedStringArray_AddString(
					sortedStringArray,
					curSymbol->name,
					(UUtInt32)curSymbol,
					NULL);
			UUmAssert(UUcError_None == error);
		}

		numEntries = AUrSharedStringArray_GetNum(sortedStringArray);
		sortedIndexList = AUrSharedStringArray_GetSortedIndexList(sortedStringArray);
		list = AUrSharedStringArray_GetList(sortedStringArray);

		if (numEntries > SLcCompletionList_MaxLength) {
			COrConsole_Printf("exceeded scripting command completion limit of %d", SLcCompletionList_MaxLength);
			numEntries = SLcCompletionList_MaxLength;
		}

		SLgCompletionList_Length = numEntries;

		for(itr = 0; itr < numEntries; itr++)
		{
			const char *completion_list_string = ((SLtSymbol*)list[sortedIndexList[itr]].data)->name;

			UUmAssert(completion_list_string != NULL);

			SLgCompletionList[itr] = completion_list_string;
		}

		AUrSharedStringArray_Delete(sortedStringArray);

		SLgPermanentSymbolList_Dirty = UUcFalse;
	}

	*outNumNames = SLgCompletionList_Length;
	*outList = (char **)SLgCompletionList;

	return;
}

UUtError
SLrDatabase_Command_DumpAll(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	SLtSymbol*	curSymbol;
	FILE*		file;

	file = fopen("script_commands.txt", "w");

	for(curSymbol = SLgPermanentSymbolList; curSymbol; curSymbol = curSymbol->next)
	{
		fprintf(file, "============================================\n");
		switch(curSymbol->kind)
		{
			case SLcSymbolKind_Var:
				break;

			case SLcSymbolKind_VarAddr:
				fprintf(file, "var: %s\n", curSymbol->name);
				break;

			case SLcSymbolKind_Func_Script:
				break;

			case SLcSymbolKind_Func_Command:
				fprintf(file, "command: %s\n", curSymbol->name);
				fprintf(file, "desc: %s\n", curSymbol->u.funcCommand.desc);
				fprintf(file, "param: %s\n", curSymbol->u.funcCommand.paramSpec);
				break;

			case SLcSymbolKind_Iterator:
				break;
		}
	}

	fclose(file);

	return UUcError_None;
}

UUtBool
SLrDatabase_IsFunctionCall(
	const char*	inName)
{
	UUtError	error;
	SLtSymbol*	symbol;

	error =
		SLrScript_Database_Symbol_Get(
			NULL,
			inName,
			&symbol);
	if(error != UUcError_None) return UUcFalse;

	if(symbol->kind == SLcSymbolKind_Func_Script ||
		symbol->kind == SLcSymbolKind_Func_Command) return UUcTrue;

	return UUcFalse;
}

