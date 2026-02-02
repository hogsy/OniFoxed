/*
	FILE:	BFW_Object.c

	AUTHOR:	Quinn Dunki, Michael Evans

	CREATED: 4/8/98

	PURPOSE: Object engine

	Copyright (c) Bungie Software 1998,2000

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Object.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"
#include "BFW_Akira.h"
#include "BFW_Console.h"
#include "BFW_AppUtilities.h"
#include "BFW_BitVector.h"
#include "BFW_Timer.h"
#include "BFW_Physics.h"
#include "Oni_Camera.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Character_Animation.h"

#define OBcMinPausedFrames		5

UUtBool OBgObjectsShowDebug = UUcFalse;

static void OBrUpdated_By_Physics(PHtPhysicsContext *inContext);
static void ONiObject_Callback_PreDispose(PHtPhysicsContext *ioContext);
static void ONiObject_Callback_ApplyForce(
	PHtPhysicsContext *ioContext,
	PHtCollider *inCollider);

#define OBmVelocityPin(a,b) ((float)fabs((a) - (b)) < 0.05f)

static UUtError OBrSetParticles(
	OBtObject *inObject,
	UUtBool inCreateParticles)
{
	UUtError error;

	if (inObject->particleArray == NULL)
		return UUcError_None;

	if (inCreateParticles) {
		if (inObject->flags & OBcFlags_ParticlesCreated) {
			return UUcError_None;
		}

		error = EPrArray_New(inObject->particleArray, EPcParticleOwner_Object, (void *) inObject,
							&inObject->physics->matrix, ONgGameState->gameTime, inObject->setup->objName);
		UUmError_ReturnOnError(error);

		inObject->flags |= OBcFlags_ParticlesCreated;
		return UUcError_None;
	} else {
		if ((inObject->flags & OBcFlags_ParticlesCreated) == 0) {
			return UUcError_None;
		}

		EPrArray_Delete(inObject->particleArray);

		inObject->flags &= ~OBcFlags_ParticlesCreated;
		return UUcError_None;
	}
}

void OBrSetAnimation(
	OBtObject *inObject,
	OBtAnimation *inAnim,
	UUtUns16 inStartFrame,
	UUtUns16 inEndFrame)
{
	/***********
	* Applies an animation to an object within the given frame range
	*/

	UUmAssert(inStartFrame != inEndFrame);

	if (inObject->setup)
	{
		inObject->physics->animContext.animation = inAnim;
		OBrAnim_Start(&inObject->physics->animContext);
		inObject->physics->level = PHcPhysics_Animate;

		inObject->physics->animContext.animationFrame = inStartFrame;
		inObject->physics->animContext.animationStop = inEndFrame;
	}

	return;
}


UUtError OBrSetAnimationByName(
	OBtObject *inObject,
	char *inAnimName)
{
	/***********
	* Applies a named animation to an object
	*/

	UUtError error;
	OBtAnimation *anim;

	// Load the named animation
	error = TMrInstance_GetDataPtr(
		OBcTemplate_Animation,
		inAnimName,
		&anim);

	if (anim) {
		OBrSetAnimation(inObject,anim,0,anim->numFrames);
	}
	else {
		error = UUcError_Generic;
	}

	return UUcError_None;
}

static void OBrObjectToBBox(OBtObject *inObject, M3tBoundingBox_MinMax *outBBox)
{
	UUtUns32 itr;

	for(itr = 0; itr < inObject->geometry_count; itr++)
	{
		M3tPoint3D center;
		float radius = inObject->geometry_list[itr]->pointArray->boundingSphere.radius;

		MUrMatrix_MultiplyPoint(&inObject->geometry_list[itr]->pointArray->boundingSphere.center, &inObject->physics->matrix, &center);

		if (0 == itr) {
			outBBox->minPoint.x = center.x - radius;
			outBBox->minPoint.y = center.y - radius;
			outBBox->minPoint.z = center.z - radius;

			outBBox->maxPoint.x = center.x + radius;
			outBBox->maxPoint.y = center.y + radius;
			outBBox->maxPoint.z = center.z + radius;
		}
		else {
			outBBox->minPoint.x = UUmMin(center.x - radius, outBBox->minPoint.x);
			outBBox->minPoint.y = UUmMin(center.y - radius, outBBox->minPoint.y);
			outBBox->minPoint.z = UUmMin(center.z - radius, outBBox->minPoint.z);

			outBBox->maxPoint.x = UUmMax(center.x + radius, outBBox->maxPoint.x);
			outBBox->maxPoint.y = UUmMax(center.y + radius, outBBox->maxPoint.y);
			outBBox->maxPoint.z = UUmMax(center.z + radius, outBBox->maxPoint.z);
		}
	}

	return;
}

