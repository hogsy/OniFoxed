// ======================================================================
// Oni_Win_Tools.h
// ======================================================================
#ifndef ONI_WIN_TOOLS_H
#define ONI_WIN_TOOLS_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "Oni_Object.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	EOcEF_AbsLocX				= 100,
	EOcEF_AbsLocY				= 101,
	EOcEF_AbsLocZ				= 102,

	EOcEF_OffLocX				= 103,
	EOcEF_OffLocY				= 104,
	EOcEF_OffLocZ				= 105,

	EOcBtn_LocX_Inc_0_1			= 106,
	EOcBtn_LocX_Dec_0_1			= 107,
	EOcBtn_LocY_Inc_0_1			= 108,
	EOcBtn_LocY_Dec_0_1			= 109,
	EOcBtn_LocZ_Inc_0_1			= 110,
	EOcBtn_LocZ_Dec_0_1			= 111,

	EOcBtn_LocX_Inc_1_0			= 112,
	EOcBtn_LocX_Dec_1_0			= 113,
	EOcBtn_LocY_Inc_1_0			= 114,
	EOcBtn_LocY_Dec_1_0			= 115,
	EOcBtn_LocZ_Inc_1_0			= 116,
	EOcBtn_LocZ_Dec_1_0			= 117,

	EOcBtn_LocX_Inc_10_0		= 118,
	EOcBtn_LocX_Dec_10_0		= 119,
	EOcBtn_LocY_Inc_10_0		= 120,
	EOcBtn_LocY_Dec_10_0		= 121,
	EOcBtn_LocZ_Inc_10_0		= 122,
	EOcBtn_LocZ_Dec_10_0		= 123,

	EOcTxt_RotType				= 148,
	EOcEF_RotX					= 124,
	EOcEF_RotY					= 125,
	EOcEF_RotZ					= 126,

	EOcBtn_RotX_Inc_1			= 127,
	EOcBtn_RotX_Dec_1			= 128,
	EOcBtn_RotY_Inc_1			= 129,
	EOcBtn_RotY_Dec_1			= 130,
	EOcBtn_RotZ_Inc_1			= 131,
	EOcBtn_RotZ_Dec_1			= 132,

	EOcBtn_RotX_Inc_15			= 133,
	EOcBtn_RotX_Dec_15			= 134,
	EOcBtn_RotY_Inc_15			= 135,
	EOcBtn_RotY_Dec_15			= 136,
	EOcBtn_RotZ_Inc_15			= 137,
	EOcBtn_RotZ_Dec_15			= 138,

	EOcBtn_RotX_Inc_90			= 139,
	EOcBtn_RotX_Dec_90			= 140,
	EOcBtn_RotY_Inc_90			= 141,
	EOcBtn_RotY_Dec_90			= 142,
	EOcBtn_RotZ_Inc_90			= 143,
	EOcBtn_RotZ_Dec_90			= 144,

	EOcBtn_Revert				= 145,
	EOcBtn_Gravity				= 147,
	EOcBtn_Apply				= 146
};

enum
{
	NOcPM_Category				= 100
};

// event properties
enum
{
	PEcEF_Type					= 100,
	PEcEF_Label0				= 101,
	PEcEF_Value0				= 102
};

// Doors
enum
{
	PDcLB_DoorTypes				= 100,
	PDcEF_ID					= 102,

	PDcCB_InitialLocked			= 103,
	PDcCB_Locked				= 104,
	PDcCB_Test					= 105,
	PDcEF_KeyID					= 106,
	PDcEF_ActivationRadius		= 107,
	PDcCB_DoubleDoors			= 108,
	PDcCB_ManualDoor			= 109,

	PDcPM_Texture0				= 110,
	PDcPM_Texture1				= 111,

	PDcCB_Mirror				= 112,

	PDcCB_OneWay				= 113,
	PDcCB_Reverse				= 114,

	PDcLB_Events				= 200,
	PDcBtn_NewEvent				= 201,
	PDcBtn_DelEvent				= 202,
	PDcBtn_EditEvent			= 203,

