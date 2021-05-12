#pragma once

/*
	FILE:	Oni_GameState.h
	
	AUTHOR:	Brent Pease, Michael Evans, Quinn Dunki, Kevin Armstrong
	
	CREATED: May 31, 1997
	
	PURPOSE:
	
	Copyright 1997 - 2000

*/
#ifndef ONI_GAMESTATE_H
#define ONI_GAMESTATE_H

#include "BFW.h"
#include "BFW_Akira.h"
#include "BFW_LocalInput.h"
//#include "BFW_DialogManager.h"

#include "Oni_GameSettings.h"

typedef enum ONtDifficultyLevel
{
	ONcDifficultyLevel_Easy		= 0,// easy
	ONcDifficultyLevel_Normal	= 1,// medium
	ONcDifficultyLevel_Hard		= 2,// hard
	ONcDifficultyLevel_Count	= 3,
	
	ONcDifficultyLevel_Min		= ONcDifficultyLevel_Easy,
	ONcDifficultyLevel_Default	= ONcDifficultyLevel_Normal,
	ONcDifficultyLevel_Max		= ONcDifficultyLevel_Hard
	
} ONtDifficultyLevel;

typedef struct ONtGameState ONtGameState;
typedef struct ONtCharacter ONtCharacter;
typedef struct ONtActiveCharacter ONtActiveCharacter;
typedef struct ONtInputState ONtInputState;
typedef struct AItCharacterSetup AItCharacterSetup;
typedef struct ONtFlag ONtFlag;
typedef struct PHtGraph PHtGraph;
typedef struct ONtLetterBox ONtLetterBox;
typedef struct ONtCharacterClass ONtCharacterClass;
typedef struct ONtContinue ONtContinue;

#define ONcNumCharacterParts ((UUtUns16) 19)

typedef enum ONtTimerMode
{
	ONcTimerMode_Off,
	ONcTimerMode_On,
	ONcTimerMode_Fail
} ONtTimerMode;

typedef tm_struct ONtCorpse_Data
{
	ONtCharacterClass		*characterClass;
	M3tMatrix4x3			matricies[19];	// must match ONcNumCharacterParts
	M3tBoundingBox_MinMax	corpse_bbox;
} ONtCorpse_Data;

typedef tm_struct ONtCorpse
{
	char					corpse_name[32];
	UUtUns32				node_list[32];
	ONtCorpse_Data			corpse_data;
} ONtCorpse;

#define ONcTemplate_CorpseArray UUm4CharToUns32('C', 'R', 'S', 'A')
typedef tm_template('C','R','S','A', "Corpse Array")
ONtCorpseArray
{
	tm_pad					pad0[12];

	UUtUns32				static_corpses;
	UUtUns32				next;

	tm_varindex UUtUns32	max_corpses;
	tm_vararray ONtCorpse	corpses[1];
} ONtCorpseArray;

struct ONtInputState				// current input state
{
	float turnUD;
	float turnLR;

	LItButtonBits buttonLast;		// last state
	LItButtonBits buttonIsDown;		// current state
	LItButtonBits buttonIsUp;		// current state
	LItButtonBits buttonWentDown;	// went down this round
	LItButtonBits buttonWentUp;		// went down this round
};

#define ONcShadowDecalStaticBufferSize		2048

#define ONcMaxCharacters		128
#define ONcMaxActiveCharacters	64
#define ONcMaxBlurs				64
#define ONcMaxCharacterShadows	32

// constants for using action markers (e.g. consoles)
#define ONcActionMarker_MaxDistance		10.0f
#define ONcActionMarker_MinDistance		-2.0f
#define ONcActionMarker_MaxSideways		8.0f
#define ONcActionMarker_MaxHeight		8.0f
#define ONcActionMarker_LookAngle		(70.0f * M3cDegToRad)
#define ONcActionMarker_ObstructionDist	3.0f

extern float	ONgGameState_InputAccel;
extern UUtBool	ONgDisplayUI;
extern UUtBool	ONgDeveloperAccess;
extern UUtBool	ONgPlayerFistsOfFury;
extern UUtBool	ONgPlayerKnockdownResistant;
extern UUtBool	ONgPlayerUnstoppable;
extern UUtBool	ONgPlayerInvincible;
extern UUtBool	ONgPlayerOmnipotent;
extern UUtBool	ONgShow_Environment;
extern UUtBool	ONgShow_Characters;
extern UUtBool	ONgShow_Objects;
extern UUtBool	ONgDebugConsoleAction;
extern ONtGameState *ONgGameState;

