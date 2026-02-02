/*
	FILE:	Oni_Platform_Mac.h

	AUTHOR:	Brent H. Pease

	CREATED: May 26, 1997

	PURPOSE: MacOS specific code

	Copyright 1997

*/
#ifndef ONI_PLATFORM_MAC_H
#define ONI_PLATFORM_MAC_H

/*---------- constants */

// display has changed dialog
enum {
	iRevertItem= 2, // user buttons
	iConfirmItem= 1,
	kSecondsToConfirm= 8, // seconds before confirm dialog is taken down
	rConfirmSwtchAlrt= 128 // ID of alert dialog
};

// Mac resource IDs & item numbers
enum {
	_mac_always_search_any_opengl_pref_flag= 0x01,
	_mac_saved_display_index_pref_mask= 0xF0000000,
	_mac_saved_display_index_pref_shift= 28,

	CURS_RES_ID= 128,
	MAC_PREF_RSRC_ID= 128,

	KEY_CONFIG_FILENAME_STR_RSRC= 1000,
	COMMAND_LINE_ARGS_STR_RSRC= 1002,
	OPTIONS_DLOG_RSRC= 135,
	OPTIONS_DLOG_RSRC_OSX= 136,
	NUMBER_OF_CUSTOMIZE_KEYS= 14,

	_save_button= 1,
	_cancel_button,
	_rh_defaults_button,
	_lh_defaults_button,
	_args_edit_text,
	_forward_menu,
	_stepleft_menu,
	_backward_menu,
	_stepright_menu,
	_swap_menu,
	_drop_menu,
	_jump_menu,
	_punch_menu,
	_kick_menu,
	_crouch_menu,
	_walk_menu,
	_action_menu,
	_hypo_menu,
	_reload_menu,
	_forward_label,
	_stepleft_label,
	_backward_Label,
	_stepright_label,
	_swap_label,
	_drop_label,
	_jump_label,
	_punch_label,
	_kick_label,
	_courch_label,
	_walk_label,
	_action_label,
	_hypo_label,
	_reload_label,
	_quit_button= 35,

	_ok_button_osx= 1,
	_quit_button_osx,
	_args_edit_text_osx,

	MAC_MBAR_RSRC_ID= 128,

	MAC_DISPLAY_SELECT_RSRC_ID= 137,

	_button_ok= 1,
	_button_cancel= 2,
	_list_item= 3,
	_save_device_check_box= 4
};

/*---------- prototypes */

UUtError ONrPlatform_GetGammaParams(
	UUtInt32 *outMinGamma,
	UUtInt32 *outMaxGamma,
	UUtInt32 *outCurrentGamma);

UUtError ONrPlatform_SetGamma(
	UUtInt32 inNewGamma);

#endif /* ONI_PLATFORM_MAC_H */
