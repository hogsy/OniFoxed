// ======================================================================
// Oni_Win_Particle.h
// ======================================================================
#ifndef ONI_WIN_PARTICLE_H
#define ONI_WIN_PARTICLE_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"

#include "Oni.h"

// ======================================================================
// enums
// ======================================================================
enum
{
	OWcParticle_NameType_Texture		= 0,
	OWcParticle_NameType_Geometry		= 1,
	OWcParticle_NameType_ParticleClass	= 2,
	OWcParticle_NameType_String			= 3
};

enum
{
	OWcParticle_Disable_Never			= 0,
	OWcParticle_Disable_LowDetail		= 1,
	OWcParticle_Disable_MediumDetail	= 2
};

// ------------------------------
// Particle Editor dialog
// ------------------------------
enum
{
	OWcParticle_Edit_New		= 100,
	OWcParticle_Edit_Edit		= 101,
	OWcParticle_Edit_Duplicate	= 102,
	OWcParticle_Edit_NewDecal	= 103,
	OWcParticle_Edit_Listbox	= 200
};

// ------------------------------
// Particle Class dialog
// ------------------------------
enum
{
	OWcParticle_Class_Save				= 100,
	OWcParticle_Class_Revert			= 101,
	OWcParticle_Class_Variables			= 102,
	OWcParticle_Class_Events			= 103,
//	OWcParticle_Class_Triggers			= 104,
	OWcParticle_Class_Emitters			= 105,
	OWcParticle_Class_Appearance		= 106,
	OWcParticle_Class_Attractor			= 107,
	OWcParticle_Class_Disable			= 108,

	OWcParticle_Class_HasVelocity		= 200,
	OWcParticle_Class_HasOrientation	= 201,
	OWcParticle_Class_HasOffsetPos		= 202,
	OWcParticle_Class_HasDynamicMatrix	= 203,
	OWcParticle_Class_DeltaPosCollision	= 204,
	OWcParticle_Class_LockToLinked		= 205,
	OWcParticle_Class_HasContrailData	= 206,
	OWcParticle_Class_IsContrailEmitter	= 207,
	OWcParticle_Class_HasDamageOwner	= 208,

	OWcParticle_Class_LifetimeText		= 300,
	OWcParticle_Class_LifetimeButton	= 301,

	OWcParticle_Class_Decorative		= 302,

	OWcParticle_Class_CollideEnv		= 303,
	OWcParticle_Class_CollideChar		= 304,

	OWcParticle_Class_RadiusText		= 305,
	OWcParticle_Class_RadiusButton		= 306,

	OWcParticle_Class_GlobalTint		= 307,

	OWcParticle_Class_IsAttracted		= 308,

	OWcParticle_Class_ExpireOnCutscene	= 309,
	OWcParticle_Class_DieOnCutscene		= 310,

	OWcParticle_Class_DodgeText			= 350,
	OWcParticle_Class_DodgeButton		= 351,
	OWcParticle_Class_AlertText			= 352,
	OWcParticle_Class_AlertButton		= 353,
	OWcParticle_Class_FlybyText			= 354,
	OWcParticle_Class_FlybyButton		= 355
};

// ------------------------------
// Particle Variables dialog
// ------------------------------
enum
{
	OWcParticle_Variables_Listbox	= 100,
	OWcParticle_Variables_MemUsage	= 101,
	OWcParticle_Variables_New		= 102,
	OWcParticle_Variables_Edit		= 103,
	OWcParticle_Variables_Rename	= 104,
	OWcParticle_Variables_Delete	= 105
};

