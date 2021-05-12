// ======================================================================
// Oni_InGameUI.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Motoko.h"
#include "WM_Picture.h"
#include "WM_Button.h"
#include "BFW_Timer.h"

#include "Oni_Character.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_InGameUI.h"
#include "Oni_Sound2.h"
#include "Oni_Persistance.h"
 

// ======================================================================
// defines
// ======================================================================
#define ONcInGameUIName						"psui_oniUI"

#define ONcPauseScreen_VolumeCut			(0.5f)
#define ONcPauseScreen_OpenSound			"pause_screen_open"
#define ONcPauseScreen_CloseSound			"pause_screen_close"

#define ONcNewMoveMessage					"new_move"
#define ONcNewObjectiveMessage				"new_objective"
#define ONcNewObjectiveMessage_FadeTime		240
#define ONcObjective_PromptTime				30
#define ONcObjective_IgnoreAfterRestoreGame	300
#define ONcObjective_SuperimposedObjective	10.0f

#define ONcNewWeaponMessage_FadeTime		240

#define ONcPauseScreen_KeyAcceptDelay		(1)	// seconds

#define ONcIGUI_HealthWarnFlashAmount_Min	0.5f
#define ONcIGUI_HealthWarnFlashAmount_Max	0.3f
#define ONcIGUI_HealthWarnPeriod_Min		40
#define ONcIGUI_HealthWarnPeriod_Max		140
#define ONcIGUI_HealthWarnFlashPeriod_Min	40
#define ONcIGUI_HealthWarnFlashPeriod_Max	75

#define cInGunStart				(15.0f * M3cDegToRad)
#define cInGunEnd				(345.0f * M3cDegToRad)
#define cInGunOuterRadius		44.0f
#define cInGunInnerRadius		30.0f

#define cMaxNumAmmo				WPcMaxAmmoBallistic
#define cAmmoStart				(180.0f * M3cDegToRad)
#define cAmmoEnd				(270.0f * M3cDegToRad)
#define cAmmoOuterRadius		64.0f
#define cAmmoInnerRadius		51.5f
#define cAmmoColor				IMcShade_None

#define cMaxNumCells			WPcMaxAmmoEnergy
#define cCellStart				(180.0f * M3cDegToRad)
#define cCellEnd				(90.0f * M3cDegToRad)
#define cCellOuterRadius		64.0f
#define cCellInnerRadius		51.5f
#define cCellColor				IMcShade_None

#define cMaxSuperHealth			2.0f
#define cSuperHealthStart		(-90.0f * M3cDegToRad)
#define cSuperHealthEnd			(-170.0f * M3cDegToRad)
#define cSuperHealthOuterRadius	55.0f
#define cSuperHealthInnerRadius	40.0f

#define cHealthGrowRate			3
#define cHealthStart			(180.0f * M3cDegToRad)
#define cHealthEnd				(-90.0f * M3cDegToRad)
#define cHealthOuterRadius		50.0f
#define cHealthInnerRadius		40.0f
#define cHealthShadowDecayTime	180
#define cHealthForeShadowAlpha	80
#define cHealthAfterShadowAlpha	127

#define cMaxNumHypos			WPcMaxHypos
#define cHyposOuterRadius		63.0f
#define cHyposInnerRadius		51.0f
#define cHypoColor				IMcShade_None

#define cMaxNumHits				4
#define cHitsDecrementPerTick	3
#define cHitOuterRadius			37.0f
#define cHitInnerRadius			30.0f
#define cHitColor				IMcShade_None

#define cInvisStart				(-2 * M3cDegToRad)
#define cInvisEnd				(-89 * M3cDegToRad)
#define cInvisOuterRadius		63.0f
#define cInvisInnerRadius		51.0f
#define cInvisColor				IMcShade_None

#define cShieldStart			(2 * M3cDegToRad)
#define cShieldEnd				(89 * M3cDegToRad)
#define cShieldOuterRadius		63.0f
#define cShieldInnerRadius		51.0f
#define cShieldColor			IMcShade_None
#define cShieldShadowDecayTime	180
#define cShieldShadowAlpha		127

#define cMaxArcLength			(M3cPi)
#define cMinArcLength			(20.0f * M3cDegToRad)
#define cRadarStart				0
#define cRadarEnd				(M3cHalfPi * 0.5f)
#define cRadarOuterRadius		50.0f
#define cRadarInnerRadius		46.0f
#define cRadarColor				IMcShade_None

#define cUpDnArrowDiameter		12.0f
#define cUpDnArrowOffsetX		36.5f
#define cUpDnHeightUnits		10.0f

#define cVitStart				(90.25f * M3cDegToRad)
#define cVitEnd					(179.75f * M3cDegToRad)
#define cVitOuterRadius			232.0f
#define cVitInnerRadius			197.65f

#define cIconDrawnDiameter		54.0f

#define cLSIIconDiameter		54.0f

#define cObjectiveCompleteColor	IMcShade_Red
#define cObjectiveCurrentColor	IMcShade_Green

#define cHelpTextXOffset		3
#define cHelpTextFontSize		TScFontSize_Default
#define cHUDHelpTextShade		IMcShade_Green
#define cHUDHelpTextShadow		IMcShade_Gray25

#define ONcKeyWidth				32
#define ONcKeySpace				2

#define ONcIGU_MaxDiaryPages	60
#define ONcIGU_MaxItems			10
#define ONcIGU_MaxWeapons		15
#define ONcIGU_MaxHelpPages		10

#define ONcIGU_HelpPageInstance	"help_pg_01"

#define ONcMaxFIStackItems		5

// ======================================================================
// enums
// ======================================================================
enum
{
	ONcPauseScreenID			= 70,
	ONcTextConsoleID			= 71
};

enum
{
	ONcPSState_None,
	ONcPSState_Objective,
	ONcPSState_Items,
	ONcPSState_Weapons,
	ONcPSState_Diary,
	ONcPSState_Help
};

enum
{
	ONcPS_Btn_Objective			= 101,
	ONcPS_Btn_Items				= 102,
	ONcPS_Btn_Weapons			= 103,
	ONcPS_Btn_Diary				= 104,
	ONcPS_Btn_Help				= 105,
	ONcPS_Btn_Previous			= 106,
	ONcPS_Btn_Next				= 107,
	ONcPS_Txt_MainArea			= 110,
	ONcPS_Txt_SubArea			= 111,
	ONcPS_Btn_HelpPrevious		= 112,
	ONcPS_Btn_HelpNext			= 113,
	ONcPS_Box_SubArea			= 120
};

enum
{
	ONcTC_Pt_Picture			= 100,
	ONcTC_Txt_Text				= 110,			// Must match ONcPS_Txt_MainArea
	ONcTC_Btn_Next				= 102,
	ONcTC_Btn_Close				= WMcDialogItem_Cancel
};

enum
{
	ONcIGUIElement_Health		= 0,
	ONcIGUIElement_DamageRing	= 1,
	ONcIGUIElement_Invisibility	= 2,
	ONcIGUIElement_Shield		= 3,
	ONcIGUIElement_Hypos		= 4,
	ONcIGUIElement_Weapon		= 5,
	ONcIGUIElement_CurrentAmmo	= 6,
	ONcIGUIElement_CompassRing	= 7,
	ONcIGUIElement_CompassArrow	= 8,
	ONcIGUIElement_BallisticAmmo= 9,
	ONcIGUIElement_EnergyCells	= 10,
	ONcIGUIElement_ItemDisplay	= 11,

	ONcIGUIElement_Max			= 12
};

#define ONcIGUIElement_All				((1 << ONcIGUIElement_Max) - 1)
#define ONmIGUIElement_Filled(elem)		((gElementFilled & (1 << elem)) > 0)
#define ONmIGUIElement_Flashing(elem)	((gElementFlashing & (1 << elem)) > 0)

// ======================================================================
// typedef
// ======================================================================
typedef struct ONtInGameUI_Arc
{
	float						outer_radius;
	float						inner_radius;
	UUtUns32					num_segments;
	UUtUns32					num_points;
	M3tPointScreen				center;
	M3tPointScreen				*outer_points;
	M3tPointScreen				*inner_points;
	M3tTextureCoord				*outer_UVs;
	M3tTextureCoord				*inner_UVs;
	M3tTextureMap				*texture;
	
} ONtInGameUI_Arc;

typedef struct tStartEnd
{
	float						start;
	float						end;
} tStartEnd;

typedef struct ONtPauseScreenData
{
	UUtUns32					state;
	UUtUns32					highlighted_sections;
	UUtUns32					start_time;
	UUtInt16					current_level;
	
	ONtObjectivePage			*objective;
	UUtInt16					objective_number;
	UUtInt16					objective_page_num;
	
	ONtDiaryPage				*diary[ONcIGU_MaxDiaryPages];
	UUtInt16					num_diary_pages;
	UUtInt16					diary_page_num;
	
	ONtItemPage					*items[ONcIGU_MaxItems];
	UUtInt16					num_items;
	UUtInt16					item_page_num;
	
	ONtWeaponPage				*weapons[ONcIGU_MaxWeapons];
	UUtInt16					num_weapon_pages;
	UUtInt16					weapon_page_num;
	
	ONtHelpPage					*help[ONcIGU_MaxHelpPages];
	UUtInt16					num_help_pages;
	UUtInt16					help_page_num;
	
} ONtPauseScreenData;

typedef struct ONtInGameUI_Meter
{
	float						percentage;
	M3tTextureMap				*texture;
	M3tPointScreen				*original_primary_points;
	M3tPointScreen				*original_secondary_points;
	M3tPointScreen				*draw_primary_points;
	M3tPointScreen				*draw_secondary_points;
	M3tTextureCoord				*draw_primary_UVs;
	M3tTextureCoord				*draw_secondary_UVs;
	
} ONtInGameUI_Meter;

typedef struct ONtTextConsoleData
{
	ONtTextConsole				*text_console;
	UUtInt32					page;
	
} ONtTextConsoleData;

typedef struct ONtIGUIElementDescriptionEntry
{
	UUtUns32					element;
	const char					*name;

} ONtIGUIElementDescriptionEntry;

typedef struct ONtPauseScreenHighlightableButton
{
	UUtUns16					window_id;
	UUtUns16					highlight_bit;
} ONtPauseScreenHighlightableButton;

// ======================================================================
// globals
// ======================================================================
static UUtInt16					ONgObjectiveNumber;
static UUtInt16					ONgDiaryPageUnlocked;
static UUtBool					ONgInGameUI_EnableHelp;
static WPtWeaponClass			*ONgInGameUI_NewWeaponClass;
static WPtPowerupType			ONgInGameUI_NewItem;

static UUtUns32					ONgInGameUI_LastRestoreGame;
static UUtBool					ONgInGameUI_NewObjective;
static UUtBool					ONgInGameUI_NewMove;
static UUtBool					ONgInGameUI_ImmediatePrompt;
static UUtUns32					ONgInGameUI_LastPromptTime;
static UUtBool					ONgInGameUI_SuppressPrompting;

static M3tTextureCoord			gFullTextureUVs[4];
static M3tTextureCoord			gFullTextureUVsInvert[4];

static M3tTextureMap			*gLeft;
static M3tTextureMap			*gRight;
static M3tTextureMap			*gLeftRing;
static M3tTextureMap			*gRightRing;
static M3tTextureMap			*gWhite;
static M3tTextureMap			*gLSIIcon;
static M3tTextureMap			*gUpDnArrow;

static ONtInGameUI_Arc			*gHealthArc[4];		// health, super health, shadow, super shadow
static ONtInGameUI_Arc			*gHyposArcs[cMaxNumHypos];
static ONtInGameUI_Arc			*gHitArcs[4];
static ONtInGameUI_Arc			*gInvisArc;
static ONtInGameUI_Arc			*gShieldArc[2];		// shield, shadow

static ONtInGameUI_Arc			*gAmmoArc;
static ONtInGameUI_Arc			*gCellArc;
static ONtInGameUI_Arc			*gInGunArc;
static ONtInGameUI_Arc			*gRadarArc;

static M3tPointScreen			gLeftCenter;
static M3tPointScreen			gRightCenter;

static UUtBool					gHasLSITarget;
static M3tPoint3D				gLSITarget;

// current enable flags (for partially showing or hiding UI)
static UUtBool					gLeftEnable;
static UUtBool					gRightEnable;
static UUtUns32					gElementFilled;
static UUtUns32					gElementFlashing;
static UUtUns32					gFlashAlpha;
static UUtUns32					gFlashStartTime;

// current state of meters
static UUtUns16					gCurNumRounds;
static UUtUns16					gCurMaxRounds;
static UUtUns16					gPrevInvisibilityRemaining;

// currently tracked health and shield values
typedef struct ONtInGame_DamageMeter {
	UUtBool					needs_update;

	UUtUns32				value, target_val;
	UUtBool					has_foreshadow, has_aftershadow;
	float					shadow_val;
	UUtBool					draw_arc[4];
} ONtInGame_DamageMeter;

ONtInGame_DamageMeter			gHealth;
ONtInGame_DamageMeter			gShield;

static ONtIGUI_HUDHelp			*gHUDHelp;
static TStTextContext			*gHUDHelpTextContext;

static tStartEnd				cHyposStartEnd[cMaxNumHypos] =
{
	{ 92 * M3cDegToRad, 103 * M3cDegToRad },
	{ 107 * M3cDegToRad, 118 * M3cDegToRad },
	{ 123 * M3cDegToRad, 133 * M3cDegToRad },
	{ 137 * M3cDegToRad, 148 * M3cDegToRad },
	{ 152 * M3cDegToRad, 163 * M3cDegToRad },
	{ 167 * M3cDegToRad, 179 * M3cDegToRad }
};

static tStartEnd				cHitStartEnd[cMaxNumHits] =
{
	{ -45 * M3cDegToRad, 45 * M3cDegToRad },	// right
	{ 45 * M3cDegToRad, 135 * M3cDegToRad },	// front
	{ 135 * M3cDegToRad, 225 * M3cDegToRad },	// left
	{ 225 * M3cDegToRad, 315 * M3cDegToRad } 	// back
};

static M3tPointScreen			gMeter1_Primary[4] =
{
	{ 81.0f, 44.0f, 0.5f, 2.0f },
	{ 311.0f, 44.0f, 0.5f, 2.0f },
	{ 311.0f, 64.0f, 0.5f, 2.0f },
	{ 81.0f, 64.0f, 0.5f, 2.0f },
};

static M3tPointScreen			gMeter1_Secondary[4] =
{
	{ 193.0f, 25.0f, 0.5f, 2.0f },
	{ 311.0f, 25.0f, 0.5f, 2.0f },
	{ 311.0f, 44.0f, 0.5f, 2.0f },
	{ 175.0f, 44.0f, 0.5f, 2.0f },
};

static M3tPointScreen			gMeter2_Primary[4] =
{
	{ 627.0f, 47.0f, 0.5f, 2.0f },
	{ 322.0f, 47.0f, 0.5f, 2.0f },
	{ 322.0f, 64.0f, 0.5f, 2.0f },
	{ 627.0f, 64.0f, 0.5f, 2.0f },
};

static M3tPointScreen			gMeter2_Secondary[4] =
{
	{ 435.0f, 25.0f, 0.5f, 2.0f },
	{ 322.0f, 25.0f, 0.5f, 2.0f },
	{ 322.0f, 47.0f, 0.5f, 2.0f },
	{ 457.0f, 47.0f, 0.5f, 2.0f },
};

static M3tPointScreen			gMeter3_Primary[4] =
{
	{ 327.0f, 453.0f, 0.5f, 2.0f },
	{ 629.0f, 453.0f, 0.5f, 2.0f },
	{ 629.0f, 470.0f, 0.5f, 2.0f },
	{ 327.0f, 470.0f, 0.5f, 2.0f },
};

static M3tPointScreen			gMeter3_Secondary[4] =
{
	{ 521.0f, 436.0f, 0.5f, 2.0f },
	{ 629.0f, 436.0f, 0.5f, 2.0f },
	{ 629.0f, 453.0f, 0.5f, 2.0f },
	{ 504.0f, 453.0f, 0.5f, 2.0f },
};

static M3tPointScreen			gMeter4_Primary[4] =
{
	{ 81.0f, 77.0f, 0.5f, 2.0f },
	{ 109.0f, 77.0f, 0.5f, 2.0f },
	{ 109.0f, 234.0f, 0.5f, 2.0f },
	{ 81.0f, 234.0f, 0.5f, 2.0f },
};

static M3tPointScreen			gMeter4_Secondary[4] =
{
	{ 109.0f, 194.0f, 0.5f, 2.0f },
	{ 122.0f, 207.0f, 0.5f, 2.0f },
	{ 122.0f, 234.0f, 0.5f, 2.0f },
	{ 109.0f, 234.0f, 0.5f, 2.0f },
};

static ONtIGUIElementDescriptionEntry	gElementDescription[] =
{
	{ ONcIGUIElement_Health,		"health" },
	{ ONcIGUIElement_DamageRing,	"damage" },
	{ ONcIGUIElement_Invisibility,	"invis" },
	{ ONcIGUIElement_Shield,		"shield" },
	{ ONcIGUIElement_Hypos,			"hypo" },
	{ ONcIGUIElement_Weapon,		"weapon" },
	{ ONcIGUIElement_CurrentAmmo,	"ammo" },
	{ ONcIGUIElement_CompassRing,	"compass" },
	{ ONcIGUIElement_CompassArrow,	"arrow" },
	{ ONcIGUIElement_BallisticAmmo,	"ballistic" },
	{ ONcIGUIElement_EnergyCells,	"energy" },
	{ ONcIGUIElement_ItemDisplay,	"lsi" },

	{ 0,							NULL }
};

static ONtPauseScreenHighlightableButton	gPSHighlightableButtons[] =
{
	{ ONcPS_Btn_Objective,			ONcPSState_Objective },
	{ ONcPS_Btn_Items,				ONcPSState_Items },
	{ ONcPS_Btn_Weapons,			ONcPSState_Weapons },
	{ ONcPS_Btn_Diary,				ONcPSState_Diary },
	{ ONcPS_Btn_Help,				ONcPSState_Help },

	{ 0,							0 }
};

