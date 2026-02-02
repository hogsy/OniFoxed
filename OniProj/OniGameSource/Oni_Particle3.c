// ======================================================================
// Oni_Particle3.c
//
//   Oni's interface to the BFW_Particle3 particle system
//
//   Chris Butcher, February 17th 2000
//
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_MathLib.h"
#include "BFW_ScriptLang.h"
#include "BFW_EnvParticle.h"

#include "Oni.h"
#include "Oni_Character.h"
#include "Oni_Weapon.h"
#include "Oni_Particle3.h"
#include "Oni_ImpactEffect.h"
#include "Oni_Motoko.h"
#include "Oni_Persistance.h"
#include "Oni_Sound2.h"

// particle logging to file - superseded by OT_Particle
#define PARTICLE_RECORD			0

// ======================================================================
// constants
// ======================================================================

#define ONcGlass_NumDetailLevels		3
#define ONcGlass_MaxDetail				(ONcGlass_NumDetailLevels - 1)

static UUtUns32 ONgGlassDetailLevel = ONcGlass_MaxDetail;

const float ONcGlass_Cellsize[ONcGlass_NumDetailLevels]			= {22.0f, 20.0f, 15.0f};
const float ONcGlass_ShardsPerCell[ONcGlass_NumDetailLevels]	= {2.5f, 3.5f, 5.0f};

#define ONcGlass_RandomVelocity			10.f
#define ONcGlass_BlastDirectedVelocity  90.f
#define ONcGlass_BlastDirectedVelDmg    25.f
#define ONcGlass_BlastNormalVelocity	12.f
#define ONcGlass_BlastOffset			2.0f
#define ONcGlass_InterpRadiusScale		1.5f

typedef enum ONtParticleCommandType {
	ONcParticleCommandType_Create,
	ONcParticleCommandType_Reset,
	ONcParticleCommandType_Kill,
	ONcParticleCommandType_Do
} ONtParticleCommandType;

// ======================================================================
// internal structures
// ======================================================================

typedef struct ONtGlassData {
	M3tMatrix3x3 orientation;
} ONtGlassData;

typedef struct ONtParticleCommandData {
	ONtParticleCommandType	type;
	UUtUns32				num_params;
	UUtUns32				event_index;
	SLtParameter_Actual		*param;
} ONtParticleCommandData;

// ======================================================================
// internal globals
// ======================================================================

static UUtBool ONgParticleDebugCollision = UUcFalse;
static UUtBool ONrParticle3_GlassBreakable = UUcTrue;
// FIXME: eventually this should be replaced with some kind of impact material reference
static P3tEffectSpecification ONgParticle3_GlassShards;
static UUtUns32 ONgDaodanShieldDisableMask;

// ======================================================================
// external globals
// ======================================================================

P3tParticleClass *ONgParticle3_DaodanShield;
UUtBool ONgParticle3_Valid = UUcFalse;

// ======================================================================
// internal prototypes
// ======================================================================

// interface to BFW particle system
static void ONiParticle3_CreationCallback(P3tParticleClass *inClass, P3tParticle *inParticle);
static void ONiParticle3_DeletionCallback(P3tParticleClass *inClass, P3tParticle *inParticle);
static void ONiParticle3_UpdateCallback(P3tParticleClass *inClass, P3tParticle *inParticle);
static void ONiParticle3_SettingsCallback(P3tQualitySettings *outQualitySettings);

// emit a piece of breaking glass from a quad
static void ONiParticle3_EmitGlassPiece(P3tEffectSpecification *inGlassType, ONtGlassData *inGlassData,
										M3tPoint3D *inPoint, UUtUns32 inTime,
										 float inDamage, M3tVector3D *inBlastSource, M3tVector3D *inBlastNormal,
										 M3tVector3D *inBlastDir, float inBlastRadius);

static void ONiParticle3_DebugCollision_VariableChanged(void);

#if PARTICLE_RECORD
static void ONiParticle3_Record_Play(void);
FILE *particle_record = NULL;
#endif

