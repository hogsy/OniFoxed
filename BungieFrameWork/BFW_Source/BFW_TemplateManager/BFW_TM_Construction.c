/*
	FILE:	BFW_TM3_Construction.c
	
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

/*
 * =========================================================================
 * D E F I N E S
 * =========================================================================
 */
 
#define TMcConstruction_MaxInstances				(128 * 1024)
//#define TMcConstruction_MaxInstancesPerTemplate		(32 * 1024)
#define TMcConstructionPoolChunkSize				(10 * 1024 * 1024)

#if defined(DEBUGGING) && DEBUGGING

	#define TMmConstruction_Verify_AllTouched() TMiConstruction_Verify_AllTouched()
	
	#define TMmConstruction_Verify_InstanceData(x) TMiConstruction_Verify_InstanceData(x)

	#define TMmConstruction_Verify_InstanceDesc(x) TMiConstruction_Verify_InstanceDesc(x)
	
#else

	#define TMmConstruction_Verify_AllTouched()
	
	#define TMmConstruction_Verify_InstanceData(x)
	
	#define TMmConstruction_Verify_InstanceDesc(x)
	
#endif

/*
 * =========================================================================
 * T Y P E S
 * =========================================================================
 */
 
/*
 * Template related build data
 */

typedef struct TMtTemplate_ConstructionData
{
	UUtUns32	numInstancesUsed;
	UUtUns32	totalSizeOfAllInstances;
	
	UUtUns32	nextInstanceIndex;
	UUtUns32*	instanceIndexList;
	
	UUtUns32	numInstancesRemoved;
	UUtUns32	removedSize;
	
} TMtTemplate_ConstructionData;

typedef struct TMtRawTracking
{
	TMtTemplateTag identifier;
	UUtUns32 raw_count;
	UUtUns32 separate_count;
} TMtRawTracking;

#define TMcMaxIdentifiers 128

/*
 * =========================================================================
 * G L O B A L S
 * =========================================================================
 */
static AUtHashTable *			TMgRawTrackingHashTable = NULL;
static TMtTemplateTag			TMgRawTrackingList[TMcMaxIdentifiers];
static UUtUns32					TMgRawTrackingCount = 0;

static UUtUns32					TMgConstruction_NumInstanceDescriptors;
static TMtInstanceDescriptor*	TMgConstruction_InstanceDescriptors = NULL;

static UUtUns32					TMgConstruction_NumNameDescriptors;
static TMtNameDescriptor*		TMgConstruction_NameDescriptors = NULL;

static UUtUns32					TMgConstruction_NumTemplateDescriptors;
static TMtTemplateDescriptor*	TMgConstruction_TemplateDescriptors = NULL;

static BFtFileRef*				TMgConstruction_FileRef = NULL;
static BFtFileRef*				TMgConstruction_RawFileRef = NULL;
static BFtFileRef*				TMgConstruction_SeparateFileRef = NULL;

static UUtUns32*				TMgConstruction_DuplicateIndexArray = NULL;

static UUtMemory_Pool*			TMgConstructionPool = NULL;

static TMtInstanceFile_ID		TMgConstruction_InstanceFileID;

static BFtFile*					TMgConstruction_RawFile = NULL;
static UUtUns32					TMgConstruction_RawBytes = 0;

static BFtFile*					TMgConstruction_SeparateFile = NULL;
static UUtUns32					TMgConstruction_SeparateBytes = 0;

/*
 * =========================================================================
 * P R I V A T E   F U N C T I O N S
 * =========================================================================
 */

static UUtBool TMrRawTrackingCompare(UUtUns32 inA, UUtUns32 inB)
{
	TMtTemplateTag templateTagA = inA;
	TMtTemplateTag templateTagB = inB;
	UUtBool result;

	TMtRawTracking *trackingA = AUrHashTable_Find(TMgRawTrackingHashTable, &templateTagA);
	TMtRawTracking *trackingB = AUrHashTable_Find(TMgRawTrackingHashTable, &templateTagB);

	result = (trackingA->raw_count + trackingA->separate_count) < (trackingB->raw_count + trackingB->separate_count);

	return result;
}

static void
TMiConstruction_DataPtr_InstanceFileID_Set(
	void*				inDataPtr,
	TMtInstanceFile_ID	inInstanceFileID)
{
	UUmAssertReadPtr(inDataPtr, sizeof(UUtUns32));
	
	UUmAssert((((UUtUns32)inDataPtr - TMcPreDataSize) & 0x1F) == 0);

	*(TMtInstanceFile_ID*)((char *)inDataPtr - sizeof(TMtInstanceFile_ID)) = inInstanceFileID;
}

static TMtPlaceHolder
TMiConstruction_DataPtr_PlaceHolder_Get(
	const void*	inDataPtr)
{
	UUmAssert((((UUtUns32)inDataPtr - TMcPreDataSize) & 0x1F) == 0);
	
	return *(TMtPlaceHolder*)((char *)inDataPtr - TMcPreDataSize);
}

static void
TMiConstruction_DataPtr_PlaceHolder_Set(
	void*			inDataPtr,
	TMtPlaceHolder	inPlaceHolder)
{
	UUmAssert((((UUtUns32)inDataPtr - TMcPreDataSize) & 0x1F) == 0);

	*(TMtPlaceHolder*)((char *)inDataPtr - TMcPreDataSize) = inPlaceHolder;
}

static UUtUns32
TMiPlaceHolder_GetIndex(
	TMtPlaceHolder	inPlaceHolder)
{
	UUtUns32	index;
	
	index = TMmPlaceHolder_GetIndex(inPlaceHolder);
	UUmAssert(index < TMgConstruction_NumInstanceDescriptors);
	
	return index;
}

static TMtPlaceHolder
TMiPlaceHolder_MakeFromIndex(
	UUtUns32	inIndex)
{
	UUmAssert(inIndex < TMgConstruction_NumInstanceDescriptors);
	
	return TMmPlaceHolder_MakeFromIndex(inIndex);
}

static void
TMiPlaceHolder_EnsureIndex(
	TMtPlaceHolder	*ioPlaceHolder)
{
	
	if(*ioPlaceHolder == 0) return;
	
	if(TMmPlaceHolder_IsPtr(*ioPlaceHolder))
	{
		*ioPlaceHolder = TMiConstruction_DataPtr_PlaceHolder_Get((void*)*ioPlaceHolder);
	}
}

static void
TMiConstruction_Verify_InstanceData(
	const void*		inDataPtr)
{
	TMtPlaceHolder	placeHolder;
	UUtUns32		index;
	
	placeHolder = TMiConstruction_DataPtr_PlaceHolder_Get(inDataPtr);
	
	index = TMiPlaceHolder_GetIndex(placeHolder);
	
	UUmAssert(index < TMgConstruction_NumInstanceDescriptors);
}

static void
TMiConstruction_Verify_InstanceDesc(
	const TMtInstanceDescriptor*	inInstanceDescriptor)
{
	
}

static TMtInstanceDescriptor*
TMiPlaceHolder_GetInstanceDesc(
	TMtPlaceHolder	inPlaceHolder)
{	
	TMtInstanceDescriptor*	instanceDesc;
	TMtPlaceHolder			placeHolder;
	UUtUns32				index;

	if(TMmPlaceHolder_IsPtr(inPlaceHolder))
	{
		TMmConstruction_Verify_InstanceData((void*)inPlaceHolder);
		
		placeHolder = TMiConstruction_DataPtr_PlaceHolder_Get((void*)inPlaceHolder);
		
		UUmAssert(!TMmPlaceHolder_IsPtr(placeHolder));

		index = TMiPlaceHolder_GetIndex(placeHolder);

		instanceDesc = TMgConstruction_InstanceDescriptors + index;
	}
	else
	{
		index = TMiPlaceHolder_GetIndex(inPlaceHolder);

		instanceDesc = TMgConstruction_InstanceDescriptors + index;
	}
	
	TMmConstruction_Verify_InstanceDesc(instanceDesc);
	
	return instanceDesc;
}

static void
TMiConstruction_Remap_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr,
	UUtUns32*			inRemapArray)
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
					
					TMiConstruction_Remap_Recursive(
						&curSwapCode,
						&curDataPtr,
						inRemapArray);
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
					TMrUtility_SkipVarArray(
						&curSwapCode);
				}
				else
				{
					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;
						
						TMiConstruction_Remap_Recursive(
							&curSwapCode,
							&curDataPtr,
							inRemapArray);
					}
				}
				break;
				
			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;
				
			case TMcSwapCode_TemplatePtr:
				
				targetInstanceDesc = NULL;
				targetPlaceHolder = *(TMtPlaceHolder*)curDataPtr;
				
				if(targetPlaceHolder != 0)
				{
					if(TMmPlaceHolder_IsPtr(targetPlaceHolder))
					{
						TMiPlaceHolder_EnsureIndex((TMtPlaceHolder*)curDataPtr);	// This maps it to the correct index
						targetPlaceHolder = *(TMtPlaceHolder*)curDataPtr;
						
						targetInstanceDesc = TMgConstruction_InstanceDescriptors + TMmPlaceHolder_GetIndex(targetPlaceHolder);
					}
					else
					{
						//targetPlaceHolder is an index and needs to be remapped
						UUtUns32	originalIndex = TMmPlaceHolder_GetIndex(targetPlaceHolder);
						UUtUns32	remappedIndex = inRemapArray[originalIndex];
					
						if(remappedIndex != UUcMaxUns32)
						{
							UUmAssert(remappedIndex < TMgConstruction_NumInstanceDescriptors);
							
							*(TMtPlaceHolder*)curDataPtr = TMmPlaceHolder_MakeFromIndex(remappedIndex);
						}
						else
						{
							UUmAssert(0);
						}
						
						targetInstanceDesc = TMgConstruction_InstanceDescriptors + remappedIndex;
					}
				}

				if(targetInstanceDesc != NULL && (*(TMtTemplateTag*)curSwapCode != 0))
				{
					UUmAssert(*(TMtTemplateTag*)curSwapCode == targetInstanceDesc->templatePtr->tag);
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
}

