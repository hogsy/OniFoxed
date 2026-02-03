// ======================================================================
// VM_View_EditField.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"
#include "BFW_Timer.h"

#include "VM_View_EditField.h"

// ======================================================================
// defines
// ======================================================================
#define DMcControl_EditField_CaretDisplayTime	30	// in ticks
#define DMcControl_EditField_CaretHideTime		20	// in ticks

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
VMiView_EditField_UpdateCaret(
	VMtView							*inView,
	VMtView_EditField				*inEditField,
	VMtView_EditField_PrivateData	*inPrivateData)
{
	UUtUns32						time;

	if (inPrivateData->has_focus == UUcFalse) return;

	time = UUrMachineTime_Sixtieths();

	if (inPrivateData->caret_time < time)
	{
		// flash the caret if it is time
		if (inPrivateData->caret_time < time)
		{
			if (inPrivateData->caret_displayed)
			{
				// the caret is being displayed, turn it off
				inPrivateData->caret_time = time + (UUtUns32)DMcControl_EditField_CaretHideTime;
				inPrivateData->caret_displayed = UUcFalse;
			}
			else
			{
				// the caret is not being displayed, turn it on
				inPrivateData->caret_time = time + (UUtUns32)DMcControl_EditField_CaretDisplayTime;
				inPrivateData->caret_displayed = UUcTrue;
			}
		}
	}
}

