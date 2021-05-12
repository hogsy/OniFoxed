/*
	FILE:	Imp_Environment.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 3, 1997

	PURPOSE: 
	
	Copyright 1997

*/
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "BFW_Akira.h"
#include "BFW_LSSolution.h"
#include "BFW_AppUtilities.h"
#include "BFW_BitVector.h"
#include "BFW_Object_Templates.h"
#include "BFW_Object.h"

#include "Imp_Common.h"
#include "Imp_Model.h"
#include "Imp_Env2.h"
#include "Imp_Texture.h"
#include "Imp_Env2_Private.h"
#include "Imp_ParseEnvFile.h"

UUtBool				IMPgEnv_Errors = UUcFalse;
FILE*				IMPgEnv_ErrorFile = NULL;
char*				IMPgEnv_CompileTime = __DATE__" "__TIME__;
FILE*				IMPgEnv_StatsFile = NULL;

float				IMPgEnv_InchesPerPixel = 12.0f;



AUtFlagElement IMPgObjectAnimFlags[] =
{
	{
		"loop",
		OBcAnimFlags_Loop
	},
	{
		"pingpong",
		OBcAnimFlags_Pingpong
	},
	{
		"random",
		OBcAnimFlags_Random
	},
	{
		"autostart",
		OBcAnimFlags_Autostart
	},
	{
		"local",
		OBcAnimFlags_Localized
	},
	{
		NULL,
		0
	}
};

typedef struct DebugStringHashTableEntry
{
	char *string;
	char *raw_string;
} DebugStringHashTableEntry;


void
IMPrEnv_LogError(
	char*	inMsg,
	...)
{
	va_list	ap;
	
	if(IMPgEnv_ErrorFile == NULL) return;
	IMPgEnv_Errors = UUcTrue;

	va_start(ap, inMsg);
	
	vfprintf(IMPgEnv_ErrorFile, inMsg, ap);
	
	va_end(ap);
	
	fprintf(IMPgEnv_ErrorFile, ""UUmNL);
	
	fprintf(IMPgEnv_ErrorFile, "*************************"UUmNL);
	
	fflush(IMPgEnv_ErrorFile);	
}



