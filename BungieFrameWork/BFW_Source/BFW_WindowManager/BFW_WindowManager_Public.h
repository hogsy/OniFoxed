// ======================================================================
// BFW_WindowManager.h
// ======================================================================
#ifndef BFW_WINDOWMANAGER_PUBLIC_H
#define BFW_WINDOWMANAGER_PUBLIC_H

#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TextSystem.h"
#include "WM_PartSpecification.h"

// ======================================================================
// defines
// ======================================================================
#define WMcMaxTitleLength				255

#define WMcMaxKeyboardCommand			30

#define WMcPrefered_PixelType			IMcPixelType_ARGB4444
#define WMcColor_Background				IMcShade_None
#define WMcColor_Text					IMcShade_Black

#define WMcAlpha_Enabled				M3cMaxAlpha
#define WMcAlpha_Disabled				(M3cMaxAlpha >> 1)

#define WMmResult_ReturnOnError(result)	\
	if ((result) != WMcResult_Handled) { return (result); }

// ======================================================================
// enums
// ======================================================================
// window messages
typedef UUtUns32			WMtMessage;

enum
{
	// Window Manager Messages
	WMcMessage_None,

	// Window non-client Messages
	WMcMessage_NC_Create,
	WMcMessage_NC_Destroy,
	WMcMessage_NC_HitTest,
	WMcMessage_NC_Paint,

	WMcMessage_NC_MouseMove,

	WMcMessage_NC_LMouseDown,
	WMcMessage_NC_LMouseUp,
	WMcMessage_NC_LMouseDblClck,

	WMcMessage_NC_MMouseDown,
	WMcMessage_NC_MMouseUp,
	WMcMessage_NC_MMouseDblClck,

	WMcMessage_NC_RMouseDown,
	WMcMessage_NC_RMouseUp,
	WMcMessage_NC_RMouseDblClck,

	WMcMessage_NC_Activate,
	WMcMessage_NC_Update,

	WMcMessage_NC_CalcClientSize,

	// Window Client Messages
	WMcMessage_Create,
	WMcMessage_Destroy,

	WMcMessage_Close,
	WMcMessage_Flatten,
	WMcMessage_Zoom,

	WMcMessage_Command,
	WMcMessage_KeyCommand,
	WMcMessage_MenuCommand,

	WMcMessage_MenuInit,

	WMcMessage_Timer,

	WMcMessage_KeyDown,
	WMcMessage_KeyUp,

	WMcMessage_MouseMove,

	WMcMessage_LMouseDown,
	WMcMessage_LMouseUp,
	WMcMessage_LMouseDblClck,

	WMcMessage_MMouseDown,
	WMcMessage_MMouseUp,
	WMcMessage_MMouseDblClck,

	WMcMessage_RMouseDown,
	WMcMessage_RMouseUp,
	WMcMessage_RMouseDblClck,

	WMcMessage_Activate,
	WMcMessage_SetFocus,
	WMcMessage_KillFocus,

	WMcMessage_Paint,
	WMcMessage_Update,

	WMcMessage_GetValue,
	WMcMessage_SetValue,

	WMcMessage_Visible,

	WMcMessage_CaptureChanged,
	WMcMessage_ParentNotify,

	WMcMessage_PositionChanging,
	WMcMessage_PositionChanged,

	WMcMessage_ResolutionChanged,

	WMcMessage_FontInfoChanged,

	WMcMessage_DrawItem,
	WMcMessage_CompareItems,

	WMcMessage_GetDialogCode,

	WMcMessage_Quit,

	// Toggle Button Messages
	TBcMessage_SetToggle,

	// Check Box Messages
	CBcMessage_SetCheck,

	// Desktop Message
	DTcMessage_SetBackground,

	// Dialog Manager Messages
	WMcMessage_InitDialog,
	WMcMessage_GetDefaultID,
	WMcMessage_SetDefaultID,

	// Edit Field Messages
	EFcMessage_GetText,
	EFcMessage_SetMaxChars,
	EFcMessage_SetText,
	EFcMessage_GetInt,
	EFcMessage_GetFloat,

	// List Box Messages
	LBcMessage_AddString,
	LBcMessage_DeleteString,
	LBcMessage_GetItemData,
	LBcMessage_GetNumLines,
	LBcMessage_GetSelection,
	LBcMessage_GetText,
	LBcMessage_GetTextLength,
	LBcMessage_InsertString,
	LBcMessage_ReplaceString,
	LBcMessage_Reset,
	LBcMessage_SelectString,
	LBcMessage_SetDirectoryInfo,
	LBcMessage_SetItemData,
	LBcMessage_SetNumLines,
	LBcMessage_SetSelection,
	LBcMessage_SetLineColor,	/* unsupported */

