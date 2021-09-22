/*
	Oni_Level.c
	
	Level stuff
	
	Author: Quinn Dunki
	c1998 Bungie
*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "BFW_Console.h"
#include "BFW_BitVector.h"
#include "BFW_AppUtilities.h"
#include "BFW_TextSystem.h"
#include "BFW_ScriptLang.h"
#include "BFW_TM_Construction.h"
#include "BFW_Materials.h"

#include "Oni_AI.h"
#include "Oni_AI_Script.h"
#include "Oni_AStar.h"
#include "Oni_Character.h"
#include "Oni_BinaryData.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"
#include "Oni.h"
#include "Oni_Weapon.h"
#include "Oni_Windows.h"
#include "Oni_Script.h"
#include "Oni_Object.h"
#include "Oni_Particle3.h"
#include "Oni_Sound2.h"
#include "Oni_Mechanics.h"
#include "Oni_ImpactEffect.h"
#include "Oni_TextureMaterials.h"
#include "Oni_Sound_Animation.h"
#include "Oni_InGameUI.h"
#include "Oni_OutGameUI.h"

UUtError AIrCreatePlayerFromTextFile(void)
{
	const AItCharacterSetup *setup;
	ONtFlag	flag;
	UUtError error;
	UUtUns16 itr, char_index;
	
	// player character is denoted by script ID 0
	setup = ONrLevel_CharacterSetup_ID_To_Pointer(0);
	if (setup == NULL) {
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "No player character, and no character with script id 0!");
	}

	// try to find the starting flag for this character
	if (!ONrLevel_Flag_ID_To_Flag(setup->defaultFlagID, &flag)) {
		COrConsole_Printf("warning: could not find flag %d for character 0", setup->defaultFlagID);

		for(itr = 0; itr <= 1000; itr++) {
			if (ONrLevel_Flag_ID_To_Flag(itr, &flag))
				break;
		}

		if (itr > 1000) {
			// there are _no_ flags that we can find. set up a temporary flag at 0, 0, 0
			flag.matrix = MUgIdentityMatrix;
			MUmVector_Set(flag.location, 0, 0, 0);
			flag.rotation = 0;
			flag.idNumber = 0;
			flag.deleted = flag.maxFlag = UUcFalse;
		}
	}

	// create the character at this flag
	error = ONrGameState_NewCharacter(NULL, setup, &flag, &char_index);
	UUmError_ReturnOnError(error);
	
	// set this character to be the player controlled character
	ONrGameState_SetPlayerNum(char_index);
		
	return UUcError_None;
}

typedef struct ONtLevel_FindLocation
{
	const OBJtObject	*closest_object;
	float				closest_distance;	/* distance of the closest_object from position */
	M3tPoint3D			position;			/* find object closest to this position */
	
} ONtLevel_FindLocation;

// ---

static UUtUns16				ONgCurrentLevel;
static WMtDialog			*ONgLevelLoadProgress;

TMtPrivateData*				ONgTemplate_Level_PrivateData = NULL;
TMtPrivateData*				ONgTemplate_FlagArray_PrivateData = NULL;

static UUtError
ONiFlagArray_TemplateHandler(
	TMtTemplateProc_Message	inMessage,
	void*					inDataPtr,
	void*					inPrivateData);

static UUtError ONrLevel_InitializeActionMarkerArray()
{
	UUmAssert( ONgLevel );

	UUrMemory_Clear( &ONgLevel->actionMarkerArray, sizeof(ONtActionMarkerArray));

	return UUcError_None;
}

static UUtError ONrLevel_UninitializeActionMarkerArray()
{

	return UUcError_None;
}

static UUtError
ONiLevel_SetDefaultReverb(
	UUtUns16				inLevelNum)
{
	UUtUns16				num_descriptors;
	UUtError				error;
	
	num_descriptors = (UUtUns16)TMrInstance_GetTagCount(ONcTemplate_Level_Descriptor);

	if (num_descriptors > 0)
	{
		UUtUns16			i;

		for (i = 0; i < num_descriptors; i++)
		{
			ONtLevel_Descriptor		*descriptor;
			
			// get a pointer to the first descriptor
			error =
				TMrInstance_GetDataPtr_ByNumber(
					ONcTemplate_Level_Descriptor,
					i,
					&descriptor);
			if (error != UUcError_None) return error;
			
			if (descriptor->level_number == inLevelNum)
			{
#if 0			
				// set the default reverb for the level
				SSrReverb_SetDefault(descriptor->default_reverb);
#endif
			}
		}
	}
	
	return UUcError_None;
}

