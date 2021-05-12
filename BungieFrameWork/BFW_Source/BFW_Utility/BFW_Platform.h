/*
	FILE:	UU_Platform.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 6, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef UU_PLATFORM_H
#define UU_PLATFORM_H

UUtError UUrPlatform_Initialize(
	UUtBool	initializeBasePlatform);
void UUrPlatform_Terminate(
	void);

// call this function on a regular basis
// inIdleTime of > 0 gives this system time
void UUrPlatform_Idle(UUtWindow mainWindow, UUtUns32 inIdleTime);

void
UUrPlatform_MameAndDestroy(
	void);

void
UUrPlatform_SIMD_Initialize(
	void);

UUtBool
UUrPlatform_SIMD_IsPresent(
	void);

void
UUrPlatform_SIMD_Report(
	void);

void
UUrPlatform_Report(
	void);

#endif /* UU_PLATFORM_H */
