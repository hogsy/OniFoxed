#if TOOL_VERSION
/*
	FILE:	Oni_Win_AI_Neutral.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: July 13, 2000
	
	PURPOSE: AI Neutral Behavior Windows
	
	Copyright (c) Bungie Software 2000

*/

#include "Oni_Character.h"
#include "Oni_Win_AI.h"
#include "Oni_Win_AI_Neutral.h"
#include "Oni_Win_Tools.h"
#include "Oni_Object.h"
#include "WM_Menu.h"
#include "WM_Text.h"
#include "WM_EditField.h"
#include "WM_PopupMenu.h"
#include "WM_CheckBox.h"
#include "Oni_AI2.h"
#include "Oni_Sound2.h"
#include "Oni_Win_Sound2.h"

static void OWiEditNeutral_FillLineBox(OBJtOSD_Neutral *inOSD, UUtUns32 inCurrentLine, WMtListBox *inListBox,
										UUtBool inDesireNotification)
{
	UUtUns32 itr;
	char buffer[128], temp[64];

	WMrListBox_Reset(inListBox);
	for (itr = 0; itr < inOSD->num_lines; itr++) {
		if (inOSD->lines[itr].flags & AI2cNeutralLineFlag_PlayerLine) {
			strcpy(buffer, "Player: ");
		} else {
			strcpy(buffer, "AI: ");
		}

		sprintf(temp, "\"%s\"", inOSD->lines[itr].sound);
		strcat(buffer, temp);

		if (inOSD->lines[itr].anim_type != ONcAnimType_None) {
			sprintf(temp, ", %s%s", ONrAnimTypeToString(inOSD->lines[itr].anim_type), (inOSD->lines[itr].flags & AI2cNeutralLineFlag_AnimOnce) ? " (once)" : "");
			strcat(buffer, temp);
		}

		if (inOSD->lines[itr].other_anim_type != ONcAnimType_None) {
			sprintf(temp, ", other: %s%s", ONrAnimTypeToString(inOSD->lines[itr].other_anim_type), (inOSD->lines[itr].flags & AI2cNeutralLineFlag_OtherAnimOnce) ? " (once)" : "");
			strcat(buffer, temp);
		}

		if (inOSD->lines[itr].flags & AI2cNeutralLineFlag_GiveItem) {
			strcat(buffer, ", Give Items");
		}

		WMrListBox_AddString(inListBox, buffer);
	}

	WMrListBox_SetSelection(inListBox, inDesireNotification, inCurrentLine);
}

static void OWiEditNeutral_SetupLineData(OBJtOSD_NeutralLine *inLine, WMtText *inSpeechText, WMtPopupMenu *inAnim, WMtPopupMenu *inOtherAnim,
										 WMtCheckBox *inPlayer, WMtCheckBox *inGiveItem, WMtCheckBox *inAnimOnce, WMtCheckBox *inOtherAnimOnce)
{
	char buffer[128];
	UUtBool checked;

	sprintf(buffer, "Speech: %s", inLine->sound);
	WMrWindow_SetTitle(inSpeechText, buffer, WMcMaxTitleLength);

	WMrPopupMenu_SetSelection(inAnim, inLine->anim_type);

	WMrPopupMenu_SetSelection(inOtherAnim, inLine->other_anim_type);

	checked = ((inLine->flags & AI2cNeutralLineFlag_PlayerLine) > 0);
	WMrCheckBox_SetCheck(inPlayer, checked);

	checked = ((inLine->flags & AI2cNeutralLineFlag_GiveItem) > 0);
	WMrCheckBox_SetCheck(inGiveItem, checked);

	checked = ((inLine->flags & AI2cNeutralLineFlag_AnimOnce) > 0);
	WMrCheckBox_SetCheck(inAnimOnce, checked);

	checked = ((inLine->flags & AI2cNeutralLineFlag_OtherAnimOnce) > 0);
	WMrCheckBox_SetCheck(inOtherAnimOnce, checked);
}


