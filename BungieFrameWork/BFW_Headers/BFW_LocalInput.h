#pragma once

// ======================================================================
// BFW_LocalInput.h
// ======================================================================
#ifndef BFW_LOCALINPUT_H
#define BFW_LOCALINPUT_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Image.h"

// ======================================================================
// defines
// ======================================================================
#define LImMakeBitMask(b)				((LItButtonBits)1 << (b))

#define LIcMaxInputNameLength			32
#define LIcMaxActionNameLength			32

// ======================================================================
// enums
// ======================================================================

typedef enum LItInputEventType
{
	LIcInputEvent_None,
	LIcInputEvent_MouseMove,
	LIcInputEvent_LMouseDown,
	LIcInputEvent_LMouseUp,
	LIcInputEvent_MMouseDown,
	LIcInputEvent_MMouseUp,
	LIcInputEvent_RMouseDown,
	LIcInputEvent_RMouseUp,
	LIcInputEvent_KeyUp,
	LIcInputEvent_KeyDown,
	LIcInputEvent_KeyRepeat
	
} LItInputEventType;

typedef enum LItMode
{
	LIcMode_Normal,
	LIcMode_Game
	
} LItMode;

// ======================================================================
typedef UUtUns64			LItButtonBits;

// ----------------------------------------------------------------------
// These enums are used to designate an analog value
enum
{
	LIc_ManCam_Pan_LR		= 0,
	LIc_ManCam_Pan_UD		= 1,
	LIc_ManCam_Move_FB		= 2,
	LIc_ManCam_Move_UD		= 3,
	LIc_ManCam_Move_LR		= 4,
	LIc_Look_UD				= 5,
	LIc_Look_LR				= 6,
	LIc_Aim_LR				= 7,
	LIc_Aim_UD				= 8,
	LIcNumAnalogValues
};

// ----------------------------------------------------------------------
enum
{
	LIc_Bit_Escape,
	LIc_Bit_Console,
	LIc_Bit_PauseScreen,
	LIc_Bit_Cutscene1,
	LIc_Bit_Cutscene2,
	LIc_Bit_F4,
	LIc_Bit_F5,
	LIc_Bit_F6,

	LIc_Bit_F7,
	LIc_Bit_F8,
	LIc_Bit_StartRecord,
	LIc_Bit_StopRecord,
	LIc_Bit_PlayRecord,
	LIc_Bit_F12,
	LIc_Bit_Unused1,
	LIc_Bit_LookMode,

	LIc_Bit_ScreenShot,
	LIc_Bit_Unused2,
	LIc_Bit_Unused3,
	LIc_Bit_Unused4,
	LIc_Bit_Unused5,
	LIc_Bit_Forward,
	LIc_Bit_Backward,
	LIc_Bit_TurnLeft,

	LIc_Bit_TurnRight,
	LIc_Bit_StepLeft,
	LIc_Bit_StepRight,
	LIc_Bit_Jump,
	LIc_Bit_Crouch,
	LIc_Bit_Punch,
	LIc_Bit_Kick,
	LIc_Bit_Block,

	LIc_Bit_Walk,
	LIc_Bit_Action,
	LIc_Bit_Hypo,
	LIc_Bit_Reload,
	LIc_Bit_Swap,
	LIc_Bit_Drop,
	LIc_Bit_Fire1,
	LIc_Bit_Fire2,
	LIc_Bit_Fire3,
	
	LIcNumButtonBits
};

#define	LIc_BitMask_Escape			LImMakeBitMask(LIc_Bit_Escape)
#define	LIc_BitMask_Console			LImMakeBitMask(LIc_Bit_Console)
#define	LIc_BitMask_PauseScreen		LImMakeBitMask(LIc_Bit_PauseScreen)
#define LIc_BitMask_Cutscene1		LImMakeBitMask(LIc_Bit_Cutscene1)
#define LIc_BitMask_Cutscene2		LImMakeBitMask(LIc_Bit_Cutscene2)
#define	LIc_BitMask_F4				LImMakeBitMask(LIc_Bit_F4)
#define	LIc_BitMask_F5				LImMakeBitMask(LIc_Bit_F5)
#define	LIc_BitMask_F6				LImMakeBitMask(LIc_Bit_F6)
#define	LIc_BitMask_F7				LImMakeBitMask(LIc_Bit_F7)
#define	LIc_BitMask_F8				LImMakeBitMask(LIc_Bit_F8)
#define	LIc_BitMask_StartRecord		LImMakeBitMask(LIc_Bit_StartRecord)
#define	LIc_BitMask_StopRecord		LImMakeBitMask(LIc_Bit_StopRecord)
#define	LIc_BitMask_PlayRecord		LImMakeBitMask(LIc_Bit_PlayRecord)
#define	LIc_BitMask_F12				LImMakeBitMask(LIc_Bit_F12)
#define	LIc_BitMask_LookMode		LImMakeBitMask(LIc_Bit_LookMode)
#define	LIc_BitMask_ScreenShot		LImMakeBitMask(LIc_Bit_ScreenShot)

