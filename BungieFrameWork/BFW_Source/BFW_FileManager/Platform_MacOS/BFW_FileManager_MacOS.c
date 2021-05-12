#if UUmPlatform	== UUmPlatform_Mac

/*
	FILE:	BFW_FileManager_MacOS.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 28, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/

#include <string.h>

#include <Files.h>

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_AppUtilities.h"

#include "IterateDirectory.h"
#include "MoreFilesExtras.h"

#define BFcMaxParamBlocks	100

// resides in FileManager_Common.c but no public prototype
UUtError BFrFile_MapClassic(BFtFileRef	*inFileRef,
	UUtUns32			inOffset,
	BFtFile				**outFile,
	void				**outPtr,
	UUtUns32			*outSize);

struct BFtFileMapping
{
	BFtFile	*file;
	void	*ptr;
};

typedef struct BFtParamBlock
{
	IOParam				paramBlock;
	UUtBool				used;
	BFtCompletion_Func	completionProc;
	void*				refCon;
	
} BFtParamBlock;


BFtParamBlock	BFgParamBlocks[BFcMaxParamBlocks];	// XXX - I am assuming this is cleared to zero

static void
BFiConvertWin2Mac(
	const char*	inOrigName,
	char*		outMungedName)
{
	const char*	src;
	char*		dst;
	char*		colonPtr;
	
	src = inOrigName;
	dst = outMungedName;
	
	colonPtr = strchr(inOrigName, ':');
	
	if(colonPtr == NULL && src[0] != '\\')
	{
		*dst++ = ':';
	}
	
	while(*src != 0)
	{
		if(src[0] == '.' && src[1] == '.' && src[2] == '\\')
		{
			*dst++ = ':';
			src += 3;
		}
		else if(*src == '\\' || src[0] == ':' || src[0] == '/')
		{
			*dst++ = ':';
			
			src++;
		}
		else
		{
			*dst++ = *src;
			
			src++;
		}
	}
	
	*dst = 0;
}

static UUtError
BFiCheckFileName(
	const char		*inFileName)
{
	const char		*name;
	
	name = inFileName;
	
	while (*name)
	{
//		if (name[0] == '\/') - This barfs on MrC
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


static void
BFrFileRef_SetFromFSSpec(
	const FSSpec	*inFSSpec,
	BFtFileRef		*outFileRef)
{
	UUtUns16	i;
	Boolean		isAlias;
	Boolean		isAliasFolder;
		
	for(i = 0; i < inFSSpec->name[0]; i++)
	{
		outFileRef->leafName[i] = inFSSpec->name[i+1];
	}
	outFileRef->leafName[inFSSpec->name[0]] = 0;
	outFileRef->fsSpec = *inFSSpec;
	
	// resolve alias if needed
	ResolveAliasFile(&outFileRef->fsSpec, true, &isAliasFolder, &isAlias);
	
	return;
}

UUtError
BFrFileRef_MakeFromFSSpec(
	const FSSpec		*inFSSpec,
	BFtFileRef	**outFileRef)
{
	BFtFileRef	*newFileRef;
//	UUtUns16	i;
//	Boolean		isAlias;
//	Boolean		isAliasFolder;
	
	newFileRef = (BFtFileRef *) UUrMemory_Block_New(sizeof(BFtFileRef));
	
	if(newFileRef == NULL)
	{
		UUrError_Report( UUcError_OutOfMemory, "Could not allocate new file ref");
		return UUcError_OutOfMemory;
	}

	BFrFileRef_SetFromFSSpec(inFSSpec, newFileRef);
		
	*outFileRef = newFileRef;
	
	return UUcError_None;
}

UUtError
BFrFileRef_Set(
	BFtFileRef*		inFileRef,
	const char*		inFileName)
{
	UUtError		error;
	OSErr			osError;
	FSSpec			fsSpec;
	char			mungedName[BFcMaxPathLength + 30];
	const char*		slashPtr;
	UUtUns16		pathLength;
	
	error = BFiCheckFileName(inFileName);
	UUmError_ReturnOnError(error);
	
	pathLength = strlen(inFileName);
	if(pathLength > BFcMaxPathLength)
	{
		UUmError_ReturnOnErrorMsgP(
			UUcError_Generic,
			"Filename \"%s\" too long, max path length is %d",
			(UUtUns32)mungedName, BFcMaxPathLength-1, 0);
	}
	
	BFiConvertWin2Mac(inFileName, mungedName + 1);

	slashPtr = strrchr(mungedName + 1, ':');
	if(slashPtr == NULL)
	{
		slashPtr = inFileName;
	}
	
	if(strlen(slashPtr) >= BFcMaxFileNameLength)
	{
		UUmError_ReturnOnErrorMsgP(
			UUcError_Generic,
			"Filename \"%s\" too long, max leaf name length is %d",
			(UUtUns32)slashPtr, BFcMaxFileNameLength-1, 0);
	}
	
	mungedName[0] = strlen(mungedName + 1);
	
	osError = FSMakeFSSpec(0, 0, (unsigned char *)mungedName, &fsSpec);
	
	BFrFileRef_SetFromFSSpec(&fsSpec, inFileRef);

	if(osError != noErr && osError != fnfErr)
	{
		return BFcError_FileNotFound;
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
	
	error =
		BFrFileRef_MakeFromFSSpec(
			&inOrigFileRef->fsSpec,
			outNewFileRef);

	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

const char *
BFrFileRef_GetLeafName(
	const BFtFileRef	*inFileRef)
{
	return inFileRef->leafName;
}

UUtBool
BFrFileRef_IsDirectory(
	BFtFileRef*		inFileRef)
{
	OSErr			osError;
	DirInfo			dirInfo;
	
	UUmAssert(inFileRef);
	
#if defined(DEBUGGING) && DEBUGGING
	UUrMemory_Clear(&dirInfo, sizeof(DirInfo));
#endif
	
	dirInfo.ioCompletion	= NULL;
	dirInfo.ioNamePtr		= inFileRef->fsSpec.name;
	dirInfo.ioVRefNum		= inFileRef->fsSpec.vRefNum;
	dirInfo.ioFDirIndex		= 0;
	dirInfo.ioDrDirID		= inFileRef->fsSpec.parID;
	
	osError = PBGetCatInfo((CInfoPBPtr)&dirInfo, 0);
	if(osError != noErr)
	{
		return UUcFalse;
	}
	
	return ((dirInfo.ioFlAttrib & (1 << 4)) != 0);
}

UUtBool
BFrFileRef_IsEqual(
	const BFtFileRef	*inFileRef1,
	const BFtFileRef	*inFileRef2)
{
	UUtBool				equal;
	
	equal = (inFileRef1->fsSpec.vRefNum == inFileRef2->fsSpec.vRefNum) &&
			(inFileRef1->fsSpec.parID == inFileRef2->fsSpec.parID) &&
			(UUrString_Compare_NoCase(inFileRef1->leafName, inFileRef2->leafName) == 0);
	
	return equal;
}

UUtBool
BFrFileRef_IsLocked(
	BFtFileRef*		inFileRef)
{
	OSErr			osError;
	HFileInfo		fileInfo;
	
	UUmAssert(inFileRef);
	
#if defined(DEBUGGING) && DEBUGGING
	UUrMemory_Clear(&fileInfo, sizeof(fileInfo));
#endif

	fileInfo.ioCompletion	= NULL;
	fileInfo.ioNamePtr		= inFileRef->fsSpec.name;
	fileInfo.ioVRefNum		= inFileRef->fsSpec.vRefNum;
	fileInfo.ioFDirIndex	= 0;
	fileInfo.ioDirID		= inFileRef->fsSpec.parID;
	
	osError = PBGetCatInfo((CInfoPBPtr)&fileInfo, 0);
	if(osError != noErr)
	{
		return UUcFalse;
	}
	
	return ((fileInfo.ioFlAttrib & (1 << 0)) != 0);
}

UUtError
BFrFileRef_GetParentDirectory(
	BFtFileRef	*inFileRef,
	BFtFileRef	**outParentDirectory)
{
	OSErr			osError;
	FSSpec			parent_FSSpec;
	
	// initialize
	*outParentDirectory = NULL;
	
	// get the parent directory
	osError =
		FSMakeFSSpec(
			inFileRef->fsSpec.vRefNum,
			inFileRef->fsSpec.parID,
			"\p:",
			&parent_FSSpec);
	if (osError != noErr)
	{
		return UUcError_Generic;
	}
	
	return BFrFileRef_MakeFromFSSpec(&parent_FSSpec, outParentDirectory);
}

UUtError
BFrFileRef_SetName(
	BFtFileRef	*inFileRef,
	const char	*inName)
{
	UUtError	error;
	UUtInt32	i;
	OSErr		osError;
	char		mungedName[128];
	const char*	slashPtr;
	UUtBool		isDirectory = UUcFalse;
	UUtBool		absolute_path = BFrPath_IsAbsolute(inName);
	
	error = BFiCheckFileName(inName);
	UUmError_ReturnOnError(error);
	
	slashPtr = strrchr(inName, '\\');
	if(slashPtr == NULL)
	{
		slashPtr = inName;
	}
	else
	{
		if(*slashPtr == '\\') slashPtr++;
		if(*slashPtr == 0) isDirectory = UUcTrue;
	}
	
	if(strlen(slashPtr) >= BFcMaxFileNameLength)
	{
		UUmError_ReturnOnErrorMsgP(
			UUcError_Generic,
			"Filename \"%s\" too long, max length is %d",
			(UUtUns32)slashPtr, BFcMaxFileNameLength-1, 0);
	}
	
	UUmAssert(strlen(inName) < 128);
	
	BFiConvertWin2Mac(inName, mungedName + 1);
	mungedName[0] = strlen(mungedName + 1);

	osError =
		FSMakeFSSpec(
			absolute_path ? 0 : inFileRef->fsSpec.vRefNum,
			absolute_path ? 0 : inFileRef->fsSpec.parID,
			(unsigned char *)mungedName,
			&inFileRef->fsSpec);
	
	if(osError != noErr && osError != fnfErr)
	{
		return BFcError_FileNotFound;
	}
	
	// Reset the leaf name
	for(i = 0; i < inFileRef->fsSpec.name[0]; i++)
	{
		inFileRef->leafName[i] = inFileRef->fsSpec.name[i+1];
	}
	inFileRef->leafName[inFileRef->fsSpec.name[0]] = 0;
	
	return UUcError_None;
}

const char *
BFrFileRef_GetFullPath(
	const BFtFileRef*		inFileRef)
{
	UUmAssert(!"Implement Me");
	
	return NULL;
}

UUtBool
BFrFileRef_FileExists(
	BFtFileRef	*inFileRef)
{
	OSErr			error;
	FSSpec			fsSpec;
	
	error = 
		FSMakeFSSpec(
			inFileRef->fsSpec.vRefNum,
			inFileRef->fsSpec.parID,
			inFileRef->fsSpec.name,
			&fsSpec);
			
	if(error != noErr)
	{
		return UUcFalse;
	}
	
	return UUcTrue;
}

UUtError
BFrFileRef_GetModTime(
	BFtFileRef	*inFileRef,
	UUtUns32	*outSecsSince1900)
{
	OSErr			osError;
	HFileParam	fileParamBlock;
	
	fileParamBlock.ioCompletion = NULL;
	fileParamBlock.ioNamePtr = inFileRef->fsSpec.name;
	fileParamBlock.ioVRefNum = inFileRef->fsSpec.vRefNum;
	fileParamBlock.ioFDirIndex = 0;
	fileParamBlock.ioDirID = inFileRef->fsSpec.parID;
	
	osError = PBHGetFInfo((HParmBlkPtr)&fileParamBlock, 0);
	
	if(osError != noErr)
	{
		return BFcError_FileNotFound;
	}
	
	*outSecsSince1900 = fileParamBlock.ioFlMdDat + UUc1904_To_1900_era_offset;
	
	return UUcError_None;
}

FSSpec*
BFrFileRef_GetFSSpec(
	BFtFileRef		*inFileRef)
{
	return &inFileRef->fsSpec;
}

UUtError
BFrFile_Create(
	BFtFileRef	*inFileRef)
{
	OSType creator= 'Oni!';
	OSType filetype= 'INST';
	char filename[64]= {0};
	UUtError error= UUcError_None;
	
	// make any adjustments necessary to filetype info
	p2cstrcpy(filename, inFileRef->fsSpec.name);
	if (strstr(filename, ".bmp") || strstr(filename, ".BMP"))
	{
		// open screenshots with QuickTime Picture Viewer
		creator= 'ogle';
		filetype= 'BMP ';
	}
	else if (strstr(filename, ".txt") || strstr(filename, ".TXT"))
	{
		// open text files with BBEdit
		creator= 'R*ch';
		filetype= 'TEXT';
	}
	else if (strstr(filename, ".c") ||
		strstr(filename, ".C") ||
		strstr(filename, ".cp") ||
		strstr(filename, ".CP") ||
		strstr(filename, ".cpp") ||
		strstr(filename, ".CPP"))
	{
		// open source code files with CodeWarrior
		creator= 'CWIE';
		filetype= 'TEXT';
	}

	error= FSpCreate(&inFileRef->fsSpec, creator, filetype,0);
	
	return error;
}

UUtError
BFrFile_Delete(
	BFtFileRef	*inFileRef)
{
	OSErr			error;

	error = 
		FSpDelete(
			&inFileRef->fsSpec);
	
	if(error != noErr)
	{
		
		return BFcError_FileNotFound;
	}
	
	return UUcError_None;
}

UUtError
BFrFile_Open(
	BFtFileRef	*inFileRef,
	char		*inMode,
	BFtFile		**outFile)
{
	OSErr			error;
	short			refNum;
	char			permission;
	UUtBool			create = UUcFalse;
	
	if(!strcmp("r", inMode))
	{
		permission = fsRdPerm;
	}
	else if(!strcmp("w", inMode))
	{
		permission = fsWrPerm;
		create = UUcTrue;
	}
	else if(!strcmp("rw", inMode))
	{
		permission = fsRdWrPerm;
		create = UUcTrue;
	}
	else
	{
		UUrError_Report(UUcError_Generic, "Illegal file mode");
		return UUcError_Generic;
	}
	
	error = 
		FSpOpenDF(
			&inFileRef->fsSpec,
			permission,
			&refNum);
			
	// if file does not exist attempt to create the file like fopen
	if ((fnfErr == error) && create)
	{
		error = BFrFile_Create(inFileRef);
		if (error != UUcError_None) return error;
		
		error = 
			FSpOpenDF(
				&inFileRef->fsSpec,
				permission,
				&refNum);
	}
	
	if(error != noErr)
	{
		return BFcError_FileNotFound;
	}
	
	*outFile = (BFtFile *)refNum;
	
	return UUcError_None;
}

UUtError
BFrFile_Flush(
	BFtFile	*inFile)
{
	IOParam			paramBlock;
	OSErr			osError;

	paramBlock.ioCompletion = NULL;
	paramBlock.ioRefNum = (short)inFile;
	
	osError = PBFlushFileSync((ParmBlkPtr)&paramBlock);
	
	if(osError != noErr)
	{
		UUrError_Report(UUcError_Generic, "could not flush file");
		return UUcError_Generic;
	}
		
	return UUcError_None;
}

void
BFrFile_Close(
	BFtFile		*inFile)
{
	OSErr	osError;

	osError = FSClose((short)inFile);
	
	if(osError != noErr)
	{
		UUrError_Report(UUcError_Generic, "could not close file");
	}
}

UUtError
BFrFile_GetLength(
	BFtFile		*inFile,
	UUtUns32	*outLength)
{
	OSErr			error;
	short			refNum;
	
	refNum = (short)inFile;
	
	error = GetEOF(refNum, (long*)outLength);
	
	if(error != noErr)
	{
		UUrError_Report(UUcError_Generic, "could not get length");
		return UUcError_Generic;
	}

	return UUcError_None;
}

UUtError
BFrFile_GetPos(
	BFtFile*		inFile,
	UUtUns32		*outPos)
{
	OSErr osErr;

	UUmAssert(inFile != NULL);
	UUmAssertWritePtr(outPos, sizeof(UUtUns32));

	osErr = GetFPos((short) inFile, (long *) outPos);

	if (noErr != osErr) {
		return UUcError_Generic;
	}

	return UUcError_None;
}

UUtError
BFrFile_SetEOF(
	BFtFile*		inFile)
{
	OSErr			osErr;
	UUtInt32		filePos;
	
	// get the current read/write position of the file
	osErr = GetFPos((short)inFile, &filePos);
	if (osErr != noErr) { return UUcError_Generic; }

	// set the End Of File (EOF) to the current position
	osErr = SetEOF((short)inFile, filePos);
	if (osErr != noErr) { return UUcError_Generic; }
	
	return UUcError_None;
}

UUtError
BFrFile_SetPos(
	BFtFile*		inFile,
	UUtUns32		inPos)
{
	OSErr osErr;
	
	UUmAssert(inFile != NULL);

	osErr = SetFPos((short) inFile, fsFromStart, (long) inPos);
	
	if (eofErr == osErr) {
		osErr = SetEOF((short) inFile, (long) inPos);
		
		if (noErr != osErr) {
			return UUcError_Generic;
		}

		osErr = SetFPos((short) inFile, fsFromStart, (long) inPos);
	}

	if (noErr != osErr) {
		return UUcError_Generic;
	}
	
	return UUcError_None;
}

UUtError
BFrFile_Read(
	BFtFile		*inFile,
	UUtUns32	inLength,
	void		*inData)
{
	OSErr				osError;
	UUtInt32			length;
	UUtUns32			num_reads;
	char				*buffer;
	const UUtInt32		max_bytes = (1 << 23);	// 1 << 23 = 8,388,608
	
	// read the data in a maximum of max_bytes at a time
	num_reads = (inLength / max_bytes) + 1;
	buffer = (char*)inData;
	length = (UUtInt32)inLength;
	
	while (num_reads--)
	{
		UUtInt32		bytes_to_read;
		
		bytes_to_read = UUmMin(length, max_bytes);
		
		osError = FSRead((short)inFile, &bytes_to_read, buffer);
		if(osError != noErr)
		{
//			UUrError_Report(UUcError_Generic, "could not read file");
			return UUcError_Generic;
		}
		
		// prepare for next read
		length -= bytes_to_read;
		buffer += bytes_to_read;
	}
	
#if 0
	const UUtUns32		max_bytes	= (1 << 23);	// 1 << 23 = 8388608
	if (inLength < max_bytes)
	{
	}
	else
	{
		UUtUns32			num_reads;
		char				*buffer;
		
		// read the data in a maximum of max_bytes at a time
		num_reads	= (inLength / max_bytes) + 1;
		buffer		= (char*)inData;
		
		while (num_reads--)
		{
			UUtUns32		bytes_to_read;
			
			// calculate the number of bytes to read
			length = UUmMin(length, max_bytes);
			
			length = bytes_to_read;
			
			// read the data from the file
			osError = FSRead((short)inFile, &length, buffer);
			if(osError != noErr)
			{
				UUrError_Report(UUcError_Generic, "could not read file");
				return UUcError_Generic;
			}
			UUmAssert(length == bytes_to_read);
			
			// prepare for next read
			length -= bytes_to_read;
			buffer += bytes_to_read;
		}
	}
#endif
	
	return UUcError_None;
}

UUtError
BFrFile_Write(
	BFtFile		*inFile,
	UUtUns32	inLength,
	void		*inData)
{
	OSErr			osError;
	UUtInt32		length = inLength;
	
	osError = FSWrite((short)inFile, &length, inData);
	
	if(osError != noErr)
	{
		UUrError_Report(UUcError_Generic, "could not read file");
		return UUcError_Generic;
	}
	
	return UUcError_None;
}

static void
BFiFile_Async_CompletionFunc(
	BFtParamBlock*	inParamBlock)
{
	inParamBlock->used = UUcFalse;
	inParamBlock->completionProc(inParamBlock->refCon);
}

UUtError
BFrFile_Async_Read(
	BFtFile*			inFile,
	UUtUns32			inLength,
	void*				inDataPtr,
	BFtCompletion_Func	inCompletionFunc,
	void*				inRefCon)
{
	UUmAssert(0);
	return UUcError_Generic;
#if 0
	BFtParamBlock*	bfParamBlock;
	OSErr			osError;
	UUtUns16		itr;
	
	for(itr = 0, bfParamBlock = BFgParamBlocks;
		itr < BFcMaxParamBlocks;
		itr++, bfParamBlock++)
	{
		if(bfParamBlock->used == UUcFalse) break;
	}
	
	if(itr == BFcMaxParamBlocks) return UUcError_Generic;
	
	bfParamBlock->completionProc = inCompletionFunc;
	bfParamBlock->refCon = inRefCon;
	
	bfParamBlock->paramBlock.ioCompletion = (IOCompletionUPP)BFiFile_Async_CompletionFunc;
	bfParamBlock->paramBlock.ioVRefNum = 0;
	bfParamBlock->paramBlock.ioRefNum = (short)inFile;
	bfParamBlock->paramBlock.ioBuffer = (char *)inDataPtr;
	bfParamBlock->paramBlock.ioReqCount = inLength;
	bfParamBlock->paramBlock.ioPosMode = fsFromMark;
	bfParamBlock->paramBlock.ioPosOffset = 0;
	
	osError = PBReadAsync((ParmBlkPtr)&bfParamBlock);
	
	if(osError != noErr)
	{
		UUrError_Report(UUcError_Generic, "could not read file");
		return UUcError_Generic;
	}
	return UUcError_None;
#endif	
}
	
UUtError
BFrFile_Async_Write(
	BFtFile*			inFile,
	UUtUns32			inLength,
	void*				inDataPtr,
	BFtCompletion_Func	inCompletionFunc,
	void*				inRefCon)
{
	UUmAssert(0);
	return UUcError_Generic;
#if 0
	BFtParamBlock*	bfParamBlock;
	OSErr			osError;
	UUtUns16		itr;
	
	for(itr = 0, bfParamBlock = BFgParamBlocks;
		itr < BFcMaxParamBlocks;
		itr++, bfParamBlock++)
	{
		if(bfParamBlock->used == UUcFalse) break;
	}
	
	if(itr == BFcMaxParamBlocks) return UUcError_Generic;
	
	bfParamBlock->completionProc = inCompletionFunc;
	bfParamBlock->refCon = inRefCon;
	
	bfParamBlock->paramBlock.ioCompletion = (IOCompletionUPP)BFiFile_Async_CompletionFunc;
	bfParamBlock->paramBlock.ioVRefNum = 0;
	bfParamBlock->paramBlock.ioRefNum = (short)inFile;
	bfParamBlock->paramBlock.ioBuffer = (char *)inDataPtr;
	bfParamBlock->paramBlock.ioReqCount = inLength;
	bfParamBlock->paramBlock.ioPosMode = fsFromMark;
	bfParamBlock->paramBlock.ioPosOffset = 0;
	
	osError = PBWriteAsync((ParmBlkPtr)&bfParamBlock);
	
	if(osError != noErr)
	{
		UUrError_Report(UUcError_Generic, "could not read file");
		return UUcError_Generic;
	}
	
	return UUcError_None;
#endif
}

static UUtBool
BFiCheckCPBValid(
	const CInfoPBRec* const	cpbPtr,
	char*					inPrefix,
	char*					inSuffix)
{
	char*			targetName;
	char			buffer[512];
	
	UUrString_PStr2CStr(cpbPtr->hFileInfo.ioNamePtr, buffer, 512);
	
	targetName = buffer;

	if(inPrefix != NULL)
	{
		// Check for prefix match
		if(UUrString_CompareLen_NoCase(inPrefix, targetName, strlen(inPrefix)) != 0)
		{
			return UUcFalse;
		}
		
	}
	
	if(inSuffix != NULL)
	{
		// Check for suffix match
		if(UUrString_Compare_NoCase(inSuffix, targetName + strlen(targetName) - strlen(inSuffix)) != 0)
		{
			return UUcFalse;
		}
	}
	
	return UUcTrue;
}

typedef struct tContentsData
{
	UUtUns16		maxNumFiles;
	UUtUns16		curFileIndex;
	//BFtFileRef**	fileRefArray;
	
	AUtSharedStringArray*	stringArray;
	
	char*			prefix;
	char*			suffix;
	
} tContentsData;

static pascal void
BFiDirectory_ContentsIterator(
	const CInfoPBRec* const		cpbPtr,
	Boolean						*outQuitFlag,
	void*						myDataPtr)
{
	tContentsData*	contentsData = (tContentsData*)myDataPtr;
	UUtError		error;
	OSErr			osErr;
	FSSpec			fsSpec;
	BFtFileRef*		newFileRef;
	char			cstr[128];
	UUtUns16		itr;
	UUtUns32		newIndex;
	
	*outQuitFlag = false;
	
	//if(!(cpbPtr->hFileInfo.ioFlAttrib & ioDirMask))
	
	if(contentsData->curFileIndex >= contentsData->maxNumFiles)
	{
		*outQuitFlag = true;
		return;
	}
	
	if(BFiCheckCPBValid(cpbPtr, contentsData->prefix, contentsData->suffix) == UUcFalse)
	{
		return;
	}
	
	osErr =	
		FSMakeFSSpec(
			cpbPtr->hFileInfo.ioVRefNum,
			cpbPtr->hFileInfo.ioFlParID,
			cpbPtr->hFileInfo.ioNamePtr,
			&fsSpec);
	if(osErr != noErr)
	{
		*outQuitFlag = true;
		return;
	}
	
	error = 
		BFrFileRef_MakeFromFSSpec(
			&fsSpec,
			&newFileRef);
			//contentsData->fileRefArray + contentsData->curFileIndex++);
	if(error != UUcError_None)
	{
		*outQuitFlag = true;
		return;
	}
	
	for(itr = 0; itr < *cpbPtr->hFileInfo.ioNamePtr; itr++)
	{
		cstr[itr] = cpbPtr->hFileInfo.ioNamePtr[itr+1];
	}
	cstr[itr] = 0;
	
	error = 
		AUrSharedStringArray_AddString(
			contentsData->stringArray,
			cstr,
			(UUtUns32)newFileRef,
			&newIndex);
	if(error != UUcError_None)
	{
		*outQuitFlag = true;
		return;
	}
	
	contentsData->curFileIndex++;
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
	OSErr					osErr;
	tContentsData			contentsData;
	AUtSharedStringArray*	stringArray;
	UUtUns16				num;
	UUtUns32*				sortedIndexList;
	AUtSharedString*		strings;
	UUtUns16				itr;
	
	*outNumFiles = 0;

	stringArray = AUrSharedStringArray_New();
	UUmError_ReturnOnNull(stringArray);
	
	contentsData.maxNumFiles = inMaxFiles;
	contentsData.curFileIndex = 0;
	//contentsData.fileRefArray = outFileRefArray;
	contentsData.prefix = inPrefix;
	contentsData.suffix = inSuffix;
	contentsData.stringArray = stringArray;
	
	osErr = 
		FSpIterateDirectory(
			&inDirectoryRef->fsSpec,
			1,
			BFiDirectory_ContentsIterator,
			(void*)&contentsData);
	
	if (fnfErr == osErr) {
		goto exit;
	}
	else if(osErr != noErr)	{
		AUrSharedStringArray_Delete(stringArray);
		return UUcError_Generic;
	}
	
	*outNumFiles = contentsData.curFileIndex;
	
	num = AUrSharedStringArray_GetNum(stringArray);
	UUmAssert(contentsData.curFileIndex == num);
	
	strings = AUrSharedStringArray_GetList(stringArray);
	sortedIndexList = AUrSharedStringArray_GetSortedIndexList(stringArray);
	
	for(itr = 0; itr < num; itr++)
	{
		outFileRefArray[itr] = (BFtFileRef*)strings[sortedIndexList[itr]].data;
	}
	
exit:
	AUrSharedStringArray_Delete(stringArray);
	return UUcError_None;
}

struct BFtFileIterator
{
	CInfoPBRec		cPB;			/* the parameter block used for PBGetCatInfo calls */
	Str63			itemName;		/* the name of the current item */
	
	UUtUns16		index;
	UUtInt32		dirID;
	
	char*			prefix;
	char*			suffix;

};

