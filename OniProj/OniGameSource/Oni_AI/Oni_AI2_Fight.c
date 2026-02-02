/*
	FILE:	Oni_AI2_Fight.c

	AUTHOR:	Chris Butcher

	CREATED: September 17, 2000

	PURPOSE: Fight AI for Oni

	Copyright (c) 2000

*/

#include "BFW.h"

#include "Oni_AI2.h"
#include "Oni_Character.h"

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cFight_MaxFights						32

#define AI2cFight_CookieTime_Initial			180
#define AI2cFight_CookieTime_DamageReceived		6
#define AI2cFight_CookieTime_DamageInflicted	10

// ------------------------------------------------------------------------------------
// -- external globals

// ------------------------------------------------------------------------------------
// -- internal globals

UUtUns32 AI2gFightBufferNumUsed;
AI2tFightState AI2gFightBuffer[AI2cFight_MaxFights];

// ------------------------------------------------------------------------------------
// -- internal function prototypes

// fight logic
static AI2tFightState *AI2iFight_NewFight(ONtCharacter *inTarget);
static void AI2iFight_DeleteFight(AI2tFightState *ioFightState);
static UUcInline AI2tFightInfo *AI2iFight_GetInfoFromCharacter(ONtCharacter *ioCharacter);

// cookie handling
static UUtBool AI2iFight_DiscardAttackCookie(AI2tFightState *ioFightState, AI2tFightInfo *ioFightInfo);
static UUtBool AI2iFight_ReleaseAttackCookie(AI2tFightState *inFightState, AI2tFightInfo *inFightInfo);

// ------------------------------------------------------------------------------------
// -- initialization and destruction

UUtError AI2rFight_Initialize(void)
{
	UUrMemory_Clear(AI2gFightBuffer, sizeof(AI2gFightBuffer));
	AI2gFightBufferNumUsed = 0;

	return UUcError_None;
}

void AI2rFight_Terminate(void)
{
	// nothing
}

static AI2tFightState *AI2iFight_NewFight(ONtCharacter *inTarget)
{
	UUtUns32 itr;
	AI2tFightState *fight;

	UUmAssert(inTarget->melee_fight == NULL);

	for (itr = 0, fight = AI2gFightBuffer; itr < AI2cFight_MaxFights; itr++, fight++) {
		if (fight->flags & AI2cFightFlag_InUse) {
			UUmAssert(fight->num_attackers > 0);
			continue;
		}

		break;
	}

	if (itr >= AI2cFight_MaxFights) {
		// there are no free fight states
		AI2_ERROR(AI2cBug, AI2cSubsystem_Fight, AI2cError_Fight_MaxFights, inTarget, AI2cFight_MaxFights, 0, 0, 0);
		return NULL;
	}

	AI2gFightBufferNumUsed = UUmMax(AI2gFightBufferNumUsed, itr + 1);
	fight->flags |= AI2cFightFlag_InUse;
	fight->num_attackers = 0;
	fight->num_attack_cookies = 0;
	fight->attacker_head = NULL;
	fight->attacker_tail = NULL;
	fight->target = inTarget;
	fight->target->melee_fight = fight;

	return fight;
}

static void AI2iFight_DeleteFight(AI2tFightState *ioFightState)
{
	UUtUns32 index;

	index = ioFightState - AI2gFightBuffer;
	UUmAssert((index >= 0) && (index < AI2gFightBufferNumUsed));

	UUmAssert(ioFightState->flags & AI2cFightFlag_InUse);
	while (ioFightState->num_attackers > 0) {
		UUmAssertReadPtr(ioFightState->attacker_head, sizeof(ONtCharacter));

		AI2rFight_RemoveAttacker(ioFightState, ioFightState->attacker_head, UUcFalse);
	}

	UUmAssertReadPtr(ioFightState->target, sizeof(ONtCharacter));
	UUmAssert(ioFightState->target->melee_fight == ioFightState);
	ioFightState->target->melee_fight = NULL;
	ioFightState->target = NULL;

	ioFightState->flags &= ~AI2cFightFlag_InUse;
	if (index + 1 == AI2gFightBufferNumUsed) {
		AI2gFightBufferNumUsed--;
	}
}

