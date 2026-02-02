/*
	FILE:	BFW_FileManager_Common.c

	AUTHOR:	Brent H. Pease

	CREATED: August 14, 1997

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_AppUtilities.h"

#include <string.h>
#include <ctype.h>
#include <stdarg.h>

UUtError BFrFile_MapClassic(BFtFileRef	*inFileRef,
	UUtUns32			inOffset,
	BFtFile				**outFile,
	void				**outPtr,
	UUtUns32			*outSize);

#define BFcMaxSearchDepth	16

struct BFtTextFile
{
	BFtFileMapping *mapping;
	char*	fileMemory;
	char*	curStr;
	char*	endFile;
};

void
BFrFile_CompletionFunc_Bool(
	void*	inBoolAddr)
{
	*(UUtBool*)inBoolAddr = UUcTrue;
}

UUtError
BFrFileRef_LoadIntoMemory(
	BFtFileRef	*inFileRef,
	UUtUns32	*outFileLength,
	void*		*outMemory)
{
	UUtError		error;
	BFtFile*		file;
	char*			newMemory;

	UUmAssert(NULL != inFileRef);
	UUmAssert(NULL != outFileLength);
	UUmAssert(NULL != outMemory);

	*outMemory = NULL;
	*outFileLength	= 0;

	error = BFrFile_Open(inFileRef, "r", &file);
	if (error) { return error; }

	error = BFrFile_GetLength(file, outFileLength);
	UUmError_ReturnOnError(error);

	newMemory = UUrMemory_Block_New(*outFileLength + 1);
	if(newMemory == NULL)
	{
		UUmError_ReturnOnError(UUcError_OutOfMemory);
	}

	error = BFrFile_Read(file, *outFileLength, newMemory);
	UUmError_ReturnOnError(error);

	BFrFile_Close(file);

	newMemory[*outFileLength] = 0;

	*outMemory = newMemory;

	return UUcError_None;
}



UUtError
BFrFileRef_Search(
	const char*		inFileName,
	BFtFileRef		*outFileRef)
{
	UUtError	error;
	UUtUns16	i;
	char		temp[BFcMaxPathLength];
	char		temp2[BFcMaxPathLength];

	UUrString_Copy(temp, inFileName, BFcMaxPathLength);

	for(i = 0; i < BFcMaxSearchDepth; i++)
	{
		error = BFrFileRef_Set(outFileRef, temp);

		if(UUcError_None == error) {
			if (BFrFileRef_FileExists(outFileRef)) {
				return UUcError_None;
			}
		}

		sprintf(temp2, "..%c%s", BFcPathSeparator, temp);
		UUrString_Copy(temp, temp2, BFcMaxPathLength);
	}

	return BFcError_FileNotFound;
}

void
BFrFileRef_SetLeafNameSuffex(
	BFtFileRef	*inFileRef,
	const char	*inNewSuffix)
{
	UUtError	error;

	char		nameBuffer[BFcMaxPathLength];
	const char	*origName;
	char		*dotp;

 	origName = BFrFileRef_GetLeafName(inFileRef);

	UUrString_Copy(nameBuffer, origName, BFcMaxPathLength);

	if(*inNewSuffix == '.') inNewSuffix++;

	dotp = strrchr(nameBuffer, '.');
	if(dotp == NULL)
	{
		UUrString_Cat(nameBuffer, ".", BFcMaxPathLength);
		UUrString_Cat(nameBuffer, inNewSuffix, BFcMaxPathLength);
	}
	else
	{
		dotp++; // slip past '.'
		*dotp++ = *inNewSuffix++;
		*dotp++ = *inNewSuffix++;
		*dotp++ = *inNewSuffix++;
	}

 	error =
 		BFrFileRef_SetName(
	 		inFileRef,
	 		nameBuffer);
 	UUmAssert(error == UUcError_None);
}

UUtError
BFrFileRef_DuplicateAndReplaceSuffix(
	const BFtFileRef*	inOrigFileRef,
	const char*			inNewSuffix,
	BFtFileRef*			*outNewFileRef)
{
	UUtError	error;
	BFtFileRef*	newFileRef;

	error = BFrFileRef_Duplicate(inOrigFileRef, &newFileRef);
	UUmError_ReturnOnError(error);

	BFrFileRef_SetLeafNameSuffex(newFileRef, inNewSuffix);

	*outNewFileRef = newFileRef;

	return UUcError_None;
}

const char *
BFrFileRef_GetSuffixName(
	const BFtFileRef*		inFileRef)
{
	const char*	fileName;
	char*	suffix;

	fileName = BFrFileRef_GetLeafName(inFileRef);

	suffix = strrchr(fileName, '.');

	if(suffix == NULL)
	{
		return NULL;
	}

	return suffix+1;
}

UUtError
BFrFileRef_DuplicateAndReplaceName(
	const BFtFileRef*		inOrigFileRef,
	const char*		inReplaceName,
	BFtFileRef*		*outNewFileRef)
{
	UUtError	error;
	BFtFileRef*	newFileRef;

	error = BFrFileRef_Duplicate(inOrigFileRef, &newFileRef);
	UUmError_ReturnOnError(error);

	error = BFrFileRef_SetName(newFileRef, inReplaceName);
	UUmError_ReturnOnError(error);

	*outNewFileRef = newFileRef;

	return UUcError_None;
}

UUtError
BFrFileRef_DuplicateAndAppendName(
	const BFtFileRef*		inOrigFileRef,		// This had best be a directory...
	char*			inAppendName,
	BFtFileRef*		*outNewFileRef)
{
	UUtError	error;
	BFtFileRef*	newFileRef;
	const char*		leafName;
	char		newName[BFcMaxPathLength];

	error = BFrFileRef_Duplicate(inOrigFileRef, &newFileRef);
	UUmError_ReturnOnError(error);

	leafName = BFrFileRef_GetLeafName(inOrigFileRef);

	sprintf(newName, "%s%c%s", leafName, BFcPathSeparator, inAppendName);

	error = BFrFileRef_SetName(newFileRef, newName);
	UUmError_ReturnOnErrorMsgP(error, "Could not set name %s", (UUtUns32)newName, 0, 0);

	*outNewFileRef = newFileRef;

	return UUcError_None;
}

UUtError
BFrFileRef_AppendName(
	BFtFileRef*		inFileRef,		// This had best be a directory...
	char*			inAppendName)
{
	UUtError	error;
	const char*		leafName;
	char		newName[BFcMaxPathLength];

	leafName = BFrFileRef_GetLeafName(inFileRef);

	sprintf(newName, "%s%c%s", leafName, BFcPathSeparator, inAppendName);

	error = BFrFileRef_SetName(inFileRef, newName);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

UUtError
BFrTextFile_MapForRead(
	BFtFileRef*		inFileRef,
	BFtTextFile*	*outTextFile)
{
	UUtError		error;
	BFtTextFile*	newTextFile;
	UUtUns32		fileLength;

	UUmAssert(NULL != inFileRef);
	UUmAssert(NULL != outTextFile);

	*outTextFile = NULL;

	newTextFile = UUrMemory_Block_New(sizeof(BFtTextFile));
	if(newTextFile == NULL)
	{
		return UUcError_OutOfMemory;
	}

	error = BFrFile_Map(inFileRef, 0, &newTextFile->mapping, &newTextFile->fileMemory, &fileLength);

	// failed to load the file into memory free and return
	if (UUcError_None != error)
	{
		UUrMemory_Block_Delete(newTextFile);
		return error;
	}

	newTextFile->curStr = newTextFile->fileMemory;
	newTextFile->endFile = newTextFile->fileMemory + fileLength;

	*outTextFile = newTextFile;

	return UUcError_None;
}

UUtError
BFrTextFile_OpenForRead(
	BFtFileRef		*inFileRef,
	BFtTextFile*	*outTextFile)
{
	UUtError		error;
	BFtTextFile*	newTextFile;
	UUtUns32		fileLength;

	UUmAssert(NULL != inFileRef);
	UUmAssert(NULL != outTextFile);

	*outTextFile = NULL;

	newTextFile = UUrMemory_Block_New(sizeof(BFtTextFile));
	if(newTextFile == NULL)
	{
		return UUcError_OutOfMemory;
	}

	error = BFrFileRef_LoadIntoMemory(inFileRef, &fileLength, &newTextFile->fileMemory);

	// failed to load the file into memory free and return
	if (UUcError_None != error)
	{
		UUrMemory_Block_Delete(newTextFile);
		return error;
	}

	newTextFile->mapping = NULL;
	newTextFile->curStr = newTextFile->fileMemory;
	newTextFile->endFile = newTextFile->fileMemory + fileLength;

	*outTextFile = newTextFile;

	return UUcError_None;
}


UUtError
BFrTextFile_MakeFromString(
	const char*		inString,
	BFtTextFile*	*outTextFile)
{
	BFtTextFile*	newTextFile;

	UUmAssert(NULL != inString);
	UUmAssert(NULL != outTextFile);

	*outTextFile = NULL;

	newTextFile = UUrMemory_Block_New(sizeof(BFtTextFile));
	if(newTextFile == NULL)
	{
		return UUcError_OutOfMemory;
	}

	newTextFile->mapping = NULL;
	newTextFile->fileMemory = UUrMemory_Block_New(strlen(inString) + 1);
	UUmError_ReturnOnNull(newTextFile->fileMemory);

	strcpy(newTextFile->fileMemory, inString);

	newTextFile->curStr = newTextFile->fileMemory;

	newTextFile->endFile = newTextFile->fileMemory + strlen(newTextFile->fileMemory);

	*outTextFile = newTextFile;

	return UUcError_None;
}

void
BFrTextFile_Close(
	BFtTextFile*	inTextFile)
{
	UUmAssert(NULL != inTextFile);

	if (inTextFile->mapping != NULL) {
		BFrFile_UnMap(inTextFile->mapping);
	}
	else {
		UUrMemory_Block_Delete(inTextFile->fileMemory);
	}

	UUrMemory_Block_Delete(inTextFile);

	return;
}

char *
BFrTextFile_GetCurStr(
	BFtTextFile*	inTextFile)
{
	return inTextFile->curStr;
}


char*
BFrTextFile_GetNextStr(
	BFtTextFile*	inTextFile)
{
	char	*returnStr;
	char	*p;

	UUmAssert(NULL != inTextFile);

	p = returnStr = inTextFile->curStr;

	if(p >= inTextFile->endFile)
	{
		return NULL;
	}

	while(*p != '\n' && *p != '\r' && *p != '\0' && p < inTextFile->endFile) p++;

	if(*p == '\r')
	{
		*p++ = 0;
		if(*p == '\n') *p++ = 0;
	}
	else if(*p == '\n')
	{
		*p++ = 0;
		if(*p == '\r') *p++ = 0;
	}
	else if (p == inTextFile->endFile)
	{
		// when we are doing this to file-mapped files, we are writing to 1 byte past the
		// file, which is technically illegal. Fortunately Windows seems to be letting this slide.
		*p= 0;
	}

	inTextFile->curStr = p;

	return returnStr;
}

char*
BFrTextFile_GetNextNWords(
	UUtUns16		inNumWordsToRead,
	BFtTextFile*	inTextFile)
{
	char	*returnStr;
	char	*p;
	short	n;

	UUmAssert(NULL != inTextFile);

	p = returnStr = inTextFile->curStr;

	if(p >= inTextFile->endFile)
	{
		return NULL;
	}

	for (n=0; n<inNumWordsToRead; n++)
	{
		while(*p != '\0' && !isspace(*p)) p++;

		if (n<inNumWordsToRead-1) while (isspace(*p)) *p++;
	}
	while (isspace(*p)) *p++ = 0;

	inTextFile->curStr = p;

	return returnStr;
}

UUtError
BFrTextFile_VerifyNextNWords(
	char			*inMatchString,
	UUtUns16		inNumWordsToRead,
	BFtTextFile*	inTextFile)
{
	char *string;
	UUtBool stringsAreEqual;

	string = BFrTextFile_GetNextNWords(inNumWordsToRead,inTextFile);
	if (!string) return UUcError_Generic;

	stringsAreEqual = (0 == strcmp(string,inMatchString));
	if (!stringsAreEqual) return UUcError_Generic;

	return UUcError_None;
}

UUtUns32
BFrTextFile_GetUUtUns32(
	BFtTextFile*	inTextFile)
{
	char			*line;
	int				fieldsScanned;
	unsigned int	scanned;
	UUtUns32		result;


	line = BFrTextFile_GetNextStr(inTextFile);

	fieldsScanned = sscanf(line, "%u", &scanned);

	UUmAssert(1 == fieldsScanned);

	result = scanned;

	return result;
}

UUtUns16
BFrTextFile_GetUUtUns16(
	BFtTextFile*	inTextFile)
{
	char			*line;
	int				fieldsScanned;
	unsigned int	scanned;
	UUtUns16		result;


	line = BFrTextFile_GetNextStr(inTextFile);

	fieldsScanned = sscanf(line, "%u", &scanned);

	UUmAssert(1 == fieldsScanned);
	UUmAssert(scanned >= UUcMinUns16);
	UUmAssert(scanned <= UUcMaxUns16);

	result = (UUtUns16) (scanned);

	return result;
}

UUtInt32
BFrTextFile_GetUUtInt32(
	BFtTextFile*	inTextFile)
{
	char			*line;
	int				fieldsScanned;
	int				scanned;
	UUtInt32		result;


	line = BFrTextFile_GetNextStr(inTextFile);

	fieldsScanned = sscanf(line, "%d", &scanned);

	UUmAssert(1 == fieldsScanned);

	result = scanned;

	return result;
}


UUtInt16
BFrTextFile_GetUUtInt16(
	BFtTextFile*	inTextFile)
{
	char			*line;
	int				fieldsScanned;
	int				scanned;
	UUtInt16		result;


	line = BFrTextFile_GetNextStr(inTextFile);

	fieldsScanned = sscanf(line, "%d", &scanned);

	UUmAssert(1 == fieldsScanned);
	UUmAssert(scanned >= UUcMinInt16);
	UUmAssert(scanned <= UUcMaxInt16);

	result = (UUtInt16) (scanned);

	return result;
}

float
BFrTextFile_GetFloat(
	BFtTextFile*	inTextFile)
{
	char			*line;
	int				fieldsScanned;
	float			result;


	line = BFrTextFile_GetNextStr(inTextFile);

	fieldsScanned = sscanf(line, "%f", &result);

	UUmAssert(1 == fieldsScanned);

	return result;
}

UUtUns32 BFrTextFile_GetOSType(BFtTextFile*	inTextFile)
{
	char *line;
	UUtUns32  result;

	line = BFrTextFile_GetNextStr(inTextFile);
	UUmAssert(line[0] != 0);
	UUmAssert(line[1] != 0);
	UUmAssert(line[2] != 0);
	UUmAssert(line[3] != 0);

	result = UUm4CharToUns32(line[0], line[1], line[2], line[3]);

	return result;
}

UUtError
BFrTextFile_VerifyNextStr(
	BFtTextFile*	inTextFile,
	const char*			inString)
{
	char *curLine;
	UUtBool success;

	UUmAssert(NULL != inTextFile);
	UUmAssert(NULL != inString);

	curLine = BFrTextFile_GetNextStr(inTextFile);

	if (!curLine)
	{
		return UUcError_Generic;
		/*UUmError_ReturnOnErrorMsgP(
			UUcError_Generic,
			"Expected string \"%s\", found NULL",
			(UUtUns32)inString,
			(UUtUns32)curLine,
			0);*/
	}

	success = (0 == strcmp(curLine, inString));

	if(!success)
	{
		return UUcError_Generic;
		/*UUmError_ReturnOnErrorMsgP(
			UUcError_Generic,
			"Expected string \"%s\", found string \"%s\"",
			(UUtUns32)inString,
			(UUtUns32)curLine,
			0);*/
	}

	return UUcError_None;
}


