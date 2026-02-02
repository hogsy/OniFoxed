
/*
	FILE:	BFW_FileManager_Win32.c

	AUTHOR:	Brent H. Pease

	CREATED: June 28, 1997

	PURPOSE:

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_AppUtilities.h"

#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <io.h>

//#include <iostream.h>

// resides in FileManager_Common.c but no public prototype
UUtError BFrFile_MapClassic(BFtFileRef	*inFileRef,
	UUtUns32			inOffset,
	BFtFile				**outFile,
	void				**outPtr,
	UUtUns32			*outSize);


#if 0
struct BFtFileMapping
{
	BFtFile	*file;
	void	*ptr;
};
#else
struct BFtFileMapping
{
	char	name[BFcMaxPathLength];
	BFtFile	*classicFile;						// if we are using classic mapping
	HANDLE	fileHandle;
	HANDLE	fileMappingHandle;
	void	*ptr;
};
#endif

UUtUns32 UUrSystemToSecsSince1900(SYSTEMTIME time);

static const char *WindowsErrorToString(DWORD error)
{
	const char *error_msg;

	switch(error)
	{
		case ERROR_INVALID_FUNCTION: error_msg = "Invalid function."; break;
		case ERROR_FILE_NOT_FOUND: error_msg = "The system cannot find the file specified."; break;
		case ERROR_PATH_NOT_FOUND: error_msg = "The system cannot find the path specified."; break;
		case ERROR_TOO_MANY_OPEN_FILES: error_msg = "The system cannot open the file."; break;
		case ERROR_ACCESS_DENIED: error_msg = "Access is denied."; break;
		case ERROR_INVALID_HANDLE: error_msg = "The handle is invalid."; break;
		case ERROR_ARENA_TRASHED: error_msg = "The storage control blocks were destroyed."; break;
		case ERROR_NOT_ENOUGH_MEMORY: error_msg = "Not enough storage is available to process this command."; break;
		case ERROR_INVALID_BLOCK: error_msg = "The storage control block address is invalid."; break;
		case ERROR_BAD_ENVIRONMENT: error_msg = "The environment is incorrect."; break;
		case ERROR_BAD_FORMAT: error_msg = "An attempt was made to load a program with an incorrect format."; break;
		case ERROR_INVALID_ACCESS: error_msg = "The access code is invalid."; break;
		case ERROR_INVALID_DATA: error_msg = "The data is invalid."; break;
		case ERROR_OUTOFMEMORY: error_msg = "Not enough storage is available to complete this operation."; break;
		case ERROR_INVALID_DRIVE: error_msg = "The system cannot find the drive specified."; break;
		case ERROR_CURRENT_DIRECTORY: error_msg = "The directory cannot be removed."; break;
		case ERROR_NOT_SAME_DEVICE: error_msg = "The system cannot move the file to a different disk drive."; break;
		case ERROR_NO_MORE_FILES: error_msg = "There are no more files."; break;
		case ERROR_WRITE_PROTECT: error_msg = "The media is write protected."; break;
		case ERROR_BAD_UNIT: error_msg = "The system cannot find the device specified."; break;
		case ERROR_NOT_READY: error_msg = "The device is not ready."; break;
		case ERROR_BAD_COMMAND: error_msg = "The device does not recognize the command."; break;
		case ERROR_CRC: error_msg = "Data error (cyclic redundancy check)."; break;
		case ERROR_BAD_LENGTH: error_msg = "The program issued a command but the command length is incorrect."; break;
		case ERROR_SEEK: error_msg = "The drive cannot locate a specific area or track on the disk."; break;
		case ERROR_NOT_DOS_DISK: error_msg = "The specified disk or diskette cannot be accessed."; break;
		case ERROR_SECTOR_NOT_FOUND: error_msg = "the drive cannot find the sector requested"; break;
		case ERROR_OUT_OF_PAPER: error_msg = "the printer is out of paper"; break;
		case ERROR_WRITE_FAULT: error_msg = "the system cannot write to the specified device"; break;
		case ERROR_READ_FAULT: error_msg = "the system cannot read from the specified device"; break;
		case ERROR_GEN_FAILURE: error_msg = "a deviced attached to the system is not functioning"; break;
		case ERROR_SHARING_VIOLATION: error_msg = "cannot access the file, in use by anther process"; break;
		case ERROR_LOCK_VIOLATION: error_msg = "another process has locked a portion of the file"; break;
		case ERROR_WRONG_DISK: error_msg = "the wrong diskette is in the drive"; break;
		case ERROR_SHARING_BUFFER_EXCEEDED: error_msg = "too many file opened for sharing"; break;
		case ERROR_HANDLE_EOF: 	error_msg = "reached the end of file"; break;
		case ERROR_HANDLE_DISK_FULL: error_msg = "the disk is full"; break;
		case ERROR_NOT_SUPPORTED:	error_msg = "the network request is not supported"; break;
		default: error_msg = "This is an error I haven't typed in yet.";
	}

	return error_msg;
}

static UUtBool
BFiFileRef_PathNameExists(
	const char*	inPath)
{
	char		buffer[BFcMaxPathLength];
	char*		bufferLeafName;
	DWORD		attributes;

	UUrString_Copy(buffer, inPath, BFcMaxPathLength);

	bufferLeafName = BFrPath_To_Name(buffer);

	if(bufferLeafName > buffer)
	{
		*bufferLeafName = 0;

		attributes = GetFileAttributes(buffer);

		if(attributes == 0xFFFFFFFF)
		{
			return UUcFalse;
		}
	}

	return UUcTrue;

}

static UUtError
BFiCheckFileName(
	const char		*inFileName)
{
	const char		*name;

	name = inFileName;

	while (*name)
	{
		if (name[0] == '/')
		{
			char	illegal_char[2];

			illegal_char[0] = name[0];
			illegal_char[1] = '\0';

			UUmError_ReturnOnErrorMsgP(
				UUcError_Generic,
				"Illegal character %s in file name.",
				(UUtUns32)illegal_char,
				0,
				0);
		}

		name++;
	}

	return UUcError_None;
}

UUtError
BFrFileRef_Set(
	BFtFileRef*		inFileRef,
	const char*		inFileName)			// like make from name but does not create
{
	UUtError	error;

	UUmAssert(NULL != inFileRef);
	UUmAssert(NULL != inFileName);

	if(strlen(inFileName) >= BFcMaxPathLength)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Path name too long");
	}

	/* Make sure the path name is valid */
	error = BFiCheckFileName(inFileName);
	UUmError_ReturnOnError(error);

	if(BFiFileRef_PathNameExists(inFileName) == UUcFalse)
	{
		return BFcError_FileNotFound;
	}

	UUrString_Copy(inFileRef->name, inFileName, BFcMaxPathLength);

	{
		char *leaf_name = BFrPath_To_Name(inFileRef->name);

		if(strlen(leaf_name) >= BFcMaxFileNameLength)
		{
			UUmError_ReturnOnErrorMsgP(
				UUcError_Generic,
				"Filename \"%s\" too long, max length is %d",
				(UUtUns32)leaf_name, BFcMaxFileNameLength-1, 0);
		}
	}

	return UUcError_None;
}