#define	LIc_BitMask_Forward			LImMakeBitMask(LIc_Bit_Forward)
#define	LIc_BitMask_Backward		LImMakeBitMask(LIc_Bit_Backward)
#define	LIc_BitMask_TurnLeft		LImMakeBitMask(LIc_Bit_TurnLeft)
#define	LIc_BitMask_TurnRight		LImMakeBitMask(LIc_Bit_TurnRight)
#define	LIc_BitMask_StepLeft		LImMakeBitMask(LIc_Bit_StepLeft)
#define	LIc_BitMask_StepRight		LImMakeBitMask(LIc_Bit_StepRight)
#define	LIc_BitMask_Jump			LImMakeBitMask(LIc_Bit_Jump)
#define	LIc_BitMask_Crouch			LImMakeBitMask(LIc_Bit_Crouch)
#define	LIc_BitMask_Punch			LImMakeBitMask(LIc_Bit_Punch)
#define	LIc_BitMask_Kick			LImMakeBitMask(LIc_Bit_Kick)
#define	LIc_BitMask_Block			LImMakeBitMask(LIc_Bit_Block)
#define	LIc_BitMask_Walk			LImMakeBitMask(LIc_Bit_Walk)
#define	LIc_BitMask_Action			LImMakeBitMask(LIc_Bit_Action)
#define	LIc_BitMask_Hypo			LImMakeBitMask(LIc_Bit_Hypo)
#define	LIc_BitMask_Reload			LImMakeBitMask(LIc_Bit_Reload)
#define	LIc_BitMask_Swap			LImMakeBitMask(LIc_Bit_Swap)
#define	LIc_BitMask_Drop			LImMakeBitMask(LIc_Bit_Drop)
#define	LIc_BitMask_Fire1			LImMakeBitMask(LIc_Bit_Fire1)
#define	LIc_BitMask_Fire2			LImMakeBitMask(LIc_Bit_Fire2)
#define	LIc_BitMask_Fire3			LImMakeBitMask(LIc_Bit_Fire3)

// used by AI2 to work out if it's moving
#define LIc_BitMask_MoveKeys		(LIc_BitMask_Forward | LIc_BitMask_StepLeft | LIc_BitMask_Backward | LIc_BitMask_StepRight)

// these keys instantly abort a neutral-char interaction
#define LIc_BitMask_NeutralAbort	(LIc_BitMask_Fire1 | LIc_BitMask_Fire2 | LIc_BitMask_Fire3 | LIc_BitMask_Punch | LIc_BitMask_Kick)

// these keys, if held down, will abort a neutral-char interaction
#define LIc_BitMask_NeutralStop		(LIc_BitMask_MoveKeys | LIc_BitMask_Crouch | LIc_BitMask_Jump)

// these keys are disabled during a neutral-char interaction
#define LIc_BitMask_NeutralLockOut	(LIc_BitMask_NeutralAbort | LIc_BitMask_NeutralStop | \
									 LIc_BitMask_Action | LIc_BitMask_Hypo | LIc_BitMask_Reload | LIc_BitMask_Swap | LIc_BitMask_Drop)

typedef enum LItInputType
{
	LIcIT_None,
	LIcIT_Button,
	LIcIT_Axis_Asymmetric,
	LIcIT_Axis_Delta,
	LIcIT_Axis_Symmetric_Pos,
	LIcIT_Axis_Symmetric_Neg
	
} LItInputType;