static UUtBool
ONiDialog_LevelLoadProgress(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool					handled;
	WMtWindow				*progress_bar;
	
	handled = UUcTrue;
	
	// get the progress bar
	progress_bar = WMrDialog_GetItemByID(inDialog, OWcLevelLoad_ProgressBar);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			WMrMessage_Send(progress_bar, WMcMessage_SetValue, 0, 0);
		break;
		
		case WMcMessage_SetValue:
			WMrMessage_Send(progress_bar, WMcMessage_SetValue, inParam1, 0);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

static PStPartSpecUI			*ONgActiveUI;

static void
ONiLevel_LevelLoadDialog_Start(
	void)
{
	WMtDialogID				id;
	
	if (ONgLevelLoadProgress != NULL) { return; }
	
	// load the dialog
#if SHIPPING_VERSION
	id = OWcDialog_OGU_LevelLoadProgress;
	ONgActiveUI = PSrPartSpecUI_GetActive();
	PSrPartSpecUI_SetActive(PSrPartSpecUI_GetByName(ONcOutGameUIName));
#else
	id = OWcDialog_LevelLoadProgress;
#endif
	
	WMrDialog_Create(
		id,
		NULL,
		ONiDialog_LevelLoadProgress,
		(UUtUns32) -1,
		&ONgLevelLoadProgress);
	
	// display the dialog on the screen
	M3rGeom_Frame_Start(0);
	WMrDisplay();
	M3rGeom_Frame_End();
}

static void
ONiLevel_LevelLoadDialog_Stop(
	void)
{
	if (ONgLevelLoadProgress == NULL) { return; }
	
	// tell the dialog to stop and do one more update so it
	// can process the stop order
	WMrWindow_Delete(ONgLevelLoadProgress);
	ONgLevelLoadProgress = NULL;

#if SHIPPING_VERSION
	PSrPartSpecUI_SetActive(ONgActiveUI);
#endif
}

void
ONrLevel_LevelLoadDialog_Update(
	UUtUns8				inPercentComplete)
{
	if (ONgLevelLoadProgress == NULL) { return; }

	// update the dialog's progress bar
	WMrMessage_Send(ONgLevelLoadProgress, WMcMessage_SetValue, (UUtUns32)inPercentComplete, 0);
	
	// display the dialog on the screen
	M3rGeom_Frame_Start(0);
	WMrDisplay();
	M3rGeom_Frame_End();
}

UUtUns16
ONrLevel_GetCurrentLevel(
	void)
{
	return ONgCurrentLevel;
}

void
ONrLevel_Unload(
	void)
{
	COrClear();
	P3rNotifyUnloadBegin();
	OSrLevel_Unload();			/* must come before OBJrLevel_Unload() */

	OBDrLevel_Unload();
	OBJrLevel_Unload();			/* must come after OBDrLevel_Unload() */

	ONrSky_LevelEnd( &ONgGameState->sky );

	ONrMechanics_LevelEnd();
	OBJrObject_LevelEnd();
	
	ONrInGameUI_LevelUnload();
	AIrLevelEnd();
	AI2rLevelEnd();
	OBrDoor_Array_LevelEnd(&ONgGameState->doors);
	ONrGameState_LevelEnd_Objects();
	AKrLevel_End();
	CArLevelEnd();
	PHrDisposeGraph(&ONgGameState->local.pathGraph);
	EPrLevelEnd();
	ONrParticle3_LevelEnd();
	WPrLevel_End();					// CB: must come before TMrLevel_Unload because it needs to access weapon classes
	P3rLevelEnd();					// CB: must come after WPrLevel_End because weapons try to delete their particles
	ONrGameState_LevelEnd();			// ME: must come after WPrLevel_End because it deletes the all physics contexts

	SSrStopAll();					// CB: this makes sure we don't start any sounds between OSrLevel_Unload and here that have
									// pointers to disposed sound data
	TMrLevel_Unload(ONgCurrentLevel);
	SS2rSoundData_FlushDeallocatedPointers();	// CB: must come after TMrLevel_Unload, it flushes pointers to disposed sound data
	WMrLevel_Unload();
	
	UUrMemory_Leak_StopAndReport();
	
	if (ONgCurrentLevel == 0)
	{
		ONgCurrentLevel = (UUtUns16)(-1);
	}
	else
	{
		ONgCurrentLevel = 0;
	}

	// reset fog
	M3rDraw_Reset_Fog();

	ONgLevel = NULL;
}

static UUtError ONrLevel_TemplateHandler(
	TMtTemplateProc_Message	inMessage,
	void*					inDataPtr,
	void*					inPrivateData)
{
	/**************
	* Level template handler
	*/
	
	ONtLevel *level = (ONtLevel *)inDataPtr;
		
	switch(inMessage)
	{
		case TMcTemplateProcMessage_Update:
		case TMcTemplateProcMessage_DisposePreProcess:
			break;
		
		case TMcTemplateProcMessage_NewPostProcess:		
		case TMcTemplateProcMessage_LoadPostProcess:				
			break;
			
			
		default:
			UUmAssert(!"Illegal message");
	}

	return UUcError_None;
}


UUtError ONrLevel_Initialize(
	void)
{
	/******************
	* Performs game launch init for level data structures
	*/
	
	UUtError error;
	
	// Install level template's private data
	error = TMrTemplate_PrivateData_New(
		ONcTemplate_Level,
		0,
		ONrLevel_TemplateHandler,
		&ONgTemplate_Level_PrivateData);
	UUmError_ReturnOnError(error);
	
	// Install flag array template's proc handler
	error = TMrTemplate_PrivateData_New(
		ONcTemplate_FlagArray,
		0,
		ONiFlagArray_TemplateHandler,
		&ONgTemplate_FlagArray_PrivateData);
	UUmError_ReturnOnError(error);
	
	ONgCurrentLevel = 0;
//	ONgLevelLoadDialog = NULL;
	
	return UUcError_None;
}

void ONrLevel_Terminate(
	void)
{
	/****************
	* Performs game shutdown of level data structures
	*/
	
	// can't possibly imagine what might go here, because the
	// template manager handles private data shutdown
}

UUtError
ONrLevel_LoadZero(
	void)
{
	UUtError			error;
	
	UUrStartupMessage("loading level 0...");

	// load the texture materials
	error = ONrTextureMaterials_Initialize();
	UUmError_ReturnOnErrorMsg(error, "Could not initialize the texture materials");
	
	// initialize the impact effects
	error = ONrImpactEffects_Initialize();
	UUmError_ReturnOnErrorMsg(error, "Could not initialize the impact effects");

	error = TMrLevel_Load(0, ONgCommandLine.allowPrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not load level 0");

	// read the game settings instance
	error = ONrGameSettings_Initialize();
	UUmError_ReturnOnErrorMsg(error, "Could not init game settings");

	// preprocess the material and impact types
	error = MArImpacts_PreProcess();
	UUmError_ReturnOnErrorMsg(error, "Could not initialize the impact types");

	error = MArMaterials_PreProcess();
	UUmError_ReturnOnErrorMsg(error, "Could not initialize the materials types");

	// initialize the oni sound variant list
	error = OSrLevel_Load(0);
	UUmError_ReturnOnErrorMsg(error, "The sound system didn't like level 0");
	
	// load binary data files (p3 particles, sounds, etc)
	error = OBDrLevel_Load(0);
	UUmError_ReturnOnErrorMsg(error, "Could not load binary data for level 0");
	
	ONrIEBinaryData_Process();
	ONrTextureMaterials_PreProcess();

	// if no impact effects were loaded, create a blank table
	if (!ONgImpactEffect_Loaded) {
		error = ONrImpactEffects_CreateBlank();
		UUmError_ReturnOnErrorMsg(error, "Could not create blank impact effect table");
	}

	OSrUpdateSoundPointers();

	P3rLoad_PostProcess();
	ONrParticle3_LevelZeroLoaded();

	return UUcError_None;
}

static UUtError ONrLevel_ParticleLoad()
{
	UUtError		error;

	// Particle3 particle system
	error = P3rLevelBegin();
	UUmError_ReturnOnError(error);

	// environmental particles
	error = EPrLevelBegin();
	UUmError_ReturnOnError(error);

	// oni particlesystem interface
	error = ONrParticle3_LevelBegin();
	UUmError_ReturnOnError(error);

	// begin particles for the level
	EPrLevelStart(0);
	
	return UUcError_None;
}

extern UUtUns16 TRgLevelNumber;

UUtError ONrLevel_Load(UUtUns16 inLevelNum, UUtBool inProgressBar)
{
	UUtError		error;
	ONtCharacter *	player_character;

	ONgLevel		= NULL;
	TRgLevelNumber = inLevelNum;

	UUrMemory_Leak_Start("These leaks are between level load and unload");
	
	// disable all sound except stuff coming from scripts
	OSrSetScriptOnly(UUcTrue);
	
	// display the level load dialog
	if (inProgressBar) {
		ONiLevel_LevelLoadDialog_Start();
		ONrLevel_LevelLoadDialog_Update(10);
	}

	// load the level from storage
	error = TMrLevel_Load(inLevelNum, TMcPrivateData_Yes);
	UUmError_ReturnOnError(error);
	
	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(20);
	}
	
	// prepare the object system for data read by the binary data loader
	error = OBJrLevel_Load(inLevelNum);
	UUmError_ReturnOnError(error);
	
	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(24);
	}

	// load binary data for the level (p3 particles, furniture, object placement etc)
	error = OBDrLevel_Load(inLevelNum);
	UUmError_ReturnOnErrorMsg(error, "Could not load binary data");
	
	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(25);
	}

	// Reset all the textures for the card
	M3rDrawContext_ResetTextures();
	
	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(28);
	}

	// Initialize the game state
	error = ONrGameState_LevelBegin(inLevelNum);
	UUmError_ReturnOnErrorMsg(error, "Could not initialize game state");

	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(30);
	}

	// texture materials must be loaded before akira (needed to set up breakable flag)
	ONrTextureMaterials_LevelLoad();
	
	// akira
	error = AKrLevel_Begin(ONgGameState->level->environment);
	UUmError_ReturnOnError(error);
	
	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(35);
	}

	// Sound
	error = ONiLevel_SetDefaultReverb(inLevelNum);
	UUmError_ReturnOnError(error);
	
	error = OSrLevel_Load(inLevelNum);
	UUmError_ReturnOnError(error);
	
	// particles must be after AKrLevel_Begin due to decal creation against the environment
	error = ONrLevel_ParticleLoad();
	UUmError_ReturnOnError(error);

	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(40);
	}

	// Weapons (call before charcters)
	error = WPrLevel_Begin();
	UUmError_ReturnOnError(error);

	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(50);
	}

	// Objects (before characters)
	error = ONrGameState_LevelBegin_Objects();
	UUmError_ReturnOnError(error);

	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(55);
	}

	// environmental particles
	error = EPrArray_New(ONgLevel->envParticleArray, EPcParticleOwner_StaticEnv, NULL, NULL, 0, NULL);
	UUmError_ReturnOnError(error);

	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(62);
	}

	// Doors (after objects)
	error = OBrDoor_Array_LevelBegin(&ONgGameState->doors,ONgGameState->level->doorArray);
	UUmError_ReturnOnErrorMsg(error, "Door init failed");	

	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(65);
	}

	// Set up the camera for a level
	error = CArLevelBegin(/*ONgGameState->local.playerCharacter*/);
	UUmError_ReturnOnError(error);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Init objects (must come before characters)
	error = OBJrObject_LevelBegin();
	UUmError_ReturnOnError(error);

	// Pathfinding - must come after doors have been loaded and initialized
	error = PHrBuildGraph(&ONgGameState->local.pathGraph, ONgGameState->level->environment);
	UUmError_ReturnOnError(error);

	// set up furniture particles
	error = OBJrFurniture_CreateParticles();
	UUmError_ReturnOnError(error);

	// initialize the AI 
	error = AIrLevelBegin();
	UUmError_ReturnOnError(error);
	
	// Initialize the script system - this must come before the AI2 system is initialized
	// or else spawn scripts will not function on characters
	error = ONrScript_LevelBegin(ONgGameState->level->name);
	UUmError_ReturnOnError(error);

	// initialize the new AI system:
	// creates characters from OBJcType_Character objects
	error = AI2rLevelBegin();
	UUmError_ReturnOnError(error);	

	// make sure that we have a player character by now. if there has not been one made
	// by the AI2 system, creates character 0 as player character.
	player_character = ONrGameState_GetPlayerCharacter();
	if (player_character == NULL) {
		error = AIrCreatePlayerFromTextFile();
		UUmError_ReturnOnError(error);

		player_character = ONrGameState_GetPlayerCharacter();
	}

	// initialize the camera
	CAgCamera.star = player_character;
	CArFollow_Enter();

	// Sky (after characters)
	error = ONrSky_LevelBegin( &ONgGameState->sky, ONgLevel->sky_class );
	UUmError_ReturnOnError(error);

	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(70);
	}

	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(75);
	}

	// set the input mode to game
	LIrMode_Set(LIcMode_Game);
	
	// enable the console display
	COrConsole_TemporaryDisable(UUcFalse);
	COrMessage_TemporaryDisable(UUcFalse);

