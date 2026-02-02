// ======================================================================
// OT_PatrolPath.c
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

#include "Oni_AI2.h"

#include "Oni_Win_AI.h"

#include <ctype.h>
#include "EulerAngles.h"

// ======================================================================
// defines
// ======================================================================
#define OBJcPatrolPath_DefaultDrawNameDistance			150.0f
#define OBJcPatrolPath_DrawSize							  6.0f
#define OBJcPatrolPath_DrawShade						IMcShade_Orange

// ======================================================================
// globals
// ======================================================================

static UUtBool				OBJgPatrolPath_DrawPositions;
static float				OBJgPatrolPath_DrawNameDistance;

#if TOOL_VERSION
// text name drawing
static UUtBool				OBJgPatrolPath_DrawInitialized = UUcFalse;
static UUtRect				OBJgPatrolPath_TextureBounds;
static TStTextContext		*OBJgPatrolPath_TextContext;
static M3tTextureMap		*OBJgPatrolPath_Texture;
static IMtPoint2D			OBJgPatrolPath_Dest;
static IMtPixel				OBJgPatrolPath_WhiteColor;
static UUtInt16				OBJgPatrolPath_TextureWidth;
static UUtInt16				OBJgPatrolPath_TextureHeight;
static float				OBJgPatrolPath_WidthRatio;
#endif

// ======================================================================
// functions
// ======================================================================
#if TOOL_VERSION
// ----------------------------------------------------------------------
UUtError
OBJrPatrolPath_DrawInitialize(
	void)
{
	if (!OBJgPatrolPath_DrawInitialized) {
		UUtError				error;
		TStFontFamily			*font_family;
		IMtPixelType			textureFormat;

		M3rDrawEngine_FindGrayscalePixelType(&textureFormat);

		OBJgPatrolPath_WhiteColor = IMrPixel_FromShade(textureFormat, IMcShade_White);

		error = TSrFontFamily_Get(TScFontFamily_Default, &font_family);
		if (error != UUcError_None)
		{
			goto cleanup;
		}

		error = TSrContext_New(font_family, TScFontSize_Default, TScStyle_Bold, TSc_SingleLine, UUcFalse, &OBJgPatrolPath_TextContext);
		if (error != UUcError_None)
		{
			goto cleanup;
		}

		OBJgPatrolPath_Dest.x = 2;
		OBJgPatrolPath_Dest.y =
			TSrFont_GetLeadingHeight(TSrContext_GetFont(OBJgPatrolPath_TextContext, TScStyle_InUse)) +
			TSrFont_GetAscendingHeight(TSrContext_GetFont(OBJgPatrolPath_TextContext, TScStyle_InUse));

		TSrContext_GetStringRect(OBJgPatrolPath_TextContext, "maximum_length_of_patrol_pt_name", &OBJgPatrolPath_TextureBounds);

		error =
			M3rTextureMap_New(
				OBJgPatrolPath_TextureBounds.right,
				OBJgPatrolPath_TextureBounds.bottom,
				textureFormat,
				M3cTexture_AllocMemory,
				M3cTextureFlags_None,
				"PatrolPath name texture",
				&OBJgPatrolPath_Texture);
		if (error != UUcError_None)
		{
			goto cleanup;
		}

		// get the REAL size of the texture that motoko allocated for us
		M3rTextureRef_GetSize(
			(void *) OBJgPatrolPath_Texture,
			(UUtUns16 *)&OBJgPatrolPath_TextureBounds.right,
			(UUtUns16 *)&OBJgPatrolPath_TextureBounds.bottom);

		OBJgPatrolPath_TextureWidth = OBJgPatrolPath_TextureBounds.right;
		OBJgPatrolPath_TextureHeight = OBJgPatrolPath_TextureBounds.bottom;

		OBJgPatrolPath_WidthRatio = (float)OBJgPatrolPath_TextureWidth / (float)OBJgPatrolPath_TextureHeight;

		OBJgPatrolPath_DrawInitialized = UUcTrue;
	}

	return UUcError_None;

cleanup:
	if (OBJgPatrolPath_TextContext)
	{
		TSrContext_Delete(OBJgPatrolPath_TextContext);
		OBJgPatrolPath_TextContext = NULL;
	}

	return UUcError_Generic;
}

