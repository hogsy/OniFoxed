#pragma once
/*
	FILE:	Oni_Script.h

	AUTHOR:	Brent H. Pease

	CREATED: Nov 13, 1999

	PURPOSE:

	Copyright 1997, 1998

*/

#ifndef ONI_SCRIPT_H
#define ONI_SCRIPT_H

UUtError
ONrScript_Initialize(
	void);

void
ONrScript_Terminate(
	void);

UUtError
ONrScript_LevelBegin(
	char*	inLevelFolderPrefix);

void
ONrScript_LevelEnd(
	void);

UUtError
ONrScript_ExecuteSimple(
	const char *inScriptName,
	const char *inParameterName);

UUtError
ONrScript_Schedule(
	const char *inScriptName,
	const char *inParameterName,
	UUtUns32 inDelayTime,
	UUtUns32 inRepeatCount);

#endif /* ONI_SCRIPT_H */
