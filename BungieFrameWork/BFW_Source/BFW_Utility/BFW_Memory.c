/*
	FILE:	BFW_Memory.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: May 18, 1997
	
	PURPOSE: memory manager
	
	Copyright 1997

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BFW.h"

#include "BFW_Memory.h"
#include "BFW_FileManager.h"
#include "BFW_BitVector.h"
#include "BFW_DebuggerSymbols.h"
#include "BFW_AppUtilities.h"
#include "BFW_Timer.h"

#if !defined(DEBUGGING_JUNKMEM)

	#define DEBUGGING_JUNKMEM 0

#endif

#define UUmMemoryBeginMagic	UUm4CharToUns32('B', 'G', 'M', 'G')
#define UUmMemoryEndMagic	UUm4CharToUns32('E', 'N', 'D', 'M')

#define	UUmMemory_EndSlopSize		(UUcProcessor_CacheLineSize * 512)

#define UUmMemLeakFileName	"MemLeak.txt"

char *UUgMemory_String_Zero = "";

typedef struct UUtMemory_Pool_Heap	UUtMemory_Pool_Heap;

struct UUtMemory_Pool_Heap
{
	char				*heapMemory;
	UUtUns32			nextFreeIndex;
	
	UUtMemory_Pool_Heap	*next;
};

struct UUtMemory_Pool
{
	UUtBool				fixed;
	UUtUns32			chunkSize;
	UUtMemory_Pool_Heap	*heapList;
	
	UUtMemory_Pool_Heap	*activeHeap;
};

typedef struct UUtMemory_Heap_Subheap	UUtMemory_Heap_Subheap;

struct UUtMemory_Heap_Subheap
{
	char*					memory;
	UUtUns32*				blockBV;
	
	UUtUns32				blocksLeft;
	
	UUtMemory_Heap_Subheap	*next;
};

struct UUtMemory_Heap
{
	UUtBool					fixed;
	UUtUns32				chunkSize;
	UUtMemory_Heap_Subheap	*subheapList;
	UUtUns32				blocksPerHeap;
};

struct UUtMemory_Array
{
	UUtUns32	elemSize;
	UUtUns32 	numElemsUsed;
	UUtUns32	allocChunkSize;
	UUtUns32	numElemsAlloced;
	void		*data;
	
};

struct UUtMemory_ParallelArray_Member
{
	UUtUns32	elemSize;
	void		*data;
	
};

struct UUtMemory_ParallelArray
{
	UUtUns32 						numElemsUsed;
	UUtUns32						numElemsAlloced;
	UUtUns32						numMemberArrays;
	UUtUns32						allocChunkSize;
	UUtMemory_ParallelArray_Member	members[UUcMemory_ParallelArray_MaxMembers];
	
};

#if DEBUGGING_MEMORY
	
	#define UUcMemory_MinUsageThreshold	(10 * 1024 * 1024)
	UUtUns32	gMemoryTracker_NumRecords = 0;
	UUtUns32	gMemoryTracker_NumBlocks;
	
	typedef struct UUtMemory_LeakStack
	{
		char*		fileName;
		UUtInt32	lineNum;
		UUtUns32	timeStamp;
		const char*	comment;
		
	} UUtMemory_LeakStack;

	#define cMaxLeakStackLength	30
	UUtUns16					gLeakStack_TOS = 0;
	static UUtMemory_LeakStack	gLeakStack[cMaxLeakStackLength];

	UUtUns32					gStartTime;

	UUtInt32	gCurMemUsage = 0;
	UUtInt32	gMaxMemUsage = 0;
	
	UUtBool		gReport_ForceGlobal = UUcFalse;
	UUtBool		UUgMemory_VerifyForceDisable	= UUcFalse;
	UUtBool		UUgMemory_VerifyForceEnable		= UUcFalse;
	UUtBool		UUgMemory_VerifyEnable			= UUcTrue;
	
	#if !defined(UUmDebugMemAggressive)
	
		#define UUmDebugMemAggressive 0
	
	#endif
	
	#if !defined(UUmDebugDelete)
	
		#define UUmDebugDelete	0
	
	#endif
	
	#define UUcMaxDebugFileLength	256
	#define UUcMaxCallChainLength	100
	
	typedef struct UUtBookkeepBlock
	{
		struct UUtBookkeepBlock	*next;
		
		UUtUns32				numChains;
		UUtUns32				callChainList[UUcMaxCallChainLength];
		
		char					file[UUcMaxDebugFileLength];
		UUtInt32				line;
		UUtUns32				realBytes;			// memory requested
		UUtUns32				allocedBytes;		// how many actually allocated (to get align, safety etc)
		UUtInt32				*beginMagic;
		UUtInt32				*endMagic;
		void*					realBlock;
		
		#if defined(UUmDebugMemAggressive) && UUmDebugMemAggressive
			
			UUtInt32				*endAggressive;
			
		#endif
		
		UUtInt32			checkSum;
		
		UUtUns32			timeStamp;
		
		UUtUns32			numTimesRealloced;
		
		UUtUns16			reported;
		
		UUtUns16			stackLevel;
		
	} UUtBookkeepBlock;
	
	#if defined(UUmDebugDelete) && UUmDebugDelete
	
		typedef struct UUtDeletedBookkeepBlock
		{
			struct UUtDeletedBookkeepBlock	*next;
			
			char							file[UUcMaxDebugFileLength];
			UUtInt32						line;
			
			void							*addr;
			
		} UUtDeletedBookkeepBlock;
	
		UUtDeletedBookkeepBlock	*gDeletedBlockList = NULL;
		
	#endif
	
	#define UUcMemoryTracker_FileLength	(32)
	
	typedef struct UUtMemoryTracker
	{
		char		fileName[UUcMemoryTracker_FileLength];
		UUtUns32	lineNum;
		
		UUtUns32	memoryAllocated;
		
	} UUtMemoryTracker;
	
	/* Eventually move this into a per instance structure */
	UUtBookkeepBlock	*gBlockListHead = NULL;

	AUtSharedElemArray*	gMemoryTracker	= NULL;
	
#endif /*	DEBUGGING_MEMORY	*/


/*---------- out-of-memory stuff */

/*
	Since Oni doesn't handle allocation failure, I'm doing it here.
	Anyone who has a problem with that can start working on the Mac build
	and then come yell at me :o)
*/ 

enum {
	RELIEF_FUND_SIZE= 1024 * 32
};

static void *stefans_emergency_relief_fund= NULL;

static void out_of_memory(
	void)
{
	UUmAssert(stefans_emergency_relief_fund);
	free(stefans_emergency_relief_fund);
	stefans_emergency_relief_fund= NULL;
	AUrMessageBox(AUcMBType_OK, "Sorry, Oni has run out of memory!");
	
#if UUmPlatform == UUmPlatform_Mac
	// somehow, someway, even calling ExitToShell() is causing atexit-registered functions
	// to be triggered, which tends to blow up on the Mac
	{
		// this makes sure that exit-procs don't get called
		extern UUtBool gTerminated; // BFW_Platform_MacOS.c
		gTerminated= UUcTrue;
		// die!!!
		ExitToShell();
	}
#endif
	
	exit(1);

	return;
}


static void *iGetRealPtr(void *inFakePtr)
{
	void *realMemory = *(UUtUns32 **)((char *)inFakePtr - 4);

	return realMemory;
}

static UUtUns32 iSizeToAllocSize(UUtUns32 inSize)
{
	UUtUns32 mungedSize		= (inSize + UUcProcessor_CacheLineSize_Mask) & ~UUcProcessor_CacheLineSize_Mask;
	UUtUns32 allocateSize	= mungedSize + UUcProcessor_CacheLineSize + 4;

	return allocateSize;
}

void *UUrAlignMemory(void *inMemory)
{
	UUtUns32	alignedMemory;
	
	alignedMemory = (UUtUns32)(((UUtUns32)inMemory + UUcProcessor_CacheLineSize_Mask) &
		~UUcProcessor_CacheLineSize_Mask);
	
	return (void *) alignedMemory;
}

/*
 * aligns and increments by four,
 */
static void *iAlignMemory_PlusPointerSpace(void *inMemory)
{
	UUtUns32	alignedMemory;
	
	alignedMemory = (UUtUns32)(((UUtUns32)inMemory + UUcProcessor_CacheLineSize_Mask + 4) &
		~UUcProcessor_CacheLineSize_Mask);
	
	return (void *) alignedMemory;
}

static void iSetRealPtr(void *inAlignedMemory, void *inRealPtr)
{
	*(UUtUns32 *)((char *)inAlignedMemory - 4) = (UUtUns32)inRealPtr;
}

static void *iAlignMemory_PlusPointerSpaceUpdate(void *inMemory)
{
	void *aligned;

	aligned = iAlignMemory_PlusPointerSpace(inMemory);
	iSetRealPtr(aligned, inMemory);

	return aligned;
}

#if DEBUGGING_MEMORY

/*******************************************************************************
 *
 ******************************************************************************/
UUtBool
UUrMemory_SetVerifyEnable(
	UUtBool inEnable)
{
	UUtBool prev_value = UUgMemory_VerifyEnable;

	UUgMemory_VerifyEnable = inEnable;

	return prev_value;
}

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_SetVerifyForce(
	UUtBool inForce)
{
	if (inForce) {
		UUgMemory_VerifyForceEnable = UUcTrue;
		UUgMemory_VerifyForceDisable = UUcFalse;
	} else {
		UUgMemory_VerifyForceEnable = UUcFalse;
		UUgMemory_VerifyForceDisable = UUcTrue;
	}
}

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_Leak_ForceGlobal_Begin_Real(
	void)
{
	gReport_ForceGlobal = UUcTrue;
}

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_Leak_ForceGlobal_End_Real(
	void)
{
	gReport_ForceGlobal = UUcFalse;
}

/*******************************************************************************
 *
 ******************************************************************************/
static UUtInt16
UUrMemory_Tracker_Compare(
	UUtMemoryTracker*	inElemA,
	UUtMemoryTracker*	inElemB)
{
	UUtInt16	result;
	
	result = strcmp(inElemA->fileName, inElemB->fileName);
	if(result != 0) return result;
	
	if(inElemA->lineNum > inElemB->lineNum) return 1;
	if(inElemA->lineNum < inElemB->lineNum) return -1;
	
	return 0;
}

/*******************************************************************************
 *
 ******************************************************************************/
static UUtBookkeepBlock *
UUiMemory_Debug_FindBookkeepBlock(
	void*	inMemory)
{
	UUtBookkeepBlock	*bookkeepBlock;
	
	for(
		bookkeepBlock = gBlockListHead;
		bookkeepBlock;
		bookkeepBlock = bookkeepBlock->next)
	{
		if((UUtUns32)inMemory > (UUtUns32)bookkeepBlock->beginMagic &&
			(UUtUns32)inMemory < (UUtUns32)bookkeepBlock->endMagic)
		{
			break;
		}
	}
	
	return bookkeepBlock;
}