#if TOOL_VERSION
	// Run the console config file	
	if (COcConfigFile_Read == ONgCommandLine.readConfigFile) COrRunConfigFile("oni_config.txt");
#endif
		
	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(85);
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	// initialize level mechanics
	error = ONrLevel_InitializeActionMarkerArray();
	UUmError_ReturnOnError(error);

	error = ONrMechanics_LevelBegin( );
	UUmError_ReturnOnError(error);
	
	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(87);
	}

	error = ONrInGameUI_LevelLoad(inLevelNum);
	UUmError_ReturnOnError(error);
	
	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(90);
	}

	ONrLevel_Preload_Textures();

	if (inProgressBar) {
		ONrLevel_LevelLoadDialog_Update(100);
	}

	// hide level load dialog
	ONiLevel_LevelLoadDialog_Stop();
	
	// save the loaded level number
	ONgCurrentLevel = inLevelNum;

	// run the main script:
	// creates legacy characters from script IDs which are used to
	// find appropriate AItCharacterSetup instances.

	error = SLrScript_ExecuteOnce("main", 0, NULL, NULL, NULL);
	
	ONrGameState_SplashScreen("intro_splash_screen", NULL, UUcFalse);

	// enable all sound
	OSrSetScriptOnly(UUcFalse);

	return UUcError_None;
}