static IMPtEnv_BuildData*
IMPiEnv_BuildData_New(
	void)
{
	IMPtEnv_BuildData*		newBuildData;
	double					sizeOfBuildData = sizeof(IMPtEnv_BuildData) / (1024.0 * 1024.0);

	Imp_PrintMessage(
		IMPcMsg_Important, 
		"size of build data is %f megabytes"UUmNL,
		sizeOfBuildData);
	
	newBuildData = UUrMemory_Block_New(sizeof(IMPtEnv_BuildData));

	if (newBuildData == NULL) 
	{
		Imp_PrintWarning("failed to allocate build data");
		return NULL;
	}
	
	newBuildData->maxGQs = 0;
	newBuildData->numGQs = 0;
	newBuildData->maxBNVs = 0;
	newBuildData->numBNVs = 0;
	newBuildData->numFlags = 0;
	newBuildData->numObjects = 0;

	newBuildData->numEnvParticles = 0;
	
	newBuildData->numAlphaQuads = 0;
	
	newBuildData->bnvList = NULL;
	newBuildData->gqList = NULL;
	
	
	newBuildData->textureMapArray = NULL;
	newBuildData->sharedPointArray = NULL;
	newBuildData->sharedPlaneEquArray = NULL;
	newBuildData->sharedBNVQuadArray = NULL;
	
	newBuildData->bspPointArray = NULL;
	newBuildData->bspQuadArray = NULL;
	newBuildData->tempMemoryPool = NULL;
	//newBuildData->rnotData.octTreePointArray = NULL;
	newBuildData->gqTakenBV = NULL;
	
	newBuildData->textureMapArray = AUrSharedStringArray_New();
	if(newBuildData->textureMapArray == NULL) return NULL;
		
	newBuildData->sharedPointArray = AUrSharedPointArray_New();
	if(newBuildData->sharedPointArray == NULL) return NULL;

	newBuildData->sharedPlaneEquArray = AUrSharedPlaneEquArray_New();
	if(newBuildData->sharedPlaneEquArray == NULL) return NULL;

	newBuildData->sharedBNVQuadArray = AUrSharedQuadArray_New();
	if(newBuildData->sharedBNVQuadArray == NULL) return NULL;

	newBuildData->sharedTextureCoordArray = AUrSharedTexCoordArray_New();
	if(newBuildData->sharedTextureCoordArray == NULL) return NULL;
	
	newBuildData->bspPointArray = AUrSharedPointArray_New();
	if(newBuildData->bspPointArray == NULL) return NULL;

	newBuildData->bspQuadArray = AUrSharedQuadArray_New();
	if(newBuildData->bspQuadArray == NULL) return NULL;
	
	newBuildData->tempMemoryPool = UUrMemory_Pool_New(1024 * 1024, UUcPool_Growable);
	if(newBuildData->tempMemoryPool == NULL) return NULL;
	
	newBuildData->edgeArray = AUrSharedEdgeArray_New();
	if(newBuildData->edgeArray == NULL) return NULL;
	
	newBuildData->debugStringTable = AUrHashTable_New(sizeof(DebugStringHashTableEntry), 1024 * 16, AUrHash_String_Hash, AUrHash_String_IsEqual);
	if (newBuildData->debugStringTable == NULL) return NULL;

	newBuildData->debugStringBytes = 0;
	newBuildData->debugStringBytes = 0;

	// gunked object ID list	
	newBuildData->object_tags	= UUrMemory_Array_New(sizeof( UUtUns32), sizeof( UUtUns32) * 16, 0, 0 );
	newBuildData->object_quads	= UUrMemory_Array_New(sizeof( UUtUns32), sizeof( UUtUns32) * 16, 0, 0 );
	
	// door frames
	newBuildData->door_count			= 0;
	newBuildData->door_frame_texture	= 0xFFFF;

	return newBuildData;
}

static void
IMPiEnv_BuildData_Delete(
	IMPtEnv_BuildData*	inBuildData)
{
	UUmAssert(inBuildData != NULL);

	IMPrFixedOctTree_Delete(inBuildData);
	
	if(inBuildData->textureMapArray != NULL)
	{
		AUrSharedStringArray_Delete(inBuildData->textureMapArray);
	}
		
	if(inBuildData->sharedPointArray != NULL)
	{
		AUrSharedPointArray_Delete(inBuildData->sharedPointArray);
	}
	
	if(inBuildData->sharedPlaneEquArray != NULL)
	{
		AUrSharedPlaneEquArray_Delete(inBuildData->sharedPlaneEquArray);
	}
	
	if(inBuildData->sharedBNVQuadArray != NULL)
	{
		AUrSharedQuadArray_Delete(inBuildData->sharedBNVQuadArray);
	}
	
	if(inBuildData->sharedTextureCoordArray != NULL)
	{
		AUrSharedTexCoordArray_Delete(inBuildData->sharedTextureCoordArray);
	}
	
	if(inBuildData->bspPointArray != NULL)
	{
		AUrSharedPointArray_Delete(inBuildData->bspPointArray);
	}
	
	if(inBuildData->bspQuadArray != NULL)
	{
		AUrSharedQuadArray_Delete(inBuildData->bspQuadArray);
	}
	
	if(inBuildData->tempMemoryPool != NULL)
	{
		UUrMemory_Pool_Delete(inBuildData->tempMemoryPool);
	}
		
	if(inBuildData->edgeArray != NULL)
	{
		AUrSharedEdgeArray_Delete(inBuildData->edgeArray);
	}
	
	if(inBuildData->bnvList != NULL)
	{
		UUrMemory_Block_Delete(inBuildData->bnvList);
	}
	
	if(inBuildData->gqList != NULL)
	{
		UUrMemory_Block_Delete(inBuildData->gqList);
	}
	
	
	if(inBuildData->gqTakenBV != NULL)
	{
		UUrBitVector_Dispose(inBuildData->gqTakenBV);
	}
	
	if (inBuildData->debugStringTable != NULL)
	{
		AUrHashTable_Delete(inBuildData->debugStringTable);
	}

	if( inBuildData->object_tags )
	{
		UUrMemory_Array_Delete(inBuildData->object_tags);
	}

	if( inBuildData->object_quads )
	{
		UUrMemory_Array_Delete(inBuildData->object_quads);
	}
	
	UUrMemory_Block_Delete(inBuildData);
}

