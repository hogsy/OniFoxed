#pragma once
#ifndef __ONI_CHARACTER_ANIMATION_H__
#define __ONI_CHARACTER_ANIMATION_H__


/*	FILE:	Oni_Character_Animation.h
	
	AUTHOR:	Michael Evans
	
	CREATED: January 8, 1998
	
	PURPOSE: animation types for oni
	
	Copyright 1998 - 1999

*/

#include "BFW.h"
#include "BFW_Totoro.h"

// must have same order as the _ins.txt file
enum
{
	ONcAnimType_None,						// 0
	ONcAnimType_Anything,
	ONcAnimType_Walk,
	ONcAnimType_Run,
	ONcAnimType_Slide,

	ONcAnimType_Jump,						// 5
	ONcAnimType_Stand,						
	ONcAnimType_Standing_Turn_Left,
	ONcAnimType_Standing_Turn_Right,
	ONcAnimType_Run_Backwards,

	ONcAnimType_Run_Sidestep_Left,			// 10
	ONcAnimType_Run_Sidestep_Right,
	ONcAnimType_Kick,
	ONcAnimType_Walk_Sidestep_Left,
	ONcAnimType_Walk_Sidestep_Right,

	ONcAnimType_Walk_Backwards,				// 15
	ONcAnimType_Stance,
	ONcAnimType_Crouch,
	ONcAnimType_Jump_Forward,
	ONcAnimType_Jump_Backward,
	
	ONcAnimType_Jump_Left,					// 20
	ONcAnimType_Jump_Right,
	ONcAnimType_Punch,
	ONcAnimType_Block,
	ONcAnimType_Land,
	
	ONcAnimType_Fly,						// 25
	ONcAnimType_Kick_Forward,
	ONcAnimType_Kick_Left,
	ONcAnimType_Kick_Right,
	ONcAnimType_Kick_Back,
	ONcAnimType_Kick_Low,					// 30

	ONcAnimType_Punch_Forward,
	ONcAnimType_Punch_Left,
	ONcAnimType_Punch_Right,
	ONcAnimType_Punch_Back,
	ONcAnimType_Punch_Low,					// 35

	ONcAnimType_Kick2,
	ONcAnimType_Kick3,

	ONcAnimType_Punch2,
	ONcAnimType_Punch3,

	ONcAnimType_Land_Forward,				// 40
	ONcAnimType_Land_Right,
	ONcAnimType_Land_Left,
	ONcAnimType_Land_Back,

	ONcAnimType_PPK,
	ONcAnimType_PKK,						// 45
	ONcAnimType_PKP,

	ONcAnimType_KPK,
	ONcAnimType_KPP,
	ONcAnimType_KKP,

	ONcAnimType_PK,							// 50
	ONcAnimType_KP,

	ONcAnimType_Punch_Heavy,
	ONcAnimType_Kick_Heavy,
	ONcAnimType_Punch_Forward_Heavy,
	ONcAnimType_Kick_Forward_Heavy,			// 55

	ONcAnimType_Aiming_Overlay,				
	ONcAnimType_Hit_Overlay,				

	ONcAnimType_Crouch_Run,				
	ONcAnimType_Crouch_Walk,

	ONcAnimType_Crouch_Run_Backwards,		// 60
	ONcAnimType_Crouch_Walk_Backwards,

	ONcAnimType_Crouch_Run_Sidestep_Left,
	ONcAnimType_Crouch_Run_Sidestep_Right,

	ONcAnimType_Crouch_Walk_Sidestep_Left,
	ONcAnimType_Crouch_Walk_Sidestep_Right,	// 65

	ONcAnimType_Run_Kick,
	ONcAnimType_Run_Punch,
	
	ONcAnimType_Run_Back_Punch,
	ONcAnimType_Run_Back_Kick,

	ONcAnimType_Sidestep_Left_Kick,			// 70
	ONcAnimType_Sidestep_Left_Punch,

