#if TOOL_VERSION
/*
	FILE:	Oni_Win_AI_Character.c
	
	AUTHOR:	Michael Evans
	
	CREATED: April 25, 2000
	
	PURPOSE: AI Character Windows
	
	Copyright (c) Bungie Software 2000

*/

#include "BFW.h"
#include "Oni_Win_AI.h"
#include "Oni_Windows.h"
#include "Oni_Object.h"
#include "Oni_AI2_Movement.h"
#include "Oni_Character.h"
#include "Oni_Win_AI_Combat.h"
#include "Oni_Win_AI_Melee.h"
#include "Oni_Win_AI_Neutral.h"
#include "Oni_Win_Tools.h"

#include "WM_DrawContext.h"
#include "WM_PartSpecification.h"
#include "WM_Scrollbar.h"
#include "WM_MenuBar.h"
#include "WM_Menu.h"
#include "WM_PopupMenu.h"
#include "WM_EditField.h"
#include "WM_CheckBox.h"
#include "WM_ListBox.h"
#include "WM_Text.h"
#include "WM_Button.h"

enum
{
	OWTcCharProp_Name			= 200,
	OWTcCharProp_Team			= 201,
	OWTcCharProp_HitPoints		= 202,
	OWTcCharProp_Weapon			= 203,
	OWTcCharProp_Ammo			= 204,
	OWTcCharProp_Class			= 205,

	OWTcCharProp_IsPlayer		= 250,
	OWTcCharProp_RandomCostume	= 251,
	OWTcCharProp_NotPresent		= 252,
	OWTcCharProp_NonCombatant	= 253,
	OWTcCharProp_SpawnMultiple	= 254,
	OWTcCharProp_Unkillable		= 255,
	OWTcCharProp_InfiniteAmmo	= 256,
	OWTcCharProp_Omniscient		= 257,
	OWTcCharProp_HasLSI			= 258,
	OWTcCharProp_Boss			= 259,
	OWTcCharProp_UpgradeDifficulty = 260,
	OWTcCharProp_NoAutoDrop		= 261,

	OWTcCharProp_SpawnScript	= 300,
	OWTcCharProp_DieScript		= 301,
	OWTcCharProp_CombatScript	= 302,
	OWTcCharProp_AlarmScript	= 303,
	OWTcCharProp_HurtScript		= 304,
	OWTcCharProp_DefeatedScript	= 305,
	OWTcCharProp_OutOfAmmoScript= 306,
	OWTcCharProp_NoPathScript	= 307,

	OWTcCharProp_InvUse			= 400,
	OWTcCharProp_InvDrop		= 450,

	OWTcCharProp_Job			= 500,
	OWTcCharProp_JobDataPrompt	= 501,
	OWTcCharProp_JobDataEdit	= 502,
	OWTcCharProp_JobDataSelect	= 503,
	OWTcCharProp_JobDataDescription	= 504,

	OWTcCharProp_InitialAlert	= 600,
	OWTcCharProp_MinimumAlert	= 601,
	OWTcCharProp_JobStartAlert	= 602,
	OWTcCharProp_InvestigateAlert= 603,

	OWTcCharProp_EditCombat		= 700,
	OWTcCharProp_CombatDescription	=701,
	OWTcCharProp_MeleeDescription= 702,
	OWTcCharProp_EditMelee		= 703,
	OWTcCharProp_NeutralDescription= 704,
	OWTcCharProp_EditNeutral	= 705,

	OWTcCharProp_AlarmGroups	= 800,

	OWTcCharProp_StrongUnseen	= 900,
	OWTcCharProp_WeakUnseen		= 901,
	OWTcCharProp_StrongSeen		= 902,
	OWTcCharProp_WeakSeen		= 903,
	OWTcCharProp_PursueLost		= 904
};

static const char *OBJgJob_Name[AI2cGoal_Max] =
{
	"None",
	"Idle",
	"Guard",
	"Patrol",
	"Team Battle",
	"Combat",
	"Melee",
	"Alarm",
	"Neutral",
	"Panic"
};
	
static const char *OBJgAlert_Name[AI2cAlertStatus_Max] =
{
	"Lull",
	"Low",
	"Medium",
	"High",
	"Combat"
};


// ======================================================================



void OWrProp_BuildWeaponMenu(WMtPopupMenu *inPopupMenu, char *inCurrentClass)
{
	UUtUns32 itr;
	UUtUns32 count = TMrInstance_GetTagCount(WPcTemplate_WeaponClass);
	UUtError error;
	UUtUns32 selection = 0;

	WMrPopupMenu_AppendItem_Light(inPopupMenu, 0, "none");

	WMrPopupMenu_AppendItem_Divider(inPopupMenu);

	for(itr = 0; itr < count; itr++)
	{
		void *weapon;
		char *weapon_name;

		error = TMrInstance_GetDataPtr_ByNumber(WPcTemplate_WeaponClass, itr, &weapon);

		if (error) {
			break;
		}

		weapon_name = TMrInstance_GetInstanceName(weapon);

		WMrPopupMenu_AppendItem_Light(inPopupMenu, (UUtUns16) (itr + 1), weapon_name);

		if ((inCurrentClass != NULL) && (UUmString_IsEqual(weapon_name, inCurrentClass))) {
			selection = itr + 1;
		}
	}

	WMrPopupMenu_SetSelection(inPopupMenu, (UUtUns16) selection);

	return;
}

