/*
	FILE:	BFW_Group.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Dec 17, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_Group.h"

typedef enum GRtToken
{
	GReToken_None,
	
	GReToken_BracketLeft,
	GReToken_BracketRight,
	GReToken_CurlyLeft,
	GReToken_CurlyRight,
	GReToken_Comma,
	GReToken_String,
	GReToken_Equal,
	GReToken_Identifier,
	
	GReToken_EOF
	
} GRtToken;

typedef struct GRtElement
{
	GRtElementType	type;
	
	union
	{
		char*				value;
		GRtGroup*			group;
		GRtElementArray*	array;
	} u;
	
} GRtElement;

struct GRtElementArray
{
	UUtUns16	numElements;
	GRtElement	elements[GRcMaxNumArray];
	
};

typedef struct GRtBinding
{
	char*		varName;
	
	GRtElement	element;

} GRtBinding;

struct GRtGroup
{
	UUtUns16	numBindings;
	GRtBinding	bindings[GRcMaxNumArray];
	GRtGroup	*recursive;
};

struct GRtGroup_Context
{
	UUtMemory_Pool*		memoryPool;
	UUtMemory_String*	memoryString;
};

GRtToken	GRgToken;
GRtToken	GRgUngotToken = GReToken_None;
char		GRgLexem[512];
char*		GRgCurLine = NULL;
UUtUns32	GRgMaxLines = UUcMaxInt32;
UUtUns32	GRgLineNum = 0;

static GRtGroup*
GRiGroup_Create(
	GRtGroup_Context*	inContext,
	GRtGroup*			inPreferenceGroup,
	BFtTextFile*		inTextFile);
	
static UUtError
GRiGroup_ParseElement(
	GRtGroup_Context*	inContext,
	GRtGroup*			inPreferenceGroup,
	BFtTextFile*		inTextFile,
	GRtElement			*outElement);

static void
GRiUngetToken(
	GRtToken	inToken)
{
	GRgUngotToken = inToken;
}

static void
GRiGetNextToken(
	BFtTextFile*		inTextFile)
{
	char	c;
	char*	lexem = GRgLexem;
	UUtBool escaped = UUcFalse;

	if(GRgUngotToken != GReToken_None)
	{
		GRgToken = GRgUngotToken;
		GRgUngotToken = GReToken_None;
		return;
	}
	
	GRgToken = GReToken_None;
	
	if(GRgCurLine == NULL || *GRgCurLine == 0)
	{
		while(1)
		{
			GRgCurLine = BFrTextFile_GetNextStr(inTextFile);
			GRgLineNum++;
			
			if(GRgCurLine == NULL || GRgLineNum >= GRgMaxLines)
			{
				GRgToken = GReToken_EOF;
				return;
			}
			// ignore blank lines
			while(1)
			{
				c = *GRgCurLine++;
				if(!UUrIsSpace(c)) break;
			}
			if(c != 0) break;
		}
	}
	else
	{
		while(1)
		{
			c = *GRgCurLine++;
			if(!UUrIsSpace(c)) break;
		}
	}

	switch(c)
	{
		case '$':
			c = *GRgCurLine++;
			while(1)
			{
				*lexem++ = c;
				c = *GRgCurLine;
				if(!(isalpha(c) || c == '_' || isdigit(c))) break;
				
				GRgCurLine++;
			}
			*lexem = 0;
			
			GRgToken = GReToken_Identifier;
			return;
		
		case '{':
			GRgToken = GReToken_CurlyLeft;
			return;

		case '}':
			GRgToken = GReToken_CurlyRight;
			return;
			
		case '[':
			GRgToken = GReToken_BracketLeft;
			return;

		case ']':
			GRgToken = GReToken_BracketRight;
			return;
		
		case '=':
			GRgToken = GReToken_Equal;
			return;
		
		case ',':
			GRgToken = GReToken_Comma;
			return;
		
		case '#':
			GRgCurLine = NULL;
			GRiGetNextToken(inTextFile);
			return;
		
		case '"':
			while(1)
			{
				*lexem++ = c;
				c = *GRgCurLine;
				if(c == 0)
				{
					break;
				}
				else if(c == '"')
				{
					GRgCurLine++;
					break;
				}
				else if(c == '\\')
				{
					c = *GRgCurLine++;
					UUmAssert(c != 0);
				}
				
				GRgCurLine++;
			}
			
			*lexem = 0;
			
			GRgToken = GReToken_String;
			break;

		case '/':
			c = *GRgCurLine++;
			UUmAssert(c != 0);
			escaped = UUcTrue;
			GRgToken = GReToken_String;
			break;
	}
	
	while(1)
	{
		*lexem++ = c;
		c = *GRgCurLine;
		if (c=='/')
		{
			escaped = UUcTrue;
			c = *(++GRgCurLine);
		}
		
		if(c == 0 || 
			c == '{' ||
			c == '}' ||
			c == '[' ||
			c == ']' ||
			c == '=' ||
			c == ',' ||
			c == '#' ||
			c == '$')
		{
			if (!escaped) break;
		}
		escaped = UUcFalse;

		GRgCurLine++;
	}
	
	*lexem = 0;
	
	GRgToken = GReToken_String;
}
	

static GRtElementArray*
GRiGroup_ParseArray(
	GRtGroup_Context*	inContext,
	GRtGroup*			inPreferenceGroup,
	BFtTextFile*		inTextFile)
{
	UUtError			error;
	GRtElementArray*	newArray;

	UUmAssert(NULL != inContext);
	UUmAssert(NULL != inTextFile);
	
	newArray = UUrMemory_Pool_Block_New(inContext->memoryPool, sizeof(GRtElementArray));
	if(newArray == NULL)
	{
		return NULL;
	}
	
	newArray->numElements = 0;
	
	while(1)
	{
		GRiGetNextToken(inTextFile);
		
		if(GRgToken == GReToken_BracketRight)
		{
			break;
		}
		
		GRiUngetToken(GRgToken);
		
		error = 
			GRiGroup_ParseElement(
				inContext,
				inPreferenceGroup,
				inTextFile,
				&newArray->elements[newArray->numElements]);
		if(error != UUcError_None)
		{
			return NULL;
		}
		
		newArray->numElements++;
	}
	
	return newArray;
}

static UUtError
GRiGroup_ParseElement(
	GRtGroup_Context*	inContext,
	GRtGroup*			inPreferenceGroup,
	BFtTextFile*		inTextFile,
	GRtElement			*outElement)
{
	UUtError		error;
	char*			p;
	char*			lastNonSpace = NULL;
	char			temp[1024];
	char*			value;
	GRtElementType	elementType;
	UUtUns32		itr;
	
	UUmAssert(NULL != inContext);
	UUmAssert(NULL != inTextFile);
	UUmAssert(NULL != outElement);
	
	GRiGetNextToken(inTextFile);
	
	if(GRgToken == GReToken_CurlyLeft)
	{
		outElement->type = GRcElementType_Group;
		outElement->u.group = GRiGroup_Create(inContext, inPreferenceGroup, inTextFile);
		if(outElement->u.group == NULL)
		{
			return UUcError_Generic;
		}
	}
	else if(GRgToken == GReToken_BracketLeft)
	{
		outElement->type = GRcElementType_Array;
		outElement->u.array = GRiGroup_ParseArray(inContext, inPreferenceGroup, inTextFile);
		if(outElement->u.array == NULL)
		{
			return UUcError_Generic;
		}
	}
	else if(GRgToken == GReToken_Identifier)
	{
		
		if(NULL == inPreferenceGroup)
		{
			UUrError_Report(UUcError_Generic, "No preferences group");
			return UUcError_Generic;
		}
		
		error = GRrGroup_GetElement(inPreferenceGroup, GRgLexem, &elementType, &value);
		if(error != UUcError_None || elementType != GRcElementType_String)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Illegal preference variable");
		}
		
		UUrString_Copy(temp, value, 1024);
		
		p = temp + strlen(temp);
		
		GRiGetNextToken(inTextFile);
		
		if(GRgToken != GReToken_String)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "expected a string here");
		}
		
		// Wierd ass crap here for backward compatibility but to remove extra spaces from strings
		if(!isspace(GRgLexem[0]))
		{
			for(itr = strlen(GRgLexem); itr-- > 0;)
			{
				if(isspace(GRgLexem[itr]))
				{
					GRgLexem[itr] = 0;
				}
				else
				{
					break;
				}
			}
		}

		UUrString_Copy(p, GRgLexem, 1024 - strlen(p));
	
		outElement->type = GRcElementType_String;
		
		outElement->u.value = UUrMemory_String_GetStr(inContext->memoryString, temp);
	}
	else if(GRgToken == GReToken_String)
	{
		// Wierd ass crap here for backward compatibility but to remove extra spaces from strings
		if(!isspace(GRgLexem[0]))
		{
			for(itr = strlen(GRgLexem); itr-- > 0;)
			{
				if(isspace(GRgLexem[itr]))
				{
					GRgLexem[itr] = 0;
				}
				else
				{
					break;
				}
			}
		}
		
		UUrString_Copy(temp, GRgLexem, 1024);
		
		outElement->type = GRcElementType_String;
		
		outElement->u.value = UUrMemory_String_GetStr(inContext->memoryString, temp);
	}
	else
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Illegal token");
	}
	
	GRiGetNextToken(inTextFile);
	
	if(GRgToken != GReToken_Comma)
	{
		GRiUngetToken(GRgToken);
	}
	
	return UUcError_None;
}

static GRtGroup*
GRiGroup_Create(
	GRtGroup_Context*	inContext,
	GRtGroup*			inPreferenceGroup,
	BFtTextFile*		inTextFile)
{
	UUtError	error;
	GRtGroup*	newGroup;
	char		c;
	
	UUmAssert(NULL != inContext);
	UUmAssert(NULL != inTextFile);
	
	newGroup = UUrMemory_Pool_Block_New(inContext->memoryPool, sizeof(GRtGroup));
	if(newGroup == NULL)
	{
		UUrError_Report(UUcError_OutOfMemory, "out of memory");
		return NULL;
	}
	
	newGroup->numBindings = 0;
	newGroup->recursive = NULL;
	
	while(1)
	{
		GRiGetNextToken(inTextFile);
		
		if(GRgToken == GReToken_CurlyRight ||
			GRgToken == GReToken_BracketRight ||
			GRgToken == GReToken_EOF) break;
		
		if(GRgToken == GReToken_Identifier)
		{
			newGroup->bindings[newGroup->numBindings].varName =
				UUrMemory_String_GetStr(inContext->memoryString, GRgLexem);
			
			GRiGetNextToken(inTextFile);
			
			if(GRgToken != GReToken_Equal)
			{
				UUrError_Report(UUcError_Generic, "Expecting $name = value");
				return NULL;
			}
			
			while(1)
			{
				c = *GRgCurLine;
				if(c == 0 || !UUrIsSpace(c)) break;
				GRgCurLine++;
			}
			
			if(*GRgCurLine != 0)
			{
				error =
					GRiGroup_ParseElement(
						inContext,
						inPreferenceGroup,
						inTextFile,
						&newGroup->bindings[newGroup->numBindings].element);
				if(error != UUcError_None)
				{
					UUrError_Report(UUcError_Generic, "Expecting $name = value");
					return NULL;
				}
			}
			else
			{
				newGroup->bindings[newGroup->numBindings].element.type = GRcElementType_String;
				
				newGroup->bindings[newGroup->numBindings].element.u.value =
					UUrMemory_String_GetStr(inContext->memoryString, "\0");
			}

			newGroup->numBindings++;
		}
	}
	
	return newGroup;
}

UUtError 
GRrGroup_Context_New(
	GRtGroup_Context	**outGroupContext)
{
	GRtGroup_Context *newContext;

	newContext = UUrMemory_Block_New(sizeof(GRtGroup_Context));
	if(NULL == newContext)
	{
		goto errorExit;
	}
	
	newContext->memoryPool = NULL;
	newContext->memoryString = NULL;
	
	newContext->memoryPool = UUrMemory_Pool_New(10 * 1024, UUcPool_Growable);
	if(newContext->memoryPool == NULL)
	{
		goto errorExit;
	}
	
	newContext->memoryString = UUrMemory_String_New(10 * 1024);
	if(newContext->memoryString == NULL)
	{
		goto errorExit;
	}

	*outGroupContext = newContext;
	return UUcError_None;

errorExit:
	GRrGroup_Context_Delete(newContext);

	return UUcError_OutOfMemory;
}

UUtError
GRrGroup_Context_NewFromPartialFile(
	BFtTextFile*		inFile,
	GRtGroup*			inPreferenceGroup,
	GRtGroup_Context	*inGroupContext,
	UUtUns16			inLines,
	GRtGroup_Context*	*outGroupContext,
	GRtGroup*			*outGroup)
{
	/***************
	* Creates a new group context based on an existing open file.
	* You specify how many lines from the current file pointer are to
	* be treated as the group.
	*/
	
	UUtError			error;
	GRtGroup_Context*	newContext;
	
	UUmAssert(NULL != inFile);
	UUmAssert(NULL != outGroupContext);
	UUmAssert(NULL != outGroup);
	
	GRgMaxLines = inLines;
	GRgLineNum = 0;
	
	*outGroupContext = NULL;
	*outGroup = NULL;
	
	if (NULL == inGroupContext)
	{
		error = GRrGroup_Context_New(&newContext);
		UUmError_ReturnOnError(error);
	}
	else 
	{
		newContext = inGroupContext;
	}
		
	*outGroup = GRiGroup_Create(newContext, inPreferenceGroup, inFile);
	
	*outGroupContext = newContext;
	
	return UUcError_None;
}