// ----------------------------------------------------------------------
static void
OBJiPatrolPath_DrawName(
	OBJtObject				*inObject,
	M3tPoint3D				*inLocation)
{
	UUtError				error;

	error = OBJrPatrolPath_DrawInitialize();
	if (error == UUcError_None) {
		char					name[OBJcMaxNameLength + 1];
		UUtRect					string_bounds;
		UUtUns16				string_width;
		IMtPoint2D				text_dest;
		float					sprite_size;

		// erase the texture and set the text contexts shade
		M3rTextureMap_Fill(OBJgPatrolPath_Texture, OBJgPatrolPath_WhiteColor, NULL);
		TSrContext_SetShade(OBJgPatrolPath_TextContext, IMcShade_Black);

		// get the patrol path's name
		OBJrObject_GetName(inObject, name, OBJcMaxNameLength);

		// work out how big the string is going to be
		TSrContext_GetStringRect(OBJgPatrolPath_TextContext, name, &string_bounds);
		string_width = string_bounds.right - string_bounds.left;

		text_dest = OBJgPatrolPath_Dest;
		text_dest.x += (OBJgPatrolPath_TextureWidth - string_width) / 2;

		sprite_size = ((float) string_width) / OBJgPatrolPath_TextureWidth;

		// draw the string to the texture
		TSrContext_DrawString(
			OBJgPatrolPath_TextContext,
			OBJgPatrolPath_Texture,
			name,
			&OBJgPatrolPath_TextureBounds,
			&text_dest);

		// draw the sprite
		M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, M3cGeomState_SpriteMode_Normal);
		M3rGeom_State_Commit();

		M3rSprite_Draw(
			OBJgPatrolPath_Texture,
			inLocation,
			OBJgPatrolPath_WidthRatio,
			1.0f,
			IMcShade_White,
			M3cMaxAlpha,
			0,
			NULL,
			NULL,
			0,
			1.0f - sprite_size, 0);
	}

	return;
}

// ----------------------------------------------------------------------
void
OBJrPatrolPath_DrawTerminate(
	void)
{
	if (OBJgPatrolPath_DrawInitialized) {

		if (OBJgPatrolPath_TextContext)
		{
			TSrContext_Delete(OBJgPatrolPath_TextContext);
			OBJgPatrolPath_TextContext = NULL;
		}

		OBJgPatrolPath_DrawInitialized = UUcFalse;
	}
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiPatrolPath_Delete(
	OBJtObject				*inObject)
{
#if TOOL_VERSION
	if (OWgCurrentPatrolPath == (OBJtOSD_PatrolPath *) inObject->object_data) {
		// we are deleting the current patrol path - it is no longer valid
		OWgCurrentPatrolPath = NULL;
		OWgCurrentPatrolPathValid = UUcFalse;
	}
#endif
}

// ----------------------------------------------------------------------
static void
OBJiPatrolPath_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
#if TOOL_VERSION
	OBJtOSD_PatrolPath		*pp_osd;
	M3tPoint3D				camera_location;

	// KNA: replaced size with OBJcPatrolPath_DrawSize so that CodeWarrior would compile
	M3tPoint3D points[6] =
	{
		{-OBJcPatrolPath_DrawSize, 0, 0},
		{+OBJcPatrolPath_DrawSize, 0, 0},
		{0, -OBJcPatrolPath_DrawSize, 0},
		{0, +OBJcPatrolPath_DrawSize, 0},
		{0, 0, -OBJcPatrolPath_DrawSize},
		{0, 0, +OBJcPatrolPath_DrawSize}
	};

	if (OBJgPatrolPath_DrawPositions == UUcFalse) { return; }

	// get a pointer to the object osd
	pp_osd = (OBJtOSD_PatrolPath *)inObject->object_data;

	// set up the matrix stack
	M3rMatrixStack_Push();
	M3rMatrixStack_ApplyTranslate(inObject->position);
	M3rGeom_State_Commit();

	// draw the inside star if this patrol path is selected
	if (inDrawFlags & OBJcDrawFlag_Selected) {
		M3rGeom_Line_Light(points + 0, points + 1, IMcShade_White);
		M3rGeom_Line_Light(points + 2, points + 3, IMcShade_White);
		M3rGeom_Line_Light(points + 4, points + 5, IMcShade_White);
	}

	// draw the patrol path's position marker
	M3rGeom_Line_Light(points + 0, points + 2, OBJcPatrolPath_DrawShade);
	M3rGeom_Line_Light(points + 0, points + 3, OBJcPatrolPath_DrawShade);
	M3rGeom_Line_Light(points + 0, points + 4, OBJcPatrolPath_DrawShade);
	M3rGeom_Line_Light(points + 0, points + 5, OBJcPatrolPath_DrawShade);
	M3rGeom_Line_Light(points + 1, points + 2, OBJcPatrolPath_DrawShade);
	M3rGeom_Line_Light(points + 1, points + 3, OBJcPatrolPath_DrawShade);
	M3rGeom_Line_Light(points + 1, points + 4, OBJcPatrolPath_DrawShade);
	M3rGeom_Line_Light(points + 1, points + 5, OBJcPatrolPath_DrawShade);
	M3rGeom_Line_Light(points + 2, points + 4, OBJcPatrolPath_DrawShade);
	M3rGeom_Line_Light(points + 2, points + 5, OBJcPatrolPath_DrawShade);
	M3rGeom_Line_Light(points + 3, points + 4, OBJcPatrolPath_DrawShade);
	M3rGeom_Line_Light(points + 3, points + 5, OBJcPatrolPath_DrawShade);

	// draw the name
	camera_location = CArGetLocation();
	if (MUrPoint_Distance(&inObject->position, &camera_location) < OBJgPatrolPath_DrawNameDistance)
	{
		OBJiPatrolPath_DrawName(inObject, points + 3);
	}

	M3rMatrixStack_Pop();
#endif

	return;
}

