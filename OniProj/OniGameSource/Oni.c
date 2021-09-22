 /*
	FILE:	Oni.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: April 2, 1997
	
	PURPOSE: main .c file for Oni
	
	Copyright 1997

*/
#include <stdio.h>
#include <stdlib.h>

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Totoro.h"
#include "BFW_LocalInput.h"
#include "BFW_TemplateManager.h"
#include "BFW_FileManager.h"
#include "BFW_Akira.h"
#include "BFW_TextSystem.h"
#include "BFW_SoundSystem2.h"
#include "BFW_Console.h"
#include "BFW_AppUtilities.h"
#include "BFW_CommandLine.h"
#include "BFW_Particle3.h"
#include "BFW_Platform.h"
#include "BFW_ScriptLang.h"
#include "BFW_Timer.h"
#include "BFW_BinaryData.h"
#include "BFW_Bink.h"
#include "BFW_LI_Private.h"

#include "Oni_Character.h"
#include "Oni.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Platform.h"
#include "Oni_Performance.h"
#include "Oni_Templates.h"
#include "Oni_Aiming.h"
#include "Oni_AI.h"
#include "Oni_Motoko.h"
#include "Oni_Film.h"
#include "Oni_Windows.h"
#include "Oni_Script.h"
#include "Oni_Object.h"
#include "Oni_BinaryData.h"
#include "Oni_Particle3.h"
#include "Oni_Cinematics.h"
#include "Oni_Sound2.h"
#include "Oni_ImpactEffect.h"
#include "Oni_TextureMaterials.h"
#include "Oni_Persistance.h"
#include "Oni_Weapon.h"
#include "Oni_InGameUI.h"

#include "Oni_Bink.h"
#include "gl_engine.h"

#if DEBUGGING
#define BRENTS_CHEESY_GAME_PERF	1
#endif

//UUtUns8	ONgMungedPassword[5] = {0x6B, 0x8B, 0x16, 0x72, 0xE1};

UUtUns32	ONgNumFrames = 0;

//UUtBool			ONgTerminateGame;
//M3tGeomContext	*ONgGeomContext = NULL;

ONtCommandLine		ONgCommandLine;

static UUtError
OniParseCommandLine(
	int argc,
	char **argv)
{
	int itr;

	ONgCommandLine.allowPrivateData = TMcPrivateData_Yes;
	ONgCommandLine.readConfigFile = COcConfigFile_Read;
	ONgCommandLine.logCombos = UUcFalse;
	ONgCommandLine.filmPlayback = UUcFalse;
	ONgCommandLine.useOpenGL = UUcFalse;
	ONgCommandLine.useGlide = UUcFalse;
	ONgCommandLine.useSound = UUcTrue;
	
	for(itr = 1; itr < argc; itr++)
	{
		char *current_parameter = argv[itr];

		if (0 == strcmp(current_parameter, "-ignore_config"))
		{
			ONgCommandLine.readConfigFile = COcConfigFile_Ignore;
		}
		else if (0 == strcmp(current_parameter, "-nosound"))
		{
			ONgCommandLine.useSound = UUcFalse;
		}
		else if (0 == strcmp(current_parameter, "-ehalt"))
		{
			UUgError_HaltOnError = UUcTrue;
		}
		else if (0 == strcmp(current_parameter, "-combos"))
		{
			ONgCommandLine.logCombos = UUcTrue;
		}
		else if (0 == strcmp(current_parameter, "-debug"))
		{
			AKgDebug_DebugMaps = UUcTrue;
		}
		else if (0 == strcmp(current_parameter, "-ignore_private_data"))
		{
			ONgCommandLine.allowPrivateData = TMcPrivateData_No;
		}
/*		else if (strcmp(current_parameter, "-noDialogs") == 0)
		{
			ONgDisplayDialogs = UUcFalse;
		}*/
		else if (strcmp(current_parameter, "-opengl") == 0)
		{
			ONgCommandLine.useOpenGL = UUcTrue;
		}
		else if (strcmp(current_parameter, "-glide") == 0)
		{
			ONgCommandLine.useGlide = UUcTrue;
		}
		else if (strcmp(current_parameter, "-noswitch") == 0)
		{
			M3gResolutionSwitch = UUcFalse;
		}
		else if (strcmp(current_parameter, "-debugfiles") == 0)
		{
			BFgDebugFileEnable = UUcTrue;
		}
		else if (strcmp(current_parameter, "-findsounds") == 0)
		{
			SSgSearchOnDisk = UUcTrue;
		}
		else if (strcmp(current_parameter, "-findsoundbinaries") == 0)
		{
			SSgSearchBinariesOnDisk = UUcTrue;
		}
#if defined(DEBUGGING) && DEBUGGING
		else if (strcmp(current_parameter, "-noverify") == 0)
		{
			UUrMemory_SetVerifyForce(UUcFalse);
		}
		else if (strcmp(current_parameter, "-forceverify") == 0)
		{
			UUrMemory_SetVerifyForce(UUcTrue);
		}
#endif
	}

	return UUcError_None;
}

