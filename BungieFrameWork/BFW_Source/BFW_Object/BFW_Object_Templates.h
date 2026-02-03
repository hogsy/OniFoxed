/*
	FILE:	BFW_Object_Templates.h

	AUTHOR:	Quinn Dunki, Michael Evans

	CREATED: 4/8/98

	PURPOSE: Interface to the Object engine

	Copyright (c) Bungie Software 1998-2000

*/

#ifndef __BFW_OBJECT_TEMPLATES_H__
#define __BFW_OBJECT_TEMPLATES_H__

#include "BFW.h"
#include "BFW_MathLib.h"
#include "BFW_Effect.h"
#include "BFW_EnvParticle.h"

#define OBcMaxObjectName 64		// Don't change without changing references below


typedef tm_struct OBtAnimationKeyFrame
{
	M3tQuaternion	quat;
	M3tPoint3D		translation;
	UUtInt32		frame;
} OBtAnimationKeyFrame;

#define OBcTemplate_Animation UUm4CharToUns32('O','B','A','N')
typedef tm_template('O','B','A','N',"Object animation")
OBtAnimation
{
	// implied 8 bytes here

//	char owner[64];		// Must be same as OBcMaxObjectName

	tm_pad			pad0[12];

	UUtUns32		animFlags;

	M3tMatrix4x3	startMatrix;
	M3tMatrix4x3	scale;
	UUtUns16		ticksPerFrame;

	UUtUns16		numFrames;
	UUtUns16		doorOpenFrames;

	tm_varindex UUtUns16				numKeyFrames;
	tm_vararray OBtAnimationKeyFrame	keyFrames[1];
} OBtAnimation;


typedef struct OBtAnimationContext
{
	OBtAnimation		*animation;

	UUtUns16			animContextFlags;
	UUtInt16			animationFrame;
	UUtInt16			animationStep;
	UUtInt16			animationStop;

	UUtUns32			frameStartTime;
	UUtUns16			numPausedFrames;
	UUtUns16			pad;
} OBtAnimationContext;


/*
 * Object setup stuff
 */

 	typedef tm_struct OBtObjectSetup
	{
		M3tGeometryArray		*geometry_array;

		OBtAnimation*			animation;

		EPtEnvParticleArray*	particleArray;

		UUtUns32				flags;
		UUtUns32				doorGhostIndex;
		UUtUns32				doorScriptID;	// 0 if not a door
		UUtUns32				physicsLevel;
		UUtUns32				scriptID;

		M3tPoint3D				position;
		M3tQuaternion			orientation;
		float					scale;
		M3tMatrix4x3			debugOrigMatrix;

		char					objName[64];	// Must match OBcMaxObjectName above
		char					fileName[64];	// Must match OBcMaxObjectName above
	} OBtObjectSetup;

	#define OBcTemplate_ObjectArray UUm4CharToUns32('O','B','O','A')
	typedef tm_template('O','B','O','A', "Starting Object Array")
	OBtObjectSetupArray
	{
		tm_pad						pad0[22];

		tm_varindex UUtUns16		numObjects;
		tm_vararray OBtObjectSetup	objects[1];
	} OBtObjectSetupArray;




#endif // __BFW_OBJECT_TEMPLATES_H__
