/*
	FILE:	BFW_TM3_InGame.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: July 10, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_AppUtilities.h"
#include "BFW_TM_Private.h"
#include "BFW_TM_Construction.h"
#include "BFW_Timer.h"


UUtBool TMgTimeLevelLoad = UUcFalse;


/*
 * =========================================================================
 * D E F I N E S
 * =========================================================================
 */
	// CB: we no longer create dynamic instances that we want to delete at every
	// level unload, so the temporary instance file is no longer necessary
	#define ENABLE_TEMPORARY_INSTANCES				0

	#define TMcPrivateData_Max						(64)
	#define TMcCacheData_Max						(16)
	

	#define TMcInstanceFileRefs_Max					(16)
	
#if TOOL_VERSION
	#define TMcDynamic_Instance_Perm_Max			(16)
	#define TMcDynamic_Memory_Perm_Size				(100 * 1024)
#else
	#define TMcDynamic_Instance_Perm_Max			(8)
	#define TMcDynamic_Memory_Perm_Size				(70 * 1024)
#endif
	#define TMcDynamic_InstanceFileIndex_Perm		(0xFFFFFFFF)

#if ENABLE_TEMPORARY_INSTANCES
	#define TMcDynamic_Instance_Temp_Max			(256)
	#define TMcDynamic_Memory_Temp_Size				(1 * 1024 * 1024)
	#define TMcDynamic_InstanceFileIndex_Temp		(0xFFFFFFFE)
#endif

#if ENABLE_TEMPORARY_INSTANCES
	#define TMcGame_InstanceFile_Dynamic_Num		(2)
#else
	#define TMcGame_InstanceFile_Dynamic_Num		(1)
#endif
	
	#define	TMcGame_InstancePtrList_Max				(4)
	#define TMcGame_PrivateInfosPerInstanceFile_Max	(20)
	
/*
 * =========================================================================
 * T Y P E S
 * =========================================================================
 */

	typedef enum TMtInstanceFile_PrivateInfo_Type
	{
		TMcPrivateInfo_PrivateData	
	} TMtInstanceFile_PrivateInfo_Type;
	
/*
 *
 */
	typedef struct TMtInstance_DynamicData
	{
		UUtUns32				timeStamp;
		TMtInstancePriorities	priority;
		
	} TMtInstance_DynamicData;
	
	struct TMtPrivateData
	{
		TMtTemplateTag			templateTag;
		UUtUns32				dataSize;
		TMtTemplateProc_Handler	procHandler;
		
	};
	
	struct TMtCache_Simple
	{
		TMtTemplateTag				templateTag;
		
		UUtUns32					maxNumEntries;
		
		TMtCacheProc_Simple_Load	loadProc;
		TMtCacheProc_Simple_Unload	unloadProc;
		
		void**						entryList;
	};
	
	typedef struct TMtInstanceFile_PrivateInfo
	{
		TMtInstanceFile_PrivateInfo_Type	type;
		
		void*								owner;
		TMtTemplateTag						templateTag;
		UUtUns16							instanceListIndex;	// if this is UUcMaxUns16 then there is no instance list
		void*								memoryPool;
		
	} TMtInstanceFile_PrivateInfo;
	
/*
 *
 */
	
/*
 * This is the instance file
 */
	struct TMtInstanceFile
	{
		char						fileName[BFcMaxFileNameLength];
		UUtUns32					fileIndex;
		
		UUtUns32					maxInstanceDescriptors;
		UUtUns32					numInstanceDescriptors;
		TMtInstanceDescriptor*		instanceDescriptors;
		
		UUtUns32					numNameDescriptors;
		TMtNameDescriptor*			nameDescriptors;		// Used for binary search
		
		UUtUns32					numTemplateDescriptors;
		TMtTemplateDescriptor*		templateDescriptors;
		
		BFtFileMapping*				mapping;				// if file mapping is used
		BFtFileMapping*				rawMapping;
		void*						rawPtr;
		BFtFile*					separateFile;

		UUtUns8*					dataBlock;

		char*						nameBlock;
		
		UUtBool						final;
		UUtBool						preparedForMemory;
		
		UUtUns32					dynamicBytesUsed;		// used for dynamic instances
		UUtMemory_Pool*				dynamicPool;
		
		UUtUns32					numInstancePtrLists;
		void**						instancePtrLists[TMcGame_InstancePtrList_Max];
		
		UUtUns32					numPrivateInfos;
		TMtInstanceFile_PrivateInfo	privateInfos[TMcGame_PrivateInfosPerInstanceFile_Max];
		
	};

	typedef struct TMtInstanceFileRef
	{
		BFtFileRef	instanceFileRef;
		UUtUns32	fileIndex;
		
	} TMtInstanceFileRef;
	
/*
 * =========================================================================
 * G L O B A L S
 * =========================================================================
 */
	static UUtBool				TMgGame_ValidLevels[TMcLevels_MaxNum];
	
	static UUtUns16				TMgGame_InstanceFileRefs_Num = 0;
	static TMtInstanceFileRef	TMgGame_InstanceFileRefs_List[TMcInstanceFileRefs_Max];
	
	static UUtUns16				TMgGame_LoadedInstanceFiles_Num = 0;
	static TMtInstanceFile*		TMgGame_LoadedInstanceFiles_List[TMcInstanceFileRefs_Max];
	
	static TMtInstanceFile*		TMgGame_DynamicInstanceFile_Temp = NULL;	// CB: this is now deprecated
	static TMtInstanceFile*		TMgGame_DynamicInstanceFile_Perm = NULL;
	
	static UUtUns16				TMgGame_PrivateData_Num = 0;
	static TMtPrivateData		TMgGame_PrivateData_List[TMcPrivateData_Max];
	
	
/*
 * =========================================================================
 * P R I V A T E   F U N C T I O N S
 * =========================================================================
 */

static UUtError
TMiGame_Instance_PrepareForMemory(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile,
	char*				inMsg,
	UUtBool				inIsVarArrayLeaf);

static UUtError
TMiGame_Instance_ByteSwap(
	UUtUns8*					*ioSwapCode,
	UUtUns8*					*ioDataPtr,
	TMtInstanceFile*			inInstanceFile,
	TMtTemplateProc_ByteSwap	inByteSwapProc,
	UUtUns8*					inOriginalDataPtr);

static UUtError
TMiGame_InstanceFile_LoadHeader(
	BFtFile*				inFile,
	TMtInstanceFile_Header	*outFileHeader,
	UUtBool					*outNeedsSwapping);

static TMtInstanceDescriptor*
TMiGame_InstanceFile_GetInstanceDesc(
	TMtInstanceFile*	inInstanceFile,
	TMtPlaceHolder		inPlaceHolder);

static TMtInstanceFile*
TMiGame_InstanceFile_GetFromIndex(
	UUtUns32	inInstanceFileIndex);
	
static UUtBool
TMiGame_Level_IsValid(
	BFtFileRef*	inInstanceFileRef,
	UUtUns32	inLevelNumber)
{
	UUtError				error;
	BFtFile*				datFile;
	TMtInstanceFile_Header	fileHeader;
	UUtBool					needsSwapping;
	TMtTemplateDescriptor*	templateDescriptors = NULL;
	UUtUns32				itr;
	TMtTemplateDefinition*	templatePtr;
	UUtBool					level_exists = UUcTrue;
	
	UUmAssertReadPtr(inInstanceFileRef, sizeof(void*));
	
	error = BFrFile_Open(inInstanceFileRef, "r", &datFile);
	if (error != UUcError_None)
	{
		UUrDebuggerMessage("level %d: Can't open file" UUmNL, inLevelNumber);
		level_exists = UUcFalse;
		goto done;
	}
	
	error = TMiGame_InstanceFile_LoadHeader(datFile, &fileHeader, &needsSwapping);
	if (error != UUcError_None)
	{
		UUrDebuggerMessage("level %d: Can't load header" UUmNL, inLevelNumber);
		level_exists = UUcFalse;
		goto done;
	}
	
	if(fileHeader.numTemplateDescriptors == 0)
	{
		UUrDebuggerMessage("level %d: no templates" UUmNL, inLevelNumber);
		level_exists = UUcFalse;
		goto done;
	}
	
	templateDescriptors = UUrMemory_Block_New(fileHeader.numTemplateDescriptors * sizeof(TMtTemplateDescriptor));
	if(templateDescriptors == NULL)
	{
		UUrDebuggerMessage("level %d: out of memory" UUmNL, inLevelNumber);
		level_exists = UUcFalse;
		goto done;
	}
	
	// Read in the template descriptors
		error = 
			BFrFile_ReadPos(
				datFile,
				sizeof(TMtInstanceFile_Header) +
					fileHeader.numInstanceDescriptors * sizeof(TMtInstanceDescriptor) + 
					fileHeader.numNameDescriptors * sizeof(TMtNameDescriptor),
				fileHeader.numTemplateDescriptors * sizeof(TMtTemplateDescriptor),
				templateDescriptors);
		if(error != UUcError_None)
		{
			UUrDebuggerMessage("level %d: can't read in template descriptors" UUmNL, inLevelNumber);
			level_exists = UUcFalse;
			goto done;
		}
	
	// check to make sure all template checksums match
		for(itr = 0; itr < fileHeader.numTemplateDescriptors; itr++)
		{
			if(needsSwapping)
			{
				UUrSwap_4Byte(&templateDescriptors[itr].tag);
			}
			
			templatePtr = TMrUtility_Template_FindDefinition(templateDescriptors[itr].tag);
			if(templatePtr == NULL)
			{
				UUrDebuggerMessage("level %d: can't find template %c%c%c%c" UUmNL,
					inLevelNumber,
					(templateDescriptors[itr].tag >> 24) & 0xFF,
					(templateDescriptors[itr].tag >> 16) & 0xFF,
					(templateDescriptors[itr].tag >> 8) & 0xFF,
					(templateDescriptors[itr].tag >> 0) & 0xFF);
				level_exists = UUcFalse;
				goto done;
			}
			
			if(needsSwapping)
			{
				UUrSwap_8Byte(&templateDescriptors[itr].checksum);
			}

			if(templatePtr->checksum != templateDescriptors[itr].checksum)
			{
				UUrDebuggerMessage("level %d: template %c%c%c%c: checksums do not match" UUmNL,
					inLevelNumber,
					(templateDescriptors[itr].tag >> 24) & 0xFF,
					(templateDescriptors[itr].tag >> 16) & 0xFF,
					(templateDescriptors[itr].tag >> 8) & 0xFF,
					(templateDescriptors[itr].tag >> 0) & 0xFF);
				level_exists = UUcFalse;
				goto done;
			}
		}
	
done:
	BFrFile_Close(datFile);
	
	if(templateDescriptors != NULL)
	{
		UUrMemory_Block_Delete(templateDescriptors);
	}
	
	return level_exists;
}

static void
TMiGame_Instance_FileAndIndex_Get(
	const void*			inDataPtr,
	TMtInstanceFile*	*outInstanceFile,
	UUtUns32			*outInstanceIndex)
{
	UUtUns32			instanceFileID;
	TMtPlaceHolder		placeHolder;
	UUtUns32			instanceIndex;
	TMtInstanceFile*	instanceFile;
	
	UUmAssertReadPtr(inDataPtr, sizeof(void*));
	UUmAssertReadPtr(outInstanceFile, sizeof(*outInstanceFile));
	UUmAssertReadPtr(outInstanceIndex, sizeof(*outInstanceIndex));
	
	placeHolder = ((UUtUns32*)inDataPtr)[-2];
	instanceFileID = ((UUtUns32*)inDataPtr)[-1];
	
	if(TMmInstanceFile_ID_IsIndex(instanceFileID))
	{
		instanceFile = TMiGame_InstanceFile_GetFromIndex(instanceFileID);
		((TMtInstanceFile**)inDataPtr)[-1] = instanceFile;
	}
	else
	{
		
		instanceFile = (TMtInstanceFile*)instanceFileID;
		
		#if defined(DEBUGGING) && DEBUGGING
		{
			UUtUns16	itr;
			
			for(itr = 0; itr < TMgGame_LoadedInstanceFiles_Num; itr++)
			{
				if(TMgGame_LoadedInstanceFiles_List[itr] == instanceFile) break;
			}
			UUmAssert(itr < TMgGame_LoadedInstanceFiles_Num);
		}
		#endif

	}
	
	UUmAssertReadPtr(instanceFile, sizeof(*instanceFile));

	instanceIndex = TMmPlaceHolder_GetIndex(placeHolder);
	
	UUmAssert(instanceIndex < instanceFile->numInstanceDescriptors);
	
	*outInstanceFile = instanceFile;
	*outInstanceIndex = instanceIndex;
	
	
}