	ONcAnimType_Sidestep_Right_Kick,
	ONcAnimType_Sidestep_Right_Punch,

	ONcAnimType_Prone,
	ONcAnimType_Flip,						// 75

	ONcAnimType_Hit_Head,
	ONcAnimType_Hit_Body,
	ONcAnimType_Hit_Foot,

	ONcAnimType_Knockdown_Head,
	ONcAnimType_Knockdown_Body,				// 80
	ONcAnimType_Knockdown_Foot,

	ONcAnimType_Hit_Crouch,
	ONcAnimType_Knockdown_Crouch,

	ONcAnimType_Hit_Fallen,

	ONcAnimType_Hit_Head_Behind,			// 85
	ONcAnimType_Hit_Body_Behind,
	ONcAnimType_Hit_Foot_Behind,

	ONcAnimType_Knockdown_Head_Behind,
	ONcAnimType_Knockdown_Body_Behind,
	ONcAnimType_Knockdown_Foot_Behind,		// 90

	ONcAnimType_Hit_Crouch_Behind,
	ONcAnimType_Knockdown_Crouch_Behind,

	ONcAnimType_Idle,
	ONcAnimType_Taunt,

	ONcAnimType_Throw,

	ONcAnimType_Thrown1,
	ONcAnimType_Thrown2,
	ONcAnimType_Thrown3,
	ONcAnimType_Thrown4,
	ONcAnimType_Thrown5,
	ONcAnimType_Thrown6,

	ONcAnimType_Special1,
	ONcAnimType_Special2,
	ONcAnimType_Special3,
	ONcAnimType_Special4,

	ONcAnimType_Throw_Forward_Punch,
	ONcAnimType_Throw_Forward_Kick,
	ONcAnimType_Throw_Behind_Punch,
	ONcAnimType_Throw_Behind_Kick,

	ONcAnimType_Run_Throw_Forward_Punch,
	ONcAnimType_Run_Throw_Behind_Punch,
	ONcAnimType_Run_Throw_Forward_Kick,
	ONcAnimType_Run_Throw_Behind_Kick,

	ONcAnimType_Thrown7,
	ONcAnimType_Thrown8,
	ONcAnimType_Thrown9,
	ONcAnimType_Thrown10,
	ONcAnimType_Thrown11,
	ONcAnimType_Thrown12,

	ONcAnimType_Startle_Left_Unused,
	ONcAnimType_Startle_Right_Unused,

	ONcAnimType_Sit,
	ONcAnimType_Stand_Special,
	ONcAnimType_Act,
	ONcAnimType_Kick3_Forward,

	ONcAnimType_Hit_Foot_Ouch,
	ONcAnimType_Hit_Jewels,
	ONcAnimType_Thrown13,
	ONcAnimType_Thrown14,
	ONcAnimType_Thrown15,
	ONcAnimType_Thrown16,
	ONcAnimType_Thrown17,

	ONcAnimType_PPKK,
	ONcAnimType_PPKKK,
	ONcAnimType_PPKKKK,

	ONcAnimType_Land_Hard,
	ONcAnimType_Land_Forward_Hard,
	ONcAnimType_Land_Right_Hard,
	ONcAnimType_Land_Left_Hard,
	ONcAnimType_Land_Back_Hard,

	ONcAnimType_Land_Dead,
	ONcAnimType_Crouch_Turn_Left,
	ONcAnimType_Crouch_Turn_Right,

	ONcAnimType_Crouch_Forward,
	ONcAnimType_Crouch_Back,
	ONcAnimType_Crouch_Left,
	ONcAnimType_Crouch_Right,

	ONcAnimType_Getup_Kick_Back,

