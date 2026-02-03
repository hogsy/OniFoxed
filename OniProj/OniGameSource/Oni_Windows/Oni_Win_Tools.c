// ======================================================================
// Oni_Win_Tools.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================

//#include <math.h>
#include "bfw_math.h"

#include "BFW.h"
#include "BFW_WindowManager.h"
#include "WM_PopupMenu.h"
#include "WM_EditField.h"
#include "WM_CheckBox.h"
#include "WM_Text.h"

#include "Oni_Character.h"
#include "Oni_Windows.h"
#include "Oni_Object.h"
#include "Oni_Object_Private.h"
#include "Oni_Sound2.h"
#include "Oni_Win_Sound2.h"
#include "Oni_Win_Tools.h"
#include "Oni_Object.h"
#include "Oni_Win_AI.h"
#include "Oni_Win_AI_Combat.h"
#include "Oni_Win_AI_Melee.h"
#include "Oni_Win_AI_Neutral.h"

// ======================================================================
// defines
// ======================================================================
#define OWcMaxCategories			16
#define ONcDoorTexturePrefix		"_DOOR"
#define ONcConsoleTexturePrefix		"_CON"

#define PPcDecalSize_EditSmall		0.1f
#define PPcDecalSize_EditLarge		1.0f
#define PPcDecalAngle_EditSmall		1.0f
#define PPcDecalAngle_EditLarge		15.0f

// ======================================================================
// typedefs
// ======================================================================

typedef struct OWtEORevert
{
	OBJtObject				*object;
	M3tPoint3D				position;
	M3tPoint3D				rotation;

} OWtEORevert;

typedef struct OWtEOPos
{
	UUtMemory_Array			*revert;

	M3tPoint3D				position_original;
	M3tPoint3D				rotation_original;

	M3tPoint3D				position_current;
	M3tPoint3D				rotation_current;

} OWtEOPos;


typedef struct OWtPTV
{
	OBJtObject				*object;

	UUtInt32				original_id;
	M3tPoint3D				original_scale;

	UUtInt32				new_id;
	M3tPoint3D				new_scale;

} OWtPTV;


typedef struct OWtCategory
{
	UUtUns32				object_type;
	char					name[OBJcMaxNameLength + 1];

} OWtCategory;

typedef struct OWtColorPopup
{
	char					*name;
	IMtShade				shade;

} OWtColorPopup;

typedef struct OWtLightProp
{
	OBJtObject				*object;
	UUtUns16				current_index;
	UUtUns32				num_ls_datas;
	OBJtLSData				*ls_data_array;

} OWtLightProp;

typedef struct OWtCharListParams
{
	WMtWindow				*listbox;
	UUtBool					restrict_list;
	UUtUns32				team_index;

} OWtCharListParams;

typedef struct OWtPropDialogSetup
{
	OBJtObjectType			object_type;
	UUtUns32				dialog_id;
	WMtDialogCallback		callback;

} OWtPropDialogSetup;

typedef struct OWtSoundProp
{
	OBJtObject				*object;
	OBJtOSD_All				osd;
	OBJtOSD_All				backup_osd;

} OWtSoundProp;

// ======================================================================
// globals
// ======================================================================
static UUtUns32				OWgNumCategories;
static OWtCategory			OWgCategories[OWcMaxCategories];

static WMtDialog			*OWgDialog_ObjNew;
static WMtDialog			*OWgDialog_Prop;
static WMtDialog			*OWgDialog_Pos;
static WMtDialog			*OWgDialog_LightProp;

#if TOOL_VERSION
static OWtColorPopup		OWgColorPopup[] =
{
	{ "Black",				IMcShade_Black		},
	{ "White",				IMcShade_White		},
	{ "Red",				IMcShade_Red		},
	{ "Green",				IMcShade_Green		},
	{ "Blue",				IMcShade_Blue		},
	{ "Yellow",				IMcShade_Yellow		},
	{ "Gray",				IMcShade_Gray75		},
	{ "Orange",				IMcShade_Orange		},
	{ "Purple",				IMcShade_Purple		},
	{ "Light Blue",			IMcShade_LightBlue	},
	{ NULL,					IMcShade_None }
};

static const char *AIgTeamList[] = { "konoko", "TCTF", "syndicate", "neutral", "security-guard", "rogue-konoko", "switzerland", "syndicate accessory", NULL };
static const char *OWgTriggerVolumeFlagList[] = {
	"<Enter Once>",
	"<Inside Once>",
	"<Exit Once>",
	"<Enter Disabled>",
	"<Inside Disabled>",
	"<Exit Disabled>",
	"<Disabled>",
	"<Player Only>",
	NULL };
#endif

// ======================================================================
// prototypes
// ======================================================================
#if TOOL_VERSION
static UUtBool
OWrProp_Particle_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);
#endif

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiColorPopup_Init(
	WMtWindow				*inPopup)
{
	OWtColorPopup			*data;
	UUtUns16				i;

	for (data = OWgColorPopup, i = 0;
		 data->name != NULL;
		 data++, i++)
	{
		UUtError			error;
		WMtMenuItemData		menu_item_data;

		menu_item_data.flags = WMcMenuItemFlag_Enabled;
		menu_item_data.id = i;
		UUrString_Copy(menu_item_data.title, data->name, WMcMaxTitleLength);

		error = WMrPopupMenu_AppendItem(inPopup, &menu_item_data);
		if (error != UUcError_None) { return; }
	}
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static IMtShade
OWiColorPopup_GetShadeFromID(
	UUtUns16				inID)
{
	return OWgColorPopup[inID].shade;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiColorPopup_SelectItemFromShade(
	WMtWindow				*inPopup,
	IMtShade				inShade)
{
	OWtColorPopup			*data;
	UUtUns16				i;

	for (data = OWgColorPopup, i = 0;
		 data->name != NULL;
		 data++, i++)
	{
		if (data->shade == inShade)
		{
			WMrPopupMenu_SetSelection(inPopup, i);
			break;
		}
	}
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiEOPos_EnableButtons(
	WMtDialog				*inDialog,
	UUtBool					inEnabled)
{
	WMtWindow				*button;
	UUtUns32				i;

	// enabled or disable the position buttons
	for (i = EOcBtn_LocX_Inc_0_1; i <= EOcBtn_LocZ_Dec_10_0; i++)
	{
		button = WMrDialog_GetItemByID(inDialog, (UUtUns16)i);
		WMrWindow_SetEnabled(button, inEnabled);
	}

	// enabled or disable the rotation buttons
	for (i = EOcBtn_RotX_Inc_1; i <= EOcBtn_RotZ_Dec_90; i++)
	{
		button = WMrDialog_GetItemByID(inDialog, (UUtUns16)i);
		WMrWindow_SetEnabled(button, inEnabled);
	}

	// enable or disable the gravity button
	button = WMrDialog_GetItemByID(inDialog, EOcBtn_Gravity);
	WMrWindow_SetEnabled(button, inEnabled);
}

// ----------------------------------------------------------------------
static float
OWiEOPos_GetValFromField(
	WMtDialog				*inDialog,
	UUtUns16				inItemID)
{
	float					value;
	WMtWindow				*edit_field;
	char					string[255];

	value = 0.0f;

	// get the field
	edit_field = WMrDialog_GetItemByID(inDialog, inItemID);
	if (edit_field == NULL) { return value; }

	// get the value from the field
	WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32)string, 255);
	sscanf(string, "%f", &value);

	return value;
}

// ----------------------------------------------------------------------
static void
OWiEOPos_SetEditFields(
	WMtDialog				*inDialog)
{
	WMtWindow				*edit_field;
	WMtWindow				*text_field;
	char					string[255];
	OWtEOPos				*pos;

	// get pos
	pos = (OWtEOPos*)WMrDialog_GetUserData(inDialog);
	UUmAssert(pos);

	// absolute location
	edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_AbsLocX);
	sprintf(string, "%5.3f", pos->position_current.x);
	WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);

	edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_AbsLocY);
	sprintf(string, "%5.3f", pos->position_current.y);
	WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);

	edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_AbsLocZ);
	sprintf(string, "%5.3f", pos->position_current.z);
	WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);

	// offset location
	edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_OffLocX);
	sprintf(string, "%5.3f", (pos->position_current.x - pos->position_original.x));
	WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);

	edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_OffLocY);
	sprintf(string, "%5.3f", (pos->position_current.y - pos->position_original.y));
	WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);

	edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_OffLocZ);
	sprintf(string, "%5.3f", (pos->position_current.z - pos->position_original.z));
	WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);

	// rotation
	edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_RotX);
	sprintf(string, "%5.3f", pos->rotation_current.x);
	WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);

	edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_RotY);
	sprintf(string, "%5.3f", pos->rotation_current.y);
	WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);

	edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_RotZ);
	sprintf(string, "%5.3f", pos->rotation_current.z);
	WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);

	if (OBJrSelectedObjects_GetNumSelected() > 1)
	{
		text_field = WMrDialog_GetItemByID(inDialog, EOcTxt_RotType);
		WMrWindow_SetTitle(text_field, "Offset:", WMcMaxTitleLength);
	}
	else
	{
		text_field = WMrDialog_GetItemByID(inDialog, EOcTxt_RotType);
		WMrWindow_SetTitle(text_field, "Absolute:", WMcMaxTitleLength);
	}
}

// ----------------------------------------------------------------------
static void
OWiEOPos_OffsetPositions(
	M3tPoint3D				*inPositionOffset)
{
	UUtUns32				i;

	for (i = 0; i < OBJrSelectedObjects_GetNumSelected(); i++)
	{
		OBJtObject				*object;
		M3tPoint3D				position;

		object = OBJrSelectedObjects_GetSelectedObject(i);
		MUmVector_Add(position, object->position, *inPositionOffset);
		OBJrObject_SetPosition(object, &position, NULL);
	}
}

// ----------------------------------------------------------------------
// inPos->rotation_current needs to be set before this function is called
// for cases where only one object is selected
static void
OWiEOPos_SetRotations(
	OWtEOPos				*inPos,
	M3tPoint3D				*inRotation,
	UUtBool					inRevert)
{
	UUtUns32				num_selected;
	OBJtObject				*object;

	while (inRotation->x < 0.0f) 	{ inRotation->x += 360.0f; }
	while (inRotation->y < 0.0f) 	{ inRotation->y += 360.0f; }
	while (inRotation->z < 0.0f) 	{ inRotation->z += 360.0f; }
	while (inRotation->x >= 360.0f) { inRotation->x -= 360.0f; }
	while (inRotation->y >= 360.0f) { inRotation->y -= 360.0f; }
	while (inRotation->z >= 360.0f) { inRotation->z -= 360.0f; }

	num_selected = OBJrSelectedObjects_GetNumSelected();
	if (num_selected > 1)
	{
		M3tMatrix4x3		rotation_matrix;
		UUtUns32			i;
		float				x_rad;
		float				y_rad;
		float				z_rad;

		// inRotation is an offset from the current rotation
		x_rad = inRotation->x * M3cDegToRad;
		y_rad = inRotation->y * M3cDegToRad;
		z_rad = inRotation->z * M3cDegToRad;

		MUrMatrix_Identity(&rotation_matrix);
		if (inRevert)
		{
			MUrMatrix_RotateZ(&rotation_matrix, z_rad);
			MUrMatrix_RotateY(&rotation_matrix, y_rad);
			MUrMatrix_RotateX(&rotation_matrix, x_rad);
		}
		else
		{
		MUrMatrix_RotateX(&rotation_matrix, x_rad);
		MUrMatrix_RotateY(&rotation_matrix, y_rad);
		MUrMatrix_RotateZ(&rotation_matrix, z_rad);
		}

		for (i = 0; i < num_selected; i++)
		{
			M3tPoint3D			delta;
			M3tPoint3D			position;
			M3tMatrix4x3		object_rotation_matrix;
			M3tMatrix4x3		rotation_result;

			object = OBJrSelectedObjects_GetSelectedObject(i);
			MUmVector_Subtract(delta, object->position, inPos->position_current);

			MUrMatrix_MultiplyPoint(&delta, &rotation_matrix, &position);
			MUmVector_Increment(position, inPos->position_current);

			OBJrObject_GetRotationMatrix(object, &object_rotation_matrix);
			MUrMatrix_Multiply(&rotation_matrix, &object_rotation_matrix, &rotation_result);

			OBJrObject_SetRotationMatrix(object, &rotation_result);

			OBJrObject_SetPosition(object, &position,NULL);
		}
	}
	else
	{
		// inRotation is an absolute rotation
		object = OBJrSelectedObjects_GetSelectedObject(0);
		OBJrObject_SetPosition(object, NULL, &inPos->rotation_current);
	}
}

// ----------------------------------------------------------------------
static void
OWiEOPos_Apply(
	WMtDialog				*inDialog,
	OWtEOPos				*inPos)
{
	M3tPoint3D				position;
	M3tPoint3D				rotation;
	M3tPoint3D				delta;

	// get the position
	position.x = OWiEOPos_GetValFromField(inDialog, EOcEF_AbsLocX);
	position.y = OWiEOPos_GetValFromField(inDialog, EOcEF_AbsLocY);
	position.z = OWiEOPos_GetValFromField(inDialog, EOcEF_AbsLocZ);

	// calculate the difference in the typed in position and the current position
	MUmVector_Subtract(delta, position, inPos->position_current);

	// save the new position
	MUmVector_Copy(inPos->position_current, position);

	// offset the position of the objects
	OWiEOPos_OffsetPositions(&delta);

	// get the rotation
	rotation.x = OWiEOPos_GetValFromField(inDialog, EOcEF_RotX);
	rotation.y = OWiEOPos_GetValFromField(inDialog, EOcEF_RotY);
	rotation.z = OWiEOPos_GetValFromField(inDialog, EOcEF_RotZ);

	// calculate the difference in the typed in rotation and the current rotation
	MUmVector_Subtract(delta, rotation, inPos->rotation_current);

	// save the new rotation
	MUmVector_Copy(inPos->rotation_current, rotation);

	// offset the rotation of the objets
	OWiEOPos_SetRotations(inPos, &delta, UUcFalse);

	// enable the position buttons
	OWiEOPos_EnableButtons(inDialog, UUcTrue);
}

// ----------------------------------------------------------------------
static void
OWiEOPos_ContentChanged(
	WMtDialog				*inDialog,
	OWtEOPos				*inPos,
	UUtUns16				inControlID)
{
	float					new_value;
	WMtWindow				*edit_field;
	char					string[128];

	new_value = OWiEOPos_GetValFromField(inDialog, inControlID);

	switch (inControlID)
	{
		case EOcEF_AbsLocX:
			edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_OffLocX);
			sprintf(string, "%5.3f", (new_value - inPos->position_original.x));
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);
		break;

		case EOcEF_AbsLocY:
			edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_OffLocY);
			sprintf(string, "%5.3f", (new_value - inPos->position_original.y));
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);
		break;

		case EOcEF_AbsLocZ:
			edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_OffLocZ);
			sprintf(string, "%5.3f", (new_value - inPos->position_original.z));
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);
		break;

		case EOcEF_OffLocX:
			edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_AbsLocX);
			sprintf(string, "%5.3f", (inPos->position_original.x + new_value));
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);
		break;

		case EOcEF_OffLocY:
			edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_AbsLocY);
			sprintf(string, "%5.3f", (inPos->position_original.y + new_value));
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);
		break;

		case EOcEF_OffLocZ:
			edit_field = WMrDialog_GetItemByID(inDialog, EOcEF_AbsLocZ);
			sprintf(string, "%5.3f", (inPos->position_original.z + new_value));
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32)string, 0);
		break;

		case EOcEF_RotX:
		case EOcEF_RotY:
		case EOcEF_RotZ:
		break;
	}

	// disable the position buttons
	OWiEOPos_EnableButtons(inDialog, UUcFalse);
}

// ----------------------------------------------------------------------
static void
OWiEOPos_Gravity(
	WMtDialog				*inDialog,
	OWtEOPos				*inPos)
{
	UUtBool					result;
	M3tVector3D				vector;
	M3tPoint3D				delta;

	MUmVector_Set(vector, 0.0f, -50000.0f, 0.0f);

	// calculate the intersection with the floor
	result =
		AKrCollision_Point(
			ONrGameState_GetEnvironment(),
			&inPos->position_current,
			&vector,
			AKcGQ_Flag_Obj_Col_Skip,
			AKcMaxNumCollisions);
	if (result == UUcFalse) { return; }

	// set the new position of the object
	MUmVector_Set(delta, 0.0f, (AKgCollisionList[0].collisionPoint.y - inPos->position_current.y), 0.0f);
	OWiEOPos_OffsetPositions(&delta);

	// update the position
	MUmVector_Increment(inPos->position_current, delta);

	// update the edit fields
	OWiEOPos_SetEditFields(inDialog);
}

// ----------------------------------------------------------------------
static void
OWiEOPos_Revert(
	WMtDialog				*inDialog,
	OWtEOPos				*inPos)
{
	UUtUns32				i;
	OWtEORevert				*revert_array;

	if (inPos->revert == NULL) { return; }

	// revert all of the objects to their original rotation and position
	revert_array = (OWtEORevert*)UUrMemory_Array_GetMemory(inPos->revert);
	for (i = 0; i < UUrMemory_Array_GetUsedElems(inPos->revert); i++)
	{
		OBJrObject_SetPosition(
			revert_array[i].object,
			&revert_array[i].position,
			&revert_array[i].rotation);
	}

	// reset the current position and rotation
	MUmVector_Copy(inPos->position_current, inPos->position_original);
	MUmVector_Copy(inPos->rotation_current, inPos->rotation_original);

	// update the edit fields
	OWiEOPos_SetEditFields(inDialog);

	// disable the position buttons
	OWiEOPos_EnableButtons(inDialog, UUcTrue);
}

// ----------------------------------------------------------------------
static void
OWiEOPos_InitDialog(
	WMtDialog				*inDialog)
{
	OWtEOPos				*pos;
	OBJtObject				*object;
	UUtUns32				num_selected;
	UUtUns32				i;
	UUtRect					window_rect;

	num_selected = OBJrSelectedObjects_GetNumSelected();

	// move the dialog to the left hand side of the screen
	WMrWindow_GetRect(inDialog, &window_rect);
	WMrWindow_SetLocation(inDialog, 0, window_rect.top);

	// allocate memory for the pos
	pos = (OWtEOPos*)UUrMemory_Block_NewClear(sizeof(OWtEOPos));
	UUmAssert(pos);
	WMrDialog_SetUserData(inDialog, (UUtUns32)pos);

	// initialize the vars
	pos->revert =
		UUrMemory_Array_New(
			sizeof(OWtEORevert),
			1,
			num_selected,
			num_selected);
	if (pos->revert == NULL)
	{
		WMrDialog_MessageBox(
			inDialog,
			"Warning",
			"Unable to revert because there is not enough memory available.",
			WMcMessageBoxStyle_OK);
	}
	else
	{
		OWtEORevert				*revert_array;

		revert_array = (OWtEORevert*)UUrMemory_Array_GetMemory(pos->revert);
		for (i = 0; i < num_selected; i++)
		{
			object = OBJrSelectedObjects_GetSelectedObject(i);
			revert_array[i].object = object;
			OBJrObject_GetPosition(object, &revert_array[i].position, &revert_array[i].rotation);
		}
	}

	MUmVector_Set(pos->position_original, 0.0f, 0.0f, 0.0f);
	MUmVector_Set(pos->rotation_original, 0.0f, 0.0f, 0.0f);

	MUmVector_Set(pos->position_current, 0.0f, 0.0f, 0.0f);
	MUmVector_Set(pos->rotation_current, 0.0f, 0.0f, 0.0f);

	// get the center of the selected objects and store the revert information
	for (i = 0; i < num_selected; i++)
	{
		object = OBJrSelectedObjects_GetSelectedObject(i);
		MUmVector_Increment(pos->position_original, object->position);
	}

	pos->position_original.x /= num_selected;
	pos->position_original.y /= num_selected;
	pos->position_original.z /= num_selected;

	// set the rotation
	if (num_selected == 1)
	{
		object = OBJrSelectedObjects_GetSelectedObject(0);
		MUmVector_Copy(pos->rotation_original, object->rotation);
	}

	// set the current position and rotation
	MUmVector_Copy(pos->position_current, pos->position_original);
	MUmVector_Copy(pos->rotation_current, pos->rotation_original);

	// update the edit fields
	OWiEOPos_SetEditFields(inDialog);
}

// ----------------------------------------------------------------------
static void
OWiEOPos_Destroy(
	WMtDialog				*inDialog)
{
	OWtEOPos				*pos;

	// get a pointer to the pos
	pos = (OWtEOPos*)WMrDialog_GetUserData(inDialog);
	if (pos)
	{
		if (pos->revert)
		{
			UUrMemory_Array_Delete(pos->revert);
			pos->revert = NULL;
		}

		UUrMemory_Block_Delete(pos);
		WMrDialog_SetUserData(inDialog, 0);
	}

	OWgDialog_Pos = NULL;
}