static void
TMiGame_Instance_File_Get(
	const void*			inDataPtr,
	TMtInstanceFile*	*outInstanceFile)
{
	UUtUns32			instanceFileID;
	TMtPlaceHolder		placeHolder;
	TMtInstanceFile*	instanceFile;
	
	UUmAssertReadPtr(inDataPtr, sizeof(void*));
	UUmAssertReadPtr(outInstanceFile, sizeof(*outInstanceFile));
	
	placeHolder = ((UUtUns32*)inDataPtr)[-2];
	instanceFileID = ((UUtUns32*)inDataPtr)[-1];
	
	if(TMmInstanceFile_ID_IsIndex(instanceFileID))
	{
		instanceFile = TMiGame_InstanceFile_GetFromIndex(instanceFileID);
		((TMtInstanceFile**)inDataPtr)[-1] = instanceFile;
	}
	else
	{
		
		instanceFile = (TMtInstanceFile*)instanceFileID;
		
		#if defined(DEBUGGING) && DEBUGGING
		{
			UUtUns16	itr;
			
			for(itr = 0; itr < TMgGame_LoadedInstanceFiles_Num; itr++)
			{
				if(TMgGame_LoadedInstanceFiles_List[itr] == instanceFile) break;
			}
			UUmAssert(itr < TMgGame_LoadedInstanceFiles_Num);
		}
		#endif

	}
	
	*outInstanceFile = instanceFile;
}


static UUtError
TMiGame_Instance_ByteSwap_Array(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile)
{
	UUtError	error;
	UUtUns8*	curSwapCode;
	UUtUns8*	curDataPtr;
	UUtUns8		count;
	UUtUns8		i;
	UUtBool		oneElementArray;
	
	UUmAssertReadPtr(ioSwapCode, sizeof(*ioSwapCode));
	UUmAssertReadPtr(ioDataPtr, sizeof(*ioDataPtr));
	
	curSwapCode = *ioSwapCode;
	curDataPtr = *ioDataPtr;

	count = *curSwapCode;
	curSwapCode += 1;

	oneElementArray = (*(curSwapCode + 1) == TMcSwapCode_EndArray);
	
	if ((*curSwapCode == TMcSwapCode_8Byte) && oneElementArray)
	{
		for(i = 0; i < count; i++)
		{
			UUrSwap_8Byte(curDataPtr);
			curDataPtr += 8;
		}
		curSwapCode += 2;
	}
	else if ((*curSwapCode == TMcSwapCode_4Byte) && oneElementArray)
	{
		for(i = 0; i < count; i++)
		{
			UUrSwap_4Byte(curDataPtr);
			curDataPtr += 4;
		}
		curSwapCode += 2;
	}
	else if ((*curSwapCode == TMcSwapCode_2Byte) && oneElementArray)
	{
		for(i = 0; i < count; i++)
		{
			UUrSwap_2Byte(curDataPtr);
			curDataPtr += 2;
		}
		curSwapCode += 2;
	}
	else if ((*curSwapCode == TMcSwapCode_1Byte) && oneElementArray)
	{
		curDataPtr += 1 * count;
		curSwapCode += 2;
	}
	else 
	{
		UUtUns8 *oldSwapCode = curSwapCode;

		for(i = 0; i < count; i++)
		{
			curSwapCode = oldSwapCode;
			
			error = 
				TMiGame_Instance_ByteSwap(
					&curSwapCode,
					&curDataPtr,
					inInstanceFile,
					NULL,
					NULL);
			UUmError_ReturnOnError(error);
		}
	}
		
	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;

	return UUcError_None;
}

