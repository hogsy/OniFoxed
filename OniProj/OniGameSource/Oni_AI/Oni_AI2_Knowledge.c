/*
	FILE:	Oni_AI2_Knowledge.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: April 18, 2000
	
	PURPOSE: Knowledge base for Oni AI
	
	Copyright (c) 2000

*/

#include "bfw_math.h"

#include "Oni_AI2.h"
#include "Oni_AI2_Knowledge.h"
#include "Oni_Character.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Path.h"

#define AI_VERBOSE_VISIBILITY					0

// ------------------------------------------------------------------------------------
// -- constants

#define AI2cKnowledge_DodgeFrames				5
#define AI2cKnowledge_DegradeCheckFrames		60
#define AI2cKnowledge_VisibilityFrames			59
#define AI2cKnowledge_VisibilityCoprime			13
#define AI2cKnowledge_GlanceFrames				15
#define AI2cKnowledge_PursuitFrames				15
#define AI2cKnowledge_StartleFrames				4
#define AI2cKnowledge_MinNotifyFrames			60

#define AI2cKnowledge_StartleDegradeFrames		180

#define AI2cKnowledge_MaxEntries				128
#define AI2cKnowledge_MaxPending				64

#define AI2cKnowledge_MaxDodgeProjectiles		64
#define AI2cKnowledge_MaxDodgeFiringSpreads		32

#define AI2cVisibility_VerticalThresholdDist	20.0f
#define AI2cInvisibility_TrackDist				40.0f

#define AI2cProjectile_DodgeTrajectoryFrames	50

#define AI2cKnowledge_DebugMaxNumSounds			20
#define AI2cKnowledge_DebugSoundDecay			30

#define AI2cKnowledge_HostilityPersistTime		900

#define AI2cKnowledge_DangerShoutVolume_Shot	120.0f
#define AI2cKnowledge_Sound_DefiniteContactDist	40.0f

#define AI2cKnowledge_InvisibleFrames			90

const char AI2cContactStrengthName[AI2cContactStrength_Max][16]
	= {"dead", "forgotten", "weak", "strong", "definite"};

// ------------------------------------------------------------------------------------
// -- internal structures

// a pending knowledge entry that hasn't yet been added to the knowledgebase
typedef struct AI2tKnowledgePending
{
	ONtCharacter *			owner;
	ONtCharacter *			enemy;

	AI2tContactStrength		strength;
	M3tPoint3D				location;
	AI2tContactType			type;
	UUtUns32				user_data;
	UUtBool					disable_notify;
	UUtBool					force_target;

} AI2tKnowledgePending;

typedef struct AI2tKnowledgeDebugSound
{
	M3tPoint3D				location;
	float					radius;
	AI2tContactType			type;
	UUtUns32				time;

} AI2tKnowledgeDebugSound;

// ------------------------------------------------------------------------------------
// -- internal globals

// AI character knowledge base
UUtUns32 AI2gKnowledge_ClearSpaceIndex;
UUtBool AI2gKnowledge_Valid = UUcFalse;
AI2tKnowledgeEntry *AI2gKnowledgeBase = NULL;
AI2tKnowledgeEntry *AI2gKnowledge_NextFreeEntry = NULL;

// pending knowledge storage
UUtBool AI2gKnowledge_ImmediatePost;
UUtUns32 AI2gKnowledge_NumPending;
AI2tKnowledgePending AI2gKnowledge_Pending[AI2cKnowledge_MaxPending];

// AI damaging projectile knowledge base
UUtUns32 AI2gKnowledge_NumDodgeProjectiles;
AI2tDodgeProjectile AI2gKnowledge_DodgeProjectile[AI2cKnowledge_MaxDodgeProjectiles];

// AI firing spread knowledge base
UUtUns32 AI2gKnowledge_NumDodgeFiringSpreads;
AI2tDodgeFiringSpread AI2gKnowledge_DodgeFiringSpread[AI2cKnowledge_MaxDodgeFiringSpreads];

#if TOOL_VERSION
// debugging storage of sounds generated since the last display update
UUtUns32 AI2gKnowledge_DebugNumSounds;
AI2tKnowledgeDebugSound AI2gKnowledge_DebugSounds[AI2cKnowledge_DebugMaxNumSounds];
const IMtShade AI2gKnowledge_DebugSoundShade[AI2cContactType_Max] = {IMcShade_Blue, IMcShade_Green, IMcShade_Yellow, IMcShade_Orange, IMcShade_Red,
																	IMcShade_White, IMcShade_White, IMcShade_White, IMcShade_White, IMcShade_White};
#endif

// ------------------------------------------------------------------------------------
// -- internal function definitions

// knowledge management
static void AI2iKnowledge_AddPending(void);
static void AI2iKnowledge_PostContact(ONtCharacter *inCharacter, AI2tContactType inType,
									 AI2tContactStrength inStrength, M3tPoint3D *inLocation,
									 ONtCharacter *inTarget, UUtUns32 inAIUserData, 
									 UUtBool inDisableNotify, UUtBool inForceTarget);
static void AI2iKnowledge_AddContact(ONtCharacter *inCharacter, AI2tContactType inType,
									 AI2tContactStrength inStrength, M3tPoint3D *inLocation,
									 ONtCharacter *inTarget, UUtUns32 inAIUserData, 
									 UUtBool inDisableNotify, UUtBool inForceTarget);
static void	AI2iKnowledge_CheckVisible(ONtCharacter *inCharacter, ONtActiveCharacter *inActiveCharacter,
									   UUtUns32 inCheckCount, ONtCharacter **inCheckList);
static UUtBool AI2iKnowledge_IsVisible(ONtCharacter *inCharacter, ONtCharacter *inTarget);
static UUtBool AI2iKnowledge_RespondToAlarm(ONtCharacter *inCharacter, UUtUns8 inAlarmID);

// contact list management
static AI2tKnowledgeEntry *AI2iKnowledge_NewEntry(ONtCharacter *inOwner, AI2tKnowledgeState *ioKnowledgeState);
static void AI2iKnowledge_RemoveEntry(AI2tKnowledgeEntry *ioEntry, UUtBool inEraseReferences);
static void AI2iKnowledge_MoveContactUp(ONtCharacter *inOwner, AI2tKnowledgeState *ioKnowledgeState,
										AI2tKnowledgeEntry *ioEntry);
static void AI2iKnowledge_MoveContactDown(ONtCharacter *inOwner, AI2tKnowledgeState *ioKnowledgeState,
										AI2tKnowledgeEntry *ioEntry);

// utility functions
static UUtUns32 AI2iKnowledge_GetDegradeTime(ONtCharacter *inCharactr, AI2tContactStrength inStrength, AI2tContactStrength inHighestStrength);
static UUtBool AI2iIgnoreEvents(ONtCharacter *inUpdatingCharacter, ONtCharacter *inCause, UUtBool inCombatEvent);

// dodge routines
static void AI2iKnowledge_UpdateDodgeProjectiles(void);
static void AI2iKnowledge_UpdateDodgeProjectile(AI2tDodgeProjectile *ioProjectile);
static void AI2iKnowledge_PurgeDodgeProjectile(AI2tDodgeProjectile *ioProjectile);

// firing-spread routines
static void AI2iKnowledge_UpdateDodgeFiringSpreads(void);
static void AI2iKnowledge_UpdateDodgeFiringSpread(AI2tDodgeFiringSpread *ioSpread);

// projectile alertness
static void AI2iKnowledge_UpdateProjectileAlertness(void);

// ------------------------------------------------------------------------------------
// -- global routines

UUtError AI2rKnowledge_LevelBegin(void)
{
	// allocate and clear the knowledge base
	AI2gKnowledgeBase = UUrMemory_Block_New(AI2cKnowledge_MaxEntries * sizeof(AI2tKnowledgeEntry));
	AI2gKnowledge_Valid = UUcTrue;

	AI2rKnowledge_Reset();

	return UUcError_None;
}

void AI2rKnowledge_LevelEnd(void)
{
	if (AI2gKnowledge_Valid) {
		UUrMemory_Block_Delete(AI2gKnowledgeBase);
		AI2gKnowledgeBase = NULL;
		AI2gKnowledge_Valid = UUcFalse;
	}
}

void AI2rKnowledge_Update(void)
{
	ONtCharacter **character_list, *update_character, *looking_character, *player_character;
	ONtActiveCharacter *active_character;
	UUtUns32 check_index, character_list_count, current_time, char_seed, itr, quick_frames;
	UUtBool degrade_forget, degrade_dead, degrade_hostility, run_vision;
	AI2tKnowledgeEntry *entry;
	AI2tKnowledgeState *knowledge_state;

	// add any knowledge events that are pending for this frame
	AI2rKnowledge_Verify();
	AI2iKnowledge_AddPending();

	player_character = ONrGameState_GetPlayerCharacter();
	current_time = ONrGameState_GetGameTime();

	// note: this must come before AI2iKnowledge_UpdateDodgeProjectiles because any projectiles
	// that are scheduled for deletion will be purged when we reach that function
	AI2iKnowledge_UpdateProjectileAlertness();

	AI2rKnowledge_Verify();

	if ((current_time % AI2cKnowledge_DodgeFrames) == 0) {
		AI2iKnowledge_UpdateDodgeProjectiles();
		AI2iKnowledge_UpdateDodgeFiringSpreads();
	}

	AI2rKnowledge_Verify();

	if ((current_time % AI2cKnowledge_DegradeCheckFrames) == 0) {
		// check all knowledge entries to see if they degrade
		for (itr = 0, entry = AI2gKnowledgeBase; itr < AI2gKnowledge_ClearSpaceIndex; itr++, entry++) {
			if (entry->owner == NULL)	// skip unused entries
				continue;
				
			if (entry->strength <= AI2cContactStrength_Forgotten)
				continue;

			UUmAssertReadPtr(entry->owner, sizeof(ONtCharacter));
			if ((entry->owner->charType != ONcChar_AI2) ||
				((entry->owner->flags & ONcCharacterFlag_InUse) == 0) ||
				(entry->owner->flags & ONcCharacterFlag_Dead)) {
				// owner AI has died - kill the entry
				AI2iKnowledge_RemoveEntry(entry, UUcFalse);
				continue;
			}

			degrade_hostility = degrade_forget = degrade_dead = UUcFalse;

			if ((entry->enemy != NULL) && 
				(((entry->enemy->flags & ONcCharacterFlag_InUse) == 0) ||
				(entry->enemy->flags & ONcCharacterFlag_Dead))) {
				// target has died
				degrade_dead = UUcTrue;

			} else if ((entry->has_been_hostile) && (current_time > entry->degrade_hostility_time)) {
				// hostile actions wear off over time
				degrade_hostility = UUcTrue;

			} else if ((entry->strength == AI2cContactStrength_Definite) && (entry->enemy != NULL) &&
						(!AI2rKnowledge_CharacterCanBeSeen(entry->enemy))) {
				// character is invisible, we can't see them any more
				degrade_forget = UUcTrue;

			} else if ((current_time >= entry->degrade_time) && ((entry->owner->ai2State.flags & AI2cFlag_Omniscient) == 0)) {
				// only degrade if it's our time, and the AI is not omniscient
				degrade_forget = UUcTrue;

			} else {
				continue;
			}

			if (degrade_hostility) {
				entry->has_been_hostile = UUcFalse;
				if (entry->priority == AI2cContactPriority_Hostile_Threat) {
					entry->priority = AI2cContactPriority_Hostile_NoThreat;
				}

			} else {
				if (degrade_dead) {
					entry->strength = AI2cContactStrength_Dead;
				} else {
					entry->strength--;
				}

				entry->degrade_time = current_time + AI2iKnowledge_GetDegradeTime(entry->owner, entry->strength, entry->highest_strength);
			}

			knowledge_state = &entry->owner->ai2State.knowledgeState;

			// sort this entry to where it belongs in the contact list
			AI2iKnowledge_MoveContactDown(entry->owner, knowledge_state, entry);
	
			if (knowledge_state->callback != NULL) {
				(knowledge_state->callback)(entry->owner, entry, UUcTrue);
			}

			entry->last_strength = entry->strength;
		}
	}

	AI2rKnowledge_Verify();

	if ((!AI2gBlind) && (!AI2gEveryonePassive)) {
		// loop over all AIs
		character_list = ONrGameState_LivingCharacterList_Get();
		character_list_count = ONrGameState_LivingCharacterList_Count();
		
		// only check roughly every second
		check_index = current_time % AI2cKnowledge_VisibilityFrames;
		
		for (itr = 0; itr < character_list_count; itr++) {
			looking_character = character_list[itr];
			if (((looking_character->flags & ONcCharacterFlag_InUse) == 0) ||
				(looking_character->flags & ONcCharacterFlag_Dead) ||
				(looking_character->charType != ONcChar_AI2))
				continue;
			
			if ((looking_character->ai2State.flags & AI2cFlag_Passive) ||
				(looking_character->ai2State.flags & AI2cFlag_Blind))
				continue;

			active_character = ONrGetActiveCharacter(looking_character);

			// FIXME: would it be more efficient to maintain character lists for
			// each team and loop over only the relevant ones?
			
			run_vision = UUcFalse;

			// respect the delay timer (used so that AIs which are looking towards the source of a sound
			// don't see anything until their head is completely turned)
			if (looking_character->ai2State.knowledgeState.vision_delay_timer > 0) {
				if (AI2rExecutor_AimReachedTarget(looking_character)) {
					// we have turned our head completely!
					looking_character->ai2State.knowledgeState.vision_delay_timer = 0;
				} else {
					// tick down timer so we don't get stuck in this state forever
					looking_character->ai2State.knowledgeState.vision_delay_timer--;
				}

				if (looking_character->ai2State.knowledgeState.vision_delay_timer == 0) {
					run_vision = UUcTrue;
				}
			}

			if (!run_vision) {
				// determine whether we want to check vision this frame
				if ((current_time - looking_character->ai2State.startleTimer < AI2cAlert_MaxStartleDuration) &&
					(looking_character->ai2State.currentGoal != AI2cGoal_Combat)) {
					// AIs that have been startled but haven't yet entered combat mode
					// check very quickly, because they are likely to be spinning around
					quick_frames = AI2cKnowledge_StartleFrames;

				} else if (looking_character->ai2State.currentGoal == AI2cGoal_Pursuit) {
					// AIs that are pursuiing check more regularly
					quick_frames = AI2cKnowledge_PursuitFrames;

				} else if (looking_character->movementOrders.glance_timer > 0) {
					// AIs that are glancing check more often because
					// their viewing direction is changing quickly
					quick_frames = AI2cKnowledge_GlanceFrames;

				} else {
					quick_frames = 0;
				}

				char_seed = looking_character->index * AI2cKnowledge_VisibilityCoprime;
				if (quick_frames) {
					// check more often than normal
					run_vision = (char_seed % quick_frames == current_time % quick_frames);
				} else {
					// otherwise, evenly spread AIs throughout the check interval
					run_vision = (char_seed % AI2cKnowledge_VisibilityFrames == check_index);
				}
			}

			if (run_vision) {
				if (active_character == NULL) {
					// inactive characters only check to see if they can see the player
					AI2iKnowledge_CheckVisible(looking_character, NULL, 1, &player_character);
				} else {
					// active characters check everyone
					AI2iKnowledge_CheckVisible(looking_character, active_character, character_list_count, character_list);
				}
			}
		}
	}

	AI2rKnowledge_Verify();

	if (!AI2gEveryonePassive) {
		// loop over all AIs and make sure that they get notifications about their knowledge at least
		// every so often
		character_list = ONrGameState_LivingCharacterList_Get();
		character_list_count = ONrGameState_LivingCharacterList_Count();
		
		for (itr = 0; itr < character_list_count; itr++) {
			update_character = character_list[itr];
			if (((looking_character->flags & ONcCharacterFlag_InUse) == 0) ||
				(looking_character->flags & ONcCharacterFlag_Dead) ||
				(looking_character->charType != ONcChar_AI2) ||
				(looking_character->ai2State.flags & AI2cFlag_Passive))
				continue;

			knowledge_state = &looking_character->ai2State.knowledgeState;
			entry = knowledge_state->contacts;
			if (entry == NULL)
				continue;

			if (entry->last_notify_time + AI2cKnowledge_MinNotifyFrames < current_time)
				continue;

			// re-notify the AI about its current highest-priority contact
			entry->last_notify_time = current_time;
			if (knowledge_state->callback != NULL) {
				(knowledge_state->callback)(looking_character, entry, UUcFalse);
			}
		}
	}
}

