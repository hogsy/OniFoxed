/*
	FILE:	BFW_Physics.c

	AUTHOR:	Quinn Dunki, Michael Evans

	CREATED: 4/8/98

	PURPOSE: Physics engine

	Copyright (c) Bungie Software 1998, 1999, 2000

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Physics.h"
#include "BFW_Object.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "BFW_Akira.h"
#include "BFW_Console.h"
#include "BFW_AppUtilities.h"
#include "BFW_BitVector.h"
#include "BFW_ScriptLang.h"
#include "Oni.h"
#include "Oni_GameStatePrivate.h"

#define OBcMaxColliders				256
#define OBcMaxCollisions			AKcMaxNumCollisions
#define PHcBackfacingMinVelocity	(0.4f / UUcFramesPerSecond)

UUtBool PHgKeyForces = 0;
UUtBool PHgPhysicsActive = UUcTrue;
UUtBool PHgShowCollisions = 0;

PHtPhysicsContextArray *PHgPhysicsContextArray = NULL;

static void PHiPhysics_Apply_Gravity(PHtPhysicsContext *inContext)
{
	/************
	* Applies gravity to an object
	*/
	if (inContext->flags & PHcFlags_Physics_NoGravity)  {
	}
	else {
		inContext->acceleration.y -= inContext->gravity;
	}

	return;
}

static PHtSphereTree *OBiCollision_SphereTree_Env(
	PHtSphereTree *inTree,
	UUtUns32 inSkipFlag,
	M3tVector3D *inV)
{
	/***********************
	* Collides a sphere tree with the environment.
	* Returns which child (if any) collided.
	*/

	UUtBool detect,needone = UUcFalse;
	PHtSphereTree *child;

	detect = AKrCollision_Sphere(
		ONgGameState->level->environment,
		&inTree->sphere,
		inV,
		inSkipFlag,
		OBcMaxCollisions);
	if (!detect) return NULL;
	UUmAssert(AKgNumCollisions < OBcMaxCollisions);	// if equal, we may have missed some

	// if we are a leaf return the collided part
	if (inTree->child == NULL)
	{
		return inTree;
	}

	// check if our children intersect
	for(child = inTree->child; child != NULL; child = child->sibling)
	{
		PHtSphereTree *collision;

		collision = OBiCollision_SphereTree_Env(
					child,
					inSkipFlag,
					inV);

		if (collision)
		{
			return collision;
		}
	}

	return NULL;
}

UUtBool PHrCollision_Point_SphereTree(
	const M3tPoint3D *inA,
	const M3tVector3D *inAV,
	PHtSphereTree *inB,
	PHtSphereTree **outChildB)	// optional
{
	/************
	* Detects if A will hit B this frame.
	*
	* Collides iff a collides with leaf node of b
	*
	* In outChildB sets the leaf of B that collided
	*
	*/

	PHtSphereTree *collideB;
	UUtBool intersect,collide = UUcFalse;
	M3tPoint3D end;

	MUmVector_Add(end,*inAV,*inA);
	intersect = CLrSphere_Line(inA,&end,&inB->sphere);

	// Velocity is not accounted for yet, but this seems to kinda work in the meantime
	if (!intersect)
	{
		collideB = NULL;
		collide = UUcFalse;
	}
	// B is child
	else if (inB->child == NULL)
	{
		collideB = inB;
		collide = UUcTrue;
	}
	else if (inB->child)
	{
		PHtSphereTree *childB;

		// Parent collided, now check for any children
		for(childB = inB->child; childB != NULL; childB = childB->sibling)
		{
			collide = PHrCollision_Point_SphereTree(
				inA,
				inAV,
				childB,
				&collideB);

			if (collide)
			{
				break;
			}
		}
	}

	if (outChildB)
	{
		*outChildB = collideB;
	}

	return collide;
}




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
	void *inColliderData)
{
	/************
	* Detects if A will hit B this frame.
	*
	* Collides iff a collides with leaf node of b
	*
	* In outChildB sets the leaf of B that collided
	*
	*/

	UUtBool collide,discardByAV,foundCollision = UUcFalse;
	UUtUns32 i;
	float dot,smallestDot=-1e9f,d;
	M3tVector3D normal,normalV;
	UUtUns16 oldNumColliders = *ioNumColliders;
	M3tPlaneEquation *collisionPlane;
	M3tPoint3D checkPoint, collisionPoint;

	normalV = *inAV;
	d = MUmVector_GetLength(normalV);

	discardByAV = !(inUseMinimumDiscardThreshold && (fabs(d) < PHcBackfacingMinVelocity));
	if (discardByAV) {
		if (d!=0.0f) MUmVector_Scale(normalV,1.0f/d);
		else return UUcFalse;
	}

	for (i=0; i<M3cNumBoundingFaces; i++)
	{
		collide = CLrQuad_Sphere(
			(CLtQuadProjection)inA->curProjections[i],
			inA->worldPoints,
			inA->curPlanes + i,
			0,
			inA->faces + i,
			&inB->sphere,
			&checkPoint);
		if (collide)
		{
			if (discardByAV) {
				// discard collisions that don't match our velocity
				AKmPlaneEqu_GetNormal(inA->curPlanes[i],normal);
				dot = MUrAngleBetweenVectors3D(&normalV,&normal);
				if (dot < M3cHalfPi)
					continue;
			}

			UUmAssert(outCollisionPoint);
			outColliders[*ioNumColliders].plane = inA->curPlanes[i];
			outColliders[*ioNumColliders].planePoint = checkPoint;
			outColliders[*ioNumColliders].type = PHcCollider_Phy;
			outColliders[*ioNumColliders].data = inColliderData;
			(*ioNumColliders)++;

			collisionPoint = checkPoint;
			collisionPlane = (M3tPlaneEquation *) &inA->curPlanes[i];
			foundCollision = UUcTrue;
		}
	}

	if (foundCollision)
	{
		PHtSphereTree *child;

		if (inB->child)
		{
			foundCollision = UUcFalse;
			for(child = inB->child; child != NULL; child = child->sibling)
			{
				// try our children
				collide = PHrCollision_Volume_SphereTree(
					inA,
					inAV,
					inUseMinimumDiscardThreshold,
					child,
					outChildB,
					&collisionPlane,
					&collisionPoint,
					outColliders,
					ioNumColliders,
					inColliderData);
				if (collide)
				{
					foundCollision = UUcTrue;
					break;
				}
			}
			if (!foundCollision) *ioNumColliders = oldNumColliders;	// Remove effect of this test if children failed
		}
	}

	if (foundCollision) {
		*outPlane = collisionPlane;
		*outCollisionPoint = collisionPoint;
	}

	return foundCollision;
}

