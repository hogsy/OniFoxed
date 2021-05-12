// ======================================================================
// OT_TriggerVolume.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ScriptLang.h"
#include "BFW_TextSystem.h"

#include "Oni_Object.h"
#include "Oni_Character.h"
#include "Oni_Object_Private.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Mechanics.h"

#include "Oni_AI2.h"

#include <ctype.h>
#include "EulerAngles.h"

// ======================================================================
// defines
// ======================================================================

// ======================================================================
// globals
// ======================================================================

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================

UUtBool OBJgTriggerVolume_Visible = UUcFalse;

UUtBool OBJrTriggerVolume_IntersectsBBox_Approximate(const OBJtObject *inObject, const M3tBoundingBox_MinMax *inBBox)
{
	OBJtOSD_All				*inOSD = (OBJtOSD_All *) inObject->object_data;
	OBJtOSD_TriggerVolume	*trigger_osd = &inOSD->osd.trigger_volume_osd;
	UUtBool hit_trigger = UUcFalse;

	{
		M3tVector3D ray_velocity;

		MUmVector_Subtract(ray_velocity, inBBox->maxPoint, inBBox->minPoint);

		hit_trigger = PHrCollision_Volume_Ray(
			&trigger_osd->volume,
			&inBBox->minPoint,
			&ray_velocity,
			NULL,
			NULL);

		if (!hit_trigger) {
			hit_trigger = PHrCollision_Volume_Point_Inside(&trigger_osd->volume, &inBBox->minPoint);
		}
	}

	return hit_trigger;
}


static UUtBool TrivialReject(M3tBoundingSphere *inSphere1, M3tBoundingSphere *inSphere2)
{
	float sum_of_two_radius_squared = UUmSQR(inSphere1->radius + inSphere2->radius);
	float distance_squared = MUrPoint_Distance_Squared(&inSphere1->center, &inSphere2->center);
	UUtBool result = distance_squared > sum_of_two_radius_squared;

	return result;
}

UUtBool OBJrTriggerVolume_IntersectsCharacter(const OBJtObject *inObject, UUtUns32 inTeamMask, const ONtCharacter *inCharacter)
{
	OBJtOSD_All				*inOSD = (OBJtOSD_All *) inObject->object_data;
	OBJtOSD_TriggerVolume	*trigger_osd = &inOSD->osd.trigger_volume_osd;
	UUtBool hit_trigger = UUcFalse;
	ONtActiveCharacter *active_character;

	if (NULL == inCharacter) {
		goto exit;
	}

	active_character = ONrGetActiveCharacter(inCharacter);
	if (NULL == active_character) {
		goto exit;
	}

	if ((1 << inCharacter->teamNumber) & (inTeamMask))
	{
		if (TrivialReject(&active_character->trigger_sphere, &trigger_osd->sphere)) {
#if DEBUGGING
			M3tVector3D ray_velocity;

			MUmVector_Subtract(ray_velocity, active_character->trigger_points[1], active_character->trigger_points[0]);

			hit_trigger = PHrCollision_Volume_Ray(
				&trigger_osd->volume,
				active_character->trigger_points + 0,
				&ray_velocity,
				NULL,
				NULL);

			if (!hit_trigger) {
				hit_trigger = PHrCollision_Volume_Point_Inside(&trigger_osd->volume, active_character->trigger_points + 0);
			}

			if (hit_trigger) {
				COrConsole_Printf("trivial rejected a trigger volume we hit");
			}
#endif
		}
		else {
			M3tVector3D ray_velocity;

			MUmVector_Subtract(ray_velocity, active_character->trigger_points[1], active_character->trigger_points[0]);

			hit_trigger = PHrCollision_Volume_Ray(
				&trigger_osd->volume,
				active_character->trigger_points + 0,
				&ray_velocity,
				NULL,
				NULL);

			if (!hit_trigger) {
				hit_trigger = PHrCollision_Volume_Point_Inside(&trigger_osd->volume, active_character->trigger_points + 0);
			}
		}
	}

exit:
	return hit_trigger;
}

