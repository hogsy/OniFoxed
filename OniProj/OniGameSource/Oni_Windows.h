// ======================================================================
// Oni_Windows.h
// ======================================================================
#ifndef ONI_WINDOWS_H
#define ONI_WINDOWS_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"

#include "Oni.h"

// ======================================================================
// enums
// ======================================================================
// ----------------------------------------------------------------------
// ------------------------------
// Oni Window Messages
// ------------------------------
enum
{
	OWcMessages				= WMcMessage_User,

	OWcMessage_RunGame,

	OWcMessage_MPHost,
	OWcMessage_MPJoin,

	OWcMessage_Particle_UpdateList
};

// ----------------------------------------------------------------------
// menubar
// ----------------------------------------------------------------------
enum
{
	OWcOniWindow_MenuBar	= 100
};

// ----------------------------------------------------------------------
// menus
// ----------------------------------------------------------------------
// ------------------------------
// Menu IDs
// ------------------------------
enum
{
	OWcMenu_Game			= 100,
	OWcMenu_Settings		= 200,
	OWcMenu_Objects			= 1000,
	OWcMenu_Test			= 2000,
	OWcMenu_Tools			= 3000,
	OWcMenu_Particle		= 3500,
	OWcMenu_Sound			= 4000,
	OWcMenu_AI				= 5000,
	OWcMenu_Misc			= 6000

/* see Oni_OutGameUI.c for 10000 */
};

// ------------------------------
// Game Menu
// ------------------------------
enum
{
	OWcMenu_Game_NewGame	= 100,
	OWcMenu_Game_MP_Host	= 102,
	OWcMenu_Game_MP_Join	= 103,
	OWcMenu_Game_Resume		= 104,
	OWcMenu_Game_Quit		= 101
};

// ------------------------------
// Settings Menu
// ------------------------------
enum
{
	OWcMenu_Settings_Player			= 100,
	OWcMenu_Settings_Options		= 101,
	OWcMenu_Settings_MainMenu		= 102,
	OWcMenu_Settings_LoadGame		= 103,
	OWcMenu_Settings_QuitGame		= 104,
	OWcMenu_Settings_TextConsole	= 110
};

// ------------------------------
// Performance Menu
// ------------------------------
enum
{
	OWcMenu_Perf_Overall	= 100,
	OWcMenu_Perf_GSD		= 101,
	OWcMenu_Perf_GSU		= 102,
	OWcMenu_Perf_Net		= 103
};

// ------------------------------
// Test Menu
// ------------------------------
enum
{
	OWcMenu_Test_TestDialog	= 100
};

// ------------------------------
// Object Menu
// ------------------------------
enum
{
	OWcMenu_Obj_New				= 100,
	OWcMenu_Obj_Delete			= 101,
	OWcMenu_Obj_Duplicate		= 102,
	OWcMenu_Obj_Save			= 103,
	OWcMenu_Obj_Position		= 104,
	OWcMenu_Obj_Properties		= 105,
	OWcMenu_Obj_ShowFlags		= 106,
	OWcMenu_Obj_ShowFurniture	= 107,
	OWcMenu_Obj_ShowTriggers	= 108,
	OWcMenu_Obj_ExportGunk		= 109,
	OWcMenu_Obj_LightProperties	= 110,
	OWcMenu_Obj_ExportFlag		= 111,
	OWcMenu_Obj_ShowCharacters	= 112,
	OWcMenu_Obj_ShowPatrols		= 113,
	OWcMenu_Obj_ShowTurrets		= 114,
	OWcMenu_Obj_ShowConsoles	= 115,
	OWcMenu_Obj_ShowParticles	= 116,
	OWcMenu_Obj_ShowDoors		= 117,
	OWcMenu_Obj_ShowPowerUps	= 118,
	OWcMenu_Obj_ShowWeapons		= 119,
	OWcMenu_Obj_ShowTriggerVolumes = 120,
	OWcMenu_Obj_ShowSounds		= 121

};

// ------------------------------
// Particle Menu
// ------------------------------
enum
{
	OWcMenu_Particle_Edit		= 100,
	OWcMenu_Particle_Save		= 200,
	OWcMenu_Particle_Discard	= 300
};