static UUtBool OBiIsVisible(OBtObject *inObject)
{
	UUtBool object_is_visible = UUcFalse;
	static const float fov_fudge= (0.25f * M3cHalfPi);

	if (inObject->flags & OBcFlags_ForceDraw)
	{
		object_is_visible= UUcTrue;
	}
	else
	{
		M3tVector3D toObj;
		M3tBoundingBox_MinMax bbox;
		M3tPoint3D center;
		float distance_squared;
		M3tPoint3D camera_location;
		M3tPoint3D view_vector;
		float farP,fov;

		OBrObjectToBBox(inObject, &bbox);

		center.x = (bbox.minPoint.x + bbox.maxPoint.x) * 0.5f;
		center.y = (bbox.minPoint.y + bbox.maxPoint.y) * 0.5f;
		center.z = (bbox.minPoint.z + bbox.maxPoint.z) * 0.5f;

		M3rCamera_GetStaticData(ONgVisibilityCamera, &fov, NULL, NULL, &farP);
		M3rCamera_GetViewData(ONgVisibilityCamera, &camera_location, &view_vector, NULL);

		distance_squared = MUrPoint_Distance_Squared(&center, &camera_location);

		MUmVector_Subtract(toObj, center, camera_location);
		MUrNormalize(&toObj);

		if ((distance_squared > UUmSQR(90)) && (MUrAngleBetweenVectors3D(&toObj,&view_vector) > (fov + fov_fudge))) {
			object_is_visible = UUcFalse;
		}
		else if (distance_squared > UUmSQR(farP)) {
			object_is_visible = UUcFalse;
		}
		else
		{
			static const float tolerance= 0.001f;
			float dx, dy, dz;

			dx= inObject->last_position.x - center.x;
			dy= inObject->last_position.y - center.y;
			dz= inObject->last_position.z - center.z;
			dx*= dx;
			dy*= dy;
			dz*= dz;

			if ((dx>tolerance) || (dy>tolerance) || (dz>tolerance) || (inObject->oct_tree_node_index[0] == 0)) {
				AKrEnvironment_NodeList_Get(&bbox, OBcObjectNodeCount, inObject->oct_tree_node_index, 0);
				inObject->last_position= center;
			}

			object_is_visible= AKrEnvironment_NodeList_Visible(inObject->oct_tree_node_index);
		}
	}

	if (object_is_visible)
	{
		inObject->num_frames_offscreen = 0;
	}
	else
	{
		inObject->num_frames_offscreen++;
	}

	object_is_visible = inObject->num_frames_offscreen <= 5;

	return object_is_visible;
}


void OBrShow(OBtObject *inObject, UUtBool inShow)
{
	/**********
	* Shows and hides objects
	*/

	UUtBool object_has_collision = !(inObject->flags & OBcFlags_NoCollision);

	if (inShow) {
		inObject->flags &= ~OBcFlags_Invisible;

		if (object_has_collision && (NULL != inObject->physics)) {
			inObject->physics->flags &= ~PHcFlags_Physics_NoCollision;
		}
	}
	else {
		inObject->flags |= OBcFlags_Invisible;

		if (NULL != inObject->physics) {
			inObject->physics->flags |= PHcFlags_Physics_NoCollision;
		}
	}

	OBrSetParticles(inObject, inShow);

	return;
}

static void OBiCallback_PostCollision(
	PHtPhysicsContext *ioContext,
	const M3tVector3D *inVector,
	PHtCollider *ioColliderArray,
	UUtUns16 *ioColliderCount)
{
	/*************
	* Standard physics callback for objects
	*/

	UUtUns16 i;
	OBtObject *object = (OBtObject *)(ioContext->callbackData);
	PHtCollider *collider;
	ONtCharacter *victim;
	PHtPhysicsContext *otherContext;

	for (i=0; i<*ioColliderCount; i++)
	{
		collider = ioColliderArray + i;

		// Character effects
		if (collider->type == PHcCollider_Phy)
		{
			otherContext = (PHtPhysicsContext *)(collider->data);
			if (otherContext->callback->type == PHcCallback_Character)
			{
				victim = (ONtCharacter *)(otherContext->callbackData);
			}
		}
	}
}