static UUtBool
TMiConstruction_CheckExists_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr)
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
					
					result = TMiConstruction_CheckExists_Recursive(
						&curSwapCode,
						&curDataPtr);
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
					TMrUtility_SkipVarArray(&curSwapCode);
				}
				else
				{
					origSwapCode = curSwapCode;
					
					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;
						
						result = TMiConstruction_CheckExists_Recursive(
							&curSwapCode,
							&curDataPtr);
						if(result == UUcFalse) return UUcFalse;
					}
				}
				break;
				
			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;
				
			case TMcSwapCode_TemplatePtr:
				{
					TMtInstanceDescriptor	*targetDesc;
					void					*targetData;
					
					targetData = *(void **)curDataPtr;
					
					if(targetData != NULL)
					{
						targetDesc = TMiPlaceHolder_GetInstanceDesc((TMtPlaceHolder)targetData);
						TMmConstruction_Verify_InstanceDesc(targetDesc);
						
						if((targetDesc->flags & TMcDescriptorFlags_Unique) && targetDesc->dataPtr == NULL)
						{
							return UUcFalse;
						}
						
						if(targetDesc->dataPtr != NULL)
						{
							UUtUns8*	swapCode;
							UUtUns8*	dataPtr;
							
							swapCode = targetDesc->templatePtr->swapCodes;
							dataPtr = targetDesc->dataPtr - TMcPreDataSize;
							
							result = TMiConstruction_CheckExists_Recursive(
								&swapCode,
								&dataPtr);
							if(result == UUcFalse) return UUcFalse;
						}
						
					}

					curSwapCode += 4;
					curDataPtr += 4;
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

static UUtInt8
TMiConstruction_Compare_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtrA,
	UUtUns8*			*ioDataPtrB,
	UUtBool				inStart)
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
	UUtUns16				startCount = 0;
	
	curSwapCode = *ioSwapCode;
	curDataPtrA = *ioDataPtrA;
	curDataPtrB = *ioDataPtrB;
	
	stop = UUcFalse;
	
	while(!stop)
	{
		swapCode = *curSwapCode++;
		
		if(startCount > 1) inStart = UUcFalse;
		
		startCount++;
		
		
		switch(swapCode)
		{
			case TMcSwapCode_8Byte:
				if(*(UUtUns64*)curDataPtrA > *(UUtUns64*)curDataPtrB) return -1;
				if(*(UUtUns64*)curDataPtrA < *(UUtUns64*)curDataPtrB) return 1;
				curDataPtrA += 8;
				curDataPtrB += 8;
				break;

			case TMcSwapCode_4Byte:
				if(!inStart)
				{
					if(*(UUtUns32*)curDataPtrA > *(UUtUns32*)curDataPtrB) return -1;
					if(*(UUtUns32*)curDataPtrA < *(UUtUns32*)curDataPtrB) return 1;
				}
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
					
					result = TMiConstruction_Compare_Recursive(
								&curSwapCode,
								&curDataPtrA,
								&curDataPtrB,
								UUcFalse);
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
					TMrUtility_SkipVarArray(
						&curSwapCode);
				}
				else
				{
					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;
						
						result = TMiConstruction_Compare_Recursive(
									&curSwapCode,
									&curDataPtrA,
									&curDataPtrB,
									UUcFalse);
						if(result != 0) return result;
					}
				}
				break;
				
			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;
				
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
				
				targetDescA = TMiPlaceHolder_GetInstanceDesc(targetPlaceHolderA);
				targetDescB = TMiPlaceHolder_GetInstanceDesc(targetPlaceHolderB);
				
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
						
						dataPtrA = targetDescA->dataPtr - TMcPreDataSize;
						dataPtrB = targetDescB->dataPtr - TMcPreDataSize;
						
						result = TMiConstruction_Compare_Recursive(
									&swapCode,
									&dataPtrA,
									&dataPtrB,
									UUcTrue);
						if(result != 0) return result;
					}
				}
				
				curSwapCode += 4;
				curDataPtrA += 4;
				curDataPtrB += 4;
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

static void
TMiConstruction_Clean_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr)
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
	
	curSwapCode = *ioSwapCode;
	curDataPtr = *ioDataPtr;
	
	stop = UUcFalse;
	
	while(!stop)
	{
		swapCode = *curSwapCode++;
		
		switch(swapCode)
		{
			case TMcSwapCode_8Byte:
				#if DEBUGGING_MEMORY
					
					*(UUtUns64*)curDataPtr = UUmMemoryBlock_Garbage;
					
				#endif
				
				curDataPtr += 8;
				break;
				
			case TMcSwapCode_4Byte:
				#if DEBUGGING_MEMORY
					
					// This is bad cause it trashes our placeholder stuff
					//*(UUtUns32*)curDataPtr = UUmMemoryBlock_Garbage;
					
				#endif
				
				curDataPtr += 4;
				break;
				
			case TMcSwapCode_2Byte:
				#if DEBUGGING_MEMORY
				
					*(UUtUns16*)curDataPtr = (UUtUns16)UUmMemoryBlock_Garbage;
					
				#endif

				curDataPtr += 2;
				break;
				
			case TMcSwapCode_1Byte:
				#if DEBUGGING_MEMORY
				
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
					
					TMiConstruction_Clean_Recursive(
						&curSwapCode,
						&curDataPtr);
					
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
					TMrUtility_SkipVarArray(&curSwapCode);
				}
				else
				{
					origSwapCode = curSwapCode;
					
					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;
						
						TMiConstruction_Clean_Recursive(
							&curSwapCode,
							&curDataPtr);
					}
				}
				break;
				
			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;
				
			case TMcSwapCode_TemplatePtr:
				{
					TMtInstanceDescriptor	*targetDesc;
					void					*targetData;
					
					targetData = *(void **)curDataPtr;
					
					if(targetData != NULL)
					{
						targetDesc = TMiPlaceHolder_GetInstanceDesc((TMtPlaceHolder)targetData);
						TMmConstruction_Verify_InstanceDesc(targetDesc);
						
						if((targetDesc->flags & TMcDescriptorFlags_Unique) && targetDesc->dataPtr != NULL)
						{
							UUtUns8*	swapCode;
							UUtUns8*	dataPtr;
							
							swapCode = targetDesc->templatePtr->swapCodes;
							dataPtr = targetDesc->dataPtr - TMcPreDataSize;
							
							TMiConstruction_Clean_Recursive(
								&swapCode,
								&dataPtr);

							targetDesc->flags |= TMcDescriptorFlags_DeleteMe;
						}
						else if(targetDesc->flags & TMcDescriptorFlags_PlaceHolder)
						{
							targetDesc->flags |= TMcDescriptorFlags_DeleteMe;
						}
						
						#if DEBUGGING_MEMORY
						{
							*(UUtUns32*)curDataPtr = UUmMemoryBlock_Garbage;
						}
						#endif
					}

					curSwapCode += 4;
					curDataPtr += 4;
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
TMiConstruction_NullOutDeletedRefs(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr)
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
					
					TMiConstruction_NullOutDeletedRefs(
						&curSwapCode,
						&curDataPtr);
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
					TMrUtility_SkipVarArray(
						&curSwapCode);
				}
				else
				{
					for(i = 0; i < count; i++)
					{
						curSwapCode = origSwapCode;
						
						TMiConstruction_NullOutDeletedRefs(
							&curSwapCode,
							&curDataPtr);
					}
				}
				break;
				
			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;
				
			case TMcSwapCode_TemplatePtr:
			
				targetPlaceHolder = *(TMtPlaceHolder*)curDataPtr;
				
				if(targetPlaceHolder != 0)
				{
					targetDesc = TMiPlaceHolder_GetInstanceDesc(targetPlaceHolder);
					
					if(targetDesc->flags & TMcDescriptorFlags_DeleteMe)
					{
						*(void**)curDataPtr = NULL;
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
}

static void
TMiConstruction_Touch_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr);

static void
TMiConstruction_Touch_Recursive_Array(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr)
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
			
			TMiConstruction_Touch_Recursive(
				&curSwapCode,
				&curDataPtr);
		}
	}
		
	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
}

static void
TMiConstruction_Touch_Recursive_VarArray(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr)
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
				
				TMiConstruction_Touch_Recursive(
					&curSwapCode,
					&curDataPtr);
			}
		}
	}

	*ioSwapCode = curSwapCode;
	*ioDataPtr = curDataPtr;
}

