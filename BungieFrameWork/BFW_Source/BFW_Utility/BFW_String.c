 /*
	FILE:	BFW_String.c

	AUTHOR:	Brent H. Pease, Michael Evans

	CREATED: May 18, 1997

	PURPOSE: General utility files

	Copyright 1997, 1998

*/

#include <ctype.h>

#include "BFW.h"

void
UUrString_Capitalize(
	char*		ioString,
	UUtUns32	inLength)
{
	UUtUns32	itr;

	UUmAssert(ioString);
	UUmAssert(strlen(ioString) < inLength);

	for(itr = 0;
		(itr < inLength) && (*ioString != 0);
		itr++, ioString++)
	{
		if ((*ioString >= 'a') && (*ioString <= 'z'))
		{
			*ioString += 'A' - 'a';
		}
	}
}

void
UUrString_StripDoubleQuotes(
	char*		ioString,
	UUtUns32	inLength)
{
	UUtUns32	itr;
	char *dst = ioString;

	UUmAssert(ioString);
	UUmAssert(strlen(ioString) < inLength);

	// GME 11/2/99 The "" string is often a constant expression.  In particular UUrTableGetNextEntry works that way.
	if ('\0' == *ioString) {
		return;
	}

	for(itr = 0; itr < inLength; itr++)
	{
		*dst = *ioString;
		if ((*dst) == '\0') break;
		if (*ioString != '"') dst++;
		ioString++;
	}
}

void
UUrString_MakeLowerCase(
	char*		ioString,
	UUtUns32	inLength)
{
	UUtUns32	itr;

	UUmAssert(ioString);
	UUmAssert(strlen(ioString) < inLength);

	for(itr = 0;
		(itr < inLength) && (*ioString != 0);
		itr++, ioString++)
	{
		if ((*ioString >= 'A') && (*ioString <= 'Z'))
		{
			*ioString -= 'A' - 'a';
		}
	}
}

UUtBool
UUrString_Substring(
	const char*	inString,
	const char*	inSub,
	UUtUns32	inLength)
{
	UUtUns32	itr;

	// Returns whether a substring exists in a string. Not fast.
	// Here because ANSI's strstr is broken

	const char *s = inSub, *c = inString;

	UUmAssert(strlen(inString) < inLength);

	for(itr = 0;
		(itr < inLength) && (*c != 0);
		itr++, c++)
	{
		if (*s == *c)
		{
			s++;
			if (!*s) return UUcTrue;
		}
		else if (s != inSub)
		{
			s = inSub;
			continue;
		}
	}

	return UUcFalse;
}

UUtBool
UUrString_HasPrefix(
	const char*	inString,
	const char*	inPrefix)
{
	const char *string;
	const char *prefix;
	UUtBool hasPrefix = UUcTrue;

	for(string = inString, prefix = inPrefix;
		*inPrefix != '\0';
		inString++, inPrefix++)
	{
		if (*inPrefix != *inString)
		{
			hasPrefix = UUcFalse;
			break;
		}
	}

	return hasPrefix;
}

void
UUrString_NukeNumSuffix(
	char*		ioString,
	UUtUns32	inLength)
{
	UUtUns32	len;
	// Nukes any numerical suffix on the string
	char *c = ioString;

	UUmAssert(strlen(ioString) < inLength);
	UUmAssert(ioString);
	UUmAssert(*ioString != '\0');

	len = strlen(ioString);
	if(len > inLength) len = inLength;

	c += len - 1;

	while (isdigit(*c))
	{
		if (c == ioString) break;
		c--;
	}
	*(c+1) = '\0';
}

void
UUrString_Copy(					// This copies the terminator as well
	char*		inDst,
	const char*	inSrc,
	UUtUns32	inDstLength)
{
	UUtUns32	itr;

	UUmAssertReadPtr(inDst, 1);
	UUmAssertReadPtr(inSrc, 1);

	UUmAssert(strlen(inSrc) < inDstLength);

#if defined(DEBUGGING) && DEBUGGING
	{
		UUtUns32 srcLen = strlen(inSrc);
	}
#endif

	for(itr = 0;
		(itr < inDstLength - 1) && (*inSrc != 0);
		itr++)
	{
		*inDst++ = *inSrc++;
	}

	*inDst = 0;
}

void
UUrString_Copy_Length(
	char*		inDst,
	const char*	inSrc,
	UUtUns32	inDstLength,
	UUtUns32	inCopyLength)
{
	UUtUns32	itr;

	UUmAssertReadPtr(inDst, 1);
	UUmAssertReadPtr(inSrc, 1);

	UUmAssert(inDstLength >= inCopyLength);

	for(itr = 0;
		(itr < inCopyLength) && (*inSrc != 0);
		itr++)
	{
		*inDst++ = *inSrc++;
	}

	// CB: string will be unterminated if inCopyLength == inDstLength
	if (inDstLength > itr)
		*inDst = 0;
}

