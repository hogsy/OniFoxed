#pragma once
/*
	FILE:	BFW.h
	
	AUTHOR:	Michael Evans
	
	CREATED: January 2, 2000
	
	PURPOSE: Timing utilities
	
	Copyright 2000

*/

#ifndef BFW_TIMER
#define BFW_TIMER
#include "BFW.h"
#include "BFW_Console.h"

typedef struct UUtTimerPhase UUtTimerPhase;
typedef struct UUtTimer UUtTimer;

typedef UUtTimerPhase	*UUtTimerPhaseRef;
typedef UUtTimer		*UUtTimerRef;

// low resolution timer
UUtUns32 UUrMachineTime_Sixtieths(void);

// high resolution timer
UUtInt64 UUrMachineTime_High(void);
double UUrMachineTime_High_Frequency(void);
extern double UUgMachineTime_High_Frequency;
double UUrMachineTime_High_To_Seconds(UUtInt64 time);

/*
 * stall timer
 * 
 */

typedef UUtUns32 UUtStallTimer;

#if TOOL_VERSION

void UUrStallTimer_Begin(UUtStallTimer *inTimer);
void UUrStallTimer_End(UUtStallTimer *inTimer, const char *inName);

#else

static UUcInline void UUrStallTimer_Begin(UUtStallTimer *inTimer)
{
	inTimer;

	return;
}

static UUcInline void UUrStallTimer_End(UUtStallTimer *inTimer, const char *inName)
{
	inTimer;
	inName;

	return;
}

#endif



/*
 * timer phase functions
 */


UUtTimerPhaseRef UUrTimerPhase_Allocate(const char *inPhaseType, const char *inPhaseName);
void UUrTimerPhase_Dispose(UUtTimerPhaseRef inPhaseRef);

void UUrTimerPhase_Begin(UUtTimerPhaseRef inPhase);
void UUrTimerPhase_End(UUtTimerPhaseRef inPhase);
void UUrTimerPhase_Beat(UUtTimerPhaseRef inReference);


/*
 * timer functions
 */


UUtTimerRef UUrTimer_Allocate(const char *inTimerType, const char *inTimerName);
void UUrTimer_Dispose(UUtTimerRef inTimer);

void UUrTimer_Begin(UUtTimerRef inTimer);
UUtInt64 UUrTimer_End(UUtTimerRef intimer);

/*
 *
 */

void UUrTimerSystem_WriteToDisk(void);

#if PERFORMANCE_TIMER

typedef struct UUtPerformanceTimer {
	char name[64];						// name of this timer
	UUtInt64 slice_time[61];			// time used in the last 60 slices
	UUtInt32 slice_quantity[61];		// number of times entered in the last 60 slices

	UUtInt64 slice_total_time;
	UUtInt32 slice_total_quantity;

	UUtUns32 current_slice;				// current time slice

	UUtUns32 active;					// timer is active if active > 0
} UUtPerformanceTimer;

UUtPerformanceTimer *UUrPerformanceTimer_New(char *name);
void UUrPerformanceTimer_Pulse(void);
void UUrPerformanceTimer_Display(COtStatusLine *lines, UUtUns32 count);
void UUrPerformanceTimer_Enter(UUtPerformanceTimer *inTimer);
void UUrPerformanceTimer_Exit(UUtPerformanceTimer *inTimer);

void UUrPerformanceTimer_SetPrefix(char *inPrefix);

/*

05. MUrXYZrEulerToQuat
11. MUrQuat_Lerp
20. MUrMatrix_Multiply

12. SSiPlatform_UpdateSoundBuffer_MSADPCM	

16. M3iGeom_TraverseBSP

19. MSrTransform_EnvPointListToFrustumScreen_ActiveVertices
24. MSrGeomContext_Method_Env_DrawGQList
15. MSrTransform_Geom_PointListAndVertexNormalToWorld

09. gl_triangle
25. gl_set_textures

*/


#endif

#endif // BFW_TIMER