// ------------------------------
// Particle Appearance dialog
// ------------------------------
enum
{
	OWcParticle_Appearance_XScale		= 100,
	OWcParticle_Appearance_YScaleEnable	= 101,
	OWcParticle_Appearance_YScale		= 102,
	OWcParticle_Appearance_Rotation		= 103,
	OWcParticle_Appearance_Alpha		= 104,
	OWcParticle_Appearance_Texture		= 105,
	OWcParticle_Appearance_TextureIndex	= 106,
	OWcParticle_Appearance_SpriteType	= 107,
	OWcParticle_Appearance_Invisible	= 108,
	OWcParticle_Appearance_DisplayType	= 109,
	OWcParticle_Appearance_TextureTime	= 110,
	OWcParticle_Appearance_ScaleToVelocity= 111,
	OWcParticle_Appearance_InitiallyHidden= 112,
	OWcParticle_Appearance_XOffset		= 113,
	OWcParticle_Appearance_XShorten		= 114,
	OWcParticle_Appearance_Tint			= 115,
	OWcParticle_Appearance_EdgeFade		= 116,
	OWcParticle_Appearance_EdgeFadeMin	= 117,
	OWcParticle_Appearance_EdgeFadeMax	= 118,
	OWcParticle_Appearance_LensFlare	= 119,
	OWcParticle_Appearance_EdgeFade1Sided= 120,
	OWcParticle_Appearance_MaxContrail	= 121,
	OWcParticle_Appearance_LensflareDist= 122,
	OWcParticle_Appearance_LensFade		= 123,
	OWcParticle_Appearance_LensInFrames	= 124,
	OWcParticle_Appearance_LensOutFrames= 125,
	OWcParticle_Appearance_MaxDecals	= 126,
	OWcParticle_Appearance_DecalFadeFrames= 127,
	OWcParticle_Appearance_DecalWrap	= 128,
	OWcParticle_Appearance_DecalFullBright= 129,
	OWcParticle_Appearance_Sky			= 130,

	OWcParticle_Appearance_XScale_Set	= 200,
	OWcParticle_Appearance_YScale_Set	= 202,
	OWcParticle_Appearance_Rotation_Set	= 203,
	OWcParticle_Appearance_Alpha_Set	= 204,
	OWcParticle_Appearance_Texture_Set	= 205,
	OWcParticle_Appearance_XOffset_Set	= 213,
	OWcParticle_Appearance_XShorten_Set	= 214,
	OWcParticle_Appearance_Tint_Set		= 215,
	OWcParticle_Appearance_EdgeFadeMin_Set= 217,
	OWcParticle_Appearance_EdgeFadeMax_Set= 218,
	OWcParticle_Appearance_MaxContrail_Set= 219,
	OWcParticle_Appearance_LensDist_Set	= 220,
	OWcParticle_Appearance_LensIn_Set	= 221,
	OWcParticle_Appearance_LensOut_Set	= 222,
	OWcParticle_Appearance_MaxDecals_Set= 223,
	OWcParticle_Appearance_DecalFade_Set= 224,
	OWcParticle_Appearance_DecalWrap_Set= 225
};

// ------------------------------
// Particle Attractor dialog
// ------------------------------
enum
{
	OWcParticle_Attractor_Iterator				= 100,
	OWcParticle_Attractor_Selector				= 101,
	OWcParticle_Attractor_Name					= 102,
	OWcParticle_Attractor_Max_Distance			= 103,
	OWcParticle_Attractor_Max_Angle				= 104,
	OWcParticle_Attractor_AngleSelect_Min		= 105,
	OWcParticle_Attractor_AngleSelect_Max		= 106,
	OWcParticle_Attractor_AngleSelect_Weight	= 107,

	OWcParticle_Attractor_CheckWalls			= 108,

	OWcParticle_Attractor_Name_Set				= 202,
	OWcParticle_Attractor_Max_Distance_Set		= 203,
	OWcParticle_Attractor_Max_Angle_Set			= 204,
	OWcParticle_Attractor_AngleSelect_Min_Set	= 205,
	OWcParticle_Attractor_AngleSelect_Max_Set	= 206,
	OWcParticle_Attractor_AngleSelect_Weight_Set= 207
};

// ------------------------------
// Particle Value Integer dialog
// ------------------------------
enum
{
	OWcParticle_Value_Int_Const		= 100,
	OWcParticle_Value_Int_Range		= 101,
	OWcParticle_Value_Int_Variable	= 102,
	OWcParticle_Value_Int_ConstVal	= 200,
	OWcParticle_Value_Int_RangeLow	= 201,
	OWcParticle_Value_Int_RangeHi	= 202,
	OWcParticle_Value_Int_VarPopup	= 203,
	OWcParticle_Value_Int_Apply		= 400
};

