/*
	FILE:	Imp_Env2_LightMaps2.c

	AUTHOR:	Brent H. Pease, Michael Evans

	CREATED: Sept 3, 1997

	PURPOSE:

	Copyright 1997, 2000

*/

#include <stdio.h>
#include <stdlib.h>

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_Motoko.h"
#include "BFW_AppUtilities.h"
#include "BFW_TM_Construction.h"
#include "BFW_MathLib.h"
#include "BFW_LSSolution.h"
#include "BFW_Collision.h"
#include "BFW_Shared_TriRaster.h"
#include "BFW_FileFormat.h"
#include "BFW_BitVector.h"
#include "BFW_Timer.h"
#include "BFW_Math.h"

#include "Imp_Env2_Private.h"
#include "Imp_Common.h"
#include "Imp_Texture.h"

#include "BFW_Shared_TriRaster.h"

/*
	texture page memory layout

	base addr
	v
	v 0      1
  0 *------> *
	|
	|
	|
  1 v


*/


#define IMPcEnv_MaxEntries	(100)

#define IMPcEnv_LM_TableMaxDim	(IMPcEnv_MaxLMDim + 3)

#define IMPcEnv_LM_SortedTableSize	(IMPcEnv_LM_TableMaxDim * IMPcEnv_LM_TableMaxDim)

#define	IMPcEnv_LM_MaxNumFillPixles	(2 * 2 * 1024 * 1024)

#define IMPcMaxLMSuperDim	(1024 * 4)
#define IMPcMaxLMDim		(1024 * 4)

typedef struct IMPtLM_FillPixel
{
	UUtUns16	x, y;

} IMPtLM_FillPixel;

typedef struct IMPtLM_SortedUVEntry
{
	UUtUns8	u;
	UUtUns8	v;

} IMPtLM_SortedUVEntry;

typedef struct IMPtLM_TexturePage_Entry
{
	UUtUns32	gqIndex;

} IMPtLM_TexturePage_Entry;

UUtUns32					gLMFillPixel_Num[2];

UUtBool						gTouched = UUcFalse;

extern UUtBool IMPgGreen;
extern UUtBool IMPgLightingFromLSFiles;





UUtUns16	IMPgLM_LastFlushedTextureMapIndex = 0;
UUtInt32	IMPgLM_TableArea = 0;
UUtUns16	IMPgLM_LastTexturePageIndex = 0;

#define LIGHTING_CACHE_VERSION 0

LStPoint *IMPgLightingPoints = NULL;
UUtUns32 IMPgLightingPointCount;
static M3tBoundingBox_MinMax IMPgLightingBBox;