// ----------------------------------------------------------------------
static void
OWiEOPos_HandleCommand(
	WMtDialog				*inDialog,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	OWtEOPos				*pos;

	// get a pointer to the pos
	pos = (OWtEOPos*)WMrDialog_GetUserData(inDialog);
	UUmAssert(pos);

	if (UUmHighWord(inParam1) == WMcNotify_Click)
	{
		UUtUns16			field;
		UUtBool				update;
		float				val;

		field = 0;
		update = UUcTrue;
		val = 0.0f;

		switch (UUmLowWord(inParam1))
		{
			// Location
			case EOcBtn_LocX_Inc_0_1:	field = EOcEF_AbsLocX; val = 0.1f;	break;
			case EOcBtn_LocY_Inc_0_1:	field = EOcEF_AbsLocY; val = 0.1f;	break;
			case EOcBtn_LocZ_Inc_0_1:	field = EOcEF_AbsLocZ; val = 0.1f;	break;

			case EOcBtn_LocX_Dec_0_1:	field = EOcEF_AbsLocX; val = -0.1f; break;
			case EOcBtn_LocY_Dec_0_1:	field = EOcEF_AbsLocY; val = -0.1f; break;
			case EOcBtn_LocZ_Dec_0_1:	field = EOcEF_AbsLocZ; val = -0.1f; break;

			case EOcBtn_LocX_Inc_1_0:	field = EOcEF_AbsLocX; val = 1.0f;	break;
			case EOcBtn_LocY_Inc_1_0:	field = EOcEF_AbsLocY; val = 1.0f;	break;
			case EOcBtn_LocZ_Inc_1_0:	field = EOcEF_AbsLocZ; val = 1.0f;	break;

			case EOcBtn_LocX_Dec_1_0:	field = EOcEF_AbsLocX; val = -1.0f;	break;
			case EOcBtn_LocY_Dec_1_0:	field = EOcEF_AbsLocY; val = -1.0f;	break;
			case EOcBtn_LocZ_Dec_1_0:	field = EOcEF_AbsLocZ; val = -1.0f;	break;

			case EOcBtn_LocX_Inc_10_0:	field = EOcEF_AbsLocX; val = 10.0f;	break;
			case EOcBtn_LocY_Inc_10_0:	field = EOcEF_AbsLocY; val = 10.0f;	break;
			case EOcBtn_LocZ_Inc_10_0:	field = EOcEF_AbsLocZ; val = 10.0f;	break;

			case EOcBtn_LocX_Dec_10_0:	field = EOcEF_AbsLocX; val = -10.0f;	break;
			case EOcBtn_LocY_Dec_10_0:	field = EOcEF_AbsLocY; val = -10.0f;	break;
			case EOcBtn_LocZ_Dec_10_0:	field = EOcEF_AbsLocZ; val = -10.0f;	break;

			// Rotation
			case EOcBtn_RotX_Inc_1:		field = EOcEF_RotX; val = 1.0f;	break;
			case EOcBtn_RotY_Inc_1:		field = EOcEF_RotY; val = 1.0f;	break;
			case EOcBtn_RotZ_Inc_1:		field = EOcEF_RotZ; val = 1.0f;	break;

			case EOcBtn_RotX_Inc_15:	field = EOcEF_RotX; val = 15.0f;	break;
			case EOcBtn_RotY_Inc_15:	field = EOcEF_RotY; val = 15.0f;	break;
			case EOcBtn_RotZ_Inc_15:	field = EOcEF_RotZ; val = 15.0f;	break;

			case EOcBtn_RotX_Inc_90:	field = EOcEF_RotX; val = 90.0f;	break;
			case EOcBtn_RotY_Inc_90:	field = EOcEF_RotY; val = 90.0f;	break;
			case EOcBtn_RotZ_Inc_90:	field = EOcEF_RotZ; val = 90.0f;	break;

			case EOcBtn_RotX_Dec_1:		field = EOcEF_RotX; val = -1.0f;	break;
			case EOcBtn_RotY_Dec_1:		field = EOcEF_RotY; val = -1.0f;	break;
			case EOcBtn_RotZ_Dec_1:		field = EOcEF_RotZ; val = -1.0f;	break;

			case EOcBtn_RotX_Dec_15:	field = EOcEF_RotX; val = -15.0f;	break;
			case EOcBtn_RotY_Dec_15:	field = EOcEF_RotY; val = -15.0f;	break;
			case EOcBtn_RotZ_Dec_15:	field = EOcEF_RotZ; val = -15.0f;	break;

			case EOcBtn_RotX_Dec_90:	field = EOcEF_RotX; val = -90.0f;	break;
			case EOcBtn_RotY_Dec_90:	field = EOcEF_RotY; val = -90.0f;	break;
			case EOcBtn_RotZ_Dec_90:	field = EOcEF_RotZ; val = -90.0f;	break;

			// buttons
			case EOcBtn_Revert:
				OWiEOPos_Revert(inDialog, pos);
			break;

			case EOcBtn_Gravity:
				OWiEOPos_Gravity(inDialog, pos);
			break;

			case EOcBtn_Apply:
				OWiEOPos_Apply(inDialog, pos);
				update = UUcFalse;
			break;

			case WMcDialogItem_Cancel:
				WMrWindow_Delete(inDialog);
				update = UUcFalse;
			break;

			default:
				update = UUcFalse;
			break;
		}

		if (update)
		{
			M3tPoint3D					delta_position;
			M3tPoint3D					delta_rotation;

			// calculate the delta_position and delta_rotation
			MUmVector_Set(delta_position, 0.0f, 0.0f, 0.0f);
			MUmVector_Set(delta_rotation, 0.0f, 0.0f, 0.0f);

			switch (field)
			{
				case EOcEF_AbsLocX:	delta_position.x += val;	break;
				case EOcEF_AbsLocY:	delta_position.y += val;	break;
				case EOcEF_AbsLocZ:	delta_position.z += val;	break;

				case EOcEF_RotX:	delta_rotation.x += val;	break;
				case EOcEF_RotY:	delta_rotation.y += val;	break;
				case EOcEF_RotZ:	delta_rotation.z += val;	break;
			}

			// adjust the current position and rotation
			MUmVector_Increment(pos->position_current, delta_position);
			MUmVector_Increment(pos->rotation_current, delta_rotation);

			// check rotation ranges
			while (pos->rotation_current.x < 0.0f)		{ pos->rotation_current.x += 360.0f; }
			while (pos->rotation_current.y < 0.0f)		{ pos->rotation_current.y += 360.0f; }
			while (pos->rotation_current.z < 0.0f)		{ pos->rotation_current.z += 360.0f; }
			while (pos->rotation_current.x >= 360.0f)	{ pos->rotation_current.x -= 360.0f; }
			while (pos->rotation_current.y >= 360.0f)	{ pos->rotation_current.y -= 360.0f; }
			while (pos->rotation_current.z >= 360.0f)	{ pos->rotation_current.z -= 360.0f; }

			// offset the selected object positions
			OWiEOPos_OffsetPositions(&delta_position);

			// set the selected object rotations
			OWiEOPos_SetRotations(pos, &delta_rotation, UUcFalse);

			// update the edit fields
			OWiEOPos_SetEditFields(inDialog);
		}
	}
	else if (UUmHighWord(inParam1) == EFcNotify_ContentChanged)
	{
		OWiEOPos_ContentChanged(inDialog, pos, UUmLowWord(inParam1));
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiEOPos_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiEOPos_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			OWiEOPos_Destroy(inDialog);
		return UUcFalse;

		case WMcMessage_Command:
			OWiEOPos_HandleCommand(inDialog, inParam1, inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrEOPos_Display(
	void)
{
	UUtError			error;

	if ((OWgDialog_Pos == NULL) && (OWgDialog_Prop == NULL))
	{
		// create the dialog
		error = WMrDialog_Create(OWcDialog_EditObjectPos, NULL, OWiEOPos_Callback, (UUtUns32) -1, &OWgDialog_Pos);
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
OWrEOPos_IsVisible(
	void)
{
	if (OWgDialog_Pos == NULL) { return UUcFalse; }

	return WMrWindow_GetVisible(OWgDialog_Pos);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OWiObjNew_CreateObject(
	WMtDialog				*inDialog)
{
	WMtWindow				*popup;
	UUtUns16				item_id;

	// get the popup menu
	popup = WMrDialog_GetItemByID(inDialog, NOcPM_Category);
	if (popup == NULL) { return UUcError_Generic; }

	// get the item id of the currently selected menu item
	WMrPopupMenu_GetItemID(popup, (UUtUns16)(-1), &item_id);

	return OWrTools_CreateObject(OWgCategories[item_id].object_type, NULL);
}

// ----------------------------------------------------------------------
static void
OWiObjNew_InitDialog(
	WMtDialog				*inDialog)
{
	WMtWindow				*popup;
	UUtUns16				i;

	// get the popup
	popup = WMrDialog_GetItemByID(inDialog, NOcPM_Category);
	if (popup == NULL) { return; }

	// fill in the categories
	for (i = 0; i < OWgNumCategories; i++)
	{
		WMtMenuItemData		item_data;

		if (OWgCategories[i].name[0] == '\0') { continue; }

		// setup the item data
		item_data.flags		= WMcMenuItemFlag_Enabled;
		item_data.id		= i;
		UUrString_Copy(item_data.title, OWgCategories[i].name, WMcMaxTitleLength);

		// add the item to the popup menu
		WMrPopupMenu_AppendItem(popup, &item_data);
	}

	// select the first item in the popup
	WMrPopupMenu_SetSelection(popup, 0);
}

// ----------------------------------------------------------------------
static void
OWiObjNew_HandleCommand(
	WMtDialog				*inDialog,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtError error;

	switch (UUmLowWord(inParam1))
	{
		case WMcDialogItem_OK:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				UUtBool object_created;
				OBJtObject *object;

				// create the object
				error = OWiObjNew_CreateObject(inDialog);

				// close the dialog
				WMrWindow_Delete(inDialog);

				if (error == UUcError_None) {
					// get the object to work on
					object = OBJrSelectedObjects_GetSelectedObject(0);

					// display the properties dialog
					object_created = OWrProp_Display(object);

					if ((object != NULL) && (!object_created)) {
						OBJrObject_Delete(object);
					}
				}
			}
		break;

		case WMcDialogItem_Cancel:
			WMrWindow_Delete(inDialog);
		break;
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiObjNew_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiObjNew_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			OWgDialog_ObjNew = NULL;
		break;

		case WMcMessage_Command:
			OWiObjNew_HandleCommand(inDialog, inParam1, inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrObjNew_Display(
void)
{
	UUtError			error;

	if ((OWgDialog_Pos == NULL) &&
		(OWgDialog_Prop == NULL))
	{
		// create the new object dialog
		error = WMrDialog_Create(OWcDialog_ObjNew, NULL, OWiObjNew_Callback, (UUtUns32) -1, &OWgDialog_ObjNew);
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
OWrObjNew_IsVisible(
	void)
{
	if (OWgDialog_ObjNew == NULL) { return UUcFalse; }

	return WMrWindow_GetVisible(OWgDialog_ObjNew);
}
// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================

// ----------------------------------------------------------------------
// Event properties
// ----------------------------------------------------------------------

#if TOOL_VERSION
static void OWiProp_Event_InitDialog( WMtDialog *inDialog )
{
	WMtWindow					*window;
	ONtEvent					*event;
	ONtEventDescription			*desc;
	char						value[100];
	char						label[100];

	event = (ONtEvent*)WMrDialog_GetUserData(inDialog);
	UUmAssert( event );

	desc = ONrEvent_GetDescription( event->event_type );
	UUmAssert( desc );

	switch( desc->params_type )
	{
		case ONcEventParams_ID16:
			sprintf( label, "ID:" );
			sprintf( value, "%d", event->params.params_id16.id );
			break;
		case ONcEventParams_ID32:
			sprintf( label, "ID:" );
			sprintf( value, "%d", event->params.params_id32.id );
			break;
		case ONcEventParams_Text:
			sprintf( label, "Value:" );
			sprintf( value, "%s", event->params.params_text.text );
			break;
		default:
			UUmAssert( UUcFalse );
	}

	WMrWindow_SetTitle( (WMtWindow*) inDialog, "Event Properties", 32 );

	window = WMrDialog_GetItemByID(inDialog, PEcEF_Type);		UUmAssert( window );
	WMrWindow_SetTitle( window, desc->name, 32 );

	window = WMrDialog_GetItemByID(inDialog, PEcEF_Label0);		UUmAssert( window );
	WMrWindow_SetTitle( window, label, 32 );

	window = WMrDialog_GetItemByID(inDialog, PEcEF_Value0);		UUmAssert( window );
	WMrMessage_Send( window, EFcMessage_SetText, (UUtUns32) value, (UUtUns32)(-1));

	WMrWindow_SetFocus(window);
}
#endif

// ----------------------------------------------------------------------

#if TOOL_VERSION
static void OWiProp_Event_HandleCommand( WMtDialog *inDialog, UUtUns32 inParam1,	UUtUns32 inParam2 )
{
	UUtError		error;
	switch (UUmLowWord(inParam1))
	{
		case WMcDialogItem_OK:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				ONtEventDescription		*desc;
				WMtWindow				*window;
				char					string[OBJcMaxNoteChars + 1];
				ONtEvent*				event;

				event = (ONtEvent*)WMrDialog_GetUserData(inDialog);			UUmAssert( event );

				desc = ONrEvent_GetDescription( event->event_type );		UUmAssert( desc );

				window = WMrDialog_GetItemByID(inDialog, PEcEF_Value0);		UUmAssert( window );

				WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, (UUtUns32)(-1));

				switch( desc->params_type )
				{
					case ONcEventParams_ID16:
						{
							UUtUns16		val;
							error = UUrString_To_Uns16( string, &val );
							if( error == UUcError_None )
								event->params.params_id16.id = val;
						}
						break;
					case ONcEventParams_ID32:
						{
							UUtUns32		val;
							error = UUrString_To_Uns32( string, &val );
							if( error == UUcError_None )
								event->params.params_id32.id = val;
						}
						break;
					case ONcEventParams_Text:
						{
							UUrString_Copy( event->params.params_text.text, string, ONcEventParams_MaxTextLength );
						}
						break;
					default:
						UUmAssert( UUcFalse );
				}

			}
			// close the dialog
			WMrDialog_ModalEnd(inDialog, UUcTrue );
		break;

		case WMcDialogItem_Cancel:
			WMrDialog_ModalEnd( inDialog, UUcFalse );
		break;
	}
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool OWiProp_Event_Callback( WMtDialog *inDialog, WMtMessage inMessage, UUtUns32 inParam1, UUtUns32	inParam2)
{
	UUtBool					handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiProp_Event_InitDialog(inDialog);
		break;

		//case WMcMessage_Destroy:
		//break;

		case WMcMessage_Command:
			OWiProp_Event_HandleCommand(inDialog, inParam1, inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool OWiProp_Event_Display(ONtEvent* inEvent)
{
	UUtError			error;
	UUtUns32			return_val;
	error = WMrDialog_ModalBegin(OWcDialog_Prop_Event, NULL, OWiProp_Event_Callback, (UUtUns32) inEvent, (UUtUns32*) &return_val );
	UUmAssert( error ==  UUcError_None );
	if( !return_val || error != UUcError_None )
		return UUcFalse;
	return UUcTrue;
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================

// ----------------------------------------------------------------------
// event list helpers
// ----------------------------------------------------------------------

// fill the event dialog type
#if TOOL_VERSION
static UUtBool OWiEvent_EnumEventTypes( const char *inName, UUtUns32 inUserData )
{
	OWtStringListDlgInstance		*dlg;

	dlg = (OWtStringListDlgInstance*)	inUserData;

	OWrStringListDialog_AddString( dlg, (char*) inName );

	return UUcTrue;
}
#endif

#if TOOL_VERSION
static UUtBool OWrEventList_NewEvent( ONtEventList* inEventList, WMtWindow* inWindow )
{
	ONtEvent				event;
	UUtUns32				count;
	ONtEventDescription		*desc;
	char					name[100];

	// first get the type of event

	OWtStringListDlgInstance	dlg;

	OWrStringListDialog_Create( &dlg );

	OWrStringListDialog_SetTitle( &dlg, "New Event" );

	count = ONrEvent_EnumerateTypes( OWiEvent_EnumEventTypes, (UUtUns32) &dlg );

	UUmAssert( count );

	dlg.selected_index = 0;

	OWrStringListDialog_DoModal( &dlg );

	OWrStringListDialog_Destroy( &dlg );

	if( !dlg.return_value || dlg.selected_index == -1 )
		return UUcFalse;

	desc = &ONcEventDescriptions[dlg.selected_index];

	UUrMemory_Clear( &event, sizeof( ONtEvent ) );

	event.event_type	= desc->type;

	if( !OWiProp_Event_Display( &event ) )
	{
		return UUcFalse;
	}

	ONrEventList_AddEvent( inEventList, &event );

	ONrEvent_GetName( &event, name, 100 );

	WMrMessage_Send( inWindow, LBcMessage_AddString, (UUtUns32) name, 0);

	WMrWindow_SetFocus(inWindow);

	return UUcTrue;
}
#endif

#if TOOL_VERSION
static UUtBool OWrEventList_DeleteEvent( ONtEventList* inEventList, WMtWindow* inWindow )
{
	UUtUns32			selection;

	UUmAssert( inEventList && inWindow );

	selection = WMrMessage_Send( inWindow, LBcMessage_GetSelection, (UUtUns32)-1, (UUtUns32)-1 );

	if( selection == LBcError)
		return UUcFalse;

	UUmAssert( selection <= inEventList->event_count );

	ONrEventList_DeleteEvent( inEventList, selection );

	WMrMessage_Send( inWindow, LBcMessage_DeleteString, (UUtUns32) -1, selection);

	WMrWindow_SetFocus(inWindow);

	return UUcTrue;
}
#endif

#if TOOL_VERSION
static UUtBool OWrEventList_EditEvent( ONtEventList* inEventList, WMtWindow* inWindow )
{
	UUtUns32				selection;
	ONtEvent				event;
	char					name[100];

	UUmAssert( inEventList && inWindow );

	selection = WMrMessage_Send( inWindow, LBcMessage_GetSelection, (UUtUns32)-1, (UUtUns32)-1 );

	if( selection == LBcError)
		return UUcFalse;

	UUmAssert( selection < inEventList->event_count );

	UUrMemory_MoveFast( &inEventList->events[selection], &event, sizeof(ONtEvent) );

	if( !OWiProp_Event_Display( &event ) )
	{
		return UUcFalse;
	}

	UUrMemory_MoveFast( &event, &inEventList->events[selection], sizeof(ONtEvent) );

	ONrEvent_GetName( &event, name, 100 );

	WMrMessage_Send( inWindow, LBcMessage_ReplaceString, (UUtUns32) name, selection );

	WMrWindow_SetFocus(inWindow);

	return UUcTrue;
}
#endif

// ----------------------------------------------------------------------
// door properties
// ----------------------------------------------------------------------

#define _FootToDist				(1.0f / UUmFeet(1))

#if TOOL_VERSION
static void OWiProp_Door_SetFields( WMtDialog *inDialog )
{
	OBJtOSD_All			*osd_all;
	WMtWindow			*window;
	char				string[255];
	OBJtObject			*object = (OBJtObject *) WMrDialog_GetUserData(inDialog);

	// get the Door properties
	osd_all = (OBJtOSD_All*)WMrDialog_GetUserData(inDialog);
	UUmAssert(osd_all);

	// ID
	window = WMrDialog_GetItemByID(inDialog, PDcEF_ID);						UUmAssert( window );
	sprintf(string, "%d", osd_all->osd.door_osd.id );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// Key ID
	window = WMrDialog_GetItemByID(inDialog, PDcEF_KeyID);					UUmAssert( window );
	sprintf(string, "%d", osd_all->osd.door_osd.key_id );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// Activation radius
	window = WMrDialog_GetItemByID(inDialog, PDcEF_ActivationRadius);		UUmAssert( window );
 	sprintf(string, "%.2f", _FootToDist * MUrSqrt( osd_all->osd.door_osd.activation_radius_squared ) );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// Locked
	window = WMrDialog_GetItemByID(inDialog, PDcCB_Locked);					UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.door_osd.flags & OBJcDoorFlag_Locked) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// Initial Locked
	window = WMrDialog_GetItemByID(inDialog, PDcCB_InitialLocked);			UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.door_osd.flags & OBJcDoorFlag_InitialLocked) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// double doors
	window = WMrDialog_GetItemByID(inDialog, PDcCB_DoubleDoors);			UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.door_osd.flags & OBJcDoorFlag_DoubleDoors) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// Manual door
	window = WMrDialog_GetItemByID(inDialog, PDcCB_ManualDoor);				UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.door_osd.flags & OBJcDoorFlag_Manual) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// One way lock
	window = WMrDialog_GetItemByID(inDialog, PDcCB_OneWay);				UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.door_osd.flags & OBJcDoorFlag_OneWay) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// Reversed door
	window = WMrDialog_GetItemByID(inDialog, PDcCB_Reverse);				UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.door_osd.flags & OBJcDoorFlag_Reverse) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// Test Mode
	window = WMrDialog_GetItemByID(inDialog, PDcCB_Test);					UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.door_osd.flags & OBJcDoorFlag_TestMode) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// Mirror
	window = WMrDialog_GetItemByID(inDialog, PDcCB_Mirror);					UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.door_osd.flags & OBJcDoorFlag_Mirror) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// texture lists
	window = WMrDialog_GetItemByID(inDialog, PDcPM_Texture0);				UUmAssert( window );
	if (strlen(osd_all->osd.door_osd.door_texture[0]) > strlen(ONcDoorTexturePrefix)) {
		WMrPopupMenu_SelectString_NoCase( window, &osd_all->osd.door_osd.door_texture[0][strlen(ONcDoorTexturePrefix)] );
	}
	else {
		WMrPopupMenu_SelectString_NoCase( window, "<default>" );
	}

	// texture lists
	window = WMrDialog_GetItemByID(inDialog, PDcPM_Texture1);				UUmAssert( window );
	if (strlen(osd_all->osd.door_osd.door_texture[1]) > strlen(ONcDoorTexturePrefix)) {
		WMrPopupMenu_SelectString_NoCase( window, &osd_all->osd.door_osd.door_texture[1][strlen(ONcDoorTexturePrefix)] );
	}
	else {
		WMrPopupMenu_SelectString_NoCase( window, "<default>" );
	}

	// fill event list
	window = WMrDialog_GetItemByID( inDialog, PDcLB_Events );				UUmAssert( window );
	ONrEventList_FillListBox( &osd_all->osd.door_osd.event_list, window );

	// enable OK button
	window = WMrDialog_GetItemByID(inDialog, WMcDialogItem_OK);
	WMrWindow_SetEnabled(window, !OBJrObjectType_IsLocked(OBJcType_Door));
}
#endif

#if TOOL_VERSION
static UUtBool OWiListBox_DoorType_EnumCallback( const char *inName, UUtUns32 inUserData )
{
	WMtWindow				*window;

	UUmAssert( inUserData );

	window = (WMtWindow*)inUserData;

	if (!window)		return UUcFalse;

	WMrMessage_Send( window, LBcMessage_AddString, (UUtUns32) inName, 0);

	return UUcTrue;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Door_SelectItem( WMtDialog *inDialog )
{
	WMtWindow			*window;
	OBJtObject			*object;
	OBJtOSD_All			*osd;

	// get a pointer to the osd
	osd = (OBJtOSD_All*)WMrDialog_GetUserData(inDialog);
	UUmAssert(osd);

	// get a pointer to the listbox
	window = WMrDialog_GetItemByID(inDialog, PDcLB_DoorTypes);
	if (window == NULL) { return; }

	// get the currently selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }

	// get the name of the selected object specific type
	if( !osd->osd.door_osd.door_class_name[0] )
	{
		WMrMessage_Send( window, LBcMessage_SetSelection, (UUtUns32) 0, 0 );
	}
	else
	{
		WMrMessage_Send( window, LBcMessage_SelectString, (UUtUns32)(-1), (UUtUns32)osd->osd.door_osd.door_class_name );
	}
}
#endif


#if TOOL_VERSION
static UUtBool OWiListBox_DoorTextures_EnumCallback( const char *inName, UUtUns32 inUserData )
{
	WMtWindow				*window;
	WMtDialog				*dialog;
	WMtMenuItemData			menu_item_data;
	UUtError				error;
	static int				id = 1;

	dialog = (WMtDialog*)inUserData;
	if (dialog == NULL) { return UUcFalse; }

	menu_item_data.flags	= WMcMenuItemFlag_Enabled;
	menu_item_data.id		= id++;
	UUrString_Copy(menu_item_data.title, inName, WMcMaxTitleLength);

	window = WMrDialog_GetItemByID( dialog, PDcPM_Texture0 );		UUmAssert( window );
	error = WMrPopupMenu_AppendItem(window, &menu_item_data);

	window = WMrDialog_GetItemByID( dialog, PDcPM_Texture1 );		UUmAssert( window );
	error = WMrPopupMenu_AppendItem(window, &menu_item_data);

	return UUcTrue;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Door_InitDialog( WMtDialog *inDialog )
{
	WMtMenuItemData			menu_item_data;
	WMtWindow			*listbox;
	OBJtObject			*object;
	OBJtObjectType		object_type;
	OBJtOSD_All			*osd;
	WMtWindow			*editfield;

	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, PDcLB_DoorTypes);
	if (listbox == NULL) { return; }

	// get the currently selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }

	// get the object type
	object_type = OBJrObject_GetType(object);

	WMrMessage_Send( listbox, LBcMessage_AddString, (UUtUns32) "<none>", 0);

	// enumerate the object specific types
	OBJrObject_Enumerate( object, OWiListBox_DoorType_EnumCallback, (UUtUns32)listbox);

	// enum the textures
	menu_item_data.flags	= WMcMenuItemFlag_Enabled;
	menu_item_data.id		= 0;
	UUrString_Copy(menu_item_data.title, "<default>", WMcMaxTitleLength);

	listbox = WMrDialog_GetItemByID(inDialog, PDcPM_Texture0 );
	if (listbox == NULL) { return; }
	WMrPopupMenu_AppendItem(listbox, &menu_item_data);

	listbox = WMrDialog_GetItemByID(inDialog, PDcPM_Texture1 );
	if (listbox == NULL) { return; }
	WMrPopupMenu_AppendItem(listbox, &menu_item_data);

	OBJrObjectUtil_EnumerateTemplate( ONcDoorTexturePrefix, M3cTemplate_TextureMap, OWiListBox_DoorTextures_EnumCallback, (UUtUns32) inDialog );

	osd = (OBJtOSD_All*)UUrMemory_Block_NewClear(sizeof(OBJtOSD_All));
	UUmAssert(osd);
	OBJrObject_GetObjectSpecificData( object, osd );

	WMrDialog_SetUserData(inDialog, (UUtUns32)osd);

	// set the maximum number of characters in the editfields
	editfield = WMrDialog_GetItemByID(inDialog, PDcEF_ID);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, 6, 0);

	// set the keyboard focus to the listbox
	WMrWindow_SetFocus(listbox);

	// hilite the item currently in use
	OWiProp_Door_SelectItem(inDialog);

	// set the fields
	OWiProp_Door_SetFields(inDialog);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtError OWiProp_Door_SaveOSD( WMtDialog *inDialog )
{
	OBJtObject			*object;
	WMtWindow			*window;
	OBJtOSD_All			osd_all;
	UUtUns32			temp_int;
	float				temp_float;
	char				string[OBJcMaxNoteChars + 1];
	UUtUns32			result;
	OBJtOSD_Door		*door_osd;
	UUtUns16			id;

	if (OBJrObjectType_IsLocked(OBJcType_Door) == UUcTrue)
	{
		WMrDialog_MessageBox(inDialog, "Error", "The Door file is locked.", WMcMessageBoxStyle_OK);
		return UUcError_None;
	}

	// get a pointer to the object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	UUmAssert(object);

	// grab the temp OSD
	door_osd = (OBJtOSD_Door*)WMrDialog_GetUserData(inDialog);
	osd_all.osd.door_osd = *door_osd;

	// get the selected item text from the listbox
	window = WMrDialog_GetItemByID(inDialog, PDcLB_DoorTypes);			UUmAssert( window );
	result = WMrMessage_Send( window, LBcMessage_GetSelection, 0, 0 );
	if( result != LBcError )
	{
		if( result == 0 )
		{
			osd_all.osd.door_osd.door_class_name[0] = 0;
		}
		else
		{
			result = WMrMessage_Send(window, LBcMessage_GetText, (UUtUns32)string, (UUtUns32)(-1));
			UUrString_Copy( osd_all.osd.door_osd.door_class_name, string, OBJcMaxNameLength);
		}
	}

	// id
	window = WMrDialog_GetItemByID(inDialog, PDcEF_ID);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 6);
	sscanf(string, "%d", &temp_int );
	osd_all.osd.door_osd.id	= (UUtUns16) temp_int;

	// key id
	window = WMrDialog_GetItemByID(inDialog, PDcEF_KeyID);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 6);
	sscanf(string, "%d", &temp_int );
	osd_all.osd.door_osd.key_id	= (UUtUns16) temp_int;

	// activation radius
	window = WMrDialog_GetItemByID(inDialog, PDcEF_ActivationRadius);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 6);
	sscanf(string, "%f", &temp_float );
	//temp_float *= _FootToDist;
	temp_float = UUmFeet( temp_float );
	osd_all.osd.door_osd.activation_radius_squared = temp_float * temp_float;

	// Locked
	window = WMrDialog_GetItemByID(inDialog, PDcCB_Locked);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.door_osd.flags |= OBJcDoorFlag_Locked;
	else
		osd_all.osd.door_osd.flags &= ~OBJcDoorFlag_Locked;

	// Initial Locked
	window = WMrDialog_GetItemByID(inDialog, PDcCB_InitialLocked);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.door_osd.flags |= OBJcDoorFlag_InitialLocked;
	else
		osd_all.osd.door_osd.flags &= ~OBJcDoorFlag_InitialLocked;

	// Double doors
	window = WMrDialog_GetItemByID(inDialog, PDcCB_DoubleDoors);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.door_osd.flags |= OBJcDoorFlag_DoubleDoors;
	else
		osd_all.osd.door_osd.flags &= ~OBJcDoorFlag_DoubleDoors;

	// Manual Door
	window = WMrDialog_GetItemByID(inDialog, PDcCB_ManualDoor);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.door_osd.flags |= OBJcDoorFlag_Manual;
	else
		osd_all.osd.door_osd.flags &= ~OBJcDoorFlag_Manual;

	// One way Door
	window = WMrDialog_GetItemByID(inDialog, PDcCB_OneWay);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.door_osd.flags |= OBJcDoorFlag_OneWay;
	else
		osd_all.osd.door_osd.flags &= ~OBJcDoorFlag_OneWay;

	// Reversed Door
	window = WMrDialog_GetItemByID(inDialog, PDcCB_Reverse);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.door_osd.flags |= OBJcDoorFlag_Reverse;
	else
		osd_all.osd.door_osd.flags &= ~OBJcDoorFlag_Reverse;

	// Test mode
	window = WMrDialog_GetItemByID(inDialog, PDcCB_Test);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.door_osd.flags |= OBJcDoorFlag_TestMode;
	else
		osd_all.osd.door_osd.flags &= ~OBJcDoorFlag_TestMode;

	// mirror door
	window = WMrDialog_GetItemByID(inDialog, PDcCB_Mirror);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.door_osd.flags |= OBJcDoorFlag_Mirror;
	else
		osd_all.osd.door_osd.flags &= ~OBJcDoorFlag_Mirror;

	// grab the textures
	window = WMrDialog_GetItemByID(inDialog, PDcPM_Texture0);
	WMrPopupMenu_GetItemID(window, -1, &id);
	if( id == 0 )
	{
		osd_all.osd.door_osd.door_texture[0][0]	= 0;
	}
	else
	{
		WMrPopupMenu_GetItemText(window, id, string );
		sprintf( osd_all.osd.door_osd.door_texture[0], "%s%s", ONcDoorTexturePrefix, string );
	}

	window = WMrDialog_GetItemByID(inDialog, PDcPM_Texture1);
	WMrPopupMenu_GetItemID(window, -1, &id);
	if( id == 0 )
	{
		osd_all.osd.door_osd.door_texture[1][0]	= 0;
	}
	else
	{
		WMrPopupMenu_GetItemText(window, id, string );
		sprintf( osd_all.osd.door_osd.door_texture[1], "%s%s", ONcDoorTexturePrefix, string );
	}

	// save the osd
	OWrObjectProperties_SetOSD(window, object, &osd_all);

	return UUcError_None;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Door_HandleCommand(	WMtDialog *inDialog, UUtUns32 inParam1,	UUtUns32 inParam2 )
{
	OBJtObject			*object;
	OBJtObjectType		object_type;
	OBJtOSD_All			*osd;
	OBJtOSD_Door		*door_osd;

	// get a pointer to the osd
	osd = (OBJtOSD_All*)WMrDialog_GetUserData(inDialog);
	UUmAssert(osd);

	door_osd = (OBJtOSD_Door*) osd;

	// get the object and object type
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }
	object_type = OBJrObject_GetType(object);

	switch (UUmLowWord(inParam1))
	{
		case PDcBtn_NewEvent:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				OWrEventList_NewEvent( &osd->osd.door_osd.event_list, WMrDialog_GetItemByID(inDialog, PDcLB_Events) );
			}
			break;
		case PDcBtn_DelEvent:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				OWrEventList_DeleteEvent( &osd->osd.door_osd.event_list, WMrDialog_GetItemByID(inDialog, PDcLB_Events) );
			}
			break;
		case PDcBtn_EditEvent:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				OWrEventList_EditEvent( &osd->osd.door_osd.event_list, WMrDialog_GetItemByID(inDialog, PDcLB_Events) );
			}
			break;
		case PDcLB_Events:
			if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
			{
				OWrEventList_EditEvent( &osd->osd.door_osd.event_list, WMrDialog_GetItemByID(inDialog, PDcLB_Events) );
			}
		break;

		case PDcLB_DoorTypes:
			if (UUmHighWord(inParam1) == LBcNotify_SelectionChanged)
			{
				WMtWindow			*listbox;
				char				type_name[256];
				OBJtOSD_All			osd_all;
				UUtUns32			result;

				listbox = WMrDialog_GetItemByID(inDialog, PDcLB_DoorTypes);			UUmAssert( listbox );

				result = WMrMessage_Send( listbox, LBcMessage_GetSelection, 0, 0 );
				if( result == LBcError )		break;
				if( result == 0 )
				{
					osd_all.osd.door_osd = osd->osd.door_osd;
					osd_all.osd.door_osd.door_class_name[0] = 0;
				}
				else
				{
					result = WMrMessage_Send( listbox, LBcMessage_GetText, (UUtUns32)type_name, (UUtUns32)(-1));
					if (result == LBcError)		break;

					osd_all.osd.door_osd = osd->osd.door_osd;
					UUrString_Copy( osd_all.osd.door_osd.door_class_name, type_name, OBJcMaxNameLength );
				}
				// set the object specific data
				OWrObjectProperties_SetOSD(inDialog, object, &osd_all);
			}
			else if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
			{
				WMrDialog_ModalEnd(inDialog, UUcTrue);
			}
		break;

		case PDcBtn_Revert:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				// set the object specific data
				OWrObjectProperties_SetOSD(inDialog, object, osd);

				// hilite the item currently in use
				OWiProp_Door_SelectItem(inDialog);
			}
		break;

		case PDcBtn_Cancel:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				WMrDialog_ModalEnd(inDialog, UUcFalse);
			}
			break;
		case PDcBtn_Save:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				// save the object specific data
				//error =
					OWiProp_Door_SaveOSD(inDialog);
				//if (error == UUcError_None)
				{
					OBJrSaveObjects(OBJcType_Door);
					WMrDialog_ModalEnd(inDialog, UUcTrue);
				}
			}
		break;
	}
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Door_HandleMenuCommand( WMtDialog *inDialog, UUtUns32 inParam1, UUtUns32 inParam2 )
{
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool OWiProp_Door_Callback( WMtDialog *inDialog, WMtMessage inMessage, UUtUns32 inParam1, UUtUns32 inParam2 )
{
	UUtBool				handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiProp_Door_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			OWgDialog_Prop = NULL;
		return UUcFalse;

		case WMcMessage_Command:
			OWiProp_Door_HandleCommand(inDialog, inParam1, inParam2);
		break;

		case WMcMessage_MenuCommand:
			OWiProp_Door_HandleMenuCommand(inDialog, inParam1, inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
// trigger properties
// ----------------------------------------------------------------------

#if TOOL_VERSION
static void OWiProp_Trigger_SetFields( WMtDialog *inDialog )
{
	OBJtOSD_All			*osd_all;
	WMtWindow			*window;
	char				string[255];
	float				temp_float;
	OBJtObject			*object;

	// get the currently selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }

	// get the Trigger properties
	osd_all = (OBJtOSD_All*)WMrDialog_GetUserData(inDialog);
	UUmAssert(osd_all);
	//UUmAssert(object->object_data == osd_all);

	// update the controls
	// ID
	window = WMrDialog_GetItemByID(inDialog, PTcEF_ID);						UUmAssert( window );
	sprintf(string, "%d", osd_all->osd.trigger_osd.id );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// Start Offset
	window = WMrDialog_GetItemByID(inDialog, PTcEF_StartOffset);			UUmAssert( window );
	sprintf(string, "%4.2f", osd_all->osd.trigger_osd.start_offset );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// get the speed
	window = WMrDialog_GetItemByID(inDialog, PTcEF_Speed);					UUmAssert( window );
	sprintf(string, "%4.2f", osd_all->osd.trigger_osd.persistant_anim_scale );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// reverse
	window = WMrDialog_GetItemByID(inDialog, PTcCB_Reverse);				UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.trigger_osd.flags & OBJcTriggerFlag_ReverseAnim) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// PONG!
	window = WMrDialog_GetItemByID(inDialog, PTcCB_PingPong);				UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.trigger_osd.flags & OBJcTriggerFlag_PingPong) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// active
	window = WMrDialog_GetItemByID(inDialog, PTcCB_Active);					UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.trigger_osd.flags & OBJcTriggerFlag_Active) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// initial active
	window = WMrDialog_GetItemByID(inDialog, PTcCB_InitialActive);			UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.trigger_osd.flags & OBJcTriggerFlag_InitialActive) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// time on
	window = WMrDialog_GetItemByID(inDialog, PTcEF_TimeOn);					UUmAssert( window );
	temp_float = (float)osd_all->osd.trigger_osd.time_on / (float)UUcFramesPerSecond;
	sprintf(string, "%4.2f", temp_float );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// time off
	window = WMrDialog_GetItemByID(inDialog, PTcEF_TimeOff);				UUmAssert( window );
	temp_float = (float)osd_all->osd.trigger_osd.time_off / (float)UUcFramesPerSecond;
	sprintf(string, "%4.2f", temp_float );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// color
	window = WMrDialog_GetItemByID(inDialog, PTcEF_Color);					UUmAssert( window );
	sprintf(string, "%08X", osd_all->osd.trigger_osd.color );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// emitter count
	window = WMrDialog_GetItemByID(inDialog, PTcEF_EmitterCount);			UUmAssert( window );
	sprintf(string, "%d", osd_all->osd.trigger_osd.emitter_count );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// fill event list
	window = WMrDialog_GetItemByID( inDialog, PTcLB_Events );				UUmAssert( window );
	ONrEventList_FillListBox( &osd_all->osd.trigger_osd.event_list, window );

	// enable OK button
	window = WMrDialog_GetItemByID(inDialog, WMcDialogItem_OK);
	WMrWindow_SetEnabled(window, !OBJrObjectType_IsLocked(OBJcType_Trigger));
}
#endif