// ------------------------------
// Particle Value Float dialog
// ------------------------------
enum
{
	OWcParticle_Value_Float_Const		= 100,
	OWcParticle_Value_Float_Range		= 101,
	OWcParticle_Value_Float_Bellcurve	= 102,
	OWcParticle_Value_Float_Variable	= 103,
	OWcParticle_Value_Float_Cycle		= 104,
	OWcParticle_Value_Float_ConstVal	= 200,
	OWcParticle_Value_Float_RangeLow	= 201,
	OWcParticle_Value_Float_RangeHi		= 202,
	OWcParticle_Value_Float_BellMean	= 203,
	OWcParticle_Value_Float_BellStddev	= 204,
	OWcParticle_Value_Float_VarPopup	= 205,
	OWcParticle_Value_Float_CycleLength	= 206,
	OWcParticle_Value_Float_CycleMax	= 207,
	OWcParticle_Value_Float_Apply		= 400
};


// ------------------------------
// Particle Value String dialog
// ------------------------------
enum
{
	OWcParticle_Value_String_Const		= 100,
	OWcParticle_Value_String_Variable	= 101,
	OWcParticle_Value_String_ConstVal	= 200,
	OWcParticle_Value_String_VarPopup	= 201,
	OWcParticle_Value_String_Apply		= 400
};

// ------------------------------
// Particle Value Shade dialog
// ------------------------------
enum
{
	OWcParticle_Value_Shade_Const		= 100,
	OWcParticle_Value_Shade_Range		= 101,
	OWcParticle_Value_Shade_Bellcurve	= 102,
	OWcParticle_Value_Shade_Variable	= 103,
	OWcParticle_Value_Shade_ConstVal	= 200,
	OWcParticle_Value_Shade_RangeLow	= 201,
	OWcParticle_Value_Shade_RangeHi		= 202,
	OWcParticle_Value_Shade_BellMean	= 203,
	OWcParticle_Value_Shade_BellStddev	= 204,
	OWcParticle_Value_Shade_VarPopup	= 205,
	OWcParticle_Value_Shade_Apply		= 400
};

// ------------------------------
// Particle Value Enum dialog
// ------------------------------
enum
{
	OWcParticle_Value_Enum_Const		= 100,
	OWcParticle_Value_Enum_Variable		= 101,
	OWcParticle_Value_Enum_ConstPopup	= 200,
	OWcParticle_Value_Enum_VarPopup		= 201,
	OWcParticle_Value_Enum_Apply		= 400
};

// ------------------------------
// Particle New Variable dialog
// ------------------------------
enum
{
	OWcParticle_NewVar_Name			= 100,
	OWcParticle_NewVar_Int			= 110,
	OWcParticle_NewVar_Float		= 111,
	OWcParticle_NewVar_String		= 112,
	OWcParticle_NewVar_Shade		= 113,
	OWcParticle_NewVar_Enum			= 114,
	OWcParticle_NewVar_EnumPopup	= 115
};

// ------------------------------
// Particle Rename Variable dialog
// ------------------------------
enum
{
	OWcParticle_RenameVar_Name		= 100
};

// ------------------------------
// Particle Actions dialog
// ------------------------------
enum
{
	OWcParticle_Actions_Listbox		= 100,
	OWcParticle_Actions_Insert		= 101,
	OWcParticle_Actions_Delete		= 102,
	OWcParticle_Actions_Edit		= 103,
	OWcParticle_Actions_Up			= 104,
	OWcParticle_Actions_Down		= 105,
	OWcParticle_Actions_Toggle		= 106,
	OWcParticle_Actions_IndexText	= 110,
	OWcParticle_Actions_IndexButton	= 111
};

// ------------------------------
// Particle Select Variable dialog
// ------------------------------
enum
{
	OWcParticle_VarRef_Popup		= 100,
	OWcParticle_VarRef_Apply		= 400
};

// ------------------------------
// Particle New Action dialog
// ------------------------------
enum
{
	OWcParticle_NewAction_Type		= 100
};

// ------------------------------
// Particle ActionList dialog
// ------------------------------
enum
{
	OWcParticle_ActionList_Start	= 100,
	OWcParticle_ActionList_End		= 101
};

// ------------------------------
// Particle Texture dialog
// ------------------------------
enum
{
	OWcParticle_Texture_Name		= 100,
	OWcParticle_Texture_Prompt		= 101,
	OWcParticle_Texture_Apply		= 400
};