static void Trigger_BuildVolume(OBJtObject *inObject)
{
	OBJtOSD_All				*inOSD = (OBJtOSD_All *) inObject->object_data;
	OBJtOSD_TriggerVolume	*trigger_osd = &inOSD->osd.trigger_volume_osd;
	M3tBoundingBox_MinMax	minmax;
	M3tBoundingBox			bbox;

	UUtUns32				shade = IMcShade_Blue;
	UUtInt32				itr;

	M3tMatrix4x3			matrix;
	M3tMatrix4x3			rotation_matrix;
	M3tBoundingVolume		*volume = &trigger_osd->volume;
	M3tPoint3D				*points = volume->worldPoints;

	OBJrObject_GetRotationMatrix(inObject, &rotation_matrix);

	MUrMatrix_Identity(&matrix);
	MUrMatrixStack_Translate(&matrix, &inObject->position);
	MUrMatrixStack_Matrix(&matrix, &rotation_matrix);

	// step 1: compute world points
	minmax.minPoint.x = 0.f;
	minmax.minPoint.y = 0.f;
	minmax.minPoint.z = 0.f;

	minmax.maxPoint.x = trigger_osd->scale.x;
	minmax.maxPoint.y = trigger_osd->scale.y;
	minmax.maxPoint.z = trigger_osd->scale.z;

	// expand into bounding box, then copy
	M3rMinMaxBBox_To_BBox(&minmax, &bbox);
	UUrMemory_MoveFast(&bbox.localPoints, points, sizeof(M3tPoint3D) * M3cNumBoundingPoints);

	MUrMatrix_MultiplyPoints(8, &matrix, points, points);

	// step 2 compute faces (easy)
	UUrMemory_MoveFast(M3gBBox_QuadList, volume->faces, sizeof(M3tQuad) * M3cNumBoundingFaces);

	// step 3 compute normals
	for (itr = 0; itr < M3cNumBoundingFaces; itr++)
	{
		MUrVector_NormalFromPoints(
			volume->worldPoints + volume->faces[itr].indices[0],
			volume->worldPoints + volume->faces[itr].indices[1],
			volume->worldPoints + volume->faces[itr].indices[2],
			volume->normals + itr);
	}

	// step 4 compute planes
	for (itr = 0; itr < M3cNumBoundingFaces; itr++)
	{
		M3tVector3D normal;
		M3tPlaneEquation *plane;
		M3tPoint3D *point;

		normal = volume->normals[itr];
	
		point = volume->worldPoints + volume->faces[itr].indices[0];
		plane = volume->curPlanes + itr;

		plane->a = normal.x;
		plane->b = normal.y;
		plane->c = normal.z;
		plane->d = -(normal.x*point->x + normal.y*point->y + normal.z*point->z);

		volume->curProjections[itr] = (UUtUns16) 
			CLrQuad_FindProjection(volume->worldPoints,	volume->faces + itr);

		// degenerate quads should be culled at import time
		UUmAssert(volume->curProjections[itr] != CLcProjection_Unknown);
	}

	M3rBVolume_To_BSphere(volume, &trigger_osd->sphere);

	return;
}


// ----------------------------------------------------------------------
static void
OBJiTriggerVolume_Delete(
	OBJtObject				*inObject)
{
}

