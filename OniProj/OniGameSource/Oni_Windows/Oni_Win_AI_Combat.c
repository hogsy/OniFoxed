#if TOOL_VERSION
/*
	FILE:	Oni_Win_Combat.c

	AUTHOR:	Michael Evans

	CREATED: May 5, 2000

	PURPOSE: AI Combat Windows

	Copyright (c) Bungie Software 2000

*/

#include "Oni_Win_AI_Combat.h"
#include "Oni_Win_Tools.h"
#include "Oni_Object.h"
#include "WM_Menu.h"
#include "WM_Text.h"
#include "WM_EditField.h"
#include "WM_PopupMenu.h"
#include "WM_CheckBox.h"
#include "Oni_AI2.h"
#include "Oni_AI2_Combat.h"


UUtBool
OWrEditCombat_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	enum
	{
		cEditCombat_Prompt_Base		= 100,
		cEditCombat_Menu_Base		= 200,
		cEditCombat_ID				= 301,
		cEditCombat_Name			= 303,
		cEditCombat_MedRange		= 400,
		cEditCombat_MeleeWhen		= 401,
		cEditCombat_NoGun			= 402,
		cEditCombat_ShortRange		= 403,
		cEditCombat_PursuitDist		= 404,
		cEditCombat_PanicGunfire	= 500,
		cEditCombat_PanicSight		= 501,
		cEditCombat_PanicHurt		= 502,
		cEditCombat_PanicMelee		= 503,
		cEditCombat_AlarmFindDist	= 600,
		cEditCombat_AlarmIgnoreDist = 601,
		cEditCombat_AlarmChaseDist	= 602,
		cEditCombat_AlarmKnockdown	= 603,
		cEditCombat_AlarmAttackDmg	= 604,
		cEditCombat_AlarmFightTimer	= 605
	};

	WMtPopupMenu *behavior_menu[AI2cCombatRange_NumStoredRanges];
	WMtText		 *behavior_prompt[AI2cCombatRange_NumStoredRanges];
	WMtEditField *id_edit				= WMrDialog_GetItemByID(inDialog, cEditCombat_ID);
	WMtEditField *name_edit				= WMrDialog_GetItemByID(inDialog, cEditCombat_Name);
	WMtEditField *range_edit			= WMrDialog_GetItemByID(inDialog, cEditCombat_MedRange);
	WMtEditField *shortrange_edit		= WMrDialog_GetItemByID(inDialog, cEditCombat_ShortRange);
	WMtEditField *pursuitdist_edit		= WMrDialog_GetItemByID(inDialog, cEditCombat_PursuitDist);
	WMtEditField *panicmelee_edit		= WMrDialog_GetItemByID(inDialog, cEditCombat_PanicMelee);
	WMtEditField *panicgunfire_edit		= WMrDialog_GetItemByID(inDialog, cEditCombat_PanicGunfire);
	WMtEditField *panicsight_edit		= WMrDialog_GetItemByID(inDialog, cEditCombat_PanicSight);
	WMtEditField *panichurt_edit		= WMrDialog_GetItemByID(inDialog, cEditCombat_PanicHurt);
	WMtPopupMenu *melee_menu			= WMrDialog_GetItemByID(inDialog, cEditCombat_MeleeWhen);
	WMtPopupMenu *nogun_menu			= WMrDialog_GetItemByID(inDialog, cEditCombat_NoGun);
	WMtEditField *alarmfind_edit		= WMrDialog_GetItemByID(inDialog, cEditCombat_AlarmFindDist);
	WMtEditField *alarmignore_edit		= WMrDialog_GetItemByID(inDialog, cEditCombat_AlarmIgnoreDist);
	WMtEditField *alarmchase_edit		= WMrDialog_GetItemByID(inDialog, cEditCombat_AlarmChaseDist);
	WMtCheckBox *alarmknockdown_cb		= WMrDialog_GetItemByID(inDialog, cEditCombat_AlarmKnockdown);
	WMtEditField *alarmattackdmg_edit	= WMrDialog_GetItemByID(inDialog, cEditCombat_AlarmAttackDmg);
	WMtEditField *alarmfighttimer_edit	= WMrDialog_GetItemByID(inDialog, cEditCombat_AlarmFightTimer);

	UUtBool				checked, handled = UUcTrue, dirty = UUcFalse;

	OBJtObject			*object = (OBJtObject *) WMrDialog_GetUserData(inDialog);
	OBJtOSD_All			object_specific_data;
	OBJtOSD_Combat		*combat_osd;

	OBJrObject_GetObjectSpecificData(object, &object_specific_data);
	combat_osd = &object_specific_data.osd.combat_osd;

	{
		UUtUns16 itr;

		for(itr = 0; itr < AI2cCombatRange_NumStoredRanges; itr++) {
			behavior_prompt[itr] = WMrDialog_GetItemByID(inDialog, cEditCombat_Prompt_Base + itr);
			behavior_menu[itr] = WMrDialog_GetItemByID(inDialog, cEditCombat_Menu_Base + itr);
		}
	}


	switch (inMessage)
	{
		case WMcDialogItem_OK:
			WMrDialog_ModalEnd(inDialog, UUcTrue);
		break;

		case WMcDialogItem_Cancel:
			WMrDialog_ModalEnd(inDialog, UUcFalse);
		break;

		case WMcMessage_InitDialog:
			{
				UUtUns32 init_itr;

				for(init_itr = 0; init_itr < AI2cCombatRange_NumStoredRanges; init_itr++)
				{
					WMrPopupMenu_AppendStringList(behavior_menu[init_itr], AI2cBehavior_Max, AI2cBehaviorName);
					WMrPopupMenu_SetSelection(behavior_menu[init_itr], (UUtUns16) combat_osd->behavior[init_itr]);

					WMrWindow_SetTitle(behavior_prompt[init_itr], AI2cCombatRangeName[init_itr + 1], WMcMaxTitleLength);
				}
			}

			WMrEditField_SetInt32(id_edit, combat_osd->id);
			WMrEditField_SetText(name_edit, combat_osd->name);
			WMrEditField_SetFloat(range_edit, combat_osd->medium_range);
			WMrEditField_SetFloat(shortrange_edit, combat_osd->short_range);
			WMrEditField_SetFloat(pursuitdist_edit, combat_osd->pursuit_distance);
			WMrEditField_SetInt32(panicmelee_edit, combat_osd->panic_melee);
			WMrEditField_SetInt32(panicgunfire_edit, combat_osd->panic_gunfire);
			WMrEditField_SetInt32(panicsight_edit, combat_osd->panic_sight);
			WMrEditField_SetInt32(panichurt_edit, combat_osd->panic_hurt);
			WMrEditField_SetFloat(alarmfind_edit, combat_osd->alarm_find_distance);
			WMrEditField_SetFloat(alarmignore_edit, combat_osd->alarm_ignore_enemy_dist);
			WMrEditField_SetFloat(alarmchase_edit, combat_osd->alarm_chase_enemy_dist);
			WMrEditField_SetInt32(alarmattackdmg_edit, combat_osd->alarm_damage_threshold);
			WMrEditField_SetInt32(alarmfighttimer_edit, combat_osd->alarm_fight_timer);

			WMrPopupMenu_AppendStringList(melee_menu, AI2cFight_Max, AI2cFightName);
			WMrPopupMenu_SetSelection(melee_menu, (UUtUns16) combat_osd->melee_when);

			WMrPopupMenu_AppendStringList(nogun_menu, AI2cNoGun_Max, AI2cNoGunName);
			WMrPopupMenu_SetSelection(nogun_menu, (UUtUns16) combat_osd->no_gun);

			WMrCheckBox_SetCheck(alarmknockdown_cb, ((combat_osd->flags & OBJcCombatFlags_Alarm_AttackIfKnockedDown) > 0));
		break;

		case WMcMessage_MenuCommand:
			{
				UUtUns16 menu_window_id = WMrWindow_GetID((WMtMenu *) inParam2);
				UUtUns16 menu_item = UUmLowWord(inParam1);

				dirty = UUcTrue;

				if ((menu_window_id >= cEditCombat_Menu_Base) && (menu_window_id < (cEditCombat_Menu_Base + AI2cCombatRange_NumStoredRanges))) {
					combat_osd->behavior[menu_window_id - cEditCombat_Menu_Base] = menu_item;
				}
				else if (menu_window_id == cEditCombat_MeleeWhen) {
					combat_osd->melee_when = menu_item;
				}
				else if (menu_window_id == cEditCombat_NoGun) {
					combat_osd->no_gun = menu_item;
				}
				else {
					dirty = UUcFalse;
					handled = UUcFalse;
				}
			}
		break;

		case WMcMessage_Command:
			dirty = UUcTrue;

			switch(UUmLowWord(inParam1))
			{
				case cEditCombat_ID:
					combat_osd->id = (UUtUns16) WMrEditField_GetInt32(id_edit);
				break;

				case cEditCombat_Name:
					WMrEditField_GetText(name_edit, combat_osd->name, sizeof(combat_osd->name));
				break;

				case cEditCombat_MedRange:
					combat_osd->medium_range = WMrEditField_GetFloat(range_edit);
				break;

				case cEditCombat_ShortRange:
					combat_osd->short_range = WMrEditField_GetFloat(shortrange_edit);
				break;

				case cEditCombat_PursuitDist:
					combat_osd->pursuit_distance = WMrEditField_GetFloat(pursuitdist_edit);
				break;

				case cEditCombat_PanicMelee:
					combat_osd->panic_melee = WMrEditField_GetInt32(panicmelee_edit);
				break;

				case cEditCombat_PanicGunfire:
					combat_osd->panic_gunfire = WMrEditField_GetInt32(panicgunfire_edit);
				break;

				case cEditCombat_PanicSight:
					combat_osd->panic_sight = WMrEditField_GetInt32(panicsight_edit);
				break;

				case cEditCombat_PanicHurt:
					combat_osd->panic_hurt = WMrEditField_GetInt32(panichurt_edit);
				break;

				case cEditCombat_AlarmFindDist:
					combat_osd->alarm_find_distance = WMrEditField_GetFloat(alarmfind_edit);
				break;

				case cEditCombat_AlarmIgnoreDist:
					combat_osd->alarm_ignore_enemy_dist = WMrEditField_GetFloat(alarmignore_edit);
				break;

				case cEditCombat_AlarmChaseDist:
					combat_osd->alarm_chase_enemy_dist = WMrEditField_GetFloat(alarmchase_edit);
				break;

				case cEditCombat_AlarmAttackDmg:
					combat_osd->alarm_damage_threshold = WMrEditField_GetInt32(alarmattackdmg_edit);
				break;

				case cEditCombat_AlarmFightTimer:
					combat_osd->alarm_fight_timer = WMrEditField_GetInt32(alarmfighttimer_edit);
				break;

				case cEditCombat_AlarmKnockdown:
					checked = WMrCheckBox_GetCheck(alarmknockdown_cb);
					if (checked) {
						combat_osd->flags |= OBJcCombatFlags_Alarm_AttackIfKnockedDown;
					} else {
						combat_osd->flags &= ~OBJcCombatFlags_Alarm_AttackIfKnockedDown;
					}
				break;

				default:
					dirty = UUcFalse;
					handled = UUcFalse;
				break;
			}
		break;

		default:
			handled = UUcFalse;
		break;
	}

	if (dirty) {
		OWrObjectProperties_SetOSD(inDialog, object, &object_specific_data);
	}

	return handled;
}

UUtUns16 OWrChooseCombat(UUtUns16 inBehaviorID)
{
	OBJtObject *object = NULL;
	OBJtOSD_All	object_specific_data;
	UUtUns16 result = (UUtUns16) -1;

	object_specific_data.osd.combat_osd.id = inBehaviorID;
	object = OBJrObjectType_Search(OBJcType_Combat, OBJcSearch_CombatID, &object_specific_data);

	object = OWrSelectObject(OBJcType_Combat, object, UUcTrue, UUcFalse);

	if (NULL != object) {
		OBJrObject_GetObjectSpecificData(object, &object_specific_data);
		result = object_specific_data.osd.combat_osd.id;
	}

	return result;
}

#endif
