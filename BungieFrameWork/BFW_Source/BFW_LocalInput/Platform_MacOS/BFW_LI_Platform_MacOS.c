// ======================================================================
// BFW_LI_Platform_MacOS.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_LocalInput.h"
#include "BFW_LI_Platform.h"
#include "BFW_LI_Private.h"
#include "BFW_WindowManager_Public.h"
#include "Oni_Platform_Mac.h"

#include <CFBundle.h>

#define USE_OLD_INPUT_SPROCKET_LABELS	0
#define USE_OLD_ISPNEED_STRUCT			0
#include <InputSprocket.h>

// define this if we are using a Mac menu bar
//#define USE_MAC_MENU

#if UUmCompiler == UUmCompiler_MWerks
#pragma profile off
#endif

// ======================================================================
// defines
// ======================================================================
#define LIcMacOS_RightMouseButtonKey	LIcMacOS_ControlKey

#define LIcNumDevices					10
#define LIcNumElements					10

#define LIcMouse_MaxMice				3
#define LIcMouse_MaxButtons				4

// ======================================================================
// enums
// ======================================================================
enum
{
	LIcMacOS_LShiftKey					= shiftKey,
	LIcMacOS_RShiftKey					= rightShiftKey,
	LIcMacOS_ShiftKey					= (LIcMacOS_LShiftKey | LIcMacOS_RShiftKey),
	LIcMacOS_LControlKey				= controlKey,
	LIcMacOS_RControlKey				= rightControlKey,
	LIcMacOS_ControlKey					= (LIcMacOS_LControlKey | LIcMacOS_RControlKey),
	LIcMacOS_LOptionKey					= optionKey,
	LIcMacOS_ROptionKey					= rightOptionKey,
	LIcMacOS_OptionKey					= (LIcMacOS_LOptionKey | LIcMacOS_ROptionKey),
	LIcMacOS_LCommandKey				= cmdKey,
	LIcMacOS_RCommandKey				= cmdKey,
	LIcMacOS_CommandKey					= (LIcMacOS_LCommandKey | LIcMacOS_RCommandKey)
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct LItKeyCodeHelper
{
	UUtUns8							ascii;
	UUtUns8							keyCode;

} LItKeyCodeHelper;

typedef struct LItMouse
{
	ISpDeviceReference		mouse;

	UUtUns32				num_buttons;
	ISpElementReference		button[LIcMouse_MaxButtons];

	ISpElementReference		xAxis;
	ISpElementReference		yAxis;
	ISpElementReference		zAxis;

} LItMouse;

// ======================================================================
// globals
// ======================================================================
static UUtWindow			LIgWindow;
static IMtPoint2D			LIgMouseLocation;

static UUtUns32				LIgMouse_NumMice= 0L;
static LItMouse				LIgMouse[LIcMouse_MaxMice];

static const float LIcFixed_to_Float			= 1.0 / (float)((UUtInt32)0x0000FFFF);

struct mac_oni_key {
	UUtUns8 oni_key;
	UUtUns8 mac_key;
};

static struct mac_oni_key mac_keyboard_translation_table[]=
{
	{LIcKeyCode_Grave, 0x32}, // ~ 0x32
	{LIcKeyCode_1, 0x12}, // 1 0x12
	{LIcKeyCode_2, 0x13}, // 2 0x13
	{LIcKeyCode_3, 0x14}, // 3 0x14
	{LIcKeyCode_4, 0x15}, // 4 0x15
	{LIcKeyCode_5, 0x17}, // 5 0x17
	{LIcKeyCode_6, 0x16}, // 6 0x16
	{LIcKeyCode_7, 0x1A}, // 7 0x1A
	{LIcKeyCode_8, 0x1C}, // 8 0x1C
	{LIcKeyCode_9, 0x19}, // 9 0x19
	{LIcKeyCode_0, 0x1D}, // 0 0x1D
	{LIcKeyCode_Minus, 0x1B}, // - 0x1B
	{LIcKeyCode_Equals, 0x18}, // = 0x18
	{LIcKeyCode_BackSpace, 0x33}, // delete 0x33
	{LIcKeyCode_Tab, 0x30}, // tab 0x30
	{LIcKeyCode_Q, 0x0C}, // Q 0x0C
	{LIcKeyCode_W, 0x0D}, // W 0x0D
	{LIcKeyCode_E, 0x0E}, // E 0x0E
	{LIcKeyCode_R, 0x0F}, // R 0x0F
	{LIcKeyCode_T, 0x11}, // T 0x11
	{LIcKeyCode_Y, 0x10}, // Y 0x10
	{LIcKeyCode_U, 0x20}, // U 0x20
	{LIcKeyCode_I, 0x22}, // I 0x22
	{LIcKeyCode_O, 0x1F}, // O 0x1F
	{LIcKeyCode_P, 0x23}, // P 0x23
	{LIcKeyCode_LeftBracket, 0x21}, // [ 0x21
	{LIcKeyCode_RightBracket, 0x1E}, // ] 0x1E
	{LIcKeyCode_BackSlash, 0x2A}, // \ 0x2A
	{LIcKeyCode_CapsLock, 0x39}, // caps 0x39
	{LIcKeyCode_A, 0x00}, // A 0x00
	{LIcKeyCode_S, 0x01}, // S 0x01
	{LIcKeyCode_D, 0x02}, // D 0x02
	{LIcKeyCode_F, 0x03}, // F 0x03
	{LIcKeyCode_G, 0x05}, // G 0x05
	{LIcKeyCode_H, 0x04}, // H 0x04
	{LIcKeyCode_J, 0x26}, // J 0x26
	{LIcKeyCode_K, 0x28}, // K 0x28
	{LIcKeyCode_L, 0x25}, // L 0x25
	{LIcKeyCode_Semicolon, 0x29}, // ; 0x29
	{LIcKeyCode_Apostrophe, 0x27}, // ' 0x27
	{LIcKeyCode_Return, 0x24}, // return 0x24
	{LIcKeyCode_LeftShift, 0x38}, // shift 0x38
	{LIcKeyCode_Z, 0x06}, // Z 0x06
	{LIcKeyCode_X, 0x07}, // X 0x07
	{LIcKeyCode_C, 0x08}, // C 0x08
	{LIcKeyCode_V, 0x09}, // V 0x09
	{LIcKeyCode_B, 0x0B}, // B 0x0B
	{LIcKeyCode_N, 0x2D}, // N 0x2D
	{LIcKeyCode_M, 0x2E}, // M 0x2E
	{LIcKeyCode_Comma, 0x2B}, // , 0x2B
	{LIcKeyCode_Period, 0x2F}, // . 0x2F
	{LIcKeyCode_Slash, 0x2C}, // / 0x2C
	{LIcKeyCode_LeftControl, 0x3B}, // control 0x3B
	{LIcKeyCode_LeftOption, 0x3A}, // option 0x3A
	{LIcKeyCode_LeftAlt, 0x37}, // command 0x37
	{LIcKeyCode_Space, 0x31}, // space 0x31
	{LIcKeyCode_UpArrow, 0x7E}, // up 0x7E
	{LIcKeyCode_DownArrow, 0x7D}, // down 0x7D
	{LIcKeyCode_LeftArrow, 0x7B}, // left 0x7B
	{LIcKeyCode_RightArrow, 0x7C}, // right 0x7C
	{LIcKeyCode_NumPadEquals, 0x51}, // num= 0x51
	{LIcKeyCode_Divide, 0x4B}, // num/ 0x4B
	{LIcKeyCode_Multiply, 0x43}, // num* 0x43
	{LIcKeyCode_Subtract, 0x4E}, // num- 0x4E
	{LIcKeyCode_Add, 0x45}, // num+ 0x45
	{LIcKeyCode_NumPadEnter, 0x4C}, // num_enter 0x4C
	{LIcKeyCode_Decimal, 0x41}, // num. 0x41
	{LIcKeyCode_NumPad0, 0x52}, // num0 0x52
	{LIcKeyCode_NumPad1, 0x53}, // num1 0x53
	{LIcKeyCode_NumPad2, 0x54}, // num2 0x54
	{LIcKeyCode_NumPad3, 0x55}, // num3 0x55
	{LIcKeyCode_NumPad4, 0x56}, // num4 0x56
	{LIcKeyCode_NumPad5, 0x57}, // num5 0x57
	{LIcKeyCode_NumPad6, 0x58}, // num6 0x58
	{LIcKeyCode_NumPad7, 0x59}, // num7 0x59
	{LIcKeyCode_NumPad8, 0x5B}, // num8 0x5B
	{LIcKeyCode_NumPad9, 0x5C}, // num9 0x5C
	{LIcKeyCode_Escape, 0x35}, // esc 0x35
	{LIcKeyCode_NumLock, 0x47}, // clear 0x47
	{LIcKeyCode_PageUp, 0x74}, // pgup 0x74
	{LIcKeyCode_PageDown, 0x79}, // pgdown 0x79
	{LIcKeyCode_Home, 0x73}, // home 0x73
	{LIcKeyCode_End, 0x77}, // end 0x77
	{LIcKeyCode_Delete, 0x75}, // forward_delete 0x75
	{LIcKeyCode_Insert, 0x72}, // help 0x72
	{LIcKeyCode_F1, 0x7A}, // f1 0x7A
	{LIcKeyCode_F2, 0x78}, // f2 0x78
	{LIcKeyCode_F3, 0x63}, // f3 0x63
	{LIcKeyCode_F4, 0x76}, // f4 0x76
	{LIcKeyCode_F5, 0x60}, // f5 0x60
	{LIcKeyCode_F6, 0x61}, // f6 0x61
	{LIcKeyCode_F7, 0x62}, // f7 0x62
	{LIcKeyCode_F8, 0x64}, // f8 0x64
	{LIcKeyCode_F9, 0x65}, // f9 0x65
	{LIcKeyCode_F10, 0x6D}, // f10 0x6D
	{LIcKeyCode_F11, 0x67}, // f11 0x67
	{LIcKeyCode_F12, 0x6F}, // f12 0x6F
	{LIcKeyCode_F13, 0x69}, // f13 0x69
	{LIcKeyCode_F14, 0x6B}, // f14 0x6B
	{LIcKeyCode_F15, 0x71} // f15 0x71
};

static const int num_mac_keys= sizeof(mac_keyboard_translation_table)/sizeof(struct mac_oni_key);

static LItKeyCodeHelper			LIgKeyCodeHelper[] =
{
	{ kHomeCharCode,			LIcKeyCode_Home				},
	{ kEnterCharCode,			LIcKeyCode_NumPadEnter		},
	{ kEndCharCode,				LIcKeyCode_End				},
	{ kHelpCharCode,			LIcKeyCode_Insert			},
	{ kBackspaceCharCode,		LIcKeyCode_BackSpace		},
	{ kTabCharCode,				LIcKeyCode_Tab				},
	{ kPageUpCharCode,			LIcKeyCode_PageUp			},
	{ kPageDownCharCode,		LIcKeyCode_PageDown			},
	{ kReturnCharCode,			LIcKeyCode_Return			},
	{ kFunctionKeyCharCode,		LIcKeyCode_F1				},
	{ kEscapeCharCode,			LIcKeyCode_Escape			},
	{ kLeftArrowCharCode,		LIcKeyCode_LeftArrow		},
	{ kRightArrowCharCode,		LIcKeyCode_RightArrow		},
	{ kUpArrowCharCode,			LIcKeyCode_UpArrow			},
	{ kDownArrowCharCode,		LIcKeyCode_DownArrow		},
	{ kDeleteCharCode,			LIcKeyCode_Delete			},
	{ 0x0000,					LIcKeyCode_None				}
};

#define LIcFKeyStart			0x00006000

static UUtUns8					LIgFunctionKey[] =
{
	/* 0x6000 */	LIcKeyCode_F5,
	/* 0x6100 */	LIcKeyCode_F6,
	/* 0x6200 */	LIcKeyCode_F7,
	/* 0x6300 */	LIcKeyCode_F3,
	/* 0x6400 */	LIcKeyCode_F8,
	/* 0x6500 */	LIcKeyCode_F9,
	/* 0x6600 */	LIcKeyCode_None,
	/* 0x6700 */	LIcKeyCode_F11,
	/* 0x6800 */	LIcKeyCode_None,
	/* 0x6900 */	LIcKeyCode_F13,
	/* 0x6A00 */	LIcKeyCode_None,
	/* 0x6B00 */	LIcKeyCode_F14,
	/* 0x6C00 */	LIcKeyCode_None,
	/* 0x6D00 */	LIcKeyCode_F10,
	/* 0x6E00 */	LIcKeyCode_None,
	/* 0x6F00 */	LIcKeyCode_F12,
	/* 0x7000 */	LIcKeyCode_None,
	/* 0x7100 */	LIcKeyCode_F15,
	/* 0x7200 */	LIcKeyCode_None,
	/* 0x7300 */	LIcKeyCode_None,
	/* 0x7400 */	LIcKeyCode_None,
	/* 0x7500 */	LIcKeyCode_None,
	/* 0x7600 */	LIcKeyCode_F4,
	/* 0x7700 */	LIcKeyCode_None,
	/* 0x7800 */	LIcKeyCode_F2,
	/* 0x7900 */	LIcKeyCode_None,
	/* 0x7A00 */	LIcKeyCode_F1,
};

static UUtUns8					LIgAsciiToKeyCode[256];
static UUtBool					using_input_sprocket= UUcFalse;

static Boolean mac_cursor_visible= true;
static CursHandle oni_cursor= NULL; // load a special invisible cursor, as other things tend to show the cursor when we aren't looking

/*---------- code */

void mac_hide_cursor(
	void)
{
	extern UUtBool M3gResolutionSwitch; // BFW_Motoko.h

	if (SHIPPING_VERSION) // never hide cursor if running a non-shipping version
	{
		extern WindowPtr oni_mac_window; // Oni_Platform_Mac.c
		Rect window_rect;

		// load the invisible cursor only if running full screen
		if ((oni_cursor == NULL) && M3gResolutionSwitch)
		{
			oni_cursor= GetCursor(CURS_RES_ID);
		}

		if (oni_cursor)
		{
			HLock(oni_cursor);
			SetCursor(*oni_cursor);
		}

		if (oni_mac_window &&
			GetWindowPortBounds(oni_mac_window, &window_rect))
		{
			Point offset;

			offset.h= window_rect.left;
			offset.v= window_rect.top;
			ShieldCursor(&window_rect, offset);
		}
		else
		{
			HideCursor();
		}

		mac_cursor_visible= false;
	}

	return;
}

void mac_show_cursor(
	void)
{
	if (!mac_cursor_visible)
	{
		if (oni_cursor)
		{
			Cursor unused= {0}, *arrow;

			if ((arrow= GetQDGlobalsArrow(&unused)) != NULL)
			{
				SetCursor(arrow);
				HUnlock(oni_cursor);
				ReleaseResource(oni_cursor);
				oni_cursor= NULL;
			}
			else
			{
				UUrDebuggerMessage("unable to restore the MacOS cursor");
			}
		}

		ShowCursor();
		mac_cursor_visible= true;
	}

	return;
}

static UUtBool input_sprocket_available(
	void)
{
	// project should weak-link with Input Sprocket
	return ((TOOL_VERSION == false) && (ISpGetVersion != 0));
}

// ----------------------------------------------------------------------
static void
LIiPlatform_Mouse_Activate(
	void)
{
	if (using_input_sprocket)
	{
		UUtUns32 mindex;

		// activate the devices
		for (mindex= 0; mindex < LIgMouse_NumMice; mindex++)
		{
			OSStatus status;
			UUtUns32 i;

			if (LIgMouse[mindex].mouse == NULL) continue;

			for (i= 0; i < LIgMouse[mindex].num_buttons; i++)
			{
				if (LIgMouse[mindex].button[i])	status= ISpElement_Flush(LIgMouse[mindex].button[i]);
			}
			if (LIgMouse[mindex].xAxis)		status= ISpElement_Flush(LIgMouse[mindex].xAxis);
			if (LIgMouse[mindex].yAxis)		status= ISpElement_Flush(LIgMouse[mindex].yAxis);
			if (LIgMouse[mindex].zAxis)		status= ISpElement_Flush(LIgMouse[mindex].zAxis);

			status= ISpDevices_Activate(1, &LIgMouse[mindex].mouse);
		}
	}

	return;
}

// ----------------------------------------------------------------------
static void
LIiPlatform_Mouse_Deactivate(
	void)
{
	if (using_input_sprocket)
	{
		UUtUns32 mindex;

		// activate the devices
		for (mindex= 0; mindex < LIgMouse_NumMice; mindex++)
		{
			OSStatus status;
			UUtUns32 i;

			if (LIgMouse[mindex].mouse == NULL) continue;

			for (i= 0; i < LIgMouse[mindex].num_buttons; i++)
			{
				if (LIgMouse[mindex].button[i])	status= ISpElement_Flush(LIgMouse[mindex].button[i]);
			}
			if (LIgMouse[mindex].xAxis)		status= ISpElement_Flush(LIgMouse[mindex].xAxis);
			if (LIgMouse[mindex].yAxis)		status= ISpElement_Flush(LIgMouse[mindex].yAxis);
			if (LIgMouse[mindex].zAxis)		status= ISpElement_Flush(LIgMouse[mindex].zAxis);

			status= ISpDevices_Deactivate(1, &LIgMouse[mindex].mouse);
		}
	}

	return;
}

// ----------------------------------------------------------------------
#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile off
#endif

static void
LIiPlatform_Mouse_GetData(
	LItAction				*outAction)
{
	if (using_input_sprocket)
	{
		UUtUns32 mindex;

		for (mindex= 0; mindex < LIgMouse_NumMice; mindex++)
		{
			UUtUns32 button_data;
			UUtUns32 delta_data;
			LItDeviceInput deviceInput;
			UUtUns32 i;

			if (LIgMouse[mindex].mouse == NULL) continue;

			// get the button data
			for (i= 0; i < LIgMouse[mindex].num_buttons; i++)
			{
				if (LIgMouse[mindex].button[i])
				{
					button_data= 0;
					ISpElement_GetSimpleState(LIgMouse[mindex].button[i], &button_data);
					if (button_data == kISpButtonDown)
					{
						// XXX - if LIcMouseCode_ButtonX ever change, this may break
						deviceInput.input= i + LIcMouseCode_Button1;
						deviceInput.analogValue= 1.0f;

						LIrActionBuffer_Add(outAction, &deviceInput);
					}
				}
			}

			// get the axis data
			if (LIgMouse[mindex].xAxis)
			{
				delta_data= 0;
				ISpElement_GetSimpleState(LIgMouse[mindex].xAxis, &delta_data);

				deviceInput.input= LIcMouseCode_XAxis;
				deviceInput.analogValue= (float)((UUtInt32)delta_data) * LIcFixed_to_Float * 160.0f;

				LIrActionBuffer_Add(outAction, &deviceInput);
			}

			if (LIgMouse[mindex].yAxis)
			{
				delta_data= 0;
				ISpElement_GetSimpleState(LIgMouse[mindex].yAxis, &delta_data);

				deviceInput.input= LIcMouseCode_YAxis;
				if (LIgMouse_Invert)
				{
					deviceInput.analogValue= (float)((UUtInt32)delta_data) * LIcFixed_to_Float * 160.0f;
				}
				else
				{
					deviceInput.analogValue= (float)((UUtInt32)delta_data) * LIcFixed_to_Float * -160.0f;
				}

				LIrActionBuffer_Add(outAction, &deviceInput);
			}

			if (LIgMouse[mindex].zAxis)
			{
				delta_data= 0;
				ISpElement_GetSimpleState(LIgMouse[mindex].zAxis, &delta_data);

				deviceInput.input = LIcMouseCode_ZAxis;
				deviceInput.analogValue = (float)((UUtInt32)delta_data) * LIcFixed_to_Float * 160.0f;

				LIrActionBuffer_Add(outAction, &deviceInput);
			}
		}
	}
	else
	{
		LItDeviceInput device_input;
		Boolean button_down;
		int mouse_dx, mouse_dy;
		GrafPtr old_port;
		Point current_mouse;
		static Point old_mouse= {0,0};

		// set the port
		GetPort(&old_port);
		SetPort(GetWindowPort(LIgWindow));

		GetMouse(&current_mouse);
		mouse_dx= current_mouse.h - old_mouse.h;
		mouse_dy= old_mouse.v - current_mouse.v; // invert Y
		old_mouse= current_mouse;
		button_down= Button();

		SetPort(old_port);

		if (button_down)
		{
			device_input.input= LIcMouseCode_Button1;
			device_input.analogValue= 1.0f;
			LIrActionBuffer_Add(outAction, &device_input);
		}
		if (mouse_dx != 0)
		{
			device_input.input= LIcMouseCode_XAxis;
			device_input.analogValue= (float)mouse_dx;
			LIrActionBuffer_Add(outAction, &device_input);
		}
		if (mouse_dy != 0)
		{
			device_input.input= LIcMouseCode_YAxis;
			device_input.analogValue= (float)mouse_dy;
			LIrActionBuffer_Add(outAction, &device_input);
		}
	}

	return;
}

#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile reset
#endif

// ----------------------------------------------------------------------
static UUtError
LIiPlatform_Mouse_Initialize(
	void)
{
	UUtError error= UUcError_None;

	if (using_input_sprocket)
	{
		OSStatus					status;
		UUtUns32					numDevices;
		ISpDeviceReference			devices[LIcNumDevices];
		UUtUns32					mindex;

		// Initialize
		UUrMemory_Clear(LIgMouse, sizeof(LIgMouse));

		// get a list of the mice attached to the system
		status =
			ISpDevices_ExtractByClass(
				kISpDeviceClass_Mouse,
				LIcNumDevices,
				&numDevices,
				devices);
		if (status != noErr)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to extract devices");
		}
		if (numDevices == 0)	return UUcError_None;

		LIgMouse_NumMice = UUmMin(LIcMouse_MaxMice, numDevices);

		for (mindex = 0; mindex < LIgMouse_NumMice; mindex++)
		{
			ISpElementListReference		elementListRef;
			UUtUns32					numElements;
			ISpElementReference			elements[LIcNumElements];
			UUtUns32					i;

			// only use the first device
			LIgMouse[mindex].mouse = devices[mindex];

			// get a list of the elements in the first device
			status =
				ISpDevice_GetElementList(
					LIgMouse[mindex].mouse,
					&elementListRef);
			if (status != noErr)
			{
				LIgMouse[mindex].mouse = NULL;
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get the element list");
			}
			if (elements == NULL)
			{
				LIgMouse[mindex].mouse = NULL;
				return UUcError_None;
			}

			// extract the elements of the device
			status =
				ISpElementList_Extract(
					elementListRef,
					LIcNumElements,
					&numElements,
					elements);
			if (status != noErr)
			{
				LIgMouse[mindex].mouse = NULL;
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to extract the elements");
			}
			if (numElements == 0)
			{
				LIgMouse[mindex].mouse = NULL;
				return UUcError_None;
			}

			// go through the elements and find uses for them
			for (i = 0; i < numElements; i++)
			{
				ISpElementInfo		elementInfo;

				// get info about the element
				status =
					ISpElement_GetInfo(
						elements[i],
						&elementInfo);
				if (status != noErr)
				{
					UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to extract devices");
				}

				// gather the info
				switch (elementInfo.theKind)
				{
					case kISpElementKind_Button:
						if (LIgMouse[mindex].num_buttons < LIcMouse_MaxButtons)
						{
							LIgMouse[mindex].button[LIgMouse[mindex].num_buttons] = elements[i];
							LIgMouse[mindex].num_buttons++;
						}
					break;

					case kISpElementKind_Delta:
						switch (elementInfo.theLabel)
						{
							case kISpElementLabel_Delta_X:
								LIgMouse[mindex].xAxis = elements[i];
							break;

							case kISpElementLabel_Delta_Y:
								LIgMouse[mindex].yAxis = elements[i];
							break;

							case kISpElementLabel_Delta_Z:
								LIgMouse[mindex].zAxis = elements[i];
							break;
						}
					break;
				}
			}
		}
	}