/*******************************************************************************
 *
 ******************************************************************************/
static void 
UUiMemory_Debug_VerifyBlock(
	UUtBookkeepBlock*	inBookkeepBlock)
{
	
	if(*inBookkeepBlock->beginMagic != UUmMemoryBeginMagic)
	{
		UUmDebugStr("beginMagic is bogus");
		return;
	}
	
	if(*inBookkeepBlock->endMagic != UUmMemoryEndMagic)
	{
		UUmDebugStr("endMagic is bogus");
		return;
	}
	
	#if defined(UUmDebugMemAggressive) && UUmDebugMemAggressive
	{
		UUtInt32	*curLong;
		
		for(
			curLong = inBookkeepBlock->endMagic + 1;
			curLong < inBookkeepBlock->endAggressive;
			curLong++)
		{
			if(*curLong != UUmMemoryBlock_Garbage)
			{
				UUmDebugStr("You have overwritten memory.");
				return;
			}
		}
	}
	#endif
}

/*******************************************************************************
 *
 ******************************************************************************/
void 
UUrMemory_Block_VerifyList(
	void)
{
	UUtBookkeepBlock	*bookkeepBlock;
	
	if (UUgMemory_VerifyForceDisable || !(UUgMemory_VerifyForceEnable || UUgMemory_VerifyEnable))
		return;

	for(
		bookkeepBlock = gBlockListHead;
		bookkeepBlock;
		bookkeepBlock = bookkeepBlock->next)
	{
		UUiMemory_Debug_VerifyBlock(bookkeepBlock);
	}
}

/*******************************************************************************
 *
 ******************************************************************************/
void 
UUrMemory_Block_Verify(
	void*	inMemory)
{
	UUtBookkeepBlock	*bookkeepBlock;
	
	bookkeepBlock = UUiMemory_Debug_FindBookkeepBlock(inMemory);
	
	if (NULL == bookkeepBlock)
	{
		UUmDebugStr("This is not a valid BFW Memory Block");
	}
	
	UUiMemory_Debug_VerifyBlock(bookkeepBlock);
}

void UUrMemory_Clear_Debug(void *inDst, UUtUns32 inSize)
{
	UUmAssertWritePtr(inDst, inSize);

	memset(inDst, 0, inSize);	// replace with something else later
}


void UUrMemory_MoveFast_Debug(const void *inSrc, void *inDst, UUtUns32 inSize)  // non overlapping buffers only
{
	UUtUns32 inSrcInt = (UUtUns32) inSrc;
	UUtUns32 inDstInt = (UUtUns32) inDst;

	UUmAssertReadPtr(inSrc, inSize);
	UUmAssertWritePtr(inDst, inSize);

	// don't overlap ( end of src before beginning of dst or beginning of src after end of dst
	UUmAssert(((inSrcInt + inSize) <= inDstInt) || (inSrcInt >= (inDstInt + inSize)));

	memcpy(inDst, inSrc, inSize);
}

void UUrMemory_MoveOverlap_Debug(const void *inSrc, void *inDst, UUtUns32 inSize)
{
	UUmAssertReadPtr(inSrc, inSize);
	UUmAssertWritePtr(inDst, inSize);

	memmove(inDst, inSrc, inSize);
}

void UUrMemory_Set8_Debug(void *inDst, UUtUns8 inFillValue, UUtUns32 inSize)
{
	UUmAssertWritePtr(inDst, inSize);

	memset(inDst, inFillValue, inSize);
}

#endif

void
UUrMemory_ArrayElement_Delete(
	const void*	inBasePtr,
	UUtUns32	inDeleteElemIndex,
	UUtUns32	inArrayLength,
	UUtUns32	inElemSize)
{
	UUmAssertReadPtr(inBasePtr, inElemSize * inArrayLength);
	UUmAssert(inDeleteElemIndex < inArrayLength);
	UUmAssert(inElemSize > 0);
	
	if(inDeleteElemIndex == inArrayLength - 1) return;	// no need to delete because it is the last element
	
	memmove(
		(char*)inBasePtr + inDeleteElemIndex * inElemSize,
		(char*)inBasePtr + (inDeleteElemIndex + 1) * inElemSize,
		(inArrayLength - inDeleteElemIndex - 1) * inElemSize);
}

void UUrMemory_Set16(void *inDst, UUtUns16 inFillValue, UUtUns32 inSize)
{
	UUtUns16 *p16Dst = (UUtUns16 *) inDst;

	UUmAssertWritePtr(inDst, inSize);

	if (inSize & 0x1) {
		UUtUns8 *firstBytePtr = (UUtUns8 *) &inFillValue;
		UUtUns8 *bytePtr = (UUtUns8 *) inDst;
		UUtUns16 newFillValue = ((inFillValue & 0xff) << 8) | ((inFillValue & 0xFF00) >> 8);

		*bytePtr = *firstBytePtr;

		UUrMemory_Set16(bytePtr + 1, newFillValue, inSize - 1);
	}
	else {
		while(inSize > 0)
		{
			*p16Dst = inFillValue;

			p16Dst++;
			inSize -= 2;
		}
	}
}

void UUrMemory_Set32(void *inDst, UUtUns32 inFillValue, UUtUns32 inSize)
{
	UUtUns32 original_write_size = inSize;
	UUtUns32 *p32Dst = (UUtUns32 *) inDst;
	
	UUmAssert(((UUtUns32)inDst & 0x3) == 0);
	UUmAssertWritePtr(inDst, inSize);
	
	UUmAssert(UUcProcessor_CacheLineSize == 32);
	
	if (inSize >= UUcProcessor_CacheLineSize) {

		// align to cache line boundary
		while((UUtUns32)p32Dst & UUcProcessor_CacheLineSize_Mask)
		{
			*p32Dst++ = inFillValue;
			inSize -= 4;
		}

		// this appears to always be the case
		while(inSize > UUcProcessor_CacheLineSize)
		{
			UUrProcessor_ZeroCacheLine(p32Dst, 0);
			
			*p32Dst++ = inFillValue;
			*p32Dst++ = inFillValue;
			*p32Dst++ = inFillValue;
			*p32Dst++ = inFillValue;
			*p32Dst++ = inFillValue;
			*p32Dst++ = inFillValue;
			*p32Dst++ = inFillValue;
			*p32Dst++ = inFillValue;
			
			inSize -= UUcProcessor_CacheLineSize;
		}
	}

	while(inSize > 3)
	{
		*p32Dst++ = inFillValue;

		inSize -= 4;
	}

	if (inSize > 0)
	{
		UUtUns8 *p8Src = (UUtUns8 *) &inFillValue;
		UUtUns8 *p8Dst = (UUtUns8 *) p32Dst;

		while(inSize > 0)
		{
			*p8Dst++ = *p8Src++;
			inSize--;
		}
	}
	
}

/*******************************************************************************
 * NOTE: I know my memory begins on a cache aligned boundary and has at least
 *			a cache line of slop at the end
 ******************************************************************************/
static void 
UUiMemory_Block_Copy(
	void*		inDest,
	void*		inSrc,
	UUtUns32	inLen)
{
	UUtInt32	i;
	double	*srcPtr, *destPtr;
	register double	ft0, ft1, ft2, ft3;

	UUmAssertReadPtr(inSrc, inLen);
	UUmAssertWritePtr(inDest, inLen);

	srcPtr = (double *)inSrc;
	destPtr = (double *)inDest;
	
	UUmAssert(((unsigned long)inDest & UUcProcessor_CacheLineSize_Mask) == 0);
	UUmAssert(((unsigned long)inSrc & UUcProcessor_CacheLineSize_Mask) == 0);
	
	for(i = (inLen + UUcProcessor_CacheLineSize_Mask) >> UUcProcessor_CacheLineBits; i-- > 0;)
	{
		UUrProcessor_ZeroCacheLine(destPtr, 0);
		
		#if UUcProcessor_CacheLineBits == 5
			
			/* XXX - Does this suck on intel ? */
			
			ft0 = srcPtr[0];
			ft1 = srcPtr[1];
			ft2 = srcPtr[2];
			ft3 = srcPtr[3];
			srcPtr += 4;
			
			destPtr[0] = ft0;
			destPtr[1] = ft1;
			destPtr[2] = ft2;
			destPtr[3] = ft3;
			destPtr += 4;
			
		#else
		
			#error handle me
		
		#endif
	}
}

/*******************************************************************************
 * Make sure memory is cache line aligned and has at least an extra cache line 
 * at the end. This lets me make some very convient assumptions about my memory!
 ******************************************************************************/
void *
UUrMemory_Block_New_Real(
	UUtUns32	inSize)
{
	void		*newMemory;
	UUtUns32	alignedMemory;
	UUtUns32	mungedSize;

	if (0 == inSize) 
	{
		return NULL;
	}
	
	// Make sure that we allocate memory in cache size chunks
		mungedSize = (inSize + UUcProcessor_CacheLineSize_Mask) & ~UUcProcessor_CacheLineSize_Mask;

	// Allocate enough room so that return address is cache line aligned
		newMemory = malloc(mungedSize + UUcProcessor_CacheLineSize + 4);
		if(newMemory == NULL)
		{
			out_of_memory();
			return NULL;
		}
	
	// Align to cache line
		alignedMemory = ((UUtUns32)newMemory + UUcProcessor_CacheLineSize_Mask + 4) &
			~UUcProcessor_CacheLineSize_Mask;
	
	// Keep the original pointer
		*(UUtUns32 *)(alignedMemory - 4) = (UUtUns32)newMemory;
	
	return (void *)(alignedMemory);
}


// optimization note: on the mac there is NewPtrClear which might be worth doing
void *
UUrMemory_Block_NewClear_Real(
	UUtUns32	inSize)
{
	void*	newMemory;
	
	newMemory = UUrMemory_Block_New_Real(inSize);

	if (NULL != newMemory) {
		UUrMemory_Clear(newMemory, inSize);
	}
		
	return newMemory;
}

#if DEBUGGING_MEMORY