static void OBiUpdate(OBtObject *ioObject)
{
	/******************
	* Updates an object for this frame
	*/


	// Track physics
	UUmAssert(ioObject->physics->sphereTree);
	if (ioObject->physics->animContext.animation) {
		ioObject->physics->sphereTree->sphere.center = MUrMatrix_GetTranslation(&ioObject->physics->matrix);
	}
	else {
		ioObject->physics->sphereTree->sphere.center = ioObject->physics->position;
	}

	return;
}

static void OBiDraw(OBtObject *inObject)
{
	UUtBool draw_object_bounding_boxes = UUcFalse;

	if (inObject->flags & OBcFlags_Invisible) {
		return;
	}

	if (OBgObjectsShowDebug)
	{
		UUtUns32 index = inObject->physics - PHgPhysicsContextArray->contexts;

		M3rGeom_State_Push();

		// Draw bounding spheres and boxes
		M3rGeom_State_Set(M3cGeomStateIntType_Fill, M3cGeomState_Fill_Line);
		M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor,IMcShade_White);
		M3rGeom_State_Set(M3cGeomStateIntType_Appearance, M3cGeomState_Appearance_Gouraud);

		if (inObject->physics->sphereTree) PHrSphereTree_Draw(inObject->physics->sphereTree);

		if (NULL != inObject->physics->worldAlignedBounds)
		{
			M3rBVolume_Draw_Line(inObject->physics->worldAlignedBounds, IMcShade_Green);
		}

		M3rGeom_State_Pop();
	}

	if (OBiIsVisible(inObject)) {
		UUtUns32 i;
		M3tMatrix4x3 *matrix = &inObject->physics->matrix;

		M3rMatrixStack_Push();
			M3rMatrixStack_Multiply(matrix);

			if (inObject->flags & OBcFlags_JelloObjects) {
				M3rGeom_State_Set(M3cGeomStateIntType_Alpha, 31);
				inObject->flags &= ~OBcFlags_JelloObjects;
			}
			else {
				M3rGeom_State_Set(M3cGeomStateIntType_Alpha, M3cMaxAlpha);
			}

			if (inObject->flags & OBcFlags_FlatLighting) {
				const static float shade_scale = 1.f / 255.f; // S.S.
				float r = shade_scale * ((inObject->flat_lighting_shade & 0xFF0000) >> 16);
				float g = shade_scale * ((inObject->flat_lighting_shade & 0xFF00) >> 8);
				float b = shade_scale * ((inObject->flat_lighting_shade & 0xFF) >> 0);

				ONrGameState_ConstantColorLighting(r, g, b);
			}

			M3rGeom_State_Commit();

			for( i = 0; i < inObject->geometry_count; i++ )
			{
				M3rGeometry_Draw(inObject->geometry_list[i]);
			}

			if (inObject->flags & OBcFlags_FlatLighting) {
				ONrGameState_SetupDefaultLighting();
			}

		M3rMatrixStack_Pop();
	}


	if (draw_object_bounding_boxes) {
		M3tBoundingBox_MinMax bbox;

		OBrObjectToBBox(inObject, &bbox);
		M3rMinMaxBBox_Draw_Line(&bbox, IMcShade_White);
	}
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================


OBtObjectList *OBrList_New(UUtUns16 inNumObjects)
{
	OBtObjectList *allocated;

	allocated = UUrMemory_Block_New(sizeof(OBtObjectList) + (inNumObjects - 1) * sizeof(OBtObject));
	if (NULL != allocated)
	{
		allocated->numObjects = 0;
		allocated->maxObjects = inNumObjects;
	}

	return allocated;
}

void OBrList_Delete(OBtObjectList *inList)
{
	UUtUns16 i;

	if (!inList) return;

	for (i=0; i<inList->numObjects; i++)
	{
		if (inList->object_list[i].flags & OBcFlags_InUse) {
			OBrDelete(inList->object_list + i);
		}
	}

	UUrMemory_Block_Delete(inList);

	return;
}

void OBrList_Update(
	OBtObjectList *ioObjectList)
{
	UUtUns16 itr;
	OBtObject *object;

	UUmAssertReadPtr(ioObjectList, sizeof(OBtObjectList));

	if (!ioObjectList->numObjects) return;

	// Update all the objects
	for(itr=0; itr < ioObjectList->numObjects; itr++)
	{
		object = &ioObjectList->object_list[itr];

		if (object->flags & OBcFlags_InUse) {
			OBiUpdate(object);
		}
	}
}