static UUtBool iCharIsLetter(char character)
{
	if ((character >= 'a') && (character <= 'z'))
	{
		return UUcTrue;
	}

	if ((character >= 'A') && (character <= 'Z'))
	{
		return UUcTrue;
	}

	return UUcFalse;
}

UUtBool
BFrPath_IsAbsolute(
	const char*			inPath)
{
#if UUmPlatform == UUmPlatform_Win32
	UUtBool networkDrive = ('\\' == inPath[0]) && ('\\' == inPath[1]);
	UUtBool localDrive = iCharIsLetter(inPath[0]) && (':' == inPath[1]);
	UUtBool absolutePath = networkDrive || localDrive;

	return absolutePath;
#elif UUmPlatform == UUmPlatform_Mac
        return UUcTrue;
#elif UUmPlatform == UUmPlatform_Linux
	return inPath[0] == '/';
#else
        #error What does this filesystem look like?
#endif
}

char *
BFrPath_To_Name(
	char*				inPath)
{
	char *scan;
	char *lastSlash = inPath;

	for(scan = inPath; *scan != '\0'; scan++)
	{
		if (BFcPathSeparator == *scan)
		{
			lastSlash = scan + 1;
		}
	}

	return lastSlash;
}

UUtError
BFrFile_ReadPos(
	BFtFile*	inFile,
	UUtUns32	inStartPos,
	UUtUns32	inLength,
	void*		inDataPtr)
{
	UUtError error;

	error = BFrFile_SetPos(inFile, inStartPos);

	if (UUcError_None == error) {
		error = BFrFile_Read(inFile, inLength, inDataPtr);
	}

	return error;
}