// ------------------------------
// Sound Menu
// ------------------------------
enum
{
	OWcMenu_Sound_Groups		= 100,
	OWcMenu_Ambient_Sounds		= 101,
	OWcMenu_Impulse_Sounds		= 102,
	OWcMenu_Animations			= 103,
	OWcMenu_ExportSoundInfo		= 104,
	OWcMenu_CreateDialogue		= 105,
	OWcMenu_CreateImpulse		= 106,
	OWcMenu_CreateGroup			= 107,
	OWcMenu_ViewAnimations		= 108,
	OWcMenu_WriteAmbAiffList	= 109,
	OWcMenu_WriteNeutralAiffList= 110,
	OWcMenu_FixMinMaxDist		= 111,
	OWcMenu_FindBrokenSound		= 112,
	OWcMenu_FixSpeechAmbients	= 113
};

// ------------------------------
// Misc Menu
// ------------------------------
enum
{
	OWcMenu_Impact_Effect		= 100,
	OWcMenu_Texture_Materials	= 101,
	OWcMenu_Impact_Effect_Write	= 102
};

// ----------------------------------------------------------------------
// Oni Window Types
// ----------------------------------------------------------------------
enum
{
	OWcWindowType_Oni		= (WMcWindowType_UserDefined + 1),
	OWcWindowType_Console,
	OWcWindowType_Performance,
	OWcWindowType_InGameUI,

	OWcWindowType_Test
};

// ----------------------------------------------------------------------
// Oni Dialogs
// ----------------------------------------------------------------------
enum
{
/*	see Oni_InGameUI.c for number 70 */

	OWcDialog_Test					= 101,

	OWcDialog_StringList			= 102,
	OWcDialog_GetMask				= 103,

	OWcDialog_MPHost				= 128,
	OWcDialog_MPJoin				= 129,
	OWcDialog_LevelLoadProgress		= 130,
	OWcDialog_SetupPlayer			= 132,
	OWcDialog_LevelLoad				= 133,

/*	see Oni_OutGameUI.c for number 150 - ? */
	OWcDialog_OGU_LevelLoadProgress = 155,

	OWcDialog_ObjNew				= 200,
	OWcDialog_EditObjectPos			= 201,

	OWcDialog_Prop_Generic			= 210,
	OWcDialog_Prop_Char				= 211,
	OWcDialog_Prop_TriggerVolume	= 212,
	OWcDialog_Prop_Flag				= 213,
	OWcDialog_Prop_Light			= 214,
	OWcDialog_Prop_Char2			= 215,
	OWcDialog_Prop_Trigger			= 216,
	OWcDialog_Prop_Turret			= 217,
	OWcDialog_Prop_Sound			= 218,
	OWcDialog_Prop_Console			= 219,
	OWcDialog_Prop_Particle			= 220,
	OWcDialog_Prop_Door				= 221,
	OWcDialog_Prop_Event			= 222,
	OWcDialog_Prop_Furniture		= 223,

	OWcDialog_Particle_Edit			= 250,
	OWcDialog_Particle_Class		= 251,
	OWcDialog_Particle_Variables	= 252,
	OWcDialog_Particle_Actions		= 253,
	OWcDialog_Particle_Appearance	= 254,
	OWcDialog_Particle_Emitters		= 255,
	OWcDialog_Particle_Events		= 256,
	OWcDialog_Particle_Attractor	= 257,

	OWcDialog_Particle_Value_Int	= 260,
	OWcDialog_Particle_Value_Float	= 261,
	OWcDialog_Particle_Value_String	= 262,
	OWcDialog_Particle_Value_Shade	= 263,
	OWcDialog_Particle_Value_Enum	= 264,

	OWcDialog_Particle_NewVar		= 270,
	OWcDialog_Particle_RenameVar	= 271,
	OWcDialog_Particle_NewAction	= 272,
	OWcDialog_Particle_VarRef		= 273,
	OWcDialog_Particle_ActionList	= 274,
	OWcDialog_Particle_Texture		= 275,
	OWcDialog_Particle_EmitterFlags	= 276,
	OWcDialog_Particle_EmitterChoice= 277,
	OWcDialog_Particle_ClassName	= 278,
	OWcDialog_Particle_Number		= 279,

	OWcDialog_Particle_DecalTextures= 280,

	OWcDialog_AI_EditPaths			= 300,
	OWcDialog_AI_EditPath			= 301,
	OWcDialog_AI_EditPathPoint		= 302,
	OWcDialog_AI_ChooseFlag			= 303,
	OWcDialog_AI_EditCharacters		= 304,
	OWcDialog_AI_ChoosePath			= 305,
	OWcDialog_AI_EditCombat			= 306,
	OWcDialog_AI_ChooseCombat		= 307,
	OWcDialog_AI_EditMelee			= 308,
	OWcDialog_AI_EditNeutral		= 309,

