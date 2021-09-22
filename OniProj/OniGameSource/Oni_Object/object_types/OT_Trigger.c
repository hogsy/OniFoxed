// ======================================================================
// OT_Trigger.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_ScriptLang.h"
#include "BFW_TextSystem.h"

#include "Oni_Object.h"
#include "Oni_Object_Private.h"
#include "Oni_BinaryData.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Mechanics.h"
#include "Oni_Sound2.h"

#include "Oni_Script.h"

#include "Oni_FX.h"

#include "BFW_ScriptLang.h"

#include <ctype.h>
#include "EulerAngles.h"

// ======================================================================
// constants
// ======================================================================

#define OBJcTriggerSelectAll	0xFFFF
#define OBJcMaxTriggerEmitters	32

// ======================================================================
// globals
// ======================================================================

static UUtBool					OBJgTrigger_DrawTriggers;
static float OBJgTrigger_SetSpeedValue = 0.f;
// ======================================================================
// function definitons
// ======================================================================

UUtError OBJrTrigger_Reset( OBJtObject* inObject );
static void OBJiTrigger_GetBoundingSphere( const OBJtObject *inObject, M3tBoundingSphere *outBoundingSphere);
static UUtError OBJiTrigger_SetOSD( OBJtObject *inObject, const OBJtOSD_All *inOSD);
static void OBJiTrigger_UpdatePosition(OBJtObject *inObject);
static void OBJrTrigger_Hide(OBJtObject *inObject);
static void OBJrTrigger_Show(OBJtObject *inObject);
static void OBJrTrigger_SetSpeed(OBJtObject *inObject, float inSpeed);

static void OBJiTrigger_StartActiveSound(OBJtObject *inObject, OBJtOSD_Trigger *ioTriggerOSD);
static void OBJiTrigger_StopActiveSound(OBJtObject *inObject, OBJtOSD_Trigger *ioTriggerOSD);

// ======================================================================
// functions
// ======================================================================

static UUtBool OBJrTrigger_ActivateTurrets_Enum( const OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Turret *turret_osd;

	UUmAssert( inObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) inObject->object_data;
	if( turret_osd->id == inUserData )
	{
		OBJrTurret_Activate( (OBJtObject*) inObject );
	}
	return UUcTrue;
}

// ----------------------------------------------------------------------
static void OBJrTrigger_OnTrigger( OBJtObject* inObject, ONtCharacter *inCharacter )
{
	OBJtOSD_Trigger*		trigger_osd;
	UUtError				error;

	trigger_osd = (OBJtOSD_Trigger*)inObject->object_data;
	
	if( trigger_osd->flags & OBJcTriggerFlag_Triggered )
		return;

	trigger_osd->flags |= OBJcTriggerFlag_Triggered;
	
	COrConsole_Printf("Trigger Triggered (%d)", trigger_osd->id );

	if ((trigger_osd->trigger_class != NULL) && (trigger_osd->trigger_class->trigger_sound != NULL)) {
		OSrImpulse_Play(trigger_osd->trigger_class->trigger_sound, &inObject->position, NULL, NULL, NULL);
	}

	error = ONrEventList_Execute( &trigger_osd->event_list, inCharacter );
	UUmAssert( error == UUcError_None );

}
// ----------------------------------------------------------------------
static void OBJiTrigger_CreateBoundingSphere( OBJtObject *inObject, OBJtOSD_Trigger *inOSD )
{
	// set the bounding sphere
	inOSD->bounding_sphere = inOSD->trigger_class->base_geometry->pointArray->boundingSphere;
}

// ----------------------------------------------------------------------
static void OBJiTrigger_Delete( OBJtObject *inObject )
{
	OBJtOSD_Trigger *trigger_osd;

	trigger_osd= (OBJtOSD_Trigger*) inObject->object_data;
	
	OBJiTrigger_StopActiveSound(inObject, trigger_osd);
	
	if (trigger_osd->flags & OBJcTriggerFlag_Initialized)
	{
		UUmAssert(trigger_osd->anim_points);

		UUrMemory_Block_Delete(trigger_osd->anim_points);
				
		trigger_osd->anim_points= NULL;
		trigger_osd->flags&= ~OBJcTriggerFlag_Initialized;
	}

	if (trigger_osd->emitters)
	{
		UUrMemory_Block_Delete(trigger_osd->emitters);
	}

	if (trigger_osd->cached_points)
	{
		UUrMemory_Block_Delete(trigger_osd->cached_points);
		trigger_osd->cached_points= NULL;
	}

	ONrEventList_Destroy(&trigger_osd->event_list);

	return;
}

static void OBJiTrigger_DrawLaserDot(M3tPoint3D *inPoint)
{
	UUtError error;
	M3tTextureMap *dotTexture;
	float scale = .003f;
	float cam_sphere_dist;
	M3tPoint3D cameraLocation;

	M3rCamera_GetViewData(ONgActiveCamera, &cameraLocation, NULL, NULL);

	error = TMrInstance_GetDataPtr(
		M3cTemplate_TextureMap, 
		"dot", 
		(void **) &dotTexture);
	UUmAssert(UUcError_None == error);

	if (error) {
		return;
	}

	cam_sphere_dist = MUmVector_GetDistance(
		cameraLocation,
		(*inPoint));

	if (cam_sphere_dist > .001f) {
		scale *= cam_sphere_dist;

		M3rSimpleSprite_Draw(dotTexture, inPoint, scale, scale, IMcShade_Red, M3cMaxAlpha);
	}

	return;
}
// ----------------------------------------------------------------------