static UUtError
ONiParticle3_Debug_SpawnParticle(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_Debug_KillAllParticles(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_Debug_KillNearestParticle(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_Debug_Count(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_Debug_CallEvent(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_Command(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_Temporary_Start(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_Temporary_Stop(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_Temporary_Kill(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_PrintTags(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_RemoveDangerous(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_DumpParticles(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

#if TOOL_VERSION
static UUtError
ONiParticle3_ListCollision(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);
#endif

static UUtError
ONiParticle3_StartAll(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_StopAll(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_DaodanShieldDisable(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

static UUtError
ONiParticle3_WriteUsedParticles(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);

#if PARTICLE_PERF_ENABLE
static UUtError
ONiParticle3_Perf(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue);
#endif

// ======================================================================
// initialization and destruction
// ======================================================================

UUtError ONrParticle3_Initialize(void)
{
	UUtError error;

	// set up interface to particle system
	P3rRegisterCallbacks(ONiParticle3_CreationCallback, ONiParticle3_DeletionCallback, NULL, ONiParticle3_SettingsCallback);

	// set up console commands
	error =
	SLrScript_Command_Register_Void(
		"p3_spawn",
		"Spawns a new P3 particle",
		"particle_class:string [velocity:float | ]",
		ONiParticle3_Debug_SpawnParticle);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"p3_killall",
		"Kills all P3 particles",
		"",
		ONiParticle3_Debug_KillAllParticles);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"p3_killnearest",
		"Kills the nearest P3 particle",
		"[no_recreate:bool | ]",
		ONiParticle3_Debug_KillNearestParticle);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"p3_count",
		"Lists a count of P3 particles",
		"",
		ONiParticle3_Debug_Count);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"p3_callevent",
		"Triggers an event on all P3 particles of a specified class",
		"particle_class:string event_index:int",
		ONiParticle3_Debug_CallEvent);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"particle_temp_start",
		"Starts temporary-particle-creation mode",
		NULL,
		ONiParticle3_Temporary_Start);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"particle_temp_stop",
		"Stops temporary-particle-creation mode",
		NULL,
		ONiParticle3_Temporary_Stop);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"particle_temp_kill",
		"Kills any temporary particles",
		NULL,
		ONiParticle3_Temporary_Kill);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"particle",
		"Sends a command to environmental particles with a given tag",
		NULL,
		ONiParticle3_Command);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"p3_printtags",
		"Prints out all environmental particles with tags",
		"",
		ONiParticle3_PrintTags);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"p3_removedangerous",
		"Removes all 'dangerous projectile' particles by making their lifetime expire",
		"",
		ONiParticle3_RemoveDangerous);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"p3_dumpparticles",
		"Dump all particles to a text file",
		"",
		ONiParticle3_DumpParticles);
	UUmError_ReturnOnError(error);

#if TOOL_VERSION
	error =
	SLrScript_Command_Register_Void(
		"p3_listcollision",
		"Dump all particle classes with collision to a text file",
		"",
		ONiParticle3_ListCollision);
	UUmError_ReturnOnError(error);
#endif

	error =
	SLrScript_Command_Register_Void(
		"p3_writeusedparticles",
		"Writes all particles used on this level to a text file",
		"",
		ONiParticle3_WriteUsedParticles);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"p3_startall",
		"Creates and starts all environmental particles",
		"",
		ONiParticle3_StartAll);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"p3_stopall",
		"Stops all environmental particles",
		"",
		ONiParticle3_StopAll);
	UUmError_ReturnOnError(error);

	error =
	SLrScript_Command_Register_Void(
		"p3_daodan_disable",
		"Disables parts of the daodan shield (for debugging)",
		NULL,
		ONiParticle3_DaodanShieldDisable);
	UUmError_ReturnOnError(error);

#if PARTICLE_PERF_ENABLE
	error =
	SLrScript_Command_Register_Void(
		"p3_perf",
		"Toggles particle performance display and sets event masks",
		NULL,
		ONiParticle3_Perf);
	UUmError_ReturnOnError(error);
#endif

	error =	SLrGlobalVariable_Register_Bool("p3_show_env_collision", "Draws particle / environment collisions.", &P3gDrawEnvCollisions);
	UUmError_ReturnOnError(error);

	error =	SLrGlobalVariable_Register_Bool("p3_glass_breakable", "Enables breakable glass.", &ONrParticle3_GlassBreakable);
	UUmError_ReturnOnError(error);

	error =	SLrGlobalVariable_Register_Bool("p3_everything_breakable", "Makes every quad breakable.", &P3gEverythingBreakable);
	UUmError_ReturnOnError(error);

	error =	SLrGlobalVariable_Register_Bool("p3_furniture_breakable", "Makes all furniture breakable.", &P3gFurnitureBreakable);
	UUmError_ReturnOnError(error);

	SLrGlobalVariable_Register(
		"p3_debug_collision",
		"Enables collision debugging display",
		SLcReadWrite_ReadWrite,
		SLcType_Int32,
		(SLtValue*)&ONgParticleDebugCollision,
		ONiParticle3_DebugCollision_VariableChanged);

	return UUcError_None;
}

void ONrParticle3_LevelZeroLoaded(void)
{
	// initialize the impact effect particles
	ONrImpactEffects_SetupParticlePointers();

	// set up the glass particle effect
	ONgParticle3_GlassShards.particle_class			= P3rGetParticleClass("glass_shard");
	ONgParticle3_GlassShards.collision_orientation	= P3cEnumCollisionOrientation_Unchanged;
	ONgParticle3_GlassShards.location_type			= P3cEffectLocationType_Default;

	ONgParticle3_DaodanShield = P3rGetParticleClass("daodanshield_e01");
}

UUtError ONrParticle3_LevelBegin(void)
{
#if PARTICLE_RECORD
	ONiParticle3_Record_Play();
#endif

	ONgGlassDetailLevel = UUmMin(ONcGlass_MaxDetail, ONrPersist_GetGraphicsQuality());
	ONgParticle3_Valid = UUcTrue;
	ONgDaodanShieldDisableMask = 0;

	return UUcError_None;
}

void ONrParticle3_LevelEnd(void)
{
#if PARTICLE_RECORD
	if (NULL != particle_record) {
		fclose(particle_record);
		particle_record = NULL;
	}
#endif

	ONgParticle3_Valid = UUcFalse;
}

// kill all particles and then recreate those that are needed
void ONrParticle3_KillAll(UUtBool inRecreate)
{
	P3rKillAll();

#if PARTICLE_RECORD
	ONiParticle3_Record_Kill();
#endif

	if (inRecreate) {
		WPrRecreateParticles();
		OBJrTurret_RecreateParticles();

		// start up environmental particles
		EPrLevelStart(ONrGameState_GetGameTime());
	}
}

static void ONiParticle3_CreationCallback(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	AI2tDodgeProjectile *projectile;
	UUtUns32 projectile_index;

	if ((inClass->definition->dodge_radius > 0) || (inClass->definition->alert_radius > 0)) {
		// set up an AI dodge context for this particle
		projectile = AI2rKnowledge_NewDodgeProjectile(&projectile_index);
		if (projectile != NULL) {
			// set up the link back to the AI knowledge table
			UUmAssert(projectile_index < UUcMaxUns8);
			inParticle->header.ai_projectile_index = (UUtUns8) projectile_index;

			AI2rKnowledge_SetupDodgeProjectile(projectile, inClass, inParticle);
		}
	}

	if (inClass->flyby_sound != NULL) {
		// set up a sound flyby context for this particle
		OSrFlybyProjectile_Create(inClass, inParticle);
	}
}

static void ONiParticle3_DeletionCallback(P3tParticleClass *inClass, P3tParticle *inParticle)
{
	if (inParticle->header.ai_projectile_index != (UUtUns8) -1) {
		AI2rKnowledge_DeleteDodgeProjectile(inParticle->header.ai_projectile_index);
	}

	if (inParticle->header.sound_flyby_index != (UUtUns8) -1) {
		OSrFlybyProjectile_Delete(inClass, inParticle);
	}
}

static void ONiParticle3_UpdateCallback(P3tParticleClass *inClass, P3tParticle *inParticle)
{
}

static void ONiParticle3_SettingsCallback(P3tQualitySettings *outQualitySettings)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();

	if (quality >= ONcGraphicsQuality_Default) {
		*outQualitySettings = P3cQuality_High;

	} else if (quality >= ONcGraphicsQuality_1) {
		*outQualitySettings = P3cQuality_Medium;

	} else {
		*outQualitySettings = P3cQuality_Low;
	}
}

void ONrParticle3_UpdateGraphicsQuality(void)
{
	P3tQualitySettings settings;

	ONiParticle3_SettingsCallback(&settings);
	P3rChangeDetail(settings);
}

// ======================================================================
// glass
// ======================================================================

// emit breaking glass from a quad
void ONrParticle3_EmitGlassPieces(AKtGQ_General *inQuad, float inDamage,
								  M3tVector3D *inBlastSource, M3tVector3D *inBlastNormal,
								  M3tVector3D *inBlastDir, float inBlastRadius)
{
	M3tQuad *quad;
	M3tPoint3D *pointarray, *points[4], emit_point;
	M3tVector3D orient_x, orient_y, orient_z, side_x0, side_x1, side_z, faux_blast_dir;
	UUtUns32 itr, current_time, cells_x, cells_z, x, z, num_shards;
	float length_x0, length_x1, length_z, pos_x, pos_z, shard_density, interp_val, exact_interp, cellsize, v_mag;
	ONtGlassData glassdata;

	if ((!ONrParticle3_GlassBreakable) || (ONgParticle3_GlassShards.particle_class == NULL))
		return;

	UUmAssertReadPtr(inQuad, sizeof(AKtGQ_General));

	// don't break already-broken quads
	if (inQuad->flags & AKcGQ_Flag_BrokenGlass)
		return;

	inQuad->flags |= AKcGQ_Flag_BrokenGlass;

	if (inBlastSource != NULL) {
		// don't emit particles stupidly fast
		inDamage = UUmMax(inDamage, 25.0f);

		UUmAssert(inBlastNormal != NULL);
		if (inBlastDir == NULL) {
			MUmVector_Copy(faux_blast_dir, *inBlastNormal);
			MUmVector_Negate(faux_blast_dir);
			inBlastDir = &faux_blast_dir;
		}
	}

	quad = &inQuad->m3Quad.vertexIndices;
	pointarray = ONrGameState_GetEnvironment()->pointArray->points;

	for (itr = 0; itr < 4; itr++) {
		points[itr] = &pointarray[quad->indices[itr]];
	}

	// get the side vectors of the quad
	MUmVector_Subtract(side_x0, *points[1], *points[0]);
	length_x0 = MUmVector_GetLength(side_x0);

	MUmVector_Subtract(side_x1, *points[2], *points[3]);
	length_x1 = MUmVector_GetLength(side_x1);

	MUmVector_Subtract(side_z, *points[3], *points[0]);
	length_z = MUmVector_GetLength(side_z);

	// compute the orientation for emitted particles
	if (fabs(length_x0) < 1e-03f) {
		// this is a degenerate quad
		return;
	}

	MUmVector_ScaleCopy(orient_x, 1.0f / length_x0, side_x0);
	MUrVector_CrossProduct(&side_z, &orient_x, &orient_y);
	v_mag = MUmVector_GetLength(orient_y);
	if (fabs(v_mag) < 1e-03f) {
		// this is an almost-collinear quad
		return;
	}
	MUmVector_Scale(orient_y, 1.0f / v_mag);

	MUrVector_CrossProduct(&orient_x, &orient_y, &orient_z);
	v_mag = MUmVector_GetLength(orient_z);
	if (fabs(v_mag) < 1e-03f) {
		// this should never happen
		return;
	}
	MUmVector_Scale(orient_z, 1.0f / v_mag);

	MUrMatrix3x3_FormOrthonormalBasis(&orient_x, &orient_y, &orient_z, &glassdata.orientation);

	current_time = ONrGameState_GetGameTime();

	// calculate how many cells to divide the quad up into
	cellsize = ONcGlass_Cellsize[ONgGlassDetailLevel];
	cells_x = (UUtUns32) (length_x0 / cellsize) + 1;
	cells_z = (UUtUns32) (length_z / cellsize) + 1;

	cells_x = UUmMin(cells_x, 50);
	cells_z = UUmMin(cells_z, 50);

	MUmVector_Scale(side_x0, 1.0f / cells_x);
	MUmVector_Scale(side_x1, 1.0f / cells_x);
	MUmVector_Scale(side_z, 1.0f / cells_z);

	// emit a number of pieces from each cell in the quad
	for (z = 0; z < cells_z; z++) {
		shard_density = ONcGlass_ShardsPerCell[ONgGlassDetailLevel];

		if (cells_z == 1) {
			interp_val = 0;
		} else {
			// triangular or trapezoidal quads may have different lengths in X
			// as Z changes from 0 to 1. because of this, we scale the number of
			// shards to emit per cell.
			if (length_x1 > length_x0) {
				// calculate density based on the _high_ end of the cell
				interp_val = ((float) z + 1) / cells_z;
			} else {
				// calculate density based on the _low_ end of the cell
				interp_val = ((float) z    ) / cells_z;
			}
			interp_val = ((float) z) / (cells_z - 1);
			shard_density *= ((1.0f - interp_val) * 1.0f + (interp_val) * (length_x1 / length_x0));
		}

		for (x = 0; x < cells_x; x++) {
			// use shard_density to calculate a whole number that will, on average, approximate this fractional density
			num_shards = MUrUnsignedSmallFloat_To_Uns_Round(shard_density);
			if (P3rRandom() < shard_density - num_shards) {
				num_shards++;
			}

			for (itr = 0; itr < num_shards; itr++) {
				pos_x = x + P3rRandom();
				pos_z = z + P3rRandom();

				exact_interp = pos_z / cells_z;

				MUmVector_Copy(emit_point, *points[0]);

				MUmVector_ScaleIncrement(emit_point, pos_z, side_z);
				MUmVector_ScaleIncrement(emit_point, pos_x * (1.0f - exact_interp), side_x0);
				MUmVector_ScaleIncrement(emit_point, pos_x * exact_interp,		  side_x1);

				ONiParticle3_EmitGlassPiece(&ONgParticle3_GlassShards, &glassdata,
											&emit_point, current_time,
											inDamage, inBlastSource, inBlastNormal,
											inBlastDir, inBlastRadius);
			}
		}
	}
}

// emit a piece of breaking glass from a quad
static void ONiParticle3_EmitGlassPiece(P3tEffectSpecification *inGlassType, ONtGlassData *inGlassData,
										M3tPoint3D *inPoint, UUtUns32 inTime,
										 float inDamage, M3tVector3D *inBlastSource, M3tVector3D *inBlastNormal,
										 M3tVector3D *inBlastDir, float inBlastRadius)
{
	float						velocity_mag, blast_distance, outwards_interp;
	M3tVector3D					blast_direction, velocity_dir;
	P3tParticle					*new_particle;
	P3tEffectData				effect_data;

	UUmAssertReadPtr(inGlassType,					sizeof(P3tEffectSpecification));
	UUmAssertReadPtr(inGlassType->particle_class,	sizeof(P3tParticleClass));

	// set up data for the particle effect
	UUrMemory_Clear(&effect_data, sizeof(effect_data));
	effect_data.time = inTime;
	effect_data.cause_type = P3cEffectCauseType_None;
	effect_data.parent_orientation = &inGlassData->orientation;
	MUmVector_Copy(effect_data.position, *inPoint);

	// give particles a small random velocity
	velocity_mag = ONcGlass_RandomVelocity * P3rRandom();
	P3rGetRandomUnitVector3D(&effect_data.velocity);
	MUmVector_Scale(effect_data.velocity, velocity_mag);

	if (inBlastSource != NULL) {
		// work out the direction to the blast center from this glass shard
		MUmVector_Subtract(blast_direction, effect_data.position, *inBlastSource);
		blast_distance = MUmVector_GetLength(blast_direction);
		MUmVector_ScaleIncrement(blast_direction, ONcGlass_BlastOffset * inBlastRadius, *inBlastDir);
		MUmVector_Normalize(blast_direction);

		// work out an interpolation value between the center and edge of the blast
		outwards_interp = blast_distance / (inBlastRadius * ONcGlass_InterpRadiusScale);
		if (outwards_interp > 1.0f)
			outwards_interp = 1.0f;

		// interpolate the velocity
		velocity_mag = ONcGlass_BlastNormalVelocity * outwards_interp
			+ ONcGlass_BlastDirectedVelocity * (inDamage / ONcGlass_BlastDirectedVelDmg) * (1.0f - outwards_interp);

		// interpolate the direction
		MUmVector_ScaleCopy(velocity_dir, 1.0f - outwards_interp, blast_direction);
		MUmVector_ScaleIncrement(velocity_dir, -outwards_interp, *inBlastNormal);

		// set up the particle's velocity
		MUmVector_ScaleIncrement(effect_data.velocity, velocity_mag, velocity_dir);
	}

	// create the glass shard
	new_particle = P3rCreateEffect(inGlassType, &effect_data);
	if (new_particle == NULL) {
		COrConsole_Printf("### glass emitting %s: unable to spawn particle", inGlassType->particle_class->classname);
	}
}

// ======================================================================
// special-case particle code
// ======================================================================

#define CHECK_PART_FACE(flag, axis, pt, u_axis, v_axis) {		\
	if (flags & ONcBodySurfaceFlag_##flag) {					\
		if (stop_area < body_part->face_area.axis) {			\
			outPoint->axis = body_part->bbox.pt.axis;			\
			outPoint->u_axis = (1.0f - u) * body_part->bbox.minPoint.u_axis + u * body_part->bbox.maxPoint.u_axis;	\
			outPoint->v_axis = (1.0f - v) * body_part->bbox.minPoint.v_axis + v * body_part->bbox.maxPoint.v_axis;	\
			break;												\
		}														\
		stop_area -= body_part->face_area.axis;					\
	}															\
}