void AI2rKnowledge_Reset(void)
{
	// clear the knowledge base for new usage. helps maintain contiguity.
	UUmAssert(AI2gKnowledge_Valid);
	UUrMemory_Clear(AI2gKnowledgeBase, AI2cKnowledge_MaxEntries * sizeof(AI2tKnowledgeEntry));

	AI2gKnowledge_ClearSpaceIndex = 0;
	AI2gKnowledge_NextFreeEntry = NULL;

	AI2gKnowledge_ImmediatePost = UUcFalse;
	AI2gKnowledge_NumPending = 0;
	UUrMemory_Clear(AI2gKnowledge_Pending, sizeof(AI2gKnowledge_Pending));

	// set up the dodge knowledge base
	AI2gKnowledge_NumDodgeProjectiles = 0;
	UUrMemory_Clear(AI2gKnowledge_DodgeProjectile, sizeof(AI2gKnowledge_DodgeProjectile));

	AI2gKnowledge_NumDodgeFiringSpreads = 0;
	UUrMemory_Clear(AI2gKnowledge_DodgeFiringSpread, sizeof(AI2gKnowledge_DodgeFiringSpread));

#if TOOL_VERSION
	AI2gKnowledge_DebugNumSounds = 0;
#endif
}

void AI2rKnowledge_ImmediatePost(UUtBool inImmediatePost)
{
	AI2gKnowledge_ImmediatePost = inImmediatePost;
}

// ------------------------------------------------------------------------------------
// -- state-update routines

void AI2rKnowledge_CharacterDeath(ONtCharacter *inDeadCharacter)
{
	UUtUns32 itr;
	AI2tKnowledgeEntry *entry;
	AI2tKnowledgeState *knowledge_state;

	if (!AI2gKnowledge_Valid) {
		// this is part of the gamestate unload process
		return;
	}

	// check all knowledge entries to see if they are about this character
	for (itr = 0, entry = AI2gKnowledgeBase; itr < AI2gKnowledge_ClearSpaceIndex; itr++, entry++) {
		if (entry->owner == NULL)	// skip unused entries
			continue;
				
		if (entry->enemy != inDeadCharacter)
			continue;

		// this entry refers to the dead character. if they already know it's dead or have
		// forgotten about it, no change
		if (entry->strength <= AI2cContactStrength_Forgotten)
			continue;

		// check to see that the AI which created this entry is still around
		UUmAssertReadPtr(entry->owner, sizeof(ONtCharacter));
		if ((entry->owner->charType != ONcChar_AI2) ||
			((entry->owner->flags & ONcCharacterFlag_InUse) == 0) ||
			(entry->owner->flags & ONcCharacterFlag_Dead)) {
			// owner AI has died - kill the entry
			AI2iKnowledge_RemoveEntry(entry, UUcFalse);
			continue;
		}

		// update the entry
		entry->strength = AI2cContactStrength_Dead;
		entry->degrade_time = 0;

		knowledge_state = &entry->owner->ai2State.knowledgeState;

		// sort this entry to where it belongs in the contact list
		AI2iKnowledge_MoveContactDown(entry->owner, knowledge_state, entry);
	
		if (knowledge_state->callback != NULL) {
			(knowledge_state->callback)(entry->owner, entry, UUcTrue);
		}

		entry->last_strength = entry->strength;
	}
}

void AI2rKnowledge_HurtBy(ONtCharacter *ioCharacter, ONtCharacter *inAttacker,
							UUtUns32 inHitPoints, UUtBool inMelee)
{
	if ((AI2gEveryonePassive) || (ioCharacter->flags & AI2cFlag_Passive))
		return;

	if ((inAttacker == NULL) || (AI2rTeam_FriendOrFoe(ioCharacter->teamNumber, inAttacker->teamNumber) == AI2cTeam_Friend)) {
		// the character in question is our friend, or we don't know who shot us
		// FIXME: move aside out of the line of fire?
	} else {
		// enter this character into our knowledge base - will cause a transition into the
		// combat state if appropriate, or run away in fear
		AI2iKnowledge_PostContact(ioCharacter, (inMelee) ? AI2cContactType_Hit_Melee : AI2cContactType_Hit_Weapon,
								(inMelee) ? AI2cContactStrength_Definite : AI2cContactStrength_Strong,
								&inAttacker->location, inAttacker, inHitPoints, UUcFalse, UUcFalse);
	}

	if (!inMelee) {
		ONtCharacter *primary, *secondary;

		if (inAttacker == NULL) {
			primary = ioCharacter;
			secondary = NULL;
		} else {
			primary = inAttacker;
			secondary = ioCharacter;
		}

		// we just took damage from gunfire, communicate this fact to our friends nearby so that they will at least
		// get their alert state raised even if they don't know where our attacker is
		AI2rKnowledge_Sound(AI2cContactType_Sound_Gunshot, &ioCharacter->location,
							AI2cKnowledge_DangerShoutVolume_Shot, primary, secondary);
	}
}

void AI2rKnowledgeState_Initialize(AI2tKnowledgeState *ioKnowledgeState)
{
	ioKnowledgeState->vision_delay_timer = 0;
	ioKnowledgeState->num_contacts = 0;
	ioKnowledgeState->contacts = NULL;
	ioKnowledgeState->callback = AI2rAlert_NotifyKnowledge;		// default knowledge callback
}

// generate a sound that AIs can hear. volume = distance at which characters with acuity 1.0 will hear
void AI2rKnowledge_Sound(AI2tContactType inImportance, M3tPoint3D *inLocation, float inVolume,
						 ONtCharacter *inTarget1, ONtCharacter *inTarget2)
{
	ONtCharacter **character_list, *character;
	M3tPoint3D char_pt, target1_pt, target2_pt;
	UUtUns32 character_list_count;
	float distance_sq, distance, hearing_dist;
	AI2tContactStrength contact_strength;
	UUtBool combat_sound, ignore_target1, ignore_target2;
	UUtUns32 itr;
	UUtError error;

#if TOOL_VERSION
	if (AI2gShowSounds && (AI2gKnowledge_DebugNumSounds < AI2cKnowledge_DebugMaxNumSounds)) {
		// add this sound to the global sound debugging buffer
		AI2tKnowledgeDebugSound *sound = &AI2gKnowledge_DebugSounds[AI2gKnowledge_DebugNumSounds++];
		
		sound->location = *inLocation;
		sound->radius = inVolume;
		sound->time = ONrGameState_GetGameTime();
		sound->type = inImportance;
	}
#endif

	if (AI2gDeaf || AI2gEveryonePassive)
		return;

	character_list = ONrGameState_LivingCharacterList_Get();
	character_list_count = ONrGameState_LivingCharacterList_Count();

	if (inTarget1 == NULL) {
		target1_pt = *inLocation;
	} else {
		ONrCharacter_GetPathfindingLocation(inTarget1, &target1_pt);
	}
	if (inTarget2 != NULL) {
		ONrCharacter_GetPathfindingLocation(inTarget2, &target2_pt);
	}

	combat_sound = AI2rKnowledge_IsCombatSound(inImportance);

	for(itr = 0; itr < character_list_count; itr++)
	{
		ONtHearingConstants *hearing;

		character = character_list[itr];

		if (character->charType != ONcChar_AI2)
			continue;

		if ((character->ai2State.flags & AI2cFlag_Passive) || (character->ai2State.flags & AI2cFlag_Deaf))
			continue;

		if (inTarget1 == NULL) {
			// don't ignore events that don't have a character
			ignore_target1 = UUcFalse;
		} else {
			ignore_target1 = AI2iIgnoreEvents(character, inTarget1, combat_sound);
		}
		if (inTarget2 == NULL) {
			// this event does not have a target character
			ignore_target2 = UUcTrue;
		} else {
			ignore_target2 = AI2iIgnoreEvents(character, inTarget2, combat_sound);
		}

		if (ignore_target1 && ignore_target2) {
			continue;
		}

		hearing = &character->characterClass->hearing;
		hearing_dist = inVolume * hearing->acuity;

		// check that we aren't out of hearing range
		distance_sq = MUmVector_GetDistanceSquared(*inLocation, character->location);
		if (distance_sq > UUmSQR(hearing_dist)) {
/*			COrConsole_Printf_Color(UUcTrue, 0xFFFF8080, 0xFF803030, "%s: distance %f > hearing dist %f",
									character->player_name, MUrSqrt(distance_sq), hearing_dist);*/
			continue;
		}

		// if there's line of sight, we can definitely hear
		ONrCharacter_GetEyePosition(character, &char_pt);
		if (!AKrLineOfSight(inLocation, &char_pt)) {
			// try building a path from the location of the sound to us; this path must have
			// a shorter length than hearing_dist.
			distance = hearing_dist;
			error = PHrPathDistance(NULL, inImportance, inLocation, &char_pt, NULL, character->currentPathnode, &distance);
			if (error != UUcError_None) {
/*				COrConsole_Printf_Color(UUcTrue, 0xFFFF8080, 0xFF803030, "%s: no path within hearing dist %f (straightline is %f)",
										character->player_name, hearing_dist, MUrSqrt(distance_sq));*/
				continue;
			} else {
/*				COrConsole_Printf_Color(UUcTrue, 0xFFFF8080, 0xFF803030, "%s: found path, dist %f within hearing dist %f (straightline is %f)",
										character->player_name, distance, hearing_dist, MUrSqrt(distance_sq));*/
			}
		} else {
/*			COrConsole_Printf_Color(UUcTrue, 0xFFFF8080, 0xFF803030, "%s: direct LOS to sound", character->player_name);*/
		}

		// important sounds that are close by get upgraded to definite... this lets us see invisible characters if they
		// start attacking people. this is only true for the character that caused the sound, NOT the character that is
		// the target of the sound.
		if ((inImportance >= AI2cContactType_Sound_Melee) &&
			(MUrPoint_Distance_Squared(&char_pt, inLocation) < UUmSQR(AI2cKnowledge_Sound_DefiniteContactDist))) {
			contact_strength = AI2cContactStrength_Definite;
		} else {
			contact_strength = AI2cContactStrength_Strong;
		}

		// NB: although the sound was generated from some location, if it concerned a particular character then we store the
		// causing characters' location **NOT** the location of the sound. This is
		// so things like warning shouts work.
		if (!ignore_target1) {
			AI2iKnowledge_PostContact(character, inImportance, contact_strength, (inTarget1 == NULL) ? inLocation : &target1_pt,
										inTarget1, (UUtUns32) inTarget2, UUcFalse, UUcFalse);
		}
		if (!ignore_target2) {
			UUmAssert(inTarget2 != NULL);
			AI2iKnowledge_PostContact(character, inImportance, AI2cContactStrength_Strong, &target2_pt, inTarget2, (UUtUns32) inTarget1, UUcFalse, UUcFalse);
		}
	}
}

