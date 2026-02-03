#if TOOL_VERSION
/*
	FILE:	Oni_Win_AI_Melee.c

	AUTHOR:	Chris Butcher

	CREATED: May 20, 2000

	PURPOSE: AI Melee Profile Editor

	Copyright (c) Bungie Software 2000

*/

#include "Oni_Win_AI_Melee.h"
#include "Oni_Win_Tools.h"
#include "Oni_Object.h"
#include "WM_Menu.h"
#include "WM_Text.h"
#include "WM_EditField.h"
#include "WM_PopupMenu.h"
#include "Oni_AI2.h"
#include "Oni_AI2_MeleeProfile.h"
#include "Oni_Character.h"

enum
{
	OWcEditMelee_ID						= 1000,
	OWcEditMelee_Name					= 1001,

	OWcEditMelee_TechniqueMenu			= 1100,
	OWcEditMelee_Notice					= 1200,
	OWcEditMelee_DodgeBase				= 1201,
	OWcEditMelee_DmgAmount				= 1202,
	OWcEditMelee_DodgeExtra				= 1203,
	OWcEditMelee_BlockSkill				= 1204,
	OWcEditMelee_BlockGroup				= 1205,
	OWcEditMelee_ClassList				= 1300,

	OWcEditMelee_WeightNotBlocked		= 1400,
	OWcEditMelee_WeightChangeStance		= 1401,
	OWcEditMelee_WeightUnblockable		= 1402,
	OWcEditMelee_WeightHasStagger		= 1403,
	OWcEditMelee_WeightHasBlockStun		= 1404,
	OWcEditMelee_WeightBlocked			= 1405,

	OWcEditMelee_DazeMin				= 1500,
	OWcEditMelee_DazeMax				= 1501,

	OWcEditMelee_ThrowDanger			= 1600,

	OWcEditMelee_TechniqueListName		= 2000,
	OWcEditMelee_TechniqueList			= 2001,
	OWcEditMelee_TechniqueNew			= 2010,
	OWcEditMelee_TechniqueDel			= 2011,
	OWcEditMelee_TechniqueUp			= 2012,
	OWcEditMelee_TechniqueDown			= 2013,

	OWcEditMelee_MoveList				= 3001,
	OWcEditMelee_MoveNew				= 3010,
	OWcEditMelee_MoveDel				= 3011,
	OWcEditMelee_MoveUp					= 3012,
	OWcEditMelee_MoveDown				= 3013,

	OWcEditMelee_TechniqueName			= 3020,
	OWcEditMelee_TechniqueWeight		= 3021,
	OWcEditMelee_TechniqueDelay			= 3022,
	OWcEditMelee_TechniqueInterrupt		= 3023,
	OWcEditMelee_TechniqueGenerous		= 3024,
	OWcEditMelee_TechniqueFearless		= 3025,

	OWcEditMelee_MoveTypeMenu			= 4000,
	OWcEditMelee_MoveTypeList			= 4001,
	OWcEditMelee_MoveParam0Name			= 4010,
	OWcEditMelee_MoveParam0Field		= 4011,
	OWcEditMelee_MoveParam1Name			= 4012,
	OWcEditMelee_MoveParam1Field		= 4013,
	OWcEditMelee_MoveParam2Name			= 4014,
	OWcEditMelee_MoveParam2Field		= 4015
};

enum
{
	OWcEditMelee_TechniqueMenu_Actions	= 0,
	OWcEditMelee_TechniqueMenu_Reactions= 1,
	OWcEditMelee_TechniqueMenu_Spacing	= 2
};

typedef struct OWtEditMelee_UserData {
	OBJtObject				*object;

	// pointers to all of the child windows
	WMtEditField			*ef_id;
	WMtEditField			*ef_name;
	WMtPopupMenu			*pm_techniquemenu;
	WMtListBox				*lb_classlist;

	WMtEditField			*ef_notice;
	WMtEditField			*ef_dodgebase;
	WMtEditField			*ef_dmgamount;
	WMtEditField			*ef_dodgeextra;
	WMtEditField			*ef_blockskill;
	WMtEditField			*ef_blockgroup;

	WMtEditField			*ef_notblocked;
	WMtEditField			*ef_changestance;
	WMtEditField			*ef_unblockable;
	WMtEditField			*ef_hasstagger;
	WMtEditField			*ef_hasblockstun;
	WMtEditField			*ef_blocked;

	WMtEditField			*ef_throwdanger;

	WMtEditField			*ef_dazemin;
	WMtEditField			*ef_dazemax;

	WMtWindow				*box_techniquelistname;
	WMtListBox				*lb_techniquelist;

	WMtListBox				*lb_movelist;

	WMtEditField			*ef_techniquename;
	WMtEditField			*ef_techniqueweight;
	WMtEditField			*ef_techniquedelay;
	WMtWindow				*cb_techniqueinterrupt;
	WMtWindow				*cb_techniquegenerous;
	WMtWindow				*cb_techniquefearless;

	WMtPopupMenu			*pm_movetypemenu;
	WMtListBox				*lb_movetypelist;
	WMtWindow				*t_moveparam0;
	WMtEditField			*ef_moveparam0;
	WMtWindow				*t_moveparam1;
	WMtEditField			*ef_moveparam1;
	WMtWindow				*t_moveparam2;
	WMtEditField			*ef_moveparam2;

	// information about the data being displayed
	OBJtOSD_All				object_specific_data;
	AI2tMeleeProfile		*profile;
	UUtUns16				current_technique_list;
	AI2tMeleeTechnique		*technique;
	AI2tMeleeMove			*move;
	UUtUns16				current_movetype;
} OWtEditMelee_UserData;



static void
OWiEditMelee_FillTechniqueMoveList(WMtDialog *inDialog,
								   OWtEditMelee_UserData *inUserData);



static void
OWiEditMelee_FillMoveList(WMtDialog *inDialog,
								   OWtEditMelee_UserData *inUserData,
								   UUtUns32 inPrevMove)
{
	UUtUns32 iterated_move, num_moves, selected_move;
	AI2tMoveTypeFunction_Iterate iterate_func;
	AI2tMoveTypeFunction_GetName getname_func;
	AI2tMoveTypeFunction_CanFollow canfollow_func;
	AI2tMeleeMoveState current_state;
	char move_name[128], tempbuf[96];
	TRtAnimation *test_animation, *test_target;
	UUtBool found_name, move_valid;

	WMrListBox_Reset(inUserData->lb_movetypelist);

	if ((inUserData->current_movetype < 0) || (inUserData->current_movetype >= AI2cMeleeNumMoveTypes))
		return;

	if ((inUserData->move == NULL) || (inUserData->technique == NULL))
		return;

	// iterate over all valid moves of this type, adding them to the listbox
	iterate_func = AI2cMeleeMoveTypes[inUserData->current_movetype].func_iterate;
	getname_func = AI2cMeleeMoveTypes[inUserData->current_movetype].func_getname;
	canfollow_func = AI2cMeleeMoveTypes[inUserData->current_movetype].func_canfollow;
	iterated_move = (UUtUns32) -1;
	selected_move = (UUtUns32) -1;
	num_moves = 0;
	while (1) {
		iterated_move = iterate_func(iterated_move);

		if (iterated_move == (UUtUns32) -1) {
			// no more moves!
			break;
		}

		if (!canfollow_func(iterated_move, inPrevMove)) {
			// this move can't follow the previous one, don't consider it
			continue;
		}

		if (inUserData->move > &inUserData->profile->move[inUserData->technique->start_index]) {
			// this is not the first move of the technique
			current_state = inUserData->move->current_state;
		} else {
			// this is the first move of the technique
			current_state = AI2cMeleeMoveState_None;
		}

		// check to see if this would be a viable move here
		move_valid = AI2rMelee_MoveIsValid(inUserData->profile->char_class->animations, iterated_move,
											NULL, NULL, &current_state, NULL, &test_animation, &test_target, NULL);
		if (!move_valid) {
			continue;
		}

		found_name = AI2cMeleeMoveTypes[inUserData->current_movetype].func_getname(iterated_move, move_name, 128);
		if (!found_name) {
			sprintf(move_name, "<unknown %s move 0x%08X>", AI2cMeleeMoveTypes[inUserData->current_movetype].name,
					iterated_move);
		}

		if (test_animation != NULL) {
			sprintf(tempbuf, " (%s)", TMrInstance_GetInstanceName(test_animation));
			strcat(move_name, tempbuf);
		}

		// add the move to the listbox
		WMrListBox_AddString(inUserData->lb_movetypelist, move_name);
		WMrListBox_SetItemData(inUserData->lb_movetypelist, iterated_move, num_moves);

		if (iterated_move == inUserData->move->move)
			selected_move = num_moves;

		num_moves++;
	}

	// select the current move type, if it's there
	if (selected_move != (UUtUns32) -1) {
		WMrListBox_SetSelection(inUserData->lb_movetypelist, UUcFalse, selected_move);
	}
}