// generate an emitted position on the surface of a character
UUtBool ONrParticle3_FindBodySurfacePosition(UUtUns32 inCharacterOwner, float inRadius,
											M3tPoint3D *outPoint, M3tMatrix4x3 **outMatrix, M3tMatrix3x3 *outOrientation)
{
	ONtCharacter *character = WPrOwner_GetCharacter(inCharacterOwner);
	ONtCharacterClass *character_class;
	ONtActiveCharacter *active_character;
	UUtUns32 itr, flags;
	UUtBool project_3d;
	ONtBodySurfacePart *body_part;
	M3tVector3D outward_vector, right_vector, up_vector;
	float y_rad, z_rad, cap_size, u, v, stop_area;
	float pos_cap, neg_cap, x_mid, project_x, x_upvector;

	if (character == NULL) {
		return UUcFalse;
	}

	active_character = ONrGetActiveCharacter(character);
	if (active_character == NULL) {
		return UUcFalse;
	}

	character_class = character->characterClass;
	ONrCharacterClass_MakeBodySurfaceCache(character_class);
	if (character_class->body_surface == NULL) {
		// body surface cache initialization failed
		return UUcFalse;
	}

	stop_area = P3rRandom() * character_class->body_surface->total_surface_area;
	stop_area = UUmPin(stop_area, 0, character_class->body_surface->total_surface_area - 0.01f);
	u = P3rRandom();
	v = P3rRandom();

	for (itr = 0, body_part = character_class->body_surface->parts; itr < character_class->body_surface->num_parts; itr++, body_part++) {
		if (stop_area < body_part->surface_area) {
			if (ONgDaodanShieldDisableMask & (1 << itr)) {
				// this part of the daodan shield is disabled, don't generate here
				return UUcFalse;
			}

			// the point lies on this body part
			*outMatrix = active_character->matricies + itr;

			// determine the face that the point lies on
			flags = ONcBodySurfacePartMasks[itr];
			while (1) {
				CHECK_PART_FACE(Neg_X, x, minPoint, y, z);
				CHECK_PART_FACE(Pos_X, x, maxPoint, y, z);
				CHECK_PART_FACE(Neg_Y, y, minPoint, z, x);
				CHECK_PART_FACE(Pos_Y, y, maxPoint, z, x);
				CHECK_PART_FACE(Neg_Z, z, minPoint, x, y);
				CHECK_PART_FACE(Pos_Z, z, maxPoint, x, y);

				// should never reach here
				UUmAssert(0);
				MUmVector_Set(*outPoint, 0, 0, 0);
				MUrMatrix3x3_Identity(outOrientation);
				return UUcTrue;
			}

			// the orientation-building code only works for body parts oriented along X
			UUmAssert((flags & ONcBodySurfaceFlag_MainAxisMask) == ONcBodySurfaceFlag_MainAxis_X);

			project_3d = UUcFalse;

			if (flags & (ONcBodySurfaceFlag_Neg_X | ONcBodySurfaceFlag_Pos_X)) {
				// calculate the cap size and the locations of the cap for the positive and negative ends
				y_rad = (body_part->bbox.maxPoint.y - body_part->bbox.minPoint.y) / 2;
				z_rad = (body_part->bbox.maxPoint.z - body_part->bbox.minPoint.z) / 2;
				cap_size = UUmMin(y_rad, z_rad);

				pos_cap = body_part->bbox.maxPoint.x - cap_size;
				neg_cap = body_part->bbox.minPoint.x + cap_size;
				x_mid = (body_part->bbox.minPoint.x + body_part->bbox.maxPoint.x) / 2;

				pos_cap = UUmMax(pos_cap, x_mid);
				neg_cap = UUmMin(neg_cap, x_mid);

				// work out where to project the orientation in to
				if ((flags & ONcBodySurfaceFlag_Neg_X) && (outPoint->x < neg_cap)) {
					project_3d = UUcTrue;
					project_x = neg_cap;
				} else if ((flags & ONcBodySurfaceFlag_Pos_X) && (outPoint->x > pos_cap)) {
					project_3d = UUcTrue;
					project_x = pos_cap;
				}
			}

			if (project_3d) {
				// calculate the orientation outwards
				outward_vector.x = outPoint->x - project_x;
				outward_vector.y = outPoint->y - (body_part->bbox.minPoint.y + body_part->bbox.maxPoint.y) / 2;
				outward_vector.z = outPoint->z - (body_part->bbox.minPoint.z + body_part->bbox.maxPoint.z) / 2;
				MUmVector_Normalize(outward_vector);

				x_upvector = (project_x > x_mid) ? +1.0f : -1.0f;

				// our up-vector is +X
				right_vector.x = 0;
				right_vector.y = -x_upvector * outward_vector.z;
				right_vector.z =  x_upvector * outward_vector.y;
				if (MUrVector_Normalize_ZeroTest(&right_vector)) {
					// we are at the precise top of the cap
					MUmVector_Set(right_vector, 0, 0, x_upvector);
				}

				MUrVector_CrossProduct(&outward_vector, &right_vector, &up_vector);
				if (MUrVector_Normalize_ZeroTest(&up_vector)) {
					// there is a problem building the orientation!
					MUmVector_Set(up_vector, 0, 1, 0);
				}

				MUrMatrix_SetXAxis((M3tMatrix4x3 *) outOrientation, &up_vector);
				MUrMatrix_SetYAxis((M3tMatrix4x3 *) outOrientation, &outward_vector);
				MUrMatrix_SetZAxis((M3tMatrix4x3 *) outOrientation, &right_vector);

				MUmVector_ScaleIncrement(*outPoint, inRadius, outward_vector);
			} else {
				// calculate the orientation outwards and in the two perpendicular directions
				outward_vector.x = 0;
				outward_vector.y = outPoint->y - (body_part->bbox.minPoint.y + body_part->bbox.maxPoint.y) / 2;
				outward_vector.z = outPoint->z - (body_part->bbox.minPoint.z + body_part->bbox.maxPoint.z) / 2;
				MUmVector_Normalize(outward_vector);

				outOrientation->m[0][0] = 1.0f;
				outOrientation->m[0][1] = 0.0f;
				outOrientation->m[0][2] = 0.0f;
				outOrientation->m[1][0] = 0.0f;
				outOrientation->m[1][1] = outward_vector.y;
				outOrientation->m[1][2] = outward_vector.z;
				outOrientation->m[2][0] = 0.0f;
				outOrientation->m[2][1] = -outward_vector.z;
				outOrientation->m[2][2] =  outward_vector.y;

				outPoint->y += inRadius * outward_vector.y;
				outPoint->z += inRadius * outward_vector.z;
			}

			return UUcTrue;
		}

		stop_area -= body_part->surface_area;
	}

	// should never reach here
	UUmAssert(0);
	MUmVector_Set(*outPoint, 0, 0, 0);
	MUrMatrix3x3_Identity(outOrientation);
	*outMatrix = active_character->matricies + 0;
	return UUcTrue;
}