#if TOOL_VERSION
static UUtBool OWiListBox_TriggerType_EnumCallback( const char *inName, UUtUns32 inUserData )
{
	WMtWindow				*listbox;

	// get the list box
	listbox = (WMtWindow*)inUserData;
	if (listbox == NULL) { return UUcFalse; }

	// add the character to the list
	WMrMessage_Send( listbox, LBcMessage_AddString, (UUtUns32) inName, 0);

	return UUcTrue;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Trigger_SelectItem(	WMtDialog *inDialog )
{
	WMtWindow			*listbox;
	OBJtObject			*object;
	OBJtOSD_Trigger		*trigger_osd;

	// get a pointer to the osd
	trigger_osd = (OBJtOSD_Trigger *)WMrDialog_GetUserData(inDialog);
	UUmAssert(trigger_osd);

	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, PTcLB_TriggerTypes);
	if (listbox == NULL) { return; }

	// get the currently selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }

	// select the appropriate item
	WMrMessage_Send( listbox, LBcMessage_SelectString, (UUtUns32)(-1), (UUtUns32) trigger_osd->trigger_class_name);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Trigger_InitDialog(	WMtDialog *inDialog )
{

	WMtWindow			*listbox;
	OBJtObject			*object;
	OBJtObjectType		object_type;
	OBJtOSD_All			*osd;
	WMtWindow			*editfield;

	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, PTcLB_TriggerTypes);
	if (listbox == NULL) { return; }

	// get the currently selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }

	// get the object type
	object_type = OBJrObject_GetType(object);

	// enumerate the object specific types
	OBJrObject_Enumerate( object, OWiListBox_TriggerType_EnumCallback, (UUtUns32)listbox);

	osd = (OBJtOSD_All*)UUrMemory_Block_NewClear(sizeof(OBJtOSD_All));
	UUmAssert(osd);
	OBJrObject_GetObjectSpecificData( object, osd );

	WMrDialog_SetUserData(inDialog, (UUtUns32)osd);

	// set the maximum number of characters in the editfields
	editfield = WMrDialog_GetItemByID(inDialog, PTcEF_ID);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, 6, 0);

	editfield = WMrDialog_GetItemByID(inDialog, PTcEF_Speed);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, 6, 0);

	editfield = WMrDialog_GetItemByID(inDialog, PTcEF_StartOffset);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, 6, 0);

	editfield = WMrDialog_GetItemByID(inDialog, PTcEF_TimeOn);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, 6, 0);

	editfield = WMrDialog_GetItemByID(inDialog, PTcEF_TimeOff);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, 6, 0);

	editfield = WMrDialog_GetItemByID(inDialog, PTcEF_Color);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, 12, 0);

	editfield = WMrDialog_GetItemByID(inDialog, PTcEF_EmitterCount);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, 4, 0);

	// set the keyboard focus to the listbox
	WMrWindow_SetFocus(listbox);

	// hilite the item currently in use
	OWiProp_Trigger_SelectItem(inDialog);

	// set the fields
	OWiProp_Trigger_SetFields(inDialog);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtError OWiProp_Trigger_SaveOSD(	WMtDialog *inDialog )
{
	OBJtObject			*object;
	WMtWindow			*window;
	OBJtOSD_All			osd_all;
	float				temp;
	UUtUns32			temp_int;
	char				string[OBJcMaxNoteChars + 1];

	OBJtOSD_Trigger		*trigger_osd;

	if (OBJrObjectType_IsLocked(OBJcType_Trigger) == UUcTrue)
	{
		WMrDialog_MessageBox(inDialog, "Error", "The Trigger file is locked.", WMcMessageBoxStyle_OK);
		return UUcError_None;
	}

	// get a pointer to the object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	UUmAssert(object);

	// grab the temp OSD
	trigger_osd = (OBJtOSD_Trigger*)WMrDialog_GetUserData(inDialog);
	osd_all.osd.trigger_osd = *trigger_osd;

	// get the selected item text from the listbox
	window = WMrDialog_GetItemByID(inDialog, PTcLB_TriggerTypes);
	WMrMessage_Send(window, LBcMessage_GetText, (UUtUns32)string, (UUtUns32)(-1));
	UUrString_Copy( osd_all.osd.trigger_osd.trigger_class_name, string, OBJcMaxNameLength);

	// get the id
	window = WMrDialog_GetItemByID(inDialog, PTcEF_ID);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 6);
	sscanf(string, "%d", &temp_int );
	osd_all.osd.trigger_osd.id	= (UUtUns16) temp_int;

	// get the speed
	window = WMrDialog_GetItemByID(inDialog, PTcEF_Speed);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 6);
	sscanf(string, "%f", &osd_all.osd.trigger_osd.persistant_anim_scale );

	// get the starting offset
	window = WMrDialog_GetItemByID(inDialog, PTcEF_StartOffset);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 6);
	sscanf(string, "%f", &osd_all.osd.trigger_osd.start_offset );

	// get the reversal
	window = WMrDialog_GetItemByID(inDialog, PTcCB_Reverse);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.trigger_osd.flags |= OBJcTriggerFlag_ReverseAnim;
	else
		osd_all.osd.trigger_osd.flags &= ~OBJcTriggerFlag_ReverseAnim;

	// get the pingpong
	window = WMrDialog_GetItemByID(inDialog, PTcCB_PingPong);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.trigger_osd.flags |= OBJcTriggerFlag_PingPong;
	else
		osd_all.osd.trigger_osd.flags &= ~OBJcTriggerFlag_PingPong;

	// get the active state
	window = WMrDialog_GetItemByID(inDialog, PTcCB_Active);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.trigger_osd.flags |= OBJcTriggerFlag_Active;
	else
		osd_all.osd.trigger_osd.flags &= ~OBJcTriggerFlag_Active;

	// get the initial active state
	window = WMrDialog_GetItemByID(inDialog, PTcCB_InitialActive);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.trigger_osd.flags |= OBJcTriggerFlag_InitialActive;
	else
		osd_all.osd.trigger_osd.flags &= ~OBJcTriggerFlag_InitialActive;

	// get the time on
	window = WMrDialog_GetItemByID(inDialog, PTcEF_TimeOn);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 6);
	sscanf(string, "%f", &temp );
	osd_all.osd.trigger_osd.time_on = (UUtUns16) ( temp * UUcFramesPerSecond );

	// get the time off
	window = WMrDialog_GetItemByID(inDialog, PTcEF_TimeOff);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 6);
	sscanf(string, "%f", &temp );
	osd_all.osd.trigger_osd.time_off = (UUtUns16) ( temp * UUcFramesPerSecond );

	// get the color
	window = WMrDialog_GetItemByID(inDialog, PTcEF_Color);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 12 );
	sscanf(string, "%x", &temp_int );
	osd_all.osd.trigger_osd.color = temp_int;

	// get the emitter count
	window = WMrDialog_GetItemByID(inDialog, PTcEF_EmitterCount);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 6);
	sscanf(string, "%d", &temp_int );
	osd_all.osd.trigger_osd.emitter_count = (UUtUns16) temp_int;

	// save the osd
	OWrObjectProperties_SetOSD(window, object, &osd_all);

	return UUcError_None;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Trigger_HandleCommand(	WMtDialog *inDialog, UUtUns32 inParam1,	UUtUns32 inParam2 )
{
	OBJtObject			*object;
	OBJtObjectType		object_type;
	OBJtOSD_All			*osd;
	OBJtOSD_Trigger		*trigger_osd;

	// get a pointer to the osd
	osd = (OBJtOSD_All*)WMrDialog_GetUserData(inDialog);
	UUmAssert(osd);

	trigger_osd = (OBJtOSD_Trigger*) osd;

	// get the object and object type
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }
	object_type = OBJrObject_GetType(object);

	switch (UUmLowWord(inParam1))
	{
		case PTcBtn_NewEvent:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				OWrEventList_NewEvent( &osd->osd.trigger_osd.event_list, WMrDialog_GetItemByID(inDialog, PTcLB_Events) );
			}
			break;
		case PTcBtn_DelEvent:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				OWrEventList_DeleteEvent( &osd->osd.trigger_osd.event_list, WMrDialog_GetItemByID(inDialog, PTcLB_Events) );
			}
			break;
		case PTcBtn_EditEvent:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				OWrEventList_EditEvent( &osd->osd.trigger_osd.event_list, WMrDialog_GetItemByID(inDialog, PTcLB_Events) );
			}
			break;
		case PTcLB_Events:
			if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
			{
				OWrEventList_EditEvent( &osd->osd.trigger_osd.event_list, WMrDialog_GetItemByID(inDialog, PTcLB_Events) );
			}
		break;

		case PTcLB_TriggerTypes:
			if (UUmHighWord(inParam1) == LBcNotify_SelectionChanged)
			{
				WMtWindow			*listbox;
				char				type_name[256];
				OBJtOSD_All			osd_all;
				UUtUns32			result;

				// grab the box
				listbox = WMrDialog_GetItemByID(inDialog, PTcLB_TriggerTypes);			UUmAssert( listbox );
				if (listbox == NULL) { return; }

				// get the selected item text from the listbox
				result = WMrMessage_Send( listbox, LBcMessage_GetText, (UUtUns32)type_name, (UUtUns32)(-1));
				if (result == LBcError) { break; }

				// set the OSD to reflect the change
				osd_all.osd.trigger_osd = osd->osd.trigger_osd;
				UUrString_Copy( osd_all.osd.trigger_osd.trigger_class_name, type_name, OBJcMaxNameLength );

				// set the object specific data
				OWrObjectProperties_SetOSD(inDialog, object, &osd_all);
			}
			else if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
			{
				WMrDialog_ModalEnd(inDialog, UUcTrue);
			}
		break;

		case PTcBtn_Revert:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				// set the object specific data
				OWrObjectProperties_SetOSD(inDialog, object, osd);

				// hilite the item currently in use
				OWiProp_Trigger_SelectItem(inDialog);
			}
		break;

		case PTcBtn_Cancel:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				WMrDialog_ModalEnd(inDialog, UUcFalse);
			}
			break;
		case PTcBtn_Save:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				// save the object specific data
				//error =
					OWiProp_Trigger_SaveOSD(inDialog);
				//if (error == UUcError_None)
				{
					OBJrSaveObjects(OBJcType_Trigger);
					WMrDialog_ModalEnd(inDialog, UUcTrue);
				}
			}
		break;
	}
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Trigger_HandleMenuCommand(
	WMtDialog			*inDialog,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	//IMtShade			shade;
	// get the shade
	//shade = OWiColorPopup_GetShadeFromID(UUmLowWord(inParam1));

}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool
OWiProp_Trigger_Callback(
	WMtDialog			*inDialog,
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	UUtBool				handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiProp_Trigger_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			OWgDialog_Prop = NULL;
		return UUcFalse;

		case WMcMessage_Command:
			OWiProp_Trigger_HandleCommand(inDialog, inParam1, inParam2);
		break;

		case WMcMessage_MenuCommand:
			OWiProp_Trigger_HandleMenuCommand(inDialog, inParam1, inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Console_SetFields( WMtDialog *inDialog )
{
	OBJtOSD_All			*osd_all;
	WMtWindow			*window;
	char				string[255];

	// get the Console properties
	osd_all = (OBJtOSD_All*)WMrDialog_GetUserData(inDialog);
	UUmAssert(osd_all);

	// update the controls

	// ID
	window = WMrDialog_GetItemByID(inDialog, PCONcEF_ID);
	UUmAssert( window );
	sprintf(string, "%d", osd_all->osd.console_osd.id );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// active
	window = WMrDialog_GetItemByID(inDialog, PCONcCB_Active);
	UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.console_osd.flags & OBJcConsoleFlag_Active) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// initial active
	window = WMrDialog_GetItemByID(inDialog, PCONcCB_InitialActive);
	WMrCheckBox_SetCheck(window, ((osd_all->osd.console_osd.flags & OBJcConsoleFlag_InitialActive) ? UUcTrue : UUcFalse));

	// punchable
	window = WMrDialog_GetItemByID(inDialog, PCONcCB_Punch);
	WMrCheckBox_SetCheck(window, ((osd_all->osd.console_osd.flags & OBJcConsoleFlag_Punch) ? UUcTrue : UUcFalse));

	// is alarm
	window = WMrDialog_GetItemByID(inDialog, PCONcCB_IsAlarm);
	WMrCheckBox_SetCheck(window, ((osd_all->osd.console_osd.flags & OBJcConsoleFlag_IsAlarm) ? UUcTrue : UUcFalse));

	// triggered
	window = WMrDialog_GetItemByID(inDialog, PCONcCB_Triggered);
	UUmAssert( window );
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.console_osd.flags & OBJcConsoleFlag_Triggered) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// set the popup menus
	window = WMrDialog_GetItemByID(inDialog, PCONcPM_InactiveScreen);
	UUmAssert( window );
	if( strlen( osd_all->osd.console_osd.screen_inactive ) > 4 ) {
		WMrPopupMenu_SelectString_NoCase( window, &osd_all->osd.console_osd.screen_inactive[4] );
	}
	else {
		WMrPopupMenu_SelectString_NoCase( window, "<default>" );
	}

	window = WMrDialog_GetItemByID(inDialog, PCONcPM_ActiveScreen);
	UUmAssert( window );
	if( strlen( osd_all->osd.console_osd.screen_active ) > 4 ) {
		WMrPopupMenu_SelectString_NoCase( window, &osd_all->osd.console_osd.screen_active[4] );
	}
	else {
		WMrPopupMenu_SelectString_NoCase( window, "<default>" );
	}

	window = WMrDialog_GetItemByID(inDialog, PCONcPM_TriggeredScreen);
	UUmAssert( window );
	if( strlen( osd_all->osd.console_osd.screen_triggered ) > 4 ) {
		WMrPopupMenu_SelectString_NoCase( window, &osd_all->osd.console_osd.screen_triggered[4] );
	}
	else {
		WMrPopupMenu_SelectString_NoCase( window, "<default>" );
	}

	// fill event list
	window = WMrDialog_GetItemByID( inDialog, PCONcLB_Events );
	UUmAssert( window );
	ONrEventList_FillListBox( &osd_all->osd.console_osd.event_list, window );

	// enable the ok button
	window = WMrDialog_GetItemByID(inDialog, WMcDialogItem_OK);
	UUmAssert( window );
	WMrWindow_SetEnabled(window, UUcTrue);

	return;
}
#endif