static UUtError
ONiInitializeAll(
	ONtPlatformData	*outPlatformData)
{
	UUtError		error;

	UUrStartupMessage("begin initializing oni");
	
	ONgTerminateGame = UUcFalse;
	
	/*
	 * Initialize the template management system
	 */

	UUrStartupMessage("looking for the game data folder");

	error = BFrFileRef_Search(ONcGameDataFolder1, &ONgGameDataFolder);

	if(error != UUcError_None) {
		UUrStartupMessage("Unable to find game data folder at %s", ONcGameDataFolder1);
	
		error =	BFrFileRef_Search(ONcGameDataFolder2, &ONgGameDataFolder);
	}
	
	if (UUcError_None != error) {
		UUrStartupMessage("Unable to find game data folder at %s", ONcGameDataFolder2);
		error = ONcError_NoDataFolder;
	}

	UUmError_ReturnOnErrorMsg(error, "Could not find game data folder");

	UUrStartupMessage("initializing the template manager");
	error = TMrInitialize(UUcTrue, &ONgGameDataFolder);
	UUmError_ReturnOnError(error);
		
	UUrStartupMessage("calling TMrRegisterTemplates");
	TMrRegisterTemplates();
	
	UUrStartupMessage("calling ONrRegisterTemplates");
	ONrRegisterTemplates();

	UUrStartupMessage("initializing oni platform specific code");
	error = ONrPlatform_Initialize(outPlatformData);
	UUmError_ReturnOnError(error);

	UUrStartupMessage("initializing sound system 2, basic level");
	error = SS2rInitializeBasic(outPlatformData->gameWindow, ONgCommandLine.useSound);
	UUmError_ReturnOnError(error);

	UUrStartupMessage("initializing oni persistance");
	ONrPersistance_Initialize();
		
	UUrStartupMessage("initializing scripting");
	error = SLrScript_Initialize();
	UUmError_ReturnOnError(error);
	
	UUrStartupMessage("initializing binary data system");
 	error = BDrInitialize();
 	UUmError_ReturnOnError(error);
	 	
	UUrStartupMessage("initializing imaging");
	error = IMrInitialize();
	UUmError_ReturnOnError(error);
	
	UUrStartupMessage("initializing motoko");
	error = M3rInitialize();
	UUmError_ReturnOnError(error);

	UUrStartupMessage("initializing physics");
	error = PHrInitialize();
	UUmError_ReturnOnError(error);

	UUrStartupMessage("initializing oni motoko");
	error = ONrMotoko_Initialize();
	UUmError_ReturnOnError(error);
	
	UUrStartupMessage("initializing local input");
	error = LIrInitialize(outPlatformData->appInstance, outPlatformData->gameWindow);
	UUmError_ReturnOnError(error);

	UUrStartupMessage("initializing animation system");
	error = TRrInitialize();
	UUmError_ReturnOnError(error);
		
	UUrStartupMessage("initializing environment");
	error = AKrInitialize();
	UUmError_ReturnOnError(error);
		
	UUrStartupMessage("initializing text system");
	error = TSrInitialize();
	UUmError_ReturnOnError(error);

	UUrStartupMessage("initializing the console");
	error =	COrInitialize();
	UUmError_ReturnOnError(error);

 	UUrStartupMessage("initializing the materials");
	error = MArMaterials_Initialize();
	UUmError_ReturnOnErrorMsg(error, "Could not initialize the materials");
	
	UUrStartupMessage("initializing the full sound system 2");
	error = SS2rInitializeFull();
 	UUmError_ReturnOnError(error);
	 	
	UUrStartupMessage("initializing particle 3");
	error = P3rInitialize();
	UUmError_ReturnOnError(error);

	UUrStartupMessage("initializing oni particle 3");
	error = ONrParticle3_Initialize();
	UUmError_ReturnOnError(error);

	UUrStartupMessage("initializing env particle system");
	error = EPrInitialize();
	UUmError_ReturnOnError(error);
	

	UUrStartupMessage("initializing physics");
	error = PHrPhysics_Initialize();
	UUmError_ReturnOnError(error);

	UUrStartupMessage("initializing game state");
	error = ONrGameState_Initialize();
	UUmError_ReturnOnError(error);
		
	UUrStartupMessage("initializing AI 2");
	error = AI2rInitialize();
	UUmError_ReturnOnError(error);
	
	UUrStartupMessage("initializing window manager");
	error = WMrInitialize();
	UUmError_ReturnOnError(error);
	
	UUrStartupMessage("initializing film system");
	error = ONrFilm_Initialize();
	UUmError_ReturnOnError(error);
	
	UUrStartupMessage("initializing level");
	error = ONrLevel_Initialize();
	UUmError_ReturnOnError(error);
	
	UUrStartupMessage("initializing oni scripting");
	error = ONrScript_Initialize();
	UUmError_ReturnOnError(error);
	
	UUrStartupMessage("initializing OBDr");
	error = OBDrInitialize();
	UUmError_ReturnOnError(error);
		
	UUrStartupMessage("initializing OBJr");
 	error = OBJrInitialize();
 	UUmError_ReturnOnError(error);
	
	UUrStartupMessage("initializing oni cinematics");
 	error = OCrInitialize();
 	UUmError_ReturnOnError(error);
	 
	UUrStartupMessage("initializing oni sound");
 	error = OSrInitialize();
 	UUmError_ReturnOnError(error);
	
	UUrStartupMessage("initializing oni movie");
	error = ONrMovie_Initialize();
	UUmError_ReturnOnError(error);
		
	UUrStartupMessage("initializing the pause screen");
	error = ONrInGameUI_Initialize();
	UUmError_ReturnOnError(error);

	CLrInitialize();
 	
	UUrStartupMessage("finished oni initializing");
	
	return UUcError_None;
}

