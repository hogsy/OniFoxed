/*
	BFW_CL_Platform_MacOS.c
	October 16, 2000  10:30 am
	Stefan
*/

/*---------- headers */

#include <string.h>
#include <ctype.h>
#include "BFW_CL_Platform_MacOS.h"
#include "Oni_Platform_Mac.h"

#if 0

// use SIOUX console
#include <console.h>

UUtInt16
CLrPlatform_GetCommandLine(
	char			***inArgV)
{
	return ccommand(inArgV);
}

#else

// use our own console

/*---------- constants */

enum {
	key_return= 0x0D,
	key_enter= 0x03,
	key_esc= 0x1B
};

// these are the only actions the user gets to mess with key bindings
#define action_forward		"forward"
#define action_stepleft		"stepleft"
#define action_backward		"backward"
#define action_stepright	"stepright"
#define action_swap			"swap"
#define action_drop			"drop"
#define action_jump			"jump"
#define action_punch		"punch"
#define action_kick			"kick"
#define action_crouch		"crouch"
#define action_walk			"walk"
#define action_action		"action"
#define action_hypo			"hypo"
#define action_reload		"reload"

#define key_config_preamble		"# Oni key_config.txt file for Mac\n# hold down the shift key at startup to edit control keys\n# from within the game\n\nunbindall\n\n"
#define key_config_rh_default	"# right handed controls\nbind w to forward\nbind a to stepleft\nbind s to backward\nbind d to stepright\nbind q to swap\nbind e to drop\nbind f to punch\nbind c to kick\n\n"
#define key_config_lh_default	"#left handed controls\nbind p to forward\nbind l to stepleft\nbind apostrophe to stepright\nbind semicolon to backward\nbind o to swap\nbind leftbracket to drop\nbind k to punch\nbind comma to kick\nbind enter to walk\n\n"
#define key_config_default_controls	"\n# default controls\nbind space to jump\nbind leftshift to crouch\nbind rightshift to crouch\nbind capslock to walk\nbind leftcontrol to action\nbind rightcontrol to action\nbind tab to hypo\nbind r to reload\n\n"
#define key_config_misc			"# misc\nbind mousebutton1 to fire1\nbind mousebutton2 to fire2\nbind mousebutton3 to fire3\nbind x to secretx\nbind y to secrety\nbind z to secretz\nbind mousexaxis to aim_LR\nbind mouseyaxis to aim_UD\nbind fkey1 to pausescreen\nbind v to lookmode\nbind backslash to profile_toggle\nbind fkey13 to screenshot\n"

/*---------- structures */


struct action_binding {
	char *oni_action_name;
	char *oni_key_name;
};

/*---------- globals */

static char *action_labels[NUMBER_OF_CUSTOMIZE_KEYS]= {
	"Forward",
	"Left",
	"Backward",
	"Right",
	"Pick up",
	"Drop",
	"Jump",
	"Punch",
	"Kick",
	"Crouch",
	"Walk",
	"Action",
	"Hypo",
	"Reload"
};

static struct action_binding action_bindings[]= {
	// same as action_bindings_rh
	{ action_forward, "w" },
	{ action_stepleft, "a" },
	{ action_backward, "s" },
	{ action_stepright, "d" },
	{ action_swap, "q" },
	{ action_drop, "e" },
	{ action_jump, "space" },
	{ action_punch, "f" },
	{ action_kick, "c" },
	{ action_crouch, "leftshift" },
	{ action_walk, "capslock" },
	{ action_action, "leftcontrol" },
	{ action_hypo, "tab" },
	{ action_reload, "r" }
};

static struct action_binding action_bindings_rh[]= {
	{ action_forward, "w" },
	{ action_stepleft, "a" },
	{ action_backward, "s" },
	{ action_stepright, "d" },
	{ action_swap, "q" },
	{ action_drop, "e" },
	{ action_jump, "space" },
	{ action_punch, "f" },
	{ action_kick, "c" },
	{ action_crouch, "leftshift" },
	{ action_walk, "capslock" },
	{ action_action, "leftcontrol" },
	{ action_hypo, "tab" },
	{ action_reload, "r" }
};

