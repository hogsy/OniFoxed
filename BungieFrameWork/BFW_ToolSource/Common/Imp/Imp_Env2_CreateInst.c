/*
	FILE:	Imp_Env_CreateInst.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 3, 1997

	PURPOSE: 
	
	Copyright 1997

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_Mathlib.h"
#include "BFW_Shared_TriRaster.h"
#include "BFW_Timer.h"

#include "Imp_Common.h"
#include "Imp_Env2_Private.h"
#include "Imp_AI_Script.h"
#include "Oni_GameStatePrivate.h"

#include <ctype.h>
#include <math.h>

extern UUtUns32 IMPgEnv_VerifyAdjCount;
extern float	gOctTreeMinDim;
extern UUtBool	IMPgBNVDebug;
extern UUtBool IMPgGreen;

#define IMPcTextureSpeed 30
#define IMPcDynamicSetups 32

static void FixedTree_Loop(float min, float max, float fixed_min, float fixed_max, UUtUns32 *out_base, UUtUns32 *out_max)
{
	UUtUns32 result_max;
	UUtUns32 result_base;
	float one_over_delta = ((float) IMPcFixedOctTreeNode_Count) / (fixed_max - fixed_min);

	// CB: in an attempt to handle floating-point precision errors causing assertions, I added
	// these float comparisons and reworded the error cases slightly.
	//
	// there was also another problem with BNV volumes extending outside inBuildData->min/max
	// (which I presume is computed from GQ vertex positions) that caused an assertion to fire.
	if (UUmFloat_CompareLE(min, fixed_min)) {
		result_base = 0;
	} else if (UUmFloat_CompareGE(min, fixed_max)) {
		result_base = IMPcFixedOctTreeNode_Count - 1;
	} else {
		result_base = MUrFloat_Round_To_Int((float)floor((min - fixed_min) * one_over_delta));
	}

	if (UUmFloat_CompareGE(max, fixed_max)) {
		result_max = IMPcFixedOctTreeNode_Count;
	} else if (UUmFloat_CompareLE(max, fixed_min)) {
		result_max = 1;
	} else {
		result_max = IMPcFixedOctTreeNode_Count - MUrFloat_Round_To_Int((float)floor((fixed_max - max) * one_over_delta));
	}

	if (result_base >= IMPcFixedOctTreeNode_Count) { result_base = IMPcFixedOctTreeNode_Count - 1; }
	if (result_max <= result_base) { result_max = result_base + 1; }

	UUmAssert(result_max > result_base);
	UUmAssert(result_max <= IMPcFixedOctTreeNode_Count);

	*out_base = result_base;
	*out_max = result_max;

	return;
}							

void IMPrFixedOctTree_Delete(IMPtEnv_BuildData*	inBuildData)
{
	UUtUns32 itr_x, itr_y, itr_z;

	for(itr_x = 0; itr_x < IMPcFixedOctTreeNode_Count; itr_x++)
	{
		for(itr_y = 0; itr_y < IMPcFixedOctTreeNode_Count; itr_y++)
		{
			for(itr_z = 0; itr_z < IMPcFixedOctTreeNode_Count; itr_z++)
			{
				if (NULL != inBuildData->fixed_oct_tree_node[itr_x][itr_y][itr_z]) {
					UUrMemory_Block_Delete(inBuildData->fixed_oct_tree_node[itr_x][itr_y][itr_z]);
					inBuildData->fixed_oct_tree_node[itr_x][itr_y][itr_z] = NULL;
				}
			}
		}
	}

	return;
}

void IMPrFixedOctTree_Create(IMPtEnv_BuildData*	inBuildData)
{
	UUtUns32 stats[100] = { 0 };
	UUtUns32 itr;
	UUtUns32 itr_x, itr_y, itr_z;
	float delta_x, delta_y, delta_z;

	inBuildData->fixed_oct_tree_temp_list = AUrDict_New(inBuildData->numGQs, inBuildData->numGQs);

	inBuildData->minX = UUcFloat_Max;
	inBuildData->minY = UUcFloat_Max;
	inBuildData->minZ = UUcFloat_Max;

	inBuildData->maxX = UUcFloat_Min;
	inBuildData->maxY = UUcFloat_Min;
	inBuildData->maxZ = UUcFloat_Min;

	for(itr = 0; itr < inBuildData->numGQs; itr++)
	{
		inBuildData->minX = UUmMin(inBuildData->gqList[itr].bBox.minPoint.x, inBuildData->minX);
		inBuildData->minY = UUmMin(inBuildData->gqList[itr].bBox.minPoint.y, inBuildData->minY);
		inBuildData->minZ = UUmMin(inBuildData->gqList[itr].bBox.minPoint.z, inBuildData->minZ);

		inBuildData->maxX = UUmMax(inBuildData->gqList[itr].bBox.maxPoint.x, inBuildData->maxX);
		inBuildData->maxY = UUmMax(inBuildData->gqList[itr].bBox.maxPoint.y, inBuildData->maxY);
		inBuildData->maxZ = UUmMax(inBuildData->gqList[itr].bBox.maxPoint.z, inBuildData->maxZ);
	}

	inBuildData->sizeX = inBuildData->maxX - inBuildData->minX;
	inBuildData->sizeY = inBuildData->maxY - inBuildData->minY;
	inBuildData->sizeZ = inBuildData->maxZ - inBuildData->minZ;

	delta_x = inBuildData->sizeX / ((float) IMPcFixedOctTreeNode_Count);
	delta_y = inBuildData->sizeY / ((float) IMPcFixedOctTreeNode_Count);
	delta_z = inBuildData->sizeZ / ((float) IMPcFixedOctTreeNode_Count);

	// ok we built the extens, now we allocate the oct tre
	for(itr_x = 0; itr_x < IMPcFixedOctTreeNode_Count; itr_x++)
	{
		for(itr_y = 0; itr_y < IMPcFixedOctTreeNode_Count; itr_y++)
		{
			for(itr_z = 0; itr_z < IMPcFixedOctTreeNode_Count; itr_z++)
			{
				inBuildData->fixed_oct_tree_node[itr_x][itr_y][itr_z] = 0;
				inBuildData->fixed_oct_tree_node_count[itr_x][itr_y][itr_z] = 0;
			}
		}
	}

	for(itr = 0; itr < inBuildData->numGQs; itr++)
	{
		IMPtEnv_GQ *current_gq = inBuildData->gqList + itr;

		UUtUns32 base_x;
		UUtUns32 max_x;

		FixedTree_Loop(current_gq->bBox.minPoint.x, current_gq->bBox.maxPoint.x, inBuildData->minX, inBuildData->maxX, &base_x, &max_x);

		for(itr_x = base_x; itr_x < max_x; itr_x++)
		{
			UUtUns32 base_y;
			UUtUns32 max_y;

			FixedTree_Loop(current_gq->bBox.minPoint.y, current_gq->bBox.maxPoint.y, inBuildData->minY, inBuildData->maxY, &base_y, &max_y);

			for(itr_y = base_y; itr_y < max_y; itr_y++)
			{
				UUtUns32 base_z;
				UUtUns32 max_z;

				FixedTree_Loop(current_gq->bBox.minPoint.z, current_gq->bBox.maxPoint.z, inBuildData->minZ, inBuildData->maxZ, &base_z, &max_z);

				for(itr_z = base_z; itr_z < max_z; itr_z++)
				{
					UUtUns32 count = inBuildData->fixed_oct_tree_node_count[itr_x][itr_y][itr_z];
					UUtUns32 *list = inBuildData->fixed_oct_tree_node[itr_x][itr_y][itr_z];

					if (0 == (count % IMPcFixedOctTreeNode_ChunkSize)) {
						list = UUrMemory_Block_Realloc(list, sizeof(UUtUns32) * (count + IMPcFixedOctTreeNode_ChunkSize));
					}

					list[count++] = itr;
					
					inBuildData->fixed_oct_tree_node_count[itr_x][itr_y][itr_z] = count;
					inBuildData->fixed_oct_tree_node[itr_x][itr_y][itr_z] = list;
				}
			}
		}		
	}

	for(itr_x = 0; itr_x < IMPcFixedOctTreeNode_Count; itr_x++)
	{
		for(itr_y = 0; itr_y < IMPcFixedOctTreeNode_Count; itr_y++)
		{
			for(itr_z = 0; itr_z < IMPcFixedOctTreeNode_Count; itr_z++)
			{
				UUtUns32 *verify_list = inBuildData->fixed_oct_tree_node[itr_x][itr_y][itr_z];
				UUtUns32 verify_count = inBuildData->fixed_oct_tree_node_count[itr_x][itr_y][itr_z];

				for(itr = 0; itr < verify_count; itr++)
				{
					AUrDict_TestAndAdd(inBuildData->fixed_oct_tree_temp_list, verify_list[itr]);
				}

				if (verify_count >= 100) {
					stats[99]++;
				}
				else {
					stats[verify_count] += 1;
				}
			}
		}
	}

	UUmAssert(inBuildData->fixed_oct_tree_temp_list->numPages == inBuildData->numGQs);

	AUrDict_Clear(inBuildData->fixed_oct_tree_temp_list);

	return;
}

void IMPrFixedOctTree_Test(IMPtEnv_BuildData *inBuildData, float minx, float maxx, float miny, float maxy, float minz, float maxz)
{
	UUtUns32 itr_x;

	UUtUns32 base_x;
	UUtUns32 max_x;

	UUtUns32 base_y;
	UUtUns32 max_y;

	UUtUns32 base_z;
	UUtUns32 max_z;

	AUrDict_Clear(inBuildData->fixed_oct_tree_temp_list);

	// clip to the limits
	minx = UUmMax(minx, inBuildData->minX);
	maxx = UUmMin(maxx, inBuildData->maxX);
	miny = UUmMax(miny, inBuildData->minY);
	maxy = UUmMin(maxy, inBuildData->maxY);
	minz = UUmMax(minz, inBuildData->minZ);
	maxz = UUmMin(maxz, inBuildData->maxZ);

	FixedTree_Loop(minx, maxx, inBuildData->minX, inBuildData->maxX, &base_x, &max_x);
	FixedTree_Loop(miny, maxy, inBuildData->minY, inBuildData->maxY, &base_y, &max_y);
	FixedTree_Loop(minz, maxz, inBuildData->minZ, inBuildData->maxZ, &base_z, &max_z);

	for(itr_x = base_x; itr_x < max_x; itr_x++)
	{
		UUtUns32 itr_y;

		for(itr_y = base_y; itr_y < max_y; itr_y++)
		{
			UUtUns32 itr_z;

			for(itr_z = base_z; itr_z < max_z; itr_z++)
			{
				UUtUns32 itr;
				UUtUns32 count = inBuildData->fixed_oct_tree_node_count[itr_x][itr_y][itr_z];
				UUtUns32 *list = inBuildData->fixed_oct_tree_node[itr_x][itr_y][itr_z];

				for(itr = 0; itr < count; itr++)
				{
					AUrDict_TestAndAdd(inBuildData->fixed_oct_tree_temp_list, list[itr]);
				}				
			}
		}
	}

	return;	
}



UUtError
IMPrEnv_CreateInstance_EnvParticles(
	UUtUns32		inNumEnvParticles,
	EPtEnvParticle*	inEnvParticles,
	EPtEnvParticleArray*	*outTemplate);
	
UUtError
IMPrEnv_CreateInstance_EnvParticles(
	UUtUns32		inNumEnvParticles,
	EPtEnvParticle*	inEnvParticles,
	EPtEnvParticleArray*	*outTemplate)
{
	UUtError	error;
	
	if(inNumEnvParticles == 0)
	{
		*outTemplate = NULL;
		return UUcError_None;
	}
	
	error =
		TMrConstruction_Instance_NewUnique(
			EPcTemplate_EnvParticleArray,
			inNumEnvParticles,
			outTemplate);
	UUmError_ReturnOnError(error);
	
	UUrMemory_MoveFast(
		inEnvParticles,
		(*outTemplate)->particle,
		inNumEnvParticles * sizeof(EPtEnvParticle));
	
	return UUcError_None;
}

static void
IMPiEnv_Verify_Pathfinding(
	IMPtEnv_BuildData*	inBuildData)
{
	IMPtEnv_GQ *curGQ;
	UUtUns32 curIndex;
	UUtUns32 curSideIndex;
	IMPtEnv_BNV *bnv;
	UUtUns32 adj=0;
	FILE *out;

	// Run some debugging checks on pathfinding
	for (curIndex=0; curIndex<inBuildData->numGQs; curIndex++)
	{
		curGQ = &inBuildData->gqList[curIndex];
		
		if (!(curGQ->flags & AKcGQ_Flag_Ghost)) continue;
		
		if (!curGQ->ghostUsed) {
			M3tPoint3D *pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
			M3tPoint3D midpoint;
			UUtUns32 i, num_p = (curGQ->flags & AKcGQ_Flag_Triangle) ? 3 : 4;

			MUmVector_Set(midpoint, 0, 0, 0);
			for (i = 0; i < num_p; i++) {
				MUmVector_Add(midpoint, midpoint, pointArray[curGQ->visibleQuad.indices[i]]);
			}
			MUmVector_Scale(midpoint, 1.0f / num_p);

			IMPrEnv_LogError("Error: Ghost quad at (%f %f %f) [GQ #%d, obj %s, file %s] does not connect two BNVs",
							midpoint.x, midpoint.y, midpoint.z, curIndex,curGQ->objName,curGQ->fileName);
		}
	}

	for (curIndex=0; curIndex<inBuildData->numBNVs; curIndex++)
	{
		bnv = &inBuildData->bnvList[curIndex];

		if (!(bnv->flags & AKcBNV_Flag_Room)) {
			IMPrEnv_LogError("Warning: BNV %s in file %s is not a room",bnv->objName,bnv->fileName);
		}
		
		for (curSideIndex=0; curSideIndex<bnv->numSides; curSideIndex++)
		{	
			adj += bnv->sides[curSideIndex].numAdjacencies;
		}
		
		if (!adj && !(bnv->flags & AKcBNV_Flag_NonAI)) {
			IMPrEnv_LogError("Warning: BNV %s in file %s has no ghost GQs",bnv->objName,bnv->fileName);
		}
	}
	
	if (IMPgBNVDebug)
	{
		out = fopen("bnvdebug.txt","wb+");
		for (curIndex=0; curIndex<inBuildData->numBNVs; curIndex++)
		{
			bnv = &inBuildData->bnvList[curIndex];
			fprintf(out,"#%d: %s in %s. V3=(%3.4f) Origin = (%3.2f,%3.2f,%3.2f)"UUmNL,
				curIndex,bnv->objName,bnv->fileName,bnv->volume,bnv->origin.x,bnv->origin.y,bnv->origin.z);
		}
		fclose(out);
	}
}


static UUtError
IMPiEnv_CreateInstance_CombatProfiles(IMPtEnv_BuildData *inBuildData, AKtEnvironment *environment)
{
	// Create the combat profiles in Akira (obsolete)
	return UUcError_None;
}

static UUtError IMPiEnv_CreateInstance_TacticalProfiles(IMPtEnv_BuildData *inBuildData, AKtEnvironment *environment)
{
	// Create the tactical profiles in Akira (obsolete)	
	return UUcError_None;
}


static UUtError
IMPiEnv_CreateInstance_Markers(IMPtEnv_BuildData *inBuildData, ONtLevel *level)
{
	UUtUns32 markerIndex;
	MXtMarker *marker;
	ONtMarker *gameMarker;
	UUtError error;
	
	// Create marker array
	error = TMrConstruction_Instance_NewUnique(
		ONcTemplate_MarkerArray,
		inBuildData->numMarkers,
		&level->markerArray);
	UUmError_ReturnOnError(error);

	// Fill in data
	for (markerIndex=0; markerIndex<inBuildData->numMarkers; markerIndex++)
	{
		marker = inBuildData->markerList + markerIndex;
		gameMarker = level->markerArray->markers + markerIndex;
		UUrMemory_Clear(gameMarker,sizeof(ONtMarker));

		// Name and position
		MUrMatrix_MultiplyPoint(&gameMarker->position,&marker->matrix,&gameMarker->position);
		UUrString_Copy(gameMarker->name,marker->name,ONcMaxMarkerName);

		// Direction
		gameMarker->direction.y = 1.0f;
		MUrMatrix_MultiplyPoint(
			&gameMarker->direction,
			&marker->matrix,
			&gameMarker->direction);
		MUmVector_Decrement(gameMarker->direction,gameMarker->position);
		MUrNormalize(&gameMarker->direction);
	}
	
	return UUcError_None;
}

static UUtError
IMPiEnv_CreateInstance_Objects(IMPtEnv_BuildData *inBuildData, ONtLevel *level)
{
	UUtUns32			i, curIndex;
	IMPtEnv_Object		*curObject;
	OBtObjectSetup		*curAkiraObject;
	UUtError			error;
	AKtEnvironment		*environment = level->environment;
	
	// Create the starting object array in Akira
	error =  TMrConstruction_Instance_NewUnique( OBcTemplate_ObjectArray, inBuildData->numObjects + IMPcDynamicSetups, &level->objectSetupArray);
	UUmError_ReturnOnError(error);
			
	for (curIndex=0; curIndex < level->objectSetupArray->numObjects; curIndex++)
	{
		curObject				= &inBuildData->objectList[curIndex];
		curAkiraObject			= &level->objectSetupArray->objects[curIndex];

		UUrMemory_Clear(curAkiraObject,sizeof(OBtObjectSetup));
		
		if (curIndex >= inBuildData->numObjects) continue;	// Dynamic obj setups are just cleared

		// Copy over the basics
		curAkiraObject->flags			= curObject->flags | OBcFlags_InUse;
		curAkiraObject->position		= curObject->position;
		curAkiraObject->orientation		= curObject->orientation;
		curAkiraObject->physicsLevel	= curObject->physicsLevel;
		curAkiraObject->scriptID		= curObject->scriptID;
		curAkiraObject->scale			= curObject->scale;
		curAkiraObject->animation		= curObject->animation;
		curAkiraObject->doorScriptID	= curObject->door_id;
		curAkiraObject->doorGhostIndex	= curObject->door_gq;

		UUrMemory_MoveFast(&curObject->debugOrigMatrix,&curAkiraObject->debugOrigMatrix,sizeof(M3tMatrix4x3));
		
		// Store debugging names
		UUrString_Copy(curAkiraObject->fileName,	curObject->fileName,	OBcMaxObjectName);
		UUrString_Copy(curAkiraObject->objName,		curObject->objName,		OBcMaxObjectName);
		
		// Create the array of geometry
		error = TMrConstruction_Instance_NewUnique( M3cTemplate_GeometryArray, curObject->geometry_count, &curAkiraObject->geometry_array );
		UUmError_ReturnOnError(error);		
		for (i = 0; i < curObject->geometry_count; i++)
		{
			curAkiraObject->geometry_array->geometries[i] = curObject->geometry_list[i];
		}
						
		// Create the initial environmental particle array
		IMPrEnv_CreateInstance_EnvParticles(curObject->numParticles, curObject->particles, &curAkiraObject->particleArray);
	}	
	return UUcError_None;
}

static void InverseMaxMatrix(M3tMatrix4x3 *ioMatrix)
{
	M3tMatrix4x3 matrix;

	UUrMemory_Clear(&matrix, sizeof(matrix));

	MUrMatrix_Identity(&matrix);

	MUrMatrix_RotateX(&matrix, 0.5f * M3cPi);
	MUrMatrix_RotateY(&matrix, 0.f * M3cPi);
	MUrMatrix_RotateZ(&matrix, 0.f * M3cPi);

	MUrMatrixStack_Matrix(ioMatrix, &matrix);

	return;
}

static void iParseFlagFileFlags(BFtFileRef *inFileRef, ONtFlag **outFlags, UUtUns16 *outFlagFileCount)
{
	UUtError error;
	ONtFlag *flagList;
	UUtUns16 flagCount;
	UUtInt32 version;
	UUtUns16 itr;
	char *curLine;

	BFtTextFile *textFile = NULL;

	error = BFrTextFile_OpenForRead(inFileRef, &textFile);

	if (error != UUcError_None) {
		Imp_PrintWarning("failed to open %s", BFrFileRef_GetLeafName(inFileRef));
		
		goto exit;
	}

	curLine = BFrTextFile_GetNextStr(textFile);

	if (!UUmString_IsEqual(curLine, "flag_file")) {
		Imp_PrintWarning("invalid flag file");
		goto exit;
	}

	version = BFrTextFile_GetUUtInt32(textFile);

	flagCount = BFrTextFile_GetUUtUns16(textFile);
	flagList = UUrMemory_Block_NewClear(sizeof(ONtFlag) * flagCount);

	if (NULL == flagList) { 
		Imp_PrintWarning("out of memory");
		goto exit;
	}

	for(itr = 0; itr < flagCount; itr++) { 
		ONtFlag *curFlag = flagList + itr;

		// matrix
		curLine = BFrTextFile_GetNextStr(textFile);
		sscanf(curLine, "%f %f %f %f", 
			&curFlag->matrix.m[0][0], 
			&curFlag->matrix.m[1][0], 
			&curFlag->matrix.m[2][0],
			&curFlag->matrix.m[3][0]);

		curLine = BFrTextFile_GetNextStr(textFile);
		sscanf(curLine, "%f %f %f %f", 
			&curFlag->matrix.m[0][1], 
			&curFlag->matrix.m[1][1], 
			&curFlag->matrix.m[2][1],
			&curFlag->matrix.m[3][1]);

		curLine = BFrTextFile_GetNextStr(textFile);
		sscanf(curLine, "%f %f %f %f", 
			&curFlag->matrix.m[0][2], 
			&curFlag->matrix.m[1][2], 
			&curFlag->matrix.m[2][2],
			&curFlag->matrix.m[3][2]);

		// translation
		curLine = BFrTextFile_GetNextStr(textFile);
		sscanf(curLine, "%f %f %f", 
			&curFlag->location.x,
			&curFlag->location.y,
			&curFlag->location.z);

		// rotation
		curFlag->rotation = BFrTextFile_GetFloat(textFile);

		// id
		curFlag->idNumber = BFrTextFile_GetUUtInt16(textFile);
	}

	*outFlags = flagList;
	*outFlagFileCount = flagCount;

exit:
	if (textFile != NULL) { BFrTextFile_Close(textFile); }

	return;
}

static UUtError
IMPiEnv_CreateInstance_Flags(BFtFileRef* inSourceFileRef,
							 const IMPtEnv_BuildData *inBuildData, ONtLevel *level)
{
	UUtUns16 flagIndex;
	UUtError error;
	UUtUns16 maxFlags = inBuildData->numFlags + 100;
	BFtFileRef *flagFileRef;

	ONtFlag *flagFileFlags;
	UUtUns16 flagFileCount;

	UUmAssertReadPtr(inBuildData, sizeof(*inBuildData));
	UUmAssertWritePtr(level, sizeof(*level));
	UUmAssert(NULL == level->flagArray);

	error = BFrFileRef_DuplicateAndReplaceName(
		inSourceFileRef, 
		"flag_file.txt", 
		&flagFileRef);

	if (UUcError_None != error) {
		Imp_PrintWarning("failed to create file ref for flag file");
		return UUcError_Generic;
	}

	if (!BFrFileRef_FileExists(flagFileRef)) {
		flagFileFlags = NULL;
		flagFileCount = 0;
	}
	else {
		iParseFlagFileFlags(flagFileRef, &flagFileFlags, &flagFileCount);
		maxFlags += flagFileCount;

	}
	
	BFrFileRef_Dispose(flagFileRef);

	error = TMrConstruction_Instance_NewUnique(
		ONcTemplate_FlagArray,
		maxFlags,
		&level->flagArray);
	UUmError_ReturnOnError(error);

	UUrMemory_Clear(level->flagArray->flags, sizeof(ONtFlag) * maxFlags);

	level->flagArray->curNumFlags = inBuildData->numFlags + flagFileCount;
	level->flagArray->maxFlags = maxFlags;

	// copy the flags from the environment
	for (flagIndex=0; flagIndex < inBuildData->numFlags; flagIndex++)
	{
		const IMPtEnv_Flag *srcFlag = inBuildData->flagList + flagIndex;
		ONtFlag *dstFlag = level->flagArray->flags + flagIndex;

		dstFlag->matrix = srcFlag->matrix;

		InverseMaxMatrix(&dstFlag->matrix);

		dstFlag->location = MUrMatrix_GetTranslation(&dstFlag->matrix);

		dstFlag->idNumber = (UUtInt16) srcFlag->idNumber;
		dstFlag->deleted = UUcFalse;
		dstFlag->maxFlag = UUcTrue;
		
		// rotation
		{
			M3tPoint3D rotPoint;
			M3tMatrix4x3 rotMatrix;

			rotPoint.x = 1; rotPoint.y = 0; rotPoint.z = 0;
			MUrMatrix_To_RotationMatrix(&dstFlag->matrix, &rotMatrix);
			MUrMatrix_MultiplyPoint(&rotPoint, &rotMatrix, &rotPoint);

			if (UUmFloat_CompareEqu(0,rotPoint.x) && UUmFloat_CompareEqu(0,rotPoint.y)) {
				dstFlag->rotation = 0.f;
			}
			else {
				dstFlag->rotation = MUrATan2(rotPoint.x, rotPoint.z);
				UUmTrig_ClipLow(dstFlag->rotation);
			}
		}
	}

	// copy the flags from the file
	UUrMemory_MoveFast(flagFileFlags, level->flagArray->flags + inBuildData->numFlags, sizeof(ONtFlag) * flagFileCount);

	if (NULL != flagFileFlags) { UUrMemory_Block_Delete(flagFileFlags); }
	
	return UUcError_None;
}

static UUtError
IMPiEnv_CreateInstance_Triggers(BFtFileRef* inSourceFileRef,
							 const IMPtEnv_BuildData *inBuildData, ONtLevel *level)
{
	UUtError error;
	UUtUns16 maxTriggers = 100;
	BFtFileRef *triggerFileRef;

	ONtTrigger *triggerFileTriggers;
	UUtUns16 triggerFileCount;

	UUmAssertReadPtr(inBuildData, sizeof(*inBuildData));
	UUmAssertWritePtr(level, sizeof(*level));
	UUmAssert(NULL == level->triggerArray);

	error = BFrFileRef_DuplicateAndReplaceName(
		inSourceFileRef, 
		"trigger_file.txt", 
		&triggerFileRef);

	if (UUcError_None != error) {
		Imp_PrintWarning("failed to create file ref for trigger file");
		return UUcError_Generic;
	}

	if (!BFrFileRef_FileExists(triggerFileRef)) {
		triggerFileTriggers = NULL;
		triggerFileCount = 0;
	}
	else {
		//iParseTriggerFileTriggers(triggerFileRef, &triggerFileFlags, &triggerFileCount);
		//maxFlags += triggerFileCount;

	}
	
	BFrFileRef_Dispose(triggerFileRef);

	error = TMrConstruction_Instance_NewUnique(
		ONcTemplate_TriggerArray,
		maxTriggers,
		&level->triggerArray);
	UUmError_ReturnOnError(error);

	UUrMemory_Clear(level->triggerArray->triggers, sizeof(ONtTrigger) * maxTriggers);

	level->triggerArray->curNumTriggers = triggerFileCount;
	level->triggerArray->maxTriggers = maxTriggers;

	// copy the triggers from the file
	UUrMemory_MoveFast(triggerFileTriggers, level->triggerArray->triggers, sizeof(ONtFlag) * triggerFileCount);

	if (NULL != triggerFileTriggers) { UUrMemory_Block_Delete(triggerFileTriggers); }
	
	return UUcError_None;
}

static UUtError
IMPiEnv_CreateInstance_OctTree(
	IMPtEnv_BuildData*	inBuildData,
	IMPtEnv_OTData*		inOTData,
	AKtOctTree*			*outOctTree)
{
	UUtError		error;
	AKtOctTree*		octTree;
	
	error = 
		TMrConstruction_Instance_NewUnique(
			AKcTemplate_OctTree,
			0,
			&octTree);
	UUmError_ReturnOnError(error);
	
	*outOctTree = octTree;
	
	error = 
		TMrConstruction_Instance_NewUnique(
			AKcTemplate_OctTree_InteriorNodeArray,
			inOTData->nextInteriorNodeIndex,
			&octTree->interiorNodeArray);
	UUmError_ReturnOnError(error);
	
	error = 
		TMrConstruction_Instance_NewUnique(
			AKcTemplate_OctTree_LeafNodeArray,
			inOTData->nextLeafNodeIndex,
			&octTree->leafNodeArray);
	UUmError_ReturnOnError(error);
	
	error = 
		TMrConstruction_Instance_NewUnique(
			AKcTemplate_QuadTree_NodeArray,
			inOTData->nextQTNodeIndex,
			&octTree->qtNodeArray);
	UUmError_ReturnOnError(error);
	
	error = 
		TMrConstruction_Instance_NewUnique(
			TMcTemplate_IndexArray,
			inOTData->nextGQIndex,
			&octTree->gqIndices);
	UUmError_ReturnOnError(error);
	
	error = 
		TMrConstruction_Instance_NewUnique(
			TMcTemplate_IndexArray,
			inOTData->nextBNVIndex,
			&octTree->bnvIndices);
	UUmError_ReturnOnError(error);
	
	UUrMemory_MoveFast(
		inOTData->interiorNodes,
		octTree->interiorNodeArray->nodes,
		inOTData->nextInteriorNodeIndex * sizeof(AKtOctTree_InteriorNode));
	
	UUrMemory_MoveFast(
		inOTData->leafNodes,
		octTree->leafNodeArray->nodes,
		inOTData->nextLeafNodeIndex * sizeof(AKtOctTree_LeafNode));
	
	UUrMemory_MoveFast(
		inOTData->qtNodes,
		octTree->qtNodeArray->nodes,
		inOTData->nextQTNodeIndex * sizeof(AKtQuadTree_Node));
	
	UUrMemory_MoveFast(
		inOTData->gqIndexArray,
		octTree->gqIndices->indices,
		inOTData->nextGQIndex * sizeof(UUtUns32));
	
	UUrMemory_MoveFast(
		inOTData->bnv_index_array,
		octTree->bnvIndices->indices,
		inOTData->nextBNVIndex * sizeof(UUtUns32));
	
	return UUcError_None;
}

static UUtError
IMPiEnv_CreateInstance_GQArray(
	IMPtEnv_BuildData*	inBuildData,
	AKtEnvironment*		environment)
{
	UUtError error;
	UUtUns32			curGQIndex;
	IMPtEnv_GQ*			curGQ;
	AKtGQ_General*		curAkiraGQGeneral;
	AKtGQ_Render*		curAkiraGQRender;
	AKtGQ_Collision*	curAkiraGQCollision;
	AKtGQ_Debug*		curAkiraGQDebug;

	UUtUns32			numTris;
	UUtUns32			numVisIgnored;
	UUtUns32			num2Sided;
	UUtUns32			numGhost;
	UUtUns32			numDoor;
	UUtUns32			numTransparent;
	UUtUns32			numLightVolume;

   error = 
		TMrConstruction_Instance_NewUnique(
			AKcTemplate_GQ_General,
			inBuildData->numGQs,
			&environment->gqGeneralArray);
	IMPmError_ReturnOnErrorMsg(error, "Could not create general gunk quad array");
	
   error = 
		TMrConstruction_Instance_NewUnique(
			AKcTemplate_GQ_Render,
			inBuildData->numGQs,
			&environment->gqRenderArray);
	IMPmError_ReturnOnErrorMsg(error, "Could not create render gunk quad array");
	
   error = 
		TMrConstruction_Instance_NewUnique(
			AKcTemplate_GQ_Collision,
			inBuildData->numGQs,
			&environment->gqCollisionArray);
	IMPmError_ReturnOnErrorMsg(error, "Could not create collision gunk quad array");

	if (IMPgEnvironmentDebug) {
		error = 
			TMrConstruction_Instance_NewUnique(
				AKcTemplate_GQ_Debug,
				inBuildData->numGQs,
				&environment->gqDebugArray);
		IMPmError_ReturnOnErrorMsg(error, "Could not create debug gunk quad array");
	} else {
		environment->gqDebugArray = NULL;
	}
	
	fprintf(IMPgEnv_StatsFile, "num gqs: %d"UUmNL, inBuildData->numGQs);
	
	UUmAssertReadPtr(environment->gqGeneralArray->gqGeneral, sizeof(AKtGQ_General) * inBuildData->numGQs);
	UUmAssertReadPtr(environment->gqRenderArray->gqRender, sizeof(AKtGQ_Render) * inBuildData->numGQs);
	UUmAssertReadPtr(environment->gqCollisionArray->gqCollision, sizeof(AKtGQ_Collision) * inBuildData->numGQs);
	if (IMPgEnvironmentDebug) {
		UUmAssertReadPtr(environment->gqDebugArray->gqDebug, sizeof(AKtGQ_Debug) * inBuildData->numGQs);
	}
	
	UUrMemory_Block_VerifyList();
	
	// copy the GQs and generate statistics
	numTris = 0;
	numVisIgnored = 0;
	num2Sided = 0;
	numGhost = 0;
	numDoor = 0;
	numTransparent = 0;
	numLightVolume = 0;

	curGQ = inBuildData->gqList;
	curAkiraGQGeneral = environment->gqGeneralArray->gqGeneral;
	curAkiraGQRender = environment->gqRenderArray->gqRender;
	if (IMPgEnvironmentDebug) {
		curAkiraGQDebug = environment->gqDebugArray->gqDebug;
	}
	curAkiraGQCollision = environment->gqCollisionArray->gqCollision;

	for(curGQIndex = 0; curGQIndex < inBuildData->numGQs; curGQIndex++)
	{
		if((curGQIndex % 500) == 0) Imp_PrintMessage(IMPcMsg_Important, ".");
	
		curAkiraGQGeneral->m3Quad.vertexIndices.indices[0] = curGQ->visibleQuad.indices[0];
		curAkiraGQGeneral->m3Quad.vertexIndices.indices[1] = curGQ->visibleQuad.indices[1];
		curAkiraGQGeneral->m3Quad.vertexIndices.indices[2] = curGQ->visibleQuad.indices[2];
		curAkiraGQGeneral->m3Quad.vertexIndices.indices[3] = curGQ->visibleQuad.indices[3];

		curAkiraGQGeneral->m3Quad.baseUVIndices.indices[0] = curGQ->baseMapIndices.indices[0];
		curAkiraGQGeneral->m3Quad.baseUVIndices.indices[1] = curGQ->baseMapIndices.indices[1];
		curAkiraGQGeneral->m3Quad.baseUVIndices.indices[2] = curGQ->baseMapIndices.indices[2];
		curAkiraGQGeneral->m3Quad.baseUVIndices.indices[3] = curGQ->baseMapIndices.indices[3];
		

		if (IMPrLM_DontProcessGQ(curGQ) || (!IMPgLightmaps)) {
			UUtUns32 default_vertex_lighting = (IMPgGreen) ? IMPcEnv_DefaultVertexLighting_Green : IMPcEnv_DefaultVertexLighting_Normal;

			curAkiraGQGeneral->m3Quad.shades[0] = default_vertex_lighting;
			curAkiraGQGeneral->m3Quad.shades[1] = default_vertex_lighting;
			curAkiraGQGeneral->m3Quad.shades[2] = default_vertex_lighting;
			curAkiraGQGeneral->m3Quad.shades[3] = default_vertex_lighting;
		}
		else {
			curAkiraGQGeneral->m3Quad.shades[0] = curGQ->shade[0];
			curAkiraGQGeneral->m3Quad.shades[1] = curGQ->shade[1];
			curAkiraGQGeneral->m3Quad.shades[2] = curGQ->shade[2];
			curAkiraGQGeneral->m3Quad.shades[3] = curGQ->shade[3];
		}
		
/*		curAkiraGQRender->adjGQIndices[0] = curGQ->adjGQIndices[0];
		curAkiraGQRender->adjGQIndices[1] = curGQ->adjGQIndices[1];
		curAkiraGQRender->adjGQIndices[2] = curGQ->adjGQIndices[2];
		curAkiraGQRender->adjGQIndices[3] = curGQ->adjGQIndices[3];*/

		curAkiraGQCollision->bBox			= curGQ->bBox;		
		curAkiraGQGeneral->flags			= curGQ->flags;
		curAkiraGQGeneral->object_tag		= curGQ->object_tag;
		curAkiraGQCollision->planeEquIndex	= curGQ->planeEquIndex;
		curAkiraGQRender->textureMapIndex	= curGQ->textureMapIndex;
		curAkiraGQGeneral->flags			|= (curGQ->projection << AKcGQ_Flag_Projection_Shift);
		//curAkiraGQ->patchArea = curGQ->patchArea;
		
		if (IMPgEnvironmentDebug) {
			// Debugging goo
			curAkiraGQDebug->object_name		= Imp_AddDebugString(inBuildData, curGQ->objName);
			curAkiraGQDebug->file_name			= Imp_AddDebugString(inBuildData, curGQ->fileName);
		}
		
		if( curGQ->object_tag != 0xFFFFFFFF )
		{
			IMPrEnv_Add_ObjectTag( inBuildData, curGQ->object_tag, curGQIndex );
		}

		if(curAkiraGQGeneral->flags & AKcGQ_Flag_Triangle)
		{
			numTris++;
		}
		if(curAkiraGQGeneral->flags & AKcGQ_Flag_Transparent)
		{
			numTransparent++;
		}
		
		if(curAkiraGQGeneral->flags & AKcGQ_Flag_No_Occlusion)
		{
			numVisIgnored++;
		}
		
		if(curAkiraGQGeneral->flags & AKcGQ_Flag_2Sided)
		{
			num2Sided++;
		}
		
		if(curAkiraGQGeneral->flags & AKcGQ_Flag_Ghost)
		{
			numGhost++;
		}
		
		if(curAkiraGQGeneral->flags & AKcGQ_Flag_Door)
		{
			numDoor++;		
		}

		curGQ++;
		curAkiraGQGeneral++;
		curAkiraGQRender++;
		curAkiraGQCollision++;
		if (IMPgEnvironmentDebug) {
			curAkiraGQDebug++;
		}
	}

	fprintf(IMPgEnv_StatsFile, "\tnumTris: %d"UUmNL, numTris);
	fprintf(IMPgEnv_StatsFile, "\tnumVisIgnore: %d"UUmNL, numVisIgnored);
	fprintf(IMPgEnv_StatsFile, "\tnum2sided: %d"UUmNL, num2Sided);
	fprintf(IMPgEnv_StatsFile, "\tnumTransparent: %d"UUmNL, numTransparent);
	fprintf(IMPgEnv_StatsFile, "\tnumGhost: %d"UUmNL, numGhost);
	fprintf(IMPgEnv_StatsFile, "\tnumDoor: %d"UUmNL, numDoor);
	fprintf(IMPgEnv_StatsFile, "\tnumLightVolume: %d"UUmNL, numLightVolume);

	return UUcError_None;
}

