/*	FILE:	Oni_DataConsole.c

	AUTHOR:	Quinn Dunki

	CREATED: April 2, 1999

	PURPOSE: control of interactive consoles in ONI

	Copyright 1998

*/


#include "BFW.h"
#include "BFW_Totoro.h"
#include "BFW_SoundSystem.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"
#include "BFW_Physics.h"
#include "BFW_Object.h"
#include "Oni_Weapon.h"
#include "Oni.h"
#include "ONI_GameStatePrivate.h"
#include "Oni_DataConsole.h"

#define ONcMaxDataConsoles 32

ONtDataConsole ONgDataConsoles[ONcMaxDataConsoles];
UUtUns16 ONgNumDataConsoles;

ONtDataConsole *ONrDataConsole_New(
	ONtDataConsoleType inType)
{
	/*************
	* Creates a new data console
	*/

	ONtDataConsole *newConsole;

	if (ONgNumDataConsoles >= ONcMaxDataConsoles) return NULL;
	newConsole = ONgDataConsoles + ONgNumDataConsoles;

	UUrMemory_Clear(newConsole,sizeof(ONtDataConsole));
	newConsole->type = inType;

	ONgNumDataConsoles++;
	return newConsole;
}

void ONrDataConsole_Delete(
	ONtDataConsole *inConsole)
{
	/*********************
	* Deletes a data console
	*/

	UUmAssert(ONgNumDataConsoles);
	ONgNumDataConsoles--;
	*inConsole = ONgDataConsoles[ONgNumDataConsoles];
}

static ONtDataConsole *ONiDataConsoleFromTrigger(
	AItScriptTrigger *inTrigger)
{
	/************
	* Finds console attached to this trigger
	*/

	UUtUns32 i;
	ONtDataConsole *console;

	for (i=0; i<ONgNumDataConsoles; i++)
	{
		console = ONgDataConsoles + i;
		if (console->trigger == inTrigger) return console;
	}

	return NULL;
}

void ONrDataConsole_Notify_Trigger(
	AItScriptTrigger *inTrigger,
	ONtCharacter *inCharacter,
	UUtBool inActivated)
{
	/**************
	* Notifies us a trigger was activated
	*/

	ONtDataConsole *console;

	// Is this a console?
	if (!(inTrigger->triggerClass->flags & AIcTriggerFlag_DataConsole)) return;

	console = ONiDataConsoleFromTrigger(inTrigger);
	UUmAssert(console);

	if (inActivated) ONrDataConsole_Activate(console,inCharacter);
	else ONrDataConsole_Deactivate(console,inCharacter);
}

void ONrDataConsole_Activate(
	ONtDataConsole *inConsole,
	ONtCharacter *inCharacter)
{
	/*******************
	* Activates this console by this character
	*/

	UUtUns32 i;
	OBtDoor *door;
	WPtWeapon *weapon;

	UUmAssert(inConsole);
	UUmAssert(inCharacter);
	if (inConsole->owner) return;

	inConsole->owner = inCharacter;

	switch (inConsole->type)
	{
	case ONcDataConsole_Doors:
		// Give user the local key
		if (inConsole->idl) WPrInventory_AddKey(&inCharacter->inventory,inConsole->idl);

		// Use global key to toggle all doors
		if (inConsole->idg)
		{
			for (i=0; i<ONgGameState->doors.numDoors; i++)
			{
				door = ONgGameState->doors.doors + i;
				if (door->doorClass->openKey != inConsole->idg &&
					door->doorClass->closeKey != inConsole->idg) continue;

				if (door->flags & OBcDoorFlag_Locked) door->flags &=~OBcDoorFlag_Locked;
				else door->flags |= OBcDoorFlag_Locked;
			}
		}
		break;

	case ONcDataConsole_Weapons:
		weapon = (WPtWeapon *)(inConsole->pointer);
		UUmAssert(weapon);
		ONrCharacter_PickupWeapon(inCharacter,weapon,UUcFalse);
		break;

	default:
		break;
	}
}

void ONrDataConsole_Deactivate(
	ONtDataConsole *inConsole,
	ONtCharacter *inCharacter)
{
	/*****************
	* Deactivates this console by this character
	*/

	UUmAssert(inConsole);
	UUmAssert(inCharacter);
	if (inCharacter != inConsole->owner) return;

	inConsole->owner = NULL;
}

void ONrDataConsole_Update(
	void)
{
	/**********************
	* Update state of data consoles
	*/

	UUtUns32 i;
	ONtDataConsole *console;

	for (i=0; i<ONgNumDataConsoles; i++)
	{
		console = ONgDataConsoles + i;
		if (!console->owner) continue;

		switch (console->type)
		{
		case ONcDataConsole_EMS:
			if (!console->timer)
			{
				ONrCharacter_Heal(console->owner,1,UUcFalse);
				console->timer = console->owner->characterClass->inventoryConstants.hypoTime;
			}
			else console->timer--;
			break;

		default: break;
		}
	}
}

UUtError ONrDataConsole_LevelBegin(
	void)
{
	/*********************
	* Init data consoles for a level
	*/

	UUrMemory_Clear(ONgDataConsoles,ONcMaxDataConsoles * sizeof(ONtDataConsole));
	ONgNumDataConsoles = 0;

	return UUcError_None;
}

void ONrDataConsole_LevelEnd(
	void)
{
	ONgNumDataConsoles = 0;
}

UUtError ONrDataConsole_Initialize(
	void)
{
	return UUcError_None;
}

void ONrDataConsole_Terminate(
	void)
{
}

