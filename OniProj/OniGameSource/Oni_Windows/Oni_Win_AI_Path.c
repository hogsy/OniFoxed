#if TOOL_VERSION
/*
	FILE:	Oni_Win_AI_Path.c

	AUTHOR:	Michael Evans

	CREATED: April 24, 2000

	PURPOSE: AI Pathing Windows

	Copyright (c) Bungie Software 2000

*/

#include "BFW.h"
#include "Oni_Win_AI.h"
#include "Oni_Windows.h"
#include "Oni_Object.h"
#include "Oni_AI2_Movement.h"
#include "Oni_Character.h"
#include "Oni_Win_Tools.h"

#include "WM_DrawContext.h"
#include "WM_PartSpecification.h"
#include "WM_Scrollbar.h"
#include "WM_MenuBar.h"
#include "WM_Menu.h"
#include "WM_PopupMenu.h"
#include "WM_EditField.h"
#include "WM_CheckBox.h"

/*** globals */

// patrol path for display and immediate add-waypoint-to
UUtBool			OWgShowPatrolPath;
UUtBool			OWgCurrentPatrolPathValid;
UUtUns16		OWgCurrentPatrolPathID;
AI2tPatrolPath	*OWgCurrentPatrolPath;

// globals for controlling waypoint editing
UUtBool			OWgWaypointHasDefaultFlag;
UUtUns16		OWgWaypointDefaultFlagID;

/*** constants */

typedef struct OWtWaypointType {
	char *				name;
	UUtUns16			id;
} OWtWaypointType;

static const OWtWaypointType cWaypointTypes[] =
{
	{"Stop At Flag",			AI2cWaypointType_MoveToFlag},
	{"Stop Near Flag",			AI2cWaypointType_MoveNearFlag},
	{"Stop And Orient To Flag",	AI2cWaypointType_MoveAndFaceFlag},
	{"Move To Flag",			AI2cWaypointType_MoveThroughFlag},
	{"",						0},
	{"Look At Flag",			AI2cWaypointType_LookAtFlag},
	{"Stop Looking",			AI2cWaypointType_StopLooking},
	{"Lock Facing",				AI2cWaypointType_LookInDirection},
	{"Free Facing",				AI2cWaypointType_FreeFacing},
	{"",						0},
	{"Glance At Flag",			AI2cWaypointType_GlanceAtFlag},
	{"Scan",					AI2cWaypointType_Scan},
	{"Stop Scanning",			AI2cWaypointType_StopScanning},
	{"Guard Flag",				AI2cWaypointType_Guard},
	{"Shoot At Flag",			AI2cWaypointType_ShootAtFlag},
	{"",						0},
	{"Lock Onto Path",			AI2cWaypointType_LockIntoPatrol},
	{"Trigger Script",			AI2cWaypointType_TriggerScript},
	{"Play Script",				AI2cWaypointType_PlayScript},
	{"Movement Mode",			AI2cWaypointType_MovementMode},
	{"Pause",					AI2cWaypointType_Pause},
	{"Loop",					AI2cWaypointType_Loop},
	{"Loop To Waypoint",		AI2cWaypointType_LoopToWaypoint},
	{"Exit Patrol",				AI2cWaypointType_Stop},
	{NULL,						0}
};

static char *cMovementModes[AI2cMovementMode_Max] =
{
	"Default",
	"Stop [obsolete]",
	"Crouch [obsolete]",
	"Creep",
	"No Aim Walk",
	"Walk",
	"NoAim Run",
	"Run"
};

static char *cMovementDirections[AI2cMovementDirection_Max] =
{
	"Forward",
	"Backpedal",
	"Sidestep Left",
	"Sidestep Right",
	"Stopped"
};

static UUtInt16 AIrWaypoint_GetFlag(AI2tWaypoint *inWaypoint)
{
	UUtInt16 flag = 0;

	switch(inWaypoint->type)
	{
		case AI2cWaypointType_MoveToFlag:
			flag = inWaypoint->data.moveToFlag.flag;
		break;

		case AI2cWaypointType_LookAtFlag:
			flag = inWaypoint->data.lookAtFlag.lookFlag;
		break;

		case AI2cWaypointType_MoveAndFaceFlag:
			flag = inWaypoint->data.moveAndFaceFlag.faceFlag;
		break;

		case AI2cWaypointType_MoveThroughFlag:
			flag = inWaypoint->data.moveThroughFlag.flag;
		break;

		case AI2cWaypointType_GlanceAtFlag:
			flag = inWaypoint->data.glanceAtFlag.glanceFlag;
		break;

		case AI2cWaypointType_MoveNearFlag:
			flag = inWaypoint->data.moveNearFlag.flag;
		break;

		case AI2cWaypointType_Guard:
			flag = inWaypoint->data.guard.flag;
		break;

		case AI2cWaypointType_ShootAtFlag:
			flag = inWaypoint->data.shootAtFlag.flag;
		break;

		default:
			flag = 0;
		break;
	}

	return flag;
}

