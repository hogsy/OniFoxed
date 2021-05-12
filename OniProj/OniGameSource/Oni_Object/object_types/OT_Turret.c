// ======================================================================
// OT_Turret.c
// =====================================================================

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
#include "Oni_Weapon.h"
#include "Oni_Mechanics.h"
#include "Oni_Sound2.h"

#include "Oni_AI2.h"
#include "oni_fx.h"

#include <ctype.h>
#include "EulerAngles.h"

// ======================================================================
// defines
// ======================================================================

#define OBJcTurretSelectAll			0xFFFF
#define OBJcMaxTurrets				32

#define OBJcTurret_NotVisibleTime	120

// ======================================================================
// globals
// ======================================================================

static UUtBool					OBJgTurret_DrawTurretDebugInfo;
static UUtBool					OBJgTurret_DrawTurrets;

// ======================================================================
// prototypes
// ======================================================================

void OBJrTurretDisplayDebuggingInfo(OBJtObject *inObject);

static void OBJiTurretCombat_CheckTargetingWeapon(OBJtObject *ioObject, AI2tCombatState *ioCombatState);
UUtError OBJrTurret_SetupParticles( OBJtObject *inObject );
UUtError OBJrTurret_DestroyParticles( OBJtObject *inObject );
UUtError OBJrTurret_Reset( OBJtObject *inObject );
static void OBJiTurret_Shutdown( OBJtObject *inObject );
static void OBJiTurret_SendParticleEvent( OBJtObject *inObject, UUtUns16 inEvent, UUtUns16 inShooter);

void OBJiTurretCombat_Exit(OBJtObject *ioObject);
void OBJiTurretCombat_Update(OBJtObject *ioObject);
void OBJiTurretCombat_Enter(OBJtObject *ioObject);
UUtBool OBJiTurret_FireProjectile( OBJtObject *inObject, UUtUns16 shooter_index );

static void OBJiTurret_GetBoundingSphere( const OBJtObject *inObject, M3tBoundingSphere *outBoundingSphere );
static UUtError OBJiTurret_SetOSD( OBJtObject *inObject, const OBJtOSD_All *inOSD );
static void OBJiTurret_UpdatePosition(OBJtObject *inObject);

static void OBJiTurret_TargetingCallback_Fire(AI2tTargetingState *inTargetingState, UUtBool inFire);
static void OBJiTurret_TargetingCallback_GetPosition(AI2tTargetingState *inTargetingState, M3tPoint3D *inPoint);
static void OBJiTurret_TargetingCallback_AimVector(AI2tTargetingState *inTargetingState, M3tVector3D *inDirection);
static void OBJiTurret_TargetingCallback_AimPoint(AI2tTargetingState *inTargetingState, M3tPoint3D *inPoint);
static void OBJiTurret_TargetingCallback_AimCharacter(AI2tTargetingState *inTargetingState, ONtCharacter *inTarget);
static void OBJiTurret_TargetingCallback_UpdateAiming(AI2tTargetingState *inTargetingState);

static void OBJiTurret_StartActiveSound(OBJtObject *inObject, OBJtOSD_Turret *ioTurretOSD);
static void OBJiTurret_StopActiveSound(OBJtObject *inObject, OBJtOSD_Turret *ioTurretOSD);

static void OBJiTurret_VisibleNodeList_Invalidate(OBJtOSD_Turret *turret_osd);
static void OBJiTurret_VisibleNodeList_Build(OBJtOSD_Turret *turret_osd);

AI2tTargetingCallbacks OBJgTurret_TargetingCallbacks = 
{
	OBJiTurret_TargetingCallback_Fire,
	NULL,	//OBJiTurret_TargetingCallback_SetAimSpeed,
	NULL,	//OBJiTurret_TargetingCallback_ForceAimWithWeapon,
	NULL,	//OBJiTurret_TargetingCallback_EndForceAim,
	OBJiTurret_TargetingCallback_GetPosition,
	OBJiTurret_TargetingCallback_AimVector,
	OBJiTurret_TargetingCallback_AimPoint,
	OBJiTurret_TargetingCallback_AimCharacter,
	OBJiTurret_TargetingCallback_UpdateAiming
};

// ======================================================================
// functions
// ======================================================================

static void OBJiTurret_StartActiveSound(OBJtObject *inObject, OBJtOSD_Turret *ioTurretOSD)
{
	if ((ioTurretOSD->flags & OBJcTurretFlag_PlayingSound) == 0) {

		if ((ioTurretOSD->turret_class != NULL) && (ioTurretOSD->turret_class->active_sound != NULL)) {
			ioTurretOSD->playing_sound = OSrAmbient_Start(ioTurretOSD->turret_class->active_sound, &inObject->position, NULL, NULL, NULL, NULL);
		} else {
			ioTurretOSD->playing_sound = SScInvalidID;
		}
		
		ioTurretOSD->flags |= OBJcTurretFlag_PlayingSound;
	}
}

static void OBJiTurret_StopActiveSound(OBJtObject *inObject, OBJtOSD_Turret *ioTurretOSD)
{
	if (ioTurretOSD->flags & OBJcTurretFlag_PlayingSound) {
		// stop any sounds that the turret is currently playing
		if (ioTurretOSD->playing_sound != SScInvalidID) {
			OSrAmbient_Stop(ioTurretOSD->playing_sound);
		}

		ioTurretOSD->flags &= ~OBJcTurretFlag_PlayingSound;
	}

	ioTurretOSD->playing_sound = SScInvalidID;
}

static void OBJrTurret_LookAtPoint(OBJtObject *inObject, M3tVector3D* inPoint)
{
	OBJtOSD_Turret			*turret_osd;
	M3tVector3D				vector;
	M3tVector3D				start_pt;
	float					horiz_facing;
	float					vert_facing;
	float					aim_horiz;
	M3tPoint3D				new_target;
	M3tMatrix4x3			inverse_matrix;
	M3tVector3D				neg_position;

		
 	UUmAssert( inObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) inObject->object_data;

	if( !turret_osd->turret_class )
		return;

	{
		new_target = *inPoint;

		// calculate the inverse of the rotation matrix
		MUrMatrix_Inverse(&turret_osd->rotation_matrix, &inverse_matrix);
		neg_position = inObject->position;
		MUmVector_Negate(neg_position);
		MUrMatrix_Translate(&inverse_matrix, &neg_position);

		// calculate a start point and an end poing in object space
		MUrMatrix_MultiplyPoint(inPoint, &inverse_matrix, &new_target);
	}

	start_pt.x	= turret_osd->turret_class->turret_position.x + turret_osd->turret_class->barrel_position.x;
	start_pt.y	= turret_osd->turret_class->turret_position.y + turret_osd->turret_class->barrel_position.y;
	start_pt.z	= turret_osd->turret_class->turret_position.z + turret_osd->turret_class->barrel_position.z;

	MUmVector_Subtract( vector, start_pt,	new_target );
	MUmVector_Subtract( vector, new_target, start_pt );

	horiz_facing	=  MUrATan2(vector.x, vector.z);
	aim_horiz		=  MUrSqrt(UUmSQR(vector.x) + UUmSQR(vector.z));
	vert_facing		=  MUrATan2(vector.y,aim_horiz);

	UUmTrig_Clip(vert_facing);
	UUmTrig_Clip(horiz_facing);

	turret_osd->desired_vert_angle	= vert_facing;
	turret_osd->desired_horiz_angle = horiz_facing;

	return;
}

// ======================================================================
// functions
// ======================================================================

// ----------------------------------------------------------------------

static void OBJiTurret_CreateBoundingSphere( OBJtObject *inObject, OBJtOSD_Turret *inOSD )
{
	M3tBoundingSphere		*sphere;
	M3tPoint3D				new_center;
	float					new_radius;
	
	new_center.x = 0.0f;
	new_center.y = 0.0f;
	new_center.z = 0.0f;
	
	if( !inOSD->turret_class )
		return;

	// calculate the center of the sphere
	sphere = &inOSD->turret_class->base_geometry->pointArray->boundingSphere;
	MUmVector_Increment(new_center, sphere->center);
	
	sphere = &inOSD->turret_class->turret_geometry->pointArray->boundingSphere;
	MUmVector_Increment(new_center, sphere->center);
	
	sphere = &inOSD->turret_class->barrel_geometry->pointArray->boundingSphere;
	MUmVector_Increment(new_center, sphere->center);
	
	new_center.x /= 3;
	new_center.y /= 3;
	new_center.z /= 3;
	
	// caculate the new radius
	new_radius = 0.0f;
	//for (i = 0; i < inOSD->furn_geom_array->num_furn_geoms; i++)
	{
//		M3tVector3D			vector;
//		float				temp_radius;
/*		
		sphere = &inOSD->furn_geom_array->furn_geom[i].geometry->pointArray->boundingSphere;
		MUmVector_Subtract(vector, new_center, sphere->center);
		temp_radius = MUrVector_Length(&vector) + sphere->radius;
		*/
//		new_radius = UUmMax(new_radius, temp_radius);
	}
	
	// set the bounding sphere
	inOSD->bounding_sphere.center = new_center;
	inOSD->bounding_sphere.radius = new_radius;
	
	//inOSD->bounding_sphere.center = sphere->center;
	inOSD->bounding_sphere.center = inOSD->turret_class->turret_position;
	inOSD->bounding_sphere.radius = (float) ( sphere->radius * 2.0 );
}

// ======================================================================
// ======================================================================

static void OBJiTurret_Delete( OBJtObject *inObject)
{
	OBJiTurret_Shutdown( inObject );
}

static void OBJiTurret_VisibleNodeList_Invalidate(OBJtOSD_Turret *turret_osd)
{
	turret_osd->visible_node_list[0] = 0;

	return;
}


