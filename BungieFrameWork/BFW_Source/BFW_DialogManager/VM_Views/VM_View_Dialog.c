// ======================================================================
// VM_View_Dialog.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"

#include "VM_View_Dialog.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
VMrView_Dialog_Callback(
	VMtView					*inView,
	VMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32					result;
	DMtDialogData				*dialog_data;
	DMtDialogData_PrivateData	*private_data;
	
	// get the dialog data
	dialog_data = (DMtDialogData*)inView->view_data;
	private_data = (DMtDialogData_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Dialog_PrivateData, dialog_data);
	UUmAssert(private_data);
	
	result = 0;
	
	switch (inMessage)
	{
		case VMcMessage_Create:
			private_data->quit_dialog			= UUcFalse;
			private_data->out_message			= 0;
			private_data->text_focus_view		= NULL;
			private_data->mouse_focus_view		= NULL;
			private_data->mouse_over_view		= NULL;
			private_data->cursor				= NULL;
			private_data->mouse_clicked_view	= NULL;
			private_data->mouse_clicked_time	= 0;
			private_data->mouse_clicked_type	= VMcMessage_None;
		return 0;
		
		case VMcMessage_KeyDown:
		case VMcMessage_KeyUp:
			if (private_data->text_focus_view)
			{
				result =
					VMrView_SendMessage(
						private_data->text_focus_view,
						inMessage,
						inParam1,
						inParam2);
			}
			
			if (result == 0)
			{
				if (private_data->callback == NULL) break;
				result = private_data->callback(inView, inMessage, inParam1, inParam2);
			}
		return 0;
		
		case VMcMessage_Destroy:
		case VMcMessage_Command:
		case VMcMessage_Timer:
		case VMcMessage_MouseMove:
		case VMcMessage_LMouseDown:
		case VMcMessage_LMouseUp:
		case VMcMessage_MMouseDown:
		case VMcMessage_MMouseUp:
		case VMcMessage_RMouseDown:
		case VMcMessage_RMouseUp:
			if (private_data->callback == NULL) break;
			result = private_data->callback(inView, inMessage, inParam1, inParam2);
		return 0;
		
		case VMcMessage_Paint:
		{
			M3tPointScreen		destination;
			UUtUns16			alpha;
			
			// set the alpha
			if (inView->flags & VMcViewFlag_Enabled)
				alpha = VUcAlpha_Enabled;
			else
				alpha = VUcAlpha_Disabled;
			
			// get a pointer to the destination
			destination = *((M3tPointScreen*)inParam2);
			
			if (dialog_data->partspec)
			{
				VUrDrawPartSpecification(
					dialog_data->partspec,
					VMcPart_All,
					&destination,
					inView->width,
					inView->height,
					alpha);
			}
			else
			
			// don't draw if there aren't any textures to draw
			if (dialog_data->b_textures->num_textures > 0)
			{
				// draw the background texture
				VUrDrawTextureRef(
					dialog_data->b_textures->textures[0].texture_ref,
					&destination,
					inView->width,
					inView->height,
					alpha);
			}
		}
		break;  // let the defaull callback draw the child views
		
		case VMcMessage_Update:
			private_data->cursor_position.x = (UUtInt16)UUmHighWord(inParam1);
			private_data->cursor_position.y = (UUtInt16)UUmLowWord(inParam1);
		return 0;
	}
	
	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtError
DMrDialogData_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inDataPtr)
{
	DMtDialogData				*dialog_data;
	DMtDialogData_PrivateData	*private_data;
	
	UUmAssert(inInstancePtr);
	
	// get the dialog data
	dialog_data = (DMtDialogData*)inInstancePtr;
	private_data = (DMtDialogData_PrivateData*)inDataPtr;
	UUmAssert(private_data);
	
	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			private_data->callback				= NULL;
			private_data->quit_dialog			= UUcFalse;
			private_data->out_message			= 0;
			private_data->text_focus_view		= NULL;
			private_data->mouse_focus_view		= NULL;
			private_data->mouse_over_view		= NULL;
			private_data->cursor				= NULL;
			private_data->mouse_clicked_view	= NULL;
			private_data->mouse_clicked_time	= 0;
			private_data->mouse_clicked_type	= VMcMessage_None;
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
