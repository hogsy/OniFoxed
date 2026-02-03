/*
	FILE:	Oni_GameState.c

	AUTHOR:	Brent H. Pease, Michael Evans

	CREATED: May 31, 1997

	PURPOSE:

	Copyright 1997-2000

*/

//#include <math.h>
#include "bfw_math.h"

#include "Oni_GameStatePrivate.h"
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_Totoro.h"
#include "BFW_LocalInput.h"
#include "BFW_Console.h"
#include "BFW_TemplateManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_LocalInput.h"
#include "BFW_Object.h"
#include "BFW_MathLib.h"
#include "BFW_AppUtilities.h"
#include "BFW_FileFormat.h"
#include "BFW_Particle3.h"
#include "BFW_Platform.h"
#include "BFW_TextSystem.h"
#include "BFW_Doors.h"
#include "BFW_Effect.h"
#include "BFW_WindowManager.h"
#include "BFW_ScriptLang.h"
#include "BFW_Timer.h"
#include "BFW_SoundSystem2.h"

#include "Oni_Camera.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Character.h"
#include "Oni_Camera.h"
#include "Oni_Aiming.h"
#include "Oni_Motoko.h"
#include "Oni_AI_Setup.h"
#include "Oni_AI.h"
#include "Oni_AI_Script.h"
#include "Oni_Combos.h"
#include "Oni_Film.h"
#include "Oni.h"
#include "Oni_Level.h"
#include "Oni_Windows.h"
#include "Oni_Win_AI.h"
#include "Oni_Object.h"
#include "Oni_Path.h"
#include "Oni_Character_Animation.h"
#include "Oni_KeyBindings.h"
#include "Oni_Object.h"
#include "Oni_Cinematics.h"
#include "Oni_BinaryData.h"
#include "Oni_Particle3.h"
#include "Oni_Sound2.h"
#include "Oni_Mechanics.h"
#include "Oni_Persistance.h"
#include "Oni_InGameUI.h"
#include "Oni_FX.h"

#if 0
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <string.h>
#endif

//#include "Akira_Private.h"

UUtInt32 ONgLineSize = 0;
M3tPoint3D *ONgLine = NULL;


#define ONcNoTurningLimitation					M3c2Pi
#define ONcGameState_AutoPromptFrames			12
#define ONcGameState_ConditionSound_MinVolume	0.4f

#define BRENTS_CHEESY_GSD_PERF 1
#define DEBUG_ANIM 0

M3tGeomCamera *ONgVisibilityCamera;
M3tGeomCamera *ONgActiveCamera;

UUtUns32 ONgCurrentCamera = 0;
static M3tGeomCamera *ONgInternalCamera[2];

static UUtInt32 ONgAnimBuffer = 8;
static	M3tDrawContext_Counters	ONgPerformanceData;
static UUtBool ONgWaitForKey = UUcFalse;
static UUtBool ONgRecoilEdit = UUcFalse;
static WPtRecoil ONgRecoil;
static UUtBool ONgSingleStep = UUcFalse;
#define ONcLetterBox_Depth 75
#define ONcLetterBox_Increment 1.25f


UUtBool	ONgShow_Environment = UUcTrue;
UUtBool	ONgShow_Characters = UUcTrue;
UUtBool	ONgShow_Objects = UUcTrue;
UUtBool ONgShow_Sky = UUcTrue;
UUtBool	ONgShow_UI = UUcTrue;

static UUtBool ONgSyncDebug = UUcFalse;
static UUtBool ONgDebugHandle = UUcFalse;
static UUtBool ONgDrawEveryFrame = UUcFalse;
static UUtBool ONgFastMode = UUcFalse;
static UUtInt32 ONgDrawEveryFrameMultiple = 1;
static UUtBool ONgChangeCharacters = UUcFalse;

#if TOOL_VERSION
COtStatusLine		perf_gsd[4];
COtStatusLine		perf_gsu[4];
COtStatusLine		occl_display[1];
COtStatusLine		soundoccl_display[1];
COtStatusLine		chr_col_display[1];
COtStatusLine		obj_col_display[1];
COtStatusLine		invis_display[1];
COtStatusLine		frame_display[1];
COtStatusLine		quad_count_display[2];
COtStatusLine		object_count_display[2];
COtStatusLine		physics_count_display[2];
COtStatusLine		script_count_display[2];
#endif

#if PERFORMANCE_TIMER
COtStatusLine		perf_overall[14];
#else
COtStatusLine		game_fps[1];
COtStatusLine		texture_info[1];
#endif

TStTextContext*		ONgTimerTextContext = NULL;

UUtBool	ONgShowPerformance_Overall = UUcFalse;
UUtBool	ONgShowPerformance_GSD = UUcFalse;
UUtBool	ONgShowPerformance_GSU = UUcFalse;
UUtBool	ONgShowQuadCount = UUcFalse;
UUtBool	ONgShowObjectCount = UUcFalse;
UUtBool	ONgShowPhysicsCount = UUcFalse;
UUtBool ONgShowScriptingCount = UUcFalse;
UUtBool	ONgStableEar = UUcTrue;
UUtBool ONgFastLookups = UUcTrue;


BFtFileRef ONgGameDataFolder;
ONtGameState *ONgGameState = NULL;
ONtLevel *ONgLevel = NULL;
UUtBool					ONgTerminateGame;
UUtBool					ONgRunMainMenu;
M3tGeomContext			*ONgGeomContext = NULL;
ONtPlatformData	ONgPlatformData;

extern UUtBool		ONgTerminateGame;
extern UUtBool		OBgObjectsActive;
extern UUtBool		OBgObjectsFriction;
extern UUtBool		OBgObjectsShowDebug;
extern UUtBool		AIgVerbose;
static UUtInt32		ONgContinueSavePoint = 0;
UUtBool ONgDebugConsoleAction = UUcFalse;

float	ONgGameState_InputAccel = 5.0f;

#if defined(DEBUGGING) && DEBUGGING
UUtBool ONgSleep				= UUcTrue;
#else
UUtBool ONgSleep				= UUcFalse;
#endif

#if SHIPPING_VERSION
UUtBool ONgDeveloperAccess = UUcFalse;
#else
UUtBool ONgDeveloperAccess = UUcTrue;
#endif

UUtBool ONgPlayerFistsOfFury = UUcFalse;
UUtBool ONgPlayerKnockdownResistant = UUcFalse;
UUtBool	ONgPlayerUnstoppable = UUcFalse;
UUtBool	ONgPlayerInvincible = UUcFalse;
UUtBool	ONgPlayerOmnipotent = UUcFalse;

UUtBool ONgParticle_Display = UUcTrue;
UUtBool ONgFontCache_Display = UUcFalse;

M3tTextureMap*  ONgLetterboxTexture = NULL;

UUtInt32 ONgScreenShotReduceAmount=0;

// debugging only
extern UUtUns32 AKgHighlightGQIndex;

#define	kStatsPlayerWidth	128
#define	kStatsKillsWidth	48
#define	kStatsDmgDoneWidth	64

#define ONcMaxPhysics 60

typedef enum {
	ONcAttackMove,
	ONcNormalMove
} ONtAttackMove;

static UUtError ONrGameState_UpdateSoundManager(ONtGameState *ioGameState, UUtUns32 deltaTicks);

static const TRtAnimation *DoTableLookup(const ONtInputState *inInput, ONtCharacter *ioCharacter,
										 ONtActiveCharacter *ioActiveCharacter, const ONtMoveLookup *table, TRtAnimType *outAnimType);
static UUtBool DoTableAnimation(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter,
								const ONtMoveLookup *table, ONtAnimPriority priority, ONtAttackMove attackMove, const TRtAnimation **outAnimation);
static void ONrGameState_MotionBlur_Display(void);
static void ONrGameState_UpdateAutoPrompt(void);
LItButtonBits ONiRemapKeys(LItButtonBits currentDown);

static void
ONiGameState_UpdateListener(
	ONtGameState				*ioGameState);

UUtUns32 last_escape_key_time = 0;

static void NoEscapeKeyForABit(void)
{
	last_escape_key_time = ONgGameState->gameTime + 15;
}

static UUtBool CanEscapeKey(void)
{
	UUtBool can_escape_key = (ONgGameState->gameTime > last_escape_key_time);

	return can_escape_key;
}

static UUtBool UserInputCleared(void)
{
	UUtBool zero_user_input;

	zero_user_input = !ONgGameState->inputEnable;
	zero_user_input = zero_user_input || ONgGameState->inside_console_action;

	// zero the user input if the player exists and has the ai movement flag
	if (ONgGameState->local.playerCharacter) {
		if (ONgGameState->local.playerCharacter->flags & ONcCharacterFlag_AIMovement) {
			zero_user_input = UUcTrue;
		}
	}

	return zero_user_input;
}

static void ONrBlanketScreen(float inA, float inR, float inG, float inB);

static float get_cosmetic_ramp(float inRamp)
{
	float result = (float) pow(inRamp , 1.9f);

	return result;
}


void
ONrGameState_Performance_UpdateOverall(
	const char *l1,
	const char *l2,
	const char *l3)
{
#if PERFORMANCE_TIMER
	UUrString_Copy(perf_overall[1].text, l1, COcMaxStatusLine);
	UUrString_Copy(perf_overall[2].text, l2, COcMaxStatusLine);
	UUrString_Copy(perf_overall[3].text, l3, COcMaxStatusLine);
#else
	UUrString_Copy(game_fps[0].text, l1, COcMaxStatusLine);
	UUrString_Copy(texture_info[0].text, l2, COcMaxStatusLine);
#endif
}

static void
ONrGameState_Performance_UpdateGSD(
	const char *l1,
	const char *l2,
	const char *l3)
{
#if TOOL_VERSION
	UUrString_Copy(perf_gsd[1].text, l1, COcMaxStatusLine);
	UUrString_Copy(perf_gsd[2].text, l2, COcMaxStatusLine);
	UUrString_Copy(perf_gsd[3].text, l3, COcMaxStatusLine);
#endif
}

static void
ONrGameState_Performance_UpdateGSU(
	const char *l1,
	const char *l2,
	const char *l3)
{
#if TOOL_VERSION
	UUrString_Copy(perf_gsu[1].text, l1, COcMaxStatusLine);
	UUrString_Copy(perf_gsu[2].text, l2, COcMaxStatusLine);
	UUrString_Copy(perf_gsu[3].text, l3, COcMaxStatusLine);
#endif
}


// ======================================================================

static UUtError
ONiGameState_Performance_Install(void)
{
	static UUtBool once = UUcTrue;

	ONrGameState_Performance_UpdateOverall("", "", "");

	return UUcError_None;
}

static UUtError
ONrTimerPrefix(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	char *new_prefix = NULL;

	if (0 == inParameterListLength) {
		new_prefix = "";

		COrConsole_Printf("prefix cleared");
	}
	else if (1 == inParameterListLength) {
		if (SLcType_String == inParameterList[0].type) {
			new_prefix = inParameterList[0].val.str;

			COrConsole_Printf("prefix = %s", new_prefix);
		}
	}

#if PERFORMANCE_TIMER
	if (NULL != new_prefix) {
		UUrPerformanceTimer_SetPrefix(new_prefix);
	}
#endif

	return UUcError_None;
}

static UUtError
ONrGameState_FarClipPlane_Set(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONgMotoko_FarPlane = inParameterList[0].val.f;

	M3rCamera_SetStaticData(
		ONgActiveCamera,
		ONgMotoko_FieldOfView,
		ONcMotoko_AspectRatio,
		ONcMotoko_NearPlane,
		ONgMotoko_FarPlane);

	return UUcError_None;
}

static UUtError
ONrGameState_FieldOfView_Set(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	float new_fov;

	new_fov = inParameterList[0].val.f;

	ONgMotoko_FieldOfView = new_fov * (M3c2Pi / 360.f);

	M3rCamera_SetStaticData(
		ONgActiveCamera,
		ONgMotoko_FieldOfView,
		ONcMotoko_AspectRatio,
		ONcMotoko_NearPlane,
		ONgMotoko_FarPlane);

	return UUcError_None;
}

UUtError ONrGameState_SetupDefaultLighting(void)
{
	M3tLight_Directional	directionalLightList[2];
	M3tLight_Ambient		ambientLight[1];
	float					flat_lighting_amount;
	UUtInt32				num_directional_lights = ONrMotoko_GraphicsQuality_NumDirectionalLights();

	num_directional_lights = UUmPin(num_directional_lights, 0, 2);

	if (num_directional_lights >= 2) {
		flat_lighting_amount = 0.55f;
	}
	else if (num_directional_lights >= 1) {
		flat_lighting_amount = 0.65f;
	}
	else {
		flat_lighting_amount = 0.85f;
	}

	ambientLight[0].color.r = flat_lighting_amount;
	ambientLight[0].color.g = flat_lighting_amount;
	ambientLight[0].color.b = flat_lighting_amount;

	directionalLightList[0].direction.x = -0.5f;
	directionalLightList[0].direction.y = -1.0f;
	directionalLightList[0].direction.z = -0.5f;

	directionalLightList[0].color.r = 0.7f;
	directionalLightList[0].color.g = 0.7f;
	directionalLightList[0].color.b = 0.7f;

	directionalLightList[1].direction.x = 0.5f;
	directionalLightList[1].direction.y = 0.5f;
	directionalLightList[1].direction.z = 1.0f;

	directionalLightList[1].color.r = 0.3f;
	directionalLightList[1].color.g = 0.3f;
	directionalLightList[1].color.b = 0.3f;

	// data is copied into motoko
	M3rLightList_Ambient(ambientLight);
	M3rLightList_Directional(num_directional_lights, directionalLightList);

	return UUcError_None;
}


static UUtError ONrGameState_CreateAndSetupCamera(void)
{
	UUtError error;
	M3tPoint3D		cameraLoc;
	M3tVector3D		viewDir;
	M3tVector3D		upDir;
	UUtUns32		itr = 0;

	ONgCurrentCamera = 0;

	for(itr = 0; itr < 2; itr++)
	{
		error = M3rCamera_New(&ONgInternalCamera[itr]);
		UUmError_ReturnOnError(error);

		ONgMotoko_FarPlane = 10000.0f;

		M3rCamera_SetStaticData(
			ONgInternalCamera[itr],
			ONgMotoko_FieldOfView,
			ONcMotoko_AspectRatio,
			ONcMotoko_NearPlane,
			ONgMotoko_FarPlane);

		cameraLoc.x = cameraLoc.y = cameraLoc.z = 0.0f;
		viewDir.x = 1.0f;
		viewDir.y = 0.0f;
		viewDir.z = 0.0f;
		upDir.x = 0.0f;
		upDir.y = 1.0f;
		upDir.z = 0.0f;

		M3rCamera_SetViewData(
			ONgInternalCamera[itr],
			&cameraLoc,
			&viewDir,
			&upDir);

	}

	ONgVisibilityCamera = ONgInternalCamera[0];
	ONgActiveCamera = ONgInternalCamera[0];

	M3rCamera_SetActive(ONgActiveCamera);

	return UUcError_None;
}

UUtError ONrGameState_PrepareGeometryEngine(void)
{
	UUtError error;

	error = ONrGameState_CreateAndSetupCamera();
	UUmError_ReturnOnErrorMsg(error, "could not create and setup camera");

	error = ONrGameState_SetupDefaultLighting();
	UUmError_ReturnOnErrorMsg(error, "could not setup default lighting");

	return UUcError_None;
}


static void ONrGameState_DisposeCamera(void)
{
	UUtUns32 itr;

	for(itr = 0; itr < 2; itr++) {
		if (NULL != ONgInternalCamera[itr]) {
			M3rCamera_Delete(ONgInternalCamera[itr]);
			ONgInternalCamera[itr] = NULL;
		}
	}

	return;
}

void ONrGameState_TearDownGeometryEngine(void)
{
	ONrGameState_DisposeCamera();
}

static UUtBool KeyWentDown(void)
{
	UUtBool key_went_down = UUcFalse;
	UUtUns32 itr;
	UUtUns32 count = 50;

	for(itr = 0; itr < count; itr++)
	{
		LItInputEvent event;

		if (LIrInputEvent_Get(&event)) {
			switch(event.type)
			{
				case LIcInputEvent_LMouseDown:
				case LIcInputEvent_MMouseDown:
				case LIcInputEvent_RMouseDown:
				case LIcInputEvent_KeyDown:
					key_went_down = UUcTrue;
					goto exit;
				break;
			}
		}
	}

exit:
	return key_went_down;
}

static void WaitForKey(UUtUns32 inTimeOut)
{
	UUtUns32 end_time = UUrMachineTime_Sixtieths() + inTimeOut;

	while(1)
	{
		UUtUns32 current_time = UUrMachineTime_Sixtieths();

		if (KeyWentDown()) {
			break;
		}

		if (current_time > end_time) {
			break;
		}
	}

	if (ONgGameState->local.in_cutscene) {
		ONgGameState->local.time_cutscene_started_or_wait_for_key_returned_in_machine_time = UUrMachineTime_Sixtieths();
	}

	return;
}

// Global app specific intializations need to happen in ONrGameState_Initialize
// Keep in mind this will get called from the game shell by the "New Game" button

static void splash_screen_internal(M3tTextureMap_Big *splash_screen, float inBlanketAmount)
{
	M3rGeom_Frame_Start(0);

	M3rGeom_State_Push();

	M3rGeom_State_Set(M3cGeomStateIntType_SubmitMode, M3cGeomState_SubmitMode_Normal);
	M3rGeom_State_Set(M3cGeomStateIntType_Shade, M3cGeomState_Shade_Vertex);
	M3rGeom_State_Set(M3cGeomStateIntType_Fill, M3cGeomState_Fill_Solid);
	M3rGeom_State_Set(M3cGeomStateIntType_Appearance, M3cGeomState_Appearance_Texture);

	M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare, M3cDrawState_ZCompare_Off);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Fog, M3cDrawStateFogDisable);
	M3rGeom_State_Commit();

	if (NULL != splash_screen) {
		UUtUns16 x_location = (M3rDraw_GetWidth() - splash_screen->width) / 2;
		UUtUns16 y_location = (M3rDraw_GetHeight() - splash_screen->height) / 2;
		M3tPointScreen point_screen;

		point_screen.x = x_location;
		point_screen.y = y_location;
		point_screen.z = 0.5f;
		point_screen.invW = 2.f;

		M3rDraw_BigBitmap(splash_screen, &point_screen, splash_screen->width, splash_screen->height, IMcShade_White, M3cMaxAlpha);
	}

	if (inBlanketAmount > 0) {
		ONrBlanketScreen(inBlanketAmount, 0.f, 0.f, 0.f);
	}

	M3rGeom_State_Pop();

	M3rGeom_Frame_End();
}

void
ONrGameState_SplashScreen(
	char *inInstanceName,
	const char *inMusicName,
	UUtBool inStopPlayingSounds)
{
	M3tTextureMap_Big *splash_screen;
	LItMode old_mode = LIrMode_Get();
	UUtError error;

	UUtUns32 fade_in_length = 45;
	UUtUns32 timeout_length = 10 * 60;
	UUtUns32 fade_length = 2 * 60;
	UUtUns32 start_time;
	UUtUns32 end_time;

	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap_Big, inInstanceName, &splash_screen);
	splash_screen = (UUcError_None != error) ? NULL : splash_screen;

	if (NULL == splash_screen) {
		goto exit;
	}

	// stop all currently playing sounds
	if (inStopPlayingSounds)
	{
		OSrSetScriptOnly(UUcTrue);
		SSrStopAll();
	}

	// start the music
	OSrMusic_Start(inMusicName, 0.0f);
	OSrMusic_SetVolume(1.0f, 1.0f);

	// fade in from black
	start_time = UUrMachineTime_Sixtieths();
	end_time = start_time + timeout_length;

	LIrMode_Set(LIcMode_Normal);

	KeyWentDown();	// flush keys

	while(1)
	{
		UUtUns32 current_time = UUrMachineTime_Sixtieths();
		float blanket_amount;

		if (KeyWentDown()) {
			goto fade_out;
		}

		if (current_time >= end_time) {
			break;
		}

		blanket_amount = ((float) (current_time - start_time)) / ((float) fade_length);
		blanket_amount = get_cosmetic_ramp(1.f - blanket_amount);

		splash_screen_internal(splash_screen, blanket_amount);

		// update the sound
		SS2rUpdate();
		ONrGameState_UpdateSoundManager(ONgGameState, 0);
	}


	// wait for a keypress
	start_time = UUrMachineTime_Sixtieths();
	end_time = start_time + timeout_length;

	while(1)
	{
		UUtUns32 current_time = UUrMachineTime_Sixtieths();

		if (current_time >= end_time) {
			break;
		}

		if (KeyWentDown()) {
			break;
		}

		splash_screen_internal(splash_screen, 0.f);

		// update the sound
		SS2rUpdate();
		ONrGameState_UpdateSoundManager(ONgGameState, 0);
	}

fade_out:
	// stop the music
	OSrMusic_SetVolume(0.0f, 1.0f);

	// fade out (can skip with a second keypress)
	start_time = UUrMachineTime_Sixtieths();
	end_time = start_time + fade_length;

	while(1)
	{
		UUtUns32 hasty_time = 30;
		UUtUns32 current_time = UUrMachineTime_Sixtieths();
		float blanket_amount = 0.f;

		// update the music
		SS2rUpdate();

		if (current_time >= end_time) {
			break;
		}

		if (current_time > (start_time + hasty_time)) {
			if (KeyWentDown()) {
				break;
			}
		}
		else {
			KeyWentDown(); // eat the input
		}

		blanket_amount = ((float) (current_time - start_time)) / ((float) fade_length);
		blanket_amount = get_cosmetic_ramp(blanket_amount);

		splash_screen_internal(splash_screen, blanket_amount);

		// update the sound
		SS2rUpdate();
		ONrGameState_UpdateSoundManager(ONgGameState, 0);
	}

	OSrMusic_Halt();

	if (ONgGameState->local.in_cutscene) {
		ONgGameState->local.time_cutscene_started_or_wait_for_key_returned_in_machine_time = UUrMachineTime_Sixtieths();
	}

	if (NULL != splash_screen) {
		M3rTextureMap_Big_Unload(splash_screen);
	}

	LIrMode_Set(old_mode);

exit:
	// always leave the screen as black, otherwise the final
	// movie will be in the middle of a screen that looks wrong
	splash_screen_internal(NULL, 1.f);

	OSrSetScriptOnly(UUcFalse);

	return;
}


UUtError
ONrGameState_LevelBegin(UUtUns16 inLevelNumber)
{
	UUtError		error;

	last_escape_key_time = 0;

	{
		ONtPlace place;

		place.level = inLevelNumber;
		place.save_point = 0;

		ONrPersist_SetPlace(&place);
	}

	ONrUnlockLevel(inLevelNumber);

	UUrMemory_Clear(ONgGameState, sizeof(*ONgGameState));

	ONgGameState->key_mask = 0xFFFFFFFFFFFFFFFF;

	if (ONgWaitForKey) {
		M3rGeom_Frame_Start(0);
		M3rGeom_Frame_End();
		WaitForKey(60 * 15);
	}

	ONgGameState->inputEnable = UUcTrue;
	ONgGameState->inside_console_action = UUcFalse;

	ONgGameState->levelNumber = inLevelNumber;

	// clear old stuff
	ONgGameState->local.prevWeapon1 = ONgGameState->local.prevWeapon2 = (WPtWeaponClass *)UUcMaxUns32;

	// clear the screen recording mode
	ONgGameState->local.recordScreen = UUcFalse;
	ONgGameState->local.cameraRecord = UUcFalse;
	ONgGameState->local.cameraPlay = UUcFalse;
	ONgGameState->local.cameraRecording = NULL;
	ONgGameState->local.cameraActive = UUcFalse;

	// Set up opacity
	ONgGameState->displayOpacity = UUcTrue;
	ONgGameState->geometryOpacity = NULL;

	// setup time
	ONgGameState->server.machineTimeLast = UUrMachineTime_Sixtieths();
	ONgGameState->gameTime = 0;
	ONgGameState->serverTime = 0;

	ONgGameState->local.slowMotionEnable = UUcFalse;
	ONgGameState->local.slowMotionTimer = 0;
	ONgGameState->local.currentPromptMessage = NULL;

	// load the level & set the environment
	error = TMrInstance_GetDataPtr_ByNumber(ONcTemplate_Level, 0, &ONgLevel);
	UUmError_ReturnOnErrorMsg(error, "could not load level");
	ONgGameState->level = ONgLevel;
	AKgEnvironment = ONgGameState->level->environment;

	// Allocate physics contexts
	error = PHrPhysicsContext_New(ONcMaxPhysics);
	UUmError_ReturnOnError(error);

	PHrPhysicsContext_Clear();

/*	error = ONrGameState_CreateAndSetupUI();
	UUmError_ReturnOnErrorMsg(error, "could not create and setup UI");	*/

	error = ONrCharacter_LevelBegin();
	UUmError_ReturnOnErrorMsg(error, "ONrCharacter_LevelBegin failed");

	ONiGameState_Performance_Install();

	// make sure that we have allocated the white texture for the letterbox
	if (ONgLetterboxTexture == NULL) {
		IMtPixelType pixelType;

		error = M3rDrawEngine_FindGrayscalePixelType(&pixelType);
		if (error == UUcError_None) {
			error = M3rTextureMap_New(4, 4, pixelType, M3cTexture_AllocMemory,
								M3cTextureFlags_None, "white texture", &ONgLetterboxTexture);

			if (error == UUcError_None) {
				M3rTextureMap_Fill_Shade(ONgLetterboxTexture, IMcShade_White, M3cFillEntireMap);
			}
		}

		if (error != UUcError_None) {
			UUmAssert(!"could not create letterbox texture");
		}
	}

	ONgGameState->local.fadeInfo.active = UUcFalse;

	// FX
	error =
		M3rGeomContext_SetEnvironment(
			ONgGameState->level->environment);
	UUmError_ReturnOnErrorMsg(error, "could not set environment");

	COrConsole_StatusLine_LevelBegin();
	ONrGameState_Timer_LevelBegin();

	WMrDialog_SetDrawCallback(ONrGameState_Display);

	FXrEffects_LevelBegin();

	// no condition sounds currently playing
	ONgGameState->condition_active = 0;
	UUrMemory_Set32(ONgGameState->condition_playid, SScInvalidID, sizeof(ONgGameState->condition_playid));

	CArEnableJello(UUcTrue);

	return UUcError_None;
}

void ONrGameState_LevelEnd(void)
{
	UUtUns32 itr;

	WMrDialog_SetDrawCallback(NULL);

	M3rDraw_State_SetPtr(M3cDrawStatePtrType_ScreenPointArray, NULL);
	M3rDraw_State_SetPtr(M3cDrawStatePtrType_ScreenShadeArray_DC, NULL);
	M3rDraw_State_SetPtr(M3cDrawStatePtrType_TextureCoordArray, NULL);
	M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, NULL);
	M3rDraw_State_SetPtr(M3cDrawStatePtrType_LightTextureMap, NULL);

	//ONrGameState_TearDownGeometryEngine();
//	ONrGameState_DisposeUI();

	// stop current condition sounds
	for (itr = 0; itr < ONcConditionSound_Max; itr++) {
		if (ONgGameState->condition_playid[itr] != SScInvalidID) {
			SSrAmbient_Stop(ONgGameState->condition_playid[itr]);
			ONgGameState->condition_playid[itr] = SScInvalidID;
		}
	}
	ONgGameState->condition_active = 0;

	M3rGeomContext_SetEnvironment(NULL);

	ONrCharacter_LevelEnd();

	COrConsole_StatusLine_LevelEnd();
	ONrGameState_Timer_LevelEnd();

	OCrCinematic_DeleteAll();

	// Free physics contexts
	PHrPhysicsContext_Delete();

	ONgGameState->level = NULL;

	return;
}

static void iWriteScreenToFile(BFtFileRef *inFileRef, UUtUns32 inScaleDown)
{
	const UUtUns16 width = M3rDraw_GetWidth();
	const UUtUns16 height = M3rDraw_GetHeight();
	const UUtUns16 scaled_width = width >> inScaleDown;
	const UUtUns16 scaled_height = height >> inScaleDown;
	UUtError error;
	UUtRect  rect;
	UUtUns8 *src_buffer;
	UUtUns32 *dst_buffer;
	UUtUns32 *dst_buffer_scaled;

	src_buffer = UUrMemory_Block_New(width * height * 3);
	dst_buffer = UUrMemory_Block_New(width * height * 4);
	dst_buffer_scaled = UUrMemory_Block_New(scaled_width * scaled_height * 4);

	rect.left = 0;
	rect.top = 0;
	rect.right = width - 1;
	rect.bottom = height - 1;

	if ((src_buffer != NULL) && (dst_buffer != NULL)) {
		UUtUns32 y,x;
		UUtUns8 *cur_src_buffer = src_buffer;

		error = M3rDraw_ScreenCapture(&rect, src_buffer);
		UUmAssert(UUcError_None == error);

		for(y = 0; y < height; y++) {
			for(x = 0; x < width; x++) {
				UUtUns32 r = cur_src_buffer[0];
				UUtUns32 g = cur_src_buffer[1];
				UUtUns32 b = cur_src_buffer[2];
				UUtUns32 index = (height - y - 1) * width + x;

				dst_buffer[index] = (r << 16) | (g << 8) | (b << 0);

				cur_src_buffer += 3;
			}
		}

		IMrImage_Scale(IMcScaleMode_Box, width, height, IMcPixelType_RGB888, dst_buffer, scaled_width, scaled_height, dst_buffer_scaled);

		if (NULL != inFileRef) {
			BFrFile_Delete(inFileRef);

			FFrWrite_2D(inFileRef, FFcFormat_2D_BMP, IMcPixelType_RGB888, scaled_width, scaled_height, IMcNoMipMap, dst_buffer_scaled);
		}
	}

	if (src_buffer != NULL) {
		UUrMemory_Block_Delete(src_buffer);
		src_buffer = NULL;
	}

	if (dst_buffer != NULL) {
		UUrMemory_Block_Delete(dst_buffer);
		dst_buffer = NULL;
	}

	if (dst_buffer_scaled != NULL) {
		UUrMemory_Block_Delete(dst_buffer_scaled);
		dst_buffer_scaled = NULL;
	}

	return;
}