static UUtError
IMPiLM_Process_LSData_Read(
	IMPtEnv_BuildData*	inBuildData,
	BFtFileRef*			inSourceFileRef)
{
	UUtError				error;
	BFtFileRef*				lsdDirectory = NULL;
	BFtFileRef*				lsDirectory = NULL;
	BFtFileRef*				lsDataFileRefs[IMPcEnv_MaxFilesInDir];

	UUtUns16				numLSDataFiles = 0;
	IMPtEnv_LSData			lsDataFiles[IMPcEnv_MaxFilesInDir];


	UUtUns16				itr;

	#if ((UUmPlatform == UUmPlatform_Win32) || (UUmPlatform == UUmPlatform_Linux)) && (UUmCompiler != UUmCompiler_MWerks)

		error =
			BFrFileRef_DuplicateAndReplaceName(
				inSourceFileRef,
				"Conversion_Files\\LS",
				&lsDirectory);

		if(error == UUcError_None && BFrFileRef_FileExists(lsDirectory))
		{
			error =
				BFrDirectory_GetFileList(
					lsDirectory,
					NULL,
					".ls",
					IMPcEnv_MaxFilesInDir,
					&numLSDataFiles,
					lsDataFileRefs);
			UUmError_ReturnOnErrorMsg(error, "Could not get LS Files");

			Imp_PrintMessage(IMPcMsg_Important, "Preprocessing LS Data files from LS directory (%d files)..."UUmNL, numLSDataFiles);
			for(itr = 0; itr < numLSDataFiles; itr++)
			{
				error =
					LSrData_CreateFromLSFile(
						BFrFileRef_GetFullPath(lsDataFileRefs[itr]),
						&lsDataFiles[itr].lsData);
				UUmError_ReturnOnError(error);

				if(strcmp(lsDataFiles[itr].lsData->prepVersion, LScPrepVersion))
				{
					UUmError_ReturnOnErrorMsg(UUcError_Generic, "LS preperation version is wrong");
				}

				UUrString_Copy(lsDataFiles[itr].fileName, BFrFileRef_GetLeafName(lsDataFileRefs[itr]), BFcMaxFileNameLength);

				BFrFileRef_Dispose(lsDataFileRefs[itr]);
			}

			BFrFileRef_Dispose(lsDirectory);
		}
		else

	#endif

	// ok, turn our lighting files into a list of lighting points

	//LStPoint *IMPgLightingPoints;
	//UUtUns32 IMPgLightingPointCount;

	{
		UUtUns32 curBFDFileIndex;

		for(curBFDFileIndex = 0; curBFDFileIndex < numLSDataFiles;	curBFDFileIndex++)
		{
			LStData* lsData = lsDataFiles[curBFDFileIndex].lsData;

			IMPgLightingBBox.minPoint.x = UUmMin(IMPgLightingBBox.minPoint.x, lsData->bbox.minPoint.x);
			IMPgLightingBBox.minPoint.y = UUmMin(IMPgLightingBBox.minPoint.y, lsData->bbox.minPoint.y);
			IMPgLightingBBox.minPoint.z = UUmMin(IMPgLightingBBox.minPoint.z, lsData->bbox.minPoint.z);

			IMPgLightingBBox.maxPoint.x = UUmMax(IMPgLightingBBox.maxPoint.x, lsData->bbox.maxPoint.x);
			IMPgLightingBBox.maxPoint.y = UUmMax(IMPgLightingBBox.maxPoint.y, lsData->bbox.maxPoint.y);
			IMPgLightingBBox.maxPoint.z = UUmMax(IMPgLightingBBox.maxPoint.z, lsData->bbox.maxPoint.z);
		}
	}

	{
		UUtUns32 curBFDFileIndex;

		IMPgLightingPointCount = 0;

		for(curBFDFileIndex = 0; curBFDFileIndex < numLSDataFiles;	curBFDFileIndex++)
		{
			LStData* lsData = lsDataFiles[curBFDFileIndex].lsData;

			IMPgLightingPointCount += lsData->num_points;
		}
	}

	IMPgLightingPoints = UUrMemory_Block_New(sizeof(LStPoint) * IMPgLightingPointCount);

	{
		UUtUns32 curBFDFileIndex;
		UUtUns32 points_read_so_far = 0;

		for(curBFDFileIndex = 0; curBFDFileIndex < numLSDataFiles;	curBFDFileIndex++)
		{
			LStData* lsData = lsDataFiles[curBFDFileIndex].lsData;

			UUrMemory_MoveFast(lsData->the_point_list, IMPgLightingPoints + points_read_so_far, sizeof(LStPoint) * lsData->num_points);

			points_read_so_far += lsData->num_points;
		}
	}

	for(itr = 0; itr < numLSDataFiles; itr++)
	{
		LSrData_Delete(lsDataFiles[itr].lsData);
	}

	return UUcError_None;
}