	OWcDialog_Sound_Group_Manager	= 600,
	OWcDialog_Sound_Group_Prop		= 601,
	OWcDialog_Sound_Perm_Prop		= 602,
	OWcDialog_WAV_Select			= 603,
	OWcDialog_Category_Name			= 604,
	OWcDialog_Ambient_Sound_Manager	= 605,
	OWcDialog_Ambient_Sound_Prop	= 606,
	OWcDialog_Impulse_Sound_Manager	= 607,
	OWcDialog_Impulse_Sound_Prop	= 608,
	OWcDialog_Select_Sound			= 609,
	OWcDialog_Sound_to_Anim			= 610,
	OWcDialog_Sound_to_Anim_Prop	= 611,
	OWcDialog_Select_Animation		= 612,
	OWcDialog_Texture_to_Material	= 613,
	OWcDialog_Create_Dialogue		= 614,
	OWcDialog_Create_Impulse		= 615,
	OWcDialog_Create_Group			= 616,
	OWcDialog_View_Animation		= 620,

	OWcDialog_Impact_Effect			= 700,
	OWcDialog_Impact_Effect_Prop	= 701
};

// ----------------------------------------------------------------------
// ------------------------------
// Level Load Dialog
// ------------------------------
enum
{
	OWcLevelLoad_LB_Level		= 100
};

// ------------------------------
// Multiplayer Host dialog
// ------------------------------
enum
{
	OWcMPHost_EF_HostName		= 100,
	OWcMPHost_LB_Level			= 101,
	OWcMPHost_LB_GameType		= 102,
	OWcMPHost_EF_NumBots		= 103,
	OWcMPHost_EF_BotSkillLevel	= 104,
	OWcMPHost_EF_NumMinutes		= 105,
	OWcMPHost_EF_MaxKills		= 106
};

// ------------------------------
// Multiplayer Join dialog
// ------------------------------
enum
{
	OWcMPJoin_LB_Servers		= 100
};

// ------------------------------
// Level Load Progress
// ------------------------------
enum
{
	OWcLevelLoad_ProgressBar	= 100
};

// ------------------------------
// Player Settings
// ------------------------------
enum
{
	OWcSettings_Player_PlayerName		= 100,
	OWcSettings_Player_CharacterClass	= 101
};

// ------------------------------
// Test dialog
// ------------------------------
enum
{
	OWcTest_ListBox				= 100,
	OWcTest_Checkbox			= 101,
	OWcTest_RadioButton_1		= 102,
	OWcTest_RadioButton_2		= 103,
	OWcTest_ProgressBar			= 104,
	OWcTest_Slider				= 105,
	OWcTest_PopupMenu			= 106,
	OWcTest_ListBox2			= 107,
	OWcTest_EditField			= 108,
	OWcTest_TestChild			= 109
};

// ======================================================================
// globals
// ======================================================================
extern UUtBool					OWgMPJoin_LevelHasLoaded;

// ======================================================================
// prototypes
// ======================================================================
UUtError
OWrInitialize(
	void);

UUtBool OWrWindow_Resize(int new_width, int new_height);

void
OWrTerminate(
	void);

void
OWrUpdate(
	void);

UUtError
OWrLevelBegin(
	void);

void
OWrLevelEnd(
	void);

// ----------------------------------------------------------------------
void
OWrConsole_Toggle(
	void);

void
OWrSplashScreen_Display(
	void);

void
OWrOniWindow_Startup(
	void);

UUtBool
OWrOniWindow_Visible(
	void);

void
OWrOniWindow_Toggle(
	void);

// ----------------------------------------------------------------------
void
OWrLevelList_Initialize(
	WMtDialog				*inDialog,
	UUtUns16				inItemID);

UUtUns16
OWrLevelList_GetLevelNumber(
	WMtDialog				*inDialog,
	UUtUns16				inItemID);

void
OWrLevelLoad_StartLevel(
	UUtUns16				inLevel);

// ----------------------------------------------------------------------
UUtBool
OWrSettings_Player_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

char*
OWrSettings_GetCharacterClass(
	void);

char*
OWrSettings_GetPlayerName(
	void);

UUtError
OWrSettings_Initialize(
	void);

UUtUns32 OWrGetMask(UUtUns32 inMask, const char **inFlagList);
void OWrMaskToString(UUtUns32 inMask, const char **inFlagList, UUtUns32 inStringLength, char *outString);

// ======================================================================
#endif /* ONI_WINDOWS_H */