static void
UUrMemory_Block_RecordUsage(
	void)
{
	UUtBookkeepBlock*	curBookkeepBlock;
	UUtMemoryTracker	curTracker;
	UUtError			error;
	UUtUns32			curTrackerIndex;
	UUtMemoryTracker*	addedTracker;

	if(gMemoryTracker == NULL) return;
	
	gMemoryTracker_NumRecords++;
	
	AUrSharedElemArray_Reset(gMemoryTracker);
	
	gMemoryTracker_NumBlocks = 0;
	
	for(
		curBookkeepBlock = gBlockListHead;
		curBookkeepBlock != NULL;
		curBookkeepBlock = curBookkeepBlock->next)
	{
		UUrString_Copy(curTracker.fileName, curBookkeepBlock->file, UUcMemoryTracker_FileLength);
		
		curTracker.lineNum = curBookkeepBlock->line;
		curTracker.memoryAllocated = 0;
		
		error = 
			AUrSharedElemArray_AddElem(
				gMemoryTracker,
				&curTracker,
				&curTrackerIndex);
		if(error != UUcError_None) return;
		
		addedTracker = (UUtMemoryTracker*)AUrSharedElemArray_GetList(gMemoryTracker) + curTrackerIndex;
		addedTracker->memoryAllocated += curBookkeepBlock->allocedBytes;
		
		gMemoryTracker_NumBlocks++;
	}
}

/*******************************************************************************
 * Since we want to return memory that is cache line aligned and has an extra 
 * cache line of slop at the end this code embarks the reader on a journey to hell.
 * enjoy.
 ******************************************************************************/
void *
UUrMemory_Block_New_Debug(
	char		*inFile,
	UUtInt32	inLine,
	UUtUns32	inSize)
{
	UUtInt32			allocSize = 0;
	UUtBookkeepBlock	*newBookkeepBlock;
	char				*newRealBlock;
	UUtUns32			mungedSize;

	if (0 == inSize)
	{
		return NULL;
	}
	
	UUrMemory_Block_VerifyList();
	
	// Make sure that we allocate memory in cache size chunks
		mungedSize = (inSize + UUcProcessor_CacheLineSize_Mask) & ~UUcProcessor_CacheLineSize_Mask;
	
	// Add space for the begin magic cookie, need to keep start address cache line aligned
		allocSize = mungedSize + UUcProcessor_CacheLineSize;
	
	// Add space for the end magic cookie
		allocSize += 4;
	
	#if defined(UUmDebugMemAggressive) && UUmDebugMemAggressive
		
		// Add enough space for end slop
			allocSize += UUmMemory_EndSlopSize;
		
	#endif

	// Allocated the debug block
		newBookkeepBlock = (UUtBookkeepBlock *)UUrMemory_Block_New_Real(sizeof(UUtBookkeepBlock));
		if (NULL == newBookkeepBlock)
		{
			return NULL;
		}

	// Allocate the actual block
		newRealBlock = (char *)UUrMemory_Block_New_Real(allocSize);
		if (NULL == newRealBlock)
		{
			UUrMemory_Block_Delete_Real(newBookkeepBlock);
			return NULL;
		}
		
		newBookkeepBlock->realBlock = newRealBlock;
		
	#if defined(UUmDebugMemAggressive) && UUmDebugMemAggressive
		
		newBookkeepBlock->endAggressive = (UUtInt32 *)(newRealBlock + allocSize);
		
	#endif
	
	// Skip over the initial cache line for the begin magic cookie
		newRealBlock += UUcProcessor_CacheLineSize;
	
	// construct the bookkeeping block
	
		// get the call chain
		newBookkeepBlock->numChains =
			UUrStack_GetPCChain(
				newBookkeepBlock->callChainList,
				UUcMaxCallChainLength);
		
		{
			char* rc;
			
			rc = strrchr(inFile, BFcPathSeparator);
			if(rc != NULL)
			{
				UUrString_Copy(newBookkeepBlock->file, rc+1, 128);
			}
			else
			{
				rc = strrchr(inFile, ':');
				if(rc != NULL)
				{
					UUrString_Copy(newBookkeepBlock->file, rc+1, 128);
				}
				else
				{
					UUrString_Copy(newBookkeepBlock->file, inFile, 128);
				}
			}
		}
		
		newBookkeepBlock->line = inLine;
		newBookkeepBlock->realBytes = inSize;
		newBookkeepBlock->allocedBytes = allocSize;
		newBookkeepBlock->beginMagic = (UUtInt32 *)(newRealBlock - 4);
		newBookkeepBlock->endMagic	= (UUtInt32 *)(newRealBlock + mungedSize);
		newBookkeepBlock->timeStamp = UUrMachineTime_Sixtieths() - gStartTime;
		newBookkeepBlock->stackLevel = gReport_ForceGlobal ? 0 : gLeakStack_TOS - 1;
		newBookkeepBlock->reported = UUcFalse;
		newBookkeepBlock->numTimesRealloced = 0;
	
	#if 0
	// add this to the shared array
	if(gMemoryTracker != NULL)
	{
		UUtMemoryTracker	curTracker;
		UUtError			error;
		UUtUns32			curTrackerIndex;
		UUtMemoryTracker*	addedTracker;
		
		curTracker.fileName = inFile;
		curTracker.lineNum = inLine;
		curTracker.memoryAllocated = 0;
		
		error = 
			AUrSharedElemArray_AddElem(
				gMemoryTracker,
				&curTracker,
				&curTrackerIndex);
		if(error != UUcError_None) return NULL;
		
		addedTracker = (UUtMemoryTracker*)AUrSharedElemArray_GetList(gMemoryTracker) + curTrackerIndex;
		addedTracker->memoryAllocated += allocSize;
	}
	#endif
	
	// set our magic values that warn us if we wandered past the ends
		*newBookkeepBlock->beginMagic = UUmMemoryBeginMagic;
		*newBookkeepBlock->endMagic = UUmMemoryEndMagic;

	// Clear out the usable memory to garbage
		#if defined(DEBUGGING_JUNKMEM) && DEBUGGING_JUNKMEM
			UUrMemory_Set32(newRealBlock, UUmMemoryBlock_Garbage, mungedSize);
		#endif
		
	#if defined(UUmDebugMemAggressive) && UUmDebugMemAggressive

		// Clear out the end slop space to garbage
			UUrMemory_Set32(newBookkeepBlock->endMagic + 1, UUmMemoryBlock_Garbage, UUmMemory_EndSlopSize);

	#endif
	
	/* add to our bookkeeping list */
	newBookkeepBlock->next = gBlockListHead;
	gBlockListHead = newBookkeepBlock;
	
	gCurMemUsage += inSize;
	if(gCurMemUsage > gMaxMemUsage)
	{
		gMaxMemUsage = gCurMemUsage;
		
		if(gCurMemUsage >= UUcMemory_MinUsageThreshold)
		{
			UUrMemory_Block_RecordUsage();
		}
	}
	
	return newRealBlock;
}

void *
UUrMemory_Block_NewClear_Debug(
	char		*inFile,
	UUtInt32	inLine,
	UUtUns32	inSize)
{
	void*	newMemory;
	
	newMemory = UUrMemory_Block_New_Debug(inFile, inLine, inSize);

	if (NULL != newMemory) {
		UUrMemory_Clear(newMemory, inSize);
	}
		
	return newMemory;
}

#endif

/*******************************************************************************
 *
 ******************************************************************************/
void 
UUrMemory_Block_Delete_Real(
	void		*inMemory)
{
	unsigned long *realMemory;

	UUmAssert(NULL != inMemory);	// we could use real memory w/ assertions some day
	
	realMemory = (unsigned long *)((char *)inMemory - 4);
	
	free((void *)*realMemory);
}

#if DEBUGGING_MEMORY

/*******************************************************************************
 *
 ******************************************************************************/
void 
UUrMemory_Block_Delete_Debug(
	char			*inFile,
	UUtInt32		inLine,
	void			*inMemory)
{
	UUtBookkeepBlock	*curBlock, *prevBlock;
	void				*targetDeleteMemory;

	UUmAssert(NULL != inMemory);
	
	(void)(inLine);
	(void)(inFile);
	
	UUrMemory_Block_VerifyList();
	for(
		curBlock = gBlockListHead, prevBlock = NULL;
		curBlock;
		curBlock = curBlock->next)
	{
		if((UUtUns32)curBlock->beginMagic < (UUtUns32)inMemory && (UUtUns32)inMemory < (UUtUns32)curBlock->endMagic)
		{
			break;
		}
		
		prevBlock = curBlock;
	}
	
	if(curBlock == NULL)
	{
		#if defined(UUmDebugDelete) && UUmDebugDelete
		{
			UUtDeletedBookkeepBlock	*curDeletedBlock;
			
			/* Traverse the deleted block list */
			
			curDeletedBlock = gDeletedBlockList;
			
			while(curDeletedBlock)
			{
				if(curDeletedBlock->addr == inMemory)
				{
					break;
				}
				
				curDeletedBlock = curDeletedBlock->next;
			}
			
			if(curDeletedBlock == NULL)
			{
				UUmDebugStr("This block was never allocated nor deleted by me...");
			}
			else
			{
				char buffer[UUcMaxFunctionHistoryLength + 512];
				
				sprintf(
					buffer,
					"This block was deleted before by:"UUmNL"File: %s"UUmNL"Line: %d"UUmNL"FuncHistory: Not Available"UUmNL,
					curDeletedBlock->file,
					curDeletedBlock->line);
				
				UUrEnterDebugger(buffer);
			}
		}
		#else
		
			UUmDebugStr("This is not a valid UU memory block");
		
		#endif
	}
	
	UUmAssert((char *)inMemory == (char *)curBlock->beginMagic + 4);
	
	
	targetDeleteMemory = curBlock->realBlock;
		
	
	/* Remove this block from the list of alloced blocks */
	if(prevBlock == NULL)
	{
		gBlockListHead = curBlock->next;
	}
	else
	{
		prevBlock->next = curBlock->next;
	}
	
	/* Garbage Out the memory */
	#if defined(DEBUGGING_JUNKMEM) && DEBUGGING_JUNKMEM
		UUrMemory_Set32(targetDeleteMemory, UUm4CharToUns32('D', 'L', 'T', 'D'), curBlock->allocedBytes);
	#endif

	
	{
		/* Actually delete the memory */
		
		UUrMemory_Block_Delete_Real(targetDeleteMemory);
	
	}
	
	#if defined(UUmDebugDelete) && UUmDebugDelete
	{	
		UUtDeletedBookkeepBlock	*newDeletedBlock;
		
		/* Add a new deleted block to gDeletedBlockList */
		 
		 newDeletedBlock = (UUtDeletedBookkeepBlock *)UUrMemory_Block_New_Real(sizeof(UUtDeletedBookkeepBlock));
		 
		 if(newDeletedBlock != NULL)
		 {
		 	
		 	UUrString_Copy(newDeletedBlock->file, inFile, UUcMaxDebugFileLength);
		 	newDeletedBlock->line = inLine;
		 	newDeletedBlock->addr = inMemory;
		 	
		 	newDeletedBlock->next =  gDeletedBlockList;
		 	gDeletedBlockList = newDeletedBlock;
		 }
		 else
		 {
		 	/* Do nothing */
		 }
	}	
	#endif
	
	gCurMemUsage -= curBlock->realBytes;
	
	UUmAssert(gCurMemUsage >= 0);
	
	/* Delete the book keeping memory */
	UUrMemory_Block_Delete_Real(curBlock);	
}

