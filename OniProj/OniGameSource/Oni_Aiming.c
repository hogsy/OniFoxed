/*
	Oni_Aiming.c
	
	This file contains all aiming related code
	
	Author: Quinn Dunki, Michael Evans
	c1998 Bungie
*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_MathLib.h"
#include "BFW_ScriptLang.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Aiming.h"
#include "Oni_Motoko.h"

// internal functions

UUtBool	AMgShow_Closest = UUcFalse;
UUtBool AMgShow_Filenames = UUcFalse;
UUtBool AMgShow_Axes = UUcFalse;
UUtBool AMgInvert = UUcFalse;
UUtBool AMgHitEverything = UUcFalse;
UUtBool AMgCharacterLaserSight = UUcFalse;

COtStatusLine ONgAimingLine[11];

/****************************** Initialization ***********************************/

void AMrInitialize(void)
{
	/*********************
	* One time initialization
	*/
	
	UUtError error;

	error;

	COrConsole_StatusLines_Begin(ONgAimingLine, &AMgShow_Filenames, 11);

	strcpy(ONgAimingLine[0].text, "*** am_show_filenames (4) ***");

#if CONSOLE_DEBUGGING_COMMANDS
	error = SLrGlobalVariable_Register_Bool("am_show_closest", "dumps the closest GQ index to console", &AMgShow_Closest);
	error = SLrGlobalVariable_Register_Bool("am_show_axes", "shows world axes", &AMgShow_Axes);
	error = SLrGlobalVariable_Register_Bool("am_show_filenames", "dumps the closest GQ index file/obj name to console", &AMgShow_Filenames);
	error = SLrGlobalVariable_Register_Bool("am_invert", "inverts the aiming", &AMgInvert);
	error = SLrGlobalVariable_Register_Bool("am_hit_everything", "makes the laser pointer hit all objects", &AMgHitEverything);
#endif

	return;
}

static void AMiDrawAxes(
	M3tTextureMap	*inTexture,
	float			inScale,
	M3tPoint3D		*inLocation)
{
	UUtError error;
	M3tGeometry *axes = NULL;
	UUtBool axesExists;
	M3tPoint3D x={50.0f,0,0}, y={0,50.0f,0}, z={0,0,50.0f};
	
	UUmAssertReadPtr(inLocation, sizeof(*inLocation));

	error = TMrInstance_GetDataPtr(M3cTemplate_Geometry, "axes", (void **) &axes);
	axesExists = (UUcError_None == error) && (NULL != axes);
	COmAssert(axes);
	if (!axes) { return; }

	if (inTexture) {
		axes->baseMap = inTexture;
	}
	else if (!axes->baseMap) {
		error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "NONE", (void **) &inTexture);
		if (error != UUcError_None) { return; }
	}
	axes->geometryFlags |= M3cGeometryFlag_SelfIlluminent;

	M3rGeom_State_Commit();

	M3rMatrixStack_Push();
		M3rMatrixStack_ApplyTranslate(*inLocation);
		M3rMatrixStack_UniformScale(inScale);
		M3rGeometry_Draw(axes);
	M3rMatrixStack_Pop();
	
	MUmVector_Increment(x,*inLocation);
	MUmVector_Increment(y,*inLocation);
	MUmVector_Increment(z,*inLocation);
	M3rGeom_Line_Light(&x,inLocation,IMcShade_Blue);
	M3rGeom_Line_Light(&y,inLocation,IMcShade_Green);
	M3rGeom_Line_Light(&z,inLocation,IMcShade_Red);

	return;
}

void AMrRenderCrosshair(void)
{
	/************
	* Renders 'crosshair'
	*/

	ONtCharacter *me = ONrGameState_GetPlayerCharacter();
	ONtActiveCharacter *me_active = ONrGetActiveCharacter(me);
	M3tPointScreen pt = {0,0,AMcMouseZ,AMcMouseW};

	if ((!me) || (!me_active)) return;

#if TOOL_VERSION
	if (AMgShow_Axes)
	{
		M3tPoint3D loc;

		if (!me->inventory.weapons[0])
		{
			loc = me_active->aimingVector;
			MUmVector_Scale(loc,1.0f);
			MUmVector_Increment(loc,me->location);
		}
		else loc = me_active->aimingTarget;
		AMiDrawAxes(NULL,1.0f,&loc);
	}
#endif

	return;
}

