/*
	Oni_AI_Script.c
	
	Script level AI stuff
	
	Author: Quinn Dunki, Michael Evans

	Copyright (c) 1998 Bungie Software
*/

#include <ctype.h>
#include <stdarg.h>

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Mathlib.h"
#include "BFW_Console.h"
#include "BFW_BitVector.h"
#include "BFW_AppUtilities.h"
#include "BFW_Console.h"
#include "BFW_TM_Construction.h"
#include "BFW_ScriptLang.h"

#include "Oni.h"
#include "Oni_AI.h"
#include "Oni_AI_Script.h"
#include "Oni_Astar.h"
#include "Oni_Character.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Film.h"
#include "Oni_Object.h"

#define AIcMaxObjectsOfOneID 128





//==========================

static void AIiObjectsFromScript(
	UUtInt32 inScriptID1,
	UUtInt32 inScriptID2,
	UUtUns32 *outNumObjects,
	OBtObject **ioListStorage)
{
	/****************
	* Finds the objects matching a scripting ID. Returns a list of object pointers.
	*/
	
	UUtUns16 i,count;
	OBtObject *object;
	OBtObjectList *objects;
	UUtUns32 id_low = UUmMin(inScriptID1, inScriptID2);
	UUtUns32 id_high = UUmMax(inScriptID1, inScriptID2);
	
	objects = ONrGameState_GetObjectList();

	*outNumObjects=0;

	if (NULL == objects) {
		return;
	}

	count = objects->numObjects;
	
	for (i=0; i<count; i++)
	{
		object = objects->object_list + i;
		
		if ((object->flags & OBcFlags_InUse) == 0)
			continue;

		if ((object->setup->scriptID >= id_low) && (object->setup->scriptID <= id_high))
		{
			ioListStorage[(*outNumObjects)++] = object;
			if (*outNumObjects >= AIcMaxObjectsOfOneID)
			{
				COrConsole_Printf("ERROR: Too many objects have the same ref number %d..%d",id_low, id_high);
			}
		}
	}

	if (0 == *outNumObjects) {
		COrConsole_Printf("failed to find any objects in the range %d .. %d", inScriptID1, inScriptID2);
	}

	return;
}

UUtError AIrScript_LevelBegin_Triggers(
	void)
{
	/**************
	* Set up triggers for a level
	*/

	UUtUns16 index;
	AItScriptTrigger *trigger;
	AItScriptTriggerClass *triggerClass;

	ONgGameState->triggers.triggers = NULL;

	if ((ONgLevel->scriptTriggerArray == NULL) || (ONgLevel->scriptTriggerArray->numTriggers == 0)) {
		return UUcError_None;
	}
	
	ONgGameState->triggers.triggers = UUrMemory_Block_New(sizeof(AItScriptTrigger) * ONgLevel->scriptTriggerArray->numTriggers);

	if (!ONgGameState->triggers.triggers) {
		COrConsole_Printf("failed to allocate triggers");
		return UUcError_OutOfMemory;
	}

	// Most trigger goo is already set up by the importer because there is very little run-time state
	for (index=0; index < ONgLevel->scriptTriggerArray->numTriggers; index++)
	{
		trigger = ONgGameState->triggers.triggers + index;
		triggerClass = ONgLevel->scriptTriggerArray->triggers + index;

		UUrMemory_Clear(trigger,sizeof(AItScriptTrigger));
		trigger->triggerClass = triggerClass;

		if (trigger->triggerClass->type == AIcTriggerType_Laser)
		{
			#if 0
			// Find FX object connected to this trigger
			trigger->ref = (void *)FXrFindByName(&ONgGameState->effects,trigger->triggerClass->refName);
			if (!trigger->ref)
			{
				COrConsole_Printf("ERROR: No FX named %s could be found for trigger",trigger->triggerClass->refName);
				trigger->triggerClass->type = AIcTriggerType_Temp;	// Effectively disables it
			}
			#endif
		}
	}

	return UUcError_None;
}

