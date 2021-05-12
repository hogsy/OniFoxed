// ======================================================================
// Oni_Win_ImpactEffects.h
// ======================================================================
#pragma once

#ifndef ONI_WIN_IMPACTEFFECTS_H
#define ONI_WIN_IMPACTEFFECTS_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// enums
// ======================================================================
// ------------------------------
// Impact Effects
// ------------------------------
enum
{
	OWcIE_LB_Impact					= 100,
	OWcIE_LB_Material				= 101,
	OWcIE_LB_AttachedEntries		= 102,
	OWcIE_LB_FoundEntries			= 103,
	OWcIE_Btn_Delete				= 104,
	OWcIE_Btn_Copy					= 105,
	OWcIE_Btn_Paste					= 106,
	OWcIE_Btn_Edit					= 107,
	OWcIE_Btn_New					= 108,
	OWcIE_Txt_Lookup				= 109,
	OWcIE_PM_Modifier				= 111	
};

// ------------------------------
// Impact Effect Properties
// ------------------------------
enum
{
	OWcIEprop_PM_ModType			= 100,
	OWcIEprop_Txt_SoundName			= 101,
	OWcIEprop_Btn_SoundSet			= 102,
	OWcIEprop_Btn_SoundEdit			= 103,
	OWcIEprop_LB_Particles			= 104,
	OWcIEprop_Btn_ParticleDelete	= 105,
	OWcIEprop_Btn_ParticleNew		= 106,
	OWcIEprop_EF_ParticleClass		= 107,
	OWcIEprop_PM_ParticleOrientation= 108,
	OWcIEprop_PM_ParticleLocation	= 109,
	OWcIEprop_Txt_Param0			= 110,
	OWcIEprop_Txt_Param1			= 111,
	OWcIEprop_Txt_Param2			= 112,
	OWcIEprop_Txt_Param3			= 113,
	OWcIEprop_Txt_Param4			= 114,
	OWcIEprop_PM_Component			= 115,
	OWcIEprop_Btn_SoundDelete		= 116,
	OWcIEprop_EF_Param0				= 120,
	OWcIEprop_EF_Param1				= 121,
	OWcIEprop_EF_Param2				= 122,
	OWcIEprop_EF_Param3				= 123,
	OWcIEprop_EF_Param4				= 124,
	OWcIEprop_CB_Check0				= 130,
	OWcIEprop_CB_Check1				= 131,
	OWcIEprop_CB_Check2				= 132,
	OWcIEprop_CB_Check3				= 133,
	OWcIEprop_CB_Check4				= 134,
	OWcIEprop_CB_AISound			= 150,
	OWcIEprop_PM_AISoundType		= 151,
	OWcIEprop_EF_AISoundRadius		= 152,
	OWcIEprop_Btn_OK				= WMcDialogItem_OK
};

// ======================================================================
// prototypes
// ======================================================================
UUtError
OWrImpactEffect_Display(
	void);

void
OWrIE_FillMaterialListBox(
	WMtDialog					*inDialog,
	WMtListBox					*inListBox);

void
OWrIE_FillImpactListBox(
	WMtDialog					*inDialog,
	WMtListBox					*inListBox);

// ======================================================================
#endif /* ONI_WIN_IMPACTEFFECTS_H */