UUtError
BFrFile_WritePos(
	BFtFile*	inFile,
	UUtUns32	inStartPos,
	UUtUns32	inLength,
	void*		inDataPtr)
{
	UUtError error;

	error = BFrFile_SetPos(inFile, inStartPos);

	if (UUcError_None == error) {
		error = BFrFile_Write(inFile, inLength, inDataPtr);
	}

	return error;
}

#if 0
UUtError
BFrFile_Async_ReadPos(
	BFtFile*			inFile,
	UUtUns32			inStartPos,
	UUtUns32			inLength,
	void*				inDataPtr,
	BFtCompletion_Func	inCompletionFunc,
	void*				inRefCon)
{
	UUtError error;

	error = BFrFile_SetPos(inFile, inStartPos);

	if (UUcError_None == error) {
		error = BFrFile_Async_Read(inFile, inLength, inDataPtr, inCompletionFunc, inRefCon);
	}

	return error;
}

UUtError
BFrFile_Async_WritePos(
	BFtFile*			inFile,
	UUtUns32			inStartPos,
	UUtUns32			inLength,
	void*				inDataPtr,
	BFtCompletion_Func	inCompletionFunc,
	void*				inRefCon)
{
	UUtError error;

	error = BFrFile_SetPos(inFile, inStartPos);

	if (UUcError_None == error) {
		error = BFrFile_Async_Write(inFile, inLength, inDataPtr, inCompletionFunc, inRefCon);
	}

	return error;
}
#endif