static void OWiProp_Char_BuildClassListBox(WMtListBox *inListBox, OBJtOSD_All *inObjectSpecificData)
{
	UUtUns32 template_type = TRcTemplate_CharacterClass;
	UUtUns32 itr;
	UUtUns32 count = TMrInstance_GetTagCount(template_type);
	UUtError error;
	UUtUns32 selection = 0;

	for(itr = 0; itr < count; itr++)
	{
		void *character_class;
		char *class_name;

		error = TMrInstance_GetDataPtr_ByNumber(template_type, itr, &character_class);

		if (error) {
			break;
		}

		class_name = TMrInstance_GetInstanceName(character_class);

		WMrListBox_AddString(inListBox, class_name);
		WMrListBox_SetItemData(inListBox, itr, itr);

		if (UUmString_IsEqual(class_name, inObjectSpecificData->osd.character_osd.character_class)) {
			selection = itr;
		}
	}

	WMrListBox_SetSelection(inListBox, LBcDontNotifyParent, selection);

	return;
}


// ----------------------------------------------------------------------
static void
OWiProp_Char_HandleCommand(
	WMtDialog			*inDialog,
	UUtUns16			inItemID,
	UUtUns16			inEvent,
	WMtWindow			*inControl)
{
	UUtBool				is_inventory;
	UUtUns16			*dataptr;
	char				string[64];
	OBJtObject			*object = (OBJtObject *) WMrDialog_GetUserData(inDialog);
	OBJtOSD_All			object_specific_data;

	OBJrObject_GetObjectSpecificData(object, &object_specific_data);

	switch(inItemID) {

	default:
		is_inventory = UUcFalse;

		if ((inItemID >= OWTcCharProp_InvUse) && (inItemID < OWTcCharProp_InvUse + OBJcCharInv_Max)) {
			is_inventory = UUcTrue;
			dataptr = &object_specific_data.osd.character_osd.inventory[inItemID - OWTcCharProp_InvUse][OBJcCharSlot_Use];

		} else if ((inItemID >= OWTcCharProp_InvDrop) && (inItemID < OWTcCharProp_InvDrop + OBJcCharInv_Max)) {
			is_inventory = UUcTrue;
			dataptr = &object_specific_data.osd.character_osd.inventory[inItemID - OWTcCharProp_InvDrop][OBJcCharSlot_Drop];
		}

		if (is_inventory) {
			WMrMessage_Send(inControl, EFcMessage_GetText, (UUtUns32) string, 64);

			if (UUrString_To_Uns16(string, dataptr) == UUcError_None) {
				// write our OSD
				OWrObjectProperties_SetOSD(inDialog, object, &object_specific_data);
			}
		}
		break;
	}
}

static UUtBool AIrJob_HasJobData(UUtUns32 inJob)
{
	UUtBool has_job_data = UUcFalse;

	switch(inJob)
	{
		case AI2cGoal_Guard:
		case AI2cGoal_Patrol:
			has_job_data = UUcTrue;
		break;
	}

	return has_job_data;
}

static UUtBool AIrJob_HasJobSelectButton(UUtUns32 inJob)
{
	UUtBool has_job_select = UUcFalse;

	switch(inJob)
	{
		case AI2cGoal_Patrol:
			has_job_select = UUcTrue;
		break;
	}

	return has_job_select;
}

// ----------------------------------------------------------------------