static void AIrWaypoint_SetFlag(AI2tWaypoint *inWaypoint, UUtInt16 inFlag)
{
	switch(inWaypoint->type)
	{
		case AI2cWaypointType_MoveToFlag:
			inWaypoint->data.moveToFlag.flag = inFlag;
		break;

		case AI2cWaypointType_LookAtFlag:
			inWaypoint->data.lookAtFlag.lookFlag = inFlag;
		break;

		case AI2cWaypointType_MoveAndFaceFlag:
			inWaypoint->data.moveAndFaceFlag.faceFlag = inFlag;
		break;

		case AI2cWaypointType_MoveThroughFlag:
			inWaypoint->data.moveThroughFlag.flag = inFlag;
		break;

		case AI2cWaypointType_GlanceAtFlag:
			inWaypoint->data.glanceAtFlag.glanceFlag = inFlag;
		break;

		case AI2cWaypointType_MoveNearFlag:
			inWaypoint->data.moveNearFlag.flag = inFlag;
		break;

		case AI2cWaypointType_Guard:
			inWaypoint->data.guard.flag = inFlag;
		break;

		case AI2cWaypointType_ShootAtFlag:
			inWaypoint->data.shootAtFlag.flag = inFlag;
		break;

		default:
		break;
	}
}

static UUtBool AIrWaypoint_HasFlag(AI2tWaypoint *inWaypoint)
{
	UUtBool has_flag;

	switch(inWaypoint->type)
	{
		case AI2cWaypointType_MoveToFlag:
		case AI2cWaypointType_LookAtFlag:
		case AI2cWaypointType_MoveAndFaceFlag:
		case AI2cWaypointType_MoveThroughFlag:
		case AI2cWaypointType_GlanceAtFlag:
		case AI2cWaypointType_MoveNearFlag:
		case AI2cWaypointType_Guard:
		case AI2cWaypointType_ShootAtFlag:
			has_flag = UUcTrue;
		break;

		default:
			has_flag = UUcFalse;
		break;
	}

	return has_flag;
}

static char *AIrWaypoint_GetIntParamName(AI2tWaypoint *inWaypoint)
{
	char *name;

	switch(inWaypoint->type)
	{
		case AI2cWaypointType_GlanceAtFlag:
			name = "Glance Frames:";
		break;

		case AI2cWaypointType_Pause:
			name = "Pause Frames:";
		break;

		case AI2cWaypointType_LoopToWaypoint:
			name = "Waypoint:";
		break;

		case AI2cWaypointType_Scan:
			name = "Scan Frames:";
		break;

		case AI2cWaypointType_Guard:
			name = "Guard Frames:";
		break;

		case AI2cWaypointType_ShootAtFlag:
			name = "Shoot Frames:";
		break;

		case AI2cWaypointType_TriggerScript:
		case AI2cWaypointType_PlayScript:
			name = "Script Number:";
		break;

		case AI2cWaypointType_LockIntoPatrol:
			name = "Lock (0/1):";
		break;

		default:
			name = NULL;
		break;
	}

	return name;
}

static UUtInt32 AIrWaypoint_GetIntParam(AI2tWaypoint *inWaypoint)
{
	UUtInt32 param;

	switch(inWaypoint->type)
	{
		case AI2cWaypointType_Pause:
			param = inWaypoint->data.pause.count;
		break;

		case AI2cWaypointType_GlanceAtFlag:
			param = inWaypoint->data.glanceAtFlag.glanceFrames;
		break;

		case AI2cWaypointType_LoopToWaypoint:
			param = inWaypoint->data.loopToWaypoint.waypoint_index + 1;
		break;

		case AI2cWaypointType_Scan:
			param = inWaypoint->data.scan.scan_frames;
		break;

		case AI2cWaypointType_Guard:
			param = inWaypoint->data.guard.guard_frames;
		break;

		case AI2cWaypointType_ShootAtFlag:
			param = inWaypoint->data.shootAtFlag.shoot_frames;
		break;

		case AI2cWaypointType_TriggerScript:
			param = inWaypoint->data.triggerScript.script_number;
		break;

		case AI2cWaypointType_PlayScript:
			param = inWaypoint->data.playScript.script_number;
		break;

		case AI2cWaypointType_LockIntoPatrol:
			param = (inWaypoint->data.lockIntoPatrol.lock) ? 1 : 0;
		break;

		default:
			param = 0;
		break;
	}

	return param;
}

static void AIrWaypoint_SetIntParam(AI2tWaypoint *inWaypoint, UUtInt32 inParam)
{
	switch(inWaypoint->type)
	{
		case AI2cWaypointType_Pause:
			inWaypoint->data.pause.count = inParam;
		break;

		case AI2cWaypointType_GlanceAtFlag:
			inWaypoint->data.glanceAtFlag.glanceFrames = inParam;
		break;

		case AI2cWaypointType_LoopToWaypoint:
			inWaypoint->data.loopToWaypoint.waypoint_index = inParam - 1;
		break;

		case AI2cWaypointType_Scan:
			inWaypoint->data.scan.scan_frames = (UUtInt16) inParam;
		break;

		case AI2cWaypointType_Guard:
			inWaypoint->data.guard.guard_frames = (UUtInt16) inParam;
		break;

		case AI2cWaypointType_ShootAtFlag:
			inWaypoint->data.shootAtFlag.shoot_frames = (UUtInt16) inParam;
		break;

		case AI2cWaypointType_TriggerScript:
			inWaypoint->data.triggerScript.script_number = (UUtInt16) inParam;
		break;

		case AI2cWaypointType_PlayScript:
			inWaypoint->data.playScript.script_number = (UUtInt16) inParam;
		break;

		case AI2cWaypointType_LockIntoPatrol:
			inWaypoint->data.lockIntoPatrol.lock = (inParam > 0);
		break;

		default:
		break;
	}

	return;
}