// ----------------------------------------------------------------------
static UUtError
OBJiPatrolPath_Enumerate(
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
OBJiPatrolPath_GetBoundingSphere(
	const OBJtObject		*inObject,
	M3tBoundingSphere		*outBoundingSphere)
{
	outBoundingSphere->center.x = 0.0f;
	outBoundingSphere->center.y = 0.0f;
	outBoundingSphere->center.z = 0.0f;
	outBoundingSphere->radius = OBJcPatrolPath_DrawSize / 2.0f;	// just an approximation
}

// ----------------------------------------------------------------------
static void
OBJiPatrolPath_OSDGetName(
	const OBJtOSD_All		*inOSD,
	char					*outName,
	UUtUns32				inNameLength)
{
	const OBJtOSD_PatrolPath *pp_osd = &inOSD->osd.patrolpath_osd;

	UUrString_Copy(outName, pp_osd->name, inNameLength);

	return;
}

// ----------------------------------------------------------------------
static void
OBJiPatrolPath_OSDSetName(
	OBJtOSD_All		*inOSD,
	const char					*inName)
{
	OBJtOSD_PatrolPath *pp_osd = &inOSD->osd.patrolpath_osd;

	UUrString_Copy(pp_osd->name, inName, sizeof(pp_osd->name));

	return;
}

// ----------------------------------------------------------------------
static void
OBJiPatrolPath_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	OBJtOSD_PatrolPath		*pp_osd;

	// get a pointer to the object osd
	pp_osd = (OBJtOSD_PatrolPath *)inObject->object_data;

	outOSD->osd.patrolpath_osd = *pp_osd;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiPatrolPath_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	UUtUns32				size, itr;
	OBJtOSD_PatrolPath		*pp_osd;

	// get a pointer to the object osd
	pp_osd = (OBJtOSD_PatrolPath *)inObject->object_data;

	size = SLcScript_MaxNameLength + sizeof(UUtUns32) + sizeof(UUtUns16) + sizeof(UUtUns16); // name, num_waypoints, id_number, flags
	for (itr = 0; itr < pp_osd->num_waypoints; itr++) {
		// waypoint type
		size += sizeof(AI2tWaypointType);

		// waypoint data
		switch(pp_osd->waypoints[itr].type) {
		case AI2cWaypointType_MoveToFlag:
			size += sizeof(UUtInt16);		// flag
			break;

		case AI2cWaypointType_Stop:
			// nothing
			break;

		case AI2cWaypointType_Pause:
			size += sizeof(UUtUns32);		// count
			break;

		case AI2cWaypointType_LookAtFlag:
			size += sizeof(UUtInt16);		// lookFlag
			break;

		case AI2cWaypointType_LookAtPoint:
			size += sizeof(M3tPoint3D);		// lookPoint
			break;

		case AI2cWaypointType_MoveAndFaceFlag:
			size += sizeof(UUtInt16);		// faceFlag
			break;

		case AI2cWaypointType_Loop:
			// nothing
			break;

		case AI2cWaypointType_MovementMode:
			size += sizeof(AI2tMovementMode);// newMode
			break;

		case AI2cWaypointType_MoveToPoint:
			size += sizeof(M3tPoint3D);		// point
			break;

		case AI2cWaypointType_LookInDirection:
			size += sizeof(AI2tMovementDirection);// facing
			break;

		case AI2cWaypointType_MoveThroughFlag:
			size += sizeof(UUtInt16);		// flag
			size += sizeof(float);			// required_distance
			break;

		case AI2cWaypointType_MoveThroughPoint:
			size += sizeof(M3tPoint3D);		// point
			size += sizeof(float);			// required_distance
			break;

		case AI2cWaypointType_StopLooking:
			// nothing
			break;

		case AI2cWaypointType_FreeFacing:
			// nothing
			break;

		case AI2cWaypointType_GlanceAtFlag:
			size += sizeof(UUtInt16);		// glanceFlag
			size += sizeof(UUtUns32);		// glanceFrames
			break;

		case AI2cWaypointType_MoveNearFlag:
			size += sizeof(UUtUns16);		// flag
			size += sizeof(float);			// required_distance
			break;

		case AI2cWaypointType_LoopToWaypoint:
			size += sizeof(UUtUns32);		// waypoint_index
			break;

		case AI2cWaypointType_Scan:
			size += sizeof(UUtInt16);		// scan_frames
			size += sizeof(float);			// angle
			break;

		case AI2cWaypointType_Guard:
			size += sizeof(UUtInt16);		// guard_frames
			size += sizeof(UUtInt16);		// flag
			size += sizeof(float);			// max_angle
			break;

		case AI2cWaypointType_StopScanning:
			// nothing
			break;

		case AI2cWaypointType_TriggerScript:
			size += sizeof(UUtInt16);		// script_number
			break;

		case AI2cWaypointType_PlayScript:
			size += sizeof(UUtInt16);		// script_number
			break;

		case AI2cWaypointType_LockIntoPatrol:
			size += sizeof(UUtBool);		// lock
			break;

		case AI2cWaypointType_ShootAtFlag:
			size += sizeof(UUtInt16);		// flag
			size += sizeof(UUtInt16);		// shoot_frames
			size += sizeof(float);			// additional_inaccuracy
			break;
		}
	}

	return size;
}

