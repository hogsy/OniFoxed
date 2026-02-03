// ======================================================================
// VM_View_Scrollbar.h
// ======================================================================
#ifndef VM_VIEW_SCROLLBAR_H
#define VM_VIEW_SCROLLBAR_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	SBcNotify_None,
	SBcNotify_LineUp,
	SBcNotify_LineDown,
	SBcNotify_PageUp,
	SBcNotify_PageDown,
	SBcNotify_ThumbPosition
};

enum
{
	VMcView_Scrollbar_UpButton		= 15,
	VMcView_Scrollbar_DownButton	= 16,
	VMcView_Scrollbar_Thumb			= 17
};

// ======================================================================
// typedefs
// ======================================================================
#define VMcTemplate_View_Scrollbar			UUm4CharToUns32('V', 'M', 'S', 'B')
typedef tm_template('V', 'M', 'S', 'B', "VM View Scrollbar")
VMtView_Scrollbar
{

	VMtPartSpec				*vertical_scrollbar;
	VMtPartSpec				*vertical_thumb;

	VMtPartSpec				*horizontal_scrollbar;
	VMtPartSpec				*horizontal_thumb;

} VMtView_Scrollbar;

typedef struct VMtView_Scrollbar_PrivateData
{
	UUtBool					is_vertical;
	UUtUns8					mouse_state;
	UUtBool					mouse_down_in_thumb;

	// scrollbar values
	UUtInt32				min;
	UUtInt32				max;
	UUtInt32				current_value;

	// thumb data
	UUtRect					thumb_move_bounds;
	IMtPoint2D				thumb_location;
	UUtUns16				thumb_width;
	UUtUns16				thumb_height;
	UUtUns16				thumb_x_track;
	UUtUns16				thumb_y_track;

	// scrollbar areas
	UUtRect					up_arrow_bounds;
	UUtRect					dn_arrow_bounds;
	UUtRect					page_up_bounds;
	UUtRect					page_dn_bounds;

} VMtView_Scrollbar_PrivateData;

extern TMtPrivateData*	DMgTemplate_Scrollbar_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_Scrollbar_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

UUtInt32
VMrView_Scrollbar_GetPosition(
	VMtView					*inView);

void
VMrView_Scrollbar_GetRange(
	VMtView					*inView,
	UUtInt32				*outMin,
	UUtInt32				*outMax);

UUtError
VMrView_Scrollbar_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);

void
VMrView_Scrollbar_SetPosition(
	VMtView					*inView,
	UUtInt32				inPosition);

void
VMrView_Scrollbar_SetRange(
	VMtView					*inView,
	UUtInt32				inMin,
	UUtInt32				inMax);

// ======================================================================
#endif /* VM_VIEW_SCROLLBAR_H */