static UUtBool
ONiTest_Proc(
	UUtUns16	inNodeIndex)
{
	return UUcFalse;
}

#ifndef USE_OPENGL_WITH_BINK
static UUtBool play_ending_movie_at_exit= UUcFalse;
#endif

static UUtError
ONiRunGame(
	void)
{
	UUtError		error;

	UUtUns16		numActionsInBuffer;		// This is also the number of elapsed ticks
	LItAction		*actionBuffer;
	UUtUns32		game_ticks;

	UUtInt64		time_frame_start, time_frame_end;
	
	#if defined(BRENTS_CHEESY_GAME_PERF) && BRENTS_CHEESY_GAME_PERF
		#define RECENT_FRAME_COUNT 128
				
		UUtInt64	time_gamestateupdate_start;
		UUtInt64	time_gamestateupdate_end;

		UUtInt64	time_gamestatedisplay_start;
		
		UUtUns32	time_frame_cur;
		static UUtUns32	time_frame_recent[RECENT_FRAME_COUNT];
		static UUtUns32	time_frame_count;
		UUtUns32	time_frame_total = 0;
		UUtUns32	time_gamestateupdate_cur;
		UUtUns32	time_gamestateupdate_total = 0;
		UUtUns32	time_gamestatedisplay_cur;
		UUtUns32	time_gamestatedisplay_total = 0;							
	#endif
	
//UUrProfile_State_Set(UUcProfile_State_On);
	
	while(!ONgTerminateGame)
	{
		time_frame_start = UUrMachineTime_High();
		
		#if defined(SScMSADPCMTimer) && (SScMSADPCMTimer == 1)
		SS2rFrame_Start();
		#endif
		
		// step 0	update the local input
		LIrUpdate();
		
		// step 1	clear the matrix stack, frame start ?
		M3rMatrixStack_Clear();
		
		// step 2	update the windows
		WMrUpdate();
		OWrUpdate();
		
		if (!ONrGameState_IsPaused())
		{
			// step 3	process local input
			LIrActionBuffer_Get(&numActionsInBuffer, &actionBuffer);
			
			{
				UUtUns32 itr;

				for(itr = 0; itr < numActionsInBuffer; itr++)
				{
					LItAction *current_action = actionBuffer + itr;

					current_action->buttonBits &= ONgGameState->key_mask;
				}
			}

			// step 4	update the console time
			error =	COrConsole_Update(numActionsInBuffer);
			UUmError_ReturnOnErrorMsg(error, "Could not update the console.");
			
			ONrGameState_UpdateServerTime(ONgGameState);

			#if defined(BRENTS_CHEESY_GAME_PERF) && BRENTS_CHEESY_GAME_PERF
				
				time_gamestateupdate_start = UUrMachineTime_High();
				
			#endif
		
			// step 6	update the game
			error = ONrGameState_Update(numActionsInBuffer, actionBuffer, &game_ticks);
			UUmError_ReturnOnErrorMsg(error, "Could not update game state.");
		
			#if defined(BRENTS_CHEESY_GAME_PERF) && BRENTS_CHEESY_GAME_PERF
				
				time_gamestateupdate_end = UUrMachineTime_High();
				
			#endif
		}

		// step 7	play the sounds
		SS2rUpdate();
		
		// step 8 draw current game state

		#if defined(BRENTS_CHEESY_GAME_PERF) && BRENTS_CHEESY_GAME_PERF
			
			time_gamestatedisplay_start = UUrMachineTime_High();
			
		#endif

		if (ONgGameState->local.pending_splash_screen[0] != '\0') {
			ONrGameState_SplashScreen(ONgGameState->local.pending_splash_screen, NULL, UUcFalse);

			ONgGameState->local.pending_splash_screen[0] = '\0';
		}
		else {
			M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare, ONgMotoko_ZCompareOn ? M3cDrawState_ZCompare_On : M3cDrawState_ZCompare_Off);
			M3rDraw_State_SetInt(M3cDrawStateIntType_BufferClear, ONgMotoko_BufferClear ? M3cDrawState_BufferClear_On : M3cDrawState_BufferClear_Off);
			M3rDraw_State_SetInt(M3cDrawStateIntType_DoubleBuffer, ONgMotoko_DoubleBuffer ? M3cDrawState_DoubleBuffer_On : M3cDrawState_DoubleBuffer_Off);
			M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_On);
			M3rDraw_State_SetInt(M3cDrawStateIntType_ClearColor, ONgMotoko_ClearColor);
			M3rDraw_State_Commit();

			M3rGeom_Frame_Start(game_ticks);
				ONrGameState_Display();
				WMrDisplay();
			M3rGeom_Frame_End();
		}

		time_frame_end = UUrMachineTime_High();
		
		// step 9 calculate the fps
		
		ONgNumFrames++;
		
		#if defined(BRENTS_CHEESY_GAME_PERF) && BRENTS_CHEESY_GAME_PERF
		{
			extern UUtBool ONgShowPerformance_Overall;
			
			time_frame_cur = (UUtUns32)(time_frame_end - time_frame_start);
			time_frame_total += time_frame_cur;
			time_frame_recent[time_frame_count] = time_frame_cur;
			time_frame_count = (time_frame_count + 1) % RECENT_FRAME_COUNT;
			time_gamestateupdate_cur = (UUtUns32)(time_gamestateupdate_end - time_gamestateupdate_start);
			time_gamestateupdate_total += time_gamestateupdate_cur;
			
			time_gamestatedisplay_cur = (UUtUns32)(time_frame_end - time_gamestatedisplay_start);
			time_gamestatedisplay_total += time_gamestatedisplay_cur;

#if PERFORMANCE_TIMER
			if(ONgShowPerformance_Overall)
			{
				char s1[128];
				char s2[128];
				char s3[128];
				
				float frames_per_second_cur;
				float frames_per_second_avg;

				float time_gamestateupdate_per_frame_cur;
				float time_gamestateupdate_per_frame_avg;
				float time_gamestatedisplay_per_frame_cur;
				float time_gamestatedisplay_per_frame_avg;
 
				frames_per_second_cur = (float)(1.0 / (double)time_frame_cur * UUgMachineTime_High_Frequency);
				frames_per_second_avg = (float)(((double)ONgNumFrames / (double)time_frame_total) * UUgMachineTime_High_Frequency);
				
				time_gamestateupdate_per_frame_cur = (float)time_gamestateupdate_cur / (float)time_frame_cur;
				time_gamestateupdate_per_frame_avg = (float)time_gamestateupdate_total / (float)time_frame_total;
				
				time_gamestatedisplay_per_frame_cur = (float)time_gamestatedisplay_cur / (float)time_frame_cur;
				time_gamestatedisplay_per_frame_avg = (float)time_gamestatedisplay_total / (float)time_frame_total;
				
				sprintf(s1, "fps_cur: %03.1f, fps_avg: %03.1f",	frames_per_second_cur, frames_per_second_avg);
				sprintf(s2, "gsu/f cur: %02.1f, gsu/f avg: %02.1f",	time_gamestateupdate_per_frame_cur * 100.0f, time_gamestateupdate_per_frame_avg * 100.0f);
				sprintf(s3,	"gsd/f cur: %02.1f, gsd/f avg: %02.1f", time_gamestatedisplay_per_frame_cur * 100.0f, time_gamestatedisplay_per_frame_avg * 100.0f);
				
				ONrGameState_Performance_UpdateOverall(
					s1,
					s2,
					s3);
			}
#else
			if(ONgShowPerformance_Overall)
			{
				char s1[128];
				char s2[128];
				UUtUns32 time_frame_index;
				UUtUns32 time_frame_recent_total;
                                extern UUtUns32 triCounter, quadCounter, pentCounter;
                                
				float frames_per_second_recent;

 				time_frame_recent_total = 0;
 				for (time_frame_index = 0; time_frame_index < RECENT_FRAME_COUNT; time_frame_index++) {
 					time_frame_recent_total += time_frame_recent[time_frame_index];
 				}
 				
				frames_per_second_recent = (float)(RECENT_FRAME_COUNT / (double)time_frame_recent_total * UUgMachineTime_High_Frequency);
				
				sprintf(s1, "fps:%03.1f 3:%d 4:%d 5:%d", frames_per_second_recent, triCounter, quadCounter, pentCounter);
				sprintf(s2, "tc:%d tm:%d", gl->num_loaded_textures, gl->current_texture_memory);

				ONrGameState_Performance_UpdateOverall(s1, s2, NULL);
                                
                                triCounter = 0;
                                quadCounter = 0;
                                pentCounter = 0;
			}
#endif
		}
		#endif
		
		#if defined(SScMSADPCMTimer) && (SScMSADPCMTimer == 1)
		SS2rFrame_End();
		#endif

		if (ONgGameState->local.pending_pause_screen) {
			LIrInputEvent_CheatHook(ONrGameState_HandleCheats);
			ONrPauseScreen_Display();
			LIrInputEvent_CheatHook(NULL);

			ONgGameState->local.pending_pause_screen = UUcFalse;
		}
			
		switch(ONgGameState->victory)
		{
			case ONcWin:
				{
					UUtUns16 old_level = ONgGameState->levelNumber;
					UUtUns16 next_level = ONrLevel_GetNextLevel(ONgGameState->levelNumber);

					ONrGameState_ClearContinue();
					
					ONrGameState_SplashScreen(ONcWinSplashScreen, OScMusicScore_Win, UUcTrue);
					ONrLevel_Unload();

					if (19 == old_level) {
						ONrPersist_MarkWonGame();

						if (NULL != strstr(gl->renderer, "3Dfx")) {
							ONrMovie_Play_Hardware("outro.bik", BKcScale_Fill_Window);
						}
						else {
#ifndef USE_OPENGL_WITH_BINK
							play_ending_movie_at_exit= UUcTrue;
#endif
						}

						ONgTerminateGame = UUcTrue;
					}
					else if (next_level != 0) {
						ONrLevel_Load(next_level, UUcTrue);	
					}
					else {
						ONgTerminateGame = UUcTrue;
					}
				}
			break;

			case ONcLose:
				{
					UUtUns16 old_level = ONgGameState->levelNumber;
					
					ONrGameState_SplashScreen(ONcFailSplashScreen, OScMusicScore_Lose, UUcTrue);
					ONrLevel_Unload();
					ONrLevel_Load(ONgGameState->levelNumber, UUcTrue);	
				}
				break;
		}
	}
	UUrMemory_Block_VerifyList();

	return UUcError_None;
}

