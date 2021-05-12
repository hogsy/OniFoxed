/*
	FILE:	BFW_EnvParticle.c
	
	AUTHOR:	Chris Butcher
	
	CREATED: May 26, 2000
	
	PURPOSE: support for environmental particles
	
	Copyright (c) 2000, Bungie Software

 */

#include "BFW.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"
#include "BFW_Particle3.h"
#include "BFW_EnvParticle.h"

#include "Oni.h"				// CB: included so that decals can collide against environment
#include "Oni_GameState.h"

// ======================================================================
// globals
// ======================================================================

#define EPcDecal_FindWallDistance			10.0f

// hash table for retrieving environmental particles by tag
#define EPcHashBuckets						256

EPtEnvParticle *EPgTagHash[EPcHashBuckets];

// enables drawing of particle position markers
UUtBool EPgDrawParticles = UUcFalse;

// have we already created env particles for this level?
UUtBool EPgCreatedParticles = UUcFalse;

// linked list of all env particles
EPtEnvParticle *EPgHead, *EPgTail;

static IMtShade	EPgPositionMarkerShades[EPcParticleOwner_Max] = {IMcShade_Blue, IMcShade_Green, IMcShade_Yellow, IMcShade_Red};

// ======================================================================
// internal prototypes
// ======================================================================

static void EPiSetupParticle(EPtEnvParticle *inParticle, UUtUns32 inTime);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
// notify a particle that one of its matrices has been changed
void EPrNotifyMatrixChange(EPtEnvParticle *inParticle, UUtUns32 inTime, UUtBool inInitialize)
{
	M3tPoint3D				*p_position;
	M3tVector3D				*p_velocity, *p_offset;
	M3tMatrix3x3			*p_orientation;
	M3tMatrix4x3			**p_dynamicmatrix;
	P3tOwner				*p_owner;
	P3tPrevPosition			*p_prev_position;
	UUtUns32				*p_texture, *p_lensframes;
	AKtOctNodeIndexCache	*p_envcache;
	UUtBool					is_dead;
	
	// if we haven't been created, don't do anything
	if ((inParticle->flags & EPcParticleFlag_Created) == 0)
		return;

	if (inParticle->particle_class == NULL)
		return;

	if( inParticle->particle_class->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal )
	{
		EPrKillParticle(inParticle);
		EPrNewParticle( inParticle, inTime );
		return;
	}
	
	// make sure there is a particle before going any further
	if (inParticle->particle == NULL)
		return;

	UUmAssert(inParticle->particle_class != NULL);
	p_position = P3rGetPositionPtr(inParticle->particle_class, inParticle->particle, &is_dead);
	if (is_dead)
		return;
	
	// set up the particle's position from the envparticle's matrix
	*p_position = MUrMatrix_GetTranslation(&inParticle->matrix);
	
	p_orientation = P3rGetOrientationPtr(inParticle->particle_class, inParticle->particle);
	if (p_orientation != NULL) {
		MUrMatrix3x3_ExtractFrom4x3(p_orientation, &inParticle->matrix);
	}
	
	p_velocity = P3rGetVelocityPtr(inParticle->particle_class, inParticle->particle);
	if (p_velocity != NULL) {
		MUmVector_Set(*p_velocity, 0, 0, 0);
	}
	
	p_offset = P3rGetOffsetPosPtr(inParticle->particle_class, inParticle->particle);
	if (p_offset != NULL) {
		MUmVector_Set(*p_offset, 0, 0, 0);
	}
	
	p_dynamicmatrix = P3rGetDynamicMatrixPtr(inParticle->particle_class, inParticle->particle);
	if (p_dynamicmatrix != NULL) {
		*p_dynamicmatrix = inParticle->dynamic_matrix;
		switch(inParticle->owner_type) {
			case EPcParticleOwner_Object:
				// this particle may move around without the particlesystem being informed
				inParticle->particle->header.flags |= P3cParticleFlag_AlwaysUpdatePosition;
				break;
		}
	}
	
	/* everything below here only needs to be done once when the particle is created */
	if (!inInitialize)
		return;

	p_owner = P3rGetOwnerPtr(inParticle->particle_class, inParticle->particle);
	if (p_owner != NULL) {
		*p_owner = 0;		// flags this particle as an environmental particle
	}
	
	// randomise texture start index
	p_texture = P3rGetTextureIndexPtr(inParticle->particle_class, inParticle->particle);
	if (p_texture != NULL) {
		*p_texture = (UUtUns32) UUrLocalRandom();
	}
	
	// set texture time index to be now
	p_texture = P3rGetTextureTimePtr(inParticle->particle_class, inParticle->particle);
	if (p_texture != NULL) {
		*p_texture = inTime;
	}

	// no previous position
	p_prev_position = P3rGetPrevPositionPtr(inParticle->particle_class, inParticle->particle);
	if (p_prev_position != NULL) {
		p_prev_position->time = 0;
	}

	// lensflares are not initially visible
	p_lensframes = P3rGetLensFramesPtr(inParticle->particle_class, inParticle->particle);
	if (p_lensframes != NULL) {
		*p_lensframes = 0;
	}

	// set up the environment cache
	p_envcache = P3rGetEnvironmentCache(inParticle->particle_class, inParticle->particle);
	if (p_envcache != NULL) {
		AKrCollision_InitializeSpatialCache(p_envcache);
	}
}

