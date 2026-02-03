/*
	FILE:	BFW_TemplateManager.c

	AUTHOR:	Brent H. Pease

	CREATED: June 8, 1997

	PURPOSE:

	Copyright 1997

	WARNING: Leave this evil place! Go! Run! Do not look back!

	However, if you stay there is a moral to be learned. Never try to rewrite something really
	complex in a day.
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

TMtTemplateDefinition*	gTemplateDefinitionsAlloced = NULL;
UUtMemory_String*		gTemplateNameAlloced = NULL;

#define	TMmBrentSucks	1

#define TMcPreDataBuffer	32	// This is the space used for bookkeeping by the template manager

#define TMcMaxLevelDatFiles	(128)

#define TMcInstanceFile_Version				UUm4CharToUns32('V', 'R', '2', '5')

#define TMcMaxNumInstances	256
#define TMcMaxInstancesPerTemplate	(32 * 1024)

#define TMcMaxInstances	(256 * 256 * 256)

/*
 * Place holder macros
 */

#if defined(DEBUGGING) && DEBUGGING

	#define TMmPlaceHolder_GetIndex(ph)			(TMrPlaceHolder_GetIndex(ph))
	#define TMmPlaceHolder_MakeFromIndex(i)		(TMrPlaceHolder_MakeFromIndex(i))

#else

	#define TMmPlaceHolder_GetIndex(ph)			(UUtUns32)(ph >> 8)
	#define TMmPlaceHolder_MakeFromIndex(i)		((i << 8) | (0x0001))

#endif

/*
 * pre instance data memory map

 -1: Place holder
 -2: Ptr to instance file
 -3: Ptr to dynamic data <- To go away
 -4: Magic Cookie
 -5: Priority
 -6: Time stamp
 -7: Template definition
*/

/*
 * Get the instance file for this instance
 */
	#define TMmInstanceData_GetInstanceFile(instanceDataPtr) \
		(*((TMtInstanceFile**)instanceDataPtr - 2))

/*
 * Get the place holder of this instance
 */
	#define TMmInstanceData_GetPlaceHolder(instanceDataPtr) \
		(*((TMtPlaceHolder *)instanceDataPtr - 1))

/*
 * Get the magic cookie of this instance
 */
	#define TMmInstanceData_GetMagicCookie(instanceDataPtr) \
		(*((UUtUns32 *)instanceDataPtr - 4))

/*
 * Get the template definition
 */
	#define TMmInstanceData_GetTemplateDefinition(instanceDataPtr) \
		(*((TMtTemplateDefinition **)instanceDataPtr - 7))

#if defined(DEBUGGING) && DEBUGGING

	#define TMmInstanceDesc_Verify(instanceDesc) \
		UUmAssert(instanceDesc->templatePtr->checksum == instanceDesc->checksum)

	#define TMmInstanceFile_Verify(instanceFile) \
		UUmAssert(instanceFile->magicCookie == TMcMagicCookie)

	#define TMmTemplatePtr_Verify(templatePtr) \
		UUmAssert(templatePtr->magicCookie == TMcMagicCookie)

	#define TMmInstanceData_Verify(dataPtr) \
		UUmAssert(NULL != ((void *) dataPtr));			\
		UUmAssertAlignedPtr(dataPtr);	\
		UUmAssert(TMmInstanceData_GetTemplateDefinition(dataPtr)->magicCookie == TMcMagicCookie);	\
		UUmAssert(TMmInstanceData_GetMagicCookie(dataPtr) == TMcMagicCookie)

	#define TMmInstanceFile_VerifyAllTouched(instanceFile) TMiInstanceFile_VerifyAllTouched(instanceFile)

#else

	#define TMmInstanceDesc_Verify(instanceDataPtr)
	#define TMmInstanceFile_Verify(instanceFile)
	#define TMmTemplatePtr_Verify(templatePtr)
	#define TMmInstanceData_Verify(dataPtr)
	#define TMmInstanceFile_VerifyAllTouched(instanceFile)

#endif

#if defined(DEBUGGING) && DEBUGGING && !(defined(TMmBrentSucks) && TMmBrentSucks)

	#define TMmInstanceFile_MajorCheck(instanceFile, absolutePlaceHolder) TMiInstanceFile_MajorCheck(instanceFile, absolutePlaceHolder)

#else

	#define TMmInstanceFile_MajorCheck(instanceFile, absolutePlaceHolder)

#endif

/*
 * Descriptor flags
 */
	typedef enum TMtDescriptorFlags
	{
		TMcDescriptorFlags_None				= 0,
		TMcDescriptorFlags_Unique			= (1 << 0),
		TMcDescriptorFlags_PlaceHolder		= (1 << 1),
		TMcDescriptorFlags_Duplicate		= (1 << 2),			// This instance does not point to its own data - it points to its duplicates
		TMcDescriptorFlags_DuplicatedSrc	= (1 << 3),			// This instance is being used by duplicate instances as a source

		/* These are not saved */
		TMcDescriptorFlags_Touched			= (1 << 20),
		TMcDescriptorFlags_InBatchFile		= (1 << 21),
		TMcDescriptorFlags_DeleteMe			= (1 << 22)

	} TMtDescriptorFlags;

	#define TMcFlags_PersistentMask	(0xFFFF)

/*
 * Template related build data
 */

typedef struct TMtTemplate_ConstructionData
{
	UUtUns32	numInstancesUsed;
	UUtUns32	totalSizeOfAllInstances;

	UUtUns32	nextInstanceIndex;
	UUtUns32	instanceIndexList[TMcMaxInstancesPerTemplate];

	UUtUns32	numInstancesRemoved;
	UUtUns32	removedSize;

} TMtTemplate_ConstructionData;


/*
 *
 */
	typedef struct TMtTemplateDescriptor
	{
		UUtUns64		checksum;		// The checksum at the time instance file was created
		TMtTemplateTag	tag;			// The tag of the template
		UUtUns32		numUsed;		// The number of instances that use this template

	} TMtTemplateDescriptor;

/*
 * This is the instance descriptor structure that is saved to disk
 */
	typedef struct TMtInstanceDescriptor
	{
		UUtUns64				checksum;		// This is the template checksum at the time the instance is written
		TMtTemplateDefinition*	templatePtr;
		void*					dataPtr;
		char*					namePtr;
		UUtUns32				size;			// This is the total size including entire var array that is written to disk
		TMtDescriptorFlags		flags;
		UUtUns32				creationDate;	// in seconds since 1900

	} TMtInstanceDescriptor;

	typedef struct TMtNameDescriptor
	{
		UUtUns32	instanceDescIndex;
		char*		namePtr;

		char		pad[24];

	} TMtNameDescriptor;

/*
 * This is the instance file
 */
	typedef struct TMtInstanceFile
	{
		char					fileName[BFcMaxFileNameLength];

		BFtFileRef*				instanceFileRef;

		UUtUns32				numInstanceDescriptors;
		TMtInstanceDescriptor*	instanceDescriptors;

		UUtUns32				numNameDescriptors;
		TMtNameDescriptor*		nameDescriptors;		// Used for binary search

		UUtUns32				numTemplateDescriptors;
		TMtTemplateDescriptor*	templateDescriptors;

		void*					allocBlock;				// The pointer which is used to alloc entire disk data
		BFtFileMapping*			mapping;				// if file mapping is used

		UUtUns32				dataBlockLength;
		void*					dataBlock;

		UUtUns32				nameBlockLength;
		char*					nameBlock;

		void*					dynamicMemory;

		UUtBool					final;

		#if defined(DEBUGGING) && DEBUGGING

			UUtUns32			magicCookie;

		#endif

	} TMtInstanceFile;

/*
 * Instance file header
 */
typedef struct TMtInstanceFile_Header
{
	UUtUns32		version;
	UUtUns32		pad1;

	UUtUns64		totalTemplateChecksum;

	UUtUns32		numInstanceDescriptors;
	UUtUns32		numNameDescriptors;
	UUtUns32		numTemplateDescriptors;

	UUtUns32		dataBlockOffset;
	UUtUns32		dataBlockLength;

	UUtUns32		nameBlockOffset;
	UUtUns32		nameBlockLength;

	UUtUns32		pad2[5];

} TMtInstanceFile_Header;

/*
 *
 */
	typedef struct TMtCache_Entry
	{
		void*		instance;
		UUtUns32	cachedSize;
		UUtUns32	offset;

	} TMtCache_Entry;

	struct TMtCache
	{
		TMtTemplateDefinition*		templatePtr;

		UUtUns32					maxNumEntries;
		UUtUns32					memoryPoolSize;

		TMtCacheProc_Load			loadProc;
		TMtCacheProc_Unload			unloadProc;
		TMtCacheProc_ComputeSize	computeSizeProc;

		void**						entryList;
		UUtMemory_Pool*				memoryPool;

	};

	struct TMtPrivateData
	{
		UUtUns32			dataSize;
		UUtMemory_Pool*		memoryPool;
	};

/*
 * TMgTemplateData points to the template definition data loaded from template.dat
 * TMgTemplateNameData points to the template name data loaded from template.nam
 */
	UUtUns8*				TMgTemplateData = NULL;
	char*					TMgTemplateNameData = NULL;

	UUtBool					gInGame;

/*
 * Globals used during construction
 */
	TMtInstanceFile*				gConstructionFile = NULL;
	UUtMemory_Array*				gInstanceDescriptorArray = NULL;
	UUtMemory_Array*				gNameDescriptorArray = NULL;
	UUtMemory_Pool*					gConstructionPool = NULL;
	UUtUns32*						gInstanceDuplicateArray = NULL;			// parallel to instance descriptors - if not UUcMaxUns32 then its the index of the duplicate instance
/*
 * TMgTemplateDefinitions is an array of TMtTemplateDefinition structures that hold the
 * template definition data
 */

/*
 * File reference to the data directory
 */
	BFtFileRef*	TMgDataFolderRef;

/*
 * Level0 data files
 */
	UUtUns16				TMgNumLevel0Files = 0;
	TMtInstanceFile*		TMgLevel0Files[TMcMaxLevelDatFiles];

/*
 * Cur level data files
 */
	UUtUns16				TMgNumCurLevelFiles = 0;
	TMtInstanceFile*		TMgCurLevelFiles[TMcMaxLevelDatFiles];

	UUtUns16				TMgLevelLoadStack = 0;

/*
 * Memory for dynamic instances
 */
	UUtMemory_Pool*			TMgPermPool = NULL;
	UUtMemory_Pool*			TMgTempPool = NULL;

	TMtInstanceFile			TMgPermInstanceFile;
	TMtInstanceFile			TMgTempInstanceFile;
	UUtMemory_Array*		TMgPermDescriptorArray = NULL;
	UUtMemory_Array*		TMgTempDescriptorArray = NULL;

	#define TMcPermPoolChunkSize	(10 * 1024 * 1024)
	#define TMcTempPoolChunkSize	(15 * 1024 * 1024)

	#define TMcConstructionPoolChunkSize	(10 * 1024 * 1024)

extern UUtUns64	gTemplateChecksum;

static UUtError
TMiInstance_PrepareForMemory(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile,
	char*				inMsg,
	UUtBool				inIsVarArrayLeaf);

static UUtError
TMiInstance_ByteSwap(
	UUtUns8*					*ioSwapCode,
	UUtUns8*					*ioDataPtr,
	TMtTemplateProc_ByteSwap	inByteSwapProc);

/*
 * This is used to skip over an empty variable array
 */
static void
TMiSkipVarArray(
	UUtUns8*		*ioSwapCode)
{

	UUtBool		stop;
	UUtUns8*	curSwapCode;
	char		swapCode;

	curSwapCode = *ioSwapCode;

	stop = UUcFalse;

	while(!stop)
	{
		swapCode = *curSwapCode++;

		switch(swapCode)
		{
			case TMcSwapCode_8Byte:
				break;

			case TMcSwapCode_4Byte:
				break;

			case TMcSwapCode_2Byte:
				break;

			case TMcSwapCode_1Byte:
				break;

			case TMcSwapCode_BeginArray:
				curSwapCode++;

				TMiSkipVarArray(&curSwapCode);
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				curSwapCode++;

				TMiSkipVarArray(
					&curSwapCode);
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
				curSwapCode += 4;
				break;

			case TMcSwapCode_TemplateReference:
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
}

#if defined(DEBUGGING) && DEBUGGING

static UUtUns32
TMrPlaceHolder_GetIndex(
	TMtPlaceHolder	inPlaceHolder)
{
	UUtUns32	result;

	if(gInGame == UUcFalse)
	{
		UUmAssert(gConstructionFile != NULL);

		UUmAssert(!TMmPlaceHolder_IsPtr(inPlaceHolder));
	}

	result = (UUtUns32)(inPlaceHolder >> 8);

	//UUmAssert(result < gConstructionFile->numInstanceDescriptors);

	return result;
}

static TMtPlaceHolder
TMrPlaceHolder_MakeFromIndex(
	UUtUns32	inIndex)
{
	if(gInGame == UUcFalse)
	{
		UUmAssert(gConstructionFile != NULL);
		UUmAssert(inIndex < gConstructionFile->numInstanceDescriptors);
	}

	return (inIndex << 8) | (0x0001);
}

#endif

static TMtInstanceDescriptor*
TMrPlaceHolder_GetInstanceDesc(
	TMtInstanceFile*	inInstanceFile,
	TMtPlaceHolder		inPlaceHolder)
{
	TMtInstanceDescriptor*	instanceDesc;
	TMtPlaceHolder			placeHolder;
	UUtUns32				index;

	TMmInstanceFile_Verify(inInstanceFile);

	if(TMmPlaceHolder_IsPtr(inPlaceHolder))
	{
		TMmInstanceData_Verify(inPlaceHolder);

		placeHolder = TMmInstanceData_GetPlaceHolder(inPlaceHolder);

		UUmAssert(!TMmPlaceHolder_IsPtr(placeHolder));

		index = TMmPlaceHolder_GetIndex(placeHolder);

		UUmAssert(index < inInstanceFile->numInstanceDescriptors);

		instanceDesc = inInstanceFile->instanceDescriptors + index;
	}
	else
	{
		index = TMmPlaceHolder_GetIndex(inPlaceHolder);

		UUmAssert(index < inInstanceFile->numInstanceDescriptors);

		instanceDesc = inInstanceFile->instanceDescriptors + index;
	}

	TMmInstanceDesc_Verify(instanceDesc);

	return instanceDesc;
}

static void
TMrPlaceHolder_EnsureIndex(
	TMtPlaceHolder	*ioPlaceHolder)
{

	if(*ioPlaceHolder == 0) return;

	if(TMmPlaceHolder_IsPtr(*ioPlaceHolder))
	{
		*ioPlaceHolder = TMmInstanceData_GetPlaceHolder(*ioPlaceHolder);
	}
}

#if defined(DEBUGGING) && DEBUGGING

static void
TMiInstance_MajorCheck_Recursive(
	UUtUns8*				*ioSwapCode,
	UUtUns8*				*ioDataPtr,
	TMtInstanceFile*		inInstanceFile,
	TMtInstanceDescriptor*	inTargetDesc,
	char*					inMsg,
	UUtUns8*				inBaseDataPtr,
	UUtBool					inAbsolutePlaceHolder)
{
	UUtBool		stop;
	UUtUns8		*curSwapCode;
	UUtUns8		swapCode;
	UUtUns8		*curDataPtr;
	UUtUns32	i;
	UUtUns32	count;
	UUtUns8		*origSwapCode;
	char		msg[2048];

	TMmInstanceFile_Verify(inInstanceFile);

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

				count = *curSwapCode++;

				origSwapCode = curSwapCode;

				UUmAssert(count < 10000);	// Arbitrary number

				for(i = 0; i < count; i++)
				{
					curSwapCode = origSwapCode;

					TMiInstance_MajorCheck_Recursive(
						&curSwapCode,
						&curDataPtr,
						inInstanceFile,
						inTargetDesc,
						inMsg,
						inBaseDataPtr,
						inAbsolutePlaceHolder);

				}
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				switch(*curSwapCode++)
				{
					case TMcSwapCode_8Byte:
						count = (UUtUns32) *(UUtUns64 *)curDataPtr;
						curDataPtr += 8;
						break;

					case TMcSwapCode_4Byte:
						count = *(UUtUns32 *)curDataPtr;
						curDataPtr += 4;
						break;

					case TMcSwapCode_2Byte:
						count = *(UUtUns16 *)curDataPtr;
						curDataPtr += 2;
						break;

					default:
						UUmDebugStr("swap codes are messed up.");
				}

				if(count == 0)
				{
					TMiSkipVarArray(&curSwapCode);
				}
				else
				{
					origSwapCode = curSwapCode;

					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;

						TMiInstance_MajorCheck_Recursive(
							&curSwapCode,
							&curDataPtr,
							inInstanceFile,
							inTargetDesc,
							inMsg,
							inBaseDataPtr,
							inAbsolutePlaceHolder);
					}
				}
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
			case TMcSwapCode_TemplateReference:
				{
					TMtInstanceDescriptor	*targetDesc;
					void					*targetData;

					targetData = *(void **)curDataPtr;

					if(targetData != NULL)
					{
						targetDesc = TMrPlaceHolder_GetInstanceDesc(inInstanceFile, (TMtPlaceHolder)targetData);
						UUmAssert(targetDesc != NULL);
						TMmInstanceDesc_Verify(targetDesc);

						if(inAbsolutePlaceHolder == UUcTrue)
						{
							UUmAssert(!TMmPlaceHolder_IsPtr(targetData));
						}

						if(targetDesc->flags & TMcDescriptorFlags_DeleteMe)
						{
							UUmAssert(targetDesc->dataPtr == NULL);
						}

						if(targetDesc->dataPtr != NULL && TMmPlaceHolder_IsPtr(targetData))
						{
							UUmAssert(!(targetDesc->flags & TMcDescriptorFlags_DeleteMe));
							UUmAssert(targetDesc->dataPtr == targetData);
						}

						#if defined(DEBUGGING) && DEBUGGING

							if(swapCode == TMcSwapCode_TemplatePtr &&
								*(TMtTemplateTag*)curSwapCode != targetDesc->templatePtr->tag)
							{
								char	expectedTag[5];
								char	actualTag[5];

								UUrString_Copy(expectedTag, UUrTag2Str(*(long*)curSwapCode), 5);
								UUrString_Copy(actualTag, UUrTag2Str(targetDesc->templatePtr->tag), 5);

								sprintf(
									msg,
									"%s: Expecting template tag %s at offset %d, instead got tag %s, name %s",
									inMsg,
									expectedTag,
									curDataPtr - inBaseDataPtr,
									actualTag,
									targetDesc->namePtr ? targetDesc->namePtr + 4 : "no name");
								UUrError_Report(UUcError_Generic, msg);
							}

						#endif

						sprintf(msg, "%s -> Template: %s, name: %s",
							inMsg,
							UUrTag2Str(targetDesc->templatePtr->tag),
							targetDesc->namePtr ? targetDesc->namePtr + 4 : "no name");

						if(!(targetDesc->flags & (TMcDescriptorFlags_PlaceHolder | TMcDescriptorFlags_DeleteMe)))
						{
							UUtUns8*	swapCode;
							UUtUns8*	dataPtr;

							UUmAssertAlignedPtr(targetDesc->dataPtr);

							swapCode = targetDesc->templatePtr->swapCodes;
							dataPtr = targetDesc->dataPtr;

							TMmInstanceData_Verify(targetDesc->dataPtr);

							TMiInstance_MajorCheck_Recursive(
								&swapCode,
								&dataPtr,
								inInstanceFile,
								targetDesc,
								msg,
								dataPtr,
								inAbsolutePlaceHolder);
						}
					}
					if(swapCode == TMcSwapCode_TemplatePtr)
					{
						curSwapCode += 4;
						curDataPtr += 4;
					}
					else
					{
						curDataPtr += 8;
					}
				}
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;

}

static void
TMiInstanceFile_MajorCheck(
	TMtInstanceFile*	inInstanceFile,
	UUtBool				inAbsolutePlaceHolder)
{
	UUtUns32				curDescIndex;

	TMtInstanceDescriptor*	curInstDesc;
	TMtNameDescriptor*		curNameDesc;

	TMtPlaceHolder			curPlaceHolder;
	UUtUns32				curIndex;

	UUtUns8*				swapCodes;
	UUtUns8*				dataPtr;

	char					msg[2048];

	for(
		curDescIndex = 0, curInstDesc = inInstanceFile->instanceDescriptors;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curInstDesc++)
	{
		if(curInstDesc->namePtr == NULL)
		{
			UUmAssert(curInstDesc->flags & TMcDescriptorFlags_Unique);
		}

		if(curInstDesc->flags & TMcDescriptorFlags_Unique)
		{
			UUmAssert(curInstDesc->namePtr == NULL);
		}

		if(curInstDesc->flags & TMcDescriptorFlags_PlaceHolder)
		{
			UUmAssert(curInstDesc->dataPtr == NULL);
		}

		if(curInstDesc->dataPtr == NULL)
		{
			UUmAssert(curInstDesc->flags & (TMcDescriptorFlags_PlaceHolder | TMcDescriptorFlags_DeleteMe));
			continue;
		}

		//UUmAssert(!((curInstDesc->flags & TMcDescriptorFlags_Touched) && (curInstDesc->flags & TMcDescriptorFlags_DeleteMe)));

		TMmInstanceData_Verify(curInstDesc->dataPtr);

		curPlaceHolder = TMmInstanceData_GetPlaceHolder(curInstDesc->dataPtr);
		UUmAssert(!TMmPlaceHolder_IsPtr(curPlaceHolder));

		curIndex = curPlaceHolder >> 16;

		UUmAssert(curIndex == curDescIndex);
	}

	for(
		curDescIndex = 0, curNameDesc = inInstanceFile->nameDescriptors;
		curDescIndex < inInstanceFile->numNameDescriptors;
		curDescIndex++, curNameDesc++)
	{
		UUmAssert(curNameDesc->namePtr != NULL);
		UUmAssert((inInstanceFile->instanceDescriptors + curNameDesc->instanceDescIndex)->namePtr != NULL);
		UUmAssert(strcmp(curNameDesc->namePtr, (inInstanceFile->instanceDescriptors + curNameDesc->instanceDescIndex)->namePtr) == 0);
	}

	for(
		curDescIndex = 0, curInstDesc = inInstanceFile->instanceDescriptors;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curInstDesc++)
	{
		swapCodes = curInstDesc->templatePtr->swapCodes;
		dataPtr = curInstDesc->dataPtr;

		if(dataPtr == NULL) continue;

		if(curInstDesc->flags & TMcDescriptorFlags_DeleteMe) continue;

		sprintf(msg, "Template: %s, name: %s",
			UUrTag2Str(curInstDesc->templatePtr->tag),
			curInstDesc->namePtr ? curInstDesc->namePtr + 4 : "no name");

		TMiInstance_MajorCheck_Recursive(
			&swapCodes,
			&dataPtr,
			inInstanceFile,
			curInstDesc,
			msg,
			dataPtr,
			inAbsolutePlaceHolder);
	}
}

#endif

TMtTemplateDefinition*
TMiTemplate_FindDefinition(
	TMtTemplateTag		inTemplateTag)
{
	TMtTemplateDefinition*	curTemplate = TMgTemplateDefinitionArray;
	UUtInt32				i;

	for(i = TMgNumTemplateDefinitions; i-- > 0;)
	{
		if(curTemplate->tag == inTemplateTag)
		{
			TMmTemplatePtr_Verify(curTemplate);

			return curTemplate;
		}

		curTemplate++;
	}

	UUrDebuggerMessage(
		"Could not find template %c%c%c%c",
		(inTemplateTag >> 24) & 0xFF,
		(inTemplateTag >> 16) & 0xFF,
		(inTemplateTag >> 8) & 0xFF,
		(inTemplateTag >> 0) & 0xFF);

	return NULL;
}

/*
 * Called for each template definition after it is loaded from disk
 */
static UUtError
TMiTemplate_InitializePostProcess(
	UUtUns8*	*ioSwapCode,
	UUtBool		inNeedsSwapping)
{
	UUtError	error;

	UUtUns8		*curSwapCode;
	UUtBool		stop;
	UUtUns8		count;

	stop = UUcFalse;

	curSwapCode = *ioSwapCode;

	while(!stop)
	{
		switch(*curSwapCode++)
		{
			case TMcSwapCode_8Byte:
			case TMcSwapCode_4Byte:
			case TMcSwapCode_2Byte:
			case TMcSwapCode_1Byte:
				break;

			case TMcSwapCode_BeginArray:
				count = *curSwapCode++;

				error =
					TMiTemplate_InitializePostProcess(
						&curSwapCode,
						inNeedsSwapping);
				UUmError_ReturnOnError(error);
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				{

					switch(*curSwapCode++)
					{
						case TMcSwapCode_8Byte:
							break;

						case TMcSwapCode_4Byte:
							break;

						case TMcSwapCode_2Byte:
							break;

						default:
							UUrError_Report(UUcError_Generic, "illegal swap code");
							return UUcError_Generic;
					}

					error =
						TMiTemplate_InitializePostProcess(
							&curSwapCode,
							inNeedsSwapping);
					UUmError_ReturnOnError(error);

				}
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
				if(inNeedsSwapping)
				{
					UUrSwap_4Byte(curSwapCode);
				}
				curSwapCode += 4;
				break;

			case TMcSwapCode_TemplateReference:
				break;

			default:
				UUrError_Report(UUcError_Generic, "illegal swap code");
				return UUcError_Generic;
		}
	}

	*ioSwapCode = curSwapCode;

	return UUcError_None;
}

/*
 * This is used to convert all pointers to instances to symbol references
 */
static void
TMiInstance_PrepareForDisk(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile,
	UUtBool				inRemap)
{

	UUtBool					stop;
	UUtUns8*				curSwapCode;
	UUtUns8*				curDataPtr;
	char					swapCode;

	UUtUns32				index;

	TMtInstanceDescriptor*	targetDesc;
	TMtPlaceHolder			targetPlaceHolder;

	UUtUns8*				origSwapCode;
	UUtUns32				count;
	UUtUns32				i;

	//UUmAssert(gInGame == UUcFalse);

	TMmInstanceFile_Verify(inInstanceFile);

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
				count = *curSwapCode++;

				origSwapCode = curSwapCode;

				for(i = 0; i < count; i++)
				{
					curSwapCode = origSwapCode;

					TMiInstance_PrepareForDisk(
						&curSwapCode,
						&curDataPtr,
						inInstanceFile,
						inRemap);
				}
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
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
					TMiSkipVarArray(
						&curSwapCode);
				}
				else
				{
					//UUmAssert(count < 160000);	//artifically random number to check against large numbers

					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;

						TMiInstance_PrepareForDisk(
							&curSwapCode,
							&curDataPtr,
							inInstanceFile,
							inRemap);
					}
				}
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplateReference:
			case TMcSwapCode_TemplatePtr:

				targetPlaceHolder = *(TMtPlaceHolder*)curDataPtr;

				if(targetPlaceHolder != 0)
				{
					if(TMmPlaceHolder_IsPtr(targetPlaceHolder))
					{
						TMmInstanceData_Verify((void*)targetPlaceHolder);

						*(TMtPlaceHolder*)curDataPtr = TMmInstanceData_GetPlaceHolder(targetPlaceHolder);
						TMrPlaceHolder_EnsureIndex((TMtPlaceHolder*)curDataPtr);
					}
					else
					{
						index = TMmPlaceHolder_GetIndex(targetPlaceHolder);
						UUmAssert(index < inInstanceFile->numInstanceDescriptors);
					}
				}

				if(targetPlaceHolder != 0)
				{
					targetPlaceHolder = *(TMtPlaceHolder*)curDataPtr;
					targetDesc = TMrPlaceHolder_GetInstanceDesc(inInstanceFile, targetPlaceHolder);
				}

				#if defined(DEBUGGING) && DEBUGGING

					if(targetPlaceHolder != 0)
					{
						UUmAssert(!(targetDesc->flags & TMcDescriptorFlags_DeleteMe));

						if(swapCode == TMcSwapCode_TemplatePtr)
						{
							UUmAssert(targetDesc->templatePtr->tag == *(TMtTemplateTag*)curSwapCode);
						}
					}

				#endif

				if(swapCode == TMcSwapCode_TemplatePtr)
				{
					curSwapCode += 4;
					curDataPtr += 4;
				}
				else
				{
					if(targetPlaceHolder != 0)
					{
						*(TMtTemplateTag *)(curDataPtr + 4) = targetDesc->templatePtr->tag;
					}
					else
					{
						*(TMtTemplateTag *)(curDataPtr + 4) = 0;
					}
					curDataPtr += 8;
				}
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
}