void AI2rKnowledge_CreateMagicAwareness(ONtCharacter *ioCharacter, ONtCharacter *inTarget,
										UUtBool inDisableNotify, UUtBool inForceTarget)
{
	AI2iKnowledge_PostContact(ioCharacter, AI2cContactType_Sight_Definite, AI2cContactStrength_Definite,
								&inTarget->actual_position, inTarget, 0, inDisableNotify, inForceTarget);
}

void AI2rKnowledge_AlarmTripped(UUtUns8 inAlarmID, ONtCharacter *inCause)
{
	ONtCharacter **character_list, *character;
	M3tPoint3D cause_pt;
	UUtUns32 character_list_count;
	UUtUns32 itr;

	if (AI2gEveryonePassive)
		return;

	character_list = ONrGameState_LivingCharacterList_Get();
	character_list_count = ONrGameState_LivingCharacterList_Count();

	ONrCharacter_GetPathfindingLocation(inCause, &cause_pt);

	for(itr = 0; itr < character_list_count; itr++)
	{
		character = character_list[itr];

		if (character->charType != ONcChar_AI2)
			continue;

		if (character->ai2State.flags & AI2cFlag_Passive)
			continue;

		if (!AI2iKnowledge_RespondToAlarm(character, inAlarmID))
			continue;

		if (AI2iIgnoreEvents(character, inCause, UUcFalse))
			continue;

		// tell the character about the alarm
		AI2iKnowledge_PostContact(character, AI2cContactType_Alarm, AI2cContactStrength_Strong,
									&cause_pt, inCause, (UUtUns32) inAlarmID, UUcFalse, UUcFalse);
		character->ai2State.flags |= AI2cFlag_RunAlarmScript;
	}
}

UUtBool AI2rKnowledge_IsCombatSound(AI2tContactType inType)
{
	switch(inType) {
		case AI2cContactType_Sound_Danger:
		case AI2cContactType_Sound_Melee:
		case AI2cContactType_Sound_Gunshot:
			return UUcTrue;

		default:
			return UUcFalse;
	}
}

UUtBool AI2rKnowledge_IsHurtEvent(AI2tContactType inType)
{
	switch(inType) {
		case AI2cContactType_Hit_Weapon:
		case AI2cContactType_Hit_Melee:
			return UUcTrue;

		default:
			return UUcFalse;
	}
}

void AI2rKnowledge_ThisCharacterWasDeleted(ONtCharacter *inCharacter)
{
	UUtUns32 itr = 0;
	AI2tKnowledgePending *pending;

	for (itr = 0, pending = AI2gKnowledge_Pending; itr < AI2gKnowledge_NumPending; ) {
		if ((pending->enemy == inCharacter) || (pending->owner == inCharacter)) {
			// remove this contact and copy the last pending contact over it
			if (itr < AI2gKnowledge_NumPending - 1) {
				*pending = AI2gKnowledge_Pending[AI2gKnowledge_NumPending - 1];
			}
			AI2gKnowledge_NumPending--;
			continue;
		}

		// this contact remains valid
		itr++;
		pending++;
	}
}

UUtBool AI2rKnowledge_CharacterCanBeSeen(ONtCharacter *ioCharacter)
{
	if ((ioCharacter->inventory.invisibilityRemaining > 0) &&
		(ioCharacter->inventory.numInvisibleFrames >= AI2cKnowledge_InvisibleFrames)) {
		// this character has been invisible for sufficiently long, we cannot see them
		return UUcFalse;
	} else {
		return UUcTrue;
	}
}

// ------------------------------------------------------------------------------------
// -- internal routines

// determine whether a given AI is part of an alarm group
static UUtBool AI2iKnowledge_RespondToAlarm(ONtCharacter *inCharacter, UUtUns8 inAlarmID)
{
	UUtUns32		alarm_mask;
	if (inAlarmID >= 32)
		return UUcFalse;
	alarm_mask	= 1 << inAlarmID;
	return ((inCharacter->ai2State.alarmGroups & alarm_mask) != 0);
}

// check to see if we already know about a target
static UUtBool AI2iKnowledge_IsVisible(ONtCharacter *inCharacter, ONtCharacter *inTarget)
{
	AI2tKnowledgeState *knowledge_state = &inCharacter->ai2State.knowledgeState;
	AI2tKnowledgeEntry *entry;

	UUmAssert(inCharacter->charType == ONcChar_AI2);
	for (entry = knowledge_state->contacts; entry != NULL; entry = entry->nextptr) {
		if (entry->enemy == inTarget) {
			return (entry->strength >= AI2cContactStrength_Definite);
		} else {
			// keep looking - but since contacts are sorted in order of strength,
			// if we haven't found the target by the time we find a non-definite, stop
			if (entry->strength < AI2cContactStrength_Definite) {
				break;
			}
		}
	}

	return UUcFalse;
}

// check to see which characters are visible by an AI
static void	AI2iKnowledge_CheckVisible(ONtCharacter *inCharacter, ONtActiveCharacter *inActiveCharacter,
									   UUtUns32 inCheckCount, ONtCharacter **inCheckList)
{
	UUtUns16 team_index;
	UUtUns32 itr;
	float max_dist, max_periph_dist, max_dist_sq, vertical_range, central_range, central_max, periph_range, periph_max;
	float dist_sq, dist, periph_dist, central_dist, sintheta, costheta, aimrel_along, aimrel_z;
	ONtCharacter *target;
	AKtEnvironment *env;
	AI2tContactType contact_type;
	AI2tContactStrength contact_strength;
	M3tPoint3D look_point, target_point, target_dir, aim_dir, aim_up, aim_right;
	UUtBool is_visible;
#if AI_VERBOSE_VISIBILITY
	UUtBool print_status;
#endif

	env = ONrGameState_GetEnvironment();
	team_index = inCharacter->teamNumber;

	ONrCharacter_GetEyePosition(inCharacter, &look_point);

	// get the AI's looking vector
	UUmAssert(inCharacter->charType == ONcChar_AI2);

	if ((inActiveCharacter != NULL) && (inActiveCharacter->isAiming)) {
		MUmVector_Copy(aim_dir, inActiveCharacter->aimingVector);

		// right (X) vector is defined by global up vector +Y and the forward (Y) direction
		// cross product of a vector (vx, vy, vz) with (0, 1, 0) is (-vz, 0, vx)
		MUmVector_Set(aim_right, -aim_dir.z, 0, aim_dir.x);

		// we now have two new vectors which allows us to put together an orthonormal basis
		MUrVector_CrossProduct(&aim_right, &aim_dir, &aim_up);
		MUmVector_Normalize(aim_up);
	} else {
		sintheta = MUrSin(inCharacter->facing);
		costheta = MUrCos(inCharacter->facing);

		MUmVector_Set(aim_dir, sintheta, 0, costheta);
		MUmVector_Set(aim_up, 0, 1, 0);
	}

	{
		const ONtVisionConstants *vision = &inCharacter->characterClass->vision;

		max_dist = vision->central_distance;
		max_periph_dist = vision->periph_distance;

		UUmAssert(max_dist >= max_periph_dist);
		max_dist_sq = UUmSQR(max_dist);

		vertical_range	= vision->vertical_range;
		central_range	= vision->central_range;
		central_max		= vision->central_max;
		periph_range	= vision->periph_range;
		periph_max		= vision->periph_max;
	}

	// loop over all targets hostile to this AI
	for (itr = 0; itr < inCheckCount; itr++) {
		target = inCheckList[itr];

		if (!AI2rKnowledge_CharacterCanBeSeen(target))
			continue;

		if (AI2iIgnoreEvents(inCharacter, target, UUcFalse))
			continue;

		// check to see if we already know about this target
		is_visible = AI2iKnowledge_IsVisible(inCharacter, target);

#if AI_VERBOSE_VISIBILITY
//		print_status = (strcmp(inCharacter->player_name, "thug") == 0) && (target == ONrGameState_GetPlayerCharacter());
		print_status = UUcFalse;

		if (print_status)
			COrConsole_Printf("%s: checking if %s is visible", inCharacter->player_name, target->player_name);
#endif

		UUmAssert(target != inCharacter);

		MUmVector_Copy(target_point, target->location);
		target_point.y += target->heightThisFrame;

		MUmVector_Subtract(target_dir, target_point, look_point);

		// is the target out of range?
		dist_sq = MUmVector_GetLengthSquared(target_dir);
		if (dist_sq > max_dist_sq) {
#if AI_VERBOSE_VISIBILITY
			if (print_status)
				COrConsole_Printf("  dist_sq %f > max %f", dist_sq, max_dist_sq);
#endif

			continue;
		}

		dist = MUrSqrt(dist_sq);

		// unless they're very close to us (in which case we can see all the way up and down)
		// then they may be outside our vertical range.
		if (dist > AI2cVisibility_VerticalThresholdDist) {
			// what is the vertical component of the target's direction relative
			// to our head orientation?
			aimrel_z = (float)fabs(MUrVector_DotProduct(&target_dir, &aim_up) / dist);
			if (aimrel_z > vertical_range) {
				// too far up or down, we can't see them
#if AI_VERBOSE_VISIBILITY
				if (print_status)
					COrConsole_Printf("  aimrel_z %f > verticalrange %f (angle is %f)",
										aimrel_z, vertical_range, M3cRadToDeg * MUrACos(aimrel_z));
#endif
				continue;
			}
		}

		// calculate the direction's component along our aiming direction
		aimrel_along = MUrVector_DotProduct(&target_dir, &aim_dir) / dist;

		if (is_visible) {
			if (aimrel_along < periph_max) {
				// the target is outside our peripheral vision!
				continue;

			} else if (dist > max_dist) {
				// the target is outside our visual range.
				continue;

			} else {
				// we can see the target definitely, even if they're only in our
				// peripheral cone.
				contact_type = AI2cContactType_Sight_Definite;
				contact_strength = AI2cContactStrength_Definite;
			}
		} else {
		
			// what viewing distance and centrality does this translate to?
			if (aimrel_along < periph_max) {
				// not visible
#if AI_VERBOSE_VISIBILITY
				if (print_status)
					COrConsole_Printf("  aimrel_along %f < periph_max %f (angle is %f)",
										aimrel_along, periph_max, M3cRadToDeg * MUrACos(aimrel_along));
#endif
				continue;
			} else if (aimrel_along < periph_range) {
				// visible if they're standing close - radius varies out to max over an angle
				periph_dist = max_periph_dist * (aimrel_along - periph_max) / (periph_range - periph_max);
				central_dist = 0;

#if AI_VERBOSE_VISIBILITY
				if (print_status)
					COrConsole_Printf("  periph_max %f < aimrel_along %f < periph_range %f (angle is %f) -> periph_dist %f",
										periph_max, aimrel_along, periph_range, M3cRadToDeg * MUrACos(aimrel_along), periph_dist);
#endif
			} else if (aimrel_along < central_max) {
				// visible peripherally all the way out to max
				periph_dist = max_periph_dist;
				central_dist = 0;

#if AI_VERBOSE_VISIBILITY
				if (print_status)
					COrConsole_Printf("  periph_range %f < aimrel_along %f < central_max %f (angle is %f)",
										periph_range, aimrel_along, central_max, M3cRadToDeg * MUrACos(aimrel_along));
#endif
			} else if (aimrel_along < central_range) {
				// visible centrally if they're standing close - threshold radius varies out to max
				periph_dist = max_periph_dist;
				central_dist = max_dist * (aimrel_along - central_max) / (central_range - central_max);

#if AI_VERBOSE_VISIBILITY
				if (print_status)
					COrConsole_Printf("  central_max %f < aimrel_along %f < central_range %f (angle is %f) -> central_dist %f",
										central_max, aimrel_along, central_range, M3cRadToDeg * MUrACos(aimrel_along), central_dist);
#endif
			} else {
				// visible centrally all the way out to max
				periph_dist = max_periph_dist;
				central_dist = max_dist;

#if AI_VERBOSE_VISIBILITY
				if (print_status)
					COrConsole_Printf("  central_range %f < aimrel_along %f (angle is %f)",
										central_range, aimrel_along, M3cRadToDeg * MUrACos(aimrel_along));
#endif
			}

			if (dist < central_dist) {
				// centrally visible
#if AI_VERBOSE_VISIBILITY
				if (print_status)
					COrConsole_Printf("  dist %f < central_dist %f (angle is %f)",
										dist, central_dist, M3cRadToDeg * MUrACos(aimrel_along));
#endif
				contact_type = AI2cContactType_Sight_Definite;
				contact_strength = AI2cContactStrength_Definite;

			} else if (dist < periph_dist) {
				// peripherally visible
#if AI_VERBOSE_VISIBILITY
				if (print_status)
					COrConsole_Printf("  dist %f < periph_dist %f (angle is %f)",
										dist, periph_dist, M3cRadToDeg * MUrACos(aimrel_along));
#endif
				contact_type = AI2cContactType_Sight_Peripheral;
				contact_strength = AI2cContactStrength_Weak;

			} else {
				// not visible
				continue;
			}
		}

		// we couldn't reject the target based on visibility cone. perform line of sight.
		// note: don't use AKrLineOfSight because it checks against characters

		is_visible = UUcFalse;

		// look at pelvis
		ONrCharacter_GetPelvisPosition(target, &target_point);
		MUmVector_Subtract(target_dir, target_point, look_point);
		if (!AKrCollision_Point(env, &look_point, &target_dir, AKcGQ_Flag_LOS_CanSee_Skip_AI, 0)) {
			is_visible = UUcTrue;

		} else {
			// look at head
			ONrCharacter_GetEyePosition(target, &target_point);
			MUmVector_Subtract(target_dir, target_point, look_point);

			if (!AKrCollision_Point(env, &look_point, &target_dir, AKcGQ_Flag_LOS_CanSee_Skip_AI, 0)) {
				is_visible = UUcTrue;

			} else {
				// look at feet
				ONrCharacter_GetFeetPosition(target, &target_point);
				MUmVector_Subtract(target_dir, target_point, look_point);

				if (!AKrCollision_Point(env, &look_point, &target_dir, AKcGQ_Flag_LOS_CanSee_Skip_AI, 0)) {
					is_visible = UUcTrue;
				}
			}
		}

		if (is_visible) {
#if AI_VERBOSE_VISIBILITY
			if (print_status)
				COrConsole_Printf("  visible %s!", (contact_type == AI2cContactType_Sight_Definite) ? "centrally" : "peripherally");
#endif
			// we can see the character!
			AI2iKnowledge_PostContact(inCharacter, contact_type, contact_strength, &target_point, target, 0, UUcFalse, UUcFalse);
		}
	}
}