UUtError
UUcArglist_Call
BFrFile_Printf(
	BFtFile*	inFile,
	const char *format,
	...)
{
	UUtError error;
	char msg[3 * 1024];
	va_list arglist;
	int return_value;

	va_start(arglist, format);
	return_value= vsprintf(msg, format, arglist);
	va_end(arglist);

	error = BFrFile_Write(inFile, strlen(msg), msg);

	return error;
}

UUtError
BFrFile_MapClassic(
	BFtFileRef	*inFileRef,
	UUtUns32			inOffset,
	BFtFile				**outFile,
	void				**outPtr,
	UUtUns32			*outSize)
{
	UUtError		error;
	BFtFile*		file;
	char*			newMemory;
	UUtUns32		size;

	UUmAssert(NULL != inFileRef);
	UUmAssert(NULL != outSize);
	UUmAssert(NULL != outPtr);

	*outFile = NULL;
	*outPtr = NULL;
	*outSize = 0;

	if (!BFrFileRef_FileExists(inFileRef)) {
		return BFcError_FileNotFound;
	}

	// to mirror the behavior of windows, try "rw" then "r"
	error = BFrFile_Open(inFileRef, "rw", &file);
	if (UUcError_None != error) { error = BFrFile_Open(inFileRef, "r", &file); }
	if (error) { return error; }

	error = BFrFile_GetLength(file, &size);
	UUmError_ReturnOnError(error);

	if (size > inOffset) {
		size -= inOffset;
	} else {
		size = 0;
	}

	if (size == 0) {
		newMemory = NULL;
	} else {
		newMemory = UUrMemory_Block_New(size);
		if(newMemory == NULL)
		{
			UUmError_ReturnOnError(UUcError_OutOfMemory);
		}

		error = BFrFile_ReadPos(file, inOffset, size, newMemory);
		UUmError_ReturnOnError(error);
	}

	*outFile = file;
	*outPtr = newMemory;
	*outSize = size;

	return UUcError_None;
}

