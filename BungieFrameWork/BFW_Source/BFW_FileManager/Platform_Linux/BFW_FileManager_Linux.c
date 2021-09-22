/*
	FILE:	BFW_FileManager_Linux.c
*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_AppUtilities.h"

#include <stdio.h>
#include <string.h>

//#include <iostream.h>
#include <dirent.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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
	UUtBool	mmapped;
	BFtFile	*file;
	void	*ptr;
};
#endif

static void
BFiConvertWin2Unix(
	const char*	inOrigName,
	char*		outMungedName)
{
	const char*	src;
	char*		dst;
	
	src = inOrigName;
	dst = outMungedName;
	
	while(*src != 0)
	{
		if(*src == '\\')
		{
			*dst++ = '/';
			
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

static UUtBool
BFiFileRef_PathNameExists(
	const char*	inPath)
{
	return access(inPath, F_OK) == 0;
}

static UUtError
BFiCheckFileName(
	const char		*inFileName)
{
	const char		*name;
	
	name = inFileName;
	
	/*
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
	*/
	
	return UUcError_None;
}

UUtError
BFrFileRef_Set(
	BFtFileRef*		inFileRef,
	const char*		inFileName)			// like make from name but does not create
{
	UUtError	error;
	char			mungedName[BFcMaxPathLength];
	
	UUmAssert(NULL != inFileRef);
	UUmAssert(NULL != inFileName);
		
	if(strlen(inFileName) >= BFcMaxPathLength)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Path name too long");
	}

	/* Make sure the path name is valid */
	error = BFiCheckFileName(inFileName);
	UUmError_ReturnOnError(error);
	
	BFiConvertWin2Unix(inFileName, mungedName);
	
	if(BFiFileRef_PathNameExists(mungedName) == UUcFalse)
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
	UUmAssert(inFileRef != NULL);
	return BFiFileRef_PathNameExists(inFileRef->name);
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
	const char	*mungedMode;
	
	UUmAssert(inFileRef != NULL);
	UUmAssert(inFileRef->name[0] != '\0');
	UUmAssert(inMode != NULL);
	UUmAssert(outFile != NULL);
	
	if(!strcmp("r", inMode))
	{
		mungedMode = "rb";
	}
	else if(!strcmp("w", inMode))
	{
		mungedMode = "wb";
	}
	else if(!strcmp("rw", inMode))
	{
		mungedMode = "rb+";
	}
	else
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Illegal file mode");
	}
	
	*outFile = (BFtFile *) fopen(inFileRef->name, mungedMode);

	if(*outFile == NULL)
	{
		return errno == ENOENT ? BFcError_FileNotFound : UUcError_Generic;
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
	mappingRef->mmapped = UUcFalse;
	mappingRef->file = file;
	mappingRef->ptr = ptr;

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
	UUtError error = 0;
	BFtFile *file = NULL;
	int fd;
	UUtUns8 *ptr = NULL;
	BFtFileMapping *mappingRef = NULL;
	struct stat statbuf;
	size_t mappingLen;

	UUmAssertReadPtr(inFileRef, sizeof(*inFileRef));
	UUmAssertWritePtr(outMappingRef, sizeof(*outMappingRef));
	UUmAssertWritePtr(outPtr, sizeof(*outPtr));

	mappingRef = UUrMemory_Block_New(sizeof(BFtFileMapping));
	if (NULL == mappingRef)
	{ 
		error = UUcError_OutOfMemory; 
		goto fail;
	}

	// try to open read/write
	error = BFrFile_Open(inFileRef, "rw", &file);
	// try to open read only
	if (error)
	{
		error = BFrFile_Open(inFileRef, "r", &file);
	}
	if (error)
	{
		goto fail;
	}

	fd = fileno((FILE*)file);
	if (fd < 0)
	{
		goto fail;
	}

	if (fstat(fd, &statbuf) != 0)
	{
		error = UUcError_Generic;
		goto fail;
	}

	if (inOffset > statbuf.st_size)
	{
		error = UUcError_Generic;
		goto fail;
	}
	mappingLen = (size_t)statbuf.st_size - (size_t)inOffset;

	ptr = mmap(
		NULL,
		mappingLen,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE,
		fd,
		inOffset);
	if (NULL == ptr)
	{
		error = UUcError_Generic;
		goto fail;
	}

	UUrString_Copy(mappingRef->name, inFileRef->name, BFcMaxPathLength);
	mappingRef->mmapped = UUcTrue;
	mappingRef->file = file;
	mappingRef->ptr = ptr;

	UUmAssert(1 == sizeof(*ptr));	// ptr arithmatic must work

	*outMappingRef = mappingRef;
	*outPtr = ptr;
	*outSize = mappingLen;

	return UUcError_None;

fail:
	if (NULL != file) { BFrFile_Close(file); }
	if (NULL != mappingRef) { UUrMemory_Block_Delete(mappingRef); }

	error = BFrFile_Map_Default(inFileRef, inOffset, outMappingRef, outPtr, outSize);

	return error;
}

