#pragma once

#ifndef __BFW_ENVPARTICLE_H__
#define __BFW_ENVPARTICLE_H__

/*
	FILE:	BFW_EnvParticle.h
	
	AUTHOR:	Chris Butcher
	
	CREATED: May 26, 2000
	
	PURPOSE: support for environmental particles
	
	Copyright (c) 2000, Bungie Software

 */

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"

// =============================================
// constants
// =============================================

#define EPcParticleTagNameLength		47

enum {
	EPcParticleOwner_StaticEnv			= 0,
	EPcParticleOwner_Object				= 1,		// an OBtObject
	EPcParticleOwner_InEngine			= 2,		// an OBJtOSD_Particle
	EPcParticleOwner_Furniture			= 3,		// an OBJtOSD_Furniture
	EPcParticleOwner_Max				= 4
};

enum {
	EPcParticleFlag_InHash				= (1 << 0),
	EPcParticleFlag_NotInitiallyCreated	= (1 << 1),
	EPcParticleFlag_InUse				= (1 << 2),
	EPcParticleFlag_Created				= (1 << 3)
};

#define EPcParticleFlag_PersistentMask	EPcParticleFlag_NotInitiallyCreated

// =============================================
// structure definitions
// =============================================

// templated structure that is both
//
//   a) stored in a level .dat file
//   b) dynamically allocated by in-engine environmental particle code

typedef tm_struct EPtEnvParticle
{
	char							classname[64];		// must match P3cParticleClassNameLength + 1
	char							tagname[48];		// must match EPcParticleTagNameLength + 1
	M3tMatrix4x3					matrix;
	float							decal_xscale;		// these are used when creating decals only
	float							decal_yscale;		// they are multiplied by the values found in particle class
	UUtUns16						flags;

	// these values are set up at initialization time by our owner, and therefore not zeroed when
	// the particle is initialized. but they aren't persistent either.
	UUtUns16						owner_type;
	tm_raw(void *)					owner;
	tm_raw(M3tMatrix4x3	*)			dynamic_matrix;

	// everything below here is zeroed at initialization time
	tm_raw(struct P3tParticleClass	*)	particle_class;
	tm_raw(struct P3tParticle *)		particle;
	tm_raw(UUtUns32)					particle_self_ref;
	tm_raw(struct EPtEnvParticle *)		globalprev;
	tm_raw(struct EPtEnvParticle *)		globalnext;
	tm_raw(struct EPtEnvParticle *)		hashlist;
	tm_raw(struct EPtEnvParticle *)		taglist;

} EPtEnvParticle;

#define EPcTemplate_EnvParticleArray UUm4CharToUns32('E','N','V','P')
typedef tm_template('E','N','V','P',"Env Particle Array")
EPtEnvParticleArray
{
	tm_pad							pad0[22];
	
	tm_varindex UUtUns16			numParticles;
	tm_vararray EPtEnvParticle		particle[1];
		
} EPtEnvParticleArray;

// callback function template, for enumerating env particles
typedef void
(*EPtEnumCallback_EnvParticle)(
	EPtEnvParticle	*inParticle,
	UUtUns32				inUserData);

// =============================================
// external globals
// =============================================

extern UUtBool EPgDrawParticles;
extern UUtBool EPgCreatedParticles;

// =============================================
// function prototypes
// =============================================

// initialization and deletion
UUtError EPrInitialize(void);
UUtError EPrRegisterTemplates(void);
void EPrTerminate(void);

// initialize for the start of the level load process
UUtError EPrLevelBegin(void);

// set up all particles at actual level start time (finds class pointers)
void EPrLevelStart(UUtUns32 inTime);

// precache all templates required to display environmental particles
UUtError EPrPrecacheParticles(void);

// shut down after a level
void EPrLevelEnd(void);

// draw particle position markers (only useful when debugging)
void EPrDisplay(void);

// print out all particle tags
void EPrPrintTags(void);

// enumerate particles globally, or by tag
void EPrEnumerateAllParticles(EPtEnumCallback_EnvParticle inCallback, UUtUns32 inUserData);
void EPrEnumerateByTag(char *inTag, EPtEnumCallback_EnvParticle inCallback, UUtUns32 inUserData);

// enumerate all environmental particle classes and possible child particle classes also
void EPrEnumerateParticleClassesRecursively(void (*inCallback)(struct P3tParticleClass *particle_class, UUtUns32 user_data), UUtUns32 inUserData);

// get the list of environmental particles with a given tag from the hash structure
EPtEnvParticle *EPrTagHash_Retrieve(char *inTag);

// notify a new env particle of its allocation, or delete an existing one
void EPrNew(EPtEnvParticle *inParticle, UUtUns32 inTime);
void EPrDelete(EPtEnvParticle *inParticle);

// create, kill or reset an env particle
UUtBool EPrNewParticle(EPtEnvParticle *inParticle, UUtUns32 inTime);
UUtBool EPrKillParticle(EPtEnvParticle *inParticle);
UUtBool EPrResetParticle(EPtEnvParticle *inParticle, UUtUns32 inTime);

// notify a particle that various of its parameters have been changed
void EPrPreNotifyTagChange(EPtEnvParticle *inParticle);
void EPrPostNotifyTagChange(EPtEnvParticle *inParticle);
void EPrNotifyClassChange(EPtEnvParticle *inParticle, UUtUns32 inTime);
void EPrNotifyMatrixChange(EPtEnvParticle *inParticle, UUtUns32 inTime, UUtBool inInitialize);

// create or delete an array of environmental particles
UUtError EPrArray_New(EPtEnvParticleArray *inParticleArray, UUtUns16 inOwnerType,
					  void *inOwner, M3tMatrix4x3 *inDynamicMatrix, UUtUns32 inTime, char *inTagPrefix);
void EPrArray_Delete(EPtEnvParticleArray *inParticleArray);

#endif /* __BFW_ENVPARTICLE_H__ */
