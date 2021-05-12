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

#include "Imp_Common.h"
#include "Imp_Model.h"
#include "Imp_Environment.h"
#include "Imp_BMP.h"

#include "Imp_Env_Private.h"

UUtBool				IMPgEnv_Errors = UUcFalse;
FILE*				IMPgEnv_ErrorFile = NULL;
char*				IMPgEnv_CompileTime = __DATE__" "__TIME__;

static void iCapitalizeString(char *ioString)
{
	while(*ioString != '\0') {
		if ((*ioString >= 'a') && (*ioString <= 'z')) {
			*ioString += 'A' - 'a';
		}

		ioString++;
	}

	return;
}

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
	
	fprintf(IMPgEnv_ErrorFile, "\n\r");
	
	fprintf(IMPgEnv_ErrorFile, "*************************\n\r");
	
	fflush(IMPgEnv_ErrorFile);	
}

static UUtError
IMPiEnv_Textures_Process(
	GRtGroup*			inGroup,
	UUtUns32			inSourceFileModDate)
{
	UUtError			error;
	
	char*				textureDirectoryName;
	BFtFileRef*			textureDirectoryRef;
	BFtFileRef*			textureFileRef;
	BFtFileIterator*	fileIterator;
	
	error = GRrGroup_GetString(inGroup, "textureDirectory", &textureDirectoryName);
	UUmError_ReturnOnErrorMsg(error, "Could not find textureDirectory name");
	
	error = BFrFileRef_MakeFromName(textureDirectoryName, &textureDirectoryRef);
	UUmError_ReturnOnErrorMsg(error, "Could not find textureDirectory");
	
	error =
		BFrDirectory_FileIterator_New(
			textureDirectoryRef,
			NULL,
			".BMP",
			&fileIterator);
	UUmError_ReturnOnErrorMsg(error, "Could not create file iterator");
	
	while(1)
	{
		const char *leafName = NULL;
		char capitalLeafName[255];

		error = 
			BFrDirectory_FileIterator_Next(
				fileIterator,
				&textureFileRef);
		if(error != UUcError_None)
		{
			break;
		}
		
		leafName = BFrFileRef_GetLeafName(textureFileRef);
		printf("%s\n\r", leafName);

		strcpy(capitalLeafName, leafName);
		iCapitalizeString(capitalLeafName);
		
		error = 	
			Imp_ProcessTexture(
				textureFileRef,
				inSourceFileModDate,
				BMPcFlagFile_Yes,
				M3cTextureFlags_HasMipMap,
				capitalLeafName);
		
		if(error != UUcError_None)
		{
			IMPrEnv_LogError("Could not process texture %s", BFrFileRef_GetLeafName(textureFileRef));
		}
		
		BFrFileRef_Dispose(textureFileRef);
	}
	
	BFrDirectory_FileIterator_Delete(fileIterator);
	
	return UUcError_None;
}

static IMPtEnv_BuildData*
IMPiEnv_BuildData_New(
	void)
{
	IMPtEnv_BuildData*	newBuildData;
	
	newBuildData = UUrMemory_Block_New(sizeof(IMPtEnv_BuildData));
	
	if(newBuildData == NULL)
	{
		return NULL;
	}
	
	newBuildData->numPRTs = 0;
	newBuildData->numPSGs = 0;
	newBuildData->numBNVs = 1;
	
	newBuildData->numVisibleQuads = 0;
	
	newBuildData->sharedPointArray = NULL;
	newBuildData->sharedPlaneEquArray = NULL;
	newBuildData->sharedQuadArray = NULL;
	newBuildData->sharedTexCoordArray = NULL;
	newBuildData->sharedEnvTexCoordArray = NULL;
	
	newBuildData->bspPointArray = NULL;
	newBuildData->bspQuadArray = NULL;
	
	newBuildData->sharedPointArray = AUrSharedPointArray_New();
	newBuildData->sharedPlaneEquArray = AUrSharedPlaneEquArray_New();
	newBuildData->sharedQuadArray = AUrSharedQuadArray_New();
	newBuildData->sharedTexCoordArray = AUrSharedTexCoordArray_New();
	newBuildData->sharedEnvTexCoordArray = AUrSharedEnvTexCoordArray_New();
	
	newBuildData->sharedPSGPlaneArray = AUrSharedPlaneEquArray_New();
	
	newBuildData->bspPointArray = AUrSharedPointArray_New();
	newBuildData->bspQuadArray = AUrSharedQuadArray_New();
	
	newBuildData->octTreeNodes = UUrMemory_Block_New(sizeof(IMPtEnv_OctTreeNode) * IMPcEnv_MaxOctTreeNodes);
	
	if(
		newBuildData->sharedPointArray == NULL ||
		newBuildData->sharedPlaneEquArray == NULL ||
		newBuildData->sharedQuadArray == NULL ||
		newBuildData->sharedTexCoordArray == NULL ||
		newBuildData->sharedEnvTexCoordArray == NULL ||
		newBuildData->bspPointArray == NULL ||
		newBuildData->bspQuadArray == NULL ||
		newBuildData->octTreeNodes == NULL)
	{
		return NULL;
	}
	
	return newBuildData;
}

