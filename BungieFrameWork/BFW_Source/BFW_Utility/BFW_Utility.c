 /*
	FILE:	BFW_Utility.c
	
	AUTHOR:	Brent H. Pease, Michael Evans
	
	CREATED: May 18, 1997
	
	PURPOSE: General utility files
	
	Copyright 1997, 1998

*/
#include <time.h>
#include <string.h>

#include "BFW.h"

#include "BFW_Platform.h"
#include "BFW_Error.h"
#include "BFW_Memory.h"
#include "BFW_MathLib.h"
#include "BFW_AppUtilities.h"
#include "BFW_DebuggerSymbols.h"
#include "BFW_Timer.h"
#include "bfw_math.h"

#if defined(DEBUGGING) && DEBUGGING

	UUtBool		UUgFalse = 0;
	UUtError	UUgError_None = 0;
	
#endif

	extern double UUgMachineTime_High_Frequency = 0;

// initialize random is internal
void UUiRandom_Initialize(void);

UUtUns32 random_seed = UUcDefaultRandomSeed;
UUtUns32 UUgLocalRandomSeed = UUcDefaultRandomSeed;

UUtError UUrInitialize(
	UUtBool	initializeBasePlatform)
{
	UUtError	error;
	
	error = UUrPlatform_Initialize(initializeBasePlatform);
	UUmError_ReturnOnErrorMsg(error, "Could not initialize platform");

	error= (bfw_math_initialize() ? UUcError_None : UUcError_Generic);
	UUmError_ReturnOnErrorMsg(error, "Could not intialize math library");
		
	DSrInitialize();
	
	MUrInitialize();

	error = UUrMemory_Initialize();
	UUmError_ReturnOnErrorMsg(error, "Could not initialize memory");
	
	error = UUrError_Initialize();
	UUmError_ReturnOnErrorMsg(error, "Could not initialize error handler");

	// initialize random
	UUiRandom_Initialize();

	// cache the machine time frequency
	UUgMachineTime_High_Frequency = UUrMachineTime_High_Frequency();
	
	if(UUrPlatform_SIMD_IsPresent())
	{
		UUrPlatform_SIMD_Initialize();
	}
	
	return UUcError_None;
}
	
void UUrTerminate(
	void)
{
	UUrMemory_Terminate();
	UUrError_Terminate();
	UUrPlatform_Terminate();
	DSrTerminate();
}

// Our linear congruential random number generator is of the form
//		x(n+1)= ((a * x(n)) + c) mod m
// We're using m= 2^32.  If we multiply 2 32-bit numbers together, the result is the low 32
// bits of the 64-bit product.  Thus, we get the "mod m" for free.  Our choices for A and C,
// which are EXTREMELY important, come from Knuth.
// 
// For more info, check out Numerical Recipes in C, pg. 284.

#define RANDOM_A (1664525L)
#define RANDOM_C (1013904223L)


#ifdef DEBUG_RANDOM_SEED
static void record_random_seed(UUtUns32 random_seed)
{
	// write to disk or something
}
#endif

void UUrRandom_SetSeed(UUtUns32 seed)
{
	random_seed = seed;
}

UUtUns32 UUrRandom_GetSeed(void)
{
	return random_seed;
}
	
UUtUns16 UUrRandom(void)
{
#ifdef DEBUG_RANDOM_SEED
	record_random_seed(random_seed);
#endif
	random_seed= RANDOM_A*random_seed + RANDOM_C;

	return (UUtUns16) (random_seed>>16);
}

UUtUns16 UUrLocalRandom(void)
{
	UUgLocalRandomSeed= RANDOM_A*UUgLocalRandomSeed + RANDOM_C;
	
	return (UUtUns16) (UUgLocalRandomSeed>>16);
}

void UUiRandom_Initialize(void)
{
#if defined(DEBUGGING) && DEBUGGING
	UUtUns16 table[100];
	int itr;

	random_seed = UUcDefaultRandomSeed;
	UUgLocalRandomSeed = UUcDefaultRandomSeed;

	for(itr = 0; itr < 100; itr++)
	{
		table[itr] = UUrRandom();
	}

	for(itr = 0; itr < 100; itr++)
	{
		table[itr] = UUmRandomRange(0, itr);
	}
#endif

	random_seed = UUcDefaultRandomSeed;
	UUgLocalRandomSeed = UUcDefaultRandomSeed;
}

#if 0

#if UUmPlatform == UUmPlatform_Mac

#pragma profile off

#endif

// /Gh enables the hook function call to __penter
// void __cdecl __penter( void );

extern UUtUns32	gNextIndex;

void
UUrCallStack_Exit(
	void)
{
	char *d;
	
	d = UUgFunctionHistory + gNextIndex;
	
	while(*d != '*') d--;
	
	*d = 0;
	
	gNextIndex = d - UUgFunctionHistory;
}

