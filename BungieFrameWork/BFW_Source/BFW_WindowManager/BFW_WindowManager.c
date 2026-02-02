// ======================================================================
// BFW_WindowManager.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_WindowManager_Private.h"
#include "BFW_LocalInput.h"
#include "BFW_TextSystem.h"
#include "BFW_Timer.h"
#include "BFW_SoundSystem2.h"

#include "WM_PartSpecification.h"
#include "WM_Cursor.h"

#include "WM_Box.h"
#include "WM_Button.h"
#include "WM_CheckBox.h"
#include "WM_Dialog.h"
#include "WM_EditField.h"
#include "WM_ListBox.h"
#include "WM_Menu.h"
#include "WM_MenuBar.h"
#include "WM_Picture.h"
#include "WM_PopupMenu.h"
#include "WM_ProgressBar.h"
#include "WM_RadioButton.h"
#include "WM_Scrollbar.h"
#include "WM_Slider.h"
#include "WM_Text.h"
#include "WM_Window.h"

#include <ctype.h>

// ======================================================================
// defines
// ======================================================================
#define WMcMaxEventsPerUpdate		10

#define WMcDoubleClickDelta			30		// ticks
#define WMcMaxDoubleClickDistance	3.0f

#define WMcMaxWindowClasses			64

#define WMcMaxWindows				255
#define WMcStack_MaxWindows			255
#define WMcMaxNumEvents				32

#define WMcWA_Error					0xFFFFFFFF

#define WMcCaret_BlinkOffTime		25
#define WMcCaret_BlinkOnTime		50

#define WMcTimer_InitialNumber		16

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcWindowArrayFlag_None			= 0x0000,
	WMcWindowArrayFlag_InUse		= 0x0001
};