static void
ONiConsole_Platform_Report(
	UUtUns32			inArgC,
	char**				inArgV,
	void*				inRefcon)
{
	UUrPlatform_Report();
}

static UUtError ONiCreateConsoleVariables(void)
{
	
	#if 0
	error = 
		COrCommand_New(
			"platform_report",
			"reports information about the target platform",
			ONiConsole_Platform_Report,
			NULL);
	UUmError_ReturnOnError(error);
	#endif
	
	return UUcError_None;
}

static void RunKeyConfigFile(const char *inFileName)
{
	UUtError		error;
	BFtFileRef		configFileRef;
	BFtTextFile*	configFile;
	char*			curLine;
	
	error = BFrFileRef_Search(inFileName, &configFileRef);
	if(error != UUcError_None) {
		goto exit;
	}
	
	if (!BFrFileRef_FileExists(&configFileRef)) {
		goto exit;
	}

	error = BFrTextFile_OpenForRead(&configFileRef, &configFile);

	if (error != UUcError_None) {
		goto exit;
	}
	
	while(1)
	{
		curLine = BFrTextFile_GetNextStr(configFile);

		if(curLine == NULL) {
			break;
		}

		// skip comments
		switch(curLine[0])
		{
			case '#':
			case '\n':
			case '\r':
			case '\v':
			case '\0':
				continue;

		}

		{
			char *command_name;

			command_name = strtok(curLine, " \t");

			if (NULL != command_name) {
				if (0 == UUrString_Compare_NoCase_NoSpace(command_name, "unbindall")) {
					LIrBindings_RemoveAll();
				}
				else if (0 == UUrString_Compare_NoCase_NoSpace(command_name, "bind")) {
					char *bound_input = NULL;
					char *to = NULL;
					char *action_name = NULL;
					char *current_token;

					current_token = strtok(NULL, " \t");

					if (NULL != current_token) {
						bound_input = current_token;
						current_token = strtok(NULL, " \t");
					}

					if (NULL != current_token) {
						to = current_token;
						current_token = strtok(NULL, " \t");
					}

					if (NULL != current_token) {
						action_name = current_token;
					}

					if ((NULL == bound_input) || (NULL == to) || (NULL == action_name) || (0 != UUrString_Compare_NoCase_NoSpace(to, "to"))) {
						COrConsole_Printf("failed to parse bind, expected:");
						COrConsole_Printf("bind <key> to <action>");
					}
					else {
						UUtError error;

						error = LIrBinding_Add(LIrTranslate_InputName(bound_input), action_name);
					}
				}
			}
		}
	}
	
	BFrTextFile_Close(configFile);

exit:	
	return;
}