#endif
	
/*

  starting at memory location inMemory shift everything shift
  bytes forward for len bytes (non destructively)

 */
static void iShiftMemory(UUtUns32 inMemory, UUtUns32 len, int shift)
{
	UUtUns32 dst;
	
	dst = inMemory;
	dst += shift;

	UUrMemory_MoveOverlap((void *)inMemory, (void *) dst, len);
}

/*******************************************************************************

 realloc returns a void pointer to the reallocated (and possibly moved) memory block. 
 The return value is NULL if the size is zero and the buffer argument is not NULL, or 
 if there is not enough available memory to expand the block to the given size. In the first case,
 the original block is freed. In the second, the original block is unchanged. The return value points 
 to a storage space that is guaranteed to be suitably aligned for storage of any type of object. 
 To get a pointer to a type other than void, use a type cast on the return value.

 ******************************************************************************/
void *
UUrMemory_Block_Realloc_Real(
	void		*inMemory,
	UUtUns32	inSize)
{
	UUtUns32		inMemoryInt;
	UUtUns32		inRealMemoryInt;
	UUtUns32		inOffset;

	UUtUns32		outMemoryInt;
	UUtUns32		outRealMemoryInt;
	UUtUns32		outOffset;

	unsigned long*	realMemory;
	void*			newMemory;
	void*			alignedMemory;
	UUtUns32		allocateSize;

	// if we are deleting go do that and leave me in peace
	if (0 == inSize) {
		if (NULL != inMemory) {
			UUrMemory_Block_Delete_Real(inMemory);
		}

		return NULL;
	}

	// get our size and our real ptr
	allocateSize = iSizeToAllocSize(inSize);

	// get our real ptr to pass to realloc
	if (inMemory != NULL) {
		realMemory = iGetRealPtr(inMemory);
	}
	else {
		realMemory = NULL;
	}

	// realloc our new block
	newMemory = realloc(realMemory, allocateSize);

	if (NULL == newMemory) {
		out_of_memory();
		return NULL;
	}

	// align our memory
	alignedMemory = iAlignMemory_PlusPointerSpace(newMemory);

	// fix the alignment of the allocated chunk
	if ((NULL != inMemory) && (NULL != newMemory)) {
		inMemoryInt			= (UUtUns32) inMemory;
		inRealMemoryInt		= (UUtUns32) realMemory;
		inOffset			= inMemoryInt - inRealMemoryInt;

		outMemoryInt		= (UUtUns32) alignedMemory;
		outRealMemoryInt	= (UUtUns32) newMemory;
		outOffset			= outMemoryInt - outRealMemoryInt;

		// in this case a fixup needs to happen
		if (inOffset != outOffset) {
			iShiftMemory(outRealMemoryInt + inOffset, inSize, outOffset - inOffset);
		}
	}

	// we have to set the real ptr after our alignment of the memory happened
	iSetRealPtr(alignedMemory, newMemory);

	return alignedMemory;
}

#if DEBUGGING_MEMORY

/*******************************************************************************
 *
 *
 * case 1 (NULL == inMemory) this is an allocate
 * case 2 (NULL == inSize) this is a delete
 * case 3 this may move the memory
 *
 *
 ******************************************************************************/
void *
UUrMemory_Block_Realloc_Debug(
	char		*inFile,
	UUtInt32	inLine,
	void		*inMemory,
	UUtUns32	inSize)
{	
	UUrMemory_Block_VerifyList();

 // case 1 (NULL == inMemory) this is an allocate
 // case 2 (NULL == inSize) this is a delete
 // case 3 traditional realloc (this may move the memory, change the bookeep block

 // case 1 (NULL == inMemory) this is an allocate
	if (NULL == inMemory) {
		void *allocated = UUrMemory_Block_New_Debug(inFile, inLine, inSize);
		
		return allocated;
	}

 // case 2 (NULL == inSize) this is a delete
 	if (0 == inSize)
	{
		UUrMemory_Block_Delete_Debug(inFile, inLine, inMemory);

		return NULL;
	}

 // case 3 traditional realloc (this may move the memory, change the bookeep block
	{
		UUtBookkeepBlock*	oldBookkeep = UUiMemory_Debug_FindBookkeepBlock(inMemory);
		char*				newMemory = UUrMemory_Block_New_Debug(inFile, inLine, inSize);
		UUtBookkeepBlock*	newBookkeep = UUiMemory_Debug_FindBookkeepBlock(newMemory);
		
		// we must find the bookkeep block
		UUmAssert(NULL != oldBookkeep);

		// if we fail to allocate we return NULL and do nothing
		if (NULL == newMemory)
		{
			return NULL;
		} 

		// otherwise copy and then delete the old
		UUrMemory_MoveFast(
			inMemory,
			newMemory,
			UUmMin(oldBookkeep->realBytes, inSize));
		
		UUrString_Copy(newBookkeep->file, oldBookkeep->file, UUcMaxDebugFileLength);
		
		newBookkeep->line = oldBookkeep->line;
		newBookkeep->timeStamp = oldBookkeep->timeStamp;
		newBookkeep->stackLevel = oldBookkeep->stackLevel;
		newBookkeep->numTimesRealloced = oldBookkeep->numTimesRealloced;
		
		newBookkeep->numTimesRealloced++;
		
		UUrMemory_Block_Delete_Debug(inFile, inLine, inMemory);

		return newMemory;	
	}
}

#endif

/*******************************************************************************
 *
 ******************************************************************************/
static UUtError
UUiMemory_Pool_AddHeap(
	UUtMemory_Pool	*inMemoryPool)
{
	UUtMemory_Pool_Heap	*newHeap;
	
	if(inMemoryPool->activeHeap && inMemoryPool->activeHeap->next != NULL)
	{
		inMemoryPool->activeHeap = inMemoryPool->activeHeap->next;
		
		goto done;
	}
	
	UUrMemory_Leak_ForceGlobal_Begin();
	
	newHeap = UUrMemory_Block_New(sizeof(UUtMemory_Pool_Heap));
	if(newHeap == NULL)
	{
		return UUcError_OutOfMemory;
	}
	
	newHeap->heapMemory = UUrMemory_Block_New(inMemoryPool->chunkSize);
	if(newHeap->heapMemory == NULL)
	{
		UUrMemory_Block_Delete(newHeap);
		
		return UUcError_OutOfMemory;
	}
	
	//UUrMemory_Set32(newHeap->heapMemory, 0xBEEFBEEF, inMemoryPool->chunkSize);
	
	UUrMemory_Leak_ForceGlobal_End();

	newHeap->nextFreeIndex = 0;
	newHeap->next = NULL;
	
	if(inMemoryPool->activeHeap != NULL)
	{
		inMemoryPool->activeHeap->next = newHeap;
		inMemoryPool->activeHeap = newHeap;
	}
	else
	{
		inMemoryPool->activeHeap = inMemoryPool->heapList = newHeap;
	}

done:
	
	#if 0// DEBUGGING_MEMORY
	{
		UUtUns32	i;
		
		for(i = 0; i < 1000; i++)
		{
			UUmAssert(*((UUtUns32*)inMemoryPool->activeHeap->heapMemory + i) == UUmMemoryBlock_Garbage);
		}
		
		UUrMemory_Set32(inMemoryPool->activeHeap->heapMemory, UUmMemoryBlock_Garbage, inMemoryPool->chunkSize);
	}
	
	#endif
		
	return UUcError_None;
}

/*******************************************************************************
 *
 ******************************************************************************/

UUtMemory_Pool* 
UUrMemory_Pool_New_Real(
	UUtUns32		inChunkSize,
	UUtBool			inFixed)
{
	UUtMemory_Pool	*newMemoryPool;
	UUtUns32 mungedSize;
	
	newMemoryPool = UUrMemory_Block_New(sizeof(UUtMemory_Pool));
	if(newMemoryPool == NULL)
	{
		return NULL;
	}

	mungedSize = (inChunkSize + UUcProcessor_CacheLineSize - 1) & ~UUcProcessor_CacheLineSize_Mask;
	
	newMemoryPool->chunkSize = mungedSize;
	newMemoryPool->heapList = NULL;
	newMemoryPool->activeHeap = NULL;
	newMemoryPool->fixed = inFixed;
	
	if(UUiMemory_Pool_AddHeap(newMemoryPool) != UUcError_None)
	{
		return NULL;
	}
	
	return newMemoryPool;
}
	
/*******************************************************************************
 *
 ******************************************************************************/
void 
UUrMemory_Pool_Delete_Real(
	UUtMemory_Pool	*inMemoryPool)
{
	UUtMemory_Pool_Heap	*curHeap, *nextHeap;
	
	curHeap = inMemoryPool->heapList;
	
	while(curHeap != NULL)
	{
		nextHeap  = curHeap->next;
		
		UUrMemory_Block_Delete(curHeap->heapMemory);
		UUrMemory_Block_Delete(curHeap);
		
		curHeap = nextHeap;
	}
	
	UUrMemory_Block_Delete(inMemoryPool);
}

#if DEBUGGING_MEMORY
/*******************************************************************************
 *
 ******************************************************************************/
UUtMemory_Pool* 
UUrMemory_Pool_New_Debug(
	char			*inFile,
	long			inLine,
	UUtUns32		inChunkSize,
	UUtBool			inFixed)
{
	return UUrMemory_Pool_New_Real(inChunkSize, inFixed);
}

/*******************************************************************************
 *
 ******************************************************************************/
void 
UUrMemory_Pool_Delete_Debug(
	char			*inFile,
	long			inLine,
	UUtMemory_Pool	*inMemoryPool)
{
	UUrMemory_Pool_Delete_Real(inMemoryPool);
}

/*******************************************************************************
 *
 ******************************************************************************/


#endif

/*******************************************************************************
 *
 ******************************************************************************/


UUtUns32
UUrMemory_Pool_Get_ChunkSize(
	UUtMemory_Pool	*inMemoryPool)
{
	UUmAssertReadPtr(inMemoryPool, sizeof(*inMemoryPool));
	return inMemoryPool->chunkSize;
}