	ONcAnimType_Autopistol_Recoil,
	ONcAnimType_Phase_Rifle_Recoil,
	ONcAnimType_Phase_Stream_Recoil,
	ONcAnimType_Superball_Recoil,
	ONcAnimType_Vandegraf_Recoil,
	ONcAnimType_Scram_Cannon_Recoil,
	ONcAnimType_Mercury_Bow_Recoil,
	ONcAnimType_Screamer_Recoil,

	ONcAnimType_Pickup_Object,
	ONcAnimType_Pickup_Pistol,
	ONcAnimType_Pickup_Rifle,

	ONcAnimType_Holster,
	ONcAnimType_Draw_Pistol,
	ONcAnimType_Draw_Rifle,

	ONcAnimType_Punch4,

	ONcAnimType_Reload_Pistol,
	ONcAnimType_Reload_Rifle,
	ONcAnimType_Reload_Stream,
	ONcAnimType_Reload_Superball,
	ONcAnimType_Reload_Vandegraf,
	ONcAnimType_Reload_Scram_Cannon,
	ONcAnimType_Reload_MercuryBow,
	ONcAnimType_Reload_Screamer,

	ONcAnimType_PF_PF,
	ONcAnimType_PF_PF_PF,

	ONcAnimType_PL_PL,
	ONcAnimType_PL_PL_PL,
	
	ONcAnimType_PR_PR,
	ONcAnimType_PR_PR_PR,
	
	ONcAnimType_PB_PB,
	ONcAnimType_PB_PB_PB,

	ONcAnimType_PD_PD,
	ONcAnimType_PD_PD_PD,

	ONcAnimType_KF_KF,
	ONcAnimType_KF_KF_KF,

	ONcAnimType_KL_KL,
	ONcAnimType_KL_KL_KL,
	
	ONcAnimType_KR_KR,
	ONcAnimType_KR_KR_KR,
	
	ONcAnimType_KB_KB,
	ONcAnimType_KB_KB_KB,

	ONcAnimType_KD_KD,
	ONcAnimType_KD_KD_KD,

	ONcAnimType_Startle_Left,
	ONcAnimType_Startle_Right,
	ONcAnimType_Startle_Back,
	ONcAnimType_Startle_Forward,
	
	ONcAnimType_Console,					// use a console
	ONcAnimType_Console_Walk,				// walk up to console
	
	ONcAnimType_Stagger,

	ONcAnimType_Watch,

	ONcAnimType_Act_No,
	ONcAnimType_Act_Yes,
	ONcAnimType_Act_Talk,
	ONcAnimType_Act_Shrug,
	ONcAnimType_Act_Shout,
	ONcAnimType_Act_Give,

	ONcAnimType_Run_Stop,
	ONcAnimType_Walk_Stop,

	ONcAnimType_Run_Start,
	ONcAnimType_Walk_Start,

	ONcAnimType_Run_Backwards_Start,
	ONcAnimType_Walk_Backwards_Start,

	ONcAnimType_Stun,
	ONcAnimType_Stagger_Behind,
	ONcAnimType_Blownup,
	ONcAnimType_Blownup_Behind,

	ONcAnimType_1Step_Stop,
	ONcAnimType_Run_Sidestep_Left_Start,
	ONcAnimType_Run_Sidestep_Right_Start,

	ONcAnimType_Powerup,
	ONcAnimType_Falling_Flail,

	ONcAnimType_Console_Punch,

	ONcAnimType_Teleport_In,
	ONcAnimType_Teleport_Out,
	ONcAnimType_Ninja_Fireball,
	ONcAnimType_Ninja_Invisible,
	ONcAnimType_Punch_Rifle,

	ONcAnimType_Pickup_Object_Mid,
	ONcAnimType_Pickup_Pistol_Mid,
	ONcAnimType_Pickup_Rifle_Mid,

	ONcAnimType_Hail,
	ONcAnimType_Muro_Thunderbolt,

	ONcAnimType_Hit_Overlay_AI,

	ONcAnimType_Num							// number of animations
};

