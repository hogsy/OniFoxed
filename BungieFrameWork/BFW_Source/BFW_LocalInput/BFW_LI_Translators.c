// ======================================================================
// BFW_LI_Translators.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_LocalInput.h"
#include "BFW_LI_Private.h"

#include <ctype.h>

#if UUmCompiler == UUmCompiler_MWerks
#pragma profile off
#endif
// ======================================================================
// typedef
// ======================================================================
typedef struct LItInputNames
{
	char				name[LIcMaxInputNameLength];
	UUtUns32			keyCode;

} LItInputNames;

// ======================================================================
// globals
// ======================================================================
/*
	UUtUns16				inputType;
	UUtUns16				actionData;
	char					actionName[32];
*/
// action descriptions names must be lower case
LItActionDescription		LIgActionDescriptions[] =
{
	{ LIcIT_Button,				LIc_Bit_Forward,			"forward"		},
	{ LIcIT_Button,				LIc_Bit_Backward,			"backward"		},
	{ LIcIT_Button,				LIc_Bit_TurnLeft,			"turnleft"		},
	{ LIcIT_Button,				LIc_Bit_TurnRight,			"turnright"		},
	{ LIcIT_Button,				LIc_Bit_StepLeft,			"stepleft"		},
	{ LIcIT_Button,				LIc_Bit_StepRight,			"stepright"		},
	{ LIcIT_Button,				LIc_Bit_Jump,				"jump"			},
	{ LIcIT_Button,				LIc_Bit_Crouch,				"crouch"		},
	{ LIcIT_Button,				LIc_Bit_Punch,				"punch"			},
	{ LIcIT_Button,				LIc_Bit_Kick,				"kick"			},
	{ LIcIT_Button,				LIc_Bit_Block,				"block"			},
	{ LIcIT_Button,				LIc_Bit_Walk,				"walk"			},
	{ LIcIT_Button,				LIc_Bit_Fire1,				"fire1"			},
	{ LIcIT_Button,				LIc_Bit_Fire2,				"fire2"			},
	{ LIcIT_Button,				LIc_Bit_Fire3,				"fire3"			},
	{ LIcIT_Button,				LIc_Bit_Escape,				"escape"		},
	{ LIcIT_Button,				LIc_Bit_Action,				"action"		},
	{ LIcIT_Button,				LIc_Bit_Hypo,				"hypo"			},
	{ LIcIT_Button,				LIc_Bit_Reload,				"reload"		},
	{ LIcIT_Button,				LIc_Bit_Swap,				"swap"			},
	{ LIcIT_Button,				LIc_Bit_Drop,				"drop"			},
	{ LIcIT_Button,				LIc_Bit_PauseScreen,		"pausescreen"	},
	{ LIcIT_Button,				LIc_Bit_Cutscene1,			"cutscene1"		},
	{ LIcIT_Button,				LIc_Bit_Cutscene2,			"cutscene2"		},
	{ LIcIT_Button,				LIc_Bit_F4,					"f4"			},
	{ LIcIT_Button,				LIc_Bit_F5,					"f5"			},
	{ LIcIT_Button,				LIc_Bit_F6,					"f6"			},
	{ LIcIT_Button,				LIc_Bit_F7,					"f7"			},
	{ LIcIT_Button,				LIc_Bit_F8,					"f8"			},
	{ LIcIT_Button,				LIc_Bit_StartRecord,		"start_record"	},
	{ LIcIT_Button,				LIc_Bit_StopRecord,			"stop_record"	},
	{ LIcIT_Button,				LIc_Bit_PlayRecord,			"play_record"	},
	{ LIcIT_Button,				LIc_Bit_F12,				"f12"			},
	{ LIcIT_Button,				LIc_Bit_Console,			"console"		},
	{ LIcIT_Button,				LIc_Bit_LookMode,			"lookmode"		},
	{ LIcIT_Button,				LIc_Bit_Reload,				"reload"		},
	{ LIcIT_Button,				LIc_Bit_ScreenShot,			"screenshot"	},

	{ LIcIT_Axis_Delta,			LIc_Aim_LR,					"aim_lr"		},
	{ LIcIT_Axis_Delta,			LIc_Aim_UD,					"aim_ud"		},
	{ LIcIT_Axis_Symmetric_Pos,	LIc_ManCam_Move_UD,			"man_cam_move_up"		},
	{ LIcIT_Axis_Symmetric_Neg,	LIc_ManCam_Move_UD,			"man_cam_move_down"		},
	{ LIcIT_Axis_Symmetric_Pos,	LIc_ManCam_Move_LR,			"man_cam_move_left"		},
	{ LIcIT_Axis_Symmetric_Neg,	LIc_ManCam_Move_LR,			"man_cam_move_right"	},
	{ LIcIT_Axis_Symmetric_Pos,	LIc_ManCam_Move_FB,			"man_cam_move_forward"	},
	{ LIcIT_Axis_Symmetric_Neg,	LIc_ManCam_Move_FB,			"man_cam_move_backward"	},
	{ LIcIT_Axis_Symmetric_Pos,	LIc_ManCam_Pan_UD,			"man_cam_pan_up"		},
	{ LIcIT_Axis_Symmetric_Neg,	LIc_ManCam_Pan_UD,			"man_cam_pan_down"		},
	{ LIcIT_Axis_Symmetric_Pos,	LIc_ManCam_Pan_LR,			"man_cam_pan_left"		},
	{ LIcIT_Axis_Symmetric_Neg,	LIc_ManCam_Pan_LR,			"man_cam_pan_right"		},
	{ LIcIT_None,				0,							""				}
};

