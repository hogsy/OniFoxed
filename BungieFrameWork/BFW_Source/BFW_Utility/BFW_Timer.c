/*
	FILE:	BFW_Timer.c
	
	AUTHOR:	Michael Evans
	
	CREATED: January 2, 2000
	
	PURPOSE: Timing utilities
	
	Copyright 2000

*/

#include "BFW.h"
#include "BFW_Timer.h"
#include "BFW_FileManager.h"
#include "BFW_Console.h"
#include "BFW_AppUtilities.h"

// CB: this causes the engine to output a (massive) text file every time
// it goes through a number of update loops
#define LOG_TOUCHPOINTS				0

typedef struct UUtListCell UUtListCell;

struct UUtListCell
{
	UUtListCell *next;
	UUtListCell *prev;
};

static void UUrList_Add(void **inList, UUtListCell *inCell)
{
	inCell->next = *inList;
	inCell->prev = NULL;

	if (inCell->next != NULL) {
		inCell->next->prev = inCell;
	}

	*inList = inCell;
}

static void UUrList_Delete(void **inList, UUtListCell *inCell)
{
	if (inCell->next != NULL) {
		inCell->next->prev = inCell->prev;
	}

	if (inCell->prev != NULL) {
		inCell->prev->next = inCell->next;
	}
	else {
		*inList = inCell->next;
	}

	return;
}

#define MAX_TIMER_STRING 128

static UUtTimerPhaseRef active_phase_list = NULL;
static UUtTimerRef living_timer_list = NULL;
static UUtTimerRef dead_timer_list = NULL;

struct UUtTimerPhase
{
	char type[MAX_TIMER_STRING];
	char name[MAX_TIMER_STRING];

	UUtTimerPhaseRef next;

	UUtInt64 total_time;
	UUtInt64 beat_time;
	UUtUns32 num_beats;
};

struct UUtTimer
{
	UUtListCell listCell;

	char type[MAX_TIMER_STRING];
	char name[MAX_TIMER_STRING];

	UUtInt64 total_time;
	UUtInt64 current_time;
};

UUtTimerPhaseRef UUrTimerPhase_Allocate(const char *inPhaseType, const char *inPhaseName)
{
	UUtTimerPhaseRef phase_reference;

	UUmAssert(strlen(inPhaseType) < MAX_TIMER_STRING);
	UUmAssert(strlen(inPhaseName) < MAX_TIMER_STRING);

	phase_reference = UUrMemory_Block_New(sizeof(UUtTimerPhase));

	if (NULL != phase_reference)
	{
		UUrString_Copy(phase_reference->type, inPhaseType, MAX_TIMER_STRING);
		UUrString_Copy(phase_reference->name, inPhaseName, MAX_TIMER_STRING);

		phase_reference->next = NULL;
		phase_reference->total_time = 0;
		phase_reference->beat_time = 0;
		phase_reference->num_beats = 0;
	}

	return phase_reference;
}

void UUrTimerPhase_Dispose(UUtTimerPhaseRef inPhaseRef)
{
	UUrMemory_Block_Delete(inPhaseRef);

	return;
}

void UUrTimerPhase_Begin(UUtTimerPhaseRef inPhase)
{
	return;
}

void UUrTimerPhase_End(UUtTimerPhaseRef inPhase)
{
	return;
}

void UUrTimerPhase_Beat(UUtTimerPhaseRef inReference)
{
	return;
}


UUtTimerRef UUrTimer_Allocate(const char *inTimerType, const char *inTimerName)
{
	UUtTimerRef timer_ref;
		
	timer_ref = UUrMemory_Block_New(sizeof(UUtTimer));

	if (NULL != timer_ref)
	{
		UUrString_Copy(timer_ref->type, inTimerType, MAX_TIMER_STRING);
		UUrString_Copy(timer_ref->name, inTimerName, MAX_TIMER_STRING);

		timer_ref->total_time = 0;
		timer_ref->current_time = 0;

		UUrList_Add(&living_timer_list, &timer_ref->listCell);
	}

	return timer_ref;
}

void UUrTimer_Dispose(UUtTimerRef inTimer)
{
	UUrList_Delete(&living_timer_list, &inTimer->listCell);
	UUrList_Add(&dead_timer_list, &inTimer->listCell);

	return;
}

