/*
	FILE:	TE_Parser.c

	AUTHOR:	Brent H. Pease

	CREATED: June 22, 1997

	PURPOSE:

	Copyright 1997

*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "BFW.h"
#include "BFW_FileManager.h"

#include "BFW_TM_Private.h"
#include "TE_Symbol.h"
#include "TE_Parser.h"

typedef enum TEtToken
{
	TEcToken_None,

	TEcToken_Struct,
	TEcToken_Template,
	TEcToken_Enum,

	TEcToken_VarIndex,
	TEcToken_VarArray,
	TEcToken_Pad,
	TEcToken_TemplateReference,

	TEcToken_SemiColon,
	TEcToken_Comma,
	TEcToken_LeftParen,
	TEcToken_RightParen,
	TEcToken_LeftBracket,
	TEcToken_RightBracket,
	TEcToken_LeftCurly,
	TEcToken_RightCurly,
	TEcToken_Star,

	TEcToken_8Byte,
	TEcToken_4Byte,
	TEcToken_2Byte,
	TEcToken_1Byte,

	TEcToken_AsciiConst,
	TEcToken_Number,
	TEcToken_ID,
	TEcToken_String,
	TEcToken_Raw,
	TEcToken_Separate,

	TEcToken_EOF

} TEtToken;

char		*TEgCurInputFileName;
UUtInt32	TEgCurInputFileLine;

char		TEgLexem[512];

char		*TEgCurDataPtr;
char		*TEgEndDataPtr;

TEtToken	TEgUngotToken = TEcToken_None;

extern UUtBool		TEgError;

extern FILE*		TEgErrorFile;

static void TEiParser_ReportSyntaxErrorAndDie(
	char *msg)
{

	if(TEgErrorFile != NULL)
	{
		fprintf(TEgErrorFile,
			"Syntax Error: File: %s, Line %d: Msg: %s, Lexem: %s"UUmNL,
			TEgCurInputFileName,
			TEgCurInputFileLine,
			msg,
			TEgLexem);
	}

	TEgError = UUcTrue;
}

static TEtToken TEiParser_GetNextToken(
	void)
{
	char		*src = TEgCurDataPtr;
	char		*dst = TEgLexem;
	TEtToken	returnToken;
	char		c;
	UUtInt32	lexemLength;
	char		*lexem;


	if(TEgUngotToken != TEcToken_None)
	{
		returnToken = TEgUngotToken;
		TEgUngotToken = TEcToken_None;

		return returnToken;
	}

	returnToken = TEcToken_None;

start:

	while(src < TEgEndDataPtr)
	{
		c = *src++;
		if(c == '\n')
		{
			if(*src == '\r') src++;
			TEgCurInputFileLine++;
		}
		else if(c == '\r')
		{
			if(*src == '\n') src++;
			TEgCurInputFileLine++;
		}
		else if(!UUrIsSpace(c))
		{
			break;
		}
	}

	if(src >= TEgEndDataPtr)
	{
		return TEcToken_EOF;
	}

	switch(c)
	{
		case '[':
			returnToken = TEcToken_LeftBracket;
			goto found;

		case ']':
			returnToken = TEcToken_RightBracket;
			goto found;

		case '{':
			returnToken = TEcToken_LeftCurly;
			goto found;

		case '}':
			returnToken = TEcToken_RightCurly;
			goto found;

		case '(':
			returnToken = TEcToken_LeftParen;
			goto found;

		case ')':
			returnToken = TEcToken_RightParen;
			goto found;

		case ',':
			returnToken = TEcToken_Comma;
			goto found;

		case '*':
			returnToken = TEcToken_Star;
			goto found;

		case ';':
			returnToken = TEcToken_SemiColon;
			goto found;

		case '\'':
			while(1)
			{
				c = *src++;
				if(c == '\'' || c == '\n' || c == '\r' || src >= TEgEndDataPtr) break;
				*dst++ = c;
			}
			*dst++ = 0;

			if(c != '\'')
			{
				TEiParser_ReportSyntaxErrorAndDie("Expecting an end quote for the 4char");
			}
			returnToken = TEcToken_AsciiConst;
			goto found;

		case '\"':
			while(1)
			{
				c = *src++;
				if(c == '\"' || c == '\n' || c == '\r' || src >= TEgEndDataPtr) break;
				*dst++ = c;
			}
			*dst++ = 0;

			if(c != '\"')
			{
				fprintf(stderr, "%s"UUmNL, TEgLexem);
				TEiParser_ReportSyntaxErrorAndDie("Expecting an end quote for the string");
			}
			returnToken = TEcToken_String;
			goto found;

		case '/':
			c = *src++;
			if(c == '/')
			{
				while(1)
				{
					c = *src++;
					if(c == '\n' || c == '\r' || src >= TEgEndDataPtr) break;
				}

				src--;

				goto start;
			}
			else if(c == '*')
			{
				while(1)
				{
					c = *src++;

					if(src >= TEgEndDataPtr) break;

					if(c == '\n')
					{
						if(*src == '\r') src++;
						TEgCurInputFileLine++;
					}
					else if(c == '\r')
					{
						if(*src == '\n') src++;
						TEgCurInputFileLine++;
					}
					else if(c == '*')
					{
						c = *src++;
						if(c == '/') break;
						src--;
					}
				}

				goto start;
			}
			goto found;
	}

	if(isalpha(c) || c == '_')
	{
		while(1)
		{
			*dst++ = c;
			c = *src++;
			if(!(isalpha(c) || c == '_' || isdigit(c)) || src >= TEgEndDataPtr) break;
		}

		src--;

		*dst++ = 0;

		if(TEgLexem[0] == 't' &&
			TEgLexem[1] == 'm' &&
			TEgLexem[2] == '_')
		{
			lexem = TEgLexem + 3;
			lexemLength = strlen(lexem);

			switch(lexemLength)
			{
				case 3:
					if(!strcmp(lexem, "pad"))
					{
						returnToken = TEcToken_Pad;
						goto found;
					}
					else if (!strcmp(lexem, "raw"))
					{
						src = strpbrk(src, "(\n\r");

						if ((NULL == src) || (*src != '('))
						{
							TEiParser_ReportSyntaxErrorAndDie("Expecting a '(' for tm_raw");
						}

						src++;

						src = strpbrk(src, ")\n\r");

						if ((NULL == src) || (*src != ')'))
						{
							TEiParser_ReportSyntaxErrorAndDie("Expecting a ')' for tm_raw");
						}

						src++;

						returnToken = TEcToken_Raw;

						goto found;
					}
					break;

				case 4:
					if(!strcmp(lexem, "enum"))
					{
						returnToken = TEcToken_Enum;
						goto found;
					}
					break;

				case 6:
					if(!strcmp(lexem, "struct"))
					{
						returnToken = TEcToken_Struct;
						goto found;
					}
					break;

				case 8:
					if(!strcmp(lexem, "template"))
					{
						returnToken = TEcToken_Template;
						goto found;
					}
					else if(!strcmp(lexem, "varindex"))
					{
						returnToken = TEcToken_VarIndex;
						goto found;
					}
					else if(!strcmp(lexem, "vararray"))
					{
						returnToken = TEcToken_VarArray;
						goto found;
					}
					else if (!strcmp(lexem, "separate"))
					{
						returnToken = TEcToken_Separate;
						goto found;
					}
					break;

				case 11:
					if(!strcmp(lexem, "templateref"))
					{
						returnToken = TEcToken_TemplateReference;
						goto found;
					}
					break;

			}
		}
		else if(TEgLexem[0] == 'U' &&
			TEgLexem[1] == 'U' &&
			TEgLexem[2] == 't')
		{
			lexem = TEgLexem + 3;

			if(!strcmp(lexem, "Int64"))
			{
				returnToken = TEcToken_8Byte;
				goto found;
			}
			else if(!strcmp(lexem, "Uns64"))
			{
				returnToken = TEcToken_8Byte;
				goto found;
			}
			else if(!strcmp(lexem, "Uns32"))
			{
				returnToken = TEcToken_4Byte;
				goto found;
			}
			else if(!strcmp(lexem, "Int32"))
			{
				returnToken = TEcToken_4Byte;
				goto found;
			}
			else if(!strcmp(lexem, "Uns16"))
			{
				returnToken = TEcToken_2Byte;
				goto found;
			}
			else if(!strcmp(lexem, "Int16"))
			{
				returnToken = TEcToken_2Byte;
				goto found;
			}
			else if(!strcmp(lexem, "Bool"))
			{
				returnToken = TEcToken_1Byte;
				goto found;
			}
			else if(!strcmp(lexem, "Uns8"))
			{
				returnToken = TEcToken_1Byte;
				goto found;
			}
			else if(!strcmp(lexem, "Int8"))
			{
				returnToken = TEcToken_1Byte;
				goto found;
			}
		}
		else if(0 == strcmp(TEgLexem, "float"))
		{
			returnToken = TEcToken_4Byte;
			goto found;
		}
		else if(0 == strcmp(TEgLexem, "char"))
		{
			returnToken = TEcToken_1Byte;
			goto found;
		}

		returnToken = TEcToken_ID;
	}
	else if(isdigit(c))
	{
		while(1)
		{
			*dst++ = c;
			c = *src++;
			if(!isdigit(c) || src >= TEgEndDataPtr) break;
		}

		src--;

		*dst++ = 0;

		returnToken = TEcToken_Number;
	}

found:

	TEgCurDataPtr = src;

	return returnToken;
}

static void
TEiParser_UngetToken(
	TEtToken	inToken)
{
	UUmAssert(inToken != TEcToken_None);

	TEgUngotToken = inToken;
}

static void
TEiParser_MatchToken(
	TEtToken	inToken)
{
	TEtToken	token;

	token = TEiParser_GetNextToken();

	if(token != inToken)
	{
		TEiParser_ReportSyntaxErrorAndDie("Illegal Token");
	}

}

static void
TEiParser_Field(
	void)
{
	TEtToken	token;

start:

	token = TEiParser_GetNextToken();

	switch(token)
	{
		case TEcToken_Raw:
			TErSymbolTable_Field_RawRef();

			TEiParser_MatchToken(TEcToken_ID);
			TErSymbolTable_Field_AddID();
			TEiParser_MatchToken(TEcToken_SemiColon);
			return;

		case TEcToken_Separate:
			TErSymbolTable_Field_SeparateIndex();

			TEiParser_MatchToken(TEcToken_ID);
			TErSymbolTable_Field_AddID();
			break;

		case TEcToken_VarIndex:
			TErSymbolTable_Field_VarIndex();
			goto start;

		case TEcToken_VarArray:
			TErSymbolTable_Field_VarArray();
			goto start;

		case TEcToken_TemplateReference:
			TErSymbolTable_Field_TemplateRef();
			TEiParser_MatchToken(TEcToken_ID);
			TErSymbolTable_Field_AddID();
			TEiParser_MatchToken(TEcToken_SemiColon);
			return;

		case TEcToken_Pad:
			TErSymbolTable_Field_Pad();

			TEiParser_MatchToken(TEcToken_ID);
			TErSymbolTable_Field_AddID();
			break;

		case TEcToken_8Byte:
			TErSymbolTable_Field_8Byte();

			TEiParser_MatchToken(TEcToken_ID);
			TErSymbolTable_Field_AddID();
			break;

		case TEcToken_4Byte:
			TErSymbolTable_Field_4Byte();

			TEiParser_MatchToken(TEcToken_ID);
			TErSymbolTable_Field_AddID();
			break;

		case TEcToken_2Byte:
			TErSymbolTable_Field_2Byte();

			TEiParser_MatchToken(TEcToken_ID);
			TErSymbolTable_Field_AddID();
			break;

		case TEcToken_1Byte:
			TErSymbolTable_Field_1Byte();

			TEiParser_MatchToken(TEcToken_ID);
			TErSymbolTable_Field_AddID();
			break;

		case TEcToken_ID:
			TErSymbolTable_Field_TypeName();

			token = TEiParser_GetNextToken();

			if(token == TEcToken_ID)
			{
				TErSymbolTable_Field_AddID();
			}
			else if(token == TEcToken_Star)
			{
				TErSymbolTable_Field_Star();

				TEiParser_MatchToken(TEcToken_ID);
				TErSymbolTable_Field_AddID();
			}
			else
			{
				TEiParser_ReportSyntaxErrorAndDie("Illegal token");
			}
			break;

		default:
			TEiParser_ReportSyntaxErrorAndDie("Illegal token");
	}

	token = TEiParser_GetNextToken();

	if(token == TEcToken_LeftBracket)
	{
		while(token != TEcToken_SemiColon)
		{
			if(token != TEcToken_LeftBracket)
			{
				TEiParser_ReportSyntaxErrorAndDie("Expected [");
			}

			TEiParser_MatchToken(TEcToken_Number);
			TErSymbolTable_Field_Array();

			TEiParser_MatchToken(TEcToken_RightBracket);

			token = TEiParser_GetNextToken();
		}
	}
	else if(token != TEcToken_SemiColon)
	{
		TEiParser_ReportSyntaxErrorAndDie("Illegal token");
	}
}

static void
TEiParser_Struct(
	void)
{
	TEtToken	token;

	TEiParser_MatchToken(TEcToken_ID);

	TErSymbolTable_AddID();

	TEiParser_MatchToken(TEcToken_LeftCurly);

	while(1)
	{
		token = TEiParser_GetNextToken();

		if(token == TEcToken_RightCurly)
		{
			break;
		}

		if(token == TEcToken_None)
		{
			TEiParser_ReportSyntaxErrorAndDie("Illegal Token");
		}

		TEiParser_UngetToken(token);

		TErSymbolTable_StartField();

		TEiParser_Field();

		TErSymbolTable_EndField();
	}
}

static void
TEiParser_Template(
	void)
{
	char	c0, c1, c2, c3;

	TEiParser_MatchToken(TEcToken_LeftParen);
	TEiParser_MatchToken(TEcToken_AsciiConst);
	c0 = TEgLexem[0];

	TEiParser_MatchToken(TEcToken_Comma);
	TEiParser_MatchToken(TEcToken_AsciiConst);
	c1 = TEgLexem[0];

	TEiParser_MatchToken(TEcToken_Comma);
	TEiParser_MatchToken(TEcToken_AsciiConst);
	c2 = TEgLexem[0];

	TEiParser_MatchToken(TEcToken_Comma);
	TEiParser_MatchToken(TEcToken_AsciiConst);
	c3 = TEgLexem[0];

	TErSymbolTable_AddTag(UUm4CharToUns32(c0, c1, c2, c3));

	TEiParser_MatchToken(TEcToken_Comma);
	TEiParser_MatchToken(TEcToken_String);

	TErSymbolTable_AddToolName();

	TEiParser_MatchToken(TEcToken_RightParen);

	TEiParser_Struct();
}

static void
TEiParser_Enum(
	void)
{
	TEiParser_MatchToken(TEcToken_ID);
	TErSymbolTable_AddID();

}

void TErParser_Initialize(
	void)
{

}

void TErParser_Terminate(
	void)
{

}

void
TErParser_ProcessFile(
	UUtUns32	inFileLength,
	char		*inFileData)
{
	TEtToken	token = TEcToken_None;

	TEgCurDataPtr = inFileData;
	TEgEndDataPtr = inFileData + inFileLength;

	while(token != TEcToken_EOF)
	{
		token = TEiParser_GetNextToken();

		switch(token)
		{
			case TEcToken_Struct:
				TErSymbolTable_StartStruct();

				TEiParser_Struct();

				TErSymbolTable_EndStruct();
				break;

			case TEcToken_Template:
				TErSymbolTable_StartTemplate();

				TEiParser_Template();

				TErSymbolTable_EndTemplate();
				break;

			case TEcToken_Enum:
				TErSymbolTable_StartEnum();

				TEiParser_Enum();

				TErSymbolTable_EndEnum();
				break;
		}
	}
}