static UUcInline M3tBoundingBox_MinMax OBJiTurret_Get_Visibility_BBox(OBJtOSD_Turret *turret_osd)
{
	M3tMatrix4x3 matrix = turret_osd->matrix;
	M3tBoundingBox_MinMax result;

	MUrMatrixStack_Matrix(&matrix, &turret_osd->turret_matrix);

	result.minPoint = MUrMatrix_GetTranslation(&matrix);
	result.maxPoint = MUrMatrix_GetTranslation(&matrix);

	result.minPoint.x -= 8.f;
	result.minPoint.y -= 8.f;
	result.minPoint.z -= 8.f;

	result.maxPoint.x += 8.f;
	result.maxPoint.y += 8.f;
	result.maxPoint.z += 8.f;

	return result;
}


static void OBJiTurret_VisibleNodeList_Build(OBJtOSD_Turret *turret_osd)
{
	if (0 == turret_osd->visible_node_list[0]) {
		M3tBoundingBox_MinMax bbox = OBJiTurret_Get_Visibility_BBox(turret_osd);

		AKrEnvironment_NodeList_Get(&bbox, OBJcTurretNodeCount, turret_osd->visible_node_list, 0);

#if TOOL_VERSION
		if (-1 == turret_osd->visible_node_list[0]) {
			COrConsole_Printf("turret failed to get a visible list (-1)");
		}
		else if (0 == turret_osd->visible_node_list[0]) {
			COrConsole_Printf("turret failed to get a visible list (0)");
		}
#endif
	}

	return;
}

static void OBJiTurret_BuildMatricies( OBJtObject *inObject )
{
	OBJtOSD_Turret			*turret_osd;

	UUmAssert( inObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*)inObject->object_data;

	if( !turret_osd->turret_class )
		return;

	// setup position & orientation matrix
	turret_osd->matrix = turret_osd->rotation_matrix;

	MUrMatrix_SetTranslation(&turret_osd->matrix, &inObject->position);

	// calc turret matrix
	MUrMatrix_Identity( &turret_osd->turret_matrix );

	MUrMatrix_RotateY( &turret_osd->turret_matrix, turret_osd->horiz_angle );

	MUrMatrix_SetTranslation(&turret_osd->turret_matrix, &turret_osd->turret_class->turret_position);

	// calc barrel matrix
	MUrMatrix_Identity( &turret_osd->barrel_matrix );

	MUrMatrix_RotateX( &turret_osd->barrel_matrix, M3c2Pi - turret_osd->vert_angle );

	MUrMatrix_SetTranslation(&turret_osd->barrel_matrix, &turret_osd->turret_class->barrel_position);

	// calculate projectile matrix
	MUrMatrix_Multiply( &turret_osd->matrix, &turret_osd->turret_matrix, &turret_osd->projectile_matrix );

	MUrMatrix_Multiply( &turret_osd->projectile_matrix, &turret_osd->barrel_matrix, &turret_osd->projectile_matrix );
}

static void OBJiTurret_Draw( OBJtObject *inObject, UUtUns32 inDrawFlags)
{
	OBJtOSD_Turret			*turret_osd;

	if (OBJgTurret_DrawTurrets == UUcFalse) { return; }
	// don't draw when the non-occluding geometry is hidden
	if (AKgDraw_Occl == UUcTrue) { return; }
		
	turret_osd = (OBJtOSD_Turret*)inObject->object_data;

	if( !turret_osd->turret_class )
		return;

	// build the matricies
	OBJiTurret_BuildMatricies( inObject );

	OBJiTurret_VisibleNodeList_Build(turret_osd);
	
	if (AKrEnvironment_NodeList_Visible(turret_osd->visible_node_list))
	{
		M3tMatrix4x3 draw_matrix;
		M3tBoundingBox_MinMax bbox;

		// apply position and orientation matrix
		draw_matrix = turret_osd->matrix;
		
#if TOOL_VERSION
		if(!(inObject->flags & OBJcObjectFlag_Gunk)) {
			M3rGeometry_MultiplyAndDraw(turret_osd->turret_class->base_geometry, &draw_matrix);
		}
#endif
		// apply turret matrix
		MUrMatrixStack_Matrix(&draw_matrix, &turret_osd->turret_matrix);
	
		// draw the turret geometry
		M3rGeometry_MultiplyAndDraw(turret_osd->turret_class->turret_geometry, &draw_matrix);
	
		// apply barrel matrix
		MUrMatrixStack_Matrix(&draw_matrix, &turret_osd->barrel_matrix);

		// determine if the barrel is visible
		M3rGeometry_GetBBox(turret_osd->turret_class->barrel_geometry, &draw_matrix, &bbox);

		M3rGeometry_MultiplyAndDraw(turret_osd->turret_class->barrel_geometry, &draw_matrix);

		turret_osd->flags |= OBJcTurretFlag_Visible;

	}
	else {
		turret_osd->flags &= ~OBJcTurretFlag_Visible;
	}

#if TOOL_VERSION
	// draw the bounding box if this is the selected object
	if (inDrawFlags & OBJcDrawFlag_Selected)
	{
		M3tBoundingBox bBox;

		M3rMatrixStack_Push();
		{

			{
				M3tBoundingBox_MinMax minmax_bbox = OBJiTurret_Get_Visibility_BBox(turret_osd);

				M3rMinMaxBBox_To_BBox(&minmax_bbox, &bBox);
				M3rBBox_Draw_Line(&bBox, IMcShade_Green);
			}

			// apply position and orientation matrix
			M3rMatrixStack_Multiply(&turret_osd->matrix);

			// base box and object rotation rings
			M3rMinMaxBBox_To_BBox( &turret_osd->turret_class->base_geometry->pointArray->minmax_boundingBox, &bBox);		
			M3rBBox_Draw_Line(&bBox, IMcShade_White);			
			OBJrObjectUtil_DrawRotationRings(inObject, &turret_osd->bounding_sphere, inDrawFlags);

			// apply turret matrix
			M3rMatrixStack_Multiply(&turret_osd->turret_matrix);		
			
			// turret box
			M3rMinMaxBBox_To_BBox( &turret_osd->turret_class->turret_geometry->pointArray->minmax_boundingBox, &bBox);		
			M3rBBox_Draw_Line(&bBox, IMcShade_White);

			// apply barrel matrix
			M3rMatrixStack_Multiply(&turret_osd->barrel_matrix);		

			// barrel box
			M3rMinMaxBBox_To_BBox( &turret_osd->turret_class->barrel_geometry->pointArray->minmax_boundingBox, &bBox);		
			M3rBBox_Draw_Line(&bBox, IMcShade_White);
		}
		M3rMatrixStack_Pop();
	}
	
	if( OBJgTurret_DrawTurretDebugInfo )
		OBJrTurretDisplayDebuggingInfo(inObject);
#endif
}

// ----------------------------------------------------------------------
static UUtError OBJiTurret_Enumerate( OBJtObject *inObject, OBJtEnumCallback_ObjectName inEnumCallback, UUtUns32 inUserData )
{
	return OBJrObjectUtil_EnumerateTemplate( "", OBJcTemplate_TurretClass, inEnumCallback, inUserData);
}

// ----------------------------------------------------------------------
static void OBJiTurret_GetBoundingSphere( const OBJtObject *inObject, M3tBoundingSphere *outBoundingSphere )
{
	OBJtOSD_Turret		*Turret_osd;
	
	Turret_osd = (OBJtOSD_Turret*)inObject->object_data;

	*outBoundingSphere = Turret_osd->bounding_sphere;
}

// ----------------------------------------------------------------------
static void OBJiTurret_OSDGetName( const OBJtOSD_All *inOSD,	char *outName, UUtUns32 inNameLength )
{
	const OBJtOSD_Turret			*turret_osd = &inOSD->osd.turret_osd;
	
	sprintf(outName, "%s_%d", turret_osd->turret_class_name, turret_osd->id);
}

// ----------------------------------------------------------------------
static void OBJiTurret_GetOSD( const OBJtObject *inObject, OBJtOSD_All *outOSD )
{
	OBJtOSD_Turret			*Turret_osd;

	Turret_osd = (OBJtOSD_Turret*)inObject->object_data;
	
	outOSD->osd.turret_osd = *Turret_osd;
}