static UUtError IMPiEnv_CreateObjectTagArray(
	ONtLevel*			inLevel,
	IMPtEnv_BuildData*	inBuildData )
{
	UUtError			error;
	UUtUns32			object_tag;
	UUtUns32*			tag_array;
	UUtUns32*			new_tags;
	UUtUns32			new_index;
	UUtUns32			tag_count;
	UUtUns32			gq_count;
	UUtUns32			i, j;
	UUtMemory_Array*	new_array;
	UUtUns32			unique_tags;
	UUtBool				found;
	ONtObjectGunkArray*	object_array;

	TMtIndexArray*		quad_array;
	UUtUns32*			gq_array;

	UUmAssert( inBuildData && inBuildData->object_tags );
	
	new_array = UUrMemory_Array_New( sizeof(UUtUns32),  sizeof(UUtUns32) * 16, 0, 0 );
	new_tags = (UUtUns32*) UUrMemory_Array_GetMemory( new_array );
	unique_tags = 0;

	UUmAssert( UUrMemory_Array_GetUsedElems( inBuildData->object_tags ) == UUrMemory_Array_GetUsedElems( inBuildData->object_quads ) );

	tag_count = UUrMemory_Array_GetUsedElems( inBuildData->object_tags );

	if( !tag_count )
		return UUcError_None;
	
	tag_array = (UUtUns32*) UUrMemory_Array_GetMemory( inBuildData->object_tags );
	gq_array = (UUtUns32*) UUrMemory_Array_GetMemory( inBuildData->object_quads );

	for( i = 0; i <tag_count; i++ )
	{
		found		= UUcFalse;
		object_tag	= tag_array[i];
		for( j = 0; j < unique_tags; j++ )
		{
			if( new_tags[j] == object_tag )
			{
				found = UUcTrue;
				break;
			}
		}
		if( !found )
		{
			error = UUrMemory_Array_GetNewElement( new_array, &new_index, NULL );
			
			UUmAssert( error == UUcError_None );

			new_tags = (UUtUns32*) UUrMemory_Array_GetMemory( new_array );

			new_tags[new_index] = object_tag;

			unique_tags++;
		}
	}
	
	if( !unique_tags )
		return UUcError_None;

	// build object gunk array
	error =  TMrConstruction_Instance_NewUnique( ONcTemplate_ObjectGunkArray, unique_tags, &object_array );
	IMPmError_ReturnOnErrorMsg(error, "Could not create object oid");

	// grab the unique tag list
	new_tags = (UUtUns32*) UUrMemory_Array_GetMemory( new_array );
	for( i = 0; i < unique_tags; i++ )
	{
		ONtObjectGunk			*object_gunk;
		UUtUns32				k;

		object_tag				= new_tags[i];
		object_gunk				= &object_array->objects[i];
		object_gunk->object_tag	= object_tag;
		gq_count				= 0;
		
		for( j = 0; j <tag_count; j++ )
		{
			if( tag_array[j] == object_tag )
			{
				if( gq_array[j] != 0xffffffff )
					gq_count++;
			}
		}				
		
		if( gq_count )
		{
			error =  TMrConstruction_Instance_NewUnique( TMcTemplate_IndexArray, gq_count, &quad_array );
			IMPmError_ReturnOnErrorMsg(error, "Could not create array");
			k = 0;

			for( j = 0; j <tag_count; j++ )
			{
				if( tag_array[j] == object_tag )
				{
					if( gq_array[j] != 0xffffffff )
						quad_array->indices[k++]	= gq_array[j];
				}
			}						
			object_gunk->gq_array	= quad_array;
		}
		else
		{
			object_gunk->gq_array	= NULL;
		}
	}

	inLevel->object_gunk_array = object_array;

	UUrMemory_Array_Delete( new_array );

	return UUcError_None;
}