static UUtError
IMPiLM_Process_LS_Cache_Data_Read(
	IMPtEnv_BuildData*	inBuildData,
	BFtFileRef*			inSourceFileRef)
{
	UUtError				error = UUcError_None;
	BFtFileRef				lighting_file_ref;

	lighting_file_ref = *inSourceFileRef;
	error = BFrFileRef_SetName(&lighting_file_ref, "Conversion_Files\\LS\\lighting.dat");

	if ((error == UUcError_None) && BFrFileRef_FileExists(&lighting_file_ref))
	{
		UUtUns32 version;
		M3tBoundingBox_MinMax bbox;
		UUtUns32 point_size;
		BFtFile *lighting_file;

		error = BFrFile_Open(&lighting_file_ref, "r", &lighting_file);
		UUmError_ReturnOnErrorMsg(error, "failed to open ls cache file");

		error = BFrFile_Read(lighting_file, sizeof(UUtUns32), &version);
		UUmError_ReturnOnErrorMsg(error, "failed to read ls cache file version");
		UUmSwapLittle_4Byte(&version);

		if (LIGHTING_CACHE_VERSION != version) {
			Imp_PrintWarning("failed to read lighting.dat");

			error = UUcError_Generic;
			goto exit;
		}

		error = BFrFile_Read(lighting_file, sizeof(bbox), &bbox);
		UUmError_ReturnOnErrorMsg(error, "failed to read ls cache file bbox");
		UUmSwapLittle_4Byte(&bbox.minPoint.x);
		UUmSwapLittle_4Byte(&bbox.minPoint.y);
		UUmSwapLittle_4Byte(&bbox.minPoint.z);
		UUmSwapLittle_4Byte(&bbox.maxPoint.x);
		UUmSwapLittle_4Byte(&bbox.maxPoint.y);
		UUmSwapLittle_4Byte(&bbox.maxPoint.z);

		error = BFrFile_Read(lighting_file, sizeof(IMPgLightingPointCount), &IMPgLightingPointCount);
		UUmError_ReturnOnErrorMsg(error, "failed to read ls cache file count");
		UUmSwapLittle_4Byte(&IMPgLightingPointCount);

		Imp_PrintMessage(IMPcMsg_Important, "cache file found %d points", IMPgLightingPointCount);

		point_size = IMPgLightingPointCount * sizeof(LStPoint);

		IMPgLightingPoints = UUrMemory_Block_New(point_size);

		error = BFrFile_Read(lighting_file, point_size, IMPgLightingPoints);
		UUmError_ReturnOnErrorMsg(error, "failed to read ls cache file lighting points");

		{
			UUtUns32 swap_itr;

			for(swap_itr = 0; swap_itr < IMPgLightingPointCount; swap_itr++)
			{
				LStPoint *current_swap_point = IMPgLightingPoints + swap_itr;

				UUmSwapLittle_4Byte(&current_swap_point->location.x);
				UUmSwapLittle_4Byte(&current_swap_point->location.y);
				UUmSwapLittle_4Byte(&current_swap_point->location.z);

				UUmSwapLittle_4Byte(&current_swap_point->normal.x);
				UUmSwapLittle_4Byte(&current_swap_point->normal.y);
				UUmSwapLittle_4Byte(&current_swap_point->normal.z);

				UUmSwapLittle_4Byte(&current_swap_point->shade);
				UUmSwapLittle_4Byte(&current_swap_point->used);
			}
		}

		BFrFile_Close(lighting_file);

		IMPgLightingBBox.minPoint.x = UUmMin(IMPgLightingBBox.minPoint.x, bbox.minPoint.x);
		IMPgLightingBBox.minPoint.y = UUmMin(IMPgLightingBBox.minPoint.y, bbox.minPoint.y);
		IMPgLightingBBox.minPoint.z = UUmMin(IMPgLightingBBox.minPoint.z, bbox.minPoint.z);

		IMPgLightingBBox.maxPoint.x = UUmMax(IMPgLightingBBox.maxPoint.x, bbox.maxPoint.x);
		IMPgLightingBBox.maxPoint.y = UUmMax(IMPgLightingBBox.maxPoint.y, bbox.maxPoint.y);
		IMPgLightingBBox.maxPoint.z = UUmMax(IMPgLightingBBox.maxPoint.z, bbox.maxPoint.z);
	}


exit:
	return error;
}

static UUtError
IMPiLM_Process_LS_Cache_Data_Write(
	IMPtEnv_BuildData*	inBuildData,
	BFtFileRef*			inSourceFileRef)
{
	UUtError				error = UUcError_None;
	BFtFileRef				lighting_file_ref;

	lighting_file_ref = *inSourceFileRef;
	error = BFrFileRef_SetName(&lighting_file_ref, "Conversion_Files\\LS\\lighting.dat");

	{
		UUtUns32 number_of_points_to_write;
		BFtFile *lighting_file;

		error = BFrFile_Open(&lighting_file_ref, "w", &lighting_file);
		UUmError_ReturnOnError(error);

		{
			UUtUns32 version = LIGHTING_CACHE_VERSION;

			error = BFrFile_Write(lighting_file, sizeof(UUtUns32), &version);
			UUmError_ReturnOnError(error);
		}

		error = BFrFile_Write(lighting_file, sizeof(IMPgLightingBBox), &IMPgLightingBBox);
		UUmError_ReturnOnError(error);

		{
			UUtUns32 itr;

			number_of_points_to_write = 0;

			for(itr = 0; itr < IMPgLightingPointCount; itr++)
			{
				if (IMPgLightingPoints[itr].used) {
					number_of_points_to_write++;
				}
			}
		}

		Imp_PrintMessage(IMPcMsg_Important, "writing cache file %d points", number_of_points_to_write);

		error = BFrFile_Write(lighting_file, sizeof(number_of_points_to_write), &number_of_points_to_write);
		UUmError_ReturnOnError(error);

		{
			UUtUns32 itr;

			for(itr = 0; itr < IMPgLightingPointCount; itr++)
			{
				if (IMPgLightingPoints[itr].used) {
					BFrFile_Write(lighting_file, sizeof(LStPoint), IMPgLightingPoints + itr);
				}
			}
		}

		BFrFile_Close(lighting_file);
	}

	return error;
}