// ----------------------------------------------------------------------
static void
OBJiPatrolPath_DuplicateOSD(
	const OBJtOSD_All *inOSD,
	OBJtOSD_All *outOSD)
{
	UUtUns32				itr;
	OBJtObject				*prev_object;

	UUrMemory_MoveFast(&inOSD->osd.patrolpath_osd, &outOSD->osd.patrolpath_osd, sizeof(OBJtOSD_PatrolPath));

	// find a name that isn't a duplicate
	for (itr = 0; itr < UUcMaxUns16; itr++)
	{
		if (itr == 0) {
			// try input name
		} else if (itr == 1) {
			sprintf(outOSD->osd.patrolpath_osd.name, "%s_copy", inOSD->osd.patrolpath_osd.name);
		} else {
			sprintf(outOSD->osd.patrolpath_osd.name, "%s_copy%d", inOSD->osd.patrolpath_osd.name, itr);
		}

		prev_object = OBJrObjectType_Search(OBJcType_PatrolPath, OBJcSearch_PatrolPathName, outOSD);
		if (prev_object != NULL) { continue; }

		break;
	}
	UUmAssert(itr < UUcMaxUns16);

	// find a new ID
	for (itr = 0; itr < 1000; itr++)
	{
		outOSD->osd.patrolpath_osd.id_number = inOSD->osd.patrolpath_osd.id_number + (UUtUns16) itr;

		prev_object = OBJrObjectType_Search(OBJcType_PatrolPath, OBJcSearch_PatrolPathID, outOSD);
		if (prev_object != NULL) { continue; }

		break;
	}
	UUmAssert(itr < 1000);

	return;
}