static void
OWiEditMelee_SetupMovePane(WMtDialog *inDialog,
						   OWtEditMelee_UserData *inUserData)
{
	UUtUns32 move_index, prev_move, num_params;
	char *stringptrs[3];

	if (inUserData->move == NULL) {
		// disable the move pane
		WMrWindow_SetEnabled(inUserData->pm_movetypemenu, UUcFalse);
		WMrWindow_SetEnabled(inUserData->lb_movetypelist, UUcFalse);

		// hide all parameters
		WMrWindow_SetVisible(inUserData->t_moveparam0, UUcFalse);
		WMrWindow_SetVisible(inUserData->ef_moveparam0, UUcFalse);
		WMrWindow_SetVisible(inUserData->t_moveparam1, UUcFalse);
		WMrWindow_SetVisible(inUserData->ef_moveparam1, UUcFalse);
		WMrWindow_SetVisible(inUserData->t_moveparam2, UUcFalse);
		WMrWindow_SetVisible(inUserData->ef_moveparam2, UUcFalse);

		// no current move type
		WMrPopupMenu_SetSelection(inUserData->pm_movetypemenu, (UUtUns16) -1);

		// empty the move list (will be emptied because inUserData->move is NULL)
		OWiEditMelee_FillMoveList(inDialog, inUserData, (UUtUns32) -1);
		inUserData->current_movetype = (UUtUns16) -1;

	} else {
		move_index = inUserData->move - inUserData->profile->move;
		UUmAssert((move_index >= 0) && (move_index < inUserData->profile->num_moves));

		// enable the move pane
		WMrWindow_SetEnabled(inUserData->pm_movetypemenu, UUcTrue);
		WMrWindow_SetEnabled(inUserData->lb_movetypelist, UUcTrue);

		// set the current move type
		inUserData->current_movetype = (UUtUns16) ((inUserData->move->move & AI2cMoveType_Mask) >> AI2cMoveType_Shift);
		WMrPopupMenu_SetSelection(inUserData->pm_movetypemenu, inUserData->current_movetype);

		// unhide as many parameters as appropriate, and set their values
		num_params = AI2cMeleeMoveTypes[inUserData->current_movetype].func_getparams(inUserData->move->move,
								(inUserData->current_technique_list == OWcEditMelee_TechniqueMenu_Spacing), stringptrs);

		if (num_params >= 1) {
			WMrWindow_SetVisible(inUserData->t_moveparam0, UUcTrue);
			WMrWindow_SetVisible(inUserData->ef_moveparam0, UUcTrue);

			WMrWindow_SetTitle(inUserData->t_moveparam0, stringptrs[0], WMcMaxTitleLength);
			WMrEditField_SetFloat(inUserData->ef_moveparam0, inUserData->move->param[0]);

			if (num_params >= 2) {
				WMrWindow_SetVisible(inUserData->t_moveparam1, UUcTrue);
				WMrWindow_SetVisible(inUserData->ef_moveparam1, UUcTrue);

				WMrWindow_SetTitle(inUserData->t_moveparam1, stringptrs[1], WMcMaxTitleLength);
				WMrEditField_SetFloat(inUserData->ef_moveparam1, inUserData->move->param[1]);

				if (num_params >= 3) {
					WMrWindow_SetVisible(inUserData->t_moveparam2, UUcTrue);
					WMrWindow_SetVisible(inUserData->ef_moveparam2, UUcTrue);

					WMrWindow_SetTitle(inUserData->t_moveparam2, stringptrs[2], WMcMaxTitleLength);
					WMrEditField_SetFloat(inUserData->ef_moveparam2, inUserData->move->param[2]);
				} else {
					WMrWindow_SetVisible(inUserData->t_moveparam2, UUcFalse);
					WMrWindow_SetVisible(inUserData->ef_moveparam2, UUcFalse);
				}
			} else {
				WMrWindow_SetVisible(inUserData->t_moveparam1, UUcFalse);
				WMrWindow_SetVisible(inUserData->ef_moveparam1, UUcFalse);
				WMrWindow_SetVisible(inUserData->t_moveparam2, UUcFalse);
				WMrWindow_SetVisible(inUserData->ef_moveparam2, UUcFalse);
			}
		} else {
			WMrWindow_SetVisible(inUserData->t_moveparam0, UUcFalse);
			WMrWindow_SetVisible(inUserData->ef_moveparam0, UUcFalse);
			WMrWindow_SetVisible(inUserData->t_moveparam1, UUcFalse);
			WMrWindow_SetVisible(inUserData->ef_moveparam1, UUcFalse);
			WMrWindow_SetVisible(inUserData->t_moveparam2, UUcFalse);
			WMrWindow_SetVisible(inUserData->ef_moveparam2, UUcFalse);
		}

		// work out what the previous move in the technique is
		UUmAssert(inUserData->technique != NULL);
		UUmAssert((move_index >= inUserData->technique->start_index) &&
				(move_index < inUserData->technique->start_index + inUserData->technique->num_moves));

		if (move_index == inUserData->technique->start_index) {
			prev_move = (UUtUns32) -1;
		} else {
			prev_move = inUserData->profile->move[move_index - 1].move;
		}

		// fill the move list with all moves that are valid considering our previous move
		OWiEditMelee_FillMoveList(inDialog, inUserData, prev_move);
	}
}

static void
OWiEditMelee_FillTechniqueMoveList(WMtDialog *inDialog,
								   OWtEditMelee_UserData *inUserData)
{
	AI2tMeleeMove *move;
	UUtUns32 itr, move_index, selected_move, prev_move, selected_prev_move;
	UUtUns16 selected_type, type_index;
	UUtBool found_name, type_valid;
	char move_name[64];

	WMrListBox_Reset(inUserData->lb_movelist);

	if (inUserData->technique == NULL)
		return;

	selected_move = (UUtUns32) -1;
	selected_type = (UUtUns16) -1;
	selected_prev_move = prev_move = (UUtUns32) -1;

	move_index = inUserData->technique->start_index;
	move = inUserData->profile->move + move_index;
	for (itr = 0; itr < inUserData->technique->num_moves; itr++, move++, move_index++) {
		UUmAssert((move_index >= 0) && (move_index < inUserData->profile->num_moves));

		found_name = type_valid = UUcFalse;

		// determine the type of the move
		type_index = (UUtUns16) ((move->move & AI2cMoveType_Mask) >> AI2cMoveType_Shift);
		if ((type_index >= 0) && (type_index < AI2cMeleeNumMoveTypes)) {
			found_name = AI2cMeleeMoveTypes[type_index].func_getname(move->move, move_name, 64);
			type_valid = UUcTrue;
		}

		if (!found_name) {
			if (type_valid) {
				sprintf(move_name, "<%s-0x%08X>", AI2cMeleeMoveTypes[type_index].name, move->move);
			} else {
				sprintf(move_name, "<type %d-0x%08X>", type_index, move->move);
			}
		}

		if (move->is_broken) {
			strcat(move_name, "**");
		}

		// add the move to the listbox
		WMrListBox_AddString(inUserData->lb_movelist, move_name);

		if (move == inUserData->move) {
			// this is the selected move
			selected_move = itr;
			selected_prev_move = prev_move;
			selected_type = type_index;
		}

		prev_move = move->move;
	}

	if (selected_move != (UUtUns32) -1) {
		// select a move in the technique
		WMrListBox_SetSelection(inUserData->lb_movelist, UUcFalse, selected_move);
	} else {
		// no selected move!
		inUserData->move = NULL;
	}

	OWiEditMelee_SetupMovePane(inDialog, inUserData);
}