static UUtUns32 AKgLightingFailure = 0;
static UUtUns32 AKgLightingSecondPass = 0;

/*

  constructs a fixed node size oct tree for the lighting

 */

#define LIGHTING_OCT_TREE_SIZE 32

static LStPoint **lighting_oct_tree[LIGHTING_OCT_TREE_SIZE][LIGHTING_OCT_TREE_SIZE][LIGHTING_OCT_TREE_SIZE] = { NULL };
static UUtUns32 lighting_oct_tree_size[LIGHTING_OCT_TREE_SIZE][LIGHTING_OCT_TREE_SIZE][LIGHTING_OCT_TREE_SIZE] = { 0 };
static M3tBoundingBox_MinMax lighting_bbox;

static void BuildLightingOctTree(IMPtEnv_BuildData*	inBuildData)
{
	float x_width_factor, y_width_factor, z_width_factor;
	const float close_enough_distance = 20.f;

	// step 1: calculate the bounds for the tree
	// step 2: insert points into bins, growing as we go

	if (NULL == IMPgLightingPoints) {
		goto exit;
	}

	// step 1
	lighting_bbox = IMPgLightingBBox;

	lighting_bbox.minPoint.x -= close_enough_distance;
	lighting_bbox.minPoint.y -= close_enough_distance;
	lighting_bbox.minPoint.z -= close_enough_distance;

	lighting_bbox.maxPoint.x += close_enough_distance;
	lighting_bbox.maxPoint.y += close_enough_distance;
	lighting_bbox.maxPoint.z += close_enough_distance;

	x_width_factor = ((float) LIGHTING_OCT_TREE_SIZE) / (lighting_bbox.maxPoint.x - lighting_bbox.minPoint.x);
	y_width_factor = ((float) LIGHTING_OCT_TREE_SIZE) / (lighting_bbox.maxPoint.y - lighting_bbox.minPoint.y);
	z_width_factor = ((float) LIGHTING_OCT_TREE_SIZE) / (lighting_bbox.maxPoint.z - lighting_bbox.minPoint.z);

	// step 2
	{
		UUtUns32 itr;

		for(itr = 0; itr < IMPgLightingPointCount; itr++)
		{
			UUtInt32 x,y,z;
			LStPoint *current_point = IMPgLightingPoints + itr;
			UUtInt32 start_x = (UUtInt32) ((current_point->location.x - lighting_bbox.minPoint.x - close_enough_distance) * x_width_factor);
			UUtInt32 start_y = (UUtInt32) ((current_point->location.y - lighting_bbox.minPoint.y - close_enough_distance) * y_width_factor);
			UUtInt32 start_z = (UUtInt32) ((current_point->location.z - lighting_bbox.minPoint.z - close_enough_distance) * z_width_factor);
			UUtInt32 end_x   = (UUtInt32) ((current_point->location.x - lighting_bbox.minPoint.x + close_enough_distance) * x_width_factor);
			UUtInt32 end_y   = (UUtInt32) ((current_point->location.y - lighting_bbox.minPoint.y + close_enough_distance) * y_width_factor);
			UUtInt32 end_z   = (UUtInt32) ((current_point->location.z - lighting_bbox.minPoint.z + close_enough_distance) * z_width_factor);

			start_x = UUmPin(start_x, 0, (LIGHTING_OCT_TREE_SIZE - 1));
			start_y = UUmPin(start_y, 0, (LIGHTING_OCT_TREE_SIZE - 1));
			start_z = UUmPin(start_z, 0, (LIGHTING_OCT_TREE_SIZE - 1));
			end_x = UUmPin(end_x, 0, (LIGHTING_OCT_TREE_SIZE - 1));
			end_y = UUmPin(end_y, 0, (LIGHTING_OCT_TREE_SIZE - 1));
			end_z = UUmPin(end_z, 0, (LIGHTING_OCT_TREE_SIZE - 1));

			// place us into the appropriate cells
			for(z = start_z; z <= end_z; z++)
			{
				for(y = start_y; y <= end_y; y++)
				{
					for(x = start_x; x <= end_x; x++)
					{
						UUtUns32 chunk_size = 512;
						UUtUns32 old_size = lighting_oct_tree_size[x][y][z];
						LStPoint **list = lighting_oct_tree[x][y][z];

						if (0 == (old_size % chunk_size)) {
							list = UUrMemory_Block_Realloc(list, sizeof(LStPoint *) * (chunk_size + old_size));
							lighting_oct_tree[x][y][z] = list;
						}

						list[old_size] = current_point;

						lighting_oct_tree_size[x][y][z] = old_size + 1;
					}
				}
			}
		}
	}

exit:
	return;
}

