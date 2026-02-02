// ======================================================================
// VM_View_Button.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TextSystem.h"
#include "BFW_DialogManager.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"
#include "BFW_LocalInput.h"

#include "VM_View_Button.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
VMiView_Button_Create(
	VMtView							*inView,
	VMtView_Button					*inButton,
	VMtView_Button_PrivateData		*inPrivateData)
{
	UUtError						error;
	UUtInt16						offset_x;
	UUtInt16						offset_y;

	if ((inPrivateData->texture_ref == NULL) &&
		(TSrString_GetLength(inButton->title) > 0))
	{
		UUtRect						string_bounds;

		// create texture_ref
		error =
			VUrCreate_StringTexture(
				inButton->title,
				inButton->flags,
				&inPrivateData->texture_ref,
				&string_bounds);
		UUmError_ReturnOnErrorMsg(error, "Unable to create title texture for the button");

		// get the string_texture_width and string_texture_height
		inPrivateData->string_texture_width = string_bounds.right;
		inPrivateData->string_texture_height = string_bounds.bottom;
	}

	// calculate offset_x and offset_y
	offset_x = offset_y = 0;
	if (inButton->flags & VMcCommonFlag_Text_HRight)
		offset_x = inView->width - inPrivateData->string_texture_width;
	else if (inButton->flags & VMcCommonFlag_Text_HCenter)
		offset_x = ((inView->width - inPrivateData->string_texture_width) >> 1);
	if (inButton->flags & VMcCommonFlag_Text_VCenter)
		offset_y = ((inView->height - inPrivateData->string_texture_height) >> 1);

	inPrivateData->text_location.x = inButton->text_location_offset.x + offset_x;
	inPrivateData->text_location.y = inButton->text_location_offset.y + offset_y;

	// initialize
	inPrivateData->mouse_state	= VMcMouseState_None;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
VMiView_Button_Paint(
	VMtView							*inView,
	VMtView_Button					*inButton,
	VMtView_Button_PrivateData		*inPrivateData,
	M3tPointScreen					*inDestination)
{
	UUtUns16						alpha;
	VMtPartSpec						*partspec;
	UUtUns8							pressed;

	// set the alpha
	if (inView->flags & VMcViewFlag_Enabled)
		alpha = VUcAlpha_Enabled;
	else
		alpha = VUcAlpha_Disabled;

	// draw the idle button
	partspec = NULL;

	pressed = VMcMouseState_MouseDown | VMcMouseState_MouseOver;
	if (((inPrivateData->mouse_state & pressed) == pressed) &&
		(inButton->pressed))
	{
		partspec = inButton->pressed;
	}
	else
	if ((inPrivateData->mouse_state & VMcMouseState_MouseOver) &&
		(inButton->mouse_over))
	{
		partspec = inButton->mouse_over;
	}
	else
	if (inButton->idle)
	{
		partspec = inButton->idle;
	}

	if (partspec)
	{
		VUrDrawPartSpecification(
			partspec,
			VMcPart_All,
			inDestination,
			inView->width,
			inView->height,
			alpha);
	}

	// draw the string texture
	if (inPrivateData->texture_ref)
	{
		M3tPointScreen				text_dest;

		// set the string destination
		text_dest = *inDestination;
		text_dest.x += (float)inPrivateData->text_location.x;
		text_dest.y += (float)inPrivateData->text_location.y;

		VUrDrawTextureRef(
			inPrivateData->texture_ref,
			&text_dest,
			inPrivateData->string_texture_width,
			inPrivateData->string_texture_height,
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
VMrView_Button_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_Button		*button;
	VMtView_Button_PrivateData	*private_data;
	VMtView_PrivateData	*view_private_data;

	// get a pointer to the view's private data
	view_private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(view_private_data);

	// get a pointer to the button data
	button = (VMtView_Button*)inView->view_data;
	private_data = (VMtView_Button_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Button_PrivateData, button);
	UUmAssert(private_data);

	switch (inMessage)
	{
		case VMcMessage_Create:
			VMiView_Button_Create(inView, button, private_data);
		return 0;

		case VMcMessage_MouseMove:
		{
			IMtPoint2D			global_point;
			IMtPoint2D			local_point;
			UUtRect				bounds;

			global_point.x = (UUtInt16)UUmHighWord(inParam1);
			global_point.y = (UUtInt16)UUmLowWord(inParam1);

			VMrView_PointGlobalToView(inView, &global_point, &local_point);

			bounds.left = 0;
			bounds.top = 0;
			bounds.right = inView->width;
			bounds.bottom = inView->height;

			if (IMrRect_PointIn(&bounds, &local_point))
			{
				private_data->mouse_state = VMcMouseState_MouseOver;
			}
			else
			{
				private_data->mouse_state &= ~VMcMouseState_MouseOver;
			}

			if (inParam2 & LIcMouseState_LButtonDown)
				private_data->mouse_state |= VMcMouseState_MouseDown;
		}
		return 0;

		case VMcMessage_MouseLeave:
			private_data->mouse_state &= ~VMcMouseState_MouseOver;
		return 0;

		case VMcMessage_LMouseDown:
			DMrDialog_SetMouseFocusView(NULL, inView);
			private_data->mouse_state |= VMcMouseState_MouseDown;
		return 0;

		case VMcMessage_LMouseUp:
		{
			UUtRect					bounds_rect;
			IMtPoint2D				global_point;
			IMtPoint2D				view_point;

			DMrDialog_ReleaseMouseFocusView(NULL, inView);

			// initialize the bounds_rect
			bounds_rect.left = 0;
			bounds_rect.top = 0;
			bounds_rect.right = inView->width;
			bounds_rect.bottom = inView->height;

			// initialize the point
			global_point.x = (UUtInt16)UUmHighWord(inParam1);
			global_point.y = (UUtInt16)UUmLowWord(inParam1);

			VMrView_PointGlobalToView(inView, &global_point, &view_point);

			// check to see if the point is in the bounds of the view
			if (IMrRect_PointIn(&bounds_rect, &view_point))
			{
				VMrView_SendMessage(
					view_private_data->parent,
					VMcMessage_Command,
					UUmMakeLong(VMcNotify_Click, inView->id),
					(UUtUns32)inView);
			}

			private_data->mouse_state &= ~VMcMouseState_MouseDown;
		}
		return 0;

		case VMcMessage_Paint:
			// draw the button
			VMiView_Button_Paint(
				inView,
				button,
				private_data,
				(M3tPointScreen*)inParam2);
		return 0;
	}

	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtError
VMrView_Button_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	VMtView_Button			*button;
	VMtView_Button_PrivateData	*private_data;

	// get a pointer to the button data
	button = (VMtView_Button*)inInstancePtr;
	private_data = (VMtView_Button_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	switch (inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initalize the private data
			private_data->mouse_state				= VMcMouseState_None;
			private_data->texture_ref				= NULL;
			private_data->string_texture_width		= 0;
			private_data->string_texture_height		= 0;
			private_data->text_location.x			= 0;
			private_data->text_location.y			= 0;
			private_data->next_animation			= 0;
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
