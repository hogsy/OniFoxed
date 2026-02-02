#pragma once

/*
	Oni_Aiming.h

	This file contains all aiming related header code

	Author: Quinn Dunki
	c1998 Bungie
*/

#ifndef __ONI_AIMING_H__
#define __ONI_AIMING_H__

#include "BFW.h"

// Constants

#define AMcMouseAccel 5.0f

#define AMcMouseSizeX 16
#define AMcMouseSizeY 16
#define AMcMouseZ -1e5f
#define AMcMouseW 10.0f
#define AMcPickLimit 1000.0f		// How far away an object can be and still be pickable

// Globals

extern UUtBool AMg2DPick;
extern UUtBool AMgInvert;

// Types
typedef struct
{
	ONtCharacter	*hitCharacter;
	UUtUns16		hitPartIndex;
} AMtRayData_Character;

typedef struct
{
	OBtObject *hitObject;
} AMtRayData_Object;

typedef struct
{
	UUtUns32 hitGQIndex;
} AMtRayData_Environment;

typedef union
{
	AMtRayData_Character		rayToCharacter;
	AMtRayData_Environment 		rayToEnvironment;
	AMtRayData_Object 	rayToObject;
} AMtRayData;

typedef enum {								// Result data possibilities
	AMcRayResult_None,
	AMcRayResult_Character,
	AMcRayResult_Environment,
	AMcRayResult_Object
} AMtRayResultType;

typedef struct
{
	AMtRayResultType	resultType;
	AMtRayData			resultData;

	M3tPoint3D			intersection;
} AMtRayResults;

// Initialization
void AMrInitialize(void);
UUtError AMrLevelBegin(void);
void AMrLevelEnd(void);

// Housekeeping
void AMrRenderCrosshair(void);
void AMrMouseMoved(LItAction *inAction);

#define AMcIgnoreCharacterIndex_None 0xffff

// Ray intersection
void AMrRayToEverything(
	UUtUns16 inIgnoreCharacterIndex,
	const M3tPoint3D *inRayOrigin,
	const M3tVector3D *inRayDir,
	AMtRayResults *outResult);

UUtBool AMrRaySphereIntersection(
	const struct M3tBoundingSphere *inSphere,
	const M3tPoint3D *inRayOrigin,
	const M3tVector3D *inRayDir,
	M3tPoint3D *outIntersection);

UUtBool AMrRayToCharacter(
	UUtUns16 inIgnoreCharacterIndex,
	const M3tPoint3D *inRayOrigin,
	const M3tVector3D *inRayDir,
	UUtBool inStopAtOneT,
	ONtCharacter **outCharacter,
	UUtUns16 *outPartIndex,
	M3tPoint3D *outIntersection);

UUtBool AMrRayToObject(
	const M3tPoint3D *inRayOrigin,
	const M3tVector3D *inRayDir,
	float inMaxDistance,
	OBtObject **outObject,
	M3tPoint3D *outIntersection);

UUtBool PHrCollision_Point(const PHtPhysicsContext *inContext, M3tPoint3D *inFrom, M3tVector3D *inVector);

#endif