UUtBool
OWrProp_Char_Callback(
	WMtDialog			*inDialog,
	WMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2)
{
	UUtBool				handled;
	WMtPopupMenu		*team_menu;
	WMtPopupMenu		*weapon_menu;
	WMtPopupMenu		*job_menu;
	WMtPopupMenu		*initial_alert_menu;
	WMtPopupMenu		*minimum_alert_menu;
	WMtPopupMenu		*jobstart_alert_menu;
	WMtPopupMenu		*investigate_alert_menu;
	WMtPopupMenu		*strong_unseen_menu;
	WMtPopupMenu		*weak_unseen_menu;
	WMtPopupMenu		*strong_seen_menu;
	WMtPopupMenu		*weak_seen_menu;
	WMtPopupMenu		*pursue_lost_menu;
	WMtEditField		*character_name;
	WMtEditField		*hit_points;
	WMtEditField		*ammo_percentage;
	WMtEditField		*spawn_script;
	WMtEditField		*die_script;
	WMtEditField		*combat_script;
	WMtEditField		*alarm_script;
	WMtEditField		*hurt_script;
	WMtEditField		*defeated_script;
	WMtEditField		*outofammo_script;
	WMtEditField		*nopath_script;
	WMtEditField		*alarm_groups;
	WMtCheckBox			*is_player;
	WMtCheckBox			*random_costume;
	WMtCheckBox			*not_present;
	WMtCheckBox			*non_combatant;
	WMtCheckBox			*unkillable;
	WMtCheckBox			*infinite_ammo;
	WMtCheckBox			*omniscient;
	WMtCheckBox			*has_lsi;
	WMtCheckBox			*boss;
	WMtCheckBox			*upgrade_difficulty;
	WMtCheckBox			*no_autodrop;
	WMtListBox			*character_class;
	WMtListBox			*spawn_multiple;
	OBJtObject			*object = (OBJtObject *) WMrDialog_GetUserData(inDialog);
	OBJtOSD_All			object_specific_data;
	OBJtOSD_Character	*char_osd;

	WMtText				*job_data_prompt;
	WMtButton			*job_data_button;
	WMtEditField		*job_data_edit;
	WMtText				*job_data_description;
	WMtText				*combat_description;
	WMtText				*melee_description;
	WMtText				*neutral_description;
	
	char				string[100];
	UUtUns32			alarm_bitmask;

	UUtBool				dirty = UUcFalse;
	UUtBool				job_has_changed = UUcFalse;
	UUtBool				job_data_changed = UUcFalse;
	UUtBool				combat_data_changed = UUcFalse;
	UUtBool				melee_data_changed = UUcFalse;
	UUtBool				neutral_data_changed = UUcFalse;

	OBJrObject_GetObjectSpecificData(object, &object_specific_data);
	
	team_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_Team);
	weapon_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_Weapon);
	job_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_Job);

	initial_alert_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_InitialAlert);
	minimum_alert_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_MinimumAlert);
	jobstart_alert_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_JobStartAlert);
	investigate_alert_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_InvestigateAlert);

	strong_unseen_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_StrongUnseen);
	weak_unseen_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_WeakUnseen);
	strong_seen_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_StrongSeen);
	weak_seen_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_WeakSeen);
	pursue_lost_menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_PursueLost);

	character_name = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_Name);
	hit_points = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_HitPoints);
	ammo_percentage = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_Ammo);
	spawn_script = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_SpawnScript);
	die_script = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_DieScript);
	combat_script = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_CombatScript);
	alarm_script = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_AlarmScript);
	hurt_script = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_HurtScript);
	defeated_script = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_DefeatedScript);
	outofammo_script = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_OutOfAmmoScript);
	nopath_script = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_NoPathScript);
	alarm_groups = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_AlarmGroups);

	is_player = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_IsPlayer);
	random_costume = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_RandomCostume);
	not_present = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_NotPresent);
	non_combatant = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_NonCombatant);
	spawn_multiple = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_SpawnMultiple);
	unkillable = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_Unkillable);
	infinite_ammo = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_InfiniteAmmo);
	omniscient = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_Omniscient);
	has_lsi = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_HasLSI);	
	boss = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_Boss);
	upgrade_difficulty = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_UpgradeDifficulty);
	no_autodrop = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_NoAutoDrop);

	character_class = (WMtListBox *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_Class);
	
	job_data_prompt = (WMtText *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_JobDataPrompt);
	job_data_button = (WMtButton *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_JobDataSelect);
	job_data_edit = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_JobDataEdit);
	job_data_description = (WMtText *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_JobDataDescription);

	combat_description = (WMtText *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_CombatDescription);
	melee_description = (WMtText *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_MeleeDescription);
	neutral_description = (WMtText *) WMrDialog_GetItemByID(inDialog, OWTcCharProp_NeutralDescription);

	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			if (NULL != team_menu) {
				WMrPopupMenu_AppendStringList(team_menu, OBJcCharacter_TeamMaxNamed, OBJgCharacter_TeamName);
				WMrPopupMenu_SetSelection(team_menu, (UUtUns16) object_specific_data.osd.character_osd.team_number);
			}

			if (NULL != job_menu) {
				WMrPopupMenu_AppendStringList(job_menu, AI2cGoal_Max, OBJgJob_Name);
				WMrPopupMenu_SetSelection(job_menu, (UUtUns16) object_specific_data.osd.character_osd.job);
			}

			if (NULL != initial_alert_menu) {
				WMrPopupMenu_AppendStringList(initial_alert_menu, AI2cAlertStatus_Max, OBJgAlert_Name);
				WMrPopupMenu_SetSelection(initial_alert_menu, (UUtUns16) object_specific_data.osd.character_osd.initial_alert);
			}

			if (NULL != minimum_alert_menu) {
				WMrPopupMenu_AppendStringList(minimum_alert_menu, AI2cAlertStatus_Max, OBJgAlert_Name);
				WMrPopupMenu_SetSelection(minimum_alert_menu, (UUtUns16) object_specific_data.osd.character_osd.minimum_alert);
			}

			if (NULL != jobstart_alert_menu) {
				WMrPopupMenu_AppendStringList(jobstart_alert_menu, AI2cAlertStatus_Max, OBJgAlert_Name);
				WMrPopupMenu_SetSelection(jobstart_alert_menu, (UUtUns16) object_specific_data.osd.character_osd.jobstart_alert);
			}

			if (NULL != investigate_alert_menu) {
				WMrPopupMenu_AppendStringList(investigate_alert_menu, AI2cAlertStatus_Max, OBJgAlert_Name);
				WMrPopupMenu_SetSelection(investigate_alert_menu, (UUtUns16) object_specific_data.osd.character_osd.investigate_alert);
			}

			if (NULL != strong_unseen_menu) {
				WMrPopupMenu_AppendStringList(strong_unseen_menu, AI2cPursuitMode_Max, AI2cPursuitModeName);
				WMrPopupMenu_SetSelection(strong_unseen_menu, (UUtUns16) object_specific_data.osd.character_osd.pursue_strong_unseen);
			}

			if (NULL != weak_unseen_menu) {
				WMrPopupMenu_AppendStringList(weak_unseen_menu, AI2cPursuitMode_Max, AI2cPursuitModeName);
				WMrPopupMenu_SetSelection(weak_unseen_menu, (UUtUns16) object_specific_data.osd.character_osd.pursue_weak_unseen);
			}

			if (NULL != strong_seen_menu) {
				WMrPopupMenu_AppendStringList(strong_seen_menu, AI2cPursuitMode_Max, AI2cPursuitModeName);
				WMrPopupMenu_SetSelection(strong_seen_menu, (UUtUns16) object_specific_data.osd.character_osd.pursue_strong_seen);
			}

			if (NULL != weak_seen_menu) {
				WMrPopupMenu_AppendStringList(weak_seen_menu, AI2cPursuitMode_Max, AI2cPursuitModeName);
				WMrPopupMenu_SetSelection(weak_seen_menu, (UUtUns16) object_specific_data.osd.character_osd.pursue_weak_seen);
			}

			if (NULL != pursue_lost_menu) {
				WMrPopupMenu_AppendStringList(pursue_lost_menu, AI2cPursuitLost_Max, AI2cPursuitLostName);
				WMrPopupMenu_SetSelection(pursue_lost_menu, (UUtUns16) object_specific_data.osd.character_osd.pursue_lost);
			}

			OWrProp_BuildWeaponMenu(weapon_menu, object_specific_data.osd.character_osd.weapon_class_name);
			OWiProp_Char_BuildClassListBox(character_class, &object_specific_data);

			WMrEditField_SetText(character_name, object_specific_data.osd.character_osd.character_name);
			WMrEditField_SetInt32(hit_points, object_specific_data.osd.character_osd.hit_points);
			WMrEditField_SetInt32(ammo_percentage, object_specific_data.osd.character_osd.ammo_percentage);
			WMrEditField_SetText(spawn_script, object_specific_data.osd.character_osd.spawn_script);
			WMrEditField_SetText(die_script, object_specific_data.osd.character_osd.die_script);
			WMrEditField_SetText(combat_script, object_specific_data.osd.character_osd.combat_script);
			WMrEditField_SetText(alarm_script, object_specific_data.osd.character_osd.alarm_script);
			WMrEditField_SetText(hurt_script, object_specific_data.osd.character_osd.hurt_script);
			WMrEditField_SetText(defeated_script, object_specific_data.osd.character_osd.defeated_script);
			WMrEditField_SetText(outofammo_script, object_specific_data.osd.character_osd.outofammo_script);
			WMrEditField_SetText(nopath_script, object_specific_data.osd.character_osd.nopath_script);

			WMrCheckBox_SetCheck(is_player, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_Player) > 0);
			WMrCheckBox_SetCheck(random_costume, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_RandomCostume) > 0);
			WMrCheckBox_SetCheck(not_present, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_NotInitiallyPresent) > 0);
			WMrCheckBox_SetCheck(non_combatant, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_NonCombatant) > 0);
			WMrCheckBox_SetCheck(spawn_multiple, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_SpawnMultiple) > 0);
			WMrCheckBox_SetCheck(unkillable, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_Unkillable) > 0);
			WMrCheckBox_SetCheck(infinite_ammo, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_InfiniteAmmo) > 0);
			WMrCheckBox_SetCheck(omniscient, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_Omniscient) > 0);
			WMrCheckBox_SetCheck(has_lsi, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_HasLSI) > 0);
			WMrCheckBox_SetCheck(boss, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_Boss) > 0);
			WMrCheckBox_SetCheck(upgrade_difficulty, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_UpgradeDifficulty) > 0);
			WMrCheckBox_SetCheck(no_autodrop, (object_specific_data.osd.character_osd.flags & OBJcCharFlags_NoAutoDrop) > 0);

			BitMaskToString(object_specific_data.osd.character_osd.alarm_groups, string, 100);
			WMrMessage_Send(alarm_groups, EFcMessage_SetText, (UUtUns32) string, 0);

			{
				UUtUns16 item_id;
				WMtEditField *set_edit_field;

				for(item_id = OWTcCharProp_InvUse; item_id < (OWTcCharProp_InvUse + OBJcCharInv_Max); item_id++) 
				{
					set_edit_field = (WMtEditField *) WMrDialog_GetItemByID(inDialog, item_id);
					WMrEditField_SetInt32(set_edit_field, object_specific_data.osd.character_osd.inventory[item_id - OWTcCharProp_InvUse][OBJcCharSlot_Use]);
				}

				for(item_id = OWTcCharProp_InvDrop; item_id < (OWTcCharProp_InvDrop + OBJcCharInv_Max); item_id++) 
				{
					set_edit_field = (WMtEditField *) WMrDialog_GetItemByID(inDialog, item_id);
					WMrEditField_SetInt32(set_edit_field, object_specific_data.osd.character_osd.inventory[item_id - OWTcCharProp_InvDrop][OBJcCharSlot_Drop]);
				}
			}

			WMrEditField_SetInt32(job_data_edit, object_specific_data.osd.character_osd.job_specific_id);
			job_has_changed = UUcTrue;
			combat_data_changed = UUcTrue;
			melee_data_changed = UUcTrue;
			neutral_data_changed = UUcTrue;
			dirty = UUcTrue;
		break;		
		
		case WMcMessage_Command:
			switch(UUmLowWord(inParam1)) 
			{
				case OWTcCharProp_JobDataSelect:
					if (AI2cGoal_Patrol == object_specific_data.osd.character_osd.job) {
						object_specific_data.osd.character_osd.job_specific_id = OWrChoosePath(object_specific_data.osd.character_osd.job_specific_id);
						WMrEditField_SetInt32(job_data_edit, object_specific_data.osd.character_osd.job_specific_id);
						job_data_changed = UUcTrue;
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_JobDataEdit:
					object_specific_data.osd.character_osd.job_specific_id = (UUtUns16) WMrEditField_GetInt32(job_data_edit);
					job_data_changed = UUcTrue;
					dirty = UUcTrue;
				break;
	
				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, UUcFalse);
				break;

				case WMcDialogItem_OK:
					WMrDialog_ModalEnd(inDialog, UUcTrue);
				break;

				case OWTcCharProp_AlarmGroups:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						WMrEditField_GetText(alarm_groups, string, 100);
						if (string[0] == '\0') {
							alarm_bitmask = 0;
						} else {
							StringToBitMask(&alarm_bitmask, string);
						}

						if (object_specific_data.osd.character_osd.alarm_groups != alarm_bitmask) {
							object_specific_data.osd.character_osd.alarm_groups = alarm_bitmask;
							dirty = UUcTrue;
						}
					}
				break;

				case OWTcCharProp_Name:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						WMrEditField_GetText(character_name, object_specific_data.osd.character_osd.character_name, ONcMaxPlayerNameLength);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_SpawnScript:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						WMrEditField_GetText(spawn_script, object_specific_data.osd.character_osd.spawn_script, SLcScript_MaxNameLength);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_DieScript:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						WMrEditField_GetText(die_script, object_specific_data.osd.character_osd.die_script, SLcScript_MaxNameLength);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_CombatScript:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						WMrEditField_GetText(combat_script, object_specific_data.osd.character_osd.combat_script, SLcScript_MaxNameLength);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_AlarmScript:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						WMrEditField_GetText(spawn_script, object_specific_data.osd.character_osd.alarm_script, SLcScript_MaxNameLength);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_HurtScript:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						WMrEditField_GetText(hurt_script, object_specific_data.osd.character_osd.hurt_script, SLcScript_MaxNameLength);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_DefeatedScript:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						WMrEditField_GetText(defeated_script, object_specific_data.osd.character_osd.defeated_script, SLcScript_MaxNameLength);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_OutOfAmmoScript:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						WMrEditField_GetText(outofammo_script, object_specific_data.osd.character_osd.outofammo_script, SLcScript_MaxNameLength);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_NoPathScript:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						WMrEditField_GetText(nopath_script, object_specific_data.osd.character_osd.nopath_script, SLcScript_MaxNameLength);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_IsPlayer:
					if (WMrCheckBox_GetCheck(is_player)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_Player;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_Player;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_RandomCostume:
					if (WMrCheckBox_GetCheck(random_costume)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_RandomCostume;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_RandomCostume;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_NotPresent:
					if (WMrCheckBox_GetCheck(not_present)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_NotInitiallyPresent;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_NotInitiallyPresent;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_NonCombatant:
					if (WMrCheckBox_GetCheck(non_combatant)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_NonCombatant;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_NonCombatant;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_SpawnMultiple:
					if (WMrCheckBox_GetCheck(spawn_multiple)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_SpawnMultiple;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_SpawnMultiple;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_Unkillable:
					if (WMrCheckBox_GetCheck(unkillable)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_Unkillable;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_Unkillable;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_InfiniteAmmo:
					if (WMrCheckBox_GetCheck(infinite_ammo)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_InfiniteAmmo;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_InfiniteAmmo;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_Omniscient:
					if (WMrCheckBox_GetCheck(omniscient)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_Omniscient;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_Omniscient;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_HasLSI:
					if (WMrCheckBox_GetCheck(has_lsi)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_HasLSI;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_HasLSI;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_Boss:
					if (WMrCheckBox_GetCheck(boss)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_Boss;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_Boss;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_UpgradeDifficulty:
					if (WMrCheckBox_GetCheck(upgrade_difficulty)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_UpgradeDifficulty;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_UpgradeDifficulty;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_NoAutoDrop:
					if (WMrCheckBox_GetCheck(no_autodrop)) {
						object_specific_data.osd.character_osd.flags |= OBJcCharFlags_NoAutoDrop;
					}
					else {
						object_specific_data.osd.character_osd.flags &= ~OBJcCharFlags_NoAutoDrop;
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_Ammo:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						object_specific_data.osd.character_osd.ammo_percentage = WMrEditField_GetInt32(ammo_percentage);
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_HitPoints:
					if (EFcNotify_ContentChanged == UUmHighWord(inParam1)) {
						object_specific_data.osd.character_osd.hit_points = WMrEditField_GetInt32(hit_points);
					}
					dirty = UUcTrue;
				break;

				case OWTcCharProp_Class:
					if (LBcNotify_SelectionChanged == UUmHighWord(inParam1)) {
						WMrListBox_GetText(character_class, object_specific_data.osd.character_osd.character_class, -1);
					}
					dirty = UUcTrue;
				break;
				
				case OWTcCharProp_EditCombat:
					object_specific_data.osd.character_osd.combat_id = OWrChooseCombat(object_specific_data.osd.character_osd.combat_id);
					combat_data_changed = UUcTrue;
					dirty = UUcTrue;
				break;

				case OWTcCharProp_EditMelee:
					object_specific_data.osd.character_osd.melee_id = OWrChooseMelee(object_specific_data.osd.character_osd.melee_id);
					melee_data_changed = UUcTrue;
					dirty = UUcTrue;
				break;

				case OWTcCharProp_EditNeutral:
					object_specific_data.osd.character_osd.neutral_id = OWrChooseNeutral(object_specific_data.osd.character_osd.neutral_id);
					neutral_data_changed = UUcTrue;
					dirty = UUcTrue;
				break;

				default:
					{
						UUtBool is_inventory = UUcFalse;
						UUtUns16 *inventory_dataptr;						
						UUtUns16 item_id = UUmLowWord(inParam1);

						if ((item_id >= OWTcCharProp_InvUse) && (item_id < (OWTcCharProp_InvUse + OBJcCharInv_Max))) {
							is_inventory = UUcTrue;
							inventory_dataptr = &object_specific_data.osd.character_osd.inventory[item_id - OWTcCharProp_InvUse][OBJcCharSlot_Use];
						}
						else if ((item_id >= OWTcCharProp_InvDrop) && (item_id < (OWTcCharProp_InvDrop + OBJcCharInv_Max))) {
							is_inventory = UUcTrue;
							inventory_dataptr = &object_specific_data.osd.character_osd.inventory[item_id - OWTcCharProp_InvDrop][OBJcCharSlot_Drop];
						}

						if (is_inventory) {
							WMtEditField *get_edit_field;

							get_edit_field = (WMtEditField *) WMrDialog_GetItemByID(inDialog, item_id);

							*inventory_dataptr = (UUtUns16) WMrEditField_GetInt32(get_edit_field);
							dirty = UUcTrue;
						}
					}
				break;
			}
		break;

		case WMcMessage_MenuCommand:
			switch (WMrWindow_GetID((WMtMenu *) inParam2))
			{
				case OWTcCharProp_Team:
					object_specific_data.osd.character_osd.team_number = UUmLowWord(inParam1);
					dirty = UUcTrue;
				break;

				case OWTcCharProp_Weapon:
					WMrPopupMenu_GetItemText(weapon_menu, UUmLowWord(inParam1), object_specific_data.osd.character_osd.weapon_class_name);
					dirty = UUcTrue;
				break;

				case OWTcCharProp_Job:
					if (object_specific_data.osd.character_osd.job != UUmLowWord(inParam1)) {
						object_specific_data.osd.character_osd.job = UUmLowWord(inParam1);
						object_specific_data.osd.character_osd.job_specific_id = 0;
						job_has_changed = UUcTrue;
					}
				break;

				case OWTcCharProp_InitialAlert:
					if (object_specific_data.osd.character_osd.initial_alert != (AI2tAlertStatus) UUmLowWord(inParam1)) {
						object_specific_data.osd.character_osd.initial_alert = (AI2tAlertStatus) UUmLowWord(inParam1);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_MinimumAlert:
					if (object_specific_data.osd.character_osd.minimum_alert != (AI2tAlertStatus) UUmLowWord(inParam1)) {
						object_specific_data.osd.character_osd.minimum_alert = (AI2tAlertStatus) UUmLowWord(inParam1);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_JobStartAlert:
					if (object_specific_data.osd.character_osd.jobstart_alert != (AI2tAlertStatus) UUmLowWord(inParam1)) {
						object_specific_data.osd.character_osd.jobstart_alert = (AI2tAlertStatus) UUmLowWord(inParam1);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_InvestigateAlert:
					if (object_specific_data.osd.character_osd.investigate_alert != (AI2tAlertStatus) UUmLowWord(inParam1)) {
						object_specific_data.osd.character_osd.investigate_alert = (AI2tAlertStatus) UUmLowWord(inParam1);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_StrongUnseen:
					if (object_specific_data.osd.character_osd.pursue_strong_unseen != (AI2tPursuitMode) UUmLowWord(inParam1)) {
						object_specific_data.osd.character_osd.pursue_strong_unseen = (AI2tPursuitMode) UUmLowWord(inParam1);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_WeakUnseen:
					if (object_specific_data.osd.character_osd.pursue_weak_unseen != (AI2tPursuitMode) UUmLowWord(inParam1)) {
						object_specific_data.osd.character_osd.pursue_weak_unseen = (AI2tPursuitMode) UUmLowWord(inParam1);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_StrongSeen:
					if (object_specific_data.osd.character_osd.pursue_strong_seen != (AI2tPursuitMode) UUmLowWord(inParam1)) {
						object_specific_data.osd.character_osd.pursue_strong_seen = (AI2tPursuitMode) UUmLowWord(inParam1);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_WeakSeen:
					if (object_specific_data.osd.character_osd.pursue_weak_seen != (AI2tPursuitMode) UUmLowWord(inParam1)) {
						object_specific_data.osd.character_osd.pursue_weak_seen = (AI2tPursuitMode) UUmLowWord(inParam1);
						dirty = UUcTrue;
					}
				break;

				case OWTcCharProp_PursueLost:
					if (object_specific_data.osd.character_osd.pursue_lost != (AI2tPursuitLostBehavior) UUmLowWord(inParam1)) {
						object_specific_data.osd.character_osd.pursue_lost = (AI2tPursuitLostBehavior) UUmLowWord(inParam1);
						dirty = UUcTrue;
					}
				break;
			}

		break;

		default:
			handled = UUcFalse;
		break;
	}

	if (job_has_changed) {
		UUtBool has_job_data = AIrJob_HasJobData(object_specific_data.osd.character_osd.job);
		UUtBool has_job_select = AIrJob_HasJobSelectButton(object_specific_data.osd.character_osd.job);

		WMrWindow_SetVisible(job_data_prompt, has_job_data);
		WMrWindow_SetVisible(job_data_edit, has_job_data);
		WMrWindow_SetVisible(job_data_button, has_job_select );

		WMrWindow_SetTitle(job_data_description, "", WMcMaxTitleLength);

		if (has_job_data) {
			WMrEditField_SetInt32(job_data_edit, object_specific_data.osd.character_osd.job_specific_id);
		}

		job_data_changed = UUcTrue;
		dirty = UUcTrue;

		WMrWindow_SetTitle(job_data_description, "", WMcMaxTitleLength);
		 
		switch(object_specific_data.osd.character_osd.job)
		{
			case AI2cGoal_Patrol:
				WMrWindow_SetTitle(job_data_description, "Patrols along an AI path.", WMcMaxTitleLength);
				WMrWindow_SetTitle(job_data_prompt, "Path:", WMcMaxTitleLength);
				WMrWindow_SetTitle(job_data_button, "Select...", WMcMaxTitleLength);
			break;

			case AI2cGoal_Idle:
				WMrWindow_SetTitle(job_data_description, "Idle behavior.", WMcMaxTitleLength);
			break;

			case AI2cGoal_Guard:
				WMrWindow_SetTitle(job_data_description, "Guards at a flag [not yet implemented].", WMcMaxTitleLength);
				WMrWindow_SetTitle(job_data_prompt, "Flag:", WMcMaxTitleLength);
			break;

			default:
				WMrWindow_SetTitle(job_data_description, "[this state cannot be a job]", WMcMaxTitleLength);
			break;
		}
	}

	if (dirty) {
		OWrObjectProperties_SetOSD(inDialog, object, &object_specific_data);

		// setOSD may change our combat and melee settings, due to changing character class
		char_osd = (OBJtOSD_Character *) object->object_data;

		if (char_osd->combat_id != object_specific_data.osd.character_osd.combat_id) {
			object_specific_data.osd.character_osd.combat_id = char_osd->combat_id;
			combat_data_changed = UUcTrue;
		}

		if (char_osd->melee_id != object_specific_data.osd.character_osd.melee_id) {
			object_specific_data.osd.character_osd.melee_id = char_osd->melee_id;
			melee_data_changed = UUcTrue;
		}
	}
	
	if (job_data_changed) {
		char job_data_changed_buffer[256];

		switch(object_specific_data.osd.character_osd.job)
		{
			case AI2cGoal_Patrol:
				{
					OBJtOSD_PatrolPath path;
					UUtBool path_found;

					path_found = ONrLevel_Path_ID_To_Path(object_specific_data.osd.character_osd.job_specific_id, &path);

					if (path_found) {
						sprintf(job_data_changed_buffer, "Path: %s (ID %d)", path.name, object_specific_data.osd.character_osd.job_specific_id);
					}
					else {
						sprintf(job_data_changed_buffer, "Path: <none>");
						if (object_specific_data.osd.character_osd.job_specific_id != (UUtUns16) -1) {
							sprintf(string, " (ID #%d not found)", object_specific_data.osd.character_osd.job_specific_id);
							strcat(job_data_changed_buffer, string);
						}
					}

					WMrWindow_SetTitle(job_data_description, job_data_changed_buffer, WMcMaxTitleLength);
				}
			break;
		}
	}

	if (combat_data_changed) {
		OBJtOSD_Combat combat;
		char combat_description_buffer[256];
		UUtBool combat_found; 

		combat_found = ONrLevel_Combat_ID_To_Combat(object_specific_data.osd.character_osd.combat_id, &combat);

		if (combat_found) {
			sprintf(combat_description_buffer, "Combat: %s", combat.name);
			if (AI2gDebugShowSettingIDs) {
				sprintf(string, " (ID %d)", object_specific_data.osd.character_osd.combat_id);
				strcat(combat_description_buffer, string);
			}
		}
		else {
			sprintf(combat_description_buffer, "Combat: <none>");
			if (AI2gDebugShowSettingIDs && (object_specific_data.osd.character_osd.combat_id != (UUtUns16) -1)) {
				sprintf(string, " (ID #%d not found)", object_specific_data.osd.character_osd.combat_id);
				strcat(combat_description_buffer, string);
			}
		}

		WMrWindow_SetTitle(combat_description, combat_description_buffer, WMcMaxTitleLength);
	}

	if (melee_data_changed) {
		AI2tMeleeProfile *profile;
		char melee_description_buffer[256];
		UUtBool melee_found; 

		melee_found = ONrLevel_Melee_ID_To_Melee_Ptr(object_specific_data.osd.character_osd.melee_id, &profile);

		if (melee_found) {
			sprintf(melee_description_buffer, "Melee: %s", profile->name);
			if (AI2gDebugShowSettingIDs) {
				sprintf(string, " (ID %d)", object_specific_data.osd.character_osd.melee_id);
				strcat(melee_description_buffer, string);
			}
		}
		else {
			sprintf(melee_description_buffer, "Melee: <none>");
			if (AI2gDebugShowSettingIDs && (object_specific_data.osd.character_osd.melee_id != (UUtUns16) -1)) {
				sprintf(string, " (ID #%d not found)", object_specific_data.osd.character_osd.melee_id);
				strcat(melee_description_buffer, string);
			}
		}

		WMrWindow_SetTitle(melee_description, melee_description_buffer, WMcMaxTitleLength);
	}

	if (neutral_data_changed) {
		OBJtOSD_Neutral neutral;
		char neutral_description_buffer[256];
		UUtBool neutral_found; 

		neutral_found = ONrLevel_Neutral_ID_To_Neutral(object_specific_data.osd.character_osd.neutral_id, &neutral);

		if (neutral_found) {
			sprintf(neutral_description_buffer, "Neutral: %s", neutral.name);
			if (AI2gDebugShowSettingIDs) {
				sprintf(string, " (ID %d)", object_specific_data.osd.character_osd.neutral_id);
				strcat(neutral_description_buffer, string);
			}
		}
		else {
			sprintf(neutral_description_buffer, "Neutral: <none>");
			if (AI2gDebugShowSettingIDs && (object_specific_data.osd.character_osd.neutral_id != (UUtUns16) -1)) {
				sprintf(string, " (ID #%d not found)", object_specific_data.osd.character_osd.neutral_id);
				strcat(neutral_description_buffer, string);
			}
		}

		WMrWindow_SetTitle(neutral_description, neutral_description_buffer, WMcMaxTitleLength);
	}

	return handled;
}

#endif