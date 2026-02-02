/*
	FILE:	Oni_AI2_Idle.c

	AUTHOR:	Chris Butcher

	CREATED: April 27, 2000

	PURPOSE: Idle AI for Oni

	Copyright (c) 2000

*/

#include "Oni_AI2.h"
#include "Oni_AI2_Idle.h"
#include "Oni_Character.h"

// ------------------------------------------------------------------------------------
// -- update and state change functions

void AI2rIdle_Enter(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Idle);

	// set up our knowledge notify function so that we are alerted by stuff
	if (ioCharacter->ai2State.flags & AI2cFlag_NonCombatant) {
		ioCharacter->ai2State.knowledgeState.callback = &AI2rNonCombatant_NotifyKnowledge;
	} else {
		ioCharacter->ai2State.knowledgeState.callback = &AI2rAlert_NotifyKnowledge;
	}

	// set up a passive movement state
	AI2rMovement_FreeFacing(ioCharacter);
	AI2rMovement_StopAiming(ioCharacter);
	AI2rMovement_StopGlancing(ioCharacter);
	AI2rPath_Halt(ioCharacter);
}

void AI2rIdle_Exit(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Idle);

	// nothing to change
}

void AI2rIdle_Update(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Idle);
	UUmAssert(ioCharacter->ai2State.currentState->begun);

	// non-active modes decay slowly in alert status
	AI2rAlert_Decay(ioCharacter);
}
