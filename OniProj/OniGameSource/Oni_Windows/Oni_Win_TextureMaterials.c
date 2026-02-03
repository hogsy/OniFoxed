#if TOOL_VERSION
// ======================================================================
// Oni_Win_TextureMaterials.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"
#include "WM_PopupMenu.h"
#include "WM_Picture.h"

#include "Oni_Windows.h"
#include "Oni_TextureMaterials.h"
#include "Oni_Win_TextureMaterials.h"
#include "Oni_Win_ImpactEffects.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	OWcTtM_LB_Textures				= 100,
	OWcTtM_PT_Picture				= 101,
	OWcTtM_LB_Materials				= 102
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OWiTtM_InitDialog(
	WMtDialog					*inDialog)
{
	M3tTextureMap				**texture_list;
	UUtUns32					num_textures;
	UUtError					error;
	UUtUns32					i;
	WMtWindow					*listbox;

	// get the number of textures
	num_textures = TMrInstance_GetTagCount(M3cTemplate_TextureMap);
	if (num_textures == 0) { return UUcError_Generic; }

	// allocate memory to hold the list of textures
	texture_list = (M3tTextureMap**)UUrMemory_Block_New(sizeof(M3tTextureMap*) * num_textures);
	if (texture_list == NULL) { return UUcError_Generic; }

	WMrDialog_SetUserData(inDialog, (UUtUns32)texture_list);

	// get a list of the textures
	error =
		TMrInstance_GetDataPtr_List(
			M3cTemplate_TextureMap,
			num_textures,
			&num_textures,
			texture_list);
	UUmError_ReturnOnError(error);

	// put the textures into the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcTtM_LB_Textures);
	for (i = 0; i < num_textures; i++)
	{
		WMrMessage_Send(listbox, LBcMessage_AddString, i, 0);
	}

	// select the first item in the list
	WMrListBox_SetSelection(listbox, UUcTrue, 0);

	// fill the materials list
	listbox = WMrDialog_GetItemByID(inDialog, OWcTtM_LB_Materials);
	OWrIE_FillMaterialListBox(inDialog, listbox);

	// start a timer with a 1 minute frequency
	WMrTimer_Start(0, (60 * 60), inDialog);

	// set the focus
	WMrWindow_SetFocus(listbox);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OWiTtM_Destroy(
	WMtDialog					*inDialog)
{
	M3tTextureMap				**texture_list;

	texture_list = (M3tTextureMap**)WMrDialog_GetUserData(inDialog);

	// stop the timer
	WMrTimer_Stop(0, inDialog);
	ONrTextureMaterialList_Save(UUcFalse);

	// delete the texture list
	UUrMemory_Block_Delete(texture_list);
	texture_list = NULL;
}

// ----------------------------------------------------------------------
static UUtError
OWiTtM_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	WMtWindow					*inControl)
{
	MAtMaterialType				selected_material;
	UUtUns32					selected_texture;
	WMtWindow					*materials_listbox;
	WMtWindow					*textures_listbox;
	M3tTextureMap				**texture_list;
	M3tTextureMap				*texture;

	// get pointers to the listboxes
	materials_listbox = WMrDialog_GetItemByID(inDialog, OWcTtM_LB_Materials);
	textures_listbox = WMrDialog_GetItemByID(inDialog, OWcTtM_LB_Textures);

	// get the selected texture index
	selected_texture = WMrListBox_GetSelection(textures_listbox);

	// get the pointer to the texture
	texture_list = (M3tTextureMap**)WMrDialog_GetUserData(inDialog);
	texture = texture_list[selected_texture];

	switch (UUmLowWord(inParam1))
	{
		case OWcTtM_LB_Textures:
			if (UUmHighWord(inParam1) != LBcNotify_SelectionChanged) { break; }

			// set the picture
			WMrPicture_SetPicture(
				WMrDialog_GetItemByID(inDialog, OWcTtM_PT_Picture),
				texture);

			// get the selected material
			selected_material = M3rTextureMap_GetMaterialType(texture);
			if (selected_material == MAcInvalidID) { selected_material = MAcMaterial_Base; }

			// select the material
			WMrListBox_SetSelection(materials_listbox, UUcFalse, selected_material);
		break;

		case OWcTtM_LB_Materials:
			if (UUmHighWord(inParam1) != LBcNotify_SelectionChanged) { break; }

			// get the selected material
			selected_material = (MAtMaterialType)WMrListBox_GetSelection(materials_listbox);

			// get a pointer to the texture
			ONrTextureMaterialList_TextureMaterial_Set(
				TMrInstance_GetInstanceName(texture),
				selected_material);
		break;

		case WMcDialogItem_Cancel:
			WMrDialog_ModalEnd(inDialog, 0);
		break;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OWiTtM_HandleDrawItem(
	WMtDialog					*inDialog,
	WMtDrawItem					*inDrawItem)
{
	IMtPoint2D					dest;
//	PStPartSpecUI				*partspec_ui;
	UUtInt16					line_width;
	UUtInt16					line_height;
	M3tTextureMap				**texture_list;
	M3tTextureMap				*texture;
	MAtMaterialType				material_type;

	// get a pointer to the partspec_ui
//	partspec_ui = PSrPartSpecUI_GetActive();
//	UUmAssert(partspec_ui);

	texture_list = (M3tTextureMap**)WMrDialog_GetUserData(inDialog);
	UUmAssert(texture_list);

	// set the dest
	dest.x = inDrawItem->rect.left;
	dest.y = inDrawItem->rect.top;

	// calc the width and height of the line
	line_width = inDrawItem->rect.right - inDrawItem->rect.left;
	line_height = inDrawItem->rect.bottom - inDrawItem->rect.top;

	dest.x = 5;

	// draw the texture name
	texture = texture_list[inDrawItem->data];
	DCrDraw_String(
		inDrawItem->draw_context,
		TMrInstance_GetInstanceName(texture),
		&inDrawItem->rect,
		&dest);

	dest.x = 160;

	material_type = M3rTextureMap_GetMaterialType(texture);
	if (material_type != MAcMaterial_Base)
	{
		const char				*name;

		name = MArMaterialType_GetName(material_type);
		if (name != NULL)
		{
			// draw the material name
			DCrDraw_String(
				inDrawItem->draw_context,
				name,
				&inDrawItem->rect,
				&dest);
		}
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiTtM_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiTtM_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			OWiTtM_Destroy(inDialog);
		break;

		case WMcMessage_Command:
			OWiTtM_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;

		case WMcMessage_Timer:
			ONrTextureMaterialList_Save(UUcTrue);
		break;

		case WMcMessage_DrawItem:
			OWiTtM_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrTextureToMaterial_Display(
	void)
{
	UUtError					error;

	error =
		WMrDialog_ModalBegin(
			OWcDialog_Texture_to_Material,
			NULL,
			OWiTtM_Callback,
			0,
			NULL);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

#endif