static void
TMiInstance_Remap_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	UUtUns32*			inRemapArray,
	TMtInstanceFile*	inInstanceFile)
{

	UUtBool					stop;
	UUtUns8*				curSwapCode;
	UUtUns8*				curDataPtr;
	char					swapCode;

	TMtPlaceHolder			targetPlaceHolder;
	TMtInstanceDescriptor*	targetInstanceDesc;

	UUtUns8*				origSwapCode;
	UUtUns32				count;
	UUtUns32				i;

	//UUmAssert(gInGame == UUcFalse);

	TMmInstanceFile_Verify(inInstanceFile);

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
				count = *curSwapCode++;

				origSwapCode = curSwapCode;

				for(i = 0; i < count; i++)
				{
					curSwapCode = origSwapCode;

					TMiInstance_Remap_Recursive(
						&curSwapCode,
						&curDataPtr,
						inRemapArray,
						inInstanceFile);
				}
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
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
					TMiSkipVarArray(
						&curSwapCode);
				}
				else
				{
					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;

						TMiInstance_Remap_Recursive(
							&curSwapCode,
							&curDataPtr,
							inRemapArray,
							inInstanceFile);
					}
				}
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplateReference:
			case TMcSwapCode_TemplatePtr:

				targetInstanceDesc = NULL;
				targetPlaceHolder = *(TMtPlaceHolder*)curDataPtr;

				if(targetPlaceHolder != 0)
				{
					if(TMmPlaceHolder_IsPtr(targetPlaceHolder))
					{
						TMrPlaceHolder_EnsureIndex((TMtPlaceHolder*)curDataPtr);	// This maps it to the correct index
						targetPlaceHolder = *(TMtPlaceHolder*)curDataPtr;

						targetInstanceDesc = inInstanceFile->instanceDescriptors + TMmPlaceHolder_GetIndex(targetPlaceHolder);
					}
					else
					{
						//targetPlaceHolder is an index and needs to be remapped
						UUtUns32	originalIndex = TMmPlaceHolder_GetIndex(targetPlaceHolder);
						UUtUns32	remappedIndex = inRemapArray[originalIndex];

						if(remappedIndex != UUcMaxUns32)
						{
							UUmAssert(remappedIndex < inInstanceFile->numInstanceDescriptors);

							*(TMtPlaceHolder*)curDataPtr = TMmPlaceHolder_MakeFromIndex(remappedIndex);
						}
						else
						{
							UUmAssert(0);
						}

						targetInstanceDesc = inInstanceFile->instanceDescriptors + remappedIndex;
					}
				}
				if(swapCode == TMcSwapCode_TemplatePtr)
				{
					if(targetInstanceDesc != NULL)
					{
						UUmAssert(*(TMtTemplateTag*)curSwapCode == targetInstanceDesc->templatePtr->tag);
					}

					curSwapCode += 4;
					curDataPtr += 4;
				}
				else
				{
					curDataPtr += 8;
				}
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
}

static UUtInt8
TMiInstance_Compare_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtrA,
	UUtUns8*			*ioDataPtrB,
	TMtInstanceFile*	inInstanceFile)
{

	UUtBool					stop;
	UUtUns8*				curSwapCode;
	UUtUns8*				curDataPtrA;
	UUtUns8*				curDataPtrB;
	char					swapCode;

	UUtInt8					result;

	UUtUns8*				origSwapCode;
	UUtUns32				count;
	UUtUns32				i;

	TMmInstanceFile_Verify(inInstanceFile);

	curSwapCode = *ioSwapCode;
	curDataPtrA = *ioDataPtrA;
	curDataPtrB = *ioDataPtrB;

	stop = UUcFalse;

	while(!stop)
	{
		swapCode = *curSwapCode++;

		switch(swapCode)
		{
			case TMcSwapCode_8Byte:
				if(*(UUtUns64*)curDataPtrA > *(UUtUns64*)curDataPtrB) return -1;
				if(*(UUtUns64*)curDataPtrA < *(UUtUns64*)curDataPtrB) return 1;
				curDataPtrA += 8;
				curDataPtrB += 8;
				break;

			case TMcSwapCode_4Byte:
				if(*(UUtUns32*)curDataPtrA > *(UUtUns32*)curDataPtrB) return -1;
				if(*(UUtUns32*)curDataPtrA < *(UUtUns32*)curDataPtrB) return 1;
				curDataPtrA += 4;
				curDataPtrB += 4;
				break;

			case TMcSwapCode_2Byte:
				if(*(UUtUns16*)curDataPtrA > *(UUtUns16*)curDataPtrB) return -1;
				if(*(UUtUns16*)curDataPtrA < *(UUtUns16*)curDataPtrB) return 1;
				curDataPtrA += 2;
				curDataPtrB += 2;
				break;

			case TMcSwapCode_1Byte:
				if(*(UUtUns8*)curDataPtrA > *(UUtUns8*)curDataPtrB) return -1;
				if(*(UUtUns8*)curDataPtrA < *(UUtUns8*)curDataPtrB) return 1;
				curDataPtrA += 1;
				curDataPtrB += 1;
				break;

			case TMcSwapCode_BeginArray:
				count = *curSwapCode++;

				origSwapCode = curSwapCode;

				for(i = 0; i < count; i++)
				{
					curSwapCode = origSwapCode;

					result = TMiInstance_Compare_Recursive(
								&curSwapCode,
								&curDataPtrA,
								&curDataPtrB,
								inInstanceFile);
					if(result != 0) return result;
				}
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				switch(*curSwapCode++)
				{
					case TMcSwapCode_8Byte:
						if(*(UUtUns64*)curDataPtrA > *(UUtUns64*)curDataPtrB) return -1;
						if(*(UUtUns64*)curDataPtrA < *(UUtUns64*)curDataPtrB) return 1;
						count = (UUtUns32) (*(UUtUns64 *)curDataPtrA);
						curDataPtrA += 8;
						curDataPtrB += 8;
						break;

					case TMcSwapCode_4Byte:
						if(*(UUtUns32*)curDataPtrA > *(UUtUns32*)curDataPtrB) return -1;
						if(*(UUtUns32*)curDataPtrA < *(UUtUns32*)curDataPtrB) return 1;
						count = *(UUtUns32 *)curDataPtrA;
						curDataPtrA += 4;
						curDataPtrB += 4;
						break;

					case TMcSwapCode_2Byte:
						if(*(UUtUns16*)curDataPtrA > *(UUtUns16*)curDataPtrB) return -1;
						if(*(UUtUns16*)curDataPtrA < *(UUtUns16*)curDataPtrB) return 1;
						count = *(UUtUns16 *)curDataPtrA;
						curDataPtrA += 2;
						curDataPtrB += 2;
						break;

					default:
						UUmDebugStr("swap codes are messed up.");
				}

				origSwapCode = curSwapCode;

				if(count == 0)
				{
					TMiSkipVarArray(
						&curSwapCode);
				}
				else
				{
					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;

						result = TMiInstance_Compare_Recursive(
									&curSwapCode,
									&curDataPtrA,
									&curDataPtrB,
									inInstanceFile);
						if(result != 0) return result;
					}
				}
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplateReference:
			case TMcSwapCode_TemplatePtr:
			{
				TMtPlaceHolder 			targetPlaceHolderA;
				TMtPlaceHolder			targetPlaceHolderB;
				TMtInstanceDescriptor*	targetDescA;
				TMtInstanceDescriptor*	targetDescB;

				targetPlaceHolderA = *(TMtPlaceHolder*)curDataPtrA;
				targetPlaceHolderB = *(TMtPlaceHolder*)curDataPtrB;

				if(targetPlaceHolderA == 0) return 1;
				if(targetPlaceHolderB == 0) return -1;

				targetDescA = TMrPlaceHolder_GetInstanceDesc(inInstanceFile, targetPlaceHolderA);
				targetDescB = TMrPlaceHolder_GetInstanceDesc(inInstanceFile, targetPlaceHolderB);

				if(targetDescA->dataPtr == NULL) return 1;
				if(targetDescB->dataPtr == NULL) return -1;

				if(targetDescA != targetDescB)
				{
					if(targetDescA->size > targetDescB->size) return -1;
					if(targetDescA->size < targetDescB->size) return 1;

					if(targetDescA->templatePtr < targetDescB->templatePtr) return -1;
					if(targetDescA->templatePtr > targetDescB->templatePtr) return 1;

					{
						UUtUns8*	swapCode;
						UUtUns8*	dataPtrA;
						UUtUns8*	dataPtrB;

						swapCode = targetDescA->templatePtr->swapCodes;

						dataPtrA = targetDescA->dataPtr;
						dataPtrB = targetDescB->dataPtr;

						result = TMiInstance_Compare_Recursive(
									&swapCode,
									&dataPtrA,
									&dataPtrB,
									inInstanceFile);
						if(result != 0) return result;
					}
				}

				if(swapCode == TMcSwapCode_TemplatePtr)
				{
					curSwapCode += 4;
					curDataPtrA += 4;
					curDataPtrB += 4;
				}
				else
				{
					curDataPtrA += 8;
					curDataPtrB += 8;
				}
				break;
			}

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtrA = curDataPtrA;
	*ioDataPtrB = curDataPtrB;

	return 0;
}

/*
 * This is used to NULL all pointers to deleted instances
 */
static void
TMiInstance_NullOutDeletedRefs(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile)
{

	UUtBool					stop;
	UUtUns8*				curSwapCode;
	UUtUns8*				curDataPtr;
	char					swapCode;

	TMtInstanceDescriptor*	targetDesc;
	TMtPlaceHolder			targetPlaceHolder;

	UUtUns8*				origSwapCode;
	UUtUns32				count;
	UUtUns32				i;

	UUmAssert(gInGame == UUcFalse);

	TMmInstanceFile_Verify(inInstanceFile);

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
				count = *curSwapCode++;

				origSwapCode = curSwapCode;

				for(i = 0; i < count; i++)
				{
					curSwapCode = origSwapCode;

					TMiInstance_NullOutDeletedRefs(
						&curSwapCode,
						&curDataPtr,
						inInstanceFile);
				}
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
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
					TMiSkipVarArray(
						&curSwapCode);
				}
				else
				{
					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;

						TMiInstance_NullOutDeletedRefs(
							&curSwapCode,
							&curDataPtr,
							inInstanceFile);
					}
				}
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplateReference:
			case TMcSwapCode_TemplatePtr:

				targetPlaceHolder = *(TMtPlaceHolder*)curDataPtr;

				if(targetPlaceHolder != 0)
				{
					targetDesc = TMrPlaceHolder_GetInstanceDesc(inInstanceFile, targetPlaceHolder);

					if(targetDesc->flags & TMcDescriptorFlags_DeleteMe)
					{
						*(void**)curDataPtr = NULL;
					}
				}

				if(swapCode == TMcSwapCode_TemplatePtr)
				{
					curSwapCode += 4;
					curDataPtr += 4;
				}
				else
				{
					curDataPtr += 8;
				}
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
}