UUtError
IMPrEnv_Initialize(
	void)
{
	IMPgEnv_ErrorFile = fopen(IMPcEnv_ErrorLogFile, "wb");
	if(IMPgEnv_ErrorFile == NULL)
	{
		IMPmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not create error file");
	}
	
	IMPgEnv_MemoryPool = UUrMemory_Pool_New(50 * 1024 * 1024, UUcPool_Growable);
	if(IMPgEnv_MemoryPool == NULL)
	{
		IMPmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not allocate IMPgEnv_MemoryPool");
	}

	return UUcError_None;
}

void
IMPrEnv_Terminate(
	void)
{
	UUrMemory_Pool_Delete(IMPgEnv_MemoryPool);

	fclose(IMPgEnv_ErrorFile);
}

UUtError
IMPrEnv_Add(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;
	
	IMPtEnv_BuildData*	buildData;
	
	UUtBool				buildInstance;
	
	// get the inches per pixel for lightmaps
		error = 
			GRrGroup_GetFloat(
				inGroup,
				"ipp",
				&IMPgEnv_InchesPerPixel);
		if(error != UUcError_None)
		{
			IMPgEnv_InchesPerPixel = 12.0f;
		}
			
	// 1. Allocate build data memory
		buildData = IMPiEnv_BuildData_New();
		if(buildData == NULL)
		{
			IMPmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not allocate build data");
		}
		buildData->environmentGroup = inGroup;
		
	// 2. Check the exists and mod date
		if(IMPgConstructing)
		{
			buildInstance = !TMrConstruction_Instance_CheckExists(AKcTemplate_Environment, inInstanceName);
		}
		else
		{
			buildInstance = UUcTrue;
		}
	
	// create the stats file
	{
		char	buffer[128];
		
		sprintf(buffer, "%s_stats.txt", inInstanceName);
		
		IMPgEnv_StatsFile = fopen(buffer, "wb");
		UUmError_ReturnOnNullMsg(IMPgEnv_StatsFile, "could not create stats folder");
	}
	
	// 4. Parse the data into our internal data structures
		error = 
			IMPrEnv_Parse(
				inSourceFileRef,
				inGroup,
				buildData,
				&buildInstance);
		IMPmError_ReturnOnError(error);
	
	if(buildInstance == UUcTrue)
	{
		// 5. Process the data		
		error = IMPrEnv_Process(buildData, inSourceFileRef);
		IMPmError_ReturnOnError(error);
		
		if(IMPgConstructing)
		{
			// 6. Create the final instance
				if(1 || IMPgEnv_Errors == UUcFalse)
				{
					error = IMPrEnv_CreateInstance(inInstanceName, inSourceFileRef, buildData);
					IMPmError_ReturnOnError(error);
				}
				else
				{
					Imp_PrintMessage(IMPcMsg_Important, "errors occured while processing environment, not creating instance"UUmNL);
					return UUcError_Generic;
				}
		}
	}

	// 7. Delete the internal data structures
	IMPiEnv_BuildData_Delete(buildData);
	
	fclose(IMPgEnv_StatsFile);
	
	if(IMPgEnv_Errors)
	{
		Imp_PrintMessage(IMPcMsg_Important, "There are environment issues - please look at enverrors.txt"UUmNL);
	}
	
	return UUcError_None;
}