UUtError
ONrGameState_Initialize(
	void);
	
void
ONrGameState_Terminate(
	void);

void ONrGameState_FadeOut(UUtUns32 inNumFrames, float inR, float inG, float inB);
void ONrGameState_FadeIn(UUtUns32 inNumFrames);

UUtError ONrGameState_LevelBegin(UUtUns16 inLevelNumber);
void ONrGameState_LevelEnd(void);

#define ONcIntroSplashScreen "intro_splash_screen"
#define ONcFailSplashScreen "fail_splash_screen"
#define ONcWinSplashScreen "win_splash_screen"

void
ONrGameState_SplashScreen(
	char *inInstanceName,
	const char *inMusicName,
	UUtBool inStopPlayingSounds);

UUtError ONrGameState_LevelBegin_Objects(
	void);
void ONrGameState_LevelEnd_Objects(
	void);
	
void ONrGameState_Delete(
	ONtGameState*	inGameState);

// called once per 1/60th of a second
void ONrGameState_ProcessHeartbeat(
	LItAction*		inAction);

UUtError ONrGameState_ProcessActions(
	LItAction*		inActionBuffer);

void ONrGameState_UpdateServerTime(
	ONtGameState*	ioGameState);

UUtError ONrGameState_Update(
	UUtUns16 numActionsInBuffer,
	LItAction *actionBuffer,
	UUtUns32 *outTicksUpdated);

UUtError ONrGameState_ConstantColorLighting(float r, float g, float b);
UUtError ONrGameState_SetupDefaultLighting(void);

void ONrGameState_Display(void);

#define ONcNoTurningLimitation M3c2Pi

void ONrCharacter_HandleHeartbeatInput(ONtCharacter *ioCharacter, ONtActiveCharacter *ioActiveCharacter);

void ONrGameState_ResetObjects(void);
void ONrGameState_ResetCharacters(void);

void ONrGameState_HandleUtilityInput(const ONtInputState*	inInput);

void ONrInput_Update_Keys(ONtInputState* ioInput, LItButtonBits inNewDown);

UUtBool ONrGameState_DropFlag(const M3tPoint3D *inLocation, const float inDesiredFacing,
								UUtBool inPrintMsg, UUtUns16 *outID);
void ONrDesireFacingModifier(ONtCharacter *inCharacter, float inDesiredFacingModifier);

// ----------------------------------------------------------------------
// Accessors to the fields of ONgGameState
// ----------------------------------------------------------------------
ONtCharacter **ONrGameState_ActiveCharacterList_Get(void);
UUtUns32 ONrGameState_ActiveCharacterList_Count(void);
void ONrGameState_ActiveCharacterList_Add(ONtCharacter *inCharacter);
void ONrGameState_ActiveCharacterList_Remove(ONtCharacter *inCharacter);
void ONrGameState_ActiveCharacterList_Lock();
void ONrGameState_ActiveCharacterList_Unlock();

ONtCharacter **ONrGameState_LivingCharacterList_Get(void);
UUtUns32 ONrGameState_LivingCharacterList_Count(void);
void ONrGameState_LivingCharacterList_Add(ONtCharacter *inCharacter);
void ONrGameState_LivingCharacterList_Remove(ONtCharacter *inCharacter);
void ONrGameState_LivingCharacterList_Lock();
void ONrGameState_LivingCharacterList_Unlock();

ONtCharacter **ONrGameState_PresentCharacterList_Get(void);
UUtUns32 ONrGameState_PresentCharacterList_Count(void);
void ONrGameState_PresentCharacterList_Add(ONtCharacter *inCharacter);
void ONrGameState_PresentCharacterList_Remove(ONtCharacter *inCharacter);
void ONrGameState_PresentCharacterList_Lock();
void ONrGameState_PresentCharacterList_Unlock();

ONtCharacter*
ONrGameState_GetCharacterList(
	void);

AKtEnvironment*
ONrGameState_GetEnvironment(
	void);

UUtUns32
ONrGameState_GetGameTime(
	void);

PHtGraph*
ONrGameState_GetGraph(
	void);
	
ONtCharacter*
ONrGameState_GetPlayerCharacter(
	void);
	
UUtUns16
ONrGameState_GetPlayerNum(
	void);
	
UUtUns16
ONrGameState_GetNumCharacters(
	void);

struct OBtObjectList*
ONrGameState_GetObjectList(
	void);
	
ONtLetterBox *ONrGameState_GetLetterBox(void);

