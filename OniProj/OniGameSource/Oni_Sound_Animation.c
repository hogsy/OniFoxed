// ======================================================================
// Oni_Sound_Animation.c
// ======================================================================

// ======================================================================
// include
// ======================================================================
#include "BFW.h"
#include "BFW_SoundSystem2.h"

#include "Oni_Sound2.h"
#include "Oni_Sound_Private.h"
#include "Oni_Sound_Animation.h"
#include "Oni_Character.h"
#include "Oni_Character_Animation.h"
#include "Oni_BinaryData.h"

// ======================================================================
// define
// ======================================================================
#define OScDefaultNumVariants					16

#define OScSAFileTag							UUm4CharToUns32('S', 'A', 'F', 'T')
#define OScVariantTag							UUm4CharToUns32('S', 'A', 'V', 'T')
#define OScSATag								UUm4CharToUns32('S', 'A', 'S', 'A')

#define OSgMaxBufferSize						1024

// ======================================================================
// typedefs
// ======================================================================
struct OStVariant
{
	const ONtCharacterVariant	*character_variant;
	UUtMemory_Array				*anim_type_sounds[OScAnimType_NumTypes];
};

typedef struct OStTotoroToSound
{
	TRtAnimType				totoro_anim_type;
	OStAnimType				sound_anim_type;
	OStModType				sound_mod_type;

} OStTotoroToSound;

// ======================================================================
// globals
// ======================================================================
static UUtMemory_Array		*OSgVariants;

static OStTypeName			OSgModTypeName[] =
{
	{ "Any",				OScModType_Any },
	{ "Crouch",				OScModType_Crouch },
	{ "Jump",				OScModType_Jump },
	{ "Heavy Damage",		OScModType_HeavyDamage },
	{ "Medium Damage",		OScModType_MediumDamage },
	{ "Light Damage",		OScModType_LightDamage },
	{ NULL,					OScModType_None }
};

static OStTypeName			OSgAnimTypeName[] =
{
	{ "Any",				OScAnimType_Any },
	{ "Animation",			OScAnimType_Animation },
	{ "Block",				OScAnimType_Block },
	{ "Draw Weapon",		OScAnimType_DrawWeapon },
	{ "Fall",				OScAnimType_Fall },
	{ "Fly",				OScAnimType_Fly },
	{ "Getting Hit",		OScAnimType_GettingHit },
	{ "Holster",			OScAnimType_Holster },
	{ "Kick",				OScAnimType_Kick },
	{ "Knockdown",			OScAnimType_Knockdown },
	{ "Land",				OScAnimType_Land },
	{ "Jump",				OScAnimType_Jump },
	{ "Pickup",				OScAnimType_Pickup },
	{ "Punch",				OScAnimType_Punch },
	{ "Reload Pistol",		OScAnimType_Reload_Pistol },
	{ "Reload Rifle",		OScAnimType_Reload_Rifle },
	{ "Reload Stream",		OScAnimType_Reload_Stream },
	{ "Reload Superball",	OScAnimType_Reload_Superball },
	{ "Reload Vandegraf",	OScAnimType_Reload_Vandegraf },
	{ "Reload Scram Cannon",OScAnimType_Reload_Scram_Cannon },
	{ "Reload Mercury Bow",	OScAnimType_Reload_MercuryBow },
	{ "Reload Screamer",	OScAnimType_Reload_Screamer },
	{ "Run",				OScAnimType_Run },
	{ "Slide",				OScAnimType_Slide },
	{ "Stand",				OScAnimType_Stand },
	{ "Startle",			OScAnimType_Startle },
	{ "Walk",				OScAnimType_Walk },
	{ "Powerup",			OScAnimType_Powerup },
	{ "Roll",				OScAnimType_Roll },
	{ "Falling Flail",		OScAnimType_FallingFlail },

	{ NULL,					OScAnimType_None }
};