static UUtError
TMiGame_Instance_ByteSwap_VarArray(
	UUtUns8*					*ioSwapCode,
	UUtUns8*					*ioDataPtr,
	TMtInstanceFile*			inInstanceFile,
	TMtTemplateProc_ByteSwap	inByteSwapProc,
	void*						inData)
{
	UUtError	error;
	UUtUns32	count;
	UUtUns32	i;
	UUtUns8*	curSwapCode;
	UUtUns8*	origSwapCode;
	UUtUns8*	curDataPtr;

	UUmAssertReadPtr(ioSwapCode, sizeof(*ioSwapCode));
	UUmAssertReadPtr(ioDataPtr, sizeof(*ioDataPtr));

	curSwapCode = *ioSwapCode;
	curDataPtr = *ioDataPtr;

	switch(*curSwapCode++)
	{
		case TMcSwapCode_8Byte:
			UUrSwap_8Byte(curDataPtr);
			count = (UUtUns32) (*(UUtUns64 *)curDataPtr);
			curDataPtr += 8;
			break;

		case TMcSwapCode_4Byte:
			UUrSwap_4Byte(curDataPtr);
			count = (*(UUtUns32 *)curDataPtr);
			curDataPtr += 4;
			break;
			
		case TMcSwapCode_2Byte:
			UUrSwap_2Byte(curDataPtr);
			count = (*(UUtUns16 *)curDataPtr);
			curDataPtr += 2;
			break;
			
		default:
			UUmDebugStr("swap codes are messed up.");
	}
	
	if(inByteSwapProc != NULL)
	{
		inByteSwapProc(inData);
		TMrUtility_SkipVarArray(&curSwapCode);
	}
	else if(count == 0)
	{
		TMrUtility_SkipVarArray(&curSwapCode);
	}
	else
	{
		UUtBool oneElementArray = (*(curSwapCode + 1) == TMcSwapCode_EndVarArray);
		
		if ((*curSwapCode == TMcSwapCode_8Byte) && oneElementArray)
		{
			for(i = 0; i < count; i++)
			{
				UUrSwap_8Byte(curDataPtr);
				curDataPtr += 8;
			}
			curSwapCode += 2;
		}
		else if ((*curSwapCode == TMcSwapCode_4Byte || *curSwapCode == TMcSwapCode_TemplatePtr) && oneElementArray)
		{
			for(i = 0; i < count; i++)
			{
				UUrSwap_4Byte(curDataPtr);
				curDataPtr += 4;
			}
			curSwapCode += 2;
		}
		else if ((*curSwapCode == TMcSwapCode_2Byte) && oneElementArray)
		{
			for(i = 0; i < count; i++)
			{
				UUrSwap_2Byte(curDataPtr);
				curDataPtr += 2;
			}
			curSwapCode += 2;
		}
		else if ((*curSwapCode == TMcSwapCode_1Byte) && oneElementArray)
		{
			curDataPtr += 1 * count;
			curSwapCode += 2;
		}
		else 
		{
			origSwapCode = curSwapCode;

			for(i = 0; i < count; i++)
			{
				curSwapCode = origSwapCode;
				
				error = 
					TMiGame_Instance_ByteSwap(
						&curSwapCode,
						&curDataPtr,
						inInstanceFile,
						NULL,
						NULL);
				UUmError_ReturnOnError(error);
			}
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;

	return UUcError_None;
}

static UUtError
TMiGame_Instance_ByteSwap(
	UUtUns8*					*ioSwapCode,
	UUtUns8*					*ioDataPtr,
	TMtInstanceFile*			inInstanceFile,
	TMtTemplateProc_ByteSwap	inByteSwapProc,
	UUtUns8*					inOriginalDataPtr)
{
	UUtError				error;
	UUtBool					stop;
	UUtUns8*				curSwapCode;
	UUtUns8*				curDataPtr;
	char					swapCode;
	TMtPlaceHolder			targetPlaceHolder;
	
	UUmAssertReadPtr(ioSwapCode, sizeof(*ioSwapCode));
	UUmAssertReadPtr(ioDataPtr, sizeof(*ioDataPtr));
	
	curSwapCode = *ioSwapCode;
	curDataPtr = *ioDataPtr;
	
	stop = UUcFalse;
	
	while(!stop)
	{
		swapCode = *curSwapCode++;
		
		switch(swapCode)
		{
			case TMcSwapCode_8Byte:
				UUrSwap_8Byte(curDataPtr);
				curDataPtr += 8;
				break;
				
			case TMcSwapCode_4Byte:
				UUrSwap_4Byte(curDataPtr);
				curDataPtr += 4;
				break;
				
			case TMcSwapCode_2Byte:
				UUrSwap_2Byte(curDataPtr);
				curDataPtr += 2;
				break;
				
			case TMcSwapCode_1Byte:
				curDataPtr += 1;
				break;
				
			case TMcSwapCode_BeginArray:
				error =
					TMiGame_Instance_ByteSwap_Array(
						&curSwapCode,
						&curDataPtr,
						inInstanceFile);
				UUmError_ReturnOnError(error);
				break;
				
			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;
				
			case TMcSwapCode_BeginVarArray:
				error =
					TMiGame_Instance_ByteSwap_VarArray(
						&curSwapCode,
						&curDataPtr,
						inInstanceFile,
						inByteSwapProc,
						inOriginalDataPtr);
				UUmError_ReturnOnError(error);
				break;
				
			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;
				
			case TMcSwapCode_RawPtr:
				UUrSwap_4Byte(curDataPtr);
				curDataPtr += 4;
				break;
				
			case TMcSwapCode_SeparateIndex:
				UUrSwap_4Byte(curDataPtr);
				curDataPtr += 4;
				break;
				
			case TMcSwapCode_TemplatePtr:
				// I think this crash means engine and data are out of sync - Michael
				UUmAssertReadPtr(curDataPtr, sizeof(TMtPlaceHolder));

				UUrSwap_4Byte(curDataPtr);

				targetPlaceHolder = *(TMtPlaceHolder*)curDataPtr;
				
				UUmAssert(targetPlaceHolder == 0 || !TMmPlaceHolder_IsPtr(targetPlaceHolder));
				UUmAssert(targetPlaceHolder == 0 || TMmPlaceHolder_GetIndex(targetPlaceHolder) < inInstanceFile->numInstanceDescriptors);

				curSwapCode += 4;
				curDataPtr += 4;
				break;
			
			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}
	
	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
	
	return UUcError_None;
}

static UUtError
TMiGame_Instance_PrepareForMemory_Array(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile,
	char*				inMsg)
{
	UUtError error;
	UUtUns8 *curSwapCode;
	UUtUns8 *curDataPtr;
	UUtUns8	count;
	UUtUns8 i;
	UUtBool oneTypeArray;

	UUmAssertReadPtr(ioSwapCode, sizeof(*ioSwapCode));
	UUmAssertReadPtr(ioDataPtr, sizeof(*ioDataPtr));

	curSwapCode = *ioSwapCode;
	curDataPtr = *ioDataPtr;

	count = *curSwapCode;
	curSwapCode += 1;

	oneTypeArray = (*(curSwapCode + 1) == TMcSwapCode_EndArray);
	
	if ((*curSwapCode == TMcSwapCode_8Byte) && oneTypeArray)
	{
		curDataPtr += 8 * count;
		curSwapCode += 2;
	}
	else if ((*curSwapCode == TMcSwapCode_4Byte) && oneTypeArray)
	{
		curDataPtr += 4 * count;
		curSwapCode += 2;
	}
	else if ((*curSwapCode == TMcSwapCode_2Byte) && oneTypeArray)
	{
		curDataPtr += 2 * count;
		curSwapCode += 2;
	}
	else if ((*curSwapCode == TMcSwapCode_1Byte) && oneTypeArray)
	{
		curDataPtr += 1 * count;
		curSwapCode += 2;
	}
	else 
	{
		UUtUns8 *oldSwapCode = curSwapCode;

		for(i = 0; i < count; i++)
		{
			curSwapCode = oldSwapCode;
			
			error = 
				TMiGame_Instance_PrepareForMemory(
					&curSwapCode,
					&curDataPtr,
					inInstanceFile,
					inMsg,
					UUcFalse);
			UUmError_ReturnOnError(error);
		}
	}
		
	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;

	return UUcError_None;
}

static UUtError
TMiGame_Instance_PrepareForMemory_VarArray(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile,
	char*				inMsg)
{
	UUtError	error;
	UUtUns32	count;
	UUtUns32	i;
	UUtUns8*	curSwapCode;
	UUtUns8*	origSwapCode;
	UUtUns8*	curDataPtr;

	UUmAssertReadPtr(ioSwapCode, sizeof(*ioSwapCode));
	UUmAssertReadPtr(ioDataPtr, sizeof(*ioDataPtr));

	curSwapCode = *ioSwapCode;
	curDataPtr = *ioDataPtr;

	switch(*curSwapCode++)
	{
		case TMcSwapCode_8Byte:
			count = (UUtUns32) (*(UUtUns64 *)curDataPtr);
			curDataPtr += 8;
			break;

		case TMcSwapCode_4Byte:
			count = (*(UUtUns32 *)curDataPtr);
			curDataPtr += 4;
			break;
			
		case TMcSwapCode_2Byte:
			count = (*(UUtUns16 *)curDataPtr);
			curDataPtr += 2;
			break;
			
		default:
			UUmDebugStr("swap codes are messed up.");
	}
	
	origSwapCode = curSwapCode;
	
	if(count == 0)
	{
		TMrUtility_SkipVarArray(&curSwapCode);
	}
	else
	{
		UUtBool oneElementArray = (*(curSwapCode + 1) == TMcSwapCode_EndVarArray);
		
		if ((*curSwapCode == TMcSwapCode_8Byte) && oneElementArray)
		{
			curDataPtr += 8 * count;
			curSwapCode += 2;
		}
		else if ((*curSwapCode == TMcSwapCode_4Byte) && oneElementArray)
		{
			curDataPtr += 4 * count;
			curSwapCode += 2;
		}
		else if ((*curSwapCode == TMcSwapCode_2Byte) && oneElementArray)
		{
			curDataPtr += 2 * count;
			curSwapCode += 2;
		}
		else if ((*curSwapCode == TMcSwapCode_1Byte) && oneElementArray)
		{
			curDataPtr += 1 * count;
			curSwapCode += 2;
		}
		else 
		{
			for(i = 0; i < count; i++)
			{
				curSwapCode = origSwapCode;
				
				error = 
					TMiGame_Instance_PrepareForMemory(
						&curSwapCode,
						&curDataPtr,
						inInstanceFile,
						inMsg,
						UUcFalse);
				UUmError_ReturnOnError(error);
				
			}
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;

	return UUcError_None;
}

#if SUPPORT_DEBUG_FILES
static BFtDebugFile *placeholder_file = NULL;
#endif

static UUtError
TMiGame_Instance_PrepareForMemory(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile,
	char*				inMsg,
	UUtBool				inIsVarArrayLeaf)
{
	UUtError				error;
	UUtBool					stop;
	UUtUns8*				curSwapCode;
	UUtUns8*				curDataPtr;
	char					swapCode;
	
	TMtPlaceHolder			targetPlaceHolder;
	
	TMtInstanceDescriptor*	targetDesc;

	static char				msg[2048];
	
	UUmAssertReadPtr(ioSwapCode, sizeof(*ioSwapCode));
	UUmAssertReadPtr(ioDataPtr, sizeof(*ioDataPtr));
	
	curSwapCode = *ioSwapCode;
	curDataPtr = *ioDataPtr;
	
	stop = UUcFalse;
	
	while(!stop)
	{
		swapCode = *curSwapCode++;
		
		switch(swapCode)
		{
			case TMcSwapCode_8Byte:
				curDataPtr += 8;
				break;
				
			case TMcSwapCode_4Byte:
				curDataPtr += 4;
				break;
				
			case TMcSwapCode_2Byte:
				curDataPtr += 2;
				break;
				
			case TMcSwapCode_1Byte:
				curDataPtr += 1;
				break;
				
			case TMcSwapCode_BeginArray:
				error =
					TMiGame_Instance_PrepareForMemory_Array(
						&curSwapCode,
						&curDataPtr,
						inInstanceFile,
						inMsg);
				UUmError_ReturnOnError(error);
				break;
				
			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;
				
			case TMcSwapCode_BeginVarArray:
				if(inIsVarArrayLeaf) return UUcError_None;
				error =
					TMiGame_Instance_PrepareForMemory_VarArray(
						&curSwapCode,
						&curDataPtr,
						inInstanceFile,
						inMsg);
				UUmError_ReturnOnError(error);
				break;
				
			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;
				
			case TMcSwapCode_TemplatePtr:
				// I think this crash means engine and data are out of sync - Michael
				UUmAssertReadPtr(curDataPtr, sizeof(TMtPlaceHolder));

				targetPlaceHolder = *(TMtPlaceHolder*)curDataPtr;
				
				if(targetPlaceHolder != 0)
				{
					UUmAssert(!TMmPlaceHolder_IsPtr(targetPlaceHolder));
					
					targetDesc = TMiGame_InstanceFile_GetInstanceDesc(inInstanceFile, targetPlaceHolder);
					UUmAssertReadPtr(targetDesc, sizeof(*targetDesc));
					
					if(targetDesc->namePtr == NULL)
					{
						if(targetDesc->dataPtr == NULL)
						{
							UUmError_ReturnOnErrorMsg(UUcError_Generic, "dataPtr is NULL");
						}
						*(void **)curDataPtr = targetDesc->dataPtr;
					}
					else
					{
						*(void **)curDataPtr = NULL;

						error =
							TMrInstance_GetDataPtr(
								targetDesc->templatePtr->tag,
								targetDesc->namePtr + 4,
								(void **)curDataPtr);

#if SUPPORT_DEBUG_FILES
						if(error != UUcError_None)
						{
							if (NULL == placeholder_file) {
								placeholder_file = BFrDebugFile_Open("placeholders.txt");
							}

							if (NULL != placeholder_file) {
								BFrDebugFile_Printf(placeholder_file, 
									"%s: could not find needed data - template %s[%s], name %s\n",
									inMsg,
									targetDesc->templatePtr->name,
									UUrTag2Str(targetDesc->templatePtr->tag),
									targetDesc->namePtr + 4);
							}
						}
#endif
					}
				}
				
				curSwapCode += 4;
				curDataPtr += 4;
				break;
			
			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}
	
	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
	
	return UUcError_None;
}

static UUtError
TMiGame_Instance_Callback(
	void*					inDataPtr,
	TMtTemplateProc_Message	inMessage)
{
	UUtError						error;
	TMtInstanceFile*				targetInstanceFile;
	UUtUns32						targetInstanceIndex;
	UUtUns16						itrPrivateInfo;
	TMtInstanceFile_PrivateInfo*	curPrivateInfo;
	TMtInstanceDescriptor*			targetInstanceDesc;
	TMtTemplateTag					targetTemplateTag;
	TMtPrivateData*					targetPrivateData;
	void**							targetInstancePtrList;
	
	UUmAssertReadPtr(inDataPtr, sizeof(void*));
	
	TMiGame_Instance_FileAndIndex_Get(
		inDataPtr,
		&targetInstanceFile,
		&targetInstanceIndex);
	
	targetInstanceDesc = targetInstanceFile->instanceDescriptors + targetInstanceIndex;
	
	targetTemplateTag = targetInstanceDesc->templatePtr->tag;
	
	for(itrPrivateInfo = 0, curPrivateInfo = targetInstanceFile->privateInfos;
		itrPrivateInfo < targetInstanceFile->numPrivateInfos;
		itrPrivateInfo++, curPrivateInfo++)
	{
		if(curPrivateInfo->type != TMcPrivateInfo_PrivateData) continue;
		if(curPrivateInfo->templateTag != targetTemplateTag) continue;
		
		targetPrivateData = curPrivateInfo->owner;
		UUmAssertReadPtr(targetPrivateData, sizeof(*targetPrivateData));
		
		if(curPrivateInfo->instanceListIndex != UUcMaxUns16)
		{
			UUmAssert(curPrivateInfo->instanceListIndex < targetInstanceFile->numInstancePtrLists);
			targetInstancePtrList = targetInstanceFile->instancePtrLists[curPrivateInfo->instanceListIndex];
		}
		else
		{
			targetInstancePtrList = NULL;
		}
		
		UUmAssert(targetPrivateData->procHandler != NULL);
		error = 
			targetPrivateData->procHandler(
				inMessage,
				inDataPtr,
				targetInstancePtrList ? targetInstancePtrList[targetInstanceIndex] : NULL);
		UUmError_ReturnOnError(error);
	}
	
	return UUcError_None;
}

static UUtError
TMiGame_InstanceFile_Callback(
	TMtInstanceFile*		inInstanceFile,
	TMtTemplateProc_Message	inMessage)
{
	UUtError						error;
	UUtUns16						itrPrivateInfo;
	TMtInstanceFile_PrivateInfo*	curPrivateInfo;
	UUtUns32						curDescIndex;
	TMtInstanceDescriptor*			curInstanceDesc;
	TMtTemplateTag					targetTemplateTag;
	TMtPrivateData*					targetPrivateData;
	void**							targetInstancePtrList;
	
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));
	
	for(itrPrivateInfo = 0, curPrivateInfo = inInstanceFile->privateInfos;
		itrPrivateInfo < inInstanceFile->numPrivateInfos;
		itrPrivateInfo++, curPrivateInfo++)
	{
		if(curPrivateInfo->type != TMcPrivateInfo_PrivateData) continue;
		
		targetTemplateTag = curPrivateInfo->templateTag;
		
		targetPrivateData = curPrivateInfo->owner;
		UUmAssertReadPtr(targetPrivateData, sizeof(*targetPrivateData));

		if(curPrivateInfo->instanceListIndex != UUcMaxUns16)
		{
			UUmAssert(curPrivateInfo->instanceListIndex < inInstanceFile->numInstancePtrLists);
			targetInstancePtrList = inInstanceFile->instancePtrLists[curPrivateInfo->instanceListIndex];
		}
		else
		{
			targetInstancePtrList = NULL;
		}
	
		for(curDescIndex = 0, curInstanceDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curInstanceDesc++)
		{
			TMtTemplateDefinition *template_def = curInstanceDesc->templatePtr;

			if(curInstanceDesc->dataPtr == NULL) continue;
			if(targetTemplateTag != template_def->tag) continue;
			 
			if (TMgTimeLevelLoad) {
				if (NULL == template_def->timer) {
					template_def->timer = UUrTimer_Allocate("callback", template_def->name);
				}

				UUrTimer_Begin(template_def->timer);
			}
			
			error = 
				targetPrivateData->procHandler(
					inMessage,
					curInstanceDesc->dataPtr,
					targetInstancePtrList ? targetInstancePtrList[curDescIndex] : NULL);
			UUmError_ReturnOnError(error);

			if (TMgTimeLevelLoad) {
				UUrTimer_End(template_def->timer);
			}
		}
	}
	
	return UUcError_None;
}

static UUtError
TMiGame_InstanceFile_LoadHeaderFromMemory(
	TMtInstanceFile_Header	*inFileHeader,
	UUtBool					*outNeedsSwapping)
{
	UUtBool needsSwapping;

	needsSwapping = inFileHeader->version != TMcInstanceFile_Version;
	
	if(needsSwapping)
	{
		UUrSwap_4Byte(&inFileHeader->version);
		
		if(inFileHeader->version != TMcInstanceFile_Version)
		{
			return TMcError_DataCorrupt;
		}
		
		UUrSwap_2Byte(&inFileHeader->sizeofHeader);
		UUrSwap_2Byte(&inFileHeader->sizeofInstanceDescriptor);
		UUrSwap_2Byte(&inFileHeader->sizeofTemplateDescriptor);
		UUrSwap_2Byte(&inFileHeader->sizeofNameDescriptor);
		
		UUrSwap_8Byte(&inFileHeader->totalTemplateChecksum);

		UUrSwap_4Byte(&inFileHeader->numInstanceDescriptors);
		UUrSwap_4Byte(&inFileHeader->numNameDescriptors);
		UUrSwap_4Byte(&inFileHeader->numTemplateDescriptors);
		
		UUrSwap_4Byte(&inFileHeader->dataBlockOffset);
		UUrSwap_4Byte(&inFileHeader->dataBlockLength);

		UUrSwap_4Byte(&inFileHeader->nameBlockOffset);
		UUrSwap_4Byte(&inFileHeader->nameBlockLength);
	}
	
	if(inFileHeader->sizeofHeader != sizeof(TMtInstanceFile_Header)) return TMcError_DataCorrupt;
	if(inFileHeader->sizeofInstanceDescriptor != sizeof(TMtInstanceDescriptor)) return TMcError_DataCorrupt;
	if(inFileHeader->sizeofTemplateDescriptor != sizeof(TMtTemplateDescriptor)) return TMcError_DataCorrupt;
	if(inFileHeader->sizeofNameDescriptor != sizeof(TMtNameDescriptor)) return TMcError_DataCorrupt;
	
	UUmAssert(inFileHeader->numNameDescriptors <= inFileHeader->numInstanceDescriptors);
	UUmAssert(inFileHeader->dataBlockOffset >= 
		sizeof(TMtInstanceFile_Header) + 
		inFileHeader->numInstanceDescriptors * sizeof(TMtInstanceDescriptor) + 
		inFileHeader->numNameDescriptors * sizeof(TMtNameDescriptor) +
		inFileHeader->numTemplateDescriptors * sizeof(TMtTemplateDescriptor));
	UUmAssert(inFileHeader->nameBlockOffset >= 
		sizeof(TMtInstanceFile_Header) + 
		inFileHeader->numInstanceDescriptors * sizeof(TMtInstanceDescriptor) + 
		inFileHeader->numNameDescriptors * sizeof(TMtNameDescriptor) + 
		inFileHeader->numTemplateDescriptors * sizeof(TMtTemplateDescriptor) +
		inFileHeader->dataBlockLength);
		
	UUmAssert(inFileHeader->dataBlockOffset + inFileHeader->dataBlockLength <= inFileHeader->nameBlockOffset);
	
	*outNeedsSwapping = needsSwapping;
	
	return UUcError_None;
}

static UUtError
TMiGame_InstanceFile_LoadHeader(
	BFtFile*				inFile,
	TMtInstanceFile_Header	*outFileHeader,
	UUtBool					*outNeedsSwapping)
{
	UUtError	error;
	
	error = 
		BFrFile_ReadPos(
			inFile,
			0,
			sizeof(TMtInstanceFile_Header),
			outFileHeader);
	UUmError_ReturnOnError(error);

	error = TMiGame_InstanceFile_LoadHeaderFromMemory(outFileHeader, outNeedsSwapping);

	return error;
}

static UUtError
TMiGame_InstanceFile_PrivateInfo_New(
	TMtInstanceFile*					inInstanceFile,
	TMtInstanceFile_PrivateInfo_Type	inPrivateInfoType,
	TMtTemplateTag						inTemplateTag,
	void*								inOwner,
	UUtBool								inAllocateList,
	TMtInstanceFile_PrivateInfo*		*outPrivateInfo)
{
	TMtInstanceFile_PrivateInfo*	newPrivateInfo;
	UUtUns32						itr;
	UUtBool							indexUsed[TMcGame_InstancePtrList_Max];
	UUtUns16						targetInstanceListIndex;
	
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));
	UUmAssertReadPtr(inOwner, sizeof(void*));
	UUmAssertReadPtr(outPrivateInfo, sizeof(*outPrivateInfo));
	
	*outPrivateInfo = NULL;
	
	// first check to see if this private info has already has been added
	for(itr = 0; itr < inInstanceFile->numPrivateInfos; itr++)
	{
		if(inInstanceFile->privateInfos[itr].owner == inOwner)
		{
			return UUcError_None;
		}
	}
	
	if(inInstanceFile->numPrivateInfos >= TMcGame_PrivateInfosPerInstanceFile_Max)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Exceeded max private infos per instance file");
	}
	
	// look for a instance ptr list index
	if(inAllocateList)
	{
		UUrMemory_Set16(indexUsed, UUcFalse, TMcGame_InstancePtrList_Max * sizeof(UUtBool));
		for(itr = 0; itr < inInstanceFile->numPrivateInfos; itr++)
		{
			if(inInstanceFile->privateInfos[itr].templateTag == inTemplateTag)
			{
				targetInstanceListIndex = inInstanceFile->privateInfos[itr].instanceListIndex;
				if(targetInstanceListIndex != UUcMaxUns16)
				{
					UUmAssert(targetInstanceListIndex < inInstanceFile->numInstancePtrLists);
					indexUsed[targetInstanceListIndex] = UUcTrue;
				}
			}
		}
		
		for(targetInstanceListIndex = 0; targetInstanceListIndex < TMcGame_InstancePtrList_Max; targetInstanceListIndex++)
		{
			if(indexUsed[targetInstanceListIndex] == UUcFalse) break;
		}
		
		// return an error if no more slots are available
		if(targetInstanceListIndex == TMcGame_InstancePtrList_Max)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "no ore instance ptr lists");
		}
		
		// allocate a new instance ptr list if needed
		if(targetInstanceListIndex >= inInstanceFile->numInstancePtrLists)
		{
			UUmAssert(inInstanceFile->numInstancePtrLists == targetInstanceListIndex);
			
			inInstanceFile->instancePtrLists[inInstanceFile->numInstancePtrLists] =
				UUrMemory_Block_NewClear(inInstanceFile->maxInstanceDescriptors * sizeof(void*));
			
			UUmError_ReturnOnNull(inInstanceFile->instancePtrLists[inInstanceFile->numInstancePtrLists]);
			
			inInstanceFile->numInstancePtrLists++;
		}
	}
	else
	{
		targetInstanceListIndex = UUcMaxUns16;
	}
	
	// create the new private info
		newPrivateInfo = inInstanceFile->privateInfos + inInstanceFile->numPrivateInfos++;
		
		newPrivateInfo->type = inPrivateInfoType;
		newPrivateInfo->owner = inOwner;
		newPrivateInfo->templateTag = inTemplateTag;
		newPrivateInfo->memoryPool = NULL;
		newPrivateInfo->instanceListIndex = targetInstanceListIndex;

	*outPrivateInfo = newPrivateInfo;
	
	return UUcError_None;
}