// must have same order as the _ins.txt file
enum
{
	ONcAnimState_None,							// 0
	ONcAnimState_Anything,
	ONcAnimState_Running_Left_Down,
	ONcAnimState_Running_Right_Down,
	ONcAnimState_Sliding,

	ONcAnimState_Walking_Left_Down,				// 5
	ONcAnimState_Walking_Right_Down,
	ONcAnimState_Standing,
	ONcAnimState_Run_Start,
	ONcAnimState_Run_Accel,

	ONcAnimState_Run_Sidestep_Left,				// 10
	ONcAnimState_Run_Sidestep_Right,
	ONcAnimState_Run_Slide,
	ONcAnimState_Run_Jump,
	ONcAnimState_Run_Jump_Land,

	ONcAnimState_Run_Back_Start,				// 15
	ONcAnimState_Running_Back_Right_Down,
	ONcAnimState_Running_Back_Left_Down,
	ONcAnimState_Fallen_Back,
	ONcAnimState_Crouch,

	ONcAnimState_Running_Upstairs_Right_Down,	// 20
	ONcAnimState_Running_Upstairs_Left_Down,
	ONcAnimState_Sidestep_Left_Left_Down,
	ONcAnimState_Sidestep_Left_Right_Down,
	ONcAnimState_Sidestep_Right_Left_Down,

	ONcAnimState_Sidestep_Right_Right_Down,		// 25
	ONcAnimState_Sidestep_Right_Jump,
	ONcAnimState_Sidestep_Left_Jump,
	ONcAnimState_Jump_Forward,
	ONcAnimState_Jump_Up,

	ONcAnimState_Run_Back_Slide,				// 30
	ONcAnimState_Lie_Back,
	ONcAnimState_Sidestep_Left_Start,
	ONcAnimState_Sidestep_Right_Start,
	ONcAnimState_Walking_Sidestep_Left,

	ONcAnimState_Crouch_Walk,					// 35
	ONcAnimState_Walking_Sidestep_Right,
	ONcAnimState_Flying,
	ONcAnimState_Falling,
	ONcAnimState_Flying_Forward,

	ONcAnimState_Falling_Forward,				// 40
	ONcAnimState_Flying_Back,
	ONcAnimState_Falling_Back,
	ONcAnimState_Flying_Left,
	ONcAnimState_Falling_Left,

	ONcAnimState_Flying_Right,					// 45
	ONcAnimState_Falling_Right,
	ONcAnimState_Crouch_Start,
	ONcAnimState_Walking_Back_Left_Down,
	ONcAnimState_Walking_Back_Right_Down,

	ONcAnimState_Fallen_Front,					// 50
	ONcAnimState_Sidestep_Left_Start_Long_Name,
	ONcAnimState_Sidestep_Right_Start_Long_Name,
	ONcAnimState_Sit,
	ONcAnimState_Punch_Low,

	ONcAnimState_Stand_Special,					// 55
	ONcAnimState_Acting,

	ONcAnimState_Crouch_Run_Left,
	ONcAnimState_Crouch_Run_Right,
	ONcAnimState_Crouch_Run_Back_Left,
	ONcAnimState_Crouch_Run_Back_Right,			// 60

	ONcAnimState_Blocking1,
	ONcAnimState_Blocking2,
	ONcAnimState_Blocking3,
	ONcAnimState_Crouch_Blocking1,

	ONcAnimState_Gliding,						// 65
	ONcAnimState_Watch_Idle,
	ONcAnimState_Stunned,
	ONcAnimState_Powerup,
	ONcAnimState_Thunderbolt,

	ONcAnimState_Max
};