static void
OWiEditMelee_SetupTechniquePane(WMtDialog *inDialog,
							   OWtEditMelee_UserData *inUserData)
{
	UUtBool checked;

	if (inUserData->technique == NULL) {
		// disable technique pane
		WMrWindow_SetEnabled(inUserData->ef_techniquename, UUcFalse);
		WMrWindow_SetEnabled(inUserData->ef_techniqueweight, UUcFalse);
		WMrWindow_SetEnabled(inUserData->ef_techniquedelay, UUcFalse);
		WMrWindow_SetEnabled(inUserData->cb_techniqueinterrupt, UUcFalse);
		WMrWindow_SetEnabled(inUserData->cb_techniquegenerous, UUcFalse);
		WMrWindow_SetEnabled(inUserData->cb_techniquefearless, UUcFalse);
		WMrWindow_SetEnabled(inUserData->lb_movelist, UUcFalse);

		// empty the fields
		WMrEditField_SetText(inUserData->ef_techniquename, "");
		WMrEditField_SetText(inUserData->ef_techniqueweight, "");
		WMrEditField_SetText(inUserData->ef_techniquedelay, "");
		WMrMessage_Send(inUserData->cb_techniqueinterrupt, CBcMessage_SetCheck, (UUtUns32) UUcFalse, (UUtUns32) -1);
		WMrMessage_Send(inUserData->cb_techniquegenerous, CBcMessage_SetCheck, (UUtUns32) UUcFalse, (UUtUns32) -1);
		WMrMessage_Send(inUserData->cb_techniquefearless, CBcMessage_SetCheck, (UUtUns32) UUcFalse, (UUtUns32) -1);
	} else {
		UUmAssert((inUserData->technique >= inUserData->profile->technique) &&
					(inUserData->technique < &inUserData->profile->technique[inUserData->profile->num_actions +
																			inUserData->profile->num_reactions +
																			inUserData->profile->num_spacing]));

		// enable technique pane
		WMrWindow_SetEnabled(inUserData->ef_techniquename, UUcTrue);
		WMrWindow_SetEnabled(inUserData->ef_techniqueweight, UUcTrue);
		WMrWindow_SetEnabled(inUserData->ef_techniquedelay, UUcTrue);
		WMrWindow_SetEnabled(inUserData->cb_techniqueinterrupt, UUcTrue);
		WMrWindow_SetEnabled(inUserData->cb_techniquegenerous, UUcTrue);
		WMrWindow_SetEnabled(inUserData->cb_techniquefearless, UUcTrue);
		WMrWindow_SetEnabled(inUserData->lb_movelist, UUcTrue);

		// set up the fields
		WMrEditField_SetText(inUserData->ef_techniquename, inUserData->technique->name);
		WMrEditField_SetInt32(inUserData->ef_techniqueweight, inUserData->technique->base_weight);
		WMrEditField_SetInt32(inUserData->ef_techniquedelay, inUserData->technique->delay_frames);

		checked = (inUserData->technique->user_flags & AI2cTechniqueUserFlag_Interruptable) ? UUcTrue : UUcFalse;
		WMrMessage_Send(inUserData->cb_techniqueinterrupt, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);

		checked = (inUserData->technique->user_flags & AI2cTechniqueUserFlag_GenerousDirection) ? UUcTrue : UUcFalse;
		WMrMessage_Send(inUserData->cb_techniquegenerous, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);

		checked = (inUserData->technique->user_flags & AI2cTechniqueUserFlag_Fearless) ? UUcTrue : UUcFalse;
		WMrMessage_Send(inUserData->cb_techniquefearless, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);
	}

	OWiEditMelee_FillTechniqueMoveList(inDialog, inUserData);

	// *** no current move because we've just changed techniques; clear the move pane
	inUserData->move = NULL;
	OWiEditMelee_SetupMovePane(inDialog, inUserData);
}

static void
OWiEditMelee_FillTechniqueList(WMtDialog *inDialog, OWtEditMelee_UserData *inUserData)
{
	UUtUns32 itr, start_index, end_index, string_index, selected_index;
	AI2tMeleeTechnique *technique;
	char item_text[64];
	UUtUns16 list_id;
	UUtBool selected;

	WMrListBox_Reset(inUserData->lb_techniquelist);

	// get the selected technique list type
	selected = WMrPopupMenu_GetItemID(inUserData->pm_techniquemenu, -1, &list_id);
	if (!selected)
		return;

	switch(list_id) {
		case OWcEditMelee_TechniqueMenu_Actions:
			start_index = 0;
			end_index = inUserData->profile->num_actions;

			WMrWindow_SetTitle(inUserData->box_techniquelistname, "Actions", WMcMaxTitleLength);
		break;

		case OWcEditMelee_TechniqueMenu_Reactions:
			start_index = inUserData->profile->num_actions;
			end_index = start_index + inUserData->profile->num_reactions;

			WMrWindow_SetTitle(inUserData->box_techniquelistname, "Reactions", WMcMaxTitleLength);
		break;

		case OWcEditMelee_TechniqueMenu_Spacing:
			start_index = inUserData->profile->num_actions + inUserData->profile->num_reactions;
			end_index = start_index + inUserData->profile->num_spacing;

			WMrWindow_SetTitle(inUserData->box_techniquelistname, "Spacing Behaviors", WMcMaxTitleLength);
		break;

		default:
			// unknown list type!
			return;
	}

	selected_index = (UUtUns32) -1;
	for (itr = start_index; itr < end_index; itr++) {
		char tempbuf[80];

		technique = inUserData->profile->technique + itr;

		sprintf(item_text, "[%02d", technique->base_weight);
		if (technique->delay_frames > 0) {
			sprintf(tempbuf, " d%d", technique->delay_frames);
			strcat(item_text, tempbuf);
		}
		strcat(item_text, "] ");
		strcat(item_text, technique->name);

		if (technique->computed_flags & AI2cTechniqueComputedFlag_Broken)
			strcat(item_text, "**");

		string_index = WMrListBox_AddString(inUserData->lb_techniquelist, item_text);

		if (technique == inUserData->technique)
			selected_index = string_index;
	}

	if (selected_index != (UUtUns32) -1) {
		WMrListBox_SetSelection(inUserData->lb_techniquelist, UUcFalse, selected_index);
	}
}

