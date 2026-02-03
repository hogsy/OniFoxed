// ======================================================================
// VM_View_Tab.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"
#include "BFW_LocalInput.h"

#include "VM_View_Tab.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
VMiView_Tab_Create(
	VMtView						*inView,
	VMtView_Tab					*inTab,
	VMtView_Tab_PrivateData		*inPrivateData)
{
	// calculate the location of the outline
	if (inTab->outline)
	{
		UUtUns16			tab_button_height;

		// calculate the tab_button height
		tab_button_height = 0;
		if (inTab->tab_button)
		{
			tab_button_height =
				inTab->tab_button->part_matrix_br[VMcColumn_Left][VMcRow_Bottom].y -
				inTab->tab_button->part_matrix_tl[VMcColumn_Left][VMcRow_Top].y;

			// take off a little bit for the overlap
			tab_button_height -= 2;
		}

		// set the outline location, width, and height
		inPrivateData->outline_location.x = 0;
		inPrivateData->outline_location.y = tab_button_height;

		inPrivateData->outline_width = inView->width;
		inPrivateData->outline_height = inView->height - tab_button_height;
	}

	// init the private data
	inPrivateData->focused_view = NULL;
	inPrivateData->tab_callback = NULL;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
VMiView_Tab_Paint(
	VMtView						*inView,
	VMtView_Tab					*inTab,
	VMtView_Tab_PrivateData		*inPrivateData,
	M3tPointScreen				*inDestination)
{
	UUtUns16					alpha;
	M3tPointScreen				dest;

	// set the alpha
	if (inView->flags & VMcViewFlag_Enabled)
		alpha = VUcAlpha_Enabled;
	else
		alpha = VUcAlpha_Disabled;

	if (inTab->outline)
	{
		dest = *inDestination;
		dest.x += (float)inPrivateData->outline_location.x;
		dest.y += (float)inPrivateData->outline_location.y;

		VUrDrawPartSpecification(
			inTab->outline,
			VMcPart_All,
			&dest,
			inPrivateData->outline_width,
			inPrivateData->outline_height,
			alpha);
	}

	if (inTab->tab_button)
	{
		dest = *inDestination;
		dest.x += (float)inPrivateData->tab_button_location.x;
		dest.y += (float)inPrivateData->tab_button_location.y;

		VUrDrawPartSpecification(
			inTab->tab_button,
			VMcPart_All,
			&dest,
			inPrivateData->outline_width,
			inPrivateData->outline_height,
			alpha);
	}
}

// ----------------------------------------------------------------------
static void
VMiView_Tab_SetFocus(
	VMtView						*inView,
	VMtView_Tab_PrivateData		*inPrivateData,
	UUtBool						inIsFocused)
{
	VMtView						*view;

	if (inIsFocused)
	{
		// get the next focasable view
		view = VMrView_GetNextFocusableView(inView, inPrivateData->focused_view);

		if (view && (view != inPrivateData->focused_view))
		{
			// turn off focus on the previous view
			if (inPrivateData->focused_view)
			{
				VMrView_SetFocus(inPrivateData->focused_view, UUcFalse);
			}

			// focus on the found view
			VMrView_SetFocus(view, UUcTrue);

			// save the new view
			inPrivateData->focused_view = view;
		}
	}
	else if (inPrivateData->focused_view)
	{
		// turn off focus on the previous view
		VMrView_SetFocus(inPrivateData->focused_view, UUcFalse);

		// no views in the tab have the focus when the tab doesn't
		// have the focus.
		inPrivateData->focused_view = NULL;
	}
}

// ----------------------------------------------------------------------
static void
VMrView_Tab_SetFocusOnView(
	VMtView						*inView,
	VMtView_Tab_PrivateData		*inPrivateData,
	VMtView						*inFocusOnView)
{
	if (inFocusOnView != inPrivateData->focused_view)
	{
		// turn off focus on the previous view
		if (inPrivateData->focused_view)
		{
			VMrView_SetFocus(inPrivateData->focused_view, UUcFalse);
		}

		// focus on inFocusOnVIew
		VMrView_SetFocus(inFocusOnView, UUcTrue);

		// save the new view
		inPrivateData->focused_view = inFocusOnView;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
VMrView_Tab_SetTabCallback(
	VMtView				*inView,
	VMtTabCallback		inTabCallback)
{
	VMtView_Tab					*tab;
	VMtView_Tab_PrivateData		*private_data;
	UUtBool						result;

	UUmAssert(inView);

	// get the private data
	tab = (VMtView_Tab*)inView->view_data;
	private_data = (VMtView_Tab_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Tab_PrivateData, tab);
	UUmAssert(private_data);

	private_data->tab_callback = inTabCallback;

	// call the create functions
	result = UUcFalse;
	if (private_data->tab_callback)
	{
		result = private_data->tab_callback(inView, TbcMessage_InitTab, 0, 0);
	}

	return result;
}

// ----------------------------------------------------------------------
UUtUns32
VMrView_Tab_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_Tab					*tab;
	VMtView_Tab_PrivateData		*private_data;
	UUtBool						result;
	VMtView_PrivateData			*view_private_data;

	result = UUcFalse;

	// get the private data
	tab = (VMtView_Tab*)inView->view_data;
	private_data = (VMtView_Tab_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Tab_PrivateData, tab);
	UUmAssert(private_data);

	view_private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(view_private_data);

	if (inMessage != VMcMessage_Create)
	{
		UUmAssert(private_data);

		// call the tab callback if it exists
		if (private_data->tab_callback)
		{
			result = private_data->tab_callback(inView, inMessage, inParam1, inParam2);
		}
	}

	switch (inMessage)
	{
		case VMcMessage_Create:
			VMiView_Tab_Create(inView, tab, private_data);
		return 0;

		case VMcMessage_Command:
			if (result == UUcFalse)
			{
				VMtView		*child = (VMtView*)inParam2;

				switch (child->type)
				{
					case VMcViewType_EditField:
						VMrView_Tab_SetFocusOnView(inView, private_data, child);
					break;

					default:
						// propogate commands up the hiearchy in this case the
						// parent should always be a tabgroup
						VMrView_SendMessage(
							view_private_data->parent,
							inMessage,
							inParam1,
							inParam2);
					break;
				}
			}
		return 0;

		case VMcMessage_KeyDown:
			if (private_data->focused_view)
			{
				result =
					(UUtBool)VMrView_SendMessage(
						private_data->focused_view,
						inMessage,
						inParam1,
						inParam2);
				if (result == UUcFalse)
				{
					// the focused view didn't handle the key, then handle
					// tabs
					if (inParam1 == LIcKeyCode_Tab)
					{
						VMiView_Tab_SetFocus(inView, private_data, UUcTrue);
					}
				}
			}
		return 0;

		case VMcMessage_Paint:
			// draw the tab
			VMiView_Tab_Paint(
				inView,
				tab,
				private_data,
				(M3tPointScreen*)inParam2);
		break;

		case VMcMessage_SetFocus:
			VMiView_Tab_SetFocus(inView, private_data, (UUtBool)inParam1);
		return 0;
	}

	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtError
VMrView_Tab_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	VMtView_Tab				*tab;
	VMtView_Tab_PrivateData	*private_data;

	// get a pointer to the button data
	tab = (VMtView_Tab*)inInstancePtr;
	private_data = (VMtView_Tab_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	switch (inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initalize the private data
			private_data->focused_view		= NULL;
			private_data->tab_callback		= NULL;
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