// ----------------------------------------------------------------------
static UUtBool OBJiTurret_IntersectsLine( const OBJtObject *inObject, const M3tPoint3D *inStartPoint, const M3tPoint3D *inEndPoint)
{
	M3tBoundingSphere		sphere;
	UUtBool					result;
	OBJtOSD_Turret			*turret_osd;

	UUrMemory_Clear(&sphere, sizeof(M3tBoundingSphere));

	turret_osd = (OBJtOSD_Turret*)inObject->object_data;

	if( !turret_osd->turret_class )
		return UUcFalse;
	
	// get the bounding sphere
	OBJiTurret_GetBoundingSphere(inObject, &sphere);
	
	sphere.center.x += inObject->position.x;
	sphere.center.y += inObject->position.y;
	sphere.center.z += inObject->position.z;
	
	// do the fast test to see if the line is colliding with the bounding sphere
	result = CLrSphere_Line(inStartPoint, inEndPoint, &sphere);
	//if (result == UUcTrue)
	{
		M3tPoint3D				start_point;
		M3tPoint3D				end_point;
		M3tMatrix4x3			inverse_matrix;
		M3tVector3D				neg_position;
		
		result = UUcFalse;
		
		// because the line collided with the bounding sphere, test to see if the line
		// collides with the bounding box of the geometries
		
		// move 
		MUrMatrix_Inverse(&turret_osd->rotation_matrix, &inverse_matrix);
		neg_position = inObject->position;
		MUmVector_Negate(neg_position);
		MUrMatrix_Translate(&inverse_matrix, &neg_position);
		// calculate a start point and an end poing in object space
		MUrMatrix_MultiplyPoint(inStartPoint, &inverse_matrix, &start_point);
		MUrMatrix_MultiplyPoint(inEndPoint, &inverse_matrix, &end_point);


		// base
		{ 
			result = CLrBox_Line( &turret_osd->turret_class->base_geometry->pointArray->minmax_boundingBox, &start_point, &end_point );
			if (result)		return UUcTrue;
		}
		start_point.x -= turret_osd->turret_class->turret_position.x;
		start_point.y -= turret_osd->turret_class->turret_position.y;
		start_point.z -= turret_osd->turret_class->turret_position.z;

		end_point.x -= turret_osd->turret_class->turret_position.x;
		end_point.y -= turret_osd->turret_class->turret_position.y;
		end_point.z -= turret_osd->turret_class->turret_position.z;
		// turret
		{
			result = CLrBox_Line( &turret_osd->turret_class->turret_geometry->pointArray->minmax_boundingBox, &start_point, &end_point );
			if (result)		return UUcTrue;
		}
		start_point.x -= turret_osd->turret_class->barrel_position.x;
		start_point.y -= turret_osd->turret_class->barrel_position.y;
		start_point.z -= turret_osd->turret_class->barrel_position.z;

		end_point.x -= turret_osd->turret_class->barrel_position.x;
		end_point.y -= turret_osd->turret_class->barrel_position.y;
		end_point.z -= turret_osd->turret_class->barrel_position.z;
		// barrel
		{
			result = CLrBox_Line( &turret_osd->turret_class->barrel_geometry->pointArray->minmax_boundingBox, &start_point, &end_point );
			if (result)		return UUcTrue;
		}		
		return result;
	}
	
	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtError OBJiTurret_SetDefaults(OBJtOSD_All *outOSD)
{
	UUtError				error;
	void				*instances[OBJcMaxInstances];
	UUtUns32			num_instances;
	char				*instance_name;

	// clear the osd
	UUrMemory_Clear(&outOSD->osd.turret_osd, sizeof(OBJtOSD_Turret));
		
	// setup default properties
	outOSD->osd.turret_osd.id				= ONrMechanics_GetNextID( OBJcType_Turret );
	outOSD->osd.turret_osd.target_teams		= 1;									// team 0

	// internal props
	outOSD->osd.turret_osd.fire_weapon		= UUcFalse;
	outOSD->osd.turret_osd.flags			= OBJcTurretFlag_None;
	outOSD->osd.turret_osd.state			= OBJcTurretState_Inactive;
	outOSD->osd.turret_osd.playing_sound	= SScInvalidID;
	
	// get a list of instances of the class
	error = TMrInstance_GetDataPtr_List( OBJcTemplate_TurretClass, OBJcMaxInstances, &num_instances, instances);
	UUmError_ReturnOnError(error);

	// copy the name of the first turret instance into the osd_all.		
	instance_name	= TMrInstance_GetInstanceName(instances[0]);
	instance_name	= (instance_name != NULL) ? instance_name : "";
		
	UUrString_Copy( outOSD->osd.turret_osd.turret_class_name, (instance_name ), OBJcMaxNameLength);


	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError OBJiTurret_New( OBJtObject *inObject, const OBJtOSD_All *inOSD)
{

	OBJtOSD_All				osd_all;
	UUtError				error;
	
	if (inOSD == NULL)
	{
		error = OBJiTurret_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);

		inOSD = &osd_all;
	}
	else
	{
		osd_all = *inOSD;
	}

	// add to global turret list
//	UUmAssert( ONgGameState && OBJgTurret_Count < OBJcMaxTurrets );
//	OBJgTurret_List[OBJgTurret_Count++] = (OBJtObject*) inObject;

	// set the object specific data and position
	error = OBJiTurret_SetOSD(inObject, &osd_all);
	UUmError_ReturnOnError(error);
	
	OBJiTurret_UpdatePosition(inObject);

	UUrMemory_Block_VerifyList();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32 OBJiTurret_Read( OBJtObject *inObject, UUtUns16 inVersion, UUtBool	inSwapIt, UUtUns8	*inBuffer)
{
	OBJtOSD_All				osd_all;
	OBJtOSD_Turret*			osd;
	UUtUns32				num_bytes;

	UUtUns32 obsolete_timeout;
	float obsolete_start_horiz_angle;
	float obsolete_start_vert_angle;
	float obsolete_max_horiz_angle;
	float obsolete_min_horiz_angle;
	float obsolete_max_horiz_speed;
	float obsolete_max_vert_angle;
	float obsolete_min_vert_angle;
	float obsolete_max_vert_speed;

	num_bytes = 0;

	osd = &osd_all.osd.turret_osd;

	OBJmGetStringFromBuffer(inBuffer, osd->turret_class_name,		OBJcMaxNameLength, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, osd->id,						UUtUns16, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, osd->flags,					UUtUns16, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, obsolete_timeout,					UUtUns32, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, obsolete_start_horiz_angle,		float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, obsolete_start_vert_angle,		float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, obsolete_max_horiz_angle,			float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, obsolete_min_horiz_angle,			float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, obsolete_max_horiz_speed,			float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, obsolete_max_vert_angle,			float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, obsolete_min_vert_angle,			float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, obsolete_max_vert_speed,			float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, osd->target_teams,			UUtUns32, inSwapIt);

	osd->flags &= OBJcTurretFlag_Persist;

	num_bytes = OBJcMaxNameLength + 
		sizeof(UUtUns16) + 
		sizeof(UUtUns16) + 
		sizeof(UUtUns32) + 
		( sizeof( float ) * 8 ) +
		sizeof(UUtUns32);

	OBJiTurret_VisibleNodeList_Invalidate(osd);

	// bring the object up to date
	OBJiTurret_SetOSD(inObject, &osd_all);
	
	return num_bytes;
}
// ----------------------------------------------------------------------
static UUtError OBJiTurret_Write( OBJtObject *inObject, UUtUns8 *ioBuffer, UUtUns32 *ioBufferSize)
{
	
	OBJtOSD_Turret			*turret_osd;
	UUtUns32				bytes_available;

	UUtUns32 obsolete_timeout = 0;
	float obsolete_start_horiz_angle = 0;
	float obsolete_start_vert_angle = 0;
	float obsolete_max_horiz_angle = 0;
	float obsolete_min_horiz_angle = 0;
	float obsolete_max_horiz_speed = 0;
	float obsolete_max_vert_angle = 0;
	float obsolete_min_vert_angle = 0;
	float obsolete_max_vert_speed = 0;


	// get a pointer to the object osd
	turret_osd = (OBJtOSD_Turret*)inObject->object_data;
	
	// set the number of bytes available
	bytes_available = *ioBufferSize;

	OBJmWriteStringToBuffer(ioBuffer, turret_osd->turret_class_name,		OBJcMaxNameLength, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, turret_osd->id,						UUtUns16, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, turret_osd->flags & OBJcTurretFlag_Persist, UUtUns16, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, obsolete_timeout,					UUtUns32, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, obsolete_start_horiz_angle,		float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, obsolete_start_vert_angle,			float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, obsolete_max_horiz_angle,			float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, obsolete_min_horiz_angle,			float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, obsolete_max_horiz_speed,			float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, obsolete_max_vert_angle,			float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, obsolete_min_vert_angle,			float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, obsolete_max_vert_speed,			float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, turret_osd->target_teams,				UUtUns32, bytes_available, OBJcWrite_Little);

	// set ioBufferSize to the number of bytes written to the buffer
	*ioBufferSize = *ioBufferSize - bytes_available;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32 OBJiTurret_GetOSDWriteSize( const OBJtObject *inObject )
{
	UUtUns32				size;
	
	size = OBJcMaxNameLength + 
		sizeof(UUtUns16) + 
		sizeof(UUtUns16) + 
		sizeof(UUtUns32) + 
		( sizeof( float ) * 8 ) +
		sizeof(UUtUns32);
	
	return size;
}

// converts particle class names into pointers, and sets a flag to say so.
// - ---------------------------------------------------------------------
static void OBJiTurret_FindParticleClasses(OBJtTurretClass *inTurret)
{
	UUtUns16 itr;
	OBJtTurretParticleAttachment *attachment;

	for (itr = 0, attachment = inTurret->attachment; itr < inTurret->attachment_count; itr++, attachment++) 
	{
		attachment->particle_class = P3rGetParticleClass(attachment->particle_class_name);
	}
}
// - ---------------------------------------------------------------------
// - ---------------------------------------------------------------------
static void OBJiTurret_Shutdown( OBJtObject *inObject )
{
	OBJtOSD_Turret			*turret_osd;
	// get a pointer to the object osd
	turret_osd = (OBJtOSD_Turret*)inObject->object_data;

	// deactivate if active
	if( turret_osd->flags & OBJcTurretFlag_Initialized )
	{
		OBJiTurret_StopActiveSound(inObject, turret_osd);

		if( turret_osd->state == OBJcTurretState_Active )
			OBJrTurret_Deactivate( inObject );
		OBJrTurret_DestroyParticles( inObject );
	}
}

static UUtError OBJiTurret_SetOSD( OBJtObject *inObject, const OBJtOSD_All *inOSD )
{
	UUtError				error;
	OBJtTurretClass			*turret_class;
	OBJtOSD_Turret			*turret_osd;
	
	UUmAssert(inOSD);
	
	OBJiTurret_Shutdown( inObject );

	// get a pointer to the object osd
	turret_osd = (OBJtOSD_Turret*)inObject->object_data;

	OBJiTurret_StopActiveSound(inObject, turret_osd);

	// copy the persistant data
	UUrString_Copy(turret_osd->turret_class_name, inOSD->osd.turret_osd.turret_class_name, OBJcMaxNameLength);
	turret_osd->id					= inOSD->osd.turret_osd.id;
	turret_osd->flags				= (turret_osd->flags & ~OBJcTurretFlag_Persist) | (inOSD->osd.turret_osd.flags & OBJcTurretFlag_Persist);
	turret_osd->target_teams 		= inOSD->osd.turret_osd.target_teams;
	
	error = TMrInstance_GetDataPtr( OBJcTemplate_TurretClass, turret_osd->turret_class_name, &turret_class );
	if( error != UUcError_None )
	{
		UUrDebuggerMessage("failed to find turret class %s\n", turret_osd->turret_class_name);

		error = TMrInstance_GetDataPtr_ByNumber(OBJcTemplate_TurretClass, 0, &turret_class);

		UUmError_ReturnOnErrorMsg(error, "failed to find any turret class");

		inObject->flags				|= OBJcObjectFlag_Temporary;
		turret_osd->turret_class	= NULL;
	}


	turret_osd->turret_class		= turret_class;		

	// setup internals
	MUrMatrix_Identity(&turret_osd->matrix);
	MUrMatrix_Identity(&turret_osd->turret_matrix);
	MUrMatrix_Identity(&turret_osd->barrel_matrix);
	turret_osd->notvisible_time = 0;
	
	if( ONgLevel )
	{
		OBJrTurret_SetupParticles( inObject );
	}

	// build the position matrix
	OBJiTurret_UpdatePosition(inObject);

	// create the bounding sphere
	OBJiTurret_CreateBoundingSphere(inObject, turret_osd);

	// set active state	
	if( inOSD->osd.turret_osd.state == OBJcTurretState_Active )
		OBJrTurret_Activate( inObject );

	UUrMemory_Block_VerifyList();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void OBJiTurret_UpdatePosition(OBJtObject *inObject)
{
	
	OBJtOSD_Turret			*Turret_osd;
	float					rot_x;
	float					rot_y;
	float					rot_z;
	MUtEuler				euler;
	
	// get a pointer to the object osd
	Turret_osd = (OBJtOSD_Turret*)inObject->object_data;
	
	// convert the rotation to radians
	rot_x = inObject->rotation.x * M3cDegToRad;
	rot_y = inObject->rotation.y * M3cDegToRad;
	rot_z = inObject->rotation.z * M3cDegToRad;
/*
	rot_x += 270 * M3cDegToRad;

	if( rot_x > 360 )
		rot_x -= 360;
*/
	 // update the rotation matrix
	euler.order = MUcEulerOrderXYZs;
	euler.x = rot_x;
	euler.y = rot_y;
	euler.z = rot_z;

	MUrEulerToMatrix(&euler, &Turret_osd->rotation_matrix);

	// mark the visible list as requireing
	OBJiTurret_VisibleNodeList_Invalidate(Turret_osd);

	return;
}

// ======================================================================

static UUtBool OBJiTurret_GetVisible( void)
{
	return OBJgTurret_DrawTurrets;
}

// ----------------------------------------------------------------------
static UUtBool OBJiTurret_Search( const OBJtObject *inObject, const UUtUns32 inSearchType, const OBJtOSD_All *inSearchParams)
{
	
	OBJtOSD_Turret			*Turret_osd;
	UUtBool					found;
	
	// get a pointer to the object osd
	Turret_osd = (OBJtOSD_Turret*)inObject->object_data;

	found = UUcFalse;
/*	
	// perform the check
	switch (inSearchType)
	{
		case OBJcSearch_TurretID:
			if (Turret_osd->id == inSearchParams->Turret_osd.id)
			{
				found = UUcTrue;
			}
		break;
	}
*/
	return found;
}

// ----------------------------------------------------------------------
static void OBJiTurret_SetVisible( UUtBool inIsVisible )
{
	OBJgTurret_DrawTurrets = inIsVisible;
}

// ======================================================================
// projectiles
// ======================================================================

static void OBJiTurret_CreateParticles( OBJtOSD_Turret *inTurret_osd )
{
	UUtUns32 itr;
	OBJtTurretParticleAttachment *attachment;
	P3tParticle *particle;
	UUtUns32 creation_time, *p_texture, *p_lensframes;
	float float_time;
	M3tMatrix3x3 *p_orientation;
	M3tMatrix4x3 **p_dynamicmatrix;
	M3tPoint3D *p_position;
	M3tVector3D *p_velocity, *p_offset;
	P3tPrevPosition *p_prev_position;
	P3tOwner *p_owner;
	OBJtTurretClass*	turret;
	AKtOctNodeIndexCache *p_envcache;
	UUtBool is_dead;

	creation_time = ONrGameState_GetGameTime();
	float_time = ((float) creation_time) / UUcFramesPerSecond;

	turret = inTurret_osd->turret_class;

	if( !turret )
		return;

	// loop through each attachment on the turret template and create a particle for the turret osd
	for (itr = 0, attachment = turret->attachment; itr < turret->attachment_count; itr++, attachment++) 
	{
		if (!attachment->particle_class) 
		{
			inTurret_osd->particle[itr] = NULL;
			continue;
		}
		
		particle = P3rCreateParticle(attachment->particle_class, creation_time);
		if (particle == NULL) 
		{
			inTurret_osd->particle[itr] = NULL;
			continue;
		}

		// position is the translation part of attachment->matrix
		p_position = P3rGetPositionPtr(attachment->particle_class, particle, &is_dead);
		if (is_dead)
			continue;

		MUrMatrix_GetCol(&attachment->matrix, 3, p_position);

		// orientation is the 3x3 matrix part of attachment->matrix
		p_orientation = P3rGetOrientationPtr(attachment->particle_class, particle);
		if (p_orientation != NULL) 
		{
			UUrMemory_MoveFast(&attachment->matrix, p_orientation, sizeof(M3tMatrix3x3));
		}

		// velocity and offset are zero if they exist
		p_velocity = P3rGetVelocityPtr(attachment->particle_class, particle);
		if (p_velocity != NULL) 
		{
			MUmVector_Set(*p_velocity, 0, 0, 0);
		}
		p_offset = P3rGetOffsetPosPtr(attachment->particle_class, particle);
		if (p_offset != NULL) 
		{
			MUmVector_Set(*p_offset, 0, 0, 0);
		}

		// dynamic matrix ensures that the particles stay attached to the weapon
		p_dynamicmatrix = P3rGetDynamicMatrixPtr(attachment->particle_class, particle);
		if (p_dynamicmatrix != NULL) 
		{
			*p_dynamicmatrix = &inTurret_osd->projectile_matrix;
			particle->header.flags |= P3cParticleFlag_AlwaysUpdatePosition;
		} 
		else 
		{
			COrConsole_Printf("### particle class '%s' attached to turret but no dynamic matrix", attachment->particle_class_name);
		}

		// the particle's owner is our owner pointer. note that since our owner may change
		// the particles need to refer to our pointer rather than taking a copy of it
		p_owner = P3rGetOwnerPtr(attachment->particle_class, particle);
		if (p_owner != NULL) 
		{
			*p_owner = WPrOwner_MakeFromTurret(inTurret_osd);
		}

		// randomise texture start index
		p_texture = P3rGetTextureIndexPtr(attachment->particle_class, particle);
		if (p_texture != NULL) 
		{
			*p_texture = (UUtUns32) UUrLocalRandom();
		}

		// set texture time index to be now
		p_texture = P3rGetTextureTimePtr(attachment->particle_class, particle);
		if (p_texture != NULL) 
		{
			*p_texture = creation_time;
		}

		// no previous position
		p_prev_position = P3rGetPrevPositionPtr(attachment->particle_class, particle);
		if (p_prev_position != NULL) {
			p_prev_position->time = 0;
		}

		// lensflares are not initially visible
		p_lensframes = P3rGetLensFramesPtr(attachment->particle_class, particle);
		if (p_lensframes != NULL) {
			*p_lensframes = 0;
		}

		// set up the environment cache
		p_envcache = P3rGetEnvironmentCache(attachment->particle_class, particle);
		if (p_envcache != NULL) {
			AKrCollision_InitializeSpatialCache(p_envcache);
		}

		// we can't have particles dying on us, because we need to keep referring to them.
		// so make sure they don't have a lifetime.
		particle->header.lifetime = 0;

		// turrets start with background effects off
		P3rSendEvent(attachment->particle_class, particle, P3cEvent_Create, float_time);
		P3rSendEvent(attachment->particle_class, particle, P3cEvent_BackgroundStop, float_time);

		inTurret_osd->particle[itr]		= particle;
		inTurret_osd->particle_ref[itr] = particle->header.self_ref;
	}

	inTurret_osd->flags |= OBJcTurretFlag_Initialized;
}

// ----------------------------------------------------------------------
// send an event to all particles attached to a turret
static void OBJiTurret_SendParticleEvent( OBJtObject *inObject, UUtUns16 inEvent, UUtUns16 inShooter)
{
	UUtUns16 itr;
	OBJtTurretParticleAttachment *attachment;
	P3tParticle *particle;
	M3tVector3D *p_velocity;
	OBJtOSD_Turret			*turret_osd;

	turret_osd = (OBJtOSD_Turret*) inObject->object_data;

	if( !turret_osd->turret_class )
		return;

	for (itr = 0, attachment = turret_osd->turret_class->attachment; itr < turret_osd->turret_class->attachment_count; itr++, attachment++) 
	{
		// only send the event to attachments with the correct shooter index - this
		// check is bypassed if either index is -1, which matches everything
		if ((inShooter != (UUtUns16) -1) && (attachment->shooter_index != (UUtUns16) -1) && (attachment->shooter_index != inShooter))
			continue;

		particle = turret_osd->particle[itr];

		if ((particle == NULL) || (particle->header.self_ref != turret_osd->particle_ref[itr])) 
		{
			// this particle has died!
			turret_osd->particle[itr]		= NULL;
			turret_osd->particle_ref[itr]	= P3cParticleReference_Null;
			continue;
		}

		if ((particle != NULL) && (attachment->particle_class))
		{
			// we must set up this particle's velocity as equal to the owner's so that emitters which are tagged as "emit parent's velocity" have something to use
			p_velocity = P3rGetVelocityPtr(attachment->particle_class, particle);
			if (p_velocity != NULL) 
			{
				p_velocity->x = 0;
				p_velocity->y = 0;
				p_velocity->z = 0;
				//MUmVector_Copy(*p_velocity, inWeapon->owner->movementThisFrame);
				//MUmVector_Scale(*p_velocity, UUcFramesPerSecond);
			}


			P3rSendEvent(attachment->particle_class, particle, inEvent, ((float) ONrGameState_GetGameTime()) / UUcFramesPerSecond);
		}

	}
	// send statup event
	// FIXME: must also call AIrBulletNotify(ioWeapon->owner,bullet->physics) somewhere?
}


UUtBool OBJiTurret_FireProjectile( OBJtObject *inObject, UUtUns16 shooter_index )
{
	OBJtOSD_Turret			*turret_osd;
	UUtUns16				attachment_index;
	const M3tPoint3D		cPosYPoint = { 0.f, 1.f, 0.f };
	OBJtTurretParticleAttachment  *shooter;

	turret_osd = (OBJtOSD_Turret*)inObject->object_data;

	if( !turret_osd->turret_class )
		return UUcFalse;
	
	// only active turrets should be firing!
	UUmAssert(turret_osd->state == OBJcTurretState_Active );
	UUmAssert((shooter_index >= 0) && (shooter_index < turret_osd->turret_class->shooter_count));
	
	// wait for chamber delay
	if (turret_osd->chamber_time)
		return UUcFalse;

	// find the shooter's attachment
	for (attachment_index = 0, shooter = turret_osd->turret_class->attachment; attachment_index < turret_osd->turret_class->attachment_count; attachment_index++, shooter++) 
	{
		if( shooter->shooter_index == shooter_index )
		break;
	}

	// couldn't find the attachment that corresponds to this shooter
	if (attachment_index >= turret_osd->turret_class->attachment_count)
		return UUcFalse;

	// we will now fire.
	if((turret_osd->flags & OBJcTurretFlag_IsFiring) == 0) 
	{
		turret_osd->flags |= OBJcTurretFlag_IsFiring;
		OBJiTurret_SendParticleEvent(inObject, P3cEvent_Start, shooter_index);
	}

	// pulse all particles attached to this shooter
	OBJiTurret_SendParticleEvent( inObject, P3cEvent_Pulse, shooter_index);

	// set shot timer
	turret_osd->chamber_time = shooter->chamber_time;

	return UUcTrue;
}

void OBJrTurret_RecreateParticles(void)
{
	UUtUns32				object_count;
	OBJtObject				*object;
	OBJtObject				**object_list;
	UUtUns32				i;

	OBJrObjectType_GetObjectList( OBJcType_Turret, &object_list, &object_count );

	for(i = 0; i < object_count; i++) 
	{
		object = object_list[i];
		UUmAssert( object );
		OBJiTurret_CreateParticles( (OBJtOSD_Turret*) object->object_data );
	}
}

UUtError OBJrTurret_DestroyParticles( OBJtObject *inObject )
{
	UUtUns32						itr;
	P3tParticle						*particle;
	OBJtOSD_Turret					*turret_osd;
	OBJtTurretClass*				turret;

	UUmAssert( inObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) inObject->object_data;

	OBJiTurret_SendParticleEvent( inObject, P3cEvent_Stop, -1 );
	OBJiTurret_SendParticleEvent( inObject, P3cEvent_BackgroundStop, -1 );

	if( !turret_osd->turret_class )
		return UUcError_None;

	turret = turret_osd->turret_class;

	if( !turret )
		return UUcError_None;
	
	for (itr = 0; itr < turret->attachment_count; itr++ ) 
	{
		if( !turret_osd->particle[itr] )
			continue;
		particle = turret_osd->particle[itr];
		P3rKillParticle(&P3gClassTable[particle->header.class_index], particle);

	}

	return UUcError_None;
}

UUtError OBJrTurret_SetupParticles( OBJtObject *inObject )
{
	OBJtOSD_Turret *turret_osd;

	UUmAssert( inObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) inObject->object_data;

	if( !turret_osd->turret_class )
		return UUcError_None;

	OBJiTurret_FindParticleClasses( turret_osd->turret_class );

	OBJiTurret_CreateParticles( turret_osd );
		
	// FIXME: make sure weapons cant fire for a second..... (particles crash otherwise!)
	turret_osd->chamber_time	= 60;

	return UUcError_None;
}

// ======================================================================
// state management
// ======================================================================

void OBJrTurret_Activate( OBJtObject *inObject )
{
	OBJtOSD_Turret			*turret_osd;

	turret_osd = (OBJtOSD_Turret*)inObject->object_data;

//	UUmAssert( turret_osd->state == OBJcTurretState_Inactive );

	if( turret_osd->state != OBJcTurretState_Inactive )
		return;

	// start the timer
	turret_osd->active_time = 0;

	turret_osd->state = OBJcTurretState_Active;

	// start playing our seeking sound
	OBJiTurret_StartActiveSound(inObject, turret_osd);

	OBJiTurret_SendParticleEvent( inObject, P3cEvent_BackgroundStart, -1 );

	// start the combat AI
	OBJiTurretCombat_Enter((OBJtObject*) inObject);
}

void OBJrTurret_Deactivate( OBJtObject *inObject )
{
	OBJtOSD_Turret			*turret_osd;

	turret_osd = (OBJtOSD_Turret*)inObject->object_data;

	if( turret_osd->state != OBJcTurretState_Active )
		return;

	turret_osd->desired_horiz_angle = 0.f;
	turret_osd->desired_vert_angle	= 0.f;

	// end the combat AI
	OBJiTurretCombat_Exit((OBJtObject*) inObject);

	if( turret_osd->flags & OBJcTurretFlag_IsFiring ) 
	{
		turret_osd->flags |= ~OBJcTurretFlag_IsFiring;
		OBJiTurret_SendParticleEvent( inObject, P3cEvent_Stop, -1 );
	}

	OBJiTurret_SendParticleEvent( inObject, P3cEvent_BackgroundStop, -1 );

	// stop playing our seeking sound
	OBJiTurret_StopActiveSound(inObject, turret_osd);

	turret_osd->fire_weapon = UUcFalse;

	turret_osd->state = OBJcTurretState_Inactive;
}

static UUtBool OBJrTurret_Activate_ID_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Turret *turret_osd;

	UUmAssert( inObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) inObject->object_data;
	
	if( inUserData == OBJcTurretSelectAll || turret_osd->id == inUserData )
	{
		OBJrTurret_Activate( (OBJtObject*) inObject );
	}

	return UUcTrue;
}

UUtError OBJrTurret_Activate_ID( UUtUns16 inID )
{
	if( inID == (UUtUns16) -1 )
		inID = OBJcTurretSelectAll;
	OBJrObjectType_EnumerateObjects( OBJcType_Turret, OBJrTurret_Activate_ID_Enum, (UUtUns32) inID ); 
	return UUcError_None;
}

static UUtBool OBJrTurret_Deactivate_ID_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Turret *Turret_osd;

	UUmAssert( inObject->object_type == OBJcType_Turret );

	Turret_osd = (OBJtOSD_Turret*) inObject->object_data;

	if( Turret_osd->id == inUserData )
	{
		OBJrTurret_Deactivate( (OBJtObject*) inObject );
	}

	return UUcTrue;
}

UUtError OBJrTurret_Deactivate_ID( UUtUns16 inID )
{
	if( inID == (UUtUns16) -1 )
		inID = OBJcTurretSelectAll;
	OBJrObjectType_EnumerateObjects( OBJcType_Turret, OBJrTurret_Deactivate_ID_Enum, (UUtUns32) inID ); 
	return UUcError_None;
}

static UUtBool OBJrTurret_Reset_ID_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Turret *turret_osd;

	UUmAssert( inObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) inObject->object_data;

	if( inUserData == OBJcTurretSelectAll || turret_osd->id == inUserData )
	{
		OBJrTurret_Reset( (OBJtObject*) inObject );
	}

	return UUcTrue;
}

UUtError OBJrTurret_Reset_ID( UUtUns16 inID )
{
	if( inID == (UUtUns16) -1 )
		inID = OBJcTurretSelectAll;
	OBJrObjectType_EnumerateObjects( OBJcType_Turret, OBJrTurret_Reset_ID_Enum, (UUtUns32) inID ); 
	return UUcError_None;
}

// ======================================================================
// Combat AI
// ======================================================================
// set up a combat state - called when the character goes into "combat mode".
void OBJiTurretCombat_Enter(OBJtObject *ioObject)
{
//	AI2tCombatState			*combat_state;
	AI2tTargetingOwner		targeting_owner;
	OBJtOSD_Turret			*turret_osd;

	UUmAssert( ioObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) ioObject->object_data;

	if( !turret_osd->turret_class )
		return;

	// set up our targeting
	targeting_owner.type		= AI2cTargetingOwnerType_Turret;
	targeting_owner.owner.turret	= ioObject;
	
	// init the targeting
	AI2rTargeting_Initialize(targeting_owner, &turret_osd->targeting, &OBJgTurret_TargetingCallbacks, &turret_osd->turret_class->ai_params, &turret_osd->projectile_matrix, &turret_osd->turret_class->targeting_params, &turret_osd->turret_class->shooting_skill);
}

// ------------------------------------------------------------------------------------
// -- line-of-sight

static UUtBool OBJrTurret_PerformLOS(OBJtObject *inObject)
{
	OBJtOSD_Turret			*turret_osd;
	

	UUmAssert( inObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) inObject->object_data;

	if( !turret_osd->turret_class )
		return UUcFalse;

/*	if(turret_osd->targeting.miss_enable) 
	{
		// we don't care about LOS for the time being
		return UUcFalse;
	}*/

	if( turret_osd->targeting.target )
	{
		M3tVector3D	los_from;
		M3tPoint3D	los_to;
		float free_distance = 15.f;

		M3tPoint3D	vector;
		M3tVector3D free_vector;

		los_from = inObject->position;
		los_to = turret_osd->targeting.target->location;
		los_to.y += turret_osd->targeting.target->heightThisFrame;

		MUmVector_Subtract(free_vector, los_to, los_from);

		if (MUmVector_GetLengthSquared(free_vector) < UUmSQR(free_distance)) {
			return UUcTrue;
		}
		else {
			MUrVector_SetLength(&free_vector, free_distance);
			MUmVector_Increment(los_from, free_vector);

			MUmVector_Subtract(vector, los_to, los_from);

			if (!AKrCollision_Point(ONrGameState_GetEnvironment(), &los_from, &vector, AKcGQ_Flag_LOS_CanShoot_Skip_AI, 0)) {
				return UUcTrue;
			}
		}
	}
	return UUcFalse;
}

// ------------------------------------------------------------------------------------
static ONtCharacter* OBJiTurretCombat_FindClosestTarget( OBJtObject *ioObject )
{
	OBJtOSD_Turret			*turret_osd;
	UUtUns32				team_mask;
	ONtCharacter			**character_list;
	ONtCharacter			*character;
	ONtCharacter			*closest_character;
	UUtUns32				character_count;
	UUtUns32				closest_distance = UUcMaxUns32;
	UUtUns32				i;
	UUtUns32				distance;

	UUmAssert( ioObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) ioObject->object_data;

	character_list = ONrGameState_LivingCharacterList_Get();

	character_count = ONrGameState_LivingCharacterList_Count();

	closest_character = NULL;

	for( i = 0; i < character_count; i++ )
	{
		character	= character_list[i];
		
		team_mask	= 1 << character->teamNumber;

		if(!(turret_osd->target_teams & team_mask))
			continue;

		distance	= (UUtUns32) MUmVector_GetDistanceSquared(ioObject->position, character->location);
		if( distance < closest_distance )
		{
			closest_distance = distance;
			closest_character = character;
		}
	}
	return closest_character;
}

static UUtBool OBJiTurretCombat_CheckTarget(OBJtObject *ioObject)
{
	OBJtOSD_Turret			*turret_osd;
	UUtBool					target_currently_dead;

	UUmAssert( ioObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) ioObject->object_data;

	if( turret_osd->targeting.target )
	{
		target_currently_dead = (UUtBool) (turret_osd->targeting.target->flags & ONcCharacterFlag_Dead);
	}
	
	turret_osd->check_target_time = (UUtUns32) ( UUcFramesPerSecond / 4.0 );

	turret_osd->targeting.target = OBJiTurretCombat_FindClosestTarget( ioObject );
	
	return ( turret_osd->targeting.target != NULL );
}

// ------------------------------------------------------------------------------------
// start background effects on the turret
static void OBJiTurret_StartBackgroundFX(OBJtObject *ioObject)
{
	OBJtOSD_Turret			*turret_osd;

	UUmAssert( ioObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) ioObject->object_data;
	if (turret_osd->flags & OBJcTurretFlag_BackgroundFX) {
		// background effects are already on!
		return;
	}

	// start background effects
	turret_osd->flags |= OBJcTurretFlag_BackgroundFX;
	OBJiTurret_SendParticleEvent(ioObject, P3cEvent_BackgroundStart, (UUtUns16) -1);
}

// stop background effects on the turret
static void OBJiTurret_StopBackgroundFX(OBJtObject *ioObject)
{
	OBJtOSD_Turret			*turret_osd;

	UUmAssert( ioObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) ioObject->object_data;
	if ((turret_osd->flags & OBJcTurretFlag_BackgroundFX) == 0) {
		// background effects are already off!
		return;
	}

	// start background effects
	turret_osd->flags &= ~OBJcTurretFlag_BackgroundFX;
	OBJiTurret_SendParticleEvent(ioObject, P3cEvent_BackgroundStop, (UUtUns16) -1);
}

// ------------------------------------------------------------------------------------
// called every tick while active
void OBJiTurretCombat_Update(OBJtObject *ioObject)
{
//	AI2tCombatState			*combat_state;
	UUtUns32				current_time;
	UUtBool					this_turret_has_a_target;
	OBJtOSD_Turret			*turret_osd;

	UUmAssert( ioObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) ioObject->object_data;

	current_time = ONrGameState_GetGameTime();

	this_turret_has_a_target	= (turret_osd->targeting.target != NULL) ? UUcTrue : UUcFalse;
	
	if( turret_osd->check_target_time ) {
		turret_osd->check_target_time--;
	}
	else {
		// check that our target is still valid... may also change targets or drop us out of the combat manager
		this_turret_has_a_target = OBJiTurretCombat_CheckTarget(ioObject);

		if (this_turret_has_a_target) {
			turret_osd->target_los = OBJrTurret_PerformLOS( ioObject );
		}
		else {
			turret_osd->target_los			= UUcFalse;
			turret_osd->desired_horiz_angle = 0.f;
			turret_osd->desired_vert_angle	= 0.f;
		}
	}

	if (this_turret_has_a_target) {
		// update the targeting system...
		AI2rTargeting_Update( &turret_osd->targeting, this_turret_has_a_target, UUcTrue, turret_osd->target_los, NULL );

		// fire our weapon if necessary...
		AI2rTargeting_Execute( &turret_osd->targeting, UUcTrue, UUcTrue, turret_osd->target_los );
	}

	return;
}

// ------------------------------------------------------------------------------------
// exit a combat state - called when the turret deactivates
void OBJiTurretCombat_Exit(OBJtObject *ioObject)
{
	OBJtOSD_Turret			*turret_osd;

	UUmAssert( ioObject->object_type == OBJcType_Turret );
	UUmAssert(ioObject->object_data);

	turret_osd = (OBJtOSD_Turret*) ioObject->object_data;
	
	turret_osd->targeting.target = NULL;

	AI2rTargeting_Terminate(&turret_osd->targeting);
}

// ------------------------------------------------------------------------------------
// -- targeting callbacks

static void OBJiTurret_TargetingCallback_Fire(AI2tTargetingState *inTargetingState, UUtBool inFire)
{
	OBJtOSD_Turret			*turret_osd;
	OBJtObject				*object;

	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Turret);

	object = (OBJtObject*) inTargetingState->owner.owner.turret;
	turret_osd = (OBJtOSD_Turret*) object->object_data;
	UUmAssert( object->object_type == OBJcType_Turret );

	turret_osd->fire_weapon = inFire;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// targeting AI callbacks

static void OBJiTurret_TargetingCallback_GetPosition(AI2tTargetingState *inTargetingState, M3tPoint3D *inPoint)
{
	OBJtOSD_Turret			*turret_osd;
	OBJtObject*				object;

	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Turret);
	
	object = (OBJtObject*) inTargetingState->owner.owner.turret;

	UUmAssert(object->object_type == OBJcType_Turret);

	turret_osd = (OBJtOSD_Turret*) object->object_data;

	inPoint->x = object->position.x + turret_osd->turret_class->turret_position.x + turret_osd->turret_class->barrel_position.x;
	inPoint->y = object->position.y + turret_osd->turret_class->turret_position.y + turret_osd->turret_class->barrel_position.y;
	inPoint->z = object->position.z + turret_osd->turret_class->turret_position.z + turret_osd->turret_class->barrel_position.z;
}