PHtSphereTree *PHrCollision_SphereTree_SphereTree(
	PHtSphereTree *inA,
	const M3tVector3D *inAV,	// optional
	PHtSphereTree *inB,
	PHtSphereTree **outChildB)	// optional
{
	/************
	* Detects if A will hit B this frame.
	*
	* Collides iff a leaf node of a collides with leaf node of b
	*
	* returns the leaf of a that collide and in outChildB sets the
	* leaf of B that collided
	*
	*/

	PHtSphereTree *collideA,*collideB;
	UUtBool intersect;

	intersect = CLrSphere_Sphere(&inA->sphere, inAV, &inB->sphere);

	// Velocity is not accounted for yet, but this seems to kinda work in the meantime
	if (!intersect)
	{
		collideA = NULL;
		collideB = NULL;
	}
	// we were both children
	else if ((inA->child == NULL) && (inB->child == NULL))
	{
		collideA = inA;
		collideB = inB;
	}
	else if (inA->child)
	{
		PHtSphereTree *childA;

		// Parent collided, now check for any children
		for(childA = inA->child; childA != NULL; childA = childA->sibling)
		{
			collideA = PHrCollision_SphereTree_SphereTree(
				childA,
				inAV,
				inB,
				&collideB);

			if (collideA)
			{
				break;
			}
		}
	}
	else if (inB->child)
	{
		PHtSphereTree *childB;

		// Parent collided, now check for any children
		for(childB = inB->child; childB != NULL; childB = childB->sibling)
		{
			collideA = PHrCollision_SphereTree_SphereTree(
				inA,
				inAV,
				childB,
				&collideB);

			if (collideA)
			{
				break;
			}
		}
	}

	if (outChildB)
	{
		*outChildB = collideB;
	}

	return collideA;
}


void PHrPhysics_Pause_Animation(OBtAnimationContext *ioAnimContext)
{
	if(!(ioAnimContext->animContextFlags & OBcAnimContextFlags_CollisionPause))
	{
		ioAnimContext->animContextFlags |= OBcAnimContextFlags_CollisionPause;
		//ioAnimContext->animationFrame	-= ioAnimContext->animationStep;
	}
}

void PHrPhysics_Unpause_Animation(OBtAnimationContext *ioAnimContext)
{
	if( ioAnimContext->animContextFlags & OBcAnimContextFlags_CollisionPause )
	{
		ioAnimContext->animContextFlags &= ~OBcAnimContextFlags_CollisionPause;
		ioAnimContext->frameStartTime = ONgGameState->gameTime;
	}
}

static UUtBool PHiPhysics_Update_AnimCollision(
	PHtPhysicsContext *inPhysics)
{
	if (inPhysics->level == PHcPhysics_Animate) {
		OBtAnimationContext *master_context = NULL;

		if ((inPhysics->master != NULL) && (inPhysics->master->level == PHcPhysics_Animate)) {
			master_context = &inPhysics->master->animContext;
			inPhysics->animContext.numPausedFrames = UUmMax(inPhysics->animContext.numPausedFrames, master_context->numPausedFrames);
		}

		if (inPhysics->animContext.numPausedFrames > 0) {
			// we are paused for a certain time
			PHrPhysics_Pause_Animation(&inPhysics->animContext);
			if (master_context) {
				PHrPhysics_Pause_Animation(master_context);
			}
			inPhysics->animContext.numPausedFrames--;

			if (inPhysics->animContext.numPausedFrames == 0) {
				PHrPhysics_Unpause_Animation(&inPhysics->animContext);
				if (master_context) {
					PHrPhysics_Unpause_Animation(master_context);
				}
			}
			return UUcTrue;
		}
	}

	return UUcFalse;
}

UUtBool PHrPhysics_Update_Animation(
	OBtAnimationContext *ioAnimContext)
{
	/**************
	* Updates object animation for the frame, returning whether
	* the animation was completed or not.
	*/

	OBtAnimation *animation;
	UUtBool done = UUcFalse;

	animation = ioAnimContext->animation;
	if (!animation) return UUcFalse;

	if (ioAnimContext->animContextFlags & OBcAnimContextFlags_CollisionPause) {
		// we are paused by external influences
		return UUcFalse;
	}

	if (!(ioAnimContext->animContextFlags & OBcAnimContextFlags_Animate) && (animation->animFlags & OBcAnimFlags_Autostart))
	{
		// Autostart animation
		OBrAnim_Start(ioAnimContext);
	}

	if (ioAnimContext->animContextFlags & OBcAnimContextFlags_Animate)
	{
		// Track frame number
		if (ONgGameState->gameTime + animation->ticksPerFrame > ioAnimContext->frameStartTime)
		{
			ioAnimContext->frameStartTime = ONgGameState->gameTime;
			ioAnimContext->animationFrame += ioAnimContext->animationStep;

			if (ioAnimContext->animationFrame >= animation->numFrames ||
				(ioAnimContext->animationStop && ioAnimContext->animationFrame >= ioAnimContext->animationStop))
			{
				// End reached- what now?
				if (animation->animFlags & OBcAnimFlags_Pingpong)
				{
					ioAnimContext->animationFrame = animation->numFrames - 1;
					ioAnimContext->animationStep = -1;
				}
				else if (animation->animFlags & OBcAnimFlags_Loop)
				{
					ioAnimContext->animationFrame = 0;
				}
				else
				{
					OBrAnim_Stop(ioAnimContext);
					done = UUcTrue;
				}
			}
			if (ioAnimContext->animationFrame < 0)
			{
				// Beginning reached- what now?
				if (animation->animFlags & OBcAnimFlags_Pingpong)
				{
					ioAnimContext->animationFrame = 0;
					ioAnimContext->animationStep = 1;
				}
				else
				{
					OBrAnim_Stop(ioAnimContext);
					done = UUcTrue;
				}
			}
		}
	}

	return done;
}

static void PHiRemoveBackfacingCollisions(
		PHtPhysicsContext *inContext,
		const M3tVector3D *inVelocity,
		PHtCollider *ioColliders,
		UUtUns16 *ioNumColliders)
{
	/******************
	* Update physics context for one frame
	*
	* 1. remove collisions that we are moving away from
	*
	*/

	UUtUns16 itr;
	UUtUns16 count = *ioNumColliders;

	for(itr = 0; itr < count; )
	{
		UUtBool remove;
		M3tVector3D colliderNormal;

		colliderNormal.x = ioColliders[itr].plane.a;
		colliderNormal.y = ioColliders[itr].plane.b;
		colliderNormal.z = ioColliders[itr].plane.c;

		remove = MUrVector_DotProduct(inVelocity, &colliderNormal) > 0.f;

		if (!remove) {
			M3tVector3D toPlane;

			MUmVector_Subtract(toPlane, ioColliders[itr].planePoint, inContext->sphereTree->sphere.center);

			// if vectors are facing each other, remove
			if (MUrVector_DotProduct(&toPlane, &colliderNormal) > 0.f) {
				remove = UUcTrue;
			}
		}

		if (remove)
		{
			count -= 1;
			ioColliders[itr] = ioColliders[count];
		}
		else
		{
			itr++;
		}
	}

	*ioNumColliders = count;

	return;
}

static int UUcExternal_Call sort_collisions_callback(const void *elem_1, const void *elem_2)
{
	int result;
	const PHtCollider *collider_1 = elem_1;
	const PHtCollider *collider_2 = elem_2;

	if (collider_1->distanceSquared < collider_2->distanceSquared)
	{
		result = -1;
	}
	else if (collider_1->distanceSquared > collider_2->distanceSquared)
	{
		result = 1;
	}
	else
	{
		result = 0;
	}

	return result;
}

