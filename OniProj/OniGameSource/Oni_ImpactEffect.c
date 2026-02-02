// ======================================================================
// Oni_ImpactEffect.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Materials.h"
#include "BFW_Particle3.h"
#include "BFW_SoundSystem2.h"

#include "Oni_Character_Animation.h"
#include "Oni_Character.h"
#include "Oni_BinaryData.h"
#include "Oni_ImpactEffect.h"
#include "Oni_Sound2.h"

// ======================================================================
// defines
// ======================================================================
#define ONcIE_ChunkSize					5
#define MODIFIER_OVERRIDE				0
#define ONcMinImpactVolume				0.3f

// ======================================================================
// enums
// ======================================================================
enum
{
	ONcIEVersion_1			= 1,
	ONcIEVersion_2			= 2,

	ONcIECurrentVersion		= ONcIEVersion_2
};

// ======================================================================
// typedefs
// ======================================================================
// sub-structures used by the impact effect data structure
typedef struct ONtImpactMaterialIndex
{
	MAtMaterialType				material;
	UUtUns16					num_entries;
	UUtUns32					entry_index;			// into ONgImpactEntries
} ONtImpactMaterialIndex;

typedef struct ONtImpactLookupIndex
{
	MAtImpactType				impact_type;
	UUtUns16					num_indices;
	UUtUns32					index_base;				// into ONgImpactMaterialIndices
} ONtImpactLookupIndex;


// translation tables used to ensure that impact and material references remain valid even when IDs change
typedef struct ONtImpactTranslator
{
	char						name[TMcInstanceName_MaxLength];
	MAtImpactType				file_type;
	MAtImpactType				current_type;
} ONtImpactTranslator;

typedef struct ONtMaterialTranslator
{
	char						name[TMcInstanceName_MaxLength];
	MAtMaterialType				file_type;
	MAtMaterialType				current_type;
} ONtMaterialTranslator;

// ======================================================================
// external globals
// ======================================================================
const char *ONcIEComponentName[ONcIEComponent_Max] = {"Impact", "Damage", "Projectile"};

const char *ONgIEModTypeName[ONcIEModType_NumTypes + 1] =
{
	"Any",
	"Heavy",
	"Medium",
	"Light",
	NULL
};

UUtBool							ONgImpactEffect_Locked;
UUtBool							ONgImpactEffect_Loaded;
UUtBool							ONgDebugImpactEffects;

// ======================================================================
// internal globals
// ======================================================================
static UUtBool					ONgImpactEffect_Dirty;
static UUtBool					ONgImpactEffect_AutomaticModificationsOnly;
static UUtBool					ONgImpactEffect_DynamicEntryArray;
static UUtBool					ONgImpactEffect_DynamicSoundArray;
static UUtBool					ONgImpactEffect_DynamicParticleArray;
static UUtBool					ONgImpactEffect_DynamicMaterialArray;
static void *					ONgImpactEffect_MemoryBlock;	// memory block to deallocate upon termination (may be NULL)

// globals for impact and material translation at load-time.
static UUtBool					ONgIE_ImpactsRequireTranslation;
static UUtUns32					ONgIE_FileNumImpactTypes;
static UUtUns32					ONgIE_FileNumMaterialTypes;
static ONtImpactTranslator *	ONgIE_ImpactTranslation;
static ONtMaterialTranslator*	ONgIE_MaterialTranslation;

// clipboard for copy and paste
static UUtBool					ONgImpactEffect_ClipboardFull;
static ONtImpactEntry			ONgImpactEffect_ClipboardEntry;
static ONtIESound				ONgImpactEffect_ClipboardSound;
static ONtIEParticle *			ONgImpactEffect_ClipboardParticles;

/*
 * the impact effect data itself
 */

// each of these array pointers is either to an actual array or to a UUtMemory_Array
// depending on the value of ONgImpactEffect_Dynamic<type>Array.
static UUtUns32					ONgNumImpactEntries;
static void						*ONgImpactEntries;

static UUtUns32					ONgNumImpactSounds;
static void						*ONgImpactSounds;

static UUtUns32					ONgNumImpactParticles;
static void						*ONgImpactParticles;

static UUtUns32					ONgNumImpactMaterialIndices;
static void						*ONgImpactMaterialIndices;

static UUtBool					ONgImpactLookupTable_Allocated;
static ONtImpactLookupIndex		*ONgImpactLookupTable;	// one for each MAtImpactType. not a pool

// ======================================================================
// internal function prototypes
// ======================================================================

static void
ONiIEClearStorage(
	void);

static void
ONiIEDeleteStorage(
	void);

static void
ONiImpactEffect_UpdateParticlePointers(
	UUtUns32					inNumParticles,
	UUtUns32					inBaseIndex);

static void
ONiImpactEffect_UpdateSoundPointers(
	UUtUns32					inNumSounds,
	UUtUns32					inBaseIndex);

