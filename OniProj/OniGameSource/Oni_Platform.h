#pragma once

/*
	FILE:	Oni_Platform.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: April 2, 1997
	
	PURPOSE: platform header file
	
	Copyright 1997

*/

#ifndef ONI_PLATFORM_H
#define ONI_PLATFORM_H

#include "BFW_Motoko.h"

#if UUmPlatform == UUmPlatform_Mac

#include "Oni_Platform_Mac.h"

#elif UUmPlatform == UUmPlatform_Win32

#include "Oni_Platform_Win32.h"

#else

#error No platform defined

#endif

typedef struct ONtPlatformData
{
	UUtAppInstance 	appInstance;
	UUtWindow		gameWindow;
	
	#if defined(DEBUGGING) && DEBUGGING
		
		UUtWindow	akiraWindow;
		UUtWindow	debugWindow;
		
	#endif
	
} ONtPlatformData;

// data is in Oni.c
extern ONtPlatformData	ONgPlatformData;

UUtError ONrPlatform_Initialize(
	ONtPlatformData		*outPlatformData);

void ONrPlatform_Terminate(
	void);

void ONrPlatform_Update(
	void);
	
void ONrPlatform_ErrorHandler(
	UUtError			theError,
	char				*debugDescription,
	UUtInt32			userDescriptionRef,
	char				*message);

UUtBool
ONrPlatform_IsForegroundApp(
	void);
	
#endif /* ONI_PLATFORM_H */

