// ======================================================================
// Oni_InGameUI.h
// ======================================================================
#pragma once

#ifndef ONI_INGAMEUI_H
#define ONI_INGAMEUI_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TextSystem.h"

#include "Oni_Weapon.h"

// ======================================================================
// defines
// ======================================================================
#define ONcMaxNumKeys					12
#define ONcMaxDiaryImages				5

//#define ONcMaxStringLength				256

#define ONcIGUI_MaxStringLength			384
#define ONcIGUI_HUDMaxStringLength		64
#define ONcIGUI_HealthWarnLevel			30

#define IGUIcFontFamily_Default			NULL
#define IGUIcFontSize_Default			14
#define IGUIcFontStyle_Default			TScStyle_Bold
#define IGUIcFontShade_Default			IMcShade_Orange

// ======================================================================
// enums
// ======================================================================
typedef tm_enum ONtKey
{
	ONcKey_None						= 0x0000,
	ONcKey_Hold						= 0x1000,
	ONcKey_Quick					= 0x2000,

	ONcKey_Plus						= 0x0100,
	ONcKey_Wait						= 0x0200,

	ONcKey_Punch					= 0x0001,
	ONcKey_Kick						= 0x0002,

	ONcKey_Forward					= 0x0004,
	ONcKey_Backward					= 0x0008,
	ONcKey_Left						= 0x0010,
	ONcKey_Right					= 0x0020,

	ONcKey_Crouch					= 0x0040,
	ONcKey_Jump						= 0x0060,

	ONcKey_HoldPunch				= (ONcKey_Hold | ONcKey_Punch),
	ONcKey_HoldKick					= (ONcKey_Hold | ONcKey_Kick),

	ONcKey_HoldForward				= (ONcKey_Hold | ONcKey_Forward),
	ONcKey_HoldBackward				= (ONcKey_Hold | ONcKey_Backward),
	ONcKey_HoldLeft					= (ONcKey_Hold | ONcKey_Left),
	ONcKey_HoldRight				= (ONcKey_Hold | ONcKey_Right),

	ONcKey_HoldCrouch				= (ONcKey_Hold | ONcKey_Crouch),
	ONcKey_HoldJump					= (ONcKey_Hold | ONcKey_Jump),

	ONcKey_QuickPunch				= (ONcKey_Quick | ONcKey_Punch),
	ONcKey_QuickKick				= (ONcKey_Quick | ONcKey_Kick),

	ONcKey_QuickForward				= (ONcKey_Quick | ONcKey_Forward),
	ONcKey_QuickBackward			= (ONcKey_Quick | ONcKey_Backward),
	ONcKey_QuickLeft				= (ONcKey_Quick | ONcKey_Left),
	ONcKey_QuickRight				= (ONcKey_Quick | ONcKey_Right),

	ONcKey_QuickCrouch				= (ONcKey_Quick | ONcKey_Crouch),
	ONcKey_QuickJump				= (ONcKey_Quick | ONcKey_Jump),

	ONcKey_Action					= 0x00FF,
	ONcKey_Modifier					= 0xF000

} ONtKey;

enum
{
	ONcWeaponStr_Name,
	ONcWeaponStr_AmmoType,
	ONcWeaponStr_CurrentAmmo,
	ONcWeaponStr_Range,
	ONcWeaponStr_FireRate,
	ONcWeaponStr_Hint,

	ONcWeaponStr_NumStrs
};

enum
{
	ONcItemStr_Name,

	// shield
	ONcShieldStr_PowerRemaining = 1,
	ONcShieldStr_Hint,
	ONcShieldStr_NumStrs,

	// hypo
	ONcHypoStr_Dosage = 1,
	ONcHypoStr_WarningLabels,
	ONcHypoStr_Hint,
	ONcHypoStr_NumStrs,

	// ammo
	ONcAmmoStr_Type = 1,
	ONcAmmoStr_Hint,
	ONcAmmoStr_NumStrs,