static void sort_collisions(
		const PHtPhysicsContext *inContext,
		PHtCollider *colliders,
		UUtUns16 numColliders)
{
	UUtUns16 itr;

	for(itr = 0; itr < numColliders; itr++)
	{
		colliders[itr].distanceSquared = MUmVector_GetDistanceSquared(
			inContext->sphereTree->sphere.center,
			colliders[itr].planePoint);
	}

	qsort(colliders, numColliders, sizeof(PHtCollider), sort_collisions_callback);

	return;
}
// xxx this function needs to be made to handle reflection for throw teleport
UUtBool OBrPhysics_Update_Reflect(PHtPhysicsContext *inContext, M3tVector3D *inVelocityThisFrame)
{
	UUtBool reflected = UUcFalse;
	UUtUns16 itr;
	UUtUns16 numColliders;
	PHtCollider colliders[OBcMaxColliders];
	AKtEnvironment *environment = ONgGameState->level->environment;
	PHtPhysicsContext *pushedContext;

	MUmAssertVector(*inVelocityThisFrame, 1e9f);

	numColliders = 0;

	switch (inContext->level)
	{
		case PHcPhysics_None:
		case PHcPhysics_Static:
			break;

		case PHcPhysics_Animate:
			if (inContext->callback->findPhyCollisions != NULL) {
				inContext->callback->findPhyCollisions(inContext, inVelocityThisFrame, colliders, &numColliders);
			}
			break;

		case PHcPhysics_Linear:
		case PHcPhysics_Newton:
			if (inContext->callback->findEnvCollisions != NULL) {
                            inContext->callback->findEnvCollisions(inContext, environment, inVelocityThisFrame, colliders, &numColliders);
                        }
			if (inContext->callback->findPhyCollisions != NULL) {
				inContext->callback->findPhyCollisions(inContext, inVelocityThisFrame, colliders, &numColliders);
			}
			break;

		default:
			UUmAssert(!"Unsupported physics type");
	}

	//
	if ((inContext->flags & PHcFlags_Physics_KeepBackfacing) == 0)
	{
		PHiRemoveBackfacingCollisions(inContext, inVelocityThisFrame, colliders, &numColliders);
	}

	//
	sort_collisions(inContext, colliders, numColliders);

	if (inContext->flags & PHcFlags_Physics_IgnoreReflection) {
		goto skip_reflection;
	}

	for(itr = 0; itr < numColliders; itr++)
	{
		M3tVector3D normal;
		M3tVector3D pushBackVector;

		normal.x = colliders[itr].plane.a;
		normal.y = colliders[itr].plane.b;
		normal.z = colliders[itr].plane.c;

		// Apply collision effects
		MUrVector_PushBack(&pushBackVector, inVelocityThisFrame, &normal);
		MUmAssertVector(pushBackVector, 1e9f);

		MUmVector_Decrement(*inVelocityThisFrame, pushBackVector);

		if (!MUmVector_IsZero(pushBackVector))
		{
			reflected = UUcTrue;

			if (inContext->callback->applyForce != NULL) {
				inContext->callback->applyForce(inContext, colliders + itr);
			}

			if (PHcCollider_Phy == colliders[itr].type) {
				pushedContext = (PHtPhysicsContext *) colliders[itr].data;

				if ((pushedContext->callback->receiveForce != NULL) && (0 == (inContext->flags & PHcFlags_Physics_DontPush))) {
					pushedContext->callback->receiveForce(pushedContext, inContext, &pushBackVector);
				}
			}

			break;
		}

		if (PHcCollider_Env == colliders[itr].type)
		{
			AKtGQ_General *gq = colliders[itr].data;

			if (PHgShowCollisions) gq->flags |= AKcGQ_Flag_Draw_Flash;
		}

		MUmAssertVector(*inVelocityThisFrame, 1e9f);
	}

	if (reflected) {
		inContext->flags |= PHcFlags_Physics_CollidedThisFrame;

		if (inContext->callback->postReflection != NULL) {
			inContext->callback->postReflection(inContext, inVelocityThisFrame);
		}
	}

skip_reflection:

	if (inContext->callback->postCollision)
	{
		inContext->callback->postCollision((PHtPhysicsContext *)inContext,colliders,&numColliders);
	}

	return reflected;
}

UUtBool OBrPhysics_Update_ReflectHalt(PHtPhysicsContext *inContext, M3tVector3D *inVelocityThisFrame)
{
	UUtBool reflected = UUcFalse;
	UUtUns16 itr;
	UUtUns16 numColliders;
	PHtCollider colliders[OBcMaxColliders];
	AKtEnvironment *environment = ONgGameState->level->environment;
	PHtPhysicsContext *pushedContext;

	// 3. recollide & respond with a halt

	// PHtCollider *closest_collider;

	numColliders = 0;

	switch (inContext->level)
	{
		case PHcPhysics_None:
		case PHcPhysics_Static:
			break;

		case PHcPhysics_Animate:
			if (inContext->callback->findPhyCollisions != NULL) {
				inContext->callback->findPhyCollisions(inContext, inVelocityThisFrame, colliders, &numColliders);
			}
			break;

		case PHcPhysics_Linear:
		case PHcPhysics_Newton:
			if (inContext->callback->findEnvCollisions != NULL) {
                            inContext->callback->findEnvCollisions(inContext, environment, inVelocityThisFrame, colliders, &numColliders);
                        }
			if (inContext->callback->findPhyCollisions != NULL) {
				inContext->callback->findPhyCollisions(inContext, inVelocityThisFrame, colliders, &numColliders);
			}
			break;

		default:
			UUmAssert(!"Unsupported physics type");
	}

	PHiRemoveBackfacingCollisions(inContext, inVelocityThisFrame, colliders, &numColliders);

	// closest_collider = find_closest_collider(numColliders, colliders);
	// distance_to_plane = magic_compute_distance_to_plane(inContext, closest_collider.plane);
	// MUrVector_Set_Length(inVelocityThisFrame, distance_to_plane);

	for(itr = 0; itr < numColliders; itr++)
	{
		M3tVector3D normal;
		M3tVector3D pushBackVector;

		normal.x = colliders[itr].plane.a;
		normal.y = colliders[itr].plane.b;
		normal.z = colliders[itr].plane.c;

		MUrVector_PushBack(&pushBackVector, inVelocityThisFrame, &normal);
		MUmAssertVector(pushBackVector, 1e9f);

		if (!MUmVector_IsZero(pushBackVector))
		{
			reflected = UUcTrue;

			if (inContext->callback->applyForce != NULL)
			{
				inContext->callback->applyForce(inContext, colliders + itr);
			}

			if (PHcCollider_Phy == colliders[itr].type)
			{
				pushedContext = (PHtPhysicsContext *) colliders[itr].data;

				if ((pushedContext->callback->receiveForce != NULL) && (0 == (inContext->flags & PHcFlags_Physics_DontPush)))
				{
					pushedContext->callback->receiveForce(pushedContext, inContext, inVelocityThisFrame);
				}
			}

			inVelocityThisFrame->x = 0.f;
			inVelocityThisFrame->y = 0.f;
			inVelocityThisFrame->z = 0.f;

			if (PHcCollider_Env == colliders[itr].type)
			{
				AKtGQ_General *gq = colliders[itr].data;

				if (PHgShowCollisions) gq->flags |= AKcGQ_Flag_Draw_Flash;
			}
		}

		MUmAssertVector(*inVelocityThisFrame, 1e9f);
	}

	MUmAssertVector(*inVelocityThisFrame, 1e9f);

	if (reflected) {
		if (inContext->callback->postReflection != NULL) {
			inContext->callback->postReflection(inContext, inVelocityThisFrame);
		}
	}

	return reflected;
}