static UUtBool IMPiEnvAnim_IsMoving(
	const AXtNode *inNode,
	UUtUns16 inNumFrames)
{
	/***********
	* Returns if the node is moving
	*/

	UUtUns16 i;
	
	for (i=0; i<inNumFrames; i++)
	{
		if (!MUrMatrix_IsEqual(inNode->matricies, inNode->matricies+i)) return UUcTrue;
	}

	return UUcFalse;
}

static UUtBool comp_u16(UUtUns16 inA, UUtUns16 inB)
{
	UUtBool result = inA > inB;

	return result;
}

static void AddKeyFrames(
		UUtUns16 inFirstFrame, 
		UUtUns16 inLastFrame,
		UUtUns16 inSkip,
		UUtUns16 **ioKeyFrameList,
		UUtUns16 *ioNumKeyFrames)
{
	UUtUns16 itr;
	UUtUns16 frameSpanSize;
	UUtUns16 *keyFrameList;
	UUtUns16 numKeyFrames;
	UUtUns16 numNewKeyFrames;

	if (inLastFrame < inFirstFrame) {
		UUtUns16 swap = inLastFrame;

		inLastFrame = inFirstFrame;
		inFirstFrame = swap;
	}

	keyFrameList = *ioKeyFrameList;
	numKeyFrames = *ioNumKeyFrames;
	frameSpanSize = inLastFrame - inFirstFrame + 1;

	numNewKeyFrames = frameSpanSize / inSkip;
	keyFrameList = UUrMemory_Block_Realloc(keyFrameList, sizeof(UUtUns16) * (numKeyFrames + numNewKeyFrames + 2));

	for(itr = 0; itr < numNewKeyFrames; itr++)
	{
		keyFrameList[itr + numKeyFrames] = inFirstFrame + (itr * inSkip);
	}

	keyFrameList[itr + numKeyFrames] = inFirstFrame;
	keyFrameList[itr + numKeyFrames + 1] = inLastFrame;

	numKeyFrames = numKeyFrames + numNewKeyFrames + 2;

	*ioKeyFrameList = keyFrameList;
	*ioNumKeyFrames = numKeyFrames;

	return;
}

static UUtError CreateObjAnimKeyFrameList(
		const AXtNode *inCurNode, 
		UUtUns16 inNumFrames, 
		GRtGroup *inGroup, 
		UUtUns16 **outKeyFrameList, 
		UUtUns16 *outNumKeyFrames)
{
	UUtError error;
	char *keyFrameString;
	UUtUns16 *keyFrameList = NULL;
	UUtUns16 numKeyFrames;
	
	// add basic keyframes
	numKeyFrames = 2;
	keyFrameList = UUrMemory_Block_Realloc(keyFrameList, sizeof(UUtUns16) * numKeyFrames);
	
	keyFrameList[0] = 0;
	keyFrameList[1] = inNumFrames - 1;

	error = GRrGroup_GetString(inGroup, "keyFrames", &keyFrameString);

	if (error != UUcError_None) {
		AddKeyFrames(0, inNumFrames - 1, 5, &keyFrameList, &numKeyFrames);
	}
	else {
		char buffer[512];
		char *field;
		char *internal;

		if (strlen(keyFrameString) >= 512) {
			Imp_PrintWarning("key frame list to long");
		}

		strcpy(buffer, keyFrameString);

		for(field = UUrString_Tokenize(buffer, " \t", &internal); field != NULL; field = UUrString_Tokenize(NULL, " \t", &internal)) {
			UUtUns16 keyFrameStart;
			UUtUns16 keyFrameEnd;
			UUtUns16 frequency;

			char *dash;
			char *colon;

			dash = strpbrk(field, "-");

			// formats:
			// *:#
			// #-#:#
			// #:#
			// *
			// #-#
			// #

			if ('*' == field[0]) {
				keyFrameStart = 0;
				keyFrameEnd = inNumFrames - 1;
			}
			else if (dash != NULL) {
				*dash = '\0';

				error = UUrString_To_Uns16(field, &keyFrameStart);
				IMPmError_ReturnOnError(error);

				field = dash + 1;

				error = UUrString_To_Uns16(field, &keyFrameEnd);
				IMPmError_ReturnOnError(error);
			}
			else {
				error = UUrString_To_Uns16(field, &keyFrameStart);
				keyFrameEnd = keyFrameStart;
				IMPmError_ReturnOnError(error);
			}

			// pin key frames to legal range
			keyFrameStart = UUmPin(keyFrameStart, 0, inNumFrames - 1);
			keyFrameEnd = UUmPin(keyFrameEnd, 0, inNumFrames - 1);
			
			// generate frequency for keyframes over this range
			colon = strpbrk(field, ":");

			if (NULL == colon) {
				frequency = 1;
			}
			else {
				field = colon + 1;
				error = UUrString_To_Uns16(field, &frequency);
				IMPmError_ReturnOnError(error);
			}

			AddKeyFrames(keyFrameStart, keyFrameEnd, frequency, &keyFrameList, &numKeyFrames);
		}
	}

	AUrQSort_16(keyFrameList, numKeyFrames, comp_u16);
	numKeyFrames -= AUrSqueeze_16(keyFrameList, numKeyFrames);

	*outKeyFrameList = keyFrameList;
	*outNumKeyFrames = numKeyFrames;

	return UUcError_None;
}