static UUtError
TMiInstance_PrepareForMemory_Array(
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
				TMiInstance_PrepareForMemory(
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
TMiInstance_PrepareForMemory_VarArray(
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
		TMiSkipVarArray(&curSwapCode);
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
					TMiInstance_PrepareForMemory(
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


static UUtError
TMiInstance_PrepareForMemory(
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

	UUmAssert(NULL != ioSwapCode);
	UUmAssert(NULL != ioDataPtr);

	TMmInstanceFile_Verify(inInstanceFile);

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
				error = TMiInstance_PrepareForMemory_Array(
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
				error = TMiInstance_PrepareForMemory_VarArray(
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
			case TMcSwapCode_TemplateReference:
				// I think this crash means engine and data are out of sync - Michael
				UUmAssertReadPtr(curDataPtr, sizeof(TMtPlaceHolder));

				targetPlaceHolder = *(TMtPlaceHolder*)curDataPtr;

				if(targetPlaceHolder != 0)
				{
					if(!TMmPlaceHolder_IsPtr(targetPlaceHolder))
					{
						targetDesc = TMrPlaceHolder_GetInstanceDesc(inInstanceFile, targetPlaceHolder);
						UUmAssert(targetDesc != NULL);
						TMmInstanceDesc_Verify(targetDesc);

						if(gInGame == UUcFalse)
						{
							if(!(targetDesc->flags & TMcDescriptorFlags_PlaceHolder))
							{
								TMmInstanceData_Verify(targetDesc->dataPtr);
								*(void **)curDataPtr = targetDesc->dataPtr;
							}
						}
						else
						{
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
								error =
									TMrInstance_GetDataPtr(
										targetDesc->templatePtr->tag,
										targetDesc->namePtr + 4,
										(void **)curDataPtr);
								if(error != UUcError_None)
								{
									sprintf(
										msg,
										"%s: could not find needed data - template %s[%s], name %s",
										inMsg,
										targetDesc->templatePtr->name,
										UUrTag2Str(targetDesc->templatePtr->tag),
										targetDesc->namePtr + 4);
									UUmError_ReturnOnErrorMsg(UUcError_Generic, msg);
								}
							}
							TMmInstanceData_Verify(*(void **)curDataPtr);
						}
					}
					else
					{
						TMmInstanceData_Verify(targetPlaceHolder);
					}
				}

				if(swapCode == TMcSwapCode_TemplatePtr)
				{
					curSwapCode += 4;
					curDataPtr += 4;
				}
				else
				{
					curDataPtr += 8;
				}
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
TMiInstance_ByteSwap_Array(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr)
{
	UUtError	error;
	UUtUns8*	curSwapCode;
	UUtUns8*	curDataPtr;
	UUtUns8		count;
	UUtUns8		i;
	UUtBool		oneElementArray;

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
				TMiInstance_ByteSwap(
					&curSwapCode,
					&curDataPtr,
					NULL);
			UUmError_ReturnOnError(error);
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;

	return UUcError_None;
}

static UUtError
TMiInstance_ByteSwap_VarArray(
	UUtUns8*					*ioSwapCode,
	UUtUns8*					*ioDataPtr,
	TMtTemplateProc_ByteSwap	inByteSwapProc,
	void*						inData)
{
	UUtError	error;
	UUtUns32	count;
	UUtUns32	i;
	UUtUns8*	curSwapCode;
	UUtUns8*	origSwapCode;
	UUtUns8*	curDataPtr;

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
		TMiSkipVarArray(&curSwapCode);
	}
	else if(count == 0)
	{
		TMiSkipVarArray(&curSwapCode);
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
		else if ((*curSwapCode == TMcSwapCode_TemplateReference) && oneElementArray)
		{
			for(i = 0; i < count; i++)
			{
				UUrSwap_4Byte(curDataPtr);
				curDataPtr += 4;
				UUrSwap_4Byte(curDataPtr);
				curDataPtr += 4;
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
					TMiInstance_ByteSwap(
						&curSwapCode,
						&curDataPtr,
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
TMiInstance_ByteSwap(
	UUtUns8*					*ioSwapCode,
	UUtUns8*					*ioDataPtr,
	TMtTemplateProc_ByteSwap	inByteSwapProc)
{
	UUtError				error;
	UUtBool					stop;
	UUtUns8*				curSwapCode;
	UUtUns8*				curDataPtr;
	char					swapCode;

	UUmAssert(NULL != ioSwapCode);
	UUmAssert(NULL != ioDataPtr);

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
					TMiInstance_ByteSwap_Array(
						&curSwapCode,
						&curDataPtr);
				UUmError_ReturnOnError(error);
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				error =
					TMiInstance_ByteSwap_VarArray(
						&curSwapCode,
						&curDataPtr,
						inByteSwapProc,
						*ioDataPtr);
				UUmError_ReturnOnError(error);
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
			case TMcSwapCode_TemplateReference:
				// I think this crash means engine and data are out of sync - Michael
				UUmAssertReadPtr(curDataPtr, sizeof(TMtPlaceHolder));

				if(swapCode == TMcSwapCode_TemplatePtr)
				{
					UUrSwap_4Byte(curDataPtr);
					curSwapCode += 4;
					curDataPtr += 4;
				}
				else
				{
					UUrSwap_4Byte(curDataPtr);
					UUrSwap_4Byte(curDataPtr + 4);
					curDataPtr += 8;
				}
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;

	return UUcError_None;
}

/*
 * This is called to delete all the nested instances that are unique
 */
static void
TMiInstance_Clean_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile)
{
	UUtBool		stop;
	UUtUns8		*curSwapCode;
	UUtUns8		swapCode;
	UUtUns8		*curDataPtr;
	UUtUns32	i;
	UUtUns32	count;
	UUtUns8		*origSwapCode;

	UUmAssert(NULL != ioSwapCode);
	UUmAssert(NULL != ioDataPtr);

	TMmInstanceFile_Verify(inInstanceFile);

	curSwapCode = *ioSwapCode;
	curDataPtr = *ioDataPtr;

	stop = UUcFalse;

	while(!stop)
	{
		swapCode = *curSwapCode++;

		switch(swapCode)
		{
			case TMcSwapCode_8Byte:
				#if defined(DEBUGGING) && DEBUGGING

					*(UUtUns64*)curDataPtr = UUmMemoryBlock_Garbage;

				#endif

				curDataPtr += 8;
				break;

			case TMcSwapCode_4Byte:
				#if defined(DEBUGGING) && DEBUGGING

					*(UUtUns32*)curDataPtr = UUmMemoryBlock_Garbage;

				#endif

				curDataPtr += 4;
				break;

			case TMcSwapCode_2Byte:
				#if defined(DEBUGGING) && DEBUGGING

					*(UUtUns16*)curDataPtr = (UUtUns16)UUmMemoryBlock_Garbage;

				#endif

				curDataPtr += 2;
				break;

			case TMcSwapCode_1Byte:
				#if defined(DEBUGGING) && DEBUGGING

					*(UUtUns8*)curDataPtr = (UUtUns8)UUmMemoryBlock_Garbage;

				#endif

				curDataPtr += 1;
				break;

			case TMcSwapCode_BeginArray:

				count = *curSwapCode++;

				origSwapCode = curSwapCode;

				for(i = 0; i < count; i++)
				{
					curSwapCode = origSwapCode;

					TMiInstance_Clean_Recursive(
						&curSwapCode,
						&curDataPtr,
						inInstanceFile);

				}
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				switch(*curSwapCode++)
				{
					case TMcSwapCode_8Byte:
						count = (UUtUns32) *(UUtUns64 *)curDataPtr;
						curDataPtr += 8;
						break;

					case TMcSwapCode_4Byte:
						count = *(UUtUns32 *)curDataPtr;
						curDataPtr += 4;
						break;

					case TMcSwapCode_2Byte:
						count = *(UUtUns16 *)curDataPtr;
						curDataPtr += 2;
						break;

					default:
						UUmDebugStr("swap codes are messed up.");
				}

				if(count == 0)
				{
					TMiSkipVarArray(&curSwapCode);
				}
				else
				{
					origSwapCode = curSwapCode;

					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;

						TMiInstance_Clean_Recursive(
							&curSwapCode,
							&curDataPtr,
							inInstanceFile);
					}
				}
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
			case TMcSwapCode_TemplateReference:
				{
					TMtInstanceDescriptor	*targetDesc;
					void					*targetData;

					targetData = *(void **)curDataPtr;

					if(targetData != NULL)
					{
						targetDesc = TMrPlaceHolder_GetInstanceDesc(inInstanceFile, (TMtPlaceHolder)targetData);
						TMmInstanceDesc_Verify(targetDesc);

						if((targetDesc->flags & TMcDescriptorFlags_Unique) && targetDesc->dataPtr != NULL)
						{
							UUtUns8*	swapCode;
							UUtUns8*	dataPtr;

							swapCode = targetDesc->templatePtr->swapCodes;
							dataPtr = targetDesc->dataPtr;

							TMiInstance_Clean_Recursive(
								&swapCode,
								&dataPtr,
								inInstanceFile);

							targetDesc->flags |= TMcDescriptorFlags_DeleteMe;
						}
						else if(targetDesc->flags & TMcDescriptorFlags_PlaceHolder)
						{
							targetDesc->flags |= TMcDescriptorFlags_DeleteMe;
						}
						//else
						#if defined(DEBUGGING) && DEBUGGING
						{
							*(UUtUns32*)curDataPtr = UUmMemoryBlock_Garbage;
						}
						#endif
					}
					if(swapCode == TMcSwapCode_TemplatePtr)
					{
						curSwapCode += 4;
						curDataPtr += 4;
					}
					else
					{
						curDataPtr += 8;
					}
				}
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
}

/*
 * This is called to make sure all unique instances are not out of date
 */
static UUtBool
TMiInstance_CheckExists_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile)
{
	UUtBool		stop;
	UUtUns8		*curSwapCode;
	UUtUns8		swapCode;
	UUtUns8		*curDataPtr;
	UUtUns32	i;
	UUtUns32	count;
	UUtUns8		*origSwapCode;
	UUtBool		result;

	UUmAssert(NULL != ioSwapCode);
	UUmAssert(NULL != ioDataPtr);

	TMmInstanceFile_Verify(inInstanceFile);

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

				count = *curSwapCode++;

				origSwapCode = curSwapCode;

				for(i = 0; i < count; i++)
				{
					curSwapCode = origSwapCode;

					result = TMiInstance_CheckExists_Recursive(
						&curSwapCode,
						&curDataPtr,
						inInstanceFile);
					if(result == UUcFalse) return UUcFalse;
				}
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				switch(*curSwapCode++)
				{
					case TMcSwapCode_8Byte:
						count = (UUtUns32) *(UUtUns64 *)curDataPtr;
						curDataPtr += 8;
						break;

					case TMcSwapCode_4Byte:
						count = *(UUtUns32 *)curDataPtr;
						curDataPtr += 4;
						break;

					case TMcSwapCode_2Byte:
						count = *(UUtUns16 *)curDataPtr;
						curDataPtr += 2;
						break;

					default:
						UUmDebugStr("swap codes are messed up.");
				}

				if(count == 0)
				{
					TMiSkipVarArray(&curSwapCode);
				}
				else
				{
					origSwapCode = curSwapCode;

					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;

						result = TMiInstance_CheckExists_Recursive(
							&curSwapCode,
							&curDataPtr,
							inInstanceFile);
						if(result == UUcFalse) return UUcFalse;
					}
				}
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
			case TMcSwapCode_TemplateReference:
				{
					TMtInstanceDescriptor	*targetDesc;
					void					*targetData;

					targetData = *(void **)curDataPtr;

					if(targetData != NULL)
					{
						targetDesc = TMrPlaceHolder_GetInstanceDesc(inInstanceFile, (TMtPlaceHolder)targetData);
						TMmInstanceDesc_Verify(targetDesc);

						if((targetDesc->flags & TMcDescriptorFlags_Unique) && targetDesc->dataPtr == NULL)
						{
							return UUcFalse;
						}

						if(targetDesc->dataPtr != NULL)
						{
							UUtUns8*	swapCode;
							UUtUns8*	dataPtr;

							swapCode = targetDesc->templatePtr->swapCodes;
							dataPtr = targetDesc->dataPtr;

							result = TMiInstance_CheckExists_Recursive(
								&swapCode,
								&dataPtr,
								inInstanceFile);
							if(result == UUcFalse) return UUcFalse;
						}

					}
					if(swapCode == TMcSwapCode_TemplatePtr)
					{
						curSwapCode += 4;
						curDataPtr += 4;
					}
					else
					{
						curDataPtr += 8;
					}
				}
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;

	return UUcTrue;
}

static void
TMiInstance_Touch_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile);

static void
TMiInstance_Touch_Recursive_Array(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile)
{
	UUtUns8 *curSwapCode;
	UUtUns8 *curDataPtr;
	UUtUns8	count;
	UUtUns8 i;
	UUtBool oneTypeArray;

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

			TMiInstance_Touch_Recursive(
				&curSwapCode,
				&curDataPtr,
				inInstanceFile);
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
}

static void
TMiInstance_Touch_Recursive_VarArray(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile)
{
	UUtUns32	count;
	UUtUns32	i;
	UUtUns8*	curSwapCode;
	UUtUns8*	origSwapCode;
	UUtUns8*	curDataPtr;

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
		TMiSkipVarArray(&curSwapCode);
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

				TMiInstance_Touch_Recursive(
					&curSwapCode,
					&curDataPtr,
					inInstanceFile);
			}
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
}

static void
TMiInstance_Touch_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile)
{
	UUtBool		stop;
	UUtUns8		*curSwapCode;
	UUtUns8		swapCode;
	UUtUns8		*curDataPtr;

	TMmInstanceFile_Verify(inInstanceFile);

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
				TMiInstance_Touch_Recursive_Array(
					&curSwapCode,
					&curDataPtr,
					inInstanceFile);
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				TMiInstance_Touch_Recursive_VarArray(
					&curSwapCode,
					&curDataPtr,
					inInstanceFile);
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
			case TMcSwapCode_TemplateReference:
				{
					TMtInstanceDescriptor	*targetDesc;
					void					*targetData;

					targetData = *(void **)curDataPtr;

					if(targetData != NULL)
					{
						targetDesc = TMrPlaceHolder_GetInstanceDesc(inInstanceFile, (TMtPlaceHolder)targetData);
						UUmAssert(targetDesc != NULL);
						TMmInstanceDesc_Verify(targetDesc);

						if(!(targetDesc->flags & TMcDescriptorFlags_Touched))
						{
							targetDesc->flags |= TMcDescriptorFlags_Touched;

							if(targetDesc->flags & TMcDescriptorFlags_PlaceHolder)
							{
								// Make sure a touched place holder does not get deleted
								targetDesc->flags &= (TMtDescriptorFlags)~TMcDescriptorFlags_DeleteMe;
							}
							else
							{
								UUtUns8*	swapCode;
								UUtUns8*	dataPtr;

								swapCode = targetDesc->templatePtr->swapCodes;
								dataPtr = targetDesc->dataPtr;

								TMmInstanceData_Verify(targetDesc->dataPtr);

								TMiInstance_Touch_Recursive(
									&swapCode,
									&dataPtr,
									inInstanceFile);
							}
						}
					}
					if(swapCode == TMcSwapCode_TemplatePtr)
					{
						curSwapCode += 4;
						curDataPtr += 4;
					}
					else
					{
						curDataPtr += 8;
					}
				}
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;

}

static UUtUns32
TMiInstance_ComputeSize_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	TMtInstanceFile*	inInstanceFile)
{
	UUtBool		stop;
	UUtUns8		*curSwapCode;
	UUtUns8		swapCode;
	UUtUns8		*curDataPtr;
	UUtUns32	i;
	UUtUns32	count;
	UUtUns8		*origSwapCode;

	UUtUns32	curSize = 0;

	TMmInstanceFile_Verify(inInstanceFile);

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

				count = *curSwapCode++;

				origSwapCode = curSwapCode;

				for(i = 0; i < count; i++)
				{
					curSwapCode = origSwapCode;

					curSize +=
						TMiInstance_ComputeSize_Recursive(
							&curSwapCode,
							&curDataPtr,
							inInstanceFile);

				}
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				switch(*curSwapCode++)
				{
					case TMcSwapCode_8Byte:
						count = (UUtUns32) *(UUtUns64 *)curDataPtr;
						curDataPtr += 8;
						break;

					case TMcSwapCode_4Byte:
						count = *(UUtUns32 *)curDataPtr;
						curDataPtr += 4;
						break;

					case TMcSwapCode_2Byte:
						count = *(UUtUns16 *)curDataPtr;
						curDataPtr += 2;
						break;

					default:
						UUmDebugStr("swap codes are messed up.");
				}

				if(count == 0)
				{
					TMiSkipVarArray(&curSwapCode);
				}
				else
				{
					origSwapCode = curSwapCode;

					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;

						curSize +=
							TMiInstance_ComputeSize_Recursive(
								&curSwapCode,
								&curDataPtr,
								inInstanceFile);
					}
				}
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
			case TMcSwapCode_TemplateReference:
				{
					TMtInstanceDescriptor	*targetDesc;
					void					*targetData;

					UUmAssertReadPtr(curDataPtr, sizeof(void *));

					targetData = *(void **)curDataPtr;

					if(targetData != NULL)
					{
						targetDesc = TMrPlaceHolder_GetInstanceDesc(inInstanceFile, (TMtPlaceHolder)targetData);
						UUmAssert(targetDesc != NULL);
						TMmInstanceDesc_Verify(targetDesc);

						targetDesc->flags |= TMcDescriptorFlags_Touched;

						curSize += targetDesc->size;

						if(!(targetDesc->flags & TMcDescriptorFlags_PlaceHolder))
						{
							UUtUns8*	swapCode;
							UUtUns8*	dataPtr;

							swapCode = targetDesc->templatePtr->swapCodes;
							dataPtr = targetDesc->dataPtr;

							TMmInstanceData_Verify(targetDesc->dataPtr);

							TMiInstance_ComputeSize_Recursive(
								&swapCode,
								&dataPtr,
								inInstanceFile);
						}
					}
					if(swapCode == TMcSwapCode_TemplatePtr)
					{
						curSwapCode += 4;
						curDataPtr += 4;
					}
					else
					{
						curDataPtr += 8;
					}
				}
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;

	return curSize;
}

/*
 * This is used to assign the variable length array
 */
static void
TMiInstance_VarArray_Reset_Recursive(
	UUtUns8*	*ioSwapCode,
	UUtUns8*	*ioDataPtr,
	UUtUns32	inInitialVarArrayLength)
{
	UUtBool		stop;
	UUtUns8		*curSwapCode;
	UUtUns8		*curDataPtr;
	UUtUns32	i;
	UUtUns32	count;
	UUtUns8		*origSwapCode;

//	UUmAssert(gInGame == UUcFalse);

	curSwapCode = *ioSwapCode;
	curDataPtr = *ioDataPtr;

	stop = UUcFalse;

	while(!stop)
	{
		switch(*curSwapCode++)
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
				count = *curSwapCode++;

				origSwapCode = curSwapCode;

				for(i = 0; i < count; i++)
				{
					curSwapCode = origSwapCode;

					TMiInstance_VarArray_Reset_Recursive(
						&curSwapCode,
						&curDataPtr,
						inInitialVarArrayLength);

				}
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_BeginVarArray:
				switch(*curSwapCode++)
				{
					case TMcSwapCode_8Byte:
						*(UUtUns64 *)curDataPtr = inInitialVarArrayLength;
						curDataPtr += 8;
						break;

					case TMcSwapCode_4Byte:
						*(UUtUns32 *)curDataPtr = inInitialVarArrayLength;
						curDataPtr += 4;
						break;

					case TMcSwapCode_2Byte:
						*(UUtUns16 *)curDataPtr = (UUtUns16)inInitialVarArrayLength;
						curDataPtr += 2;
						break;

					default:
						UUmDebugStr("swap codes are messed up.");
				}

				origSwapCode = curSwapCode;

				if(inInitialVarArrayLength > 0)
				{
					for(i = 0; i < inInitialVarArrayLength; i++)
					{
						curSwapCode = origSwapCode;

						TMiInstance_VarArray_Reset_Recursive(
							&curSwapCode,
							&curDataPtr,
							inInitialVarArrayLength);
					}
				}
				else
				{
					TMiSkipVarArray(&curSwapCode);
				}
				break;

			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
				*(void**)curDataPtr = NULL;
				curSwapCode += 4;
				curDataPtr += 4;
				break;

			case TMcSwapCode_TemplateReference:
				*(void**)curDataPtr = NULL;
				curDataPtr += 8;
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
}

static void
TMiInstanceFile_DumpPlaceHolders(
	TMtInstanceFile*	inInstanceFile,
	char*				inFileName,
	FILE*				inFile)
{
	UUtUns32				curDescIndex;
	TMtNameDescriptor*		curNameDesc;
	UUtUns32				numPlaceHolders = 0;

	fprintf(inFile, "Place holder dump"UUmNL);
	fprintf(inFile, "fileName: %s"UUmNL, BFrFileRef_GetLeafName(inInstanceFile->instanceFileRef));

	for(curDescIndex = 0, curNameDesc = inInstanceFile->nameDescriptors;
		curDescIndex < inInstanceFile->numNameDescriptors;
		curDescIndex++, curNameDesc++)
	{
		if((inInstanceFile->instanceDescriptors + curNameDesc->instanceDescIndex)->flags & TMcDescriptorFlags_PlaceHolder)
		{
			fprintf(inFile, "[%d] Name: %s"UUmNL, curDescIndex, curNameDesc->namePtr);
			numPlaceHolders++;
		}
	}

	if(numPlaceHolders > 0)
	{
		fprintf(stderr, "YO: I found %d place holders see %s for details."UUmNL, numPlaceHolders, inFileName);
	}
}

static UUtError
TMiInstanceFile_PrepareForMemory(
	TMtInstanceFile*	inInstanceFile,
	char*				inMsg)
{
	UUtError				error;
	UUtUns32				curDescIndex;
	TMtInstanceDescriptor*	curDesc;

	UUtUns8*				swapCode;
	UUtUns8*				dataPtr;
	char					msg[2048];

	TMmInstanceFile_Verify(inInstanceFile);

	for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curDesc++)
	{
		if(curDesc->templatePtr->flags & TMcTemplateFlag_Leaf) continue;

		swapCode = curDesc->templatePtr->swapCodes;
		dataPtr = (UUtUns8 *)curDesc->dataPtr;

		if(dataPtr == NULL) continue;

		if (NULL == dataPtr) {
			UUmError_ReturnOnErrorMsgP(UUcError_Generic, "Loading, could not resolve place holder '%s'.", (UUtUns32) curDesc->namePtr, 0, 0);
		}

		UUmAssert(dataPtr != NULL); //  && "This probably means that there is a bogus place holder...");

		TMmInstanceDesc_Verify(curDesc);

		sprintf(
			msg,
			"%s: template %s[%s], name %s",
			inMsg,
			curDesc->templatePtr->name,
			UUrTag2Str(curDesc->templatePtr->tag),
			(curDesc->namePtr!=NULL) ? curDesc->namePtr + 4 : "NULL");

		error =
			TMiInstance_PrepareForMemory(
				&swapCode,
				&dataPtr,
				inInstanceFile,
				msg,
				(curDesc->templatePtr->flags & TMcTemplateFlag_VarArrayIsLeaf) ? UUcTrue : UUcFalse );
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

static UUtError
TMiInstanceFile_PrepareForDisk(
	TMtInstanceFile*	inInstanceFile)
{
	UUtUns32				curDescIndex;
	TMtInstanceDescriptor*	curDesc;

	UUtUns8*				swapCode;
	UUtUns8*				dataPtr;

	TMmInstanceFile_Verify(inInstanceFile);

	for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curDesc++)
	{
		swapCode = curDesc->templatePtr->swapCodes;
		dataPtr = (UUtUns8 *)curDesc->dataPtr;

		TMmInstanceDesc_Verify(curDesc);

		if(dataPtr == NULL) continue;

		TMiInstance_PrepareForDisk(
			&swapCode,
			&dataPtr,
			inInstanceFile,
			UUcFalse);
	}

	return UUcError_None;
}

static UUtError
TMiInstanceFile_CallProcs(
	TMtInstanceFile*		inInstanceFile,
	TMtTemplateProc_Message	inMessage)
{
	UUtError				error;
	UUtUns32				curDescIndex;
	TMtInstanceDescriptor*	curDesc;

	TMmInstanceFile_Verify(inInstanceFile);

	for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curDesc++)
	{
		TMmInstanceDesc_Verify(curDesc);

		if(curDesc->dataPtr == NULL) continue;

		if(curDesc->templatePtr->handler != NULL)
		{
			error = curDesc->templatePtr->handler(inMessage, curDesc->templatePtr->tag, curDesc->dataPtr);
			UUmError_ReturnOnErrorMsg(error, "Could not call proc");
		}
	}

	return UUcError_None;
}

static UUtError
TMiInstanceFile_New(
	BFtFileRef *inInstanceFileRef)
{
	BFtFile*	emptyFile;
	UUtError				error;
	TMtInstanceFile_Header	fileHeader;

	// make it an empty template file (delete, create, open, write file header goo and close)
	error = BFrFile_Delete(inInstanceFileRef);

	error = BFrFile_Create(inInstanceFileRef);
	if (error) {
		AUrMessageBox(AUcMBType_OK, "Could not create the .dat file (is it locked ?).");
	}
	UUmError_ReturnOnError(error);

	error = BFrFile_Open(inInstanceFileRef, "w", &emptyFile);
	UUmError_ReturnOnError(error);

	fileHeader.version = TMcInstanceFile_Version;
	fileHeader.totalTemplateChecksum = gTemplateChecksum;
	fileHeader.numInstanceDescriptors = 0;
	fileHeader.numNameDescriptors = 0;
	fileHeader.numTemplateDescriptors = 0;
	fileHeader.dataBlockOffset = sizeof(TMtInstanceFile_Header);
	fileHeader.nameBlockOffset = sizeof(TMtInstanceFile_Header);
	fileHeader.dataBlockLength = 0;
	fileHeader.nameBlockLength = 0;

	// Write out endian detector
		error = BFrFile_WritePos(emptyFile, 0, sizeof(fileHeader), &fileHeader);
		UUmError_ReturnOnError(error);

	BFrFile_Close(emptyFile);

	return UUcError_None;
}

static UUtError
TMiInstanceFile_LoadHeaderFromMemory(
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

		UUrSwap_8Byte(&inFileHeader->totalTemplateChecksum);

		UUrSwap_4Byte(&inFileHeader->numInstanceDescriptors);
		UUrSwap_4Byte(&inFileHeader->numNameDescriptors);
		UUrSwap_4Byte(&inFileHeader->numTemplateDescriptors);

		UUrSwap_4Byte(&inFileHeader->dataBlockOffset);
		UUrSwap_4Byte(&inFileHeader->dataBlockLength);

		UUrSwap_4Byte(&inFileHeader->nameBlockOffset);
		UUrSwap_4Byte(&inFileHeader->nameBlockLength);
	}

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
TMiInstanceFile_LoadHeader(
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

	error = TMiInstanceFile_LoadHeaderFromMemory(outFileHeader, outNeedsSwapping);

	return error;
}


static UUtError
TMiInstanceFile_TraverseAndSet(
	TMtInstanceFile*	inInstanceFile,
	UUtBool				inNeedsSwapping,
	UUtUns32			*outDynamicMemSize)
{
	UUtError				error;
	UUtUns32				curDescIndex;
	TMtInstanceDescriptor*	curDesc;
	UUtUns32				dynamicMemSize;

	TMtNameDescriptor*		curNameDesc;

	UUrMemory_Block_VerifyList();

	dynamicMemSize = 0;

	UUmAssertAlignedPtr(inInstanceFile->dataBlock);

	// Traverse through the instances and set the fields
	for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curDesc++)
	{
		if(inNeedsSwapping)
		{
			UUrSwap_4Byte(&curDesc->templatePtr);
			UUrSwap_4Byte(&curDesc->dataPtr);
			UUrSwap_4Byte(&curDesc->namePtr);
			UUrSwap_4Byte(&curDesc->size);
			UUrSwap_4Byte(&curDesc->flags);
			UUrSwap_4Byte(&curDesc->creationDate);
			UUrSwap_8Byte(&curDesc->checksum);
		}

		curDesc->templatePtr = TMiTemplate_FindDefinition((TMtTemplateTag)curDesc->templatePtr);

		UUmAssert(curDesc->templatePtr != NULL);

		curDesc->flags = (TMtDescriptorFlags)(curDesc->flags & TMcFlags_PersistentMask);

		if(!(curDesc->flags & TMcDescriptorFlags_PlaceHolder))
		{
			// Check for an out of date instance
			if(curDesc->checksum != curDesc->templatePtr->checksum)
			{
				if(gInGame == UUcTrue)
				{
					UUmError_ReturnOnErrorMsg(UUcError_Generic, "instance checksum does not match");
				}

				curDesc->dataPtr = NULL;
				if(curDesc->flags & TMcDescriptorFlags_Unique)
				{
					curDesc->flags |= TMcDescriptorFlags_DeleteMe;
				}
				else
				{
					// Turn it into a place holder
					curDesc->flags |= TMcDescriptorFlags_PlaceHolder;
				}
			}
			else
			{
				curDesc->dataPtr = (char*)inInstanceFile->dataBlock + (UUtUns32)curDesc->dataPtr;
			}

			if(inNeedsSwapping)
			{
				UUtUns8*	swapCode;
				UUtUns8*	dataPtr;

				swapCode = curDesc->templatePtr->swapCodes;
				dataPtr = curDesc->dataPtr;

				error = TMiInstance_ByteSwap(&swapCode, &dataPtr, curDesc->templatePtr->byteSwapProc);
				UUmError_ReturnOnError(error);
			}
		}
		else
		{
			curDesc->dataPtr = NULL;
		}

		if(!(curDesc->flags & TMcDescriptorFlags_Unique))
		{
			curDesc->namePtr = (char*)inInstanceFile->nameBlock + (UUtUns32)curDesc->namePtr;
		}
		else
		{
			curDesc->namePtr = NULL;
		}


		dynamicMemSize += curDesc->templatePtr->privateDataSize;

		// this is where all the majority of the time is spent loading the file (on the PC)
		if(curDesc->dataPtr != NULL)
		{
			UUmAssertAlignedPtr(curDesc->dataPtr);
			TMmInstanceData_GetPlaceHolder(curDesc->dataPtr) = TMmPlaceHolder_MakeFromIndex(curDescIndex);
			TMmInstanceData_GetInstanceFile(curDesc->dataPtr) = inInstanceFile;
			TMmInstanceData_GetMagicCookie(curDesc->dataPtr) = TMcMagicCookie;
			TMmInstanceData_GetTemplateDefinition(curDesc->dataPtr) = curDesc->templatePtr;
		}
	}

// Traverse through the name descs and update pointers
	for(curDescIndex = 0, curNameDesc = inInstanceFile->nameDescriptors;
		curDescIndex < inInstanceFile->numNameDescriptors;
		curDescIndex++, curNameDesc++)
	{
		if(inNeedsSwapping)
		{
			UUrSwap_4Byte(&curNameDesc->instanceDescIndex);
		}

		UUmAssert(curNameDesc->instanceDescIndex < inInstanceFile->numInstanceDescriptors);

		curNameDesc->namePtr =
			(inInstanceFile->instanceDescriptors + curNameDesc->instanceDescIndex)->namePtr;

		UUmAssert(curNameDesc->namePtr != NULL);
	}

	*outDynamicMemSize = dynamicMemSize;

	return UUcError_None;
}

static UUtError
TMiInstanceFile_Reload_InGame(
	TMtInstanceFile*	inInstanceFile)
{
	UUtError				error;

	BFtFile*				instancePhysicalFile = NULL;

	UUtBool					needsSwapping;

	UUtUns32				totalFileLength;

	UUtUns32				curDescIndex;
	TMtInstanceDescriptor*	curDesc;

	UUtUns32				dynamicMemSize;
	char*					curDynamicMemPtr;
	UUtUns8					*mappingPtr;

	TMtInstanceFile_Header	*fileHeader;

	UUrMemory_Block_VerifyList();

	TMmInstanceFile_Verify(inInstanceFile);

	/*
	 * open the instance file data
	 */

		if(inInstanceFile->mapping != NULL)
		{
			BFrFile_UnMap(inInstanceFile->mapping);
			inInstanceFile->mapping = NULL;
		}

		error = BFrFile_Map(
			inInstanceFile->instanceFileRef,
			0,
			&(inInstanceFile->mapping),
			&mappingPtr,
			&totalFileLength);

		if (error != UUcError_None)
		{
			error = error != UUcError_OutOfMemory ? TMcError_DataCorrupt : UUcError_OutOfMemory;
			UUmError_ReturnOnErrorMsg(error, "Could not open instance file");
		}

	UUrMemory_Block_VerifyList();


		fileHeader = (TMtInstanceFile_Header *) mappingPtr;
		error = TMiInstanceFile_LoadHeaderFromMemory(fileHeader, &needsSwapping);

		UUmAssert(totalFileLength == fileHeader->nameBlockOffset + fileHeader->nameBlockLength);

		inInstanceFile->numInstanceDescriptors = fileHeader->numInstanceDescriptors;
		inInstanceFile->numNameDescriptors = fileHeader->numNameDescriptors;
		inInstanceFile->dataBlockLength = fileHeader->dataBlockLength;
		inInstanceFile->nameBlockLength = fileHeader->nameBlockLength;

		inInstanceFile->numTemplateDescriptors = 0;
		inInstanceFile->templateDescriptors = NULL;

		UUrMemory_Block_VerifyList();

		// map the stuff
		inInstanceFile->instanceDescriptors = (struct TMtInstanceDescriptor *) (mappingPtr + sizeof(TMtInstanceFile_Header));
		inInstanceFile->nameDescriptors = (struct TMtNameDescriptor *) (mappingPtr + sizeof(TMtInstanceFile_Header) + inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor));
		inInstanceFile->dataBlock = (mappingPtr + fileHeader->dataBlockOffset);
		inInstanceFile->nameBlock = (char*)(mappingPtr + fileHeader->nameBlockOffset);

	/// xxxx
	error = TMiInstanceFile_TraverseAndSet(inInstanceFile, needsSwapping, &dynamicMemSize);
	UUmError_ReturnOnErrorMsg(error, "TMiInstanceFile_TraverseAndSet failed");

	UUrMemory_Block_VerifyList();

	// If in game allocate dynamic memory
	if(dynamicMemSize > 0)
	{
		inInstanceFile->dynamicMemory = curDynamicMemPtr = UUrMemory_Block_New(dynamicMemSize);
		if(curDynamicMemPtr == NULL)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not allocate dynamic mem");
		}

		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->templatePtr->privateDataSize != 0 && curDesc->dataPtr != NULL)
			{
				TMmInstance_GetDynamicData(curDesc->dataPtr) = curDynamicMemPtr;
				curDynamicMemPtr += curDesc->templatePtr->privateDataSize;
			}
			else if(curDesc->dataPtr != NULL)
			{
				TMmInstance_GetDynamicData(curDesc->dataPtr) = NULL;
			}
		}
	}
	else
	{
		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if (curDesc->dataPtr != NULL)
			{
				TMmInstance_GetDynamicData(curDesc->dataPtr) = NULL;
			}
		}
	}

	return UUcError_None;
}