void OBrList_Draw(OBtObjectList *inObjectList)
{
	UUtStallTimer stall_timer;
	UUtUns16 itr;

	M3rGeom_State_Push();

	for(itr = 0; itr < inObjectList->numObjects; itr++)
	{
		OBtObject *object = inObjectList->object_list + itr;

		if (object->flags & OBcFlags_InUse) {
			UUrStallTimer_Begin(&stall_timer);
			OBiDraw(object);
			UUrStallTimer_End(&stall_timer, "OBrList_Draw -- single object");
		}
	}

	M3rGeom_State_Pop();

	return;
}

OBtObject *OBrList_Add(OBtObjectList *inObjectList)
{
	UUtUns32 itr;
	OBtObject *object = NULL;

	UUmAssertReadPtr(inObjectList, sizeof(OBtObjectList));

	for(itr = 0; itr < inObjectList->numObjects; itr++)
	{
		if (inObjectList->object_list[itr].flags & OBcFlags_InUse) {
			continue;
		}

		object = inObjectList->object_list + itr;
	}

	if (NULL == object) {
		if (inObjectList->numObjects < inObjectList->maxObjects) {
			object = inObjectList->object_list + inObjectList->numObjects;
			inObjectList->numObjects++;
		}
	}

	return object;
}

// ===========================

void OBrAnim_Reset(
	OBtAnimationContext *ioAnimContext)
{
	/*************
	* Resets our animation
	*/

	if ((ioAnimContext->animation != NULL) && (ioAnimContext->animation->animFlags & OBcAnimFlags_Random)) {
		ioAnimContext->animationFrame = UUmRandomRange(0,ioAnimContext->animation->numFrames);
	}
	else {
		ioAnimContext->animationFrame = 0;
	}

	ioAnimContext->numPausedFrames = 0;
	ioAnimContext->frameStartTime = 0;
	ioAnimContext->animationStep = 1;

	PHrPhysics_Unpause_Animation(ioAnimContext);
}

void OBrAnim_Stop(
	OBtAnimationContext *ioAnimContext)
{
	/************
	* Stop animating
	*/

	if (ioAnimContext->animationFrame >= ioAnimContext->animation->numFrames) {
		ioAnimContext->animationFrame = ioAnimContext->animation->numFrames-1;
	}

	if (ioAnimContext->animationFrame < 0) {
		ioAnimContext->animationFrame = 0;
	}

	ioAnimContext->animContextFlags &= ~OBcAnimContextFlags_Animate;
}

void OBrAnim_Start(
	OBtAnimationContext *ioAnimContext)
{
	/***************
	* Start animating
	*/

	ioAnimContext->animContextFlags |= OBcAnimContextFlags_Animate;

	OBrAnim_Reset(ioAnimContext);
}

void OBrAnim_Matrix(
	const OBtAnimationContext *ioAnimContext,
	M3tMatrix4x3 *outMatrix)
{
	OBrAnimation_GetMatrix(
		ioAnimContext->animation,
		ioAnimContext->animationFrame,
		ioAnimContext->animContextFlags,
		outMatrix);

	return;
}


void OBrAnimation_GetMatrix(
	OBtAnimation *inAnimation,
	UUtInt32 inFrame,
	UUtUns32 inAnimContextFlags,
	M3tMatrix4x3 *outMatrix)
{
	/*********
	* Extract matrix from animation & frame
	*/

	const char *animation_name = TMrInstance_GetInstanceName(inAnimation);
	UUtUns16 keyFrame;
	OBtAnimationKeyFrame *start;
	OBtAnimationKeyFrame *end;

	M3tQuaternion quat;
	M3tPoint3D translation;

	M3tMatrix4x3 rotateTM, translateTM, finalTM;

	UUmAssertReadPtr(inAnimation, sizeof(*inAnimation));
	UUmAssertWritePtr(outMatrix, sizeof(*outMatrix));

	// lookup frame, if frame is a keyframe, return it
	// otherwise genrate a matrix via lerp


	for(keyFrame = 0; keyFrame < inAnimation->numKeyFrames; keyFrame++) {
		OBtAnimationKeyFrame *current = inAnimation->keyFrames + keyFrame;

		if (current->frame == inFrame) {
			start = current;
			end = NULL;

			break;
		}
		else if (current->frame > inFrame) {
			start = inAnimation->keyFrames + keyFrame - 1;
			end = current;

			break;
		}
	}

	UUmAssert(start->frame <= inFrame);
	UUmAssert((NULL == end) || (end->frame > inFrame));

	if (NULL == end) {
		quat = start->quat;
		translation = start->translation;
	}
	else {
		float numFrames = (float) (end->frame - start->frame);
		float curFrame = (float) (inFrame - start->frame);
		float distance = curFrame / numFrames;

		UUmAssert(distance > 0);

		MUrPoint3D_Lerp(&start->translation, &end->translation, distance, &translation);
		MUrQuat_Lerp(&start->quat, &end->quat, distance, &quat);
	}


	// stretch * rotate * translate
	// (stretchRotateTM * stretchTM * invStretchRotateTM) * rotateTM * translateTM;
	MUrQuatToMatrix(&quat, &rotateTM);

	MUrMatrix_BuildTranslate(translation.x, translation.y, translation.z, &translateTM);

	MUrMatrix_Identity(&finalTM);
	MUrMatrixStack_Matrix(&finalTM, &translateTM);

	if (0 == (inAnimContextFlags & OBcAnimContextFlags_NoRotation))
		MUrMatrixStack_Matrix(&finalTM, &rotateTM);

	MUrMatrixStack_Matrix(&finalTM, &inAnimation->scale);

	UUrMemory_MoveFast(&finalTM, outMatrix, sizeof(M3tMatrix4x3));

	// For local animations, convert from Max coordinate system
	if (inAnimation->animFlags & OBcAnimFlags_Localized)
		MUrMatrix_FromMax(outMatrix);

//	if (inAnimContextFlags & OBcAnimContextFlags_RotateY180)
//		MUrMatrixStack_RotateY( outMatrix, M3cPi );

/*
	if (inAnimContextFlags & OBcAnimContextFlags_RotateY180)
	{
		M3tMatrix4x3		m1;
		MUrMatrix_Identity(&m1);
		MUrMatrix_BuildRotateZ( M3cPi, &m1);
		MUrMatrixStack_Matrix( outMatrix, &m1);
	}
*/
	return;
}