void AIrScript_LevelEnd_Triggers(
	void)
{
	/*************
	* Frees up triggers
	*/

	if (ONgGameState->triggers.triggers) UUrMemory_Block_Delete(ONgGameState->triggers.triggers);
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
ONtCharacter*
AIrUtility_New_GetCharacterFromID(
	UUtUns16	inID,
	UUtBool		inExhaustive)
{
	ONtCharacter **present_character_list = ONrGameState_PresentCharacterList_Get();
	UUtUns32 present_character_count = ONrGameState_PresentCharacterList_Count();
	ONtCharacter *curChr, *result = NULL;
	UUtUns32		itr;

	for(itr = 0; itr < present_character_count; itr++)
	{
		curChr = present_character_list[itr];

		UUmAssert(curChr->flags & ONcCharacterFlag_InUse);
		
		if (curChr->scriptID == inID) {
			result = curChr;
			break;
		}
	}
	
	if ((result == NULL) && (inExhaustive)) {
		// search the entire character array, not just the present list
		present_character_count = ONrGameState_GetNumCharacters();
		curChr = ONrGameState_GetCharacterList();

		for (itr = 0; itr < present_character_count; itr++, curChr++) {
			if (curChr->flags & ONcCharacterFlag_InUse) {
				if (curChr->scriptID == inID) {
					result = curChr;
					break;
				}
			}
		}
	}

	return result;
}

static UUtError
AIiScript_CameraInterpolate_Internal(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue,
	UUtBool					inBlock)
{
	/**********
	* cm_interpolate animation number-of-frames
	*/
	
	CAtCamera*		camera = ONgGameState->local.camera;
	OBtAnimation*	animation;
	M3tMatrix4x3	matrix;
	UUtUns32		numFrames;
	const char		*animation_name = inParameterList[0].val.str;

	if (inBlock && CArIsBusy()) {
		*outStalled = UUcTrue;
		return UUcError_None;
	}
	else {
		*outStalled = UUcFalse;
	}

	// Load the named animation
	animation = TMrInstance_GetFromName(OBcTemplate_Animation, animation_name);

	if (NULL == animation) {
		SLrScript_ReportError(inErrorContext, "failed to load object animation %s.", animation_name);
		return UUcError_None;
	}

	ONrGameState_NotifyCameraCut();
	OBrAnimation_GetMatrix(animation, 0, 0, &matrix);

	numFrames = (UUtUns32) UUmMax(inParameterList[1].val.i, 1);
	CArInterpolate_Enter_Matrix(&matrix, numFrames);
	
	return UUcError_None;
}

static UUtError
AIiScript_DetachCamera(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	CArInterpolate_Enter_Detached();

	return UUcError_None;
}


static UUtError
AIiScript_CameraInterpolate_NoBlock(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError error;

	error = AIiScript_CameraInterpolate_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStalled, ioReturnValue, UUcFalse);

	return error;
}

static UUtError
AIiScript_CameraInterpolate_Block(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError error;

	error = AIiScript_CameraInterpolate_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStalled, ioReturnValue, UUcTrue);

	return error;
}

static UUtError
AIiScript_CameraAnim_Internal(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue,
	UUtBool					inBlock)
{
	/**********
	* cm_anim [move/look/both] animation
	*/
	
	UUtError		error;
	CAtCamera*		camera = ONgGameState->local.camera;
	OBtAnimation*	animation;

	if (inBlock && CArIsBusy()) {
		*outStalled = UUcTrue;
		return UUcError_None;
	}
	else {
		*outStalled = UUcFalse;
	}

	
	// Load the named animation
	error =
		TMrInstance_GetDataPtr(
			OBcTemplate_Animation,
			inParameterList[1].val.str,
			&animation);

#if TOOL_VERSION
	COrConsole_Printf("cm_anim %s", inParameterList[1].val.str);
#endif

	if (error != UUcError_None)
	{
		SLrScript_ReportError(inErrorContext, "failed to load object animation %s.", inParameterList[1].val.str);
		return UUcError_Generic;
	}

	CArAnimate_Enter(animation);
	
	return UUcError_None;
}


static UUtError
AIiScript_CameraAnim_NoBlock(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError error;

	error = AIiScript_CameraAnim_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStalled, ioReturnValue, UUcFalse);

	return error;
}

static UUtError
AIiScript_CameraAnim_Block(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError error;

	error = AIiScript_CameraAnim_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStalled, ioReturnValue, UUcTrue);

	return error;
}

static UUtError
AIiScrip_CameraWait(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/**********
	* cm_wait
	*/

	if (CArIsBusy()) {
		*outStalled = UUcTrue;
		return UUcError_None;
	}
	else {
		*outStalled = UUcFalse;
	}

	return UUcError_None;
}


static UUtError
AIiScript_CameraReset(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	// cm_reset
	ONrGameState_NotifyCameraCut();
	CArFollow_Enter();

	return UUcError_None;
}

static UUtError
AIiScript_CameraOrbit_Internal(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue,
	UUtBool					inBlock)
{
	float stop_angle = -1;
	if (inBlock && CArIsBusy()) {
		*outStalled = UUcTrue;
		return UUcError_None;
	}
	else {
		*outStalled = UUcFalse;
	}

	if (inParameterListLength >= 2) {
		UUmAssert(inParameterList[1].type = SLcType_Float);
		stop_angle = inParameterList[1].val.f;
	}

	CArOrbit_Enter(inParameterList[0].val.f * (M3c2Pi / 360.f), stop_angle * (M3c2Pi / 360.f));

	return UUcError_None;
}



static UUtError
AIiScript_CameraOrbit_NoBlock(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError error;

	error = AIiScript_CameraOrbit_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStalled, ioReturnValue, UUcFalse);

	return error;
}

static UUtError
AIiScript_CameraOrbit_Block(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError error;

	error = AIiScript_CameraOrbit_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStalled, ioReturnValue, UUcTrue);

	return error;
}

static UUtError
AIiScript_ForceDraw(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/******************
	* obj_force_draw id id
	*/
	
	OBtObject *objects[AIcMaxObjectsOfOneID];
	UUtUns32 i, count;
	UUtInt32 p0, p1;
	
	p0 = inParameterList[0].val.i;
	p1 = (inParameterListLength >= 2) ? inParameterList[1].val.i : p0;

	AIiObjectsFromScript(p0, p1, &count, objects);

	for (i=0; i<count; i++)
	{
		objects[i]->flags |= OBcFlags_ForceDraw;
	}
	
	return UUcError_None;
}