void
UUrCallStack_Enter(
	char*	inFunctionName)
{
	char*	orig;
	char*	d;
	char*	s;
	
	if(gNextIndex + 64 > UUcMaxFunctionHistoryLength)
	{
		return;
	}
	
	orig = d = UUgFunctionHistory + gNextIndex;
	s = inFunctionName;
	
	*d++ = '*';
	
	while(*s != 0) *d++ = *s++;
	
	*d = 0;
	
	gNextIndex += (d - orig);
}

#if UUmPlatform == UUmPlatform_Mac

#pragma profile reset

#endif

#endif

UUtUns32
UUrConvertStrToSecsSince1900(
	char*	inStr)				// MMM DD YYYY HH:MM:SS
{
	static const char month_name[12][4] =
	{
		"Jan",
		"Feb",
		"Mar",
		"Apr",
		"May",
		"Jun",
		"Jul",
		"Aug",
		"Sep",
		"Oct",
		"Nov",
		"Dec"
	};
	
	UUtUns16	month;
	struct tm			time;
	
	UUmAssert(strlen(inStr) == 20);
	
	for(month = 0; month < 12; month++)
	{
		if(!strncmp(inStr, month_name[month], 3))
		{
			break;
		}
	}
	
	if(month == 12) return -1;
	
	time.tm_mon = month;
	
	sscanf(inStr + 4, "%d", &time.tm_mday);
	sscanf(inStr + 7, "%d", &time.tm_year);
	sscanf(inStr + 12, "%d", &time.tm_hour);
	sscanf(inStr + 15, "%d", &time.tm_min);
	sscanf(inStr + 18, "%d", &time.tm_sec);
	
	time.tm_year -= 1900;
	time.tm_isdst = -1;
	
	return (UUtUns32)mktime(&time);
}

void
UUrConvertSecsSince1900ToStr(
	UUtUns32	inSecs,
	char*		inBuffer)		
	
	// MMM DD YYYY HH:MM:SS <- under the mac
	// DOW MMM DD HH:MM:SS YYYY <- under windows
	// 0123456789012345678901234
{	
	// NOTE: not internationalized, string length here is a magic, not reliable number
	// from ctime implementation to ctime implementation
	time_t time = inSecs;
	UUrString_Copy(inBuffer, ctime( &time ), 26 );

	return;
}

typedef struct UUtQueue
{
	UUtUns16 head;		// points to the front item of the queue
	UUtUns16 tail;		// points to the last item in the queue (if head == tail we are empty)
	UUtUns16 elemSize;		
	UUtUns16 queueSize;		
	UUtUns16 count;
} ONtCommandQueue;


UUtQueue *UUrQueue_Create(UUtUns16 inNumElements, UUtUns16 inSize)
{
	UUtQueue *outQueue;

	outQueue = UUrMemory_Block_New(sizeof(UUtQueue) + (inNumElements * inSize));
	if (NULL == outQueue) {
		goto errorExit;
	}

	outQueue->head = 0;
	outQueue->tail = 0;
	outQueue->queueSize = inNumElements;
	outQueue->elemSize = inSize;
	outQueue->count = 0;

errorExit:
	return outQueue;
}

void UUrQueue_Dispose(UUtQueue *inQueue)
{
	UUmAssert(NULL != inQueue);

	UUrMemory_Block_Delete(inQueue);
}

void *UUrQueue_Get(UUtQueue *inQueue)
{
	void *result = UUrQueue_Peek(inQueue, 0);

	if (NULL == result) {
		return NULL;
	}

	inQueue->head = (inQueue->head + 1) % inQueue->queueSize;
	inQueue->count -= 1;

	return result;
}

void *UUrQueue_Peek(UUtQueue *inQueue, UUtUns16 peekOffset)
{
	void *result;
	UUtUns16 index;

	UUmAssert(NULL != inQueue);

	if (peekOffset >= inQueue->count) {
		return NULL;
	}

	index = (inQueue->head + peekOffset) % (inQueue->queueSize);

	result = ((char *) inQueue) + sizeof(UUtQueue) + (index * inQueue->elemSize);

	return result;
}

UUtError UUrQueue_Add(UUtQueue *ioQueue, void *inElement)
{
	UUtUns16 newTail;
	void *newAddr;

	UUmAssert(ioQueue != NULL);
	UUmAssert(inElement != NULL);

	if (ioQueue->count == ioQueue->queueSize) {
		return UUcError_Generic;
	}

	// calculate the new tail
	newTail = (UUtUns16) ((ioQueue->tail + 1) % (ioQueue->queueSize));

	// add the command
	newAddr = ((char *) ioQueue) + sizeof(UUtQueue) + (ioQueue->tail * ioQueue->elemSize);
	UUrMemory_MoveFast(inElement, newAddr, ioQueue->elemSize);

	// increment the tail, note element as added
	ioQueue->tail = newTail;
	ioQueue->count += 1;

	return UUcError_None;
}

UUtUns16 UUrQueue_CountMembers(UUtQueue *inQueue)
{
	UUtUns16 count;

	count = inQueue->count;

	return count;
}