static UUtError
TMiInstanceFile_Reload(
	TMtInstanceFile*	inInstanceFile,
	UUtBool				inOKToRebuild)
{
	UUtError				error;

	BFtFile*				instancePhysicalFile = NULL;

	UUtBool					needsSwapping;

	UUtUns32				totalFileLength;

	UUtUns32				curDescIndex;
	TMtInstanceDescriptor*	curDesc;

	UUtUns32				dynamicMemSize;
	char*					curDynamicMemPtr;

	TMtInstanceFile_Header	fileHeader;

	UUmAssert(gInGame == UUcTrue ? inOKToRebuild == UUcFalse : 1);
	if (gInGame) {
		error = TMiInstanceFile_Reload_InGame(inInstanceFile);
		return error;
	}

	UUrMemory_Block_VerifyList();

	TMmInstanceFile_Verify(inInstanceFile);

	/*
	 * open the instance file data
	 */
		error = BFrFile_Open(inInstanceFile->instanceFileRef, "r", &instancePhysicalFile);

		// Check to see if we should create it
		if ((error != UUcError_None) && (gInGame == UUcFalse))
		{
			instancePhysicalFile = NULL;

fuckingRebuild:
			//UUmAssert(!"Rebuilding, ok to continue");

			inOKToRebuild = UUcFalse;

			if(instancePhysicalFile != NULL)
			{
				BFrFile_Close(instancePhysicalFile);
			}

			error = TMiInstanceFile_New(inInstanceFile->instanceFileRef);
			UUmError_ReturnOnErrorMsg(error, "Could not create instance file");

			// try to open it again
			error = BFrFile_Open(inInstanceFile->instanceFileRef, "r", &instancePhysicalFile);
		}

		if (error != UUcError_None)
		{
			error = error != UUcError_OutOfMemory ? TMcError_DataCorrupt : UUcError_OutOfMemory;
			UUmError_ReturnOnErrorMsg(error, "Could not open instance file");
		}

	#if defined(TMmBrentSucks) && TMmBrentSucks

		if(inOKToRebuild) goto fuckingRebuild;

	#endif

	UUrMemory_Block_VerifyList();

		// Get total file length
	 		error =
	 			BFrFile_GetLength(
	 				instancePhysicalFile,
	 				&totalFileLength);
			UUmError_ReturnOnError(error);

		error = TMiInstanceFile_LoadHeader(instancePhysicalFile, &fileHeader, &needsSwapping);
		if(error != UUcError_None && inOKToRebuild && gInGame)
		{
			inOKToRebuild = UUcFalse;
			goto fuckingRebuild;
		}

		UUmAssert(totalFileLength == fileHeader.nameBlockOffset + fileHeader.nameBlockLength);

		inInstanceFile->numInstanceDescriptors = fileHeader.numInstanceDescriptors;
		inInstanceFile->numNameDescriptors = fileHeader.numNameDescriptors;
		inInstanceFile->dataBlockLength = fileHeader.dataBlockLength;
		inInstanceFile->nameBlockLength = fileHeader.nameBlockLength;

		inInstanceFile->numTemplateDescriptors = 0;
		inInstanceFile->templateDescriptors = NULL;

		UUrMemory_Block_VerifyList();

		if(gInGame == UUcTrue)
		{
			if(inInstanceFile->allocBlock != NULL)
			{
				UUrMemory_Block_Delete(inInstanceFile->allocBlock);
			}

			inInstanceFile->allocBlock = UUrMemory_Block_New(totalFileLength - sizeof(TMtInstanceFile_Header));

			if(inInstanceFile->allocBlock == NULL)
			{
				UUmError_ReturnOnErrorMsgP(UUcError_OutOfMemory, "%s big alloc of %d",
						(UUtUns32) BFrFileRef_GetLeafName(inInstanceFile->instanceFileRef),
						totalFileLength - sizeof(TMtInstanceFile_Header),
						0);
			}

			inInstanceFile->instanceDescriptors = (TMtInstanceDescriptor*)(inInstanceFile->allocBlock);

			inInstanceFile->nameDescriptors =
				(TMtNameDescriptor*)((char*)inInstanceFile->allocBlock +
					inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor));

			inInstanceFile->dataBlock = (char*)inInstanceFile->allocBlock + fileHeader.dataBlockOffset - sizeof(TMtInstanceFile_Header);

			inInstanceFile->nameBlock = (char*)inInstanceFile->allocBlock + fileHeader.nameBlockOffset - sizeof(TMtInstanceFile_Header);
		}
		else
		{
			if(gInstanceDescriptorArray == NULL)
			{
				gInstanceDescriptorArray =
					UUrMemory_Array_New(
						sizeof(TMtInstanceDescriptor),
						20,
						fileHeader.numInstanceDescriptors,
						fileHeader.numInstanceDescriptors + 50);
				if(gInstanceDescriptorArray == NULL)
				{
					UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not initialize gInstanceDescriptorArray");
				}
			}
			else
			{
				error = UUrMemory_Array_SetUsedElems(gInstanceDescriptorArray, fileHeader.numInstanceDescriptors, NULL);
				UUmError_ReturnOnErrorMsg(error, "Could not initialize gInstanceDescriptorArray");
			}

			if(gNameDescriptorArray == NULL)
			{
				gNameDescriptorArray =
					UUrMemory_Array_New(
						sizeof(TMtNameDescriptor),
						20,
						fileHeader.numNameDescriptors,
						fileHeader.numNameDescriptors + 50);
				if(gNameDescriptorArray == NULL)
				{
					UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not initialize gNameDescriptorArray");
				}
			}
			else
			{
				error = UUrMemory_Array_SetUsedElems(gNameDescriptorArray, fileHeader.numNameDescriptors, NULL);
				UUmError_ReturnOnErrorMsg(error, "Could not initialize gNameDescriptorArray");
			}

			inInstanceFile->instanceDescriptors = UUrMemory_Array_GetMemory(gInstanceDescriptorArray);
			UUmAssert(inInstanceFile->instanceDescriptors != NULL);

			inInstanceFile->nameDescriptors = UUrMemory_Array_GetMemory(gNameDescriptorArray);
			UUmAssert(inInstanceFile->nameDescriptors != NULL);

			inInstanceFile->allocBlock = NULL;

			if(inInstanceFile->dataBlock != NULL)
			{
				UUrMemory_Block_Delete(inInstanceFile->dataBlock);
				inInstanceFile->dataBlock = NULL;
			}

			if(inInstanceFile->nameBlock != NULL)
			{
				UUrMemory_Block_Delete(inInstanceFile->nameBlock);
				inInstanceFile->nameBlock = NULL;
			}

			UUrMemory_Block_VerifyList();

			if(inInstanceFile->dataBlockLength > 0)
			{
				inInstanceFile->dataBlock = UUrMemory_Block_New(inInstanceFile->dataBlockLength);
				if(inInstanceFile->dataBlock == NULL)
				{
					UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not alloc data block");
				}
			}

			if(inInstanceFile->nameBlockLength > 0)
			{
				inInstanceFile->nameBlock = UUrMemory_Block_New(inInstanceFile->nameBlockLength);
				if(inInstanceFile->nameBlock == NULL)
				{
					UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not alloc name block");
				}
			}

			// load the template descriptor array
			if(fileHeader.numTemplateDescriptors > 0)
			{
				inInstanceFile->numTemplateDescriptors = fileHeader.numTemplateDescriptors;
				UUmAssert(inInstanceFile->numTemplateDescriptors < TMgNumTemplateDefinitions);
				inInstanceFile->templateDescriptors = UUrMemory_Block_New(inInstanceFile->numTemplateDescriptors * sizeof(TMtTemplateDescriptor));
				UUmError_ReturnOnNull(inInstanceFile->templateDescriptors);
				error =
					BFrFile_ReadPos(
						instancePhysicalFile,
						sizeof(TMtInstanceFile_Header) +
							inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor) +
							inInstanceFile->numNameDescriptors * sizeof(TMtNameDescriptor),
						inInstanceFile->numTemplateDescriptors * sizeof(TMtTemplateDescriptor),
						inInstanceFile->templateDescriptors);
				if(error != UUcError_None)
				{
					UUmError_ReturnOnErrorMsg(TMcError_DataCorrupt, "Could not read instance descs");
				}
			}
		}

		if(inInstanceFile->numInstanceDescriptors > 0)
		{
			//Read in the instance descriptors
				error =
					BFrFile_ReadPos(
						instancePhysicalFile,
						sizeof(TMtInstanceFile_Header),
						 inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor),
						inInstanceFile->instanceDescriptors);
				if(error != UUcError_None)
				{
					UUmError_ReturnOnErrorMsg(TMcError_DataCorrupt, "Could not read instance descs");
				}
		}

		UUrMemory_Block_VerifyList();
		if(inInstanceFile->numNameDescriptors > 0)
		{
			//Read in the name descriptors
				error =
					BFrFile_ReadPos(
						instancePhysicalFile,
						sizeof(TMtInstanceFile_Header) + inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor),
						inInstanceFile->numNameDescriptors * sizeof(TMtNameDescriptor),
						inInstanceFile->nameDescriptors);
				if(error != UUcError_None)
				{
					UUmError_ReturnOnErrorMsg(TMcError_DataCorrupt, "Could not read instance descs");
				}
		}

		UUrMemory_Block_VerifyList();
		if(fileHeader.dataBlockLength > 0)
		{
			//Read in the data block
			error =
				BFrFile_ReadPos(
					instancePhysicalFile,
					fileHeader.dataBlockOffset,
					inInstanceFile->dataBlockLength,
					inInstanceFile->dataBlock);
			if(error != UUcError_None)
			{
				UUmError_ReturnOnErrorMsg(TMcError_DataCorrupt, "Could not read instance descs");
			}
		}

		UUrMemory_Block_VerifyList();
		if(fileHeader.nameBlockLength > 0)
		{
			//Read in the name block
			error =
				BFrFile_ReadPos(
					instancePhysicalFile,
					fileHeader.nameBlockOffset,
					inInstanceFile->nameBlockLength,
					inInstanceFile->nameBlock);
			if(error != UUcError_None)
			{
				UUmError_ReturnOnErrorMsg(TMcError_DataCorrupt, "Could not read instance descs");
			}
		}

	UUrMemory_Block_VerifyList();

	/// xxxx
	error = TMiInstanceFile_TraverseAndSet(inInstanceFile, needsSwapping, &dynamicMemSize);
	UUmError_ReturnOnErrorMsg(error, "TMiInstanceFile_TraverseAndSet failed");

	UUrMemory_Block_VerifyList();

	// If in game allocate dynamic memory
	if(gInGame == UUcTrue && dynamicMemSize > 0)
	{
		inInstanceFile->dynamicMemory = curDynamicMemPtr = UUrMemory_Block_New(dynamicMemSize);
		if(curDynamicMemPtr == NULL)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not allocate dynamic mem");
		}

		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->templatePtr->privateDataSize != 0 && curDesc->dataPtr != NULL)
			{
				TMmInstance_GetDynamicData(curDesc->dataPtr) = curDynamicMemPtr;
				curDynamicMemPtr += curDesc->templatePtr->privateDataSize;
			}
			else if(curDesc->dataPtr != NULL)
			{
				TMmInstance_GetDynamicData(curDesc->dataPtr) = NULL;
			}
		}
	}
	else
	{
		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if (curDesc->dataPtr != NULL)
			{
				TMmInstance_GetDynamicData(curDesc->dataPtr) = NULL;
			}
		}
	}

	BFrFile_Close(instancePhysicalFile);

	if(!gInGame)
	{
		TMmInstanceFile_MajorCheck(inInstanceFile, UUcTrue);
	}

	return UUcError_None;
}

static UUtError
TMiInstanceFile_MakeFromFileRef(
	BFtFileRef*			inInstanceFileRef,
	TMtInstanceFile*	*outNewInstanceFile,
	UUtBool				inOKToRebuild)
{
	UUtError				error;
	TMtInstanceFile			*newInstanceFile;

	*outNewInstanceFile = NULL;

	/*
	 * Create the instance file structure
	 */
		newInstanceFile = (TMtInstanceFile *)UUrMemory_Block_New(sizeof(TMtInstanceFile));
		if(newInstanceFile == NULL)
		{
			UUrError_Report(UUcError_OutOfMemory, "Could not open instance file");
			return UUcError_OutOfMemory;
		}

	/*
	 * Copy the instanceFileRef into the new structure
	 */
		error = BFrFileRef_Duplicate(inInstanceFileRef, &newInstanceFile->instanceFileRef);
		UUmError_ReturnOnErrorMsg(error, "Could not duplicate instance file reference");

	newInstanceFile->dataBlock = NULL;
	newInstanceFile->nameBlock = NULL;
	newInstanceFile->instanceDescriptors = NULL;
	newInstanceFile->nameDescriptors = NULL;
	newInstanceFile->allocBlock = NULL;
	newInstanceFile->mapping = NULL;
	newInstanceFile->dynamicMemory = NULL;
	newInstanceFile->dataBlockLength = 0;
	newInstanceFile->nameBlockLength = 0;
	newInstanceFile->numInstanceDescriptors = 0;
	newInstanceFile->numNameDescriptors = 0;

	if(!gInGame)
	{
		gConstructionFile = newInstanceFile;
	}

	#if defined(DEBUGGING) && DEBUGGING

		newInstanceFile->magicCookie = TMcMagicCookie;

	#endif

	error = TMiInstanceFile_Reload(newInstanceFile, inOKToRebuild);
	UUmError_ReturnOnErrorMsg(error, "Could not reload file");

	*outNewInstanceFile = newInstanceFile;

	return UUcError_None;
}

static UUtUns32				TMgCompare_TemplateIndex = UUcMaxUns32;
static TMtInstanceFile*		TMgCompare_InstanceFile = NULL;
static TMtInstanceDescriptor*	TMgCompare_InstanceBase = NULL;

static UUtInt8
TMiInstanceFile_RemoveIdentical_Compare(
	UUtUns32	inA,
	UUtUns32	inB)
{
	TMtInstanceDescriptor*	instanceDescA = TMgCompare_InstanceBase + inA;
	TMtInstanceDescriptor*	instanceDescB = TMgCompare_InstanceBase + inB;

	UUmAssertReadPtr(TMgCompare_InstanceFile, sizeof(*TMgCompare_InstanceFile));

	UUmAssert(instanceDescA->templatePtr == instanceDescB->templatePtr);

	if(TMgCompare_TemplateIndex != UUcMaxUns32)
	{
		UUtInt32	extIndex = instanceDescA->templatePtr - TMgTemplateDefinitionArray;

		UUmAssert(TMgCompare_TemplateIndex == (UUtUns32)extIndex);
	}

	// first compare based on size
	if(instanceDescA->size > instanceDescB->size) return -1;
	if(instanceDescA->size < instanceDescB->size) return 1;

	if(instanceDescA->dataPtr == NULL) return 1;
	if(instanceDescB->dataPtr == NULL) return -1;

	// next compare based on value of the data
	{
		UUtUns8*	swapCode;
		UUtUns8*	dataPtrA;
		UUtUns8*	dataPtrB;

		swapCode = instanceDescA->templatePtr->swapCodes;
		dataPtrA = instanceDescA->dataPtr;
		dataPtrB = instanceDescB->dataPtr;

		return TMiInstance_Compare_Recursive(&swapCode, &dataPtrA, &dataPtrB, TMgCompare_InstanceFile);
	}
}

static TMtTemplate_ConstructionData* gTemplateConstructionList = NULL;

static UUtInt8
TMiDumpStats_Compare_InstanceCount(
	UUtUns32	inA,
	UUtUns32	inB)
{
	UUmAssertReadPtr(gTemplateConstructionList, sizeof(TMtTemplateDescriptor));

	if(gTemplateConstructionList[inA].numInstancesUsed == gTemplateConstructionList[inB].numInstancesUsed) return 0;
	if(gTemplateConstructionList[inA].numInstancesUsed > gTemplateConstructionList[inB].numInstancesUsed) return -1;
	return 1;
}

static UUtInt8
TMiDumpStats_Compare_MemSize(
	UUtUns32	inA,
	UUtUns32	inB)
{
	UUmAssertReadPtr(gTemplateConstructionList, sizeof(TMtTemplateDescriptor));

	if(gTemplateConstructionList[inA].totalSizeOfAllInstances == gTemplateConstructionList[inB].totalSizeOfAllInstances) return 0;
	if(gTemplateConstructionList[inA].totalSizeOfAllInstances > gTemplateConstructionList[inB].totalSizeOfAllInstances) return -1;
	return 1;
}

static UUtInt8
TMiDumpStats_Compare_NumRemoved(
	UUtUns32	inA,
	UUtUns32	inB)
{
	UUmAssertReadPtr(gTemplateConstructionList, sizeof(TMtTemplateDescriptor));

	if(gTemplateConstructionList[inA].numInstancesRemoved == gTemplateConstructionList[inB].numInstancesRemoved) return 0;
	if(gTemplateConstructionList[inA].numInstancesRemoved > gTemplateConstructionList[inB].numInstancesRemoved) return -1;
	return 1;
}