BFtFile *BFrFile_FOpen(char *inName, char *inMode)
{
	UUtError error;
	BFtFileRef fileRef;
	BFtFile *file;

	error = BFrFileRef_Set(&fileRef, inName);
	if (error != UUcError_None) {
		goto errorExit;
	}

	error = BFrFile_Open(&fileRef, inMode, &file);
	if (error != UUcError_None) {
		goto errorExit;
	}

	return file;

errorExit:
	return NULL;
}

UUtError
BFrFileRef_MakeFromName(
	const char	*inFileName,
	BFtFileRef	**outFileRef)
{
	UUtError error = UUcError_None;
	BFtFileRef *new_file_ref = UUrMemory_Block_New(sizeof(BFtFileRef));

	if (NULL == new_file_ref) {
		error = UUcError_OutOfMemory;
		goto exit;
	}

	error = BFrFileRef_Set(new_file_ref, inFileName);
	if (error != UUcError_None) {
		UUrMemory_Block_Delete(new_file_ref);
		goto exit;
	}

	*outFileRef = new_file_ref;

exit:
	return error;
}


BFtFileRef *BFrFileRef_MakeUnique(const char *prefix, const char *suffix, UUtInt32 *ioNumber, UUtInt32 inMaxNumber)
{
	BFtFileRef *fileRef;
	char fileName[256];
	UUtInt32 count = *ioNumber;
	UUtInt32 max_digits = UUrString_CountDigits(inMaxNumber);

	while(1)
	{
		UUtError error;
		char number_buffer[32];

		UUrString_PadNumber(count, max_digits, number_buffer);

		sprintf(fileName, "%s%s%s", prefix, number_buffer, suffix);

		error = BFrFileRef_MakeFromName(fileName, &fileRef);

		if (error) {
			break;
		}

		if (!BFrFileRef_FileExists(fileRef)) {
			break;
		}

		BFrFileRef_Dispose(fileRef);
		fileRef = NULL;

		count++;

		if (count > inMaxNumber) {
			break;
		}
	}

	*ioNumber = count;

	return fileRef;
}

