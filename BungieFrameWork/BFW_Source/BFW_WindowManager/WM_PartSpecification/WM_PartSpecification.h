// ======================================================================
// WM_PartSpecification.h
// ======================================================================
#ifndef BFW_PARTSPECIFICATION_H
#define BFW_PARTSPECIFICATION_H

#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	PScPartSpecType_BackgroundColor_None		= 0x0000,
	PScPartSpecType_BackgroundColor_Black		= 0x0001,
	PScPartSpecType_BackgroundColor_White		= 0x0002,
	PScPartSpecType_BackgroundColor_Red			= 0x0003,
	PScPartSpecType_BackgroundColor_Green		= 0x0004,
	PScPartSpecType_BackgroundColor_Blue		= 0x0005,
	
	PScPartSpecType_BorderType_None				= 0x0000,
	PScPartSpecType_BorderType_Square			= 0x0010,
	PScPartSpecType_BorderType_Curved			= 0x0020,
	PScPartSpecType_BorderType_Bevel			= 0x0030,
	
	PScPartSpecType_BorderColor_None			= 0x0000,
	PScPartSpecType_BorderColor_Black			= 0x0100,
	PScPartSpecType_BorderColor_White			= 0x0200,
	PScPartSpecType_BorderColor_Red				= 0x0300,
	PScPartSpecType_BorderColor_Green			= 0x0400,
	PScPartSpecType_BorderColor_Blue			= 0x0500
};

enum
{
	PScPart_None			= 0x0000,
	PScPart_LeftTop			= 0x0001,
	PScPart_LeftMiddle		= 0x0002,
	PScPart_LeftBottom		= 0x0004,
	PScPart_MiddleTop		= 0x0008,
	PScPart_MiddleMiddle	= 0x0010,
	PScPart_MiddleBottom	= 0x0020,
	PScPart_RightTop		= 0x0040,
	PScPart_RightMiddle		= 0x0080,
	PScPart_RightBottom		= 0x0100,
	PScPart_LeftColumn		= PScPart_LeftTop | PScPart_LeftMiddle | PScPart_LeftBottom,
	PScPart_MiddleColumn	= PScPart_MiddleTop | PScPart_MiddleMiddle | PScPart_MiddleBottom,
	PScPart_RightColumn		= PScPart_RightTop | PScPart_RightMiddle | PScPart_RightBottom,
	PScPart_TopRow			= PScPart_LeftTop | PScPart_MiddleTop | PScPart_RightTop,
	PScPart_MiddleRow		= PScPart_LeftMiddle | PScPart_MiddleMiddle | PScPart_RightMiddle,
	PScPart_BottomRow		= PScPart_LeftBottom | PScPart_MiddleBottom | PScPart_RightBottom,
	PScPart_All				= 0xFFFF
	
};

enum
{
	PScColumn_Left			= 0,
	PScColumn_Middle		= 1,
	PScColumn_Right			= 2,

	PScRow_Top				= 0,
	PScRow_Middle			= 1,
	PScRow_Bottom			= 2
};

// ======================================================================
// typedefs
// ======================================================================
typedef UUtUns16			PStPartSpecType;

// ----------------------------------------------------------------------
#define PScTemplate_PartSpecification		UUm4CharToUns32('P', 'S', 'p', 'c')
typedef tm_template('P', 'S', 'p', 'c', "Part Specification")
PStPartSpec
{
	
	IMtPoint2D 							part_matrix_tl[3][3];	// [column][row]
	IMtPoint2D 							part_matrix_br[3][3];	// [column][row]
	tm_templateref						texture;
	
} PStPartSpec;

typedef struct PStPartSpec_PrivateData
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
	
	UUtInt16				l_width;
	UUtInt16				r_width;
	UUtInt16				t_height;
	UUtInt16				b_height;

} PStPartSpec_PrivateData;

extern TMtPrivateData*	PSgTemplate_PartSpec_PrivateData;

// ----------------------------------------------------------------------
typedef tm_struct PStPartSpecDesc
{
	UUtUns32				partspec_type;
	tm_templateref			partspec;
	
} PStPartSpecDesc;