static void iScreenShot(UUtUns32 inScaleDown)
{
	static UUtInt32 number = 0;
	BFtFileRef *fileRef = NULL;

	fileRef = BFrFileRef_MakeUnique("screen_shot", ".bmp", &number, 99999);

	if (NULL != fileRef) {
		iWriteScreenToFile(fileRef, inScaleDown);
		BFrFileRef_Dispose(fileRef);
	}

	return;
}

static float iGetTurnPerFrame(const ONtInputState *inInput, ONtCharacter *ioCharacter)
{
	const float aiRunningTurn = 0.1f;
	const float medTurn = (M3cPi * 1.5f) / 60.f;
	const float slowTurn = (M3cPi * 1.5f) / 120.f;
	float turnAmt;

	if (ONrCharacter_IsAI(ioCharacter)) {
		turnAmt = aiRunningTurn;
	}
	else if (inInput->buttonIsDown & LIc_BitMask_Walk) {
		turnAmt = slowTurn;
	}
	else {
		turnAmt = medTurn;
	}

	return turnAmt;
}

static void HandleTurnInput(const ONtInputState *inInput, float mousex, float mousey, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{

	float scaled_mouse_x = mousex;
	float scaled_mouse_y = mousey;
	float turnAmt = 0;
	const WPtWeaponClass *weapon_class = (ioCharacter->inventory.weapons[0] != NULL) ? WPrGetClass(ioCharacter->inventory.weapons[0]) : NULL;
	UUtBool is_ai = ONrCharacter_IsAI(ioCharacter);

	if (ioCharacter->inventory.weapons[0] != NULL) {
		float scale_factor = weapon_class->aimingSpeed;
		scale_factor *= 0.5f;

		scaled_mouse_x *= scale_factor;
		scaled_mouse_y *= scale_factor;
	}

	if (inInput->buttonIsDown & LIc_BitMask_LookMode) {
		ioActiveCharacter->aimingLR -= scaled_mouse_x / 64.f;

		// mouse y is look
		ioActiveCharacter->aimingUD += scaled_mouse_y / 64.f;
	}
	else {
		turnAmt += -scaled_mouse_x / 64.f;

		// mouse y is look
		ioActiveCharacter->aimingUD += scaled_mouse_y / 64.f;
	}

	if (NULL == weapon_class) {
		ioCharacter->recoilSpeed = 0.f;
		ioCharacter->recoil = 0.f;
	}
	else {
		const WPtRecoil *recoil = WPrClass_GetRecoilInfo(weapon_class);


		// COrConsole_Printf("%f,%f base %f return %f factor %f max %f", ioCharacter->recoil, ioCharacter->recoilSpeed, weapon_class->recoil, weapon_class->recoilReturnSpeed, weapon_class->recoilFactor, weapon_class->recoilMax);

		if (0 != ioCharacter->recoilSpeed) {
			ioCharacter->recoil += ioCharacter->recoilSpeed;
			ioCharacter->recoilSpeed /= recoil->factor;

			if (ioCharacter->recoilSpeed < 0.001f) { ioCharacter->recoilSpeed = 0.f; }
		}
		else if (0 != ioCharacter->recoil) {
			if (WPrGetChamberTimeScale(ioCharacter->inventory.weapons[0]) == 0) {
				// the weapon is not firing
				ioCharacter->recoil -= recoil->returnSpeed;
			} else {
				ioCharacter->recoil -= recoil->firingReturnSpeed;
			}
		}

		if ((!is_ai) && (weapon_class->flags & WPcWeaponClassFlag_RecoilDirect)) {
			ioActiveCharacter->aimingUD += ioCharacter->recoil;
			ioCharacter->recoil = 0;
		}
		else {
			ioCharacter->recoil = UUmPin(ioCharacter->recoil, 0.f, recoil->max);
		}
	}


	ioActiveCharacter->aimingLR = UUmPin(ioActiveCharacter->aimingLR, -(M3c2Pi / 4.f), (M3c2Pi / 4.f));
	ioActiveCharacter->aimingUD = UUmPin(ioActiveCharacter->aimingUD, -(M3c2Pi / 4.f), (M3c2Pi / 4.f));


	// == handle turning if we are in an animation where turning is legal==
	if (!TRrAnimation_TestFlag(ioActiveCharacter->animation, ONcAnimFlag_NoTurn))
	{
		float btnTurnAmt = iGetTurnPerFrame(inInput, ioCharacter);

		if (inInput->buttonIsDown & LIc_BitMask_TurnLeft)
		{
			turnAmt += btnTurnAmt;
		}
		else if (inInput->buttonIsDown & LIc_BitMask_TurnRight)
		{
			turnAmt -= btnTurnAmt;
		}

		turnAmt = UUmPin(turnAmt, -M3cPi, +M3cPi);

		ONrCharacter_AdjustDesiredFacing(ioCharacter, turnAmt);
	}

	return;
}


static void HandleMouseInput(const ONtInputState *inInput, float mousex, float mousey, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	HandleTurnInput(inInput, mousex, mousey, ioCharacter, ioActiveCharacter);

	return;
}

static void HandleTurning(const ONtInputState *inInput, float mousex, float mousey, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	float turnPerFrame;
	float desireOffset;
	UUtBool turn;
	float turnAmt;
	UUtBool can_turn_left = ioActiveCharacter->pleaseTurnLeft;
	UUtBool can_turn_right = ioActiveCharacter->pleaseTurnRight;

	// AI2 system handles its own turning
	if (!ONrCharacter_IsPlayingFilm(ioCharacter) && (ioCharacter->flags & ONcCharacterFlag_AIMovement)) {
		return;
	}

	turnPerFrame = iGetTurnPerFrame(inInput, ioCharacter);
	desireOffset = ONrCharacter_GetDesiredFacingOffset(ioCharacter);

	if (desireOffset < -turnPerFrame) {
		// we are turning right
		ioActiveCharacter->pleaseTurnLeft = UUcFalse;
	}
	else if (desireOffset > turnPerFrame) {
		// we are turning left
		ioActiveCharacter->pleaseTurnRight = UUcFalse;
	}
	else {
		// we are either not turning or this is our last frame of turning
		ioActiveCharacter->pleaseTurnLeft = UUcFalse;
		ioActiveCharacter->pleaseTurnRight = UUcFalse;
	}

	switch(ioActiveCharacter->curAnimType)
	{
		case ONcAnimType_Crouch_Turn_Left:
		case ONcAnimType_Crouch_Turn_Right:
		case ONcAnimType_Standing_Turn_Left:
		case ONcAnimType_Standing_Turn_Right:
			// in a standing turn animation we can turn
			// but only if we are going the righ way
			turn = UUcTrue;
		break;

		default:
			// if we are not in a standing turn animation we can turn
			// either way if this is an animation where we can turn
			turn = !ONrCharacter_IsUnableToTurn(ioCharacter);
			can_turn_left = turn;
			can_turn_right = turn;
		break;
	}

	if (turn && (desireOffset > 0) && can_turn_left) {
		if (desireOffset > turnPerFrame) {
			turnAmt = turnPerFrame;
		}
		else {
			turnAmt = desireOffset;
		}
	}
	else if (turn && (desireOffset < 0) && can_turn_right) {
		if (desireOffset < -turnPerFrame) {
			turnAmt = -turnPerFrame;
		}
		else {
			turnAmt = desireOffset;
		}
	}
	else {
		turn = UUcFalse;
	}

	if (turn) {
		UUmAssertTrigRange(ioCharacter->facing);

		ioCharacter->facing += turnAmt;
		UUmTrig_Clip(ioCharacter->facing);

		UUmAssertTrigRange(ioCharacter->facing);
	}

	return;
}

static UUtBool HandleStartTaunt(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool animFound = UUcFalse;
	const ONtMoveLookup table[] = {
		{ LIc_BitMask_Action,	0,		ONcAnimType_None,	ONcAnimType_Taunt, UUcFalse },
		{ 0,					0,		ONcAnimType_None,	ONcAnimType_None, UUcFalse }
	};

	animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, table, ONcAnimPriority_Appropriate, ONcAttackMove, NULL);

	return animFound;
}

UUtBool ONrGameState_TryActionMarker(ONtCharacter *inCharacter, ONtActionMarker *inActionMarker, UUtBool inGenerous)
{
	M3tPoint3D pelvis_pt, target_pt;
	M3tVector3D delta_pt;
	float delta_angle, sintheta, costheta, p_dist, p_sideways, facing, tolerance;

	if (inActionMarker == NULL)
		return UUcFalse;

	// check the character is in a valid animtype to use the console
	if (!ONrCharacter_CanUseConsole(inCharacter)) {
		if (ONgDebugConsoleAction) {
			COrConsole_Printf("%s: can't use console, invalid animtype %s", inCharacter, ONrAnimTypeToString(ONrCharacter_GetAnimType(inCharacter)));
		}
		return UUcFalse;
	}

	/*
	 * height check
	 */
	ONrCharacter_GetPelvisPosition(inCharacter, &pelvis_pt);
	MUmVector_Subtract(delta_pt, pelvis_pt, inActionMarker->position);
	if ((float)fabs(delta_pt.y) > ONcActionMarker_MaxHeight) {
		if (ONgDebugConsoleAction) {
			COrConsole_Printf("%s: can't use console, pelvis height %f - console height %f = %f > tolerance %f",
							inCharacter, pelvis_pt.y, inActionMarker->position.y, (float)fabs(delta_pt.y), ONcActionMarker_MaxHeight);
		}
		return UUcFalse;
	}

	/*
	 * perpendicular distance check
	 */
	sintheta = MUrSin(inActionMarker->direction);
	costheta = MUrCos(inActionMarker->direction);
	p_dist = - (delta_pt.x * sintheta + delta_pt.z * costheta);
	if (p_dist < ONcActionMarker_MinDistance) {
		if (ONgDebugConsoleAction) {
			COrConsole_Printf("%s: can't use console, distance %f < min tolerance %f",
							inCharacter, p_dist, ONcActionMarker_MinDistance);
		}
		return UUcFalse;
	} else if (p_dist > ONcActionMarker_MaxDistance) {
		if (ONgDebugConsoleAction) {
			COrConsole_Printf("%s: can't use console, distance %f > tolerance %f",
							inCharacter, p_dist, ONcActionMarker_MaxDistance);
		}
		return UUcFalse;
	}

	/*
	 * sideways distance check
	 */
	tolerance = ONcActionMarker_MaxSideways;
	if (inGenerous) {
		tolerance *= 2;
	}
	p_sideways = delta_pt.x * costheta - delta_pt.z * sintheta;
	if ((float)fabs(p_sideways) > tolerance) {
		if (ONgDebugConsoleAction) {
			COrConsole_Printf("%s: can't use console, sideways distance %f > tolerance %f",
							inCharacter, (float)fabs(p_sideways), tolerance);
		}
		return UUcFalse;
	}

	/*
	 * angle check
	 */
	if (inCharacter->charType == ONcChar_Player) {
		facing = ONrCharacter_GetCosmeticFacing(inCharacter);
	} else {
		facing = inCharacter->facing;
	}

	tolerance = ONcActionMarker_LookAngle;
	if (inGenerous) {
		tolerance *= 1.5f;
	}
	delta_angle = facing - inActionMarker->direction;
	UUmTrig_ClipAbsPi(delta_angle);

	if ((float)fabs(delta_angle) > tolerance) {
		if (ONgDebugConsoleAction) {
			COrConsole_Printf("%s: can't use console, facing %f - direction %f = %f > tolerance %f",
							inCharacter, facing, inActionMarker->direction,
							(float)fabs(delta_angle), tolerance);
		}
		return UUcFalse;
	}

	/*
	 * blockage check
	 */

	if (MUmVector_GetLengthSquared(delta_pt) > UUmSQR(ONcActionMarker_ObstructionDist)) {
		// we are a sufficient distance away from the action marker that we need to
		// make sure there are no obstructions between it and us
		MUmVector_Copy(target_pt, inActionMarker->position);
		target_pt.x -= sintheta * ONcActionMarker_ObstructionDist;
		target_pt.z -= costheta * ONcActionMarker_ObstructionDist;

		if (AI2rManeuver_CheckBlockage(inCharacter, &target_pt)) {
			if (ONgDebugConsoleAction) {
				COrConsole_Printf("%s: can't use console, blocked by environment", inCharacter);
			}
			return UUcFalse;
		}

		if (AI2rManeuver_CheckInterveningCharacters(inCharacter, &target_pt)) {
			if (ONgDebugConsoleAction) {
				COrConsole_Printf("%s: can't use console, blocked by character(s)", inCharacter);
			}
			return UUcFalse;
		}
	}

	if (ONgDebugConsoleAction) {
		COrConsole_Printf("%s: can use console");
	}
	return UUcTrue;
}

// handle taunt / action key
static UUtBool HandleStartTauntOrAction(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool				use_console = UUcFalse, use_door = UUcFalse, taunt_enable, huge_weapon;
	ONtActionMarker*	action_marker = NULL;
	M3tPoint3D			pos;
	OBJtObject *		closest_door;
	UUtBool				door_failed = UUcFalse;

	// if we are currently doing the action, update it...
	if ((ioActiveCharacter != NULL) && (ioCharacter->console_marker))
	{
		ONrCharacter_ConsoleAction_Update(ioCharacter, ioActiveCharacter);
		return UUcTrue;
	}

	// if the action key wasnt pressed, exit...
	if ((inInput->buttonWentDown & LIc_BitMask_Action) == 0) {
		return UUcFalse;
	}

	huge_weapon = (ioCharacter->inventory.weapons[0] != NULL) && (WPrGetClass(ioCharacter->inventory.weapons[0])->flags & WPcWeaponClassFlag_Unstorable);

	// AIs do not use objects or doors through this function, but rather directly
	if (ONrCharacter_IsPlayingFilm(ioCharacter) || ((ioCharacter->flags & ONcCharacterFlag_AIMovement) == 0)) {
		// find a marker near us
		ONrCharacter_GetPelvisPosition(ioCharacter, &pos);
		action_marker = ONrLevel_ActionMarker_FindNearest(&pos);

		// if you are reloading, you can't use a console
		if ((ONcChar_Player == ioCharacter->charType) && ONrCharacter_IsReloading(ioCharacter)) {
			// can't reload we are a player & we are reloading
		}
		else {
			// we must be able to put away our weapon to use a console
			use_console = (!huge_weapon) && ONrGameState_TryActionMarker(ioCharacter, action_marker, UUcFalse);
			if (use_console) {
				ONrCharacter_ConsoleAction_Begin(ioCharacter, action_marker);
				return UUcTrue;
			}
		}

		use_door = OBJrDoor_TryDoorAction(ioCharacter, &closest_door);
		if (closest_door != NULL) {
			if (use_door) {
				use_door = OBJrDoor_CharacterOpen(ioCharacter, closest_door);
			}

			if (use_door) {
				ONrGameState_EventSound_Play(ONcEventSound_DoorAction_Success, &closest_door->position);
				return UUcTrue;
			} else {
				door_failed = UUcTrue;
			}
		}

		// look for neutral characters to activate
		if (AI2rTryNeutralInteraction(ioCharacter, UUcTrue)) {
			return UUcTrue;
		}

		// door failure is less important then NCI, but more important then taunting
		if (door_failed) {
			ONrGameState_EventSound_Play(ONcEventSound_DoorAction_Fail, &closest_door->position);
			return UUcTrue;
		}
	}

	if (ioCharacter == ONrGameState_GetPlayerCharacter()) {
		taunt_enable = ONrGameState_IsTauntEnabled();
	} else {
		taunt_enable = UUcTrue;
	}

	// we can't taunt when holding a huge weapon (e.g. barabbas' cannon)
	taunt_enable = taunt_enable && !huge_weapon;

	if (taunt_enable) {
		// action button maps to taunt
		return HandleStartTaunt(inInput, ioCharacter, ioActiveCharacter);
	} else {
		// action button does nothing
		return UUcFalse;
	}
}


static UUtBool HandlePriorAction(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool priorAction = UUcFalse;

	switch(ioActiveCharacter->nextAnimType)
	{
		case ONcAnimType_Slide:
		case ONcAnimType_Jump:
		case ONcAnimType_Jump_Forward:
		case ONcAnimType_Jump_Backward:
		case ONcAnimType_Jump_Left:
		case ONcAnimType_Jump_Right:
		case ONcAnimType_Kick:
		case ONcAnimType_Punch:
		case ONcAnimType_Kick_Forward:
		case ONcAnimType_Kick_Left:
		case ONcAnimType_Kick_Right:
		case ONcAnimType_Kick_Back:
		case ONcAnimType_Kick_Low:

		case ONcAnimType_Punch_Forward:
		case ONcAnimType_Punch_Left:
		case ONcAnimType_Punch_Right:
		case ONcAnimType_Punch_Back:
		case ONcAnimType_Punch_Low:

		case ONcAnimType_Kick2:
		case ONcAnimType_Kick3:
		case ONcAnimType_Kick3_Forward:

		case ONcAnimType_Punch2:
		case ONcAnimType_Punch3:
		case ONcAnimType_Punch4:

		case ONcAnimType_PPKKKK:
		case ONcAnimType_PPKKK:
		case ONcAnimType_PPKK:
		case ONcAnimType_PPK:
		case ONcAnimType_PKK:
		case ONcAnimType_PKP:
		case ONcAnimType_KPK:
		case ONcAnimType_KPP:
		case ONcAnimType_KKP:
		case ONcAnimType_PK:
		case ONcAnimType_KP:

		case ONcAnimType_PF_PF:
		case ONcAnimType_PF_PF_PF:
		case ONcAnimType_PL_PL:
		case ONcAnimType_PL_PL_PL:
		case ONcAnimType_PR_PR:
		case ONcAnimType_PR_PR_PR:
		case ONcAnimType_PB_PB:
		case ONcAnimType_PB_PB_PB:
		case ONcAnimType_PD_PD:
		case ONcAnimType_PD_PD_PD:
		case ONcAnimType_KF_KF:
		case ONcAnimType_KF_KF_KF:
		case ONcAnimType_KL_KL:
		case ONcAnimType_KL_KL_KL:
		case ONcAnimType_KR_KR:
		case ONcAnimType_KR_KR_KR:
		case ONcAnimType_KB_KB:
		case ONcAnimType_KB_KB_KB:
		case ONcAnimType_KD_KD:
		case ONcAnimType_KD_KD_KD:

		case ONcAnimType_Run_Kick:
		case ONcAnimType_Run_Punch:

		case ONcAnimType_Run_Back_Punch:
		case ONcAnimType_Run_Back_Kick:

		case ONcAnimType_Sidestep_Left_Kick:
		case ONcAnimType_Sidestep_Left_Punch:

		case ONcAnimType_Sidestep_Right_Kick:
		case ONcAnimType_Sidestep_Right_Punch:

		case ONcAnimType_Hit_Head:
		case ONcAnimType_Hit_Body:
		case ONcAnimType_Hit_Foot:

		case ONcAnimType_Land_Dead:
		case ONcAnimType_Knockdown_Head:
		case ONcAnimType_Knockdown_Body:
		case ONcAnimType_Knockdown_Foot:

		case ONcAnimType_Hit_Crouch:
		case ONcAnimType_Knockdown_Crouch:

		case ONcAnimType_Hit_Fallen:

		case ONcAnimType_Hit_Head_Behind:
		case ONcAnimType_Hit_Body_Behind:
		case ONcAnimType_Hit_Foot_Behind:

		case ONcAnimType_Knockdown_Head_Behind:
		case ONcAnimType_Knockdown_Body_Behind:
		case ONcAnimType_Knockdown_Foot_Behind:

		case ONcAnimType_Hit_Crouch_Behind:
		case ONcAnimType_Knockdown_Crouch_Behind:
		case ONcAnimType_Taunt:

		case ONcAnimType_Pickup_Object_Mid:
		case ONcAnimType_Pickup_Pistol_Mid:
		case ONcAnimType_Pickup_Rifle_Mid:

		case ONcAnimType_Pickup_Object:
		case ONcAnimType_Pickup_Pistol:
		case ONcAnimType_Pickup_Rifle:

		case ONcAnimType_Holster:
		case ONcAnimType_Draw_Pistol:
		case ONcAnimType_Draw_Rifle:

		case ONcAnimType_Blownup:
		case ONcAnimType_Blownup_Behind:
		case ONcAnimType_Powerup:
		case ONcAnimType_Muro_Thunderbolt:

		case ONcAnimType_Thrown1:
		case ONcAnimType_Thrown2:
		case ONcAnimType_Thrown3:
		case ONcAnimType_Thrown4:
		case ONcAnimType_Thrown5:
		case ONcAnimType_Thrown6:
		case ONcAnimType_Thrown7:
		case ONcAnimType_Thrown8:
		case ONcAnimType_Thrown9:
		case ONcAnimType_Thrown10:
		case ONcAnimType_Thrown11:
		case ONcAnimType_Thrown12:
		case ONcAnimType_Thrown13:
		case ONcAnimType_Thrown14:
		case ONcAnimType_Thrown15:
		case ONcAnimType_Thrown16:
		case ONcAnimType_Thrown17:
			priorAction = UUcTrue;
		break;
	}

	return priorAction;
}

static UUtBool HandleStun(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool handled = UUcFalse;
	const TRtAnimation *animation;

	if (ioActiveCharacter->blockStun > 0) {
		ONrCharacter_FightMode(ioCharacter);
		animation = ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, ONcAnimPriority_Appropriate, ONcAnimType_Block);

		if (NULL != animation) {
			handled = UUcTrue;
		}
		else {
			COrConsole_Printf("failed to find valid block animation");
		}
	}

	if (ioActiveCharacter->hitStun > 0) {
		ONrCharacter_FightMode(ioCharacter);

		if ((ioActiveCharacter->animFrame + 1) < TRrAnimation_GetDuration(ioActiveCharacter->animation)) {
			if (ONrCharacter_IsVictimAnimation(ioCharacter)) {
				handled = UUcTrue;
			}
		}
	}

	if (ioActiveCharacter->dizzyStun > 0) {
		ONtAnimPriority dizzy_priority;
		ONrCharacter_FightMode(ioCharacter);

		switch(ioActiveCharacter->curAnimType)
		{
			case ONcAnimType_Stagger:
			case ONcAnimType_Stagger_Behind:
				if (ioActiveCharacter->staggerStun > 0) {
					dizzy_priority = ONcAnimPriority_Queue;
				}
				else {
					dizzy_priority = ONcAnimPriority_Force;
				}
			break;

			case ONcAnimType_Stun:
				dizzy_priority = ONcAnimPriority_Queue;
			break;

			default:
				dizzy_priority = ONcAnimPriority_Force;
			break;
		}

		animation = ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, dizzy_priority, ONcAnimType_Stun);

		if (NULL != animation) {
			handled = UUcTrue;
		} else {
//			COrConsole_Printf("failed to find animtype Stun ... fromstate %s tostate %s",
//				ONrAnimStateToString(ioActiveCharacter->curFromState), ONrAnimStateToString(ioActiveCharacter->nextAnimState));
			ioActiveCharacter->dizzyStun = 0;
		}
	}

	if (ioActiveCharacter->staggerStun > 0) {
		ONrCharacter_FightMode(ioCharacter);

		switch(ioActiveCharacter->curAnimType)
		{
			case ONcAnimType_Stagger:
				animation = ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, ONcAnimPriority_Queue, ONcAnimType_Stagger);
			break;

			case ONcAnimType_Stagger_Behind:
				animation = ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, ONcAnimPriority_Queue, ONcAnimType_Stagger_Behind);
			break;
		}

		if (NULL != animation) {
			handled = UUcTrue;
		}
		else {
			ioActiveCharacter->staggerStun = 0;
		}
	}

	return handled;
}


static UUtBool HandleStartSidestep(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool leftDown = (inInput->buttonIsDown & LIc_BitMask_StepLeft) ? UUcTrue : UUcFalse;
	UUtBool rightDown = (inInput->buttonIsDown & LIc_BitMask_StepRight) ? UUcTrue : UUcFalse;
	UUtBool walkDown = (inInput->buttonIsDown & LIc_BitMask_Walk) ? UUcTrue : UUcFalse;
	UUtBool crouchDown = (inInput->buttonIsDown & LIc_BitMask_Crouch) ? UUcTrue : UUcFalse;
	UUtBool restricted = ((ioCharacter->flags & ONcCharacterFlag_RestrictedMovement) > 0);
	UUtBool animFound = UUcFalse;
	UUtBool moveForwards;
	TRtAnimType sidestep_animtype;
	TRtAnimType rotated_animtype;
	const TRtAnimation *sidestep_anim;
	const TRtAnimation *rotated_anim;
	const TRtAnimation *played_anim;

	const ONtMoveLookup sidestep_run_table[] =
	{
		{ 0, LIc_BitMask_Crouch | LIc_BitMask_StepLeft,		ONcAnimType_None,	ONcAnimType_Crouch_Run_Sidestep_Left , UUcFalse },
		{ 0, LIc_BitMask_Crouch | LIc_BitMask_StepRight,	ONcAnimType_None,	ONcAnimType_Crouch_Run_Sidestep_Right , UUcFalse },
		{ 0, LIc_BitMask_StepLeft,							ONcAnimType_None,	ONcAnimType_Run_Sidestep_Left_Start , UUcFalse },
		{ 0, LIc_BitMask_StepLeft,							ONcAnimType_None,	ONcAnimType_Run_Sidestep_Left , UUcFalse },
		{ 0, LIc_BitMask_StepLeft,							ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Left , UUcFalse },
		{ 0, LIc_BitMask_StepRight,							ONcAnimType_None,	ONcAnimType_Run_Sidestep_Right_Start , UUcFalse },
		{ 0, LIc_BitMask_StepRight,							ONcAnimType_None,	ONcAnimType_Run_Sidestep_Right , UUcFalse },
		{ 0, LIc_BitMask_StepRight,							ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Right , UUcFalse },
		{ 0,	0,										ONcAnimType_None, UUcFalse  }
	};

	const ONtMoveLookup sidestep_walk_table[] =
	{
		{ 0, LIc_BitMask_Crouch | LIc_BitMask_Walk |
			 LIc_BitMask_StepLeft,							ONcAnimType_None, ONcAnimType_Crouch_Walk_Sidestep_Left , UUcFalse },

		{ 0, LIc_BitMask_Crouch | LIc_BitMask_Walk |
			LIc_BitMask_StepRight,							ONcAnimType_None, ONcAnimType_Crouch_Walk_Sidestep_Right , UUcFalse },

		{ 0, LIc_BitMask_StepLeft | LIc_BitMask_Walk,		ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Left , UUcFalse },
		{ 0, LIc_BitMask_StepLeft,							ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Left , UUcFalse },

		{ 0, LIc_BitMask_StepRight | LIc_BitMask_Walk,		ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Right , UUcFalse },
		{ 0, LIc_BitMask_StepRight,							ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Right , UUcFalse },

		{ 0,	0,										ONcAnimType_None }
	};

	const ONtMoveLookup sidestep_restricted_table[] =
	{
		{ 0, LIc_BitMask_Crouch | LIc_BitMask_Walk |
			 LIc_BitMask_StepLeft,							ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Left , UUcFalse },

		{ 0, LIc_BitMask_Crouch | LIc_BitMask_Walk |
			LIc_BitMask_StepRight,							ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Right , UUcFalse },

		{ 0, LIc_BitMask_StepLeft | LIc_BitMask_Walk,		ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Left , UUcFalse },
		{ 0, LIc_BitMask_StepLeft,							ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Left , UUcFalse },

		{ 0, LIc_BitMask_StepRight | LIc_BitMask_Walk,		ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Right , UUcFalse },
		{ 0, LIc_BitMask_StepRight,							ONcAnimType_None,	ONcAnimType_Walk_Sidestep_Right , UUcFalse },

		{ 0,	0,										ONcAnimType_None }
	};

	const ONtMoveLookup *sidestep_table = NULL;

	// if neither left nor right is down, can't sidestep!
	if (!leftDown && !rightDown) {
		goto exit;
	}

	if (ioActiveCharacter->inAirControl.numFramesInAir > 0) {
		goto exit;
	}

	sidestep_anim = NULL;
	rotated_anim = NULL;
	sidestep_animtype = ONcAnimType_None;
	rotated_animtype = ONcAnimType_None;

	sidestep_table = restricted ? sidestep_restricted_table : (walkDown ? sidestep_walk_table : sidestep_run_table);
	sidestep_anim = DoTableLookup(inInput, ioCharacter, ioActiveCharacter, sidestep_table, &sidestep_animtype);

	if ((sidestep_anim != NULL) && (TRrAnimation_GetVarient(sidestep_anim) == ioActiveCharacter->animVarient)) {
		// we found an animation with the correct varient, play it and then return
		animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, sidestep_table, ONcAnimPriority_Appropriate, ONcNormalMove, &played_anim);
		if (animFound) {
			return animFound;
		} else {
			// CB: if DoTableLookup is not in sync with DoTableAnimation, this could be worthy of investigation in future
			sidestep_anim = NULL;
		}
	}

	if (!animFound) {
		// try to turn the character and walk or run, since we didn't find a sidestep animation with the right varient flags.
		TRtAnimType runWalk[4];
		UUtInt32 itr;
		ONtCharacterClass *characterClass = ioCharacter->characterClass;
		TRtAnimationCollection *collection = characterClass->animations;

		if (ioCharacter->flags & ONcCharacterFlag_AIMovement) {
			// ask the AI whether to move forwards or backwards if we can't sidestep.
			moveForwards = AI2rMovementState_ForwardMovementPreference(ioCharacter, &ioActiveCharacter->movement_state);
		} else {
			// we are a player - move forwards instead and use a facing modifier
			moveForwards = UUcTrue;
		}

		if (moveForwards) {
			if (restricted) {
				runWalk[0] = ONcAnimType_Walk;
				runWalk[1] = ONcAnimType_None;
				runWalk[2] = ONcAnimType_None;
				runWalk[3] = ONcAnimType_None;
			}
			else if (walkDown && crouchDown) {
				runWalk[0] = ONcAnimType_Crouch_Walk;
				runWalk[1] = ONcAnimType_Crouch_Run;
				runWalk[2] = ONcAnimType_Walk;
				runWalk[3] = ONcAnimType_Run;
			}
			else if (crouchDown) {
				runWalk[0] = ONcAnimType_Crouch_Run;
				runWalk[1] = ONcAnimType_Crouch_Walk;
				runWalk[2] = ONcAnimType_Run;
				runWalk[3] = ONcAnimType_Walk;
			}
			else if (walkDown) {
				runWalk[0] = ONcAnimType_Walk;
				runWalk[1] = ONcAnimType_Run;
				runWalk[2] = ONcAnimType_None;
				runWalk[3] = ONcAnimType_None;
			}
			else {
				runWalk[0] = ONcAnimType_Run;
				runWalk[1] = ONcAnimType_Walk;
				runWalk[2] = ONcAnimType_None;
				runWalk[3] = ONcAnimType_None;
			}
		} else {
			if (restricted) {
				runWalk[0] = ONcAnimType_Walk_Backwards;
				runWalk[1] = ONcAnimType_None;
				runWalk[2] = ONcAnimType_None;
				runWalk[3] = ONcAnimType_None;
			}
			else if (walkDown && crouchDown) {
				runWalk[0] = ONcAnimType_Crouch_Walk_Backwards;
				runWalk[1] = ONcAnimType_Crouch_Run_Backwards;
				runWalk[2] = ONcAnimType_Walk_Backwards;
				runWalk[3] = ONcAnimType_Run_Backwards;
			}
			else if (crouchDown) {
				runWalk[0] = ONcAnimType_Crouch_Run_Backwards;
				runWalk[1] = ONcAnimType_Crouch_Walk_Backwards;
				runWalk[2] = ONcAnimType_Run_Backwards;
				runWalk[3] = ONcAnimType_Walk_Backwards;
			}
			else if (walkDown) {
				runWalk[0] = ONcAnimType_Walk_Backwards;
				runWalk[1] = ONcAnimType_Run_Backwards;
				runWalk[2] = ONcAnimType_None;
				runWalk[3] = ONcAnimType_None;
			}
			else {
				runWalk[0] = ONcAnimType_Run_Backwards;
				runWalk[1] = ONcAnimType_Walk_Backwards;
				runWalk[2] = ONcAnimType_None;
				runWalk[3] = ONcAnimType_None;
			}
		}

		animFound = UUcFalse;

		for(itr = 0; itr < 4; itr++)
		{
			if (ONcAnimType_None != runWalk[itr])
			{
				TRtAnimType start_type = ONcAnimType_None;

				// first, try just straight moving in this direction
				rotated_anim = TRrCollection_Lookup(collection, runWalk[itr], ioActiveCharacter->nextAnimState, ioActiveCharacter->animVarient);
				if (rotated_anim != NULL) {
					rotated_animtype = runWalk[itr];
					break;
				}

				// second, try starting to move in this direction
				switch(runWalk[itr])
				{
					case ONcAnimType_Run:
						start_type = ONcAnimType_Run_Start;
					break;

					case ONcAnimType_Run_Backwards:
						start_type = ONcAnimType_Run_Backwards_Start;
					break;

					case ONcAnimType_Walk:
						start_type = ONcAnimType_Walk_Start;
					break;

					case ONcAnimType_Walk_Backwards:
						start_type = ONcAnimType_Walk_Backwards_Start;
					break;
				}

				if (start_type != ONcAnimType_None) {
					rotated_anim = TRrCollection_Lookup(collection, start_type, ioActiveCharacter->nextAnimState, ioActiveCharacter->animVarient);
					if (rotated_anim != NULL) {
						rotated_animtype = start_type;
						break;
					}
				}
			}
		}
	}

	if (rotated_anim != NULL) {
		// play the rotated anim only if it has better varient flags than the sidestep animation
		if ((sidestep_anim == NULL) || (TRrAnimation_GetVarient(rotated_anim) > TRrAnimation_GetVarient(sidestep_anim))) {
			played_anim = ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, ONcAnimPriority_Appropriate, rotated_animtype);

			if (played_anim) {
				animFound = UUcTrue;
				if (ioCharacter->flags & ONcCharacterFlag_AIMovement) {
					// tell the AI that we are using a rotated forwards or backwards movement, and that it shouldn't
					// try using a sidestep in the future.
					AI2rMovementState_SidestepNotFound(ioCharacter, &ioActiveCharacter->movement_state, walkDown);
				}
			} else {
				rotated_anim = NULL;
			}
		}
	}

	if (!animFound && (sidestep_anim != NULL)) {
		// we found an animation with the correct varient, play it and then return
		animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, sidestep_table, ONcAnimPriority_Appropriate, ONcNormalMove, &played_anim);
		if (animFound) {
			return animFound;
		}
	}

	if (animFound) {
		// if we actually ended up finding a forwards or backwards animation, set up
		// the facing modifier appropriately
		switch(TRrAnimation_GetType(ioActiveCharacter->animation))
		{
			case ONcAnimType_Crouch_Walk:
			case ONcAnimType_Crouch_Run:
			case ONcAnimType_Walk:
			case ONcAnimType_Run:
			case ONcAnimType_Run_Start:
			case ONcAnimType_Walk_Start:
			case ONcAnimType_Walk_Backwards_Start:
			case ONcAnimType_Run_Backwards_Start:

				if (leftDown) {
					ONrDesireFacingModifier(ioCharacter, (M3cPi/2.f));
				}
				else if (rightDown) {
					ONrDesireFacingModifier(ioCharacter, -(M3cPi/2.f));
				}
			break;
		}
	}

