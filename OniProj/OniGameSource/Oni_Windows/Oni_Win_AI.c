#if TOOL_VERSION
/*
	FILE:	Oni_Win_AI.c
	
	AUTHOR:	Michael Evans
	
	CREATED: April 11, 2000
	
	PURPOSE: AI Windows
	
	Copyright (c) Bungie Software 2000

*/

#include "BFW.h"
#include "Oni_Win_AI.h"
#include "Oni_Windows.h"
#include "Oni_Object.h"
#include "Oni_AI2.h"
#include "Oni_Win_Tools.h"

#include "WM_DrawContext.h"
#include "WM_PartSpecification.h"
#include "WM_Scrollbar.h"
#include "WM_MenuBar.h"
#include "WM_Menu.h"
#include "WM_PopupMenu.h"
#include "WM_EditField.h"

// ------------------------------
// AI Menu
// ------------------------------

static UUtBool
OWiChooseFlag_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	enum
	{
		cChooseFlag_Listbox = 100
	};

	UUtBool handled = UUcTrue;
	WMtWindow *listbox = WMrDialog_GetItemByID(inDialog, cChooseFlag_Listbox);
	UUtBool finish_dialog = UUcFalse;

	UUtUns32 selected_index = LBcError;
	OBJtObject *selected_object = NULL;
	OBJtOSD_All selected_object_specific_data;

	if (listbox != NULL) {
		selected_index = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
	}

	if (LBcError != selected_index) {
		selected_object	= (OBJtObject *) WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
		OBJrObject_GetObjectSpecificData(selected_object, &selected_object_specific_data);
	}

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OBJrObjectType_BuildListBox(OBJcType_Flag, listbox, UUcFalse);

			{
				UUtUns32 initial_flag_id = WMrDialog_GetUserData(inDialog);
				UUtUns32 scan_itr;
				UUtUns32 scan_count = WMrMessage_Send(listbox, LBcMessage_GetNumLines, 0, 0);

				for(scan_itr = 0; scan_itr < scan_count; scan_itr++)
				{
					selected_object = (OBJtObject *) WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, scan_itr);

					OBJrObject_GetObjectSpecificData(selected_object, &selected_object_specific_data);

					if (initial_flag_id == selected_object_specific_data.osd.flag_osd.id_number) {
						WMrMessage_Send(listbox, LBcMessage_SetSelection, UUcFalse, scan_itr);
						break;
					}
				}
			}

			// ok, scan and select the correct current flag
		break;
		
		case WMcMessage_Destroy:
		break;

		case WMcMessage_MenuCommand:
			switch (WMrWindow_GetID((WMtMenu *) inParam2))
			{
			}
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_OK:
				case WMcDialogItem_Cancel:
					finish_dialog = UUcTrue;
				break;


				case cChooseFlag_Listbox:
					if (WMcNotify_DoubleClick == UUmHighWord(inParam1)) {
						finish_dialog = UUcTrue;
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}

	UUrMemory_Block_VerifyList();

	if (finish_dialog) {
		UUtUns32 result_flag_id = WMrDialog_GetUserData(inDialog);

		if (selected_object) {
			result_flag_id = selected_object_specific_data.osd.flag_osd.id_number;
		}

		WMrDialog_ModalEnd(inDialog, result_flag_id);
	}
	
	return handled;
}


