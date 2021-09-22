#pragma once
/*
	FILE:	BFW_FileManager.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 28, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef BFW_FILEMANAGER_H
#define BFW_FILEMANAGER_H

#include "BFW.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BFcMaxPathLength		(255)	// This is imposed by Mac OS 9
#define BFcMaxFileNameLength	(32)	// This includes the 0 terminater!!!!

/*
 * Define some errors
 */

	
/*
 * The inMode parameter to BFrFile_Open can be one of the following
 *	"r" - open for reading
 *	"w" - open for writing
 *	"rw" - open for reading and writing
 */

/*
 * A file ref can point to a directory or to a file.
 * A file ref is either a absolute file path or a partial path derived
 * from the current working directory which is the directory containing the
 * logical location of the executable file.
 * A file ref does not need to point to a file that exists but all directories
 * in the path must exist.
 */

#if UUmPlatform == UUmPlatform_Win32

#define BFcPathSeparator '\\'

typedef struct BFtFileRef
{
	char	name[BFcMaxPathLength];
} BFtFileRef;

#elif UUmPlatform == UUmPlatform_Mac

#define BFcPathSeparator ':'

typedef struct BFtFileRef
{
	FSSpec		fsSpec;
	char		leafName[64];	// This should only be the name of the base file or folder
	
} BFtFileRef;

#elif UUmPlatform == UUmPlatform_Linux

#define BFcPathSeparator '/'

typedef struct BFtFileRef
{
	char	name[BFcMaxPathLength];
} BFtFileRef;

#else
#error what is a BFtFileRef?
#error what is BFcPathSeparator?
#endif

typedef struct BFtFile				BFtFile;
typedef struct BFtTextFile			BFtTextFile;
typedef struct BFtFileIterator		BFtFileIterator;
typedef struct BFtFileMapping		BFtFileMapping;
typedef struct BFtFileCache			BFtFileCache;

typedef void
(*BFtCompletion_Func)(
	void*	inRefCon);

void
BFrFile_CompletionFunc_Bool(
	void*	inBoolAddr);

#if UUmPlatform == UUmPlatform_Mac

	UUtError
	BFrFileRef_MakeFromFSSpec(
		const FSSpec*	inFSSpec,
		BFtFileRef*		*outFileRef);
	
	FSSpec*
	BFrFileRef_GetFSSpec(
		BFtFileRef		*inFileRef);
	
#elif UUmPlatform == UUmPlatform_Win32 || UUmPlatform == UUmPlatform_Linux

#else

	#error implement me
	
#endif

UUtError
BFrFileRef_MakeFromName(
	const char*		inFileName,
	BFtFileRef*		*outFileRef);

UUtError
BFrFileRef_Search(
	const char*		inFileName,
	BFtFileRef		*outFileRef);

BFtFileRef *
BFrFileRef_MakeUnique(
	const char	*inPrefix, 
	const char	*inSuffix, 
	UUtInt32	*ioNumber, 
	UUtInt32	inMaxNumber);
	
void
BFrFileRef_Dispose(
	BFtFileRef*		inFileRef);

UUtError
BFrFileRef_Duplicate(
	const BFtFileRef*		inOrigFileRef,
	BFtFileRef*		*outNewFileRef);

const char *
BFrFileRef_GetLeafName(
	const BFtFileRef*		inFileRef);

const char *
BFrFileRef_GetFullPath(
	const BFtFileRef*		inFileRef);

UUtError
BFrFileRef_GetParentDirectory(
	BFtFileRef	*inFileRef,
	BFtFileRef	**outParentDirectory);
	
const char *
BFrFileRef_GetSuffixName(
	const BFtFileRef*		inFileRef);

UUtBool
BFrFileRef_IsDirectory(
	BFtFileRef*		inFileRef);

UUtBool
BFrFileRef_IsLocked(
	BFtFileRef*		inFileRef);
	
UUtBool
BFrFileRef_IsEqual(
	const BFtFileRef	*inFileRef1,
	const BFtFileRef	*inFileRef2);
	
UUtError
BFrFileRef_SetName(
	BFtFileRef*		inFileRef,
	const char*		inName);			// Could be an absolute path or a relative path

UUtError
BFrFileRef_Set(
	BFtFileRef*		inFileRef,
	const char*		inName);			// like make from name but does not create

void
BFrFileRef_SetLeafNameSuffex(
	BFtFileRef*		inFileRef,
	const char*		inLeafNameSuffex);

UUtError
BFrFileRef_DuplicateAndReplaceSuffix(
	const BFtFileRef*	inOrigFileRef,
	const char*			inNewSuffix,
	BFtFileRef*			*outNewFileRef);