static UUtError
AIiScript_ObjShow(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/******************
	* obj_show id id
	*/
	
	OBtObject *objects[AIcMaxObjectsOfOneID];
	UUtUns32 i, count;
	UUtInt32 p0, p1;
	
	p0 = inParameterList[0].val.i;
	p1 = (inParameterListLength >= 2) ? inParameterList[1].val.i : p0;

	AIiObjectsFromScript(p0, p1, &count, objects);

	for (i=0; i<count; i++)
	{
		OBrShow(objects[i], UUcTrue);
	}
	
	return UUcError_None;
}

static UUtError
AIiScript_ObjHide(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/******************
	* obj_hide id id
	*/
	
	OBtObject *objects[AIcMaxObjectsOfOneID];
	UUtUns32 i, count;
	UUtInt32 p0, p1;

	p0 = inParameterList[0].val.i;
	p1 = (inParameterListLength >= 2) ? inParameterList[1].val.i : p0;

	AIiObjectsFromScript(p0, p1, &count, objects);

	for (i=0; i<count; i++)
	{
		OBrShow(objects[i], UUcFalse);
	}
	
	return UUcError_None;
}


static UUtError
AIiScript_ObjKill(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)

{
	/******************
	* obj_kill id id 
	*/
	
	OBtObject *objects[AIcMaxObjectsOfOneID];
	UUtUns32 i, count;
	UUtInt32 p0, p1;

	p0 = inParameterList[0].val.i;
	p1 = (inParameterListLength >= 2) ? inParameterList[1].val.i : p0;

	AIiObjectsFromScript(p0, p1, &count, objects);

	for (i=0; i < count; i++)
	{
		OBrDelete(objects[i]);
	}
	
	return UUcError_None;
}

//AIiScript_ObjShade
//{ "obj_shade", "turns display of an object on or off", "obj_id:int obj_id:int r:int g:int b:int", AIiScript_ObjShade },
static UUtError
AIiScript_ObjShade(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)

{
	/******************
	* obj_shade id id 
	*/
	
	OBtObject *objects[AIcMaxObjectsOfOneID];
	UUtUns32 i, count;
	UUtInt32 p0, p1;
	float r,g,b;
	UUtUns32 shade;

	p0 = inParameterList[0].val.i;
	p1 = inParameterList[1].val.i;

	r = UUmPin(inParameterList[2].val.f, 0, 1.f);
	g = UUmPin(inParameterList[3].val.f, 0, 1.f);
	b = UUmPin(inParameterList[4].val.f, 0, 1.f);

	shade = 0xFF000000;
	shade |= (MUrUnsignedSmallFloat_To_Uns_Round(r * 255) << 16);
	shade |= (MUrUnsignedSmallFloat_To_Uns_Round(g * 255) << 8);
	shade |= (MUrUnsignedSmallFloat_To_Uns_Round(b * 255) << 0);

	AIiObjectsFromScript(p0, p1, &count, objects);

	for (i=0; i < count; i++)
	{
		objects[i]->flags |= OBcFlags_FlatLighting;
		objects[i]->flat_lighting_shade = shade;
	}
	
	return UUcError_None;
}

static UUtError
AIiScript_ObjCreate(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)

{
	/******************
	* obj_create id id
	*/
		
	
	UUtUns32 i;
	UUtUns32 id_low;
	UUtUns32 id_high;

	if (1 == inParameterListLength) {
		id_low = inParameterList[0].val.i;
		id_high = inParameterList[0].val.i;
	}
	else {
		UUtUns32 i0 = (UUtUns32) inParameterList[0].val.i;
		UUtUns32 i1 = (UUtUns32) inParameterList[1].val.i;

		id_low = UUmMin(i0, i1);
		id_high = UUmMax(i0, i1);
	}
	
	for (i=0; i < ONgGameState->level->objectSetupArray->numObjects; i++)
	{
		OBtObjectSetup *setup = ONgGameState->level->objectSetupArray->objects + i;

		if ((setup->scriptID >= id_low) && (setup->scriptID <= id_high)) {		
			OBtObject *object;

			if (!(setup->flags & OBcFlags_InUse)) continue;

			object = OBrList_Add(ONgGameState->objects);
			OBrInit(object,setup);			
		}
	}
	
	return UUcError_None;
}
	

static UUtError
AIiScript_EnvSetAnim_Internal(
	SLtErrorContext			*inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue,
	UUtBool					inBlock)
{
	/****************
	* env_setanim object newanim
	*/
	
	OBtObject *objects[AIcMaxObjectsOfOneID];
	UUtUns32 i,count;
	UUtError error;
	char *name = inParameterList[1].val.str;
	UUtInt32 p0;

	p0 = inParameterList[0].val.i;

	AIiObjectsFromScript(p0, p0, &count, objects);

	if (0 == count) {
		SLrScript_ReportError(inErrorContext, "env_setanim cannot find specified object");
		return UUcError_Generic;
	}

	// verify the objects and stall if blocked
	for(i = 0; i < count; i++)
	{
		const PHtPhysicsContext *verify_physics = objects[i]->physics;

		if (NULL == objects[i]->physics)
		{
			SLrScript_ReportError(inErrorContext, "env_anim could not load physics for object %d", inParameterList[0].val.i);
			return UUcError_Generic;
		}

		if (verify_physics->animContext.animation != NULL) {
			if (verify_physics->animContext.animContextFlags & OBcAnimContextFlags_Animate) {
				if (inBlock && ((verify_physics->animContext.animationFrame + 1) < verify_physics->animContext.animation->numFrames)) {
					*outStalled = UUcTrue;
					return UUcError_None;
				}
			}
		}
	}

	for (i=0; i < count; i++)
	{
		error = OBrSetAnimationByName(objects[i], name);
		
		if (error != UUcError_None) {
			SLrScript_ReportError(inErrorContext, "Couldn't load animation for scripted swap");
			return error;
		}

		OBrAnim_Start(&objects[i]->physics->animContext);
	}

	return UUcError_None;
}

