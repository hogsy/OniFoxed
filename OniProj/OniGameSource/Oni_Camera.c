/*
	FILE:	BFW.h
	
	AUTHOR:	Michael Evans, Quinn Dunki
	
	CREATED: May 17, 1997
	
	PURPOSE: camera code
	
	Copyright 1997 - 1999

*/

#include "bfw_math.h"

#include <stdarg.h>

#include "BFW.h"
#include "Oni.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Camera.h"
#include "BFW_MathLib.h"
#include "BFW_Akira.h"
#include "BFW_AppUtilities.h"
#include "BFW_ScriptLang.h"

static UUtBool CArFollow_Update(UUtUns32 ticks, UUtUns32 inNumActions, const LItAction *actionBuffer);
static UUtBool CArManual_Update(UUtUns32 inTicks, UUtUns32 inNumActions, const LItAction *actionBuffer);
static UUtBool CArInterpolate_Update(UUtUns32 inTicks, UUtUns32 inNumActions, const LItAction *actionBuffer);

static void CArMatrix_To_ViewData(const M3tMatrix4x3 *inMatrix, CAtViewData *outViewData);

/************************ Globals **********************************/

CAtCamera	CAgCamera;
UUtBool		CAgPlaneTest = UUcTrue;
float		CAgHeight = 15.f;
float		CAgDistance = 33.f;
float		CAgCanter_Weapon = 7.f;
float		CAgCanter_Unarmed = 8.f;
float		CAgJello_Radius = 12.f;
UUtBool		CAgJello = UUcTrue;
UUtInt32	CAgJelloAmt = 20;

#define		CAcMaxJello		512
#define		CAcJelloAlpha	0x50
#define		CAcMaxJelloRays 5

UUtUns32	CAgNumJelloRays = 1;

/*********************** Housekeeping ******************************/

CAtCameraMode CArGetMode(void)
{
	return CAgCamera.mode;
}

void CArEnableJello(UUtBool inJello)
{
	CAgJello = inJello;
}

static UUtError
CAiJello(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	CArEnableJello(inParameterList[0].val.i > 0);
	return UUcError_None;
}

float M3rPlaneEquation_Evaluate(M3tPlaneEquation *inPlaneEquation, M3tPoint3D *inPoint)
{
	float result;
	
	result =inPlaneEquation->a * inPoint->x + inPlaneEquation->b * inPoint->y + inPlaneEquation->c * inPoint->z + inPlaneEquation->d;

	return result;
}