static void OBiPhysics_Update(PHtPhysicsContext *inContext)
{
	/******************
	* Update physics context for one frame
	*
	*/

	M3tVector3D velocityThisFrame;
	AKtEnvironment *environment = ONgGameState->level->environment;

	// set up for the new frame
	inContext->flags &= ~PHcFlags_Physics_CollidedThisFrame;

	// 0. track animation
	PHiPhysics_Update_AnimCollision(inContext);
	PHrPhysics_Update_Animation(&inContext->animContext);

	// 1. build velocity
	switch (inContext->level)
	{
		case PHcPhysics_None:
		case PHcPhysics_Static:
			velocityThisFrame.x = 0.0f;
			velocityThisFrame.y = 0.0f;
			velocityThisFrame.z = 0.0f;
			break;

		case PHcPhysics_Animate:
			velocityThisFrame.x = 0.0f;
			velocityThisFrame.y = 0.0f;
			velocityThisFrame.z = 0.0f;
			PHrPhysics_MaintainMatrix(inContext);
			return;

		case PHcPhysics_Linear:
		case PHcPhysics_Newton:
			MUmAssertVector(inContext->velocity, 1e9f);
			velocityThisFrame = inContext->velocity;
			break;

		default:
			UUmAssert(!"Unsupported physics type");
			break;
	}

	// 2. reflect velocity based on collision
	//
	if (!(inContext->flags & PHcFlags_Physics_NoCollision))
	{
		UUtBool reflected;

		// initial collision pass uses slightly different rules for rejecting
		// backfacing collisions
		inContext->flags |= PHcFlags_Physics_InitialCollisionPass;
		reflected = OBrPhysics_Update_Reflect(inContext, &velocityThisFrame);
		inContext->flags &= ~PHcFlags_Physics_InitialCollisionPass;

		if (0 == (inContext->flags & PHcFlags_Physics_InUse)) {
			return;
		}

		if (inContext->flags & PHcFlags_Physics_TwoReflectionPasses) {
			reflected = OBrPhysics_Update_Reflect(inContext, &velocityThisFrame);

			if (0 == (inContext->flags & PHcFlags_Physics_InUse)) {
				return;
			}
		}

		if (reflected) {
			OBrPhysics_Update_ReflectHalt(inContext, &velocityThisFrame);
		}

		if (0 == (inContext->flags & PHcFlags_Physics_InUse)) {
			return;
		}
	}

	// 3. verify velocity
	switch (inContext->level)
	{
		case PHcPhysics_None:
		case PHcPhysics_Static:
		case PHcPhysics_Animate:
			velocityThisFrame.x = 0.0f;
			velocityThisFrame.y = 0.0f;
			velocityThisFrame.z = 0.0f;
			break;

		case PHcPhysics_Linear:
		case PHcPhysics_Newton:
			break;

		default:
			UUmAssert(!"Unsupported physics type");
			break;
	}


	MUmAssertVector(inContext->velocity, 1e9f);
	MUmAssertVector(velocityThisFrame, 1e9f);

	// 4. apply movement
	MUmVector_Increment(inContext->position, velocityThisFrame);

	MUmAssertVector(inContext->velocity, 1e9f);
	MUmAssertVector(velocityThisFrame, 1e9f);

	MUmAssertVector(inContext->velocity, 1e9f);
	MUmAssertVector(velocityThisFrame, 1e9f);

	// 5. private movement
	if (NULL != inContext->callback->privateMovement)
	{
		inContext->callback->privateMovement(inContext);
	}

	MUmAssertVector(inContext->velocity, 1e9f);
	MUmAssertVector(velocityThisFrame, 1e9f);

	// 6. dampen velocity based on collision
	inContext->velocity = velocityThisFrame;

	MUmAssertVector(inContext->velocity, 1e9f);
	MUmAssertVector(velocityThisFrame, 1e9f);

	if (((inContext->flags & PHcFlags_Physics_NoMidairFriction) == 0) || (inContext->flags & PHcFlags_Physics_CollidedThisFrame)) {
		// 7. apply friction
		if (inContext->callback->friction != NULL) {
			inContext->callback->friction(inContext, &inContext->velocity);
		}
	}

	MUmAssertVector(inContext->velocity, 1e9f);
	MUmAssertVector(velocityThisFrame, 1e9f);

	// 8. update
	if (NULL != inContext->callback->update)
	{
		inContext->callback->update(inContext);
	}
	PHrPhysics_MaintainMatrix(inContext);

	MUmAssertVector(inContext->velocity, 1e9f);
	MUmAssertVector(velocityThisFrame, 1e9f);

	return;
}

static void OBiUpdateDebugging(
	PHtPhysicsContext *inContext)
{
	// Debugging forces
	if (PHgKeyForces)
	{
		ONtInputState *localInput = &ONgGameState->local.localInput;
		M3tVector3D dir = {0,0,0};
		#define cMagnitude 0.5f

		if (localInput->buttonWentDown & LIc_BitMask_Forward) dir.z = cMagnitude;
		if (localInput->buttonWentDown & LIc_BitMask_Backward) dir.z = -cMagnitude;
		if (localInput->buttonWentDown & LIc_BitMask_StepLeft) dir.x = cMagnitude;
		if (localInput->buttonWentDown & LIc_BitMask_StepRight) dir.x = -cMagnitude;

		if (dir.x || dir.y || dir.z)
		{
			PHrPhysics_Accelerate(inContext,&dir);
		}
	}
}

UUtError PHrPhysicsContext_New(UUtUns16 inMaxContexts)
{
	/*************
	* Makes a new physics context array
	*/

	UUtUns32 sizeOfArray = sizeof(PHtPhysicsContextArray) + ((inMaxContexts-1) * sizeof(PHtPhysicsContext));
	UUtError error = UUcError_None;

	UUmAssert(NULL == PHgPhysicsContextArray);

	PHgPhysicsContextArray = UUrMemory_Block_New(sizeOfArray);

	if (NULL == PHgPhysicsContextArray) {
		error = UUcError_OutOfMemory;
	}
	else {
		PHgPhysicsContextArray->max_contexts= inMaxContexts;
		PHrPhysicsContext_Clear();
	}

	return error;
}