void
BFrFileRef_Dispose(
	BFtFileRef	*inFileRef)
{

	UUmAssert(inFileRef != NULL);

	UUrMemory_Block_Delete(inFileRef);

}

UUtError
BFrFileRef_Duplicate(
	const BFtFileRef	*inOrigFileRef,
	BFtFileRef	**outNewFileRef)
{
	UUtError error;

	UUmAssert(inOrigFileRef != NULL);
	UUmAssert(outNewFileRef != NULL);

	error =
		BFrFileRef_MakeFromName(
			inOrigFileRef->name,
			outNewFileRef);

	UUmError_ReturnOnError(error);

	return UUcError_None;
}

const char *
BFrFileRef_GetLeafName(
	const BFtFileRef	*inFileRef)
{
	const char *leaf_name;
	UUmAssert(inFileRef != NULL);

	leaf_name = BFrPath_To_Name((char *) inFileRef->name);

	return leaf_name;
}

const char *
BFrFileRef_GetFullPath(
	const BFtFileRef*		inFileRef)
{
	return inFileRef->name;
}

// 1. if inName is absolute path replace inFileRef
// 2. else find name part of inName append only that to inFileRef

UUtError
BFrFileRef_SetName(
	BFtFileRef	*inFileRef,
	const char	*inName)
{
	UUtError	error;

	UUmAssert(inFileRef != NULL);
	UUmAssert(inName != NULL);

	error = BFiCheckFileName(inName);
	UUmError_ReturnOnError(error);

	if (BFrPath_IsAbsolute(inName))
	{
		if(strlen(inName) >= BFcMaxPathLength)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Path name too long");
		}
		UUrString_Copy(inFileRef->name, inName, BFcMaxPathLength);
	}
	else
	{
		char *leaf_name = BFrPath_To_Name(inFileRef->name);

		if((strlen(inFileRef->name) - strlen(leaf_name) + strlen(inName)) > BFcMaxPathLength)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Path name too long");
		}

		UUrString_Copy(leaf_name, inName, BFcMaxPathLength);
	}

	{
		char *leaf_name = BFrPath_To_Name(inFileRef->name);

		if(strlen(leaf_name) >= BFcMaxFileNameLength)
		{
			UUmError_ReturnOnErrorMsgP(
				UUcError_Generic,
				"Filename \"%s\" too long, max length is %d",
				(UUtUns32)leaf_name, BFcMaxFileNameLength-1, 0);
		}
	}

	if(BFiFileRef_PathNameExists(inFileRef->name) == UUcFalse)
	{
		return BFcError_FileNotFound;
	}

	return UUcError_None;
}