// ======================================================================
// these enums are used to designate the keycodes
typedef enum
{
	LIcKeyCode_None				= 0x00,
	
	LIcKeyCode_Escape			= 0x1B,
	LIcKeyCode_F1				= 0x81,
	LIcKeyCode_F2				= 0x82,
	LIcKeyCode_F3				= 0x83,
	LIcKeyCode_F4				= 0x84,
	LIcKeyCode_F5				= 0x85,
	LIcKeyCode_F6				= 0x86,
	LIcKeyCode_F7				= 0x87,
	LIcKeyCode_F8				= 0x88,
	LIcKeyCode_F9				= 0x89,
	LIcKeyCode_F10				= 0x8A,
	LIcKeyCode_F11				= 0x8B,
	LIcKeyCode_F12				= 0x8C,
	LIcKeyCode_F13				= 0x8D,
	LIcKeyCode_F14				= 0x8E,
	LIcKeyCode_F15				= 0x8F,

	LIcKeyCode_Grave			= '`',
	LIcKeyCode_1				= '1',
	LIcKeyCode_2				= '2',
	LIcKeyCode_3				= '3',
	LIcKeyCode_4				= '4',
	LIcKeyCode_5				= '5',
	LIcKeyCode_6				= '6',
	LIcKeyCode_7				= '7',
	LIcKeyCode_8				= '8',
	LIcKeyCode_9				= '9',
	LIcKeyCode_0				= '0',
	LIcKeyCode_Minus			= '-',
	LIcKeyCode_Equals			= '=',
	LIcKeyCode_BackSpace		= 0x08,
	
	LIcKeyCode_Tab				= 0x09,
	LIcKeyCode_Q				= 'q',
	LIcKeyCode_W				= 'w',
	LIcKeyCode_E				= 'e',
	LIcKeyCode_R				= 'r',
	LIcKeyCode_T				= 't',
	LIcKeyCode_Y				= 'y',
	LIcKeyCode_U				= 'u',
	LIcKeyCode_I				= 'i',
	LIcKeyCode_O				= 'o',
	LIcKeyCode_P				= 'p',
	LIcKeyCode_LeftBracket		= '[',
	LIcKeyCode_RightBracket		= ']',
	LIcKeyCode_BackSlash		= '\\',
	
	LIcKeyCode_CapsLock			= 0x90,
	LIcKeyCode_A				= 'a',
	LIcKeyCode_S				= 's',
	LIcKeyCode_D				= 'd',
	LIcKeyCode_F				= 'f',
	LIcKeyCode_G				= 'g',
	LIcKeyCode_H				= 'h',
	LIcKeyCode_J				= 'j',
	LIcKeyCode_K				= 'k',
	LIcKeyCode_L				= 'l',
	LIcKeyCode_Semicolon		= ';',
	LIcKeyCode_Apostrophe		= '\'',
	LIcKeyCode_Return			= 0x0D,
	
	LIcKeyCode_LeftShift		= 0x91,
	LIcKeyCode_Z				= 'z',
	LIcKeyCode_X				= 'x',
	LIcKeyCode_C				= 'c',
	LIcKeyCode_V				= 'v',
	LIcKeyCode_B				= 'b',
	LIcKeyCode_N				= 'n',
	LIcKeyCode_M				= 'm',
	LIcKeyCode_Comma			= ',',
	LIcKeyCode_Period			= '.',
	LIcKeyCode_Slash			= '/',
	LIcKeyCode_RightShift		= 0x92,
	
	LIcKeyCode_LeftControl		= 0x93,
	LIcKeyCode_LeftOption		= 0x94,
	LIcKeyCode_LeftAlt			= 0x95,
	LIcKeyCode_Space			= 0x20,
	LIcKeyCode_RightAlt			= 0x96,
	LIcKeyCode_RightOption		= 0x97,
	LIcKeyCode_AppMenuKey		= 0x98,
	LIcKeyCode_RightControl		= 0x99,
	
	LIcKeyCode_PrintScreen		= LIcKeyCode_F13,
	LIcKeyCode_ScrollLock		= LIcKeyCode_F14,
	LIcKeyCode_Pause			= LIcKeyCode_F15,
	
	LIcKeyCode_Insert			= 0xAA,
	LIcKeyCode_Home				= 0xAB,
	LIcKeyCode_PageUp			= 0xAC,
	LIcKeyCode_Delete			= 0xAD,
	LIcKeyCode_End				= 0xAE,
	LIcKeyCode_PageDown			= 0xAF,
	
	LIcKeyCode_UpArrow			= 0xB0,
	LIcKeyCode_LeftArrow		= 0xB1,
	LIcKeyCode_DownArrow		= 0xB2,
	LIcKeyCode_RightArrow		= 0xB3,
	
	LIcKeyCode_NumLock			= 0xB4,
	LIcKeyCode_Divide			= 0xB5,
	LIcKeyCode_Multiply			= 0xB6,
	LIcKeyCode_Subtract			= 0xB7,
	LIcKeyCode_Add				= 0xB8,
	LIcKeyCode_Decimal			= 0xB9,
	LIcKeyCode_NumPadEnter		= 0xBA,
	LIcKeyCode_NumPadComma		= 0xBB,
	LIcKeyCode_NumPadEquals		= 0xBC,
	
	LIcKeyCode_NumPad0			= 0xBD,
	LIcKeyCode_NumPad1			= 0xBE,
	LIcKeyCode_NumPad2			= 0xBF,
	LIcKeyCode_NumPad3			= 0xC0,
	LIcKeyCode_NumPad4			= 0xC1,
	LIcKeyCode_NumPad5			= 0xC2,
	LIcKeyCode_NumPad6			= 0xC3,
	LIcKeyCode_NumPad7			= 0xC4,
	LIcKeyCode_NumPad8			= 0xC5,
	LIcKeyCode_NumPad9			= 0xC6,
	
	LIcKeyCode_Tilde			= '~',
	LIcKeyCode_Exclamation		= '!',
	LIcKeyCode_At				= '@',
	LIcKeyCode_Pound			= '#',
	LIcKeyCode_Dollar			= '$',
	LIcKeyCode_Percent			= '%',
	LIcKeyCode_Hat				= '^',
	LIcKeyCode_Ampersand		= '&',
	LIcKeyCode_Star				= '*',
	LIcKeyCode_LeftParen		= '(',
	LIcKeyCode_RightParen		= ')',
	LIcKeyCode_Underscore		= '_',
	LIcKeyCode_Plus				= '+',
	LIcKeyCode_LeftCurlyBracket	= '{',
	LIcKeyCode_RightCurlyBracket	= '}',
	LIcKeyCode_VerticalLine		= '|',
	LIcKeyCode_Colon			= ':',
	LIcKeyCode_Quote			= '\"',
	LIcKeyCode_LessThan			= '<',
	LIcKeyCode_GreaterThan		= '>',
	LIcKeyCode_Question			= '?',
	
	LIcKeyCode_LeftWindowsKey	= 0xC7,
	LIcKeyCode_RightWindowsKey	= 0xC9
	
} LItKeyCode;