// The Doors

static UUtError IMPiEnv_CreateTheDoors( ONtLevel* inLevel, IMPtEnv_BuildData* inBuildData )
{
	UUtError				error;
	UUtUns32				i;
	UUtUns32				door_count;
	AKtDoorFrameArray		*door_frame_array;
	AKtDoorFrame			*door_frame;
	UUtUns32				gq_index;
	IMPtEnv_GQ*				gq;
	IMPtEnv_GQ*				gq_list;
	AKtEnvironment*			env;

	M3tPoint3D				position;
	AKtGQ_Collision			*collision;
	AKtGQ_General			*quad;
	AKtGQ_Render			*rend;
	UUtUns32				v1, v2, v3, v4;
	M3tPoint3D				points[4];
	float					horiz_angle;
	M3tPoint3D				center_point;
	float					width;
	float					height;
	M3tBoundingBox_MinMax	bBox;	
	float					delta_x;
	float					delta_z;

	UUmAssert( inBuildData && inLevel && inLevel->environment );

	// grab some stuff for The Doors
	gq_list		= inBuildData->gqList;

	door_count	= inBuildData->door_count;

	env			= inLevel->environment;

	// build master Doors list
	error =  TMrConstruction_Instance_NewUnique( AKcTemplate_DoorFrameArray, door_count, &door_frame_array );
	IMPmError_ReturnOnErrorMsg(error, "Could not create door frame array");

	// process The Doors
	for( i = 0; i < door_count; i++ )
	{
		door_frame = &door_frame_array->door_frames[i];

		gq_index = inBuildData->door_gq_indexes[i];

		gq	= &gq_list[gq_index];
		quad = &env->gqGeneralArray->gqGeneral[ gq_index ];
		rend = &env->gqRenderArray->gqRender[ gq_index ];
		collision = &env->gqCollisionArray->gqCollision[ gq_index ];

		// make sure we have a door quad 
		UUmAssert( gq->flags & AKcGQ_Flag_Door );

		// add some flags
		gq->flags |= AKcGQ_Flag_No_Collision | AKcGQ_Flag_DrawBothSides | AKcGQ_Flag_Transparent;

		bBox = collision->bBox;

		delta_z = bBox.maxPoint.z - bBox.minPoint.z;
		delta_x = bBox.maxPoint.x - bBox.minPoint.x;
		height = bBox.maxPoint.y - bBox.minPoint.y;

		width = MUrSqrt( delta_z * delta_z + delta_x * delta_x );

		horiz_angle = MUrATan2(delta_z, delta_x);	// * M3cRadToDeg;

		v1 = quad->m3Quad.vertexIndices.indices[0];
		v2 = quad->m3Quad.vertexIndices.indices[1];
		v3 = quad->m3Quad.vertexIndices.indices[2];
		v4 = quad->m3Quad.vertexIndices.indices[3];

		points[0] = env->pointArray->points[v1];
		points[1] = env->pointArray->points[v2];
		points[2] = env->pointArray->points[v3];
		points[3] = env->pointArray->points[v4];
		
		center_point.x = points[0].x + points[1].x + points[2].x + points[3].x;
		center_point.y = points[0].y + points[1].y + points[2].y + points[3].y;
		center_point.z = points[0].z + points[1].z + points[2].z + points[3].z;
		
		center_point.x /= 4;
		center_point.y /= 4;
		center_point.z /= 4;

		position		= center_point;
		//position.y		= bBox.minPoint.y;

		// build The Door
		door_frame->gq_index	= gq_index;
		door_frame->bBox		= bBox;
		door_frame->position	= position;
		door_frame->facing		= horiz_angle;
		door_frame->width		= width;
		door_frame->height		= height;
	}
	
	env->door_frames = door_frame_array;

	return UUcError_None;
}