	// Picture
	PTcMessage_SetPartSpec,
	PTcMessage_SetPicture,

	// Scroll Bar Messages
	SBcMessage_HorizontalScroll,
	SBcMessage_VerticalScroll,

	// Tab Messages
/*	TbcMessage_InitTab,*/

	// Radio Button Messages
	RBcMessage_SetCheck,

	// all user messages must come after this one
	WMcMessage_User

};




// message results
enum
{
	WMcResult_Handled,
	WMcResult_Error,
	WMcResult_NoWindow
};

// window types
enum
{
	WMcWindowType_None,
	WMcWindowType_Desktop,
	WMcWindowType_Window,
	WMcWindowType_Box,
	WMcWindowType_Button,
	WMcWindowType_CheckBox,
	WMcWindowType_Dialog,
	WMcWindowType_EditField,
	WMcWindowType_ListBox,
	WMcWindowType_MenuBar,
	WMcWindowType_Menu,
	WMcWindowType_Picture,
	WMcWindowType_PopupMenu,
	WMcWindowType_ProgressBar,
	WMcWindowType_RadioButton,
	WMcWindowType_RadioGroup,
	WMcWindowType_Scrollbar,
	WMcWindowType_Slider,
	WMcWindowType_Tab,
	WMcWindowType_TabGroup,
	WMcWindowType_Text,

	WMcWindowType_UserDefined		= 0x4000

};

// window areas
typedef enum WMtWindowArea
{
	WMcWindowArea_None				= 0x0000,

	WMcWindowArea_NC_Border			= 0x0001,

	WMcWindowArea_NC_Left			= 0x0002,
	WMcWindowArea_NC_Right			= 0x0004,
	WMcWindowArea_NC_Top			= 0x0008,
	WMcWindowArea_NC_Bottom			= 0x0010,

	WMcWindowArea_NC_TopLeft		= WMcWindowArea_NC_Top | WMcWindowArea_NC_Left,
	WMcWindowArea_NC_TopRight		= WMcWindowArea_NC_Top | WMcWindowArea_NC_Right,
	WMcWindowArea_NC_BottomLeft		= WMcWindowArea_NC_Bottom | WMcWindowArea_NC_Left,
	WMcWindowArea_NC_BottomRight	= WMcWindowArea_NC_Bottom | WMcWindowArea_NC_Right,

	WMcWindowArea_NC_Drag			= 0x0020,
	WMcWindowArea_NC_Grow			= 0x0040,

	WMcWindowArea_NC_Close			= 0x0080,
	WMcWindowArea_NC_Flatten		= 0x0100,
	WMcWindowArea_NC_Zoom			= 0x0200,

	WMcWindowArea_Client			= 0x8000

} WMtWindowArea;

// window flags
enum
{
	WMcWindowFlag_None				= 0x0000,

	WMcWindowFlag_Visible			= 0x0001,
	WMcWindowFlag_Disabled			= 0x0002,
	WMcWindowFlag_MouseTransparent	= 0x0004,

	WMcWindowFlag_Child				= 0x2000,
	WMcWindowFlag_TopMost			= 0x4000,
	WMcWindowFlag_PopUp				= 0x8000

};

// standard window styles
enum
{
	WMcWindowStyle_None				= 0x00000000,
	WMcWindowStyle_HasBackground	= 0x00000001,
	WMcWindowStyle_HasBorder		= 0x00000002,
	WMcWindowStyle_HasDrag			= 0x00000004,
	WMcWindowStyle_HasTitle			= 0x00000008,
	WMcWindowStyle_HasClose			= 0x00000010,
	WMcWindowStyle_HasZoom			= 0x00000020,
	WMcWindowStyle_HasFlatten		= 0x00000040,
	WMcWindowStyle_HasGrowBox		= 0x00000080	/* not supported yet */

};

#define WMcWindowStyle_Basic		(WMcWindowStyle_HasBackground | WMcWindowStyle_HasDrag | WMcWindowStyle_HasTitle | WMcWindowStyle_HasClose)
#define WMcWindowStyle_Standard		(WMcWindowStyle_Basic | WMcWindowStyle_HasBorder | WMcWindowStyle_HasZoom | WMcWindowStyle_HasFlatten)

// position change flags
enum
{
	WMcPosChangeFlag_None			= 0x0000,
	WMcPosChangeFlag_NoMove			= 0x0001,
	WMcPosChangeFlag_NoSize			= 0x0002,
	WMcPosChangeFlag_NoZOrder		= 0x0004
};

// text flags
enum
{
	WMcCommonFlag_None				= 0x0000,

	WMcCommonFlag_Text_HLeft		= 0x0100,
	WMcCommonFlag_Text_HCenter		= 0x0200,
	WMcCommonFlag_Text_HRight		= 0x0400,
	WMcCommonFlag_Text_VCenter		= 0x0800,