// ------------------------------------------------------------------------------------
// -- recursive file cache

struct BFtFileCacheEntry
{
	BFtFileRef *reference;				// pointer into cache->references array
	const char *leafName;				// cached leaf name for this reference
};

struct BFtFileCache
{
	UUtBool preparedForUse;
	UUtUns32 numEntries;
	BFtFileRef *references;				// array of file references
	struct BFtFileCacheEntry *entries;	// sorted alphabetically, case-insensitively
};

#define BFcFileCache_ChunkSize 128

static int UUcExternal_Call BFiFileCache_Sort(const void *inEntryA, const void *inEntryB)
{
	const struct BFtFileCacheEntry *entry_a = (struct BFtFileCacheEntry *) inEntryA;
	const struct BFtFileCacheEntry *entry_b = (struct BFtFileCacheEntry *) inEntryB;

	return UUrString_Compare_NoCase(entry_a->leafName, entry_b->leafName);
}

UUtError BFrFileCache_PrepareForUse(BFtFileCache *inFileCache)
{
	UUtUns32 itr;
	struct BFtFileCacheEntry *entry;

	if (inFileCache->preparedForUse) {
		return UUcError_None;

	} else if (inFileCache->numEntries == 0) {
		goto success;
	}

	inFileCache->entries = UUrMemory_Block_Realloc(inFileCache->entries, sizeof(struct BFtFileCacheEntry) * inFileCache->numEntries);
	UUmError_ReturnOnNull(inFileCache->entries);

	for (itr = 0, entry = inFileCache->entries; itr < inFileCache->numEntries; itr++, entry++) {
		entry->reference = inFileCache->references + itr;
		entry->leafName = BFrFileRef_GetLeafName(entry->reference);
	}

	// sort the file cache
	qsort(inFileCache->entries, inFileCache->numEntries, sizeof(struct BFtFileCacheEntry), BFiFileCache_Sort);

success:
	inFileCache->preparedForUse = UUcTrue;
	return UUcError_None;
}