// ----------------------------------------------------------------------
static void
OBJiTriggerVolume_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
#if TOOL_VERSION
	UUtUns32				itr;
	OBJtOSD_All				*inOSD = (OBJtOSD_All *) inObject->object_data;
	OBJtOSD_TriggerVolume	*trigger_osd = &inOSD->osd.trigger_volume_osd;
	M3tPoint3D				*points = trigger_osd->volume.worldPoints;
	M3tPoint3D				trigger_center = MUgZeroPoint;
	UUtUns32				shade = 0xFFFFFF;
	UUtBool					is_selected = ((inDrawFlags & OBJcDrawFlag_Selected) != 0);

	if (!OBJgTriggerVolume_Visible) {
		return;
	}

	if (OBJrTriggerVolume_IntersectsCharacter(inObject, trigger_osd->team_mask, ONgGameState->local.playerCharacter)) {
		shade = IMcShade_Red;
	}
	else if (is_selected)
	{
		shade = IMcShade_Green;
	}
	else
	{
		shade = IMcShade_Blue;
	}

	M3rGeom_Line_Light(points + 0, points + 1, shade);
	M3rGeom_Line_Light(points + 1, points + 3, shade);
	M3rGeom_Line_Light(points + 3, points + 2, shade);
	M3rGeom_Line_Light(points + 2, points + 0, shade);

	M3rGeom_Line_Light(points + 4, points + 5, shade);
	M3rGeom_Line_Light(points + 5, points + 7, shade);
	M3rGeom_Line_Light(points + 7, points + 6, shade);
	M3rGeom_Line_Light(points + 6, points + 4, shade);

	M3rGeom_Line_Light(points + 0, points + 4, shade);
	M3rGeom_Line_Light(points + 1, points + 5, shade);
	M3rGeom_Line_Light(points + 3, points + 7, shade);
	M3rGeom_Line_Light(points + 2, points + 6, shade);

	if (is_selected) {
		M3rGeom_Line_Light(points + 0, points + 3, shade);
		M3rGeom_Line_Light(points + 1, points + 2, shade);

		M3rGeom_Line_Light(points + 4, points + 7, shade);
		M3rGeom_Line_Light(points + 5, points + 6, shade);

		M3rGeom_Line_Light(points + 2, points + 4, shade);
		M3rGeom_Line_Light(points + 1, points + 7, shade);
		M3rGeom_Line_Light(points + 0, points + 6, shade);
		M3rGeom_Line_Light(points + 3, points + 5, shade);

		M3rGeom_Line_Light(points + 2, points + 7, shade);
		M3rGeom_Line_Light(points + 3, points + 6, shade);
		M3rGeom_Line_Light(points + 0, points + 5, shade);
		M3rGeom_Line_Light(points + 1, points + 4, shade);
	}

	trigger_center = MUgZeroPoint;
	for(itr = 0; itr < M3cNumBoundingPoints; itr++)
	{
		MUmVector_Add(trigger_center, trigger_center, points[itr]);
	}

	MUmVector_Scale(trigger_center, (1.0f / ((float) M3cNumBoundingPoints)));
#endif

	return;
}

// ----------------------------------------------------------------------
static UUtError
OBJiTriggerVolume_Enumerate(
	OBJtObject						*inObject,
	OBJtEnumCallback_ObjectName		inEnumCallback,
	UUtUns32						inUserData)
{
	char							name[OBJcMaxNameLength + 1];
	
	OBJrObject_GetName(inObject, name, OBJcMaxNameLength);
	inEnumCallback(name, inUserData);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiTriggerVolume_GetBoundingSphere(
	const OBJtObject		*inObject,
	M3tBoundingSphere		*outBoundingSphere)
{
	outBoundingSphere->center.x = 0.0f;
	outBoundingSphere->center.y = 0.0f;
	outBoundingSphere->center.z = 0.0f;
	outBoundingSphere->radius = 0.f;

	return;
}

// ----------------------------------------------------------------------
static void
OBJiTriggerVolume_OSDGetName(
	const OBJtOSD_All		*inOSD,
	char					*outName,
	UUtUns32				inNameLength)
{
	const OBJtOSD_TriggerVolume		*trigger_osd;

	// get a pointer to the object osd
	trigger_osd = &inOSD->osd.trigger_volume_osd;
	
	UUrString_Copy(outName, trigger_osd->name, inNameLength);
	
	return;
}

static void
OBJiTriggerVolume_OSDSetName(
	OBJtOSD_All				*inOSD,
	const char				*outName)
{
	OBJtOSD_TriggerVolume		*trigger_osd;

	// get a pointer to the object osd
	trigger_osd = &inOSD->osd.trigger_volume_osd;
	
	UUrString_Copy(trigger_osd->name, outName, sizeof(trigger_osd->name));

	return;
}

// ----------------------------------------------------------------------
static void
OBJiTriggerVolume_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	OBJtOSD_TriggerVolume		*trigger_osd;

	// get a pointer to the object osd
	trigger_osd = (OBJtOSD_TriggerVolume *)inObject->object_data;
	
	outOSD->osd.trigger_volume_osd = *trigger_osd;

	return;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiTriggerVolume_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	UUtUns32 trigger_volume_size;
	OBJtOSD_All osd;

	osd;

	trigger_volume_size = 0;
	trigger_volume_size += sizeof(osd.osd.trigger_volume_osd.name);
	trigger_volume_size += sizeof(osd.osd.trigger_volume_osd.entry_script);
	trigger_volume_size += sizeof(osd.osd.trigger_volume_osd.inside_script);
	trigger_volume_size += sizeof(osd.osd.trigger_volume_osd.exit_script);
	trigger_volume_size += sizeof(osd.osd.trigger_volume_osd.team_mask);
	trigger_volume_size += sizeof(osd.osd.trigger_volume_osd.scale);
	trigger_volume_size += sizeof(osd.osd.trigger_volume_osd.id);
	trigger_volume_size += sizeof(osd.osd.trigger_volume_osd.parent_id);
	trigger_volume_size += sizeof(osd.osd.trigger_volume_osd.note);
	trigger_volume_size += sizeof(osd.osd.trigger_volume_osd.authored_flags);

	return trigger_volume_size;
}