static UUtBool
ONiFlagsExist(
	OBJtObject				*inObject,
	UUtUns32				inUserData)
{
	UUtBool					*found;
	
	found = (UUtBool*)inUserData;
	*found = UUcTrue;
	
	return UUcFalse;
}

static UUtError
ONiFlagArray_TemplateHandler(
	TMtTemplateProc_Message	inMessage,
	void*					inDataPtr,
	void*					inPrivateData)
{
	ONtFlagArray			*flag_array;
	
	flag_array = (ONtFlagArray*)inDataPtr;
	UUmAssert(flag_array);
	
	switch(inMessage)
	{
		case TMcTemplateProcMessage_Update:
		case TMcTemplateProcMessage_DisposePreProcess:
		case TMcTemplateProcMessage_NewPostProcess:		
		break;
		
		case TMcTemplateProcMessage_LoadPostProcess:
		{
			UUtUns32		i;
			UUtBool			flags_exist;
			UUtError		error;
			OBJtOSD_All		osd_all;
			
			// don't create any flags from the flag array if
			// any flags have already been created
			flags_exist = UUcFalse;
			OBJrObjectType_EnumerateObjects(
				OBJcType_Flag,
				ONiFlagsExist,
				(UUtUns32)&flags_exist); 
			if (flags_exist) { break; }
			
			// set up default flag properties
			error = OBJrObject_OSD_SetDefaults(OBJcType_Flag, &osd_all);
			UUmError_ReturnOnError(error);

			// create objects for each flag in the flag_array
			for (i = 0; i < flag_array->curNumFlags; i++)
			{
				M3tPoint3D		rotation;
				ONtFlag			*flag;
				
				flag = &flag_array->flags[i];
				
				osd_all.osd.flag_osd.shade = IMcShade_Green;
				osd_all.osd.flag_osd.id_prefix = 'MX';
				osd_all.osd.flag_osd.id_number = flag->idNumber;
#if TOOL_VERSION
				osd_all.osd.flag_osd.note[0] = '\0';
#endif
				
				rotation.x = 0.0f;
				rotation.y = flag->rotation * M3cRadToDeg; 
				rotation.z = 0.0f;
				
				OBJrObject_New(
					OBJcType_Flag,
					&flag->location,
					&rotation,
					&osd_all);
			}
		}
		break;	
		
		default:
			UUmAssert(!"Illegal message");
		break;
	}

	return UUcError_None;
}