static void BuildObjectKeyFrame(const AXtNode *inNode, UUtUns16 inFrame, OBtAnimationKeyFrame *outKeyFrame)
{
	MUtAffineParts parts;

	MUrMatrix_DecomposeAffine(inNode->matricies + inFrame, &parts);

	outKeyFrame->quat = parts.rotation;
	outKeyFrame->translation = parts.translation;
	outKeyFrame->frame = inFrame;

	return;
}

UUtError
IMPrEnvAnim_Add(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{ 
	UUtError error = UUcError_Generic;
	AXtHeader *header = NULL;
	BFtFileRef *EVAFileRef = NULL;
	char *fileName;
	UUtUns16 nodeIndex;//,mIndex;
	GRtElementType groupType;
	GRtElementArray *groupFlags;
	OBtAnimation *anim;
	UUtBool found;
	char nodeMatchName[256];
	//M3tPoint3D relPos,prevPos;

	error = GRrGroup_GetString(inGroup, "file", &fileName);
	IMPmError_ReturnOnError(error);

	{
		char *group_name;
		error = GRrGroup_GetString(inGroup, "maxName", &group_name);

		if (UUcError_None == error) {
			strcpy(nodeMatchName, group_name);
		}
		else {
			sprintf(nodeMatchName, "%s%s", IMPcGunkObjectNamePrefix, inInstanceName);
		}
	}

	error = BFrFileRef_DuplicateAndReplaceName(inSourceFile, fileName, &EVAFileRef);
	UUmError_ReturnOnErrorMsgP(error, "Could not find file \"%s\"", (UUtUns32)fileName, 0, 0);

	if (!BFrFileRef_FileExists(EVAFileRef))
	{
		Imp_PrintWarning("EVA file %s does not exist!", BFrFileRef_GetLeafName(EVAFileRef));
		error = UUcError_Generic;
		goto exit;
	}
	
	error = Imp_ParseEvaFile(EVAFileRef, &header);
	
	found = UUcFalse;

	// File parsed, now create the instances
	for (nodeIndex=0; nodeIndex<header->numNodes; nodeIndex++)
	{
		UUtUns16 keyFrameItr;
		UUtBool isEqual;
		const AXtNode *curNode = header->nodes + nodeIndex;

		isEqual = 0 == UUrString_Compare_NoCase(curNode->name, nodeMatchName);

		if (!isEqual) { continue; }

		{
			UUtUns16 numKeyFrames;
			UUtUns16 *keyFrameList;

			error = CreateObjAnimKeyFrameList(
				curNode, 
				header->numFrames, 
				inGroup, 
				&keyFrameList, 
				&numKeyFrames);
			UUmError_ReturnOnError(error);


			found = UUcTrue;

			// Create instance
			error = TMrConstruction_Instance_Renew(
				OBcTemplate_Animation,
				inInstanceName,
				numKeyFrames,
				(void *)&anim);
			UUmError_ReturnOnError(error);

			// clear the block
			UUrMemory_Clear(anim, sizeof(*anim));

			// Fill in data
			anim->ticksPerFrame = header->ticksPerFrame;
			anim->numKeyFrames = numKeyFrames;
			anim->numFrames = header->numFrames;

			for(keyFrameItr = 0; keyFrameItr < numKeyFrames; keyFrameItr++) {
				BuildObjectKeyFrame(curNode, keyFrameList[keyFrameItr], anim->keyFrames + keyFrameItr);
			}

			UUrMemory_Block_Delete(keyFrameList);
			keyFrameList = NULL;

			UUrMemory_MoveFast(
				curNode->matricies,
				&anim->startMatrix,
				sizeof(M3tMatrix4x3));
 
			MUrMatrix_To_ScaleMatrix(curNode->matricies, &anim->scale);

			// Parse flags
			error = GRrGroup_GetElement(inGroup, "flags", &groupType, (void *)&groupFlags);

			if (error == UUcError_None)
			{
				error = AUrFlags_ParseFromGroupArray(
					IMPgObjectAnimFlags,
					groupType,
					groupFlags,
					&anim->animFlags);
				UUmError_ReturnOnError(error);
			}
			
			// Parse door open length (for door animations only)
			error = GRrGroup_GetUns16(inGroup,"openframes",&anim->doorOpenFrames);
			if (error != UUcError_None)
			{
				error = UUcError_None;
				anim->doorOpenFrames = 0;
			}

			break;
		}
	}

	if (!found) 
	{
		Imp_PrintWarning("could not find %s in %s", inInstanceName, BFrFileRef_GetLeafName(inSourceFile));
		error = UUcError_Generic;
	}


exit:
	if (NULL != header)
	{
		Imp_EvaFile_Delete(header);
	}

	if (NULL != EVAFileRef)
	{
		BFrFileRef_Dispose(EVAFileRef);
	}
	
	return error;
}


UUtError Imp_AddQuadMaterial(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError error;
	AKtGQMaterial *material;
	//GRtElementArray *groupFlags;
	//GRtElementType groupType;
	//char *string;
	

	if (TMrConstruction_Instance_CheckExists(AKcTemplate_GQ_Material, inInstanceName))
	{
		return UUcError_None;
	}

	// Create our instance
	error = TMrConstruction_Instance_Renew(
			AKcTemplate_GQ_Material,
			inInstanceName,
			0,
			&material);
	IMPmError_ReturnOnError(error);

	// Initialize
	UUrMemory_Clear(material,sizeof(AKtGQMaterial));
	
	// Colour
	error = GRrGroup_GetUns16(inGroup,"damage",&material->damage);
	UUmError_ReturnOnError(error);
	error = GRrGroup_GetUns16(inGroup,"effectDelay",&material->effectTime);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}


char *Imp_AddDebugString(
	IMPtEnv_BuildData *inBuildData,
	char *inString)
{
	DebugStringHashTableEntry new_entry;
	DebugStringHashTableEntry *hash_table_entry;

	new_entry.string = inString;
	new_entry.raw_string = NULL;

	hash_table_entry = AUrHashTable_Find(inBuildData->debugStringTable, &new_entry);

	if (NULL == hash_table_entry) {
		hash_table_entry = AUrHashTable_Add(inBuildData->debugStringTable, &new_entry);

		if (NULL != hash_table_entry) {
			UUtUns32 string_total_size = strlen(inString + 1);

			inBuildData->debugStringBytes += string_total_size;

			hash_table_entry->raw_string = TMrConstruction_Raw_New(string_total_size, sizeof(char), AKcTemplate_GQ_Debug);
			UUmAssert(NULL != hash_table_entry->raw_string);

			strcpy(hash_table_entry->raw_string, inString);

			hash_table_entry->raw_string = TMrConstruction_Raw_Write(hash_table_entry->raw_string );
		}
	}

	return hash_table_entry->raw_string;
}