static UUtBool AIrWaypoint_HasIntParam(AI2tWaypoint *inWaypoint)
{
	UUtBool has_param;

	switch(inWaypoint->type)
	{
		case AI2cWaypointType_Pause:
		case AI2cWaypointType_GlanceAtFlag:
		case AI2cWaypointType_LoopToWaypoint:
		case AI2cWaypointType_Scan:
		case AI2cWaypointType_Guard:
		case AI2cWaypointType_TriggerScript:
		case AI2cWaypointType_PlayScript:
		case AI2cWaypointType_LockIntoPatrol:
		case AI2cWaypointType_ShootAtFlag:
			has_param = UUcTrue;
		break;

		default:
			has_param = UUcFalse;
		break;
	}

	return has_param;
}

static char *AIrWaypoint_GetFloatParamName(AI2tWaypoint *inWaypoint)
{
	char *name;

	switch(inWaypoint->type)
	{
		case AI2cWaypointType_MoveThroughFlag:
		case AI2cWaypointType_MoveThroughPoint:
		case AI2cWaypointType_MoveNearFlag:
			name = "Distance:";
		break;

		case AI2cWaypointType_Scan:
		case AI2cWaypointType_Guard:
			name = "Turn Angle:";
		break;

		case AI2cWaypointType_ShootAtFlag:
			name = "Inaccuracy:";
		break;

		default:
			name = NULL;
		break;
	}

	return name;
}

static float AIrWaypoint_GetFloatParam(AI2tWaypoint *inWaypoint)
{
	float param;

	switch(inWaypoint->type)
	{
		case AI2cWaypointType_MoveThroughFlag:
			param = inWaypoint->data.moveThroughFlag.required_distance;
		break;

		case AI2cWaypointType_MoveThroughPoint:
			param = inWaypoint->data.moveThroughPoint.required_distance;
		break;

		case AI2cWaypointType_MoveNearFlag:
			param = inWaypoint->data.moveNearFlag.required_distance;
		break;

		case AI2cWaypointType_Scan:
			param = inWaypoint->data.scan.angle;
		break;

		case AI2cWaypointType_Guard:
			param = inWaypoint->data.guard.max_angle;
		break;

		case AI2cWaypointType_ShootAtFlag:
			param = inWaypoint->data.shootAtFlag.additional_inaccuracy;
		break;

		default:
			param = 0.f;
		break;
	}

	return param;
}

static void AIrWaypoint_SetFloatParam(AI2tWaypoint *inWaypoint, float inParam)
{
	switch(inWaypoint->type)
	{
		case AI2cWaypointType_MoveThroughFlag:
			inWaypoint->data.moveThroughFlag.required_distance = inParam;
		break;

		case AI2cWaypointType_MoveThroughPoint:
			inWaypoint->data.moveThroughPoint.required_distance = inParam;
		break;

		case AI2cWaypointType_MoveNearFlag:
			inWaypoint->data.moveNearFlag.required_distance = inParam;
		break;

		case AI2cWaypointType_Scan:
			inWaypoint->data.scan.angle = inParam;
		break;

		case AI2cWaypointType_Guard:
			inWaypoint->data.guard.max_angle = inParam;
		break;

		case AI2cWaypointType_ShootAtFlag:
			inWaypoint->data.shootAtFlag.additional_inaccuracy = inParam;
		break;

		default:
		break;
	}

	return;
}


static UUtBool AIrWaypoint_HasFloatParam(AI2tWaypoint *inWaypoint)
{
	UUtBool has_param;

	switch(inWaypoint->type)
	{
		case AI2cWaypointType_MoveThroughFlag:
		case AI2cWaypointType_MoveThroughPoint:
		case AI2cWaypointType_MoveNearFlag:
		case AI2cWaypointType_Scan:
		case AI2cWaypointType_Guard:
		case AI2cWaypointType_ShootAtFlag:
			has_param = UUcTrue;
		break;

		default:
			has_param = UUcFalse;
		break;
	}

	return has_param;
}

static void
OWiPathPoint_SetupWaypointMenu(
	WMtPopupMenu			*inMenu,
	AI2tWaypointType		inType)
{
	UUtUns16 itr;

	WMrPopupMenu_Reset(inMenu);

	for(itr = 0; cWaypointTypes[itr].name != NULL; itr++)
	{
		if (UUmString_IsEqual(cWaypointTypes[itr].name, "")) {
			WMrPopupMenu_AppendItem_Divider(inMenu);
		} else {
			WMrPopupMenu_AppendItem_Light(inMenu, cWaypointTypes[itr].id, cWaypointTypes[itr].name);
		}
	}

	WMrPopupMenu_SetSelection(inMenu, inType);
}