static UUtError
AIiScript_EnvSetAnim_Block(
	SLtErrorContext			*inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError error;

	error = AIiScript_EnvSetAnim_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStalled, ioReturnValue, UUcTrue);

	return error;
}

static UUtError
AIiScript_EnvSetAnim_NoBlock(
	SLtErrorContext			*inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError error;

	error = AIiScript_EnvSetAnim_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStalled, ioReturnValue, UUcFalse);

	return error;
}

static UUtError
AIiScript_EnvAnim_Internal(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue,
	UUtBool					inBlock)
{
	/****************
	* env_anim object_id object_id
	*/
	
	OBtObject *objects[AIcMaxObjectsOfOneID];
	UUtUns32 count;
	UUtUns32 i;
	UUtInt32 p0, p1;

	*outStalled = UUcFalse;

	p0 = inParameterList[0].val.i;
	p1 = (inParameterListLength >= 2) ? inParameterList[1].val.i : p0;

	AIiObjectsFromScript(p0, p1, &count, objects);

	// verify the objects and stall if blocked
	for(i = 0; i < count; i++)
	{
		const PHtPhysicsContext *verify_physics = objects[i]->physics;

		if (NULL == objects[i]->physics)
		{
			SLrScript_ReportError(inErrorContext, "env_anim could not load physics for object %d", inParameterList[0].val.i);
			return UUcError_Generic;
		}

		if (inParameterList[1].val.i) {
			if (verify_physics->animContext.animContextFlags & OBcAnimContextFlags_Animate) {
				if (verify_physics->animContext.animation != NULL) {
					if (inBlock && ((verify_physics->animContext.animationFrame + 1) < verify_physics->animContext.animation->numFrames)) {
						*outStalled = UUcTrue;
						return UUcError_None;
					}
				}
			}

		}
	}

	for (i = 0; i < count; i++)
	{
		OBrAnim_Start(&objects[i]->physics->animContext);
	}

	return UUcError_None;
}

static UUtError
AIiScript_EnvAnim_NoBlock(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError error;

	error = AIiScript_EnvAnim_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStalled, ioReturnValue, UUcFalse);

	return error;
}

static UUtError
AIiScript_EnvAnim_Block(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtError error;

	error = AIiScript_EnvAnim_Internal(inErrorContext, inParameterListLength, inParameterList, outTicksTillCompletion, outStalled, ioReturnValue, UUcTrue);

	return error;
}

static UUtError
AIiScript_Letterbox(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/**********
	* letterbox [0|1]
	*/
	
	UUtUns16	state = (UUtUns16)inParameterList[0].val.i;

	if (state) {
		ONrGameState_LetterBox_Start(&ONgGameState->local.letterbox);
	}
	else {
		ONrGameState_LetterBox_Stop(&ONgGameState->local.letterbox);
	}

	return UUcError_None;
}

static UUtError
AIiScript_BeginCutScene(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32 itr;
	UUtUns32 count = inParameterListLength;
	UUtUns32 flags = 0;

	for(itr = 0; itr < count; itr++)
	{
		const char *string;

		if (inParameterList[itr].type != SLcType_String) {
			COrConsole_Printf("invalid begin_cutscene flag (must be a string)");
			continue;
		}

		string = inParameterList[itr].val.str;

		if (0 == UUrString_Compare_NoCase_NoSpace(string, "weapon")) {
			flags |= ONcCutsceneFlag_RetainWeapon;
		}
		else if (0 == UUrString_Compare_NoCase_NoSpace(string, "jello")) {
			flags |= ONcCutsceneFlag_RetainJello;
		}
		else if (0 == UUrString_Compare_NoCase_NoSpace(string, "ai")) {
			flags |= ONcCutsceneFlag_RetainAI;
		}
		else if (0 == UUrString_Compare_NoCase_NoSpace(string, "invis")) {
			flags |= ONcCutsceneFlag_RetainInvisibility;
		}
		else if (0 == UUrString_Compare_NoCase_NoSpace(string, "nosync")) {
			flags |= ONcCutsceneFlag_DontSync;
		}
		else if (0 == UUrString_Compare_NoCase_NoSpace(string, "keepparticles")) {
			flags |= ONcCutsceneFlag_DontKillParticles;
		}
		else if (0 == UUrString_Compare_NoCase_NoSpace(string, "animation")) {
			flags |= ONcCutsceneFlag_RetainAnimation;
		}
		else if (0 == UUrString_Compare_NoCase_NoSpace(string, "nojump")) {
			flags |= ONcCutsceneFlag_NoJump;
		}
		else {
			COrConsole_Printf("unknown begin_cutscene flag %s", string);
			COrConsole_Printf("  valid flags: weapon jello ai invis nosync keepparticles animation nojump");
		}
	}

	ONrGameState_BeginCutscene(flags);

	return UUcError_None;
}

static UUtError
AIiScript_EndCutScene(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	ONrGameState_EndCutscene();

	return UUcError_None;
}