UUtError
GRrGroup_Context_NewFromString(
	const char*			inString,
	GRtGroup*			inPreferenceGroup,
	GRtGroup_Context	*inGroupContext,
	GRtGroup_Context*	*outGroupContext,
	GRtGroup*			*outGroup)
{
	UUtError		error;
	BFtTextFile*	textFile;
	
	GRgMaxLines = UUcMaxInt32;
	GRgLineNum = 0;
	
	error = 
		BFrTextFile_MakeFromString(
			inString,
			&textFile);
	UUmError_ReturnOnError(error);
	
	error = 
		GRrGroup_Context_NewFromPartialFile(
			textFile,
			inPreferenceGroup,
			inGroupContext,
			UUcMaxUns16,
			outGroupContext,
			outGroup);
	
	BFrTextFile_Close(textFile);
	
	return error;
}
	
UUtError
GRrGroup_Context_NewFromFileRef(
	BFtFileRef*			inFileRef,
	GRtGroup*			inPreferenceGroup,
	GRtGroup_Context	*inGroupContext,
	GRtGroup_Context*	*outGroupContext,
	GRtGroup*			*outGroup)
{
	UUtError			error;
	GRtGroup_Context*	newContext;
	BFtTextFile*		textFile;

	UUmAssert(NULL != inFileRef);
	
	if (NULL != outGroupContext) 
	{
		*outGroupContext = NULL;
	}

	if (NULL != outGroup) 
	{
		*outGroup = NULL;
	}
	
	GRgMaxLines = UUcMaxInt32;
	GRgLineNum = 0;

	if (NULL == inGroupContext)
	{
		error = GRrGroup_Context_New(&newContext);
		UUmError_ReturnOnError(error);
	}
	else 
	{
		newContext = inGroupContext;
	}
	
	error = BFrTextFile_OpenForRead(inFileRef, &textFile);

	if (UUcError_None != error) {
		UUrError_ReportP(error, "failed to open group file %s", (UUtUns32) BFrFileRef_GetLeafName(inFileRef),0,0);
	}
	
	if (NULL != outGroup) 
	{
		*outGroup = GRiGroup_Create(newContext, inPreferenceGroup, textFile);
	}

	if (NULL != outGroupContext)
	{
		*outGroupContext = newContext;
	}
	
	BFrTextFile_Close(textFile);
	
	return UUcError_None;
}