static UUtBool
OWiChoosePath_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	enum
	{
		cChoosePath_Listbox = 100
	};

	UUtBool handled = UUcTrue;
	WMtWindow *listbox = WMrDialog_GetItemByID(inDialog, cChoosePath_Listbox);
	UUtBool finish_dialog = UUcFalse;
	UUtBool cancel_dialog = UUcFalse;

	UUtUns32 selected_index = LBcError;
	OBJtObject *selected_object = NULL;
	OBJtOSD_All selected_object_specific_data;

	if (listbox != NULL) {
		selected_index = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
	}

	if (LBcError != selected_index) {
		selected_object	= (OBJtObject *) WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
		OBJrObject_GetObjectSpecificData(selected_object, &selected_object_specific_data);
	}

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OBJrObjectType_BuildListBox(OBJcType_PatrolPath, listbox, UUcFalse);
			{
				UUtUns32 initial_path_id = WMrDialog_GetUserData(inDialog);
				UUtUns32 scan_itr;
				UUtUns32 scan_count = WMrMessage_Send(listbox, LBcMessage_GetNumLines, 0, 0);

				for(scan_itr = 0; scan_itr < scan_count; scan_itr++)
				{
					selected_object = (OBJtObject *) WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, scan_itr);

					OBJrObject_GetObjectSpecificData(selected_object, &selected_object_specific_data);

					if (initial_path_id == selected_object_specific_data.osd.patrolpath_osd.id_number) {
						WMrMessage_Send(listbox, LBcMessage_SetSelection, UUcFalse, scan_itr);
						break;
					}
				}
			}

			// ok, scan and select the correct current flag
		break;
		
		case WMcMessage_Destroy:
		break;

		case WMcMessage_MenuCommand:
			switch (WMrWindow_GetID((WMtMenu *) inParam2))
			{
			}
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_OK:
					finish_dialog = UUcTrue;
				break;

				case WMcDialogItem_Cancel:
					cancel_dialog = UUcTrue;
				break;


				case cChoosePath_Listbox:
					if (WMcNotify_DoubleClick == UUmHighWord(inParam1)) {
						finish_dialog = UUcTrue;
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}

	UUrMemory_Block_VerifyList();

	if (finish_dialog || cancel_dialog) {
		UUtUns32 result_path_id = WMrDialog_GetUserData(inDialog);

		if (selected_object && !cancel_dialog) {
			result_path_id = selected_object_specific_data.osd.patrolpath_osd.id_number;
		}

		WMrDialog_ModalEnd(inDialog, result_path_id);
	}
	
	return handled;
}

UUtInt16 OWrChooseFlag(UUtInt16 inFlagID)
{
	OBJtObject *object = NULL;
	OBJtOSD_All	object_specific_data;
	UUtInt16 result = 0;

	object_specific_data.osd.flag_osd.id_number = inFlagID;
	object = OBJrObjectType_Search(OBJcType_Flag, OBJcSearch_FlagID, &object_specific_data);
	
	object = OWrSelectObject(OBJcType_Flag, object, UUcFalse, UUcTrue);

	if (NULL != object) {
		OBJrObject_GetObjectSpecificData(object, &object_specific_data);
		result = object_specific_data.osd.flag_osd.id_number;
	}

	return result;
}

UUtUns16 OWrChoosePath(UUtUns16	inPathID)
{
	OBJtObject *object = NULL;
	OBJtOSD_All	object_specific_data;
	UUtUns16 result = 0;

	object_specific_data.osd.patrolpath_osd.id_number = inPathID;
	object = OBJrObjectType_Search(OBJcType_PatrolPath, OBJcSearch_PatrolPathID, &object_specific_data);
	
	object = OWrSelectObject(OBJcType_PatrolPath, object, UUcFalse, UUcFalse);

	if (NULL != object) {
		OBJrObject_GetObjectSpecificData(object, &object_specific_data);
		result = object_specific_data.osd.patrolpath_osd.id_number;
	}

	return result;
}

static UUtBool OWiIsPathNameValid(WMtDialog *inDialog, char *inString)
{
	UUtBool string_is_valid;
	OBJtOSD_All search_param;
	OBJtObject *found_object;

	UUrString_Copy(search_param.osd.patrolpath_osd.name, inString, SLcScript_MaxNameLength);
	found_object = OBJrObjectType_Search(OBJcType_PatrolPath, OBJcSearch_PatrolPathName, &search_param);

	string_is_valid = (NULL == found_object);

	if (!string_is_valid) {
		WMrDialog_MessageBox(NULL, "NO", "Sorry that path name is already in use", WMcMessageBoxStyle_OK);
	}

	return string_is_valid;
}	

enum
{
	OWcMenu_AI_EditPaths			= 100,
	OWcMenu_AI_EditSpawns			= 101,
	OWcMenu_AI_EditMelee			= 102,
	OWcMenu_AI_EditTriggerVolume	= 103,
	OWcMenu_AI_EditLaserTrigger		= 104,
	OWcMenu_AI_EditTurret			= 105,
	OWcMenu_AI_EditCombat			= 106,
	OWcMenu_AI_ShowCurrentPath		= 107,
	OWcMenu_AI_EditNeutral			= 108,
	OWcMenu_AI_EditDoor				= 109,
	OWcMenu_AI_EditConsole			= 110
};

