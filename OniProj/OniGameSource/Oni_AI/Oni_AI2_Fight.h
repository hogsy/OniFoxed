#pragma once
#ifndef ONI_AI2_FIGHT_H
#define ONI_AI2_FIGHT_H

/*
	FILE:	Oni_AI2_Fight.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: September 17, 2000
	
	PURPOSE: Fight AI for Oni
	
	Copyright (c) 2000

*/

// ------------------------------------------------------------------------------------
// -- constants

enum {
	AI2cFightFlag_InUse			= (1 << 0)
};

// ------------------------------------------------------------------------------------
// -- structure definitions

// information about an AI's participation in a melee fight, stored in AI2tMeleeState.
// NULL if the AI is not in a multiple-participant fight
typedef struct AI2tFightInfo {
	struct AI2tFightState *fight_owner;
	ONtCharacter *prev, *next;

	UUtBool has_attack_cookie;
	UUtUns32 cookie_expire_time;
} AI2tFightInfo;

// a melee fight in progress
typedef struct AI2tFightState {
	UUtUns32 flags;
	ONtCharacter *target;

	UUtUns32 num_attackers;
	UUtUns32 num_attack_cookies;
	ONtCharacter *attacker_head, *attacker_tail;
} AI2tFightState;

// ------------------------------------------------------------------------------------
// -- globals


// ------------------------------------------------------------------------------------
// -- initialization and destruction

UUtError AI2rFight_Initialize(void);
void AI2rFight_Terminate(void);

// ------------------------------------------------------------------------------------
// -- fight logic

// find out whether a fight is going on around a target
AI2tFightState *AI2rFight_FindByTarget(ONtCharacter *inTarget, UUtBool inCreateIfNecessary);

// add an attacker to a fight
void AI2rFight_AddAttacker(AI2tFightState *ioFightState, ONtCharacter *inAttacker);

// remove an attacker from a fight
void AI2rFight_RemoveAttacker(AI2tFightState *ioFightState, ONtCharacter *inAttacker, UUtBool inAutomaticallyDelete);

// stop a fight around a character (most likely because they died)
void AI2rFight_StopFight(ONtCharacter *ioCharacter);

// notify about damage inflicted
void AI2rFight_AdjustForDamage(ONtCharacter *inAttacker, ONtCharacter *inVictim, UUtUns32 inDamage);

// ------------------------------------------------------------------------------------
// -- cookie handling

// request an attack cookie
UUtBool AI2rFight_GetAttackCookie(ONtCharacter *ioCharacter);

// give up the attack cookie... this can be called on any character, even if they aren't in melee
void AI2rFight_ReleaseAttackCookie(ONtCharacter *ioCharacter);

#endif