enum
{
	ONcAnimVarientMask_None					= 0x0000,
	ONcAnimVarientMask_Panic				= 0x8000,
	ONcAnimVarientMask_Lefty_Rifle			= 0x4000,
	ONcAnimVarientMask_Righty_Rifle			= 0x2000,
	ONcAnimVarientMask_Lefty_Pistol			= 0x1000,
	ONcAnimVarientMask_Righty_Pistol		= 0x0800,
	ONcAnimVarientMask_Shoulder				= 0x0400,
	ONcAnimVarientMask_Fight				= 0x0200,
	ONcAnimVarientMask_Sprint				= 0x0100
};


#define ONcAnimVarientMask_Any_Weapon	(ONcAnimVarientMask_Lefty_Rifle | ONcAnimVarientMask_Righty_Rifle | ONcAnimVarientMask_Lefty_Pistol | ONcAnimVarientMask_Righty_Pistol)
#define ONcAnimVarientMask_Any_Rifle	(ONcAnimVarientMask_Lefty_Rifle | ONcAnimVarientMask_Righty_Rifle)
#define ONcAnimVarientMask_Any_Pistol	(ONcAnimVarientMask_Lefty_Pistol | ONcAnimVarientMask_Righty_Pistol)

// CB: ONcAnimFlag_Aim_* are now deprecated because the melee system needs to know about
// this in greater detail and it doesn't make sense to replicate the same info in
// two places. they are read in from the INS file, but then used to set an animation's
// moveDirection and zeroed
enum 
{
	ONcAnimFlag_Invulnerable = 
			TRcAnimFlag_FirstPublic,	// mask = 0x00000002

	ONcAnimFlag_BlockHigh,				// mask = 0x00000004
	ONcAnimFlag_BlockLow,				// mask = 0x00000008
	ONcAnimFlag_Attack,					// mask = 0x00000010
	ONcAnimFlag_DropWeapon,				// mask = 0x00000020

	ONcAnimFlag_InAir,					// mask = 0x00000040
	ONcAnimFlag_Atomic,					// mask = 0x00000080
	ONcAnimFlag_NoTurn,					// mask = 0x00000100

	ONcAnimFlag_Aim_Forward,			// mask = 0x00000200
	ONcAnimFlag_Aim_Left,				// mask = 0x00000400
	ONcAnimFlag_Aim_Right,				// mask = 0x00000800
	ONcAnimFlag_Aim_Backward,			// mask = 0x00001000

	ONcAnimFlag_Overlay,				// mask = 0x00002000
	ONcAnimFlag_NoInterpVelocity,		// mask = 0x00004000
	ONcAnimFlag_ThrowSource,			// mask = 0x00008000
	ONcAnimFlag_ThrowTarget,			// mask = 0x00010000
	ONcAnimFlag_RealWorld,				// mask = 0x00020000

	ONcAnimFlag_DoAim,					// mask = 0x00040000
	ONcAnimFlag_DontAim,				// mask = 0x00080000
	ONcAnimFlag_Can_Pickup,				// mask = 0x00100000
	ONcAnimFlag_Aim_360,				// mask = 0x00200000
	ONcAnimFlag_DisableShield,			// mask = 0x00400000
	ONcAnimFlag_NoAIPickup				// mask = 0x00800000
};

enum
{
	ONcShortcutFlag_Nuke
};

enum
{
	ONcAttackFlag_Unblockable,
	ONcAttackFlag_AttackHigh,
	ONcAttackFlag_AttackLow,
	ONcAttackFlag_SpecialMove
};

// animation damage levels
enum
{
	ONcAnimDamage_None					= 0,
	ONcAnimDamage_Light					= 1,		// CB: this is now unused, all values between ONcAnimDamage_None and ONcAnimDamage_Medium are light
	ONcAnimDamage_Medium				= 10,
	ONcAnimDamage_Heavy					= 15
};

typedef struct ONtCharacterAnimationImpactType {
	const char *name;
	UUtUns32 type;
} ONtCharacterAnimationImpactType;

extern const ONtCharacterAnimationImpactType ONcAnimImpactTypes[];

#endif // __ONI_CHARACTER_ANIMATION_H__