static struct action_binding action_bindings_lh[]= {
	{ action_forward, "p" },
	{ action_stepleft, "l" },
	{ action_backward, "semicolon" },
	{ action_stepright, "apostrophe" },
	{ action_swap, "o" },
	{ action_drop, "leftbracket" },
	{ action_jump, "space" },
	{ action_punch, "f" },
	{ action_kick, "comma" },
	{ action_crouch, "leftshift" },
	{ action_walk, "enter" },
	{ action_action, "leftcontrol" },
	{ action_hypo, "tab" },
	{ action_reload, "r" }
};

const int num_action_bindings= sizeof(action_bindings)/sizeof(action_bindings[0]);

static char *key_bindings[]= {
	// these names must match those used in BFW_LI_Translators.c & elsewhere
	// the order of these must match the order of the appropriate MENU resource too
	"backspace",
	"tab",
	"capslock",
	"enter",
	"leftshift",
	// { "rightshift", },
	"leftcontrol",
	//{ "leftwindows", },
	"leftoption",
	"leftalt",
	"space",
	//{ "rightalt",	},
	//{ "rightoption",},
	//{ "rightwindows",},
	//{ "rightcontrol",	},
	//{ "printscreen",},
	//{ "scrolllock",},
	//{ "pause",},
	//{ "insert",},
	"home",
	"pageup",
	"delete",
	"end",
	"pagedown",
	"uparrow",
	"leftarrow",
	"downarrow",
	"rightarrow",
	"numlock",
	"divide",
	"multiply",
	"subtract",
	"add",
	"numpadequals",
	"numpadenter",
	"decimal",
	"numpad0",
	"numpad1",
	"numpad2",
	"numpad3",
	"numpad4",
	"numpad5",
	"numpad6",
	"numpad7",
	"numpad8",
	"numpad9",
	"backslash",
	"semicolon",
	"period",
	"apostrophe",
	"slash",
	"leftbracket",
	"rightbracket",
	"comma",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"0",
	"q",
	"w",
	"e",
	"r",
	"t",
	"y",
	"u",
	"i",
	"o",
	"p",
	"a",
	"s",
	"d",
	"f",
	"g",
	"h",
	"j",
	"k",
	"l",
	"z",
	"x",
	"c",
	"v",
	"b",
	"n",
	"m"
};

static char *key_labels[]= {
	"backspace",
	"tab",
	"caps lock",
	"enter",
	"shift",
	// { "rightshift", },
	"control",
	//{ "leftwindows", },
	"option",
	"\xf0 (command)",
	"space",
	//{ "rightalt",	},
	//{ "rightoption",},
	//{ "rightwindows",},
	//{ "rightcontrol",	},
	//{ "printscreen",},
	//{ "scrolllock",},
	//{ "pause",},
	//{ "insert",},
	"home",
	"page up",
	"delete",
	"end",
	"page down",
	"up arrow",
	"left arrow",
	"down arrow",
	"right arrow",
	"num lock",
	"divide",
	"multiply",
	"subtract",
	"add",
	"numpad =",
	"numpad enter",
	"decimal",
	"numpad 0",
	"numpad 1",
	"numpad 2",
	"numpad 3",
	"numpad 4",
	"numpad 5",
	"numpad 6",
	"numpad 7",
	"numpad 8",
	"numpad 9",
	"\\",
	";",
	". (period)",
	"' (apostrophe)",
	"/",
	"[",
	"]",
	", (comma)",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"0",
	"Q",
	"W",
	"E",
	"R",
	"T",
	"Y",
	"U",
	"I",
	"O",
	"P",
	"A",
	"S",
	"D",
	"F",
	"G",
	"H",
	"J",
	"K",
	"L",
	"Z",
	"X",
	"C",
	"V",
	"B",
	"N",
	"M"
};

const int num_key_bindings= sizeof(key_bindings)/sizeof(char *);


/*---------- code */

static void pstrcopy (
	StringPtr p1,
	StringPtr p2)
{
	register int len;

	len= *p2++= *p1++;
	while (--len >= 0)
	{
		*p2++= *p1++;
	}

	return;
}