static UUtError
TMiInstanceFile_RemoveIdentical(
	TMtInstanceFile*	inInstanceFile)
{
	TMtTemplate_ConstructionData*	curTemplateData;
	UUtUns32						curTemplateDataIndex;

	UUtUns32						curInstanceIndex;
	UUtUns32						targetCheckInstanceIndex;
	UUtUns32						curInstanceIndexItr;
	UUtUns32						targetCheckInstanceIndexItr;
	TMtInstanceDescriptor*			curInstanceDesc;
	TMtInstanceDescriptor*			targetCheckInstance;
	UUtUns32*						duplicateRemapArray;
	TMtTemplate_ConstructionData*	templateConstructionData;

	UUtUns32						numTemplateDescriptors = 0;

	UUtUns32						numTotalInstancesRemoved = 0;
	UUtUns32						totalRemovedSize = 0;

	UUtUns32						numNewInstances;

	UUtUns32						curNameDescIndex;
	TMtNameDescriptor*				curNameDesc;

	TMtInstanceDescriptor*			originalDescriptorList;
	UUtUns32*						originalDuplicateDescriptorArray;

	fprintf(stderr, "Removing identical...\n");

	TMgCompare_InstanceFile = inInstanceFile;
	TMgCompare_InstanceBase = inInstanceFile->instanceDescriptors;

	duplicateRemapArray = UUrMemory_Block_New(inInstanceFile->numInstanceDescriptors * sizeof(UUtInt32));
	UUmError_ReturnOnNull(duplicateRemapArray);

	UUrMemory_Set32(duplicateRemapArray, UUcMaxUns32, inInstanceFile->numInstanceDescriptors * sizeof(UUtUns32));

	templateConstructionData = UUrMemory_Block_NewClear(TMgNumTemplateDefinitions * sizeof(TMtTemplate_ConstructionData));
	UUmError_ReturnOnNull(templateConstructionData);

	UUmAssertReadPtr(gInstanceDuplicateArray, sizeof(UUtUns32));

	UUrMemory_Set32(gInstanceDuplicateArray, UUcMaxUns32, inInstanceFile->numInstanceDescriptors * sizeof(UUtUns32));

	originalDescriptorList = UUrMemory_Block_New(inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor));
	UUmError_ReturnOnNull(originalDescriptorList);

	originalDuplicateDescriptorArray = UUrMemory_Block_New(inInstanceFile->numInstanceDescriptors * sizeof(UUtUns32));
	UUmError_ReturnOnNull(originalDuplicateDescriptorArray);

	// Build the template construction data
	for(curInstanceIndex = 0, curInstanceDesc = inInstanceFile->instanceDescriptors;
		curInstanceIndex < inInstanceFile->numInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		UUtInt32	templateIndex = curInstanceDesc->templatePtr - TMgTemplateDefinitionArray;

		UUmAssert(templateIndex >= 0);
		UUmAssert(templateIndex < (UUtInt32)TMgNumTemplateDefinitions);

		templateConstructionData[templateIndex].totalSizeOfAllInstances += curInstanceDesc->size;

		if(templateConstructionData[templateIndex].numInstancesUsed >= TMcMaxInstancesPerTemplate)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Too many instances for template");
		}

		templateConstructionData[templateIndex].numInstancesUsed++;

		templateConstructionData[templateIndex].instanceIndexList[templateConstructionData[templateIndex].nextInstanceIndex++] = curInstanceIndex;
	}

	fprintf(stderr, "\tSorting each template type");

	// Sort the instances for each template type
	for(curTemplateDataIndex = 0, curTemplateData = templateConstructionData;
		curTemplateDataIndex < TMgNumTemplateDefinitions;
		curTemplateDataIndex++, curTemplateData++)
	{
		if((curTemplateDataIndex % 10) == 0) fprintf(stderr, ".");

		if(curTemplateData->nextInstanceIndex > 0) numTemplateDescriptors++;

		if(curTemplateData->nextInstanceIndex <= 1) continue;

		TMgCompare_TemplateIndex = curTemplateDataIndex;

		AUrQSort_32(
			curTemplateData->instanceIndexList,
			curTemplateData->nextInstanceIndex,
			TMiInstanceFile_RemoveIdentical_Compare);

		TMgCompare_TemplateIndex = UUcMaxUns32;
	}

	fprintf(stderr, "\n");
	fprintf(stderr, "\tLooking for duplicates");

	// now look for duplicates
	for(curTemplateDataIndex = 0, curTemplateData = templateConstructionData;
		curTemplateDataIndex < TMgNumTemplateDefinitions;
		curTemplateDataIndex++, curTemplateData++)
	{
		if((curTemplateDataIndex % 10) == 0) fprintf(stderr, ".");

		if(curTemplateData->nextInstanceIndex == 0) continue;
		if(!(TMgTemplateDefinitionArray[curTemplateDataIndex].flags & TMcTemplateFlag_AllowFolding)) continue;

		TMgCompare_TemplateIndex = curTemplateDataIndex;

		// go through the indices comparing the current one to the subsequent ones looking for identical instances
		curInstanceIndexItr = 0;
		while(curInstanceIndexItr < curTemplateData->nextInstanceIndex)
		{
			targetCheckInstanceIndexItr = curInstanceIndexItr;
			targetCheckInstanceIndex = curTemplateData->instanceIndexList[targetCheckInstanceIndexItr];
			targetCheckInstance = inInstanceFile->instanceDescriptors + targetCheckInstanceIndex;

			// make sure the target is not already a duplicate
			UUmAssert(!(targetCheckInstance->flags & TMcDescriptorFlags_Duplicate));

			while(++curInstanceIndexItr < curTemplateData->nextInstanceIndex)
			{
				curInstanceIndex = curTemplateData->instanceIndexList[curInstanceIndexItr];

				if(TMiInstanceFile_RemoveIdentical_Compare(
					targetCheckInstanceIndex,
					curInstanceIndex) != 0) break;

				// we have an identical instance

				// make sure it has not beeing assigned a target instance already
				UUmAssert(gInstanceDuplicateArray[curInstanceIndex] == UUcMaxUns32);

				// assign this instance a source(target)
				gInstanceDuplicateArray[curInstanceIndex] = targetCheckInstanceIndex;	// This needs to get remaped later

				// Mark the target instance as a source
				targetCheckInstance->flags |= TMcDescriptorFlags_DuplicatedSrc;

				// make sure the source is not a duplicate itself
				UUmAssert(!(targetCheckInstance->flags & TMcDescriptorFlags_Duplicate));

				curInstanceDesc = inInstanceFile->instanceDescriptors + curInstanceIndex;

				// make sure this instance is not a source - because it is a duplicate
				UUmAssert(!(curInstanceDesc->flags & TMcDescriptorFlags_DuplicatedSrc));

				// mark this instance as being a duplicate
				curInstanceDesc->flags |= TMcDescriptorFlags_Duplicate;

				// update some stats
				curTemplateData->numInstancesRemoved++;
				numTotalInstancesRemoved++;

				curTemplateData->removedSize += curInstanceDesc->size;

				totalRemovedSize += curInstanceDesc->size;

				UUmAssert(curTemplateData->numInstancesUsed > 0);
				UUmAssert(curTemplateData->totalSizeOfAllInstances > curInstanceDesc->size);

				curTemplateData->numInstancesUsed--;
				curTemplateData->totalSizeOfAllInstances -= curInstanceDesc->size;
			}
		}
		TMgCompare_TemplateIndex = UUcMaxUns32;
	}

	fprintf(stderr, "\n");
	fprintf(stderr, "\tRemapping");

	UUrMemory_MoveFast(gInstanceDuplicateArray, originalDuplicateDescriptorArray, inInstanceFile->numInstanceDescriptors * sizeof(UUtUns32));
	UUrMemory_MoveFast(inInstanceFile->instanceDescriptors, originalDescriptorList, inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor));

	// compute the remap array and move the structures
		numNewInstances = 0;

		for(curInstanceIndex = 0, curInstanceDesc = originalDescriptorList;
			curInstanceIndex < inInstanceFile->numInstanceDescriptors;
			curInstanceIndex++, curInstanceDesc++)
		{
			if((curInstanceIndex % 1000) == 0) fprintf(stderr, ".");

			if(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate)
			{
				UUmAssert(originalDuplicateDescriptorArray[curInstanceIndex] < inInstanceFile->numInstanceDescriptors);
			}
			else
			{
				UUmAssert(originalDuplicateDescriptorArray[curInstanceIndex] == UUcMaxUns32);
			}

			if(!(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate) || curInstanceDesc->dataPtr == NULL || curInstanceDesc->namePtr != NULL)
			{
				gInstanceDuplicateArray[numNewInstances] = originalDuplicateDescriptorArray[curInstanceIndex];
				inInstanceFile->instanceDescriptors[numNewInstances] = *curInstanceDesc;
				duplicateRemapArray[curInstanceIndex] = numNewInstances++;
			}
			else
			{
				UUmAssert(originalDuplicateDescriptorArray[curInstanceIndex] < inInstanceFile->numInstanceDescriptors);
			}

		}

	TMgCompare_InstanceBase = originalDescriptorList;

	fprintf(stderr, "\n");
	fprintf(stderr, "\tMore remapping");

	// do some verification
	for(curInstanceIndex = 0, curInstanceDesc = originalDescriptorList;
		curInstanceIndex < inInstanceFile->numInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		UUtUns32	remappedInstanceIndex = duplicateRemapArray[curInstanceIndex];

		if((curInstanceIndex % 1000) == 0) fprintf(stderr, ".");

		if(remappedInstanceIndex == UUcMaxUns32)
		{
			// curInstanceIndex is being deleted

			// make sure it has a duplicate
			UUmAssert(originalDuplicateDescriptorArray[curInstanceIndex] < inInstanceFile->numInstanceDescriptors);

			targetCheckInstance = originalDescriptorList + originalDuplicateDescriptorArray[curInstanceIndex];

			// make sure that the duplicate flag is set
			UUmAssert(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate);

			// make sure its source is marked
			UUmAssert(targetCheckInstance->flags & TMcDescriptorFlags_DuplicatedSrc);

			// make sure it should really be deleted
			UUmAssert(curInstanceDesc->namePtr == NULL);

			// this deleted instance need to point to the duplicate source
			duplicateRemapArray[curInstanceIndex] = duplicateRemapArray[originalDuplicateDescriptorArray[curInstanceIndex]];
			UUmAssert(duplicateRemapArray[curInstanceIndex] < numNewInstances);
		}
		else
		{
			// curInstanceIndex is not being deleted

			// make sure it is legal
			UUmAssert(remappedInstanceIndex < numNewInstances);

			// if it is a duplicate then set its dataPtr to NULL
			if(originalDuplicateDescriptorArray[curInstanceIndex] != UUcMaxUns32)
			{
				UUmAssert(originalDuplicateDescriptorArray[curInstanceIndex] < inInstanceFile->numInstanceDescriptors);

				// curInstanceIndex is a duplicate and it is not being deleted
				// screwing up tests - curInstanceDesc->dataPtr = NULL; // This gets remapped to the duplicate data later
			}
		}

		if(originalDuplicateDescriptorArray[curInstanceIndex] != UUcMaxUns32)
		{
			// curInstanceDesc has a duplicate

			targetCheckInstance = originalDescriptorList + originalDuplicateDescriptorArray[curInstanceIndex];

			// make sure it is legal
			UUmAssert(originalDuplicateDescriptorArray[curInstanceIndex] < inInstanceFile->numInstanceDescriptors);

			// make sure its dupicate it not being deleted
			UUmAssert(duplicateRemapArray[originalDuplicateDescriptorArray[curInstanceIndex]] != UUcMaxUns32);

			UUmAssert(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate);

			// make sure its source is marked
			UUmAssert(targetCheckInstance->flags & TMcDescriptorFlags_DuplicatedSrc);
		}
		else
		{
			// curInstanceDesc does not have a duplicate

			UUmAssert(!(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate));
		}
	}

	UUrMemory_Array_SetUsedElems(gInstanceDescriptorArray, numNewInstances, NULL);
	inInstanceFile->numInstanceDescriptors = numNewInstances;

	fprintf(stderr, "\n");
	fprintf(stderr, "\tUpdating place holders");

	// update the place holders, remap gInstanceDuplicateArray, and other stuff
	for(curInstanceIndex = 0, curInstanceDesc = inInstanceFile->instanceDescriptors;
		curInstanceIndex < inInstanceFile->numInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		if((curInstanceIndex % 1000) == 0) fprintf(stderr, ".");

		if(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate)
		{
			UUmAssert(gInstanceDuplicateArray[curInstanceIndex] != UUcMaxUns32);

			gInstanceDuplicateArray[curInstanceIndex] = duplicateRemapArray[gInstanceDuplicateArray[curInstanceIndex]];

			UUmAssert(gInstanceDuplicateArray[curInstanceIndex] < inInstanceFile->numInstanceDescriptors);

		}
		else
		{
			UUmAssert(gInstanceDuplicateArray[curInstanceIndex] == UUcMaxUns32);
		}

		if(curInstanceDesc->dataPtr == NULL) continue;

		TMmInstanceData_GetPlaceHolder(curInstanceDesc->dataPtr) = TMmPlaceHolder_MakeFromIndex(curInstanceIndex);
	}

	fprintf(stderr, "\n");
	fprintf(stderr, "\tRemapping instance data");

	// remap everthing
	for(curInstanceIndex = 0, curInstanceDesc = inInstanceFile->instanceDescriptors;
		curInstanceIndex < inInstanceFile->numInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		if((curInstanceIndex % 1000) == 0) fprintf(stderr, ".");

		if(curInstanceDesc->dataPtr != NULL)
		{
			UUtUns8*	swapCode;
			UUtUns8*	dataPtr;

			swapCode = curInstanceDesc->templatePtr->swapCodes;
			dataPtr = curInstanceDesc->dataPtr;

			TMiInstance_Remap_Recursive(&swapCode, &dataPtr, duplicateRemapArray, inInstanceFile);
		}

	}

	// do some more verifing
	for(curInstanceIndex = 0, curInstanceDesc = inInstanceFile->instanceDescriptors;
		curInstanceIndex < inInstanceFile->numInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		if(gInstanceDuplicateArray[curInstanceIndex] != UUcMaxUns32)
		{
			// curInstanceDesc has a duplicate

			targetCheckInstance = inInstanceFile->instanceDescriptors + gInstanceDuplicateArray[curInstanceIndex];

			// make sure it is legal
			UUmAssert(gInstanceDuplicateArray[curInstanceIndex] < inInstanceFile->numInstanceDescriptors);

			UUmAssert(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate);

			// make sure its source is marked
			UUmAssert(targetCheckInstance->flags & TMcDescriptorFlags_DuplicatedSrc);
		}
		else
		{
			// curInstanceDesc does not have a duplicate

			UUmAssert(!(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate));
		}
	}

	// now remap the name descriptor array
	for(curNameDescIndex = 0, curNameDesc = inInstanceFile->nameDescriptors;
		curNameDescIndex < inInstanceFile->numNameDescriptors;
		curNameDescIndex++, curNameDesc++)
	{
		UUtUns32	remapedIndex = duplicateRemapArray[curNameDesc->instanceDescIndex];

		UUmAssert(remapedIndex < inInstanceFile->numInstanceDescriptors);

		UUmAssert(inInstanceFile->instanceDescriptors[remapedIndex].namePtr != NULL);
		UUmAssert(strcmp(
					inInstanceFile->instanceDescriptors[remapedIndex].namePtr,
					curNameDesc->namePtr) == 0);

		curNameDesc->instanceDescIndex = remapedIndex;
	}

	for(curInstanceIndex = 0, curInstanceDesc = inInstanceFile->instanceDescriptors;
		curInstanceIndex < inInstanceFile->numInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		if(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate)
		{
			UUmAssert(gInstanceDuplicateArray[curInstanceIndex] != UUcMaxUns32);
		}
	}

	fprintf(stderr, "num instances saved: %d\n", numTotalInstancesRemoved);
	fprintf(stderr, "size saved: %d\n", totalRemovedSize);

	UUrMemory_Block_Delete(duplicateRemapArray);

	inInstanceFile->numTemplateDescriptors = numTemplateDescriptors;
	inInstanceFile->templateDescriptors = UUrMemory_Block_New(numTemplateDescriptors * sizeof(TMtTemplateDescriptor));
	numTemplateDescriptors = 0;

	for(curTemplateDataIndex = 0, curTemplateData = templateConstructionData;
		curTemplateDataIndex < TMgNumTemplateDefinitions;
		curTemplateDataIndex++, curTemplateData++)
	{
		if(curTemplateData->numInstancesUsed == 0) continue;

		inInstanceFile->templateDescriptors[numTemplateDescriptors].checksum = TMgTemplateDefinitionArray[curTemplateDataIndex].checksum;
		inInstanceFile->templateDescriptors[numTemplateDescriptors].tag = TMgTemplateDefinitionArray[curTemplateDataIndex].tag;
		inInstanceFile->templateDescriptors[numTemplateDescriptors].numUsed = curTemplateData->numInstancesUsed;
		numTemplateDescriptors++;
	}


	{
		UUtUns32	curDescIndex;
		UUtUns32*	sortedList;
		FILE*		file;
		char		instanceFileName[BFcMaxFileNameLength];
		char		fileName[BFcMaxFileNameLength];

		UUrString_Copy(instanceFileName, BFrFileRef_GetLeafName(inInstanceFile->instanceFileRef), BFcMaxFileNameLength);
		UUrString_StripExtension(instanceFileName);

		sprintf(fileName, "%s_stats.txt", instanceFileName);

		file = fopen(fileName, "w");

		if(file != NULL)
		{
			gTemplateConstructionList = templateConstructionData;

			sortedList = UUrMemory_Block_New(TMgNumTemplateDefinitions * sizeof(UUtUns32));
			UUmError_ReturnOnNull(sortedList);

			for(curDescIndex = 0; curDescIndex < TMgNumTemplateDefinitions; curDescIndex++)
			{
				sortedList[curDescIndex] = curDescIndex;
			}

			AUrQSort_32(
				sortedList,
				TMgNumTemplateDefinitions,
				TMiDumpStats_Compare_InstanceCount);

			fprintf(file, "Instance file dump\n");
			fprintf(file, "fileName: %s\n", BFrFileRef_GetLeafName(inInstanceFile->instanceFileRef));

			fprintf(file, "Template descriptor list(sorted by instance count)\n");
			for(curDescIndex = 0;
				curDescIndex < TMgNumTemplateDefinitions;
				curDescIndex++)
			{
				UUtInt32						templateDefIndex = sortedList[curDescIndex];
				TMtTemplate_ConstructionData*	curTemplateData = templateConstructionData + templateDefIndex;
				TMtTemplateDefinition*			templateDef = TMgTemplateDefinitionArray + templateDefIndex;

				if(curTemplateData->numInstancesUsed == 0) continue;

				fprintf(file, "\tName: %s\n", templateDef->name);
				fprintf(file, "\tTag: %c%c%c%c\n",
					(templateDef->tag >> 24) & 0xFF,
					(templateDef->tag >> 16) & 0xFF,
					(templateDef->tag >> 8) & 0xFF,
					(templateDef->tag >> 0) & 0xFF);
				fprintf(file, "\tNumber Instances Used: %d\n", curTemplateData->numInstancesUsed);
				fprintf(file, "\tTotal Memory Size: %d\n", curTemplateData->totalSizeOfAllInstances);
				fprintf(file, "\tNum Instances Saved: %d\n", curTemplateData->numInstancesRemoved);
				fprintf(file, "\tMemory Saved: %d\n", curTemplateData->removedSize);
				fprintf(file, "\t*******************************\n");
			}

			for(curDescIndex = 0; curDescIndex < TMgNumTemplateDefinitions; curDescIndex++)
			{
				sortedList[curDescIndex] = curDescIndex;
			}

			AUrQSort_32(
				sortedList,
				TMgNumTemplateDefinitions,
				TMiDumpStats_Compare_MemSize);

			fprintf(file, UUmNL"Template descriptor list(sorted by memory size)\n");
			for(curDescIndex = 0;
				curDescIndex < TMgNumTemplateDefinitions;
				curDescIndex++)
			{
				UUtInt32						templateDefIndex = sortedList[curDescIndex];
				TMtTemplate_ConstructionData*	curTemplateData = templateConstructionData + templateDefIndex;
				TMtTemplateDefinition*			templateDef = TMgTemplateDefinitionArray + templateDefIndex;

				if(curTemplateData->numInstancesUsed == 0) continue;

				fprintf(file, "\tName: %s\n", templateDef->name);
				fprintf(file, "\tTag: %c%c%c%c\n",
					(templateDef->tag >> 24) & 0xFF,
					(templateDef->tag >> 16) & 0xFF,
					(templateDef->tag >> 8) & 0xFF,
					(templateDef->tag >> 0) & 0xFF);
				fprintf(file, "\tNumber Instances Used: %d\n", curTemplateData->numInstancesUsed);
				fprintf(file, "\tTotal Memory Size: %d\n", curTemplateData->totalSizeOfAllInstances);
				fprintf(file, "\tNum Instances Saved: %d\n", curTemplateData->numInstancesRemoved);
				fprintf(file, "\tMemory Saved: %d\n", curTemplateData->removedSize);
				fprintf(file, "\t*******************************\n");
			}

			for(curDescIndex = 0; curDescIndex < TMgNumTemplateDefinitions; curDescIndex++)
			{
				sortedList[curDescIndex] = curDescIndex;
			}

			AUrQSort_32(
				sortedList,
				TMgNumTemplateDefinitions,
				TMiDumpStats_Compare_NumRemoved);

			fprintf(file, UUmNL"Template descriptor list(sorted by number removed)\n");
			for(curDescIndex = 0;
				curDescIndex < TMgNumTemplateDefinitions;
				curDescIndex++)
			{
				UUtInt32						templateDefIndex = sortedList[curDescIndex];
				TMtTemplate_ConstructionData*	curTemplateData = templateConstructionData + templateDefIndex;
				TMtTemplateDefinition*			templateDef = TMgTemplateDefinitionArray + templateDefIndex;

				if(curTemplateData->numInstancesUsed == 0) continue;

				fprintf(file, "\tName: %s\n", templateDef->name);
				fprintf(file, "\tTag: %c%c%c%c\n",
					(templateDef->tag >> 24) & 0xFF,
					(templateDef->tag >> 16) & 0xFF,
					(templateDef->tag >> 8) & 0xFF,
					(templateDef->tag >> 0) & 0xFF);
				fprintf(file, "\tNumber Instances Used: %d\n", curTemplateData->numInstancesUsed);
				fprintf(file, "\tTotal Memory Size: %d\n", curTemplateData->totalSizeOfAllInstances);
				fprintf(file, "\tNum Instances Saved: %d\n", curTemplateData->numInstancesRemoved);
				fprintf(file, "\tMemory Saved: %d\n", curTemplateData->removedSize);
				fprintf(file, "\t*******************************\n");
			}

			fclose(file);

			UUrMemory_Block_Delete(sortedList);
		}
	}

	UUrMemory_Block_Delete(templateConstructionData);
	UUrMemory_Block_Delete(originalDuplicateDescriptorArray);
	UUrMemory_Block_Delete(originalDescriptorList);

	TMgCompare_InstanceFile = NULL;

	return UUcError_None;
}


static UUtError
TMiInstanceFile_DeleteUnusedInstances(
	TMtInstanceFile*	inInstanceFile)
{
	UUtUns32				curDescIndex;
	TMtInstanceDescriptor*	curDesc;

	UUtUns32				curNameDescIndex;
	TMtNameDescriptor*		curNameDesc;

	UUtUns32*				deletedRemapArray = NULL;

	UUtUns8*				swapCodes;
	UUtUns8*				dataPtr;

	UUtUns32				newInstanceIndex;

	TMtInstanceDescriptor*	originalDescriptorList;

	fprintf(stderr, "Deleting unused...\n");

	deletedRemapArray = UUrMemory_Block_New(inInstanceFile->numInstanceDescriptors * sizeof(UUtInt32));
	UUmError_ReturnOnNull(deletedRemapArray);

	UUrMemory_Set32(deletedRemapArray, UUcMaxUns32, inInstanceFile->numInstanceDescriptors * sizeof(UUtInt32));

	originalDescriptorList = UUrMemory_Block_New(inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor));
	UUmError_ReturnOnNull(originalDescriptorList);


	UUrMemory_Block_VerifyList();

	/*
	 * Prepare for deletion
	 */
		// go through and decide all the instances that need to be deleted
		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			TMmInstanceDesc_Verify(curDesc);

			if(!(curDesc->flags & TMcDescriptorFlags_Touched))
			{
				// instance has not been touched, delete it
				UUmAssert(!(curDesc->flags & TMcDescriptorFlags_InBatchFile));
				curDesc->flags |= TMcDescriptorFlags_DeleteMe;
			}
			else if(!(curDesc->flags & TMcDescriptorFlags_Unique))
			{
				// Instance has been touched and is not unique
				// Check to see if it was specified in the batch file
				if(!(curDesc->flags & TMcDescriptorFlags_InBatchFile))
				{
					// It was not in the batch file, turn it into a place holder
					curDesc->flags = TMcDescriptorFlags_PlaceHolder;
					if(curDesc->dataPtr != NULL)
					{
						swapCodes = curDesc->templatePtr->swapCodes;
						dataPtr = curDesc->dataPtr;

						TMiInstance_Clean_Recursive(
							&swapCodes,
							&dataPtr,
							inInstanceFile);
					}
					curDesc->dataPtr = NULL;
				}
			}
		}

	/*
	 * NULL out pointers to deleted instances
	 */
		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			swapCodes = curDesc->templatePtr->swapCodes;
			dataPtr = (UUtUns8 *)curDesc->dataPtr;

			if((curDesc->flags & TMcDescriptorFlags_DeleteMe) || dataPtr == NULL)
			{
				continue;
			}

			TMiInstance_NullOutDeletedRefs(&swapCodes, &dataPtr, inInstanceFile);
		}


	UUrMemory_MoveFast(inInstanceFile->instanceDescriptors, originalDescriptorList, inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor));

	/*
	 * Compute the remap array
	 */
		newInstanceIndex = 0;

		for(curDescIndex = 0, curDesc = originalDescriptorList;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			TMmInstanceDesc_Verify(curDesc);

			if(!(curDesc->flags & TMcDescriptorFlags_DeleteMe))
			{
				inInstanceFile->instanceDescriptors[newInstanceIndex] = *curDesc;
				deletedRemapArray[curDescIndex] = newInstanceIndex++;
				continue;
			}

			if(curDesc->namePtr == NULL)
			{
				continue;
			}

			/* we must delete the name descriptor */
			for(curNameDescIndex = 0, curNameDesc = inInstanceFile->nameDescriptors;
				curNameDescIndex < inInstanceFile->numNameDescriptors;
				curNameDescIndex++, curNameDesc++)
			{
				if(curNameDesc->instanceDescIndex == curDescIndex)
				{
					UUmAssert(strcmp(curNameDesc->namePtr, curDesc->namePtr) == 0);

					UUrMemory_Array_DeleteElement(gNameDescriptorArray, curNameDescIndex);
					inInstanceFile->numNameDescriptors--;
					break;
				}
			}
		}

	for(curDescIndex = 0, curDesc = originalDescriptorList;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curDesc++)
	{
		if(deletedRemapArray[curDescIndex] != UUcMaxUns32)
		{
			TMtInstanceDescriptor*	originalDesc = originalDescriptorList + curDescIndex;
			TMtInstanceDescriptor*	newDesc = inInstanceFile->instanceDescriptors + deletedRemapArray[curDescIndex];

			UUmAssert(originalDesc->templatePtr == newDesc->templatePtr);
			UUmAssert(originalDesc->dataPtr == newDesc->dataPtr);
		}
	}

	inInstanceFile->numInstanceDescriptors = newInstanceIndex;
	UUrMemory_Array_SetUsedElems(gInstanceDescriptorArray, newInstanceIndex,  NULL);

	UUrMemory_Block_VerifyList();

	/*
	 * Remap the name descriptor indices
	 */
		for(curNameDescIndex = 0, curNameDesc = inInstanceFile->nameDescriptors;
			curNameDescIndex < inInstanceFile->numNameDescriptors;
			curNameDescIndex++, curNameDesc++)
		{
			curNameDesc->instanceDescIndex = deletedRemapArray[curNameDesc->instanceDescIndex];
			UUmAssert(curNameDesc->instanceDescIndex < inInstanceFile->numInstanceDescriptors);

			UUmAssert(inInstanceFile->instanceDescriptors[curNameDesc->instanceDescIndex].namePtr != NULL);
			UUmAssert(strcmp(
						inInstanceFile->instanceDescriptors[curNameDesc->instanceDescIndex].namePtr,
						curNameDesc->namePtr) == 0);
		}

	/*
	 * update the place holder data
	 */
		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->dataPtr == NULL) continue;

			TMmInstanceData_GetPlaceHolder(curDesc->dataPtr) = TMmPlaceHolder_MakeFromIndex(curDescIndex);
		}

	/*
	 * update the place holder data and apply the remap
	 */
		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->dataPtr == NULL) continue;

			swapCodes = curDesc->templatePtr->swapCodes;
			dataPtr = curDesc->dataPtr;

			TMiInstance_Remap_Recursive(&swapCodes, &dataPtr, deletedRemapArray, inInstanceFile);
		}

	UUrMemory_Block_Delete(deletedRemapArray);
	UUrMemory_Block_Delete(originalDescriptorList);

	UUrMemory_Block_VerifyList();

	return UUcError_None;
}

static UUtError
TMiInstanceFile_Save(
	TMtInstanceFile*	inInstanceFile)
{
	UUtError				error;
	BFtFile*				instancePhysicalFile;

	UUtUns32				curDescIndex;
	TMtInstanceDescriptor*	curDesc;

	UUtUns32				curDataFilePos;
	UUtUns32				curNameFilePos;

	UUtUns32				curDataOffset;
	UUtUns32				curNameOffset;

	UUtUns32				curNameLen;

	TMtTemplateDefinition*	templateDefinition;


	TMtInstanceFile_Header	fileHeader;

	TMmInstanceFile_Verify(inInstanceFile);

	UUrMemory_Block_VerifyList();

	if(inInstanceFile->numInstanceDescriptors == 0)
	{
		UUmAssert(inInstanceFile->numNameDescriptors == 0);
		UUmAssert(inInstanceFile->numTemplateDescriptors == 0);
		UUmAssert(inInstanceFile->dataBlockLength == 0);
		UUmAssert(inInstanceFile->nameBlockLength == 0);

		return UUcError_None;
	}

	if(gInstanceDuplicateArray != NULL)
	{
		UUrMemory_Block_Delete(gInstanceDuplicateArray);
	}

	gInstanceDuplicateArray = UUrMemory_Block_New(inInstanceFile->numInstanceDescriptors * sizeof(UUtUns32));
	UUmError_ReturnOnNull(gInstanceDuplicateArray);

	// delete unused instances
		error = TMiInstanceFile_DeleteUnusedInstances(inInstanceFile);
		UUmError_ReturnOnError(error);

	/*
	 * Remove identical instance
	 */
		error = TMiInstanceFile_RemoveIdentical(inInstanceFile);
		UUmError_ReturnOnError(error);

	/*
	 * Prepare all instances for disk
	 */

		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			templateDefinition = curDesc->templatePtr;

			curDesc->templatePtr = (void*)templateDefinition->tag;
		}

	UUrMemory_Block_VerifyList();

	/*
	 * Delete the instance file
	 */
		error = BFrFile_Delete(inInstanceFile->instanceFileRef);
		UUmError_ReturnOnErrorMsg(error, "Could not open instance file for writing");

		error = BFrFile_Create(inInstanceFile->instanceFileRef);
		UUmError_ReturnOnErrorMsg(error, "Could not open instance file for writing");

	/*
	 * open the instance files for writing
	 */
		error = BFrFile_Open(inInstanceFile->instanceFileRef, "w", &instancePhysicalFile);
		UUmError_ReturnOnErrorMsg(error, "Could not open instance file for writing");

	/*
	 * Calculate the new file header
	 */
		fileHeader.version = TMcInstanceFile_Version;
		fileHeader.totalTemplateChecksum = gTemplateChecksum;
		fileHeader.numInstanceDescriptors = inInstanceFile->numInstanceDescriptors;
		fileHeader.numNameDescriptors = inInstanceFile->numNameDescriptors;
		fileHeader.numTemplateDescriptors = inInstanceFile->numTemplateDescriptors;
		fileHeader.dataBlockOffset =
			sizeof(TMtInstanceFile_Header) +
			fileHeader.numInstanceDescriptors * sizeof(TMtInstanceDescriptor) +
			fileHeader.numNameDescriptors * sizeof(TMtNameDescriptor) +
			fileHeader.numTemplateDescriptors * sizeof(TMtTemplateDescriptor);
		fileHeader.dataBlockOffset = UUmMakeMultipleCacheLine(fileHeader.dataBlockOffset);

	/*
	 * Write out the instance data and compute the new offsets
	 */
		curDataOffset = 0;
		curDataFilePos = fileHeader.dataBlockOffset;

		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->flags & TMcDescriptorFlags_Duplicate)
			{
				continue;
			}

			curDataOffset += TMcPreDataBuffer;
			curDataFilePos += TMcPreDataBuffer;

			// First write the data to the new offset
			error =
				BFrFile_WritePos(
					instancePhysicalFile,
					curDataFilePos,
					curDesc->size,
					curDesc->dataPtr);
			UUmError_ReturnOnErrorMsg(error, "Could not write instance data size");

			curDesc->dataPtr = (void*)curDataOffset;
			curDataOffset += curDesc->size;
			curDataFilePos += curDesc->size;

		}

		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->flags & TMcDescriptorFlags_Duplicate)
			{
				curDesc->dataPtr = inInstanceFile->instanceDescriptors[gInstanceDuplicateArray[curDescIndex]].dataPtr;
			}
		}

		UUrMemory_Block_VerifyList();

		fileHeader.dataBlockLength = inInstanceFile->dataBlockLength = curDataOffset;

		curNameOffset = 0;
		fileHeader.nameBlockOffset = curNameFilePos = curDataFilePos;

		for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
			curDescIndex < inInstanceFile->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->namePtr == NULL)
			{
				continue;
			}

			// First write the name to the new offset
			curNameLen = strlen(curDesc->namePtr) + 1;

			error =
				BFrFile_WritePos(
					instancePhysicalFile,
					curNameFilePos,
					curNameLen,
					curDesc->namePtr);
			UUmError_ReturnOnErrorMsg(error, "Could not write instance data size");

			curDesc->namePtr = (char*)curNameOffset;
			curNameOffset += curNameLen;

			curNameFilePos += curNameLen;
		}

		fileHeader.nameBlockLength = inInstanceFile->nameBlockLength = curNameOffset;

	UUrMemory_Block_VerifyList();

	/*
	 * Write out the file header
	 */
		UUmAssert(fileHeader.numNameDescriptors <= fileHeader.numInstanceDescriptors);
		UUmAssert(fileHeader.dataBlockOffset >=
			sizeof(TMtInstanceFile_Header) +
			fileHeader.numInstanceDescriptors * sizeof(TMtInstanceDescriptor) +
			fileHeader.numNameDescriptors * sizeof(TMtNameDescriptor) +
			fileHeader.numTemplateDescriptors * sizeof(TMtTemplateDescriptor));
		UUmAssert(fileHeader.nameBlockOffset >=
			sizeof(TMtInstanceFile_Header) +
			fileHeader.numInstanceDescriptors * sizeof(TMtInstanceDescriptor) +
			fileHeader.numNameDescriptors * sizeof(TMtNameDescriptor) +
			fileHeader.numTemplateDescriptors * sizeof(TMtTemplateDescriptor) +
			fileHeader.dataBlockLength);

		UUmAssert(fileHeader.dataBlockOffset + fileHeader.dataBlockLength <= fileHeader.nameBlockOffset);

		error = BFrFile_WritePos(instancePhysicalFile, 0, sizeof(TMtInstanceFile_Header), &fileHeader);
		UUmError_ReturnOnErrorMsg(error, "Could not write endian detector");

	/*
	 * Write out the instance descriptors
	 */
		error =
			BFrFile_WritePos(
				instancePhysicalFile,
					sizeof(TMtInstanceFile_Header),
					inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor),
					inInstanceFile->instanceDescriptors);
		UUmError_ReturnOnErrorMsg(error, "Could not write instance descriptors");

	/*
	 * Write out the name descriptors
	 */
		error =
			BFrFile_WritePos(
				instancePhysicalFile,
					sizeof(TMtInstanceFile_Header) + inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor),
					inInstanceFile->numNameDescriptors * sizeof(TMtNameDescriptor),
					inInstanceFile->nameDescriptors);
		UUmError_ReturnOnErrorMsg(error, "Could not write name descriptors");

	/*
	 * Write out the template descriptors
	 */
		error =
			BFrFile_WritePos(
				instancePhysicalFile,
					sizeof(TMtInstanceFile_Header) +
						inInstanceFile->numInstanceDescriptors * sizeof(TMtInstanceDescriptor) +
						inInstanceFile->numNameDescriptors * sizeof(TMtNameDescriptor),
					inInstanceFile->numTemplateDescriptors * sizeof(TMtTemplateDescriptor),
					inInstanceFile->templateDescriptors);
		UUmError_ReturnOnErrorMsg(error, "Could not write name descriptors");

	BFrFile_Close(instancePhysicalFile);

	UUrMemory_Block_VerifyList();

	UUrMemory_Pool_Reset(gConstructionPool);

	UUrMemory_Block_Delete(inInstanceFile->templateDescriptors);
	inInstanceFile->templateDescriptors = NULL;

	return TMiInstanceFile_Reload(inInstanceFile, UUcFalse);
}