static void
TMiGame_InstanceFile_PrivateInfo_Delete_ForRealThisTime(
	TMtInstanceFile*				inInstanceFile,
	TMtInstanceFile_PrivateInfo*	inPrivateInfo)
{
	if(inPrivateInfo->type == TMcPrivateInfo_PrivateData)
	{
		if(inPrivateInfo->memoryPool != NULL)
		{
			UUrMemory_Block_Delete(inPrivateInfo->memoryPool);
		}
	}

	return;
}

static void
TMiGame_InstanceFile_PrivateInfo_Delete(
	TMtInstanceFile*	inInstanceFile,
	void*				inOwner)
{
	UUtUns16						itrPrivateInfo;
	TMtInstanceFile_PrivateInfo*	curPrivateInfo;
	
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));
	UUmAssertReadPtr(inOwner, sizeof(void*));

	for(itrPrivateInfo = 0, curPrivateInfo = inInstanceFile->privateInfos;
		itrPrivateInfo < inInstanceFile->numPrivateInfos;
		itrPrivateInfo++, curPrivateInfo++)
	{
		if(curPrivateInfo->owner == inOwner)
		{			
			TMiGame_InstanceFile_PrivateInfo_Delete_ForRealThisTime(inInstanceFile, curPrivateInfo);
			
			UUrMemory_ArrayElement_Delete(
				inInstanceFile->privateInfos,
				itrPrivateInfo,
				inInstanceFile->numPrivateInfos,
				sizeof(TMtInstanceFile_PrivateInfo));
			
			inInstanceFile->numPrivateInfos--;
			
			return;
		}
	}
}

static UUtError
TMiGame_InstanceFile_PrivateData_New(
	TMtInstanceFile*	inInstanceFile,
	TMtPrivateData*		inPrivateData,
	UUtBool				inAllocateDynamicData)
{
	UUtError						error;
	TMtInstanceFile_PrivateInfo*	newPrivateInfo;
	UUtUns32						itr;
	TMtInstanceDescriptor*			curInstanceDesc;
	UUtUns32						numInstances = 0;
	UUtUns8*						curMemoryPtr;
	void**							targetInstancePtrList;
	
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));
	UUmAssertReadPtr(inPrivateData, sizeof(*inPrivateData));
	
	error = 
		TMiGame_InstanceFile_PrivateInfo_New(
			inInstanceFile,
			TMcPrivateInfo_PrivateData,
			inPrivateData->templateTag,
			inPrivateData,
			inPrivateData->dataSize > 0 ? UUcTrue : UUcFalse,
			&newPrivateInfo);
	UUmError_ReturnOnError(error);
	
	if(inPrivateData->dataSize == 0) return UUcError_None;
	if(inAllocateDynamicData == UUcFalse) return UUcError_None;
	
	for(itr = 0, curInstanceDesc = inInstanceFile->instanceDescriptors;
		itr < inInstanceFile->numInstanceDescriptors;
		itr++, curInstanceDesc++)
	{
		if(curInstanceDesc->dataPtr == NULL) continue;
		
		if(curInstanceDesc->templatePtr->tag == inPrivateData->templateTag) numInstances++;
	}
	
	newPrivateInfo->memoryPool = curMemoryPtr = UUrMemory_Block_New(numInstances * inPrivateData->dataSize);
	UUmError_ReturnOnNull(newPrivateInfo->memoryPool);
	
	UUmAssert(newPrivateInfo->instanceListIndex < inInstanceFile->numInstancePtrLists);
	targetInstancePtrList = inInstanceFile->instancePtrLists[newPrivateInfo->instanceListIndex];

	for(itr = 0, curInstanceDesc = inInstanceFile->instanceDescriptors;
		itr < inInstanceFile->numInstanceDescriptors;
		itr++, curInstanceDesc++)
	{
		if(curInstanceDesc->dataPtr == NULL) continue;

		if(curInstanceDesc->templatePtr->tag == inPrivateData->templateTag)
		{
			targetInstancePtrList[itr] = curMemoryPtr;
			curMemoryPtr += inPrivateData->dataSize;
		}
	}
	
	return UUcError_None;
}

static void
TMiGame_InstanceFile_PrivateData_Delete(
	TMtInstanceFile*	inInstanceFile,
	TMtPrivateData*		inPrivateData)
{
	
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));
	UUmAssertReadPtr(inPrivateData, sizeof(*inPrivateData));

	TMiGame_InstanceFile_PrivateInfo_Delete(inInstanceFile, inPrivateData);
}

static UUtError
TMiGame_InstanceFile_Dynamic_New(
	UUtUns32			inInstance_Max,
	UUtUns32			inMemoryPool_Size,
	UUtUns32			inInstanceFileIndex,
	TMtInstanceFile*	*outInstanceFile)
{
	TMtInstanceFile*		newInstanceFile;


	*outInstanceFile = NULL;
	
	/*
	 * Create the instance file structure
	 */
		newInstanceFile = (TMtInstanceFile *)UUrMemory_Block_New(sizeof(TMtInstanceFile));
		UUmError_ReturnOnNull(newInstanceFile);

	UUrString_Copy(newInstanceFile->fileName, "dynamic", BFcMaxFileNameLength);
	
	newInstanceFile->fileIndex = inInstanceFileIndex;

	newInstanceFile->numInstanceDescriptors = 0;
	newInstanceFile->instanceDescriptors = UUrMemory_Block_NewClear(inInstance_Max * sizeof(TMtInstanceDescriptor));
	UUmError_ReturnOnNull(newInstanceFile->instanceDescriptors);
		
	newInstanceFile->maxInstanceDescriptors = inInstance_Max;
	newInstanceFile->numNameDescriptors = 0;
	newInstanceFile->nameDescriptors = NULL;
	newInstanceFile->numTemplateDescriptors = 0;
	newInstanceFile->templateDescriptors = NULL;
	newInstanceFile->mapping= NULL;
	newInstanceFile->rawMapping= NULL;
	newInstanceFile->rawPtr = NULL;
	newInstanceFile->separateFile = NULL;

	newInstanceFile->dataBlock = NULL;
	newInstanceFile->nameBlock = NULL;
	newInstanceFile->final = UUcFalse;
	newInstanceFile->preparedForMemory = UUcTrue;
	newInstanceFile->dynamicBytesUsed = 0;
	newInstanceFile->dynamicPool = UUrMemory_Pool_New(inMemoryPool_Size, UUcPool_Fixed);
	UUmError_ReturnOnNull(newInstanceFile->dynamicPool);
	
	newInstanceFile->numInstancePtrLists = 0;
	newInstanceFile->numPrivateInfos = 0;
	
	*outInstanceFile = newInstanceFile;
	
	return UUcError_None;
}