UUtBool
BFrFileRef_FileExists(
	BFtFileRef	*inFileRef)
{
	DWORD attributes;

	attributes = GetFileAttributes(inFileRef->name);

	if(attributes == 0xFFFFFFFF)
	{
		return UUcFalse;
	}

	return UUcTrue;
}

UUtError
BFrFile_Create(
	BFtFileRef	*inFileRef)
{
	FILE	*file;

	UUmAssert(inFileRef != NULL);
	UUmAssert(inFileRef->name[0] != '\0');

	remove(inFileRef->name);

	file = fopen(inFileRef->name, "wb");

	if(file == NULL)
	{
		return UUcError_Generic; //XXX
	}

	fclose(file);

	return UUcError_None;
}

UUtError
BFrFile_Delete(
	BFtFileRef	*inFileRef)
{
	UUmAssert(inFileRef != NULL);

	remove(inFileRef->name);

	return UUcError_None;
}

UUtError
BFrFile_Open(
	BFtFileRef	*inFileRef,
	char		*inMode,
	BFtFile		**outFile)
{
	char	*mungedMode;
	DWORD	dwDesiredAccess;

	UUmAssert(inFileRef != NULL);
	UUmAssert(inFileRef->name[0] != '\0');
	UUmAssert(inMode != NULL);
	UUmAssert(outFile != NULL);

	if(!strcmp("r", inMode))
	{
		mungedMode = "rb";
		dwDesiredAccess = GENERIC_READ;
	}
	else if(!strcmp("w", inMode))
	{
		mungedMode = "wb";
		dwDesiredAccess = GENERIC_WRITE;
	}
	else if(!strcmp("rw", inMode))
	{
		mungedMode = "rb+";
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	}
	else
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Illegal file mode");
	}

	*outFile = (BFtFile *) fopen(inFileRef->name, mungedMode);

	if(*outFile == NULL)
	{
#if defined(DEBUGGING) && DEBUGGING
		// so we can no what our directory is for debugging (handy enough)
		HANDLE fileHandle;
		char cwdBuffer[1024];
		_getcwd(cwdBuffer,1024);
#endif

		if (!BFrFileRef_FileExists(inFileRef)) {
			return BFcError_FileNotFound;
		}

#if defined(DEBUGGING) && DEBUGGING
		// try to open read/write
		fileHandle = CreateFile(
			inFileRef->name,
			dwDesiredAccess,
			FILE_SHARE_READ,
			0,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (INVALID_HANDLE_VALUE == fileHandle) {
			const char *error_msg;
			DWORD windowsErrorCode = GetLastError();
			error_msg = WindowsErrorToString(windowsErrorCode);
		}
		else {
			CloseHandle(fileHandle);
		}
#endif

		return BFcError_FileNotFound;
	}

	return UUcError_None;
}