static UUtError BFrFileCache_Expand(BFtFileCache *inCache, BFtFileRef *inDirRef, BFtCacheDepth inDepth)
{
	UUtError				error;
	BFtFileIterator			*file_iterator;

	// if we have previously been prepared for use, this invalidates our current state
	inCache->preparedForUse = UUcFalse;

	// create a file iterator for the binary directory
	error =	BFrDirectory_FileIterator_New(inDirRef,	NULL, NULL,	&file_iterator);
	UUmError_ReturnOnError(error);

	while (1)
	{
		BFtFileRef file_ref;

		error = BFrDirectory_FileIterator_Next(file_iterator, &file_ref);
		if (error != UUcError_None) { break; }

		if (BFrFileRef_IsDirectory(&file_ref)) {
			if (BFcFileCache_Recursive == inDepth) {
				error = BFrFileCache_Expand(inCache, &file_ref, inDepth);
			}
		}
		else {
			if (0 == (inCache->numEntries % BFcFileCache_ChunkSize)) {
				inCache->references = UUrMemory_Block_Realloc(inCache->references, sizeof(BFtFileRef) * (inCache->numEntries + BFcFileCache_ChunkSize));
			}

			inCache->references[inCache->numEntries] = file_ref;
			inCache->numEntries++;
		}

		if (error != UUcError_None) { break; }
	}

	// delete the file iterator
	BFrDirectory_FileIterator_Delete(file_iterator);
	file_iterator = NULL;

	return UUcError_None;
}

BFtFileCache *BFrFileCache_New(BFtFileRef *inDirRef, BFtCacheDepth inDepth)
{
	BFtFileCache *cache = UUrMemory_Block_NewClear(sizeof(BFtFileCache));
	UUtError error;

	if (NULL == cache) {
		goto exit;
	}

	error = BFrFileCache_Expand(cache, inDirRef, inDepth);
	if (UUcError_None != error) {
		BFrFileCache_Dispose(cache);
		cache = NULL;
	}

exit:
	return cache;
}

