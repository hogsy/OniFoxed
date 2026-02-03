#pragma once

/*	FILE:	Oni_DataConsole.h

	AUTHOR:	Quinn Dunki

	CREATED: April 2 1999

	PURPOSE: control of interactive consoles in ONI

	Copyright 1998

*/

#ifndef __ONI_DATACONSOLE_H__
#define __ONI_DATACONSOLE_H__

typedef enum ONtDataConsoleType
{
	ONcDataConsole_Doors,
	ONcDataConsole_EMS,
	ONcDataConsole_Weapons
} ONtDataConsoleType;


typedef struct ONtDataConsole
{
	ONtDataConsoleType type;

	UUtUns16 flags;		// None defined
	AItScriptTrigger *trigger;	// What we're attached to
	ONtCharacter *owner;	// Who is currently using us

	// Multipurpose data:
	UUtUns16 timer;
	UUtUns16 idl,idg;
	void *pointer;
} ONtDataConsole;



UUtError ONrDataConsole_LevelBegin(
	void);

void ONrDataConsole_LevelEnd(
	void);

UUtError ONrDataConsole_Initialize(
	void);

void ONrDataConsole_Terminate(
	void);

void ONrDataConsole_Update(
	void);

void ONrDataConsole_Notify_Trigger(
	AItScriptTrigger *inTrigger,
	ONtCharacter *inCharacter,
	UUtBool inActivated);

void ONrDataConsole_Activate(
	ONtDataConsole *inConsole,
	ONtCharacter *inCharacter);

void ONrDataConsole_Deactivate(
	ONtDataConsole *inConsole,
	ONtCharacter *inCharacter);

ONtDataConsole *ONrDataConsole_New(
	ONtDataConsoleType inType);

void ONrDataConsole_Delete(
	ONtDataConsole *inConsole);

#endif