// ------------------------------
// Particle Emitters dialog
// ------------------------------
enum
{
	OWcParticle_Emitters_Listbox	= 100,
	OWcParticle_Emitters_Insert		= 101,
	OWcParticle_Emitters_Delete		= 102,
	OWcParticle_Emitters_Edit		= 103
};

// ------------------------------
// Particle Emitter Flags dialog
// ------------------------------
enum
{
	OWcParticle_EmitterFlags_ClassName			= 100,
	OWcParticle_EmitterFlags_InitiallyOn		= 200,
	OWcParticle_EmitterFlags_IncreaseCount		= 201,
	OWcParticle_EmitterFlags_OffAtThreshold		= 202,
	OWcParticle_EmitterFlags_OnAtThreshold		= 203,
	OWcParticle_EmitterFlags_ParentVelocity		= 204,
	OWcParticle_EmitterFlags_DynamicMatrix		= 205,
	OWcParticle_EmitterFlags_OrientToVelocity	= 206,
	OWcParticle_EmitterFlags_InheritTint		= 207,
	OWcParticle_EmitterFlags_OnePerAttractor	= 208,
	OWcParticle_EmitterFlags_AtLeastOne			= 209,
	OWcParticle_EmitterFlags_RotateAttractors	= 210,

	OWcParticle_EmitterFlags_Apply				= 400
};

// ------------------------------
// Particle Emitter Choice dialog
// ------------------------------
enum
{
	OWcParticle_EmitterChoice_Text			= 100,
	OWcParticle_EmitterChoice_Popup			= 101
};

// ------------------------------
// Particle Event List dialog
// ------------------------------
enum
{
	OWcParticle_EventList_TextBase			= 100,
	OWcParticle_EventList_DescBase			= 200,
	OWcParticle_EventList_ButtonBase		= 300
};

// ------------------------------
// Particle Class Name dialog
// ------------------------------
enum
{
	OWcParticle_Class_Name					= 100
};

// ------------------------------
// Particle Display Type menu
// ------------------------------
enum
{
	OWcParticle_DisplayType_Sprite			= 200,
	OWcParticle_DisplayType_Geometry		= 201,
	OWcParticle_DisplayType_Vector			= 202,
	OWcParticle_DisplayType_Decal			= 203
};

// ------------------------------
// Particle Number dialog
// ------------------------------
enum
{
	OWcParticle_Number_Text					= 100,
	OWcParticle_Number_EditField			= 101,
	OWcParticle_Number_Apply				= 400
};

// ------------------------------
// Decal texture dialog
// ------------------------------
enum
{
	OWcParticle_Decal_Textures			= 100,
	OWcParticle_Decal_Picture			= 101,
	OWcParticle_Decal_Select			= 102,
	OWcParticle_Decal_Cancel			= 103
};

// ======================================================================
// structures
// ======================================================================
// information passed to a particle class editing dialog
typedef struct OWtParticle_Edit_Data {
	P3tParticleClass *				classptr;
} OWtParticle_Edit_Data;

// information about how to notify a particle list dialog that one of its
// lines must be updated
typedef struct OWtParticle_NotifyStruct {
	WMtWindow *						notify_window;
	UUtUns16						line_to_notify;
} OWtParticle_NotifyStruct;

// information passed to a particle value editing dialog
typedef struct OWtParticle_Value_Data {
	P3tParticleClass *				classptr;
	P3tValue *						valptr;
	UUtBool							variable_allowed;	// UUcFalse for variable initialisers
	P3tEnumTypeInfo					enum_info;			// only used for enum values
	char							title[64];
	OWtParticle_NotifyStruct		notify_struct;
} OWtParticle_Value_Data;

// information passed to a particle variable reference editing dialog
typedef struct OWtParticle_VarRef_Data {
	P3tParticleClass *				classptr;
	P3tVariableReference *			varptr;
	char							title[64];
	P3tDataType						allowed_types;
	OWtParticle_NotifyStruct		notify_struct;
} OWtParticle_VarRef_Data;

// information stored for each action in the action dialog
typedef struct OWtParticle_Action_Entry {
	P3tActionTemplate *				actiontemplate;		// template for the action
	UUtUns16 						actionpos;			// position of the action in the list
} OWtParticle_Action_Entry;