void OWrAI_Menu_HandleMenuCommand(WMtMenu *inMenu, UUtUns16 inMenuItemID)
{
	UUtUns16 flags;
	OBJtObject *object;
	AI2tPatrolPath *path_data;

	switch(inMenuItemID)
	{
		case OWcMenu_AI_EditPaths:
			object = OWrSelectObject(OBJcType_PatrolPath, NULL, UUcFalse, UUcFalse);
			if (object != NULL) {
				UUmAssert(object->object_type == OBJcType_PatrolPath);
				path_data = (AI2tPatrolPath *) object->object_data;
				OWgCurrentPatrolPathValid = UUcTrue;
				OWgCurrentPatrolPathID = path_data->id_number;
				OWgCurrentPatrolPath = path_data;
			}
		break;

		case OWcMenu_AI_EditSpawns:
			OWrSelectObject(OBJcType_Character, NULL, UUcFalse, UUcTrue);
		break;

		case OWcMenu_AI_EditMelee:
			OWrSelectObject(OBJcType_Melee, NULL, UUcFalse, UUcFalse);
		break;

		case OWcMenu_AI_EditCombat:
			OWrSelectObject(OBJcType_Combat, NULL, UUcFalse, UUcFalse);
		break;

		case OWcMenu_AI_EditNeutral:
			OWrSelectObject(OBJcType_Neutral, NULL, UUcFalse, UUcFalse);
		break;

		case OWcMenu_AI_EditTriggerVolume:
			OWrSelectObject(OBJcType_TriggerVolume, NULL, UUcFalse, UUcTrue);
		break;

		case OWcMenu_AI_EditLaserTrigger:
			OWrSelectObject(OBJcType_Trigger, NULL, UUcFalse, UUcTrue);
		break;

		case OWcMenu_AI_EditTurret:
			OWrSelectObject(OBJcType_Turret, NULL, UUcFalse, UUcTrue);
		break;

		case OWcMenu_AI_EditDoor:
			OWrSelectObject(OBJcType_Door, NULL, UUcFalse, UUcTrue);
		break;

		case OWcMenu_AI_EditConsole:
			OWrSelectObject(OBJcType_Console, NULL, UUcFalse, UUcTrue);
		break;

		case OWcMenu_AI_ShowCurrentPath:
			WMrMenu_GetItemFlags(inMenu, inMenuItemID, &flags);
			OWgShowPatrolPath = (flags & WMcMenuItemFlag_Checked) ? UUcFalse : UUcTrue;
			OWrAI_FindCurrentPath();
		break;

		default:
			WMrDialog_MessageBox(NULL, "error", "unknown menu item in OWrAI_Menu_HandleMenuCommand", WMcMessageBoxStyle_OK);
	}

	return;
}

void OWrAI_Menu_Init(WMtMenu *inMenu)
{
	WMrMenu_CheckItem(inMenu, OWcMenu_AI_ShowCurrentPath, OWgShowPatrolPath);
}

UUtError OWrAI_Initialize(void)
{
	OWgShowPatrolPath = UUcFalse;
	OWgCurrentPatrolPathValid = UUcFalse;
	OWgCurrentPatrolPathID = (UUtUns16) -1;
	OWgCurrentPatrolPath = NULL;

	return UUcError_None;
}

void OWrAI_FindCurrentPath(void)
{
	OBJtOSD_All					osd_all;
	OBJtObject					*object;
	
	// search for the currently selected patrol path
	osd_all.osd.patrolpath_osd.id_number = OWgCurrentPatrolPathID;
	
	// find the object
	object = OBJrObjectType_Search(OBJcType_PatrolPath, OBJcSearch_PatrolPathID, &osd_all);
	if (object == NULL) {
		OWgCurrentPatrolPathValid = UUcFalse;
		OWgCurrentPatrolPath = NULL;
	} else {
		OWgCurrentPatrolPathValid = UUcTrue;
		OWgCurrentPatrolPath = (AI2tPatrolPath *) object->object_data;
	}
}