#if TOOL_VERSION
static UUtBool OWiListBox_ConsoleScreens_EnumCallback( const char *inName, UUtUns32 inUserData )
{
	WMtWindow				*window;
	WMtDialog				*dialog;
	WMtMenuItemData			menu_item_data;
	UUtError				error;
	static int				id = 1;

	dialog = (WMtDialog*)inUserData;
	if (dialog == NULL) { return UUcFalse; }

	menu_item_data.flags	= WMcMenuItemFlag_Enabled;
	menu_item_data.id		= id++;
	UUrString_Copy(menu_item_data.title, inName, WMcMaxTitleLength);

	// fill the screen list
	window = WMrDialog_GetItemByID( dialog, PCONcPM_InactiveScreen );		UUmAssert( window );
	error = WMrPopupMenu_AppendItem(window, &menu_item_data);

	window = WMrDialog_GetItemByID( dialog, PCONcPM_ActiveScreen );			UUmAssert( window );
	error = WMrPopupMenu_AppendItem(window, &menu_item_data);

	window = WMrDialog_GetItemByID( dialog, PCONcPM_TriggeredScreen );		UUmAssert( window );
	error = WMrPopupMenu_AppendItem(window, &menu_item_data);

	return UUcTrue;
}
#endif

#if TOOL_VERSION
static UUtBool OWiListBox_ConsoleType_EnumCallback( const char *inName, UUtUns32 inUserData )
{
	WMtWindow				*listbox;
	// get the list box
	listbox = (WMtWindow*)inUserData;
	if (listbox == NULL) { return UUcFalse; }

	// add the character to the list
	WMrMessage_Send( listbox, LBcMessage_AddString, (UUtUns32) inName, 0);

	return UUcTrue;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Console_SelectItem( WMtDialog *inDialog )
{
	WMtWindow				*listbox;
	OBJtObject				*object;
	OBJtOSD_Console			*console_osd;

	// get a pointer to the osd
	console_osd = (OBJtOSD_Console *) WMrDialog_GetUserData(inDialog);
	UUmAssert(console_osd);

	// get the currently selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }

	// set the type list
	listbox = WMrDialog_GetItemByID(inDialog, PCONcLB_ConsoleTypes);
	UUmAssert( listbox );
	WMrMessage_Send( listbox, LBcMessage_SelectString, (UUtUns32)(-1), (UUtUns32) console_osd->console_class_name);

}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Console_InitDialog(
	WMtDialog			*inDialog)
{
	WMtWindow				*window;
	WMtWindow				*listbox;
	OBJtObject				*object;
	OBJtOSD_All				*osd;
	UUtError				error;
	WMtMenuItemData			menu_item_data;

	// get the currently selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }

	// add default values to screen lists
	menu_item_data.flags	= WMcMenuItemFlag_Enabled;
	menu_item_data.id		= 0;
	UUrString_Copy(menu_item_data.title, "<default>", WMcMaxTitleLength);

	listbox = WMrDialog_GetItemByID( inDialog, PCONcPM_InactiveScreen );	UUmAssert( listbox );
	error = WMrPopupMenu_AppendItem(listbox, &menu_item_data);

	listbox = WMrDialog_GetItemByID( inDialog, PCONcPM_ActiveScreen );		UUmAssert( listbox );
	error = WMrPopupMenu_AppendItem(listbox, &menu_item_data);

	listbox = WMrDialog_GetItemByID( inDialog, PCONcPM_TriggeredScreen );	UUmAssert( listbox );
	error = WMrPopupMenu_AppendItem(listbox, &menu_item_data);

	// fill the type list
	window = WMrDialog_GetItemByID(inDialog, PCONcLB_ConsoleTypes);
	if (window == NULL) { return; }
	error = OBJrObject_Enumerate( object, OWiListBox_ConsoleType_EnumCallback, (UUtUns32)window );

	error =	OBJrObjectUtil_EnumerateTemplate( ONcConsoleTexturePrefix, M3cTemplate_TextureMap, OWiListBox_ConsoleScreens_EnumCallback, (UUtUns32) inDialog );

	// setup the OSD of the new object
	osd = (OBJtOSD_All*)UUrMemory_Block_NewClear(sizeof(OBJtOSD_All));
	UUmAssert(osd);
	OBJrObject_GetObjectSpecificData( object, osd );
	WMrDialog_SetUserData(inDialog, (UUtUns32)osd);

	// set the maximum number of characters in the editfields
	// ID
	window = WMrDialog_GetItemByID(inDialog, PCONcEF_ID);
	WMrMessage_Send(window, EFcMessage_SetMaxChars, 6, 0);

	// set the keyboard focus to the listbox
	window = WMrDialog_GetItemByID(inDialog, PCONcLB_ConsoleTypes);
	if (window == NULL) { return; }
	WMrWindow_SetFocus(window);

	// hilite the item currently in use
	OWiProp_Console_SelectItem(inDialog);

	// set the fields
	OWiProp_Console_SetFields(inDialog);

	return;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtError OWiProp_Console_SaveOSD( WMtDialog *inDialog)
{
	OBJtObject			*object;
	WMtWindow			*window;
	OBJtOSD_All			osd_all;
	char				string[OBJcMaxNoteChars + 1];
	OBJtOSD_Console		*console_osd;
	UUtUns16			id;

	if (OBJrObjectType_IsLocked(OBJcType_Console) == UUcTrue)
	{
		WMrDialog_MessageBox(inDialog, "Error", "The Console file is locked.", WMcMessageBoxStyle_OK);
		return UUcError_None;
	}

	// get a pointer to the object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	UUmAssert(object);

	// grab the temp OSD
	console_osd = (OBJtOSD_Console*)WMrDialog_GetUserData(inDialog);
	osd_all.osd.console_osd = *console_osd;

	// get the id
	window = WMrDialog_GetItemByID(inDialog, PCONcEF_ID);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 6);
	sscanf(string, "%d", &osd_all.osd.console_osd.id );

	// get the selected item text from the listbox
	window = WMrDialog_GetItemByID(inDialog, PCONcLB_ConsoleTypes);
	WMrMessage_Send(window, LBcMessage_GetText, (UUtUns32)string, (UUtUns32)(-1));
	UUrString_Copy( osd_all.osd.console_osd.console_class_name, string, OBJcMaxNameLength);

	// get the active state
	window = WMrDialog_GetItemByID(inDialog, PCONcCB_Active);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.console_osd.flags |= OBJcConsoleFlag_Active;
	else
		osd_all.osd.console_osd.flags &= ~OBJcConsoleFlag_Active;

	// get the initial active state
	window = WMrDialog_GetItemByID(inDialog, PCONcCB_InitialActive);
	if (WMrCheckBox_GetCheck(window)) {
		osd_all.osd.console_osd.flags |= OBJcConsoleFlag_InitialActive;
	}
	else {
		osd_all.osd.console_osd.flags &= ~OBJcConsoleFlag_InitialActive;
	}

	// get the punch state
	window = WMrDialog_GetItemByID(inDialog, PCONcCB_Punch);
	if (WMrCheckBox_GetCheck(window)) {
		osd_all.osd.console_osd.flags |= OBJcConsoleFlag_Punch;
	}
	else {
		osd_all.osd.console_osd.flags &= ~OBJcConsoleFlag_Punch;
	}

	// get the alarm state
	window = WMrDialog_GetItemByID(inDialog, PCONcCB_IsAlarm);
	if (WMrCheckBox_GetCheck(window)) {
		osd_all.osd.console_osd.flags |= OBJcConsoleFlag_IsAlarm;
	}
	else {
		osd_all.osd.console_osd.flags &= ~OBJcConsoleFlag_IsAlarm;
	}

	// get the triggered state
	window = WMrDialog_GetItemByID(inDialog, PCONcCB_Triggered);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.console_osd.flags |= OBJcConsoleFlag_Triggered;
	else
		osd_all.osd.console_osd.flags &= ~OBJcConsoleFlag_Triggered;

	// grab the screens
	window = WMrDialog_GetItemByID(inDialog, PCONcPM_ActiveScreen);
	WMrPopupMenu_GetItemID(window, -1, &id);
	if( id == 0 )
	{
		osd_all.osd.console_osd.screen_active[0] = 0;
	}
	else
	{
		WMrPopupMenu_GetItemText(window, id, string );
		sprintf( osd_all.osd.console_osd.screen_active, "_con%s", string );
	}

	window = WMrDialog_GetItemByID(inDialog, PCONcPM_InactiveScreen);
	WMrPopupMenu_GetItemID(window, -1, &id);
	if( id == 0 )
	{
		osd_all.osd.console_osd.screen_inactive[0] = 0;
	}
	else
	{
		WMrPopupMenu_GetItemText(window, id, string );
		sprintf( osd_all.osd.console_osd.screen_inactive, "_con%s", string );
	}

	window = WMrDialog_GetItemByID(inDialog, PCONcPM_TriggeredScreen);
	WMrPopupMenu_GetItemID(window, -1, &id);
	if( id == 0 )
	{
		osd_all.osd.console_osd.screen_triggered[0] = 0;
	}
	else
	{
		WMrPopupMenu_GetItemText(window, id, string );
		sprintf( osd_all.osd.console_osd.screen_triggered, "_con%s", string );
	}

	OWrObjectProperties_SetOSD(window, object, &osd_all);

	return UUcError_None;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Console_HandleCommand(
	WMtDialog			*inDialog,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	OBJtObject			*object;
	WMtWindow			*listbox;
	OBJtObjectType		object_type;
	OBJtOSD_All			*osd;

	// get a pointer to the osd
	osd = (OBJtOSD_All*)WMrDialog_GetUserData(inDialog);
	UUmAssert(osd);

	// get the object and object type
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }
	object_type = OBJrObject_GetType(object);

	// get the listbox
	listbox = WMrDialog_GetItemByID(inDialog, PCONcLB_ConsoleTypes);
	if (listbox == NULL) { return; }

	switch (UUmLowWord(inParam1))
	{
		case PTcEF_ID:
			break;
		case PCONcBtn_NewEvent:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				OWrEventList_NewEvent( &osd->osd.console_osd.event_list, WMrDialog_GetItemByID(inDialog, PCONcLB_Events) );
			}
			break;
		case PCONcBtn_DelEvent:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				OWrEventList_DeleteEvent( &osd->osd.console_osd.event_list, WMrDialog_GetItemByID(inDialog, PCONcLB_Events) );
			}
			break;
		case PCONcBtn_EditEvent:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				OWrEventList_EditEvent( &osd->osd.console_osd.event_list, WMrDialog_GetItemByID(inDialog, PCONcLB_Events) );
			}
			break;
		case PCONcLB_Events:
			if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
			{
				OWrEventList_EditEvent( &osd->osd.console_osd.event_list, WMrDialog_GetItemByID(inDialog, PCONcLB_Events) );
			}
		break;

		case PCONcLB_ConsoleTypes:
			if (UUmHighWord(inParam1) == LBcNotify_SelectionChanged)
			{
				char				type_name[256];
				OBJtOSD_All			osd_all;
				UUtUns32			result;

				// get the selected item text from the listbox
				result = WMrMessage_Send( listbox, LBcMessage_GetText, (UUtUns32)type_name, (UUtUns32)(-1));
				if (result == LBcError) { break; }

				// set the OSD to reflect the change
				osd_all.osd.console_osd = osd->osd.console_osd;
				UUrString_Copy( osd_all.osd.console_osd.console_class_name, type_name, OBJcMaxNameLength);
				// set the object specific data
				OWrObjectProperties_SetOSD(inDialog, object, &osd_all);
			}
			else if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
			{
				WMrDialog_ModalEnd(inDialog, UUcTrue);
			}
		break;

		case PTcBtn_Revert:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				// set the object specific data
				OWrObjectProperties_SetOSD(inDialog, object, osd);

				// hilite the item currently in use
				OWiProp_Console_SelectItem(inDialog);
			}
		break;

		case PTcBtn_Cancel:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				WMrDialog_ModalEnd(inDialog, UUcFalse);
			}
			break;
		case PTcBtn_Save:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				// save the object specific data
				//error =
					OWiProp_Console_SaveOSD(inDialog);
				//if (error == UUcError_None)
				{
					OBJrSaveObjects(OBJcType_Console);
					WMrDialog_ModalEnd(inDialog, UUcTrue);
				}
			}
		break;
	}
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Console_HandleMenuCommand(
	WMtDialog			*inDialog,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool OWiProp_Console_Callback(
	WMtDialog			*inDialog,
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	UUtBool				handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiProp_Console_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			OWgDialog_Prop = NULL;
		return UUcFalse;

		case WMcMessage_Command:
			OWiProp_Console_HandleCommand(inDialog, inParam1, inParam2);
		break;

		case WMcMessage_MenuCommand:
			OWiProp_Console_HandleMenuCommand(inDialog, inParam1, inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Turret_SetFields( WMtDialog *inDialog)
{
	WMtWindow			*window;
	char				string[255];
	OBJtObject			*object = (OBJtObject *) WMrDialog_GetUserData(inDialog);
	OBJtOSD_All			*osd_all = (OBJtOSD_All *) object->object_data;

	// update the controls

	// ID
	window = WMrDialog_GetItemByID(inDialog, PTUcEF_ID);
	sprintf(string, "%d", osd_all->osd.turret_osd.id );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// active
	window = WMrDialog_GetItemByID(inDialog, PTUcCB_Active);
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) osd_all->osd.turret_osd.state == OBJcTurretState_Active, (UUtUns32) -1);

	// initial active
	window = WMrDialog_GetItemByID(inDialog, PTUcCB_InitialActive);
	WMrMessage_Send(window, CBcMessage_SetCheck, (UUtUns32) ((osd_all->osd.turret_osd.flags & OBJcTurretFlag_InitialActive) ? UUcTrue : UUcFalse), (UUtUns32) -1);

	// max horiz speed
	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxHorizSpeed);
	sprintf(string, "%4.2f", (float)osd_all->osd.turret_osd.turret_class->max_horiz_speed * UUcRadPerFrameToDegPerSec );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// max horiz speed
	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxVertSpeed);
	sprintf(string, "%4.2f", (float)osd_all->osd.turret_osd.turret_class->max_vert_speed * UUcRadPerFrameToDegPerSec );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// min horiz angle
	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MinHorizAngle);
	sprintf(string, "%.0f", ((float)osd_all->osd.turret_osd.turret_class->min_horiz_angle * M3cRadToDeg ));
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// max horiz angle
	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxHorizAngle);
	sprintf(string, "%.0f",  ((float)osd_all->osd.turret_osd.turret_class->max_horiz_angle * M3cRadToDeg ));
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// min vert angle
	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MinVertAngle);
	sprintf(string, "%.0f", ((float)osd_all->osd.turret_osd.turret_class->min_vert_angle * M3cRadToDeg ));
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// max vert angle
	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxVertAngle);
	sprintf(string, "%.0f", ((float)osd_all->osd.turret_osd.turret_class->max_vert_angle * M3cRadToDeg ));
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	//timeout
	window = WMrDialog_GetItemByID(inDialog, PTUcEF_Timeout);
	sprintf(string, "%.0f", ((float) osd_all->osd.turret_osd.turret_class->timeout / UUcFramesPerSecond ) );
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	//target teams
	window = WMrDialog_GetItemByID(inDialog, PTUcEF_TargetTeams);
	OWrMaskToString(osd_all->osd.turret_osd.target_teams, AIgTeamList, sizeof(string), string);
	WMrWindow_SetTitle(window, string, 255);

	// get the teams mask
	window = WMrDialog_GetItemByID(inDialog, PUTcEF_TargetTeamsNumber);
	WMrEditField_SetInt32(window, osd_all->osd.turret_osd.target_teams);
	COrConsole_Printf("set fields = %x", osd_all->osd.turret_osd.target_teams);

	// enable OK button
	//window = WMrDialog_GetItemByID(inDialog, WMcDialogItem_OK);
	//WMrWindow_SetEnabled(window, !OBJrObjectType_IsLocked(OBJcType_Turret));
	WMrWindow_SetEnabled(window, UUcTrue);
}
#endif