static UUtBool
ONiLevel_Flag_GetByLocation(
	OBJtObject					*inObject,
	UUtUns32					inUserData)
{
	ONtLevel_FindLocation		*find_loc;
	float						distance_squared;
	
	find_loc = (ONtLevel_FindLocation*)inUserData;
	
	distance_squared = MUrPoint_Distance_Squared(&inObject->position, &find_loc->position);
	if (distance_squared < find_loc->closest_distance)
	{
		find_loc->closest_distance = distance_squared;
		find_loc->closest_object = inObject;
	}
	
	return UUcTrue;
}

UUtBool
ONrLevel_Flag_ID_To_Location(
	UUtInt16					inID,
	M3tPoint3D					*outLocation)
{
	OBJtOSD_All					osd_all;
	OBJtObject					*object;
	
	// set the search params
	osd_all.osd.flag_osd.id_number = inID;
	
	// find the object
	object = OBJrObjectType_Search(OBJcType_Flag, OBJcSearch_FlagID, &osd_all);
	if (object == NULL) { return UUcFalse; }
	
	// return the object location
	OBJrObject_GetPosition(object, outLocation, NULL);
	
	return UUcTrue;
}

UUtBool ONrLevel_Path_ID_To_Path(UUtInt16 inID, OBJtOSD_PatrolPath *outPath)
{
	OBJtOSD_All					osd_all;
	OBJtObject					*object;
	
	// set the search params
	osd_all.osd.patrolpath_osd.id_number = inID;
	
	// find the object
	object = OBJrObjectType_Search(OBJcType_PatrolPath, OBJcSearch_PatrolPathID, &osd_all);
	if (object == NULL) { return UUcFalse; }

	OBJrObject_GetObjectSpecificData(object, &osd_all);
	
	// build an ONtFlag 
	*outPath = osd_all.osd.patrolpath_osd;
	
	return UUcTrue;
}

UUtBool ONrLevel_Combat_ID_To_Combat(UUtUns16 inID, OBJtOSD_Combat *outCombat)
{
	OBJtOSD_All					osd_all;
	OBJtObject					*object;
	
	// set the search params
	osd_all.osd.combat_osd.id = inID;
	
	// find the object
	object = OBJrObjectType_Search(OBJcType_Combat, OBJcSearch_CombatID, &osd_all);
	if (object == NULL) { return UUcFalse; }

	OBJrObject_GetObjectSpecificData(object, &osd_all);
	
	// store the combat settings
	*outCombat = osd_all.osd.combat_osd;
	
	return UUcTrue;
}