void UUrTimer_Begin(UUtTimerRef inTimer)
{
	UUtInt64 time = UUrMachineTime_High();

	inTimer->current_time = -time;

	return;
}

UUtInt64 UUrTimer_End(UUtTimerRef inTimer)
{
	UUtInt64 time = UUrMachineTime_High();
	UUtInt64 delta;

	inTimer->current_time += time;
	delta = inTimer->current_time;

	inTimer->total_time += delta;

	return delta;
}

static void UUrTimerSystem_PurgeCells(void)
{
	while(dead_timer_list != NULL)
	{
		UUtTimerRef current_cell = dead_timer_list;

		UUrList_Delete(&dead_timer_list, &current_cell->listCell);

		UUrMemory_Block_Delete(current_cell);
	}

	return;
}




void UUrTimerSystem_WriteToDisk(void)
{
	UUtInt32 file_number = 0;
	BFtFile *stream = NULL;
	BFtFileRef *file_reference = NULL;

	file_reference = BFrFileRef_MakeUnique("timer_file_", ".txt", &file_number, 999);

	if (NULL != file_reference) {
		BFrFile_Open(file_reference, "w", &stream);

		BFrFileRef_Dispose(file_reference);
		file_reference = NULL;
	}
	
	if (NULL != stream) {
		UUtTimerRef current_cell;
		UUtInt32 itr;

		for(itr = 0; itr < 2; itr++) {
			current_cell = (0 == itr) ? living_timer_list : dead_timer_list;

			for(; current_cell != NULL; current_cell = (UUtTimerRef) current_cell->listCell.next)
			{
				double total_time;

				total_time = UUrMachineTime_High_To_Seconds(current_cell->total_time);

				BFrFile_Printf(stream, "%s %s %f"UUmNL, current_cell->type, current_cell->name, total_time);
			}
		}
		BFrFile_Close(stream);
	}

	UUrTimerSystem_PurgeCells();

	return;

}

#if TOOL_VERSION

void UUrStallTimer_Begin(UUtStallTimer *inTimer)
{
	*inTimer = UUrMachineTime_Sixtieths();

	return;
}

#if LOG_TOUCHPOINTS
static const char *UUgDebugLastTouchpoint = NULL;
#endif

void UUrStallTimer_End(UUtStallTimer *inTimer, const char *inName)
{
	UUtUns32 start = *inTimer;
	UUtUns32 end = UUrMachineTime_Sixtieths();
	UUtUns32 duration = end - start;
	UUtUns32 duration_threshold = 30;

#if LOG_TOUCHPOINTS
	// store the most recent place where we were
	UUgDebugLastTouchpoint = inName;

#if SUPPORT_DEBUG_FILES
	{
		static BFtDebugFile *log_file = NULL;
		
		if (NULL == log_file) {
			log_file = BFrDebugFile_Open("touchpoint.txt");
		}

		if (NULL != log_file) {
			BFrDebugFile_Printf(log_file, "time %d, touched %s\n", end, inName);
		}
	}
#endif
#endif

#if SUPPORT_DEBUG_FILES
	if (duration > duration_threshold) {
		static BFtDebugFile *debug_file = NULL;
		
		if (NULL == debug_file) {
			debug_file = BFrDebugFile_Open("stalls.txt");
		}

		if (NULL != debug_file) {
			BFrDebugFile_Printf(debug_file, "stalled in %s for %f seconds\n", inName, duration / 60.f);
		}
	}
#endif

	return;
}

#endif // TOOL_VERSION

#if PERFORMANCE_TIMER

#define MAX_PERFORMANCE_TIMERS 20
static UUtPerformanceTimer UUgPerformanceTimers[MAX_PERFORMANCE_TIMERS];
static UUtUns32 UUgPerformanceTimerCount = 0;
char UUgTimerRequiredPrefix[128];

UUtPerformanceTimer *UUrPerformanceTimer_New(char *name)
{
	UUtPerformanceTimer *result = NULL;

	if (UUgPerformanceTimerCount >= MAX_PERFORMANCE_TIMERS) {
		goto exit;
	}

	result = UUgPerformanceTimers + UUgPerformanceTimerCount;
	UUgPerformanceTimerCount++;

	UUrMemory_Clear(result, sizeof(*result));

	strcpy(result->name, name);

exit:
	return result;
}