void *
UUrMemory_Pool_Block_New(
	UUtMemory_Pool	*inMemoryPool,
	UUtUns32		inSize)
{
	UUtUns32			mungedSize;
	void				*returnMemory;
	UUtMemory_Pool_Heap	*curHeap;
	UUtError			error;

	UUmAssertReadPtr(inMemoryPool, sizeof(*inMemoryPool));
		
	mungedSize = (inSize + UUcProcessor_CacheLineSize - 1) & ~UUcProcessor_CacheLineSize_Mask;
	
	if(mungedSize > inMemoryPool->chunkSize)
	{
		return NULL;
	}

	/*
	 * Check active heap
	 */
		if(inMemoryPool->activeHeap->nextFreeIndex + mungedSize < inMemoryPool->chunkSize)
		{
			returnMemory = inMemoryPool->activeHeap->heapMemory + inMemoryPool->activeHeap->nextFreeIndex;
			UUmAssertReadPtr(returnMemory, inSize);
			
			inMemoryPool->activeHeap->nextFreeIndex += mungedSize;
			
			goto done;
		};
	
	if(inMemoryPool->fixed == UUcTrue)
	{
		return NULL;
	}
	
	/*
	 * Check previous heaps
	 */
		curHeap = inMemoryPool->heapList;
		while(curHeap != inMemoryPool->activeHeap)
		{
			if(curHeap->nextFreeIndex + mungedSize < inMemoryPool->chunkSize)
			{
				returnMemory = curHeap->heapMemory + curHeap->nextFreeIndex;
				UUmAssertReadPtr(returnMemory, inSize);
				
				curHeap->nextFreeIndex += mungedSize;
				goto done;
			}
			
			curHeap = curHeap->next;
		}
	
	/*
	 * Add a new heap
	 */
		error = UUiMemory_Pool_AddHeap(inMemoryPool);
		if(error != UUcError_None)
		{
			return NULL;
		}
	
	returnMemory = inMemoryPool->activeHeap->heapMemory;
	UUmAssertReadPtr(returnMemory, inSize);
	
	inMemoryPool->activeHeap->nextFreeIndex += mungedSize;

done:
	
	UUmAssertReadPtr(returnMemory, inSize);
	
	#if defined(DEBUGGING_JUNKMEM) && DEBUGGING_JUNKMEM
	
		UUrMemory_Set32(returnMemory, UUmMemoryBlock_Garbage, inSize);

	#endif
	
	return returnMemory;
}

/*******************************************************************************
 *
 ******************************************************************************/
void 
UUrMemory_Pool_Reset(
	UUtMemory_Pool	*inMemoryPool)
{
	UUtMemory_Pool_Heap	*curHeap;

	UUmAssert(NULL != inMemoryPool);
	
	curHeap = inMemoryPool->heapList;
	
	while(curHeap)
	{
		curHeap->nextFreeIndex = 0;
		
		#if defined(DEBUGGING_JUNKMEM) && DEBUGGING_JUNKMEM
			
			UUrMemory_Set32(curHeap->heapMemory, UUmMemoryBlock_Garbage, inMemoryPool->chunkSize);
			
		#endif
		
		curHeap = curHeap->next;
	}
	
	inMemoryPool->activeHeap = inMemoryPool->heapList;
}

/*******************************************************************************
 *
 ******************************************************************************/
static UUtError
UUiMemory_Heap_AddSubheap(
	UUtMemory_Heap	*inMemoryHeap)
{
	UUtMemory_Heap_Subheap	*newSubheap;
	
	newSubheap = UUrMemory_Block_New(sizeof(UUtMemory_Heap_Subheap));
	UUmError_ReturnOnNull(newSubheap);
	
	newSubheap->memory = UUrMemory_Block_New(inMemoryHeap->chunkSize);
	UUmError_ReturnOnNull(newSubheap->memory);
	
	newSubheap->blockBV = UUrBitVector_New(inMemoryHeap->blocksPerHeap);
	UUmError_ReturnOnNull(newSubheap->blockBV);
	
	UUrBitVector_ClearBitAll(newSubheap->blockBV, inMemoryHeap->blocksPerHeap);
	
	newSubheap->next = NULL;
	newSubheap->blocksLeft = inMemoryHeap->blocksPerHeap;
	
	newSubheap->next = inMemoryHeap->subheapList;
	inMemoryHeap->subheapList = newSubheap;
		
	return UUcError_None;
}

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_Heap_Reset(
	UUtMemory_Heap	*inMemoryHeap)
{
	UUtMemory_Heap_Subheap	*curSubheap;
	
	for(curSubheap = inMemoryHeap->subheapList; curSubheap; curSubheap = curSubheap->next)
	{
		UUrBitVector_ClearBitAll(curSubheap->blockBV, inMemoryHeap->blocksPerHeap);
		curSubheap->blocksLeft = inMemoryHeap->blocksPerHeap;
	}
}

#if DEBUGGING_MEMORY
/*******************************************************************************
 *
 ******************************************************************************/
UUtMemory_Heap* 
UUrMemory_Heap_New_Debug(
	char			*inFile,
	long			inLine,
	UUtUns32		inChunkSize,
	UUtBool			inFixed)
{
	return UUrMemory_Heap_New_Real(inChunkSize, inFixed);
}

/*******************************************************************************
 *
 ******************************************************************************/
void 
UUrMemory_Heap_Delete_Debug(
	char			*inFile,
	long			inLine,
	UUtMemory_Heap	*inMemoryHeap)
{
	UUrMemory_Heap_Delete_Real(inMemoryHeap);
}

/*******************************************************************************
 *
 ******************************************************************************/


#endif
/*******************************************************************************
 *
 ******************************************************************************/
UUtMemory_Heap* 
UUrMemory_Heap_New_Real(
	UUtUns32		inChunkSize,
	UUtBool			inFixed)
{
	UUtMemory_Heap	*newMemoryHeap;
	UUtUns32		blocksPerHeap;
	UUtUns32		chunkSize;
	
	newMemoryHeap = UUrMemory_Block_New(sizeof(UUtMemory_Heap));
	if(newMemoryHeap == NULL) return NULL;
	
	chunkSize = (inChunkSize + UUcProcessor_CacheLineSize - 1) & ~UUcProcessor_CacheLineSize_Mask;
	newMemoryHeap->chunkSize = chunkSize;

	blocksPerHeap = chunkSize >> UUcProcessor_CacheLineBits;
	newMemoryHeap->blocksPerHeap = blocksPerHeap;

	newMemoryHeap->subheapList = NULL;
	newMemoryHeap->fixed = inFixed;
	
	if(UUiMemory_Heap_AddSubheap(newMemoryHeap) != UUcError_None)
	{
		return NULL;
	}
	
	return newMemoryHeap;
}
	
/*******************************************************************************
 *
 ******************************************************************************/
void 
UUrMemory_Heap_Delete_Real(
	UUtMemory_Heap	*inMemoryHeap)
{
	UUtMemory_Heap_Subheap	*curSubheap, *nextSubheap;
	
	curSubheap = inMemoryHeap->subheapList;
	
	while(curSubheap != NULL)
	{
		nextSubheap  = curSubheap->next;
		
		UUrMemory_Block_Delete(curSubheap->memory);
		UUrMemory_Block_Delete(curSubheap->blockBV);
		UUrMemory_Block_Delete(curSubheap);
		
		curSubheap = nextSubheap;
	}
	
	UUrMemory_Block_Delete(inMemoryHeap);
}

#if DEBUGGING_MEMORY
/*******************************************************************************
 *
 ******************************************************************************/
void *
UUrMemory_Heap_Block_New_Debug(		// This is used to allocate a block within a heap
	char*			inFile,
	long			inLine,
	UUtMemory_Heap	*inMemoryHeap,
	UUtUns32		inSize)
{
	return UUrMemory_Heap_Block_New_Real(inMemoryHeap, inSize);
}

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_Heap_Block_Delete_Debug(	// This is used to allocate a block within a heap
	char*			inFile,
	long			inLine,
	UUtMemory_Heap	*inMemoryHeap,
	void*			inBlock)
{
	UUrMemory_Heap_Block_Delete_Real(inMemoryHeap, inBlock);
}
#endif

/*******************************************************************************
 *
 ******************************************************************************/
void *
UUrMemory_Heap_Block_New_Real(
	UUtMemory_Heap	*inMemoryHeap,
	UUtUns32		inSize)
{
	UUtError				error;
	UUtUns32				numBlocks;
	UUtMemory_Heap_Subheap*	curSubheap;
	
	UUtUns32				blocksPerHeap;
	UUtUns32				curBlockIndex;
	UUtUns32				nextBlockIndex;
	
	char*					resultMemory;
	
	UUmAssert(NULL != inMemoryHeap);
	
	UUrMemory_Block_VerifyList();
		
	numBlocks = (inSize + UUcProcessor_CacheLineSize - 1) & ~UUcProcessor_CacheLineSize_Mask;
	numBlocks >>= UUcProcessor_CacheLineBits;
	
	curSubheap = inMemoryHeap->subheapList;
	
	blocksPerHeap = inMemoryHeap->blocksPerHeap;
	
	if(numBlocks > blocksPerHeap) return NULL;
	
	if(numBlocks < 2) numBlocks = 2;
	
	for(
		curSubheap = inMemoryHeap->subheapList;
		curSubheap != NULL;
		curSubheap = curSubheap->next)
	{
		if(numBlocks > curSubheap->blocksLeft) continue;
		
		curBlockIndex = 0;
		
		while(curBlockIndex != UUcBitVector_None && curBlockIndex + numBlocks < blocksPerHeap)
		{
			nextBlockIndex =
				UUrBitVector_FindFirstSetRange(
					curSubheap->blockBV,
					curBlockIndex,
					curBlockIndex+numBlocks-1);
			
			if(nextBlockIndex == UUcBitVector_None)
			{
				// found a space big enough
				resultMemory = curSubheap->memory + curBlockIndex * UUcProcessor_CacheLineSize;
				goto found;
			}
			
			curBlockIndex =
				UUrBitVector_FindFirstSetRange(
					curSubheap->blockBV,
					nextBlockIndex + 1,
					blocksPerHeap) + 1;
		}
	}
		
	error = UUiMemory_Heap_AddSubheap(inMemoryHeap);
	if(error != UUcError_None) return NULL;
	
	UUmAssert(inMemoryHeap->subheapList->blocksLeft == blocksPerHeap);

	curBlockIndex = 0;
	curSubheap = inMemoryHeap->subheapList;
	resultMemory = curSubheap->memory;
	
found:
	
	UUrBitVector_SetBit(curSubheap->blockBV, curBlockIndex);
	UUrBitVector_SetBit(curSubheap->blockBV, curBlockIndex+numBlocks-1);
	
	curSubheap->blocksLeft -= numBlocks;	
	
	return resultMemory;
}

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_Heap_Block_Delete_Real(	// This is used to allocate a block within a heap
	UUtMemory_Heap	*inMemoryHeap,
	void*			inBlock)
{
	UUtMemory_Heap_Subheap*	curSubheap;
	UUtUns32				chunkSize;
	UUtUns32				startBVIndex;
	UUtUns32				endBVIndex;
	UUtUns32				blocksPerHeap;
	
	chunkSize = inMemoryHeap->chunkSize;
	blocksPerHeap = inMemoryHeap->blocksPerHeap;

	for(
		curSubheap = inMemoryHeap->subheapList;
		curSubheap != NULL;
		curSubheap = curSubheap->next)
	{
		if((curSubheap->memory <= (char*)inBlock) && ((char*)inBlock < curSubheap->memory + chunkSize))
		{
			startBVIndex = ((char*)inBlock - curSubheap->memory) >> UUcProcessor_CacheLineBits;
			
			UUmAssert(UUrBitVector_TestBit(curSubheap->blockBV, startBVIndex));
			
			UUrBitVector_ClearBit(curSubheap->blockBV, startBVIndex);
			endBVIndex = 
				UUrBitVector_FindFirstSetRange(
					curSubheap->blockBV,
					startBVIndex + 1,
					blocksPerHeap);

			UUmAssert(UUrBitVector_TestBit(curSubheap->blockBV, endBVIndex));

			UUrBitVector_ClearBit(curSubheap->blockBV, endBVIndex);
			
			curSubheap->blocksLeft += endBVIndex - startBVIndex + 1;
			
			return;
		}
	}
	
	UUmAssert(0);
}