static void
TMiGame_InstanceFile_Dynamic_Reset(
	TMtInstanceFile*	inInstanceFile)
{
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));

	// call the dispose proc
	TMiGame_InstanceFile_Callback(inInstanceFile, TMcTemplateProcMessage_DisposePreProcess);

	inInstanceFile->numInstanceDescriptors = 0;
	inInstanceFile->dynamicBytesUsed = 0;
	UUrMemory_Pool_Reset(inInstanceFile->dynamicPool);
}
	
	
static UUtError
TMiGame_InstanceFile_New_FromFileRef(
	BFtFileRef*			inInstanceFileRef,
	TMtInstanceFile*	*outInstanceFile)
{
	UUtError				error;
	BFtFile*				instancePhysicalFile = NULL;
	UUtBool					needsSwapping;
	TMtInstanceFile_Header*	fileHeader;
	UUtUns32				totalFileLength;
	TMtInstanceFile*		newInstanceFile;
	UUtUns32				curDescIndex;
	TMtInstanceDescriptor*	curDesc;
	UUtUns8*				mappingPtr;
	TMtNameDescriptor*		curNameDesc;

	UUmAssertReadPtr(inInstanceFileRef, sizeof(void*));

	*outInstanceFile = NULL;
	
	/*
	 * Create the instance file structure
	 */
		newInstanceFile = (TMtInstanceFile *)UUrMemory_Block_New(sizeof(TMtInstanceFile));
		UUmError_ReturnOnNull(newInstanceFile);
		
	/*
	 * map the file
	 */
		error = BFrFile_Map(
			inInstanceFileRef,
			0,
			&(newInstanceFile->mapping),
			&mappingPtr,
			&totalFileLength);

		if (error != UUcError_None)
		{
			error = error != UUcError_OutOfMemory ? TMcError_DataCorrupt : UUcError_OutOfMemory;
			UUmError_ReturnOnErrorMsg(error, "Could not open instance file");
		}
	

		{
			BFtFileRef *rawFileRef;
			UUtUns32 rawFileLength;

			error = TMrUtility_DataRef_To_BinaryRef(inInstanceFileRef, &rawFileRef, "raw");
			UUmError_ReturnOnError(error);

			error = BFrFile_Map(rawFileRef, 0, &newInstanceFile->rawMapping, &newInstanceFile->rawPtr, &rawFileLength);

			if (error) {
				newInstanceFile->rawPtr = NULL;
			}

			BFrFileRef_Dispose(rawFileRef);
		}
	
		{
			BFtFileRef *separateFileRef;

			error = TMrUtility_DataRef_To_BinaryRef(inInstanceFileRef, &separateFileRef, "sep");
			UUmError_ReturnOnError(error);

			error = BFrFile_Open(separateFileRef, "r", &newInstanceFile->separateFile);
			if (error != UUcError_None) {
				newInstanceFile->separateFile = NULL;
			}

			BFrFileRef_Dispose(separateFileRef);
		}

	fileHeader = (TMtInstanceFile_Header *) mappingPtr;
	error = TMiGame_InstanceFile_LoadHeaderFromMemory(fileHeader, &needsSwapping);
	
	UUmAssert(totalFileLength == fileHeader->nameBlockOffset + fileHeader->nameBlockLength);
	
	UUrString_Copy(newInstanceFile->fileName, BFrFileRef_GetLeafName(inInstanceFileRef), BFcMaxFileNameLength);
	
	newInstanceFile->preparedForMemory = UUcFalse;
	newInstanceFile->numInstanceDescriptors = newInstanceFile->maxInstanceDescriptors = fileHeader->numInstanceDescriptors;
	newInstanceFile->numNameDescriptors = fileHeader->numNameDescriptors;

	newInstanceFile->numTemplateDescriptors = 0;
	newInstanceFile->templateDescriptors = NULL;
		
	newInstanceFile->instanceDescriptors = (struct TMtInstanceDescriptor *) (mappingPtr + sizeof(TMtInstanceFile_Header));
	newInstanceFile->nameDescriptors = (struct TMtNameDescriptor *) (mappingPtr + sizeof(TMtInstanceFile_Header) + newInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor));
	newInstanceFile->dataBlock = (mappingPtr + fileHeader->dataBlockOffset);
	newInstanceFile->nameBlock = (char*)(mappingPtr + fileHeader->nameBlockOffset);
	newInstanceFile->dynamicBytesUsed = 0;
	newInstanceFile->dynamicPool = NULL;
	
	newInstanceFile->numInstancePtrLists = 0;
	newInstanceFile->numPrivateInfos = 0;
	
	// traverse the instance descriptors
	for(curDescIndex = 0, curDesc = newInstanceFile->instanceDescriptors;
		curDescIndex < newInstanceFile->numInstanceDescriptors;
		curDescIndex++, curDesc++)
	{
		if(needsSwapping)
		{
			UUrSwap_4Byte(&curDesc->templatePtr);
			UUrSwap_4Byte(&curDesc->dataPtr);
			UUrSwap_4Byte(&curDesc->namePtr);
			UUrSwap_4Byte(&curDesc->size);
			UUrSwap_4Byte(&curDesc->flags);
		}
		
		curDesc->templatePtr = TMrUtility_Template_FindDefinition((TMtTemplateTag)curDesc->templatePtr);
		
		UUmAssert(curDesc->templatePtr != NULL);

		curDesc->flags = (TMtDescriptorFlags)(curDesc->flags & TMcDescriptorFlags_PersistentMask);
		
		if(!(curDesc->flags & TMcDescriptorFlags_PlaceHolder))
		{
			curDesc->dataPtr = newInstanceFile->dataBlock + (UUtUns32)curDesc->dataPtr;
			
			if(needsSwapping && !(curDesc->flags & TMcDescriptorFlags_Duplicate))
			{
				UUtUns8*	swapCode;
				UUtUns8*	dataPtr;
				
				swapCode = curDesc->templatePtr->swapCodes;
				dataPtr = curDesc->dataPtr - TMcPreDataSize;
				
				error = TMiGame_Instance_ByteSwap(&swapCode, &dataPtr, newInstanceFile, curDesc->templatePtr->byteSwapProc, curDesc->dataPtr);
				UUmError_ReturnOnError(error);
			}
		}
		else
		{
			curDesc->dataPtr = NULL;
		}
		
		if(!(curDesc->flags & TMcDescriptorFlags_Unique))
		{
			curDesc->namePtr = (char*)newInstanceFile->nameBlock + (UUtUns32)curDesc->namePtr;
		}
		else
		{
			curDesc->namePtr = NULL;
		}
	}
	
	// Traverse through the name descs and update pointers
	for(curDescIndex = 0, curNameDesc = newInstanceFile->nameDescriptors;
		curDescIndex < newInstanceFile->numNameDescriptors;
		curDescIndex++, curNameDesc++)
	{
		if(needsSwapping)
		{
			UUrSwap_4Byte(&curNameDesc->instanceDescIndex);
		}
		
		UUmAssert(curNameDesc->instanceDescIndex < newInstanceFile->numInstanceDescriptors);
		
		curNameDesc->namePtr = 
			(newInstanceFile->instanceDescriptors + curNameDesc->instanceDescIndex)->namePtr;
		
		UUmAssert(curNameDesc->namePtr != NULL);
	}
	
	// add the private datas to this instance file
	
	*outInstanceFile = newInstanceFile;
	
	return UUcError_None;
}

static void
TMiGame_InstanceFile_Delete(
	TMtInstanceFile*	inInstanceFile)
{
	UUtUns16						itr;
	TMtInstanceFile_PrivateInfo*	curPrivateInfo;
	
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));
	
	TMiGame_InstanceFile_Callback(inInstanceFile, TMcTemplateProcMessage_DisposePreProcess);
	
	for(itr = 0, curPrivateInfo = inInstanceFile->privateInfos;
		itr < inInstanceFile->numPrivateInfos;
		itr++, curPrivateInfo++)
	{
		TMiGame_InstanceFile_PrivateInfo_Delete_ForRealThisTime(inInstanceFile, curPrivateInfo);
	}
	
	for(itr = 0; itr < inInstanceFile->numInstancePtrLists; itr++)
	{
		UUrMemory_Block_Delete(inInstanceFile->instancePtrLists[itr]);
	}
	
	if(inInstanceFile->mapping != NULL)
	{
		BFrFile_UnMap(inInstanceFile->mapping);
	}
	
	if (inInstanceFile->rawMapping != NULL)
	{
		BFrFile_UnMap(inInstanceFile->rawMapping);
	}
	
	if (inInstanceFile->separateFile != NULL)
	{
		BFrFile_Close(inInstanceFile->separateFile);
	}
	
	if(inInstanceFile->dynamicPool != NULL)
	{
		UUrMemory_Pool_Delete(inInstanceFile->dynamicPool);
		UUrMemory_Block_Delete(inInstanceFile->instanceDescriptors);
	}
	
	UUrMemory_Block_Delete(inInstanceFile);
}

static TMtInstanceDescriptor*
TMiGame_InstanceFile_GetInstanceDesc(
	TMtInstanceFile*	inInstanceFile,
	TMtPlaceHolder		inPlaceHolder)
{
	UUtUns32	index;
	
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));

	if(TMmPlaceHolder_IsPtr(inPlaceHolder))
	{
		UUmAssert(0);
	}
	else
	{
		index = TMmPlaceHolder_GetIndex(inPlaceHolder);
		
		UUmAssert(index < inInstanceFile->numInstanceDescriptors);
		
		return inInstanceFile->instanceDescriptors + index;
	}
	
	return NULL;
}

static void*
TMiGame_InstanceFile_GetDataPtr(
	TMtInstanceFile*	inInstanceFile,
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName)	
{
	UUtUns32			midIndex;
	UUtUns32			lowIndex, highIndex;
	TMtNameDescriptor*	curNameDesc;
	char				buffer[BFcMaxFileNameLength * 2];
	UUtInt16			compareResult;
	
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));
	UUmAssertReadPtr(inInstanceName, 1);
	UUmAssert(strlen(inInstanceName) < TMcInstanceName_MaxLength);

	buffer[0] = (char)((inTemplateTag >> 24) & 0xFF);
	buffer[1] = (char)((inTemplateTag >> 16) & 0xFF);
	buffer[2] = (char)((inTemplateTag >> 8) & 0xFF);
	buffer[3] = (char)((inTemplateTag >> 0) & 0xFF);
	
	UUrString_Copy(buffer + 4, inInstanceName, BFcMaxFileNameLength * 2 - 4);
	
	lowIndex = 0;
	highIndex = inInstanceFile->numNameDescriptors;
	
	while(lowIndex < highIndex)
	{
		midIndex = (lowIndex + highIndex) >> 1;
		
		curNameDesc = inInstanceFile->nameDescriptors + midIndex;
		
		compareResult = strcmp(buffer, curNameDesc->namePtr);
		
		if(compareResult == 0)
		{
			return (inInstanceFile->instanceDescriptors + curNameDesc->instanceDescIndex)->dataPtr;
		}
		else if(compareResult > 0)
		{
			lowIndex = midIndex + 1;
		}
		else
		{
			highIndex = midIndex;
		}
	}
	
	#if 0
	for(itrNameDesc = 0, curNameDesc = inInstanceFile->nameDescriptors;
		itrNameDesc < inInstanceFile->numNameDescriptors;
		itrNameDesc++, curNameDesc++)
	{
		if(curNameDesc->namePtr[0] != c0 ||
			curNameDesc->namePtr[1] != c1 ||
			curNameDesc->namePtr[2] != c2 ||
			curNameDesc->namePtr[3] != c3) continue;
			
		if(!strcmp(curNameDesc->namePtr + 4, inInstanceName))
		{
			return (inInstanceFile->instanceDescriptors + curNameDesc->instanceDescIndex)->dataPtr;
		}
	}
	#endif
	
	return NULL;
}

static UUtError
TMiGame_InstanceFile_PrepareForMemory(
	TMtInstanceFile*	inInstanceFile)
{
	UUtError				error;
	UUtUns32				curDescIndex;
	TMtInstanceDescriptor*	curDesc;
	
	UUtUns8*				swapCode;
	UUtUns8*				dataPtr;
	char					msg[2048];
	
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));
	
	if(inInstanceFile->preparedForMemory == UUcTrue) return UUcError_None;
	inInstanceFile->preparedForMemory = UUcTrue;
	
	for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curDesc++)
	{
		if(curDesc->templatePtr->flags & TMcTemplateFlag_Leaf) continue;
		
		if(curDesc->flags & TMcDescriptorFlags_Duplicate) continue;
		
		if(curDesc->dataPtr == NULL) continue;

		swapCode = curDesc->templatePtr->swapCodes;
		dataPtr = curDesc->dataPtr - TMcPreDataSize;
		
		sprintf(
			msg,
			"%s: template %s[%s], name %s",
			inInstanceFile->fileName,
			curDesc->templatePtr->name,
			UUrTag2Str(curDesc->templatePtr->tag),
			(curDesc->namePtr!=NULL) ? curDesc->namePtr + 4 : "NULL");
		
		error = 
			TMiGame_Instance_PrepareForMemory(
				&swapCode,
				&dataPtr,
				inInstanceFile,
				msg,
				(curDesc->templatePtr->flags & TMcTemplateFlag_VarArrayIsLeaf) ? UUcTrue : UUcFalse);
		UUmError_ReturnOnError(error);
	}
	
	error = TMiGame_InstanceFile_Callback(inInstanceFile, TMcTemplateProcMessage_LoadPostProcess);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