static void OBJiTrigger_Force_UpdateAnimation(OBJtOSD_Trigger *trigger_osd)
{
	OBtAnimation *anim = trigger_osd->trigger_class->animation;
	UUtUns32 i;

	if (NULL == anim) {
		return;
	}

	for( i = 0; i < trigger_osd->emitter_count; i++ )
	{
		OBJtTriggerEmitterInstance	*instance		= &trigger_osd->emitters[i];
		float						anim_frame		= instance->anim_frame;
		OBtAnimationContext			*anim_context	= &instance->anim_context;

		anim_context->frameStartTime = ONgGameState->gameTime;

		instance->anim_frame += (float)anim_context->animationStep * trigger_osd->current_anim_scale;

		if (instance->anim_frame >= anim->numFrames || (anim_context->animationStop && anim_frame >= anim_context->animationStop))
		{
			if (trigger_osd->flags & OBJcTriggerFlag_PingPong) {
				instance->anim_frame = (float) anim->numFrames - 1;
				anim_context->animationStep = -1;
			}
			else {
				instance->anim_frame = 0;
			}
		}

		if (instance->anim_frame < 0)
		{
			if (trigger_osd->flags & OBJcTriggerFlag_PingPong) {
				instance->anim_frame = 0;
				anim_context->animationStep = 1;
			}
			else {
				instance->anim_frame = (float) anim->numFrames - 1;
			}
		}
		// put floating frame into anim context
		anim_context->animationFrame = (UUtInt16) instance->anim_frame;

		OBrAnim_Matrix(anim_context, &instance->matrix);
	}
}

static void OBJiTrigger_UpdateAnimation( OBJtObject *inObject )
{
	OBJtOSD_Trigger *trigger_osd = (OBJtOSD_Trigger*)inObject->object_data;

	if (trigger_osd->flags & OBJcTriggerFlag_Active ) {
		OBJiTrigger_Force_UpdateAnimation(trigger_osd);
	}

	return;
}

static void OBJiTrigger_Update(OBJtObject *inObject)
{
	OBJtOSD_Trigger			*trigger_osd;

	trigger_osd = (OBJtOSD_Trigger*)inObject->object_data;

	// update sound
	if (((trigger_osd->flags & OBJcTriggerFlag_Hidden) == 0) && (trigger_osd->flags & OBJcTriggerFlag_Active)) {
		OBJiTrigger_StartActiveSound(inObject, trigger_osd);
	} else {
		OBJiTrigger_StopActiveSound(inObject, trigger_osd);
	}

	if ((trigger_osd->flags & OBJcTriggerFlag_PlayingSound) && (trigger_osd->playing_sound != SScInvalidID)) {
		OSrAmbient_Update(trigger_osd->playing_sound, &inObject->position, NULL, NULL);
	}

	// update animation
	if( trigger_osd->trigger_class->animation )
	{
		OBJiTrigger_UpdateAnimation( inObject );
	}

	// update times
	if( trigger_osd->change_time )
	{
		trigger_osd->change_time--;
	}
	else if( trigger_osd->time_off || trigger_osd->time_on )
	{
		if( trigger_osd->laser_on )
		{
			if( trigger_osd->time_off )
			{
				trigger_osd->laser_on		= UUcFalse;
				trigger_osd->change_time	= trigger_osd->time_off;
			}
		}
		else
		{
			if( trigger_osd->time_on )
			{
				trigger_osd->laser_on		= UUcTrue;
				trigger_osd->change_time	= trigger_osd->time_on;
			}
		}
	}
}

static UUtBool cached_trigger_points_dirty= UUcTrue;

void OBJrTrigger_Dirty(
	void)
{
	cached_trigger_points_dirty= UUcTrue;

	return;
}

void OBJrTrigger_Clean(
	void)
{
	cached_trigger_points_dirty= UUcFalse;

	return;
}

static void OBJrTrigger_GetWorldPoint(OBJtOSD_Trigger *trigger_osd, M3tPoint3D *from, M3tVector3D *vector, UUtUns32 i, M3tPoint3D *outPoint)
{
#if 1
	// we save every possible ray endpoint so that we only have to do the
	// expensive collision test against the environment once
	{
		OBJtTriggerEmitterInstance *instance= &trigger_osd->emitters[i];
		
		if (instance)
		{
			UUtUns16 current_frame_index= (UUtInt16)instance->anim_frame;

			if ((trigger_osd->cached_points[current_frame_index].x == 0.f) &&
				(trigger_osd->cached_points[current_frame_index].y == 0.f) &&
				(trigger_osd->cached_points[current_frame_index].z == 0.f))
			{
				// calculate the point and save it
				if (AKrCollision_Point(ONgGameState->level->environment, from, vector, AKcGQ_Flag_Obj_Col_Skip, 1))
				{
					trigger_osd->cached_points[current_frame_index] = AKgCollisionList[0].collisionPoint;
				}
			}

			// use cached point
			*outPoint= trigger_osd->cached_points[current_frame_index];
		}
		else
		{
			// this is probably something terrible but pretend it isn't
			UUmAssert(0);

			if (AKrCollision_Point(ONgGameState->level->environment, from, vector, AKcGQ_Flag_Obj_Col_Skip, 1))
			{
				*outPoint = AKgCollisionList[0].collisionPoint;
			}
		}
	}
#else
	if (AKrCollision_Point(ONgGameState->level->environment, &world_points[0], &world_dir, AKcGQ_Flag_Obj_Col_Skip, 1)) {
		world_points[1] = AKgCollisionList[0].collisionPoint;
		hit_player_character = UUcFalse;
	}
#endif
}


