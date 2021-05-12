/*
	FILE:	Imp_AI2.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: April 11, 2000
	
	PURPOSE: Definitions for AI2 data importer
	
	Copyright (c) 2000 Bungie Software

*/

#ifndef IMP_AI2_H
#define IMP_AI2_H

#include "Oni_AI2.h"


UUtError Imp_AI2_ReadClassBehavior(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName,
	ONtCharacterClass	*inClass);

#endif
