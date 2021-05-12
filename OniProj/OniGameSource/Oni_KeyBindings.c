/*
	FILE:	Oni_KeyBindings.c
	
	AUTHOR:	Michael Evans
	
	CREATED: January 26, 2000
	
	PURPOSE:
	
	Copyright 2000

*/

#include "BFW.h"
#include "BFW_LocalInput.h"

#include "Oni_KeyBindings.h"
#include "Oni_GameState.h"

static UUtBool gDebugKeyIsDown[ONcDebugKey_Num] = { UUcFalse };

typedef enum ONtDebugKeyModifiers
{
	ONcDebugKey_Needs_Modifier,
	ONcDebugKey_Without_Modifier
	
} ONtDebugKeyModifiers;

typedef enum ONtDebugKeyWhen
{
	ONcDebugKey_Tool,
	ONcDebugKey_Game
} ONtDebugKeyWhen;

typedef struct ONtDebugKeyBinding
{
	ONtDebuggingKey			binding;
	ONtDebugKeyWhen			when;
	ONtDebugKeyModifiers	modifiers;
	LItKeyCode				keyCode1;
	LItKeyCode				keyCode2;
} ONtDebugKeyBinding;



const ONtDebugKeyBinding bindings[ONcDebugKey_Num] =
{
	{	ONcDebugKey_Occl,					ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_O	},		
	{	ONcDebugKey_Invis,					ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_I	},
	{	ONcDebugKey_Perf_Overall,			ONcDebugKey_Game,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_Y	},	
	{	ONcDebugKey_Character_Collision,	ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_T	},
	{	ONcDebugKey_Object_Collision,		ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_R	},
	{	ONcDebugKey_FastMode,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_F	},
	{	ONcDebugKey_DrawEveryFrame,			ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_G	},
	{	ONcDebugKey_Secret_X,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_X	},
	{	ONcDebugKey_Secret_Y,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_Y	},
	{	ONcDebugKey_Secret_Z,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_Z	},
	{	ONcDebugKey_AddFlag,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_Insert	},
	{	ONcDebugKey_DeleteFlag,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_Delete	},
	{	ONcDebugKey_Unstick,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_U	},
	{	ONcDebugKey_Camera_Record,			ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_N	},
	{	ONcDebugKey_Camera_Stop,			ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_M	},
	{	ONcDebugKey_Camera_Play,			ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_Comma	},
	{	ONcDebugKey_Place_Quad,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_None	},
	{	ONcDebugKey_Place_Quad_Mode,		ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_None	},
	{	ONcDebugKey_ProfileToggle,			ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_BackSlash	},
	{	ONcDebugKey_RecordScreen,			ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_L	},
	{	ONcDebugKey_Test_One,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_1 },
	{	ONcDebugKey_Test_Two,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_2 },
	{	ONcDebugKey_Test_Three,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_3 },
	{	ONcDebugKey_Test_Four,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_4 },
	{	ONcDebugKey_Camera_Mode,			ONcDebugKey_Tool,	ONcDebugKey_Without_Modifier,	LIcKeyCode_NumPadEnter,	LIcKeyCode_Return },
	{	ONcDebugKey_SingleStep,				ONcDebugKey_Tool,	ONcDebugKey_Without_Modifier,	LIcKeyCode_None,		LIcKeyCode_Slash },
	{	ONcDebugKey_ToggleActiveCamera,		ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_Home },
	{	ONcDebugKey_SingleStepMode,			ONcDebugKey_Tool,	ONcDebugKey_Without_Modifier,	LIcKeyCode_None,		LIcKeyCode_Period },
	{	ONcDebugKey_CharToCamera,			ONcDebugKey_Tool,	ONcDebugKey_Without_Modifier,	LIcKeyCode_None,		LIcKeyCode_End },
	{	ONcDebugKey_KillParticles,			ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_K },
	{	ONcDebugKey_AIBreakPoint,			ONcDebugKey_Tool,	ONcDebugKey_Without_Modifier,	LIcKeyCode_None,		LIcKeyCode_BackSpace },
	{	ONcDebugKey_ExplodeOne,				ONcDebugKey_Tool,	ONcDebugKey_Without_Modifier,	LIcKeyCode_None,		LIcKeyCode_7 },
	{	ONcDebugKey_ExplodeTwo,				ONcDebugKey_Tool,	ONcDebugKey_Without_Modifier,	LIcKeyCode_None,		LIcKeyCode_8 },
	{	ONcDebugKey_ExplodeThree,			ONcDebugKey_Tool,	ONcDebugKey_Without_Modifier,	LIcKeyCode_None,		LIcKeyCode_9 },
	{	ONcDebugKey_ExplodeFour,			ONcDebugKey_Tool,	ONcDebugKey_Without_Modifier,	LIcKeyCode_None,		LIcKeyCode_0 },
	{	ONcDebugKey_ResetParticles,			ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_J },
	{	ONcDebugKey_DropFlagAndAddWaypoint,	ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_W },
	{	ONcDebugKey_ShowMeleeStatus,		ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_B },
	{	ONcDebugKey_Freeze,					ONcDebugKey_Tool,	ONcDebugKey_Without_Modifier,	LIcKeyCode_None,		LIcKeyCode_RightBracket },
	{	ONcDebugKey_FrameAdvance,			ONcDebugKey_Tool,	ONcDebugKey_Without_Modifier,	LIcKeyCode_None,		LIcKeyCode_LeftBracket },
	{	ONcDebugKey_SoundOccl,				ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_S },
	{	ONcDebugKey_ChangeCharacters,		ONcDebugKey_Game,	ONcDebugKey_Without_Modifier,	LIcKeyCode_None,		LIcKeyCode_F8 },
	{	ONcDebugKey_Perf_Particle,			ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_P },
	{	ONcDebugKey_Perf_Particle_Lock,		ONcDebugKey_Tool,	ONcDebugKey_Needs_Modifier,		LIcKeyCode_None,		LIcKeyCode_Semicolon }
};

