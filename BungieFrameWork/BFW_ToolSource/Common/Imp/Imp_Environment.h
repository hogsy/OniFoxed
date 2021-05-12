/*
	FILE:	Imp_Environment.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 3, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef IMP_ENVIRONMENT_H
#define IMP_ENVIRONMENT_H

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_Group.h"

UUtError
IMPrEnv_Initialize(
	void);

void
IMPrEnv_Terminate(
	void);

UUtError
IMPrEnv_Add(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
IMPrEnv_Add_Debug(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

#endif /* IMP_ENVIRONMENT_H */