static TMtInstanceFile*
TMiGame_InstanceFile_GetFromIndex(
	UUtUns32	inInstanceFileIndex)
{
	UUtUns16	itr;
	
	for(itr = 0; itr < TMgGame_LoadedInstanceFiles_Num; itr++)
	{
		if(TMgGame_LoadedInstanceFiles_List[itr]->fileIndex == inInstanceFileIndex)
		{
			return TMgGame_LoadedInstanceFiles_List[itr];
		}
	}
	
	return NULL;
}

static UUtError
TMiGame_InstanceFile_Instance_Dynamic_New(
	TMtInstanceFile*	inInstanceFile,
	TMtTemplateTag		inTemplateTag,
	UUtUns32			inInitialVarArrayLength,
	void*				*outDataPtr)
{
	UUtError						error;
	TMtInstanceDescriptor*			newInstanceDesc;
	UUtUns32						newInstanceDescIndex;
	TMtTemplateDefinition*			templateDef;
	UUtUns32						dataSize;
	UUtUns8*						newDataPtr;
	UUtUns16						itr;
	UUtUns16						itrPrivateInfo;
	TMtInstanceFile_PrivateInfo*	curPrivateInfo;
	void*							newPrivateData;
	void**							targetInstancePtrList;
	TMtPrivateData*					targetPDOwner;
	
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));

	*outDataPtr = NULL;
	
	templateDef = TMrUtility_Template_FindDefinition(inTemplateTag);
	if(templateDef == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find template definition");
	}
	
	if(inInstanceFile->numInstanceDescriptors >= inInstanceFile->maxInstanceDescriptors)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Out of instance descriptors");
	}
	
	newInstanceDescIndex = inInstanceFile->numInstanceDescriptors++;
	newInstanceDesc = inInstanceFile->instanceDescriptors + newInstanceDescIndex;
	
	newInstanceDesc->templatePtr = templateDef;
	newInstanceDesc->namePtr = NULL;
	
	dataSize = templateDef->size + inInitialVarArrayLength * templateDef->varArrayElemSize;
	
	dataSize = (dataSize + UUcProcessor_CacheLineSize_Mask) & ~UUcProcessor_CacheLineSize_Mask;

	inInstanceFile->dynamicBytesUsed += dataSize;
	newDataPtr = UUrMemory_Pool_Block_New(inInstanceFile->dynamicPool, dataSize);
	UUmError_ReturnOnNull(newDataPtr);
	
	//UUrMemory_Set32(newDataPtr, 0xBADBADBA, dataSize);
	
	((UUtUns32*)newDataPtr)[0] = TMmPlaceHolder_MakeFromIndex(newInstanceDescIndex);
	((TMtInstanceFile**)newDataPtr)[1] = inInstanceFile;
	
	newDataPtr = newDataPtr + TMcPreDataSize;

	newInstanceDesc->size = dataSize;
	newInstanceDesc->dataPtr = newDataPtr;
	
	{
		UUtUns8*	dataPtr	= newDataPtr - TMcPreDataSize;
		UUtUns8*	swapCodes = templateDef->swapCodes;
		
		TMrUtility_VarArrayReset_Recursive(
			&swapCodes,
			&dataPtr,
			inInitialVarArrayLength);
	}
	
	UUrMemory_Leak_ForceGlobal_Begin();
	
	// make sure that all private datas of this template type are added to this instance file
	for(itr = 0; itr < TMgGame_PrivateData_Num; itr++)
	{
		if(TMgGame_PrivateData_List[itr].templateTag == inTemplateTag)
		{
			error = 
				TMiGame_InstanceFile_PrivateData_New(
					inInstanceFile,
					TMgGame_PrivateData_List + itr,
					UUcFalse);
			UUmError_ReturnOnError(error);
		}
	}
	
	// allocate all the private data needed
	for(itrPrivateInfo = 0, curPrivateInfo = inInstanceFile->privateInfos;
		itrPrivateInfo < inInstanceFile->numPrivateInfos;
		itrPrivateInfo++, curPrivateInfo++)
	{
		if(curPrivateInfo->type != TMcPrivateInfo_PrivateData) continue;
		
		targetPDOwner = curPrivateInfo->owner;
		if(targetPDOwner->dataSize == 0) continue;
		
		UUmAssert(curPrivateInfo->instanceListIndex < inInstanceFile->numInstancePtrLists);
		targetInstancePtrList = inInstanceFile->instancePtrLists[curPrivateInfo->instanceListIndex];
		
		inInstanceFile->dynamicBytesUsed += targetPDOwner->dataSize;
		newPrivateData =
			UUrMemory_Pool_Block_New(
				inInstanceFile->dynamicPool,
				targetPDOwner->dataSize);
		UUmError_ReturnOnNull(newPrivateData);
		
		targetInstancePtrList[newInstanceDescIndex] = newPrivateData;
	}
	 	
	UUrMemory_Leak_ForceGlobal_End();
	
	*outDataPtr = newDataPtr;
	
	return UUcError_None;
}

static UUtBool
TMiGame_InstanceFile_ContainsTemplate(
	TMtInstanceFile*	inInstanceFile,
	TMtTemplateTag		inTemplateTag)
{
	UUtUns32				itr;
	TMtInstanceDescriptor*	curInstanceDesc;
	
	UUmAssertReadPtr(inInstanceFile, sizeof(*inInstanceFile));

	for(itr = 0, curInstanceDesc = inInstanceFile->instanceDescriptors;
		itr < inInstanceFile->numInstanceDescriptors;
		itr++, curInstanceDesc++)
	{
		if(curInstanceDesc->dataPtr == NULL) continue;
		
		if(curInstanceDesc->templatePtr->tag == inTemplateTag) return UUcTrue;
	}
	
	return UUcFalse;
}

static void*
TMiGame_InstanceFile_PrivateInfo_GetDataPtr(
	TMtInstanceFile*				inInstanceFile,
	TMtInstanceFile_PrivateInfo*	inPrivateInfo,
	UUtUns32						inInstanceIndex)
{
	void**							targetInstancePtrList;
	
	if(inPrivateInfo->instanceListIndex == UUcMaxUns16) return NULL;
	
	UUmAssert(inPrivateInfo->instanceListIndex < inInstanceFile->numInstancePtrLists);
	
	targetInstancePtrList = inInstanceFile->instancePtrLists[inPrivateInfo->instanceListIndex];
	
	return targetInstancePtrList[inInstanceIndex];
}	

static UUtError
TMiGame_LoadedInstanceFiles_Add(
	UUtUns16	inInstanceFileRefIndex)
{
	UUtError			error;
	TMtInstanceFileRef*	targetInstanceFileRef;
	TMtInstanceFile*	newInstanceFile;
	UUtUns16			itr;
	
	targetInstanceFileRef = TMgGame_InstanceFileRefs_List + inInstanceFileRefIndex;
	
	error =
		TMiGame_InstanceFile_New_FromFileRef(
			&targetInstanceFileRef->instanceFileRef,
			&newInstanceFile);
	UUmError_ReturnOnError(error);
	
	newInstanceFile->fileIndex = targetInstanceFileRef->fileIndex;
	
	TMgGame_LoadedInstanceFiles_List[TMgGame_LoadedInstanceFiles_Num] = newInstanceFile;
	TMgGame_LoadedInstanceFiles_Num++;
	
	// loop through all the private datas and see if any applied to this instance file
	for(itr = 0; itr < TMgGame_PrivateData_Num; itr++)
	{
		if(TMiGame_InstanceFile_ContainsTemplate(
			newInstanceFile,
			TMgGame_PrivateData_List[itr].templateTag) == UUcTrue)
		{
			error = 
				TMiGame_InstanceFile_PrivateData_New(
					newInstanceFile,
					TMgGame_PrivateData_List + itr,
					UUcTrue);
			UUmError_ReturnOnError(error);
		}
	}
		
	return UUcError_None;
}

static void
TMiGame_LoadedInstanceFiles_Remove(
	UUtUns16	inLevelNumber)
{
	UUtInt16	itr;
	UUtUns16	numDeleted = 0;
	
	for(itr = TMgGame_LoadedInstanceFiles_Num; itr-- > 0;)
	{
		if(TMmFileIndex_LevelNumber_Get(TMgGame_LoadedInstanceFiles_List[itr]->fileIndex) == inLevelNumber)
		{
			TMiGame_InstanceFile_Delete(TMgGame_LoadedInstanceFiles_List[itr]);
			
			if(TMgGame_LoadedInstanceFiles_Num - itr - 1 > 0)
			{
				UUrMemory_ArrayElement_Delete(
					TMgGame_LoadedInstanceFiles_List,
					itr,
					TMgGame_LoadedInstanceFiles_Num,
					sizeof(TMtInstanceFile*));
			}
			
			TMgGame_LoadedInstanceFiles_Num--;
		}
	}
}

static void
TMiGame_LoadedInstanceFiles_PrepareForMemory(
	void)
{
	UUtUns16	itr;
	
	for(itr = 0; itr < TMgGame_LoadedInstanceFiles_Num; itr++)
	{
		TMiGame_InstanceFile_PrepareForMemory(TMgGame_LoadedInstanceFiles_List[itr]);
	}
}

static UUtError
TMiGame_LoadedInstanceFiles_PrivateData_Add(
	TMtPrivateData*	inPrivateData)
{
	UUtError	error;
	UUtUns16	itr;
	
	UUmAssertReadPtr(inPrivateData, sizeof(*inPrivateData));
	
	// loop through all the loaded files. If a file has some instances of the same template as inPrivateData
	// then add a private data instance file entry
	for(itr = 0; itr < TMgGame_LoadedInstanceFiles_Num; itr++)
	{
		if(TMiGame_InstanceFile_ContainsTemplate(
				TMgGame_LoadedInstanceFiles_List[itr],
				inPrivateData->templateTag) == UUcTrue)
		{
			// create and add a TMtPrivateData_InstanceFile
			error = 
				TMiGame_InstanceFile_PrivateData_New(
					TMgGame_LoadedInstanceFiles_List[itr],
					inPrivateData,
					UUcTrue);
			UUmError_ReturnOnError(error);
		}
	}
	
	return UUcError_None;
}

static void
TMiGame_LoadedInstanceFiles_PrivateData_Remove(
	TMtPrivateData*	inPrivateData)
{
	UUtUns16			itrInstanceFile;
	TMtInstanceFile*	curInstanceFile;
	
	UUmAssertReadPtr(inPrivateData, sizeof(*inPrivateData));
	
	for(itrInstanceFile = 0; itrInstanceFile < TMgGame_LoadedInstanceFiles_Num; itrInstanceFile++)
	{
		curInstanceFile = TMgGame_LoadedInstanceFiles_List[itrInstanceFile];
		
		TMiGame_InstanceFile_PrivateData_Delete(curInstanceFile, inPrivateData);
	}
	
}

static UUtError
TMiGame_LoadedInstanceFiles_PrivateData_Callback(
	TMtPrivateData*			inPrivateData,
	TMtTemplateProc_Message	inMessage)
{
	UUtError						error;
	UUtUns16						itrInstanceFile;
	TMtInstanceFile*				curInstanceFile;
	UUtUns16						itrPrivateInfo;
	TMtInstanceFile_PrivateInfo*	curPrivateInfo;
	UUtUns32						curDescIndex;
	TMtInstanceDescriptor*			curInstanceDesc;
	TMtTemplateTag					targetTemplateTag;
	void**							targetInstancePtrList;
	
	targetTemplateTag = inPrivateData->templateTag;

	for(itrInstanceFile = 0; itrInstanceFile < TMgGame_LoadedInstanceFiles_Num; itrInstanceFile++)
	{
		curInstanceFile = TMgGame_LoadedInstanceFiles_List[itrInstanceFile];
		
		// check to see if this private data is in this instance file
		for(itrPrivateInfo = 0, curPrivateInfo = curInstanceFile->privateInfos;
			itrPrivateInfo < curInstanceFile->numPrivateInfos;
			itrPrivateInfo++, curPrivateInfo++)
		{
			if(curPrivateInfo->owner == inPrivateData) break;
		}
		
		if(itrPrivateInfo < curInstanceFile->numPrivateInfos)
		{
			if(curPrivateInfo->instanceListIndex != UUcMaxUns16)
			{
				UUmAssert(curPrivateInfo->instanceListIndex < curInstanceFile->numInstancePtrLists);
				targetInstancePtrList = curInstanceFile->instancePtrLists[curPrivateInfo->instanceListIndex];
			}
			else
			{
				targetInstancePtrList = NULL;
			}
			
			// do the callback
			for(curDescIndex = 0, curInstanceDesc = curInstanceFile->instanceDescriptors;
				curDescIndex < curInstanceFile->numInstanceDescriptors;
				curDescIndex++, curInstanceDesc++)
			{
				if(curInstanceDesc->dataPtr == NULL) continue;
				
				if(targetTemplateTag == curInstanceDesc->templatePtr->tag)
				{
					error = 
						inPrivateData->procHandler(
							inMessage,
							curInstanceDesc->dataPtr,
							targetInstancePtrList ? targetInstancePtrList[curDescIndex] : NULL);
					UUmError_ReturnOnError(error);
				}
			}
		}
	}
	
	return UUcError_None;
}