// new knowledge has been received
static void AI2iKnowledge_PostContact(ONtCharacter *inCharacter, AI2tContactType inType,
									 AI2tContactStrength inStrength, M3tPoint3D *inLocation,
									 ONtCharacter *inTarget, UUtUns32 inAIUserData, 
									 UUtBool inDisableNotify, UUtBool inForceTarget)
{
	AI2tKnowledgePending *pending;
	UUtUns32 itr;

	if (AI2gKnowledge_ImmediatePost) {
		// add this contact immediately
		AI2iKnowledge_AddContact(inCharacter, inType, inStrength, inLocation, inTarget,
								inAIUserData, inDisableNotify, inForceTarget);
		return;
	}

	// look for an entry in the pending array; try to overwrite one that already exists for
	// this owner / target pair, if we can
	for (itr = 0, pending = AI2gKnowledge_Pending; itr < AI2gKnowledge_NumPending; itr++, pending++) {
		if ((inTarget == NULL) || (pending->owner != inCharacter) || (pending->enemy != inTarget)) {
			// we can't overwrite this entry
			continue;
		}

		if ((pending->strength > inStrength) && (inType < AI2cContactType_Hit_Weapon)) {
			// we already have pending knowledge which is more important than this knowledge
			return;
		} else if (pending->type < AI2cContactType_Hit_Weapon) {
			// overwrite this knowledge with our new entry
			break;
		}
	}

	if (itr >= AI2gKnowledge_NumPending) {
		// instead of overwriting an existing entry, we are creating a new one.
		if (AI2gKnowledge_NumPending >= AI2cKnowledge_MaxPending) {
			// there are no free entries in the pending knowledge database
			AI2_ERROR(AI2cBug, AI2cSubsystem_Knowledge, AI2cError_Knowledge_MaxPending, inCharacter,
						AI2gKnowledge_NumPending, 0, 0, 0);
			return;
		} else {
			// expand the array.
			AI2gKnowledge_NumPending++;
		}
	}

	pending->owner = inCharacter;
	pending->type = inType;
	pending->strength = inStrength;
	pending->location = *inLocation;
	pending->enemy = inTarget;
	pending->user_data = inAIUserData;
	pending->disable_notify = inDisableNotify;
	pending->force_target = inForceTarget;
}

// add all pending contacts
static void AI2iKnowledge_AddPending(void)
{
	UUtUns32 itr;
	AI2tKnowledgePending *pending;

	for (itr = 0, pending = AI2gKnowledge_Pending; itr < AI2gKnowledge_NumPending; itr++, pending++) {
		if (pending->enemy != NULL) {
			UUmAssert(pending->enemy->flags & ONcCharacterFlag_InUse);
		}
		AI2iKnowledge_AddContact(pending->owner, pending->type, pending->strength, &pending->location,
								pending->enemy, pending->user_data, pending->disable_notify, pending->force_target);
	}

	AI2gKnowledge_NumPending = 0;
}

// add data to our contact list
static void AI2iKnowledge_AddContact(ONtCharacter *inCharacter, AI2tContactType inType,
									 AI2tContactStrength inStrength, M3tPoint3D *inLocation,
									 ONtCharacter *inTarget, UUtUns32 inAIUserData,
									 UUtBool inDisableNotify, UUtBool inForceTarget)
{
	AI2tKnowledgeState *knowledge_state;
	AI2tKnowledgeEntry *entry, *checkentry;
	UUtUns32 current_time;
	UUtBool hostile_towards_friend;

	if (((inCharacter->flags & ONcCharacterFlag_InUse) == 0) || (inCharacter->charType != ONcChar_AI2) ||
		((inCharacter->ai2State.flags & AI2cFlag_InUse) == 0)) {
		// we can't add this contact (in fact it should have never been entered
		// into the pending list, or it should have been erased when the character died / was deleted)
		UUmAssert(!"AI2iKnowledge_AddContact: tried to add contact to invalid knowledgestate");
		return;
	}

	current_time = ONrGameState_GetGameTime();

	// do we already know about this character?
	knowledge_state = &inCharacter->ai2State.knowledgeState;
	entry = AI2rKnowledge_FindEntry(inCharacter, knowledge_state, inTarget);
	if (entry == NULL) {
		entry = AI2iKnowledge_NewEntry(inCharacter, knowledge_state);
		if (entry == NULL) {
			// we can't respond to this
			return;
		}
		
		entry->owner = inCharacter;
		entry->enemy = inTarget;
		
		// by default all contacts are just interesting information, not a threat
		entry->priority = AI2cContactPriority_Friendly;
		if (entry->enemy != NULL) {
			// might they start shooting back at us?
			if (AI2rCharacter_PotentiallyHostile(inTarget, inCharacter, UUcFalse)) {
				entry->priority = AI2cContactPriority_Hostile_Threat;
			} else if (AI2rCharacter_PotentiallyHostile(inCharacter, inTarget, UUcFalse)) {
				entry->priority = AI2cContactPriority_Hostile_NoThreat;
			}
		}
		
		entry->first_time = current_time;
		entry->highest_strength = AI2cContactStrength_Forgotten;
		entry->has_hurt_me = UUcFalse;
		entry->has_hurt_me_in_melee = UUcFalse;
		entry->has_been_hostile = UUcFalse;
		entry->degrade_hostility_time = (UUtUns32) -1;
		entry->total_damage = 0;
		entry->ignored_events = 0;
		entry->last_time = 0;
		entry->last_notify_time = 0;
		entry->last_type = AI2cContactType_Sound_Ignore;
		entry->last_strength = AI2cContactStrength_Forgotten;
	}
	
	if (inForceTarget) {
		entry->priority = AI2cContactPriority_ForceTarget;
	}

	if ((entry->last_type == AI2cContactType_Sound_Ignore) && (inType == AI2cContactType_Sound_Ignore)) {
		// multiple ignore sounds can get promoted to an 'interesting' sound, potentially
		if (current_time - entry->last_time < (UUtUns32) AI2gSound_StopIgnoring_Time) {
			entry->ignored_events++;

			if (entry->ignored_events >= (UUtUns32) AI2gSound_StopIgnoring_Events) {
				inType = AI2cContactType_Sound_Interesting;
			}
		} else {
			entry->ignored_events = 0;
		}
	} else {
		entry->ignored_events = 0;
	}

	entry->last_time = current_time;
	entry->last_location = *inLocation;
	entry->last_type = inType;
	entry->last_user_data = inAIUserData;
	
	// update the strength of our contact with this character
	if (inStrength > entry->highest_strength) {
		entry->highest_strength = inStrength;
		entry->first_sighting = UUcTrue;
	} else {
		entry->first_sighting = UUcFalse;
	}
	
	if (inStrength >= entry->strength) {
		entry->strength = inStrength;
		entry->degrade_time = current_time + AI2iKnowledge_GetDegradeTime(entry->owner, entry->strength, entry->highest_strength);
	}
	
	if ((inType == AI2cContactType_Hit_Weapon) || (inType == AI2cContactType_Hit_Melee)) {
		// user data for hurt events is the amount of damage taken
		entry->has_hurt_me = UUcTrue;
		if (inType == AI2cContactType_Hit_Melee) {
			entry->has_hurt_me_in_melee = UUcTrue;
		}
		entry->total_damage += inAIUserData;

		if (entry->priority == AI2cContactPriority_Friendly) {
			// determine whether this is a hostile action
			if (entry->total_damage > inCharacter->maxHitPoints / 2) {
				entry->has_been_hostile = UUcTrue;
				entry->degrade_hostility_time = entry->last_time + AI2cKnowledge_HostilityPersistTime;
			} else {
				// look to see if we are already in a dangerous situation; if so, don't take offense
				for (checkentry = knowledge_state->contacts; checkentry != NULL; checkentry = checkentry->nextptr) {
					if ((checkentry->strength >= AI2cContactStrength_Strong) && (checkentry->priority == AI2cContactPriority_Hostile_Threat) &&
						(MUmVector_GetDistanceSquared(checkentry->last_location, inCharacter->location) < UUmSQR(inCharacter->ai2State.neutralEnemyRange))) {
						break;
					}
				}
				if (checkentry == NULL) {
					entry->has_been_hostile = UUcTrue;
					entry->degrade_hostility_time = entry->last_time + AI2cKnowledge_HostilityPersistTime;
				} else {
					// FIXME: say 'ow' or 'watch your fire'.
				}
			}
			if (entry->has_been_hostile) {
				// this character has been hostile towards us
				entry->priority = AI2cContactPriority_Hostile_Threat;
			}
		} else {
			// this is a threatening action, upgrade our priority if we have to
			entry->priority = UUmMin(entry->priority, AI2cContactPriority_Hostile_Threat);
			entry->has_been_hostile = UUcTrue;
			entry->degrade_hostility_time = (UUtUns32) -1;
		}

	} else if ((inType == AI2cContactType_Sound_Melee) || (inType == AI2cContactType_Sound_Gunshot)) {
		// determine whether someone is attacking or shooting a friend of ours
		ONtCharacter *target_character = (ONtCharacter *) inAIUserData;
		
		if ((entry->enemy != NULL) && (target_character != NULL)) {
			UUtUns32 enemy_status = AI2rTeam_FriendOrFoe(inCharacter->teamNumber, entry->enemy->teamNumber);
			UUtUns32 target_status = AI2rTeam_FriendOrFoe(inCharacter->teamNumber, target_character->teamNumber);

			if ((entry->priority == AI2cContactPriority_Friendly) && (entry->strength == AI2cContactStrength_Definite) && 
				(enemy_status == AI2cTeam_Neutral) && (target_status == AI2cTeam_Friend)) {
				// a character that we are neutral towards is attacking a friend of ours, and we saw it. if our friend considers
				// this a hostile attack, so will we.
				checkentry = NULL;
				if (target_character->charType == ONcChar_AI2) {
					checkentry = AI2rKnowledge_FindEntry(target_character, &target_character->ai2State.knowledgeState, entry->enemy);
					if (checkentry == NULL) {
						// our friend has no idea about them
						hostile_towards_friend = UUcFalse;
					} else {
						hostile_towards_friend = ((checkentry->has_been_hostile) || (checkentry->priority >= AI2cContactPriority_Hostile_Threat));
					}
				} else {
					// our friend is not an AI, they presumably will always be hostile towards people that punch them or
					// shoot at them
					hostile_towards_friend = UUcTrue;
				}

				if (hostile_towards_friend) {
					// this is a threatening action towards our friend, but not yet towards us
					entry->priority = AI2cContactPriority_Hostile_NoThreat;
					entry->has_been_hostile = UUcTrue;
					entry->degrade_hostility_time = entry->last_time + AI2cKnowledge_HostilityPersistTime;
					if ((checkentry != NULL) && (checkentry->has_been_hostile)) {
						entry->degrade_hostility_time = UUmMin(entry->degrade_hostility_time, checkentry->degrade_hostility_time);
					}
				}
			}
		}
	}

	// sort this entry to where it belongs in the contact list
	AI2iKnowledge_MoveContactUp(inCharacter, knowledge_state, entry);
	
	entry->last_notify_time = current_time;
	if (!inDisableNotify && (knowledge_state->callback != NULL)) {
		(knowledge_state->callback)(inCharacter, entry, UUcFalse);
	}

	entry->last_strength = entry->strength;

	AI2rKnowledge_Verify();
}