static UUtBool
OWiPathPoint_Edit_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	enum
	{
		cPathPointEdit_Type_PopupText = 100,
		cPathPointEdit_Type_Popup = 101,
		cPathPointEdit_Flag_Text= 102,
		cPathPointEdit_Flag_EditText = 103,
		cPathPointEdit_Flag_Button = 104,

		cPathPointEdit_IntParam_Text= 105,
		cPathPointEdit_IntParam_EditText = 106,

		cPathPointEdit_FloatParam_Text= 107,
		cPathPointEdit_FloatParam_EditText = 108,

		cPathPointEdit_Mode_Text= 109,
		cPathPointEdit_Mode_Popup = 110,

		cPathPointEdit_Direction_Text= 111,
		cPathPointEdit_Direction_Popup = 112
	};

	UUtBool handled = UUcTrue;
	WMtPopupMenu *type_popup_menu = WMrDialog_GetItemByID(inDialog, cPathPointEdit_Type_Popup);
	AI2tWaypoint *waypoint = (AI2tWaypoint *) WMrDialog_GetUserData(inDialog);

	WMtEditField	*flag_edittext = WMrDialog_GetItemByID(inDialog, cPathPointEdit_Flag_EditText);
	WMtWindow		*flag_text = WMrDialog_GetItemByID(inDialog, cPathPointEdit_Flag_Text);
	WMtWindow		*flag_button = WMrDialog_GetItemByID(inDialog, cPathPointEdit_Flag_Button);

	WMtWindow		*intparam_text = WMrDialog_GetItemByID(inDialog, cPathPointEdit_IntParam_Text);
	WMtWindow		*intparam_edittext = WMrDialog_GetItemByID(inDialog, cPathPointEdit_IntParam_EditText);

	WMtWindow		*floatparam_text = WMrDialog_GetItemByID(inDialog, cPathPointEdit_FloatParam_Text);
	WMtWindow		*floatparam_edittext = WMrDialog_GetItemByID(inDialog, cPathPointEdit_FloatParam_EditText);

	WMtWindow		*mode_text= WMrDialog_GetItemByID(inDialog, cPathPointEdit_Mode_Text);
	WMtPopupMenu	*mode_popup = WMrDialog_GetItemByID(inDialog, cPathPointEdit_Mode_Popup);

	WMtWindow		*direction_text= WMrDialog_GetItemByID(inDialog, cPathPointEdit_Direction_Text);
	WMtPopupMenu	*direction_popup = WMrDialog_GetItemByID(inDialog, cPathPointEdit_Direction_Popup);

	UUtInt16 flag_id = AIrWaypoint_GetFlag(waypoint);
	UUtBool set_flag_id = UUcFalse;
	UUtBool waypoint_type_changed = UUcFalse;

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// cWaypointTypeNames
			{
				UUtUns16 popup_append_itr;

				for(popup_append_itr = 0; popup_append_itr < AI2cMovementMode_Max; popup_append_itr++)
				{
					WMrPopupMenu_AppendItem_Light(mode_popup, popup_append_itr, cMovementModes[popup_append_itr]);
				}

				for(popup_append_itr = 0; popup_append_itr < AI2cMovementDirection_Max; popup_append_itr++)
				{
					WMrPopupMenu_AppendItem_Light(direction_popup, popup_append_itr, cMovementDirections[popup_append_itr]);
				}
			}

			set_flag_id = UUcTrue;
			waypoint_type_changed = UUcTrue;
			if (OWgWaypointHasDefaultFlag) {
				AIrWaypoint_SetFlag(waypoint, OWgWaypointDefaultFlagID);
				flag_id = OWgWaypointDefaultFlagID;
			}
		break;

		case WMcMessage_Destroy:
		break;

		case WMcMessage_MenuCommand:
			switch (WMrWindow_GetID((WMtMenu *) inParam2))
			{
				case cPathPointEdit_Type_Popup:
					{
						UUtUns16 new_type_menu_item_id;

						WMrPopupMenu_GetItemID(type_popup_menu, -1, &new_type_menu_item_id);

						if (new_type_menu_item_id != waypoint->type) {
							UUtInt16 old_flag = 0;
							UUtInt32 old_intparam = 0;
							float old_floatparam = 0.f;

							if (AIrWaypoint_HasFlag(waypoint)) {
								old_flag = AIrWaypoint_GetFlag(waypoint);
							} else if (OWgWaypointHasDefaultFlag) {
								old_flag = OWgWaypointDefaultFlagID;
								flag_id = OWgWaypointDefaultFlagID;
							}
							set_flag_id = UUcTrue;

							if (AIrWaypoint_HasIntParam(waypoint)) {
								old_intparam = AIrWaypoint_GetIntParam(waypoint);
							}

							if (AIrWaypoint_HasFloatParam(waypoint)) {
								old_floatparam = AIrWaypoint_GetFloatParam(waypoint);
							}

							UUrMemory_Clear(waypoint, sizeof(*waypoint));
							waypoint->type = (AI2tWaypointType)new_type_menu_item_id;
							waypoint_type_changed = UUcTrue;

							if (AIrWaypoint_HasFlag(waypoint)) {
								AIrWaypoint_SetFlag(waypoint, old_flag);
							}

							if (AIrWaypoint_HasIntParam(waypoint)) {
								 AIrWaypoint_SetIntParam(waypoint, old_intparam);
							}

							if (AIrWaypoint_HasFloatParam(waypoint)) {
								AIrWaypoint_SetFloatParam(waypoint, old_floatparam);
							}
						}
					}
				break;

				case cPathPointEdit_Mode_Popup:
					if (AI2cWaypointType_MovementMode == waypoint->type) {
						UUtUns16 new_mode_menu_item_id;

						WMrPopupMenu_GetItemID(mode_popup, -1, &new_mode_menu_item_id);

						waypoint->data.movementMode.newMode = new_mode_menu_item_id;
					}
				break;

				case cPathPointEdit_Direction_Popup:
					if (AI2cWaypointType_LookInDirection == waypoint->type) {
						UUtUns16 new_direction_menu_item_id;

						WMrPopupMenu_GetItemID(direction_popup, -1, &new_direction_menu_item_id);

						waypoint->data.movementMode.newMode = new_direction_menu_item_id;
					}
				break;
			}
		break;

		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{

				case WMcDialogItem_OK:
					WMrDialog_ModalEnd(inDialog, UUcTrue);
				break;

				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, UUcFalse);
				break;

				case cPathPointEdit_Flag_Button:
					flag_id = OWrChooseFlag(flag_id);

					set_flag_id = UUcTrue;
				break;

				case cPathPointEdit_Flag_EditText:
					{
						UUtInt32 flag_edittext_number = WMrEditField_GetInt32(flag_edittext);

						AIrWaypoint_SetFlag(waypoint, (UUtInt16) flag_edittext_number);
					}
				break;

				case cPathPointEdit_IntParam_EditText:
					{
						UUtInt32 intparam_edittext_number = WMrEditField_GetInt32(intparam_edittext);

						AIrWaypoint_SetIntParam(waypoint, intparam_edittext_number);
					}
				break;

				case cPathPointEdit_FloatParam_EditText:
					{
						float floatparam_edittext_number = WMrEditField_GetFloat(floatparam_edittext);

						AIrWaypoint_SetFloatParam(waypoint, floatparam_edittext_number);
					}
				break;
			}
		break;

		default:
			handled = UUcFalse;
		break;
	}

	if (set_flag_id) {
		char flag_text_set_buffer[128];

		sprintf(flag_text_set_buffer, "%d", flag_id);
		WMrMessage_Send(flag_edittext, EFcMessage_SetText, (UUtUns32) flag_text_set_buffer, 0);

		AIrWaypoint_SetFlag(waypoint, flag_id);
	}

	if (waypoint_type_changed) {
		UUtBool flag_field_visibility = AIrWaypoint_HasFlag(waypoint);
		UUtBool intparam_field_visibility = AIrWaypoint_HasIntParam(waypoint);
		UUtBool floatparam_field_visibility = AIrWaypoint_HasFloatParam(waypoint);
		UUtBool mode_field_visibility = AI2cWaypointType_MovementMode == waypoint->type;
		UUtBool direction_field_visibility = AI2cWaypointType_LookInDirection == waypoint->type;

		OWiPathPoint_SetupWaypointMenu(type_popup_menu, waypoint->type);

		WMrWindow_SetVisible(flag_edittext, flag_field_visibility);
		WMrWindow_SetVisible(flag_text, flag_field_visibility);
		WMrWindow_SetVisible(flag_button, flag_field_visibility);

		WMrWindow_SetVisible(intparam_text, intparam_field_visibility);
		WMrWindow_SetVisible(intparam_edittext, intparam_field_visibility);

		WMrWindow_SetVisible(floatparam_text, floatparam_field_visibility);
		WMrWindow_SetVisible(floatparam_edittext, floatparam_field_visibility);

		WMrWindow_SetVisible(mode_text, mode_field_visibility);
		WMrWindow_SetVisible(mode_popup, mode_field_visibility);

		WMrWindow_SetVisible(direction_text, direction_field_visibility);
		WMrWindow_SetVisible(direction_popup, direction_field_visibility);

		if (AI2cWaypointType_MovementMode == waypoint->type) {
			WMrPopupMenu_SetSelection(mode_popup, (UUtUns16) waypoint->data.movementMode.newMode);
		}

		if (AI2cWaypointType_LookInDirection == waypoint->type) {
			WMrPopupMenu_SetSelection(direction_popup, (UUtUns16) waypoint->data.lookInDirection.facing);
		}

		if (AIrWaypoint_HasIntParam(waypoint)) {
			WMrWindow_SetTitle(intparam_text, AIrWaypoint_GetIntParamName(waypoint), WMcMaxTitleLength);
			WMrEditField_SetInt32(intparam_edittext, AIrWaypoint_GetIntParam(waypoint));
		}

		if (AIrWaypoint_HasFloatParam(waypoint)) {
			WMrWindow_SetTitle(floatparam_text, AIrWaypoint_GetFloatParamName(waypoint), WMcMaxTitleLength);
			WMrEditField_SetFloat(floatparam_edittext, AIrWaypoint_GetFloatParam(waypoint));
		}

		if (AIrWaypoint_HasFlag(waypoint)) {
			WMrEditField_SetInt32(flag_edittext, AIrWaypoint_GetFlag(waypoint));
		}
	}

	return handled;
}