static void CAiTrackJello(CAtCamera *inCamera)
{
	UUtUns32 itr;
	UUtUns32 num_jello_collisions = 0;
	UUtUns32 rayIndex;
	AKtEnvironment *env = ONgGameState->level->environment;
	M3tVector3D toFocus,sideVector;
	M3tPoint3D focus;
	M3tPoint3D rayTip;
	UUtUns32 jello_indices[CAcMaxJello];
	float min_jello_location;

	AKtGQ_General *gq_general_array = env->gqGeneralArray->gqGeneral;
	AKtGQ_Render *gq_render_array = env->gqRenderArray->gqRender;
	AKtGQ_Collision *gq_collision_array = env->gqCollisionArray->gqCollision;
	M3tPlaneEquation *plane_array = env->planeArray->planes;

	typedef struct JelloRayOffset 
	{
		float x;
		float y;
	} JelloRayOffset;

	if (!CAgJello) {
		return;
	}
	
	MUrVector_CrossProduct(&inCamera->viewData.upVector,&inCamera->viewData.viewVector,&sideVector);
	sideVector.y+=2.0f;
	MUmVector_Scale(sideVector,3.0f);

	focus = inCamera->star->location;
	focus.y += ONcCharacterHeight - 3.f;

	min_jello_location = inCamera->star->feetLocation + 1.5f;

	CAgNumJelloRays = UUmPin(CAgNumJelloRays, 0, CAcMaxJelloRays);

	for (rayIndex=0; rayIndex < CAgNumJelloRays; rayIndex++)
	{	
		if (rayIndex == 1 || rayIndex == 3) {
			sideVector.y = -sideVector.y;
		}
		if (rayIndex == 2) {
			MUmVector_Negate(sideVector);
		}

		if (rayIndex < 4) {
			MUmVector_Add(rayTip, focus, sideVector);
		}
		else {
			rayTip = focus;
		}

		MUmVector_Subtract(toFocus, rayTip, inCamera->viewData.location);

		if (CAgJello_Radius > 0.f) {
			M3tBoundingSphere sphere;
			float length_to_focus = MUmVector_GetLength(toFocus);
			M3tVector3D jello_vector = toFocus;

			MUrVector_SetLength(&jello_vector, UUmMax(length_to_focus - CAgJello_Radius, 1.f));	

			sphere.center = inCamera->viewData.location;
			sphere.radius = CAgJello_Radius;

			AKrCollision_Sphere(env, &sphere, &jello_vector, AKcGQ_Flag_Jello_Skip, (UUtUns16) (CAcMaxJello - num_jello_collisions));
		}
		else {
			AKrCollision_Point(env, &inCamera->viewData.location, &toFocus, AKcGQ_Flag_Jello_Skip, (UUtUns16) (CAcMaxJello - num_jello_collisions));
		}

		for (itr=0; itr<AKgNumCollisions; itr++)
		{
			if (num_jello_collisions >= CAcMaxJello) { break; }

			jello_indices[num_jello_collisions] = AKgCollisionList[itr].gqIndex;
			num_jello_collisions++;
		}

		// raycast against the objects
		{
			UUtUns32 object_itr;
			UUtUns32 object_count = 0;

			for(object_itr = 0; object_itr < ONgGameState->objects->numObjects; object_itr++)
			{
				OBtObject *object = ONgGameState->objects->object_list + object_itr;
				if (object->flags & OBcFlags_InUse) {
					if (PHrCollision_Point(object->physics, &inCamera->viewData.location, &toFocus)) {
						object->flags |= OBcFlags_JelloObjects;
					}
				}
			}
		}
	}

	for(itr = 0; itr < num_jello_collisions; itr++)
	{
		AKtGQ_General *gq_general = gq_general_array + jello_indices[itr];
		AKtGQ_Render *gq_render = gq_render_array + jello_indices[itr];
		AKtGQ_Collision *gq_collision = gq_collision_array + jello_indices[itr];

		UUtBool jello_the_floor = UUcFalse;

		if (!jello_the_floor) {
			float max_y;

			if (gq_general->flags & AKcGQ_Flag_Triangle) {
				UUtUns32 tri_index_0 = gq_general->m3Quad.vertexIndices.indices[0];
				UUtUns32 tri_index_1 = gq_general->m3Quad.vertexIndices.indices[1];
				UUtUns32 tri_index_2 = gq_general->m3Quad.vertexIndices.indices[2];
				float tri_y0 = env->pointArray->points[tri_index_0].y;
				float tri_y1 = env->pointArray->points[tri_index_1].y;
				float tri_y2 = env->pointArray->points[tri_index_2].y;

				max_y = UUmMax3(tri_y0, tri_y1, tri_y2);
			}
			else {
				UUtUns32 quad_index_0 = gq_general->m3Quad.vertexIndices.indices[0];
				UUtUns32 quad_index_1 = gq_general->m3Quad.vertexIndices.indices[1];
				UUtUns32 quad_index_2 = gq_general->m3Quad.vertexIndices.indices[2];
				UUtUns32 quad_index_3 = gq_general->m3Quad.vertexIndices.indices[3];
				float quad_y0 = env->pointArray->points[quad_index_0].y;
				float quad_y1 = env->pointArray->points[quad_index_1].y;
				float quad_y2 = env->pointArray->points[quad_index_2].y;
				float quad_y3 = env->pointArray->points[quad_index_3].y;

				max_y = UUmMax4(quad_y0, quad_y1, quad_y2, quad_y3);
			}

			if (max_y < min_jello_location) { continue; }
		}


		if (CAgPlaneTest) {
			M3tPlaneEquation *plane_equation = plane_array + AKmPlaneEqu_GetIndex(gq_collision->planeEquIndex);
			UUtBool char_side_1_positive;
			UUtBool char_side_2_positive;
			UUtBool camera_side_positive;

			M3tPoint3D character_location_1 = inCamera->star->location;
			M3tPoint3D character_location_2 = inCamera->star->location;

			character_location_1.y = inCamera->star->feetLocation;
			character_location_2.y += ONcCharacterHeight;

			camera_side_positive = M3rPlaneEquation_Evaluate(plane_equation, &inCamera->viewData.location) > 0;
			char_side_1_positive = M3rPlaneEquation_Evaluate(plane_equation, &character_location_1) > 0;
			char_side_2_positive = M3rPlaneEquation_Evaluate(plane_equation, &character_location_2) > 0;

			if ((camera_side_positive & char_side_1_positive & char_side_1_positive) == (camera_side_positive | char_side_1_positive | char_side_2_positive)) 
			{
				continue;
			}
		}

		gq_general->flags |= AKcGQ_Flag_Jello;
		gq_render->alpha = (UUtUns16) CAgJelloAmt;
	}

	return;
}