static void FreeLightingOctTree(void)
{
	UUtInt32 x,y,z;

	for(z = 0; z < LIGHTING_OCT_TREE_SIZE; z++)
	{
		for(y = 0; y < LIGHTING_OCT_TREE_SIZE; y++)
		{
			for(x = 0; x < LIGHTING_OCT_TREE_SIZE; x++)
			{
				lighting_oct_tree[x][y][z] = UUrMemory_Block_Realloc(lighting_oct_tree[x][y][z], 0);
				lighting_oct_tree_size[x][y][z] = 0;
			}
		}
	}

	return;
}

static UUtUns32 LightingOctTree_GetCount(const M3tPoint3D *inPoint)
{
	UUtUns32 count = 0;
	float x_width_factor = ((float) LIGHTING_OCT_TREE_SIZE) / (lighting_bbox.maxPoint.x - lighting_bbox.minPoint.x);
	float y_width_factor = ((float) LIGHTING_OCT_TREE_SIZE) / (lighting_bbox.maxPoint.y - lighting_bbox.minPoint.y);
	float z_width_factor = ((float) LIGHTING_OCT_TREE_SIZE) / (lighting_bbox.maxPoint.z - lighting_bbox.minPoint.z);
	UUtInt32 x = (UUtInt32) ((inPoint->x - lighting_bbox.minPoint.x) * x_width_factor);
	UUtInt32 y = (UUtInt32) ((inPoint->y - lighting_bbox.minPoint.y) * y_width_factor);
	UUtInt32 z = (UUtInt32) ((inPoint->z - lighting_bbox.minPoint.z) * z_width_factor);

	if (inPoint->x < lighting_bbox.minPoint.x) {
		goto exit;
	}

	if (inPoint->y < lighting_bbox.minPoint.y) {
		goto exit;
	}

	if (inPoint->z < lighting_bbox.minPoint.z) {
		goto exit;
	}

	if (inPoint->x > lighting_bbox.maxPoint.x) {
		goto exit;
	}

	if (inPoint->y > lighting_bbox.maxPoint.y) {
		goto exit;
	}

	if (inPoint->z > lighting_bbox.maxPoint.z) {
		goto exit;
	}

	x = UUmPin(x, 0, (LIGHTING_OCT_TREE_SIZE - 1));
	y = UUmPin(y, 0, (LIGHTING_OCT_TREE_SIZE - 1));
	z = UUmPin(z, 0, (LIGHTING_OCT_TREE_SIZE - 1));

	count = lighting_oct_tree_size[x][y][z];

exit:
	return count;
}

