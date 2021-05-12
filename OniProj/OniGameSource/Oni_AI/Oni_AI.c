/*
	Oni_AI.c
	
	Top level AI stuff
	
	Author: Quinn Dunki
	c1998 Bungie
*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Path.h"
#include "BFW_Image.h"
#include "BFW_AppUtilities.h"
#include "BFW_TextSystem.h"
#include "BFW_ScriptLang.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_AI.h"
#include "Oni_Astar.h"
#include "Oni_AI_Setup.h"
#include "Oni_AI_Script.h"

#include "Oni.h"
#include "Oni_Combos.h"
#include "Oni_Path.h"
#include "Oni_Film.h"
#include "Oni_Level.h"
#include "Oni_Character_Animation.h"


UUtError AIrRegisterTemplates(
	void)
{
	/**************
	* Register templates
	*/
	
	UUtError error;
		
	error = TMrTemplate_Register(AIcTemplate_WaypointArray,sizeof(AItWaypointArray),TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(AIcTemplate_CharacterSetupArray,sizeof(AItCharacterSetupArray),TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	error = TMrTemplate_Register(NMcTemplate_SpawnPointArray,sizeof(NMtSpawnPointArray),TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	
	error = TMrTemplate_Register(AIcTemplate_ScriptTriggerClassArray,sizeof(AItScriptTriggerClassArray),TMcFolding_Allow);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}


UUtError AIrLevelBegin(
	void)
{
	UUtError error;

	error = AIrScript_LevelBegin_Triggers();
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

void AIrLevelEnd(
	void)
{
	AIrScript_LevelEnd_Triggers();
}