// ----------------------------------------------------------------------
// create an env decal
static UUtBool EPrNewDecal(EPtEnvParticle *inParticle, UUtUns32 inTime)
{
	AKtEnvironment *		environment;
	UUtUns32				quad_index;
	AKtGQ_Collision *		gqCollision;
	P3tEffectSpecification	effect_spec;
	P3tEffectData			effect_data;
	M3tPoint3D				position;
	M3tVector3D				findwall_ray;
	float					plane_dist;

	/*
	 * create a decal here
	 */

	// find the wall that this decal is resting on
	position = MUrMatrix_GetTranslation(&inParticle->matrix);
	MUrMatrix_GetCol(&inParticle->matrix, 1, &findwall_ray);
	MUmVector_Scale(findwall_ray, -EPcDecal_FindWallDistance);

	environment = ONrGameState_GetEnvironment();
	AKrCollision_Point(environment, &position, &findwall_ray, AKcGQ_Flag_Obj_Col_Skip, 1);
	if (AKgNumCollisions == 0) 
		return UUcFalse;

	quad_index	= AKgCollisionList[0].gqIndex;
	gqCollision = ONrGameState_GetEnvironment()->gqCollisionArray->gqCollision + quad_index;

	// set up the effect data to create this decal
	UUrMemory_Clear(&effect_data, sizeof(effect_data));
	effect_data.cause_type					= P3cEffectCauseType_ParticleHitWall;
	effect_data.cause_data.particle_wall.gq_index = quad_index;
	effect_data.time						= inTime;	
	effect_data.position					= AKgCollisionList[0].collisionPoint;
	MUrMatrix_GetCol(&inParticle->matrix, 2, &effect_data.upvector);
	effect_data.decal_xscale				= inParticle->decal_xscale;
	effect_data.decal_yscale				= inParticle->decal_yscale;

	AKmPlaneEqu_GetComponents(gqCollision->planeEquIndex, environment->planeArray->planes,
								effect_data.normal.x, effect_data.normal.y, effect_data.normal.z, plane_dist);

	// setup the effect spec for the decal's class
	effect_spec.particle_class						= inParticle->particle_class;
	effect_spec.location_type						= P3cEffectLocationType_Decal;
	effect_spec.location_data.decal.static_decal	= UUcTrue;
	effect_spec.location_data.decal.random_rotation	= UUcFalse;
	effect_spec.collision_orientation				= P3cEnumCollisionOrientation_Normal;

	inParticle->particle = P3rCreateEffect(&effect_spec, &effect_data);
	if (inParticle->particle == NULL) {
		// could not create the particle!
		inParticle->particle_self_ref = P3cParticleReference_Null;
		return UUcFalse;
	} else {
		// save the particle's self_ref
		inParticle->particle_self_ref = inParticle->particle->header.self_ref;
		return UUcTrue;
	}
}