UUtError
BFrDirectory_FileIterator_New(
	BFtFileRef*			inDirectoryRef,		/* The file ref for the desired directory */
	char*				inPrefix,			/* The prefix name for returned files, NULL for none */
	char*				inSuffix,			/* The suffix name for returned files, NULL for none */
	BFtFileIterator*	*outFileIterator)	/* The find iterator to use in BFrDirectory_FindNext */
{
	OSErr				osErr;
	BFtFileIterator*	newFileIterator;
	Boolean				isDirectory;
	UUtInt16			theVRefNum;
	UUtInt32			theDirID;
	
	newFileIterator = UUrMemory_Block_New(sizeof(BFtFileIterator));
	if(newFileIterator == NULL)
	{
		return UUcError_OutOfMemory;
	}
	
	/* Get the real directory ID and make sure it is a directory */
	osErr =
		GetDirectoryID(
			inDirectoryRef->fsSpec.vRefNum,
			inDirectoryRef->fsSpec.parID,
			inDirectoryRef->fsSpec.name,
			&theDirID,
			&isDirectory);
	if(osErr != noErr)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "GetDirectoryID return an error");
	}
	
	if(isDirectory == false)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "inDirectoryRef must be a directory");
	}
	
	/* Get the real vRefNum */
	osErr = DetermineVRefNum(inDirectoryRef->fsSpec.name, inDirectoryRef->fsSpec.vRefNum, &theVRefNum);
	if(osErr != noErr)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "DetermineVRefNum return an error");
	}

	newFileIterator->cPB.hFileInfo.ioNamePtr = (StringPtr)&newFileIterator->itemName;
	newFileIterator->cPB.hFileInfo.ioVRefNum = theVRefNum;
	newFileIterator->dirID = theDirID;
	newFileIterator->itemName[0] = 0;
	newFileIterator->index = 1;
	newFileIterator->prefix = inPrefix;
	newFileIterator->suffix = inSuffix;
	
	*outFileIterator = newFileIterator;
	
	return UUcError_None;
}