// ------------------------------------------------------------------------------------
// -- fight logic

// get the fightinfo structure in a character's state
static UUcInline AI2tFightInfo *AI2iFight_GetInfoFromCharacter(ONtCharacter *ioCharacter)
{
	UUmAssert(ioCharacter->charType == ONcChar_AI2);
	UUmAssert(ioCharacter->ai2State.currentGoal == AI2cGoal_Combat);

	return &ioCharacter->ai2State.currentState->state.combat.melee.fight_info;
}

// stop a fight around a character (most likely because they died)
void AI2rFight_StopFight(ONtCharacter *ioCharacter)
{
	if (ioCharacter->melee_fight == NULL)
		return;

	AI2iFight_DeleteFight(ioCharacter->melee_fight);
}

// find out whether a fight is going on around a target
AI2tFightState *AI2rFight_FindByTarget(ONtCharacter *inTarget, UUtBool inCreateIfNecessary)
{
	if (inTarget == NULL)
		return NULL;

	if (inTarget->melee_fight != NULL) {
		return inTarget->melee_fight;

	} else if (inCreateIfNecessary) {
		return AI2iFight_NewFight(inTarget);

	} else {
		return NULL;
	}
}

// add an attacker to a fight
void AI2rFight_AddAttacker(AI2tFightState *ioFightState, ONtCharacter *inAttacker)
{
	AI2tFightInfo *fight_info, *other_info;

	fight_info = AI2iFight_GetInfoFromCharacter(inAttacker);
	if (fight_info->fight_owner == ioFightState) {
		// the character is already part of this fight!
		return;

	} else if (fight_info->fight_owner != NULL) {
		// the character is in another fight
		AI2rFight_RemoveAttacker(fight_info->fight_owner, inAttacker, UUcTrue);
	}

	// add the attacker to the end of the linked list
	UUmAssert(fight_info->fight_owner == NULL);
	fight_info->fight_owner = ioFightState;
	fight_info->prev = ioFightState->attacker_tail;
	fight_info->next = NULL;
	fight_info->has_attack_cookie = UUcFalse;

	if (fight_info->prev == NULL) {
		ioFightState->attacker_head = inAttacker;
	} else {
		other_info = AI2iFight_GetInfoFromCharacter(fight_info->prev);
		other_info->next = inAttacker;
	}
	ioFightState->attacker_tail = inAttacker;

	ioFightState->num_attackers++;
}

// remove an attacker from a fight
void AI2rFight_RemoveAttacker(AI2tFightState *ioFightState, ONtCharacter *inAttacker, UUtBool inAutomaticallyDelete)
{
	AI2tFightInfo *fight_info, *other_info;

	UUmAssert(ioFightState->flags & AI2cFightFlag_InUse);
	UUmAssert(ioFightState->num_attackers > 0);

	fight_info = AI2iFight_GetInfoFromCharacter(inAttacker);
	UUmAssert(fight_info->fight_owner == ioFightState);

	AI2iFight_DiscardAttackCookie(ioFightState, fight_info);

	// remove the attacker from the linked list
	if (fight_info->prev == NULL) {
		ioFightState->attacker_head = fight_info->next;
	} else {
		other_info = AI2iFight_GetInfoFromCharacter(fight_info->prev);
		other_info->next = fight_info->next;
	}
	if (fight_info->next == NULL) {
		ioFightState->attacker_tail = fight_info->prev;
	} else {
		other_info = AI2iFight_GetInfoFromCharacter(fight_info->next);
		other_info->prev = fight_info->prev;
	}

	fight_info->prev = NULL;
	fight_info->next = NULL;
	fight_info->fight_owner = NULL;

	ioFightState->num_attackers--;
	if (ioFightState->num_attackers == 0) {
		UUmAssert(ioFightState->attacker_head == NULL);
		UUmAssert(ioFightState->attacker_tail == NULL);
		if (inAutomaticallyDelete) {
			AI2iFight_DeleteFight(ioFightState);
		}
	}
}

// ------------------------------------------------------------------------------------
// -- cookie handling