	PDcBtn_Save					= WMcDialogItem_OK,
	PDcBtn_Cancel				= WMcDialogItem_Cancel,
	PDcBtn_Revert				= 101

};

// Triggers
enum
{
	PTcLB_TriggerTypes			= 100,
	PTcEF_ID					= 102,
	PTcCB_Active				= 103,
	PTcCB_InitialActive			= 109,

	PTcCB_Reverse				= 105,
	PTcCB_PingPong				= 120,
	PTcEF_StartOffset			= 106,
	PTcEF_Speed					= 107,

	PTcEF_TimeOn				= 113,
	PTcEF_TimeOff				= 114,
	PTcEF_EmitterCount			= 115,
	PTcEF_Color					= 116,

	PTcLB_Events				= 108,

	PTcBtn_Save					= WMcDialogItem_OK,
	PTcBtn_Cancel				= WMcDialogItem_Cancel,
	PTcBtn_Revert				= 101,

	PTcBtn_NewEvent				= 110,
	PTcBtn_DelEvent				= 111,
	PTcBtn_EditEvent			= 112
};

// Consoles
enum
{
	PCONcLB_ConsoleTypes		= 100,
	PCONcEF_ID					= 101,
	PCONcCB_Active				= 102,
	PCONcCB_InitialActive		= 103,
	PCONcCB_Triggered			= 104,
	PCONcCB_Punch				= 105,
	PCONcCB_IsAlarm				= 106,

	PCONcLB_Events				= 108,

	PCONcPM_InactiveScreen		= 200,
	PCONcPM_ActiveScreen		= 201,
	PCONcPM_TriggeredScreen		= 202,

	PCONcBtn_NewEvent			= 110,
	PCONcBtn_DelEvent			= 111,
	PCONcBtn_EditEvent			= 112,

	PCONcBtn_Save				= WMcDialogItem_OK,
	PCONcBtn_Cancel				= WMcDialogItem_Cancel,
	PCONcBtn_Revert				= 101

};

// Turrets
enum
{
	PTUcLB_TurretTypes			= 100,
	PTUcEF_ID					= 102,
	PTUcCB_Active				= 103,
	PTUcCB_InitialActive		= 109,

	PTUcEF_MaxHorizSpeed		= 106,
	PTUcEF_MaxVertSpeed			= 107,

	PTUcEF_MinHorizAngle		= 110,
	PTUcEF_MaxHorizAngle		= 111,
	PTUcEF_MinVertAngle			= 112,
	PTUcEF_MaxVertAngle			= 113,

	PTUcEF_TargetTeams			= 114,
	PTUcEF_Timeout				= 115,
	PTUcEF_TargetTeamsButton	= 116,
	PUTcEF_TargetTeamsNumber	= 117,

	PTUcBtn_Save				= WMcDialogItem_OK,
	PTUcBtn_Cancel				= WMcDialogItem_Cancel,
	PTUcBtn_Revert				= 101
};

//OWcDialog_StringList
enum
{
	DSLcLB_StringList			= 100,
	DSLcLB_StringTitle			= 101,
	DSLcBtn_Save				= WMcDialogItem_OK,
	DSLcBtn_Cancel				= WMcDialogItem_Cancel
};


enum
{
	PGcLB_ObjectTypes			= 100,
	PGcBtn_Save					= WMcDialogItem_OK,
	PGcBtn_Cancel				= WMcDialogItem_Cancel,
	PGcBtn_Revert				= 101
};

enum
{
	PFUcEF_Tag					= 102
};

enum
{
	PTVcEF_ID					= 100,
	PTVcEF_Width				= 101,
	PTVcEF_Height				= 102,
	PTVcEF_Depth				= 103,
	PTVcBtn_IncW				= 104,
	PTVcBtn_DecW				= 105,
	PTVcBtn_IncH				= 106,
	PTVcBtn_DecH				= 107,
	PTVcBtn_IncD				= 108,
	PTVcBtn_DecD				= 109,
	PTVcBtn_Revert				= 110,
	PTVcBtn_Cancel				= 2,
	PTVcBtn_Save				= 1
};