static void OBJiTurret_TargetingCallback_AimPoint(AI2tTargetingState *inTargetingState, M3tPoint3D *inPoint)
{
	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Turret);
	OBJrTurret_LookAtPoint( (OBJtObject*) inTargetingState->owner.owner.turret, inPoint);
}

static void OBJrTurret_AimCharacter(OBJtObject *inObject, ONtCharacter *inCharacter)
{
	M3tPoint3D	loc;
	loc = inCharacter->location;
	loc.y += inCharacter->heightThisFrame;
	OBJrTurret_LookAtPoint( inObject, &loc );
}

static void OBJiTurret_TargetingCallback_AimCharacter(AI2tTargetingState *inTargetingState, ONtCharacter *inCharacter)
{
	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Turret);

	OBJrTurret_AimCharacter( (OBJtObject*) inTargetingState->owner.owner.turret, inCharacter);
}

static void OBJiTurret_TargetingCallback_UpdateAiming(AI2tTargetingState *inTargetingState)
{
	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Turret);
}

static void OBJiTurret_TargetingCallback_AimVector(AI2tTargetingState *inTargetingState, M3tVector3D *inDirection)
{
	M3tPoint3D			loc;
	OBJtOSD_Turret		*turret_osd;
	OBJtObject*			object;

	UUmAssert(inTargetingState->owner.type == AI2cTargetingOwnerType_Turret);
	
	object = (OBJtObject*) inTargetingState->owner.owner.turret;

	UUmAssert(object->object_type == OBJcType_Turret);

	turret_osd = (OBJtOSD_Turret*) object->object_data;
	if( !turret_osd->turret_class )
		return;

	loc.x = object->position.x + turret_osd->turret_class->turret_position.x + turret_osd->turret_class->barrel_position.x + inDirection->x;
	loc.y = object->position.y + turret_osd->turret_class->turret_position.y + turret_osd->turret_class->barrel_position.y + inDirection->y;
	loc.z = object->position.z + turret_osd->turret_class->turret_position.z + turret_osd->turret_class->barrel_position.z + inDirection->z;

	OBJrTurret_LookAtPoint( object, &loc );
}

