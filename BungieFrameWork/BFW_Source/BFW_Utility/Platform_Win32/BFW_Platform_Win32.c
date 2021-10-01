/*
	FILE:	UU_Platform_Win32.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 6, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/

#include "BFW.h"
//#include "BFW_Console.h"
#include "BFW_Platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <imagehlp.h>


//static void OurWalkStack(void);
//static void DebuggingInitialize(void);

#if 1
// this is setting the priority of this thread higher the the priority
// of the system reading input
#define cThreadPriority			THREAD_PRIORITY_TIME_CRITICAL 
#define cThreadPriorityClass	REALTIME_PRIORITY_CLASS
#else
#define cThreadPriority			THREAD_PRIORITY_ABOVE_NORMAL 
#define cThreadPriorityClass	HIGH_PRIORITY_CLASS 
#endif

#define UUcInterrupt_ThreadTimeout (3*1000)

UUtUns32 UUrSystemToSecsSince1900(SYSTEMTIME time);

struct UUtInterruptProcRef
{
	UUtInterruptProc	interruptProc;
	UUtInt32			period;
	void				*refCon;

	UUtBool				active;
	DWORD 				id;
	UUtBool				quit;

	HANDLE				thread;
};

static DWORD WINAPI UUiInterruptProc_Thread(
	UUtInterruptProcRef *interruptProcRef)
{
	UUtBool		continue_iterating = UUcTrue;
	long		sleep_time = interruptProcRef->period;
	long		initial_ticks, time_slept;

	SetThreadPriority(GetCurrentThread(), cThreadPriority);

	interruptProcRef->active = UUcTrue;
	
	while(continue_iterating)
	{
		if(!interruptProcRef->quit)
		{
			
			continue_iterating = interruptProcRef->interruptProc(interruptProcRef->refCon);
			
			/* Now sleep... */
			initial_ticks = GetTickCount();
			SleepEx(sleep_time, FALSE);

			/* Modify the sleep time based on how responsive we are.. */
			time_slept = GetTickCount() - initial_ticks;
			
			if(time_slept < interruptProcRef->period)
			{
				sleep_time++;
			}
			else if(time_slept > interruptProcRef->period)
			{
				sleep_time--;
				
				if(sleep_time < 0)
				{
					sleep_time = 0; // don't ever suspend indefinately.
				}
			}
		}
		else
		{
			continue_iterating = UUcFalse;
		}
	}
	
	interruptProcRef->active = UUcFalse;
	
	/* Note that if this ends prematurely, the memory is lost. (unless kill_thread is */
	/*  called anyway) */
	return 0l;	
}

UUtError UUrInterruptProc_Install(
	UUtInterruptProc	interruptProc,
	UUtInt32			tiggersPerSec,
	void				*refCon,
	UUtInterruptProcRef	**outInterruptProcRef)
{
	UUtInterruptProcRef *interruptProcRef;
	HANDLE 				thread;
	
	*outInterruptProcRef = NULL;
	
	interruptProcRef = UUrMemory_Block_New(sizeof(UUtInterruptProcRef));
	
	if(interruptProcRef == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "UUrInterruptProc_Install(): Could not allocate memory for proc ref");
	}
	
	interruptProcRef->interruptProc = interruptProc;
	interruptProcRef->refCon = refCon;
	interruptProcRef->period = 1000 / tiggersPerSec;
	interruptProcRef->quit = UUcFalse;
	interruptProcRef->active = UUcFalse;

	thread =
		CreateThread(
			NULL,
			128,
			(LPTHREAD_START_ROUTINE)UUiInterruptProc_Thread,
			interruptProcRef,
			0,
			&interruptProcRef->id);

	if(thread == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "UUrInterruptProc_Install(): Could not create thread");
	}

	SetThreadPriority(thread, cThreadPriority);
	interruptProcRef->thread= thread;
	
	*outInterruptProcRef = interruptProcRef;
	
	return UUcError_None;
}

void UUrInterruptProc_Deinstall(
	UUtInterruptProcRef	*interruptProcRef)
{
	DWORD initial_tick_count = GetTickCount();

	interruptProcRef->quit = UUcTrue;
	
	while(interruptProcRef->active &&
		(GetTickCount() - initial_tick_count) < UUcInterrupt_ThreadTimeout) {};

	CloseHandle(interruptProcRef->thread);
	UUrMemory_Block_Delete(interruptProcRef);
}

FILE *UUrFOpen(
	char	*name,
	char	*mode)
{
	
	return fopen(name, mode);
}