static void OWrPath_BuildListBox(
	WMtWindow *inWindow,
	OBJtObject *inObject)
{
	UUtUns32 itr;
	OBJtOSD_All object_specific_data;
	OBJtOSD_PatrolPath *path;

	WMrMessage_Send(inWindow, LBcMessage_Reset, 0, 0);

	OBJrObject_GetObjectSpecificData(inObject, &object_specific_data);
	path = &object_specific_data.osd.patrolpath_osd;

	for(itr = 0; itr < path->num_waypoints; itr++)
	{
		AI2tWaypoint *waypoint = path->waypoints + itr;
		AI2tWaypointData *data = &waypoint->data;
		char buffer[256];

		switch(waypoint->type)
		{
			case AI2cWaypointType_MoveToFlag:
				sprintf(buffer, "#%d Stop At Flag %d", itr + 1, data->moveToFlag.flag);
			break;

			case AI2cWaypointType_Stop:
				sprintf(buffer, "#%d Exit Patrol", itr + 1);
			break;

			case AI2cWaypointType_Pause:
				sprintf(buffer, "#%d Pause for %d frames", itr + 1, data->pause.count);
			break;

			case AI2cWaypointType_LookAtFlag:
				sprintf(buffer, "#%d Look At Flag %d", itr + 1, data->lookAtFlag.lookFlag);
			break;

			case AI2cWaypointType_MoveAndFaceFlag:
				sprintf(buffer, "#%d Stop And Orient To Flag %d", itr + 1, data->moveAndFaceFlag.faceFlag);
			break;

			case AI2cWaypointType_Loop:
				sprintf(buffer, "#%d Loop", itr + 1);
			break;

			case AI2cWaypointType_MovementMode:
				sprintf(buffer, "#%d Movement Mode %s", itr + 1, cMovementModes[data->movementMode.newMode]);
			break;

			case AI2cWaypointType_LookInDirection:
				sprintf(buffer, "#%d Lock Facing %s", itr + 1, cMovementDirections[data->lookInDirection.facing]);
			break;

			case AI2cWaypointType_MoveThroughFlag:
				sprintf(buffer, "#%d Move To Flag %d (distance %f)", itr + 1, data->moveThroughFlag.flag, data->moveThroughFlag.required_distance);
			break;

			case AI2cWaypointType_StopLooking:
				sprintf(buffer, "#%d Stop Looking", itr + 1);
			break;

			case AI2cWaypointType_FreeFacing:
				sprintf(buffer, "#%d Free Facing", itr + 1);
			break;

			case AI2cWaypointType_GlanceAtFlag:
				sprintf(buffer, "#%d Glance At Flag %d for %d frames", itr + 1, data->glanceAtFlag.glanceFlag, data->glanceAtFlag.glanceFrames);
			break;

			case AI2cWaypointType_MoveNearFlag:
				sprintf(buffer, "#%d Stop Near Flag %d (distance %f)", itr + 1, data->moveNearFlag.flag, data->moveNearFlag.required_distance);
			break;

			case AI2cWaypointType_LoopToWaypoint:
				sprintf(buffer, "#%d Loop To Waypoint #%d", itr + 1, data->loopToWaypoint.waypoint_index + 1);
			break;

			case AI2cWaypointType_Scan:
				if (data->scan.scan_frames == 0) {
					sprintf(buffer, "#%d Scan forever (angle %f)", itr + 1, data->scan.angle);
				} else {
					sprintf(buffer, "#%d Scan for %d frames (angle %f)", itr + 1, data->scan.scan_frames, data->scan.angle);
				}
			break;

			case AI2cWaypointType_StopScanning:
				sprintf(buffer, "#%d Stop Scanning", itr + 1);
			break;

			case AI2cWaypointType_Guard:
				if (data->guard.guard_frames == 0) {
					sprintf(buffer, "#%d Guard Flag %d forever (angle %f)", itr + 1, data->guard.flag, data->guard.max_angle);
				} else {
					sprintf(buffer, "#%d Guard Flag %d for %d frames (angle %f)", itr + 1, data->guard.flag, data->guard.guard_frames, data->guard.max_angle);
				}
			break;

			case AI2cWaypointType_TriggerScript:
				sprintf(buffer, "#%d Trigger Script patrolscript%04d", itr + 1, waypoint->data.triggerScript.script_number);
			break;

			case AI2cWaypointType_PlayScript:
				sprintf(buffer, "#%d Play Script patrolscript%04d", itr + 1, waypoint->data.playScript.script_number);
			break;

			case AI2cWaypointType_LockIntoPatrol:
				sprintf(buffer, "#%d Lock Into Patrol = %s", itr + 1, waypoint->data.lockIntoPatrol.lock ? "yes" : "no");
			break;

			case AI2cWaypointType_ShootAtFlag:
				sprintf(buffer, "#%d Shoot At Flag %d for %d frames (inaccuracy %f)", itr + 1, data->shootAtFlag.flag, data->shootAtFlag.shoot_frames,
						data->shootAtFlag.additional_inaccuracy);
			break;

			default:
				sprintf(buffer, "#%d Unknown Waypoint (type %d)", itr + 1, waypoint->type);
			break;
		}

		WMrMessage_Send(inWindow, LBcMessage_AddString, (UUtUns32) buffer, 0);
		WMrMessage_Send(inWindow, LBcMessage_SetItemData, itr, itr);

	}

	return;
}