UUtBool ONrParticle3_FindBodyBonePosition(UUtUns32 inCharacterOwner, float inRadius, M3tPoint3D *outPoint, M3tMatrix4x3 **outMatrix)
{
	ONtCharacter *character = WPrOwner_GetCharacter(inCharacterOwner);
	ONtCharacterClass *character_class;
	ONtActiveCharacter *active_character;
	UUtUns32 itr, flags;
	ONtBodySurfacePart *body_part;
	float u, stop_length, offset_rad, y_rad, z_rad, cap_size;
	float pos_cap, neg_cap, x_min, x_max, x_mid, theta;

	if (character == NULL) {
		return UUcFalse;
	}

	active_character = ONrGetActiveCharacter(character);
	if (active_character == NULL) {
		return UUcFalse;
	}

	character_class = character->characterClass;
	ONrCharacterClass_MakeBodySurfaceCache(character_class);
	if (character_class->body_surface == NULL) {
		// body surface cache initialization failed
		return UUcFalse;
	}

	stop_length = P3rRandom() * character_class->body_surface->total_length;
	stop_length = UUmPin(stop_length, 0, character_class->body_surface->total_length - 0.01f);

	for (itr = 0, body_part = character_class->body_surface->parts; itr < character_class->body_surface->num_parts; itr++, body_part++) {
		if (stop_length < body_part->length) {
			// the point lies on this body part
			flags = ONcBodySurfacePartMasks[itr];
			*outMatrix = active_character->matricies + itr;

			// this code only works for body parts oriented along X
			UUmAssert((flags & ONcBodySurfaceFlag_MainAxisMask) == ONcBodySurfaceFlag_MainAxis_X);

			if (inRadius > 0) {
				// calculate the cap size and the locations of the cap for the positive and negative ends
				y_rad = (body_part->bbox.maxPoint.y - body_part->bbox.minPoint.y) / 2;
				z_rad = (body_part->bbox.maxPoint.z - body_part->bbox.minPoint.z) / 2;
				cap_size = UUmMin(y_rad, z_rad);

				pos_cap = body_part->bbox.maxPoint.x - cap_size;
				neg_cap = body_part->bbox.minPoint.x + cap_size;
				x_mid = (body_part->bbox.minPoint.x + body_part->bbox.maxPoint.x) / 2;

				x_min = pos_cap + inRadius;
				x_max = neg_cap - inRadius;

			} else {
				x_min = body_part->bbox.minPoint.x;
				x_max = body_part->bbox.maxPoint.x;
			}

			// work out the X coordinate of the point
			u = P3rRandom();
			outPoint->x = (1.0f - u) * x_min + u * x_max;
			outPoint->y = 0;
			outPoint->z = 0;

			if (inRadius > 0) {
				// calculate the offset radius
				offset_rad = inRadius;
				if ((flags & ONcBodySurfaceFlag_Neg_X) && (outPoint->x < x_min + inRadius)) {
					// our radius is decreased by the fact that we're part of the negative cap
					offset_rad = MUrSqrt(UUmSQR(inRadius) - UUmSQR(outPoint->x - x_min));

				} else if ((flags & ONcBodySurfaceFlag_Pos_X) && (outPoint->x > x_max - inRadius)) {
					// our radius is decreased by the fact that we're part of the negative cap
					offset_rad = MUrSqrt(UUmSQR(inRadius) - UUmSQR(x_max - outPoint->x));
				}

				// offset randomly in the YZ plane by this amount
				theta = P3rRandom() * M3c2Pi;
				outPoint->y = MUrCos(theta) * inRadius;
				outPoint->z = MUrSin(theta) * inRadius;
			}

			return UUcTrue;
		}

		stop_length -= body_part->length;
	}

	// should never reach here
	UUmAssert(0);
	MUmVector_Set(*outPoint, 0, 0, 0);
	*outMatrix = active_character->matricies + 0;
	return UUcTrue;
}


