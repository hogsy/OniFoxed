// ======================================================================
// BFW_LI_Platform_Win32.c
// BLEARGH! this code sucks!
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_LocalInput.h"
#include "BFW_LI_Platform.h"
#include "BFW_LI_Private.h"
#include "BFW_Console.h"
#include "BFW_LI_Platform_Win32.h"
#include "BFW_Timer.h"
#include "BFW_ScriptLang.h"

#include <dinput.h>


// ======================================================================
// defines
// ======================================================================
#define LIcPlatform_MaxRetries  	3

// ======================================================================
// globals
// ======================================================================
static UUtWindow			LIgWindow;
static UUtBool				LIgUseDirectInput;

// WindowsNT 4 vars
UUtBool						LIgSetCursorPosThisFrame = UUcTrue;
UUtBool						LIgSetCursorPosEver = UUcTrue;
static POINT				LIgCenter;

// Direct Input vars
static LPDIRECTINPUT		LIgDirectInput;
static LPDIRECTINPUTDEVICE	LIgMouse;
static LPDIRECTINPUTDEVICE	LIgKeyboard;

// DIK_* to LIcKeyCode_* translation table
static const UUtUns8 LIgDIKeyboardTranslation[256] =
{
	LIcKeyCode_None,			//					   0x00
	LIcKeyCode_Escape,			// DIK_ESCAPE          0x01
	LIcKeyCode_1,				// DIK_1               0x02
	LIcKeyCode_2,				// DIK_2               0x03
	LIcKeyCode_3,				// DIK_3               0x04
	LIcKeyCode_4,				// DIK_4               0x05
	LIcKeyCode_5,				// DIK_5               0x06
	LIcKeyCode_6,				// DIK_6               0x07
	LIcKeyCode_7,				// DIK_7               0x08
	LIcKeyCode_8,				// DIK_8               0x09
	LIcKeyCode_9,				// DIK_9               0x0A
	LIcKeyCode_0,				// DIK_0               0x0B
	LIcKeyCode_Minus,			// DIK_MINUS           0x0C    /* - on main keyboard */
	LIcKeyCode_Equals,			// DIK_EQUALS          0x0D
	LIcKeyCode_BackSpace,		// DIK_BACK            0x0E    /* backspace */
	LIcKeyCode_Tab,				// DIK_TAB             0x0F
	LIcKeyCode_Q,				// DIK_Q               0x10
	LIcKeyCode_W,				// DIK_W               0x11
	LIcKeyCode_E,				// DIK_E               0x12
	LIcKeyCode_R,				// DIK_R               0x13
	LIcKeyCode_T,				// DIK_T               0x14
	LIcKeyCode_Y,				// DIK_Y               0x15
	LIcKeyCode_U,				// DIK_U               0x16
	LIcKeyCode_I,				// DIK_I               0x17
	LIcKeyCode_O,				// DIK_O               0x18
	LIcKeyCode_P,				// DIK_P               0x19
	LIcKeyCode_LeftBracket,		// DIK_LBRACKET        0x1A
	LIcKeyCode_RightBracket,	// DIK_RBRACKET        0x1B
	LIcKeyCode_Return,			// DIK_RETURN          0x1C    /* Enter on main keyboard */
	LIcKeyCode_LeftControl,		// DIK_LCONTROL        0x1D
	LIcKeyCode_A,				// DIK_A               0x1E
	LIcKeyCode_S,				// DIK_S               0x1F
	LIcKeyCode_D,				// DIK_D               0x20
	LIcKeyCode_F,				// DIK_F               0x21
	LIcKeyCode_G,				// DIK_G               0x22
	LIcKeyCode_H,				// DIK_H               0x23
	LIcKeyCode_J,				// DIK_J               0x24
	LIcKeyCode_K,				// DIK_K               0x25
	LIcKeyCode_L,				// DIK_L               0x26
	LIcKeyCode_Semicolon,		// DIK_SEMICOLON       0x27
	LIcKeyCode_Apostrophe,		// DIK_APOSTROPHE      0x28
	LIcKeyCode_Grave,			// DIK_GRAVE           0x29    /* accent grave */
	LIcKeyCode_LeftShift,		// DIK_LSHIFT          0x2A
	LIcKeyCode_BackSlash,		// DIK_BACKSLASH       0x2B
	LIcKeyCode_Z,				// DIK_Z               0x2C
	LIcKeyCode_X,				// DIK_X               0x2D
	LIcKeyCode_C,				// DIK_C               0x2E
	LIcKeyCode_V,				// DIK_V               0x2F
	LIcKeyCode_B,				// DIK_B               0x30
	LIcKeyCode_N,				// DIK_N               0x31
	LIcKeyCode_M,				// DIK_M               0x32
	LIcKeyCode_Comma,			// DIK_COMMA           0x33
	LIcKeyCode_Period,			// DIK_PERIOD          0x34    /* . on main keyboard */
	LIcKeyCode_Slash,			// DIK_SLASH           0x35    /* / on main keyboard */
	LIcKeyCode_RightShift,		// DIK_RSHIFT          0x36
	LIcKeyCode_Multiply,		// DIK_MULTIPLY        0x37    /* * on numeric keypad */
	LIcKeyCode_LeftAlt,			// DIK_LMENU           0x38    /* left Alt */
	LIcKeyCode_Space,			// DIK_SPACE           0x39
	LIcKeyCode_CapsLock,		// DIK_CAPITAL         0x3A
	LIcKeyCode_F1,				// DIK_F1              0x3B
	LIcKeyCode_F2,				// DIK_F2              0x3C
	LIcKeyCode_F3,				// DIK_F3              0x3D
	LIcKeyCode_F4,				// DIK_F4              0x3E
	LIcKeyCode_F5,				// DIK_F5              0x3F
	LIcKeyCode_F6,				// DIK_F6              0x40
	LIcKeyCode_F7,				// DIK_F7              0x41
	LIcKeyCode_F8,				// DIK_F8              0x42
	LIcKeyCode_F9,				// DIK_F9              0x43
	LIcKeyCode_F10,				// DIK_F10             0x44
	LIcKeyCode_NumLock,			// DIK_NUMLOCK         0x45
	LIcKeyCode_ScrollLock,		// DIK_SCROLL          0x46    /* Scroll Lock */
	LIcKeyCode_NumPad7,			// DIK_NUMPAD7         0x47
	LIcKeyCode_NumPad8,			// DIK_NUMPAD8         0x48
	LIcKeyCode_NumPad9,			// DIK_NUMPAD9         0x49
	LIcKeyCode_Subtract,		// DIK_SUBTRACT        0x4A    /* - on numeric keypad */
	LIcKeyCode_NumPad4,			// DIK_NUMPAD4         0x4B
	LIcKeyCode_NumPad5,			// DIK_NUMPAD5         0x4C
	LIcKeyCode_NumPad6,			// DIK_NUMPAD6         0x4D
	LIcKeyCode_Add,				// DIK_ADD             0x4E    /* + on numeric keypad */
	LIcKeyCode_NumPad1,			// DIK_NUMPAD1         0x4F
	LIcKeyCode_NumPad2,			// DIK_NUMPAD2         0x50
	LIcKeyCode_NumPad3,			// DIK_NUMPAD3         0x51
	LIcKeyCode_NumPad0,			// DIK_NUMPAD0         0x52
	LIcKeyCode_Decimal,			// DIK_DECIMAL         0x53    /* . on numeric keypad */
	LIcKeyCode_None,			//					   0x54
	LIcKeyCode_None,			//					   0x55
	LIcKeyCode_None,			//					   0x56
	LIcKeyCode_F11,				// DIK_F11             0x57
	LIcKeyCode_F12,				// DIK_F12             0x58
	LIcKeyCode_None,			//					   0x59
	LIcKeyCode_None,			//					   0x5A
	LIcKeyCode_None,			//					   0x5B
	LIcKeyCode_None,			//					   0x5C
	LIcKeyCode_None,			//					   0x5D
	LIcKeyCode_None,			//					   0x5E
	LIcKeyCode_None,			//					   0x5F
	LIcKeyCode_None,			//					   0x60
	LIcKeyCode_None,			//					   0x61
	LIcKeyCode_None,			//					   0x62
	LIcKeyCode_None,			//					   0x63
	LIcKeyCode_F13,				// DIK_F13             0x64    /*                     (NEC PC98) */
	LIcKeyCode_F14,				// DIK_F14             0x65    /*                     (NEC PC98) */
	LIcKeyCode_F15,				// DIK_F15             0x66    /*                     (NEC PC98) */
	LIcKeyCode_None,			//					   0x67
	LIcKeyCode_None,			//					   0x68
	LIcKeyCode_None,			//					   0x69
	LIcKeyCode_None,			//					   0x6A
	LIcKeyCode_None,			//					   0x6B
	LIcKeyCode_None,			//					   0x6C
	LIcKeyCode_None,			//					   0x6D
	LIcKeyCode_None,			//					   0x6E
	LIcKeyCode_None,			//					   0x6F
	LIcKeyCode_None,			// DIK_KANA            0x70    /* (Japanese keyboard)            */
	LIcKeyCode_None,			//					   0x71
	LIcKeyCode_None,			//					   0x72
	LIcKeyCode_None,			//					   0x73
	LIcKeyCode_None,			//					   0x74
	LIcKeyCode_None,			//					   0x75
	LIcKeyCode_None,			//					   0x76
	LIcKeyCode_None,			//					   0x77
	LIcKeyCode_None,			//					   0x78
	LIcKeyCode_None,			// DIK_CONVERT         0x79    /* (Japanese keyboard)            */
	LIcKeyCode_None,			//					   0x7A
	LIcKeyCode_None,			// DIK_NOCONVERT       0x7B    /* (Japanese keyboard)            */
	LIcKeyCode_None,			//					   0x7C
	LIcKeyCode_None,			// DIK_YEN             0x7D    /* (Japanese keyboard)            */
	LIcKeyCode_None,			//					   0x7E
	LIcKeyCode_None,			//					   0x7F
	LIcKeyCode_None,			//					   0x80
	LIcKeyCode_None,			//					   0x81
	LIcKeyCode_None,			//					   0x82
	LIcKeyCode_None,			//					   0x83
	LIcKeyCode_None,			//					   0x84
	LIcKeyCode_None,			//					   0x85
	LIcKeyCode_None,			//					   0x86
	LIcKeyCode_None,			//					   0x87
	LIcKeyCode_None,			//					   0x88
	LIcKeyCode_None,			//					   0x89
	LIcKeyCode_None,			//					   0x8A
	LIcKeyCode_None,			//					   0x8B
	LIcKeyCode_None,			//					   0x8C
	LIcKeyCode_NumPadEquals,	// DIK_NUMPADEQUALS    0x8D    /* = on numeric keypad (NEC PC98) */
	LIcKeyCode_None,			//					   0x8E
	LIcKeyCode_None,			//					   0x8F
	LIcKeyCode_None,			// DIK_CIRCUMFLEX      0x90    /* (Japanese keyboard)            */
	LIcKeyCode_None,			// DIK_AT              0x91    /*                     (NEC PC98) */
	LIcKeyCode_None,			// DIK_COLON           0x92    /*                     (NEC PC98) */
	LIcKeyCode_None,			// DIK_UNDERLINE       0x93    /*                     (NEC PC98) */
	LIcKeyCode_None,			// DIK_KANJI           0x94    /* (Japanese keyboard)            */
	LIcKeyCode_None,			// DIK_STOP            0x95    /*                     (NEC PC98) */
	LIcKeyCode_None,			// DIK_AX              0x96    /*                     (Japan AX) */
	LIcKeyCode_None,			// DIK_UNLABELED       0x97    /*                        (J3100) */
	LIcKeyCode_None,			//					   0x98
	LIcKeyCode_None,			//					   0x99
	LIcKeyCode_None,			//					   0x9A
	LIcKeyCode_None,			//					   0x9B
	LIcKeyCode_NumPadEnter,		// DIK_NUMPADENTER     0x9C    /* Enter on numeric keypad */
	LIcKeyCode_RightControl,	// DIK_RCONTROL        0x9D
	LIcKeyCode_None,			//					   0x9E
	LIcKeyCode_None,			//					   0x9F
	LIcKeyCode_None,			//					   0xA0
	LIcKeyCode_None,			//					   0xA1
	LIcKeyCode_None,			//					   0xA2
	LIcKeyCode_None,			//					   0xA3
	LIcKeyCode_None,			//					   0xA4
	LIcKeyCode_None,			//					   0xA5
	LIcKeyCode_None,			//					   0xA6
	LIcKeyCode_None,			//					   0xA7
	LIcKeyCode_None,			//					   0xA8
	LIcKeyCode_None,			//					   0xA9
	LIcKeyCode_None,			//					   0xAA
	LIcKeyCode_None,			//					   0xAB
	LIcKeyCode_None,			//					   0xAC
	LIcKeyCode_None,			//					   0xAD
	LIcKeyCode_None,			//					   0xAE
	LIcKeyCode_None,			//					   0xAF
	LIcKeyCode_None,			//					   0xB0
	LIcKeyCode_None,			//					   0xB1
	LIcKeyCode_None,			//					   0xB2
	LIcKeyCode_NumPadComma,		// DIK_NUMPADCOMMA     0xB3    /* , on numeric keypad (NEC PC98) */
	LIcKeyCode_None,			//					   0xB4
	LIcKeyCode_Divide,			// DIK_DIVIDE          0xB5    /* / on numeric keypad */
	LIcKeyCode_None,			//					   0xB6
	LIcKeyCode_None,			// DIK_SYSRQ           0xB7
	LIcKeyCode_RightAlt,		// DIK_RMENU           0xB8    /* right Alt */
	LIcKeyCode_None,			//					   0xB9
	LIcKeyCode_None,			//					   0xBA
	LIcKeyCode_None,			//					   0xBB
	LIcKeyCode_None,			//					   0xBC
	LIcKeyCode_None,			//					   0xBD
	LIcKeyCode_None,			//					   0xBE
	LIcKeyCode_None,			//					   0xBF
	LIcKeyCode_None,			//					   0xC0
	LIcKeyCode_None,			//					   0xC1
	LIcKeyCode_None,			//					   0xC2
	LIcKeyCode_None,			//					   0xC3
	LIcKeyCode_None,			//					   0xC4
	LIcKeyCode_None,			//					   0xC5
	LIcKeyCode_None,			//					   0xC6
	LIcKeyCode_Home,			// DIK_HOME            0xC7    /* Home on arrow keypad */
	LIcKeyCode_UpArrow,			// DIK_UP              0xC8    /* UpArrow on arrow keypad */
	LIcKeyCode_PageUp,			// DIK_PRIOR           0xC9    /* PgUp on arrow keypad */
	LIcKeyCode_None,			//					   0xCA
	LIcKeyCode_LeftArrow,		// DIK_LEFT            0xCB    /* LeftArrow on arrow keypad */
	LIcKeyCode_None,			//					   0xCC
	LIcKeyCode_RightArrow,		// DIK_RIGHT           0xCD    /* RightArrow on arrow keypad */
	LIcKeyCode_None,			//					   0xCE
	LIcKeyCode_End,				// DIK_END             0xCF    /* End on arrow keypad */
	LIcKeyCode_DownArrow,		// DIK_DOWN            0xD0    /* DownArrow on arrow keypad */
	LIcKeyCode_None,			// DIK_NEXT            0xD1    /* PgDn on arrow keypad */
	LIcKeyCode_Insert,			// DIK_INSERT          0xD2    /* Insert on arrow keypad */
	LIcKeyCode_Delete,			// DIK_DELETE          0xD3    /* Delete on arrow keypad */
	LIcKeyCode_None,			//					   0xD4
	LIcKeyCode_None,			//					   0xD5
	LIcKeyCode_None,			//					   0xD6
	LIcKeyCode_None,			//					   0xD7
	LIcKeyCode_None,			//					   0xD8
	LIcKeyCode_None,			//					   0xD9
	LIcKeyCode_None,			//					   0xDA
	LIcKeyCode_LeftWindowsKey,	// DIK_LWIN            0xDB    /* Left Windows key */
	LIcKeyCode_RightWindowsKey,	// DIK_RWIN            0xDC    /* Right Windows key */
	LIcKeyCode_AppMenuKey		// DIK_APPS            0xDD    /* AppMenu key */
};

