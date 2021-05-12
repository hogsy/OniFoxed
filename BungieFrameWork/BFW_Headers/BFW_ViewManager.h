#pragma once

// ======================================================================
// BFW_ViewManager.h
// ======================================================================
#ifndef BFW_VIEWMANAGER_H
#define BFW_VIEWMANAGER_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Image.h"
#include "BFW_Motoko.h"

// ======================================================================
// defines
// ======================================================================
#define VMcColor_Background			IMcShade_None
#define VMcColor_Outline			IMcShade_White
#define VMcColor_Text				IMcShade_White

#define VMcColorsTexture_BackgroundColorX		0
#define VMcColorsTexture_BackgroundColorY		0

#define VMcColorsTexture_OutlineColorX			1
#define VMcColorsTexture_OutlineColorY			0

#define VMcColorsTexture_TextColorX				0
#define VMcColorsTexture_TextColorY				1

#define VMcControl_PixelType					IMcPixelType_ARGB4444

#define VMcMaxTitleLength						32

// ======================================================================
// enums
// ======================================================================
typedef enum VMtMessage
{
	// View Manager Messages
	VMcMessage_None,
	
	VMcMessage_Create,
	VMcMessage_Destroy,
	
	VMcMessage_Command,
	VMcMessage_Timer,
	
	VMcMessage_KeyDown,
	VMcMessage_KeyUp,
	
	VMcMessage_MouseMove,
	VMcMessage_MouseLeave,

	VMcMessage_LMouseDown,
	VMcMessage_LMouseUp,
	VMcMessage_LMouseDblClck,
	
	VMcMessage_MMouseDown,
	VMcMessage_MMouseUp,
	VMcMessage_MMouseDblClck,

	VMcMessage_RMouseDown,
	VMcMessage_RMouseUp,
	VMcMessage_RMouseDblClck,
	
	VMcMessage_SetFocus,
	
	VMcMessage_Paint,
	VMcMessage_Update,
	
	VMcMessage_GetValue,
	VMcMessage_SetValue,
	
	VMcMessage_Location,
	VMcMessage_Size,
	
	// Dialog Manager Messages
	DMcMessage_InitDialog,
	
	// List Box Messages
	LBcMessage_AddString,
	LBcMessage_ReplaceString,
	LBcMessage_DeleteString,
	LBcMessage_InsertString,
	LBcMessage_GetNumLines,
	LBcMessage_GetCurrentSelection,
	LBcMessage_GetText,
	LBcMessage_GetTextLength,
	LBcMessage_Reset,
	LBcMessage_SetSelection,
	LBcMessage_SetLineColor,
	
	// Scroll Bar Messages
	SBcMessage_HorizontalScroll,
	SBcMessage_VerticalScroll,
	
	// Tab Messages
	TbcMessage_InitTab
	
} VMtMessage;

enum
{
	VMcNotify_Click				= 1,
	VMcNotify_DoubleClick		= 2,
	
	RBcNotify_Off				= 30,	// radio button went off
	RBcNotify_On				= 31,	// radio button went on
	
	LBcNotify_SelectionChanged	= 40	// List Box Selection Changed
};

enum
{
	VMcViewFlag_None				= 0x0000,
	
	VMcViewFlag_Visible				= 0x0001,
	VMcViewFlag_Enabled				= 0x0002,
	VMcViewFlag_Active				= 0x0004,
	VMcViewFlag_CanFocus			= 0x0008
};

enum
{
	VMcAnimationFlag_None			= 0x0000,
	VMcAnimationFlag_Random			= 0x0010,
	VMcAnimationFlag_PingPong		= 0x0020
};

enum
{
	VMcAnimationDirection_Neg		= UUcFalse,
	VMcAnimationDirection_Pos		= UUcTrue
};

enum
{
	VMcCommonFlag_None				= 0x0000,
	
	VMcCommonFlag_Text_HLeft		= 0x0100,
	VMcCommonFlag_Text_HCenter		= 0x0200,
	VMcCommonFlag_Text_HRight		= 0x0400,
	VMcCommonFlag_Text_VCenter		= 0x0800,
	