	// LSIs
	ONcLSIStr_Description
};

enum
{
	IGUIcFontInfoFlag_None				= 0x0000,
	IGUIcFontInfoFlag_FontFamily		= 0x0001,
	IGUIcFontInfoFlag_FontStyle			= 0x0002,
	IGUIcFontInfoFlag_FontShade			= 0x0004,
	IGUIcFontInfoFlag_FontSize			= 0x0008
};

// ======================================================================
// typedefs
// ======================================================================
typedef tm_struct ONtImagePlacement
{
	tm_pad								pad[3];
	UUtBool								visible;		// used at runtime
	IMtPoint2D							dest;
	M3tTextureMap						*image;

} ONtImagePlacement;

// ----------------------------------------------------------------------
typedef tm_struct
ONtIGUI_FontInfo
{
	TStFontFamily						*font_family;
	TStFontStyle						font_style;
	UUtUns32							font_shade;
	UUtUns16							font_size;
	UUtUns16							flags;

} ONtIGUI_FontInfo;

#define ONcTemplate_IGUI_String			UUm4CharToUns32('I', 'G', 'S', 't')
typedef tm_template('I', 'G', 'S', 't', "IGUI String")
ONtIGUI_String
{
	ONtIGUI_FontInfo					font_info;
	char								string[384];	// must match ONcIGUI_MaxStringLength

} ONtIGUI_String;

#define ONcTemplate_IGUI_StringArray		UUm4CharToUns32('I', 'G', 'S', 'A')
typedef tm_template('I', 'G', 'S', 'A', "IGUI String Array")
ONtIGUI_StringArray
{
	tm_pad								pad0[20];

	tm_varindex UUtUns32				numStrings;
	tm_vararray ONtIGUI_String			*string[1];

} ONtIGUI_StringArray;

#define ONcTemplate_IGUI_Page			UUm4CharToUns32('I', 'G', 'P', 'G')
typedef tm_template('I', 'G', 'P', 'G', "IGUI Page")
ONtIGUI_Page
{
	ONtIGUI_FontInfo					font_info;
	tm_templateref						pict;
	ONtIGUI_StringArray					*text;
	ONtIGUI_StringArray					*hint;

} ONtIGUI_Page;

#define ONcTemplate_IGUI_PageArray		UUm4CharToUns32('I', 'G', 'P', 'A')
typedef tm_template('I', 'G', 'P', 'A', "IGUI Page Array")
ONtIGUI_PageArray
{
	tm_pad								pad0[20];

	tm_varindex	UUtUns32				num_pages;
	tm_vararray ONtIGUI_Page			*pages[1];

} ONtIGUI_PageArray;


// ----------------------------------------------------------------------
#define ONcTemplate_DiaryPage			UUm4CharToUns32('D', 'P', 'g', 'e')
typedef tm_template('D', 'P', 'g', 'e', "Diary Page")
ONtDiaryPage
{
	UUtInt16							level_number;
	UUtInt16							page_number;
	UUtBool								has_new_move;
	tm_pad								pad[3];

	ONtKey								keys[12];		// must match ONcMaxNumKeys above
	ONtIGUI_Page						*page;

} ONtDiaryPage;

#define ONcTemplate_ItemPage			UUm4CharToUns32('I', 'P', 'g', 'e')
typedef tm_template('I', 'P', 'g', 'e', "Item Page")
ONtItemPage
{
	WPtPowerupType						powerup_type;
	ONtIGUI_Page						*page;

} ONtItemPage;

#define ONcTemplate_ObjectivePage		UUm4CharToUns32('O', 'P', 'g', 'e')
typedef tm_template('O', 'P', 'g', 'e', "Objective Page")
ONtObjectivePage
{
	tm_pad								pad[2];

	UUtInt16							level_number;
	ONtIGUI_PageArray					*objectives;

} ONtObjectivePage;

