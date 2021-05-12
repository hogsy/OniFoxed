// ======================================================================
// VM_View_Slider.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_DialogManager.h"
#include "BFW_ViewUtilities.h"
#include "BFW_LocalInput.h"
#include "BFW_TextSystem.h"

#include "VM_View_Slider.h"
#include "VM_View_Scrollbar.h"


// ======================================================================
// defines
// ======================================================================
#define VMcView_Slider_ThumbWidth		4
#define VMcView_Slider_ThumbHeight		8

#define VMcView_Slider_BLCurve			0
#define VMcView_Slider_BRCurve			1

#define VMcView_Slider_ButtonTimer				2
#define VMcView_Slider_ButtonTimerFrequency		10

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
VMiView_Slider_HandleHorizontalScroll(
	VMtView						*inView,
	VMtView_Slider				*inSlider,
	VMtView_Slider_PrivateData	*inPrivateData,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtInt32					scroll_increment;
	UUtInt32					scroll_range;
	UUtInt32					min;
	UUtInt32					max;
	UUtInt32					current_pos;
	
	// get the scroll range
	current_pos = VMrView_Scrollbar_GetPosition(inPrivateData->scrollbar);
	VMrView_Scrollbar_GetRange(inPrivateData->scrollbar, &min, &max);
	scroll_range = max - min;
	
	// interpret the parameters of the message
	switch (UUmLowWord(inParam1))
	{
		case SBcNotify_LineUp:
			scroll_increment = -1;
		break;
		
		case SBcNotify_LineDown:
			scroll_increment = 1;
		break;
		
		case SBcNotify_PageUp:
			scroll_increment = UUmMin(-1, -scroll_range / 10);
		break;
		
		case SBcNotify_PageDown:
			scroll_increment = UUmMax(1, scroll_range / 10);
		break;
		
		case SBcNotify_ThumbPosition:
			scroll_increment = inParam2 - current_pos;
		break;
		
		default:
			scroll_increment = 0;
		break;
	}
	
	// adjust the the thumb position and redraw the texture
	if (scroll_increment != 0)
	{
		VMtView_PrivateData			*view_private_data;
		
		view_private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
		UUmAssert(view_private_data);
		
		current_pos += scroll_increment;
		VMrView_Scrollbar_SetPosition(inPrivateData->scrollbar, current_pos);
		
		// tell the parent about the new slider value
		VMrView_SendMessage(
			view_private_data->parent,
			VMcMessage_Command,
			(UUtUns32)UUmMakeLong(inView->id, SLcNotify_NewPosition),
			(UUtUns32)VMrView_Scrollbar_GetPosition(inPrivateData->scrollbar));
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
VMiView_Slider_Create(
	VMtView						*inView,
	VMtView_Slider				*inSlider,
	VMtView_Slider_PrivateData	*inPrivateData)
{
	UUtError					error;
	
	// ------------------------------
	// create title texture
	// ------------------------------
	if ((inPrivateData->title_texture_ref == NULL) &&
		(TSrString_GetLength(inSlider->title) > 0))
	{
		UUtRect					string_bounds;
		
		error =
			VUrCreate_StringTexture(
				inSlider->title,
				inSlider->flags,
				&inPrivateData->title_texture_ref,
				&string_bounds);
		UUmError_ReturnOnErrorMsg(error, "Unable to create title texture for the button");
		
		// get the title_texture_width and title_texture_height
		inPrivateData->title_texture_width = string_bounds.right;
		inPrivateData->title_texture_height = string_bounds.bottom;
		
		inPrivateData->title_location.x = 4;
		inPrivateData->title_location.y = 2;
	}
	
	// ------------------------------
	// initialize the scrollbar
	// ------------------------------
	if (inView->num_child_views > 0)
	{
		inPrivateData->scrollbar = (VMtView*)inView->child_views[0].view_ref;
		if (inPrivateData->scrollbar->type == VMcViewType_Scrollbar)
		{
			IMtPoint2D			new_location;
			UUtUns16			new_width;
			
			if (inSlider->outline)
			{
				new_location.x = 4;
				new_location.y = inView->height - inPrivateData->scrollbar->height - 4;
				new_width = inView->width - 8;
			}
			else
			{
				new_location.x = 0;
				new_location.y = inView->height - inPrivateData->scrollbar->height;
				new_width = inView->width;
			}
			
			VMrView_SetLocation(inPrivateData->scrollbar, &new_location);
			VMrView_SetSize(inPrivateData->scrollbar, new_width, inPrivateData->scrollbar->height);

			VMrView_Scrollbar_SetRange(inPrivateData->scrollbar, 0, 100);
			VMrView_Scrollbar_SetPosition(inPrivateData->scrollbar, 0);
		}
		else
		{
			inPrivateData->scrollbar = NULL;
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
VMiView_Slider_Paint(
	VMtView						*inView,
	VMtView_Slider				*inSlider,
	VMtView_Slider_PrivateData	*inPrivateData,
	M3tPointScreen				*inDestination)
{
	UUtUns16					alpha;
	M3tPointScreen				dest;

	// set the alpha
	if (inView->flags & VMcViewFlag_Enabled)
		alpha = VUcAlpha_Enabled;
	else
		alpha = VUcAlpha_Disabled;
	
	// draw the outline
	if (inSlider->outline)
	{
		VUrDrawPartSpecification(
			inSlider->outline,
			VMcPart_All,
			inDestination,
			inView->width,
			inView->height,
			alpha);
	}
	
	// draw the title
	if (inPrivateData->title_texture_ref)
	{
		dest = *inDestination;
		dest.x += (float)inPrivateData->title_location.x;
		dest.y += (float)inPrivateData->title_location.y;
		
		VUrDrawTextureRef(
			inPrivateData->title_texture_ref,
			&dest,
			inPrivateData->title_texture_width,
			inPrivateData->title_texture_height,
			alpha);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
VMrView_Slider_Callback(
	VMtView					*inView,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	VMtView_Slider				*slider;
	VMtView_Slider_PrivateData	*private_data;
	
	// get the data
	slider = (VMtView_Slider*)inView->view_data;
	private_data = (VMtView_Slider_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Slider_PrivateData, slider);
	UUmAssert(private_data);

	switch (inMessage)
	{
		case VMcMessage_Create:
			VMiView_Slider_Create(inView, slider, private_data);
		return 0;
		
		case VMcMessage_Paint:
			// draw the slider
			VMiView_Slider_Paint(
				inView,
				slider,
				private_data,
				(M3tPointScreen*)inParam2);
		break;

		case SBcMessage_HorizontalScroll:
			VMiView_Slider_HandleHorizontalScroll(
				inView,
				slider,
				private_data,
				inParam1,
				inParam2);
		return 0;
	}
	
	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtInt32
VMrView_Slider_GetPosition(
	VMtView					*inView)
{
	VMtView_Slider				*slider;
	VMtView_Slider_PrivateData	*private_data;
	
	UUmAssert(inView);
	
	// get a pointer to the data
	slider = (VMtView_Slider*)inView->view_data;
	private_data = (VMtView_Slider_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Slider_PrivateData, slider);
	UUmAssert(private_data);
	UUmAssert(private_data->scrollbar);
	
	// set the new range values
	return VMrView_Scrollbar_GetPosition(private_data->scrollbar);
}

// ----------------------------------------------------------------------
UUtError
VMrView_Slider_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	VMtView_Slider				*slider;
	VMtView_Slider_PrivateData	*private_data;
	
	// get a pointer to the button data
	slider = (VMtView_Slider*)inInstancePtr;
	private_data = (VMtView_Slider_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	switch (inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initalize the private data
			private_data->title_texture_ref	= NULL;
			private_data->scrollbar			= NULL;
		break;
		
		case TMcTemplateProcMessage_DisposePreProcess:
		break;
		
		case TMcTemplateProcMessage_Update:
		break;
		
		default:
			UUmAssert(!"Illegal message");
		break;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
VMrView_Slider_SetPosition(
	VMtView					*inView,
	UUtInt32				inPosition)
{
	VMtView_Slider				*slider;
	VMtView_Slider_PrivateData	*private_data;
	
	UUmAssert(inView);
	
	// get a pointer to the data
	slider = (VMtView_Slider*)inView->view_data;
	private_data = (VMtView_Slider_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Slider_PrivateData, slider);
	UUmAssert(private_data);
	UUmAssert(private_data->scrollbar);

	// set the new position
	VMrView_Scrollbar_SetPosition(private_data->scrollbar, inPosition);
}

// ----------------------------------------------------------------------
void
VMrView_Slider_SetRange(
	VMtView					*inView,
	UUtInt32				inMin,
	UUtInt32				inMax)
{
	VMtView_Slider				*slider;
	VMtView_Slider_PrivateData	*private_data;
	
	UUmAssert(inView);
	
	// get a pointer to the data
	slider = (VMtView_Slider*)inView->view_data;
	private_data = (VMtView_Slider_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Slider_PrivateData, slider);
	UUmAssert(private_data);
	UUmAssert(private_data->scrollbar);
	
	// set the new range values
	VMrView_Scrollbar_SetRange(private_data->scrollbar, inMin, inMax);
}