// ----------------------------------------------------------------------
static void OBJiTrigger_Draw( OBJtObject *inObject, UUtUns32 inDrawFlags)
{
	OBJtTriggerEmitterClass	*emitter;
	OBJtOSD_Trigger			*trigger_osd;
	UUtUns32				i;
	M3tPoint3D				laserFrom;
	M3tPoint3D				laserTo;
	M3tPoint3D				world_points[2];

	M3tMatrix4x3			matrix_stack[2];

	
	if (OBJgTrigger_DrawTriggers == UUcFalse) { return; }
	// don't draw when the non-occluding geometry is hidden
	if (AKgDraw_Occl == UUcTrue) { return; }
	
	trigger_osd = (OBJtOSD_Trigger*)inObject->object_data;

	UUmAssert( trigger_osd->trigger_class && trigger_osd->trigger_class->emitter && trigger_osd->trigger_class->base_geometry );

	emitter = trigger_osd->trigger_class->emitter;
	

	MUrMatrix_BuildTranslate(inObject->position.x, inObject->position.y, inObject->position.z, matrix_stack + 0);
	MUrMatrixStack_Matrix(matrix_stack + 0, &trigger_osd->rotation_matrix);

	M3rGeom_State_Commit();

#if TOOL_VERSION
	// dont draw the rail if its been gunkified
	if (!(inObject->flags & OBJcObjectFlag_Gunk)) {
		AKrEnvironment_DrawIfVisible(trigger_osd->trigger_class->base_geometry, matrix_stack + 0);
	}
#endif

	if (trigger_osd->flags & OBJcTriggerFlag_Hidden) {
		// don't draw
	}
	else {
		for( i = 0; i < trigger_osd->emitter_count; i++ )
		{
			matrix_stack[1] = matrix_stack[0];

			{
				MUrMatrixStack_Matrix(matrix_stack + 1, &trigger_osd->emitters[i].matrix);
		
				AKrEnvironment_DrawIfVisible_Point(trigger_osd->trigger_class->emitter->geometry, matrix_stack + 1);

				// draw the laser if active
				if ((trigger_osd->flags & OBJcTriggerFlag_Active) && trigger_osd->laser_on)
				{
					M3tVector3D world_dir;
					
					// shoot ray to find endpoint
					{	
						UUtBool hit_player_character = UUcFalse;
						M3tVector3D laserVector;
						
						laserFrom	= emitter->emit_position;
						laserVector = emitter->emit_vector;

						MUmVector_Normalize(laserVector);

						MUmVector_Add( world_points[1], laserFrom, laserVector);
						
						world_points[0] = laserFrom;

						MUrMatrix_MultiplyPoints(2, matrix_stack + 1, world_points, world_points);

						// velocity and position
						MUmVector_Subtract(world_dir, world_points[1], world_points[0]);
						
						// do the collision
						{
							M3tPoint3D environment_end_point;
							MUrVector_SetLength(&world_dir, 500.f);

							OBJrTrigger_GetWorldPoint(trigger_osd, world_points + 0, &world_dir, i, &environment_end_point);
							world_points[1] = environment_end_point;

							MUmVector_Subtract(world_dir, environment_end_point, world_points[0]);

							if (ONgGameState->local.playerActiveCharacter != NULL) {
								if (ONrCharacter_Collide_Ray(ONgGameState->local.playerCharacter, &world_points[0], &world_dir, UUcFalse, NULL, NULL, &(world_points[1]), NULL)) {

									// we only hit the player if this end point is close then the environment end point
									if (MUmVector_GetDistanceSquared(world_points[0], world_points[1]) > MUmVector_GetDistanceSquared(world_points[0], environment_end_point)) {
										world_points[1] = environment_end_point;
									}
									else {
										hit_player_character = UUcTrue;
									}
								}
							}
						}

						{
							float offsetAmt = 2.f;
							float distance;
							float uAmt = 1.f;

							if (hit_player_character) {
								OBJrTrigger_OnTrigger( inObject, ONgGameState->local.playerCharacter);
							}
							else  {
								if (trigger_osd->flags & OBJcTriggerFlag_Triggered) {
									trigger_osd->flags &= ~OBJcTriggerFlag_Triggered;
								}
							}

							distance = MUmVector_GetDistance(world_points[1], world_points[0]);

							if (distance > UUcEpsilon) {
								uAmt = offsetAmt / distance;
								uAmt = UUmPin(uAmt, 0.f, 1.f);
								uAmt = 1.f - uAmt;
							}
							
							MUrLineSegment_ComputePoint(&world_points[0], &world_points[1], uAmt, &world_points[1]);

							MUmVector_Scale( laserVector, distance * uAmt );
							MUmVector_Add(laserTo, laserFrom, laserVector);
						}

						{
							M3tBoundingBox_MinMax laser_bbox;

							laser_bbox.minPoint.x = UUmMin(world_points[0].x, world_points[1].x) - 1.f;
							laser_bbox.minPoint.y = UUmMin(world_points[0].y, world_points[1].y) - 1.f;
							laser_bbox.minPoint.z = UUmMin(world_points[0].z, world_points[1].z) - 1.f;

							laser_bbox.maxPoint.x = UUmMax(world_points[0].x, world_points[1].x) + 1.f;
							laser_bbox.maxPoint.y = UUmMax(world_points[0].y, world_points[1].y) + 1.f;
							laser_bbox.maxPoint.z = UUmMax(world_points[0].z, world_points[1].z) + 1.f;

							//if (AKrEnvironment_IsBoundingBoxMinMaxVisible(&laser_bbox))
							{
								UUtUns32 color= trigger_osd->color;
							
							#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
								color= ((color&0xFF000000)>>24)|((color& 0x00FF0000)>>8)|((color&0x0000FF00)<<8)|((color&0x000000FF)<<24);
							#endif
								FXrDrawLaser(world_points + 0, world_points + 1, color);
							}
						}
					}
				}
			}
		}
	}


#if TOOL_VERSION
	// selected object stuff....
	if (inDrawFlags & OBJcDrawFlag_Selected)
	{
		M3tBoundingBox			bBox;

		M3rMatrixStack_Push();

		// draw laser destination points (points are world space)
		if( trigger_osd->flags & OBJcTriggerFlag_Active && trigger_osd->flags & OBJcTriggerFlag_Initialized )
		{
			M3rGeometry_LineDraw((UUtUns16) trigger_osd->trigger_class->animation->numFrames, trigger_osd->anim_points, IMcShade_Blue);
			M3rMinMaxBBox_To_BBox( &trigger_osd->bounding_box, &bBox);	
			M3rBBox_Draw_Line(&bBox, IMcShade_Yellow );
		}

		// apply position and orientation
		M3rMatrixStack_ApplyTranslate(inObject->position);
		M3rMatrixStack_Multiply(&trigger_osd->rotation_matrix);

		// Draw the base boxes
		M3rMinMaxBBox_To_BBox( &trigger_osd->trigger_class->base_geometry->pointArray->minmax_boundingBox, &bBox);			
		M3rBBox_Draw_Line(&bBox, IMcShade_White);

		// draw the rings
		OBJrObjectUtil_DrawRotationRings(inObject, &trigger_osd->bounding_sphere, inDrawFlags);

		// draw the animation points
		if( trigger_osd->trigger_class->animation )
		{
			if (trigger_osd->trigger_class->animation->numFrames > 0) 
			{
				M3tPoint3D		*anim_points = NULL;
				anim_points = UUrMemory_Block_New( sizeof(M3tPoint3D) * trigger_osd->trigger_class->animation->numFrames);
				for(i = 0; i < trigger_osd->trigger_class->animation->numFrames; i++) 
				{
					M3tMatrix4x3	anim_matrix;
					OBrAnimation_GetMatrix(trigger_osd->trigger_class->animation, i, 0, &anim_matrix);
					anim_points[i] = MUrMatrix_GetTranslation(&anim_matrix);
				}
				M3rGeometry_LineDraw((UUtUns16) trigger_osd->trigger_class->animation->numFrames, anim_points, IMcShade_White);
				UUrMemory_Block_Delete( anim_points );
			}
		}

		// draw emitter boxes
		for( i = 0; i < trigger_osd->emitter_count; i++ )
		{
			M3rMatrixStack_Push();			
			M3rMatrixStack_Multiply(&trigger_osd->emitters[i].matrix);
			M3rMinMaxBBox_To_BBox( &trigger_osd->trigger_class->emitter->geometry->pointArray->minmax_boundingBox, &bBox);
			M3rBBox_Draw_Line(&bBox, IMcShade_White);
			M3rMatrixStack_Pop();
		}

		M3rMatrixStack_Pop();
	}
#endif
}