// given a view vector, builds a trivial up vector
M3tVector3D CArBuildUpVector(const M3tVector3D *inViewVector)
{	
	M3tVector3D upVector;
	M3tVector3D sideVector;

	// Maintain up vector
	sideVector.x = inViewVector->z;
	sideVector.y = 0;
	sideVector.z = -inViewVector->x;
	MUrVector_CrossProduct(inViewVector,&sideVector,&upVector);
	
	// Keep things on the level
	MUrNormalize(&upVector);

	return upVector;
}

UUtError CArLevelBegin(void)
{
	ONgGameState->local.camera = &CAgCamera;
	UUrMemory_Clear(&CAgCamera, sizeof(CAgCamera));

	return UUcError_None;
}

void CArLevelEnd(
	void)
{
	
}


UUtError CArInitialize(void)
{
	UUtError error;

#if CONSOLE_DEBUGGING_COMMANDS
	SLrGlobalVariable_Register_Bool("cm_plane_test", "jello camera plane test", &CAgPlaneTest);
	SLrGlobalVariable_Register_Float("cm_height", "camera height", &CAgHeight);
	SLrGlobalVariable_Register_Float("cm_distance", "camera distance", &CAgDistance);
	SLrGlobalVariable_Register_Float("cm_canter_weapon", "camera canter", &CAgCanter_Weapon);
	SLrGlobalVariable_Register_Float("cm_canter_unarmed", "camera canter", &CAgCanter_Unarmed);
	SLrGlobalVariable_Register_Float("cm_jello_radius", "bla bla", &CAgJello_Radius);
	SLrGlobalVariable_Register_Int32("cm_jello_amt", "bla bla", &CAgJelloAmt);
#endif

	error = SLrScript_Command_Register_Void(
			"cm_jello",
			"toggles camera Jello(tm) mode",
			"mode:int{0 | 1 }",
			CAiJello);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

void CArTerminate(void)
{
}

static void CArViewData_GetMatrix(const CAtViewData *inViewData, M3tMatrix4x3 *outMatrix)
{
	M3tVector3D cross_product;
	CAtViewData testViewData;
	M3tVector3D viewVector;
	M3tVector3D upVector;

	viewVector.x = -inViewData->viewVector.x;
	viewVector.y = -inViewData->viewVector.y;
	viewVector.z = -inViewData->viewVector.z;

	upVector.x = inViewData->upVector.x;
	upVector.y = inViewData->upVector.y;
	upVector.z = inViewData->upVector.z;

	MUrVector_CrossProduct(&upVector, &viewVector, &cross_product);

	outMatrix->m[0][0] = cross_product.x;
	outMatrix->m[0][1] = cross_product.y;
	outMatrix->m[0][2] = cross_product.z;

	outMatrix->m[1][0] = upVector.x;
	outMatrix->m[1][1] = upVector.y;
	outMatrix->m[1][2] = upVector.z;

	outMatrix->m[2][0] = viewVector.x;
	outMatrix->m[2][1] = viewVector.y;
	outMatrix->m[2][2] = viewVector.z;

	outMatrix->m[3][0] = inViewData->location.x;
	outMatrix->m[3][1] = inViewData->location.y;
	outMatrix->m[3][2] = inViewData->location.z;

	CArMatrix_To_ViewData(outMatrix, &testViewData);

	return;
}

void CArGetMatrix(const CAtCamera *inCamera, M3tMatrix4x3 *outMatrix)
{
	CArViewData_GetMatrix(&inCamera->viewData, outMatrix);
}


static void CAiRecordCamera(const CAtViewData *inViewData)
{
	M3tMatrix4x3 matrix;
	AXtHeader *pHeader = ONgGameState->local.cameraRecording;
	AXtNode *pNode;

	CArViewData_GetMatrix(inViewData, &matrix);

	UUmAssert(NULL != pHeader);
	pNode = pHeader->nodes + 0;

	pNode->matricies = UUrMemory_Block_Realloc(pNode->matricies, sizeof(M3tMatrix4x3) * (pHeader->numFrames + 1));
	pNode->matricies[pHeader->numFrames] = matrix;

	pHeader->numFrames += 1;

	return;
}

static void CAiActivateCamera(CAtCamera *inCamera)
{
	CAtViewData buffer;
	CAtViewData *pViewData = &inCamera->viewData;
	ONtGameState *gameState = ONgGameState;

	if (ONgGameState->local.cameraPlay) {
		UUtUns32 frame_number = ONgGameState->local.cameraFrame;
		UUtUns32 max_frame_number = ONgGameState->local.cameraRecording->numFrames;

		if (frame_number >= max_frame_number) {
			COrConsole_Printf("camera %d/%d", frame_number, max_frame_number);

			frame_number = max_frame_number - 1;
		}
		CArMatrix_To_ViewData(ONgGameState->local.cameraRecording->nodes[0].matricies + frame_number, &buffer);
		pViewData = &buffer;
	}

	if (ONgGameState->local.cameraRecord) {
		CAiRecordCamera(pViewData);
	}

	M3rCamera_SetViewData(
		ONgActiveCamera,
		&pViewData->location,
		&pViewData->viewVector,
		&pViewData->upVector);

	return;
}

static UUtBool CArOrbit_Update(UUtUns32 ticks, UUtUns32 inNumActions, const LItAction *actionBuffer)
{
	UUtBool activate = UUcFalse;
	ONtActiveCharacter *active_character;

	if (ticks > 0) 
	{
		CAtCamera *camera = ONgGameState->local.camera;
		CAtOrbitData *orbitData = &camera->modeData.orbit;
		float old_angle = orbitData->current_angle;
		float stop_angle = orbitData->stop_angle;
		float delta_angle = UUmPin(orbitData->speed * ticks, -M3c2Pi, M3c2Pi);

		orbitData->current_angle += delta_angle;

		if (orbitData->stop_angle >= 0) {
			UUtBool stop = UUcFalse;
			float new_angle = orbitData->current_angle;
			
			stop |= (old_angle <= stop_angle) & (new_angle >= stop_angle);
			stop |= (old_angle >= stop_angle) & (new_angle <= stop_angle);
			stop |= (old_angle <= (stop_angle + M3c2Pi)) & (new_angle >= (stop_angle + M3c2Pi));
			stop |= (old_angle >= (stop_angle + M3c2Pi)) & (new_angle <= (stop_angle + M3c2Pi));
			stop |= (old_angle <= (stop_angle - M3c2Pi)) & (new_angle >= (stop_angle - M3c2Pi));
			stop |= (old_angle >= (stop_angle - M3c2Pi)) & (new_angle <= (stop_angle - M3c2Pi));

			if (stop) {
				orbitData->current_angle = stop_angle;
			}
		}

		UUmTrig_Clip(orbitData->current_angle);

		camera->viewData.viewVector.x = 0.f;
		camera->viewData.viewVector.y = 0.f;
		camera->viewData.viewVector.z = 1.f;

		{
			float use_angle = orbitData->current_angle;

			active_character = ONrGetActiveCharacter(camera->star);
			if (active_character != NULL) {
				use_angle += active_character->aimingLR;
				UUmTrig_Clip(use_angle);
			}

			use_angle += ONrCharacter_GetCosmeticFacing(camera->star);
			UUmTrig_Clip(use_angle);

			MUrPoint_RotateYAxis(&camera->viewData.viewVector, use_angle, &camera->viewData.viewVector);
			MUmVector_Scale(camera->viewData.viewVector, CAgDistance);
			camera->viewData.viewVector.y -= CAgCanter_Unarmed;
		}

		MUmVector_Subtract(camera->viewData.location, camera->star->location, camera->viewData.viewVector);
		camera->viewData.location.y += CAgHeight;
		MUrNormalize(&camera->viewData.viewVector);

		camera->viewData.upVector = CArBuildUpVector(&camera->viewData.viewVector);

		activate = UUcTrue;
	}

	return activate;
}


static UUtBool CArAnimate_Update(UUtUns32 ticks, UUtUns32 inNumActions, const LItAction *actionBuffer)
{
	UUtBool activate = UUcFalse;

	if (ticks > 0) 
	{
		CAtCamera *camera = ONgGameState->local.camera;
		CAtAnimateData *modeData = &camera->modeData.animate;
		M3tMatrix4x3 matrix;
		UUtUns32 itr;

		for(itr = 0; itr < ticks; itr++) {
			PHrPhysics_Update_Animation(&modeData->animation);
		}

		OBrAnim_Matrix(&modeData->animation, &matrix);

		// get location
		camera->viewData.location = MUrMatrix_GetTranslation(&matrix);

		// get focus
		{
			M3tPoint3D viewVector = { 0.0f, 0.0f, -1.0f };
			M3tPoint3D upVector = { 0.f, 1.f, 0.f };

			MUrMatrix_MultiplyNormal(&viewVector, &matrix, &camera->viewData.viewVector);
			MUrMatrix_MultiplyNormal(&upVector, &matrix, &camera->viewData.upVector);
		}

		camera->is_busy = modeData->animation.animationFrame < (modeData->animation.animation->numFrames - 1);
		activate = UUcTrue;
	}

	return activate;
}



static void CArUpdate_Internal(UUtUns32 ticks, UUtUns32 numActions, const LItAction *actionBuffer)
{
	/**********
	* Performs camera maintenance for a frame
	*/
	
	CAtCamera *camera = ONgGameState->local.camera;
	UUtBool activate = UUcFalse;

	M3rGeom_Clear_Jello();

	switch(camera->mode)
	{
		case CAcMode_Orbit:
			activate = CArOrbit_Update(ticks, numActions, actionBuffer);
		break;

		case CAcMode_Follow:
			activate = CArFollow_Update(ticks, numActions, actionBuffer);
		break;

		case CAcMode_Animate:
			activate = CArAnimate_Update(ticks, numActions, actionBuffer);
		break;

		case CAcMode_Manual:
			activate = CArManual_Update(ticks, numActions, actionBuffer);
		break;

		case CAcMode_Interpolate:
			activate = CArInterpolate_Update(ticks, numActions, actionBuffer);
		break;

		default:
			UUmAssert(!"blam");
	}
	
	// 11) notify the geometry context
	if (activate) {
		CAiActivateCamera(camera);
	}
	
	return;
}


void CArUpdate(UUtUns32 ticks, UUtUns32 numActions, const LItAction *actionBuffer)
{
	if (ONgGameState->local.cameraRecord) {
		UUtUns32 itr;

		for(itr = 0; itr < ticks; itr++) {
			CArUpdate_Internal(1, 0, NULL);
		}

		for(itr = 0; itr < numActions; itr++) {
			CArUpdate_Internal(0, 1, actionBuffer + itr);
		}
	}
	else {
		CArUpdate_Internal(ticks, numActions, actionBuffer);
	}
}

void CArOrbit_Enter(float inSpeed, float inStopAngle)
{
	CAtCamera *camera = ONgGameState->local.camera;
	CAtOrbitData *orbitData = &camera->modeData.orbit;

	UUrMemory_Clear(orbitData, sizeof(*orbitData));

	orbitData->speed = inSpeed;
	orbitData->stop_angle = inStopAngle;

	camera->mode = CAcMode_Orbit;
	camera->is_busy = UUcFalse;
	
	return;
}

static M3tPoint3D CArFollow_Get_GoalFocus(CAtCamera *inCamera)
{
	M3tPoint3D goal_focus;

	goal_focus = inCamera->star->location;
	goal_focus.y += CAgHeight;

	return goal_focus;
}

static M3tPoint3D CArFollow_Get_GoalPosition(CAtCamera *inCamera)
{
	ONtActiveCharacter *active_character = ONrGetActiveCharacter(inCamera->star);
	M3tPoint3D goal_position;

	if (active_character == NULL) {
		MUmVector_Set(goal_position, 0, 0, 0);
	} 
	else {
		M3tVector3D camera_vector = active_character->cameraVector;

		if (active_character->aimingUD > 0.9f) {
			float distance_to_up = (active_character->aimingUD - 0.9f) * (M3cHalfPi - 0.9f);
			M3tVector3D straigt_up_vector = { 0.f, 1.f, 0.f };

			distance_to_up = UUmPin(distance_to_up, 0.f, 1.f);

			MUrPoint3D_Lerp(&camera_vector, &straigt_up_vector, distance_to_up, &camera_vector);
			MUrNormalize(&camera_vector);
		}

		// aiming vector cannot be zero
		UUmAssert(!MUrVector_IsEqual(&camera_vector, &MUgZeroVector));

		goal_position = camera_vector;
		MUmVector_Scale(goal_position, -inCamera->modeData.follow.distance);
	}
	MUmVector_Increment(goal_position, inCamera->star->location);

	goal_position.y += CAgHeight + inCamera->modeData.follow.canter;

	// don't ever put the camera below my feet
	goal_position.y = UUmMax(goal_position.y, inCamera->star->location.y + 0.5f);

	return goal_position;
}

static void CArFollow_Set(M3tPoint3D *inLocation, M3tPoint3D *inFocus)
{
	CAtCamera *camera = ONgGameState->local.camera;

	camera->viewData.location = *inLocation;

	MUmVector_Subtract(camera->viewData.viewVector, *inFocus, *inLocation);
	MUrNormalize(&camera->viewData.viewVector);

	camera->viewData.upVector = CArBuildUpVector(&camera->viewData.viewVector);

	return;
}

static M3tPoint3D CArFollow_GetCurrentFocus(CAtCamera *inCamera)
{
	M3tPoint3D focus;

	// focus = inCamera->viewData.location + inCamera->viewData.viewVector * CAgDistance;
	focus = inCamera->viewData.viewVector;
	MUmVector_Scale(focus, inCamera->modeData.follow.distance);
	MUmVector_Increment(focus, inCamera->viewData.location);

	return focus;
}

static float CArFollow_Calculate_Delay(M3tVector3D *inVector, float baseDelay, float distanceScale, float scale)
{
	float delay;

	delay = MUmVector_GetLengthSquared(*inVector);
	delay *= distanceScale;
	delay = UUmMax(delay, 1.0f);
	delay = (float) log(delay);
	delay = baseDelay - (scale * 0.5f * delay);
	delay = UUmMax(delay, 2);

	return delay;
}

static void CArFollow_Interpolate_OnePoint_OneTick(M3tPoint3D *ioPoint, const M3tPoint3D *inGoal, float baseDelay, float distanceScale, float scale)
{
	float delay;
	M3tPoint3D vector_to_target;

	MUmVector_Subtract(vector_to_target, *inGoal, *ioPoint);
	delay = CArFollow_Calculate_Delay(&vector_to_target, baseDelay, distanceScale, scale);

	// ioPoint = ioPoint + (vector_to_target / delay)
	MUmVector_Scale(vector_to_target, (1.f / delay));
	MUmVector_Increment(*ioPoint, vector_to_target);

	return;
}

static void CArFollow_Interpolate_OneTick(M3tPoint3D *ioPosition, M3tPoint3D *ioFocus, const M3tPoint3D *inGoalPosition, const M3tPoint3D *inGoalFocus)
{
	CArFollow_Interpolate_OnePoint_OneTick(ioPosition, inGoalPosition, 8.f, 1.f, 0.85f);
	CArFollow_Interpolate_OnePoint_OneTick(ioFocus, inGoalFocus, 8.f, 3.f, 0.85f);

	return;
}

static UUtBool CArFollow_Update(UUtUns32 ticks, UUtUns32 inNumActions, const LItAction *actionBuffer)
{
	UUtBool activate = UUcFalse;
	ONtActiveCharacter *active_character;
	CAtCamera *camera = ONgGameState->local.camera;

	if (ticks > 0) {
		CAtFollowData *followData = &camera->modeData.follow;
		M3tPoint3D goal_position;
		M3tPoint3D goal_focus;

		active_character = ONrGetActiveCharacter(camera->star);

		if (active_character != NULL) {
			UUtBool aiming_weapon = ((camera->star->inventory.weapons[0] != NULL) && ((active_character->animVarient & ONcAnimVarientMask_Any_Weapon) > 0));

			// update the canter
			{
				float canter = followData->canter;
				float desired_canter = (aiming_weapon) ? CAgCanter_Weapon : CAgCanter_Unarmed;
				float canter_delta = 0.12f;

				// move the canter to zero as we get close to the ud ends of the aiming arc
				{
					float aiming_ud_full_canter = 0.5f;
					float aiming_ud_zero_canter = 1.2f;
					float aiming_ud = UUmMin(aiming_ud_zero_canter, active_character->aimingUD);

					if (aiming_ud > aiming_ud_full_canter) {
						float percent_to_zero_canter =  (aiming_ud - aiming_ud_full_canter) / (aiming_ud_zero_canter - aiming_ud_full_canter);

						desired_canter *= aiming_ud_zero_canter - percent_to_zero_canter;
					}
					else if (aiming_weapon && (aiming_ud < -aiming_ud_full_canter)) {
						float percent_to_double_canter = (-aiming_ud - aiming_ud_full_canter) / (aiming_ud_zero_canter - aiming_ud_full_canter);

						desired_canter *= 1.f + percent_to_double_canter;
					}
				}
				
				if (canter < desired_canter) {
					canter = UUmMin(desired_canter, canter + canter_delta * ticks);
				}
				else {
					canter = UUmMax(desired_canter, canter - canter_delta * ticks);
				}

				followData->canter = canter;
			}

			// update the distance

			{
				if (!aiming_weapon) {
					followData->distance = CAgDistance;
				}
				else {
					if (active_character->aimingUD > -0.5f) {
						followData->distance = CAgDistance;
					}
					else {
						followData->distance = CAgDistance * (0.5f + UUmMax(0.f, 1.f + active_character->aimingUD));
					}
				}
			}
		}

		// get the goal position
		goal_position = CArFollow_Get_GoalPosition(camera);
		goal_focus = CArFollow_Get_GoalFocus(camera);

		if (followData->rigid) {
			CArFollow_Set(&goal_position, &goal_focus);
		}
		else {
			M3tPoint3D current_position = camera->viewData.location;
			M3tPoint3D current_focus = CArFollow_GetCurrentFocus(camera);
			UUtUns32 itr;

			for(itr = 0; itr < ticks; itr++) {
				CArFollow_Interpolate_OneTick(&current_position, &current_focus, &goal_position, &goal_focus);
			}

			CArFollow_Set(&current_position, &current_focus);
		}

		activate = UUcTrue;
	}

	CAiTrackJello(camera);

	return activate;
}

void CArFollow_Enter(void)
{
	CAtCamera *camera = ONgGameState->local.camera;
	CAtFollowData *followData = &camera->modeData.follow;

	UUrMemory_Clear(followData, sizeof(*followData));
	camera->modeData.follow.canter = (NULL == camera->star->inventory.weapons[0]) ? CAgCanter_Unarmed : CAgCanter_Weapon;
	camera->modeData.follow.distance = CAgDistance;
	camera->mode = CAcMode_Follow;

	{
		M3tPoint3D goal_focus = CArFollow_Get_GoalFocus(camera);
		M3tPoint3D goal_position = CArFollow_Get_GoalPosition(camera);

		CArFollow_Set(&goal_position, &goal_focus);
	}

	
	camera->is_busy = UUcFalse;
	

	return;
}

void CArAnimate_Enter(OBtAnimation *inAnimation)
{
	CAtCamera *camera = ONgGameState->local.camera;
	CAtAnimateData *animateData = &camera->modeData.animate;

	UUrMemory_Clear(animateData, sizeof(*animateData));

	animateData->animation.animation = inAnimation;
	OBrAnim_Reset(&animateData->animation);
	OBrAnim_Start(&animateData->animation);

	camera->mode = CAcMode_Animate;
	camera->is_busy = UUcTrue;

	return;
}

/*
 *
 * MANUAL CAMERA
 *
 */

void CArManual_Enter(void)
{
	CAtCamera *camera = ONgGameState->local.camera;
	CAtManualData *manualData = &camera->modeData.manual;

	UUrMemory_Clear(manualData, sizeof(*manualData));

	camera->mode = CAcMode_Manual;
	camera->is_busy = UUcFalse;
	
	return;
}

void CArManual_LookAt(M3tPoint3D *inLocation, float	inDistance)
{
	CAtCamera			*camera = ONgGameState->local.camera;
	M3tVector3D			vector;
	
	if (camera->mode != CAcMode_Manual) { return; }
	
	vector = camera->viewData.viewVector;
	MUmVector_Negate(vector);
	MUmVector_Scale(vector, inDistance);
	MUmVector_Increment(vector, *inLocation);
	
	camera->viewData.location = vector;
	camera->viewData.upVector = CArBuildUpVector(&camera->viewData.viewVector);

	return;
}

static UUtBool CArManual_Update(UUtUns32 inTicks, UUtUns32 inNumActions, const LItAction *actionBuffer)
{
	UUtBool activate = UUcFalse;

	if (inNumActions > 0) 
	{
		CAtCamera *camera = ONgGameState->local.camera;
		CAtManualData *manualData = &camera->modeData.manual;
		UUtUns32 itr;

		for(itr = 0; itr < inNumActions; itr++)
		{
			const LItAction *action = actionBuffer + itr;

			float moveFB;
			float moveUD;
			float stepLR;

			float panLR;
			float panUD;

			M3tVector3D sideVector;

			// input
			moveFB = action->analogValues[LIc_ManCam_Move_FB] * ONgGameState_InputAccel;
			stepLR = action->analogValues[LIc_ManCam_Move_LR] * ONgGameState_InputAccel;
			moveUD = action->analogValues[LIc_ManCam_Move_UD] * 0.5f * ONgGameState_InputAccel;

			panLR = action->analogValues[LIc_ManCam_Pan_LR] * -(1.0f / 60.0f) * M3c2Pi;
			panUD = action->analogValues[LIc_ManCam_Pan_UD] * -(1.0f / 60.0f) * M3c2Pi;

			if (manualData->useMouseLook) {
				panLR += action->analogValues[LIc_Aim_LR] * -(1.0f / 60.0f) /* * M3c2Pi */;
				panUD += action->analogValues[LIc_Aim_UD] * -(1.0f / 60.0f) /* * M3c2Pi */;
			}

			panLR = UUmPin(panLR, -M3cPi, +M3cPi);
			panUD = UUmPin(panUD, -M3cPi, +M3cPi);

			MUrVector_CrossProduct(&camera->viewData.upVector,&camera->viewData.viewVector,&sideVector);
			MUmVector_Normalize(sideVector);

			camera->viewData.location.x += sideVector.x * stepLR;
			camera->viewData.location.x += camera->viewData.viewVector.x * moveFB;
				
			camera->viewData.location.y += action->analogValues[LIc_ManCam_Move_UD];

			if (manualData->useMouseLook)
			{
				camera->viewData.location.y += camera->viewData.viewVector.y * moveFB;
			}
				
			camera->viewData.location.z += sideVector.z * (stepLR);		
			camera->viewData.location.z += camera->viewData.viewVector.z * (moveFB);
			
			UUmTrig_ClipLow(panUD);
			UUmTrig_ClipLow(panLR);
			
			MUrPoint_RotateAboutAxis(
				&camera->viewData.viewVector,
				&camera->viewData.upVector,
				panLR,
				&camera->viewData.viewVector);
				
			MUrPoint_RotateAboutAxis(
				&camera->viewData.viewVector,
				&sideVector,
				panUD,
				&camera->viewData.viewVector);

			MUmVector_Normalize(camera->viewData.viewVector);
			
			camera->viewData.upVector = CArBuildUpVector(&camera->viewData.viewVector);
		}

		activate = UUcTrue;
	}

	return activate;
}

/*
 *
 * Interpolate Camera
 *
 */

void CArInterpolate_Enter_Matrix(const M3tMatrix4x3 *inMatrix, UUtUns32 inNumFrames)
{
	CAtCamera *camera = ONgGameState->local.camera;
	CAtInterpolateData *interpolateData = &camera->modeData.interpolate;

	CArMatrix_To_ViewData(inMatrix, &interpolateData->ending);
	interpolateData->initial = camera->viewData;

	camera->mode = CAcMode_Interpolate;

	interpolateData->currentFrame = 0;
	interpolateData->endFrame = inNumFrames;

	camera->is_busy = UUcTrue;
	
	return;
}


void CArInterpolate_Enter_ViewData(CAtViewData *inViewData, UUtUns32 inNumFrames)
{
	CAtCamera *camera = ONgGameState->local.camera;
	CAtInterpolateData *interpolateData = &camera->modeData.interpolate;

	interpolateData->ending = *inViewData;
	interpolateData->initial = camera->viewData;

	camera->mode = CAcMode_Interpolate;

	interpolateData->currentFrame = 0;
	interpolateData->endFrame = inNumFrames;

	camera->is_busy = UUcTrue;
	
	return;
}

void CArInterpolate_Enter_Detached(void)
{
	CAtCamera *camera = ONgGameState->local.camera;
	CAtViewData view_data = camera->viewData;

	CArInterpolate_Enter_ViewData(&view_data, 1);

	return;
}

static UUtBool CArInterpolate_Update(UUtUns32 inTicks, UUtUns32 inNumActions, const LItAction *actionBuffer)
{
	UUtBool activate = UUcFalse;

	if (inTicks > 0)  {
		CAtCamera *camera = ONgGameState->local.camera;
		CAtInterpolateData *interpolateData = &camera->modeData.interpolate;

		float amount_ending;

		interpolateData->currentFrame += inTicks;

		if (interpolateData->currentFrame >= interpolateData->endFrame) {
			interpolateData->currentFrame = interpolateData->endFrame;
			camera->is_busy = UUcFalse;
		}

		amount_ending = ((float) interpolateData->currentFrame) / ((float) interpolateData->endFrame);
		UUmAssert((amount_ending >= 0.f) && (amount_ending <= 1.f));

		amount_ending = amount_ending * M3cPi - M3cHalfPi;
		UUmTrig_ClipLow(amount_ending);

		amount_ending = (MUrSin(amount_ending) + 1.f) / 2.f;
		UUmAssert((amount_ending >= 0.f) && (amount_ending <= 1.f));

		MUrPoint3D_Lerp(&interpolateData->initial.location, &interpolateData->ending.location, amount_ending, &camera->viewData.location);
		MUrPoint3D_Lerp(&interpolateData->initial.viewVector, &interpolateData->ending.viewVector, amount_ending, &camera->viewData.viewVector);
		MUrNormalize(&camera->viewData.viewVector);
		camera->viewData.upVector = CArBuildUpVector(&camera->viewData.viewVector);

		activate = UUcTrue;
	}

	return activate;
}

static void CArMatrix_To_ViewData(const M3tMatrix4x3 *inMatrix, CAtViewData *outViewData)
{
	UUmAssertReadPtr(inMatrix, sizeof(*inMatrix));
	UUmAssertWritePtr(outViewData, sizeof(*outViewData));

	outViewData->location = MUrMatrix_GetTranslation(inMatrix);

	// get focus
	{
		M3tPoint3D viewVector = { 0.0f, 0.0f, -1.0f };
		M3tPoint3D upVector = { 0.f, 1.f, 0.f };

		MUrMatrix_MultiplyNormal(&viewVector, inMatrix, &outViewData->viewVector);
		MUrMatrix_MultiplyNormal(&upVector, inMatrix, &outViewData->upVector);

		outViewData->upVector = CArBuildUpVector(&outViewData->viewVector);
	}

	return;
}

UUtBool CArIsBusy(void)
{
	CAtCamera *camera = ONgGameState->local.camera;
	
	return camera->is_busy;
}