// ----------------------------------------------------------------------
static void
OBJiPatrolPath_GetUniqueOSD(
	OBJtOSD_All *inOSD)
{
	UUtUns32				itr;
	OBJtObject				*prev_object;
	OBJtOSD_PatrolPath			*patrol_osd = &inOSD->osd.patrolpath_osd;

	UUrMemory_Clear(patrol_osd, sizeof(*patrol_osd));

	for (itr = 0; itr < UUcMaxUns32; itr++)
	{
		sprintf(patrol_osd->name, "patrol_%02d", itr);
		patrol_osd->id_number = (UUtUns16) itr;

		prev_object = OBJrObjectType_Search(OBJcType_PatrolPath, OBJcSearch_PatrolPathName, inOSD);
		if (prev_object != NULL) { continue; }

		prev_object = OBJrObjectType_Search(OBJcType_PatrolPath, OBJcSearch_PatrolPathID, inOSD);
		if (prev_object != NULL) { continue; }


		break;
	}

	patrol_osd->num_waypoints = 0;

	return;
}


// ----------------------------------------------------------------------
static UUtBool
OBJiPatrolPath_IntersectsLine(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inStartPoint,
	const M3tPoint3D		*inEndPoint)
{
	M3tBoundingSphere		sphere;

	UUrMemory_Clear(&sphere, sizeof(M3tBoundingSphere));

	// get the bounding sphere
	OBJrObject_GetBoundingSphere(inObject, &sphere);

	sphere.center.x += inObject->position.x;
	sphere.center.y += inObject->position.y;
	sphere.center.z += inObject->position.z;

	return CLrSphere_Line(inStartPoint, inEndPoint, &sphere);
}