// find an existing contact in a list
AI2tKnowledgeEntry *AI2rKnowledge_FindEntry(ONtCharacter *inOwner, AI2tKnowledgeState *ioKnowledgeState,
													ONtCharacter *inContact)
{
	AI2tKnowledgeEntry *entry;

	for (entry = ioKnowledgeState->contacts; entry != NULL; entry = entry->nextptr) {
		UUmAssert(entry->owner == inOwner);
		if (entry->enemy == inContact)
			break;
	}

	return entry;
}

// create a new entry in a character's contact list
static AI2tKnowledgeEntry *AI2iKnowledge_NewEntry(ONtCharacter *inOwner, AI2tKnowledgeState *ioKnowledgeState)
{
	AI2tKnowledgeEntry *entry;
	UUtUns32 itr;

	if (AI2gKnowledge_NextFreeEntry == NULL) {
		if (AI2gKnowledge_ClearSpaceIndex < AI2cKnowledge_MaxEntries) {
			// there are empty spaces at the end of the knowledge array
			AI2gKnowledge_NextFreeEntry = AI2gKnowledgeBase + AI2gKnowledge_ClearSpaceIndex++;

		} else {
			// clean up looking for purgeable entries
			for (itr = 0, entry = AI2gKnowledgeBase; itr < AI2cKnowledge_MaxEntries; itr++, entry++) {
				UUmAssert(entry->owner != NULL);

				if (entry->strength <= AI2cContactStrength_Forgotten) {
					// purge this entry from our knowledge base
					AI2iKnowledge_RemoveEntry(entry, UUcTrue);

				} else if (((entry->owner->flags & ONcCharacterFlag_InUse) == 0) ||
							(entry->owner->flags & ONcCharacterFlag_Dead)) {
					// owner AI has died, kill this entry
					AI2iKnowledge_RemoveEntry(entry, UUcFalse);
				}
			}

			// FIXME: we could consider growing the knowledgebase here
			if (AI2gKnowledge_NextFreeEntry == NULL) {
				AI2_ERROR(AI2cBug, AI2cSubsystem_Knowledge, AI2cError_Knowledge_MaxEntries, inOwner,
							AI2cKnowledge_MaxEntries, 0, 0, 0);
				return NULL;
			}
		}
	}

	entry = AI2gKnowledge_NextFreeEntry;
	
#if defined(DEBUGGING) && DEBUGGING
	UUmAssertReadPtr(entry, sizeof(AI2tKnowledgeEntry));
	UUmAssert(entry >= AI2gKnowledgeBase);
	UUmAssert(entry - AI2gKnowledgeBase < AI2cKnowledge_MaxEntries);
	UUmAssert(entry->owner == NULL);
#endif

	// maintain the freelist
	AI2gKnowledge_NextFreeEntry = entry->nextptr;

#if defined(DEBUGGING) && DEBUGGING
	if (AI2gKnowledge_NextFreeEntry != NULL) {
		UUmAssertReadPtr(AI2gKnowledge_NextFreeEntry, sizeof(AI2tKnowledgeEntry));
		UUmAssert(AI2gKnowledge_NextFreeEntry >= AI2gKnowledgeBase);
		UUmAssert(AI2gKnowledge_NextFreeEntry - AI2gKnowledgeBase < AI2cKnowledge_MaxEntries);
		UUmAssert(AI2gKnowledge_NextFreeEntry->owner == NULL);
	}
#endif

	entry->nextptr = ioKnowledgeState->contacts;
	if (entry->nextptr != NULL) {
		entry->nextptr->prevptr = entry;
	}

	entry->prevptr = NULL;
	ioKnowledgeState->contacts = entry;

	ioKnowledgeState->num_contacts++;

	return entry;
}

// delete a knowledge entry
static void AI2iKnowledge_RemoveEntry(AI2tKnowledgeEntry *ioEntry, UUtBool inRemoveReferences)
{
	AI2tKnowledgeState *knowledge_state;

	UUmAssertReadPtr(ioEntry, sizeof(AI2tKnowledgeEntry));
	UUmAssert(ioEntry->owner != NULL);
	
	if ((ioEntry->owner->flags & ONcCharacterFlag_InUse) &&
		(ioEntry->owner->charType == ONcChar_AI2)) {

		if (inRemoveReferences) {
			ONtCharacter *character = ioEntry->owner;

			// depending on the AI's state, make sure that this entry isn't being referenced elsewhere
			switch(character->ai2State.currentGoal) {
				case AI2cGoal_Combat:
					AI2rCombat_NotifyKnowledgeRemoved(character, &character->ai2State.currentState->state.combat, ioEntry);
				break;

				case AI2cGoal_Pursuit:
					AI2rPursuit_NotifyKnowledgeRemoved(character, &character->ai2State.currentState->state.pursuit, ioEntry);
				break;

				case AI2cGoal_Panic:
					AI2rPanic_NotifyKnowledgeRemoved(character, &character->ai2State.currentState->state.panic, ioEntry);
				break;
			}
		}

		// this entry is no longer a viable contact for the AI
		knowledge_state = &ioEntry->owner->ai2State.knowledgeState;
		UUmAssert(knowledge_state->num_contacts > 0);
		knowledge_state->num_contacts--;

		if (knowledge_state->contacts == ioEntry)
			knowledge_state->contacts = ioEntry->nextptr;
	}

	// patch up the linked contact list (even if it's no longer connected
	// to an AI)
	if (ioEntry->prevptr != NULL) {
		ioEntry->prevptr->nextptr = ioEntry->nextptr;
	}
	if (ioEntry->nextptr != NULL) {
		ioEntry->nextptr->prevptr = ioEntry->prevptr;
	}

	// add the entry to the freelist
#if defined(DEBUGGING) && DEBUGGING
	UUrMemory_Set8((void *) ioEntry, 0xdd, sizeof(AI2tKnowledgeEntry));
#endif
	ioEntry->owner = NULL;
	ioEntry->nextptr = AI2gKnowledge_NextFreeEntry;
	AI2gKnowledge_NextFreeEntry = ioEntry;
}

void AI2rKnowledge_Delete(ONtCharacter *ioCharacter)
{
	AI2tKnowledgeState *knowledge_state = &ioCharacter->ai2State.knowledgeState;
	AI2tKnowledgeEntry *entry, *next_entry;
	AI2tKnowledgePending *pending;
	UUtUns32 itr = 0;

	if (!AI2gKnowledge_Valid) {
		// this is part of the gamestate unload process
//		COrConsole_Printf("TEMPKNOWLEDGEDEBUG: gamestate unload, don't delete");
		return;
	}

	for (entry = knowledge_state->contacts; entry != NULL; entry = next_entry) {
		next_entry = entry->nextptr;

//		COrConsole_Printf("TEMPKNOWLEDGEDEBUG: killing entry");
		AI2iKnowledge_RemoveEntry(entry, UUcFalse);
	}

	// in deleting all of these entries we must have removed the entire list
	UUmAssert(knowledge_state->num_contacts == 0);
	UUmAssert(knowledge_state->contacts == NULL);
//	COrConsole_Printf("TEMPKNOWLEDGEDEBUG: knowledge deletion done");

	// remove any pending knowledge contacts for this character
	for (itr = 0, pending = AI2gKnowledge_Pending; itr < AI2gKnowledge_NumPending; ) {
		if (pending->owner == ioCharacter) {
			// remove this contact and copy the last pending contact over it
			if (itr < AI2gKnowledge_NumPending - 1) {
				*pending = AI2gKnowledge_Pending[AI2gKnowledge_NumPending - 1];
			}
			AI2gKnowledge_NumPending--;
			continue;
		}

		// this contact remains valid
		itr++;
		pending++;
	}

	AI2rKnowledge_Verify();
}

// compare two entries to see which has higher prosecution priority. returns -1 if inEntry1, +1 if inEntry2, 0 if equal.
UUtInt32 AI2rKnowledge_CompareFunc(AI2tKnowledgeEntry *inEntry1, AI2tKnowledgeEntry *inEntry2, UUtBool inSticky, ONtCharacter *inHostileCheck)
{
	AI2tContactStrength ignorable_strength;
	UUtBool hostile1, hostile2;
	UUtInt32 boss_preference = 0;

	if (AI2gBossBattle && (inHostileCheck != NULL)) {
		UUtBool us_boss, target1_boss, target2_boss;
		ONtCharacter *target1, *target2;

		// boss characters prefer to go after other boss characters and non-bosses prefer to go after non-bosses
		target1 = inEntry1->enemy;
		target2 = inEntry2->enemy;
		if ((target1 != NULL) && (target2 != NULL)) {
			us_boss = ((inHostileCheck->flags & ONcCharacterFlag_Boss) > 0);
			target1_boss = ((target1->flags & ONcCharacterFlag_Boss) > 0);
			target2_boss = ((target2->flags & ONcCharacterFlag_Boss) > 0);

			if ((us_boss == target1_boss) && (us_boss != target2_boss)) {
				// we prefer target 1
				boss_preference = -1;

			} else if ((us_boss != target1_boss) && (us_boss == target2_boss)) {
				// we prefer target 2
				boss_preference = +1;
			}
		}
	}

	// if we have a boss preference, only stop pursuing them if the contact drops to forgotten
	ignorable_strength = (boss_preference == 0) ? AI2cContactStrength_Weak : AI2cContactStrength_Forgotten;

	// strong contacts take precedence over weak ones
	if ((inEntry1->strength <= ignorable_strength) && (inEntry2->strength >= AI2cContactStrength_Strong)) {
		return +1;
	} else if ((inEntry2->strength <= ignorable_strength) && (inEntry1->strength >= AI2cContactStrength_Strong)) {
		return -1;
	}

	if (boss_preference != 0) {
		return boss_preference;
	}

	// threats take precedence over non-threats
	if (inEntry1->priority > inEntry2->priority)
		return -1;
	else if (inEntry1->priority < inEntry2->priority)
		return +1;
		
	if (inHostileCheck != NULL) {
		// potentially hostile takes precedence over not potentially hostile
		hostile1 = (inEntry1->enemy != NULL) && AI2rCharacter_PotentiallyHostile(inEntry1->enemy, inHostileCheck, UUcTrue);
		hostile2 = (inEntry2->enemy != NULL) && AI2rCharacter_PotentiallyHostile(inEntry2->enemy, inHostileCheck, UUcTrue);

		if (hostile1 && !hostile2) {
			return -1;
		} else if (!hostile1 && hostile2) {
			return +1;
		}
	}

	// sort by strength
	if (inEntry1->strength > inEntry2->strength)
		return -1;
	else if (inEntry1->strength < inEntry2->strength)
		return +1;

	// sort by has-hurt-me
	if ((inEntry1->has_hurt_me) && (!inEntry2->has_hurt_me)) {
		return -1;
	} else if ((!inEntry1->has_hurt_me) && (inEntry2->has_hurt_me)) {
		return +1;
	}

	// sort by has-hurt-me-in-melee
	if ((inEntry1->has_hurt_me_in_melee) && (!inEntry2->has_hurt_me_in_melee)) {
		return -1;
	} else if ((!inEntry1->has_hurt_me_in_melee) && (inEntry2->has_hurt_me_in_melee)) {
		return +1;
	}

	if (inSticky) {
		// this is a combat test to see if we should change targets... rank equally
		return 0;
	}

	// sort by highest strength
	if (inEntry1->highest_strength > inEntry2->highest_strength)
		return -1;
	else if (inEntry1->highest_strength < inEntry2->highest_strength)
		return +1;

	return 0;
}