static void init_bindings_from_file(
	char *filename)
{
	FILE *key_config_file;

	if ((key_config_file= fopen(filename, "r")) != NULL)
	{
		for (;;)
		{
			char *str, *token, *delimiters= " \t\r\n";
			char line[128];

			str= fgets(line, 127, key_config_file);

			if (str[0] == '\r')
			{
				BlockMove(line+1, line, 127);
			}

			if ((str[0] == '#') || (str[0] == '\n') || (str[0] == '\r'))
			{
				continue;
			}

			if ((str == NULL) || (str[0] == '\0'))
			{
				break; // EOF
			}

			// search for strings of the form "bind oni_key_name to oni_action_name"
			token= strtok(line, delimiters);
			if (token && strcmp(token, "bind") == 0)
			{
				int i;

				if ((token= strtok(NULL, delimiters)) != NULL)
				{
					for (i= 0; i<num_key_bindings; i++)
					{
						if (strcmp(token, key_bindings[i]) == 0)
						{
							int j;

							// what action is this key bound to? is it one we care about?
							token= strtok(NULL, delimiters);
							if (token && strcmp(token, "to") == 0)
							{
								if ((token= strtok(NULL, delimiters)) != NULL)
								{
									for (j= 0; j<num_action_bindings; j++)
									{
										#define IF_FIND_ACTION_FROM_TOKEN(action) \
											if (strcmp(token, action_bindings[j].oni_action_name) == 0) \
											{ \
												action_bindings[j].oni_key_name= key_bindings[i]; \
												break; \
											}

										IF_FIND_ACTION_FROM_TOKEN(action_forward) else
										IF_FIND_ACTION_FROM_TOKEN(action_stepleft) else
										IF_FIND_ACTION_FROM_TOKEN(action_backward) else
										IF_FIND_ACTION_FROM_TOKEN(action_stepright) else
										IF_FIND_ACTION_FROM_TOKEN(action_swap) else
										IF_FIND_ACTION_FROM_TOKEN(action_drop) else
										IF_FIND_ACTION_FROM_TOKEN(action_jump) else
										IF_FIND_ACTION_FROM_TOKEN(action_punch) else
										IF_FIND_ACTION_FROM_TOKEN(action_kick) else
										IF_FIND_ACTION_FROM_TOKEN(action_crouch) else
										IF_FIND_ACTION_FROM_TOKEN(action_walk) else
										IF_FIND_ACTION_FROM_TOKEN(action_action) else
										IF_FIND_ACTION_FROM_TOKEN(action_hypo) else
										IF_FIND_ACTION_FROM_TOKEN(action_reload)
									}
								}
							}
						}
					}
				}
			}
		}
		fclose(key_config_file);
	}
	else if ((key_config_file= fopen(filename, "w")) != NULL)
	{
		// create key_config file
		fprintf(key_config_file, key_config_preamble);
		fprintf(key_config_file, key_config_rh_default);
		fprintf(key_config_file, key_config_lh_default);
		fprintf(key_config_file, key_config_default_controls);
		fprintf(key_config_file, key_config_misc);
		fclose(key_config_file);
	}

	return;
}

static void save_bindings_to_file(
	char *filename)
{
	FILE *key_config_file;

	if ((key_config_file= fopen(filename, "w")) != NULL)
	{
		int i;

		fprintf(key_config_file, key_config_preamble);
		for (i= 0; i<num_action_bindings; i++)
		{
			fprintf(key_config_file, "bind %s to %s\n", action_bindings[i].oni_key_name, action_bindings[i].oni_action_name);
		}
		fprintf(key_config_file, key_config_default_controls);
		fprintf(key_config_file, key_config_misc);
		fclose(key_config_file);
	}

	return;
}

static void set_dialog_item_text(
	DialogPtr dialog,
	short item,
	StringPtr text)
{
	Handle item_handle;
	Rect item_rect;
	short item_type;

	UUmAssert(dialog);
	UUmAssert(text);

	GetDialogItem(dialog, item, &item_type, &item_handle, &item_rect);
	UUmAssert(item_handle);
	SetDialogItemText(item_handle, text);

	return;
}

static void set_dialog_from_action_bindings(
	DialogPtr dialog)
{
	int i, j;

	UUmAssert(dialog);
	for (i= 0; i<num_action_bindings; i++)
	{
		short menu_item_number= i + _forward_menu;
		short label_item_number= menu_item_number + NUMBER_OF_CUSTOMIZE_KEYS;

		for (j= 0; j<num_key_bindings; j++)
		{
			if (strcmp(action_bindings[i].oni_key_name, key_bindings[j]) == 0)
			{
				char label[64]= "";
				short menu_selection= j+1;
				short item_type;
				Handle item_handle;
				Rect item_rect;

				GetDialogItem(dialog, menu_item_number, &item_type, &item_handle, &item_rect);
				SetControlValue((ControlHandle)item_handle, menu_selection);

				sprintf(label, "%s : %s", action_labels[i], key_labels[j]);
				c2pstrcpy(label, label);
				set_dialog_item_text(dialog, label_item_number, label);

				break;
			}
		}
	}

	return;
}