// ----------------------------------------------------------------------
// create an env particle
UUtBool EPrNewParticle(EPtEnvParticle *inParticle, UUtUns32 inTime)
{	
	inParticle->flags |= EPcParticleFlag_Created;

	if (inParticle->particle_class == NULL)
		return UUcFalse;
	
	if (inParticle->particle_class->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal) {
		// create a decal instead
		return EPrNewDecal(inParticle, inTime);
	}

	// create the particle
	inParticle->particle = P3rCreateParticle(inParticle->particle_class, inTime);
	if (inParticle->particle == NULL)
		return UUcFalse;
	
	// save the particle's self_ref
	inParticle->particle_self_ref = inParticle->particle->header.self_ref;
	
	EPrNotifyMatrixChange(inParticle, inTime, UUcTrue);
	
	P3rSendEvent(inParticle->particle_class, inParticle->particle, P3cEvent_Create, ((float) inTime) / UUcFramesPerSecond);

	return UUcTrue;
}

// ----------------------------------------------------------------------
// kill an env particle
UUtBool EPrKillParticle(EPtEnvParticle *inParticle)
{
	inParticle->flags &= ~EPcParticleFlag_Created;

	if (inParticle->particle == NULL) {
		return UUcFalse;
	}
	
	if (inParticle->particle->header.self_ref != inParticle->particle_self_ref) {
		// this particle is already dead
		inParticle->particle_self_ref = P3cParticleReference_Null;
		inParticle->particle = NULL;

		return UUcFalse;
	}

	// we have a particle, we must have a particle class
	UUmAssert(inParticle->particle_class != NULL);
	
	P3rKillParticle(inParticle->particle_class, inParticle->particle);
	inParticle->particle = NULL;
	inParticle->particle_self_ref = P3cParticleReference_Null;
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
// reset an env particle to its starting state
UUtBool EPrResetParticle(EPtEnvParticle *inParticle, UUtUns32 inTime)
{
	EPrKillParticle(inParticle);
	if (inParticle->flags & EPcParticleFlag_NotInitiallyCreated)
		return UUcFalse;
	else
		return EPrNewParticle(inParticle, inTime);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
// add an environmental particle to the tag structure
static UUtBool EPiTagHash_Add(EPtEnvParticle *inParticle)
{
	UUtUns8 bucket_id;
	EPtEnvParticle *prev_entry, *current_entry;

	if (inParticle->tagname[0] == '\0') {
		// this particle has no tag and doesn't have to be added to the hash table
		return UUcFalse;
	}

	bucket_id = (UUtUns8) (AUrString_GetHashValue(inParticle->tagname) & 0xFF);
	UUmAssert((bucket_id >= 0) && (bucket_id < EPcHashBuckets));

	// find whether there are any particles with this tag in the bucket
	prev_entry = NULL;
	current_entry = EPgTagHash[bucket_id];
	while (current_entry != NULL) {
		UUmAssertReadPtr(current_entry, sizeof(EPtEnvParticle));

		if (strcmp(current_entry->tagname, inParticle->tagname) == 0)
			break;

		prev_entry = current_entry;
		current_entry = current_entry->hashlist;
	}

	if (current_entry == NULL) {
		// there are no particles with this tag already in the bucket - add it to
		// the head of the bucket
		inParticle->taglist = NULL;
		inParticle->hashlist = EPgTagHash[bucket_id];
		EPgTagHash[bucket_id] = inParticle;
	} else {
		// add this particle to the second position in the list of particles with this tag
		// (faster than adding to head or tail)
		inParticle->hashlist = NULL;
		inParticle->taglist = current_entry->taglist;
		current_entry->taglist = inParticle;
	}

	return UUcTrue;
}

// remove an environmental particle from the tag structure
static UUtBool EPiTagHash_Remove(EPtEnvParticle *inParticle)
{
	UUtUns8 bucket_id;
	EPtEnvParticle *prev_hashentry, *current_hashentry;
	EPtEnvParticle *prev_tagentry, *current_tagentry;

	if (inParticle->tagname[0] == '\0') {
		// this particle has no tag and doesn't have to be added to the hash table
		return UUcFalse;
	}

	bucket_id = (UUtUns8) (AUrString_GetHashValue(inParticle->tagname) & 0xFF);
	UUmAssert((bucket_id >= 0) && (bucket_id < EPcHashBuckets));

	// find the head of the list of particles with this tag
	prev_hashentry = NULL;
	current_hashentry = EPgTagHash[bucket_id];
	while (current_hashentry != NULL) {
		UUmAssertReadPtr(current_hashentry, sizeof(EPtEnvParticle));

		if (strcmp(current_hashentry->tagname, inParticle->tagname) == 0)
			break;

		prev_hashentry = current_hashentry;
		current_hashentry = current_hashentry->hashlist;
	}

	if (current_hashentry == NULL) {
		// there are no particles with this tag
		return UUcFalse;
	}

	// find this particle in the tag list
	prev_tagentry = NULL;
	current_tagentry = current_hashentry;
	while (current_tagentry != NULL) {
		UUmAssertReadPtr(current_tagentry, sizeof(EPtEnvParticle));

		if (current_tagentry == inParticle)
			break;

		prev_tagentry = current_tagentry;
		current_tagentry = current_tagentry->taglist;
	}

	if (current_tagentry == NULL) {
		// this particle is not present in the list of particles with this tag
		return UUcFalse;
	}

	if (prev_tagentry == NULL) {
		// this particle is at the head of its taglist
		if (inParticle->taglist == NULL) {
			// it is the only one in its taglist - remove the taglist completely
			if (prev_hashentry == NULL) {
				// this particle is the only one in its bucket
				EPgTagHash[bucket_id] = inParticle->hashlist;
			} else {
				// remove this particle from the hashlist
				prev_hashentry->hashlist = inParticle->hashlist;
			}
		} else {
			// move up the next entry in the taglist into the hashlist
			inParticle->taglist->hashlist = inParticle->hashlist;
			if (prev_hashentry == NULL) {
				// move it all the way up to the bucket
				EPgTagHash[bucket_id] = inParticle->taglist;
			} else {
				// link up the previous entry in the haslist
				prev_hashentry->hashlist = inParticle->taglist;
			}
		}
	} else {
		// remove this particle from its taglist
		prev_tagentry->taglist = inParticle->taglist;
	}

	return UUcTrue;
}

// get the list of environmental particles with a given tag from the hash structure
EPtEnvParticle *EPrTagHash_Retrieve(char *inTag)
{
	UUtUns8 bucket_id;
	EPtEnvParticle *entry;

	if (inTag[0] == '\0') {
		// this is not a valid tag name!
		return NULL;
	}

	bucket_id = (UUtUns8) (AUrString_GetHashValue(inTag) & 0xFF);
	UUmAssert((bucket_id >= 0) && (bucket_id < EPcHashBuckets));

	// find whether there are any particles with this tag in the bucket
	entry = EPgTagHash[bucket_id];
	while (entry != NULL) {
		UUmAssertReadPtr(entry, sizeof(EPtEnvParticle));

		if (strcmp(entry->tagname, inTag) == 0)
			return entry;

		entry = entry->hashlist;
	}

	return NULL;
}

// print out all particle tags
void EPrPrintTags(void)
{
	UUtUns32 itr, num_objects, num_particles;
	EPtEnvParticle *entry, *tagentry;

	COrConsole_Printf("list of all particle tags with number of tagged particle objects and number of living particles");
	for (itr = 0; itr < EPcHashBuckets; itr++) {
		for (entry = EPgTagHash[itr]; entry != NULL; entry = entry->hashlist) {
			num_objects = 0;
			num_particles = 0;
			for (tagentry = entry; tagentry != NULL; tagentry = tagentry->taglist) {
				num_objects++;
				if (tagentry->particle != NULL) {
					num_particles++;
				}
			}
			COrConsole_Printf("  %s: %d objects, %d particles", entry->tagname, num_objects, num_particles);
		}
	}
}

// enumerate particles by tag
void EPrEnumerateByTag(char *inTag, EPtEnumCallback_EnvParticle inCallback, UUtUns32 inUserData)
{
	EPtEnvParticle *particle;

	particle = EPrTagHash_Retrieve(inTag);

	while (particle != NULL) {
		UUmAssertReadPtr(particle, sizeof(EPtEnvParticle));

		inCallback(particle, inUserData);
		
		particle = particle->taglist;
	}
}

// enumerate particles globally, or by tag
void EPrEnumerateAllParticles(EPtEnumCallback_EnvParticle inCallback, UUtUns32 inUserData)
{
	EPtEnvParticle *particle;

	for (particle = EPgHead; particle != NULL; particle = particle->globalnext) {
		UUmAssertReadPtr(particle, sizeof(EPtEnvParticle));

		inCallback(particle, inUserData);
	}
}

typedef struct EPtEnumerateRecursivelyUserData
{
	P3tClassCallback callback;
	UUtUns32 callback_data;
} EPtEnumerateRecursivelyUserData;

static void EPiTraverseParticleClassesRecursively(EPtEnvParticle *inParticle, UUtUns32 inUserData)
{
	EPtEnumerateRecursivelyUserData *user_data = (EPtEnumerateRecursivelyUserData *) inUserData;

	UUmAssertReadPtr(user_data, sizeof(*user_data));
	P3rTraverseParticleClass(inParticle->particle_class, user_data->callback, user_data->callback_data);
}

// enumerate all environmental particle classes and possible child particle classes also
void EPrEnumerateParticleClassesRecursively(P3tClassCallback inCallback, UUtUns32 inUserData)
{
	EPtEnumerateRecursivelyUserData user_data;

	user_data.callback = inCallback;
	user_data.callback_data = inUserData;

	P3rSetupTraverse();
	EPrEnumerateAllParticles(EPiTraverseParticleClassesRecursively, (UUtUns32) &user_data);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
// set up a new env particle - done once the particle classes have been loaded
static void EPiSetupParticle(EPtEnvParticle *inParticle, UUtUns32 inTime)
{
	// make sure that we don't have any particles currently
	EPrKillParticle(inParticle);

	inParticle->particle_class = P3rGetParticleClass(inParticle->classname);

	if (((inParticle->flags & EPcParticleFlag_NotInitiallyCreated) == 0) && (inParticle->particle_class != NULL))
		EPrNewParticle(inParticle, inTime);
}

// set up a particle at level start
static void EPiLevelStartEnumCallback(EPtEnvParticle *inParticle, UUtUns32 inUserData)
{
	EPiSetupParticle(inParticle, inUserData);
}

// ----------------------------------------------------------------------
// notify that a particle's class name has been changed
void EPrNotifyClassChange(EPtEnvParticle *inParticle, UUtUns32 inTime)
{
	EPiSetupParticle(inParticle, inTime);
}

// notify that a particle's tag name is *about to be* changed
void EPrPreNotifyTagChange(EPtEnvParticle *inParticle)
{
	UUtBool was_in_hash;

	if (inParticle->flags & EPcParticleFlag_InHash) {
		// remove from the hash structure
		was_in_hash = EPiTagHash_Remove(inParticle);

		UUmAssert(was_in_hash);
		inParticle->flags &= ~EPcParticleFlag_InHash;
	}
}

// notify that a particle's tag name has been changed
void EPrPostNotifyTagChange(EPtEnvParticle *inParticle)
{
	UUmAssert((inParticle->flags & EPcParticleFlag_InHash) == 0);

	if (EPiTagHash_Add(inParticle)) {
		inParticle->flags |= EPcParticleFlag_InHash;
	}
}

// notify a new env particle of its allocation - requires that main fields be set up (all those
// specified as nonzero in BFW_EnvParticle.h)
void EPrNew(EPtEnvParticle *inParticle, UUtUns32 inTime)
{
	// this should never be called for a particle that
	// has already been added to the particle list
	UUmAssert((inParticle->flags & EPcParticleFlag_InUse) == 0);
	inParticle->flags |= EPcParticleFlag_InUse;

	// zero all of the non-persistent fields
	inParticle->particle_class = NULL;
	inParticle->particle = NULL;
	inParticle->particle_self_ref = P3cParticleReference_Null;
	inParticle->hashlist = NULL;
	inParticle->taglist = NULL;

	// add this to the global linked list
	if (EPgTail == NULL) {
		EPgHead = inParticle;
	} else {
		EPgTail->globalnext = inParticle;
	}
	inParticle->globalprev = EPgTail;
	inParticle->globalnext = NULL;
	EPgTail = inParticle;

	// add this to the hash structure
	inParticle->flags &= ~EPcParticleFlag_InHash;
	EPrPostNotifyTagChange(inParticle);

	if (EPgCreatedParticles) {
		// we have already created env particles this level, it won't be automatically initialized
		// at the end of the particle class binary data loading. create it now.
		EPiSetupParticle(inParticle, inTime);

		if (!UUmString_IsEqual(inParticle->tagname, "")) {
			// particle classes may need to recalculate their attractor pointers now that a new tagged
			// env particle has been added
			P3rSetupAttractorPointers(NULL);
		}
	}
}

// delete an existing env particle
void EPrDelete(EPtEnvParticle *inParticle)
{
	UUtBool					was_in_hash;
	
	// this should never be called for a particle that
	// hasn't already been added to the particle list
	UUmAssert(inParticle->flags & EPcParticleFlag_InUse);

	EPrKillParticle(inParticle);

	// remove this particle from the tag hash table if necessary
	if (inParticle->flags & EPcParticleFlag_InHash) {
		was_in_hash = EPiTagHash_Remove(inParticle);

		UUmAssert(was_in_hash);
		inParticle->flags &= ~EPcParticleFlag_InHash;
	}

	if (EPgCreatedParticles && !UUmString_IsEqual(inParticle->tagname, "")) {
		// particle classes may need to recalculate their attractor pointers now that a tagged
		// env particle has been deleted
		P3rSetupAttractorPointers(NULL);
	}

	// delete this from the global linked list
	if (inParticle->globalprev == NULL) {
		EPgHead = inParticle->globalnext;
	} else {
		inParticle->globalprev->globalnext = inParticle->globalnext;
	}
	if (inParticle->globalnext == NULL) {
		EPgTail = inParticle->globalprev;
	} else {
		inParticle->globalnext->globalprev = inParticle->globalprev;
	}

	inParticle->globalprev = NULL;
	inParticle->globalnext = NULL;

	inParticle->flags &= ~EPcParticleFlag_InUse;
}

// ----------------------------------------------------------------------
UUtError EPrArray_New(EPtEnvParticleArray *inParticleArray, UUtUns16 inOwnerType,
					  void *inOwner, M3tMatrix4x3 *inDynamicMatrix, UUtUns32 inTime, char *inTagPrefix)
{
	UUtUns32 itr;
	EPtEnvParticle *particle;
	char particle_tag[256];
	UUtBool skip_prefix = ((inTagPrefix == NULL) || (inTagPrefix[0] == '\0'));
	
	if (inParticleArray == NULL)
		return UUcError_None;

	for (itr = 0, particle = inParticleArray->particle; itr < inParticleArray->numParticles; itr++, particle++) {
		particle->owner_type = inOwnerType;
		particle->owner = inOwner;
		particle->dynamic_matrix = inDynamicMatrix;

		if (!skip_prefix) {
			if (UUrString_HasPrefix(particle->tagname, inTagPrefix)) {
				// we already have been prefixed with this tag, maybe these particles have already been
				// attached to this object and are now being recreated
			} else {
				// prefix the particle's tag with an identifier
				sprintf(particle_tag, "%s_%s", inTagPrefix, particle->tagname);
				UUrString_Copy(particle->tagname, particle_tag, EPcParticleTagNameLength + 1);
			}
		}

		EPrNew(particle, inTime);
	}
	
	return UUcError_None;
}

void EPrArray_Delete(EPtEnvParticleArray *inParticleArray)
{
	UUtUns32 itr;
	EPtEnvParticle *particle;
	
	if (inParticleArray == NULL)
		return;

	for (itr = 0, particle = inParticleArray->particle; itr < inParticleArray->numParticles; itr++, particle++) {
		if (particle->flags & EPcParticleFlag_InUse) {
			EPrDelete(particle);
		}
	}
}

// ----------------------------------------------------------------------
static void EPiDrawPositionMarker(EPtEnvParticle *inParticle, UUtUns32 inUserData)
{
	UUtUns8					block[(sizeof(M3tPoint3D) * 4) + (2 * UUcProcessor_CacheLineSize)];
	M3tPoint3D				*tri = UUrAlignMemory(block);
	IMtShade				shade;

	UUmAssert((inParticle->owner_type >= 0) && (inParticle->owner_type < EPcParticleOwner_Max));
	shade = EPgPositionMarkerShades[inParticle->owner_type];

	// setup the matrix stack
	M3rMatrixStack_Push();
	if (inParticle->dynamic_matrix != NULL) {
		M3rMatrixStack_Multiply(inParticle->dynamic_matrix);
	}
	M3rMatrixStack_Multiply(&inParticle->matrix);
	M3rGeom_State_Commit();

	// draw the triangle
	MUmVector_Set(tri[0], 0.0f, 0.0f, 1.0f);
	MUmVector_Set(tri[1], 0.8660f, 0.0f, -0.5f);
	MUmVector_Set(tri[2], -0.8660f, 0.0f, -0.5f);
	tri[3] = tri[0];
	M3rGeometry_LineDraw(4, tri, shade);
	
	MUmVector_Set(tri[0], 0.0f, 0.0f, 0.0f);
	MUmVector_Set(tri[1], 0.0f, 1.0f, 0.0f);
	M3rGeometry_LineDraw(2, tri, shade);
	
	M3rMatrixStack_Pop();
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError EPrInitialize(void)
{
	UUtError error;

	error = EPrRegisterTemplates();
	UUmError_ReturnOnError(error);

	// clear the hash table
	UUrMemory_Clear(EPgTagHash, EPcHashBuckets * sizeof(EPtEnvParticle *));

	// zero the global linked list
	EPgHead = NULL;
	EPgTail = NULL;

	EPgCreatedParticles = UUcFalse;

	return UUcError_None;
}

UUtError EPrRegisterTemplates(void)
{
	UUtError error;
	
	error = TMrTemplate_Register(EPcTemplate_EnvParticleArray, sizeof(EPtEnvParticleArray), TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

void EPrTerminate(void)
{
}

UUtError EPrLevelBegin(void)
{
	return UUcError_None;
}

// set up all particles at level start
void EPrLevelStart(UUtUns32 inTime)
{
	UUmAssert(P3gNumClasses >= 0);

	EPrEnumerateAllParticles(EPiLevelStartEnumCallback, inTime);
	EPgCreatedParticles = UUcTrue;
}

static void EPiPrecacheParticles_ClassCallback(P3tParticleClass *inClass, UUtUns32 inUserData)
{
	P3rPrecacheParticleClass(inClass);
}

// precache all templates required to display environmental particles
UUtError EPrPrecacheParticles(void)
{
	EPrEnumerateParticleClassesRecursively(EPiPrecacheParticles_ClassCallback, 0);
	return UUcError_None;
}

void EPrLevelEnd(void)
{
	// clear the hash table
	UUrMemory_Clear(EPgTagHash, EPcHashBuckets * sizeof(EPtEnvParticle *));

	// zero the global linked list
	EPgHead = NULL;
	EPgTail = NULL;

	EPgCreatedParticles = UUcFalse;
}

void EPrDisplay(void)
{
	if (EPgDrawParticles)
		EPrEnumerateAllParticles(EPiDrawPositionMarker, 0);
}