// move an updated entry up the contact list if necessary
static void AI2iKnowledge_MoveContactUp(ONtCharacter *inOwner, AI2tKnowledgeState *ioKnowledgeState,
										AI2tKnowledgeEntry *ioEntry)
{
	AI2tKnowledgeEntry *prev_entry;

	while ((prev_entry = ioEntry->prevptr) != NULL) {
		if (AI2rKnowledge_CompareFunc(ioEntry, prev_entry, UUcFalse, inOwner) > 0) {
			// the previous entry has a higher priority. no further changes.
			return;
		}

		// move prev_entry back to behind ioEntry
		if ((ioEntry->prevptr = prev_entry->prevptr) == NULL) {
			ioKnowledgeState->contacts = ioEntry;
		} else {
			ioEntry->prevptr->nextptr = ioEntry;
		}

		if ((prev_entry->nextptr = ioEntry->nextptr) != NULL) {
			prev_entry->nextptr->prevptr = prev_entry;
		}

		ioEntry->nextptr = prev_entry;
		prev_entry->prevptr = ioEntry;
	}
}

// move a degraded entry down the contact list if necessary
static void AI2iKnowledge_MoveContactDown(ONtCharacter *inOwner, AI2tKnowledgeState *ioKnowledgeState,
										AI2tKnowledgeEntry *ioEntry)
{
	AI2tKnowledgeEntry *next_entry;

	while ((next_entry = ioEntry->nextptr) != NULL) {
		if (AI2rKnowledge_CompareFunc(ioEntry, next_entry, UUcFalse, inOwner) < 0) {
			// the next entry has a lower priority. no further changes.
			return;
		}

		// move next_entry up in front of ioEntry
		if ((next_entry->prevptr = ioEntry->prevptr) == NULL) {
			ioKnowledgeState->contacts = next_entry;
		} else {
			next_entry->prevptr->nextptr = next_entry;
		}

		if ((ioEntry->nextptr = next_entry->nextptr) != NULL) {
			ioEntry->nextptr->prevptr = ioEntry;
		}

		ioEntry->prevptr = next_entry;
		next_entry->nextptr = ioEntry;
	}
}

static UUtUns32 AI2iKnowledge_GetDegradeTime(ONtCharacter *inCharacter, AI2tContactStrength inStrength, AI2tContactStrength inHighestStrength)
{
	const ONtMemoryConstants *memory = &inCharacter->characterClass->memory;
	UUtUns32 degrade_time;

	switch(inStrength) 
	{
		case AI2cContactStrength_Dead:
		case AI2cContactStrength_Forgotten:
			degrade_time = 0;
		break;

		case AI2cContactStrength_Weak:
			degrade_time = memory->weak_to_forgotten;
		break;

		case AI2cContactStrength_Strong:
			degrade_time = memory->strong_to_weak;
		break;

		case AI2cContactStrength_Definite:
			degrade_time = memory->definite_to_strong;
		break;

		default:
			UUmAssert(!"AI2iKnowledge_GetDegradeTime: unknown contact strength!");
			degrade_time = 0;
		break;
	}

	return degrade_time;
}

static UUtBool AI2iIgnoreEvents(ONtCharacter *inUpdatingCharacter, ONtCharacter *inCause, UUtBool inCombatOverride)
{
	if (AI2gEveryonePassive || (inUpdatingCharacter->ai2State.flags & AI2cFlag_Passive))
		return UUcTrue;
	
	if (inUpdatingCharacter == inCause)
		return UUcTrue;

	// sounds that come from no character at all get responded to by everyone
	if (inCause == NULL)
		return UUcFalse;

	// ignore dead players (needed because they may be being shot at by a friendly)
	// FIXME: respond to friendly corpses maybe?
	if ((inCause->flags & ONcCharacterFlag_Dead) || ((inCause->flags & ONcCharacterFlag_InUse) == 0))
		return UUcTrue;

	// everyone pays attention to the player, even if they aren't hostile
	if (inCause->charType == ONcChar_Player)
		return ((AI2gIgnorePlayer) || (inUpdatingCharacter->ai2State.flags & AI2cFlag_IgnorePlayer));

	// respond to friendly characters if they're making combat sounds
	if (inCombatOverride)
		return UUcTrue;

	// only ignore all characters that there couldn't be hostility between
	if (AI2rCharacter_PotentiallyHostile(inCause, inUpdatingCharacter, UUcTrue) ||
		AI2rCharacter_PotentiallyHostile(inUpdatingCharacter, inCause, UUcTrue)) {
		return UUcFalse;
	} else {
		return UUcTrue;
	}
}

// ------------------------------------------------------------------------------------
// -- dodge routines

AI2tDodgeProjectile *AI2rKnowledge_NewDodgeProjectile(UUtUns32 *outIndex)
{
	UUtUns32 itr;
	AI2tDodgeProjectile *projectile;

	for (itr = 0; itr < AI2gKnowledge_NumDodgeProjectiles; itr++) {
		if ((AI2gKnowledge_DodgeProjectile[itr].flags & AI2cDodgeProjectileFlag_InUse) == 0) {
			break;
		}
	}
	if (itr >= AI2cKnowledge_MaxDodgeProjectiles) {
		// we cannot create any more dodge projectiles
		AI2_ERROR(AI2cBug, AI2cSubsystem_Knowledge, AI2cError_Knowledge_MaxDodgeProjectiles, NULL,
				AI2cKnowledge_MaxDodgeProjectiles, 0, 0, 0);
		return NULL;
	}

	projectile = &AI2gKnowledge_DodgeProjectile[itr];
	projectile->flags = AI2cDodgeProjectileFlag_InUse;
	if (outIndex != NULL) {
		*outIndex = itr;
	}

	if (itr >= AI2gKnowledge_NumDodgeProjectiles) {
		AI2gKnowledge_NumDodgeProjectiles++;
		UUmAssert(AI2gKnowledge_NumDodgeProjectiles <= AI2cKnowledge_MaxDodgeProjectiles);
	}

	return projectile;
}

AI2tDodgeProjectile *AI2rKnowledge_GetDodgeProjectile(UUtUns32 inIndex)
{
	UUmAssert(AI2gKnowledge_DodgeProjectile[inIndex].flags & AI2cDodgeProjectileFlag_InUse);
	return &AI2gKnowledge_DodgeProjectile[inIndex];
}

static void AI2iKnowledge_PurgeDodgeProjectile(AI2tDodgeProjectile *ioProjectile)
{
	UUmAssert(ioProjectile->flags & AI2cDodgeProjectileFlag_InUse);
	ioProjectile->flags &= ~(AI2cDodgeProjectileFlag_InUse | AI2cDodgeProjectileFlag_Dead);

	if (ioProjectile + 1 == &AI2gKnowledge_DodgeProjectile[AI2gKnowledge_NumDodgeProjectiles]) {
		AI2gKnowledge_NumDodgeProjectiles--;
	}
}

void AI2rKnowledge_DeleteDodgeProjectile(UUtUns32 inIndex)
{
	AI2tDodgeProjectile *projectile;

	UUmAssert((inIndex >= 0) && (inIndex < AI2cKnowledge_MaxDodgeProjectiles));
	projectile = &AI2gKnowledge_DodgeProjectile[inIndex];
	UUmAssert(projectile->flags & AI2cDodgeProjectileFlag_InUse);

	// get the final position of the projectile
	AI2iKnowledge_UpdateDodgeProjectile(projectile);

	// mark the projectile as dead, it won't be deleted until the next
	// pass through AI2iKnowledge_UpdateDodgeProjectiles. this means that
	// we get one last pass through AI2iKnowledge_UpdateProjectileAlertness which
	// could be important for projectiles hitting a wall etc.
	projectile->flags |= AI2cDodgeProjectileFlag_Dead;

	UUmAssert(projectile->particle->header.ai_projectile_index == (UUtUns8) inIndex);
	projectile->particle->header.ai_projectile_index = (UUtUns8) -1;
	projectile->particle = NULL;
	projectile->particle_class = NULL;
	projectile->particle_ref = P3cParticleReference_Null;
}

// set up a newly created dodge projectile
void AI2rKnowledge_SetupDodgeProjectile(AI2tDodgeProjectile *ioProjectile, P3tParticleClass *inClass, P3tParticle *inParticle)
{
	M3tVector3D *velocity;
	P3tOwner *owner;

	// copy information about the particle
	ioProjectile->dodge_radius = inClass->definition->dodge_radius;
	ioProjectile->alert_radius = inClass->definition->alert_radius;
	P3rGetRealPosition(inClass, inParticle, &ioProjectile->initial_position);

	ioProjectile->prev_position = ioProjectile->initial_position;
	ioProjectile->position = ioProjectile->initial_position;
	velocity = P3rGetVelocityPtr(inClass, inParticle);
	if (velocity == NULL) {
		MUmVector_Set(ioProjectile->velocity, 0, 0, 0);
	} else {
		ioProjectile->velocity = *velocity;
	}

	owner = P3rGetOwnerPtr(inClass, inParticle);
	if (owner == NULL) {
		ioProjectile->owner = 0;
	} else {
		ioProjectile->owner = *owner;
	}
	ioProjectile->particle_class = inClass;
	ioProjectile->particle = inParticle;
	ioProjectile->particle_ref = inParticle->header.self_ref;
}

// update a dodge projectile's position from the particle
void AI2iKnowledge_UpdateDodgeProjectile(AI2tDodgeProjectile *ioProjectile)
{
	M3tVector3D *velocity;

	UUmAssert((ioProjectile >= &AI2gKnowledge_DodgeProjectile[0]) && 
			  (ioProjectile < &AI2gKnowledge_DodgeProjectile[AI2gKnowledge_NumDodgeProjectiles]));
	UUmAssert(ioProjectile->flags & AI2cDodgeProjectileFlag_InUse);


	if (ioProjectile->particle->header.self_ref != ioProjectile->particle_ref) {
		UUmAssert(!"AI2iKnowledge_UpdateDodgeProjectile: dodge projectile has broken link");
		ioProjectile->flags &= ~AI2cDodgeProjectileFlag_InUse;
		return;
	}

#if defined(DEBUGGING) && DEBUGGING
	{
		P3tParticleClass *classptr;
		P3tParticle *particleptr;

		P3mUnpackParticleReference(ioProjectile->particle_ref, classptr, particleptr);
		UUmAssert(classptr == ioProjectile->particle_class);
		UUmAssert(particleptr == ioProjectile->particle);
	}
#endif

	// update particle's position
	ioProjectile->prev_position = ioProjectile->position;
	P3rGetRealPosition(ioProjectile->particle_class, ioProjectile->particle, &ioProjectile->position);

	// update particle's velocity
	velocity = P3rGetVelocityPtr(ioProjectile->particle_class, ioProjectile->particle);
	if (velocity != NULL) {
		ioProjectile->velocity = *velocity;
	}
}

// update all dodge projectiles
static void AI2iKnowledge_UpdateDodgeProjectiles(void)
{
	UUtUns32 itr;
	AI2tDodgeProjectile *projectile;

	for (itr = 0, projectile = AI2gKnowledge_DodgeProjectile; itr < AI2gKnowledge_NumDodgeProjectiles; itr++, projectile++) {
		if ((projectile->flags & AI2cDodgeProjectileFlag_InUse) == 0)
			continue;

		if (projectile->flags & AI2cDodgeProjectileFlag_Dead) {
			AI2iKnowledge_PurgeDodgeProjectile(projectile);
			continue;
		}

		AI2iKnowledge_UpdateDodgeProjectile(projectile);
	}
}