static UUtError
IMPrEnv_CreateCorpses(
	ONtLevel*			inLevel,
	BFtFileRef*			inSourceFileRef)
{
#define IMPcMaxCorpses 20
	BFtFileRef *corpse_directory_ref;
	BFtFileRef *corpse_files[IMPcMaxCorpses];
	UUtUns16 num_corpse_files;
	UUtUns16 itr;
	UUtError error;
	ONtCorpseArray *corpse_array;

	error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, "Conversion_Files\\Corpses", &corpse_directory_ref);
	error = BFrDirectory_GetFileList(corpse_directory_ref, NULL, "corpse.dat", IMPcMaxCorpses, &num_corpse_files, corpse_files);
	BFrFileRef_Dispose(corpse_directory_ref);

	num_corpse_files = UUmMin(num_corpse_files, IMPcMaxCorpses - 2);

	error = TMrConstruction_Instance_NewUnique(ONcTemplate_CorpseArray, IMPcMaxCorpses, &corpse_array);
	corpse_array->max_corpses = IMPcMaxCorpses;
	corpse_array->static_corpses = 0;
	
	for(itr = 0; itr < corpse_array->max_corpses; itr++)
	{
		ONtCorpse *corpse = corpse_array->corpses + itr;
		UUrMemory_Clear(corpse , sizeof(*corpse));

		if (itr < num_corpse_files) 
		{
			BFtFile *stream = NULL;
			
			BFrFile_Open(corpse_files[itr], "r", &stream);

			if (NULL != stream) {
				char character_class_name[128];
				UUtUns16 i;

				UUrString_Copy(corpse->corpse_name, BFrFileRef_GetLeafName(corpse_files[itr]), sizeof(corpse->corpse_name));
				
				BFrFile_Read(stream, 128, &character_class_name);
				BFrFile_Read(stream, sizeof(corpse->corpse_data), &corpse->corpse_data);
				
				for (i = 0; i < ONcNumCharacterParts; i++)
				{
					UUtUns16			j;
					M3tMatrix4x3		*matrix;
					
					matrix = &corpse->corpse_data.matricies[i];
					
					for (j = 0; j < 4; j++)
					{
						UUtUns16		k;
						
						for (k = 0; k < 3; k++)
						{
							UUmSwapLittle_4Byte(&matrix->m[j][k]);
						}
					}
				}
				
				UUmSwapLittle_4Byte(&corpse->corpse_data.corpse_bbox.minPoint.x);
				UUmSwapLittle_4Byte(&corpse->corpse_data.corpse_bbox.minPoint.y);
				UUmSwapLittle_4Byte(&corpse->corpse_data.corpse_bbox.minPoint.z);
				
				UUmSwapLittle_4Byte(&corpse->corpse_data.corpse_bbox.maxPoint.x);
				UUmSwapLittle_4Byte(&corpse->corpse_data.corpse_bbox.maxPoint.y);
				UUmSwapLittle_4Byte(&corpse->corpse_data.corpse_bbox.maxPoint.z);
				
				TMrConstruction_Instance_GetPlaceHolder(TRcTemplate_CharacterClass, character_class_name, (UUtUns32 *) &corpse->corpse_data.characterClass);
				
				BFrFile_Close(stream);
			}

			corpse_array->static_corpses++;
		}
	}

	corpse_array->next = corpse_array->static_corpses;

	inLevel->corpseArray = corpse_array;

	return UUcError_None;
}