// ----------------------------------------------------------------------
static UUtError
OBJiTrigger_Enumerate( OBJtObject *inObject, OBJtEnumCallback_ObjectName inEnumCallback, UUtUns32 inUserData )
{
	return OBJrObjectUtil_EnumerateTemplate( "", OBJcTemplate_TriggerClass, inEnumCallback, inUserData);
}

// ----------------------------------------------------------------------
static void OBJiTrigger_GetBoundingSphere( const OBJtObject *inObject, M3tBoundingSphere *outBoundingSphere)
{
	OBJtOSD_Trigger		*trigger_osd;
	
	trigger_osd = (OBJtOSD_Trigger*)inObject->object_data;

	*outBoundingSphere = trigger_osd->bounding_sphere;
}

// ----------------------------------------------------------------------
static void OBJiTrigger_OSDGetName( const OBJtOSD_All *inObject, char *outName, UUtUns32 inNameLength)
{
	const OBJtOSD_Trigger *trigger_osd = &inObject->osd.trigger_osd;
	
	sprintf(outName, "%s_%d", trigger_osd->trigger_class_name, trigger_osd->id);

	return;
}
// ----------------------------------------------------------------------
static void OBJiTrigger_GetOSD( const OBJtObject *inObject, OBJtOSD_All *outOSD)
{
	OBJtOSD_Trigger			*trigger_osd;

	trigger_osd		= (OBJtOSD_Trigger*)inObject->object_data;
	
	UUrMemory_MoveFast( trigger_osd, &outOSD->osd.trigger_osd, sizeof(OBJtOSD_Trigger) );
	
	ONrEventList_Copy( &trigger_osd->event_list, &outOSD->osd.trigger_osd.event_list );
}

// ----------------------------------------------------------------------
static UUtBool
OBJiTrigger_IntersectsLine(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inStartPoint,
	const M3tPoint3D		*inEndPoint)
{
	M3tBoundingSphere		sphere;

	UUrMemory_Clear(&sphere, sizeof(M3tBoundingSphere));
	
	// get the bounding sphere
	OBJiTrigger_GetBoundingSphere(inObject, &sphere);
	
	sphere.center.x += inObject->position.x;
	sphere.center.y += inObject->position.y;
	sphere.center.z += inObject->position.z;
	
	return CLrSphere_Line(inStartPoint, inEndPoint, &sphere);
}