// ----------------------------------------------------------------------
static UUtError
OBJiPatrolPath_SetDefaults(
	OBJtOSD_All			*outOSD)
{
	OBJiPatrolPath_GetUniqueOSD(outOSD);

	outOSD->osd.patrolpath_osd.flags = AI2cPatrolPathFlag_ReturnToNearest;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiPatrolPath_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_All				osd_all;
	UUtError				error;

	if (inOSD == NULL)
	{
		error = OBJiPatrolPath_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);
	}
	else
	{
		OBJiPatrolPath_DuplicateOSD(inOSD, &osd_all);
	}

	// set the object specific data and position
	error = OBJrObject_SetObjectSpecificData(inObject, &osd_all);
	UUmError_ReturnOnError(error);

	OBJrObject_UpdatePosition(inObject);

	UUrMemory_Block_VerifyList();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiPatrolPath_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_PatrolPath		*pp_osd = (OBJtOSD_PatrolPath *) inObject->object_data;
	AI2tWaypoint			*waypoint;
	UUtUns32				itr;
	UUtUns32				bytes_read = 0;

	bytes_read += OBJmGetStringFromBuffer(inBuffer, pp_osd->name,			SLcScript_MaxNameLength,	inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, pp_osd->num_waypoints,	UUtUns32,					inSwapIt);

	if (inVersion >= OBJcVersion_5) {
		bytes_read += OBJmGet2BytesFromBuffer(inBuffer, pp_osd->id_number,	UUtUns16,					inSwapIt);
	}
	else {
		pp_osd->id_number = 0;
	}

	if (inVersion >= OBJcVersion_27) {
		bytes_read += OBJmGet2BytesFromBuffer(inBuffer, pp_osd->flags,		UUtUns16,					inSwapIt);
	} else {
		pp_osd->flags = 0;
	}

	for (itr = 0, waypoint = pp_osd->waypoints;
		itr < pp_osd->num_waypoints; itr++, waypoint++) {
		// waypoint type
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->type,	AI2tWaypointType,	inSwapIt);

		switch(waypoint->type)
		{
			case AI2cWaypointType_MoveToFlag:
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.moveToFlag.flag,	UUtInt16,	inSwapIt);
			break;

			case AI2cWaypointType_Stop:
				// nothing
			break;

			case AI2cWaypointType_Pause:
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.pause.count,		UUtInt32,	inSwapIt);
			break;

			case AI2cWaypointType_LookAtFlag:
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.lookAtFlag.lookFlag,UUtInt16,	inSwapIt);
			break;

			case AI2cWaypointType_LookAtPoint:
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.lookAtPoint.lookPoint.x, float,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.lookAtPoint.lookPoint.y, float,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.lookAtPoint.lookPoint.z, float,	inSwapIt);
			break;

			case AI2cWaypointType_MoveAndFaceFlag:
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.moveAndFaceFlag.faceFlag,		UUtInt16,	inSwapIt);
			break;

			case AI2cWaypointType_Loop:
				// nothing
			break;

			case AI2cWaypointType_MovementMode:
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.movementMode.newMode,		UUtUns32,	inSwapIt);
			break;

			case AI2cWaypointType_MoveToPoint:
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.moveToPoint.point.x, float,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.moveToPoint.point.y, float,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.moveToPoint.point.z, float,	inSwapIt);
			break;

			case AI2cWaypointType_LookInDirection:
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.lookInDirection.facing,UUtUns32,	inSwapIt);
			break;

			case AI2cWaypointType_MoveThroughFlag:
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.moveThroughFlag.flag,	UUtInt16,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.moveThroughFlag.required_distance,	float,	inSwapIt);
			break;

			case AI2cWaypointType_MoveThroughPoint:
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.moveThroughPoint.point.x,	float,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.moveThroughPoint.point.y,	float,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.moveThroughPoint.point.z,	float,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.moveThroughPoint.required_distance,	float,	inSwapIt);
			break;

			case AI2cWaypointType_StopLooking:
				// nothing
			break;

			case AI2cWaypointType_FreeFacing:
				// nothing
			break;

			case AI2cWaypointType_GlanceAtFlag:
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.glanceAtFlag.glanceFlag,	UUtInt16,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.glanceAtFlag.glanceFrames,	UUtUns32,	inSwapIt);
			break;

			case AI2cWaypointType_MoveNearFlag:
				if (inVersion >= OBJcVersion_19) {
					bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.moveNearFlag.flag,	UUtInt16,	inSwapIt);
					bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.moveNearFlag.required_distance,	float,	inSwapIt);
				} else {
					waypoint->data.moveNearFlag.flag = 0;
					waypoint->data.moveNearFlag.required_distance = 0;
				}
			break;

			case AI2cWaypointType_LoopToWaypoint:
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.loopToWaypoint.waypoint_index,	UUtUns32,	inSwapIt);
			break;

			case AI2cWaypointType_Scan:
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.scan.scan_frames,	UUtInt16,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.scan.angle,			float,		inSwapIt);
			break;

			case AI2cWaypointType_StopScanning:
				// nothing
			break;

			case AI2cWaypointType_Guard:
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.guard.guard_frames,	UUtInt16,	inSwapIt);
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.guard.flag,			UUtInt16,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.guard.max_angle,		float,		inSwapIt);
			break;

			case AI2cWaypointType_TriggerScript:
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.triggerScript.script_number,		UUtInt16,	inSwapIt);
			break;

			case AI2cWaypointType_PlayScript:
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.playScript.script_number,		UUtInt16,	inSwapIt);
			break;

			case AI2cWaypointType_LockIntoPatrol:
				bytes_read += OBJmGetByteFromBuffer(inBuffer, waypoint->data.lockIntoPatrol.lock,				UUtBool,	inSwapIt);
			break;

			case AI2cWaypointType_ShootAtFlag:
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.shootAtFlag.flag,			UUtInt16,	inSwapIt);
				bytes_read += OBJmGet2BytesFromBuffer(inBuffer, waypoint->data.shootAtFlag.shoot_frames,	UUtInt16,	inSwapIt);
				bytes_read += OBJmGet4BytesFromBuffer(inBuffer, waypoint->data.shootAtFlag.additional_inaccuracy,float,	inSwapIt);
			break;
		}
	}

	// bring the object up to date
	OBJrObject_UpdatePosition(inObject);

	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtError
OBJiPatrolPath_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_PatrolPath		*pp_osd;
	UUtUns32				itr;

	UUmAssert(inOSD);

	// get a pointer to the object osd
	pp_osd = (OBJtOSD_PatrolPath *)inObject->object_data;

	// copy the data from inOSD to char_osd
	UUrString_Copy(pp_osd->name, inOSD->osd.patrolpath_osd.name, SLcScript_MaxNameLength);

	pp_osd->num_waypoints = inOSD->osd.patrolpath_osd.num_waypoints;
	pp_osd->id_number = inOSD->osd.patrolpath_osd.id_number;
	pp_osd->flags = inOSD->osd.patrolpath_osd.flags;

	for (itr = 0; itr < pp_osd->num_waypoints; itr++) {
		pp_osd->waypoints[itr] = inOSD->osd.patrolpath_osd.waypoints[itr];
	}

	UUrMemory_Block_VerifyList();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiPatrolPath_UpdatePosition(
	OBJtObject				*inObject)
{
	// nothing
}