void PHrPhysicsContext_Clear(void)
{
	/*************
	* clears a physics context array
	*/

	UUmAssert(NULL != PHgPhysicsContextArray);

	if (NULL != PHgPhysicsContextArray) {
		UUtUns32 max_contexts = PHgPhysicsContextArray->max_contexts;
		UUtUns32 size_of_array = sizeof(PHtPhysicsContextArray) + ((max_contexts - 1) * sizeof(PHtPhysicsContext));

		UUrMemory_Clear(PHgPhysicsContextArray, size_of_array);

		PHgPhysicsContextArray->max_contexts = max_contexts;
		PHgPhysicsContextArray->num_contexts = 0;
	}

	return;
}

void PHrPhysicsContext_Delete(void)
{
	/*****************
	* Deletes a physics context array
	*/


	if (NULL != PHgPhysicsContextArray) {
		UUtUns32 itr;
		UUtUns32 count = PHgPhysicsContextArray->num_contexts;

		for (itr = 0; itr < count; itr++)
		{
			if (PHgPhysicsContextArray->contexts[itr].flags & PHcFlags_Physics_InUse) {
				PHrPhysics_Delete(PHgPhysicsContextArray->contexts + itr);
			}
		}

		UUrMemory_Block_Delete(PHgPhysicsContextArray);

		PHgPhysicsContextArray = NULL;
	}

	return;
}

PHtPhysicsContext *PHrPhysicsContext_Add(void)
{
	/*****************
	* Makes a new physics context and returns
	* a pointer to it
	*/

	PHtPhysicsContext *result = NULL;

	UUmAssert(NULL != PHgPhysicsContextArray);

	if (NULL != PHgPhysicsContextArray) {
		UUtUns32 itr;

		for (itr=0; itr < PHgPhysicsContextArray->max_contexts; itr++)
		{
			if (PHgPhysicsContextArray->contexts[itr].flags & PHcFlags_Physics_InUse) {
				continue;
			}

			// maintain the watermark
			PHgPhysicsContextArray->num_contexts = UUmMax(PHgPhysicsContextArray->num_contexts, itr + 1);

			result = PHgPhysicsContextArray->contexts + itr;
			PHrPhysics_Init(result);

			goto exit;
		}
	}

	UUmAssert(!"Ran out of physics contexts! Fatal!");

exit:
	return result;
}

void PHrPhysicsContext_Remove(PHtPhysicsContext *inContext)
{
	/*******************
	* Removes a context from an array
	*/

	PHrPhysics_Delete(inContext);
	inContext->flags &=~PHcFlags_Physics_InUse;

	return;
}

void PHrPhysicsContext_Update(void)
{
	/*********************
	* Updates all the physics contexts
	*/

	UUtUns32 i;
	PHtPhysicsContext *context;

	if (!PHgPhysicsActive) return;

	// Prepare physics for the frame
	for (i=0; i < PHgPhysicsContextArray->num_contexts; i++)
	{
		context = PHgPhysicsContextArray->contexts + i;

		if (context->flags & PHcFlags_Physics_InUse) {
			PHiPhysics_Apply_Gravity(context);
		}
	}

	// apply pending acceleration for the frame
	for (i=0; i<PHgPhysicsContextArray->num_contexts; i++)
	{
		context = PHgPhysicsContextArray->contexts+i;

		if (context->flags & PHcFlags_Physics_InUse) {
			MUmVector_Increment(context->velocity, context->acceleration);
			context->acceleration = MUgZeroPoint;
		}
	}

	// Evaluate physics for the frame
	for (i=0; i<PHgPhysicsContextArray->num_contexts; i++)
	{
		context = PHgPhysicsContextArray->contexts+i;

		if (context->flags & PHcFlags_Physics_InUse) {
			OBiPhysics_Update(context);
		}
	}

	return;
}

PHtSphereTree *PHrSphereTree_New(PHtSphereTree *inData, PHtSphereTree *inParent)
{
	/*****************
	* Makes a new sphere node in a sphere tree
	*/

	PHtSphereTree *tree;

	// allocate the tree
	if (inData)
	{
		tree = inData;
		UUrMemory_Clear(tree, sizeof(*tree));
	}
	else
	{
		tree = UUrMemory_Block_NewClear(sizeof(PHtSphereTree));
	}

	if (tree != NULL)
	{
		// place it inside the parent
		if (inParent)
		{
			tree->sibling = inParent->child;
			inParent->child = tree;
		}
	}

	return tree;
}

void PHrSphereTree_UpdateFromLeaf(PHtSphereTree *ioTree)
{
	M3tBoundingBox_MinMax bbox;
	PHtSphereTree *curChild;

	if (NULL == ioTree->child)
	{
		return;
	}

	// create the bbox from our first child
	curChild = ioTree->child;
	PHrSphereTree_UpdateFromLeaf(curChild);
	MUrMinMaxBBox_CreateFromSphere(NULL, &curChild->sphere, &bbox);

	// grow it by all our other children
	for(curChild = curChild->sibling; curChild != NULL; curChild = curChild->sibling)
	{
		PHrSphereTree_UpdateFromLeaf(curChild);
		MUrMinMaxBBox_ExpandBySphere(NULL, &curChild->sphere, &bbox);
	}

	// convert it back to a sphere
	MUrMinMaxBBox_To_Sphere(&bbox, &ioTree->sphere);

	return;
}

void PHrSphereTree_Delete(PHtSphereTree *inTree)
{
	/***************
	* Recursively deletes a sphere tree
	*/
	if (!inTree) return;

	if (inTree->sibling)
	{
		PHrSphereTree_Delete(inTree->sibling);
		PHrSphereTree_Delete(inTree->child);
	}

	UUrMemory_Block_Delete(inTree);
}

static void PHiSphereTree_Draw(PHtSphereTree *tree)
{
	/**************
	* Draws a sphere tree. Sets up drawing and geometry states
	*/

	ONrDrawSphere(
		NULL,
		tree->sphere.radius,
		&tree->sphere.center);

	if (tree->child)
	{
		PHiSphereTree_Draw(tree->child);
	}

	if (tree->sibling)
	{
		PHiSphereTree_Draw(tree->sibling);
	}

	return;
}

void PHrSphereTree_Draw(PHtSphereTree *inTree)
{
	/**************
	* Draws a sphere tree. Sets up drawing and geometry states
	*/

	M3rGeom_State_Push();
		M3rGeom_State_Set(M3cGeomStateIntType_Fill, M3cGeomState_Fill_Line);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor,IMcShade_White);
		M3rGeom_State_Set(M3cGeomStateIntType_Appearance, M3cGeomState_Appearance_Gouraud);

		PHiSphereTree_Draw(inTree);
	M3rGeom_State_Pop();

	return;
}