exit:
	return animFound;
}

static UUtBool HandleStartRunning(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool animFound = UUcFalse;
	UUtBool crouchDown = ((inInput->buttonIsDown & LIc_BitMask_Crouch) > 0);
	const TRtAnimation *found_anim;
	const ONtMoveLookup *table;
	const ONtMoveLookup stand_table[] =
	{
		{ 0, LIc_BitMask_Forward | LIc_BitMask_Walk,		ONcAnimType_None,	ONcAnimType_Walk_Start , UUcFalse },
		{ 0, LIc_BitMask_Forward | LIc_BitMask_Walk,		ONcAnimType_None,	ONcAnimType_Walk , UUcFalse },
		{ 0, LIc_BitMask_Backward | LIc_BitMask_Walk,		ONcAnimType_None,	ONcAnimType_Walk_Backwards_Start , UUcFalse },
		{ 0, LIc_BitMask_Backward | LIc_BitMask_Walk,		ONcAnimType_None,	ONcAnimType_Walk_Backwards , UUcFalse },

		{ 0, LIc_BitMask_Forward,							ONcAnimType_None,	ONcAnimType_Run_Start , UUcFalse },
		{ 0, LIc_BitMask_Forward,							ONcAnimType_None,	ONcAnimType_Run , UUcFalse },
		{ 0, LIc_BitMask_Forward,							ONcAnimType_None,	ONcAnimType_Walk_Start , UUcFalse },
		{ 0, LIc_BitMask_Forward,							ONcAnimType_None,	ONcAnimType_Walk , UUcFalse },
		{ 0, LIc_BitMask_Backward,							ONcAnimType_None,	ONcAnimType_Run_Backwards_Start , UUcFalse },
		{ 0, LIc_BitMask_Backward,							ONcAnimType_None,	ONcAnimType_Run_Backwards , UUcFalse },
		{ 0, LIc_BitMask_Backward,							ONcAnimType_None,	ONcAnimType_Walk_Backwards_Start , UUcFalse },
		{ 0, LIc_BitMask_Backward,							ONcAnimType_None,	ONcAnimType_Walk_Backwards , UUcFalse },

		{ 0,			0,								ONcAnimType_None, UUcFalse  }
	};

	const ONtMoveLookup restricted_table[] =
	{
		{ 0, LIc_BitMask_Forward,							ONcAnimType_None,	ONcAnimType_Walk_Start , UUcFalse },
		{ 0, LIc_BitMask_Forward,							ONcAnimType_None,	ONcAnimType_Walk , UUcFalse },
		{ 0, LIc_BitMask_Backward,							ONcAnimType_None,	ONcAnimType_Walk_Backwards_Start , UUcFalse },
		{ 0, LIc_BitMask_Backward,							ONcAnimType_None,	ONcAnimType_Walk_Backwards , UUcFalse },

		{ 0,			0,								ONcAnimType_None, UUcFalse  }
	};

	const ONtMoveLookup crouch_table[] =
	{
		{ 0, LIc_BitMask_Crouch |
			LIc_BitMask_Walk | LIc_BitMask_Forward,			ONcAnimType_None, ONcAnimType_Crouch_Walk , UUcFalse },
		{ 0, LIc_BitMask_Crouch | LIc_BitMask_Forward,		ONcAnimType_None, ONcAnimType_Crouch_Run , UUcFalse },
		{ 0, LIc_BitMask_Crouch | LIc_BitMask_Forward,		ONcAnimType_None, ONcAnimType_Crouch_Walk , UUcFalse },

		{ 0, LIc_BitMask_Crouch |
			LIc_BitMask_Walk | LIc_BitMask_Backward,		ONcAnimType_None, ONcAnimType_Crouch_Walk_Backwards , UUcFalse },
		{ 0, LIc_BitMask_Crouch | LIc_BitMask_Backward,		ONcAnimType_None, ONcAnimType_Crouch_Run_Backwards , UUcFalse },
		{ 0, LIc_BitMask_Crouch | LIc_BitMask_Backward,		ONcAnimType_None, ONcAnimType_Crouch_Walk_Backwards , UUcFalse },

		{ 0,			0,								ONcAnimType_None, UUcFalse  }
	};

	if (0 == (inInput->buttonIsDown & (LIc_BitMask_Forward | LIc_BitMask_Backward))) {
		goto exit;
	}

	if (ioActiveCharacter->inAirControl.numFramesInAir > 0) {
		goto exit;
	}

	if (ioCharacter->flags & ONcCharacterFlag_RestrictedMovement) {
		if (crouchDown) {
			// cannot move
			return UUcFalse;
		} else {
			table = restricted_table;
		}
	} else {
		table = crouchDown ? crouch_table : stand_table;
	}

	animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, table, ONcAnimPriority_Appropriate, ONcNormalMove, &found_anim);

	if (animFound) {
		float facingModifier;

		// turn a different direction depending on backards of forwards
		if (inInput->buttonIsDown & LIc_BitMask_Forward) {
			facingModifier = M3cPi / 4;
		}
		else {
			facingModifier = -M3cPi / 4;
		}

		// ok if you are running and sidestepping you get rotation
		switch(TRrAnimation_GetType(found_anim))
		{
			case ONcAnimType_Walk_Backwards:
			case ONcAnimType_Walk_Backwards_Start:
			case ONcAnimType_Run_Backwards:
			case ONcAnimType_Run_Backwards_Start:
			case ONcAnimType_Run_Sidestep_Left_Start:
			case ONcAnimType_Run_Sidestep_Right_Start:
			case ONcAnimType_Walk:
			case ONcAnimType_Walk_Start:
			case ONcAnimType_Run:
			case ONcAnimType_Run_Start:
			case ONcAnimType_Walk_Stop:
			case ONcAnimType_Run_Stop:
				if (inInput->buttonIsDown & LIc_BitMask_StepRight) {
					ONrDesireFacingModifier(ioCharacter, -facingModifier);
				}
				else if (inInput->buttonIsDown & LIc_BitMask_StepLeft) {
					ONrDesireFacingModifier(ioCharacter, facingModifier);
				}
			break;
		}
	}

	if (!animFound)
	{
		const TRtAnimation *oldAnimation = ioActiveCharacter->animation;

		switch(ioActiveCharacter->nextAnimState)
		{
			case ONcAnimState_Fallen_Back:
			case ONcAnimState_Fallen_Front:
				animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, table, ONcAnimPriority_Appropriate, ONcAttackMove, NULL);

				if (animFound) {
					COrConsole_Printf("### help, I am fallen but without the fight varient I can't get up (%s).", TRrAnimation_GetName(oldAnimation));
				}
			break;
		}
	}


exit:
	return animFound;
}

static UUtBool HandleStop(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool animFound = UUcFalse;
	const ONtMoveLookup stop_table[] =
	{
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_1Step_Stop , UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Run_Stop , UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Walk_Stop , UUcFalse },
		{ 0,	LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Crouch , UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Stand , UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_None, UUcFalse  }
	};

	switch(ioActiveCharacter->curAnimType)
	{
		case ONcAnimType_Standing_Turn_Left:
		case ONcAnimType_Standing_Turn_Right:

		case ONcAnimType_Run:
		case ONcAnimType_Run_Start:
		case ONcAnimType_Run_Backwards:
		case ONcAnimType_Run_Backwards_Start:
		case ONcAnimType_Run_Sidestep_Left:
		case ONcAnimType_Run_Sidestep_Left_Start:
		case ONcAnimType_Run_Sidestep_Right:
		case ONcAnimType_Run_Sidestep_Right_Start:

		case ONcAnimType_Walk:
		case ONcAnimType_Walk_Start:
		case ONcAnimType_Walk_Backwards:
		case ONcAnimType_Walk_Backwards_Start:
		case ONcAnimType_Walk_Sidestep_Left:
		case ONcAnimType_Walk_Sidestep_Right:

		case ONcAnimType_Crouch_Run:
		case ONcAnimType_Crouch_Run_Backwards:
		case ONcAnimType_Crouch_Run_Sidestep_Left:
		case ONcAnimType_Crouch_Run_Sidestep_Right:

		case ONcAnimType_Crouch_Walk:
		case ONcAnimType_Crouch_Walk_Backwards:
		case ONcAnimType_Crouch_Walk_Sidestep_Left:
		case ONcAnimType_Crouch_Walk_Sidestep_Right:
			animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, stop_table, ONcAnimPriority_Appropriate, ONcNormalMove, NULL);
		break;
	}

	return animFound;
}

static UUtBool HandleLeaveStun(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool animFound = UUcFalse;

	const ONtMoveLookup leave_stun_table[] =
	{
		{ 0,	LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Crouch, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Stand, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Crouch, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_None, UUcFalse },
	};

	switch(ioActiveCharacter->curAnimType)
	{
		case ONcAnimType_Stagger:
		case ONcAnimType_Stagger_Behind:
		case ONcAnimType_Stun:
		case ONcAnimType_Block:
			animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, leave_stun_table, ONcAnimPriority_Force, ONcNormalMove, NULL);
			if (animFound) { goto exit; }
		break;
	}

exit:
	return animFound;
}


static UUtBool HandleChangeStance(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool animFound = UUcFalse;

	const ONtMoveLookup crouchTable[] =
	{
		{ 0,	LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Crouch, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_None, UUcFalse },
	};

	const ONtMoveLookup standTable[] =
	{
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Stand, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_None, UUcFalse },
	};

	switch(ioActiveCharacter->nextAnimState)
	{
		case ONcAnimState_Standing:
			if (LIc_BitMask_Crouch == (inInput->buttonIsDown & LIc_BitMask_Crouch))
			{
				animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, crouchTable, ONcAnimPriority_Appropriate, ONcNormalMove, NULL);
				if (animFound) { goto exit; }
		}
		break;

		case ONcAnimState_Crouch_Start:
			animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, crouchTable, ONcAnimPriority_Appropriate, ONcNormalMove, NULL);
			if (animFound) { goto exit; }

			animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, standTable, ONcAnimPriority_Appropriate, ONcNormalMove, NULL);
			if (animFound) { goto exit; }
		break;

		case ONcAnimState_Crouch:
			if (0 == (inInput->buttonIsDown & LIc_BitMask_Crouch))
			{
				animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, standTable, ONcAnimPriority_Appropriate, ONcNormalMove, NULL);
				if (animFound) { goto exit; }
			}
		break;
	}

exit:
	return animFound;
}

static UUtBool HandleFallThrough(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	ONtAnimPriority animPriority = ONcAnimPriority_Queue;
	const TRtAnimation *oldAnimation = ioActiveCharacter->animation;
	UUtUns16 fromState = TRrAnimation_GetFrom(ioActiveCharacter->animation);
	UUtUns16 toState = TRrAnimation_GetTo(ioActiveCharacter->animation);
	UUtBool loop = fromState == toState;
	UUtBool animFound = UUcFalse;
	UUtBool in_bad_varient = UUcFalse;
	const ONtMoveLookup crouchTable[] =
	{
		{ 0,	LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Crouch, UUcFalse },
		{ 0,	LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Prone, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_None , UUcFalse },
	};

	const ONtMoveLookup standTable[] =
	{
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_1Step_Stop, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Run_Stop, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Walk_Stop, UUcFalse },
		{ 0,	LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Crouch, UUcFalse },
		{ 0,	LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Prone, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Stand, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Crouch, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Prone, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_None, UUcFalse },
	};

	in_bad_varient = ONrCharacter_InBadVarientState(ioCharacter, ioActiveCharacter);

	if (in_bad_varient) {
		if (!TRrAnimation_IsAtomic(ioActiveCharacter->animation, ioActiveCharacter->animFrame)) {
			animPriority = ONcAnimPriority_Force;
		}
	}

	if (ioActiveCharacter->inAirControl.numFramesInAir > 0) {
		switch(ioActiveCharacter->curAnimType)
		{
			case ONcAnimType_Run:
			case ONcAnimType_Run_Backwards:
			case ONcAnimType_Run_Sidestep_Left:
			case ONcAnimType_Run_Sidestep_Right:
			case ONcAnimType_Walk:
			case ONcAnimType_Walk_Sidestep_Left:
			case ONcAnimType_Walk_Sidestep_Right:
			case ONcAnimType_Walk_Backwards:
			case ONcAnimType_Crouch_Run:
			case ONcAnimType_Crouch_Walk:
			case ONcAnimType_Crouch_Run_Backwards:
			case ONcAnimType_Crouch_Walk_Backwards:
			case ONcAnimType_Crouch_Run_Sidestep_Left:
			case ONcAnimType_Crouch_Run_Sidestep_Right:
			case ONcAnimType_Crouch_Walk_Sidestep_Left:
			case ONcAnimType_Crouch_Walk_Sidestep_Right:
			case ONcAnimType_Run_Stop:
			case ONcAnimType_Walk_Stop:
			case ONcAnimType_Run_Start:
			case ONcAnimType_Walk_Start:
			case ONcAnimType_Run_Backwards_Start:
			case ONcAnimType_Walk_Backwards_Start:
			case ONcAnimType_1Step_Stop:
			case ONcAnimType_Run_Sidestep_Left_Start:
			case ONcAnimType_Run_Sidestep_Right_Start:
				ONrCharacter_Knockdown(NULL, ioCharacter, ONcAnimType_Fly);
			goto exit;
		}
	}

	// if we are in a jump then the default action is to fly
	switch(ioActiveCharacter->curAnimType)
	{
		case ONcAnimType_Fly:
		case ONcAnimType_Jump:
		case ONcAnimType_Jump_Forward:
		case ONcAnimType_Jump_Backward:
		case ONcAnimType_Jump_Left:
		case ONcAnimType_Jump_Right:
	 		animFound = NULL != ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, ONcAnimPriority_Queue , ONcAnimType_Fly);
			if (animFound) goto exit;
		break;
	}

	// if we are in the air then our default action is to fly
	if (ioActiveCharacter->inAirControl.numFramesInAir > 0) {
 		animFound = NULL != ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, ONcAnimPriority_Queue, ONcAnimType_Fly);
		if (animFound) goto exit;

 		animFound = NULL != ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, ONcAnimPriority_Queue, ONcAnimType_Falling_Flail);
		if (animFound) goto exit;

		ONrCharacter_Knockdown(NULL, ioCharacter, ONcAnimType_Fly);
		goto exit;
	}

	if (AI2rEnableIdleAnimation(ioCharacter)) {
		if (ONgGameState->gameTime >= (ioCharacter->lastActiveAnimationTime + ioCharacter->idleDelay)) {
 			animFound = NULL != ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, animPriority, ONcAnimType_Idle);

			// idle delay is random each time we do an idle anim
			ONrCharacter_RecalculateIdleDelay(ioCharacter);
		}
		if (animFound) goto exit;
	}

	// ok optimizations here
	if (ONgFastLookups && !in_bad_varient) {
		UUtBool crouch_down = (inInput->buttonIsDown & LIc_BitMask_Crouch) > 0;

		// if we are crouching to the crouch with the crouch button down just set next anim type as crouch
		if ((ONcAnimType_Crouch == ioActiveCharacter->curAnimType) && (ONcAnimState_Crouch == ioActiveCharacter->nextAnimState) && crouch_down) {
			ioActiveCharacter->nextAnimType = ONcAnimType_Crouch;
			animFound = UUcTrue;
			goto exit;
		}

		// if we are standing to the stand without the crouch button down then just stand
		if ((ONcAnimType_Stand == ioActiveCharacter->curAnimType) && (ONcAnimState_Standing == ioActiveCharacter->nextAnimState) && !crouch_down) {
			ioActiveCharacter->nextAnimType = ONcAnimType_Stand;
			animFound = UUcTrue;
			goto exit;
		}
	}

	animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, standTable, animPriority, ONcNormalMove, NULL);
	if (animFound) goto exit;

	animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, standTable, animPriority, ONcAttackMove, NULL);

	if (animFound) {
		COrConsole_Printf("### help, I could not s/c/p without the fight varient %s->%s",
			TRrAnimation_GetName(oldAnimation), TRrAnimation_GetName(ioActiveCharacter->animation));
		goto exit;
	}

	// we have failed to find any animation, stick the character in the stand
	ONrCharacter_ForceStand(ioCharacter);

exit:
	return animFound;
}

static UUtBool HandleStartSliding(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool animFound = UUcFalse;
	TRtAnimType animType = TRrAnimation_GetType(ioActiveCharacter->animation);
	const ONtMoveLookup table[] = {
		{ LIc_BitMask_Crouch,	0,		ONcAnimType_None,	ONcAnimType_Slide, UUcFalse },
		{ 0,					0,		ONcAnimType_None,	ONcAnimType_None, UUcFalse }
	};

	switch (ioActiveCharacter->curAnimType)
	{
		case ONcAnimType_Run:
		case ONcAnimType_Run_Start:
		case ONcAnimType_Run_Backwards:
		case ONcAnimType_Run_Backwards_Start:
		case ONcAnimType_Run_Sidestep_Left:
		case ONcAnimType_Run_Sidestep_Left_Start:
		case ONcAnimType_Run_Sidestep_Right:
		case ONcAnimType_Run_Sidestep_Right_Start:
			animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, table, ONcAnimPriority_Appropriate, ONcAttackMove, NULL);
		break;
	}

	return animFound;
}


static UUtBool HandleStandingTurn(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	const float turnStart = ONgAimingWidth * M3cDegToRad;
	UUtBool found;
	const float desire = ONrCharacter_GetDesiredFacingOffset(ioCharacter);
	float turnPerFrame = iGetTurnPerFrame(inInput, ioCharacter);

	const ONtMoveLookup turn_left_table[] =
	{
		{ 0, LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Crouch_Turn_Left, UUcFalse },
		{ 0, 0,							ONcAnimType_None,	ONcAnimType_Standing_Turn_Left, UUcFalse },
		{ 0, 0,							ONcAnimType_None,	ONcAnimType_Crouch_Turn_Left, UUcFalse },
		{ 0, 0,							ONcAnimType_None,	ONcAnimType_None, UUcFalse }
	};

	const ONtMoveLookup turn_right_table[] =
	{
		{ 0, LIc_BitMask_Crouch,		ONcAnimType_None,	ONcAnimType_Crouch_Turn_Right, UUcFalse },
		{ 0, 0,							ONcAnimType_None,	ONcAnimType_Standing_Turn_Right, UUcFalse },
		{ 0, 0,							ONcAnimType_None,	ONcAnimType_Crouch_Turn_Right, UUcFalse },
		{ 0, 0,							ONcAnimType_None,	ONcAnimType_None, UUcFalse }
	};

	if ((desire > turnStart) || ioActiveCharacter->pleaseTurnLeft) {
		found = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, turn_left_table, ONcAnimPriority_Appropriate, ONcNormalMove, NULL);
		ioActiveCharacter->pleaseTurnLeft = UUcTrue;
	}
	else if ((desire < -turnStart) || (ioActiveCharacter->pleaseTurnRight)) {
		found = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, turn_right_table, ONcAnimPriority_Appropriate, ONcNormalMove, NULL);
		ioActiveCharacter->pleaseTurnRight = UUcTrue;
	}
	else {
		found = UUcFalse;
	}

	return found;
}

// CB: DoTableLookup checks to see whether DoTableAnimation is going to be able to find an animation, without actually
// going ahead and implementing that animation
static const TRtAnimation *DoTableLookup(const ONtInputState *inInput, ONtCharacter *ioCharacter,
										 ONtActiveCharacter *ioActiveCharacter, const ONtMoveLookup *table, TRtAnimType *outAnimType)
{
	const TRtAnimation *animFound;
	const ONtMoveLookup *curEntry;
	ONtCharacterClass *characterClass = ioCharacter->characterClass;
	TRtAnimationCollection *collection = characterClass->animations;

	UUmAssertReadPtr(inInput, sizeof(*inInput));
	UUmAssertWritePtr(ioCharacter, sizeof(*ioCharacter));
	UUmAssertReadPtr(table, sizeof(*table));

	for(curEntry = table; curEntry->moveType != ONcAnimType_None; curEntry++) {
		if ((inInput->buttonIsDown & curEntry->moveMask) != curEntry->moveMask) { continue; }
		if ((curEntry->oldType != ONcAnimType_None) && (ioActiveCharacter->lastAttack != curEntry->oldType)) { continue; }
		if ((inInput->buttonWentDown & curEntry->newMask) != curEntry->newMask) { continue; }

		animFound = TRrCollection_Lookup(collection, curEntry->moveType, ioActiveCharacter->nextAnimState, ioActiveCharacter->animVarient);
		if (animFound != NULL) {
			if (outAnimType != NULL) {
				*outAnimType = curEntry->moveType;
			}

			return animFound;
		}
	}

	if (outAnimType != NULL) {
		*outAnimType = ONcAnimType_None;
	}
	return NULL;
}

static UUtBool DoTableAnimation(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter,
								const ONtMoveLookup *table, ONtAnimPriority priority, ONtAttackMove attackMove, const TRtAnimation **outAnimation)
{
	TRtAnimType type;
	const TRtAnimation *animFound;
	const ONtMoveLookup *curEntry;
	UUtBool call_fight_mode = ONcAttackMove == attackMove;
	TRtAnimVarient originalVarient = ioActiveCharacter->animVarient;

	animFound = UUcFalse;

	UUmAssertReadPtr(inInput, sizeof(*inInput));
	UUmAssertWritePtr(ioCharacter, sizeof(*ioCharacter));
	UUmAssertReadPtr(table, sizeof(*table));

	if (ONcAttackMove == attackMove)
	{
		ioActiveCharacter->animVarient |= ONcAnimVarientMask_Fight;
	}

	for(curEntry = table; curEntry->moveType != ONcAnimType_None; curEntry++) {
		if ((inInput->buttonIsDown & curEntry->moveMask) != curEntry->moveMask) { continue; }
		if ((curEntry->oldType != ONcAnimType_None) && (ioActiveCharacter->lastAttack != curEntry->oldType)) { continue; }
		if ((inInput->buttonWentDown & curEntry->newMask) != curEntry->newMask) { continue; }

		type = curEntry->moveType;

		if ((ONcAttackMove == attackMove) && (curEntry->backInTime) && (ONcChar_Player == ioCharacter->charType)) {
			animFound = ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, (ONtAnimPriority)(priority | ONcAnimPriority_BackInTime), type);
		}
		else {
			animFound = ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, priority, type);
		}

		if (animFound) {
			break;
		}
	}

	// restore the original varient
	ioActiveCharacter->animVarient = originalVarient;

	// call fight mode if we got an animation that had varient fight
	if (call_fight_mode) {
		if (NULL != animFound) {
			TRtAnimVarient varient = TRrAnimation_GetVarient(animFound);

			if (varient & ONcAnimVarientMask_Fight) {
				ONrCharacter_FightMode(ioCharacter);
			}
		}
	}

	if (outAnimation != NULL) {
		*outAnimation = animFound;
	}

	return NULL != animFound;
}

static UUtBool HandleStartAttack(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool any_inventory_management_is_active = ONrOverlay_IsActive(ioActiveCharacter->overlay + ONcOverlayIndex_InventoryManagement);
	UUtBool animFound = UUcFalse;

	if ((!any_inventory_management_is_active) && (!ioActiveCharacter->pleaseFire) && (!ioActiveCharacter->pleaseFireAlternate)) {
		if (0 != inInput->buttonWentDown)
		{
			animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, ONgComboTable, ONcAnimPriority_Appropriate, ONcAttackMove, NULL);
		}
	}

	return animFound;
}

static UUtBool HandleStartJumping(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool animFound;
	TRtAnimType animType = TRrAnimation_GetType(ioActiveCharacter->animation);
	const ONtMoveLookup table[] = {
		{ 0,	LIc_BitMask_Forward,	ONcAnimType_None,	ONcAnimType_Jump_Forward, UUcFalse },
		{ 0,	LIc_BitMask_StepLeft,	ONcAnimType_None,	ONcAnimType_Jump_Left, UUcFalse },
		{ 0,	LIc_BitMask_StepRight,	ONcAnimType_None,	ONcAnimType_Land_Right, UUcFalse },
		{ 0,	LIc_BitMask_Backward,	ONcAnimType_None,	ONcAnimType_Jump_Backward, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Jump, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_None, UUcFalse },
	};

	if (!(inInput->buttonWentDown & LIc_BitMask_Jump))
	{
		return UUcFalse;
	}

	switch(animType)
	{
		case ONcAnimType_Jump_Forward:
		case ONcAnimType_Jump_Left:
		case ONcAnimType_Land_Right:
		case ONcAnimType_Jump_Backward:
		case ONcAnimType_Jump:
			return UUcFalse;
		break;
	}

	ONrCharacter_BuildJump(ioCharacter, ioActiveCharacter);

	animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, table, ONcAnimPriority_Appropriate, ONcNormalMove, NULL);

	if (!animFound && (ioActiveCharacter != NULL)) {
		// couldn't find a jump, cancel the build
		ioActiveCharacter->inAirControl.buildValid = UUcFalse;
	}

	return animFound;
}