// ======================================================================
// debugging routines
// ======================================================================

#if PARTICLE_RECORD
static char *ONiParticle3_GetFileName(void)
{
	static char buffer[1024];

	sprintf(buffer, "%s particles.txt", ONgLevel->name);

	return buffer;
}



static void ONiParticle3_Record_CreateParticle(char *inParticleClassString, M3tPoint3D *inLocation, M3tVector3D *inVector, float inVelocity)
{
	if (NULL == particle_record) {
		particle_record = fopen(ONiParticle3_GetFileName(), "a+");
	}

	if (NULL != particle_record) {
		fprintf(particle_record, "%s\n", inParticleClassString);
		fprintf(particle_record, "%f\n", inLocation->x);
		fprintf(particle_record, "%f\n", inLocation->y);
		fprintf(particle_record, "%f\n", inLocation->z);
		fprintf(particle_record, "%f\n", inVector->x);
		fprintf(particle_record, "%f\n", inVector->y);
		fprintf(particle_record, "%f\n", inVector->z);
		fprintf(particle_record, "%f\n", inVelocity);
	}

	return;
}
#endif

static UUtError ONiParticle3_CreateParticle(char *inParticleClassString, M3tPoint3D *inLocation, M3tVector3D *inVector, float inVelocity)
{
	P3tParticleClass *particle_class;
	P3tParticle *new_particle;
	M3tPoint3D *p_position;
	M3tVector3D particle_offset, cross_vec, global_up, particle_dir, particle_up, *p_velocity, *p_offset;
	M3tMatrix3x3 *p_orientation;
	M3tMatrix4x3 **p_dynamicmatrix;
	P3tPrevPosition *p_prev_position;
	P3tOwner *p_owner;
	AKtOctNodeIndexCache *p_envcache;
	UUtUns32 *p_texture, *p_lensframes, current_time;
	UUtBool is_dead;

	particle_class = P3rGetParticleClass(inParticleClassString);
	if (particle_class == NULL) {
		COrConsole_Printf("### unknown particle class '%s'", inParticleClassString);
		return UUcError_None;
	}

	current_time = ONrGameState_GetGameTime();
	new_particle = P3rCreateParticle(particle_class, current_time);
	if (new_particle == NULL) {
		return UUcError_None;
	}

	MUmVector_Copy(particle_dir, *inVector);
	MUmVector_Normalize(particle_dir);

	p_position = P3rGetPositionPtr(particle_class, new_particle, &is_dead);
	if (is_dead) {
		COrConsole_Printf("### particle dies immediately after emission because nothing to lock position to!");
		return UUcError_None;
	}

	MUmVector_Copy(particle_offset, particle_dir);
	MUmVector_Scale(particle_offset, 10.0f);			// particle starts 1 metre away
	MUmVector_Add(*p_position, particle_offset, *inLocation);

	p_orientation = P3rGetOrientationPtr(particle_class, new_particle);
	if (p_orientation != NULL) {
		// form a 3x3 orientation matrix for the new particle.
		// particle's velocity is +Y, and they orient their +Z towards the global upvector (which is worldspace +Y)
		global_up.x = 0; global_up.y = +1; global_up.z = 0;
		MUrVector_CrossProduct(&particle_dir, &global_up, &cross_vec);
		MUmVector_Normalize(cross_vec);
		MUrVector_CrossProduct(&cross_vec, &particle_dir, &particle_up);
		MUmVector_Normalize(particle_up);

		MUrMatrix3x3_FormOrthonormalBasis(&cross_vec, &particle_dir, &particle_up, p_orientation);
	}

	p_velocity = P3rGetVelocityPtr(particle_class, new_particle);
	if (p_velocity != NULL) {
		*p_velocity = particle_dir;
		MUmVector_Scale(*p_velocity, inVelocity); // initial velocity 10 ms-1
	}

	p_offset = P3rGetOffsetPosPtr(particle_class, new_particle);
	if (p_offset != NULL) {
		MUmVector_Set(*p_offset, 0, 0, 0);
	}

	p_dynamicmatrix = P3rGetDynamicMatrixPtr(particle_class, new_particle);
	if (p_dynamicmatrix != NULL) {
		*p_dynamicmatrix = NULL;
	}

	p_owner = P3rGetOwnerPtr(particle_class, new_particle);
	if (p_owner != NULL) {
		*p_owner = 0;
	}

	// randomise texture start index
	p_texture = P3rGetTextureIndexPtr(particle_class, new_particle);
	if (p_texture != NULL) {
		*p_texture = (UUtUns32) UUrLocalRandom();
	}

	// set texture time index to be now
	p_texture = P3rGetTextureTimePtr(particle_class, new_particle);
	if (p_texture != NULL) {
		*p_texture = current_time;
	}

	// no previous position
	p_prev_position = P3rGetPrevPositionPtr(particle_class, new_particle);
	if (p_prev_position != NULL) {
		p_prev_position->time = 0;
	}

	// lensflares are not initially visible
	p_lensframes = P3rGetLensFramesPtr(particle_class, new_particle);
	if (p_lensframes != NULL) {
		*p_lensframes = 0;
	}

	// set up the environment cache
	p_envcache = P3rGetEnvironmentCache(particle_class, new_particle);
	if (p_envcache != NULL) {
		AKrCollision_InitializeSpatialCache(p_envcache);
	}

	P3rSendEvent(particle_class, new_particle, P3cEvent_Create, ((float) current_time) / UUcFramesPerSecond);

	return UUcError_None;
}