// called every frame by display code
void OBJrTurretDisplayDebuggingInfo(OBJtObject *inObject)
{
	OBJtOSD_Turret			*turret_osd;
	//AI2tCombatState *combat_state = &ioCharacter->ai2State.currentState->state.combat;
	AI2tTargetingState *targeting_state;
	M3tVector3D p0, p1, y_ax, z_ax;
	UUtUns32 itr;
	IMtShade shade;

	UUmAssert(inObject->object_type == OBJcType_Turret);

	turret_osd = (OBJtOSD_Turret*) inObject->object_data;

	targeting_state = &turret_osd->targeting;

#if 0
	// draw a debug line for aiming
	if( (turret_osd->flags & OBJcTurretFlag_Active) && targeting_state->.target )
	{
		M3tPoint3D	laserFrom;
		M3tPoint3D	laserTo;
		float		dist;

		laserFrom = inObject->position;
		laserFrom.x += turret_osd->turret_class->barrel_position.x;
		laserFrom.x += turret_osd->turret_class->turret_position.x;

		laserFrom.y += turret_osd->turret_class->barrel_position.y;
		laserFrom.y += turret_osd->turret_class->turret_position.y;

		laserFrom.z += turret_osd->turret_class->barrel_position.z;
		laserFrom.z += turret_osd->turret_class->turret_position.z;

		laserTo = turret_osd->targeting.target->location;
		laserTo.y += turret_osd->targeting.target->heightThisFrame;

		M3rGeom_Line_Light(&laserFrom, &laserTo,  0x00FF00);

		dist = MUrPoint_Distance( &laserFrom, &laserTo );

		laserTo.x = 0;
		laserTo.y = 0;
		laserTo.z = dist;
		
		MUrPoint_RotateXAxis( &laserTo, turret_osd->vert_angle , &laserTo );
		MUrPoint_RotateYAxis( &laserTo, turret_osd->horiz_angle , &laserTo );

		laserTo.x += laserFrom.x;
		laserTo.y += laserFrom.y;
		laserTo.z += laserFrom.z;

		M3rGeom_Line_Light(&laserFrom, &laserTo,  0x0000FF);
	}
#endif
	// display combat debugging info
	if ( targeting_state->last_computation_success) 
	{
		M3tMatrix3x3 shooter_matrix, worldmatrix;
		M3tPoint3D weapon_dir, shoot_at;

		/*
		 * draw debugging info about our targeting vectors
		 */
		
		shade = (targeting_state->last_computed_ontarget) ? IMcShade_Green : IMcShade_Red;

		MUmVector_Copy(p0, targeting_state->current_aim_pt);
		MUmVector_Copy(p1, p0);
		
		p0.x += 3.0f;
		p1.x -= 3.0f;
		M3rGeom_Line_Light(&p0, &p1, IMcShade_Blue);
		
		p0.x -= 3.0f;
		p1.x += 3.0f;
		p0.y += 3.0f;
		p1.y -= 3.0f;
		M3rGeom_Line_Light(&p0, &p1, IMcShade_Blue);
		
		p0.y -= 3.0f;
		p1.y += 3.0f;
		p0.z += 3.0f;
		p1.z -= 3.0f;
		M3rGeom_Line_Light(&p0, &p1, IMcShade_Blue);

		// calculate the current matrix from shooterspace to worldspace
		MUrMatrix3x3_Transpose((M3tMatrix3x3 *) &targeting_state->weapon_parameters->shooter_inversematrix, &shooter_matrix);
		MUrMatrix3x3_Multiply((M3tMatrix3x3 *) targeting_state->weapon_matrix, &shooter_matrix, &worldmatrix);
		MUrMatrix_GetCol((M3tMatrix4x3 *) &worldmatrix, 1, &y_ax);
		MUrMatrix_GetCol((M3tMatrix4x3 *) &worldmatrix, 2, &z_ax);

		// work out where this points our gun
		MUrMatrix_GetCol((M3tMatrix4x3 *) &worldmatrix, 0, &weapon_dir);
		MUmVector_Copy(shoot_at, targeting_state->weapon_pt);
		MUmVector_ScaleIncrement(shoot_at, targeting_state->aiming_distance, weapon_dir);

		M3rGeom_Line_Light(&targeting_state->weapon_pt, &targeting_state->current_aim_pt, IMcShade_Blue);

		// draw our targeting vector
		MUmVector_Copy(p0, targeting_state->targeting_frompt);
		MUmVector_Add(p1, p0, targeting_state->targeting_vector);
		M3rGeom_Line_Light(&p0, &p1, IMcShade_Yellow);

		// draw our gun's aiming vector
		MUmVector_Copy(p0, shoot_at);
		MUmVector_Copy(p1, shoot_at);

		M3rGeom_Line_Light(&targeting_state->weapon_pt, &shoot_at, shade);
		MUmVector_ScaleIncrement(p0,  1.0f, y_ax);
		MUmVector_ScaleIncrement(p1, -1.0f, y_ax);
		M3rGeom_Line_Light(&p0, &p1, shade);

		MUmVector_ScaleIncrement(p0, -1.0f, y_ax);
		MUmVector_ScaleIncrement(p1,  1.0f, y_ax);
		MUmVector_ScaleIncrement(p0,  1.0f, z_ax);
		MUmVector_ScaleIncrement(p1, -1.0f, z_ax);
		M3rGeom_Line_Light(&p0, &p1, shade);

		// draw accuracy ring around the target point
		MUmVector_Copy(p1, targeting_state->current_aim_pt);
		MUmVector_ScaleIncrement(p1, targeting_state->last_computed_accuracy, y_ax);
		for (itr = 0; itr <= 32; itr++) {
			MUmVector_Copy(p0, p1);
			MUmVector_Copy(p1, targeting_state->current_aim_pt);
			MUmVector_ScaleIncrement(p1, targeting_state->last_computed_accuracy * MUrCos(itr * M3c2Pi / 32), y_ax);
			MUmVector_ScaleIncrement(p1, targeting_state->last_computed_accuracy * MUrSin(itr * M3c2Pi / 32), z_ax);
			M3rGeom_Line_Light(&p0, &p1, shade);
		}

		// draw tolerance ring around the target point
		MUmVector_Copy(p1, targeting_state->current_aim_pt);
		MUmVector_ScaleIncrement(p1, targeting_state->last_computed_tolerance, y_ax);
		for (itr = 0; itr <= 32; itr++) {
			MUmVector_Copy(p0, p1);
			MUmVector_Copy(p1, targeting_state->current_aim_pt);
			MUmVector_ScaleIncrement(p1, targeting_state->last_computed_tolerance * MUrCos(itr * M3c2Pi / 32), y_ax);
			MUmVector_ScaleIncrement(p1, targeting_state->last_computed_tolerance * MUrSin(itr * M3c2Pi / 32), z_ax);
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Blue);
		}
	}
	
	if ( (targeting_state->predictionbuf != NULL)) {
		/*
		 * draw debugging info about our prediction vectors
		 */
		
		UUtUns32 itr, sample_trend, sample_vel, sample_now;
		UUtUns32 trend_frames, velocity_frames, delay_frames;			
		
		trend_frames = targeting_state->targeting_params->predict_trendframes;
		velocity_frames = targeting_state->targeting_params->predict_velocityframes;
		delay_frames = targeting_state->targeting_params->predict_delayframes;
		
		// draw our prediction buffer
		for (itr = 0; itr < trend_frames; itr++) {
			// work out which indices we're using to build the current motion estimate
			sample_trend = targeting_state->next_sample + UUmMin(targeting_state->num_samples_taken, trend_frames);
			sample_vel   = targeting_state->next_sample + UUmMin(targeting_state->num_samples_taken, velocity_frames);
			sample_now   = targeting_state->next_sample + UUmMin(targeting_state->num_samples_taken, delay_frames);
			
			// the buffer is predict_trendframes long
			sample_trend %= trend_frames;
			sample_vel   %= trend_frames;
			sample_now   %= trend_frames;
			
			if ((targeting_state->num_samples_taken >= trend_frames) ||
				(itr > targeting_state->next_sample)) {
				MUmVector_Copy(p0, targeting_state->predictionbuf[itr]);
				MUmVector_Copy(p1, targeting_state->predictionbuf[itr]);
				p0.y += 5.0f;
				p1.y -= 5.0f;
				
				if (itr == sample_now) {
					shade = IMcShade_Yellow;
				} else if (itr == sample_vel) {
					shade = IMcShade_Green;
				} else if (itr == sample_trend) {
					shade = IMcShade_Red;
				} else {
					shade = IMcShade_White;
				}
				
				M3rGeom_Line_Light(&p0, &p1, shade);
			}
		}

		if (targeting_state->valid_target_pt) {
			// draw our target point
			MUmVector_Copy(p0, targeting_state->target_pt);
			MUmVector_Copy(p1, p0);
			
			p0.x += 3.0f;
			p1.x -= 3.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Purple);
			
			p0.x -= 3.0f;
			p1.x += 3.0f;
			p0.y += 3.0f;
			p1.y -= 3.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Purple);
			
			p0.y -= 3.0f;
			p1.y += 3.0f;
			p0.z += 3.0f;
			p1.z -= 3.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Purple);
		}
		
		if (targeting_state->target != NULL) 
		{
			// draw the target's predicted velocity and trend vectors
			MUmVector_Copy(p0, targeting_state->target->location);
			p0.y += targeting_state->target->heightThisFrame;
			
			MUmVector_Add(p1, p0, targeting_state->predicted_trend);
			M3rGeom_Line_Light(&p0, &p1, IMcShade_LightBlue);
			
			MUmVector_Add(p1, p0, targeting_state->predicted_velocity);
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Blue);
			
			// draw a little tick on the velocity vector
			MUmVector_ScaleIncrement(p0, targeting_state->current_prediction_accuracy, targeting_state->predicted_velocity);
			MUmVector_Copy(p1, p0);
			p0.y += 2.0f;
			p1.y -= 2.0f;
			M3rGeom_Line_Light(&p0, &p1, IMcShade_Orange);
		}
	}
}