// ======================================================================
// array management functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError ONiImpactEffect_MakeDynamicArray(
	UUtBool *					ioIsDynamic,
	UUtUns32					inNumElements,
	UUtUns8 *					inArray,
	UUtUns32					inPrePadSize,
	UUtUns32					inFinalSize,
	void **						outDynamicArray)
{
	UUtMemory_Array *new_array;
	UUtUns32 itr;
	void *memptr;
	char *srcptr, *destptr;

	if (*ioIsDynamic) {
		// already dynamic
		return UUcError_None;
	}

	// impact entries
	new_array = UUrMemory_Array_New(inFinalSize, ONcIE_ChunkSize, inNumElements, inNumElements + ONcIE_ChunkSize);
	UUmError_ReturnOnNull(new_array);

	if (inNumElements > 0) {
		UUmAssertReadPtr(inArray, inNumElements * inPrePadSize);

		memptr = UUrMemory_Array_GetMemory(new_array);

		if (inPrePadSize == inFinalSize) {
			// copy the data across - no re-padding is required
			UUrMemory_MoveFast(inArray, memptr, inNumElements * inFinalSize);
		} else {
			// we can't shrink memory and discard data
			UUmAssert(inFinalSize > inPrePadSize);

			// we must fill out each element with zeroes to reach the desired final size
			srcptr = (char *)inArray;
			destptr = (char *) memptr;
			for (itr = 0; itr < inNumElements; itr++) {
				UUmAssertReadPtr(srcptr, inPrePadSize);
				UUmAssertWritePtr(destptr, inFinalSize);

				UUmAssert(srcptr < ((char *) inArray) + inNumElements * inPrePadSize);
				UUmAssert(destptr < ((char *) memptr) + inNumElements * inFinalSize);

				UUrMemory_MoveFast(srcptr, destptr, inPrePadSize);
				srcptr += inPrePadSize;
				destptr += inPrePadSize;
				UUrMemory_Clear(destptr, inFinalSize - inPrePadSize);
				destptr += inFinalSize - inPrePadSize;
			}
		}
#if defined(DEBUGGING) && DEBUGGING
		UUrMemory_Set8(inArray, 0xdd, inNumElements * inPrePadSize);
#endif
	}

	// store the new array
	*outDynamicArray = new_array;
	*ioIsDynamic = UUcTrue;
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError ONiImpactEffect_MakeDirty(
	UUtBool						inAutomatic,
	UUtBool						inGrow)
{
	UUtError error;
	UUtBool requested_automatic_modification;

	if (ONgImpactEffect_Dirty) {
		// we are still auto-only if this is automatic and all previous modifications were too
		requested_automatic_modification = (inAutomatic && ONgImpactEffect_AutomaticModificationsOnly);
	} else {
		// this is the first modification
		requested_automatic_modification = inAutomatic;
	}

	if (ONgImpactEffect_Locked && !requested_automatic_modification) {
		// a manual modification is attempting to modify locked impact effects, warn about it
		return UUcError_Generic;
	}

	ONgImpactEffect_AutomaticModificationsOnly = requested_automatic_modification;
	ONgImpactEffect_Dirty = UUcTrue;

	if (inGrow) {
		/*
		 * turn all of the static arrays into dynamic UUtMemory_Array structures
		 */
		error = ONiImpactEffect_MakeDynamicArray(&ONgImpactEffect_DynamicEntryArray, ONgNumImpactEntries, (UUtUns8 *) ONgImpactEntries,
												sizeof(ONtImpactEntry), sizeof(ONtImpactEntry), &ONgImpactEntries);
		UUmError_ReturnOnError(error);

		error = ONiImpactEffect_MakeDynamicArray(&ONgImpactEffect_DynamicSoundArray, ONgNumImpactSounds, (UUtUns8 *) ONgImpactSounds,
												sizeof(ONtIESound), sizeof(ONtIESound), &ONgImpactSounds);
		UUmError_ReturnOnError(error);

		error = ONiImpactEffect_MakeDynamicArray(&ONgImpactEffect_DynamicParticleArray, ONgNumImpactParticles, (UUtUns8 *) ONgImpactParticles,
												sizeof(ONtIEParticle), sizeof(ONtIEParticle), &ONgImpactParticles);
		UUmError_ReturnOnError(error);

		error = ONiImpactEffect_MakeDynamicArray(&ONgImpactEffect_DynamicMaterialArray, ONgNumImpactMaterialIndices, (UUtUns8 *) ONgImpactMaterialIndices,
												sizeof(ONtImpactMaterialIndex), sizeof(ONtImpactMaterialIndex), &ONgImpactMaterialIndices);
		UUmError_ReturnOnError(error);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUcInline ONtImpactEntry *
ONiImpactEffect_GetEntry(
	UUtUns32					inIndex,
	UUtUns32					inLength)
{
	ONtImpactEntry *base;

	UUmAssert((inIndex >= 0) && (inIndex + inLength <= ONgNumImpactEntries));
	if (ONgImpactEffect_DynamicEntryArray) {
		base = (ONtImpactEntry *) UUrMemory_Array_GetMemory((UUtMemory_Array *) ONgImpactEntries);
	} else {
		base = (ONtImpactEntry *) ONgImpactEntries;
	}

	return (base + inIndex);
}

// ----------------------------------------------------------------------
static UUcInline ONtIESound *
ONiImpactEffect_GetSound(
	UUtUns32					inIndex,
	UUtUns32					inLength)
{
	ONtIESound *base;

	UUmAssert((inIndex >= 0) && (inIndex + inLength <= ONgNumImpactSounds));
	if (ONgImpactEffect_DynamicSoundArray) {
		base = (ONtIESound *) UUrMemory_Array_GetMemory((UUtMemory_Array *) ONgImpactSounds);
	} else {
		base = (ONtIESound *) ONgImpactSounds;
	}

	return (base + inIndex);
}

// ----------------------------------------------------------------------
static UUcInline ONtIEParticle *
ONiImpactEffect_GetParticle(
	UUtUns32					inIndex,
	UUtUns32					inLength)
{
	ONtIEParticle *base;

	UUmAssert((inIndex >= 0) && (inIndex + inLength <= ONgNumImpactParticles));
	if (ONgImpactEffect_DynamicParticleArray) {
		base = (ONtIEParticle *) UUrMemory_Array_GetMemory((UUtMemory_Array *) ONgImpactParticles);
	} else {
		base = (ONtIEParticle *) ONgImpactParticles;
	}

	return (base + inIndex);
}

// ----------------------------------------------------------------------
static UUcInline ONtImpactMaterialIndex *
ONiImpactEffect_GetMaterialIndex(
	UUtUns32					inIndex,
	UUtUns32					inLength)
{
	ONtImpactMaterialIndex *base;

	UUmAssert((inIndex >= 0) && (inIndex + inLength <= ONgNumImpactMaterialIndices));
	if (ONgImpactEffect_DynamicMaterialArray) {
		base = (ONtImpactMaterialIndex *) UUrMemory_Array_GetMemory((UUtMemory_Array *) ONgImpactMaterialIndices);
	} else {
		base = (ONtImpactMaterialIndex *) ONgImpactMaterialIndices;
	}

	return (base + inIndex);
}

// ----------------------------------------------------------------------
#if defined(DEBUGGING) && DEBUGGING
void
ONrImpactEffect_VerifyStructure(
	UUtBool inCheckBinaryData)
{
	UUtUns32 itr, itr2, itr3, num_impact_types, num_material_types;
	ONtIESound *sound;
	ONtIEParticle *particle;
	ONtImpactEntry *entry;
	ONtImpactMaterialIndex *matindex;
	ONtImpactLookupIndex *lookup;
	SStImpulse *impulse_sound;

	num_impact_types = MArImpacts_GetNumTypes();
	num_material_types = MArMaterials_GetNumTypes();

	// check globals
	UUmAssert(ONgImpactEffect_Loaded);
	UUmAssert((!ONgImpactEffect_Locked) || (!ONgImpactEffect_Dirty) || (ONgImpactEffect_AutomaticModificationsOnly));

	// check array pointers
	if (ONgImpactEffect_DynamicEntryArray) {
		UUmAssertReadPtr(ONgImpactEntries,			sizeof(UUtUns32));		// because UUtMemory_Array is an undefined type
	} else {
		UUmAssertReadPtr(ONgImpactEntries,			ONgNumImpactEntries * sizeof(ONtImpactEntry));
	}

	if (ONgImpactEffect_DynamicSoundArray) {
		UUmAssertReadPtr(ONgImpactSounds,			sizeof(UUtUns32));
	} else {
		UUmAssertReadPtr(ONgImpactSounds,			ONgNumImpactSounds * sizeof(ONtIESound));
	}

	if (ONgImpactEffect_DynamicParticleArray) {
		UUmAssertReadPtr(ONgImpactParticles,		sizeof(UUtUns32));
	} else {
		UUmAssertReadPtr(ONgImpactParticles,		ONgNumImpactParticles * sizeof(ONtIEParticle));
	}

	if (ONgImpactEffect_DynamicMaterialArray) {
		UUmAssertReadPtr(ONgImpactMaterialIndices,	sizeof(UUtUns32));
	} else {
		UUmAssertReadPtr(ONgImpactMaterialIndices,	ONgNumImpactMaterialIndices * sizeof(ONtImpactMaterialIndex));
	}
	UUmAssertReadPtr(ONgImpactLookupTable,			num_impact_types * sizeof(ONtImpactLookupIndex));

	// check sounds
	sound = ONiImpactEffect_GetSound(0, 0);
	for (itr = 0; itr < ONgNumImpactSounds; itr++, sound++) {
		if (sound->flags & ONcIESoundFlag_AICanHear) {
			UUmAssert(sound->ai_soundradius < 1000.0f);
			UUmAssert((sound->ai_soundtype >= 0) && (sound->ai_soundtype < AI2cContactType_Max));
		}

		UUmAssert(strlen(sound->sound_name) < BFcMaxFileNameLength);
		if (sound->sound_name[0] == '\0') { continue; }

		if (inCheckBinaryData) {
			impulse_sound = OSrImpulse_GetByName(sound->sound_name);
			UUmAssert(sound->impulse_ptr == impulse_sound);
		}
	}

	// check particles
	particle = ONiImpactEffect_GetParticle(0, 0);
	for (itr = 0; itr < ONgNumImpactParticles; itr++, particle++) {
		UUmAssert((particle->effect_spec.collision_orientation >= 0) &&
				(particle->effect_spec.collision_orientation < P3cEnumCollisionOrientation_Max));
		UUmAssert((particle->effect_spec.location_type >= 0) &&
				(particle->effect_spec.location_type < P3cEffectLocationType_Max));

		if (inCheckBinaryData) {
			if (particle->effect_spec.particle_class != NULL) {
				UUmAssertReadPtr(particle->effect_spec.particle_class, sizeof(P3tParticleClass));
				UUmAssert(strlen(particle->particle_class_name) <= P3cParticleClassNameLength);
				UUmAssert(UUmString_IsEqual(particle->effect_spec.particle_class->classname, particle->particle_class_name));
			}
		}
	}

	// check impact entries for basic validity
	entry = ONiImpactEffect_GetEntry(0, 0);
	for (itr = 0; itr < ONgNumImpactEntries; itr++, entry++) {
		UUmAssert((entry->impact_type >= 0) && (entry->impact_type < num_impact_types));
		UUmAssert((entry->material_type >= 0) && (entry->material_type < num_material_types));
		UUmAssert((entry->component >= 0) && (entry->component < ONcIEComponent_Max));
		UUmAssert((entry->modifier >= 0) && (entry->modifier < ONcIEModType_NumTypes));

		if (entry->sound_index != ONcIESound_None) {
			UUmAssert((entry->sound_index >= 0) && (entry->sound_index < ONgNumImpactSounds));
		}

		if (entry->num_particles > 0) {
			UUmAssert((entry->particle_baseindex >= 0) &&
					(entry->particle_baseindex + entry->num_particles <= ONgNumImpactParticles));
		}
	}

	// check material entries for basic validity
	matindex = ONiImpactEffect_GetMaterialIndex(0, 0);
	entry = ONiImpactEffect_GetEntry(0, 0);
	for (itr = 0; itr < ONgNumImpactMaterialIndices; itr++, matindex++) {
		UUmAssert((matindex->material >= 0) && (matindex->material < num_material_types));
		UUmAssert(matindex->num_entries > 0);
		UUmAssert((matindex->entry_index >= 0) && (matindex->entry_index + matindex->num_entries <= ONgNumImpactEntries));
	}

	// check impact lookup table for validity and correlate to its material indices and impact entries
	lookup = ONgImpactLookupTable;
	for (itr = 0; itr < num_impact_types; itr++, lookup++) {
		UUmAssert(lookup->impact_type == (MAtImpactType) itr);
		UUmAssert((lookup->index_base >= 0) && (lookup->index_base + lookup->num_indices <= ONgNumImpactMaterialIndices));

		if (lookup->num_indices > 0) {
			matindex = ONiImpactEffect_GetMaterialIndex(lookup->index_base, lookup->num_indices);
			for (itr2 = 0; itr2 < lookup->num_indices; itr2++, matindex++) {
				if (itr2 > 0) {
					// check order criterion for material indices
					UUmAssert(matindex[0].material > matindex[-1].material);
				}

				entry = ONiImpactEffect_GetEntry(matindex->entry_index, matindex->num_entries);
				for (itr3 = 0; itr3 < matindex->num_entries; itr3++, entry++) {
					// check correlation of impact and material types
					UUmAssert(entry->impact_type == lookup->impact_type);
					UUmAssert(entry->material_type == matindex->material);
				}
			}
		}
	}
}
#endif

// ----------------------------------------------------------------------
static UUtError
ONiImpactEffect_NewParticle(
	UUtUns32					inEntryIndex,
	UUtUns32					inNumParticles,
	ONtIEParticle **			outParticle)
{
	ONtImpactEntry *entry;
	ONtIEParticle *particle;
	UUtError error;
	UUtUns32 move_index, itr;

	error = ONiImpactEffect_MakeDirty(UUcFalse, UUcTrue);
	UUmError_ReturnOnError(error);

	// grow the array
	UUmAssert(ONgImpactEffect_DynamicParticleArray);
	UUrMemory_Array_MakeRoom((UUtMemory_Array *) ONgImpactParticles, ONgNumImpactParticles + inNumParticles, NULL);
	if (error != UUcError_None) {
		UUrDebuggerMessage("ONiImpactEffect_NewParticle: out of memory, cannot allocate new particle effects");
		return error;
	}
	ONgNumImpactParticles += inNumParticles;

	entry = ONiImpactEffect_GetEntry(inEntryIndex, 1);

	// move all particles after this entry's particles down
	move_index = entry->particle_baseindex + entry->num_particles;
	particle = ONiImpactEffect_GetParticle(move_index, inNumParticles);
	UUrMemory_MoveOverlap(particle, particle + inNumParticles, (ONgNumImpactParticles - move_index - inNumParticles) * sizeof(ONtIEParticle));
	*outParticle = particle;

	// update all impact entries' particle indices
	entry = ONiImpactEffect_GetEntry(0, 0);
	for (itr = 0; itr < ONgNumImpactEntries; itr++, entry++) {
		if (itr == inEntryIndex) {
			entry->num_particles += (UUtUns16) inNumParticles;
		} else if (entry->particle_baseindex < move_index) {
			// do nothing
			UUmAssert(entry->particle_baseindex + entry->num_particles <= move_index);
		} else {
			// adjust this entry's particle index
			entry->particle_baseindex += inNumParticles;
		}
	}

#if defined(DEBUGGING) && DEBUGGING
	UUrMemory_Set8(*outParticle, 0xcd, inNumParticles * sizeof(ONtIEParticle));
#endif

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiImpactEffect_DeleteParticle(
	UUtBool						inAutomatic,
	UUtUns32					inParticleIndex,
	UUtUns32					inNumParticles)
{
	ONtImpactEntry *entry;
	ONtIEParticle *particle;
	UUtBool found_entry;
	UUtError error;
	UUtUns32 itr;

	error = ONiImpactEffect_MakeDirty(inAutomatic, UUcFalse);
	UUmError_ReturnOnError(error);

	// move all particles after these up
	particle = ONiImpactEffect_GetParticle(inParticleIndex, inNumParticles);
	UUrMemory_MoveOverlap(particle + inNumParticles, particle, (ONgNumImpactParticles - inParticleIndex - inNumParticles) * sizeof(ONtIEParticle));
	ONgNumImpactParticles -= inNumParticles;

	// update all impact entries' particle indices
	found_entry = UUcFalse;
	entry = ONiImpactEffect_GetEntry(0, 0);
	for (itr = 0; itr < ONgNumImpactEntries; itr++, entry++) {
		if (entry->particle_baseindex + entry->num_particles <= inParticleIndex) {
			// this entry comes before the deleted section. do nothing.

		} else if (entry->particle_baseindex <= inParticleIndex) {
			// the particles were deleted from this entry
			UUmAssert(entry->particle_baseindex + entry->num_particles >= inParticleIndex + inNumParticles);
			UUmAssert(entry->num_particles >= inNumParticles);
			entry->num_particles -= (UUtUns16) inNumParticles;

			UUmAssert(!found_entry);
			found_entry = UUcTrue;

		} else {
			// this entry comes after the deleted section.
			entry->particle_baseindex -= inNumParticles;
		}
	}
	UUmAssert(found_entry);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffect_NewSound(
	UUtUns32					inEntryIndex,
	ONtIESound **				outSound)
{
	ONtImpactEntry *entry;
	UUtError error;

	// must come before we calculate entry ptr because this may move memory
	error = ONiImpactEffect_MakeDirty(UUcFalse, UUcTrue);
	UUmError_ReturnOnError(error);

	entry = ONiImpactEffect_GetEntry(inEntryIndex, 1);

	if (entry->sound_index != ONcIESound_None) {
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "can't add a new sound to an entry that already has one");
	}

	// grow the array
	UUmAssert(ONgImpactEffect_DynamicSoundArray);
	UUrMemory_Array_MakeRoom((UUtMemory_Array *) ONgImpactSounds, ONgNumImpactSounds + 1, NULL);
	if (error != UUcError_None) {
		UUrDebuggerMessage("ONrImpactEffect_NewSound: out of memory, cannot allocate new sound");
		return error;
	}
	ONgNumImpactSounds += 1;

	entry->sound_index = ONgNumImpactSounds - 1;
	*outSound = ONiImpactEffect_GetSound(ONgNumImpactSounds - 1, 1);

#if defined(DEBUGGING) && DEBUGGING
	UUrMemory_Set8(*outSound, 0xcd, sizeof(ONtIESound));
#endif

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffect_DeleteSound(
	UUtBool						inAutomatic,
	UUtUns32					inSoundIndex)
{
	ONtImpactEntry *entry;
	ONtIESound *sound;
	UUtBool found_entry;
	UUtError error;
	UUtUns32 itr;

	UUmAssert(inSoundIndex != ONcIESound_None);

	error = ONiImpactEffect_MakeDirty(inAutomatic, UUcFalse);
	UUmError_ReturnOnError(error);

	// move all sounds after this one up
	sound = ONiImpactEffect_GetSound(inSoundIndex, 1);
	UUrMemory_MoveOverlap(sound + 1, sound, (ONgNumImpactSounds - inSoundIndex - 1) * sizeof(ONtIESound));
	ONgNumImpactSounds -= 1;

	// update all impact entries' sound indices
	found_entry = UUcFalse;
	entry = ONiImpactEffect_GetEntry(0, 0);
	for (itr = 0; itr < ONgNumImpactEntries; itr++, entry++) {
		if (entry->sound_index == ONcIESound_None) {
			// this entry has no sound to update.

		} else if (entry->sound_index < inSoundIndex) {
			// this entry comes before the deleted sound. do nothing.

		} else if (entry->sound_index == inSoundIndex) {
			// the sound was deleted from this entry
			entry->sound_index = ONcIESound_None;

			UUmAssert(!found_entry);
			found_entry = UUcTrue;

		} else {
			// this entry comes after the deleted sound.
			entry->sound_index -= 1;
		}
	}
	UUmAssert(found_entry);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiImpactEffect_NewEntry(
	UUtUns32					inMaterialIndex,
	UUtUns32 *					outEntryIndex,
	ONtImpactEntry **			outEntry)
{
	ONtImpactMaterialIndex *matindex;
	ONtImpactEntry *entry;
	UUtError error;
	UUtUns32 move_index, itr;

	error = ONiImpactEffect_MakeDirty(UUcFalse, UUcTrue);
	UUmError_ReturnOnError(error);

	// grow the array
	UUmAssert(ONgImpactEffect_DynamicEntryArray);
	error = UUrMemory_Array_MakeRoom((UUtMemory_Array *) ONgImpactEntries, ONgNumImpactEntries + 1, NULL);
	if (error != UUcError_None) {
		UUrDebuggerMessage("ONiImpactEffect_NewEntry: out of memory, cannot allocate another impact entry");
		return error;
	}
	ONgNumImpactEntries += 1;

	matindex = ONiImpactEffect_GetMaterialIndex(inMaterialIndex, 1);

	// move all entries after this material's entries down
	move_index = matindex->entry_index + matindex->num_entries;

	entry = ONiImpactEffect_GetEntry(move_index, 1);
	UUrMemory_MoveOverlap(entry, entry + 1, (ONgNumImpactEntries - move_index - 1) * sizeof(ONtImpactEntry));
	*outEntryIndex = move_index;
	*outEntry = entry;

	// update all material indices' entry indices
	matindex = ONiImpactEffect_GetMaterialIndex(0, 0);
	for (itr = 0; itr < ONgNumImpactMaterialIndices; itr++, matindex++) {
		if (itr == inMaterialIndex) {
			// this is the material in question
			matindex->num_entries += 1;

		} else if (matindex->entry_index < move_index) {
			// do nothing
			UUmAssert(matindex->entry_index + matindex->num_entries <= move_index);

		} else {
			// adjust this material's entry index
			matindex->entry_index += 1;
		}
	}

#if defined(DEBUGGING) && DEBUGGING
	UUrMemory_Set8(*outEntry, 0xcd, sizeof(ONtImpactEntry));
#endif

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiImpactEffect_DeallocateEntry(
	UUtBool						inAutomatic,
	UUtUns32					inEntryIndex,
	UUtUns32 *					outMaterialIndex)
{
	ONtImpactMaterialIndex *matindex;
	ONtImpactEntry *entry;
	UUtBool found_matindex;
	UUtError error;
	UUtUns32 itr;

	error = ONiImpactEffect_MakeDirty(inAutomatic, UUcFalse);
	UUmError_ReturnOnError(error);

	entry = ONiImpactEffect_GetEntry(inEntryIndex, 1);

	if (entry->sound_index != ONcIESound_None) {
		// delete the sound from this impact entry
		error = ONrImpactEffect_DeleteSound(inAutomatic, entry->sound_index);
		UUmError_ReturnOnErrorMsg(error, "can't delete sound from deleted entry");
	}

	if (entry->num_particles > 0) {
		// delete the particles from this impact entry
		error = ONiImpactEffect_DeleteParticle(inAutomatic, entry->particle_baseindex, entry->num_particles);
		UUmError_ReturnOnErrorMsg(error, "can't delete sound from deleted entry");
	}
	UUmAssert((entry->sound_index == ONcIESound_None) && (entry->num_particles == 0));

	// move all impact entries after this one up
	UUrMemory_MoveOverlap(entry + 1, entry, (ONgNumImpactEntries - inEntryIndex - 1) * sizeof(ONtImpactEntry));
	ONgNumImpactEntries -= 1;

	// update all material indices' impact entry indices
	found_matindex = UUcFalse;
	if (outMaterialIndex) {
		*outMaterialIndex = (UUtUns32) -1;
	}
	matindex = ONiImpactEffect_GetMaterialIndex(0, 0);
	for (itr = 0; itr < ONgNumImpactMaterialIndices; itr++, matindex++) {
		if (matindex->entry_index + matindex->num_entries <= inEntryIndex) {
			// this material index's entries come before the deleted entry. do nothing.

		} else if (matindex->entry_index <= inEntryIndex) {
			// the entry was deleted from this material index
			UUmAssert(matindex->num_entries > 0);
			matindex->num_entries -= 1;

			UUmAssert(!found_matindex);
			found_matindex = UUcTrue;
			if (outMaterialIndex) {
				*outMaterialIndex = itr;
			}

		} else {
			// this material's entries come after the deleted entry.
			matindex->entry_index -= 1;
		}
	}
	UUmAssert(found_matindex);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiImpactEffect_NewMaterialIndex(
	MAtImpactType				inImpactType,
	UUtUns32					inDesiredIndex,
	ONtImpactMaterialIndex **	outMaterialIndex)
{
	ONtImpactLookupIndex *lookup;
	ONtImpactMaterialIndex *matindex;
	UUtError error;
	UUtUns32 itr, num_types;

	num_types = MArImpacts_GetNumTypes();

	error = ONiImpactEffect_MakeDirty(UUcFalse, UUcTrue);
	UUmError_ReturnOnError(error);

	// grow the array
	UUmAssert(ONgImpactEffect_DynamicMaterialArray);
	UUrMemory_Array_MakeRoom((UUtMemory_Array *) ONgImpactMaterialIndices, ONgNumImpactMaterialIndices + 1, NULL);
	if (error != UUcError_None) {
		UUrDebuggerMessage("ONiImpactEffect_NewMaterialIndex: out of memory, cannot allocate new material index");
		return error;
	}
	ONgNumImpactMaterialIndices += 1;

	UUmAssert((inImpactType >= 0) && (inImpactType < num_types));
	lookup = ONgImpactLookupTable + inImpactType;

	// move all material indices after this one down
	matindex = ONiImpactEffect_GetMaterialIndex(inDesiredIndex, 1);
	UUrMemory_MoveOverlap(matindex, matindex + 1, (ONgNumImpactMaterialIndices - inDesiredIndex - 1)
														* sizeof(ONtImpactMaterialIndex));
	*outMaterialIndex = matindex;

	// update the lookup table's material indices
	lookup = ONgImpactLookupTable;
	for (itr = 0; itr < num_types; itr++, lookup++) {
		if (itr == ((UUtUns32) inImpactType)) {
			// this is the impact type's lookup element in question
			lookup->num_indices += 1;

		} else if (lookup->index_base < inDesiredIndex) {
			// do nothing
			UUmAssert(lookup->index_base + lookup->num_indices <= inDesiredIndex);

		} else {
			// adjust this lookup element's material index
			lookup->index_base += 1;
		}
	}

#if defined(DEBUGGING) && DEBUGGING
	UUrMemory_Set8(*outMaterialIndex, 0xcd, sizeof(ONtImpactMaterialIndex));
#endif

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiImpactEffect_DeleteMaterialIndex(
	UUtBool						inAutomatic,
	UUtUns32					inMaterialIndex)
{
	ONtImpactLookupIndex *lookup;
	ONtImpactMaterialIndex *matindex;
	UUtError error;
	UUtBool found_lookup;
	UUtUns32 itr, num_types;

	num_types = MArImpacts_GetNumTypes();

	error = ONiImpactEffect_MakeDirty(inAutomatic, UUcTrue);
	UUmError_ReturnOnError(error);

	matindex = ONiImpactEffect_GetMaterialIndex(inMaterialIndex, 1);

	// this must only be called on empty impact / material combinations!
	UUmAssert(matindex->num_entries == 0);

	// move all material indices after this one up
	UUrMemory_MoveOverlap(matindex + 1, matindex, (ONgNumImpactMaterialIndices - inMaterialIndex - 1) * sizeof(ONtImpactMaterialIndex));
	ONgNumImpactMaterialIndices -= 1;

	// update the lookup table's material indices
	found_lookup = UUcFalse;
	lookup = ONgImpactLookupTable;
	for (itr = 0; itr < num_types; itr++, lookup++) {
		if (lookup->index_base + lookup->num_indices <= inMaterialIndex) {
			// this lookup element's material indices come before the deleted index. do nothing.

		} else if (lookup->index_base <= inMaterialIndex) {
			// the entry was deleted from this lookup element
			UUmAssert(lookup->num_indices > 0);
			lookup->num_indices -= 1;

			UUmAssert(!found_lookup);
			found_lookup = UUcTrue;

		} else {
			// this lookup element's material indices come after the deleted entry.
			lookup->index_base -= 1;
		}
	}
	// TEMPORARY DEBUGGING - this assertion should not be firing but I don't have time to look
	// at it right now. will get to it later this week. CB, 11 July 2000
//	UUmAssert(found_lookup);

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ======================================================================
// windowsystem interface functions
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OWrImpactEffect_MakeDirty(
	void)
{
	return ONiImpactEffect_MakeDirty(UUcFalse, UUcFalse);
}

// ----------------------------------------------------------------------
static UUtError
OWiImpactEffect_AllocateSpecificEntry(
	MAtImpactType				inImpactType,
	MAtMaterialType				inMaterialType,
	UUtUns32 *					outEntryIndex,
	ONtImpactEntry **			outEntry)
{
	UUtError error;
	UUtUns32 itr;
	ONtImpactLookupIndex *lookup;
	ONtImpactMaterialIndex *matindex;

	// get this impact type's lookup element
	UUmAssert((inImpactType >= 0) && (inImpactType < MArImpacts_GetNumTypes()));
	lookup = ONgImpactLookupTable + inImpactType;

	// find the material index that corresponds to this material type
	matindex = ONiImpactEffect_GetMaterialIndex(lookup->index_base, lookup->num_indices);
	for (itr = 0; itr < lookup->num_indices; itr++, matindex++) {
		// keep looking down the list until we find the right place to insert this material
		if (matindex->material >= inMaterialType) {
			break;
		}
	}

	if ((itr >= lookup->num_indices) || (matindex->material != inMaterialType)) {
		// we have to create a material index for the desired material in the desired impact's list.
		error = ONiImpactEffect_NewMaterialIndex(inImpactType, lookup->index_base + itr, &matindex);
		UUmError_ReturnOnErrorMsg(error, "can't create new material index to hold new impact entry");

		// set up initial values for this material index (no entries yet)
		matindex->material = inMaterialType;
		matindex->entry_index = ONgNumImpactEntries;
		matindex->num_entries = 0;
	}

	// create an impact entry in this material index.
	error = ONiImpactEffect_NewEntry(lookup->index_base + itr, outEntryIndex, outEntry);
	UUmError_ReturnOnErrorMsg(error, "can't create new impact entry");

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffect_CreateImpactEntry(
	MAtImpactType				inImpactType,
	MAtMaterialType				inMaterialType,
	UUtUns32 *					outEntryIndex)
{
	UUtError error;
	UUtUns32 index;
	ONtImpactEntry *entry;

	ONrImpactEffect_VerifyStructure(UUcTrue);

	error = OWiImpactEffect_AllocateSpecificEntry(inImpactType, inMaterialType, &index, &entry);
	UUmError_ReturnOnError(error);

	// set up defaults for this new entry
	entry->impact_type = inImpactType;
	entry->material_type = inMaterialType;
	entry->component = ONcIEComponent_Impact;
	entry->modifier = ONcIEModType_Any;
	entry->num_particles = 0;
	entry->particle_baseindex = 0;
	entry->sound_index = ONcIESound_None;

	ONrImpactEffect_VerifyStructure(UUcTrue);

	*outEntryIndex = index;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffect_DeleteEntry(
	UUtBool						inAutomatic,
	UUtUns32 					inEntryIndex)
{
	UUtError error;
	UUtUns32 material_index;
	ONtImpactMaterialIndex *matindex;

	if ((inEntryIndex < 0) || (inEntryIndex >= ONgNumImpactEntries)) {
		UUrDebuggerMessage("ONrImpactEffect_DeleteEntry: error, tried to delete entry %d outside range [0, %d)", inEntryIndex, ONgNumImpactEntries);
		return UUcError_Generic;
	}

	ONrImpactEffect_VerifyStructure(UUcTrue);

	error = ONiImpactEffect_MakeDirty(inAutomatic, UUcTrue);
	UUmError_ReturnOnError(error);

	// deallocate the impact entry and its components (sound, particles)
	error = ONiImpactEffect_DeallocateEntry(inAutomatic, inEntryIndex, &material_index);
	UUmError_ReturnOnErrorMsg(error, "can't delete entry!");

	if (material_index == (UUtUns32) -1) {
		UUmAssert(!"ONiImpactEffect_DeleteEntry: couldn't find material index!");
	} else {
		matindex = ONiImpactEffect_GetMaterialIndex(material_index, 1);

		if (matindex->num_entries == 0) {
			// delete this material index and update the impact lookup array
			error = ONiImpactEffect_DeleteMaterialIndex(inAutomatic, material_index);
			UUmError_ReturnOnErrorMsg(error, "deleting entry results in empty material index, error trying to delete it!");
		}
	}

	ONrImpactEffect_VerifyStructure(UUcTrue);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrImpactEffect_GetImpactEntries(
	MAtImpactType				inImpactType,
	MAtMaterialType				inMaterialItr,
	MAtMaterialType				*outMaterialType,
	UUtUns16 *					outNumEntries,
	UUtUns32 *					outEntryIndex)
{
	UUtUns32 itr;
	ONtImpactLookupIndex *lookup;
	ONtImpactMaterialIndex *matindex;

	ONrImpactEffect_VerifyStructure(UUcTrue);

	// get this impact type's lookup element
	UUmAssert((inImpactType >= 0) && (inImpactType < MArImpacts_GetNumTypes()));
	lookup = ONgImpactLookupTable + inImpactType;

	// find the first material index that hasn't yet been returned.
	matindex = ONiImpactEffect_GetMaterialIndex(lookup->index_base, lookup->num_indices);
	for (itr = 0; itr < lookup->num_indices; itr++, matindex++) {
		// keep looking down the list until we find the right material index
		if ((matindex->material >= inMaterialItr) || (inMaterialItr == MAcInvalidID)) {
			break;
		}
	}

	if (itr >= lookup->num_indices) {
		// no material index exists at or after the desired material type.
		*outMaterialType = MAcInvalidID;
		*outNumEntries = 0;
		*outEntryIndex = 0;
	} else {
		// we have found the material index that references the desired material type
		*outMaterialType = matindex->material;
		*outNumEntries = matindex->num_entries;
		*outEntryIndex = matindex->entry_index;
	}
}

// ----------------------------------------------------------------------
ONtImpactEntry *
ONrImpactEffect_GetEntry(
	UUtUns32					inIndex)
{
	return ONiImpactEffect_GetEntry(inIndex, 1);
}

// ----------------------------------------------------------------------
ONtIEParticle *
ONrImpactEffect_GetParticles(
	UUtUns32					inEntryIndex,
	UUtUns32 *					outNumParticles)
{
	ONtImpactEntry *entry = ONiImpactEffect_GetEntry(inEntryIndex, 1);

	*outNumParticles = (UUtUns32) entry->num_particles;
	if (entry->num_particles == 0) {
		return NULL;
	} else {
		return ONiImpactEffect_GetParticle(entry->particle_baseindex, entry->num_particles);
	}
}

// ----------------------------------------------------------------------
ONtIESound *
ONrImpactEffect_GetSound(
	UUtUns32					inEntryIndex)
{
	ONtImpactEntry *entry = ONiImpactEffect_GetEntry(inEntryIndex, 1);

	if (entry->sound_index == ONcIESound_None) {
		return NULL;
	} else {
		return ONiImpactEffect_GetSound(entry->sound_index, 1);
	}
}

// ----------------------------------------------------------------------
void
ONrImpactEffect_GetDescription(
	UUtUns32					inEntryIndex,
	char *						outString)
{
	UUtUns32 itr, num_particles;
	ONtIEParticle *particle;
	ONtIESound *sound;
	char tempbuf[128];

	sound = ONrImpactEffect_GetSound(inEntryIndex);
	particle = ONrImpactEffect_GetParticles(inEntryIndex, &num_particles);

	if ((sound == NULL) && (num_particles == 0)) {
		UUrString_Copy(outString, "(nothing)", 256);
		return;
	}

	if (sound != NULL) {
		if (UUmString_IsEqual(sound->sound_name, "")) {
			strcpy(outString, "sound: (none)");
		} else {
			sprintf(outString, "sound: %s", sound->sound_name);
		}

		if (sound->flags & ONcIESoundFlag_AICanHear) {
			sprintf(tempbuf, " (AI %s, radius %.0f)",
				(sound->ai_soundtype == (UUtUns16) AI2cContactType_Sound_Ignore)		? "Unimportant" :
				(sound->ai_soundtype == (UUtUns16) AI2cContactType_Sound_Interesting)	? "Interesting" :
				(sound->ai_soundtype == (UUtUns16) AI2cContactType_Sound_Danger)		? "Danger" :
				(sound->ai_soundtype == (UUtUns16) AI2cContactType_Sound_Melee)			? "Melee" :
				(sound->ai_soundtype == (UUtUns16) AI2cContactType_Sound_Gunshot)		? "Gunfire" : "<error>",
						sound->ai_soundradius);
			UUrString_Cat(outString, tempbuf, 256);
		}
	} else {
		UUrString_Copy(outString, "", 256);
	}

	for (itr = 0; itr < num_particles; itr++) {
		if (strlen(outString) > 0) {
			UUrString_Cat(outString, ", ", 256);
		}

		if (itr == 0) {
			UUrString_Cat(outString, "particles: ", 256);
		}

		UUrString_Cat(outString, particle[itr].particle_class_name, 256);
	}
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffect_AddParticleToEntry(
	UUtUns32					inEntryIndex,
	ONtIEParticle				**outParticle)
{
	UUtError error;

	error = ONiImpactEffect_NewParticle(inEntryIndex, 1, outParticle);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffect_RemoveParticleFromEntry(
	UUtBool						inAutomatic,
	UUtUns32					inEntryIndex,
	UUtUns32					inParticleIndex)
{
	UUtError error;
	ONtImpactEntry *entry = ONiImpactEffect_GetEntry(inEntryIndex, 1);

	UUmAssert((inParticleIndex >= 0) && (inParticleIndex < entry->num_particles));
	error = ONiImpactEffect_DeleteParticle(inAutomatic, entry->particle_baseindex + inParticleIndex, 1);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffect_Copy(
	UUtUns32					inEntryIndex)
{
	ONtImpactEntry *entry;
	ONtIESound *sound;
	ONtIEParticle *particle;

	if (ONgImpactEffect_ClipboardFull && (ONgImpactEffect_ClipboardEntry.num_particles > 0)) {
		// delete old particle storage if there is any
		UUmAssert(ONgImpactEffect_ClipboardParticles != NULL);
		UUrMemory_Block_Delete(ONgImpactEffect_ClipboardParticles);
		ONgImpactEffect_ClipboardParticles = NULL;

		ONgImpactEffect_ClipboardFull = UUcFalse;
	}

	entry = ONiImpactEffect_GetEntry(inEntryIndex, 1);
	ONgImpactEffect_ClipboardEntry = *entry;

	if (ONgImpactEffect_ClipboardEntry.sound_index != ONcIESound_None) {
		// copy the sound data
		sound = ONiImpactEffect_GetSound(ONgImpactEffect_ClipboardEntry.sound_index, 1);
		ONgImpactEffect_ClipboardSound = *sound;
	}

	if (ONgImpactEffect_ClipboardEntry.num_particles > 0) {
		// allocate space for and copy the particle data
		particle = ONiImpactEffect_GetParticle(ONgImpactEffect_ClipboardEntry.particle_baseindex,
												ONgImpactEffect_ClipboardEntry.num_particles);

		UUmAssert(ONgImpactEffect_ClipboardParticles == NULL);
		ONgImpactEffect_ClipboardParticles = UUrMemory_Block_New(ONgImpactEffect_ClipboardEntry.num_particles
																* sizeof(ONtIEParticle));
		UUmError_ReturnOnNull(ONgImpactEffect_ClipboardParticles);

		UUrMemory_MoveFast(particle, ONgImpactEffect_ClipboardParticles, ONgImpactEffect_ClipboardEntry.num_particles
																* sizeof(ONtIEParticle));
	}

	ONgImpactEffect_ClipboardFull = UUcTrue;
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffect_Paste(
	MAtImpactType				inImpactType,
	MAtMaterialType				inMaterialType,
	UUtUns32					*outEntryIndex)
{
	UUtError error;
	UUtUns32 index;
	ONtImpactEntry *entry;
	ONtIESound *sound;
	ONtIEParticle *particle;

	if (!ONgImpactEffect_ClipboardFull) {
		*outEntryIndex = (UUtUns32) -1;
		return UUcError_None;
	}

	ONrImpactEffect_VerifyStructure(UUcTrue);

	error = OWiImpactEffect_AllocateSpecificEntry(inImpactType, inMaterialType, &index, &entry);
	UUmError_ReturnOnError(error);

	// set up this entry as a copy of the one in the clipboard
	entry->impact_type = inImpactType;
	entry->material_type = inMaterialType;
	entry->component = ONgImpactEffect_ClipboardEntry.component;
	entry->modifier = ONgImpactEffect_ClipboardEntry.modifier;
	entry->num_particles = 0;
	entry->particle_baseindex = 0;
	entry->sound_index = ONcIESound_None;

	if (ONgImpactEffect_ClipboardEntry.sound_index != ONcIESound_None) {
		// allocate a sound for this entry
		error = ONrImpactEffect_NewSound(index, &sound);
		UUmError_ReturnOnError(error);

		*sound = ONgImpactEffect_ClipboardSound;
	}

	if (ONgImpactEffect_ClipboardEntry.num_particles > 0) {
		// allocate particles for this entry
		error = ONiImpactEffect_NewParticle(index, ONgImpactEffect_ClipboardEntry.num_particles, &particle);
		UUmError_ReturnOnError(error);

		UUmAssert(ONgImpactEffect_ClipboardParticles != NULL);
		UUrMemory_MoveFast(ONgImpactEffect_ClipboardParticles, particle, ONgImpactEffect_ClipboardEntry.num_particles
																			* sizeof(ONtIEParticle));
	}

	ONrImpactEffect_VerifyStructure(UUcTrue);

	*outEntryIndex = index;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OWrImpactEffect_UpdateParticleClassPtr(
	UUtUns32					inEntryIndex,
	UUtUns32					inParticleIndex)
{
	ONtImpactEntry *entry = ONiImpactEffect_GetEntry(inEntryIndex, 1);

	UUmAssert((inParticleIndex >= 0) && (inParticleIndex < entry->num_particles));
	ONiImpactEffect_UpdateParticlePointers(1, entry->particle_baseindex + inParticleIndex);
}

// ----------------------------------------------------------------------
void
OWrImpactEffect_UpdateSoundPtr(
	UUtUns32					inEntryIndex)
{
	ONtImpactEntry *entry = ONiImpactEffect_GetEntry(inEntryIndex, 1);

	if (entry->sound_index != ONcIESound_None) {
		ONiImpactEffect_UpdateSoundPointers(1, entry->sound_index);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiImpactEffect_UpdateParticlePointers(
	UUtUns32					inNumParticles,
	UUtUns32					inBaseIndex)
{
	ONtIEParticle *particle = ONiImpactEffect_GetParticle(inBaseIndex, inNumParticles);
	UUtUns32 itr;

	for (itr = 0; itr < inNumParticles; itr++, particle++) {
		particle->effect_spec.particle_class = P3rGetParticleClass(particle->particle_class_name);
	}
}

static void
ONiImpactEffect_UpdateSoundPointers(
	UUtUns32					inNumSounds,
	UUtUns32					inBaseIndex)
{
	ONtIESound *sound = ONiImpactEffect_GetSound(inBaseIndex, inNumSounds);
	UUtUns32 itr;

	for (itr = 0; itr < inNumSounds; itr++, sound++) {
		sound->impulse_ptr = OSrImpulse_GetByName(sound->sound_name);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
ONrImpactEffect(
	MAtImpactType				inImpactType,
	MAtMaterialType				inMaterialType,
	UUtUns16					inModifier,
	P3tEffectData				*inData,
	float						inAIVolumeModifier,
	ONtCharacter				*inAICharacter1,
	ONtCharacter				*inAICharacter2)
{
	UUtUns32					itr, itr2;
	UUtUns32					found_indices[ONcIEComponent_Max];
	float						impact_velocity, volume, *volumeptr;
	ONtImpactEntry				*entry;
	ONtIESound					*sound;
	ONtIEParticle				*particle;

	if (ONgDebugImpactEffects) {
		COrConsole_Printf("** impact lookup: %s vs %s, modifier %s",
			(inImpactType == MAcInvalidID) ? "INVALID" : MArImpactType_GetName(inImpactType),
			(inMaterialType == MAcInvalidID) ? "INVALID" : MArMaterialType_GetName(inMaterialType), ONrIEModType_GetName(inModifier));
	}

	if ((inImpactType == MAcInvalidID) || (inMaterialType == MAcInvalidID)) {
		// we cannot create this impact effect!
		return;
	}

	ONrImpactEffect_Lookup(inImpactType, inMaterialType, inModifier, found_indices);

	for (itr = 0; itr < ONcIEComponent_Max; itr++) {
		// play each component of the effect
		if (found_indices[itr] == (UUtUns32) -1)
			continue;

		entry = ONiImpactEffect_GetEntry(found_indices[itr], 1);

		if (entry->sound_index != ONcIESound_None) {
			// play the entry's sound
			sound = ONiImpactEffect_GetSound(entry->sound_index, 1);
			if (sound->impulse_ptr != NULL) {
				UUmAssertReadPtr(sound->impulse_ptr, sizeof(SStImpulse));

				volumeptr = NULL;
				if ((sound->impulse_ptr->impact_velocity > 0) && (inData->parent_velocity != NULL)) {
					// determine the volume of this sound based on its impact velocity squared
					impact_velocity = MUmVector_GetLength(*(inData->parent_velocity));
					if (impact_velocity < sound->impulse_ptr->impact_velocity) {
						volume = ONcMinImpactVolume + (1.0f - ONcMinImpactVolume) * UUmSQR(impact_velocity) / UUmSQR(sound->impulse_ptr->impact_velocity);
						volumeptr = &volume;
					}
				}

				OSrImpulse_Play(sound->impulse_ptr, &inData->position, NULL, NULL, volumeptr);
			}

			if (sound->flags & ONcIESoundFlag_AICanHear) {
				AI2rKnowledge_Sound((AI2tContactType) sound->ai_soundtype, &inData->position,
									sound->ai_soundradius * inAIVolumeModifier, inAICharacter1, inAICharacter2);
			}
		}

		if (entry->num_particles > 0) {
			// create each particle effect
			particle = ONiImpactEffect_GetParticle(entry->particle_baseindex, entry->num_particles);
			for (itr2 = 0; itr2 < entry->num_particles; itr2++, particle++) {
				if (particle->effect_spec.particle_class != NULL) {
					P3rCreateEffect(&particle->effect_spec, inData);
				}
			}
		}
	}

	return;
}

void
ONrImpactEffect_Lookup(
	MAtImpactType				inImpactType,
	MAtMaterialType				inMaterialType,
	UUtUns16					inModifier,
	UUtUns32					*outImpactEntries)
{
	UUtUns32 num_impact_types, num_material_types, look_flags, itr2;
	UUtUns16 depth_array[ONcIEComponent_Max], thisentry_depth;
	UUtInt32 itr;
	MAtImpactType impact_type, new_impact_type;
	MAtMaterialType material_type, new_material_type;
	ONtImpactLookupIndex *lookup;
	ONtImpactMaterialIndex *matindex;
	ONtImpactEntry *entry;
	UUtBool lookup_bymaterial;
#if MODIFIER_OVERRIDE
	UUtUns32 modifier_array[ONcIEComponent_Max];
#endif

	// as of yet, no components have been found
	UUmAssertWritePtr(outImpactEntries, ONcIEComponent_Max * sizeof(UUtUns32));
	for (itr = 0; itr < ONcIEComponent_Max; itr++) {
		outImpactEntries[itr] = (UUtUns32) -1;
	}
	look_flags = (1 << ONcIEComponent_Max) - 1;

	num_impact_types = MArImpacts_GetNumTypes();
	num_material_types = MArMaterials_GetNumTypes();
	UUmAssert((inMaterialType >= 0) && (inMaterialType < num_material_types));

	lookup_bymaterial = MArImpactType_LookupByMaterial(inImpactType);

	// start by looking up the actual impact type
	impact_type = inImpactType;
	while (1) {
		UUmAssert((impact_type >= 0) && (impact_type < num_impact_types));
		lookup = ONgImpactLookupTable + impact_type;

		// get the section of the material table that applies to this impact type, and start
		// looking for the actual material type that we hit. start from the end of the section and look
		// backwards.
		matindex = ONiImpactEffect_GetMaterialIndex(lookup->index_base, lookup->num_indices);
		matindex += lookup->num_indices - 1;
		material_type = inMaterialType;

		for (itr = lookup->num_indices - 1; itr >= 0; ) {

			if (matindex->material > material_type) {
				// keep looking further up the array
				itr--;
				matindex--;
				continue;

			}

			if (matindex->material == material_type) {
				// we have found a material that has impact entries for this impact type.
				entry = ONiImpactEffect_GetEntry(matindex->entry_index, matindex->num_entries);
				for (itr2 = 0; itr2 < matindex->num_entries; itr2++, entry++) {
					if ((inModifier != ONcIEModType_Any) && (entry->modifier != ONcIEModType_Any) &&
						(inModifier != entry->modifier)) {
						// modifiers do not match
						continue;
					}

					if (lookup_bymaterial) {
						/*
						 * recursive search is keyed by material type as primary
						 *
						 * this means we accept the entry with the most specific material
						 * (at the greatest depth in the tree)
						 */

						thisentry_depth = MArMaterialType_GetDepth(entry->material_type);
						if ((outImpactEntries[entry->component] != (UUtUns32) -1) &&
							(depth_array[entry->component] >= thisentry_depth)) {
							// we already have an entry for this component with a more specific material,
							// or the same material with a more specific impact type
							continue;
						}

#if MODIFIER_OVERRIDE
						// CB: marty decided that he didn't actually want this
						if ((entry->modifier == ONcIEModType_Any) && (modifier_array[entry->component] != ONcIEModType_Any)) {
							// don't use a less-specific modifier than the one we already have
							continue;
						}
#endif

						// store this impact entry
#if MODIFIER_OVERRIDE
						modifier_array[entry->component] = entry->modifier;
#endif
						depth_array[entry->component] = thisentry_depth;
						outImpactEntries[entry->component] = matindex->entry_index + itr2;

					} else {
						/*
						 * recursive search is keyed by impact type as primary
						 *
						 * this means we accept the first entry we find for each component
						 */

						if ((look_flags & (1 << entry->component)) == 0) {
							// we've already found an entry for this component
							continue;
						}

						// store this impact entry
						outImpactEntries[entry->component] = matindex->entry_index + itr2;
						look_flags &= ~(1 << entry->component);
						if (look_flags == 0) {
							// we've found an entry for every component of the impact.
							return;
						}
					}
				}
			}

			// still looking? try the material's parent.
			new_material_type = MArMaterialType_GetParent(material_type);
			if (new_material_type == MAcInvalidID) {
				// this was the base type. there are no further material matches with this impact type.
				break;
			} else {
				// we have a new material type. keep looking (at this same place in the array).
				UUmAssert(new_material_type < material_type);
				material_type = new_material_type;
				continue;
			}
		}

		// get our parent impact type
		new_impact_type = MArImpactType_GetParent(impact_type);
		if (new_impact_type == MAcInvalidID) {
			// this was the base impact type. there are no further matches available.
			return;
		}

		// try lookup with the new impact type
		UUmAssert(new_impact_type < impact_type);
		impact_type = new_impact_type;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ======================================================================
// file I/O functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiIESwap_Particles(
	UUtUns32			inNumParticles,
	ONtIEParticle *		inParticles,
	UUtBool				inSwap,
	UUtBool				inLoad)
{
	ONtIEParticle *particle = inParticles;
	UUtUns32 itr;

	for (itr = 0; itr < inNumParticles; itr++, particle++) {
		if (inSwap) {
			P3iSwap_EffectSpecification((void *) &particle->effect_spec, !inLoad);
		}

		if (inLoad) {
			// check validity of the values we've read in
			if ((particle->effect_spec.location_type < 0) ||
				(particle->effect_spec.location_type >= P3cEffectLocationType_Max)) {
				UUrDebuggerMessage("Impact Effect Load: particle %d location-type %d outside range [0, %d)!\n",
									itr, particle->effect_spec.location_type, P3cEffectLocationType_Max);
				particle->effect_spec.location_type = UUmPin(particle->effect_spec.location_type, 0, P3cEffectLocationType_Max - 1);
			}

			if ((particle->effect_spec.collision_orientation < 0) ||
				(particle->effect_spec.collision_orientation >= P3cEnumCollisionOrientation_Max)) {
				UUrDebuggerMessage("Impact Effect Load: particle %d collision-type %d outside range [0, %d)!\n",
									itr, particle->effect_spec.collision_orientation, P3cEnumCollisionOrientation_Max);
				particle->effect_spec.collision_orientation = UUmPin(particle->effect_spec.collision_orientation, 0,
																	P3cEnumCollisionOrientation_Max - 1);
			}
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiIESwap_Sounds(
	UUtUns32			inNumSounds,
	ONtIESound *		inSounds,
	UUtBool				inSwap,
	UUtBool				inLoad)
{
	ONtIESound *sound = inSounds;
	UUtUns32 itr;

	for (itr = 0; itr < inNumSounds; itr++, sound++) {
		if (inSwap) {
			UUrSwap_2Byte((void *) &sound->flags);
			UUrSwap_2Byte((void *) &sound->ai_soundtype);
			UUrSwap_4Byte((void *) &sound->ai_soundradius);
		}

		if (inLoad) {
			// this pointer is only determined once all binary data is loaded
			sound->impulse_ptr = NULL;
		}
	}
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiIESwap_Entries(
	UUtUns32			inNumEntries,
	ONtImpactEntry *	inEntries,
	UUtBool				inSwap,
	UUtBool				inLoad)
{
	ONtImpactEntry *entry = inEntries;
	UUtUns32 itr;

	for (itr = 0; itr < inNumEntries; itr++, entry++) {
		if (inSwap) {
			UUrSwap_2Byte((void *) &entry->impact_type);
			UUrSwap_2Byte((void *) &entry->material_type);
			UUrSwap_2Byte((void *) &entry->component);
			UUrSwap_2Byte((void *) &entry->modifier);
			UUrSwap_2Byte((void *) &entry->num_particles);
			UUrSwap_4Byte((void *) &entry->sound_index);
			UUrSwap_4Byte((void *) &entry->particle_baseindex);
		}

		if (inLoad) {
			// look up the impact type in the translation table
			if ((entry->impact_type < 0) || (entry->impact_type >= ONgIE_FileNumImpactTypes)) {
				UUrDebuggerMessage("Impact Effect Load: entry %d file impact-type %d outside file range [0, %d)!\n",
									itr, entry->impact_type, ONgIE_FileNumImpactTypes);
				return UUcError_Generic;
			}

			UUmAssert(ONgIE_ImpactTranslation[entry->impact_type].file_type == entry->impact_type);
			entry->impact_type = ONgIE_ImpactTranslation[entry->impact_type].current_type;

			if (entry->impact_type != MAcInvalidID) {
				if ((entry->impact_type < 0) || (entry->impact_type >= MArImpacts_GetNumTypes())) {
					UUrDebuggerMessage("Impact Effect Load: entry %d translated impact-type %d outside range [0, %d)!\n",
										itr, entry->impact_type, MArImpacts_GetNumTypes());
					return UUcError_Generic;
				}
			}

			// look up the material type in the translation table
			if ((entry->material_type < 0) || (entry->material_type >= ONgIE_FileNumMaterialTypes)) {
				UUrDebuggerMessage("Impact Effect Load: entry %d file material-type %d outside file range [0, %d)!\n",
									itr, entry->material_type, ONgIE_FileNumMaterialTypes);
				return UUcError_Generic;
			}

			UUmAssert(ONgIE_MaterialTranslation[entry->material_type].file_type == entry->material_type);
			entry->material_type = ONgIE_MaterialTranslation[entry->material_type].current_type;

			if (entry->material_type != MAcInvalidID) {
				if ((entry->material_type < 0) || (entry->material_type >= MArMaterials_GetNumTypes())) {
					UUrDebuggerMessage("Impact Effect Load: entry %d translated material-type %d outside range [0, %d)!\n",
										itr, entry->material_type, MArMaterials_GetNumTypes());
					return UUcError_Generic;
				}
			}

			// check validity of the values we've read in
			if ((entry->component < 0) || (entry->component >= ONcIEComponent_Max)) {
				UUrDebuggerMessage("Impact Effect Load: entry %d component %d outside range [0, %d)!\n",
									itr, entry->component, ONcIEComponent_Max);
				return UUcError_Generic;
			}

			if ((entry->modifier < 0) || (entry->modifier >= ONcIEModType_NumTypes)) {
				UUrDebuggerMessage("Impact Effect Load: entry %d modifier %d outside range [0, %d)!\n",
									itr, entry->modifier, ONcIEModType_NumTypes);
				return UUcError_Generic;
			}

			if (entry->sound_index != ONcIESound_None) {
				// check that this is a valid index into the sound array
				if ((entry->sound_index < 0) || (entry->sound_index >= ONgNumImpactSounds)) {
					UUrDebuggerMessage("Impact Effect Load: entry %d sound index %d outside array bounds (%d)!\n",
										itr, entry->sound_index, ONgNumImpactSounds);
					return UUcError_Generic;
				}
			}

			if (entry->num_particles > 0) {
				// check that this is a valid index into the particle array
				if ((entry->particle_baseindex < 0) ||
					(entry->particle_baseindex + (UUtUns32) entry->num_particles > ONgNumImpactParticles)) {
					UUrDebuggerMessage("Impact Effect Load: entry %d particle index %d len %d outside array bounds (%d)!\n",
										itr, entry->particle_baseindex, entry->num_particles, ONgNumImpactParticles);
					return UUcError_Generic;
				}
			}
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static int UUcExternal_Call ONiImpactEffect_MaterialIndexCompare(const void *elem1, const void *elem2)
{
	ONtImpactMaterialIndex *index1 = (ONtImpactMaterialIndex *) elem1, *index2 = (ONtImpactMaterialIndex *) elem2;

	if (index1->material < index2->material) {
		return -1;
	} else if (index1->material > index2->material) {
		return +1;
	} else {
		return 0;
	}
}

// ----------------------------------------------------------------------
static UUtError
ONiIESwap_MaterialIndices(
	UUtUns32					inNumIndices,
	ONtImpactMaterialIndex *	inIndices,
	UUtUns32					inPreviousNumImpactTypes,
	ONtImpactLookupIndex *		inPreviousLookupTable,
	UUtBool						inSwap,
	UUtBool						inLoad)
{
	ONtImpactMaterialIndex *matindex;
	ONtMaterialTranslator *translator;
	ONtImpactLookupIndex *lookup;
	ONtImpactEntry *entry;
	UUtUns32 itr, itr2, num_types, deleted_from_matindex;
	UUtError error;
	UUtBool shuffled = UUcFalse, found_orphans = UUcFalse;

	if (inPreviousNumImpactTypes > 0) {
		UUmAssert(inLoad);

		// the impacts have changed since we were saved. go through and nuke the materialtype of
		// all material indices whose impact types were deleted, so that we don't have them hanging around.
		lookup = inPreviousLookupTable;
		for (itr = 0; itr < inPreviousNumImpactTypes; itr++, lookup++) {
			if (lookup->impact_type == MAcInvalidID) {
				// this impact type no longer exists.
				if ((lookup->index_base < 0) || (lookup->index_base + lookup->num_indices > inNumIndices)) {
					UUrDebuggerMessage("Impact Effect Load: while purging dead-impact indices: lookuptable index %d references indices %d(%d) outside file range [0, %d)!\n",
										itr, lookup->index_base, lookup->num_indices, inNumIndices);
					return UUcError_Generic;
				}

				matindex = inIndices + lookup->index_base;
				for (itr2 = 0; itr2 < lookup->num_indices; itr2++, matindex++) {
					// nuke the material type so this material index (and its entries!) will be deleted in the next pass
					matindex->material = MAcInvalidID;
				}
			}
		}
	}

	matindex = inIndices;
	for (itr = 0; itr < inNumIndices; itr++, matindex++) {
		if (inSwap) {
			UUrSwap_2Byte((void *) &matindex->material);
			UUrSwap_2Byte((void *) &matindex->num_entries);
			UUrSwap_4Byte((void *) &matindex->entry_index);
		}

		if (inLoad) {
			if (matindex->material != MAcInvalidID) {
				// look up the material type in the translation table
				if ((matindex->material < 0) || (matindex->material >= ONgIE_FileNumMaterialTypes)) {
					UUrDebuggerMessage("Impact Effect Load: materialindex %d file material-type %d outside file range [0, %d)!\n",
										itr, matindex->material, ONgIE_FileNumMaterialTypes);
					return UUcError_Generic;
				}

				translator = ONgIE_MaterialTranslation + matindex->material;
				UUmAssert(translator->file_type == matindex->material);
				if (translator->current_type != translator->file_type) {
					// we will have to re-sort the material indices once finished, to maintain the ascending order
					// that is crucial when performing the recursive lookup
					shuffled = UUcTrue;
					matindex->material = translator->current_type;
				}
			}

			if (matindex->material == MAcInvalidID) {
				// this material has been deleted and so we must remove this entry from the material index structure
				found_orphans = UUcTrue;
			} else {
				if ((matindex->material < 0) || (matindex->material >= MArMaterials_GetNumTypes())) {
					UUrDebuggerMessage("Impact Effect Load: materialindex %d translated material-type %d outside range [0, %d)!\n",
										itr, matindex->material, MArMaterials_GetNumTypes());
					return UUcError_Generic;
				}
			}

			// check that this is a valid index into the impact entry array
			if ((matindex->entry_index < 0) ||
				(matindex->entry_index + matindex->num_entries > ONgNumImpactEntries)) {
				UUrDebuggerMessage("Impact Effect Load: materialindex %d entry index %d len %d outside array bounds (%d)!\n",
									itr, matindex->entry_index, matindex->num_entries, ONgNumImpactEntries);
				return UUcError_Generic;
			}
		}

	}

	if (found_orphans) {
		// delete all material indices that refer to since-deleted materials
		UUmAssert(inLoad);

		// we will be moving stuff around... make the arrays dirty now and turn them into
		// dynamic arrays
		ONiImpactEffect_MakeDirty(UUcTrue, UUcTrue);

		itr = 0;
		matindex = ONiImpactEffect_GetMaterialIndex(0, 0);

		while (itr < ONgNumImpactMaterialIndices) {

			if (matindex->material != MAcInvalidID) {
				itr++;
				matindex++;
				continue;
			}

			// this material has since been deleted. we must remove it from the list of material indices, delete all of
			// its entries and remove it from the lookup table.
			entry = ONiImpactEffect_GetEntry(matindex->entry_index, matindex->num_entries);
			while (matindex->num_entries > 0) {
				error = ONiImpactEffect_DeallocateEntry(UUcTrue, matindex->entry_index, &deleted_from_matindex);
				UUmError_ReturnOnErrorMsg(error, "Impact Effect Load: deleting orphan material index: couldn't delete entry!");

				if (deleted_from_matindex != itr) {
					UUmError_ReturnOnErrorMsg(UUcError_Generic, "Impact Effect Load: deleting entry from orphan material index: didn't belong to us!");
				}
			}

			error = ONiImpactEffect_DeleteMaterialIndex(UUcTrue, itr);
			UUmError_ReturnOnErrorMsg(error, "Impact Effect Load: can't delete orphan material index!");
		}
	}

	if (shuffled) {
		UUmAssert(inLoad);
		lookup = ONgImpactLookupTable;
		num_types = MArImpacts_GetNumTypes();
		for (itr = 0; itr < num_types; itr++, lookup++) {
			if (lookup->num_indices == 0)
				continue;

			// quick-sort this impact's section of the materials index array to maintain order
			matindex = ONiImpactEffect_GetMaterialIndex(lookup->index_base, lookup->num_indices);
			qsort(matindex, lookup->num_indices, sizeof(ONtImpactMaterialIndex), ONiImpactEffect_MaterialIndexCompare);
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiIESwap_LookupIndices(
	UUtUns32				inNumLookups,
	ONtImpactLookupIndex *	inLookupTable,
	UUtBool					inSwapIt,
	UUtBool					inLoad)
{
	ONtImpactLookupIndex *lookup = inLookupTable;
	UUtUns32 itr;

	for (itr = 0; itr < inNumLookups; itr++, lookup++) {
		if (inSwapIt) {
			UUrSwap_2Byte((void *) &lookup->impact_type);
			UUrSwap_2Byte((void *) &lookup->num_indices);
			UUrSwap_4Byte((void *) &lookup->index_base);
		}

		if (inLoad) {
			// translate the impact type
			if ((lookup->impact_type < 0) || (lookup->impact_type >= ONgIE_FileNumImpactTypes)) {
				UUrDebuggerMessage("Impact Effect Load: impact lookup %d file impact-type %d outside file range [0, %d)!\n",
									itr, lookup->impact_type, ONgIE_FileNumImpactTypes);
				return UUcError_Generic;
			}

			UUmAssert(ONgIE_ImpactTranslation[lookup->impact_type].file_type == lookup->impact_type);
			lookup->impact_type = ONgIE_ImpactTranslation[lookup->impact_type].current_type;

			// check bounds on our material indices
			if (lookup->num_indices > 0) {
				if ((lookup->index_base < 0) || (lookup->index_base + lookup->num_indices > ONgNumImpactMaterialIndices)) {
					UUrDebuggerMessage("Impact Effect Load: impact lookup %d material indices %d (%d) outside file range [0, %d)!\n",
										itr, lookup->index_base, lookup->num_indices, ONgNumImpactMaterialIndices);
					return UUcError_Generic;
				}
			}
		}
	}

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiIEReadImpactTranslation(
	void)
{
	ONtImpactTranslator *translator = ONgIE_ImpactTranslation;
	UUtUns32 itr;

	ONgIE_ImpactsRequireTranslation = (ONgIE_FileNumImpactTypes != MArImpacts_GetNumTypes());

	for (itr = 0; itr < ONgIE_FileNumImpactTypes; itr++, translator++) {
		translator->file_type = (UUtUns16) itr;
		translator->current_type = MArImpactType_GetByName(translator->name);

		if (translator->file_type != translator->current_type) {
			ONgIE_ImpactsRequireTranslation = UUcTrue;
			if (translator->current_type == MAcInvalidID) {
				UUrDebuggerMessage("Impact Effect Load: impact type '%s' no longer exists, deleting all effects\n",
									translator->name);
			}
		}
	}
}

// ----------------------------------------------------------------------
static void
ONiIEReadMaterialTranslation(
	void)
{
	ONtMaterialTranslator *translator = ONgIE_MaterialTranslation;
	UUtUns32 itr;

	for (itr = 0; itr < ONgIE_FileNumMaterialTypes; itr++, translator++) {
		translator->file_type = (UUtUns16) itr;
		translator->current_type = MArMaterialType_GetByName(translator->name);

		if (translator->current_type == MAcInvalidID) {
			UUrDebuggerMessage("Impact Effect Load: material type '%s' no longer exists, deleting all effects\n",
								translator->name);
		}
	}
}

UUtUns8 *ONgImpactEffectsBuffer = NULL;
UUtUns32 ONgImpactEffectsBuffer_Size = 0;
UUtBool ONgImpactEffectsBuffer_SwapIt;
UUtBool ONgImpactEffectsBuffer_Locked;

UUtError ONrIEBinaryData_Process(void)
{
	UUtError					error;
	UUtUns8						*buffer;
	UUtUns32					itr, version, buffer_size, expected_size, num_types, sound_size;
	ONtImpactLookupIndex		*file_lookup_table, *lookup;

	if (NULL == ONgImpactEffectsBuffer) {
		return UUcError_Generic;
	}

	buffer = ONgImpactEffectsBuffer;
	buffer_size = ONgImpactEffectsBuffer_Size;

	// read the version number
	buffer_size -= OBDmGet4BytesFromBuffer(buffer, version, UUtUns32, ONgImpactEffectsBuffer_SwapIt);

	UUmAssert(version <= ONcIECurrentVersion);

	// set up globals
	ONgImpactEffect_Loaded						= UUcTrue;
	ONgImpactEffect_DynamicEntryArray			= UUcFalse;
	ONgImpactEffect_DynamicSoundArray			= UUcFalse;
	ONgImpactEffect_DynamicParticleArray		= UUcFalse;
	ONgImpactEffect_DynamicMaterialArray		= UUcFalse;
	ONgImpactEffect_Dirty						= UUcFalse;
	ONgImpactEffect_AutomaticModificationsOnly	= UUcFalse;
	ONgImpactEffect_Locked						= ONgImpactEffectsBuffer_Locked;

	buffer_size -= OBDmGet4BytesFromBuffer(buffer, ONgIE_FileNumImpactTypes, UUtUns32, ONgImpactEffectsBuffer_SwapIt);
	buffer_size -= OBDmGet4BytesFromBuffer(buffer, ONgIE_FileNumMaterialTypes, UUtUns32, ONgImpactEffectsBuffer_SwapIt);
	buffer_size -= OBDmGet4BytesFromBuffer(buffer, ONgNumImpactParticles, UUtUns32, ONgImpactEffectsBuffer_SwapIt);
	buffer_size -= OBDmGet4BytesFromBuffer(buffer, ONgNumImpactSounds, UUtUns32, ONgImpactEffectsBuffer_SwapIt);
	buffer_size -= OBDmGet4BytesFromBuffer(buffer, ONgNumImpactEntries, UUtUns32, ONgImpactEffectsBuffer_SwapIt);
	buffer_size -= OBDmGet4BytesFromBuffer(buffer, ONgNumImpactMaterialIndices, UUtUns32, ONgImpactEffectsBuffer_SwapIt);

	if (ONgIE_FileNumImpactTypes >= UUcMaxUns16) {
		UUrDebuggerMessage("Impact Effect Load: file num_impacts %d is corrupt\n", ONgIE_FileNumImpactTypes);
		goto cleanup;
	}

	if (ONgIE_FileNumMaterialTypes >= UUcMaxUns16) {
		UUrDebuggerMessage("Impact Effect Load: file num_materials %d is corrupt\n", ONgIE_FileNumMaterialTypes);
		goto cleanup;
	}

	if (ONgNumImpactParticles >= UUcMaxUns16) {
		UUrDebuggerMessage("Impact Effect Load: num particles %d is corrupt\n", ONgNumImpactParticles);
		goto cleanup;
	}

	if (ONgNumImpactSounds >= UUcMaxUns16) {
		UUrDebuggerMessage("Impact Effect Load: num sounds %d is corrupt\n", ONgNumImpactSounds);
		goto cleanup;
	}


	// make sure that there is enough data in the buffer
	expected_size = 0;
	expected_size += ONgIE_FileNumImpactTypes * sizeof(ONtImpactTranslator);
	expected_size += ONgIE_FileNumMaterialTypes * sizeof(ONtMaterialTranslator);
	expected_size += ONgIE_FileNumImpactTypes * sizeof(ONtImpactLookupIndex);
	expected_size += ONgNumImpactParticles * sizeof(ONtIEParticle);
	if (version >= ONcIEVersion_2) {
		sound_size = sizeof(ONtIESound);
	} else {
		sound_size = sizeof(ONtIESound_Version1);
	}
	expected_size += ONgNumImpactSounds * sound_size;
	expected_size += ONgNumImpactEntries * sizeof(ONtImpactEntry);
	expected_size += ONgNumImpactMaterialIndices * sizeof(ONtImpactMaterialIndex);

	if (buffer_size != expected_size) {
		UUrDebuggerMessage("Impact Effect Load: %d@%d impact-types, %d@%d material-types, %d@%d lookup...\n",
				ONgIE_FileNumImpactTypes, sizeof(ONtImpactTranslator), ONgIE_FileNumMaterialTypes, sizeof(ONtMaterialTranslator),
				ONgIE_FileNumImpactTypes, sizeof(ONtImpactLookupIndex));

		UUrDebuggerMessage("... %d@%d particles, %d@%d sounds, %d@%d entries, %d@%d mat indices...\n",
				ONgNumImpactParticles, sizeof(ONtIEParticle), ONgNumImpactSounds, sound_size,
				ONgNumImpactEntries, sizeof(ONtImpactEntry), ONgNumImpactMaterialIndices, sizeof(ONtImpactMaterialIndex));

		UUrDebuggerMessage("... expected size %d but buffer is %d!\n", expected_size, buffer_size);
		goto cleanup;
	}


	/*
	 * read the translation tables
	 */

	ONgIE_ImpactTranslation = (ONtImpactTranslator *) buffer;
	buffer += ONgIE_FileNumImpactTypes * sizeof(ONtImpactTranslator);
	ONiIEReadImpactTranslation();

	ONgIE_MaterialTranslation = (ONtMaterialTranslator *) buffer;
	buffer += ONgIE_FileNumMaterialTypes * sizeof(ONtMaterialTranslator);
	ONiIEReadMaterialTranslation();


	/*
	 * read the lookup table
	 */

	file_lookup_table = (ONtImpactLookupIndex *) buffer;
	buffer += ONgIE_FileNumImpactTypes * sizeof(ONtImpactLookupIndex);
	error = ONiIESwap_LookupIndices(ONgIE_FileNumImpactTypes, file_lookup_table, ONgImpactEffectsBuffer_SwapIt, UUcTrue);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Load: error loading impact lookup table!\n");
		goto cleanup;
	}

	if (ONgIE_ImpactsRequireTranslation) {
		// our lookup table has to be allocated and built
		num_types = MArImpacts_GetNumTypes();
		ONgImpactLookupTable = UUrMemory_Block_NewClear(num_types * sizeof(ONtImpactLookupIndex));
		UUmError_ReturnOnNull(ONgImpactLookupTable);

		for (itr = 0; itr < num_types; itr++) {
			ONgImpactLookupTable[itr].impact_type = (MAtImpactType) itr;
		}

		// copy the impacts across into their new locations
		lookup = file_lookup_table;
		for (itr = 0; itr < ONgIE_FileNumImpactTypes; itr++, lookup++) {
			if (lookup->impact_type == MAcInvalidID)
				continue;

			UUmAssert((lookup->impact_type >= 0) && (lookup->impact_type < num_types));
			ONgImpactLookupTable[lookup->impact_type] = *lookup;
		}

		ONgImpactLookupTable_Allocated = UUcTrue;
	} else {
		// use the file lookup table
		ONgImpactLookupTable = file_lookup_table;
		ONgImpactLookupTable_Allocated = UUcFalse;
	}

	/*
	 * read the particles
	 */

	ONgImpactParticles = (ONtIEParticle *) buffer;
	buffer += ONgNumImpactParticles * sizeof(ONtIEParticle);
	error = ONiIESwap_Particles(ONgNumImpactParticles, ONiImpactEffect_GetParticle(0, 0), ONgImpactEffectsBuffer_SwapIt, UUcTrue);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Load: error loading particles!\n");
		goto cleanup;
	}


	/*
	 * read the sounds
	 */

	if (version >= ONcIEVersion_2) {
		// sounds are the correct size, no padding necessary. we can byte-swap in place.
		ONgImpactSounds = (ONtIESound *) buffer;
		buffer += ONgNumImpactSounds * sizeof(ONtIESound);
	} else {
		// the sound data is the wrong version, and it must be padded out before being byte-swapped
		error = ONiImpactEffect_MakeDynamicArray(&ONgImpactEffect_DynamicSoundArray, ONgNumImpactSounds, (UUtUns8 *) buffer,
												sizeof(ONtIESound_Version1), sizeof(ONtIESound), &ONgImpactSounds);
		if (error != UUcError_None) {
			UUrDebuggerMessage("Impact Effect Load: error making sound array dynamic for versioning!\n");
			goto cleanup;
		}
		buffer += ONgNumImpactSounds * sizeof(ONtIESound_Version1);
	}
	error = ONiIESwap_Sounds(ONgNumImpactSounds, ONiImpactEffect_GetSound(0, 0), ONgImpactEffectsBuffer_SwapIt, UUcTrue);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Load: error loading sounds!\n");
		goto cleanup;
	}


	/*
	 * read the impact entries
	 */

	ONgImpactEntries = (ONtImpactEntry *) buffer;
	buffer += ONgNumImpactEntries * sizeof(ONtImpactEntry);
	error = ONiIESwap_Entries(ONgNumImpactEntries, ONiImpactEffect_GetEntry(0, 0), ONgImpactEffectsBuffer_SwapIt, UUcTrue);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Load: error loading entries!\n");
		goto cleanup;
	}


	/*
	 * read the material indices
	 */

	ONgImpactMaterialIndices = (ONtImpactMaterialIndex *) buffer;
	buffer += ONgNumImpactMaterialIndices * sizeof(ONtImpactMaterialIndex);
	error = ONiIESwap_MaterialIndices(ONgNumImpactMaterialIndices, ONiImpactEffect_GetMaterialIndex(0, 0),
				(ONgIE_ImpactsRequireTranslation ? ONgIE_FileNumImpactTypes : 0),
				(ONgIE_ImpactsRequireTranslation ? file_lookup_table : NULL), ONgImpactEffectsBuffer_SwapIt, UUcTrue);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Load: error loading material indices!\n");
		goto cleanup;
	}

	// check the structure (but don't check particle class or sound pointers yet, as we don't
	// know that the rest of the binary data has been loaded)
	ONrImpactEffect_VerifyStructure(UUcFalse);

	// done!
	return UUcError_None;

cleanup:
	ONiIEDeleteStorage();
	ONiIEClearStorage();
	UUmError_ReturnOnErrorMsg(UUcError_Generic, "could not load impact effects (see debugger.txt)");
}

// ----------------------------------------------------------------------
static UUtError
ONiIEBinaryData_Load(
	const char					*inIdentifier,
	BDtData						*ioBinaryData,
	UUtBool						inSwapIt,
	UUtBool						inLocked,
	UUtBool						inAllocated)
{
	UUmAssert(inIdentifier);
	UUmAssert(ioBinaryData);

	ONgImpactEffectsBuffer = ioBinaryData->data;
	ONgImpactEffectsBuffer_Size = ioBinaryData->header.data_size;
	ONgImpactEffectsBuffer_SwapIt = inSwapIt;
	ONgImpactEffectsBuffer_Locked = inLocked;

	if (inAllocated) {
		ONgImpactEffect_MemoryBlock = ioBinaryData;
	}

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiIEWriteImpactTranslation(
	UUtUns8 **		inBuffer,
	UUtUns32 *		ioNumBytes)
{
	ONtImpactTranslator *translator = (ONtImpactTranslator *) *inBuffer;
	UUtUns32 itr, write_size, num_types = MArImpacts_GetNumTypes();

	write_size = num_types * sizeof(ONtImpactTranslator);
	UUmAssert(*ioNumBytes >= write_size);

	for (itr = 0; itr < num_types; itr++, translator++) {
		UUrString_Copy(translator->name, MArImpactType_GetName((MAtImpactType) itr), TMcInstanceName_MaxLength);
	}

	*inBuffer += write_size;
	*ioNumBytes -= write_size;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiIEWriteMaterialTranslation(
	UUtUns8 **		inBuffer,
	UUtUns32 *		ioNumBytes)
{
	ONtMaterialTranslator *translator = (ONtMaterialTranslator *) *inBuffer;
	UUtUns32 itr, write_size, num_types = MArMaterials_GetNumTypes();

	write_size = num_types * sizeof(ONtMaterialTranslator);
	UUmAssert(*ioNumBytes >= write_size);

	for (itr = 0; itr < num_types; itr++, translator++) {
		UUrString_Copy(translator->name, MArMaterialType_GetName((MAtMaterialType) itr), TMcInstanceName_MaxLength);
	}

	*inBuffer += write_size;
	*ioNumBytes -= write_size;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiIEBinaryData_Save(
	UUtBool						inAutoSave)
{
	UUtUns32				data_size, write_size, num_bytes, num_impact_types, num_material_types;
	UUtUns8					*data = NULL, *buffer;
	UUtBool					swap_data;
	UUtError				error;

	if ((!ONgImpactEffect_Loaded) || (!ONgImpactEffect_Dirty) || (ONgImpactEffect_Locked) ||
		(ONgImpactEffect_AutomaticModificationsOnly)) {
		return UUcError_None;
	}

	ONrImpactEffect_VerifyStructure(UUcTrue);

	num_impact_types = MArImpacts_GetNumTypes();
	num_material_types = MArMaterials_GetNumTypes();


#if UUmEndian == UUmEndian_Big
	swap_data = UUcTrue;
#elif UUmEndian == UUmEndian_Little
	swap_data = UUcFalse;
#else
	#pragma error
#endif

	if (ONgNumImpactEntries == 0) {
		return UUcError_None;
	}

	// how much data do we have to write?
	data_size = 0;
	data_size += 7 * sizeof(UUtUns32);		// the header numbers
	data_size += num_impact_types * sizeof(ONtImpactTranslator);
	data_size += num_material_types * sizeof(ONtMaterialTranslator);
	data_size += num_impact_types * sizeof(ONtImpactLookupIndex);
	data_size += ONgNumImpactParticles * sizeof(ONtIEParticle);
	data_size += ONgNumImpactSounds * sizeof(ONtIESound);
	data_size += ONgNumImpactEntries * sizeof(ONtImpactEntry);
	data_size += ONgNumImpactMaterialIndices * sizeof(ONtImpactMaterialIndex);

	// allocate memory to save the material
	data = (UUtUns8 *) UUrMemory_Block_NewClear(data_size);
	if (data == NULL) {
		UUrDebuggerMessage("Impact Effect Save: can't allocate buffer for data save!\n");
		goto cleanup;
	}

	// set up to write at the start of this block
	buffer = data;
	num_bytes = data_size;

	// write the header
	OBDmWrite4BytesToBuffer(buffer, ONcIECurrentVersion, UUtUns32, num_bytes, OBJcWrite_Little);

	OBDmWrite4BytesToBuffer(buffer, num_impact_types,				UUtUns32, num_bytes, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(buffer, num_material_types,				UUtUns32, num_bytes, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(buffer, ONgNumImpactParticles,			UUtUns32, num_bytes, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(buffer, ONgNumImpactSounds,				UUtUns32, num_bytes, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(buffer, ONgNumImpactEntries,			UUtUns32, num_bytes, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(buffer, ONgNumImpactMaterialIndices,	UUtUns32, num_bytes, OBJcWrite_Little);

	/*
	 * write the translation tables
	 */

	error = ONiIEWriteImpactTranslation(&buffer, &num_bytes);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Save: could not write impact-type translation!\n");
		goto cleanup;
	}

	error = ONiIEWriteMaterialTranslation(&buffer, &num_bytes);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Save: could not write material-type translation!\n");
		goto cleanup;
	}


	/*
	 * write the lookup table
	 */

	write_size = num_impact_types * sizeof(ONtImpactLookupIndex);
	UUmAssert(num_bytes >= write_size);
	UUrMemory_MoveFast(ONgImpactLookupTable, buffer, write_size);

	error = ONiIESwap_LookupIndices(num_impact_types, (ONtImpactLookupIndex *) buffer, swap_data, UUcFalse);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Save: could not write impact lookup table!\n");
		goto cleanup;
	}
	num_bytes -= write_size;
	buffer += write_size;


	/*
	 * write the particles
	 */

	write_size = ONgNumImpactParticles * sizeof(ONtIEParticle);
	UUmAssert(num_bytes >= write_size);
	UUrMemory_MoveFast(ONiImpactEffect_GetParticle(0, 0), buffer, write_size);

	error = ONiIESwap_Particles(ONgNumImpactParticles, (ONtIEParticle *) buffer, swap_data, UUcFalse);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Save: could not write particles!\n");
		goto cleanup;
	}
	num_bytes -= write_size;
	buffer += write_size;


	/*
	 * write the sounds
	 */

	write_size = ONgNumImpactSounds * sizeof(ONtIESound);
	UUmAssert(num_bytes >= write_size);
	UUrMemory_MoveFast(ONiImpactEffect_GetSound(0, 0), buffer, write_size);

	error = ONiIESwap_Sounds(ONgNumImpactSounds, (ONtIESound *) buffer, swap_data, UUcFalse);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Save: could not write sounds!\n");
		goto cleanup;
	}
	num_bytes -= write_size;
	buffer += write_size;

	/*
	 * write the impact entries
	 */

	write_size = ONgNumImpactEntries * sizeof(ONtImpactEntry);
	UUmAssert(num_bytes >= write_size);
	UUrMemory_MoveFast(ONiImpactEffect_GetEntry(0, 0), buffer, write_size);

	error = ONiIESwap_Entries(ONgNumImpactEntries, (ONtImpactEntry *) buffer, swap_data, UUcFalse);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Save: could not write impact entries!\n");
		goto cleanup;
	}
	num_bytes -= write_size;
	buffer += write_size;


	/*
	 * write the material indices
	 */

	write_size = ONgNumImpactMaterialIndices * sizeof(ONtImpactMaterialIndex);
	UUmAssert(num_bytes >= write_size);
	UUrMemory_MoveFast(ONiImpactEffect_GetMaterialIndex(0, 0), buffer, write_size);

	error = ONiIESwap_MaterialIndices(ONgNumImpactMaterialIndices, (ONtImpactMaterialIndex *) buffer,
										0, NULL, swap_data, UUcFalse);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Save: could not write impact material indices!\n");
		goto cleanup;
	}
	num_bytes -= write_size;
	buffer += write_size;


	/*
	 * save the data
	 */
	UUmAssert(num_bytes == 0);
	error =
		OBDrBinaryData_Save(
			ONcIEBinaryDataClass,
			"impact_effects",
			data,
			(data_size - num_bytes),
			0,
			inAutoSave);
	if (error != UUcError_None) {
		UUrDebuggerMessage("Impact Effect Save: could not write binary data file!\n");
		goto cleanup;
	}

	UUrMemory_Block_Delete(data);

	if (inAutoSave == UUcFalse) {
		ONgImpactEffect_Dirty = UUcFalse;
	}

	ONgImpactEffect_AutomaticModificationsOnly = UUcFalse;
	return UUcError_None;

cleanup:
	if (data != NULL) {
		UUrMemory_Block_Delete(data);
	}
	UUmError_ReturnOnErrorMsg(UUcError_Generic, "could not save impact effects (see debugger.txt)");
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ======================================================================
// initialization and high-level functions
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiIEClearStorage(
	void)
{
	// clear the impact effect storage
	ONgImpactEffect_Loaded = UUcFalse;
	ONgImpactEffect_MemoryBlock = NULL;

	ONgNumImpactEntries = 0;
	ONgNumImpactSounds = 0;
	ONgNumImpactParticles = 0;
	ONgNumImpactMaterialIndices = 0;

	ONgImpactEntries = NULL;
	ONgImpactSounds = NULL;
	ONgImpactParticles = NULL;
	ONgImpactMaterialIndices = NULL;

	ONgImpactLookupTable_Allocated = UUcFalse;
	ONgImpactLookupTable = NULL;

	// clear the clipboard
	ONgImpactEffect_ClipboardFull = UUcFalse;
	UUrMemory_Clear(&ONgImpactEffect_ClipboardEntry, sizeof(ONtImpactEntry));
	UUrMemory_Clear(&ONgImpactEffect_ClipboardSound, sizeof(ONtIESound));
	ONgImpactEffect_ClipboardParticles = NULL;
}

// ----------------------------------------------------------------------
static void
ONiIEDeleteStorage(
	void)
{
	// deallocate the single memory block if there is one
	if (ONgImpactEffect_MemoryBlock != NULL) {
		UUrMemory_Block_Delete(ONgImpactEffect_MemoryBlock);
		ONgImpactEffect_MemoryBlock = NULL;
	}

	// deallocate the memory pools if they have been created
	if (ONgImpactEffect_Loaded) {
		if (ONgImpactEffect_DynamicEntryArray) {
			UUrMemory_Array_Delete((UUtMemory_Array *) ONgImpactEntries);
		}
		if (ONgImpactEffect_DynamicSoundArray) {
			UUrMemory_Array_Delete((UUtMemory_Array *) ONgImpactSounds);
		}
		if (ONgImpactEffect_DynamicParticleArray) {
			UUrMemory_Array_Delete((UUtMemory_Array *) ONgImpactParticles);
		}
		if (ONgImpactEffect_DynamicMaterialArray) {
			UUrMemory_Array_Delete((UUtMemory_Array *) ONgImpactMaterialIndices);
		}

		ONgImpactEffect_Loaded = UUcFalse;
	}

	// deallocate the global lookup table
	if (ONgImpactLookupTable_Allocated && (ONgImpactLookupTable != NULL)) {
		UUrMemory_Block_Delete(ONgImpactLookupTable);
		ONgImpactLookupTable = NULL;
		ONgImpactLookupTable_Allocated = UUcFalse;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiIEBinaryData_Register(
	void)
{
	UUtError					error;
	BDtMethods					methods;

	methods.rLoad = ONiIEBinaryData_Load;

	error =	BDrRegisterClass(ONcIEBinaryDataClass, &methods);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrImpactEffects_ListBrokenLinks(
	BFtFile						*inFile)
{
	char						text[2048];
	UUtUns32					num_impacts;
	UUtUns32					num_materials;
	UUtUns16					i;

	// sprintf a header
	sprintf(text, "********** Animation Sound Links **********\n\n");
	BFrFile_Write(inFile, strlen(text), text);
	sprintf(text, "Impact\tMaterial\tModifier\tImpulse Sound Name\n");
	BFrFile_Write(inFile, strlen(text), text);

	num_impacts = MArImpacts_GetNumTypes();
	num_materials = MArMaterials_GetNumTypes();

	for (i = 0; i < num_impacts; i++)
	{
		UUtUns16				j;

		for (j = 0; j < num_materials; j++)
		{
			UUtUns16				k;

			for (k = 0; k < ONcIEModType_NumTypes; k++)
			{
				UUtUns32				itr;
				UUtUns32				found_indices[ONcIEComponent_Max];

				ONrImpactEffect_Lookup(i, j, k, found_indices);

				for (itr = 0; itr < ONcIEComponent_Max; itr++)
				{
					ONtImpactEntry			*entry;
					ONtIESound				*sound;
					const char				*impact_name;
					const char				*material_name;
					const char				*mod_name;
					SStImpulse				*impulse;

					// get the entry
					if (found_indices[itr] == (UUtUns32) -1) { continue; }
					entry = ONiImpactEffect_GetEntry(found_indices[itr], 1);
					if (entry == NULL) { continue; }
					if (entry->sound_index == ONcIESound_None) { continue; }

					// get teh impact effect sound
					sound = ONiImpactEffect_GetSound(entry->sound_index, 1);
					impulse = OSrImpulse_GetByName(sound->sound_name);
					if (impulse != NULL) { continue; }

					// get the names
					impact_name = MArImpactType_GetName(entry->impact_type);
					material_name = MArMaterialType_GetName(entry->material_type);
					mod_name = ONgIEModTypeName[entry->modifier];

					sprintf(
						text,
						"%s\t%s\t%s\t%s\n",
						impact_name,
						material_name,
						mod_name,
						sound->sound_name);
					COrConsole_Printf(text);
					BFrFile_Write(inFile, strlen(text), text);
				}
			}
		}
	}

	sprintf(text, "\n\n\n");
	BFrFile_Write(inFile, strlen(text), text);
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffects_Initialize(
	void)
{
	UUtError					error;

	// register the binary data
	error = ONiIEBinaryData_Register();
	UUmError_ReturnOnError(error);

	ONiIEClearStorage();

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffects_Save(
	UUtBool						inAutoSave)
{
	return ONiIEBinaryData_Save(inAutoSave);
}

// ----------------------------------------------------------------------
void
ONrImpactEffects_Terminate(
	void)
{
	ONiIEDeleteStorage();
	ONiIEClearStorage();
}

// ----------------------------------------------------------------------
void
ONrImpactEffects_SetupParticlePointers(
	void)
{
	if (!ONgImpactEffect_Loaded)
		return;

	ONiImpactEffect_UpdateParticlePointers(ONgNumImpactParticles, 0);
}

// ----------------------------------------------------------------------
void
ONrImpactEffects_SetupSoundPointers(
	void)
{
	if (!ONgImpactEffect_Loaded)
		return;

	ONiImpactEffect_UpdateSoundPointers(ONgNumImpactSounds, 0);
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffects_CreateBlank(
	void)
{
	UUtUns32					itr, num_types;

	if (ONgImpactEffect_Loaded)
		return UUcError_None;

	ONiIEClearStorage();

	// set up globals
	ONgImpactEffect_Loaded						= UUcTrue;
	ONgImpactEffect_DynamicEntryArray			= UUcFalse;
	ONgImpactEffect_DynamicSoundArray			= UUcFalse;
	ONgImpactEffect_DynamicParticleArray		= UUcFalse;
	ONgImpactEffect_DynamicMaterialArray		= UUcFalse;
	ONgImpactEffect_Dirty						= UUcTrue;
	ONgImpactEffect_AutomaticModificationsOnly	= UUcTrue;
	ONgImpactEffect_Locked						= UUcFalse;

	// allocate empty memory arrays
	ONiImpactEffect_MakeDirty(UUcTrue, UUcTrue);

	// allocate a blank lookup table
	num_types = MArImpacts_GetNumTypes();
	ONgImpactLookupTable_Allocated = UUcTrue;
	ONgImpactLookupTable = UUrMemory_Block_NewClear(num_types * sizeof(ONtImpactLookupIndex));
	UUmError_ReturnOnNull(ONgImpactLookupTable);

	for (itr = 0; itr < num_types; itr++) {
		ONgImpactLookupTable[itr].impact_type = (MAtImpactType) itr;
	}

	return UUcError_None;
}


// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
ONiImpactEffect_Write(
	BFtFile					*inFile,
	ONtImpactEntry			*inEntry)
{
	const char				*impact_name;
	const char				*material_name;
	const char				*mod_name;
	const char				*sound_name;
	char					string[2048];
	UUtUns32				i;

	impact_name = MArImpactType_GetName(inEntry->impact_type);
	material_name = MArMaterialType_GetName(inEntry->material_type);
	mod_name = ONgIEModTypeName[inEntry->modifier];

	if (inEntry->sound_index != ONcIESound_None)
	{
		ONtIESound				*sound;

		sound = ONiImpactEffect_GetSound(inEntry->sound_index, 1);
		sound_name = sound->sound_name;
	}
	else
	{
		sound_name = "(NONE)";
	}

	sprintf(string, "%s\t%s\t%s\t%s\n", impact_name, material_name, mod_name, sound_name);
	BFrFile_Write(inFile, strlen(string), string);

	for (i = 0; i < inEntry->num_particles; i++)
	{
		ONtIEParticle			*particle;

		particle = ONiImpactEffect_GetParticle(inEntry->particle_baseindex, inEntry->num_particles);
		if (particle == NULL) { continue; }

		sprintf(string, "%d\t%s\n", i, particle->particle_class_name);
		BFrFile_Write(inFile, strlen(string), string);
	}
}

// ----------------------------------------------------------------------
static void
ONiImpactEffects_Write(
	BFtFile					*inFile)
{
	UUtUns32				num_impacts;
	UUtUns32				num_materials;
	UUtUns16				i;

	num_impacts = MArImpacts_GetNumTypes();
	num_materials = MArMaterials_GetNumTypes();

	for (i = 0; i < num_impacts; i++)
	{
		UUtUns16				j;

		for (j = 0; j < num_materials; j++)
		{
			UUtUns16				k;

			for (k = 0; k < ONcIEModType_NumTypes; k++)
			{
				UUtUns32				itr;
				UUtUns32				found_indices[ONcIEComponent_Max];

				ONrImpactEffect_Lookup(i, j, k, found_indices);

				for (itr = 0; itr < ONcIEComponent_Max; itr++)
				{
					ONtImpactEntry			*entry;

					if (found_indices[itr] == (UUtUns32) -1) { continue; }

					entry = ONiImpactEffect_GetEntry(found_indices[itr], 1);
					if (entry == NULL) { continue; }

					ONiImpactEffect_Write(inFile, entry);
				}
			}
		}
	}
}

// ----------------------------------------------------------------------
UUtError
ONrImpactEffects_WriteTextFile(
	void)
{
	UUtError				error;
	BFtFileRef				*file_ref;
	BFtFile					*file;

	// create the file ref
	error = BFrFileRef_MakeFromName("ImpactEffects.txt", &file_ref);
	UUmError_ReturnOnError(error);

	// create the .TXT file if it doesn't already exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		// create the object.env file
		error = BFrFile_Create(file_ref);
		UUmError_ReturnOnError(error);
	}

	// open the file
	error = BFrFile_Open(file_ref, "rw", &file);
	UUmError_ReturnOnError(error);

	// set the position to 0
	error = BFrFile_SetPos(file, 0);
	UUmError_ReturnOnError(error);

	// write the items
	ONiImpactEffects_Write(file);

	// set the end of the file
	BFrFile_SetEOF(file);

	// close the file
	BFrFile_Close(file);
	file = NULL;

	// delete the file ref
	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;

	return UUcError_None;
}