// ----------------------------------------------------------------------
static void
OBJiTriggerVolume_DuplicateOSD(
	const OBJtOSD_All *inOSD,
	OBJtOSD_All *outOSD)
{
	UUtUns32				itr;
	OBJtObject				*prev_object;

	UUrMemory_MoveFast(&inOSD->osd.trigger_volume_osd, &outOSD->osd.trigger_volume_osd, sizeof(OBJtOSD_TriggerVolume));

	// find a name that isn't a duplicate
	for (itr = 0; itr < UUcMaxUns16; itr++)
	{
		if (itr == 0) {
			// try input name
		} else if (itr == 1) {
			sprintf(outOSD->osd.trigger_volume_osd.name, "%s_copy", inOSD->osd.trigger_volume_osd.name);
		} else {
			sprintf(outOSD->osd.trigger_volume_osd.name, "%s_copy%d", inOSD->osd.trigger_volume_osd.name, itr);
		}

		prev_object = OBJrObjectType_Search(OBJcType_TriggerVolume, OBJcSearch_TriggerVolumeName, outOSD);
		if (prev_object != NULL) { continue; }

		break;
	}
	UUmAssert(itr < UUcMaxUns16);

	// find a new ID
	for (itr = 0; itr < 1000; itr++)
	{
		outOSD->osd.trigger_volume_osd.id = inOSD->osd.trigger_volume_osd.id + itr;

		prev_object = OBJrObjectType_Search(OBJcType_TriggerVolume, OBJcSearch_TriggerVolumeID, outOSD);
		if (prev_object != NULL) { continue; }

		break;
	}
	UUmAssert(itr < 1000);

	return;
}