	VMcCommonFlag_Text_Small		= 0x1000,
	VMcCommonFlag_Text_Bold			= 0x2000,
	VMcCommonFlag_Text_Italic		= 0x4000
	
};

enum
{
	VMcMouseState_None				= 0x0000,
	VMcMouseState_MouseOver			= 0x0001,
	VMcMouseState_MouseDown			= 0x0002
};

enum
{
	VMcViewType_None,
	VMcViewType_Box,
	VMcViewType_Button,
	VMcViewType_CheckBox,
	VMcViewType_Dialog,
	VMcViewType_EditField,
	VMcViewType_ListBox,
	VMcViewType_Picture,
	VMcViewType_RadioButton,
	VMcViewType_RadioGroup,
	VMcViewType_Scrollbar,
	VMcViewType_Slider,
	VMcViewType_Tab,
	VMcViewType_TabGroup,
	VMcViewType_Text
};

// ======================================================================
// typedef
// ======================================================================
typedef struct VMtViewTimer
{
	struct VMtViewTimer	*next;
	UUtUns16			timer_id;
	UUtUns32			timer_frequency;	// in ticks
	UUtUns32			next_update;		// in ticks
	
} VMtViewTimer;

// ----------------------------------------------------------------------
typedef tm_struct
VMtViewRef
{
	tm_templateref				view_ref;
	
} VMtViewRef;

#define VMcTemplate_View				UUm4CharToUns32('V', 'M', '_', 'V')
typedef tm_template('V', 'M', '_', 'V', "VM View")
VMtView
{
	// implied 8 bytes here
	//tm_pad					pad[32];
	
	UUtUns16				id;
	UUtUns16				type;
	UUtUns32				flags;
	
	IMtPoint2D				location;
	UUtInt16				width;
	UUtInt16				height;
	
	tm_templateref			view_data;
	
	tm_varindex	UUtUns32	num_child_views;
	tm_vararray	VMtViewRef	child_views[1];
	
} VMtView;

