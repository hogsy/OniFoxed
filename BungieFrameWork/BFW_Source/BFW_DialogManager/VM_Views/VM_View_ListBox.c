// ======================================================================
// VM_View_ListBox.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_ViewUtilities.h"
#include "BFW_TextSystem.h"
#include "BFW_LocalInput.h"

#include "VM_View_ListBox.h"
#include "VM_View_Scrollbar.h"

// ======================================================================
// defines
// ======================================================================
#define VMcView_ListBox_MaxCharsPerEntry		64

#define VMcText_X_Offset	4
#define VMcText_Y_Offset	4

// ======================================================================
// typedefs
// ======================================================================
typedef struct VMtView_ListBox_Entry
{
	IMtPixel			color;
	char				string[VMcView_ListBox_MaxCharsPerEntry];
	
} VMtView_ListBox_Entry;

// ======================================================================
// prototypes
// ======================================================================
static UUtBool
VMiView_ListBox_Reset(
	VMtView						*inView,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
VMiView_ListBox_AdjustScrollbar(
	VMtView						*inView,
	VMtView_ListBox_PrivateData	*inPrivateData)
{
	// calculate the scroll max and pos
	inPrivateData->scroll_max =
		UUmMax(
			0,
			inPrivateData->num_entries -
			inPrivateData->num_visible_entries);
	inPrivateData->scroll_pos =
		UUmMin(inPrivateData->scroll_pos, inPrivateData->scroll_max);
	
	// set the scrollbar range and position
	VMrView_Scrollbar_SetRange(inPrivateData->scrollbar, 0, inPrivateData->scroll_max);
	VMrView_Scrollbar_SetPosition(inPrivateData->scrollbar, inPrivateData->scroll_pos);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
VMiView_ListBox_Create(
	VMtView						*inView,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData)
{
	UUtError					error;
	UUtUns16					i;
	UUtUns16					width;
	UUtUns16					height;
	
	// ------------------------------
	// set up the scrollbar view
	// ------------------------------
	inPrivateData->scrollbar = NULL;
	for (i = 0; i < inView->num_child_views; i++)
	{
		VMtView	*child = (VMtView*)inView->child_views[i].view_ref;
		
		if (child->type == VMcViewType_Scrollbar)
		{
			inPrivateData->scrollbar = child;
			break;
		}
	}
	
	if (inPrivateData->scrollbar)
	{
		IMtPoint2D				new_location;
		
		// set the location and height of the scrollbar
		new_location.x = inView->width - inPrivateData->scrollbar->width;
		new_location.y = 0;
		VMrView_SetLocation(inPrivateData->scrollbar, &new_location);
		VMrView_SetSize(inPrivateData->scrollbar, inPrivateData->scrollbar->width, inView->height);
	}
	
	// ------------------------------
	// set the colors
	// ------------------------------
	inPrivateData->text_background_color = IMrPixel_FromShade(VMcControl_PixelType, VMcColor_Background);
	inPrivateData->text_color = IMrPixel_FromShade(VMcControl_PixelType, VMcColor_Text);
		
	// ------------------------------
	// create the text texture
	// ------------------------------
	if (inPrivateData->text_texture_ref == NULL)
	{
		// calculate the width, and height
		width = inView->width - 4;
		height = inView->height - 4;
		
		if (inPrivateData->scrollbar)
		{
			width -= (inPrivateData->scrollbar->width + 2);
		}
		
		// create the texture
		error =
			VUrCreate_Texture(
				width,
				height,
				&inPrivateData->text_texture_ref);
		UUmError_ReturnOnErrorMsg(error, "Unable to create the texture");
	
		// save the location, width, and height
		inPrivateData->text_texture_width = width;
		inPrivateData->text_texture_height = height;
	}
	
	// ------------------------------
	// create the text context
	// ------------------------------
	if (inPrivateData->text_context == NULL)
	{
		TStFontFamily		*font_family;
		char				*desired_font_family;
		TStFontStyle		desired_font_style;
		UUtUns16			desired_font_format;
		
		// interpret the flags
		if (inListBox->flags & VMcCommonFlag_Text_Small)
			desired_font_family = TScFontFamily_RomanSmall;
		else
			desired_font_family = TScFontFamily_Roman;
		
		if (inListBox->flags & VMcCommonFlag_Text_Bold)
			desired_font_style = TScStyle_Bold;
		else if (inListBox->flags & VMcCommonFlag_Text_Italic)
			desired_font_style = TScStyle_Italic;
		else
			desired_font_style = TScStyle_Plain;

		if (inListBox->flags & VMcCommonFlag_Text_HCenter)
			desired_font_format = TSc_HCenter;
		else if (inListBox->flags & VMcCommonFlag_Text_HRight)
			desired_font_format = TSc_HRight;
		else
			desired_font_format = TSc_HLeft;

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
		error = TSrContext_SetColor(inPrivateData->text_context, inPrivateData->text_color);
		UUmError_ReturnOnErrorMsg(error, "Unable to set the text color");
		
		// get the height of the text
		inPrivateData->text_line_height =
			TSrFont_GetLineHeight(TSrContext_GetFont(inPrivateData->text_context, TScStyle_InUse));
	}
	
	// ------------------------------
	// allocate the text array
	// ------------------------------
	if (inPrivateData->text_array == NULL)
	{
		// allocate the array
		inPrivateData->text_array =
			UUrMemory_Array_New(
				sizeof(VMtView_ListBox_Entry),
				sizeof(VMtView_ListBox_Entry),
				0,
				0);
		UUmError_ReturnOnNull(inPrivateData->text_array);
	}
	
	// ------------------------------
	// setup the scrollbar
	// ------------------------------
	inPrivateData->scroll_pos = 0;
	inPrivateData->scroll_max = 0;
	inPrivateData->num_visible_entries = 
		(inPrivateData->text_texture_height / inPrivateData->text_line_height);
	VMiView_ListBox_AdjustScrollbar(inView, inPrivateData);
	
	// ------------------------------
	// initialize the other vars
	// ------------------------------
	inPrivateData->current_selection = -1;
	inPrivateData->text_texture_dirty = UUcTrue;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
VMiView_ListBox_Destroy(
	VMtView						*inView,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData)
{
	VMiView_ListBox_Reset(inView, inListBox, inPrivateData);
}

// ----------------------------------------------------------------------
static void
VMiView_ListBox_Paint(
	VMtView						*inView,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData,
	M3tPointScreen				*inDestination)
{
	UUtUns16					alpha;
	
	// set the alpha
	if (inView->flags & VMcViewFlag_Enabled)
		alpha = VUcAlpha_Enabled;
	else
		alpha = VUcAlpha_Disabled;
	
	// draw the border and background
	if (inListBox->backborder)
	{
		UUtUns16				width;
		
		width = inView->width;
		if (inPrivateData->scrollbar)
		{
			width -= (inPrivateData->scrollbar->width + 2);
		}
	
		VUrDrawPartSpecification(
			inListBox->backborder,
			VMcPart_All,
			inDestination,
			width,
			inView->height,
			alpha);
	}
	
	// draw the text
	if (inPrivateData->text_texture_ref)
	{
		M3tPointScreen			dest;
		
		dest = *inDestination;
		dest.x += 2.0f;
		dest.y += 2.0f;
		
		VUrDrawTextureRef(
			inPrivateData->text_texture_ref,
			&dest,
			inPrivateData->text_texture_width,
			inPrivateData->text_texture_height,
			alpha);
	}
}

// ----------------------------------------------------------------------
static void
VMiView_ListBox_Update(
	VMtView						*inView,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData)
{
	UUtInt32					start_index;
	UUtInt32					stop_index;
	UUtUns16					draw_line;
	UUtUns16					line_height;
	UUtUns16					descending_height;
	TStFont						*font;
	UUtRect						bounds;
	VMtView_ListBox_Entry		*array_pointer;
	UUtInt32					i;
	IMtPoint2D					dest;
	
	// get the 
	font = TSrContext_GetFont(inPrivateData->text_context, TScStyle_InUse);
	line_height = TSrFont_GetLineHeight(font);
	descending_height = TSrFont_GetDescendingHeight(font);
	
	// set the bounds to draw in
	bounds.left = 0;
	bounds.top = 0;
	bounds.right = inPrivateData->text_texture_width;
	bounds.bottom = inPrivateData->text_texture_height;
	
	// get the array pointer
	array_pointer = 
		(VMtView_ListBox_Entry*)UUrMemory_Array_GetMemory(inPrivateData->text_array);
	
	// erase the texture
	if ((inView->width > M3cTextureMap_MaxWidth) ||
		(inView->height > M3cTextureMap_MaxHeight))
	{
		M3rTextureMap_Big_Fill(
			inPrivateData->text_texture_ref,
			inPrivateData->text_background_color,
			&bounds);
	}
	else
	{
		M3rTextureMap_Fill(
			inPrivateData->text_texture_ref,
			inPrivateData->text_background_color,
			&bounds);
	}

	// calculate the start and stop index
	start_index = UUmMax(0, inPrivateData->scroll_pos);
	stop_index =
		UUmMin(
			inPrivateData->num_entries,
			(inPrivateData->scroll_pos +
			inPrivateData->num_visible_entries));
	
	// draw the lines of text
	for (i = start_index, draw_line = 1; i < stop_index; i++, draw_line++)
	{
		// calculate the destination
		dest.x = VMcText_X_Offset;
		dest.y = (draw_line * line_height) - descending_height + VMcText_Y_Offset;
		
		// set the text color
		TSrContext_SetColor(inPrivateData->text_context, array_pointer[i].color);
		
		// draw the string
		TSrContext_DrawString(
			inPrivateData->text_context,
			inPrivateData->text_texture_ref,
			array_pointer[i].string,
			&bounds,
			&dest);
	}

	// draw the selected line of text
	if ((inPrivateData->current_selection >= start_index) &&
		(inPrivateData->current_selection < stop_index))
	{
		UUtRect				invert_bounds;
		
		// calculate the draw_line
		draw_line = inPrivateData->current_selection - start_index + 1;
				
		// calculate the invert bounds
		invert_bounds.left = 0;
		invert_bounds.top = ((draw_line - 1) * line_height) + VMcText_Y_Offset + 1;
		invert_bounds.right = invert_bounds.left + inPrivateData->text_texture_width;
		invert_bounds.bottom = invert_bounds.top + inPrivateData->text_line_height;
		IMrRect_Inset(&invert_bounds, VMcText_X_Offset, 0);
		
		// draw the hilight box
		if ((inView->width > M3cTextureMap_MaxWidth) ||
			(inView->height > M3cTextureMap_MaxHeight))
		{
			M3rTextureMap_Big_Fill(
				inPrivateData->text_texture_ref,
				inPrivateData->text_color,
				&invert_bounds);
		}
		else
		{
			M3rTextureMap_Fill(
				inPrivateData->text_texture_ref,
				inPrivateData->text_color,
				&invert_bounds);
		}
		
		// calculate the destination
		dest.x = VMcText_X_Offset;
		dest.y = (draw_line * line_height) - descending_height + VMcText_Y_Offset;
		
		// set the text color
		TSrContext_SetColor(inPrivateData->text_context, inPrivateData->text_background_color);
		
		// draw the string
		TSrContext_DrawString(
			inPrivateData->text_context,
			inPrivateData->text_texture_ref,
			array_pointer[inPrivateData->current_selection].string,
			&bounds,
			&dest);
	
		// reset the text color
		TSrContext_SetColor(inPrivateData->text_context, inPrivateData->text_color);
	}
	
	// clear the dirty field
	inPrivateData->text_texture_dirty = UUcFalse;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
VMiView_ListBox_DeleteString(
	VMtView						*inView,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData,
	UUtInt32					inIndex)
{
	// check the range
	if ((inIndex < 0) || (inIndex >= inPrivateData->num_entries)) return UUcFalse;
	
	// delete the element
	UUrMemory_Array_DeleteElement(inPrivateData->text_array, inIndex);

	// adjust the scrollbar
	VMiView_ListBox_AdjustScrollbar(inView, inPrivateData);

	// update the textures at a later time
	inPrivateData->text_texture_dirty = UUcTrue;
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
VMiView_ListBox_GetText(
	VMtView_ListBox_PrivateData	*inPrivateData,
	UUtInt32					inIndex,
	char						*ioString)
{
	UUtInt32					index;
	VMtView_ListBox_Entry		*array_pointer;
	
	// set the index
	if (inIndex == -1)
		index = inPrivateData->current_selection;
	else
		index = inIndex;
	
	// check the bounds on the index
	if ((index < 0) || (index >= inPrivateData->num_entries)) return UUcFalse;
	
	// get the array pointer
	array_pointer = 
		(VMtView_ListBox_Entry*)UUrMemory_Array_GetMemory(inPrivateData->text_array);
	
	// assume that ioString is big enough to hold the contents of the array
	UUrString_Copy_Length(
		ioString,
		array_pointer[index].string,
		VMcView_ListBox_MaxCharsPerEntry,
		TSrString_GetLength(array_pointer[index].string) + 1);
	
	return UUcTrue;
}
// ----------------------------------------------------------------------
static UUtUns16
VMiView_ListBox_GetTextLength(
	VMtView_ListBox_PrivateData	*inPrivateData,
	UUtInt32					inIndex)
{
	UUtInt32					index;
	VMtView_ListBox_Entry		*array_pointer;
	UUtUns16					string_length;
	
	// set the index
	if (inIndex == -1)
		index = inPrivateData->current_selection;
	else
		index = inIndex;
	
	// check the bounds on the index
	if ((index < 0) || (index >= inPrivateData->num_entries)) return 0;
	
	// get the array pointer
	array_pointer = 
		(VMtView_ListBox_Entry*)UUrMemory_Array_GetMemory(inPrivateData->text_array);
	
	// assume that ioString is big enough to hold the contents of the array
	string_length = TSrString_GetLength(array_pointer[index].string);
	
	return string_length;
}

// ----------------------------------------------------------------------
static UUtUns32
VMiView_ListBox_ReplaceString(
	VMtView						*inView,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData *inPrivateData,
	UUtInt32					inIndex,
	char						*inString)
{
	WMtListBox_Item			*items;
	VMtView_ListBox_Entry	*array_pointer;
	
	// CB: check that we are within the bounds of the array
	if ((inIndex < 0) || (inIndex >= inPrivateData->num_entries)) {
		return LBcInvalidIndex;
	}

	// get the array pointer
	array_pointer = 
		(VMtView_ListBox_Entry*)UUrMemory_Array_GetMemory(inPrivateData->text_array);

	// copy the string into the entry
	UUrString_Copy(
		array_pointer[inIndex].string,
		inString,
		VMcView_ListBox_MaxCharsPerEntry);
	
	// set the default color
	array_pointer[inIndex].color = inPrivateData->text_color;
	
	// update the textures at a later time
	inPrivateData->text_texture_dirty = UUcTrue;
	
	return (UUtInt32) inIndex;
}

// ----------------------------------------------------------------------
static UUtInt32
VMiView_ListBox_InsertString(
	VMtView						*inView,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData,
	UUtInt32					inIndex,
	char						*inString)
{
	UUtError					error;
	UUtBool						mem_moved;
	VMtView_ListBox_Entry		*array_pointer;
	UUtInt32					index;
	
	// make room in the array
	if (inIndex != -1)
	{
		// insert the element into the array
		error =
			UUrMemory_Array_InsertElement(
				inPrivateData->text_array,
				inIndex,
				&mem_moved);
		if (error != UUcError_None) return LBcInvalidIndex;
		
		index = inIndex;
	}
	else
	{
		// add the element to the array
		error =
			UUrMemory_Array_GetNewElement(
				inPrivateData->text_array,
				(UUtUns32*)&index,
				&mem_moved);
		if (error != UUcError_None) return LBcInvalidIndex;
	}
	
	// update the number of entries
	inPrivateData->num_entries++;
	
	// get the array pointer
	array_pointer = 
		(VMtView_ListBox_Entry*)UUrMemory_Array_GetMemory(inPrivateData->text_array);

	// copy the string into the entry
	UUrString_Copy(
		array_pointer[index].string,
		inString,
		VMcView_ListBox_MaxCharsPerEntry);
	
	// set the default color
	array_pointer[index].color = inPrivateData->text_color;
	
	// adjust the scrollbar
	VMiView_ListBox_AdjustScrollbar(inView, inPrivateData);

	// update the textures at a later time
	inPrivateData->text_texture_dirty = UUcTrue;
	
	return (UUtInt32)index;
}

// ----------------------------------------------------------------------
static UUtBool
VMiView_ListBox_Reset(
	VMtView						*inView,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData)
{
	UUtUns32					num_elements;
	
	// clear the array
	num_elements = UUrMemory_Array_GetUsedElems(inPrivateData->text_array);
	if (num_elements == 0)
	{
		inPrivateData->num_entries = 0;
		return UUcTrue;
	}
	
	// delete all the elements in the array
	while (num_elements--)
	{
		UUrMemory_Array_DeleteElement(inPrivateData->text_array, num_elements);
	}
	
	// no more entries
	inPrivateData->num_entries = 0;
	
	// adjust the scrollbar
	VMiView_ListBox_AdjustScrollbar(inView, inPrivateData);

	// update the textures at a later time
	inPrivateData->text_texture_dirty = UUcTrue;
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
VMiView_ListBox_SetSelection(
	VMtView						*inView,
	VMtView_PrivateData			*inViewPrivateData,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData,
	UUtInt32					inIndex,
	UUtBool						inNotifyParent)
{
	UUtInt32					diff;
	
	// clear the selection
	if (inIndex == -1)
	{
		// set the selection
		inPrivateData->current_selection = inIndex;

		// update the textures at a later time
		inPrivateData->text_texture_dirty = UUcTrue;
		
		return UUcTrue;
	}
	
	// check the bounds on the index
	if ((inIndex < 0) || (inIndex >= inPrivateData->num_entries)) return UUcFalse;
	
	// set the selection
	inPrivateData->current_selection = inIndex;
	
	// update the textures at a later time
	inPrivateData->text_texture_dirty = UUcTrue;
	
	// adjust the list to see the newly selected
	diff = inPrivateData->scroll_pos - (UUtInt32)inPrivateData->current_selection;
	if (diff > 0)
	{
		// current selection is above the scroll pos in the list
		if (((UUtInt32)inPrivateData->current_selection - diff) < inPrivateData->scroll_pos) 
		{
			// the scroll position needs to be changed to make the 
			// current selection visible
			inPrivateData->scroll_pos = (UUtInt32)inPrivateData->current_selection;
		}
	}
	else if (diff < 0)
	{
		// current selection is below the scroll pos in the list
		if ((UUtUns16)UUmABS(diff) >= inPrivateData->num_visible_entries)
		{
			inPrivateData->scroll_pos =
				(UUtInt32)inPrivateData->current_selection -
				(UUtInt32)inPrivateData->num_visible_entries +
				1;
		}
	}
	
	if (inNotifyParent)
	{
		// tell the parent
		VMrView_SendMessage(
			inViewPrivateData->parent,
			VMcMessage_Command,
			UUmMakeLong(LBcNotify_SelectionChanged, inView->id),
			(UUtUns32)inView);
	}
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
VMiView_ListBox_SetLineColor(
	VMtView						*inView,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData,
	UUtInt32					inIndex,
	IMtPixel					*inPixel)
{
	if (inIndex == -1)
	{
		inPrivateData->text_color = *inPixel;
	}
	else
	{
		VMtView_ListBox_Entry		*array_pointer;

		// check the bounds on the index
		if ((inIndex < 0) || (inIndex >= inPrivateData->num_entries))
		{
			return UUcFalse;
		}

		// get the array pointer
		array_pointer = 
			(VMtView_ListBox_Entry*)UUrMemory_Array_GetMemory(inPrivateData->text_array);
		
		// set the text color of the line
		array_pointer[inIndex].color = *inPixel;
	}
	
	return UUcTrue;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
VMiView_ListBox_HandleKeyDown(
	VMtView						*inView,
	VMtView_PrivateData			*inViewPrivateData,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData,
	UUtUns16					inKey)
{
	UUtBool						handled;
	UUtInt32					new_selection;
	
	handled = UUcTrue;
	
	switch (inKey)
	{
		case LIcKeyCode_UpArrow:
			new_selection = inPrivateData->current_selection - 1;
			if (new_selection >= 0)
			{
				VMiView_ListBox_SetSelection(
					inView,
					inViewPrivateData,
					inListBox,
					inPrivateData,
					new_selection,
					UUcTrue);
			}
		break;
		
		case LIcKeyCode_DownArrow:
			new_selection = inPrivateData->current_selection + 1;
			if (new_selection < (UUtInt32)inPrivateData->num_entries)
			{
				VMiView_ListBox_SetSelection(
					inView,
					inViewPrivateData,
					inListBox,
					inPrivateData,
					new_selection,
					UUcTrue);
			}			
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
static void
VMiView_ListBox_HandleLMouseUp(
	VMtView						*inView,
	VMtView_PrivateData			*inViewPrivateData,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData,
	IMtPoint2D					*inLocalPoint)
{	
	UUtInt32					selected_entry;
	
	// the point is in local coordinates so check to see
	// if it is withing the text display area
	if ((inLocalPoint->x > inPrivateData->text_texture_width) ||
		(inLocalPoint->y > inPrivateData->text_texture_height))
	{
		// the point is outside the text display area
		return;
	}
	
	// figure out which entry the user clicked on
	selected_entry =
		((inLocalPoint->y - VMcText_Y_Offset) / inPrivateData->text_line_height) +
		(UUtInt16)UUmMax(0, inPrivateData->scroll_pos);
	
	// set the selected entry
	if ((selected_entry < 0) || (selected_entry >= inPrivateData->num_entries))
	{
		VMiView_ListBox_SetSelection(
			inView,
			inViewPrivateData,
			inListBox,
			inPrivateData,
			-1,
			UUcTrue);
	}
	else
	{
		VMiView_ListBox_SetSelection(
			inView,
			inViewPrivateData,
			inListBox,
			inPrivateData,
			selected_entry,
			UUcTrue);
	}

	// tell the parent
/*	VMrView_SendMessage(
		inViewPrivateData->parent,
		VMcMessage_Command,
		UUmMakeLong(VMcNotify_Click, inView->id),
		(UUtUns32)inView);*/
}

// ----------------------------------------------------------------------
static void
VMiView_ListBox_HandleVerticalScroll(
	VMtView						*inView,
	VMtView_ListBox				*inListBox,
	VMtView_ListBox_PrivateData	*inPrivateData,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtInt32					scroll_increment;
	
	// interpret the parameters of the message
	switch (UUmLowWord(inParam1))
	{
		case SBcNotify_LineUp:
			scroll_increment = -1;
		break;
		
		case SBcNotify_LineDown:
			scroll_increment = 1;
		break;
		
		case SBcNotify_PageUp:
			scroll_increment =
				UUmMin(
					-1,
					-inPrivateData->text_texture_height / inPrivateData->text_line_height);
		break;
		
		case SBcNotify_PageDown:
			scroll_increment =
				UUmMax(
					1,
					inPrivateData->text_texture_height / inPrivateData->text_line_height);
		break;
		
		case SBcNotify_ThumbPosition:
			scroll_increment = inParam2 - inPrivateData->scroll_pos;
		break;
		
		default:
			scroll_increment = 0;
		break;
	}
	
	// calculat the increment
	scroll_increment =
		UUmMax(
			-inPrivateData->scroll_pos,
			UUmMin(
				scroll_increment,
				inPrivateData->scroll_max - inPrivateData->scroll_pos));
	
	// adjust the the thumb position and redraw the texture
	if (scroll_increment != 0)
	{
		inPrivateData->scroll_pos += scroll_increment;
		VMrView_Scrollbar_SetPosition(inPrivateData->scrollbar, inPrivateData->scroll_pos);

		// update the textures at a later time
		inPrivateData->text_texture_dirty = UUcTrue;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
VMrView_ListBox_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	VMtView_PrivateData				*view_private_data;
	VMtView_ListBox					*listbox;
	VMtView_ListBox_PrivateData		*private_data;
	UUtBool							result;
	UUtInt32						index;
	
	// get the data
	view_private_data = (VMtView_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_View_PrivateData, inView);
	UUmAssert(view_private_data);
	listbox = (VMtView_ListBox*)inView->view_data;
	private_data = (VMtView_ListBox_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_ListBox_PrivateData, listbox);
	UUmAssert(private_data);
	
	switch (inMessage)
	{
		case VMcMessage_Create:
			VMiView_ListBox_Create(inView, listbox, private_data);
		return 0;
		
		case VMcMessage_Destroy:
			VMiView_ListBox_Destroy(inView, listbox, private_data);
		return 0;
		
		case VMcMessage_KeyDown:
			result =
				VMiView_ListBox_HandleKeyDown(
					inView,
					view_private_data,
					listbox,
					private_data,
					(UUtUns16)inParam1);
		return (UUtUns32)result;
		
		case VMcMessage_LMouseDown:
		return 0;
		
		case VMcMessage_LMouseUp:
		{
			IMtPoint2D			global_mouse_loc;
			IMtPoint2D			local_mouse_loc;
			
			// get the mouse location in local coordinates
			global_mouse_loc.x = (UUtInt16)UUmHighWord(inParam1);
			global_mouse_loc.y = (UUtInt16)UUmLowWord(inParam1);
			VMrView_PointGlobalToView(inView, &global_mouse_loc, &local_mouse_loc);

			VMiView_ListBox_HandleLMouseUp(
				inView,
				view_private_data,
				listbox,
				private_data,
				&local_mouse_loc);
		}
		return 0;
		
		case VMcMessage_LMouseDblClck:
			// tell the parent
			VMrView_SendMessage(
				view_private_data->parent,
				VMcMessage_Command,
				UUmMakeLong(VMcNotify_DoubleClick, inView->id),
				(UUtUns32)inView);
		return 0;
		
		case VMcMessage_Paint:
			// update the texture if it is dirty
			if (private_data->text_texture_dirty)
			{
				VMiView_ListBox_Update(inView, listbox, private_data);
			}
			
			// draw the button
			VMiView_ListBox_Paint(
				inView,
				listbox,
				private_data,
				(M3tPointScreen*)inParam2);
		break;
		
		case LBcMessage_AddString:
			index =
				VMiView_ListBox_InsertString(
					inView,
					listbox,
					private_data,
					-1,
					(char*)inParam2);
		return (UUtUns32)index;
		
		case LBcMessage_ReplaceString:
			result =
				VMiView_ListBox_ReplaceString(
					inview,
					listbox,
					private_data,
					(UUtInt32)inParam2,
					(char*)inParam1);
			return result;

		case LBcMessage_DeleteString:
			result =
				VMiView_ListBox_DeleteString(
					inView,
					listbox,
					private_data,
					(UUtInt32)inParam1);
		return (UUtUns32)result;
		
		case LBcMessage_InsertString:
			index = 
				VMiView_ListBox_InsertString(
					inView,
					listbox,
					private_data,
					(UUtInt32)inParam1,
					(char*)inParam2);
		return (UUtUns32)index;
		
		case LBcMessage_GetNumLines:
		return (UUtUns32)private_data->num_entries;
		
		case LBcMessage_GetCurrentSelection:
		return (UUtUns32)private_data->current_selection;
		
		case LBcMessage_GetText:
			result =
				VMiView_ListBox_GetText(
				private_data,
				(UUtInt32)inParam1,
				(char*)inParam2);
		return (UUtUns32)result;
		
		case LBcMessage_GetTextLength:
		return (UUtUns32)VMiView_ListBox_GetTextLength(private_data, (UUtInt32)inParam1);
		
		case LBcMessage_Reset:
			VMiView_ListBox_Reset(inView, listbox, private_data);
		return 0;
		
		case LBcMessage_SetSelection:
			result =
				VMiView_ListBox_SetSelection(
					inView,
					view_private_data,
					listbox,
					private_data,
					(UUtInt32)inParam1,
					UUcFalse);
		return (UUtUns32)result;
		
		case LBcMessage_SetLineColor:
			result =
				VMiView_ListBox_SetLineColor(
					inView,
					listbox,
					private_data,
					(UUtInt32)inParam1,
					(IMtPixel*)inParam2);
		return (UUtUns32)result;
		
		case SBcMessage_VerticalScroll:
			VMiView_ListBox_HandleVerticalScroll(
				inView,
				listbox,
				private_data,
				inParam1,
				inParam2);
		return 0;
	}
	
	return VMrView_DefaultCallback(inView, inMessage, inParam1, inParam2);
}

// ----------------------------------------------------------------------
UUtError
VMrView_ListBox_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	VMtView_ListBox				*listbox;
	VMtView_ListBox_PrivateData	*private_data;
	
	// get the data
	listbox = (VMtView_ListBox*)inInstancePtr;
	private_data = (VMtView_ListBox_PrivateData*)inPrivateData;
	UUmAssert(private_data);
	
	switch (inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// initalize the private data
			private_data->text_texture_ref			= NULL;
			private_data->text_texture_dirty		= UUcTrue;
			private_data->text_context				= NULL;
			private_data->text_array				= NULL;
			private_data->num_entries				= 0;
			private_data->current_selection			= -1;
			private_data->scrollbar					= NULL;
			private_data->scroll_max				= 0;
			private_data->scroll_pos				= 0;
			private_data->num_visible_entries		= 0;
		break;
		
		case TMcTemplateProcMessage_DisposePreProcess:
			if (private_data->text_context)
			{
				TSrContext_Delete(private_data->text_context);
				private_data->text_context = NULL;
			}
			
			if (private_data->text_array)
			{
				UUrMemory_Array_Delete(private_data->text_array);
				private_data->text_array = NULL;
				private_data->num_entries = 0;
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

