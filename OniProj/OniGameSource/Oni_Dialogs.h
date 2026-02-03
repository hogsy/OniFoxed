#if 0

// ======================================================================
// Oni_Dialogs.h
// ======================================================================
#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_DialogManager.h"

// ======================================================================
// defines
// ======================================================================
#define ONcMainMenu_SwitchTime		20		/* in ticks */

// ======================================================================
// enums
// ======================================================================
enum
{
	ONcDialogID_SplashScreen		= 1000,
	ONcDialogID_MainMenu			= 2000,
	ONcDialogID_NewGame				= 3000,
	ONcDialogID_LoadSave			= 4000,
	ONcDialogID_Multiplayer			= 5000,
	ONcDialogID_Options				= 6000,
	ONcDialogID_Quit				= 7000,
	ONcDialogID_Joining				= 8000
};

// ----------------------------------------------------------------------
enum
{
	ONcBtn_Back						= 50
};

// ----------------------------------------------------------------------
// oni main menu
enum
{
	ONcMM_Btn_NewGame				= 100,
	ONcMM_Btn_LoadSave				= 101,
	ONcMM_Btn_Multiplayer			= 102,
	ONcMM_Btn_Options				= 103,
	ONcMM_Btn_Quit					= 104
};

// ----------------------------------------------------------------------
// new game
enum
{
	ONcNG_LB_Level					= 100,
	ONcNG_Btn_Start					= 101
};

// ----------------------------------------------------------------------
// load save films
enum
{
	ONcLS_Btn_Load					= 100,
	ONcLS_Btn_Save					= 101,
	ONcLS_Btn_Film					= 102,

	ONcLS_Tab_Load					= 400,
	ONcLSL_LB_SavedGames			= 401,
	ONcLSL_EF_NewName				= 402,
	ONcLSL_Btn_Delete				= 403,
	ONcLSL_Btn_Rename				= 404,
	ONcLSL_Btn_Load					= 405,

	ONcLS_Tab_Save					= 500,
	ONcLSS_LB_SavedGames			= 501,
	ONcLSS_EF_NewSavedGame			= 502,
	ONcLSS_Btn_Save					= 503,

	ONcLS_Tab_Film					= 600,
	ONcLSF_LB_SavedFilms			= 601,
	ONcLSF_EF_NewName				= 602,
	ONcLSF_Btn_Delete				= 603,
	ONcLSF_Btn_Rename				= 604,
	ONcLSF_Btn_View					= 605
};

// ----------------------------------------------------------------------
// options dialog
enum
{
	ONcOptns_Btn_AV					= 100,
	ONcOptns_Btn_Controls			= 101,
	ONcOptns_Btn_Advanced			= 102,

	ONcOptns_Tab_Presets			= 400,
	ONcOP_LB_Presets				= 401,

	ONcOptns_Tab_AV					= 500,
	ONcOAV_Sldr_Volume				= 501,
	ONcOAV_Sldr_Music				= 502,
	ONcOAV_Sldr_FX					= 503,
	ONcOAV_Sldr_Ambient				= 504,
	ONcOAV_List_Resolution			= 505,
	ONcOAV_List_Driver				= 506,
	ONcOAV_Sldr_Gamma				= 507,

	ONcOptns_Tab_Controls			= 600,
	ONcOC_EF_Forward1				= 601,
	ONcOC_EF_Forward2				= 602,
	ONcOC_EF_Backward1				= 603,
	ONcOC_EF_Backward2				= 604,
	ONcOC_EF_SidestepLeft1			= 605,
	ONcOC_EF_SidestepLeft2			= 606,
	ONcOC_EF_SidestepRight1			= 607,
	ONcOC_EF_SidestepRight2			= 608,
	ONcOC_EF_TurnLeft1				= 609,
	ONcOC_EF_TurnLeft2				= 610,
	ONcOC_EF_TurnRight1				= 611,
	ONcOC_EF_TurnRight2				= 612,
	ONcOC_EF_Jump1					= 613,
	ONcOC_EF_Jump2					= 614,
	ONcOC_EF_Crouch1				= 615,
	ONcOC_EF_Crouch2				= 616,
	ONcOC_EF_Punch1					= 617,
	ONcOC_EF_Punch2					= 618,
	ONcOC_EF_Kick1					= 619,
	ONcOC_EF_Kick2					= 620,
	ONcOC_EF_Special1				= 621,
	ONcOC_EF_Special2				= 622,
	ONcOC_CB_InvertMouse			= 624,

	ONcOptns_Tab_Advanced			= 700,
	ONcOA_CB_Blood					= 701,
	ONcOA_CB_MotionBlur				= 702,
	ONcOA_CB_LightMaps				= 703
};

// ----------------------------------------------------------------------
// multiplayer dialog
enum
{
	ONcMP_Btn_HostGame				= 100,
	ONcMP_Btn_JoinGame				= 101,
	ONcMP_Btn_Options				= 102,

	ONcMP_Tab_HostGame				= 500,
	ONcMPHG_Btn_StartHost			= 501,
	ONcMPHG_EF_HostName				= 502,
	ONcMPHG_EF_Description			= 503,
	ONcMPHG_LB_Level				= 504,
	ONcMPHG_EF_NumBots				= 509,

	ONcMP_Tab_JoinGame				= 600,
	ONcMPJG_Btn_Join				= 601,
	ONcMPJG_EF_HostAddress			= 602,
	ONcMPJG_EF_Password				= 603,
	ONcMPJG_CB_RememberPassword		= 604,
	ONcMPJG_LB_Hosts				= 605,

	ONcMP_Tab_Options				= 700,
	ONcMPO_EF_PlayerName			= 701,
	ONcMPO_EF_TeamName				= 702
};

// ----------------------------------------------------------------------
// quit dialog
enum
{
	ONcQuit_Btn_Yes					= 100,
	ONcQuit_Btn_No					= 101
};

// ======================================================================
// globals
// ======================================================================
extern UUtBool				ONgDisplayDialogs;
extern UUtUns32				ONgMainMenu_SwitchTime;

// ======================================================================
// prototypes
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
ONrDialogCallback_Joining(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
ONrDialogCallback_MainMenu(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

// ----------------------------------------------------------------------
UUtBool
ONrDialogCallback_NewGame(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
ONrDialogCallback_LoadSave(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
ONrDialogCallback_Multiplayer(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
ONrDialogCallback_Options(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
ONrDialogCallback_Quit(
	DMtDialog				*inDialog,
	DMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

// ----------------------------------------------------------------------
void
ONrDialog_MainMenu(
	void);

void
ONrDialog_SplashScreen(
	void);

// ----------------------------------------------------------------------
UUtError
ONrDialog_Initialize(
	void);

void
ONrDialog_Terminate(
	void);

#endif