#if PARTICLE_RECORD
static void ONiParticle3_Record_Kill(void)
{
	if (NULL != particle_record) {
		fclose(particle_record);
		particle_record = NULL;
	}

	if (NULL == particle_record) {
		particle_record = fopen(ONiParticle3_GetFileName(), "w");
	}

	if (NULL != particle_record) {
		fclose(particle_record);
		particle_record = NULL;
	}
}

static void ONiParticle3_Record_Play(void)
{
	if (NULL != particle_record) {
		fclose(particle_record);
		particle_record = NULL;
	}

	if (NULL == particle_record) {
		particle_record = fopen(ONiParticle3_GetFileName(), "r");
	}

	if (NULL != particle_record) {
		char name[1024];
		char number[1024];
		char *string;

		M3tPoint3D location;
		M3tPoint3D vector;
		float velocity;

		while(string = fgets(name, 1024, particle_record))
		{
			strtok(name, "\n\r");

			string = fgets(number, 1024, particle_record);
			if (NULL == string) { break; }
			sscanf(number, "%f", &location.x);

			string = fgets(number, 1024, particle_record);
			if (NULL == string) { break; }
			sscanf(number, "%f", &location.y);

			string = fgets(number, 1024, particle_record);
			if (NULL == string) { break; }
			sscanf(number, "%f", &location.z);

			string = fgets(number, 1024, particle_record);
			if (NULL == string) { break; }
			sscanf(number, "%f", &vector.x);

			string = fgets(number, 1024, particle_record);
			if (NULL == string) { break; }
			sscanf(number, "%f", &vector.y);

			string = fgets(number, 1024, particle_record);
			if (NULL == string) { break; }
			sscanf(number, "%f", &vector.z);

			string = fgets(number, 1024, particle_record);
			if (NULL == string) { break; }
			sscanf(number, "%f", &velocity);

			ONiParticle3_CreateParticle(name, &location, &vector, velocity);
		}
	}

	if (NULL != particle_record) {
		fclose(particle_record);
		particle_record = NULL;
	}

	return;
}
#endif

static UUtError
ONiParticle3_Temporary_Start(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	P3rSetTemporaryCreation(UUcTrue);

	return UUcError_None;
}

static UUtError
ONiParticle3_Temporary_Stop(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	P3rSetTemporaryCreation(UUcFalse);

	return UUcError_None;
}

static UUtError
ONiParticle3_Temporary_Kill(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	P3rKillTemporaryParticles();

	return UUcError_None;
}

static UUtError
ONiParticle3_Debug_SpawnParticle(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter *player_character;
	ONtActiveCharacter *active_character;
	UUtError error;
	float velocity;

	if (inParameterListLength < 1) {
		COrConsole_Printf("usage: p3_spawn <particle class name> [particle velocity]");
		return UUcError_Generic;
	}

	player_character = ONrGameState_GetPlayerCharacter();
	active_character = ONrGetActiveCharacter(player_character);
	if (active_character == NULL) {
		COrConsole_Printf("### player character is not active!");
		return UUcError_Generic;
	}

	if (1 == inParameterListLength) {
		velocity = 0.f;
	}
	else if (inParameterListLength >= 2) {
		if (SLcType_Int32 == inParameterList[1].type) {
			velocity = (float) inParameterList[1].val.i;
		}
		else if (SLcType_Float == inParameterList[1].type) {
			velocity = inParameterList[1].val.f;
		}
		else {
			velocity = 0.f;
		}
	}
	else {
		UUmAssert(!"invalid case");
	}


#if PARTICLE_RECORD
	ONiParticle3_Record_CreateParticle(inParameterList[0].val.str, &player_character->location, &active_character->aimingVector, velocity);
#endif
	error = ONiParticle3_CreateParticle(inParameterList[0].val.str, &player_character->location, &active_character->aimingVector, velocity);

	return error;
}

static UUtError
ONiParticle3_Debug_KillNearestParticle(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtCharacter *player_character;
	M3tPoint3D particle_dist, *positionptr;
	float d_sq, closest_d_sq;
	P3tParticleClass *closest_class, *this_class;
	P3tParticle *closest_particle, *particle;
	UUtUns16 itr;
	UUtBool is_dead;

	UUmAssert(inParameterListLength == 0);

	player_character = ONrGameState_GetPlayerCharacter();

	// no particles found yet
	closest_d_sq = 1e+09;
	closest_class = NULL;
	closest_particle = NULL;

	// iterate over all classes to find the closest particle
	for (itr = 0; itr < P3gNumClasses; itr++) {
		this_class = &P3gClassTable[itr];
		for (particle = this_class->headptr; particle != NULL; particle = particle->header.nextptr) {
			positionptr = P3rGetPositionPtr(this_class, particle, &is_dead);
			if (is_dead) {
				continue;
			}

			MUmVector_Subtract(particle_dist, *positionptr, player_character->location);
			d_sq = MUmVector_GetLengthSquared(particle_dist);

			if (d_sq < closest_d_sq) {
				closest_d_sq = d_sq;
				closest_class = this_class;
				closest_particle = particle;
			}
		}
	}

	if (closest_particle != NULL) {
		P3rKillParticle(closest_class, closest_particle);
	}

	return UUcError_None;
}

