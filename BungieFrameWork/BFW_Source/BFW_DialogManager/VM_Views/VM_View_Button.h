// ======================================================================
// VM_View_Button.h
// ======================================================================
#ifndef VM_VIEW_BUTTON_H
#define VM_VIEW_BUTTON_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Image.h"
#include "BFW_ViewManager.h"

// ======================================================================
// defines
// ======================================================================
#define VMcView_Button_DefaultAnimationRate		20

// ======================================================================
// typedefs
// ======================================================================
#define VMcTemplate_View_Button			UUm4CharToUns32('V', 'M', '_', 'B')
typedef tm_template('V', 'M', '_', 'B', "VM View Button")
VMtView_Button
{

	// button part specifications
	VMtPartSpec			*idle;
	VMtPartSpec			*mouse_over;
	VMtPartSpec			*pressed;

	IMtPoint2D			text_location_offset;	// offset from top left corner of button
												// for the text to be drawn at

	UUtUns16			flags;
	UUtUns16			animation_rate;			// number of ticks between texture changes

	char				title[32];

} VMtView_Button;

typedef struct VMtView_Button_PrivateData
{
	UUtUns8				mouse_state;

	// vars for title text
	tm_templateref		texture_ref;
	UUtInt16			string_texture_width;
	UUtInt16			string_texture_height;
	IMtPoint2D			text_location;

	// time in ticks of next animation
	UUtUns32			next_animation;

} VMtView_Button_PrivateData;

extern TMtPrivateData*	DMgTemplate_Button_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_Button_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

UUtError
VMrView_Button_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);

// ======================================================================
#endif /* VM_VIEW_BUTTON_H */