#if TOOL_VERSION
static UUtBool OWiListBox_TurretType_EnumCallback( const char *inName, UUtUns32 inUserData )
{
	WMtWindow				*listbox;

	// get the list box
	listbox = (WMtWindow*)inUserData;
	if (listbox == NULL) { return UUcFalse; }

	// add the character to the list
	WMrMessage_Send( listbox, LBcMessage_AddString, (UUtUns32) inName, 0);

	return UUcTrue;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Turret_SelectItem( WMtDialog *inDialog)
{
	WMtWindow			*listbox;
	OBJtObject			*object;
	OBJtOSD_All			*osd;

	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, PTUcLB_TurretTypes);
	if (listbox == NULL) { return; }

	// get the currently selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }

	osd = (OBJtOSD_All *) object->object_data;

	// select the appropriate item
	WMrMessage_Send( listbox, LBcMessage_SelectString, (UUtUns32)(-1), (UUtUns32)osd->osd.turret_osd.turret_class_name);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Turret_InitDialog( WMtDialog *inDialog)
{
	WMtWindow			*window;
	OBJtObject			*object = (OBJtObject *) WMrDialog_GetUserData(inDialog);
	OBJtObjectType		object_type = OBJrObject_GetType(object);

	// get a pointer to the listbox
	window = WMrDialog_GetItemByID(inDialog, PTUcLB_TurretTypes);		UUmAssert( window );
	if (window == NULL) { return; }

	// enumerate the object specific types
	OBJrObject_Enumerate( object, OWiListBox_TurretType_EnumCallback, (UUtUns32)window);

	// set the maximum number of characters in the editfields
	window = WMrDialog_GetItemByID(inDialog, PTUcEF_ID);
	WMrMessage_Send(window, EFcMessage_SetMaxChars, 6, 0);

	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxHorizSpeed);
	WMrMessage_Send(window, EFcMessage_SetMaxChars, 6, 0);

	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxVertSpeed);
	WMrMessage_Send(window, EFcMessage_SetMaxChars, 6, 0);

	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxHorizAngle);
	WMrMessage_Send(window, EFcMessage_SetMaxChars, 6, 0);

	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MinHorizAngle);
	WMrMessage_Send(window, EFcMessage_SetMaxChars, 6, 0);

	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxVertAngle);
	WMrMessage_Send(window, EFcMessage_SetMaxChars, 6, 0);

	window = WMrDialog_GetItemByID(inDialog, PTUcEF_MinVertAngle);
	WMrMessage_Send(window, EFcMessage_SetMaxChars, 6, 0);

	window = WMrDialog_GetItemByID(inDialog, PTUcEF_TargetTeams);
	WMrMessage_Send(window, EFcMessage_SetMaxChars, 30, 0);

	window = WMrDialog_GetItemByID(inDialog, PTUcEF_Timeout);
	WMrMessage_Send(window, EFcMessage_SetMaxChars, 6, 0);

	// set the keyboard focus to the listbox
	window = WMrDialog_GetItemByID(inDialog, PTUcLB_TurretTypes);		UUmAssert( window );
	WMrWindow_SetFocus(window);

	// hilite the item currently in use
	OWiProp_Turret_SelectItem(inDialog);

	// set the fields
	OWiProp_Turret_SetFields(inDialog);

	return;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtError OWiProp_Turret_SaveOSD( WMtDialog *inDialog)
{
	OBJtObject			*object;
	WMtWindow			*window;
	OBJtOSD_All			osd_all;
	OBJtOSD_Turret		*turret_osd;
	char				string[OBJcMaxNoteChars + 1];

	if (OBJrObjectType_IsLocked(OBJcType_Turret) == UUcTrue)
	{
		WMrDialog_MessageBox(inDialog, "Error", "The Turret file is locked.", WMcMessageBoxStyle_OK);
		return UUcError_None;
	}

	// get a pointer to the object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	UUmAssert(object);

	turret_osd = (OBJtOSD_Turret*) object->object_data;

	osd_all.osd.turret_osd = *turret_osd;

	// get the selected item text from the listbox
	window = WMrDialog_GetItemByID(inDialog, PTUcLB_TurretTypes);
	WMrMessage_Send(window, LBcMessage_GetText, (UUtUns32)string, (UUtUns32)(-1));
	UUrString_Copy( osd_all.osd.turret_osd.turret_class_name, string, OBJcMaxNameLength);

	// get the id
	window = WMrDialog_GetItemByID(inDialog, PTUcEF_ID);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, OBJcMaxNoteChars);
	sscanf(string, "%d", &osd_all.osd.turret_osd.id );

	// get the initial active state
	window = WMrDialog_GetItemByID(inDialog, PTUcCB_InitialActive);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.turret_osd.flags |= OBJcTurretFlag_InitialActive;
	else
		osd_all.osd.turret_osd.flags &= ~OBJcTurretFlag_InitialActive;

	// get the active state
	window = WMrDialog_GetItemByID(inDialog, PTUcCB_Active);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.turret_osd.state = OBJcTurretState_Active;
	else
		osd_all.osd.turret_osd.state = OBJcTurretState_Inactive;

	// get the teams mask
	window = WMrDialog_GetItemByID(inDialog, PUTcEF_TargetTeamsNumber);
	osd_all.osd.turret_osd.target_teams = (UUtUns32) WMrEditField_GetInt32(window);
	COrConsole_Printf("save = %x", osd_all.osd.turret_osd.target_teams);


	// save the data
	OWrObjectProperties_SetOSD(window, object, &osd_all);

	return UUcError_None;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void OWiProp_Turret_HandleCommand( WMtDialog *inDialog, UUtUns32 inParam1, UUtUns32 inParam2)
{
	OBJtObject			*object = (OBJtObject *) WMrDialog_GetUserData(inDialog);
	OBJtObjectType		object_type;
	const OBJtOSD_All	*osd = (OBJtOSD_All	*) object->object_data;

	// get the object and object type
	object_type = OBJrObject_GetType(object);

	switch (UUmLowWord(inParam1))
	{
		case PTUcEF_ID:
			{
			}
		break;

		case PTUcLB_TurretTypes:
			if (UUmHighWord(inParam1) == LBcNotify_SelectionChanged)
			{
				char				type_name[256];
				char				string[256];
				OBJtOSD_All			osd_all;
				UUtUns32			result;
				WMtWindow			*listbox;
				WMtWindow			*window;

				// get the listbox
				listbox = WMrDialog_GetItemByID(inDialog, PTUcLB_TurretTypes);
				if (listbox == NULL) { return; }

				// get the selected item text from the listbox
				result = WMrMessage_Send( listbox, LBcMessage_GetText, (UUtUns32)type_name, (UUtUns32)(-1));
				if (result == LBcError) { break; }

				// set the OSD to reflect the change
				osd_all.osd.turret_osd = osd->osd.turret_osd;
				UUrString_Copy( osd_all.osd.turret_osd.turret_class_name, type_name, OBJcMaxNameLength);
				// set the object specific data
				OWrObjectProperties_SetOSD(inDialog, object, &osd_all);

				OBJrObject_GetObjectSpecificData(object, &osd_all);
				// max horiz speed
				window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxHorizSpeed);
				sprintf(string, "%4.2f", (float)osd_all.osd.turret_osd.turret_class->max_horiz_speed * UUcRadPerFrameToDegPerSec );
				WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

				// max horiz speed
				window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxVertSpeed);
				sprintf(string, "%4.2f", (float)osd_all.osd.turret_osd.turret_class->max_vert_speed * UUcRadPerFrameToDegPerSec );
				WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

				// min horiz angle
				window = WMrDialog_GetItemByID(inDialog, PTUcEF_MinHorizAngle);
				sprintf(string, "%.0f", ((float)osd_all.osd.turret_osd.turret_class->min_horiz_angle * M3cRadToDeg ));
				WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

				// max horiz angle
				window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxHorizAngle);
				sprintf(string, "%.0f",  ((float)osd_all.osd.turret_osd.turret_class->max_horiz_angle * M3cRadToDeg ));
				WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

				// min vert angle
				window = WMrDialog_GetItemByID(inDialog, PTUcEF_MinVertAngle);
				sprintf(string, "%.0f", ((float)osd_all.osd.turret_osd.turret_class->min_vert_angle * M3cRadToDeg ));
				WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

				// max vert angle
				window = WMrDialog_GetItemByID(inDialog, PTUcEF_MaxVertAngle);
				sprintf(string, "%.0f", ((float)osd_all.osd.turret_osd.turret_class->max_vert_angle * M3cRadToDeg ));
				WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

			}
			else if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
			{
				WMrDialog_ModalEnd(inDialog, UUcTrue);
			}
		break;

		case PTUcEF_TargetTeamsButton:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				WMtWindow *team_number_window;
				UUtUns32 team_number;

				team_number_window = WMrDialog_GetItemByID(inDialog, PUTcEF_TargetTeamsNumber);
				team_number = (UUtUns32) WMrEditField_GetInt32(team_number_window);

				team_number = OWrGetMask(team_number, AIgTeamList);
				COrConsole_Printf("button = %x", team_number);

				WMrEditField_SetInt32(team_number_window, team_number);

					//target teams
				{
					char mask_for_title[255];
					WMtWindow *team_string_window;

					team_string_window = WMrDialog_GetItemByID(inDialog, PTUcEF_TargetTeams);

					OWrMaskToString(team_number, AIgTeamList, sizeof(mask_for_title), mask_for_title);
					WMrWindow_SetTitle(team_string_window, mask_for_title, 255);
				}

			}
		break;

		case PTcBtn_Revert:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				OWiProp_Turret_SetFields(inDialog);

				// hilite the item currently in use
				OWiProp_Turret_SelectItem(inDialog);
			}
		break;

		case PTcBtn_Cancel:
			if (UUmHighWord(inParam1) == WMcNotify_Click) {
				OBJrSaveObjects(OBJcType_Turret);
				WMrDialog_ModalEnd(inDialog, UUcFalse);
			}
		break;

		case PTcBtn_Save:
			if (UUmHighWord(inParam1) == WMcNotify_Click) {
				OWiProp_Turret_SaveOSD(inDialog);
				OBJrSaveObjects(OBJcType_Turret);
				WMrDialog_ModalEnd(inDialog, UUcTrue);
			}
		break;
	}

	return;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Turret_HandleMenuCommand(
	WMtDialog			*inDialog,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	IMtShade			shade;

	// get the shade
	shade = OWiColorPopup_GetShadeFromID(UUmLowWord(inParam1));

}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool
OWiProp_Turret_Callback(
	WMtDialog			*inDialog,
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	UUtBool				handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiProp_Turret_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			OWgDialog_Prop = NULL;
		return UUcFalse;

		case WMcMessage_Command:
			OWiProp_Turret_HandleCommand(inDialog, inParam1, inParam2);
		break;

		case WMcMessage_MenuCommand:
			OWiProp_Turret_HandleMenuCommand(inDialog, inParam1, inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Flag_SetFields(
	WMtDialog			*inDialog)
{
	OBJtObject			*object;
	OBJtOSD_All			osd_all;
	WMtWindow			*window;
	char				string[255];
	char				char_1;
	char				char_2;

	// get the flag properties
	object = (OBJtObject*)WMrDialog_GetUserData(inDialog);
	UUmAssert(object);

	// get the object specific data
	OBJrObject_GetObjectSpecificData(object, &osd_all);

	// update the controls
	window = WMrDialog_GetItemByID(inDialog, PFcEF_ID_Prefix);
	char_1 = UUmHighByte(osd_all.osd.flag_osd.id_prefix);
	char_2 = UUmLowByte(osd_all.osd.flag_osd.id_prefix);
	sprintf(string, "%c%c", char_1, char_2);
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	window = WMrDialog_GetItemByID(inDialog, PFcEF_ID_Number);
	sprintf(string, "%d", osd_all.osd.flag_osd.id_number);
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	window = WMrDialog_GetItemByID(inDialog, PFcPM_Color);
	OWiColorPopup_SelectItemFromShade(window, osd_all.osd.flag_osd.shade);

	window = WMrDialog_GetItemByID(inDialog, PFcEF_Note);
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)osd_all.osd.flag_osd.note, 0);

	window = WMrDialog_GetItemByID(inDialog, WMcDialogItem_OK);
	WMrWindow_SetEnabled(window, !OBJrObjectType_IsLocked(OBJcType_Flag));

	return;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Flag_InitDialog(
	WMtDialog			*inDialog)
{
	OBJtObject			*object;
	WMtWindow			*popup;
	WMtWindow			*editfield;

	// get a pointer to the object
	object = (OBJtObject *) WMrDialog_GetUserData(inDialog);
	if (object == NULL)
	{
		WMrDialog_ModalEnd(inDialog, UUcTrue);
		return;
	}

	// set the maximum number of characters in the editfields
	editfield = WMrDialog_GetItemByID(inDialog, PFcEF_ID_Prefix);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, 3, 0);

	editfield = WMrDialog_GetItemByID(inDialog, PFcEF_ID_Number);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, 6, 0);

	editfield = WMrDialog_GetItemByID(inDialog, PFcEF_Note);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, OBJcMaxNoteChars, 0);

	// initialize the popup menu
	popup = WMrDialog_GetItemByID(inDialog, PFcPM_Color);
	OWiColorPopup_Init(popup);

	// set the fields
	OWiProp_Flag_SetFields(inDialog);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtError
OWiProp_Flag_SaveOSD(
	WMtDialog			*inDialog)
{
	OBJtObject			*object;
	WMtWindow			*window;
	OBJtOSD_All			osd_all;
	char				string[OBJcMaxNoteChars + 1];
	char				char_1;
	char				char_2;
	UUtUns32			id_number;
	UUtUns16			color_id;

	if (OBJrObjectType_IsLocked(OBJcType_Flag) == UUcTrue)
	{
		WMrDialog_MessageBox(inDialog, "Error", "The flag file is locked.", WMcMessageBoxStyle_OK);
		return UUcError_None;
	}

	// get a pointer to the object
	object = (OBJtObject*)WMrDialog_GetUserData(inDialog);
	UUmAssert(object);

	// get the id prefix
	window = WMrDialog_GetItemByID(inDialog, PFcEF_ID_Prefix);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, OBJcMaxNoteChars);
	sscanf(string, "%c%c", &char_1, &char_2);
	osd_all.osd.flag_osd.id_prefix = UUmMakeShort(char_1, char_2);

	// get the id number
	window = WMrDialog_GetItemByID(inDialog, PFcEF_ID_Number);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, OBJcMaxNoteChars);
	sscanf(string, "%d", &id_number);
	osd_all.osd.flag_osd.id_number = (UUtInt16)id_number;

	// get the color
	window = WMrDialog_GetItemByID(inDialog, PFcPM_Color);
	WMrPopupMenu_GetItemID(window, -1, &color_id);
	osd_all.osd.flag_osd.shade = OWiColorPopup_GetShadeFromID(color_id);

	// get the note
	window = WMrDialog_GetItemByID(inDialog, PFcEF_Note);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, OBJcMaxNoteChars);
	UUrString_Copy(osd_all.osd.flag_osd.note, string, OBJcMaxNoteChars);

	OWrObjectProperties_SetOSD(window, object, &osd_all);

	return UUcError_None;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Flag_HandleCommand(
	WMtDialog			*inDialog,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	OBJtObject			*object;
	UUtError			error;

	// get a pointer to the object
	object = (OBJtObject*)WMrDialog_GetUserData(inDialog);
	UUmAssert(object);

	switch (UUmLowWord(inParam1))
	{
		case PFcEF_ID_Prefix:
		break;

		case PFcEF_ID_Number:
		{
			WMtWindow				*editfield;
			char					string[OBJcMaxNoteChars + 1];
			OBJtOSD_All				osd_all;
			UUtUns32				id_number;
			OBJtObject				*found_object;
			WMtWindow				*save_button;

			// get the id number from the id number editfield and make sure it is in range
			editfield = WMrDialog_GetItemByID(inDialog, PFcEF_ID_Number);
			WMrMessage_Send(editfield, EFcMessage_GetText, (UUtUns32)string, OBJcMaxNoteChars);
			if (string[0] != '\0')
			{
				sscanf(string, "%d", &id_number);
				if (id_number > UUcMaxInt16)
				{
					WMrMessage_Send(editfield, WMcMessage_KeyDown, (UUtUns32)LIcKeyCode_BackSpace, 0);
				}
			}
			else
			{
				id_number = 0;
			}

			// set the save button to enabled or disabled depending on if the user has typed in
			// a duplicate id number or no id number at all
			osd_all.osd.flag_osd.id_number = (UUtUns16)id_number;
			found_object = OBJrObjectType_Search(OBJcType_Flag, OBJcSearch_FlagID, &osd_all);
			save_button = WMrDialog_GetItemByID(inDialog, WMcDialogItem_OK);
			if ((found_object != NULL) && (found_object != object))
			{
				WMrWindow_SetEnabled(save_button, UUcFalse);
			}
			else
			{
				WMrWindow_SetEnabled(save_button, UUcTrue);
			}
		}
		break;

		case PFcEF_Note:
		break;

		case WMcDialogItem_Cancel:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				OBJrSaveObjects(OBJcType_Flag);
				WMrDialog_ModalEnd(inDialog, UUcFalse);
			}
		break;

		case WMcDialogItem_OK:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				// save the object specific data
				error = OWiProp_Flag_SaveOSD(inDialog);
				if (error == UUcError_None)
				{
					OBJrSaveObjects(OBJcType_Flag);
					WMrDialog_ModalEnd(inDialog, UUcTrue);
				}
			}
		break;
	}
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Flag_HandleMenuCommand(
	WMtDialog			*inDialog,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	IMtShade			shade;

	// get the shade
	shade = OWiColorPopup_GetShadeFromID(UUmLowWord(inParam1));
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool
OWiProp_Flag_Callback(
	WMtDialog			*inDialog,
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	UUtBool				handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiProp_Flag_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			OWgDialog_Prop = NULL;
		return UUcFalse;

		case WMcMessage_Command:
			OWiProp_Flag_HandleCommand(inDialog, inParam1, inParam2);
		break;

		case WMcMessage_MenuCommand:
			OWiProp_Flag_HandleMenuCommand(inDialog, inParam1, inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Sound_EnableButtons(
	WMtDialog					*inDialog)
{
	OWtSoundProp				*sound_prop;
	OBJtOSD_Sound				*sound_osd;
	WMtWindow					*button;
	WMtWindow					*editfield;
	UUtUns16					i;

	// get the sound_prop
	sound_prop = (OWtSoundProp*)WMrDialog_GetUserData(inDialog);
	sound_osd = &sound_prop->osd.osd.sound_osd;

	// set the edit button
	button = WMrDialog_GetItemByID(inDialog, OWcSP_Btn_Edit);
	WMrWindow_SetEnabled(button, (sound_osd->ambient != NULL));

	// set the edit fields
	switch (sound_osd->type)
	{
		case OBJcSoundType_Spheres:
			// disable bounding volume editifields and buttons
			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Length);
			WMrWindow_SetEnabled(editfield, UUcFalse);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Width);
			WMrWindow_SetEnabled(editfield, UUcFalse);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Height);
			WMrWindow_SetEnabled(editfield, UUcFalse);

			for (i = OWcSP_Btn_LenInc1; i <= OWcSP_Btn_HgtDec10; i++)
			{
				editfield = WMrDialog_GetItemByID(inDialog, i);
				WMrWindow_SetEnabled(editfield, UUcFalse);
			}

			// enable sphere editfields
			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_MaxVolDist);
			WMrWindow_SetEnabled(editfield, UUcTrue);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_MinVolDist);
			WMrWindow_SetEnabled(editfield, UUcTrue);
		break;

		case OBJcSoundType_BVolume:
			// disable sphere editfields
			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_MaxVolDist);
			WMrWindow_SetEnabled(editfield, UUcFalse);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_MinVolDist);
			WMrWindow_SetEnabled(editfield, UUcFalse);

			// enable bounding volume editifields and buttons
			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Length);
			WMrWindow_SetEnabled(editfield, UUcTrue);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Width);
			WMrWindow_SetEnabled(editfield, UUcTrue);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Height);
			WMrWindow_SetEnabled(editfield, UUcTrue);

			for (i = OWcSP_Btn_LenInc1; i <= OWcSP_Btn_HgtDec10; i++)
			{
				editfield = WMrDialog_GetItemByID(inDialog, i);
				WMrWindow_SetEnabled(editfield, UUcTrue);
			}
		break;
	}

	editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Pitch);
	WMrWindow_SetEnabled(editfield, UUcFalse);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Sound_SetFields(
	WMtDialog					*inDialog)
{
	OWtSoundProp				*sound_prop;
	OBJtOSD_Sound				*sound_osd;
	WMtWindow					*text;
	WMtWindow					*editfield;

	// get the sound_prop
	sound_prop = (OWtSoundProp*)WMrDialog_GetUserData(inDialog);
	sound_osd = &sound_prop->osd.osd.sound_osd;

	// get the ambient sound
	// fill in the text fields
	text = WMrDialog_GetItemByID(inDialog, OWcSP_Txt_Name);
	WMrWindow_SetTitle(text, sound_osd->ambient_name, SScMaxNameLength);
	if (sound_osd->ambient == NULL) { WMrText_SetShade(text, IMcShade_Red); }
	else { WMrText_SetShade(text, IMcShade_Black); }

	switch (sound_osd->type)
	{
		case OBJcSoundType_Spheres:
			WMrDialog_RadioButtonCheck(inDialog, OWcSP_RB_Spheres, OWcSP_RB_Volume, OWcSP_RB_Spheres);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_MinVolDist);
			WMrEditField_SetFloat(editfield, sound_osd->u.spheres.min_volume_distance);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_MaxVolDist);
			WMrEditField_SetFloat(editfield, sound_osd->u.spheres.max_volume_distance);
		break;

		case OBJcSoundType_BVolume:
		{
			float					length;
			float					width;
			float					height;

			length =
				sound_osd->u.bvolume.bbox.maxPoint.x +
				-sound_osd->u.bvolume.bbox.minPoint.x;

			width =
				sound_osd->u.bvolume.bbox.maxPoint.z +
				-sound_osd->u.bvolume.bbox.minPoint.z;

			height =
				sound_osd->u.bvolume.bbox.maxPoint.y +
				-sound_osd->u.bvolume.bbox.minPoint.y;

			WMrDialog_RadioButtonCheck(inDialog, OWcSP_RB_Spheres, OWcSP_RB_Volume, OWcSP_RB_Volume);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Length);
			WMrEditField_SetFloat(editfield, length);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Width);
			WMrEditField_SetFloat(editfield, width);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Height);
			WMrEditField_SetFloat(editfield, height);
		}
		break;

		default:
			UUmAssert(!"something is VERY wrong");
		break;
	}

	editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Pitch);
	WMrEditField_SetFloat(editfield, sound_osd->pitch);

	editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Volume);
	WMrEditField_SetFloat(editfield, sound_osd->volume);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Sound_SetAmbient(
	WMtDialog					*inDialog)
{
	OWtSoundProp				*sound_prop;
	OBJtOSD_Sound				*sound_osd;
	SStAmbient					*ambient;
	OWtSelectResult				result;

	// get the sound_prop
	sound_prop = (OWtSoundProp*)WMrDialog_GetUserData(inDialog);
	sound_osd = &sound_prop->osd.osd.sound_osd;

	// select the ambient sound
	result = OWrSelect_AmbientSound(&ambient);
	if (result == OWcSelectResult_Cancel) { return; }

	// set the sound osd data
	if (ambient == NULL)
	{
		UUrMemory_Clear(sound_osd->ambient_name, SScMaxNameLength);
	}
	else
	{
		UUrString_Copy(sound_osd->ambient_name, ambient->ambient_name, SScMaxNameLength);
	}
	sound_osd->ambient = ambient;

	// fill in the fields
	OWiProp_Sound_SetFields(inDialog);

	// enable the buttons fields
	OWiProp_Sound_EnableButtons(inDialog);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtError
OWiProp_Sound_EditAmbient(
	WMtDialog					*inDialog)
{
	UUtError					error;
	OWtSoundProp				*sound_prop;
	OBJtOSD_Sound				*sound_osd;

	// get the sound_prop
	sound_prop = (OWtSoundProp*)WMrDialog_GetUserData(inDialog);
	sound_osd = &sound_prop->osd.osd.sound_osd;

	// edit the properties of the ambient sound
	error = OWrAmbientProperties_Display(inDialog, sound_osd->ambient);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Sound_SaveData(
	WMtDialog					*inDialog)
{
	OWtSoundProp				*sound_prop;
	OBJtOSD_Sound				*sound_osd;
	WMtWindow					*editfield;

	// get the sound_prop
	sound_prop = (OWtSoundProp*)WMrDialog_GetUserData(inDialog);
	sound_osd = &sound_prop->osd.osd.sound_osd;

	// get the data from the fields
	switch (sound_osd->type)
	{
		case OBJcSoundType_Spheres:
			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_MaxVolDist);
			sound_osd->u.spheres.max_volume_distance = WMrEditField_GetFloat(editfield);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_MinVolDist);
			sound_osd->u.spheres.min_volume_distance = WMrEditField_GetFloat(editfield);
		break;

		case OBJcSoundType_BVolume:
		{
			float						length;
			float						width;
			float						height;

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Length);
			length = WMrEditField_GetFloat(editfield);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Width);
			width = WMrEditField_GetFloat(editfield);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Height);
			height = WMrEditField_GetFloat(editfield);

			// set the min max data based on the position of the object
			sound_osd->u.bvolume.bbox.minPoint.x = -(length * 0.5f);
			sound_osd->u.bvolume.bbox.minPoint.y = -(height * 0.5f);
			sound_osd->u.bvolume.bbox.minPoint.z = -(width * 0.5f);

			sound_osd->u.bvolume.bbox.maxPoint.x = (length * 0.5f);
			sound_osd->u.bvolume.bbox.maxPoint.y = (height * 0.5f);
			sound_osd->u.bvolume.bbox.maxPoint.z = (width * 0.5f);
		}
		break;
	}

	editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Pitch);
	sound_osd->pitch = WMrEditField_GetFloat(editfield);

	editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Volume);
	sound_osd->volume = WMrEditField_GetFloat(editfield);

	// set the object specific data
	OBJrObject_SetObjectSpecificData(sound_prop->object, &sound_prop->osd);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Sound_InitDialog(
	WMtDialog					*inDialog)
{
	OBJtObject					*object;
	OWtSoundProp				*sound_prop;

	// get a pointer to the selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL)
	{
		WMrDialog_ModalEnd(inDialog, UUcTrue);
		return;
	}

	sound_prop = (OWtSoundProp*)UUrMemory_Block_NewClear(sizeof(OWtSoundProp));
	if (sound_prop == NULL)
	{
		WMrDialog_ModalEnd(inDialog, UUcTrue);
		return;
	}

	sound_prop->object = object;

	// get the object specific data
	OBJrObject_GetObjectSpecificData(object, &sound_prop->osd);
	OBJrObject_GetObjectSpecificData(object, &sound_prop->backup_osd);

	// set the user data
	WMrDialog_SetUserData(inDialog, (UUtUns32)sound_prop);

	// set the fields
	OWiProp_Sound_SetFields(inDialog);

	// enable buttons
	OWiProp_Sound_EnableButtons(inDialog);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Sound_Destroy(
	WMtDialog					*inDialog)
{
	OWtSoundProp				*sound_prop;

	// get the sound_prop
	sound_prop = (OWtSoundProp*)WMrDialog_GetUserData(inDialog);
	if (sound_prop != NULL)
	{
		UUrMemory_Block_Delete(sound_prop);
		sound_prop = NULL;

		WMrDialog_SetUserData(inDialog, 0);
	}
}
#endif

// ----------------------------------------------------------------------
#define OWmInc(id, x)	\
	do {	editfield = WMrDialog_GetItemByID(inDialog, id); \
			value = WMrEditField_GetFloat(editfield) + x; \
			WMrEditField_SetFloat(editfield, value); } while (0);

#if TOOL_VERSION
static void
OWiProp_Sound_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	WMtWindow					*inControl)
{
	UUtUns16					command_type;
	UUtUns16					control_id;
	UUtBool						update_fields;
	OWtSoundProp				*sound_prop;
	OBJtOSD_Sound				*sound_osd;
	float						value;
	WMtWindow					*editfield;

	// get the sound_prop
	sound_prop = (OWtSoundProp*)WMrDialog_GetUserData(inDialog);
	sound_osd = &sound_prop->osd.osd.sound_osd;

	// interpret inParam1
	command_type = UUmHighWord(inParam1);
	control_id = UUmLowWord(inParam1);

	// handle the command
	update_fields = UUcTrue;
	switch (control_id)
	{
		case OWcSP_Btn_Set:
			OWiProp_Sound_SetAmbient(inDialog);
		break;

		case OWcSP_Btn_Edit:
			OWiProp_Sound_EditAmbient(inDialog);
		break;

		case OWcSP_RB_Spheres:
			if (sound_osd->type == OBJcSoundType_Spheres) { break; }

			sound_osd->type = OBJcSoundType_Spheres;
			WMrDialog_RadioButtonCheck(inDialog, OWcSP_RB_Spheres, OWcSP_RB_Volume, control_id);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_MaxVolDist);
			WMrEditField_SetFloat(editfield, 5.0f);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_MinVolDist);
			WMrEditField_SetFloat(editfield, 10.0f);
		break;

		case OWcSP_RB_Volume:
			if (sound_osd->type == OBJcSoundType_BVolume) { break; }

			sound_osd->type = OBJcSoundType_BVolume;
			WMrDialog_RadioButtonCheck(inDialog, OWcSP_RB_Spheres, OWcSP_RB_Volume, control_id);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Length);
			WMrEditField_SetFloat(editfield, 100.0f);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Width);
			WMrEditField_SetFloat(editfield, 100.0f);

			editfield = WMrDialog_GetItemByID(inDialog, OWcSP_EF_Height);
			WMrEditField_SetFloat(editfield, 100.0f);
		break;

		case OWcSP_Btn_LenInc1:		OWmInc(OWcSP_EF_Length, 1.0f);		break;
		case OWcSP_Btn_WidInc1:		OWmInc(OWcSP_EF_Width, 1.0f);		break;
		case OWcSP_Btn_HgtInc1:		OWmInc(OWcSP_EF_Height, 1.0f);		break;

		case OWcSP_Btn_LenDec1:		OWmInc(OWcSP_EF_Length, -1.0f);		break;
		case OWcSP_Btn_WidDec1:		OWmInc(OWcSP_EF_Width, -1.0f);		break;
		case OWcSP_Btn_HgtDec1:		OWmInc(OWcSP_EF_Height, -1.0f);		break;

		case OWcSP_Btn_LenInc10:	OWmInc(OWcSP_EF_Length, 10.0f);		break;
		case OWcSP_Btn_WidInc10:	OWmInc(OWcSP_EF_Width, 10.0f);		break;
		case OWcSP_Btn_HgtInc10:	OWmInc(OWcSP_EF_Height, 10.0f);		break;

		case OWcSP_Btn_LenDec10:	OWmInc(OWcSP_EF_Length, -10.0f);	break;
		case OWcSP_Btn_WidDec10:	OWmInc(OWcSP_EF_Width, -10.0f);		break;
		case OWcSP_Btn_HgtDec10:	OWmInc(OWcSP_EF_Height, -10.0f);	break;

		case OWcSP_Btn_Cancel:
			OBJrObject_SetObjectSpecificData(sound_prop->object, &sound_prop->backup_osd);
			WMrDialog_ModalEnd(inDialog, UUcFalse);
		return;

		case OWcSP_Btn_Save:
			OWiProp_Sound_SaveData(inDialog);
			WMrDialog_ModalEnd(inDialog, UUcTrue);
		return;

		default:
			update_fields = UUcFalse;
		break;
	}

	OWiProp_Sound_SaveData(inDialog);

	// update the fields
	if (update_fields == UUcTrue)
	{
		OWiProp_Sound_SetFields(inDialog);
	}

	// enable buttons
	OWiProp_Sound_EnableButtons(inDialog);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool
OWiProp_Sound_Callback(
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
			OWiProp_Sound_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			OWiProp_Sound_Destroy(inDialog);
			OWgDialog_Prop = NULL;
		break;

		case WMcMessage_Command:
			OWiProp_Sound_HandleCommand(
				inDialog,
				inParam1,
				(WMtWindow*)inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Generic_SelectItem(
	WMtDialog			*inDialog)
{
	WMtWindow			*listbox;
	OBJtObject			*object;
	char				osd_name[256];
	OBJtOSD_All			*osd;

	// get a pointer to the osd
	osd = (OBJtOSD_All*)WMrDialog_GetUserData(inDialog);
	UUmAssert(osd);

	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, PGcLB_ObjectTypes);
	if (listbox == NULL) { return; }

	// get the currently selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }

	// get the name of the selected object specific type
	OBJrObject_GetName(object, osd_name, 256);

	// select the appropriate item
	WMrMessage_Send(
		listbox,
		LBcMessage_SelectString,
		(UUtUns32)(-1),
		(UUtUns32)osd_name);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool
OWiListBox_ObjectType_EnumCallback(
	const char				*inName,
	UUtUns32				inUserData)
{
	WMtWindow				*listbox;

	// get the list box
	listbox = (WMtWindow*)inUserData;
	if (listbox == NULL) { return UUcFalse; }

	// add the character to the list
	WMrMessage_Send(
		listbox,
		LBcMessage_AddString,
		(UUtUns32) inName,
		0);

	return UUcTrue;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Generic_InitDialog(
	WMtDialog			*inDialog)
{
	WMtWindow			*listbox;
	OBJtObject			*object;
	OBJtObjectType		object_type;
	OBJtOSD_All			*osd;

	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, PGcLB_ObjectTypes);
	if (listbox == NULL) { return; }

	// get the currently selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }

	// get the object type
	object_type = OBJrObject_GetType(object);

	// enumerate the object specific types
	OBJrObject_Enumerate(
		object,
		OWiListBox_ObjectType_EnumCallback,
		(UUtUns32)listbox);

	// allocate memory for the osd
	osd = (OBJtOSD_All*)UUrMemory_Block_NewClear(sizeof(OBJtOSD_All));
	UUmAssert(osd);
	WMrDialog_SetUserData(inDialog, (UUtUns32)osd);

	// get the object specific data
	OBJrObject_GetObjectSpecificData(object, osd);

	// set the keyboard focus to the listbox
	WMrWindow_SetFocus(listbox);

	// hilite the item currently in use
	OWiProp_Generic_SelectItem(inDialog);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Generic_Destroy(
	WMtDialog			*inDialog)
{
	void				*data;

	data = (void*)WMrDialog_GetUserData(inDialog);
	UUmAssert(data);

	UUrMemory_Block_Delete(data);
	WMrDialog_SetUserData(inDialog, 0);

	OWgDialog_Prop = NULL;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool
OWiProp_Generic_HandleCommand(
	WMtDialog			*inDialog,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	OBJtObject			*object;
	WMtWindow			*listbox;
	OBJtObjectType		object_type;
	OBJtOSD_All			*osd;

	// get a pointer to the osd
	osd = (OBJtOSD_All*)WMrDialog_GetUserData(inDialog);
	UUmAssert(osd);

	// get the object and object type
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return UUcTrue; }
	object_type = OBJrObject_GetType(object);

	switch (UUmLowWord(inParam1))
	{
		case PGcLB_ObjectTypes:
			if (UUmHighWord(inParam1) == LBcNotify_SelectionChanged)
			{
				char				type_name[256];
				OBJtOSD_All			osd_all;
				UUtUns32			result;

				// get the listbox
				listbox = WMrDialog_GetItemByID(inDialog, PGcLB_ObjectTypes);
				if (listbox == NULL) { return UUcTrue; }

				// clear the OSD so we don't have garbage in it
				UUrMemory_Clear(&osd_all, sizeof(OBJtOSD_All));

				// get the selected item text from the listbox
				result =
					WMrMessage_Send(
						listbox,
						LBcMessage_GetText,
						(UUtUns32)type_name,
						(UUtUns32)(-1));
				if (result == LBcError) { break; }

				// set the OSD to reflect the change
				switch (object_type)
				{
					case OBJcType_Furniture:
						// copy the existing OSD data
						osd_all.osd.furniture_osd = *((OBJtOSD_Furniture *) object->object_data);

						UUrString_Copy(
							osd_all.osd.furniture_osd.furn_geom_name,
							type_name,
							BFcMaxFileNameLength);
					break;

					case OBJcType_Particle:
						// copy the existing OSD data
						osd_all.osd.particle_osd = *((OBJtOSD_Particle *) object->object_data);

						UUrString_Copy(
							osd_all.osd.particle_osd.particle.classname,
							type_name,
							P3cParticleClassNameLength);
					break;

					case OBJcType_PowerUp:
						// copy the existing OSD data
						osd_all.osd.powerup_osd = *((OBJtOSD_PowerUp *) object->object_data);

						osd_all.osd.powerup_osd.powerup_type = OBJrPowerUp_NameToType(type_name);
						osd_all.osd.powerup_osd.powerup = NULL;
					break;

					case OBJcType_Weapon:
						// copy the existing OSD data
						osd_all.osd.weapon_osd = *((OBJtOSD_Weapon *) object->object_data);

						UUrString_Copy(
							osd_all.osd.weapon_osd.weapon_class_name,
							type_name,
							BFcMaxFileNameLength);
					break;

					case OBJcType_PatrolPath:
						// copy the existing OSD data
						osd_all.osd.patrolpath_osd = *((OBJtOSD_PatrolPath *) object->object_data);

						UUrString_Copy(osd_all.osd.patrolpath_osd.name, type_name, SLcScript_MaxNameLength);
					break;

					case OBJcType_Combat:
						// copy the existing OSD data
						osd_all.osd.combat_osd = *((OBJtOSD_Combat *) object->object_data);

						UUrString_Copy(osd_all.osd.combat_osd.name, type_name, SLcScript_MaxNameLength);
					break;

					case OBJcType_Melee:
						// copy the existing OSD data
						osd_all.osd.melee_osd = *((OBJtOSD_Melee *) object->object_data);

						UUrString_Copy(osd_all.osd.melee_osd.name, type_name, SLcScript_MaxNameLength);
					break;
				}

				// set the object specific data
				OWrObjectProperties_SetOSD(inDialog, object, &osd_all);
			}
			else if (UUmHighWord(inParam1) == WMcNotify_DoubleClick)
			{
				WMrDialog_ModalEnd(inDialog, UUcTrue);
			}
		break;

		case PGcBtn_Revert:
			if (UUmHighWord(inParam1) == WMcNotify_Click)
			{
				// set the object specific data
				OWrObjectProperties_SetOSD(inDialog, object, osd);

				// hilite the item currently in use
				OWiProp_Generic_SelectItem(inDialog);
			}
		break;

		case WMcDialogItem_Cancel:
			WMrDialog_ModalEnd(inDialog, UUcFalse);
		break;

		case PGcBtn_Save:
			WMrDialog_ModalEnd(inDialog, UUcTrue);
		break;

		default:
			// not handled
			return UUcFalse;
	}

	// handled
	return UUcTrue;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool
OWiProp_Generic_Callback(
	WMtDialog			*inDialog,
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	UUtBool				handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiProp_Generic_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			OWiProp_Generic_Destroy(inDialog);
		return UUcFalse;

		case WMcMessage_Command:
			handled = OWiProp_Generic_HandleCommand(inDialog, inParam1, inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool
OWiProp_Furniture_Callback(
	WMtDialog			*inDialog,
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	UUtBool				handled;
	WMtEditField *		editfield = WMrDialog_GetItemByID(inDialog, PFUcEF_Tag);
	OBJtOSD_Furniture *	furniture_osd;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiProp_Generic_InitDialog(inDialog);

			if (editfield) {
				furniture_osd = (OBJtOSD_Furniture *) WMrDialog_GetUserData(inDialog);

				WMrEditField_SetText(editfield, furniture_osd->furn_tag);
			}
		break;

		case WMcMessage_Destroy:
			OWiProp_Generic_Destroy(inDialog);
		return UUcFalse;

		case WMcMessage_Command:
			handled = OWiProp_Generic_HandleCommand(inDialog, inParam1, inParam2);

			if (!handled) {
				switch(UUmLowWord(inParam1)) {
					case PFUcEF_Tag:
					{
						OBJtOSD_All	new_osd;
						OBJtObject *		object;

						if (!editfield)
							break;

						// get the selected object
						object = OBJrSelectedObjects_GetSelectedObject(0);
						if (object == NULL)
							break;

						// clear the OSD so we don't have garbage in it
						UUrMemory_Clear(&new_osd, sizeof(OBJtOSD_All));

						// get the furniture tag name from the editfield
						WMrEditField_GetText(editfield, new_osd.osd.furniture_osd.furn_tag, EPcParticleTagNameLength + 1);

						// get the current (unchanged) furn geom name
						furniture_osd = (OBJtOSD_Furniture *) WMrDialog_GetUserData(inDialog);
						UUrString_Copy(new_osd.osd.furniture_osd.furn_geom_name, furniture_osd->furn_geom_name,
										BFcMaxFileNameLength);

						// set the object specific data
						OWrObjectProperties_SetOSD(inDialog, object, &new_osd);

						handled = UUcTrue;
						break;
					}
				}
			}
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}
#endif

typedef struct OWtFloatIncrement
{
	UUtUns16 float_item_id;
	float *float_pointer;
	UUtUns16 button_item_id;
	float increment_amount;
} OWtFloatIncrement;

#if TOOL_VERSION
static UUtBool OWrHandleFloatIncrement(OWtFloatIncrement *inTable, void *inPtr, WMtDialog *inDialog, WMtMessage inMessage, UUtUns32 inParam1, UUtUns32 inParam2)
{
	OWtFloatIncrement *current_entry;
	UUtBool handled = UUcFalse;

	if (WMcMessage_Command != inMessage) {
		goto exit;
	}

	for(current_entry = inTable; current_entry->button_item_id != 0; current_entry++)
	{
		if (current_entry->button_item_id == UUmLowWord(inParam1))
		{
			WMtEditField *edit_field = WMrDialog_GetItemByID(inDialog, current_entry->float_item_id);
			float *float_ptr = (float *)  ((UUtUns32) current_entry->float_pointer + (UUtUns32) (inPtr));

			*float_ptr += current_entry->increment_amount;
			WMrEditField_SetFloat(edit_field, *float_ptr);

			handled = UUcTrue;

			break;
		}
	}

exit:
	return handled;
}
#endif

#if TOOL_VERSION
	static UUtBool
OWiProp_TriggerVolume_Callback(
	WMtDialog			*inDialog,
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	enum
	{
		cEditTriggerVolume_Name				= 100,

		cEditTriggerVolume_Entry_Script		= 200,
		cEditTriggerVolume_Inside_Script	= 201,
		cEditTriggerVolume_Exit_Script		= 202,

		cEditTriggerVolume_Id				= 300,
		cEditTriggerVolume_ParentId			= 301,

		cEditTriggerVolume_ScaleX			= 400,
		cEditTriggerVolume_ScaleY			= 401,
		cEditTriggerVolume_ScaleZ			= 402,

		cEditTriggerVolume_ScaleX_Plus_One	= 500,
		cEditTriggerVolume_ScaleX_Minus_One	= 501,
		cEditTriggerVolume_ScaleY_Plus_One	= 502,
		cEditTriggerVolume_ScaleY_Minus_One	= 503,
		cEditTriggerVolume_ScaleZ_Plus_One	= 504,
		cEditTriggerVolume_ScaleZ_Minus_One	= 505,

		cEditTriggerVolume_ScaleX_Plus_Five	= 600,
		cEditTriggerVolume_ScaleX_Minus_Five= 601,
		cEditTriggerVolume_ScaleY_Plus_Five	= 602,
		cEditTriggerVolume_ScaleY_Minus_Five= 603,
		cEditTriggerVolume_ScaleZ_Plus_Five	= 604,
		cEditTriggerVolume_ScaleZ_Minus_Five= 605,

		cEditTriggerVolume_ScaleX_Plus_Ten	= 700,
		cEditTriggerVolume_ScaleX_Minus_Ten	= 701,
		cEditTriggerVolume_ScaleY_Plus_Ten	= 702,
		cEditTriggerVolume_ScaleY_Minus_Ten	= 703,
		cEditTriggerVolume_ScaleZ_Plus_Ten	= 704,
		cEditTriggerVolume_ScaleZ_Minus_Ten	= 705,

		cEditTriggerVolume_Note				= 800,

		cEditTriggerVolume_TeamText			= 900,
		cEditTriggerVolume_TeamButton		= 901,

		cEditTriggerVolume_FlagsText			= 1000,
		cEditTriggerVolume_FlagsButton			= 1001

	};



#define base_ptr ((OBJtOSD_TriggerVolume *) NULL)
	OWtFloatIncrement float_increment_table[] =
	{
		{ cEditTriggerVolume_ScaleX, &base_ptr->scale.x, cEditTriggerVolume_ScaleX_Plus_One, +1.f },
		{ cEditTriggerVolume_ScaleX, &base_ptr->scale.x, cEditTriggerVolume_ScaleX_Minus_One, -1.f },
		{ cEditTriggerVolume_ScaleY, &base_ptr->scale.y, cEditTriggerVolume_ScaleY_Plus_One, +1.f },
		{ cEditTriggerVolume_ScaleY, &base_ptr->scale.y, cEditTriggerVolume_ScaleY_Minus_One, -1.f },
		{ cEditTriggerVolume_ScaleZ, &base_ptr->scale.z, cEditTriggerVolume_ScaleZ_Plus_One, +1.f },
		{ cEditTriggerVolume_ScaleZ, &base_ptr->scale.z, cEditTriggerVolume_ScaleZ_Minus_One, -1.f },

		{ cEditTriggerVolume_ScaleX, &base_ptr->scale.x, cEditTriggerVolume_ScaleX_Plus_Five, +5.f },
		{ cEditTriggerVolume_ScaleX, &base_ptr->scale.x, cEditTriggerVolume_ScaleX_Minus_Five, -5.f },
		{ cEditTriggerVolume_ScaleY, &base_ptr->scale.y, cEditTriggerVolume_ScaleY_Plus_Five, +5.f },
		{ cEditTriggerVolume_ScaleY, &base_ptr->scale.y, cEditTriggerVolume_ScaleY_Minus_Five, -5.f },
		{ cEditTriggerVolume_ScaleZ, &base_ptr->scale.z, cEditTriggerVolume_ScaleZ_Plus_Five, +5.f },
		{ cEditTriggerVolume_ScaleZ, &base_ptr->scale.z, cEditTriggerVolume_ScaleZ_Minus_Five, -5.f },

		{ cEditTriggerVolume_ScaleX, &base_ptr->scale.x, cEditTriggerVolume_ScaleX_Plus_Ten, +10.f },
		{ cEditTriggerVolume_ScaleX, &base_ptr->scale.x, cEditTriggerVolume_ScaleX_Minus_Ten, -10.f },
		{ cEditTriggerVolume_ScaleY, &base_ptr->scale.y, cEditTriggerVolume_ScaleY_Plus_Ten, +10.f },
		{ cEditTriggerVolume_ScaleY, &base_ptr->scale.y, cEditTriggerVolume_ScaleY_Minus_Ten, -10.f },
		{ cEditTriggerVolume_ScaleZ, &base_ptr->scale.z, cEditTriggerVolume_ScaleZ_Plus_Ten, +10.f },
		{ cEditTriggerVolume_ScaleZ, &base_ptr->scale.z, cEditTriggerVolume_ScaleZ_Minus_Ten, -10.f },
		{ 0, 0, 0 },
	};


	UUtBool handled;

	OBJtObject			*object = (OBJtObject *) WMrDialog_GetUserData(inDialog);
	OBJtOSD_All			object_specific_data;
	OBJtOSD_TriggerVolume *trigger_volume_osd;

	UUtBool				dirty = UUcFalse;
	UUtBool				rebuild_scale_fields = UUcFalse;
	UUtBool				rebuild_mask = UUcFalse;
	UUtBool				rebuild_flag_mask = UUcFalse;

	WMtEditField *name_edit = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_Name);
	WMtEditField *entry_script_edit = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_Entry_Script);
	WMtEditField *exit_script_edit = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_Exit_Script);
	WMtEditField *inside_script_edit = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_Inside_Script);
	WMtEditField *id_edit = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_Id);
	WMtEditField *parent_id_edit = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_ParentId);
	WMtEditField *note_edit = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_Note);
	WMtEditField *scaleX_edit = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_ScaleX);
	WMtEditField *scaleY_edit = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_ScaleY);
	WMtEditField *scaleZ_edit = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_ScaleZ);

	OBJrObject_GetObjectSpecificData(object, &object_specific_data);
	trigger_volume_osd = &object_specific_data.osd.trigger_volume_osd;


	switch (inMessage)
	{

		case WMcMessage_InitDialog:
			WMrEditField_SetText(name_edit, trigger_volume_osd->name);
			WMrEditField_SetText(entry_script_edit, trigger_volume_osd->entry_script);
			WMrEditField_SetText(exit_script_edit, trigger_volume_osd->exit_script);
			WMrEditField_SetText(inside_script_edit, trigger_volume_osd->inside_script);
			WMrEditField_SetText(note_edit, trigger_volume_osd->note);
			WMrEditField_SetInt32(id_edit, trigger_volume_osd->id);
			WMrEditField_SetInt32(parent_id_edit, trigger_volume_osd->parent_id);
			rebuild_scale_fields = UUcTrue;
			rebuild_mask = UUcTrue;
			rebuild_flag_mask = UUcTrue;
		break;

		case WMcMessage_Command:
			dirty = UUcTrue;
			handled = UUcTrue;

			switch(UUmLowWord(inParam1))
			{
				case cEditTriggerVolume_Name:
					WMrEditField_GetText(name_edit, trigger_volume_osd->name, sizeof(trigger_volume_osd->name));
				break;

				case cEditTriggerVolume_Entry_Script:
					WMrEditField_GetText(entry_script_edit, trigger_volume_osd->entry_script, sizeof(trigger_volume_osd->entry_script));
				break;

				case cEditTriggerVolume_Inside_Script:
					WMrEditField_GetText(inside_script_edit, trigger_volume_osd->inside_script, sizeof(trigger_volume_osd->inside_script));
				break;

				case cEditTriggerVolume_Exit_Script:
					WMrEditField_GetText(exit_script_edit, trigger_volume_osd->exit_script, sizeof(trigger_volume_osd->exit_script));
				break;

				case cEditTriggerVolume_Id:
					trigger_volume_osd->id = WMrEditField_GetInt32(id_edit);
				break;

				case cEditTriggerVolume_ParentId:
					trigger_volume_osd->parent_id = WMrEditField_GetInt32(parent_id_edit);
					rebuild_mask = UUcTrue;
				break;

				case cEditTriggerVolume_ScaleX:
					trigger_volume_osd->scale.x = WMrEditField_GetFloat(scaleX_edit);
					COrConsole_Printf("scale x %f", trigger_volume_osd->scale.x);
					trigger_volume_osd->scale.x = UUmMax(trigger_volume_osd->scale.x, 1.f);
					COrConsole_Printf("scale x %f", trigger_volume_osd->scale.x);
				break;

				case cEditTriggerVolume_ScaleY:
					trigger_volume_osd->scale.y = UUmMax(1.f, WMrEditField_GetFloat(scaleY_edit));
				break;

				case cEditTriggerVolume_ScaleZ:
					trigger_volume_osd->scale.z = UUmMax(1.f, WMrEditField_GetFloat(scaleZ_edit));
				break;

				case WMcDialogItem_OK:
					{
						char *error_string = OBJrTriggerVolume_IsInvalid(object);

						if (NULL != error_string) {
							WMrDialog_MessageBox(NULL, "error", error_string, WMcMessageBoxStyle_OK);
						}
						else {
							WMrDialog_ModalEnd(inDialog, UUcTrue);
						}

						dirty = UUcFalse;
					}
				break;

				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, UUcFalse);
				break;

				case cEditTriggerVolume_Note:
					WMrEditField_GetText(note_edit, trigger_volume_osd->note, sizeof(trigger_volume_osd->note));
				break;


				case cEditTriggerVolume_TeamButton:
					trigger_volume_osd->team_mask = OWrGetMask(trigger_volume_osd->team_mask, AIgTeamList);
					rebuild_mask = UUcTrue;
				break;

				case cEditTriggerVolume_FlagsButton:
					trigger_volume_osd->authored_flags = OWrGetMask(trigger_volume_osd->authored_flags, OWgTriggerVolumeFlagList);
					rebuild_flag_mask = UUcTrue;
				break;

				default:
					dirty = UUcFalse;
					handled = UUcFalse;
				break;
			}
		break;

		default:
			handled = UUcFalse;
	}

	if ((!handled) && (!dirty)) {
		dirty = handled = OWrHandleFloatIncrement(float_increment_table, trigger_volume_osd, inDialog, inMessage, inParam1, inParam2);

		if (handled) {
			rebuild_scale_fields = UUcTrue;
		}
	}

	if (rebuild_scale_fields) {
		trigger_volume_osd->scale.x = UUmMax(trigger_volume_osd->scale.x, 1.f);
		trigger_volume_osd->scale.y = UUmMax(trigger_volume_osd->scale.y, 1.f);
		trigger_volume_osd->scale.z = UUmMax(trigger_volume_osd->scale.z, 1.f);

		WMrEditField_SetFloat(scaleX_edit, trigger_volume_osd->scale.x);
		WMrEditField_SetFloat(scaleY_edit, trigger_volume_osd->scale.y);
		WMrEditField_SetFloat(scaleZ_edit, trigger_volume_osd->scale.z);
	}

	if (rebuild_mask) {
		WMtWindow *team_text = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_TeamText);

		if (0 == trigger_volume_osd->parent_id) {
			char mask_string[128];

			OWrMaskToString(trigger_volume_osd->team_mask, AIgTeamList, sizeof(mask_string), mask_string);
			WMrWindow_SetTitle(team_text, mask_string, WMcMaxTitleLength);
		}
		else {
			WMrWindow_SetTitle(team_text, "same as parent", WMcMaxTitleLength);
		}

		dirty = UUcTrue;
	}

	if (rebuild_flag_mask) {
		WMtWindow *flag_text = WMrDialog_GetItemByID(inDialog, cEditTriggerVolume_FlagsText);
		char flag_mask_string[128];

		OWrMaskToString(trigger_volume_osd->authored_flags, OWgTriggerVolumeFlagList, sizeof(flag_mask_string), flag_mask_string);
		WMrWindow_SetTitle(flag_text, flag_mask_string, WMcMaxTitleLength);

		dirty = UUcTrue;
	}

	if (dirty) {
		UUmAssert(trigger_volume_osd->scale.x >= 1.f);
		UUmAssert(trigger_volume_osd->scale.y >= 1.f);
		UUmAssert(trigger_volume_osd->scale.z >= 1.f);

		OWrObjectProperties_SetOSD(inDialog, object, &object_specific_data);
	}

	return handled;
}
#endif


// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
OWrProp_Display(
	OBJtObject					*inObject)
{
#if TOOL_VERSION
	OBJtOSD_All					object_specific_data;
	UUtUns32					message, num_obj;
	UUtError					error;
	UUtBool						success;

	OBJtObjectType				this_object_type;
	const OWtPropDialogSetup	*this_setup;
	const OWtPropDialogSetup	setup_list[] =
	{
		{ OBJcType_Character,		OWcDialog_Prop_Char2,			OWrProp_Char_Callback		},
		{ OBJcType_Particle,		OWcDialog_Prop_Particle,		OWrProp_Particle_Callback	},
		{ OBJcType_Flag,			OWcDialog_Prop_Flag,			OWiProp_Flag_Callback		},
		{ OBJcType_Door,			OWcDialog_Prop_Door,			OWiProp_Door_Callback		},
		{ OBJcType_Console,			OWcDialog_Prop_Console,			OWiProp_Console_Callback	},
		{ OBJcType_Turret,			OWcDialog_Prop_Turret,			OWiProp_Turret_Callback		},
		{ OBJcType_Trigger,			OWcDialog_Prop_Trigger,			OWiProp_Trigger_Callback	},
		{ OBJcType_TriggerVolume,	OWcDialog_Prop_TriggerVolume,	OWiProp_TriggerVolume_Callback	},
		{ OBJcType_PatrolPath,		OWcDialog_AI_EditPath,			OWrPath_Edit_Callback		},
		{ OBJcType_Sound,			OWcDialog_Prop_Sound,			OWiProp_Sound_Callback		},
		{ OBJcType_Combat,			OWcDialog_AI_EditCombat,		OWrEditCombat_Callback		},
		{ OBJcType_Melee,			OWcDialog_AI_EditMelee,			OWrEditMelee_Callback		},
		{ OBJcType_Furniture,		OWcDialog_Prop_Furniture,		OWiProp_Furniture_Callback	},
		{ OBJcType_Neutral,			OWcDialog_AI_EditNeutral,		OWrEditNeutral_Callback		},
		{ OBJcType_Generic,			OWcDialog_Prop_Generic,			OWiProp_Generic_Callback	},
	};

	if (NULL == inObject)
	{
		num_obj = OBJrSelectedObjects_GetNumSelected();

		if (num_obj == 0) {
			// no objects selected.
			return UUcFalse;

		} else if (OBJrSelectedObjects_GetNumSelected() > 1) {
			// make sure only one object is being edited at a time
			WMrDialog_MessageBox(
				NULL,
				"Sorry...",
				"You can only edit the properties of one object at a time.",
				WMcMessageBoxStyle_OK);
			return UUcFalse;
		}

		// get a pointer to the selected object
		inObject = OBJrSelectedObjects_GetSelectedObject(0);
		if (NULL == inObject) { return UUcFalse; }
	}
	else
	{
		// select the object that was passed in
		OBJrSelectedObjects_Select(inObject, UUcFalse);
	}

	// get the osd
	OBJrObject_GetObjectSpecificData(inObject, &object_specific_data);

	// get the object type
	this_object_type = OBJrObject_GetType(inObject);
	error = UUcError_Generic;

	// display the properties dialog for the selected object
	for(this_setup = setup_list; 1; this_setup++)
	{
		if ((this_setup->object_type == this_object_type) || (OBJcType_Generic == this_setup->object_type))
		{
			error = WMrDialog_ModalBegin(this_setup->dialog_id, NULL, this_setup->callback, (UUtUns32)inObject, &message);
			break;
		}
	}

	// if there was an error or the user decided not to keep their changes then restore the object specific data
	if ((error != UUcError_None) || (!message))
	{
		success = UUcFalse;
		OBJrObject_SetObjectSpecificData(inObject, &object_specific_data);
	}
	else
	{
		success = UUcTrue;
	}

	return success;
#else
	return UUcFalse;
#endif
}

// ----------------------------------------------------------------------
UUtBool
OWrProp_IsVisible(
	void)
{
	if (OWgDialog_Prop == NULL) { return UUcFalse; }

	return WMrWindow_GetVisible(OWgDialog_Prop);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OWiLightProp_SaveFields(
	WMtDialog			*inDialog)
{
	OWtLightProp		*light_prop;
	WMtWindow			*window;
	UUtUns16			index;
	char				string[255];

	// get the light properties
	light_prop = (OWtLightProp*)WMrDialog_GetUserData(inDialog);
	UUmAssert(light_prop);

	// get the index
	index = light_prop->current_index;

	// get the filter color
	window = WMrDialog_GetItemByID(inDialog, PLcEF_R);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 255);
	sscanf(string, "%f", &light_prop->ls_data_array[index].filter_color[0]);

	window = WMrDialog_GetItemByID(inDialog, PLcEF_G);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 255);
	sscanf(string, "%f", &light_prop->ls_data_array[index].filter_color[1]);

	window = WMrDialog_GetItemByID(inDialog, PLcEF_B);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 255);
	sscanf(string, "%f", &light_prop->ls_data_array[index].filter_color[2]);

	// get the light type
	light_prop->ls_data_array[index].light_flags &=
		~(OBJcLightFlag_Type_Area | OBJcLightFlag_Type_Linear | OBJcLightFlag_Type_Point);

	window = WMrDialog_GetItemByID(inDialog, PLcRB_Area);
	if ((UUtBool)WMrMessage_Send(window, WMcMessage_GetValue, 0, 0) == UUcTrue)
	{
		light_prop->ls_data_array[index].light_flags |= OBJcLightFlag_Type_Area;
	}
	else
	{
		window = WMrDialog_GetItemByID(inDialog, PLcRB_Linear);
		if ((UUtBool)WMrMessage_Send(window, WMcMessage_GetValue, 0, 0) == UUcTrue)
		{
			light_prop->ls_data_array[index].light_flags |= OBJcLightFlag_Type_Linear;
		}
		else
		{
			light_prop->ls_data_array[index].light_flags |= OBJcLightFlag_Type_Point;
		}
	}

	// get the light distribution
	light_prop->ls_data_array[index].light_flags &= ~(OBJcLightFlag_Dist_Diffuse | OBJcLightFlag_Dist_Spot);

	window = WMrDialog_GetItemByID(inDialog, PLcRB_Diffuse);
	if ((UUtBool)WMrMessage_Send(window, WMcMessage_GetValue, 0, 0) == UUcTrue)
	{
		light_prop->ls_data_array[index].light_flags |= OBJcLightFlag_Dist_Diffuse;
	}
	else
	{
		light_prop->ls_data_array[index].light_flags |= OBJcLightFlag_Dist_Spot;
	}

	// get the light intensity
	window = WMrDialog_GetItemByID(inDialog, PLcEF_Intensity);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 255);
	sscanf(string, "%d", &light_prop->ls_data_array[index].light_intensity);

	// get the light beam angle
	window = WMrDialog_GetItemByID(inDialog, PLcEF_BeamAngle);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 255);
	sscanf(string, "%f", &light_prop->ls_data_array[index].beam_angle);

	// get the light field angle
	window = WMrDialog_GetItemByID(inDialog, PLcEF_FieldAngle);
	WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, 255);
	sscanf(string, "%f", &light_prop->ls_data_array[index].field_angle);
}

// ----------------------------------------------------------------------
static void
OWiLightProp_SaveToObject(
	WMtDialog			*inDialog)
{
	OWtLightProp		*light_prop;
	OBJtOSD_All			osd_all;
	UUtUns32			size;

	// get the light properties
	light_prop = (OWtLightProp*)WMrDialog_GetUserData(inDialog);
	UUmAssert(light_prop);

	// get the object osd
	OBJrObject_GetObjectSpecificData(light_prop->object, &osd_all);

	// calculate the size of the ls_data_array
	size = sizeof(OBJtLSData) * light_prop->num_ls_datas;

	// copy the data from the light properties to the object's ls_data_array
	UUrMemory_MoveFast(light_prop->ls_data_array, osd_all.osd.furniture_osd.ls_data_array, size);
}

// ----------------------------------------------------------------------
static void
OWiLightProp_SetFields(
	WMtDialog			*inDialog)
{
	OWtLightProp		*light_prop;
	WMtWindow			*window;
	UUtUns16			index;
	UUtUns16			radiobutton;
	char				string[255];

	// get the light properties
	light_prop = (OWtLightProp*)WMrDialog_GetUserData(inDialog);
	UUmAssert(light_prop);

	// get the current node being edited
	window = WMrDialog_GetItemByID(inDialog, PLcPM_NodeName);
	WMrPopupMenu_GetItemID(window, -1, &index);

	// save the current index number
	light_prop->current_index = index;

	// set the filter color
	window = WMrDialog_GetItemByID(inDialog, PLcEF_R);
	sprintf(string, "%5.3f", light_prop->ls_data_array[index].filter_color[0]);
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	window = WMrDialog_GetItemByID(inDialog, PLcEF_G);
	sprintf(string, "%5.3f", light_prop->ls_data_array[index].filter_color[1]);
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	window = WMrDialog_GetItemByID(inDialog, PLcEF_B);
	sprintf(string, "%5.3f", light_prop->ls_data_array[index].filter_color[2]);
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// set the light type
	if (light_prop->ls_data_array[index].light_flags & OBJcLightFlag_Type_Area)
	{
		radiobutton = (UUtUns16)PLcRB_Area;
	}
	else if (light_prop->ls_data_array[index].light_flags & OBJcLightFlag_Type_Linear)
	{
		radiobutton = (UUtUns16)PLcRB_Linear;
	}
	else
	{
		radiobutton = (UUtUns16)PLcRB_Point;
	}

	WMrDialog_RadioButtonCheck(inDialog, PLcRB_Area, PLcRB_Point, radiobutton);

	// set the light distribution
	if (light_prop->ls_data_array[index].light_flags & OBJcLightFlag_Dist_Diffuse)
	{
		radiobutton = (UUtUns16)PLcRB_Diffuse;
	}
	else
	{
		radiobutton = (UUtUns16)PLcRB_Spot;
	}

	WMrDialog_RadioButtonCheck(inDialog, PLcRB_Diffuse, PLcRB_Spot, radiobutton);

	// set the light intensity
	window = WMrDialog_GetItemByID(inDialog, PLcEF_Intensity);
	sprintf(string, "%d", light_prop->ls_data_array[index].light_intensity);
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// set the light beam angle
	window = WMrDialog_GetItemByID(inDialog, PLcEF_BeamAngle);
	sprintf(string, "%5.3f", light_prop->ls_data_array[index].beam_angle);
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);

	// set the light field angle
	window = WMrDialog_GetItemByID(inDialog, PLcEF_FieldAngle);
	sprintf(string, "%5.3f", light_prop->ls_data_array[index].field_angle);
	WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32)string, 0);
}

// ----------------------------------------------------------------------
static void
OWiLightProp_InitDialog(
	WMtDialog			*inDialog)
{
	OBJtObject			*object;
	OWtLightProp		*light_prop;
	UUtUns32			i;
	WMtWindow			*popup;
	OBJtOSD_All			osd_all;
	UUtUns32			size;

	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL)
	{
		WMrDialog_ModalEnd(inDialog, UUcTrue);
		return;
	}

	light_prop = (OWtLightProp*)UUrMemory_Block_NewClear(sizeof(OWtLightProp));
	if (light_prop == NULL)
	{
		WMrDialog_ModalEnd(inDialog, UUcTrue);
		return;
	}

	// save the light properties
	WMrDialog_SetUserData(inDialog, (UUtUns32)light_prop);

	// get the object specific data
	OBJrObject_GetObjectSpecificData(object, &osd_all);

	// set the light prop data
	light_prop->object = object;
	light_prop->current_index = 0;
	light_prop->num_ls_datas = osd_all.osd.furniture_osd.num_ls_datas;
	light_prop->ls_data_array = NULL;

	// calculate the size of the ls_data_array
	size = sizeof(OBJtLSData) * light_prop->num_ls_datas;

	// allocate memory for the ls_data_array
	light_prop->ls_data_array = (OBJtLSData*)UUrMemory_Block_New(size);
	if (light_prop->ls_data_array == NULL)
	{
		UUrMemory_Block_Delete(light_prop);
		WMrDialog_ModalEnd(inDialog, UUcTrue);
		return;
	}

	// copy the gq_data_array from the osd to the light_prop
	UUrMemory_MoveFast(osd_all.osd.furniture_osd.ls_data_array, light_prop->ls_data_array, size);

	// build the popup menu
	popup = WMrDialog_GetItemByID(inDialog, PLcPM_NodeName);
	for (i = 0; i < light_prop->num_ls_datas; i++)
	{
		WMtMenuItemData			item_data;
		UUtUns32				index;

		index = light_prop->ls_data_array[i].index;

		item_data.flags = WMcMenuItemFlag_Enabled;
		item_data.id = (UUtUns16)i;
		UUrString_Copy(
			item_data.title,
			TMrInstance_GetInstanceName(osd_all.osd.furniture_osd.furn_geom_array->furn_geom[index].geometry),
			WMcMaxTitleLength);

		WMrPopupMenu_AppendItem(popup, &item_data);
	}

	WMrPopupMenu_SetSelection(popup, 0);
}

// ----------------------------------------------------------------------
static void
OWiLightProp_Destroy(
	WMtDialog			*inDialog)
{
	OWtLightProp		*light_prop;

	// delete the memory used
	light_prop = (OWtLightProp*)WMrDialog_GetUserData(inDialog);
	if (light_prop)
	{
		if (light_prop->ls_data_array)
		{
			UUrMemory_Block_Delete(light_prop->ls_data_array);
			light_prop->ls_data_array = NULL;
		}

		UUrMemory_Block_Delete(light_prop);
	}

	OWgDialog_LightProp = NULL;
}

