#pragma once

#ifndef __BFW_PHYSICS_H__
#define __BFW_PHYSICS_H__

/*
	FILE:	BFW_Physics.h

	AUTHOR:	Quinn Dunki

	CREATED: 4/8/98

	PURPOSE: Physics engine

	Copyright 1998

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "BFW_Akira.h"
#include "BFW_Console.h"
#include "BFW_AppUtilities.h"
#include "BFW_Object_Templates.h"

#define PHcDefaultStaticFriction	0.0025f
#define PHcDefaultDynamicFriction	0.95f


enum {
	OBcFlags_Force_Field =	0x01
};

enum {
	PHcFlags_Physics_Projectile =			0x00000001,
	PHcFlags_Physics_InUse =				0x00000002,
	PHcFlags_Physics_NoCollision =			0x00000004,
	PHcFlags_Physics_DontPush =				0x00000008,
	PHcFlags_Physics_DontBlock =			0x00000010,
	PHcFlags_Physics_PointCollision=		0x00000020,
	PHcFlags_Physics_NoGravity =			0x00000040,
	PHcFlags_Physics_KeepBackfacing =		0x00000100,
	PHcFlags_Physics_FaceCollision =		0x00000200,
	PHcFlags_Physics_DontPopCharacters =	0x00000400,
	PHcFlags_Physics_IgnoreReflection =		0x00000800,
	PHcFlags_Physics_NoMidairFriction =		0x00001000,
	PHcFlags_Physics_CollidedThisFrame =	0x00002000,
	PHcFlags_Physics_CharacterCollisionSkip=0x00004000,
	PHcFlags_Physics_TwoReflectionPasses =	0x00008000,
	PHcFlags_Physics_InitialCollisionPass =	0x00010000
};

enum
{
	PHcUpdateFlag_None				= 0x0000,
	PHcUpdateFlag_Position			= 0x0001

};

typedef enum PHtColliderType
{
	PHcCollider_Env,
	PHcCollider_Phy
} PHtColliderType;

typedef enum PHtPhysicsLevel
{
	PHcPhysics_None,
	PHcPhysics_Static,
	PHcPhysics_Linear,
	PHcPhysics_Animate,
	PHcPhysics_Newton
} PHtPhysicsLevel;

typedef enum PHtCallbackType
{
	PHcCallback_Generic,
	PHcCallback_Character,
	PHcCallback_Object,
	PHcCallback_Camera,
	PHcCallback_Ammo,
	PHcCallback_Weapon,
	PHcCallback_Particle,
	PHcCallback_Powerup
} PHtCallbackType;

typedef struct PHtPhysicsContextArray PHtPhysicsContextArray;
typedef struct PHtPhysicsContext PHtPhysicsContext;

extern PHtPhysicsContextArray *PHgPhysicsContextArray;

typedef struct PHtSphereTree
{
	M3tBoundingSphere sphere;

	struct PHtSphereTree *child;
	struct PHtSphereTree *sibling;
} PHtSphereTree;

typedef struct PHtCollider
{
	PHtColliderType type;
	void *data;					// * PHcCollider_Env = gq, PHcCollider_Phy, physics context

	M3tPlaneEquation plane;		// * Plane of collision (computed by called)
	M3tPoint3D planePoint;		// * Point on plane first touched (computed by called)
	float distanceSquared;		// x distance from object (computed by caller)
} PHtCollider;

typedef void
(*PHtCallback_FindEnvCollisions)(
	const PHtPhysicsContext *inContext,
	const AKtEnvironment *inEnvironment,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders);


typedef void
(*PHtCallback_FindPhyCollisions)(
	const PHtPhysicsContext *inContext,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders);

typedef void
(*PHtCallback_PrivateMovement)(
	PHtPhysicsContext *ioContext);

typedef void
(*PHtCallback_PostReflection)(
	PHtPhysicsContext *ioContext,
	M3tVector3D *inVelocityThisFrame);

typedef void
(*PHtCallback_ReceiveForce)(
	PHtPhysicsContext *ioContext,
	const PHtPhysicsContext *inPushingContext,
	const M3tVector3D *inForce);

typedef void
(*PHtCallback_ApplyForce)(
	PHtPhysicsContext *ioContext,
	PHtCollider *inCollider);

typedef void
(*PHtCallback_PostCollision)(
	PHtPhysicsContext *inContext,
	PHtCollider *ioColliders,
	UUtUns16 *ioNumColliders);

typedef void
(*PHtCallback_Update)(
	PHtPhysicsContext *inContext);

typedef void
(*PHtCallback_PreDispose)(
	PHtPhysicsContext *ioContext);

typedef void
(*PHtCallback_Friction)(
	PHtPhysicsContext *inContext,
	M3tVector3D *ioVelocity);

typedef UUtBool
(*PHtCallback_SkipPhyCollisions)(
	const PHtPhysicsContext *inContext,
	const PHtPhysicsContext *inOtherContext);

void
PHrPhysics_Callback_ReceiveForce(
	PHtPhysicsContext *ioContext,
	const PHtPhysicsContext *inPushingContext,
	const M3tVector3D *inForce);

UUtBool
PHrPhysics_Single_PhyCollision(
	const PHtPhysicsContext *inContext,
	PHtPhysicsContext *inTargetContext,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders);

void
PHrPhysics_Callback_FindEnvCollisions(
	const PHtPhysicsContext *inContext,
	const AKtEnvironment *inEnvironment,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders);

void
PHrPhysics_Callback_FindPhyCollisions(
	const PHtPhysicsContext *inContext,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders);

void PHrPhysics_Callback_Friction(
	PHtPhysicsContext *inContext,
	M3tVector3D *ioVelocity);

typedef struct PHtPhysicsCallbacks
{
	PHtCallbackType						type;

	PHtCallback_FindEnvCollisions		findEnvCollisions;
	PHtCallback_FindPhyCollisions		findPhyCollisions;
	PHtCallback_SkipPhyCollisions		skipPhyCollisions;
	PHtCallback_PrivateMovement			privateMovement;
	PHtCallback_PostReflection			postReflection;
	PHtCallback_PostCollision			postCollision;

	PHtCallback_ApplyForce				applyForce;
	PHtCallback_ReceiveForce			receiveForce;
	PHtCallback_Friction				friction;
	PHtCallback_Update					update;
	PHtCallback_PreDispose				preDispose;
} PHtPhysicsCallbacks;

struct PHtPhysicsContext
{
	// Bounding data
	PHtSphereTree			*sphereTree;
	M3tBoundingBox			axisBox;			// Must be a M3tBoundingBox
	M3tBoundingVolume		*worldAlignedBounds;	// axisBox * matrix

	// Physical data
	M3tVector3D				velocity;
	M3tQuaternion			rotationalVelocity;

	M3tPoint3D				position;
	M3tQuaternion			orientation;
	float					scale;
	M3tMatrix4x3			matrix;
	M3tMatrix4x3			origin;

	float					mass;
	float					gravity;
	float					dampening;
	float					elasticity;
	float					staticFriction;
	float					dynamicFriction;

	// status info
	M3tVector3D				acceleration;

	// Status info
	UUtUns32				flags;
	PHtPhysicsLevel			level;

	// Animation
	OBtAnimationContext		animContext;

	// slaved physics
	PHtPhysicsContext		*master;

	// Callbacks
	PHtPhysicsCallbacks		*callback;

	void					*callbackData;
};

struct PHtPhysicsContextArray
{
	UUtUns32 num_contexts;	// number to loop over
	UUtUns32 max_contexts;	// maximum

	PHtPhysicsContext contexts[1];
};

void PHrPhysicsContext_Remove(
	PHtPhysicsContext *inContext);

PHtPhysicsContext *PHrPhysicsContext_Add(void);

void PHrPhysicsContext_Delete(void);

void PHrPhysicsContext_Clear(void);

UUtError PHrPhysicsContext_New(UUtUns16 inMaxContexts);

void PHrPhysicsContext_Update(void);

void PHrPhysics_MaintainMatrix(
	PHtPhysicsContext *ioObject);

PHtSphereTree *PHrSphereTree_New(
	PHtSphereTree *inData,
	PHtSphereTree *inParent);

void PHrSphereTree_UpdateFromLeaf(
	PHtSphereTree *ioTree);

void PHrSphereTree_Delete(
	PHtSphereTree *inTree);

void PHrSphereTree_Draw(
	PHtSphereTree *tree);

UUtBool PHrCollision_Point_SphereTree(
	const M3tPoint3D *inA,
	const M3tVector3D *inAV,
	PHtSphereTree *inB,
	PHtSphereTree **outChildB);	// optional

PHtSphereTree *PHrCollision_SphereTree_SphereTree(
	PHtSphereTree *inA,
	const M3tVector3D *inAV,	// optional
	PHtSphereTree *inB,
	PHtSphereTree **outChildB);	// optional

UUtBool PHrCollision_Volume_SphereTree(
	const M3tBoundingVolume *inA,
	const M3tVector3D *inAV,
	UUtBool inUseMinimumDiscardThreshold,
	const PHtSphereTree *inB,
	PHtSphereTree **outChildB,	// optional
	M3tPlaneEquation **outPlane,		// optional
	M3tPoint3D *outCollisionPoint,		// optional
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders,
	void *inColliderData);

UUtBool PHrCollision_Volume_Ray(
	const M3tBoundingVolume *inA,
	const M3tPoint3D *inPoint,
	const M3tVector3D *inV,
	M3tPlaneEquation *outPlane,	// optional
	M3tPoint3D *outPoint);		// optional

UUtBool PHrCollision_Volume_Point_Inside(
	const M3tBoundingVolume *inA,
	const M3tPoint3D *inPoint);

UUtBool
PHrCollision_Quad_SphereTreeVector(
	const AKtEnvironment*		inEnvironment,
	UUtUns32					inGQIndex,
	const PHtSphereTree			*inTree,
	const M3tVector3D*			inVector,
	M3tPoint3D					*outIntersection);	// NULL if you don't care


UUtBool OBrPhysics_Update_ReflectHalt(PHtPhysicsContext *inContext, M3tVector3D *inVelocityThisFrame);
UUtBool OBrPhysics_Update_Reflect(PHtPhysicsContext *inContext, M3tVector3D *inVelocityThisFrame);

void PHrPhysics_Init(
	PHtPhysicsContext *ioPhysics);

void PHrPhysics_Delete(
	PHtPhysicsContext *ioPhysics);

void PHrPhysics_Accelerate(
	PHtPhysicsContext *physics,
	const M3tVector3D *inAcceleration);

UUtBool PHrPhysics_Update_Animation(
	OBtAnimationContext *ioObject);

UUtError PHrPhysics_Initialize(
	void);

void PHrPhysics_Terminate(
	void);

void
PHrPhysics_Colliders_GetFromSphere(
	M3tBoundingSphere*		inSphere,
	M3tVector3D*			inVector,
	UUtUns16				*outNumColliders,
	PHtCollider				*outColliders);

void PHrPhysics_Pause_Animation(OBtAnimationContext *ioAnimContext);
void PHrPhysics_Unpause_Animation(OBtAnimationContext *ioAnimContext);

#endif // #define __BFW_PHYSICS_H__