static UUtError
ONiParticle3_Debug_KillAllParticles(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtBool recreate = UUcTrue;

	if (inParameterListLength >= 1)
		recreate = !inParameterList[0].val.b;

	ONrParticle3_KillAll(recreate);

	return UUcError_None;
}

static UUtError
ONiParticle3_Debug_Count(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16 itr;
	UUtUns32 total_particles;
	P3tParticleClass *this_class;

	total_particles = 0;
	for (itr = 0, this_class = P3gClassTable; itr < P3gNumClasses; itr++, this_class++) {
		if (this_class->num_particles > 0) {
			COrConsole_Printf("%s: %d", this_class->classname, this_class->num_particles);
			total_particles += this_class->num_particles;
		}
	}
	COrConsole_Printf("total particles: %d", total_particles);

	return UUcError_None;
}

static UUtError
ONiParticle3_Debug_CallEvent(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	P3tParticleClass *this_class;
	P3tParticle *particle, *nextptr;
	UUtUns16 event;

	this_class = P3rGetParticleClass(inParameterList[0].val.str);
	if (this_class == NULL) {
		COrConsole_Printf("### unknown particle class '%s'", inParameterList[0].val.str);
		return UUcError_Generic;
	}

	event = (UUtUns16) inParameterList[1].val.i;
	if ((event < 0) || (event >= P3cEvent_NumEvents)) {
		COrConsole_Printf("### event %d outside allowable range [0-%d)", event, P3cEvent_NumEvents);
		return UUcError_Generic;
	}

	for (particle = this_class->headptr; particle != NULL; particle = nextptr) {
		nextptr = particle->header.nextptr;

		P3rSendEvent(this_class, particle, event, ((float) ONrGameState_GetGameTime()) / UUcFramesPerSecond);
	}

	return UUcError_None;
}

static void
ONiParticle3_CommandCallback(
	EPtEnvParticle *			inParticle,
	UUtUns32					inUserData)
{
	ONtParticleCommandData *command_data = (ONtParticleCommandData *) inUserData;
	P3tParticle *particle;
	UUtBool kill_particle;
	float time;
	UUtUns32 tick_num = ONrGameState_GetGameTime();

	UUmAssertReadPtr(inParticle, sizeof(EPtEnvParticle));
	UUmAssertReadPtr(command_data, sizeof(ONtParticleCommandData));

	switch(command_data->type) {
	case ONcParticleCommandType_Create:
		EPrNewParticle(inParticle, tick_num);
		break;

	case ONcParticleCommandType_Kill:
		EPrKillParticle(inParticle);
		break;

	case ONcParticleCommandType_Reset:
		EPrResetParticle(inParticle, tick_num);
		break;

	case ONcParticleCommandType_Do:
		particle = inParticle->particle;
		if (particle == NULL)
			break;

		UUmAssertReadPtr(particle, sizeof(P3tParticle));
		if (particle->header.self_ref != inParticle->particle_self_ref)
			break;

		time = ((float) tick_num) / UUcFramesPerSecond;
		kill_particle = P3rSendEvent(inParticle->particle_class, particle, (UUtUns16) command_data->event_index, time);
		if (kill_particle) {
			P3rKillParticle(inParticle->particle_class, particle);
			inParticle->particle = NULL;
			inParticle->particle_self_ref = P3cParticleReference_Null;
		}
		break;

	default:
		UUmAssert(!"ONiParticle3_CommandCallback: unknown command type");
	}
}

static UUtError
ONiParticle3_Command(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	ONtParticleCommandData command_data;
	UUtUns32 itr;

	if ((inParameterListLength < 2) || (inParameterList[1].type != SLcType_String)) {
		COrConsole_Printf("syntax: particle <tag> <command> [params...]");
		COrConsole_Printf("  commands: create kill reset");
		return UUcError_Generic;
	}

	if (inParameterList[0].type != SLcType_String) {
		COrConsole_Printf("syntax: particle <tag> <command> [params...]");
		COrConsole_Printf("  <tag> must contain at least one non-number character");
		return UUcError_Generic;
	}

	// work out which command to call
	if (strcmp(inParameterList[1].val.str, "create") == 0) {
		command_data.type = ONcParticleCommandType_Create;

	} else if (strcmp(inParameterList[1].val.str, "reset") == 0) {
		command_data.type = ONcParticleCommandType_Reset;

	} else if (strcmp(inParameterList[1].val.str, "kill") == 0) {
		command_data.type = ONcParticleCommandType_Kill;

	} else if (strcmp(inParameterList[1].val.str, "do") == 0) {
		command_data.type = ONcParticleCommandType_Do;
		if (inParameterListLength < 3) {
			COrConsole_Printf("syntax: particle <tag> do <eventname>");
			COrConsole_Printf("  you must supply an event name!");
			COrConsole_Printf("  see particle-editing event-list dialog for a list of event names");
			return UUcError_Generic;
		}
		for (itr = 0; itr < P3cEvent_NumEvents; itr++) {
			if (strcmp(inParameterList[2].val.str, P3gEventName[itr]) == 0) {
				command_data.event_index = itr;
				break;
			}
		}
		if (itr >= P3cEvent_NumEvents) {
			COrConsole_Printf("syntax: particle <tag> do <eventname>");
			COrConsole_Printf("  unknown event name '%s'", inParameterList[2].val.str);
			COrConsole_Printf("  see particle-editing event-list dialog for a list of event names");
			return UUcError_Generic;
		}

	} else {
		// try looking for an eventname right here
		for (itr = 0; itr < P3cEvent_NumEvents; itr++) {
			if (strcmp(inParameterList[1].val.str, P3gEventName[itr]) == 0) {
				command_data.event_index = itr;
				break;
			}
		}
		if (itr >= P3cEvent_NumEvents) {
			// couldn't match the command string
			COrConsole_Printf("syntax: particle <tag> <command> [params...]");
			COrConsole_Printf("  unknown command '%s'. valid commands: create kill reset or <event name>", inParameterList[1].val.str);
			COrConsole_Printf("  see particle-editing event-list dialog for a list of event names");
			return UUcError_Generic;
		} else {
			// we matched an event name
			command_data.type = ONcParticleCommandType_Do;
		}
	}

	command_data.num_params = inParameterListLength - 2;
	command_data.param = &inParameterList[2];

	// iterate over all environmental particles with this tag
	EPrEnumerateByTag(inParameterList[0].val.str, ONiParticle3_CommandCallback, (UUtUns32) &command_data);

	return UUcError_None;
}

// temporary code: explodes all particles named "explode%d"
void ONrParticle3_Explode(UUtUns32 inExplodeID)
{
	ONtParticleCommandData command_data;
	char temptag[32];

	command_data.type = ONcParticleCommandType_Do;
	command_data.event_index = P3cEvent_Explode;
	command_data.num_params = 0;
	command_data.param = NULL;
	sprintf(temptag, "explode%d", inExplodeID);

	EPrEnumerateByTag(temptag, ONiParticle3_CommandCallback, (UUtUns32) &command_data);
}

