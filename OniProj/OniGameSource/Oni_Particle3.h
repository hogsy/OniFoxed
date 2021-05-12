// ======================================================================
// Oni_Particle3.h
// ======================================================================
#ifndef ONI_PARTICLE3_H
#define ONI_PARTICLE3_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Particle3.h"

#include "Oni.h"



// ======================================================================
// external globals
// ======================================================================

extern P3tParticleClass *ONgParticle3_DaodanShield;
extern UUtBool ONgParticle3_Valid;

// ======================================================================
// prototypes
// ======================================================================

// initialise the particlesystem interface
UUtError ONrParticle3_Initialize(void);
void ONrParticle3_LevelZeroLoaded(void);
UUtError ONrParticle3_LevelBegin(void);
void ONrParticle3_LevelEnd(void);

// kill all particles and then recreate those that are needed
void ONrParticle3_KillAll(UUtBool inRecreate);

// emit breaking glass from a quad
void ONrParticle3_EmitGlassPieces(AKtGQ_General *inQuad, float inDamage,
								  M3tVector3D *inBlastSource, M3tVector3D *inBlastNormal,
								  M3tVector3D *inBlastDir, float inBlastRadius);

// temporary code: explodes all particles named "explode%d"
void ONrParticle3_Explode(UUtUns32 inExplodeID);

// generate an emitted position on the surface of a character
UUtBool ONrParticle3_FindBodySurfacePosition(UUtUns32 inCharacterOwner, float inRadius, M3tPoint3D *outPoint,
											 M3tMatrix4x3 **outMatrix, M3tMatrix3x3 *outOrientation);

// generate an emitted position on a bone of a character
UUtBool ONrParticle3_FindBodyBonePosition(UUtUns32 inCharacterOwner, float inRadius, M3tPoint3D *outPoint, M3tMatrix4x3 **outMatrix);

// update the particle system for a changed graphics quality value
void ONrParticle3_UpdateGraphicsQuality(void);

// ======================================================================
#endif /* ONI_PARTICLE3_H */
