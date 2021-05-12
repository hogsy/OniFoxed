// ======================================================================
// WM_EditField.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_TextSystem.h"
#include "BFW_LocalInput.h"

#include "WM_EditField.h"

#include <ctype.h>

// ======================================================================
// enums
// ======================================================================
#define WMcEditField_DefaultMaxChars	255
#define WMcEditField_Text_Buffer		2

// ======================================================================
// enums
// ======================================================================
enum
{
	WMcEditFieldFlag_None		= 0x0000 << 16,
	WMcEditFieldFlag_HasFocus	= 0x0001 << 16,
	WMcEditFieldFlag_FocusClick = 0x0002 << 16

};

// ======================================================================
// typedefs
// ======================================================================
typedef struct WMtEditField_PrivateData
{
	UUtUns16					max_chars;
	char						*string;
	
	UUtUns32					editfield_data;
	UUtInt32					insert_before;
	UUtUns16					insert_segment;
	
	UUtRect						text_bounds;
	
} WMtEditField_PrivateData;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
WMiEditField_SetCaretPosition(
	WMtEditField				*inEditField,
	WMtEditField_PrivateData	*inPrivateData,
	TStStringFormat				*inStringFormat)
{
	UUtInt16					x, y;
	UUtUns16					i;
	UUtUns16					string_length;
	TStFont						*font;
	TStStringFormat				string_format;
	UUtUns16					index_in_segment;
	TStFontInfo					font_info;
	
	WMrWindow_GetFontInfo(inEditField, &font_info);
	DCrText_SetFontInfo(&font_info);
	
	// if this edit field doesn't have the focus then don't set the caret's position
	if ((inPrivateData->editfield_data & WMcEditFieldFlag_HasFocus) == 0) { return; }
	
	// generate a formated string if one hasn't been supplied
	if (inStringFormat == NULL)
	{
		UUtError				error;
		IMtPoint2D				dest;
		
		// format the text
		dest.x = inPrivateData->text_bounds.left + WMcEditField_Text_Buffer;
		dest.y =
			inPrivateData->text_bounds.top +
			DCrText_GetAscendingHeight() +
			DCrText_GetLeadingHeight();
		
		error =
			TSrContext_FormatString(
				DCrText_GetTextContext(),
				inPrivateData->string,
				&inPrivateData->text_bounds,
				&dest,
				&string_format);
		if (error != UUcError_None) { return; }
		
		inStringFormat = &string_format;
	}
	
	// figure out which segment the caret should be on
	index_in_segment = (UUtUns16) inPrivateData->insert_before;
	for (i = 0; i < inStringFormat->num_segments; i++)
	{
		if (index_in_segment < inStringFormat->num_characters[i]) {
			break;
		} else {
			index_in_segment -= inStringFormat->num_characters[i];
		}
	}
	inPrivateData->insert_segment = i;

	if (inPrivateData->insert_segment < inStringFormat->num_segments) {
		// calculate the x position of the caret within this segment
		x = inStringFormat->bounds[inPrivateData->insert_segment].left;
		y = inStringFormat->bounds[inPrivateData->insert_segment].top;

		font = DCrText_GetFont();
		string_length = TSrString_GetLength(inStringFormat->segments[inPrivateData->insert_segment]);
		for (i = 0; i < index_in_segment; i++)
		{
			UUtUns16				character;
			UUtUns16				width;
			
			character = TSrString_GetCharacterAtIndex(inStringFormat->segments[inPrivateData->insert_segment], i);
			TSrFont_GetCharacterSize(font, character, &width, NULL);
			x += (UUtInt16)width;
		}

	} else if (inStringFormat->num_segments > 0) {
		// place the caret at the end of the last segment
		x = inStringFormat->bounds[inStringFormat->num_segments - 1].right;
		y = inStringFormat->bounds[inStringFormat->num_segments - 1].top;

	} else {
		// place the caret at the start of the text bounds
		x = inPrivateData->text_bounds.left;
		y = inPrivateData->text_bounds.top;
	}

	// set the position of the caret
	WMrCaret_SetPosition(WMcEditField_Text_Buffer + x - 1, y);
}