void BFrFileCache_Dispose(BFtFileCache *inFileCache)
{
	if (inFileCache->entries != NULL) {
		UUrMemory_Block_Delete(inFileCache->entries);
	}
	if (inFileCache->references != NULL) {
		UUrMemory_Block_Delete(inFileCache->references);
	}

	UUrMemory_Block_Delete(inFileCache);

	return;
}

BFtFileRef *BFrFileCache_Search(BFtFileCache *inFileCache, const char *inString, const char *inSuffix)
{
	char search_name[BFcMaxFileNameLength * 2];
	struct BFtFileCacheEntry *entry;
	BFtFileRef *result = NULL;

	UUmAssert(inFileCache->preparedForUse);
	if (inFileCache->entries == NULL) {
		return NULL;
	}

	if (NULL == inSuffix) {
		strcpy(search_name, inString);
	}
	else {
		sprintf(search_name, "%s%s", inString, inSuffix);
	}

#if 1
	{
		// binary search on sorted entry list
		UUtInt32 min, max, midpoint, compare;

		min = 0;
		max = inFileCache->numEntries - 1;

		while(min <= max) {
			// S.S. midpoint = (min + max) / 2;
			midpoint= (min + max) >> 1;

			entry = inFileCache->entries + midpoint;
			compare = UUrString_Compare_NoCase(search_name, entry->leafName);

			if (compare == 0) {
				// found, return
				result = entry->reference;
				goto exit;

			} else if (compare < 0) {
				// must lie below here
				max = midpoint - 1;

			} else {
				// must lie above here
				min = midpoint + 1;
			}
		}
	}
#else
	{
		// linear search on entry list
		UUtUns32 itr;

		for(itr = 0, entry = inFileCache->entries; itr < inFileCache->numEntries; itr++, entry++)
		{
			if (0 == UUrString_Compare_NoCase(search_name, entry->leafName)) {
				result = entry->reference;
				goto exit;
			}
		}
	}
#endif

exit:
	return result;
}

UUtUns32 BFrFileCache_Count(BFtFileCache *inFileCache)
{
	UUtUns32 count = inFileCache->numEntries;

	return count;
}

BFtFileRef *BFrFileCache_GetIndex(BFtFileCache *inFileCache, UUtUns32 inIndex)
{
	BFtFileRef *result = &inFileCache->references[inIndex];

	UUmAssert(inIndex < inFileCache->numEntries);

	return result;
}




UUtBool BFgDebugFileEnable = UUcFalse;	// needs to be set on during startup

BFtDebugFile *BFrDebugFile_Open(const char *inName)
{
	FILE *stream;
#ifdef SUPPORT_DEBUG_FILES
	UUtBool support_debug_files = BFgDebugFileEnable;
#else
	UUtBool support_debug_files = UUcFalse;
#endif

	if (support_debug_files) {
		stream = fopen(inName, "w");
	}
	else {
		stream = (FILE *) 0xFFFFFFFF;
	}

	return (BFtDebugFile *) stream;
}

void UUcArglist_Call BFrDebugFile_Printf(BFtDebugFile *inFile, const char *format, ...)
{
	FILE *stream = (FILE *) inFile;
#ifdef SUPPORT_DEBUG_FILES
	UUtBool support_debug_files = BFgDebugFileEnable;
#else
	UUtBool support_debug_files = UUcFalse;
#endif

	if (support_debug_files) {
		if (NULL != stream) {
			char msg[4 * 1024];
			va_list arglist;
			int return_value;

			va_start(arglist, format);
			return_value= vsprintf(msg, format, arglist);
			va_end(arglist);

			fprintf(stream, "%s", msg);
			fflush(stream);
		}
	}

	return;
}

void BFrDebugFile_Close(BFtDebugFile *inDebugFile)
{
	FILE *stream = (FILE *) inDebugFile;
#ifdef SUPPORT_DEBUG_FILES
	UUtBool support_debug_files = BFgDebugFileEnable;
#else
	UUtBool support_debug_files = UUcFalse;
#endif

	if (support_debug_files) {
		if (NULL != stream) {
			fclose(stream);
		}
	}

	return;
}