// ======================================================================
// mechanics handlers
// ======================================================================

static UUtError OBJiTurret_LevelBegin( OBJtObject *inObject )
{
	OBJtOSD_Turret *turret_osd;

	UUmAssert( inObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*) inObject->object_data;

	if( !turret_osd->turret_class )
		return UUcError_None;

	OBJrTurret_SetupParticles( (OBJtObject*) inObject );

	turret_osd->flags |= OBJcTurretFlag_Initialized;

	return UUcError_None;
}

static UUtError OBJiTurret_LevelEnd( OBJtObject *inObject )
{
	OBJiTurret_Shutdown( inObject );

	return UUcError_None;
}

UUtError OBJrTurret_Reset( OBJtObject *inObject )
{
	OBJtOSD_Turret			*turret_osd;

	turret_osd = (OBJtOSD_Turret*)inObject->object_data;

	if( !turret_osd->turret_class )
		return UUcError_None;

	// deactivate the turret
	OBJrTurret_Deactivate( inObject );

	return UUcError_None;
}

float OBJiTurret_AngleDelta( float inAngle, float inDesired )
{
	float	dist1;
	float	dist2;

	if( inDesired > inAngle )
	{
		dist1	= inDesired - inAngle;
		dist2	= -((M3c2Pi - inDesired) + inAngle);
	}
	else
	{
		dist1	= -(inAngle - inDesired);
		dist2	= (M3c2Pi - inAngle) + inDesired;
	}
	if( fabs(dist1) < fabs(dist2) )
		return dist1;
	return dist2;
}