static void
IMPiEnv_BuildData_Delete(
	IMPtEnv_BuildData*	inBuildData)
{
	UUmAssert(inBuildData != NULL);
	
	if(inBuildData->sharedPointArray != NULL)
	{
		AUrSharedPointArray_Delete(inBuildData->sharedPointArray);
	}
	
	if(inBuildData->sharedPlaneEquArray != NULL)
	{
		AUrSharedPlaneEquArray_Delete(inBuildData->sharedPlaneEquArray);
	}
	
	if(inBuildData->sharedQuadArray != NULL)
	{
		AUrSharedQuadArray_Delete(inBuildData->sharedQuadArray);
	}
	
	if(inBuildData->sharedTexCoordArray != NULL)
	{
		AUrSharedTexCoordArray_Delete(inBuildData->sharedTexCoordArray);
	}
	
	if(inBuildData->sharedEnvTexCoordArray != NULL)
	{
		AUrSharedEnvTexCoordArray_Delete(inBuildData->sharedEnvTexCoordArray);
	}
	
	if(inBuildData->sharedPSGPlaneArray != NULL)
	{
		AUrSharedPlaneEquArray_Delete(inBuildData->sharedPSGPlaneArray);
	}
	
	if(inBuildData->bspPointArray != NULL)
	{
		AUrSharedPointArray_Delete(inBuildData->bspPointArray);
	}
	
	if(inBuildData->bspQuadArray != NULL)
	{
		AUrSharedQuadArray_Delete(inBuildData->bspQuadArray);
	}
	
	if(inBuildData->octTreeNodes != NULL)
	{
		UUrMemory_Block_Delete(inBuildData->octTreeNodes);
	}
	
	UUrMemory_Block_Delete(inBuildData);
}

UUtError
IMPrEnv_Initialize(
	void)
{
	IMPgEnv_ErrorFile = fopen(IMPcEnv_ErrorLogFile, "w");
	if(IMPgEnv_ErrorFile == NULL)
	{
		UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not create error file");
	}
	
	return UUcError_None;
}

void
IMPrEnv_Terminate(
	void)
{
	fclose(IMPgEnv_ErrorFile);
}

UUtError IMPiEnv_CreateObjectTagArray(
	IMPtEnv_BuildData*	inBuildData )
{
	UUtError			error;
	UUtUns32			object_tag;
	UUtUns32*			tag_array;
	UUtUns32*			new_tags;
	UUtUns32			new_index;
	UUtUns32			tag_count;
	UUtUns32			i, j;
	UUtMemory_Array*	new_array;
	UUtUns32			unique_tags;

	UUmAssert( inBuildData && inBuildData->object_tags );

	new_array = UUrMemory_Array_New( sizeof(UUtUns32),  sizeof(UUtUns32) * 16, 0, 0 );
	new_tags = (UUtUns32*) UUrMemory_Array_GetMemory( new_array );
	unique_tags = 0;

	tag_count = UUrMemory_Array_GetUsedElems( inBuildData->object_tags );
	tag_array = (UUtUns32*) UUrMemory_Array_GetMemory( inBuildData->object_tags );

	for( i = 0; i <tag_count; i++ )
	{
		object_tag = tag_array[i];
		for( j = 0; j < unique_tags; j++ )
		{
			if( new_tags[j] == object_tag )
			{
				error = UUrMemory_Array_GetNewElement( new_array, &new_index, NULL );
				
				UUmAssert( error == UUcError_None );

				new_tags = (UUtUns32*) UUrMemory_Array_GetMemory( new_array );

				new_tags[new_index] = object_tag;

				unique_tags++;

				break;
			}
		}
	}
	
	// build master object tag list
	error =  TMrConstruction_Instance_Renew( TMcTemplate_IndexArray, "ObjectTags", unique_tags, &object_array );
	IMPmError_ReturnOnErrorMsg(error, "Could not create object oid");

	// grab the unique tag list
	new_tags = (UUtUns32*) UUrMemory_Array_GetMemory( new_array );
	for( i = 0; i < unique_tags; i++ )
	{
		object_array->indices[i] = new_tags[i];
	}

	return UUcError_None;
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
	UUtUns32			createDate;
	UUtUns32			compileDate;
	float				globalSlopX, globalSlopY, globalSlopZ;
	
	// 1. Allocate build data memory
		buildData = IMPiEnv_BuildData_New();
		if(buildData == NULL)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not allocate build data");
		}
	
	// 2. Check the exists and mod date
		if(
			TMrConstruction_Instance_CheckExists(
				AKcTemplate_Environment,
				inInstanceName,
				&createDate) == UUcFalse)
		{
			buildInstance = UUcTrue;
		}
		else
		{
			buildInstance = (UUtBool)(createDate < inSourceFileModDate);
		}
		
		compileDate = UUrConvertStrToSecsSince1900(IMPgEnv_CompileTime);
		
		buildInstance = buildInstance || (createDate < compileDate);
	
	// 3. Process all the textures
		printf("adding the texture maps\n\r");
		error =
			IMPiEnv_Textures_Process(
				inGroup,
				inSourceFileModDate);
		UUmError_ReturnOnError(error);

	// 4. Parse the data into our internal data structures
		error = 
			IMPrEnv_Parse(
				inSourceFileRef,
				inGroup,
				buildData,
				&buildInstance);
		UUmError_ReturnOnError(error);
	
	if(buildInstance == UUcTrue)
	{
		// 5. Process the data
			
			error = GRrGroup_GetFloat(inGroup, "globalSlopX", &globalSlopX);
			if(error != UUcError_None)
			{
				globalSlopX = 0.0f;
			}
			
			error = GRrGroup_GetFloat(inGroup, "globalSlopY", &globalSlopY);
			if(error != UUcError_None)
			{
				globalSlopY = 0.0f;
			}
			
			error = GRrGroup_GetFloat(inGroup, "globalSlopZ", &globalSlopZ);
			if(error != UUcError_None)
			{
				globalSlopZ = 0.0f;
			}
			
			error = IMPrEnv_Process(buildData, globalSlopX, globalSlopY, globalSlopZ);
			UUmError_ReturnOnError(error);
		
		// 6. Create the final instance
			if(1 || IMPgEnv_Errors == UUcFalse)
			{
				error = IMPrEnv_CreateInstance(inInstanceName, buildData);
				UUmError_ReturnOnError(error);
			}
			else
			{
				fprintf(stderr, "errors occured while processing environment, not creating instance\n");
				return UUcError_Generic;
			}
	}
	// 7. Create the gunked object tag list
	IMPiEnv_CreateObjectTagArray(buildData);
	
	// 8. Delete the internal data structures
	IMPiEnv_BuildData_Delete(buildData);
	
	return UUcError_None;
}

