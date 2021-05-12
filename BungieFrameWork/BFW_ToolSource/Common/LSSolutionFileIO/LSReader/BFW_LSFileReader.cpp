/*
	FILE:	BFW_LSFileReader.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Jan 1, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#include "SolutionFactory.h"

#include "BFW.h"

#include "BFW_LSSolution.h"
#include "BFW_LSBuilder.h"

#include "BFW_AppUtilities.h"

extern "C" {
#include "Imp_Common.h"
}

UUtBool	gError = UUcFalse;


UUtError
LSrData_CreateFromLSFile(
	const char*		inLSFilePath,
	LStData*	*outNewData)
{
	LStData*			newData;
	
	*outNewData = NULL;
	
	newData = (LStData*)UUrMemory_Block_New(sizeof(LStData));
	if(newData == NULL)
	{
		Imp_PrintWarning("failed to allocate ls data %d bytes", sizeof(LStData));
		return UUcError_OutOfMemory;
	}
		
	newData->num_points = 0;
	
	newData->flags = LScFlag_RGB;

	newData->bbox.maxPoint.x = -1e15f;
	newData->bbox.maxPoint.y = -1e15f;
	newData->bbox.maxPoint.z = -1e15f;

	newData->bbox.minPoint.x = 1e15f;
	newData->bbox.minPoint.y = 1e15f;
	newData->bbox.minPoint.z = 1e15f;
	
	{
		//------------------------------------------------
		//		Create the factory
		BFW_LSBuilderFactory factory_count( newData, UUcFalse );

		//------------------------------------------------
		//		Load the Solution File
		::LtSolutionImport( inLSFilePath, factory_count );
	}
	
	{
		const char *leaf_name = inLSFilePath;
		const char *leaf_name_loop = inLSFilePath;

		for(leaf_name_loop = leaf_name; *leaf_name_loop != '\0'; leaf_name_loop++)
		{
			if (('\\' == (*leaf_name_loop)) || ('/' == (*leaf_name_loop)))
			{
				leaf_name = leaf_name_loop + 1;
			}

			leaf_name_loop++;
		}

		Imp_PrintMessage(IMPcMsg_Important, "ls file %s points %d"UUmNL, leaf_name, newData->num_points);
	}

	newData->the_point_list = (LStPoint*)UUrMemory_Block_New(sizeof(LStPoint) * newData->num_points);
	if(newData->the_point_list == NULL)
	{
		Imp_PrintWarning("failed to allocated ls file point list %d = %d bytes", 
			newData->num_points,
			sizeof(M3tPoint3D) * newData->num_points);
		return UUcError_OutOfMemory;
	}
		
	newData->num_points = 0;

	{
		//------------------------------------------------
		//		Create the factory
		BFW_LSBuilderFactory factory_build( newData, UUcTrue );

		//------------------------------------------------
		//		Load the Solution File
		::LtSolutionImport( inLSFilePath, factory_build );
	}
	
	if(gError == UUcTrue) return UUcError_Generic;
		
	strcpy(newData->prepVersion, LScPrepVersion);
	
	*outNewData = newData;
	
	fprintf(stderr, "maxX = %f"UUmNL, newData->bbox.maxPoint.x);
	fprintf(stderr, "maxY = %f"UUmNL, newData->bbox.maxPoint.y);
	fprintf(stderr, "maxZ = %f"UUmNL, newData->bbox.maxPoint.z);
	fprintf(stderr, "minX = %f"UUmNL, newData->bbox.minPoint.x);
	fprintf(stderr, "minY = %f"UUmNL, newData->bbox.minPoint.y);
	fprintf(stderr, "minZ = %f"UUmNL, newData->bbox.minPoint.z);
	fprintf(stderr, "brightness = %f"UUmNL, newData->brightness);
	fprintf(stderr, "contrast = %f"UUmNL, newData->contrast);
	
	return UUcError_None;
}

