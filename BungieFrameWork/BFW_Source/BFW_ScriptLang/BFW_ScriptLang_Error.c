/*
	FILE:	BFW_ScriptLang_Error.c

	AUTHOR:	Brent H. Pease

	CREATED: Oct 29, 1999

	PURPOSE:

	Copyright 1999

*/
#include <stdarg.h>

#include "BFW.h"
#include "BFW_TemplateManager.h"

#include "BFW_Console.h"
#include "BFW_ScriptLang.h"
#include "BFW_ScriptLang_Private.h"
#include "BFW_ScriptLang_Token.h"
#include "BFW_ScriptLang_Error.h"


BFtDebugFile *gScriptingLogFile = NULL;

static void UUcArglist_Call SLrScript_DisplayError(char *logText,	...)
{
	/*************
	* Displays an error to the console and the ai_new_script_log.txt file
	*/

	char buffer[2048];
	va_list ap;


	va_start(ap,logText);
	vsprintf(buffer,logText,ap);
	va_end(ap);

	COrConsole_Printf("%s", buffer);

#if SUPPORT_DEBUG_FILES
	if (!gScriptingLogFile) {
		gScriptingLogFile = BFrDebugFile_Open("ai_new_script_log.txt");
	}

	if (gScriptingLogFile)
	{
		BFrDebugFile_Printf(gScriptingLogFile, "%s"UUmNL, buffer);
	}
	else
	{
		COrConsole_Printf("### failed to open ai_new_script_log");
	}
#endif

	return;
}

void UUcArglist_Call
SLrScript_ReportError(
	SLtErrorContext*	inErrorContext,
	char*				inMsg,
	...)
{
	char buffer[2048];
	char buffer2[2048];
	va_list arglist;
	int return_value;

	va_start(arglist, inMsg);
	return_value= vsprintf(buffer, inMsg, arglist);
	va_end(arglist);

	sprintf(buffer2, "Func \"%s\", File \"%s\", Line %d: %s",
		inErrorContext->funcName,
		inErrorContext->fileName,
		inErrorContext->line,
		buffer);

	SLrScript_DisplayError("%s", buffer2);

	return;
}

void UUcArglist_Call
SLrScript_Error_Parse(
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken,
	char*				inMsg,
	...)
{
	char buffer[2048];
	char buffer2[2048];
	va_list arglist;
	int return_value;

	va_start(arglist, inMsg);
	return_value= vsprintf(buffer, inMsg, arglist);
	va_end(arglist);

	sprintf(buffer2, "parse error, token \"%s\", msg: %s", inToken->lexem, buffer);

	SLrScript_ReportError(inErrorContext, buffer2);
}

void UUcArglist_Call
SLrScript_Error_Lexical(
	SLtErrorContext*	inErrorContext,
	char				inChar,
	char*				inMsg,
	...)
{
	char buffer[2048];
	char buffer2[2048];
	va_list arglist;
	int return_value;

	va_start(arglist, inMsg);
	return_value= vsprintf(buffer, inMsg, arglist);
	va_end(arglist);

	sprintf(buffer2, "lexical error, \'%c\', msg : %s", inChar, inMsg);

	SLrScript_ReportError(inErrorContext, buffer2);
}

void
SLrScript_Error_Parse_IllegalToken(
	SLtErrorContext*	inErrorContext,
	SLtToken*			inToken,
	SLtToken_Code		inTokenCode)
{
	char buffer[2048];

	sprintf(
		buffer,
		"Illegal token, got \"%s\" expected \"%s\"",
		inToken->lexem,
		SLrTokenCode_ConvertToString(inTokenCode));

	SLrScript_ReportError(inErrorContext, buffer);
}

void UUcArglist_Call
SLrScript_Error_Semantic(
	SLtErrorContext*	inErrorContext,
	char*				inMsg,
	...)
{
	char buffer[2048];
	char buffer2[2048];
	va_list arglist;

	va_start(arglist, inMsg);
	vsprintf(buffer, inMsg, arglist);
	va_end(arglist);

	sprintf(buffer2, "semantic error, %s", buffer);

	SLrScript_ReportError(inErrorContext, buffer2);
}


void
SLrDebugMessage_LogParamList(
	UUtUns16			inParameterListLength,
	SLtParameter_Actual*		inParameterList)
{
	UUtUns16 itr;

	for(itr = 0; itr < inParameterListLength; itr++)
	{
		switch(inParameterList[itr].type)
		{
			case SLcType_Int32:
				BFrDebugFile_Printf(SLgDebugFile, "int: %d", inParameterList[itr].val.i);
				break;

			case SLcType_String:
				BFrDebugFile_Printf(SLgDebugFile, "string: %s", inParameterList[itr].val.str);
				break;

			case SLcType_Float:
				BFrDebugFile_Printf(SLgDebugFile, "float: %f", inParameterList[itr].val.f);
				break;

			case SLcType_Bool:
				BFrDebugFile_Printf(SLgDebugFile, "bool: %d", inParameterList[itr].val.b);
				break;

			default:
				UUmAssert(0);
		}
	}
}