UUtError
BFrDirectory_FileIterator_Next(
	BFtFileIterator*	inFileIterator,
	BFtFileRef			*outFileRef)
{
	OSErr		osErr;
//	UUtError	error;
	FSSpec		fsSpec;
	
	do
	{
		inFileIterator->cPB.dirInfo.ioFDirIndex = inFileIterator->index++;
		inFileIterator->cPB.dirInfo.ioDrDirID = inFileIterator->dirID;
		
		osErr = PBGetCatInfoSync((CInfoPBPtr)&inFileIterator->cPB);
		if(osErr != noErr)
		{
			return UUcError_Generic;
		}
		
	} while(BFiCheckCPBValid(&inFileIterator->cPB, inFileIterator->prefix, inFileIterator->suffix) == UUcFalse);

	osErr =	
		FSMakeFSSpec(
			inFileIterator->cPB.hFileInfo.ioVRefNum,
			inFileIterator->cPB.hFileInfo.ioFlParID,
			inFileIterator->cPB.hFileInfo.ioNamePtr,
			&fsSpec);
	if(osErr != noErr)
	{
		return UUcError_Generic;
	}
	

	BFrFileRef_SetFromFSSpec(&fsSpec, outFileRef);
	
	return UUcError_None;
}

void
BFrDirectory_FileIterator_Delete(
	BFtFileIterator*	inFileIterator)
{
	UUrMemory_Block_Delete(inFileIterator);
}