void
ONrGameState_SetPlayerNum(
	UUtUns16			inPlayerNum);

UUtBool
ONrGameState_IsPaused(
	void);
	
UUtBool
ONrGameState_IsSingleStep(
	void);

void
ONrGameState_Pause(
	UUtBool				inPause);

void
ONrGameState_Performance_UpdateOverall(
	const char *l1,
	const char *l2,
	const char *l3);


typedef struct ONtMotionBlur
{
	M3tGeometry		*geometry;
	M3tTextureMap	*texture;
	M3tMatrix4x3	matrix;

	UUtUns32		date_of_birth;
	UUtUns32		lifespan;

	float			blur_base;
} ONtMotionBlur;

void ONrGameState_MotionBlur_Add(ONtMotionBlur *inMotionBlur);

UUtError ONrGameState_PrepareGeometryEngine(void);
void ONrGameState_TearDownGeometryEngine(void);

void ONrDrawNumberToScreen(UUtInt32 inNumber, UUtUns32 inForeground, UUtUns32 inBackground, M3tPoint3D *inLocation);

// cutscene functions

typedef enum ONtCutsceneFlags {
	ONcCutsceneFlag_RetainJello			= 0x0001,
	ONcCutsceneFlag_RetainWeapon		= 0x0002,
	ONcCutsceneFlag_RetainInvisibility	= 0x0004,
	ONcCutsceneFlag_RetainAI			= 0x0008,
	ONcCutsceneFlag_DontSync			= 0x0010,
	ONcCutsceneFlag_DontKillParticles	= 0x0020,
	ONcCutsceneFlag_RetainAnimation		= 0x0040,
	ONcCutsceneFlag_NoJump				= 0x0080
} ONtCutsceneFlags;

void ONrGameState_BeginCutscene(UUtUns32 inCutsceneFlags);
void ONrGameState_EndCutscene(void);
void ONrGameState_Cutscene_Sync_Mark(void);

void ONrGameState_SkipCutscene(void);
UUtBool ONrGameState_IsSkippingCutscene(void);

// letterbox functions
void ONrGameState_LetterBox_Start(ONtLetterBox *inLetterBox);
void ONrGameState_LetterBox_Stop(ONtLetterBox *inLetterBox);
void ONrGameState_LetterBox_Update(ONtLetterBox *ioLetterBox, UUtUns32 inTicks);
void ONrGameState_LetterBox_Display(ONtLetterBox *inLetterBox);
UUtBool ONrGameState_LetterBox_Active(ONtLetterBox *inLetterBox);

// timer functions
ONtTimerMode ONrGameState_Timer_GetMode(void);
void ONrGameState_Timer_Start(char *inScriptName, UUtUns32 inTicks);
void ONrGameState_Timer_Stop(void);
void ONrGameState_Timer_Update(void);
void ONrGameState_Timer_Display(const M3tPointScreen *inCenter);
UUtError ONrGameState_Timer_LevelBegin(void);
void ONrGameState_Timer_LevelEnd(void);

void ONrGameState_Continue_SetFromSave(UUtInt32 inSavePoint, const ONtContinue *inContinue);
void ONrGameState_MakeContinue(UUtUns32 inSavePoint, UUtBool inAutoSave);
void ONrGameState_UseContinue(void);
void ONrGameState_ClearContinue(void);

// check to see if a character can perform an action
struct ONtActionMarker;
UUtBool ONrGameState_TryActionMarker(ONtCharacter *inCharacter, struct ONtActionMarker *inActionMarker, UUtBool inGenerous);

// event and condition sounds
void ONrGameState_EventSound_Play(UUtUns32 inEvent, M3tPoint3D *inPoint);		// inPoint may be NULL for sounds with no position
void ONrGameState_ConditionSound_Start(UUtUns32 inCondition, float inVolume);
void ONrGameState_ConditionSound_Stop(UUtUns32 inCondition);
UUtBool ONrGameState_ConditionSound_Active(UUtUns32 inCondition);

// taunt enable logic
void ONrGameState_TauntEnable(UUtUns32 inFrames);
UUtBool ONrGameState_IsTauntEnabled(void);
UUtBool ONrGameState_IsInputEnabled(void);
void ONrGameState_HandleCheats(const LItInputEvent *inEvent);

// shadow storage management
UUtUns16 ONrGameState_NewShadow(void);
void ONrGameState_DeleteShadow(UUtUns16 inShadowIndex);

#endif /* ONI_GAMESTATE_H */
