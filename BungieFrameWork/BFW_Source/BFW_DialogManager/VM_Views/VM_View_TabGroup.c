// ======================================================================
// VM_View_TabGroup.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"

#include "VM_View_TabGroup.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
VMiView_TabGroup_Create(
	VMtView				*inView)
{
	VMtView_TabGroup_PrivateData	*private_data;
	UUtUns16						i;
	
	if (inView->view_data == NULL)
	{
		// allocate memory for the private data
		private_data =
			(VMtView_TabGroup_PrivateData*)UUrMemory_Block_New(
				sizeof(VMtView_TabGroup_PrivateData));
		UUmError_ReturnOnNull(private_data);
		
		// store the private data
		inView->view_data = private_data;
	}
		
	// hide all of the tabs except the first one
	for (i = 0; i < inView->num_child_views; i++)
	{
		VMtView			*view = (VMtView*)inView->child_views[i].view_ref;

		if (i == 0)
		{
			private_data->current_tab = view;
			VMrView_SetVisible(view, UUcTrue);
		}
		else
		{
			VMrView_SetVisible(view, UUcFalse);
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
VMiView_TabGroup_Destroy(
	VMtView							*inView,
	VMtView_TabGroup_PrivateData	*private_data)
{
	UUrMemory_Block_Delete(private_data);
	inView->view_data = NULL;
}

// ----------------------------------------------------------------------
static void
VMiView_TabGroup_Paint(
	VMtView							*inView,
	VMtView_TabGroup_PrivateData	*private_data,
	UUtUns32						inParam1,
	UUtUns32						inParam2)
{
	if ((private_data->current_tab) &&
		(private_data->current_tab->flags & VMcViewFlag_Visible))
	{
		VMrView_Draw(private_data->current_tab, (M3tPointScreen*)inParam2);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
VMrView_TabGroup_ActivateTab(
	VMtView				*inTabGroup,
	UUtUns16			inTabID)
{
	UUtUns16			i;
	VMtView_TabGroup_PrivateData	*private_data;
	
	private_data = (VMtView_TabGroup_PrivateData*)inTabGroup->view_data;
	UUmAssert(private_data);
	
	// find the tab with inTabID
	for (i = 0; i < inTabGroup->num_child_views; i++)
	{
		VMtView			*child = (VMtView*)inTabGroup->child_views[i].view_ref;
		
		if (child->id == inTabID)
		{
			// turn off the current tab
			if (private_data->current_tab)
			{
				// turn off its focus (focus must be done when the tab is visible)
				VMrView_SetFocus(private_data->current_tab, UUcFalse);

				// hide the current tab
				VMrView_SetVisible(private_data->current_tab, UUcFalse);
			}
			
			// don't reshow the tab that was just turned off (this is how the tabs toggle)
			if (child != private_data->current_tab)
			{
				// show the desired tab
				VMrView_SetVisible(child, UUcTrue);
				
				// turn on its focus (focus must be done when the tab is visible)
				VMrView_SetFocus(child, UUcTrue);
				
				// the child is now the current tab
				private_data->current_tab = child;
			}
			else
			{
				// revert to the first tab
				private_data->current_tab = 
					(VMtView*)inTabGroup->child_views[0].view_ref;
				
				// show the desired tab
				VMrView_SetVisible(private_data->current_tab, UUcTrue);
					
				// turn on its focus (focus must be done when the tab is visible)
				VMrView_SetFocus(private_data->current_tab, UUcTrue);
			}
			
			break;
		}
	}
}
	
// ----------------------------------------------------------------------
UUtUns32
VMrView_TabGroup_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_TabGroup_PrivateData	*private_data;
	
	// get the private data
	private_data = (VMtView_TabGroup_PrivateData*)inView->view_data;
	if (inMessage != VMcMessage_Create)
	{
		UUmAssert(private_data);
	}
	
	switch (inMessage)
	{
		case VMcMessage_Create:
			VMiView_TabGroup_Create(inView);
		return 0;
		
		case VMcMessage_Destroy:
			VMiView_TabGroup_Destroy(inView, private_data);
		return 0;
		
		case VMcMessage_Command:
		{
			VMtView_PrivateData		*view_private_data;
			
			view_private_data =
				(VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
			
			VMrView_SendMessage(
				view_private_data->parent,
				inMessage,
				inParam1,
				inParam2);
		}
		return 0;
		
		case VMcMessage_KeyDown:
			if (private_data->current_tab)
			{
				VMrView_SendMessage(
					private_data->current_tab,
					inMessage,
					inParam1,
					inParam2);
			}
		return 0;

		case VMcMessage_SetFocus:
			if (private_data->current_tab)
			{
				VMrView_SetFocus(private_data->current_tab, (UUtBool)inParam1);
			}
		return 0;
		
		case VMcMessage_Paint:
			// only draw the currently active tab
			VMiView_TabGroup_Paint(inView, private_data, inParam1, inParam2);
		return 0;	// tabgroups decide which tab to draw and draw it
	}
	
	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}