static UUtBool
OWiEditMelee_InitDialog(WMtDialog *inDialog)
{
	OWtEditMelee_UserData *user_data;
	ONtCharacterClass *char_class;
	UUtUns32 itr, num_classes, class_index;
	UUtError error;
	char *class_name;
	WMtMenuItemData menu_item;

	user_data = UUrMemory_Block_NewClear(sizeof(OWtEditMelee_UserData));
	if (user_data == NULL) {
		// can't allocate space for user data, bail out
		WMrDialog_SetUserData(inDialog, 0);
		WMrDialog_ModalEnd(inDialog, UUcFalse);
		return UUcTrue;
	}

	user_data->object = (OBJtObject *) WMrDialog_GetUserData(inDialog);
	WMrDialog_SetUserData(inDialog, (UUtUns32) user_data);

	// set up pointers to all our child windows
	user_data->ef_id					= WMrDialog_GetItemByID(inDialog, OWcEditMelee_ID);
	user_data->ef_name					= WMrDialog_GetItemByID(inDialog, OWcEditMelee_Name);
	user_data->pm_techniquemenu			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_TechniqueMenu);
	user_data->ef_notice				= WMrDialog_GetItemByID(inDialog, OWcEditMelee_Notice);
	user_data->ef_dodgebase				= WMrDialog_GetItemByID(inDialog, OWcEditMelee_DodgeBase);
	user_data->ef_dmgamount				= WMrDialog_GetItemByID(inDialog, OWcEditMelee_DmgAmount);
	user_data->ef_dodgeextra			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_DodgeExtra);
	user_data->ef_blockskill			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_BlockSkill);
	user_data->ef_blockgroup			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_BlockGroup);
	user_data->ef_notblocked			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_WeightNotBlocked);
	user_data->ef_changestance			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_WeightChangeStance);
	user_data->ef_unblockable			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_WeightUnblockable);
	user_data->ef_hasstagger			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_WeightHasStagger);
	user_data->ef_hasblockstun			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_WeightHasBlockStun);
	user_data->ef_blocked				= WMrDialog_GetItemByID(inDialog, OWcEditMelee_WeightBlocked);
	user_data->ef_throwdanger			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_ThrowDanger);
	user_data->ef_dazemin				= WMrDialog_GetItemByID(inDialog, OWcEditMelee_DazeMin);
	user_data->ef_dazemax				= WMrDialog_GetItemByID(inDialog, OWcEditMelee_DazeMax);
	user_data->lb_classlist				= WMrDialog_GetItemByID(inDialog, OWcEditMelee_ClassList);
	user_data->box_techniquelistname	= WMrDialog_GetItemByID(inDialog, OWcEditMelee_TechniqueListName);
	user_data->lb_techniquelist			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_TechniqueList);
	user_data->lb_movelist				= WMrDialog_GetItemByID(inDialog, OWcEditMelee_MoveList);
	user_data->ef_techniquename			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_TechniqueName);
	user_data->ef_techniqueweight		= WMrDialog_GetItemByID(inDialog, OWcEditMelee_TechniqueWeight);
	user_data->ef_techniquedelay		= WMrDialog_GetItemByID(inDialog, OWcEditMelee_TechniqueDelay);
	user_data->cb_techniqueinterrupt	= WMrDialog_GetItemByID(inDialog, OWcEditMelee_TechniqueInterrupt);
	user_data->cb_techniquegenerous		= WMrDialog_GetItemByID(inDialog, OWcEditMelee_TechniqueGenerous);
	user_data->cb_techniquefearless		= WMrDialog_GetItemByID(inDialog, OWcEditMelee_TechniqueFearless);
	user_data->pm_movetypemenu			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_MoveTypeMenu);
	user_data->lb_movetypelist			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_MoveTypeList);
	user_data->t_moveparam0				= WMrDialog_GetItemByID(inDialog, OWcEditMelee_MoveParam0Name);
	user_data->ef_moveparam0			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_MoveParam0Field);
	user_data->t_moveparam1				= WMrDialog_GetItemByID(inDialog, OWcEditMelee_MoveParam1Name);
	user_data->ef_moveparam1			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_MoveParam1Field);
	user_data->t_moveparam2				= WMrDialog_GetItemByID(inDialog, OWcEditMelee_MoveParam2Name);
	user_data->ef_moveparam2			= WMrDialog_GetItemByID(inDialog, OWcEditMelee_MoveParam2Field);

	// get the current melee profile
	OBJrObject_GetObjectSpecificData(user_data->object, &user_data->object_specific_data);

	// currently we aren't displaying anything
	user_data->profile = (AI2tMeleeProfile *) &user_data->object_specific_data.osd.melee_osd;
	user_data->technique = NULL;
	user_data->move = NULL;

	// prepare the profile for use
	error = AI2rMelee_PrepareForUse(user_data->profile);
	if (error != UUcError_None) {
		// can't edit this profile! bail out (user data will be deallocated by destroy code)
		WMrDialog_ModalEnd(inDialog, UUcFalse);
		return UUcTrue;
	}

	// fill the class list
	num_classes = TMrInstance_GetTagCount(TRcTemplate_CharacterClass);
	class_index = (UUtUns32) -1;
	for (itr = 0; itr < num_classes; itr++) {
		if (TMrInstance_GetDataPtr_ByNumber(TRcTemplate_CharacterClass, itr, &char_class) != UUcError_None) {
			break;
		}

		class_name = TMrInstance_GetInstanceName(char_class);
		WMrListBox_AddString(user_data->lb_classlist, class_name);

		if (UUmString_IsEqual(class_name, user_data->profile->char_classname)) {
			UUmAssert(user_data->profile->char_class == char_class);
			class_index = itr;
		}
	}
	WMrListBox_SetSelection(user_data->lb_classlist, UUcFalse, class_index);

	// fill the move-type menu
	for (itr = 0; itr < AI2cMeleeNumMoveTypes; itr++) {
		menu_item.flags = WMcMenuItemFlag_Enabled;
		menu_item.id = (UUtUns16) itr;
		UUrString_Copy(menu_item.title, AI2cMeleeMoveTypes[itr].name, 64);

		WMrPopupMenu_AppendItem(user_data->pm_movetypemenu, &menu_item);
	}
	user_data->current_movetype = (UUtUns16) -1;

	// *** set up technique parameters
	WMrEditField_SetText(user_data->ef_name, user_data->profile->name);
	WMrEditField_SetInt32(user_data->ef_id, user_data->profile->id);

	WMrEditField_SetInt32(user_data->ef_notice, user_data->profile->attacknotice_percentage);
	WMrEditField_SetInt32(user_data->ef_dodgebase, user_data->profile->dodge_base_percentage);
	WMrEditField_SetInt32(user_data->ef_dmgamount, user_data->profile->dodge_damage_threshold);
	WMrEditField_SetInt32(user_data->ef_dodgeextra, user_data->profile->dodge_additional_percentage);
	WMrEditField_SetInt32(user_data->ef_blockskill, user_data->profile->blockskill_percentage);
	WMrEditField_SetInt32(user_data->ef_blockgroup, user_data->profile->blockgroup_percentage);

	WMrEditField_SetFloat(user_data->ef_notblocked, user_data->profile->weight_notblocking_any);
	WMrEditField_SetFloat(user_data->ef_changestance, user_data->profile->weight_notblocking_changestance);
	WMrEditField_SetFloat(user_data->ef_unblockable, user_data->profile->weight_blocking_unblockable);
	WMrEditField_SetFloat(user_data->ef_hasstagger, user_data->profile->weight_blocking_stagger);
	WMrEditField_SetFloat(user_data->ef_hasblockstun, user_data->profile->weight_blocking_blockstun);
	WMrEditField_SetFloat(user_data->ef_blocked, user_data->profile->weight_blocking);

	WMrEditField_SetFloat(user_data->ef_throwdanger, user_data->profile->weight_throwdanger);

	WMrEditField_SetFloat(user_data->ef_dazemin, ((float) user_data->profile->dazed_minframes) / UUcFramesPerSecond);
	WMrEditField_SetFloat(user_data->ef_dazemax, ((float) user_data->profile->dazed_maxframes) / UUcFramesPerSecond);

	// *** fill the technique list with actions
	user_data->current_technique_list = OWcEditMelee_TechniqueMenu_Actions;
	WMrPopupMenu_SetSelection(user_data->pm_techniquemenu, user_data->current_technique_list);
	OWiEditMelee_FillTechniqueList(inDialog, user_data);

	// *** set up the technique pane (this will clear the move pane too)
	OWiEditMelee_SetupTechniquePane(inDialog, user_data);

	return UUcTrue;
}

