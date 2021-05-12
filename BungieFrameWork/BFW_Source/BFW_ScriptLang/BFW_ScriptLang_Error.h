#pragma once
/*
	FILE:	BFW_ScriptLang_Error.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/
#ifndef BFW_SCRIPTLANG_ERROR_H
#define BFW_SCRIPTLANG_ERROR_H

void UUcArglist_Call
SLrScript_Error_Parse(
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken,
	char*				inMsg,
	...);

void UUcArglist_Call
SLrScript_Error_Lexical(
	SLtErrorContext*	inErrorContext,
	char				inChar,
	char*				inMsg,
	...);

void
SLrScript_Error_Parse_IllegalToken(
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken,
	SLtToken_Code		inTokenCode);

void UUcArglist_Call
SLrScript_Error_Semantic(
	SLtErrorContext*	inErrorContext,
	char*				inMsg,
	...);

void UUcArglist_Call
SLrDebugMessage_Log(
	char*	inMsg,
	...);
	
void
SLrDebugMessage_LogParamList(
	UUtUns16			inParameterListLength,
	SLtParameter_Actual*		inParameterList);
	

#endif