#if 0
UUtError
BFrFile_Map(
	BFtFileRef			*inFileRef,
	UUtUns32			inOffset,
	BFtFileMapping		**outMapping,
	void				**outPtr,
	UUtUns32			*outSize)
{
	UUtError error;
	BFtFile *file;
	void *ptr;
	UUtUns32 size;
	BFtFileMapping *mapping;

	mapping = UUrMemory_Block_New(sizeof(BFtFileMapping));
	if (NULL == mapping) {
		error = UUcError_OutOfMemory;
		goto fail;
	}

	error = BFrFile_MapClassic(inFileRef, inOffset, &file, &ptr, &size);
	if (error != UUcError_None) {
		goto fail;
	}

	mapping->file = file;
	mapping->ptr = ptr;

	*outMapping = mapping;
	*outPtr = ptr;
	*outSize = size;

	return UUcError_None;

fail:
	if (NULL != mapping) { UUrMemory_Block_Delete(mapping); }

	return error;
}


void
BFrFile_UnMap(
	BFtFileMapping	*inMapping)
{
	UUmAssertReadPtr(inMapping, sizeof(*inMapping));

	BFrFile_Close(inMapping->file);
	UUrMemory_Block_Delete(inMapping->ptr);
	UUrMemory_Block_Delete(inMapping);

	return;
}

#else

static UUtError
BFrFile_Map_Default(
	BFtFileRef			*inFileRef,
	UUtUns32			inOffset,
	BFtFileMapping		**outMappingRef,
	void				**outPtr,
	UUtUns32			*outSize)
{
	UUtError error;
	BFtFile *file;
	void *ptr;
	UUtUns32 size;
	BFtFileMapping *mappingRef;

	mappingRef = UUrMemory_Block_New(sizeof(BFtFileMapping));
	if (NULL == mappingRef) {
		error = UUcError_OutOfMemory;
		goto fail;
	}

	error = BFrFile_MapClassic(inFileRef, inOffset, &file, &ptr, &size);
	if (error != UUcError_None) {
		goto fail;
	}

	UUrString_Copy(mappingRef->name, inFileRef->name, BFcMaxPathLength);
	mappingRef->classicFile = file;
	mappingRef->ptr = ptr;
	mappingRef->fileHandle = NULL;
	mappingRef->fileMappingHandle = NULL;

	*outMappingRef = mappingRef;
	*outPtr = ptr;
	*outSize = size;

	return UUcError_None;

fail:
	if (NULL != mappingRef) { UUrMemory_Block_Delete(mappingRef); }

	return error;
}