static float OBJiTurret_ClipAngle( float inAngle, float min_angle, float max_angle)
{
	float clipped_angle = (inAngle < M3cPi) ? inAngle : (inAngle - M3c2Pi);

	if (clipped_angle < min_angle) {
		clipped_angle = min_angle;
	}
	else if (clipped_angle > max_angle) {
		clipped_angle = max_angle;
	}

	UUmTrig_Clip(clipped_angle);

	return clipped_angle;
}


static void OBJiTurret_Update( OBJtObject *inObject )
{
	OBJtOSD_Turret			*turret_osd;
	float					delta_theta;

	UUmAssert( inObject->object_type == OBJcType_Turret );

	turret_osd = (OBJtOSD_Turret*)inObject->object_data;

	// update sound
	if (turret_osd->playing_sound) {
		OSrAmbient_Update(turret_osd->playing_sound, &inObject->position, NULL, NULL);
	}

	// if we are active...
	if( turret_osd->state == OBJcTurretState_Active ) 
	{
		// check for timeout
		if (turret_osd->turret_class->timeout && turret_osd->active_time > turret_osd->turret_class->timeout) {
			if (turret_osd->desired_vert_angle == turret_osd->vert_angle && turret_osd->desired_horiz_angle == turret_osd->horiz_angle) {
				OBJrTurret_Deactivate( inObject );
			}
		}
		else {
			// update timer
			turret_osd->active_time++;

			// update combat logic
			OBJiTurretCombat_Update( inObject );
		}
	}
	else
	{
		turret_osd->fire_weapon = UUcFalse;
	}
	
	// clip desired angle to limits
	turret_osd->desired_vert_angle = OBJiTurret_ClipAngle( turret_osd->desired_vert_angle, turret_osd->turret_class->min_vert_angle, turret_osd->turret_class->max_vert_angle );
	turret_osd->desired_horiz_angle = OBJiTurret_ClipAngle( turret_osd->desired_horiz_angle, turret_osd->turret_class->min_horiz_angle, turret_osd->turret_class->max_horiz_angle );

	// update movement
	if( turret_osd->desired_vert_angle != turret_osd->vert_angle || turret_osd->desired_horiz_angle != turret_osd->horiz_angle )
	{
		delta_theta = OBJiTurret_AngleDelta( turret_osd->vert_angle, turret_osd->desired_vert_angle );
		delta_theta = UUmPin( delta_theta, -turret_osd->turret_class->max_vert_speed, turret_osd->turret_class->max_vert_speed );
		turret_osd->vert_angle += delta_theta;
		UUmTrig_Clip(turret_osd->vert_angle);

		delta_theta = OBJiTurret_AngleDelta( turret_osd->horiz_angle, turret_osd->desired_horiz_angle );
		delta_theta = UUmPin( delta_theta, -turret_osd->turret_class->max_horiz_speed, turret_osd->turret_class->max_horiz_speed );
		turret_osd->horiz_angle += delta_theta;
		UUmTrig_Clip(turret_osd->horiz_angle);
	}

	// update background FX
	if (turret_osd->flags & OBJcTurretFlag_Visible) {
		turret_osd->notvisible_time = 0;
		OBJiTurret_StartBackgroundFX(inObject);
	} else {
		turret_osd->notvisible_time++;
		if (turret_osd->notvisible_time >= OBJcTurret_NotVisibleTime) {
			OBJiTurret_StopBackgroundFX(inObject);
		}
	}

	// update firing
	if (turret_osd->chamber_time) {
		turret_osd->chamber_time--;
	}
	else if((!AI2gEveryonePassive) && (turret_osd->fire_weapon)) {
		OBJiTurret_FireProjectile( inObject, 0 );
		turret_osd->fire_weapon = UUcFalse;
	}
	else if( turret_osd->flags & OBJcTurretFlag_IsFiring ) {
		OBJiTurret_SendParticleEvent( inObject, P3cEvent_Stop, -1 );
		turret_osd->flags &= ~OBJcTurretFlag_IsFiring;
	}

	return;
}