#define ONcTemplate_WeaponPage			UUm4CharToUns32('W', 'P', 'g', 'e')
typedef tm_template('W', 'P', 'g', 'e', "Weapon Page")
ONtWeaponPage
{
	WPtWeaponClass						*weaponClass;
	ONtIGUI_Page						*page;

} ONtWeaponPage;

#define ONcTemplate_HelpPage			UUm4CharToUns32('H', 'P', 'g', 'e')
typedef tm_template('H', 'P', 'g', 'e', "Help Page")
ONtHelpPage
{
	tm_pad								pad[2];

	UUtInt16							page_number;
	ONtIGUI_Page						*page;

} ONtHelpPage;

#define ONcTemplate_KeyIcons			UUm4CharToUns32('K', 'e', 'y', 'I')
typedef tm_template('K', 'e', 'y', 'I', "Key Icons")
ONtKeyIcons
{
	M3tTextureMap						*punch;
	M3tTextureMap						*kick;

	M3tTextureMap						*forward;
	M3tTextureMap						*backward;
	M3tTextureMap						*left;
	M3tTextureMap						*right;

	M3tTextureMap						*crouch;
	M3tTextureMap						*jump;

	M3tTextureMap						*hold;
	M3tTextureMap						*plus;

} ONtKeyIcons;

// --------------------------------------------
#define ONcTemplate_TextConsole			UUm4CharToUns32('T', 'x', 't', 'C')
typedef tm_template('T', 'x', 't', 'C', "Text Console")
ONtTextConsole
{
	ONtIGUI_PageArray					*console_data;

} ONtTextConsole;

// --------------------------------------------
typedef tm_struct
ONtIGUI_HUDTextItem
{
	char								text[64];		// must match ONcIGUI_HUDMaxStringLength
	IMtPoint2D							offset;

} ONtIGUI_HUDTextItem;

#define ONcTemplate_IGUI_HUDHelp		UUm4CharToUns32('I', 'G', 'H', 'H')
typedef tm_template('I', 'G', 'H', 'H', "IGUI HUD Help")
ONtIGUI_HUDHelp
{
	tm_pad								pad[28];

	M3tTextureMap						*left_texture;
	M3tTextureMap						*right_texture;
	IMtPoint2D							left_offset;
	IMtPoint2D							right_offset;
	UUtUns32							num_left_items;
	UUtUns32							num_right_items;

	tm_varindex	UUtUns32				num_text_items;
	tm_vararray ONtIGUI_HUDTextItem		text_items[1];

} ONtIGUI_HUDHelp;

// ======================================================================
// prototypes
// ======================================================================
void
ONrInGameUI_Display(
	void);

// ----------------------------------------------------------------------
void
ONrPauseScreen_Display(
	void);

void
ONrPauseScreen_OverrideMessage(
	const char *inMessage);

// ----------------------------------------------------------------------
void
ONrEnemyScanner_Display(
	void);

UUtBool
ONrEnemyScanner_IsOn(
	void);

void
ONrEnemyScanner_Stop(
	void);

// ----------------------------------------------------------------------
UUtError
ONrInGameUI_Initialize(
	void);

UUtError
ONrInGameUI_LevelLoad(
	UUtUns32					inLevelNum);

void
ONrInGameUI_LevelUnload(
	void);

UUtError
ONrInGameUI_RegisterTemplates(
	void);

void
ONrInGameUI_Terminate(
	void);

void
ONrInGameUI_Update(
	void);

void
ONrInGameUI_EnableHelpDisplay(
	UUtBool inEnable);

void
ONrInGameUI_NewWeapon(
	WPtWeaponClass *inWeaponClass);

void
ONrInGameUI_NewItem(
	WPtPowerupType inPowerupType);

void
ONrInGameUI_NotifyRestoreGame(
	UUtUns32 inGameTime);

// ----------------------------------------------------------------------
void
ONrInGameUI_TextConsole_Display(
	ONtTextConsole				*inTextConsole);

// ======================================================================
#endif /* ONI_INGAMEUI_H */