void
GRrGroup_Context_Delete(
	GRtGroup_Context*	inGroupContext)
{
	if(inGroupContext != NULL)
	{
		if(inGroupContext->memoryPool != NULL)
		{
			UUrMemory_Pool_Delete(inGroupContext->memoryPool);
		}
		
		if(inGroupContext->memoryString != NULL)
		{
			UUrMemory_String_Delete(inGroupContext->memoryString);
		}
		
		UUrMemory_Block_Delete(inGroupContext);
	}	
}

UUtError
GRrGroup_GetElement(
	GRtGroup*		inGroup,
	const char*		inVarName,
	GRtElementType	*outElementType,	// optional
	void*			*outDataPtr)		// Either a GRtGroup, GRtElementArray, or char*
{
	UUtUns16	i;

	UUmAssert(inGroup != NULL);
	UUmAssert(inVarName != NULL);
	UUmAssertWritePtr(outDataPtr, sizeof(void *));

	for(i = 0; i < inGroup->numBindings; i++)
	{
		if(!strcmp(inGroup->bindings[i].varName, inVarName))
		{
			if (outElementType) *outElementType = inGroup->bindings[i].element.type;
			*outDataPtr = inGroup->bindings[i].element.u.value;

			return UUcError_None;
		}
	}

	if (inGroup->recursive != NULL) 
	{
		return GRrGroup_GetElement(
			inGroup->recursive, 
			inVarName, 
			outElementType, 
			outDataPtr);
	}
	
	return GRcError_ElementNotFound;
}
	