// NB: this function returns a pointer to the melee profile rather than a copy of
// the OSD which all the other XXX_ID_To_XXX functions do. this is because the
// designers want to be able to change the melee behavior of characters on the fly,
// and for various reasons this wasn't appropriate for other kinds of data.
UUtBool ONrLevel_Melee_ID_To_Melee_Ptr(UUtUns16 inID, OBJtOSD_Melee **outMelee)
{
	OBJtOSD_All					osd_all;
	OBJtObject					*object;
	
	// find the object with this ID
	osd_all.osd.melee_osd.id = inID;
	object = OBJrObjectType_Search(OBJcType_Melee, OBJcSearch_MeleeID, &osd_all);

	if (object == NULL) {
		return UUcFalse;
	} else {
		*outMelee = (OBJtOSD_Melee *) object->object_data;
		return UUcTrue;
	}
}

UUtBool ONrLevel_Neutral_ID_To_Neutral(UUtUns16 inID, OBJtOSD_Neutral *outNeutral)
{
	OBJtOSD_All					osd_all;
	OBJtObject					*object;
	
	// set the search params
	osd_all.osd.neutral_osd.id = inID;
	
	// find the object
	object = OBJrObjectType_Search(OBJcType_Neutral, OBJcSearch_NeutralID, &osd_all);
	if (object == NULL) { return UUcFalse; }

	OBJrObject_GetObjectSpecificData(object, &osd_all);
	
	// store the neutral behavior 
	*outNeutral = osd_all.osd.neutral_osd;
	
	return UUcTrue;
}

UUtBool
ONrLevel_Flag_ID_To_Flag(
	UUtInt16					inID,
	ONtFlag						*outFlag)
{
	OBJtOSD_All					osd_all;
	OBJtObject					*object;
	M3tPoint3D					z_axis;
	
	// set the search params
	osd_all.osd.flag_osd.id_number = inID;
	
	// find the object
	object = OBJrObjectType_Search(OBJcType_Flag, OBJcSearch_FlagID, &osd_all);
	if (object == NULL) { return UUcFalse; }
	
	// build an ONtFlag 
	OBJrObject_GetObjectSpecificData(object, &osd_all);
	
	// CB: to make sure that the flag's *actual* rotation is drawn we don't just use
	// the euler Y angle any more, but rather work out where the rotation matrix takes
	// the +Z axis to.
	OBJrObject_GetPosition(object, &outFlag->location, NULL);
	MUrMatrix_GetCol(&osd_all.osd.flag_osd.rotation_matrix, 2, &z_axis);
	outFlag->rotation	= MUrATan2(z_axis.x, z_axis.z);
	UUmTrig_ClipLow(outFlag->rotation);

	outFlag->idNumber	= osd_all.osd.flag_osd.id_number;
	outFlag->deleted	= UUcFalse;
	outFlag->maxFlag	= UUcFalse;
	
	return UUcTrue;
}

void
ONrLevel_Flag_GetVector(
	ONtFlag						*inFlag,
	M3tVector3D					*outVector)
{
	const M3tPoint3D defFacing = {0,0,1.0f};
	
	UUmAssertReadPtr(inFlag, sizeof(*inFlag));
	UUmAssertWritePtr(outVector, sizeof(*outVector));
	
	MUrPoint_RotateYAxis(&defFacing, inFlag->rotation, outVector);
	MUrNormalize(outVector);
}

const AItCharacterSetup *ONrLevel_CharacterSetup_ID_To_Pointer(UUtInt16 inID)
{
	AItCharacterSetupArray *characterSetupArray = ONgLevel->characterSetupArray;
	UUtUns16 itr;

	for(itr = 0; itr < characterSetupArray->numCharacterSetups; itr++) {
		const AItCharacterSetup *curSetup = characterSetupArray->characterSetups + itr;

		if (curSetup->defaultScriptID == inID) {
			return curSetup;
		}
	}

	return NULL;
}

UUtBool
ONrLevel_Flag_Location_To_Flag(
	const M3tPoint3D			*inLocation,
	ONtFlag						*outFlag)
{
	ONtLevel_FindLocation		find_loc;
	OBJtOSD_All					osd_all;
	
	UUrMemory_Clear(outFlag, sizeof(ONtFlag));
	
	// setup the find_id struct
	find_loc.closest_object = NULL;
	find_loc.closest_distance = UUcFloat_Max;
	find_loc.position = *inLocation;
	
	// enumerate the objects
	OBJrObjectType_EnumerateObjects(OBJcType_Flag, ONiLevel_Flag_GetByLocation, (UUtUns32)&find_loc);
	
	// if no object was found, return NULL
	if (find_loc.closest_object == NULL) { return UUcFalse; }
	
	OBJrObject_GetObjectSpecificData(find_loc.closest_object, &osd_all);
	
	outFlag->location	= find_loc.closest_object->position;
	outFlag->rotation	= (find_loc.closest_object->rotation.y * M3cDegToRad);
	outFlag->idNumber	= osd_all.osd.flag_osd.id_number;
	outFlag->deleted	= UUcFalse;
	outFlag->maxFlag	= UUcFalse;
	
	return UUcTrue;
}