void PHrPhysics_MaintainMatrix(PHtPhysicsContext *physics)
{
	M3tVector3D normal;
	M3tPoint3D *point;
	M3tPlaneEquation *plane;
	UUtUns32 i;

	// Apply all positional data
	MUmAssertVector(physics->position, 1e9f);

	switch (physics->level)
	{
		case PHcPhysics_Static:
		case PHcPhysics_Linear:
noanim:	case PHcPhysics_Newton:
		case PHcPhysics_None:
			MUrMatrix_BuildTranslate(physics->position.x, physics->position.y, physics->position.z, &physics->matrix);
			MUrMatrixStack_Quaternion(&physics->matrix, &(physics->orientation));
			MUrMatrixStack_Scale(&physics->matrix, physics->scale);
			break;

		case PHcPhysics_Animate:
			if (physics->animContext.animation)
			{
				if( !(physics->animContext.animContextFlags & OBcAnimContextFlags_CollisionPause) )
				{
					if( physics->animContext.animContextFlags & OBcAnimContextFlags_Slave )
					{
						UUmAssert( physics->master );
						physics->matrix = physics->master->matrix;
						OBrAnim_Matrix(&physics->master->animContext, &physics->matrix);
					}
					else
					{
						OBrAnim_Matrix(&physics->animContext, &physics->matrix);
					}

					if (physics->animContext.animation->animFlags & OBcAnimFlags_Localized)
					{
						MUrMatrix_Multiply(&physics->origin, &physics->matrix, &physics->matrix);
					}

					// FIXME: jtd - this is a hack for double doors
					if (physics->animContext.animContextFlags & OBcAnimContextFlags_RotateY180)
					{
						MUrMatrixStack_RotateY( &physics->matrix, M3cPi );
						MUrMatrixStack_RotateZ( &physics->matrix, M3cPi );
					}
				}
			}
			else goto noanim;
			break;

		default:
			UUmAssert(!"Unsupported physics level");
	}


	// Update bounding quads and plane equations
	if (physics->flags & PHcFlags_Physics_FaceCollision)
	{
		// Update bounding volume points
		MUrMatrix_MultiplyPoints(
			M3cNumBoundingPoints,
			&physics->matrix,
			physics->axisBox.localPoints,
			physics->worldAlignedBounds->worldPoints);

		// Update face planes and projections
		for (i=0; i<M3cNumBoundingFaces; i++)
		{
			MUrMatrix_MultiplyNormal(
				physics->worldAlignedBounds->normals + i,
				&physics->matrix,
				&normal);

			point = physics->worldAlignedBounds->worldPoints + physics->worldAlignedBounds->faces[i].indices[0];
			plane = physics->worldAlignedBounds->curPlanes + i;

			plane->a = normal.x;
			plane->b = normal.y;
			plane->c = normal.z;
			plane->d = -(normal.x*point->x + normal.y*point->y + normal.z*point->z);

			physics->worldAlignedBounds->curProjections[i] = CLrQuad_FindProjection(
				physics->worldAlignedBounds->worldPoints,
				physics->worldAlignedBounds->faces + i);

			// degenerate quads should be culled at import time
			UUmAssert(physics->worldAlignedBounds->curProjections[i] != CLcProjection_Unknown);
		}
	}
}


void PHrPhysics_Delete(PHtPhysicsContext *ioPhysics)
{
	/**********
	* Cleans up a physics context
	*/

	if (ioPhysics->callback->preDispose)
	{
		ioPhysics->callback->preDispose(ioPhysics);
	}

	if (ioPhysics->sphereTree)
	{
		PHrSphereTree_Delete(ioPhysics->sphereTree);
	}

	UUrMemory_Set8(ioPhysics, 0xff, sizeof(*ioPhysics));

	return;

}

void PHrPhysics_Init(PHtPhysicsContext *ioPhysics)
{
	/************
	* Allocates a physics context
	*/
	UUtUns32 sizeof_context = sizeof(PHtPhysicsContext);
	UUtUns32 sizeof_bounding_volume = sizeof(M3tBoundingVolume);

	UUmAssertWritePtr(ioPhysics, sizeof(*ioPhysics));

	UUrMemory_Clear(ioPhysics,sizeof(PHtPhysicsContext));

	ioPhysics->rotationalVelocity = MUgZeroQuat;
	ioPhysics->orientation = MUgZeroQuat;

	ioPhysics->gravity = UUcGravity;
	ioPhysics->mass = 1.0f;
	ioPhysics->scale = 1.0f;
	ioPhysics->flags = PHcFlags_Physics_InUse;

	MUrMatrix_Identity((M3tMatrix4x3 *)&ioPhysics->matrix);
	MUrMatrix_Identity((M3tMatrix4x3 *)&ioPhysics->origin);

	return;
}

static UUtError PHrStatus(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStall,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32 itr;
	UUtUns32 count = PHgPhysicsContextArray->num_contexts;

	for(itr = 0; itr < count; itr++)
	{
		PHtPhysicsContext *context = PHgPhysicsContextArray->contexts + itr;
		if (context->flags & PHcFlags_Physics_InUse) {
			char *collision_type = context->worldAlignedBounds ? " bbox" : " no-bbox";
			char *any_collision = (context->flags & PHcFlags_Physics_NoCollision) ? " no collision" : "";
			char *face_collision = (context->flags & PHcFlags_Physics_FaceCollision) ? " face-collsion" : " sphere-collision";
			char *type;

			switch(context->callback->type)
			{
				case PHcCallback_Generic: type = "generic"; break;
				case PHcCallback_Character: type = "character"; break;
				case PHcCallback_Object: type = "object"; break;
				case PHcCallback_Camera: type = "camera"; break;
				case PHcCallback_Ammo: type = "ammo"; break;
				case PHcCallback_Weapon: type = "weapon"; break;
				case PHcCallback_Particle: type = "particle"; break;
				case PHcCallback_Powerup: type = "powerup"; break;
				default: type = "unknown"; break;
			}

			COrConsole_Printf("%d %s (%2.2f,%2.2f,%2.2f)%s%s%s", itr, type, context->position.x, context->position.y, context->position.z, collision_type, any_collision, face_collision);
		}
	}

	return UUcError_None;
}


UUtError PHrPhysics_Initialize(void)
{
	// Game initialization
	SLrGlobalVariable_Register_Bool("ph_debug_keyforces",	"Toggle keyboard applied forces",		&PHgKeyForces);
	SLrGlobalVariable_Register_Bool("ph_active",			"enable physics",						&PHgPhysicsActive);
	SLrGlobalVariable_Register_Bool("ph_show_collisions",	"toggle display of colliding quads",	&PHgShowCollisions);
	SLrScript_Command_Register_Void("ph_status", "xxx", "",	PHrStatus);

	return UUcError_None;
}

void PHrPhysics_Terminate(void)
{
	//Game shutdown
}

void PHrPhysics_Callback_ReceiveForce( PHtPhysicsContext *ioContext, const PHtPhysicsContext *inPushingContext, const M3tVector3D *inForce)
{
	switch (ioContext->level)
	{
		case PHcPhysics_None:
		case PHcPhysics_Static:
		case PHcPhysics_Animate:
			break;

		case PHcPhysics_Linear:
		case PHcPhysics_Newton:
			PHrPhysics_Accelerate(ioContext, inForce);
			break;

		default:
			UUmAssert(!"Unsupported physics level");
	}

	return;
}

