#pragma once
/*
	FILE:	BFW_ScriptLang_Scheduler.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Oct 29, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/
#ifndef BFW_SCRIPTLANG_SCHEDULER_H
#define BFW_SCRIPTLANG_SCHEDULER_H

// This will execute a command and schedule it if needed
// This only schedules stuff called from the engine
UUtError	
SLrScheduler_Execute(
	const char*			inName,
	SLtErrorContext*	inErrorContext,
	UUtUns16			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	SLtParameter_Actual		*ioReturnValue,
	UUtUns32			inTimeDelta,		// if 0 script is executed immediately
	UUtUns32			inNumberOfTimes,	// if 0 script executes every game tick(not recommended)
	SLtContext **				ioReferencePtr);		// used for getting a reference to track currently-running scripts

// This is used to sleep a context
UUtError
SLrScheduler_Schedule(
	SLtContext*			inContext,
	SLtErrorContext*	inErrorContext,
	UUtUns32			inTicksTillWake);

UUtError
SLrScheduler_Update(
	UUtUns32	inGameTime);

UUtError
SLrScheduler_Initialize(
	void);

void
SLrScheduler_Terminate(
	void);


#endif /* BFW_SCRIPTLANG_SCHEDULER_H */
