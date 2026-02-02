// ======================================================================
// Oni_Win_Sound.h
// ======================================================================
#pragma once

#ifndef ONI_WIN_SOUND_H
#define ONI_WIN_SOUND_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#include "Oni_Sound.h"

// ======================================================================
// enums
// ======================================================================
// ------------------------------
// Sound Collection Manager
// ------------------------------
enum
{
	OWcSCM_PM_Category				= 100,
	OWcSCM_LB_Items					= 101,
	OWcSCM_Btn_NewItem				= 102,
	OWcSCM_Btn_EditItem				= 103,
	OWcSCM_Btn_PlayItem				= 104,
	OWcSCM_Btn_DeleteItem			= 105,
	OWcSCM_Btn_CopyItem				= 106,
	OWcSCM_Btn_PasteItem			= 107,
	OWcSCM_Btn_NewCategory			= 108,
	OWcSCM_Btn_RenameCategory		= 109,
	OWcSCM_Btn_DeleteCategory		= 110
};

// ------------------------------
// Sound Group Properties
// ------------------------------
enum
{
	OWcSGP_EF_Name					= 100,
	OWcSGP_LB_Permutations			= 101,
	OWcSGP_Btn_AddPerm				= 102,
	OWcSGP_Btn_EditPerm				= 103,
	OWcSGP_Btn_PlayPerm				= 104,
	OWcSGP_Btn_DeletePerm			= 105,
	OWcSGP_Btn_Play					= 106,
	OWcSGP_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcSGP_Btn_Save					= WMcDialogItem_OK
};

// ------------------------------
// Sound Permutation Properties
// ------------------------------
enum
{
	OWcSPP_Txt_Name					= 100,
	OWcSPP_EF_Weight				= 101,
	OWcSPP_EF_MinVol				= 102,
	OWcSPP_EF_MaxVol				= 103,
	OWcSPP_EF_MinPitch				= 104,
	OWcSPP_EF_MaxPitch				= 105,
	OWcSPP_Btn_Save					= WMcDialogItem_OK,
	OWcSPP_Btn_Cancel				= WMcDialogItem_Cancel
};

// ------------------------------
// WAV Select
// ------------------------------
enum
{
	OWcWS_PM_Directory				= 100,
	OWcWS_LB_DirContents			= 101,
	OWcWS_Btn_Select				= WMcDialogItem_OK,
	OWcWS_Btn_Cancel				= WMcDialogItem_Cancel
};

// ------------------------------
// Category Name
// ------------------------------
enum
{
	OWcCN_EF_Name					= 100,
	OWcCN_Btn_OK					= WMcDialogItem_OK,
	OWcCN_Btn_Cancel				= WMcDialogItem_Cancel
};

// ------------------------------
// Impulse Sound Properties
// ------------------------------
enum
{
	OWcISP_EF_Name					= 100,
	OWcISP_Txt_Group				= 101,
	OWcISP_Btn_Set					= 102,
	OWcISP_Btn_Edit					= 103,
	OWcISP_PM_Priority				= 104,
	OWcISP_EF_MaxVolDist			= 105,
	OWcISP_EF_MinVolDist			= 106,
	OWcISP_EF_MaxVolAngle			= 107,
	OWcISP_EF_MinVolAngle			= 108,
	OWcISP_EF_MinAngleAttn			= 109,
	OWcISP_Btn_Play					= 110,
	OWcISP_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcISP_Btn_Save					= WMcDialogItem_OK
};