UUtError
GRrGroup_Array_GetElement(
	GRtElementArray*	inElementArray,
	UUtUns32			inIndex,
	GRtElementType		*outElementType,
	void*				*outDataPtr)
{
	UUmAssert(inElementArray != NULL);
	UUmAssertWritePtr(outDataPtr, sizeof(void *));

	if(inIndex < inElementArray->numElements)
	{
		if (outElementType) *outElementType = inElementArray->elements[inIndex].type;
		*outDataPtr = inElementArray->elements[inIndex].u.value;
		
		return UUcError_None;
	}
	
	return GRcError_ElementNotFound;
}

UUtUns32
GRrGroup_Array_GetLength(
	GRtElementArray*	inElementArray)
{
	UUmAssert(inElementArray != NULL);

	return inElementArray->numElements;
}


	 
UUtError
GRrGroup_GetString(
	GRtGroup*		inGroup,
	const char*		inVarName,
	char*			*outString)
{
	UUtError error;
	GRtElementType elementType;
	char *string;

	UUmAssert(NULL != inGroup);
	UUmAssert(NULL != inVarName);
	UUmAssert(NULL != outString);

	error = GRrGroup_GetElement(
					inGroup,
					inVarName,
					&elementType,
					&string);

	if (error)
	{
		return error;
	}

	if (elementType != GRcElementType_String)
	{
		return UUcError_Generic;
	}

	// success
	*outString = string;

	return UUcError_None;
}