// for trapping alt+tab and friends
#if defined(SHIPPING_VERSION) && (SHIPPING_VERSION==1)
static HHOOK alt_tab_interceptor= NULL;
static BOOL running_nt= FALSE;

static LRESULT CALLBACK low_level_keyboard_proc(
	int code, 
	WPARAM wparam,
	LPARAM lparam)
{
	BOOL eat_keystroke= FALSE;

	if (code == HC_ACTION)
	{
		PKBDLLHOOKSTRUCT p;

		if (running_nt)
		{
			switch (wparam)
			{
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
				case WM_KEYUP:
				case WM_SYSKEYUP: 
					p= (PKBDLLHOOKSTRUCT)lparam;
					eat_keystroke= 
						// Windows key, Apps key trapped
						((p->vkCode == VK_LWIN) || (p->vkCode == VK_RWIN) || (p->vkCode == VK_APPS)) ||
						// alt+tab trapped
						((p->vkCode == VK_TAB) && ((p->flags & LLKHF_ALTDOWN) != 0)) ||
						// alt+esc trapped
						((p->vkCode == VK_ESCAPE) && ((p->flags & LLKHF_ALTDOWN) != 0)) ||
						// ctrl+esc trapped
						((p->vkCode == VK_ESCAPE) && ((GetKeyState(VK_CONTROL) & 0x8000) != 0) ||
						// alt + F4 trapped
						((p->vkCode == VK_F4) && ((p->flags & LLKHF_ALTDOWN) != 0)));
					break;
			}
		}
		else
		{
			switch (wparam)
			{
				case VK_LWIN: // not working
				case VK_RWIN: // not working
				case VK_APPS: // gets trapped
					eat_keystroke= TRUE;
					break;
				case VK_TAB: // alt + tab; not working
					if (HIWORD(lparam) & KF_ALTDOWN)
					{
						eat_keystroke= TRUE;
					}
					break;
				case VK_ESCAPE: // alt + esc, control + esc; not working
					if ((HIWORD(lparam) & KF_ALTDOWN) || (GetKeyState(VK_CONTROL) & 0x8000))
					{
						eat_keystroke= TRUE;
					}
					break;
				case VK_F4: // alt + F4; gets trapped
					if (HIWORD(lparam) & KF_ALTDOWN)
					{
						eat_keystroke= TRUE;
					}
					break;
			}
		}
	}

	return (eat_keystroke ? 1 : CallNextHookEx(NULL, code, wparam, lparam));
}

static void __cdecl keyboard_interceptor_delete(
	void)
{
	if (alt_tab_interceptor)
	{
		UnhookWindowsHookEx(alt_tab_interceptor);
		alt_tab_interceptor= NULL;
	}

	return;
}
#endif

UUtError UUrPlatform_Initialize(
	UUtBool	initializeBasePlatform)
{
	BOOL screen_saver_active= FALSE;
	STICKYKEYS lil_sticky_turds= {0};
	
	if (initializeBasePlatform)
	{
	}
	
	// to disable alt-tab we have to pretend we are a screensaver
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE, &screen_saver_active, 0);
	// disable stickey keys
	lil_sticky_turds.cbSize= sizeof(lil_sticky_turds);
	lil_sticky_turds.dwFlags= 0;
	SystemParametersInfo(SPI_SETSTICKYKEYS, FALSE, &lil_sticky_turds, 0);
	SetLastError(NO_ERROR);

	// DebuggingInitialize();
	// OurWalkStack();

	{
		// prevent system from enterring sleep; this only works on Win98 & higher
		HANDLE kernel32dll;

		kernel32dll= GetModuleHandle("kernel32.dll");
		if (kernel32dll || ((kernel32dll= LoadLibrary("kernel32.dll")) != NULL))
		{
			EXECUTION_STATE (WINAPI *MySetThreadExecutionState)(EXECUTION_STATE esFlags);

			if ((MySetThreadExecutionState= (void *)GetProcAddress(kernel32dll, "SetThreadExecutionState")) != NULL)
			{
				if (MySetThreadExecutionState(ES_SYSTEM_REQUIRED|ES_DISPLAY_REQUIRED) == 0)
				{
					UUrStartupMessage("failed to disable sleep");
				}
				else
				{
					UUrStartupMessage("system sleep disabled");
				}
			}
			else
			{
				UUrStartupMessage("failed to disable sleep; SetThreadExecutionState() not found");
			}
		}
		else
		{
			UUrStartupMessage("failed to disable sleep; kernel32.dll not found");
		}

		// intercept alt-tab and friends in shipping build
	#if defined(SHIPPING_VERSION) && (SHIPPING_VERSION==1)
		{
			extern HINSTANCE ONgAppHInstance; // Oni_Platform_Win32.c

			if (ONgAppHInstance)
			{
				int hook_type= WH_KEYBOARD;
				OSVERSIONINFO os_version_info= {0};

				// find out which OS the game is running under
				os_version_info.dwOSVersionInfoSize= sizeof(OSVERSIONINFO);
				if (GetVersionEx(&os_version_info) && (os_version_info.dwPlatformId == VER_PLATFORM_WIN32_NT))
				{
					// use WH_KEYBOARD_LL under NT
					hook_type= WH_KEYBOARD_LL;
					running_nt= TRUE;
				}

				alt_tab_interceptor= SetWindowsHookEx(hook_type, low_level_keyboard_proc, ONgAppHInstance, 0);
				atexit(keyboard_interceptor_delete);
				UUrStartupMessage("keystroke traps installed");
			}
			else
			{
				UUrStartupMessage("keystroke traps not installed; no application instance");
			}
		}
	#else
		UUrStartupMessage("keystroke traps not installed; this is not the shipping version of Oni");
	#endif
	}

	return UUcError_None;
}