// input names must be all lower case
LItInputNames				LIgInputNames[] =
{
	{ "fkey1",					LIcKeyCode_F1			},
	{ "fkey2",					LIcKeyCode_F2			},
	{ "fkey3",					LIcKeyCode_F3			},
	{ "fkey4",					LIcKeyCode_F4			},
	{ "fkey5",					LIcKeyCode_F5			},
	{ "fkey6",					LIcKeyCode_F6			},
	{ "fkey7",					LIcKeyCode_F7			},
	{ "fkey8",					LIcKeyCode_F8			},
	{ "fkey9",					LIcKeyCode_F9			},
	{ "fkey10",				LIcKeyCode_F10			},
	{ "fkey11",				LIcKeyCode_F11			},
	{ "fkey12",				LIcKeyCode_F12			},
	{ "fkey13",				LIcKeyCode_F13			},
	{ "fkey14",				LIcKeyCode_F14			},
	{ "fkey15",				LIcKeyCode_F15			},

	{ "backspace",			LIcKeyCode_BackSpace	},
	{ "tab",				LIcKeyCode_Tab			},
	{ "capslock",			LIcKeyCode_CapsLock		},
	{ "enter",				LIcKeyCode_Return		},
	{ "leftshift",			LIcKeyCode_LeftShift	},
	{ "rightshift",			LIcKeyCode_RightShift	},
	{ "leftcontrol",		LIcKeyCode_LeftControl	},
	{ "leftwindows",		LIcKeyCode_LeftOption	},
	{ "leftoption",			LIcKeyCode_LeftOption	},
	{ "leftalt",			LIcKeyCode_LeftAlt		},
	{ "space",				LIcKeyCode_Space		},
	{ "rightalt",			LIcKeyCode_RightAlt		},
	{ "rightoption",		LIcKeyCode_RightOption	},
	{ "rightwindows",		LIcKeyCode_RightOption	},
	{ "rightcontrol",		LIcKeyCode_RightControl	},

	{ "printscreen",		LIcKeyCode_PrintScreen	},
	{ "scrolllock",			LIcKeyCode_ScrollLock	},
	{ "pause",				LIcKeyCode_Pause		},

	{ "insert",				LIcKeyCode_Insert		},
	{ "home",				LIcKeyCode_Home			},
	{ "pageup",				LIcKeyCode_PageUp		},
	{ "delete",				LIcKeyCode_Delete		},
	{ "end",				LIcKeyCode_End			},
	{ "pagedown",			LIcKeyCode_PageDown		},

	{ "uparrow",			LIcKeyCode_UpArrow		},
	{ "leftarrow",			LIcKeyCode_LeftArrow	},
	{ "downarrow",			LIcKeyCode_DownArrow	},
	{ "rightarrow",			LIcKeyCode_RightArrow	},

	{ "numlock",			LIcKeyCode_NumLock		},
	{ "divide",				LIcKeyCode_Divide		},
	{ "multiply",			LIcKeyCode_Multiply		},
	{ "subtract",			LIcKeyCode_Subtract		},
	{ "add",				LIcKeyCode_Add			},
	{ "numpadequals",		LIcKeyCode_NumPadEquals	},
	{ "numpadenter",		LIcKeyCode_NumPadEnter	},
	{ "decimal",			LIcKeyCode_Decimal		},
	{ "numpad0",			LIcKeyCode_NumPad0		},
	{ "numpad1",			LIcKeyCode_NumPad1		},
	{ "numpad2",			LIcKeyCode_NumPad2		},
	{ "numpad3",			LIcKeyCode_NumPad3		},
	{ "numpad4",			LIcKeyCode_NumPad4		},
	{ "numpad5",			LIcKeyCode_NumPad5		},
	{ "numpad6",			LIcKeyCode_NumPad6		},
	{ "numpad7",			LIcKeyCode_NumPad7		},
	{ "numpad8",			LIcKeyCode_NumPad8		},
	{ "numpad9",			LIcKeyCode_NumPad9		},

	{ "backslash",			LIcKeyCode_BackSlash	},
	{ "semicolon",			LIcKeyCode_Semicolon	},
	{ "period",				LIcKeyCode_Period		},
	{ "apostrophe",			LIcKeyCode_Apostrophe	},
	{ "slash",				LIcKeyCode_Slash		},
	{ "leftbracket",		LIcKeyCode_LeftBracket	},
	{ "rightbracket",		LIcKeyCode_RightBracket	},
	{ "comma",				LIcKeyCode_Comma		},

	{ "mousebutton1",		LIcMouseCode_Button1	},
	{ "mousebutton2",		LIcMouseCode_Button2	},
	{ "mousebutton3",		LIcMouseCode_Button3	},
	{ "mousebutton4",		LIcMouseCode_Button4	},
	{ "mousexaxis",			LIcMouseCode_XAxis		},
	{ "mouseyaxis",			LIcMouseCode_YAxis		},
	{ "mousezaxis",			LIcMouseCode_ZAxis		},

	{ "",					LIcKeyCode_None			}
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
LIrTranslate_InputCode(
	UUtUns32			inKeyCode,
	char				*outInputName)
{
	LItInputNames		*name;
	UUtBool				found;

	// initialize outInputName
	UUrString_Copy(outInputName, "", LIcMaxInputNameLength);

	// find the input name whose keycode that matches inKeyCode
	found = UUcFalse;
	for (name = LIgInputNames;
		 name->name[0] != '\0';
		 name++)
	{
		if (name->keyCode == inKeyCode)
		{
			UUrString_Copy(outInputName, name->name, LIcMaxInputNameLength);
			found = UUcTrue;
			break;
		}
	}

	if (found == UUcFalse)
	{
		// it wasn't a complex name
		sprintf(outInputName, "%c", inKeyCode);
	}
}

// ----------------------------------------------------------------------
UUtUns32
LIrTranslate_InputName(
	char				*inInputName)
{
	UUtUns32			name_length;
	UUtUns32			out_keycode;
	char				*src;

	// init the vars
	out_keycode = LIcKeyCode_None;

	// get the length of the string
	name_length = strlen(inInputName);

	// make the input name all lower case
	src = inInputName;
	while (*src)
	{
		*src = tolower(*src);
		src++;
	}

	if (name_length == 1)
	{
		// it is a letter or number or symbol and can easily be
		// translated into Key Code
		out_keycode = inInputName[0];
	}
	else
	{
		LItInputNames	*name;

		// go through the input names list and translate
		// the name into the key
		for (name = LIgInputNames;
			 name->name[0] != '\0';
			 name++)
		{
			if (strcmp(name->name, inInputName) == 0)
			{
				out_keycode = name->keyCode;
				break;
			}
		}
	}

	return out_keycode;
}
