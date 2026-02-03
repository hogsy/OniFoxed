#pragma once
#ifndef ONI_AI2_Idle_H
#define ONI_AI2_Idle_H

/*
	FILE:	Oni_AI2_Idle.h

	AUTHOR:	Chris Butcher

	CREATED: April 27, 2000

	PURPOSE: Idle AI for Oni

	Copyright (c) 2000

*/

typedef enum AI2tIdleType
{
	temp
} AI2tIdleType;

typedef struct AI2tIdleState
{
	AI2tIdleType idleType;
} AI2tIdleState;


// ------------------------------------------------------------------------------------
// -- update and state change functions

void AI2rIdle_Enter(ONtCharacter *ioCharacter);
void AI2rIdle_Exit(ONtCharacter *ioCharacter);
void AI2rIdle_Update(ONtCharacter *ioCharacter);

#endif