// ----------------------------------------------------------------------
static UUtError OBJiTrigger_SetDefaults(OBJtOSD_All *outOSD)
{
	UUtError				error;	
	void					*instances[OBJcMaxInstances];
	UUtUns32				num_instances;
	char					*instance_name;

	// clear the osd
	UUrMemory_Clear(&outOSD->osd.trigger_osd, sizeof(OBJtOSD_Trigger));
	
	// get a list of instances of the class
	error = TMrInstance_GetDataPtr_List( OBJcTemplate_TriggerClass, OBJcMaxInstances, &num_instances, instances);
	UUmError_ReturnOnError(error);

	// copy the name of the first trigger instance into the osd_all.			
	instance_name = TMrInstance_GetInstanceName(instances[0]);
			
	UUrString_Copy(outOSD->osd.trigger_osd.trigger_class_name,instance_name,OBJcMaxNameLength);

	// default initial data
	outOSD->osd.trigger_osd.id					= ONrMechanics_GetNextID( OBJcType_Trigger );
	outOSD->osd.trigger_osd.flags				= OBJcTriggerFlag_InitialActive | OBJcTriggerFlag_Active;
	outOSD->osd.trigger_osd.color				= 0xFFFF0000;
	outOSD->osd.trigger_osd.persistant_anim_scale = 1.f;
	outOSD->osd.trigger_osd.current_anim_scale	= 1.f;
	outOSD->osd.trigger_osd.start_offset		= 0;
	outOSD->osd.trigger_osd.time_on				= 0;
	outOSD->osd.trigger_osd.time_off			= 0;
	outOSD->osd.trigger_osd.emitter_count		= 1;
	outOSD->osd.trigger_osd.playing_sound		= SScInvalidID;

	// initialize the event list
	ONrEventList_Initialize( &outOSD->osd.trigger_osd.event_list );

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError OBJiTrigger_New( OBJtObject *inObject, const OBJtOSD_All *inOSD )
{
	OBJtOSD_All				osd_all;
	UUtError				error;
	
	if (inOSD == NULL)
	{
		error = OBJiTrigger_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);

		inOSD = &osd_all;
	}
	else
	{
		osd_all = *inOSD;
	}

	// set the object specific data and position
	error = OBJiTrigger_SetOSD(inObject, &osd_all);
	UUmError_ReturnOnError(error);
	
	OBJiTrigger_UpdatePosition(inObject);

	UUrMemory_Block_VerifyList();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiTrigger_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_All				osd_all;
	UUtUns32				num_bytes;

	num_bytes = 0;

	// read the data
	OBJmGetStringFromBuffer(inBuffer, osd_all.osd.trigger_osd.trigger_class_name,	OBJcMaxNameLength, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, osd_all.osd.trigger_osd.id,					UUtUns16,	inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, osd_all.osd.trigger_osd.flags,				UUtUns16,	inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, osd_all.osd.trigger_osd.color,				UUtUns32,	inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, osd_all.osd.trigger_osd.start_offset,			float,		inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, osd_all.osd.trigger_osd.persistant_anim_scale,	float,		inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, osd_all.osd.trigger_osd.emitter_count,		UUtUns16,	inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, osd_all.osd.trigger_osd.time_on,				UUtUns16,	inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, osd_all.osd.trigger_osd.time_off,				UUtUns16,	inSwapIt);

	num_bytes = 
		OBJcMaxNameLength + 
		sizeof(UUtUns16) +
		sizeof(UUtUns16) + 
		sizeof(UUtUns32) + 
		sizeof(float) + 
		sizeof(float) +
		sizeof(UUtUns16) +
		sizeof(UUtUns16) +
		sizeof(UUtUns16);

	// initialize and read the event list
	ONrEventList_Initialize( &osd_all.osd.trigger_osd.event_list );
	num_bytes += ONrEventList_Read( &osd_all.osd.trigger_osd.event_list, inVersion, inSwapIt, inBuffer );

	// bring the object up to date
	OBJiTrigger_SetOSD(inObject, &osd_all);
	
	ONrEventList_Destroy(&osd_all.osd.trigger_osd.event_list);
	
	OBJiTrigger_UpdatePosition(inObject);

	return num_bytes;
}
// ----------------------------------------------------------------------
static UUtError
OBJiTrigger_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	
	OBJtOSD_Trigger			*trigger_osd;
	UUtUns32				bytes_available;

	// get a pointer to the object osd
	trigger_osd = (OBJtOSD_Trigger*)inObject->object_data;
	
	// set the number of bytes available
	bytes_available = *ioBufferSize;
	
	// put the trigger name, shade, id, and note
	OBJmWriteStringToBuffer(ioBuffer, trigger_osd->trigger_class_name,	OBJcMaxNameLength, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, trigger_osd->id,					UUtInt16,	bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, trigger_osd->flags,				UUtUns16,	bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, trigger_osd->color,				UUtInt32,	bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, trigger_osd->start_offset,		float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, trigger_osd->persistant_anim_scale,	float,	bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, trigger_osd->emitter_count,		UUtInt16,	bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, trigger_osd->time_on,				UUtInt16,	bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, trigger_osd->time_off,			UUtInt16,	bytes_available, OBJcWrite_Little);
	
	ONrEventList_Write( &trigger_osd->event_list, ioBuffer, &bytes_available );

	*ioBufferSize -= bytes_available;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32 OBJiTrigger_GetOSDWriteSize( const OBJtObject *inObject )
{
	UUtUns32				size;
	OBJtOSD_Trigger			*trigger_osd;

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;
	
	size = 
		OBJcMaxNameLength +
		sizeof(UUtUns16) +
		sizeof(UUtUns16) + 
		sizeof(UUtUns32) + 
		sizeof(float) + 
		sizeof(float) +
		sizeof(UUtUns16) +
		sizeof(UUtUns16) +
		sizeof(UUtUns16);

	size += ONrEventList_GetWriteSize( &trigger_osd->event_list );

	return size;
}

// - ---------------------------------------------------------------------
static void OBJiTrigger_StartActiveSound(OBJtObject *inObject, OBJtOSD_Trigger *ioTriggerOSD)
{
	if ((ioTriggerOSD->flags & OBJcTriggerFlag_PlayingSound) == 0) {

		if ((ioTriggerOSD->trigger_class != NULL) && (ioTriggerOSD->trigger_class->active_sound != NULL)) {
			ioTriggerOSD->playing_sound = OSrAmbient_Start(ioTriggerOSD->trigger_class->active_sound, &inObject->position, NULL, NULL, NULL, NULL);
		} else {
			ioTriggerOSD->playing_sound = SScInvalidID;
		}
		
		ioTriggerOSD->flags |= OBJcTriggerFlag_PlayingSound;
	}
}

// - ---------------------------------------------------------------------
static void OBJiTrigger_StopActiveSound(OBJtObject *inObject, OBJtOSD_Trigger *ioTriggerOSD)
{
	if (ioTriggerOSD->flags & OBJcTriggerFlag_PlayingSound) {
		// stop any sounds that the trigger is currently playing
		if (ioTriggerOSD->playing_sound != SScInvalidID) {
			OSrAmbient_Stop(ioTriggerOSD->playing_sound);
		}

		ioTriggerOSD->flags &= ~OBJcTriggerFlag_PlayingSound;
	}

	ioTriggerOSD->playing_sound = SScInvalidID;
}