// ------------------------------
// Ambient Sound Properties
// ------------------------------
enum
{
	OWcASP_EF_Name					= 100,
	OWcASP_Txt_InGroup				= 101,
	OWcASP_Btn_InGroupSet			= 102,
	OWcASP_Btn_InGroupEdit			= 103,
	OWcASP_Txt_OutGroup				= 104,
	OWcASP_Btn_OutGroupSet			= 105,
	OWcASP_Btn_OutGroupEdit			= 106,
	OWcASP_Txt_BT1					= 107,
	OWcASP_Btn_BT1Set				= 108,
	OWcASP_Btn_BT1Edit				= 109,
	OWcASP_Txt_BT2					= 110,
	OWcASP_Btn_BT2Set				= 111,
	OWcASP_Btn_BT2Edit				= 112,
	OWcASP_Txt_Detail				= 113,
	OWcASP_Btn_DetailSet			= 114,
	OWcASP_Btn_DetailEdit			= 115,
	OWcASP_CB_InterruptOnStop		= 116,
	OWcASP_EF_MinElapsedTime		= 117,
	OWcASP_EF_MaxElapsedTime		= 118,
	OWcASP_EF_SphereRadius			= 119,
	OWcASP_Btn_Inc1					= 120,
	OWcASP_Btn_Dec1					= 121,
	OWcASP_Btn_Inc10				= 122,
	OWcASP_Btn_Dec10				= 123,
	OWcASP_EF_MinVolDist			= 124,
	OWcASP_EF_MaxVolDist			= 125,
	OWcASP_Btn_Start				= 126,
	OWcASP_Btn_Stop					= 127,
	OWcASP_PM_Priority				= 128,
	OWcASP_CB_PlayOnce				= 129,
	OWcASP_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcASP_Btn_Save					= WMcDialogItem_OK
};

// ------------------------------
// Select Sound Group
// ------------------------------
enum
{
	OWcSS_PM_Category				= 100,
	OWcSS_LB_Items					= 101,
	OWcSS_Btn_None					= 102,
	OWcSS_Btn_New					= 103,
	OWcSS_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcSS_Btn_Select				= WMcDialogItem_OK
};

// ------------------------------
// Environment Sound Properties
// ------------------------------
enum
{
	OWcSP_Txt_Name					= 100,
	OWcSP_Btn_Set					= 101,
	OWcSP_Btn_Edit					= 102,
	OWcSP_EF_MaxVolDist				= 103,
	OWcSP_EF_MinVolDist				= 104,
	OWcSP_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcSP_Btn_Save					= WMcDialogItem_OK
};

// ------------------------------
// Sound to Animation
// ------------------------------
enum
{
	OWcStA_LB_Data					= 100,
	OWcStA_Btn_Delete				= 101,
	OWcStA_Btn_Play					= 102,
	OWcStA_Btn_Edit					= 103,
	OWcStA_Btn_New					= 104
};

// ------------------------------
// Sound to Animation Properties
// ------------------------------
enum
{
	OWcStAP_PM_Variant				= 100,
	OWcStAP_PM_AnimType				= 101,
	OWcStAP_Txt_AnimName			= 102,
	OWcStAP_Txt_SoundName			= 103,
	OWcStAP_Btn_SetAnim				= 104,
	OWcStAP_Btn_SetSound			= 105,
	OWcStAP_EF_Frame				= 106,
	OWcStAP_PM_Modifier				= 107,
	OWcStAP_Txt_Duration			= 108,
	OWcStAP_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcStAP_Btn_Save				= WMcDialogItem_OK
};

// ------------------------------
// Select Animation
// ------------------------------
enum
{
	OWcSA_PB_LoadProgress			= 100,
	OWcSA_LB_Animations				= 101,
	OWcSA_Btn_None					= 102,
	OWcSA_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcSA_Btn_Select				= WMcDialogItem_OK
};

// ======================================================================
// prototypes
// ======================================================================
UUtError
OWrSCM_Display(
	OStCollectionType			inCollectionType);

UUtError
OWrASP_Display(
	OStItem						*inItem);

UUtError
OWrSAS_Display(
	WMtDialog					*inParentDialog,
	UUtUns32					*ioAmbientSoundID,
	SStAmbient					**ioAmbientSound);

UUtError
OWrSIS_Display(
	WMtDialog					*inParentDialog,
	UUtUns32					*ioImpulseSoundID,
	SStImpulse					**ioImpulseSound);

UUtError
OWrSoundToAnim_Display(
	void);

void
OWrImpulseSoundProp_Display(
	WMtDialog					*inParentDialog,
	OStItem						*inItem);

// ======================================================================
#endif /* ONI_WIN_SOUND_H */