enum
{
	PCcEF_Name					= 100,
	PCcPM_Team					= 101,
	PCcEF_Health				= 102,
	PCcLB_Class					= 103,
	PCcEF_AmmoBallistic_Use		= 104,
	PCcEF_AmmoBallistic_Drop	= 105,
	PCcEF_AmmoEnergy_Use		= 106,
	PCcEF_AmmoEnergy_Drop		= 107,
	PCcEF_Hypo_Use				= 108,
	PCcEF_Hypo_Drop				= 109,
	PCcEF_ShieldGreen_Use		= 110,
	PCcEF_ShieldGreen_Drop		= 111,
	PCcEF_ShieldBlue_Use		= 112,
	PCcEF_ShieldBlue_Drop		= 113,
	PCcEF_ShieldRed_Use			= 114,
	PCcEF_ShieldRed_Drop		= 115,
	PCcPM_Weapon				= 116,
	PCcPM_LevelSpecificItem		= 117,

	PCcPM_Lull					= 118,
	PCcPM_UnknownNoise			= 119,
	PCcPM_DangerousNoise		= 120,
	PCcPM_GunshotHeard			= 121,
	PCcPM_VisualContact			= 122,
	PCcPM_Attacked				= 123,

	PCcPM_LongRange				= 124,
	PCcPM_MediumRange			= 125,
	PCcPM_HandToHand			= 126,
	PCcPM_MediumRetreat			= 127,
	PCcPM_LongRetreat			= 128,

	PCcBtn_Revert				= 129,
	PCcBtn_Cancel				= WMcDialogItem_Cancel,
	PCcBtn_Save					= WMcDialogItem_OK

};

enum
{
	PFcEF_ID_Prefix				= 100,
	PFcEF_ID_Number				= 101,
	PFcPM_Color					= 102,
	PFcEF_Note					= 103
};

enum
{
	PLcPM_NodeName				= 100,
	PLcEF_R						= 101,
	PLcEF_G						= 102,
	PLcEF_B						= 103,
	PLcRB_Area					= 104,
	PLcRB_Linear				= 105,
	PLcRB_Point					= 106,
	PLcRB_Diffuse				= 107,
	PLcRB_Spot					= 108,
	PLcEF_Intensity				= 109,
	PLcEF_BeamAngle				= 110,
	PLcEF_FieldAngle			= 111

};

enum
{
	PPcLB_ClassList				= 100,
	PPcEF_Velocity				= 101,
	PPcEF_Tag					= 102,
	PPcCB_NotInitiallyCreated	= 103,

	PPcEF_SizeX					= 200,
	PPcEF_SizeY					= 201,
	PPcBtn_SizeXPlusSmall		= 210,
	PPcBtn_SizeXMinusSmall		= 211,
	PPcBtn_SizeYPlusSmall		= 212,
	PPcBtn_SizeYMinusSmall		= 213,
	PPcBtn_SizeXPlusLarge		= 220,
	PPcBtn_SizeXMinusLarge		= 221,
	PPcBtn_SizeYPlusLarge		= 222,
	PPcBtn_SizeYMinusLarge		= 223,
	PPcTxt_SizeDesc				= 230,
	PPcTxt_SizeStep0			= 231,
	PPcTxt_SizeStep1			= 232,
	PPcTxt_SizeX				= 233,
	PPcTxt_SizeY				= 234,

	PPcEF_Angle					= 300,
	PPcBtn_AnglePlusSmall		= 310,
	PPcBtn_AngleMinusSmall		= 311,
	PPcBtn_AnglePlusLarge		= 320,
	PPcBtn_AngleMinusLarge		= 321,
	PPcTxt_AngleDesc			= 330,
	PPcTxt_AngleStep0			= 331,
	PPcTxt_AngleStep1			= 332,
	PPcTxt_Angle				= 333
};