// - ---------------------------------------------------------------------
static UUtError OBJiTrigger_SetOSD( OBJtObject *inObject, const OBJtOSD_All *inOSD)
{
	UUtError				error;
	OBJtTriggerClass		*trigger_class;
	OBJtOSD_Trigger			*trigger_osd;

	UUmAssert(inOSD);

	// get a pointer to the object osd
	trigger_osd = (OBJtOSD_Trigger*)inObject->object_data;
	
	OBJiTrigger_StopActiveSound(inObject, trigger_osd);

	// copy persistant data
	UUrString_Copy(trigger_osd->trigger_class_name, inOSD->osd.trigger_osd.trigger_class_name, OBJcMaxNameLength);
	trigger_osd->id				= inOSD->osd.trigger_osd.id;
	trigger_osd->flags			= inOSD->osd.trigger_osd.flags & OBJcTriggerFlag_Persist;
	trigger_osd->start_offset	= inOSD->osd.trigger_osd.start_offset;
	trigger_osd->persistant_anim_scale	= inOSD->osd.trigger_osd.persistant_anim_scale;
	trigger_osd->current_anim_scale = trigger_osd->persistant_anim_scale;
	trigger_osd->emitter_count  = inOSD->osd.trigger_osd.emitter_count;
	trigger_osd->time_on		= inOSD->osd.trigger_osd.time_on;
	trigger_osd->time_off		= inOSD->osd.trigger_osd.time_off;
	trigger_osd->color			= inOSD->osd.trigger_osd.color;

	ONrEventList_Copy( &((OBJtOSD_All*)inOSD)->osd.trigger_osd.event_list, &trigger_osd->event_list );

	// clip ranges
	trigger_osd->emitter_count	= UUmPin( trigger_osd->emitter_count, 1, OBJcMaxTriggerEmitters );	// 0 -> 32
	trigger_osd->start_offset	= UUmPin( trigger_osd->start_offset,  0, 1 );				// 0 -> 1.0
	trigger_osd->persistant_anim_scale		= UUmPin( trigger_osd->persistant_anim_scale,	  0, 100 );				// 0 -> 100 times
	trigger_osd->current_anim_scale		= UUmPin( trigger_osd->current_anim_scale,	  0, 100 );				// 0 -> 100 times
	trigger_osd->time_on		= UUmPin( trigger_osd->time_on,		  0, 60 * 60 * 100 );	// 0 -> 100 min
	trigger_osd->time_off		= UUmPin( trigger_osd->time_off,	  0, 60 * 60 * 100 );	// 0 -> 100 min

	// alloc space for emitters
	trigger_osd->emitters = UUrMemory_Block_Realloc( trigger_osd->emitters, sizeof(OBJtTriggerEmitterInstance) * trigger_osd->emitter_count );

	// grab the template
	error = TMrInstance_GetDataPtr( OBJcTemplate_TriggerClass, trigger_osd->trigger_class_name, &trigger_class );	
	if( error != UUcError_None )
	{
		UUrDebuggerMessage("failed to find trigger class %s", trigger_osd->trigger_class_name);

		error = TMrInstance_GetDataPtr_ByNumber(OBJcTemplate_TriggerClass, 0, &trigger_class);

		UUmError_ReturnOnErrorMsg(error, "failed to find any trigger class");

		inObject->flags				|= OBJcObjectFlag_Temporary;		
		trigger_osd->trigger_class	= NULL;
	}

	trigger_osd->trigger_class = trigger_class;

	// clear the initialized bit
	trigger_osd->flags	&= ~OBJcTriggerFlag_Initialized;

	// build the position matrix
	OBJiTrigger_UpdatePosition(inObject);
	
	// create the bounding sphere
	OBJiTrigger_CreateBoundingSphere(inObject, trigger_osd);

	// reset everything (build emitter instances)
	OBJrTrigger_Reset( inObject );

	// force an update of the animation
	OBJiTrigger_Force_UpdateAnimation(trigger_osd);

	// S.S. allocate space for cached points
	if (trigger_osd->cached_points == NULL)
	{
		UUtUns16 num_frames= (UUtUns16)trigger_osd->trigger_class->animation->numFrames;
		trigger_osd->cached_points= (M3tPoint3D *)UUrMemory_Block_NewClear(num_frames * sizeof(M3tPoint3D));
		UUmAssert(trigger_osd->cached_points != NULL);
		// I was just noticing how we never check for allocation failures.
		// I think I will go modify our memory allocation functions to do that right now...
		// OK I'm done. Back to work!
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void OBJiTrigger_UpdatePosition(OBJtObject *inObject)
{
	
	OBJtOSD_Trigger			*trigger_osd;
	float					rot_x;
	float					rot_y;
	float					rot_z;
	MUtEuler				euler;
	
	// get a pointer to the object osd
	trigger_osd = (OBJtOSD_Trigger*)inObject->object_data;
	
	// convert the rotation to radians
	rot_x = inObject->rotation.x * M3cDegToRad;
	rot_y = inObject->rotation.y * M3cDegToRad;
	rot_z = inObject->rotation.z * M3cDegToRad;
	
	// update the rotation matrix
	euler.order = MUcEulerOrderXYZs;
	euler.x = rot_x;
	euler.y = rot_y;
	euler.z = rot_z;
	MUrEulerToMatrix(&euler, &trigger_osd->rotation_matrix);

	// delete precomputed data
	if( trigger_osd->flags & OBJcTriggerFlag_Initialized )
	{
		UUmAssert( trigger_osd->anim_points );

		UUrMemory_Block_Delete( trigger_osd->anim_points );

		trigger_osd->anim_points = NULL;
		trigger_osd->flags &= ~OBJcTriggerFlag_Initialized;
	}
}
// ----------------------------------------------------------------------
static UUtBool OBJiTrigger_GetVisible(void)
{
	return OBJgTrigger_DrawTriggers;
}

// ----------------------------------------------------------------------
static UUtBool OBJiTrigger_Search( const OBJtObject *inObject, const UUtUns32 inSearchType, const OBJtOSD_All *inSearchParams)
{
	
	OBJtOSD_Trigger			*trigger_osd;
	UUtBool					found;
	
	// get a pointer to the object osd
	trigger_osd = (OBJtOSD_Trigger*)inObject->object_data;
	
	// perform the check
	found = UUcFalse;
	switch (inSearchType)
	{
		case OBJcSearch_TriggerID:
			if (trigger_osd->id == inSearchParams->osd.trigger_osd.id)
			{
				found = UUcTrue;
			}
		break;
	}
	return found;
}

// ----------------------------------------------------------------------
static void OBJiTrigger_SetVisible( UUtBool inIsVisible )
{
	OBJgTrigger_DrawTriggers = inIsVisible;
}

// ----------------------------------------------------------------------

static UUtError OBJrTrigger_Activate( OBJtObject *inObject )
{
	OBJtOSD_Trigger *trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;
	
	trigger_osd->flags		|= OBJcTriggerFlag_Active;
	trigger_osd->laser_on	= UUcTrue;

	if( trigger_osd->time_off || trigger_osd->time_on )
	{
		trigger_osd->change_time	= trigger_osd->time_on;
	}
	else
	{
		trigger_osd->change_time	= 0;
	}

	return UUcError_None;
}

static UUtError OBJrTrigger_Deactivate( OBJtObject *inObject )
{
	OBJtOSD_Trigger *trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;
	
	trigger_osd->flags &= ~OBJcTriggerFlag_Active;

	return UUcError_None;
}

static UUtBool OBJrTrigger_Activate_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Trigger *trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;
	
	if( trigger_osd->id == inUserData )
	{
		OBJrTrigger_Activate( (OBJtObject*) inObject );
	}
	return UUcTrue;
}

static UUtBool OBJrTrigger_Deactivate_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Trigger *trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;

	if( trigger_osd->id == inUserData )
	{
		OBJrTrigger_Deactivate( (OBJtObject*) inObject );
	}

	return UUcTrue;
}