static UUtError
TMiGame_InstanceFileRef_Add(
	BFtFileRef*	inInstanceFileRef,
	UUtUns32	inInstanceFileIndex)
{
	UUtUns16	itr;
	
	UUmAssertReadPtr(inInstanceFileRef, sizeof(void*));

	for(itr = 0; itr < TMgGame_InstanceFileRefs_Num; itr++)
	{
		if(TMgGame_InstanceFileRefs_List[itr].fileIndex == inInstanceFileIndex)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Conflicting file indices");
		}
	}
	
	TMgGame_InstanceFileRefs_List[TMgGame_InstanceFileRefs_Num].instanceFileRef = *inInstanceFileRef;
	TMgGame_InstanceFileRefs_List[TMgGame_InstanceFileRefs_Num].fileIndex = inInstanceFileIndex;
	TMgGame_InstanceFileRefs_Num++;
	
	return UUcError_None;
}

static UUtError
TMiGame_InstanceFileRef_LoadLevel(
	UUtUns16	inLevelNumber,
	UUtBool		inAllowPrivateData)
{
	UUtError	error;
	UUtUns16	itr;
	
	for(itr = 0; itr < TMgGame_InstanceFileRefs_Num; itr++)
	{
		if(TMmFileIndex_LevelNumber_Get(TMgGame_InstanceFileRefs_List[itr].fileIndex) == inLevelNumber)
		{
			error = TMiGame_LoadedInstanceFiles_Add(itr);
			UUmError_ReturnOnError(error);
		}
	}
	
	return UUcError_None;
}	


/*
 * =========================================================================
 * P U B L I C   F U N C T I O N S
 * =========================================================================
 */

UUtError
TMrGame_Initialize(
	void)
{
	UUtError			error;
	BFtFileIterator*	fileIterator;
	UUtUns32			itr;
	UUtUns32			curFileLevelNum;
	UUtBool				curFileLevelIsFinal;
	UUtUns32			curFileIndex;
	char				curFileSuffix[BFcMaxFileNameLength];
	
	// Initialize the global variables
		for(itr = 0; itr < TMcLevels_MaxNum; itr++) TMgGame_ValidLevels[itr] = UUcFalse;
	
	// Loop through each level and check to see if it is legal	
		error =
			BFrDirectory_FileIterator_New(
				&TMgDataFolderRef,
				"level",
				".dat",
				&fileIterator);
		if (error != UUcError_None)
		{
			UUrStartupMessage("Unable to create a file iterator for the data folder.");
			return UUcFalse;
		}
		UUrStartupMessage("Created a file iterator for the data folder.");
		
		while(1)
		{
			BFtFileRef curDatFileRef;

			// get the next file in the directory from the file iterator
			error = BFrDirectory_FileIterator_Next(fileIterator, &curDatFileRef);
			if(error != UUcError_None)
			{
				break;
			}

#if SHIPPING_VERSION
			{
				const char *file_leaf_name = BFrFileRef_GetLeafName(&curDatFileRef);

				if (file_leaf_name != NULL) {
					UUtBool is_tool_file = UUcFalse;

					is_tool_file = is_tool_file || (NULL != strstr(file_leaf_name, "_tools.dat"));
					is_tool_file = is_tool_file || (NULL != strstr(file_leaf_name, "_Tools.dat"));

					if (is_tool_file) {
						UUrStartupMessage("skipping tool file %s", file_leaf_name);
						
						continue;
					}
				}
			}
#endif
			
			// Get the file info
			error = 
				TMrUtility_LevelInfo_Get(
					&curDatFileRef,
					&curFileLevelNum,
					curFileSuffix,
					&curFileLevelIsFinal,
					&curFileIndex);
			if(error != UUcError_None)
			{
				UUrStartupMessage("Unable to get Level Info for %s.", BFrFileRef_GetLeafName(&curDatFileRef));
				continue;
			}
			UUrStartupMessage("Got Level Info for %s.", BFrFileRef_GetLeafName(&curDatFileRef));
			
			// Check to see if this is really a up to date level
			if(TMiGame_Level_IsValid(&curDatFileRef, curFileLevelNum))
			{
				if(curFileLevelIsFinal)
				{
					UUmAssert(TMgGame_ValidLevels[curFileLevelNum] == UUcFalse);
					
					TMgGame_ValidLevels[curFileLevelNum] = UUcTrue;
					UUrStartupMessage("Valid Level %s", BFrFileRef_GetLeafName(&curDatFileRef));
				}
				
				error = 
					TMiGame_InstanceFileRef_Add(
						&curDatFileRef,
						curFileIndex);
				UUmError_ReturnOnError(error);
			}
			else
			{
				UUrStartupMessage("Invalid Level %s", BFrFileRef_GetLeafName(&curDatFileRef));
			}
		}
		
		BFrDirectory_FileIterator_Delete(fileIterator);
	
	// create the dynamic instance files

#if ENABLE_TEMPORARY_INSTANCES
		error =
			TMiGame_InstanceFile_Dynamic_New(
				TMcDynamic_Instance_Temp_Max,
				TMcDynamic_Memory_Temp_Size,
				TMcDynamic_InstanceFileIndex_Temp,
				&TMgGame_DynamicInstanceFile_Temp);
		UUmError_ReturnOnError(error);
#else
		// CB: we no longer create instance files that we want to only persist
		// between level load / unload, so this memory pool is no longer useful
		TMgGame_DynamicInstanceFile_Temp = NULL;
#endif

		error =
			TMiGame_InstanceFile_Dynamic_New(
				TMcDynamic_Instance_Perm_Max,
				TMcDynamic_Memory_Perm_Size,
				TMcDynamic_InstanceFileIndex_Perm,
				&TMgGame_DynamicInstanceFile_Perm);
		UUmError_ReturnOnError(error);
		
#if ENABLE_TEMPORARY_INSTANCES
	TMgGame_LoadedInstanceFiles_List[0] = TMgGame_DynamicInstanceFile_Temp;
	TMgGame_LoadedInstanceFiles_List[1] = TMgGame_DynamicInstanceFile_Perm;
	TMgGame_LoadedInstanceFiles_Num = 2;
#else
	TMgGame_LoadedInstanceFiles_List[0] = TMgGame_DynamicInstanceFile_Perm;
	TMgGame_LoadedInstanceFiles_Num = 1;
#endif

	UUmAssert(TMgGame_LoadedInstanceFiles_Num == TMcGame_InstanceFile_Dynamic_Num);

	return UUcError_None;
}

void
TMrGame_Terminate(
	void)
{
	if (TMgGame_DynamicInstanceFile_Temp != NULL) {
		TMiGame_InstanceFile_Delete(TMgGame_DynamicInstanceFile_Temp);
	}

	if (TMgGame_DynamicInstanceFile_Perm != NULL) {
		TMiGame_InstanceFile_Delete(TMgGame_DynamicInstanceFile_Perm);
	}
	
	TMgGame_LoadedInstanceFiles_Num = 0;
	TMgGame_InstanceFileRefs_Num = 0;
	TMgGame_PrivateData_Num = 0;
}

UUtError
TMrTemplate_CallProc(
	TMtTemplateTag				inTemplateTag,
	TMtTemplateProc_Message		inMessage)
{

	return UUcError_None;
}

UUtBool
TMrLevel_Exists(
	UUtUns16			inLevelNumber)
{
	if(inLevelNumber >= TMcLevels_MaxNum) return UUcFalse;
	
	return TMgGame_ValidLevels[inLevelNumber];
}

UUtError
TMrLevel_Load(
	UUtUns16			inLevelNumber,
	TMtAllowPrivateData	inAllowPrivateData)
{
	UUtError	error;
	static UUtTimerRef timer = NULL;
	
	if (TMgTimeLevelLoad) {
		if (NULL == timer) {
			timer = UUrTimer_Allocate("level load timer", "");
		}

		UUrTimer_Begin(timer);
	}
	
	if(TMgGame_ValidLevels[inLevelNumber] == UUcFalse)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "This level is not valid");
	}
	
	if (TMgGame_DynamicInstanceFile_Temp != NULL) {
		// Reset temporary dynamic memory
		TMiGame_InstanceFile_Dynamic_Reset(TMgGame_DynamicInstanceFile_Temp);	// XXX - Might want to do this at a "real" level load since we support multiple levels loaded simultaneously now
	}
	
	error = TMiGame_InstanceFileRef_LoadLevel(inLevelNumber, inAllowPrivateData);
	UUmError_ReturnOnError(error);
	
	// prepare for memory
	TMiGame_LoadedInstanceFiles_PrepareForMemory();
	
	if (TMgTimeLevelLoad) {
		UUrTimer_End(timer);

		UUrTimerSystem_WriteToDisk();
	}
	
	return UUcError_None;
}

void
TMrLevel_Unload(
	UUtUns16	inLevelNumber)
{
	TMiGame_LoadedInstanceFiles_Remove(inLevelNumber);
}
	
UUtError
TMrInstance_GetDataPtr(
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName,
	void*				*outDataPtr)
{
	UUtError	error;
	
	UUmAssertReadPtr(outDataPtr, sizeof(void*));
	UUmAssertReadPtr(inInstanceName, 1);
	UUmAssert(strlen(inInstanceName) < TMcInstanceName_MaxLength);
	
	*outDataPtr = TMrInstance_GetFromName(inTemplateTag, inInstanceName);
	
	error = (*outDataPtr != NULL) ? UUcError_None : UUcError_Generic;

	return error;
}

void *TMrInstance_GetFromName(TMtTemplateTag inTemplateTag, const char *inInstanceName)
{
	UUtUns16	itr;
	void		*dataPtr = NULL;
	
	UUmAssertReadPtr(inInstanceName, 1);
	UUmAssert(strlen(inInstanceName) < TMcInstanceName_MaxLength);

	for(itr = TMcGame_InstanceFile_Dynamic_Num; itr < TMgGame_LoadedInstanceFiles_Num; itr++)
	{
		dataPtr = TMiGame_InstanceFile_GetDataPtr(TMgGame_LoadedInstanceFiles_List[itr], inTemplateTag, inInstanceName);

		if (dataPtr != NULL) {
			break;
		}
	}
	
	return dataPtr;
}

typedef UUtBool
(*TMtDataPtr_Loop_Callback)(
	void *inDataPtr, void *inRefCon);

static void TMrInstance_DataPtr_Loop(TMtTemplateTag	inTemplateTag, void *inRefCon, TMtDataPtr_Loop_Callback inCallback)
{
	UUtUns32			itrNameDesc;
	TMtNameDescriptor*	curNameDesc;
	char				c0, c1, c2, c3;
	void*				dataPtr;
	UUtUns16			itrInstanceFile;
	TMtInstanceFile*	curInstanceFile;
	
	c0 = (char)((inTemplateTag >> 24) & 0xFF);
	c1 = (char)((inTemplateTag >> 16) & 0xFF);
	c2 = (char)((inTemplateTag >> 8) & 0xFF);
	c3 = (char)((inTemplateTag >> 0) & 0xFF);
	
	for(itrInstanceFile = 0; itrInstanceFile < TMgGame_LoadedInstanceFiles_Num; itrInstanceFile++)
	{
		curInstanceFile = TMgGame_LoadedInstanceFiles_List[itrInstanceFile];
		
		for(itrNameDesc = 0, curNameDesc = curInstanceFile->nameDescriptors;
			itrNameDesc < curInstanceFile->numNameDescriptors;
			itrNameDesc++, curNameDesc++)
		{
			UUtBool continue_looping;

			if(curNameDesc->namePtr[0] != c0 ||
				curNameDesc->namePtr[1] != c1 ||
				curNameDesc->namePtr[2] != c2 ||
				curNameDesc->namePtr[3] != c3) continue;

			dataPtr = (curInstanceFile->instanceDescriptors + curNameDesc->instanceDescIndex)->dataPtr;

			if (NULL == dataPtr) { continue; }	

			continue_looping = inCallback(dataPtr, inRefCon);

			if (!continue_looping) { return; }
		}
	}

	return;
}