void
ONrLevel_Flag_Delete(
	UUtUns16 inID)
{
	OBJtOSD_All					osd_all;
	OBJtObject					*object;
	
	// set the search params
	osd_all.osd.flag_osd.id_number = inID;
	
	// find the object
	object = OBJrObjectType_Search(OBJcType_Flag, OBJcSearch_FlagID, &osd_all);
	if (object == NULL) { return; }
	
	OBJrObject_Delete(object);
}

ONtActionMarker* ONrLevel_ActionMarker_New( void)
{
	ONtActionMarkerArray	*array;
	ONtActionMarker			*marker = NULL;

	UUmAssert(ONgLevel != NULL);

	array = &ONgLevel->actionMarkerArray;

	if (array->num_markers < ONcMaxActionMarkers)
	{
		marker = &array->markers[array->num_markers];
		marker->position.x = 0;
		marker->position.y = 0;
		marker->position.z = 0;
		marker->direction = 0;
		marker->object = 0;
		array->num_markers++;
	}
	return marker;
}

ONtActionMarker* ONrLevel_ActionMarker_FindNearest(const M3tPoint3D *inLocation)
{
	ONtActionMarkerArray		*array;
	ONtActionMarker				*marker, *closest_marker;
	float						distance, closest_distance;
	UUtUns32					i;

	UUmAssert(ONgLevel != NULL);

	array = &ONgLevel->actionMarkerArray;

	closest_distance = UUcFloat_Max;
	closest_marker = NULL;

	for( i = 0; i < array->num_markers; i++ )
	{
		marker		= &array->markers[i];

		distance	= MUrPoint_Distance_Squared( &marker->position, inLocation );
		if (distance < closest_distance)
		{
			closest_distance = distance;
			closest_marker = marker;
		}
	}

	return closest_marker;
}

UUtBool ONrLevel_FindObjectTag( UUtUns32 inObjectTag )
{
	UUtUns32		i;
	UUtUns32		num_objects;

	UUmAssert(ONgLevel != NULL);

	if (NULL == ONgLevel->object_gunk_array)
		return UUcFalse;

	num_objects = ONgLevel->object_gunk_array->num_objects;

	for( i = 0; i < num_objects; i++ )
	{
		if( ONgLevel->object_gunk_array->objects[i].object_tag == inObjectTag )
			return UUcTrue;
	}
	return UUcFalse;
}

ONtObjectGunk* ONrLevel_FindObjectGunk( UUtUns32 inObjectTag )
{
	UUtUns32		i;
	UUtUns32		num_objects;

	UUmAssert(ONgLevel != NULL);

	if (NULL == ONgLevel->object_gunk_array) {
		return NULL;
	}

	num_objects = ONgLevel->object_gunk_array->num_objects;

	for( i = 0; i < num_objects; i++ )
	{
		if( ONgLevel->object_gunk_array->objects[i].object_tag == inObjectTag ) {
			return &ONgLevel->object_gunk_array->objects[i];
		}
	}

	return NULL;
}

// returns 0 if that was the last level
UUtUns16 ONrLevel_GetNextLevel(UUtUns16 inLevelNum)
{
	UUtError	error;
	UUtUns32	num_descriptors;
	UUtUns32	i;
	UUtUns16	next_level = 0xFFFF;
	
	// get the number of descriptors
	num_descriptors = TMrInstance_GetTagCount(ONcTemplate_Level_Descriptor);

	for (i = 0; i < num_descriptors; i++)
	{
		ONtLevel_Descriptor		*descriptor;
		
		// get a pointer to the first descriptor
		error =	TMrInstance_GetDataPtr_ByNumber(ONcTemplate_Level_Descriptor, i, &descriptor);
		UUmAssert(UUcError_None == error);

		if (UUcError_None == error) {
			if (descriptor->level_number > inLevelNum) {
				if (TMrLevel_Exists(descriptor->level_number)) {
					next_level = UUmMin(descriptor->level_number, next_level);
				}
			}
		}
	}

	if (0xFFFF == next_level) {
		next_level = 0;
	}

	return next_level;
}


