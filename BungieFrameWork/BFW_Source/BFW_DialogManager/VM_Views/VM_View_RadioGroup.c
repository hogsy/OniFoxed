// ======================================================================
// VM_View_RadioGroup.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"

#include "VM_View_RadioGroup.h"
#include "VM_View_RadioButton.h"

// ======================================================================
// defines
// ======================================================================
#define VMcView_RadioGroup_TLCurve			0
#define VMcView_RadioGroup_BLCurve			1

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
VMiView_RadioGroup_Create(
	VMtView							*inView,
	VMtView_RadioGroup				*inRadioGroup,
	VMtView_RadioGroup_PrivateData	*inPrivateData)
{
	UUtUns16			i;
	IMtPoint2D			dest;
	
	// ------------------------------
	// set the location and state of the child views
	// ------------------------------
	if (inRadioGroup->outline)
	{
		dest.x = 4;
		dest.y = 4;
	}
	else
	{
		dest.x = 0;
		dest.y = 0;
	}
	
	for (i = 0; i < inView->num_child_views; i++)
	{
		VMtView			*child;
		
		child = (VMtView*)inView->child_views[i].view_ref;
		
		VMrView_SetValue(child, VMcRadioButton_Off);
		VMrView_SetLocation(child, &dest);
		
		dest.y += child->height + 4;
	}
	
	// turn the first child in the group on
	inPrivateData->current_radiobutton = (VMtView*)inView->child_views[0].view_ref;
	VMrView_SetValue(inPrivateData->current_radiobutton, VMcRadioButton_On);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
VMiView_RadioGroup_Command(
	VMtView							*inView,
	VMtView_RadioGroup				*inRadioGroup,
	VMtView_RadioGroup_PrivateData	*inPrivateData,
	UUtUns16						inCommandType,
	VMtView							*inRadioButton)
{
	if ((inRadioButton != inPrivateData->current_radiobutton) &&
		(inCommandType == RBcNotify_On))
	{
		// turn off the current radio button
		VMrView_SetValue(inPrivateData->current_radiobutton, VMcRadioButton_Off);
		
		// set the new current radiobutton
		inPrivateData->current_radiobutton = inRadioButton;
	}
}

// ----------------------------------------------------------------------
static void
VMiView_RadioGroup_Paint(
	VMtView							*inView,
	VMtView_RadioGroup				*inRadioGroup,
	VMtView_RadioGroup_PrivateData	*inPrivateData,
	M3tPointScreen					*inDestination)
{
	UUtUns16						alpha;
	
	// set the alpha
	if (inView->flags & VMcViewFlag_Enabled)
		alpha = VUcAlpha_Enabled;
	else
		alpha = VUcAlpha_Disabled;

	// draw the outline
	if (inRadioGroup->outline)
	{
		VUrDrawPartSpecification(
			inRadioGroup->outline,
			VMcPart_All,
			inDestination,
			inView->width,
			inView->height,
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
VMrView_RadioGroup_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_RadioGroup				*radiogroup;
	VMtView_RadioGroup_PrivateData	*private_data;
	
	radiogroup = (VMtView_RadioGroup*)inView->view_data;
	private_data =
		(VMtView_RadioGroup_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_RadioGroup_PrivateData, radiogroup);
	UUmAssert(private_data);
	
	switch (inMessage)
	{
		case VMcMessage_Create:
			VMiView_RadioGroup_Create(inView, radiogroup, private_data);
		return 0;
		
		case VMcMessage_Command:
			VMiView_RadioGroup_Command(
				inView,
				radiogroup,
				private_data,
				(UUtUns16)UUmHighWord(inParam1),
				(VMtView*)inParam2);
		return 0;
		
		case VMcMessage_Paint:
			// draw the button
			VMiView_RadioGroup_Paint(
				inView,
				radiogroup,
				private_data,
				(M3tPointScreen*)inParam2);
		break;	// break to paint the child views
	}
	
	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtError
VMrView_RadioGroup_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	VMtView_RadioGroup				*radiobutton;
	VMtView_RadioGroup_PrivateData	*private_data;
	
	// get pointers to the data
	radiobutton = (VMtView_RadioGroup*)inInstancePtr;
	private_data = (VMtView_RadioGroup_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	switch (inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initalize the private data
			private_data->current_radiobutton = NULL;
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