static void KeyConfig(void)
{
#if TOOL_VERSION
	return;
#endif

	{
 		FILE *key_config = fopen("key_config.txt", "r");

		if (NULL == key_config) {
			key_config = fopen("key_config.txt", "w");

			if (NULL != key_config) {
				const char **loop;
				const char *default_key_config_file[] =
				{
					"unbindall",
					"",
					"bind w to forward",
					"bind a to stepleft",
					"bind s to backward",
					"bind d to stepright",
					"bind q to swap",
					"bind e to drop",
					"bind f to punch",
					"bind c to kick",
					"",
					"bind space to jump",
					"bind mousebutton1 to fire1",
					"bind mousebutton2 to fire2",
					"bind mousebutton3 to fire3",
					"bind mousexaxis to aim_LR",
					"bind mouseyaxis to aim_UD",
					"bind fkey1 to pausescreen",
					"bind v to lookmode",
					"",
					"bind leftshift to crouch",
					"bind rightshift to crouch",
					"",
					"bind capslock to walk",
					"bind leftcontrol to action",
					"",
					"bind rightcontrol to action",
					"bind tab to hypo",
					"bind r to reload",
					"bind backslash to profile_toggle",
					"bind fkey13 to screenshot",
					"",
					"bind p to forward",
					"bind l to stepleft",
					"bind apostrophe to stepright",
					"bind semicolon to backward",
					"bind o to swap",
					"bind leftbracket to drop",
					"bind k to punch",
					"bind comma to kick",
					"bind enter to walk",
					"",
					NULL
				};

				for(loop = default_key_config_file; *loop != NULL; loop++)
				{
					fprintf(key_config, "%s\n", *loop);
				}
			}
		}

		if (NULL != key_config) {
			fclose(key_config);
		}
	}

	RunKeyConfigFile("key_config.txt");

	return;
}