// VK_* to LIcKeyCode_* translation table
static const UUtUns8 LIgVKKeyboardTranslation[256] =
{
	LIcKeyCode_None,			//					 0x00
	LIcKeyCode_None,			// VK_LBUTTON        0x01
	LIcKeyCode_None,			// VK_RBUTTON        0x02
	LIcKeyCode_None,			// VK_CANCEL         0x03
	LIcKeyCode_None,			// VK_MBUTTON        0x04    /* NOT contiguous with L & RBUTTON */
	LIcKeyCode_None,			//					 0x05
	LIcKeyCode_None,			//					 0x06
	LIcKeyCode_None,			//					 0x07
	LIcKeyCode_BackSpace,		// VK_BACK           0x08
	LIcKeyCode_Tab,				// VK_TAB            0x09
	LIcKeyCode_None,			//					 0x0A
	LIcKeyCode_None,			//					 0x0B
	LIcKeyCode_None,			// VK_CLEAR          0x0C
	LIcKeyCode_Return,			// VK_RETURN         0x0D
	LIcKeyCode_None,			//					 0x0E
	LIcKeyCode_None,			//					 0x0F
	LIcKeyCode_LeftShift,		// VK_SHIFT          0x10
	LIcKeyCode_LeftControl,		// VK_CONTROL        0x11
	LIcKeyCode_AppMenuKey,		// VK_MENU           0x12
	LIcKeyCode_Pause,			// VK_PAUSE          0x13
	LIcKeyCode_CapsLock,		// VK_CAPITAL        0x14
	LIcKeyCode_None,			// VK_KANA           0x15
	LIcKeyCode_None,			//					 0x16
	LIcKeyCode_None,			// VK_JUNJA          0x17
	LIcKeyCode_None,			// VK_FINAL          0x18
	LIcKeyCode_None,			// VK_KANJI          0x19
	LIcKeyCode_None,			//					 0x1A
	LIcKeyCode_Escape,			// VK_ESCAPE         0x1B
	LIcKeyCode_None,			// VK_CONVERT        0x1C
	LIcKeyCode_None,			// VK_NONCONVERT     0x1D
	LIcKeyCode_None,			// VK_ACCEPT         0x1E
	LIcKeyCode_None,			// VK_MODECHANGE     0x1F
	LIcKeyCode_Space,			// VK_SPACE          0x20
	LIcKeyCode_PageUp,			// VK_PRIOR          0x21
	LIcKeyCode_PageDown,		// VK_NEXT           0x22
	LIcKeyCode_End,				// VK_END            0x23
	LIcKeyCode_Home,			// VK_HOME           0x24
	LIcKeyCode_LeftArrow,		// VK_LEFT           0x25
	LIcKeyCode_UpArrow,			// VK_UP             0x26
	LIcKeyCode_RightArrow,		// VK_RIGHT          0x27
	LIcKeyCode_DownArrow,		// VK_DOWN           0x28
	LIcKeyCode_None,			// VK_SELECT         0x29
	LIcKeyCode_None,			// VK_PRINT          0x2A
	LIcKeyCode_None,			// VK_EXECUTE        0x2B
	LIcKeyCode_None,			// VK_SNAPSHOT       0x2C
	LIcKeyCode_Insert,			// VK_INSERT         0x2D
	LIcKeyCode_Delete,			// VK_DELETE         0x2E
	LIcKeyCode_None,			// VK_HELP           0x2F
	LIcKeyCode_0,				//					 0x30
	LIcKeyCode_1,				//					 0x31
	LIcKeyCode_2,				//					 0x32
	LIcKeyCode_3,				//					 0x33
	LIcKeyCode_4,				//					 0x34
	LIcKeyCode_5,				//					 0x35
	LIcKeyCode_6,				//					 0x36
	LIcKeyCode_7,				//					 0x37
	LIcKeyCode_8,				//					 0x38
	LIcKeyCode_9,				//					 0x39
	LIcKeyCode_None,			//					 0x3A
	LIcKeyCode_None,			//					 0x3B
	LIcKeyCode_None,			//					 0x3C
	LIcKeyCode_None,			//					 0x3D
	LIcKeyCode_None,			//					 0x3E
	LIcKeyCode_None,			//					 0x3F
	LIcKeyCode_None,			//					 0x40
	LIcKeyCode_A,				//					 0x41
	LIcKeyCode_B,				//					 0x42
	LIcKeyCode_C,				//					 0x43
	LIcKeyCode_D,				//					 0x44
	LIcKeyCode_E,				//					 0x45
	LIcKeyCode_F,				//					 0x46
	LIcKeyCode_G,				//					 0x47
	LIcKeyCode_H,				//					 0x48
	LIcKeyCode_I,				//					 0x49
	LIcKeyCode_J,				//					 0x4A
	LIcKeyCode_K,				//					 0x4B
	LIcKeyCode_L,				//					 0x4C
	LIcKeyCode_M,				//					 0x4D
	LIcKeyCode_N,				//					 0x4E
	LIcKeyCode_O,				//					 0x4F
	LIcKeyCode_P,				//					 0x50
	LIcKeyCode_Q,				//					 0x51
	LIcKeyCode_R,				//					 0x52
	LIcKeyCode_S,				//					 0x53
	LIcKeyCode_T,				//					 0x54
	LIcKeyCode_U,				//					 0x55
	LIcKeyCode_V,				//					 0x56
	LIcKeyCode_W,				//					 0x57
	LIcKeyCode_X,				//					 0x58
	LIcKeyCode_Y,				//					 0x59
	LIcKeyCode_Z,				//					 0x5A
	LIcKeyCode_LeftWindowsKey,	// VK_LWIN           0x5B
	LIcKeyCode_RightWindowsKey,	// VK_RWIN           0x5C
	LIcKeyCode_AppMenuKey,		// VK_APPS           0x5D
	LIcKeyCode_None,			//					 0x5E
	LIcKeyCode_None,			//					 0x5F
	LIcKeyCode_NumPad0,			// VK_NUMPAD0        0x60
	LIcKeyCode_NumPad1,			// VK_NUMPAD1        0x61
	LIcKeyCode_NumPad2,			// VK_NUMPAD2        0x62
	LIcKeyCode_NumPad3,			// VK_NUMPAD3        0x63
	LIcKeyCode_NumPad4,			// VK_NUMPAD4        0x64
	LIcKeyCode_NumPad5,			// VK_NUMPAD5        0x65
	LIcKeyCode_NumPad6,			// VK_NUMPAD6        0x66
	LIcKeyCode_NumPad7,			// VK_NUMPAD7        0x67
	LIcKeyCode_NumPad8,			// VK_NUMPAD8        0x68
	LIcKeyCode_NumPad9,			// VK_NUMPAD9        0x69
	LIcKeyCode_Multiply,		// VK_MULTIPLY       0x6A
	LIcKeyCode_Add,				// VK_ADD            0x6B
	LIcKeyCode_None,			// VK_SEPARATOR      0x6C
	LIcKeyCode_Subtract,		// VK_SUBTRACT       0x6D
	LIcKeyCode_Decimal,			// VK_DECIMAL        0x6E
	LIcKeyCode_Divide,			// VK_DIVIDE         0x6F
	LIcKeyCode_F1,				// VK_F1             0x70
	LIcKeyCode_F2,				// VK_F2             0x71
	LIcKeyCode_F3,				// VK_F3             0x72
	LIcKeyCode_F4,				// VK_F4             0x73
	LIcKeyCode_F5,				// VK_F5             0x74
	LIcKeyCode_F6,				// VK_F6             0x75
	LIcKeyCode_F7,				// VK_F7             0x76
	LIcKeyCode_F8,				// VK_F8             0x77
	LIcKeyCode_F9,				// VK_F9             0x78
	LIcKeyCode_F10,				// VK_F10            0x79
	LIcKeyCode_F11,				// VK_F11            0x7A
	LIcKeyCode_F12,				// VK_F12            0x7B
	LIcKeyCode_F13,				// VK_F13            0x7C
	LIcKeyCode_F14,				// VK_F14            0x7D
	LIcKeyCode_F15,				// VK_F15            0x7E
	LIcKeyCode_None,			// VK_F16            0x7F
	LIcKeyCode_None,			// VK_F17            0x80
	LIcKeyCode_None,			// VK_F18            0x81
	LIcKeyCode_None,			// VK_F19            0x82
	LIcKeyCode_None,			// VK_F20            0x83
	LIcKeyCode_None,			// VK_F21            0x84
	LIcKeyCode_None,			// VK_F22            0x85
	LIcKeyCode_None,			// VK_F23            0x86
	LIcKeyCode_None,			// VK_F24            0x87
	LIcKeyCode_None,			//					 0x88
	LIcKeyCode_None,			//					 0x89
	LIcKeyCode_None,			//					 0x8A
	LIcKeyCode_None,			//					 0x8B
	LIcKeyCode_None,			//					 0x8C
	LIcKeyCode_None,			//					 0x8D
	LIcKeyCode_None,			//					 0x8E
	LIcKeyCode_None,			//					 0x8F
	LIcKeyCode_NumLock,			// VK_NUMLOCK        0x90
	LIcKeyCode_ScrollLock,		// VK_SCROLL         0x91
	LIcKeyCode_None,			//					 0x92
	LIcKeyCode_None,			//					 0x93
	LIcKeyCode_None,			//					 0x94
	LIcKeyCode_None,			//					 0x95
	LIcKeyCode_None,			//					 0x96
	LIcKeyCode_None,			//					 0x97
	LIcKeyCode_None,			//					 0x98
	LIcKeyCode_None,			//					 0x99
	LIcKeyCode_None,			//					 0x9A
	LIcKeyCode_None,			//					 0x9B
	LIcKeyCode_None,			//					 0x9C
	LIcKeyCode_None,			//					 0x9D
	LIcKeyCode_None,			//					 0x9E
	LIcKeyCode_None,			//					 0x9F
	LIcKeyCode_LeftShift,		// VK_LSHIFT         0xA0
	LIcKeyCode_RightShift,		// VK_RSHIFT         0xA1
	LIcKeyCode_LeftControl,		// VK_LCONTROL       0xA2
	LIcKeyCode_RightControl,	// VK_RCONTROL       0xA3
	LIcKeyCode_LeftAlt,			// VK_LMENU          0xA4
	LIcKeyCode_RightAlt,		// VK_RMENU          0xA5
	LIcKeyCode_None,			//					 0xA6
	LIcKeyCode_None,			//					 0xA7
	LIcKeyCode_None,			//					 0xA8
	LIcKeyCode_None,			//					 0xA9
	LIcKeyCode_None,			//					 0xAA
	LIcKeyCode_None,			//					 0xAB
	LIcKeyCode_None,			//					 0xAC
	LIcKeyCode_None,			//					 0xAD
	LIcKeyCode_None,			//					 0xAE
	LIcKeyCode_None,			//					 0xAF
	LIcKeyCode_None,			//					 0xB0
	LIcKeyCode_None,			//					 0xB1
	LIcKeyCode_None,			//					 0xB2
	LIcKeyCode_None,			//					 0xB3
	LIcKeyCode_None,			//					 0xB4
	LIcKeyCode_None,			//					 0xB5
	LIcKeyCode_None,			//					 0xB6
	LIcKeyCode_None,			//					 0xB7
	LIcKeyCode_None,			//					 0xB8
	LIcKeyCode_None,			//					 0xB9
	LIcKeyCode_Semicolon,		//					 0xBA
	LIcKeyCode_Equals,			//					 0xBB
	LIcKeyCode_Comma,			//					 0xBC
	LIcKeyCode_Minus,			//					 0xBD
	LIcKeyCode_Period,			//					 0xBE
	LIcKeyCode_Slash,			//					 0xBF
	LIcKeyCode_Grave,			//					 0xC0
	LIcKeyCode_None,			//					 0xC1
	LIcKeyCode_None,			//					 0xC2
	LIcKeyCode_None,			//					 0xC3
	LIcKeyCode_None,			//					 0xC4
	LIcKeyCode_None,			//					 0xC5
	LIcKeyCode_None,			//					 0xC6
	LIcKeyCode_None,			//					 0xC7
	LIcKeyCode_None,			//					 0xC8
	LIcKeyCode_None,			//					 0xC9
	LIcKeyCode_None,			//					 0xCA
	LIcKeyCode_None,			//					 0xCB
	LIcKeyCode_None,			//					 0xCC
	LIcKeyCode_None,			//					 0xCD
	LIcKeyCode_None,			//					 0xCE
	LIcKeyCode_None,			//					 0xCF
	LIcKeyCode_None,			//					 0xD0
	LIcKeyCode_None,			//					 0xD1
	LIcKeyCode_None,			//					 0xD2
	LIcKeyCode_None,			//					 0xD3
	LIcKeyCode_None,			//					 0xD4
	LIcKeyCode_None,			//					 0xD5
	LIcKeyCode_None,			//					 0xD6
	LIcKeyCode_None,			//					 0xD7
	LIcKeyCode_None,			//					 0xD8
	LIcKeyCode_None,			//					 0xD9
	LIcKeyCode_None,			//					 0xDA
	LIcKeyCode_LeftBracket,		//					 0xDB
	LIcKeyCode_BackSlash,		//					 0xDC
	LIcKeyCode_RightBracket,	//					 0xDD
	LIcKeyCode_Apostrophe,		//					 0xDE
	LIcKeyCode_None,			//					 0xDF
	LIcKeyCode_None,			//					 0xE0
	LIcKeyCode_None,			//					 0xE1
	LIcKeyCode_None,			//					 0xE2
	LIcKeyCode_None,			//					 0xE3
	LIcKeyCode_None,			//					 0xE4
	LIcKeyCode_None,			// VK_PROCESSKEY     0xE5
	LIcKeyCode_None,			//					 0xE6
	LIcKeyCode_None,			//					 0xE7
	LIcKeyCode_None,			//					 0xE8
	LIcKeyCode_None,			//					 0xE9
	LIcKeyCode_None,			//					 0xEA
	LIcKeyCode_None,			//					 0xEB
	LIcKeyCode_None,			//					 0xEC
	LIcKeyCode_None,			//					 0xED
	LIcKeyCode_None,			//					 0xEE
	LIcKeyCode_None,			//					 0xEF
	LIcKeyCode_None,			//					 0xF0
	LIcKeyCode_None,			//					 0xF1
	LIcKeyCode_None,			//					 0xF2
	LIcKeyCode_None,			//					 0xF3
	LIcKeyCode_None,			//					 0xF4
	LIcKeyCode_None,			//					 0xF5
	LIcKeyCode_None,			// VK_ATTN           0xF6
	LIcKeyCode_None,			// VK_CRSEL          0xF7
	LIcKeyCode_None,			// VK_EXSEL          0xF8
	LIcKeyCode_None,			// VK_EREOF          0xF9
	LIcKeyCode_None,			// VK_PLAY           0xFA
	LIcKeyCode_None,			// VK_ZOOM           0xFB
	LIcKeyCode_None,			// VK_NONAME         0xFC
	LIcKeyCode_None,			// VK_PA1            0xFD
	LIcKeyCode_None,			// VK_OEM_CLEAR      0xFE
};