// ----------------------------------------------------------------------
static void
OBJiTriggerVolume_GetUniqueOSD(
	OBJtOSD_All *inOSD)
{
	UUtUns32				itr;
	OBJtObject				*prev_object;

	UUrMemory_Clear(&inOSD->osd.trigger_volume_osd, sizeof(OBJtOSD_TriggerVolume));

	inOSD->osd.trigger_volume_osd.team_mask = 0xFFFFFFFF;

	for (itr = 1; itr < UUcMaxUns16; itr++)
	{
		sprintf(inOSD->osd.trigger_volume_osd.name, "trigger_volume_%02d", itr);
		inOSD->osd.trigger_volume_osd.id = (UUtUns16) itr;

		prev_object = OBJrObjectType_Search(OBJcType_TriggerVolume, OBJcSearch_TriggerVolumeName, inOSD);
		if (prev_object != NULL) { continue; }

		prev_object = OBJrObjectType_Search(OBJcType_TriggerVolume, OBJcSearch_TriggerVolumeID, inOSD);
		if (prev_object != NULL) { continue; }

		break;
	}

	return;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiTriggerVolume_IntersectsLine(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inStartPoint,
	const M3tPoint3D		*inEndPoint)
{
	OBJtOSD_All				*inOSD = (OBJtOSD_All *) inObject->object_data;
	OBJtOSD_TriggerVolume	*trigger_osd = &inOSD->osd.trigger_volume_osd;
	M3tPoint3D				vector;
	UUtBool					volume_intersects_line;

	// vector = inEndPoint - inStartPoint;
	MUmVector_Subtract(vector, *inEndPoint, *inStartPoint);

	volume_intersects_line = PHrCollision_Volume_Ray(&trigger_osd->volume, inStartPoint, &vector, NULL, NULL);

	return volume_intersects_line;
}

// ----------------------------------------------------------------------
static UUtError
OBJiTriggerVolume_SetDefaults(
	OBJtOSD_All			*outOSD)
{
	OBJiTriggerVolume_GetUniqueOSD(outOSD);

	outOSD->osd.trigger_volume_osd.scale.x = 1.f;
	outOSD->osd.trigger_volume_osd.scale.y = 1.f;
	outOSD->osd.trigger_volume_osd.scale.z = 1.f;
	strcpy(outOSD->osd.trigger_volume_osd.entry_script, "");
	strcpy(outOSD->osd.trigger_volume_osd.exit_script, "");
	strcpy(outOSD->osd.trigger_volume_osd.entry_script, "");
	strcpy(outOSD->osd.trigger_volume_osd.note, "");

	outOSD->osd.trigger_volume_osd.authored_flags = OBJcOSD_TriggerVolume_PlayerOnly;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiTriggerVolume_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_All				osd_all;
	UUtError				error;
	
	if (NULL == inOSD) {
		error = OBJiTriggerVolume_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);
	}
	else
	{
		OBJiTriggerVolume_DuplicateOSD(inOSD, &osd_all);
	}
	
	// set the object specific data and position
	error = OBJrObject_SetObjectSpecificData(inObject, &osd_all);
	UUmError_ReturnOnError(error);
	
	OBJrObject_UpdatePosition(inObject);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrTriggerVolume_Reset(
	OBJtObject				*inObject)
{
	OBJtOSD_TriggerVolume *trig_osd;

	UUmAssertReadPtr(inObject, sizeof(*inObject));
	UUmAssert(inObject->object_type == OBJcType_TriggerVolume);

	trig_osd = (OBJtOSD_TriggerVolume *) inObject->object_data;
	UUmAssertReadPtr(trig_osd, sizeof(*trig_osd));

	trig_osd ->in_game_flags = trig_osd->authored_flags;
	UUrString_Copy(trig_osd->cur_entry_script, trig_osd->entry_script, sizeof(trig_osd->cur_entry_script));
	UUrString_Copy(trig_osd->cur_inside_script, trig_osd->inside_script, sizeof(trig_osd->cur_inside_script));
	UUrString_Copy(trig_osd->cur_exit_script, trig_osd->exit_script, sizeof(trig_osd->cur_exit_script));

	Trigger_BuildVolume(inObject);
		
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiTriggerVolume_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_TriggerVolume	*trigger_osd = (OBJtOSD_TriggerVolume *) inObject->object_data;
	UUtUns32				bytes_read = 0;

	bytes_read += OBJmGetStringFromBuffer(inBuffer, trigger_osd->name, sizeof(trigger_osd->name), inSwapIt);
	bytes_read += OBJmGetStringFromBuffer(inBuffer, trigger_osd->entry_script, sizeof(trigger_osd->entry_script), inSwapIt);
	bytes_read += OBJmGetStringFromBuffer(inBuffer, trigger_osd->inside_script, sizeof(trigger_osd->inside_script), inSwapIt);
	bytes_read += OBJmGetStringFromBuffer(inBuffer, trigger_osd->exit_script, sizeof(trigger_osd->exit_script), inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, trigger_osd->team_mask, UUtUns32, inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, trigger_osd->scale.x, float, inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, trigger_osd->scale.y, float, inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, trigger_osd->scale.z, float, inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, trigger_osd->id, UUtInt32, inSwapIt);
	if (inVersion >= OBJcVersion_22) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, trigger_osd->parent_id, UUtInt32, inSwapIt);
	}
	else {
		trigger_osd->parent_id = 0;
	}
	bytes_read += OBJmGetStringFromBuffer(inBuffer, trigger_osd->note, sizeof(trigger_osd->note), inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, trigger_osd->authored_flags, UUtUns32, inSwapIt);

	// set up non-persistent data
	OBJrTriggerVolume_Reset(inObject);

	// bring the object up to date
	OBJrObject_UpdatePosition(inObject);
	
	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtError
OBJiTriggerVolume_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_TriggerVolume	*dst_osd = (OBJtOSD_TriggerVolume *) inObject->object_data;
	
	UUmAssert(inOSD);

	UUrMemory_MoveFast(inOSD, dst_osd, sizeof(*dst_osd));

	// reset non-persistent data
	OBJrTriggerVolume_Reset(inObject);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiTriggerVolume_UpdatePosition(
	OBJtObject				*inObject)
{
	Trigger_BuildVolume(inObject);
}