void PHrPhysics_Callback_FindEnvCollisions(
	const PHtPhysicsContext *inContext,
	const AKtEnvironment *inEnvironment,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders)
{
	M3tVector3D localVelocity;
	ONtCharacter *character;
	UUtUns16 itr;
	UUtUns32 collision_skipflag;

	UUmAssertReadPtr(inContext, sizeof(*inContext));
	UUmAssertReadPtr(inEnvironment, sizeof(*inEnvironment));
	UUmAssertReadPtr(inVelocity, sizeof(*inVelocity));
	UUmAssertWritePtr(outColliders, sizeof(*outColliders));
	UUmAssertWritePtr(ioNumColliders, sizeof(*ioNumColliders));
	MUmAssertVector(*inVelocity, 1e9f);
	UUmAssertReadPtr(inContext->sphereTree, sizeof(*inContext->sphereTree));
	UUmAssert(inContext->sphereTree->sphere.radius >= 0);

	localVelocity = *inVelocity;
	character = (ONtCharacter *) inContext->callbackData;

	collision_skipflag = (inContext->flags & PHcFlags_Physics_CharacterCollisionSkip) ? AKcGQ_Flag_Chr_Col_Skip : AKcGQ_Flag_Obj_Col_Skip;

	if (inContext->flags & PHcFlags_Physics_PointCollision)
	{
		AKrCollision_Point(
			inEnvironment,
			&inContext->sphereTree->sphere.center,
			&localVelocity,
			collision_skipflag,
			AKcMaxNumCollisions);
	}
	else
	{
		AKrCollision_Sphere(
			inEnvironment,
			&inContext->sphereTree->sphere,
			&localVelocity,
			collision_skipflag,
			AKcMaxNumCollisions);
	}

	for(itr = 0; itr < AKgNumCollisions; itr++)
	{
		AKtCollision *curCollision = AKgCollisionList + itr;
		AKtGQ_General *gqGeneral = inEnvironment->gqGeneralArray->gqGeneral + curCollision->gqIndex;
		AKtGQ_Collision *gqCollision = inEnvironment->gqCollisionArray->gqCollision + curCollision->gqIndex;
		UUtBool touched;

		touched = UUcTrue;

		if (touched)
		{
			M3tPlaneEquation plane;
			PHtCollider *newCollider = outColliders + *ioNumColliders;

			AKmPlaneEqu_GetComponents(
				gqCollision->planeEquIndex,
				inEnvironment->planeArray->planes,
				plane.a,
				plane.b,
				plane.c,
				plane.d);

			UUrMemory_Clear(newCollider, sizeof(*newCollider));

			newCollider->type = PHcCollider_Env;
			newCollider->data = gqGeneral;
			newCollider->plane = plane;
			newCollider->planePoint = curCollision->collisionPoint;

			MUmAssertVector(newCollider->planePoint, 1e9f);
			MUmAssertPlane(newCollider->plane, 1e9f);

			(*ioNumColliders)++;

			if (gqGeneral->flags & AKcGQ_Flag_2Sided)
			{
				PHtCollider *secondNewCollider = outColliders + *ioNumColliders;

				*secondNewCollider = *newCollider;

				secondNewCollider->plane.a *= -1.f;
				secondNewCollider->plane.b *= -1.f;
				secondNewCollider->plane.c *= -1.f;
				secondNewCollider->plane.d *= -1.f;

				(*ioNumColliders)++;
			}
		}
	}

	return;
}

UUtBool
PHrPhysics_Single_PhyCollision(
	const PHtPhysicsContext *inContext,
	PHtPhysicsContext *inTargetContext,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders)
{
	const PHtSphereTree *collideTree;
	UUtBool collide, useDiscardThreshold;
	M3tPlaneEquation *collisionFacePlane;
	M3tPoint3D collisionFacePoint;
	UUtUns16 oldNumColliders;

	if (!(inTargetContext->flags & PHcFlags_Physics_InUse))		return UUcFalse;
	if (!(inContext->flags & PHcFlags_Physics_InUse))			return UUcFalse;
	if (inTargetContext == inContext)							return UUcFalse;
	if (inTargetContext->flags & PHcFlags_Physics_DontBlock)	return UUcFalse;
	if (inContext->flags & PHcFlags_Physics_NoCollision)		return UUcFalse;
	if (inTargetContext->flags & PHcFlags_Physics_NoCollision)	return UUcFalse;

	oldNumColliders = *ioNumColliders;

	useDiscardThreshold = (inTargetContext->level == PHcPhysics_Animate) && (inContext->flags & PHcFlags_Physics_InitialCollisionPass);

	// test these two contexts to see if they collide
	if (inContext->flags & PHcFlags_Physics_PointCollision)
	{
		collide = PHrCollision_Point_SphereTree(
			&inContext->sphereTree->sphere.center,
			inVelocity,
			inTargetContext->sphereTree,
			NULL);
	}
	else if (inTargetContext->flags & PHcFlags_Physics_FaceCollision)
	{
		// optimization
		if (MUrPoint_Distance_Squared(
			&inContext->sphereTree->sphere.center,
			&inTargetContext->sphereTree->sphere.center) >
			UUmSQR(inContext->sphereTree->sphere.radius + inTargetContext->sphereTree->sphere.radius))
		{
			collide = UUcFalse;
		}
		else
		{
			collide = PHrCollision_Volume_SphereTree(
				inTargetContext->worldAlignedBounds,
				inVelocity,
				useDiscardThreshold,
				inContext->sphereTree,
				NULL,
				&collisionFacePlane,
				&collisionFacePoint,
				outColliders,
				ioNumColliders,
				inTargetContext);
		}
	}
	else
	{
		collideTree = PHrCollision_SphereTree_SphereTree(
			inContext->sphereTree,
			inVelocity,
			inTargetContext->sphereTree,
			NULL);
		collide = collideTree != NULL;
	}

	if (collide)
	{
		if (inTargetContext->flags & PHcFlags_Physics_FaceCollision)
		{
			// multiple colliders have already been created inside PHrCollision_Volume_SphereTree
		}
		else
		{
			PHtCollider *newCollider = outColliders + *ioNumColliders;
			M3tPlaneEquation plane;
			M3tPoint3D collisionPoint;
			M3tVector3D vectorToMe;
			M3tPoint3D myPoint, targetPoint;
			M3tVector3D temp;

			// calculate a point half way between the outer edges of the bounding circles
			// build a plane that correspon to that point and a normal pointing toward us
			MUmVector_Subtract(vectorToMe, inContext->sphereTree->sphere.center, inTargetContext->sphereTree->sphere.center);
			if (MUrVector_Normalize_ZeroTest(&vectorToMe)) {
				// the spheretrees are coincident, use an arbitrary normal and collision point
				collisionPoint = inContext->sphereTree->sphere.center;
				plane.a = 0;
				plane.b = 1.0f;
				plane.c = 0;
				plane.d = - collisionPoint.y;
			} else {
				// targetPoint = target_center + radius * vectorToMe
				temp = vectorToMe;
				MUmVector_Scale(temp, inTargetContext->sphereTree->sphere.radius);
				targetPoint = inTargetContext->sphereTree->sphere.center;
				MUmVector_Increment(targetPoint, temp);

				// myPoint = my_center + -radius * vectorToMe
				temp = vectorToMe;
				MUmVector_Scale(temp, -inContext->sphereTree->sphere.radius);
				myPoint = inContext->sphereTree->sphere.center;
				MUmVector_Increment(myPoint, temp);

				// average those two points
				MUmVector_Add(collisionPoint, targetPoint, myPoint);
				MUmVector_Scale(collisionPoint, 0.5f);

				// compute the plane
				plane.a = vectorToMe.x;
				plane.b = vectorToMe.y;
				plane.c = vectorToMe.z;

				// ax + by + cz + d = 0
				// d = -(ax + by + cz)
				plane.d = -(
					(plane.a * collisionPoint.x) +
					(plane.b * collisionPoint.y) +
					(plane.c * collisionPoint.z));
			}

			newCollider->type = PHcCollider_Phy;
			newCollider->data = inTargetContext;
			newCollider->plane = plane;
			newCollider->planePoint = collisionPoint;

			(*ioNumColliders) += 1;
		}

		if (inTargetContext->callback->type == PHcCallback_Object)
		{
			OBtObject *object = (OBtObject*) inTargetContext->callbackData;

			if (object->flags & OBcFlags_NotifyCollision) {
				// CB: we notify the object system of the collision(s) which may cause it to
				// stop moving if it is a door.     -- 18 october, 2000
				OBrNotifyPhysicsCollision(object, inContext, inVelocity, *ioNumColliders - oldNumColliders, outColliders + oldNumColliders);
			}
		}
	}

	return collide;
}