void UUrPlatform_Terminate(
	void)
{
	BOOL screen_saver_active= TRUE;

	SetLastError(NO_ERROR);
	SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, &screen_saver_active, 0);
	
	return;
}


#if UUmCompiler	== UUmCompiler_VisC

UUtProfileState UUgProfileState = UUcProfile_State_Off;
static HINSTANCE vtuneDLLInst = NULL;

typedef BOOL ( __stdcall *vtune_fpt )( void );

vtune_fpt UUgVtPauseSampling = NULL;
vtune_fpt UUgVtResumeSampling = NULL;

UUtError
UUrProfile_Initialize(
	void)
{
    vtuneDLLInst = LoadLibrary("vtuneapi.dll");

    if(vtuneDLLInst) 
	{
		UUgVtPauseSampling = (vtune_fpt) GetProcAddress(vtuneDLLInst, "VtPauseSampling");
		UUgVtResumeSampling = (vtune_fpt) GetProcAddress(vtuneDLLInst, "VtResumeSampling");
	}

	return UUcError_None;
}

void
UUrProfile_Terminate(
	void)
{
}

void
UUrProfile_State_Set(
	UUtProfileState	inProfileState)
{
	if (UUgProfileState != inProfileState)
	{
		switch(inProfileState)
		{
			case UUcProfile_State_Off:
				if (UUgVtPauseSampling) { UUgVtPauseSampling(); }
			break;

			case UUcProfile_State_On:
				if (UUgVtResumeSampling) { UUgVtResumeSampling(); }
			break;
		}

		UUgProfileState = inProfileState;
	}

	return;
}
				
UUtProfileState
UUrProfile_State_Get(
	void)
{
	return UUgProfileState;
}
				
void
UUrProfile_Dump(
	char*	inFileName)
{
}

#endif