UUtError
GRrGroup_GetUns32(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtUns32		*outNum)
{
	UUtError error;
	GRtElementType elementType;
	char *string;
	int numParsed;

	UUmAssert(NULL != inGroup);
	UUmAssert(NULL != inVarName);
	UUmAssert(NULL != outNum);

	error = GRrGroup_GetElement(
					inGroup,
					inVarName,
					&elementType,
					&string);

	if (error)
	{
		return error;
	}

	if (elementType != GRcElementType_String)
	{
		return UUcError_Generic;
	}

	if (('0' == string[0]) && ('x' == string[1])) {
		numParsed = sscanf(string + 2, "%x", outNum);
	}
	else {
		numParsed = sscanf(string, "%d", outNum);
	}

	if (1 != numParsed)
	{
		return UUcError_Generic;
	}

	return UUcError_None;
}

UUtError
GRrGroup_GetInt32(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtInt32		*outNum)
{
	UUtError error;
	GRtElementType elementType;
	char *string;

	UUmAssert(NULL != inGroup);
	UUmAssert(NULL != inVarName);
	UUmAssert(NULL != outNum);

	error = GRrGroup_GetElement(
					inGroup,
					inVarName,
					&elementType,
					&string);

	if (error)
	{
		return error;
	}

	if (elementType != GRcElementType_String)
	{
		return UUcError_Generic;
	}

	error = UUrString_To_Int32(string, outNum);

	return error;
}

UUtError
GRrGroup_GetInt16(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtInt16		*outNum)
{
	UUtError error;
	UUtInt32 temp;

	UUmAssert(NULL != inGroup);
	UUmAssert(NULL != inVarName);
	UUmAssert(NULL != outNum);

	error = GRrGroup_GetInt32(inGroup, inVarName, &temp);
	if (error != UUcError_None)
	{
		return error;
	}

	if ((temp < UUcMinInt16) || (temp > UUcMaxInt16))
	{
		return UUcError_Generic;
	}

	*outNum = (UUtInt16) (temp);

	return UUcError_None;
}