void OniExit(
	void)
{
	UUrMemory_Block_VerifyList();

	UUrStartupMessage("beginning exit process...");

	// draw context needs to die first
	ONrMotoko_TearDownDrawing();

	UUrMemory_Block_VerifyList();
	
	// destropy the oni windows
	OWrTerminate();
	
	// destroy the window manager before unloading level 0
	WMrTerminate();

	// Unload level 0
	TMrLevel_Unload(0);
		
	UUrMemory_Block_VerifyList();
	TMrTerminate();

	UUrMemory_Block_VerifyList();

	M3rTerminate();
    
#ifndef USE_OPENGL_WITH_BINK
    // now is the time to play the ending movie if we're going to do it in software
    // (after Motoko is terminated but while local input is still working)
    if (play_ending_movie_at_exit == UUcTrue)
    {
        ONrMovie_Play("outro.bik", BKcScale_Fill_Window);
    }
#endif
    
	LIrTerminate();
	
	ONrInGameUI_Terminate();
	WPrTerminate();
	ONrImpactEffects_Terminate();
	ONrTextureMaterials_Terminate();
	PHrTerminate();
	OSrTerminate();
	OCrTerminate();
	OBDrTerminate();
	OBJrTerminate();
//	ONrDataConsole_Terminate();
	ONrLevel_Terminate();
	AI2rTerminate();
	CArTerminate();
	ONrFilm_Terminate();
	ONrGameState_Terminate();
	ONrPlatform_Terminate();
	PHrPhysics_Terminate();
	AKrTerminate();
	EPrTerminate();
	P3rTerminate();
	TRrTerminate();
	TSrTerminate();
	COrTerminate();
	SS2rTerminate();
	MArMaterials_Terminate();
	BDrTerminate();
	IMrTerminate();
	ONrScript_Terminate();
	SLrScript_Terminate();
		
	UUrMemory_Block_VerifyList();
	UUrStartupMessage("oni exit complete, shutting down...");

	// change this if we ever want to check for leaks
	UUrTerminate();

	return;
}

