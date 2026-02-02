// ======================================================================
// Oni_ImpactEffect.h
// ======================================================================
#pragma once

#ifndef ONI_IMPACTEFFECT_H
#define ONI_IMPACTEFFECT_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Materials.h"
#include "BFW_Particle3.h"

#include "Oni_ImpactEffect.h"

// ======================================================================
// defines
// ======================================================================
#define ONcIEBinaryDataClass			UUm4CharToUns32('O', 'N', 'I', 'E')
#define ONcIESound_None					((UUtUns32) -1)

// ======================================================================
// constants
// ======================================================================
enum
{
	ONcIEModType_Any,
	ONcIEModType_HeavyDamage,
	ONcIEModType_MediumDamage,
	ONcIEModType_LightDamage,

	ONcIEModType_NumTypes,

	ONcIEModType_None			= 0xFFFFFFFF
};

enum
{
	ONcIEComponent_Impact,
	ONcIEComponent_Damage,
	ONcIEComponent_Projectile,

	ONcIEComponent_Max
};

enum
{
	ONcIESoundFlag_AICanHear		= (1 << 0)
};

// ======================================================================
// globals
// ======================================================================
extern const char *ONcIEComponentName[];
extern const char *ONgIEModTypeName[];

extern UUtBool ONgImpactEffect_Locked;
extern UUtBool ONgImpactEffect_Loaded;
extern UUtBool ONgDebugImpactEffects;

// ======================================================================
// typedefs
// ======================================================================
typedef struct ONtIEParticle
{
	char						particle_class_name[P3cParticleClassNameLength + 1];
	P3tEffectSpecification		effect_spec;

} ONtIEParticle;

typedef struct ONtIESound_Version1
{
	char						sound_name[BFcMaxFileNameLength];
	UUtUns32					impulse_id;
	SStImpulse *				impulse_ptr;
} ONtIESound_Version1;

typedef struct ONtIESound
{
	char						sound_name[BFcMaxFileNameLength];
	UUtUns32					dummy;			// impulse_id is not used any more, but leaving this in the structure makes loading/versioning easier
	SStImpulse *				impulse_ptr;
	UUtUns16					flags;
	UUtUns16					ai_soundtype;
	float						ai_soundradius;
} ONtIESound;

typedef struct ONtImpactEntry
{
	MAtImpactType				impact_type;
	MAtMaterialType				material_type;
	UUtUns16					component;
	UUtUns16					modifier;

	UUtUns16					num_particles;
	UUtUns16					pad;
	UUtUns32					sound_index;			// into ONgImpactSounds (may be ONcIESound_None)
	UUtUns32					particle_baseindex;		// into ONgImpactParticles
} ONtImpactEntry;

// ======================================================================
// prototypes
// ======================================================================
// accessors
void
ONrImpactEffect_GetImpactEntries(
	MAtImpactType				inImpactType,
	MAtMaterialType				inMaterialItr,
	MAtMaterialType				*outMaterialType,
	UUtUns16 *					outNumEntries,
	UUtUns32 *					outEntryIndex);

ONtImpactEntry *
ONrImpactEffect_GetEntry(
	UUtUns32					inIndex);

ONtIEParticle *
ONrImpactEffect_GetParticles(
	UUtUns32					inEntryIndex,
	UUtUns32 *					outNumParticles);

ONtIESound *
ONrImpactEffect_GetSound(
	UUtUns32					inEntryIndex);

void
ONrImpactEffect_GetDescription(
	UUtUns32					inEntryIndex,
	char *						outString);

// ----------------------------------------------------------------------
// data management; creation, deletion, shuffling
UUtError
OWrImpactEffect_MakeDirty(
	void);

UUtError
ONrImpactEffect_CreateImpactEntry(
	MAtImpactType				inImpactType,
	MAtMaterialType				inMaterialType,
	UUtUns32 *					outEntryIndex);

UUtError
ONrImpactEffect_DeleteEntry(
	UUtBool						inAutomatic,
	UUtUns32 					inEntryIndex);

UUtError
ONrImpactEffect_NewSound(
	UUtUns32					inEntryIndex,
	ONtIESound **				outSound);

UUtError
ONrImpactEffect_DeleteSound(
	UUtBool						inAutomatic,
	UUtUns32					inSoundIndex);

UUtError
ONrImpactEffect_AddParticleToEntry(
	UUtUns32					inEntryIndex,
	ONtIEParticle				**outParticle);

UUtError
ONrImpactEffect_RemoveParticleFromEntry(
	UUtBool						inAutomatic,
	UUtUns32					inEntryIndex,
	UUtUns32					inParticleIndex);

void
OWrImpactEffect_UpdateParticleClassPtr(
	UUtUns32					inEntryIndex,
	UUtUns32					inParticleIndex);

void
OWrImpactEffect_UpdateSoundPtr(
	UUtUns32					inEntryIndex);

UUtError
ONrImpactEffect_Copy(
	UUtUns32					inEntryIndex);

UUtError
ONrImpactEffect_Paste(
	MAtImpactType				inImpactType,
	MAtMaterialType				inMaterialType,
	UUtUns32					*outEntryIndex);


// ----------------------------------------------------------------------
// the actual impact functions
void
ONrImpactEffect(
	MAtImpactType				inImpactType,
	MAtMaterialType				inMaterialType,
	UUtUns16					inModifier,
	P3tEffectData				*inData,
	float						inAIVolumeModifier,
	struct ONtCharacter			*inAICharacter1,
	struct ONtCharacter			*inAICharacter2);

void
ONrImpactEffect_Lookup(
	MAtImpactType				inImpactType,
	MAtMaterialType				inMaterialType,
	UUtUns16					inModifier,
	UUtUns32					*outImpactEntries);		// array of ONcIEComponent_Max

// ----------------------------------------------------------------------
// miscellaneous
static UUcInline const char *
ONrIEComponent_GetName(
	UUtUns32					inComponent)
{
	UUmAssert((inComponent >= 0) && (inComponent < ONcIEComponent_Max));
	return ONcIEComponentName[inComponent];
}

static UUcInline const char*
ONrIEModType_GetName(
	UUtUns32					inModType)
{
	UUmAssert(inModType < ONcIEModType_NumTypes);
	return ONgIEModTypeName[inModType];
}

static UUcInline UUtUns16
ONrImpactEffect_GetModifierByName(
	char *						inModName)
{
	UUtUns32 itr;

	for (itr = 0; itr < ONcIEModType_NumTypes; itr++) {
		if (strcmp(inModName, ONgIEModTypeName[itr]) == 0) {
			return (UUtUns16) itr;
		}
	}

	return ONcIEModType_Any;
}

// ----------------------------------------------------------------------
// zeroes all arrays in preparation for the binary data load process
UUtError
ONrImpactEffects_Initialize(
	void);

void
ONrImpactEffects_SetupParticlePointers(
	void);

void
ONrImpactEffects_SetupSoundPointers(
	void);

UUtError
ONrImpactEffects_CreateBlank(
	void);

UUtError
ONrImpactEffects_Save(
	UUtBool						inAutoSave);

void
ONrImpactEffects_Terminate(
	void);

#if defined(DEBUGGING) && DEBUGGING
void
ONrImpactEffect_VerifyStructure(
	UUtBool inCheckBinaryData);
#else
#define ONrImpactEffect_VerifyStructure(x)
#endif

UUtError ONrIEBinaryData_Process(void);

// ----------------------------------------------------------------------
UUtError
ONrImpactEffects_WriteTextFile(
	void);

// ======================================================================
#endif /* ONI_IMPACTEFFECT_H */