void
BFrFile_UnMap(
	BFtFileMapping	*inMapping)
{
	UUmAssertReadPtr(inMapping, sizeof(*inMapping));

	if (!inMapping->mmapped)
	{
		if (inMapping->ptr != NULL) {
			UUrMemory_Block_Delete(inMapping->ptr);
		}
	}
	BFrFile_Close(inMapping->file);
	
	UUrMemory_Block_Delete(inMapping);
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
	struct stat		statbuf;
	return stat(inFileRef->name, &statbuf) == 0 && S_ISDIR(statbuf.st_mode);
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
	return access(inFileRef->name, W_OK) != 0;
}
	
UUtError
BFrFile_GetLength(
	BFtFile		*inFile,
	UUtUns32	*outLength)
{
	struct stat statbuf;
	int fd = fileno((FILE*)inFile);
	if (fd < 0 || fstat(fd, &statbuf) != 0) {
		UUmError_ReturnOnError(UUcError_Generic);
	}
	
	*outLength = statbuf.st_size;
	
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
	struct stat statbuf;
	int fd = fileno((FILE*)inFileRef);
	if (fd < 0 || fstat(fd, &statbuf) != 0) {
		UUmError_ReturnOnError(UUcError_Generic);
	}

	//TODO: duplicated with UUrGetSecsSince1900()
	static const UUtUns32 secondsBetween1900And1970 = 2208988800;
	*outSecsSince1900 = statbuf.st_mtime + secondsBetween1900And1970;
	
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
	DIR *				directory;
	struct dirent*		dirent;
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

	*outNumFiles = 0;
	
	curFileIndex = 0;
	
	stringArray = AUrSharedStringArray_New();
	
	directory = opendir(inDirectoryRef->name);
	if(directory == NULL)
	{
		if (errno == ENOENT)
		{
			return UUcError_None;
		}
		return UUcError_Generic;
	}
	
	while((dirent = readdir(directory)))
	{
		fileName = dirent->d_name;
		
		//TODO: symlink are probably fine as well if they resolve to a regular file
		if(dirent->d_type != DT_REG || iCheckFileNameValid(fileName, inPrefix, inSuffix) == UUcFalse)
		{
			continue;
		}
		
		if(curFileIndex >= inMaxFiles)
		{
			break;
		}
		
		// make a new file ref
		sprintf(fullFilePath, "%s/%s", inDirectoryRef->name, fileName);

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
		
	}
	
	closedir(directory);
	
	if (errno)
	{
		return UUcError_Generic;
	}
	
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

	DIR *				directory;
	
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
	newFileIterator->directory = NULL;
	newFileIterator->prefix = inPrefix;
	newFileIterator->suffix = inSuffix;

	if( inDirectoryRef->name[strlen(inDirectoryRef->name) - 1] != '/' ) {
		sprintf(newFileIterator->fileRef.name, "%s/", inDirectoryRef->name);
	}
	
	*outFileIterator = newFileIterator;
	
	return UUcError_None;
}

UUtError
BFrDirectory_FileIterator_Next(
	BFtFileIterator*	inFileIterator,
	BFtFileRef			*outFileRef)
{
	struct dirent*		dirent;
	UUtBool				stop;
	
	UUmAssert(inFileIterator->fileRef.name[ strlen( inFileIterator->fileRef.name)-1 ] == '/');
	
	if(!inFileIterator->directory)
	{
		inFileIterator->directory = opendir(inFileIterator->fileRef.name);
		
		if(inFileIterator->directory == NULL)
		{
			return UUcError_Generic;
		}
		
		stop = UUcFalse;
	}
	
	while((dirent = readdir(inFileIterator->directory)))
	{
		if(dirent->d_type == DT_REG && iCheckFileNameValid(dirent->d_name, inFileIterator->prefix, inFileIterator->suffix))
		{			
			sprintf(outFileRef->name, "%s%s", inFileIterator->fileRef.name, dirent->d_name);
			
			return UUcError_None;
		}
	}
	
	return UUcError_Generic;
}

void
BFrDirectory_FileIterator_Delete(
	BFtFileIterator*	inFileIterator)
{
	closedir(inFileIterator->directory);
	UUrMemory_Block_Delete(inFileIterator);
}


UUtError
BFrDirectory_DeleteContentsOnly(
	BFtFileRef*	inDirectoryRef)
{
	UUtError			error;
	BFtFileIterator*	fileIterator;
	BFtFileRef			curFileRefToDelete;
	struct stat			statbuf;
	int					res;

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
		
	
		res = stat(curFileRefToDelete.name, &statbuf);
		if(res != 0)
		{
			UUmAssert(UUcFalse);
			statbuf.st_mode = 0;
		}
		
		if(S_ISDIR(statbuf.st_mode))
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

	success = rmdir(inDirectoryRef->name) == 0;

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
		success = mkdir(inDirRef->name, 0777) == 0;
	}
	else
	{
		sprintf(buffer, "%s/%s", inDirRef->name, inDirName);
		success = mkdir(buffer, 0777) == 0;
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
	
	last_dir = strrchr(path, BFcPathSeparator);
	if (last_dir == NULL) {
		return UUcError_Generic;
	}

	last_dir[0] = '\0';
	
	// create the texture directory file ref
	error = BFrFileRef_MakeFromName(path, outParentDirectory);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}