void
UUrString_Cat(
	char*		inDst,
	const char*	inSrc,
	UUtUns32	inLength)
{
	UUtUns32 dstLength = strlen(inDst);
	UUmAssert(strlen(inDst) + strlen(inSrc) < inLength);

	UUrString_Copy(
		inDst + dstLength,
		inSrc,
		inLength - dstLength);
}

UUtBool
UUrString_IsSpace(
	const char *inString)
{
	UUtBool isSpace = UUcTrue;
	const char *curChr;

	for(curChr = inString; (isSpace && (*curChr != '\0')); curChr++)
	{
		isSpace = UUrIsSpace(*curChr);
	}

	return isSpace;
}

void
UUrString_StripExtension(
	char*	inStr)
{
	char*	internal = NULL;

	UUrString_Tokenize(inStr, ".", &internal);
}


/*
 * If the string compare functions are ever used much
 * under windows there are library versions.
 *
 * int _strnicmp( const char *string1, const char *string2, size_t count );
 * int _stricmp( const char *string1, const char *string2 );
 *
 */

// Attempt to be like ANSI C strcmp
UUtInt32
UUrString_Compare_NoCase(
	const char*	inStrA,
	const char*	inStrB)
{
	char	ca;
	char	cb;

	do
	{
		ca = *inStrA++;
		cb = *inStrB++;

		ca = UUmChar_Upper(ca);
		cb = UUmChar_Upper(cb);

	} while((ca == cb) && (ca != '\0'));

	return (ca - cb);
}

UUtInt32
UUrString_CompareLen_NoCase(
	const char*		inStrA,
	const char*		inStrB,
	UUtUns32	inLength)
{
	char	ca;
	char	cb;

	do
	{
		ca = *inStrA++;
		cb = *inStrB++;

		ca = UUmChar_Upper(ca);
		cb = UUmChar_Upper(cb);

	} while((ca == cb) && (ca != '\0') && (--inLength != 0));

	return (ca - cb);
}


UUtInt32 UUrString_Compare_NoCase_NoSpace(const char *inStrA, const char *inStrB)
{
	char	ca;
	char	cb;

	do
	{
		do
		{
			ca = *inStrA++;
		} while(UUrIsSpace(ca));

		do
		{
			cb = *inStrB++;
		} while(UUrIsSpace(cb));

		ca = UUmChar_Upper(ca);
		cb = UUmChar_Upper(cb);

	} while((ca == cb) && (ca != '\0'));

	return (ca - cb);
}

UUtError UUrString_To_Int32(const char *inString, UUtInt32 *outNum)
{
	UUtError error;
	int parsed;
	int number;

	if (!inString) return UUcError_Generic;

	UUmAssertReadPtr(inString, sizeof(char));
	UUmAssertWritePtr(outNum, sizeof(*outNum));

	if (('0' == inString[0]) && (('x' == inString[1]) || ('X' == inString[1])))	{
		parsed = sscanf(inString + 2, "%x", &number);
	}
	else {
		parsed = sscanf(inString, "%d", &number);
	}

	if (1 == parsed) {
		error = UUcError_None;
		*outNum = number;
	}
	else {
		error = UUcError_Generic;
	}

	return error;
}

UUtError UUrString_To_Uns32(const char *inString, UUtUns32 *outNum)
{
	UUtError error;
	UUtInt32 number;

	error = UUrString_To_Int32(inString, &number);

	if (UUcError_None == error) {
		*outNum = (UUtUns32) number;
	}

	return error;
}

UUtError UUrString_To_Int16(const char *inString, UUtInt16 *outNum)
{
	UUtError error;
	UUtInt32 number;

	error = UUrString_To_Int32(inString, &number);

	if (UUcError_None == error) {
		*outNum = (UUtInt16) number;
	}

	return error;
}

UUtError UUrString_To_Uns16(const char *inString, UUtUns16 *outNum)
{
	UUtError error;
	UUtInt32 number;

	error = UUrString_To_Int32(inString, &number);

	if (UUcError_None == error) {
		*outNum = (UUtUns16) number;
	}

	return error;
}

UUtError UUrString_To_Int8(const char *inString, UUtInt8 *outNum)
{
	UUtError error;
	UUtInt32 number;

	error = UUrString_To_Int32(inString, &number);

	if (UUcError_None == error) {
		*outNum = (UUtInt8) number;
	}

	return error;
}