typedef UUtUns32
(*VMtViewCallback)(
	VMtView					*inView,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

typedef struct VMtView_PrivateData
{
	VMtView					*parent;
	VMtViewCallback			callback;
	VMtViewTimer			*timers;
	UUtUns16				layer;
	
} VMtView_PrivateData;

// ----------------------------------------------------------------------
typedef enum
VMtTextureGroup
{
	VMcNoEffectsTexture,
	VMcIdleTextures,
	VMcIdleToMouseOverTextures,
	VMcMouseOverTextures,
	VMcMouseOverToClickedTextures,
	VMcClickedTextures
	
} VMtTextureGroup;

typedef tm_struct VMtTexture
{
	tm_templateref			texture_ref;
	
} VMtTexture;

#define VMcTemplate_TextureList		UUm4CharToUns32('V', 'M', 'T', 'L')
typedef tm_template('V', 'M', 'T', 'L', "VM Texture List")
VMtTextureList
{
	tm_pad								pad0[22];
	
	tm_varindex	UUtUns16				num_textures;
	tm_vararray	VMtTexture				textures[1];
	
} VMtTextureList;

// ----------------------------------------------------------------------
enum
{
	VMcPart_None			= 0x0000,
	VMcPart_LeftTop			= 0x0001,
	VMcPart_LeftMiddle		= 0x0002,
	VMcPart_LeftBottom		= 0x0004,
	VMcPart_MiddleTop		= 0x0008,
	VMcPart_MiddleMiddle	= 0x0010,
	VMcPart_MiddleBottom	= 0x0020,
	VMcPart_RightTop		= 0x0040,
	VMcPart_RightMiddle		= 0x0080,
	VMcPart_RightBottom		= 0x0100,
	VMcPart_LeftColumn		= VMcPart_LeftTop | VMcPart_LeftMiddle | VMcPart_LeftBottom,
	VMcPart_MiddleColumn	= VMcPart_MiddleTop | VMcPart_MiddleMiddle | VMcPart_MiddleBottom,
	VMcPart_RightColumn		= VMcPart_RightTop | VMcPart_RightMiddle | VMcPart_RightBottom,
	VMcPart_TopRow			= VMcPart_LeftTop | VMcPart_MiddleTop | VMcPart_RightTop,
	VMcPart_MiddleRow		= VMcPart_LeftMiddle | VMcPart_MiddleMiddle | VMcPart_RightMiddle,
	VMcPart_BottomRow		= VMcPart_LeftBottom | VMcPart_MiddleBottom | VMcPart_RightBottom,
	VMcPart_All				= 0xFFFF
	
};

enum
{
	VMcColumn_Left			= 0,
	VMcColumn_Middle		= 1,
	VMcColumn_Right			= 2,

	VMcRow_Top				= 0,
	VMcRow_Middle			= 1,
	VMcRow_Bottom			= 2
};

#define VMcTemplate_PartSpecification		UUm4CharToUns32('V', 'M', 'P', 'S')
typedef tm_template('V', 'M', 'P', 'S', "VM Part Specification")
VMtPartSpec
{
	
	IMtPoint2D 							part_matrix_tl[3][3];	// [column][row]
	IMtPoint2D 							part_matrix_br[3][3];	// [column][row]
	tm_templateref						texture;
	
} VMtPartSpec;

typedef struct VMtPartSpec_PrivateData
{
	M3tTextureCoord			lt[4];	// [0] = tl, [1] = tr, [2] = bl. [3] = br
	M3tTextureCoord			lm[4];
	M3tTextureCoord			lb[4];

	M3tTextureCoord			mt[4];
	M3tTextureCoord			mm[4];
	M3tTextureCoord			mb[4];
	
	M3tTextureCoord			rt[4];
	M3tTextureCoord			rm[4];
	M3tTextureCoord			rb[4];

} VMtPartSpec_PrivateData;

extern TMtPrivateData*	DMgTemplate_View_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_DefaultCallback(
	VMtView					*inView,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

// ----------------------------------------------------------------------
void
VMrView_Draw(
	VMtView					*inView,
	M3tPointScreen			*inDestination);
	
VMtView*
VMrView_GetNextFocusableView(
	VMtView					*inView,
	VMtView					*inCurrentFocus);
	
VMtView*
VMrView_GetParent(
	VMtView					*inView);

UUtUns32
VMrView_GetValue(
	VMtView					*inView);
	
VMtView*
VMrView_GetViewUnderPoint(
	VMtView					*inView,
	IMtPoint2D				*inPoint,
	UUtUns16				inFlags);
	
UUtError
VMrView_Load(
	VMtView					*inView,
	VMtView					*inParent);
	
void
VMrView_PointGlobalToView(
	const VMtView			*inView,
	const IMtPoint2D		*inPoint,
	IMtPoint2D				*outPoint);
	
UUtError
VMrView_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);
	
UUtUns32
VMrView_SendMessage(
	VMtView					*inView,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

void
VMrView_SetActive(
	VMtView					*inView,
	UUtBool					inIsActive);
	
void
VMrView_SetCallback(
	VMtView					*inView,
	VMtViewCallback			inCallback);
	
void
VMrView_SetEnabled(
	VMtView					*inView,
	UUtBool					inIsEnabled);
	
void
VMrView_SetFocus(
	VMtView					*inView,
	UUtBool					inIsFocused);
	
void
VMrView_SetLocation(
	VMtView					*inView,
	IMtPoint2D				*inLocation);
	
void
VMrView_SetSize(
	VMtView					*inView,
	UUtUns16				inWidth,
	UUtUns16				inHeight);
	
void
VMrView_SetValue(
	VMtView					*inView,
	UUtUns32				inValue);
	
void
VMrView_SetVisible(
	VMtView					*inView,
	UUtBool					inIsVisible);
	
UUtError
VMrView_Timer_Start(
	VMtView					*inView,
	UUtUns16				inTimerID,
	UUtUns32				inTimerFrequency);

void
VMrView_Timer_Stop(
	VMtView					*inView,
	UUtUns16				inTimerID);

void
VMrView_Timer_Update(
	VMtView					*inView);

void
VMrView_Unload(
	VMtView					*inView);

// ----------------------------------------------------------------------
UUtError
VMrInitialize(
	void);
	
void
VMrTerminate(
	void);

// ======================================================================
#endif /* BFW_VIEWMANAGER_H */