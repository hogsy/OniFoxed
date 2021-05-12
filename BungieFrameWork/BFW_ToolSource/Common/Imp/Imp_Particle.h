
/*
	FILE:	Imp_Particle.h
	
	AUTHOR:	Michael Evans
	
	CREATED: Dec 10, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/
#ifndef IMP_PARTICLE_H
#define IMP_PARTICLE_H

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_Group.h"

UUtError Imp_AddParticleClass(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
				
UUtError Imp_AddStreamClass(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError Imp_AddLocalPhysicsClass(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);
	
#endif /* IMP_PARTICLE_H */