static void set_action_bindings_from_dialog(
	DialogPtr dialog)
{
	int item_number;

	for (item_number= _forward_menu; item_number<=_reload_menu; item_number++)
	{
		Handle item_handle;
		Rect item_rect;
		short item_type, action_index, menu_selection;
		Str15 text= "\p";

		action_index= item_number - _forward_menu;

		GetDialogItem(dialog, item_number, &item_type, &item_handle, &item_rect);
		menu_selection= GetControlValue((ControlHandle)item_handle);
		if (menu_selection> 0)
		{
			action_bindings[action_index].oni_key_name= key_bindings[menu_selection - 1];
		}
	}

	return;
}

static Boolean handle_dialog_event(
	DialogPtr dialog,
	EventRecord *event,
	Boolean *save_prefs)
{
	Boolean done= false;
	DialogPtr which_dialog= NULL;
	WindowPtr which_window= NULL;
	short item_hit= 0, item_type= 0;
	Handle item_handle= NULL;

	if (DialogSelect(event, &which_dialog, &item_hit ) == false)
	{
		// no extra work needed ,  just return
	}
	else if (which_dialog == dialog)
	{
		if ((item_hit == _args_edit_text) && ((event->what == keyDown) || (event->what == autoKey)))
		{
			// carriage return & enter should count as hitting the OK button;
			// esc should count as hitting the cancel button
			unsigned char key_code= (event->message & charCodeMask);
			Boolean eat_key= false;

			if ((key_code == key_return) || (key_code == key_enter))
			{
				item_hit= _save_button;
				eat_key= true;
			}
			else if (key_code == key_esc)
			{
				item_hit= _cancel_button;
				eat_key= true;
			}

			if (eat_key)
			{
				Str255 edit_text= "\p";
				Rect item_rect;

				GetDialogItem(dialog, _args_edit_text, &item_type, &item_handle, &item_rect);
				UUmAssert(item_handle);
				GetDialogItemText(item_handle, edit_text);
				--edit_text[0];
				SetDialogItemText(item_handle, edit_text);
			}
		}

		if (item_hit > 0)
		{
			switch (item_hit)
			{
				case _forward_menu:
				case _stepleft_menu:
				case _backward_menu:
				case _stepright_menu:
				case _swap_menu:
				case _drop_menu:
				case _jump_menu:
				case _punch_menu:
				case _kick_menu:
				case _crouch_menu:
				case _walk_menu:
				case _action_menu:
				case _hypo_menu:
				case _reload_menu:
					{
						char label[64]= "";
						short menu_selection;
						short item_type;
						Handle item_handle;
						Rect item_rect;
						int action_index= item_hit - _forward_menu;

						GetDialogItem(dialog, item_hit, &item_type, &item_handle, &item_rect);
						menu_selection= GetControlValue((ControlHandle)item_handle);

						action_bindings[action_index].oni_key_name= key_bindings[menu_selection - 1];
						sprintf(label, "%s : %s", action_labels[action_index], key_labels[menu_selection - 1]);
						c2pstrcpy(label, label);
						set_dialog_item_text(dialog, item_hit + NUMBER_OF_CUSTOMIZE_KEYS, label);
					}
					break;
				case _save_button:
					done= true;
					*save_prefs= true;
					break;
				case _cancel_button:
					done= true;
					*save_prefs= false;
					break;
				case _rh_defaults_button:
					memcpy(action_bindings, action_bindings_rh, sizeof(action_bindings));
					set_dialog_from_action_bindings(dialog);
					break;
				case _lh_defaults_button:
					memcpy(action_bindings, action_bindings_lh, sizeof(action_bindings));
					set_dialog_from_action_bindings(dialog);
					break;
				case _quit_button:
					exit(0);
					break;
				default:
					break;
			}
		}
	}

	return done;
}

