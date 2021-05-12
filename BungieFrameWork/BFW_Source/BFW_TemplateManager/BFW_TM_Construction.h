/*
	FILE:	BFW_TM_Construction.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 8, 1997
	
	PURPOSE: Manage data file construction
	
	Copyright 1997

*/
#ifndef BFW_TM_CONSTRUCTION_H
#define BFW_TM_CONSTRUCTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"

#define TMmPlaceHolder_IsPtr(ph)			(!((UUtUns32)ph & 0x01))

#if defined(DEBUGGING) && DEBUGGING

	void TMrConstruction_MajorCheck(
		void);
	
	#define TMmConstruction_MajorCheck()	TMrConstruction_MajorCheck()
	
#else

	#define TMmConstruction_MajorCheck()
	
#endif

UUtError
TMrConstruction_Start(
	BFtFileRef*			inInstanceFileRef);

UUtError
TMrConstruction_Stop(
	UUtBool				inDumpStuff);

UUtError
TMrConstruction_Instance_Renew(
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName,				// NULL means unique
	UUtUns32			inInitialVarArrayLength,
	void*				*outNewInstance);

void *
TMrConstruction_Raw_New(
	UUtUns32 inSize,
	UUtUns32 inAlignment,
	TMtTemplateTag inTemplateTag);

void *
TMrConstruction_Separate_New(
	UUtUns32 inSize,
	TMtTemplateTag inTemplateTag);

void *
TMrConstruction_Raw_Write(
	void *inPtr);

void *
TMrConstruction_Separate_Write(
	void *inPtr);

UUtError
TMrConstruction_Instance_GetPlaceHolder(
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName,
	TMtPlaceHolder		*outPlaceHolder);

UUtBool
TMrConstruction_Instance_CheckExists(
	TMtTemplateTag		inTemplateTag,
	const char*			inInstanceName);
	
UUtError
TMrConstruction_Instance_NewUnique(
	TMtTemplateTag		inTemplateTag,
	UUtUns32			inInitialVarArrayLength,
	void*				*outReference);			// This needs to point into the refering structure

UUtError
TMrConstruction_Instance_Keep(
	TMtTemplateTag		inTemplateTag,
	char*				inInstanceName);

UUtError
TMrConstruction_ConvertToNativeEndian(
	void);

UUtUns32
TMrConstruction_GetNumInstances(
	void);


#ifdef __cplusplus
}
#endif

#endif /* BFW_TM_CONSTRUCTION_H */