static void
TMiConstruction_Touch_Recursive(
	UUtUns8*			*ioSwapCode,
	UUtUns8*			*ioDataPtr)
{
	UUtBool		stop;
	UUtUns8		*curSwapCode;
	UUtUns8		swapCode;
	UUtUns8		*curDataPtr;
	
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
				TMiConstruction_Touch_Recursive_Array(
					&curSwapCode,
					&curDataPtr);
				break;
				
			case TMcSwapCode_EndArray:
				stop = UUcTrue;
				break;
				
			case TMcSwapCode_BeginVarArray:
				TMiConstruction_Touch_Recursive_VarArray(
					&curSwapCode,
					&curDataPtr);
				break;
				
			case TMcSwapCode_EndVarArray:
				stop = UUcTrue;
				break;
				
			case TMcSwapCode_TemplatePtr:
				{
					TMtInstanceDescriptor	*targetDesc;
					void					*targetData;
					
					targetData = *(void **)curDataPtr;
					
					if(targetData != NULL)
					{
						targetDesc = TMiPlaceHolder_GetInstanceDesc((TMtPlaceHolder)targetData);
						UUmAssert(targetDesc != NULL);
						TMmConstruction_Verify_InstanceDesc(targetDesc);
						
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
								
								TMmConstruction_Verify_InstanceData(targetDesc->dataPtr);

								swapCode = targetDesc->templatePtr->swapCodes;
								dataPtr = targetDesc->dataPtr - TMcPreDataSize;
								
								TMiConstruction_Touch_Recursive(
									&swapCode,
									&dataPtr);
							}
						}
					}

					curSwapCode += 4;
					curDataPtr += 4;
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
TMiConstruction_Verify_AllTouched(
	void)
{
	UUtUns32				curDescIndex;
	
	TMtInstanceDescriptor*	curInstDesc;
	
	UUtUns8*				swapCodes;
	UUtUns8*				dataPtr;

	for(
		curDescIndex = 0, curInstDesc = TMgConstruction_InstanceDescriptors;
		curDescIndex < TMgConstruction_NumInstanceDescriptors;
		curDescIndex++, curInstDesc++)
	{
		curInstDesc->flags &= (TMtDescriptorFlags)~TMcDescriptorFlags_Touched;
	}
	
	for(
		curDescIndex = 0, curInstDesc = TMgConstruction_InstanceDescriptors;
		curDescIndex < TMgConstruction_NumInstanceDescriptors;
		curDescIndex++, curInstDesc++)
	{
		if((curDescIndex % 500) == 0) fputc('.', stderr);
		
		if(curInstDesc->flags & (TMcDescriptorFlags_Unique | TMcDescriptorFlags_PlaceHolder))
		{
			continue;
		}
		
		if(curInstDesc->dataPtr == NULL) continue;
		
		swapCodes = curInstDesc->templatePtr->swapCodes;
		dataPtr = curInstDesc->dataPtr - TMcPreDataSize;
		
		TMiConstruction_Touch_Recursive(
			&swapCodes,
			&dataPtr);
	}
	fputc('\n', stderr);
	
	for(
		curDescIndex = 0, curInstDesc = TMgConstruction_InstanceDescriptors;
		curDescIndex < TMgConstruction_NumInstanceDescriptors;
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

static UUtError
TMiConstruction_DeleteUnusedInstances(
	void)
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
	
	deletedRemapArray = UUrMemory_Block_New(TMgConstruction_NumInstanceDescriptors * sizeof(UUtInt32));
	UUmError_ReturnOnNull(deletedRemapArray);
	
	UUrMemory_Set32(deletedRemapArray, UUcMaxUns32, TMgConstruction_NumInstanceDescriptors * sizeof(UUtInt32));
	
	originalDescriptorList = UUrMemory_Block_New(TMgConstruction_NumInstanceDescriptors * sizeof(TMtInstanceDescriptor));
	UUmError_ReturnOnNull(originalDescriptorList);
	
	
	UUrMemory_Block_VerifyList();

	/*
	 * Prepare for deletion
	 */
		// go through and decide all the instances that need to be deleted
		for(curDescIndex = 0, curDesc = TMgConstruction_InstanceDescriptors;
			curDescIndex < TMgConstruction_NumInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			TMmConstruction_Verify_InstanceDesc(curDesc);
			
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
						dataPtr = curDesc->dataPtr - TMcPreDataSize;
						
						TMiConstruction_Clean_Recursive(
							&swapCodes,
							&dataPtr);
					}
					curDesc->dataPtr = NULL;
				}
			}
		}
	
	/*
	 * NULL out pointers to deleted instances
	 */
		for(curDescIndex = 0, curDesc = TMgConstruction_InstanceDescriptors;
			curDescIndex < TMgConstruction_NumInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if((curDesc->flags & TMcDescriptorFlags_DeleteMe) || curDesc->dataPtr == NULL)
			{
				continue;
			}

			swapCodes = curDesc->templatePtr->swapCodes;
			dataPtr = (UUtUns8 *)curDesc->dataPtr - TMcPreDataSize;
			
			TMiConstruction_NullOutDeletedRefs(&swapCodes, &dataPtr);
		}
	
		
	UUrMemory_MoveFast(TMgConstruction_InstanceDescriptors, originalDescriptorList, TMgConstruction_NumInstanceDescriptors * sizeof(TMtInstanceDescriptor));
	
	/*
	 * Compute the remap array
	 */
		newInstanceIndex = 0;
		
		for(curDescIndex = 0, curDesc = originalDescriptorList;
			curDescIndex < TMgConstruction_NumInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			TMmConstruction_Verify_InstanceDesc(curDesc);
			
			if(!(curDesc->flags & TMcDescriptorFlags_DeleteMe))
			{
				TMgConstruction_InstanceDescriptors[newInstanceIndex] = *curDesc;
				deletedRemapArray[curDescIndex] = newInstanceIndex++;
				continue;
			}
			
			if(curDesc->namePtr == NULL)
			{
				continue;
			}
			
			/* we must delete the name descriptor */
			for(curNameDescIndex = 0, curNameDesc = TMgConstruction_NameDescriptors;
				curNameDescIndex < TMgConstruction_NumNameDescriptors;
				curNameDescIndex++, curNameDesc++)
			{
				if(curNameDesc->instanceDescIndex == curDescIndex)
				{
					UUmAssert(strcmp(curNameDesc->namePtr, curDesc->namePtr) == 0);
					
					UUrMemory_MoveOverlap(
						TMgConstruction_NameDescriptors + curNameDescIndex + 1,
						TMgConstruction_NameDescriptors + curNameDescIndex,
						(TMgConstruction_NumNameDescriptors - curNameDescIndex) * sizeof(TMtNameDescriptor));
						
					TMgConstruction_NumNameDescriptors--;
					break;
				}
			}
		}
		
	for(curDescIndex = 0, curDesc = originalDescriptorList;
		curDescIndex < TMgConstruction_NumInstanceDescriptors;
		curDescIndex++, curDesc++)
	{
		if(deletedRemapArray[curDescIndex] != UUcMaxUns32)
		{
			TMtInstanceDescriptor*	originalDesc = originalDescriptorList + curDescIndex;
			TMtInstanceDescriptor*	newDesc = TMgConstruction_InstanceDescriptors + deletedRemapArray[curDescIndex];
			
			UUmAssert(originalDesc->templatePtr == newDesc->templatePtr);
			UUmAssert(originalDesc->dataPtr == newDesc->dataPtr);
		}	
	}
	
	TMgConstruction_NumInstanceDescriptors = newInstanceIndex;

	UUrMemory_Block_VerifyList();
	
	/*
	 * Remap the name descriptor indices
	 */
		for(curNameDescIndex = 0, curNameDesc = TMgConstruction_NameDescriptors;
			curNameDescIndex < TMgConstruction_NumNameDescriptors;
			curNameDescIndex++, curNameDesc++)
		{
			curNameDesc->instanceDescIndex = deletedRemapArray[curNameDesc->instanceDescIndex];
			UUmAssert(curNameDesc->instanceDescIndex < TMgConstruction_NumInstanceDescriptors);
			
			UUmAssert(TMgConstruction_InstanceDescriptors[curNameDesc->instanceDescIndex].namePtr != NULL);
			UUmAssert(strcmp(
						TMgConstruction_InstanceDescriptors[curNameDesc->instanceDescIndex].namePtr,
						curNameDesc->namePtr) == 0);
		}
	
	/*
	 * update the place holder data
	 */
		for(curDescIndex = 0, curDesc = TMgConstruction_InstanceDescriptors;
			curDescIndex < TMgConstruction_NumInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->dataPtr == NULL) continue;
			
			TMiConstruction_DataPtr_PlaceHolder_Set(
				curDesc->dataPtr,
				TMiPlaceHolder_MakeFromIndex(curDescIndex));
			//OLD - TMmInstanceData_GetPlaceHolder(curDesc->dataPtr) = TMmPlaceHolder_MakeFromIndex(curDescIndex);
		}
	
	/*
	 * update the place holder data and apply the remap
	 */
		for(curDescIndex = 0, curDesc = TMgConstruction_InstanceDescriptors;
			curDescIndex < TMgConstruction_NumInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->dataPtr == NULL) continue;
			
			swapCodes = curDesc->templatePtr->swapCodes;
			dataPtr = curDesc->dataPtr - TMcPreDataSize;
			
			TMiConstruction_Remap_Recursive(&swapCodes, &dataPtr, deletedRemapArray);
		}
	
	UUrMemory_Block_Delete(deletedRemapArray);
	UUrMemory_Block_Delete(originalDescriptorList);
	
	UUrMemory_Block_VerifyList();
	
	return UUcError_None;
}

static UUtUns32	TMgCompare_TemplateIndex;
static TMtTemplate_ConstructionData* gTemplateConstructionList = NULL;

static UUtInt8
TMiConstruction_RemoveIdentical_Compare(
	UUtUns32	inA,
	UUtUns32	inB)
{
	TMtInstanceDescriptor*	instanceDescA = TMgConstruction_InstanceDescriptors + inA;
	TMtInstanceDescriptor*	instanceDescB = TMgConstruction_InstanceDescriptors + inB;
	
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
		dataPtrA = instanceDescA->dataPtr - TMcPreDataSize;
		dataPtrB = instanceDescB->dataPtr - TMcPreDataSize;
		
		return TMiConstruction_Compare_Recursive(&swapCode, &dataPtrA, &dataPtrB, UUcTrue);
	}
}


static UUtBool TMiConstruction_RemoveIdentical_Compare_QSort(UUtUns32 inA, UUtUns32 inB)
{
	UUtInt8 difference = TMiConstruction_RemoveIdentical_Compare(inA, inB);
	UUtBool result = difference > 0;

	return result;
}


static UUtBool
TMiDumpStats_Compare_InstanceCount(
	UUtUns32	inA,
	UUtUns32	inB)
{
	UUmAssertReadPtr(gTemplateConstructionList, sizeof(TMtTemplateDescriptor));
	
	if(gTemplateConstructionList[inA].numInstancesUsed >= gTemplateConstructionList[inB].numInstancesUsed) return 0;

	return 1;
}

static UUtBool
TMiDumpStats_Compare_MemSize(
	UUtUns32	inA,
	UUtUns32	inB)
{
	UUmAssertReadPtr(gTemplateConstructionList, sizeof(TMtTemplateDescriptor));
	
	if(gTemplateConstructionList[inA].totalSizeOfAllInstances >= gTemplateConstructionList[inB].totalSizeOfAllInstances) return 0;

	return 1;
}

static UUtBool
TMiDumpStats_Compare_NumRemoved(
	UUtUns32	inA,
	UUtUns32	inB)
{
	UUmAssertReadPtr(gTemplateConstructionList, sizeof(TMtTemplateDescriptor));
	
	if(gTemplateConstructionList[inA].numInstancesRemoved >= gTemplateConstructionList[inB].numInstancesRemoved) return 0;

	return 1;
}