static UUtError
AIiScript_CutsceneSyncMark(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	if (0 != inParameterListLength) {
		if (SLcType_String == inParameterList[0].type) {
			const char *string = inParameterList[0].val.str;

			if (0 == UUrString_Compare_NoCase_NoSpace(string, "mark")) {
				// do nothing, this is automatic
			}
			else if (0 == UUrString_Compare_NoCase_NoSpace(string, "on")) {
				ONgGameState->local.sync_enabled = UUcTrue;
			}
			else if (0 == UUrString_Compare_NoCase_NoSpace(string, "off")) {
				ONgGameState->local.sync_enabled = UUcFalse;
			}
			else {
				COrConsole_Printf("invalid parameter to cutscene_sync");
			}
		}
	}

	ONrGameState_Cutscene_Sync_Mark();

	return UUcError_None;
}

static UUtError
AIiScript_Input(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/**************
	* input [0/1]
	*/
	
	UUtUns16 value;

	value = (UUtUns16)inParameterList[0].val.i;

	if (value) {
		ONgGameState->inputEnable = UUcTrue;
	}
	else {
		ONgGameState->inputEnable = UUcFalse;
	}

	return UUcError_None;
}

static UUtError
AIiScript_FadeIn(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/****************
	* fade_in ticks
	*/

	ONrGameState_FadeIn(inParameterList[0].val.i);

	return UUcError_None;
}

static UUtError
AIiScript_FadeOut(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/****************
	* fade_out r g b ticks
	*/

	UUtUns32	numFrames;
	float		r;
	float		g;
	float		b;
	
	r = (inParameterList[0].type == SLcType_Int32) ? inParameterList[0].val.i : inParameterList[0].val.f;
	g = (inParameterList[1].type == SLcType_Int32) ? inParameterList[1].val.i : inParameterList[1].val.f;
	b = (inParameterList[2].type == SLcType_Int32) ? inParameterList[2].val.i : inParameterList[2].val.f;
	
	numFrames = inParameterList[3].val.i;

	r = UUmPin(r, 0.f, 1.f);
	g = UUmPin(g, 0.f, 1.f);
	b = UUmPin(b, 0.f, 1.f);

	ONrGameState_FadeOut(numFrames, r, g, b);

	return UUcError_None;
}

static UUtError
AIiScript_EnvTexSwap(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/****************
	* env_texswap gqstart texname
	*/
	
	UUtError error;
	UUtUns32 gqS,i,gq;
	
	M3tTextureMap *texture;
	AKtEnvironment *env = ONgGameState->level->environment;
	char texname[AIcMaxScriptName];

	COrConsole_Printf("obsolete env_texswap called");
	
	gqS = inParameterList[0].val.i;
	
	for (i=0; i<=env->envQuadRemaps->numIndices; i++) 
	{
		UUtUns16 texture_map_index;

		if (env->envQuadRemaps->indices[i] != gqS) continue;
		gq = env->envQuadRemapIndices->indices[i];

		// Load the named texture
		UUrString_Copy(texname,	inParameterList[1].val.str,AIcMaxScriptName);
		UUrString_StripExtension(texname);
		UUrString_Capitalize(texname, AIcMaxScriptName);
		
		error =
			TMrInstance_GetDataPtr(
				M3cTemplate_TextureMap,
				texname,
				&texture);
		
		if (error != UUcError_None)
		{
			SLrScript_ReportError(inErrorContext, "caCould not load texture map \"%s\" for scripted swap", texname);
			return UUcError_Generic;
		}
	
		// Find the index for that texture
		for (texture_map_index = 0; texture_map_index < env->textureMapArray->numMaps; texture_map_index++)
		{
			if (env->textureMapArray->maps[texture_map_index] == texture) break;
		}
	
		UUmAssert(texture_map_index < env->textureMapArray->numMaps);

		env->gqRenderArray->gqRender[gq].textureMapIndex = texture_map_index;
	}

	return UUcError_None;
}

static UUtError
AIiScript_EnvShow(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/****************
	* env_show ref [0/1]
	*/
	
	UUtUns32 ref,i,gq;
	UUtBool t;
	AKtEnvironment *env = ONgGameState->level->environment;
	
	ref = inParameterList[0].val.i;
	t = (UUtBool)inParameterList[1].val.i;
	
	for (i=0; i<=env->envQuadRemaps->numIndices; i++) 
	{
		if (env->envQuadRemaps->indices[i] != ref) continue;
		gq = env->envQuadRemapIndices->indices[i];

		if (t) {
			env->gqGeneralArray->gqGeneral[gq].flags &=~ AKcGQ_Flag_BrokenGlass;
		}
		else {
			env->gqGeneralArray->gqGeneral[gq].flags |= AKcGQ_Flag_BrokenGlass;
		}
	}

	AKrEnvironment_GunkChanged();

	return UUcError_None;
}

#define AIcEnvBroken_BVLength		32

