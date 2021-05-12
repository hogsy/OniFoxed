// ======================================================================
// VM_View_Box.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"
#include "BFW_Image.h"

#include "VM_View_Box.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
VMiView_Box_Paint(
	VMtView							*inView,
	VMtView_Box						*inBox,
	M3tPointScreen					*inDestination)
{
	UUtUns16						alpha;

	// set the alpha
	if (inView->flags & VMcViewFlag_Enabled)
		alpha = VUcAlpha_Enabled;
	else
		alpha = VUcAlpha_Disabled;

	if (inBox->box)
	{
		VUrDrawPartSpecification(
			inBox->box,
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
VMrView_Box_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_Box				*box;
	
	// get a pointer to the data
	box = (VMtView_Box*)inView->view_data;
	
	switch (inMessage)
	{
		case VMcMessage_Paint:
			// draw the button
			VMiView_Box_Paint(
				inView,
				box,
				(M3tPointScreen*)inParam2);
		return 0;
	}
	
	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}
