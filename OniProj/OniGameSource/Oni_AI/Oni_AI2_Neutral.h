#pragma once
#ifndef ONI_AI2_NEUTRAL_H
#define ONI_AI2_NEUTRAL_H

/*
	FILE:	Oni_AI2_Neutral.h

	AUTHOR:	Chris Butcher

	CREATED: July 13, 2000

	PURPOSE: Neutral-Character Interaction AI for Oni

	Copyright (c) Bungie Software, 2000

*/

#include "Oni_Object.h"

// ------------------------------------------------------------------------------------
// -- constants

// flags in AI2tNeutralBehavior
enum {
	AI2cNeutralBehaviorFlag_NoResume			= (1 << 0),
	AI2cNeutralBehaviorFlag_NoResumeAfterItems	= (1 << 1),
	AI2cNeutralBehaviorFlag_Uninterruptable		= (1 << 2)
};

// give_flags
enum {
	AI2cNeutralGive_Shield				= (1 << 0),
	AI2cNeutralGive_Invisibility		= (1 << 1),
	AI2cNeutralGive_LSI					= (1 << 2)
};

// flags in AI2tNeutralLine
enum {
	AI2cNeutralLineFlag_PlayerLine		= (1 << 0),
	AI2cNeutralLineFlag_GiveItem		= (1 << 1),
	AI2cNeutralLineFlag_AnimOnce		= (1 << 2),
	AI2cNeutralLineFlag_OtherAnimOnce	= (1 << 3)
};

// neutralFlags in AI2tState... persistent through multiple entries into the neutral state
enum {
	AI2cNeutralFlag_Triggered			= (1 << 0),
	AI2cNeutralFlag_ConversationBegun	= (1 << 1),
	AI2cNeutralFlag_ItemsGiven			= (1 << 2),
	AI2cNeutralFlag_Completed			= (1 << 3),
	AI2cNeutralFlag_DisableResume		= (1 << 4)
};

#define AI2cNeutral_KeyAbortFrames		15

// ------------------------------------------------------------------------------------
// -- structures

typedef struct AI2tNeutralState
{
	ONtCharacter			*target_character;
	AI2tNeutralBehavior		behavior;

	UUtBool					target_accepted;
	UUtBool					played_line;
	UUtBool					linechar_animstarted;
	UUtBool					otherchar_animstarted;
	UUtUns32				current_line;

	UUtUns32				hail_timer;
	UUtUns32				giveup_timer;
	UUtUns32				last_runto_update;
	UUtUns32				last_enemy_check;
	M3tPoint3D				triggered_point;

	UUtBool					played_abort;
	UUtBool					initiated_give;
	UUtBool					completed_give;
	UUtBool					giveanim1_started;
	UUtBool					giveanim2_started;
	UUtUns32				receive_delay;

} AI2tNeutralState;

// ------------------------------------------------------------------------------------
// -- detection and triggering

// switch to a new kind of neutral behavior
UUtBool AI2rNeutral_ChangeBehavior(ONtCharacter *ioCharacter, UUtUns16 inNeutralID, UUtBool inInitialize);

// check to see if we want to run our neutral behavior
UUtBool AI2rNeutral_CheckTrigger(ONtCharacter *ioCharacter, ONtCharacter *inTarget, UUtBool inAutomaticCheck);

// check to see if we want to accept an action event from a character
UUtBool AI2rNeutral_CheckActionAcceptance(ONtCharacter *ioCharacter, ONtCharacter *inTarget);

// ------------------------------------------------------------------------------------
// -- high-level neutral interaction functions

// enter or exit the neutral behavior
void AI2rNeutral_Enter(ONtCharacter *ioCharacter);
void AI2rNeutral_Exit(ONtCharacter *ioCharacter);

// run neutral behavior for a single tick
void AI2rNeutral_Update(ONtCharacter *ioCharacter);

// stop a neutral interaction. note: may be called with either one of the characters involved.
// returns whether it was stopped.
UUtBool AI2rNeutral_Stop(ONtCharacter *ioCharacter, UUtBool inForce, UUtBool inAbort,
						UUtBool inHostileAbort, UUtBool inResetState);

// handle pathfinding errors
UUtBool AI2rNeutral_PathfindingHandler(ONtCharacter *inCharacter, UUtUns32 inErrorID,
										UUtUns32 inParam1, UUtUns32 inParam2,
										UUtUns32 inParam3, UUtUns32 inParam4);

#endif // ONI_AI2_NEUTRAL_H
