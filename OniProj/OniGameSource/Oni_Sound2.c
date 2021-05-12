// ======================================================================
// Oni_Sound2.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_AppUtilities.h"
#include "BFW_BinaryData.h"
#include "BFW_FileManager.h"
#include "BFW_ScriptLang.h"
#include "BFW_SoundSystem2.h"
#include "BFW_WindowManager.h"
#include "BFW_Particle3.h"
#include "BFW_Timer.h"

#include "Oni_GameState.h"
#include "Oni_Persistance.h"
#include "Oni_Object.h"
#include "Oni_Sound2.h"
#include "Oni_Sound_Private.h"
#include "Oni_Sound_Animation.h"
#include "Oni_ImpactEffect.h"

// ======================================================================
// defines
// ======================================================================
#define OScMaxBufferSize		1024

#define OScAmbientDataClass		UUm4CharToUns32('O', 'S', 'A', 'm')
#define OScGroupDataClass		UUm4CharToUns32('O', 'S', 'G', 'r')
#define OScImpulseDataClass		UUm4CharToUns32('O', 'S', 'I', 'm')

#define OScNumBuckets			(256)

#define OScUpdateDist			(2.0f)

#define OScOccludeAmount		(0.5f)

#define OScMaxPlayingAmbientHandles		(1024)
#define OScSubtitleStayTime		60

#define OScMaxScriptAmbients	16

#define OScMaxFlybyProjectiles	24

#define OScOcclusion_CollisionInterval			12

#define OScOcclusion_CoplanarDistance			0.1f
#define OScOcclusion_SkipFurnitureSourceDist	2.0f
#define OScOcclusion_SkipFurnitureContinueDist	8.0f
#define OScOcclusion_SkipFurnitureBackDist		3.0f

#define OScMaxCollisions		3

#define OScVolumeAdjust			(0.05f)

// ======================================================================
// enums
// ======================================================================
enum
{
	OScPOState_Off				= 0x00,
	OScPOState_Playing			= 0x01,
	OScPOState_Starting			= 0x02,
	OScPOState_Stopping			= 0x04
};