/*************************************** Ray intersections ******************************************/

void AMrRayToEverything(
	UUtUns16 inIgnoreCharacterIndex,
	const M3tPoint3D *inRayOrigin,
	const M3tVector3D *inRayDir,
	AMtRayResults *outResult)
{
	/**********************
	* Takes a ray and returns the first thing in the world that
	* it hits. Could be an object, wall, character, anything.
	* 'inRayDir' must be normalized. If nothing is hit with the
	* ray (which is pretty unlikely) the outResult->resultType
	* will be AMcRayResult_None.
	*/
		
	M3tPoint3D 			rayp = *inRayOrigin;
	M3tVector3D 		rayv = *inRayDir;
	ONtCharacter 		*hitCharacter;
	OBtObject			*hitObject;
	UUtUns16	 		hitPartIndex;
	M3tPoint3D 			charPoint = {1e9f,1e9f,1e9f};
	M3tPoint3D			objPoint = {1e9f,1e9f,1e9f};
	M3tPoint3D			envPoint = {1e9f,1e9f,1e9f};
	CAtCamera 			*camera = ONgGameState->local.camera;
	AMtRayResultType 	returnWhat;
	UUtBool				collision;

#if TOOL_VERSION
	UUtBool				character_laser_sight;

	character_laser_sight = AMgCharacterLaserSight;
	AMgCharacterLaserSight = UUcFalse;
#endif

	if (!ONgGameState->level->environment)
	{
		outResult->resultType = AMcRayResult_None;
		return;
	}

	UUmAssertReadPtr(inRayOrigin, sizeof(M3tPoint3D));
	UUmAssertReadPtr(outResult, sizeof(M3tVector3D));

	// Check for character collision
	if( inIgnoreCharacterIndex != (UUtUns16) -2 )
		AMrRayToCharacter(inIgnoreCharacterIndex, &rayp,&rayv,UUcFalse,&hitCharacter,&hitPartIndex,&charPoint);
	
	// Check for object collision
	AMrRayToObject(&rayp,&rayv,AMcPickLimit,&hitObject,&objPoint);

	// Check for environment collision
	MUmVector_Scale(rayv,AMcPickLimit);
	collision = AKrCollision_Point(ONgGameState->level->environment, &rayp, &rayv, AMgHitEverything ? 0 : AKcGQ_Flag_Obj_Col_Skip, 1);
	if (collision) envPoint = AKgCollisionList[0].collisionPoint;

	if (hitObject && !collision && !hitCharacter)
	{
		returnWhat = AMcRayResult_Object;
	}
	else if (!hitObject && collision && !hitCharacter)
	{
		returnWhat = AMcRayResult_Environment;
	}
	else if (!hitObject && !collision && hitCharacter)
	{
		returnWhat = AMcRayResult_Character;
	}
	else if (!hitObject && !collision && !hitCharacter)
	{
		returnWhat = AMcRayResult_None;
	}
	else
	{
		float distance_to_environment;
		float distance_to_character;
		float distance_to_object;

		distance_to_environment = MUrPoint_Distance_Squared(&rayp,&envPoint);
		distance_to_character = MUrPoint_Distance_Squared(&rayp,&charPoint);
		distance_to_object = MUrPoint_Distance_Squared(&rayp,&objPoint);

		if ((distance_to_character < distance_to_environment) & (distance_to_character < distance_to_object)) {
			returnWhat = AMcRayResult_Character;
		}
		else if (distance_to_object < distance_to_environment) {
			returnWhat = AMcRayResult_Object;
		}
		else {
			returnWhat = AMcRayResult_Environment;
		}
	}
	
#if TOOL_VERSION
	if (character_laser_sight) {
		if (AMgShow_Closest && collision) {
			AKtEnvironment*		env = ONgGameState->level->environment;
			AKtGQ_Render*		gq = env->gqRenderArray->gqRender + AKgCollisionList[0].gqIndex;
			AKtGQ_Collision*	gqCol = env->gqCollisionArray->gqCollision + AKgCollisionList[0].gqIndex;
			M3tTextureMap*		textureMap = env->textureMapArray->maps[gq->textureMapIndex];
			
			COrConsole_Printf("%d, %s, %x", AKgCollisionList[0].gqIndex, textureMap->debugName, gqCol->planeEquIndex);
		}
		
		if (AMgShow_Filenames) {
			{
				UUtUns32 clear_itr;

				for(clear_itr = 1; clear_itr < 11; clear_itr++) {
					strcpy(ONgAimingLine[clear_itr].text, "");
				}
			}

			if (hitObject) {
				sprintf(ONgAimingLine[1].text, "Object %s in file %s",hitObject->setup->objName,hitObject->setup->fileName);
			}
			else if (hitCharacter) {
				sprintf(ONgAimingLine[1].text, "Character %s, script ID %d",hitCharacter->player_name, hitCharacter->scriptID);
			}
			else if (collision) {
				UUtUns32 gq_index = AKgCollisionList[0].gqIndex;
				AKtEnvironment *environment = ONgGameState->level->environment;
				AKtGQ_General *gq_general = environment->gqGeneralArray->gqGeneral + gq_index;
				AKtGQ_Render *gq_render = environment->gqRenderArray->gqRender + gq_index;
				AKtGQ_Collision *gq_collision = environment->gqCollisionArray->gqCollision + gq_index;
				M3tPoint3D *environment_points = environment->pointArray->points;
				M3tPoint3D *this_environment_point;
				M3tTextureMap **environment_textures = environment->textureMapArray->maps;
				UUtUns32 num_environment_textures = environment->textureMapArray->numMaps;
				char *texture_name = "this quad has no texture";
				const char *mip_map_string = "";
				const char *type_string = "";
				M3tPlaneEquation plane;
				AKtGQ_Debug	*gqDebug;

				AKmPlaneEqu_GetComponents(gq_collision->planeEquIndex, environment->planeArray->planes, plane.a, plane.b, plane.c, plane.d);

				if (gq_render->textureMapIndex < num_environment_textures) {
					M3tTextureMap *this_texture_map = environment_textures[gq_render->textureMapIndex];

					if (NULL != this_texture_map) {
						texture_name = TMrInstance_GetInstanceName(this_texture_map);

						type_string = IMrPixelTypeToString(this_texture_map->texelType);
						mip_map_string = (this_texture_map->flags & M3cTextureFlags_HasMipMap) ? "MIPMAP" : "NOMIPMAP";
					}
				}

				if ((ONgGameState->level->environment->gqDebugArray == NULL) ||
					(ONgGameState->level->environment->gqDebugArray->gqDebug == NULL)) {
					gqDebug = NULL;
				} else {
					gqDebug = ONgGameState->level->environment->gqDebugArray->gqDebug + AKgCollisionList[0].gqIndex;
				}

				sprintf(ONgAimingLine[1].text, "Gunk [%d]", AKgCollisionList[0].gqIndex);
				sprintf(ONgAimingLine[2].text, "%s in file %s", 
						(gqDebug == NULL) ? "UNKNOWN" : gqDebug->object_name, 
						(gqDebug == NULL) ? "UNKNOWN" : gqDebug->file_name);
				sprintf(ONgAimingLine[3].text, "%s %s %s", texture_name, type_string, mip_map_string);
				sprintf(ONgAimingLine[4].text, "gq flags = %x", gq_general->flags);
				sprintf(ONgAimingLine[5].text, "2sd %d !c %d !oc %d !cc %d !ocl %d", 
					(gq_general->flags & AKcGQ_Flag_DrawBothSides) ? 1 : 0,
					(gq_general->flags & AKcGQ_Flag_No_Collision) ? 1 : 0,
					(gq_general->flags & AKcGQ_Flag_No_Object_Collision) ? 1 : 0,
					(gq_general->flags & AKcGQ_Flag_No_Character_Collision) ? 1 : 0,
					(gq_general->flags & AKcGQ_Flag_No_Occlusion) ? 1 : 0);
				sprintf(ONgAimingLine[6].text, "plane %x %2.2f, %2.2f, %2.2f %2.2f",gq_collision->planeEquIndex, plane.a, plane.b, plane.c, plane.d);

				this_environment_point = environment_points + gq_general->m3Quad.vertexIndices.indices[0];
				sprintf(ONgAimingLine[7].text, "%2.2f %2.2f %2.2f", this_environment_point->x, this_environment_point->y, this_environment_point->z);

				this_environment_point = environment_points + gq_general->m3Quad.vertexIndices.indices[1];
				sprintf(ONgAimingLine[8].text, "%2.2f %2.2f %2.2f", this_environment_point->x, this_environment_point->y, this_environment_point->z);

				this_environment_point = environment_points + gq_general->m3Quad.vertexIndices.indices[2];
				sprintf(ONgAimingLine[9].text, "%2.2f %2.2f %2.2f", this_environment_point->x, this_environment_point->y, this_environment_point->z);

				this_environment_point = environment_points + gq_general->m3Quad.vertexIndices.indices[3];
				sprintf(ONgAimingLine[10].text, "%2.2f %2.2f %2.2f", this_environment_point->x, this_environment_point->y, this_environment_point->z);				
			}
		}
	}
#endif // TOOL_VERSION
	
	switch(returnWhat)
	{
		case AMcRayResult_Character:	
			outResult->resultType = AMcRayResult_Character;
			outResult->resultData.rayToCharacter.hitPartIndex = hitPartIndex;
			outResult->resultData.rayToCharacter.hitCharacter = hitCharacter;
			outResult->intersection = charPoint;
			break;

		case AMcRayResult_Object:	
			outResult->resultType = AMcRayResult_Object;
			outResult->resultData.rayToObject.hitObject = hitObject;
			outResult->intersection = objPoint;
			break;

		case AMcRayResult_Environment:	
			outResult->resultType = AMcRayResult_Environment;
			outResult->resultData.rayToEnvironment.hitGQIndex = AKgCollisionList[0].gqIndex;
			outResult->intersection = AKgCollisionList[0].collisionPoint;
			break;

		case AMcRayResult_None:
			outResult->resultType = AMcRayResult_None;
			MUmVector_Add(outResult->intersection, rayv, rayp);
			break;

		default: 
			UUmAssert(!"unhandled case");
	}
}