static Boolean handle_dialog_event_osx(
	DialogPtr dialog,
	EventRecord *event)
{
	Boolean done= false;
	DialogPtr which_dialog= NULL;
	WindowPtr which_window= NULL;
	short item_hit= 0, item_type= 0;
	Handle item_handle= NULL;

	if (DialogSelect(event, &which_dialog, &item_hit ) == false)
	{
		// no extra work needed ,  just return
	}
	else if (which_dialog == dialog)
	{
		if ((item_hit == _args_edit_text_osx) && ((event->what == keyDown) || (event->what == autoKey)))
		{
			// carriage return & enter should count as hitting the OK button;
			// esc should count as hitting the cancel button
			unsigned char key_code= (event->message & charCodeMask);
			Boolean eat_key= false;

			if ((key_code == key_return) || (key_code == key_enter))
			{
				Str255 edit_text= "\p";
				Rect item_rect;

				item_hit= _ok_button_osx;

				GetDialogItem(dialog, _args_edit_text_osx, &item_type, &item_handle, &item_rect);
				UUmAssert(item_handle);
				GetDialogItemText(item_handle, edit_text);
				--edit_text[0];
				SetDialogItemText(item_handle, edit_text);
			}
			else if (key_code == key_esc)
			{
				item_hit= _quit_button_osx;
			}
		}

		if (item_hit > 0)
		{
			switch (item_hit)
			{
				case _ok_button_osx:
					done= true;
					break;
				case _quit_button_osx:
					exit(0);
					break;
				default:
					break;
			}
		}
	}

	return done;
}

static UUtInt16 CLrPlatform_GetCommandLine_PreOSX(
	char ***argv)
{
	DialogPtr dialog;
	StringHandle key_config_filename_string= NULL, command_line_string= NULL;
	UUtInt16 argc= 0;

	command_line_string= GetString(COMMAND_LINE_ARGS_STR_RSRC);

	if ((key_config_filename_string= GetString(KEY_CONFIG_FILENAME_STR_RSRC)) != NULL)
	{
		HLock(key_config_filename_string);
		p2cstrcpy((char *)(*key_config_filename_string), *key_config_filename_string);
		init_bindings_from_file((char *)(*key_config_filename_string));
	}

	dialog= GetNewDialog(OPTIONS_DLOG_RSRC, nil, (WindowPtr)(-1L));

	if (dialog && key_config_filename_string)
	{
		Boolean done= false, save_prefs= false;
		GrafPtr save_port;

		GetPort(&save_port);

		SetDialogDefaultItem(dialog, _save_button);
		SetDialogCancelItem(dialog, _cancel_button);
		SetDialogTracksCursor(dialog, true);

		SetPort(GetWindowPort(dialog));
		ShowWindow(GetWindowPort(dialog));

		// setup menus & labels
		set_dialog_from_action_bindings(dialog);
		if (command_line_string)
		{
			HLock(command_line_string);
			set_dialog_item_text(dialog, _args_edit_text, *command_line_string);
			HUnlock(command_line_string);
		}
		else
		{
			set_dialog_item_text(dialog, _args_edit_text, "\p");
		}

		while (done == false)
		{
			EventRecord event;

			if (GetNextEvent( everyEvent, &event))
			{
				done= handle_dialog_event(dialog, &event, &save_prefs);
			}
		}

		if (save_prefs && key_config_filename_string)
		{
			save_bindings_to_file((char *)(*key_config_filename_string));
		}

		if (key_config_filename_string)
		{
			HUnlock(key_config_filename_string);
			ReleaseResource(key_config_filename_string);
		}

		// get command line args
		{
			Handle item_handle;
			short item_type;
			Rect item_rect;
			static char command_line[256]= "";
			static char *command_line_args[32]= { "Oni" };

			GetDialogItem(dialog, _args_edit_text, &item_type, &item_handle, &item_rect);
			UUmAssert(item_handle);
			GetDialogItemText(item_handle, (StringPtr)command_line);
			if (command_line_string)
			{
				if (save_prefs)
				{
					SetHandleSize(command_line_string, command_line[0]+1);
					HLock(command_line_string);
					pstrcopy(command_line, *command_line_string);
					HUnlock(command_line_string);
					ChangedResource(command_line_string);
					WriteResource(command_line_string);
				}
				ReleaseResource(command_line_string);
			}
			p2cstrcpy(command_line, command_line); // an in-place copy

			argc= 1; // "Oni"
			if (command_line[0] != '\0')
			{
				char *arg= strstr(command_line, "-");
				while (arg && (arg[0] != '\0') && (argc < 31))
				{
					int length= 0;

					while ((arg[length] != '\0') && !isspace(arg[length]) && (length < 63))
					{
						++length;
					}
					command_line_args[argc]= arg;
					argc++;
					if (arg[length] != '\0')
					{
						arg[length]= '\0';
						arg+= (length+1);
					}
					else
					{
						arg+= length;
					}
				}
			}

			*argv= command_line_args;
		}


		SetPort(save_port);
		DisposeDialog(dialog);
	}

		return argc;
}