/*******************************************************************************
 *
 ******************************************************************************/
#if DEBUGGING_MEMORY
void
UUrMemory_Heap_Block_Verify(
	UUtMemory_Heap*	inMemoryHeap,
	void*			inPtr,
	UUtUns32		inLength)
{
	UUtMemory_Heap_Subheap*	curSubheap;
	UUtUns32				chunkSize;

	chunkSize = inMemoryHeap->chunkSize;
	for(
		curSubheap = inMemoryHeap->subheapList;
		curSubheap != NULL;
		curSubheap = curSubheap->next)
	{
		if((curSubheap->memory <= (char*)inPtr) && ((char*)inPtr < curSubheap->memory + chunkSize))
		{
			if((char*)inPtr + inLength < curSubheap->memory + chunkSize)
			{
				return;
			}
			UUmAssert(!"This block is illegal");
		}
	}
	
	UUmAssert(!"This block is illegal");
}
#endif

/*******************************************************************************
 *
 ******************************************************************************/
UUtMemory_Array* 
UUrMemory_Array_New_Real(			// This is used to create a new array
	UUtUns32			inElementSize,			// The size of each element
	UUtUns32			inAllocChunkSize,		// The chunk size of incremental allocations
	UUtUns32			inNumInitialElemsUsed,	// The number of elements initially used
	UUtUns32			inNumElemsToAlloc)		// The number of elements to allocate
{
	UUtMemory_Array	*newMemoryArray;
	
	newMemoryArray = UUrMemory_Block_New(sizeof(UUtMemory_Array));

	if (NULL == newMemoryArray) 
	{
		return NULL;
	}
	
	newMemoryArray->elemSize = inElementSize;
	newMemoryArray->allocChunkSize = inAllocChunkSize;
	newMemoryArray->numElemsUsed = inNumInitialElemsUsed;
	newMemoryArray->numElemsAlloced = inNumElemsToAlloc;
	
	if(inNumElemsToAlloc > 0)
	{
		newMemoryArray->data = UUrMemory_Block_New(inElementSize * inNumElemsToAlloc);
		if(newMemoryArray->data == NULL)
		{
			return NULL;
		}
	}
	else
	{
		newMemoryArray->data = NULL;
	}
	
	return newMemoryArray;
}

#if DEBUGGING_MEMORY

UUtMemory_Array* 
UUrMemory_Array_New_Debug(			// This is used to create a new array
	char*				inFile,
	UUtInt32			inLine,
	UUtUns32			inElementSize,			// The size of each element
	UUtUns32			inAllocChunkSize,		// The chunk size of incremental allocations
	UUtUns32			inNumInitialElemsUsed,	// The number of elements initially used
	UUtUns32			inNumElemsToAlloc)		// The number of elements to allocate
{
	UUtMemory_Array	*newMemoryArray;
	
	newMemoryArray = UUrMemory_Block_New_Debug(inFile, inLine, sizeof(UUtMemory_Array));

	if (NULL == newMemoryArray) 
	{
		return NULL;
	}
	
	newMemoryArray->elemSize = inElementSize;
	newMemoryArray->allocChunkSize = inAllocChunkSize;
	newMemoryArray->numElemsUsed = inNumInitialElemsUsed;
	newMemoryArray->numElemsAlloced = inNumElemsToAlloc;
	
	if (inNumElemsToAlloc > 0)
	{
		newMemoryArray->data = UUrMemory_Block_New(inElementSize * inNumElemsToAlloc);
		if(newMemoryArray->data == NULL)
		{
			return NULL;
		}
	}
	else
	{
		newMemoryArray->data = NULL;
	}
	
	return newMemoryArray;
}

#endif

/*******************************************************************************
 *
 ******************************************************************************/
void 
UUrMemory_Array_Delete_Real(		// This is used to delete a array
	UUtMemory_Array*	inMemoryArray)
{
	UUmAssert(inMemoryArray != NULL);
	
	if(inMemoryArray->data != NULL)
	{
		UUrMemory_Block_Delete(inMemoryArray->data);
	}
	
	UUrMemory_Block_Delete(inMemoryArray);
}

#if DEBUGGING_MEMORY

void 
UUrMemory_Array_Delete_Debug(		// This is used to delete a array
	char*				inFile,
	UUtInt32			inLine,
	UUtMemory_Array*	inMemoryArray)
{
	UUmAssert(inMemoryArray != NULL);
	
	if(inMemoryArray->data != NULL)
	{
		UUrMemory_Block_Delete_Debug(inFile, inLine, inMemoryArray->data);
	}
	
	UUrMemory_Block_Delete(inMemoryArray);
}

#endif

/*******************************************************************************
 *
 ******************************************************************************/
UUtError 
UUrMemory_Array_MakeRoom(			// This is used to make sure that enough elems have been allocated
	UUtMemory_Array*	inMemoryArray,
	UUtUns32			inNumElems,
	UUtBool				*outMemoryMoved)
{
	void	*newData;
	
	UUmAssert(inMemoryArray != NULL);
	
	if(outMemoryMoved != NULL)
	{
		*outMemoryMoved = UUcFalse;
	}
	
	if(inNumElems > inMemoryArray->numElemsAlloced)
	{
		if(inMemoryArray->data == NULL)
		{
			newData = UUrMemory_Block_New(inMemoryArray->elemSize * inNumElems);
		}
		else
		{
			newData =
				UUrMemory_Block_Realloc(inMemoryArray->data, inMemoryArray->elemSize * inNumElems);
		}

		if(newData == NULL)
		{
			goto failure;
		}
		
		if(outMemoryMoved != NULL)
		{
			*outMemoryMoved = UUcTrue;
		}
		
		inMemoryArray->data = newData;
		inMemoryArray->numElemsAlloced = inNumElems;
	}
	
	return UUcError_None;

failure:

	return UUcError_OutOfMemory;
}

/*******************************************************************************
 *
 ******************************************************************************/
void*
UUrMemory_Array_GetMemory(			// This is used to get the pointer to the linear array memory
	UUtMemory_Array*	inMemoryArray)
{
	UUmAssert(inMemoryArray != NULL);

	return inMemoryArray->data;
}

/*******************************************************************************
 *
 ******************************************************************************/
UUtError
UUrMemory_Array_GetNewElement(		// This is used to get an additional element from the array
	UUtMemory_Array*	inMemoryArray,
	UUtUns32			*outNewIndex,
	UUtBool				*outMemoryMoved)
{
	UUtUns32	newIndex;
	
	UUmAssert(inMemoryArray != NULL);

	if(outMemoryMoved != NULL)
	{
		*outMemoryMoved = UUcFalse;
	}
	
	newIndex = inMemoryArray->numElemsUsed++;
	
	if(inMemoryArray->numElemsUsed >= inMemoryArray->numElemsAlloced)
	{
		if(UUrMemory_Array_MakeRoom(
			inMemoryArray,
			inMemoryArray->numElemsAlloced + inMemoryArray->allocChunkSize,
			outMemoryMoved) != UUcError_None)
		{
			goto failure;
		}
	}
	
	*outNewIndex = newIndex;
	
	return UUcError_None;

failure:
	
	return UUcError_OutOfMemory;

}

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_Array_DeleteElement(		// This always moves memory
	UUtMemory_Array*	inMemoryArray,
	UUtUns32			inIndex)
{
	UUmAssert(inMemoryArray != NULL);
	UUmAssert(inMemoryArray->numElemsUsed > 0);
	
	UUmAssert(inIndex < inMemoryArray->numElemsUsed);
	
	inMemoryArray->numElemsUsed -= 1;
	
	if(inIndex < inMemoryArray->numElemsUsed)
	{
		char	*dst	= (char *)inMemoryArray->data + inMemoryArray->elemSize * inIndex;
		char	*src	= (char *)inMemoryArray->data + inMemoryArray->elemSize * (inIndex + 1);
		UUtUns32 size	= inMemoryArray->elemSize * (inMemoryArray->numElemsUsed - inIndex);

		UUrMemory_MoveOverlap(src, dst, size);
	}					
}

/*******************************************************************************
 *
 ******************************************************************************/
UUtUns32
UUrMemory_Array_GetUsedElems(	// This is used to return the number of elements in use
	UUtMemory_Array*	inMemoryArray)
{
	UUmAssert(inMemoryArray != NULL);

	return inMemoryArray->numElemsUsed;
}

/*******************************************************************************
 *
 ******************************************************************************/
UUtError
UUrMemory_Array_SetUsedElems(	// This is used to set the number of elements in use
	UUtMemory_Array*	inMemoryArray,
	UUtUns32			inElemsUsed,
	UUtBool				*outMemoryMoved)
{
	UUmAssert(inMemoryArray != NULL);

	inMemoryArray->numElemsUsed = inElemsUsed;
	
	return UUrMemory_Array_MakeRoom(inMemoryArray, inElemsUsed, outMemoryMoved);
}

/*******************************************************************************
 *
 ******************************************************************************/