// ----------------------------------------------------------------------
static UUtError
OBJiTriggerVolume_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	OBJtOSD_TriggerVolume	*trigger_osd = (OBJtOSD_TriggerVolume *) inObject->object_data;
	UUtUns32				bytes_available = bytes_available = *ioBufferSize;

	OBJmWriteStringToBuffer(ioBuffer, trigger_osd->name, sizeof(trigger_osd->name), bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, trigger_osd->entry_script, sizeof(trigger_osd->entry_script), bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, trigger_osd->inside_script, sizeof(trigger_osd->inside_script), bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, trigger_osd->exit_script, sizeof(trigger_osd->exit_script), bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, trigger_osd->team_mask, UUtUns32, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, trigger_osd->scale.x, float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, trigger_osd->scale.y, float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, trigger_osd->scale.z, float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, trigger_osd->id, UUtInt32, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, trigger_osd->parent_id, UUtInt32, bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, trigger_osd->note, sizeof(trigger_osd->note), bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, trigger_osd->authored_flags, UUtInt32, bytes_available, OBJcWrite_Little);

	// set ioBufferSize to the number of bytes written to the buffer
	*ioBufferSize = *ioBufferSize - bytes_available;

	return UUcError_None;
}

char *OBJrTriggerVolume_IsInvalid(
	OBJtObject *inObject)
{
	UUtUns32 count;
	UUtUns32 itr;

	OBJtOSD_TriggerVolume *in_trigger_volume_osd = (OBJtOSD_TriggerVolume *)inObject->object_data;
	OBJtObjectType object_type = inObject->object_type;

	count = OBJrObjectType_EnumerateObjects(object_type, NULL, 0);

	for(itr = 0; itr < count; itr++)
	{
		OBJtObject *test_object = OBJrObjectType_GetObject_ByNumber(object_type, itr);

		if (test_object != inObject) {
			OBJtOSD_TriggerVolume *test_trigger_volume_osd = (OBJtOSD_TriggerVolume *)test_object->object_data;
			static char *duplicate_name = "Duplicate Name";
			static char *duplicate_id = "Duplicate ID";

			if (UUmString_IsEqual(in_trigger_volume_osd->name, test_trigger_volume_osd->name)) {
				return duplicate_name;
			}

			if (in_trigger_volume_osd->id == test_trigger_volume_osd->id) {
				return duplicate_id;
			}
		}
	}

	if (0 == in_trigger_volume_osd->id) {
		static char *trigger_volume_id_cannot_be_zero = "trigger volume id cannot be zero";
		return trigger_volume_id_cannot_be_zero;
	}

	return NULL;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OBJiTriggerVolume_GetVisible(
	void)
{
	return OBJgTriggerVolume_Visible;
}

// ----------------------------------------------------------------------
static void
OBJiTriggerVolume_SetVisible(
	UUtBool					inIsVisible)
{
	OBJgTriggerVolume_Visible = inIsVisible;

	return;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiTriggerVolume_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	OBJtOSD_TriggerVolume		*trigger_osd;
	UUtBool				found;
	
	// get a pointer to the object osd
	trigger_osd = (OBJtOSD_TriggerVolume *)inObject->object_data;
	
	// perform the check
	switch (inSearchType)
	{
		case OBJcSearch_CombatName:
			found = UUmString_IsEqual(trigger_osd->name, inSearchParams->osd.trigger_volume_osd.name);
		break;

		case OBJcSearch_CombatID:
			found = trigger_osd->id == inSearchParams->osd.trigger_volume_osd.id;
		break;

		default:
			found = UUcFalse;
		break;
	}
	
	return found;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBJrTriggerVolume_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;
	ONtMechanicsMethods		mechanics_methods;
	
	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));
	
	// set up the methods structure
	methods.rNew				= OBJiTriggerVolume_New;
	methods.rSetDefaults		= OBJiTriggerVolume_SetDefaults;
	methods.rDelete				= OBJiTriggerVolume_Delete;
	methods.rDraw				= OBJiTriggerVolume_Draw;
	methods.rEnumerate			= OBJiTriggerVolume_Enumerate;
	methods.rGetBoundingSphere	= OBJiTriggerVolume_GetBoundingSphere;
	methods.rOSDGetName			= OBJiTriggerVolume_OSDGetName;
	methods.rOSDSetName			= OBJiTriggerVolume_OSDSetName;
	methods.rIntersectsLine		= OBJiTriggerVolume_IntersectsLine;
	methods.rUpdatePosition		= OBJiTriggerVolume_UpdatePosition;
	methods.rGetOSD				= OBJiTriggerVolume_GetOSD;
	methods.rGetOSDWriteSize	= OBJiTriggerVolume_GetOSDWriteSize;
	methods.rSetOSD				= OBJiTriggerVolume_SetOSD;
	methods.rRead				= OBJiTriggerVolume_Read;
	methods.rWrite				= OBJiTriggerVolume_Write;
	methods.rGetClassVisible	= OBJiTriggerVolume_GetVisible;
	methods.rSearch				= OBJiTriggerVolume_Search;
	methods.rSetClassVisible	= OBJiTriggerVolume_SetVisible;
	methods.rGetUniqueOSD		= OBJiTriggerVolume_GetUniqueOSD;
	
	// set up the mechanics methods
	UUrMemory_Clear(&mechanics_methods, sizeof(ONtMechanicsMethods));
	mechanics_methods.rInitialize		= NULL;
	mechanics_methods.rTerminate		= NULL;
	mechanics_methods.rReset			= OBJrTriggerVolume_Reset;
	mechanics_methods.rUpdate			= NULL;
	mechanics_methods.rGetID			= NULL;

 	mechanics_methods.rClassLevelBegin	= NULL;
	mechanics_methods.rClassLevelEnd	= NULL;
	mechanics_methods.rClassReset		= NULL;
	mechanics_methods.rClassUpdate		= NULL;

	// register the methods
	error = ONrMechanics_Register(OBJcType_TriggerVolume, OBJcTypeIndex_TriggerVolume, "Trigger Volume", sizeof(OBJtOSD_TriggerVolume),
									&methods, OBJcObjectGroupFlag_CanSetName, &mechanics_methods );
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

