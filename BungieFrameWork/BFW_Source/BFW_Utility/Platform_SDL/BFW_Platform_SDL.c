/*
	FILE:	UU_Platform_SDL.c
*/

#include "BFW.h"
//#include "BFW_Console.h"
#include "BFW_Platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL2/SDL_timer.h>


struct UUtInterruptProcRef
{
	UUtInterruptProc	interruptProc;
	void				*refCon;
	SDL_TimerID			timer;
};

static Uint32 UUiTimerProc(
	Uint32 interval,
	void *param)
{
	UUtInterruptProcRef *interruptProcRef = param;
	return interruptProcRef->interruptProc(interruptProcRef->refCon) ? interval : 0;
}

UUtError UUrInterruptProc_Install(
	UUtInterruptProc	interruptProc,
	UUtInt32			tiggersPerSec,
	void				*refCon,
	UUtInterruptProcRef	**outInterruptProcRef)
{
	UUtInterruptProcRef *interruptProcRef;
	
	*outInterruptProcRef = NULL;
	
	interruptProcRef = UUrMemory_Block_New(sizeof(UUtInterruptProcRef));
	
	if(interruptProcRef == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "UUrInterruptProc_Install(): Could not allocate memory for proc ref");
	}
	
	interruptProcRef->interruptProc = interruptProc;
	interruptProcRef->refCon = refCon;
	Uint32 interval = 1000 / tiggersPerSec;

	interruptProcRef->timer = SDL_AddTimer(interval, UUiTimerProc, interruptProcRef);

	if(interruptProcRef->timer == 0)
	{
		//TODO: SDL_GetError()
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "UUrInterruptProc_Install(): Could not create thread");
	}

	*outInterruptProcRef = interruptProcRef;
	
	return UUcError_None;
}

void UUrInterruptProc_Deinstall(
	UUtInterruptProcRef	*interruptProcRef)
{
	SDL_RemoveTimer(interruptProcRef->timer);
	UUrMemory_Block_Delete(interruptProcRef);
}

FILE *UUrFOpen(
	char	*name,
	char	*mode)
{
	
	return fopen(name, mode);
}

UUtError UUrPlatform_Initialize(
	UUtBool	initializeBasePlatform)
{

	return UUcError_None;
}

void UUrPlatform_Terminate(
	void)
{
}

// GetTickCount wraps after ~49.7 days (32-bit 1000th of a second timer wraps)
// call >= ~once every 10 days to it will notice that wrap
static Uint64 iGetTickCount(void)
{
	static Uint64 lastTickCount = 0;
	static Uint64 adjustment = 0;
	Uint64 curTickCount = SDL_GetTicks() + adjustment;

	if (curTickCount < lastTickCount) {
		Uint64 bump = 0x100000000;

		adjustment += bump;
		curTickCount += bump;
	}

	lastTickCount = curTickCount;

	return curTickCount;
}


// NOTE: this function bails after: 
// warps after ~2.26 years (32-bit 60th of a second timer wraps)
// must be called once atleast about once every 10 days to notice the wrap
// of the 1000th of a second timer
UUtUns32 UUrMachineTime_Sixtieths(
	void)
{
	Uint64 sdlTicks = iGetTickCount();
	Uint64 tempTicks;
	UUtUns32 outTicks;

	tempTicks = sdlTicks;
	tempTicks *= 3;
	tempTicks /= 50;

	outTicks = (UUtUns32) tempTicks;

	return outTicks;
}

UUtError
UUrAtExit_Register(
	UUtAtExitProc	inAtExitProc)
{
	atexit(inAtExitProc);

	return UUcError_None;
}

#if UUmPlatform == UUmPlatform_Win32
// NOTE: referenced by windows file code
UUtUns32 UUrSystemToSecsSince1900(SYSTEMTIME time)
{
	UUtInt32 years, leap_years, leap_days, days, i, secs;

	static UUtInt32 MonthDays[] =
		{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		//jan feb mar apr may jun jul aug sep oct nov dec

	// Calculate the number of full years
		years = time.wYear - 1970;

	// Calculate number of leap years
		leap_years = years;
		if (time.wMonth > 2)
		{
		    leap_years++;
		}
		leap_days = (leap_years + 1) / 4;


	// Calculate total days
		days = (years * 365) + leap_days;

	// Calculate days in this year
		for (i=1; i<time.wMonth; i++)
		{
		    days += MonthDays[i-1];
		}
		days += time.wDay - 1;
	
	// and calculate the seconds
		secs =  (((((days * 24) + time.wHour) * 60) + time.wMinute) * 60) + time.wSecond;
	
	return secs;
}
#endif

UUtUns32
UUrGetSecsSince1900(
	void)
{
	UUtUns32 secs;
#if UUmPlatform == UUmPlatform_Linux
	//FIXME: this is going to fail in a few years
	UUtUns32 secsSince1970 = (UUtUns32)time(NULL);
	static const UUtUns32 secondsBetween1900And1970 = 2208988800;
	secs = secsSince1970 + secondsBetween1900And1970;
#elif UUmPlatform == UUmPlatform_Win32
	SYSTEMTIME time;

	// Get the time from Win32
	GetLocalTime(&time);

	// change to seconds
	secs = UUrSystemToSecsSince1900(time);
#elif UUmPlatform == UUmPlatform_Mac
	unsigned long	time;
	
	GetDateTime(&time);
	
	secs = (UUtUns32)time + UUc1904_To_1900_era_offset;
#else
	#error
#endif

	return secs;
}

// call this function on a regular basis
// inIdleTime of > 0 gives this system time
void UUrPlatform_Idle(UUtWindow mainWindow, UUtUns32 inIdleTime)
{
	//TODO: needed?
#if 0
	BOOL				was_event;
	MSG					message;

	do
	{
		was_event =
			PeekMessage(
				&message,
				mainWindow,
				0,
				0,
				PM_REMOVE);
		if (was_event)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}
	while (was_event);
#endif

	if (inIdleTime > 0) { 
		SDL_Delay(4);
	}


	return;
}

UUtInt64 UUrMachineTime_High(void)
{

	return SDL_GetPerformanceCounter();
}

double UUrMachineTime_High_Frequency(void)
{
	return SDL_GetPerformanceFrequency();
}

UUtUns32
UUrStack_GetPCChain(
	UUtUns32	*ioChainList,
	UUtUns32	inMaxNumChains)
{
	return 0;
}

void
UUrPlatform_MameAndDestroy(
	void)
{
	//TODO: MameAndDestroy (SDL)
	//      note: this is OS specific, but doesn't seem useful anyways

	return;
}

UUtBool
UUrPlatform_SIMD_IsPresent(
	void)
{
	return UUcFalse;
}

void
UUrPlatform_SIMD_Report(
	void)
{
	//COrConsole_Print("SIMD not compiled");
}

void
UUrPlatform_Report(
	void)
{
	//COrConsole_Print("platform is SDL");
	UUrPlatform_SIMD_Report();
}

void
UUrPlatform_SIMD_Initialize(
	void)
{

}

void
UUrWindow_GetSize(
	UUtWindow			inWindow,
	UUtUns16			*outWidth,
	UUtUns16			*outHeight)
{
	int w, h;
	SDL_GL_GetDrawableSize(inWindow, &w, &h);
	*outWidth = (UUtUns16)w;
	*outHeight = (UUtUns16)h;
}