enum
{
	OScFlybyProjectileFlag_InUse	= (1 << 0)
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct OStHashElement
{
	const char					*name;
	union
	{
		SStAmbient				*ambient;
		SStGroup				*group;
		SStImpulse				*impulse;
	} u;
	
} OStHashElement;

// ----------------------------------------------------------------------
typedef struct OStOcclusionCache
{
	UUtUns32					update_offset;				// a number from 0..OScOcclusion_CollisionInterval-1
	UUtUns32					last_update;
	float						occlusion_volume;

	AKtOctNodeIndexCache		octree_cache;

} OStOcclusionCache;

// ----------------------------------------------------------------------
typedef struct OStPlayingAmbient
{
	UUtUns32					index;
	SStPlayID					ambient_id;
	const SStAmbient			*ambient;
	SStPlayID					play_id;
	
	UUtBool						has_position;
	M3tPoint3D					position;
	float						max_volume_distance;
	float						min_volume_distance;
	
	float						volume;
	OStOcclusionCache			occlusion_cache;

} OStPlayingAmbient;

// ----------------------------------------------------------------------
typedef struct OStPlayingObject
{
	const OBJtObject			*object;
	SStPlayID					play_id;
	
	float						volume;
	float						transition_volume;
	float						object_volume;
	
	UUtUns32					state;		// for sound volumes
	UUtUns32					start_time;

	OStOcclusionCache			occlusion_cache;
	
} OStPlayingObject;

// ----------------------------------------------------------------------
typedef struct OStScriptAmbient
{
	SStPlayID					play_id;
	float						volume;
	char						name[SScMaxNameLength];

} OStScriptAmbient;

// ----------------------------------------------------------------------
typedef struct OStFlybyProjectile
{
	UUtUns32					flags;

	M3tPoint3D					prev_position;
	M3tPoint3D					position;

	UUtUns32					particle_ref;
	P3tParticleClass *			particle_class;
	P3tParticle *				particle;

} OStFlybyProjectile;

// ======================================================================
// globals
// ======================================================================
#if TOOL_VERSION
static BFtFileRef				*OSgBinaryDir;
#endif

static AUtHashTable				*OSgAmbientTable;
static AUtHashTable				*OSgGroupTable;
static AUtHashTable				*OSgImpulseTable;

static UUtMemory_Array			*OSgPlayingAmbient;
static OStPlayingAmbient		**OSgPlayingAmbientHandles;

static UUtMemory_Array			*OSgPlayingObjects;
static OStScriptAmbient			OSgScriptAmbients[OScMaxScriptAmbients];
static UUtBool					OSgDialog_Playing;
static SStPlayID				OSgDialog_PlayID;
static UUtUns32					OSgDialog_EndTime;

static M3tPoint3D				OSgListener_Position;
static M3tPoint3D				OSgListener_Facing;
static AKtOctNodeIndexCache		OSgListener_OctreeCache;

static BFtFileCache				*OSgBinFiles_Load_Cache = NULL;

static UUtBool					OSgEnabled;
static float					OSgPrevVolume;

static UUtBool					OSgScriptOnly;

static SStPlayID				OSgMusic_PlayID;

TMtPrivateData					*OSgBinaryData_PrivateData = NULL;

#if TOOL_VERSION
// debugging occlusion display
UUtBool							OSgShowOcclusions = UUcFalse;
static UUtUns32					OSgNumOcclusionLines;
static M3tPoint3D				OSgOcclusionLines[64][2];
static UUtUns32					OSgNumOcclusionPoints;
static M3tPoint3D				OSgOcclusionPoints[128];
static IMtShade					OSgOcclusionShades[128];
#endif

// AI damaging projectile knowledge base
static UUtUns32					OSgNumFlybyProjectiles;
static OStFlybyProjectile		OSgFlybyProjectile[OScMaxFlybyProjectiles];

// ======================================================================
// prototypes
// ======================================================================
static OStPlayingAmbient*
OSiPlayingAmbient_GetByID(
	UUtUns32				inAmbientID);

static void
OSrUpdateImpulsePointers(
	SStImpulse *inImpulse,
	UUtBool inDeleted);

static void
OSrUpdateAmbientPointers(
	SStAmbient *inAmbient,
	UUtBool inDeleted);

static UUtUns32
OSiAmbient_GetSimpleDuration(
	SStAmbient *inAmbient);

static void
OSiFlybyProjectile_Clear(
	void);

static void
OSiFlybyProjectiles_Update(
	void);

static void
OSiFlybyProjectile_DeleteByIndex(
	UUtUns8					inIndex);

static void
OSiNeutralInteractions_ListBrokenSounds(
	BFtFile						*inFile);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
OSrDebugDisplay(
	void)
{
	UUtUns32				i;
	
	if (SSgShowDebugInfo) {
		// go through all of the ambient sources that are active
		// and draw them
		for (i = 0; i < OScMaxPlayingAmbientHandles; i++)
		{
			OStPlayingAmbient		*playing_ambient;
			
			playing_ambient = OSiPlayingAmbient_GetByID(i);
			if (playing_ambient == NULL) { continue; }
			
			M3rGeom_Draw_DebugSphere(
				&playing_ambient->position,
				playing_ambient->max_volume_distance * UUmFeet(1), 
				IMcShade_Red);

			M3rGeom_Draw_DebugSphere(
				&playing_ambient->position,
				playing_ambient->min_volume_distance * UUmFeet(1), 
				IMcShade_Green);
		}
	}

#if TOOL_VERSION
	if (OSgShowOcclusions) {
		IMtShade				shade;
		M3tPoint3D				p0, p1;
		
		// draw occlusion lines and points
		for (i = 0; i < OSgNumOcclusionLines; i++) {
			M3rGeom_Line_Light(&OSgOcclusionLines[i][0], &OSgOcclusionLines[i][1], IMcShade_White);
		}

		for (i = 0; i < OSgNumOcclusionPoints; i++) {
			shade = OSgOcclusionShades[i];

			p0 = p1 = OSgOcclusionPoints[i];
			p0.x -= 2.0f;
			p1.x += 2.0f;
			M3rGeom_Line_Light(&p0, &p1, shade);

			p0 = p1 = OSgOcclusionPoints[i];
			p0.y -= 2.0f;
			p1.y += 2.0f;
			M3rGeom_Line_Light(&p0, &p1, shade);

			p0 = p1 = OSgOcclusionPoints[i];
			p0.z -= 2.0f;
			p1.z += 2.0f;
			M3rGeom_Line_Light(&p0, &p1, shade);
		}
	}
#endif
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
#if TOOL_VERSION
// ----------------------------------------------------------------------
static UUtBool
OSiAddDebugOcclusionPoint(
	const M3tPoint3D		*inPoint,
	IMtShade				inShade)
{
	if (OSgShowOcclusions && (OSgNumOcclusionPoints < 128)) {
		OSgOcclusionPoints[OSgNumOcclusionPoints] = *inPoint;
		OSgOcclusionShades[OSgNumOcclusionPoints] = inShade;
		OSgNumOcclusionPoints++;
		return UUcTrue;
	}

	return UUcFalse;
}
#endif

// ----------------------------------------------------------------------
static UUtUns16
OSiIsOccluded(
	const M3tPoint3D		*inListener,
	const M3tPoint3D		*inSource,
	AKtOctNodeIndexCache	*ioSourceCache)
{
	M3tVector3D				sightline;
	AKtEnvironment			*env = ONrGameState_GetEnvironment();
	AKtCollision			*collision;
	UUtBool					had_collision, possibly_inside_furniture, last_point_is_source;
	UUtBool					last_point_reversenormal, close_enough, reversed_normal, lastpoint_was_collision;
	M3tPoint3D				last_point, last_collision_point;
	M3tPlaneEquation		collision_plane;
	UUtUns16				num_collisions, itr;
	UUtUns32				inside_object_tag;
	float					dot, dist_tolerance;
	
	if (env == NULL) { return OScMaxCollisions; }
	
	MUmVector_Subtract(sightline, *inListener, *inSource);
	
#if TOOL_VERSION
	if ((OSgShowOcclusions) && (OSgNumOcclusionLines < 64)) {
		OSgOcclusionLines[OSgNumOcclusionLines][0] = *inSource;
		OSgOcclusionLines[OSgNumOcclusionLines][1] = *inListener;
		OSgNumOcclusionLines++;
	}
#endif
	
	had_collision = AKrCollision_Point_SpatialCache(env, inSource, &sightline, AKcGQ_Flag_SoundOcclude_Skip,
													AKcMaxNumCollisions, ioSourceCache, &OSgListener_OctreeCache);
	if (!had_collision) {
		return 0;
	}
	
	// process the collision list
	inside_object_tag = (UUtUns32) -1;
	num_collisions = 0;
	possibly_inside_furniture = UUcTrue;
	last_point_is_source = UUcTrue;
	lastpoint_was_collision = UUcFalse;
	last_point = *inSource;
	for(itr = 0, collision = AKgCollisionList; itr < AKgNumCollisions; itr++, collision++) {
		AKtGQ_General			*general_gq;
		AKtGQ_Collision			*collision_gq;
		
		if (collision->collisionType != CLcCollision_Face) {
			continue;
		}
		
		general_gq = env->gqGeneralArray->gqGeneral + collision->gqIndex;
		
		if (num_collisions == 0) {
			if (MUmVector_GetDistanceSquared(*inSource, collision->collisionPoint) < UUmSQR(OScOcclusion_CoplanarDistance)) {
				// ignore this collision so that we don't get occluded when we create a sound that is exactly
				// coplanar with a quad
#if TOOL_VERSION
				OSiAddDebugOcclusionPoint(&collision->collisionPoint, IMcShade_Purple);
#endif
				continue;
			}
		}

		if (((general_gq->flags & AKcGQ_Flag_Furniture) == 0) || (!possibly_inside_furniture)) {
			// non-furniture collisions are always valid
			num_collisions++;
			possibly_inside_furniture = UUcFalse;
#if TOOL_VERSION
			OSiAddDebugOcclusionPoint(&collision->collisionPoint, IMcShade_Red);
#endif
			continue;
		}

		if (inside_object_tag != (UUtUns32) -1) {
			if (general_gq->object_tag == inside_object_tag) {
				// we ignore this object from now on
#if TOOL_VERSION
				OSiAddDebugOcclusionPoint(&collision->collisionPoint, IMcShade_Pink);
#endif

			} else {
				// this is a different object to the one that we are inside, its
				// collsions are always valid
#if TOOL_VERSION
				OSiAddDebugOcclusionPoint(&collision->collisionPoint, IMcShade_Yellow);
#endif
				num_collisions++;
			}
			continue;
		}

		close_enough = UUcTrue;
		if (last_point_is_source) {
			// we are checking from the listener's point
			dist_tolerance = OScOcclusion_SkipFurnitureSourceDist;

		} else if (last_point_reversenormal) {
			// we just had a reversed-normal occlusion
			dist_tolerance = OScOcclusion_SkipFurnitureContinueDist;

		} else {
			// we just had a valid occlusion that was skipped, don't
			// skip again
			close_enough = UUcFalse;
		}
		close_enough = close_enough && (MUmVector_GetDistanceSquared(last_point, collision->collisionPoint) < UUmSQR(dist_tolerance));

		if (general_gq->flags & AKcGQ_Flag_DrawBothSides) {
			// we are never 'inside' two-sided quads
			reversed_normal = UUcFalse;
		} else {
			// we are occluding from 'inside' a furniture object if the collision plane
			// lies along our intersection vector
			collision_gq = env->gqCollisionArray->gqCollision + collision->gqIndex;
			AKmPlaneEqu_GetComponents(collision_gq->planeEquIndex, env->planeArray->planes,
										collision_plane.a, collision_plane.b, collision_plane.c, collision_plane.d);
			dot = sightline.x * collision_plane.a + sightline.y * collision_plane.b + sightline.z * collision_plane.c;
			reversed_normal = (dot > 0);
		}

		if (lastpoint_was_collision && reversed_normal) {
			if (MUmVector_GetDistanceSquared(collision->collisionPoint, last_collision_point) < UUmSQR(OScOcclusion_SkipFurnitureBackDist)) {
				// our LAST collision should also be rejected as it's very close to the one we just found,
				// which had a reversed normal
				UUmAssert(num_collisions > 0);
				num_collisions--;
#if TOOL_VERSION
				if (OSgShowOcclusions) {
					UUmAssert(OSgNumOcclusionPoints > 0);
					OSgOcclusionShades[OSgNumOcclusionPoints - 1] = IMcShade_LightBlue;
					OSgNumOcclusionPoints--;
				}
#endif
			}
		}

		// store the collision point
		last_point_reversenormal = reversed_normal;
		last_point_is_source = UUcFalse;
		lastpoint_was_collision = UUcFalse;
		last_point = collision->collisionPoint;

#if TOOL_VERSION
		if (reversed_normal) {
			OSiAddDebugOcclusionPoint(&collision->collisionPoint, IMcShade_Blue);
		} else if (close_enough) {
			OSiAddDebugOcclusionPoint(&collision->collisionPoint, IMcShade_Green);
		}
#endif

		if (reversed_normal || close_enough) {
			// reject this collision
			if (general_gq->object_tag != (UUtUns32) -1) {
				inside_object_tag = general_gq->object_tag;
			}
			continue;
		}

		// add this collusion to the occlusion list
#if TOOL_VERSION
		OSiAddDebugOcclusionPoint(&collision->collisionPoint, IMcShade_Orange);
#endif
		num_collisions++;
		lastpoint_was_collision = UUcTrue;
		last_collision_point = last_point;
	}
	
	return num_collisions;
}

// ----------------------------------------------------------------------
static void
OSiSetupOcclusionCache(
	UUtUns32				inSalt,
	OStOcclusionCache		*outOcclusionCache)
{
	// set up the occlusion cache with its starting values
	outOcclusionCache->update_offset = (inSalt % OScOcclusion_CollisionInterval);
	outOcclusionCache->last_update = 0;
	outOcclusionCache->occlusion_volume = 1.0f;

	AKrCollision_InitializeSpatialCache(&outOcclusionCache->octree_cache);
}

// ----------------------------------------------------------------------
static UUtBool
OSiCalc_PositionAndVolume(
	const M3tPoint3D		*inPosition,
	float					inMaxVolumeDistance,
	float					inMinVolumeDistance,
	float					inMinOcclusion,
	M3tPoint3D				*outPosition,
	float					*outVolume,
	UUtBool					inUseOcclusionCache,
	OStOcclusionCache		*ioOcclusionCache)
{
	M3tPoint3D				position;
	float					direct_dist;
	UUtUns32				current_time = ONrGameState_GetGameTime();
	
	// NULL position indicates non-spatialized audio,
	// but must be handled outside this function!
	UUmAssert(inPosition != NULL);
	position = *outPosition = *inPosition;
	*outVolume = 0.0f;
	
	// determine if the sound is within the minimum volume distance
	direct_dist = MUrPoint_Distance_Squared(&position, &OSgListener_Position) * UUmSQR(SScFootToDist);
	if (direct_dist > UUmSQR(inMinVolumeDistance)) { return UUcFalse; }

	if (inMinOcclusion > 0.99f)
	{
		*outVolume= 1.0f;
	}
	else
	{
		UUtBool	use_cached_volume = UUcFalse;

		if (inUseOcclusionCache && (ioOcclusionCache != NULL)) {
			UUtUns32 desired_update_time;

			// work out what tick we want to update at next
			desired_update_time = ioOcclusionCache->last_update - (ioOcclusionCache->last_update % OScOcclusion_CollisionInterval);
			desired_update_time += OScOcclusion_CollisionInterval + ioOcclusionCache->update_offset;

			if (current_time < desired_update_time) {
				// we don't check volume by casting a ray, we continue to use the existing one
				use_cached_volume = UUcTrue;
				*outVolume = ioOcclusionCache->occlusion_volume;
//				COrConsole_Printf("%d: occlusion cached %f (next %d)", current_time, ioOcclusionCache->occlusion_volume, desired_update_time);
			}
		}

		if (!use_cached_volume) {
			// calculate occlusion by casting a ray
			UUtUns16 num_collisions = OSiIsOccluded(&OSgListener_Position, &position,
													(ioOcclusionCache == NULL) ? NULL : &ioOcclusionCache->octree_cache);

			if (num_collisions > 0)
			{
				static const float one_twelvth= 0.0833333333f;

				num_collisions = UUmMin(OScMaxCollisions, num_collisions);
				// set the volume
				*outVolume = UUmMax(inMinOcclusion, 0.8f - ((float)(num_collisions * num_collisions) * one_twelvth/*(1.0f / 12.0f)*/));
			}
			else
			{
				// no occlusion
				*outVolume = 1.0f;
			}

			if (ioOcclusionCache != NULL) {
				ioOcclusionCache->last_update = current_time;
			}
//			COrConsole_Printf("%d: occlusion calc %f", current_time, *outVolume);
		}
	}
	
	if (ioOcclusionCache != NULL) {
		ioOcclusionCache->occlusion_volume = *outVolume;
	}

	return UUcTrue;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static OStHashElement*
OSiHasElement_GetByName(
	AUtHashTable			*inHashTable,
	const char				*inName)
{
	OStHashElement			find_me;
	OStHashElement			*hash_elem;
	
	if (inHashTable == NULL) { return NULL; }
	if (inName == NULL) { return NULL; }
	
	find_me.name = inName;
	find_me.u.ambient = NULL;
	
	hash_elem = (OStHashElement*)AUrHashTable_Find(inHashTable, &find_me);
	
	return hash_elem;
}

// ----------------------------------------------------------------------
static UUtUns32
OSiHashElement_GetHashValue(
	void					*inElement)
{
	OStHashElement			*hash_elem;
	UUtUns32				hash_value;
	
	hash_elem = (OStHashElement*)inElement;
	hash_value = AUrString_GetHashValue(hash_elem->name);
	
	return hash_value;
}

// ----------------------------------------------------------------------
static UUtBool
OSiHashElement_IsEqual(
	void					*inElement1,
	void					*inElement2)
{
	OStHashElement			*hash_elem1;
	OStHashElement			*hash_elem2;
	UUtBool					result;
	
	hash_elem1 = (OStHashElement*)inElement1;
	hash_elem2 = (OStHashElement*)inElement2;
	
	result = UUrString_Compare_NoCase(hash_elem1->name, hash_elem2->name) == 0;
	
	return result;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OSiPlayingAmbientHandles_Update(
	void)
{
	OStPlayingAmbient		*playing_ambient_array;
	UUtUns32				num_elements;
	UUtUns32				i;
	
	UUrMemory_Clear(OSgPlayingAmbientHandles, (sizeof(OStPlayingAmbient*) * OScMaxPlayingAmbientHandles));
	
	playing_ambient_array = (OStPlayingAmbient*)UUrMemory_Array_GetMemory(OSgPlayingAmbient);
	num_elements = UUrMemory_Array_GetUsedElems(OSgPlayingAmbient);
	for (i = 0; i < num_elements; i++)
	{
		UUmAssert(OSgPlayingAmbientHandles[playing_ambient_array[i].ambient_id] == NULL);	// somehow two ambients have the same index

		playing_ambient_array[i].index = i;
		OSgPlayingAmbientHandles[playing_ambient_array[i].ambient_id] = &playing_ambient_array[i];
	}
}

// ----------------------------------------------------------------------
static void
OSiPlayingAmbient_FreeElement(
	OStPlayingAmbient		*inPlayingAmbient)
{
	UUmAssert(inPlayingAmbient != NULL);
	UUmAssert(inPlayingAmbient->ambient_id < OScMaxPlayingAmbientHandles);
	UUmAssert(inPlayingAmbient->play_id == SScInvalidID);
	
	UUrMemory_Array_DeleteElement(OSgPlayingAmbient, inPlayingAmbient->index);
	OSiPlayingAmbientHandles_Update();
}

// ----------------------------------------------------------------------
static UUtError
OSiPlayingAmbient_GetNewElement(
	OStPlayingAmbient		**outPlayingAmbient,
	UUtUns32				*outIndex)
{
	UUtError				error;
	OStPlayingAmbient		*playing_ambient_array;
	UUtUns32				index;
	UUtBool					mem_moved;
	UUtUns32				i;
	
	*outPlayingAmbient = NULL;
	
	if (UUrMemory_Array_GetUsedElems(OSgPlayingAmbient) >= OScMaxPlayingAmbientHandles)
	{
		return UUcError_Generic;
	}
	
	// get a new element from the array
	error = UUrMemory_Array_GetNewElement(OSgPlayingAmbient, &index, &mem_moved);
	UUmError_ReturnOnError(error);
	
	playing_ambient_array = (OStPlayingAmbient*)UUrMemory_Array_GetMemory(OSgPlayingAmbient);
	
	// set the index
	playing_ambient_array[index].index = index;
	
	// add a link to the playing ambient to the handle array
	for (i = 0; i < OScMaxPlayingAmbientHandles; i++)
	{
		if (OSgPlayingAmbientHandles[i] != NULL) { continue; }
	
		// set the ambient_id
		playing_ambient_array[index].ambient_id = i;
		
		if (mem_moved == UUcFalse)
		{
			OSgPlayingAmbientHandles[i] = &playing_ambient_array[index];
		}
		break;
	}
	
	if (mem_moved) { OSiPlayingAmbientHandles_Update(); }
		
	// set the outgoing playing ambient
	*outPlayingAmbient = &playing_ambient_array[index];
	*outIndex = index;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static OStPlayingAmbient*
OSiPlayingAmbient_GetByID(
	UUtUns32				inAmbientID)
{
	OStPlayingAmbient		*playing_ambient;
	
	UUmAssert(inAmbientID < OScMaxPlayingAmbientHandles);
	
	playing_ambient = OSgPlayingAmbientHandles[inAmbientID];
	
	return playing_ambient;
}

// ----------------------------------------------------------------------
static UUtError
OSiPlayingAmbient_Initialize(
	void)
{
	// allocate the playing ambient handles
	OSgPlayingAmbientHandles =
		(OStPlayingAmbient**)UUrMemory_Block_NewClear(sizeof(OStPlayingAmbient*) * OScMaxPlayingAmbientHandles);
	UUmError_ReturnOnNull(OSgPlayingAmbientHandles);
	
	// initialize the playing ambient array
	OSgPlayingAmbient =	UUrMemory_Array_New(sizeof(OStPlayingAmbient), 16, 0, 0);
	UUmError_ReturnOnNull(OSgPlayingAmbient);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OSiPlayingAmbient_Terminate(
	void)
{
	UUrMemory_Array_Delete(OSgPlayingAmbient);
	OSgPlayingAmbient = NULL;
	
	UUrMemory_Block_Delete(OSgPlayingAmbientHandles);
	OSgPlayingAmbientHandles = NULL;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OSrAmbient_BuildHashTable(
	void)
{
	UUtUns32				num_ambients;
	UUtUns32				i;
	
	// delete the old table
	if (OSgAmbientTable != NULL)
	{
		AUrHashTable_Delete(OSgAmbientTable);
		OSgAmbientTable = NULL;
	}
	
	// allocate a new table
	OSgAmbientTable =
		AUrHashTable_New(
			sizeof(OStHashElement),
			OScNumBuckets,
			OSiHashElement_GetHashValue,
			OSiHashElement_IsEqual);
	UUmError_ReturnOnNull(OSgAmbientTable);
	
	// add the ambient sounds to the table
	num_ambients = SSrAmbient_GetNumAmbientSounds();
	for (i = 0; i < num_ambients; i++)
	{
		SStAmbient				*ambient;
		OStHashElement			hash_elem;
		
		ambient = SSrAmbient_GetByIndex(i);
		if (ambient == NULL) { break; }
		
		hash_elem.name = ambient->ambient_name;
		hash_elem.u.ambient = ambient;
		
		AUrHashTable_Add(OSgAmbientTable, &hash_elem);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrAmbient_ChangeName(
	SStAmbient				*inAmbient,
	const char				*inName)
{
	char					name[SScMaxNameLength];
	
	UUmAssert(inAmbient);
	UUmAssert(inName);
	
	// save the old name
	UUrString_Copy(name, inAmbient->ambient_name, SScMaxNameLength);
	
	// change the ambients's name
	UUrString_Copy(inAmbient->ambient_name, inName, SScMaxNameLength);
	
	// update the name
	
	// rebuild the hash table
	OSrAmbient_BuildHashTable();
}

// ----------------------------------------------------------------------
UUtError
OSrAmbient_Delete(
	const char				*inName)
{
	UUtError				error;
	
	SSrAmbient_Delete(inName, UUcTrue);
	
	error = OSrAmbient_BuildHashTable();
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
// CodeWarrior computes the true part of the first if below, regardless of the inContext NULL-ness and then later picks the right result.

SStAmbient*
OSrAmbient_GetByName(
	const char				*inName)
{
	OStHashElement			*hash_elem;
	
	// find the hash element for the ambient sound named inName
	hash_elem = OSiHasElement_GetByName(OSgAmbientTable, inName);
	if (hash_elem == NULL) { return NULL; }
	
	// return the ambient pointer
	return hash_elem->u.ambient;
}


// ----------------------------------------------------------------------
void
OSrAmbient_Halt(
	SStPlayID					inAmbientID)
{
	OStPlayingAmbient			*playing_ambient;
	
	if (inAmbientID == SScInvalidID) { return; }
	
	// go through and find the ambient to halt
	playing_ambient = OSiPlayingAmbient_GetByID(inAmbientID);
	if (playing_ambient == NULL) { return; }
	
	if (playing_ambient->play_id != SScInvalidID)
	{
		SSrAmbient_Halt(playing_ambient->play_id);

/*		if (SSgShowDebugInfo)
		{
			COrConsole_Printf(
				"OSrAmbient_Halt %d %s",
				playing_ambient->play_id,
				playing_ambient->ambient->ambient_name);
		}*/
	
		playing_ambient->play_id = SScInvalidID;
	}
	
	OSiPlayingAmbient_FreeElement(playing_ambient);
}

// ----------------------------------------------------------------------
static UUtError
OSiAmbient_Load(
	const char				*inIdentifier,
	BDtData					*ioBinaryData,
	UUtBool					inSwapIt,
	UUtBool					inLocked,
	UUtBool					inAllocated)
{
	UUtError				error;
	SStAmbient				*ambient;
	
	UUtUns8					*buffer;
	UUtUns32				buffer_size;
	UUtUns32				version;
	
	char					name[SScMaxNameLength];
	
	UUmAssert(inIdentifier);
	UUmAssert(ioBinaryData);
		
	// ------------------------------
	// create a new ambient sound
	// ------------------------------
	OSrMakeGoodName(inIdentifier, name);
	
	// if no ambient sound with this name exists, create a new one,
	// if one does exist, take it over
	ambient = SSrAmbient_GetByName(name);
	if (ambient == NULL)
	{
		// create a new ambient sound
		error = SSrAmbient_New(name, &ambient);
		UUmError_ReturnOnError(error);
	}
	else
	{
		ambient->in_sound = NULL;
		ambient->out_sound = NULL;
		ambient->base_track1 = NULL;
		ambient->base_track2 = NULL;
		ambient->detail = NULL;
	}
	
	// ------------------------------
	// read the binary data
	// ------------------------------
	buffer = ioBinaryData->data;
	buffer_size = ioBinaryData->header.data_size;

	// read the version number
	BDmGet4BytesFromBuffer(buffer, version, UUtUns32, inSwapIt);
	
	BDmGet4BytesFromBuffer(buffer, ambient->priority, SStPriority2, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, ambient->flags, UUtUns32, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, ambient->sphere_radius, float, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, ambient->min_detail_time, float, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, ambient->max_detail_time, float, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, ambient->max_volume_distance, float, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, ambient->min_volume_distance, float, inSwapIt);
	BDmGetStringFromBuffer(buffer, ambient->detail_name, SScMaxNameLength, inSwapIt);
	BDmGetStringFromBuffer(buffer, ambient->base_track1_name, SScMaxNameLength, inSwapIt);
	BDmGetStringFromBuffer(buffer, ambient->base_track2_name, SScMaxNameLength, inSwapIt);
	BDmGetStringFromBuffer(buffer, ambient->in_sound_name, SScMaxNameLength, inSwapIt);
	BDmGetStringFromBuffer(buffer, ambient->out_sound_name, SScMaxNameLength, inSwapIt);
	
	if (version < OS2cVersion_5)
	{
		ambient->threshold = SScAmbientThreshold;
	}
	else
	{
		BDmGet4BytesFromBuffer(buffer, ambient->threshold, UUtUns32, inSwapIt);
	}
	
	if (version < OS2cVersion_6)
	{
		ambient->min_occlusion = 0.0f;
	}
	else
	{
		BDmGet4BytesFromBuffer(buffer, ambient->min_occlusion, float, inSwapIt);
	}
	
	if (inAllocated == UUcTrue)
	{
		UUrMemory_Block_Delete(ioBinaryData);
	}
		
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiAmbient_LevelLoad(
	void)
{
	UUtError				error;
	
	error = OSrAmbient_BuildHashTable();
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OSiAmbient_LevelUnload(
	void)
{
	// delete the hash table
	if (OSgAmbientTable != NULL)
	{
		AUrHashTable_Delete(OSgAmbientTable);
		OSgAmbientTable = NULL;
	}
}

// ----------------------------------------------------------------------
UUtError
OSrAmbient_New(
	const char				*inName,
	SStAmbient				**outAmbient)
{
	UUtError				error;
	char					name[SScMaxNameLength];
	
	OSrMakeGoodName(inName, name);
	
	error = SSrAmbient_New(name, outAmbient);
	UUmError_ReturnOnError(error);
	
	error = OSrAmbient_BuildHashTable();
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OSrAmbient_Save(
	SStAmbient				*inAmbient,
	BFtFileRef				*inParentDirRef)
{
	UUtError				error;
	BFtFileRef				*file_ref;
	BFtFile					*file;
	UUtUns8					block[OScMaxBufferSize + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8					*data = UUrAlignMemory(block);
	UUtUns8					*buffer;
	UUtUns32				bytes_avail;
	char					name[BFcMaxFileNameLength];

	// create the file ref
	sprintf(name, "%s.%s", inAmbient->ambient_name, OScAmbientSuffix);
	error = BFrFileRef_DuplicateAndAppendName(inParentDirRef, name, &file_ref);
	UUmError_ReturnOnError(error);
	
	// create the file if it doesn't exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		error = BFrFile_Create(file_ref);
		UUmError_ReturnOnErrorMsg(error, "Could not create the file.");
	}
	
	// see if the file is locked
	if (BFrFileRef_IsLocked(file_ref) == UUcTrue)
	{
		char					error_string[1024];
		
		sprintf(
			error_string,
			"Unable to save ambient sound, because the file %s is locked",
			name);
		
		WMrDialog_MessageBox(
			NULL,
			"Error",
			error_string,
			WMcMessageBoxStyle_OK);
			
		BFrFileRef_Dispose(file_ref);
		file_ref = NULL;
		
		return UUcError_None;
	}
	
	// open the file for writing
	error =
		BFrFile_Open(
			file_ref,
			"w",
			&file);
	UUmError_ReturnOnErrorMsg(error, "Could not open the group file.");
	
	// set the pos to the beginning of the file
	BFrFile_SetPos(file, 0);
	
	// setup the buffer pointer
	buffer = data;
	bytes_avail = OScMaxBufferSize;
	
	// write the version
	BDmWrite4BytesToBuffer(buffer, OS2cCurrentVersion, UUtUns32, bytes_avail, BDcWrite_Little);
	
	// write the ambient sound to the buffer
	BDmWrite4BytesToBuffer(buffer, inAmbient->priority, SStPriority2, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inAmbient->flags, UUtUns32, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inAmbient->sphere_radius, float, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inAmbient->min_detail_time, float, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inAmbient->max_detail_time, float, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inAmbient->max_volume_distance, float, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inAmbient->min_volume_distance, float, bytes_avail, BDcWrite_Little);
	BDmWriteStringToBuffer(buffer, inAmbient->detail_name, SScMaxNameLength, bytes_avail, BDcWrite_Little);
	BDmWriteStringToBuffer(buffer, inAmbient->base_track1_name, SScMaxNameLength, bytes_avail, BDcWrite_Little);
	BDmWriteStringToBuffer(buffer, inAmbient->base_track2_name, SScMaxNameLength, bytes_avail, BDcWrite_Little);
	BDmWriteStringToBuffer(buffer, inAmbient->in_sound_name, SScMaxNameLength, bytes_avail, BDcWrite_Little);
	BDmWriteStringToBuffer(buffer, inAmbient->out_sound_name, SScMaxNameLength, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inAmbient->threshold, UUtUns32, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inAmbient->min_occlusion, float, bytes_avail, BDcWrite_Little);
	
	// write the ambient sound
	error =
		BDrBinaryData_Write(
			OScAmbientDataClass,
			data,
			(OScMaxBufferSize - bytes_avail),
			file);
	UUmError_ReturnOnErrorMsg(error, "Unable to write to file.");
	
	// set the EOF
	BFrFile_SetEOF(file);
	
	// close the file
	BFrFile_Close(file);
	file = NULL;
	
	UUrMemory_Block_Delete(file_ref);
	file_ref = NULL;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
SStPlayID
OSrAmbient_Start(
	const SStAmbient			*inAmbient,
	const M3tPoint3D			*inPosition,		// inPosition == NULL will play non-spatial
	const M3tVector3D			*inDirection,		/* unused if inPosition == NULL */
	const M3tVector3D			*inVelocity,		/* unused if inPosition == NULL */
	const float					*inMaxVolDistance,	/* unused if inPosition == NULL */
	const float					*inMinVolDistance)	/* unused if inPosition == NULL */
{
	UUtError					error;
	UUtBool						can_play;
	UUtUns32					playing_ambient_index;
	M3tPoint3D					position;
	float						volume;
	float						max_volume_distance;
	float						min_volume_distance;
	
	OStPlayingAmbient			*playing_ambient;
	
	// set the min and max volume distances
	if (inMaxVolDistance == NULL)
	{
		max_volume_distance = inAmbient->max_volume_distance;
	}
	else
	{
		max_volume_distance = *inMaxVolDistance;
	}
	
	if (inMinVolDistance == NULL)
	{
		min_volume_distance = inAmbient->min_volume_distance;
	}
	else
	{
		min_volume_distance = *inMinVolDistance;
	}
	
	// get an entry in the playing ambient array
	error = OSiPlayingAmbient_GetNewElement(&playing_ambient, &playing_ambient_index);
	if (error != UUcError_None)
	{
/*		if (SSgShowDebugInfo)
		{
			COrConsole_Printf(
				"OSrAmbient_Start: ran out of playing_ambients for %s",
				inAmbient->ambient_name);
		}*/
		return SScInvalidID;
	}
	
/*	{
	float dist = MUrPoint_Distance(inPosition, &OSgListener_Position);
	
	COrConsole_Printf(
		"OSrAmbient_Start %d: %s %d at (%f, %f, %f) %f",
		ONrGameState_GetGameTime(),
		inAmbient->ambient_name,
		playing_ambient->ambient_id,
		inPosition->x,
		inPosition->y,
		inPosition->z,
		dist);
	}*/
	
	// initialize the playing ambient
	playing_ambient->play_id = SScInvalidID;
	playing_ambient->ambient = inAmbient;
	playing_ambient->max_volume_distance = max_volume_distance;
	playing_ambient->min_volume_distance = min_volume_distance;
	playing_ambient->has_position = UUcFalse;
	OSiSetupOcclusionCache(playing_ambient_index, &playing_ambient->occlusion_cache);
	
	// play the ambient sound if possible
	if (inPosition == NULL)
	{
		if (OSgScriptOnly == UUcFalse)
		{
			playing_ambient->play_id = SSrAmbient_Start_Simple(inAmbient, NULL);
			
/*			if (SSgShowDebugInfo)
			{
				COrConsole_Printf(
					"OSrAmbient_Start: playing %d %s",
					playing_ambient->play_id,
					playing_ambient->ambient->ambient_name);
			}*/
		}
	}
	else
	{
		playing_ambient->position = *inPosition;
		playing_ambient->has_position = UUcTrue;
		
		// calculate the position and volume of the sound
		can_play =
			OSiCalc_PositionAndVolume(
				inPosition,
				max_volume_distance,
				min_volume_distance,
				inAmbient->min_occlusion,
				&position,
				&volume,
				UUcFalse,
				&playing_ambient->occlusion_cache);
		if ((can_play) && (OSgScriptOnly == UUcFalse))
		{
			// start the ambient sound
			playing_ambient->play_id =
				SSrAmbient_Start(
					inAmbient,
					&position,
					inDirection,
					inVelocity,
					max_volume_distance,
					min_volume_distance,
					&volume);
			playing_ambient->volume = volume;
			
/*			if (SSgShowDebugInfo)
			{
				if (playing_ambient->play_id == SScInvalidID)
				{
					COrConsole_Printf(
						"OSrAmbient_Start: SSrAmbient_Start did not play %s",
						playing_ambient->ambient->ambient_name);
				}
				else
				{
					float direct_dist = MUrPoint_Distance(&position, &OSgListener_Position) * SScFootToDist;
					
					COrConsole_Printf(
						"SSrAmbient_Start: %d %s, %f <=> %f, (%f, %f, %f), (%f, %f, %f)",
						playing_ambient->play_id,
						playing_ambient->ambient->ambient_name,
						direct_dist,
						playing_ambient->ambient->min_volume_distance,
						position.x,
						position.y,
						position.z,
						OSgListener_Position.x,
						OSgListener_Position.y,
						OSgListener_Position.z);
				}
			}*/
		}
		else if ((inAmbient->flags & SScAmbientFlag_PlayOnce) != 0)
		{
/*			if (SSgShowDebugInfo)
			{
				COrConsole_Printf(
					"OSrAmbient_Start: could not play %s",
					playing_ambient->ambient->ambient_name);
			}*/
			OSiPlayingAmbient_FreeElement(playing_ambient);
			return SScInvalidID;
		}
/*		else if (SSgShowDebugInfo)
		{
			COrConsole_Printf(
				"OSrAmbient_Start: could not play once %s",
				playing_ambient->ambient->ambient_name);
		}*/
	}
	
	return playing_ambient->ambient_id;
}

// ----------------------------------------------------------------------
void
OSrAmbient_Stop(
	SStPlayID					inAmbientID)
{
	OStPlayingAmbient			*playing_ambient;
	
	if (inAmbientID == SScInvalidID) { return; }
	
	// go through and find the ambient to stop
	playing_ambient = OSiPlayingAmbient_GetByID(inAmbientID);
	if (playing_ambient == NULL) { return; }
	
	if (playing_ambient->play_id != SScInvalidID)
	{
		SSrAmbient_Stop(playing_ambient->play_id);

/*		if (SSgShowDebugInfo)
		{
			COrConsole_Printf(
				"OSrAmbient_Stop %d: %s %d",
				ONrGameState_GetGameTime(),
				playing_ambient->ambient->ambient_name,
				inAmbientID);
		}*/
	
		playing_ambient->play_id = SScInvalidID;
	}
	
	OSiPlayingAmbient_FreeElement(playing_ambient);
}

// ----------------------------------------------------------------------
static UUtBool
OSiAmbient_Update(
	SStPlayID					inAmbientID,
	OStPlayingAmbient			*inPlayingAmbient,
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inDirection,
	const M3tVector3D			*inVelocity)
{
	UUmAssert(inPlayingAmbient);
	
	if (inPosition != NULL)
	{
		UUtBool						is_playing;
		UUtBool						can_play;
		M3tPoint3D					position;
		float						volume;
		
		// update the playing ambient
		inPlayingAmbient->position = *inPosition;
		inPlayingAmbient->has_position = UUcTrue;
		
		can_play =
			OSiCalc_PositionAndVolume(
				&inPlayingAmbient->position,
				inPlayingAmbient->max_volume_distance,
				inPlayingAmbient->min_volume_distance,
				inPlayingAmbient->ambient->min_occlusion,
				&position,
				&volume,
				UUcTrue,
				&inPlayingAmbient->occlusion_cache);
//		COrConsole_Printf("ambient update - volume %f", volume);
		if (can_play == UUcFalse)
		{
			// the ambient sound is already being played, update it so
			// that the sound system code can deal with stopping it if
			// necessary
			if (inPlayingAmbient->play_id == SScInvalidID) { return UUcTrue; }
			
/*			if (SSgShowDebugInfo)
			{
				float direct_dist = MUrPoint_Distance(&position, &OSgListener_Position) * SScFootToDist;
				
				COrConsole_Printf(
					"SSrAmbient_Update: %d %s, %f <=> %f, (%f, %f, %f), (%f, %f, %f)",
					inPlayingAmbient->play_id,
					inPlayingAmbient->ambient->ambient_name,
					direct_dist,
					inPlayingAmbient->ambient->min_volume_distance,
					position.x,
					position.y,
					position.z,
					OSgListener_Position.x,
					OSgListener_Position.y,
					OSgListener_Position.z);
			}*/

			is_playing =
				SSrAmbient_Update(
					inPlayingAmbient->play_id,
					&position,
					inDirection,
					inVelocity,
					&volume);
			if (is_playing == UUcFalse)
			{
				inPlayingAmbient->play_id = SScInvalidID;
				if ((inPlayingAmbient->ambient->flags & SScAmbientFlag_PlayOnce) != 0)
				{
/*					if (SSgShowDebugInfo)
					{
						COrConsole_Printf(
							"OSiAmbient_Stop %d %s",
							inPlayingAmbient->play_id,
							inPlayingAmbient->ambient->ambient_name);
					}*/
					
					OSrAmbient_Stop(inAmbientID);
					return UUcFalse;
				}
			}
			// play once sounds need to finish playing so don't halt them
			else if ((inPlayingAmbient->ambient->flags & SScAmbientFlag_PlayOnce) == 0)
			{
/*				if (SSgShowDebugInfo)
				{
					COrConsole_Printf(
						"OSrAmbient_Halt %d %s",
						inPlayingAmbient->play_id,
						inPlayingAmbient->ambient->ambient_name);
				}*/
			
				SSrAmbient_Halt(inPlayingAmbient->play_id);
				inPlayingAmbient->play_id = SScInvalidID;
			}
			
			return UUcTrue;
		}
		
		if (inPlayingAmbient->play_id == SScInvalidID)
		{
			if (OSgScriptOnly == UUcFalse)
			{
				// the ambient sound is not currently playing, so start it
				inPlayingAmbient->play_id =
					SSrAmbient_Start(
						inPlayingAmbient->ambient,
						&position,
						inDirection,
						inVelocity,
						inPlayingAmbient->max_volume_distance,
						inPlayingAmbient->min_volume_distance,
						&volume);

/*				if (SSgShowDebugInfo)
				{
					float direct_dist = MUrPoint_Distance(&position, &OSgListener_Position) * SScFootToDist;

					COrConsole_Printf(
						"OSrAmbient_Start %d %s",
						inPlayingAmbient->play_id,
						inPlayingAmbient->ambient->ambient_name);
				}*/
			}
		}
		else
		{
			float			volume_delta;
			
//			COrConsole_Printf("volume changing, %f -> %f", inPlayingAmbient->volume, volume);

			// update occluding volumes over time
			volume_delta = inPlayingAmbient->volume - volume;
			if (volume_delta > 0.0f)
			{
				volume =
					UUmPin(
						(inPlayingAmbient->volume - UUmMin(OScVolumeAdjust, volume_delta)),
						0.0f,
						1.0f);
			}
			else if (volume_delta < 0.0f)
			{
				volume =
					UUmPin(
						(inPlayingAmbient->volume - UUmMax(-OScVolumeAdjust, volume_delta)),
						0.0f,
						1.0f);
			}
						
			// update the ambient sound
//			COrConsole_Printf("volume makechange %f", inPlayingAmbient->volume, volume);
			is_playing =
				SSrAmbient_Update(
					inPlayingAmbient->play_id,
					&position,
					inDirection,
					inVelocity,
					&volume);
			if (is_playing == UUcFalse)
			{
				inPlayingAmbient->play_id = SScInvalidID;
				if ((inPlayingAmbient->ambient->flags & SScAmbientFlag_PlayOnce) != 0)
				{
//					COrConsole_Printf("OSiAmbient_Update %d: OSrAmbient_Stop because it finished.");
					OSrAmbient_Stop(inAmbientID);
					return UUcFalse;
				}
			}
		}
		
		inPlayingAmbient->volume = volume;
	}
		
	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
OSrAmbient_Update(
	SStPlayID					inAmbientID,
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inDirection,
	const M3tVector3D			*inVelocity)
{
	OStPlayingAmbient			*playing_ambient;
	UUtBool						result;
	
	if (inAmbientID == SScInvalidID) { return UUcFalse; }
	
	// get the a pointer to the playing ambient
	playing_ambient = OSiPlayingAmbient_GetByID(inAmbientID);
	if (playing_ambient == NULL) { return UUcFalse; }
	
/*	{
	float dist = MUrPoint_Distance(inPosition, &OSgListener_Position);
	
	COrConsole_Printf(
		"OSrAmbient_Update %d: %s %d at (%f, %f, %f) %f",
		ONrGameState_GetGameTime(),
		playing_ambient->ambient->ambient_name,
		inAmbientID,
		inPosition->x,
		inPosition->y,
		inPosition->z,
		dist);
	}*/
	
	result = OSiAmbient_Update(inAmbientID, playing_ambient, inPosition, inDirection, inVelocity);
	
	return result;
}

// ----------------------------------------------------------------------
static void
OSiAmbient_UpdateAll(
	void)
{
	UUtUns32					i;
	
	for (i = 0; i < OScMaxPlayingAmbientHandles; i++)
	{
		OStPlayingAmbient			*playing_ambient;
		
		playing_ambient = OSiPlayingAmbient_GetByID(i);
		if (playing_ambient == NULL) { continue; }
		if (playing_ambient->has_position == UUcFalse) { continue; }
		
//		COrConsole_Printf("update playingambient %s", playing_ambient->ambient->ambient_name);
		OSiAmbient_Update(i, playing_ambient, &playing_ambient->position, NULL, NULL);
	}
}

// ----------------------------------------------------------------------
static UUtUns32
OSiAmbient_GetSimpleDuration(
	SStAmbient *inAmbient)
{
	SStGroup *base_group;
	SStPermutation *perm_array, *permutation;
	UUtUns32 num_permutations, itr;

	if (inAmbient == NULL) {
		return 0;
	}

	base_group = inAmbient->base_track1;
	if (base_group == NULL) {
		return 0;
	}

	num_permutations = UUrMemory_Array_GetUsedElems(base_group->permutations);
	if (num_permutations == 0) {
		return 0;
	}

	perm_array = (SStPermutation *) UUrMemory_Array_GetMemory(base_group->permutations);
	for (itr = 0, permutation = perm_array; itr < num_permutations; itr++, permutation++) {
		if (permutation->sound_data == NULL) {
			continue;
		}

		return SSrSoundData_GetDuration(permutation->sound_data);
	}
	
	// found no non-NULL permutations
	return 0;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OSrGroup_BuildHashTable(
	void)
{
	UUtUns32				num_groups;
	UUtUns32				i;
	
	// delete the old table
	if (OSgGroupTable != NULL)
	{
		AUrHashTable_Delete(OSgGroupTable);
		OSgGroupTable = NULL;
	}
	
	// allocate a new table
	OSgGroupTable =
		AUrHashTable_New(
			sizeof(OStHashElement),
			OScNumBuckets,
			OSiHashElement_GetHashValue,
			OSiHashElement_IsEqual);
	UUmError_ReturnOnNull(OSgGroupTable);
	
	// add the sound groups to the table
	num_groups = SSrGroup_GetNumSoundGroups();
	for (i = 0; i < num_groups; i++)
	{
		SStGroup				*group;
		OStHashElement			hash_elem;
		
		group = SSrGroup_GetByIndex(i);
		if (group == NULL) { break; }
		
		hash_elem.name = group->group_name;
		hash_elem.u.group = group;
		
		AUrHashTable_Add(OSgGroupTable, &hash_elem);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrGroup_ChangeName(
	SStGroup				*inGroup,
	const char				*inName)
{
	char					name[SScMaxNameLength];
	
	UUmAssert(inGroup);
	UUmAssert(inName);
	
	// save the old name
	UUrString_Copy(name, inGroup->group_name, SScMaxNameLength);
	
	// change the group's name
	UUrString_Copy(inGroup->group_name, inName, SScMaxNameLength);
	
	// update the ambient and impulse sounds
	SSrAmbient_UpdateGroupName(name, inName);
	SSrImpulse_UpdateGroupName(name, inName);
	
	// rebuild the hash table
	OSrGroup_BuildHashTable();
}

// ----------------------------------------------------------------------
UUtError
OSrGroup_Delete(
	const char				*inName)
{
	UUtError				error;
	
	// delete the group
	SSrGroup_Delete(inName, UUcTrue);
	
	error = OSrGroup_BuildHashTable();
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
SStGroup*
OSrGroup_GetByName(
	const char				*inName)
{
	OStHashElement			*hash_elem;
	
	// find the hash element for the sound group named inName
	hash_elem = OSiHasElement_GetByName(OSgGroupTable, inName);
	if (hash_elem == NULL) { return NULL; }
	
	// return the group pointer
	return hash_elem->u.group;
}

// ----------------------------------------------------------------------
static UUtError
OSiGroup_Load(
	const char				*inIdentifier,
	BDtData					*ioBinaryData,
	UUtBool					inSwapIt,
	UUtBool					inLocked,
	UUtBool					inAllocated)
{
	UUtError				error;
	SStGroup				*group;
	
	UUtUns8					*buffer;
	UUtUns32				buffer_size;
	UUtUns32				version;
	
	UUtUns32				i;
	UUtUns32				num_permutations;
	UUtMemory_Array			*permutations;
	SStPermutation			*perm_array;
	
	char					name[SScMaxNameLength];
	
	UUmAssert(inIdentifier);
	UUmAssert(ioBinaryData);
	
	// ------------------------------
	// create a new group sound
	// ------------------------------
	OSrMakeGoodName(inIdentifier, name);
	
	// if no group with this name exists, create a new one,
	// if one does exist, take it over
	group = SSrGroup_GetByName(name);
	if (group == NULL)
	{
		// create a new sound group
		error = SSrGroup_New(name, &group);
		UUmError_ReturnOnError(error);
	}
	else
	{
		UUrMemory_Array_Delete(group->permutations);
		group->permutations = NULL;
	}
	
	// ------------------------------
	// read the binary data
	// ------------------------------
	buffer = ioBinaryData->data;
	buffer_size = ioBinaryData->header.data_size;

	// read the version number
	BDmGet4BytesFromBuffer(buffer, version, UUtUns32, inSwapIt);
	
	// read the group volume in file versions 2 or higher, before then, use
	// a default of 1.0f
	if (version < OS2cVersion_2)
	{
		group->group_volume = 1.0f;
	}
	else
	{
		BDmGet4BytesFromBuffer(buffer, group->group_volume, float, inSwapIt);
	}
	
	// read the group pitch in file versions 3 or higher, before then, use
	// a default of 1.0f
	if (version < OS2cVersion_3)
	{
		group->group_pitch = 1.0f;
	}
	else
	{
		BDmGet4BytesFromBuffer(buffer, group->group_pitch, float, inSwapIt);
	}
	
	// read the group flags from files version 6 or higher
	if (version < OS2cVersion_6)
	{
		group->flags = SScGroupFlag_None;
		group->flag_data = 0;
	}
	else
	{
		BDmGet2BytesFromBuffer(buffer, group->flags, UUtUns16, inSwapIt);
		BDmGet2BytesFromBuffer(buffer, group->flag_data, UUtUns16, inSwapIt);
	}
	
	BDmGet4BytesFromBuffer(buffer, group->num_channels, UUtUns32, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, num_permutations, UUtUns32, inSwapIt);
	
	// for older groups, default to SScGroupFlag_PreventRepeats if
	// num_permutations >= SScPreventRepeats_Default
	if ((version < OS2cVersion_6) && (num_permutations >= SScPreventRepeats_Default))
	{
		group->flags |= SScGroupFlag_PreventRepeats;
	}
	
	// create a memory array to hold the permutations
	permutations =
		UUrMemory_Array_New(
			sizeof(SStPermutation),
			1,
			num_permutations,
			num_permutations);
	UUmError_ReturnOnNull(permutations);
	
	if (group->permutations)
	{
		UUrMemory_Array_Delete(group->permutations);
	}
	group->permutations = permutations;
	
	// read the permutations
	perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(permutations);
	for (i = 0; i < num_permutations; i++)
	{
		// read the permutation data into the array
		BDmGet4BytesFromBuffer(buffer, perm_array[i].weight, UUtUns32, inSwapIt);
		BDmGet4BytesFromBuffer(buffer, perm_array[i].min_volume_percent, float, inSwapIt);
		BDmGet4BytesFromBuffer(buffer, perm_array[i].max_volume_percent, float, inSwapIt);
		BDmGet4BytesFromBuffer(buffer, perm_array[i].min_pitch_percent, float, inSwapIt);
		BDmGet4BytesFromBuffer(buffer, perm_array[i].max_pitch_percent, float, inSwapIt);
		BDmGetStringFromBuffer(buffer, perm_array[i].sound_data_name, BFcMaxFileNameLength, inSwapIt);
		
		// get a pointer to the sound buffer
		perm_array[i].sound_data = NULL;
	}
	
	if (inAllocated == UUcTrue)
	{
		UUrMemory_Block_Delete(ioBinaryData);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiGroup_LevelLoad(
	void)
{
	UUtError					error;
	
	error = OSrGroup_BuildHashTable();
	UUmError_ReturnOnError(error);
	
	SSrGroup_UpdateSoundDataPointers();
		
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OSiGroup_LevelUnload(
	void)
{
	// delete the hash table
	if (OSgGroupTable != NULL)
	{
		AUrHashTable_Delete(OSgGroupTable);
		OSgGroupTable = NULL;
	}
}

// ----------------------------------------------------------------------
UUtError
OSrGroup_New(
	const char				*inName,
	SStGroup				**outGroup)
{
	UUtError				error;
	SStGroup				*group;
	char					name[SScMaxNameLength];
	
	*outGroup = NULL;
	
	OSrMakeGoodName(inName, name);
	
	group = OSrGroup_GetByName(name);
	if (group != NULL) { return UUcError_Generic; }
	
	error = SSrGroup_New(name, outGroup);
	UUmError_ReturnOnError(error);
	
	error = OSrGroup_BuildHashTable();
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OSrGroup_Save(
	SStGroup				*inGroup,
	BFtFileRef				*inParentDirRef)
{
	UUtError				error;
	BFtFileRef				*file_ref;
	BFtFile					*file;
	UUtUns32				num_permutations;
	UUtUns32				i;
	SStPermutation			*perm_array;
	UUtUns8					block[OScMaxBufferSize + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8					*data = UUrAlignMemory(block);
	UUtUns32				data_size = OScMaxBufferSize;
	UUtUns8					*buffer;
	UUtUns32				bytes_avail;
	char					name[BFcMaxFileNameLength];
	UUtUns32				bytes_needed;
	UUtBool					mem_allocated;
	
	// create the file ref
	sprintf(name, "%s.%s", inGroup->group_name, OScGroupSuffix);
	error = BFrFileRef_DuplicateAndAppendName(inParentDirRef, name, &file_ref);
	UUmError_ReturnOnError(error);
	
	// create the file if it doesn't exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		error = BFrFile_Create(file_ref);
		UUmError_ReturnOnErrorMsg(error, "Could not create the file.");
	}
		
	// open the file for writing
	error =
		BFrFile_Open(
			file_ref,
			"w",
			&file);
	UUmError_ReturnOnErrorMsg(error, "Could not open the group file.");
	
	// set the pos to the beginning of the file
	BFrFile_SetPos(file, 0);
	
	// get the number of permutations
	num_permutations = UUrMemory_Array_GetUsedElems(inGroup->permutations);
	
	// determine if memory needs to be allocated
	mem_allocated = UUcFalse;
	bytes_needed =
		(num_permutations * sizeof(SStPermutation)) +
		(sizeof(UUtUns32) * 3) +
		(sizeof(UUtUns16) * 2) +
		(sizeof(float) * 2);
	if (bytes_needed > data_size)
	{
		data = UUrMemory_Block_New(bytes_needed);
		UUmError_ReturnOnNull(data);
		
		data_size = bytes_needed;
		mem_allocated = UUcTrue;
	}

	// setup the buffer pointer
	buffer = data;
	bytes_avail = data_size;
	
	// write the version
	BDmWrite4BytesToBuffer(buffer, OS2cCurrentVersion, UUtUns32, bytes_avail, BDcWrite_Little);
	
	// write the group volume and pitch
	BDmWrite4BytesToBuffer(buffer, inGroup->group_volume, float, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inGroup->group_pitch, float, bytes_avail, BDcWrite_Little);
	
	// write the flag and flag data
	BDmWrite2BytesToBuffer(buffer, (inGroup->flags & SScGroupFlag_WriteMask), UUtUns16, bytes_avail, BDcWrite_Little);
	BDmWrite2BytesToBuffer(buffer, inGroup->flag_data, UUtUns16, bytes_avail, BDcWrite_Little);
	
	// write the group channels and permutations to the buffer
	BDmWrite4BytesToBuffer(buffer, inGroup->num_channels, UUtUns32, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, num_permutations, UUtUns32, bytes_avail, BDcWrite_Little);
	
	perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(inGroup->permutations);
	for (i = 0; i < num_permutations; i++)
	{
		SStPermutation			*perm;
		
		perm = &perm_array[i];
		
		BDmWrite4BytesToBuffer(buffer, perm->weight, UUtUns32, bytes_avail, BDcWrite_Little);
		BDmWrite4BytesToBuffer(buffer, perm->min_volume_percent, float, bytes_avail, BDcWrite_Little);
		BDmWrite4BytesToBuffer(buffer, perm->max_volume_percent, float, bytes_avail, BDcWrite_Little);
		BDmWrite4BytesToBuffer(buffer, perm->min_pitch_percent, float, bytes_avail, BDcWrite_Little);
		BDmWrite4BytesToBuffer(buffer, perm->max_pitch_percent, float, bytes_avail, BDcWrite_Little);
		BDmWriteStringToBuffer(buffer, perm->sound_data_name, SScMaxNameLength, bytes_avail, BDcWrite_Little);
	}
	
	// write the group
	error =
		BDrBinaryData_Write(
			OScGroupDataClass,
			data,
			(data_size - bytes_avail),
			file);
	UUmError_ReturnOnErrorMsg(error, "Unable to write to file.");
	
	if (mem_allocated)
	{
		UUrMemory_Block_Delete(data);
		data = NULL;
	}
	
	// set the EOF
	BFrFile_SetEOF(file);
	
	// close the file
	BFrFile_Close(file);
	file = NULL;
	
	UUrMemory_Block_Delete(file_ref);
	file_ref = NULL;
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OSrImpulse_BuildHashTable(
	void)
{
	UUtUns32				num_impulses;
	UUtUns32				i;
	
	// delete the old table
	if (OSgImpulseTable != NULL)
	{
		AUrHashTable_Delete(OSgImpulseTable);
		OSgImpulseTable = NULL;
	}
	
	// allocate a new table
	OSgImpulseTable =
		AUrHashTable_New(
			sizeof(OStHashElement),
			OScNumBuckets,
			OSiHashElement_GetHashValue,
			OSiHashElement_IsEqual);
	UUmError_ReturnOnNull(OSgImpulseTable);
	
	// add the ipulse sounds to the table
	num_impulses = SSrImpulse_GetNumImpulseSounds();
	for (i = 0; i < num_impulses; i++)
	{
		SStImpulse				*impulse;
		OStHashElement			hash_elem;
		
		impulse = SSrImpulse_GetByIndex(i);
		if (impulse == NULL) { break; }
		
		hash_elem.name = impulse->impulse_name;
		hash_elem.u.impulse = impulse;
		
		AUrHashTable_Add(OSgImpulseTable, &hash_elem);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrImpulse_ChangeName(
	SStImpulse				*inImpulse,
	const char				*inName)
{
	char					name[SScMaxNameLength];

	UUmAssert(inImpulse);
	UUmAssert(inName);
	
	// save the old name
	UUrString_Copy(name, inImpulse->impulse_name, SScMaxNameLength);
	
	// change the impulse's name
	UUrString_Copy(inImpulse->impulse_name, inName, SScMaxNameLength);
	
	// update the impulse names
	OSrVariantList_UpdateImpulseName(name, inName);
	
	// rebuild the hash table
	OSrImpulse_BuildHashTable();
}

// ----------------------------------------------------------------------
UUtError
OSrImpulse_Delete(
	const char				*inName)
{
	UUtError				error;
	
	SSrImpulse_Delete(inName, UUcTrue);
	
	error = OSrImpulse_BuildHashTable();
	UUmError_ReturnOnError(error);

	// an impulse sound has been deleted, we must possibly delete its pointer
	ONrImpactEffects_SetupSoundPointers();
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
SStImpulse*
OSrImpulse_GetByName(
	const char				*inName)
{
	OStHashElement			*hash_elem;
	
	// find the hash element for the impulse sound named inName
	hash_elem = OSiHasElement_GetByName(OSgImpulseTable, inName);
	if (hash_elem == NULL) { return NULL; }
	
	// return the impulse pointer
	return hash_elem->u.impulse;
}

// ----------------------------------------------------------------------
static UUtError
OSiImpulse_Load(
	const char				*inIdentifier,
	BDtData					*ioBinaryData,
	UUtBool					inSwapIt,
	UUtBool					inLocked,
	UUtBool					inAllocated)
{
	UUtError				error;
	SStImpulse				*impulse;
	
	UUtUns8					*buffer;
	UUtUns32				buffer_size;
	UUtUns32				version;
	
	char					name[SScMaxNameLength];
	
	UUmAssert(inIdentifier);
	UUmAssert(ioBinaryData);
	
	// ------------------------------
	// create a new impulse sound
	// ------------------------------
	OSrMakeGoodName(inIdentifier, name);
	
	// if no group with this name exists, create a new one,
	// if one does exist, take it over
	impulse = SSrImpulse_GetByName(name);
	if (impulse == NULL)
	{
		// create a new impulse sound
		error = SSrImpulse_New(name, &impulse);
		UUmError_ReturnOnError(error);
	}
	else
	{
		impulse->impulse_group = NULL;
		impulse->alt_impulse = NULL;
	}
	
	// ------------------------------
	// read the binary data
	// ------------------------------
	buffer = ioBinaryData->data;
	buffer_size = ioBinaryData->header.data_size;
	
	// read the version number
	BDmGet4BytesFromBuffer(buffer, version, UUtUns32, inSwapIt);
	
	BDmGetStringFromBuffer(buffer, impulse->impulse_group_name, SScMaxNameLength, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, impulse->priority, SStPriority2, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, impulse->max_volume_distance, float, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, impulse->min_volume_distance, float, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, impulse->max_volume_angle, float, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, impulse->min_volume_angle, float, inSwapIt);
	BDmGet4BytesFromBuffer(buffer, impulse->min_angle_attenuation, float, inSwapIt);
	
	if (version < OS2cVersion_4)
	{
		impulse->alt_threshold = 0;
		impulse->alt_impulse = NULL;
		UUrMemory_Clear(impulse->alt_impulse_name, SScMaxNameLength);
	}
	else
	{
		BDmGet4BytesFromBuffer(buffer, impulse->alt_threshold, UUtUns32, inSwapIt);
		BDmGetStringFromBuffer(buffer, impulse->alt_impulse_name, SScMaxNameLength, inSwapIt);
	}
	
	if (version < OS2cVersion_5)
	{
		impulse->impact_velocity = 0;
	}
	else
	{
		BDmGet4BytesFromBuffer(buffer, impulse->impact_velocity, float, inSwapIt);
	}
	
	if (version < OS2cVersion_6)
	{
		impulse->min_occlusion = 0.0f;
	}
	else
	{
		BDmGet4BytesFromBuffer(buffer, impulse->min_occlusion, float, inSwapIt);
	}
	
	if (inAllocated == UUcTrue)
	{
		UUrMemory_Block_Delete(ioBinaryData);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiImpulse_LevelLoad(
	void)
{
	UUtError					error;
	
	error = OSrImpulse_BuildHashTable();
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OSiImpulse_LevelUnload(
	void)
{
	// delete the hash table
	if (OSgImpulseTable != NULL)
	{
		AUrHashTable_Delete(OSgImpulseTable);
		OSgImpulseTable = NULL;
	}
}

// ----------------------------------------------------------------------
UUtError
OSrImpulse_New(
	const char				*inName,
	SStImpulse				**outImpulse)
{
	UUtError				error;
	char					name[SScMaxNameLength];
	
	OSrMakeGoodName(inName, name);

	error = SSrImpulse_New(name, outImpulse);
	UUmError_ReturnOnError(error);
	
	error = OSrImpulse_BuildHashTable();
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrImpulse_Play(
	SStImpulse				*inImpulse,
	const M3tPoint3D		*inPosition,
	const M3tVector3D		*inDirection,
	const M3tVector3D		*inVelocity,
	float					*inVolume)
{
	UUtBool					play;
	M3tPoint3D				position;
	float					volume;
		
	UUmAssert(inImpulse);
	
	if (inPosition == NULL) {
		// do not play spatialized
		volume = 1.0f;
		play = UUcTrue;
	} else {
		// calculate spatialization and occlusion
		play =
			OSiCalc_PositionAndVolume(
				inPosition,
				inImpulse->max_volume_distance,
				inImpulse->min_volume_distance,
				inImpulse->min_occlusion,
				&position,
				&volume,
				UUcFalse,
				NULL);
	}
	
	if (inVolume != NULL)
	{
		volume *= (*inVolume);
	}
	
	if ((play) && (OSgScriptOnly == UUcFalse))
	{
		SSrImpulse_Play(inImpulse, inPosition, inDirection, inVelocity, &volume);
	}
/*	else if (SSgShowDebugInfo)
	{
		COrConsole_Printf("OSrImpulse_Play: could not play %s", inImpulse->impulse_name);
	}*/
}

// ----------------------------------------------------------------------
void
OSrImpulse_PlayByName(
	const char				*inImpulseName,
	const M3tPoint3D		*inPosition,
	const M3tVector3D		*inDirection,
	const M3tVector3D		*inVelocity,
	float					*inVolume)
{
	SStImpulse				*impulse;
	
	impulse = OSrImpulse_GetByName(inImpulseName);
	if (impulse == NULL) { return; }
	
	OSrImpulse_Play(impulse, inPosition, inDirection, inVelocity, inVolume);
}

// ----------------------------------------------------------------------
UUtError
OSrImpulse_Save(
	SStImpulse				*inImpulse,
	BFtFileRef				*inParentDirRef)
{
	UUtError				error;
	BFtFileRef				*file_ref;
	BFtFile					*file;
	UUtUns8					block[OScMaxBufferSize + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8					*data = UUrAlignMemory(block);
	UUtUns8					*buffer;
	UUtUns32				bytes_avail;
	char					name[BFcMaxFileNameLength];
	
	// create the file ref
	sprintf(name, "%s.%s", inImpulse->impulse_name, OScImpulseSuffix);
	error = BFrFileRef_DuplicateAndAppendName(inParentDirRef, name, &file_ref);
	UUmError_ReturnOnError(error);
	
	// create the file if it doesn't exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		error = BFrFile_Create(file_ref);
		UUmError_ReturnOnErrorMsg(error, "Could not create the file.");
	}
	
	// open the file for writing
	error =
		BFrFile_Open(
			file_ref,
			"w",
			&file);
	UUmError_ReturnOnErrorMsg(error, "Could not open the group file.");
	
	// set the pos to the beginning of the file
	BFrFile_SetPos(file, 0);
	
	// setup the buffer pointer
	buffer = data;
	bytes_avail = OScMaxBufferSize;
	
	// write the version
	BDmWrite4BytesToBuffer(buffer, OS2cCurrentVersion, UUtUns32, bytes_avail, BDcWrite_Little);
	
	// write the ambient sound to the buffer
	BDmWriteStringToBuffer(buffer, inImpulse->impulse_group_name, SScMaxNameLength, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inImpulse->priority, SStPriority2, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inImpulse->max_volume_distance, float, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inImpulse->min_volume_distance, float, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inImpulse->max_volume_angle, float, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inImpulse->min_volume_angle, float, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inImpulse->min_angle_attenuation, float, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inImpulse->alt_threshold, UUtUns32, bytes_avail, BDcWrite_Little);
	BDmWriteStringToBuffer(buffer, inImpulse->alt_impulse_name, SScMaxNameLength, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inImpulse->impact_velocity, float, bytes_avail, BDcWrite_Little);
	BDmWrite4BytesToBuffer(buffer, inImpulse->min_occlusion, float, bytes_avail, BDcWrite_Little);
	
	// write the impulse sound
	error =
		BDrBinaryData_Write(
			OScImpulseDataClass,
			data,
			(OScMaxBufferSize - bytes_avail),
			file);
	UUmError_ReturnOnErrorMsg(error, "Unable to write to file.");
	
	// set the EOF
	BFrFile_SetEOF(file);
	
	// close the file
	BFrFile_Close(file);
	file = NULL;
	
	UUrMemory_Block_Delete(file_ref);
	file_ref = NULL;
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OSiTest(
	void)
{
	UUtUns32				itr;
	UUtUns32				num_items;
	
	// test groups
	num_items = SSrGroup_GetNumSoundGroups();
	for (itr = 0; itr < num_items; itr++)
	{
		SStGroup				*group;
		
		group = SSrGroup_GetByIndex(itr);
		if (group != OSrGroup_GetByName(group->group_name))
		{
			COrConsole_Printf("OSrGroup_GetByName() failed to find %s", group->group_name);
		}
	}

	// test impulse
	num_items = SSrImpulse_GetNumImpulseSounds();
	for (itr = 0; itr < num_items; itr++)
	{
		SStImpulse				*impulse;
		
		impulse = SSrImpulse_GetByIndex(itr);
		if (impulse != OSrImpulse_GetByName(impulse->impulse_name))
		{
			COrConsole_Printf("OSrImpulse_GetByName() failed to find %s", impulse->impulse_name);
		}
	}


	// test ambients
	num_items = SSrAmbient_GetNumAmbientSounds();
	for (itr = 0; itr < num_items; itr++)
	{
		SStAmbient				*ambient;
		
		ambient = SSrAmbient_GetByIndex(itr);
		if (ambient != OSrAmbient_GetByName(ambient->ambient_name))
		{
			COrConsole_Printf("OSrAmbient_GetByName() failed to find %s", ambient->ambient_name);
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
SStPlayID
OSrInGameDialog_Play(
	const char				*inName,
	M3tPoint3D				*inLocation,
	UUtUns32				*outDuration,
	UUtBool					*outSoundNotFound)
{
	SStAmbient				*ambient;
	SStPlayID				play_id = SScInvalidID;
	UUtUns32				sound_duration = 0;
	UUtBool					has_all_sounds, not_found = UUcTrue;

	ambient = OSrAmbient_GetByName(inName);
	if (ambient == NULL) {
		goto exit;
	}
	
	has_all_sounds = SSrAmbient_CheckSoundData(ambient);
	if (has_all_sounds) {
		// we found all the sound data
		not_found = UUcFalse;
	}
	
	play_id = OSrAmbient_Start(ambient, inLocation, NULL, NULL, NULL, NULL);
	sound_duration = OSiAmbient_GetSimpleDuration(ambient);
	if (sound_duration == 0) {
		// could not successfully get a duration from the sound, get a default duration
		sound_duration = 120;
	}
	
	OSrSubtitle_Draw(ambient, sound_duration + OScSubtitleStayTime);

exit:
	if (outDuration) {
		*outDuration = sound_duration;
	}

	if (outSoundNotFound) {
		*outSoundNotFound = not_found;
	}

	return play_id;
}

// ----------------------------------------------------------------------
static UUtBool
OSiDialog_Start(
	const char				*inName,
	UUtBool					inInterrupt)
{
	SStAmbient				*ambient;
	float					volume;
	UUtUns32				sound_duration;
	UUtBool					setup_endcue;
	
	if (OSgDialog_Playing) {
		// there is already dialogue playing
		if (inInterrupt) {
			// halt the existing dialogue
			SSrAmbient_Halt(OSgDialog_PlayID);

			OSgDialog_Playing = UUcFalse;
			OSgDialog_PlayID = SScInvalidID;
			OSgDialog_EndTime = 0;
		} else {
			// stall until the dialogue is finished
			return UUcTrue;
		}
	}
	
	ambient = OSrAmbient_GetByName(inName);
	if (ambient == NULL) {
		return UUcFalse;
	}
	
	OSgDialog_Playing = UUcTrue;
	
	// scripted dialog is always highest priority
	ambient->priority = SScPriority2_Highest;
	
	// start the ambient sound playing
	volume = 1.0f;
	OSgDialog_PlayID = SSrAmbient_Start_Simple(ambient, &volume);
	
	setup_endcue = UUcTrue;
	sound_duration = OSiAmbient_GetSimpleDuration(ambient);
	if (sound_duration == 0) {
		// could not successfully get a duration from the sound.
		if (OSgDialog_PlayID == SScInvalidID) {
			// the sound is not actually playing, get a default duration
			sound_duration = 120;
		} else {
			// the sound is playing, but we couldn't get a duration from it;
			// don't set up an end cue, use the sound instead
			setup_endcue = UUcFalse;
		}
	}
	
	if (setup_endcue) {
		// cue up the dialogue to stop blocking at the correct game tick, even if the sound is
		// still playing (maybe because we are in fast-forward mode)
		OSgDialog_EndTime = ONrGameState_GetGameTime() + sound_duration;
	} else {
		OSgDialog_EndTime = 0;
	}
	
	OSrSubtitle_Draw(ambient, sound_duration + OScSubtitleStayTime);
	
	// we completed, don't stall any longer
	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtError
OSiDialog_Play(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	OSiDialog_Start(inParameterList[0].val.str, UUcFalse);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiDialog_Play_Blocked(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	*outStall = OSiDialog_Start(inParameterList[0].val.str, UUcFalse);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiDialog_Play_Interrupted(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	OSiDialog_Start(inParameterList[0].val.str, UUcTrue);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrListBrokenSounds(
	void)
{
	UUtError				error;
	BFtFileRef				*file_ref;
	BFtFile					*file;
	
	// create a file ref
	error = BFrFileRef_MakeFromName("BrokenSoundLinks.txt", &file_ref);
	if (error != UUcError_None) {
		return;
	}
	
	// create the .TXT file if it doesn't already exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		// create the broken sound file
		error = BFrFile_Create(file_ref);
		if (error != UUcError_None) {
			return;
		}
	}
	
	// open the file
	error = BFrFile_Open(file_ref, "rw", &file);
	if (error != UUcError_None) {
		return;
	}
	
	// set the position to 0
	error = BFrFile_SetPos(file, 0);
	if (error != UUcError_None) {
		return;
	}
	
	// write the broken links
	SSrListBrokenSounds(file);
	OSrSA_ListBrokenSounds(file);
	P3rListBrokenLinks(file);
	ONrCharacter_ListBrokenSounds(file);
	OSiNeutralInteractions_ListBrokenSounds(file);

	// set the end of the file
	BFrFile_SetEOF(file);
	
	// close the file
	BFrFile_Close(file);
	file = NULL;
	
	// delete the file ref
	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;
}

// ----------------------------------------------------------------------
static UUtError
OSiListBrokenSounds_Callback(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	OSrListBrokenSounds();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiScriptAmbient_Start(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	SStAmbient				*ambient;
	float					volume;
	UUtUns32				itr;
	OStScriptAmbient		*script_ambient;
	
	// get the ambient sound from the name
	ambient = OSrAmbient_GetByName(inParameterList[0].val.str);
	if (ambient == NULL) { return UUcError_None; }
	
	// get the volume
	if (inParameterListLength == 2)
	{
		volume = UUmPin(inParameterList[1].val.f, 0.0f, 1.0f);
	}
	else
	{
		volume = 1.0f;
	}
	
	// Find an available slot to play the script ambient
	script_ambient = NULL;
	for (itr = 0; itr < OScMaxScriptAmbients; itr++)
	{
		script_ambient = &OSgScriptAmbients[itr];
		if (OSgScriptAmbients[itr].play_id == SScInvalidID) { break; }
	}
	if (itr == OScMaxScriptAmbients) { return UUcError_None; }
	
	// save the name for later scripts and save the user desired volume
	UUrString_Copy(script_ambient->name, inParameterList[0].val.str, SScMaxNameLength);
	script_ambient->volume = volume;
		
	// scripted ambients are always highest priority
	ambient->priority = SScPriority2_Highest;
	
	// start the ambient sound
	script_ambient->play_id = SSrAmbient_Start_Simple(ambient, &volume);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiScriptAmbient_Stop(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32				itr;
	
	for (itr = 0; itr < OScMaxScriptAmbients; itr++)
	{
		OStScriptAmbient		*script_ambient;
		UUtInt32				result;
		
		script_ambient = &OSgScriptAmbients[itr];
		if (script_ambient->play_id == SScInvalidID) { continue; }

		result = UUrString_Compare_NoCase(script_ambient->name, inParameterList[0].val.str);
		if (result != 0) { continue; }
		
		// stop the music
		SSrAmbient_Stop(script_ambient->play_id);
		break;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiScriptAmbient_Volume(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32				itr;
	float					volume;
	float					time;
	
	volume = UUmPin(inParameterList[1].val.f, 0.0f, 1.0f);
	
	if (inParameterListLength == 3)
	{
		time = inParameterList[2].val.f;
	}
	else
	{
		time = 0.0f;
	}
	
	for (itr = 0; itr < OScMaxScriptAmbients; itr++)
	{
		OStScriptAmbient		*script_ambient;
		UUtInt32				result;
		
		script_ambient = &OSgScriptAmbients[itr];
		
		result = UUrString_Compare_NoCase(script_ambient->name, inParameterList[0].val.str);
		if (result != 0) { continue; }
		
		// save the music volume
		script_ambient->volume = inParameterList[1].val.f;

		// set the volume
		SSrAmbient_SetVolume(script_ambient->play_id, volume, time);
		
		break;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiSound_Impulse_Play(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	SStImpulse				*impulse;
	float					volume;
	
	impulse = OSrImpulse_GetByName(inParameterList[0].val.str);
	if (impulse == NULL) { return UUcError_None; }
	
	if (inParameterListLength == 2)
	{
		volume = UUmPin(inParameterList[1].val.f, 0.0f, 1.0f);
	}
	else
	{
		volume = 1.0f;
	}
	
	OSrImpulse_Play(impulse, NULL, NULL, NULL, &volume);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiScriptCommands_Initialize(
	void)
{
	UUtError					error;
	UUtUns32					itr;
	
	// initialize the globals
	OSgDialog_Playing = UUcFalse;
	OSgDialog_PlayID = SScInvalidID;
	OSgDialog_EndTime = 0;
	
	for (itr = 0; itr < OScMaxScriptAmbients; itr++)
	{
		OSgScriptAmbients[itr].play_id = SScInvalidID;
	}
		
	// initialize the commands
	error = 
		SLrScript_Command_Register_Void(
			"sound_music_start",
			"function to start music playing",
			"name:string [volume:float | ]",
			OSiScriptAmbient_Start);
	UUmError_ReturnOnError(error);
	
	error = 
		SLrScript_Command_Register_Void(
			"sound_music_stop",
			"function to stop playing music",
			"name:string",
			OSiScriptAmbient_Stop);
	UUmError_ReturnOnError(error);
	
	error = 
		SLrScript_Command_Register_Void(
			"sound_music_volume",
			"function to set the volume of playing music",
			"name:string volume:float [time:float | ]",
			OSiScriptAmbient_Volume);
	UUmError_ReturnOnError(error);
	
	error = 
		SLrScript_Command_Register_Void(
			"sound_dialog_play",
			"function to start character dialog playing",
			"name:string",
			OSiDialog_Play);
	UUmError_ReturnOnError(error);

	error = 
		SLrScript_Command_Register_Void(
			"sound_dialog_play_block",
			"function to start character dialog playing after the current dialog finishes",
			"name:string",
			OSiDialog_Play_Blocked);
	UUmError_ReturnOnError(error);
	
	error = 
		SLrScript_Command_Register_Void(
			"sound_dialog_play_interrupt",
			"function to interrupt the current character dialog and play a new one",
			"name:string",
			OSiDialog_Play_Interrupted);
	UUmError_ReturnOnError(error);
	
	error =
		SLrScript_Command_Register_Void(
			"sound_ambient_start",
			"function to start an ambient sound",
			"name:string [volume:float | ]",
			OSiScriptAmbient_Start);
	UUmError_ReturnOnError(error);

	error =
		SLrScript_Command_Register_Void(
			"sound_ambient_stop",
			"function to stop an ambient sound",
			"name:string",
			OSiScriptAmbient_Stop);
	UUmError_ReturnOnError(error);

	error =
		SLrScript_Command_Register_Void(
			"sound_ambient_volume",
			"function to set the volume of a playing ambient sound",
			"name:string volume:float [time:float | ]",
			OSiScriptAmbient_Volume);
	UUmError_ReturnOnError(error);

	error =
		SLrScript_Command_Register_Void(
			"sound_impulse_play",
			"function plays an impulse sound",
			"name:string [volume:float | ]",
			OSiSound_Impulse_Play);
	UUmError_ReturnOnError(error);
	
	error =
		SLrScript_Command_Register_Void(
			"sound_list_broken_links",
			"function writes a list of sounds which have broken links to a file",
			"",
			OSiListBrokenSounds_Callback);
	UUmError_ReturnOnError(error);

#if TOOL_VERSION
	error =
		SLrGlobalVariable_Register_Bool(
			"sound_show_occlusions",
			"enables debugging display of sound occlusions",
			&OSgShowOcclusions);
#endif

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OSiScriptCommands_Terminate(
	void)
{
	UUtUns32					itr;
	
	for (itr = 0; itr < OScMaxScriptAmbients; itr++)
	{
		if (OSgScriptAmbients[itr].play_id == SScInvalidID) { continue; }
		
		SSrAmbient_Halt(OSgScriptAmbients[itr].play_id);
		OSgScriptAmbients[itr].play_id = SScInvalidID;
	}
}

// ----------------------------------------------------------------------
static void
OSiScriptCommands_Update(
	void)
{
	UUtUns32					current_time;
	UUtUns32					itr;

	// check to playing ambients for sounds that have stopped
	for (itr = 0; itr < OScMaxScriptAmbients; itr++)
	{
		UUtInt32			result;
		OStScriptAmbient	*script_ambient;
		
		script_ambient = &OSgScriptAmbients[itr];
		if (script_ambient->play_id == SScInvalidID) { continue; }
		
		result = SSrAmbient_Update(script_ambient->play_id, NULL, NULL, NULL, NULL);
		if (result == UUcFalse)
		{
			script_ambient->play_id = SScInvalidID;
		}
	}
	
	if (!OSgDialog_Playing) {
		UUmAssert(OSgDialog_PlayID == SScInvalidID);
		return;
	}
	
	current_time = ONrGameState_GetGameTime();
	if (OSgDialog_PlayID != SScInvalidID)
	{
		UUtBool					active;
		
		active = SSrAmbient_Update(OSgDialog_PlayID, NULL, NULL, NULL, NULL);
		if (active == UUcFalse)
		{
			// the piece of dialogue has finished
			OSgDialog_Playing = UUcFalse;
			OSgDialog_PlayID = SScInvalidID;
			OSgDialog_EndTime = 0;
			return;
		}
	}
	
	if (current_time >= OSgDialog_EndTime) {
		// the dialogue's time is up, even if the sound is still playing we
		// should go on to the next one (i.e. stop blocking). this makes fast-forwarding
		// through cutscenes more palatable.
		OSgDialog_Playing = UUcFalse;
		OSgDialog_EndTime = 0;
		OSgDialog_PlayID = SScInvalidID;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OSrMusic_Halt(
	void)
{
	if (OSgMusic_PlayID == SScInvalidID) { return; }
	
	SSrAmbient_Halt(OSgMusic_PlayID);
	OSgMusic_PlayID = SScInvalidID;
}

// ----------------------------------------------------------------------
UUtBool
OSrMusic_IsPlaying(
	void)
{
	return (OSgMusic_PlayID != SScInvalidID);
}

// ----------------------------------------------------------------------
void
OSrMusic_SetVolume(
	float					inVolume,
	float					inTime)
{
	if (OSgMusic_PlayID == SScInvalidID) { return; }
	
	SSrAmbient_SetVolume(OSgMusic_PlayID, inVolume, inTime);
}

// ----------------------------------------------------------------------
void
OSrMusic_Start(
	const char				*inMusicName,
	float					inVolume)
{
	SStAmbient				*ambient;
	float					volume;
	
	if ((inMusicName == NULL) /*|| (OSgMusic_PlayID != SScInvalidID)*/) { return; }
	
	ambient = OSrAmbient_GetByName(inMusicName);
	if (ambient == NULL) { return; }
	
	// music is always highest priority
	ambient->priority = SScPriority2_Highest;
	
	volume = inVolume;
	OSgMusic_PlayID = SSrAmbient_Start_Simple(ambient, &volume);
}

// ----------------------------------------------------------------------
void
OSrMusic_Stop(
	void)
{
	if (OSgMusic_PlayID == SScInvalidID) { return; }
	
	SSrAmbient_Stop(OSgMusic_PlayID);
}

// ----------------------------------------------------------------------
static void
OSiMusic_Update(
	void)
{
	if (OSgMusic_PlayID == SScInvalidID) { return; }
	
	if (SSrAmbient_Update(OSgMusic_PlayID, NULL, NULL, NULL, NULL) == UUcFalse)
	{
		OSgMusic_PlayID = SScInvalidID;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OSiBinFile_Load(
	BFtFileRef				*inFileRef)
{
	UUtError			error;

	// if the file exists
	if (BFrFileRef_FileExists(inFileRef))
	{
		void			*data;
		UUtUns32		data_size;
		char			identifier[BFcMaxFileNameLength];
		UUtBool			locked;
		
		// load the file into memory
		error = BFrFileRef_LoadIntoMemory(inFileRef, &data_size, &data);
		UUmError_ReturnOnError(error);
		
		// get the identifier
		UUrString_Copy(identifier, BFrFileRef_GetLeafName(inFileRef), BFcMaxFileNameLength);
		UUrString_StripExtension(identifier);
		
		// get the locked status of the file
		locked = BFrFileRef_IsLocked(inFileRef);
		
		// process the file
		error =
			BDrBinaryLoader(
				identifier,
				data,
				data_size,
				locked,
				UUcTrue);
		UUmError_ReturnOnError(error);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiBinFiles_GetDirectory(
	BFtFileRef				**outDirectoryRef,
	UUtBool					inCreate)
{
	UUtError				error;
	BFtFileRef				prefFileRef;
	GRtGroup_Context		*prefGroupContext;
	GRtGroup				*prefGroup;
	char					*binaryDirName;
	char					binaryDirNameStr[BFcMaxPathLength];
	BFtFileRef				*binaryDataFolder;
	BFtFileRef				*soundDirRef;
	
	UUmAssert(outDirectoryRef);
	
	// find the preferences file
#if defined(SHIPPING_VERSION) && (!SHIPPING_VERSION)
	error = BFrFileRef_Search("Preferences.txt", &prefFileRef);
#else
	error= UUcError_Generic;
#endif
	if (error != UUcError_None)
	{
		*outDirectoryRef = NULL;
		return UUcError_Generic;
	}
	
	// create a group from the file
	error =
		GRrGroup_Context_NewFromFileRef(
			&prefFileRef,
			NULL,
			NULL,
			&prefGroupContext,
			&prefGroup);	
	if (error != UUcError_None) { return error; }
	
	// get the data file directory
	error = GRrGroup_GetString(prefGroup, "BinFileDir", &binaryDirName);
	if (error != UUcError_None)
	{
		char						*dataFileDir;
		
		// get the data file directory
		error = GRrGroup_GetString(prefGroup, "DataFileDir", &dataFileDir);
		if (error != UUcError_None) { return error; }
		
		// and append the binary data path to the data file dir name
		sprintf(binaryDirNameStr, "%s\\Binary", dataFileDir);
		binaryDirName = binaryDirNameStr;
	}	

	// make a file ref for the binary data folder
	error = BFrFileRef_MakeFromName(binaryDirName, &binaryDataFolder);
	if (error != UUcError_None) { return error; }
	
	GRrGroup_Context_Delete(prefGroupContext);

	// make sure the binary data folder exists
	if (BFrFileRef_FileExists(binaryDataFolder) == UUcFalse)
	{
		UUrMemory_Block_Delete(binaryDataFolder);
		binaryDataFolder = NULL;
		
		*outDirectoryRef = NULL;
		return UUcError_Generic;
	}
	
	// find the level0\sound directory
	error =
		BFrFileRef_DuplicateAndAppendName(
			binaryDataFolder,
			"Level0\\Sounds",
			&soundDirRef);
	UUmError_ReturnOnErrorMsg(error, "Could not create a file ref for Binary\\Level0\\Sounds");
	
	// delete the binary data folder
	UUrMemory_Block_Delete(binaryDataFolder);
	binaryDataFolder = NULL;
	
	// make sure the sound directory exists
	if (BFrFileRef_FileExists(soundDirRef) == UUcFalse)
	{
		if (inCreate)
		{
			// make the directory
			error = BFrDirectory_Create(soundDirRef, NULL);
			UUmError_ReturnOnErrorMsg(error, "Could not create the folder on disk");
		}
		else
		{
			// can't find the directory
			UUrMemory_Block_Delete(soundDirRef);
			soundDirRef = NULL;
			
			return UUcError_Generic;
		}
	}
	
	*outDirectoryRef = soundDirRef;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiBinFiles_Load(
	BFtFileRef				*inDirRef,
	const char				*inSuffix)
{
	UUtError error;
	UUtUns32 count = BFrFileCache_Count(OSgBinFiles_Load_Cache);
	UUtUns32 itr;
		
	for(itr = 0; itr < count; itr++)
	{
		BFtFileRef	*file_ref = BFrFileCache_GetIndex(OSgBinFiles_Load_Cache, itr);
		const char	*file_ext = BFrFileRef_GetSuffixName(file_ref);

		if ((file_ext != NULL) && (UUrString_Compare_NoCase(file_ext, inSuffix) == 0)) {
			error = OSiBinFile_Load(file_ref);

			if (error != UUcError_None) { break; }
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
static UUtError
OSiLoadNothing(
	const char				*inIdentifier,
	BDtData					*ioBinaryData,
	UUtBool					inSwapIt,
	UUtBool					inLocked,
	UUtBool					inAllocated)
{
	if (inAllocated == UUcTrue)
	{
		UUrMemory_Block_Delete(ioBinaryData);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiBinaryData_Register(
	void)
{
	UUtError				error;
	BDtMethods				methods;
	
	methods.rLoad = OSiAmbient_Load;
	error = BDrRegisterClass(OScAmbientDataClass, &methods);
	UUmError_ReturnOnError(error);

	methods.rLoad = OSiGroup_Load;
	error = BDrRegisterClass(OScGroupDataClass, &methods);
	UUmError_ReturnOnError(error);

	methods.rLoad = OSiImpulse_Load;
	error = BDrRegisterClass(OScImpulseDataClass, &methods);
	UUmError_ReturnOnError(error);
	
	methods.rLoad = OSiLoadNothing;
	error =	BDrRegisterClass(OScBinaryDataClass, &methods);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
OSrCheckBrokenAmbientSound(
	BFtFile						*inFile,
	char						*inCallerName,
	char						*inEventName,
	char						*inSoundName)
{
	SStAmbient					*ambient;

	if (inSoundName[0] == '\0')
		return UUcFalse;

	ambient = OSrAmbient_GetByName(inSoundName);

	if (ambient != NULL)
		return UUcFalse;

	// if this is just an old NCI placeholder then don't report it as an error
	if (UUmString_IsEqual(inEventName, "abort") && UUmString_IsEqual(inSoundName, "see_you_later")) {
		return UUcFalse;
	}
	if (UUmString_IsEqual(inEventName, "trigger") && UUmString_IsEqual(inSoundName, "hey_konoko")) {
		return UUcFalse;
	}
	if (UUmString_IsEqual(inEventName, "enemy") && UUmString_IsEqual(inSoundName, "what_the")) {
		return UUcFalse;
	}
	if ((strncmp(inEventName, "line", 4) == 0) && UUmString_IsEqual(inSoundName, "talking")) {
		return UUcFalse;
	}

	BFrFile_Printf(inFile, "%s\t%s\t%s"UUmNL, inCallerName, inEventName, inSoundName);
	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtBool
OSrCheckBrokenImpulseSound(
	BFtFile						*inFile,
	char						*inCallerName,
	char						*inEventName,
	char						*inSoundName)
{
	SStImpulse					*impulse;

	if (inSoundName[0] == '\0')
		return UUcFalse;

	impulse = OSrImpulse_GetByName(inSoundName);

	if (impulse != NULL)
		return UUcFalse;

	BFrFile_Printf(inFile, "%s\t%s\t%s"UUmNL, inCallerName, inEventName, inSoundName);
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
OSiNeutral_ListBrokenSounds(
	OBJtObject					*inObject,
	UUtUns32					inUserData)
{
	UUtUns32					itr;
	BFtFile						*file;
	OBJtOSD_Neutral				*neutral_osd;
	OBJtOSD_NeutralLine			*line;
	char						eventbuf[64];

	UUmAssert(inObject->object_type == OBJcType_Neutral);
	neutral_osd = (OBJtOSD_Neutral *) inObject->object_data;
	file = (BFtFile *)inUserData;
	
	OSrCheckBrokenAmbientSound(file, neutral_osd->name, "abort",		neutral_osd->abort_sound);
	OSrCheckBrokenAmbientSound(file, neutral_osd->name, "trigger",		neutral_osd->trigger_sound);
	OSrCheckBrokenAmbientSound(file, neutral_osd->name, "enemy",		neutral_osd->enemy_sound);

	for (itr = 0, line = neutral_osd->lines; itr < neutral_osd->num_lines; itr++, line++) {
		sprintf(eventbuf, "line%d", itr + 1);
		OSrCheckBrokenAmbientSound(file, neutral_osd->name, eventbuf,	line->sound);
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
OSiNeutralInteractions_ListBrokenSounds(
	BFtFile						*inFile)
{
	BFrFile_Printf(inFile, "********** Neutral Character Interactions **********"UUmNL);
	BFrFile_Printf(inFile, UUmNL);

	OBJrObjectType_EnumerateObjects(
		OBJcType_Neutral,
		OSiNeutral_ListBrokenSounds,
		(UUtUns32) inFile);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OSiGroup_AddToList(
	SStGroup					*inGroup,
	AUtSharedStringArray		*inStringArray)
{
	UUtError					error;
	UUtUns32					num_permutations;
	UUtUns32					itr;
	
	num_permutations = SSrGroup_GetNumPermutations(inGroup);
	
	for (itr = 0; itr < num_permutations; itr++)
	{
		SStPermutation				*permutation;
		UUtUns32					index;
	
		permutation = SSrGroup_Permutation_Get(inGroup, itr);
		if (permutation == NULL) { continue; }
		
		error =
			AUrSharedStringArray_AddString(
				inStringArray,
				permutation->sound_data_name,
				0,
				&index);
		if (error != UUcError_None)
		{
			COrConsole_Printf("Unable to add %s to string list", permutation->sound_data_name);
			return error;
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrAmbient_AddToStringList(
	SStAmbient					*inAmbient,
	AUtSharedStringArray		*inStringArray)
{
	UUtError error;

	if (inAmbient == NULL) {
		return;
	}

	// add the .aifs the ambient sound uses to the string array
	if (inAmbient->detail)
	{
		error = OSiGroup_AddToList(inAmbient->detail, inStringArray);
		if (error != UUcError_None)
		{
			COrConsole_Printf("Error adding detail .aifs of ambient %s", inAmbient->ambient_name);
			return;
		}
	}
	
	if (inAmbient->base_track1)
	{
		error = OSiGroup_AddToList(inAmbient->base_track1, inStringArray);
		if (error != UUcError_None)
		{
			COrConsole_Printf("Error adding base_track1 .aifs of ambient %s", inAmbient->ambient_name);
			return;
		}
	}
	
	if (inAmbient->base_track2)
	{
		error = OSiGroup_AddToList(inAmbient->base_track2, inStringArray);
		if (error != UUcError_None)
		{
			COrConsole_Printf("Error adding base_track2 .aifs of ambient %s", inAmbient->ambient_name);
			return;
		}
	}
	
	if (inAmbient->in_sound)
	{
		error = OSiGroup_AddToList(inAmbient->in_sound, inStringArray);
		if (error != UUcError_None)
		{
			COrConsole_Printf("Error adding in_sound .aifs of ambient %s", inAmbient->ambient_name);
			return;
		}
	}
	
	if (inAmbient->out_sound)
	{
		error = OSiGroup_AddToList(inAmbient->out_sound, inStringArray);
		if (error != UUcError_None)
		{
			COrConsole_Printf("Error adding out_sound .aifs of ambient %s", inAmbient->ambient_name);
			return;
		}
	}
}

// ----------------------------------------------------------------------
static UUtBool
OSiSoundObject_BuildStringList(
	OBJtObject					*inObject,
	UUtUns32					inUserData)
{
	SStAmbient					*ambient;
	
	ambient = OBJrSound_GetAmbient(inObject);
	OSrAmbient_AddToStringList(ambient, (AUtSharedStringArray *) inUserData);
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtBool
OSiNeutralInteraction_BuildStringList(
	OBJtObject					*inObject,
	UUtUns32					inUserData)
{
	UUtUns32					itr;
	AUtSharedStringArray		*string_array;
	SStAmbient					*ambient;
	OBJtOSD_Neutral				*neutral_osd;
	OBJtOSD_NeutralLine			*line;

	UUmAssert(inObject->object_type == OBJcType_Neutral);
	neutral_osd = (OBJtOSD_Neutral *) inObject->object_data;
	string_array = (AUtSharedStringArray*)inUserData;
	
	ambient = OSrAmbient_GetByName(neutral_osd->abort_sound);
	OSrAmbient_AddToStringList(ambient, string_array);

	ambient = OSrAmbient_GetByName(neutral_osd->trigger_sound);
	OSrAmbient_AddToStringList(ambient, string_array);

	ambient = OSrAmbient_GetByName(neutral_osd->enemy_sound);
	OSrAmbient_AddToStringList(ambient, string_array);

	for (itr = 0, line = neutral_osd->lines; itr < neutral_osd->num_lines; itr++, line++) {
		ambient = OSrAmbient_GetByName(line->sound);
		OSrAmbient_AddToStringList(ambient, string_array);
	}

	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtError
OSiObjects_WriteAiffList(
	UUtUns32					inObjectType,
	const char *				inObjectName,
	OBJtEnumCallback_Object		inAddSoundCallback)
{
	UUtError					error;
	AUtSharedStringArray		*string_array;
	AUtSharedString				*string_list;
	BFtFileRef					*file_ref;
	BFtFile						*file;
	UUtUns32					itr;
	char						file_name[BFcMaxFileNameLength];
	UUtUns32					num_strings;
	
	// create the list
	string_array = AUrSharedStringArray_New();
	UUmError_ReturnOnNull(string_array);
	
	// fill in the list
	OBJrObjectType_EnumerateObjects(
		inObjectType,
		inAddSoundCallback,
		(UUtUns32)string_array);
	
	// set the file_name
	sprintf(file_name, "L%d_%s_aiff.txt", ONrLevel_GetCurrentLevel(), inObjectName);
	
	// make file ref
	error = BFrFileRef_MakeFromName(file_name, &file_ref);
	UUmError_ReturnOnErrorMsg(error, "Unable to create file.");
	
	// create file if it doesn't exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		error = BFrFile_Create(file_ref);
		UUmError_ReturnOnErrorMsg(error, "Unable to create file.");
	}
	
	// open the file
	error = BFrFile_Open(file_ref, "rw", &file);
	UUmError_ReturnOnErrorMsg(error, "Unable to open file.");
	
	// set the position to 0
	error = BFrFile_SetPos(file, 0);
	UUmError_ReturnOnError(error);

	// write the list to a file
	string_list = AUrSharedStringArray_GetList(string_array);
	num_strings = AUrSharedStringArray_GetNum(string_array);
	for (itr = 0; itr < num_strings; itr++)
	{
		BFrFile_Printf(file, "%s"UUmNL, string_list[itr].string);
	}
	
	// close the file
	BFrFile_Close(file);
	file = NULL;
	
	// delete the file ref
	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;
	
	// delete the list
	AUrSharedStringArray_Delete(string_array);
	string_array = NULL;
	string_list = NULL;
	
	COrConsole_Printf("Finished writing %s AIFF list to file.", inObjectName);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OSrSoundObjects_WriteAiffList(
	void)
{
	return OSiObjects_WriteAiffList(OBJcType_Sound, "ambient", OSiSoundObject_BuildStringList);
}

// ----------------------------------------------------------------------
UUtError
OSrNeutralInteractions_WriteAiffList(
	void)
{
	return OSiObjects_WriteAiffList(OBJcType_Neutral, "neutral", OSiNeutralInteraction_BuildStringList);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OSiSoundObject_BuildPlayList(
	OBJtObject					*inObject,
	UUtUns32					inUserData)
{
	UUtError					error;
	OBJtOSD_All					osd_all;
	UUtUns32					index;
	OStPlayingObject			*playing_object_array;
	
	// get the object specific data
	OBJrObject_GetObjectSpecificData(inObject, &osd_all);
	
	// get the ambient sound
	error = UUrMemory_Array_GetNewElement(OSgPlayingObjects, &index, NULL);
	if (error != UUcError_None) { return UUcFalse; }
	
	// get a pointer to the memory array
	playing_object_array = (OStPlayingObject*)UUrMemory_Array_GetMemory(OSgPlayingObjects);
	if (playing_object_array == NULL) { return UUcFalse; }
	
	// initialize the array element
	playing_object_array[index].object = inObject;
	playing_object_array[index].play_id = SScInvalidID;
	playing_object_array[index].state = OScPOState_Off;
	playing_object_array[index].volume = 0.0f;
	playing_object_array[index].transition_volume = 0.0f;
	playing_object_array[index].object_volume = OBJrSound_GetVolume(inObject);
	OSiSetupOcclusionCache(index, &playing_object_array[index].occlusion_cache);
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtError
OSrPlayList_Build(
	void)
{
	UUtUns32					i;
	OStPlayingObject			*playing_object_array;
	UUtUns32					num_elements;
	
	// go through the ambient sound play list and stop the playing sounds
	playing_object_array = (OStPlayingObject*)UUrMemory_Array_GetMemory(OSgPlayingObjects);
	num_elements = UUrMemory_Array_GetUsedElems(OSgPlayingObjects);
	for (i = 0; i < num_elements; i++)
	{
		if (playing_object_array[i].play_id == SScInvalidID) { continue; }
		
		switch (OBJrSound_GetType(playing_object_array[i].object))
		{
			case OBJcSoundType_Spheres:
				SSrAmbient_Stop(playing_object_array[i].play_id);
			break;
			
			case OBJcSoundType_BVolume:
				SSrAmbient_Stop(playing_object_array[i].play_id);
			break;
		}
		
		playing_object_array[i].play_id = SScInvalidID;
	}
	
	// there are no more objects in the playing object array
	UUrMemory_Array_SetUsedElems(OSgPlayingObjects, 0, NULL);
	
	// rebuild the list
	OBJrObjectType_EnumerateObjects(OBJcType_Sound, OSiSoundObject_BuildPlayList, 0);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiPlayList_Initialize(
	void)
{
	// create the ambient sound play list
	OSgPlayingObjects =
		UUrMemory_Array_New(
			sizeof(OStPlayingObject),
			1,
			0,
			10);
	UUmError_ReturnOnNull(OSgPlayingObjects);
	
	// build a list of the spatial and evironmental ambient sounds in the level
	OBJrObjectType_EnumerateObjects(
		OBJcType_Sound,
		OSiSoundObject_BuildPlayList,
		0);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OSiPlayList_Terminate(
	void)
{
	UUtUns32					i;
	OStPlayingObject			*playing_object_array;
	UUtUns32					num_elements;
	
	// go through the ambient sound play list and stop the playing sounds
	playing_object_array = (OStPlayingObject*)UUrMemory_Array_GetMemory(OSgPlayingObjects);
	num_elements = UUrMemory_Array_GetUsedElems(OSgPlayingObjects);
	for (i = 0; i < num_elements; i++)
	{
		if (playing_object_array[i].play_id == SScInvalidID) { continue; }
		
		switch (OBJrSound_GetType(playing_object_array[i].object))
		{
			case OBJcSoundType_Spheres:
				SSrAmbient_Stop(playing_object_array[i].play_id);
			break;
			
			case OBJcSoundType_BVolume:
				SSrAmbient_Stop(playing_object_array[i].play_id);
			break;
		}
		
		playing_object_array[i].play_id = SScInvalidID;
	}
	
	// delete the ambient sound play list
	UUrMemory_Array_Delete(OSgPlayingObjects);
	OSgPlayingObjects = NULL;
}

// ----------------------------------------------------------------------
static void
OSiPlayList_CalcSphereVolume(
	OStPlayingObject			*inPlayingObject,
	float						inVolume)
{
	float						volume;
	float						volume_delta;
	
	// update occluding volumes over time
	volume = inVolume;
	volume_delta = inPlayingObject->transition_volume - volume;
	if (volume_delta > 0.0f)
	{
		inPlayingObject->transition_volume =
			UUmPin(
				(inPlayingObject->transition_volume - UUmMin(OScVolumeAdjust, volume_delta)),
				0.0f,
				1.0f);
	}
	else if (volume_delta < 0.0f)
	{
		inPlayingObject->transition_volume =
			UUmPin(
				(inPlayingObject->transition_volume - UUmMax(-OScVolumeAdjust, volume_delta)),
				0.0f,
				1.0f);
	}
	inPlayingObject->volume = inPlayingObject->transition_volume * inPlayingObject->object_volume;
}

// ----------------------------------------------------------------------
static void
OSiPlayList_Update_Spheres(
	OStPlayingObject			*inPlayingObject,
	const M3tPoint3D			*inPosition,
	const M3tPoint3D			*inFacing)
{
	M3tPoint3D					position;
	SStAmbient					*ambient;
	M3tVector3D					velocity;
	float						max_volume_distance;
	float						min_volume_distance;
	UUtBool						can_play;
	float						volume;
	
	if (OSgScriptOnly == UUcTrue) { return; }

	if (OBJrSound_PointIn(inPlayingObject->object, inPosition) == UUcFalse)
	{
		if (inPlayingObject->play_id != SScInvalidID)
		{
			SSrAmbient_Stop(inPlayingObject->play_id);
			inPlayingObject->play_id = SScInvalidID;
		}
		
		return;
	}
	
	// get the ambient sound to play
	ambient = OBJrSound_GetAmbient(inPlayingObject->object);
	if (ambient == NULL) { return; }
	
	// sound objects are always low priority
	ambient->priority = SScPriority2_Low;
	
	if (inPlayingObject->play_id != SScInvalidID)
	{
		UUtBool						is_playing;
		
		is_playing = SSrAmbient_Update(inPlayingObject->play_id, NULL, NULL, NULL, NULL);
		if (is_playing == UUcFalse)
		{
			inPlayingObject->play_id = SScInvalidID;
			return;
		}
		
		OBJrSound_GetMinMaxDistances(
			inPlayingObject->object,
			&max_volume_distance,
			&min_volume_distance);

		OBJrObject_GetPosition(inPlayingObject->object, &position, NULL);

		can_play =
			OSiCalc_PositionAndVolume(
				&position,
				max_volume_distance,
				min_volume_distance,
				ambient->min_occlusion,
				&position,
				&volume,
				UUcTrue,
				&inPlayingObject->occlusion_cache);

		if (can_play)
		{
			OSiPlayList_CalcSphereVolume(inPlayingObject, volume);
			SSrAmbient_SetVolume(inPlayingObject->play_id, inPlayingObject->volume, 0.0f);
		}
		
		return;
	}
	
	// play the ambient sound
	OBJrObject_GetPosition(inPlayingObject->object, &position, NULL);
	OBJrSound_GetMinMaxDistances(
		inPlayingObject->object,
		&max_volume_distance,
		&min_volume_distance);
	
	// calculate the position and volume of the sound
	can_play =
		OSiCalc_PositionAndVolume(
			&position,
			max_volume_distance,
			min_volume_distance,
			ambient->min_occlusion,
			&position,
			&volume,
			UUcTrue,
			&inPlayingObject->occlusion_cache);
	if (can_play)
	{
		OSiPlayList_CalcSphereVolume(inPlayingObject, volume);
		
		MUmVector_Set(velocity, 0.0f, 0.0f, 0.0f);
		
		// reuse the volume variable
		volume = 0.0f;
		
		inPlayingObject->play_id =
			SSrAmbient_Start(
				ambient,
				&position,
				inFacing,
				&velocity,
				max_volume_distance,
				min_volume_distance,
				&volume);
		
		SSrAmbient_SetVolume(inPlayingObject->play_id, inPlayingObject->volume, 0.25f);
	}
}

// ----------------------------------------------------------------------
#define OScPL_FadeInTimeSec					(2.0f)
#define OScPL_FadeOutTimeSec				(2.0f)

#define OScPL_FadeInTimeTicks				(OScPL_FadeInTimeSec * 60.0f)
#define OScPL_FadeOutTimeTicks				(OScPL_FadeOutTimeSec * 60.0f)

/*
	Function:
		y = 1 - (((x-n) * (x-n)) / (n * n))
		x = number of ticks that have passed since starting from a volume of zero
		n = num seconds to fade in over * 60 ticks per second
*/
static UUcInline float
OSiPL_CalcFadeInVol(
	float						inTicks)
{
	float						volume;
	float						x_minus_n;
	float						n_squared;
	
	x_minus_n = (inTicks - OScPL_FadeInTimeTicks);
	n_squared = OScPL_FadeInTimeTicks * OScPL_FadeInTimeTicks;
	volume = 1.0f - ((x_minus_n * x_minus_n) / n_squared);
	
	return volume;
}

static UUcInline float
OSiPL_CalcFadeOutVol(
	float						inTicks)
{
	float						volume;
	float						x_squared;
	float						n_squared;
	
	x_squared = (inTicks * inTicks);
	n_squared = (OScPL_FadeInTimeTicks * OScPL_FadeInTimeTicks);
	volume = 1.0f - (x_squared / n_squared);
	
	return volume;
}

static UUcInline UUtUns32
OSiPL_CalcFadeInStartTime(
	float						inVolume)
{
	UUtUns32					time;
	
	time = (UUtUns32)(OScPL_FadeInTimeTicks - (OScPL_FadeInTimeTicks * MUrSqrt(1 - inVolume)));
	
	return time;
}

static UUtBool
OSiPlayList_Transfer(
	OStPlayingObject			*inPlayingObject,
	SStAmbient					*inAmbient,
	UUtUns32					inTime)
{
	UUtUns32					i;
	OStPlayingObject			*playing_object_array;
	UUtUns32					num_elements;
	UUtBool						result;
	
	result = UUcFalse;
	
	// go through the ambient sound play list and update the playing sounds
	playing_object_array = (OStPlayingObject*)UUrMemory_Array_GetMemory(OSgPlayingObjects);
	num_elements = UUrMemory_Array_GetUsedElems(OSgPlayingObjects);
	for (i = 0; i < num_elements; i++)
	{
		OStPlayingObject			*playing_object;
		
		playing_object = &playing_object_array[i];
		
		if (OBJrSound_GetType(playing_object->object) != OBJcSoundType_BVolume)	{ continue; }
		if (playing_object->play_id == SScInvalidID) { continue; }
		if (OBJrSound_GetAmbient(playing_object->object) != inAmbient) { continue; }
		
		// transfer control of this ambient sound to inPlayingObject
		inPlayingObject->play_id = playing_object->play_id;
		inPlayingObject->state = OScPOState_Starting;
		inPlayingObject->volume = playing_object->volume;
		inPlayingObject->start_time = inTime - OSiPL_CalcFadeInStartTime(playing_object->volume);
		
		// release control from the playing ambient
		playing_object->play_id = SScInvalidID;
		playing_object->state = OScPOState_Off;
		playing_object->volume = 0.0f;
		
		result = UUcTrue;
		break;
	}
	
	return result;
}

static void
OSiPlayList_Update_BVolume(
	OStPlayingObject			*inPlayingObject,
	const M3tPoint3D			*inPosition,
	const M3tPoint3D			*inFacing)
{
	SStAmbient					*ambient;
	float						volume;
	float						ticks;
	UUtUns32					time;
	UUtBool						is_playing;
	
	time = UUrMachineTime_Sixtieths();
	ticks = (float)(time - inPlayingObject->start_time);
	
	if (OSgScriptOnly == UUcTrue) { return; }
	
	if (inPlayingObject->state == OScPOState_Off)
	{
		UUtBool						result;
		
		if (OBJrSound_PointIn(inPlayingObject->object, inPosition) == UUcFalse) { return; }
		
		ambient = OBJrSound_GetAmbient(inPlayingObject->object);
		if (ambient == NULL) { return; }
		
		result = OSiPlayList_Transfer(inPlayingObject, ambient, time);
		if (result == UUcTrue) { return; }
		
		// sound objects are always low priority
		ambient->priority = SScPriority2_Low;
		
		inPlayingObject->state = OScPOState_Starting;
		inPlayingObject->volume = 0.0f;
		
		inPlayingObject->play_id = SSrAmbient_Start_Simple(ambient, &inPlayingObject->volume);
		inPlayingObject->start_time = time;
		
		return;
	}
	
	// handle the starting and stopping states
	if (inPlayingObject->state == OScPOState_Starting)
	{
		if (ticks > OScPL_FadeInTimeTicks) { ticks = OScPL_FadeInTimeTicks; }
		inPlayingObject->volume = OSiPL_CalcFadeInVol(ticks);
		
		
		if (inPlayingObject->volume >= 1.0f)
		{
			inPlayingObject->volume = 1.0f;
			inPlayingObject->state = OScPOState_Playing;
		}
	}
	else if (inPlayingObject->state == OScPOState_Stopping)
	{
		if (ticks > OScPL_FadeOutTimeTicks) { ticks = OScPL_FadeOutTimeTicks; }
		inPlayingObject->volume = OSiPL_CalcFadeOutVol(ticks);
		
		if (inPlayingObject->volume <= 0.0f)
		{
			inPlayingObject->volume = 0.0f;
			inPlayingObject->state = OScPOState_Off;
			SSrAmbient_Halt(inPlayingObject->play_id);
			inPlayingObject->play_id = SScInvalidID;
		}
	}
	
	is_playing = SSrAmbient_Update(inPlayingObject->play_id, NULL, NULL, NULL, NULL);
	if (is_playing == UUcFalse)
	{
		inPlayingObject->volume = 0.0f;
		inPlayingObject->state = OScPOState_Off;
		inPlayingObject->play_id = SScInvalidID;
		return;
	}
	
	volume = inPlayingObject->volume * inPlayingObject->object_volume;
	SSrAmbient_SetVolume(inPlayingObject->play_id, volume, 0.0f);
	
	// handle general playing
	if (inPlayingObject->state != OScPOState_Off)
	{
		if (OBJrSound_PointIn(inPlayingObject->object, inPosition) == UUcFalse)
		{
			if (inPlayingObject->state != OScPOState_Stopping)
			{
				inPlayingObject->start_time = time;
			}
			
			// user has moved out of the sound volume so stop playing
			inPlayingObject->state = OScPOState_Stopping;
		}
		else if (inPlayingObject->state == OScPOState_Stopping)
		{
			// the user has moved back into the sound volume so go back to starting
			inPlayingObject->state = OScPOState_Starting;
		}
	}
}

// ----------------------------------------------------------------------
static UUtError
OSiPlayList_Reset(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	OSiPlayList_Terminate();
	OSiPlayList_Initialize();
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OSiPlayList_Update(
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inFacing)
{
	UUtUns32					i;
	OStPlayingObject			*playing_object_array;
	UUtUns32					num_elements;
	
	if (OSgPlayingObjects == NULL) { return; }
	
	// go through the ambient sound play list and update the playing sounds
	playing_object_array = (OStPlayingObject*)UUrMemory_Array_GetMemory(OSgPlayingObjects);
	num_elements = UUrMemory_Array_GetUsedElems(OSgPlayingObjects);
	for (i = 0; i < num_elements; i++)
	{
		switch (OBJrSound_GetType(playing_object_array[i].object))
		{
			case OBJcSoundType_Spheres:
				OSiPlayList_Update_Spheres(&playing_object_array[i], inPosition, inFacing);
			break;
			
			case OBJcSoundType_BVolume:
				OSiPlayList_Update_BVolume(
					&playing_object_array[i],
					inPosition,
					inFacing);
			break;
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OSrSubtitle_Draw(
	SStAmbient				*inAmbient,
	UUtUns32				inDuration)
{
	const char				*subtitle;
	ONtLetterBox			*letterbox;
	
	if (!ONrPersist_AreSubtitlesOn()) { return; }
	
	// get a pointer to the subtitle text
	subtitle = SSrAmbient_GetSubtitle(inAmbient);
	if (subtitle == NULL) { return; }

	letterbox = ONrGameState_GetLetterBox();
	if (ONrGameState_LetterBox_Active(letterbox)) {
		COrConsole_PrintIdentified(COcPriority_Subtitle, COgDefaultTextShade, COgDefaultTextShadowShade, subtitle, NULL, inDuration);
	} else {
		COrInGameSubtitle_Print(subtitle, NULL, inDuration);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
UUtError
OSrRegisterTemplates(
	void)
{
	UUtError	error;
	
	error =
		TMrTemplate_Register(
			OScTemplate_SoundBinaryData,
			sizeof(OStBinaryData),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiBinaryData_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void					*inPrivateData)
{
	UUtError				error;
	OStBinaryData			*binary_data;
	BFtFile					*separate_data_file;
	BDtData					*data= NULL;
	UUtBool					swap_it;
	char					name[BFcMaxFileNameLength];
	
#if (UUmEndian == UUmEndian_Big)
	swap_it = UUcTrue;
// uhh hey this is a NULL pointer... -S.S.	
//	UUrSwap_4Byte(&data->header.class_type);
//	UUrSwap_4Byte(&data->header.data_size);
#else
	swap_it = UUcFalse;
#endif

	binary_data = (OStBinaryData*)inInstancePtr;
	
	switch (inMessage)
	{
		case TMcTemplateProcMessage_LoadPostProcess:
			separate_data_file = TMrInstance_GetSeparateFile(inInstancePtr);
			if ((separate_data_file != NULL) && (binary_data->data_size > 0)) {
				// allocate new memory for the sound binary data
				data = (BDtData *) UUrMemory_Block_New(binary_data->data_size);
				UUmError_ReturnOnNull(data);

				// load the binary data from disk
				error = BFrFile_ReadPos(separate_data_file, binary_data->data_index, binary_data->data_size, data);
				UUmError_ReturnOnError(error);

				// S.S.
				if (swap_it)
				{
					UUrSwap_4Byte(&data->header.class_type);
					UUrSwap_4Byte(&data->header.data_size);
				}
				
				UUrString_Copy(
					name,
					TMrInstance_GetInstanceName(inInstancePtr),
					BFcMaxFileNameLength);
				UUrString_StripExtension(name);
				
				switch (data->header.class_type)
				{
					case OScAmbientDataClass:
						error = 
							OSiAmbient_Load(
								name,
								data,
								swap_it,
								UUcFalse,
								UUcTrue);
						UUmError_ReturnOnError(error);
					break;
					
					case OScGroupDataClass:
						error =
							OSiGroup_Load(
								name,
								data,
								swap_it,
								UUcFalse,
								UUcTrue);
						UUmError_ReturnOnError(error);
					break;
					
					case OScImpulseDataClass:
						error =
							OSiImpulse_Load(
								name,
								data,
								swap_it,
								UUcFalse,
								UUcTrue);
						UUmError_ReturnOnError(error);
					break;
				}
			}
		break;
	}
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OSrGetSoundBinaryDirectory(
	BFtFileRef				**outDirRef)
{
	UUtError				error;
	
	UUmAssert(outDirRef);
	
	*outDirRef = NULL;
	
	error = OSiBinFiles_GetDirectory(outDirRef, UUcTrue);
	UUmError_ReturnOnError(error);
	
	return error;
}

// ----------------------------------------------------------------------
UUtError
OSrGetSoundFileRef(
	const char				*inName,
	const char				*inSuffix,
	BFtFileRef				*inDirRef,
	BFtFileRef				**outFileRef)
{
	UUtError				error;
	BFtFileIterator			*file_iterator;
	
	UUmAssert(inName);
	UUmAssert(inSuffix);
	UUmAssert(inDirRef);
	UUmAssert(outFileRef);
	
	*outFileRef = NULL;
	
	// make a file iterator
	error =
		BFrDirectory_FileIterator_New(
			inDirRef,
			NULL,
			NULL,
			&file_iterator);
	UUmError_ReturnOnError(error);
	
	while (1)
	{
		BFtFileRef				ref;
		
		// get the next ref
		error = BFrDirectory_FileIterator_Next(file_iterator, &ref);
		if (error != UUcError_None) { break; }
		
		// process the ref
		if (BFrFileRef_IsDirectory(&ref) == UUcTrue)
		{
			error = OSrGetSoundFileRef(inName, inSuffix, &ref, outFileRef);
			
			UUmError_ReturnOnError(error);
			
			if (*outFileRef != NULL) { break; }
		}
		else
		{
			const char			*file_name;
			char				find_name[BFcMaxFileNameLength];
			
			file_name = BFrFileRef_GetLeafName(&ref);
			sprintf(find_name, "%s.%s", inName, inSuffix);
			if (UUrString_Compare_NoCase(file_name, find_name) == 0)
			{
				BFrFileRef_Duplicate(&ref, outFileRef);
				break;
			}
		}
	}
	
	// delete the file iterator
	BFrDirectory_FileIterator_Delete(file_iterator);
	file_iterator = NULL;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OSrInitialize(
	void)
{
	UUtError				error;
	
	// register the binary data classes
	error = OSiBinaryData_Register();
	UUmError_ReturnOnError(error);
	
	// get the binary directory for the sound files
#if TOOL_VERSION
	{
		BFtFileRef				*binary_dir;

		error = OSiBinFiles_GetDirectory(&binary_dir, UUcFalse);
		if (error == UUcError_None)
		{
			OSgBinaryDir = binary_dir;
		}
		else
		{
			OSgBinaryDir = NULL;
		}
	}
#endif
	
	// register the script commands
	error = OSiScriptCommands_Initialize();
	UUmError_ReturnOnError(error);
	
	// initialize the sound animations
	error = OSrSA_Initialize();
	UUmError_ReturnOnError(error);
	
	// intitialize the hash tables
	error = OSrAmbient_BuildHashTable();
	UUmError_ReturnOnError(error);
	
	error = OSrGroup_BuildHashTable();
	UUmError_ReturnOnError(error);
	
	error = OSrImpulse_BuildHashTable();
	UUmError_ReturnOnError(error);
	
	// initialize the playing ambient array
	error = OSiPlayingAmbient_Initialize();
	UUmError_ReturnOnError(error);
	
	// initialize the binary data
	error = OSrRegisterTemplates();
	UUmError_ReturnOnError(error);
	
	// install the proc handler
	error =
		TMrTemplate_PrivateData_New(
			OScTemplate_SoundBinaryData,
			0,
			OSiBinaryData_ProcHandler,
			&OSgBinaryData_PrivateData);
	UUmError_ReturnOnError(error);

	// initialize the globals
	OSgEnabled = UUcTrue;
	OSgScriptOnly = UUcFalse;
	OSgMusic_PlayID = SScInvalidID;
	
	// install pointer-update handlers into BFW_SoundSystem2
	SS2rInstallPointerHandlers(OSrUpdateImpulsePointers, OSrUpdateAmbientPointers);

	error = 
		SLrScript_Command_Register_Void(
			"sound_objects_reset",
			"reloads the sounds objects",
			"",
			OSiPlayList_Reset);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OSrLevel_Load(
	UUtUns32				inLevelNum)
{
	UUtError				error;
	
	// start the sound data file cache
	SSrSoundData_GetByName_StartCache();
	
	// enable the playing of sounds
	OSgEnabled = UUcTrue;
	
	// set the sound volume
	SS2rVolume_Set(ONrPersist_GetOverallVolume());
	
	if (inLevelNum == 0)
	{
		// initialize the variant list
		error = OSrVariantList_Initialize();
		UUmError_ReturnOnError(error);
		
		// build the hash tables out of the data loaded from the .dat file
		error = OSrAmbient_BuildHashTable();
		UUmError_ReturnOnError(error);
		
		error = OSrGroup_BuildHashTable();
		UUmError_ReturnOnError(error);
		
		error = OSrImpulse_BuildHashTable();
		UUmError_ReturnOnError(error);
		
#if TOOL_VERSION
		if (SSgSearchOnDisk || SSgSearchBinariesOnDisk)
		{
			if (OSgBinaryDir != NULL)
			{
				// build a sound cache to speed up finding these binary files
				OSgBinFiles_Load_Cache = BFrFileCache_New(OSgBinaryDir, BFcFileCache_Recursive);

				// load the binary data
				error = OSiBinFiles_Load(OSgBinaryDir, OScAmbientSuffix);
				UUmError_ReturnOnError(error);
				
				error = OSiBinFiles_Load(OSgBinaryDir, OScGroupSuffix);
				UUmError_ReturnOnError(error);
				
				error = OSiBinFiles_Load(OSgBinaryDir, OScImpulseSuffix);
				UUmError_ReturnOnError(error);

				BFrFileCache_Dispose(OSgBinFiles_Load_Cache);
				OSgBinFiles_Load_Cache = NULL;
			}
			
			// build the hash tables out of the data loaded from the .dat file
			// and the binary files
			error = OSrAmbient_BuildHashTable();
			UUmError_ReturnOnError(error);
			
			error = OSrGroup_BuildHashTable();
			UUmError_ReturnOnError(error);
			
			error = OSrImpulse_BuildHashTable();
			UUmError_ReturnOnError(error);
		}

//		OSiTest();
#endif		
		
		// update the pointers
		SSrGroup_UpdateSoundDataPointers();
		SSrAmbient_UpdateGroupPointers();
		SSrImpulse_UpdateGroupPointers();
	}
	else
	{
		// update pointers
		OSiAmbient_LevelLoad();
		OSiGroup_LevelLoad();
		OSiImpulse_LevelLoad();
		
		// initialize the playlist
		OSiPlayList_Initialize();
	}
	
	error = OSrVariantList_LevelLoad();
	UUmError_ReturnOnError(error);
	
	OSrUpdateSoundPointers();
	
	SSrSoundData_GetByName_StopCache();
	
	OSiFlybyProjectile_Clear();

	AKrCollision_InitializeSpatialCache(&OSgListener_OctreeCache);
	
	SSrLevel_Begin();
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrUpdateSoundPointers(
	void)
{
	// recalculate pointers in all systems that handle any sounds
	P3rRecalculateSoundPointers();
	ONrImpactEffects_SetupSoundPointers();
	OBJrDoor_UpdateSoundPointers();
	OBJrTurret_UpdateSoundPointers();
	OBJrTrigger_UpdateSoundPointers();
	ONrGameSettings_CalculateSoundPointers();
	WPrUpdateSoundPointers();
}

// ----------------------------------------------------------------------
static void
OSrUpdateImpulsePointers(
	SStImpulse *inImpulse,
	UUtBool inDeleted)
{
	// recalculate pointers in all systems that handle impulse sounds
	P3rRecalculateSoundPointers();
	ONrImpactEffects_SetupSoundPointers();
	OBJrDoor_UpdateSoundPointers();
	OBJrTrigger_UpdateSoundPointers();
	ONrGameSettings_CalculateSoundPointers();
	WPrUpdateSoundPointers();
}

// ----------------------------------------------------------------------
static void
OSrUpdateAmbientPointers(
	SStAmbient *inAmbient,
	UUtBool inDeleted)
{
	// recalculate pointers in all systems that handle ambient sounds
	P3rRecalculateSoundPointers();
	OBJrTurret_UpdateSoundPointers();
	OBJrTrigger_UpdateSoundPointers();
	ONrGameSettings_CalculateSoundPointers();
}

// ----------------------------------------------------------------------
void
OSrLevel_Unload(
	void)
{
	SSrStopAll();
	
	OSrVariantList_LevelUnload();

	OSiAmbient_LevelUnload();
	OSiGroup_LevelUnload();
	OSiImpulse_LevelUnload();
	
	OSiPlayList_Terminate();
}

// ----------------------------------------------------------------------
void
OSrMakeGoodName(
	const char				*inName,
	char					*outName)
{
	UUtUns32	itr;
	
	UUmAssert(inName);
	UUmAssert(outName);
	
	for(itr = 0;
		(itr < (SScMaxNameLength - 1)) && (*inName != 0);
		itr++)
	{
		char				letter;
		
		letter = *inName++;
		
		if (letter == ' ') { letter = '_'; }
		
		*outName++ = letter;
	}
	
	*outName = 0;
}

// ----------------------------------------------------------------------
void
OSrSetEnabled(
	UUtBool					inEnabled)
{
	UUtBool					prev;
	
	prev = OSgEnabled;
	
	OSgEnabled = inEnabled;
	
	if (OSgEnabled == UUcFalse)
	{
		if (prev == UUcTrue)
		{
			// turn off the volume
			OSgPrevVolume = SS2rVolume_Set(0.0f);
		}
	}
	else
	{
		if (prev == UUcFalse)
		{
			// restore the volume
			SS2rVolume_Set(OSgPrevVolume);
		}
	}
}

// ----------------------------------------------------------------------
void
OSrSetScriptOnly(
	UUtBool					inIsScriptOnly)
{
	OSgScriptOnly = inIsScriptOnly;
	SSrDetails_SetCanPlay(!OSgScriptOnly);
}

// ----------------------------------------------------------------------
void
OSrTerminate(
	void)
{
	SSrStopAll();

	OSiPlayingAmbient_Terminate();
	OSrVariantList_Terminate();
	OSiScriptCommands_Terminate();
	
	OSiAmbient_LevelUnload();
	OSiGroup_LevelUnload();
	OSiImpulse_LevelUnload();
	
#if TOOL_VERSION
	{
		if (OSgBinaryDir != NULL)
		{
			UUrMemory_Block_Delete(OSgBinaryDir);
			OSgBinaryDir = NULL;
		}
	}
#endif
	
	if (OSgPlayingAmbient != NULL)
	{
		UUrMemory_Array_Delete(OSgPlayingAmbient);
		OSgPlayingAmbient = NULL;
	}
}

// ----------------------------------------------------------------------
void
OSrUpdate(
	const M3tPoint3D			*inPosition,
	const M3tVector3D			*inFacing)
{
#if TOOL_VERSION
	OSgNumOcclusionLines = 0;
	OSgNumOcclusionPoints = 0;
#endif
	
	// set the listener's position
	SSrListener_SetPosition(inPosition, inFacing);
	OSgListener_Position = *inPosition;
	OSgListener_Facing = *inFacing;
	
	// update the playing ambient sounds
	OSiAmbient_UpdateAll();
	
	// update flyby sounds
	OSiFlybyProjectiles_Update();
	
	// update the sound object play list
	OSiPlayList_Update(inPosition, inFacing);
	
	// update the script commands.
	OSiScriptCommands_Update();
	
	// update the music
	OSiMusic_Update();
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
OSrFlybyProjectile_Create(
	P3tParticleClass		*inClass,
	P3tParticle				*inParticle)
{
	UUtBool dead;
	UUtUns32 itr;
	OStFlybyProjectile *projectile;

	// find a free entry in the projectile table
	for (itr = 0; itr < OSgNumFlybyProjectiles; itr++) {
		if ((OSgFlybyProjectile[itr].flags & OScFlybyProjectileFlag_InUse) == 0) {
			break;
		}
	}
	if (itr >= OScMaxFlybyProjectiles) {
		// we cannot create any more flyby projectiles
		return UUcFalse;
	}

	projectile = &OSgFlybyProjectile[itr];
	projectile->flags |= OScFlybyProjectileFlag_InUse;
	if (itr >= OSgNumFlybyProjectiles) {
		OSgNumFlybyProjectiles++;
		UUmAssert(OSgNumFlybyProjectiles <= OScMaxFlybyProjectiles);
	}

	// set up the projectile entry from the particle
	dead = P3rGetRealPosition(inClass, inParticle, &projectile->position);
	if (dead) {
		OSiFlybyProjectile_DeleteByIndex((UUtUns8) itr);
		return UUcFalse;
	}

	// calculate 

	projectile->prev_position = projectile->position;

	projectile->particle_class = inClass;
	projectile->particle = inParticle;
	projectile->particle_ref = inParticle->header.self_ref;

	// set up the link back to this projectile table entry from the particle
	UUmAssert(itr < UUcMaxUns8);
	inParticle->header.sound_flyby_index = (UUtUns8) itr;
	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
OSiFlybyProjectile_DeleteByIndex(
	UUtUns8					inIndex)
{
	UUmAssert((inIndex >= 0) && (inIndex < OScMaxFlybyProjectiles));
	UUmAssert(OSgFlybyProjectile[inIndex].flags & OScFlybyProjectileFlag_InUse);

	// delete this projectile from our table
	OSgFlybyProjectile[inIndex].flags &= ~OScFlybyProjectileFlag_InUse;
	if (((UUtUns32) inIndex + 1) == OSgNumFlybyProjectiles) {
		OSgNumFlybyProjectiles--;
	}
}

// ----------------------------------------------------------------------
static UUtBool
OSiFlybyProjectiles_UpdateProjectile(
	P3tParticleClass		*inClass,
	P3tParticle				*inParticle,
	OStFlybyProjectile		*ioProjectile)
{
	M3tVector3D dv, d0, d1, sound_point;
	float t0, t1;
	UUtBool dead;

	UUmAssert((ioProjectile >= OSgFlybyProjectile) && (ioProjectile < &OSgFlybyProjectile[OScMaxFlybyProjectiles]));

	// update particle's position
	ioProjectile->prev_position = ioProjectile->position;
	dead = P3rGetRealPosition(ioProjectile->particle_class, ioProjectile->particle, &ioProjectile->position);
	if (dead) {
		OSiFlybyProjectile_DeleteByIndex((UUtUns8) (ioProjectile - OSgFlybyProjectile));
		return UUcTrue;
	}

	// calculate the triangle formed by both points and the listener
	MUmVector_Subtract(dv, ioProjectile->position, ioProjectile->prev_position);
	MUmVector_Subtract(d0, ioProjectile->prev_position, OSgListener_Position);
	MUmVector_Subtract(d1, ioProjectile->position, OSgListener_Position);
	t0 = MUrVector_DotProduct(&d0, &dv);
	t1 = MUrVector_DotProduct(&d1, &dv);

	if (t0 * t1 < 0) {
		// the projectile has crossed its line of closest approach with the listener
		if (fabs(t1 - t0) > 1e-06f) {
			MUmVector_ScaleCopy(sound_point, t1 / (t1 - t0), ioProjectile->prev_position);
			MUmVector_ScaleIncrement(sound_point, -t0 / (t1 - t0), ioProjectile->position);
		} else {
			MUmVector_Add(sound_point, ioProjectile->prev_position, ioProjectile->position);
			MUmVector_Scale(sound_point, 0.5f);
		}

		// play the flyby sound here
		if (ioProjectile->particle_class->flyby_sound != NULL) {
			OSrImpulse_Play(ioProjectile->particle_class->flyby_sound, &sound_point, NULL, NULL, NULL);
		}
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtBool
OSrFlybyProjectile_Delete(
	P3tParticleClass		*inClass,
	P3tParticle				*inParticle)
{
	UUtBool dead;

	dead = OSiFlybyProjectiles_UpdateProjectile(inClass, inParticle, &OSgFlybyProjectile[inParticle->header.sound_flyby_index]);
	if (!dead) {
		OSiFlybyProjectile_DeleteByIndex(inParticle->header.sound_flyby_index);
	}

	inParticle->header.sound_flyby_index = (UUtUns8) -1;
	return UUcTrue;
}

// ----------------------------------------------------------------------
static void
OSiFlybyProjectile_Clear(
	void)
{
	OSgNumFlybyProjectiles = 0;
	UUrMemory_Clear(OSgFlybyProjectile, sizeof(OSgFlybyProjectile));
}


// ----------------------------------------------------------------------
static void
OSiFlybyProjectiles_Update(
	void)
{
	UUtUns32 itr;
	OStFlybyProjectile *projectile;
	
	if (OSgFlybyProjectile == NULL) { return; }

	for (itr = 0, projectile = OSgFlybyProjectile; itr < OSgNumFlybyProjectiles; itr++, projectile++) {
		if (projectile->flags & OScFlybyProjectileFlag_InUse) {
			// sanity check the particle still exists
			if (projectile->particle->header.self_ref != projectile->particle_ref) {
				UUmAssert(!"OSiFlybyProjectile_Update: flyby projectile has broken link");
				OSiFlybyProjectile_DeleteByIndex((UUtUns8) itr);
				continue;
			}

#if defined(DEBUGGING) && DEBUGGING
			{
				// sanity check the particle reference matches our cached pointers
				P3tParticleClass *classptr;
				P3tParticle *particleptr;

				P3mUnpackParticleReference(projectile->particle_ref, classptr, particleptr);
				UUmAssert(classptr == projectile->particle_class);
				UUmAssert(particleptr == projectile->particle);
			}
#endif

			OSiFlybyProjectiles_UpdateProjectile(projectile->particle_class, projectile->particle, projectile);
		}
	}
}