static void ONiGameState_FindPickupItems(ONtCharacter *ioCharacter, WPtWeapon **outWeapon,
										WPtPowerup **outPowerup, UUtBool *outPowerupInventoryFull)
{
	float min_y = ioCharacter->location.y + ONcStartPickupMinY;
	float max_y = ioCharacter->location.y + ONcStartPickupMaxY;

	*outPowerup = WPrFind_Powerup_XZ(ioCharacter, &ioCharacter->location, min_y, max_y, NULL, ONcStartPickupRadius, outPowerupInventoryFull);
	*outWeapon  = WPrFind_Weapon_XZ (			  &ioCharacter->location, min_y, max_y, NULL, ONcStartPickupRadius);
}

static UUtBool HandleStartPickup(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	float min_y = ioCharacter->location.y + ONcStartPickupMinY;
	float max_y = ioCharacter->location.y + ONcStartPickupMaxY;
	UUtBool found_animation = UUcFalse;
	TRtAnimType pickup_anim_type = ONcAnimType_None;
	WPtWeapon *weapon_on_ground;
	WPtPowerup *powerup_on_ground;
	WPtWeapon *stowed_weapon;
	UUtBool can_pickup_weapon;
	UUtBool can_pickup_powerup;
	UUtBool powerup_inventory_full;
	UUtUns16 slot_number;

	if (!(inInput->buttonWentDown & LIc_BitMask_Swap)) {
		goto exit;
	}

	// can't pick up or holster stuff while running a reload or holster animation
	if (ONrOverlay_IsActive(ioActiveCharacter->overlay + ONcOverlayIndex_InventoryManagement)) {
		goto exit;
	}

	// step 1: test what kind of pickup animation to run
	// step 2: try running that pickup


	// Hitting pickup key(s), so attempt a pickup
	ONiGameState_FindPickupItems(ioCharacter, &weapon_on_ground, &powerup_on_ground, &powerup_inventory_full);
	stowed_weapon = WPrSlot_FindLastStowed(&ioCharacter->inventory, WPcPrimarySlot, &slot_number);
	can_pickup_weapon = (NULL != weapon_on_ground) && (NULL == stowed_weapon);
	can_pickup_powerup = NULL != powerup_on_ground;

	if (!can_pickup_powerup && !can_pickup_weapon) {

		// if we can't pick anything up then swap weapons
		if (ioCharacter->inventory.weapons[0] != NULL) {
			ONrCharacter_Holster(ioCharacter, ioActiveCharacter, UUcFalse);
		}
		else if (stowed_weapon != NULL) {
			ONrCharacter_Draw(ioCharacter, ioActiveCharacter, NULL);
		}
	}
	else if (TRrAnimation_TestFlag(ioActiveCharacter->animation, ONcAnimFlag_Can_Pickup)) {
		if (can_pickup_weapon) {
			ONrCharacter_PickupWeapon(ioCharacter, weapon_on_ground, UUcFalse);
		} else if (can_pickup_powerup) {
			ONrCharacter_PickupPowerup(ioCharacter, powerup_on_ground);
		}
	}
	else {
		if (can_pickup_powerup) {
			if (powerup_on_ground->position.y > (ioCharacter->location.y + 9.f)) {
				pickup_anim_type = ONcAnimType_Pickup_Object_Mid;
			}
			else {
				pickup_anim_type = ONcAnimType_Pickup_Object;
			}
			ioCharacter->pickup_target = powerup_on_ground;
		}
		else if (can_pickup_weapon) {
			const WPtWeaponClass *weapon_class = WPrGetClass(weapon_on_ground);
			M3tPoint3D weapon_location_on_ground;
			UUtBool pickup_weapon_mid;

			WPrGetPosition(weapon_on_ground, &weapon_location_on_ground);
			pickup_weapon_mid = weapon_location_on_ground.y > (ioCharacter->location.y + 9.f);

			if (weapon_class->flags & WPcWeaponClassFlag_TwoHanded) {
				pickup_anim_type = pickup_weapon_mid ? ONcAnimType_Pickup_Rifle_Mid : ONcAnimType_Pickup_Rifle;
			}
			else {
				pickup_anim_type = pickup_weapon_mid ? ONcAnimType_Pickup_Pistol_Mid : ONcAnimType_Pickup_Pistol;
			}

			ioCharacter->pickup_target = weapon_on_ground;
		}
	}

	if (pickup_anim_type != ONcAnimType_None) {
		const TRtAnimation *pickup_animation;

		pickup_animation = ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, ONcAnimPriority_Appropriate, pickup_anim_type);

		found_animation = (NULL != pickup_animation);
	} else if (powerup_inventory_full) {
		if (ioCharacter == ONgGameState->local.playerCharacter) {
			// our inventory is full, and there is a powerup here that we can't pick up because of it
			ONrGameState_EventSound_Play(ONcEventSound_InventoryFail, NULL);
		}
	}

exit:
	return found_animation;
}


static UUtBool HandleStartLanding(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool animFound = UUcFalse;

	const ONtMoveLookup land_hard_table[] = {
		{ 0,	LIc_BitMask_Forward,	ONcAnimType_None,	ONcAnimType_Land_Forward_Hard, UUcFalse },
		{ 0,	LIc_BitMask_StepLeft,	ONcAnimType_None,	ONcAnimType_Land_Left_Hard, UUcFalse },
		{ 0,	LIc_BitMask_StepRight,	ONcAnimType_None,	ONcAnimType_Land_Right_Hard, UUcFalse },
		{ 0,	LIc_BitMask_Backward,	ONcAnimType_None,	ONcAnimType_Land_Back_Hard, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Land_Hard, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_None, UUcFalse }
	};

	const ONtMoveLookup land_table[] = {
		{ 0,	LIc_BitMask_Forward,	ONcAnimType_None,	ONcAnimType_Land_Forward, UUcFalse },
		{ 0,	LIc_BitMask_StepLeft,	ONcAnimType_None,	ONcAnimType_Land_Left, UUcFalse },
		{ 0,	LIc_BitMask_StepRight,	ONcAnimType_None,	ONcAnimType_Land_Right, UUcFalse },
		{ 0,	LIc_BitMask_Backward,	ONcAnimType_None,	ONcAnimType_Land_Back, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_Land, UUcFalse },
		{ 0,	0,						ONcAnimType_None,	ONcAnimType_None, UUcFalse }
	};

	if (!(ioActiveCharacter->pleaseLand)) {
		return UUcFalse;
	}

	if (ioActiveCharacter->pleaseLandHard) {
		TRtAnimState old_anim_state = ioActiveCharacter->nextAnimState;

		animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, land_hard_table, ONcAnimPriority_Force, ONcNormalMove, NULL);

		if (!animFound) {
			ioActiveCharacter->nextAnimState = ONcAnimState_Falling;
			animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, land_hard_table, ONcAnimPriority_Force, ONcNormalMove, NULL);
		}

		if (!animFound) {
			ioActiveCharacter->nextAnimState = ONcAnimState_Flying;
			animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, land_hard_table, ONcAnimPriority_Force, ONcNormalMove, NULL);
		}

		if (!animFound) {
			ioActiveCharacter->nextAnimState = old_anim_state;
		}
	}

	if (!animFound) {
		animFound = DoTableAnimation(inInput, ioCharacter, ioActiveCharacter, land_table, ONcAnimPriority_Force, ONcNormalMove, NULL);
	}

	ioActiveCharacter->pleaseLand = UUcFalse;
	ioActiveCharacter->pleaseLandHard = UUcFalse;

	return animFound;
}

static UUtBool HandleStartFalling(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool anim_found = UUcFalse;

	if (0 == ioActiveCharacter->inAirControl.numFramesInAir) {
		goto exit;
	}

	if ((ioActiveCharacter->inAirControl.pickupReleaseTimer > 0) || (ioActiveCharacter->inAirControl.numFramesInAir > 90)) {
		const TRtAnimation *flail_animation;

		flail_animation = ONrCharacter_DoAnimation(ioCharacter, ioActiveCharacter, ONcAnimPriority_Force, ONcAnimType_Falling_Flail);
		anim_found = flail_animation != NULL;
	}


exit:
	return anim_found;
}

void ONrDesireFacingModifier(ONtCharacter *inCharacter, float inDesiredFacingModifier)
{
	float delta;
	float inFacing = inCharacter->facing;
	float inFacingModifier = inCharacter->facingModifier;

	if (!ONrCharacter_IsPlayingFilm(inCharacter) && (inCharacter->flags & ONcCharacterFlag_AIMovement)) {
		// AI2 characters do their own movement and do not
		// require facingModifier - in fact it will actively break their code
		inDesiredFacingModifier = 0;
	}

	UUmAssert(inDesiredFacingModifier >= (-M3c2Pi));
	UUmAssert(inDesiredFacingModifier <= (M3c2Pi));
	UUmAssertTrigRange(inCharacter->facing);

	delta = inDesiredFacingModifier - inCharacter->facingModifier;

	// adjust facing modifier and facing
	inCharacter->facingModifier += delta;
	ONrCharacter_AdjustDesiredFacing(inCharacter, delta);

	UUmAssertTrigRange(inCharacter->facing);

	return;
}

static void ONiWeaponCheat(void)
{
	ONtCharacter *localCharacter = ONgGameState->local.playerCharacter;
	UUtUns32 itr, num_classes;
	WPtWeaponClass *weapon_classes[64];
	WPtWeapon *weapon;
	M3tPoint3D position;
	float theta;
	UUtError error;

	// get a list of pointers to the weapon classes
	error = TMrInstance_GetDataPtr_List(WPcTemplate_WeaponClass, 64, &num_classes, weapon_classes);
	if (error != UUcError_None) {
		return;
	}

	for (itr = 0; itr < num_classes; itr++) {
		weapon = WPrNew(weapon_classes[itr], WPcCheatWeapon);
		if (weapon == NULL)
			continue;

		theta = (itr * M3c2Pi) / num_classes;
		position = localCharacter->actual_position;
		position.x += 8.0f * MUrSin(theta);
		position.y += 10.0f;
		position.z += 8.0f * MUrCos(theta);

		theta += M3cHalfPi;
		UUmTrig_ClipHigh(theta);
		WPrSetPosition(weapon, &position, theta);
		WPrSetTimers(weapon, 240, 0);
	}
}

static void ONiCycle_Weapon(const ONtInputState *inInput)
{
	ONtCharacter *localCharacter = ONgGameState->local.playerCharacter;
	ONtActiveCharacter *localActiveCharacter = ONrGetActiveCharacter(localCharacter);
	UUtError				error;
	WPtWeaponClass			*weapon_class[64];
	UUtUns32				num_classes;
	UUtUns32				i;

	UUmAssertReadPtr(localActiveCharacter, sizeof(*localActiveCharacter));

	// can't do this while running a reload or holster animation
	if (ONrOverlay_IsActive(localActiveCharacter->overlay + ONcOverlayIndex_InventoryManagement)) {
		return;
	}

	// get a list of pointers to the weapon classes
	error =
		TMrInstance_GetDataPtr_List(
			WPcTemplate_WeaponClass,
			64,
			&num_classes,
			weapon_class);

	// find the weapon class of the weapon currently in use by the character
	if (localCharacter->inventory.weapons[0])
	{
		for (i = 0; i < num_classes; i++)
		{
			if (weapon_class[i] == WPrGetClass(localCharacter->inventory.weapons[0])) { break; }
		}
	}
	else
	{
		i = 0;
	}

	// pick the index of the next weapon to use
	if (inInput->buttonIsDown & LIc_BitMask_Crouch)
	{
		i--;

		if (i >= num_classes)
		{
			i = num_classes - 1;
		}
	}
	else
	{
		i++;

		if (i >= num_classes)
		{
			i = 0;
		}
	}

	// create a new weapon for use
	ONrCharacter_UseNewWeapon(localCharacter, weapon_class[i]);

	return;
}

static void ONiCycle_Class(const ONtInputState *inInput)
{
	UUtError error;
	ONtCharacter *localCharacter = ONgGameState->local.playerCharacter;
	ONtCharacterClass *newClass;
	UUtUns16 numClasses = (UUtUns16)TMrInstance_GetTagCount(TRcTemplate_CharacterClass);

	if (localCharacter == NULL) return;

	if (inInput->buttonIsDown & LIc_BitMask_Crouch) {
		localCharacter->characterClassNumber += numClasses - 1;
	}
	else {
		localCharacter->characterClassNumber += 1;
	}

	if (numClasses > 0) {
		localCharacter->characterClassNumber = localCharacter->characterClassNumber % numClasses;

		error = TMrInstance_GetDataPtr_ByNumber(TRcTemplate_CharacterClass, localCharacter->characterClassNumber, &newClass);

		if ((newClass != NULL) && (UUcError_None == error)) {
			ONrCharacter_SetCharacterClass(localCharacter, newClass);

			COrConsole_Printf("character class %d %s",
				localCharacter->characterClassNumber,
				TMrInstance_GetInstanceName(newClass));
		}
	}

	return;
}

typedef enum ONtCheat {
	ONcCheat_ChangeCharacters,
	ONcCheat_Invincible,
	ONcCheat_Omnipotent,
	ONcCheat_Unstoppable,
	ONcCheat_FatLoot,
	ONcCheat_GlassFurniture,
	ONcCheat_WinLevel,
	ONcCheat_LoseLevel,
	ONcCheat_BigHead,
	ONcCheat_MiniMe,
	ONcCheat_SuperAmmo,
	ONcCheat_DeveloperAccess,
	ONcCheat_LastManStanding,
	ONcCheat_GatlingGuns,
	ONcCheat_DaodanPower,
	ONcCheat_Behemoth,
	ONcCheat_Regeneration,
	ONcCheat_Invisibility,
	ONcCheat_SpawnWeapons,
	ONcCheat_FistsOfFury,
	ONcCheat_UltraMode,
	ONcCheat_SlowMotion
} ONtCheat;

typedef struct ONtCheatTable {
	const char *cheat_string;
	const char *cheat_message1;
	const char *cheat_message2;
	ONtCheat cheat_code;
} ONtCheatTable;

UUtBool ONrCheater(ONtCheat inCheat)
{
	ONtCharacter *player_character = ONgGameState->local.playerCharacter;

	if (NULL == player_character) {
		goto exit;
	}

	switch(inCheat)
	{
		case ONcCheat_ChangeCharacters:
			ONgChangeCharacters = !ONgChangeCharacters;
			return ONgChangeCharacters;

		case ONcCheat_Invincible:
			ONgPlayerInvincible = !ONgPlayerInvincible;
			return ONgPlayerInvincible;

		case ONcCheat_Omnipotent:
			ONgPlayerOmnipotent = !ONgPlayerOmnipotent;
			return ONgPlayerOmnipotent;

		case ONcCheat_Unstoppable:
			ONgPlayerUnstoppable = !ONgPlayerUnstoppable;
			return ONgPlayerUnstoppable;

		case ONcCheat_FatLoot:
			player_character->inventory.ammo = 6;
			player_character->inventory.cell = 6;
			player_character->inventory.hypo = 6;
			return UUcTrue;

		case ONcCheat_GlassFurniture:
			P3gFurnitureBreakable = !P3gFurnitureBreakable;
			return P3gFurnitureBreakable;

		case ONcCheat_WinLevel:
			ONgGameState->victory = ONcWin;
			return UUcTrue;

		case ONcCheat_LoseLevel:
			ONgGameState->victory = ONcLose;
			return UUcTrue;

		case ONcCheat_BigHead:
			{
				extern UUtBool ONgBigHead;

				ONgBigHead = !ONgBigHead;
				return ONgBigHead;
			}

		case ONcCheat_MiniMe:
			if (player_character->scale >= 0.75f) {
				player_character->scale = 0.25f;
				return UUcTrue;
			}
			else {
				player_character->scale = 1.f;
				return UUcFalse;
			}

		case ONcCheat_SuperAmmo:
			if (player_character->flags & ONcCharacterFlag_InfiniteAmmo) {
				player_character->flags &= ~ONcCharacterFlag_InfiniteAmmo;
				return UUcFalse;
			} else {
				player_character->flags |= ONcCharacterFlag_InfiniteAmmo;
				return UUcTrue;
			}

		case ONcCheat_DeveloperAccess:
			ONgDeveloperAccess = !ONgDeveloperAccess;
			if (ONgDeveloperAccess) {
				LIrInitializeDeveloperKeys();
				COrConsole_SetPriority(COcPriority_Console);
			} else {
				LIrUnbindDeveloperKeys();
				COrConsole_SetPriority(COcPriority_Subtitle);
			}
			return ONgDeveloperAccess;

		case ONcCheat_LastManStanding:
			AI2gLastManStanding = !AI2gLastManStanding;
			return AI2gLastManStanding;

		case ONcCheat_GatlingGuns:
			WPgGatlingCheat = !WPgGatlingCheat;
			return WPgGatlingCheat;

		case ONcCheat_DaodanPower:
			if (player_character->external_super_amount > 1.0f) {
				player_character->external_super_amount = 0.0f;
				player_character->flags &= ~ONcCharacterFlag_BossShield;
				return UUcFalse;
			} else {
				player_character->external_super_amount = 10.0f;
				player_character->flags |= ONcCharacterFlag_BossShield;
				return UUcTrue;
			}

		case ONcCheat_Behemoth:
			ONgPlayerKnockdownResistant = !ONgPlayerKnockdownResistant;
			if (ONgPlayerKnockdownResistant) {
				player_character->scale = 1.8f;
			} else {
				// reset player scale
				player_character->scale =
					UUmRandomRangeFloat(player_character->characterClass->scaleLow, player_character->characterClass->scaleHigh);
			}
			return ONgPlayerKnockdownResistant;

		case ONcCheat_Regeneration:
			WPgRegenerationCheat = !WPgRegenerationCheat;
			return WPgRegenerationCheat;

		case ONcCheat_Invisibility:
			if (player_character->inventory.invisibilityRemaining == (UUtUns16) -1) {
				player_character->inventory.invisibilityRemaining = 0;
				return UUcFalse;
			} else {
				player_character->inventory.invisibilityRemaining = (UUtUns16) -1;
				return UUcTrue;
			}

		case ONcCheat_SpawnWeapons:
			ONiWeaponCheat();
			return UUcTrue;

		case ONcCheat_FistsOfFury:
			ONgPlayerFistsOfFury = !ONgPlayerFistsOfFury;
			return ONgPlayerFistsOfFury;

		case ONcCheat_UltraMode:
			AI2gUltraMode = !AI2gUltraMode;
			return AI2gUltraMode;

		case ONcCheat_SlowMotion:
			ONgGameState->local.slowMotionEnable = !(ONgGameState->local.slowMotionEnable);
			return ONgGameState->local.slowMotionEnable;
	}

exit:
	return UUcFalse;
}

void ONrGameState_HandleCheats(const LItInputEvent *inEvent)
{
#define MAX_CHEAT_STRING 20
	static char cheat_buffer[MAX_CHEAT_STRING + 1] = { 0 };
	static ONtCheatTable ONgCheatTable[] =
	{
		{ "shapeshifter",		"Change Characters Enabled",	"Change Characters Disabled",	ONcCheat_ChangeCharacters },
		{ "liveforever",		"Invincibility Enabled",		"Invincibility Disabled",		ONcCheat_Invincible },
		{ "touchofdeath",		"Omnipotence Enabled",			"Omnipotence Disabled",			ONcCheat_Omnipotent },
		{ "canttouchthis",		"Unstoppable Enabled",			"Unstoppable Disabled",			ONcCheat_Unstoppable },
		{ "fatloot",			"Fat Loot Received",			NULL,							ONcCheat_FatLoot },
		{ "glassworld",			"Glass Furniture Enabled",		"Glass Furniture Disabled",		ONcCheat_GlassFurniture },
		{ "winlevel",			"Instantly Win Level",			NULL,							ONcCheat_WinLevel },
		{ "loselevel",			"Instantly Lose Level",			NULL,							ONcCheat_LoseLevel },
		{ "bighead",			"Big Head Enabled",				"Big Head Disabled",			ONcCheat_BigHead },
		{ "minime",				"Mini Mode Enabled",			"Mini Mode Disabled",			ONcCheat_MiniMe },
		{ "superammo",			"Super Ammo Mode Enabled",		"Super Ammo Mode Disabled",		ONcCheat_SuperAmmo },
		{ "reservoirdogs",		"Last Man Standing Enabled",	"Last Man Standing Disabled",	ONcCheat_LastManStanding },
		{ "roughjustice",		"Gatling Guns Enabled",			"Gatling Guns Disabled",		ONcCheat_GatlingGuns },
		{ "chenille",			"Daodan Power Enabled",			"Daodan Power Disabled",		ONcCheat_DaodanPower },
		{ "behemoth",			"Godzilla Mode Enabled",		"Godzilla Mode Disabled",		ONcCheat_Behemoth },
		{ "elderrune",			"Regeneration Enabled",			"Regeneration Disabled",		ONcCheat_Regeneration },
		{ "moonshadow",			"Phase Cloak Enabled",			"Phase Cloak Disabled",			ONcCheat_Invisibility },
		{ "munitionfrenzy",		"Weapons Locker Created",		NULL,							ONcCheat_SpawnWeapons },
		{ "fistsoflegend",		"Fists Of Legend Enabled",		"Fists Of Legend Disabled",		ONcCheat_FistsOfFury },
		{ "killmequick",		"Ultra Mode Enabled",			"Ultra Mode Disabled",			ONcCheat_UltraMode },
		{ "carousel",			"Slow Motion Enabled",			"Slow Motion Disabled",			ONcCheat_SlowMotion },

		// CB: REMOVE THIS for last final candidate(s) and GM
#if THE_DAY_IS_MINE
		{ "thedayismine",		"Developer Access Enabled",		"Developer Access Disabled",	ONcCheat_DeveloperAccess },
#endif
		{ NULL, NULL, NULL, 0 }
	};

	if (!ONrPersist_GetWonGame()) {
		goto exit;
	}

	if (LIcInputEvent_KeyDown != inEvent->type) {
		goto exit;
	}

	if ((inEvent->key < 'a') || (inEvent->key > 'z')) {
		goto exit;
	}

	if (inEvent->key) {
		UUtInt32 start;
		UUrMemory_MoveOverlap(cheat_buffer + 1, cheat_buffer, (MAX_CHEAT_STRING - 1));
		cheat_buffer[MAX_CHEAT_STRING - 1] = (char) inEvent->key;

		// COrConsole_Printf("cheat = %s", cheat_buffer);

		for(start = 0; start < MAX_CHEAT_STRING; start++)
		{
			ONtCheatTable *test_cheat;
			const char *string = cheat_buffer + start;

			for(test_cheat = ONgCheatTable; test_cheat->cheat_string != NULL; test_cheat++)
			{
				if (UUrString_HasPrefix(string, test_cheat->cheat_string)) {
					UUtBool enabled;
					const char *message;

					//COrConsole_Printf("cheater %s", test_cheat->cheat_string);
					enabled = ONrCheater(test_cheat->cheat_code);
					message = (enabled) ? test_cheat->cheat_message1 : test_cheat->cheat_message2;

					if (message != NULL) {
						ONrPauseScreen_OverrideMessage(message);
					}

					UUrMemory_Clear(cheat_buffer, sizeof(cheat_buffer));
				}
			}
		}
	}

exit:
	return;
}

UUtBool
ONrGameState_DropFlag(
	const M3tPoint3D		*inLocation,
	const float				inDesiredFacing,
	UUtBool					inPrintMsg,
	UUtUns16				*outID)
{
	OBJtObject				*object;
	M3tPoint3D				rotation;
	UUtError				error;

	// calculate the facing
	MUmVector_Set(rotation, 0.0f, inDesiredFacing * M3cRadToDeg, 0.0f);

	// create the new object
	error = OBJrObject_New(OBJcType_Flag, inLocation, &rotation, NULL);
	if (error != UUcError_None) {
		if (inPrintMsg) {
			COrConsole_Printf("unable to create flag; maybe flags.bin is not checked out");
		}
		return UUcFalse;
	}

	object = OBJrSelectedObjects_GetSelectedObject(0);
	if (object)
	{
		OBJtOSD_Flag *flag_osd;

		if (inPrintMsg) {
			char				name[OBJcMaxNameLength + 1];

			OBJrObject_GetName(object, name, OBJcMaxNameLength);
			COrConsole_Printf("flag %s added", name);
		}

		UUmAssert(object->object_type == OBJcType_Flag);
		flag_osd = (OBJtOSD_Flag *) object->object_data;

		if (outID) *outID = flag_osd->id_number;

		return UUcTrue;
	} else {
		return UUcFalse;
	}
}

static void
DeleteFlag(
	const M3tPoint3D		*inLocation)
{
	ONtFlag					flag;
	float					max_distance_squared = UUmSQR(15.f);
	UUtBool					found;

	// get the closest flag
	found = ONrLevel_Flag_Location_To_Flag(inLocation, &flag);
	if (found == UUcFalse)
	{
		COrConsole_Printf("could not find flag");
		return;
	}

	if (MUrPoint_Distance_Squared(inLocation, &flag.location) > max_distance_squared)
	{
		COrConsole_Printf("not close enough to flag");
		return;
	}

	ONrLevel_Flag_Delete(flag.idNumber);
}

#if TOOL_VERSION
static void WriteCameraRecord(void)
{
	AXtHeader *pHeader = ONgGameState->local.cameraRecording;
	BFtFile *file;
	BFtFile *textFile;
	UUtError error;

	if (NULL == pHeader) { return; }

	{
		UUtInt32 fileRefItr;
		BFtFileRef *fileRef;

		for(fileRefItr = 0; fileRefItr < 100; fileRefItr++)
		{
			char fileString[128];
			sprintf(fileString, "cameraRecord%d.eva", fileRefItr);

			error = BFrFileRef_MakeFromName(fileString, &fileRef);
			UUmAssert(UUcError_None == error);

			if (BFrFileRef_FileExists(fileRef)) {
				BFrFileRef_Dispose(fileRef);
				fileRef = NULL;
				continue;
			}

			error = BFrFile_Open(fileRef, "w", &file);
			UUmAssert(UUcError_None == error);

			BFrFileRef_Dispose(fileRef);

			// camera info file
			sprintf(fileString, "cameraInfo%d.txt", fileRefItr);

			error = BFrFileRef_MakeFromName(fileString, &fileRef);
			UUmAssert(UUcError_None == error);

			if (BFrFileRef_FileExists(fileRef)) {
				BFrFile_Delete(fileRef);
			}

			error = BFrFile_Open(fileRef, "w", &textFile);
			UUmAssert(UUcError_None == error);


			break;
		}
	}

	// write animation data
	BFrFile_Write(file, sizeof(*pHeader), pHeader);
	BFrFile_Write(file, sizeof(AXtNode), pHeader->nodes);
	BFrFile_Write(file, sizeof(M3tMatrix4x3) * pHeader->numFrames, pHeader->nodes[0].matricies);

	BFrFile_Close(file);

	if (ONgGameState->local.cameraMark != -1) {
		BFrFile_Printf(textFile, "ghost animation ON"UUmNL);
		BFrFile_Printf(textFile, "ghost animation frame %d"UUmNL, ONgGameState->local.cameraMark);
	}
	else {
		BFrFile_Printf(textFile, "ghost animation OFF"UUmNL);
	}

	BFrFile_Printf(textFile, "camera has %d frames"UUmNL, pHeader->numFrames);

	BFrFile_Close(textFile);

	return;
}
#endif

// used for changing how some debugging display timers work
UUtBool ONrGameState_IsSingleStep(void)
{
	return ONgSingleStep;
}

#if TOOL_VERSION
static void ONrGameState_HandleUtilityInput_Tool(const ONtInputState *inInput)
{
	if (ONrDebugKey_WentDown(ONcDebugKey_Camera_Record)) {
		if (!ONgGameState->local.cameraRecord) {
			COrConsole_Printf("camera recording");
			ONgGameState->local.cameraRecord = UUcTrue;

			ONgGameState->local.cameraMark = -1;

			{
				AXtHeader *pAXHeader;

				ONgGameState->local.cameraActive = UUcFalse;
				ONgGameState->local.cameraRecording = UUrMemory_Block_NewClear(sizeof(AXtHeader));
				ONgGameState->local.cameraRecording->nodes = UUrMemory_Block_NewClear(sizeof(AXtNode));

				pAXHeader = ONgGameState->local.cameraRecording;

				pAXHeader->stringENVA[0] = 'E';
				pAXHeader->stringENVA[0] = 'N';
				pAXHeader->stringENVA[0] = 'V';
				pAXHeader->stringENVA[0] = 'A';

				pAXHeader->version = 0;
				pAXHeader->numNodes = 1;
				pAXHeader->numFrames = 0;
				pAXHeader->startFrame = 0;
				pAXHeader->endFrame = -1;
				pAXHeader->ticksPerFrame = 1;
				pAXHeader->startTime = 0;
				pAXHeader->endTime = 0;

				strcpy(pAXHeader->nodes[0].name, "camera");
				strcpy(pAXHeader->nodes[0].parentName, "<none>");
				pAXHeader->nodes[0].parentNode = NULL;
				pAXHeader->nodes[0].sibling = 0;
				pAXHeader->nodes[0].child = 0;
				pAXHeader->nodes[0].flags = 0;
				pAXHeader->nodes[0].objectTM = MUgIdentityMatrix;
				pAXHeader->nodes[0].matricies = NULL;
			}
		}
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_Camera_Stop)) {
		if (ONgGameState->local.cameraRecord) {
			COrConsole_Printf("stop camera recording");
			ONgGameState->local.cameraRecord = UUcFalse;

			WriteCameraRecord();
		}
		else if ((ONgGameState->local.cameraRecording) && (ONgGameState->local.cameraActive)) {
			COrConsole_Printf("stop camera playback");
			ONgGameState->local.cameraActive = UUcFalse;
			ONgGameState->local.cameraPlay = UUcFalse;
		}
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_Camera_Play)) {
		if ((!ONgGameState->local.cameraRecord) && (ONgGameState->local.cameraRecording) && (ONgGameState->local.cameraRecording->numFrames > 0)) {
			if (!ONgGameState->local.cameraActive) {
				ONgGameState->local.cameraPlay = UUcTrue;
				ONgGameState->local.cameraActive = UUcTrue;
				ONgGameState->local.cameraFrame = 0;
			}
			else {
				ONgGameState->local.cameraPlay = !ONgGameState->local.cameraPlay;
			}

			if (ONgGameState->local.cameraPlay) {
				COrConsole_Printf("camera playback");
			}
			else {
				COrConsole_Printf("camera pause playback");
			}
		}
	}
}
#endif