static UUtError
TMiConstruction_RemoveIdentical(
	void)
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
	UUtUns32*						originalDuplicateIndexArray;
	
	UUtUns32*						templateInstanceIndexMemory;
	UUtUns32*						curTemplateInstanceIndexMem;
	
	fprintf(stderr, "Removing identical...\n");
	
	duplicateRemapArray = UUrMemory_Block_New(TMgConstruction_NumInstanceDescriptors * sizeof(UUtInt32));
	UUmError_ReturnOnNull(duplicateRemapArray);
	
	UUrMemory_Set32(duplicateRemapArray, UUcMaxUns32, TMgConstruction_NumInstanceDescriptors * sizeof(UUtUns32));
	
	templateConstructionData = UUrMemory_Block_NewClear(TMgNumTemplateDefinitions * sizeof(TMtTemplate_ConstructionData));
	UUmError_ReturnOnNull(templateConstructionData);
	
	UUmAssertReadPtr(TMgConstruction_DuplicateIndexArray, sizeof(UUtUns32));
	
	UUrMemory_Set32(TMgConstruction_DuplicateIndexArray, UUcMaxUns32, TMgConstruction_NumInstanceDescriptors * sizeof(UUtUns32));
	
	originalDescriptorList = UUrMemory_Block_New(TMgConstruction_NumInstanceDescriptors * sizeof(TMtInstanceDescriptor));
	UUmError_ReturnOnNull(originalDescriptorList);
	
	originalDuplicateIndexArray = UUrMemory_Block_New(TMgConstruction_NumInstanceDescriptors * sizeof(UUtUns32));
	UUmError_ReturnOnNull(originalDuplicateIndexArray);
	
	// Build the template construction data
	for(curInstanceIndex = 0, curInstanceDesc = TMgConstruction_InstanceDescriptors;
		curInstanceIndex < TMgConstruction_NumInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		UUtInt32	templateIndex = curInstanceDesc->templatePtr - TMgTemplateDefinitionArray;
		
		UUmAssert(templateIndex >= 0);
		UUmAssert(templateIndex < (UUtInt32)TMgNumTemplateDefinitions);

		templateConstructionData[templateIndex].totalSizeOfAllInstances += curInstanceDesc->size;
		
		templateConstructionData[templateIndex].numInstancesUsed++;
	}
	
	templateInstanceIndexMemory = curTemplateInstanceIndexMem = UUrMemory_Block_New(TMgConstruction_NumInstanceDescriptors * sizeof(UUtUns32));
	UUmError_ReturnOnNull(templateInstanceIndexMemory);
	
	for(curTemplateDataIndex = 0, curTemplateData = templateConstructionData;
		curTemplateDataIndex < TMgNumTemplateDefinitions;
		curTemplateDataIndex++, curTemplateData++)
	{
		curTemplateData->instanceIndexList = curTemplateInstanceIndexMem;
		
		curTemplateInstanceIndexMem += curTemplateData->numInstancesUsed;
	}
	
	for(curInstanceIndex = 0, curInstanceDesc = TMgConstruction_InstanceDescriptors;
		curInstanceIndex < TMgConstruction_NumInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		UUtInt32	templateIndex = curInstanceDesc->templatePtr - TMgTemplateDefinitionArray;
		
		curTemplateData = templateConstructionData + templateIndex;
		
		UUmAssert(curTemplateData->nextInstanceIndex < curTemplateData->numInstancesUsed);
		
		curTemplateData->instanceIndexList[curTemplateData->nextInstanceIndex++] = curInstanceIndex;
	}
	
	for(curTemplateDataIndex = 0, curTemplateData = templateConstructionData;
		curTemplateDataIndex < TMgNumTemplateDefinitions;
		curTemplateDataIndex++, curTemplateData++)
	{
		UUmAssert(curTemplateData->numInstancesUsed == curTemplateData->nextInstanceIndex);
	}

	UUrMemory_Block_VerifyList();
	
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
			TMiConstruction_RemoveIdentical_Compare_QSort);
			
		TMgCompare_TemplateIndex = UUcMaxUns32;
	}
	
	fprintf(stderr, "\n\tLooking for duplicates");
	
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
			targetCheckInstance = TMgConstruction_InstanceDescriptors + targetCheckInstanceIndex;
			
			// make sure the target is not already a duplicate
			UUmAssert(!(targetCheckInstance->flags & TMcDescriptorFlags_Duplicate));

			while(++curInstanceIndexItr < curTemplateData->nextInstanceIndex)
			{
				curInstanceIndex = curTemplateData->instanceIndexList[curInstanceIndexItr];
				
				if(TMiConstruction_RemoveIdentical_Compare(
					targetCheckInstanceIndex,
					curInstanceIndex) != 0) break;
				
				// we have an identical instance
				
				// make sure it has not beeing assigned a target instance already
				UUmAssert(TMgConstruction_DuplicateIndexArray[curInstanceIndex] == UUcMaxUns32);
				
				// assign this instance a source(target)
				TMgConstruction_DuplicateIndexArray[curInstanceIndex] = targetCheckInstanceIndex;	// This needs to get remaped later
				
				// Mark the target instance as a source
				targetCheckInstance->flags |= TMcDescriptorFlags_DuplicatedSrc;
				
				// make sure the source is not a duplicate itself
				UUmAssert(!(targetCheckInstance->flags & TMcDescriptorFlags_Duplicate));
				
				curInstanceDesc = TMgConstruction_InstanceDescriptors + curInstanceIndex;
				
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

	UUrMemory_MoveFast(TMgConstruction_DuplicateIndexArray, originalDuplicateIndexArray, TMgConstruction_NumInstanceDescriptors * sizeof(UUtUns32));
	UUrMemory_MoveFast(TMgConstruction_InstanceDescriptors, originalDescriptorList, TMgConstruction_NumInstanceDescriptors * sizeof(TMtInstanceDescriptor));
		
	// compute the remap array and move the structures
		numNewInstances = 0;
		
		for(curInstanceIndex = 0, curInstanceDesc = originalDescriptorList;
			curInstanceIndex < TMgConstruction_NumInstanceDescriptors;
			curInstanceIndex++, curInstanceDesc++)
		{
			if((curInstanceIndex % 1000) == 0) fprintf(stderr, ".");

			if(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate)
			{
				UUmAssert(originalDuplicateIndexArray[curInstanceIndex] < TMgConstruction_NumInstanceDescriptors);
			}
			else
			{
				UUmAssert(originalDuplicateIndexArray[curInstanceIndex] == UUcMaxUns32);
			}
			
			if(!(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate) || curInstanceDesc->dataPtr == NULL || curInstanceDesc->namePtr != NULL)
			{
				TMgConstruction_DuplicateIndexArray[numNewInstances] = originalDuplicateIndexArray[curInstanceIndex];
				TMgConstruction_InstanceDescriptors[numNewInstances] = *curInstanceDesc;
				duplicateRemapArray[curInstanceIndex] = numNewInstances++;
			}
			else
			{
				UUmAssert(originalDuplicateIndexArray[curInstanceIndex] < TMgConstruction_NumInstanceDescriptors);
			}
			
		}


	// do some verification
	for(curInstanceIndex = 0, curInstanceDesc = originalDescriptorList;
		curInstanceIndex < TMgConstruction_NumInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		UUtUns32	remappedInstanceIndex = duplicateRemapArray[curInstanceIndex];
	
		if((curInstanceIndex % 1000) == 0) fprintf(stderr, ".");

		if(remappedInstanceIndex == UUcMaxUns32)
		{
			// curInstanceIndex is being deleted
			
			// make sure it has a duplicate
			UUmAssert(originalDuplicateIndexArray[curInstanceIndex] < TMgConstruction_NumInstanceDescriptors);
			
			targetCheckInstance = originalDescriptorList + originalDuplicateIndexArray[curInstanceIndex];
			
			// make sure that the duplicate flag is set
			UUmAssert(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate);
			
			// make sure its source is marked
			UUmAssert(targetCheckInstance->flags & TMcDescriptorFlags_DuplicatedSrc);
			
			// make sure it should really be deleted
			UUmAssert(curInstanceDesc->namePtr == NULL);
			
			// this deleted instance need to point to the duplicate source
			duplicateRemapArray[curInstanceIndex] = duplicateRemapArray[originalDuplicateIndexArray[curInstanceIndex]];
			UUmAssert(duplicateRemapArray[curInstanceIndex] < numNewInstances);
		}
		else
		{
			// curInstanceIndex is not being deleted
			
			// make sure it is legal
			UUmAssert(remappedInstanceIndex < numNewInstances);
			
			// if it is a duplicate then set its dataPtr to NULL
			if(originalDuplicateIndexArray[curInstanceIndex] != UUcMaxUns32)
			{
				UUmAssert(originalDuplicateIndexArray[curInstanceIndex] < TMgConstruction_NumInstanceDescriptors);
				
				// curInstanceIndex is a duplicate and it is not being deleted
				// screwing up tests - curInstanceDesc->dataPtr = NULL; // This gets remapped to the duplicate data later
			}
		}
		
		if(originalDuplicateIndexArray[curInstanceIndex] != UUcMaxUns32)
		{
			// curInstanceDesc has a duplicate
			
			targetCheckInstance = originalDescriptorList + originalDuplicateIndexArray[curInstanceIndex];

			// make sure it is legal
			UUmAssert(originalDuplicateIndexArray[curInstanceIndex] < TMgConstruction_NumInstanceDescriptors);
			
			// make sure its dupicate it not being deleted
			UUmAssert(duplicateRemapArray[originalDuplicateIndexArray[curInstanceIndex]] != UUcMaxUns32);
			
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
	
	TMgConstruction_NumInstanceDescriptors = numNewInstances;
	
	fprintf(stderr, "\n");
	fprintf(stderr, "\tUpdating place holders");

	// update the place holders, remap TMgConstruction_DuplicateIndexArray, and other stuff
	for(curInstanceIndex = 0, curInstanceDesc = TMgConstruction_InstanceDescriptors;
		curInstanceIndex < TMgConstruction_NumInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		if((curInstanceIndex % 1000) == 0) fprintf(stderr, ".");

		if(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate)
		{
			UUmAssert(TMgConstruction_DuplicateIndexArray[curInstanceIndex] != UUcMaxUns32);
			
			TMgConstruction_DuplicateIndexArray[curInstanceIndex] = duplicateRemapArray[TMgConstruction_DuplicateIndexArray[curInstanceIndex]];
			
			UUmAssert(TMgConstruction_DuplicateIndexArray[curInstanceIndex] < TMgConstruction_NumInstanceDescriptors);
			
		}
		else
		{
			UUmAssert(TMgConstruction_DuplicateIndexArray[curInstanceIndex] == UUcMaxUns32);
		}
		
		if(curInstanceDesc->dataPtr == NULL) continue;
		
	}
	
	fprintf(stderr, "\n\tRemapping instance data");

	// remap everthing
	for(curInstanceIndex = 0, curInstanceDesc = TMgConstruction_InstanceDescriptors;
		curInstanceIndex < TMgConstruction_NumInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		if((curInstanceIndex % 1000) == 0) fprintf(stderr, ".");

		if(curInstanceDesc->dataPtr != NULL)
		{
			UUtUns8*	swapCode;
			UUtUns8*	dataPtr;
			
			TMiConstruction_DataPtr_PlaceHolder_Set(
				curInstanceDesc->dataPtr,
				TMiPlaceHolder_MakeFromIndex(curInstanceIndex));
			
			swapCode = curInstanceDesc->templatePtr->swapCodes;
			dataPtr = curInstanceDesc->dataPtr - TMcPreDataSize;
			
			TMiConstruction_Remap_Recursive(&swapCode, &dataPtr, duplicateRemapArray);
		}

	}
	
	// do some more verifing
	for(curInstanceIndex = 0, curInstanceDesc = TMgConstruction_InstanceDescriptors;
		curInstanceIndex < TMgConstruction_NumInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		if(TMgConstruction_DuplicateIndexArray[curInstanceIndex] != UUcMaxUns32)
		{
			// curInstanceDesc has a duplicate
			
			targetCheckInstance = TMgConstruction_InstanceDescriptors + TMgConstruction_DuplicateIndexArray[curInstanceIndex];

			// make sure it is legal
			UUmAssert(TMgConstruction_DuplicateIndexArray[curInstanceIndex] < TMgConstruction_NumInstanceDescriptors);
			
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
	for(curNameDescIndex = 0, curNameDesc = TMgConstruction_NameDescriptors;
		curNameDescIndex < TMgConstruction_NumNameDescriptors;
		curNameDescIndex++, curNameDesc++)
	{
		UUtUns32	remapedIndex = duplicateRemapArray[curNameDesc->instanceDescIndex];
		
		UUmAssert(remapedIndex < TMgConstruction_NumInstanceDescriptors);
		
		UUmAssert(TMgConstruction_InstanceDescriptors[remapedIndex].namePtr != NULL);
		UUmAssert(strcmp(
					TMgConstruction_InstanceDescriptors[remapedIndex].namePtr,
					curNameDesc->namePtr) == 0);

		curNameDesc->instanceDescIndex = remapedIndex;
	}

	for(curInstanceIndex = 0, curInstanceDesc = TMgConstruction_InstanceDescriptors;
		curInstanceIndex < TMgConstruction_NumInstanceDescriptors;
		curInstanceIndex++, curInstanceDesc++)
	{
		if(curInstanceDesc->flags & TMcDescriptorFlags_Duplicate)
		{
			UUmAssert(TMgConstruction_DuplicateIndexArray[curInstanceIndex] != UUcMaxUns32);
		}
	}

	fprintf(stderr, "\nnum instances saved: %d\n", numTotalInstancesRemoved);
	fprintf(stderr, "size saved: %d\n", totalRemovedSize);
	
	UUrMemory_Block_Delete(duplicateRemapArray);

	TMgConstruction_NumTemplateDescriptors = numTemplateDescriptors;
	
	if(TMgConstruction_TemplateDescriptors != NULL)
	{
		UUrMemory_Block_Delete(TMgConstruction_TemplateDescriptors);
	}
	
	TMgConstruction_TemplateDescriptors = UUrMemory_Block_New(numTemplateDescriptors * sizeof(TMtTemplateDescriptor));
	numTemplateDescriptors = 0;
	
	for(curTemplateDataIndex = 0, curTemplateData = templateConstructionData;
		curTemplateDataIndex < TMgNumTemplateDefinitions;
		curTemplateDataIndex++, curTemplateData++)
	{
		if(curTemplateData->numInstancesUsed == 0) continue;
		
		TMgConstruction_TemplateDescriptors[numTemplateDescriptors].checksum = TMgTemplateDefinitionArray[curTemplateDataIndex].checksum;
		TMgConstruction_TemplateDescriptors[numTemplateDescriptors].tag = TMgTemplateDefinitionArray[curTemplateDataIndex].tag;
		TMgConstruction_TemplateDescriptors[numTemplateDescriptors].numUsed = curTemplateData->numInstancesUsed;
		numTemplateDescriptors++;
	}
	
	{
		UUtUns32	curDescIndex;
		UUtUns32*	sortedList;
		FILE*		file;
		char		instanceFileName[BFcMaxFileNameLength];
		char		fileName[BFcMaxFileNameLength];
		
		UUrString_Copy(instanceFileName, BFrFileRef_GetLeafName(TMgConstruction_FileRef), BFcMaxFileNameLength);
		UUrString_StripExtension(instanceFileName);
		
		sprintf(fileName, "%s_stats.txt", instanceFileName);
		
		file = fopen(fileName, "w");
		
		if(file != NULL)
		{
			{
				UUtUns32 raw_itr;

				fprintf(file, "raw stats:\n");
				
				AUrQSort_32(
					TMgRawTrackingList, 
					TMgRawTrackingCount, 
					TMrRawTrackingCompare);

				for(raw_itr = 0; raw_itr < TMgRawTrackingCount; raw_itr++)
				{
					TMtRawTracking *tracking_ptr = AUrHashTable_Find(TMgRawTrackingHashTable, TMgRawTrackingList + raw_itr);

					fprintf(file, "\t %c%c%c%c = %d/%d\n", 
						(tracking_ptr->identifier >> 24) & 0xff, 
						(tracking_ptr->identifier >> 16) & 0xff, 
						(tracking_ptr->identifier >> 8) & 0xff, 
						(tracking_ptr->identifier >> 0) & 0xff, 
						tracking_ptr->raw_count,
						tracking_ptr->separate_count);
				}

				fprintf(file, "\n\n\n");
			}

			gTemplateConstructionList = templateConstructionData;
			
			sortedList = UUrMemory_Block_New(TMgNumTemplateDefinitions * sizeof(UUtUns32));
			UUmError_ReturnOnNull(sortedList);
			
			for(curDescIndex = 0; curDescIndex < TMgNumTemplateDefinitions; curDescIndex++)
			{
				sortedList[curDescIndex] = curDescIndex;
			}

			{
				UUtUns32 total_instances_used = 0;
				UUtUns32 total_memory_used = 0;
				UUtUns32 total_instances_saved = 0;
				UUtUns32 total_memory_saved = 0;

				for(curDescIndex = 0;
					curDescIndex < TMgNumTemplateDefinitions;
					curDescIndex++)
				{
					UUtInt32						templateDefIndex = sortedList[curDescIndex];
					TMtTemplate_ConstructionData*	curTemplateData = templateConstructionData + templateDefIndex;
					TMtTemplateDefinition*			templateDef = TMgTemplateDefinitionArray + templateDefIndex;
									
					if(curTemplateData->numInstancesUsed == 0) continue;

					total_instances_used += curTemplateData->numInstancesUsed;
					total_memory_used += curTemplateData->totalSizeOfAllInstances;
					total_instances_saved += curTemplateData->numInstancesRemoved;
					total_memory_saved += curTemplateData->removedSize;
				}

				fprintf(file, "\t*******************************\n");
				fprintf(file, "\tTotal Instances Used: %d\n", total_instances_used);
				fprintf(file, "\tTotal Memory Size: %d\n", total_memory_used);
				fprintf(file, "\tNum Instances Saved: %d\n", total_instances_saved);
				fprintf(file, "\tMemory Saved: %d\n", total_memory_saved);
				fprintf(file, "\t*******************************\n");
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
				TMiDumpStats_Compare_InstanceCount);
			
			fprintf(file, "Instance file dump\n");
			fprintf(file, "fileName: %s\n", BFrFileRef_GetLeafName(TMgConstruction_FileRef));
			
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
	UUrMemory_Block_Delete(originalDuplicateIndexArray);
	UUrMemory_Block_Delete(originalDescriptorList);
	UUrMemory_Block_Delete(templateInstanceIndexMemory);
	
	return UUcError_None;
}

static void
TMiConstruction_SaveBinaryFile(
	BFtFileRef**			ioFileRef,
	BFtFile**				ioFile,
	UUtUns32*				ioBytes)
{
	if (*ioFileRef != NULL) {
		BFrFileRef_Dispose(*ioFileRef);
	}

	if (*ioFile != NULL) {
		BFrFile_Close(*ioFile);
	}

	*ioFileRef = NULL;
	*ioFile = NULL;
	*ioBytes = 0;
}

static UUtError
TMiConstruction_Save(
	void)
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
		
	UUmAssertReadPtr(TMgConstruction_FileRef, sizeof(UUtUns32));
	
	UUrMemory_Block_VerifyList();
	
	if(TMgConstruction_NumInstanceDescriptors == 0)
	{
		return UUcError_None;
	}
	
	if(TMgConstruction_DuplicateIndexArray != NULL)
	{
		UUrMemory_Block_Delete(TMgConstruction_DuplicateIndexArray);
		TMgConstruction_DuplicateIndexArray = NULL;
	}
	
	TMgConstruction_DuplicateIndexArray = UUrMemory_Block_New(TMgConstruction_NumInstanceDescriptors * sizeof(UUtUns32));
	UUmError_ReturnOnNull(TMgConstruction_DuplicateIndexArray);
	
	// delete unused instances
		error = TMiConstruction_DeleteUnusedInstances();
		UUmError_ReturnOnError(error);
	
	/*
	 * Remove identical instance
	 */
		error = TMiConstruction_RemoveIdentical();
		UUmError_ReturnOnError(error);
	
	/*
	 * Prepare all instances for disk 
	 */
		for(curDescIndex = 0, curDesc = TMgConstruction_InstanceDescriptors;
			curDescIndex < TMgConstruction_NumInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			templateDefinition = curDesc->templatePtr;
			
			curDesc->templatePtr = (void*)templateDefinition->tag;
		}
		
	UUrMemory_Block_VerifyList();

	/*
	 * Save out binary data files
	 */

	TMiConstruction_SaveBinaryFile(&TMgConstruction_RawFileRef, &TMgConstruction_RawFile, &TMgConstruction_RawBytes);
	TMiConstruction_SaveBinaryFile(&TMgConstruction_SeparateFileRef, &TMgConstruction_SeparateFile, &TMgConstruction_SeparateBytes);

	/*
	 * Delete any old instance file
	 */
		BFrFile_Delete(TMgConstruction_FileRef);
	 
		error = BFrFile_Create(TMgConstruction_FileRef);
		UUmError_ReturnOnErrorMsg(error, "Could create instance file");
	 
	/*
	 * open the instance files for writing
	 */
		error = BFrFile_Open(TMgConstruction_FileRef, "w", &instancePhysicalFile);
		UUmError_ReturnOnErrorMsg(error, "Could not open instance file for writing");
	
	/*
	 * Calculate the new file header
	 */
		fileHeader.version = TMcInstanceFile_Version;
		
		fileHeader.sizeofHeader = sizeof(TMtInstanceFile_Header);
		fileHeader.sizeofInstanceDescriptor = sizeof(TMtInstanceDescriptor);
		fileHeader.sizeofTemplateDescriptor = sizeof(TMtTemplateDescriptor);
		fileHeader.sizeofNameDescriptor = sizeof(TMtNameDescriptor);
		
		fileHeader.totalTemplateChecksum = TMgTemplateChecksum;
		fileHeader.numInstanceDescriptors = TMgConstruction_NumInstanceDescriptors;
		fileHeader.numNameDescriptors = TMgConstruction_NumNameDescriptors;
		fileHeader.numTemplateDescriptors = TMgConstruction_NumTemplateDescriptors;
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
		
		for(curDescIndex = 0, curDesc = TMgConstruction_InstanceDescriptors;
			curDescIndex < TMgConstruction_NumInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if((curDesc->flags & TMcDescriptorFlags_Duplicate) || curDesc->dataPtr == NULL)
			{
				continue;
			}
			
			TMiConstruction_DataPtr_InstanceFileID_Set(
				curDesc->dataPtr,
				TMgConstruction_InstanceFileID);
			
			// First write the data to the new offset
			error =
				BFrFile_WritePos(
					instancePhysicalFile,
					curDataFilePos,
					curDesc->size,
					curDesc->dataPtr - TMcPreDataSize);
			UUmError_ReturnOnErrorMsg(error, "Could not write instance data size");

			curDesc->dataPtr = (void*)(curDataOffset + TMcPreDataSize);
			curDataOffset += curDesc->size;
			curDataFilePos += curDesc->size;
		}
		
		for(curDescIndex = 0, curDesc = TMgConstruction_InstanceDescriptors;
			curDescIndex < TMgConstruction_NumInstanceDescriptors;
			curDescIndex++, curDesc++)
		{
			if(curDesc->flags & TMcDescriptorFlags_Duplicate)
			{
				curDesc->dataPtr = TMgConstruction_InstanceDescriptors[TMgConstruction_DuplicateIndexArray[curDescIndex]].dataPtr;
			}
		}
		
		UUrMemory_Block_VerifyList();

		fileHeader.dataBlockLength = curDataOffset;
		
		curNameOffset = 0;
		fileHeader.nameBlockOffset = curNameFilePos = curDataFilePos;
		
		for(curDescIndex = 0, curDesc = TMgConstruction_InstanceDescriptors;
			curDescIndex < TMgConstruction_NumInstanceDescriptors;
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

		fileHeader.nameBlockLength = curNameOffset;
	
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
					fileHeader.numInstanceDescriptors * sizeof(TMtInstanceDescriptor),
					TMgConstruction_InstanceDescriptors);
		UUmError_ReturnOnErrorMsg(error, "Could not write instance descriptors");
	
	/*
	 * Write out the name descriptors
	 */
		error =
			BFrFile_WritePos(
				instancePhysicalFile,
					sizeof(TMtInstanceFile_Header) + TMgConstruction_NumInstanceDescriptors * sizeof(TMtInstanceDescriptor),
					fileHeader.numNameDescriptors * sizeof(TMtNameDescriptor),
					TMgConstruction_NameDescriptors);
		UUmError_ReturnOnErrorMsg(error, "Could not write name descriptors");
	
	/*
	 * Write out the template descriptors
	 */
		error =
			BFrFile_WritePos(
				instancePhysicalFile,
					sizeof(TMtInstanceFile_Header) +
						fileHeader.numInstanceDescriptors * sizeof(TMtInstanceDescriptor) +
						fileHeader.numNameDescriptors * sizeof(TMtNameDescriptor),
					fileHeader.numTemplateDescriptors * sizeof(TMtTemplateDescriptor),
					TMgConstruction_TemplateDescriptors);
		UUmError_ReturnOnErrorMsg(error, "Could not write name descriptors");
		
	BFrFile_Close(instancePhysicalFile);
	
	UUrMemory_Block_VerifyList();

	UUrMemory_Pool_Reset(TMgConstructionPool);
	
	return UUcError_None;
}

static void
TMiConstruction_DumpPlaceHolders(
	char*	inFileName,
	FILE*	inFile)
{
	UUtUns32				curDescIndex;
	TMtNameDescriptor*		curNameDesc;
	UUtUns32				numPlaceHolders = 0;
	
	fprintf(inFile, "Place holder dump"UUmNL);
	fprintf(inFile, "fileName: %s"UUmNL, BFrFileRef_GetLeafName(TMgConstruction_FileRef));
	
	for(curDescIndex = 0, curNameDesc = TMgConstruction_NameDescriptors;
		curDescIndex < TMgConstruction_NumNameDescriptors;
		curDescIndex++, curNameDesc++)
	{
		if((TMgConstruction_InstanceDescriptors + curNameDesc->instanceDescIndex)->flags & TMcDescriptorFlags_PlaceHolder)
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

static void
TMiConstruction_DumpInstanceNames(
	char*	inFileName,
	FILE*	inFile)
{
	UUtUns32				curDescIndex;
	TMtNameDescriptor*		curNameDesc;
	
	fprintf(inFile, "Instance name dump"UUmNL);
	fprintf(inFile, "fileName: %s"UUmNL, BFrFileRef_GetLeafName(TMgConstruction_FileRef));
	
	for(curDescIndex = 0, curNameDesc = TMgConstruction_NameDescriptors;
		curDescIndex < TMgConstruction_NumNameDescriptors;
		curDescIndex++, curNameDesc++)
	{
		fprintf(inFile, "[%d] Name: %s"UUmNL, curDescIndex, curNameDesc->namePtr);
	}
}

static TMtInstanceDescriptor*
TMiConstruction_FindInstanceDescriptor(
	TMtTemplateTag	inTemplateTag,
	const char*		inInstanceName)
{
	UUtUns32				curDescIndex;
	TMtNameDescriptor*		curDesc;
	
	for(curDescIndex = 0, curDesc = TMgConstruction_NameDescriptors;
		curDescIndex < TMgConstruction_NumNameDescriptors;
		curDescIndex++, curDesc++)
	{
		UUmAssert(curDesc->namePtr != NULL);
		
		if(	curDesc->namePtr[0] == (char)((inTemplateTag >> 24) & 0xFF) &&
			curDesc->namePtr[1] == (char)((inTemplateTag >> 16) & 0xFF) &&
			curDesc->namePtr[2] == (char)((inTemplateTag >> 8) & 0xFF) &&
			curDesc->namePtr[3] == (char)((inTemplateTag >> 0) & 0xFF))
		{
			const char *curDescName = curDesc->namePtr + 4;

			if (0 == strcmp(inInstanceName, curDescName))
			{
				return TMgConstruction_InstanceDescriptors + curDesc->instanceDescIndex;
			}
		}
	}
	
	return NULL;
}

static UUtError
TMiConstruction_CreateNewDescriptor(
	TMtTemplateTag	inTemplateTag,
	const char*		inInstanceName,
	UUtUns32		*outNewDescIndex)
{	
	TMtTemplateDefinition*	templateDefinition;
	TMtInstanceDescriptor*	newInstanceDescriptor;
	
	char*					newNamePtr;

	UUtUns32				newIndex;

	UUtUns32				curNameDescIndex;
	TMtNameDescriptor*		curNameDesc;
	TMtNameDescriptor*		newNameDescriptor;
	
	UUrMemory_Block_VerifyList();
	
	*outNewDescIndex = UUcMaxUns32;
	
	templateDefinition = TMrUtility_Template_FindDefinition(inTemplateTag);
	if(templateDefinition == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find template definition");
	}
	
	if(TMgConstruction_NumInstanceDescriptors >= TMcConstruction_MaxInstances)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Exceeded maximum number of instances in a file");
	}
	
	newIndex = TMgConstruction_NumInstanceDescriptors++;
	
	newInstanceDescriptor = TMgConstruction_InstanceDescriptors + newIndex;

	if(inInstanceName != NULL)
	{
		newNamePtr = UUrMemory_Pool_Block_New(TMgConstructionPool, strlen(inInstanceName) + 5);
		if(newNamePtr == NULL)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not alloc contruction pool");
		}
		
		strcpy(newNamePtr + 4, inInstanceName);
		newNamePtr[0] = (char)((inTemplateTag >> 24) & 0xFF);
		newNamePtr[1] = (char)((inTemplateTag >> 16) & 0xFF);
		newNamePtr[2] = (char)((inTemplateTag >> 8) & 0xFF);
		newNamePtr[3] = (char)((inTemplateTag) & 0xFF);
	}
	else
	{
		newNamePtr = NULL;
	}
	
	newInstanceDescriptor->templatePtr = templateDefinition;
	newInstanceDescriptor->namePtr = newNamePtr;
	newInstanceDescriptor->flags = inInstanceName == NULL ? TMcDescriptorFlags_Unique : TMcDescriptorFlags_None;
	newInstanceDescriptor->dataPtr = NULL;
	newInstanceDescriptor->size = 0;
	
	newInstanceDescriptor->flags |= TMcDescriptorFlags_Touched;

	if(inInstanceName != NULL)
	{
		for(curNameDescIndex = 0, curNameDesc = TMgConstruction_NameDescriptors;
			curNameDescIndex < TMgConstruction_NumNameDescriptors;
			curNameDescIndex++, curNameDesc++)
		{
			if(strcmp(newNamePtr, curNameDesc->namePtr) < 0)
			{
				break;
			}
		}
		
		UUrMemory_MoveOverlap(
			TMgConstruction_NameDescriptors + curNameDescIndex,
			TMgConstruction_NameDescriptors + curNameDescIndex + 1,
			(TMgConstruction_NumInstanceDescriptors - curNameDescIndex) * sizeof(TMtNameDescriptor));

		newNameDescriptor = TMgConstruction_NameDescriptors + curNameDescIndex;
		
		newNameDescriptor->namePtr = newInstanceDescriptor->namePtr;
		newNameDescriptor->instanceDescIndex = newIndex;
		
		TMgConstruction_NumNameDescriptors++;
	}
	
	*outNewDescIndex = newIndex;
	
	return UUcError_None;
}

static void*
TMiConstruction_Instance_Create(
	TMtTemplateTag	inTemplateTag,
	const char*		inInstanceName,
	UUtUns32		inVarArrayLength)
{
	UUtError				error;
	TMtTemplateDefinition*	templateDefinition;
	TMtInstanceDescriptor*	newInstanceDescriptor;
	
	UUtUns8*				newDataPtr;
	UUtUns32				dataSize;
	
	UUtUns32				newDescIndex;
	
	if(inInstanceName != NULL)
	{
		UUmAssertReadPtr(inInstanceName, 1);
		UUmAssert(strlen(inInstanceName) < TMcInstanceName_MaxLength);
	}
		
	templateDefinition = TMrUtility_Template_FindDefinition(inTemplateTag);
	if(templateDefinition == NULL)
	{
		UUrError_Report(UUcError_Generic, "Could not find template definition");
		return NULL;
	}
	
	newInstanceDescriptor = NULL;
	
	if(inInstanceName != NULL)
	{
		/*
		 * First check to see if instance already exists and is a place holder
		 */
			newInstanceDescriptor =
				TMiConstruction_FindInstanceDescriptor(
					inTemplateTag,
					inInstanceName);
			if(newInstanceDescriptor != NULL)
			{
				if(!(newInstanceDescriptor->flags & TMcDescriptorFlags_PlaceHolder))
				{
					UUrError_Report(UUcError_Generic, "An instance already exists with this name");
					return NULL;
				}
				// Replace the place holder
				newInstanceDescriptor->flags &= (TMtDescriptorFlags)~TMcDescriptorFlags_PlaceHolder;
				
				newDescIndex = newInstanceDescriptor - TMgConstruction_InstanceDescriptors;
			}
	}
	
	if(newInstanceDescriptor == NULL)
	{
		error = 
			TMiConstruction_CreateNewDescriptor(
				inTemplateTag,
				inInstanceName,
				&newDescIndex);
		if(error != UUcError_None)
		{
			UUrError_Report(UUcError_Generic, "Could not create descriptor");
			return NULL;
		}
		
		newInstanceDescriptor = TMgConstruction_InstanceDescriptors + newDescIndex;
	}

	dataSize = templateDefinition->size + inVarArrayLength * templateDefinition->varArrayElemSize;
	
	dataSize = (dataSize + UUcProcessor_CacheLineSize_Mask) & ~UUcProcessor_CacheLineSize_Mask;

	newDataPtr = UUrMemory_Pool_Block_New(TMgConstructionPool, dataSize);
	if(newDataPtr == NULL)
	{
		UUmAssert(!"Out of memory");
		return NULL;
	}
	
	UUrMemory_Set32(newDataPtr, 0xDEADDEAD, dataSize);
	
	UUmAssertAlignedPtr(newDataPtr);
	
	newDataPtr = newDataPtr + TMcPreDataSize;
	
	TMiConstruction_DataPtr_PlaceHolder_Set(
		newDataPtr,
		TMiPlaceHolder_MakeFromIndex(newDescIndex));

	UUmAssertReadPtr(newDataPtr, dataSize);
	
	newInstanceDescriptor->size = dataSize;
	newInstanceDescriptor->dataPtr = newDataPtr;

	{
		UUtUns8*	dataPtr	= newDataPtr - TMcPreDataSize;
		UUtUns8*	swapCodes = newInstanceDescriptor->templatePtr->swapCodes;
		
		TMrUtility_VarArrayReset_Recursive(
			&swapCodes,
			&dataPtr,
			inVarArrayLength);
	}

	return newDataPtr;
}

static UUtError
TMiConstruction_Instance_Clean(
	TMtTemplateTag	inTemplateTag,
	const char*		inInstanceName)
{
	UUtUns8*				swapCode;
	UUtUns8*				dataPtr;
	
	TMtInstanceDescriptor*	targetDesc;
	
	targetDesc = TMiConstruction_FindInstanceDescriptor(inTemplateTag, inInstanceName);
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
	dataPtr = targetDesc->dataPtr - TMcPreDataSize;
	
	TMiConstruction_Clean_Recursive(
		&swapCode,
		&dataPtr);
	
	return UUcError_None;
}	

static void*
TMiConstruction_Instance_ResizeVarArray(
	void*			inDataPtr,
	UUtUns32		inVarArrayLength)
{
	TMtInstanceDescriptor*	instanceDesc;
	UUtUns32				newSize;
	UUtUns8*				newDataPtr;
	TMtPlaceHolder			placeHolder;
	
	TMmConstruction_Verify_InstanceData(inDataPtr);
	
	instanceDesc = TMiPlaceHolder_GetInstanceDesc((TMtPlaceHolder)inDataPtr);
	UUmAssert(instanceDesc != NULL);
	TMmConstruction_Verify_InstanceDesc(instanceDesc);
	
	newSize = instanceDesc->templatePtr->size + instanceDesc->templatePtr->varArrayElemSize * inVarArrayLength + TMcPreDataSize;
	
	newSize = (newSize + UUcProcessor_CacheLineSize_Mask) & ~UUcProcessor_CacheLineSize_Mask;

	newDataPtr = UUrMemory_Pool_Block_New(TMgConstructionPool, newSize);
	
	if(newDataPtr == NULL)
	{
		return NULL;
	}
	
	newDataPtr = newDataPtr + TMcPreDataSize;
	
	placeHolder = TMiConstruction_DataPtr_PlaceHolder_Get(inDataPtr);
	TMiConstruction_DataPtr_PlaceHolder_Set(newDataPtr, placeHolder);
	
	instanceDesc->dataPtr = newDataPtr;
	
	{
		UUtUns8*	dataPtr	= newDataPtr - TMcPreDataSize;
		UUtUns8*	swapCodes = instanceDesc->templatePtr->swapCodes;
		
		TMrUtility_VarArrayReset_Recursive(
			&swapCodes,
			&dataPtr,
			inVarArrayLength);
	}
	
	return newDataPtr;
}

static UUtError
TMiConstruction_Instance_CreatePlaceHolder(
	TMtTemplateTag	inTemplateTag,
	const char*		inInstanceName,
	TMtPlaceHolder	*outPlaceHolder)
{
	UUtError	error;
	UUtUns32	newIndex;
	TMtInstanceDescriptor*	targetDesc;
	
	error = 
		TMiConstruction_CreateNewDescriptor(
			inTemplateTag,
			inInstanceName,
			&newIndex);
	UUmError_ReturnOnError(error);
	
	*outPlaceHolder = TMmPlaceHolder_MakeFromIndex(newIndex);
	
	targetDesc = TMiPlaceHolder_GetInstanceDesc(*outPlaceHolder);
	targetDesc->flags |= TMcDescriptorFlags_PlaceHolder;
	
	return UUcError_None;
}
	
static void
TMiConstruction_Instance_Touch(
	TMtInstanceDescriptor*	inInstanceDescriptor)
{
	UUtUns8*				swapCodes;
	UUtUns8*				dataPtr;
	
	TMmConstruction_Verify_InstanceDesc(inInstanceDescriptor);
	
	if(inInstanceDescriptor->dataPtr != NULL)
	{
		swapCodes = inInstanceDescriptor->templatePtr->swapCodes;
		dataPtr = inInstanceDescriptor->dataPtr - TMcPreDataSize;
		
		TMiConstruction_Touch_Recursive(
			&swapCodes,
			&dataPtr);
	}
	inInstanceDescriptor->flags |= TMcDescriptorFlags_Touched;
}

/*
 * =========================================================================
 * P U B L I C   F U N C T I O N S
 * =========================================================================
 */

UUtError
TMrConstruction_Initialize(
	void)
{
	TMgConstruction_InstanceDescriptors = UUrMemory_Block_New(sizeof(TMtInstanceDescriptor) * TMcConstruction_MaxInstances);
	UUmError_ReturnOnNull(TMgConstruction_InstanceDescriptors);
	
	TMgConstruction_NameDescriptors = UUrMemory_Block_New(sizeof(TMtNameDescriptor) * TMcConstruction_MaxInstances);
	UUmError_ReturnOnNull(TMgConstruction_NameDescriptors);
	
	TMgConstructionPool = UUrMemory_Pool_New(TMcConstructionPoolChunkSize, UUcPool_Growable);
	UUmError_ReturnOnNull(TMgConstructionPool);

	return UUcError_None;
}

void
TMrConstruction_Terminate(
	void)
{
	UUtUns32	i;
	
	// dispose of memory
	if(TMgConstruction_InstanceDescriptors != NULL)
	{
		UUrMemory_Block_Delete(TMgConstruction_InstanceDescriptors);
		TMgConstruction_InstanceDescriptors = NULL;
	}
	if(TMgConstruction_NameDescriptors != NULL)
	{
		UUrMemory_Block_Delete(TMgConstruction_NameDescriptors);
		TMgConstruction_NameDescriptors = NULL;
	}
	
	if(TMgConstructionPool != NULL)
	{
		UUrMemory_Pool_Delete(TMgConstructionPool);
		TMgConstructionPool = NULL;
	}
	
	if(TMgConstruction_DuplicateIndexArray != NULL)
	{
		UUrMemory_Block_Delete(TMgConstruction_DuplicateIndexArray);
		TMgConstruction_DuplicateIndexArray = NULL;
	}
	
	if(TMgConstruction_TemplateDescriptors != NULL)
	{
		UUrMemory_Block_Delete(TMgConstruction_TemplateDescriptors);
	}

	if(TMgTemplateDefinitionsAlloced != NULL)
	{
		for(i = 0; i < TMgNumTemplateDefinitions; i++)
		{	
			if(TMgTemplateDefinitionArray[i].swapCodes != NULL)
			{
				UUrMemory_Block_Delete(TMgTemplateDefinitionArray[i].swapCodes);
			}
		}

		UUrMemory_Block_Delete(TMgTemplateDefinitionsAlloced);
		TMgTemplateDefinitionsAlloced = NULL;
	}
	
	if(TMgTemplateNameAlloced != NULL)
	{
		UUrMemory_String_Delete(TMgTemplateNameAlloced);
	}
}

static UUtError
TMiConstruction_PrepareBinaryFile(
	BFtFileRef*			inInstanceFileRef,
	BFtFileRef**		outFileRef,
	BFtFile**			outFile,
	UUtUns32*			outBytes,
	const char *		inExtension)
{
	UUtError	error;

	error = TMrUtility_DataRef_To_BinaryRef(inInstanceFileRef, outFileRef, inExtension);
	UUmError_ReturnOnError(error);

	UUmAssertReadPtr(*outFileRef, sizeof(UUtUns32));
	BFrFile_Delete(*outFileRef);

	error = BFrFile_Open(*outFileRef, "w", outFile);
	UUmError_ReturnOnError(error);
	
	if (*outFile == NULL) {
		return UUcError_Generic;
	}

	// CB: I think we skip the first 32 bytes of the file so that 0 or NULL will never be
	// a valid returned offset. certainly nothing seems to be ever written into these bytes.
	*outBytes = 32;
	error = BFrFile_SetPos(*outFile, *outBytes);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

UUtError
TMrConstruction_Start(
	BFtFileRef*			inInstanceFileRef)
{
	UUtError	error;
	UUtUns32	levelNum;
	
	UUmAssertReadPtr(TMgConstruction_InstanceDescriptors, sizeof(TMtInstanceDescriptor) * TMcConstruction_MaxInstances);
	UUmAssertReadPtr(TMgConstruction_NameDescriptors, sizeof(TMtNameDescriptor) * TMcConstruction_MaxInstances);
	
	TMgConstruction_NumInstanceDescriptors = 0;
	UUrMemory_Clear(TMgConstruction_InstanceDescriptors, sizeof(TMtInstanceDescriptor) * TMcConstruction_MaxInstances);
	
	TMgConstruction_NumNameDescriptors = 0;
	UUrMemory_Clear(TMgConstruction_NameDescriptors, sizeof(TMtNameDescriptor) * TMcConstruction_MaxInstances);
	
	TMgConstruction_NumTemplateDescriptors = 0;
	
	TMgConstruction_FileRef = inInstanceFileRef;

	UUmAssert(sizeof(TMtTemplateTag) == sizeof(UUtUns32));
	TMgRawTrackingHashTable = AUrHashTable_New(sizeof(TMtRawTracking), TMcMaxIdentifiers, AUrHash_4Byte_Hash, AUrHash_4Byte_IsEqual);
	TMgRawTrackingCount = 0;
	
	error = TMiConstruction_PrepareBinaryFile(inInstanceFileRef, &TMgConstruction_RawFileRef,
											&TMgConstruction_RawFile, &TMgConstruction_RawBytes, "raw");
	UUmError_ReturnOnError(error);
	
	error = TMiConstruction_PrepareBinaryFile(inInstanceFileRef, &TMgConstruction_SeparateFileRef, 
											&TMgConstruction_SeparateFile, &TMgConstruction_SeparateBytes, "sep");
	UUmError_ReturnOnError(error);
	
	error = 
		TMrUtility_LevelInfo_Get(
			inInstanceFileRef,
			&levelNum,
			NULL,
			NULL,
			&TMgConstruction_InstanceFileID);
	UUmError_ReturnOnError(error);
					
	return UUcError_None;
}


UUtError
TMrConstruction_Stop(
	UUtBool				inDumpStuff)
{
	UUtError	error;
	char		temp[128];
	char		fileName[128];
	char*		p;
	FILE*		file;

	UUtInt64	delta;

	delta = -UUrMachineTime_High();

	UUmAssert(TMgInGame == UUcFalse);
	
	fprintf(stderr, "running major check... \n");
	TMmConstruction_MajorCheck();
	
	fprintf(stderr, "writing to disk... \n");
	error = TMiConstruction_Save();
	UUmError_ReturnOnError(error);
	
	//fprintf(stderr, "verifying all touched... \n");
	//TMmConstruction_Verify_AllTouched();
	

	// dump place holders
		printf("dumping placeholders... "UUmNL);
		
		sprintf(temp, "%s", BFrFileRef_GetLeafName(TMgConstruction_FileRef));
		p = strchr(temp, '.');
		if(p != NULL) *p = 0;
		
		sprintf(fileName, "%s_phs.txt", temp);
		
		file = fopen(fileName, "wb");
		TMiConstruction_DumpPlaceHolders(fileName, file);
		fclose(file);
	
	// dump names
		printf("dumping names... "UUmNL);
		
		sprintf(temp, "%s", BFrFileRef_GetLeafName(TMgConstruction_FileRef));
		p = strchr(temp, '.');
		if(p != NULL) *p = 0;
		
		sprintf(fileName, "%s_nms.txt", temp);
		
		file = fopen(fileName, "wb");
		TMiConstruction_DumpInstanceNames(fileName, file);
		fclose(file);
	
	delta += UUrMachineTime_High();

	fprintf(stderr, "unaccounted time %f seconds\n", UUrMachineTime_High_To_Seconds(delta));

	AUrHashTable_Delete(TMgRawTrackingHashTable);
	TMgRawTrackingHashTable = NULL;
	
	return UUcError_None;
}

UUtError
TMrConstruction_Instance_Renew(
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName,				// NULL means unique
	UUtUns32			inInitialVarArrayLength,
	void*				*outNewInstance)
{
	void*	dataPtr;
	TMtInstanceDescriptor*	targetInstDesc;
	
	UUmAssert(TMgInGame == UUcFalse);
	
	*outNewInstance = NULL;
	
	if(inInstanceName == NULL)
	{
		return TMrConstruction_Instance_NewUnique(inTemplateTag, inInitialVarArrayLength, outNewInstance);
	}
	
	targetInstDesc = TMiConstruction_FindInstanceDescriptor(inTemplateTag, inInstanceName);
	if(targetInstDesc == NULL || targetInstDesc->dataPtr == NULL)
	{
		dataPtr = TMiConstruction_Instance_Create(inTemplateTag, inInstanceName, inInitialVarArrayLength);
		targetInstDesc = TMiConstruction_FindInstanceDescriptor(inTemplateTag, inInstanceName);
	}
	else
	{
		TMiConstruction_Instance_Clean(inTemplateTag, inInstanceName);
		dataPtr = TMiConstruction_Instance_ResizeVarArray(targetInstDesc->dataPtr, inInitialVarArrayLength);
		
		targetInstDesc->dataPtr = dataPtr;
	}
	
	if(dataPtr == NULL)
	{
		return UUcError_OutOfMemory;
	}
	
	UUmAssert(targetInstDesc != NULL);
	
	if(!(targetInstDesc->templatePtr->flags & TMcTemplateFlag_Registered))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "template not registered");
	}
	
	targetInstDesc->flags |= (TMtDescriptorFlags)(TMcDescriptorFlags_Touched | TMcDescriptorFlags_InBatchFile);

	// CB: don't flag this instance for deletion, it actually has some data now! (if it was initially created
	// just as a placeholder and then overwritten, it might be flagged for deletion, which would cause it
	// to be nuked in the cleanup phase even if its data was required)
	targetInstDesc->flags &= ~((TMtDescriptorFlags)TMcDescriptorFlags_DeleteMe);

	TMmConstruction_Verify_InstanceData(dataPtr);
	
	*outNewInstance = dataPtr;
	
	return UUcError_None;
}

static void *TMiConstruction_Binary_New(UUtUns32 inBytes, TMtTemplateTag inTemplateTag, UUtBool inSeparate)
{
	UUtUns32 alloc_size, *ptr;

	if (0 == inBytes) {
		return NULL;
	}

	UUmAssert(TMgInGame == UUcFalse);
	inBytes = UUmMakeMultipleCacheLine(inBytes);

	alloc_size = inBytes + 32;

	{
		TMtRawTracking *tracking_ptr;

		tracking_ptr = AUrHashTable_Find(TMgRawTrackingHashTable, &inTemplateTag);

		if (NULL == tracking_ptr) {
			TMtRawTracking tracking_block;

			tracking_block.identifier = inTemplateTag;
			tracking_block.raw_count = 0;
			tracking_block.separate_count = 0;
		
			tracking_ptr = AUrHashTable_Add(TMgRawTrackingHashTable, &tracking_block);

			TMgRawTrackingList[TMgRawTrackingCount] = inTemplateTag;
			TMgRawTrackingCount++;
		}

		UUmAssert(NULL != tracking_ptr);

		if (NULL != tracking_ptr) {
			if (inSeparate) {
				tracking_ptr->separate_count += alloc_size;
			} else {
				tracking_ptr->raw_count += alloc_size;
			}
		}
	}

	ptr = (UUtUns32 *) UUrMemory_Block_New(alloc_size);
	
	if (NULL != ptr) {
		ptr[0] = inBytes;
		ptr += 8;
	}

	return ptr;
}

void *TMrConstruction_Raw_New(UUtUns32 bytes, UUtUns32 alignment, TMtTemplateTag inTemplateTag)
{
	return TMiConstruction_Binary_New(bytes, inTemplateTag, UUcFalse);
}

void *TMrConstruction_Separate_New(UUtUns32 bytes, TMtTemplateTag inTemplateTag)
{
	return TMiConstruction_Binary_New(bytes, inTemplateTag, UUcTrue);
}

static void *TMiConstruction_Binary_Write(BFtFile *inFile, UUtUns32 *ioBytes, void *inPtr)
{
	UUtError error;
	UUtUns32 *privateInfo, offset;
	UUtUns32 chunkSize;

	UUmAssert(TMgInGame == UUcFalse);

	if (NULL == inPtr) { 
		return NULL;
	}

	privateInfo = inPtr;
	privateInfo -= 8;

	chunkSize = privateInfo[0];

	error = BFrFile_Write(inFile, chunkSize, inPtr);
	UUmAssert(UUcError_None == error);

	UUrMemory_Block_Delete(privateInfo);

	offset = *ioBytes;
	*ioBytes += chunkSize;

	return ((void *) offset);
}

void *TMrConstruction_Raw_Write(void *inPtr)
{
	return TMiConstruction_Binary_Write(TMgConstruction_RawFile, &TMgConstruction_RawBytes, inPtr);
}

void *TMrConstruction_Separate_Write(void *inPtr)
{
	return TMiConstruction_Binary_Write(TMgConstruction_SeparateFile, &TMgConstruction_SeparateBytes, inPtr);
}

UUtError
TMrConstruction_Instance_GetPlaceHolder(
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName,
	TMtPlaceHolder		*outPlaceHolder)
{
	UUtError				error;
	TMtInstanceDescriptor*	targetDesc;

	UUmAssertReadPtr(inInstanceName, sizeof(char));
	UUmAssertWritePtr(outPlaceHolder, sizeof(TMtPlaceHolder));
	UUmAssert(TMgInGame == UUcFalse);
	
	if('\0' == inInstanceName[0]) 
	{
		return UUcError_Generic;
	}
	
	*outPlaceHolder = 0;
	
	targetDesc = TMiConstruction_FindInstanceDescriptor(inTemplateTag, inInstanceName);
	if(targetDesc == NULL)
	{
		error =
			TMiConstruction_Instance_CreatePlaceHolder(
				inTemplateTag,
				inInstanceName,
				outPlaceHolder);
		UUmError_ReturnOnErrorMsg(error, "Could not create place holder");
		
		targetDesc = TMiConstruction_FindInstanceDescriptor(inTemplateTag, inInstanceName);
		UUmAssert(targetDesc != NULL);
	}
	
	TMiConstruction_Instance_Touch(targetDesc);
	
	// Always return an index
		*outPlaceHolder = TMmPlaceHolder_MakeFromIndex(targetDesc - TMgConstruction_InstanceDescriptors);
	
	return UUcError_None;
}

UUtBool
TMrConstruction_Instance_CheckExists(
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName)
{
	TMtInstanceDescriptor*	targetDesc;
	UUtUns8*				swapCode;
	UUtUns8*				dataPtr;
	UUtBool					result;
	
	targetDesc = TMiConstruction_FindInstanceDescriptor(inTemplateTag, inInstanceName);
	if(targetDesc == NULL || targetDesc->dataPtr == NULL)
	{
		return UUcFalse;
	}
	
	/*
	 * Recursively look for unique instances that got out of date
	 */
	{
		swapCode = targetDesc->templatePtr->swapCodes;
		dataPtr = targetDesc->dataPtr - TMcPreDataSize;
		
		result = TMiConstruction_CheckExists_Recursive(
			&swapCode,
			&dataPtr);
		
		if(result == UUcFalse) return UUcFalse;
	}
	
	TMiConstruction_Instance_Touch(targetDesc);
	
	targetDesc->flags |= (TMtDescriptorFlags)(TMcDescriptorFlags_Touched | TMcDescriptorFlags_InBatchFile);
		
	return UUcTrue;
}
	
UUtError
TMrConstruction_Instance_NewUnique(
	TMtTemplateTag		inTemplateTag,
	UUtUns32			inInitialVarArrayLength,
	void*				*outReference)
{
	void*					dataPtr;
	TMtTemplateDefinition* templatePtr;
	
	UUmAssert(TMgInGame == UUcFalse);
	
	templatePtr = TMrUtility_Template_FindDefinition(inTemplateTag);
	
	#if defined(DEBUGGING) && DEBUGGING
	
		if(!(templatePtr->flags & TMcTemplateFlag_Registered))
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "template not registered");
		}
	
	#endif
	
	dataPtr =
		TMiConstruction_Instance_Create(
			inTemplateTag,
			0,
			inInitialVarArrayLength);
	
	*outReference = dataPtr;
	
	return UUcError_None;
}

UUtError
TMrConstruction_Instance_Keep(
	TMtTemplateTag		inTemplateTag,
	char*				inInstanceName)
{
	TMtInstanceDescriptor*	targetDesc;
	
	targetDesc = TMiConstruction_FindInstanceDescriptor(inTemplateTag, inInstanceName);
	if(targetDesc == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find instance");
	}
	
	TMiConstruction_Instance_Touch(targetDesc);
	
	targetDesc->flags |= (TMtDescriptorFlags)(TMcDescriptorFlags_Touched | TMcDescriptorFlags_InBatchFile);
	
	return UUcError_None;
}

UUtError
TMrConstruction_ConvertToNativeEndian(
	void)
{
	
	return UUcError_None;
}

#if defined(DEBUGGING) && DEBUGGING

void TMrConstruction_MajorCheck(
	void)
{

}

#endif

UUtUns32
TMrConstruction_GetNumInstances(
	void)
{
	return TMgConstruction_NumInstanceDescriptors;
}