static ONtPauseScreenData		ONgPauseScreenData;
static ONtKeyIcons				*ONgKeyIcons;
static const char				*ONgPauseScreenOverrideMessage;

static float					gMaxDistanceSquared;
static float					gMinDistanceSquared;

static TStFontInfo				ONgFIStack[ONcMaxFIStackItems];
static UUtInt32					ONgFIStackIndex;


// ======================================================================
// prototypes
// ======================================================================
static UUtError
ONiTextConsole_Display(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiInGameUI_SetHelpDisplayEnable(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiInGameUI_ShowElement(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiInGameUI_FillElement(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiInGameUI_FlashElement(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStall,
	SLtParameter_Actual		*ioReturnValue);


// void 
static UUtBool ONrGameState_EventSound_InCutscene(void) 
{
	UUtBool result = ONgGameState->local.in_cutscene;

	result = result || (ONgGameState->gameTime < 30);

	return result;
}

// ======================================================================
// function
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiInGameUI_DrawArc(
	ONtInGameUI_Arc				*ioArc,
	UUtUns32					inNumSegmentsToDraw,
	IMtShade					inShade,
	UUtUns8						inAlpha)
{
	M3tPointScreen				dest[3];
	M3tTextureCoord				uv[3];
	UUtUns32					i;
	
	// set the shade, alpha and texture
	M3rDraw_State_Push();
	M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, inAlpha);
	if (inShade != IMcShade_None)
	{
		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, inShade);
	}
	M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, ioArc->texture);
	M3rDraw_State_Commit();
	
	// draw the arc
	for (i = 0; i < inNumSegmentsToDraw; i++)
	{
		dest[0] = ioArc->inner_points[i];
		dest[1] = ioArc->outer_points[i];
		dest[2] = ioArc->outer_points[i + 1];
		uv[0] = ioArc->inner_UVs[i];
		uv[1] = ioArc->outer_UVs[i];
		uv[2] = ioArc->outer_UVs[i + 1];
		M3rDraw_TriSprite(dest, uv);

		dest[1] = ioArc->outer_points[i + 1];
		dest[2] = ioArc->inner_points[i + 1];
		uv[1] = ioArc->outer_UVs[i + 1];
		uv[2] = ioArc->inner_UVs[i + 1];
		M3rDraw_TriSprite(dest, uv);
	}

	M3rDraw_State_Pop();
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_CalcArc(
	float						inStart,	// radians
	float						inEnd,		// radians
	ONtInGameUI_Arc				*ioArc)
{
	float						radians_per_segment;
	UUtUns32					i;
	float						uv_outer_radius;
	float						uv_inner_radius;

	radians_per_segment = (inEnd - inStart) / ioArc->num_segments;
	
	uv_outer_radius = ioArc->outer_radius / (float)(ioArc->texture->width >> 1);
	uv_inner_radius = ioArc->inner_radius / (float)(ioArc->texture->width >> 1);
	
	// calculate the arc points
	for (i = 0; i < ioArc->num_points; i++)
	{
		float		theta;
		float		cos_theta;
		float		sin_theta;
		
		theta = (inStart + (i * radians_per_segment));
		while (theta < 0.0f) { theta += M3c2Pi; }
		while (theta > M3c2Pi) { theta -= M3c2Pi; }
		cos_theta = MUrCos(theta);
		sin_theta = MUrSin(theta);
		
		ioArc->outer_points[i].x = (cos_theta * ioArc->outer_radius) + ioArc->center.x;
		ioArc->outer_points[i].y = (sin_theta * ioArc->outer_radius) + ioArc->center.y;
		ioArc->outer_points[i].z = 0.5f;
		ioArc->outer_points[i].invW = 2.0f;
		
		ioArc->inner_points[i].x = (cos_theta * ioArc->inner_radius) + ioArc->center.x;
		ioArc->inner_points[i].y = (sin_theta * ioArc->inner_radius) + ioArc->center.y;
		ioArc->inner_points[i].z = 0.5f;
		ioArc->inner_points[i].invW = 2.0f;
		
		ioArc->outer_UVs[i].u = (cos_theta * 0.5f) * uv_outer_radius + 0.5f;
		ioArc->outer_UVs[i].v = (sin_theta * 0.5f) * uv_outer_radius + 0.5f;
		
		ioArc->inner_UVs[i].u = (cos_theta * 0.5f) * uv_inner_radius + 0.5f;
		ioArc->inner_UVs[i].v = (sin_theta * 0.5f) * uv_inner_radius + 0.5f;
	}
}

// ----------------------------------------------------------------------
static UUtError
ONiInGameUI_MakeArc(
	float						inStart,	// radians
	float						inEnd,		// radians
	float						inOuterRadius,
	float						inInnerRadius,
	M3tPointScreen				*inCenter,
	UUtUns32					inNumSegments,
	M3tTextureMap				*inTextureMap,
	ONtInGameUI_Arc				**outArc)
{
	UUtUns32					num_points;
	ONtInGameUI_Arc				*arc;
	
	UUmAssert(inCenter);
	UUmAssert(inTextureMap);
	UUmAssert(outArc);
	
	num_points = inNumSegments + 1;
	
	// allocate memory for the arc
	arc =
		UUrMemory_Block_NewClear(
			sizeof(ONtInGameUI_Arc) +
			((sizeof(M3tPointScreen) * num_points) * 2) +
			((sizeof(M3tTextureCoord) * num_points) * 2));
	UUmError_ReturnOnNull(arc);
	
	// set up the vars
	arc->outer_radius = inOuterRadius;
	arc->inner_radius = inInnerRadius;
	arc->center = *inCenter;
	arc->texture = inTextureMap;
	arc->num_segments = inNumSegments;
	arc->num_points = num_points;
	arc->outer_points = (M3tPointScreen*)(((UUtUns8*)arc) + sizeof(ONtInGameUI_Arc));
	arc->inner_points = (arc->outer_points + num_points);
	arc->outer_UVs = (M3tTextureCoord*)(arc->inner_points + num_points);
	arc->inner_UVs = (arc->outer_UVs + num_points);
	
	ONiInGameUI_CalcArc(inStart, inEnd, arc);
	
	*outArc = arc;
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_SetArcTexture(
	ONtInGameUI_Arc				*inArc,
	M3tTextureMap				*inTextureMap)
{
	UUmAssert(inArc);
	UUmAssert(inTextureMap);
	
	// the texture must be the same size as the previous texture
	UUmAssert((inTextureMap->width == inArc->texture->width) &&
				(inTextureMap->width == inArc->texture->width));
	
	inArc->texture = inTextureMap;
}
	
// ----------------------------------------------------------------------
static void
ONiInGameUI_DrawMeter(
	ONtInGameUI_Meter			*inMeter,
	IMtShade					inShade)
{
	M3tPointScreen				dest[3];
	M3tTextureCoord				uv[3];
	
	// set the shade and texture
	M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, inShade);
	M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, inMeter->texture);
	M3rDraw_State_Commit();
	
	dest[0] = inMeter->draw_primary_points[3];
	dest[1] = inMeter->draw_primary_points[0];
	dest[2] = inMeter->draw_primary_points[1];
	uv[0] = inMeter->draw_primary_UVs[3];
	uv[1] = inMeter->draw_primary_UVs[0];
	uv[2] = inMeter->draw_primary_UVs[1];
	M3rDraw_TriSprite(dest, uv);

	dest[1] = inMeter->draw_primary_points[1];
	dest[2] = inMeter->draw_primary_points[2];
	uv[1] = inMeter->draw_primary_UVs[1];
	uv[2] = inMeter->draw_primary_UVs[2];
	M3rDraw_TriSprite(dest, uv);

	dest[0] = inMeter->draw_secondary_points[3];
	dest[1] = inMeter->draw_secondary_points[0];
	dest[2] = inMeter->draw_secondary_points[1];
	uv[0] = inMeter->draw_secondary_UVs[3];
	uv[1] = inMeter->draw_secondary_UVs[0];
	uv[2] = inMeter->draw_secondary_UVs[1];
	M3rDraw_TriSprite(dest, uv);

	dest[1] = inMeter->draw_secondary_points[1];
	dest[2] = inMeter->draw_secondary_points[2];
	uv[1] = inMeter->draw_secondary_UVs[1];
	uv[2] = inMeter->draw_secondary_UVs[2];
	M3rDraw_TriSprite(dest, uv);
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_CalcMeter(
	float						inPercentage,
	ONtInGameUI_Meter			*inMeter)
{
	UUtBool						is_vertical;
	float						h_diff;
	float						v_diff;
	
	h_diff = inMeter->original_primary_points[1].x - inMeter->original_primary_points[0].x;
	v_diff = inMeter->original_primary_points[3].y - inMeter->original_primary_points[0].y;
	
	is_vertical = (fabs(v_diff) > fabs(h_diff)) ? UUcTrue : UUcFalse;
	
	if (is_vertical)
	{
		float					sec_v_diff;
		
		// copy [0], and [1]
		inMeter->draw_primary_points[0] = inMeter->original_primary_points[0];
		inMeter->draw_primary_points[1] = inMeter->original_primary_points[1];
		
		// calc [3], and [2]
		inMeter->draw_primary_points[3].x = inMeter->draw_primary_points[0].x;
		inMeter->draw_primary_points[3].y = inMeter->draw_primary_points[0].y + (v_diff * inPercentage);

		inMeter->draw_primary_points[2].x = inMeter->draw_primary_points[1].x;
		inMeter->draw_primary_points[2].y = inMeter->draw_primary_points[1].y + (v_diff * inPercentage);
		
		// calculate the secondary points
		sec_v_diff = inMeter->original_secondary_points[0].y - inMeter->draw_primary_points[2].y;
		if (((sec_v_diff < 0) && (v_diff > 0)) || ((sec_v_diff > 0) && (v_diff < 0)))
		{
			// copy [0], and [1]
			inMeter->draw_secondary_points[0] = inMeter->original_secondary_points[0];
			inMeter->draw_secondary_points[1] = inMeter->original_secondary_points[1];

			// calc [2]
			inMeter->draw_secondary_points[2].x = inMeter->original_secondary_points[2].x;
			inMeter->draw_secondary_points[2].y = inMeter->draw_primary_points[2].y;
			
			// calc [3]
			inMeter->draw_secondary_points[3].x = inMeter->original_secondary_points[3].x;
			inMeter->draw_secondary_points[3].y = inMeter->draw_primary_points[2].y;
			
			// clip
			sec_v_diff = inMeter->draw_secondary_points[2].y - inMeter->original_secondary_points[1].y;
			if (((sec_v_diff < 0) && (v_diff > 0)) || ((sec_v_diff > 0) && (v_diff < 0)))
			{
				inMeter->draw_secondary_points[2].x =
					inMeter->draw_secondary_points[0].x +
					(float)fabs(inMeter->draw_secondary_points[3].y - inMeter->draw_secondary_points[0].y);
				inMeter->draw_secondary_points[1] = inMeter->draw_secondary_points[2];
			}
		}
	}
	else
	{
		float					sec_h_diff;
		
		// copy [0], and [3]
		inMeter->draw_primary_points[0] = inMeter->original_primary_points[0];
		inMeter->draw_primary_points[3] = inMeter->original_primary_points[3];
		
		// calc [1], and [2]
		inMeter->draw_primary_points[1].x = inMeter->draw_primary_points[0].x + (h_diff * inPercentage);
		inMeter->draw_primary_points[1].y = inMeter->draw_primary_points[0].y;

		inMeter->draw_primary_points[2].x = inMeter->draw_primary_points[3].x + (h_diff * inPercentage);
		inMeter->draw_primary_points[2].y = inMeter->draw_primary_points[3].y;

		// calculate the secondary points
		sec_h_diff = inMeter->original_secondary_points[3].x - inMeter->draw_primary_points[1].x;
		if (((sec_h_diff < 0) && (h_diff > 0)) || ((sec_h_diff > 0) && (h_diff < 0)))
		{
			// copy [0], and [3]
			inMeter->draw_secondary_points[0] = inMeter->original_secondary_points[0];
			inMeter->draw_secondary_points[3] = inMeter->original_secondary_points[3];
			
			// calc [2]
			inMeter->draw_secondary_points[2].x = inMeter->draw_primary_points[1].x;
			inMeter->draw_secondary_points[2].y = inMeter->original_secondary_points[2].y;
			
			// calc[1]
			inMeter->draw_secondary_points[1].x = inMeter->draw_secondary_points[2].x;
			inMeter->draw_secondary_points[1].y = inMeter->original_secondary_points[1].y;
			
			// clip
			sec_h_diff = inMeter->draw_secondary_points[1].x - inMeter->draw_secondary_points[0].x;
			if (((sec_h_diff < 0) && (h_diff > 0)) || ((sec_h_diff > 0) && (h_diff < 0)))
			{
				inMeter->draw_secondary_points[1].y =
					inMeter->draw_secondary_points[3].y -
					(float)fabs(inMeter->draw_secondary_points[2].x - inMeter->draw_secondary_points[3].x);
				inMeter->draw_secondary_points[0] = inMeter->draw_secondary_points[1];
			}
		}
	}
}

// ----------------------------------------------------------------------
static UUtError
ONiInGameUI_MakeMeter(
	M3tPointScreen				*inPrimaryPoints,
	M3tPointScreen				*inSecondaryPoints,
	M3tTextureMap				*inTextureMap,
	float						inPercentage,
	ONtInGameUI_Meter			**outMeter)
{
	ONtInGameUI_Meter			*meter;
	UUtUns32					i;
	
	UUmAssert(inPrimaryPoints);
	UUmAssert(inSecondaryPoints);
	UUmAssert(inTextureMap);
	UUmAssert(outMeter);
	
	// allocate memory for the meter
	meter =
		UUrMemory_Block_NewClear(
			sizeof(ONtInGameUI_Arc) +
			(sizeof(M3tPointScreen) * 4) +						// original primary points
			(sizeof(M3tPointScreen) * 4) +						// original secondary points
			(sizeof(M3tPointScreen) * 4) +						// draw primary points
			(sizeof(M3tPointScreen) * 4) +						// draw secondary points
			(sizeof(M3tTextureCoord) * 4) +						// draw	primary UVs
			(sizeof(M3tTextureCoord) * 4));						// draw	secondary UVs
	UUmError_ReturnOnNull(meter);
	
	// set up the vars
	meter->texture = inTextureMap;
	meter->original_primary_points = (M3tPointScreen*)(((UUtUns8*)meter) + sizeof(ONtInGameUI_Meter));
	meter->original_secondary_points = (meter->original_primary_points + 4);
	meter->draw_primary_points = (meter->original_secondary_points + 4);
	meter->draw_secondary_points = (meter->draw_primary_points + 4);
	meter->draw_primary_UVs = (M3tTextureCoord*)(meter->draw_secondary_points + 4);
	meter->draw_secondary_UVs = (meter->draw_primary_UVs + 4);
	
	// save the points
	for (i = 0; i < 4; i++)
	{
		meter->original_primary_points[i] = inPrimaryPoints[i];
		meter->original_secondary_points[i] = inSecondaryPoints[i];
	}

	ONiInGameUI_CalcMeter(inPercentage, meter);
	
	*outMeter = meter;
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayAmmoClips(
	ONtCharacter				*inPlayer)
{
	UUtUns16					draw_clips;
	UUtUns32					alpha;

	alpha = ONmIGUIElement_Flashing(ONcIGUIElement_BallisticAmmo) ? gFlashAlpha : M3cMaxAlpha;

	// set the number of segments to draw
	if (ONmIGUIElement_Filled(ONcIGUIElement_BallisticAmmo)) {
		draw_clips = cMaxNumAmmo;
	} else {
		draw_clips = UUmMin(cMaxNumAmmo, inPlayer->inventory.ammo);
	}
	
	if (draw_clips == 0) { return; }
	
	// draw the arc
	ONiInGameUI_DrawArc(gAmmoArc, draw_clips, cAmmoColor, (UUtUns8) alpha);
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayCellClips(
	ONtCharacter				*inPlayer)
{
	UUtUns16					draw_clips;
	UUtUns32					alpha;
	
	alpha = (ONmIGUIElement_Flashing(ONcIGUIElement_EnergyCells) ? gFlashAlpha : M3cMaxAlpha);

	// set the number of segments to draw
	if (ONmIGUIElement_Filled(ONcIGUIElement_EnergyCells)) {
		draw_clips = cMaxNumCells;
	} else {
		draw_clips = UUmMin(cMaxNumCells, inPlayer->inventory.cell);
	}

	if (draw_clips == 0) { return; }
	
	// draw the arc
	ONiInGameUI_DrawArc(gCellArc, draw_clips, cCellColor, (UUtUns8) alpha);
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayHealth(
	ONtCharacter				*inPlayer)
{
	float						health_percentage, super_health_percentage, shadow_percentage, super_shadow_percentage;
	float						low_health_amount, low_health_period, low_health_flashperiod, flash_t, flash_amount;
	IMtShade					health_shade;
	UUtUns32					itr, health_colorval;
	UUtUns16					main_alpha, shadow_alpha;
	
	if (ONmIGUIElement_Filled(ONcIGUIElement_Health)) {
		// draw a full health bar
		gHealth.draw_arc[0] = UUcTrue;
		gHealth.draw_arc[1] = UUcTrue;
		gHealth.draw_arc[2] = UUcFalse;
		gHealth.draw_arc[3] = UUcFalse;
		ONiInGameUI_CalcArc(cHealthStart, cHealthEnd, gHealthArc[0]);
		ONiInGameUI_CalcArc(cSuperHealthStart, cSuperHealthEnd, gHealthArc[1]);

		gHealth.needs_update = UUcTrue;

	} else if (gHealth.needs_update) {
		float						start;
		float						end;

		if (gHealth.value == 0) {
			// don't draw main health arc
			health_percentage = 0;
			super_health_percentage = 0;
			gHealth.draw_arc[0] = gHealth.draw_arc[1] = 0;
		} else {
			health_percentage = ((float) gHealth.value) / inPlayer->maxHitPoints;

			// work out primary arc
			start = cHealthStart;
			end = start + ((cHealthEnd - start) * UUmMin(1.0f, health_percentage));
		
			gHealth.draw_arc[0] = UUcTrue;
			ONiInGameUI_CalcArc(start, end, gHealthArc[0]);

			if (health_percentage > 1.0f)
			{
				// work out secondary arc
				super_health_percentage = (UUmMin(health_percentage, ONgGameSettings->boosted_health) - 1.0f);
				
				start = cSuperHealthStart;
				end = start + ((cSuperHealthEnd - start) * super_health_percentage);
				
				gHealth.draw_arc[1] = UUcTrue;
				ONiInGameUI_CalcArc(start, end, gHealthArc[1]);
			}
			else
			{
				super_health_percentage = 0;
				gHealth.draw_arc[1] = UUcFalse;
			}
		}

		if (gHealth.has_aftershadow || gHealth.has_foreshadow) {
			// compute the amount of shadow health
			shadow_percentage = gHealth.shadow_val / inPlayer->maxHitPoints;

			if (health_percentage > 1.0f) {
				// normal health is over 100%, there is no primary shadow arc
				gHealth.draw_arc[2] = UUcFalse;
			} else {
				// work out primary shadow arc
				start = cHealthStart + health_percentage * (cHealthEnd - cHealthStart);
				end = cHealthStart + UUmMin(shadow_percentage, 1.0f) * (cHealthEnd - cHealthStart);
			
				gHealth.draw_arc[2] = UUcTrue;
				ONiInGameUI_CalcArc(start, end, gHealthArc[2]);
			}

			if (shadow_percentage > 1.0f)
			{
				// work out secondary shadow arc
				super_shadow_percentage = (UUmMin(shadow_percentage, ONgGameSettings->boosted_health) - 1.0f);
				
				if (super_health_percentage > 0.0f) {
					start = cSuperHealthStart + super_health_percentage * (cSuperHealthEnd - cSuperHealthStart);
				} else {
					start = cSuperHealthStart;
				}
				end = cSuperHealthStart + ((cSuperHealthEnd - cSuperHealthStart) * super_shadow_percentage);
				
				gHealth.draw_arc[3] = UUcTrue;
				ONiInGameUI_CalcArc(start, end, gHealthArc[3]);
			}
			else
			{
				gHealth.draw_arc[3] = UUcFalse;
			}
		} else {
			// don't draw shadow health arc
			gHealth.draw_arc[2] = gHealth.draw_arc[3] = 0;
		}

		gHealth.needs_update = UUcFalse;
	}
	
	if (gHealth.has_aftershadow) {
		// health color is based on the current value of our shadow
		health_colorval = MUrUnsignedSmallFloat_To_Uns_Round(gHealth.shadow_val);
	} else {
		health_colorval = inPlayer->hitPoints;
	}
	health_shade = ONrCharacter_GetHealthShade(health_colorval, inPlayer->maxHitPoints);

	if (gHealth.has_aftershadow) {
		shadow_alpha = cHealthAfterShadowAlpha;
	} else {
		shadow_alpha = cHealthForeShadowAlpha;
	}
	main_alpha = M3cMaxAlpha;

	if (ONmIGUIElement_Flashing(ONcIGUIElement_Health)) {
		main_alpha = (UUtUns16) ((gFlashAlpha * main_alpha) / M3cMaxAlpha);
		shadow_alpha = (UUtUns16) ((gFlashAlpha * shadow_alpha) / M3cMaxAlpha);
	}
	
	if (!gHealth.has_foreshadow) {
		low_health_amount = (((float) inPlayer->hitPoints) / inPlayer->maxHitPoints) / (ONcIGUI_HealthWarnLevel / 100.0f);
		if (low_health_amount < 1.0f) {
			// calculate health flashing
			low_health_period = ONcIGUI_HealthWarnPeriod_Min			+ low_health_amount * (ONcIGUI_HealthWarnPeriod_Max - ONcIGUI_HealthWarnPeriod_Min);
			low_health_flashperiod = ONcIGUI_HealthWarnFlashPeriod_Min  + low_health_amount * (ONcIGUI_HealthWarnFlashPeriod_Max - ONcIGUI_HealthWarnFlashPeriod_Min);

			flash_t = (float) fmod((double) UUrMachineTime_Sixtieths(), (double) low_health_period);
			if (flash_t < low_health_flashperiod) {
				flash_amount = ONcIGUI_HealthWarnFlashAmount_Min + low_health_amount * (ONcIGUI_HealthWarnFlashAmount_Max - ONcIGUI_HealthWarnFlashAmount_Min);
				flash_amount *= MUrSin(M3cPi * flash_t / low_health_flashperiod);

				main_alpha = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(main_alpha * (1.0f - flash_amount));
//				health_shade = IMrShade_Interpolate(health_shade, IMcShade_White, flash_amount);
			}
		}
	}

	// draw the arcs
	for (itr = 0; itr < 4; itr++) {
		if (gHealth.draw_arc[itr]) {
			ONiInGameUI_DrawArc(gHealthArc[itr], gHealthArc[itr]->num_segments, health_shade,
								(itr < 2) ? main_alpha : shadow_alpha);
		}
	}
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayHits(
	ONtCharacter				*inPlayer)
{
	UUtUns32					i, alpha;
	UUtBool						filled, flashing;

	filled = ONmIGUIElement_Filled(ONcIGUIElement_DamageRing);
	flashing = ONmIGUIElement_Flashing(ONcIGUIElement_DamageRing);

	for (i = 0; i < cMaxNumHits; i++)
	{
		if (filled || (inPlayer->hits[i] > 0)) {
			alpha = (filled) ? M3cMaxAlpha : inPlayer->hits[i];
			if (flashing) {
				alpha = (gFlashAlpha * alpha) / M3cMaxAlpha;
			}

			ONiInGameUI_DrawArc(gHitArcs[i], gHitArcs[i]->num_segments, cHitColor, (UUtUns8) alpha);
		}
	}
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayHypos(
	ONtCharacter				*inPlayer)
{
	UUtUns16					i;
	UUtUns16					draw_hypos;
	UUtUns32					alpha;
	
	alpha = (ONmIGUIElement_Flashing(ONcIGUIElement_Hypos) ? gFlashAlpha : M3cMaxAlpha);

	// set the number of segments to draw
	if (ONmIGUIElement_Filled(ONcIGUIElement_Hypos)) {
		draw_hypos = cMaxNumHypos;
	} else {
		draw_hypos = UUmMin(inPlayer->inventory.hypo, cMaxNumHypos);
	}

	if (draw_hypos == 0) { return; }

	for (i = 0; i < draw_hypos; i++)
	{
		// draw the arc
		ONiInGameUI_DrawArc(gHyposArcs[i], gHyposArcs[i]->num_segments, cHypoColor, (UUtUns8) alpha);
	}
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayInGun(
	ONtCharacter				*inPlayer)
{
	WPtWeapon					*weapon;
	WPtWeaponClass				*weapon_class;
	UUtUns16					num_rounds;
	UUtUns16					max_rounds;
	M3tPointScreen				dest[3];
	UUtUns16					outslot;
	UUtUns32					alpha;
	
	weapon = inPlayer->inventory.weapons[0];
	if (weapon == NULL)
	{
		weapon = WPrSlot_FindLastStowed(&inPlayer->inventory, WPcPrimarySlot, &outslot);
		if (weapon == NULL) { return; }
	}
	weapon_class = WPrGetClass(weapon);
	if (weapon_class->hud_empty == NULL) { return; }
	
	alpha = (ONmIGUIElement_Flashing(ONcIGUIElement_CurrentAmmo) ? gFlashAlpha : M3cMaxAlpha);

	// set the number of segments to draw
	max_rounds = WPrMaxAmmo(weapon);
	if (ONmIGUIElement_Filled(ONcIGUIElement_CurrentAmmo)) {
		num_rounds = max_rounds;
	} else {
		num_rounds = WPrGetAmmo(weapon);
	}

	dest[0].z = 0.5f;
	dest[0].invW = 2.0f;
	dest[1].z = 0.5f;
	dest[1].invW = 2.0f;
	
	// draw the empty ammo slots
	dest[0].x = gLeftCenter.x - (gLeft->width >> 1);
	dest[0].y = gLeftCenter.y - (gLeft->height >> 1);
	dest[1].x = dest[0].x + gLeft->width;
	dest[1].y = dest[0].y + gLeft->height;
	
	M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
	M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, weapon_class->hud_empty);
	M3rDraw_State_Commit();
	M3rDraw_Sprite(dest, gFullTextureUVs);

	if (num_rounds > 0)
	{
		// draw the filled ammo slots
		if ((num_rounds != gCurNumRounds) || (max_rounds != gCurMaxRounds))
		{
			float						rounds_percentage;
			float						start;
			float						end;

			// we need to recompute the ammo arc
			gCurNumRounds = num_rounds;
			gCurMaxRounds = max_rounds;

			rounds_percentage = ((float) gCurNumRounds) / gCurMaxRounds;

			start = cInGunStart;
			end = start + ((cInGunEnd - start) * rounds_percentage);
			
			ONiInGameUI_CalcArc(start, end, gInGunArc);
		}
		
		// draw the arc
		if (weapon_class->hud_fill != NULL)
		{
			ONiInGameUI_SetArcTexture(gInGunArc, weapon_class->hud_fill);
			ONiInGameUI_DrawArc(gInGunArc, gInGunArc->num_segments, IMcShade_None, (UUtUns8) alpha);
		}
	}
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayInvisibility(
	ONtCharacter				*inPlayer)
{
	UUtUns32 alpha;

	if (ONmIGUIElement_Filled(ONcIGUIElement_Invisibility)) {
		ONiInGameUI_CalcArc(cInvisStart, cInvisEnd, gInvisArc);
		gPrevInvisibilityRemaining = (UUtUns16) -1;

	} else {
		if (inPlayer->inventory.invisibilityRemaining == 0) { return; }
		
		if (gPrevInvisibilityRemaining != inPlayer->inventory.invisibilityRemaining)
		{
			float						invis_percentage;
			float						start;
			float						end;
			
			invis_percentage = (float)inPlayer->inventory.invisibilityRemaining / (float)WPcMaxInvisibility;
			
			start = cInvisStart;
			end = start + ((cInvisEnd - start) * invis_percentage);
			
			ONiInGameUI_CalcArc(start, end, gInvisArc);
			
			gPrevInvisibilityRemaining = inPlayer->inventory.invisibilityRemaining;
		}
	}
	
	// draw the arc
	alpha = (ONmIGUIElement_Flashing(ONcIGUIElement_Invisibility) ? gFlashAlpha : M3cMaxAlpha);
	ONiInGameUI_DrawArc(gInvisArc, gInvisArc->num_segments, cInvisColor, (UUtUns8) alpha);
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayRadar(
	ONtCharacter				*inPlayer)
{
	float						arc_length;
	float						angle;
	float						start;
	float						end;
	M3tVector3D					player_to_target;
	float						distance;
	float						delta_x;
	float						delta_z;
	float						target_angle;
	UUtUns8						alpha;
	UUtBool						draw_ring;
	UUtBool						draw_uparrow;
	UUtBool						draw_downarrow;
	M3tPointScreen				dest[2];

	/*
	 * draw the compass ring
	 */

	draw_ring = UUcFalse;

	if (ONgInGameUI_NewObjective || ONmIGUIElement_Flashing(ONcIGUIElement_CompassRing)) {
		alpha = (UUtUns8) gFlashAlpha;
	} else {
		alpha = M3cMaxAlpha;
	}

	if (ONmIGUIElement_Filled(ONcIGUIElement_CompassRing)) {
		start = 0;
		end = M3c2Pi;
		draw_ring = UUcTrue;

	} else if (gHasLSITarget) {
		// calculate what the angle is to our target
		MUmVector_Subtract(player_to_target, gLSITarget, inPlayer->location);
		
		distance = MUmVector_GetLengthSquared(player_to_target);
		if (distance < gMinDistanceSquared)
		{
			arc_length = cMaxArcLength;
		}
		else if (distance > gMaxDistanceSquared)
		{
			arc_length = cMinArcLength;
		}
		else
		{
			arc_length = M3cHalfPi - (M3cHalfPi * (distance / gMaxDistanceSquared));
		}
		arc_length = UUmPin(arc_length, cMinArcLength, cMaxArcLength);
		
		delta_x = gLSITarget.x - inPlayer->location.x;
		delta_z = gLSITarget.z - inPlayer->location.z;
		target_angle = MUrATan2(delta_x, delta_z);
		
		angle = ONrCharacter_GetCosmeticFacing(inPlayer) - target_angle;
		UUmTrig_ClipAbsPi(angle);

		start = (angle - arc_length) + (M3cHalfPi * 3.0f);
		end = (angle + arc_length) + (M3cHalfPi * 3.0f);
		draw_ring = UUcTrue;

	} else if (ONgInGameUI_NewObjective) {
		// no target available, but we have to flash something - flash the whole ring
		start = 0;
		end = M3c2Pi;
		draw_ring = UUcTrue;
	}
	
	if (draw_ring) {
		ONiInGameUI_CalcArc(start, end, gRadarArc);
		ONiInGameUI_DrawArc(gRadarArc, gRadarArc->num_segments, cRadarColor, alpha);
	}
	
	/*
	 * draw the height arrow
	 */

	if (ONmIGUIElement_Flashing(ONcIGUIElement_CompassArrow)) {
		alpha = (UUtUns8) gFlashAlpha;
	} else {
		alpha = M3cMaxAlpha;
	}
	
	draw_uparrow = UUcFalse;
	draw_downarrow = UUcFalse;
	if (gHasLSITarget) {
		if (player_to_target.y > cUpDnHeightUnits) {
			draw_uparrow = UUcTrue;

		} else if (player_to_target.y < -cUpDnHeightUnits) {
			draw_downarrow = UUcTrue;
		}
	}

	if (!draw_downarrow && ONmIGUIElement_Filled(ONcIGUIElement_CompassArrow)) {
		draw_uparrow = UUcTrue;
	}

	// draw the up or down arrow
	if (draw_uparrow || draw_downarrow)
	{
		dest[0].x = gLeftCenter.x - (cUpDnArrowDiameter * 0.5f) + cUpDnArrowOffsetX;
		dest[0].y = gLeftCenter.y - (cUpDnArrowDiameter * 0.5f);
		dest[0].z = 0.5f;
		dest[0].invW = 2.0f;
		
		dest[1].x = dest[0].x + cUpDnArrowDiameter;
		dest[1].y = dest[0].y + cUpDnArrowDiameter;
		dest[1].z = 0.5f;
		dest[1].invW = 2.0f;
		
		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, gUpDnArrow);
		M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, alpha);
		M3rDraw_State_Commit();
	
		if (draw_uparrow)
		{
			M3rDraw_Sprite(dest, gFullTextureUVs);
		}
		else if (draw_downarrow)
		{
			M3rDraw_Sprite(dest, gFullTextureUVsInvert);
		}

		if (alpha < M3cMaxAlpha) {
			// restore alpha to full
			M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, M3cMaxAlpha);
		}		
	}
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayShield(
	ONtCharacter				*inPlayer)
{
	float						shield_percentage, shadow_percentage;
	UUtUns32					main_alpha, shadow_alpha;

	if (ONmIGUIElement_Filled(ONcIGUIElement_Shield)) {
		gShield.draw_arc[0] = UUcTrue;
		gShield.draw_arc[1] = UUcFalse;
		ONiInGameUI_CalcArc(cShieldStart, cShieldEnd, gShieldArc[0]);
		gShield.needs_update = UUcTrue;

	} else if (gShield.needs_update) {
		float						start;
		float						end;

		if (gShield.value == 0) {
			// don't draw main shield arc
			shield_percentage = 0;
			gShield.draw_arc[0] = UUcFalse;
		} else {
			shield_percentage = ((float) gShield.value) / WPcMaxShields;

			// work out primary arc
			start = cShieldStart;
			end = start + ((cShieldEnd - start) * UUmMin(1.0f, shield_percentage));
		
			gShield.draw_arc[0] = UUcTrue;
			ONiInGameUI_CalcArc(start, end, gShieldArc[0]);
		}

		if (gShield.has_foreshadow || gShield.has_aftershadow) {
			// compute the amount of shadow shield
			shadow_percentage = gShield.shadow_val / WPcMaxShields;

			// work out shadow arc
			start = cShieldStart + shield_percentage * (cShieldEnd - cShieldStart);
			end = cShieldStart + UUmMin(1.0f, shadow_percentage) * (cShieldEnd - cShieldStart);
		
			gShield.draw_arc[1] = UUcTrue;
			ONiInGameUI_CalcArc(start, end, gShieldArc[1]);
		} else {
			gShield.draw_arc[1] = UUcFalse;
		}

		gShield.needs_update = UUcFalse;
	}
	
	main_alpha = M3cMaxAlpha;
	shadow_alpha = cShieldShadowAlpha;

	if (ONmIGUIElement_Flashing(ONcIGUIElement_Shield)) {
		main_alpha = (main_alpha * gFlashAlpha) / main_alpha;
		shadow_alpha = (main_alpha * gFlashAlpha) / shadow_alpha;
	}

	// draw the arcs
	if (gShield.draw_arc[0]) {
		ONiInGameUI_DrawArc(gShieldArc[0], gShieldArc[0]->num_segments, cShieldColor, (UUtUns8) main_alpha);
	}
	if (gShield.draw_arc[1]) {
		ONiInGameUI_DrawArc(gShieldArc[1], gShieldArc[1]->num_segments, cShieldColor, (UUtUns8) shadow_alpha);
	}
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayWeaponIcon(
	ONtCharacter				*inPlayer)
{
	WPtWeapon					*weapon;
	WPtWeaponClass				*weapon_class;
	M3tPointScreen				dest[2];
	UUtUns32					alpha;
//	WPtWeapon					*stored_weapon;
//	UUtUns16					outslot;
	
	weapon = inPlayer->inventory.weapons[0];
	if (weapon != NULL)
	{
		weapon_class = WPrGetClass(weapon);
		if (weapon_class->icon != NULL)
		{
			UUmAssert(weapon_class->icon->width == 64);
			UUmAssert(weapon_class->icon->height == 64);
			
			if (ONmIGUIElement_Flashing(ONcIGUIElement_Weapon)) {
				alpha = gFlashAlpha;
			} else {
				alpha = M3cMaxAlpha;
			}

			dest[0].x = gLeftCenter.x - (cIconDrawnDiameter * 0.5f);
			dest[0].y = gLeftCenter.y - (cIconDrawnDiameter * 0.5f);
			dest[0].z = 0.5f;
			dest[0].invW = 2.0f;
			
			dest[1].x = dest[0].x + cIconDrawnDiameter;
			dest[1].y = dest[0].y + cIconDrawnDiameter;
			dest[1].z = 0.5f;
			dest[1].invW = 2.0f;
			
			M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
			M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, weapon_class->icon);
			M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, alpha);
			M3rDraw_State_Commit();
			
			M3rDraw_Sprite(dest, gFullTextureUVs);

			if (alpha < M3cMaxAlpha) {
				M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, M3cMaxAlpha);
			}
		}
	}
	
/*	// draw the one in the holster
	stored_weapon = WPrSlot_FindLastStowed(inPlayer->inventory.weapons, WPcPrimarySlot, &outslot);
	if (stored_weapon != NULL)
	{
		weapon_class = WPrGetClass(stored_weapon);
		if (weapon_class->icon != NULL)
		{
			UUmAssert(weapon_class->icon->width == 64);
			UUmAssert(weapon_class->icon->height == 64);
			
			dest[0].x = gLeftCenter.x - (cIconStoredDiameter * 0.5f) + cIconStoredOffsetX;
			dest[0].y = gLeftCenter.y - (cIconStoredDiameter * 0.5f);
			dest[0].z = 0.5f;
			dest[0].invW = 2.0f;
			
			dest[1].x = dest[0].x + cIconStoredDiameter;
			dest[1].y = dest[0].y + cIconStoredDiameter;
			dest[1].z = 0.5f;
			dest[1].invW = 2.0f;
			
			M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
			M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, weapon_class->icon);
			M3rDraw_State_Commit();
			
			M3rDraw_Sprite(dest, gFullTextureUVs);
		}
	}*/
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayLSIIcon(
	ONtCharacter				*inPlayer,
	UUtUns16					inAlpha)
{
	M3tPointScreen				dest[2];
	
	if (inPlayer->inventory.has_lsi && (gLSIIcon != NULL))
	{
		UUmAssert(gLSIIcon->width == 64);
		UUmAssert(gLSIIcon->height == 64);
		
		if (ONmIGUIElement_Flashing(ONcIGUIElement_ItemDisplay)) {
			inAlpha = (UUtUns16) ((inAlpha * gFlashAlpha) / M3cMaxAlpha);
		}

		dest[0].x = gRightCenter.x - (cLSIIconDiameter * 0.5f);
		dest[0].y = gRightCenter.y - (cLSIIconDiameter * 0.5f);
		dest[0].z = 0.5f;
		dest[0].invW = 2.0f;
		
		dest[1].x = dest[0].x + cLSIIconDiameter;
		dest[1].y = dest[0].y + cLSIIconDiameter;
		dest[1].z = 0.5f;
		dest[1].invW = 2.0f;

		if (inAlpha < M3cMaxAlpha) {
			M3rGeom_State_Push();
			M3rGeom_State_Set(M3cGeomStateIntType_Alpha, inAlpha);
		}
		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, gLSIIcon);
		M3rGeom_State_Commit();
		
		M3rDraw_Sprite(dest, gFullTextureUVs);

		if (inAlpha < M3cMaxAlpha) {
			M3rGeom_State_Pop();
		}
	}
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_DisplayHelp(
	void)
{
	M3tPointScreen dest[2];
	IMtPoint2D left_position, right_position;
	ONtIGUI_HUDTextItem *text_item;
	UUtUns32 i, j;
	UUtError error;

	if (!ONgInGameUI_EnableHelp || (gHUDHelp == NULL)) {
		return;
	}

	// determine the center of the left help texture
	left_position.x = (UUtInt16) MUrUnsignedSmallFloat_To_Uns_Round(gLeftCenter.x) + gHUDHelp->left_offset.x;
	left_position.y = (UUtInt16) MUrUnsignedSmallFloat_To_Uns_Round(gLeftCenter.y) + gHUDHelp->left_offset.y;
	if (gHUDHelp->left_texture != NULL) {
		// offset this by the size of the left help texture
		left_position.x -= gHUDHelp->left_texture->width / 2;
		left_position.y -= gHUDHelp->left_texture->height / 2;

	} else if (gLeft != NULL) {
		// no left help texture, offset by the size of the left HUD texture instead
		left_position.x -= gLeft->width / 2;
		left_position.y -= gLeft->height / 2;

	} else {
		// neither texture exists, offset by 128x128 (default fallback)
		left_position.x -= 128 / 2;
		left_position.y -= 128 / 2;
	}

	// determine the center of the right help texture
	right_position.x = (UUtInt16) MUrUnsignedSmallFloat_To_Uns_Round(gRightCenter.x) + gHUDHelp->right_offset.x;
	right_position.y = (UUtInt16) MUrUnsignedSmallFloat_To_Uns_Round(gRightCenter.y) + gHUDHelp->right_offset.y;
	if (gHUDHelp->right_texture != NULL) {
		// offset this by the size of the right help texture
		right_position.x -= gHUDHelp->right_texture->width / 2;
		right_position.y -= gHUDHelp->right_texture->height / 2;

	} else if (gRight != NULL) {
		// no left help texture, offset by the size of the right HUD texture instead
		right_position.x -= gRight->width / 2;
		right_position.y -= gRight->height / 2;

	} else {
		// neither texture exists, offset by 128x128 (default fallback)
		right_position.x -= 128 / 2;
		right_position.y -= 128 / 2;
	}

	if (gLeftEnable && (gHUDHelp->left_texture != NULL)) {
		// draw left help overlay
		dest[0].x = left_position.x;
		dest[0].y = left_position.y;
		dest[0].z = 0.5f;
		dest[0].invW = 2.0f;
		
		dest[1].x = dest[0].x + gHUDHelp->left_texture->width;
		dest[1].y = dest[0].y + gHUDHelp->left_texture->height;
		dest[1].z = 0.5f;
		dest[1].invW = 2.0f;

		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, gHUDHelp->left_texture);
		M3rDraw_State_Commit();
		
		M3rDraw_Sprite(dest, gFullTextureUVs);
	}

	if (gRightEnable && (gHUDHelp->right_texture != NULL)) {
		// draw right help overlay
		dest[0].x = right_position.x;
		dest[0].y = right_position.y;
		dest[0].z = 0.5f;
		dest[0].invW = 2.0f;
		
		dest[1].x = dest[0].x + gHUDHelp->right_texture->width;
		dest[1].y = dest[0].y + gHUDHelp->right_texture->height;
		dest[1].z = 0.5f;
		dest[1].invW = 2.0f;

		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, gHUDHelp->right_texture);
		M3rDraw_State_Commit();
		
		M3rDraw_Sprite(dest, gFullTextureUVs);
	}

	if (gHUDHelpTextContext != NULL) {
		UUtRect bounds;

		// draw the help text
		TSrContext_SetShade(gHUDHelpTextContext, cHUDHelpTextShade);
		bounds.left = 0;
		bounds.top = 0;
		bounds.right = M3rDraw_GetWidth();
		bounds.bottom = M3rDraw_GetHeight();

		for (i = 0, text_item = gHUDHelp->text_items; i < gHUDHelp->num_text_items; i++, text_item++) {
			IMtPoint2D text_dest, offset;
			TStStringFormat formatted_string;
			IMtShade shadow_color;
			UUtBool is_right;

			// get parameters for this text string
			text_dest = text_item->offset;
			if (i < gHUDHelp->num_left_items) {
				if (!gLeftEnable) {
					// this side of the HUD is hidden
					continue;
				}

				// this is one of the left-hand text labels
				text_dest.x += left_position.x + cHelpTextXOffset;
				text_dest.y += left_position.y;
				is_right = UUcFalse;
			} else {
				if (!gRightEnable) {
					// this side of the HUD is hidden
					continue;
				}

				// this is one of the right-hand text labels
				text_dest.x += right_position.x - cHelpTextXOffset;
				text_dest.y += right_position.y;
				is_right = UUcTrue;
			}

			// format the help text
			error = TSrContext_FormatString(gHUDHelpTextContext, text_item->text, &bounds, &text_dest, &formatted_string);
			if ((error != UUcError_None) || (formatted_string.num_segments == 0)) {
				continue;
			}

			// determine the final destination for this text
			if (is_right) {
				offset.x = text_dest.x - formatted_string.bounds[0].right;
			} else {
				offset.x = text_dest.x - formatted_string.bounds[0].left;
			}
			offset.y = text_dest.y - (formatted_string.bounds[formatted_string.num_segments - 1].bottom + formatted_string.bounds[0].top) / 2;

			for (j = 0; j < formatted_string.num_segments; j++) {
				formatted_string.bounds[j].left += offset.x;
				formatted_string.bounds[j].right += offset.x;
				formatted_string.bounds[j].top += offset.y;
				formatted_string.bounds[j].bottom += offset.y;

				formatted_string.destination[j].x += offset.x;
				formatted_string.destination[j].y += offset.y;
			}

			// draw the drop shadow and then the main text
			offset.x = +1; offset.y = +1;
			shadow_color = cHUDHelpTextShadow;
			TSrContext_DrawFormattedText(gHUDHelpTextContext, &formatted_string, M3cMaxAlpha, &offset, &shadow_color);
			TSrContext_DrawFormattedText(gHUDHelpTextContext, &formatted_string, M3cMaxAlpha, NULL, NULL);
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
ONrInGameUI_Display(
	void)
{
	ONtCharacter				*player;
	M3tPointScreen				dest[3];
	UUtUns16					alpha;
	UUtUns32 cur_time;
	
	// get a pointer to the player's character
	player = ONrGameState_GetPlayerCharacter();
	if (player == NULL) { return; }


	cur_time = UUrMachineTime_Sixtieths();
	if (ONgInGameUI_NewObjective || (gElementFlashing > 0)) {
		// calculate the alpha of flashing UI elements
		gFlashAlpha = MUrUnsignedSmallFloat_To_Uns_Round((float)(M3cMaxAlpha * (float) exp((float)(- 0.03 * ((cur_time - gFlashStartTime) % 60)))));
	} else {
		// nothing is flashing
		gFlashAlpha = M3cMaxAlpha;
		gFlashStartTime = cur_time;
	}

	dest[0].z = 0.5f;
	dest[0].invW = 2.0f;
	dest[1].z = 0.5f;
	dest[1].invW = 2.0f;

	M3rDraw_State_Push();
	M3rDraw_State_SetInt(M3cDrawStateIntType_Appearence, M3cDrawState_Appearence_Texture_Unlit);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Interpolation, M3cDrawState_Interpolation_None);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Fill, M3cDrawState_Fill_Solid);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, M3cMaxAlpha);
	M3rDraw_State_Commit();

	if (gLeftEnable) {
		// ----------------------------------------
		// draw the left grid
		dest[0].x = gLeftCenter.x - (gLeft->width >> 1);
		dest[0].y = gLeftCenter.y - (gLeft->height >> 1);
		dest[1].x = dest[0].x + gLeft->width;
		dest[1].y = dest[0].y + gLeft->height;
		
		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, gLeft);
		M3rDraw_State_Commit();
		M3rDraw_Sprite(dest, gFullTextureUVs);
		
		// draw the ballistic clips arc
		ONiInGameUI_DisplayAmmoClips(player);
		
		// draw the energy clips arc	
		ONiInGameUI_DisplayCellClips(player);

		// draw the radar arc
		ONiInGameUI_DisplayRadar(player);
		
		// draw the ammo arc
		ONiInGameUI_DisplayInGun(player);
		
		// draw the weapon icon
		ONiInGameUI_DisplayWeaponIcon(player);
	}
	
	if (gRightEnable) {
		// ----------------------------------------
		// draw the right grid
		dest[0].x = gRightCenter.x - (gRight->width >> 1);
		dest[0].y = gRightCenter.y - (gRight->height >> 1);;
		dest[1].x = dest[0].x + gRight->width;
		dest[1].y = dest[0].y + gRight->height;
		
		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
		M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, gRight);
		M3rDraw_State_Commit();
		M3rDraw_Sprite(dest, gFullTextureUVs);

		// draw the health arc
		ONiInGameUI_DisplayHealth(player);
		
		// draw the invisibility arc
		ONiInGameUI_DisplayInvisibility(player);
		
		// draw the shield arc
		ONiInGameUI_DisplayShield(player);

		// draw the health packets arc
		ONiInGameUI_DisplayHypos(player);

		// draw the hit ring
		ONiInGameUI_DisplayHits(player);

		// draw the LSI icon
		alpha = M3cMaxAlpha;
		if (ONrGameState_Timer_GetMode() != ONcTimerMode_Off) {
			alpha /= 3;
		}
		ONiInGameUI_DisplayLSIIcon(player, alpha);

		// draw the timer
		ONrGameState_Timer_Display(&gRightCenter);
	}

	// draw the help overlays
	ONiInGameUI_DisplayHelp();
	
	M3rDraw_State_Pop();
}

// ----------------------------------------------------------------------
static void
ONrInGameUI_SetArcPoints(
	void)
{
	UUtUns32 					i;
	
	// calculate the left and right centers
	gLeftCenter.x = 10.0f + (gLeft->width >> 1);
	gLeftCenter.y = (float)M3rDraw_GetHeight() - (gLeft->height >> 1);
	gLeftCenter.z = 0.5f;
	gLeftCenter.invW = 2.0f;

	gRightCenter.x = (float)M3rDraw_GetWidth() - 10.0f - (gRight->width >> 1);
	gRightCenter.y = (float)M3rDraw_GetHeight() - (gRight->height >> 1);
	gRightCenter.z = 0.5f;
	gRightCenter.invW = 2.0f;
	
	// make the arcs
	for (i = 0; i < 2; i++) {
		ONiInGameUI_MakeArc(
			cHealthStart,
			cHealthEnd,
			cHealthOuterRadius,
			cHealthInnerRadius,
			&gRightCenter,
			24,
			gRightRing,
			&gHealthArc[2*i + 0]);

		ONiInGameUI_MakeArc(
			cSuperHealthStart,
			cSuperHealthEnd,
			cSuperHealthOuterRadius,
			cSuperHealthInnerRadius,
			&gRightCenter,
			24,
			gRightRing,
			&gHealthArc[2*i + 1]);
	}
		
	ONiInGameUI_MakeArc(
		cInvisStart,
		cInvisEnd,
		cInvisOuterRadius,
		cInvisInnerRadius,
		&gRightCenter,
		8,
		gRightRing,
		&gInvisArc);
		
	ONiInGameUI_MakeArc(
		cShieldStart,
		cShieldEnd,
		cShieldOuterRadius,
		cShieldInnerRadius,
		&gRightCenter,
		8,
		gRightRing,
		&gShieldArc[0]);
		
	ONiInGameUI_MakeArc(
		cShieldStart,
		cShieldEnd,
		cShieldOuterRadius,
		cShieldInnerRadius,
		&gRightCenter,
		8,
		gRightRing,
		&gShieldArc[1]);
		
	ONiInGameUI_MakeArc(
		cAmmoStart,
		cAmmoEnd,
		cAmmoOuterRadius,
		cAmmoInnerRadius,
		&gLeftCenter,
		cMaxNumAmmo,
		gLeftRing,
		&gAmmoArc);
	
	ONiInGameUI_MakeArc(
		cCellStart,
		cCellEnd,
		cCellOuterRadius,
		cCellInnerRadius,
		&gLeftCenter,
		cMaxNumCells,
		gLeftRing,
		&gCellArc);

	ONiInGameUI_MakeArc(
		cInGunStart,
		cInGunEnd,
		cInGunOuterRadius,
		cInGunInnerRadius,
		&gLeftCenter,
		24,
		gLeftRing,
		&gInGunArc);

	ONiInGameUI_MakeArc(
		cRadarStart,
		cRadarEnd,
		cRadarOuterRadius,
		cRadarInnerRadius,
		&gLeftCenter,
		32,
		gLeftRing,
		&gRadarArc);
	
	for (i = 0; i < cMaxNumHypos; i++)
	{
		ONiInGameUI_MakeArc(
			cHyposStartEnd[i].start,
			cHyposStartEnd[i].end,
			cHyposOuterRadius,
			cHyposInnerRadius,
			&gRightCenter,
			1,
			gRightRing,
			&gHyposArcs[i]);
	}
	
	for (i = 0; i < cMaxNumHits; i++)
	{
		ONiInGameUI_MakeArc(
			cHitStartEnd[i].start,
			cHitStartEnd[i].end,
			cHitOuterRadius,
			cHitInnerRadius,
			&gRightCenter,
			4,
			gRightRing,
			&gHitArcs[i]);
	}
}

// ----------------------------------------------------------------------
static UUtError
ONiInGameUI_LevelLoad(
	void)
{
	UUtError					error;
	
	// load the textures
	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "left", &gLeft);
	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "right", &gRight);
	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "leftRing", &gLeftRing);
	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "rightRing", &gRightRing);
	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "lsi_icon", &gLSIIcon);
	if (error != UUcError_None) {
		error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "notfoundtex", &gLSIIcon);
		if (error != UUcError_None) {
			gLSIIcon = NULL;
		}
	}
	error = TMrInstance_GetDataPtr(ONcTemplate_IGUI_HUDHelp, "hud_help_info", &gHUDHelp);
	if (error != UUcError_None) { gHUDHelp = NULL; }

	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "updn_arrow", &gUpDnArrow);

	ONrInGameUI_SetArcPoints();
	
	// set initial HUD state
	ONgObjectiveNumber = 0;
	ONgDiaryPageUnlocked = 0;		// no diary pages have been read yet
	gHasLSITarget = UUcFalse;
	ONgInGameUI_EnableHelp = UUcFalse;
	gLeftEnable = UUcTrue;
	gRightEnable = UUcTrue;
	gElementFilled = 0;
	gElementFlashing = 0;
	gFlashAlpha = M3cMaxAlpha;
	gFlashStartTime = 0;

	ONgInGameUI_LastRestoreGame = (UUtUns32) -1;
	gHUDHelpTextContext = NULL;
	ONgInGameUI_NewWeaponClass = NULL;
	ONgInGameUI_NewItem = (UUtUns32) -1;
	ONgInGameUI_NewObjective = UUcFalse;
	ONgInGameUI_NewMove = UUcFalse;
	ONgInGameUI_ImmediatePrompt = UUcFalse;
	ONgInGameUI_SuppressPrompting = UUcFalse;
	ONgInGameUI_LastPromptTime = 0;

	// clear any override messages
	ONrPauseScreen_OverrideMessage(NULL);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_LevelUnload(
	void)
{
	UUtUns32					i;
	
	gLeft = NULL;
	gRight = NULL;
	gLeftRing = NULL;
	gRightRing = NULL;
	gWhite = NULL;
	gUpDnArrow = NULL;
	
	for (i = 0; i < 4; i++) {
		UUrMemory_Block_Delete(gHealthArc[i]); gHealthArc[i] = NULL;
	}

	for (i = 0; i < 2; i++) {
		UUrMemory_Block_Delete(gShieldArc[i]); gShieldArc[i] = NULL;
	}

	UUrMemory_Block_Delete(gAmmoArc); gAmmoArc = NULL;
	UUrMemory_Block_Delete(gCellArc); gCellArc = NULL;
	UUrMemory_Block_Delete(gInGunArc); gInGunArc = NULL;
	UUrMemory_Block_Delete(gRadarArc); gRadarArc = NULL;
	UUrMemory_Block_Delete(gInvisArc); gInvisArc = NULL;

	for (i = 0; i < cMaxNumHypos; i++)
	{
		UUrMemory_Block_Delete(gHyposArcs[i]); gHyposArcs[i] = NULL;
	}
	
	for (i = 0; i < cMaxNumHits; i++)
	{
		UUrMemory_Block_Delete(gHitArcs[i]); gHitArcs[i] = NULL;
	}

	if (gHUDHelpTextContext != NULL) {
		TSrContext_Delete(gHUDHelpTextContext);
		gHUDHelpTextContext = NULL;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiFIStack_Init(
	TStFontFamily				*inFontFamily,
	UUtUns16					inFontSize,
	TStFontStyle				inFontStyle,
	IMtShade					inFontShade)
{
	UUmAssert(inFontFamily);
	
	ONgFIStack[0].font_family = inFontFamily;
	ONgFIStack[0].font_size = inFontSize;
	ONgFIStack[0].font_style = inFontStyle;
	ONgFIStack[0].font_shade = inFontShade;
	
	ONgFIStackIndex = 0;
}

// ----------------------------------------------------------------------
static void
ONiFIStack_Push(
	ONtIGUI_FontInfo			*inFontInfo)
{
	if ((ONgFIStackIndex + 1) == ONcMaxFIStackItems) { return; }
	
	ONgFIStackIndex++;
	
	// copy all of the entries
	ONgFIStack[ONgFIStackIndex] = ONgFIStack[ONgFIStackIndex - 1];

	if (inFontInfo->flags != IGUIcFontInfoFlag_None)
	{
		// copy only the entries that need to change
		if ((inFontInfo->flags & IGUIcFontInfoFlag_FontFamily) != 0)
		{
			ONgFIStack[ONgFIStackIndex].font_family = inFontInfo->font_family;
		}
		
		if ((inFontInfo->flags & IGUIcFontInfoFlag_FontSize) != 0)
		{
			ONgFIStack[ONgFIStackIndex].font_size = inFontInfo->font_size;
		}
		
		if ((inFontInfo->flags & IGUIcFontInfoFlag_FontStyle) != 0)
		{
			ONgFIStack[ONgFIStackIndex].font_style = inFontInfo->font_style;
		}
		
		if ((inFontInfo->flags & IGUIcFontInfoFlag_FontShade) != 0)
		{
			ONgFIStack[ONgFIStackIndex].font_shade = inFontInfo->font_shade;
		}
	}
	
	// set the new font info
	DCrText_SetFontInfo(&ONgFIStack[ONgFIStackIndex]);
}

// ----------------------------------------------------------------------
static void
ONiFIStack_Pop(
	void)
{
	if (ONgFIStackIndex != 0)
	{
		ONgFIStackIndex--;
	}
	
	// set the font info
	DCrText_SetFontInfo(&ONgFIStack[ONgFIStackIndex]);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiIGUI_Page_Paint(
	ONtIGUI_Page				*inPage,
	WMtDrawItem					*inDrawItem)
{
	IMtPoint2D					dest;
	UUtUns32					itr;
	UUtRect						rect;
		
	if (inPage == NULL) { return; }
	
	// push the font info
	ONiFIStack_Push(&inPage->font_info);
	
	// set the initial dest
	dest.x = 0;
	dest.y = 0;
		
	if (inDrawItem->window_id == ONcPS_Txt_MainArea)
	{
		if (inPage->pict)
		{
			UUtInt16				win_width;
			
			WMrWindow_GetSize(inDrawItem->window, &win_width, NULL);
			
			switch (TMrInstance_GetTemplateTag(inPage->pict))
			{
				case M3cTemplate_TextureMap:
				{
					UUtUns16				pict_width;
					UUtUns16				pict_height;
					
					M3rTextureRef_GetSize(inPage->pict, &pict_width, &pict_height);
				
					dest.x = (win_width - (UUtInt16)pict_width) >> 1;
				
					// draw the picture		
					DCrDraw_TextureRef(
						inDrawItem->draw_context,
						inPage->pict,
						&dest,
						-1,
						-1,
						UUcFalse,
						M3cMaxAlpha);
					
					dest.y += pict_height;
				}
				break;
				
				case PScTemplate_PartSpecification:
				{
					UUtInt16				part_height;
					
					PSrPartSpec_GetSize(inPage->pict, PScPart_LeftTop, NULL, &part_height);
					
					dest.x = 0;
					
					// draw the partspec
					DCrDraw_PartSpec(
						inDrawItem->draw_context,
						inPage->pict,
						PScPart_TopRow,
						&dest,
						win_width,
						part_height,
						M3cMaxAlpha);
					
					dest.y += part_height;
				}
				break;
			}
			
			// leave some space between the picture and the text
			dest.y += 10;
		}
		
		if (inPage->text)
		{
			// draw the item text
			dest.x = 0;
			dest.y += DCrText_GetAscendingHeight();
			for (itr = 0; itr < inPage->text->numStrings; itr++)
			{
				if (inPage->text->string[itr] == NULL) { continue; }
				
				// set the font info
				ONiFIStack_Push(&inPage->text->string[itr]->font_info);
				
				DCrDraw_String2(
					inDrawItem->draw_context,
					inPage->text->string[itr]->string,
					NULL,
					&dest,
					&rect);
				
				dest.y += (rect.bottom - rect.top);
				
				// restore the font info
				ONiFIStack_Pop();
			}
		}
	}
	else if (inDrawItem->window_id == ONcPS_Txt_SubArea)
	{
		if (inPage->hint)
		{
			// draw the hint text
			dest.y += DCrText_GetAscendingHeight();
			for (itr = 0; itr < inPage->hint->numStrings; itr++)
			{
				if (inPage->hint->string[itr] == NULL) { continue; }
				
				// set the font info
				ONiFIStack_Push(&inPage->hint->string[itr]->font_info);
				
				DCrDraw_String2(
					inDrawItem->draw_context,
					inPage->hint->string[itr]->string,
					NULL,
					&dest,
					&rect);
				
				dest.y += (rect.bottom - rect.top);

				// restore the font info
				ONiFIStack_Pop();
			}
		}
	}
	
	// pop the font info
	ONiFIStack_Pop();
}

// ----------------------------------------------------------------------
static void
ONiIGUI_Page_PaintOverlay(
	ONtIGUI_Page				*inPage,
	WMtDrawItem					*inDrawItem)
{
	IMtPoint2D					dest;
	UUtUns32					itr;
	UUtRect						rect;
	
	if (inPage == NULL) { return; }
	
	// push the font info
	ONiFIStack_Push(&inPage->font_info);
	
	// set the initial dest
	dest.x = 0;
	dest.y = 0;
	
	if (inDrawItem->window_id == ONcPS_Txt_MainArea)
	{
		UUtUns16				pict_width;
		UUtUns16				pict_height;
		UUtInt16				win_width;
		UUtInt16				win_height;
		
		if (inPage->text)
		{
			// draw the item text
			dest.x = 0;
			dest.y = DCrText_GetAscendingHeight();
			for (itr = 0; itr < inPage->text->numStrings; itr++)
			{
				if (inPage->text->string[itr] == NULL) { continue; }
				
				// set the font info
				ONiFIStack_Push(&inPage->text->string[itr]->font_info);
				
				DCrDraw_String2(
					inDrawItem->draw_context,
					inPage->text->string[itr]->string,
					NULL,
					&dest,
					&rect);
				
				dest.y += (rect.bottom - rect.top);
				
				// restore the font info
				ONiFIStack_Pop();
			}
		}
		
		if ((inPage->pict) &&
			(TMrInstance_GetTemplateTag(inPage->pict) == M3cTemplate_TextureMap))
		{
			WMrWindow_GetSize(inDrawItem->window, &win_width, &win_height);
			M3rTextureRef_GetSize(inPage->pict, &pict_width, &pict_height);
		
			dest.x = (win_width - (UUtInt16)pict_width) >> 1;
			dest.y = (win_height - (UUtInt16)pict_height) >> 1;
		
			// draw the picture		
			DCrDraw_TextureRef(
				inDrawItem->draw_context,
				inPage->pict,
				&dest,
				-1,
				-1,
				UUcFalse,
				M3cMaxAlpha);
		}
	}
	else if (inDrawItem->window_id == ONcPS_Txt_SubArea)
	{
		if (inPage->hint)
		{
			// draw the hint text
			dest.y += DCrText_GetAscendingHeight();
			for (itr = 0; itr < inPage->hint->numStrings; itr++)
			{
				if (inPage->hint->string[itr] == NULL) { continue; }
				
				// set the font info
				ONiFIStack_Push(&inPage->hint->string[itr]->font_info);
				
				DCrDraw_String2(
					inDrawItem->draw_context,
					inPage->hint->string[itr]->string,
					NULL,
					&dest,
					&rect);
				
				dest.y += (rect.bottom - rect.top);

				// restore the font info
				ONiFIStack_Pop();
			}
		}
	}
	
	// pop the font info
	ONiFIStack_Pop();
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiPS_ShowHintArea(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData,
	UUtBool						inShow)
{
	WMtWindow					*prev;
	WMtWindow					*next;
	WMtWindow					*hint_text;
	WMtWindow					*hint_box;
		
	prev = WMrDialog_GetItemByID(inDialog, ONcPS_Btn_Previous);
	next = WMrDialog_GetItemByID(inDialog, ONcPS_Btn_Next);
	hint_text = WMrDialog_GetItemByID(inDialog, ONcPS_Txt_SubArea);
	hint_box = WMrDialog_GetItemByID(inDialog, ONcPS_Box_SubArea);
	
	WMrWindow_SetVisible(prev, inShow);
	WMrWindow_SetVisible(next, inShow);
	WMrWindow_SetVisible(hint_text, inShow);
	WMrWindow_SetVisible(hint_box, inShow);
	
	prev = WMrDialog_GetItemByID(inDialog, ONcPS_Btn_HelpPrevious);
	next = WMrDialog_GetItemByID(inDialog, ONcPS_Btn_HelpNext);
	
// remove the UUcFalse if you want to have multiple pages for help
	WMrWindow_SetVisible(prev, UUcFalse/*!inShow*/);
	WMrWindow_SetVisible(next, UUcFalse/*!inShow*/);
}

// ----------------------------------------------------------------------
static void
ONiPS_EnableButtons(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData)
{
	UUtBool						prev_on;
	UUtBool						next_on;
	
	WMtWindow					*prev;
	WMtWindow					*next;
	
	prev_on = UUcTrue;
	next_on = UUcTrue;
	
	prev = WMrDialog_GetItemByID(inDialog, ONcPS_Btn_Previous);
	next = WMrDialog_GetItemByID(inDialog, ONcPS_Btn_Next);
	
	switch (inData->state)
	{
		case ONcPSState_Objective:
		{
			UUtInt16			max_objective;
			
			if (inData->objective == NULL) {
				// CB: no objectives, so no buttons
				prev_on = next_on = UUcFalse;
			} else {
				max_objective =
					UUmMin(
						(UUtInt16)inData->objective->objectives->num_pages,
						ONgObjectiveNumber);
				max_objective = UUmPin(max_objective, 0, (UUtInt16)inData->objective->objectives->num_pages);
						
				if (inData->objective_page_num == 0) { next_on = UUcFalse; }
				if ((inData->objective_page_num + 1 >= max_objective) || (ONgObjectiveNumber == 0))
				{
					prev_on = UUcFalse;
				}
			}
		}
		break;
		
		case ONcPSState_Items:
			if (inData->num_items == 0)
			{
				prev_on = UUcFalse;
				next_on = UUcFalse;
				break;
			}
			if (inData->item_page_num == 0) { prev_on = UUcFalse; }
			if (inData->item_page_num + 1 >= inData->num_items) { next_on = UUcFalse; }
		break;
		
		case ONcPSState_Weapons:
			if (inData->num_weapon_pages == 0)
			{
				prev_on = UUcFalse;
				next_on = UUcFalse;
				break;
			}
			if (inData->weapon_page_num == 0) { prev_on = UUcFalse; }
			if (inData->weapon_page_num + 1 >= inData->num_weapon_pages) { next_on = UUcFalse; }
		break;
		
		case ONcPSState_Diary:
			if (inData->num_diary_pages == 0)
			{
				prev_on = UUcFalse;
				next_on = UUcFalse;
				break;
			}
			if (inData->diary_page_num == 0) { prev_on = UUcFalse; }
			if (inData->diary_page_num + 1 >= inData->num_diary_pages) { next_on = UUcFalse; }
		break;

		case ONcPSState_Help:
// enable this code if you are going to use multiple help pages
/*			if (inData->num_help_pages == 0)
			{
				prev_on = UUcFalse;
				next_on = UUcFalse;
				break;
			}
			prev = WMrDialog_GetItemByID(inDialog, ONcPS_Btn_HelpPrevious);
			next = WMrDialog_GetItemByID(inDialog, ONcPS_Btn_HelpNext);
			if (inData->help_page_num == 0) { prev_on = UUcFalse; }
			if (inData->help_page_num + 1 >= inData->num_help_pages) { next_on = UUcFalse; }*/
		break;
	}
	
	WMrWindow_SetEnabled(prev, prev_on);
	WMrWindow_SetEnabled(next, next_on);
}

// ----------------------------------------------------------------------
static int UUcExternal_Call
ONiDP_SortByLevel(
	const void					*inItem1,
	const void					*inItem2)
{
	const ONtDiaryPage			*page1;
	const ONtDiaryPage			*page2;
	int							result;
	
	page1 = *((ONtDiaryPage**)inItem1);
	page2 = *((ONtDiaryPage**)inItem2);
	
	result = (page1->level_number - page2->level_number);

	return result;
}

static int UUcExternal_Call
ONiDP_SortByPage(
	const void					*inItem1,
	const void					*inItem2)
{
	const ONtDiaryPage			*page1;
	const ONtDiaryPage			*page2;
	int							result;
	
	page1 = *((ONtDiaryPage**)inItem1);
	page2 = *((ONtDiaryPage**)inItem2);
	
	result = (page1->page_number - page2->page_number);
	
	return result;
}

static UUtError
ONiPS_DiaryPage_Init(
	ONtPauseScreenData			*inData,
	UUtInt16					inUnlockedPage)
{
	UUtError					error;
	ONtDiaryPage				*pages[1024];
	UUtUns32					num_pages;
	UUtInt16					itr;
	UUtInt16					index;
	UUtInt16					first_page_index, target_page_index;
	UUtInt16					target_page, prev_unlocked_page;
	
	prev_unlocked_page = ONgDiaryPageUnlocked;
	ONgDiaryPageUnlocked = inUnlockedPage;

	// default initializations
	inData->num_diary_pages = 0;
	inData->diary_page_num = 0;
	
	// get a list of pointers
	error =
		TMrInstance_GetDataPtr_List(
			ONcTemplate_DiaryPage,
			1024,
			&num_pages,
			pages);
	UUmError_ReturnOnError(error);
	
	// add all of the valid diary pages to the list
	for (itr = 0; itr < (UUtInt16)num_pages; itr++)
	{
		// don't add pages from future levels
		if (pages[itr]->level_number > inData->current_level) { continue; }

		// if only certain pages have been unlocked on this level,
		// add only those
		if ((pages[itr]->level_number == inData->current_level) &&
			(inUnlockedPage != -1) &&
			(pages[itr]->page_number > inUnlockedPage)) { continue; }
		
		// add the page to the data
		inData->diary[inData->num_diary_pages] = pages[itr];
		inData->num_diary_pages++;
	}
	
	// sort by level number and page number
	qsort(
		inData->diary,
		inData->num_diary_pages,
		sizeof(ONtDiaryPage*),
		ONiDP_SortByLevel);

	index = 0;
	do
	{
		UUtInt16					num_level_pages;
		UUtInt16					sort_level;
		
		if (index == inData->num_diary_pages) { break; }
		
		sort_level = inData->diary[index]->level_number;
		num_level_pages = 0;
		for (itr = index; itr < inData->num_diary_pages ; itr++)
		{
			if (inData->diary[itr]->level_number != sort_level) { break; }
			num_level_pages++;
		}
		
		// sort the level pages
		if (num_level_pages > 1)
		{
			qsort(
				&inData->diary[index],
				num_level_pages,
				sizeof(ONtDiaryPage*),
				ONiDP_SortByPage);
		}
		
		// start at the next set of level pages
		index += num_level_pages;
	}
	while (1);
	
	// find the target page of the current level
	target_page = (inUnlockedPage == -1) ? 1 : inUnlockedPage;
	first_page_index = -1;
	target_page_index = -1;
	for (itr = 0; itr < inData->num_diary_pages; itr++)
	{
		if (ONgInGameUI_LastRestoreGame == (UUtUns32) -1) {
			// this message should only ever appear if you didn't load from a saved game
			if ((inData->diary[itr]->has_new_move) && (inData->diary[itr]->level_number == inData->current_level) &&
				(inData->diary[itr]->page_number > prev_unlocked_page)) {
				// this is a page with a new move which was only just unlocked
				UUmAssert((inUnlockedPage == -1) || (inData->diary[itr]->page_number <= inUnlockedPage));
				ONgInGameUI_NewMove = UUcTrue;
				ONgInGameUI_ImmediatePrompt = UUcTrue;
			}
		}

		if ((first_page_index == -1) && (inData->diary[itr]->level_number >= inData->current_level)) {
			// we found the first page for this level
			first_page_index = itr;
		}

		if ((inData->diary[itr]->level_number == inData->current_level) &&
			(inData->diary[itr]->page_number == target_page)) {
			// this is the page that we want to start on
			target_page_index = itr;
		}
	}
			
	if (target_page_index != -1) {
		inData->diary_page_num = target_page_index;

	} else if (first_page_index != -1) {
		inData->diary_page_num = first_page_index;

	} else {
		inData->diary_page_num = 0;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiPS_ItemPage_Init(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData)
{
	UUtError					error;
	ONtItemPage					*pages[1024];
	UUtUns32					num_pages;
	ONtCharacter				*player;
	UUtUns32					i;
	UUtUns32					pages_added;
	UUtBool						page_enabled;
	
	inData->num_items = 0;

	// get a list of pointers
	error =
		TMrInstance_GetDataPtr_List(
			ONcTemplate_ItemPage,
			1024,
			&num_pages,
			pages);
	UUmError_ReturnOnError(error);
	
	// count the items
	player = ONrGameState_GetPlayerCharacter();
	pages_added = 0;
	
	// by default, show the first page
	inData->item_page_num = 0;

	// save the items
	for (i = 0; i < num_pages; i++)
	{
		if (pages_added & (1 << pages[i]->powerup_type)) {
			// we have already added a page for this item type
			continue;
		}

		if (pages[i]->powerup_type == WPcPowerup_LSI) {
			// the LSI does not persist, you can only read about it if you
			// are currently carrying it
			page_enabled = (player->inventory.has_lsi > 0);
		} else {
			page_enabled = ONrPersist_ItemUnlocked(pages[i]->powerup_type);
		}

		if (page_enabled) {
			if (pages[i]->powerup_type == ONgInGameUI_NewItem) {
				// display this item since the player just picked it up
				inData->item_page_num = inData->num_items;
			}

			inData->items[inData->num_items] = pages[i];
			inData->num_items++;
			pages_added |= (1 << pages[i]->powerup_type);
		}

		if (inData->num_items >= ONcIGU_MaxItems) {
			UUmAssert(!"ONiPS_ItemPage_Init: too many items available!");
			break;
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiPS_ObjectivePage_Init(
	ONtPauseScreenData			*inData)
{
	UUtError					error;
	ONtObjectivePage			*pages[1024];
	UUtUns32					num_pages;
	UUtUns32					i;
	
	// clear the vars
	inData->objective_number = 0;
	inData->objective_page_num = 0;
	
	// get a list of pointers
	error =
		TMrInstance_GetDataPtr_List(
			ONcTemplate_ObjectivePage,
			1024,
			&num_pages,
			pages);
	UUmError_ReturnOnError(error);
	
	// get the objective pages for this level
	for (i = 0; i < num_pages; i++)
	{
		if (pages[i]->level_number != inData->current_level) { continue; }

		inData->objective = pages[i];
		break;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiPS_WeaponPage_Init(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData)
{
	UUtError					error;
	ONtWeaponPage				*pages[1024];
	UUtUns32					num_pages;
	UUtUns32					i;
	UUtInt16					j;
	ONtCharacter				*player;
	WPtWeaponClass				*cur_weapon;

	// initialize the vars
	inData->num_weapon_pages = 0;
	inData->weapon_page_num = 0;
	
	// get a list of pointers
	error =
		TMrInstance_GetDataPtr_List(
			ONcTemplate_WeaponPage,
			1024,
			&num_pages,
			pages);
	UUmError_ReturnOnError(error);
	
#if 1
	// CB: this displays weapon pages for all weapons that we have unlocked
	for (i = 0; i < num_pages; i++) {
		if (!ONrPersist_WeaponUnlocked(pages[i]->weaponClass))
			continue;

		if (inData->num_weapon_pages >= ONcIGU_MaxWeapons)
			break;

		inData->weapons[inData->num_weapon_pages] = pages[i];
		inData->num_weapon_pages++;
	}

	// work out which weapon to display by default
	player = ONrGameState_GetPlayerCharacter();
	cur_weapon = NULL;
	if (ONgInGameUI_NewWeaponClass != NULL) {
		// display the new weapon that we unlocked
		cur_weapon = ONgInGameUI_NewWeaponClass;

	} else if (player != NULL) {
		// display the player's current weapons
		for (j = 0; j < WPcMaxSlots; j++) {
			if (player->inventory.weapons[j] != NULL) {
				cur_weapon = WPrGetClass(player->inventory.weapons[j]);
				break;
			}
		}
	}

	// find the page for this weapon if we can
	if (cur_weapon != NULL) {
		for (i = 0; i < (UUtUns32) inData->num_weapon_pages; i++) {
			if (inData->weapons[i]->weaponClass == cur_weapon) {
				inData->weapon_page_num = (UUtInt16) i;
				break;
			}
		}
	}
#else
	// CB: this displays only a weapon page for the weapon that we are carrying

	// save the number of weapon pages to use
	player = ONrGameState_GetPlayerCharacter();
	if (player)
	{
		for (i = 0; i < WPcMaxSlots; i++)
		{
			WPtWeapon					*weapon;
			UUtBool						found;
			
			weapon = player->inventory.weapons[i];
			if (weapon == NULL) { continue; }
			
			// see if this weapon already has a page
			found = UUcFalse;
			for (j = 0; j < inData->num_weapon_pages; j++)
			{
				if (inData->weapons[j]->weaponClass == WPrGetClass(weapon))
				{
					found = UUcTrue;
					break;
				}
			}
			if (found == UUcTrue) { continue; }
			
			// find an open page for the weapon
			for (j = 0; j < ONcIGU_MaxWeapons; j++)
			{
				if (inData->weapons[j] != NULL) { continue; }
				break;
			}
			
			// break if there are no more available pages
			if (j == ONcIGU_MaxWeapons) { break; }

			// find the page to store
			for (j = 0;	j < (UUtInt16)num_pages; j++)
			{			
				if (pages[j]->weaponClass != WPrGetClass(weapon))
				{
					continue;
				}
				
				inData->weapons[inData->num_weapon_pages] = pages[j];
				inData->num_weapon_pages++;
				break;
			}
		}
	}
#endif
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static int UUcExternal_Call
ONiHP_SortByPage(
	const void					*inItem1,
	const void					*inItem2)
{
	const ONtHelpPage			*page1;
	const ONtHelpPage			*page2;
	int							result;
	
	page1 = *((ONtHelpPage**)inItem1);
	page2 = *((ONtHelpPage**)inItem2);
	
	result = (page1->page_number - page2->page_number);
	
	return result;
}

static UUtError
ONiPS_HelpPage_Init(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData)
{
	UUtError					error;
	ONtHelpPage					*pages[1024];
	UUtUns32					num_pages;
	UUtUns32					itr;
	
	// initialize the vars
	inData->num_help_pages = 0;
	inData->help_page_num = 0;
	
	// get a list of pointers
	error =
		TMrInstance_GetDataPtr_List(
			ONcTemplate_HelpPage,
			1024,
			&num_pages,
			pages);
	UUmError_ReturnOnError(error);
	
	// put pointers to all of the help pages into the list
	for (itr = 0; itr < num_pages; itr++)
	{
		inData->help[inData->num_help_pages] = pages[itr];
		inData->num_help_pages++;
	}
	
	// sort the help pages by page number
	qsort(
		inData->help,
		inData->num_help_pages,
		sizeof(ONtHelpPage*),
		ONiHP_SortByPage);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
ONiPS_Paint_Diary(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData,
	WMtDrawItem					*inDrawItem)
{
	ONtDiaryPage				*page;
	
	page = inData->diary[inData->diary_page_num];

	// mark that we have now read this page
	inData->highlighted_sections &= ~(1 << ONcPSState_Diary);
	ONgInGameUI_NewMove = UUcFalse;
	ONrPersist_MarkDiaryPageRead(page->level_number, page->page_number);
	
	// draw the diary page
//	if (inDrawItem->window_id == ONcPS_Txt_MainArea)
	{
		// draw the diary page
		ONiIGUI_Page_PaintOverlay(page->page, inDrawItem);
	}
//	else if (inDrawItem->window_id == ONcPS_Txt_SubArea)
	if (inDrawItem->window_id == ONcPS_Txt_SubArea)
	{
		IMtPoint2D					dest;
		UUtInt16					i;
		UUtInt16					num_keys;
		UUtInt16					keys_width;
		UUtInt16					window_width;
		
		WMrWindow_GetSize(inDrawItem->window, &window_width, NULL);
		
		// count the keys
		num_keys = 0;
		for (i = 0; i < ONcMaxNumKeys; i++)
		{
			if (page->keys[i] == 0) { break; }
			num_keys++;
		}
		
		// calculate the starting points for the keys
		keys_width = (ONcKeyWidth + ONcKeySpace) * num_keys;
		
		// draw the keys
		dest.x = (window_width - keys_width) >> 1;
		dest.y = 5;
		
		// draw the move icons
		for (i = 0; i < ONcMaxNumKeys; i++)
		{
			M3tTextureMap				*icon;
			
			if (page->keys[i] == 0) { break; }
			
			icon = NULL;
			
			switch (page->keys[i] & ONcKey_Action)
			{
				case ONcKey_Punch:		icon = ONgKeyIcons->punch; 		break;
				case ONcKey_Kick:		icon = ONgKeyIcons->kick; 		break;
				case ONcKey_Forward:	icon = ONgKeyIcons->forward; 	break;
				case ONcKey_Backward:	icon = ONgKeyIcons->backward; 	break;
				case ONcKey_Left:		icon = ONgKeyIcons->left; 		break;
				case ONcKey_Right:		icon = ONgKeyIcons->right; 		break;
				case ONcKey_Crouch:		icon = ONgKeyIcons->crouch; 	break;
				case ONcKey_Jump:		icon = ONgKeyIcons->jump; 		break;
			}
			
			switch (page->keys[i])
			{
				case ONcKey_Plus:		icon = ONgKeyIcons->plus; 		break;
				case ONcKey_Wait:		/* no icon on purpose */		break;
			}
			
			if (icon != NULL)
			{
				if (page->keys[i] & ONcKey_Quick)
				{
					IMtPoint2D			quick_dest;
					
					quick_dest = dest;
					quick_dest.y += (icon->height >> 1);
					
					DCrDraw_TextureRef(
						inDrawItem->draw_context,
						icon,
						&quick_dest,
						(icon->width >> 1),
						(icon->height >> 1),
						UUcTrue,
						M3cMaxAlpha);
				}
				else
				{
					DCrDraw_TextureRef(
						inDrawItem->draw_context,
						icon,
						&dest,
						-1,
						-1,
						UUcFalse,
						M3cMaxAlpha);
				}
			}
			
			// draw a ring around the icon if it is a hold
			if (page->keys[i] & ONcKey_Hold)
			{
				DCrDraw_TextureRef(
					inDrawItem->draw_context,
					ONgKeyIcons->hold,
					&dest,
					-1,
					-1,
					UUcFalse,
					M3cMaxAlpha);
			}
			
			// move to the next position
			dest.x += 34;
		}
	}
}

// ----------------------------------------------------------------------
static void
ONiPS_Paint_Items(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData,
	WMtDrawItem					*inDrawItem)
{
	inData->highlighted_sections &= ~(1 << ONcPSState_Items);
	ONgInGameUI_NewItem = (UUtUns32) -1;

	if (inData->num_items == 0) { return; }
	if (inData->items[inData->item_page_num] == NULL) { return; }
	ONiIGUI_Page_Paint(inData->items[inData->item_page_num]->page, inDrawItem);
}

// ----------------------------------------------------------------------
static void
ONiPS_Paint_Objective(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData,
	WMtDrawItem					*inDrawItem)
{
	ONtIGUI_FontInfo			font_info;
	UUtInt16					page_num;
	
	// we are looking at the objectives page, stop highlighting it
	inData->highlighted_sections &= ~(1 << ONcPSState_Objective);
	ONgInGameUI_NewObjective = UUcFalse;
	COrMessage_Remove(ONcNewObjectiveMessage);

	if (ONgObjectiveNumber == 0) { return; }
	if (inData->objective == NULL) { return; }
	if (inData->objective->objectives->pages[inData->objective_page_num] == NULL)
	{
		return;
	}
	
	// set the font color
	if (inData->objective_page_num == inData->objective_number)
	{
		font_info.flags = IGUIcFontInfoFlag_FontShade;
		font_info.font_shade = IMcShade_Green;
	}
	else
	{
		font_info.flags = IGUIcFontInfoFlag_FontShade;
		font_info.font_shade = IMcShade_Red;
	}
	
	ONiFIStack_Push(&font_info);
	
	page_num = UUmMax((ONgObjectiveNumber - inData->objective_page_num) - 1, 0);
	ONiIGUI_Page_Paint(
		inData->objective->objectives->pages[page_num],
		inDrawItem);
	
	ONiFIStack_Pop();
}

// ----------------------------------------------------------------------
static void
ONiPS_Paint_Weapons(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData,
	WMtDrawItem					*inDrawItem)
{
	inData->highlighted_sections &= ~(1 << ONcPSState_Weapons);
	ONgInGameUI_NewWeaponClass = NULL;

	if (inData->num_weapon_pages == 0) { return; }
	if (inData->weapons[inData->weapon_page_num] == NULL) { return; }
	ONiIGUI_Page_Paint(inData->weapons[inData->weapon_page_num]->page, inDrawItem);
}

// ----------------------------------------------------------------------
static void
ONiPS_Paint_Help(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData,
	WMtDrawItem					*inDrawItem)
{
	if (inData->help[inData->help_page_num] == NULL) { return; }
	ONiIGUI_Page_Paint(inData->help[inData->help_page_num]->page, inDrawItem);
}

// ----------------------------------------------------------------------
static void
ONiPS_Previous(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData)
{
	switch (inData->state)
	{
		case ONcPSState_Objective:
		{
			UUtInt16					max_objective;
			
			inData->objective_page_num++;
			max_objective =
				UUmMin(
					(UUtInt16)inData->objective->objectives->num_pages,
					ONgObjectiveNumber);
			max_objective = UUmPin(max_objective, 0, (UUtInt16)inData->objective->objectives->num_pages);
			if (inData->objective_page_num >= max_objective)
			{
				inData->objective_page_num = (max_objective - 1);
			}
		}
		break;
		
		case ONcPSState_Items:
			inData->item_page_num--;
			if (inData->item_page_num < 0) { inData->item_page_num = 0; }
		break;
		
		case ONcPSState_Weapons:
			inData->weapon_page_num--;
			if (inData->weapon_page_num < 0) { inData->weapon_page_num = 0; }
		break;
		
		case ONcPSState_Diary:
			inData->diary_page_num--;
			if (inData->diary_page_num < 0) { inData->diary_page_num = 0; }
		break;

		case ONcPSState_Help:
			inData->help_page_num--;
			if (inData->help_page_num < 0) { inData->help_page_num = 0; }
		break;
	}
	
	ONiPS_EnableButtons(inDialog, inData);
}

// ----------------------------------------------------------------------
static void
ONiPS_Next(
	WMtDialog					*inDialog,
	ONtPauseScreenData			*inData)
{
	switch (inData->state)
	{
		case ONcPSState_Objective:
			inData->objective_page_num--;
			if (inData->objective_page_num < 0) { inData->objective_page_num = 0; }
		break;
		
		case ONcPSState_Items:
			inData->item_page_num++;
			if (inData->item_page_num >= inData->num_items)
			{
				inData->item_page_num = (inData->num_items - 1);
			}
		break;
		
		case ONcPSState_Weapons:
			inData->weapon_page_num++;
			if (inData->weapon_page_num >= inData->num_weapon_pages)
			{
				inData->weapon_page_num = (inData->num_weapon_pages - 1);
			}
		break;
		
		case ONcPSState_Diary:
			inData->diary_page_num++;
			if (inData->diary_page_num >= inData->num_diary_pages)
			{
				inData->diary_page_num = (inData->num_diary_pages - 1);
			}
		break;

		case ONcPSState_Help:
			inData->help_page_num++;
			if (inData->help_page_num >= inData->num_help_pages)
			{
				inData->help_page_num = (inData->num_help_pages - 1);
			}
		break;
	}
}

// ----------------------------------------------------------------------
static void
ONiPauseScreen_InitDialog(
	WMtDialog					*inDialog)
{
	UUtError					error;
	ONtPauseScreenData			*data;
	
	// create the data
	data = &ONgPauseScreenData;
	WMrDialog_SetUserData(inDialog, (UUtUns32)data);
	
	// initialize
	error = ONiPS_ItemPage_Init(inDialog, data);
	if (error != UUcError_None) { WMrDialog_ModalEnd(inDialog, 0); return; }

	// CB: diary and objective pages have already been initialized at level load

	error = ONiPS_WeaponPage_Init(inDialog, data);
	if (error != UUcError_None) { WMrDialog_ModalEnd(inDialog, 0); return; }
	
	error = ONiPS_HelpPage_Init(inDialog, data);
	if (error != UUcError_None) { WMrDialog_ModalEnd(inDialog, 0); return; }

	// determine which entries in the dialog are highlighted
	data->highlighted_sections = 0;
	if (ONgInGameUI_NewObjective) {
		data->highlighted_sections |= (1 << ONcPSState_Objective);
	}
	if (ONgInGameUI_NewWeaponClass != NULL) {
		data->highlighted_sections |= (1 << ONcPSState_Weapons);
	}
	if (ONgInGameUI_NewItem != (UUtUns32) -1) {
		data->highlighted_sections |= (1 << ONcPSState_Items);
	}
	if (ONgInGameUI_NewMove) {
		data->highlighted_sections |= (1 << ONcPSState_Diary);
	}

	// select the objective screen
	WMrDialog_ToggleButtonCheck(inDialog, ONcPS_Btn_Objective, ONcPS_Btn_Help, ONcPS_Btn_Objective);
	data->state = ONcPSState_Objective;
	
	// set the start time
	data->start_time = UUrMachineTime_Sixtieths() + (ONcPauseScreen_KeyAcceptDelay * 60);
	
	// hide the hint area
	ONiPS_ShowHintArea(inDialog, data, UUcTrue);

	// enable the buttons
	ONiPS_EnableButtons(inDialog, data);

	// turn off the message window while the pause display is up
	COrConsole_TemporaryDisable(UUcTrue);
	COrMessage_TemporaryDisable(UUcTrue);
}

// ----------------------------------------------------------------------
static void
ONiPauseScreen_Destroy(
	WMtDialog					*inDialog)
{
	ONrInGameUI_EnableHelpDisplay(UUcFalse);
	COrConsole_TemporaryDisable(UUcFalse);
	COrMessage_TemporaryDisable(UUcFalse);
}

// ----------------------------------------------------------------------
static void
ONiPauseScreen_DrawItem(
	WMtDialog					*inDialog,
	WMtDrawItem					*inDrawItem)
{
	ONtPauseScreenData			*data;
	TStFontInfo					font_info;
	
	WMrWindow_GetFontInfo(inDrawItem->window, &font_info);
	
	ONiFIStack_Init(
		font_info.font_family,
		font_info.font_size,
		font_info.font_style,
		font_info.font_shade);
	DCrText_SetFormat(TSc_HLeft | TSc_VTop);

	if ((inDrawItem->window_id == ONcPS_Txt_SubArea) && (ONgPauseScreenOverrideMessage != NULL)) {
		IMtPoint2D dest;
		UUtRect rect;

		// draw the override text instead of the normal hint text
		dest.x = 0;
		dest.y = DCrText_GetAscendingHeight();
		
		DCrDraw_String2(
			inDrawItem->draw_context,
			ONgPauseScreenOverrideMessage,
			NULL,
			&dest,
			&rect);				

		return;
	}
	
	data = (ONtPauseScreenData*)WMrDialog_GetUserData(inDialog);
	switch (data->state)
	{
		case ONcPSState_Objective:
			ONiPS_Paint_Objective(inDialog, data, inDrawItem);
		break;
		
		case ONcPSState_Items:
			ONiPS_Paint_Items(inDialog, data, inDrawItem);
		break;
		
		case ONcPSState_Weapons:
			ONiPS_Paint_Weapons(inDialog, data, inDrawItem);
		break;
		
		case ONcPSState_Diary:
			ONiPS_Paint_Diary(inDialog, data, inDrawItem);
		break;

		case ONcPSState_Help:
			ONiPS_Paint_Help(inDialog, data, inDrawItem);
		break;
	}
}

// ----------------------------------------------------------------------
static void
ONiPauseScreen_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	WMtWindow					*inControl)
{
	UUtUns16					control_id;
	ONtPauseScreenData			*data;
	UUtBool						show_help;
	
	data = (ONtPauseScreenData*)WMrDialog_GetUserData(inDialog);
	
	control_id = UUmLowWord(inParam1);
	
	switch (control_id)
	{
		case ONcPS_Btn_Objective:
			data->state = ONcPSState_Objective;
		break;
		
		case ONcPS_Btn_Items:
			data->state = ONcPSState_Items;
		break;
		
		case ONcPS_Btn_Weapons:
			data->state = ONcPSState_Weapons;
		break;
		
		case ONcPS_Btn_Diary:
			data->state = ONcPSState_Diary;
		break;
		
		case ONcPS_Btn_Help:
			data->state = ONcPSState_Help;
		break;

		case ONcPS_Btn_Previous:
		case ONcPS_Btn_HelpPrevious:
			ONiPS_Previous(inDialog, data);
		break;
		
		case ONcPS_Btn_Next:
		case ONcPS_Btn_HelpNext:
			ONiPS_Next(inDialog, data);
		break;
		
		case WMcDialogItem_Cancel:
			WMrDialog_ModalEnd(inDialog, 0);
		break;
	}
	
	switch (control_id)
	{
		case ONcPS_Btn_Objective:
		case ONcPS_Btn_Items:
		case ONcPS_Btn_Weapons:
		case ONcPS_Btn_Diary:
		case ONcPS_Btn_Help:
			WMrDialog_ToggleButtonCheck(inDialog, ONcPS_Btn_Objective, ONcPS_Btn_Help, control_id);

			// if we are on the help page, disable the hint area and turn on the HUD indicators
			show_help = (data->state == ONcPSState_Help);
			ONiPS_ShowHintArea(inDialog, data, !show_help);
			ONrInGameUI_EnableHelpDisplay(show_help);
		break;
	}
	
	// enable the buttons
	ONiPS_EnableButtons(inDialog, data);
}

// ----------------------------------------------------------------------
static void
ONiPauseScreen_Paint(
	WMtDialog					*inDialog)
{
	UUtUns32					itr, time;
	UUtBool						force_selected;
	float						flash_val;
	WMtWindow					*window;
	WMtWindowClass				*window_class;
	TStFontInfo					font_info;
	ONtPauseScreenData			*data;
	ONtPauseScreenHighlightableButton	*button;
	IMtShade					selected_shade, highlighted_shade, unselected_shade;

	data = (ONtPauseScreenData*)WMrDialog_GetUserData(inDialog);
	
	if (data->start_time < UUrMachineTime_Sixtieths())
	{
		if (LIrTestKey(LIcKeyCode_F1) == UUcTrue)
		{
			// stop the dialog
			WMrDialog_ModalEnd(inDialog, 0);
		}
	}

	// determine the button colors
	selected_shade = IMcShade_White;
	highlighted_shade = IMcShade_Yellow;
	unselected_shade = IMcShade_Orange;

	time = (UUrMachineTime_Sixtieths() + 180 - data->start_time) % 90;
	if (time < 60) {
		flash_val = MUrSin((M3cPi * time) / 60);
		highlighted_shade = IMrShade_Interpolate(unselected_shade, IMcShade_Yellow, flash_val);
	} else {
		highlighted_shade = unselected_shade;
	}

	// set the colors of buttons based on whether they are highlighted or not
	for (itr = 0, button = gPSHighlightableButtons; (button->window_id != 0); itr++, button++) {
		window = WMrDialog_GetItemByID(inDialog, button->window_id);
		if (window == NULL)
			continue;

		force_selected = UUcFalse;
		window_class = WMrWindow_GetClass(window);
		if ((window_class != NULL) && (window_class->type == WMcWindowType_Button)) {
			force_selected = WMrButton_IsHighlighted(window);
		}

		WMrWindow_GetFontInfo(window, &font_info);
		if (force_selected || (button->highlight_bit == data->state)) {
			font_info.font_shade = selected_shade;

		} else if (data->highlighted_sections & (1 << button->highlight_bit)) {
			font_info.font_shade = highlighted_shade;

		} else {
			font_info.font_shade = unselected_shade;
		}
		WMrWindow_SetFontInfo(window, &font_info);
	}
}

// ----------------------------------------------------------------------
static UUtBool
ONiPauseScreen_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			ONiPauseScreen_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			ONiPauseScreen_Destroy(inDialog);
		break;
		
		case WMcMessage_Command:
			ONiPauseScreen_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;
		
		case WMcMessage_DrawItem:
			ONiPauseScreen_DrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		case WMcMessage_Paint:
			ONiPauseScreen_Paint(inDialog);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
void
ONrPauseScreen_Display(
	void)
{
	PStPartSpecUI			*partspec_ui;
	PStPartSpecUI			*temp_ui;
	LItMode					mode;
	float					orig_volume;
	SStImpulse				*impulse;
	
	// save the current ui
	partspec_ui = PSrPartSpecUI_GetActive();
	
	// set the ui to the out of game ui
	temp_ui = PSrPartSpecUI_GetByName(ONcInGameUIName);
	if (temp_ui != NULL) { PSrPartSpecUI_SetActive(temp_ui); }
	
	mode = LIrMode_Get();
	LIrMode_Set(LIcMode_Normal);
	WMrActivate();
	
	// play the open pause screen sound
	impulse = OSrImpulse_GetByName(ONcPauseScreen_OpenSound);
	if (impulse != NULL) { OSrImpulse_Play(impulse, NULL, NULL, NULL, NULL); }
	
	// lower the volume
	orig_volume = SS2rVolume_Get();
	SS2rVolume_Set(orig_volume * ONcPauseScreen_VolumeCut);
	
	WMrDialog_ModalBegin(
		ONcPauseScreenID,
		NULL,
		ONiPauseScreen_Callback,
		0,
		NULL);
	
	// restore the volume
	SS2rVolume_Set(orig_volume);
	
	// play the close pause screen sound
	impulse = OSrImpulse_GetByName(ONcPauseScreen_CloseSound);
	if (impulse != NULL) { OSrImpulse_Play(impulse, NULL, NULL, NULL, NULL); }
	
	WMrDeactivate();
	LIrMode_Set(mode);
	
	// reset the active ui
	PSrPartSpecUI_SetActive(partspec_ui);
	
	// clear any override messages
	ONrPauseScreen_OverrideMessage(NULL);
}

// ----------------------------------------------------------------------
void
ONrPauseScreen_OverrideMessage(
	const char *inMessage)
{
	ONgPauseScreenOverrideMessage = inMessage;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiPauseScreen_RegisterTemplates(
	void)
{
	UUtError					error;
	
	error =
		TMrTemplate_Register(
			ONcTemplate_IGUI_String,
			sizeof(ONtIGUI_String),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			ONcTemplate_IGUI_StringArray,
			sizeof(ONtIGUI_StringArray),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			ONcTemplate_IGUI_Page,
			sizeof(ONtIGUI_Page),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			ONcTemplate_IGUI_PageArray,
			sizeof(ONtIGUI_PageArray),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	
	error =
		TMrTemplate_Register(
			ONcTemplate_DiaryPage,
			sizeof(ONtDiaryPage),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			ONcTemplate_ItemPage,
			sizeof(ONtItemPage),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			ONcTemplate_ObjectivePage,
			sizeof(ONtObjectivePage),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			ONcTemplate_WeaponPage,
			sizeof(ONtWeaponPage),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			ONcTemplate_HelpPage,
			sizeof(ONtHelpPage),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			ONcTemplate_IGUI_HUDHelp,
			sizeof(ONtIGUI_HUDHelp),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiPauseScreen_Initialize(
	void)
{
	UUtError					error;
	
	// register the templates
	error = ONiPauseScreen_RegisterTemplates();
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiPauseScreen_LevelLoad(
	UUtUns32					inLevelNum)
{
	UUtError					error;
	
	// intialize the pause screen data
	UUrMemory_Clear(&ONgPauseScreenData, sizeof(ONtPauseScreenData));
	ONgPauseScreenData.current_level = (UUtInt16)inLevelNum;
	
	// initialize the objective page
	error = ONiPS_ObjectivePage_Init(&ONgPauseScreenData);
	UUmError_ReturnOnError(error);
	
	// initialize the diary page
	error = ONiPS_DiaryPage_Init(&ONgPauseScreenData, -1);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
ONiPauseScreen_LevelUnload(
	void)
{
}

// ----------------------------------------------------------------------
static void
ONiPauseScreen_Terminate(
	void)
{
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiInGameUI_DiaryPageUnlock(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONiPS_DiaryPage_Init(&ONgPauseScreenData, (UUtInt16) inParameterList[0].val.i);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiInGameUI_ObjectiveSet(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtBool silent = UUcFalse;
	UUtInt16 new_objective;

	if (NULL == ONgPauseScreenData.objective) {
		COrConsole_Printf("ONiInGameUI_ObjectiveSet found NULL ONgPauseScreenData.objective");
		goto exit;
	}

	if (NULL == ONgPauseScreenData.objective->objectives) {
		COrConsole_Printf("ONiInGameUI_ObjectiveSet found NULL ONgPauseScreenData.objective->objectives");
		goto exit;
	}

	// get the number of the objective to set
	new_objective = UUmMin((UUtInt16)inParameterList->val.i,
							(UUtInt16)ONgPauseScreenData.objective->objectives->num_pages);
	if (new_objective == ONgObjectiveNumber) {
		// we already have this objective set
		return UUcError_None;
	}
	ONgObjectiveNumber = new_objective;
	
	if ((inParameterListLength >= 2) && (inParameterList[1].type == SLcType_String) &&
		(UUrString_Compare_NoCase(inParameterList[1].val.str, "silent") == 0)) {
		// this objective is set silently - no prompting
		silent = UUcTrue;
	}

	if ((ONgInGameUI_LastRestoreGame != (UUtUns32) -1) && 
		(ONrGameState_GetGameTime() < ONgInGameUI_LastRestoreGame + ONcObjective_IgnoreAfterRestoreGame)) {
		// we restored our game recently, this objective should be silent
		silent = UUcTrue;
	}

	if (!silent) {
		// we should tell the player about this new objective
		ONgInGameUI_NewObjective = UUcTrue;
		ONgInGameUI_ImmediatePrompt = UUcTrue;
	}

exit:
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiInGameUI_ObjectiveComplete(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	// play the objective complete sound
	ONrGameState_EventSound_Play(ONcEventSound_ObjectiveComplete, NULL);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiInGameUI_TargetSet(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtInt16				flag_number;
	UUtBool					result;
	M3tPoint3D				location;
	
	// if the distance value is 0.0f, then turn off the radar
	if (inParameterList[1].val.f == 0.0f)
	{
		gHasLSITarget = UUcFalse;
		return UUcError_None;
	}

	// get the number of the flag to make the target
	flag_number = (UUtInt16)inParameterList[0].val.i;
	
	result = ONrLevel_Flag_ID_To_Location(flag_number, &location);
	if (result == UUcTrue)
	{
		ONtCharacter				*player;
		M3tVector3D					player_to_target;
		
		if (gHasLSITarget && (MUrPoint_Distance_Squared(&location, &gLSITarget) < UUmSQR(ONcObjective_SuperimposedObjective))) {
			// we already have this target set, ignore the subsequent event
			return UUcError_None;
		}

		gLSITarget = location;

		// set the maximum distance
		player = ONrGameState_GetPlayerCharacter();
		MUmVector_Subtract(player_to_target, gLSITarget, player->location);
		gMaxDistanceSquared = MUmVector_GetLengthSquared(player_to_target);
		
		// set the minimum distance squared
		gMinDistanceSquared = inParameterList[1].val.f * inParameterList[1].val.f;
		
		gHasLSITarget = UUcTrue;
	}
	else
	{
		COrConsole_Printf("No Flag %d exists.", flag_number);
		gHasLSITarget = UUcFalse;
	}
	
	if (!ONrGameState_EventSound_InCutscene()) {
		ONrGameState_EventSound_Play(ONcEventSound_CompassNew, NULL);
	}

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
ONrInGameUI_Initialize(
	void)
{
	UUtError					error;
	
	// initialize
	error = ONiPauseScreen_Initialize();
	UUmError_ReturnOnError(error);
	
	// preset the UVs
	gFullTextureUVs[0].u = 0.0f;
	gFullTextureUVs[0].v = 0.0f;
	gFullTextureUVs[1].u = 1.0f;
	gFullTextureUVs[1].v = 0.0f;
	gFullTextureUVs[2].u = 0.0f;
	gFullTextureUVs[2].v = 1.0f;
	gFullTextureUVs[3].u = 1.0f;
	gFullTextureUVs[3].v = 1.0f;
	
	gFullTextureUVsInvert[0].u = 0.0f;
	gFullTextureUVsInvert[0].v = 1.0f;
	gFullTextureUVsInvert[1].u = 1.0f;
	gFullTextureUVsInvert[1].v = 1.0f;
	gFullTextureUVsInvert[2].u = 0.0f;
	gFullTextureUVsInvert[2].v = 0.0f;
	gFullTextureUVsInvert[3].u = 1.0f;
	gFullTextureUVsInvert[3].v = 0.0f;

	// create the script command
#if CONSOLE_DEBUGGING_COMMANDS
	error = 
		SLrScript_Command_Register_Void(
			"diary_page_unlock",
			"Unlocks a specific diary page on the current level.",
			"page:int",
			ONiInGameUI_DiaryPageUnlock);
	UUmError_ReturnOnError(error);

	error = 
		SLrScript_Command_Register_Void(
			"objective_complete",
			"Plays the objective-complete sound.",
			"",
			ONiInGameUI_ObjectiveComplete);
	UUmError_ReturnOnError(error);

	error = 
		SLrGlobalVariable_Register_Float(
			"target_max_distance",
			"Sets the distance at which the radar will start to change from its minimum.",
			&gMaxDistanceSquared);
	UUmError_ReturnOnError(error);

	error =
		SLrScript_Command_Register_Void(
			"ui_show_help",
			"debugging: enables the HUD help overlays",
			"enable:int",
			ONiInGameUI_SetHelpDisplayEnable);
	UUmError_ReturnOnError(error);

	error =
		SLrScript_Command_Register_Void(
			"ui_fill_element",
			"sets part of the HUD to completely filled",
			"element_name:string fill:int",
			ONiInGameUI_FillElement);
	UUmError_ReturnOnError(error);
#endif

	error = 
		SLrScript_Command_Register_Void(
			"objective_set",
			"Sets the current objective page.",
			"page:int [make_silent:string{\"silent\"} | ]",
			ONiInGameUI_ObjectiveSet);
	UUmError_ReturnOnError(error);

	error = 
		SLrScript_Command_Register_Void(
			"target_set",
			"Sets the target to the specified flag",
			"flag_id:int min_distance:float",
			ONiInGameUI_TargetSet);
	UUmError_ReturnOnError(error);
	
	gMaxDistanceSquared = 75000.0f;
	
	error =
		SLrScript_Command_Register_Void(
			"text_console",
			"Turns on the text console display",
			"name:string",
			ONiTextConsole_Display);
	UUmError_ReturnOnError(error);


	error =
		SLrScript_Command_Register_Void(
			"ui_show_element",
			"shows or hides part of the HUD",
			"element_name:string show:int",
			ONiInGameUI_ShowElement);
	UUmError_ReturnOnError(error);

	error =
		SLrScript_Command_Register_Void(
			"ui_flash_element",
			"sets part of the HUD to flash or not flash",
			"element_name:string fill:int",
			ONiInGameUI_FlashElement);
	UUmError_ReturnOnError(error);

	error = 
		SLrGlobalVariable_Register_Bool(
			"ui_suppress_prompt",
			"Suppresses UI prompting about new objectives or moves.",
			&ONgInGameUI_SuppressPrompting);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrInGameUI_LevelLoad(
	UUtUns32					inLevelNum)
{
	UUtError					error;
	
	error = ONiInGameUI_LevelLoad();
	UUmError_ReturnOnError(error);
	
	error = ONiPauseScreen_LevelLoad(inLevelNum);
	UUmError_ReturnOnError(error);

	// get a pointer to the key icons
	error = TMrInstance_GetDataPtr(ONcTemplate_KeyIcons, "keyicons", &ONgKeyIcons);
	UUmError_ReturnOnError(error);

	// set up our global variables so that we don't "fill up" to initial state
	gCurNumRounds	= (UUtUns16) -1;
	gCurMaxRounds	= (UUtUns16) -1;

	gHealth.needs_update = UUcTrue;
	gHealth.value = 0;
	gHealth.target_val = 0;
	gHealth.has_foreshadow = UUcFalse;
	gHealth.has_aftershadow = UUcFalse;

	gShield.needs_update = UUcTrue;
	gShield.value = 0;
	gShield.target_val = 0;
	gShield.has_foreshadow = UUcFalse;
	gShield.has_aftershadow = UUcFalse;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrInGameUI_LevelUnload(
	void)
{
	ONiInGameUI_LevelUnload();
	ONiPauseScreen_LevelUnload();
}

// ----------------------------------------------------------------------
UUtError
ONrInGameUI_RegisterTemplates(
	void)
{
	UUtError					error;
	
	error = ONiPauseScreen_RegisterTemplates();
	UUmError_ReturnOnError(error);
	
	error =
		TMrTemplate_Register(
			ONcTemplate_KeyIcons,
			sizeof(ONtKeyIcons),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			ONcTemplate_TextConsole,
			sizeof(ONtTextConsole),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrInGameUI_Terminate(
	void)
{
	ONiPauseScreen_Terminate();
}

// ----------------------------------------------------------------------
static void
ONiInGameUI_UpdateDamageMeter(
	ONtInGame_DamageMeter		*ioMeter,
	UUtUns32					inValue,
	UUtUns32					inTarget,
	float						inShadowDecayRate)
{
	if (ioMeter->has_aftershadow) {
		// decay our after-shadow over time
		ioMeter->shadow_val -= inShadowDecayRate;
		if (ioMeter->shadow_val <= UUmMax(0, ioMeter->value)) {
			ioMeter->has_aftershadow = UUcFalse;
		}

		ioMeter->needs_update = UUcTrue;
	}

	if (inTarget > inValue) {
		// we have a target value that is greater than our current value,
		// and must foreshadow
		ioMeter->has_aftershadow = UUcFalse;
		ioMeter->has_foreshadow = UUcTrue;
		if (ioMeter->target_val != inTarget) {
			ioMeter->target_val = inTarget;
			ioMeter->shadow_val = (float) ioMeter->target_val;
			ioMeter->needs_update = UUcTrue;
		}
	} else {
		// we have no target value
		ioMeter->target_val = inValue;
		if (ioMeter->has_foreshadow) {
			// we must have just reached our target value, remove the foreshadow
			ioMeter->has_foreshadow = UUcFalse;
			ioMeter->needs_update = UUcTrue;
		}
	}

	if (inValue > ioMeter->value) {
		// value is increasing
		if (ioMeter->has_aftershadow) {
			// remove our after-shadow effect
			ioMeter->has_aftershadow = UUcFalse;
		}
		ioMeter->needs_update = UUcTrue;

	} else if (inValue < ioMeter->value) {
		// value is decreasing, start after-shadow effect if we don't have a shadow effect
		if (!ioMeter->has_aftershadow && !ioMeter->has_foreshadow) {
			ioMeter->has_foreshadow = UUcFalse;
			ioMeter->has_aftershadow = UUcTrue;
			ioMeter->shadow_val = (float) ioMeter->value;
		}
		ioMeter->needs_update = UUcTrue;
	}

	ioMeter->value = inValue;
}

// ----------------------------------------------------------------------
void
ONrInGameUI_Update(
	void)
{
	ONtCharacter				*player;
	const char					*message_name, *message;
	UUtUns32					i, current_time;
	UUtInt16					hits;
	
	// get a pointer to the player's character
	player = ONrGameState_GetPlayerCharacter();
	if (player == NULL) { return; }

	// fade the hit quadrants
	for (i = 0; i < 4; i++) {
		if (player->hits[i] == 0)
			continue;

		hits = ((UUtInt16) player->hits[i]) - cHitsDecrementPerTick;
		if (hits < 0) {
			player->hits[i] = 0;
		} else {
			player->hits[i] = (UUtUns8) hits;
		}
	}

	// update the damage meters
	ONiInGameUI_UpdateDamageMeter(&gHealth, player->hitPoints, player->hitPoints + player->inventory.hypoRemaining,
					((float) player->maxHitPoints) / cHealthShadowDecayTime);
	ONiInGameUI_UpdateDamageMeter(&gShield, player->inventory.shieldDisplayAmount, player->inventory.shieldRemaining,
					((float) WPcMaxShields) / cShieldShadowDecayTime);

	/*
	 * intermittently prompt about our new objective
	 */

	if (!ONgInGameUI_SuppressPrompting && (ONgInGameUI_NewObjective || ONgInGameUI_NewMove))
	{
		current_time = ONrGameState_GetGameTime();

		if (ONgInGameUI_ImmediatePrompt || (current_time - ONgInGameUI_LastPromptTime > ONcObjective_PromptTime * 60)) {
			// bring up the prompt message
			if (ONgInGameUI_NewMove) {
				message_name = ONcNewMoveMessage;
			} else {
				message_name = ONcNewObjectiveMessage;
			}

			if (!ONrGameState_EventSound_InCutscene()) {
				if (ONgInGameUI_ImmediatePrompt) {
					ONrGameState_EventSound_Play(ONcEventSound_ObjectiveNew, NULL);
				} else {
					ONrGameState_EventSound_Play(ONcEventSound_ObjectivePrompt, NULL);
				}
			}

			// note: don't print another prompt message if one already exists
			if (!COrMessage_Visible(message_name)) {
				message = SSrMessage_Find(message_name);
				if (message != NULL) {
					COrMessage_Print(message, message_name, ONcNewObjectiveMessage_FadeTime);
				}
			}
			ONgInGameUI_ImmediatePrompt = UUcFalse;
			ONgInGameUI_LastPromptTime = current_time;
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiTC_SetPicture(
	WMtDialog					*inDialog,
	ONtTextConsoleData			*inTCD)
{
	WMtWindow					*picture;
	UUtInt32					i;
	
	// get a pointer to the picture control
	picture = WMrDialog_GetItemByID(inDialog, ONcTC_Pt_Picture);
	if (picture == NULL) { return; }
	
	// set the console_data picture
	for (i = inTCD->page; i >= 0; i--)
	{
		if (inTCD->text_console->console_data->pages[i]->pict == NULL) { continue; }
		
		WMrPicture_SetPicture(
			picture,
			inTCD->text_console->console_data->pages[i]->pict);
		break;
	}
}

// ----------------------------------------------------------------------
static void
ONiTC_SetButtons(
	WMtDialog					*inDialog,
	ONtTextConsoleData			*inTCD)
{
	WMtWindow					*next;
	WMtWindow					*close;
	
	next = WMrDialog_GetItemByID(inDialog, ONcTC_Btn_Next);
	close = WMrDialog_GetItemByID(inDialog, ONcTC_Btn_Close);
	
	if (inTCD->page >= ((UUtInt32)inTCD->text_console->console_data->num_pages) - 1)
	{
		WMrWindow_SetVisible(next, UUcFalse);
		WMrWindow_SetVisible(close, UUcTrue);
	}
	else
	{
		WMrWindow_SetVisible(next, UUcTrue);
		WMrWindow_SetVisible(close, UUcFalse);
	}
}

// ----------------------------------------------------------------------
static void
ONiTC_InitDialog(
	WMtDialog					*inDialog)
{
	ONtTextConsoleData			*tcd;
	
	// get a pointer to the text console data
	tcd = (ONtTextConsoleData*)WMrDialog_GetUserData(inDialog);
	if (tcd == NULL) { goto end; }
	
	tcd->page = 0;
	
	ONiTC_SetButtons(inDialog, tcd);
	
	return;
	
end:
	WMrDialog_ModalEnd(inDialog, 0);
}

// ----------------------------------------------------------------------
static void
ONiTC_HandleDrawItem(
	WMtDialog					*inDialog,
	WMtDrawItem					*inDrawItem)
{
	ONtTextConsoleData			*tcd;
	TStFontInfo					font_info;
	
	// get a pointer to the text console data
	tcd = (ONtTextConsoleData*)WMrDialog_GetUserData(inDialog);
	
	WMrWindow_GetFontInfo(inDrawItem->window , &font_info);
	ONiFIStack_Init(
		font_info.font_family,
		font_info.font_size,
		font_info.font_style,
		font_info.font_shade);
	DCrText_SetFormat(TSc_VTop | TSc_HLeft);

	ONiIGUI_Page_Paint(tcd->text_console->console_data->pages[tcd->page], inDrawItem);
}

// ----------------------------------------------------------------------
static void
ONiTC_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	WMtWindow					*inControl)
{
	ONtTextConsoleData			*tcd;
	UUtUns16					command_type;
	UUtUns16					control_id;
	
	// get a pointer to the text console data
	tcd = (ONtTextConsoleData*)WMrDialog_GetUserData(inDialog);

	command_type = UUmHighWord(inParam1);
	control_id = UUmLowWord(inParam1);
	
	switch (control_id)
	{
		case ONcTC_Btn_Next:
			tcd->page++;
			if (tcd->page >= ((UUtInt32)tcd->text_console->console_data->num_pages))
			{
				tcd->page = (((UUtInt32)tcd->text_console->console_data->num_pages) - 1);
				WMrDialog_ModalEnd(inDialog, 0);
			}
		break;
		
		case ONcTC_Btn_Close:
			WMrDialog_ModalEnd(inDialog, 0);
		break;
	}
	
	ONiTC_SetButtons(inDialog, tcd);
}

// ----------------------------------------------------------------------
static void
ONiTC_HandleKeyDown(
	WMtDialog					*inDialog,
	UUtUns32					inKey)
{
	WMtWindow					*control;
	
	switch (inKey)
	{
		case LIcKeyCode_Escape:
			control = WMrDialog_GetItemByID(inDialog, ONcTC_Btn_Close);
		break;
		
		case LIcKeyCode_Space:
			control = WMrDialog_GetItemByID(inDialog, ONcTC_Btn_Next);
		break;
	}
	
	// simulate a button click
	WMrMessage_Send(
		inDialog,
		WMcMessage_Command,
		UUmMakeLong(WMcNotify_Click, WMrWindow_GetID(control)),
		(UUtUns32)control);
}

// ----------------------------------------------------------------------
static UUtBool
ONiTextConsole_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			ONiTC_InitDialog(inDialog);
		break;
		
		case WMcMessage_Command:
			ONiTC_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
			if (UUmLowWord(inParam1) == ONcTC_Btn_Close)
			{
				WMrDialog_ModalEnd(inDialog, 0);
			}
		break;
		
		case WMcMessage_KeyDown:
			ONiTC_HandleKeyDown(inDialog, inParam1);
		break;
		
		case WMcMessage_DrawItem:
			if (inParam1 != ONcTC_Txt_Text) { break; }
			ONiTC_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;

		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ----------------------------------------------------------------------
void
ONrInGameUI_TextConsole_Display(
	ONtTextConsole				*inTextConsole)
{
	ONtTextConsoleData			tcd;
	PStPartSpecUI				*partspec_ui;
	PStPartSpecUI				*temp_ui;
	LItMode						mode;
	
	if (inTextConsole == NULL) { return; }
	
	// save the current ui
	partspec_ui = PSrPartSpecUI_GetActive();
	
	// set the ui to the out of game ui
	temp_ui = PSrPartSpecUI_GetByName(ONcInGameUIName);
	if (temp_ui != NULL) { PSrPartSpecUI_SetActive(temp_ui); }

	mode = LIrMode_Get();
	LIrMode_Set(LIcMode_Normal);
	WMrActivate();
	
	tcd.text_console = inTextConsole;
	tcd.page = 0;
	
	WMrDialog_ModalBegin(
		ONcTextConsoleID,
		NULL,
		ONiTextConsole_Callback,
		(UUtUns32)&tcd,
		NULL);
	
	WMrDeactivate();
	LIrMode_Set(mode);

	// reset the active ui
	PSrPartSpecUI_SetActive(partspec_ui);
}

// ----------------------------------------------------------------------
static UUtError
ONiTextConsole_Display(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtTextConsole			*text_console;
	
	if (inParameterListLength != 1) { return UUcError_Generic; }
	if (inParameterList == NULL) { return UUcError_Generic; }
	
	text_console =
		(ONtTextConsole*)TMrInstance_GetFromName(
			ONcTemplate_TextConsole,
			inParameterList[0].val.str);
	if (text_console == NULL) { return UUcError_Generic; }
	
	ONrInGameUI_TextConsole_Display(text_console);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiInGameUI_SetHelpDisplayEnable(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	// CB: this is a debugging command until the pause-screen button is hooked up properly
	ONrInGameUI_EnableHelpDisplay(inParameterList[0].val.i > 0);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiInGameUI_ShowElement(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	if (UUmString_IsEqual(inParameterList[0].val.str, "left")) {
		gLeftEnable = (inParameterList[1].val.i > 0);

	} else if (UUmString_IsEqual(inParameterList[0].val.str, "right")) {
		gRightEnable = (inParameterList[1].val.i > 0);

	} else {
		COrConsole_Printf("ui_show_element: unknown element, valid element names are:");
		COrConsole_Printf("  left");
		COrConsole_Printf("  right");
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
ONiInGameUI_GetElementMask(
	const char *			inName)
{
	ONtIGUIElementDescriptionEntry *element;
	UUtUns32 mask;

	if (UUmString_IsEqual(inName, "all")) {
		mask = ONcIGUIElement_All;
	} else {
		mask = 0;
		for (element = gElementDescription; element->name != NULL; element++) {
			if (UUmString_IsEqual(element->name, inName)) {
				mask = (1 << element->element);
				break;
			}
		}
	}

	if (mask == 0) {
		COrConsole_Printf("No such in-game UI element '%s' - possible elements are:", inName);
		for (element = gElementDescription; element->name != NULL; element++) {
			COrConsole_Printf("  %s", element->name);
		}
	}

	return mask;
}

// ----------------------------------------------------------------------
static UUtError
ONiInGameUI_FillElement(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32 mask = ONiInGameUI_GetElementMask(inParameterList[0].val.str);

	if (inParameterList[1].val.i > 0) {
		gElementFilled |= mask;
	} else {
		gElementFilled &= ~mask;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiInGameUI_FlashElement(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32 mask = ONiInGameUI_GetElementMask(inParameterList[0].val.str);

	if (inParameterList[1].val.i > 0) {
		gElementFlashing |= mask;
	} else {
		gElementFlashing &= ~mask;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrInGameUI_EnableHelpDisplay(
	UUtBool inEnable)
{
	UUtError error;
	TStFontFamily *font_family;

	if (ONgInGameUI_EnableHelp == inEnable)
		return;

	ONgInGameUI_EnableHelp = inEnable;
	if (ONgInGameUI_EnableHelp) {
		// FIXME: disable message area?

		// set up the text context for the help display
		error = TSrFontFamily_Get(TScFontFamily_Default, &font_family);
		if (error == UUcError_None) {
			TSrContext_New(font_family, cHelpTextFontSize, TScStyle_Plain, TSc_HLeft, UUcFalse, &gHUDHelpTextContext);
		}
	} else {
		// FIXME: re-enable message area?
	}
}

// ----------------------------------------------------------------------
void
ONrInGameUI_NewWeapon(
	WPtWeaponClass *inWeaponClass)
{
	ONgInGameUI_NewWeaponClass = inWeaponClass;

	// remove any pre-existing weapon prompt messages
	COrMessage_Remove("new_weapon");
	if (inWeaponClass != NULL) {
		const char *classname = TMrInstance_GetInstanceName(inWeaponClass);
		const char *message_text;

		message_text = SSrMessage_Find(classname);
		if (message_text == NULL) {
			COrConsole_Printf("### unable to find new-weapon message for '%s'", classname);
		} else {
			COrMessage_Print(message_text, "new_weapon", ONcNewWeaponMessage_FadeTime);
		}
	}
}

// ----------------------------------------------------------------------
void
ONrInGameUI_NewItem(
	WPtPowerupType inPowerupType)
{
	if (ONgInGameUI_NewItem != WPcPowerup_LSI) {
		ONgInGameUI_NewItem = inPowerupType;
	}
}

// ----------------------------------------------------------------------
void
ONrInGameUI_NotifyRestoreGame(
	UUtUns32 inGameTime)
{
	ONgInGameUI_LastRestoreGame = inGameTime;

	// if we have any objectives, stop them from prompting
	ONgInGameUI_NewObjective = UUcFalse;
	COrMessage_Remove(ONcNewObjectiveMessage);

	// we don't want to know about new moves learnt, we have loaded from a saved game
	ONgInGameUI_NewMove = UUcFalse;
}