// ------------------------------
// Environment Sound Properties
// ------------------------------
enum
{
	OWcSP_Txt_Name					= 100,
	OWcSP_Btn_Set					= 101,
	OWcSP_Btn_Edit					= 102,
	OWcSP_RB_Spheres				= 103,
	OWcSP_RB_Volume					= 104,
	OWcSP_EF_MaxVolDist				= 105,
	OWcSP_EF_MinVolDist				= 106,
	OWcSP_EF_Length					= 107,
	OWcSP_EF_Width					= 108,
	OWcSP_EF_Height					= 109,
	OWcSP_Btn_LenInc1				= 110,
	OWcSP_Btn_LenDec1				= 111,
	OWcSP_Btn_WidInc1				= 112,
	OWcSP_Btn_WidDec1				= 113,
	OWcSP_Btn_HgtInc1				= 114,
	OWcSP_Btn_HgtDec1				= 115,
	OWcSP_Btn_LenInc10				= 116,
	OWcSP_Btn_LenDec10				= 117,
	OWcSP_Btn_WidInc10				= 118,
	OWcSP_Btn_WidDec10				= 119,
	OWcSP_Btn_HgtInc10				= 120,
	OWcSP_Btn_HgtDec10				= 121,
	OWcSP_EF_Pitch					= 122,
	OWcSP_EF_Volume					= 123,
	OWcSP_Btn_Cancel				= WMcDialogItem_Cancel,
	OWcSP_Btn_Save					= WMcDialogItem_OK
};

// ======================================================================
// structures
// ======================================================================

// ======================================================================
// prototypes
// ======================================================================
UUtError
OWrTools_CreateObject(
	OBJtObjectType			inObjectType,
	const OBJtOSD_All		*inObjectSpecificData);

UUtError
OWrTools_Initialize(
	void);

void
OWrTools_Terminate(
	void);

UUtBool OWrObjectProperties_SetOSD(WMtDialog *inDialog, OBJtObject *inObject, OBJtOSD_All *inOSD);

// ----------------------------------------------------------------------
UUtError
OWrEOPos_Display(
	void);

UUtBool
OWrEOPos_IsVisible(
	void);

// ----------------------------------------------------------------------
UUtError
OWrObjNew_Display(
	void);

UUtBool
OWrObjNew_IsVisible(
	void);

// ----------------------------------------------------------------------
UUtBool
OWrProp_Display(OBJtObject *inObject);

UUtBool
OWrProp_IsVisible(
	void);

// ----------------------------------------------------------------------
UUtError
OWrLightProp_Display(
	void);

UUtBool
OWrLightProp_HasLight(
	void);

UUtBool
OWrLightProp_IsVisible(
	void);

// ----------------------------------------------------------------------

ONtEvent* OWrNewEvent_Display( );

// ----------------------------------------------------------------------
// string list dialog box
// ----------------------------------------------------------------------

#define OWcStringListDlg_MaxStringLength		32

typedef struct OWtStringListDlgInstance
{
	char					title[OWcStringListDlg_MaxStringLength];
	char					list_title[OWcStringListDlg_MaxStringLength];
	UUtUns32				return_value;
	UUtInt32				selected_index;
	WMtDialog				*dialog;
	UUtMemory_Array*		string_array;
} OWtStringListDlgInstance;

UUtError OWrStringListDialog_Create( OWtStringListDlgInstance* inInstance );
UUtError OWrStringListDialog_Destroy( OWtStringListDlgInstance* inInstance );
UUtError OWrStringListDialog_AddString( OWtStringListDlgInstance* inInstance, char *inString );
UUtError OWrStringListDialog_DoModal( OWtStringListDlgInstance* inInstance );
UUtError OWrStringListDialog_SetTitle( OWtStringListDlgInstance* inInstance, char* inTitle );
UUtError OWrStringListDialog_SetListTitle( OWtStringListDlgInstance* inInstance, char* inTitle );


// ======================================================================
#endif /* ONI_WIN_TOOLS_H */