UUtError OWrAI_LevelBegin(void)
{
	OWrAI_FindCurrentPath();

	return UUcError_None;
}

void OWrAI_Display(void)
{
	UUtUns32 itr;
	UUtBool has_flag, look_at_flag, first_waypoint;
	UUtUns16 flag_id;
	AI2tWaypoint *waypoint;
	M3tVector3D last_move_point, first_point;
	ONtFlag flag;

	if (!(OWgShowPatrolPath && OWgCurrentPatrolPathValid))
		return;

	first_waypoint = UUcTrue;
	MUmVector_Set(last_move_point, 0, 0, 0);

	for (itr = 0, waypoint = OWgCurrentPatrolPath->waypoints;
		itr < OWgCurrentPatrolPath->num_waypoints; itr++, waypoint++) {
		switch(waypoint->type) {
			case AI2cWaypointType_MoveToFlag:
				has_flag = UUcTrue;
				look_at_flag = UUcFalse;
				flag_id = waypoint->data.moveToFlag.flag;
				break;

			case AI2cWaypointType_Stop:
				has_flag = UUcFalse;
				break;

			case AI2cWaypointType_Pause:
				has_flag = UUcFalse;
				break;

			case AI2cWaypointType_LookAtFlag:
				has_flag = UUcTrue;
				look_at_flag = UUcTrue;
				flag_id = waypoint->data.lookAtFlag.lookFlag;
				break;

			case AI2cWaypointType_LookAtPoint:
				// not used
				has_flag = UUcFalse;
				break;

			case AI2cWaypointType_MoveAndFaceFlag:
				has_flag = UUcTrue;
				look_at_flag = UUcFalse;
				flag_id = waypoint->data.moveAndFaceFlag.faceFlag;
				break;

			case AI2cWaypointType_Loop:
				M3rGeom_Line_Light(&last_move_point, &first_point, IMcShade_Green);
				return;

			case AI2cWaypointType_MovementMode:
				// not used
				has_flag = UUcFalse;
				break;

			case AI2cWaypointType_MoveToPoint:
				// not used
				has_flag = UUcFalse;
				break;

			case AI2cWaypointType_LookInDirection:
				// not used
				has_flag = UUcFalse;
				break;

			case AI2cWaypointType_MoveThroughFlag:
				has_flag = UUcTrue;
				look_at_flag = UUcFalse;
				flag_id = waypoint->data.moveThroughFlag.flag;
				break;

			case AI2cWaypointType_MoveThroughPoint:
				// not used
				has_flag = UUcFalse;
				break;

			case AI2cWaypointType_StopLooking:
				// not used
				has_flag = UUcFalse;
				break;

			case AI2cWaypointType_FreeFacing:
				// not used
				has_flag = UUcFalse;
				break;

			case AI2cWaypointType_GlanceAtFlag:
				has_flag = UUcTrue;
				look_at_flag = UUcTrue;
				flag_id = waypoint->data.glanceAtFlag.glanceFlag;
				break;

			case AI2cWaypointType_LoopToWaypoint:
				// not used
				has_flag = UUcFalse;
				break;

			case AI2cWaypointType_Scan:
				// not used
				has_flag = UUcFalse;
				break;

			case AI2cWaypointType_Guard:
				has_flag = UUcTrue;
				look_at_flag = UUcFalse;
				flag_id = waypoint->data.guard.flag;
				break;

			default:
				has_flag = UUcFalse;
				break;
		}

		if (!has_flag)
			continue;

		if (!ONrLevel_Flag_ID_To_Flag(flag_id, &flag))
			continue;

		// don't draw lines embedded in floor
		flag.location.y += 1.0f;

		if (!first_waypoint) {
			M3rGeom_Line_Light(&last_move_point, &flag.location,
							look_at_flag ? IMcShade_Blue : IMcShade_Green);
		}

		if (!look_at_flag) {
			last_move_point = flag.location;
			if (first_waypoint) {
				first_point = flag.location;
				first_waypoint = UUcFalse;
			}
		}
	}
}

#endif