UUtInt32 OBJgTriggerVolume_InsideCount;

static UUtBool AI2iScript_TriggerVolume_CountInside_Callback(OBJtObject *inObject, UUtUns32 inUserData)
{
	OBJtOSD_TriggerVolume *trig_osd;
	UUtInt32 match_id = (UUtInt32) inUserData;

//	UUmAssertReadPtr(data, sizeof(*data));

	UUmAssert(inObject->object_type == OBJcType_TriggerVolume);

	trig_osd = (OBJtOSD_TriggerVolume *) inObject->object_data;

	if (trig_osd->id == match_id) {
		OBJgTriggerVolume_InsideCount += ONrGameState_CountInsideTrigger(inObject);
	}

	return UUcTrue;
}

UUtInt32 OBJrTriggerVolumeID_CountInside(UUtInt32 id)
{
	OBJgTriggerVolume_InsideCount = 0;

	OBJrObjectType_EnumerateObjects(OBJcType_TriggerVolume, AI2iScript_TriggerVolume_CountInside_Callback, id);

	return OBJgTriggerVolume_InsideCount;
}

static UUtBool AI2iScript_TriggerVolume_DeleteCorpsesInside_Callback(OBJtObject *inObject, UUtUns32 inUserData)
{
	OBJtOSD_TriggerVolume *trig_osd;
	UUtInt32 match_id = (UUtInt32) inUserData;

//	UUmAssertReadPtr(data, sizeof(*data));

	UUmAssert(inObject->object_type == OBJcType_TriggerVolume);

	trig_osd = (OBJtOSD_TriggerVolume *) inObject->object_data;

	if (trig_osd->id == match_id) {
		ONrGameState_KillCorpsesInsideTrigger(inObject);
	}

	return UUcTrue;
}

void OBJrTriggerVolumeID_DeleteCorpsesInside(UUtInt32 id)
{
	OBJrObjectType_EnumerateObjects(OBJcType_TriggerVolume, AI2iScript_TriggerVolume_DeleteCorpsesInside_Callback, id);

	return;
}

static UUtBool AI2iScript_TriggerVolume_DeleteCharactersInside_Callback(OBJtObject *inObject, UUtUns32 inUserData)
{
	OBJtOSD_TriggerVolume *trig_osd;
	UUtInt32 match_id = (UUtInt32) inUserData;

	UUmAssert(inObject->object_type == OBJcType_TriggerVolume);

	trig_osd = (OBJtOSD_TriggerVolume *) inObject->object_data;

	if (trig_osd->id == match_id) {
		ONrGameState_DeleteCharactersInsideTrigger(inObject);
	}

	return UUcTrue;
}

void OBJrTriggerVolumeID_DeleteCharactersInside(UUtInt32 id)
{
	OBJrObjectType_EnumerateObjects(OBJcType_TriggerVolume, AI2iScript_TriggerVolume_DeleteCharactersInside_Callback, id);

	return;
}