enum
{
	WMcKeyCategory_None				= 0x0000,
	WMcKeyCategory_Alpha			= 0x0001,
	WMcKeyCategory_Digit			= 0x0002,
	WMcKeyCategory_Punctuation		= 0x0004,
	WMcKeyCategory_Control			= 0x0008,
	WMcKeyCategory_Arrow			= 0x0010,
	WMcKeyCategory_Navigation		= 0x0020,
	WMcKeyCategory_Delete			= 0x0040
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct WMtMouseEventMap
{
	LItInputEventType			event_type;
	WMtMessage					window_message;

} WMtMouseEventMap;

typedef struct WMtEventData
{
	WMtWindow					*prev_mouse_over_window;

	// for tracking double clicks
	WMtWindow					*prev_mouse_click_window;
	IMtPoint2D					prev_mouse_click_location;
	WMtMessage					prev_mouse_click_message;
	UUtUns32					prev_mouse_click_time;

} WMtEventData;

// ----------------------------------------------------------------------
typedef struct WMtTimer
{
	WMtWindow				*window;
	UUtUns32				timer_id;
	UUtUns32				timer_frequency;
	UUtUns32				timer_next_firing;

} WMtTimer;

// ----------------------------------------------------------------------
typedef struct WMtWindowArray
{
	UUtUns32				flags;
	WMtWindow				*window;

} WMtWindowArray;

// ======================================================================
// globals
// ======================================================================
static UUtBool				WMgInitialized;

static UUtUns32				WMgNumWindowClasses;
static WMtWindowClass		WMgWindowClassList[WMcMaxWindowClasses];

static WMtEvent				WMgEvents[WMcMaxNumEvents];
static UUtUns32				WMgEventHead;
static UUtUns32				WMgEventTail;

static WMtWindow			*WMgDesktop;

static WMtCursor			*WMgCursor;
static IMtPoint2D			WMgCursorPosition;

static WMtWindow			*WMgActiveWindow;
static WMtWindow			*WMgMouseFocus;
static WMtWindow			*WMgKeyboardFocus;

static UUtBool				WMgActive;

static UUtMemory_Array		*WMgTimers;

static WMtWindowArray		WMgWindowArray[WMcMaxWindows];
static UUtUns32				WMgWAIndex;

static WMtCaret				WMgCaret;

// ----------------------------------------------------------------------
static WMtMouseEventMap 	WMgMouseEventMap[] =
{
	{ LIcInputEvent_MouseMove,	WMcMessage_MouseMove },
	{ LIcInputEvent_LMouseDown,	WMcMessage_LMouseDown },
	{ LIcInputEvent_LMouseUp,	WMcMessage_LMouseUp },
	{ LIcInputEvent_MMouseDown,	WMcMessage_MMouseDown },
	{ LIcInputEvent_MMouseUp,	WMcMessage_MMouseUp },
	{ LIcInputEvent_RMouseDown,	WMcMessage_RMouseDown },
	{ LIcInputEvent_RMouseUp,	WMcMessage_RMouseUp },
	{ LIcInputEvent_None,		WMcMessage_None }
};

static WMtMouseEventMap 	WMgMouseNCEventMap[] =
{
	{ LIcInputEvent_MouseMove,	WMcMessage_NC_MouseMove },
	{ LIcInputEvent_LMouseDown,	WMcMessage_NC_LMouseDown },
	{ LIcInputEvent_LMouseUp,	WMcMessage_NC_LMouseUp },
	{ LIcInputEvent_MMouseDown,	WMcMessage_NC_MMouseDown },
	{ LIcInputEvent_MMouseUp,	WMcMessage_NC_MMouseUp },
	{ LIcInputEvent_RMouseDown,	WMcMessage_NC_RMouseDown },
	{ LIcInputEvent_RMouseUp,	WMcMessage_NC_RMouseUp },
	{ LIcInputEvent_None,		WMcMessage_None }
};

static UUtBool				WMgEventOverride;

static WMtEventData			WMgEventData;

static FILE					*WMgMessagesFile;

// ======================================================================
// prototypes
// ======================================================================
static void
WMiCaret_Draw(
	WMtWindow				*inWindow);

static void
WMiTimer_WindowDestroyed(
	WMtWindow				*inWindow);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiWA_Add(
	UUtUns32				inIndex,
	WMtWindow				*inWindow)
{
	UUmAssert(inIndex != WMcWA_Error);
	if (inWindow == NULL) { return; }

	WMgWindowArray[inIndex].flags |= WMcWindowArrayFlag_InUse;
	WMgWindowArray[inIndex].window = inWindow;
	inWindow->index = inIndex;
}

// ----------------------------------------------------------------------
static void
WMiWA_Free(
	WMtWindow				*inWindow)
{
	if (inWindow == NULL) { return; }

	WMgWindowArray[inWindow->index].flags &= ~WMcWindowArrayFlag_InUse;
}

// ----------------------------------------------------------------------
static void
WMiWA_FreeUnused(
	void)
{
	UUtUns32				i;

	for (i = 0; i < WMcMaxWindows; i++)
	{
		// don't dispose of windows that are in use
		if (WMgWindowArray[i].flags & WMcWindowArrayFlag_InUse) { continue; }

		// free the unused memory
		if (WMgWindowArray[i].window)
		{
			UUrMemory_Block_Delete(WMgWindowArray[i].window);
			WMgWindowArray[i].window = NULL;
		}
	}
}

// ----------------------------------------------------------------------
static UUtError
WMiWA_Initialize(
	void)
{
	UUtUns32				i;

	WMgWAIndex = 0;

	for (i = 0; i < WMcMaxWindows; i++)
	{
		WMgWindowArray[i].flags = WMcWindowArrayFlag_None;
		WMgWindowArray[i].window = NULL;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiWA_New(
	void)
{
	UUtUns32				i;

	// search through the array from WMgWAIndex until the end
	for (i = (WMgWAIndex + 1); i < WMcMaxWindows; i++)
	{
		if ((WMgWindowArray[i].flags & WMcWindowArrayFlag_InUse) == 0)
		{
			break;
		}
	}

	// if no available index was found, search from the beginning until WMgWAIndex
	if (i == WMcMaxWindows)
	{
		for (i = 0; i < WMgWAIndex; i++)
		{
			if ((WMgWindowArray[i].flags & WMcWindowArrayFlag_InUse) == 0)
			{
				break;
			}
		}
	}

	// if no available index was found, return an error
	if (i == WMgWAIndex) { return WMcWA_Error; }

	WMgWAIndex = i;

	// delete old window memory
	if (WMgWindowArray[i].window)
	{
		UUrMemory_Block_Delete(WMgWindowArray[i].window);
	}

	// return the available index
	return i;
}

// ----------------------------------------------------------------------
static void
WMiWA_Terminate(
	void)
{
	UUtUns32				i;

	for (i = 0; i < WMcMaxWindows; i++)
	{
		if (WMgWindowArray[i].window)
		{
			UUrMemory_Block_Delete(WMgWindowArray[i].window);
		}

		WMgWindowArray[i].flags = WMcWindowArrayFlag_None;
		WMgWindowArray[i].window = NULL;
	}
}

// ----------------------------------------------------------------------
static UUcInline UUtBool
WMiWA_Valid(
	WMtWindow				*inWindow)
{
	if (inWindow == NULL) {	return UUcFalse; }
	if (inWindow->index < 0) { return UUcFalse; }
	if (inWindow->index >= WMcMaxWindows) { return UUcFalse; }

	return ((WMgWindowArray[inWindow->index].flags & WMcWindowArrayFlag_InUse) != 0);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
WMiWindow_AddToList(
	WMtWindow				*inWindow)
{
	WMtWindow				*list_owner;

	UUmAssert(inWindow);

	// check the window's type
	if (inWindow->window_class->type == WMcWindowType_Desktop)
	{
		return UUcError_None;
	}

	// get the window which owns the list the window will be placed into
	if (inWindow->flags & WMcWindowFlag_Child)
	{
		list_owner = inWindow->parent;
	}
	else
	{
		list_owner = WMgDesktop;
	}

	if (list_owner == NULL) { return UUcError_Generic; }

	// add inWindow as the last child
	if (list_owner->child)
	{
		WMtWindow			*child;

		// find the last child in the list
		child = list_owner->child;
		while (child->next)
		{
			child = child->next;
		}

		// put in list after child
		child->next = inWindow;
		inWindow->prev = child;
		inWindow->next = NULL;
	}
	else
	{
		// no children exist, put inWindow at the beginning of the list
		list_owner->child = inWindow;
		inWindow->prev = NULL;
		inWindow->next = NULL;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
WMiWindow_RemoveFromList(
	WMtWindow				*inWindow)
{
	WMtWindow				*list_owner;

	UUmAssert(inWindow);

	// check the window's type
	if (inWindow->window_class->type == WMcWindowType_Desktop)
	{
		return;
	}

	// get the window which owns the list the window will be placed into
	if (inWindow->flags & WMcWindowFlag_Child)
	{
		list_owner = inWindow->parent;
	}
	else
	{
		list_owner = WMgDesktop;
	}

	if (list_owner == NULL) { return; }

	// remove inWindow from the parent if it is the first item in the list
	if (list_owner->child == inWindow)
	{
		list_owner->child = inWindow->next;
	}

	// make the previous window in the list point to the next window
	if (inWindow->prev)
	{
		inWindow->prev->next = inWindow->next;
	}

	// make the next window in the list point to the prev window
	if (inWindow->next)
	{
		inWindow->next->prev = inWindow->prev;
	}

	// the window is alone in the world
	inWindow->prev = NULL;
	inWindow->next = NULL;
}

// ----------------------------------------------------------------------
static void
WMiWindow_InsertAfterChild(
	WMtWindow				*inWindow,
	WMtWindow				*inChild)
{
	WMtWindow				*list_owner;

	UUmAssert(inWindow);

	// check the window's type
	if (inWindow->window_class->type == WMcWindowType_Desktop)
	{
		return;
	}

	// get the window which owns the list the window will be placed into
	if (inWindow->flags & WMcWindowFlag_Child)
	{
		list_owner = inWindow->parent;
	}
	else
	{
		list_owner = WMgDesktop;
	}

	if (list_owner == NULL) { return; }

	// remove inWindow from the list it is in
	WMiWindow_RemoveFromList(inWindow);

	if (inChild)
	{
		// put in list after child
		inWindow->prev = inChild;
		inWindow->next = inChild->next;

		if (inChild->next)
		{
			inChild->next->prev = inWindow;
		}

		inChild->next = inWindow;
	}
	else
	{
		if (list_owner->child)
		{
			// insert at the front of the list
			inWindow->prev = NULL;
			inWindow->next = list_owner->child;
			list_owner->child->prev = inWindow;
			list_owner->child = inWindow;
		}
		else
		{
			// no children exist, put inWindow at the beginning of the list
			list_owner->child = inWindow;
			inWindow->prev = NULL;
			inWindow->next = NULL;
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
WMiWindow_DeleteOwned(
	WMtWindow				*inWindow,
	UUtUns32				inUserData)
{
	WMtWindow				*owner;

	// delete the window if inWindow->owner is equal to owner
	owner = (WMtWindow*)inUserData;
	if (inWindow->owner == owner)
	{
		WMrWindow_Delete(inWindow);
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
WMiWindow_Draw(
	WMtWindow				*inWindow)
{
	WMtWindow				*child;

	// don't draw the window if it isn't visible
	if ((inWindow->flags & WMcWindowFlag_Visible) == 0)
	{
		return;
	}

	// send the draw message to the window
	WMrMessage_Send(
		inWindow,
		WMcMessage_NC_Paint,
		0,
		0);

	// send the draw message to the window
	WMrMessage_Send(
		inWindow,
		WMcMessage_Paint,
		0,
		0);

	// draw the caret
	if (WMgCaret.owner == inWindow)
	{
		WMiCaret_Draw(inWindow);
	}

	// draw the child windows
	if (inWindow->child)
	{
		// first go to the last child in the list
		child = inWindow->child;
		while (child->next)
		{
			child = child->next;
		}

		// draw the child windows back to front
		while (child)
		{
			WMiWindow_Draw(child);
			child = child->prev;
		}
	}
}

// ----------------------------------------------------------------------
static WMtWindow*
WMiWindow_GetWindowUnderPoint(
	WMtWindow				*inWindow,
	IMtPoint2D				*inPoint,
	UUtUns32				inFlags)
{
	WMtWindow				*window_under_point;
	UUtRect					window_rect;
	UUtRect					client_rect;

	// check the window flags
	if ((inWindow->flags & inFlags) != inFlags)
	{
		return NULL;
	}

	// see if the point is even within this window
	WMrWindow_GetRect(inWindow, &window_rect);
	if (IMrRect_PointIn(&window_rect, inPoint) == UUcFalse) { return NULL; }

	// inWindow is under the point
	window_under_point = inWindow;

	// check to see if the point is within the client rect of the window
	client_rect = inWindow->client_rect;
	IMrRect_Offset(&client_rect, window_rect.left, window_rect.top);
	if (IMrRect_PointIn(&client_rect, inPoint))
	{
		WMtWindow			*child;

		// check the child windows to see if any of them are under the point
		child = inWindow->child;
		while (child)
		{
			WMtWindow			*child_under_point;

			child_under_point = WMiWindow_GetWindowUnderPoint(child, inPoint, inFlags);
			if (child_under_point)
			{
				window_under_point = child_under_point;
				break;
			}
			child = child->next;
		}
	}

	// check the undesirable flags
	if (window_under_point == inWindow)
	{
		UUtUns32			undesirable_flags;

		undesirable_flags = WMcWindowFlag_Disabled | WMcWindowFlag_MouseTransparent;
		if ((inWindow->flags & undesirable_flags) != 0)
		{
			window_under_point = NULL;
		}
	}

	return window_under_point;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
WMtWindow*
WMrWindow_Activate(
	WMtWindow				*inWindow)
{
	WMtWindow				*old_active;

	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return NULL; }

	// if this is a child window don't do anything
	if (inWindow->flags & WMcWindowFlag_Child)
	{
		return WMrWindow_Activate(inWindow->parent);
	}

	// if there is no inWindow or if inWindow is already active,
	// then leave the currently active window active
	if ((inWindow == NULL) || (inWindow == WMgActiveWindow))
	{
		return WMgActiveWindow;
	}

	// save the currently active window
	old_active = WMgActiveWindow;

	// deactivate the currently active window
	if (WMgActiveWindow)
	{
		WMrMessage_Send(
			WMgActiveWindow,
			WMcMessage_Activate,
			(UUtUns32)UUcFalse,
			0);

		WMrMessage_Send(
			WMgActiveWindow,
			WMcMessage_NC_Activate,
			(UUtUns32)UUcFalse,
			0);
	}

	// whichever window has the mouse capture, loses it
	WMrWindow_CaptureMouse(NULL);

	WMgActiveWindow = inWindow;

	// bring the window to the front
	WMrWindow_BringToFront(inWindow);

	// tell the window it was activated
	WMrMessage_Send(
		inWindow,
		WMcMessage_NC_Activate,
		(UUtUns32)UUcTrue,
		0);

	WMrMessage_Send(
		inWindow,
		WMcMessage_Activate,
		(UUtUns32)UUcTrue,
		0);

	return old_active;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_BringToFront(
	WMtWindow				*inWindow)
{
	WMtWindow				*list_owner;
	WMtWindow				*insert_after;

	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	if (inWindow->window_class->type == WMcWindowType_Desktop) { return UUcTrue; }

	// bring the parent forward
	if (inWindow->parent)
	{
		WMrWindow_BringToFront(inWindow->parent);
	}

	// get the window which owns the list the window will be placed into
	if (inWindow->flags & WMcWindowFlag_Child)
	{
		list_owner = inWindow->parent;
	}
	else
	{
		list_owner = WMgDesktop;
	}

	// find the window to place inWindow after
	if (inWindow->flags & WMcWindowFlag_PopUp)
	{
		insert_after = NULL;
	}
	else if (inWindow->flags & WMcWindowFlag_TopMost)
	{
		insert_after = list_owner->child;
		while ((insert_after) && (insert_after->next))
		{
			if ((insert_after->flags & WMcWindowFlag_PopUp) == 0)
			{
				insert_after = insert_after->prev;
				break;
			}
			insert_after = insert_after->next;
		}
	}
	else
	{
		insert_after = list_owner->child;
		while ((insert_after) && (insert_after->next))
		{
			if ((insert_after->flags & (WMcWindowFlag_PopUp | WMcWindowFlag_TopMost)) == 0)
			{
				insert_after = insert_after->prev;
				break;
			}
			insert_after = insert_after->next;
		}
	}

	// check to see if the window is already sorted
	if (insert_after == inWindow)
	{
		return UUcTrue;
	}

	// set the position of the window
	WMrWindow_SetPosition(
		inWindow,
		insert_after,
		inWindow->location.x,
		inWindow->location.y,
		inWindow->width,
		inWindow->height,
		WMcPosChangeFlag_NoMove | WMcPosChangeFlag_NoSize);

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_CaptureMouse(
	WMtWindow				*inWindow)
{
	WMtWindow				*old_focus;

	if (WMgMouseFocus == inWindow) { return UUcTrue; }

	old_focus = WMgMouseFocus;
	WMgMouseFocus = inWindow;

	if (old_focus)
	{
		WMrMessage_Send(
			old_focus,
			WMcMessage_CaptureChanged,
			(UUtUns32)inWindow,
			0);
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_Delete(
	WMtWindow				*inWindow)
{
	WMtWindow				*child;

	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	// deactivate the window
	if (WMgActiveWindow == inWindow)
	{
		if (inWindow->window_class->type != WMcWindowType_Desktop)
		{
			// active the window below this one if one exists
			// if the next one doesn't exist, activate the one above it
			// if the prev one doesn't exist, activate the the owner
			// if the owner doesn't exist, activate the desktop
			if (inWindow->next)
			{
				WMrWindow_Activate(inWindow->next);
			}
			else if (inWindow->prev)
			{
				WMrWindow_Activate(inWindow->prev);
			}
			else if (inWindow->owner)
			{
				WMrWindow_Activate(inWindow->owner);
			}
			else
			{
				WMrWindow_Activate(WMgDesktop);
			}
		}
	}

	// turn off the mouse focus
	if (WMgMouseFocus == inWindow)
	{
		WMrWindow_CaptureMouse(NULL);
	}

	// turn off the keyboard focus
	if (WMgKeyboardFocus == inWindow)
	{
		WMrWindow_SetFocus(NULL);
	}

	// notify the parent
	if (inWindow->parent)
	{
		WMtWindow			*parent;

		parent = inWindow->parent;

		WMrMessage_Send(
			parent,
			WMcMessage_ParentNotify,
			WMcMessage_Destroy,
			(UUtUns32)inWindow);
	}

	// remove the window from the list
	WMiWindow_RemoveFromList(inWindow);

	// stop any timers this window may have started
	WMiTimer_WindowDestroyed(inWindow);

	// delete the window
	WMrMessage_Send(inWindow, WMcMessage_Destroy, 0, 0);

	// destroy the child windows
	child = inWindow->child;
	while (child)
	{
		WMtWindow			*delete_me;

		delete_me = child;
		child = child->next;

		WMrWindow_Delete(delete_me);
	}

	// destroy the owned windows
	WMrEnumWindows(WMiWindow_DeleteOwned, (UUtUns32)inWindow);

	WMrMessage_Send(inWindow, WMcMessage_NC_Destroy, 0, 0);

	WMiWA_Free(inWindow);

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_GetClientRect(
	WMtWindow				*inWindow,
	UUtRect					*outRect)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	// return the client rect with the upper left corner at (0, 0)
	*outRect = inWindow->client_rect;
	IMrRect_Offset(outRect, -outRect->left, -outRect->top);

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_GetEnabled(
	WMtWindow				*inWindow)
{
	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	return ((inWindow->flags & WMcWindowFlag_Disabled) == 0);
}

// ----------------------------------------------------------------------
UUtUns32
WMrWindow_GetFlags(
	WMtWindow				*inWindow)
{
	if (!WMiWA_Valid(inWindow)) { return 0; }

	return inWindow->flags;
}

// ----------------------------------------------------------------------
WMtWindow*
WMrWindow_GetFocus(
	void)
{
	return WMgKeyboardFocus;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_GetFontInfo(
	WMtWindow				*inWindow,
	TStFontInfo				*outFontInfo)
{
	UUmAssert(inWindow);
	UUmAssert(outFontInfo);

	if (!WMiWA_Valid(inWindow)) { return 0; }

	*outFontInfo = inWindow->font_info;

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtUns16
WMrWindow_GetID(
	WMtWindow				*inWindow)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return 0; }

	return inWindow->id;
}

// ----------------------------------------------------------------------
UUtUns32
WMrWindow_GetLong(
	WMtWindow				*inWindow,
	UUtInt32				inOffset)
{
	UUtUns32				data;

	if (!WMiWA_Valid(inWindow)) { return 0; }

	if ((inOffset < 0) || (inOffset > (UUtInt32)inWindow->window_class->private_data_size))
	{
		return 0;
	}

	data = *((UUtUns32*)((UUtUns32)inWindow + sizeof(WMtWindow) + inOffset));

	return data;
}

// ----------------------------------------------------------------------
WMtWindow*
WMrWindow_GetOwner(
	WMtWindow				*inWindow)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return NULL; }

	return inWindow->owner;
}

// ----------------------------------------------------------------------
WMtWindow*
WMrWindow_GetParent(
	WMtWindow				*inWindow)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return NULL; }

	return inWindow->parent;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_GetRect(
	WMtWindow				*inWindow,
	UUtRect					*outRect)
{
	WMtWindow				*parent;
	WMtWindow				*child;
	UUtInt16				x;
	UUtInt16				y;

	UUmAssert(inWindow);
	UUmAssert(outRect);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	x = inWindow->location.x;
	y = inWindow->location.y;

	if (inWindow->flags & WMcWindowFlag_Child)
	{
		child = inWindow;
		parent = inWindow->parent;
		while (parent)
		{
			if (child->flags & WMcWindowFlag_Child)
			{
				x += parent->location.x + parent->client_rect.left;
				y += parent->location.y + parent->client_rect.top;
			}

			child = parent;
			parent = parent->parent;
		}
	}

	outRect->left = x;
	outRect->top = y;
	outRect->right = outRect->left + inWindow->width;
	outRect->bottom = outRect->top + inWindow->height;

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_GetSize(
	WMtWindow				*inWindow,
	UUtInt16				*outWidth,
	UUtInt16				*outHeight)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	if (outWidth)
	{
		*outWidth = inWindow->width;
	}

	if (outHeight)
	{
		*outHeight = inWindow->height;
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtUns32
WMrWindow_GetStyle(
	WMtWindow				*inWindow)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	return inWindow->style;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_GetTitle(
	WMtWindow				*inWindow,
	char					*outTitle,
	UUtUns16				inMaxCharacters)
{
	UUmAssert(inWindow);
	UUmAssert(outTitle);
	UUmAssert(inMaxCharacters <= WMcMaxTitleLength);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	UUrString_Copy(outTitle, inWindow->title, inMaxCharacters);

	return UUcTrue;
}

// ----------------------------------------------------------------------
char*
WMrWindow_GetTitlePtr(
	WMtWindow				*inWindow)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	return inWindow->title;
}

// ----------------------------------------------------------------------
WMtWindowClass*
WMrWindow_GetClass(
	WMtWindow				*inWindow)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return NULL; }

	return inWindow->window_class;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_GetVisible(
	WMtWindow				*inWindow)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	return ((inWindow->flags & WMcWindowFlag_Visible) == WMcWindowFlag_Visible);
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_GlobalToLocal(
	WMtWindow				*inWindow,
	IMtPoint2D				*inGlobal,
	IMtPoint2D				*outLocal)
{
	UUtRect					window_rect;

	UUmAssert(inWindow);
	UUmAssert(inGlobal);
	UUmAssert(outLocal);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	WMrWindow_GetRect(inWindow, &window_rect);

	outLocal->x = inGlobal->x - inWindow->client_rect.left - window_rect.left;
	outLocal->y = inGlobal->y - inWindow->client_rect.top - window_rect.top;

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_IsChild(
	WMtWindow				*inChild,
	WMtWindow				*inParent)
{
	WMtWindow				*parent;

	if ((inChild == NULL) || (inParent == NULL)) { return UUcFalse; }

	parent = inChild->parent;
	while (parent)
	{
		if (parent == inParent) { return UUcTrue; }
		parent = parent->parent;
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_LocalToGlobal(
	WMtWindow				*inWindow,
	IMtPoint2D				*inLocal,
	IMtPoint2D				*outGlobal)
{
	UUtRect					window_rect;

	UUmAssert(inWindow);
	UUmAssert(inLocal);
	UUmAssert(outGlobal);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	WMrWindow_GetRect(inWindow, &window_rect);

	outGlobal->x = inLocal->x + inWindow->client_rect.left + window_rect.left;
	outGlobal->y = inLocal->y + inWindow->client_rect.top + window_rect.top;

	return UUcTrue;
}

// ----------------------------------------------------------------------
WMtWindow*
WMrWindow_New(
	WMtWindowType			inWindowType,
	char					*inTitle,
	UUtUns32				inFlags,
	UUtUns32				inStyle,
	UUtUns16				inWindowID,
	UUtInt16				inX,
	UUtInt16				inY,
	UUtInt16				inWidth,
	UUtInt16				inHeight,
	WMtWindow				*inParent,
	UUtUns32				inCreationData)
{
	UUtError				error;
	WMtWindow				*window;
	UUtUns32				index;
	WMtWindowClass			*window_class;
	UUtUns32				mem_size;

	UUmAssert(WMgInitialized);

	// there can be only one desktop
	if ((inWindowType == WMcWindowType_Desktop) && (WMgDesktop != NULL))
	{
		return NULL;
	}

	// get the index of an open item in the window array
	index = WMiWA_New();
	if (index == WMcWA_Error)
	{
		UUmAssert(!"Out of windows");
		return NULL;
	}

	// allocate memory for the window
	window_class = WMrWindowClass_GetClassByType(inWindowType);
	if (window_class == NULL) { goto cleanup; }

	mem_size = sizeof(WMtWindow) + window_class->private_data_size;

	window = (WMtWindow*)UUrMemory_Block_NewClear(mem_size);
	if (window)
	{
		UUtUns32				result;
		UUtRect					bounds;

		WMiWA_Add(index, window);

		// initialize the window
		window->prev				= NULL;
		window->next				= NULL;
		window->parent				= NULL;
		window->owner				= NULL;
		window->child				= NULL;
		window->window_class		= window_class;
		window->flags				= inFlags;
		window->style				= inStyle;
		window->id					= inWindowID;
		window->index				= index;
		window->location.x			= inX;
		window->location.y			= inY;
		window->width				= inWidth;
		window->height				= inHeight;
		window->client_rect.left	= 0;
		window->client_rect.top		= 0;
		window->client_rect.right	= 0;
		window->client_rect.bottom	= 0;
		UUrString_Copy(window->title, inTitle, WMcMaxTitleLength);

		DCrText_GetStringRect(window->title, &bounds);

		window->title_width			= bounds.right - bounds.left;
		window->title_height		= bounds.bottom - bounds.top;

		window->had_focus			= window;

		error = TSrFontFamily_Get(TScFontFamily_Default, &window->font_info.font_family);
		window->font_info.font_size = TScFontSize_Default;
		window->font_info.font_style = TScFontStyle_Default;
		window->font_info.font_shade = TScFontShade_Default;

		// set the parent and owner
		if (window->flags & WMcWindowFlag_Child)
		{
			WMtWindow				*owner;

			UUmAssert(inParent);
			window->parent = inParent;

			// only a top level window can be the owner of a child window
			owner = inParent;
			while (owner)
			{
				if ((owner->flags & WMcWindowFlag_Child) == 0)
				{
					window->owner = owner;
					break;
				}
				owner = owner->parent;
			}
		}
		else
		{
			window->parent = NULL;
			window->owner = inParent;
		}

		// add the window to a list
		error = WMiWindow_AddToList(window);
		if (error != UUcError_None) { goto cleanup; }

		// send WMcMessage_NC_Create
		result =
			WMrMessage_Send(
				window,
				WMcMessage_NC_Create,
				inCreationData,
				0);
		if (result == WMcResult_Error)
		{
			goto cleanup;
		}

		// calculate the client rect
		WMrMessage_Send(
			window,
			WMcMessage_NC_CalcClientSize,
			(UUtUns32)&window->client_rect,
			0);

		// send WMcMessage_Create
		result =
			WMrMessage_Send(
				window,
				WMcMessage_Create,
				inCreationData,
				0);
		if (result == WMcResult_Error)
		{
			goto cleanup;
		}

		// tell parent that the child was created
		if (window->parent)
		{
			WMrMessage_Send(
				window->parent,
				WMcMessage_ParentNotify,
				WMcMessage_Create,
				(UUtUns32)window);
		}

		// activate the new window
		WMrWindow_Activate(window);
	}

	UUrMemory_Block_VerifyList();

	return window;

cleanup:
	if (window)
	{
		WMiWindow_RemoveFromList(window);

		WMiWA_Free(window);

		UUrMemory_Block_Delete(window);
	}

	return NULL;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_PointInWindow(
	WMtWindow				*inWindow,
	IMtPoint2D				*inPoint)
{
	UUtRect					bounds;

	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	WMrWindow_GetRect(inWindow, &bounds);
	return IMrRect_PointIn(&bounds, inPoint);
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_SetEnabled(
	WMtWindow				*inWindow,
	UUtBool					inEnabled)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	if (inEnabled)
	{
		inWindow->flags &= ~WMcWindowFlag_Disabled;
	}
	else
	{
		inWindow->flags |= WMcWindowFlag_Disabled;
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_SetFocus(
	WMtWindow				*inWindow)
{
	if (WMgKeyboardFocus)
	{
		WMrMessage_Send(
			WMgKeyboardFocus,
			WMcMessage_KillFocus,
			(UUtUns32)inWindow,
			0);
	}

	WMgKeyboardFocus = inWindow;

	if (inWindow)
	{
		WMtWindow			*window_losing_focus;

		window_losing_focus = WMgKeyboardFocus;

		WMrMessage_Send(
			inWindow,
			WMcMessage_SetFocus,
			(UUtUns32)window_losing_focus,
			0);
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_SetFontInfo(
	WMtWindow				*inWindow,
	const TStFontInfo		*inFontInfo)
{
	UUmAssert(inWindow);
	UUmAssert(inFontInfo);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	inWindow->font_info = *inFontInfo;

	if (inFontInfo->font_family == NULL)
	{
		UUtError				error;

		error =
			TSrFontFamily_Get(
				TScFontFamily_Default,
				&inWindow->font_info.font_family);
		if (error != UUcError_None) { return UUcFalse; }
	}

	WMrMessage_Send(inWindow, WMcMessage_FontInfoChanged, 0, 0);

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_SetLocation(
	WMtWindow				*inWindow,
	UUtInt16				inX,
	UUtInt16				inY)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	return
		WMrWindow_SetPosition(
			inWindow,
			NULL,
			inX,
			inY,
			inWindow->width,
			inWindow->height,
			WMcPosChangeFlag_NoSize | WMcPosChangeFlag_NoZOrder);
}

// ----------------------------------------------------------------------
UUtUns32
WMrWindow_SetLong(
	WMtWindow				*inWindow,
	UUtInt32				inOffset,
	UUtUns32				inData)
{
	UUtUns32				data;

	if (!WMiWA_Valid(inWindow)) { return 0; }

	if ((inOffset < 0) || (inOffset > (UUtInt32)inWindow->window_class->private_data_size))
	{
		return 0;
	}

	data = *((UUtUns32*)((UUtUns32)inWindow + sizeof(WMtWindow) + inOffset));

	*(UUtUns32*)((UUtUns32)inWindow + sizeof(WMtWindow) + inOffset) = inData;

	return data;
}

// ----------------------------------------------------------------------
WMtWindow*
WMrWindow_SetParent(
	WMtWindow				*inWindow,
	WMtWindow				*inNewParent)
{
	WMtWindow				*old_parent;

	UUmAssert(inWindow);
	UUmAssert(inNewParent);

	if (!WMiWA_Valid(inWindow)) { return NULL; }

	old_parent = inWindow->parent;

	if (old_parent)
	{
		WMiWindow_RemoveFromList(inWindow);
	}

	if (inNewParent)
	{
		inWindow->parent = inNewParent;
		WMiWindow_AddToList(inWindow);
	}
	else
	{
		inWindow->parent = NULL;
		WMiWindow_AddToList(inWindow);
	}

	return old_parent;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_SetPosition(
	WMtWindow				*inWindow,
	WMtWindow				*inInsertAfter,
	UUtInt16				inX,
	UUtInt16				inY,
	UUtInt16				inWidth,
	UUtInt16				inHeight,
	UUtUns16				inFlags)
{
	WMtPosChange			pos_change;

	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	// init pos_change
	pos_change.insert_after = inInsertAfter;
	pos_change.x = inX;
	pos_change.y = inY;
	pos_change.width = inWidth;
	pos_change.height = inHeight;
	pos_change.flags = inFlags;

	// tell the window that the position is about to change
	WMrMessage_Send(
		inWindow,
		WMcMessage_PositionChanging,
		(UUtUns32)&pos_change,
		0);

	// change the location field
	if ((pos_change.flags & WMcPosChangeFlag_NoMove) == 0)
	{
		inWindow->location.x = pos_change.x;
		inWindow->location.y = pos_change.y;
	}

	// change the width and height
	if ((pos_change.flags & WMcPosChangeFlag_NoSize) == 0)
	{
		UUmAssert(inWidth > 0);
		UUmAssert(inHeight > 0);

		inWindow->width = pos_change.width;
		inWindow->height = pos_change.height;
	}

	// change the z order
	if ((pos_change.flags & WMcPosChangeFlag_NoZOrder) == 0)
	{
		// insert the child
		WMiWindow_InsertAfterChild(inWindow, pos_change.insert_after);
	}

	// recalculate the client rect
	WMrMessage_Send(
		inWindow,
		WMcMessage_NC_CalcClientSize,
		(UUtUns32)&inWindow->client_rect,
		0);

	// tell the window that the position changed
	WMrMessage_Send(
		inWindow,
		WMcMessage_PositionChanged,
		(UUtUns32)&pos_change,
		0);

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_SetSize(
	WMtWindow				*inWindow,
	UUtInt16				inWidth,
	UUtInt16				inHeight)
{
	UUmAssert(inWindow);
	UUmAssert(inWidth > 0);
	UUmAssert(inHeight > 0);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	return
		WMrWindow_SetPosition(
			inWindow,
			NULL,
			inWindow->location.x,
			inWindow->location.y,
			inWidth,
			inHeight,
			WMcPosChangeFlag_NoMove | WMcPosChangeFlag_NoZOrder);
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_SetTitle(
	WMtWindow				*inWindow,
	const char				*inTitle,
	UUtUns16				inMaxCharacters)
{
	UUtRect					bounds;

	UUmAssert(inWindow);
	UUmAssert(inTitle);
	UUmAssert(inMaxCharacters <= WMcMaxTitleLength);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	UUrString_Copy(inWindow->title, inTitle, inMaxCharacters);

	DCrText_GetStringRect(inWindow->title, &bounds);

	inWindow->title_width = bounds.right - bounds.left;
	inWindow->title_height = bounds.bottom - bounds.top;

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
WMrWindow_SetVisible(
	WMtWindow				*inWindow,
	UUtBool					inVisible)
{
	UUmAssert(inWindow);

	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	if (inVisible)
	{
		inWindow->flags |= WMcWindowFlag_Visible;
	}
	else
	{
		inWindow->flags &= ~WMcWindowFlag_Visible;
	}

	WMrMessage_Send(
		inWindow,
		WMcMessage_Visible,
		(UUtUns32)inVisible,
		0);

	return UUcTrue;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrWindowClass_Register(
	WMtWindowClass			*inWindowClass)
{
	UUtUns32				i;

	UUmAssert(inWindowClass);
	UUmAssert(inWindowClass->callback);

	// look for a previously registered class of inType
	for (i = 0; i < WMgNumWindowClasses; i++)
	{
		if (WMgWindowClassList[i].type == inWindowClass->type)
		{
			return UUcError_Generic;
		}
	}

	// add to the class list
	WMgWindowClassList[WMgNumWindowClasses] = *inWindowClass;
	WMgNumWindowClasses++;

	return UUcError_None;
}

// ----------------------------------------------------------------------
WMtWindowClass*
WMrWindowClass_GetClassByType(
	WMtWindowType			inType)
{
	UUtUns32				i;

	// find the class of type inType
	for (i = 0; i < WMgNumWindowClasses; i++)
	{
		if (WMgWindowClassList[i].type == inType)
		{
			return &WMgWindowClassList[i];
		}
	}

	return NULL;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiCaret_Draw(
	WMtWindow				*inWindow)
{
	DCtDrawContext			*draw_context;
	PStPartSpecUI			*partspec_ui;
	UUtUns32				time;

	// make sure the caret can be drawn
	if ((WMgCaret.owner == NULL) ||
		(WMgCaret.owner != inWindow) ||
		(WMgCaret.visible == UUcFalse))
	{
		return;
	}

	// do the blinking
	time = UUrMachineTime_Sixtieths();
	if (WMgCaret.blink_time < time)
	{
		if (WMgCaret.blink_visible)
		{
			WMgCaret.blink_visible = UUcFalse;
			WMgCaret.blink_time = time + WMcCaret_BlinkOffTime;
			return;
		}
		else
		{
			WMgCaret.blink_visible = UUcTrue;
			WMgCaret.blink_time = time + WMcCaret_BlinkOnTime;
		}
	}

	if (WMgCaret.blink_visible == UUcFalse) { return; }

	// get the active partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }

	draw_context = DCrDraw_Begin(inWindow);

	// draw the caret
	DCrDraw_PartSpec(
		draw_context,
		partspec_ui->text_caret,
		PScPart_MiddleMiddle,
		&WMgCaret.position,
		WMgCaret.width,
		WMgCaret.height,
		M3cMaxAlpha);

	DCrDraw_End(draw_context, inWindow);
}

// ----------------------------------------------------------------------
UUtBool
WMrCaret_Create(
	WMtWindow				*inWindow,
	UUtInt16				inWidth,
	UUtInt16				inHeight)
{
	if (!WMiWA_Valid(inWindow)) { return UUcFalse; }

	// initialize the caret
	WMgCaret.owner = inWindow;
	WMgCaret.position.x = 0;
	WMgCaret.position.y = 0;
	WMgCaret.width = inWidth;
	WMgCaret.height = inHeight;
	WMgCaret.visible = UUcFalse;
	WMgCaret.blink_visible = UUcTrue;
	WMgCaret.blink_time = UUrMachineTime_Sixtieths();

	return UUcTrue;
}

// ----------------------------------------------------------------------
void
WMrCaret_Destroy(
	void)
{
	WMgCaret.owner = NULL;
	WMgCaret.visible = UUcFalse;
}

// ----------------------------------------------------------------------
void
WMrCaret_GetPosition(
	IMtPoint2D				*outPosition)
{
	*outPosition = WMgCaret.position;
}

// ----------------------------------------------------------------------
void
WMrCaret_SetPosition(
	UUtInt16				inX,
	UUtInt16				inY)
{
	WMgCaret.position.x = inX;
	WMgCaret.position.y = inY;
}

// ----------------------------------------------------------------------
void
WMrCaret_SetVisible(
	UUtBool					inIsVisible)
{
	WMgCaret.visible = inIsVisible;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
WMrTimer_Initialize(
	void)
{
	WMgTimers =
		UUrMemory_Array_New(
			sizeof(WMtTimer),
			WMcTimer_InitialNumber,
			0,
			WMcTimer_InitialNumber);
	UUmError_ReturnOnNull(WMgTimers);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
WMrTimer_Start(
	UUtUns32				inTimerID,
	UUtUns32				inTimerFrequency,
	WMtWindow				*inWindow)
{
	UUtUns32				i;
	UUtUns32				num_timers;
	WMtTimer				*timer_array;
	UUtError				error;
	UUtBool					mem_moved;

	UUmAssert(inWindow);

	num_timers = UUrMemory_Array_GetUsedElems(WMgTimers);
	timer_array = (WMtTimer*)UUrMemory_Array_GetMemory(WMgTimers);

	// make sure there are no duplicates
	for (i = 0; i < num_timers; i++)
	{
		if ((timer_array[i].timer_id == inTimerID) &&
			(timer_array[i].window == inWindow))
		{
			return UUcFalse;
		}
	}

	// add a timer
	error = UUrMemory_Array_GetNewElement(WMgTimers, &i, &mem_moved);
	if (error != UUcError_None) { return UUcFalse; }
	if (mem_moved)
	{
		timer_array = (WMtTimer*)UUrMemory_Array_GetMemory(WMgTimers);
	}

	timer_array[i].window = inWindow;
	timer_array[i].timer_id = inTimerID;
	timer_array[i].timer_frequency = inTimerFrequency;
	timer_array[i].timer_next_firing = UUrMachineTime_Sixtieths() + inTimerFrequency;

	return UUcTrue;
}

// ----------------------------------------------------------------------
void
WMrTimer_Stop(
	UUtUns32				inTimerID,
	WMtWindow				*inWindow)
{
	UUtUns32				i;
	WMtTimer				*timer_array;

	UUmAssert(inWindow);

	// find the timer in the list
	timer_array = (WMtTimer*)UUrMemory_Array_GetMemory(WMgTimers);
	for (i = 0; i < UUrMemory_Array_GetUsedElems(WMgTimers); i++)
	{
		if ((timer_array[i].timer_id == inTimerID) &&
			(timer_array[i].window == inWindow))
		{
			UUrMemory_Array_DeleteElement(WMgTimers, i);
		}
	}
}

// ----------------------------------------------------------------------
static void
WMiTimer_Terminate(
	void)
{
	UUrMemory_Array_Delete(WMgTimers);
	WMgTimers = NULL;
}

// ----------------------------------------------------------------------
static void
WMiTimer_Update(
	void)
{
	UUtUns32				i;
	UUtUns32				time;
	WMtTimer				*timer_array;
	UUtUns32				num_timers;

	time = UUrMachineTime_Sixtieths();

update:
	timer_array = (WMtTimer*)UUrMemory_Array_GetMemory(WMgTimers);
	num_timers = UUrMemory_Array_GetUsedElems(WMgTimers);
	for (i = 0; i < num_timers; i++)
	{
		if (timer_array[i].timer_next_firing < time)
		{
			// calculate the next firing
			timer_array[i].timer_next_firing = time + timer_array[i].timer_frequency;

			// send the timer message
			WMrMessage_Send(
				timer_array[i].window,
				WMcMessage_Timer,
				timer_array[i].timer_id,
				0);
			// don't do any more processing with this timer after the message is sent
			// because the window may have destroyed it or itself during the message

			if (num_timers != UUrMemory_Array_GetUsedElems(WMgTimers))
			{
				// the timer and or window was delete start the update over
				goto update;
			}
		}
	}
}

// ----------------------------------------------------------------------
static void
WMiTimer_WindowDestroyed(
	WMtWindow				*inWindow)
{
	UUtUns32				i;
	WMtTimer				*timer_array;

destroyer:
	timer_array = (WMtTimer*)UUrMemory_Array_GetMemory(WMgTimers);
	for (i = 0; i < UUrMemory_Array_GetUsedElems(WMgTimers); i++)
	{
		if (timer_array[i].window == inWindow)
		{
			UUrMemory_Array_DeleteElement(WMgTimers, i);
			goto destroyer;
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
isarrow(
	UUtInt16				inKey)
{
	UUtBool					result;

	switch (inKey)
	{
		case LIcKeyCode_UpArrow:
		case LIcKeyCode_DownArrow:
		case LIcKeyCode_LeftArrow:
		case LIcKeyCode_RightArrow:
			result = UUcTrue;
		break;

		default:
			result = UUcFalse;
		break;
	}

	return result;
}

// ----------------------------------------------------------------------
static UUtBool
isdelete(
	UUtInt16				inKey)
{
	UUtBool					result;

	switch (inKey)
	{
		case LIcKeyCode_BackSpace:
		case LIcKeyCode_Delete:
			result = UUcTrue;
		break;

		default:
			result = UUcFalse;
		break;
	}

	return result;
}

// ----------------------------------------------------------------------
static UUtBool
isnavigation(
	UUtInt16				inKey)
{
	UUtBool					result;

	switch (inKey)
	{
		case LIcKeyCode_Home:
		case LIcKeyCode_End:
		case LIcKeyCode_PageUp:
		case LIcKeyCode_PageDown:
			result = UUcTrue;
		break;

		default:
			result = UUcFalse;
		break;
	}

	return result;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiEvent_GetKeyCategory(
	LItInputEvent			*inInputEvent)
{
	UUtUns32				result;

	result = WMcKeyCategory_None;

	if (isalpha(inInputEvent->key) != 0)		{ result |= WMcKeyCategory_Alpha; }
	if (isdigit(inInputEvent->key) != 0)		{ result |= WMcKeyCategory_Digit; }
	if (ispunct(inInputEvent->key) != 0)		{ result |= WMcKeyCategory_Punctuation; }
	if (isarrow(inInputEvent->key) != 0)		{ result |= WMcKeyCategory_Arrow; }
	if (isnavigation(inInputEvent->key) != 0)	{ result |= WMcKeyCategory_Navigation; }

	// don't let delete be part of control
	if (isdelete(inInputEvent->key) != 0)		{ result |= WMcKeyCategory_Delete; }
	else if (iscntrl(inInputEvent->key) != 0)	{ result |= WMcKeyCategory_Control; }

	return result;
}

// ----------------------------------------------------------------------
static UUtBool
WMiEvent_IsDoubleClick(
	LItInputEvent			*inInputEvent,
	WMtWindow				*inWindow,
	WMtMessage				*ioMessage)
{
	UUtBool					is_double_click;
	WMtMessage				message;

	is_double_click = UUcFalse;
	message = *ioMessage;

	switch (message)
	{
		case WMcMessage_NC_LMouseUp:
		case WMcMessage_NC_MMouseUp:
		case WMcMessage_NC_RMouseUp:
		case WMcMessage_LMouseUp:
		case WMcMessage_MMouseUp:
		case WMcMessage_RMouseUp:
		{
			UUtUns32			time;
			float				distance;

			time = UUrMachineTime_Sixtieths();
			distance =
				IMrPoint2D_Distance(
					&WMgEventData.prev_mouse_click_location,
					&inInputEvent->where);

			if ((WMgEventData.prev_mouse_click_window == inWindow) &&
				(WMgEventData.prev_mouse_click_message == message) &&
				(time < (WMgEventData.prev_mouse_click_time + WMcDoubleClickDelta)) &&
				(distance <= WMcMaxDoubleClickDistance))
			{
				switch (message)
				{
					case WMcMessage_NC_LMouseUp: message = WMcMessage_NC_LMouseDblClck; break;
					case WMcMessage_NC_MMouseUp: message = WMcMessage_NC_MMouseDblClck; break;
					case WMcMessage_NC_RMouseUp: message = WMcMessage_NC_RMouseDblClck; break;
					case WMcMessage_LMouseUp: message = WMcMessage_LMouseDblClck; break;
					case WMcMessage_MMouseUp: message = WMcMessage_MMouseDblClck; break;
					case WMcMessage_RMouseUp: message = WMcMessage_RMouseDblClck; break;
				}

				// clear the info
				WMgEventData.prev_mouse_click_window = NULL;
				WMgEventData.prev_mouse_click_time = 0;
				WMgEventData.prev_mouse_click_message = WMcMessage_None;

				is_double_click = UUcTrue;
			}
			else
			{
				// record the necessary info
				WMgEventData.prev_mouse_click_window = inWindow;
				WMgEventData.prev_mouse_click_time = time;
				WMgEventData.prev_mouse_click_message = message;
				WMgEventData.prev_mouse_click_location = inInputEvent->where;
			}
		}
		break;
	}

	*ioMessage = message;
	return is_double_click;
}

// ----------------------------------------------------------------------
static void
WMiEvents_HandleMouseEvent(
	LItInputEvent			*inInputEvent)
{
	WMtWindow				*window;
	WMtMessage				message;
	UUtUns32				param1;
	UUtUns32				param2;

	WMtMouseEventMap		*event_map;

	WMtWindowArea			part;
	UUtBool					is_dbl_click;


	// record the cursor position for the cursor drawing to happen later
	WMgCursorPosition = inInputEvent->where;

	// get the window under the mouse
	window = WMrGetWindowUnderPoint(&inInputEvent->where);

	// check the undesirable flags
	if (NULL != window)
	{
		UUtUns32	undesirable_flags = WMcWindowFlag_Disabled | WMcWindowFlag_MouseTransparent;

		if ((window->flags & undesirable_flags) != 0)
		{
			window = NULL;
		}
	}

	// set the window to the Mouse Focus window
	if (WMgMouseFocus)
	{
		window = WMgMouseFocus;
	}

	// set param1
	param1 = UUmMakeLong(inInputEvent->where.x, inInputEvent->where.y);

	// get the part of the window the mouse is over
	if (window == NULL)
	{
		part = WMcWindowArea_Client;
	}
	else
	{
		part =
			(WMtWindowArea)WMrMessage_Send(
				window,
				WMcMessage_NC_HitTest,
				param1,
				0);
	}

	// set the message type based on the hittest
	if (part == WMcWindowArea_Client)
	{
		event_map = WMgMouseEventMap;
		param2 = inInputEvent->modifiers;
	}
	else
	{
		event_map = WMgMouseNCEventMap;
		param2 = (UUtUns32)part;
	}

	for (;
		 event_map->window_message != WMcMessage_None;
		 event_map++)
	{
		if (event_map->event_type == inInputEvent->type)
		{
			message = event_map->window_message;
			break;
		}
	}

	// don't generate WMcMessage_NC_MouseMove messages when the mouse is captured
	if ((WMgMouseFocus) && (message == WMcMessage_NC_MouseMove))
	{
		message = WMcMessage_MouseMove;
	}

	// check for double clicks
	is_dbl_click = WMiEvent_IsDoubleClick(inInputEvent, window, &message);

	// send a message to the window
	WMrMessage_Post(window, message, param1, param2);

	// send a mouse up message if message is a double clicked message
	if (is_dbl_click)
	{
		switch (message)
		{
			case WMcMessage_NC_LMouseDblClck: message = WMcMessage_NC_LMouseUp; break;
			case WMcMessage_NC_MMouseDblClck: message = WMcMessage_NC_MMouseUp; break;
			case WMcMessage_NC_RMouseDblClck: message = WMcMessage_NC_RMouseUp; break;
			case WMcMessage_LMouseDblClck: message = WMcMessage_LMouseUp; break;
			case WMcMessage_MMouseDblClck: message = WMcMessage_MMouseUp; break;
			case WMcMessage_RMouseDblClck: message = WMcMessage_RMouseUp; break;
		}

		WMrMessage_Post(window, message, param1, param2);
	}
}

// ----------------------------------------------------------------------
static void
WMiEvents_HandleKeyEvent(
	LItInputEvent				*inInputEvent)
{
	WMtMessage					message;
	UUtUns32					param1;
	UUtUns32					param2;
	WMtWindow					*window;

	if (WMgKeyboardFocus == NULL) return;

	// set param1 and param2
	param1 = (UUtUns32)inInputEvent->key;
	param2 = (UUtUns32)inInputEvent->modifiers;

	// set the message
	switch (inInputEvent->type)
	{
		case LIcInputEvent_KeyDown:
		case LIcInputEvent_KeyRepeat:
			message = WMcMessage_KeyDown;
		break;

		case LIcInputEvent_KeyUp:
			message = WMcMessage_KeyUp;
		break;
	}

	// if the keyboard focus is the child window of a dialog then determine
	// if the child or the dialog should get the keyboard event
	window = WMgKeyboardFocus;
	if (window->flags & WMcWindowFlag_Child)
	{
		WMtWindow				*parent;

		parent = WMrWindow_GetParent(window);
		if ((parent) && (parent->window_class->type == WMcWindowType_Dialog))
		{
			UUtUns32			result;
			UUtUns32			key_category;

			key_category = WMiEvent_GetKeyCategory(inInputEvent);
			result = WMrMessage_Send(window, WMcMessage_GetDialogCode, 0, 0);
			switch (key_category)
			{
				case WMcKeyCategory_Alpha:
					if ((result & WMcDialogCode_WantAlphas) == 0) { window = parent; }
				break;

				case WMcKeyCategory_Digit:
					if ((result & WMcDialogCode_WantDigits) == 0) { window = parent; }
				break;

				case WMcKeyCategory_Punctuation:
					if ((result & WMcDialogCode_WantPunctuation) == 0) { window = parent; }
				break;

				case WMcKeyCategory_Control:
					window = parent;
				break;

				case WMcKeyCategory_Navigation:
					if ((result & WMcDialogCode_WantNavigation) == 0) { window = parent; }
				break;

				case WMcKeyCategory_Arrow:
					if ((result & WMcDialogCode_WantArrows) == 0) { window = parent; }
				break;

				case WMcKeyCategory_Delete:
					if ((result & WMcDialogCode_WantDelete) == 0) { window = parent; }
				break;
			}
		}
	}

	// send a message to the focused view
	WMrMessage_Post(window, message, param1, param2);
}

// ----------------------------------------------------------------------
static void
WMiEvents_Process(
	void)
{
	UUtUns32					i;

	if (LIrMode_Get() == LIcMode_Game) return;

	// get the events and send them to the dialog
	for (i = 0; i < WMcMaxEventsPerUpdate; i++)
	{
		UUtBool					event_avail;
		LItInputEvent			input_event;
		// get an event from the input context
		event_avail = LIrInputEvent_Get(&input_event);
		if (event_avail)
		{
			// don't let the mouse get outside of the desktop
			if (input_event.where.x < 0) { input_event.where.x = 0; }
			if (input_event.where.y < 0) { input_event.where.y = 0; }
			if (WMgDesktop)
			{
				if (input_event.where.x > WMgDesktop->width)
					{ input_event.where.x = WMgDesktop->width; }
				if (input_event.where.y > WMgDesktop->height)
					{ input_event.where.y = WMgDesktop->height; }
			}

			// handle the input event
			switch (input_event.type)
			{
				case LIcInputEvent_MouseMove:
				case LIcInputEvent_LMouseDown:
				case LIcInputEvent_LMouseUp:
				case LIcInputEvent_MMouseDown:
				case LIcInputEvent_MMouseUp:
				case LIcInputEvent_RMouseDown:
				case LIcInputEvent_RMouseUp:
					WMiEvents_HandleMouseEvent(&input_event);
				break;

				case LIcInputEvent_KeyDown:
		 		case LIcInputEvent_KeyRepeat:
		 			WMiEvents_HandleKeyEvent(&input_event);
				break;
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
WMiCursor_Load(
	void)
{
	// load the cursor
	WMgCursor = WMrCursor_Get(WMcCursorType_Arrow);
	if (WMgCursor == NULL)
	{
		return UUcError_Generic;
	}

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiDesktop_Paint(
	WMtWindow				*inDesktop)
{
	void					*background;
	TMtTemplateTag			template_tag;
	DCtDrawContext			*draw_context;
	IMtPoint2D				dest;
	UUtInt16				desktop_width;
	UUtInt16				desktop_height;

	// start drawing
	draw_context = DCrDraw_Begin(inDesktop);

	// get the background
	background = (void*)WMrWindow_GetLong(inDesktop, 0);
	if (background == 0) { return; }

	// get the size of the desktop
	WMrWindow_GetSize(inDesktop, &desktop_width, &desktop_height);

	// draw the appropriate template type
	template_tag = TMrInstance_GetTemplateTag(background);
	switch (template_tag)
	{
		case PScTemplate_PartSpecification:
			// set the dest
			dest.x = 0;
			dest.y = 0;

			// draw the partspec
			DCrDraw_PartSpec(
				draw_context,
				(PStPartSpec*)background,
				PScPart_MiddleMiddle,
				&dest,
				desktop_width,
				desktop_height,
				M3cMaxAlpha);
		break;

		case M3cTemplate_TextureMap:
		case M3cTemplate_TextureMap_Big:
		{
			UUtUns16				width;
			UUtUns16				height;

			// get the size of the texture
			M3rTextureRef_GetSize(background, &width, &height);

			// set the dest
			dest.x = (desktop_width - (UUtInt16)width) >> 1;
			dest.y = (desktop_height - (UUtInt16)height) >> 1;

			DCrDraw_TextureRef(
				draw_context,
				background,
				&dest,
				width,
				height,
				UUcFalse,
				M3cMaxAlpha);
		}
		break;
	}

	// stop drawing
	DCrDraw_End(draw_context, inDesktop);
}

// ----------------------------------------------------------------------
static UUtUns32
WMiDesktop_Callback(
	WMtWindow				*inDesktop,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	switch(inMessage)
	{
		case WMcMessage_Create:
			WMrWindow_SetLong(inDesktop, 0, 0);
		return WMcResult_Handled;

		case WMcMessage_Destroy:
			WMrWindow_SetLong(inDesktop, 0, 0);
		return WMcResult_Handled;

		case WMcMessage_Paint:
			WMiDesktop_Paint(inDesktop);
		return WMcResult_Handled;

		case WMcMessage_ResolutionChanged:
			WMrWindow_SetSize(
				inDesktop,
				(UUtInt16)UUmHighWord(inParam1),
				(UUtInt16)UUmLowWord(inParam1));
		break;

		case DTcMessage_SetBackground:
			WMrWindow_SetLong(inDesktop, 0, inParam1);
		return WMcResult_Handled;
	}

	return WMrWindow_DefaultCallback(inDesktop, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
static UUtError
WMiDesktop_Create(
	void)
{
	WMtWindowClass			window_class;
	UUtError				error;

	UUmAssert(WMgDesktop == NULL);

	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_Desktop;
	window_class.callback = WMiDesktop_Callback;
	window_class.private_data_size = sizeof(UUtUns32);

	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);

	// create the oni window
	WMgDesktop =
		WMrWindow_New(
			WMcWindowType_Desktop,
			"Desktop",
			WMcWindowFlag_Visible | WMcWindowFlag_MouseTransparent,
			WMcDesktopStyle_None,
			0,
			0,
			0,
			M3rDraw_GetWidth(),
			M3rDraw_GetHeight(),
			NULL,
			0);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
WMrMessage_Dispatch(
	WMtEvent				*inEvent)
{
	UUtUns32				result;

	if (inEvent->window)
	{
		WMtWindow		*mouse_over;
		IMtPoint2D		point;

		// get the window the mouse was actually clicked on
		mouse_over = NULL;
		switch (inEvent->message)
		{
			case WMcMessage_NC_LMouseDown:
			case WMcMessage_NC_MMouseDown:
			case WMcMessage_NC_RMouseDown:
			case WMcMessage_LMouseDown:
			case WMcMessage_MMouseDown:
			case WMcMessage_RMouseDown:
				// get the window the mouse is over
				point.x = UUmHighWord(inEvent->param1);
				point.y = UUmLowWord(inEvent->param1);
				mouse_over = WMrGetWindowUnderPoint(&point);
				if (mouse_over == inEvent->window) { mouse_over = NULL; }

				// activate the window the user clicked on
				WMrWindow_Activate(inEvent->window);
			break;
		}

		// send the message to the window
		result =
			WMrMessage_Send(
				inEvent->window,
				inEvent->message,
				inEvent->param1,
				inEvent->param2);

		// activate the window the mouse was actually clicked on
		if (mouse_over) { WMrWindow_Activate(mouse_over); }
	}
	else
	{
		result = WMcResult_NoWindow;
	}

	return result;
}

// ----------------------------------------------------------------------
UUtBool
WMrMessage_Get(
	WMtEvent				*outEvent)
{
	// if the event pointed to by the head has a message of none,
	// there are no messages
	if (WMgEvents[WMgEventHead].message == WMcMessage_None)
	{
		return UUcFalse;
	}

	// set the output
	*outEvent = WMgEvents[WMgEventHead];

	// clear the message
	WMgEvents[WMgEventHead].message = WMcMessage_None;

	// advance the message head to the next message, but don't pass the tail
	if (WMgEventHead != WMgEventTail)
	{
		WMgEventHead++;
		if (WMgEventHead >= WMcMaxNumEvents)
		{
			WMgEventHead = 0;
		}
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtError
WMiMessage_Initialize(
	void)
{
	UUtUns32				i;

	// initialize the events
	for (i = 0; i < WMcMaxNumEvents; i++)
	{
		WMgEvents[i].window = NULL;
		WMgEvents[i].message = WMcMessage_None;
		WMgEvents[i].param1 = 0;
		WMgEvents[i].param2 = 0;
	}

	// initialize the queue indices
	WMgEventHead = 0;
	WMgEventTail = 0;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
WMrMessage_Post(
	WMtWindow				*inWindow,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	// store the event at the tail
	WMgEvents[WMgEventTail].window = inWindow;
	WMgEvents[WMgEventTail].message = inMessage;
	WMgEvents[WMgEventTail].param1 = inParam1;
	WMgEvents[WMgEventTail].param2 = inParam2;

	// update the tail
	WMgEventTail++;
	if (WMgEventTail >= WMcMaxNumEvents)
	{
		WMgEventTail = 0;
	}

	if (WMgEventTail == WMgEventHead)
	{
		UUmAssert(!"went all the way araound");
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtUns32
WMrMessage_Send(
	WMtWindow				*inWindow,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	if (inWindow == NULL) { return 0; }

	if (!WMiWA_Valid(inWindow)) { return 0; }

	return inWindow->window_class->callback(inWindow, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtBool
WMrMessage_TranslateKeyCommand(
	WMtEvent				*inEvent,
	WMtKeyboardCommands		*inKeyboardCommands,
	WMtWindow				*inDestWindow)
{
	UUtUns32				i;

	UUmAssert(inEvent);
	UUmAssert(inKeyboardCommands);

	// only key downs get changed into commands
	if (inEvent->message != WMcMessage_KeyDown) { return UUcFalse; }
	if (inEvent->param2 != LIcKeyState_CommandDown) { return UUcFalse; }

	// look for the key in the inKeyboardCommands table
	for (i = 0; i < inKeyboardCommands->num_commands; i++)
	{
		if ((UUtUns16)inEvent->param1 == inKeyboardCommands->commands[i])
		{
			// change the message
			inEvent->window = inDestWindow;
			inEvent->message = WMcMessage_KeyCommand;
			inEvent->param2 = 0;

			// exit
			return UUcTrue;
		}
	}

	return UUcFalse;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
WMrActivate(
	void)
{
	LItInputEvent			event;

	WMgActive = UUcTrue;
	WMrCursor_SetVisible(UUcTrue);

	// get the current mouse position
	LIrInputEvent_GetMouse(&event);
	WMgCursorPosition = event.where;
}

// ----------------------------------------------------------------------
void
WMrDisplay(
	void)
{
	// set the draw state
	M3rDraw_State_Push();
	M3rDraw_State_SetInt(M3cDrawStateIntType_SubmitMode, M3cDrawState_SubmitMode_Normal);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare, M3cDrawState_ZCompare_Off);

	// draw the windows
	if (WMgDesktop)
	{
		WMiWindow_Draw(WMgDesktop);
	}

	// draw the cursor
	if (WMgCursor)
	{
		WMrCursor_Draw(WMgCursor, &WMgCursorPosition);
	}

	// set the draw state
	M3rDraw_State_Pop();
}

// ----------------------------------------------------------------------
void
WMrDeactivate(
	void)
{
	WMgActive = UUcFalse;
	WMrCursor_SetVisible(UUcFalse);
}

// ----------------------------------------------------------------------
void
WMrEnumWindows(
	WMtWindowEnumCallback	inEnumCallback,
	UUtUns32				inUserData)
{
	WMtWindow				*child;

	UUmAssert(inEnumCallback);

	child = WMgDesktop->child;
	while (child)
	{
		UUtBool				result;
		WMtWindow			*enum_window;

		enum_window = child;
		child = child->next;

		result = inEnumCallback(enum_window, inUserData);
		if (result == UUcFalse) { break; }
	}
}

// ----------------------------------------------------------------------
WMtWindow*
WMrGetDesktop(
	void)
{
	return WMgDesktop;
}

// ----------------------------------------------------------------------
WMtWindow*
WMrGetWindowUnderPoint(
	IMtPoint2D				*inPoint)
{
	return WMiWindow_GetWindowUnderPoint(WMgDesktop, inPoint, WMcWindowFlag_Visible);
}

// ----------------------------------------------------------------------
UUtError
WMrInitialize(
	void)
{
	UUtError				error;

	// initialize globals
	WMgEventOverride	= UUcFalse;
	WMgDesktop			= NULL;
	WMgCursor			= NULL;
	WMgActiveWindow		= NULL;
	WMgMouseFocus		= NULL;
	WMgKeyboardFocus	= NULL;

	// initialize the sub-systems
	error = WMiWA_Initialize();
	UUmError_ReturnOnError(error);

	error = WMiMessage_Initialize();
	UUmError_ReturnOnError(error);

	error = PSrInitialize();
	UUmError_ReturnOnError(error);

	error = WMrTimer_Initialize();
	UUmError_ReturnOnError(error);

	// clear the window class list
	WMgNumWindowClasses = 0;
	UUrMemory_Clear(WMgWindowClassList, sizeof(WMgWindowClassList));

	// initialize the standard windows
	error = WMrBox_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrButton_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrCheckBox_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrDialog_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrEditField_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrListBox_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrMenu_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrMenuBar_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrPicture_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrPopupMenu_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrProgressBar_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrRadioButton_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrScrollbar_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrSlider_Initialize();
	UUmError_ReturnOnError(error);

	error = WMrText_Initialize();
	UUmError_ReturnOnError(error);

	WMgInitialized = UUcTrue;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
WMrLevel_Unload(
	void)
{
	WMiWA_FreeUnused();
}

// ----------------------------------------------------------------------
UUtError
WMrRegisterTemplates(
	void)
{
	UUtError				error;

	error = PSrRegisterTemplates();
	UUmError_ReturnOnError(error);

	error = WMrDialog_RegisterTemplates();
	UUmError_ReturnOnError(error);

	error = WMrCursor_RegisterTemplates();
	UUmError_ReturnOnError(error);

	error = WMrMenu_RegisterTemplates();
	UUmError_ReturnOnError(error);

	error = WMrMenuBar_RegisterTemplates();
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
WMrSetDesktopBackground(
	void					*inBackground)
{
	if (WMgDesktop)
	{
		WMrMessage_Send(
			WMgDesktop,
			DTcMessage_SetBackground,
			(UUtUns32)inBackground,
			0);
	}
}

// ----------------------------------------------------------------------
void
WMrSetResolution(
	UUtInt16				inWidth,
	UUtInt16				inHeight)
{
	if (WMgDesktop)
	{
		WMrMessage_Send(
			WMgDesktop,
			WMcMessage_ResolutionChanged,
			UUmMakeLong(inWidth, inHeight),
			0);
	}
}

// ----------------------------------------------------------------------
UUtError
WMrStartup(
	void)
{
	UUtError				error;

	// initialize the draw context text
	error = DCrText_Initialize();
	UUmError_ReturnOnError(error);

	// create the desktop
	error = WMiDesktop_Create();
	UUmError_ReturnOnError(error);

	// load the cursor
	error = WMiCursor_Load();
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
WMrTerminate(
	void)
{
	// delete the desktop
	if (WMgDesktop)
	{
		WMrWindow_Delete(WMgDesktop);
	}

	// delete the window classes
	WMgNumWindowClasses = 0;
	UUrMemory_Clear(WMgWindowClassList, sizeof(WMgWindowClassList));

	// terminate the sub-systems
	DCrText_Terminate();
	WMiWA_Terminate();
	WMiTimer_Terminate();

	UUrMemory_Block_VerifyList();

	WMgInitialized = UUcFalse;
}

// ----------------------------------------------------------------------
void
WMrUpdate(
	void)
{
	if (WMgActive == UUcFalse) return;

	// ------------------------------
	// update the timers
	// ------------------------------
	WMiTimer_Update();

	// ------------------------------
	// process events
	// ------------------------------
	WMiEvents_Process();

	// ------------------------------
	// update the sound manager
	// ------------------------------
	SS2rUpdate();
}