static void
TMiInstanceFile_Dispose(
	TMtInstanceFile*	inInstanceFile)
{
	TMmInstanceFile_Verify(inInstanceFile);

	BFrFileRef_Dispose(inInstanceFile->instanceFileRef);

	if(gInGame == UUcTrue)
	{
		BFrFile_UnMap(inInstanceFile->mapping);

		if(inInstanceFile->dynamicMemory != NULL)
		{
			UUrMemory_Block_Delete(inInstanceFile->dynamicMemory);
		}
	}
	else
	{
		UUmAssert(inInstanceFile->allocBlock == NULL);

		if(inInstanceFile->dataBlock != NULL)
		{
			UUrMemory_Block_Delete(inInstanceFile->dataBlock);
		}

		if(inInstanceFile->nameBlock != NULL)
		{
			UUrMemory_Block_Delete(inInstanceFile->nameBlock);
		}

		if(inInstanceFile->templateDescriptors != NULL)
		{
			UUrMemory_Block_Delete(inInstanceFile->templateDescriptors);
		}
	}

	UUrMemory_Block_Delete(inInstanceFile);
}

static TMtInstanceDescriptor*
TMiInstanceFile_Instance_FindInstanceDescriptor(
	TMtInstanceFile*	inInstanceFile,
	TMtTemplateTag		inTemplateTag,
	const char*				inName)
{
	UUtUns32				curDescIndex;
	TMtNameDescriptor*		curDesc;

	TMmInstanceFile_Verify(inInstanceFile);

	for(curDescIndex = 0, curDesc = inInstanceFile->nameDescriptors;
		curDescIndex < inInstanceFile->numNameDescriptors;
		curDescIndex++, curDesc++)
	{
		UUmAssert(curDesc->namePtr != NULL);

		if(	curDesc->namePtr[0] == (char)((inTemplateTag >> 24) & 0xFF) &&
			curDesc->namePtr[1] == (char)((inTemplateTag >> 16) & 0xFF) &&
			curDesc->namePtr[2] == (char)((inTemplateTag >> 8) & 0xFF) &&
			curDesc->namePtr[3] == (char)((inTemplateTag >> 0) & 0xFF))
		{
			const char *curDescName = curDesc->namePtr + 4;

			if (0 == strcmp(inName, curDescName))
			{
				return inInstanceFile->instanceDescriptors + curDesc->instanceDescIndex;
			}
		}
	}

	return NULL;
}

#if 0
static void*
TMiInstanceFile_Instance_GetDataPtr_DoesNotWork(
	TMtInstanceFile*	inInstanceFile,
	TMtTemplateTag		inTemplateTag,
	char*				inName)
{
	UUtUns32			lowRange;
	UUtUns32			highRange;
	UUtUns32			midPoint;
	UUtInt16			result;
	TMtNameDescriptor*	curNameDesc;
	TMtInstanceDescriptor*	targetInstanceDesc;

	TMmInstanceFile_Verify(inInstanceFile);

	lowRange = 0;
	highRange = inInstanceFile->numNameDescriptors;

	while(1)
	{
		if(lowRange >= highRange) break;

		midPoint = (lowRange + highRange) >> 1;

		curNameDesc = inInstanceFile->nameDescriptors + midPoint;

		result = strcmp(inName, curNameDesc->namePtr);

		if(result  == 0)
		{
			UUmAssert(curNameDesc->instanceDescIndex < inInstanceFile->numInstanceDescriptors);

			targetInstanceDesc = inInstanceFile->instanceDescriptors + curNameDesc->instanceDescIndex;

			TMmInstanceDesc_Verify(targetInstanceDesc);
			TMmInstanceData_Verify(targetInstanceDesc->dataPtr);

			return targetInstanceDesc->dataPtr;
		}
		else if(result < 0)
		{
			highRange = midPoint;
		}
		else
		{
			lowRange = midPoint;
		}
	}

	return NULL;
}
#endif

static UUtError
TMiInstanceFile_Instance_CreateNewDescriptor(
	TMtInstanceFile*	inInstanceFile,
	TMtTemplateTag		inTemplateTag,
	const char*			inName,
	UUtUns32			*outIndex)
{
	UUtError				error;

	TMtTemplateDefinition*	templateDefinition;
	TMtInstanceDescriptor*	newInstanceDescriptor;

	char*					newNamePtr;

	UUtUns32				newIndex;

	UUtUns32				curNameDescIndex;
	TMtNameDescriptor*		curNameDesc;
	TMtNameDescriptor*		newNameDescriptor;

	TMmInstanceFile_Verify(inInstanceFile);
	UUmAssert(gInGame == UUcFalse);

	UUrMemory_Block_VerifyList();

	*outIndex = 0xFFFF;

	templateDefinition = TMiTemplate_FindDefinition(inTemplateTag);
	if(templateDefinition == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find template definition");
	}

	error = UUrMemory_Array_GetNewElement(gInstanceDescriptorArray, &newIndex, NULL);
	UUmError_ReturnOnErrorMsg(error, "Could not get new index");

	if(newIndex > TMcMaxInstances)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Exceeded maximum number of instances in a file");
	}

	inInstanceFile->instanceDescriptors = UUrMemory_Array_GetMemory(gInstanceDescriptorArray);

	newInstanceDescriptor = inInstanceFile->instanceDescriptors + newIndex;

	if(inName != NULL)
	{
		newNamePtr = UUrMemory_Pool_Block_New(gConstructionPool, strlen(inName) + 5);
		if(newNamePtr == NULL)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not alloc contruction pool");
		}

		strcpy(newNamePtr + 4, inName);
		newNamePtr[0] = (char)((inTemplateTag >> 24) & 0xFF);
		newNamePtr[1] = (char)((inTemplateTag >> 16) & 0xFF);
		newNamePtr[2] = (char)((inTemplateTag >> 8) & 0xFF);
		newNamePtr[3] = (char)((inTemplateTag) & 0xFF);
	}
	else
	{
		newNamePtr = NULL;
	}

	UUrMemory_Block_VerifyList();

	newInstanceDescriptor->templatePtr = templateDefinition;
	newInstanceDescriptor->namePtr = newNamePtr;
	newInstanceDescriptor->flags = inName == NULL ? TMcDescriptorFlags_Unique : TMcDescriptorFlags_None;
	newInstanceDescriptor->checksum = templateDefinition->checksum;
	newInstanceDescriptor->creationDate = UUrGetSecsSince1900();
	newInstanceDescriptor->dataPtr = NULL;
	newInstanceDescriptor->size = 0;

	newInstanceDescriptor->flags |= TMcDescriptorFlags_Touched;

	UUrMemory_Block_VerifyList();

	if(inName != NULL)
	{
		for(curNameDescIndex = 0, curNameDesc = inInstanceFile->nameDescriptors;
			curNameDescIndex < inInstanceFile->numNameDescriptors;
			curNameDescIndex++, curNameDesc++)
		{
			if(strcmp(newNamePtr, curNameDesc->namePtr) < 0)
			{
				break;
			}
		}

		UUrMemory_Block_VerifyList();

		error = UUrMemory_Array_InsertElement(gNameDescriptorArray, curNameDescIndex, NULL);
		if(error != UUcError_None)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not insert into name desc array");
		}
		inInstanceFile->nameDescriptors = UUrMemory_Array_GetMemory(gNameDescriptorArray);
		newNameDescriptor = inInstanceFile->nameDescriptors + curNameDescIndex;

		newNameDescriptor->namePtr = newInstanceDescriptor->namePtr;
		newNameDescriptor->instanceDescIndex = newIndex;

		inInstanceFile->numNameDescriptors++;
	}

	inInstanceFile->numInstanceDescriptors++;

	*outIndex = newIndex;

	return UUcError_None;
}

static UUtError
TMiInstanceFile_Instance_CreatePlaceHolder(
	TMtInstanceFile*	inInstanceFile,
	TMtTemplateTag		inTemplateTag,
	const char*			inName,
	TMtPlaceHolder		*outPlaceHolder)
{
	UUtError	error;
	UUtUns32	newIndex;
	TMtInstanceDescriptor*	targetDesc;

	TMmInstanceFile_Verify(inInstanceFile);

	error =
		TMiInstanceFile_Instance_CreateNewDescriptor(
			inInstanceFile,
			inTemplateTag,
			inName,
			&newIndex);
	UUmError_ReturnOnError(error);

	*outPlaceHolder = TMmPlaceHolder_MakeFromIndex(newIndex);

	targetDesc = TMrPlaceHolder_GetInstanceDesc(inInstanceFile, *outPlaceHolder);
	targetDesc->flags |= TMcDescriptorFlags_PlaceHolder;

	return UUcError_None;
}

static void*
TMiInstanceFile_Instance_Create(
	TMtInstanceFile*	inInstanceFile,
	TMtTemplateTag		inTemplateTag,
	const char*			inName,
	UUtUns32			inVarArrayLength)
{
	UUtError				error;
	TMtTemplateDefinition*	templateDefinition;
	TMtInstanceDescriptor*	newInstanceDescriptor;

	void*					newDataPtr;
	UUtUns32				dataSize;

	UUtUns32				newDescIndex;

	TMmInstanceFile_Verify(inInstanceFile);

	templateDefinition = TMiTemplate_FindDefinition(inTemplateTag);
	if(templateDefinition == NULL)
	{
		UUrError_Report(UUcError_Generic, "Could not find template definition");
		return NULL;
	}

	newInstanceDescriptor = NULL;

	if(inName != NULL)
	{
		/*
		 * First check to see if instance already exists and is a place holder
		 */
			newInstanceDescriptor =
				TMiInstanceFile_Instance_FindInstanceDescriptor(
					inInstanceFile,
					inTemplateTag,
					inName);
			if(newInstanceDescriptor != NULL)
			{
				if(!(newInstanceDescriptor->flags & TMcDescriptorFlags_PlaceHolder))
				{
					UUrError_Report(UUcError_Generic, "An instance already exists with this name");
					return NULL;
				}
				// Replace the place holder
				newInstanceDescriptor->flags &= (TMtDescriptorFlags)~TMcDescriptorFlags_PlaceHolder;

				newDescIndex = newInstanceDescriptor - inInstanceFile->instanceDescriptors;
			}
	}

	if(newInstanceDescriptor == NULL)
	{
		error =
			TMiInstanceFile_Instance_CreateNewDescriptor(
				inInstanceFile,
				inTemplateTag,
				inName,
				&newDescIndex);
		if(error != UUcError_None)
		{
			UUrError_Report(UUcError_Generic, "Could not create descriptor");
			return NULL;
		}

		newInstanceDescriptor = inInstanceFile->instanceDescriptors + newDescIndex;
	}

	dataSize = templateDefinition->size + inVarArrayLength * templateDefinition->varArrayElemSize;

	dataSize = (dataSize + UUcProcessor_CacheLineSize_Mask) & ~UUcProcessor_CacheLineSize_Mask;

	newDataPtr = UUrMemory_Pool_Block_New(gConstructionPool, dataSize + TMcPreDataBuffer);
	if(newDataPtr == NULL)
	{
		UUmAssert(!"Out of memory");
		return NULL;
	}

	UUrMemory_Set32(newDataPtr, 0xDEADDEAD, dataSize + TMcPreDataBuffer);

	UUmAssertAlignedPtr(newDataPtr);

	newDataPtr = (char*)newDataPtr + TMcPreDataBuffer;

	UUmAssertReadPtr(newDataPtr, dataSize);

	TMmInstanceData_GetPlaceHolder(newDataPtr) = TMmPlaceHolder_MakeFromIndex(newDescIndex);
	TMmInstanceData_GetInstanceFile(newDataPtr) = inInstanceFile;
	TMmInstanceData_GetMagicCookie(newDataPtr) = TMcMagicCookie;
	TMmInstanceData_GetTemplateDefinition(newDataPtr) = templateDefinition;

	newInstanceDescriptor->checksum = templateDefinition->checksum;
	newInstanceDescriptor->size = dataSize;
	newInstanceDescriptor->dataPtr = newDataPtr;

	{
		UUtUns8*	dataPtr	= newDataPtr;
		UUtUns8*	swapCodes = newInstanceDescriptor->templatePtr->swapCodes;

		TMiInstance_VarArray_Reset_Recursive(
			&swapCodes,
			&dataPtr,
			inVarArrayLength);
	}

	return newDataPtr;
}

static UUtError
TMiInstanceFile_Instance_Clean(
	TMtInstanceFile*	inInstanceFile,
	TMtTemplateTag		inTemplateTag,
	const char*			inName)
{
	UUtUns8*				swapCode;
	UUtUns8*				dataPtr;

	TMtInstanceDescriptor*	targetDesc;

	TMmInstanceFile_Verify(inInstanceFile);

	targetDesc = TMiInstanceFile_Instance_FindInstanceDescriptor(inInstanceFile, inTemplateTag, inName);
	if(targetDesc == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find descriptor");
	}

	if(targetDesc->dataPtr == NULL)
	{
		return UUcError_None;
	}

	targetDesc->flags |= TMcDescriptorFlags_Touched;

	swapCode = targetDesc->templatePtr->swapCodes;
	dataPtr = targetDesc->dataPtr;

	TMiInstance_Clean_Recursive(
		&swapCode,
		&dataPtr,
		inInstanceFile);

	return UUcError_None;
}

static void*
TMiInstanceFile_Instance_ResizeVarArray(
	void*		inDataPtr,
	UUtUns32	inVarArrayLength)
{
	TMtInstanceDescriptor*	instanceDesc;
	UUtUns32				newSize;
	void*					newDataPtr;

	TMmInstanceData_Verify(inDataPtr);

	instanceDesc = TMrPlaceHolder_GetInstanceDesc(gConstructionFile, (TMtPlaceHolder)inDataPtr);
	UUmAssert(instanceDesc != NULL);
	TMmInstanceDesc_Verify(instanceDesc);

	newSize = instanceDesc->templatePtr->size +
		instanceDesc->templatePtr->varArrayElemSize * inVarArrayLength;

	newDataPtr = UUrMemory_Pool_Block_New(gConstructionPool, newSize + TMcPreDataBuffer);

	if(newDataPtr == NULL)
	{
		return NULL;
	}

	newDataPtr = (char*)newDataPtr + TMcPreDataBuffer;

	TMmInstanceData_GetInstanceFile(newDataPtr) = TMmInstanceData_GetInstanceFile(instanceDesc->dataPtr);
	TMmInstanceData_GetPlaceHolder(newDataPtr) = TMmInstanceData_GetPlaceHolder(instanceDesc->dataPtr);
	TMmInstanceData_GetMagicCookie(newDataPtr) = TMcMagicCookie;
	TMmInstanceData_GetTemplateDefinition(newDataPtr) = instanceDesc->templatePtr;

	instanceDesc->dataPtr = newDataPtr;

	{
		UUtUns8*	dataPtr	= newDataPtr;
		UUtUns8*	swapCodes = instanceDesc->templatePtr->swapCodes;

		TMiInstance_VarArray_Reset_Recursive(
			&swapCodes,
			&dataPtr,
			inVarArrayLength);
	}

	return newDataPtr;
}

static void
TMiInstanceFile_Instance_Touch(
	TMtInstanceFile*		inInstanceFile,
	TMtInstanceDescriptor*	inInstanceDesc)
{
	UUtUns8*				swapCodes;
	UUtUns8*				dataPtr;

	TMmInstanceFile_Verify(inInstanceFile);
	TMmInstanceDesc_Verify(inInstanceDesc);

	if(inInstanceDesc->dataPtr != NULL)
	{
		swapCodes = inInstanceDesc->templatePtr->swapCodes;
		dataPtr = inInstanceDesc->dataPtr;

		TMiInstance_Touch_Recursive(
			&swapCodes,
			&dataPtr,
			gConstructionFile);
	}
	inInstanceDesc->flags |= TMcDescriptorFlags_Touched;
}

#if defined(DEBUGGING) && DEBUGGING
static void
TMiInstanceFile_VerifyAllTouched(
	TMtInstanceFile*	inInstanceFile)
{
	UUtUns32				curDescIndex;

	TMtInstanceDescriptor*	curInstDesc;

	UUtUns8*				swapCodes;
	UUtUns8*				dataPtr;

	for(
		curDescIndex = 0, curInstDesc = inInstanceFile->instanceDescriptors;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curInstDesc++)
	{
		curInstDesc->flags &= (TMtDescriptorFlags)~TMcDescriptorFlags_Touched;
	}

	for(
		curDescIndex = 0, curInstDesc = inInstanceFile->instanceDescriptors;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curInstDesc++)
	{
		if((curDescIndex % 100) == 0) fputc('.', stderr);

		if(curInstDesc->flags & (TMcDescriptorFlags_Unique | TMcDescriptorFlags_PlaceHolder))
		{
			continue;
		}

		swapCodes = curInstDesc->templatePtr->swapCodes;
		dataPtr = curInstDesc->dataPtr;

		if(dataPtr == NULL) continue;

		TMiInstance_Touch_Recursive(
			&swapCodes,
			&dataPtr,
			inInstanceFile);
	}
	fputc('\n', stderr);

	for(
		curDescIndex = 0, curInstDesc = inInstanceFile->instanceDescriptors;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curInstDesc++)
	{
		static UUtBool ignore = UUcFalse;

		//if(curInstDesc->flags & TMcDescriptorFlags_PlaceHolder) continue;
		UUmAssert(!(curInstDesc->flags & TMcDescriptorFlags_DeleteMe));
		if(!(curInstDesc->flags & TMcDescriptorFlags_Unique)) continue;

		if(curInstDesc->flags & TMcDescriptorFlags_Touched) continue;

		if (!ignore) {
			AUtMB_ButtonChoice button;

			button = AUrMessageBox(AUcMBType_OKCancel, "A unique instance or place holder (template = %s) (instance = %s) has not been touched.",
						curInstDesc->templatePtr->name, (curInstDesc->namePtr != NULL) ? curInstDesc->namePtr : "no instance name");

			if (AUcMBChoice_Cancel == button) {
				ignore = UUcTrue;
			}
		}
	}

}


#endif

UUtError
TMrRegisterTemplates(
	void)
{

	TMrTemplate_Register(TMcTemplate_IndexArray, sizeof(TMtIndexArray), TMcFolding_Allow);
	TMrTemplate_Register(TMcTemplate_LongIndexArray, sizeof(TMtLongIndexArray), TMcFolding_Allow);
	TMrTemplate_Register(TMcTemplate_TemplateRefArray, sizeof(TMtTemplateRefArray), TMcFolding_Allow);
	TMrTemplate_Register(TMcTemplate_FloatArray, sizeof(TMtFloatArray), TMcFolding_Allow);

	return UUcError_None;
}

/*
 * Initialize the template manager
 *	IN
 *		inGame				- UUcTrue if we are running the game
 *		inGameDataFolderRef	- A folder reference for the game data
 */
UUtError
TMrInitialize(
	UUtBool			inGame,
	BFtFileRef*		inGameDataFolderRef)
{
	UUtError				error;

	gInGame = inGame;

	UUmAssert(sizeof(TMtInstanceFile_Header) == UUcProcessor_CacheLineSize * 2);

	/*
	 * Get the folder ref
	 */
		error = BFrFileRef_Duplicate(inGameDataFolderRef, &TMgDataFolderRef);
		UUmError_ReturnOnErrorMsg(error, "Could not duplicate game data file ref");

	TMrTemplate_BuildList();


	if(inGame == UUcTrue)
	{
		/*
		 * Allocate dynamic instance pools
		 */
		TMgPermPool = UUrMemory_Pool_New(TMcPermPoolChunkSize, UUcPool_Fixed);
		if(TMgPermPool == NULL)
		{
			UUrError_Report(UUcError_OutOfMemory, "could not create perm memory pool");
			return UUcError_OutOfMemory;
		}

		TMgTempPool = UUrMemory_Pool_New(TMcTempPoolChunkSize, UUcPool_Fixed);
		if(TMgTempPool == NULL)
		{
			UUrError_Report(UUcError_OutOfMemory, "could not create Temp memory pool");
			return UUcError_OutOfMemory;
		}

		/*
		 * Initialize the dynamic instance fies
		 */
			TMgPermInstanceFile.instanceFileRef			= NULL;
			TMgPermInstanceFile.numInstanceDescriptors	= 0;
			TMgPermInstanceFile.numNameDescriptors		= 0;
			TMgPermInstanceFile.allocBlock				= NULL;
			TMgPermInstanceFile.dataBlockLength			= 0;
			TMgPermInstanceFile.dataBlock				= NULL;
			TMgPermInstanceFile.nameBlockLength			= 0;
			TMgPermInstanceFile.nameBlock				= NULL;
			TMgPermInstanceFile.dynamicMemory			= NULL;

			TMgTempInstanceFile.instanceFileRef			= NULL;
			TMgTempInstanceFile.numInstanceDescriptors	= 0;
			TMgTempInstanceFile.numNameDescriptors		= 0;
			TMgTempInstanceFile.allocBlock				= NULL;
			TMgTempInstanceFile.dataBlockLength			= 0;
			TMgTempInstanceFile.dataBlock				= NULL;
			TMgTempInstanceFile.nameBlockLength			= 0;
			TMgTempInstanceFile.nameBlock				= NULL;
			TMgTempInstanceFile.dynamicMemory			= NULL;

			#if defined(DEBUGGING) && DEBUGGING

				TMgPermInstanceFile.magicCookie = TMcMagicCookie;
				TMgTempInstanceFile.magicCookie = TMcMagicCookie;

			#endif

			TMgPermDescriptorArray =
				UUrMemory_Array_New(
					sizeof(TMtInstanceDescriptor),
					20,
					0,
					10);
			if(TMgPermDescriptorArray == NULL)
			{
				UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not alloc perm desc array");
			}

			TMgPermInstanceFile.instanceDescriptors = UUrMemory_Array_GetMemory(TMgPermDescriptorArray);

			TMgTempDescriptorArray =
				UUrMemory_Array_New(
					sizeof(TMtInstanceDescriptor),
					20,
					0,
					10);
			if(TMgPermDescriptorArray == NULL)
			{
				UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not alloc perm desc array");
			}

			TMgTempInstanceFile.instanceDescriptors = UUrMemory_Array_GetMemory(TMgTempDescriptorArray);
	}
	else
	{
		gConstructionPool = UUrMemory_Pool_New(TMcConstructionPoolChunkSize, UUcPool_Growable);
		UUmError_ReturnOnNull(gConstructionPool);
	}

	return UUcError_None;
}

/*
 * Terminate the template manager
 */
void
TMrTerminate(
	void)
{
	UUtUns32	i;

	if(gInGame == UUcTrue)
	{

		// TMiInstanceFile_CallProcs(&TMgTempInstanceFile, TMcTemplateProcMessage_DisposePreProcess);
		UUrMemory_Array_Delete(TMgPermDescriptorArray);
		TMgPermDescriptorArray = NULL;
		UUrMemory_Array_Delete(TMgTempDescriptorArray);
		TMgTempDescriptorArray = NULL;

	}

	if(gTemplateDefinitionsAlloced != NULL)
	{

		for(i = 0; i < TMgNumTemplateDefinitions; i++)
		{
			if(TMgTemplateDefinitionArray[i].swapCodes != NULL)
			{
				UUrMemory_Block_Delete(TMgTemplateDefinitionArray[i].swapCodes);
			}
		}

		UUrMemory_Block_Delete(gTemplateDefinitionsAlloced);
		gTemplateDefinitionsAlloced = NULL;
	}

	if(gTemplateNameAlloced != NULL)
	{
		UUrMemory_String_Delete(gTemplateNameAlloced);
	}

	if(TMgTemplateNameData != NULL)
	{
		UUrMemory_Block_Delete(TMgTemplateNameData);
	}

	if(TMgTemplateData != NULL)
	{
		UUrMemory_Block_Delete(TMgTemplateData);
	}

	if(TMgPermPool != NULL)
	{
		UUrMemory_Pool_Delete(TMgPermPool);
		TMgPermPool = NULL;
	}

	if(TMgTempPool != NULL)
	{
		UUrMemory_Pool_Delete(TMgTempPool);
		TMgTempPool = NULL;
	}

	if(TMgDataFolderRef != NULL)
	{
		BFrFileRef_Dispose(TMgDataFolderRef);
	}

	TMgTemplateData = NULL;
	TMgTemplateNameData = NULL;
	TMgNumTemplateDefinitions = 0;

	UUmAssert(gConstructionFile == NULL);

	if(gInGame == UUcFalse)
	{
		if(gInstanceDescriptorArray != NULL)
		{
			UUrMemory_Array_Delete(gInstanceDescriptorArray);
			gInstanceDescriptorArray = NULL;
		}

		if(gNameDescriptorArray != NULL)
		{
			UUrMemory_Array_Delete(gNameDescriptorArray);
			gNameDescriptorArray = NULL;
		}

		if(gConstructionPool != NULL)
		{
			UUrMemory_Pool_Delete(gConstructionPool);
			gConstructionPool = NULL;
		}

		if(gInstanceDuplicateArray != NULL)
		{
			UUrMemory_Block_Delete(gInstanceDuplicateArray);
		}
	}
}