	return error;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
LIiPlatform_Keyboard_Activate(
	void)
{
	return;
}

// ----------------------------------------------------------------------
static void
LIiPlatform_Keyboard_Deactivate(
	void)
{
	return;
}

// ----------------------------------------------------------------------
#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile off
#endif

static void LIiPlatform_Keyboard_GetData(
	LItAction *action)
{
	unsigned char keymap[16];
	int i;

	GetKeys((unsigned long *)keymap);
	for (i= 0; i<num_mac_keys; i++)
	{
		if ((keymap[mac_keyboard_translation_table[i].mac_key>>3] >> (mac_keyboard_translation_table[i].mac_key & 7) ) & 1)
		{
			LItDeviceInput device_input;

			device_input.input= mac_keyboard_translation_table[i].oni_key;
			device_input.analogValue= 1.0f;
			LIrActionBuffer_Add(action, &device_input);
		}
	}

	return;
}

#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile reset
#endif

// ----------------------------------------------------------------------
static UUtError
LIiPlatform_Keyboard_Initialize(
	void)
{
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
LIiPlatform_Devices_Activate(
	void)
{
	// flush the events in the MacOS event queue
	FlushEvents(everyEvent, 0);

	// activate the devices
	LIiPlatform_Mouse_Activate();
	LIiPlatform_Keyboard_Activate();

	return;
}

// ----------------------------------------------------------------------
static void
LIiPlatform_Devices_Deactivate(
	void)
{
	// deactivate the devices
	LIiPlatform_Mouse_Deactivate();
	LIiPlatform_Keyboard_Deactivate();

	// flush the events in the MacOS event queue
	FlushEvents(everyEvent, 0);

	return;
}

// ----------------------------------------------------------------------
static UUtError
LIiPlatform_Devices_Initialize(
	void)
{
	UUtError error;

	// initialize the mouse
	error= LIiPlatform_Mouse_Initialize();
	UUmError_ReturnOnError(error);

	// initialize the keyboard
	error= LIiPlatform_Keyboard_Initialize();
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile off
#endif

static void
LIiPlatform_Devices_GetData(
	LItAction				*outAction)
{
	// get mouse data
	LIiPlatform_Mouse_GetData(outAction);

	// get the keyboard data
	LIiPlatform_Keyboard_GetData(outAction);

	return;
}

#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile reset
#endif

// ----------------------------------------------------------------------
static void
LIiPlatform_Devices_Terminate(
	void)
{
	return;
}

#ifdef USE_MAC_MENU

enum {
	_mac_shortcuts_menu= 129,
	_mac_quit= 1
};

static void mac_handle_menu_choice(
	long menu_result)
{
	short menu, menu_item;

	menu= HiWord(menu_result);
	menu_item= LoWord(menu_result);

	switch (menu)
	{
		case _mac_shortcuts_menu:
			switch (menu_item)
			{
				case _mac_quit:
					// this doesn't really work... I may or may not fix it
					WMrMessage_Post(NULL, WMcMessage_Quit, 0, 0);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	HiliteMenu(0);

	return;
}

#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
LIiPlatform_InputEvent_GetEvent(
	LItMode inMode)
{
	UUtBool event_avail, was_event;
	EventRecord event= {0};
	GrafPtr old_port;
	LItInputEvent inputEvent= {0};

	// don't call WaitNextEvent() unless the mode is LIcMode_Normal
	if (inMode == LIcMode_Game) return UUcFalse;

	// set the port
	GetPort(&old_port);
	SetPort(GetWindowPort(LIgWindow));

	// get keyboard events and mouse events
	was_event= WaitNextEvent(everyEvent, &event, 0, NULL);
	if (was_event)
	{
		switch (event.what)
		{
			case mouseDown:
			case mouseUp:
				{
					UUtInt16 part;
					WindowPtr window;

					part= FindWindow(event.where, &window);
					switch (part)
					{
					#ifdef USE_MAC_MENU
						case inMenuBar:
							mac_handle_menu_choice(MenuSelect(event.where));
							break;
					#endif

						case inSysWindow:
							/* Carbon -S.S.
							SystemClick(&event, window);
							*/
							was_event= UUcFalse;
							break;

						case inContent:
							// put the event in local coordinates
							GlobalToLocal(&event.where);

							// set the where
							inputEvent.where.x= event.where.h;
							inputEvent.where.y= event.where.v;
							inputEvent.modifiers= event.modifiers;

							// set the type
							if (inputEvent.modifiers & LIcMacOS_RightMouseButtonKey)
							{
								if (event.what == mouseDown)
								{
									inputEvent.type= LIcInputEvent_RMouseDown;
								}
								else
								{
									inputEvent.type= LIcInputEvent_RMouseUp;
								}
							}
							else
							{
								if (event.what == mouseDown)
								{
									inputEvent.type= LIcInputEvent_LMouseDown;
								}
								else
								{
									inputEvent.type= LIcInputEvent_LMouseUp;
								}
							}
							break;
					}
				}
				break;

			case keyDown:
				inputEvent.type= LIcInputEvent_KeyDown;
				inputEvent.key= LIgAsciiToKeyCode[(event.message & charCodeMask)];
				inputEvent.modifiers= event.modifiers;

				if (inputEvent.key == LIcKeyCode_F1)
				{
					UUtUns32 index= ((event.message & keyCodeMask) - LIcFKeyStart) >> 8;
					inputEvent.key= LIgFunctionKey[index];
				}
			#ifdef USE_MAC_MENU
				if (event.modifiers & cmdKey)
				{
					mac_handle_menu_choice(MenuKey(inputEvent.key));
				}
			#endif
				break;

			case keyUp:
				inputEvent.type= LIcInputEvent_KeyUp;
				inputEvent.key= LIgAsciiToKeyCode[(event.message & charCodeMask)];
				inputEvent.modifiers= event.modifiers;

				if (inputEvent.key == LIcKeyCode_F1)
				{
					UUtUns32 index= ((event.message & keyCodeMask) - LIcFKeyStart) >> 8;
					inputEvent.key= LIgFunctionKey[index];
				}
				break;

			case autoKey:
				inputEvent.type= LIcInputEvent_KeyRepeat;
				inputEvent.key= LIgAsciiToKeyCode[(event.message & charCodeMask)];
				inputEvent.modifiers= event.modifiers;

				if (inputEvent.key == LIcKeyCode_F1)
				{
					UUtUns32 index= ((event.message & keyCodeMask) - LIcFKeyStart) >> 8;
					inputEvent.key= LIgFunctionKey[index];
				}
			#ifdef USE_MAC_MENU
				if (event.modifiers & cmdKey)
				{
					mac_handle_menu_choice(MenuKey(inputEvent.key));
				}
			#endif
				break;

			case updateEvt:
				BeginUpdate((WindowPtr)event.message);
				mac_hide_cursor();
				EndUpdate((WindowPtr)event.message);
				was_event= UUcFalse;
				break;

			case osEvt:
				{
					UUtUns8 osEvtType;

					osEvtType= (event.message & osEvtMessageMask) >> 24;

					if (osEvtType & suspendResumeMessage)
					{
						if (event.message & resumeFlag)
						{
							// resume
						}
						else
						{
							// suspend
						}
					}

					if (osEvtType & mouseMovedMessage)
					{
						// if the mouse changed position, generate a mouse moved event
						if ((event.where.h != LIgMouseLocation.x) ||
							(event.where.v != LIgMouseLocation.y))
						{
							inputEvent.type= LIcInputEvent_MouseMove;
							inputEvent.where.x= event.where.h;
							inputEvent.where.y= event.where.v;
							inputEvent.modifiers= event.modifiers;

						// set the previous mouse location
							LIgMouseLocation= inputEvent.where;
						}
						else
						{
							was_event= UUcFalse;
						}
					}
				}
				break;

			default:
				was_event= UUcFalse;
				break;
		}

		// set the previous mouse location
		LIgMouseLocation= inputEvent.where;
	}
	else
	{
		Point current_mouse_location;

		// get the current mouse position
		GetMouse(&current_mouse_location);

		// if the mouse changed position, generate a mouse moved event
		if ((current_mouse_location.h != LIgMouseLocation.x) ||
			(current_mouse_location.v != LIgMouseLocation.y))
		{
			EventRecord fakeEvent= {0};

			// get a fake os event
			/* Carbon
			GetOSEvent(0, &fakeEvent);*/
			//GetNextEvent(0, &fakeEvent); no need for this

			// set the type and location
			inputEvent.type= LIcInputEvent_MouseMove;
			inputEvent.where.x= current_mouse_location.h;
			inputEvent.where.y= current_mouse_location.v;
			//inputEvent.modifiers= fakeEvent.modifiers;

			// an event was generated
			was_event= UUcTrue;

			// set the previous mouse location
			LIgMouseLocation= inputEvent.where;
		}
	}
	event_avail= was_event;

	// add the event to the event queue
	if (event_avail)
	{
		LIrInputEvent_Add(
			inputEvent.type,
			&inputEvent.where,
			inputEvent.key,
			inputEvent.modifiers);
	}

	// restore the port
	SetPort(old_port);

	return event_avail;
}

// ----------------------------------------------------------------------
void
LIrPlatform_InputEvent_GetMouse(
	LItMode inMode,
	LItInputEvent *outInputEvent)
{
	// set up the outInputEvent
	outInputEvent->type= LIcInputEvent_None;
	outInputEvent->where.x= 0;
	outInputEvent->where.y= 0;
	outInputEvent->key= 0;
	outInputEvent->modifiers= 0;

	if (inMode == LIcMode_Normal)
	{
		GrafPtr old_port;
		Point mouse_position;
		EventRecord fakeEvent;

		// set the port
		GetPort(&old_port);
		SetPort(GetWindowPort(LIgWindow));

		// get the mouse position
		GetMouse(&mouse_position);

		// set the input event
		outInputEvent->where.x= mouse_position.h;
		outInputEvent->where.y= mouse_position.v;

		// get a fake os event
		/* Carbon
		GetOSEvent(0, &fakeEvent);*/
		GetNextEvent(0, &fakeEvent);

		// check the shift key
		if (fakeEvent.modifiers & LIcMacOS_ShiftKey)
			outInputEvent->modifiers|= LIcMouseState_LShiftDown;

		// check the button state
		if (!(fakeEvent.modifiers & btnState))
		{
			if (fakeEvent.modifiers & LIcMacOS_RightMouseButtonKey)
				outInputEvent->modifiers|= LIcMouseState_RButtonDown;
			else
				outInputEvent->modifiers|= LIcMouseState_LButtonDown;
		}

		// restore the port
		SetPort(old_port);
	}

	return;
}

// ----------------------------------------------------------------------
UUtUns32
LIrPlatform_InputEvent_InterpretModifiers(
	LItInputEventType inEventType,
	UUtUns32 inModifiers)
{
	UUtUns32 modifiers;

	// init
	modifiers= 0;

	switch (inEventType)
	{
		case LIcInputEvent_MouseMove:
		case LIcInputEvent_LMouseDown:
		case LIcInputEvent_LMouseUp:
		case LIcInputEvent_MMouseDown:
		case LIcInputEvent_MMouseUp:
		case LIcInputEvent_RMouseDown:
		case LIcInputEvent_RMouseUp:
			// set the modifiers
			if (inModifiers & LIcMacOS_LShiftKey)
				modifiers|= LIcMouseState_LShiftDown;

			if (inModifiers & LIcMacOS_RShiftKey)
				modifiers|= LIcMouseState_RShiftDown;

			if (inModifiers & LIcMacOS_LOptionKey)
				modifiers|= LIcMouseState_LControlDown;

			if (inModifiers & LIcMacOS_ROptionKey)
				modifiers|= LIcMouseState_RControlDown;

			if (!(inModifiers & btnState))
			{
				if (inModifiers & LIcMacOS_RightMouseButtonKey)
					modifiers|= LIcMouseState_RButtonDown;
				else
					modifiers|= LIcMouseState_LButtonDown;
			}
			break;

		case LIcInputEvent_KeyUp:
		case LIcInputEvent_KeyDown:
		case LIcInputEvent_KeyRepeat:
			// set the modifiers
			if (inModifiers & LIcMacOS_ShiftKey)
			{
				modifiers|= LIcKeyState_ShiftDown;
			}

			if (inModifiers & LIcMacOS_CommandKey)
			{
				modifiers|= LIcKeyState_CommandDown;
			}

			if (inModifiers & LIcMacOS_OptionKey)
			{
				modifiers|= LIcKeyState_AltDown;
			}
			break;
	}

	return modifiers;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
LIrPlatform_Mode_Set(
	LItMode inMode)
{
	if (inMode == LIcMode_Game)
	{
		LIiPlatform_Devices_Activate();
	}
	else
	{
		LIiPlatform_Devices_Deactivate();
	}

	return;
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
	UUtError error;
	NumVersion ISpVersion= {0};
	UUtUns16 i;
	LItKeyCodeHelper *help;

	// ------------------------------
	// initialize the globals
	// ------------------------------
	LIgWindow= inWindow;
	LIgMouseLocation.x= 0;
	LIgMouseLocation.y= 0;

	// initialize LIgAsciiToKeyCode
	for (i = 0; i < 256; i++)
	{
		LIgAsciiToKeyCode[i] = i;
	}

	for (help= LIgKeyCodeHelper; help->ascii != 0x0000; help++)
	{
		LIgAsciiToKeyCode[help->ascii] = help->keyCode;
	}

	// ------------------------------
	// initialize InputSprocket
	// ------------------------------
	using_input_sprocket= input_sprocket_available();
	if (!using_input_sprocket)
	{
		UUrStartupMessage("InputSprocket is not being used");
	}
	else
	{
		// check the ISp version number (make sure it is 1.1 or greater)
		ISpVersion= ISpGetVersion();
		if ((ISpVersion.minorAndBugRev < 0x01) && (ISpVersion.majorRev < 0x10))
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "This version of InputSprocket is not supported.");
		}

		// startup ISp
		if (ISpStartup() != noErr)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to startup InputSprocket.");
		}
	}

	// initialize the devices
	error= LIiPlatform_Devices_Initialize();
	UUmError_ReturnOnErrorMsg(error, "Unable to initialize the devices.");

	return UUcError_None;
}

// ----------------------------------------------------------------------
#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile off
#endif

void
LIrPlatform_PollInputForAction(
	LItAction				*outAction)
{
	UUtUns16				i;

	// clear the action
	outAction->buttonBits = 0;
	for (i= 0; i<LIcNumAnalogValues; i++)
	{
		outAction->analogValues[i] = 0.0f;
	}

	// get the action data
	LIiPlatform_Devices_GetData(outAction);

	return;
}

#if UUmCompiler == UUmCompiler_MWerks
//#pragma profile reset
#endif

// ----------------------------------------------------------------------
void
LIrPlatform_Terminate(
	void)
{
	if (using_input_sprocket)
	{
		// shutdown ISp
		if (ISpShutdown() != noErr)
		{
			UUrError_Report(UUcError_Generic, "Unable to shutdown InputSprocket.");
		}
	}

	return;
}

// ----------------------------------------------------------------------
UUtBool
LIrPlatform_Update(
	LItMode inMode)
{
	return LIiPlatform_InputEvent_GetEvent(inMode);
}


UUtBool LIrPlatform_TestKey(
	LItKeyCode keycode,
	LItMode unsued_mode)
{
	unsigned char keymap[16];
	UUtBool down= UUcFalse;
	int i;

	GetKeys((unsigned long *)keymap);
	for (i= 0; i<num_mac_keys; i++)
	{
		if (mac_keyboard_translation_table[i].oni_key == keycode)
		{
			down= ((keymap[mac_keyboard_translation_table[i].mac_key>>3] >> (mac_keyboard_translation_table[i].mac_key & 7) ) & 1) ? UUcTrue : UUcFalse;
			break;
		}
	}

	return down;
}