UUtBool ONrDebugKey_WentDown(ONtDebuggingKey inKey)
{
	UUtBool		needs_modifier = bindings[inKey].modifiers == ONcDebugKey_Needs_Modifier;
	UUtBool		not_in_shipping_version = bindings[inKey].when != ONcDebugKey_Game;
	LItKeyCode	keyCode1 = bindings[inKey].keyCode1;
	LItKeyCode	keyCode2 = bindings[inKey].keyCode2;
	UUtBool		has_modifier=  UUcFalse;
	UUtBool		key_went_down = UUcFalse;

	UUmAssert(bindings[inKey].binding == inKey);

#if SHIPPING_VERSION && !defined(ALLOW_DEVELOPER_IN_SHIPPING_VERSION)
	if (not_in_shipping_version && !ONgDeveloperAccess) {
		return UUcFalse;
	}
#endif

	if (LIrMode_Get() != LIcMode_Game) {
		return UUcFalse;
	}

	if (LIrTestKey(LIcKeyCode_LeftControl) || LIrTestKey(LIcKeyCode_RightControl)) 
	{
		UUtBool shift_down = LIrTestKey(LIcKeyCode_LeftShift) || LIrTestKey(LIcKeyCode_RightShift);
		UUtBool alt_down = LIrTestKey(LIcKeyCode_LeftAlt) || LIrTestKey(LIcKeyCode_RightAlt);

		if (shift_down || alt_down)
		{
			has_modifier = UUcTrue;
		}
	}

	if (has_modifier || !needs_modifier)
	{
		UUtBool key_down = UUcFalse;

		if (keyCode1 != LIcKeyCode_None) {
			key_down = key_down || LIrTestKey(keyCode1);
		}

		if (keyCode2 != LIcKeyCode_None) {
			key_down = key_down || LIrTestKey(keyCode2);
		}

		UUmAssert((UUcFalse == key_down) || (UUcTrue == key_down));

		key_went_down = key_down & !(gDebugKeyIsDown[inKey]);
		gDebugKeyIsDown[inKey] = key_down;
	}

	return key_went_down;
}