static UUtUns16 OBJrTurret_GetID( OBJtObject *inObject )
{
	OBJtOSD_Turret			*osd;
	UUmAssert( inObject && inObject->object_data );
	osd = (OBJtOSD_Turret*)inObject->object_data;

	return osd->id;
}

// ======================================================================
// initialize the turret class
// ======================================================================

UUtError OBJrTurret_Initialize(void)
{
	UUtError							error;
	OBJtMethods							methods;
	ONtMechanicsMethods					mechanics_methods;
	
	// intialize the globals
	OBJgTurret_DrawTurretDebugInfo		= UUcFalse;
	OBJgTurret_DrawTurrets				= UUcTrue;

	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));
	
	// set up the methods structure
	methods.rNew						= OBJiTurret_New;
	methods.rSetDefaults				= OBJiTurret_SetDefaults;
	methods.rDelete						= OBJiTurret_Delete;
	methods.rDraw						= OBJiTurret_Draw;
	methods.rEnumerate					= OBJiTurret_Enumerate;
	methods.rGetBoundingSphere			= OBJiTurret_GetBoundingSphere;
	methods.rOSDGetName					= OBJiTurret_OSDGetName;
	methods.rIntersectsLine				= OBJiTurret_IntersectsLine;
	methods.rUpdatePosition				= OBJiTurret_UpdatePosition;
	methods.rGetOSD						= OBJiTurret_GetOSD;
	methods.rGetOSDWriteSize			= OBJiTurret_GetOSDWriteSize;
	methods.rSetOSD						= OBJiTurret_SetOSD;
	methods.rRead						= OBJiTurret_Read;
	methods.rWrite						= OBJiTurret_Write;
	
	// set up the type methods
	methods.rGetClassVisible			= OBJiTurret_GetVisible;
	methods.rSetClassVisible			= OBJiTurret_SetVisible;
	methods.rSearch						= OBJiTurret_Search;
	
	// set up the mechanics methods
	mechanics_methods.rInitialize		= OBJiTurret_LevelBegin;
	mechanics_methods.rTerminate		= OBJiTurret_LevelEnd;
	mechanics_methods.rReset			= OBJrTurret_Reset;
	mechanics_methods.rUpdate			= OBJiTurret_Update;
	mechanics_methods.rGetID			= OBJrTurret_GetID;

 	mechanics_methods.rClassLevelBegin	= NULL;
	mechanics_methods.rClassLevelEnd	= NULL;
	mechanics_methods.rClassReset		= NULL;
	mechanics_methods.rClassUpdate		= NULL;
	
	// register the methods
	error = ONrMechanics_Register( OBJcType_Turret, OBJcTypeIndex_Turret, "Turret", sizeof(OBJtOSD_Turret), &methods, 0, &mechanics_methods );
	UUmError_ReturnOnError(error);
	
#if CONSOLE_DEBUGGING_COMMANDS
	// register the id set function
	error = SLrGlobalVariable_Register_Bool( "turret_show_debug", "Enables the display of turret debug lines",  &OBJgTurret_DrawTurretDebugInfo);
	UUmError_ReturnOnError(error);

	// show or hides the turrets
	error = SLrGlobalVariable_Register_Bool( "show_turrets", "Enables the display of turrets", &OBJgTurret_DrawTurrets);
	UUmError_ReturnOnError(error);
#endif
	
	return UUcError_None;
}

void OBJrTurret_UpdateSoundPointers(void)
{
	UUtUns32 itr, num_classes;
	OBJtTurretClass *turret_class;
	UUtError error;

	num_classes = TMrInstance_GetTagCount(OBJcTemplate_TurretClass);

	for (itr = 0; itr < num_classes; itr++) {
		error = TMrInstance_GetDataPtr_ByNumber(OBJcTemplate_TurretClass, itr, (void **) &turret_class);
		if (error == UUcError_None) {
			turret_class->active_sound  = OSrAmbient_GetByName(turret_class->active_soundname);
		}
	}
}