static UUtBool
OWiEditMelee_MenuCommand(WMtDialog *inDialog,
						 OWtEditMelee_UserData *inUserData,
						 WMtMenu *inMenu,
						 UUtUns16 inItemID)
{
	UUtBool handled = UUcTrue;
	UUtUns16 menu_id = WMrWindow_GetID(inMenu);

	switch(menu_id) {
		case OWcEditMelee_TechniqueMenu:
			if (inItemID != inUserData->current_technique_list) {
				// we are switching our technique list view. no currently
				// selected technique or move.
				inUserData->current_technique_list = inItemID;

				inUserData->technique = NULL;
				inUserData->move = NULL;
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
				OWiEditMelee_SetupTechniquePane(inDialog, inUserData);
			}
		break;

		case OWcEditMelee_MoveTypeMenu:
			if (inItemID != inUserData->current_movetype) {
				UUtUns32 prev_move, move_index;

				// get our current move
				if (inUserData->move == NULL)
					break;

				move_index = inUserData->move - inUserData->profile->move;

				// work out what the previous move in the technique is
				if (inUserData->technique == NULL)
					break;

				UUmAssert((move_index >= inUserData->technique->start_index) &&
						(move_index < inUserData->technique->start_index + inUserData->technique->num_moves));

				if (move_index == inUserData->technique->start_index) {
					prev_move = (UUtUns32) -1;
				} else {
					prev_move = inUserData->profile->move[move_index - 1].move;
				}

				// switch to the new move type
				inUserData->current_movetype = inItemID;

				// fill the move list with all moves of the new type that are
				// valid considering our previous move
				OWiEditMelee_FillMoveList(inDialog, inUserData, prev_move);
			}
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

static UUtBool
OWiEditMelee_Command(WMtDialog *inDialog,
					 OWtEditMelee_UserData *inUserData,
					 UUtUns16 inWindowID)
{
	UUtBool handled = UUcTrue;
	UUtError error;
	UUtUns32 move_index, technique_index, total_techniques;
	AI2tMeleeTechnique *new_technique;
	AI2tMeleeMove *new_move;

	switch (inWindowID) {
		case WMcDialogItem_OK:
			// update the melee profile with our changes
			OWrObjectProperties_SetOSD(inDialog, inUserData->object, &inUserData->object_specific_data);
			WMrDialog_ModalEnd(inDialog, UUcTrue);
		break;

		case WMcDialogItem_Cancel:
			WMrDialog_ModalEnd(inDialog, UUcFalse);
		break;

		case OWcEditMelee_ID:
			inUserData->profile->id = WMrEditField_GetInt32(inUserData->ef_id);
		break;

		case OWcEditMelee_Name:
			WMrEditField_GetText(inUserData->ef_name, inUserData->profile->name, sizeof(inUserData->profile->name));
		break;

		case OWcEditMelee_Notice:
			inUserData->profile->attacknotice_percentage = WMrEditField_GetInt32(inUserData->ef_notice);
		break;

		case OWcEditMelee_DodgeBase:
			inUserData->profile->dodge_base_percentage = WMrEditField_GetInt32(inUserData->ef_dodgebase);
		break;

		case OWcEditMelee_DmgAmount:
			inUserData->profile->dodge_damage_threshold = WMrEditField_GetInt32(inUserData->ef_dmgamount);
		break;

		case OWcEditMelee_DodgeExtra:
			inUserData->profile->dodge_additional_percentage = WMrEditField_GetInt32(inUserData->ef_dodgeextra);
		break;

		case OWcEditMelee_BlockSkill:
			inUserData->profile->blockskill_percentage = WMrEditField_GetInt32(inUserData->ef_blockskill);
		break;

		case OWcEditMelee_BlockGroup:
			inUserData->profile->blockgroup_percentage = WMrEditField_GetInt32(inUserData->ef_blockgroup);
		break;

		case OWcEditMelee_WeightNotBlocked:
			inUserData->profile->weight_notblocking_any = WMrEditField_GetFloat(inUserData->ef_notblocked);
		break;

		case OWcEditMelee_WeightChangeStance:
			inUserData->profile->weight_notblocking_changestance = WMrEditField_GetFloat(inUserData->ef_changestance);
		break;

		case OWcEditMelee_WeightUnblockable:
			inUserData->profile->weight_blocking_unblockable = WMrEditField_GetFloat(inUserData->ef_unblockable);
		break;

		case OWcEditMelee_WeightHasStagger:
			inUserData->profile->weight_blocking_stagger = WMrEditField_GetFloat(inUserData->ef_hasstagger);
		break;

		case OWcEditMelee_WeightHasBlockStun:
			inUserData->profile->weight_blocking_blockstun = WMrEditField_GetFloat(inUserData->ef_hasblockstun);
		break;

		case OWcEditMelee_WeightBlocked:
			inUserData->profile->weight_blocking = WMrEditField_GetFloat(inUserData->ef_blocked);
		break;

		case OWcEditMelee_ThrowDanger:
			inUserData->profile->weight_throwdanger = WMrEditField_GetFloat(inUserData->ef_throwdanger);
		break;

		case OWcEditMelee_DazeMin:
			inUserData->profile->dazed_minframes = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(UUcFramesPerSecond * WMrEditField_GetFloat(inUserData->ef_dazemin));
		break;

		case OWcEditMelee_DazeMax:
			inUserData->profile->dazed_maxframes = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(UUcFramesPerSecond * WMrEditField_GetFloat(inUserData->ef_dazemax));
		break;


		case OWcEditMelee_ClassList:
			{
				UUtUns32 class_index = WMrListBox_GetSelection(inUserData->lb_classlist);
				char *new_name;
				ONtCharacterClass *new_class, *old_class;

				old_class = inUserData->profile->char_class;

				error = TMrInstance_GetDataPtr_ByNumber(TRcTemplate_CharacterClass, class_index, &new_class);
				if (error != UUcError_None)
					break;

				new_name = TMrInstance_GetInstanceName(new_class);
				UUrString_Copy(inUserData->profile->char_classname, new_name, sizeof(inUserData->profile->char_classname));

				// prepare the profile for use
				error = AI2rMelee_PrepareForUse(inUserData->profile);
				if (error != UUcError_None) {
					char errbuf[96];

					// the newly selected class is invalid
					sprintf(errbuf, "Error: couldn't find character class '%s'!", new_name);
					WMrDialog_MessageBox(inDialog, "Cannot change character class", errbuf, WMcMessageBoxStyle_OK);

					inUserData->profile->char_class = old_class;
					break;
				}

				// our class has changed; techniques may be broken and need to be displayed as such
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
				OWiEditMelee_FillTechniqueMoveList(inDialog, inUserData);

				if (inUserData->profile->reference_count > 0) {
					// tell the AI that this profile has changed
					AI2rNotifyMeleeProfile(inUserData->profile, UUcFalse);
				}
			}
		break;

		case OWcEditMelee_TechniqueName:
			if (inUserData->technique != NULL) {
				WMrEditField_GetText(inUserData->ef_techniquename, inUserData->technique->name,
									sizeof(inUserData->technique->name));
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
			}
		break;

		case OWcEditMelee_TechniqueWeight:
			if (inUserData->technique != NULL) {
				inUserData->technique->base_weight = WMrEditField_GetInt32(inUserData->ef_techniqueweight);
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
			}
		break;

		case OWcEditMelee_TechniqueDelay:
			if (inUserData->technique != NULL) {
				inUserData->technique->delay_frames = WMrEditField_GetInt32(inUserData->ef_techniquedelay);
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
			}
		break;

		case OWcEditMelee_TechniqueInterrupt:
			if (inUserData->technique != NULL) {
				inUserData->technique->user_flags &= ~AI2cTechniqueUserFlag_Interruptable;

				if (WMrMessage_Send(inUserData->cb_techniqueinterrupt, WMcMessage_GetValue,
										(UUtUns32) -1, (UUtUns32) -1)) {
					inUserData->technique->user_flags |= AI2cTechniqueUserFlag_Interruptable;
				}
			}
		break;

		case OWcEditMelee_TechniqueGenerous:
			if (inUserData->technique != NULL) {
				inUserData->technique->user_flags &= ~AI2cTechniqueUserFlag_GenerousDirection;

				if (WMrMessage_Send(inUserData->cb_techniquegenerous, WMcMessage_GetValue,
										(UUtUns32) -1, (UUtUns32) -1)) {
					inUserData->technique->user_flags |= AI2cTechniqueUserFlag_GenerousDirection;
				}
			}
		break;

		case OWcEditMelee_TechniqueFearless:
			if (inUserData->technique != NULL) {
				inUserData->technique->user_flags &= ~AI2cTechniqueUserFlag_Fearless;

				if (WMrMessage_Send(inUserData->cb_techniquefearless, WMcMessage_GetValue,
										(UUtUns32) -1, (UUtUns32) -1)) {
					inUserData->technique->user_flags |= AI2cTechniqueUserFlag_Fearless;
				}
			}
		break;

		case OWcEditMelee_TechniqueList:
			technique_index = WMrListBox_GetSelection(inUserData->lb_techniquelist);
			if (technique_index == (UUtUns32) -1) {
				new_technique = NULL;
			} else {
				if (inUserData->current_technique_list == OWcEditMelee_TechniqueMenu_Actions) {
					// nothing

				} else if (inUserData->current_technique_list == OWcEditMelee_TechniqueMenu_Reactions) {
					technique_index += inUserData->profile->num_actions;

				} else if (inUserData->current_technique_list == OWcEditMelee_TechniqueMenu_Spacing) {
					technique_index += inUserData->profile->num_actions + inUserData->profile->num_reactions;

				} else {
					UUmAssert(0);
				}

				new_technique = inUserData->profile->technique + technique_index;
			}

			if (new_technique != inUserData->technique) {
				inUserData->technique = new_technique;
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
				OWiEditMelee_SetupTechniquePane(inDialog, inUserData);
			}
		break;

		case OWcEditMelee_TechniqueNew:
			{
				UUtUns32 newsize;

				total_techniques = inUserData->profile->num_actions + inUserData->profile->num_reactions + inUserData->profile->num_spacing;
				newsize = (total_techniques + 1) * sizeof(AI2tMeleeTechnique);
				inUserData->profile->technique = UUrMemory_Block_Realloc(inUserData->profile->technique, newsize);

				technique_index = WMrListBox_GetNumLines(inUserData->lb_techniquelist);
				if (inUserData->current_technique_list == OWcEditMelee_TechniqueMenu_Spacing) {
					// we add the new technique on the end of the technique array
					new_technique = &inUserData->profile->technique[total_techniques];
					inUserData->profile->num_spacing++;

				} else if (inUserData->current_technique_list == OWcEditMelee_TechniqueMenu_Reactions) {
					// we must insert the new technique into the middle of the technique array
					new_technique = &inUserData->profile->technique[inUserData->profile->num_actions + inUserData->profile->num_reactions];

					// move all of the spacing behaviors down the array by one element
					UUrMemory_MoveOverlap(new_technique, new_technique + 1,
							inUserData->profile->num_spacing * sizeof(AI2tMeleeTechnique));

					inUserData->profile->num_reactions++;

					if (inUserData->profile->num_reactions > AI2cMeleeMaxTechniques) {
						char msg[200];

						sprintf(msg, "Melee profiles can currently only have %d techniques of a given category.",
								AI2cMeleeMaxTechniques);
						WMrDialog_MessageBox(inDialog, "Too many reactions", msg, WMcMessageBoxStyle_OK);
					}

				} else if (inUserData->current_technique_list == OWcEditMelee_TechniqueMenu_Actions) {
					// we must insert the new technique into the middle of the technique array
					new_technique = &inUserData->profile->technique[inUserData->profile->num_actions];

					// move all of the reactions and spacing behaviors down the array by one element
					UUrMemory_MoveOverlap(new_technique, new_technique + 1,
							(inUserData->profile->num_reactions + inUserData->profile->num_spacing) * sizeof(AI2tMeleeTechnique));

					inUserData->profile->num_actions++;

					if (inUserData->profile->num_actions > AI2cMeleeMaxTechniques) {
						char msg[200];

						sprintf(msg, "Melee profiles can currently only have %d techniques of a given category.",
								AI2cMeleeMaxTechniques);
						WMrDialog_MessageBox(inDialog, "Too many actions", msg, WMcMessageBoxStyle_OK);
					}
				} else {
					UUmAssert(0);
				}
				UUrMemory_Clear(new_technique, sizeof(AI2tMeleeTechnique));

				// set up the new technique
				UUrString_Copy(new_technique->name, "Untitled Technique", sizeof(new_technique->name));
				new_technique->base_weight = 10;
				new_technique->importance = 10;
				new_technique->delay_frames = 0;
				new_technique->user_flags = 0;

				// the moves for this technique will be placed at the end of the array
				new_technique->start_index = inUserData->profile->num_moves;
				new_technique->num_moves = 0;

				// our techniques have changed - must re-prepare the profile for use and check
				// anim states
				error = AI2rMelee_PrepareForUse(inUserData->profile);
				if (error != UUcError_None) {
					// can't edit this profile! bail out
					WMrDialog_ModalEnd(inDialog, UUcFalse);
					return UUcTrue;
				}

				// display the new technique
				inUserData->technique = new_technique;
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
				OWiEditMelee_SetupTechniquePane(inDialog, inUserData);
			}
		break;

		case OWcEditMelee_TechniqueDel:
			{
				UUtUns32 move_start, move_end, itr;
				AI2tMeleeTechnique *del_technique = inUserData->technique;

				if (del_technique == NULL)
					break;

				total_techniques = inUserData->profile->num_actions + inUserData->profile->num_reactions + inUserData->profile->num_spacing;
				technique_index = del_technique - inUserData->profile->technique;
				UUmAssert((technique_index >= 0) && (technique_index < total_techniques));

				// delete our moves from the move array
				move_start = del_technique->start_index;
				move_end = move_start + del_technique->num_moves;
				if ((move_end > move_start) && (move_end < inUserData->profile->num_moves)) {
					// we have to move all of the following moves up the array to pack them in
					UUrMemory_MoveOverlap(inUserData->profile->move + move_end, inUserData->profile->move + move_start,
							(inUserData->profile->num_moves - move_end) * sizeof(AI2tMeleeMove));

					// alter all of the techniques' stored move indices so that they still
					// point to the right moves
					for (itr = 0; itr < total_techniques; itr++) {
						if (itr == technique_index)
							continue;

						if (inUserData->profile->technique[itr].start_index >= move_start) {
							UUmAssert(inUserData->profile->technique[itr].start_index >= move_end);
							inUserData->profile->technique[itr].start_index -= del_technique->num_moves;

							UUmAssert((inUserData->profile->technique[itr].start_index >= 0) &&
										(inUserData->profile->technique[itr].start_index +
										inUserData->profile->technique[itr].num_moves <= inUserData->profile->num_moves));
						}
					}

					inUserData->profile->num_moves -= del_technique->num_moves;
				}

				if (technique_index < total_techniques - 1) {
					// move all of the techniques after this one up the array by one element
					UUrMemory_MoveOverlap(del_technique + 1, del_technique,
							(total_techniques - (technique_index + 1)) * sizeof(AI2tMeleeTechnique));
				}

				// change the number of techniques remaining, and update our current-technique pointer
				if (inUserData->current_technique_list == OWcEditMelee_TechniqueMenu_Actions) {
					UUmAssert(inUserData->profile->num_actions > 0);
					inUserData->profile->num_actions--;

					if (inUserData->profile->num_actions == 0) {
						inUserData->technique = NULL;

					} else if (technique_index >= inUserData->profile->num_actions) {
						// we must move our technique pointer up so it remains on the action array
						inUserData->technique = inUserData->profile->technique + (inUserData->profile->num_actions - 1);
					}

				} else if (inUserData->current_technique_list == OWcEditMelee_TechniqueMenu_Reactions) {
					UUmAssert(inUserData->profile->num_reactions > 0);
					inUserData->profile->num_reactions--;

					if (inUserData->profile->num_reactions == 0) {
						inUserData->technique = NULL;

					} else if (technique_index >= inUserData->profile->num_actions + inUserData->profile->num_reactions) {
						// we must move our technique pointer up so it remains on the reaction array
						inUserData->technique = inUserData->profile->technique +
								inUserData->profile->num_actions + (inUserData->profile->num_reactions - 1);
					}

				} else if (inUserData->current_technique_list == OWcEditMelee_TechniqueMenu_Spacing) {
					UUmAssert(inUserData->profile->num_spacing > 0);
					inUserData->profile->num_spacing--;

					if (inUserData->profile->num_spacing == 0) {
						inUserData->technique = NULL;

					} else if (technique_index >= inUserData->profile->num_actions + inUserData->profile->num_reactions + inUserData->profile->num_spacing) {
						// we must move our technique pointer up so it remains on the reaction array
						inUserData->technique = inUserData->profile->technique +
								inUserData->profile->num_actions + inUserData->profile->num_reactions + (inUserData->profile->num_spacing - 1);
					}

				} else {
					UUmAssert(0);
				}

				// our techniques have changed - must re-prepare the profile for use and check
				// anim states
				error = AI2rMelee_PrepareForUse(inUserData->profile);
				if (error != UUcError_None) {
					// can't edit this profile! bail out
					WMrDialog_ModalEnd(inDialog, UUcFalse);
					return UUcTrue;
				}

				// clear the technique pane
				inUserData->technique = NULL;
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
				OWiEditMelee_SetupTechniquePane(inDialog, inUserData);
			}
		break;

		case OWcEditMelee_TechniqueUp:
			{
				AI2tMeleeTechnique swap_technique;

				if (inUserData->technique == NULL)
					break;

				total_techniques = inUserData->profile->num_actions + inUserData->profile->num_reactions + inUserData->profile->num_spacing;
				technique_index = inUserData->technique - inUserData->profile->technique;
				UUmAssert((technique_index >= 0) && (technique_index < total_techniques));

				if ((technique_index == 0) || (technique_index == inUserData->profile->num_actions) ||
					(technique_index == inUserData->profile->num_actions + inUserData->profile->num_reactions)) {
					// we are already at the top of one of the arrays
					break;
				}

				new_technique = inUserData->technique - 1;

				// swap the techniques in the array
				swap_technique = *(inUserData->technique);
				*(inUserData->technique) = *new_technique;
				*new_technique = swap_technique;

				// our techniques have changed - must re-prepare the profile for use and check
				// anim states
				error = AI2rMelee_PrepareForUse(inUserData->profile);
				if (error != UUcError_None) {
					// can't edit this profile! bail out
					WMrDialog_ModalEnd(inDialog, UUcFalse);
					return UUcTrue;
				}

				// redraw the technique list
				inUserData->technique = new_technique;
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
			}
		break;

		case OWcEditMelee_TechniqueDown:
			{
				AI2tMeleeTechnique swap_technique;

				if (inUserData->technique == NULL)
					break;

				total_techniques = inUserData->profile->num_actions + inUserData->profile->num_reactions + inUserData->profile->num_spacing;
				technique_index = inUserData->technique - inUserData->profile->technique;
				UUmAssert((technique_index >= 0) && (technique_index < total_techniques));

				if ((technique_index == inUserData->profile->num_actions - 1) ||
					(technique_index == inUserData->profile->num_actions + inUserData->profile->num_reactions - 1) ||
					(technique_index == total_techniques - 1)) {
					// we are already at the bottom of one of the arrays
					break;
				}

				new_technique = inUserData->technique + 1;

				// swap the techniques in the array
				swap_technique = *(inUserData->technique);
				*(inUserData->technique) = *new_technique;
				*new_technique = swap_technique;

				// our techniques have changed - must re-prepare the profile for use and check
				// anim states
				error = AI2rMelee_PrepareForUse(inUserData->profile);
				if (error != UUcError_None) {
					// can't edit this profile! bail out
					WMrDialog_ModalEnd(inDialog, UUcFalse);
					return UUcTrue;
				}

				// redraw the technique list
				inUserData->technique = new_technique;
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
			}
		break;

		case OWcEditMelee_MoveList:
			if (inUserData->technique == NULL)
				break;

			move_index = WMrListBox_GetSelection(inUserData->lb_movelist);

			if (move_index == (UUtUns32) -1) {
				new_move = NULL;
			} else {
				UUmAssert(move_index < inUserData->technique->num_moves);
				move_index += inUserData->technique->start_index;

				new_move = inUserData->profile->move + move_index;
			}

			if (new_move != inUserData->move) {
				// select this new move, and show its data
				inUserData->move = new_move;
				OWiEditMelee_FillTechniqueMoveList(inDialog, inUserData);
			}
		break;

		case OWcEditMelee_MoveNew:
			{
				UUtUns32 newsize, move_index, itr;

				if (inUserData->technique == NULL)
					break;

				// grow the move array
				newsize = (inUserData->profile->num_moves + 1) * sizeof(AI2tMeleeMove);
				inUserData->profile->move = UUrMemory_Block_Realloc(inUserData->profile->move, newsize);

				move_index = inUserData->technique->start_index + inUserData->technique->num_moves;
				new_move = &inUserData->profile->move[move_index];

				if (move_index < inUserData->profile->num_moves) {
					// this move is inserted in the middle of the move array - we must move all moves
					// following the insertion location down by one.
					UUrMemory_MoveOverlap(new_move, new_move + 1,
							(inUserData->profile->num_moves - move_index) * sizeof(AI2tMeleeMove));

					// any techniques that stored their moves in this section of the array must have
					// their start index incremented by one
					total_techniques = inUserData->profile->num_actions + inUserData->profile->num_reactions + inUserData->profile->num_spacing;
					technique_index = inUserData->technique - inUserData->profile->technique;
					UUmAssert((technique_index >= 0) && (technique_index < total_techniques));

					for (itr = 0; itr < total_techniques; itr++) {
						if (itr == technique_index)
							continue;

						if (inUserData->profile->technique[itr].start_index >= move_index)
							inUserData->profile->technique[itr].start_index++;
					}
				}

				inUserData->technique->num_moves++;
				inUserData->profile->num_moves++;
				UUrMemory_Clear(new_move, sizeof(AI2tMeleeMove));
				new_move->move = (UUtUns32) -1;		// garbage value will be set up by the faux click on lb_movetypelist

				inUserData->move = new_move;

				if (WMrListBox_GetNumLines(inUserData->lb_movetypelist) == 0) {
					// there are no currently available moves of the current type. switch to
					// the attack movetype.
					UUmAssert(inUserData->current_movetype != (AI2cMoveType_Attack >> AI2cMoveType_Shift));
					WMrPopupMenu_SetSelection(inUserData->pm_movetypemenu, (AI2cMoveType_Attack >> AI2cMoveType_Shift));
				}

				// set up the first choice from the listbox, just as if we clicked on it
				UUmAssert(WMrListBox_GetNumLines(inUserData->lb_movetypelist) != 0);
				WMrListBox_SetSelection(inUserData->lb_movetypelist, UUcTrue, 0);

				// our moves have changed - must re-prepare the profile for use and check
				// anim states
				error = AI2rMelee_PrepareForUse(inUserData->profile);
				if (error != UUcError_None) {
					// can't edit this profile! bail out
					WMrDialog_ModalEnd(inDialog, UUcFalse);
					return UUcTrue;
				}

				// display the new move - the technique may also be broken so redisplay its list
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
				OWiEditMelee_FillTechniqueMoveList(inDialog, inUserData);

				if (inUserData->profile->reference_count > 0) {
					// tell the AI that this profile has changed
					AI2rNotifyMeleeProfile(inUserData->profile, UUcFalse);
				}
			}
		break;

		case OWcEditMelee_MoveDel:
			{
				UUtUns32 move_index, itr;

				if (inUserData->move == NULL)
					break;

				move_index = inUserData->move - inUserData->profile->move;
				UUmAssert((move_index >= 0) && (move_index < inUserData->profile->num_moves));

				UUmAssert(inUserData->technique != NULL);
				UUmAssert((move_index >= inUserData->technique->start_index) &&
							(move_index < inUserData->technique->start_index + inUserData->technique->num_moves));

				if (move_index < inUserData->profile->num_moves - 1) {
					// this move is deleted from the middle of the move array - we must move all moves
					// following the insertion location up by one.
					UUrMemory_MoveOverlap(inUserData->move + 1, inUserData->move,
							(inUserData->profile->num_moves - (move_index + 1)) * sizeof(AI2tMeleeMove));

					// any techniques that stored their moves in this section of the array must have
					// their start index decremented by one
					total_techniques = inUserData->profile->num_actions + inUserData->profile->num_reactions + inUserData->profile->num_spacing;
					technique_index = inUserData->technique - inUserData->profile->technique;
					UUmAssert((technique_index >= 0) && (technique_index < total_techniques));

					for (itr = 0; itr < total_techniques; itr++) {
						if (itr == technique_index)
							continue;

						if (inUserData->profile->technique[itr].start_index >= move_index)
							inUserData->profile->technique[itr].start_index--;
					}
				}

				UUmAssert(inUserData->technique->num_moves > 0);
				inUserData->technique->num_moves--;

				UUmAssert(inUserData->profile->num_moves > 0);
				inUserData->profile->num_moves--;

				// no move selected any more
				inUserData->move = NULL;

				// our moves have changed - must re-prepare the profile for use and check
				// anim states
				error = AI2rMelee_PrepareForUse(inUserData->profile);
				if (error != UUcError_None) {
					// can't edit this profile! bail out
					WMrDialog_ModalEnd(inDialog, UUcFalse);
					return UUcTrue;
				}

				// our technique may be broken and needs to be displayed as such
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
				OWiEditMelee_FillTechniqueMoveList(inDialog, inUserData);

				if (inUserData->profile->reference_count > 0) {
					// tell the AI that this profile has changed
					AI2rNotifyMeleeProfile(inUserData->profile, UUcFalse);
				}
			}
		break;

		case OWcEditMelee_MoveUp:
			{
				AI2tMeleeMove swap_move;

				if (inUserData->move == NULL)
					break;

				move_index = inUserData->move - inUserData->profile->move;
				UUmAssert((move_index >= 0) && (move_index < inUserData->profile->num_moves));

				UUmAssert(inUserData->technique != NULL);
				UUmAssert((move_index >= inUserData->technique->start_index) &&
							(move_index < inUserData->technique->start_index + inUserData->technique->num_moves));

				if (move_index == inUserData->technique->start_index)
					break;

				UUmAssert(move_index > 0);
				new_move = inUserData->move - 1;

				// swap the moves in the array
				swap_move = *(inUserData->move);
				*(inUserData->move) = *new_move;
				*new_move = swap_move;

				inUserData->move = new_move;

				// our moves have changed - must re-prepare the profile for use and check
				// anim states
				error = AI2rMelee_PrepareForUse(inUserData->profile);
				if (error != UUcError_None) {
					// can't edit this profile! bail out
					WMrDialog_ModalEnd(inDialog, UUcFalse);
					return UUcTrue;
				}

				// redraw the move and technique lists
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
				OWiEditMelee_FillTechniqueMoveList(inDialog, inUserData);

				if (inUserData->profile->reference_count > 0) {
					// tell the AI that this profile has changed
					AI2rNotifyMeleeProfile(inUserData->profile, UUcFalse);
				}
			}
		break;

		case OWcEditMelee_MoveDown:
			{
				AI2tMeleeMove swap_move;

				if (inUserData->move == NULL)
					break;

				move_index = inUserData->move - inUserData->profile->move;
				UUmAssert((move_index >= 0) && (move_index < inUserData->profile->num_moves));

				UUmAssert(inUserData->technique != NULL);
				UUmAssert((move_index >= inUserData->technique->start_index) &&
							(move_index < inUserData->technique->start_index + inUserData->technique->num_moves));

				if (move_index == inUserData->technique->start_index + inUserData->technique->num_moves - 1)
					break;

				UUmAssert(move_index < inUserData->profile->num_moves - 1);
				new_move = inUserData->move + 1;

				// swap the moves in the array
				swap_move = *(inUserData->move);
				*(inUserData->move) = *new_move;
				*new_move = swap_move;

				inUserData->move = new_move;

				// our moves have changed - must re-prepare the profile for use and check
				// anim states
				error = AI2rMelee_PrepareForUse(inUserData->profile);
				if (error != UUcError_None) {
					// can't edit this profile! bail out
					WMrDialog_ModalEnd(inDialog, UUcFalse);
					return UUcTrue;
				}

				// redraw the move and technique lists
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
				OWiEditMelee_FillTechniqueMoveList(inDialog, inUserData);

				if (inUserData->profile->reference_count > 0) {
					// tell the AI that this profile has changed
					AI2rNotifyMeleeProfile(inUserData->profile, UUcFalse);
				}
			}
		break;

		case OWcEditMelee_MoveTypeList:
			{
				UUtUns32 new_move, newmove_index;

				// where did we click?
				newmove_index = WMrListBox_GetSelection(inUserData->lb_movetypelist);
				if (newmove_index == (UUtUns32) -1)
					break;

				// get the move that corresponds to the line on which we clicked
				new_move = WMrListBox_GetItemData(inUserData->lb_movetypelist, newmove_index);
				UUmAssert(new_move != (UUtUns32) -1);

				if (inUserData->move == NULL)
					break;

				if (inUserData->move->move == new_move)
					break;

				// set the current move
				inUserData->move->move = new_move;

				// our moves have changed - must re-prepare the profile for use and check
				// anim states
				error = AI2rMelee_PrepareForUse(inUserData->profile);
				if (error != UUcError_None) {
					// can't edit this profile! bail out
					WMrDialog_ModalEnd(inDialog, UUcFalse);
					return UUcTrue;
				}

				// redraw the move and technique lists
				OWiEditMelee_FillTechniqueList(inDialog, inUserData);
				OWiEditMelee_FillTechniqueMoveList(inDialog, inUserData);

				if (inUserData->profile->reference_count > 0) {
					// tell the AI that this profile has changed
					AI2rNotifyMeleeProfile(inUserData->profile, UUcFalse);
				}
			}
		break;

		case OWcEditMelee_MoveParam0Field:
			{
				if (inUserData->move == NULL)
					break;

				inUserData->move->param[0] = WMrEditField_GetFloat(inUserData->ef_moveparam0);
			}
		break;

		case OWcEditMelee_MoveParam1Field:
			{
				if (inUserData->move == NULL)
					break;

				inUserData->move->param[1] = WMrEditField_GetFloat(inUserData->ef_moveparam1);
			}
		break;

		case OWcEditMelee_MoveParam2Field:
			{
				if (inUserData->move == NULL)
					break;

				inUserData->move->param[2] = WMrEditField_GetFloat(inUserData->ef_moveparam2);
			}
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}

UUtBool
OWrEditMelee_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool handled = UUcTrue;
	OWtEditMelee_UserData *user_data = (OWtEditMelee_UserData *) WMrDialog_GetUserData(inDialog);

	switch (inMessage)
	{
		case WMcMessage_Destroy:
			// note: user_data may be null if we failed during initdialog
			if (user_data != NULL) {
				// delete our melee profile's variable-length arrays
				if (user_data->object_specific_data.osd.melee_osd.technique != NULL) {
					UUrMemory_Block_Delete(user_data->object_specific_data.osd.melee_osd.technique);
				}
				if (user_data->object_specific_data.osd.melee_osd.move != NULL) {
					UUrMemory_Block_Delete(user_data->object_specific_data.osd.melee_osd.move);
				}

				// restore our user data pointer to what it was before we started monkeying with it
				WMrDialog_SetUserData(inDialog, (UUtUns32) user_data->object);
				UUrMemory_Block_Delete(user_data);
			}
		break;

		case WMcMessage_InitDialog:
			handled = OWiEditMelee_InitDialog(inDialog);
		break;

		case WMcMessage_MenuCommand:
			handled = OWiEditMelee_MenuCommand(inDialog, user_data, (WMtMenu *) inParam2, UUmLowWord(inParam1));
		break;

		case WMcMessage_Command:
			handled = OWiEditMelee_Command(inDialog, user_data, UUmLowWord(inParam1));
		break;

		default:
			handled = UUcFalse;
		break;
	}

	return handled;
}







UUtUns16 OWrChooseMelee(UUtUns16 inProfileID)
{
	OBJtObject *object = NULL;
	OBJtOSD_All	object_specific_data;
	UUtUns16 result = (UUtUns16) -1;

	object_specific_data.osd.melee_osd.id = inProfileID;
	object = OBJrObjectType_Search(OBJcType_Melee, OBJcSearch_MeleeID, &object_specific_data);

	object = OWrSelectObject(OBJcType_Melee, object, UUcTrue, UUcFalse);

	if (NULL != object) {
		OBJrObject_GetObjectSpecificData(object, &object_specific_data);
		result = (UUtUns16) object_specific_data.osd.melee_osd.id;
	}

	return result;
}
#endif