// ----------------------------------------------------------------------
static void
WMiEditField_SetInsertBeforeFromPoint(
	WMtEditField				*inEditField,
	WMtEditField_PrivateData	*inPrivateData,
	IMtPoint2D					*inLocalPoint)
{
	UUtInt16					x;
	UUtUns16					i, j, segment_index;
	UUtUns16					string_length;
	TStFont						*font;
	TStStringFormat				string_format;
	UUtError					error;
	IMtPoint2D					dest;
	UUtInt16					test;
	TStFontInfo					font_info;
	
	WMrWindow_GetFontInfo(inEditField, &font_info);
	DCrText_SetFontInfo(&font_info);
	
	// format the text
	dest.x = inPrivateData->text_bounds.left + WMcEditField_Text_Buffer;
	dest.y =
		inPrivateData->text_bounds.top +
		DCrText_GetAscendingHeight() +
		DCrText_GetLeadingHeight();
	
	error =
		TSrContext_FormatString(
			DCrText_GetTextContext(),
			inPrivateData->string,
			&inPrivateData->text_bounds,
			&dest,
			&string_format);
	if (error != UUcError_None) { return; }
	
	// find out which line the user clicked in
	inPrivateData->insert_before = 0;
	segment_index = 0;
	for (i = 0; i < string_format.num_lines; i++)
	{
		test = string_format.bounds[segment_index].bottom;
		
		if (inLocalPoint->y < test)
		{
			break;
		}
		
		for (j = 0; j < string_format.line_num_segments[i]; j++, segment_index++) {
			inPrivateData->insert_before += string_format.num_characters[segment_index];
		}
	}

	if (i < string_format.num_lines) {
		// find out which segment in this line the user clicked in
		for (j = 0; j < string_format.line_num_segments[i]; j++, segment_index++) {
			test = string_format.bounds[segment_index].right;

			if (inLocalPoint->x < test)
			{
				break;
			}

			inPrivateData->insert_before += string_format.num_characters[segment_index];
		}
	}

	if (segment_index < string_format.num_segments) {
		// find the character index that the click is closest to
		x = string_format.bounds[segment_index].left + WMcEditField_Text_Buffer;
		string_length = TSrString_GetLength(string_format.segments[segment_index]);
		font = DCrText_GetFont();
		
		for (i = 0; i < string_length; i++)
		{
			UUtUns16				character;
			UUtUns16				width;
			
			character = TSrString_GetCharacterAtIndex(string_format.segments[segment_index], i);
			TSrFont_GetCharacterSize(font, character, &width, NULL);
			
			// when testing the position, test to see if the user clicked
			// in the first half of the character
			test = x + (width >> 1);
			
			if (inLocalPoint->x < test)
			{
				break;
			}
			
			// add in the second half of the character
			x += width;
		}
		
		// i is the index of the character that the insertion point should go before
		// note that i == string_length, a point in the string wasn't found, the
		// user clicked beyond the text so we put the insert before point at the end of the string.
		inPrivateData->insert_before += i;
	}
	
	// update the caret position
	WMiEditField_SetCaretPosition(inEditField, inPrivateData, &string_format);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
WMiEditField_Create(
	WMtEditField				*inEditField)
{
	PStPartSpecUI				*partspec_ui;
	UUtInt16					left_width;
	UUtInt16					top_height;
	UUtInt16					right_width;
	UUtInt16					bottom_height;
	WMtEditField_PrivateData	*private_data;
	
	// create the private data
	private_data = 
		(WMtEditField_PrivateData*)UUrMemory_Block_NewClear(
			sizeof(WMtEditField_PrivateData));
	UUmError_ReturnOnNull(private_data);
	WMrWindow_SetLong(inEditField, 0, (UUtUns32)private_data);
	
	// initialize
	private_data->max_chars = WMcEditField_DefaultMaxChars;
	private_data->string = NULL;
	private_data->editfield_data = WMcEditFieldFlag_None;
	private_data->insert_before = 0;
	
	// allocate memory for the string
	private_data->string = UUrMemory_Block_NewClear(private_data->max_chars + 1);
	UUmError_ReturnOnNull(private_data->string);
	
	// get the active partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return UUcError_Generic; }
	
	// get the size of the parts
	PSrPartSpec_GetSize(
		partspec_ui->editfield,
		PScPart_LeftTop,
		&left_width,
		&top_height);
	
	PSrPartSpec_GetSize(
		partspec_ui->editfield,
		PScPart_RightBottom,
		&right_width,
		&bottom_height);
	
	// set the text bounds
	WMrWindow_GetClientRect(inEditField, &private_data->text_bounds);
	
	private_data->text_bounds.left += left_width;
	private_data->text_bounds.top += top_height;
	private_data->text_bounds.right -= right_width;
	private_data->text_bounds.bottom -= bottom_height;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
WMiEditField_Destroy(
	WMtEditField				*inEditField)
{
	WMtEditField_PrivateData	*private_data;
	
	// get the private data
	private_data = (WMtEditField_PrivateData*)WMrWindow_GetLong(inEditField, 0);
	if (private_data == NULL) { return; }
	
	// delete the string
	if (private_data->string)
	{
		UUrMemory_Block_Delete(private_data->string);
		private_data->string = NULL;
	}
		
	// delete the private data
	UUrMemory_Block_Delete(private_data);
	WMrWindow_SetLong(inEditField, 0, 0);
}

// ----------------------------------------------------------------------
static UUtUns32
WMiEditField_HandleGetDialogCode(
	WMtEditField				*inEditField)
{
	UUtUns32					result;
	
	result =
		WMcDialogCode_WantDigits |
		WMcDialogCode_WantPunctuation |
		WMcDialogCode_WantArrows |
		WMcDialogCode_WantDelete;
	if ((WMrWindow_GetStyle(inEditField) & WMcEditFieldStyle_NumbersOnly) == 0)
	{
		result |= WMcDialogCode_WantAlphas;
	}
	
	return result;
}

// ----------------------------------------------------------------------
static UUtUns32	WMiEditField_HandleGetFloat(
	WMtEditField *inEditField,
	float *outFloat)
{
	UUtError error;

	WMtEditField_PrivateData	*private_data;
	
	private_data = (WMtEditField_PrivateData*)WMrWindow_GetLong(inEditField, 0);

	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->string == NULL) { return UUcError_Generic; }

	error = UUrString_To_Float(private_data->string, outFloat);

	return error;
}

// ----------------------------------------------------------------------
static UUtUns32	WMiEditField_HandleGetInt(
	WMtEditField *inEditField,
	UUtUns32 *outNumber)
{
	UUtError error;

	WMtEditField_PrivateData	*private_data;
	
	private_data = (WMtEditField_PrivateData*)WMrWindow_GetLong(inEditField, 0);

	if (private_data == NULL) { return UUcError_Generic; }
	if (private_data->string == NULL) { return UUcError_Generic; }

	error = UUrString_To_Int32(private_data->string, (UUtInt32*)outNumber);

	return error;
}

// ----------------------------------------------------------------------
static UUtUns32
WMiEditField_HandleGetText(
	WMtEditField				*inEditField,
	char						*outStringBuffer,
	UUtUns32					inStringBufferLength)
{
	WMtEditField_PrivateData	*private_data;
	UUtUns32					string_length;
	
	private_data = (WMtEditField_PrivateData*)WMrWindow_GetLong(inEditField, 0);
	if (private_data == NULL) { return 0; }
	if (private_data->string == NULL) { return 0; }
	
	string_length = TSrString_GetLength(private_data->string);
	UUrString_Copy(outStringBuffer, private_data->string, inStringBufferLength);
	
	return string_length;
}

// ----------------------------------------------------------------------
static void
WMiEditField_HandleKeyDown(
	WMtEditField				*inEditField,
	UUtUns16					inKey)
{
	WMtEditField_PrivateData	*private_data;
	UUtUns32					style;
	UUtUns16					string_length;
	UUtBool						notify_parent;
	UUtBool						inserted;
	
	// disabled edit fields don't handle key events
	if (WMrWindow_GetEnabled(inEditField) == UUcFalse) { return; }
	
	private_data = (WMtEditField_PrivateData*)WMrWindow_GetLong(inEditField, 0);
	if (private_data == NULL) { return; }
	if (private_data->string == NULL) { return; }
	
	style = WMrWindow_GetStyle(inEditField);
	
	notify_parent = UUcTrue;
	string_length = TSrString_GetLength(private_data->string);	
	
	switch (inKey)
	{
		case LIcKeyCode_Tab:
		{
			UUtUns32				i;
			
			if (style & WMcEditFieldStyle_NumbersOnly) { break; }
			
			// insert 4 spaces
			for (i = 0; i < 4; i ++)
			{
				inserted = 
					TSrString_InsertChar(
						private_data->string,
						(UUtUns16)private_data->insert_before,
						' ',
						private_data->max_chars);
				if (inserted)
				{
					private_data->insert_before++;
				}
			}
		}
		break;
		
		case LIcKeyCode_BackSpace:
			if (private_data->insert_before > 0)
			{
				TSrString_DeleteChar(
					private_data->string,
					(private_data->insert_before - 1),
					private_data->max_chars);
				
				private_data->insert_before--;
			}
		break;
		
		case LIcKeyCode_Delete:	/* forward delete */
			TSrString_DeleteChar(
				private_data->string,
				(UUtUns16)private_data->insert_before,
				private_data->max_chars);
		break;
		
		case LIcKeyCode_Return:
		case LIcKeyCode_PageUp:
		case LIcKeyCode_PageDown:
		case LIcKeyCode_Home:
		case LIcKeyCode_End:
		case LIcKeyCode_Insert:
		case LIcKeyCode_UpArrow:
		case LIcKeyCode_DownArrow:
			notify_parent = UUcFalse;
		break;
		
		case LIcKeyCode_RightArrow:
			if (private_data->insert_before < string_length)
			{
				private_data->insert_before++;
			}
			notify_parent = UUcFalse;
		break;
		
		case LIcKeyCode_LeftArrow:
			if (private_data->insert_before > 0)
			{
				private_data->insert_before--;
			}
			notify_parent = UUcFalse;
		break;
		
		default:
			// check the numbers only style
			if ((style & WMcEditFieldStyle_NumbersOnly) &&
				(isdigit(inKey) == UUcFalse) &&
				(inKey != LIcKeyCode_Decimal) &&
				(inKey != LIcKeyCode_Period) &&
				(inKey != LIcKeyCode_Subtract) &&
				(inKey != LIcKeyCode_Minus))
			{
				notify_parent = UUcFalse;
				break;
			}
			
			// insert the key into the string
			inserted =
				TSrString_InsertChar(
					private_data->string,
					(UUtUns16)private_data->insert_before,
					inKey,
					private_data->max_chars);
			if (inserted)
			{
				private_data->insert_before++;
			}
		break;
	}
	
	// update the caret position
	WMiEditField_SetCaretPosition(inEditField, private_data, NULL);

	if (notify_parent)
	{
		// tell the parent that the editfield's value has changed
		WMrMessage_Send(
			WMrWindow_GetParent(inEditField),
			WMcMessage_Command,
			UUmMakeLong(EFcNotify_ContentChanged, WMrWindow_GetID(inEditField)),
			(UUtUns32)inEditField);
	}
}

// ----------------------------------------------------------------------
static void
WMiEditField_HandleMouseEvent(
	WMtEditField				*inEditField,
	UUtUns32					inParam1)
{
	WMtEditField_PrivateData	*private_data;
	IMtPoint2D					global_mouse;
	IMtPoint2D					local_mouse;
	
	// get the private data
	private_data = (WMtEditField_PrivateData*)WMrWindow_GetLong(inEditField, 0);
	if (private_data == NULL) { return; }
	
	// get the mouse in local coordinates
	global_mouse.x = (UUtInt16)UUmHighWord(inParam1);
	global_mouse.y = (UUtInt16)UUmLowWord(inParam1);
	WMrWindow_GlobalToLocal(inEditField, &global_mouse, &local_mouse);
	
	// set the position of the cursor
	if (private_data->editfield_data & WMcEditFieldFlag_FocusClick)
	{
		private_data->editfield_data &= ~WMcEditFieldFlag_FocusClick;
	}
	else
	{
		WMiEditField_SetInsertBeforeFromPoint(inEditField, private_data, &local_mouse);
	}
	
	// notify the parent of the click
	WMrMessage_Send(
		WMrWindow_GetParent(inEditField),
		WMcMessage_Command,
		UUmMakeLong(WMcNotify_Click, WMrWindow_GetID(inEditField)),
		(UUtUns32)inEditField);
}

// ----------------------------------------------------------------------
static void
WMiEditField_HandleSetFocus(
	WMtEditField				*inEditField,
	UUtBool						inHasFocus)
{
	WMtEditField_PrivateData	*private_data;
	
	// get the private data
	private_data = (WMtEditField_PrivateData*)WMrWindow_GetLong(inEditField, 0);
	if (private_data == NULL) { return; }
	
	if (inHasFocus)
	{
		private_data->editfield_data |= (WMcEditFieldFlag_HasFocus | WMcEditFieldFlag_FocusClick);
		
		// initialize the caret
		WMrCaret_Create(
			inEditField,
			1,
			(UUtInt16)(DCrText_GetAscendingHeight() + DCrText_GetDescendingHeight()));
		WMrCaret_SetVisible(UUcTrue);
		WMiEditField_SetCaretPosition(inEditField, private_data, NULL);
	}
	else
	{
		private_data->editfield_data &= ~WMcEditFieldFlag_HasFocus;
		
		// destroy the caret
		WMrCaret_Destroy();
	}
}

// ----------------------------------------------------------------------
static UUtError
WMiEditField_HandleSetMaxChars(
	WMtEditField				*inEditField,
	UUtUns32					inMaxChars)
{
	WMtEditField_PrivateData	*private_data;
	char						*new_string;
	
	// get the private data
	private_data = (WMtEditField_PrivateData*)WMrWindow_GetLong(inEditField, 0);
	if (private_data == NULL) { return UUcError_None; }
	
	// set the new number of maximum characters
	private_data->max_chars = (UUtUns16)inMaxChars;
	
	// allocate memory for the string
	new_string = UUrMemory_Block_NewClear(private_data->max_chars + 1);
	UUmError_ReturnOnNull(new_string);
	
	// copy the data from the old string to the new string
	UUrString_Copy(new_string, private_data->string, (private_data->max_chars + 1));
	
	// delete the old string
	UUrMemory_Block_Delete(private_data->string);
	
	// set the string to the new string
	private_data->string = new_string;
	
	return UUcError_None;	
}

// ----------------------------------------------------------------------
static void
WMiEditField_HandleSetText(
	WMtEditField				*inEditField,
	char						*inString)
{
	WMtEditField_PrivateData	*private_data;
	
	private_data = (WMtEditField_PrivateData*)WMrWindow_GetLong(inEditField, 0);
	if (private_data == NULL) { return; }
	if (private_data->string == NULL) { return; }
	
	UUrString_Copy(private_data->string, inString, (private_data->max_chars + 1));
	private_data->insert_before = TSrString_GetLength(private_data->string);
	WMiEditField_SetCaretPosition(inEditField, private_data, NULL);
}

// ----------------------------------------------------------------------
static void
WMiEditField_Paint(
	WMtEditField				*inEditField)
{
	DCtDrawContext				*draw_context;
	PStPartSpecUI				*partspec_ui;
	IMtPoint2D					dest;
	UUtInt16					width;
	UUtInt16					height;
	WMtEditField_PrivateData	*private_data;
	UUtBool						enabled;
	UUtBool						has_focus;
	TStFontInfo					font_info;
	
	// get the private_data
	private_data = (WMtEditField_PrivateData*)WMrWindow_GetLong(inEditField, 0);
	if (private_data == NULL) { return; }
	
	// get the active partspec ui
	partspec_ui = PSrPartSpecUI_GetActive();
	if (partspec_ui == NULL) { return; }
	
	WMrWindow_GetSize(inEditField, &width, &height);
	
	draw_context = DCrDraw_Begin(inEditField);
	
	enabled = WMrWindow_GetEnabled(inEditField);
	has_focus = (private_data->editfield_data & WMcEditFieldFlag_HasFocus) != 0;
	
	// set dest
	dest.x = 0;
	dest.y = 0;
	
	// draw the background
	DCrDraw_PartSpec(
		draw_context,
		has_focus ? partspec_ui->ef_hasfocus : partspec_ui->editfield,
		PScPart_All,
		&dest,
		width,
		height,
		enabled ? (UUtUns16)WMcAlpha_Enabled : (UUtUns16)WMcAlpha_Disabled);
		
	dest.x = private_data->text_bounds.left;
	dest.y = private_data->text_bounds.top;
		
	WMrWindow_GetFontInfo(inEditField, &font_info);
	DCrText_SetFontInfo(&font_info);
	
	DCrText_SetShade(enabled ? font_info.font_shade : IMcShade_Gray50);
	DCrText_SetFormat(TSc_HLeft);
	dest.x += WMcEditField_Text_Buffer;
	dest.y += DCrText_GetAscendingHeight() + DCrText_GetLeadingHeight();
	
	DCrDraw_String(draw_context, private_data->string, &private_data->text_bounds, &dest);
	
	DCrDraw_End(draw_context, inEditField);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiEditField_Callback(
	WMtEditField			*inEditField,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtError				error;
	UUtUns32				result;
	
	switch(inMessage)
	{
		case WMcMessage_NC_HitTest:
		return WMcWindowArea_Client;
		
		case WMcMessage_Create:
			error = WMiEditField_Create(inEditField);
			if (error != UUcError_None)
			{
				return WMcResult_Error;
			}
		return WMcResult_Handled;
		
		case WMcMessage_Destroy:
			WMiEditField_Destroy(inEditField);
		return WMcResult_Handled;
		
		case WMcMessage_Paint:
			WMiEditField_Paint(inEditField);
		return WMcResult_Handled;
		
		case WMcMessage_KeyDown:
			WMiEditField_HandleKeyDown(inEditField, (UUtUns16)inParam1);
		return WMcResult_Handled;
		
		case WMcMessage_LMouseDown:
			if (WMrWindow_GetFocus() != inEditField)
			{
				WMrWindow_SetFocus(inEditField);
			}
		return WMcResult_Handled;
		
		case WMcMessage_LMouseUp:
			WMiEditField_HandleMouseEvent(inEditField, inParam1);
		return WMcResult_Handled;
		
		case WMcMessage_SetFocus:
			WMiEditField_HandleSetFocus(inEditField, UUcTrue);
		return WMcResult_Handled;

		case WMcMessage_KillFocus:
			WMiEditField_HandleSetFocus(inEditField, UUcFalse);
		return WMcResult_Handled;
		
		case EFcMessage_GetText:
			result = WMiEditField_HandleGetText(inEditField, (char*)inParam1, inParam2);
		return result;

		case EFcMessage_GetInt:
			result = WMiEditField_HandleGetInt(inEditField, (UUtUns32 *) inParam1);
		return result;

		case EFcMessage_GetFloat:
			result = WMiEditField_HandleGetFloat(inEditField, (float *) inParam1);
		return result;

		case EFcMessage_SetMaxChars:
			WMiEditField_HandleSetMaxChars(inEditField, inParam1);
		return WMcResult_Handled;

		case EFcMessage_SetText:
			WMiEditField_HandleSetText(inEditField, (char*)inParam1);
		return WMcResult_Handled;
		
		case WMcMessage_GetDialogCode:
			result = WMiEditField_HandleGetDialogCode(inEditField);
		return result;
	}
	
	return WMrWindow_DefaultCallback(inEditField, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrEditField_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;
	
	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_EditField;
	window_class.callback = WMiEditField_Callback;
	window_class.private_data_size = sizeof(WMtEditField_PrivateData*);
	
	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}


UUtInt32
WMrEditField_GetInt32(WMtEditField *inEditField)
{
	UUtInt32 edit_field_number;
	UUtError error;
	
	error = (UUtError) WMrMessage_Send(inEditField, EFcMessage_GetInt, (UUtUns32) &edit_field_number, 0);

	if (UUcError_None != error) {
		edit_field_number = 0;
	}

	return edit_field_number;
}

float
WMrEditField_GetFloat(WMtEditField *inEditField)
{
	float edit_field_number;
	UUtError error;
		
	error = (UUtError) WMrMessage_Send(inEditField, EFcMessage_GetFloat, (UUtUns32) &edit_field_number, 0);

	if (UUcError_None != error) {
		edit_field_number = 0.f;
	}

	return edit_field_number;
}

void
WMrEditField_SetInt32(WMtEditField *inEditField, UUtInt32 inNumber)
{
	char buffer[128];

	sprintf(buffer, "%d", inNumber);

	WMrMessage_Send(inEditField, EFcMessage_SetText, (UUtUns32) buffer, 0);

	return;
}

void
WMrEditField_SetFloat(WMtEditField *inEditField, float inFloat)
{
	char buffer[128];
	UUtUns32 len;

	sprintf(buffer, "%f", inFloat);

	if (strchr(buffer, '.') != NULL) {
		// strip trailing zeroes from this string... leave decimal point and at least one zero
		len = strlen(buffer);
		while ((len > 2) && (buffer[len - 1] == '0') && (buffer[len - 2] == '0')) {
			buffer[--len] = '\0';
		}
	}
	
	WMrMessage_Send(inEditField, EFcMessage_SetText, (UUtUns32) buffer, 0);

	return;
}

void
WMrEditField_SetText(WMtEditField *inEditField, const char *inString)
{
	WMrMessage_Send(inEditField, EFcMessage_SetText, (UUtUns32) inString, 0);

	return;
}

void
WMrEditField_GetText(WMtEditField *inEditField, char *outString, UUtUns32 inStringLength)
{
	WMrMessage_Send(inEditField, EFcMessage_GetText, (UUtUns32) outString, inStringLength);

	return;
}