UUtError UUrString_To_Uns8(const char *inString, UUtUns8 *outNum)
{
	UUtError error;
	UUtInt32 number;

	error = UUrString_To_Int32(inString, &number);

	if (UUcError_None == error) {
		*outNum = (UUtUns8) number;
	}

	return error;
}

UUtError UUrString_To_Float(const char *inString, float *outNum)
{
	UUtError error;
	int parsed;
	float number;

	UUmAssertReadPtr(inString, sizeof(char));
	UUmAssertWritePtr(outNum, sizeof(*outNum));

	parsed = sscanf(inString, "%f", &number);

	if (1 == parsed) {
		error = UUcError_None;
		*outNum = number;
	}
	else {
		error = UUcError_Generic;
	}

	return error;
}

UUtError UUrString_To_FloatRange(const char *inString, float *outLow, float *outHigh)
{
	// Parses string of form "x" or "x,y". If "x", then 'x' is put in both outLow and outHigh
	int parsed;

	if (UUrString_Substring(inString,",",strlen(inString)+1))
	{
		parsed = sscanf(inString,"%f,%f",outLow,outHigh);
		if (parsed != 2) return UUcError_Generic;

		return UUcError_None;
	}

	parsed = sscanf(inString,"%f",outLow);
	*outHigh = *outLow;
	if (parsed != 1) return UUcError_Generic;

	return UUcError_None;
}


UUtBool	 UUrIsSpace(char c)
{
	UUtBool isSpace = UUcFalse;

	switch(c)
	{
		case '\t':
		case ' ':
		case '\r':
		case '\n':
			isSpace = UUcTrue;
		break;

		default:
			isSpace = UUcFalse;
		break;
	}

	return isSpace;
}


UUtUns32 UUrStringToOSType(char *inString)
{
	UUtUns32 osType = UUm4CharToUns32(inString[0], inString[1], inString[2], inString[3]);

	return osType;
}

char *UUrString_Tokenize1(char *ioString, const char *inStrDelimit, char **internal)
{
	char *result;
	char *nextString = *internal;

	nextString = ioString ? ioString : nextString;

	result = nextString;

	// if there is more string to parse
	if (NULL != nextString) {
		nextString = strpbrk(nextString, inStrDelimit);

		if (NULL != nextString) {
			// terminate this string and go to the next string
			*nextString++ = '\0';
		}
	}

	*internal = nextString;

	return result;
}

char *UUrString_Tokenize(char *ioString, const char *inStrDelimit, char **internal)
{
	char *result;
	char *nextString = *internal;

	nextString = ioString ? ioString : nextString;

	// skip any initial delimiters
	if (NULL != nextString) {
		nextString += strspn(nextString, inStrDelimit);
		nextString = ('\0' != *nextString) ? nextString : NULL;
	}

	result = nextString;

	// if there is more string to parse
	if (NULL != nextString) {
		nextString = strpbrk(nextString, inStrDelimit);

		if (NULL != nextString) {
			// terminate this string and go to the next string
			*nextString++ = '\0';
		}
	}

	*internal = nextString;

	return result;
}

void
UUrString_PStr2CStr(
		const unsigned char*const	inPStr,
		char						*outCStr,
		UUtUns32					inBufferSize)
{
	UUtUns32	count;
	UUtUns32	itr;
	const unsigned char*		pp;
	char*		cp;

	pp = inPStr;
	count = *pp++;
	cp = outCStr;

	for(itr = 0; itr < count; itr++)
	{
		if(itr >= inBufferSize-1) break;

		*cp++ = *pp++;
	}

	*cp = 0;
}



UUtInt32 UUrString_CountDigits(UUtInt32 inNumber)
{
	UUtInt32 table[] =
	{
		9,
		99,
		999,
		9999,
		99999,
		999999,
		9999999,
		99999999,
		999999999,
/*		2147483648 */
		UUcMaxInt32
	};

	UUtInt32 total_digits;

	UUmAssert((-2147483647 - 1) == UUcMinInt32);
	UUmAssert(2147483647 == UUcMaxInt32);

	if (UUcMinInt32 == inNumber) {
		total_digits = 11;
	}
	else if (inNumber < 0) {
		total_digits = 1 + UUrString_CountDigits(-inNumber);
	}
	else {
		for(total_digits = 0; inNumber > table[total_digits]; total_digits++)
		{
		}

		total_digits++;
	}

	return total_digits;
}

void UUrString_PadNumber(UUtInt32 inNumber, UUtInt32 inDigits, char *outString)
{
	UUtInt32 actual_digits = UUrString_CountDigits(inNumber);

	while(actual_digits < inDigits) {
		*outString = '0';

		outString++;
		actual_digits++;
	}

	sprintf(outString, "%d", inNumber);

	return;
}