// information passed to / stored by the particle action dialog
typedef struct OWtParticle_Action_Data {
	P3tParticleClass *				classptr;
	UUtUns16						listindex;
	UUtUns16						num_actions;
	OWtParticle_Action_Entry *		action;
	OWtParticle_NotifyStruct		notify_struct;
} OWtParticle_Action_Data;

// information returned by a particle new-variable dialog
typedef struct OWtParticle_NewVar_Data {
	P3tString						name;
	P3tDataType						type;
} OWtParticle_NewVar_Data;

// information passed to the particle actionlist dialog
typedef struct OWtParticle_ActionList_Data {
	UUtUns16						max_actions;
	P3tActionList *					actionlistptr;
	char							title[64];
} OWtParticle_ActionList_Data;

// information passed to a texture name editing dialog
typedef struct OWtParticle_Texture_Data {
	P3tParticleClass *				classptr;
	OWtParticle_NotifyStruct		notify_struct;
	UUtUns32						name_type;
	char *							texture_name;
	char							title[64];
	char							editmsg[64];
} OWtParticle_Texture_Data;

// information passed to / stored by the particle emitters dialog
typedef struct OWtParticle_Emitter_Data {
	P3tParticleClass *				classptr;
	UUtUns16						num_emitters;
	UUtUns16 *						emitter_pos;
	UUtBool							set_percentage_chance;
	float							temp_percentage_chance;
	UUtUns16 *						store_percentage_chance;
} OWtParticle_Emitter_Data;

// information passed to the emitter flags dialog
typedef struct OWtParticle_EmitterFlags_Data {
	P3tParticleClass *				classptr;
	P3tEmitter *					emitter;
	OWtParticle_NotifyStruct		notify_struct;
} OWtParticle_EmitterFlags_Data;

// information passed to the emitter component-choice dialog
typedef struct OWtParticle_EmitterChoice_Data {
	P3tParticleClass *				classptr;
	P3tEmitter *					emitter;
	char *							classname;
	P3tEmitterSettingDescription *	class_desc;
	UUtUns32						current_choice;
	UUtUns32 *						new_choice;
} OWtParticle_EmitterChoice_Data;

// information passed to a number editing dialog
typedef struct OWtParticle_Number_Data {
	P3tParticleClass *				classptr;
	OWtParticle_NotifyStruct		notify_struct;
	UUtBool							is_float;
	union {
		UUtUns16 *						numberptr;
		float *							floatptr;
	} ptr;
	char							title[64];
	char							editmsg[64];
} OWtParticle_Number_Data;

// information passed to a linktype editing dialog
typedef struct OWtParticle_LinkType_Data {
	P3tParticleClass *				classptr;
	OWtParticle_NotifyStruct		notify_struct;
	P3tEmitter *					emitter;
	char							title[64];
	char							editmsg[64];
} OWtParticle_LinkType_Data;


// ======================================================================
// globals
// ======================================================================


// ======================================================================
// prototypes
// ======================================================================

// initialise the particle windowing system
UUtError OWrParticle_Initialize(void);

UUtBool
OWrParticle_Edit_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);
	
UUtBool
OWrParticle_Class_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);
	
UUtBool
OWrParticle_Variables_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);
	
UUtBool
OWrParticle_Appearance_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);
	
UUtBool
OWrParticle_Attractor_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);
	
UUtBool
OWrParticle_Value_Int_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_Value_Float_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_Value_String_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_Value_Shade_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_Value_Enum_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_NewVar_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_RenameVar_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);
	
UUtBool
OWrParticle_Actions_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_NewAction_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_VarRef_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_ActionList_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_Texture_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_Emitters_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_EmitterFlags_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_EmitterChoice_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_EventList_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_ClassName_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_Number_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtBool
OWrParticle_LinkType_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2);

UUtError OWrSave_Particles(UUtBool autosave);
UUtError OWrSave_Particle(P3tParticleClass *inClass);
UUtError OWrRevert_Particles(void);
UUtError OWrRevert_Particle(P3tParticleClass *inClass);

// ======================================================================
#endif /* ONI_WIN_PARTICLE_H */