void ONrGameState_HandleUtilityInput(const ONtInputState*	inInput)
{
	/*
	 *
	 * DO NOT REMOVE THIS FUNCTIONALITY WITHOUT CHECKING AROUND TO
	 * MAKE SURE IT IS NO LONGER REQUIRED
	 *
	 * THANK YOU
	 *
	 */

	ONtCharacter *localCharacter = ONgGameState->local.playerCharacter;
	ONtActiveCharacter *localActiveCharacter = ONrGetActiveCharacter(localCharacter);
	static UUtUns32 class_time = 0;

	UUmAssertReadPtr(localActiveCharacter, sizeof(*localActiveCharacter));

	if (inInput->buttonWentDown & LIc_BitMask_ScreenShot) {
		iScreenShot(ONgScreenShotReduceAmount);
	}

#if SHIPPING_VERSION
	if (ONgChangeCharacters) {
		if (ONrDebugKey_WentDown(ONcDebugKey_ChangeCharacters)) {
			ONiCycle_Class(inInput);

			class_time = ONgGameState->gameTime + 30;
		}
	}
#endif

	if (!ONgDeveloperAccess) { return; }

	if (inInput->buttonWentDown & LIc_BitMask_F8) {
		ONiCycle_Class(inInput);

		class_time = ONgGameState->gameTime + 30;
	}
	else if (inInput->buttonIsDown & LIc_BitMask_F8) {
		if (ONgGameState->gameTime > class_time)
		{
			ONiCycle_Class(inInput);
			class_time += 5;
		}
	}

	if (inInput->buttonWentDown & LIc_BitMask_F6) {
		AI2rSmite(NULL, UUcFalse);
	}

	if (inInput->buttonWentDown & LIc_BitMask_F7) {
		if (inInput->buttonIsDown & LIc_BitMask_Action) {
			ONrCharacter_Knockdown(NULL, localCharacter, ONcAnimType_Knockdown_Head);
		}
		else {
			ONiCycle_Weapon(inInput);
		}
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_SingleStepMode) &&
		ONrPlatform_IsForegroundApp())
	{
		ONgSingleStep = !ONgSingleStep;
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_Camera_Mode)) {
		if (CAcMode_Manual == CAgCamera.mode) {
			if (CAgCamera.modeData.manual.useMouseLook) {
				CArFollow_Enter();
			}
			else {
				CAgCamera.modeData.manual.useMouseLook = UUcTrue;
			}
		}
		else {
			CArManual_Enter();
		}
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_ToggleActiveCamera)) {
		ONgCurrentCamera = (ONgCurrentCamera + 1) % 2;

		ONgActiveCamera = ONgInternalCamera[ONgCurrentCamera];
		M3rCamera_SetActive(ONgActiveCamera);
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_AddFlag)) {
		ONrGameState_DropFlag(&localCharacter->location, localCharacter->desiredFacing,
								UUcFalse, NULL);
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_DropFlagAndAddWaypoint)) {
		UUtBool window_visible = OWrOniWindow_Visible();

		if (!window_visible) {
			OWrOniWindow_Toggle();
		}
		OWrAI_DropAndAddFlag();
		if (!window_visible) {
			OWrOniWindow_Toggle();
		}
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_DeleteFlag)) {
		DeleteFlag(&localCharacter->location);
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_Unstick)) {
		float unstick_r;
		float unstick_x;
		float unstick_z;

		unstick_r = M3c2Pi - localCharacter->facing;
		unstick_r += M3cHalfPi;
		UUmTrig_Clip(unstick_r);

		unstick_x = MUrCos(unstick_r);
		unstick_z = MUrSin(unstick_r);

		localCharacter->location.x += unstick_x;
		localCharacter->location.z += unstick_z;

		localActiveCharacter->physics->position.x += unstick_x;
		localActiveCharacter->physics->position.z += unstick_z;
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_RecordScreen)) {
		ONgGameState->local.recordScreen = !ONgGameState->local.recordScreen;
	}


	if (ONrDebugKey_WentDown(ONcDebugKey_CharToCamera))
	{
		ONtCharacter		*character;

		// set the character's location to the camera's location
		character = ONrGameState_GetPlayerCharacter();

		if ((character == NULL) || (character->flags & ONcCharacterFlag_InUse == 0)) {
			// must create character
			character = AI2rRecreatePlayer();

			if (character == NULL) {
				AIrCreatePlayerFromTextFile();
				character = ONrGameState_GetPlayerCharacter();
			}

		} else if (character->flags & ONcCharacterFlag_Dead) {
			ONrCharacter_SetHitPoints(character, character->maxHitPoints);
		}

		if (character == NULL) {
			COrConsole_Printf("### unable to find player character or any player start point");
		} else {
			if (localActiveCharacter->physics == NULL) {
				ONrCharacter_EnablePhysics(localCharacter);
				UUmAssert(localActiveCharacter->physics != NULL);
			}

			localActiveCharacter->physics->position = CArGetLocation();
			character->location = localActiveCharacter->physics->position;

			// set the camera to follow mode
			CArFollow_Enter();
		}
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_AIBreakPoint)) {
		AI2rDebug_BreakPoint();
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_ShowMeleeStatus)) {
		AI2rMelee_ToggleStatusDisplay();
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_ExplodeOne)) {
		ONrParticle3_Explode(1);
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_ExplodeTwo)) {
		ONrParticle3_Explode(2);
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_ExplodeThree)) {
		ONrParticle3_Explode(3);
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_ExplodeFour)) {
		ONrParticle3_Explode(4);
	}

#if TOOL_VERSION
	ONrGameState_HandleUtilityInput_Tool(inInput);
#endif

	return;
}


static UUtBool HandleSpecialAnim(const ONtInputState *inInput, ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	const UUtUns16 interpolation = 6;
	const TRtAnimation *newAnimation = NULL;

	if (ioActiveCharacter->filmState.flags & ONcFilmFlag_Play) {
		if (inInput->buttonWentDown & LIc_BitMask_Cutscene1) {
			newAnimation = ioActiveCharacter->filmState.film->special_anim[0];

			if (NULL == newAnimation) {
				COrConsole_Printf("film playback failed to find animation for %s", ioCharacter->player_name);
			}
		}
		else if (inInput->buttonWentDown & LIc_BitMask_Cutscene2) {
			newAnimation = ioActiveCharacter->filmState.film->special_anim[1];

			if (NULL == newAnimation) {
				COrConsole_Printf("film playback failed to find animation for %s", ioCharacter->player_name);
			}
		}
	}
	else if (ioActiveCharacter->filmState.flags & ONcFilmFlag_Record) {
		if (inInput->buttonWentDown & LIc_BitMask_Cutscene1) {
			newAnimation = TRrAnimation_GetFromName(ONgBoundF2);
			ioActiveCharacter->filmState.film->special_anim[0] = (TRtAnimation *) newAnimation;

			if (NULL == newAnimation) {
				COrConsole_Printf("film record failed to find F2 animation %s", ONgBoundF2);
			}
		}
		else if (inInput->buttonWentDown & LIc_BitMask_Cutscene2) {
			newAnimation = TRrAnimation_GetFromName(ONgBoundF3);
			ioActiveCharacter->filmState.film->special_anim[1] = (TRtAnimation *) newAnimation;

			if (NULL == newAnimation) {
				COrConsole_Printf("film record failed to find F3 animation %s", ONgBoundF3);
			}
		}
	}

	if (newAnimation != NULL) {
		ONrCharacter_SetAnimation_External(
			ioCharacter,
			ioActiveCharacter->nextAnimState,
			newAnimation,
			interpolation);

		return UUcTrue;
	}

	return UUcFalse;
}

static void ONrCharacter_DebugHandle(char *inString)
{
	if (ONgDebugHandle) {
		COrConsole_Printf("%s", inString);
	}

	return;
}

static UUtBool ONrCharacter_IsBeingThrown(ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool is_being_thrown = UUcFalse;

	switch(ioActiveCharacter->curAnimType)
	{
		case ONcAnimType_Thrown1:
		case ONcAnimType_Thrown2:
		case ONcAnimType_Thrown3:
		case ONcAnimType_Thrown4:
		case ONcAnimType_Thrown5:
		case ONcAnimType_Thrown6:
		case ONcAnimType_Thrown7:
		case ONcAnimType_Thrown8:
		case ONcAnimType_Thrown9:
		case ONcAnimType_Thrown10:
		case ONcAnimType_Thrown11:
		case ONcAnimType_Thrown12:
		case ONcAnimType_Thrown13:
		case ONcAnimType_Thrown14:
		case ONcAnimType_Thrown15:
		case ONcAnimType_Thrown16:
		case ONcAnimType_Thrown17:
			is_being_thrown = UUcTrue;
		break;
	}

	return is_being_thrown;
}