UUtError
UUrMemory_Array_InsertElement(	
	UUtMemory_Array*	inMemoryArray,
	UUtUns32			inIndex,
	UUtBool				*outMemoryMoved)
{
	UUtError	error;
	
	inMemoryArray->numElemsUsed++;
	
	error = UUrMemory_Array_MakeRoom(inMemoryArray, inMemoryArray->numElemsUsed, outMemoryMoved);
	UUmError_ReturnOnError(error);
	
	UUrMemory_MoveOverlap(
		(char*)inMemoryArray->data + inMemoryArray->elemSize * inIndex,
		(char*)inMemoryArray->data + inMemoryArray->elemSize * (inIndex + 1),
		(inMemoryArray->numElemsUsed - inIndex - 1) * inMemoryArray->elemSize);
	
	return UUcError_None;
}

/*******************************************************************************
 *
 ******************************************************************************/
UUtMemory_ParallelArray* 
UUrMemory_ParallelArray_New_Real(		// This is used to create a new parallel array
	UUtUns32					inAllocChunkSize,		// The chunk size of incremental allocations
	UUtUns32					inNumInitialElemsUsed,	// The number of elements initially used
	UUtUns32					inNumElemsToAlloc)		// The number of elements to allocate
{
	UUtMemory_ParallelArray	*newParallelArray;
	
	newParallelArray = UUrMemory_Block_New(sizeof(UUtMemory_ParallelArray));
	
	if (newParallelArray == NULL)
	{
		return NULL;
	}
	
	newParallelArray->numElemsUsed = inNumInitialElemsUsed;
	newParallelArray->numElemsAlloced = inNumElemsToAlloc;
	newParallelArray->allocChunkSize = inAllocChunkSize;
	newParallelArray->numMemberArrays = 0;
	
	return newParallelArray;
}

#if DEBUGGING_MEMORY

UUtMemory_ParallelArray* 
UUrMemory_ParallelArray_New_Debug(
	char*			inFile,
	long			inLine,
	UUtUns32		inAllocChunkSize,
	UUtUns32		inNumInitialElemsUsed,
	UUtUns32		inNumElemsToAlloc)
{
	UUtMemory_ParallelArray	*newParallelArray;
	
	newParallelArray = UUrMemory_Block_New_Debug(inFile, inLine, sizeof(UUtMemory_ParallelArray));
	
	if (newParallelArray == NULL)
	{
		goto failure;
	}
	
	newParallelArray->numElemsUsed = inNumInitialElemsUsed;
	newParallelArray->numElemsAlloced = inNumElemsToAlloc;
	newParallelArray->allocChunkSize = inAllocChunkSize;
	newParallelArray->numMemberArrays = 0;
	
	return newParallelArray;

failure:
	
	return NULL;
}

#endif

/*******************************************************************************
 *
 ******************************************************************************/
void 
UUrMemory_ParallelArray_Delete_Real(	// This is used to delete a parallel array
	UUtMemory_ParallelArray*	inParallelArray)
{
	UUtUns32	i;

	UUmAssert(NULL != inParallelArray);
	
	for(i = 0; i < inParallelArray->numMemberArrays; i++)
	{
		if(inParallelArray->members[i].data != NULL)
		{
			UUrMemory_Block_Delete(inParallelArray->members[i].data);
		}
	}
	
	UUrMemory_Block_Delete(inParallelArray);
}

#if DEBUGGING_MEMORY

void 
UUrMemory_ParallelArray_Delete_Debug(
	char*						inFile,
	long						inLine,
	UUtMemory_ParallelArray*	inParallelArray)
{
	UUtUns32	i;

	UUmAssert(NULL != inParallelArray);

	for(i = 0; i < inParallelArray->numMemberArrays; i++)
	{
		if(inParallelArray->members[i].data != NULL)
		{
			UUrMemory_Block_Delete_Debug(inFile, inLine, inParallelArray->members[i].data);
		}
	}
	
	UUrMemory_Block_Delete_Debug(inFile, inLine, inParallelArray);
}

#endif
/*******************************************************************************
 *
 ******************************************************************************/
UUtMemory_ParallelArray_Member* 
UUrMemory_ParallelArray_Member_New(	// This is used to create a new member array
	UUtMemory_ParallelArray*	inParallelArray,
	UUtUns32					inElementSize)
{
	UUtMemory_ParallelArray_Member	*newParallelArrayMember;

	UUmAssert(NULL != inParallelArray);
	
	if(inParallelArray->numMemberArrays >= UUcMemory_ParallelArray_MaxMembers)
	{
		return NULL;
	}
	
	newParallelArrayMember = &inParallelArray->members[inParallelArray->numMemberArrays++];
	
	newParallelArrayMember->elemSize = inElementSize;
	
	if(inParallelArray->numElemsAlloced > 0)
	{
		newParallelArrayMember->data = UUrMemory_Block_New(inElementSize * inParallelArray->numElemsAlloced);
		
		if(newParallelArrayMember->data == NULL)
		{
			goto failure;
		}
	}
	
	return newParallelArrayMember;
	
failure:
	
	return NULL;
	
}

/*******************************************************************************
 *
 ******************************************************************************/
UUtError 
UUrMemory_ParallelArray_MakeRoom(		// This is used to make sure that enough elems have been allocated
	UUtMemory_ParallelArray*	inParallelArray,
	UUtUns32					inNumElems,
	UUtBool						*outMemoryMoved)
{
	UUtUns32	i;
	void		*newData;

	UUmAssert(NULL != inParallelArray);

	if(outMemoryMoved != NULL)
	{
		*outMemoryMoved = UUcFalse;
	}
	
	if(inNumElems > inParallelArray->numElemsAlloced)
	{
		inParallelArray->numElemsAlloced = inNumElems;
		if(outMemoryMoved != NULL)
		{
			*outMemoryMoved = UUcTrue;
		}
		
		for(i = 0; i < inParallelArray->numMemberArrays; i++)
		{
			if(inParallelArray->members[i].data == NULL)
			{
				inParallelArray->members[i].data = UUrMemory_Block_New(inNumElems * inParallelArray->members[i].elemSize);
				if(inParallelArray->members[i].data == NULL)
				{
					goto failure;
				}
			}
			else
			{
				newData = UUrMemory_Block_Realloc(inParallelArray->members[i].data, inNumElems * inParallelArray->members[i].elemSize);
				if(newData == NULL)
				{
					goto failure;
				}
				
				inParallelArray->members[i].data = newData;
			}
		}
	}
	
	return UUcError_None;

failure:

	return UUcError_OutOfMemory;
}

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_ParallelArray_DeleteElement(	// This is used to delete a element in every member array
	UUtMemory_ParallelArray*	inParallelArray,
	UUtUns32					inIndex)
{
	UUtUns32	i;

	UUmAssert(NULL != inParallelArray);
	
	inParallelArray->numElemsUsed -= 1;
	
	for(i = 0; i < inParallelArray->numMemberArrays; i++)
	{
		char	*src	= (char *)inParallelArray->members[i].data + inParallelArray->members[i].elemSize * (inIndex + 1);
		char	*dst	= (char *)inParallelArray->members[i].data + inParallelArray->members[i].elemSize * inIndex;
		UUtUns32 size	= inParallelArray->members[i].elemSize * (inParallelArray->numElemsUsed - inIndex);

		UUrMemory_MoveOverlap(src, dst, size);
	}
}

/*******************************************************************************
 *
 ******************************************************************************/
UUtUns32
UUrMemory_ParallelArray_GetNumUsedElems(	// This is used to return the number of used elements
	UUtMemory_ParallelArray*	inParallelArray)
{
	UUmAssert(NULL != inParallelArray);

	return inParallelArray->numElemsUsed;
}

/*******************************************************************************
 *
 ******************************************************************************/
UUtError
UUrMemory_ParallelArray_SetUsedElems(	// This is used to set the number of elements in use
	UUtMemory_ParallelArray*	inParallelArray,
	UUtUns32					inElemsUsed,
	UUtBool						*outMemoryMoved)
{
	UUmAssert(inParallelArray != NULL);
	
	inParallelArray->numElemsUsed = inElemsUsed;

	return UUrMemory_ParallelArray_MakeRoom(inParallelArray, inElemsUsed, outMemoryMoved);
}
			
/*******************************************************************************
 *
 ******************************************************************************/
UUtError
UUrMemory_ParallelArray_AddNewElem(			// This adds a new element to every member array - use UUrMemory_ParallelArray_Member_GetNewElement_Real to get the memory
	UUtMemory_ParallelArray*	inParallelArray,
	UUtUns32					*outNewIndex,
	UUtBool						*outMemoryMoved)
{
	UUmAssert(NULL != inParallelArray);
	UUmAssert(NULL != outNewIndex);

	*outNewIndex = inParallelArray->numElemsUsed++;

	if (NULL != outMemoryMoved) {
		*outMemoryMoved = UUcFalse;
	}
	
	if(inParallelArray->numElemsUsed >= inParallelArray->numElemsAlloced)
	{
		if(UUrMemory_ParallelArray_MakeRoom(
			inParallelArray,
			inParallelArray->numElemsAlloced + inParallelArray->allocChunkSize,
			outMemoryMoved) != UUcError_None)
		{
			return UUcError_OutOfMemory;
		}
	}
	
	return UUcError_None;
}

/*******************************************************************************
 *
 ******************************************************************************/
void*
UUrMemory_ParallelArray_Member_GetMemory(	// This is used to get the pointer to the linear array memory
	UUtMemory_ParallelArray*		inParallelArray,
	UUtMemory_ParallelArray_Member*	inMemberArray)
{
	UUmAssert(NULL != inMemberArray);

	return inMemberArray->data;
}

/*******************************************************************************
 *
 ******************************************************************************/
void*
UUrMemory_ParallelArray_Member_GetNewElement(	// This is used to get the new element from a member array
	UUtMemory_ParallelArray*		inParallelArray,
	UUtMemory_ParallelArray_Member*	inMemberArray)
{
	char *		base;
	UUtUns32	elemSize;
	UUtUns32	numElemsUsed;
	void *		newElement;

	UUmAssert(NULL != inMemberArray);
	UUmAssert(NULL != inParallelArray);

	base			= inMemberArray->data;
	elemSize		= inMemberArray->elemSize;
	numElemsUsed	= inParallelArray->numElemsUsed;

	newElement		= base + elemSize * (numElemsUsed - 1);

	return newElement;
}

/*******************************************************************************
 *
 ******************************************************************************/
struct UUtMemory_String
{
	UUtUns32	tableSize;
	char		data[4];	// the real length is tableSize
};

/*******************************************************************************
 *
 ******************************************************************************/
UUtMemory_String*
UUrMemory_String_New(
	UUtUns32	inTableSize)
{
	UUtMemory_String*	newMemoryString;
	
	newMemoryString = UUrMemory_Block_New(sizeof(UUtMemory_String) + inTableSize - 4);
	
	if(newMemoryString == NULL)
	{
		return NULL;
	}
	
	newMemoryString->tableSize = inTableSize;
	newMemoryString->data[0] = 0;
	
	return newMemoryString;
}

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_String_Delete(
	UUtMemory_String*	inMemoryString)
{
	UUrMemory_Block_Delete(inMemoryString);
}