// update an AI's dodge table with any nearby projectiles
void AI2rKnowledge_FindNearbyProjectiles(ONtCharacter *ioCharacter, AI2tManeuverState *ioManeuverState)
{
	UUtUns32 itr;
	UUtBool must_dodge;
	AKtEnvironment *environment;
	AI2tDodgeProjectile *projectile;
	M3tPoint3D pelvis_pt, projectile_start, closest_point, cur_dist;
	M3tVector3D projectile_vel, vector_to_character;
	ONtCharacter *owner_character;
	ONtActiveCharacter *active_character;
	float a, b, c, r, disc, t, closest_t, dist_mag;

	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL) {
		return;
	}

	environment = ONrGameState_GetEnvironment();
	ONrCharacter_GetPelvisPosition(ioCharacter, &pelvis_pt);

	for (itr = 0, projectile = AI2gKnowledge_DodgeProjectile; itr < AI2gKnowledge_NumDodgeProjectiles; itr++, projectile++) {
		if ((projectile->flags & AI2cDodgeProjectileFlag_InUse) == 0)
			continue;

		if (WPrOwner_IsCharacter(projectile->owner, &owner_character) && (owner_character == ioCharacter)) {
			// don't try to dodge our own projectiles
			continue;
		}

		must_dodge = UUcFalse;

		if (projectile->dodge_radius > 0) {
			// calculate the projectile's position relative to the character, and its expected velocity over the
			// next AI2cProjectile_DodgeTrajectoryFrames
			MUmVector_Subtract(projectile_start, projectile->position, pelvis_pt);
			MUmVector_ScaleCopy(projectile_vel, ((float) AI2cProjectile_DodgeTrajectoryFrames) / UUcFramesPerSecond, projectile->velocity);
			
			r = active_character->boundingSphere.radius + projectile->dodge_radius;
			a = MUmVector_GetLengthSquared(projectile_vel);
			b = 2 * MUrVector_DotProduct(&projectile_start, &projectile_vel);
			c = MUmVector_GetLengthSquared(projectile_start) - UUmSQR(r);

			if (c <= 0) {
				// we are already inside the projectile's danger sphere!
				closest_t = 0;
				must_dodge = UUcTrue;

			} else if ((fabs(a) > 1e-03f) && (b <= 0)) {
				// the projectile is not stationary, and is heading towards us

				disc = UUmSQR(b) - 4*a*c;
				if (disc > 0) {
					// the projectile will get close enough to us to be considered

					// work out the t-value of the first contact point
					t = (-b + MUrSqrt(disc)) / (2 * a);
					if ((t > 0) && (t < 1)) {
						// the projectile intersects us within the dodge period
						closest_t = -b / (2 * a);
						must_dodge = UUcTrue;
					}
				}
			}

			if (must_dodge) {
				// calculate the closest point along the projectile's trajectory to us... this gives us an indication of
				// which direction we should move in to get away from it
				MUmVector_Copy(closest_point, projectile->position);
				MUmVector_ScaleIncrement(closest_point, closest_t, projectile_vel);

				// we only need to dodge projectiles which aren't hidden by walls
				MUmVector_Subtract(vector_to_character, pelvis_pt, closest_point);
				if (AKrCollision_Point(environment, &closest_point, &vector_to_character, AKcGQ_Flag_LOS_CanShoot_Skip_AI, 0) == 0) {
					MUmVector_Copy(cur_dist, closest_point);
					if (closest_t < 0) {
						MUmVector_ScaleIncrement(cur_dist, -closest_t, projectile_vel);

					} else if (closest_t > 1) {
						MUmVector_ScaleIncrement(cur_dist, 1.0f - closest_t, projectile_vel);
					}
					dist_mag = MUmVector_GetLength(cur_dist);

					// send a notification of this projectile to the maneuver state
					AI2rManeuver_NotifyNearbyProjectile(ioCharacter, ioManeuverState, projectile, &projectile_vel, dist_mag, &closest_point);
				}
			}
		}
	}
}

AI2tDodgeFiringSpread *AI2rKnowledge_NewDodgeFiringSpread(UUtUns32 *outIndex)
{
	UUtUns32 itr;
	AI2tDodgeFiringSpread *spread;

	for (itr = 0; itr < AI2gKnowledge_NumDodgeFiringSpreads; itr++) {
		if ((AI2gKnowledge_DodgeFiringSpread[itr].flags & AI2cDodgeFiringSpreadFlag_InUse) == 0) {
			break;
		}
	}
	if (itr >= AI2cKnowledge_MaxDodgeFiringSpreads) {
		// we cannot create any more firing spreads
		AI2_ERROR(AI2cBug, AI2cSubsystem_Knowledge, AI2cError_Knowledge_MaxDodgeFiringSpreads, NULL,
				AI2cKnowledge_MaxDodgeFiringSpreads, 0, 0, 0);
		return NULL;
	}

	spread = &AI2gKnowledge_DodgeFiringSpread[itr];
	spread->flags |= AI2cDodgeFiringSpreadFlag_InUse;
	if (outIndex != NULL) {
		*outIndex = itr;
	}

	if (itr >= AI2gKnowledge_NumDodgeFiringSpreads) {
		AI2gKnowledge_NumDodgeFiringSpreads++;
		UUmAssert(AI2gKnowledge_NumDodgeFiringSpreads <= AI2cKnowledge_MaxDodgeFiringSpreads);
	}

	return spread;
}

AI2tDodgeFiringSpread *AI2rKnowledge_GetDodgeFiringSpread(UUtUns32 inIndex)
{
	UUmAssert(AI2gKnowledge_DodgeFiringSpread[inIndex].flags & AI2cDodgeFiringSpreadFlag_InUse);
	return &AI2gKnowledge_DodgeFiringSpread[inIndex];
}

void AI2rKnowledge_DeleteDodgeFiringSpread(UUtUns32 inIndex)
{
	UUmAssert(AI2gKnowledge_DodgeFiringSpread[inIndex].flags & AI2cDodgeFiringSpreadFlag_InUse);

	AI2gKnowledge_DodgeFiringSpread[inIndex].flags &= ~AI2cDodgeFiringSpreadFlag_InUse;
	if (inIndex + 1 == AI2gKnowledge_NumDodgeFiringSpreads) {
		AI2gKnowledge_NumDodgeFiringSpreads--;
	}
}

// update all dodge projectiles
static void AI2iKnowledge_UpdateDodgeFiringSpreads(void)
{
	UUtUns32 itr;
	AI2tDodgeFiringSpread *spread;

	for (itr = 0, spread = AI2gKnowledge_DodgeFiringSpread; itr < AI2gKnowledge_NumDodgeFiringSpreads; itr++, spread++) {
		if (spread->flags & AI2cDodgeFiringSpreadFlag_InUse) {
			AI2iKnowledge_UpdateDodgeFiringSpread(spread);
		}
	}
}

// set up a newly created firing spread
void AI2rKnowledge_SetupDodgeFiringSpread(AI2tDodgeFiringSpread *ioSpread, WPtWeapon *ioWeapon)
{
	WPtWeaponClass *weapon_class = WPrGetClass(ioWeapon);
	M3tMatrix4x3 *matrix = WPrGetMatrix(ioWeapon);

	// copy information about the weapon
	ioSpread->angle = weapon_class->ai_parameters.dodge_pattern_angle;
	ioSpread->length = weapon_class->ai_parameters.dodge_pattern_length;
	ioSpread->radius_base = weapon_class->ai_parameters.dodge_pattern_base;

	ioSpread->position = MUrMatrix_GetTranslation(matrix);
	MUrMatrix3x3_MultiplyPoint(&weapon_class->ai_parameters.shooterdir_gunspace, (M3tMatrix3x3 *) matrix, &ioSpread->unit_direction);

	ioSpread->weapon = ioWeapon;
}

static void AI2iKnowledge_UpdateDodgeFiringSpread(AI2tDodgeFiringSpread *ioSpread)
{
	WPtWeaponClass *weapon_class;
	M3tMatrix4x3 *matrix;

	UUmAssert((ioSpread >= &AI2gKnowledge_DodgeFiringSpread[0]) && 
			  (ioSpread < &AI2gKnowledge_DodgeFiringSpread[AI2gKnowledge_NumDodgeFiringSpreads]));
	UUmAssert(ioSpread->flags & AI2cDodgeFiringSpreadFlag_InUse);

	if (!WPrInUse(ioSpread->weapon)) {
		UUmAssert(!"AI2iKnowledge_UpdateDodgeFiringSpread: dodge firing spread attached to deleted weapon");
		ioSpread->flags &= ~AI2cDodgeFiringSpreadFlag_InUse;
		return;
	}

	// update firing spread's position and orientation
	matrix = WPrGetMatrix(ioSpread->weapon);
	ioSpread->position = MUrMatrix_GetTranslation(matrix);
	weapon_class = WPrGetClass(ioSpread->weapon);
	MUrMatrix3x3_MultiplyPoint(&weapon_class->ai_parameters.shooterdir_gunspace, (M3tMatrix3x3 *) matrix, &ioSpread->unit_direction);
}

// update an AI's dodge table with any nearby firing spreads
void AI2rKnowledge_FindNearbyFiringSpreads(ONtCharacter *ioCharacter, struct AI2tManeuverState *ioManeuverState)
{
	UUtUns32 itr;
	AI2tDodgeFiringSpread *spread;
	M3tPoint3D pelvis_pt;
	M3tVector3D incoming_vector, center_vector;
	ONtActiveCharacter *active_character;
	float axial_dot, spread_r, dist_to_center, dist_sq, dist_mag;
	AKtEnvironment *environment;

	active_character = ONrGetActiveCharacter(ioCharacter);
	if (active_character == NULL) {
		return;
	}

	// calculate this character's location
	ONrCharacter_GetPelvisPosition(ioCharacter, &pelvis_pt);

	for (itr = 0, spread = AI2gKnowledge_DodgeFiringSpread; itr < AI2gKnowledge_NumDodgeFiringSpreads; itr++, spread++) {
		if ((spread->flags & AI2cDodgeFiringSpreadFlag_InUse) == 0)
			continue;

		if ((spread->weapon != NULL) && (WPrGetOwner(spread->weapon) == ioCharacter)) {
			// don't try to dodge our own gun!
			continue;
		}

		// calculate the vector from the firing spread to the character
		MUmVector_Subtract(incoming_vector, pelvis_pt, spread->position);
		dist_sq = MUmVector_GetLengthSquared(incoming_vector);
		if (dist_sq > UUmSQR(spread->length)) {
			// too far away, this spread is not relevant
			continue;
		}

		// calculate the perpendicular component of the incoming vector, this is the distance from the center
		// of the spread
		axial_dot = MUrVector_DotProduct(&spread->unit_direction, &incoming_vector);
		if (axial_dot < 0) {
			// the character is behind the firing spread
			continue;
		}
		MUmVector_Copy(center_vector, incoming_vector);
		MUmVector_ScaleIncrement(center_vector, -axial_dot, spread->unit_direction);

		// calculate the radius of the firing spread at this distance away from the base, and account for the character's
		// bounding sphere
		dist_mag = MUrSqrt(dist_sq);
		spread_r = spread->radius_base + dist_mag * MUrTan(spread->angle) + active_character->boundingSphere.radius;

		dist_sq = MUmVector_GetLengthSquared(center_vector);
		if (dist_sq > UUmSQR(spread_r)) {
			// the character is outside the firing spread
			continue;
		}

		// if the character is hidden by a wall, we can ignore this spread
		environment = ONrGameState_GetEnvironment();
		if (AKrCollision_Point(environment, &spread->position, &incoming_vector, AKcGQ_Flag_LOS_CanShoot_Skip_AI, 0)) {
			continue;
		}

		// negate the center direction so that we get distance to the center, not distance from the center
		dist_to_center = MUrSqrt(dist_sq);
		MUmVector_Negate(center_vector);

		// send a notification of this firing spread to the maneuver state
		AI2rManeuver_NotifyFiringSpread(ioCharacter, ioManeuverState, spread, &incoming_vector, spread_r, dist_to_center, &center_vector);
	}
}

// ------------------------------------------------------------------------------------
// -- projectile alertness routines

// update all AI about the position of any projectiles that might alert them
static void AI2iKnowledge_UpdateProjectileAlertness(void)
{
	ONtCharacter **character_list, *character, *owner_character;
	UUtUns32 itr, itr2, character_list_count;
	AI2tDodgeProjectile *projectile;
	M3tPoint3D pelvis_pt, projectile_start;
	M3tVector3D projectile_vel;
	UUtUns16 owner_team;
	float a, b, c, disc, t;
	UUtBool alerted;

	if (AI2gDeaf || AI2gEveryonePassive)
		return;

	character_list = ONrGameState_LivingCharacterList_Get();
	character_list_count = ONrGameState_LivingCharacterList_Count();

	for (itr = 0, projectile = AI2gKnowledge_DodgeProjectile; itr < AI2gKnowledge_NumDodgeProjectiles; itr++, projectile++) {
		if (((projectile->flags & AI2cDodgeProjectileFlag_InUse) == 0) || (projectile->alert_radius == 0))
			continue;

		if (!WPrOwner_IsCharacter(projectile->owner, &owner_character))
			continue;

		if (owner_character == NULL)
			continue;

		owner_team = owner_character->teamNumber;

		// this projectile alerts nearby AIs... precalculate constants for its motion this frame
		MUmVector_Subtract(projectile_vel, projectile->position, projectile->prev_position);
		a = MUmVector_GetLengthSquared(projectile_vel);
		
		for (itr2 = 0; itr2 < character_list_count; itr2++) {
			character = character_list[itr2];
			if (((character->flags & ONcCharacterFlag_InUse) == 0) ||
				(character->flags & ONcCharacterFlag_Dead) ||
				(character->charType != ONcChar_AI2))
				continue;
			
			if (character->ai2State.flags & (AI2cFlag_Passive | AI2cFlag_Deaf))
				continue;

			// don't get alerted by our own shots, or those from people on our team
			if ((character == owner_character) ||
				(AI2rTeam_FriendOrFoe(character->teamNumber, owner_team) == AI2cTeam_Friend))
				continue;

			// calculate the projectile's location relative to us and the remaining terms of
			// the quadratic equation for the intersection of its alertness sphere with our location
			ONrCharacter_GetPelvisPosition(character, &pelvis_pt);
			MUmVector_Subtract(projectile_start, projectile->prev_position, pelvis_pt);			
			b = 2 * MUrVector_DotProduct(&projectile_start, &projectile_vel);
			c = MUmVector_GetLengthSquared(projectile_start) - UUmSQR(projectile->alert_radius);

			alerted = UUcFalse;

			if (c <= 0) {
				// we are already inside the projectile's alert sphere
				alerted = UUcTrue;

			} else if (fabs(a) > 1e-03f) {
				// the projectile is not stationary

				disc = UUmSQR(b) - 4*a*c;
				if (disc > 0) {
					// the projectile will get close enough to us to be considered

					// work out the t-value of the first contact point
					t = (-b + MUrSqrt(disc)) / (2 * a);
					if ((t > 0) && (t < 1)) {
						// the projectile intersects our alert sphere within the time period
						alerted = UUcTrue;
					}
				}
			}

			if (alerted) {
				if (0 == (owner_character->flags & ONcCharacterFlag_InUse)) {
					COrConsole_Printf("projectile from deleted character entering the knowladge base");
				}

				// tell the AI about the projectile, and where it was fired from
				AI2iKnowledge_PostContact(character, AI2cContactType_Sound_Gunshot, AI2cContactStrength_Strong,
											&projectile->initial_position, owner_character, 0, UUcFalse, UUcFalse);
			}
		}
	}
}