	WMcCommonFlag_Text_Small		= 0x1000,
	WMcCommonFlag_Text_Bold			= 0x2000,
	WMcCommonFlag_Text_Italic		= 0x4000

};

// notify flags
enum
{
	WMcNotify_Click					= 1,
	WMcNotify_DoubleClick			= 2

};

// draw item states
enum
{
	WMcDrawItemState_None			= 0x0000,
	WMcDrawItemState_Selected		= 0x0001
};

// ======================================================================
// typedefs
// ======================================================================
typedef UUtUns16				WMtWindowType;
typedef struct WMtWindow		WMtWindow;
typedef struct WMtCaret			WMtCaret;

typedef UUtUns32
(*WMtWindowCallback)(
	WMtWindow				*inWindow,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

typedef UUtBool
(*WMtWindowEnumCallback)(
	WMtWindow				*inWindow,
	UUtUns32				inUserData);

// ----------------------------------------------------------------------
typedef struct WMtWindowClass
{
	WMtWindowType			type;
	WMtWindowCallback		callback;
	UUtUns32				private_data_size;

} WMtWindowClass;

// ----------------------------------------------------------------------
typedef struct WMtEvent
{
	WMtWindow				*window;
	WMtMessage				message;
	UUtUns32				param1;
	UUtUns32				param2;

} WMtEvent;

typedef struct WMtKeyboardCommands
{
	UUtUns32				num_commands;
	UUtUns16				commands[WMcMaxKeyboardCommand];

} WMtKeyboardCommands;

// ----------------------------------------------------------------------
typedef struct WMtPosChange
{
	WMtWindow				*insert_after;
	UUtInt16				x;
	UUtInt16				y;
	UUtInt16				width;
	UUtInt16				height;
	UUtUns16				flags;

} WMtPosChange;

// ----------------------------------------------------------------------
typedef struct WMtDrawItem
{
	struct DCtDrawContext	*draw_context;
	WMtWindow				*window;
	UUtUns16				window_id;
	UUtUns16				state;
	UUtUns32				item_id;
	UUtRect					rect;
	const char				*string;
	UUtUns32				data;

} WMtDrawItem;

// ----------------------------------------------------------------------
typedef struct WMtCompareItems
{
	WMtWindow				*window;
	UUtUns16				window_id;

	UUtUns32				item1_index;
	UUtUns32				item1_data;
	UUtUns32				item2_index;
	UUtUns32				item2_data;

} WMtCompareItems;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
WMrWindow_DefaultCallback(
	WMtWindow				*inWindow,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

// ----------------------------------------------------------------------
WMtWindow*
WMrWindow_Activate(
	WMtWindow				*inWindow);

UUtBool
WMrWindow_BringToFront(
	WMtWindow				*inWindow);

UUtBool
WMrWindow_CaptureMouse(
	WMtWindow				*inWindow);

UUtBool
WMrWindow_Delete(
	WMtWindow				*inWindow);

UUtBool
WMrWindow_GetClientRect(
	WMtWindow				*inWindow,
	UUtRect					*outRect);

UUtBool
WMrWindow_GetEnabled(
	WMtWindow				*inWindow);

UUtUns32
WMrWindow_GetFlags(
	WMtWindow				*inWindow);

WMtWindow*
WMrWindow_GetFocus(
	void);

UUtBool
WMrWindow_GetFontInfo(
	WMtWindow				*inWindow,
	TStFontInfo				*outFontInfo);

UUtUns16
WMrWindow_GetID(
	WMtWindow				*inWindow);

UUtUns32
WMrWindow_GetLong(
	WMtWindow				*inWindow,
	UUtInt32				inOffset);

WMtWindow*
WMrWindow_GetOwner(
	WMtWindow				*inWindow);

WMtWindow*
WMrWindow_GetParent(
	WMtWindow				*inWindow);

UUtBool
WMrWindow_GetRect(
	WMtWindow				*inWindow,
	UUtRect					*outRect);

UUtBool
WMrWindow_GetSize(
	WMtWindow				*inWindow,
	UUtInt16				*outWidth,
	UUtInt16				*outHeight);

UUtUns32
WMrWindow_GetStyle(
	WMtWindow				*inWindow);

UUtBool
WMrWindow_GetTitle(
	WMtWindow				*inWindow,
	char					*outTitle,
	UUtUns16				inMaxCharacters);

char*
WMrWindow_GetTitlePtr(
	WMtWindow				*inWindow);

WMtWindowClass*
WMrWindow_GetClass(
	WMtWindow				*inWindow);

UUtBool
WMrWindow_GetVisible(
	WMtWindow				*inWindow);

UUtBool
WMrWindow_IsChild(
	WMtWindow				*inChild,
	WMtWindow				*inParent);

UUtBool
WMrWindow_GlobalToLocal(
	WMtWindow				*inWindow,
	IMtPoint2D				*inGlobal,
	IMtPoint2D				*outLocal);

UUtBool
WMrWindow_LocalToGlobal(
	WMtWindow				*inWindow,
	IMtPoint2D				*inLocal,
	IMtPoint2D				*outGlobal);

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
	UUtUns32				inCreationData);

UUtBool
WMrWindow_PointInWindow(
	WMtWindow				*inWindow,
	IMtPoint2D				*inPoint);

UUtBool
WMrWindow_SetEnabled(
	WMtWindow				*inWindow,
	UUtBool					inEnabled);

UUtBool
WMrWindow_SetFocus(
	WMtWindow				*inWindow);

UUtBool
WMrWindow_SetFontInfo(
	WMtWindow				*inWindow,
	const TStFontInfo		*inFontInfo);

UUtBool
WMrWindow_SetLocation(
	WMtWindow				*inWindow,
	UUtInt16				inX,
	UUtInt16				inY);

UUtUns32
WMrWindow_SetLong(
	WMtWindow				*inWindow,
	UUtInt32				inOffset,
	UUtUns32				inData);

WMtWindow*
WMrWindow_SetParent(
	WMtWindow				*inWindow,
	WMtWindow				*inNewParent);

UUtBool
WMrWindow_SetPosition(
	WMtWindow				*inWindow,
	WMtWindow				*inInsertAfter,
	UUtInt16				inX,
	UUtInt16				inY,
	UUtInt16				inWidth,
	UUtInt16				inHeight,
	UUtUns16				inFlags);

UUtBool
WMrWindow_SetSize(
	WMtWindow				*inWindow,
	UUtInt16				inWidth,
	UUtInt16				inHeight);

UUtBool
WMrWindow_SetTitle(
	WMtWindow				*inWindow,
	const char				*inTitle,
	UUtUns16				inMaxCharacters);

UUtBool
WMrWindow_SetVisible(
	WMtWindow				*inWindow,
	UUtBool					inVisible);

// ----------------------------------------------------------------------
UUtError
WMrWindowClass_Register(
	WMtWindowClass			*inWindowClass);

WMtWindowClass*
WMrWindowClass_GetClassByType(
	WMtWindowType			inType);

// ----------------------------------------------------------------------
UUtBool
WMrCaret_Create(
	WMtWindow				*inWindow,
	UUtInt16				inWidth,
	UUtInt16				inHeight);

void
WMrCaret_Destroy(
	void);

void
WMrCaret_GetPosition(
	IMtPoint2D				*outPosition);

void
WMrCaret_SetPosition(
	UUtInt16				inX,
	UUtInt16				inY);

void
WMrCaret_SetVisible(
	UUtBool					inIsVisible);

// ----------------------------------------------------------------------
UUtBool
WMrTimer_Start(
	UUtUns32				inTimerID,
	UUtUns32				inTimerFrequency,
	WMtWindow				*inWindow);

void
WMrTimer_Stop(
	UUtUns32				inTimerID,
	WMtWindow				*inWindow);

// ----------------------------------------------------------------------
UUtUns32
WMrMessage_Dispatch(
	WMtEvent				*inEvent);

UUtBool
WMrMessage_Get(
	WMtEvent				*outEvent);

UUtBool
WMrMessage_Post(
	WMtWindow				*inWindow,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtUns32
WMrMessage_Send(
	WMtWindow				*inWindow,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
WMrMessage_TranslateKeyCommand(
	WMtEvent				*inEvent,
	WMtKeyboardCommands		*inKeyboardCommands,
	WMtWindow				*inDestWindow);

// ----------------------------------------------------------------------
void
WMrActivate(
	void);

void
WMrDisplay(
	void);

void
WMrDeactivate(
	void);

void
WMrEnumWindows(
	WMtWindowEnumCallback	inEnumCallback,
	UUtUns32				inUserData);

WMtWindow*
WMrGetDesktop(
	void);

WMtWindow*
WMrGetWindowUnderPoint(
	IMtPoint2D				*inPoint);

UUtError
WMrInitialize(
	void);

void
WMrLevel_Unload(
	void);

UUtError
WMrRegisterTemplates(
	void);

void
WMrSetDesktopBackground(
	void					*inBackground);

void
WMrSetResolution(
	UUtInt16				inWidth,
	UUtInt16				inHeight);

UUtError
WMrStartup(
	void);

void
WMrTerminate(
	void);

void
WMrUpdate(
	void);


// ======================================================================
#endif /* BFW_WINDOWMANAGER_H */