UUtError
BFrFileRef_DuplicateAndReplaceName(
	const BFtFileRef*	inOrigFileRef,
	const char*			inReplaceName,		// Could be an absolute path or a relative path
	BFtFileRef*			*outNewFileRef);
	
UUtError
BFrFileRef_DuplicateAndAppendName(
	const BFtFileRef*		inOrigFileRef,		// This had best be a directory...
	char*			inAppendName,		// This should only be a relative path
	BFtFileRef*		*outNewFileRef);

UUtError
BFrFileRef_AppendName(
	BFtFileRef*		inFileRef,			// This had best be a directory...
	char*			inAppendName);		// This should only be a relative path

UUtBool
BFrFileRef_FileExists(
	BFtFileRef*		inFileRef);

UUtError
BFrFileRef_GetModTime(
	BFtFileRef*		inFileRef,
	UUtUns32*		outSecsSince1900);
	
UUtError
BFrFile_Create(
	BFtFileRef*		inFileRef);

UUtError
BFrFile_Delete(
	BFtFileRef*		inFileRef);
	
UUtError
BFrFile_Open(
	BFtFileRef*		inFileRef,
	char*			inMode,
	BFtFile*		*outFile);

BFtFile *
BFrFile_FOpen(char *inName, char *inMode);

UUtError
BFrFile_Map(
	BFtFileRef	*inFileRef,
	UUtUns32			inOffset,
	BFtFileMapping		**outMapping,
	void				**outPtr,
	UUtUns32			*outSize);

void
BFrFile_UnMap(
	BFtFileMapping	*inMapping);
	
UUtError
BFrFile_Flush(
	BFtFile*		inFile);

void
BFrFile_Close(
	BFtFile*		inFile);

UUtError
BFrFile_GetLength(
	BFtFile*		inFile,
	UUtUns32*		outLength);

UUtError
BFrFile_SetEOF(
	BFtFile*		inFile);
	
UUtError
BFrFile_SetPos(
	BFtFile*		inFile,
	UUtUns32		inPos);

UUtError
BFrFile_GetPos(
	BFtFile*		inFile,
	UUtUns32		*outPos);

UUtError
BFrFile_Async_Read(
	BFtFile*			inFile,
	UUtUns32			inLength,
	void*				inDataPtr,
	BFtCompletion_Func	inCompletionFunc,
	void*				inRefCon);
	
UUtError
BFrFile_Async_Write(
	BFtFile*			inFile,
	UUtUns32			inLength,
	void*				inDataPtr,
	BFtCompletion_Func	inCompletionFunc,
	void*				inRefCon);

UUtError
BFrFile_Async_ReadPos(
	BFtFile*			inFile,
	UUtUns32			inStartPos,
	UUtUns32			inLength,
	void*				inDataPtr,
	BFtCompletion_Func	inCompletionFunc,
	void*				inRefCon);

UUtError
BFrFile_Async_WritePos(
	BFtFile*			inFile,
	UUtUns32			inStartPos,
	UUtUns32			inLength,
	void*				inDataPtr,
	BFtCompletion_Func	inCompletionFunc,
	void*				inRefCon);

UUtError
BFrFile_Read(
	BFtFile*	inFile,
	UUtUns32	inLength,
	void*		inDataPtr);

UUtError
BFrFile_Write(
	BFtFile*	inFile,
	UUtUns32	inLength,
	void*		inDataPtr);

UUtError
BFrFile_ReadPos(
	BFtFile*	inFile,
	UUtUns32	inStartPos,
	UUtUns32	inLength,
	void*		inDataPtr);

UUtError
BFrFile_WritePos(
	BFtFile*	inFile,
	UUtUns32	inStartPos,
	UUtUns32	inLength,
	void*		inDataPtr);

UUtError 
UUcArglist_Call 
BFrFile_Printf(
	BFtFile*	inFile,
	const char *format, 
	...);

UUtError
BFrTextFile_OpenForRead(
	BFtFileRef*		inFileRef,
	BFtTextFile*	*outTextFile);

UUtError
BFrTextFile_MapForRead(
	BFtFileRef*		inFileRef,
	BFtTextFile*	*outTextFile);

UUtError
BFrTextFile_MakeFromString(
	const char*		inString,
	BFtTextFile*	*outTextFile);

void
BFrTextFile_Close(
	BFtTextFile*	inTextFile);

char *
BFrTextFile_GetCurStr(
	BFtTextFile*	inTextFile);
	
char*
BFrTextFile_GetNextStr(
	BFtTextFile*	inTextFile);

UUtUns32
BFrTextFile_GetUUtUns32(
	BFtTextFile*	inTextFile);

UUtUns16
BFrTextFile_GetUUtUns16(
	BFtTextFile*	inTextFile);