// handle input for a character for a single game tick
void ONrCharacter_HandleHeartbeatInput(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter)
{
	UUtBool found, can_attack, restrict_movement;
	ONtInputState input = ioActiveCharacter->inputState;
	TRtAnimType old_next_anim_type;
	const TRtAnimation *old_animation;

	if ((ONcChar_Player == ioCharacter->charType) && (!ONrCharacter_IsPlayingFilm(ioCharacter)) &&
		((ioCharacter->flags & ONcCharacterFlag_AIMovement) == 0)) {
		if (UserInputCleared()) {
			input.buttonIsDown = 0;
			input.buttonWentDown = 0;
			input.buttonIsUp = 0xFFFFFFFFFFFFFFFF;
			input.buttonWentUp = 0;
			input.turnLR = 0.f;
			input.turnUD = 0.f;
		}
	}

	if (ioCharacter->flags & ONcCharacterFlag_Dead) {
		if (ONrCharacter_IsVictimAnimation(ioCharacter)) {
			goto exit;
		}
	}

	if (ioActiveCharacter->animationLockFrames > 0) {
		goto exit;
	}

	// make sure we are called <= once per tick
	UUmAssert(ioActiveCharacter->lastHeartbeat != ONgGameState->gameTime);
	ioActiveCharacter->lastHeartbeat = ONgGameState->gameTime;

	if ((ioActiveCharacter->lastAttackTime != ONcLastAttackTime_StillActive) &&
		((ONgGameState->gameTime - ioActiveCharacter->lastAttackTime) > 10))
	{
		ioActiveCharacter->lastAttack = ONcAnimType_None;
	}

	if (ioCharacter->flags & ONcCharacterFlag_Dead) {
		input.buttonIsDown = 0;
		input.buttonIsUp = 0xffffffffffffffff;
		input.buttonWentDown = 0;
		input.buttonWentUp = 0;

		HandleFallThrough(&input, ioCharacter, ioActiveCharacter);

		goto exit;
	}

	if (ONrCharacter_IsBeingThrown(ioActiveCharacter)) {
		input.buttonIsDown = 0;
		input.buttonIsUp = 0xffffffffffffffff;
		input.buttonWentDown = 0;
		input.buttonWentUp = 0;
	}

	ONrDesireFacingModifier(ioCharacter, 0.f);

	if (ioCharacter->inventory.weapons[0] != NULL) {
		if (input.buttonWentDown & LIc_BitMask_Fire1) {
			if (ONrCharacter_OutOfAmmo(ioCharacter)) {
				ioCharacter->flags2 |= ONcCharacterFlag2_WeaponEmpty;
			} else {
				ioCharacter->flags2 &= ~ONcCharacterFlag2_WeaponEmpty;
			}
		}

		if ((ioCharacter->flags2 & ONcCharacterFlag2_WeaponEmpty) ||
			ONrOverlay_IsActive(ioActiveCharacter->overlay + ONcOverlayIndex_InventoryManagement)) {

			// character cannot fire
			ONrCharacter_ReleaseTrigger(ioCharacter, ioActiveCharacter);
			ONrCharacter_ReleaseAlternateTrigger(ioCharacter, ioActiveCharacter);
		} else {
			if (input.buttonWentDown & LIc_BitMask_Fire1) {
				ONrCharacter_Trigger(ioCharacter, ioActiveCharacter);

			} else if (input.buttonIsDown & LIc_BitMask_Fire1) {
				ONrCharacter_AutoTrigger(ioCharacter, ioActiveCharacter);

			} else {
				if (input.buttonWentUp & LIc_BitMask_Fire1) {
					ONrCharacter_ReleaseTrigger(ioCharacter, ioActiveCharacter);
				}

				if (input.buttonWentDown & LIc_BitMask_Fire2) {
					ONrCharacter_AlternateTrigger(ioCharacter, ioActiveCharacter);

				} else if (input.buttonWentUp & LIc_BitMask_Fire2) {
					ONrCharacter_ReleaseAlternateTrigger(ioCharacter, ioActiveCharacter);
				}
			}
		}
	} else {
		ioCharacter->flags2 &= ~ONcCharacterFlag2_WeaponEmpty;
	}


	// if you have a weapon, the act of looking around marks you as not idle
	if (ioActiveCharacter->animVarient & ONcAnimVarientMask_Any_Weapon) {
		if ((input.turnLR != 0) || (input.turnUD != 0))
		{
			ONrCharacter_NotIdle(ioCharacter);
		}
	}

	HandleMouseInput(&input, input.turnLR, input.turnUD, ioCharacter, ioActiveCharacter);

	ONrCharacter_HandlePickupInput(&input, ioCharacter, ioActiveCharacter);

	old_next_anim_type = ioActiveCharacter->nextAnimType;
	old_animation = ioActiveCharacter->animation;

	restrict_movement = ((ioCharacter->flags & ONcCharacterFlag_RestrictedMovement) > 0);

	while(1)
	{
		// consider block stun / hit stun

		found = HandleSpecialAnim(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleSpecialAnim"); break; }

		found = HandleStartLanding(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleStartLanding"); break; }

		found = HandleStartFalling(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleStartFalling"); break; }

		found = HandleStun(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleStun"); break; }

		found = HandleLeaveStun(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleLeaveStun"); break; }

		found = HandleStartPickup(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleStartPickup"); break; }

		// player can always attack; AI can attack when playing a film, or when deliberately trying to;
		// nobody can attack if they are carrying a huge weapon
		can_attack = UUcFalse;
		can_attack = can_attack || ((ioCharacter->flags & ONcCharacterFlag_AIMovement) == 0);
		can_attack = can_attack || (ONrCharacter_IsPlayingFilm(ioCharacter)) || (AI2rTryingToAttack(ioCharacter));
		can_attack = can_attack && !restrict_movement;

		if (can_attack) {
			found = HandleStartSliding(&input, ioCharacter, ioActiveCharacter);
			if (found) { ONrCharacter_DebugHandle("HandleStartSliding"); break; }

			found = HandleStartAttack(&input, ioCharacter, ioActiveCharacter);
			if (found) { ONrCharacter_DebugHandle("HandleStartAttack"); break; }
		}

		if (!restrict_movement) {
			found = HandleStartJumping(&input, ioCharacter, ioActiveCharacter);
			if (found) { ONrCharacter_DebugHandle("HandleStartJumping"); break; }
		}

		found = HandleStartTauntOrAction(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleStartTauntOrAction"); break; }

		found = HandlePriorAction(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandlePriorAction"); break; }

		found = HandleStartRunning(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleStartRunning"); break; }

		found = HandleStartSidestep(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleStartSidestep"); break; }

		found = HandleChangeStance(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleChangeStance"); break; }

		found = HandleStandingTurn(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleStandingTurn"); break; }

		found = HandleStop(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleStop"); break; }

		found = HandleFallThrough(&input, ioCharacter, ioActiveCharacter);
		if (found) { ONrCharacter_DebugHandle("HandleFallThrough"); break; }

		break;
	}

	if (ONgAnimBuffer != 0)
	{
		if (ioActiveCharacter->animation == old_animation)
		{
			if ((ioActiveCharacter->animFrame + ONgAnimBuffer + 1) < TRrAnimation_GetDuration(ioActiveCharacter->animation))
			{
				ioActiveCharacter->nextAnimType = old_next_anim_type;
			}
		}
	}

	if ((ioActiveCharacter->inAirControl.numFramesInAir > 0) && (input.buttonIsDown & LIc_BitMask_Jump))
	{
		const ONtAirConstants *airConstants = ONrCharacter_GetAirConstants(ioCharacter);

   		if (ioActiveCharacter->inAirControl.numFramesInAir < airConstants->numJumpAccelFrames) {
			ioActiveCharacter->inAirControl.velocity.y += airConstants->jumpAccelAmount;
		}
	}

	ONrCharacter_UpdateAnimationForVarient(ioCharacter, ioActiveCharacter);

	HandleTurning(&input, input.turnLR, input.turnUD, ioCharacter, ioActiveCharacter);

exit:
	return;
}

void
ONrInput_Update_Keys(
	ONtInputState*		ioInput,
	LItButtonBits		inNewDown)
{
	LItButtonBits lastDown;
	LItButtonBits lastUp;
	LItButtonBits currentDown;
	LItButtonBits wentDown;
	LItButtonBits currentUp;
	LItButtonBits wentUp;

	UUmAssertWritePtr(ioInput, sizeof(ONtInputState));

	lastUp = ioInput->buttonIsUp;
	lastDown = ioInput->buttonIsDown;

	currentUp = ~inNewDown;
	currentDown = inNewDown;

	wentUp = (currentUp) & (lastDown);
	wentDown = (currentDown) & (lastUp);

	ioInput->buttonIsUp = currentUp;
	ioInput->buttonIsDown = currentDown;
	ioInput->buttonWentUp = wentUp;
	ioInput->buttonWentDown = wentDown;

	return;
}

static LItButtonBits ONiGameState_ScanButtons(
	LItAction*		inAction)
{
	LItButtonBits currentDown;

	UUmAssertWritePtr(ONgGameState, sizeof(*ONgGameState));

	if (NULL == inAction) {
		currentDown = ONgGameState->local.localInput.buttonIsDown;
	}
	else {
		currentDown = inAction->buttonBits;
	}

	return currentDown;
}

static void ONiGameState_ProcessMiscActions(ONtGameState *ioGameState)
{
	UUtBool can_escape;

#if TOOL_VERSION
	can_escape = UUcTrue;
#else
	can_escape = !ioGameState->local.in_cutscene;
#endif

	if (can_escape) {
		if (ioGameState->local.localInput.buttonWentDown & LIc_BitMask_Escape)
		{
			if (CanEscapeKey()) {
				OWrOniWindow_Toggle();
				NoEscapeKeyForABit();
			}
		}

		if (ioGameState->local.localInput.buttonWentDown & LIc_BitMask_PauseScreen)
		{
			NoEscapeKeyForABit();

			// CB: don't display pause screen when player is dead - october 03, 2000
			if ((ioGameState->local.playerCharacter->flags & ONcCharacterFlag_Dead) == 0) {
				ioGameState->local.pending_pause_screen = UUcTrue;
			}
		}
	}

	if (!ONgDeveloperAccess) { return; }

	if (ioGameState->local.localInput.buttonWentDown & LIc_BitMask_Console)
	{
		COrConsole_Activate();

//		OWrConsole_Toggle();
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_ProfileToggle))
	{
		#if defined(PROFILE) && PROFILE

			UUtProfileState	profileState = UUrProfile_State_Get();

			profileState = (UUtProfileState)!profileState;

			if(profileState == UUcProfile_State_On)
			{
				COrConsole_Printf("Profile on");
			}
			else
			{
				COrConsole_Printf("Profile off");
			}

			UUrProfile_State_Set(profileState);

		#endif
	}
}

UUtError ONrGameState_ProcessActions(
	LItAction*		inActionBuffer)
{
	if (NULL != inActionBuffer) {
		ONiGameState_ProcessMiscActions(ONgGameState);
	}

	return UUcError_None;
}

LItButtonBits ONiRemapKeys(LItButtonBits currentDown)
{
	ONtCharacter *localCharacter = ONrGameState_GetPlayerCharacter();

	if (NULL == localCharacter)
	{
		goto exit;
	}

	if (localCharacter->neutral_interaction_char != NULL) {
		// neutral-character interaction... mask out user input but still allow illusion of control.

		if (currentDown & LIc_BitMask_NeutralAbort) {
			// the character wants to abort right now, let them
			AI2rNeutral_Stop(localCharacter, UUcFalse, UUcTrue, UUcFalse, UUcTrue);

		} else if (currentDown & LIc_BitMask_NeutralStop) {
			if ((localCharacter->flags & ONcCharacterFlag_NeutralUninterruptable) == 0) {
				localCharacter->neutral_keyabort_frames += 2;
			}

			if (localCharacter->neutral_keyabort_frames >= AI2cNeutral_KeyAbortFrames) {
				// the character has been pressing keys that indicate they want to abort, let them
				AI2rNeutral_Stop(localCharacter, UUcFalse, UUcTrue, UUcFalse, UUcTrue);
			}

		} else {
			// the character isn't pressing any abort keys
			if (localCharacter->neutral_keyabort_frames > 0) {
				localCharacter->neutral_keyabort_frames -= 2;
			}
		}

		if (localCharacter->neutral_interaction_char != NULL) {
			// lock out any keypresses that would stop the neutral interaction
			currentDown &= ~LIc_BitMask_NeutralLockOut;
		}
	}

	if (currentDown & LIc_BitMask_Fire1) {
		if ((localCharacter->inventory.weapons[0] == NULL) || (localCharacter->flags2 & ONcCharacterFlag2_WeaponEmpty)) {
			currentDown |= LIc_BitMask_Punch;
		}
	}

	if (currentDown & LIc_BitMask_Fire2) {
		if ((localCharacter->inventory.weapons[0] == NULL) || ((WPrGetClass(localCharacter->inventory.weapons[0])->flags & WPcWeaponClassFlag_HasAlternate) == 0) ||
			(localCharacter->flags2 & ONcCharacterFlag2_WeaponEmpty)) {
			currentDown |= LIc_BitMask_Kick;
		}
	}

	if (currentDown & LIc_BitMask_Fire3) {
		currentDown |= LIc_BitMask_Crouch;
	}

	if ((localCharacter->inventory.weapons[0] != NULL) && (WPrGetClass(localCharacter->inventory.weapons[0])->flags & WPcWeaponClassFlag_WalkOnly)) {
		localCharacter->flags |= ONcCharacterFlag_RestrictedMovement;
		currentDown |= LIc_BitMask_Walk;
	} else {
		localCharacter->flags &= ~ONcCharacterFlag_RestrictedMovement;
	}

exit:
	return currentDown;
}

static void ONrGameState_UpdateConditionSounds(void)
{
	UUtUns32 itr;
	float volume;
	ONtCharacter *player;
	WPtInventory *inventory;

	if ((ONgGameState->local.camera->mode != CAcMode_Follow) || (ONgGameState->local.letterbox.active)) {
		// set all condition sounds down to zero volume
		for (itr = 0; itr < ONcConditionSound_Max; itr++) {
			if (ONrGameState_ConditionSound_Active(itr)) {
				ONrGameState_ConditionSound_Start(itr, 0.0f);
			}
		}
		return;
	}

	player = ONrGameState_GetPlayerCharacter();
	if (player == NULL) {
		return;
	}
	inventory = &player->inventory;

	// note: these volume calculations are all parabolic fades from full volume down to ONcGameState_ConditionSound_MinVolume

	if ((player->hitPoints < (player->maxHitPoints / 5)) && (player->hitPoints > 0)) {
		// volume varies by the amount of hitpoints the player has left
		volume = ((((float) player->hitPoints) * 5.0f) / ((float)player->maxHitPoints));
		volume = 1.0f - (1.0f - ONcGameState_ConditionSound_MinVolume) * UUmSQR(volume);
		ONrGameState_ConditionSound_Start(ONcConditionSound_HealthLow, volume);
	} else {
		ONrGameState_ConditionSound_Stop(ONcConditionSound_HealthLow);
	}

	if (player->hitPoints > player->maxHitPoints) {
		// determine the amount of boosted health
		volume = (player->hitPoints - player->maxHitPoints) /
						(player->maxHitPoints * (ONgGameSettings->boosted_health - 1.0f));
		volume = UUmPin(volume, 0.0f, 1.0f);
		volume = 1.0f - (1.0f - ONcGameState_ConditionSound_MinVolume) * UUmSQR(1.0f - volume);
		ONrGameState_ConditionSound_Start(ONcConditionSound_HealthOver, volume);
	} else {
		ONrGameState_ConditionSound_Stop(ONcConditionSound_HealthOver);
	}

#if 0
	// CB: removed the forcefield sound because people find it annoying - october 30, 2000
	if (inventory->shieldRemaining > 0) {
		// volume varies by the amount of shield
		volume = (((float) player->inventory.shieldDisplayAmount) / WPcMaxShields);
		volume = 1.0f - (1.0f - ONcGameState_ConditionSound_MinVolume) * UUmSQR(1.0f - volume);
		ONrGameState_ConditionSound_Start(ONcConditionSound_Shield, volume);
	} else {
		ONrGameState_ConditionSound_Stop(ONcConditionSound_Shield);
	}
#endif

	if (inventory->invisibilityRemaining > 0) {
		// volume varies by the amount of invisibility
		volume = (((float) player->inventory.invisibilityRemaining) / WPcMaxInvisibility);
		volume = 1.0f - (1.0f - ONcGameState_ConditionSound_MinVolume) * UUmSQR(1.0f - volume);
		ONrGameState_ConditionSound_Start(ONcConditionSound_Invis, volume);
	} else {
		ONrGameState_ConditionSound_Stop(ONcConditionSound_Invis);
	}
}

#define BRENTS_CHEESY_GSU_PERF	1


UUtInt64	time_ai = 0;

static void ZeroUserInput(LItAction *inAction)
{
	UUtBool zero_user_input = UserInputCleared();

	if (NULL != inAction) {
		if (zero_user_input) {
			inAction->buttonBits &= (LIc_BitMask_Console + LIc_BitMask_Escape + LIc_BitMask_ScreenShot);

			inAction->analogValues[LIc_Look_LR] = 0.f;
			inAction->analogValues[LIc_Look_UD] = 0.f;
			inAction->analogValues[LIc_Aim_LR] = 0.f;
			inAction->analogValues[LIc_Aim_UD] = 0.f;
		}
	}

	return;
}

void
ONrGameState_ProcessHeartbeat(
	   LItAction*		inAction)
{
	ONtCharacter *localCharacter;
	const ONtInputState *localInput;
	float mousex;
	float mousey;
	LItButtonBits currentDown;

	UUtStallTimer heartbeat;
	UUtStallTimer heartbeat_internal;

	#if defined(BRENTS_CHEESY_GSU_PERF) && BRENTS_CHEESY_GSU_PERF

		UUtUns64	time_start;
		UUtUns32	time_total;

		float		time_ai_per_gsu;

		UUtInt64	time_chr_start;
		UUtUns32	time_chr = 0;
		float		time_chr_per_gsu;

		UUtInt64	time_phs_start;
		UUtUns32	time_phs = 0;
		float		time_phs_per_gsu;

		UUtInt64	time_prt_start;
		UUtUns32	time_prt = 0;
		float		time_prt_per_gsu;

		char		s1[128], s2[128];

		time_start = UUrMachineTime_High();

		// clear the timing counts
		time_ai = 0;

	#endif

#if PERFORMANCE_TIMER
		UUrPerformanceTimer_Pulse();
#endif

	ZeroUserInput(inAction);

	UUrStallTimer_Begin(&heartbeat);

	switch(ONgGameState->local.cutscene_skip_mode)
	{
		case ONcCutsceneSkip_starting:
			if (ONgGameState->local.cutscene_skip_timer > 0) {
				ONgGameState->local.cutscene_skip_timer--;
			}

			if (0 == ONgGameState->local.cutscene_skip_timer) {
				ONgGameState->local.cutscene_skip_mode = ONcCutsceneSkip_after_fill_black;
			}
		break;

		case ONcCutsceneSkip_fading_in_after_skipping:
			if (ONgGameState->local.cutscene_skip_timer > 0) {
				ONgGameState->local.cutscene_skip_timer--;
			}

			if (0 == ONgGameState->local.cutscene_skip_timer) {
				ONgGameState->local.cutscene_skip_mode = ONcCutsceneSkip_idle;
			}
		break;
	}

	ONrGameState_Timer_Update();

	if (ONgGameState->local.slowMotionTimer > 0) {
		ONgGameState->local.slowMotionTimer--;
	}

	if (ONgGameState->local.cameraPlay && ONgGameState->local.cameraRecording) {
		ONgGameState->local.cameraFrame += 1;
		ONgGameState->local.cameraFrame %= ONgGameState->local.cameraRecording->numFrames;
	}

// CB: gTickNum is no longer used anywhere, I believe
//	gTickNum++;

	localCharacter = ONrGameState_GetPlayerCharacter();
	localInput = &ONgGameState->local.localInput;

	// updateProjectiles
	// updateCharacters
	// .
	// .
	// .

	if (localCharacter->neutral_interaction_char != NULL) {
		// CB: block turning input while in neutral interaction mode
		mousex = 0.f;
		mousey = 0.f;
	} else if (inAction != NULL) {
		mousex = inAction->analogValues[LIc_Aim_LR];
		mousey = inAction->analogValues[LIc_Aim_UD];
	}
	else {
		mousex = 0.f;
		mousey = 0.f;
	}

	if (AMgInvert) {
		mousey = -mousey;
	}

	currentDown = ONiGameState_ScanButtons(inAction);
	currentDown = ONiRemapKeys(currentDown);

	ONrInput_Update_Keys(&(ONgGameState->local.localInput), currentDown);

	if (ONgGameState->gameTime < 14) {
		ONgGameState->local.localInput.buttonIsDown &= ~(LIc_BitMask_Fire1 | LIc_BitMask_Fire2 | LIc_BitMask_Fire3);
		ONgGameState->local.localInput.buttonWentUp = 0;
		ONgGameState->local.localInput.buttonWentDown = 0;
	}

	ONgGameState->local.localInput.turnLR = mousex;
	ONgGameState->local.localInput.turnUD = mousey;

	ONrGameState_ProcessActions(inAction);
	ONrGameState_HandleUtilityInput(localInput);

	if (localCharacter != NULL) {
		ONrCharacter_UpdateInput(localCharacter, &ONgGameState->local.localInput);
	}

//	ONrNet_NetCharacterUpdate();

	// Update the AI

	UUrStallTimer_Begin(&heartbeat_internal);
		AI2rUpdateAI();
	UUrStallTimer_End(&heartbeat_internal, "ONrGameState_ProcessHeartbeat - AI2rUpdateAI");


	UUrStallTimer_Begin(&heartbeat_internal);
		SLrScript_Update(ONgGameState->gameTime);
	UUrStallTimer_End(&heartbeat_internal, "ONrGameState_ProcessHeartbeat - SLrScript_Update");


	OBrDoor_Array_Update(&ONgGameState->doors);

	// Update the characters
		#if defined(BRENTS_CHEESY_GSU_PERF) && BRENTS_CHEESY_GSU_PERF

			time_chr_start = UUrMachineTime_High();

		#endif

		ONrGameState_UpdateCharacters();

		#if defined(BRENTS_CHEESY_GSU_PERF) && BRENTS_CHEESY_GSU_PERF

			time_chr += (UUtUns32)(UUrMachineTime_High() - time_chr_start);

		#endif

		// update the sky
		//ONrSky_Update( &ONgGameState->sky );

		// update the game mechanics...

		ONrMechanics_Update();

	// Update the weapons
		WPrUpdate();
		WPrPowerup_Update();

	// Update the physics
		#if defined(BRENTS_CHEESY_GSU_PERF) && BRENTS_CHEESY_GSU_PERF

			time_phs_start = UUrMachineTime_High();

		#endif

		PHrPhysicsContext_Update();

		#if defined(BRENTS_CHEESY_GSU_PERF) && BRENTS_CHEESY_GSU_PERF

			time_phs += (UUtUns32)(UUrMachineTime_High() - time_phs_start);

		#endif

	// Update the particles
		#if defined(BRENTS_CHEESY_GSU_PERF) && BRENTS_CHEESY_GSU_PERF

			time_prt_start = UUrMachineTime_High();

		#endif

		// update all essential P3 particles for one heartbeat
		P3rUpdate(ONgGameState->gameTime + 1, UUcTrue);

		#if defined(BRENTS_CHEESY_GSU_PERF) && BRENTS_CHEESY_GSU_PERF

			time_prt += (UUtUns32)(UUrMachineTime_High() - time_prt_start);

		#endif

	UUrStallTimer_Begin(&heartbeat_internal);
		OBrList_Update(ONgGameState->objects);
	UUrStallTimer_End(&heartbeat_internal, "ONrGameState_ProcessHeartbeat - OBrList_Update");

	// update the cinematics
	UUrStallTimer_Begin(&heartbeat_internal);
		OCrCinematic_Update();
	UUrStallTimer_End(&heartbeat_internal, "ONrGameState_ProcessHeartbeat - OCrCinematic_Update");


	// update the in-game UI
	UUrStallTimer_Begin(&heartbeat_internal);
		ONrInGameUI_Update();
	UUrStallTimer_End(&heartbeat_internal, "ONrGameState_ProcessHeartbeat - ONrInGameUI_Update");


	// update the condition sounds (shield, invis, low health, etc)
	ONrGameState_UpdateConditionSounds();

	if ((ONgGameState->gameTime % ONcGameState_AutoPromptFrames) == 0) {
		ONrGameState_UpdateAutoPrompt();
	}

	#if defined(BRENTS_CHEESY_GSU_PERF) && BRENTS_CHEESY_GSU_PERF

		if(ONgShowPerformance_GSU)
		{

			time_total = (UUtUns32)(UUrMachineTime_High() - time_start);

			time_ai_per_gsu = (float)time_ai / (float)time_total;
			time_chr_per_gsu = (float)time_chr / (float)time_total;
			time_phs_per_gsu = (float)time_phs / (float)time_total;
			time_prt_per_gsu = (float)time_prt / (float)time_total;

			sprintf(
				s1,
				"gsu, ai: %02.1f, chr: %02.1f",
				time_ai_per_gsu * 100.0f,
				time_chr_per_gsu * 100.0f);
			sprintf(
				s2,
				"gsu, phs: %02.1f, prt: %02.1f",
				time_phs_per_gsu * 100.0f,
				time_prt_per_gsu * 100.0f);

			ONrGameState_Performance_UpdateGSU(
				s1,
				s2,
				"");
		}

	#endif

	UUrStallTimer_End(&heartbeat, "ONrGameState_ProcessHeartbeat");

	return;
}

static UUtError ONrGameState_UpdateCamera(ONtGameState *ioGameState, UUtUns32 deltaTicks)
{
	// hey like this should do something sometime

	return UUcError_None;
}

static UUtError ONrGameState_UpdateSoundManager(ONtGameState *ioGameState, UUtUns32 deltaTicks)
{
	M3tPoint3D		location;
	M3tPoint3D		facing;
	CAtCamera		*camera;

	camera = ONgGameState->local.camera;

	// determine which mode to use
	if ((camera->mode == CAcMode_Follow) &&	(ONgGameState->local.playerCharacter != NULL)) {
		// set listener at character
		if (ONgStableEar) {
			ONrCharacter_GetEarPosition(ONgGameState->local.playerCharacter, &location);
		}
		else {
			ONrCharacter_GetEyePosition(ONgGameState->local.playerCharacter, &location);
		}
		ONrCharacter_GetFacingVector(ONgGameState->local.playerCharacter, &facing);
	}
	else {
		// set listener at camera
		location = CArGetLocation();
		facing = CArGetFacing();
	}

	OSrUpdate(&location, &facing);

	return UUcError_None;
}

#define cMaxTicksPerFrame 6

static UUtUns8 iComputeDeltaTicks(ONtGameState *ioGameState)
{
	UUtUns32 timing_delta_ticks;
	UUtUns32 engine_delta_ticks;

	timing_delta_ticks = ioGameState->serverTime - ioGameState->gameTime;

	if ((ioGameState->local.sync_enabled) && (ioGameState->local.sync_frames_behind > 0)) {
		if (ioGameState->local.sync_frames_behind > (60 * 5)) {
			COrConsole_Printf("we are %d frames behind, marking out of sync", ioGameState->local.sync_frames_behind);
			ioGameState->local.sync_frames_behind = 0;
		}
		else {
			if (ONgSyncDebug) {
				COrConsole_Printf("we are %d frames behind, syncing", ioGameState->local.sync_frames_behind);
			}
		}

		timing_delta_ticks += ioGameState->local.sync_frames_behind;
		ioGameState->local.sync_frames_behind = 0;
	}

	engine_delta_ticks = timing_delta_ticks;

	engine_delta_ticks = UUmMin(engine_delta_ticks, cMaxTicksPerFrame);

	if (ONgSingleStep) {
		engine_delta_ticks = 0;
		UUrPlatform_Idle(ONgPlatformData.gameWindow, 4);

		if (ONrDebugKey_WentDown(ONcDebugKey_SingleStep)) {
			engine_delta_ticks = 1;
		}
	}
	else if ((ONgDrawEveryFrame) || (ONgGameState->local.recordScreen)) {
		engine_delta_ticks = ONgDrawEveryFrameMultiple;
	}
	else if (ONgFastMode) {
		engine_delta_ticks = 24;
	}
	else if (ONrGameState_IsSkippingCutscene()) {
		engine_delta_ticks = 32;
	}
	else {
		if (ioGameState->local.sync_enabled) {
			ioGameState->local.sync_frames_behind = timing_delta_ticks - engine_delta_ticks;
		}
	}

	ioGameState->serverTime = ioGameState->gameTime + engine_delta_ticks;

	return (UUtUns8) engine_delta_ticks;
}

typedef struct {
	ONtDebuggingKey	keyCode;
	UUtBool			*variable;
} KeyBindEntry;

static void Testing_Update(void)
{
	static KeyBindEntry key_bindings[] =
	{
		{ ONcDebugKey_Occl,					&AKgDraw_Occl, },
		{ ONcDebugKey_SoundOccl,			&AKgDraw_SoundOccl, },
		{ ONcDebugKey_Invis,				&AKgDraw_Invis },
		{ ONcDebugKey_Perf_Overall,			&ONgShowPerformance_Overall },
		{ ONcDebugKey_FastMode,				&ONgFastMode },
		{ ONcDebugKey_DrawEveryFrame,		&ONgDrawEveryFrame },
		{ ONcDebugKey_Character_Collision,	&AKgDraw_CharacterCollision },
		{ ONcDebugKey_Object_Collision,		&AKgDraw_ObjectCollision },
#if PARTICLE_PERF_ENABLE
		{ ONcDebugKey_Perf_Particle,		&P3gPerfDisplay },
		{ ONcDebugKey_Perf_Particle_Lock,	&P3gPerfLockClasses },
#endif
		{ ONcDebugKey_None,					NULL },
	};

	KeyBindEntry *current_binding;

	for(current_binding = key_bindings; current_binding->variable != NULL; current_binding++)
	{
		if (ONrDebugKey_WentDown(current_binding->keyCode)) {
			*(current_binding->variable) = !(*(current_binding->variable));
		}
	}

	if (ONrDebugKey_WentDown(ONcDebugKey_SoundOccl)) {
		AKgDraw_SoundOccl = !AKgDraw_SoundOccl;
#if TOOL_VERSION
		OSgShowOcclusions = AKgDraw_SoundOccl;
#endif
	}

	return;
}

UUtError ONrGameState_Update(
	UUtUns16 numActionsInBuffer,
	LItAction *actionBuffer,
	UUtUns32 *outTicksUpdated)
{
	UUtError error;
	UUtUns16 itr;
	UUtUns16 deltaTicks;// = desiredTicks;

	extern void OBJrTrigger_Clean(void); // OT_Trigger.c
	OBJrTrigger_Clean();

	if (ONgGameState->local.in_cutscene) {
		static UUtBool old_space_was_down = UUcFalse;
		UUtBool space_went_down = LIrKeyWentDown(LIcKeyCode_Space, &old_space_was_down);

		if (space_went_down) {
			// ONrGameState_SkipCutscene();
		}
	}

	Testing_Update();

	deltaTicks = iComputeDeltaTicks(ONgGameState);

	#if UUmCompiler	!= UUmCompiler_VisC
		#if defined(PROFILE) && PROFILE
			deltaTicks = 2;
		#endif
	#endif

	// ** call the heartbeat
	for(itr = 0; itr < deltaTicks; itr++)
	{
		LItAction *action;

		if (itr < numActionsInBuffer) {
			action = actionBuffer + itr;
		}
		else {
			action = NULL;
		}

		ONrGameState_ProcessHeartbeat(action);
		CArUpdate(1, 0, NULL);

		ONgGameState->gameTime += 1;
	}

#if PERFORMANCE_TIMER
	UUrPerformanceTimer_Display(perf_overall + 4, 10);
#endif

	CArUpdate(0, numActionsInBuffer, actionBuffer);

	// record the screen if requested
	if (ONgGameState->local.recordScreen)
	{
		iScreenShot(ONgScreenShotReduceAmount);
	}

	// update all decorative particles to match the current game time
	P3rUpdate(ONgGameState->gameTime, UUcFalse);

	// ** update support libraries and tell them how many ticks passed
	//error = ONrGameState_UpdateCamera(ioGameState, deltaTicks);
	error = ONrGameState_UpdateSoundManager(ONgGameState, deltaTicks);

	// update_status_bar(deltaTicks)

	// check to see if we need to autosave binary data files
	OBDrUpdate(ONgGameState->gameTime);

	if (outTicksUpdated != NULL) {
		*outTicksUpdated = (UUtUns32) deltaTicks;
	}

	return UUcError_None;
}

UUtError ONrGameState_ConstantColorLighting(float r, float g, float b)
{
	static M3tLight_Ambient		ambientLight[1];

	ambientLight[0].color.r = r;
	ambientLight[0].color.g = g;
	ambientLight[0].color.b = b;

	M3rLightList_Ambient(ambientLight);
	M3rLightList_Point(0, NULL);
	M3rLightList_Directional(0, NULL);

	return UUcError_None;
}

typedef struct Display_Performance_Variables
{
	UUtInt64	time_total;
	UUtInt64	time_chr;
	UUtInt64	time_env;
	UUtInt64	time_obj;
	UUtInt64	time_prt;
} Display_Performance_Variables;

static void ONi_GameState_Display_All_UI_Elements(Display_Performance_Variables *inPerfVar)
{
	const UUtUns16 screen_width = M3rDraw_GetWidth();
	const UUtUns16 screen_height = M3rDraw_GetHeight();
	float perfTop = 128;

	if (ONrGameState_IsSkippingCutscene()) {
		goto exit;
	}

	// Display the ingame UI
	if ((ONgShow_UI) && (!ONgGameState->local.letterbox.active))
	{
		ONrInGameUI_Display();
//		ONrEnemyScanner_Display();
	}

#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF

	if(ONgShowPerformance_GSD)
	{
		double		time_total = (double) inPerfVar->time_total;
		double		time_env_per_gsd;
		double		time_chr_per_gsd;
		double		time_obj_per_gsd;
		double		time_prt_per_gsd;

		char		s1[128], s2[128], s3[128];

		extern UUtUns16	ONgNumCharactersDrawn;

		time_env_per_gsd = inPerfVar->time_env / time_total;
		time_chr_per_gsd = inPerfVar->time_chr / time_total;
		time_obj_per_gsd = inPerfVar->time_obj / time_total;
		time_prt_per_gsd = inPerfVar->time_prt / time_total;

		sprintf(
			s1,
			"gsd, env: %02.1f, chr: %02.1f obj: %02.1f, prt: %02.1f",
			time_env_per_gsd * 100.0f,
			time_chr_per_gsd * 100.0f,
			time_obj_per_gsd * 100.0f,
			time_prt_per_gsd * 100.0f);

		{
			static UUtUns32 highDownload;
			static UUtUns32 highDownloadTime = 0;
			char downloadStr[32];
			char maxDownloadStr[32];


			if ((ONgPerformanceData.textureDownload > highDownload) || (UUrMachineTime_Sixtieths() > (highDownloadTime+120)))
			{
				highDownload = ONgPerformanceData.textureDownload;
				highDownloadTime = UUrMachineTime_Sixtieths();
			}

			AUrNumToCommaString(ONgPerformanceData.textureDownload, downloadStr);
			AUrNumToCommaString(highDownload, maxDownloadStr);

			sprintf(
				s2,
				"gsd, texture = %s max %s M", downloadStr, maxDownloadStr);
		}

		sprintf(
			s3,
			"nc: %02d, np: %04d, nq: %05d, na: %04d",
			ONgNumCharactersDrawn,
			0,	// counter for num particles goes here
			ONgPerformanceData.numTriSplits + ONgPerformanceData.numQuadSplits * 2 + ONgPerformanceData.numPentSplits * 3,
			ONgPerformanceData.numAlphaSortedObjs);

		ONrGameState_Performance_UpdateGSD(
			s1,
			s2,
			s3);
	}
#endif

exit:
	return;
}

static void ONrBlanketScreen(float inA, float inR, float inG, float inB)
{
	const UUtUns16 screen_width = M3rDraw_GetWidth();
	const UUtUns16 screen_height = M3rDraw_GetHeight();
	M3tTextureCoord uv[4];
	M3tPointScreen dest[2];
	UUtUns32 intShade, intR, intG, intB;

	if (ONrGameState_IsSkippingCutscene() || (0.f == inA)) {
		goto exit;
	}

	if (ONgLetterboxTexture == NULL) {
		goto exit;
	}

	dest[0].x = 0.f;
	dest[0].y = 0.f;
	dest[0].z = 1.f;
	dest[0].invW = 10.f;

	dest[1].x = (float) (screen_width);
	dest[1].y = (float) (screen_height);
	dest[1].z = 1.f;
	dest[1].invW = 10.f;

	uv[0].u = 0.f;
	uv[0].v = 0.f;
	uv[1].u = 1.f;
	uv[1].v = 0.f;
	uv[2].u = 0.f;
	uv[2].v = 1.f;
	uv[3].u = 1.f;
	uv[3].v = 1.f;

	intR = (UUtUns8) MUrUnsignedSmallFloat_To_Uns_Round(inR * 255.f);
	intG = (UUtUns8) MUrUnsignedSmallFloat_To_Uns_Round(inG * 255.f);
	intB = (UUtUns8) MUrUnsignedSmallFloat_To_Uns_Round(inB * 255.f);

	intShade = (intR << 16) | (intG << 8) | (intB);

	M3rDraw_State_Push();

	M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, intShade);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, MUrUnsignedSmallFloat_To_Uns_Round(M3cMaxAlpha * inA));

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_BaseTextureMap,
		ONgLetterboxTexture);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_None);

	M3rDraw_State_Commit();

	M3rDraw_Sprite(
		dest,
		uv);

	M3rDraw_State_Pop();

exit:
	return;
}

static void ONi_GameState_Display_SkipCutsceneFade()
{
	float distance;

	switch(ONgGameState->local.cutscene_skip_mode)
	{
		case ONcCutsceneSkip_idle:
		case ONcCutsceneSkip_skipping:
			goto exit;

		case ONcCutsceneSkip_starting:
			distance = (float) (ONcNumber_of_frames_for_cutscene_skip_timer - ONgGameState->local.cutscene_skip_timer);
		break;

		case ONcCutsceneSkip_after_fill_black:
			distance = (float) ONcNumber_of_frames_for_cutscene_skip_timer;
		break;

		case ONcCutsceneSkip_fading_in_after_skipping:
			distance = (float) ONgGameState->local.cutscene_skip_timer;
		break;
	}

	distance /= ONcNumber_of_frames_for_cutscene_skip_timer;
	UUmAssert((distance >= 0.f) && (distance <= 1.f));
	distance = get_cosmetic_ramp(distance);

	//COrConsole_Printf("cutscene fade = %f", distance);
	ONrBlanketScreen(distance, 0.f, 0.f, 0.f);

	if (ONcCutsceneSkip_after_fill_black == ONgGameState->local.cutscene_skip_mode)
	{
		ONgGameState->local.cutscene_skip_mode = ONcCutsceneSkip_skipping;
	}

exit:
	return;
}

static void ONi_GameState_Display_FadeInOut()
{
	float curA, curR, curG, curB;
	float distance;
	float percent_start;
	float percent_end;

	if (!ONgGameState->local.fadeInfo.active) { return; }

	distance = (float) (ONgGameState->local.fadeInfo.endTime - ONgGameState->local.fadeInfo.startTime);

	if ((ONgGameState->gameTime < ONgGameState->local.fadeInfo.endTime) && (distance > 0))
	{
		percent_end = (float) (UUmMin(ONgGameState->gameTime, ONgGameState->local.fadeInfo.endTime) - ONgGameState->local.fadeInfo.startTime);
		percent_end /= distance;
		percent_end = (float) pow(percent_end, 1.9f);
		percent_start = 1 - percent_end;
	}
	else
	{
		percent_end = 1.f;
		percent_start = 0.f;
	}

	curA = (ONgGameState->local.fadeInfo.start_a * percent_start) + (ONgGameState->local.fadeInfo.end_a * percent_end);
	curR = (ONgGameState->local.fadeInfo.start_r * percent_start) + (ONgGameState->local.fadeInfo.end_r * percent_end);
	curG = (ONgGameState->local.fadeInfo.start_g * percent_start) + (ONgGameState->local.fadeInfo.end_g * percent_end);
	curB = (ONgGameState->local.fadeInfo.start_b * percent_start) + (ONgGameState->local.fadeInfo.end_b * percent_end);

	if (curA > 0.f) {
		ONrBlanketScreen(curA, curR, curG, curB);
	}
	else {
		ONgGameState->local.fadeInfo.active = UUcFalse;
	}

	return;
}

static void ONiGameState_Display_Overlay_Elements(Display_Performance_Variables	*ioPerfVars)
{
	UUtBool is_skipping_cutscene = (ONgGameState->local.in_cutscene) && (ONcCutsceneSkip_skipping == ONgGameState->local.cutscene_skip_mode);

	// commit the environment alpha
	M3rGeom_State_Push();
		M3rGeom_State_Set(M3cGeomStateIntType_SubmitMode, M3cGeomState_SubmitMode_Normal);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
		M3rGeom_State_Commit();
		M3rGeom_Draw_Environment_Alpha();
	M3rGeom_State_Pop();

	// commit the alpha
	M3rGeom_State_Push();
		M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);
		M3rDraw_Commit_Alpha();
	M3rGeom_State_Pop();

	ONrGameState_DisplayAiming();

	// Draw the ui elements & fade
	M3rGeom_State_Push();
		M3rGeom_State_Set(M3cGeomStateIntType_SubmitMode, M3cGeomState_SubmitMode_Normal);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare, M3cDrawState_ZCompare_Off);
		M3rDraw_State_SetInt(M3cDrawStateIntType_Fog, M3cDrawStateFogDisable);
		M3rGeom_State_Commit();

		if ((!is_skipping_cutscene) && (ONgParticle_Display)) {
			// display lensflare particles
			P3rDisplay(UUcTrue);
		}

		if (ONgShow_Sky) {
			ONrSky_DrawLensFlare(&ONgGameState->sky);
		}

		M3rDraw_Commit_Alpha();
		M3rGeom_State_Set(M3cGeomStateIntType_SubmitMode, M3cGeomState_SubmitMode_Normal);
		M3rGeom_State_Commit();

		ONrGameState_LetterBox_Display(&ONgGameState->local.letterbox);

#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
		ONi_GameState_Display_All_UI_Elements(ioPerfVars);
#else
		ONi_GameState_Display_All_UI_Elements(NULL);
#endif

		SSrShowDebugInfo();

		if (ONgFontCache_Display) {
			TSrFont_DisplayCacheTexture();
		}

		COrConsole_StatusLine_Display();

		ONi_GameState_Display_FadeInOut();

		if ((!ONgGameState->local.letterbox.active) && (!ONgGameState->local.fadeInfo.active)) {
			// Display the text messages
			COrConsole_Display_Messages();
		}

		// Display the console
		COrConsole_Display_Lines();

		// draw the cinematics
		OCrCinematic_Draw();

		ONi_GameState_Display_SkipCutsceneFade();

		COrConsole_Display_Console();
	M3rGeom_State_Pop();

	return;
}

#if TOOL_VERSION
static void ONiGameState_Display_NonReflectable_Tool(Display_Performance_Variables	*ioPerfVars)
{
	if (ONgShowQuadCount) {
		sprintf(quad_count_display[1].text, "quad count %d / %d",
			AKrEnvironment_GetVisCount(ONgGameState->level->environment),
			ONgGameState->level->environment->gqRenderArray->numGQs);
	}

	if (ONgShowObjectCount) {
		UUtUns32 object_itr;
		UUtUns32 object_count = 0;

		for(object_itr = 0; object_itr < ONgGameState->objects->numObjects; object_itr++)
		{
			if (ONgGameState->objects->object_list[object_itr].flags & OBcFlags_InUse) {
				object_count++;
			}
		}

		sprintf(object_count_display[1].text, "object count %d/%d [watermark %d]",
			object_count, ONgGameState->objects->maxObjects, ONgGameState->objects->numObjects);
	}

	if (ONgShowPhysicsCount) {
		UUtUns32 physics_itr;
		UUtUns32 physics_count = 0;

		for(physics_itr = 0; physics_itr < PHgPhysicsContextArray->num_contexts; physics_itr++)
		{
			if (PHgPhysicsContextArray->contexts[physics_itr].flags & PHcFlags_Physics_InUse) {
				physics_count++;
			}
		}

		sprintf(physics_count_display[1].text, "physics count %d/%d [watermark %d]", physics_count, PHgPhysicsContextArray->max_contexts, PHgPhysicsContextArray->num_contexts);
	}

	if (ONgShowScriptingCount) {
		SLtScript_Debug_Counters debug_counters;

		SLrScript_Debug_Counters(&debug_counters);

		sprintf(script_count_display[1].text, "scripting count %d/%d [watermark %d]", debug_counters.scripts_running, debug_counters.scripts_running_max, debug_counters.scripts_running_watermark);
	}

}
#endif

static UUtError
ONiGameState_Display_NonReflectable(
	Display_Performance_Variables	*ioPerfVars)
{
	UUtError		error;
	M3tGeomCamera*	activeCamera;
	UUtBool			render_sky_this_frame = UUcTrue;
	UUtBool			is_skipping_cutscene = (ONgGameState->local.in_cutscene) && (ONcCutsceneSkip_skipping == ONgGameState->local.cutscene_skip_mode);

	// start the environment
	if (ONgShow_Environment)
	{
		#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
		ioPerfVars->time_env -= UUrMachineTime_High();
		#endif

		M3rCamera_GetActive(&activeCamera);

		error = AKrEnvironment_StartFrame(ONgGameState->level->environment, ONgVisibilityCamera, &render_sky_this_frame);
		UUmError_ReturnOnError(error);

		render_sky_this_frame &= !is_skipping_cutscene;

		#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
		ioPerfVars->time_env += UUrMachineTime_High();
		#endif

#if 0
		M3rMinMaxBBox_Draw_Line(&ONgGameState->level->environment->visible_bbox, IMcShade_White);

		COrConsole_Printf("x %f, %f",
			ONgGameState->level->environment->visible_bbox.minPoint.x,
			ONgGameState->level->environment->visible_bbox.maxPoint.x);
		COrConsole_Printf("y %f, %f",
			ONgGameState->level->environment->visible_bbox.minPoint.y,
			ONgGameState->level->environment->visible_bbox.maxPoint.y);
		COrConsole_Printf("z %f, %f",
			ONgGameState->level->environment->visible_bbox.minPoint.z,
			ONgGameState->level->environment->visible_bbox.maxPoint.z);
#endif
	}

	if (render_sky_this_frame)
	{
		// don't fog the sky - S.S.
		M3rDraw_State_Push();
		M3rDraw_State_SetInt(M3cDrawStateIntType_Fog, M3cDrawStateFogDisable);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare, M3cDrawState_ZCompare_Off);
		M3rDraw_State_Commit();

		ONrSky_Draw(&ONgGameState->sky);

		M3rDraw_State_Pop();
	}

	if ((AKgHighlightGQIndex >= 0) && (AKgHighlightGQIndex < ONgGameState->level->environment->gqGeneralArray->numGQs)) {
		UUtUns32 itr;
		M3tPoint3D points[4], midpoint;
		AKtGQ_General *gq = ONgGameState->level->environment->gqGeneralArray->gqGeneral + AKgHighlightGQIndex;

		// draw a debugging outline around the highlighted GQ
		MUmVector_Set(midpoint, 0, 0, 0);
		for (itr = 0; itr < 4; itr++) {
			points[itr] = ONgGameState->level->environment->pointArray->points[gq->m3Quad.vertexIndices.indices[itr]];
			MUmVector_ScaleIncrement(midpoint, 0.25f, points[itr]);
		}

		for (itr = 0; itr < 4; itr++) {
			M3rGeom_Line_Light(&points[itr], &points[(itr + 1) % 4], IMcShade_Yellow);
		}
		M3rGeom_Line_Light(&ONgGameState->local.playerCharacter->actual_position, &midpoint, IMcShade_Yellow);
	}


	P3rNotifySkyVisible(render_sky_this_frame);

	// End the environment
	if (ONgShow_Environment)
	{
		#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
		ioPerfVars->time_env -= UUrMachineTime_High();
		#endif

		M3rDraw_State_SetInt(M3cDrawStateIntType_Time, ONgGameState->gameTime);

		error = AKrEnvironment_EndFrame(ONgGameState->level->environment);
		UUmError_ReturnOnError(error);

		#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
		ioPerfVars->time_env += UUrMachineTime_High();
		#endif
	}

#if TOOL_VERSION
	ONiGameState_Display_NonReflectable_Tool(ioPerfVars);
#endif

	// Draw the cross hair
	AMrRenderCrosshair();

	return UUcError_None;
}

static UUtError
ONiGameState_Display_Reflectable(
	Display_Performance_Variables	*ioPerfVars)
{
	UUtStallTimer reflectable;
	UUtStallTimer reflectable_internal;
	UUtBool is_skipping_cutscene = (ONgGameState->local.in_cutscene) && (ONcCutsceneSkip_skipping == ONgGameState->local.cutscene_skip_mode);

	UUrStallTimer_Begin(&reflectable);

	UUrStallTimer_Begin(&reflectable_internal);

	if (ONgLineSize > 0) {
		COrConsole_Printf("drawing line of %d parts", ONgLineSize);
		M3rGeometry_LineDraw((UUtUns16) ONgLineSize, ONgLine, IMcShade_White);
	}

	UUrStallTimer_End(&reflectable_internal, "ONiGameState_Display_Reflectable - line drawing");


	UUrStallTimer_Begin(&reflectable_internal);

	// Draw the characters
	if(ONgShow_Characters)
	{
		UUtUns32	itr=0;

		#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
		ioPerfVars->time_chr -= UUrMachineTime_High();
		#endif

		M3rDraw_State_SetInt(M3cDrawStateIntType_Time, ONgGameState->gameTime);

		ONrGameState_DisplayCharacters();
		ONrGameState_MotionBlur_Display();

		#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
		ioPerfVars->time_chr += UUrMachineTime_High();
		#endif
	}

	UUrStallTimer_End(&reflectable_internal, "ONiGameState_Display_Reflectable - characters");

	// Draw the object list
	#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
	ioPerfVars->time_obj -= UUrMachineTime_High();
	#endif

	if (ONgShow_Objects) {
		M3rDraw_State_SetInt(M3cDrawStateIntType_Time, ONgGameState->gameTime);

		UUrStallTimer_Begin(&reflectable_internal);
		OBrList_Draw(ONgGameState->objects);
		UUrStallTimer_End(&reflectable_internal, "ONiGameState_Display_Reflectable - ONiGameState_Display_Reflectable");

		UUrStallTimer_Begin(&reflectable_internal);
		OBJrDrawObjects();
		UUrStallTimer_End(&reflectable_internal, "ONiGameState_Display_Reflectable - OBJrDrawObjects");

		// CB: this isn't actually particle drawing, but rather just the
		// little marker triangles around environmental particles.
		UUrStallTimer_Begin(&reflectable_internal);
		EPrDisplay();
		UUrStallTimer_End(&reflectable_internal, "ONiGameState_Display_Reflectable - EPrDisplay");
	}

	#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
	ioPerfVars->time_obj += UUrMachineTime_High();
	#endif

	// Draw the weapons
	UUrStallTimer_Begin(&reflectable_internal);

	WPrDisplay();

	UUrStallTimer_End(&reflectable_internal, "ONiGameState_Display_Reflectable - WPrDisplay");

	// Draw the effects
	#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
	ioPerfVars->time_prt -= UUrMachineTime_High();
	#endif

	UUrStallTimer_Begin(&reflectable_internal);

	if (!is_skipping_cutscene) {
		M3rDraw_State_SetInt(M3cDrawStateIntType_Time, ONgGameState->gameTime);

		M3rGeom_State_Set(M3cGeomStateIntType_SubmitMode, M3cGeomState_SubmitMode_Normal);
		M3rGeom_State_Commit();
		P3rDisplayStaticDecals( );
		P3rDisplayDynamicDecals( );
		M3rGeom_State_Set(M3cGeomStateIntType_SubmitMode, M3cGeomState_SubmitMode_SortAlphaTris);
		M3rGeom_State_Commit();
	}

	UUrStallTimer_End(&reflectable_internal, "ONiGameState_Display_Reflectable - decals");

	UUrStallTimer_Begin(&reflectable_internal);

	if (!is_skipping_cutscene && ONgParticle_Display)
	{
		P3rDisplay(UUcFalse);	// display all non-lensflare classes
//		FXrEffect_Display();
	}

	UUrStallTimer_End(&reflectable_internal, "ONiGameState_Display_Reflectable - particles");

	#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
	ioPerfVars->time_prt += UUrMachineTime_High();
	#endif

	UUrStallTimer_Begin(&reflectable_internal);

	// Draw the ammo
	WPrPowerup_Display();

	UUrStallTimer_End(&reflectable_internal, "ONiGameState_Display_Reflectable - powerup display");

#if TOOL_VERSION
	UUrStallTimer_Begin(&reflectable_internal);
	// DEBUGGING: draw currently selected AI path
	OWrAI_Display();
	OSrDebugDisplay();
	UUrStallTimer_End(&reflectable_internal, "ONiGameState_Display_Reflectable - TOOL_VERSION ai & osr deubg");
#endif

	UUrStallTimer_End(&reflectable, "ONiGameState_Display_Reflectable");

	return UUcError_None;
}

void ONrGameState_Display(void)
{
	UUtStallTimer stall_timer_display;
	UUtStallTimer stall_timer_display_internal;

#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
	Display_Performance_Variables perf_var;

	perf_var.time_total = 0;
	perf_var.time_chr = 0;
	perf_var.time_env = 0;
	perf_var.time_obj = 0;
	perf_var.time_prt = 0;
#endif

	UUrStallTimer_Begin(&stall_timer_display);

	AKrEnvironment_FastMode(ONrGameState_IsSkippingCutscene());

	UUmAssertReadPtr(ONgGameState->level->environment,sizeof(AKtEnvironment));

	#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
	perf_var.time_total -= UUrMachineTime_High();
	#endif

	MUmVector_Verify(ONgGameState->local.camera->viewData.viewVector);

	M3rGeom_State_Set(M3cGeomStateIntType_SubmitMode, M3cGeomState_SubmitMode_SortAlphaTris);
	M3rGeom_State_Set(M3cGeomStateIntType_Shade, ONgMotoko_ShadeVertex ? M3cGeomState_Shade_Vertex : M3cGeomState_Shade_Face);
	M3rGeom_State_Set(M3cGeomStateIntType_Fill, ONgMotoko_FillSolid ? M3cGeomState_Fill_Solid : M3cGeomState_Fill_Line);
	M3rGeom_State_Set(M3cGeomStateIntType_Appearance, ONgMotoko_Texture ? M3cGeomState_Appearance_Texture : M3cGeomState_Appearance_Gouraud);
	M3rGeom_State_Set(M3cGeomStateIntType_FastMode, ONrGameState_IsSkippingCutscene());

	M3rGeom_State_Set(M3cGeomStateIntType_Alpha, 0xff);

	// These have no geom state equivalent
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare, ONgMotoko_ZCompareOn ? M3cDrawState_ZCompare_On : M3cDrawState_ZCompare_Off);
	M3rDraw_State_SetInt(M3cDrawStateIntType_BufferClear, ONgMotoko_BufferClear ? M3cDrawState_BufferClear_On : M3cDrawState_BufferClear_Off);
	M3rDraw_State_SetInt(M3cDrawStateIntType_DoubleBuffer, ONgMotoko_DoubleBuffer ? M3cDrawState_DoubleBuffer_On : M3cDrawState_DoubleBuffer_Off);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_On);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ClearColor, ONgMotoko_ClearColor);
	M3rGeom_State_Commit();

	// main drawing
	UUrStallTimer_Begin(&stall_timer_display_internal);
		ONiGameState_Display_NonReflectable(&perf_var);
	UUrStallTimer_End(&stall_timer_display_internal, "Display - ONiGameState_Display_NonReflectable");

	UUrStallTimer_Begin(&stall_timer_display_internal);
		ONiGameState_Display_Reflectable(&perf_var);
	UUrStallTimer_End(&stall_timer_display_internal, "Display - ONiGameState_Display_Reflectable");

	// call this last, these elements just overlay
	UUrStallTimer_Begin(&stall_timer_display_internal);
		ONiGameState_Display_Overlay_Elements(&perf_var);
	UUrStallTimer_End(&stall_timer_display_internal, "Display - ONiGameState_Display_Overlay_Elements");

	#if defined(BRENTS_CHEESY_GSD_PERF) && BRENTS_CHEESY_GSD_PERF
	perf_var.time_total += UUrMachineTime_High();
	#endif

	ONgPerformanceData = *M3rDraw_Counters_Get();

	UUrStallTimer_Begin(&stall_timer_display_internal);
		UUrPlatform_Idle(ONgPlatformData.gameWindow, ONgSleep ? 4 : 0);
	UUrStallTimer_End(&stall_timer_display_internal, "Display - UUrPlatform_Idle");

	UUrStallTimer_End(&stall_timer_display, "ONrGameState_Display");

	return;
}

// should be perhaps get and process
void ONrGameState_UpdateServerTime(
	ONtGameState*	ioGameState)
{
	// game time
	UUtUns32 multiplier = ((ONgGameState->local.slowMotionEnable) || (ONgGameState->local.slowMotionTimer > 0)) ? 2 : 1;
	UUtUns32 machineTime =	UUrMachineTime_Sixtieths();
	UUtUns32 deltaMachineTime;

	// calculate the number of ticks that have passed for the server
	deltaMachineTime = machineTime - ioGameState->server.machineTimeLast;
	deltaMachineTime -= (deltaMachineTime % multiplier);

	ioGameState->server.machineTimeLast += deltaMachineTime;

	// this is the new time
	ioGameState->serverTime += (deltaMachineTime / multiplier);

	return;
}

#if 0
static UUtError
iDebugExportGunk(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError error;

	error = OBJrENVFile_Write(UUcTrue);
	COrConsole_Printf("gunk export %s", (error == UUcError_None) ? "succeeded" : "failed!");

	return UUcError_None;
}
#endif

static UUtError
iDebugEnvAnim(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	char *animation_name = inParameterList[0].val.str;
	OBtAnimation *animation;
	UUtError error;

	COrConsole_Printf("debug_env_anim %s", animation_name);

	error = TMrInstance_GetDataPtr(OBcTemplate_Animation, animation_name, &animation);

	if (UUcError_None == error) {
		ONgLineSize = animation->numFrames;
		COrConsole_Printf("found %d frames", ONgLineSize);
	}
	else {
		ONgLineSize = 0;
		COrConsole_Printf("not found");
	}

	ONgLine = UUrMemory_Block_Realloc(ONgLine, sizeof(M3tPoint3D) * ONgLineSize);

	{
		UUtInt32 itr;

		for(itr = 0; itr < ONgLineSize; itr++)
		{
			M3tMatrix4x3 matrix;

			OBrAnimation_GetMatrix(animation, itr, 0, &matrix);

			ONgLine[itr] = MUrMatrix_GetTranslation(&matrix);
		}
	}

	return UUcError_None;
}


UUtError
ONrGameState_Initialize(
	void)
{
	UUtError	error;
	SLtRegisterBoolTable bool_table[] =
	{
#if CONSOLE_DEBUGGING_COMMANDS
		{ "gs_show_particles", "Turns on the drawing of particles", &ONgParticle_Display },
		{ "gs_show_sky", "Turns on the sky", &ONgShow_Sky },
		{ "gs_show_characters", "Turns on the drawing of characters", &ONgShow_Characters },
		{ "gs_show_objects", "Turns on the drawing of objects", &ONgShow_Objects },
		{ "gs_show_ui", "Turns on the ui", &ONgShow_UI },
		{ "chr_debug_handle", "turns no debugging for the handle code", &ONgDebugHandle },
		{ "gs_stable_ear", "makes the ear stable", &ONgStableEar },
		{ "env_show_quad_count", "shows the count of visable environment quads", &ONgShowQuadCount },
		{ "gs_show_object_count", "shows the count of objects", &ONgShowObjectCount },
		{ "gs_show_physics_count", "shows the count of physics", &ONgShowPhysicsCount},
		{ "gs_show_scripting_count", "shows the count of active scripting contexts", &ONgShowScriptingCount},
		{ "single_step", "puts the game in single step mode", &ONgSingleStep },
		{ "wait_for_key",	"makes the game wait for a key before level load", &ONgWaitForKey },
		{ "draw_every_frame", "forces drawing of every frame", &ONgDrawEveryFrame },
		{ "fast_mode", "makes the game run fast", &ONgFastMode },
		{ "show_performance", "Enables performance display", &ONgShowPerformance_Overall },
		{ "show_performance_gsd", "Enables performance display", &ONgShowPerformance_GSD },
		{ "show_performance_gsu",	"Enables performance display", &ONgShowPerformance_GSU },
		{ "gs_sleep", "Turns on a call to sleep to make debugging easier", &ONgSleep },
		{ "gs_show_environment", "Turns on doing the environment", &ONgShow_Environment },
		{ "ob_show_debug", "enable physics debugging visuals", &OBgObjectsShowDebug },
		{ "debug_consoles", "prints debugging information about console usage", &ONgDebugConsoleAction },
		{ "debug_font_cache", "displays font cache texture", &ONgFontCache_Display },
		{ "fast_lookup", "enables fast lookup", &ONgFastLookups },
		{ "sync_debug", "enables sync debugging", &ONgSyncDebug },
#endif
		{ "invincible", "makes player invincible", &ONgPlayerInvincible },
		{ "omnipotent", "makes player omnipotent", &ONgPlayerOmnipotent },
		{ "unstoppable", "makes player unstoppable", &ONgPlayerUnstoppable },
		{ NULL, NULL, NULL }
	};

	SLtRegisterFloatTable float_table[] =
	{
#if CONSOLE_DEBUGGING_COMMANDS
		{ "gs_input_accel", "Use this to control the input sensitivity", &ONgGameState_InputAccel },
#endif
		{ NULL, NULL, NULL }
	};

	SLtRegisterInt32Table int32_table[] =
	{
#if CONSOLE_DEBUGGING_COMMANDS
		{ "gs_screen_shot_reduce", "2^n amount of reduction", &ONgScreenShotReduceAmount },
		{ "chr_buffer_size", "0 = normal buffer, otherwise buffer duration", &ONgAnimBuffer },
		{ "draw_every_frame_multiple", "draw every frame multiple", &ONgDrawEveryFrameMultiple },
#endif
		{ "save_point", "which save point we are on", &ONgContinueSavePoint },
		{ NULL, NULL, NULL }
	};

	SLtRegisterVoidFunctionTable scripting_function_table[] =
	{
#if CONSOLE_DEBUGGING_COMMANDS
		{ "perf_prefix", "sets the perf prefix", NULL, ONrTimerPrefix },
		{ "gs_fov_set", "sets the field of view", "fov_degrees:float", ONrGameState_FieldOfView_Set },
		{ "debug_env_anim", "draws a line for an environment animation", "name:string", iDebugEnvAnim },
#endif
		{ "gs_farclipplane_set", "sets the far clipping plane", "plane:float", ONrGameState_FarClipPlane_Set },
//		{ "debug_export_gunk", "exports gunk and writes debugging info", "", iDebugExportGunk },
		{ NULL, NULL, NULL, NULL }
	};

	SLrGlobalVariable_Register_Bool_Table(bool_table);
	SLrGlobalVariable_Register_Int32_Table(int32_table);
	SLrGlobalVariable_Register_Float_Table(float_table);
	SLrScript_CommandTable_Register_Void(scripting_function_table);

#if PERFORMANCE_TIMER
	COrConsole_StatusLines_Begin(perf_overall, &ONgShowPerformance_Overall, 14);
#else
	COrConsole_StatusLines_Begin(game_fps, &ONgShowPerformance_Overall, 1);
	strcpy(game_fps[0].text, "");
	COrConsole_StatusLines_Begin(texture_info, &ONgShowPerformance_Overall, 1);
	strcpy(texture_info[0].text, "");
#endif

#if TOOL_VERSION
	// Global initializations go here
	COrConsole_StatusLines_Begin(perf_gsd, &ONgShowPerformance_GSD, 4);
	COrConsole_StatusLines_Begin(perf_gsu, &ONgShowPerformance_GSU, 4);
	COrConsole_StatusLines_Begin(occl_display, &AKgDraw_Occl, 1);
	COrConsole_StatusLines_Begin(soundoccl_display, &AKgDraw_SoundOccl, 1);
	COrConsole_StatusLines_Begin(chr_col_display, &AKgDraw_CharacterCollision, 1);
	COrConsole_StatusLines_Begin(obj_col_display, &AKgDraw_ObjectCollision, 1);
	COrConsole_StatusLines_Begin(invis_display, &AKgDraw_Invis, 1);
	COrConsole_StatusLines_Begin(frame_display, &ONgDrawEveryFrame, 1);
	COrConsole_StatusLines_Begin(quad_count_display, &ONgShowQuadCount, 2);
	COrConsole_StatusLines_Begin(object_count_display, &ONgShowObjectCount, 2);
	COrConsole_StatusLines_Begin(physics_count_display, &ONgShowPhysicsCount, 2);
	COrConsole_StatusLines_Begin(script_count_display, &ONgShowScriptingCount, 2);

	strcpy(perf_overall[0].text, "*** perf overall ***");
	strcpy(perf_gsd[0].text, "*** perf display ***");
	strcpy(perf_gsu[0].text, "*** perf update ***");
	strcpy(occl_display[0].text, "*** env draw occl ***");
	strcpy(soundoccl_display[0].text, "*** env draw sound occl ***");
	strcpy(chr_col_display[0].text, "*** env draw chr col ***");
	strcpy(obj_col_display[0].text, "*** env draw obj col ***");
	strcpy(invis_display[0].text, "*** env draw invis ***");
	strcpy(frame_display[0].text,	"*** draw every frame ***");
	strcpy(quad_count_display[0].text,	"*** quad count display ***");
	strcpy(quad_count_display[1].text, "");
	strcpy(object_count_display[0].text,	"*** object count display ***");
	strcpy(object_count_display[1].text, "");
	strcpy(physics_count_display[0].text,	"*** physics count display ***");
	strcpy(physics_count_display[1].text, "");
	strcpy(script_count_display[0].text,	"*** script count display ***");
	strcpy(script_count_display[1].text, "");
#endif

	ONgGameState = (ONtGameState *) UUrMemory_Block_NewClear(sizeof(ONtGameState));
	if (NULL == ONgGameState)
	{
		UUmError_ReturnOnError(UUcError_OutOfMemory);
	}

	CArInitialize();

	error = AI2rInstallConsoleVariables();
	UUmError_ReturnOnError(error);

	// initialize the gamestate dialogs
/*	error = ONrGameState_Dialog_Initialize();
	UUmError_ReturnOnError(error);*/

	// Install the console variables for the characters
	error = ONrGameState_InstallConsoleVariables();
	UUmError_ReturnOnError(error);

	// Install the console variables for the weapons
	error = WPrInitialize();
	UUmError_ReturnOnError(error);

	// Install the console variables for Oni_FX
	error = FXrEffects_Initialize();
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

void
ONrGameState_Terminate(
	void)
{
	if(ONgGameState != NULL)
	{
		UUmAssert(ONgGameState == ONgGameState);

		// Delete the ganestate
		UUrMemory_Block_Delete(ONgGameState);
		ONgGameState = NULL;
	}

	// terminate the gamestate dialogs
//	ONrGameState_Dialog_Terminate();

	return;
}

// ----------------------------------------------------------------------
// Accessors to the fields of ONgGameState
// ----------------------------------------------------------------------
ONtCharacter*
ONrGameState_GetCharacterList(
	void)
{
	return ONgGameState->characters;
}

AKtEnvironment*
ONrGameState_GetEnvironment(
	void)
{
	AKtEnvironment *environment = NULL;

	if (NULL == ONgGameState) {
		goto exit;
	}

	if (NULL == ONgGameState->level) {
		goto exit;
	}

	environment = ONgGameState->level->environment;

exit:
	return environment;
}

UUtUns32
ONrGameState_GetGameTime(
	void)
{
	return ONgGameState->gameTime;
}

PHtGraph*
ONrGameState_GetGraph(
	void)
{
	return &ONgGameState->local.pathGraph;
}

ONtCharacter*
ONrGameState_GetPlayerCharacter(
	void)
{
//	UUmAssert(ONgGameState->local.playerCharacter);
	return ONgGameState->local.playerCharacter;
}

UUtUns16
ONrGameState_GetPlayerNum(
	void)
{
	if (ONgGameState->local.playerCharacter) {
		return ONrCharacter_GetIndex(ONgGameState->local.playerCharacter);
	} else {
		return ONcCharacterIndex_None;
	}
}

UUtUns16
ONrGameState_GetNumCharacters(
	void)
{
	return ONgGameState->numCharacters;
}

OBtObjectList*
ONrGameState_GetObjectList(
	void)
{
	return ONgGameState->objects;
}

void
ONrGameState_SetPlayerNum(
	UUtUns16			inPlayerNum)
{
	if (inPlayerNum == UUcMaxUns16)
	{
		ONgGameState->local.playerCharacter = NULL;
		ONgGameState->local.playerActiveCharacter = NULL;
	}
	else
	{
		ONgGameState->local.playerCharacter =
			ONgGameState->characters + inPlayerNum;
		ONgGameState->local.playerActiveCharacter = ONrGetActiveCharacter(ONgGameState->local.playerCharacter);
		UUmAssert(ONgGameState->local.playerCharacter->flags & ONcCharacterFlag_InUse);
		UUmAssert(ONgGameState->local.playerActiveCharacter != NULL);
	}
}

UUtBool
ONrGameState_IsPaused(
	void)
{
	return ONgGameState->paused;
}

// ----------------------------------------------------------------------
void
ONrGameState_Pause(
	UUtBool				inPause)
{
	static UUtProfileState	savedProfileState = UUcProfile_State_Off;

	ONgGameState->paused = inPause;

	if (inPause == UUcFalse)
	{
		// have to remove cMaxTicksPerFrame or else the
		// camera doesn't get setup correctly.
		ONgGameState->server.machineTimeLast =
			UUrMachineTime_Sixtieths() -
			cMaxTicksPerFrame;

		#if defined(PROFILE) && PROFILE

			UUrProfile_State_Set(savedProfileState);

		#endif
	}
	else
	{
		#if defined(PROFILE) && PROFILE

			savedProfileState = UUrProfile_State_Get();

			UUrProfile_State_Set(UUcProfile_State_Off);

		#endif
	}
}

UUtError ONrGameState_LevelBegin_Objects(void)
{
	/****************
	* Sets up objects for a level
	*/

	ONgGameState->objects = OBrList_New(ONcMaxObjects);
	if (!ONgGameState->objects) return UUcError_OutOfMemory;

	return UUcError_None;
}

void ONrGameState_LevelEnd_Objects(void)
{
	/****************
	* Kills objects for a level
	*/

	OBrList_Delete(ONgGameState->objects);
}

void
ONrGameState_ResetObjects(void)
{
	// For debugging only
	ONrGameState_LevelEnd_Objects();
	ONrGameState_LevelBegin_Objects();
}

// ----------------------------------------------------------------------
// this function erases the character array
void
ONrGameState_ResetCharacters(
	void)
{
	UUtUns16			i;

	// delete all of the characters
	for (i = 0; i < ONgGameState->numCharacters; i++)
	{
		ONrGameState_DeleteCharacter(ONgGameState->characters + i);
	}
	ONgGameState->numCharacters = 0;

	ONrGameState_SetPlayerNum(UUcMaxUns16);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================

ONtCharacter **ONrGameState_ActiveCharacterList_Get(void)
{
	return ONgGameState->activeCharacters;
}


UUtUns32 ONrGameState_ActiveCharacterList_Count(void)
{
	return ONgGameState->numActiveCharacters;
}

void ONrGameState_ActiveCharacterList_Lock()
{
	ONgGameState->activeCharacterLockDepth += 1;

	return;
}

void ONrGameState_ActiveCharacterList_Unlock()
{
	UUmAssert(ONgGameState->activeCharacterLockDepth > 0);

	ONgGameState->activeCharacterLockDepth -= 1;

	return;
}

void ONrGameState_ActiveCharacterList_Add(ONtCharacter *inCharacter)
{
	UUtUns16 itr;

	UUmAssert(0 == ONgGameState->activeCharacterLockDepth);

	for(itr = 0; itr < ONgGameState->numActiveCharacters; itr++)
	{
		ONtCharacter *curCharacter = ONgGameState->activeCharacters[itr];

		if (curCharacter == inCharacter) { return; }
	}

	ONgGameState->activeCharacters[ONgGameState->numActiveCharacters] = inCharacter;
	ONgGameState->numActiveCharacters++;

	return;
}

void ONrGameState_ActiveCharacterList_Remove(ONtCharacter *inCharacter)
{
	UUtUns16 itr;

	UUmAssert(0 == ONgGameState->activeCharacterLockDepth);

	for(itr = 0; itr < ONgGameState->numActiveCharacters; itr++)
	{
		ONtCharacter *curCharacter = ONgGameState->activeCharacters[itr];

		if (curCharacter == inCharacter) {
			ONgGameState->numActiveCharacters--;
			ONgGameState->activeCharacters[itr] =
				ONgGameState->activeCharacters[ONgGameState->numActiveCharacters];
			return;
		}
	}

	return;
}

ONtCharacter **ONrGameState_LivingCharacterList_Get(void)
{
	return ONgGameState->livingCharacters;
}


UUtUns32 ONrGameState_LivingCharacterList_Count(void)
{
	return ONgGameState->numLivingCharacters;
}

void ONrGameState_LivingCharacterList_Lock()
{
	ONgGameState->livingCharacterLockDepth += 1;

	return;
}

void ONrGameState_LivingCharacterList_Unlock()
{
	UUmAssert(ONgGameState->livingCharacterLockDepth > 0);

	ONgGameState->livingCharacterLockDepth -= 1;

	return;
}

void ONrGameState_LivingCharacterList_Add(ONtCharacter *inCharacter)
{
	UUtUns16 itr;

	UUmAssert(0 == ONgGameState->livingCharacterLockDepth);

	for(itr = 0; itr < ONgGameState->numLivingCharacters; itr++)
	{
		ONtCharacter *curCharacter = ONgGameState->livingCharacters[itr];

		if (curCharacter == inCharacter) { return; }
	}

	ONgGameState->livingCharacters[ONgGameState->numLivingCharacters] = inCharacter;
	ONgGameState->numLivingCharacters++;

	return;
}

void ONrGameState_LivingCharacterList_Remove(ONtCharacter *inCharacter)
{
	UUtUns16 itr;

	UUmAssert(0 == ONgGameState->livingCharacterLockDepth);

	for(itr = 0; itr < ONgGameState->numLivingCharacters; itr++)
	{
		ONtCharacter *curCharacter = ONgGameState->livingCharacters[itr];

		if (curCharacter == inCharacter) {
			ONgGameState->numLivingCharacters--;
			ONgGameState->livingCharacters[itr] =
				ONgGameState->livingCharacters[ONgGameState->numLivingCharacters];
			return;
		}
	}

	return;
}

ONtCharacter **ONrGameState_PresentCharacterList_Get(void)
{
	return ONgGameState->presentCharacters;
}

UUtUns32 ONrGameState_PresentCharacterList_Count(void)
{
	return ONgGameState->numPresentCharacters;
}

void ONrGameState_PresentCharacterList_Lock()
{
	ONgGameState->presentCharacterLockDepth += 1;
}

void ONrGameState_PresentCharacterList_Unlock()
{
	UUmAssert(ONgGameState->presentCharacterLockDepth > 0);

	ONgGameState->presentCharacterLockDepth -= 1;
}

void ONrGameState_PresentCharacterList_Add(ONtCharacter *inCharacter)
{
	UUtUns16 itr;

	UUmAssert(0 == ONgGameState->presentCharacterLockDepth);

	for(itr = 0; itr < ONgGameState->numPresentCharacters; itr++)
	{
		ONtCharacter *curCharacter = ONgGameState->presentCharacters[itr];

		if (curCharacter == inCharacter) { return; }
	}

	ONgGameState->presentCharacters[ONgGameState->numPresentCharacters] = inCharacter;
	ONgGameState->numPresentCharacters++;

	return;
}

void ONrGameState_PresentCharacterList_Remove(ONtCharacter *inCharacter)
{
	UUtUns16 itr;

	UUmAssert(0 == ONgGameState->presentCharacterLockDepth);

	for(itr = 0; itr < ONgGameState->numPresentCharacters; itr++)
	{
		ONtCharacter *curCharacter = ONgGameState->presentCharacters[itr];

		if (curCharacter == inCharacter) {
			ONgGameState->numPresentCharacters--;
			ONgGameState->presentCharacters[itr] =
				ONgGameState->presentCharacters[ONgGameState->numPresentCharacters];
			return;
		}
	}

	return;
}

void ONrGameState_FadeOut(UUtUns32 inNumFrames, float inR, float inG, float inB)
{
	ONgGameState->local.fadeInfo.start_a = 0.f;
	ONgGameState->local.fadeInfo.start_r = inR;
	ONgGameState->local.fadeInfo.start_g = inG;
	ONgGameState->local.fadeInfo.start_b = inB;

	ONgGameState->local.fadeInfo.end_a = 1.f;
	ONgGameState->local.fadeInfo.end_r = inR;
	ONgGameState->local.fadeInfo.end_g = inG;
	ONgGameState->local.fadeInfo.end_b = inB;

	ONgGameState->local.fadeInfo.active = UUcTrue;
	ONgGameState->local.fadeInfo.startTime = ONgGameState->gameTime;
	ONgGameState->local.fadeInfo.endTime = ONgGameState->gameTime + inNumFrames;

	return;
}

void ONrGameState_FadeIn(UUtUns32 inNumFrames)
{
	if (ONgGameState->local.fadeInfo.active)
	{
		ONgGameState->local.fadeInfo.startTime = ONgGameState->gameTime;
		ONgGameState->local.fadeInfo.endTime = ONgGameState->gameTime + inNumFrames;

		ONgGameState->local.fadeInfo.start_a = ONgGameState->local.fadeInfo.end_a;
		ONgGameState->local.fadeInfo.start_r = ONgGameState->local.fadeInfo.end_r;
		ONgGameState->local.fadeInfo.start_g = ONgGameState->local.fadeInfo.end_g;
		ONgGameState->local.fadeInfo.start_b = ONgGameState->local.fadeInfo.end_b;

		ONgGameState->local.fadeInfo.end_a = 0.f;
		ONgGameState->local.fadeInfo.end_r = ONgGameState->local.fadeInfo.end_r;
		ONgGameState->local.fadeInfo.end_g = ONgGameState->local.fadeInfo.end_g;
		ONgGameState->local.fadeInfo.end_b = ONgGameState->local.fadeInfo.end_b;
	}

	return;
}

static void ONrTrigger_DisplaySide(const ONtTrigger *inTrigger, M3tBBoxSide inSide, UUtUns32 inShade)
{
	const M3tQuad *quad = M3rBBox_GetSide(inSide);

	if (NULL != quad) {
		M3rGeom_Line_Light(inTrigger->volume.worldPoints + quad->indices[0], inTrigger->volume.worldPoints + quad->indices[2], inShade);
		M3rGeom_Line_Light(inTrigger->volume.worldPoints + quad->indices[1], inTrigger->volume.worldPoints + quad->indices[3], inShade);
	}

	return;
}

static void ONrGameState_MotionBlur_Display_One(ONtMotionBlur *inMotionBlur)
{
	float alpha_amount;
	UUtUns32 integer_alpha_amount;
	UUtUns32 age;

	age = ONgGameState->gameTime - inMotionBlur->date_of_birth;

	UUmAssert(age < inMotionBlur->lifespan);

	alpha_amount = inMotionBlur->blur_base;
	alpha_amount *= (inMotionBlur->lifespan - age);
	alpha_amount /= inMotionBlur->lifespan;

	integer_alpha_amount = MUrUnsignedSmallFloat_To_Uns_Round(alpha_amount * M3cMaxAlpha);
	integer_alpha_amount = UUmPin(integer_alpha_amount, 0, M3cMaxAlpha);

	M3rGeom_State_Set(M3cGeomStateIntType_Alpha, integer_alpha_amount);
	M3rGeom_State_Commit();

	M3rMatrixStack_Push();
		M3rMatrixStack_Multiply(&inMotionBlur->matrix);

		inMotionBlur->geometry->baseMap = inMotionBlur->texture;
		M3rGeometry_Draw(inMotionBlur->geometry);
	M3rMatrixStack_Pop();

	return;
}

static void ONrGameState_MotionBlur_Display(void)
{
	UUtUns32 itr;
	UUtUns32 count = ONgGameState->numBlurs;

	if (0 == count)
	{
		return;
	}

	// step 1 remove dead
	itr = 0;

	while(itr < count)
	{
		ONtMotionBlur *blur = ONgGameState->blur + itr;
		UUtUns32 time_of_death;

		time_of_death = blur->date_of_birth + blur->lifespan;

		if (ONgGameState->gameTime >= time_of_death)
		{
			ONtMotionBlur *last_blur = ONgGameState->blur + count - 1;

			*blur = *last_blur;
			count--;

			continue;
		}

		itr++;
	}

	ONgGameState->numBlurs = count;

	// step 2 draw
	M3rGeom_State_Push();
		for(itr = 0; itr < count; itr++)
		{
			ONrGameState_MotionBlur_Display_One(ONgGameState->blur + itr);
		}
	M3rGeom_State_Pop();

	return;
}

void ONrGameState_MotionBlur_Add(ONtMotionBlur *inMotionBlur)
{
	if (ONcMaxBlurs == ONgGameState->numBlurs)
	{
		// try to scavange
		return;
	}

	ONgGameState->blur[ONgGameState->numBlurs] = *inMotionBlur;
	ONgGameState->numBlurs++;

	return;
}

void ONrGameState_LetterBox_Start(ONtLetterBox *ioLetterBox)
{
	ioLetterBox->last = ONgGameState->gameTime;
	ioLetterBox->active = UUcTrue;
	ioLetterBox->updating = UUcTrue;

	return;
}

void ONrGameState_LetterBox_Stop(ONtLetterBox *ioLetterBox)
{
	ioLetterBox->last = ONgGameState->gameTime;
	ioLetterBox->active = UUcFalse;
	ioLetterBox->updating = UUcTrue;

	COrSubtitle_Clear();

	return;
}

void ONrGameState_LetterBox_Update(ONtLetterBox *ioLetterBox, UUtUns32 inTicks)
{
	if (ioLetterBox->active) {
		ioLetterBox->position += ONcLetterBox_Increment * inTicks;

		if (ioLetterBox->position >= ONcLetterBox_Depth) {
			ioLetterBox->position = ONcLetterBox_Depth;
			ioLetterBox->updating = UUcFalse;
		}
	}
	else {
		ioLetterBox->position -= ONcLetterBox_Increment * inTicks;

		if (ioLetterBox->position <= 0) {
			ioLetterBox->position = 0;
			ioLetterBox->updating = UUcFalse;
		}
	}

	return;
}

void ONrGameState_LetterBox_Display(ONtLetterBox *ioLetterBox)
{
	UUtUns16 screen_width;
	UUtUns16 screen_height;

	if (ONrGameState_IsSkippingCutscene()) {
		goto exit;
	}

	if (ioLetterBox->updating)  {
		UUtUns32 ticks;

		ticks = ONgGameState->gameTime - ioLetterBox->last;
		ioLetterBox->last = ONgGameState->gameTime;

		ONrGameState_LetterBox_Update(ioLetterBox, ticks);
	}

	screen_width = M3rDraw_GetWidth();
	screen_height = M3rDraw_GetHeight();

	if ((ioLetterBox->position > 0) && (ONgLetterboxTexture != NULL)) {
		float relative_position = ioLetterBox->position * (screen_height / 480.f);
		M3tPointScreen points[2];
		M3tTextureCoord uv[4];

		M3rDraw_State_Push();

		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_Black);
		M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, M3cMaxAlpha);

		M3rDraw_State_SetPtr(
			M3cDrawStatePtrType_BaseTextureMap,
			ONgLetterboxTexture);

		M3rDraw_State_SetInt(
			M3cDrawStateIntType_Interpolation,
			M3cDrawState_Interpolation_None);

		points[0].x = 0.f;
		points[0].y = 0.f;
		points[0].z = 0.5f;
		points[0].invW = 2.f;

		points[1].x = screen_width;
		points[1].y = relative_position;
		points[1].z = 0.5f;
		points[1].invW = 2.f;

		M3rDraw_State_Commit();

		M3rDraw_Sprite(
			points,
			uv);

		points[0].y = (float) (screen_height - relative_position);
		points[1].y = (float) (screen_height);

		M3rDraw_Sprite(
			points,
			uv);

		M3rDraw_State_Pop();
	}

exit:
	return;
}

UUtBool ONrGameState_LetterBox_Active(ONtLetterBox *inLetterBox)
{
	return inLetterBox->active;
}

ONtLetterBox *ONrGameState_GetLetterBox(void)
{
	return &ONgGameState->local.letterbox;
}

void ONrGameState_BeginCutscene(UUtUns32 inCutsceneFlags)
{
	UUtBool sync_enabled = UUcTrue;
	ONtCharacter *player_character = ONgGameState->local.playerCharacter;
	ONtActiveCharacter *player_active_character = ONgGameState->local.playerActiveCharacter;

	ONrGameState_LetterBox_Start(&ONgGameState->local.letterbox);

	ONrCharacter_ExitFightMode(player_character);

	if (!(inCutsceneFlags & ONcCutsceneFlag_RetainAnimation)) {
		ONrCharacter_ForceStand(player_character);

		if (player_active_character != NULL) {
			player_active_character->stitch.velocity = MUgZeroVector;
		}
	}

	if (inCutsceneFlags & ONcCutsceneFlag_NoJump) {
		if (player_active_character != NULL) {
			if (player_active_character->inAirControl.numFramesInAir > 0) {
				player_active_character->inAirControl.velocity.x = 0.f;
				player_active_character->inAirControl.velocity.z = 0.f;
			}
		}
	}

	if (!(inCutsceneFlags & ONcCutsceneFlag_RetainWeapon)) {
		ONrCharacter_CinematicHolster(player_character);
	}

	if (!(inCutsceneFlags & ONcCutsceneFlag_RetainInvisibility)) {
		player_character->inventory.invisibilityRemaining = 0;
	}

	if (!(inCutsceneFlags & ONcCutsceneFlag_RetainAI)) {
		AI2rAllPassive(UUcTrue);
	}

	if (!(inCutsceneFlags & ONcCutsceneFlag_RetainJello)) {
		CArEnableJello(UUcFalse);
	}

	if (inCutsceneFlags & ONcCutsceneFlag_DontSync) {
		sync_enabled = UUcFalse;
	}

	if (!(inCutsceneFlags & ONcCutsceneFlag_DontKillParticles)) {
		P3rRemoveDangerousProjectiles();
	}

	OSrSetEnabled(UUcTrue);

	ONgGameState->inputEnable = UUcFalse;
	ONgGameState->local.in_cutscene = UUcTrue;
	ONgGameState->local.cutscene_skip_mode = ONcCutsceneSkip_idle;
	ONgGameState->local.time_cutscene_started_or_wait_for_key_returned_in_machine_time = UUrMachineTime_Sixtieths();
	ONgGameState->local.sync_enabled = sync_enabled;
	ONgGameState->local.sync_frames_behind = 0;

	return;
}

static void ONrGameState_CancelSkipping(void)
{
	if (ONgGameState->local.cutscene_skip_mode != ONcCutsceneSkip_idle) {
		if (ONcCutsceneSkip_starting == ONgGameState->local.cutscene_skip_mode) {
			ONgGameState->local.cutscene_skip_timer = ONcNumber_of_frames_for_cutscene_skip_timer - ONgGameState->local.cutscene_skip_timer;
		}
		else {
			ONgGameState->local.cutscene_skip_timer = ONcNumber_of_frames_for_cutscene_skip_timer;
		}

		ONgGameState->local.cutscene_skip_mode = ONcCutsceneSkip_fading_in_after_skipping;
	}

	return;
}

void ONrGameState_EndCutscene(void)
{
	ONgGameState->inputEnable = UUcTrue;

	CArEnableJello(UUcTrue);

	AI2rAllPassive(UUcFalse);

	OCrCinematic_DeleteAll();

	ONrGameState_LetterBox_Stop(&ONgGameState->local.letterbox);
	ONgGameState->local.in_cutscene = UUcFalse;

	ONrGameState_CancelSkipping();

	OSrSetEnabled(UUcTrue);

	if (NULL == ONgGameState->local.playerCharacter->inventory.weapons[0]) {
		ONrCharacter_CinematicEndHolster(ONgGameState->local.playerCharacter, UUcTrue);
	}

	ONgGameState->local.sync_enabled = UUcFalse;

	return;
}

#if 0
void ONrGameState_CutscenePoint(void)
{
	ONrGameState_CancelSkipping();
}
#endif

void ONrGameState_Cutscene_Sync_Mark(void)
{
	ONgGameState->local.sync_frames_behind = 0;

	return;
}

UUtBool ONrGameState_IsSkippingCutscene(void)
{
	UUtBool is_skipping_cutscene = ONcCutsceneSkip_skipping == ONgGameState->local.cutscene_skip_mode;

	return is_skipping_cutscene;
}

void ONrGameState_SkipCutscene(void)
{
	UUtUns32 current_time = UUrMachineTime_Sixtieths();
	UUtUns32 time_after_which_we_can_skip_cutscene = ONgGameState->local.time_cutscene_started_or_wait_for_key_returned_in_machine_time + 10;

	UUmAssert(ONgGameState->local.in_cutscene);

	if (ONgGameState->local.in_cutscene) {
		if (current_time >= time_after_which_we_can_skip_cutscene) {
			if (ONcCutsceneSkip_idle == ONgGameState->local.cutscene_skip_mode) {

				ONgGameState->local.cutscene_skip_timer = ONcNumber_of_frames_for_cutscene_skip_timer;
				ONgGameState->local.cutscene_skip_mode = ONcCutsceneSkip_starting;

				OSrSetEnabled(UUcFalse);
			}
		}
	}

	return;
}

// timer functions

UUtError ONrGameState_Timer_LevelBegin(void)
{
	UUtError error;
	TStFontFamily	*fontFamily;

	error = TMrInstance_GetDataPtr(TScTemplate_FontFamily, TScFontFamily_Default,	&fontFamily);
	UUmError_ReturnOnErrorMsg(error, "Could not get font family for the timer");

	error = TSrContext_New(fontFamily, TScFontSize_Default, TScStyle_Bold, TSc_HLeft,UUcFalse, &ONgTimerTextContext);
	UUmError_ReturnOnErrorMsg(error, "Could not create text context for the timer");

	TSrContext_SetShade(ONgTimerTextContext, IMcShade_White);

	return error;
}

void ONrGameState_Timer_LevelEnd(void)
{
	if (NULL != ONgTimerTextContext) {
		TSrContext_Delete(ONgTimerTextContext);
		ONgTimerTextContext = NULL;
	}

}

ONtTimerMode ONrGameState_Timer_GetMode(void)
{
	return ONgGameState->local.timer.timer_mode;
}

void ONrGameState_Timer_Start(char *inScriptName, UUtUns32 inTicks)
{
	ONtTimer *timer = &ONgGameState->local.timer;

	timer->timer_mode = ONcTimerMode_On;
	UUrString_Copy(timer->script, inScriptName, sizeof(timer->script));
	timer->num_ticks_drawn = 0;
	timer->ticks_remaining = inTicks;
	timer->draw_timer = UUcTrue;

	return;
}

void ONrGameState_Timer_Stop(void)
{
	ONtTimer *timer = &ONgGameState->local.timer;

	if (timer->timer_mode == ONcTimerMode_On) {
		// timer completed successfully
		ONrGameState_EventSound_Play(ONcEventSound_TimerSuccess, NULL);
	}

	timer->timer_mode = ONcTimerMode_Off;

	return;
}

void ONrGameState_Timer_Update(void)
{
	ONtTimer *timer = &ONgGameState->local.timer;
	UUtBool critical;

	if ((timer->timer_mode == ONcTimerMode_Off) || (timer->ticks_remaining > 10 * UUcFramesPerSecond)) {
		critical = UUcFalse;
		ONrGameState_ConditionSound_Stop(ONcConditionSound_TimerCritical);
	} else {
		critical = UUcTrue;
		ONrGameState_ConditionSound_Start(ONcConditionSound_TimerCritical, 1.0f);
	}

	switch(ONgGameState->local.timer.timer_mode)
	{
		case ONcTimerMode_On:
			if ((!critical) && ((timer->ticks_remaining % UUcFramesPerSecond) == 0)) {
				ONrGameState_EventSound_Play(ONcEventSound_TimerTick, NULL);
			}

			timer->ticks_remaining--;
			timer->num_ticks_drawn++;

			if (0 == timer->ticks_remaining) {
				timer->timer_mode = ONcTimerMode_Fail;
				timer->ticks_remaining = 5 * UUcFramesPerSecond;	// 5 seconds of 00:00 blinking
				SLrScript_ExecuteOnce(timer->script, 0, NULL, NULL, NULL);
			}
		break;

		case ONcTimerMode_Fail:
			timer->ticks_remaining--;
			timer->num_ticks_drawn++;

			if (0 == timer->ticks_remaining) {
				timer->timer_mode = ONcTimerMode_Off;
			}

		break;

		default:
		case ONcTimerMode_Off:
		break;
	}

	return;
}



void ONrGameState_Timer_Display(const M3tPointScreen *inCenter)
{
	ONtTimer *timer = &ONgGameState->local.timer;

	{
		if (ONcTimerMode_Off == timer->timer_mode) {
			goto exit;
		}
		else {
			UUtUns32 interval = 0;

			if (ONcTimerMode_On == timer->timer_mode) {
				if (timer->ticks_remaining < (60 * 5)) {
					interval = 4;
				}
				else if (timer->ticks_remaining < (60 * 10)) {
					interval = 8;
				}
				else if (timer->ticks_remaining < (60 * 20)) {
					interval = 15;
				}
			}
			else {
				interval = 15;
			}

			if (0 != interval) {
				UUtBool draw_this_time = timer->draw_timer;

				if (timer->num_ticks_drawn > interval) {
					timer->draw_timer = !timer->draw_timer;
					timer->num_ticks_drawn = 0;
				}

				if (!timer->draw_timer) {
					goto exit;
				}
			}
		}
	}

	{
		char display_string[30];

		switch(timer->timer_mode)
		{
			case ONcTimerMode_On:
				{
					UUtUns32 minutes, seconds, tenths;

					tenths = ONgGameState->local.timer.ticks_remaining / (UUcFramesPerSecond / 10);
					seconds = ONgGameState->local.timer.ticks_remaining / UUcFramesPerSecond;
					minutes = ONgGameState->local.timer.ticks_remaining / (UUcFramesPerSecond * 60);

					tenths %= 10;
					seconds %= 60;
					if ((minutes > 0) || (seconds >= 10)) {
						sprintf(display_string, "%01d:%02d", minutes, seconds);
					} else {
						sprintf(display_string, "%01d:%02d.%01d", minutes, seconds, tenths);
					}
				}
			break;

			case ONcTimerMode_Fail:
				{
					sprintf(display_string, "0:00.0");
				}
			break;

			default:
			case ONcTimerMode_Off:
				UUmAssert(!"invalid case in timer display");
			break;
		}


		{
			UUtRect rect;
			IMtPoint2D draw_location;
			const UUtUns16 screen_width = M3rDraw_GetWidth();
			const UUtUns16 screen_height = M3rDraw_GetHeight();
			UUtUns16 string_width;
			UUtUns16 string_height;

			TSrContext_GetStringRect(ONgTimerTextContext, display_string, &rect);
			string_width = rect.right - rect.left;
			string_height = TSrFont_GetAscendingHeight(TSrContext_GetFont(ONgTimerTextContext, TScStyle_Plain));

			draw_location.x = ((UUtUns16)inCenter->x) - (string_width / 2);
			draw_location.y = ((UUtUns16)inCenter->y) + (string_height / 2);

			TSrContext_DrawText(ONgTimerTextContext, display_string, M3cMaxAlpha, NULL, &draw_location);
		}
	}

exit:
	return;
}


static ONtContinue ONgContinue;
#define ONcAutosaveMessage		"autosave"
#define ONcAutosaveFadeTime		120

#if 0
static char *get_time_string(void)
{
	time_t current_time;
	char *result;

	time(&current_time);

	result = asctime(localtime(&current_time));

	return result;
}
#endif


void ONrGameState_MakeContinue(UUtUns32 inSavePoint, UUtBool inAutoSave)
{
	ONtCharacter *character = ONgGameState->local.playerCharacter;
	const char *message;

	if (inAutoSave) {
		ONrGameState_EventSound_Play(ONcEventSound_Autosave, NULL);
		message = SSrMessage_Find(ONcAutosaveMessage);
		if (message != NULL) {
			COrMessage_Print(message, NULL, ONcAutosaveFadeTime);
		}
	}

	if (0 == inSavePoint) {
		COrConsole_Printf("invalid save point");
	}

	if (ONgContinueSavePoint == ((UUtInt32) inSavePoint)) {
		// don't double save at a save point
	}

	ONrGameState_ClearContinue();

	ONgContinueSavePoint = inSavePoint;

	if (1 == ONgGameState->levelNumber) {
		if (1 == inSavePoint) {
			sprintf(ONgContinue.name, "Syndicate Warehouse");
		}
		else {
			sprintf(ONgContinue.name, "    Save Point %d", ONgContinueSavePoint - 1);
		}
	}
	else {
		sprintf(ONgContinue.name, "     Save Point %d", ONgContinueSavePoint);
	}

	ONgContinue.maxHitPoints = character->maxHitPoints;
	ONgContinue.hitPoints = character->hitPoints + character->inventory.hypoRemaining;
	ONgContinue.actual_position = character->actual_position;
	ONgContinue.facing = character->facing;

	ONgContinue.ammo = character->inventory.ammo;
	ONgContinue.cell = character->inventory.cell;
	ONgContinue.shieldRemaining = character->inventory.shieldRemaining;
	ONgContinue.invisibilityRemaining = character->inventory.invisibilityRemaining;
	ONgContinue.hypo = character->inventory.hypo;
	ONgContinue.keys = character->inventory.keys;
	ONgContinue.has_lsi = character->inventory.has_lsi;
	ONgContinue.continue_flags = ONcContinueFlag_Valid;

	{
		UUtUns32 itr;

		for(itr = 0; itr < WPcMaxSlots; itr++) {
			WPtWeapon *weapon = character->inventory.weapons[itr];

			if (NULL != weapon) {
				WPtWeaponClass *weapon_class = WPrGetClass(weapon);
				if (NULL != weapon_class) {
					const char *weapon_instance_name;

					weapon_instance_name = TMrInstance_GetInstanceName(weapon_class);

					strcpy(ONgContinue.weapon_save[itr].weapon, weapon_instance_name);
					ONgContinue.weapon_save[itr].weapon_ammo = WPrGetAmmo(weapon);
				}
			}
		}
	}

	ONrPersist_SetContinue(ONgGameState->levelNumber, inSavePoint, &ONgContinue);

	{
		ONtPlace place;

		place.level = ONgGameState->levelNumber;
		place.save_point = inSavePoint;

		ONrPersist_SetPlace(&place);
	}

	return;
}

void ONrGameState_UseContinue(void)
{
	ONtCharacter *character = ONgGameState->local.playerCharacter;

	if (0 == ONgContinueSavePoint) {
		COrConsole_Printf("failed to continue, invalid save point");
		goto exit;
	}

	if (ONgContinue.continue_flags & ONcContinueFlag_Ignore_Restore) {
		COrConsole_Printf("valid continue but ignoring it");
		goto exit;
	}

	if (character->maxHitPoints == ONgContinue.maxHitPoints) {
		character->hitPoints = ONgContinue.hitPoints;
	} else {
		// CB: we must keep the character's ratio of hit points to max hit points the same, as they
		// might be loading a game as a different character class, or on a different difficulty level
		character->hitPoints = (ONgContinue.hitPoints * character->maxHitPoints) / ONgContinue.maxHitPoints;
	}

	ONrCharacter_Teleport(character, &ONgContinue.actual_position, UUcFalse);
	character->facing = ONgContinue.facing;
	character->facingModifier = 0;
	character->desiredFacing = ONgContinue.facing;

	// inventory
	character->inventory.ammo = (UUtUns16) ONgContinue.ammo;
	character->inventory.cell = (UUtUns16) ONgContinue.cell;
	character->inventory.shieldRemaining = (UUtUns16) ONgContinue.shieldRemaining;
	character->inventory.invisibilityRemaining = (UUtUns16) ONgContinue.invisibilityRemaining;
	character->inventory.hypo = (UUtUns16) ONgContinue.hypo;
	character->inventory.keys = ONgContinue.keys;
	character->inventory.has_lsi = (UUtBool) ONgContinue.has_lsi;

	{
		UUtUns32 itr;

		character->inventory.weapons[0] = NULL;
		ONrCharacter_NotifyReleaseWeapon(character);

		for(itr =0; itr < WPcMaxSlots; itr++)
		{
			if (character->inventory.weapons[itr]!= NULL) {
				WPrDelete(character->inventory.weapons[itr]);
			}
		}

		UUrMemory_Clear(character->inventory.weapons, sizeof(character->inventory.weapons));

		if (ONgContinue.weapon_save[2].weapon[0] != '\0') {
			ONrCharacter_UseWeapon_NameAmmo(character, ONgContinue.weapon_save[2].weapon, ONgContinue.weapon_save[2].weapon_ammo);
			ONrCharacter_PickupWeapon(character, NULL, UUcFalse);
		}

		if (ONgContinue.weapon_save[1].weapon[0] != '\0') {
			ONrCharacter_UseWeapon_NameAmmo(character, ONgContinue.weapon_save[1].weapon, ONgContinue.weapon_save[1].weapon_ammo);
			ONrCharacter_PickupWeapon(character, NULL, UUcTrue);
		}

		if (ONgContinue.weapon_save[0].weapon[0] != '\0') {
			ONrCharacter_UseWeapon_NameAmmo(character, ONgContinue.weapon_save[0].weapon, ONgContinue.weapon_save[0].weapon_ammo);
		}

		ONrCharacter_ResetWeaponVarient(character);
	}

exit:
	ONrInGameUI_NotifyRestoreGame(ONgGameState->gameTime);

	return;
}

void ONrGameState_ClearContinue(void)
{
	UUrMemory_Clear(&ONgContinue, sizeof(ONgContinue));
	ONgContinueSavePoint = 0;

	return;
}


void ONrGameState_Continue_SetFromSave(UUtInt32 inSavePoint, const ONtContinue *inContinue)
{
	ONgContinue = *inContinue;
	ONgContinueSavePoint = inSavePoint;

	return;
}

// ------------------------------------------------------------------------------------
// -- event and condition sounds
void ONrGameState_EventSound_Play(UUtUns32 inEvent, M3tPoint3D *inPoint)
{
	UUmAssert((inEvent >= 0) && (inEvent < ONcEventSound_Max));

	if (ONgGameSettingsRuntime.event_sound[inEvent] != NULL) {
		OSrImpulse_Play(ONgGameSettingsRuntime.event_sound[inEvent], inPoint, NULL, NULL, NULL);
	}
//	COrConsole_Printf("event %s", ONcEventSoundName[inEvent]);
}

void ONrGameState_ConditionSound_Start(UUtUns32 inCondition, float inVolume)
{
	UUtUns32 mask = (1 << inCondition);

	UUmAssert((inCondition >= 0) && (inCondition < ONcEventSound_Max));

	if ((ONgGameState->condition_active & mask) == 0) {
		// start the sound!
		ONgGameState->condition_active |= mask;

		if (ONgGameSettingsRuntime.condition_sound[inCondition] != NULL) {
			ONgGameState->condition_playid[inCondition] =
						SSrAmbient_Start_Simple(ONgGameSettingsRuntime.condition_sound[inCondition], &inVolume);
		}
//		COrConsole_Printf("condition %s start %f", ONcConditionSoundName[inCondition], inVolume);

	} else if (ONgGameState->condition_playid[inCondition] != SScInvalidID) {
		// the sound is already playing, just change its volume
		SSrAmbient_SetVolume(ONgGameState->condition_playid[inCondition], inVolume, 1.0f);
//		COrConsole_Printf("condition %s setvolume %f", ONcConditionSoundName[inCondition], inVolume);
	}
}

void ONrGameState_ConditionSound_Stop(UUtUns32 inCondition)
{
	UUtUns32 mask = (1 << inCondition);

	UUmAssert((inCondition >= 0) && (inCondition < ONcEventSound_Max));

	if (ONgGameState->condition_active & mask) {
		ONgGameState->condition_active &= ~mask;

		if (ONgGameState->condition_playid[inCondition] != SScInvalidID) {
			SSrAmbient_Stop(ONgGameState->condition_playid[inCondition]);
			ONgGameState->condition_playid[inCondition] = SScInvalidID;
		}
//		COrConsole_Printf("condition %s stop", ONcConditionSoundName[inCondition]);
	}
}

UUtBool ONrGameState_ConditionSound_Active(UUtUns32 inCondition)
{
	UUmAssert((inCondition >= 0) && (inCondition < ONcEventSound_Max));

	return ((ONgGameState->condition_active & (1 << inCondition)) > 0);
}

// taunt enable logic
void ONrGameState_TauntEnable(UUtUns32 inFrames)
{
	UUtUns32 new_time;

	new_time = ONrGameState_GetGameTime() + inFrames;

	ONgGameState->local.tauntEnableTimer = UUmMax(ONgGameState->local.tauntEnableTimer, new_time);
}

UUtBool ONrGameState_IsTauntEnabled(void)
{
	return (ONgGameState->local.tauntEnableTimer >= ONrGameState_GetGameTime());
}

UUtBool ONrGameState_IsInputEnabled(void)
{
	return ONgGameState->inputEnable;
}

static void ONiGameState_FindAutoPromptMessage(const char *inEvent, const char **ioMessage)
{
	ONtAutoPrompt *prompt;
	UUtUns32 itr;

	for (itr = 0, prompt = ONgGameSettings->autoprompts; itr < ONgGameSettings->num_autoprompts; itr++, prompt++) {
		if ((ONgGameState->levelNumber < prompt->start_level) || (ONgGameState->levelNumber > prompt->end_level))
			continue;

		if (UUrString_Compare_NoCase(inEvent, prompt->event_name) == 0) {
			*ioMessage = prompt->message_name;
			return;
		}
	}
}

static void ONrGameState_UpdateAutoPrompt(void)
{
	ONtCharacter *player;
	WPtWeapon *weapon;
	WPtPowerup *powerup;
	OBJtObject *closest_door, *console_object;
	OBJtOSD_Console *console_osd;
	ONtActionMarker *action_marker;
	M3tPoint3D pos;
	UUtBool inventory_full, huge_weapon, use_door, use_console;
	const char *prompt_type, *message, *message_text;

	player = ONrGameState_GetPlayerCharacter();
	if (player == NULL) {
		return;
	}

	ONiGameState_FindPickupItems(player, &weapon, &powerup, &inventory_full);
	if (inventory_full) {
		powerup = NULL;
	}

	// determine what message to display
	message = NULL;

	if ((player->flags & ONcCharacterFlag_Dead) == 0) {
		// prompt to pick up a weapon
		if ((weapon != NULL) && WPrHasAmmo(weapon)) {
			ONiGameState_FindAutoPromptMessage("weapon", &message);
		}

		// powerups have a higher priority than weapons
		if (powerup != NULL) {
			ONiGameState_FindAutoPromptMessage(WPgPowerupName[powerup->type], &message);
		}

		// NCIs have a higher priority than pickups
		if (AI2rTryNeutralInteraction(player, UUcFalse)) {
			ONiGameState_FindAutoPromptMessage("nci", &message);
		}

		// doors have a higher priority than NCIs
		use_door = OBJrDoor_TryDoorAction(player, &closest_door);
		if ((closest_door != NULL) && use_door) {
			ONiGameState_FindAutoPromptMessage("usedoor", &message);
		}

		// consoles have a higher priority than doors
		huge_weapon = (player->inventory.weapons[0] != NULL) && (WPrGetClass(player->inventory.weapons[0])->flags & WPcWeaponClassFlag_Unstorable);
		if (!huge_weapon) {
			ONrCharacter_GetPelvisPosition(player, &pos);
			action_marker = ONrLevel_ActionMarker_FindNearest(&pos);

			if (action_marker != NULL) {
				use_console = ONrGameState_TryActionMarker(player, action_marker, UUcFalse);
				if (use_console) {
					console_object = (OBJtObject*) action_marker->object;
					console_osd = (OBJtOSD_Console *) console_object->object_data;

					if ((console_osd->flags & OBJcConsoleFlag_Active) &&
						((console_osd->flags & OBJcConsoleFlag_Triggered) == 0)) {
						if ((console_osd->console_class != NULL) && (console_osd->console_class->flags & OBJcConsoleClassFlag_AlarmPrompt)) {
							prompt_type = "usealarm";
						} else {
							prompt_type = "useconsole";
						}

						ONiGameState_FindAutoPromptMessage(prompt_type, &message);
					}
				}
			}
		}

		// hypo usage has a higher priority than using consoles
		if (player->inventory.hypo > 0) {
			if (player->hitPoints <= (player->maxHitPoints * ONcIGUI_HealthWarnLevel) / 100) {
				ONiGameState_FindAutoPromptMessage("usehypo", &message);
			}
		}
	}

	if (ONgGameState->local.currentPromptMessage == message) {
		// just leave our current message up
		return;
	} else {
		// we are changing messages
		COrMessage_Remove("autoprompt");
		ONgGameState->local.currentPromptMessage = message;
		if (message != NULL) {
			// print our new message
			message_text = SSrMessage_Find(message);
			if (message_text == NULL) {
				COrConsole_Printf("### unable to find autoprompt message '%s'", message);
			} else {
				COrMessage_Print(message_text, "autoprompt", 0);
			}
		}
	}
}

/*
 * shadow storage management
 */

UUtUns16 ONrGameState_NewShadow(void)
{
	ONtCharacterShadowStorage *shadow;
	UUtUns32 itr;

	for (itr = 0, shadow = ONgGameState->shadowStorage; itr < ONcMaxCharacterShadows; itr++, shadow++) {
		if (shadow->character_index == ONcCharacterIndex_None)
			break;
	}

	if (itr >= ONcMaxCharacterShadows) {
		// no shadow available
		return (UUtUns16) -1;
	} else {
		ONgGameState->numShadows = UUmMax(ONgGameState->numShadows, itr + 1);
		return (UUtUns16) itr;
	}
}

void ONrGameState_DeleteShadow(UUtUns16 inShadowIndex)
{
	UUmAssert((inShadowIndex >= 0) && (inShadowIndex < ONgGameState->numShadows));
	ONgGameState->shadowStorage[inShadowIndex].character_index	= ONcCharacterIndex_None;
	ONgGameState->shadowStorage[inShadowIndex].active_index		= ONcCharacterIndex_None;

	if (ONgGameState->numShadows == (UUtUns32) (inShadowIndex + 1)) {
		ONgGameState->numShadows--;
	}
}