UUtError
IMPrEnv_Add_Debug(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;
	
	IMPtEnv_BuildData*	buildData;
	
	UUtBool				buildInstance;
	UUtUns32			createDate;
	UUtUns32			compileDate;
	
	float				globalSlopX, globalSlopY, globalSlopZ;

	// 1. Allocate build data memory
		buildData = IMPiEnv_BuildData_New();
		if(buildData == NULL)
		{
			UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "Could not allocate build data");
		}
	
	// 2. Check the exists and mod date
		if(
			TMrConstruction_Instance_CheckExists(
				AKcTemplate_EnvironmentDebug,
				inInstanceName,
				&createDate) == UUcFalse)
		{
			buildInstance = UUcTrue;
		}
		else
		{
			buildInstance = (UUtBool)(createDate < inSourceFileModDate);
		}
		
		compileDate = UUrConvertStrToSecsSince1900(IMPgEnv_CompileTime);
		
		buildInstance = buildInstance || (createDate < compileDate);
	
	// 3. Process all the textures
		error =
			IMPiEnv_Textures_Process(
				inGroup,
				inSourceFileModDate);
		UUmError_ReturnOnError(error);

	// 4. Parse the data into our internal data structures
		error = 
			IMPrEnv_Parse(
				inSourceFileRef,
				inGroup,
				buildData,
				&buildInstance);
		UUmError_ReturnOnError(error);
	
	if(buildInstance == UUcTrue)
	{
		// 5. Process the data
			error = GRrGroup_GetFloat(inGroup, "globalSlopX", &globalSlopX);
			if(error != UUcError_None)
			{
				globalSlopX = 0.0f;
			}
			
			error = GRrGroup_GetFloat(inGroup, "globalSlopY", &globalSlopY);
			if(error != UUcError_None)
			{
				globalSlopY = 0.0f;
			}
			
			error = GRrGroup_GetFloat(inGroup, "globalSlopZ", &globalSlopZ);
			if(error != UUcError_None)
			{
				globalSlopZ = 0.0f;
			}
			
			error = IMPrEnv_Process(buildData, globalSlopX, globalSlopY, globalSlopZ);
			UUmError_ReturnOnError(error);
		
		// 6. Create the final instance
			error = IMPrEnv_CreateInstance_Debug(inInstanceName, buildData);
			UUmError_ReturnOnError(error);
	}

	// 7. Create the gunked object tag list
	IMPiEnv_CreateObjectTagArray(buildData);

	// 8. Delete the internal data structures
	IMPiEnv_BuildData_Delete(buildData);
	
	return UUcError_None;
}