UUtUns16 UUrQueue_CountVacant(UUtQueue *inQueue)
{
	UUtUns16 count;

	count = inQueue->queueSize - inQueue->count;

	return count;
}

char*
UUrTag2Str(
	UUtUns32	inTag)
{
	static char	buffer[5];
	
	buffer[0] = (char)((inTag >> 24) & 0xFF);
	buffer[1] = (char)((inTag >> 16) & 0xFF);
	buffer[2] = (char)((inTag >> 8) & 0xFF);
	buffer[3] = (char)(inTag & 0xFF);
	buffer[4] = 0;
	
	return buffer;
}

UUtUns32 UUrMachineTime_Sixtieths_Cached;

double UUrMachineTime_High_To_Seconds(UUtInt64 time)
{
	double double_time;

	double_time = (double) time;
	double_time /= UUrMachineTime_High_Frequency();

	return double_time;
}

UUtUns32
UUrOneWayFunction_Uns32(
	UUtUns32	inUns32)
{
	inUns32 *= ~inUns32;
	inUns32 ^= (inUns32 << 1);
	
	return (((~inUns32 & 0x3FFFF) << 14) | ((inUns32 & 0xFFFC0000) >> 18)) * 0xDEAD;
}

UUtUns16
UUrOneWayFunction_Uns16(
	UUtUns16	inUns16)
{
	inUns16 *= ~inUns16;
	inUns16 ^= (inUns16 >> 1);
	
	return (((~inUns16 & 0x7F) << 9) | ((inUns16 & 0xFF80) >> 7)) * 0xFEFE;
}

UUtUns8
UUrOneWayFunction_Uns8(
	UUtUns8	inUns8)
{
	inUns8 *= ~inUns8;
	inUns8 ^= (inUns8 << 1);
	
	return (((~inUns8 & 0x1F) << 3) | ((inUns8 & 0xE0) >> 5)) * 0xEF;
}

void
UUrOneWayFunction_ConvertString(
	char*		ioString,
	UUtUns16	inLength)
{
	while(inLength >= 4)
	{
		*(UUtUns32*)ioString = UUrOneWayFunction_Uns32(*(UUtUns32*)ioString);
		inLength -=4;
		ioString += 4;
	}
	
	if(inLength >= 2)
	{
		*(UUtUns16*)ioString = UUrOneWayFunction_Uns16(*(UUtUns16*)ioString);
		inLength -= 2;
		ioString += 2;
	}
	
	if(inLength >= 1)
	{
		*(UUtUns8*)ioString = UUrOneWayFunction_Uns8(*(UUtUns8*)ioString);
		inLength -= 1;
		ioString += 1;
	}
	
	UUmAssert(inLength == 0);
}

UUtBool
UUrOneWayFunction_CompareString(
	char*		inNormalString,
	char*		inMungedString,
	UUtUns16	inLength)
{
	if(inLength == 0) return UUcFalse;
	
	while(inLength >= 4)
	{
		if(*(UUtUns32*)inMungedString !=
			UUrOneWayFunction_Uns32(*(UUtUns32*)inNormalString))
		{
			return UUcFalse;
		}
		inLength -= 4;
		inNormalString += 4;
		inMungedString += 4;
	}
	
	if(inLength >= 2)
	{
		if(*(UUtUns16*)inMungedString !=
			UUrOneWayFunction_Uns16(*(UUtUns16*)inNormalString))
		{
			return UUcFalse;
		}
		inLength -= 2;
		inNormalString += 2;
		inMungedString += 2;
	}
	
	if(inLength >= 1)
	{
		if(*(UUtUns8*)inMungedString !=
			UUrOneWayFunction_Uns8(*(UUtUns8*)inNormalString))
		{
			return UUcFalse;
		}
		inLength -= 1;
		inNormalString += 1;
		inMungedString += 1;
	}
	
	UUmAssert(inLength == 0);
	
	return UUcTrue;
}

void UUrSwap_2Byte_Array(void *inAddr, UUtUns32 count)
{
	UUtUns16 *p2Byte = inAddr;
	UUtUns32 itr;

	for(itr = 0; itr < count; itr++)
	{
		UUrSwap_2Byte(p2Byte);

		p2Byte++;
	}

	return;
}

void UUrSwap_4Byte_Array(void *inAddr, UUtUns32 count)
{
	UUtUns32 *p4Byte = inAddr;
	UUtUns32 itr;

	for(itr = 0; itr < count; itr++)
	{
		UUrSwap_4Byte(p4Byte);

		p4Byte++;
	}

	return;
}

void UUrSwap_8Byte_Array(void *inAddr, UUtUns32 count)
{
	UUtUns64 *p8Byte = inAddr;
	UUtUns32 itr;

	for(itr = 0; itr < count; itr++)
	{
		UUrSwap_8Byte(p8Byte);

		p8Byte++;
	}

	return;
}