UUtError OBJrTrigger_Reset(OBJtObject* inObject)
{
	UUtUns16						i;
	float							space;
	OBJtTriggerEmitterInstance		*instance;
	OBJtOSD_Trigger					*trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;

	// setup time
	trigger_osd->laser_on = UUcTrue;		

	if( trigger_osd->time_on || trigger_osd->time_off )
		trigger_osd->change_time = trigger_osd->time_on;
	else
		trigger_osd->change_time = 0;

	// reset the animation
	if( trigger_osd->trigger_class->animation )
	{
		// setup emitters
		space = (float)trigger_osd->trigger_class->animation->numFrames / (float)trigger_osd->emitter_count;
		for( i = 0; i < trigger_osd->emitter_count; i++ )
		{
			instance								= &trigger_osd->emitters[i];			
			instance->anim_context.animation		= trigger_osd->trigger_class->animation;
			instance->anim_context.animationFrame	= 0;
			instance->anim_context.animationStep	= 1;
			instance->anim_context.animationStop	= 0;
			instance->anim_context.animContextFlags = 0;
			instance->anim_context.frameStartTime	= 0;
			OBrAnim_Start(&instance->anim_context);

			//instance->anim_frame					= (float) fmod( ( (float) (space * i) + ( trigger_osd->start_offset * trigger_osd->trigger_class->animation->numFrames )), trigger_osd->trigger_class->animation->numFrames );	
			if( trigger_osd->start_offset == 1.0f ) {
				instance->anim_frame					= trigger_osd->trigger_class->animation->numFrames - 1.f;
			}
			else {
				instance->anim_frame					= (float) fmod( ( (float) (space * i) + ( trigger_osd->start_offset * (trigger_osd->trigger_class->animation->numFrames - 1) )), (float)(trigger_osd->trigger_class->animation->numFrames-1));
			}

			if ( trigger_osd->flags & OBJcTriggerFlag_ReverseAnim ) {
				instance->anim_context.animationStep = -1;
			}
			else {
				instance->anim_context.animationStep = 1;
			}
		}
	}

	// clear the triggered bit
	trigger_osd->flags &= ~OBJcTriggerFlag_Triggered;

	// set the active bit to the inital state
	if (trigger_osd->flags & OBJcTriggerFlag_InitialActive) {
		OBJrTrigger_Activate(inObject);
	} else {
		OBJrTrigger_Deactivate(inObject);
	}

	// do an initial update
	OBJiTrigger_Update((OBJtObject*) inObject);

	OBJiTrigger_UpdateAnimation( inObject );

	return UUcError_None;
}

static void OBJrTrigger_Hide(OBJtObject *inObject)
{
	OBJtOSD_Trigger					*trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;

	trigger_osd->flags |= OBJcTriggerFlag_Hidden;

	return;
}

static void OBJrTrigger_Show(OBJtObject *inObject)
{
	OBJtOSD_Trigger					*trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;

	trigger_osd->flags &= ~OBJcTriggerFlag_Hidden;

	return;
}

static void OBJrTrigger_SetSpeed(OBJtObject *inObject, float inSpeed)
{
	OBJtOSD_Trigger					*trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;

	trigger_osd->current_anim_scale = trigger_osd->persistant_anim_scale * inSpeed;

	return;
}


static UUtBool OBJrTrigger_Reset_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Trigger *trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;

	if( inUserData == OBJcTriggerSelectAll || trigger_osd->id == inUserData )
	{
		OBJrTrigger_Reset(inObject);
	}

	return UUcTrue;
}

static UUtBool OBJrTrigger_Show_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Trigger *trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;

	if( inUserData == OBJcTriggerSelectAll || trigger_osd->id == inUserData )
	{
		OBJrTrigger_Show(inObject);
	}

	return UUcTrue;
}

static UUtBool OBJrTrigger_Hide_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Trigger *trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;

	if( inUserData == OBJcTriggerSelectAll || trigger_osd->id == inUserData )
	{
		OBJrTrigger_Hide(inObject);
	}

	return UUcTrue;
}

static UUtBool OBJrTrigger_SetSpeed_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Trigger *trigger_osd;

	UUmAssert( inObject->object_type == OBJcType_Trigger );

	trigger_osd = (OBJtOSD_Trigger*) inObject->object_data;

	if( inUserData == OBJcTriggerSelectAll || trigger_osd->id == inUserData )
	{
		OBJrTrigger_SetSpeed(inObject, OBJgTrigger_SetSpeedValue);
	}

	return UUcTrue;
}

void OBJrTrigger_Activate_ID( UUtUns16 inID )
{
	if (inID == (UUtUns16) -1) {
		inID = OBJcTriggerSelectAll;
	}

	OBJrObjectType_EnumerateObjects( OBJcType_Trigger, OBJrTrigger_Activate_Enum, (UUtUns32) inID ); 

	return;
}

void OBJrTrigger_Deactivate_ID( UUtUns16 inID )
{
	if (inID == (UUtUns16) -1) {
		inID = OBJcTriggerSelectAll;
	}

	OBJrObjectType_EnumerateObjects( OBJcType_Trigger, OBJrTrigger_Deactivate_Enum, (UUtUns32) inID ); 

	return;
}

