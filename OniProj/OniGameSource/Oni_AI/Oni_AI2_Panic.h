#pragma once
#ifndef ONI_AI2_PANIC_H
#define ONI_AI2_PANIC_H

/*
	FILE:	Oni_AI2_Panic.h

	AUTHOR:	Chris Butcher

	CREATED: July 16, 2000

	PURPOSE: Noncombatant AI for Oni

	Copyright (c) Bungie Software, 2000

*/

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cPanic_DefaultMelee			600
#define AI2cPanic_DefaultGunfire		900
#define AI2cPanic_DefaultSight			600
#define AI2cPanic_DefaultHurt			1200

// ------------------------------------------------------------------------------------
// -- structures

typedef struct AI2tPanicState
{
	AI2tKnowledgeEntry		*panic_cause;
	UUtUns32				panic_endtime;

	UUtBool					kneeling;
	UUtBool					currently_dazed;
	UUtBool					panicked_by_hostile;
	UUtUns32				say_timer;

} AI2tPanicState;

// ------------------------------------------------------------------------------------
// -- high-level state functions

// enter or exit the panic behavior
void AI2rPanic_Enter(ONtCharacter *ioCharacter);
void AI2rPanic_Exit(ONtCharacter *ioCharacter);

// run panic behavior for a single tick
void AI2rPanic_Update(ONtCharacter *ioCharacter);

// ------------------------------------------------------------------------------------
// -- contact and knowledge management

// new-knowledge notification callback for non-combatant characters
void AI2rNonCombatant_NotifyKnowledge(ONtCharacter *ioCharacter, AI2tKnowledgeEntry *inEntry,
										UUtBool inDegrade);

// an entry in our knowledge base has just been removed, forcibly delete any references we had to it
UUtBool AI2rPanic_NotifyKnowledgeRemoved(ONtCharacter *ioCharacter, AI2tPanicState *ioPanicState, AI2tKnowledgeEntry *inEntry);

// check to see if a character needs to worry about a hostile contact
// optional inInteractingWithCharacter: also consider if these enemies are a danger to character we're talking to
UUtBool AI2rCharacter_IsDangerousContact(ONtCharacter *ioCharacter, ONtCharacter *inInteractingWithCharacter,
								   UUtBool inConsiderOnlyHostiles, AI2tKnowledgeEntry *inEntry, M3tPoint3D *inCurrentLocation);

// check to see if a neutral character can see enemies close enough to be scared
// optional inInteractingWithCharacter: also consider if these enemies are a danger to character we're talking to
UUtBool AI2rCharacter_InDanger(ONtCharacter *ioCharacter, ONtCharacter *inInteractingWithCharacter,
							   UUtBool inConsiderOnlyHostiles, AI2tKnowledgeEntry **outDangerousContact);

// we're in danger; panic
void AI2rCharacter_Panic(ONtCharacter *ioCharacter, AI2tKnowledgeEntry *inEntry, UUtUns32 inPanicTime);

#endif // ONI_AI2_PANIC_H