static LStPoint **LightingOctTree_GetList(const M3tPoint3D *inPoint)
{
	LStPoint **list = NULL;
	float x_width_factor = ((float) LIGHTING_OCT_TREE_SIZE) / (lighting_bbox.maxPoint.x - lighting_bbox.minPoint.x);
	float y_width_factor = ((float) LIGHTING_OCT_TREE_SIZE) / (lighting_bbox.maxPoint.y - lighting_bbox.minPoint.y);
	float z_width_factor = ((float) LIGHTING_OCT_TREE_SIZE) / (lighting_bbox.maxPoint.z - lighting_bbox.minPoint.z);
	UUtInt32 x = (UUtInt32) ((inPoint->x - lighting_bbox.minPoint.x) * x_width_factor);
	UUtInt32 y = (UUtInt32) ((inPoint->y - lighting_bbox.minPoint.y) * y_width_factor);
	UUtInt32 z = (UUtInt32) ((inPoint->z - lighting_bbox.minPoint.z) * z_width_factor);

	if (inPoint->x < lighting_bbox.minPoint.x) {
		goto exit;
	}

	if (inPoint->y < lighting_bbox.minPoint.y) {
		goto exit;
	}

	if (inPoint->z < lighting_bbox.minPoint.z) {
		goto exit;
	}

	if (inPoint->x > lighting_bbox.maxPoint.x) {
		goto exit;
	}

	if (inPoint->y > lighting_bbox.maxPoint.y) {
		goto exit;
	}

	if (inPoint->z > lighting_bbox.maxPoint.z) {
		goto exit;
	}

	x = UUmPin(x, 0, (LIGHTING_OCT_TREE_SIZE - 1));
	y = UUmPin(y, 0, (LIGHTING_OCT_TREE_SIZE - 1));
	z = UUmPin(z, 0, (LIGHTING_OCT_TREE_SIZE - 1));

	list = lighting_oct_tree[x][y][z];

exit:
	return list;
}

static UUtUns32
IMPiLightEnvironmentVertex(
	IMPtEnv_BuildData*		inBuildData,
	const M3tPoint3D*		inLocation,
	const M3tVector3D*		inNormal,
	UUtBool					inApproximate)
{
	UUtBool				hit = UUcFalse;
	M3tPoint3D			*sharedPoints = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	M3tPlaneEquation	*sharedPlanes = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);
	UUtUns32 default_vertex_lighting = (IMPgGreen) ? IMPcEnv_DefaultVertexLighting_Green : IMPcEnv_DefaultVertexLighting_Normal;
	UUtBool got_lighting = UUcFalse;
	UUtUns32 shade = default_vertex_lighting;
	UUtInt32 approximation_to_beat = 0xff;
	float acceptable_angle = (float) MUrCos((45.f / 360.f) * M3c2Pi);

	LStPoint **point_list;
	LStPoint **current_point_in_list;
	LStPoint **max_point;
	UUtUns32 point_count;

	point_list = LightingOctTree_GetList(inLocation);
	point_count = LightingOctTree_GetCount(inLocation);

	max_point = point_list + point_count;

	for(current_point_in_list = point_list; current_point_in_list < max_point; current_point_in_list++)
	{
		LStPoint *current_point = *current_point_in_list;
		UUtInt32 dot_product_approximation = 0;
		UUtInt32 approximation = 0;

		{
			float dot_product;

			dot_product = MUrVector_DotProduct(&current_point->normal, inNormal);

			if (inApproximate) {
				if (dot_product < 0) {
					continue;
				}

				if (dot_product > acceptable_angle) {
					dot_product_approximation = 0;
				}
				else {
					UUmAssert(dot_product <= (1.f+UUcEpsilon));

					dot_product_approximation = MUrFloat_Round_To_Int(25 * (acceptable_angle - dot_product));

					UUmAssert((dot_product_approximation >= 0) && (dot_product_approximation <= 25));
				}
			}
			else {
				if (dot_product < acceptable_angle) {
					continue;
				}
			}
		}

		{
			float distance_squared = MUrPoint_Distance_Squared(inLocation, &current_point->location);

			if (inApproximate) {
				float max_approximate_distance = 20.f;

				if (distance_squared > UUmSQR(max_approximate_distance)) {
					continue;
				}
				else {
					approximation = dot_product_approximation;
					approximation += MUrFloat_Round_To_Int(MUrSqrt(distance_squared) * (100 / max_approximate_distance));

					UUmAssert((approximation >= 0) && (approximation <= 200));
				}
			}
			else {
				if (distance_squared > 1.f) {
					continue;
				}
			}
		}

		if (approximation == approximation_to_beat) {
			shade = current_point->shade;

			shade = (shade & 0xFFFFFF00) | UUmMax((shade & 0x0000FF), (current_point->shade & 0x0000FF));
			shade = (shade & 0xFFFF00FF) | UUmMax((shade & 0x00FF00), (current_point->shade & 0x00FF00));
			shade = (shade & 0xFF00FFFF) | UUmMax((shade & 0xFF0000), (current_point->shade & 0xFF0000));

			current_point->used = UUcTrue;
		}
		else if (approximation < approximation_to_beat) {
			shade = current_point->shade;
			shade &= 0x00FFFFFF;
			shade |= (approximation << 24);

			approximation_to_beat = approximation;
			current_point->used = UUcTrue;
		}
	}

	if (0xff == approximation_to_beat) {
		if (inApproximate || !IMPgLightMap_HighQuality) {
			static UUtBool once = UUcTrue;

			AKgLightingFailure++;

			if (once) {
				Imp_PrintMessage(IMPcMsg_Important, "failed to light a gq");
				once = UUcFalse;
			}
		}
		else {
			AKgLightingSecondPass++;
			shade = IMPiLightEnvironmentVertex(inBuildData, inLocation, inNormal, UUcTrue);
		}
	}

	return shade;
}


