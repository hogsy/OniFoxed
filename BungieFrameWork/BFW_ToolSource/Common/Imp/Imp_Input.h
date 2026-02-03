/*
	FILE:	Imp_Input.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 3, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef IMP_INPUT_H
#define IMP_INPUT_H

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_LocalInput.h"

UUtError
Imp_AddInput(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

#endif /* IMP_INPUT_H */