// ----------------------------------------------------------------------
static UUtError
OBJiPatrolPath_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	OBJtOSD_PatrolPath		*pp_osd;
	AI2tWaypoint			*waypoint;
	UUtUns32				itr, bytes_available;

	pp_osd = (OBJtOSD_PatrolPath *) inObject->object_data;
	bytes_available = *ioBufferSize;

	OBJmWriteStringToBuffer(ioBuffer, pp_osd->name, SLcScript_MaxNameLength,bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, pp_osd->num_waypoints, UUtUns32, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, pp_osd->id_number, UUtUns16, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, pp_osd->flags, UUtUns16, bytes_available, OBJcWrite_Little);

	for (itr = 0, waypoint = pp_osd->waypoints; itr < pp_osd->num_waypoints; itr++, waypoint++) {
		// waypoint type
		OBJmWrite4BytesToBuffer(ioBuffer, waypoint->type,	UUtUns32,				bytes_available, OBJcWrite_Little);

		switch(waypoint->type) {
		case AI2cWaypointType_MoveToFlag:
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.moveToFlag.flag,	UUtInt16,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_Stop:
			// nothing
			break;

		case AI2cWaypointType_Pause:
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.pause.count,		UUtInt32,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_LookAtFlag:
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.lookAtFlag.lookFlag,UUtInt16,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_LookAtPoint:
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.lookAtPoint.lookPoint.x, float,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.lookAtPoint.lookPoint.y, float,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.lookAtPoint.lookPoint.z, float,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_MoveAndFaceFlag:
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.moveAndFaceFlag.faceFlag,		UUtInt16,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_Loop:
			// nothing
			break;

		case AI2cWaypointType_MovementMode:
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.movementMode.newMode,		UUtUns32,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_MoveToPoint:
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.moveToPoint.point.x,	float,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.moveToPoint.point.y,	float,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.moveToPoint.point.z,	float,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_LookInDirection:
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.lookInDirection.facing, UUtUns32,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_MoveThroughFlag:
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.moveThroughFlag.flag,	UUtInt16,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.moveThroughFlag.required_distance,	float,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_MoveThroughPoint:
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.moveThroughPoint.point.x,	float,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.moveThroughPoint.point.y,	float,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.moveThroughPoint.point.z,	float,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.moveThroughPoint.required_distance,	float,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_StopLooking:
			// nothing
			break;

		case AI2cWaypointType_FreeFacing:
			// nothing
			break;

		case AI2cWaypointType_GlanceAtFlag:
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.glanceAtFlag.glanceFlag,UUtInt16,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.glanceAtFlag.glanceFrames,UUtUns32,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_MoveNearFlag:
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.moveNearFlag.flag,					UUtInt16,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.moveNearFlag.required_distance,	float,		bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_LoopToWaypoint:
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.loopToWaypoint.waypoint_index,		UUtUns32,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_Scan:
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.scan.scan_frames,					UUtInt16,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.scan.angle,						float,		bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_StopScanning:
			// nothing
			break;

		case AI2cWaypointType_Guard:
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.guard.guard_frames,				UUtInt16,	bytes_available, OBJcWrite_Little);
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.guard.flag,						UUtInt16,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.guard.max_angle,					float,		bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_TriggerScript:
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.triggerScript.script_number,		UUtInt16,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_PlayScript:
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.playScript.script_number,			UUtInt16,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_LockIntoPatrol:
			OBJmWriteByteToBuffer(ioBuffer, waypoint->data.lockIntoPatrol.lock,					UUtUns8,	bytes_available, OBJcWrite_Little);
			break;

		case AI2cWaypointType_ShootAtFlag:
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.shootAtFlag.flag,					UUtInt16,	bytes_available, OBJcWrite_Little);
			OBJmWrite2BytesToBuffer(ioBuffer, waypoint->data.shootAtFlag.shoot_frames,			UUtInt16,	bytes_available, OBJcWrite_Little);
			OBJmWrite4BytesToBuffer(ioBuffer, waypoint->data.shootAtFlag.additional_inaccuracy,	float,		bytes_available, OBJcWrite_Little);
			break;
		}
	}

	// set ioBufferSize to the number of bytes written to the buffer
	*ioBufferSize = *ioBufferSize - bytes_available;

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OBJiPatrolPath_GetVisible(
	void)
{
	return OBJgPatrolPath_DrawPositions;
}

