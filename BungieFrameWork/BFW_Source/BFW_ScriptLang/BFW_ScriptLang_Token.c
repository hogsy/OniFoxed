/*
	FILE:	BFW_ScriptLang_Token.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/

#include <ctype.h>

#include "BFW.h"
#include "BFW_TemplateManager.h"

#include "BFW_ScriptLang.h"
#include "BFW_ScriptLang_Private.h"
#include "BFW_ScriptLang_Token.h"
#include "BFW_ScriptLang_Error.h"

#define SLcMaxTokens			(8 * 1024)
#define SLcLexem_Buffer_MaxSize	(256)

SLtToken	SLgTokenArray[SLcMaxTokens];

char		SLgLexemBuffer[SLcLexem_Buffer_MaxSize];
char*		SLgLexem_CurPtr;
UUtUns16	SLgLexemBuffer_Len;
UUtUns16	SLgLineNum;

SLtToken*	SLgLastToken = NULL;

SLtErrorContext	SLgToken_ErrorContext;

static void
SLiLexem_Start(
	void)
{
	SLgLexem_CurPtr = SLgLexemBuffer;
	SLgLexemBuffer_Len = 0;
}

static void
SLiLexem_AddChar(
	char	inC)
{
	UUmAssertReadPtr(SLgLexem_CurPtr, sizeof(char));
	
	if(SLgLexemBuffer_Len < SLcLexem_Buffer_MaxSize)
	{
		*SLgLexem_CurPtr++ = inC;
		SLgLexemBuffer_Len++;
	}
}

static void
SLiLexem_End(
	void)
{
	UUmAssertReadPtr(SLgLexem_CurPtr, sizeof(char));
	*SLgLexem_CurPtr = 0;
}

static UUtBool
SLiLexem_IsReservedWord(
	SLtToken_Code	*outReservedWord)
{
	if(SLgLexemBuffer_Len > 8 || SLgLexemBuffer_Len < 2) return UUcFalse;
	
	switch(SLgLexemBuffer_Len)
	{
		case 2:
			switch(SLgLexemBuffer[0])
			{
				case 'a':
					if(!strncmp(SLgLexemBuffer, "at", 2))
					{
						*outReservedWord = SLcToken_At;
						return UUcTrue;
					}
					break;
				case 'e':
					if(!strncmp(SLgLexemBuffer, "eq", 2))
					{
						*outReservedWord = SLcToken_Cmp_EQ;
						return UUcTrue;
					}
					break;
				case 'i':
					if(!strncmp(SLgLexemBuffer, "if", 2))
					{
						*outReservedWord = SLcToken_If;
						return UUcTrue;
					}
					break;
				case 'n':
					if(!strncmp(SLgLexemBuffer, "ne", 2))
					{
						*outReservedWord = SLcToken_Cmp_NE;
						return UUcTrue;
					}
					break;
				case 'o':
					if(!strncmp(SLgLexemBuffer, "or", 2))
					{
						*outReservedWord = SLcToken_LogicalOr;
						return UUcTrue;
					}
					break;
			}
			break;
			
		case 3:
			switch(SLgLexemBuffer[0])
			{
				case 'a':
					if(!strncmp(SLgLexemBuffer, "and", 3))
					{
						*outReservedWord = SLcToken_LogicalAnd;
						return UUcTrue;
					}
					break;
				case 'f':
					if(!strncmp(SLgLexemBuffer, "for", 3))
					{
						*outReservedWord = SLcToken_For;
						return UUcTrue;
					}
					break;
				case 'i':
					if(!strncmp(SLgLexemBuffer, "int", 3))
					{
						*outReservedWord = SLcToken_Int;
						return UUcTrue;
					}
					break;
				case 'v':
					if(!strncmp(SLgLexemBuffer, "var", 3))
					{
						*outReservedWord = SLcToken_Var;
						return UUcTrue;
					}
					break;
			}
			break;
			
		case 4:
			switch(SLgLexemBuffer[0])
			{
				case 'b':
					if(!strncmp(SLgLexemBuffer, "bool", 4))
					{
						*outReservedWord = SLcToken_Bool;
						return UUcTrue;
					}
					break;
				case 'e':
					if(!strncmp(SLgLexemBuffer, "else", 4))
					{
						*outReservedWord = SLcToken_Else;
						return UUcTrue;
					}
					break;
				case 'f':
					if(!strncmp(SLgLexemBuffer, "func", 4))
					{
						*outReservedWord = SLcToken_Func;
						return UUcTrue;
					}
					else if(!strncmp(SLgLexemBuffer, "fork", 4))
					{
						*outReservedWord = SLcToken_Fork;
						return UUcTrue;
					}
					break;
				case 'o':
					if(!strncmp(SLgLexemBuffer, "over", 4))
					{
						*outReservedWord = SLcToken_Over;
						return UUcTrue;
					}
					break;
				case 't':
					if(!strncmp(SLgLexemBuffer, "true", 4))
					{
						*outReservedWord = SLcToken_True;
						return UUcTrue;
					}
					break;
				case 'v':
					if(!strncmp(SLgLexemBuffer, "void", 4))
					{
						*outReservedWord = SLcToken_Void;
						return UUcTrue;
					}
					break;
			}
			break;
			
		case 5:
			switch(SLgLexemBuffer[0])
			{
				case 'e':
					if(!strncmp(SLgLexemBuffer, "every", 5))
					{
						*outReservedWord = SLcToken_Every;
						return UUcTrue;
					}
					break;
				case 'f':
					if(!strncmp(SLgLexemBuffer, "false", 5))
					{
						*outReservedWord = SLcToken_False;
						return UUcTrue;
					}
					else if(!strncmp(SLgLexemBuffer, "float", 5))
					{
						*outReservedWord = SLcToken_Float;
						return UUcTrue;
					}
					break;
				case 'u':
					if(!strncmp(SLgLexemBuffer, "using", 5))
					{
						*outReservedWord = SLcToken_Using;
						return UUcTrue;
					}
					break;
				case 's':
					if(!strncmp(SLgLexemBuffer, "sleep", 5))
					{
						*outReservedWord = SLcToken_Sleep;
						return UUcTrue;
					}
					break;
			}
			break;
			
		case 6:
			switch(SLgLexemBuffer[0])
			{
				case 'r':
					if(!strncmp(SLgLexemBuffer, "repeat", 6))
					{
						*outReservedWord = SLcToken_Repeat;
						return UUcTrue;
					}
					if(!strncmp(SLgLexemBuffer, "return", 6))
					{
						*outReservedWord = SLcToken_Return;
						return UUcTrue;
					}
					break;
				case 's':
					if(!strncmp(SLgLexemBuffer, "string", 6))
					{
						*outReservedWord = SLcToken_String;
						return UUcTrue;
					}
					break;
			}
			break;
			
		case 7:
			if(!strncmp(SLgLexemBuffer, "iterate", 7))
			{
				*outReservedWord = SLcToken_Iterate;
				return UUcTrue;
			}
			break;
			
		case 8:
			if(!strncmp(SLgLexemBuffer, "schedule", 8))
			{
				*outReservedWord = SLcToken_Schedule;
				return UUcTrue;
			}
			break;
	}
	
	return UUcFalse;
}

static UUtError
SLiToken_Make(
	UUtMemory_String*	inMemoryString,
	const char*			*ioCP,
	UUtBool				*outAllocationFailed,
	SLtToken			*outCurToken)
{
	const char*	cp = *ioCP;
	char	c;
	SLtToken_Code	reservedWord;
	SLtToken_Code	constantType;
	
	*outAllocationFailed = UUcFalse;
	outCurToken->lexem = NULL;
	outCurToken->line = SLgLineNum;
	outCurToken->flags = SLcTokenFlag_None;
	
startOver:
	SLiLexem_Start();
	
	c = *cp++;
	
	if(c == 0)
	{
		outCurToken->token = SLcToken_EOF;
		if(SLgLastToken != NULL) SLgLastToken->flags |= SLcTokenFlag_PrecedesNewline;
	}
	
	else if(c == '\n')
	{
		if(*cp == '\r') cp++;
		SLgLineNum++;
		
		if(SLgLastToken != NULL) SLgLastToken->flags |= SLcTokenFlag_PrecedesNewline;
		
		goto startOver;
	}
	else if(c == '\r')
	{
		if(*cp == '\n') cp++;
		SLgLineNum++;
		
		if(SLgLastToken != NULL) SLgLastToken->flags |= SLcTokenFlag_PrecedesNewline;
		
		goto startOver;
	}
	
	else if(isspace(c))
	{
		while(isspace(c) && c != '\n' && c != '\r' && c != 0)
		{
			c = *cp++;
		}
		cp--;
		
		goto startOver;
	}
	else if(c == 'f' && isdigit(cp[0]))
	{
		c = *cp++;
		
		constantType = SLcToken_Constant_Int;
		
		while((isdigit(c) || (c == '.')) && c != 0)
		{
			if(c == '.') constantType = SLcToken_Constant_Float;
			SLiLexem_AddChar(c);
			c = *cp++;
		}
		cp--;
		
		outCurToken->token = constantType;
	}
	else if(isalpha(c) || c == '_')
	{
		while((isalnum(c) || c == '_') && c != 0)
		{
			SLiLexem_AddChar(c);
			c = *cp++;
		}
		cp--;
		
		SLiLexem_End();
		
		if(SLiLexem_IsReservedWord(&reservedWord))
		{
			if(reservedWord == SLcToken_True)
			{
				outCurToken->token = SLcToken_Constant_Bool;
				SLiLexem_Start();
				SLiLexem_AddChar('1');
			}
			else if(reservedWord == SLcToken_False)
			{
				outCurToken->token = SLcToken_Constant_Bool;
				SLiLexem_Start();
				SLiLexem_AddChar('0');
			}
			else
			{
				outCurToken->token = reservedWord;
			}
		}
		else
		{
			outCurToken->token = SLcToken_Identifier;
		}
	}
	
	else if(isdigit(c) || (c == '.') || ((c == '-') && isdigit(cp[0])))
	{
		constantType = SLcToken_Constant_Int;
		
		while((isdigit(c) || (c == '.') || (c == '-')) && c != 0)
		{
			if(c == '.') constantType = SLcToken_Constant_Float;
			SLiLexem_AddChar(c);
			c = *cp++;
		}
		cp--;
		
		outCurToken->token = constantType;
	}
	
	else
	{
		switch(c)
		{
			case '!':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_LogicalNot;
				break;
			
			case '\"':
				c = *cp++;
				while(c != '\"' && c != 0 && c != '\n' && c != '\r')
				{
					SLiLexem_AddChar(c);
					c = *cp++;
				}
				if(c != '\"')
				{
					SLgToken_ErrorContext.line = SLgLineNum;
					SLrScript_Error_Lexical(
						&SLgToken_ErrorContext,
						c,
						"Expecting a \" to terminate the string");
				}
				outCurToken->token = SLcToken_Constant_String;
				break;
			
			case '(':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_LeftParen;
				break;
				
			case ')':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_RightParen;
				break;
				
			case '[':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_LeftBracket;
				break;
				
			case ']':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_RightBracket;
				break;
				
			case '+':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_Plus;
				break;
				
			case ',':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_Comma;
				break;
				
			case '-':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_Minus;
				break;
				
			case ';':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_SemiColon;
				break;
				
			case ':':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_Colon;
				break;
				
			case '=':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_Assign;
				break;
				
			case '{':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_LeftCurley;
				break;
				
			case '}':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_RightCurley;
				break;
			
			case '|':
				SLiLexem_AddChar(c);
				outCurToken->token = SLcToken_Bar;
				break;
			
			case '<':
				SLiLexem_AddChar(c);
				
				if(cp[0] == '=')
				{
					SLiLexem_AddChar(c);
					outCurToken->token = SLcToken_Cmp_LE;
					cp++;
				}
				else
				{
					outCurToken->token = SLcToken_Cmp_LT;
				}
				break;
			
			case '>':
				SLiLexem_AddChar(c);
				
				if(cp[0] == '=')
				{
					SLiLexem_AddChar(c);
					outCurToken->token = SLcToken_Cmp_GE;
					cp++;
				}
				else
				{
					outCurToken->token = SLcToken_Cmp_GT;
				}
				break;
			
			
			case '#':
				
				while(1)
				{
					c = *cp++;
					
					if(c == 0) break;
					
					if(c == '\n')
					{
						if(*cp == '\r') cp++;
						break;
					}
					if(c == '\r')
					{
						if(*cp == '\n') cp++;
						break;
					}
					
				}
				if(c == 0) cp--;
				SLgLineNum++;
				goto startOver;
				
			default:
				SLgToken_ErrorContext.line = SLgLineNum;
				SLrScript_Error_Lexical(
					&SLgToken_ErrorContext,
					c,
					"Illegal character");
				
				return UUcError_Generic;
		}
	}
	
	SLiLexem_End();

	if(*SLgLexemBuffer != 0)
	{
		outCurToken->lexem = UUrMemory_String_GetStr(inMemoryString, SLgLexemBuffer);
	}
	
	*ioCP = cp;
	
	SLgLastToken = outCurToken;
	
	return UUcError_None;
}

UUtError
SLrScript_TextToToken(
	UUtMemory_String*	inMemoryString,
	const char*			inFileName,
	const char*			inText,
	UUtUns16			*outNumTokens,
	SLtToken*			*outTokens)	// This memory is allocated, you must free it
{
	const char*	cp = inText;
	SLtToken*	ct = SLgTokenArray;
	UUtBool allocation_failed;
	UUtUns32 string_size;

	UUtUns16	numTokens = 0;
	
	SLgLineNum = 1;
	
	SLgToken_ErrorContext.funcName = "(none)";
	SLgToken_ErrorContext.fileName = inFileName;
	
	SLgLastToken = NULL;
	
	while(1)
	{
		UUmAssert(numTokens < SLcMaxTokens);

		if (numTokens >= SLcMaxTokens) {
			UUmError_ReturnOnErrorMsgP(UUcError_Generic, "script file %s had more then %d tokens", (UUtUns32) inFileName, SLcMaxTokens, 0);
		}
		
		SLiToken_Make(inMemoryString, &cp, &allocation_failed, ct);

		if (allocation_failed) {
			string_size = UUrMemory_String_GetSize(inMemoryString);
			UUmError_ReturnOnErrorMsgP(UUcError_Generic, "cannot parse text from file %s, exhausted script parsing memory (%d bytes)",
										(UUtUns32) inFileName, string_size, 0);
		}

		if(ct->token == SLcToken_EOF) break;
		
		numTokens++;
		ct++;
	}
	
	*outNumTokens = numTokens;
	*outTokens = SLgTokenArray;
	
	return UUcError_None;
}

void
SLrScript_Token_Print(
	FILE*		inFile,
	SLtToken*	inToken)
{
	fprintf(inFile, "%d: %s\n", inToken->line, inToken->lexem);
}

char*
SLrTokenCode_ConvertToString(
	SLtToken_Code	inTokenCode)
{
	switch(inTokenCode)
	{
		case SLcToken_LeftParen:
			return "(";
				
		case SLcToken_RightParen:
			return ")";
				
		case SLcToken_LeftCurley:
			return "{";
				
		case SLcToken_RightCurley:
			return "}";
				
		case SLcToken_SemiColon:
			return ";";
				
		case SLcToken_Comma:
			return ",";
				
		case SLcToken_Assign:
			return "=";
				
		case SLcToken_Plus:
			return "+";
				
		case SLcToken_Minus:
			return "-";
				
		case SLcToken_LogicalAnd:
			return "and";
				
		case SLcToken_LogicalOr:
			return "or";
				
		case SLcToken_LogicalNot:
			return "!";
				
		case SLcToken_Cmp_EQ:
			return "eq";
				
		case SLcToken_Cmp_NE:
			return "ne";
				
		case SLcToken_Cmp_LT:
			return "<";
				
		case SLcToken_Cmp_GT:
			return ">";
				
		case SLcToken_Cmp_LE:
			return "<=";
		
		case SLcToken_Cmp_GE:
			return ">=";
				
		case SLcToken_Identifier:
			return "identifier";
				
		case SLcToken_Constant_Int:
			return "int";
				
		case SLcToken_Constant_Float:
			return "float";
				
		case SLcToken_Constant_String:
			return "string";
				
		case SLcToken_Schedule:
			return "schedule";
				
		case SLcToken_Iterate:
			return "iterate";
				
		case SLcToken_Repeat:
			return "repeat";
				
		case SLcToken_Return:
			return "return";
				
		case SLcToken_String:
			return "string";
				
		case SLcToken_Every:
			return "every";
				
		case SLcToken_Float:
			return "float";
			
		case SLcToken_Using:
			return "using";
				
		case SLcToken_Sleep:
			return "sleep";
				
		case SLcToken_Else:
			return "else";
				
		case SLcToken_Func:
			return "func";
				
		case SLcToken_Over:
			return "over";
				
		case SLcToken_Void:
			return "void";
				
		case SLcToken_For:
			return "for";
				
		case SLcToken_Int:
			return "int";
				
		case SLcToken_Var:
			return "var";
				
		case SLcToken_At:
			return "at";
				
		case SLcToken_If:
			return "if";
				
		case SLcToken_EOF:
			return "EOF";
		
		default:
			UUmAssert(0);
			return "Unknown";
	}
}