UUtInt32
BFrTextFile_GetUUtInt32(
	BFtTextFile*	inTextFile);

UUtInt16
BFrTextFile_GetUUtInt16(
	BFtTextFile*	inTextFile);

float
BFrTextFile_GetFloat(
	BFtTextFile*	inTextFile);

char*
BFrTextFile_GetNextNWords(
	UUtUns16		inNumWordsToRead,
	BFtTextFile*	inTextFile);

UUtError
BFrTextFile_VerifyNextNWords(
	char			*inMatchString,
	UUtUns16		inNumWordsToRead,
	BFtTextFile*	inTextFile);
	
UUtUns32
BFrTextFile_GetOSType(
	BFtTextFile*	inTextFile);

UUtError
BFrTextFile_VerifyNextStr(
	BFtTextFile*	inTextFile,
	const char*		string);

#define BFmTextFile_RequireNextStr(inTextFile, inString)					\
do {																		\
	UUtError macroError = BFrTextFile_VerifyNextStr(inTextFile, inString);	\
	if (macroError != UUcError_None) { return macroError; }					\
} while(0)																	
	
UUtError
BFrFileRef_LoadIntoMemory(
	BFtFileRef*	inFileRef,
	UUtUns32*	outFileLength,
	void*		*outMemory);

// This function returns a *sorted* file list
UUtError
BFrDirectory_GetFileList(
	BFtFileRef*	inDirectoryRef,		/* The file ref for the desired directory */
	char*		inPrefix,			/* The case insensitive prefix name for returned files, NULL for none */
	char*		inSuffix,			/* The case insensitive suffix name for returned files, NULL for none */
	UUtUns16	inMaxFiles,			/* The maximum number of files to find */
	UUtUns16	*outNumFiles,		/* The number of files returned */
	BFtFileRef*	*outFileRefArray);	/* A pointer to the array of BFtFileRef for the returned files */

// The file iterator functions returns the list of files in an undefined platform specific order
UUtError
BFrDirectory_FileIterator_New(
	BFtFileRef*			inDirectoryRef,		/* The file ref for the desired directory */
	char*				inPrefix,			/* The case insensitive prefix name for returned files, NULL for none */
	char*				inSuffix,			/* The case insensitive suffix name for returned files, NULL for none */
	BFtFileIterator*	*outFileIterator);	/* The find iterator to use in BFrDirectory_FileIterator_Delete */

UUtError
BFrDirectory_FileIterator_Next(
	BFtFileIterator*	inFileIterator,
	BFtFileRef			*outFileRef);

void
BFrDirectory_FileIterator_Delete(
	BFtFileIterator*	inFileIterator);

UUtError
BFrDirectory_DeleteContentsOnly(
	BFtFileRef*	inDirectoryRef);

UUtError
BFrDirectory_DeleteDirectoryAndContents(
	BFtFileRef*	inDirectoryRef);

UUtError
BFrDirectory_Create(
	BFtFileRef*	inDirRef,
	char*		inDirName);	// if non-null then inDirRef refers to the parent dir

// ***** cache functions ******

typedef enum BFtCacheDepth {
	BFcFileCache_Flat,
	BFcFileCache_Recursive
} BFtCacheDepth;

BFtFileCache *BFrFileCache_New(BFtFileRef *inDirRef, BFtCacheDepth inDepth);
void BFrFileCache_Dispose(BFtFileCache *inFileCache);
BFtFileRef *BFrFileCache_Search(BFtFileCache *inFileCache, const char *inString, const char *inSuffix);
UUtUns32 BFrFileCache_Count(BFtFileCache *inFileCache);
BFtFileRef *BFrFileCache_GetIndex(BFtFileCache *inFileCache, UUtUns32 inIndex);
UUtError BFrFileCache_PrepareForUse(BFtFileCache *inFileCache);


// ***** debug file functions *****

#if SHIPPING_VERSION
#define SUPPORT_DEBUG_FILES 0
#else
#define SUPPORT_DEBUG_FILES 1
#endif

typedef struct BFtDebugFile BFtDebugFile;

extern UUtBool BFgDebugFileEnable;	// needs to be set on during startup

BFtDebugFile *BFrDebugFile_Open(const char *inName);
void UUcArglist_Call BFrDebugFile_Printf(BFtDebugFile *inFile, const char *format, ...);
void BFrDebugFile_Close(BFtDebugFile *inDebugFile);

//***************************** path functions **************************
//
// operate on pc style paths

char *
BFrPath_To_Name(
	char*				inPath);

UUtBool
BFrPath_IsAbsolute(
	const char*			inPath);

#ifdef __cplusplus
}
#endif

#endif /* BFW_FILEMANAGER_H */