/*******************************************************************************
 *
 ******************************************************************************/
char*
UUrMemory_String_GetStr(
	UUtMemory_String*	inMemoryString,
	const char*			inStr)
{
	UUtInt32	length;
	char		*p;

	if ('\0' == *inStr) {
		return UUgMemory_String_Zero;
	}
	
	length = (UUtInt32)strlen(inStr);
	if(length >= (UUtInt32)inMemoryString->tableSize) return NULL;
	
	p = inMemoryString->data;
	
	while(1)
	{
		if(*p == 0)
		{
			break;
		}
		
		if(!strcmp(p, inStr))
		{
			return p;
		}
		
		p += strlen(p) + 1;
	}
	
	length = strlen(inStr);
	
	if((UUtUns32)((p - inMemoryString->data) + length + 2) > inMemoryString->tableSize)
	{
		return NULL;
	}
	
	strcpy(p, inStr);
	*(p + length) = 0;
	*(p + length + 1) = 0;
	
	return p;
}

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_String_Reset(
	UUtMemory_String*	inMemoryString)
{
	inMemoryString->data[0] = 0;
}

/*******************************************************************************
 *
 ******************************************************************************/
UUtUns32
UUrMemory_String_GetSize(
	UUtMemory_String*	inMemoryString)
{
	return inMemoryString->tableSize;
}

#if DEBUGGING_MEMORY

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_Leak_Start_Real(
	char*		inFileName,
	UUtInt32	inLineNum,
	const char*	inComment)
{
	UUmAssert(gLeakStack_TOS < cMaxLeakStackLength);
	
	gLeakStack[gLeakStack_TOS].fileName = inFileName;
	gLeakStack[gLeakStack_TOS].lineNum = inLineNum;
	gLeakStack[gLeakStack_TOS].timeStamp = UUrMachineTime_Sixtieths();
	gLeakStack[gLeakStack_TOS].comment = inComment;
	
	gLeakStack_TOS++;
}

/*******************************************************************************
 *
 ******************************************************************************/
void
UUrMemory_Leak_StopAndReport_Real(
	void)
{
	UUtError	error;
	
	FILE *memLeakFile = NULL;

	UUtBookkeepBlock*	curBookkeepBlock;
	UUtInt32			giveBeep = 0;
	UUtUns32			leakSize = 0;
	char				file[DScNameBufferSize];
	char				function[DScNameBufferSize];
	UUtUns32			line;
	UUtUns16			itr;
	UUtBool				firstTime = UUcTrue;
	
	gLeakStack_TOS--;
	
	if (memLeakFile == NULL)
	{
		memLeakFile = UUrFOpen(UUmMemLeakFileName, "a");
	}
	
	if(memLeakFile == NULL) return;
	
	for(
		curBookkeepBlock = gBlockListHead;
		curBookkeepBlock != NULL;
		curBookkeepBlock = curBookkeepBlock->next)
	{
		if(curBookkeepBlock->stackLevel < gLeakStack_TOS ||
			curBookkeepBlock->reported)
		{
			continue;
		}
		
		if(firstTime == UUcTrue)
		{
			firstTime = UUcFalse;
			fprintf(memLeakFile, "**************\n");
			fprintf(memLeakFile, "* File: %s\n", gLeakStack[gLeakStack_TOS].fileName);
			fprintf(memLeakFile, "* Line: %d\n", gLeakStack[gLeakStack_TOS].lineNum);
			fprintf(memLeakFile, "* TimeStamp: %d\n", gLeakStack[gLeakStack_TOS].timeStamp);
			fprintf(memLeakFile, "* Comment: %s\n", gLeakStack[gLeakStack_TOS].comment);
			fprintf(memLeakFile, "**************\n");
		}

		curBookkeepBlock->reported = UUcTrue;
		
		fprintf(
			memLeakFile,
			"Memory leak; FILE %s\n""\tLINE %d\n""\tSIZE %d\n""\tTIMSTAMP: %d\n",
			curBookkeepBlock->file,
			curBookkeepBlock->line,
			curBookkeepBlock->realBytes,
			curBookkeepBlock->timeStamp);
		
		fprintf(
			stderr,
			"Memory leak; FILE %s\n""\tLINE %d\n""\tSIZE %d\n""\tTIMSTAMP: %d\n",
			curBookkeepBlock->file,
			curBookkeepBlock->line,
			curBookkeepBlock->realBytes,
			curBookkeepBlock->timeStamp);
		
		for(itr = 0; itr < curBookkeepBlock->numChains; itr++)
		{
			error = 
				DSrProgramCounter_GetFileAndLine(
					curBookkeepBlock->callChainList[itr],
					file,
					function,
					&line);
			if(error == UUcError_None)
			{
				fprintf(memLeakFile, "FUNCTION: %s\n", function);
				fprintf(memLeakFile, "FILE: %s\n", file);
				fprintf(memLeakFile, "LINE: %d\n", line);
			}
			else
			{
				fprintf(memLeakFile, "Unknown stack frame\n");
				break;
			}
			fprintf(memLeakFile, "---\n");
		}
		
		fprintf(memLeakFile,"*****************************************************\n");
		
		leakSize += curBookkeepBlock->realBytes;
		
		giveBeep = 1;
	}
	
	if(giveBeep)
	{
		if(memLeakFile != NULL)
		{
			fprintf(memLeakFile, "**************\n");
			fprintf(memLeakFile, "Leak Size: %d\n", leakSize);
			fprintf(memLeakFile, "**************\n");
		}
		
		fprintf(stderr, "Detected memory leak, %s\n", gLeakStack[gLeakStack_TOS].comment);
	}

	if (NULL != memLeakFile) 
	{
		fclose(memLeakFile);
	}
}
#endif

UUtError 
UUrMemory_Initialize(
	void)
{
	UUtError err= UUcError_None;

	#if DEBUGGING_MEMORY
	{
		FILE*	file;
		
		gStartTime = UUrMachineTime_Sixtieths();
		
		file = UUrFOpen(UUmMemLeakFileName, "w");
		if(file)
		{
			fclose(file);
		}
	
	}
	UUrMemory_Leak_Start("These leaks are global");
	
	gMemoryTracker = AUrSharedElemArray_New(sizeof(UUtMemoryTracker), (AUtSharedElemArray_Equal)UUrMemory_Tracker_Compare);
	UUmError_ReturnOnNull(gMemoryTracker);
	#endif

	if (stefans_emergency_relief_fund == NULL)
	{
		stefans_emergency_relief_fund= malloc(RELIEF_FUND_SIZE);
	}
	if (stefans_emergency_relief_fund == NULL)
	{
		err= UUcError_Generic;
	}
	
	return err;
}

#if DEBUGGING_MEMORY
static UUtMemoryTracker*	gTrackerList = NULL;

static UUtBool
UUrMemory_Tracker_Sort_Compare(
	UUtUns32	inElemA,
	UUtUns32	inElemB)
{
	UUtBool result;
	UUmAssertReadPtr(gTrackerList, sizeof(*gTrackerList));
	
	result = (gTrackerList[inElemA].memoryAllocated < gTrackerList[inElemB].memoryAllocated);
	
	return 0;
}
#endif

void 
UUrMemory_Terminate(
	void)
{

#if DEBUGGING_MEMORY
	fprintf(stderr, "Max mem used: %d\n", gMaxMemUsage);
	
	{
		FILE*	file;
		
		file = fopen("memTracker.txt", "w");
		if(file != NULL)
		{
			UUtMemoryTracker*	trackerList;
			UUtMemoryTracker*	curTracker;
			UUtUns32			trackerItr;
			UUtUns32			numTrackers;
			UUtUns32*			sortedIndexList;
			UUtUns32*			allocSortedIndexList;
			UUtUns32			itr;
			
			allocSortedIndexList = UUrMemory_Block_New((AUrSharedElemArray_GetNum(gMemoryTracker) + 16) * sizeof(UUtUns32));
			if(allocSortedIndexList == NULL)
			{
				UUmAssert(0);
				return;
			}
			
			trackerList = (UUtMemoryTracker*)AUrSharedElemArray_GetList(gMemoryTracker);
			numTrackers = AUrSharedElemArray_GetNum(gMemoryTracker);
			sortedIndexList = AUrSharedElemArray_GetSortedIndexList(gMemoryTracker);
			
			for(itr = 0; itr < numTrackers; itr++)
			{
				allocSortedIndexList[itr] = itr;
			}
			
			gTrackerList = (UUtMemoryTracker*)AUrSharedElemArray_GetList(gMemoryTracker);
			

			AUrQSort_32(
				allocSortedIndexList,
				numTrackers,
				UUrMemory_Tracker_Sort_Compare);
			
			
			fprintf(file, "num times recorded: %d\n", gMemoryTracker_NumRecords);
			fprintf(file, "num blocks: %d\n", gMemoryTracker_NumBlocks);
			fprintf(file, "====================================\n");
			fprintf(file, "Memory Usage sorted by size\n");
			fprintf(file, "====================================\n");
			for(trackerItr = 0; trackerItr < numTrackers; trackerItr++)
			{
				curTracker = trackerList + allocSortedIndexList[trackerItr];
				
				fprintf(file, "************************************\n");
				fprintf(file, "FILE: %s\n", curTracker->fileName);
				fprintf(file, "LINE: %d\n", curTracker->lineNum);
				fprintf(file, "SIZE: %u\n", curTracker->memoryAllocated);
			}

			fprintf(file, "====================================\n");
			fprintf(file, "Memory Usage sorted by file and line\n");
			fprintf(file, "====================================\n");
			for(trackerItr = 0; trackerItr < numTrackers; trackerItr++)
			{
				curTracker = trackerList + sortedIndexList[trackerItr];
				
				fprintf(file, "************************************\n");
				fprintf(file, "FILE: %s\n", curTracker->fileName);
				fprintf(file, "LINE: %d\n", curTracker->lineNum);
				fprintf(file, "SIZE: %u\n", curTracker->memoryAllocated);
			}
			
			UUrMemory_Block_Delete(allocSortedIndexList);
			
			fclose(file);
		}
	
	}
	
	AUrSharedElemArray_Delete(gMemoryTracker);
	gMemoryTracker = NULL;
	
	UUrMemory_Leak_StopAndReport();

#endif

	if (stefans_emergency_relief_fund)
	{
		free(stefans_emergency_relief_fund);
		stefans_emergency_relief_fund= NULL;
	}

	return;
}