#define TMcMameAndDestroy_Garb_Size (1024)

UUtError
TMrMameAndDestroy(
	void)
{
	UUtError			error;
	BFtFileIterator*	fileIterator;
	BFtFileRef*			datFileRef;
	BFtFile*			datFile;
	void*				garbage;

	error =
		BFrDirectory_FileIterator_New(
			TMgDataFolderRef,
			"level",
			".dat",
			&fileIterator);
	UUmError_ReturnOnErrorMsg(error, "Could not create file iterator");

	garbage = UUrMemory_Block_New(TMcMameAndDestroy_Garb_Size);
	UUmError_ReturnOnNull(garbage);

	UUrMemory_Set32(garbage, 0xDEAD, TMcMameAndDestroy_Garb_Size);

	while(1)
	{
		error = BFrDirectory_FileIterator_Next(fileIterator, &datFileRef);
		if(error != UUcError_None)
		{
			break;
		}

		error = BFrFile_Open(datFileRef, "w", &datFile);

		if(error == UUcError_None)
		{
			BFrFile_Write(datFile, TMcMameAndDestroy_Garb_Size, garbage);
			BFrFile_Close(datFile);
		}

		BFrFileRef_Dispose(datFileRef);
	}

	BFrDirectory_FileIterator_Delete(fileIterator);

	UUrMemory_Block_Delete(garbage);

	return UUcError_None;
}

/*
 * Register a template.
 *	IN
 *		inTemplateTag - The template to be registered
 *		inSize - The expected size
 *	NOTES
 *		This is important since the compiler can pad structures in unexpected ways...
 */
UUtError
TMrTemplate_Register(
	TMtTemplateTag		inTemplateTag,
	UUtUns32			inSize,
	TMtAllowFolding		inAllowFolding)
{
	TMtTemplateDefinition	*curTemplateDefinition;

	curTemplateDefinition = TMiTemplate_FindDefinition(inTemplateTag);

	if(curTemplateDefinition == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "template does not exist");
	}

	if(curTemplateDefinition->flags & TMcTemplateFlag_Registered)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "template already registered");
	}

	if(curTemplateDefinition->size + curTemplateDefinition->varArrayElemSize != inSize)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "size mismatch, either you need to run the extractor or structure padding is wrong");
	}

	curTemplateDefinition->flags |= TMcTemplateFlag_Registered;

	if(inAllowFolding == TMcFolding_Allow)
	{
		curTemplateDefinition->flags |= TMcTemplateFlag_AllowFolding;
	}

	curTemplateDefinition->handler = NULL;

	return UUcError_None;
}

/*
 * This is used to add custom runtime data to a template and specify a runtime proc
 * to receive template messages
 */
UUtError
TMrTemplate_InstallPrivateData(
	TMtTemplateTag			inTemplateTag,
	UUtUns32				inDynamicSize,
	TMtTemplateProc_Handler	inProcHandler)
{
	TMtTemplateDefinition*	templatePtr;

	templatePtr = TMiTemplate_FindDefinition(inTemplateTag);
	if(templatePtr == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find template");
	}

	if(templatePtr->handler != NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "A handler is already installed");
	}

	templatePtr->privateDataSize = inDynamicSize;

	templatePtr->handler = inProcHandler;

	return UUcError_None;
}

UUtError
TMrTemplate_InstallByteSwap(
	TMtTemplateTag				inTemplateTag,
	TMtTemplateProc_ByteSwap	inProc)
{
	TMtTemplateDefinition*	templatePtr;

	templatePtr = TMiTemplate_FindDefinition(inTemplateTag);
	if(templatePtr == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find template");
	}

	UUmAssert(templatePtr->varArrayElemSize > 0);

	templatePtr->byteSwapProc = inProc;

	return UUcError_None;
}

/*
 * Call a handler function for every instance of the given type
 */
UUtError
TMrTemplate_CallProc(
	TMtTemplateTag				inTemplateTag,
	TMtTemplateProc_Message		inMessage)
{
	UUtUns32			curFileIndex;

	for(curFileIndex = 0; curFileIndex < TMgNumLevel0Files; curFileIndex++)
	{
		if(TMgLevel0Files[curFileIndex] != NULL)
		{
			TMiInstanceFile_CallProcs(TMgLevel0Files[curFileIndex], inMessage);
		}
	}

	for(curFileIndex = 0; curFileIndex < TMgNumCurLevelFiles; curFileIndex++)
	{
		if(TMgCurLevelFiles[curFileIndex] != NULL)
		{
			TMiInstanceFile_CallProcs(TMgCurLevelFiles[curFileIndex], inMessage);
		}
	}

	return UUcError_None;
}

/*
 * Get a pointer to the requested instance
 *	IN
 *		inTemplateTag - The template
 *		inInstanceTag - The instance
 *	OUT
 *		outDataPtr - The pointer to the data
 */
UUtError
TMrInstance_GetDataPtr(
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName,
	void*				*outDataPtr)
{
	UUtUns32				curFileIndex;
	TMtInstanceDescriptor*	targetInstDesc;
	TMtTemplateDefinition*	templatePtr;

	UUmAssert(gInGame == UUcTrue);
	UUmAssert(gConstructionFile == NULL);

	*outDataPtr = NULL;

	templatePtr = TMiTemplate_FindDefinition(inTemplateTag);
	if(templatePtr == NULL)
	{
		UUrDebuggerMessage(
			"Could not find template %c%c%c%c for instance %s",
			(inTemplateTag >> 24) & 0xFF,
			(inTemplateTag >> 16) & 0xFF,
			(inTemplateTag >> 8) & 0xFF,
			(inTemplateTag >> 0) & 0xFF,
			inInstanceName);
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find template definition");
	}

	// First look in non final current level instance finals
	for(curFileIndex = 0; curFileIndex < TMgNumCurLevelFiles; curFileIndex++)
	{
		if(TMgCurLevelFiles[curFileIndex]->final == UUcTrue) continue;

		targetInstDesc =
			TMiInstanceFile_Instance_FindInstanceDescriptor(
				TMgCurLevelFiles[curFileIndex],
				inTemplateTag,
				inInstanceName);

		if(targetInstDesc != NULL && targetInstDesc->dataPtr != NULL)
		{
			TMmInstanceData_Verify(targetInstDesc->dataPtr);

			*outDataPtr = targetInstDesc->dataPtr;
			return UUcError_None;
		}
	}

	// Look in final current level instance finals
	for(curFileIndex = 0; curFileIndex < TMgNumCurLevelFiles; curFileIndex++)
	{
		if(TMgCurLevelFiles[curFileIndex]->final == UUcFalse) continue;

		targetInstDesc =
			TMiInstanceFile_Instance_FindInstanceDescriptor(
				TMgCurLevelFiles[curFileIndex],
				inTemplateTag,
				inInstanceName);

		if(targetInstDesc != NULL && targetInstDesc->dataPtr != NULL)
		{
			TMmInstanceData_Verify(targetInstDesc->dataPtr);

			*outDataPtr = targetInstDesc->dataPtr;
			return UUcError_None;
		}
	}

	// First look in non final level 0 instance finals
	for(curFileIndex = 0; curFileIndex < TMgNumLevel0Files; curFileIndex++)
	{
		if(TMgLevel0Files[curFileIndex]->final == UUcTrue) continue;

		targetInstDesc =
			TMiInstanceFile_Instance_FindInstanceDescriptor(
				TMgLevel0Files[curFileIndex],
				inTemplateTag,
				inInstanceName);
		if(targetInstDesc != NULL && targetInstDesc->dataPtr != NULL)
		{

			TMmInstanceData_Verify(targetInstDesc->dataPtr);

			*outDataPtr = targetInstDesc->dataPtr;
			return UUcError_None;
		}
	}

	// Look in final level 0 instance finals
	for(curFileIndex = 0; curFileIndex < TMgNumLevel0Files; curFileIndex++)
	{
		if(TMgLevel0Files[curFileIndex]->final == UUcFalse) continue;

		targetInstDesc =
			TMiInstanceFile_Instance_FindInstanceDescriptor(
				TMgLevel0Files[curFileIndex],
				inTemplateTag,
				inInstanceName);
		if(targetInstDesc != NULL && targetInstDesc->dataPtr != NULL)
		{

			TMmInstanceData_Verify(targetInstDesc->dataPtr);

			*outDataPtr = targetInstDesc->dataPtr;
			return UUcError_None;
		}
	}

	return UUcError_Generic;
}

UUtError
TMrInstance_GetDataPtr_List(
	TMtTemplateTag		inTemplateTag,
	UUtUns32			inMaxPtrs,
	UUtUns32			*outNumPtrs,
	void*				*outPtrList)
{
	UUtUns32				curFileIndex;
	TMtInstanceDescriptor*	curDesc;
	TMtTemplateDefinition*	templatePtr;
	UUtUns32				curDescIndex;

	UUtUns32				numPtrs;

	UUmAssert(gInGame == UUcTrue);
	UUmAssert(gConstructionFile == NULL);

	numPtrs = 0;

	templatePtr = TMiTemplate_FindDefinition(inTemplateTag);
	if(templatePtr == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find template definition");
	}

	// First look in the non final files
	for(curFileIndex = 0; curFileIndex < TMgNumCurLevelFiles; curFileIndex++)
	{
		UUmAssertReadPtr(TMgCurLevelFiles[curFileIndex], sizeof(TMtInstanceFile));

		if(TMgCurLevelFiles[curFileIndex]->final == UUcTrue) continue;

		for(curDescIndex = 0, curDesc = TMgCurLevelFiles[curFileIndex]->instanceDescriptors;
			curDescIndex < TMgCurLevelFiles[curFileIndex]->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->templatePtr == templatePtr && curDesc->namePtr != NULL && curDesc->dataPtr != NULL)
			{
				if(numPtrs < inMaxPtrs)
				{
					if (NULL != outPtrList)
					{
						outPtrList[numPtrs] = curDesc->dataPtr;
					}

					numPtrs++;
				}
			}
		}
	}

	// next look in the final files
	for(curFileIndex = 0; curFileIndex < TMgNumCurLevelFiles; curFileIndex++)
	{
		if(TMgCurLevelFiles[curFileIndex]->final == UUcFalse) continue;

		for(curDescIndex = 0, curDesc = TMgCurLevelFiles[curFileIndex]->instanceDescriptors;
			curDescIndex < TMgCurLevelFiles[curFileIndex]->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->templatePtr == templatePtr && curDesc->namePtr != NULL && curDesc->dataPtr != NULL)
			{
				if(numPtrs < inMaxPtrs)
				{
					if (NULL != outPtrList)
					{
						outPtrList[numPtrs] = curDesc->dataPtr;
					}

					numPtrs++;
				}
			}
		}
	}

	for(curFileIndex = 0; curFileIndex < TMgNumLevel0Files; curFileIndex++)
	{
		if(TMgLevel0Files[curFileIndex]->final == UUcTrue) continue;

		for(curDescIndex = 0, curDesc = TMgLevel0Files[curFileIndex]->instanceDescriptors;
			curDescIndex < TMgLevel0Files[curFileIndex]->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->templatePtr == templatePtr && curDesc->namePtr != NULL && curDesc->dataPtr != NULL)
			{
				if(numPtrs < inMaxPtrs)
				{
					if (NULL != outPtrList)
					{
						outPtrList[numPtrs] = curDesc->dataPtr;
					}

					numPtrs++;
				}
			}
		}
	}

	for(curFileIndex = 0; curFileIndex < TMgNumLevel0Files; curFileIndex++)
	{
		if(TMgLevel0Files[curFileIndex]->final == UUcFalse) continue;

		for(curDescIndex = 0, curDesc = TMgLevel0Files[curFileIndex]->instanceDescriptors;
			curDescIndex < TMgLevel0Files[curFileIndex]->numInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->templatePtr == templatePtr && curDesc->namePtr != NULL && curDesc->dataPtr != NULL)
			{
				if(numPtrs < inMaxPtrs)
				{
					if (NULL != outPtrList)
					{
						outPtrList[numPtrs] = curDesc->dataPtr;
					}

					numPtrs++;
				}
			}
		}
	}

	*outNumPtrs = numPtrs;

	return UUcError_None;
}

// this is bogus, fix at some point
UUtError
TMrInstance_GetDataPtr_ByNumber(
	TMtTemplateTag		inTemplateTag,
	UUtUns32			inNumber,
	void*				*outDataPtr)
{
	const UUtUns32 tempMemorySize = 10000;
	UUtError error = UUcError_None;
	UUtUns32 numPointers;
	void **tempMemory = NULL;

	*outDataPtr = NULL;

	tempMemory = UUrMemory_Block_New(tempMemorySize * sizeof(void *));
	UUmError_ReturnOnNull(tempMemory);

	error = TMrInstance_GetDataPtr_List(inTemplateTag, tempMemorySize, &numPointers, tempMemory);

	if (UUcError_None != error) {
		goto exit;
	}

	if (inNumber >= numPointers) {
		error = UUcError_Generic;
		goto exit;
	}

	*outDataPtr = tempMemory[inNumber];

exit:
	if (NULL != tempMemory) {
		UUrMemory_Block_Delete(tempMemory);
	}

	return error;
}

UUtUns32
TMrInstance_GetTagCount(
	TMtTemplateTag		inTemplateTag)
{
	UUtUns32 numPointers;
	UUtError error;

	error = TMrInstance_GetDataPtr_List(inTemplateTag, TMcMaxInstances, &numPointers, NULL);
	UUmAssert(UUcError_None == error);

	return numPointers;
}

/*
 * This is used to create a runtime instance
 */
UUtError
TMrInstance_Dynamic_New(
	TMtTempSpace		inTempSpace,
	TMtTemplateTag		inTemplateTag,
	UUtUns32			inInitialVarArrayLength,
	void*				*outDataPtr)
{
	UUtError				error;
	TMtTemplateDefinition*	templatePtr;
	UUtUns32				size;
	void*					newDataPtr;
	TMtInstanceDescriptor*	newDesc;
	UUtUns32				newIndex;
	void*					newDynamicMemory;

	UUmAssert(gInGame == UUcTrue);
	UUmAssert(gConstructionFile == NULL);
	UUmAssert((TMcTemporarySpace == inTempSpace) || (TMcPermanentSpace == inTempSpace));

	templatePtr = TMiTemplate_FindDefinition(inTemplateTag);
	if(templatePtr == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find template");
	}

	newDynamicMemory = NULL;

	size = templatePtr->size + templatePtr->varArrayElemSize * inInitialVarArrayLength;

	if(inTempSpace == TMcTemporarySpace)
	{
		newDataPtr = UUrMemory_Pool_Block_New(TMgTempPool, size + TMcPreDataBuffer);

		error = UUrMemory_Array_MakeRoom(TMgTempDescriptorArray, TMgTempInstanceFile.numInstanceDescriptors + 1, NULL);
		UUmError_ReturnOnErrorMsg(error, "Could not make room");

		TMgTempInstanceFile.instanceDescriptors = UUrMemory_Array_GetMemory(TMgTempDescriptorArray);
		newDesc = TMgTempInstanceFile.instanceDescriptors + TMgTempInstanceFile.numInstanceDescriptors;
		newIndex = TMgTempInstanceFile.numInstanceDescriptors++;

		if(templatePtr->privateDataSize != 0)
		{
			newDynamicMemory = UUrMemory_Pool_Block_New(TMgTempPool, templatePtr->privateDataSize);
			if(newDynamicMemory == NULL)
			{
				UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not allocate dynamic memory");
			}
		}
	}
	else
	{
		newDataPtr = UUrMemory_Pool_Block_New(TMgPermPool, size + TMcPreDataBuffer);

		error = UUrMemory_Array_MakeRoom(TMgPermDescriptorArray, TMgPermInstanceFile.numInstanceDescriptors + 1, NULL);
		UUmError_ReturnOnErrorMsg(error, "Could not make room");

		TMgPermInstanceFile.instanceDescriptors = UUrMemory_Array_GetMemory(TMgPermDescriptorArray);
		newDesc = TMgPermInstanceFile.instanceDescriptors + TMgPermInstanceFile.numInstanceDescriptors;
		newIndex = TMgPermInstanceFile.numInstanceDescriptors++;

		if(templatePtr->privateDataSize != 0)
		{
			newDynamicMemory = UUrMemory_Pool_Block_New(TMgPermPool, templatePtr->privateDataSize);
			if(newDynamicMemory == NULL)
			{
				UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not allocate dynamic memory");
			}
		}
	}

	if(newDataPtr == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "could not allocate dynamic instance");
	}

	newDataPtr = (char*)newDataPtr + TMcPreDataBuffer;

	newDesc->templatePtr = templatePtr;
	newDesc->dataPtr = newDataPtr;
	newDesc->namePtr = NULL;
	newDesc->size = size;
	newDesc->checksum = templatePtr->checksum;

	TMmInstance_GetDynamicData(newDataPtr) = newDynamicMemory;

	TMmInstanceData_GetInstanceFile(newDataPtr) = (inTempSpace == TMcTemporarySpace) ? &TMgTempInstanceFile : &TMgPermInstanceFile;
	TMmInstanceData_GetPlaceHolder(newDataPtr) = TMmPlaceHolder_MakeFromIndex(newIndex);
	TMmInstanceData_GetMagicCookie(newDataPtr) = TMcMagicCookie;
	TMmInstanceData_GetTemplateDefinition(newDataPtr) = templatePtr;

	/*
	 * Set the variable length array
	 */
	{
		UUtUns8*	swapCode = templatePtr->swapCodes;
		UUtUns8*	dataPtr = (UUtUns8 *)newDataPtr;

		TMiInstance_VarArray_Reset_Recursive(
			&swapCode,
			&dataPtr,
			inInitialVarArrayLength);
	}

	if(templatePtr->handler != NULL)
	{
		error = templatePtr->handler(TMcTemplateProcMessage_NewPostProcess, templatePtr->tag, newDataPtr);
		UUmError_ReturnOnErrorMsg(error, "Could not new post process");
	}

	*outDataPtr = newDataPtr;

	return UUcError_None;
}

/*
 * This is called when an instances data has been dynamically changed
 */
UUtError
TMrInstance_Update(
	void*				inDataPtr)
{
	TMtTemplateDefinition*	templatePtr;

	TMmInstanceData_Verify(inDataPtr);

	templatePtr = TMmInstanceData_GetTemplateDefinition(inDataPtr);

	if(templatePtr->handler == NULL) return UUcError_None;

	return templatePtr->handler(TMcTemplateProcMessage_Update, templatePtr->tag, inDataPtr);
}

/*
 * This is called when an instances is about to be used
 */
UUtError
TMrInstance_PrepareForUse(
	void*				inDataPtr)
{
	TMtTemplateDefinition*	templatePtr;

	TMmInstanceData_Verify(inDataPtr);

	templatePtr = TMmInstanceData_GetTemplateDefinition(inDataPtr);

	if(templatePtr->handler == NULL) return UUcError_None;

	return templatePtr->handler(TMcTemplateProcMessage_PrepareForUse, templatePtr->tag, inDataPtr);
}

/*
 * Get the template tag for this instance
 */
TMtTemplateTag
TMrInstance_GetTemplateTag(
	const void*	inDataPtr)
{
	TMtInstanceDescriptor*	targetDesc;
	TMtInstanceFile*		targetInstanceFile;

	TMmInstanceData_Verify(inDataPtr);

	targetInstanceFile = TMmInstanceData_GetInstanceFile(inDataPtr);
	TMmInstanceFile_Verify(targetInstanceFile);

	targetDesc = targetInstanceFile->instanceDescriptors + TMmPlaceHolder_GetIndex(TMmInstanceData_GetPlaceHolder(inDataPtr));
	TMmInstanceDesc_Verify(targetDesc);

	return targetDesc->templatePtr->tag;
}

/*
 * Get the instance name for this instance
 */
char*
TMrInstance_GetInstanceName(
	const void*	inDataPtr)
{
	TMtInstanceDescriptor*	targetDesc;
	TMtInstanceFile*		targetInstanceFile;
	char *					namePtr;

	TMmInstanceData_Verify(inDataPtr);

	targetInstanceFile = TMmInstanceData_GetInstanceFile(inDataPtr);
	TMmInstanceFile_Verify(targetInstanceFile);

	targetDesc = targetInstanceFile->instanceDescriptors + TMmPlaceHolder_GetIndex(TMmInstanceData_GetPlaceHolder(inDataPtr));
	TMmInstanceDesc_Verify(targetDesc);

	namePtr = targetDesc->namePtr;

	if (NULL != namePtr)
	{
		namePtr += 4;
	}

	return namePtr;
}

// ----------------------------------------------------------------------
UUtBool
TMrLevel_Exists(
	UUtUns16			inLevelNumber,
	UUtBool				inCheckTemplateChecksum)
{
	UUtError			error;
	UUtBool				level_exists = UUcFalse;

	char				fileName[128];
	BFtFileIterator*	fileIterator;

	BFtFileRef*			datFileRef;

	char*				temp;
	char				curFileName[256];
	UUtUns32			curFileLevelNum;

	BFtFile*				datFile;
	TMtInstanceFile_Header	fileHeader;

	UUtBool					needsSwapping;

	TMtTemplateDescriptor*	templateDescriptors = NULL;
	UUtUns32				itr;
	TMtTemplateDefinition*	templatePtr;

	level_exists = UUcFalse;
	datFileRef = NULL;

	// set the file name
	sprintf(fileName, "level%d", inLevelNumber);

	// create the file iterator
	error =
		BFrDirectory_FileIterator_New(
			TMgDataFolderRef,
			fileName,
			".dat",
			&fileIterator);
	if (error != UUcError_None)
	{
		return UUcFalse;
	}

	// go through all the files in the file iterator and look for
	// a file with the same level number as inLevelNumber
	while (1)
	{
		// get the next file in the directory from the file iterator
		error = BFrDirectory_FileIterator_Next(fileIterator, &datFileRef);
		if(error != UUcError_None)
		{
			datFileRef = NULL;
			break;
		}

		// copy the name of the file
		UUrString_Copy(curFileName, BFrFileRef_GetLeafName(datFileRef), 256);

		// get the level number from the file name
		temp = strrchr(curFileName, '_');
		UUmAssert(temp != NULL);
		*temp = 0;

		sscanf(curFileName + 5, "%d", &curFileLevelNum);

		if(curFileLevelNum == inLevelNumber && inCheckTemplateChecksum)
		{
			error = BFrFile_Open(datFileRef, "r", &datFile);
			if (error != UUcError_None)
			{
				UUrDebuggerMessage("level %d: Can't open file", inLevelNumber);
				level_exists = UUcFalse;
				goto done;
			}

			error = TMiInstanceFile_LoadHeader(datFile, &fileHeader, &needsSwapping);
			if (error != UUcError_None)
			{
				UUrDebuggerMessage("level %d: Can't load header", inLevelNumber);
				level_exists = UUcFalse;
				goto done;
			}

			if(fileHeader.numTemplateDescriptors == 0)
			{
				UUrDebuggerMessage("level %d: no templates", inLevelNumber);
				level_exists = UUcFalse;
				goto done;
			}

			if(templateDescriptors != NULL)
			{
				UUrMemory_Block_Delete(templateDescriptors);
			}

			templateDescriptors = UUrMemory_Block_New(fileHeader.numTemplateDescriptors * sizeof(TMtTemplateDescriptor));
			if(templateDescriptors == NULL)
			{
				UUrDebuggerMessage("level %d: out of memory", inLevelNumber);
				level_exists = UUcFalse;
				goto done;
			}

			//Read in the template descriptors
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
					UUrDebuggerMessage("level %d: can't read in template descriptors", inLevelNumber);
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

					templatePtr = TMiTemplate_FindDefinition(templateDescriptors[itr].tag);
					if(templatePtr == NULL)
					{
						UUrDebuggerMessage("level %d: can't find template %c%c%c%c",
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
						UUrDebuggerMessage("level %d: template %c%c%c%c: checksums do not match",
							inLevelNumber,
							(templateDescriptors[itr].tag >> 24) & 0xFF,
							(templateDescriptors[itr].tag >> 16) & 0xFF,
							(templateDescriptors[itr].tag >> 8) & 0xFF,
							(templateDescriptors[itr].tag >> 0) & 0xFF);
						level_exists = UUcFalse;
						goto done;
					}
				}

			BFrFile_Close(datFile);
		}

		if(curFileLevelNum == inLevelNumber)
		{
			level_exists = UUcTrue;
		}
		// dispose of the file ref
		BFrFileRef_Dispose(datFileRef);
		datFileRef = NULL;
	}

done:

	// delete the file iterator
	BFrDirectory_FileIterator_Delete(fileIterator);

	if(datFileRef != NULL)
	{
		BFrFileRef_Dispose(datFileRef);
	}

	if(templateDescriptors != NULL)
	{
		UUrMemory_Block_Delete(templateDescriptors);
	}

	return level_exists;
}