void OBJrTrigger_Reset_ID( UUtUns16 inID )
{
	if (inID == (UUtUns16) -1) {
		inID = OBJcTriggerSelectAll; 
	}

	OBJrObjectType_EnumerateObjects( OBJcType_Trigger, OBJrTrigger_Reset_Enum, (UUtUns32) inID ); 

	return;
}

void OBJrTrigger_Hide_ID(UUtUns16 inID)
{
	if (inID == (UUtUns16) -1) {
		inID = OBJcTriggerSelectAll;
	}

	OBJrObjectType_EnumerateObjects( OBJcType_Trigger, OBJrTrigger_Hide_Enum, (UUtUns32) inID ); 

	return;
}

void OBJrTrigger_Show_ID(UUtUns16 inID)
{
	if (inID == (UUtUns16) -1) {
		inID = OBJcTriggerSelectAll;
	}

	OBJrObjectType_EnumerateObjects( OBJcType_Trigger, OBJrTrigger_Show_Enum, (UUtUns32) inID ); 

	return;
}

void OBJrTrigger_SetSpeed_ID(UUtUns16 inID, float inSpeed)
{
	if (inID == (UUtUns16) -1) {
		inID = OBJcTriggerSelectAll;
	}

	OBJgTrigger_SetSpeedValue = inSpeed;

	OBJrObjectType_EnumerateObjects( OBJcType_Trigger, OBJrTrigger_SetSpeed_Enum, (UUtUns32) inID ); 

	return;
}


// ----------------------------------------------------------------------

void OBJrTrigger_AddEvent( OBJtOSD_Trigger *inTrigger_osd, ONtEvent *inEvent )
{	
	ONrEventList_AddEvent( &inTrigger_osd->event_list, inEvent );

	return;
}

void OBJrTrigger_DeleteEvent( OBJtOSD_Trigger *inTrigger_osd, UUtUns32 inIndex )
{	
	ONrEventList_DeleteEvent( &inTrigger_osd->event_list, inIndex );

	return;
}

static UUtUns16 OBJrTrigger_GetID( OBJtObject *inObject )
{
	OBJtOSD_Trigger			*osd;
	UUmAssert( inObject && inObject->object_data );
	osd = (OBJtOSD_Trigger*)inObject->object_data;
	return osd->id;
}

extern UUtBool		FXgLaser_UseAlpha;
// ----------------------------------------------------------------------
UUtError OBJrTrigger_Initialize(void)
{
	
	UUtError					error;
	OBJtMethods					methods;
	ONtMechanicsMethods			mechanics_methods;
	
	// intialize the globals
	OBJgTrigger_DrawTriggers			= UUcTrue;
	
	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));
	
	// set up the methods structure
	methods.rNew						= OBJiTrigger_New;
	methods.rSetDefaults				= OBJiTrigger_SetDefaults;
	methods.rDelete						= OBJiTrigger_Delete;
	methods.rDraw						= OBJiTrigger_Draw;
	methods.rEnumerate					= OBJiTrigger_Enumerate;
	methods.rGetBoundingSphere			= OBJiTrigger_GetBoundingSphere;
	methods.rOSDGetName					= OBJiTrigger_OSDGetName;
	methods.rIntersectsLine				= OBJiTrigger_IntersectsLine;
	methods.rUpdatePosition				= OBJiTrigger_UpdatePosition;
	methods.rGetOSD						= OBJiTrigger_GetOSD;
	methods.rGetOSDWriteSize			= OBJiTrigger_GetOSDWriteSize;
	methods.rSetOSD						= OBJiTrigger_SetOSD;
	methods.rRead						= OBJiTrigger_Read;
	methods.rWrite						= OBJiTrigger_Write;
	
	// set up the type methods
	methods.rGetClassVisible			= OBJiTrigger_GetVisible;
	methods.rSearch						= OBJiTrigger_Search;
	methods.rSetClassVisible			= OBJiTrigger_SetVisible;
	
	// set up the mechanics methods
	mechanics_methods.rInitialize		= NULL;
	mechanics_methods.rTerminate		= NULL;
	mechanics_methods.rReset			= OBJrTrigger_Reset;
	mechanics_methods.rUpdate			= OBJiTrigger_Update;
	mechanics_methods.rGetID			= OBJrTrigger_GetID;

 	mechanics_methods.rClassLevelBegin	= NULL;
	mechanics_methods.rClassLevelEnd 	= NULL;
	mechanics_methods.rClassReset		= NULL;
	mechanics_methods.rClassUpdate		= NULL;
	
	// register the trigger methods
	error = ONrMechanics_Register( OBJcType_Trigger, OBJcTypeIndex_Trigger, "Trigger", sizeof(OBJtOSD_Trigger), &methods, 0, &mechanics_methods );
	UUmError_ReturnOnError(error);

#if CONSOLE_DEBUGGING_COMMANDS
	error = SLrGlobalVariable_Register_Bool( "show_triggers", "Enables the display of triggers", &OBJgTrigger_DrawTriggers);
	UUmError_ReturnOnError(error);

	error = SLrGlobalVariable_Register_Bool( "laser_alpha", "Enables the display of triggers", &FXgLaser_UseAlpha);
	UUmError_ReturnOnError(error);
#endif

	return UUcError_None;
}

void OBJrTrigger_UpdateSoundPointers(void)
{
	UUtUns32 itr, num_classes;
	OBJtTriggerClass *trigger_class;
	UUtError error;

	num_classes = TMrInstance_GetTagCount(OBJcTemplate_TriggerClass);

	for (itr = 0; itr < num_classes; itr++) {
		error = TMrInstance_GetDataPtr_ByNumber(OBJcTemplate_TriggerClass, itr, (void **) &trigger_class);
		if (error == UUcError_None) {
			trigger_class->active_sound  = OSrAmbient_GetByName(trigger_class->active_soundname);
			trigger_class->trigger_sound = OSrImpulse_GetByName(trigger_class->trigger_soundname);
		}
	}
}