UUtBool AMrRayToCharacter(
	UUtUns16 inIgnoreCharacterIndex,
	const M3tPoint3D *inRayOrigin, 
	const M3tVector3D *inRayDir,
	UUtBool inStopAtOneT,
	ONtCharacter **outCharacter, 
	UUtUns16 *outPartIndex, 
	M3tPoint3D *outIntersection)
{
	/************************
	* Given a ray in worldspace, returns a pointer
	* to the first character that intersects that ray.
	* NULL if none are found. Also returns the body
	* part index on that character, and the point of intersection.
	* If you don't care about any of those return values, you
	* may pass in NULLs in any combination.
	*
	* Also returns UUcTrue if there was a collision, UUcFalse otherwise.
	*
	*/
	

	ONtCharacter *closestCharacter = NULL;
	float current_distance_squared;
	float min_distance_squared = UUcFloat_Max;
	UUtUns32 closestPartIndex, partIndex;
	M3tPoint3D curIntersection;
	M3tPoint3D closestIntersection;

	ONtCharacter **active_character_list;
	UUtUns32 active_character_count;
	UUtUns32 itr;
	UUtBool stop_at_first_intersection;

	if (outCharacter != NULL)
	{
		*outCharacter = NULL;
	}

	UUmAssert(ONgGameState != NULL);
	UUmAssert(inRayOrigin != NULL);
	UUmAssert(inRayDir != NULL);

	active_character_list = ONrGameState_ActiveCharacterList_Get();
	active_character_count = ONrGameState_ActiveCharacterList_Count();

	stop_at_first_intersection = ((outCharacter == NULL) && (outPartIndex == NULL) && (outIntersection == NULL));
	
	for(itr = 0; itr < active_character_count; itr++)
	{
		ONtCharacter *curCharacter = active_character_list[itr];
		ONtActiveCharacter *active_character = ONrGetActiveCharacter(curCharacter);
		TRtBody *curBody = ONrCharacter_GetBody(curCharacter, TRcBody_SuperHigh);
		UUtBool intersection;
		
		if (curCharacter->index == inIgnoreCharacterIndex) {
			continue;
		}

		UUmAssert(curCharacter->flags & ONcCharacterFlag_InUse);
		UUmAssertReadPtr(curCharacter->characterClass,sizeof(UUtUns32));
		UUmAssertReadPtr(active_character, sizeof(*active_character));

		// check the ray against the character's body parts
		intersection = ONrCharacter_Collide_Ray(curCharacter, (M3tPoint3D *) inRayOrigin, (M3tVector3D *) inRayDir,
												inStopAtOneT, &partIndex, NULL, &curIntersection, NULL);
		if (!intersection) {
			continue;
		}

		if (stop_at_first_intersection) {
			// we have found an intersection and don't care about any of the return
			// values, stop right now
			return UUcTrue;
		}

		current_distance_squared = MUrPoint_Distance_Squared(&curIntersection, inRayOrigin);
		if (current_distance_squared < min_distance_squared) {
			min_distance_squared	= current_distance_squared;
			closestCharacter	= curCharacter;
			closestPartIndex	= partIndex;
			closestIntersection	= curIntersection;
		}
	}
	
	if (closestCharacter != NULL)
	{
		if (outCharacter != NULL) {
			*outCharacter = closestCharacter;
		}

		if (outPartIndex != NULL) {
			*outPartIndex = (UUtUns16) closestPartIndex;
		}

		if (outIntersection != NULL) {
			*outIntersection = closestIntersection;
		}
	}

	return (closestCharacter != NULL);
}

