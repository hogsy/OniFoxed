// ======================================================================
// VM_View_ListBox.h
// ======================================================================
#ifndef VM_VIEW_LISTBOX_H
#define VM_VIEW_LISTBOX_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"
#include "BFW_TextSystem.h"

// ======================================================================
// defines
// ======================================================================
#define LBcInvalidIndex				-1

// ======================================================================
// typedefs
// ======================================================================
#define VMcTemplate_View_ListBox		UUm4CharToUns32('V', 'M', 'L', 'B')
typedef tm_template('V', 'M', 'L', 'B', "VM View ListBox")
VMtView_ListBox
{
	
	VMtPartSpec			*backborder;
	
	UUtUns16			flags;
	UUtUns16			reserved;
	
} VMtView_ListBox;

typedef struct VMtView_ListBox_PrivateData
{
	// text texture data
	tm_templateref		text_texture_ref;
	UUtUns16			text_texture_width;
	UUtUns16			text_texture_height;
	IMtPixel			text_background_color;
	UUtBool				text_texture_dirty;
	
	// text data
	TStTextContext		*text_context;
	UUtMemory_Array		*text_array;
	IMtPixel			text_color;
	UUtUns16			text_line_height;

	// entries
	UUtInt16			num_entries;
	UUtUns16			num_visible_entries;
	UUtInt32			current_selection;
	
	// scrollbar
	VMtView				*scrollbar;
	UUtInt32			scroll_max;
	UUtInt32			scroll_pos;
	
} VMtView_ListBox_PrivateData;

extern TMtPrivateData*	DMgTemplate_ListBox_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_ListBox_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

UUtError
VMrView_ListBox_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData);

// ======================================================================
#endif /* VM_VIEW_LISTBOX_H */