UUtError
GRrGroup_GetInt8(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtInt8			*outNum)
{
	UUtError error;
	UUtInt32 temp;

	UUmAssert(NULL != inGroup);
	UUmAssert(NULL != inVarName);
	UUmAssert(NULL != outNum);

	error = GRrGroup_GetInt32(inGroup, inVarName, &temp);
	if (error != UUcError_None)
	{
		return error;
	}

	if ((temp < UUcMinInt8) || (temp > UUcMaxInt8))
	{
		return UUcError_Generic;
	}

	*outNum = (UUtInt8) (temp);

	return UUcError_None;
}

UUtError
GRrGroup_GetUns16(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtUns16		*outNum)
{
	UUtError error;
	UUtUns32 temp;

	UUmAssert(NULL != inGroup);
	UUmAssert(NULL != inVarName);
	UUmAssert(NULL != outNum);

	error = GRrGroup_GetUns32(inGroup, inVarName, &temp);
	if (error != UUcError_None)
	{
		return error;
	}

	if ((temp < UUcMinUns16) || (temp > UUcMaxUns16))
	{
		return UUcError_Generic;
	}

	*outNum = (UUtUns16) (temp);

	return UUcError_None;
}


UUtError
GRrGroup_GetUns8(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtUns8			*outNum)
{
	UUtError error;
	UUtUns32 temp;

	UUmAssert(NULL != inGroup);
	UUmAssert(NULL != inVarName);
	UUmAssert(NULL != outNum);

	error = GRrGroup_GetUns32(inGroup, inVarName, &temp);
	if (error != UUcError_None)
	{
		return error;
	}

	if ((temp < UUcMinUns8) || (temp > UUcMaxUns8))
	{
		return UUcError_Generic;
	}

	*outNum = (UUtUns8) (temp);

	return UUcError_None;
}

UUtError
GRrGroup_GetBool(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtBool			*outBool)
{
	UUtError error;
	UUtUns32 temp;

	UUmAssertReadPtr(inGroup, sizeof(GRtGroup));
	UUmAssertReadPtr(inVarName, sizeof(char));
	UUmAssertWritePtr(outBool, sizeof(UUtBool));

	error = GRrGroup_GetUns32(inGroup, inVarName, &temp);
	if (error != UUcError_None)
	{
		return error;
	}

	switch(temp)
	{
		case UUcTrue:
			*outBool = UUcTrue;
		break;

		case UUcFalse:
			*outBool = UUcFalse;
		break;

		default:
			return UUcError_Generic;
	}

	return UUcError_None;
}


UUtError
GRrGroup_GetFloat(
	GRtGroup*		inGroup,
	const char*		inVarName,
	float			*outNum)
{
	UUtError error;
	GRtElementType elementType;
	char *string;
	int numParsed;

	UUmAssert(NULL != inGroup);
	UUmAssert(NULL != inVarName);
	UUmAssert(NULL != outNum);

	error = GRrGroup_GetElement(
					inGroup,
					inVarName,
					&elementType,
					&string);

	if (error)
	{
		return error;
	}

	if (elementType != GRcElementType_String)
	{
		return UUcError_Generic;
	}

	numParsed = sscanf(string, "%f", outNum);

	if (1 != numParsed)
	{
		return UUcError_Generic;
	}

	return UUcError_None;
}

UUtError
GRrGroup_GetOSType(
	GRtGroup*		inGroup,
	const char*		inVarName,
	UUtUns32		*outOSType)
{
	UUtError error;
	char *string;

	UUmAssert(NULL != inGroup);
	UUmAssert(NULL != inVarName);
	UUmAssert(NULL != outOSType);

	error = GRrGroup_GetString(inGroup, inVarName, &string);

	if (error) 
	{
		return error;
	}

	if (strlen(string) < 4)
	{
		return error;
	}

	*outOSType = UUm4CharToUns32(string[0], string[1], string[2], string[3]);

	return UUcError_None;
}

UUtUns32
GRrGroup_GetNumVariables(
	const GRtGroup*		inGroup)
{
	return inGroup->numBindings;
}

const char *
GRrGroup_GetVarName(
	const GRtGroup*		inGroup,
	UUtUns32		inVarNumber)
{
	const char *varName = NULL;
	
	if (inVarNumber < inGroup->numBindings) 
	{
		varName = inGroup->bindings[inVarNumber].varName;
	}

	return varName;
}

void
GRrGroup_SetRecursive(
	GRtGroup*	inGroup,
	GRtGroup*	inRecursive)
{
	inGroup->recursive = inRecursive;

	return;
}