UUtBool
OWrEditNeutral_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	enum
	{
		cEditNeutral_Name				= 100,
		cEditNeutral_TriggerSpeechText	= 101,
		cEditNeutral_TriggerSpeechBtn	= 102,
		cEditNeutral_AbortSpeechText	= 103,
		cEditNeutral_AbortSpeechBtn		= 104,
		cEditNeutral_SuccessScript		= 105,
		cEditNeutral_EnemySpeechText	= 106,
		cEditNeutral_EnemySpeechBtn		= 107,

		cEditNeutral_RewardWeapon		= 150,
		cEditNeutral_RewardBallistic	= 151,
		cEditNeutral_RewardEnergy		= 152,
		cEditNeutral_RewardHypos		= 153,
		cEditNeutral_RewardShield		= 154,
		cEditNeutral_RewardInvisibility	= 155,
		cEditNeutral_RewardLSI			= 156,

		cEditNeutral_FlagNoResume		= 160,
		cEditNeutral_FlagNoResumeAfterGive= 161,
		cEditNeutral_FlagUninterruptable= 162,

		cEditNeutral_TriggerRange		= 200,
		cEditNeutral_TalkRange			= 201,
		cEditNeutral_FollowRange		= 202,
		cEditNeutral_EnemyRange			= 203,

		cEditNeutral_LineBox			= 300,
		cEditNeutral_LineNew			= 301,
		cEditNeutral_LineDel			= 302,
		cEditNeutral_LineUp				= 303,
		cEditNeutral_LineDown			= 304,

		cEditNeutral_LineSpeechText		= 400,
		cEditNeutral_LineSpeechBtn		= 401,
		cEditNeutral_LineAnim			= 402,
		cEditNeutral_LineIsPlayer		= 403,
		cEditNeutral_LineGiveItem		= 404,
		cEditNeutral_LineAnimOnce		= 405,
		cEditNeutral_LineOtherAnim		= 406,
		cEditNeutral_LineOtherAnimOnce	= 407
	};

	WMtEditField *name_edit				= WMrDialog_GetItemByID(inDialog, cEditNeutral_Name);
	WMtText      *trigger_text			= WMrDialog_GetItemByID(inDialog, cEditNeutral_TriggerSpeechText);
	WMtText      *abort_text			= WMrDialog_GetItemByID(inDialog, cEditNeutral_AbortSpeechText);
	WMtText      *enemy_text			= WMrDialog_GetItemByID(inDialog, cEditNeutral_EnemySpeechText);
	WMtEditField *success_edit			= WMrDialog_GetItemByID(inDialog, cEditNeutral_SuccessScript);

	WMtPopupMenu *reward_weapon			= WMrDialog_GetItemByID(inDialog, cEditNeutral_RewardWeapon);
	WMtEditField *reward_ballistic		= WMrDialog_GetItemByID(inDialog, cEditNeutral_RewardBallistic);
	WMtEditField *reward_energy			= WMrDialog_GetItemByID(inDialog, cEditNeutral_RewardEnergy);
	WMtEditField *reward_hypos			= WMrDialog_GetItemByID(inDialog, cEditNeutral_RewardHypos);
	WMtCheckBox  *reward_shield			= WMrDialog_GetItemByID(inDialog, cEditNeutral_RewardShield);
	WMtCheckBox  *reward_invisibility	= WMrDialog_GetItemByID(inDialog, cEditNeutral_RewardInvisibility);
	WMtCheckBox  *reward_lsi			= WMrDialog_GetItemByID(inDialog, cEditNeutral_RewardLSI);

	WMtEditField *trigger_range_edit	= WMrDialog_GetItemByID(inDialog, cEditNeutral_TriggerRange);
	WMtEditField *talk_range_edit		= WMrDialog_GetItemByID(inDialog, cEditNeutral_TalkRange);
	WMtEditField *follow_range_edit		= WMrDialog_GetItemByID(inDialog, cEditNeutral_FollowRange);
	WMtEditField *enemy_range_edit		= WMrDialog_GetItemByID(inDialog, cEditNeutral_EnemyRange);

	WMtCheckBox  *flag_noresume			= WMrDialog_GetItemByID(inDialog, cEditNeutral_FlagNoResume);
	WMtCheckBox  *flag_noresumeaftergive= WMrDialog_GetItemByID(inDialog, cEditNeutral_FlagNoResumeAfterGive);
	WMtCheckBox  *flag_uninterruptable	= WMrDialog_GetItemByID(inDialog, cEditNeutral_FlagUninterruptable);

	WMtListBox   *line_box				= WMrDialog_GetItemByID(inDialog, cEditNeutral_LineBox);

	WMtText      *linespeech_text		= WMrDialog_GetItemByID(inDialog, cEditNeutral_LineSpeechText);
	WMtPopupMenu *lineanim				= WMrDialog_GetItemByID(inDialog, cEditNeutral_LineAnim);
	WMtPopupMenu *lineotheranim			= WMrDialog_GetItemByID(inDialog, cEditNeutral_LineOtherAnim);
	WMtCheckBox  *lineplayer			= WMrDialog_GetItemByID(inDialog, cEditNeutral_LineIsPlayer);
	WMtCheckBox  *linegive				= WMrDialog_GetItemByID(inDialog, cEditNeutral_LineGiveItem);
	WMtCheckBox  *lineanimonce			= WMrDialog_GetItemByID(inDialog, cEditNeutral_LineAnimOnce);
	WMtCheckBox  *lineotheranimonce		= WMrDialog_GetItemByID(inDialog, cEditNeutral_LineOtherAnimOnce);
	UUtBool		handled = UUcTrue;

	OBJtObject			*object = (OBJtObject *) WMrDialog_GetUserData(inDialog);
	OBJtOSD_All			object_specific_data;
	OBJtOSD_Neutral		*neutral_osd;
	OBJtOSD_NeutralLine	swap_line;
	char				buffer[128];
	UUtBool				checked, dirty = UUcFalse, refill_listbox = UUcFalse, notify_on_refill;
	SStAmbient			*new_sound;
	static UUtUns32		current_line;			// ugly - would be better stored in user data but unfortunately userdata
												// is constrained by oni_win_tools to be a pointer to the object, no room
	
	OWtSelectResult		result;
	
	OBJrObject_GetObjectSpecificData(object, &object_specific_data);
	neutral_osd = &object_specific_data.neutral_osd;

	switch (inMessage)
	{
		case WMcDialogItem_OK:
			WMrDialog_ModalEnd(inDialog, UUcTrue);
		break;

		case WMcDialogItem_Cancel:
			WMrDialog_ModalEnd(inDialog, UUcFalse);
		break;

		case WMcMessage_InitDialog:
			WMrEditField_SetText(name_edit, neutral_osd->name);

			sprintf(buffer, "Trigger Speech: %s", (neutral_osd->trigger_sound[0] == '\0') ? "<none>" : neutral_osd->trigger_sound);
			WMrWindow_SetTitle(trigger_text, buffer, WMcMaxTitleLength);

			sprintf(buffer, "Abort Speech: %s", (neutral_osd->abort_sound[0] == '\0') ? "<none>" : neutral_osd->abort_sound);
			WMrWindow_SetTitle(abort_text, buffer, WMcMaxTitleLength);

			sprintf(buffer, "Enemy Speech: %s", (neutral_osd->enemy_sound[0] == '\0') ? "<none>" : neutral_osd->enemy_sound);
			WMrWindow_SetTitle(enemy_text, buffer, WMcMaxTitleLength);

			WMrEditField_SetText(success_edit, neutral_osd->end_script);


			OWrProp_BuildWeaponMenu(reward_weapon, neutral_osd->give_weaponclass);
			WMrEditField_SetInt32(reward_ballistic, neutral_osd->give_ballistic_ammo);
			WMrEditField_SetInt32(reward_energy, neutral_osd->give_energy_ammo);
			WMrEditField_SetInt32(reward_hypos, neutral_osd->give_hypos);
			WMrCheckBox_SetCheck(reward_shield, ((neutral_osd->give_flags & AI2cNeutralGive_Shield) > 0));
			WMrCheckBox_SetCheck(reward_invisibility, ((neutral_osd->give_flags & AI2cNeutralGive_Invisibility) > 0));
			WMrCheckBox_SetCheck(reward_lsi, ((neutral_osd->give_flags & AI2cNeutralGive_LSI) > 0));

			WMrEditField_SetFloat(trigger_range_edit, neutral_osd->trigger_range);
			WMrEditField_SetFloat(talk_range_edit, neutral_osd->conversation_range);
			WMrEditField_SetFloat(follow_range_edit, neutral_osd->follow_range);
			WMrEditField_SetFloat(enemy_range_edit, neutral_osd->abort_enemy_range);

			WMrCheckBox_SetCheck(flag_noresume, ((neutral_osd->flags & AI2cNeutralBehaviorFlag_NoResume) > 0));
			WMrCheckBox_SetCheck(flag_noresumeaftergive, ((neutral_osd->flags & AI2cNeutralBehaviorFlag_NoResumeAfterItems) > 0));
			WMrCheckBox_SetCheck(flag_uninterruptable, ((neutral_osd->flags & AI2cNeutralBehaviorFlag_Uninterruptable) > 0));

			current_line = 0;
			refill_listbox = UUcTrue;
			notify_on_refill = UUcTrue;
		break;

		case WMcMessage_MenuCommand:
			{
				UUtUns16 menu_window_id = WMrWindow_GetID((WMtMenu *) inParam2);
				UUtUns16 menu_item = UUmLowWord(inParam1);

				dirty = UUcTrue;
				switch(menu_window_id) {
					case cEditNeutral_RewardWeapon:
						WMrPopupMenu_GetItemText(reward_weapon, menu_item, neutral_osd->give_weaponclass);
					break;

					case cEditNeutral_LineAnim:
						// if we aren't on a valid line then this field should be disabled
						UUmAssert((current_line >= 0) && (current_line < neutral_osd->num_lines));

						WMrPopupMenu_GetItemID(lineanim, (UUtUns16) -1, &neutral_osd->lines[current_line].anim_type);
						refill_listbox = UUcTrue;
						notify_on_refill = UUcFalse;
					break;

					case cEditNeutral_LineOtherAnim:
						// if we aren't on a valid line then this field should be disabled
						UUmAssert((current_line >= 0) && (current_line < neutral_osd->num_lines));

						WMrPopupMenu_GetItemID(lineotheranim, (UUtUns16) -1, &neutral_osd->lines[current_line].other_anim_type);
						refill_listbox = UUcTrue;
						notify_on_refill = UUcFalse;
					break;

					default:
						dirty = UUcFalse;
						handled = UUcFalse;
					break;
				}
			}
		break;

		case WMcMessage_Command:
			dirty = UUcTrue;

			switch(UUmLowWord(inParam1)) 
			{
				case cEditNeutral_Name:
					WMrEditField_GetText(name_edit, neutral_osd->name, sizeof(neutral_osd->name));
				break;

				case cEditNeutral_TriggerSpeechBtn:
					new_sound = OSrAmbient_GetByName(neutral_osd->trigger_sound);
					result = OWrSelect_AmbientSound(&new_sound);
					if (result == OWcSelectResult_Cancel) {
						dirty = UUcFalse;
						break;
					}
					
					if (new_sound == NULL) {
						neutral_osd->trigger_sound[0] = '\0';
					} else {
						UUrString_Copy(neutral_osd->trigger_sound, new_sound->ambient_name, SScMaxNameLength);
					}
					sprintf(buffer, "Trigger Speech: %s", (neutral_osd->trigger_sound[0] == '\0') ? "<none>" : neutral_osd->trigger_sound);
					WMrWindow_SetTitle(trigger_text, buffer, WMcMaxTitleLength);
				break;

				case cEditNeutral_AbortSpeechBtn:
					new_sound = OSrAmbient_GetByName(neutral_osd->abort_sound);
					result = OWrSelect_AmbientSound(&new_sound);
					if (result == OWcSelectResult_Cancel) {
						dirty = UUcFalse;
						break;
					}

					if (new_sound == NULL) {
						neutral_osd->abort_sound[0] = '\0';
					} else {
						UUrString_Copy(neutral_osd->abort_sound, new_sound->ambient_name, SScMaxNameLength);
					}
					sprintf(buffer, "Abort Speech: %s", (neutral_osd->abort_sound[0] == '\0') ? "<none>" : neutral_osd->abort_sound);
					WMrWindow_SetTitle(abort_text, buffer, WMcMaxTitleLength);
				break;

				case cEditNeutral_EnemySpeechBtn:
					new_sound = OSrAmbient_GetByName(neutral_osd->enemy_sound);
					result = OWrSelect_AmbientSound(&new_sound);
					if (result == OWcSelectResult_Cancel) {
						dirty = UUcFalse;
						break;
					}

					if (new_sound == NULL) {
						neutral_osd->enemy_sound[0] = '\0';
					} else {
						UUrString_Copy(neutral_osd->enemy_sound, new_sound->ambient_name, SScMaxNameLength);
					}
					sprintf(buffer, "Enemy Speech: %s", (neutral_osd->enemy_sound[0] == '\0') ? "<none>" : neutral_osd->enemy_sound);
					WMrWindow_SetTitle(enemy_text, buffer, WMcMaxTitleLength);
				break;

				case cEditNeutral_SuccessScript:
					WMrEditField_GetText(success_edit, neutral_osd->end_script, sizeof(neutral_osd->end_script));
				break;

				case cEditNeutral_RewardBallistic:
					neutral_osd->give_ballistic_ammo = (UUtUns8) UUmMin(WMrEditField_GetInt32(reward_ballistic), UUcMaxUns8);
				break;

				case cEditNeutral_RewardEnergy:
					neutral_osd->give_energy_ammo = (UUtUns8) UUmMin(WMrEditField_GetInt32(reward_energy), UUcMaxUns8);
				break;

				case cEditNeutral_RewardHypos:
					neutral_osd->give_hypos = (UUtUns8) UUmMin(WMrEditField_GetInt32(reward_hypos), UUcMaxUns8);
				break;

				case cEditNeutral_RewardShield:
					if (WMrCheckBox_GetCheck(reward_shield)) {
						neutral_osd->give_flags |= AI2cNeutralGive_Shield;
					} else {
						neutral_osd->give_flags &= ~AI2cNeutralGive_Shield;
					}
				break;

				case cEditNeutral_RewardInvisibility:
					if (WMrCheckBox_GetCheck(reward_invisibility)) {
						neutral_osd->give_flags |= AI2cNeutralGive_Invisibility;
					} else {
						neutral_osd->give_flags &= ~AI2cNeutralGive_Invisibility;
					}
				break;

				case cEditNeutral_RewardLSI:
					if (WMrCheckBox_GetCheck(reward_lsi)) {
						neutral_osd->give_flags |= AI2cNeutralGive_LSI;
					} else {
						neutral_osd->give_flags &= ~AI2cNeutralGive_LSI;
					}
				break;

				case cEditNeutral_TriggerRange:
					neutral_osd->trigger_range = WMrEditField_GetFloat(trigger_range_edit);
				break;

				case cEditNeutral_TalkRange:
					neutral_osd->conversation_range = WMrEditField_GetFloat(talk_range_edit);
				break;

				case cEditNeutral_FollowRange:
					neutral_osd->follow_range = WMrEditField_GetFloat(follow_range_edit);
				break;

				case cEditNeutral_EnemyRange:
					neutral_osd->abort_enemy_range = WMrEditField_GetFloat(enemy_range_edit);
				break;

				case cEditNeutral_FlagNoResume:
					checked = WMrCheckBox_GetCheck(flag_noresume);
					if (checked) {
						neutral_osd->flags |= AI2cNeutralBehaviorFlag_NoResume;
					} else {
						neutral_osd->flags &= ~AI2cNeutralBehaviorFlag_NoResume;
					}
				break;

				case cEditNeutral_FlagNoResumeAfterGive:
					checked = WMrCheckBox_GetCheck(flag_noresumeaftergive);
					if (checked) {
						neutral_osd->flags |= AI2cNeutralBehaviorFlag_NoResumeAfterItems;
					} else {
						neutral_osd->flags &= ~AI2cNeutralBehaviorFlag_NoResumeAfterItems;
					}
				break;

				case cEditNeutral_FlagUninterruptable:
					checked = WMrCheckBox_GetCheck(flag_uninterruptable);
					if (checked) {
						neutral_osd->flags |= AI2cNeutralBehaviorFlag_Uninterruptable;
					} else {
						neutral_osd->flags &= ~AI2cNeutralBehaviorFlag_Uninterruptable;
					}
				break;

				case cEditNeutral_LineBox:
					current_line = WMrListBox_GetSelection(line_box);
					if (current_line == (UUtUns32) -1) {
						WMrWindow_SetEnabled(linespeech_text, UUcFalse);
						WMrWindow_SetEnabled(lineanim, UUcFalse);
						WMrWindow_SetEnabled(lineplayer, UUcFalse);
						WMrWindow_SetEnabled(linegive, UUcFalse);
					} else {
						WMrWindow_SetEnabled(linespeech_text, UUcTrue);
						WMrWindow_SetEnabled(lineanim, UUcTrue);
						WMrWindow_SetEnabled(lineplayer, UUcTrue);
						WMrWindow_SetEnabled(linegive, UUcTrue);

						current_line = UUmPin(current_line, 0, (UUtUns32) (neutral_osd->num_lines - 1));
						OWiEditNeutral_SetupLineData(&neutral_osd->lines[current_line],
													linespeech_text, lineanim, lineotheranim,
													lineplayer, linegive, lineanimonce, lineotheranimonce);
					}
				break;

				case cEditNeutral_LineNew:
					if (neutral_osd->num_lines >= OBJcNeutral_MaxLines) {
						char errmsg[256];

						sprintf(errmsg, "This neutral behavior already has the maximum number of lines (%d). Sorry.",
								OBJcNeutral_MaxLines);
						WMrDialog_MessageBox(inDialog, "Cannot Add Line", errmsg, WMcMessageBoxStyle_OK);
						break;
					}

					current_line = neutral_osd->num_lines;
					UUrMemory_Clear(&neutral_osd->lines[neutral_osd->num_lines], sizeof(OBJtOSD_NeutralLine));
					// TEMPORARY DEBUGGING.... until sound selection is working
					UUrString_Copy(neutral_osd->lines[neutral_osd->num_lines].sound, "talking", sizeof(neutral_osd->lines[neutral_osd->num_lines].sound));
					neutral_osd->num_lines += 1;

					refill_listbox = UUcTrue;
					notify_on_refill = UUcTrue;
				break;

				case cEditNeutral_LineDel:
					if ((current_line < 0) || (current_line >= neutral_osd->num_lines)) {
						// no line to delete
						break;
					}

					if (current_line + 1 < neutral_osd->num_lines) {
						UUrMemory_MoveOverlap(&neutral_osd->lines[current_line + 1], &neutral_osd->lines[current_line],
									(neutral_osd->num_lines - (current_line + 1)) * sizeof(OBJtOSD_NeutralLine));
					} else if (current_line > 0) {
						current_line--;
					}
					neutral_osd->num_lines--;

					refill_listbox = UUcTrue;
					notify_on_refill = UUcTrue;
				break;

				case cEditNeutral_LineUp:
					if ((current_line < 1) || (current_line >= neutral_osd->num_lines)) {
						// can't move this line up
						break;
					}

					swap_line = neutral_osd->lines[current_line - 1];
					neutral_osd->lines[current_line - 1] = neutral_osd->lines[current_line];
					neutral_osd->lines[current_line] = swap_line;
					current_line -= 1;

					refill_listbox = UUcTrue;
					notify_on_refill = UUcTrue;
				break;

				case cEditNeutral_LineDown:
					if ((current_line < 0) || (current_line + 1 >= neutral_osd->num_lines)) {
						// can't move this line down
						break;
					}

					swap_line = neutral_osd->lines[current_line + 1];
					neutral_osd->lines[current_line + 1] = neutral_osd->lines[current_line];
					neutral_osd->lines[current_line] = swap_line;
					current_line += 1;

					refill_listbox = UUcTrue;
					notify_on_refill = UUcTrue;
				break;

				case cEditNeutral_LineSpeechBtn:
					new_sound = OSrAmbient_GetByName(neutral_osd->lines[current_line].sound);
					result = OWrSelect_AmbientSound(&new_sound);
					if (result == OWcSelectResult_Cancel) {
						dirty = UUcFalse;
						break;
					}

					if (new_sound == NULL) {
						neutral_osd->lines[current_line].sound[0] = '\0';
					} else {
						UUrString_Copy(neutral_osd->lines[current_line].sound, new_sound->ambient_name, SScMaxNameLength);
					}
					sprintf(buffer, "Speech: %s", (neutral_osd->lines[current_line].sound[0] == '\0') ? "<none>" : neutral_osd->lines[current_line].sound);
					WMrWindow_SetTitle(linespeech_text, buffer, WMcMaxTitleLength);

					refill_listbox = UUcTrue;
					notify_on_refill = UUcFalse;
				break;

				case cEditNeutral_LineIsPlayer:
					// if we aren't on a valid line then this field should be disabled
					UUmAssert((current_line >= 0) && (current_line < neutral_osd->num_lines));

					checked = WMrCheckBox_GetCheck(lineplayer);
					if (checked) {
						neutral_osd->lines[current_line].flags |= AI2cNeutralLineFlag_PlayerLine;
					} else {
						neutral_osd->lines[current_line].flags &= ~AI2cNeutralLineFlag_PlayerLine;
					}
					refill_listbox = UUcTrue;
					notify_on_refill = UUcFalse;
				break;

				case cEditNeutral_LineGiveItem:
					// if we aren't on a valid line then this field should be disabled
					UUmAssert((current_line >= 0) && (current_line < neutral_osd->num_lines));

					checked = WMrCheckBox_GetCheck(linegive);
					if (checked) {
						neutral_osd->lines[current_line].flags |= AI2cNeutralLineFlag_GiveItem;
					} else {
						neutral_osd->lines[current_line].flags &= ~AI2cNeutralLineFlag_GiveItem;
					}
					refill_listbox = UUcTrue;
					notify_on_refill = UUcFalse;
				break;

				case cEditNeutral_LineAnimOnce:
					// if we aren't on a valid line then this field should be disabled
					UUmAssert((current_line >= 0) && (current_line < neutral_osd->num_lines));

					checked = WMrCheckBox_GetCheck(lineanimonce);
					if (checked) {
						neutral_osd->lines[current_line].flags |= AI2cNeutralLineFlag_AnimOnce;
					} else {
						neutral_osd->lines[current_line].flags &= ~AI2cNeutralLineFlag_AnimOnce;
					}
					refill_listbox = UUcTrue;
					notify_on_refill = UUcFalse;
				break;

				case cEditNeutral_LineOtherAnimOnce:
					// if we aren't on a valid line then this field should be disabled
					UUmAssert((current_line >= 0) && (current_line < neutral_osd->num_lines));

					checked = WMrCheckBox_GetCheck(lineotheranimonce);
					if (checked) {
						neutral_osd->lines[current_line].flags |= AI2cNeutralLineFlag_OtherAnimOnce;
					} else {
						neutral_osd->lines[current_line].flags &= ~AI2cNeutralLineFlag_OtherAnimOnce;
					}
					refill_listbox = UUcTrue;
					notify_on_refill = UUcFalse;
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

	if (refill_listbox) {
		OWiEditNeutral_FillLineBox(neutral_osd, current_line, line_box, notify_on_refill);
	}

	return handled;
}

UUtUns16 OWrChooseNeutral(UUtUns16 inNeutralID)
{
	OBJtObject *object = NULL;
	OBJtOSD_All	object_specific_data;
	UUtUns16 result = (UUtUns16) -1;

	object_specific_data.neutral_osd.id = inNeutralID;
	object = OBJrObjectType_Search(OBJcType_Neutral, OBJcSearch_NeutralID, &object_specific_data);
	
	object = OWrSelectObject(OBJcType_Neutral, object, UUcTrue, UUcFalse);

	if (object != NULL) {
		OBJrObject_GetObjectSpecificData(object, &object_specific_data);
		UUmAssert(object_specific_data.neutral_osd.id != 0);
		result = object_specific_data.neutral_osd.id;
	}

	return result;
}
#endif