UUtError
BFrFile_Map(
	BFtFileRef			*inFileRef,
	UUtUns32			inOffset,
	BFtFileMapping		**outMappingRef,
	void				**outPtr,
	UUtUns32			*outSize)
{
	UUtError error;
	DWORD win32Error;
	const char *error_msg;
	HANDLE fileHandle = NULL;
	HANDLE fileMappingHandle = NULL;
	UUtUns8 *ptr;
	BFtFileMapping *mappingRef;
	UUtUns32 fileLength;

	UUmAssertReadPtr(inFileRef, sizeof(*inFileRef));
	UUmAssertWritePtr(outMappingRef, sizeof(*outMappingRef));
	UUmAssertWritePtr(outPtr, sizeof(*outPtr));

	mappingRef = UUrMemory_Block_New(sizeof(BFtFileMapping));
	if (NULL == mappingRef) {
		error = UUcError_OutOfMemory;
		goto fail;
	}

	// try to open read/write
	fileHandle = CreateFile(
		inFileRef->name,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	// try to open read only
	if (INVALID_HANDLE_VALUE == fileHandle) {
		fileHandle = CreateFile(
			inFileRef->name,
			GENERIC_READ,
			FILE_SHARE_READ,
			0,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	}

	if (INVALID_HANDLE_VALUE == fileHandle) {
		win32Error = GetLastError();
		error_msg = WindowsErrorToString(win32Error);
		error = UUcError_Generic;
		fileHandle = NULL;
		goto fail;
	}

	fileLength = GetFileSize(fileHandle, 0);
	if (0xffffffff == fileLength) {
		error = UUcError_Generic;
		goto fail;
	}

	fileMappingHandle = CreateFileMapping(
		fileHandle,
		NULL,
		PAGE_WRITECOPY,
		0,
		0,
		NULL);

	if ((fileMappingHandle != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS))
	{
		error = UUcError_Generic;
		goto fail;
	}

	if (NULL == fileMappingHandle) {
		win32Error = GetLastError();
		error_msg = WindowsErrorToString(win32Error);
		error = UUcError_Generic;
		goto fail;
	}

	ptr = MapViewOfFile(
		fileMappingHandle,
		FILE_MAP_COPY,
		0,
		0,
		0);
	if (NULL == ptr)
	{
		win32Error = GetLastError();
		error_msg = WindowsErrorToString(win32Error);
		error = UUcError_Generic;
		goto fail;
	}

	UUrString_Copy(mappingRef->name, inFileRef->name, BFcMaxPathLength);
	mappingRef->classicFile = NULL;
	mappingRef->fileHandle = fileHandle;
	mappingRef->fileMappingHandle = fileMappingHandle;
	mappingRef->ptr = ptr;

	UUmAssert(1 == sizeof(*ptr));	// ptr arithmatic must work

	*outMappingRef = mappingRef;
	*outPtr = (ptr + inOffset);
	*outSize = fileLength - inOffset;

	return UUcError_None;

fail:
	if (NULL != fileMappingHandle) { CloseHandle(fileMappingHandle); }
	if (NULL != fileHandle) { CloseHandle(fileHandle); }
	if (NULL != mappingRef) { UUrMemory_Block_Delete(mappingRef); }

	error = BFrFile_Map_Default(inFileRef, inOffset, outMappingRef, outPtr, outSize);

	return error;
}

void
BFrFile_UnMap(
	BFtFileMapping	*inMapping)
{
	BOOL unmapResult;
	UUmAssertReadPtr(inMapping, sizeof(*inMapping));

	if (NULL != inMapping->fileMappingHandle)
	{
		UUmAssert(NULL == inMapping->classicFile);

		unmapResult = UnmapViewOfFile(inMapping->ptr);
		UUmAssert(unmapResult);

		CloseHandle(inMapping->fileMappingHandle);
		CloseHandle(inMapping->fileHandle);

		UUrMemory_Block_Delete(inMapping);
	}
	else
	{
		UUmAssert(NULL == inMapping->fileHandle);
		UUmAssert(NULL == inMapping->fileMappingHandle);

		BFrFile_Close(inMapping->classicFile);
		if (inMapping->ptr != NULL) {
			UUrMemory_Block_Delete(inMapping->ptr);
		}
		UUrMemory_Block_Delete(inMapping);
	}

	return;
}
#endif

UUtError
BFrFile_Flush(
	BFtFile	*inFile)
{
	UUmAssert(NULL != inFile);

	fflush((FILE *)inFile);

	return UUcError_None;
}

void
BFrFile_Close(
	BFtFile		*inFile)
{
	UUmAssert(NULL != inFile);

	fclose((FILE *)inFile);

}

UUtBool
BFrFileRef_IsDirectory(
	BFtFileRef*		inFileRef)
{
	DWORD		attributes;

	// is this a file reference to a directory
	// routine returns UUcTrue if file ref is a directory
	attributes = GetFileAttributes(inFileRef->name);
	return ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

UUtBool
BFrFileRef_IsEqual(
	const BFtFileRef	*inFileRef1,
	const BFtFileRef	*inFileRef2)
{
	UUtInt32			result;
	UUtBool				equal;

	result = UUrString_Compare_NoCase(inFileRef1->name, inFileRef2->name);
	equal = (result == 0);

	return equal;
}

UUtBool
BFrFileRef_IsLocked(
	BFtFileRef*		inFileRef)
{
	DWORD		attributes;

	// do we have write access to the file?
	// routine returns UUcTrue if file is locked
	attributes = GetFileAttributes(inFileRef->name);
	return ((attributes & FILE_ATTRIBUTE_READONLY) != 0);
}

UUtError
BFrFile_GetLength(
	BFtFile		*inFile,
	UUtUns32	*outLength)
{
	fpos_t	currentPos;

	UUmAssert(NULL != inFile);
	UUmAssert(NULL != outLength);

	if(fgetpos((FILE *)inFile, &currentPos) != 0)
	{
		UUmError_ReturnOnError(UUcError_Generic);
	}

	if(fseek((FILE *)inFile, 0, SEEK_END) != 0)
	{
		UUmError_ReturnOnError(UUcError_Generic);
	}

	*outLength = ftell((FILE *)inFile);

	if(fsetpos((FILE *)inFile, &currentPos) != 0)
	{
		UUmError_ReturnOnError(UUcError_Generic);
	}

	return UUcError_None;
}

UUtError
BFrFile_GetPos(
	BFtFile*		inFile,
	UUtUns32		*outPos)
{
	UUmAssert(inFile != NULL);
	UUmAssertWritePtr(outPos, sizeof(UUtUns32));

	*outPos = ftell((FILE *) inFile);

	return UUcError_None;
}

UUtError
BFrFile_SetEOF(
	BFtFile			*inFile)
{
	return UUcError_None;
}

UUtError
BFrFile_SetPos(
	BFtFile*		inFile,
	UUtUns32		inPos)
{
	int result;
	UUtError error;

	result = fseek((FILE *) inFile, inPos, SEEK_SET);

	error = (0 == result) ? UUcError_None : UUcError_Generic;

	return error;
}

UUtError
BFrFile_Read(
	BFtFile		*inFile,
	UUtUns32	inLength,
	void		*inData)
{
	UUtUns32 count;
	UUtError error;

	if (0 == inLength) {
		return UUcError_None;
	}

	UUmAssert(NULL != inFile);
	UUmAssert(NULL != inData);

	count = fread(inData, 1, inLength, (FILE *)inFile);

	error = (count == inLength) ? UUcError_None : UUcError_Generic;

	return UUcError_None;

}

UUtError
BFrFile_Write(
	BFtFile		*inFile,
	UUtUns32	inLength,
	void		*inData)
{
	UUtUns32 count;
	UUtError error;

	UUmAssert(NULL != inFile);
	UUmAssert((NULL != inData) || (0 == inLength));

	count = fwrite(inData, 1, inLength, (FILE *)inFile);

	error = (count == inLength) ? UUcError_None : UUcError_Generic;

	return UUcError_None;
}

UUtError
BFrFileRef_GetModTime(
	BFtFileRef*		inFileRef,
	UUtUns32*		outSecsSince1900)
{
	HANDLE fileHandle;
	FILETIME creationTime;
	FILETIME lastAccessTime;
	FILETIME lastWriteTime;
	FILETIME lastWriteTimeLocal;
	SYSTEMTIME systemTime;
	BOOL success;
	const char *error_msg;

	if (!BFrFileRef_FileExists(inFileRef)) {
		return BFcError_FileNotFound;
	}

	fileHandle = CreateFile(inFileRef->name, 0, FILE_SHARE_READ, 0, OPEN_EXISTING, 0 ,0);

	if (INVALID_HANDLE_VALUE == fileHandle)
	{
		DWORD windowsErrorCode = GetLastError();
		error_msg = WindowsErrorToString(windowsErrorCode);
		UUmError_ReturnOnErrorMsg(BFcError_FileNotFound, error_msg);
	}

	success = GetFileTime(fileHandle, &creationTime, &lastAccessTime, &lastWriteTime);

	CloseHandle(fileHandle);

	if (!success)
	{
		UUmError_ReturnOnError(UUcError_Generic);
	}

	FileTimeToLocalFileTime(&lastWriteTime, &lastWriteTimeLocal);
	FileTimeToSystemTime(&lastWriteTimeLocal, &systemTime);

	*outSecsSince1900 = UUrSystemToSecsSince1900(systemTime);

	return UUcError_None;
}

static UUtBool
iCheckFileNameValid(
	char*	inFileName,
	char*	inPrefix,
	char*	inSuffix)
{
	if(!strcmp(inFileName, ".")) return UUcFalse;
	if(!strcmp(inFileName, "..")) return UUcFalse;

	if(inPrefix != NULL)
	{
		// Check for prefix match
		if(UUrString_CompareLen_NoCase(inPrefix, inFileName, strlen(inPrefix)) != 0)
		{
			return UUcFalse;
		}

	}

	if(inSuffix != NULL)
	{
		// Check for suffix match
		if(UUrString_Compare_NoCase(inSuffix, inFileName + strlen(inFileName) - strlen(inSuffix)) != 0)
		{
			return UUcFalse;
		}
	}

	return UUcTrue;
}

UUtError
BFrDirectory_GetFileList(
	BFtFileRef*	inDirectoryRef,		/* The file ref for the desired directory */
	char*		inPrefix,			/* The prefix name for returned files, NULL for none */
	char*		inSuffix,			/* The suffix name for returned files, NULL for none */
	UUtUns16	inMaxFiles,			/* The maximum number of files to find */
	UUtUns16	*outNumFiles,		/* The number of files returned */
	BFtFileRef*	*outFileRefArray)	/* A pointer to the array of BFtFileRef for the returned files */
{
	UUtError			error;
	HANDLE				fileHandle;
	WIN32_FIND_DATA		curFindData;
	char*				fileName;

	UUtUns32			curFileIndex;

	char				fullFilePath[1024];

	AUtSharedStringArray*	stringArray;
	UUtUns32				num;
	UUtUns32*				sortedIndexList;
	AUtSharedString*		strings;
	UUtUns32				itr;
	BFtFileRef*				newFileRef;
	UUtUns32				newIndex;

	sprintf(fullFilePath, "%s\\*.*", inDirectoryRef->name);

	fileHandle = FindFirstFile(fullFilePath, &curFindData);

	*outNumFiles = 0;

	curFileIndex = 0;

	stringArray = AUrSharedStringArray_New();

	if(fileHandle == INVALID_HANDLE_VALUE)
	{
		return UUcError_None;
	}

	do
	{
		fileName = curFindData.cFileName;

		if(iCheckFileNameValid(fileName, inPrefix, inSuffix) == UUcFalse)
		{
			continue;
		}

		if(curFileIndex >= inMaxFiles)
		{
			break;
		}

		// make a new file ref
		sprintf(fullFilePath, "%s\\%s", inDirectoryRef->name, fileName);

		error =
			BFrFileRef_MakeFromName(
				fullFilePath,
				&newFileRef);
		UUmError_ReturnOnErrorMsg(error, "Could not create file ref");

		error =
			AUrSharedStringArray_AddString(
				stringArray,
				fileName,
				(UUtUns32)newFileRef,
				&newIndex);
		UUmError_ReturnOnErrorMsg(error, "Could not add file ref");

		curFileIndex++;

	} while((FindNextFile(fileHandle, &curFindData) == TRUE));

	FindClose(fileHandle);

	*outNumFiles = (UUtUns16)curFileIndex;

	num = AUrSharedStringArray_GetNum(stringArray);
	UUmAssert(curFileIndex == num);

	strings = AUrSharedStringArray_GetList(stringArray);
	sortedIndexList = AUrSharedStringArray_GetSortedIndexList(stringArray);

	for(itr = 0; itr < num; itr++)
	{
		outFileRefArray[itr] = (BFtFileRef*)strings[sortedIndexList[itr]].data;
	}

	AUrSharedStringArray_Delete(stringArray);

	return UUcError_None;
}

struct BFtFileIterator
{
	BFtFileRef			fileRef;

	HANDLE				fileHandle;
	WIN32_FIND_DATA		curFindData;

	char*				prefix;
	char*				suffix;

};


UUtError
BFrDirectory_FileIterator_New(
	BFtFileRef*			inDirectoryRef,		/* The file ref for the desired directory */
	char*				inPrefix,			/* The prefix name for returned files, NULL for none */
	char*				inSuffix,			/* The suffix name for returned files, NULL for none */
	BFtFileIterator*	*outFileIterator)	/* The find iterator to use in BFrDirectory_FindNext */
{
	BFtFileIterator*	newFileIterator;

	UUmAssert(NULL != inDirectoryRef);
	UUmAssert(NULL != outFileIterator);
	UUmAssert(BFrFileRef_FileExists(inDirectoryRef));

	newFileIterator = UUrMemory_Block_New(sizeof(BFtFileIterator));
	if(newFileIterator == NULL)
	{
		return UUcError_OutOfMemory;
	}

	newFileIterator->fileRef = *inDirectoryRef;
	newFileIterator->fileHandle = NULL;
	newFileIterator->prefix = inPrefix;
	newFileIterator->suffix = inSuffix;

	if( inDirectoryRef->name[strlen(inDirectoryRef->name) - 1] != '\\' ) {
		sprintf(newFileIterator->fileRef.name, "%s\\", inDirectoryRef->name);
	}

	*outFileIterator = newFileIterator;

	return UUcError_None;
}

UUtError
BFrDirectory_FileIterator_Next(
	BFtFileIterator*	inFileIterator,
	BFtFileRef			*outFileRef)
{
	WIN32_FIND_DATA		curFindData;
	UUtBool				stop;

	UUmAssert(inFileIterator->fileRef.name[ strlen( inFileIterator->fileRef.name)-1 ] == '\\');

	if(inFileIterator->fileHandle == NULL)
	{
		char fullFilePath[1024];

		sprintf(fullFilePath, "%s*.*", inFileIterator->fileRef.name);

		inFileIterator->fileHandle = FindFirstFile(fullFilePath, &curFindData);

		if(inFileIterator->fileHandle == INVALID_HANDLE_VALUE)
		{
			return UUcError_Generic;
		}

		stop = UUcFalse;
	}
	else
	{
		stop = FindNextFile(inFileIterator->fileHandle, &curFindData) == FALSE;
	}

	while(!stop)
	{
		if(iCheckFileNameValid(curFindData.cFileName, inFileIterator->prefix, inFileIterator->suffix))
		{
			sprintf(outFileRef->name, "%s%s", inFileIterator->fileRef.name, curFindData.cFileName);

			return UUcError_None;
		}

		stop = FindNextFile(inFileIterator->fileHandle, &curFindData) == FALSE;
	}

	return UUcError_Generic;
}

void
BFrDirectory_FileIterator_Delete(
	BFtFileIterator*	inFileIterator)
{
	FindClose(inFileIterator->fileHandle);
	UUrMemory_Block_Delete(inFileIterator);
}


UUtError
BFrDirectory_DeleteContentsOnly(
	BFtFileRef*	inDirectoryRef)
{
	UUtError			error;
	BFtFileIterator*	fileIterator;
	BFtFileRef			curFileRefToDelete;
	DWORD attributes;

	error =
		BFrDirectory_FileIterator_New(
			inDirectoryRef,
			NULL,
			NULL,
			&fileIterator);
	UUmError_ReturnOnError(error);

	while(1)
	{
		error =	BFrDirectory_FileIterator_Next(fileIterator, &curFileRefToDelete);
		if(error != UUcError_None) break;


		attributes = GetFileAttributes(curFileRefToDelete.name);

		UUmAssert(attributes != 0xFFFFFFFF);

		if(attributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			BFrDirectory_DeleteDirectoryAndContents(&curFileRefToDelete);
		}
		else
		{
			BFrFile_Delete(&curFileRefToDelete);
		}
	}

	BFrDirectory_FileIterator_Delete(fileIterator);

	return UUcError_None;
}

UUtError
BFrDirectory_DeleteDirectoryAndContents(
	BFtFileRef*	inDirectoryRef)
{
	UUtError	error;
	UUtBool		success;

	error = BFrDirectory_DeleteContentsOnly(inDirectoryRef);
	UUmError_ReturnOnError(error);

	success = RemoveDirectory(inDirectoryRef->name);

	if(success == UUcFalse)
	{
		return UUcError_Generic;
	}

	return UUcError_None;
}

UUtError
BFrDirectory_Create(
	BFtFileRef*	inDirRef,
	char*		inDirName)	// if non-null then inDirRef refers to the parent dir
{
	UUtBool	success;
	char	buffer[BFcMaxPathLength];

	if(inDirName == NULL)
	{
		success = CreateDirectory(inDirRef->name, NULL);
	}
	else
	{
		sprintf(buffer, "%s\\%s", inDirRef->name, inDirName);
		success = CreateDirectory(buffer, NULL);
	}

	if(success == UUcFalse) return UUcError_Generic;

	return UUcError_None;
}

UUtError
BFrFileRef_GetParentDirectory(
	BFtFileRef	*inFileRef,
	BFtFileRef	**outParentDirectory)
{
	UUtError		error;
	char			path[BFcMaxPathLength];
	char			*last_dir;

	UUmAssert(inFileRef);
	UUmAssert(outParentDirectory);

	// initialize
	*outParentDirectory = NULL;

	UUrString_Copy(path, inFileRef->name, BFcMaxPathLength);

	last_dir = strrchr(path, '\\');
	if (last_dir == NULL) {
		return UUcError_Generic;
	}

	last_dir[0] = '\0';

	// create the texture directory file ref
	error = BFrFileRef_MakeFromName(path, outParentDirectory);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