// GetTickCount wraps after ~49.7 days (32-bit 1000th of a second timer wraps)
// call >= ~once every 10 days to it will notice that wrap
static LONGLONG iGetTickCountLONGLONG(void)
{
	static LONGLONG lastTickCount = 0;
	static LONGLONG adjustment = 0;
	LONGLONG curTickCount = GetTickCount() + adjustment;

	if (curTickCount < lastTickCount) {
		LONGLONG bump = 0x100000000;

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
	LONGLONG windowsTicks = iGetTickCountLONGLONG();	// 1/1000 of a second
	LONGLONG tempTicks;
	UUtUns32 outTicks;

	tempTicks = windowsTicks;
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

UUtUns32
UUrGetSecsSince1900(
	void)
{
	UUtUns32 secs;
	SYSTEMTIME time;

	// Get the time from Win32
	GetLocalTime(&time);

	// change to seconds
	secs = UUrSystemToSecsSince1900(time);

	return secs;
}

#if 0

static void DebuggingInitialize(void)
{
	HANDLE process;
	HANDLE thread;
	BOOL success;
	DWORD error;

	process = GetCurrentProcess();
	error = GetLastError();

	thread = GetCurrentThread();
	error = GetLastError();

	success = SymInitialize(process, NULL, TRUE);
	error = GetLastError();

	return;
}

static void OurWalkStack(void)
{
	HANDLE process;
	HANDLE thread;
	BOOL success;
	STACKFRAME stackFrame;
	CONTEXT context;
	DWORD error;

	SetLastError(0);

	process = GetCurrentProcess();
	error = GetLastError();

	thread = GetCurrentThread();
	error = GetLastError();

	UUrMemory_Clear( &context, sizeof(context) );
	context.ContextFlags = CONTEXT_CONTROL;
	success = GetThreadContext(thread, &context);
	error = GetLastError();

	UUrMemory_Clear( &stackFrame, sizeof(stackFrame) );

    // Initialize the STACKFRAME structure for the first call.  This is only
    // necessary for Intel CPUs, and isn't mentioned in the documentation.
    stackFrame.AddrPC.Offset       = context.Eip;
    stackFrame.AddrPC.Mode         = AddrModeFlat;
    stackFrame.AddrStack.Offset    = context.Esp;
    stackFrame.AddrStack.Mode      = AddrModeFlat;
    stackFrame.AddrFrame.Offset    = context.Ebp;
    stackFrame.AddrFrame.Mode      = AddrModeFlat;
	
	do {
		DWORD displacement = 0; 
		CHAR symbol_buf[sizeof(IMAGEHLP_SYMBOL)+512]; 
		PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL) symbol_buf;
		char *name;

		success = StackWalk(
			IMAGE_FILE_MACHINE_I386, 
			process, 
			thread, 
			&stackFrame, 
			NULL,		// context record
			NULL,		// read memory routine
			SymFunctionTableAccess,		// function table access routine
			SymGetModuleBase,		// get moudle base routine
			NULL);		// translate address

		if (success) {
			UUrMemory_Clear(pSymbol, sizeof(symbol_buf));
			pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
			pSymbol->MaxNameLength = 512;

			if (SymGetSymFromAddr( process, stackFrame.AddrPC.Offset, &displacement, pSymbol )) { 
				name = pSymbol->Name; 
			} 
			else { 
				error = GetLastError();

				name = "<nosymbols>"; 
			} 
		}
	} while(success);

	return;
}

#endif

// call this function on a regular basis
// inIdleTime of > 0 gives this system time
void UUrPlatform_Idle(UUtWindow mainWindow, UUtUns32 inIdleTime)
{
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
		SleepEx(4, FALSE);
	}


	return;
}

UUtInt64 UUrMachineTime_High(void)
{
	LARGE_INTEGER time;
	BOOL success;
	UUtInt64 result;

	success = QueryPerformanceCounter(&time);
	
	if (success)
	{
		result = time.QuadPart;
	}
	else 
	{
		result = GetTickCount();
	}

	return time.QuadPart;
}

double UUrMachineTime_High_Frequency(void)
{
	LARGE_INTEGER frequency;
	BOOL success;
	double result;

	success = QueryPerformanceFrequency(&frequency);

	if (success)
	{
		result = (double)frequency.QuadPart;
	}
	else
	{
		result = 1000.0;	// resolution of GetTickCount
	}

	return result;
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
	BOOL success;
	char *commandLine = GetCommandLine();
	char ourFileName[MAX_PATH];
	char *ptr;

	strncpy(ourFileName, commandLine, MAX_PATH);
	ourFileName[MAX_PATH - 1] = 0;

	for(ptr = ourFileName; ((*ptr != ' ') && (*ptr != '\0')); ptr++) { }
	*ptr = '\0';

	success = DeleteFile(
		ourFileName);

	if (!success) {
		success = MoveFileEx(
			ourFileName,
			NULL,
			MOVEFILE_DELAY_UNTIL_REBOOT);
	}

	return;
}


#if UUmCompiler	== UUmCompiler_VisC
long __cdecl _ftol(float number)
{
	__asm 
	{
//		push        ebp
//		mov         ebp,esp
		add         esp,0F4h
		wait
		fnstcw      word ptr [ebp-2]
		wait
		mov         ax,word ptr [ebp-2]
		or          ah,0Ch
		mov         word ptr [ebp-4],ax
		fldcw       word ptr [ebp-4]
		fistp       qword ptr [ebp-0Ch]
		fldcw       word ptr [ebp-2]
		mov         eax,dword ptr [ebp-0Ch]
		mov         edx,dword ptr [ebp-8]
		leave
		ret
	}
}
#endif

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
	//COrConsole_Print("platform is Win32");
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
	RECT				rect;
	
	UUmAssert(inWindow);
	UUmAssert(outWidth);
	UUmAssert(outHeight);
	
	GetWindowRect(inWindow, &rect);
	
	*outWidth = (UUtUns16)(rect.right - rect.left);
	*outHeight = (UUtUns16)(rect.bottom - rect.top);
}