UUtError
BFrDirectory_DeleteContentsOnly(
	BFtFileRef*	inDirectoryRef)
{
	OSErr	err;
	
	err =
		DeleteDirectoryContents(
			inDirectoryRef->fsSpec.vRefNum,
			inDirectoryRef->fsSpec.parID,
			inDirectoryRef->fsSpec.name);
	
	if(err != noErr)
	{
		return UUcError_Generic;
	}
	
	return UUcError_None;
}

UUtError
BFrDirectory_DeleteDirectoryAndContents(
	BFtFileRef*	inDirectoryRef)
{
	OSErr	err;
	
	err =
		DeleteDirectory(
			inDirectoryRef->fsSpec.vRefNum,
			inDirectoryRef->fsSpec.parID,
			inDirectoryRef->fsSpec.name);
	
	if(err != noErr)
	{
		return UUcError_Generic;
	}
	
	return UUcError_None;
}

UUtError
BFrDirectory_Create(
	BFtFileRef*	inParentDirRef,
	char*		inDirName)
{
	OSErr		err;
	char		mungedName[BFcMaxPathLength];
	UUtInt32	newDirID;
	
	if(inDirName == NULL)
	{
		err =
			DirCreate(
				inParentDirRef->fsSpec.vRefNum,
				inParentDirRef->fsSpec.parID,
				inParentDirRef->fsSpec.name,
				&newDirID);
	}
	else
	{
		BFiConvertWin2Mac(inDirName, mungedName + 1);
		
		if(strlen(mungedName + 1) > 255)
		{
			return UUcError_Generic;
		}
		
		mungedName[0] = strlen(mungedName + 1);
		
		err =
			DirCreate(
				inParentDirRef->fsSpec.vRefNum,
				inParentDirRef->fsSpec.parID,
				(unsigned char *)mungedName,
				&newDirID);
	}
	
	if(err != noErr)
	{
		return UUcError_Generic;
	}
	
	return UUcError_None;
}

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
	if (inMapping->ptr != NULL) {
		UUrMemory_Block_Delete(inMapping->ptr);
	}
	UUrMemory_Block_Delete(inMapping);

	return;
}

#endif // UUmPlatform	== UUmPlatform_Mac