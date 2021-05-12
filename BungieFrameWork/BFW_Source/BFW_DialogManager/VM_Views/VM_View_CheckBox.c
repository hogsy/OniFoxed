// ======================================================================
// VM_View_CheckBox.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TextSystem.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"

#include "VM_View_CheckBox.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
VMiView_CheckBox_Create(
	VMtView							*inView,
	VMtView_CheckBox				*inCheckBox,
	VMtView_CheckBox_PrivateData	*inPrivateData)
{
	UUtError						error;
	
	// ------------------------------
	// setup the checkbox
	// ------------------------------
	if (inCheckBox->off && inCheckBox->on)
	{
		inPrivateData->checkbox_width =
			inCheckBox->off->part_matrix_br[VMcColumn_Left][VMcRow_Top].x -
			inCheckBox->off->part_matrix_tl[VMcColumn_Left][VMcRow_Top].x;
		inPrivateData->checkbox_height =
			inCheckBox->off->part_matrix_br[VMcColumn_Left][VMcRow_Top].y -
			inCheckBox->off->part_matrix_tl[VMcColumn_Left][VMcRow_Top].y;

		// the checkbox location is offset from the view's location
		inPrivateData->checkbox_location.x = 4;
		inPrivateData->checkbox_location.y = 4;
	}
	
	// ------------------------------
	// create the title_texture_ref
	// ------------------------------
	if (inPrivateData->title_texture_ref == NULL)
	{
		UUtRect						string_bounds;
		
		// create the background texture with the outline
		error =
			VUrCreate_StringTexture(
				inCheckBox->title,
				inCheckBox->flags,
				&inPrivateData->title_texture_ref,
				&string_bounds);
		UUmError_ReturnOnErrorMsg(error, "Unable to create the texture");
		
		inPrivateData->title_texture_width = string_bounds.right;
		inPrivateData->title_texture_height = string_bounds.bottom;

		// calculate the text location
		if (inCheckBox->off && inCheckBox->on)
		{
			inPrivateData->title_location.x =
				inPrivateData->checkbox_location.x +
				inPrivateData->checkbox_width +
				inCheckBox->title_location_offset.x;
			inPrivateData->title_location.y =
				(inView->height - inPrivateData->title_texture_height) >> 1;
		}
		else
		{
			inPrivateData->title_location = inCheckBox->title_location_offset;
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
VMiView_CheckBox_HandleMouseEvent(
	VMtView							*inView,
	VMtView_CheckBox				*inCheckBox,
	VMtView_CheckBox_PrivateData	*inPrivateData)
{
	// switch checkbox states
	inPrivateData->checkbox_state = !inPrivateData->checkbox_state;
}

// ----------------------------------------------------------------------
static void
VMiView_CheckBox_Paint(
	VMtView							*inView,
	VMtView_CheckBox				*inCheckBox,
	VMtView_CheckBox_PrivateData	*inPrivateData,
	M3tPointScreen					*inDestination)
{
	UUtUns16						alpha;
	M3tPointScreen					dest;

	// set the alpha
	if (inView->flags & VMcViewFlag_Enabled)
		alpha = VUcAlpha_Enabled;
	else
		alpha = VUcAlpha_Disabled;

	// draw the outline
	if (inCheckBox->outline)
	{
		VUrDrawPartSpecification(
			inCheckBox->outline,
			VMcPart_All,
			inDestination,
			inView->width,
			inView->height,
			alpha);
	}
	
	// draw the checkbox
	if (inCheckBox->off && inCheckBox->on)
	{
		dest = *inDestination;
		dest.x += (float)inPrivateData->checkbox_location.x;
		dest.y += (float)inPrivateData->checkbox_location.y;
		
		if (inPrivateData->checkbox_state == VMcCheckBox_Off)
		{
			VUrDrawPartSpecification(
				inCheckBox->off,
				VMcPart_LeftTop,
				&dest,
				inPrivateData->checkbox_width,
				inPrivateData->checkbox_height,
				alpha);
		}
		else
		{
			VUrDrawPartSpecification(
				inCheckBox->on,
				VMcPart_LeftTop,
				&dest,
				inPrivateData->checkbox_width,
				inPrivateData->checkbox_height,
				alpha);
		}
	}
	
	// draw the title texture
	if (inPrivateData->title_texture_ref)
	{
		// offset the title
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
VMrView_CheckBox_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_CheckBox				*checkbox;
	VMtView_CheckBox_PrivateData	*private_data;
	
	// get pointers to the data
	checkbox = (VMtView_CheckBox*)inView->view_data;
	private_data = (VMtView_CheckBox_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_CheckBox_PrivateData, checkbox);
	UUmAssert(private_data);
	
	switch (inMessage)
	{
		case VMcMessage_Create:
			VMiView_CheckBox_Create(inView, checkbox, private_data);
		return 0;
		
		case VMcMessage_LMouseUp:
			VMiView_CheckBox_HandleMouseEvent(inView, checkbox, private_data);
		return 0;
		
		case VMcMessage_Paint:
			// draw the button
			VMiView_CheckBox_Paint(
				inView,
				checkbox,
				private_data,
				(M3tPointScreen*)inParam2);
		return 0;
		
		case VMcMessage_GetValue:
		return (UUtUns32)private_data->checkbox_state;
		
		case VMcMessage_SetValue:
			private_data->checkbox_state = (UUtBool)inParam1;
		return 0;
	}
	
	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtError
VMrView_CheckBox_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	VMtView_CheckBox				*checkbox;
	VMtView_CheckBox_PrivateData	*private_data;
	
	// get pointers to the data
	checkbox = (VMtView_CheckBox*)inInstancePtr;
	private_data = (VMtView_CheckBox_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	switch (inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initalize the private data
			private_data->checkbox_state			= VMcCheckBox_Off;
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