// ------------------------------------------------------------------------------------
// -- debugging routines

#if defined(DEBUGGING) && DEBUGGING
// verify the knowledge base
void AI2rKnowledge_Verify(void)
{
	static UUtUns32 traverse_id = 1;
	UUtUns32 itr, itr2, index, num_found_chars;
	UUtBool found;
	AI2tKnowledgeState *knowledge_state;
	AI2tKnowledgeEntry *entry, *checkentry, *preventry;
	ONtCharacter *found_chars[20];

	if (!AI2gKnowledge_Valid)
		return;

	// get unique ID for this traverse
	traverse_id++;

	// tag all entries known to be free
	for (entry = AI2gKnowledge_NextFreeEntry; entry != NULL; entry = entry->nextptr) {
		UUmAssert((entry >= AI2gKnowledgeBase) && (entry < AI2gKnowledgeBase + AI2gKnowledge_ClearSpaceIndex));
		UUmAssert(entry->owner == NULL);
		entry->verify_traverse_id = traverse_id;
	}

	// loop over the knowledge base and make sure that there aren't any orphan entries
	entry = AI2gKnowledgeBase;
	for (itr = 0; itr < AI2gKnowledge_ClearSpaceIndex; itr++, entry++) {
		if (entry->owner == NULL) {
			// this must be a member of the freelist
			UUmAssert(entry->verify_traverse_id == traverse_id);
			continue;
		}

		knowledge_state = &entry->owner->ai2State.knowledgeState;
		if (knowledge_state->verify_traverse_id != traverse_id) {
			// loop over all of the entries in the owner's knowledge list and tag those entries as found to belong
			knowledge_state->verify_traverse_id = traverse_id;

			index = 0;
			found = UUcFalse;
			num_found_chars = 0;
			preventry = NULL;
			for (checkentry = knowledge_state->contacts; checkentry != NULL; checkentry = checkentry->nextptr, index++) {
				UUmAssert(checkentry->owner == entry->owner);
				UUmAssert(checkentry->enemy != entry->owner);

				UUmAssert(checkentry->prevptr == preventry);
				if (checkentry->prevptr == NULL) {
					UUmAssert(index == 0);
				} else {
					UUmAssert(checkentry->prevptr->nextptr = checkentry);
				}

				if (checkentry == entry) {
					found = UUcTrue;
				}

				for (itr2 = 0; itr2 < num_found_chars; itr2++) {
					UUmAssert(checkentry->enemy != found_chars[itr2]);
				}
				if (num_found_chars < 20) {
					found_chars[num_found_chars++] = checkentry->enemy;
				}

				UUmAssert(checkentry->verify_traverse_id != traverse_id);
				checkentry->verify_traverse_id = traverse_id;
				preventry = checkentry;
			}
			UUmAssert(found);
			UUmAssert(index == knowledge_state->num_contacts);
		} else {
			// this entry's owner has already had its list traversed... if we are a valid member of this list
			// then we must have had our ID set up already
			UUmAssert(entry->verify_traverse_id == traverse_id);
		}
	}
}
#endif

// force a character to forget about one or all of its contacts
void AI2rKnowledgeState_Forget(ONtCharacter *ioCharacter, AI2tKnowledgeState *ioKnowledgeState,
								ONtCharacter *inForget)
{
	AI2tKnowledgeEntry *entry, *prev_entry;

	entry = ioKnowledgeState->contacts;
	if (entry == NULL)
		return;

	// we skip to the end of the contact list
	while (entry->nextptr != NULL) {
		UUmAssert(entry->owner == ioCharacter);
		entry = entry->nextptr;
	}

	// now delete backwards all the way to the start
	while (entry != NULL) {
		prev_entry = entry->prevptr;

		if ((inForget == NULL) || (entry->enemy == inForget)) {
			// erase this contact by degrading it to forgotten, then deleting it
			entry->strength = AI2cContactStrength_Forgotten;
			if (ioKnowledgeState->callback != NULL) {
				(ioKnowledgeState->callback)(entry->owner, entry, UUcTrue);
			}

			AI2iKnowledge_RemoveEntry(entry, UUcTrue);
		}
		entry = prev_entry;
	}
}

// report in
void AI2rKnowledgeState_Report(ONtCharacter *ioCharacter, AI2tKnowledgeState *ioKnowledgeState,
								UUtBool inVerbose, AI2tReportFunction inFunction)
{
	AI2tKnowledgeEntry *entry;
	char longbuf[2048], reportbuf[256], tempbuf[128];

	UUmAssertReadPtr(ioKnowledgeState, sizeof(AI2tKnowledgeState));

	sprintf(longbuf, "  %d contacts:", ioKnowledgeState->num_contacts);

	if (inVerbose) {
		inFunction(longbuf);
	}
	
	entry = ioKnowledgeState->contacts;
	while (entry != NULL) {
		UUmAssert(entry->owner == ioCharacter);

		if (inVerbose) {
			sprintf(reportbuf, " %s:", (entry->enemy == NULL) ? "nobody" : entry->enemy->player_name);
			inFunction(reportbuf);

			sprintf(reportbuf, "%sthreat strength ", (entry->priority == AI2cContactPriority_Hostile_Threat) ? "hostile" :
													 (entry->priority == AI2cContactPriority_Hostile_NoThreat) ? "hostile-nothreat" : "friendly");

			if ((entry->strength < 0) || (entry->strength >= AI2cContactStrength_Max)) {
				sprintf(tempbuf, "<unknown: %d>", entry->strength);
				strcat(reportbuf, tempbuf);
			} else {
				strcat(reportbuf, AI2cContactStrengthName[entry->strength]);
			}

			if (entry->highest_strength != entry->strength) {
				strcat(reportbuf, " (max ");

				if ((entry->highest_strength < 0) || (entry->highest_strength >= AI2cContactStrength_Max)) {
					sprintf(tempbuf, "<unknown: %d>", entry->highest_strength);
					strcat(reportbuf, tempbuf);
				} else {
					strcat(reportbuf, AI2cContactStrengthName[entry->highest_strength]);
				}
				
				strcat(reportbuf, ")");
			}

			sprintf(tempbuf, " initiate %d, degrade %d", entry->first_time, entry->degrade_time);
			strcat(reportbuf, tempbuf);
			inFunction(reportbuf);

			strcpy(reportbuf, "  last: ");
			switch(entry->last_type) {
			case AI2cContactType_Sound_Ignore:
				strcat(reportbuf, "sound-ignore");
				break;

			case AI2cContactType_Sound_Interesting:
				strcat(reportbuf, "sound-interesting");
				break;

			case AI2cContactType_Sound_Danger:
				strcat(reportbuf, "sound-danger");
				break;

			case AI2cContactType_Sound_Melee:
				strcat(reportbuf, "sound-melee");
				break;

			case AI2cContactType_Sound_Gunshot:
				strcat(reportbuf, "sound-gunshot");
				break;

			case AI2cContactType_Sight_Peripheral:
				strcat(reportbuf, "sight-peripheral");
				break;

			case AI2cContactType_Sight_Definite:
				strcat(reportbuf, "sight-definite");
				break;

			case AI2cContactType_Hit_Weapon:
				strcat(reportbuf, "hit-weapon");
				break;

			case AI2cContactType_Hit_Melee:
				strcat(reportbuf, "hit-melee");
				break;

			default:
				sprintf(tempbuf, "<unknown: %d>", entry->last_type);
				strcat(reportbuf, tempbuf);
				break;
			}

			sprintf(tempbuf, " time %d, at %f %f %f", entry->last_time,
					entry->last_location.x, entry->last_location.y, entry->last_location.z);
			strcat(reportbuf, tempbuf);
			inFunction(reportbuf);
		} else {
			if ((entry->strength < 0) || (entry->strength >= AI2cContactStrength_Max)) {
				sprintf(tempbuf, "<unknown: %d>", entry->strength);
			} else {
				strcpy(tempbuf, AI2cContactStrengthName[entry->strength]);
			}

			sprintf(reportbuf, " (%s-%s)", (entry->enemy == NULL) ? "nobody" : entry->enemy->player_name, tempbuf);
			strcat(longbuf, reportbuf);
		}

		entry = entry->nextptr;
	}

	if (!inVerbose) {
		inFunction(longbuf);
	}
}

// display debugging information about projectile awareness
void AI2rKnowledge_DisplayProjectiles(void)
{
	UUtUns32 itr;
	AI2tDodgeProjectile *projectile;

	for (itr = 0, projectile = AI2gKnowledge_DodgeProjectile; itr < AI2gKnowledge_NumDodgeProjectiles; itr++, projectile++) {
		if (projectile->flags & AI2cDodgeProjectileFlag_InUse) {
			M3rGeom_Line_Light(&projectile->prev_position, &projectile->position, IMcShade_Blue);
			if (projectile->dodge_radius > 0) {
				M3rGeom_Draw_DebugSphere(&projectile->position, projectile->dodge_radius, IMcShade_Blue);
			}
			if (projectile->alert_radius > 0) {
				M3rGeom_Draw_DebugSphere(&projectile->position, projectile->alert_radius, IMcShade_Red);
			}
		}
	}
}

// display debugging information about firing spread awareness
void AI2rKnowledge_DisplayFiringSpreads(void)
{
	UUtUns32 itr, itr2;
	AI2tDodgeFiringSpread *spread;
	M3tVector3D right, up;
	M3tPoint3D base[4], end[4];
	float r_end;

	for (itr = 0, spread = AI2gKnowledge_DodgeFiringSpread; itr < AI2gKnowledge_NumDodgeFiringSpreads; itr++, spread++) {
		if (spread->flags & AI2cDodgeFiringSpreadFlag_InUse) {
			// build an orientation for this firing spread
			right.x = -spread->unit_direction.z;
			right.y = 0;
			right.z = spread->unit_direction.x;

			MUrVector_CrossProduct(&right, &spread->unit_direction, &up);

			// calculate the firing spread's end radius
			r_end = spread->radius_base + MUrTan(spread->angle) * spread->length;

			// display the firing cone
			MUmVector_ScaleCopy(base[0], spread->radius_base, right);
			MUmVector_ScaleCopy(base[1], spread->radius_base, up);
			MUmVector_ScaleCopy(base[2], -spread->radius_base, right);
			MUmVector_ScaleCopy(base[3], -spread->radius_base, up);
			MUmVector_ScaleCopy(end[0], r_end, right);
			MUmVector_ScaleCopy(end[1], r_end, up);
			MUmVector_ScaleCopy(end[2], -r_end, right);
			MUmVector_ScaleCopy(end[3], -r_end, up);

			for (itr2 = 0; itr2 < 4; itr2++) {
				MUmVector_ScaleIncrement(end[itr2], spread->length, spread->unit_direction);
				MUmVector_Add(base[itr2], base[itr2], spread->position);
				MUmVector_Add(end[itr2], end[itr2], spread->position);
				M3rGeom_Line_Light(&base[itr2], &end[itr2], IMcShade_Red);
			}

			for (itr2 = 0; itr2 < 4; itr2++) {
				M3rGeom_Line_Light(&base[itr2], &base[(itr2 + 1) % 4], IMcShade_Red);
				M3rGeom_Line_Light(&end[itr2], &end[(itr2 + 1) % 4], IMcShade_Red);
			}
		}
	}
}

// display debugging information about AI sounds
void AI2rKnowledge_DisplaySounds(void)
{
#if TOOL_VERSION
	UUtUns32 itr;

	for (itr = 0; itr < AI2gKnowledge_DebugNumSounds; ) {
		if (AI2gKnowledge_DebugSounds[itr].time + AI2cKnowledge_DebugSoundDecay <= ONrGameState_GetGameTime()) {
			// delete this sound
			if (itr < AI2gKnowledge_DebugNumSounds - 1) {
				AI2gKnowledge_DebugSounds[itr] = AI2gKnowledge_DebugSounds[AI2gKnowledge_DebugNumSounds - 1];
			}
			AI2gKnowledge_DebugNumSounds--;
			continue;
		}

		M3rGeom_Draw_DebugSphere(&AI2gKnowledge_DebugSounds[itr].location, AI2gKnowledge_DebugSounds[itr].radius,
								AI2gKnowledge_DebugSoundShade[AI2gKnowledge_DebugSounds[itr].type]);
		itr++;
	}
#endif
}