void
PHrPhysics_Callback_FindPhyCollisions(
	const PHtPhysicsContext *inContext,
	const M3tVector3D *inVelocity,
	PHtCollider *outColliders,
	UUtUns16 *ioNumColliders)
{
	UUtUns32 i;
	PHtCallback_SkipPhyCollisions skip_callback;

	skip_callback = inContext->callback->skipPhyCollisions;

	for (i = 0; i < PHgPhysicsContextArray->max_contexts; i++)
	{
		PHtPhysicsContext *targetContext;

		targetContext = PHgPhysicsContextArray->contexts + i;
		if (!(targetContext->flags & PHcFlags_Physics_InUse)) continue;
		if (targetContext == inContext) continue;
		if (targetContext->flags & PHcFlags_Physics_DontBlock) continue;
		if ((skip_callback != NULL) && (*skip_callback)(inContext, targetContext)) continue;

		PHrPhysics_Single_PhyCollision(
			inContext,
			targetContext,
			inVelocity,
			outColliders,
			ioNumColliders);
	}

	return;
}

void PHrPhysics_Accelerate(
	PHtPhysicsContext *physics,
	const M3tVector3D *inAcceleration)
{
	MUmVector_Increment(physics->acceleration, *inAcceleration);

	return;
}

void
PHrPhysics_Callback_Friction(
	PHtPhysicsContext *inContext,
	M3tVector3D *ioVelocity)
{
	float oldLength;
	float staticLength;
	float dynamicLength;
	float newLength;
	float scale;

	MUmAssertVector(*ioVelocity, 1e9f);

	switch (inContext->level)
	{
		case PHcPhysics_None:
		case PHcPhysics_Static:
		case PHcPhysics_Animate:
			return;

		case PHcPhysics_Linear:
		case PHcPhysics_Newton:
			break;

		default:
			UUmAssert(!"Unsupported physics type");
	}

	oldLength = MUmVector_GetLength(*ioVelocity);

	// if it is essentially zero, zero it
	if (UUmFloat_CompareEqu(oldLength, 0.f))
	{
		ioVelocity->x = 0.f;
		ioVelocity->y = 0.f;
		ioVelocity->z = 0.f;
	}
	else
	{
		staticLength = UUmMax(0.f, oldLength - inContext->staticFriction);
		dynamicLength = oldLength * inContext->dynamicFriction;
		newLength = UUmMin(staticLength, dynamicLength);
		scale = newLength / oldLength;

		MUmVector_Scale(*ioVelocity, scale);
	}

	MUmAssertVector(*ioVelocity, 1e9f);

	return;
}

UUtBool
PHrCollision_Quad_SphereTreeVector(
	const AKtEnvironment*		inEnvironment,
	UUtUns32					inGQIndex,
	const PHtSphereTree			*inTree,
	const M3tVector3D*			inVector,
	M3tPoint3D					*outIntersection)
{
	UUtBool collide;
	M3tPoint3D *intersection;

	if (inTree->child) {
		intersection = NULL;
	}
	else {
		intersection = outIntersection;
	}

	collide =
		AKrGQ_SphereVector(
			inEnvironment,
			inGQIndex,
			&inTree->sphere,
			inVector,
			intersection);

	if (collide && (NULL == inTree->child))
	{
		// we did collide, woo hoo!
	}
	else if (collide)
	{
		PHtSphereTree *child;

		for(child = inTree->child; child != NULL; child = child->sibling)
		{
			// try our children
			collide = PHrCollision_Quad_SphereTreeVector(
						inEnvironment,
						inGQIndex,
						child,
						inVector,
						outIntersection);

			if (collide) { break; }
		}
	}

	return collide;
}

void
PHrPhysics_Colliders_GetFromSphere(
	M3tBoundingSphere*		inSphere,
	M3tVector3D*			inVector,
	UUtUns16				*ioNumColliders,
	PHtCollider				*outColliders)
{
	UUtUns16 		itr;
	PHtSphereTree	sphereTree;
	UUtUns16		maxColliders;
	UUtUns16		curColliderIndex = 0;
	PHtCollider*	curCollider;

	UUmAssertReadPtr(PHgPhysicsContextArray, sizeof(*PHgPhysicsContextArray));
	UUmAssertReadPtr(inSphere, sizeof(*inSphere));
	UUmAssertReadPtr(ioNumColliders, sizeof(*ioNumColliders));
	UUmAssertReadPtr(outColliders, sizeof(*outColliders));

	maxColliders = *ioNumColliders;

	sphereTree.sphere = *inSphere;
	sphereTree.child = sphereTree.sibling = NULL;

	for(itr = 0; itr < PHgPhysicsContextArray->num_contexts; itr++)
	{
		PHtPhysicsContext *targetContext = PHgPhysicsContextArray->contexts + itr;

		if (!(targetContext->flags & PHcFlags_Physics_InUse)) continue;

		if (PHrCollision_SphereTree_SphereTree(&sphereTree, inVector, targetContext->sphereTree, NULL) != NULL)
		{
			// add this to colliders
			curCollider = outColliders + curColliderIndex++;

			curCollider->type = PHcCollider_Phy;
			curCollider->data = targetContext;
			curCollider->distanceSquared = MUmVector_GetDistanceSquared(sphereTree.sphere.center, targetContext->sphereTree->sphere.center);

			if(curColliderIndex >= maxColliders) break;
		}
	}

	if(curColliderIndex > 1)
	{
		qsort(outColliders, curColliderIndex, sizeof(PHtCollider), sort_collisions_callback);
	}

	*ioNumColliders = curColliderIndex;
}