static UUtUns8 LIgVKKeyboardTranslation_Reverse[256];
static UUtUns8 LIgDIKeyboardTranslation_Reverse[256];
static UUtUns8 global_keys[256]= {0};

// ======================================================================
// prototypes
// ======================================================================
static char*
LIiGetDIErrorMsg(
	HRESULT					inError);
	
// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
LIiPlatform_Acquire(
	void)
{
	HRESULT					result;
	
	// acquire the mouse
	if (LIgMouse)
	{
		result = IDirectInputDevice_Acquire(LIgMouse);
		if (result != DI_OK)
		{
			UUrError_Report(UUcError_Generic, LIiGetDIErrorMsg(result));
		}
	}
	
	// acquire the keyboard
	if (LIgKeyboard)
	{
		result = IDirectInputDevice_Acquire(LIgKeyboard);
		if (result != DI_OK)
		{
			UUrError_Report(UUcError_Generic, LIiGetDIErrorMsg(result));
		}
	}
}

// ----------------------------------------------------------------------
static void
LIiPlatform_Unacquire(
	void)
{
	HRESULT					result;
	
	// unacquire the mouse
	if (LIgMouse)
	{
		result = IDirectInputDevice_Unacquire(LIgMouse);
		if ((result != DI_OK) && (result != DI_NOEFFECT))
		{
			UUrError_Report(UUcError_Generic, LIiGetDIErrorMsg(result));
		}
	}

	// unaacquire the keyboard
	if (LIgKeyboard)
	{
		result = IDirectInputDevice_Unacquire(LIgKeyboard);
		if ((result != DI_OK) && (result != DI_NOEFFECT))
		{
			UUrError_Report(UUcError_Generic, LIiGetDIErrorMsg(result));
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static BOOL CALLBACK
LIiPlatform_Mouse_Callback(
	LPCDIDEVICEINSTANCE		inMouseInstance,
	LPVOID 					inSomething)
{
	HRESULT					result;
	LPDIRECTINPUTDEVICE		temp_device;
	
	// create the device
	result =
		IDirectInput_CreateDevice(
			LIgDirectInput,
			&GUID_SysMouse,
			&temp_device,
			NULL);
	if (result != DI_OK)
	{
		UUrError_Report(UUcError_Generic, LIiGetDIErrorMsg(result));
		return DIENUM_STOP;
	}
	
	// set the data format
	result = IDirectInputDevice_SetDataFormat(temp_device, &c_dfDIMouse);
	if (result != DI_OK)
	{
		UUrError_Report(UUcError_Generic, LIiGetDIErrorMsg(result));
		return DIENUM_STOP;
	}
	
	// set the cooperative level
	result =
		IDirectInputDevice_SetCooperativeLevel(
				temp_device,
				LIgWindow,
//				DISCL_EXCLUSIVE | DISCL_FOREGROUND);
				DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	if (result != DI_OK)
	{
		UUrError_Report(UUcError_Generic, LIiGetDIErrorMsg(result));
		return DIENUM_STOP;
	}
	
	// set the device global
	LIgMouse = temp_device;
	
	// no more
	return DIENUM_STOP;
}

// ----------------------------------------------------------------------
#if UUmCompiler == UUmCompiler_MWerks
#pragma profile off
#endif


static void CenterCursor(void)
{
	// center the cursor
	if (LIgSetCursorPosThisFrame && LIgSetCursorPosEver) {
		SetCursorPos(LIgCenter.x, LIgCenter.y);
	}

	return;
}

static UUtBool
LIiPlatform_Mouse_GetData_DirectInput(
	DIMOUSESTATE			*outMouseData)
{
	HRESULT					result;
	
	// get the device state
	result= IDirectInputDevice_GetDeviceState(LIgMouse, sizeof(DIMOUSESTATE), outMouseData);
	if (result != DI_OK)
	{
		if (result == DIERR_INPUTLOST)
		{	
			LIiPlatform_Acquire();
			result= IDirectInputDevice_GetDeviceState(LIgMouse, sizeof(DIMOUSESTATE), outMouseData);
		}
	}

	CenterCursor();
	
	return (result == DI_OK);
}

// ----------------------------------------------------------------------
static UUtBool
LIiPlatform_Mouse_GetData_Win32(
	DIMOUSESTATE			*outMouseData)
{
	UUtBool					success;
	POINT					cursor_pos;

	if (GetCursorPos(&cursor_pos))
	{
		// get the axis data
		outMouseData->lX = cursor_pos.x - LIgCenter.x;
		outMouseData->lY = cursor_pos.y - LIgCenter.y;
		outMouseData->lZ = 0;
		
		// get the button data
		outMouseData->rgbButtons[0] = GetAsyncKeyState(MK_LBUTTON) >> 8;
		outMouseData->rgbButtons[1] = GetAsyncKeyState(MK_RBUTTON) >> 8;
		outMouseData->rgbButtons[2] = GetAsyncKeyState(MK_MBUTTON) >> 8;
		outMouseData->rgbButtons[3] = UUcFalse;
		
		CenterCursor();
		
		success = UUcTrue;
	}
	else
	{
		success = UUcFalse;
	}

	return success;
}

// ----------------------------------------------------------------------
static void
LIiPlatform_Mouse_GetData(
	LItAction				*outAction)
{
	UUtBool					got_data;
	DIMOUSESTATE			mouse_data;
	LItDeviceInput			deviceInput;
	
	// get the mouse data
	if (LIgUseDirectInput)
	{
		got_data = LIiPlatform_Mouse_GetData_DirectInput(&mouse_data);
	}
	else
	{
		got_data = LIiPlatform_Mouse_GetData_Win32(&mouse_data);
	}
	if (got_data == UUcFalse) return;
	
	// interpret the data
	if (mouse_data.rgbButtons[0] == 0x80)
	{
		deviceInput.input = LIcMouseCode_Button1;
		deviceInput.analogValue = 1.0f;
		LIrActionBuffer_Add(outAction, &deviceInput);
	}
	
	if (mouse_data.rgbButtons[1] == 0x80)
	{
		deviceInput.input = LIcMouseCode_Button2;
		deviceInput.analogValue = 1.0f;
		LIrActionBuffer_Add(outAction, &deviceInput);
	}
	
	if (mouse_data.rgbButtons[2] == 0x80)
	{
		deviceInput.input = LIcMouseCode_Button3;
		deviceInput.analogValue = 1.0f;
		LIrActionBuffer_Add(outAction, &deviceInput);
	}
	
	if (mouse_data.rgbButtons[3] == 0x80)
	{
		deviceInput.input = LIcMouseCode_Button4;
		deviceInput.analogValue = 1.0f;
		LIrActionBuffer_Add(outAction, &deviceInput);
	}
	
	deviceInput.input = LIcMouseCode_XAxis;
	deviceInput.analogValue =
		(float)UUmPin((mouse_data.lX << 1), UUcMinInt32, UUcMaxInt32) / 8.0f;
	LIrActionBuffer_Add(outAction, &deviceInput);

	deviceInput.input = LIcMouseCode_YAxis;
	if (LIgMouse_Invert)
	{
		deviceInput.analogValue =
			-(float)UUmPin((mouse_data.lY << 1), UUcMinInt32, UUcMaxInt32) / 8.0f;
	}
	else
	{
		deviceInput.analogValue =
			(float)UUmPin((mouse_data.lY << 1), UUcMinInt32, UUcMaxInt32) / 8.0f;
	}
	LIrActionBuffer_Add(outAction, &deviceInput);

	deviceInput.input = LIcMouseCode_ZAxis;
	deviceInput.analogValue =
		(float)UUmPin((mouse_data.lZ << 1), UUcMinInt32, UUcMaxInt32) / 8.0f;
	LIrActionBuffer_Add(outAction, &deviceInput);
}

#if UUmCompiler == UUmCompiler_MWerks
#pragma profile reset
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static BOOL CALLBACK
LIiPlatform_Keyboard_Callback(
	LPCDIDEVICEINSTANCE		inKeyboardInstance,
	LPVOID 					inSomething)
{
	HRESULT					result;
	LPDIRECTINPUTDEVICE		temp_device;
	
	// create the device
	result =
		IDirectInput_CreateDevice(
			LIgDirectInput,
			&GUID_SysKeyboard,
			&temp_device,
			NULL);
	if (result != DI_OK)
	{
		UUrError_Report(UUcError_Generic, LIiGetDIErrorMsg(result));
		return DIENUM_STOP;
	}
	
	// set the data format
	result = IDirectInputDevice_SetDataFormat(temp_device, &c_dfDIKeyboard);
	if (result != DI_OK)
	{
		UUrError_Report(UUcError_Generic, LIiGetDIErrorMsg(result));
		return DIENUM_STOP;
	}
	
	// set the cooperative level
	result =
		IDirectInputDevice_SetCooperativeLevel(
				temp_device,
				LIgWindow,
//				DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
				DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	if (result != DI_OK)
	{
		UUrError_Report(UUcError_Generic, LIiGetDIErrorMsg(result));
		return DIENUM_STOP;
	}
	
	// set the device global
	LIgKeyboard = temp_device;
	
	// no more
	return DIENUM_STOP;
}

// ----------------------------------------------------------------------
#if UUmCompiler == UUmCompiler_MWerks
#pragma profile off
#endif

static UUtBool
LIiPlatform_Keyboard_GetData_DirectInput(
	char					*keys)
{
	HRESULT					result;
	
	// set the device state
	result= IDirectInputDevice_GetDeviceState(LIgKeyboard, 256, keys);
	if (result != DI_OK)
	{
		if (result == DIERR_INPUTLOST)
		{	
			LIiPlatform_Acquire();
			result= IDirectInputDevice_GetDeviceState(LIgKeyboard, 256, keys);
		}
		else
		{
			LIiGetDIErrorMsg(result);
		}
	}
	
	return (result == DI_OK);
}

// ----------------------------------------------------------------------
static UUtBool
LIiPlatform_Keyboard_GetData_Win32(
	char					*keys)
{
	UUtUns16				i;
	
	for (i = 0; i < 256; i++)
	{
		if (LIgVKKeyboardTranslation[i] != LIcKeyCode_None) 
		{
			keys[i] = GetAsyncKeyState(i) >> 8;
		}
		else 
		{
			keys[i] = 0;
		}
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
LIiPlatform_Keyboard_GetData(
	LItAction				*outAction)
{
	UUtBool					got_data;
	char					keys[256];
	UUtUns16				i;
	const UUtUns8					*translation;
	
	// get the data
	if (LIgUseDirectInput)
	{
		got_data = LIiPlatform_Keyboard_GetData_DirectInput(keys);
		translation = LIgDIKeyboardTranslation;
	}
	else
	{
		got_data = LIiPlatform_Keyboard_GetData_Win32(keys);
		translation = LIgVKKeyboardTranslation;
	}
	if (got_data == UUcFalse) return;
	
	// interpret the data
	for (i = 0; i < 256; i++)
	{
		if (keys[i] & 0x80)
		{
			LItDeviceInput			deviceInput;
			
			deviceInput.input = translation[i];
			deviceInput.analogValue = 1.0f;
			
			if (deviceInput.input != LIcKeyCode_None) 
			{
				LIrActionBuffer_Add(outAction, &deviceInput);
			}
		}
	}
}

#if UUmCompiler == UUmCompiler_MWerks
#pragma profile reset
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
LIiPlatform_Devices_Initialize(
	void)
{
	HRESULT					result;
	
	// enumerate the mouse
	result =
		IDirectInput_EnumDevices(
			LIgDirectInput,
			DIDEVTYPE_MOUSE,
			LIiPlatform_Mouse_Callback,
			NULL,
			DIEDFL_ATTACHEDONLY);
	if (result != DI_OK)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic,	LIiGetDIErrorMsg(result));
	}
	
	// enumerate the keyboard
	result =
		IDirectInput_EnumDevices(
			LIgDirectInput,
			DIDEVTYPE_KEYBOARD,
			LIiPlatform_Keyboard_Callback,
			NULL,
			DIEDFL_ATTACHEDONLY);
	if (result != DI_OK)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic,	LIiGetDIErrorMsg(result));
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
#if UUmCompiler == UUmCompiler_MWerks
#pragma profile off
#endif

static void
LIiPlatform_Devices_GetData(
	LItAction				*outAction)
{
	// get mouse data
	if (LIcMode_Game == LIgMode_Internal)
	{
		LIiPlatform_Mouse_GetData(outAction);
	}
	
	// get the keyboard data
	LIiPlatform_Keyboard_GetData(outAction);
}

#if UUmCompiler == UUmCompiler_MWerks
#pragma profile reset
#endif

// ----------------------------------------------------------------------
static void
LIiPlatform_Devices_Terminate(
	void)
{
	// release the mouse
	if (LIgMouse)
	{
		IDirectInputDevice_Release(LIgMouse);
	}
	
	// release the keyboard
	if (LIgKeyboard)
	{
		IDirectInputDevice_Release(LIgKeyboard);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
LIrPlatform_InputEvent_InterpretModifiers(
	LItInputEventType	inEventType,
	UUtUns32			inModifiers)
{
	UUtUns32			outModifiers;
	
	outModifiers = 0;
	
	switch (inEventType)
	{
		case LIcInputEvent_MouseMove:
		case LIcInputEvent_LMouseDown:
		case LIcInputEvent_LMouseUp:
		case LIcInputEvent_MMouseDown:
		case LIcInputEvent_MMouseUp:
		case LIcInputEvent_RMouseDown:
		case LIcInputEvent_RMouseUp:
			if (inModifiers & MK_SHIFT)
				outModifiers |= LIcMouseState_ShiftDown;
			if (inModifiers & MK_CONTROL)
				outModifiers |= LIcMouseState_ControlDown;
			if (inModifiers & MK_RBUTTON)
				outModifiers |= LIcMouseState_RButtonDown;
			if (inModifiers & MK_LBUTTON)
				outModifiers |= LIcMouseState_LButtonDown;
		break;
		
		case LIcInputEvent_KeyUp:
		case LIcInputEvent_KeyDown:
		case LIcInputEvent_KeyRepeat:
			if (inModifiers & MK_SHIFT)
				outModifiers |= LIcKeyState_ShiftDown;
			if (inModifiers & MK_CONTROL)
				outModifiers |= LIcKeyState_CommandDown;
			if (inModifiers & MK_ALT)
				outModifiers |= LIcKeyState_AltDown;
		break;
	}
	
	
	return outModifiers;
}

// ----------------------------------------------------------------------
void
LIrPlatform_InputEvent_GetMouse(
	LItMode				inMode,
	LItInputEvent		*outInputEvent)
{
	// set up the outInputEvent
	outInputEvent->type			= LIcInputEvent_None;
	outInputEvent->where.x		= 0;
	outInputEvent->where.y		= 0;
	outInputEvent->key			= 0;
	outInputEvent->modifiers	= 0;

	if (inMode == LIcMode_Normal)
	{
		BOOL				status;
		POINT				mouse_position;
		
		// get the current position of the cursor
		status = GetCursorPos(&mouse_position);
		if (status)
		{
			outInputEvent->where.x = (UUtInt16)mouse_position.x;
			outInputEvent->where.y = (UUtInt16)mouse_position.y;
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
LIrPlatform_Mode_Set(
	LItMode				inMode)
{
	if (inMode == LIcMode_Normal)
	{
		// unacquire the input devices
		LIiPlatform_Unacquire();
	}
	else
	{
		if (LIgUseDirectInput)
		{
			// acquire the input devices
			LIiPlatform_Acquire();
		}

		{
			RECT				window_rect;
		
			// set the cursor center position based on the rect of the window being used
			if (GetWindowRect(LIgWindow, &window_rect))
			{
				LIgSetCursorPosThisFrame = UUcTrue;
				LIgCenter.x = (window_rect.right - window_rect.left) / 2;
				LIgCenter.y = (window_rect.bottom - window_rect.top) / 2;
			}
			else
			{
				LIgSetCursorPosThisFrame = UUcFalse;
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
UUtError
LIrPlatform_Initialize(
	UUtAppInstance		inInstance,
	UUtWindow			inWindow)
{
	UUtError			error;
	HRESULT				result;
	OSVERSIONINFO		info;
	
	UUmAssert(inInstance);
	UUmAssert(inWindow);
	
	// ------------------------------
	// initialize the globals
	// ------------------------------
	LIgUseDirectInput			= UUcTrue;
	LIgDirectInput				= NULL;
	LIgCenter.x					= 0;
	LIgCenter.y					= 0;
	LIgWindow					= inWindow;
	LIgSetCursorPosThisFrame	= UUcFalse;
	
	// find out which OS the game is running under
	info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&info))
	{
		// don't use DirectInput under NT
		if (VER_PLATFORM_WIN32_NT == info.dwPlatformId) {
			LIgUseDirectInput = UUcFalse;
		}
	}

	// ------------------------------
	// initialize DirectInput
	// ------------------------------
	if (LIgUseDirectInput)
	{
		// start direct input
		result =
			DirectInputCreate(
				inInstance,
				DIRECTINPUT_VERSION,
				&LIgDirectInput,
				NULL);
		if (result != DI_OK)
		{
			UUmError_ReturnOnErrorMsg(
				UUcError_Generic,
				"DirectInputCreate failed.");
		}
		
		// initialize the devices
		error = LIiPlatform_Devices_Initialize();
		UUmError_ReturnOnError(error);
	}

	// ------------------------------
	// console variables
	// ------------------------------

#if THE_DAY_IS_MINE
	error =	SLrGlobalVariable_Register_Bool("li_center_cursor", "Should we center the cursor", &LIgSetCursorPosEver);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new console variable.");	
#endif

	//
	// build the reverse maps
	//

	{
		UUtUns32 i;

		UUrMemory_Clear(LIgVKKeyboardTranslation_Reverse, 256);
		UUrMemory_Clear(LIgDIKeyboardTranslation_Reverse, 256);
		for (i=0; i < 256; i++)
		{
			LIgVKKeyboardTranslation_Reverse[LIgVKKeyboardTranslation[i]] = (UUtUns8)i;
			LIgDIKeyboardTranslation_Reverse[LIgDIKeyboardTranslation[i]]= (UUtUns8)i;
		}
	}
		
	return UUcError_None;
}

// ----------------------------------------------------------------------
#if UUmCompiler == UUmCompiler_MWerks
#pragma profile off
#endif

void
LIrPlatform_PollInputForAction(
	LItAction				*outAction)
{
	UUtUns16				i;
	
	// clear the action
	outAction->buttonBits = 0;
	for (i = 0; i < LIcNumAnalogValues; i++)
		outAction->analogValues[i] = 0.0f;
	
	// get the action data
	LIiPlatform_Devices_GetData(outAction);
}

#if UUmCompiler == UUmCompiler_MWerks
#pragma profile reset
#endif

// ----------------------------------------------------------------------
void
LIrPlatform_Terminate(
	void)
{
	// unacquire the devices
	LIiPlatform_Unacquire();
	
	// terminate the devices
	LIiPlatform_Devices_Terminate();
	
	// stop Direct Input
	if (LIgDirectInput)
	{
		IDirectInput_Release(LIgDirectInput);
		LIgDirectInput = NULL;
	}
}

// ----------------------------------------------------------------------
UUtBool
LIrPlatform_Update(
	LItMode					inMode)
{
	BOOL					was_event;
	UUtUns16				itr = LIcPlatform_MaxRetries;
	
	// check the windows message queue LIcPlatform_MaxRetries times
	do
	{
		MSG					message;

		// get keyboard events and mouse events for the window associate
		// with this local input context
		was_event =
			PeekMessage(
				&message,
				LIgWindow,
				0,
				0,
				PM_REMOVE);
		if (was_event)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}
	while (--itr);
	
	return was_event;
}

// ----------------------------------------------------------------------
UUtUns32
LIrPlatform_Win32_TranslateVirtualKey(
	UUtUns32				inVirtualKey)
{
	return (UUtUns32)LIgVKKeyboardTranslation[inVirtualKey];
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static char*
LIiGetDIErrorMsg(
	HRESULT					inError)
{
	char					*errorMsg;

	switch (inError)
	{
		case S_FALSE:
			errorMsg = "S_FALSE";
			break;
		case DI_POLLEDDEVICE:
			errorMsg = "DI_POLLEDDEVICE";
			break;
		case DIERR_OLDDIRECTINPUTVERSION:
			errorMsg = "DIERR_OLDDIRECTINPUTVERSION";
			break;
		case DIERR_BETADIRECTINPUTVERSION:
			errorMsg = "DIERR_BETADIRECTINPUTVERSION";
			break;
		case DIERR_BADDRIVERVER:
			errorMsg = "DIERR_BADDRIVERVER";
			break;
		case DIERR_DEVICENOTREG:
			errorMsg = "DIERR_DEVICENOTREG";
			break;
		case DIERR_OBJECTNOTFOUND:
			errorMsg = "DIERR_OBJECTNOTFOUND";
			break;
		case DIERR_INVALIDPARAM:
			errorMsg = "DIERR_INVALIDPARAM";
			break;
		case DIERR_NOINTERFACE:
			errorMsg = "DIERR_NOINTERFACE";
			break;
		case DIERR_GENERIC:
			errorMsg = "DIERR_GENERIC";
			break;
		case DIERR_OUTOFMEMORY:
			errorMsg = "DIERR_OUTOFMEMORY";
			break;
		case DIERR_UNSUPPORTED:
			errorMsg = "DIERR_UNSUPPORTED";
			break;
		case DIERR_NOTINITIALIZED:
			errorMsg = "DIERR_NOTINITIALIZED";
			break;
		case DIERR_ALREADYINITIALIZED:
			errorMsg = "DIERR_ALREADYINITIALIZED";
			break;
		case DIERR_NOAGGREGATION:
			errorMsg = "DIERR_NOAGGREGATION";
			break;
		case DIERR_OTHERAPPHASPRIO:
			errorMsg = "DIERR_OTHERAPPHASPRIO";
			break;
		case DIERR_INPUTLOST:
			errorMsg = "DIERR_INPUTLOST";
			break;
		case DIERR_ACQUIRED:
			errorMsg = "DIERR_ACQUIRED";
			break;
		case DIERR_NOTACQUIRED:
			errorMsg = "DIERR_NOTACQUIRED";
			break;
		default:
			errorMsg = "Unknown DIERR_";
			break;
	}

	return errorMsg;
}


static BYTE test_key_cache[256];
UUtUns32 test_key_cache_time = UUcMaxUns32;

static UUtBool
LIiPlatform_TestKey_Win32(LItKeyCode inKeyCode)
{
	UUtBool keyDown;
	UUtUns32 time = UUrMachineTime_Sixtieths();

	if (time != test_key_cache_time) {
		test_key_cache_time = time;
		LIiPlatform_Keyboard_GetData_Win32((char*)test_key_cache);
	}

	keyDown= (LIgVKKeyboardTranslation_Reverse[inKeyCode] != 0) && (test_key_cache[LIgVKKeyboardTranslation_Reverse[inKeyCode]] & 0x80);

	return keyDown;
}

static UUtBool
LIiPlatform_TestKey_DirectInput(LItKeyCode inKeyCode)
{
	UUtBool keyDown;
	UUtUns32 time = UUrMachineTime_Sixtieths();

	if (time != test_key_cache_time) {
		test_key_cache_time = time;
		LIiPlatform_Keyboard_GetData_DirectInput((char*) test_key_cache);
	}

	keyDown= (LIgDIKeyboardTranslation_Reverse[inKeyCode] != 0) && (test_key_cache[LIgDIKeyboardTranslation_Reverse[inKeyCode]] & 0x80);

	return keyDown;
}

UUtBool
LIrPlatform_TestKey(
	LItKeyCode			inKeyCode,
	LItMode				inMode)
{
	UUtBool					keyDown;

	if ((LIgUseDirectInput) && (inMode == LIcMode_Game))
	{
		keyDown= LIiPlatform_TestKey_DirectInput(inKeyCode);
	}
	else
	{
		keyDown= LIiPlatform_TestKey_Win32(inKeyCode);
	}

	return keyDown;
}