// ----------------------------------------------------------------------
static void
OBJiPatrolPath_SetVisible(
	UUtBool					inIsVisible)
{
	OBJgPatrolPath_DrawPositions = inIsVisible;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiPatrolPath_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	OBJtOSD_PatrolPath		*pp_osd;
	UUtBool					found;

	// get a pointer to the object osd
	pp_osd = (OBJtOSD_PatrolPath *)inObject->object_data;

	// perform the check
	switch (inSearchType)
	{
		case OBJcSearch_PatrolPathName:
			found = strcmp(pp_osd->name, inSearchParams->osd.patrolpath_osd.name) == 0;
		break;

		case OBJcSearch_PatrolPathID:
			found = pp_osd->id_number == inSearchParams->osd.patrolpath_osd.id_number;
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
OBJrPatrolPath_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;

	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));

	// set up the methods structure
	methods.rNew				= OBJiPatrolPath_New;
	methods.rSetDefaults		= OBJiPatrolPath_SetDefaults;
	methods.rDelete				= OBJiPatrolPath_Delete;
	methods.rDraw				= OBJiPatrolPath_Draw;
	methods.rEnumerate			= OBJiPatrolPath_Enumerate;
	methods.rGetBoundingSphere	= OBJiPatrolPath_GetBoundingSphere;
	methods.rOSDGetName			= OBJiPatrolPath_OSDGetName;
	methods.rOSDSetName			= OBJiPatrolPath_OSDSetName;
	methods.rIntersectsLine		= OBJiPatrolPath_IntersectsLine;
	methods.rUpdatePosition		= OBJiPatrolPath_UpdatePosition;
	methods.rGetOSD				= OBJiPatrolPath_GetOSD;
	methods.rGetOSDWriteSize	= OBJiPatrolPath_GetOSDWriteSize;
	methods.rSetOSD				= OBJiPatrolPath_SetOSD;
	methods.rRead				= OBJiPatrolPath_Read;
	methods.rWrite				= OBJiPatrolPath_Write;
	methods.rGetClassVisible	= OBJiPatrolPath_GetVisible;
	methods.rSearch				= OBJiPatrolPath_Search;
	methods.rSetClassVisible	= OBJiPatrolPath_SetVisible;
	methods.rGetUniqueOSD		= OBJiPatrolPath_GetUniqueOSD;

	// register the furniture methods
	error =
		OBJrObjectGroup_Register(
			OBJcType_PatrolPath,
			OBJcTypeIndex_PatrolPath,
			"Patrol Path",
			sizeof(OBJtOSD_PatrolPath),
			&methods,
			OBJcObjectGroupFlag_CanSetName | OBJcObjectGroupFlag_HasUniqueName);
	UUmError_ReturnOnError(error);

#if CONSOLE_DEBUGGING_COMMANDS
	error =
		SLrGlobalVariable_Register_Bool(
			"show_patrolpaths",
			"Enables the display of AI patrol path objects",
			&OBJgPatrolPath_DrawPositions);
	UUmError_ReturnOnError(error);

	error =
		SLrGlobalVariable_Register_Float(
			"patrolpath_name_distance",
			"Specifies the distance from the camera that patrol point names no longer draw.",
			&OBJgPatrolPath_DrawNameDistance);
	UUmError_ReturnOnError(error);
#endif

	// intialize the globals
	OBJgPatrolPath_DrawNameDistance = OBJcPatrolPath_DefaultDrawNameDistance;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrPatrolPath_ReadPathData(
	OBJtObject *inObject,
	OBJtOSD_PatrolPath *ioPath)
{
	OBJtOSD_All osd_all;

	if ((inObject == NULL) || (inObject->object_type != OBJcType_PatrolPath))
		return UUcError_Generic;

	UUmAssertWritePtr(ioPath, sizeof(OBJtOSD_PatrolPath));

	OBJrObject_GetObjectSpecificData(inObject, &osd_all);
	UUrMemory_MoveFast(&osd_all.osd.patrolpath_osd, ioPath, sizeof(*ioPath));

	return UUcError_None;
}

UUtError
OBJrPatrolPath_WritePathData(
	OBJtObject *ioObject,
	OBJtOSD_PatrolPath *inPath)
{
	if ((ioObject == NULL) || (ioObject->object_type != OBJcType_PatrolPath))
		return UUcError_Generic;

	UUmAssertReadPtr(inPath, sizeof(OBJtOSD_PatrolPath));

	OBJrObject_SetObjectSpecificData(ioObject, (OBJtOSD_All *) inPath);

	return UUcError_None;
}