// get rid of the attack cookie
static UUtBool AI2iFight_DiscardAttackCookie(AI2tFightState *ioFightState, AI2tFightInfo *ioFightInfo)
{
	if (!ioFightInfo->has_attack_cookie)
		return UUcFalse;

	ioFightInfo->has_attack_cookie = UUcFalse;

	UUmAssert(ioFightState->num_attack_cookies > 0);
	ioFightState->num_attack_cookies--;
	return UUcTrue;
}

// request an attack cookie
UUtBool AI2rFight_GetAttackCookie(ONtCharacter *ioCharacter)
{
	AI2tFightState *fight_state;
	AI2tFightInfo *fight_info;
	UUtUns32 current_time = ONrGameState_GetGameTime();

	fight_info = AI2iFight_GetInfoFromCharacter(ioCharacter);
	fight_state = fight_info->fight_owner;

	if (fight_state == NULL) {
		// there is no fight manager governing this character
		fight_info->has_attack_cookie = UUcTrue;

	} else if (fight_info->has_attack_cookie) {
		// check to see if we have run out of time
		if (fight_info->cookie_expire_time > current_time) {
			// we keep our existing attack cookie
			return UUcTrue;
		} else {
			// lose the cookie, our time is up
			if (AI2iFight_ReleaseAttackCookie(fight_state, fight_info)) {
				// someone else can now take the cookie
				return UUcFalse;
			}
		}

	} else if (fight_state->num_attack_cookies < (UUtUns32) AI2gSpacingMaxCookies) {
		// there is a free cookie
		fight_info->has_attack_cookie = UUcTrue;
		fight_state->num_attack_cookies++;

	} else {
		// there are no free cookies
		return UUcFalse;
	}

	// we have a new attack cookie, reset our timer
	UUmAssert(fight_info->has_attack_cookie);
	fight_info->cookie_expire_time = current_time + AI2cFight_CookieTime_Initial;
	return UUcTrue;
}

// give up the attack cookie to anyone who wants it
static UUtBool AI2iFight_ReleaseAttackCookie(AI2tFightState *inFightState, AI2tFightInfo *inFightInfo)
{
	UUmAssertReadPtr(inFightState, sizeof(AI2tFightState));
	UUmAssertReadPtr(inFightInfo, sizeof(AI2tFightInfo));

	if (inFightState->num_attackers <= inFightState->num_attack_cookies) {
		// there is nobody to give the cookie to
		return UUcFalse;
	}

	return AI2iFight_DiscardAttackCookie(inFightState, inFightInfo);
}

// give up the attack cookie to anyone who wants it... this can be called on any character, even if they aren't in melee
void AI2rFight_ReleaseAttackCookie(ONtCharacter *ioCharacter)
{
	if ((ioCharacter->charType == ONcChar_AI2) && (ioCharacter->ai2State.currentGoal == AI2cGoal_Combat)) {
		// check to see if the character is part of a fight
		AI2tFightInfo *fight_info = AI2iFight_GetInfoFromCharacter(ioCharacter);
		AI2tFightState *fight_state = fight_info->fight_owner;

		if (fight_state != NULL) {
			// give up the attack cookie, if we have it
			AI2iFight_ReleaseAttackCookie(fight_state, fight_info);
		}
	}
}

// notify about damage inflicted
void AI2rFight_AdjustForDamage(ONtCharacter *inAttacker, ONtCharacter *inVictim, UUtUns32 inDamage)
{
	AI2tFightInfo *fight_info;

	if ((inAttacker->charType == ONcChar_AI2) && (inAttacker->ai2State.currentGoal == AI2cGoal_Combat)) {
		fight_info = &inAttacker->ai2State.currentState->state.combat.melee.fight_info;

		if (fight_info->has_attack_cookie) {
			// the attack is more likely to continue fighting
			fight_info->cookie_expire_time += AI2cFight_CookieTime_DamageInflicted * inDamage;
		}
	}

	if ((inVictim->charType == ONcChar_AI2) && (inVictim->ai2State.currentGoal == AI2cGoal_Combat)) {
		fight_info = &inVictim->ai2State.currentState->state.combat.melee.fight_info;

		if (fight_info->has_attack_cookie) {
			fight_info->cookie_expire_time -= AI2cFight_CookieTime_DamageReceived * inDamage;
		}
	}
}