void OBrAnim_Position(
	OBtAnimationContext *ioObject,
	M3tPoint3D *outPosition)
{
	/*********
	* Calculate physical position from animation
	*/

	M3tMatrix4x3 matrix;

	OBrAnim_Matrix(ioObject, &matrix);

	*outPosition = MUrMatrix_GetTranslation(&matrix);

	return;
}

UUtError OBrRegisterTemplates(
	void)
{
	/****************
	* Register object system
	*/

	UUtError error;

	error = TMrTemplate_Register(OBcTemplate_ObjectArray,sizeof(OBtObjectSetupArray),TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(OBcTemplate_Animation,sizeof(OBtAnimation),TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error = TMrTemplate_Register(OBcTemplate_DoorClassArray,sizeof(OBtDoorClassArray),TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

void OBrDelete(
	OBtObject *ioObject)
{
	/*********
	* Deletes an object
	*/

	if ((ioObject->flags & OBcFlags_InUse) == 0) {
		UUmAssert(!"OBrDelete on a deleted object");
		return;
	}
	ioObject->flags &= ~OBcFlags_InUse;

	OBrSetParticles(ioObject, UUcFalse);

	PHrPhysicsContext_Remove(ioObject->physics);
	ioObject->physics = NULL;

	return;
}

static PHtSphereTree *OBiMakeSphereTree(
	OBtObject *ioObject,
	OBtObjectSetup *ioSetup)
{
	/**************
	* Constructs a best-fit sphere tree
	*/

	float best = -1e9f;
	UUtUns16 j;
	M3tPoint3D *p;
	PHtSphereTree *tree;

	UUtUns32		i;
	for( i = 0; i < ioObject->geometry_count; i++ )
	{
		for (j=0; j<ioObject->geometry_list[i]->pointArray->numPoints; j++)
		{
			p = ioObject->geometry_list[i]->pointArray->points + j;
			if (fabs(p->x - ioSetup->position.x) > best) best = (float)fabs(p->x);
			if (fabs(p->y - ioSetup->position.y) > best) best = (float)fabs(p->y);
			if (fabs(p->z - ioSetup->position.z) > best) best = (float)fabs(p->z);

		}
	}

	tree = PHrSphereTree_New(ioObject->sphere_tree_memory, NULL);

	tree->sphere.center = ioSetup->position;
	tree->sphere.radius = UUmMax(1.f, best);


	return tree;
}

UUtError OBrInit(
	OBtObject *ioObject,
	OBtObjectSetup *inSetup)
{
	/**********
	* Allocates an object
	*/
	static PHtPhysicsCallbacks callback;
	UUtUns16 i,j;
	PHtPhysicsContext *physics;
	M3tBoundingVolume *bounding_volume_memory = NULL;
	UUtError error;

	UUmAssertWritePtr(ioObject, sizeof(OBtObject));
	UUrMemory_Clear(ioObject, sizeof(OBtObject));

	// if we are going to need a bounding volume allocate it first
	if ((inSetup != NULL) && (inSetup->flags & OBcFlags_FaceCollision))
	{
		bounding_volume_memory = &ioObject->bounding_volume_memory;
	}


	ioObject->ignoreCharacterIndex = 0xffff;		// note make this a constant, in oni character ?
	ioObject->flat_lighting_shade = IMcShade_White;

	ioObject->allow_pause = NULL;
	ioObject->pausing_context = NULL;

	ioObject->physics = PHrPhysicsContext_Add();
	physics = ioObject->physics;
	PHrPhysics_Init(physics);
	OBrAnim_Reset(&physics->animContext);

	// Set up basic physics goo
	UUrMemory_Clear(&callback, sizeof(callback));
	callback.type= PHcCallback_Object;
	callback.findEnvCollisions = PHrPhysics_Callback_FindEnvCollisions;
	callback.findPhyCollisions = PHrPhysics_Callback_FindPhyCollisions;
	callback.skipPhyCollisions = NULL;
	callback.receiveForce = PHrPhysics_Callback_ReceiveForce;
	callback.friction = PHrPhysics_Callback_Friction;
	callback.update = OBrUpdated_By_Physics;
	callback.preDispose = ONiObject_Callback_PreDispose;
	callback.applyForce = ONiObject_Callback_ApplyForce;

	physics->callbackData = ioObject;
	physics->callback = &callback;
	physics->staticFriction = PHcDefaultStaticFriction;
	physics->dynamicFriction = PHcDefaultDynamicFriction;

	// Perform Setup setup
	if (inSetup)
	{
		M3tBoundingBox_MinMax	limits;

		PHtSphereTree *tree;

		ioObject->flags = (UUtUns16)inSetup->flags;
		ioObject->setup = inSetup;

		UUmAssert(inSetup->geometry_array->numGeometries <= OBcMaxGeometries);
		ioObject->geometry_count = UUmMin(inSetup->geometry_array->numGeometries, OBcMaxGeometries);

		for( i = 0; i < ioObject->geometry_count; i++ )
		{
			ioObject->geometry_list[i] = inSetup->geometry_array->geometries[i];

			if( i == 0 )
			{
				limits = ioObject->geometry_list[i]->pointArray->minmax_boundingBox;
			}
			else
			{
				limits.minPoint.x = UUmMin( ioObject->geometry_list[i]->pointArray->minmax_boundingBox.minPoint.x, limits.minPoint.x );
				limits.minPoint.y = UUmMin( ioObject->geometry_list[i]->pointArray->minmax_boundingBox.minPoint.y, limits.minPoint.y );
				limits.minPoint.z = UUmMin( ioObject->geometry_list[i]->pointArray->minmax_boundingBox.minPoint.z, limits.minPoint.z );

				limits.maxPoint.x = UUmMax( ioObject->geometry_list[i]->pointArray->minmax_boundingBox.maxPoint.x, limits.maxPoint.x );
				limits.maxPoint.y = UUmMax( ioObject->geometry_list[i]->pointArray->minmax_boundingBox.maxPoint.y, limits.maxPoint.y );
				limits.maxPoint.z = UUmMax( ioObject->geometry_list[i]->pointArray->minmax_boundingBox.maxPoint.z, limits.maxPoint.z );
			}

			if (ioObject->geometry_list[i]->triNormalArray->numVectors == 1)
			{
				M3tPlaneEquation	plane;
				M3tPoint3D			*point;
				M3tVector3D			*normal;
				const float			growAmt = 5.f;

				UUmAssert(ioObject->geometry_list[i]->pointArray != NULL);

				point	= ioObject->geometry_list[i]->pointArray->points;
				normal	= ioObject->geometry_list[i]->triNormalArray->vectors;

				plane.a = normal->x;
				plane.b = normal->y;
				plane.c = normal->z;
				plane.d = -(plane.a * point->x + plane.b * point->y + plane.c * point->z);

				M3rBBox_GrowFromPlane(&ioObject->physics->axisBox, &plane, growAmt);
			}
		}

		M3rMinMaxBBox_To_BBox(&limits, &physics->axisBox);
		// Set up the bounding volumes
		tree = OBiMakeSphereTree(ioObject,inSetup);

		if (ioObject->flags & OBcFlags_FaceCollision)
		{
			physics->worldAlignedBounds = bounding_volume_memory;

			M3rBBox_To_LocalBVolume(&ioObject->physics->axisBox, ioObject->physics->worldAlignedBounds);

			// Set up the plane equations for face collision
			for (j=0; j<M3cNumBoundingFaces; j++)
			{
				MUrVector_NormalFromPoints(
					physics->worldAlignedBounds->worldPoints + physics->worldAlignedBounds->faces[j].indices[0],
					physics->worldAlignedBounds->worldPoints + physics->worldAlignedBounds->faces[j].indices[1],
					physics->worldAlignedBounds->worldPoints + physics->worldAlignedBounds->faces[j].indices[2],
					physics->worldAlignedBounds->normals + j);
			}
		}
		// Set up other physics goo
		if (ioObject->flags & OBcFlags_FaceCollision) {
			ioObject->physics->flags |= PHcFlags_Physics_FaceCollision;
		}
		else if (ioObject->flags & OBcFlags_NoCollision) {
			ioObject->physics->flags |= PHcFlags_Physics_NoCollision;
		}

		if (ioObject->flags & OBcFlags_NoGravity) {
			ioObject->physics->flags |= PHcFlags_Physics_NoGravity;
		}

		physics->origin			= inSetup->debugOrigMatrix;
		physics->sphereTree		= tree;
		physics->orientation	= inSetup->orientation;
		physics->scale			= inSetup->scale;
		physics->position		= inSetup->position;

		// Set up animation
		physics->animContext.animation	= inSetup->animation;
		if (inSetup->animation)
		{
			physics->level		= PHcPhysics_Animate;
			physics->gravity	= 0.0f;
		}
		else {
			physics->level		= (PHtPhysicsLevel)inSetup->physicsLevel;
		}

		// Set up attached particles
		ioObject->particleArray = inSetup->particleArray;
		error = OBrSetParticles(ioObject, UUcTrue);
		UUmError_ReturnOnError(error);
	}

	PHrPhysics_MaintainMatrix(physics);

	return UUcError_None;
}

void
OBrConsole_Reset(
	UUtUns32			inArgC,
	char**				inArgV,
	void*				inRefcon)
{
	ONrGameState_ResetObjects();
}

static void OBrUpdated_By_Physics(PHtPhysicsContext *inContext)
{
	OBtObject *object = (OBtObject *) inContext->callbackData;

	UUmAssert(PHcCallback_Object == inContext->callback->type);

	// worldAxisBox = axisBox + inContext->position
//	MUmVector_Add(object->worldAxisBox.minPoint, object->axisBox.minPoint, inContext->position);
//	MUmVector_Add(object->worldAxisBox.maxPoint, object->axisBox.maxPoint, inContext->position);

	return;
}


static void ONiObject_Callback_PreDispose(PHtPhysicsContext *ioContext)
{
	ioContext->sphereTree = NULL;
	ioContext->worldAlignedBounds = NULL;

	return;
}

static void ONiObject_Callback_ApplyForce(
	PHtPhysicsContext *ioContext,
	PHtCollider *inCollider)
{
	OBtObject *object;

	UUmAssertReadPtr(ioContext, sizeof(*ioContext));
	UUmAssertReadPtr(inCollider, sizeof(*inCollider));
	UUmAssert(PHcCallback_Object == ioContext->callback->type);

	object = ioContext->callbackData;

	if (PHcCollider_Phy == inCollider->type)
	{
		PHtPhysicsContext *context = inCollider->data;

		if (PHcCallback_Character == context->callback->type)
		{
			ONtCharacter *character = context->callbackData;
		}
	}

	return;
}

UUtUns32 gNumObjectCollisionPoints = 0;
UUtUns32 gNextObjectCollisionPoint = 0;
M3tPoint3D gObjectCollisionPoints[1024];

void OBrNotifyPhysicsCollision(OBtObject *inObject, const PHtPhysicsContext *inContext, const M3tVector3D *inVelocity,
								UUtUns16 inNumColliders, PHtCollider *inColliders)
{
	UUtBool desire_pause = UUcFalse;

	if ((inObject->flags & OBcFlags_InUse) == 0)
		return;

	if (inNumColliders == 0)
		return;

	if ((inObject->physics != NULL) && (inObject->physics->level == PHcPhysics_Animate)) {
		OBtAnimationContext *anim_context = &inObject->physics->animContext;

		if (anim_context->animContextFlags & OBcAnimContextFlags_Animate) {
			UUtUns16 itr;
			UUtUns32 test_frame;
			UUtBool lookahead;
			PHtCollider *collider;
			float dot_product, delta_pos_sq, plane_t;
			M3tPoint3D prev_point, next_point;
			M3tVector3D delta_point, direction_of_motion;
			M3tMatrix4x3 current_matrix, other_matrix, inv_matrix, inv_origin_matrix, delta_matrix;

			// determine whether these collisions might make us want to stop our animation
			if (anim_context->animationFrame < anim_context->animationStep) {
				test_frame = anim_context->animationFrame + anim_context->animationStep;
				lookahead = UUcTrue;
			} else {
				test_frame = anim_context->animationFrame - anim_context->animationStep;
				lookahead = UUcFalse;
			}

			if (test_frame < anim_context->animation->numFrames) {
				// get the matrices for the current frame and a single step offset from the current frame
				OBrAnimation_GetMatrix(anim_context->animation, anim_context->animationFrame, anim_context->animContextFlags, &current_matrix);
				OBrAnimation_GetMatrix(anim_context->animation, test_frame, anim_context->animContextFlags, &other_matrix);

				// form a matrix that describes our movement in a single step
				MUrMatrix_Inverse(&current_matrix, &inv_matrix);
				MUrMatrix_Multiply(&other_matrix, &inv_matrix, &delta_matrix);

				if (anim_context->animation->animFlags & OBcAnimFlags_Localized) {
					// take account of the physics context's local coordinate frame as well
					MUrMatrix_Inverse(&inObject->physics->origin, &inv_origin_matrix);

					MUrMatrix_Multiply(&delta_matrix, &inv_origin_matrix, &delta_matrix);
					MUrMatrix_Multiply(&inObject->physics->origin, &delta_matrix, &delta_matrix);
				}

				for (itr = 0, collider = inColliders; itr < inNumColliders; itr++, collider++) {
					// work out in which direction the point on the object that was touched is moving
					if (lookahead) {
						// we are looking ahead to the next frame
						MUrMatrix_MultiplyPoint(&collider->planePoint, &delta_matrix, &next_point);
						MUmVector_Subtract(delta_point, next_point, collider->planePoint);
					} else {
						// we are looking back to the previous frame
						MUrMatrix_MultiplyPoint(&collider->planePoint, &delta_matrix, &prev_point);
						MUmVector_Subtract(delta_point, collider->planePoint, prev_point);
					}
					delta_pos_sq = MUmVector_GetLengthSquared(delta_point);
					if (delta_pos_sq < UUmSQR(1e-02f)) {
						// the point is not moving appreciably, this collision does not pause our animation
						continue;
					} else {
						delta_pos_sq = MUrOneOverSqrt(delta_pos_sq);
						MUmVector_ScaleCopy(direction_of_motion, delta_pos_sq, delta_point);
					}

					// store these points in the global collision point buffer
					gObjectCollisionPoints[gNextObjectCollisionPoint] = (lookahead) ? collider->planePoint : prev_point;
					gNextObjectCollisionPoint = (gNextObjectCollisionPoint + 1) % 1024;
					gObjectCollisionPoints[gNextObjectCollisionPoint] = (lookahead) ? next_point : collider->planePoint;
					gNextObjectCollisionPoint = (gNextObjectCollisionPoint + 1) % 1024;
					gNumObjectCollisionPoints = UUmMax(gNumObjectCollisionPoints, gNextObjectCollisionPoint);

					// compare this direction with the normal vector of the collision
					dot_product = collider->plane.a * direction_of_motion.x +
									collider->plane.b * direction_of_motion.y +
									collider->plane.c * direction_of_motion.z;
					if (dot_product < 0.5f) {
						// our direction of movement is away from the plane of the collision, we can continue moving
					} else {
						// our direction of movement is towards the plane of the collision
						MUmVector_Subtract(delta_point, collider->planePoint, inContext->position);
						plane_t = inContext->position.x * collider->plane.a +
								inContext->position.y * collider->plane.b +
								inContext->position.z * collider->plane.c + collider->plane.d;
						if (plane_t < 0) {
							// the physics context that we are colliding with lies behind the
							// plane of the collision, it is not actually blocking us (and it probably
							// shouldn't have generated this collision in the first place, but that's
							// a completely separate problem)
						} else {
							// we are blocked by this collision
							desire_pause = UUcTrue;
						}
					}
				}
			}
		}

		if (desire_pause) {
			if ((inObject->allow_pause != NULL) && (!(inObject->allow_pause)(inObject, inContext))) {
				// this object cannot be blocked by this kind of physics context
				return;
			}

			// we are colliding with an object, pause our animation
			anim_context->numPausedFrames = OBcMinPausedFrames;
			inObject->pausing_context = (PHtPhysicsContext *) inContext;
		}
	}
}