static OStTotoroToSound		OSgTotoroToSoundInternal[ONcAnimType_Num] =
{
	{ ONcAnimType_None,							OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Anything,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Walk,							OScAnimType_Walk,		OScModType_Any },
	{ ONcAnimType_Run,							OScAnimType_Run,		OScModType_Any },
	{ ONcAnimType_Slide,						OScAnimType_Slide,		OScModType_Any },
	{ ONcAnimType_Jump,							OScAnimType_Jump,		OScModType_Any },
	{ ONcAnimType_Stand,						OScAnimType_Stand,		OScModType_Any },

	{ ONcAnimType_Standing_Turn_Left,			OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Standing_Turn_Right,			OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Run_Backwards,				OScAnimType_Run,		OScModType_Any },
	{ ONcAnimType_Run_Sidestep_Left,			OScAnimType_Run,		OScModType_Any },
	{ ONcAnimType_Run_Sidestep_Right,			OScAnimType_Run,		OScModType_Any },
	{ ONcAnimType_Kick,							OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Walk_Sidestep_Left,			OScAnimType_Walk,		OScModType_Any },
	{ ONcAnimType_Walk_Sidestep_Right,			OScAnimType_Walk,		OScModType_Any },
	{ ONcAnimType_Walk_Backwards,				OScAnimType_Walk,		OScModType_Any },
	{ ONcAnimType_Stance,						OScAnimType_Stand,		OScModType_Any },

	{ ONcAnimType_Crouch,						OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Jump_Forward,					OScAnimType_Jump,		OScModType_Any },
	{ ONcAnimType_Jump_Backward,				OScAnimType_Jump,		OScModType_Any },
	{ ONcAnimType_Jump_Left,					OScAnimType_Jump,		OScModType_Any },
	{ ONcAnimType_Jump_Right,					OScAnimType_Jump,		OScModType_Any },
	{ ONcAnimType_Punch,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Block,						OScAnimType_Block,		OScModType_Any },
	{ ONcAnimType_Land,							OScAnimType_Land,		OScModType_Any },
	{ ONcAnimType_Fly,							OScAnimType_Fly,		OScModType_Any },
	{ ONcAnimType_Kick_Forward,					OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Kick_Left,					OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Kick_Right,					OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Kick_Back,					OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Kick_Low,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Punch_Forward,				OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Punch_Left,					OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Punch_Right,					OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Punch_Back,					OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Punch_Low,					OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Kick2,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Kick3,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Punch2,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Punch3,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Land_Forward,					OScAnimType_Land,		OScModType_Any },
	{ ONcAnimType_Land_Right,					OScAnimType_Land,		OScModType_Any },
	{ ONcAnimType_Land_Left,					OScAnimType_Land,		OScModType_Any },
	{ ONcAnimType_Land_Back,					OScAnimType_Land,		OScModType_Any },
	{ ONcAnimType_PPK,							OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_PKK,							OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_PKP,							OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_KPK,							OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_KPP,							OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_KKP,							OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_PK,							OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_KP,							OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Punch_Heavy,					OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Kick_Heavy,					OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Punch_Forward_Heavy,			OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Kick_Forward_Heavy,			OScAnimType_Kick,		OScModType_Any },

	{ ONcAnimType_Aiming_Overlay, 				OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Hit_Overlay,					OScAnimType_GettingHit,	OScModType_Any },
	{ ONcAnimType_Crouch_Run,					OScAnimType_None,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Walk,					OScAnimType_None,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Run_Backwards,			OScAnimType_None,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Walk_Backwards,		OScAnimType_None,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Run_Sidestep_Left,		OScAnimType_None,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Run_Sidestep_Right,	OScAnimType_None,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Walk_Sidestep_Left,	OScAnimType_None,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Walk_Sidestep_Right,	OScAnimType_None,		OScModType_Crouch },

	{ ONcAnimType_Run_Kick,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Run_Punch,					OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Run_Back_Punch,				OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Run_Back_Kick,				OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Sidestep_Left_Kick,			OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Sidestep_Left_Punch,			OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Sidestep_Right_Kick,			OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Sidestep_Right_Punch,			OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Prone,						OScAnimType_Fall,		OScModType_Any },

	{ ONcAnimType_Flip,							OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Hit_Head,						OScAnimType_GettingHit,	OScModType_Any },
	{ ONcAnimType_Hit_Body,						OScAnimType_GettingHit,	OScModType_Any },
	{ ONcAnimType_Hit_Foot,						OScAnimType_GettingHit,	OScModType_Any },
	{ ONcAnimType_Knockdown_Head,				OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Knockdown_Body,				OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Knockdown_Foot,				OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Hit_Crouch,					OScAnimType_GettingHit,	OScModType_Crouch },
	{ ONcAnimType_Knockdown_Crouch,				OScAnimType_None,		OScModType_Crouch },
	{ ONcAnimType_Hit_Fallen,					OScAnimType_GettingHit,	OScModType_Any },
	{ ONcAnimType_Hit_Head_Behind,				OScAnimType_GettingHit,	OScModType_Any },
	{ ONcAnimType_Hit_Body_Behind,				OScAnimType_GettingHit,	OScModType_Any },
	{ ONcAnimType_Hit_Foot_Behind,				OScAnimType_GettingHit,	OScModType_Any },
	{ ONcAnimType_Knockdown_Head_Behind,		OScAnimType_Knockdown,	OScModType_Any },
	{ ONcAnimType_Knockdown_Body_Behind,		OScAnimType_Knockdown,	OScModType_Any },
	{ ONcAnimType_Knockdown_Foot_Behind,		OScAnimType_Knockdown,	OScModType_Any },
	{ ONcAnimType_Hit_Crouch_Behind,			OScAnimType_GettingHit,	OScModType_Any },
	{ ONcAnimType_Knockdown_Crouch_Behind,		OScAnimType_Knockdown,	OScModType_Any },
	{ ONcAnimType_Idle,							OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Taunt,						OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Throw,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown1,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown2,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown3,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown4,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown5,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown6,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Special1,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Special2,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Special3,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Special4,						OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Throw_Forward_Punch,			OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Throw_Forward_Kick,			OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Throw_Behind_Punch,			OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Throw_Behind_Kick,			OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Run_Throw_Forward_Punch,		OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Run_Throw_Behind_Punch,		OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_Run_Throw_Forward_Kick,		OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Run_Throw_Behind_Kick,		OScAnimType_Kick,		OScModType_Any },

	{ ONcAnimType_Thrown7,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown8,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown9,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown10,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown11,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown12,						OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Startle_Left_Unused,			OScAnimType_Startle,	OScModType_Any },
	{ ONcAnimType_Startle_Right_Unused,			OScAnimType_Startle,	OScModType_Any },

	{ ONcAnimType_Sit,							OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Stand_Special,				OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Act,							OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Kick3_Forward,				OScAnimType_Kick,		OScModType_Any },

	{ ONcAnimType_Hit_Foot_Ouch,				OScAnimType_GettingHit,	OScModType_Any },
	{ ONcAnimType_Hit_Jewels,					OScAnimType_GettingHit,	OScModType_Any },

	{ ONcAnimType_Thrown13,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown14,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown15,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown16,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Thrown17,						OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_PPKK,							OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_PPKKK,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_PPKKKK,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_Land_Hard,					OScAnimType_Land,		OScModType_Any },
	{ ONcAnimType_Land_Forward_Hard,			OScAnimType_Land,		OScModType_Any },
	{ ONcAnimType_Land_Right_Hard,				OScAnimType_Land,		OScModType_Any },
	{ ONcAnimType_Land_Left_Hard,				OScAnimType_Land,		OScModType_Any },
	{ ONcAnimType_Land_Back_Hard,				OScAnimType_Land,		OScModType_Any },
	{ ONcAnimType_Land_Dead,					OScAnimType_Land,		OScModType_Any },

	{ ONcAnimType_Crouch_Turn_Left,				OScAnimType_None,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Turn_Right,			OScAnimType_None,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Forward,				OScAnimType_Roll,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Back,					OScAnimType_Roll,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Left,					OScAnimType_Roll,		OScModType_Crouch },
	{ ONcAnimType_Crouch_Right,					OScAnimType_Roll,		OScModType_Crouch },

	{ ONcAnimType_Getup_Kick_Back,				OScAnimType_Kick,		OScModType_Any },

	{ ONcAnimType_Autopistol_Recoil,			OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Phase_Rifle_Recoil,			OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Phase_Stream_Recoil,			OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Superball_Recoil,				OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Vandegraf_Recoil,				OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Scram_Cannon_Recoil,			OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Mercury_Bow_Recoil,			OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Screamer_Recoil,				OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Pickup_Object,				OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Pickup_Pistol,				OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Pickup_Rifle,					OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Holster,						OScAnimType_Holster,	OScModType_Any },
	{ ONcAnimType_Draw_Pistol,					OScAnimType_DrawWeapon,	OScModType_Any },
	{ ONcAnimType_Draw_Rifle,					OScAnimType_DrawWeapon,	OScModType_Any },

	{ ONcAnimType_Punch4,						OScAnimType_Punch,		OScModType_Any },

	{ ONcAnimType_Reload_Pistol,				OScAnimType_Reload_Pistol,		OScModType_Any },
	{ ONcAnimType_Reload_Rifle,					OScAnimType_Reload_Rifle,		OScModType_Any },
	{ ONcAnimType_Reload_Stream,				OScAnimType_Reload_Stream,		OScModType_Any },
	{ ONcAnimType_Reload_Superball,				OScAnimType_Reload_Superball,	OScModType_Any },
	{ ONcAnimType_Reload_Vandegraf,				OScAnimType_Reload_Vandegraf,	OScModType_Any },
	{ ONcAnimType_Reload_Scram_Cannon,			OScAnimType_Reload_Scram_Cannon,	OScModType_Any },
	{ ONcAnimType_Reload_MercuryBow,			OScAnimType_Reload_MercuryBow,		OScModType_Any },
	{ ONcAnimType_Reload_Screamer,				OScAnimType_Reload_Screamer,		OScModType_Any },

	{ ONcAnimType_PF_PF,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_PF_PF_PF,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_PL_PL,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_PL_PL_PL,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_PR_PR,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_PR_PR_PR,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_PB_PB,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_PB_PB_PB,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_PD_PD,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_PD_PD_PD,						OScAnimType_Punch,		OScModType_Any },
	{ ONcAnimType_KF_KF,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_KF_KF_KF,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_KL_KL,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_KL_KL_KL,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_KR_KR,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_KR_KR_KR,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_KB_KB,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_KB_KB_KB,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_KD_KD,						OScAnimType_Kick,		OScModType_Any },
	{ ONcAnimType_KD_KD_KD,						OScAnimType_Kick,		OScModType_Any },

	{ ONcAnimType_Startle_Left,					OScAnimType_Startle,	OScModType_Any },
	{ ONcAnimType_Startle_Right,				OScAnimType_Startle,	OScModType_Any },
	{ ONcAnimType_Startle_Back,					OScAnimType_Startle,	OScModType_Any },
	{ ONcAnimType_Startle_Forward,				OScAnimType_Startle,	OScModType_Any },
	{ ONcAnimType_Console,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Console_Walk,					OScAnimType_Walk,		OScModType_Any },
	{ ONcAnimType_Stagger,						OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Watch,						OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Act_No,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Act_Yes,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Act_Talk,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Act_Shrug,					OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Act_Shout,					OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Act_Give,						OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Run_Stop,						OScAnimType_Run,		OScModType_Any },
	{ ONcAnimType_Walk_Stop,					OScAnimType_Walk,		OScModType_Any },

	{ ONcAnimType_Run_Start,					OScAnimType_Run,		OScModType_Any },
	{ ONcAnimType_Walk_Start,					OScAnimType_Walk,		OScModType_Any },
	{ ONcAnimType_Run_Backwards_Start,			OScAnimType_Run,		OScModType_Any },
	{ ONcAnimType_Walk_Backwards_Start,			OScAnimType_Walk,		OScModType_Any },

	{ ONcAnimType_Stun,							OScAnimType_Run,		OScModType_Any },
	{ ONcAnimType_Stagger_Behind,				OScAnimType_Walk,		OScModType_Any },
	{ ONcAnimType_Blownup,						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Blownup_Behind,				OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_1Step_Stop,					OScAnimType_Run,		OScModType_Any },
	{ ONcAnimType_Run_Sidestep_Left_Start,		OScAnimType_Run,		OScModType_Any },
	{ ONcAnimType_Run_Sidestep_Right_Start,		OScAnimType_Run,		OScModType_Any },

	{ ONcAnimType_Powerup,						OScAnimType_Powerup,	OScModType_Any },
	{ ONcAnimType_Falling_Flail,				OScAnimType_FallingFlail,OScModType_Any },
	{ ONcAnimType_Console_Punch,				OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Teleport_In,					OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Teleport_Out,					OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Ninja_Fireball,				OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Ninja_Invisible,				OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Punch_Rifle,					OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Pickup_Object_Mid, 			OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Pickup_Pistol_Mid, 			OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Pickup_Rifle_Mid, 			OScAnimType_None,		OScModType_Any },

	{ ONcAnimType_Hail, 						OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Muro_Thunderbolt, 			OScAnimType_None,		OScModType_Any },
	{ ONcAnimType_Hit_Overlay_AI,	 			OScAnimType_GettingHit,	OScModType_Any }

};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static OStTotoroToSound*
OSiTotoroToSound(
	TRtAnimType					inType)
{
	OStTotoroToSound			*result;

	#if defined(DEBUGGING) && (DEBUGGING)
	{
		static UUtBool once = UUcTrue;

		if (once) {
			UUtUns32 itr;

			for(itr = 0; itr < ONcAnimType_Num; itr++)
			{
				UUmAssert(OSgTotoroToSoundInternal[itr].totoro_anim_type == itr);
			}

			once = UUcFalse;
		}
	}
	#endif

	if (inType < ONcAnimType_Num) {
		result = OSgTotoroToSoundInternal + inType;
	}
	else {
		result = OSgTotoroToSoundInternal + ONcAnimType_None;
	}

	return result;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
OSiVariant_CalcWriteSize(
	const OStVariant			*inVariant)
{
	UUtUns32					num_elements;
	UUtUns32					i;
	UUtUns32					element_size;
	UUtUns32					size;

	// calculate the total number of elements in the variant
	num_elements = 0;
	for (i = 0; i < OScAnimType_NumTypes; i++)
	{
		num_elements += UUrMemory_Array_GetUsedElems(inVariant->anim_type_sounds[i]);
	}

	// calculate the number of bytes needed to save one OStSoundAnimation
	element_size =
		sizeof(UUtTag) +			/* tag */
//		sizeof(UUtUns32) +			/* impulse_id */
		sizeof(UUtUns32) +			/* frame */
		SScMaxNameLength +			/* anim_type mod name */
		SScMaxNameLength +			/* anim_type dir name */
		SScMaxNameLength +			/* anim_type base name */
		OScMaxAnimNameLength +		/* OScMaxAnimNameLength */
		SScMaxNameLength;			/* impulse sound name */

	// calculate the total number of bytes needed to save all of the OStSoundAnimations
	size = num_elements * element_size;

	return size;
}

// ----------------------------------------------------------------------
static void
OSiVariant_DeleteAllSounds(
	OStVariant					*ioVariant)
{
	UUtUns32					itr;
	UUtMemory_Array				*type_array;
	OStSoundAnimation			*sound_array;
	UUtUns32					num_elements;

	UUmAssert(ioVariant);

	// clear the sound of all animations
	type_array = ioVariant->anim_type_sounds[OScAnimType_Animation];
	sound_array = (OStSoundAnimation*)UUrMemory_Array_GetMemory(type_array);
	num_elements = UUrMemory_Array_GetUsedElems(type_array);
	for (itr = 0; itr < num_elements; itr++)
	{
		TRtAnimation				*animation;

		animation = sound_array[itr].animation;
		if (animation == NULL) { continue; }

		TRrAnimation_SetSoundName(animation, NULL, (TRtAnimTime)0);
	}

	// delete the anim type sounds
	for (itr = 0; itr < OScAnimType_NumTypes; itr++)
	{
		UUrMemory_Array_SetUsedElems(ioVariant->anim_type_sounds[itr], 0, NULL);
	}
}

// ----------------------------------------------------------------------
const char*
OSrVariant_GetName(
	const OStVariant			*inVariant)
{
	UUmAssert(inVariant);
	return inVariant->character_variant->name;
}

// ----------------------------------------------------------------------
UUtUns32
OSrVariant_GetNumSounds(
	const OStVariant			*inVariant)
{
	UUtUns32					i;
	UUtUns32					num_sounds;

	UUmAssert(inVariant);

	num_sounds = 0;
	for (i = 0; i < OScAnimType_NumTypes; i++)
	{
		num_sounds += UUrMemory_Array_GetUsedElems(inVariant->anim_type_sounds[i]);
	}

	return num_sounds;
}

// ----------------------------------------------------------------------
static SStImpulse*
OSiVariant_Impulse_Get(
	const OStVariant			*inVariant,
	const TRtAnimation			*inAnimation,
	const OStAnimType			inAnimType,
	const OStModType			inModType)
{
	SStImpulse					*out_impulse;
	const char					*impulse_name;

	UUmAssert(inVariant);

	out_impulse = NULL;

	// see if the animation has a valid impulse sound
	if (inAnimation != NULL)
	{
		impulse_name = TRrAnimation_GetSoundName(inAnimation);

		if (impulse_name != NULL)
		{
			out_impulse = OSrImpulse_GetByName(impulse_name);
			if (out_impulse == NULL) {
				COrConsole_Printf("OSiVariant_Impulse_Get: failed to find impulse sound %s on animation %s",
									impulse_name, TRrAnimation_GetName(inAnimation));
			}

			goto exit;
		}
	}

	if ((out_impulse == NULL) && (inAnimType != OScAnimType_Animation))
	{
		UUtMemory_Array				*type_array;
		OStSoundAnimation			*sound_array;
		UUtUns32					i;
		UUtUns32					num_elements;

		// search the inAnimType sound array for the best sound
		type_array = inVariant->anim_type_sounds[inAnimType];
		sound_array = (OStSoundAnimation*)UUrMemory_Array_GetMemory(type_array);
		num_elements = UUrMemory_Array_GetUsedElems(type_array);
		for (i = 0; i < num_elements; i++)
		{
			if (sound_array[i].anim_type != inAnimType) { continue; }

			if ((sound_array[i].mod_type == OScModType_Any) ||
				(sound_array[i].mod_type == inModType))
			{
				out_impulse = OSrImpulse_GetByName(sound_array[i].impulse_name);

				if (sound_array[i].mod_type == inModType) { break; }
			}
		}

		if (out_impulse == NULL)
		{
			// search the OScAnimType_Any sound array for the best sound
			type_array = inVariant->anim_type_sounds[OScAnimType_Any];
			sound_array = (OStSoundAnimation*)UUrMemory_Array_GetMemory(type_array);
			num_elements = UUrMemory_Array_GetUsedElems(type_array);
			for (i = 0; i < num_elements; i++)
			{
				if ((sound_array[i].mod_type == OScModType_Any) ||
					(sound_array[i].mod_type == inModType))
				{
					out_impulse = OSrImpulse_GetByName(sound_array[i].impulse_name);

					if (sound_array[i].mod_type == inModType) { break; }
				}
			}
		}
	}

exit:
	return out_impulse;
}

// ----------------------------------------------------------------------
static SStImpulse*
OSiVariant_Impulse_GetExact(
	const OStVariant			*inVariant,
	const TRtAnimation			*inAnimation,
	const OStAnimType			inAnimType,
	const OStModType			inModType)
{
	SStImpulse					*out_impulse;
	const char					*impulse_name;

	UUmAssert(inVariant);

	out_impulse = NULL;

	// see if the animation has a valid impulse id
	if (inAnimation != NULL)
	{
		impulse_name = TRrAnimation_GetSoundName(inAnimation);
		if (impulse_name != NULL)
		{
			out_impulse = OSrImpulse_GetByName(impulse_name);
		}
	}

	if ((out_impulse == NULL) && (inAnimType != OScAnimType_Animation))
	{
		UUtMemory_Array				*type_array;
		OStSoundAnimation			*sound_array;
		UUtUns32					i;
		UUtUns32					num_elements;

		// search the inAnimType sound array for the best sound
		type_array = inVariant->anim_type_sounds[inAnimType];
		sound_array = (OStSoundAnimation*)UUrMemory_Array_GetMemory(type_array);
		num_elements = UUrMemory_Array_GetUsedElems(type_array);
		for (i = 0; i < num_elements; i++)
		{
			if (sound_array[i].mod_type != inModType) { continue; }

			out_impulse = OSrImpulse_GetByName(sound_array[i].impulse_name);
			break;
		}
	}

	return out_impulse;
}

// ----------------------------------------------------------------------
static UUtError
OSiVariant_Initialize(
	OStVariant					*ioVariant,
	ONtCharacterVariant			*inCharacterVariant)
{
	UUtUns32					i;

	UUmAssert(ioVariant);
	UUmAssert(inCharacterVariant);

	// set the character variant
	ioVariant->character_variant = inCharacterVariant;

	// allocate the memory arrays
	for (i = 0; i < OScAnimType_NumTypes; i++)
	{
		ioVariant->anim_type_sounds[i] =
			UUrMemory_Array_New(
				sizeof(OStSoundAnimation),
				1,
				0,
				0);
		UUmError_ReturnOnNull(ioVariant->anim_type_sounds[i]);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OSrVariant_SoundAnimation_Add(
	OStVariant					*ioVariant,
	OStAnimType					inAnimType,
	OStModType					inModType,
	const char					*inAnimationName, /* unused if inAnimType != OScAnimType_Animation */
	UUtUns32					inFrame,
	const char					*inImpulseName)
{
	UUtError					error;
	SStImpulse					*impulse;
	TRtAnimation				*animation;
	UUtMemory_Array				*type_array;
	UUtUns32					index;
	OStSoundAnimation			*sound_array;
	const char					*anim_name;

	UUmAssert(ioVariant);
	UUmAssert(inImpulseName);

	// get a pointer to the instance
	if (inAnimType == OScAnimType_Animation)
	{
		UUmAssert(inAnimationName);

		error = TMrInstance_GetDataPtr(TRcTemplate_Animation, inAnimationName, &animation);
		if (error != UUcError_None) { animation = NULL; }
		anim_name = inAnimationName;
	}
	else
	{
		animation = NULL;
		anim_name = "";
	}

	// see if a sound animation already exists for these parameters
	impulse = OSiVariant_Impulse_GetExact(ioVariant, animation, inAnimType, inModType);
	if (impulse != NULL) { return UUcError_Generic; }

	// add the sound animation to the variant
	type_array = ioVariant->anim_type_sounds[inAnimType];
	error = UUrMemory_Array_GetNewElement(type_array, &index, NULL);
	UUmError_ReturnOnError(error);

	sound_array = (OStSoundAnimation*)UUrMemory_Array_GetMemory(type_array);
	UUmAssert(sound_array);

	// initialize the element
	sound_array[index].anim_type = inAnimType;
	sound_array[index].mod_type = inModType;
	sound_array[index].impulse = OSrImpulse_GetByName(inImpulseName);
	sound_array[index].frame = inFrame;
	sound_array[index].animation = animation;
	UUrString_Copy(sound_array[index].anim_name, anim_name, OScMaxAnimNameLength);
	UUrString_Copy(sound_array[index].impulse_name, inImpulseName, SScMaxNameLength);

	// set the sound id of the animation
	if (animation != NULL)
	{
		TRrAnimation_SetSoundName(animation, sound_array[index].impulse_name, (TRtAnimTime)inFrame);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrVariant_SoundAnimation_DeleteByIndex(
	OStVariant					*ioVariant,
	UUtUns32					inIndex)
{
	UUtUns32					i;
	UUtUns32					index;

	UUmAssert(ioVariant);

	index = inIndex;

	// go through all of the anim types and find the one which has this index
	// then delete the OStSoundAnimation from the anim type sound array
	for (i = 0; i < OScAnimType_NumTypes; i++)
	{
		UUtUns32					num_elements;

		num_elements = UUrMemory_Array_GetUsedElems(ioVariant->anim_type_sounds[i]);
		if (index < num_elements)
		{
			// clear animations of their sound id
			if (i == OScAnimType_Animation)
			{
				OStSoundAnimation			*sound_array;

				sound_array =
					(OStSoundAnimation*)UUrMemory_Array_GetMemory(ioVariant->anim_type_sounds[i]);
				if (sound_array[index].animation)
				{
					TRrAnimation_SetSoundName(sound_array[index].animation, NULL, 0);
				}
			}

			UUrMemory_Array_DeleteElement(ioVariant->anim_type_sounds[i], index);
			break;
		}

		index -= num_elements;
	}
}

// ----------------------------------------------------------------------
const OStSoundAnimation*
OSrVariant_SoundAnimation_GetByIndex(
	OStVariant					*inVariant,
	UUtUns32					inIndex)
{
	OStSoundAnimation			*sound_animation;
	UUtUns32					i;
	UUtUns32					index;

	UUmAssert(inVariant);

	sound_animation = NULL;
	index = inIndex;

	// go through all of the anim types and find the one which has this index
	// then get the OStSoundAnimation from the anim type sound array
	for (i = 0; i < OScAnimType_NumTypes; i++)
	{
		UUtUns32					num_elements;

		num_elements = UUrMemory_Array_GetUsedElems(inVariant->anim_type_sounds[i]);
		if (index < num_elements)
		{
			OStSoundAnimation		*sound_array;

			sound_array =
				(OStSoundAnimation*)UUrMemory_Array_GetMemory(inVariant->anim_type_sounds[i]);
			sound_animation = &sound_array[index];

			break;
		}

		index -= num_elements;
	}

	return sound_animation;
}

// ----------------------------------------------------------------------
static void
OSiVariant_Terminate(
	OStVariant					*ioVariant)
{
	UUtUns32					i;

	UUmAssert(ioVariant);

	// delete the memory arrays
	for (i = 0; i < OScAnimType_NumTypes; i++)
	{
		UUrMemory_Array_Delete(ioVariant->anim_type_sounds[i]);
		ioVariant->anim_type_sounds[i] = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OSiVariant_UpdateImpulseName(
	OStVariant				*ioVariant,
	const char				*inOldImpulseName,
	const char				*inNewImpulseName)
{
	UUtUns32				i;

	for (i = 0; i < OScAnimType_NumTypes; i++)
	{
		UUtMemory_Array			*type_array;
		OStSoundAnimation		*sound_array;
		UUtUns32				num_elements;
		UUtUns32				j;

		type_array = ioVariant->anim_type_sounds[i];
		sound_array = (OStSoundAnimation*)UUrMemory_Array_GetMemory(type_array);
		num_elements = UUrMemory_Array_GetUsedElems(type_array);
		for (j = 0; j < num_elements; j++)
		{
			if (UUrString_Compare_NoCase(sound_array[j].impulse_name, inOldImpulseName) == 0)
			{
				if (inNewImpulseName)
				{
					UUrString_Copy(sound_array[j].impulse_name, inNewImpulseName, SScMaxNameLength);
				}
				else
				{
					UUrMemory_Clear(sound_array[j].impulse_name, SScMaxNameLength);
					sound_array[j].impulse = NULL;
				}

				if (sound_array[j].animation != NULL)
				{
					TRrAnimation_SetSoundName(
						sound_array[j].animation,
						sound_array[j].impulse_name,
						(TRtAnimTime)sound_array[j].frame);
				}
			}
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OSiSABinaryData_Load(
	const char				*inIdentifier,
	BDtData					*ioBinaryData,
	UUtBool					inSwapIt,
	UUtBool					inLocked,
	UUtBool					inAllocated)
{
	UUtError				error;
	UUtUns8					*buffer;
	UUtUns32				buffer_size;
	UUtUns8					*sa_file_data;
	UUtUns32				sa_file_data_length;
	UUtUns32				version;
	UUtUns8					*variant_data;
	UUtUns32				variant_data_length;
	char					*variant_name;
	OStVariant				*variant;


	UUmAssert(inIdentifier);
	UUmAssert(ioBinaryData);

UUrMemory_Block_VerifyList();

	// make sure the variant list is initialized
	if (OSgVariants == NULL)
	{
		error = OSrVariantList_Initialize();
		UUmError_ReturnOnError(error);
	}

	buffer = ioBinaryData->data;
	buffer_size = ioBinaryData->header.data_size;

	// find the tags
	error =
		UUrFindTagData(
			buffer,
			buffer_size,
			UUcFile_LittleEndian,
			OScSAFileTag,
			&sa_file_data,
			&sa_file_data_length);
	UUmError_ReturnOnError(error);

	// read the version number
	OBDmGet4BytesFromBuffer(sa_file_data, version, UUtUns32, inSwapIt);

	// find the variant tags
	error =
		UUrFindTagData(
			buffer,
			buffer_size,
			UUcFile_LittleEndian,
			OScVariantTag,
			&variant_data,
			&variant_data_length);
	UUmError_ReturnOnError(error);

	// read the variant name
	variant_name = (char*)variant_data;
	variant_data += ONcMaxVariantNameLength;

	// get a pointer to the variant
	variant = OSrVariantList_Variant_GetByName(variant_name);
	if (variant != NULL)
	{
		// delete all of the sound animations in the variant
		OSiVariant_DeleteAllSounds(variant);
	}

	buffer = variant_data;	/* variant_data should be at the end of the VariantTag block */
	buffer_size = ioBinaryData->header.data_size - (buffer - ioBinaryData->data);

	// process the sound animations
	// NOTE: the sound animations for a variant MUST be grouped together after
	// the OScVariantTag
	while (1)
	{
		UUtUns8			*sa_data;
		UUtUns32		sa_data_length;
		OStAnimType		anim_type;
		OStModType		mod_type;
		char			*mod_name;
		char			*base_name;
		char			*anim_name;
		const char		*impulse_name;
		UUtUns32		frame;

		// find the sound animation tag
		error =
			UUrFindTagData(
				buffer,
				buffer_size,
				UUcFile_LittleEndian,
				OScSATag,
				&sa_data,
				&sa_data_length);
		if (error != UUcError_None) { break; }

		// read the frame
		OBDmGet4BytesFromBuffer(sa_data, frame, UUtUns32, inSwapIt);

		// get the mod_type and anim_type
		mod_name = (char*)sa_data;
		sa_data += SScMaxNameLength;

		base_name = (char*)sa_data;
		sa_data += SScMaxNameLength;

		anim_type = OSrAnimType_GetByName(base_name);
		mod_type = OSrModType_GetByName(mod_name);

		// read the anim name
		anim_name = (char*)sa_data;
		sa_data += OScMaxAnimNameLength;

		// read the impulse name
		impulse_name = (char*)sa_data;
		sa_data += SScMaxNameLength;

		// get a pointer to the animation and add the sound to the variant
		OSrVariant_SoundAnimation_Add(variant, anim_type, mod_type, anim_name, frame, impulse_name);

		// move to a location in the buffer after the tag that was just processed
		buffer = sa_data;
		buffer_size = ioBinaryData->header.data_size - (buffer - ioBinaryData->data);
	}

	// dispose of allocated memory
	if (inAllocated)
	{
		UUrMemory_Block_Delete(ioBinaryData);
	}

UUrMemory_Block_VerifyList();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiSABinaryData_Register(
	void)
{
	UUtError				error;
	BDtMethods				methods;

	methods.rLoad = OSiSABinaryData_Load;

	error =	BDrRegisterClass(OScSABinaryDataClass, &methods);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OSiSABinaryData_WriteSoundAnimation(
	OStSoundAnimation				*inSoundAnimation,
	UUtUns8							*ioBuffer,
	UUtUns32						inBufferSize)
{
	UUtUns8							block[OSgMaxBufferSize + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8							*aligned_block;
	UUtUns8							*temp_buffer;
	UUtUns32						temp_buffer_size;
	UUtUns32						bytes_written;

	aligned_block = UUrAlignMemory(block);

	temp_buffer = aligned_block;
	temp_buffer_size = OSgMaxBufferSize;
	UUrMemory_Clear(temp_buffer, OSgMaxBufferSize);

	// write the impulse id
//	OBDmWrite4BytesToBuffer(temp_buffer, inSoundAnimation->impulse_id, UUtUns32, temp_buffer_size, OBJcWrite_Little);

	// write the frame
	OBDmWrite4BytesToBuffer(temp_buffer, inSoundAnimation->frame, UUtUns32, temp_buffer_size, OBJcWrite_Little);

	// write the mod type name
	UUrString_Copy(
		(char*)temp_buffer,
		OSgModTypeName[inSoundAnimation->mod_type].name,
		SScMaxNameLength);
	temp_buffer += SScMaxNameLength;
	temp_buffer_size -= SScMaxNameLength;

	// write the anim type name
	UUrString_Copy(
		(char*)temp_buffer,
		OSgAnimTypeName[inSoundAnimation->anim_type].name,
		SScMaxNameLength);
	temp_buffer += SScMaxNameLength;
	temp_buffer_size -= SScMaxNameLength;

	// write the animation name
	if (inSoundAnimation->animation)
	{
		UUrString_Copy(
			(char*)temp_buffer,
			TMrInstance_GetInstanceName(inSoundAnimation->animation),
			OScMaxAnimNameLength);
	}
	else
	{
		UUrString_Copy(
			(char*)temp_buffer,
			inSoundAnimation->anim_name,
			OScMaxAnimNameLength);
	}
	temp_buffer += SScMaxNameLength;
	temp_buffer_size -= OScMaxAnimNameLength;

	// write the impulse name
	UUrString_Copy(
		(char*)temp_buffer,
		inSoundAnimation->impulse_name,
		SScMaxNameLength);
	temp_buffer += SScMaxNameLength;
	temp_buffer_size -= SScMaxNameLength;

	// write the tag
	bytes_written =
		UUrWriteTagDataToBuffer(
			ioBuffer,
			inBufferSize,
			OScSATag,
			aligned_block,
			(OSgMaxBufferSize - temp_buffer_size),
			UUcFile_LittleEndian);

	return bytes_written;
}

// ----------------------------------------------------------------------
static UUtError
OSiSABinaryData_Save(
	UUtBool							inAutoSave)
{
	UUtUns32						i;
	OStVariant						*variant_array;
	UUtUns32						num_variants;

	// write each variant to it's own file
	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);
	num_variants = UUrMemory_Array_GetUsedElems(OSgVariants);
	for (i = 0; i < num_variants; i++)
	{
		OStVariant					*variant;

		UUtUns32					sa_file_tag_size;
		UUtUns32					variant_tag_size;
		UUtUns32					sound_animations_size;
		UUtUns8						*buffer;
		UUtUns32					buffer_size;

		UUtUns8						*write_buffer;
		UUtUns32					num_bytes;
		UUtUns32					bytes_written;

		UUtUns8						block[OSgMaxBufferSize + (2 * UUcProcessor_CacheLineSize)];
		UUtUns8						*aligned_block;
		UUtUns8						*temp_buffer;
		UUtUns32					temp_buffer_size;

		UUtUns32					j;
		OStSoundAnimation			*sound_animation_array;

		variant = &variant_array[i];

		sa_file_tag_size =
			sizeof(UUtTag) +						/* tag */
			sizeof(UUtUns32);						/* version */
		variant_tag_size =
			sizeof(UUtTag) + 						/* tag */
			ONcMaxVariantNameLength;				/* variant name */
		sound_animations_size =
			OSiVariant_CalcWriteSize(variant);		/* size of all animation sounds and their tags */

		buffer_size = sa_file_tag_size + variant_tag_size + sound_animations_size;
		buffer = (UUtUns8*)UUrMemory_Block_New(buffer_size);

		write_buffer = buffer;
		num_bytes = buffer_size;
		aligned_block = UUrAlignMemory(block);

		// ------------------------------
		// write the Sound Animation File Tag
		// ------------------------------
		temp_buffer = aligned_block;
		temp_buffer_size = OSgMaxBufferSize;
		UUrMemory_Clear(temp_buffer, OSgMaxBufferSize);

		// write the version
		OBDmWrite4BytesToBuffer(temp_buffer, OS2cCurrentVersion, UUtUns32, temp_buffer_size, OBJcWrite_Little);

		// write the tag
		bytes_written =
			UUrWriteTagDataToBuffer(
				write_buffer,
				num_bytes,
				OScSAFileTag,
				aligned_block,
				(OSgMaxBufferSize - temp_buffer_size),
				UUcFile_LittleEndian);
		write_buffer += bytes_written;
		num_bytes -= bytes_written;

		// ------------------------------
		// write the Variant File Tag
		// ------------------------------
		temp_buffer = aligned_block;
		temp_buffer_size = OSgMaxBufferSize;
		UUrMemory_Clear(temp_buffer, OSgMaxBufferSize);

		// write the variant name
		UUrString_Copy(
			(char*)temp_buffer,
			variant->character_variant->name,
			ONcMaxVariantNameLength);
		temp_buffer += ONcMaxVariantNameLength;
		temp_buffer_size -= ONcMaxVariantNameLength;

		// write the tag
		bytes_written =
			UUrWriteTagDataToBuffer(
				write_buffer,
				num_bytes,
				OScVariantTag,
				aligned_block,
				(OSgMaxBufferSize - temp_buffer_size),
				UUcFile_LittleEndian);
		write_buffer += bytes_written;
		num_bytes -= bytes_written;

		// ------------------------------
		// write the sound animations
		// ------------------------------
		for (j = 0; j < OScAnimType_NumTypes; j++)
		{
			UUtUns32					k;
			UUtMemory_Array				*type_array;

			type_array = variant->anim_type_sounds[j];

			sound_animation_array = (OStSoundAnimation*)UUrMemory_Array_GetMemory(type_array);
			for (k = 0; k < UUrMemory_Array_GetUsedElems(type_array); k++)
			{
				bytes_written =
					OSiSABinaryData_WriteSoundAnimation(
						&sound_animation_array[k],
						write_buffer,
						num_bytes);

				write_buffer += bytes_written;
				num_bytes -= bytes_written;
			}
		}

		// ------------------------------
		// write the buffer to the binary datafile
		// ------------------------------
		OBDrBinaryData_Save(
			OScSABinaryDataClass,
			variant->character_variant->name,
			buffer,
			(buffer_size - num_bytes),
			0,
			inAutoSave);

		// delete the buffer
		UUrMemory_Block_Delete(buffer);
		buffer = NULL;
	}

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static int UUcExternal_Call
OSiCompareAnimations(
	const void					*inA,
	const void					*inB)
{
	OStSoundAnimation			*a;
	OStSoundAnimation			*b;

	a = (OStSoundAnimation*)inA;
	b = (OStSoundAnimation*)inB;

	return ((int)a->animation - (int)b->animation);
}

// ----------------------------------------------------------------------
static int UUcExternal_Call
OSiCompareVariants(
	const void					*inA,
	const void					*inB)
{
	OStVariant					*a;
	OStVariant					*b;

	a = (OStVariant*)inA;
	b = (OStVariant*)inB;

	return ((int)a->character_variant - (int)b->character_variant);
}

// ----------------------------------------------------------------------
UUtUns32
OSrVariantList_GetNumVariants(
	void)
{
	return UUrMemory_Array_GetUsedElems(OSgVariants);
}

// ----------------------------------------------------------------------
UUtError
OSrVariantList_Initialize(
	void)
{
	UUtError					error;
	ONtVariantList				*variant_list;
	OStVariant					*variant_array;
	UUtUns32					i;
	UUtUns32					num_elements;

	// if OSgVariants != NULL then assume that it has already been
	// initialized properly
	if (OSgVariants != NULL) { return UUcError_None; }

	// get a pointer to the variant list
	error =
		TMrInstance_GetDataPtr(
			ONcTemplate_VariantList,
			"variant_list",
			&variant_list);
	UUmError_ReturnOnError(error);

	// make sure all of the variants except "Any" have a parent
#if defined(DEBUGGING) && (DEBUGGING)
	for (i = 0; i < variant_list->numVariants; i++)
	{
		if ((variant_list->variant[i]->parent == NULL) &&
			(UUrString_Compare_NoCase(variant_list->variant[i]->name, OScDefaultVariant) != 0))
		{
			UUmAssert(!"the variant doesn't have a parent");
		}
	}
#endif

	// allocate the memory array
	OSgVariants =
		UUrMemory_Array_New(
			sizeof(OStVariant),
			1,
			variant_list->numVariants,
			variant_list->numVariants);
	UUmError_ReturnOnNull(OSgVariants);

	// initialize the variants
	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);
	num_elements = UUrMemory_Array_GetUsedElems(OSgVariants);
	for (i = 0; i < num_elements; i++)
	{
		error = OSiVariant_Initialize(&variant_array[i], variant_list->variant[i]);
		UUmError_ReturnOnError(error);
	}

	// qsort the variant_array
	qsort(variant_array, num_elements, sizeof(OStVariant), OSiCompareVariants);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OSrVariantList_LevelLoad(
	void)
{
	OStVariant					*variant_array;
	UUtUns32					i;
	UUtUns32					num_variants;

	// go through all of the OStSoundAnimations in every variant
	// and get a pointer to the animation
	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);
	num_variants = UUrMemory_Array_GetUsedElems(OSgVariants);
	for (i = 0; i < num_variants; i++)
	{
		OStVariant				*variant;
		UUtMemory_Array			*animation_type_array;
		OStSoundAnimation		*sound_array;
		UUtUns32				j;
		UUtUns32				num_elements;
		TRtAnimation			*animation;

		variant = &variant_array[i];

		animation_type_array = variant->anim_type_sounds[OScAnimType_Animation];
		sound_array =	(OStSoundAnimation*)UUrMemory_Array_GetMemory(animation_type_array);
		num_elements = UUrMemory_Array_GetUsedElems(animation_type_array);
		for (j = 0; j < num_elements; j++)
		{
			UUtError			error;

			// get a pointer to the animation
			sound_array[j].animation = NULL;

			error =
				TMrInstance_GetDataPtr(
					TRcTemplate_Animation,
					sound_array[j].anim_name,
					&animation);
			if (error != UUcError_None) { continue; }

			sound_array[j].animation = animation;
		}

		// sort the animations
		qsort(sound_array, num_elements, sizeof(OStSoundAnimation), OSiCompareAnimations);
	}

	// This next stage must come after the qsort() above
	// go through all of the OStSoundAnimations in every variant
	// and set the sound name for the animation
	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);
	num_variants = UUrMemory_Array_GetUsedElems(OSgVariants);
	for (i = 0; i < num_variants; i++)
	{
		OStVariant				*variant;
		UUtMemory_Array			*animation_type_array;
		OStSoundAnimation		*sound_array;
		UUtUns32				j;
		UUtUns32				num_elements;

		variant = &variant_array[i];

		animation_type_array = variant->anim_type_sounds[OScAnimType_Animation];
		sound_array =	(OStSoundAnimation*)UUrMemory_Array_GetMemory(animation_type_array);
		num_elements = UUrMemory_Array_GetUsedElems(animation_type_array);
		for (j = 0; j < num_elements; j++)
		{
			if (sound_array[j].animation == NULL) { continue; }

			TRrAnimation_SetSoundName(
				sound_array[j].animation,
				sound_array[j].impulse_name,
				(UUtUns16)sound_array[j].frame);
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrVariantList_LevelUnload(
	void)
{
	UUtUns32					num_variants;
	OStVariant					*variant_array;
	UUtUns32					i;

	if (OSgVariants == NULL) { return; }

	// go through all the variants and clear tha animation sounds
	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);
	num_variants = UUrMemory_Array_GetUsedElems(OSgVariants);
	for (i = 0; i < num_variants; i++)
	{
		OStVariant				*variant;
		UUtMemory_Array			*animation_type_array;
		OStSoundAnimation		*sound_array;
		UUtUns32				j;
		UUtUns32				num_elements;

		variant = &variant_array[i];

		// clear the animation pointers
		animation_type_array = variant->anim_type_sounds[OScAnimType_Animation];
		sound_array =	(OStSoundAnimation*)UUrMemory_Array_GetMemory(animation_type_array);
		num_elements = UUrMemory_Array_GetUsedElems(animation_type_array);
		for (j = 0; j < num_elements; j++)
		{
			// set the animations impulse name to invalid
			if (sound_array[j].animation)
			{
				TRrAnimation_SetSoundName(
					sound_array[j].animation,
					NULL,
					0);
			}

			// clear the animation pointer
			sound_array[j].animation = NULL;
		}
	}
}

// ----------------------------------------------------------------------
UUtError
OSrVariantList_Save(
	UUtBool						inAutoSave)
{
	return OSiSABinaryData_Save(inAutoSave);
}

// ----------------------------------------------------------------------
void
OSrVariantList_Terminate(
	void)
{
	OStVariant					*variant_array;
	UUtUns32					num_variants;
	UUtUns32					i;

	// CB: don't terminate unless we have previously initialized
	if (OSgVariants == NULL) { return; }

	// terminate the variants
	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);
	num_variants = UUrMemory_Array_GetUsedElems(OSgVariants);
	for (i = 0; i < num_variants; i++)
	{
		OSiVariant_Terminate(&variant_array[i]);
	}

	UUrMemory_Array_Delete(OSgVariants);
	OSgVariants = NULL;
}

// ----------------------------------------------------------------------
OStVariant*
OSrVariantList_Variant_GetByIndex(
	UUtUns32					inIndex)
{
	OStVariant					*variant_array;

	// check the range
	if (inIndex >= UUrMemory_Array_GetUsedElems(OSgVariants)) { return NULL; }

	// get a pointer to the variant array
	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);

	return &variant_array[inIndex];
}

// ----------------------------------------------------------------------
OStVariant*
OSrVariantList_Variant_GetByName(
	const char					*inVariantName)
{
	OStVariant					*out_variant;
	OStVariant					*variant_array;
	UUtUns32					num_variants;
	UUtUns32					i;

	UUmAssert(inVariantName);

	out_variant = NULL;

	// find the variant with the name inVariantName
	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);
	num_variants = UUrMemory_Array_GetUsedElems(OSgVariants);
	for (i = 0; i < num_variants; i++)
	{
		if (UUrString_Compare_NoCase(variant_array[i].character_variant->name, inVariantName) == 0)
		{
			out_variant = &variant_array[i];
			break;
		}
	}

	return out_variant;
}

// ----------------------------------------------------------------------
OStVariant*
OSrVariantList_Variant_GetByVariant(
	const ONtCharacterVariant	*inCharacterVariant)
{
	OStVariant					*variant_array;
	OStVariant					find_me;
	OStVariant					*out_variant;
	UUtUns32					num_elements;

	UUmAssert(inCharacterVariant);

	// get a pointer to the variant array and the number of elements in the array
	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);
	num_elements = UUrMemory_Array_GetUsedElems(OSgVariants);

	// set the find parameter
	find_me.character_variant = inCharacterVariant;

	// find the variant
	out_variant =
		(OStVariant*)bsearch(
			&find_me,
			variant_array,
			num_elements,
			sizeof(OStVariant),
			OSiCompareVariants);

	return out_variant;
}

// ----------------------------------------------------------------------
void
OSrVariantList_UpdateImpulseName(
	const char					*inOldImpulseName,
	const char					*inNewImpulseName)
{
	OStVariant					*variant_array;
	UUtUns32					num_variants;
	UUtUns32					i;

	// get a pointer to the variant array and the number of elements in the array
	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);
	num_variants = UUrMemory_Array_GetUsedElems(OSgVariants);
	for (i = 0; i < num_variants; i++)
	{
		OSiVariant_UpdateImpulseName(&variant_array[i], inOldImpulseName, inNewImpulseName);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
const char*
OSrAnimType_GetName(
	OStAnimType					inAnimType)
{
	UUmAssert(inAnimType < OScAnimType_NumTypes);
	UUmAssert(OSgAnimTypeName[inAnimType].type == inAnimType);

	return OSgAnimTypeName[inAnimType].name;
}

// ----------------------------------------------------------------------
OStAnimType
OSrAnimType_GetByName(
	const char					*inAnimTypeName)
{
	OStTypeName					*atn;
	OStAnimType					anim_type;
	UUtInt32					result;

	anim_type = OScAnimType_Any;

	for (atn = OSgAnimTypeName; atn->name != NULL; atn++)
	{
		result = UUrString_Compare_NoCase(atn->name, inAnimTypeName);
		if (result == 0)
		{
			anim_type = atn->type;
			break;
		}
	}

	return anim_type;
}

// ----------------------------------------------------------------------
const char*
OSrModType_GetName(
	OStModType					inModType)
{
	UUmAssert(inModType < OScModType_NumTypes);
	UUmAssert(OSgModTypeName[inModType].type == inModType);

	return OSgModTypeName[inModType].name;
}

// ----------------------------------------------------------------------
OStModType
OSrModType_GetByName(
	const char					*inModTypeName)
{
	OStTypeName					*mtn;
	OStAnimType					mod_type;
	UUtInt32					result;

	mod_type = OScModType_Any;

	for (mtn = OSgModTypeName; mtn->name != NULL; mtn++)
	{
		result = UUrString_Compare_NoCase(mtn->name, inModTypeName);
		if (result == 0)
		{
			mod_type = mtn->type;
			break;
		}
	}

	return mod_type;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OSrSA_Initialize(
	void)
{
	UUtError					error;

	// initialize the binary data class
	error = OSiSABinaryData_Register();
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrSA_ListBrokenSounds(
	BFtFile						*inFile)
{
	OStVariant					*variant_array;
	UUtUns32					i;
	UUtUns32					num_variants;
	char						text[2048];

	// printf a header
	BFrFile_Printf(inFile, "********** Animation Sound Links **********"UUmNL);
	BFrFile_Printf(inFile, "Anim Type\tModifier\tImpulse Sound Name"UUmNL);

	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);
	num_variants = UUrMemory_Array_GetUsedElems(OSgVariants);
	for (i = 0; i < num_variants; i++)
	{
		OStVariant				*variant;
		UUtMemory_Array			*animation_type_array;
		OStSoundAnimation		*sound_array;
		UUtUns32				j;
		UUtUns32				num_elements;
		UUtUns32				itr2;

		variant = &variant_array[i];

		for (itr2 = 0; itr2 < OScAnimType_NumTypes; itr2++)
		{
			animation_type_array = variant->anim_type_sounds[itr2];
			sound_array = (OStSoundAnimation*)UUrMemory_Array_GetMemory(animation_type_array);
			num_elements = UUrMemory_Array_GetUsedElems(animation_type_array);
			for (j = 0; j < num_elements; j++)
			{
				SStImpulse			*impulse;

				impulse = OSrImpulse_GetByName(sound_array[j].impulse_name);
				if (impulse != NULL) { continue; }

				sprintf(
					text,
					"%s\t%s\t%s",
					OSgAnimTypeName[sound_array[j].anim_type],
					OSgModTypeName[sound_array[j].mod_type],
					sound_array[j].impulse_name);
				COrConsole_Printf(text);
				BFrFile_Printf(inFile, "%s"UUmNL, text);
			}
		}
	}

	BFrFile_Printf(inFile, UUmNL);
	BFrFile_Printf(inFile, UUmNL);
	BFrFile_Printf(inFile, UUmNL);
}

// ----------------------------------------------------------------------
void
OSrSA_UpdatePointers(
	void)
{
	OStVariant					*variant_array;
	UUtUns32					num_variants;
	UUtUns32					itr;

	variant_array = (OStVariant*)UUrMemory_Array_GetMemory(OSgVariants);
	num_variants = UUrMemory_Array_GetUsedElems(OSgVariants);
	for (itr = 0; itr < num_variants; itr++)
	{
		OStVariant				*variant;
		UUtMemory_Array			*animation_type_array;
		OStSoundAnimation		*sound_array;
		UUtUns32				itr2;
		UUtUns32				num_elements;

		variant = &variant_array[itr];

		for (itr2 = 0; itr2 < OScAnimType_NumTypes; itr2++)
		{
			UUtUns32			itr3;

			animation_type_array = variant->anim_type_sounds[itr2];
			sound_array = (OStSoundAnimation*)UUrMemory_Array_GetMemory(animation_type_array);
			num_elements = UUrMemory_Array_GetUsedElems(animation_type_array);
			for (itr3 = 0; itr3 < num_elements; itr3++)
			{
				sound_array[itr3].impulse = OSrImpulse_GetByName(sound_array[itr3].impulse_name);
				sound_array[itr3].animation =
					TMrInstance_GetFromName(
						TRcTemplate_Animation,
						sound_array[itr3].anim_name);
			}
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OSrSoundAnimation_Play(
	const ONtCharacterVariant	*inCharacterVariant,
	const TRtAnimation			*inAnimation,
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inDirection,
	const M3tVector3D			*inVelocity)
{
	OStVariant					*variant;
	TRtAnimType					totoro_anim_type;
	OStAnimType					sound_anim_type;
	OStModType					sound_mod_type;
	SStImpulse					*impulse = NULL;
	UUtBool						has_sound;

	UUmAssert(inCharacterVariant);
	UUmAssert(inPosition);
//	UUmAssert(inDirection);
//	UUmAssert(inVelocity);

	// get the anim type
	totoro_anim_type = TRrAnimation_GetType(inAnimation);
	UUmAssert(OSiTotoroToSound(totoro_anim_type)->totoro_anim_type == totoro_anim_type);
	sound_anim_type = OSiTotoroToSound(totoro_anim_type)->sound_anim_type;
	sound_mod_type = OSiTotoroToSound(totoro_anim_type)->sound_mod_type;
	has_sound = (TRrAnimation_GetSoundName(inAnimation) != NULL);
	if ((sound_anim_type == OScAnimType_None) && (has_sound == UUcFalse)) { return; }

	// get a mod type based on damage
	if (sound_mod_type == OScModType_Any)
	{
		UUtUns32				damage;

		damage = TRrAnimation_GetMaximumDamage(inAnimation);

		if (damage >= ONcAnimDamage_Heavy)
		{
			sound_mod_type = OScModType_HeavyDamage;
		}
		else if (damage >= ONcAnimDamage_Medium)
		{
			sound_mod_type = OScModType_MediumDamage;
		}
		else if (damage > ONcAnimDamage_None)
		{
			sound_mod_type = OScModType_LightDamage;
		}
	}

	// get a pointer to the variant
	variant = OSrVariantList_Variant_GetByVariant(inCharacterVariant);
	while (variant != NULL)
	{
		// get the impulse sound from the variant
		impulse = OSiVariant_Impulse_Get(variant, inAnimation, sound_anim_type, sound_mod_type);
		if (impulse != NULL) { break; }

		// get the parent variant
		if (variant->character_variant->parent == NULL) { break; }
		variant = OSrVariantList_Variant_GetByVariant(variant->character_variant->parent);
		UUmAssert(variant);
	}

	// play the impulse sound
	if (impulse != NULL)
	{
		OSrImpulse_Play(impulse, inPosition, inDirection, inVelocity, NULL);
	}
}
