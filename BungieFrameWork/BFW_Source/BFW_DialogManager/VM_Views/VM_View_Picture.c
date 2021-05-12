// ======================================================================
// VM_View_Picture.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"

#include "VM_View_Picture.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
VMiView_Picture_Paint(
	VMtView							*inView,
	VMtView_Picture					*inPicture,
	M3tPointScreen					*inDestination)
{
	UUtUns16			alpha;
	
	// set the alpha
	if (inView->flags & VMcViewFlag_Enabled)
		alpha = VUcAlpha_Enabled;
	else
		alpha = VUcAlpha_Disabled;
	
	if (inPicture->partspec)
	{
		VUrDrawPartSpecification(
			inPicture->partspec,
			VMcPart_MiddleMiddle,
			inDestination,
			inView->width,
			inView->height,
			alpha);
	}
	else
	{
		// draw the background texture
		if (inPicture->b_textures->num_textures > 0)
		{
			VUrDrawTextureRef(
				inPicture->b_textures->textures[0].texture_ref,
				inDestination,
				inView->width,
				inView->height,
				alpha);
		}
	}
}

// ----------------------------------------------------------------------
UUtUns32
VMrView_Picture_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_Picture		*picture;
	
	// get a pointer to the picture data
	picture = (VMtView_Picture*)inView->view_data;
	
	switch (inMessage)
	{
		case VMcMessage_Paint:
			// draw the button
			VMiView_Picture_Paint(
				inView,
				picture,
				(M3tPointScreen*)inParam2);
		break;
	}
	
	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}