static UUtInt16 CLrPlatform_GetCommandLine_OSX(
	char ***argv)
{
	DialogPtr dialog;
	StringHandle command_line_string= NULL;
	UUtInt16 argc= 0;

	command_line_string= GetString(COMMAND_LINE_ARGS_STR_RSRC);

	dialog= GetNewDialog(OPTIONS_DLOG_RSRC_OSX, nil, (WindowPtr)(-1L));

	if (dialog)
	{
		Boolean done= false;
		GrafPtr save_port;

		GetPort(&save_port);

		SetDialogDefaultItem(dialog, _ok_button_osx);
		SetDialogCancelItem(dialog, _quit_button_osx);
		SetDialogTracksCursor(dialog, true);

		SetPort(GetWindowPort(dialog));
		ShowWindow(GetWindowPort(dialog));

		// setup menus & labels
		if (command_line_string)
		{
			HLock(command_line_string);
			set_dialog_item_text(dialog, _args_edit_text_osx, *command_line_string);
			HUnlock(command_line_string);
		}
		else
		{
			set_dialog_item_text(dialog, _args_edit_text_osx, "\p");
		}

		while (done == false)
		{
			EventRecord event;

			if (GetNextEvent( everyEvent, &event))
			{
				done= handle_dialog_event_osx(dialog, &event);
			}
		}

		// get command line args
		{
			Handle item_handle;
			short item_type;
			Rect item_rect;
			static char command_line[256]= "";
			static char *command_line_args[32]= { "Oni" };

			GetDialogItem(dialog, _args_edit_text_osx, &item_type, &item_handle, &item_rect);
			UUmAssert(item_handle);
			GetDialogItemText(item_handle, (StringPtr)command_line);
			if (command_line_string)
			{
				SetHandleSize(command_line_string, command_line[0]+1);
				HLock(command_line_string);
				pstrcopy(command_line, *command_line_string);
				HUnlock(command_line_string);
				ChangedResource(command_line_string);
				WriteResource(command_line_string);
				ReleaseResource(command_line_string);
			}
			p2cstrcpy(command_line, command_line); // an in-place copy

			argc= 1; // "Oni"
			if (command_line[0] != '\0')
			{
				char *arg= command_line;

				while (isspace(*arg)) ++arg;

				while (arg && (arg[0] != '\0') && (argc < 31))
				{
					int length= 0;

					while ((arg[length] != '\0') && !isspace(arg[length]) && (length < 63))
					{
						++length;
					}
					command_line_args[argc]= arg;
					argc++;
					if (arg[length] != '\0')
					{
						arg[length]= '\0';
						arg+= (length+1);
					}
					else
					{
						arg+= length;
					}
				}
			}

			*argv= command_line_args;
		}


		SetPort(save_port);
		DisposeDialog(dialog);
	}

	return argc;
}

UUtInt16 CLrPlatform_GetCommandLine(
	char ***argv)
{
	OSErr err;
	unsigned long value;
	UUtInt16 argc= 0;

	// check OS version; the pre-OSX version has key-config items that use special
	// dialog controls which puke under OSX, hence the 2 versions
	err= Gestalt(gestaltSystemVersion, &value);
	if (err == noErr)
	{
		unsigned long major_version;

		// value will look like this: 0x00000904 (OS 9.0.4)
		major_version= (value & 0x0000FF00)>>8;
		if (major_version >= 10)
		{
			argc= CLrPlatform_GetCommandLine_OSX(argv);
		}
		else
		{
			argc= CLrPlatform_GetCommandLine_PreOSX(argv);
		}
	}

	return argc;
}

#endif
