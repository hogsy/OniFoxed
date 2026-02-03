// ======================================================================
// VM_View_Slider.h
// ======================================================================
#ifndef VM_VIEW_SLIDER_H
#define VM_VIEW_SLIDER_H

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
	VMcView_Slider_LeftButton	= 10,
	VMcView_Slider_RightButton	= 11,
	VMcView_Slider_Thumb		= 12,
	VMcView_Slider_Middle		= 13
};

// ======================================================================
// typedefs
// ======================================================================
#define VMcTemplate_View_Slider			UUm4CharToUns32('V', 'M', '_', 'S')
typedef tm_template('V', 'M', '_', 'S', "VM View Slider")
VMtView_Slider
{

	UUtUns16				flags;
	UUtUns16				reserved;

	VMtPartSpec				*outline;

	char					title[32];

} VMtView_Slider;

typedef struct VMtView_Slider_PrivateData
{
	// title texture
	tm_templateref			title_texture_ref;
	IMtPoint2D				title_location;
	UUtUns16				title_texture_width;
	UUtUns16				title_texture_height;

	// scrollbar
	VMtView					*scrollbar;

} VMtView_Slider_PrivateData;

extern TMtPrivateData*	DMgTemplate_Slider_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_Slider_Callback(
	VMtView					*inView,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtInt32
VMrView_Slider_GetPosition(
	VMtView					*inView);

UUtError
VMrView_Slider_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);

void
VMrView_Slider_SetPosition(
	VMtView					*inView,
	UUtInt32				inPosition);

void
VMrView_Slider_SetRange(
	VMtView					*inView,
	UUtInt32				inMin,
	UUtInt32				inMax);

// ======================================================================
#endif /* VM_VIEW_SLIDER_H */