UUtBool
OWrPath_Edit_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	enum
	{
		cPathEdit_ListBox = 100,
		cPathEdit_Insert = 101,
		cPathEdit_Delete = 102,
		cPathEdit_Edit = 103,
		cPathEdit_Up = 104,
		cPathEdit_Down = 105,
		cPathEdit_Append = 106,
		cPathEdit_ID_Prompt = 107,
		cPathEdit_ID_Edit = 108,
		cPathEdit_Name_Prompt = 109,
		cPathEdit_Name_Edit = 110,
		cPathEdit_CB_ReturnToNearest = 120
	};

	UUtBool handled = UUcTrue;
	WMtWindow *listbox = WMrDialog_GetItemByID(inDialog, cPathEdit_ListBox);
	OBJtObject *object = (OBJtObject *) WMrDialog_GetUserData(inDialog);
	OBJtOSD_All object_specific_data;
	OBJtOSD_PatrolPath *path;
	UUtUns32 selected_index = LBcError;
	AI2tWaypoint *waypoints;
	AI2tWaypoint *selected_waypoint = NULL;

	UUtBool edit_selected_waypoint = UUcFalse;
	UUtBool finish_dialog_accept_changes = UUcFalse;
	UUtBool object_name_changed = UUcFalse;
	UUtBool dirty = UUcFalse;

	WMtEditField	*id_edit = WMrDialog_GetItemByID(inDialog, cPathEdit_ID_Edit);
	WMtEditField	*name_edit = WMrDialog_GetItemByID(inDialog, cPathEdit_Name_Edit);
	WMtCheckBox		*cb_nearest = WMrDialog_GetItemByID(inDialog, cPathEdit_CB_ReturnToNearest);

	if (listbox != NULL) {
		selected_index = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
	}

	OBJrObject_GetObjectSpecificData(object, &object_specific_data);
	path = &object_specific_data.osd.patrolpath_osd;
	waypoints = path->waypoints;

	if (LBcError != selected_index) {
		selected_waypoint = waypoints + selected_index;
	}

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			{
				WMrEditField_SetInt32(id_edit, object_specific_data.osd.patrolpath_osd.id_number);
				WMrEditField_SetText(name_edit, object_specific_data.osd.patrolpath_osd.name);
				WMrCheckBox_SetCheck(cb_nearest, ((object_specific_data.osd.patrolpath_osd.flags & AI2cPatrolPathFlag_ReturnToNearest) > 0));

				dirty = UUcTrue;
				object_name_changed = UUcTrue;
			}
		break;

		case WMcMessage_Destroy:
		break;

		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case cPathEdit_ListBox:
					if (WMcNotify_DoubleClick == UUmHighWord(inParam1)) {
						edit_selected_waypoint = UUcTrue;
					}
				break;

				case cPathEdit_Insert:
					if (path->num_waypoints >= AI2cMax_WayPoints) {
						WMrDialog_MessageBox(NULL, "NO", "Maximum # of path points has been reached.", WMcMessageBoxStyle_OK);
					}
					else {
						if (selected_index != LBcError) {
							UUrMemory_MoveOverlap(
								waypoints + selected_index + 0,
								waypoints + selected_index + 1,
								sizeof(AI2tWaypoint) * (path->num_waypoints - selected_index));
						}
						else {
							selected_index = path->num_waypoints;
						}

						path->num_waypoints += 1;
						selected_waypoint = path->waypoints + selected_index;

						UUrMemory_Clear(selected_waypoint, sizeof(*selected_waypoint));

						edit_selected_waypoint = UUcTrue;
					}
				break;

				case cPathEdit_Append:
					if (path->num_waypoints >= AI2cMax_WayPoints) {
						WMrDialog_MessageBox(NULL, "NO", "Maximum # of path points has been reached.", WMcMessageBoxStyle_OK);
					}
					else {
						if (selected_index != LBcError) {
							UUrMemory_MoveOverlap(
								waypoints + selected_index + 1,
								waypoints + selected_index + 2,
								sizeof(AI2tWaypoint) * (path->num_waypoints - selected_index - 1));

							selected_index += 1;
						}
						else {
							selected_index = path->num_waypoints;
						}

						path->num_waypoints += 1;
						selected_waypoint = path->waypoints + selected_index;

						UUrMemory_Clear(selected_waypoint, sizeof(*selected_waypoint));

						edit_selected_waypoint = UUcTrue;
					}
				break;

				case cPathEdit_Delete:
					if (selected_index != LBcError) {
						UUrMemory_MoveOverlap(
							waypoints + selected_index + 1,
							waypoints + selected_index,
							sizeof(AI2tWaypoint) * (path->num_waypoints - selected_index - 1));

						path->num_waypoints -= 1;

						dirty = UUcTrue;
					}
				break;

				case cPathEdit_Edit:
					edit_selected_waypoint = UUcTrue;
				break;

				case cPathEdit_CB_ReturnToNearest:
					if (WMrCheckBox_GetCheck(cb_nearest)) {
						object_specific_data.osd.patrolpath_osd.flags |= AI2cPatrolPathFlag_ReturnToNearest;
					} else {
						object_specific_data.osd.patrolpath_osd.flags &= ~AI2cPatrolPathFlag_ReturnToNearest;
					}
					dirty = UUcTrue;
				break;

				case cPathEdit_Up:
					if ((path->num_waypoints > 1) && (selected_index != LBcError) && (selected_index > 0)) {
						AI2tWaypoint swap_up;

						swap_up = waypoints[selected_index - 1];
						waypoints[selected_index - 1] = waypoints[selected_index];
						waypoints[selected_index] = swap_up;

						selected_index -= 1;
						dirty = UUcTrue;
					}
				break;

				case cPathEdit_Down:
					if ((path->num_waypoints > 1) && (selected_index != LBcError) && ((selected_index + 1) < path->num_waypoints)) {
						AI2tWaypoint swap_down;

						swap_down = waypoints[selected_index + 1];
						waypoints[selected_index + 1] = waypoints[selected_index];
						waypoints[selected_index] = swap_down;

						selected_index += 1;
						dirty = UUcTrue;
					}
				break;

				case WMcDialogItem_OK:
					finish_dialog_accept_changes = UUcTrue;
					OWrAI_FindCurrentPath();
				break;

				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, UUcFalse);
				break;

				case cPathEdit_ID_Edit:
					object_specific_data.osd.patrolpath_osd.id_number = (UUtUns16) WMrEditField_GetInt32(id_edit);
					object_name_changed = UUcTrue;
				break;

				case cPathEdit_Name_Edit:
					WMrEditField_GetText(name_edit, object_specific_data.osd.patrolpath_osd.name, SLcScript_MaxNameLength);
					object_name_changed = UUcTrue;
				break;

			}
		break;

		default:
			handled = UUcFalse;
		break;
	}

	UUrMemory_Block_VerifyList();

	if (edit_selected_waypoint) {
		if (NULL != selected_waypoint) {
			UUtUns32 out_message;

			OWgWaypointHasDefaultFlag = UUcFalse;
			WMrDialog_ModalBegin(OWcDialog_AI_EditPathPoint, NULL, OWiPathPoint_Edit_Callback, (UUtUns32) selected_waypoint, &out_message);

			dirty = out_message ? UUcTrue : UUcFalse;
		}
	}

	if (object_name_changed) {
		char title_buffer[WMcMaxTitleLength];

		sprintf(title_buffer, "Edit Path %s", object_specific_data.osd.patrolpath_osd.name);

		WMrWindow_SetTitle(inDialog, title_buffer, WMcMaxTitleLength);

		dirty = UUcTrue;
	}

	if (dirty) {
		OWrObjectProperties_SetOSD(inDialog, object, &object_specific_data);
		OWrPath_BuildListBox(listbox, object);

		if (selected_index < path->num_waypoints) {
			WMrMessage_Send(listbox, LBcMessage_SetSelection, UUcFalse, selected_index);
		}
		else if (path->num_waypoints > 0) {
			WMrMessage_Send(listbox, LBcMessage_SetSelection, UUcFalse, path->num_waypoints - 1);
		}
	}

	if (finish_dialog_accept_changes) {
		// verify that our name & id are unique

		WMrDialog_ModalEnd(inDialog, UUcTrue);
	}

	return handled;
}