UUtBool PHrCollision_Point(const PHtPhysicsContext *inContext, M3tPoint3D *inFrom, M3tVector3D *inVector)
{
	UUtBool found_collision = UUcFalse;

	if (inContext->flags & PHcFlags_Physics_FaceCollision) {
		found_collision = PHrCollision_Volume_Ray(inContext->worldAlignedBounds, inFrom, inVector, NULL, NULL);
	}
	else {
		found_collision = CLrSphere_Line(inFrom, inVector, &inContext->sphereTree->sphere);
	}

	return found_collision;
}

UUtBool AMrRayToObject(
	const M3tPoint3D *inRayOrigin, 
	const M3tVector3D *inRayDir,
	float inMaxDistance,
	OBtObject **outObject, 
	M3tPoint3D *outIntersection)
{
	/************************
	* Given a ray in worldspace, returns a pointer
	* to the first object that intersects that ray.
	* NULL if none are found. Also returns the point of intersection.
	* If you don't care about any of those return values, you
	* may pass in NULLs in any combination.
	*
	* Also returns UUcTrue if there was a collision, UUcFalse otherwise.
	*
	*/
	

	OBtObject *closestObject = NULL;
	float current_distance_squared;
	float min_distance_squared = UUcFloat_Max, max_distance_squared;
	UUtUns16 objectIndex;
	M3tPoint3D curIntersection;
	M3tPoint3D closestIntersection;
	M3tVector3D V;
	UUtBool intersection, stop_at_closest_intersection;

	if (outObject != NULL)
	{
		*outObject = NULL;
	}

	UUmAssert(ONgGameState != NULL);
	UUmAssert(inRayOrigin != NULL);
	UUmAssert(inRayDir != NULL);

	max_distance_squared = UUmSQR(inMaxDistance);
	stop_at_closest_intersection = ((outObject == NULL) && (outIntersection == NULL));

	for (objectIndex = 0; objectIndex < ONgGameState->objects->numObjects; objectIndex++)
	{
		OBtObject *curObject = ONgGameState->objects->object_list + objectIndex;
		
		if (0 == (curObject->flags & OBcFlags_InUse)) {
			continue;
		}

		UUmAssert(curObject->physics);

		if (curObject->physics->flags & PHcFlags_Physics_FaceCollision) {
			// Face collision
			V = *inRayDir;
			MUmVector_Scale(V, 1000.0f);

			intersection = PHrCollision_Volume_Ray(curObject->physics->worldAlignedBounds, inRayOrigin, &V, NULL, &curIntersection);
		}
		else {
			// Sphere tree collision
			UUmAssert(curObject->physics->sphereTree);

			intersection = AMrRaySphereIntersection(&curObject->physics->sphereTree->sphere, inRayOrigin, inRayDir, &curIntersection);
		}
		
		if (intersection) {
			// CB: I do not understand why this was a good idea, surely we want the closest intersection rather than the intersection with
			// the closest object?
			// current_distance_squared = MUrPoint_Distance_Squared(&curObject->physics->sphereTree->sphere.center, inRayOrigin);
			current_distance_squared = MUrPoint_Distance_Squared(&curIntersection, inRayOrigin);

			if (current_distance_squared > max_distance_squared) {
				// reject this intersection
				continue;

			} else if (current_distance_squared < min_distance_squared) {
				if (stop_at_closest_intersection) {
					// there is an intersection, abort and return early
					return UUcTrue;
				}

				// choose this intersection
				min_distance_squared	= current_distance_squared;
				closestObject			= curObject;
				closestIntersection		= curIntersection;
			}
		}
	}
	
	if (closestObject != NULL) {
		if (outObject != NULL) {
			*outObject = closestObject;
		}

		if (outIntersection != NULL) {
			*outIntersection = closestIntersection;
		}
	}

	return (closestObject != NULL);
}

