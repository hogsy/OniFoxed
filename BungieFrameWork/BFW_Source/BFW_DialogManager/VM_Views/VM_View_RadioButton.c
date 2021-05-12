// ======================================================================
// VM_View_RadioButton.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TextSystem.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"

#include "VM_View_RadioButton.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
VMiView_RadioButton_Create(
	VMtView							*inView,
	VMtView_RadioButton				*inRadioButton,
	VMtView_RadioButton_PrivateData	*inPrivateData)
{
	UUtError						error;
	
	// ------------------------------
	// setup the radiobutton
	// ------------------------------
	if (inRadioButton->off && inRadioButton->on)
	{
		// calculate the width and height of the radiobutton parts
		// both parts must be the same size
		inPrivateData->radiobutton_width =
			inRadioButton->off->part_matrix_br[VMcColumn_Left][VMcRow_Top].x -
			inRadioButton->off->part_matrix_tl[VMcColumn_Left][VMcRow_Top].x;
		inPrivateData->radiobutton_height =
			inRadioButton->off->part_matrix_br[VMcColumn_Left][VMcRow_Top].y -
			inRadioButton->off->part_matrix_tl[VMcColumn_Left][VMcRow_Top].y;
	}
	
	// ------------------------------
	// set up the title texture
	// ------------------------------
	if (inPrivateData->title_texture_ref == NULL)
	{
		UUtRect				bounds;
		
		// create the title_texture_ref
		error =
			VUrCreate_StringTexture(
				inRadioButton->title,
				inRadioButton->flags,
				&inPrivateData->title_texture_ref,
				&bounds);
		
		// save the width and height of the title
		inPrivateData->title_texture_width = bounds.right;
		inPrivateData->title_texture_height = bounds.bottom;
			
		// calculate the text location
		if (inRadioButton->off && inRadioButton->on)
		{
			// offset from the right of the picture
			inPrivateData->title_location.x =
				inPrivateData->radiobutton_width +
				VMcRadioButton_TextOffset_X;
				
			// center it relative to the picture
			inPrivateData->title_location.y =
				(inPrivateData->radiobutton_height -
				inPrivateData->title_texture_height) >>
				1;
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
VMiView_RadioButton_HandleMouseEvent(
	VMtView							*inView,
	VMtView_RadioButton				*inRadioButton,
	VMtView_RadioButton_PrivateData	*inPrivateData)
{
	// switch radiobutton states
	inPrivateData->radiobutton_state = !inPrivateData->radiobutton_state;
}

// ----------------------------------------------------------------------
static void
VMiView_RadioButton_Paint(
	VMtView							*inView,
	VMtView_RadioButton				*inRadioButton,
	VMtView_RadioButton_PrivateData	*inPrivateData,
	M3tPointScreen					*inDestination)
{
	UUtUns16						alpha;
	M3tPointScreen					dest;

	// set the alpha
	if (inView->flags & VMcViewFlag_Enabled)
		alpha = VUcAlpha_Enabled;
	else
		alpha = VUcAlpha_Disabled;

	// draw the radiobutton
	if (inRadioButton->off && inRadioButton->on)
	{
		if (inPrivateData->radiobutton_state == VMcRadioButton_Off)
		{
			VUrDrawPartSpecification(
				inRadioButton->off,
				VMcPart_LeftTop,
				inDestination,
				inPrivateData->radiobutton_width,
				inPrivateData->radiobutton_height,
				alpha);
		}
		else
		{
			VUrDrawPartSpecification(
				inRadioButton->on,
				VMcPart_LeftTop,
				inDestination,
				inPrivateData->radiobutton_width,
				inPrivateData->radiobutton_height,
				alpha);
		}
	}

	// draw the title texture
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
VMrView_RadioButton_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_RadioButton				*radiobutton;
	VMtView_RadioButton_PrivateData	*private_data;
	VMtView_PrivateData				*view_private_data;
	
	// get pointers to the data
	radiobutton = (VMtView_RadioButton*)inView->view_data;
	private_data = (VMtView_RadioButton_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_RadioButton_PrivateData, radiobutton);
	UUmAssert(private_data);
			
	view_private_data =
		(VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
			
	switch (inMessage)
	{
		case VMcMessage_Create:
			VMiView_RadioButton_Create(inView, radiobutton, private_data);
		return 0;
		
		case VMcMessage_LMouseUp:
			VMrView_SendMessage(
				inView,
				VMcMessage_SetValue,
				!private_data->radiobutton_state,
				0);
		return 0;
		
		case VMcMessage_Paint:
			// draw the button
			VMiView_RadioButton_Paint(
				inView,
				radiobutton,
				private_data,
				(M3tPointScreen*)inParam2);
		return 0;
		
		case VMcMessage_GetValue:
		return (UUtUns32)private_data->radiobutton_state;

		case VMcMessage_SetValue:
			private_data->radiobutton_state = (UUtBool)inParam1;
			if ((private_data->radiobutton_state != VMcRadioButton_Off) &&
				(private_data->radiobutton_state != VMcRadioButton_On))
			{
				private_data->radiobutton_state = VMcRadioButton_On;
			}
			
			if (view_private_data->parent != NULL)
			{
				if (private_data->radiobutton_state == VMcRadioButton_Off)
				{
					VMrView_SendMessage(
						view_private_data->parent,
						VMcMessage_Command,
						UUmMakeLong(RBcNotify_Off, inView->id),
						(UUtUns32)inView);
				}
				else
				{
					VMrView_SendMessage(
						view_private_data->parent,
						VMcMessage_Command,
						UUmMakeLong(RBcNotify_On, inView->id),
						(UUtUns32)inView);
				}
			}
		return 0;
	}
	
	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtError
VMrView_RadioButton_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	VMtView_RadioButton				*radiobutton;
	VMtView_RadioButton_PrivateData	*private_data;
	
	// get pointers to the data
	radiobutton = (VMtView_RadioButton*)inInstancePtr;
	private_data = (VMtView_RadioButton_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	switch (inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initalize the private data
			private_data->radiobutton_state			= VMcRadioButton_Off;
			private_data->title_texture_ref			= NULL;
			private_data->title_texture_width		= 0;
			private_data->title_texture_height		= 0;
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