// ----------------------------------------------------------------------
static void
OWiLightProp_HandleCommand(
	WMtDialog			*inDialog,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	switch (UUmLowWord(inParam1))
	{
		case WMcDialogItem_OK:
			OWiLightProp_SaveFields(inDialog);
			OWiLightProp_SaveToObject(inDialog);
			WMrDialog_ModalEnd(inDialog, UUcTrue);
		break;

		case WMcDialogItem_Cancel:
			WMrDialog_ModalEnd(inDialog, UUcFalse);
		break;

		case PLcRB_Area:
		case PLcRB_Linear:
		case PLcRB_Point:
			WMrDialog_RadioButtonCheck(inDialog, PLcRB_Area, PLcRB_Point, (UUtUns16)UUmLowWord(inParam1));
			OWiLightProp_SaveFields(inDialog);
		break;

		case PLcRB_Diffuse:
		case PLcRB_Spot:
			WMrDialog_RadioButtonCheck(inDialog, PLcRB_Diffuse, PLcRB_Spot, (UUtUns16)UUmLowWord(inParam1));
			OWiLightProp_SaveFields(inDialog);
		break;

		default:
			OWiLightProp_SaveFields(inDialog);
		break;
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiLightProp_Callback(
	WMtDialog			*inDialog,
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	UUtBool				handled;

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiLightProp_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			OWiLightProp_Destroy(inDialog);
		return UUcFalse;

		case WMcMessage_Command:
			OWiLightProp_HandleCommand(inDialog, inParam1, inParam2);
		break;

		case WMcMessage_MenuCommand:
			OWiLightProp_SetFields(inDialog);
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrLightProp_Display(
	void)
{
	UUtError				error;

	if ((OWgDialog_Pos == NULL) &&
		(OWgDialog_Prop == NULL) &&
		(OBJrSelectedObjects_GetNumSelected() == 1) &&
		(OWrLightProp_HasLight() == UUcTrue))
	{
		error = WMrDialog_Create(OWcDialog_Prop_Light, NULL, OWiLightProp_Callback, 0, &OWgDialog_LightProp);
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
OWrLightProp_HasLight(
	void)
{
	OBJtObject				*object;
	OBJtOSD_All				osd_all;
	UUtUns32				i;

	// must be only one object selected
	if (OBJrSelectedObjects_GetNumSelected() != 1) { return UUcFalse; }

	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return UUcFalse; }

	// object must be of type furniture
	if (OBJrObject_GetType(object) != OBJcType_Furniture) { return UUcFalse; }

	// get the object specific data
	OBJrObject_GetObjectSpecificData(object, &osd_all);

	// see if any of the geometries have light data associated with them
	for (i = 0; i < osd_all.osd.furniture_osd.furn_geom_array->num_furn_geoms; i++)
	{
		if (osd_all.osd.furniture_osd.furn_geom_array->furn_geom[i].ls_data != NULL) { return UUcTrue; }
	}

	return UUcFalse;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Particle_SetFields(
	WMtDialog			*inDialog)
{
	OBJtOSD_All			*osd_all;
	WMtWindow			*window;
//	char				string[255];

	// get the particle object's properties
	osd_all = (OBJtOSD_All*)WMrDialog_GetUserData(inDialog);
	UUmAssert(osd_all);

	// update the controls

	// particle class
	window = WMrDialog_GetItemByID(inDialog, PPcLB_ClassList);
	if (window) {
		WMrMessage_Send(window, LBcMessage_SelectString, (UUtUns32) -1, (UUtUns32) osd_all->osd.particle_osd.particle.classname);
	}

/*
	// CB: this is now obsolete

	// velocity
	window = WMrDialog_GetItemByID(inDialog, PPcEF_Velocity);
	if (window) {
		sprintf(string, "%f", osd_all->osd.particle_osd.velocity);
		WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32) string, 0);
	}*/

	// tag name
	window = WMrDialog_GetItemByID(inDialog, PPcEF_Tag);
	if (window) {
		WMrMessage_Send(window, EFcMessage_SetText, (UUtUns32) osd_all->osd.particle_osd.particle.tagname, 0);
	}

	// flags
	window = WMrDialog_GetItemByID(inDialog, PPcCB_NotInitiallyCreated);
	if (window) {
		WMrMessage_Send(window, CBcMessage_SetCheck,
			(UUtUns32) ((osd_all->osd.particle_osd.particle.flags & EPcParticleFlag_NotInitiallyCreated)
							? UUcTrue : UUcFalse), (UUtUns32) -1);
	}

	// disable the ok button if the object type is locked
	window = WMrDialog_GetItemByID(inDialog, WMcDialogItem_OK);
	WMrWindow_SetEnabled(window, !OBJrObjectType_IsLocked(OBJcType_Particle));
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Particle_InitDialog(
	WMtDialog			*inDialog)
{

	WMtWindow			*listbox;
	OBJtObject			*object;
	OBJtObjectType		object_type;
	OBJtOSD_All			*osd;
	P3tParticleClassIterateState	particle_itr;
	P3tParticleClass	*particle_class;
//	WMtWindow			*popup;
	WMtWindow			*editfield;

	// get the currently selected object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object == NULL) { return; }

	// must be a particle object!
	object_type = OBJrObject_GetType(object);
	if (object_type != OBJcType_Particle) {
		WMrDialog_ModalEnd(inDialog, UUcTrue);
		return;
	}

	// get a pointer to the listbox
	listbox = WMrDialog_GetItemByID(inDialog, PPcLB_ClassList);
	if (listbox == NULL) { return; }

	// iterate all particle classes and place their names in the list
	particle_itr = P3cParticleClass_None;
	while (P3rIterateClasses(&particle_itr, &particle_class)) {
		WMrMessage_Send(listbox, LBcMessage_AddString, (UUtUns32) particle_class->classname, 0);
	}

	// allocate memory for the dialog's OSD
	osd = (OBJtOSD_All*)UUrMemory_Block_NewClear(sizeof(OBJtOSD_All));
	UUmAssert(osd);
	OBJrObject_GetObjectSpecificData( object, osd );

	WMrDialog_SetUserData(inDialog, (UUtUns32)osd);

	// set the maximum number of characters in the editfields
	editfield = WMrDialog_GetItemByID(inDialog, PPcEF_Tag);
	WMrMessage_Send(editfield, EFcMessage_SetMaxChars, OBJcParticleTagNameLength, 0);

	// set the keyboard focus to the listbox
	WMrWindow_SetFocus(listbox);

	// set the fields
	OWiProp_Particle_SetFields(inDialog);
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OWiProp_Particle_NewClass(
	WMtDialog			*inDialog)
{
	char				classname[P3cParticleClassNameLength + 1];
	UUtUns32			returncode, itr;
	WMtWindow			*listbox, *window;
	WMtEditField		*editfield;
	P3tParticleClass	*classptr;
	UUtBool				enable_decal_fields;
	OBJtOSD_All			*osd_all;
	OBJtObject			*object;

	osd_all = (OBJtOSD_All *) WMrDialog_GetUserData(inDialog);

	// get the particle class
	listbox = WMrDialog_GetItemByID(inDialog, PPcLB_ClassList);
	if (listbox == NULL) {
		return;
	}

	returncode = WMrMessage_Send(listbox, LBcMessage_GetText, (UUtUns32) classname, (UUtUns32) -1);
	if (returncode != LBcNoError) {
		return;
	}

	classptr = P3rGetParticleClass(classname);
	enable_decal_fields = ((classptr != NULL) && (classptr->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal));

	for (itr = PPcEF_SizeX; itr <= PPcTxt_Angle; itr++) {
		window = WMrDialog_GetItemByID(inDialog, (UUtUns16) itr);
		if (window == NULL)
			continue;

		WMrWindow_SetVisible(window, enable_decal_fields);
	}

	if (enable_decal_fields) {
		// get a pointer to the object
		object = OBJrSelectedObjects_GetSelectedObject(0);
		UUmAssert(object);

		// set the decal sizes and angle
		editfield = (WMtEditField *) WMrDialog_GetItemByID(inDialog, PPcEF_SizeX);
		WMrEditField_SetFloat(editfield, osd_all->osd.particle_osd.particle.decal_xscale);

		editfield = (WMtEditField *) WMrDialog_GetItemByID(inDialog, PPcEF_SizeY);
		WMrEditField_SetFloat(editfield, osd_all->osd.particle_osd.particle.decal_yscale);

		// note: we round rotation to the nearest whole angle
		editfield = (WMtEditField *) WMrDialog_GetItemByID(inDialog, PPcEF_Angle);
		WMrEditField_SetFloat(editfield, (float)floor(object->rotation.y));
	}
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtError
OWiProp_Particle_SaveOSD(
	WMtDialog			*inDialog)
{
	OBJtObject			*object;
	P3tParticleClass	*classptr;
	WMtWindow			*window;
	WMtEditField		*editfield;
	OBJtOSD_All			osd_all;
	OBJtOSD_Particle	*particle_osd;
	char				string[OBJcMaxNoteChars + 1];
	float				x_scale, y_scale, angle;

	if (OBJrObjectType_IsLocked(OBJcType_Particle) == UUcTrue)
	{
		WMrDialog_MessageBox(inDialog, "Error", "The Particle file is locked, you cannot make changes.",
							WMcMessageBoxStyle_OK);
		WMrDialog_ModalEnd(inDialog, UUcFalse);
		return UUcError_Generic;
	}

	// get a pointer to the object
	object = OBJrSelectedObjects_GetSelectedObject(0);
	UUmAssert(object);

	particle_osd = (OBJtOSD_Particle *) object->object_data;

	osd_all.osd.particle_osd = *particle_osd;

	// get the class name from the listbox
	window = WMrDialog_GetItemByID(inDialog, PPcLB_ClassList);
	if (window) {
		if (WMrMessage_Send(window, LBcMessage_GetText, (UUtUns32)string, (UUtUns32)(-1)) == LBcNoError) {
			UUrString_Copy( osd_all.osd.particle_osd.particle.classname, string, P3cParticleClassNameLength + 1);
		}
	}

	// get the tag name
	window = WMrDialog_GetItemByID(inDialog, PPcEF_Tag);
	if (window) {
		WMrMessage_Send(window, EFcMessage_GetText, (UUtUns32)string, EPcParticleTagNameLength);
		UUrString_Copy( osd_all.osd.particle_osd.particle.tagname, string, EPcParticleTagNameLength + 1);
	}

	// get the flags
	window = WMrDialog_GetItemByID(inDialog, PPcCB_NotInitiallyCreated);
	if (WMrMessage_Send(window, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1))
		osd_all.osd.particle_osd.particle.flags |= EPcParticleFlag_NotInitiallyCreated;
	else
		osd_all.osd.particle_osd.particle.flags &= ~EPcParticleFlag_NotInitiallyCreated;

	classptr = P3rGetParticleClass(osd_all.osd.particle_osd.particle.classname);
	if ((classptr != NULL) && (classptr->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal)) {
		// get the decal sizes
		editfield = (WMtEditField *) WMrDialog_GetItemByID(inDialog, PPcEF_SizeX);
		x_scale = WMrEditField_GetFloat(editfield);

		editfield = (WMtEditField *) WMrDialog_GetItemByID(inDialog, PPcEF_SizeY);
		y_scale = WMrEditField_GetFloat(editfield);

		editfield = (WMtEditField *) WMrDialog_GetItemByID(inDialog, PPcEF_Angle);
		angle = WMrEditField_GetFloat(editfield);
		angle = UUmPin(0.0f, angle, 360.0f);
		WMrEditField_SetFloat(editfield, angle);

		osd_all.osd.particle_osd.particle.decal_xscale = x_scale;
		osd_all.osd.particle_osd.particle.decal_yscale = y_scale;
		object->rotation.y = angle;
	} else {
		// not a decal class
		osd_all.osd.particle_osd.particle.decal_xscale = 1.0f;
		osd_all.osd.particle_osd.particle.decal_yscale = 1.0f;
	}

	OWrObjectProperties_SetOSD(inDialog, object, &osd_all);
	OBJrObject_UpdatePosition(object);

	return UUcError_None;
}
#endif

// ----------------------------------------------------------------------
#if TOOL_VERSION
static UUtBool
OWrProp_Particle_Callback(
	WMtDialog			*inDialog,
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	UUtBool				handled;
	UUtError			error;
	UUtUns16			value_field;
	float				value, value_delta;
	OBJtOSD_All *		osd_all;
	WMtWindow *			window;

	handled = UUcTrue;
	osd_all = (OBJtOSD_All *) WMrDialog_GetUserData(inDialog);

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiProp_Particle_InitDialog(inDialog);
		break;

		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(osd_all);
		break;

		case WMcMessage_Command:
			value_delta = 0;
			value_field = (UUtUns16) -1;
			switch (UUmLowWord(inParam1))
			{
				case PPcLB_ClassList:
					OWiProp_Particle_NewClass(inDialog);
				break;

				case PPcBtn_SizeXPlusSmall:		value_field = PPcEF_SizeX;		value_delta = +PPcDecalSize_EditSmall;		break;
				case PPcBtn_SizeXMinusSmall:	value_field = PPcEF_SizeX;		value_delta = -PPcDecalSize_EditSmall;		break;
				case PPcBtn_SizeYPlusSmall:		value_field = PPcEF_SizeY;		value_delta = +PPcDecalSize_EditSmall;		break;
				case PPcBtn_SizeYMinusSmall:	value_field = PPcEF_SizeY;		value_delta = -PPcDecalSize_EditSmall;		break;

				case PPcBtn_SizeXPlusLarge:		value_field = PPcEF_SizeX;		value_delta = +PPcDecalSize_EditLarge;		break;
				case PPcBtn_SizeXMinusLarge:	value_field = PPcEF_SizeX;		value_delta = -PPcDecalSize_EditLarge;		break;
				case PPcBtn_SizeYPlusLarge:		value_field = PPcEF_SizeY;		value_delta = +PPcDecalSize_EditLarge;		break;
				case PPcBtn_SizeYMinusLarge:	value_field = PPcEF_SizeY;		value_delta = -PPcDecalSize_EditLarge;		break;

				case PPcBtn_AnglePlusSmall:		value_field = PPcEF_Angle;		value_delta = +PPcDecalAngle_EditSmall;		break;
				case PPcBtn_AngleMinusSmall:	value_field = PPcEF_Angle;		value_delta = -PPcDecalAngle_EditSmall;		break;
				case PPcBtn_AnglePlusLarge:		value_field = PPcEF_Angle;		value_delta = +PPcDecalAngle_EditLarge;		break;
				case PPcBtn_AngleMinusLarge:	value_field = PPcEF_Angle;		value_delta = -PPcDecalAngle_EditLarge;		break;

				case PPcEF_SizeX:
				case PPcEF_SizeY:
				case PPcEF_Angle:
					// update the particle so that we can see our changes
					OWiProp_Particle_SaveOSD(inDialog);
				break;

				case WMcDialogItem_OK:
					error = OWiProp_Particle_SaveOSD(inDialog);
					WMrDialog_ModalEnd(inDialog, (error == UUcError_None) ? UUcTrue : UUcFalse);
				break;

				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, UUcFalse);
				break;
			}

			if (value_delta != 0) {
				UUmAssert(value_field != (UUtUns16) -1);
				window = WMrDialog_GetItemByID(inDialog, value_field);
				if (window != NULL) {
					// set the changed value
					value = WMrEditField_GetFloat((WMtEditField *) window);
					value += value_delta;
					if (value_field == PPcEF_Angle) {
						// clip angle to trig range
						if (value < 0) {
							value += 360.0f;
						} else if (value >= 360.0f) {
							value -= 360.0f;
						}
					}
					WMrEditField_SetFloat((WMtEditField *) window, value);
				}

				// update the particle so that we can see our changes immediately
				OWiProp_Particle_SaveOSD(inDialog);
			}
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
UUtError
OWrTools_CreateObject(
	OBJtObjectType			inObjectType,
	const OBJtOSD_All		*inObjectSpecificData)
{
	M3tPoint3D				position;
	M3tPoint3D				rotation;
	M3tVector3D				facing;
	UUtError				error;
	ONtCharacter			*player_character;
	float					horiz_facing;
	float					pitch;

	// determine the position of the object
	switch(inObjectType)
	{
		case OBJcType_Door:
		case OBJcType_Console:
			if (CAgCamera.mode == CAcMode_Follow)
			{
				player_character = CAgCamera.star;
				UUmAssert(player_character != NULL);

				position = CArGetLocation();
				facing = CArGetFacing();

				MUmVector_Set(rotation, 0, player_character->facing * M3cRadToDeg, 0);

				rotation.y += 180;
				if( rotation.y > 360.0 )
					rotation.y -= 360;

				position.x = player_character->location.x;
				position.z = player_character->location.z;

				position.x += MUrSin( player_character->facing ) * 10;
				position.z += MUrCos( player_character->facing ) * 10;
			}
			else
			{
				position = CArGetLocation();
				facing = CArGetFacing();

				horiz_facing = MUrATan2(facing.x, facing.z) * M3cRadToDeg;

				horiz_facing += 180;
				if( horiz_facing > 360.0 )
					horiz_facing -= 360;

				MUmVector_Scale(facing, 35.0f);

				// put the object in front of the camera
				position.x += facing.x;
				position.y += facing.y;
				position.z += facing.z;

				rotation.x = 0.0f;
				rotation.y = horiz_facing;
				rotation.z = 0.0f;
			}
			break;

		case OBJcType_Particle:
			{
				facing			= CArGetFacing();

				horiz_facing	= MUrATan2(facing.x, facing.z);

				pitch			= MUrASin( (float)fabs(facing.y) );

				if( facing.y > 0 )		pitch		= -pitch;

				UUmTrig_Clip( pitch );
				UUmTrig_Clip( horiz_facing );


				if (CAgCamera.mode == CAcMode_Follow)
				{
					ONrCharacter_GetEyePosition(CAgCamera.star, &position);
					MUmVector_ScaleIncrement(position, 10.0f, facing);
				}
				else
				{
					position		= CAgCamera.viewData.location;
					MUmVector_ScaleIncrement(position, 35.0f, facing);
				}

				MUmVector_Set(rotation, pitch * M3cRadToDeg, horiz_facing * M3cRadToDeg, 0);
			}
			break;

		case OBJcType_Character:
		case OBJcType_Flag:
			// appear at the position of the player character if possible
			if (CAgCamera.mode == CAcMode_Follow) {
				player_character = CAgCamera.star;
				UUmAssert(player_character != NULL);

				position = player_character->location;

				MUmVector_Set(rotation, 0, player_character->facing * M3cRadToDeg, 0);
				break;
			}

			// otherwise fall through

		default:
			// appear in front of the camera
			position = CArGetLocation();
			facing = CArGetFacing();

			MUmVector_Scale(facing, 35.0f);

			// put the object in front of the camera
			position.x += facing.x;
			position.y += facing.y;
			position.z += facing.z;

			rotation.x = 0.0f;
			rotation.y = 0.0f;
			rotation.z = 0.0f;
			break;
	}

	// apply gravity for specific object types
	switch (inObjectType)
	{
		case OBJcType_Console:
		case OBJcType_Door:
		case OBJcType_Turret:
		case OBJcType_Trigger:
		case OBJcType_Flag:
		case OBJcType_Furniture:
		case OBJcType_PowerUp:
		case OBJcType_Weapon:
		{
			UUtBool				result;
			M3tVector3D			vector;

			MUmVector_Set(vector, 0.0f, -255.0f, 0.0f);

			result =
				AKrCollision_Point(
					ONrGameState_GetEnvironment(),
					&position,
					&vector,
					AKcGQ_Flag_Obj_Col_Skip,
					AKcMaxNumCollisions);
			if (result == UUcTrue)
			{
				// move the object to the floor
				position.y = AKgCollisionList[0].collisionPoint.y;
			}
		}
		break;
	}

	// create the object based on its type and name
	error =
		OBJrObject_New(
			inObjectType,
			&position,
			&rotation,
			inObjectSpecificData);
	if (error != UUcError_None)
	{
		WMrDialog_MessageBox(
			NULL,
			"Error",
			"Unable to create new object.  The .bin file may be locked.",
			WMcMessageBoxStyle_OK);
	}

	return error;
}

// ----------------------------------------------------------------------
static UUtBool
OWiTools_EnumCategories(
	OBJtObjectType			inObjectType,
	const char				*inName,
	UUtUns32				inUserData)
{
	OWgCategories[OWgNumCategories].object_type = inObjectType;
	UUrString_Copy(OWgCategories[OWgNumCategories].name, inName, OBJcMaxNameLength);
	OWgNumCategories++;

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtError
OWrTools_Initialize(
	void)
{
	UUmAssert(OWgDialog_ObjNew == NULL);
	UUmAssert(OWgDialog_Prop == NULL);

	// clear the categories
	UUrMemory_Clear(OWgCategories, sizeof(OWtCategory) * OWcMaxCategories);
	OWgNumCategories = 0;

	// enumerate the object types
	OBJrObjectTypes_Enumerate(OWiTools_EnumCategories, 0);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void OWrTools_Terminate(void)
{
}

// ----------------------------------------------------------------------
// string list dialog box
// ----------------------------------------------------------------------

UUtError OWrStringListDialog_Create( OWtStringListDlgInstance* inInstance )
{
	UUmAssert( inInstance );

	inInstance->string_array	= UUrMemory_Array_New( OWcStringListDlg_MaxStringLength,  OWcStringListDlg_MaxStringLength, 0, 0 );
	inInstance->dialog			= NULL;
	inInstance->selected_index	= -1;
	inInstance->title[0]		= 0;
	inInstance->list_title[0]	= 0;

	return UUcError_None;
}

UUtError OWrStringListDialog_Destroy( OWtStringListDlgInstance* inInstance )
{
	UUmAssert( inInstance );
	UUmAssert( inInstance->string_array );

	UUrMemory_Array_Delete( inInstance->string_array );

	inInstance->string_array	= NULL;
	inInstance->dialog			= NULL;

	return UUcError_None;
}

UUtError OWrStringListDialog_AddString( OWtStringListDlgInstance* inInstance, char *inString )
{
	UUtError			error;
	UUtUns32			new_index;
	char*				data;

	UUmAssert( inInstance );
	UUmAssert( inString && strlen( inString ) < OWcStringListDlg_MaxStringLength );

	error = UUrMemory_Array_GetNewElement( inInstance->string_array, &new_index, NULL );
	UUmAssert( error == UUcError_None );

	data = (char*) UUrMemory_Array_GetMemory( inInstance->string_array );
	UUrString_Copy( &data[ new_index * OWcStringListDlg_MaxStringLength ], inString, OWcStringListDlg_MaxStringLength );

	return UUcError_None;
}

static UUtBool OWiStringListDialog_Callback( WMtDialog *inDialog, WMtMessage inMessage, UUtUns32 inParam1, UUtUns32 inParam2 )
{
	UUtBool							handled;
	OWtStringListDlgInstance*		instance;
	UUtUns32						i;
	WMtWindow*						window;

	instance = (OWtStringListDlgInstance*) WMrDialog_GetUserData(inDialog);

	UUmAssert( instance && instance->string_array );

	handled = UUcTrue;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			{
				char		*element;
				UUtUns32	element_count;

				UUmAssert( !instance->dialog );

				instance->dialog = inDialog;

				// set title
				if( instance->title[0] )
 			{
					WMrWindow_SetTitle( (WMtWindow*) inDialog, instance->title, OWcStringListDlg_MaxStringLength );
				}

				// set list title
				if( instance->list_title[0] )
				{
					window = WMrDialog_GetItemByID( inDialog, DSLcLB_StringTitle );			UUmAssert( window );
					WMrWindow_SetTitle( window, instance->list_title, OWcStringListDlg_MaxStringLength );
				}

				// fill the screen list
				window			= WMrDialog_GetItemByID( inDialog, DSLcLB_StringList );			UUmAssert( window );
				element_count	= UUrMemory_Array_GetUsedElems( instance->string_array );
				element			= (char*) UUrMemory_Array_GetMemory( instance->string_array );
				for( i = 0; i < element_count; i++ )
				{
					WMrMessage_Send( window, LBcMessage_AddString, (UUtUns32) element, 0);
					element += OWcStringListDlg_MaxStringLength;
				}

				WMrMessage_Send( window, LBcMessage_SetSelection, (UUtUns32) 0, instance->selected_index );
				WMrWindow_SetFocus( window );
			}
		break;

		case WMcMessage_Destroy:
			UUmAssert( instance->dialog );
			instance->dialog = NULL;
		break;

		case WMcMessage_Command:
			UUmAssert( instance->dialog );
			switch (UUmLowWord(inParam1))
			{
				case DSLcLB_StringList:
				case WMcDialogItem_OK:
					if((( UUmLowWord(inParam1) == WMcDialogItem_OK ) && (UUmHighWord(inParam1) == WMcNotify_Click)) || ( UUmHighWord(inParam1) == WMcNotify_DoubleClick ))
					{
						window = WMrDialog_GetItemByID( inDialog, DSLcLB_StringList );			UUmAssert( window );
						instance->selected_index = WMrMessage_Send( window, LBcMessage_GetSelection, (UUtUns32)-1, (UUtUns32)-1 );
						WMrDialog_ModalEnd( inDialog, 1 );
					}
				break;

				case WMcDialogItem_Cancel:
					if (UUmHighWord(inParam1) == WMcNotify_Click)
					{
						WMrDialog_ModalEnd( inDialog, 0 );
					}
				break;
			}
		break;

		default:
			handled = UUcFalse;
			break;
	}
	return handled;
}

UUtError OWrStringListDialog_SetTitle( OWtStringListDlgInstance* inInstance, char* inTitle )
{
	UUmAssert( inInstance && inInstance->string_array );

	UUrString_Copy( inInstance->title, inTitle, OWcStringListDlg_MaxStringLength );

	if( inInstance->dialog )
	{
		WMrWindow_SetTitle( (WMtWindow*) inInstance->dialog, inInstance->title, OWcStringListDlg_MaxStringLength );
	}

	return UUcError_None;
}

UUtError OWrStringListDialog_SetListTitle( OWtStringListDlgInstance* inInstance, char* inTitle )
{
	UUmAssert( inInstance && inInstance->string_array );

	UUrString_Copy( inInstance->list_title, inTitle, OWcStringListDlg_MaxStringLength );

	if( inInstance->dialog )
	{
		WMtWindow*		window;
		window = WMrDialog_GetItemByID( inInstance->dialog, DSLcLB_StringTitle );		UUmAssert( window );
		WMrMessage_Send( window, EFcMessage_SetText, (UUtUns32) inInstance->list_title, 0);
	}

	return UUcError_None;
}

UUtError OWrStringListDialog_DoModal( OWtStringListDlgInstance* inInstance )
{
	UUtError			error;

	UUmAssert( inInstance && !inInstance->dialog );

	if( inInstance->dialog )
		return UUcError_Generic;

	error = WMrDialog_ModalBegin( OWcDialog_StringList, NULL, OWiStringListDialog_Callback, (UUtUns32) inInstance, (UUtUns32*) &inInstance->return_value );

	UUmAssert( error == UUcError_None );

	return UUcError_None;
}

UUtBool OWrObjectProperties_SetOSD(WMtDialog *inDialog, OBJtObject *inObject, OBJtOSD_All *inOSD)
{
	UUtError			error;

	if (OBJrObjectType_IsLocked(inObject->object_type)) {
		char errmsg[256];

		sprintf(errmsg, "%s.bin is locked; you cannot change this object.", OBJrObjectType_GetName(inObject->object_type));
		WMrDialog_MessageBox(inDialog, "Object Locked", errmsg, WMcMessageBoxStyle_OK);
		return UUcFalse;
	}

	error = OBJrObject_SetObjectSpecificData(inObject, inOSD);
	if (error != UUcError_None) {
		WMrDialog_MessageBox(inDialog, "Internal Error", "There was an error changing this object's properties.",
							WMcMessageBoxStyle_OK);
		return UUcFalse;
	}

	return UUcTrue;
}