static UUtError
IMPiLM_Process(
	IMPtEnv_BuildData*	inBuildData,
	BFtFileRef*			inSourceFileRef)
{
	UUtError				error;
	UUtUns16				itr;
	M3tPoint3D			*sharedPoints = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	M3tPlaneEquation	*sharedPlanes = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);
	UUtUns32 default_vertex_lighting = (IMPgGreen) ? IMPcEnv_DefaultVertexLighting_Green : IMPcEnv_DefaultVertexLighting_Normal;

	// ok, here is where we read the 'cache' version or the real version
	IMPgLightingBBox.minPoint.x = inBuildData->minX;
	IMPgLightingBBox.maxPoint.x = inBuildData->maxX;
	IMPgLightingBBox.minPoint.y = inBuildData->minY;
	IMPgLightingBBox.maxPoint.y = inBuildData->maxY;
	IMPgLightingBBox.minPoint.z = inBuildData->minZ;
	IMPgLightingBBox.maxPoint.z = inBuildData->maxZ;

	if (IMPgLightingFromLSFiles) {
		Imp_PrintMessage(IMPcMsg_Important, "\treading ls file (vertex lighting)"UUmNL);
		error = IMPiLM_Process_LSData_Read(inBuildData, inSourceFileRef);
		UUmError_ReturnOnError(error);
	}
	else {
		Imp_PrintMessage(IMPcMsg_Important, "\treading ls cache files (vertex lighting)"UUmNL);
		error = IMPiLM_Process_LS_Cache_Data_Read(inBuildData, inSourceFileRef);
		UUmError_ReturnOnError(error);
	}

	Imp_PrintMessage(IMPcMsg_Important, "\tread %d lighting points"UUmNL, IMPgLightingPointCount);


	Imp_PrintMessage(IMPcMsg_Important, "\tbuilding lighting oct tree"UUmNL);
	BuildLightingOctTree(inBuildData);

	Imp_PrintMessage(IMPcMsg_Important, "\tprocessing vertex lighting");

	AKgLightingFailure = 0;

	Imp_PrintMessage(IMPcMsg_Important, "%d gqs:", inBuildData->numGQs);


	// Loop over each gq
	for(itr = 0; itr < inBuildData->numGQs; itr++)
	{
		IMPtEnv_GQ *current_gq = inBuildData->gqList + itr;

		if ((itr % 100) == 0) {
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}

		if ((itr % 1000) == 0) {
			Imp_PrintMessage(IMPcMsg_Important, " %d%% [%d fail %d 2nd pass]", (100 * itr) / inBuildData->numGQs, AKgLightingFailure, AKgLightingSecondPass);
		}

		{

			UUtUns32 vertex_itr;
			UUtUns32 vertex_count = (current_gq->flags & AKcGQ_Flag_Triangle) ? 3 : 4;
			M3tVector3D face_normal;
			float d;

			AKmPlaneEqu_GetComponents(current_gq->planeEquIndex, sharedPlanes, face_normal.x, face_normal.y, face_normal.z, d);

			current_gq->shade[3] = default_vertex_lighting;

			for(vertex_itr = 0; vertex_itr < vertex_count; vertex_itr++)
			{
				const M3tPoint3D *vertex_location = sharedPoints + current_gq->visibleQuad.indices[vertex_itr];

				current_gq->shade[vertex_itr] = IMPiLightEnvironmentVertex(inBuildData, vertex_location, &face_normal, UUcFalse);
			}
		}
	}

	FreeLightingOctTree();

	if (0 != AKgLightingFailure) {
		Imp_PrintMessage(IMPcMsg_Important, "failed to light %d gqs", AKgLightingFailure);
	}

	// clear lighting data
	if (IMPgLightingFromLSFiles) {
		IMPiLM_Process_LS_Cache_Data_Write(inBuildData, inSourceFileRef);
	}

	IMPgLightingPoints = UUrMemory_Block_Realloc(IMPgLightingPoints, 0);
	IMPgLightingPointCount = 0;

	return UUcError_None;
}