void
TMrLevel_Unload(
	void)
{
	UUtUns32			curFileIndex;

	TMgLevelLoadStack--;

	if(TMgLevelLoadStack == 0)
	{
		for(curFileIndex = 0; curFileIndex < TMgNumLevel0Files; curFileIndex++)
		{
			TMiInstanceFile_CallProcs(TMgLevel0Files[curFileIndex], TMcTemplateProcMessage_DisposePreProcess);
			TMiInstanceFile_Dispose(TMgLevel0Files[curFileIndex]);
			TMgLevel0Files[curFileIndex] = NULL;
		}

		// Delete all the level instance files after calling dispose
			TMiInstanceFile_CallProcs(&TMgPermInstanceFile, TMcTemplateProcMessage_DisposePreProcess);
	}
	else
	{
		/*
		 * First delete the previous instance files
		 */
			for(curFileIndex = 0; curFileIndex < TMgNumCurLevelFiles; curFileIndex++)
			{
				TMiInstanceFile_CallProcs(TMgCurLevelFiles[curFileIndex], TMcTemplateProcMessage_DisposePreProcess);
				TMiInstanceFile_Dispose(TMgCurLevelFiles[curFileIndex]);
				TMgCurLevelFiles[curFileIndex] = NULL;
			}

		#if 0
		BHP - no longer do this because it prevents us from accessing level 0 after an unload
		/*
		 * Next prepare the level 0 files for disk
		 */
			for(curFileIndex = 0; curFileIndex < TMgNumLevel0Files; curFileIndex++)
			{
				TMiInstanceFile_CallProcs(TMgLevel0Files[curFileIndex], TMcTemplateProcMessage_DisposePreProcess);
				TMiInstanceFile_PrepareForDisk(TMgLevel0Files[curFileIndex]);
			}

		#endif
	}

	// Next delete the temporary dynamic instances
		TMiInstanceFile_CallProcs(&TMgTempInstanceFile, TMcTemplateProcMessage_DisposePreProcess);
		TMgTempInstanceFile.numInstanceDescriptors = 0;
		UUrMemory_Pool_Reset(TMgTempPool);

	TMgNumCurLevelFiles = 0;
}

/*
 * This is used to load a new level
 */
UUtError
TMrLevel_Load(
	UUtUns16			inLevelNumber,
	TMtAllowPrivateData	inAllowPrivateData)
{
	UUtError			error;

	char				*desiredSuffix;

	BFtFileIterator*	fileIterator;
	char				fileName[128];

	BFtFileRef*			datFileRef;

	UUtUns16			curFileIndex;
	char				msg[256];

	TMtInstanceFile*	curInstanceFile;
	char*				temp;
	char				curFileName[256];

	TMtInstanceFile*	(*instanceFileArray)[TMcMaxLevelDatFiles];
	UUtUns16			numLevelFiles;

	UUtUns32			curFileLevelNum;

	UUmAssert(gInGame == UUcTrue);
	UUmAssert(gConstructionFile == NULL);

	sprintf(fileName, "level%d", inLevelNumber);

	if(inLevelNumber == 0)
	{
		if(TMrLevel_Exists(0, UUcTrue) == UUcFalse)
		{
			UUmError_ReturnOnErrorMsg(TMcError_DataCorrupt, "Level 0 out of data");
		}

		UUmAssert(TMgNumLevel0Files == 0);
		instanceFileArray = &TMgLevel0Files;
	}
	else
	{
		TMgNumCurLevelFiles = 0;
		instanceFileArray = &TMgCurLevelFiles;
	}

	if (TMcPrivateData_Yes == inAllowPrivateData)
	{
		desiredSuffix = ".dat";
	}
	else
	{
		desiredSuffix = "_Final.dat";
	}

	error =
		BFrDirectory_FileIterator_New(
			TMgDataFolderRef,
			fileName,
			desiredSuffix,
			&fileIterator);
	UUmError_ReturnOnErrorMsg(error, "Could not create file iterator");

	numLevelFiles = 0;

	while(1)
	{
		error = BFrDirectory_FileIterator_Next(fileIterator, &datFileRef);
		if(error != UUcError_None)
		{
			break;
		}

		UUrString_Copy(curFileName, BFrFileRef_GetLeafName(datFileRef), 256);

		temp = strrchr(curFileName, '_');
		UUmAssert(temp != NULL);
		*temp = 0;

		sscanf(curFileName + 5, "%d", &curFileLevelNum);

		if((UUtUns32)curFileLevelNum != inLevelNumber)
		{
			BFrFileRef_Dispose(datFileRef);
			continue;
		}

		// Create the instance file from a file ref
		error =
			TMiInstanceFile_MakeFromFileRef(
				datFileRef,
				&(*instanceFileArray)[numLevelFiles],
				UUcFalse);
		UUmError_ReturnOnErrorMsg(error, "Could not make instance file");

		curInstanceFile = (*instanceFileArray)[numLevelFiles];

		curInstanceFile->final = (strncmp(temp + 1, "Final", 5) == 0);
		UUrString_Copy(
			curInstanceFile->fileName,
			BFrFileRef_GetLeafName(datFileRef),
			BFcMaxFileNameLength);
		numLevelFiles++;
		BFrFileRef_Dispose(datFileRef);
	}

	BFrDirectory_FileIterator_Delete(fileIterator);

	if(numLevelFiles == 0)
	{
		UUmError_ReturnOnErrorMsg(TMcError_NoDatFile, "Could not find and .dat files");
	}

	sprintf(msg, "Loading level %d", inLevelNumber);

	if(inLevelNumber == 0)
	{
		TMgNumLevel0Files = numLevelFiles;
		/*
		 * Prepare everything for memory
		 */
			for(curFileIndex = 0; curFileIndex < TMgNumLevel0Files; curFileIndex++)
			{
				UUmAssertReadPtr(TMgLevel0Files[curFileIndex], sizeof(TMtInstanceFile));
				error = TMiInstanceFile_PrepareForMemory(TMgLevel0Files[curFileIndex], msg);
				UUmError_ReturnOnErrorMsg(error, "Could not prepare instance file for memory");
			}

		/*
		 * Call the load postprocess message
		 */
			for(curFileIndex = 0; curFileIndex < TMgNumLevel0Files; curFileIndex++)
			{
				UUmAssertReadPtr(TMgLevel0Files[curFileIndex], sizeof(TMtInstanceFile));
				error = TMiInstanceFile_CallProcs(TMgLevel0Files[curFileIndex], TMcTemplateProcMessage_LoadPostProcess);
				UUmError_ReturnOnErrorMsg(error, "Could not prepare instance file for memory");
			}
	}
	else
	{
		TMgNumCurLevelFiles = numLevelFiles;
		/*
		 * Prepare everything for memory
		 */
			for(curFileIndex = 0; curFileIndex < TMgNumCurLevelFiles; curFileIndex++)
			{
				UUmAssertReadPtr(TMgCurLevelFiles[curFileIndex], sizeof(TMtInstanceFile));
				error = TMiInstanceFile_PrepareForMemory(TMgCurLevelFiles[curFileIndex], msg);
				UUmError_ReturnOnErrorMsg(error, "Could not prepare instance file for memory");
			}

		/*
		 * Call the load postprocess message
		 */
			for(curFileIndex = 0; curFileIndex < TMgNumCurLevelFiles; curFileIndex++)
			{
				UUmAssertReadPtr(TMgCurLevelFiles[curFileIndex], sizeof(TMtInstanceFile));
				error = TMiInstanceFile_CallProcs(TMgCurLevelFiles[curFileIndex], TMcTemplateProcMessage_LoadPostProcess);
				UUmError_ReturnOnErrorMsg(error, "Could not prepare instance file for memory");
			}
	}

	TMgLevelLoadStack++;

	return UUcError_None;
}

UUtError
TMrConstruction_Start(
	BFtFileRef*			inInstanceFileRef)
{
	UUtError	error;

	UUmAssert(gInGame == UUcFalse);

	error = TMiInstanceFile_MakeFromFileRef(inInstanceFileRef, &gConstructionFile, UUcTrue);

	return error;
}

UUtError
TMrConstruction_Stop(
	UUtBool				inDumpStuff)
{
	char	temp[128];
	char	fileName[128];
	char*	p;
	FILE*	file;

	UUmAssert(gInGame == UUcFalse);
	UUmAssert(gConstructionFile != NULL);

	printf("running major check... "UUmNL);
	TMmInstanceFile_MajorCheck(gConstructionFile, UUcFalse);

	printf("writing to disk... "UUmNL);
	TMiInstanceFile_Save(gConstructionFile);

	printf("verifying all touched... "UUmNL);
	TMmInstanceFile_VerifyAllTouched(gConstructionFile);

	printf("dumping placeholders... "UUmNL);

	sprintf(temp, "%s", BFrFileRef_GetLeafName(gConstructionFile->instanceFileRef));
	p = strchr(temp, '.');
	if(p != NULL) *p = 0;

	sprintf(fileName, "%s_phs.txt", temp);

	file = fopen(fileName, "wb");
	TMiInstanceFile_DumpPlaceHolders(gConstructionFile, fileName, file);
	fclose(file);


	TMiInstanceFile_Dispose(gConstructionFile);
	gConstructionFile = NULL;

	return UUcError_None;
}

UUtError
TMrConstruction_Instance_Renew(
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName,
	UUtUns32			inVarArrayLength,
	void*				*outNewInstance)
{
	void*	dataPtr;
	TMtInstanceDescriptor*	targetInstDesc;

	UUmAssert(gInGame == UUcFalse);
	UUmAssert(gConstructionFile != NULL);

	*outNewInstance = NULL;

	if(inInstanceName == NULL)
	{
		return TMrConstruction_Instance_NewUnique(inTemplateTag, inVarArrayLength, outNewInstance);
	}

	targetInstDesc = TMiInstanceFile_Instance_FindInstanceDescriptor(gConstructionFile, inTemplateTag, inInstanceName);
	if(targetInstDesc == NULL || targetInstDesc->dataPtr == NULL)
	{
		dataPtr = TMiInstanceFile_Instance_Create(gConstructionFile, inTemplateTag, inInstanceName, inVarArrayLength);
		targetInstDesc = TMiInstanceFile_Instance_FindInstanceDescriptor(gConstructionFile, inTemplateTag, inInstanceName);
	}
	else
	{
		TMiInstanceFile_Instance_Clean(gConstructionFile, inTemplateTag, inInstanceName);
		dataPtr = TMiInstanceFile_Instance_ResizeVarArray(targetInstDesc->dataPtr, inVarArrayLength);

		targetInstDesc->dataPtr = dataPtr;
	}

	if(dataPtr == NULL)
	{
		return UUcError_OutOfMemory;
	}

	UUmAssert(targetInstDesc != NULL);

	#if defined(DEBUGGING) && DEBUGGING

		if(!(targetInstDesc->templatePtr->flags & TMcTemplateFlag_Registered))
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "template not registered");
		}

	#endif

	targetInstDesc->flags |= (TMtDescriptorFlags)(TMcDescriptorFlags_Touched | TMcDescriptorFlags_InBatchFile);

	TMmInstanceData_Verify(dataPtr);

	*outNewInstance = dataPtr;

	return UUcError_None;
}

UUtError
TMrConstruction_Instance_NewUnique(
	TMtTemplateTag		inTemplateTag,
	UUtUns32			inVarArrayLength,
	void*				*outReference)			// This needs to point into the refering structure
{
	void*	dataPtr;
	TMtTemplateDefinition* templatePtr;

	UUmAssert(gInGame == UUcFalse);
	UUmAssert(gConstructionFile != NULL);

	templatePtr = TMiTemplate_FindDefinition(inTemplateTag);

	#if defined(DEBUGGING) && DEBUGGING

		if(!(templatePtr->flags & TMcTemplateFlag_Registered))
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "template not registered");
		}

	#endif

	dataPtr =
		TMiInstanceFile_Instance_Create(
			gConstructionFile,
			inTemplateTag,
			0,
			inVarArrayLength);

	*outReference = dataPtr;

	return UUcError_None;
}

UUtError
TMrConstruction_Instance_GetPlaceHolder(
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName,
	TMtPlaceHolder		*outPlaceHolder)
{
	UUtError	error;
	TMtInstanceDescriptor*	targetDesc;

	UUmAssertReadPtr(inInstanceName, sizeof(char));
	UUmAssertWritePtr(outPlaceHolder, sizeof(TMtPlaceHolder));

	if('\0' == inInstanceName[0])
	{
		return UUcError_Generic;
	}

	if(gInGame == UUcTrue)
	{
		return TMrInstance_GetDataPtr(inTemplateTag, inInstanceName, (void**)outPlaceHolder);
	}

	//UUmAssert(gInGame == UUcFalse);
	//UUmAssert(gConstructionFile != NULL);

	*outPlaceHolder = 0;

	targetDesc = TMiInstanceFile_Instance_FindInstanceDescriptor(gConstructionFile, inTemplateTag, inInstanceName);
	if(targetDesc == NULL)
	{
		error =
			TMiInstanceFile_Instance_CreatePlaceHolder(
				gConstructionFile,
				inTemplateTag,
				inInstanceName,
				outPlaceHolder);
		UUmError_ReturnOnErrorMsg(error, "Could not create place holder");
		targetDesc = TMiInstanceFile_Instance_FindInstanceDescriptor(gConstructionFile, inTemplateTag, inInstanceName);
		UUmAssert(targetDesc != NULL);
	}

	TMiInstanceFile_Instance_Touch(gConstructionFile, targetDesc);

	// Always return an index
	if( 0 && targetDesc->dataPtr != NULL)
	{
		*outPlaceHolder = (TMtPlaceHolder)targetDesc->dataPtr;
	}
	else
	{
		*outPlaceHolder = TMmPlaceHolder_MakeFromIndex(targetDesc - gConstructionFile->instanceDescriptors);
	}

	return UUcError_None;
}

UUtBool
TMrConstruction_Instance_CheckExists(
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName,
	UUtUns32			*outCreationDate)
{
	TMtInstanceDescriptor*	targetDesc;
	UUtUns8*				swapCode;
	UUtUns8*				dataPtr;
	UUtBool					result;

	targetDesc = TMiInstanceFile_Instance_FindInstanceDescriptor(gConstructionFile, inTemplateTag, inInstanceName);
	if(targetDesc == NULL || targetDesc->dataPtr == NULL)
	{
		return UUcFalse;
	}

	/*
	 * Recursively look for unique instances that got out of date
	 */
	{
		swapCode = targetDesc->templatePtr->swapCodes;
		dataPtr = targetDesc->dataPtr;

		result = TMiInstance_CheckExists_Recursive(
			&swapCode,
			&dataPtr,
			gConstructionFile);

		if(result == UUcFalse) return UUcFalse;
	}

	TMiInstanceFile_Instance_Touch(gConstructionFile, targetDesc);

	targetDesc->flags |= (TMtDescriptorFlags)(TMcDescriptorFlags_Touched | TMcDescriptorFlags_InBatchFile);

	if(outCreationDate != NULL)
	{
		*outCreationDate = targetDesc->creationDate;
	}

	return UUcTrue;
}

UUtError
TMrConstruction_Instance_Keep(
	TMtTemplateTag		inTemplateTag,
	char*				inInstanceName)
{
	TMtInstanceDescriptor*	targetDesc;

	targetDesc = TMiInstanceFile_Instance_FindInstanceDescriptor(gConstructionFile, inTemplateTag, inInstanceName);
	if(targetDesc == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find instance");
	}

	TMiInstanceFile_Instance_Touch(gConstructionFile, targetDesc);

	targetDesc->flags |= (TMtDescriptorFlags)(TMcDescriptorFlags_Touched | TMcDescriptorFlags_InBatchFile);

	return UUcError_None;
}

#if defined(DEBUGGING) && DEBUGGING

void TMrConstruction_MajorCheck(
	void)
{
	//TMmInstanceFile_MajorCheck(gConstructionFile, UUcFalse);
}


#endif

UUtError
TMrConstruction_ConvertToNativeEndian(
	void)
{
	UUtError				error;
	BFtFileRef*				datFileRef;
	BFtFile*				datFile;
	BFtFileIterator*		fileIterator;
	TMtInstanceFile_Header	fileHeader;
	UUtBool					needsSwapping;
	TMtInstanceFile*		instanceFile;
	TMtInstanceDescriptor*	curInstDesc;
	UUtUns32				curDescIndex;

	// create the file iterator
	error =
		BFrDirectory_FileIterator_New(
			TMgDataFolderRef,
			"level",
			".dat",
			&fileIterator);
	UUmError_ReturnOnError(error);

	while (1)
	{
		// get the next file in the directory from the file iterator
		error = BFrDirectory_FileIterator_Next(fileIterator, &datFileRef);
		if(error != UUcError_None)
		{
			break;
		}

		error = BFrFile_Open(datFileRef, "r", &datFile);
		UUmError_ReturnOnError(error);

		error = TMiInstanceFile_LoadHeader(datFile, &fileHeader, &needsSwapping);
		UUmError_ReturnOnError(error);

		if(needsSwapping)
		{
			fprintf(stderr, "converting instance file %s\n", BFrFileRef_GetLeafName(datFileRef));

			error = TMiInstanceFile_MakeFromFileRef(datFileRef, &instanceFile, UUcFalse);
			UUmError_ReturnOnError(error);

			BFrFile_Close(datFile);
			BFrFileRef_Dispose(datFileRef);

			// Touch all the instances so they do not get deleted
			for(
				curDescIndex = 0, curInstDesc = instanceFile->instanceDescriptors;
				curDescIndex < instanceFile->numInstanceDescriptors;
				curDescIndex++, curInstDesc++)
			{
				curInstDesc->flags |= TMcDescriptorFlags_Touched;

				if(!(curInstDesc->flags & TMcDescriptorFlags_PlaceHolder))
				{
					curInstDesc->flags |= TMcDescriptorFlags_InBatchFile;
				}
			}

			error = TMiInstanceFile_Save(instanceFile);
			UUmError_ReturnOnError(error);

			TMiInstanceFile_Dispose(instanceFile);
		}
		else
		{
			BFrFile_Close(datFile);
			BFrFileRef_Dispose(datFileRef);
		}
	}

	// delete the file iterator
	BFrDirectory_FileIterator_Delete(fileIterator);

	gConstructionFile = NULL;

	return UUcError_None;
}



static void
TMiInstanceFile_VerifyRobust(
	TMtInstanceFile*		inInstanceFile)
{
	UUtUns32				curDescIndex;
	TMtInstanceDescriptor*	curDesc;

	TMmInstanceFile_Verify(inInstanceFile);

	for(curDescIndex = 0, curDesc = inInstanceFile->instanceDescriptors;
		curDescIndex < inInstanceFile->numInstanceDescriptors;
		curDescIndex++, curDesc++)
	{
		TMmInstanceDesc_Verify(curDesc);

		if (curDesc->dataPtr != NULL) {
			TMmTemplatePtr_Verify(curDesc->templatePtr);
			TMmInstanceData_Verify(curDesc->dataPtr);
		}
	}

	return;
}

static void
TMrVerifyAll(
	void)
{
	UUtUns16			curFileIndex;

	TMiInstanceFile_VerifyRobust(&TMgTempInstanceFile);

	for(curFileIndex = 0; curFileIndex < TMgNumLevel0Files; curFileIndex++)
	{
		TMiInstanceFile_VerifyRobust(TMgLevel0Files[curFileIndex]);
	}

	for(curFileIndex = 0; curFileIndex < TMgNumCurLevelFiles; curFileIndex++)
	{
		TMiInstanceFile_VerifyRobust(TMgCurLevelFiles[curFileIndex]);
	}

}

static void
TMiSwapCode_Dump_Indent(
	FILE*		inFile,
	UUtUns16	inIndent,
	UUtUns32	inDataOffset,
	UUtUns32	inSwapCodeOffset)
{
	UUtUns32	itr;

	fprintf(inFile, "[%04d, %04d]", inSwapCodeOffset, inDataOffset);

	for(itr = 0; itr < inIndent; itr++)
	{
		fprintf(inFile, "\t");
	}
}

static void
TMiSwapCode_Dump_Recursive(
	FILE*		inFile,
	UUtUns16	inIndent,
	UUtUns8*	*ioSwapCode,
	UUtUns32	*ioDataOffset,
	UUtUns8*	inSwapCodeBase)
{
	UUtUns8*				curSwapCode;
	UUtUns32				curDataOffset = *ioDataOffset;
	char					swapCode;
	UUtBool					stop;
	UUtUns8					count;
	UUtUns32				savedDataOffset;

	curSwapCode = *ioSwapCode;

	stop = UUcFalse;

	while(!stop)
	{
		swapCode = *curSwapCode++;

		switch(swapCode)
		{
			case TMcSwapCode_8Byte:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "8Byte(%02x)\n", TMcSwapCode_8Byte);
				curDataOffset += 8;
				break;

			case TMcSwapCode_4Byte:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "4Byte(%02x)\n", TMcSwapCode_4Byte);
				curDataOffset += 4;
				break;

			case TMcSwapCode_2Byte:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "2Byte(%02x)\n", TMcSwapCode_2Byte);
				curDataOffset += 2;
				break;

			case TMcSwapCode_1Byte:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "1Byte(%02x)\n", TMcSwapCode_1Byte);
				curDataOffset += 1;
				break;

			case TMcSwapCode_BeginArray:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "begin array(%02x)\n", TMcSwapCode_BeginArray);
				count = *curSwapCode++;
				TMiSwapCode_Dump_Indent(inFile, inIndent+1, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "count: %d\n", count);
				savedDataOffset = curDataOffset;
				TMiSwapCode_Dump_Recursive(
					inFile,
					inIndent+1,
					&curSwapCode,
					&curDataOffset,
					inSwapCodeBase);
				curDataOffset += (curDataOffset - savedDataOffset) * (count - 1);
				break;

			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				TMiSwapCode_Dump_Indent(inFile, inIndent-1, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "end array(%02x)\n", TMcSwapCode_EndArray);
				break;

			case TMcSwapCode_BeginVarArray:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "begin vararray(%02x)\n", TMcSwapCode_BeginVarArray);
				TMiSwapCode_Dump_Indent(inFile, inIndent+1, curDataOffset, curSwapCode - inSwapCodeBase);
				fprintf(inFile, "counttype: ");
				switch(*curSwapCode++)
				{
					case TMcSwapCode_8Byte:
						fprintf(inFile, "8byte(%02x)\n", TMcSwapCode_8Byte);
						curDataOffset += 8;
						break;
					case TMcSwapCode_4Byte:
						fprintf(inFile, "4byte(%02x)\n", TMcSwapCode_4Byte);
						curDataOffset += 4;
						break;
					case TMcSwapCode_2Byte:
						fprintf(inFile, "2byte(%02x)\n", TMcSwapCode_2Byte);
						curDataOffset += 2;
						break;
					default:
						UUmAssert(!"Swap codes fucked up");
				}

				TMiSwapCode_Dump_Recursive(
					inFile,
					inIndent+1,
					&curSwapCode,
					&curDataOffset,
					inSwapCodeBase);
				break;

			case TMcSwapCode_EndVarArray:
				TMiSwapCode_Dump_Indent(inFile, inIndent-1, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				fprintf(inFile, "end vararray(%02x)\n", TMcSwapCode_EndVarArray);
				stop = UUcTrue;
				break;

			case TMcSwapCode_TemplatePtr:
			case TMcSwapCode_TemplateReference:
				TMiSwapCode_Dump_Indent(inFile, inIndent, curDataOffset, curSwapCode - inSwapCodeBase - 1);
				if(swapCode == TMcSwapCode_TemplatePtr)
				{
					fprintf(inFile, "templatePtr(%02x): %c%c%c%c\n", TMcSwapCode_TemplatePtr,
						(*(UUtUns32*)curSwapCode >> 24) & 0xFF,
						(*(UUtUns32*)curSwapCode >> 16) & 0xFF,
						(*(UUtUns32*)curSwapCode >> 8) & 0xFF,
						(*(UUtUns32*)curSwapCode >> 0) & 0xFF);

					curSwapCode += 4;
					curDataOffset += 4;
				}
				else
				{
					fprintf(inFile, "templateReference(%02x)\n", TMcSwapCode_TemplateReference);
					curDataOffset += 8;
				}
				break;

			default:
				UUmDebugStr("swap codes are messed up.");
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataOffset = curDataOffset;
}

void
TMrSwapCode_DumpDefinition(
	FILE*					inFile,
	TMtTemplateDefinition*	inTemplateDefinition)
{
	fprintf(inFile, "Name: %s\n", inTemplateDefinition->name);
	fprintf(inFile, "Tag: %c%c%c%c\n",
		(inTemplateDefinition->tag >> 24) & 0xFF,
		(inTemplateDefinition->tag >> 16) & 0xFF,
		(inTemplateDefinition->tag >> 8) & 0xFF,
		(inTemplateDefinition->tag >> 0) & 0xFF);

	fprintf(inFile, "Swap Codes:\n");
	{
		UUtUns8*	swapCodes = inTemplateDefinition->swapCodes;
		UUtUns32	dataOffset = 0;

		TMiSwapCode_Dump_Indent(inFile, 0, 0, 0);
		fprintf(inFile, "begin array\n");
		TMiSwapCode_Dump_Recursive(inFile, 1, &swapCodes, &dataOffset, inTemplateDefinition->swapCodes);
	}
}

void
TMrSwapCode_DumpAll(
	FILE*	inFile)
{
	UUtUns32	itr;

	for(itr = 0; itr < TMgNumTemplateDefinitions; itr++)
	{
		fprintf(inFile, "******************************************\n");
		TMrSwapCode_DumpDefinition(inFile, TMgTemplateDefinitionArray + itr);
	}

}