static UUtError
AIiScript_EnvBroken(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/****************
	* env_broken ref
	*/
	
	UUtUns32 refstart,refend,i,gq,flags,bv_index;
	UUtUns32 unbroken_bv[AIcEnvBroken_BVLength];
	UUtUns32 num_unbroken, num_broken;
	AKtEnvironment *env = ONgGameState->level->environment;

	ioReturnValue->type = SLcType_Int32;
	ioReturnValue->val.i = 0;

	refstart = inParameterList[0].val.i;
	if (inParameterListLength >= 2) {
		refend = inParameterList[1].val.i;
	} else {
		refend = refstart;
	}
	
	if (refend >= refstart + 32 * AIcEnvBroken_BVLength) {
		COrConsole_Printf("### env_broken: trying to read objects %d-%d, exceeded max object count of %d",
							refstart, refend, 32 * AIcEnvBroken_BVLength);
		return UUcError_None;
	}

	num_unbroken = 0;
	UUrBitVector_ClearBitAll(unbroken_bv, AIcEnvBroken_BVLength);
	for (i = 0; i <= env->envQuadRemaps->numIndices; i++) 
	{
		if ((env->envQuadRemaps->indices[i] < refstart) || (env->envQuadRemaps->indices[i] > refend))
			continue;

		gq = env->envQuadRemapIndices->indices[i];

		flags = env->gqGeneralArray->gqGeneral[gq].flags;
		if ((flags & AKcGQ_Flag_Breakable) && ((flags & AKcGQ_Flag_BrokenGlass) == 0)) {
			// this quad is still intact, which of the objects does it belong to?
			bv_index = env->envQuadRemaps->indices[i] - refstart;
			UUmAssert((bv_index >= 0) && (bv_index < 32 * AIcEnvBroken_BVLength));

			if (UUrBitVector_TestAndSetBit(unbroken_bv, bv_index) == 0) {
				// mark this object as unbroken
				num_unbroken++;
			}
		}
	}

	// compute the number of broken objects
	num_broken = ((refend - refstart) + 1) - num_unbroken;
	ioReturnValue->val.i = UUmMax(0, num_broken);

	return UUcError_None;
}

static UUtError
AIiScript_EnvShade(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	/****************
	* env_shade ref ref r g b
	*/
	
	UUtUns32 ref_low;
	UUtUns32 ref_high;
	UUtUns32 shade;
	UUtUns32 i;

	AKtEnvironment *env = ONgGameState->level->environment;
	
	ref_low = UUmMin(inParameterList[0].val.i, inParameterList[1].val.i);
	ref_high = UUmMax(inParameterList[0].val.i, inParameterList[1].val.i);

	{
		float r;
		float g;
		float b;

		r = UUmPin(inParameterList[2].val.f, 0, 1.f);
		g = UUmPin(inParameterList[3].val.f, 0, 1.f);
		b = UUmPin(inParameterList[4].val.f, 0, 1.f);

		shade = 0xFF000000;
		shade |= (MUrUnsignedSmallFloat_To_Uns_Round(r * 255) << 16);
		shade |= (MUrUnsignedSmallFloat_To_Uns_Round(g * 255) << 8);
		shade |= (MUrUnsignedSmallFloat_To_Uns_Round(b * 255) << 0);
	}
	
	for (i=0; i<=env->envQuadRemaps->numIndices; i++) 
	{
		if ((env->envQuadRemaps->indices[i] >= ref_low) && (env->envQuadRemaps->indices[i] <= ref_high)) {
			UUtUns32 gq_index;
			AKtGQ_General *gq;

			gq_index = env->envQuadRemapIndices->indices[i];
			gq = env->gqGeneralArray->gqGeneral + gq_index;

			gq->m3Quad.shades[0] = shade;
			gq->m3Quad.shades[1] = shade;
			gq->m3Quad.shades[2] = shade;
			gq->m3Quad.shades[3] = shade;
		}
	}

	return UUcError_None;
}


static UUtError
AIiScript_Win(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	ONgGameState->victory = ONcWin;

	return UUcError_None;
}

static UUtError
AIiScript_Lose(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	ONgGameState->victory = ONcLose;

	return UUcError_None;
}

static UUtError
AIiScript_SlowMotion(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtInt32 new_timer_value = inParameterList[0].val.i;

	UUmAssert(1 == inParameterListLength);
	UUmAssert(SLcType_Int32 == inParameterList[0].type);

	if (0 == new_timer_value) {
		ONgGameState->local.slowMotionTimer = 0;
	}
	else {
		ONgGameState->local.slowMotionTimer = UUmMax(ONgGameState->local.slowMotionTimer, new_timer_value);
	}

	return UUcError_None;
}

static UUtError
AIiScript_Timer_Start(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	float one_hour_in_frames = (UUtUns32) (60 * 60 * 60);
	UUtUns32 time_in_frames = MUrUnsignedSmallFloat_To_Uns_Round(UUmPin(inParameterList[0].val.f * 60.f, 0.f, one_hour_in_frames));
	char *script_name = inParameterList[1].val.str;

	ONrGameState_Timer_Start(script_name, time_in_frames);

	return UUcError_None;
}

static UUtError
AIiScript_Timer_Stop(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	ONrGameState_Timer_Stop();

	return UUcError_None;
}

static UUtError
AIiScript_SaveGame(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	UUtBool autosave = UUcFalse;

	if ((inParameterListLength >= 2) && UUmString_IsEqual(inParameterList[1].val.str, "autosave")) {
		autosave = UUcTrue;
	}

	COrConsole_Printf("%s game %d", (autosave ? "autosave" : "save"), inParameterList[0].val.i);

	ONrGameState_MakeContinue(inParameterList[0].val.i, autosave);

	return UUcError_None;
}

static UUtError
AIiScript_RestoreGame(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	ONrGameState_UseContinue();

	return UUcError_None;
}


// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================

