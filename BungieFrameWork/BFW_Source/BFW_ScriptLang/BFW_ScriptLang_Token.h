#pragma once
/*
	FILE:	BFW_ScriptLang_Token.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/
#ifndef BFW_SCRIPTLANG_TOKEN_H
#define BFW_SCRIPTLANG_TOKEN_H

UUtError
SLrScript_TextToToken(
	UUtMemory_String*	inMemoryString,
	const char*			inFileName,
	const char*			inText,
	UUtUns16			*outNumTokens,
	SLtToken*			*outTokens);	// This memory is global do not free it

void
SLrScript_Token_Print(
	FILE*		inFile,
	SLtToken*	inToken);

char*
SLrTokenCode_ConvertToString(
	SLtToken_Code	inTokenCode);


#endif
