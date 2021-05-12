#pragma once

/*
	FILE:	BFW_Object.h
	
	AUTHOR:	Quinn Dunki, Michael Evans, Kevin Armstrong
	
	CREATED: 4/8/98
	
	PURPOSE: Interface to the Object engine
	
	Copyright 1998,2000

*/

#ifndef __BFW_OBJECT_H__
#define __BFW_OBJECT_H__

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_Object_Templates.h"
#include "BFW_Physics.h"

#define OBcInfinity (65535)


enum {
	OBcFlags_IsDoor =				0x0002,
	OBcFlags_IgnoreCharacters =		0x0004,
	OBcFlags_ForceDraw =			0x0008,
	OBcFlags_LightSource =			0x0010,
	OBcFlags_DeleteMe =				0x0040,
	OBcFlags_NotifyCollision = 		0x0080,
	OBcFlags_Invisible = 			0x0100,
	OBcFlags_InUse =				0x0200,
	OBcFlags_NoCollision =			0x0400,
	OBcFlags_NoGravity =			0x0800,
	OBcFlags_FaceCollision =		0x1000,
	OBcFlags_ParticlesCreated =		0x2000,
	OBcFlags_JelloObjects =			0x4000,
	OBcFlags_FlatLighting =			0x8000
};

enum {
	OBcAnimFlags_Loop =					0x0001,
	OBcAnimFlags_Pingpong =				0x0002,
	OBcAnimFlags_Random =				0x0004,
	OBcAnimFlags_Autostart =			0x0008,
	OBcAnimFlags_Localized =			0x0010
};

enum {
	OBcAnimContextFlags_Animate =		0x0001,
	OBcAnimContextFlags_NoRotation =	0x0002,
	OBcAnimContextFlags_CollisionPause= 0x0010,
	OBcAnimContextFlags_RotateY180 =	0x0020,
	OBcAnimContextFlags_Slave =			0x0040

};

struct OBtObject;	
typedef UUtBool (*OBtAllowPauseCallback)(const struct OBtObject *inObject, const struct PHtPhysicsContext *inCollidingCallback);

#define OBcObjectNodeCount 32

#define OBcMaxGeometries 8

typedef struct OBtObject
{
	M3tGeometry				*geometry_list[OBcMaxGeometries];
	UUtUns32				geometry_count;

	M3tGeometry				*damagedGeometry;
	
	EPtEnvParticleArray*	particleArray;
		
	UUtUns16				index;
	UUtUns16				flags;

	UUtUns16				ignoreCharacterIndex;	// only until first collision or OBcIgnoreFrames pass
	UUtUns16				pad;

	PHtPhysicsContext		*physics;				// Data for physics engine
	
	UUtUns32				num_frames_offscreen;
	IMtShade				flat_lighting_shade;
	void					*owner;
	OBtObjectSetup			*setup;

	OBtAllowPauseCallback	allow_pause;
	PHtPhysicsContext		*pausing_context;

	M3tPoint3D last_position; // S.S.
	UUtUns32 oct_tree_node_index[OBcObjectNodeCount]; // S.S. length-prefixed list of oct tree nodes;

	PHtSphereTree			sphere_tree_memory[OBcMaxGeometries];
	M3tBoundingVolume		bounding_volume_memory;
} OBtObject;

typedef tm_struct OBtObjectList
{
	UUtUns16				maxObjects;
	UUtUns16				numObjects;
	
	OBtObject				object_list[1];
} OBtObjectList;
	
UUtError OBrRegisterTemplates(void);

UUtBool OBrUpdate(OBtObject *ioObject, UUtBool *deleteMe, UUtInt16 *redoObject);
void OBrDraw(OBtObject *inObject);

void OBrNotifyPhysicsCollision(OBtObject *inObject, const PHtPhysicsContext *inContext, const M3tVector3D *inVelocity,
								UUtUns16 inNumColliders, PHtCollider *inColliders);

void OBrChooseGeometry(OBtObject *ioObject);
void OBrShow(OBtObject *inObject, UUtBool inShow);

OBtObjectList *OBrList_New(UUtUns16 inNumObjects);
void OBrList_Delete(OBtObjectList *inList);
void OBrList_Update(OBtObjectList *ioObjectList);
void OBrList_Draw(OBtObjectList *ioObjectList);
OBtObject *OBrList_Add(OBtObjectList *inObjectList);
void OBrList_Nuke(OBtObjectList *inList);
OBtObject *OBrList_NthItem(OBtObjectList *inList, UUtUns16 i);
UUtUns16 OBrList_NumItems(OBtObjectList *inList);

void OBrConsole_SetHP(UUtUns32 inArgC,char** inArgV,void* inRefcon);
void OBrConsole_Reset(UUtUns32 inArgC,char** inArgV, void* inRefcon);

void OBrDelete(OBtObject *ioObject);

UUtError OBrInit(
OBtObject *ioObject,
	OBtObjectSetup *inSetup);


void OBrAnim_Stop( OBtAnimationContext *ioObject);
void OBrAnim_Start( OBtAnimationContext *ioObject);
void OBrAnim_Reset( OBtAnimationContext *ioObject);
void OBrAnim_Matrix( const OBtAnimationContext *inObject, M3tMatrix4x3 *outMatrix);
void OBrAnim_Position( OBtAnimationContext *ioObject, M3tPoint3D *outPosition);
UUtError OBrSetAnimationByName( OBtObject *inObject, char *inAnimName);
void OBrSetAnimation( OBtObject *inObject, OBtAnimation *inAnim, UUtUns16 inStartFrame, UUtUns16 inEndFrame);
void OBrAnimation_GetMatrix( OBtAnimation *inAnimation, UUtInt32 inFrame, UUtUns32 inFlags, M3tMatrix4x3 *outMatrix);

#endif // __BFW_OBJECT_H__