static UUtError
AIiScript_DPrint(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	char *string = inParameterList[0].val.str;

	COrConsole_Printf("%s", string);

	return UUcError_None;
}

static UUtError
AIiScript_DMsg(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	char *string = inParameterList[0].val.str;

	COrMessage_Print(string, NULL, 600);

	return UUcError_None;
}

static UUtError AIiScript_Message(
	SLtErrorContext			*inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	const char *message_tag = inParameterList[0].val.str;
	const char *message;
	UUtUns32 timer = 0;

	if (inParameterListLength >= 2) {
		timer = (UUtUns32) inParameterList[1].val.i;
	}

	// first: try finding this message exactly as specified, in the messages file
	message = SSrMessage_Find(message_tag);
	if (message != NULL) {
		COrMessage_Print(message, message_tag, timer);
		return UUcError_None;
	}

	// next: try finding this message in the subtitles file. strip the first character (e.g. c01_02_50training)
	message = SSrSubtitle_Find(message_tag+1);
	if (message != NULL) {
		COrConsole_PrintIdentified(COcPriority_Content, COgDefaultTextShade, COgDefaultTextShadowShade, message, message_tag, timer);
		return UUcError_None;
	}

	// unable to print this message
	COrConsole_Printf("failed to find message %s", message_tag);
	return UUcError_None;
}

static UUtError AIiScript_MessageRemove(
	SLtErrorContext			*inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	const char *message_tag = NULL;
	
	if (inParameterListLength > 0) {
		message_tag = inParameterList[0].val.str;
	}

	// first try removing the message, then try removing the subtitle
	if (COrMessage_Remove(message_tag)) {
		return UUcError_None;
	}

	if (message_tag != NULL) {
		COrConsole_Remove(message_tag);
	}

	return UUcError_None;
}

static UUtError AIiScript_SplashScreen(
	SLtErrorContext			*inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	const char *texture_name;

	if ((inParameterListLength != 1) || (inParameterList[0].type != SLcType_String)) {
		COrConsole_Printf("splash_screen: expected name of splash screen");
	}

	{
		texture_name = inParameterList[0].val.str;

		{
			UUtUns32 max_string_length = sizeof(ONgGameState->local.pending_splash_screen) - 1;

			UUmAssert(0 == strlen(""));

			if (strlen(texture_name) > max_string_length) {
				COrConsole_Printf("splash_screen: texture name %s exceeded max length %d", texture_name, max_string_length);
			}
		}

		strcpy(ONgGameState->local.pending_splash_screen, texture_name);
	}

	return UUcError_None;
}

static UUtError AIiScript_LockKeys(
	SLtErrorContext			*inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual		*inParameterList,
	UUtUns32				*outTicksTillCompletion,
	UUtBool					*outStalled,
	SLtParameter_Actual		*ioReturnValue)
{
	typedef struct LockKeyTable
	{
		const char *match_string;
		UUtUns64 mask;
	} LockKeyTable;

	LockKeyTable lock_key_table[] =
	{
		{ "keys_movement",	LIc_BitMask_Forward | LIc_BitMask_Backward | LIc_BitMask_StepLeft | LIc_BitMask_StepRight },
		{ "keys_jump", LIc_BitMask_Jump },
		{ "keys_crouch", LIc_BitMask_Crouch | LIc_BitMask_Fire3 },
		{ "keys_attack", LIc_BitMask_Punch | LIc_BitMask_Kick | LIc_BitMask_Fire1 | LIc_BitMask_Fire2 },
		{ "keys_pause", LIc_BitMask_PauseScreen },
		{ "keys_action", LIc_BitMask_Action },
		{ "keys_inventory", LIc_BitMask_Swap | LIc_BitMask_Drop },
		{ "keys_walk", LIc_BitMask_Walk },
		{ "keys_hypo", LIc_BitMask_Hypo },
		{ "keys_reload", LIc_BitMask_Reload },
		{ "keys_all", 0xFFFFFFFFFFFFFFFF },
		{ NULL, 0 }
	};

	if (0 == inParameterListLength) {
		ONgGameState->key_mask = LIc_BitMask_ScreenShot | LIc_BitMask_Console | LIc_BitMask_Escape;
	}
	else if ((inParameterListLength != 1) || (inParameterList[0].type != SLcType_String)) {
		COrConsole_Printf("lock_keys: expected command of what keys to lock");
	}
	else {
		UUtInt32 itr;

		for(itr = 0; lock_key_table[itr].match_string != NULL; itr++)
		{
			if (0 == UUrString_Compare_NoCase_NoSpace(lock_key_table[itr].match_string, inParameterList[0].val.str)) {
				ONgGameState->key_mask |= lock_key_table[itr].mask;

				goto exit;
			}
		}

		COrConsole_Printf("lock_keys failed, legal commands are:");
		COrConsole_Printf("lock_keys");

		for(itr = 0; lock_key_table[itr].match_string != NULL; itr++)
		{
			COrConsole_Printf("lock_keys %s", lock_key_table[itr].match_string);
		}
	}

exit:
	return UUcError_None;
}