UUtBool AMrRaySphereIntersection(const M3tBoundingSphere *inSphere, const M3tPoint3D *inRayOrigin, const M3tVector3D *inRayDir, M3tPoint3D *outIntersection)
{
	/*****************************
	* Returns TRUE if the given ray intersects with the given
	* sphere. NOTE: 'inRayDir' must be normalized.
	*
	* outIntersection is optional
	*/
	

	UUtBool intersection;
	float v,disc,d, EO_sq, r_sq;
	M3tVector3D EO,rayDir;

	UUmAssertReadPtr(inSphere, sizeof(*inSphere));
	UUmAssertReadPtr(inRayDir, sizeof(*inRayDir));
	UUmAssert(MUmVector_IsNormalized(*inRayDir));
	UUmAssert(UUmFloat_CompareEquSloppy(MUmVector_GetLength(*inRayDir), 1.f));
	
	MUmVector_Subtract(EO,inSphere->center,(*inRayOrigin));
	EO_sq = MUrVector_DotProduct(&EO,&EO);
	r_sq = UUmSQR(inSphere->radius);
	if (EO_sq < r_sq) {
		// CB: the ray starts inside the sphere, there is an intersection - 21 august 2000
		return UUcTrue;
	}

	v = MUrVector_DotProduct(&EO,inRayDir);
	disc = r_sq - ((EO_sq - v*v));
	
	if (disc < 0) {
		intersection = UUcFalse;
	}
	else {
		M3tVector3D rayToIntersection;
		M3tPoint3D intersectionPoint;
		float dot;
		
		d = MUrSqrt(disc);
		rayDir = *inRayDir;
		MUmVector_Scale(rayDir,v-d);
		MUmVector_Add(intersectionPoint, (*inRayOrigin), rayDir);

		MUmVector_Subtract(rayToIntersection,intersectionPoint, *inRayOrigin);
		dot = MUrVector_DotProduct(&rayToIntersection, inRayDir);

		if (dot < 0) {
			intersection = UUcFalse;
		}
		else if (outIntersection != NULL) {
			*outIntersection = intersectionPoint;
		}
	}

	return intersection;
}