#define PScTemplate_PartSpecList	UUm4CharToUns32('P', 'S', 'p', 'L')
typedef tm_template('P', 'S', 'p', 'L', "Part Specification List")
PStPartSpecList
{
	tm_pad							pad[20];
	
	tm_varindex UUtUns32			num_partspec_descs;
	tm_vararray PStPartSpecDesc		partspec_descs[1];
	
} PStPartSpecList;

// ----------------------------------------------------------------------
#define PScTemplate_PartSpecUI		UUm4CharToUns32('P', 'S', 'U', 'I')
typedef tm_template('P', 'S', 'U', 'I', "Part Specifications UI")
PStPastSpecUI
{
	// basic window
	PStPartSpec				*background;
	PStPartSpec				*border;
	PStPartSpec				*title;
	PStPartSpec				*grow;
	
	PStPartSpec				*close_idle;
	PStPartSpec				*close_pressed;
	
	PStPartSpec				*zoom_idle;
	PStPartSpec				*zoom_pressed;
	
	PStPartSpec				*flatten_idle;
	PStPartSpec				*flatten_pressed;
	
	// text
	PStPartSpec				*text_caret;
	
	// box
	PStPartSpec				*outline;
	
	// button
	PStPartSpec				*button_off;
	PStPartSpec				*default_button;
	PStPartSpec				*button_on;
	
	// checkbox
	PStPartSpec				*checkbox_on;
	PStPartSpec				*checkbox_off;
	
	// editfield
	PStPartSpec				*editfield;
	PStPartSpec				*ef_hasfocus;
	
	// listbox uses editfield
	
	// menu uses background and outline
	PStPartSpec				*hilite;
	PStPartSpec				*divider;
	PStPartSpec				*check;
	
	// menubar uses background and hilite
	
	// popup menu
	PStPartSpec				*popup_menu;
	
	// progress bar
	PStPartSpec				*progressbar_track;
	PStPartSpec				*progressbar_fill;
	
	// radio button
	PStPartSpec				*radiobutton_on;
	PStPartSpec				*radiobutton_off;
	
	// scrollbar
	PStPartSpec				*up_arrow;
	PStPartSpec				*up_arrow_pressed;
	PStPartSpec				*dn_arrow;
	PStPartSpec				*dn_arrow_pressed;
	PStPartSpec				*sb_v_track;
	
	PStPartSpec				*lt_arrow;
	PStPartSpec				*lt_arrow_pressed;
	PStPartSpec				*rt_arrow;
	PStPartSpec				*rt_arrow_pressed;
	PStPartSpec				*sb_h_track;
	
	PStPartSpec				*sb_thumb;
	
	// slider
	PStPartSpec				*sl_thumb;
	PStPartSpec				*sl_track;
	
	// Tab
	PStPartSpec				*tab_active;
	PStPartSpec				*tab_inactive;
	
	// files & folders
	PStPartSpec				*file;
	PStPartSpec				*folder;
		
} PStPartSpecUI;

// ======================================================================
// prototypes
// ======================================================================
void
PSrPartSpec_Draw(
	PStPartSpec				*inPartSpec,
	UUtUns16				inFlags,
	M3tPointScreen			*inLocation,
	UUtInt16				inWidth,
	UUtInt16				inHeight,
	UUtUns16				inAlpha);

void
PSrPartSpec_GetSize(
	PStPartSpec				*inPartSpec,
	UUtUns16				inPart,
	UUtInt16				*outWidth,
	UUtInt16				*outHeight);
	
PStPartSpec*
PSrPartSpec_LoadByType(
	PStPartSpecType			inPartSpecType);
	
// ----------------------------------------------------------------------
PStPartSpecUI*
PSrPartSpecUI_GetActive(
	void);
	
PStPartSpecUI*
PSrPartSpecUI_GetByName(
	const char				*inName);
	
void
PSrPartSpecUI_SetActive(
	PStPartSpecUI			*inPartSpecUI);
	
// ----------------------------------------------------------------------
UUtError
PSrInitialize(
	void);

UUtError
PSrRegisterTemplates(
	void);

// ======================================================================
#endif /* BFW_PARTSPECIFICATION_H */
