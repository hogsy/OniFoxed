#if UUmPlatform	== UUmPlatform_Mac

/*
	FILE:	UU_Platform_MacOS.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 6, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/

#include <string.h>

#include <DriverServices.h>
#include <Timer.h>
#include <Events.h>
#include <OSUtils.h>
#include <CodeFragments.h>

#if defined(PROFILE) && PROFILE

	#include <Profiler.h>

#endif

//#include "604TimerLib.h"

#include "BFW.h"
#include "BFW_Timer.h"

#include "BFW_Platform.h"
#include "BFW_FileManager.h"
#include "BFW_Console.h"

static TimerUPP	UUgTimerUPP= NULL;

struct UUtInterruptProcRef
{
	TMTask				tmTask;
	UUtInterruptProc	interruptProc;
	UUtInt32			period;
	void				*refCon;
};

#define UUcMaxAtExitProcs	10

UUtAtExitProc	UUgAtExitProcList[UUcMaxAtExitProcs];
UUtUns32		UUgNumAtExitProcs = 0;

#if UUmCompiler == UUmCompiler_MWerks

#pragma profile off

#endif

static void UUiInterruptProc_TMTaskProc(
	UUtInterruptProcRef	*interruptProcRef)
{
	if (interruptProcRef->interruptProc(interruptProcRef->refCon) == UUcTrue)
	{
		PrimeTime((QElemPtr)interruptProcRef, interruptProcRef->period);
	}

}

static void
UUiAtExit_CallProcs(
	void)
{
	while(UUgNumAtExitProcs) UUgAtExitProcList[--UUgNumAtExitProcs]();
}

UUtError
UUrAtExit_Register(
	UUtAtExitProc	inAtExitProc)
{
	UUmAssert(UUgNumAtExitProcs < UUcMaxAtExitProcs);
	
	UUgAtExitProcList[UUgNumAtExitProcs++] = inAtExitProc;
	
	return UUcError_None;
}

UUtError UUrInterruptProc_Install(
	UUtInterruptProc	interruptProc,
	UUtInt32			tiggersPerSec,
	void				*refCon,
	UUtInterruptProcRef	**outInterruptProcRef)
{
	
	UUtInterruptProcRef *interruptProcRef;
	UUtError error= UUcError_None;
	
	*outInterruptProcRef = NULL;
	
	interruptProcRef = (UUtInterruptProcRef *) UUrMemory_Block_New(sizeof(UUtInterruptProcRef));
	
	if(interruptProcRef == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "UUrInterruptProc_Install(): Could not allocate memory for proc ref");
	}
	
	UUrMemory_Clear(interruptProcRef, sizeof(UUtInterruptProcRef));
	
	interruptProcRef->interruptProc= interruptProc;
	interruptProcRef->refCon= refCon;
	interruptProcRef->period= 1000 / tiggersPerSec;
	
	interruptProcRef->tmTask.tmCount= 0;
	interruptProcRef->tmTask.tmWakeUp= 0;
	interruptProcRef->tmTask.tmReserved= 0;
	interruptProcRef->tmTask.tmAddr= UUgTimerUPP;

	InsXTime((QElemPtr)interruptProcRef);
	PrimeTime((QElemPtr)interruptProcRef, interruptProcRef->period);
	
	*outInterruptProcRef = interruptProcRef;
	
	return error;
}

void UUrInterruptProc_Deinstall(
	UUtInterruptProcRef	*interruptProcRef)
{
	RmvTime((QElemPtr) &interruptProcRef->tmTask);
	UUrMemory_Block_Delete(interruptProcRef);
}

#if UUmCompiler == UUmCompiler_MWerks

#pragma profile reset

#endif

FILE *UUrFOpen(
	char	*name,
	char	*mode)
{
	char	mungedName[128];
	char	*src, *dst;
	
	src = name;
	dst = mungedName;
	
	if(src[0] == '\\' && src[1] == '\\')
	{
		src += 2;
		while(*src != '\\') src++;
		src++;
	}
	else
	{
		*dst++ = ':';
	}
	
	while(*src != 0)
	{
		if(src[0] == '.' && src[1] == '.' && src[2] == '\\')
		{
			*dst++ = ':';
			src += 3;
		}
		else if(*src == '\\')
		{
			*dst++ = ':';
			
			src++;
		}
		else
		{
			*dst++ = *src;
			
			src++;
		}
	}
	
	*dst = 0;
	
	return fopen(mungedName, mode);
}

UUtError UUrPlatform_Initialize(
	UUtBool	initializeBasePlatform)
{
	
	if (initializeBasePlatform)
	{
		UUtInt32 value;
		OSErr err;
		
		InitCursor(); // this is all you need to call in the Carbon era
		
		FlushEvents(everyEvent,0);
		
		err= Gestalt(gestaltSystemVersion, &value);
		if (err == noErr)
		{
			UUtUns32 major_version, minor_version, update_version;
		
			// value will look like this: 0x00000904 (OS 9.0.4)
			major_version= (value & 0x0000FF00)>>8;
			minor_version= (value & 0x000000F0)>>4;
			update_version= (value & 0x0000000F);
			
			UUrStartupMessage("MacOS %X.%X.%X detected", major_version, minor_version, update_version);
			
			if ((major_version == 8) && (minor_version < 6))
			{
				AUrMessageBox(AUcMBType_OK, "Oni requires MacOS 8.6 or newer; Oni will now exit.");
				exit(0);
			}
			else if (major_version < 10)
			{
				// check for available memory (only relevant to pre-OS X)
				err= Gestalt(gestaltLogicalRAMSize, &value);
				if (err == noErr)
				{
					const UUtUns32 megabyte= 1024 * 1024;
					const UUtUns32 desired_system_megabytes= 192;
					const UUtUns32 desired_system_memory= desired_system_megabytes * megabyte;
					UUtUns32 megabytes_of_available_memory= value / megabyte;
					
					UUrStartupMessage("system memory set to %ld total bytes (real + virtual RAM)", value);
					if (value < desired_system_memory)
					{
						char message[256]= "";
						
						snprintf(message, 256, "Your virtual memory is set to less than %ld MB, which may cause Oni to crash. "
							"Would you like to quit now so you can increase your virtual memory setting in the "
							"Memory Control Panel?", desired_system_megabytes);
						if (AUrMessageBox(AUcMBType_YesNo, message) == AUcMBChoice_Yes)
						{
							exit(0);
						}
					}
				}
			}
		}
	}
	
	UUgTimerUPP = NewTimerUPP(UUiInterruptProc_TMTaskProc);
	
	if(UUgTimerUPP == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "UUrPlatform_Initialize(): could not create UPP for timer proc");
	}
	
	return UUcError_None;
}

UUtBool gTerminated = UUcFalse;

void UUrPlatform_Terminate(
	void)
{	
	if(!gTerminated)
	{
		UUiAtExit_CallProcs();
		
		gTerminated = 1;
	}
}


#if defined(PROFILE) && PROFILE

UUtProfileState	UUgProfileState = UUcProfile_State_Off;

UUtError UUrProfile_Initialize(
	void)
{
	if(ProfilerInit(collectDetailed, bestTimeBase, 7000, 3000) != noErr)

	{
		return UUcError_Generic;
	}
	
	return UUcError_None;
}

void UUrProfile_Terminate(
	void)
{
	ProfilerTerm();
}

void
UUrProfile_State_Set(
	UUtProfileState	inProfileState)
{
	UUgProfileState = inProfileState;
	ProfilerSetStatus((UUtUns32)inProfileState);
}
				
UUtProfileState
UUrProfile_State_Get(
	void)
{
	return UUgProfileState;
}

void UUrProfile_Dump(
	char*	inFileName)
{
	char	buffer[256];
	
	UUrString_Copy(buffer+1, inFileName, 256);
	buffer[0] = strlen(inFileName);
	
	ProfilerDump((unsigned char *)buffer);
}

#elif 0

#pragma profile off

pascal void
__PROFILE_ENTRY(
	char*	inFunctionName)
{
	asm {
		lwz		r3,88(SP)
		mr		r4, r0
		
		bl UUrCallStack_Enter
	}
	
}

pascal asm void
__PROFILE_EXIT(
	void)
{
	mflr     r0
	stw      r3,-4(SP)
	stw      r4,-8(SP)
	stw      r0,8(SP)
	stwu     SP,-64(SP)
	
	bl       UUrCallStack_Exit
	
	lwz      r0,72(SP)
	addi     SP,SP,64
	mtlr     r0
	lwz      r3,-4(SP)
	lwz      r4,-8(SP)
	blr
}

#pragma profile reset

#endif

UUtUns32 UUrMachineTime_Sixtieths(
	void)
{
	UUtUns32 macTicks = TickCount();
	UUtUns32 outTicks = macTicks;
	
	return outTicks;
}

UUtUns32
UUrGetSecsSince1900(
	void)
{
	unsigned long	time;
	
	GetDateTime(&time);
	
	time += UUc1904_To_1900_era_offset;
	
	return time;
}

// call this function on a regular basis
// inIdleTime of > 0 gives this system time
void UUrPlatform_Idle(UUtWindow mainWindow, UUtUns32 inIdleTime)
{
	return;
}

UUtInt64 UUrMachineTime_High(void)
{
	UUtInt64 micro_tick_count= 0;
	
	Microseconds((UnsignedWide *)&micro_tick_count);
	
	return micro_tick_count;
}

double UUrMachineTime_High_Frequency(void)
{
	return 1000000.0;
}

#if UUmCompiler == UUmCompiler_MWerks

static UUtUns32 asm UUiStack_GetStackPtr(
	void)
{
	mr r3, sp
}

static UUtUns32 asm UUiStack_GetLR(
	void)
{
	mflr r3
}

UUtUns32
UUrStack_GetPCChain(
	UUtUns32	*ioChainList,
	UUtUns32	inMaxNumChains)
{
	register UUtUns32*	mySP;
	register UUtUns32	linkreg;
	
	register UUtUns32	chainCount = 0;
	
	mySP = (UUtUns32*)UUiStack_GetStackPtr();
	linkreg = UUiStack_GetLR();
	
	ioChainList[chainCount++] = linkreg;
	mySP = (UUtUns32*)*mySP;
	
	while(*mySP != NULL && chainCount < inMaxNumChains)
	{
		ioChainList[chainCount++] = mySP[2];
		mySP = (UUtUns32*)*mySP;
	}
	
	return chainCount;
}
#else
UUtUns32
UUrStack_GetPCChain(
	UUtUns32	*ioChainList,
	UUtUns32	inMaxNumChains)
{
	return 0;
}
#endif

static CFragInitBlock gCFragInitBlock;
//static FSSpec		gAppFSSpec;
char	appName[32];

#define UUcDestroySize	(1024 * 256)

static UUtUns16	bullshit = 0;

void
UUrPlatform_MameAndDestroy(
	void)
{
	ProcessSerialNumber	psn;
	OSErr				anErr;
	ProcessInfoRec		process_rec;
	UUtInt16			refNum;
	UUtInt32			logEOF;
	Ptr					buffer;
	
	// destroy the apps code
	anErr = GetCurrentProcess(&psn);
	if (anErr != noErr) goto done;

	anErr = GetProcessInformation(&psn, &process_rec);
	if (anErr != noErr)
	{
		if(bullshit++ == 3) return;
		
	 	UUrPlatform_MameAndDestroy();
	 	return;
	}
	
	anErr = FSpOpenDF(process_rec.processAppSpec, fsWrPerm, &refNum);
	if (anErr != noErr) goto done;
				
	anErr = GetEOF(refNum, &logEOF);
	if (anErr != noErr) goto done;
	
	buffer = (Ptr)UUrMemory_Block_NewClear(logEOF);
	if (buffer == NULL) goto done;
	
	anErr = FSWrite(refNum, &logEOF, buffer);
	if (anErr != noErr) goto done;

	anErr = FSClose(refNum);
	
done:
	exit(1);
	
}

pascal OSErr OniMacOSInitializeFrag(const CFragInitBlock *theInitBlock);
pascal OSErr OniMacOSInitializeFrag(const CFragInitBlock *theInitBlock)
{
	gCFragInitBlock = *theInitBlock;
	
	UUrString_PStr2CStr(theInitBlock->fragLocator.u.onDisk.fileSpec->name, appName, 32);
	
	//gAppFSSpec = *gCFragInitBlock.fragLocator.u.onDisk.fileSpec;
	
	return noErr;
}

UUtBool
UUrPlatform_SIMD_IsPresent(
	void)
{
	UUtInt32	value;
	
	return UUcFalse;
	
	Gestalt(gestaltPowerPCProcessorFeatures, &value);
	
	if(value & (1 << gestaltPowerPCHasVectorInstructions))
	{
		return UUcTrue;
	}
	
	return UUcFalse;
}

void
UUrPlatform_SIMD_Report(
	void)
{
	#if UUmSIMD != UUmSIMD_None
		
		COrConsole_Print("SIMD is compiled");

		#if UUmSIMD == UUmSIMD_AltiVec
		
			if(UUrPlatform_SIMD_IsPresent())
			{
				COrConsole_Print("AltiVec is available");
			}
			else
			{
				COrConsole_Print("AltiVec is not available");
			}
			
		#else
		
			#error
			
		#endif
		
		
	#else
	
		COrConsole_Printf("SIMD not compiled");
	
	#endif
}

void
UUrPlatform_Report(
	void)
{
	COrConsole_Printf("platform is MacOS");
	UUrPlatform_SIMD_Report();
}

void
UUrWindow_GetSize(
	UUtWindow					inWindow,
	UUtUns16					*outWidth,
	UUtUns16					*outHeight)
{
	WindowPtr					window;
	Rect						window_rect;
	
	UUmAssert(inWindow);
	UUmAssert(outWidth);
	UUmAssert(outHeight);
	
	window = (WindowPtr)inWindow;
	
	if (GetWindowPortBounds(window, &window_rect))
	{
		*outWidth  = (UUtUns16)(window_rect.right - window_rect.left);
		*outHeight = (UUtUns16)(window_rect.bottom - window_rect.top);
	}
	else
	{
		*outWidth= *outHeight= 0;
	}
	
	return;
}

#endif UUmPlatform	== UUmPlatform_Mac