UUtError AIrScript_Initialize(
	void)
{
	/****************
	* Initializes script AI for a game
	*/
	UUtError				error;
	SLtRegisterVoidFunctionTable	table[] =
	{		
		{ "cm_anim", "initiates a camera animation", "cam_spec:string{\"move\" | \"look\" | \"both\"} anim_name:string", AIiScript_CameraAnim_NoBlock },
		{ "cm_anim_block", "initiates a camera animation", "cam_spec:string{\"move\" | \"look\" | \"both\"} anim_name:string", AIiScript_CameraAnim_Block },

		{ "cm_interpolate", "initiates a camera interpolation", "anim_name:string num_frames:int", AIiScript_CameraInterpolate_NoBlock },
		{ "cm_interpolate_block", "initiates a camera interpolation", "anim_name:string num_frames:int", AIiScript_CameraInterpolate_Block },

		{ "cm_orbit", "puts the camera in orbit mode", "speed:float [stopangle:float | ]", AIiScript_CameraOrbit_NoBlock },
		{ "cm_orbit_block", "puts the camera in orbit mode", "speed:float [stopangle:float| ]", AIiScript_CameraOrbit_Block },

		{ "cm_detach", "detaches the camera", "", AIiScript_DetachCamera },

		{ "cm_wait", "makes the camera wait until it is no longer busy", "", AIiScrip_CameraWait },

		{ "cm_reset", "resets the camera", "[maxspeed:float | ] [maxFocalAccel:float | ]",  AIiScript_CameraReset},

		{ "obj_hide", "turns display of an object on or off", "obj_id:int [ obj_id:int | ]", AIiScript_ObjHide },
		{ "obj_show", "turns display of an object on or off", "obj_id:int [ obj_id:int | ]", AIiScript_ObjShow },
		{ "obj_force_draw", "locks an object as visible", "obj_id:int [ obj_id:int | ]", AIiScript_ForceDraw },

		{ "obj_shade", "turns display of an object on or off", "obj_id:int obj_id:int r:float g:float b:float", AIiScript_ObjShade },

		{ "obj_kill", "kills a range of object", "obj_id:int [ obj_id:int | ]", AIiScript_ObjKill },
		{ "obj_create", "creates a range of objects", "obj_id:int [ obj_id:int | ]", AIiScript_ObjCreate },

		{ "env_anim", "initiates a environment animation: blocks until completed", "obj_id:int [ obj_id:int | ]", AIiScript_EnvAnim_NoBlock },
		{ "env_anim_block", "initiates a environment animation: blocks until completed", "obj_id:int [ obj_id:int | ]", AIiScript_EnvAnim_Block },

		{ "env_setanim", "sets up an animation for an object", "obj_id:int object_name:string", AIiScript_EnvSetAnim_NoBlock },
		{ "env_setanim_block", "sets up an animation for an object", "obj_id:int object_name:string", AIiScript_EnvSetAnim_Block },
		
		{ "letterbox", "starts or stops letterboxing", "start_stop:int{0 | 1}",	AIiScript_Letterbox },
		{ "input", "turns input on or off", "on_off:int{0 | 1}", AIiScript_Input },
		{ "fade_in", "fades the screen in", "ticks:int", AIiScript_FadeIn },
		{ "fade_out", "fades the screen out", "[r:float | r:int] [g:float | g:int] [b:float | b:int] ticks:int", AIiScript_FadeOut },
		{ "env_texswap", "swaps an environment texture", "gq_start:int tex_name:string", AIiScript_EnvTexSwap },
		{ "env_show", "turns on or off specified parts of the environment", "gq_ref:int on_off:int{0 | 1}", AIiScript_EnvShow },
		{ "env_shade", "sets the shade of a block of objects", "gq_ref:int gq_ref:int r:float g:float b:float", AIiScript_EnvShade },
		{ "begin_cutscene", "begins a cutscene", NULL, AIiScript_BeginCutScene },
		{ "end_cutscene", "ends a cutscene", "", AIiScript_EndCutScene },
		{ "cutscene_sync", "marks a point in a cutscene", NULL, AIiScript_CutsceneSyncMark },		
		{ "dprint", "debugging print", "astring:string", AIiScript_DPrint },
		{ "dmsg", "debugging message", "astring:string", AIiScript_DMsg },
		{ "win", "win this level", NULL, AIiScript_Win },
		{ "lose", "lose this level", NULL, AIiScript_Lose },
		{ "slowmo", "starts the slowmotion timer", "duration:int", AIiScript_SlowMotion },
		{ "timer_start", "starts the countdown timer", "duration:float script:string", AIiScript_Timer_Start },
		{ "timer_stop", "starts the countdown timer", "", AIiScript_Timer_Stop },

		{ "save_game", "saves the game", "save_point:int [type:string{\"autosave\"} | ]", AIiScript_SaveGame },
		{ "restore_game", "restores the game", "", AIiScript_RestoreGame },


		{ "message", "sends a message from the subtitle file", "message:string [timer:int]", AIiScript_Message },
		{ "message_remove", "removes a currently displayed message", "[message:string | ]", AIiScript_MessageRemove },

		{ "splash_screen", "displays a splash screen", "texture:string", AIiScript_SplashScreen },
		{ "lock_keys", "locks keys out", NULL, AIiScript_LockKeys },

		{ NULL, NULL, NULL, NULL }
	};

	SLrScript_Command_Register_ReturnType("env_broken", "computes the number of objects in a range that have all their glass broken", "gq_ref:int [gq_endref:int | ]", SLcType_Int32, AIiScript_EnvBroken);

	error = SLrScript_CommandTable_Register_Void(table);

	return UUcError_None;
}

void AIrScript_Terminate(
	void)
{
	/*******************
	* Game shutdown
	*/
	
	return;
}