UUtError
IMPrEnv_CreateInstance(
	char*				inInstanceName,
	BFtFileRef*			inSourceFileRef,
	IMPtEnv_BuildData*	inBuildData)
{
	UUtError			error;
	AKtEnvironment*		environment;
	ONtLevel*			level;
	
	AKtBNVNodeArray*	bnvNodeArray;
	
	UUtUns32			curBNVIndex;
	IMPtEnv_BNV*		curBNV;
	
	UUtUns32			curSideIndex;
	IMPtEnv_BNV_Side*	curSide;
	
	
	AKtBNVNode*			curAkiraNode;
	AKtBNV_BSPNode*		curBSPNode;
		
	UUtUns32			numVisibleQuads = 0;
	UUtUns32			numBSPNodes = 0;
	UUtUns32			numBNVQuadIndices = 0;
	UUtUns32			numPartitionIndices = 0;
	
	UUtUns32			posNodeIndex;
	UUtUns32			negNodeIndex;
	
	UUtUns32			numGQIndIndices = 0;
	
	UUtUns32			numSides = 0;
	AKtBNVNode_Side*	curAkiraSide;
	
	UUtUns32			curBSPIndex;
	
	UUtUns32			curIndex;
	
	UUtUns32			numAdjacencies;
	AKtAdjacency*		curAkiraAdjacency;
	
	UUtUns16			curTextureIndex;
	AUtSharedString*	curEnvTextureMap;
	AUtSharedString*	textureList;
	UUtUns16			numTextures;
	char*				string;
	UUtInt64			time;
		
	fprintf(IMPgEnv_StatsFile, "** environment data"UUmNL);
	
	time = UUrMachineTime_High();
	Imp_PrintMessage(IMPcMsg_Important, "building instance...");

	// Create level
	
	error = 
		TMrConstruction_Instance_Renew(
			ONcTemplate_Level,
			inInstanceName,
			0,
			&level);
	IMPmError_ReturnOnErrorMsg(error, "Could not renew level");
	GRrGroup_GetString(inBuildData->environmentGroup,"instance",&string);
	UUrString_Copy(level->name,string,ONcMaxLevelName);


	IMPrEnv_CreateCorpses(level, inSourceFileRef);
	
	// Create environment
	
	error = 
		TMrConstruction_Instance_Renew(
			AKcTemplate_Environment,
			inInstanceName,
			0,
			&level->environment);
	IMPmError_ReturnOnErrorMsg(error, "Could not renew environment");
	environment = level->environment;
	
	level->environment->inchesPerPixel = IMPgEnv_InchesPerPixel;

	// Create the point array
		error = 
			AUrSharedPointArray_CreateTemplate(
				inBuildData->sharedPointArray,
				NULL,
				&environment->pointArray);
		IMPmError_ReturnOnErrorMsg(error, "Could not create shared point array");
		Imp_PrintMessage(IMPcMsg_Cosmetic, "num points: %d"UUmNL, environment->pointArray->numPoints);
		fprintf(IMPgEnv_StatsFile, "num points: %d"UUmNL, environment->pointArray->numPoints);
	
	// Create the planeArray
		error = 
			AUrSharedPlaneEquArray_CreateTemplate(
				inBuildData->sharedPlaneEquArray,
				NULL,
				&environment->planeArray);
		IMPmError_ReturnOnErrorMsg(error, "Could not create shared plane array");
		Imp_PrintMessage(IMPcMsg_Cosmetic, "num planes: %d"UUmNL, environment->planeArray->numPlanes);
		fprintf(IMPgEnv_StatsFile, "num planes: %d"UUmNL, environment->planeArray->numPlanes);
	
	// Create the texture coord array
		error = 
			AUrSharedTexCoordArray_CreateTemplate(
				inBuildData->sharedTextureCoordArray,
				NULL,
				&environment->textureCoordArray);
		IMPmError_ReturnOnErrorMsg(error, "Could not create shared texture coord array");
		Imp_PrintMessage(IMPcMsg_Cosmetic, "num texture coords: %d"UUmNL, environment->textureCoordArray->numTextureCoords);
		fprintf(IMPgEnv_StatsFile, "num texture coords: %d"UUmNL, environment->textureCoordArray->numTextureCoords);
	
	// Create the oct tree
		error = 
			IMPiEnv_CreateInstance_OctTree(
				inBuildData,
				&inBuildData->otData,
				&environment->octTree);
		UUmError_ReturnOnError(error);
		
	// Create the gunk quad array
		error = IMPiEnv_CreateInstance_GQArray(inBuildData, environment);
		IMPmError_ReturnOnErrorMsg(error, "Could not create the GQ array");
				
	// Create the texture map array
		textureList = AUrSharedStringArray_GetList(inBuildData->textureMapArray);
		numTextures = (UUtUns16)AUrSharedStringArray_GetNum(inBuildData->textureMapArray);
		
		fprintf(IMPgEnv_StatsFile, "num texture maps: %d"UUmNL, numTextures);

		error = 
			TMrConstruction_Instance_NewUnique(
				M3cTemplate_TextureMapArray,
				numTextures,
				&environment->textureMapArray);
		IMPmError_ReturnOnErrorMsg(error, "Could not create texturemap array");
	
		for(curTextureIndex = 0, curEnvTextureMap = textureList;
			curTextureIndex < numTextures;
			curTextureIndex++, curEnvTextureMap++)
		{
			if(inBuildData->envTextureList[curTextureIndex].texture == 0)
			{
				Imp_PrintMessage(IMPcMsg_Important, "NULL texture, %s\n", curEnvTextureMap->string);
				UUmAssert(0);
			}
			environment->textureMapArray->maps[curTextureIndex] = inBuildData->envTextureList[curTextureIndex].texture;
		}
		fprintf(IMPgEnv_StatsFile, "num texture maps: %d"UUmNL, numTextures);
		
			
	// Create the node array
		error = 
			TMrConstruction_Instance_NewUnique(
				AKcTemplate_BNVNodeArray,
				inBuildData->numBNVs,
				&environment->bnvNodeArray);
		IMPmError_ReturnOnErrorMsg(error, "Could not create node array");
		fprintf(IMPgEnv_StatsFile, "num BNVs: %d"UUmNL, inBuildData->numBNVs);
		
		bnvNodeArray = environment->bnvNodeArray;
		
		for(curBNVIndex = 0, curBNV = inBuildData->bnvList;
			curBNVIndex < inBuildData->numBNVs;
			curBNVIndex++, curBNV++)
		{
			curAkiraNode = bnvNodeArray->nodes + curBNVIndex;
			curAkiraNode->index = (UUtUns32)curBNVIndex;
			
			// Assign child and next nodes
				curAkiraNode->childIndex = curBNV->child;
				curAkiraNode->nextIndex = curBNV->next;	
			
			// Assign root bsp node
				curAkiraNode->bspRootNode = numBSPNodes;
				numBSPNodes += curBNV->numNodes;
			
			// Assign the side indices
				curAkiraNode->sideStartIndex = numSides;
				numSides += curBNV->numSides;
				curAkiraNode->sideEndIndex = numSides;
			
			// Assign gunk quad nodes
				//curAkiraNode->gunkQuadStartIndex = numGunkQuads;
				//numGunkQuads += curBNV->numGQs;
				//curAkiraNode->gunkQuadEndIndex = numGunkQuads;
			
			// Set type flags
				curAkiraNode->flags = curBNV->flags;
				
			// Store plane and height of stairs
				curAkiraNode->stairPlane = curBNV->stairPlane;
				curAkiraNode->stairHeight = curBNV->stairHeight;
			
			// Pathfinding node will be set at runtime
				curAkiraNode->pathnodeIndex = (UUtUns32) -1;
		}
	
	// Create the side array
		error = 
			TMrConstruction_Instance_NewUnique(
				AKcTemplate_BNVNodeSideArray,
				numSides,
				&environment->bnvSideArray);
		IMPmError_ReturnOnErrorMsg(error, "Could not create node array");
		
		curAkiraSide = environment->bnvSideArray->sides;
		numAdjacencies = 0;
		
		for(curBNVIndex = 0, curBNV = inBuildData->bnvList;
			curBNVIndex < inBuildData->numBNVs;
			curBNVIndex++, curBNV++)
		{
			for(curSideIndex = 0, curSide = curBNV->sides;
				curSideIndex < curBNV->numSides;
				curSideIndex++, curSide++)
			{
				
				curAkiraSide->planeEquIndex = curSide->planeEquIndex;
				
				// Init adjacency indices
					curAkiraSide->adjacencyStartIndex = numAdjacencies;
					UUmAssert(curSide->numAdjacencies>=0 && curSide->numAdjacencies<IMPcEnv_MaxAdjacencies);
					numAdjacencies += curSide->numAdjacencies;
					curAkiraSide->adjacencyEndIndex = numAdjacencies;
				
				// Init ghost quad indices
					curAkiraSide->ghostGQStartIndIndex = numGQIndIndices;
					numGQIndIndices += curSide->numGQGhostIndices;
					curAkiraSide->ghostGQEndIndIndex = numGQIndIndices;
					
				// Assign solid quad and partition index
					curAkiraSide->bnvQuadStartIndIndex = numBNVQuadIndices;
					numBNVQuadIndices += curSide->numBNVQuads;
					curAkiraSide->bnvQuadEndIndIndex = numBNVQuadIndices;
					
				curAkiraSide++;
			}
		}

	// Create the BSP node array
		error = 
			TMrConstruction_Instance_NewUnique(
				AKcTemplate_BSPNodeArray,
				numBSPNodes,
				&environment->bspNodeArray);
		IMPmError_ReturnOnErrorMsg(error, "Could not create node array");
		
		for(curBNVIndex = 0, curBNV = inBuildData->bnvList, curAkiraNode = bnvNodeArray->nodes;
			curBNVIndex < inBuildData->numBNVs;
			curBNVIndex++, curBNV++, curAkiraNode++)
		{
			for(curBSPIndex = 0, curBSPNode = environment->bspNodeArray->nodes + curAkiraNode->bspRootNode;
				curBSPIndex < curBNV->numNodes;
				curBSPIndex++, curBSPNode++)
			{
				curBSPNode->planeEquIndex = curBNV->bspNodes[curBSPIndex].planeEquIndex;
				
				posNodeIndex = curBNV->bspNodes[curBSPIndex].posNodeIndex;
				negNodeIndex = curBNV->bspNodes[curBSPIndex].negNodeIndex;
				
				if(posNodeIndex == 0xFFFFFFFF)
				{
					curBSPNode->posNodeIndex = 0xFFFFFFFF;
				}
				else
				{
					curBSPNode->posNodeIndex = curBNV->bspNodes[curBSPIndex].posNodeIndex + curAkiraNode->bspRootNode;
				}
				
				if(negNodeIndex == 0xFFFFFFFF)
				{
					curBSPNode->negNodeIndex = 0xFFFFFFFF;
				}
				else
				{
					curBSPNode->negNodeIndex = curBNV->bspNodes[curBSPIndex].negNodeIndex + curAkiraNode->bspRootNode;
				}
			}
		}
	
	// create the alpha bsp node array
	#if 1
		error = 
			TMrConstruction_Instance_NewUnique(
				AKcTemplate_AlphaBSPTree_NodeArray,
				inBuildData->numAlphaBSPNodes,
				&environment->alphaBSPNodeArray);
		IMPmError_ReturnOnErrorMsg(error, "Could not create node array");
		
		UUrMemory_MoveFast(
			inBuildData->alphaBSPNodes,
			environment->alphaBSPNodeArray->nodes,
			inBuildData->numAlphaBSPNodes * sizeof(AKtAlphaBSPTree_Node));
	#else
	
		environment->alphaBSPNodeArray = NULL;
		
	#endif
	// Create the adjacency data
		
		UUmAssert(numAdjacencies == IMPgEnv_VerifyAdjCount);
		error = 
			TMrConstruction_Instance_NewUnique(
				AKcTemplate_AdjacencyArray,
				numAdjacencies,
				&environment->adjacencyArray);
		
		curAkiraAdjacency = environment->adjacencyArray->adjacencies;
		
		numSides = 0;	
		for(curBNVIndex = 0, curBNV = inBuildData->bnvList;
			curBNVIndex < inBuildData->numBNVs;
			curBNVIndex++, curBNV++)
		{
			if (!(curBNV->flags & AKcBNV_Flag_Room)) continue;
			
			for(curSideIndex = 0, curSide = curBNV->sides;
				curSideIndex < curBNV->numSides;
				curSideIndex++, curSide++)
			{
				for (curIndex = 0; curIndex < curSide->numAdjacencies; curIndex++)
				{
					UUmAssert(numSides < numAdjacencies);
					*curAkiraAdjacency = curSide->adjacencyList[curIndex];
					curAkiraAdjacency++;
					numSides++;
				}
			}
		}
	
	// Create the quad remap array
	{
		UUtUns16 remapCount;

		error = IMPrScript_GetRemapData(&remapCount);
		UUmError_ReturnOnError(error);

		error = TMrConstruction_Instance_NewUnique(
			TMcTemplate_IndexArray,
			remapCount,
			&environment->envQuadRemapIndices);
		UUmError_ReturnOnError(error);

		error = TMrConstruction_Instance_NewUnique(
			TMcTemplate_IndexArray,
			remapCount,
			&environment->envQuadRemaps);
		UUmError_ReturnOnError(error);

		IMPrScript_UnzipRemapArray(
			environment->envQuadRemapIndices->indices,
			environment->envQuadRemaps->indices);
	}

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	// Create pathfinding data
	Imp_PrintMessage(IMPcMsg_Important,UUmNL"Pathfinding");
	time = UUrMachineTime_High();

	for(curBNVIndex = 0, curBNV = inBuildData->bnvList;
		curBNVIndex < inBuildData->numBNVs;
		curBNVIndex++, curBNV++)
	{
		if (0 == (curBNVIndex % 100)) {
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		curAkiraNode = bnvNodeArray->nodes + curBNVIndex;

		PHrCreateRoomData(inBuildData,&inBuildData->bnvList[curBNVIndex],curAkiraNode,environment);
	}
	
	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	{
		extern UUtUns32 PHgOriginalMemoryUsage;
		extern UUtUns32 PHgCompressedMemoryUsage;

		Imp_PrintMessage(IMPcMsg_Important, UUmNL "pathfinding memory usage %2.2fk (old was %2.2fk)" UUmNL, 
			PHgCompressedMemoryUsage / (1024.f),
			PHgOriginalMemoryUsage / (1024.f));
	} 
		
	// Set up objects
	IMPiEnv_CreateInstance_Objects(inBuildData,level);
	
	// Set up marker nodes
	IMPiEnv_CreateInstance_Markers(inBuildData,level);
		
	// Set up flags
	IMPiEnv_CreateInstance_Flags(inSourceFileRef, inBuildData, level);

	// Set up triggers
	IMPiEnv_CreateInstance_Triggers(inSourceFileRef, inBuildData, level);
			
	// Create the environmental particles
	IMPrEnv_CreateInstance_EnvParticles(
		inBuildData->numEnvParticles,
		inBuildData->envParticles,
		&level->envParticleArray);
		
	// Set up combat profiles
	IMPiEnv_CreateInstance_CombatProfiles(inBuildData,environment);
	IMPiEnv_CreateInstance_TacticalProfiles(inBuildData,environment);	
		
	// Character setup array
	error = GRrGroup_GetString(inBuildData->environmentGroup,"scripts",&string);
	if (UUcError_None != error) {
		Imp_PrintWarning("Environment was missing characterSetup array");
		level->characterSetupArray = NULL;
	}

	error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_CharacterSetupArray, string, (void *) &level->characterSetupArray);
	UUmError_ReturnOnError(error);

	error = TMrConstruction_Instance_GetPlaceHolder(AIcTemplate_ScriptTriggerClassArray, string, (void *) &level->scriptTriggerArray);
	UUmError_ReturnOnError(error);

	error = TMrConstruction_Instance_GetPlaceHolder(ONcTemplate_SpawnArray, string, (void *) &level->spawnArray);
	UUmError_ReturnOnError(error);

	error = TMrConstruction_Instance_GetPlaceHolder(OBcTemplate_DoorClassArray, string, (void *) &level->doorArray);
	UUmError_ReturnOnError(error);

	// create the unique object tag list
	error = IMPiEnv_CreateObjectTagArray( level, inBuildData );
	UUmError_ReturnOnError(error);

	// The Doors
	error = IMPiEnv_CreateTheDoors( level, inBuildData );
	UUmError_ReturnOnError(error);

	// sky
	error = GRrGroup_GetString(inBuildData->environmentGroup,"sky",&string);
	if (UUcError_None != error) 
	{
		//Imp_PrintWarning("Environment was missing sky name");
		level->sky_class = NULL;
	}
	else
	{
		float sky_height;

		error = TMrConstruction_Instance_GetPlaceHolder( ONcTemplate_SkyClass, string, (TMtPlaceHolder*) &level->sky_class );
		IMPmError_ReturnOnError(error);

		error = GRrGroup_GetFloat(inBuildData->environmentGroup,"sky_height",&sky_height);
		if( error != UUcError_None )
			level->sky_height = 0;
		else
			level->sky_height = sky_height;
	}

	// Verify
	Imp_PrintMessage(IMPcMsg_Important,"Verifying pathfinding..."UUmNL);
	IMPiEnv_Verify_Pathfinding(inBuildData);

	// build the extents 
	level->environment->bbox.minPoint.x = inBuildData->minX;
	level->environment->bbox.minPoint.y = inBuildData->minY;
	level->environment->bbox.minPoint.z = inBuildData->minZ;

	level->environment->bbox.maxPoint.x = inBuildData->maxX;
	level->environment->bbox.maxPoint.y = inBuildData->maxY;
	level->environment->bbox.maxPoint.z = inBuildData->maxZ;
	

	return UUcError_None;
}


