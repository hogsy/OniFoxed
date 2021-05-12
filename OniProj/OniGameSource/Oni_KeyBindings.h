#pragma once
#ifndef __ONI_KEYBINDINGS_H__
#define __ONI_KEYBINDINGS_H__

/*
	FILE:	Oni_KeyBindings.h
	
	AUTHOR:	Michael Evans
	
	CREATED: January 26, 2000
	
	PURPOSE:
	
	Copyright 2000

*/

#include "BFW.h"

typedef enum ONtDebuggingKey
{
	ONcDebugKey_Occl,
	ONcDebugKey_Invis,
	ONcDebugKey_Perf_Overall,
	ONcDebugKey_Character_Collision,
	ONcDebugKey_Object_Collision,
	ONcDebugKey_FastMode,
	ONcDebugKey_DrawEveryFrame,

	ONcDebugKey_Secret_X,
	ONcDebugKey_Secret_Y,
	ONcDebugKey_Secret_Z,

	ONcDebugKey_AddFlag,
	ONcDebugKey_DeleteFlag,
	ONcDebugKey_Unstick,

	ONcDebugKey_Camera_Record,
	ONcDebugKey_Camera_Stop,
	ONcDebugKey_Camera_Play,

	ONcDebugKey_Place_Quad,
	ONcDebugKey_Place_Quad_Mode,

	ONcDebugKey_ProfileToggle,
	ONcDebugKey_RecordScreen,

	ONcDebugKey_Test_One,
	ONcDebugKey_Test_Two,
	ONcDebugKey_Test_Three,
	ONcDebugKey_Test_Four,
	ONcDebugKey_Camera_Mode,

	ONcDebugKey_SingleStep,
	ONcDebugKey_ToggleActiveCamera,

	ONcDebugKey_SingleStepMode,
	
	ONcDebugKey_CharToCamera,

	ONcDebugKey_KillParticles,

	ONcDebugKey_AIBreakPoint,

	ONcDebugKey_ExplodeOne,
	ONcDebugKey_ExplodeTwo,
	ONcDebugKey_ExplodeThree,
	ONcDebugKey_ExplodeFour,

	ONcDebugKey_ResetParticles,

	ONcDebugKey_DropFlagAndAddWaypoint,
	ONcDebugKey_ShowMeleeStatus,
	ONcDebugKey_Freeze,
	ONcDebugKey_FrameAdvance,
	ONcDebugKey_SoundOccl,

	ONcDebugKey_ChangeCharacters,
	ONcDebugKey_Perf_Particle,
	ONcDebugKey_Perf_Particle_Lock,

	ONcDebugKey_Num,
	ONcDebugKey_None
	
} ONtDebuggingKey;

UUtBool ONrDebugKey_WentDown(ONtDebuggingKey inKey);


#endif // __ONI_KEYBINDINGS_H__