UUtBool
OWrAI_DropAndAddFlag(
	void)
{
	UUtBool dropped_flag;
	UUtUns16 flag_id;
	UUtUns32 dialog_msg;
	ONtCharacter *player_char;
	AI2tWaypoint *waypoint;

	if (!OWgCurrentPatrolPathValid) {
		COrConsole_Printf("### no currently selected path to add waypoint to!");
		return UUcFalse;
	}

	UUmAssertReadPtr(OWgCurrentPatrolPath, sizeof(AI2tPatrolPath));
	if (OWgCurrentPatrolPath->num_waypoints >= AI2cMax_WayPoints) {
		COrConsole_Printf("### path already has max waypoints (%d)", AI2cMax_WayPoints);
		return UUcFalse;
	}

	// drop a flag in case we need one
	player_char = ONrGameState_GetPlayerCharacter();
	if (!player_char) {
		COrConsole_Printf("### couldn't get player character to drop flag");
		return UUcFalse;
	}

	dropped_flag = ONrGameState_DropFlag(&player_char->location, player_char->desiredFacing,
										UUcFalse, &flag_id);
	if (!dropped_flag) {
		COrConsole_Printf("### could not drop flag (.bin file locked?)");
		return UUcFalse;
	}

	// set up space for a new waypoint in the patrol path
	waypoint = OWgCurrentPatrolPath->waypoints + OWgCurrentPatrolPath->num_waypoints;
	UUrMemory_Clear(waypoint, sizeof(AI2tWaypoint));

	// edit this waypoint
	OWgWaypointHasDefaultFlag = UUcTrue;
	OWgWaypointDefaultFlagID = flag_id;
	WMrDialog_ModalBegin(OWcDialog_AI_EditPathPoint, NULL, OWiPathPoint_Edit_Callback, (UUtUns32) waypoint, &dialog_msg);

	if ((!dialog_msg) || (!AIrWaypoint_HasFlag(waypoint)) || (AIrWaypoint_GetFlag(waypoint) != flag_id)) {
		OBJtOSD_All					osd_all;
		OBJtObject					*object;

		// we aren't using the newly created flag... delete it.
		osd_all.osd.flag_osd.id_number = flag_id;
		object = OBJrObjectType_Search(OBJcType_Flag, OBJcSearch_FlagID, &osd_all);

		if (object == NULL) {
			COrConsole_Printf("### warning: couldn't find newly dropped flag to delete it (it is unused)!");
		} else {
			OBJrObject_Delete(object);
		}
	}

	if (!dialog_msg) {
		// user cancelled out of dialog
		return UUcFalse;
	}

	OWgCurrentPatrolPath->num_waypoints++;
	COrConsole_Printf("added waypoint #%d to path %s", OWgCurrentPatrolPath->num_waypoints,
						OWgCurrentPatrolPath->name);
	return UUcTrue;
}
#endif