UUtBool
IMPrLM_DontProcessGQ(
	IMPtEnv_GQ*	inGQ)
{
	if(inGQ->lmFlags & IMPcGQ_LM_ForceOn) return UUcFalse;
	if(inGQ->lmFlags & IMPcGQ_LM_ForceOff) return UUcTrue;
	if(inGQ->isLuminaire) return UUcTrue;
	if(inGQ->flags & AKcGQ_Flag_DontLight) return UUcTrue;

	return UUcFalse;
}

UUtError
IMPrEnv_Process_LightMap(
	IMPtEnv_BuildData*	inBuildData,
	BFtFileRef*			inSourceFileRef)
{
	UUtUns32	dotCount = 0;
	UUtInt64	time;

	time = UUrMachineTime_High();

	Imp_PrintMessage(IMPcMsg_Important, "preparing to compute lightmaps");

	IMPgLM_LastFlushedTextureMapIndex = 0;
	IMPgLM_TableArea = 0;

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	Imp_PrintMessage(IMPcMsg_Important, "lightmaps");
	time = UUrMachineTime_High();

	if(IMPgLightmaps)
	{
		if(IMPgLightmap_OutputPrepFile)
		{
			UUtError error;

			#if ((UUmPlatform == UUmPlatform_Win32) || (UUmPlatform == UUmPlatform_Linux)) && (UUmCompiler != UUmCompiler_MWerks)
			{
				BFtFileRef*		lpOutputDir;
				BFtFileRef*		curLPFileRef;
				UUtUns16		curFileIndex;

				error =
					BFrFileRef_DuplicateAndReplaceName(
						inSourceFileRef,
						"Conversion_Files\\LP",
						&lpOutputDir);
				UUmError_ReturnOnErrorMsg(error, "Could not locate LP directory");

				if(!BFrFileRef_FileExists(lpOutputDir))
				{
					error = BFrDirectory_Create(lpOutputDir, NULL);
					UUmError_ReturnOnErrorMsg(error, "Could not create LP directory");
				}
				else
				{
					BFrDirectory_DeleteContentsOnly(lpOutputDir);
				}

				if(IMPgLightmap_OutputPrepFileOne)
				{
					char			tempName[BFcMaxFileNameLength];
					char			allOneFileName[BFcMaxFileNameLength];
					char*			cp;

					sprintf(tempName, "%s", BFrFileRef_GetLeafName(inSourceFileRef));

					cp = strchr(tempName, '.');
					if(cp != NULL) *cp = 0;

					sprintf(allOneFileName, "%s_lp1.lp", tempName);

					error =
						BFrFileRef_DuplicateAndAppendName(
							lpOutputDir,
							allOneFileName,
							&curLPFileRef);
					UUmError_ReturnOnErrorMsg(error, "Could not locate LP directory");

					// create the LP file
					error =
						LSrPreperationFile_Create(
							inBuildData,
							NULL,
							BFrFileRef_GetFullPath(curLPFileRef));
					UUmError_ReturnOnError(error);

					BFrFileRef_Dispose(curLPFileRef);
				}
				else
				{
					for(curFileIndex = 0; curFileIndex < inBuildData->numGQFiles; curFileIndex++)
					{
						error =
							BFrFileRef_DuplicateAndAppendName(
								lpOutputDir,
								inBuildData->gqFileNames[curFileIndex],
								&curLPFileRef);
						UUmError_ReturnOnErrorMsg(error, "Could not locate LP directory");

						BFrFileRef_SetLeafNameSuffex(curLPFileRef, "lp");

						// create the LP file
						error =
							LSrPreperationFile_Create(
								inBuildData,
								inBuildData->gqFileNames[curFileIndex],
								BFrFileRef_GetFullPath(curLPFileRef));
						UUmError_ReturnOnError(error);

						BFrFileRef_Dispose(curLPFileRef);
					}
				}

				BFrFileRef_Dispose(lpOutputDir);
			}
			#endif
		}
		else {
			IMPiLM_Process(inBuildData, inSourceFileRef);
		}
	}

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	return UUcError_None;
}
