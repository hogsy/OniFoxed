#pragma once

/*
	FILE:	Oni_GameStatePrivate.h

	AUTHOR:	Michael Evans

	CREATED: Jan, 1998

	PURPOSE:

	Copyright 1997, 1998

*/

#ifndef ONI_GAMESTATE_PRIVATE_H
#define ONI_GAMESTATE_PRIVATE_H

#include "BFW.h"
#include "BFW_Console.h"
#include "BFW_Object.h"

#include "Oni_Character.h"
#include "Oni_Camera.h"
#include "Oni_Aiming.h"
#include "Oni_Path.h"
#include "Oni_GameState.h"
#include "Oni_Level.h"
#include "EnvFileFormat.h"

#define ONcMaxObjects		40
#define ONcMaxTriggers		32

typedef struct ONtServerState
{
	UUtUns32 machineTimeLast;		// machine time last time we did an update
} ONtServerState;

struct ONtLetterBox
{
	UUtUns32	last;
	float		position;
	UUtBool		active;
	UUtBool		updating;
};

typedef struct ONtFadeInfo
{
	float start_a;
	float start_r;
	float start_g;
	float start_b;

	float end_a;
	float end_r;
	float end_g;
	float end_b;

	UUtUns32 startTime;
	UUtUns32 endTime;

	UUtBool active;
} ONtFadeInfo;


// camera to be used for visibility determination
extern M3tGeomCamera *ONgVisibilityCamera;

// camera currently being controlled
extern M3tGeomCamera *ONgActiveCamera;


#define ONcNumber_of_frames_for_cutscene_skip_timer 20
typedef enum ONtCutsceneSkipMode
{
	ONcCutsceneSkip_idle,
	ONcCutsceneSkip_starting,
	ONcCutsceneSkip_after_fill_black,
	ONcCutsceneSkip_skipping,
	ONcCutsceneSkip_fading_in_after_skipping
} ONtCutsceneSkipMode;

typedef struct ONtTimer
{
	ONtTimerMode timer_mode;
	char script[SLcScript_MaxNameLength];

	UUtUns32 ticks_remaining;

	UUtBool draw_timer;
	UUtUns32 num_ticks_drawn;
} ONtTimer;

typedef struct ONtCharacterShadowStorage {
	UUtUns16		character_index;
	UUtUns16		active_index;
	char			decalbuffer[2048];

} ONtCharacterShadowStorage;

typedef struct ONtLocalGameState
{
	ONtTimer		timer;
	ONtLetterBox	letterbox;
	UUtBool			in_cutscene;				// we are in a cutscene

	ONtCutsceneSkipMode cutscene_skip_mode;

	// sync_ controls stuff when we are trying to sync to real world things like sound
	UUtUns32		sync_frames_behind;
	UUtBool			sync_enabled;

	UUtUns32		cutscene_skip_timer;
	UUtUns32		time_cutscene_started_or_wait_for_key_returned_in_machine_time;

	ONtFadeInfo		fadeInfo;

	CAtCamera		*camera;

	PHtGraph		pathGraph;			// Pathfinding digraph for environment

	ONtCharacter	*playerCharacter;	// active character on local machine
	ONtActiveCharacter	*playerActiveCharacter;	// active character on local machine
	ONtInputState	localInput;

	WPtWeaponClass		*prevWeapon1,*prevWeapon2;

	UUtBool				recordScreen;		// screen recording mode

	UUtUns32			cameraMark;			// frame of a ghost animation camera started on

	UUtBool				cameraRecord;
	UUtBool				cameraPlay;
	UUtBool				cameraActive;

	AXtHeader			*cameraRecording;
	UUtUns32			cameraFrame;
	// input queue, that gets parsed into commands like double tab and so on

	UUtBool				slowMotionEnable;
	UUtInt32			slowMotionTimer;
	UUtInt32			savePoint;

	UUtUns32			tauntEnableTimer;

	const char			*currentPromptMessage;

	char				pending_splash_screen[32];
	UUtBool				pending_pause_screen;
} ONtLocalGameState;

typedef enum
{
	ONcNormal,
	ONcWin,
	ONcLose
} ONtVictory;

struct ONtGameState
{
	ONtLocalGameState	local;			// local data (different on different machines)
	ONtServerState		server;

	UUtUns32			gameTime;		// time in heartbeats of the game (on our local machine)
	UUtUns32			serverTime;		// server time (we have gotten all commands up to this time)

	M3tGeometry*		geometryOpacity;

	ONtLevel*			level;

	UUtBool				displayOpacity;

	// motion blurs
	ONtMotionBlur		blur[ONcMaxBlurs];
	UUtUns32			numBlurs;

	// character salt indices
	UUtUns16			nextCharacterSalt;
	UUtUns16			nextActiveCharacterSalt;

	// all characters
	ONtCharacter		characters[ONcMaxCharacters];
	UUtUns16			numCharacters;

	// active characters
	ONtActiveCharacter	activeCharacterStorage[ONcMaxActiveCharacters];
	UUtUns16			usedActiveCharacters;

	// character shadow storage
	ONtCharacterShadowStorage	shadowStorage[ONcMaxCharacterShadows];
	UUtUns32			numShadows;

	// characters who are active
	ONtCharacter		*activeCharacters[ONcMaxCharacters];
	UUtUns32			numActiveCharacters;
	UUtUns32			activeCharacterLockDepth;

	// characters who are alive
	ONtCharacter		*livingCharacters[ONcMaxCharacters];
	UUtUns32			numLivingCharacters;
	UUtUns32			livingCharacterLockDepth;

	// characters who are in use and not death stage four
	ONtCharacter		*presentCharacters[ONcMaxCharacters];
	UUtUns32			numPresentCharacters;
	UUtUns32			presentCharacterLockDepth;

	// physics
	OBtObjectList		*objects;

	// doors
	OBtDoorArray		doors;

	// FX
	//FXtEffectList		effects;
	//FXtSky				sky;

	ONtSky				sky;

	// Triggers
	AItScriptTriggerList triggers;

	// misc
	UUtBool				paused;
	UUtBool				inputEnable;
	UUtBool				inside_console_action;

	UUtUns64			key_mask;

	// Film recording
	struct ONtFilmData			*filmData;

	// have we won or lost this level?
	ONtVictory			victory;

	// condition sounds currently playing
	UUtUns32			condition_active;
	SStPlayID			condition_playid[ONcConditionSound_Max];

	UUtUns16			levelNumber;
};





#endif