void UUrPerformanceTimer_Pulse(void)
{
	UUtUns32 itr;

	for(itr = 0; itr < UUgPerformanceTimerCount; itr++)
	{
		UUtPerformanceTimer *performance_timer = UUgPerformanceTimers + itr;

		performance_timer->slice_total_time += performance_timer->slice_time[performance_timer->current_slice];
		performance_timer->slice_total_quantity += performance_timer->slice_quantity[performance_timer->current_slice];

		performance_timer->current_slice = (performance_timer->current_slice + 1) % 61;

		performance_timer->slice_total_time -= performance_timer->slice_time[performance_timer->current_slice];
		performance_timer->slice_total_quantity -= performance_timer->slice_quantity[performance_timer->current_slice];

		performance_timer->slice_quantity[performance_timer->current_slice] = 0;
		performance_timer->slice_time[performance_timer->current_slice] = 0;
	}

	return;
}

UUtBool UUrPeformanceTimer_Display_Sort(UUtUns32 inA, UUtUns32 inB)
{
	UUtPerformanceTimer *timer_a = (UUtPerformanceTimer *) inA;
	UUtPerformanceTimer *timer_b = (UUtPerformanceTimer *) inB;
	UUtBool result;

	if ('\0' == UUgTimerRequiredPrefix[0]) {
		result = timer_a->slice_total_time < timer_b->slice_total_time;
	}
	else {
		UUtBool string_a_has_prefix = UUrString_HasPrefix(timer_a->name, UUgTimerRequiredPrefix);
		UUtBool string_b_has_prefix = UUrString_HasPrefix(timer_b->name, UUgTimerRequiredPrefix);

		if (string_a_has_prefix != string_b_has_prefix) {
			result = !string_a_has_prefix;
		}
		else {
			result = timer_a->slice_total_time < timer_b->slice_total_time;
			//result = UUrString_Compare_NoCase(timer_a->name, timer_b->name) > 0;
		}
	}

	return result;
}


static void UUrPerformanceTimer_WriteString(UUtPerformanceTimer *performance_timer, char *dest)
{
	if (dest != NULL) {
		sprintf(dest, "%s count: %2.2f time: %2.2f", 
			performance_timer->name,
			((float) performance_timer->slice_total_quantity) / 60.f,
			100 * UUrMachineTime_High_To_Seconds(performance_timer->slice_total_time));
	}

	return;
}

void UUrPerformanceTimer_Display(COtStatusLine *lines, UUtUns32 count)
{
	// ok, display the top xxx offenders
	UUtPerformanceTimer *list[MAX_PERFORMANCE_TIMERS];
	UUtUns32 itr;

	for(itr = 0; itr < UUgPerformanceTimerCount; itr++)
	{
		list[itr] = UUgPerformanceTimers + itr;
	}

	AUrQSort_32(list, UUgPerformanceTimerCount, UUrPeformanceTimer_Display_Sort);

	for(itr = 0; itr < count; itr++)
	{
		lines[itr].text[0] = '\0';
	}

	count = UUmMin(count, UUgPerformanceTimerCount);

	for(itr = 0; itr < count ; itr++)
	{
		if ('\0' != UUgTimerRequiredPrefix[0]) {
			if (!UUrString_HasPrefix(list[itr]->name, UUgTimerRequiredPrefix)) {
				continue;
			}
		}

		UUrPerformanceTimer_WriteString(list[itr], lines[itr].text);
	}

	return;
}

void UUrPerformanceTimer_Enter(UUtPerformanceTimer *inTimer)
{
	if (inTimer)
	{
		if (inTimer->active == 0)
		{
			UUtUns32 current_slice = inTimer->current_slice;

			inTimer->slice_quantity[current_slice]++;
			inTimer->slice_time[current_slice] -= UUrMachineTime_High();
		}
		++inTimer->active;
	}

	return;
}

void UUrPerformanceTimer_Exit(UUtPerformanceTimer *inTimer)
{
	if (NULL != inTimer)
	{
		--inTimer->active;
		if (inTimer->active == 0)
		{
			UUtUns32 current_slice = inTimer->current_slice;

			inTimer->slice_time[current_slice] += UUrMachineTime_High();
		}
	}

	return;
}

void UUrPerformanceTimer_SetPrefix(char *inPrefix)
{
	UUrString_Copy(UUgTimerRequiredPrefix, inPrefix, sizeof(UUgTimerRequiredPrefix));

	return;
}

#endif
