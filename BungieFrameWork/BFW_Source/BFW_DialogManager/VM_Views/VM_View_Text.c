// ======================================================================
// VM_View_Text.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"
#include "BFW_TextSystem.h"
#include "BFW_Image.h"

#include "VM_View_Text.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
VMiView_Text_Update(
	VMtView							*inView,
	VMtView_Text					*inText,
	VMtView_Text_PrivateData		*inPrivateData)
{
	UUtError						error;
	UUtRect							bounds;
	IMtPoint2D						dest;
	
	// clear the texture
	VUrClearTexture(
		inPrivateData->text_texture_ref,
		NULL,
		IMrPixel_FromShade(VMcControl_PixelType, IMcShade_None));
	
	// set the bounds
	bounds.left		= 0;
	bounds.top		= 0;
	bounds.right	= inView->width;
	bounds.bottom	= inView->height;
	
	// set the dest
	dest.x = inText->text_location_offset.x;
	dest.y =
		inText->text_location_offset.y +
		inPrivateData->font_line_height -
		inPrivateData->font_descending_height;
	
	// draw the text into the texture map
	error =
		TSrContext_DrawString(
			inPrivateData->text_context,
			inPrivateData->text_texture_ref,
			inText->string,
			&bounds,
			&dest);
	UUmError_ReturnOnErrorMsg(error, "Unable to draw string in texture");
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
VMiView_Text_Create(
	VMtView							*inView,
	VMtView_Text					*inText,
	VMtView_Text_PrivateData		*inPrivateData)
{
	UUtError						error;
	
	TStFont							*font;
	TStFontFamily					*font_family;
	char							*desired_font_family;
	TStFontStyle					desired_font_style;
	UUtUns16						desired_font_format;
	IMtPixel						white;

	// ------------------------------
	// create the text_texture_ref
	// ------------------------------
	if (inPrivateData->text_texture_ref == NULL)
	{
		error =
			VUrCreate_Texture(
				inView->width,
				inView->height,
				&inPrivateData->text_texture_ref);
		UUmError_ReturnOnErrorMsg(error, "Unable to create the background texture");
	}
	
	// ------------------------------
	// create the text context
	// ------------------------------
	if (inPrivateData->text_context == NULL)
	{
		// interpret the flags
		if (inText->flags & VMcCommonFlag_Text_Small)
			desired_font_family = TScFontFamily_RomanSmall;
		else
			desired_font_family = TScFontFamily_Roman;
		
		if (inText->flags & VMcCommonFlag_Text_Bold)
			desired_font_style = TScStyle_Bold;
		else if (inText->flags & VMcCommonFlag_Text_Italic)
			desired_font_style = TScStyle_Italic;
		else
			desired_font_style = TScStyle_Plain;
		
		if (inText->flags & VMcCommonFlag_Text_HCenter)
			desired_font_format = TSc_HCenter;
		else if (inText->flags & VMcCommonFlag_Text_HRight)
			desired_font_format = TSc_HRight;
		else
			desired_font_format = TSc_HLeft;
		
		if (inText->flags & VMcCommonFlag_Text_VCenter)
			desired_font_format |= TSc_VCenter;
			
		// get the font family
		error =
			TMrInstance_GetDataPtr(
				TScTemplate_FontFamily,
				desired_font_family,
				&font_family);
		UUmError_ReturnOnErrorMsg(error, "Unable to load font family");
		
		// create a new text context
		error =
			TSrContext_New(
				font_family,
				desired_font_style,
				desired_font_format,
				&inPrivateData->text_context);
		UUmError_ReturnOnErrorMsg(error, "Unable to create text context");
		
		// set the color of the text
		white = IMrPixel_FromShade(VMcControl_PixelType, IMcShade_White);
		error = TSrContext_SetColor(inPrivateData->text_context, white);
		UUmError_ReturnOnErrorMsg(error, "Unable to set the text color");
		
		// get the font heights
		font = TSrContext_GetFont(inPrivateData->text_context, TScStyle_InUse);
		inPrivateData->font_line_height = TSrFont_GetLineHeight(font);
		inPrivateData->font_descending_height = TSrFont_GetDescendingHeight(font);
	}
	
	// ------------------------------
	// draw the title onto the title_texture_ref
	// ------------------------------
	VMiView_Text_Update(inView, inText, inPrivateData);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
VMiView_Text_Destroy(
	VMtView							*inView,
	VMtView_Text					*inText,
	VMtView_Text_PrivateData		*inPrivateData)
{
	if (inPrivateData->text_context)
	{
		TSrContext_Delete(inPrivateData->text_context);
		inPrivateData->text_context = NULL;
	}
}

// ----------------------------------------------------------------------
static void
VMiView_Text_Paint(
	VMtView							*inView,
	VMtView_Text					*inText,
	VMtView_Text_PrivateData		*inPrivateData,
	M3tPointScreen					*inDestination)
{
	UUtUns16						alpha;

	// set the alpha
	if (inView->flags & VMcViewFlag_Enabled)
		alpha = VUcAlpha_Enabled;
	else
		alpha = VUcAlpha_Disabled;
	
	if (inText->outline)
	{
		VUrDrawPartSpecification(
			inText->outline,
			VMcPart_All,
			inDestination,
			inView->width,
			inView->height,
			alpha);
	}
	
	if (inPrivateData->text_texture_ref)
	{
		VUrDrawTextureRef(
			inPrivateData->text_texture_ref,
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
VMrView_Text_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_Text		*text;
	VMtView_Text_PrivateData	*private_data;
	
	// get a pointer to the data
	text = (VMtView_Text*)inView->view_data;
	private_data = (VMtView_Text_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_Text_PrivateData, text);
	UUmAssert(private_data);
	
	switch (inMessage)
	{
		case VMcMessage_Create:
			VMiView_Text_Create(inView, text, private_data);
		return 0;
		
		case VMcMessage_Destroy:
			VMiView_Text_Destroy(inView, text, private_data);
		return 0;
		
		case VMcMessage_Paint:
			// draw the button
			VMiView_Text_Paint(
				inView,
				text,
				private_data,
				(M3tPointScreen*)inParam2);
		return 0;
		
		case VMcMessage_SetValue:
			UUrString_Copy(text->string, (char*)inParam1, VMcView_Text_MaxStringLength);
			VMiView_Text_Update(inView, text, private_data);
		return 0;
	}
	
	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtError
VMrView_Text_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	VMtView_Text			*text;
	VMtView_Text_PrivateData	*private_data;
	
	// get a pointer to the button data
	text = (VMtView_Text*)inInstancePtr;
	private_data = (VMtView_Text_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	switch (inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initalize the private data
			private_data->text_texture_ref			= NULL;
			private_data->text_context				= NULL;
			private_data->font_line_height			= 0;
			private_data->font_descending_height	= 0;
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
