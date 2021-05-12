/*
	Imp_AI_Script.h
	
	Script level AI stuff
	
	Author: Quinn Dunki
	c1998 Bungie
*/

#include "Imp_Env2_Private.h"

#define IMPcRemapFilePrefix "ScriptQuadRemaps_lvl_"
#define IMPcMaxQuadRemaps 4096

extern UUtUns16	IMPgAI_QuadRefRemap[2][IMPcMaxQuadRemaps];
extern UUtUns16	IMPgAI_QuadRemapCount;

UUtError
Imp_AddAIScript(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError Imp_AddAIScriptTable(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
void IMPrScript_AddQuadRemap(
	UUtUns32 inQuadIndex,
	UUtUns32 inRefNumber,
	GRtGroup *inEnvGroup);
	
UUtError IMPrScript_GetRemapData(
	UUtUns16 *outCount);

void IMPrScript_UnzipRemapArray(
	UUtUns32 *outRemapIndices,
	UUtUns32 *outRemaps);
