/*
	FILE:	BFW_LSSolution.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Jan 1, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#include "BFW.h"
#include "BFW_FileManager.h"

#include "BFW_LSSolution.h"

#define LScEndianDetector	UUm4CharToUns32('S', 'P', 'E', 'D')

UUtError
LSrData_WriteBinary(
	BFtFileRef*		inFileRef,
	LStData*		inData)
{
	UUtError	error;
	BFtFile*	file;
	UUtUns32	endianDetector;

	if(BFrFileRef_FileExists(inFileRef) == UUcFalse)
	{
		error = BFrFile_Create(inFileRef);
		UUmError_ReturnOnError(error);
	}

	error = BFrFile_Open(inFileRef, "w", &file);
	UUmError_ReturnOnError(error);
	
	// Write the endian detector
		endianDetector = LScEndianDetector;

		error = BFrFile_Write(file, 4, &endianDetector);
		UUmError_ReturnOnError(error);

		error = BFrFile_Write(file, LScPrepVersion_Length, inData->prepVersion);
		UUmError_ReturnOnError(error);

		error = BFrFile_Write(file, 4, &inData->num_points);
		UUmError_ReturnOnError(error);

		error = BFrFile_Write(file, 4, &inData->brightness);
		UUmError_ReturnOnError(error);

		error = BFrFile_Write(file, 4, &inData->contrast);
		UUmError_ReturnOnError(error);

		error = BFrFile_Write(file, 4, &inData->flags);
		UUmError_ReturnOnError(error);

	// Write the point array
		error =	BFrFile_Write(file, inData->num_points * sizeof(LStPoint), inData->the_point_list);
		UUmError_ReturnOnError(error);
		
	BFrFile_Close(file);

	return UUcError_None;
}

UUtError
LSrData_ReadBinary(
	BFtFileRef*		inFileRef,
	LStData*		*outNewData)
{
	UUtError			error;
	LStData*			newData;
	UUtUns32			curFilePos = 0;
	BFtFile*			file;
	UUtBool				needsSwapping;
	UUtUns32			endianDetector;
	UUtUns32			i;
	LStPoint*			curPoint;
	
	error = BFrFile_Open(inFileRef, "r", &file);
	UUmError_ReturnOnError(error);

	newData = (LStData*)UUrMemory_Block_New(sizeof(LStData));
	if(newData == NULL)
	{
		return UUcError_OutOfMemory;
	}
	
	// Read the endian detector
		error = BFrFile_Read(file, 4, &endianDetector);
		UUmError_ReturnOnError(error);

		needsSwapping = endianDetector != LScEndianDetector;

		if(needsSwapping == UUcTrue)
		{
			UUrSwap_4Byte(&endianDetector);

			if(endianDetector != LScEndianDetector)
			{
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "ilegal file type");
			}
		}
	
		error = BFrFile_ReadPos(file, curFilePos, LScPrepVersion_Length, newData->prepVersion);
		UUmError_ReturnOnError(error);
		curFilePos += LScPrepVersion_Length;
		
		if(strcmp(newData->prepVersion, LScPrepVersion)) return UUcError_Generic;
		
	// Read the number of points and faces
	error = BFrFile_Read(file, 4, &newData->num_points);
	UUmError_ReturnOnError(error);

	if(needsSwapping)
	{
		UUrSwap_4Byte(&newData->num_points);
	}

	error = BFrFile_Read(file, 4, &newData->brightness);
	UUmError_ReturnOnError(error);

	if(needsSwapping)
	{
		UUrSwap_4Byte(&newData->brightness);
	}
	
	error = BFrFile_Read(file, 4, &newData->contrast);
	UUmError_ReturnOnError(error);

	if(needsSwapping)
	{
		UUrSwap_4Byte(&newData->contrast);
	}
	
	error = BFrFile_Read(file, 4, &newData->flags);
	UUmError_ReturnOnError(error);

	if(needsSwapping)
	{
		UUrSwap_4Byte(&newData->flags);
	}
		
	// Allocate memory for the points and faces
	newData->the_point_list = UUrMemory_Block_New(newData->num_points * sizeof(LStPoint));
	if(newData->the_point_list == NULL)
	{
		UUmError_ReturnOnError(UUcError_OutOfMemory);
	}

	// Read in the point and face data
	error =
		BFrFile_Read(
			file,
			newData->num_points * sizeof(LStPoint),
			newData->the_point_list);
	UUmError_ReturnOnError(error);

	// Swap data if needed
	if(needsSwapping)
	{
		for(i = 0, curPoint = newData->the_point_list; i < newData->num_points; i++, curPoint++)
		{
			UUrSwap_4Byte(&curPoint->location.x);
			UUrSwap_4Byte(&curPoint->location.y);
			UUrSwap_4Byte(&curPoint->location.z);

			UUrSwap_4Byte(&curPoint->normal.x);
			UUrSwap_4Byte(&curPoint->normal.y);
			UUrSwap_4Byte(&curPoint->normal.z);

			UUrSwap_4Byte(&curPoint->shade);
			UUrSwap_4Byte(&curPoint->used);
		}
	}

	BFrFile_Close(file);
	
	*outNewData = newData;

	return UUcError_None;
}

UUtUns32
LSrData_GetSize(
	LStData*		inData)
{
	return sizeof(UUtUns32) * 4 + 
			inData->num_points * sizeof(LStPoint);
}

void
LSrData_Delete(
	LStData*	inData)
{
	UUrMemory_Block_Delete(inData->the_point_list);
	UUrMemory_Block_Delete(inData);
}