typedef struct TMtInstance_GetDataPtr_List_ParamaterBlock
{
	UUtUns32 inMaxPtrs;
	UUtUns32 numPtrs;
	void	 **outPtrList;
} TMtInstance_GetDataPtr_List_ParamaterBlock;

static UUtBool TMrInstance_GetDataPtr_List_Loop_Callback(void *inDataPtr, void *inRefCon)
{
	TMtInstance_GetDataPtr_List_ParamaterBlock *param_block = inRefCon;
	UUtBool continue_looping;

	param_block->outPtrList[param_block->numPtrs] = inDataPtr;
	param_block->numPtrs += 1;

	continue_looping = param_block->numPtrs < param_block->inMaxPtrs;

	return continue_looping;
}

UUtError
TMrInstance_GetDataPtr_List(
	TMtTemplateTag		inTemplateTag,
	UUtUns32			inMaxPtrs,
	UUtUns32			*outNumPtrs,
	void*				*outPtrList)
{
	if (0 == inMaxPtrs) {
		*outNumPtrs = 0;
	}
	else {
		TMtInstance_GetDataPtr_List_ParamaterBlock param_block;

		*outPtrList = 0;

		param_block.inMaxPtrs = inMaxPtrs;
		param_block.numPtrs = 0;
		param_block.outPtrList = outPtrList;

		TMrInstance_DataPtr_Loop(inTemplateTag, &param_block, TMrInstance_GetDataPtr_List_Loop_Callback);

		*outNumPtrs = param_block.numPtrs;
	}

	return UUcError_None;
}

typedef struct TMrInstance_GetDataPtr_ByNumber_ParamBlock
{
	UUtUns32 current_number;
	UUtUns32 inNumber;
	void	 *dataPtr;
} TMrInstance_GetDataPtr_ByNumber_ParamBlock;

static UUtBool TMrInstance_GetDataPtr_ByNumber_Loop_Callback(void *inDataPtr, void *inRefCon)
{
	UUtBool continue_looping = UUcTrue;
	TMrInstance_GetDataPtr_ByNumber_ParamBlock *param_block = inRefCon;
	UUtUns32 current_number = param_block->current_number;

	if (param_block->current_number == param_block->inNumber) {
		param_block->dataPtr = inDataPtr;
		continue_looping = UUcFalse;
	}
	else {
		param_block->current_number = current_number + 1;
	}

	return continue_looping;
}

UUtError
TMrInstance_GetDataPtr_ByNumber(
	TMtTemplateTag		inTemplateTag,
	UUtUns32			inNumber,
	void*				*outDataPtr)
{
	TMrInstance_GetDataPtr_ByNumber_ParamBlock param_block;
	UUtError error;

	param_block.current_number = 0;
	param_block.inNumber = inNumber;
	param_block.dataPtr = NULL;

	TMrInstance_DataPtr_Loop(inTemplateTag, &param_block, TMrInstance_GetDataPtr_ByNumber_Loop_Callback);

	if (NULL == param_block.dataPtr)  {
		error = UUcError_Generic;
	}
	else {
		*outDataPtr = param_block.dataPtr;
		error = UUcError_None;
	}

	return error;
}

static UUtBool TMrInstance_GetTagCount_Loop_Callback(void *inDataPtr, void *inRefCon)
{
	UUtUns32 *count = inRefCon;
	UUtBool continue_looping = UUcTrue;

	(*count) = (*count) + 1;

	return continue_looping;
}

UUtUns32
TMrInstance_GetTagCount(
	TMtTemplateTag		inTemplateTag)
{
	UUtUns32 tag_count = 0;

	TMrInstance_DataPtr_Loop(inTemplateTag, &tag_count, TMrInstance_GetTagCount_Loop_Callback);

	return tag_count;
}

UUtError
TMrInstance_Dynamic_New(
	TMtDynamicPool_Type		inDynamicPoolType,
	TMtTemplateTag			inTemplateTag,
	UUtUns32				inInitialVarArrayLength,
	void*					*outDataPtr)
{
	UUtError			error;
	TMtInstanceFile*	targetInstanceFile = NULL;
	

	if(inDynamicPoolType == TMcDynamicPool_Type_Temporary)
	{
		targetInstanceFile = TMgGame_DynamicInstanceFile_Temp;
	}
	else if(inDynamicPoolType == TMcDynamicPool_Type_Permanent)
	{
		targetInstanceFile = TMgGame_DynamicInstanceFile_Perm;
	}
	else
	{
		UUmAssert(!"Unknown dynamic pool type");
	}
	
	if (targetInstanceFile == NULL) {
		// we cannot allocate an instance from this pool
		UUmAssert(!"attempted to use deprecated dynamic instance file");
		return UUcError_Generic;
	}

	error = 
		TMiGame_InstanceFile_Instance_Dynamic_New(
			targetInstanceFile,
			inTemplateTag,
			inInitialVarArrayLength,
			outDataPtr);
	UUmError_ReturnOnError(error);
	
	// Call the new callback
	error = 
		TMiGame_Instance_Callback(
			*outDataPtr,
			TMcTemplateProcMessage_NewPostProcess);
	UUmError_ReturnOnError(error);
		
	return UUcError_None;
}

void *TMrInstancePool_Allocate(
	TMtDynamicPool_Type		inDynamicPoolType,
	UUtUns32				inNumBytes)
{
	UUtUns32 dataSize;
	void *newDataPtr;
	TMtInstanceFile*	targetInstanceFile = NULL;

	if(inDynamicPoolType == TMcDynamicPool_Type_Temporary)
	{
		targetInstanceFile = TMgGame_DynamicInstanceFile_Temp;
	}
	else if(inDynamicPoolType == TMcDynamicPool_Type_Permanent)
	{
		targetInstanceFile = TMgGame_DynamicInstanceFile_Perm;
	}
	else
	{
		UUmAssert(!"Unknown dynamic pool type");
	}

	if (targetInstanceFile == NULL) {
		// we cannot allocate an instance from this pool
		UUmAssert(!"attempted to use deprecated dynamic instance file");
		return NULL;
	}

	dataSize = inNumBytes;
	dataSize = (dataSize + UUcProcessor_CacheLineSize_Mask) & ~UUcProcessor_CacheLineSize_Mask;

	targetInstanceFile->dynamicBytesUsed += dataSize;
	newDataPtr = UUrMemory_Pool_Block_New(targetInstanceFile->dynamicPool, dataSize);

	return newDataPtr;
}

UUtError
TMrInstance_Update(
	void*				inDataPtr)
{
	UUtError	error;
	
	UUmAssertReadPtr(inDataPtr, sizeof(void*));

	error = 
		TMiGame_Instance_Callback(
			inDataPtr,
			TMcTemplateProcMessage_Update);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

UUtError
TMrInstance_PrepareForUse(
	void*				inDataPtr)
{
	TMtInstanceFile*	targetInstanceFile;
	UUtUns32			targetInstanceIndex;
	
	UUmAssertReadPtr(inDataPtr, sizeof(void*));
		
	// mark date
	TMiGame_Instance_FileAndIndex_Get(
		inDataPtr,
		&targetInstanceFile,
		&targetInstanceIndex);
		
	return UUcError_None;
}

TMtTemplateTag
TMrInstance_GetTemplateTag(
	const void*	inDataPtr)
{
	TMtInstanceFile*	targetInstanceFile;
	UUtUns32			targetInstanceIndex;
	
	TMiGame_Instance_FileAndIndex_Get(
		inDataPtr,
		&targetInstanceFile,
		&targetInstanceIndex);

	return targetInstanceFile->instanceDescriptors[targetInstanceIndex].templatePtr->tag;
}

char*
TMrInstance_GetInstanceName(
	const void*	inDataPtr)
{
	TMtInstanceFile*	targetInstanceFile;
	UUtUns32			targetInstanceIndex;
	char				*namePtr;
	char				*result;

	if (NULL  == inDataPtr) {
		return NULL;
	}
	
	TMiGame_Instance_FileAndIndex_Get(
		inDataPtr,
		&targetInstanceFile,
		&targetInstanceIndex);

	namePtr = targetInstanceFile->instanceDescriptors[targetInstanceIndex].namePtr;
	result = (NULL == namePtr) ? NULL : (namePtr + 4);

	return result;
}

void*
TMrInstance_GetRawOffset(
	const void*	inDataPtr)
{
	TMtInstanceFile*	targetInstanceFile;
	
	TMiGame_Instance_File_Get(
		inDataPtr,
		&targetInstanceFile);

	return targetInstanceFile->rawPtr;
}

BFtFile*
TMrInstance_GetSeparateFile(
	const void*	inDataPtr)
{
	TMtInstanceFile*	targetInstanceFile;
	
	TMiGame_Instance_File_Get(
		inDataPtr,
		&targetInstanceFile);

	return targetInstanceFile->separateFile;
}

UUtError
TMrMameAndDestroy(
	void)
{

	return UUcError_None;
}

UUtError
TMrTemplate_PrivateData_New(
	TMtTemplateTag				inTemplateTag,
	UUtUns32					inDataSize,
	TMtTemplateProc_Handler		inProcHandler,
	TMtPrivateData*				*outPrivateData)
{
	UUtError		error;
	UUtUns16		newPrivateDataIndex;
	TMtPrivateData*	newPrivateData;
	
	*outPrivateData = NULL;
	
	UUmAssert(inProcHandler != NULL);
	
	if(TMgGame_PrivateData_Num >= TMcPrivateData_Max)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Too many private datas");
	}
	
	newPrivateDataIndex = TMgGame_PrivateData_Num++;
	newPrivateData = TMgGame_PrivateData_List + newPrivateDataIndex;
	
	newPrivateData->templateTag = inTemplateTag;
	newPrivateData->dataSize = inDataSize;
	newPrivateData->procHandler = inProcHandler;
	
	error = TMiGame_LoadedInstanceFiles_PrivateData_Add(newPrivateData);
	UUmError_ReturnOnError(error);
	
	error = TMiGame_LoadedInstanceFiles_PrivateData_Callback(newPrivateData, TMcTemplateProcMessage_LoadPostProcess);
	UUmError_ReturnOnError(error);
	
	*outPrivateData = newPrivateData;
	
	return UUcError_None;
}

void
TMrTemplate_PrivateData_Delete(
	TMtPrivateData*				inPrivateData)
{
	UUtInt16	targetPrivateDataIndex;
	
	targetPrivateDataIndex = inPrivateData - TMgGame_PrivateData_List;
	
	UUmAssert(targetPrivateDataIndex >= 0);
	if(targetPrivateDataIndex >= TMgGame_PrivateData_Num) return;
	
	TMiGame_LoadedInstanceFiles_PrivateData_Callback(inPrivateData, TMcTemplateProcMessage_DisposePreProcess);

	TMiGame_LoadedInstanceFiles_PrivateData_Remove(inPrivateData);
	
	UUrMemory_ArrayElement_Delete(
		TMgGame_PrivateData_List,
		targetPrivateDataIndex,
		TMgGame_PrivateData_Num,
		sizeof(TMtPrivateData));
	
	TMgGame_PrivateData_Num--;
	
}

void*
TMrTemplate_PrivateData_GetDataPtr(
	TMtPrivateData*				inPrivateData,
	const void*					inInstancePtr)
{
	TMtInstanceFile*				targetInstanceFile;
	UUtUns32						targetInstanceIndex;
	UUtUns16						itrPrivateInfo;
	TMtInstanceFile_PrivateInfo*	curPrivateInfo;
	void**							targetInstancePtrList;
	
	if(inInstancePtr == NULL) return NULL;

	TMiGame_Instance_FileAndIndex_Get(
		inInstancePtr,
		&targetInstanceFile,
		&targetInstanceIndex);
	
	for(itrPrivateInfo = 0, curPrivateInfo = targetInstanceFile->privateInfos;
		itrPrivateInfo < targetInstanceFile->numPrivateInfos;
		itrPrivateInfo++, curPrivateInfo++)
	{
		if(curPrivateInfo->owner == inPrivateData)
		{
			if(inPrivateData->dataSize == 0) return NULL;
			
			UUmAssert(curPrivateInfo->instanceListIndex < targetInstanceFile->numInstancePtrLists);
			targetInstancePtrList = targetInstanceFile->instancePtrLists[curPrivateInfo->instanceListIndex];
			
			return targetInstancePtrList[targetInstanceIndex];
		}
	}
	
	UUmAssert(0);
	
	return NULL;
}