enum
{
	LIcMouseCode_Button1		= 0x0001,
	LIcMouseCode_Button2		= 0x0002,
	LIcMouseCode_Button3		= 0x0003,
	LIcMouseCode_Button4		= 0x0004,
	LIcMouseCode_XAxis			= 0x0005,
	LIcMouseCode_YAxis			= 0x0006,
	LIcMouseCode_ZAxis			= 0x0007
};

enum
{
	LIcMouseState_None			= 0x0000,
	LIcMouseState_LButtonDown	= 0x0001,
	LIcMouseState_MButtonDown	= 0x0002,
	LIcMouseState_RButtonDown	= 0x0004,
	LIcMouseState_LShiftDown	= 0x0008,
	LIcMouseState_RShiftDown	= 0x0010,
	LIcMouseState_ShiftDown		= (LIcMouseState_LShiftDown | LIcMouseState_RShiftDown),
	LIcMouseState_LControlDown	= 0x0020,
	LIcMouseState_RControlDown	= 0x0040,
	LIcMouseState_ControlDown	= (LIcMouseState_LControlDown | LIcMouseState_RControlDown)
};

enum
{
	LIcKeyState_None			= 0x0000,
	LIcKeyState_LShiftDown		= 0x0001,
	LIcKeyState_RShiftDown		= 0x0002,
	LIcKeyState_LCommandDown	= 0x0004,	/* left control key under Win32 */
	LIcKeyState_RCommandDown	= 0x0008,	/* right control key under Win32 */
	LIcKeyState_LAltDown		= 0x0010,
	LIcKeyState_RAltDown		= 0x0020,
	