static UUtError
ONiParticle3_PrintTags(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	EPrPrintTags();
	return UUcError_None;
}

static UUtError
ONiParticle3_RemoveDangerous(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	P3rRemoveDangerousProjectiles();
	return UUcError_None;
}

static UUtError
ONiParticle3_DumpParticles(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	P3rDumpParticles();
	return UUcError_None;
}

static void ONiParticle3_DebugCollision_VariableChanged(void)
{
	P3rDebugCollision(ONgParticleDebugCollision);
}

typedef struct ONtParticle3_AllEnvParticleData {
	UUtUns32 tick;
	float time;
	UUtUns32 event;
} ONtParticle3_AllEnvParticleData;

static void
ONiParticle3_EnumerateAllCallback(
	EPtEnvParticle		*inParticle,
	UUtUns32			inUserData)
{
	ONtParticle3_AllEnvParticleData *user_data = (ONtParticle3_AllEnvParticleData *) inUserData;
	UUmAssertReadPtr(user_data, sizeof(*user_data));

	if (inParticle->particle == NULL) {
		EPrNewParticle(inParticle, user_data->tick);
	}
	if (inParticle->particle != NULL) {
		P3rSendEvent(inParticle->particle_class, inParticle->particle, (UUtUns16) user_data->event, user_data->time);
	}
}

static UUtError
ONiParticle3_StartAll(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	ONtParticle3_AllEnvParticleData user_data;

	user_data.tick = ONrGameState_GetGameTime();
	user_data.time = ((float) user_data.tick) / UUcFramesPerSecond;
	user_data.event = P3cEvent_Start;

	EPrEnumerateAllParticles(ONiParticle3_EnumerateAllCallback, (UUtUns32) &user_data);
	return UUcError_None;
}

static UUtError
ONiParticle3_StopAll(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	ONtParticle3_AllEnvParticleData user_data;

	user_data.tick = ONrGameState_GetGameTime();
	user_data.time = ((float) user_data.tick) / UUcFramesPerSecond;
	user_data.event = P3cEvent_Stop;

	EPrEnumerateAllParticles(ONiParticle3_EnumerateAllCallback, (UUtUns32) &user_data);
	return UUcError_None;
}

#if TOOL_VERSION
static UUtError
ONiParticle3_ListCollision(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	P3rListCollision();
	return UUcError_None;
}
#endif

static UUtError
ONiParticle3_DaodanShieldDisable(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32 itr, part;

	ONgDaodanShieldDisableMask = 0;
	for (itr = 0; itr < inParameterListLength; itr++) {
		if (inParameterList[itr].type != SLcType_Int32)
			continue;

		part = inParameterList[itr].val.i;
		if ((part < 0) || (part >= ONcNumCharacterParts))
			continue;

		ONgDaodanShieldDisableMask |= (1 << part);
	}

	return UUcError_None;
}

typedef struct ONtParticle3_WriteUsedParticles_UserData {
	UUtUns32 num_classes;
	BFtFile *file;
} ONtParticle3_WriteUsedParticles_UserData;

static void ONiParticle3_WriteUsedParticles_EnumCallback(P3tParticleClass *inClass, UUtUns32 inUserData)
{
	ONtParticle3_WriteUsedParticles_UserData *user_data = (ONtParticle3_WriteUsedParticles_UserData *) inUserData;
	char linebuf[512], tempbuf[256];
	UUtUns32 len, fill_len, sprite_type;

	sprintf(linebuf, "%s:", inClass->classname);

	len = strlen(linebuf);
	if (len < 34) {
		fill_len = 34 - len;
		UUrMemory_Set8(&linebuf[len], ' ', fill_len);
		len += fill_len;
		linebuf[len] = '\0';
	}

	if (inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_Invisible) {
		strcat(linebuf, "invisible");
	} else {
		if (inClass->definition->flags & P3cParticleClassFlag_Appearance_Geometry) {
			sprintf(tempbuf, "geometry  '%s'", inClass->definition->appearance.instance);

		} else if (inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_Vector) {
			sprintf(tempbuf, "vector");

		} else if (inClass->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal) {
			sprintf(tempbuf, "decal     '%s'", inClass->definition->appearance.instance);

		} else {
			sprite_type = (inClass->definition->flags & P3cParticleClassFlag_Appearance_SpriteTypeMask) >> P3cParticleClassFlags_SpriteTypeShift;

			if ((sprite_type == M3cGeomState_SpriteMode_Contrail) || (sprite_type == M3cGeomState_SpriteMode_ContrailRotated)) {
				sprintf(tempbuf, "contrail  '%s'", inClass->definition->appearance.instance);
			} else {
				sprintf(tempbuf, "sprite    '%s'", inClass->definition->appearance.instance);
			}
		}
		strcat(linebuf, tempbuf);
	}

	BFrFile_Printf(user_data->file, "%s"UUmNL, linebuf);
	user_data->num_classes++;
}

static UUtError
ONiParticle3_WriteUsedParticles(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError				error;
	BFtFileRef				*file_ref;
	ONtParticle3_WriteUsedParticles_UserData user_data;
	char					filename[64];

	// create a file ref
	sprintf(filename, "L%d_Particles.txt", ONrLevel_GetCurrentLevel());
	error = BFrFileRef_MakeFromName(filename, &file_ref);
	UUmError_ReturnOnError(error);

	// create the text file if it doesn't already exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		error = BFrFile_Create(file_ref);
		UUmError_ReturnOnError(error);
	}

	// open the file
	error = BFrFile_Open(file_ref, "w", &user_data.file);
	UUmError_ReturnOnError(error);

	// write out particles used
	user_data.num_classes = 0;
	EPrEnumerateParticleClassesRecursively(ONiParticle3_WriteUsedParticles_EnumCallback, (UUtUns32) &user_data);
	COrConsole_Printf("Wrote %d particle classes to file %s...", user_data.num_classes, filename);

	// cleanup
	BFrFile_Close(user_data.file);
	BFrFileRef_Dispose(file_ref);

	return UUcError_None;
}

#if PARTICLE_PERF_ENABLE
static UUtError
ONiParticle3_Perf(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32 itr, itr2, mask;
	P3tPerfMask *mask_def;

	if (inParameterListLength == 0) {
		// toggle performance on/off
		P3gPerfDisplay = !P3gPerfDisplay;
		return UUcError_None;
	}

	mask = 0;
	for (itr = 0; itr < inParameterListLength; itr++) {
		if (inParameterList[itr].type != SLcType_String)
			continue;

		// look for this mask in the event list
		for (itr2 = 0; itr2 < P3cPerfEvent_Max; itr2++) {
			if (UUmString_IsEqual(P3cPerfEventName[itr2], inParameterList[itr].val.str)) {
				mask |= (1 << itr2);
				break;
			}
		}

		// look for this mask in the mask list
		for (mask_def = P3cPerfMasks; mask_def->name != NULL; mask_def++) {
			if (UUmString_IsEqual(mask_def->name, inParameterList[itr].val.str)) {
				mask |= mask_def->mask;
				break;
			}
		}
	}

	if (mask != 0) {
		P3rPerformanceToggle(mask);
	}

	return UUcError_None;
}
#endif