// ----------------------------------------------------------------------
static void
VMiView_EditField_UpdateTextures(
	VMtView							*inView,
	VMtView_EditField				*inEditField,
	VMtView_EditField_PrivateData	*inPrivateData)
{
	UUtError						error;
	UUtRect							bounds;
	IMtPoint2D						dest;

	// calculate the bounds
	error =
		TSrContext_GetStringRect(
			inPrivateData->text_context,
			inPrivateData->string,
			&bounds);
	if (error != UUcError_None) return;

	inPrivateData->string_width = bounds.right;
	inPrivateData->string_height = bounds.bottom;

	if ((inView->width > M3cTextureMap_MaxWidth) ||
		(inView->height > M3cTextureMap_MaxHeight))
	{
		// erase the string texture
		M3rTextureMap_Big_Fill(
			inPrivateData->string_texture,
			inPrivateData->erase_color,
			&bounds);
	}
	else
	{
		// erase the string texture
		M3rTextureMap_Fill(
			inPrivateData->string_texture,
			inPrivateData->erase_color,
			&bounds);
	}

	// set dest
	dest.x = 0;
	dest.y = 0;

	// draw the string
	error =
		TSrContext_DrawString(
			inPrivateData->text_context,
			inPrivateData->string_texture,
			inPrivateData->string,
			&bounds,
			&dest);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
VMiView_EditField_Create(
	VMtView							*inView,
	VMtView_EditField				*inEditField,
	VMtView_EditField_PrivateData	*inPrivateData)
{
	UUtError			error;
	TStFontFamily		*font_family;
	char				*desired_font_family;
	TStFontStyle		desired_font_style;

	IMtPixel			text_color;

	M3tTextureMap_Big	*texture_big;
	M3tTextureMap		*texture;

	char				temp_string[2];
	UUtRect				bounds;
	IMtPoint2D			dest;

	UUtInt16			offset_x;
	UUtInt16			offset_y;

	// ------------------------------
	// set the colors to draw with
	// ------------------------------
	text_color = IMrPixel_FromShade(VMcControl_PixelType, VMcColor_Text);
	inPrivateData->erase_color = IMrPixel_FromShade(VMcControl_PixelType, VMcColor_Background);

	// ------------------------------
	// create the text context
	// ------------------------------
	if (inPrivateData->text_context == NULL)
	{
		// interpret the flags
		if (inEditField->flags & VMcCommonFlag_Text_Small)
			desired_font_family = TScFontFamily_RomanSmall;
		else
			desired_font_family = TScFontFamily_Roman;

		if (inEditField->flags & VMcCommonFlag_Text_Bold)
			desired_font_style = TScStyle_Bold;
		else if (inEditField->flags & VMcCommonFlag_Text_Italic)
			desired_font_style = TScStyle_Italic;
		else
			desired_font_style = TScStyle_Plain;

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
				TSc_HLeft | TSc_VCenter,
				&inPrivateData->text_context);
		UUmError_ReturnOnErrorMsg(error, "Unable to create text context");

		// set the color of the text
		error = TSrContext_SetColor(inPrivateData->text_context, text_color);
		UUmError_ReturnOnErrorMsg(error, "Unable to set the text color");

		// ------------------------------
		// create the string
		// ------------------------------
		if (inPrivateData->string == NULL)
		{
			inPrivateData->string = UUrMemory_Block_New(inEditField->max_chars + 1);
			UUmError_ReturnOnNull(inPrivateData->string);
			inPrivateData->string[0] = 0;
		}
	}

	// ------------------------------
	// create the string texture
	// ------------------------------
	if (inPrivateData->string_texture == NULL)
	{
		if ((inView->width > M3cTextureMap_MaxWidth) ||
			(inView->height > M3cTextureMap_MaxHeight))
		{
			// create a big texture map
			error =
				M3rTextureMap_Big_New(
					inView->width,
					inView->height,
					VMcControl_PixelType,
					M3cTexture_AllocMemory,
					M3cTextureFlags_None,
					"edit field",
					&texture_big);
			UUmError_ReturnOnErrorMsg(error, "Unable to create texture map");

			// clear the texture
			M3rTextureMap_Big_Fill(texture_big, inPrivateData->erase_color, M3cFillEntireMap);

			// store the texture
			inPrivateData->string_texture = texture_big;
		}
		else
		{
			// create a texture map
			error =
				M3rTextureMap_New(
					inView->width,
					inView->height,
					VMcControl_PixelType,
					M3cTexture_AllocMemory,
					M3cTextureFlags_None,
					"edit field",
					&texture);
			UUmError_ReturnOnErrorMsg(error, "Unable to create texture map");

			// clear the texture
			M3rTextureMap_Fill(texture, inPrivateData->erase_color, M3cFillEntireMap);

			// store the texture
			inPrivateData->string_texture = texture;
		}
	}

	// ------------------------------
	// create the caret texture
	// ------------------------------
	if (inPrivateData->caret_texture == NULL)
	{
		// calculate the width and height needed
		temp_string[0] = (char)255;
		temp_string[1] = 0;
		error =
			TSrContext_GetStringRect(
				inPrivateData->text_context,
				temp_string,
				&bounds);
		UUmError_ReturnOnErrorMsg(error, "Unable to get caret string rect");

		inPrivateData->caret_width = bounds.right;
		inPrivateData->caret_height = bounds.bottom;

		// create the caret texture
		error =
			M3rTextureMap_New(
				bounds.right,
				bounds.bottom,
				VMcControl_PixelType,
				M3cTexture_AllocMemory,
				M3cTextureFlags_None,
				"caret texture",
				&texture);
		UUmError_ReturnOnErrorMsg(error, "Unable to create texture map");

		// clear the texture
		M3rTextureMap_Fill(texture, inPrivateData->erase_color, M3cFillEntireMap);

		// draw the caret into the texture
		dest.x = 0;
		dest.y = 0;

		error =
			TSrContext_DrawString(
				inPrivateData->text_context,
				texture,
				temp_string,
				&bounds,
				&dest);
		UUmError_ReturnOnErrorMsg(error, "Unable to draw string in texture");

		inPrivateData->caret_texture = texture;
	}

	// ------------------------------
	// calculate the text location
	// ------------------------------
	inPrivateData->string_width = 0;
	inPrivateData->string_height =
		TSrFont_GetLineHeight(
			TSrContext_GetFont(inPrivateData->text_context, TScStyle_InUse));

	// calculate offset_x and offset_y
	offset_x = offset_y = 0;
	if (inEditField->flags & VMcCommonFlag_Text_HRight)
		offset_x = inView->width - inPrivateData->string_width;
	else if (inEditField->flags & VMcCommonFlag_Text_HCenter)
		offset_x = ((inView->width - inPrivateData->string_width) >> 1);
	if (inEditField->flags & VMcCommonFlag_Text_VCenter)
		offset_y = ((inView->height - inPrivateData->string_height) >> 1);

	inPrivateData->text_location.x = inEditField->text_location_offset.x + offset_x;
	inPrivateData->text_location.y = inEditField->text_location_offset.y + offset_y;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
VMiView_EditField_Paint(
	VMtView							*inView,
	VMtView_EditField				*inEditField,
	VMtView_EditField_PrivateData	*inPrivateData,
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
	if (inEditField->outline)
	{
		VUrDrawPartSpecification(
			inEditField->outline,
			VMcPart_All,
			inDestination,
			inView->width,
			inView->height,
			alpha);
	}

	// draw the string
	if (inPrivateData->string_texture)
	{
		dest = *inDestination;
		dest.x += (float)inPrivateData->text_location.x;
		dest.y += (float)inPrivateData->text_location.y;

		VUrDrawTextureRef(
			inPrivateData->string_texture,
			&dest,
			inPrivateData->string_width,
			inPrivateData->string_height,
			alpha);
	}

	// draw the caret
	if ((inPrivateData->caret_texture) &&
		(inPrivateData->caret_displayed) &&
		(inView->flags & VMcViewFlag_Enabled))
	{
		dest = *inDestination;
		dest.x += (float)(inPrivateData->text_location.x + inPrivateData->string_width);
		dest.y += (float)(inPrivateData->text_location.y);

		VUrDrawTextureRef(
			inPrivateData->caret_texture,
			&dest,
			inPrivateData->caret_width,
			inPrivateData->caret_height,
			alpha);
	}
}

// ----------------------------------------------------------------------
static UUtBool
VMiVew_EditField_HandleKeyDown(
	VMtView							*inView,
	VMtView_EditField				*inEditField,
	VMtView_EditField_PrivateData	*inPrivateData,
	UUtUns16						inKey)
{
	UUtBool							handled;

	UUmAssert(inPrivateData->string);
	UUmAssert(inPrivateData->string_texture);
	UUmAssert(inPrivateData->text_context);

	// add the key to the string
	handled =
		TSrString_AppendChar(
			inPrivateData->string,
			inKey,
			inEditField->max_chars);

	// update the textures
	VMiView_EditField_UpdateTextures(inView, inEditField, inPrivateData);

	return handled;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
VMrView_EditField_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_EditField				*editfield;
	VMtView_EditField_PrivateData	*private_data;
	UUtUns32						result;
	VMtView_PrivateData				*view_private_data;

	// get the views data
	editfield = (VMtView_EditField*)inView->view_data;
	private_data = (VMtView_EditField_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_EditField_PrivateData, editfield);
	UUmAssert(private_data);

	// get the view's private data
	view_private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(view_private_data);

	result = 0;

	switch (inMessage)
	{
		case VMcMessage_Create:
			VMiView_EditField_Create(inView, editfield, private_data);


			private_data->has_focus = UUcFalse;
			private_data->caret_displayed = UUcFalse;
			private_data->string[0] = '\0';
		return 0;

		case VMcMessage_KeyDown:
			result =
				VMiVew_EditField_HandleKeyDown(
					inView,
					editfield,
					private_data,
					(UUtUns16)inParam1);
			return result;
		return 0;

		case VMcMessage_LMouseUp:
		{
			VMrView_SendMessage(
				view_private_data->parent,
				VMcMessage_Command,
				UUmMakeLong(VMcNotify_Click, inView->id),
				(UUtUns32)inView);
		}
		return 0;

		case VMcMessage_SetFocus:
			if (inParam1)
			{
				private_data->has_focus = UUcTrue;
			}
			else
			{
				private_data->has_focus = UUcFalse;
				private_data->caret_displayed = UUcFalse;
			}
		return 0;

		case VMcMessage_Paint:
			// update the caret
			VMiView_EditField_UpdateCaret(inView, editfield, private_data);

			// draw the button
			VMiView_EditField_Paint(
				inView,
				editfield,
				private_data,
				(M3tPointScreen*)inParam2);
		return 0;

		case VMcMessage_GetValue:
		return (UUtUns32)private_data->string;

		case VMcMessage_SetValue:
		{
			// copy the string
			UUrString_Copy(private_data->string, (char*)inParam1, editfield->max_chars);

			// update the textures
			VMiView_EditField_UpdateTextures(inView, editfield, private_data);
		}
		return 0;
	}

	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtError
VMrView_EditField_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	VMtView_EditField				*editfield;
	VMtView_EditField_PrivateData	*private_data;

	// get a pointer to the button data
	editfield = (VMtView_EditField*)inInstancePtr;
	private_data = (VMtView_EditField_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	switch (inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initalize the private data
			private_data->text_context			= NULL;
			private_data->string				= NULL;

			private_data->caret_texture			= NULL;
			private_data->caret_width			= 0;
			private_data->caret_height			= 0;
			private_data->caret_displayed		= UUcFalse;
			private_data->caret_time			= 0;

			private_data->string_texture		= NULL;
			private_data->string_width			= editfield->text_location_offset.x;
			private_data->string_height			= editfield->text_location_offset.y;

			private_data->has_focus				= UUcFalse;
		break;

		case TMcTemplateProcMessage_DisposePreProcess:
			if (private_data->text_context)
			{
				TSrContext_Delete(private_data->text_context);
				private_data->text_context = NULL;
			}

			if (private_data->string)
			{
				UUrMemory_Block_Delete(private_data->string);
				private_data->string = NULL;
			}
		break;

		case TMcTemplateProcMessage_Update:
		break;

		default:
			UUmAssert(!"Illegal message");
		break;
	}

	return UUcError_None;
}