	LIcKeyState_ShiftDown		= (LIcKeyState_LShiftDown | LIcKeyState_RShiftDown),
	LIcKeyState_CommandDown		= (LIcKeyState_LCommandDown | LIcKeyState_RCommandDown),
	LIcKeyState_AltDown			= (LIcKeyState_LAltDown | LIcKeyState_RAltDown)
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct LItAction
{
	LItButtonBits		buttonBits;
	float				analogValues[LIcNumAnalogValues];
	
} LItAction;

typedef struct LItInputEvent
{
	LItInputEventType	type;
	IMtPoint2D			where;
	UUtInt16			key;
	UUtUns32			modifiers;
	
} LItInputEvent;

// ----------------------------------------------------------------------
// device input is bound to a series of actions.
//
// bind a to stepleft
//
// when the user hits 'a' on the keyboard, the device will generate an
// input and call LIrActionBuffer_Add().  LIrActionBufer_Add() will
// search the bindings looking for one whose boundInput is 'a'.  When
// it finds the binding which has 'a' bound to stepleft, it will generate
// a step left button bit in the active actionbuffer.
// ----------------------------------------------------------------------
// input from the input device (keyboard, mouse, etc)
typedef struct LItDeviceInput
{
	UUtUns32				input;
	float					analogValue;
	
} LItDeviceInput;

// actions that the game understands
typedef struct LItActionDescription
{
	UUtUns16				inputType;
	UUtUns16				actionData;		// AnalogValue index or LIc_Bit_*
	char					actionName[LIcMaxActionNameLength];
	
} LItActionDescription;

// binding of device input to actions
typedef struct LItBinding
{
	UUtUns32				boundInput;
	LItActionDescription	*action;
	
} LItBinding;

// enumerator function pointer typedef
typedef UUtBool
(*LItEnumBindings)(
	UUtUns32			inBoundInput,
	UUtUns16			inActionType,
	UUtUns32			inUserParam);

// ======================================================================
// prototypes
// ======================================================================

UUtBool
LIrTestKey(LItKeyCode inKeyCode);

UUtBool 
LIrKeyWentDown(LItKeyCode inKeyCode, UUtBool *ioOldKeyDown);

void
LIrActionBuffer_Add(
	LItAction			*outAction,
	LItDeviceInput		*inDeviceInput);
	
void
LIrActionBuffer_Get(
	UUtUns16			*outNumActionsInBuffer,
	LItAction			**outActionBuffer);

// ----------------------------------------------------------------------
UUtError
LIrBinding_Add(
	UUtUns32			inBoundInput,
	const char			*inActionName);

void
LIrBindings_Enumerate(
	LItEnumBindings		inBindingEnumerator,
	UUtUns32			inUserParam);
	
void
LIrBinding_Remove(
	UUtUns32			inBoundInput);

void
LIrBindings_RemoveAll(
	void);
	
// ----------------------------------------------------------------------
void
LIrInputEvent_Add(
	LItInputEventType	inEventType,
	IMtPoint2D			*inPoint,
	UUtUns32			inKey,
	UUtUns32			inModifiers);
	
UUtBool
LIrInputEvent_Get(
	LItInputEvent		*outInputEvent);

void
LIrInputEvent_GetMouse(
	LItInputEvent		*outInputEvent);

typedef void (*LItCheatHook)(const LItInputEvent *outInputEvent);

void LIrInputEvent_CheatHook(LItCheatHook inHook);

// ----------------------------------------------------------------------
LItMode
LIrMode_Get(
	void);
	
void
LIrMode_Set(
	LItMode				inMode);

void LIrGameIsActive(UUtBool inGameIsActive);
	
// ----------------------------------------------------------------------
void
LIrTranslate_InputCode(
	UUtUns32			inKeyCode,
	char				*outInputName);

// ----------------------------------------------------------------------
UUtError
LIrInitialize(
	UUtAppInstance		inInstance,
	UUtWindow			inWindow);

void
LIrInitializeDeveloperKeys(
	void);

void
LIrUnbindDeveloperKeys(
	void);

void
LIrTerminate(
	void);

void
LIrUpdate(
	void);

// ----------------------------------------------------------------------
UUtError
LIrRegisterTemplates(
	void);
	
// ======================================================================
#endif /* BFW_LOCALINPUT_H */