static UUtError
ONiMain(
	UUtUns16	argc,
	char**		argv)
{
	UUtError					error;
	
	ONtGameState*				gameState = NULL;
	
	
	// Brents debugging stuff
	#if 0
	{
		#define ns (30)
		
		UUtUns32	i;
		UUtInt64*	samples;
		UUtUns32	final[ns];
		
		samples = UUrMemory_Block_New(sizeof(UUtInt64) * ns);
		UUmError_ReturnOnNull(samples);
		
		for(i = 0; i < ns; i++)
		{
			Microseconds((UnsignedWide*)&samples[i]);
			Delay(60, &final[i]);
		}
		
		for(i = 1; i < ns; i++)
		{
			fprintf(stderr, "%d, %d\n", (UUtInt32)(samples[i] - samples[i-1]), final[i]);
		}
		
		return;
	}
	#endif
	
	if(OniParseCommandLine(argc, argv) != UUcError_None)
	{
		return UUcError_BadCommandLine;
	}
	
	/*
	 * Initialize the Universal Utilities. This does a base level platform init. So if
	 * this succedes we can bring up error dialogs
	 */
		error =
			UUrInitialize(
				UUcTrue);	// Init basic platform also
		if(error != UUcError_None)
		{
			/* XXX - This is really bad - should never happen */
			return error;
		}
		
	#if defined(PROFILE) && PROFILE
		error = UUrProfile_Initialize();
		UUmError_ReturnOnErrorMsg(error, "Could not initialize profiler.");
		
		UUrProfile_State_Set(UUcProfile_State_Off);
	#endif
		
	/*
	 * Initialize all components
	 */
		error = ONiInitializeAll(&ONgPlatformData);
		UUmError_ReturnOnErrorMsg(error, "Could not initialize components.");
	
	/*
	 * Load Level Zero
	 */
		error = ONrLevel_LoadZero();
		UUmError_ReturnOnErrorMsg(error, "Could not load level zero");
		
#ifndef USE_OPENGL_WITH_BINK
	/*
	 * play movie before OpenGL takes over
	 */
		ONrMovie_Play("intro.bik", BKcScale_Fill_Window);
#endif

	/*
	 * Get the motoko drawing context
	 */
		error = 
			ONrMotoko_SetupDrawing(
				&ONgPlatformData);
		UUmError_ReturnOnError(error);
	
		// Now new texture maps can be created, so aiming can make its texture
		AMrInitialize();
			
	/*
	 * initialize the Oni Window
	 */
		UUrStartupMessage("Initializing the Oni Window...");	
		error = OWrInitialize();
		UUmError_ReturnOnErrorMsg(error, "Unable to initialize the Oni Window");

	/*
	 * display the splash screen
	 */
		UUrStartupMessage("displaying splash screen...");
		OWrSplashScreen_Display();

	/*
	 * Configure the console
	 */
		UUrStartupMessage("configuring console...");

		error = ONiCreateConsoleVariables();
		UUmError_ReturnOnErrorMsg(error, "Could not create the console variables.");

		error = COrConfigure();
		UUmError_ReturnOnErrorMsg(error, "Could not set up the Console.");

#ifdef USE_OPENGL_WITH_BINK
	/*
	 * play movie after OpenGL is initialized
	 */
		ONrMovie_Play("intro.bik", BKcScale_Fill_Window);
#endif

		KeyConfig();
		
	/*
	 * run the main menu
	 */
		UUrStartupMessage("engine startup complete, launch the out-of-game UI...");
		OWrOniWindow_Startup();

		UUrStartupMessage("out-of-game UI exited...");
	if (ONgTerminateGame == UUcFalse)
	{
		/*
		 * Load the level
		 */
			
#if TOOL_VERSION
		// Run the console config file	
		UUrStartupMessage("loading config file...");
		if (COcConfigFile_Read == ONgCommandLine.readConfigFile) COrRunConfigFile("oni_config.txt");
#endif

		
		/*
		 * Run the game
		 */
				
		UUrStartupMessage("running game...");
		error =	ONiRunGame();
		UUmError_ReturnOnErrorMsg(error, "error running game");
		
		#if defined(PROFILE) && PROFILE
	
			UUrProfile_State_Set(UUcProfile_State_Off);
	
			UUrProfile_Dump("OniProfile");
			UUrProfile_Terminate();
		
		#endif
	
		UUrMemory_Block_VerifyList();

		if (NULL != ONgLevel) {
			UUrStartupMessage("game over, unloading level...");
			ONrLevel_Unload();
		}
	}
	
	OniExit();

	return UUcError_None;
}

#if UUmSDL
#include "SDL2/SDL_main.h"
#endif

int UUcExternal_Call
main(
	int		argc,
	char*	argv[])
{
	UUtError	error;

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac) && DEBUGGING // if Macintosh...
	if (1) // take command line args if it is not a shipping version of the game
#else // shift key reads in command line params
	if (LIrTestKey(LIcKeyCode_LeftShift) || LIrTestKey(LIcKeyCode_RightShift))
#endif
	{	
		argc= CLrGetCommandLine(argc, argv, &argv);
	}

	error= ONiMain(argc, argv);

	return 0;
}