IMtShade ONrLevel_ObjectGunk_GetShade(ONtObjectGunk	*inObjectGunk)
{
	UUtUns32 min_component = 0x3f;
	AKtEnvironment *environment = ONgLevel->environment;
	UUtUns32 itr;
	UUtUns32 count = inObjectGunk->gq_array->numIndices;
	UUtUns32 *indices = inObjectGunk->gq_array->indices;
	AKtGQ_General *gq_general_array = ONgGameState->level->environment->gqGeneralArray->gqGeneral;
	UUtUns32 r = 0;
	UUtUns32 g = 0;
	UUtUns32 b = 0;
	UUtUns32 valid_count = 0;
	IMtShade shade;

	if (0 == count) {
		shade = 0xFFFFFF;

		goto exit;
	}

	for(itr = 0; itr < count; itr++)
	{
		UUtUns32 index = indices[itr];
		AKtGQ_General *gq_general = gq_general_array + index;
		UUtUns32 vertex_count = (gq_general->flags & AKcGQ_Flag_Triangle) ? 3 : 4;
		UUtUns32 vertex_itr;

		for(vertex_itr = 0; vertex_itr < vertex_count; vertex_itr++)
		{
			UUtUns32 vertex_shade = gq_general->m3Quad.shades[vertex_itr];
			UUtUns32 this_r = (vertex_shade >> 16) & 0xFF;
			UUtUns32 this_g = (vertex_shade >> 8) & 0xFF;
			UUtUns32 this_b = (vertex_shade >> 0) & 0xFF;

			if ((this_r > min_component) | (this_g > min_component) | (this_b > min_component)) {
				r += this_r;
				g += this_g;
				b += this_b;

				valid_count++;
			}
		}
	}

	if (valid_count > 0) {
		r += (valid_count / 2);
		g += (valid_count / 2);
		b += (valid_count / 2);

		r /= valid_count;
		g /= valid_count;
		b /= valid_count;
	}
	else {
		r = min_component;
		g = min_component;
		b = min_component;
	}


	UUmAssert(r <= 0xFF);
	UUmAssert(g <= 0xFF);
	UUmAssert(b <= 0xFF);

	shade = (0xFF << 24) | (r << 16) | (g << 8) | (b << 0);

exit:
	return shade;
}

void ONrLevel_ObjectGunk_SetShade(ONtObjectGunk *inObjectGunk, IMtShade inShade)
{
	AKtEnvironment *environment = ONgLevel->environment;
	UUtUns32 itr;
	UUtUns32 count = inObjectGunk->gq_array->numIndices;
	UUtUns32 *indices = inObjectGunk->gq_array->indices;
	AKtGQ_General *gq_general_array = ONgGameState->level->environment->gqGeneralArray->gqGeneral;

	for(itr = 0; itr < count; itr++)
	{
		UUtUns32 index = indices[itr];
		AKtGQ_General *gq_general = gq_general_array + index;
		UUtUns32 vertex_count = (gq_general->flags & AKcGQ_Flag_Triangle) ? 3 : 4;
		UUtUns32 vertex_itr;

		for(vertex_itr = 0; vertex_itr < vertex_count; vertex_itr++)
		{
			gq_general->m3Quad.shades[vertex_itr] = inShade;
		}
	}

	return;
}

static void ONrLevel_Preload_TextureMap_Array(M3tTextureMapArray *inArray)
{
	if (NULL != inArray) {
		M3tTextureMap **texture_map_list = inArray->maps;
		UUtUns32 count = inArray->numMaps;
		UUtUns32 itr;

		for(itr = 0; itr < count; itr++)
		{
			M3tTextureMap *texture_map = texture_map_list[itr];

			M3rDraw_Texture_EnsureLoaded(texture_map);
		}
	}

	return;
}

static void ONrLevel_Preload_Textures_Environment(void)
{
	AKtEnvironment *environment = ONgLevel->environment;

	if (NULL == environment) {
		COrConsole_Printf("failed to preload environment textures");
		goto exit;
	}

	ONrLevel_Preload_TextureMap_Array(environment->textureMapArray);

exit:
	return;
}

static void ONrLevel_Preload_Texture_OneBody(TRtBodyTextures *inBodyTextures)
{
	if (NULL != inBodyTextures) {
		M3tTextureMap **texture_map_list = inBodyTextures->maps;
		UUtUns32 count = inBodyTextures->numMaps;
		UUtUns32 itr;

		for(itr = 0; itr < count; itr++)
		{
			M3tTextureMap *texture_map = texture_map_list[itr];

			M3rDraw_Texture_EnsureLoaded(texture_map);
		}
	}

	return;
}

static void ONrLevel_Preload_Textures_Characters(void)
{
	UUtUns32 itr;
	UUtUns32 count = TMrInstance_GetTagCount(TRcTemplate_BodyTextures);

	for(itr = 0; itr < count; itr++)
	{
		TRtBodyTextures *body_textures = NULL;

		TMrInstance_GetDataPtr_ByNumber(TRcTemplate_BodyTextures, itr, &body_textures);

		ONrLevel_Preload_Texture_OneBody(body_textures);
	}

	return;
}

void ONrLevel_Preload_Textures(void)
{
	ONrLevel_Preload_Textures_Environment();
	ONrLevel_Preload_Textures_Characters();

	// precache all placed particles' textures (those on environment, furniture, objects)
	EPrPrecacheParticles();

	return;
}