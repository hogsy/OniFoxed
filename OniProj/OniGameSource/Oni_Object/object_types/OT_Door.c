// ======================================================================
// OT_Door.c - The Doors
// ======================================================================

// ======================================================================
// includes
// ======================================================================

#include "BFW.h"
#include "EulerAngles.h"
#include "Oni_Object.h"
#include "Oni_Object_Private.h"
#include "Oni_BinaryData.h"		
#include "Oni_GameStatePrivate.h"
#include "BFW_TM_Construction.h"
#include "Oni_Mechanics.h"
#include "Oni_Level.h"
#include "Oni_Sound2.h"

// ======================================================================
// externals
// ======================================================================

// ======================================================================
// globals
// ======================================================================

UUtBool							OBJgDoor_DrawFrames;
static UUtBool					OBJgDoor_DrawDoor;
static UUtBool					OBJgDoor_IgnoreLocks;
static UUtBool					OBJgDoor_ShowDebug;
static UUtBool					OBJgDoor_PopLighting = UUcFalse;

static UUtBool					OBJgDoor_ShowActivationCircles;

#if TOOL_VERSION
// text name drawing
static UUtBool					OBJgDoor_DrawInitialized = UUcFalse;
static UUtRect					OBJgDoor_TextureBounds;
static TStTextContext			*OBJgDoor_TextContext;
static M3tTextureMap			*OBJgDoor_Texture;
static IMtPoint2D				OBJgDoor_Dest;
static IMtPixel					OBJgDoor_WhiteColor;
static UUtInt16					OBJgDoor_TextureWidth;
static UUtInt16					OBJgDoor_TextureHeight;
static float					OBJgDoor_WidthRatio;
#endif

static IMtShade					OBJgDoor_Shade;
static UUtUns32					OBJgDoor_CurrentDoor;

// ======================================================================
// prototypes
// ======================================================================
	
#define OBcDoorFaceAngle			1.4f // 80 degrees
#define OBcDoorOpenUpwardsHeight	15.0f

#define OBcDoorTotallyStuckFrames	420
#define OBcDoorReverseFrames		90
#define OBcDoor_CollisionProxDisable 180

UUtError OBJtDoor_Reset( OBJtObject *inObject );
void OBJrDoor_Open( OBJtObject *inObject, ONtCharacter* inCharacter );
static void OBJrDoor_Close( OBJtObject *inObject );
static void OBJiDoor_UpdateAnimation( OBJtObject *inObject );
static void OBJiDoor_GetBoundingSphere( const OBJtObject *inObject, M3tBoundingSphere *outBoundingSphere );
static UUtError OBJiDoor_SetOSD( OBJtObject *inObject, const OBJtOSD_All *inOSD);
static void OBJiDoor_UpdatePosition( OBJtObject *inObject);
static void CreateDoorObjects(OBJtObject *inDoor);
static void DeleteDoorObjects(OBJtObject *inDoor);
static void OBJiDoor_DumpDebugLine(char *inMsg, BFtFile *inFile);
static void OBJiDoor_DumpDebugInfo(OBJtObject *inDoor, BFtFile *inFile);

// ======================================================================
// functions
// ======================================================================
static float OBJiDoor_GetGeometryWidth( OBJtObject *inObject )
{
	OBJtOSD_Door			*door_osd;
	UUtUns32				i;
	float					max_x;
	float					min_x;

	door_osd = (OBJtOSD_Door*)inObject->object_data;

	if (door_osd->door_class == NULL) {
		return 0;
	}

	max_x	= -100000;
	min_x	= 100000;

	for (i = 0; i < door_osd->door_class->geometry_array[0]->num_furn_geoms; i++)
	{
		max_x = UUmMax( door_osd->door_class->geometry_array[0]->furn_geom[0].geometry->pointArray->minmax_boundingBox.maxPoint.x, max_x );
		min_x = UUmMin( door_osd->door_class->geometry_array[0]->furn_geom[0].geometry->pointArray->minmax_boundingBox.minPoint.x, min_x );
	}

	return UUmMax(0, max_x - min_x);
}

static void OBJiDoor_CreateBoundingSphere( OBJtObject *inObject, OBJtOSD_Door *inOSD)
{
	UUtUns32				i;
	M3tBoundingSphere		*sphere;
	M3tPoint3D				new_center;
	float					new_radius;
	OBJtOSD_Door			*door_osd;
	
	new_center.x = 0.0f;
	new_center.y = 0.0f;
	new_center.z = 0.0f;

	door_osd = (OBJtOSD_Door*)inObject->object_data;

	if( !door_osd->door_class && !door_osd->door_frame )
		return;

	UUmAssert( door_osd->door_class || door_osd->door_frame );		// if we dont have a door class, we better have a door frame

	if( door_osd->door_class )
	{
		// calculate the center of the sphere
		for (i = 0; i < door_osd->door_class->geometry_array[0]->num_furn_geoms; i++)
		{
			sphere = &door_osd->door_class->geometry_array[0]->furn_geom[i].geometry->pointArray->boundingSphere;
			MUmVector_Increment(new_center, sphere->center);
		}
	
		new_center.x /= door_osd->door_class->geometry_array[0]->num_furn_geoms;
		new_center.y /= door_osd->door_class->geometry_array[0]->num_furn_geoms;
		new_center.z /= door_osd->door_class->geometry_array[0]->num_furn_geoms;
		
		// caculate the new radius
		new_radius = 0.0f;
		for (i = 0; i < door_osd->door_class->geometry_array[0]->num_furn_geoms; i++)
		{
			M3tVector3D			vector;			
			float				temp_radius;
			sphere = &door_osd->door_class->geometry_array[0]->furn_geom[i].geometry->pointArray->boundingSphere;
			MUmVector_Subtract(vector, new_center, sphere->center);
			temp_radius = MUrVector_Length(&vector) + sphere->radius;						
			new_radius = UUmMax(new_radius, temp_radius);
		}
		if( door_osd->flags & OBJcDoorFlag_DoubleDoors )
		{
			new_radius *= 2.0;
		}
	}
	else
	{
		UUmAssert( door_osd->door_frame );
		new_radius		= UUmMax( door_osd->door_frame->width,  door_osd->door_frame->height ) / 2;
	}

	new_center.x		 += inOSD->center_offset.x;
	new_center.y		 += inOSD->center_offset.y;
	new_center.z		 += inOSD->center_offset.z;
	// set the bounding sphere
	inOSD->bounding_sphere.center = new_center;
	inOSD->bounding_sphere.radius = new_radius;
}
// ----------------------------------------------------------------------
static OBJtObject* OBJrDoor_FindDoorWithFrame( AKtDoorFrame* inDoorFrame )
{
	OBJtObject			*object;
	OBJtOSD_Door		*door_osd;
	UUtUns32			i;
	UUtUns32			object_count;
	OBJtObject			**object_list;

	OBJrObjectType_GetObjectList( OBJcType_Door, &object_list, &object_count );

	for( i = 0; i < object_count; i++ )
	{
		object		= object_list[i];
		door_osd	= (OBJtOSD_Door*)object->object_data;
		if( door_osd->door_frame == inDoorFrame )
			return object;
	}
	return NULL;
}

OBJtObject* OBJrDoor_FindDoorWithID( UUtUns16 inID )
{
	UUtUns32			i;
	OBJtObject			*object;
	OBJtOSD_Door		*door_osd;
	UUtUns32			object_count;
	OBJtObject			**object_list;

	OBJrObjectType_GetObjectList( OBJcType_Door, &object_list, &object_count );

	for( i = 0; i < object_count; i++ )
	{
		object		= object_list[i];
		door_osd = (OBJtOSD_Door*)object->object_data;
		if( door_osd->id == inID )
			return object;
	}
	return NULL;
}

static OBJtObject* OBJrDoor_FindDoorAt( M3tPoint3D *position )
{
	OBJtObject			*object;
	UUtUns32			i;
	UUtUns32			object_count;
	OBJtObject			**object_list;

	OBJrObjectType_GetObjectList( OBJcType_Door, &object_list, &object_count );

	for( i = 0; i < object_count; i++ )
	{
		object		= object_list[i];
		if( object->position.x == position->x && object->position.y == position->y && object->position.z == position->z )
			return object;
	}
	return NULL;
}
// ----------------------------------------------------------------------
static AKtDoorFrame* OBJrDoor_FindDoorFrameAt( M3tPoint3D *position )
{
	AKtDoorFrame*	door_frame;
	UUtUns32		i;
	UUtUns32		door_count;
	
	door_count = ONgLevel->environment->door_frames->door_count;

	for( i = 0; i < door_count; i++ )
	{
		door_frame = &ONgLevel->environment->door_frames->door_frames[i];
		if( door_frame->position.x == position->x && door_frame->position.y == position->y && door_frame->position.z == position->z )
			return door_frame;
	}
	return NULL;
}

// ----------------------------------------------------------------------
// search for a door by ID
OBJtObject *OBJrDoor_GetByID(UUtUns16 inID)
{
	OBJtOSD_All search_params;

	search_params.osd.door_osd.id = inID;
	return OBJrObjectType_Search(OBJcType_Door, OBJcSearch_DoorID, &search_params);
}


// ----------------------------------------------------------------------
static void OBJiDoor_Delete( OBJtObject *inObject)
{
	OBJtOSD_Door			*door_osd;
	UUmAssert( inObject && inObject->object_data );
	door_osd = (OBJtOSD_Door*)inObject->object_data;

	if (door_osd->pathfinding_connection[0] != NULL) {
		door_osd->pathfinding_connection[0]->door_link = NULL;
		door_osd->pathfinding_connection[0] = NULL;
	}

	if (door_osd->pathfinding_connection[1] != NULL) {
		door_osd->pathfinding_connection[1]->door_link = NULL;
		door_osd->pathfinding_connection[1] = NULL;
	}

	return;
}

static UUtUns16 OBJrDoor_GetID( OBJtObject *inObject )
{
	OBJtOSD_Door			*door_osd;
	UUmAssert( inObject && inObject->object_data );
	door_osd = (OBJtOSD_Door*)inObject->object_data;
	return door_osd->id;
}

// ----------------------------------------------------------------------
// locking / unlocking
// ----------------------------------------------------------------------

static UUtBool OBJiDoor_LockDoors_Enum( OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Door *osd;

	UUmAssert( inObject->object_type == OBJcType_Door );

	osd = (OBJtOSD_Door*) inObject->object_data;

	if ((osd->id == inUserData) || (inUserData == 0xffff)) {
		if (OBJgDoor_ShowDebug) {
			COrConsole_Printf("locking door, class = %s was locked = %s", osd->door_class_name, (osd->flags & OBJcDoorFlag_Locked) ? "LOCKED" : "UNLOCKED");
		}

		if ((osd->flags & OBJcDoorFlag_Locked) == 0) {
			ONrGameState_EventSound_Play(ONcEventSound_Door_Lock, &inObject->position);
			osd->flags |= OBJcDoorFlag_Locked;
		}
	}

	return UUcTrue;
}


static UUtBool OBJiDoor_UnlockDoors_Enum( OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Door *osd;

	UUmAssert( inObject->object_type == OBJcType_Door );

	osd = (OBJtOSD_Door*) inObject->object_data;

	if ((osd->id == inUserData) || (inUserData == 0xffff)) {
		if (OBJgDoor_ShowDebug) {
			COrConsole_Printf("unlocking door, class = %s old state = %s", osd->door_class_name, (osd->flags & OBJcDoorFlag_Locked) ? "LOCKED" : "UNLOCKED");
		}

		if (osd->flags & OBJcDoorFlag_Locked) {
			ONrGameState_EventSound_Play(ONcEventSound_Door_Unlock, &inObject->position);
			osd->flags &= ~OBJcDoorFlag_Locked;
		}
	}

	return UUcTrue;
}

UUtError OBJrDoor_Lock_ID( UUtUns16 inID )
{
	if (inID == (UUtUns16) -1) {
		inID = 0xffff;
	}

	if (OBJgDoor_ShowDebug) {
		COrConsole_Printf("locking doors of id %d", inID);
	}

	OBJrObjectType_EnumerateObjects( OBJcType_Door, OBJiDoor_LockDoors_Enum, (UUtUns32) inID ); 

	return UUcError_None;
}

UUtError OBJrDoor_Unlock_ID( UUtUns16 inID )
{
	if (inID == (UUtUns16) -1) {
		inID = 0xffff;
	}

	if (OBJgDoor_ShowDebug) {
		COrConsole_Printf("unlocking doors of id %d", inID);
	}

	OBJrObjectType_EnumerateObjects( OBJcType_Door, OBJiDoor_UnlockDoors_Enum, (UUtUns32) inID ); 

	return UUcError_None;
}

void OBJrDoor_ForceOpen(UUtUns16 inID)
{
	OBJtObject *door = OBJrDoor_GetByID(inID);

	if (door != NULL) {
		OBJrDoor_Open(door, NULL);
	}
}

void OBJrDoor_ForceClose(UUtUns16 inID)
{
	OBJtObject *door = OBJrDoor_GetByID(inID);

	if (door != NULL) {
		OBJrDoor_Close(door);
	}
}

// ----------------------------------------------------------------------
// AI pathfinding interface
// ----------------------------------------------------------------------

typedef struct OBJtDoor_MakeConnectionUserData {
	PHtConnection	*connection;
	float			connection_radius_sq;
	M3tPoint3D		*side_test_point;
} OBJtDoor_MakeConnectionUserData;

static UUtBool OBJiDoor_MakeConnectionLink_Enum( OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Door *osd = (OBJtOSD_Door *) inObject->object_data;
	OBJtDoor_MakeConnectionUserData *user_data = (OBJtDoor_MakeConnectionUserData *) inUserData;
	UUtBool exclude0 = UUcFalse, exclude1 = UUcFalse;
	float dist_sq;
	UUtUns32 index;

	UUmAssert(inObject->object_type == OBJcType_Door);
	UUmAssertReadPtr(user_data, sizeof(*user_data));
	UUmAssertReadPtr(osd, sizeof(OBJtOSD_Door));

	// CB: determine whether the door is close enough to be considered for this connection.
	// note that inObject->position is the base of the door and so can be compared to the
	// input point which is the midpoint of the base of the ghost quad.
	dist_sq = MUmVector_GetDistanceSquared(inObject->position, user_data->connection->connection_midpoint);
	if (dist_sq > UUmSQR(user_data->connection->connection_width / 2.0f)) {
		// outside max range, keep looking
		return UUcTrue;
	}

	// exclude replacing 0 or 1 if we are the same direction as the other connection... this is so that
	// we don't get linked to two connections that go in the same direction
	if ((osd->pathfinding_connection[1] != NULL) &&
		((user_data->connection->from == osd->pathfinding_connection[1]->from) && (user_data->connection->to == osd->pathfinding_connection[1]->to))) {
		exclude0 = UUcTrue;
	}
	if ((osd->pathfinding_connection[0] != NULL) &&
		((user_data->connection->from == osd->pathfinding_connection[0]->from) && (user_data->connection->to == osd->pathfinding_connection[0]->to))) {
		exclude1 = UUcTrue;
	}

	if (!exclude0 && (osd->pathfinding_connection[0] == NULL)) {
		// index 0 is free
		index = 0;

	} else if (!exclude1 && (osd->pathfinding_connection[1] == NULL)) {
		// index 1 is free
		index = 1;

	} else if (!exclude0 && (osd->pathfinding_conndistsq[0] > 1.1f * dist_sq)) {
		// this is a closer connection than index 0, replace it
		index = 0;

	} else if (!exclude1 && (osd->pathfinding_conndistsq[1] > 1.1f * dist_sq)) {
		// this is a closer connection than index 1, replace it
		index = 1;

	} else if (!exclude0 && ((osd->pathfinding_conndistsq[0] > 0.9f * dist_sq) &&
				(osd->pathfinding_connwidth[0] > user_data->connection->connection_width))) {
		// this is roughly the same dist as index 0 and is a narrower connection, replace it
		index = 0;

	} else if (!exclude1 && ((osd->pathfinding_conndistsq[1] > 0.9f * dist_sq) &&
				(osd->pathfinding_connwidth[1] > user_data->connection->connection_width))) {
		// this is roughly the same dist as index 1 and is a narrower connection, replace it
		index = 1;

	} else if (!exclude0 && ((osd->pathfinding_conndistsq[0] > 0.9f * dist_sq) &&
				(osd->pathfinding_connwidth[0] > 0.9f * user_data->connection->connection_width) &&
				(osd->pathfinding_connection[1] != NULL) &&
				(user_data->connection->from == osd->pathfinding_connection[1]->to) &&
				(user_data->connection->to == osd->pathfinding_connection[1]->from))) {
		// this is roughly the same dist and width as index 0 and is the dual of connection 1, replace it
		index = 0;

	} else if (!exclude1 && ((osd->pathfinding_conndistsq[1] > 0.9f * dist_sq) &&
				(osd->pathfinding_connwidth[1] > 0.9f * user_data->connection->connection_width) &&
				(osd->pathfinding_connection[0] != NULL) &&
				(user_data->connection->from == osd->pathfinding_connection[0]->to) &&
				(user_data->connection->to == osd->pathfinding_connection[0]->from))) {
		// this is roughly the same dist and width as index 0 and is the dual of connection 1, replace it
		index = 1;

	} else {
		// this door is already linked to two connections that are more desirable, don't link to this connection

#if 0
		{
			char buffer[256], temp[128];

			// TEMPORARY DEBUGGING PRINT
			sprintf(buffer, "OBJrDoor_MakeConnectionLink: door %d rejecting connection %d -> %d (d%f w%f)", osd->id,
					user_data->connection->from->location->index, user_data->connection->to->location->index, dist_sq, user_data->connection->connection_width);

			if (osd->pathfinding_connection[0] != NULL) {
				sprintf(temp, ": already had %d -> %d (d%f w%f)",
					osd->pathfinding_connection[0]->from->location->index, osd->pathfinding_connection[0]->to->location->index, osd->pathfinding_conndistsq[0], osd->pathfinding_connwidth[0]);
				strcat(buffer, temp);
			}
			if (osd->pathfinding_connection[1] != NULL) {
				sprintf(temp, " and %d -> %d (d%f w%f)",
					osd->pathfinding_connection[1]->from->location->index, osd->pathfinding_connection[1]->to->location->index, osd->pathfinding_conndistsq[1], osd->pathfinding_connwidth[1]);
				strcat(buffer, temp);
			}
			UUrDebuggerMessage("%s\n", buffer);
		}
#endif
		return UUcTrue;
	}

	if (user_data->connection->door_link != NULL) {
		UUrDebuggerMessage("OBJrDoor_MakeConnectionLink: door %d tried linking to a connection that was already linked to door %d\n",
			osd->id, ((OBJtOSD_Door *) user_data->connection->door_link->object_data)->id);
		return UUcFalse;
	}

	user_data->connection->door_link = inObject;
	user_data->connection->door_side = (UUtBool) OBJrDoor_PointOnSide(user_data->side_test_point, inObject);
	osd->pathfinding_connection[index] = user_data->connection;
	osd->pathfinding_conndistsq[index] = dist_sq;
	osd->pathfinding_connwidth [index] = user_data->connection->connection_width;

	// stop enumerating, we've stored a link
	return UUcFalse;
}

void OBJrDoor_MakeConnectionLink(struct PHtConnection *ioConnection, M3tPoint3D *inSidePoint)
{
	OBJtDoor_MakeConnectionUserData user_data;

	user_data.connection = ioConnection;
	user_data.side_test_point = inSidePoint;

	OBJrObjectType_EnumerateObjects(OBJcType_Door, OBJiDoor_MakeConnectionLink_Enum, (UUtUns32) &user_data); 
}

// ----------------------------------------------------------------------
// drawing
// ----------------------------------------------------------------------

#if TOOL_VERSION
static void OBJiDoor_DrawName( OBJtObject *inObject, M3tPoint3D *inLocation )
{
	UUtError				error;

	error = OBJrDoor_DrawInitialize();
	if (error == UUcError_None) {
		char					name[OBJcMaxNameLength + 1];

		// erase the texture and set the text contexts shade
		M3rTextureMap_Fill( OBJgDoor_Texture, OBJgDoor_WhiteColor, NULL);

		TSrContext_SetShade(OBJgDoor_TextContext, IMcShade_Black);
		
		// get the flag's title
		OBJrObject_GetName(inObject, name, OBJcMaxNameLength);
		
		// draw the string to the texture
		TSrContext_DrawString( OBJgDoor_TextContext, OBJgDoor_Texture, name, &OBJgDoor_TextureBounds, &OBJgDoor_Dest);
		
		// draw the sprite
		M3rSimpleSprite_Draw( OBJgDoor_Texture, inLocation, OBJgDoor_WidthRatio, 1.0f, IMcShade_White, M3cMaxAlpha );
	}
	
	return;
}

static void OBJiDoor_Draw_Frame( OBJtObject *inObject, UUtUns32 inDrawFlags)
{
	OBJtOSD_Door			*door_osd;
	IMtShade				shade;
	M3tPoint3D				camera_location;
	AKtDoorFrame			*frame;
	M3tPoint3D				center_point = { 0,0,0 };

	M3tPoint3D points[4] = 
	{
		{ 0.0f,  0.0f,  0.0f },
		{ 0.0f,  0.0f,  0.0f },
		{ 0.0f,  0.0f,  0.0f },
		{ 0.0f,  0.0f,  0.0f }
	};

	door_osd = (OBJtOSD_Door*)inObject->object_data;

	center_point	= door_osd->center_offset;
	
	frame = door_osd->door_frame;

	UUmAssert( frame );

	if( !frame )		return;

	if (inDrawFlags & OBJcDrawFlag_Selected)
	{
		door_osd->general_quad->flags |= AKcGQ_Flag_Invisible;
	}
	else
	{
		door_osd->general_quad->flags &= ~AKcGQ_Flag_Invisible;
	}

	M3rMatrixStack_Push();		
	M3rMatrixStack_ApplyTranslate(inObject->position);
	M3rMatrixStack_Multiply(&door_osd->rotation_matrix);

	if (inDrawFlags & OBJcDrawFlag_Selected)
		shade	= IMcShade_Red;
	else
		shade	= IMcShade_White;

	points[0].x = -frame->width  / 2.0f;
	points[0].y =  frame->height;

	points[1].x =  frame->width  / 2.0f;
	points[1].y =  frame->height;

	points[2].x =  frame->width  / 2.0f;
	points[2].y =  0.0;

	points[3].x = -frame->width  / 2.0f;
	points[3].y =  0.0;

	M3rGeom_Line_Light(&points[0], &points[1], shade);
	M3rGeom_Line_Light(&points[1], &points[2], shade);
	M3rGeom_Line_Light(&points[2], &points[3], shade);
	M3rGeom_Line_Light(&points[3], &points[0], shade);

	M3rGeom_Line_Light(&points[0], &points[2], shade);
	M3rGeom_Line_Light(&points[1], &points[3], shade);
	
	camera_location = CArGetLocation();
	if( MUrPoint_Distance( &frame->position, &camera_location ) < 300 )
	{
		OBJiDoor_DrawName( inObject, &door_osd->center_offset );
	}


	M3rMatrixStack_Pop();
}
#endif

static void _OBJrFurniture_DrawArray( OBJtFurnGeomArray* inGeometryArray, M3tMatrix4x3 *inMatrix, UUtUns32 inDrawFlags, M3tTextureMap *inTexture )
{
	UUtUns32				flags;
	M3tBoundingBox			bBox;
	UUtUns32				i;
	M3tTextureMap			*old_texture;

	if (NULL != inMatrix) {
		M3rMatrixStack_Push();
		M3rMatrixStack_Multiply(inMatrix);
	}

	for (i = 0; i < inGeometryArray->num_furn_geoms; i++)
	{
		// check the visibility flags
		flags = inGeometryArray->furn_geom[i].gq_flags;
		if (((flags & AKcGQ_Flag_Invisible) != 0) && (AKgDraw_Invis == UUcFalse))	continue;
		if (((flags & AKcGQ_Flag_No_Occlusion) != 0) &&	(AKgDraw_Occl == UUcTrue))	continue;

		// draw the bounding box if this is the selected object
		if (inDrawFlags & OBJcDrawFlag_Selected)
		{
			M3rMinMaxBBox_To_BBox( &inGeometryArray->furn_geom[i].geometry->pointArray->minmax_boundingBox, &bBox);
			
			M3rBBox_Draw_Line(&bBox, IMcShade_White);
		}
		// draw the geometry
		if( inTexture )
		{
			old_texture = inGeometryArray->furn_geom[i].geometry->baseMap;
			inGeometryArray->furn_geom[i].geometry->baseMap	= inTexture;
			M3rGeometry_Draw( inGeometryArray->furn_geom[i].geometry );
			inGeometryArray->furn_geom[i].geometry->baseMap	= old_texture;
		}
		else
		{
			M3rGeometry_Draw( inGeometryArray->furn_geom[i].geometry );
		}
	}

	if (NULL != inMatrix) {
		M3rMatrixStack_Pop();
	}

	return;
}

static void OBJiDoor_Draw( OBJtObject *inObject, UUtUns32 inDrawFlags)
{
#if TOOL_VERSION
	OBJtOSD_Door			*door_osd;

	M3tPoint3D				center_point = { 0,0.5,0 };

	M3tMatrix4x3			m1;

	UUtUns32				i;

	M3tBoundingBox			bBox;

	OBJtFurnGeomArray		*geometry_array;

	M3tPoint3D points[4] = 
	{
		{ 0.0f,  0.0f,  0.0f },
		{ 0.0f,  0.0f,  0.0f },
		{ 0.0f,  0.0f,  0.0f },
		{ 0.0f,  0.0f,  0.0f }
	};

	if( !OBJgDoor_DrawDoor )						return;	

	door_osd = (OBJtOSD_Door*)inObject->object_data;

	if (!door_osd->door_class && !door_osd->door_frame)
		return;

	M3rGeom_State_Commit();

	if( !door_osd->door_class )
	{
		if (OBJgDoor_DrawFrames) {
			OBJiDoor_Draw_Frame( inObject, inDrawFlags );
		}
	}
	else if( !(inObject->flags & OBJcObjectFlag_Gunk) || OBJgDoor_ShowDebug )
	{
		M3tMatrix4x3 matrix_stack[2];

		UUmAssert(door_osd->door_class->geometry_array[0]);
		
		if (door_osd->door_class->geometry_array[0] == NULL)	return;
		
		{
			MUrMatrix_BuildTranslate(inObject->position.x, inObject->position.y, inObject->position.z, matrix_stack + 0);
			MUrMatrixStack_Matrix(matrix_stack + 0, &door_osd->rotation_matrix);
		
			MUrMatrix_BuildRotateX(M3c2Pi -M3cHalfPi,&m1);
			
			// door 1
			{
				matrix_stack[1] = matrix_stack[0];
				MUrMatrixStack_Matrix(matrix_stack + 1, &door_osd->door_matrix[0]);	
				MUrMatrixStack_Matrix(matrix_stack + 1, &m1);
				MUrMatrixStack_Matrix(matrix_stack + 1, &door_osd->animation_matrix);				

				_OBJrFurniture_DrawArray(door_osd->door_class->geometry_array[0], matrix_stack + 1, inDrawFlags, door_osd->door_texture_ptr[0] );
			}

			// door 2
			if( door_osd->flags & OBJcDoorFlag_DoubleDoors )
			{
				matrix_stack[1] = matrix_stack[0];

				MUrMatrixStack_Matrix(matrix_stack + 1, &door_osd->door_matrix[1]);
				MUrMatrixStack_Matrix(matrix_stack + 1, &m1);

				if( door_osd->flags & OBJcDoorFlag_TestMode )
				{				
					MUrMatrix_BuildRotateY( M3cPi, &m1 );
					MUrMatrix_RotateZ( &m1, M3cPi );
					MUrMatrixStack_Matrix(matrix_stack + 1, &m1);

					// animate
					MUrMatrixStack_Matrix(matrix_stack + 1, &door_osd->animation_matrix);

					// convert from max
					MUrMatrix_BuildRotateX( M3cPi, &m1 );
					MUrMatrixStack_Matrix(matrix_stack + 1, &m1);
				}

				if( door_osd->door_class->geometry_array[1] ) {
					_OBJrFurniture_DrawArray( door_osd->door_class->geometry_array[1], matrix_stack + 1, inDrawFlags, door_osd->door_texture_ptr[1] );
				}
				else {
					_OBJrFurniture_DrawArray( door_osd->door_class->geometry_array[0], matrix_stack + 1, inDrawFlags, door_osd->door_texture_ptr[1] );
				}
			}
		}
	}

	// draw the rotation rings if this is the selected object
	if (inDrawFlags & OBJcDrawFlag_Selected)
	{
		M3rMatrixStack_Push();		
		{
			MUrMatrix_BuildRotateX(M3c2Pi-M3cHalfPi,&m1);
			M3rMatrixStack_ApplyTranslate(inObject->position);
			M3rMatrixStack_Multiply(&door_osd->rotation_matrix);
			OBJrObjectUtil_DrawRotationRings(inObject, &door_osd->bounding_sphere, inDrawFlags);

			M3rDisplay_Circle( &center_point, MUrSqrt(door_osd->activation_radius_squared ), IMcShade_Green );

			if( door_osd->door_class )
			{
				M3rMatrixStack_Push();		
				M3rMatrixStack_Multiply(&door_osd->door_matrix[0]);
				M3rMatrixStack_Multiply(&m1);
				for (i = 0; i < door_osd->door_class->geometry_array[0]->num_furn_geoms; i++)
				{
					M3rMinMaxBBox_To_BBox( &door_osd->door_class->geometry_array[0]->furn_geom[i].geometry->pointArray->minmax_boundingBox, &bBox);
				
					M3rBBox_Draw_Line(&bBox, IMcShade_White);
				}				
				M3rMatrixStack_Pop();		
				if( door_osd->flags & OBJcDoorFlag_DoubleDoors )
				{
					geometry_array = door_osd->door_class->geometry_array[1] ? door_osd->door_class->geometry_array[1] : door_osd->door_class->geometry_array[0];
					M3rMatrixStack_Push();		
					M3rMatrixStack_Multiply(&door_osd->door_matrix[1]);
					M3rMatrixStack_Multiply(&m1);
					for (i = 0; i < geometry_array->num_furn_geoms; i++)
					{
						M3rMinMaxBBox_To_BBox( &geometry_array->furn_geom[i].geometry->pointArray->minmax_boundingBox, &bBox);
					
						M3rBBox_Draw_Line(&bBox, IMcShade_White);
					}
					M3rMatrixStack_Pop();		
				}				
			}
			else if( door_osd->door_frame )
			{
			}
		}
		M3rMatrixStack_Pop();
	}
	else if( OBJgDoor_ShowActivationCircles )
	{
		M3tPoint3D				circle_center = { 0,0.5,0 };
		M3rMatrixStack_Push();		
			M3rMatrixStack_ApplyTranslate(inObject->position);
			M3rMatrixStack_Multiply(&door_osd->rotation_matrix);
			M3rGeom_State_Commit();
			M3rDisplay_Circle( &circle_center, MUrSqrt(door_osd->activation_radius_squared ), IMcShade_Green );
		M3rMatrixStack_Pop();
	}
#endif

	return;
}
// ----------------------------------------------------------------------
static UUtError OBJiDoor_Enumerate( OBJtObject *inObject, OBJtEnumCallback_ObjectName		inEnumCallback, UUtUns32 inUserData)
{
	return OBJrObjectUtil_EnumerateTemplate( "", OBJcTemplate_DoorClass, inEnumCallback, inUserData);
}
// ----------------------------------------------------------------------
static void OBJiDoor_GetBoundingSphere( const OBJtObject *inObject, M3tBoundingSphere *outBoundingSphere )
{
	OBJtOSD_Door		*door_osd;
	
	door_osd = (OBJtOSD_Door*)inObject->object_data;

	*outBoundingSphere = door_osd->bounding_sphere;
}

// ----------------------------------------------------------------------
static void OBJiDoor_OSDGetName( const OBJtOSD_All *inObject, char *outName, UUtUns32 inNameLength )
{
	const OBJtOSD_Door		*door_osd = &inObject->osd.door_osd;
	char				name[100];

	if( door_osd->door_class )
		sprintf( name, "door_%02d", door_osd->id );	
	else
		sprintf( name, "door_frame_%02d", door_osd->id );	

	UUrString_Copy(outName, name, inNameLength);	

}
// ----------------------------------------------------------------------
static void OBJiDoor_GetOSD( const OBJtObject *inObject, OBJtOSD_All *outOSD)
{
	OBJtOSD_Door		*door_osd;
	
	door_osd = (OBJtOSD_Door*)inObject->object_data;
	
	UUrMemory_MoveFast( door_osd, &outOSD->osd.door_osd, sizeof(OBJtOSD_Door) );

	ONrEventList_Copy( &door_osd->event_list, &outOSD->osd.door_osd.event_list );
}
// ----------------------------------------------------------------------
static UUtBool OBJiDoor_IntersectsLine( const OBJtObject	*inObject, const M3tPoint3D	*inStartPoint, const M3tPoint3D *inEndPoint)
{
	M3tBoundingSphere		sphere;
	UUtBool					result = UUcFalse;
	OBJtOSD_Door			*door_osd;

	door_osd = (OBJtOSD_Door*)inObject->object_data;
	if( door_osd->door_class )
	{

		UUrMemory_Clear(&sphere, sizeof(M3tBoundingSphere));
		
		// get the bounding sphere
		OBJiDoor_GetBoundingSphere(inObject, &sphere);
		
		sphere.center.x += inObject->position.x;
		sphere.center.y += inObject->position.y;
		sphere.center.z += inObject->position.z;
		
		// do the fast test to see if the line is colliding with the bounding sphere
		result = CLrSphere_Line(inStartPoint, inEndPoint, &sphere);
		if (result == UUcTrue)
		{
			UUtUns32				i;
			M3tPoint3D				start_point;
			M3tPoint3D				end_point;
			M3tMatrix4x3			inverse_matrix;
			M3tMatrix4x3			matrix;
			M3tMatrix4x3			max_matrix;
			M3tVector3D				neg_position;
			OBJtFurnGeomArray		*geometry_array;

			// build the max coordinate conversion matrix
			MUrMatrix_BuildRotateX(M3c2Pi - M3cHalfPi,&max_matrix);

			result = UUcFalse;

			// calculate the inverse of the rotation matrix
			neg_position	= inObject->position;
			MUmVector_Negate(neg_position);

			MUrMatrix_Identity( &matrix );
			MUrMatrixStack_Matrix( &matrix, &door_osd->rotation_matrix );
			MUrMatrixStack_Matrix( &matrix, &door_osd->door_matrix[0] );
			MUrMatrix_Inverse(	   &matrix,	&inverse_matrix );

			MUrMatrix_Translate(&inverse_matrix, &neg_position);

			MUrMatrix_Multiply( &max_matrix, &inverse_matrix, &inverse_matrix );

			// calculate a start point and an end poing in object space
			MUrMatrix_MultiplyPoint(inStartPoint, &inverse_matrix, &start_point);
			MUrMatrix_MultiplyPoint(inEndPoint, &inverse_matrix, &end_point);
		
			if (door_osd->door_class == NULL) {
				// CB: just a door frame... return TRUE if intersects anywhere on bounding sphere
				// this isn't the best solution, but it's quick and doesn't crash
				result = UUcTrue;
			} else {
				for (i = 0; i < door_osd->door_class->geometry_array[0]->num_furn_geoms; i++)
				{
					result = CLrBox_Line( &door_osd->door_class->geometry_array[0]->furn_geom[i].geometry->pointArray->minmax_boundingBox, &start_point, &end_point);
					if (result == UUcTrue)  break;
				}
			}
			
			if( !result && ( door_osd->flags & OBJcDoorFlag_DoubleDoors ) && (door_osd->door_class != NULL))
			{
				geometry_array = door_osd->door_class->geometry_array[1] ? door_osd->door_class->geometry_array[1] : door_osd->door_class->geometry_array[0];

				// calculate the inverse of the rotation matrix
				MUrMatrix_Identity( &matrix );
				MUrMatrixStack_Matrix( &matrix, &door_osd->rotation_matrix );
				MUrMatrixStack_Matrix( &matrix, &door_osd->door_matrix[1] );

				MUrMatrix_Inverse(	   &matrix,	&inverse_matrix );

				MUrMatrix_Translate(&inverse_matrix, &neg_position);

				MUrMatrix_Multiply( &max_matrix, &inverse_matrix, &inverse_matrix );
			
				// calculate a start point and an end poing in object space
				MUrMatrix_MultiplyPoint(inStartPoint, &inverse_matrix, &start_point);
				MUrMatrix_MultiplyPoint(inEndPoint, &inverse_matrix, &end_point);

				for (i = 0; i < geometry_array->num_furn_geoms; i++)
				{
					result = CLrBox_Line( &geometry_array->furn_geom[i].geometry->pointArray->minmax_boundingBox, &start_point, &end_point);
					if (result == UUcTrue)  break;
				}
			}
		}
	}
	else if (door_osd->door_frame)
	{
		result = CLrBox_Line( &door_osd->door_frame->bBox, inStartPoint, inEndPoint );
	}
	return result;
}
// ----------------------------------------------------------------------
static UUtError OBJiDoor_SetDefaults(OBJtOSD_All *outOSD)
{
	UUtError				error;
	void					*instances[OBJcMaxInstances];
	UUtUns32				num_instances;
	char					*instance_name;
	
	// clear the osd
	UUrMemory_Clear(&outOSD->osd.door_osd, sizeof(OBJtOSD_Door));
	
	// get a list of instances of the class
	error = TMrInstance_GetDataPtr_List( OBJcTemplate_DoorClass, OBJcMaxInstances, &num_instances, instances);
	UUmError_ReturnOnError(error);
	
	// error out if we have no door instances
	if(!num_instances)			return UUcError_Generic;

	// copy the name of the first door template name
	instance_name = TMrInstance_GetInstanceName(instances[0]);		
	
	UUrString_Copy( outOSD->osd.door_osd.door_class_name, instance_name, OBJcMaxNameLength);

	// default initial data
	outOSD->osd.door_osd.id							= ONrMechanics_GetNextID( OBJcType_Door );
	outOSD->osd.door_osd.flags						= OBJcDoorFlag_TestMode;
	outOSD->osd.door_osd.activation_radius_squared	= UUmSQR( UUmFeetToUnits( 10 ) );
	outOSD->osd.door_osd.door_texture[0][0]			= 0;
	outOSD->osd.door_osd.door_texture[1][0]			= 0;
	
	// initialize the event list
	ONrEventList_Initialize( &outOSD->osd.door_osd.event_list );

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError OBJiDoor_New( OBJtObject *inObject, const OBJtOSD_All *inOSD)
{
	OBJtOSD_All				osd_all;
	UUtError				error;
	
	if (inOSD == NULL)
	{	
		error = OBJiDoor_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);

		inOSD = &osd_all;
	}
	else
	{
		// this is a door frame created at level load 
		if( inOSD->osd.door_osd.id == 0xFFFF ) 
		{
			osd_all.osd.door_osd.id							= ONrMechanics_GetNextID( OBJcType_Door );
			osd_all.osd.door_osd.flags						= OBJcDoorFlag_InDoorFrame | OBJcDoorFlag_TestMode;
			osd_all.osd.door_osd.render_quad				= inOSD->osd.door_osd.render_quad;
			osd_all.osd.door_osd.general_quad				= inOSD->osd.door_osd.general_quad;
			osd_all.osd.door_osd.bBox						= inOSD->osd.door_osd.bBox;
			osd_all.osd.door_osd.door_frame					= inOSD->osd.door_osd.door_frame;
			osd_all.osd.door_osd.door_frame_position		= inOSD->osd.door_osd.door_frame_position;
			osd_all.osd.door_osd.door_class_name[0]			= 0;
			osd_all.osd.door_osd.key_id						= inOSD->osd.door_osd.key_id;
			osd_all.osd.door_osd.activation_radius_squared	= inOSD->osd.door_osd.activation_radius_squared;
			UUrString_Copy( osd_all.osd.door_osd.door_texture[0],	inOSD->osd.door_osd.door_texture[0],			OBJcMaxNameLength );
			UUrString_Copy( osd_all.osd.door_osd.door_texture[1],	inOSD->osd.door_osd.door_texture[1],			OBJcMaxNameLength );
			
			ONrEventList_Initialize( &osd_all.osd.door_osd.event_list );

			inOSD = &osd_all;
		}
	}
	
	// set the object specific data and position
	OBJiDoor_SetOSD(inObject, inOSD);
	OBJiDoor_UpdatePosition(inObject);
	
	return UUcError_None;
}
// ----------------------------------------------------------------------

static UUtError OBJiDoor_SetOSD( OBJtObject *inObject, const OBJtOSD_All *inOSD)
{
	UUtError				error;
	OBJtOSD_Door			*door_osd;
	OBJtDoorClass			*door_class;

	UUmAssert(inOSD);

	// get a pointer to the object osd
	door_osd = (OBJtOSD_Door*)inObject->object_data;

	// save persistant data
	UUrString_Copy( door_osd->door_class_name,			inOSD->osd.door_osd.door_class_name,			OBJcMaxNameLength );
	door_osd->id							= inOSD->osd.door_osd.id;
//	door_osd->flags							= (door_osd->flags & ~OBJcDoorFlag_Persist) | (inOSD->osd.door_osd.flags & (OBJcDoorFlag_Persist));
	door_osd->flags							= inOSD->osd.door_osd.flags;
	door_osd->key_id						= inOSD->osd.door_osd.key_id;
	door_osd->render_quad					= inOSD->osd.door_osd.render_quad;
	door_osd->general_quad					= inOSD->osd.door_osd.general_quad;
	door_osd->door_frame					= inOSD->osd.door_osd.door_frame;
	door_osd->door_frame_position			= inOSD->osd.door_osd.door_frame_position;
	door_osd->bBox							= inOSD->osd.door_osd.bBox;
	door_osd->activation_radius_squared		= inOSD->osd.door_osd.activation_radius_squared;
	door_osd->open_time						= inOSD->osd.door_osd.open_time;
	door_osd->open_time						= UUcFramesPerSecond * 3;

	UUrString_Copy( door_osd->door_texture[0],			inOSD->osd.door_osd.door_texture[0],			OBJcMaxNameLength );
	UUrString_Copy( door_osd->door_texture[1],			inOSD->osd.door_osd.door_texture[1],			OBJcMaxNameLength );

	ONrEventList_Initialize( &door_osd->event_list );

	ONrEventList_Copy( &((OBJtOSD_All*)inOSD)->osd.door_osd.event_list, &door_osd->event_list );

	// get a pointer to the door template
	door_class = NULL;
	if( inOSD->osd.door_osd.door_class_name[0] )
	{
		error = TMrInstance_GetDataPtr( OBJcTemplate_DoorClass, inOSD->osd.door_osd.door_class_name, &door_class );
		if( error != UUcError_None )
		{
			inObject->flags		|= OBJcObjectFlag_Temporary;
			door_class			= NULL;
		}
		else
		{
			inObject->flags		&= ~OBJcObjectFlag_Temporary;
		}
	}
	else
	{

		inObject->flags		|= OBJcObjectFlag_Temporary;
	}

	door_osd->door_class = door_class;

	MUrMatrix_Identity( &door_osd->door_matrix[0] );
	MUrMatrix_Identity( &door_osd->door_matrix[1] );

	// claculate center offset
	if( door_class )
	{
		door_osd->center_offset.y = -door_class->geometry_array[0]->furn_geom[0].geometry->pointArray->minmax_boundingBox.minPoint.z;	// + door_class->geometry_array->furn_geom[0].geometry->pointArray->minmax_boundingBox.minPoint.x) / 2;
		door_osd->center_offset.x = (door_class->geometry_array[0]->furn_geom[0].geometry->pointArray->minmax_boundingBox.maxPoint.x + door_class->geometry_array[0]->furn_geom[0].geometry->pointArray->minmax_boundingBox.minPoint.x) / 2;
		door_osd->center_offset.z = (door_class->geometry_array[0]->furn_geom[0].geometry->pointArray->minmax_boundingBox.maxPoint.y + door_class->geometry_array[0]->furn_geom[0].geometry->pointArray->minmax_boundingBox.minPoint.y) / 2;
	}
	else if( door_osd->door_frame )
	{
		door_osd->center_offset.y = door_osd->door_frame->height / 2;
		door_osd->center_offset.x = 0;
		door_osd->center_offset.z = 0;
	}
	else
	{
		door_osd->center_offset.y = 0;
		door_osd->center_offset.x = 0;
		door_osd->center_offset.z = 0;
	}
	
	if( door_osd->flags & OBJcDoorFlag_DoubleDoors )
	{
		//door_osd->center_offset
		if( door_osd->door_class )
		{
			M3tVector3D			offset_pos;
			float				width;

			width				= OBJiDoor_GetGeometryWidth( inObject );

			// door 1
			offset_pos			= door_osd->center_offset;
			offset_pos.x		= width / 2;

			MUrMatrix_Translate( &door_osd->door_matrix[0], &offset_pos );
			if( door_osd->flags & OBJcDoorFlag_Mirror )
				MUrMatrix_RotateY( &door_osd->door_matrix[0], M3cPi );

			// door 2
			offset_pos			= door_osd->center_offset;
			offset_pos.x		= width / 2;

			MUrMatrix_Translate( &door_osd->door_matrix[1], &offset_pos );
			if( !(door_osd->flags & OBJcDoorFlag_Mirror ))
				MUrMatrix_RotateY( &door_osd->door_matrix[1], M3cPi );
		}
	}
	else
	{
		MUrMatrix_Translate( &door_osd->door_matrix[0], &door_osd->center_offset );
		if( door_osd->flags & OBJcDoorFlag_Mirror )
		{
//			MUrMatrix_RotateZ( &door_osd->door_matrix[0], M3cPi );
			MUrMatrix_RotateY( &door_osd->door_matrix[0], M3cPi );
		}
	}

	if (door_osd->door_texture[0][0]) {
		char uppercase_door_texture_0[32];

		UUmAssert((strlen(door_osd->door_texture[0]) + 1) < sizeof(uppercase_door_texture_0));

		strcpy(uppercase_door_texture_0, door_osd->door_texture[0]);
		UUrString_Capitalize(uppercase_door_texture_0, 32);

		door_osd->door_texture_ptr[0] = TMrInstance_GetFromName(M3cTemplate_TextureMap, uppercase_door_texture_0);
	}
	else {
		door_osd->door_texture_ptr[0] = NULL;
	}

	if (door_osd->door_texture[1][0]) {
		char uppercase_door_texture_1[32];

		UUmAssert((strlen(door_osd->door_texture[0]) + 1) < sizeof(uppercase_door_texture_1));

		strcpy(uppercase_door_texture_1, door_osd->door_texture[0]);
		UUrString_Capitalize(uppercase_door_texture_1, 32);

		door_osd->door_texture_ptr[1] = TMrInstance_GetFromName(M3cTemplate_TextureMap, uppercase_door_texture_1);
	}
	else {
		door_osd->door_texture_ptr[1] = NULL;
	}

	OBJtDoor_Reset( inObject );

	// make sure locked flag doesnt get reset
	door_osd->flags							= (door_osd->flags & ~OBJcDoorFlag_Locked) | (inOSD->osd.door_osd.flags & OBJcDoorFlag_Locked);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void OBJiDoor_UpdatePosition( OBJtObject *inObject)
{
	OBJtOSD_Door			*door_osd;
	float					rot_x;
	float					rot_y;
	float					rot_z;
	MUtEuler				euler;

	// get a pointer to the object osd
	door_osd = (OBJtOSD_Door*)inObject->object_data;
	
	// convert the rotation to radians
	rot_x = inObject->rotation.x * M3cDegToRad;
	rot_y = inObject->rotation.y * M3cDegToRad;
	rot_z = inObject->rotation.z * M3cDegToRad;
	
	// update the rotation matrix
	euler.order = MUcEulerOrderXYZs;
	euler.x = rot_x;
	euler.y = rot_y;
	euler.z = rot_z;
	MUrEulerToMatrix(&euler, &door_osd->rotation_matrix);
}

// ----------------------------------------------------------------------
static UUtUns32 OBJiDoor_Read_v1( OBJtObject *inObject, UUtUns16 inVersion, UUtBool inSwapIt, UUtUns8 *inBuffer)
{
	OBJtOSD_Door			*door_osd;
	OBJtOSD_All				osd_all;
	UUtUns32				num_bytes;

	UUrMemory_Clear(&osd_all, sizeof(osd_all));

	num_bytes = 0;

	door_osd = &osd_all.osd.door_osd;

	OBJmGetStringFromBuffer(inBuffer, door_osd->door_class_name,			OBJcMaxNameLength, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, door_osd->id,							UUtUns16, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, door_osd->key_id,						UUtUns16, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, door_osd->flags,						UUtUns16, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, door_osd->door_frame_position.x,		float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, door_osd->door_frame_position.y,		float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, door_osd->door_frame_position.z,		float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, door_osd->activation_radius_squared,	float, inSwapIt);

	door_osd->flags &= OBJcDoorFlag_Persist;

	num_bytes = 
		OBJcMaxNameLength + 
		sizeof(UUtUns16) + 
		sizeof(UUtUns16) +
		sizeof(UUtUns16) +
		sizeof(float) + 
		sizeof(float) + 
		sizeof(float) + 
		sizeof(float);

	// initialize and read the event list
	ONrEventList_Initialize( &door_osd->event_list );
	num_bytes += ONrEventList_Read( &door_osd->event_list, inVersion, inSwapIt, inBuffer );

	// bring the object up to date
	OBJiDoor_SetOSD(inObject, &osd_all);

	OBJiDoor_UpdatePosition(inObject);
		
	return num_bytes;
}


static UUtUns32 OBJiDoor_Read_v2( OBJtObject *inObject, UUtUns16 inVersion, UUtBool inSwapIt, UUtUns8 *inBuffer)
{
	OBJtOSD_Door			*door_osd;
	OBJtOSD_All				osd_all;
	UUtUns32				num_bytes;

	UUrMemory_Clear(&osd_all, sizeof(osd_all));

	num_bytes = 0;

	door_osd = &osd_all.osd.door_osd;

	OBJmGetStringFromBuffer(inBuffer, door_osd->door_class_name,			OBJcMaxNameLength, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, door_osd->id,							UUtUns16, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, door_osd->key_id,						UUtUns16, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, door_osd->flags,						UUtUns16, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, door_osd->door_frame_position.x,		float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, door_osd->door_frame_position.y,		float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, door_osd->door_frame_position.z,		float, inSwapIt);
	OBJmGet4BytesFromBuffer(inBuffer, door_osd->activation_radius_squared,	float, inSwapIt);
	OBJmGetStringFromBuffer(inBuffer, door_osd->door_texture[0],			OBJcMaxNameLength, inSwapIt);
	OBJmGetStringFromBuffer(inBuffer, door_osd->door_texture[1],			OBJcMaxNameLength, inSwapIt);

	door_osd->flags &= OBJcDoorFlag_Persist;

	num_bytes = 
		OBJcMaxNameLength + 
		sizeof(UUtUns16) + 
		sizeof(UUtUns16) +
		sizeof(UUtUns16) +
		sizeof(float) + 
		sizeof(float) + 
		sizeof(float) + 
		sizeof(float) +
		OBJcMaxNameLength + 
		OBJcMaxNameLength;

	// initialize and read the event list
	ONrEventList_Initialize( &door_osd->event_list );
	num_bytes += ONrEventList_Read( &door_osd->event_list, inVersion, inSwapIt, inBuffer );

	// bring the object up to date
	OBJiDoor_SetOSD(inObject, &osd_all);

	OBJiDoor_UpdatePosition(inObject);
		
	return num_bytes;
}


static UUtUns32 OBJiDoor_Read( OBJtObject *inObject, UUtUns16 inVersion, UUtBool inSwapIt, UUtUns8 *inBuffer)
{
	UUtUns32				num_bytes;
	if( inVersion >= OBJcVersion_16 )
	{
		num_bytes = OBJiDoor_Read_v2( inObject, inVersion, inSwapIt, inBuffer );
	}
	else
	{
		num_bytes = OBJiDoor_Read_v1( inObject, inVersion, inSwapIt, inBuffer );
	}
	return num_bytes;
}

// ----------------------------------------------------------------------
static UUtError OBJiDoor_Write( OBJtObject *inObject, UUtUns8 *ioBuffer, UUtUns32 *ioBufferSize )
{
	OBJtOSD_Door			*door_osd;
	UUtUns32				bytes_available;

	door_osd = (OBJtOSD_Door*)inObject->object_data;
	
	// set the number of bytes available
	bytes_available = *ioBufferSize;
		
	// put the geometry name in the buffer
	OBJmWriteStringToBuffer(ioBuffer, door_osd->door_class_name,				OBJcMaxNameLength, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, door_osd->id,								UUtUns16, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, door_osd->key_id,							UUtUns16, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, door_osd->flags & OBJcDoorFlag_Persist,	UUtUns16, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, door_osd->door_frame_position.x,			float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, door_osd->door_frame_position.y,			float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, door_osd->door_frame_position.z,			float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, door_osd->activation_radius_squared,		float, bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, door_osd->door_texture[0],				OBJcMaxNameLength, bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, door_osd->door_texture[1],				OBJcMaxNameLength, bytes_available, OBJcWrite_Little);

	ONrEventList_Write( &door_osd->event_list, ioBuffer, &bytes_available );

	// set ioBufferSize to the number of bytes written to the buffer
	*ioBufferSize = *ioBufferSize - bytes_available;
	
	return UUcError_None;
}
// ----------------------------------------------------------------------
static UUtUns32 OBJiDoor_GetOSDWriteSize( const OBJtObject *inObject )
{
	OBJtOSD_Door		*door_osd;
	UUtUns32				size;
	
	// get a pointer to the door_osd
	door_osd = (OBJtOSD_Door*)inObject->object_data;
	
	// calculate the number of bytes needed to save the OSD
	size = 
		OBJcMaxNameLength + 
		sizeof(UUtUns16) + 
		sizeof(UUtUns16) + 
		sizeof(UUtUns16) +
		sizeof(float) +
		sizeof(float) +
		sizeof(float) +
		sizeof(float) +
		OBJcMaxNameLength + 
		OBJcMaxNameLength;

	size += ONrEventList_GetWriteSize( &door_osd->event_list );

	return size;
}
// ----------------------------------------------------------------------
static UUtBool OBJiDoor_GetVisible(void)
{
	return OBJgDoor_DrawDoor;
}
// ----------------------------------------------------------------------
static UUtBool OBJiDoor_Search( const OBJtObject *inObject, const UUtUns32 inSearchType, const OBJtOSD_All *inSearchParams)
{
	OBJtOSD_Door		*door_osd;
	UUtBool				found;
	
	// get a pointer to the object osd
	door_osd = (OBJtOSD_Door *) inObject->object_data;
	
	// perform the check
	switch (inSearchType)
	{
		case OBJcSearch_DoorID:
			found = door_osd->id == inSearchParams->osd.door_osd.id;
		break;

		default:
			found = UUcFalse;
		break;
	}
	
	return found;
}

// ----------------------------------------------------------------------
static void OBJiDoor_SetVisible( UUtBool inIsVisible)
{
	OBJgDoor_DrawDoor = inIsVisible;
}

// ======================================================================
//
// ======================================================================

static UUtBool OBJrDoor_LevelEnd_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	return UUcTrue;
}

static UUtBool OBJrDoor_LevelBegin_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	return UUcTrue;
}

static UUtError OBJrDoor_LevelEnd( )
{
	return UUcError_None;
}

static UUtError OBJrDoor_HandleCharacterAction( OBJtObject *inObject, ONtCharacter *inCharacter )
{
	return UUcError_None;
}

#if TOOL_VERSION
UUtError OBJrDoor_DrawInitialize(void)
{
	if (!OBJgDoor_DrawInitialized) {
		UUtError				error;
		TStFontFamily			*font_family;
		IMtPixelType			textureFormat;
		
		M3rDrawEngine_FindGrayscalePixelType(&textureFormat);
		
		OBJgDoor_WhiteColor = IMrPixel_FromShade(textureFormat, IMcShade_White);

		error = TSrFontFamily_Get(TScFontFamily_Default, &font_family);
		if (error != UUcError_None)
		{
			goto cleanup;
		}
		
		error = TSrContext_New(font_family, TScFontSize_Default, TScStyle_Bold, TSc_HLeft, UUcFalse, &OBJgDoor_TextContext);
		if (error != UUcError_None)
		{
			goto cleanup;
		}
		
		OBJgDoor_Dest.x = 2;
		OBJgDoor_Dest.y = TSrFont_GetLeadingHeight(TSrContext_GetFont(OBJgDoor_TextContext, TScStyle_InUse)) + TSrFont_GetAscendingHeight(TSrContext_GetFont(OBJgDoor_TextContext, TScStyle_InUse));
		
		TSrContext_GetStringRect(OBJgDoor_TextContext, "XXXX-XXXXX-XX", &OBJgDoor_TextureBounds);
		
		OBJgDoor_TextureWidth = OBJgDoor_TextureBounds.right;
		OBJgDoor_TextureHeight = OBJgDoor_TextureBounds.bottom;
		
		OBJgDoor_WidthRatio = (float)OBJgDoor_TextureWidth / (float)OBJgDoor_TextureHeight;
		
		error = M3rTextureMap_New( OBJgDoor_TextureWidth, OBJgDoor_TextureHeight, textureFormat, M3cTexture_AllocMemory, M3cTextureFlags_None, "Door texture", &OBJgDoor_Texture);
		if (error != UUcError_None)
		{
			goto cleanup;
		}

		OBJgDoor_DrawInitialized = UUcTrue;
	}
	
	return UUcError_None;

cleanup:
	if (OBJgDoor_TextContext)
	{
		TSrContext_Delete(OBJgDoor_TextContext);
		OBJgDoor_TextContext = NULL;
	}

	return UUcError_Generic;
}

void
OBJrDoor_DrawTerminate(
	void)
{
	if (OBJgDoor_DrawInitialized) {
		if (OBJgDoor_TextContext != NULL) {
			TSrContext_Delete(OBJgDoor_TextContext);
			OBJgDoor_TextContext = NULL;
		}

		OBJgDoor_DrawInitialized = UUcFalse;
	}
}
#endif

// ==========================================================================================
// event management
// ==========================================================================================

static UUtError OBJrDoor_AddEvent( OBJtOSD_Door *inDoor_osd, ONtEvent *inEvent )
{	
	ONrEventList_AddEvent( &inDoor_osd->event_list, inEvent );
	return UUcError_None;
}

static UUtError OBJrDoor_DeleteEvent( OBJtOSD_Door *inDoor_osd, UUtUns32 inIndex )
{	
	ONrEventList_DeleteEvent( &inDoor_osd->event_list, inIndex );
	return UUcError_None;
}

static UUtError OBJrDoor_Mechanics_LevelBegin( )
{
	return UUcError_None;
}

// ==========================================================================================
// open and closing
// ==========================================================================================

UUtUns8 OBJrDoor_PointOnSide(M3tPoint3D *inPoint, OBJtObject *inObject)
{
	static M3tVector3D		origin		= {0.0f, 0.0f, 0.0f};
	static M3tVector3D		x_vector	= {1.0f, 0.0f, 0.0f};
	static M3tVector3D		y_vector	= {0.0f, 1.0f, 0.0f};
	M3tVector3D				x_vector_2;
	M3tVector3D				y_vector_2;
	M3tVector3D				origin_2;
//	M3tVector3D				obj_center;
	M3tMatrix4x3			matrix;
//	M3tMatrix4x3			from_max;
	M3tPlaneEquation		plane;
//	UUtUns32				side;
	OBJtOSD_Door			*door_osd;

	UUmAssert( inObject->object_type == OBJcType_Door );

	door_osd = (OBJtOSD_Door*)inObject->object_data;

	// CB: this code is no longer used so I'm commenting it out - 10 Jul 00
/*	MUrMatrix_Identity( &matrix );
	MUrMatrix_Translate( &matrix, &inObject->position );
	MUrMatrix_Multiply( &matrix, &door_osd->rotation_matrix, &matrix );
	MUrMatrix_Multiply( &matrix, &door_osd->door_matrix[0], &matrix );
	MUrMatrix_BuildRotateX(M3c2Pi -M3cHalfPi, &from_max);
	MUrMatrix_Multiply( &matrix, &from_max, &matrix );*/

	MUrMatrix_Identity(&matrix);
	MUrMatrix_RotateX(&matrix, (inObject->rotation.x * M3cDegToRad));
	MUrMatrix_RotateY(&matrix, (inObject->rotation.y * M3cDegToRad));
	MUrMatrix_RotateZ(&matrix, (inObject->rotation.z * M3cDegToRad));

	MUrMatrix_MultiplyPoint(&origin,   &matrix, &origin_2);
	MUrMatrix_MultiplyPoint(&x_vector, &matrix, &x_vector_2);
	MUrMatrix_MultiplyPoint(&y_vector, &matrix, &y_vector_2);

	MUmVector_Increment(origin_2,   inObject->position);
	MUmVector_Increment(x_vector_2, inObject->position);
	MUmVector_Increment(y_vector_2, inObject->position);

	MUrVector_PlaneFromEdges( &origin_2, &x_vector_2, &origin_2, &y_vector_2, &plane );

	return MUrPoint_PointBehindPlane(inPoint, &plane);
}

static UUtBool OBJrDoor_CharacterOnSide( ONtCharacter* inCharacter, OBJtObject *inObject )
{
	return OBJrDoor_PointOnSide(&inCharacter->location, inObject);
}

static UUtBool OBJrDoor_NeedToCheckNearbyCharacters(OBJtObject *inObject)
{
	OBJtOSD_Door			*door_osd;
	
	UUmAssert( inObject->object_type == OBJcType_Door );

	door_osd = (OBJtOSD_Door*)inObject->object_data;

	// manual-opening doors don't open automatically
	if ((door_osd->flags & OBJcDoorFlag_Manual) && (door_osd->state == OBJcDoorState_Closed))
		return UUcFalse;

	// our free pass
	if( OBJgDoor_IgnoreLocks )
		return UUcTrue;

	// door is jammed and cannot be opened
	if (door_osd->flags & OBJcDoorFlag_Jammed)
		return UUcFalse;

	// door is unlocked and may be opened
	if( !(door_osd->flags & OBJcDoorFlag_Locked) )
		return UUcTrue;

	// door is one sided - its open on a side
	if( door_osd->flags & OBJcDoorFlag_OneWay )
		return UUcTrue;

	// door may be opened with a key
	if (door_osd->key_id)
		return UUcTrue;

	// the door is locked and nobody can open it
	return UUcFalse;
}

UUtBool OBJrDoor_IsOpen(OBJtObject *inObject)
{
	OBJtOSD_Door			*door_osd;
	
	UUmAssert( inObject->object_type == OBJcType_Door );

	door_osd = (OBJtOSD_Door*)inObject->object_data;
	
	return (door_osd->state == OBJcDoorState_Open);
}

UUtBool OBJrDoor_OpensManually(OBJtObject *inObject)
{
	OBJtOSD_Door			*door_osd;
	
	UUmAssert( inObject->object_type == OBJcType_Door );

	door_osd = (OBJtOSD_Door*)inObject->object_data;
	
	return (door_osd->flags & OBJcDoorFlag_Manual);
}

UUtBool OBJrDoor_CharacterCanOpen( ONtCharacter* inCharacter, OBJtObject *inObject, UUtUns8 inSide )
{
	OBJtOSD_Door			*door_osd;
	
	UUmAssert( inObject->object_type == OBJcType_Door );

	door_osd = (OBJtOSD_Door*)inObject->object_data;

	// our free pass
	if( OBJgDoor_IgnoreLocks )
		return UUcTrue;

	if (door_osd->flags & OBJcDoorFlag_Jammed)
		return UUcFalse;

	// Door is locked
	if( !(door_osd->flags & OBJcDoorFlag_Locked) )
		return UUcTrue;

	// door is one sided - its open on a side
	if( door_osd->flags & OBJcDoorFlag_OneWay )
	{
		if (inSide == OBJcDoor_EitherSide)
			return UUcTrue;

		if( door_osd->flags & OBJcDoorFlag_Reverse )
			inSide = !inSide;

		if( inSide )
			return UUcTrue;
	}

	// Door requires a key
	if( door_osd->key_id && WPrInventory_HasKey(&inCharacter->inventory,door_osd->key_id) )
		return UUcTrue;

	// DENIED!
	return UUcFalse;
}

UUtBool OBJrDoor_CharacterOpen( ONtCharacter* inCharacter, OBJtObject *inObject )
{
	UUtUns8 side = OBJrDoor_CharacterOnSide(inCharacter, inObject);

	// see if we can open 
	if (OBJrDoor_CharacterCanOpen( inCharacter, inObject, side))
	{
		OBJrDoor_Open( inObject, inCharacter );
		return UUcTrue;
	}
	else
	{
		return UUcFalse;
	}
}

static void OBJrDoor_Close( OBJtObject *inObject )
{
	OBJtOSD_Door			*door_osd;
	UUtUns16				frame_start;
	UUtUns16				frame_stop;
	OBtAnimationContext		*anim_context;
	OBtAnimation			*anim;
	UUtBool					already_closed = UUcFalse;

	UUmAssert( inObject->object_type == OBJcType_Door );

	door_osd = (OBJtOSD_Door*)inObject->object_data;
	if (door_osd->door_class == NULL) {
		goto exit;
	}

	anim = door_osd->door_class->animation;
	if (!anim) {
		goto exit;
	}

	if( door_osd->state != OBJcDoorState_Closed )
	{		
		door_osd->state	= OBJcDoorState_Closed;

		if (door_osd->flags & OBJcDoorFlag_Busy) {
			// we are closing a door that was in the process of opening
			UUmAssert(door_osd->internal_door_object[0] != NULL);
			anim_context = &door_osd->internal_door_object[0]->physics->animContext;

			anim_context->animationFrame = anim->doorOpenFrames + (anim->doorOpenFrames - anim_context->animationFrame);
			if (door_osd->internal_door_object[1] != NULL) {
				door_osd->internal_door_object[1]->physics->animContext.animationFrame = anim_context->animationFrame;
			}

			already_closed = (anim_context->animationFrame == anim->numFrames);
		}

		if (!already_closed) {
			door_osd->flags	|= OBJcDoorFlag_Busy;

			if( door_osd->internal_door_object[0] )
			{
				anim_context	= &door_osd->internal_door_object[0]->physics->animContext;
				frame_start		= anim_context->animationFrame;
				frame_stop		= anim->numFrames;
				//		door_osd->door_object[0]->physics->animContext.animationFrame = door_osd->door_class->animation->numFrames - door_osd->door_object[0]->physics->animContext.animationFrame;
				OBrSetAnimation( door_osd->internal_door_object[0], anim, frame_start, frame_stop );
				if( door_osd->internal_door_object[1] )
				{
					anim_context	= &door_osd->internal_door_object[1]->physics->animContext;
					frame_start		= anim_context->animationFrame;
					frame_stop		= anim->numFrames;
					//		door_osd->door_object->physics->animContext.animationFrame = door_osd->door_class->animation->numFrames - door_osd->door_object->physics->animContext.animationFrame;
					OBrSetAnimation( door_osd->internal_door_object[1], anim, frame_start, frame_stop );
				}
			}
			else
			{
				anim_context					= &door_osd->animation_context;
				frame_start						= anim_context->animationFrame;
				frame_stop						= anim->numFrames;
				anim_context->animationStop		= frame_stop;
			}
		}

//		anim_context->animationFrame = anim->numFrames - anim_context->animationFrame;

		if (door_osd->door_class->close_sound != NULL) {
			OSrImpulse_Play(door_osd->door_class->close_sound, &inObject->position, NULL, NULL, NULL);
		}

		// play AI sound
		if (door_osd->door_class->ai_soundtype != (UUtUns32) -1) {
			AI2rKnowledge_Sound((AI2tContactType) door_osd->door_class->ai_soundtype, &inObject->position,
								door_osd->door_class->ai_soundradius, NULL, NULL);
		}

		// tell the pathfinding grid that our state has changed
		if (door_osd->pathfinding_connection[0] != NULL) {
			PHrNotifyDoorStateChange(door_osd->pathfinding_connection[0]);
		}
		if (door_osd->pathfinding_connection[1] != NULL) {
			PHrNotifyDoorStateChange(door_osd->pathfinding_connection[1]);
		}
	}

exit:
	return;
}

typedef enum OBJtDoor_HideShowGunk
{
	OBJcDoor_HideGunk,
	OBJcDoor_ShowGunk
} OBJtDoor_HideShowGunk;

static void OBJrDoor_HideShowGunk(OBJtOSD_Door *door_osd, OBJtDoor_HideShowGunk hideShow)
{
	ONtLevel *level;
	AKtEnvironment *environment;
	AKtGQ_General *gq_general_array;
	ONtObjectGunk *object_gunk;

	level = ONgGameState->level;

	if (NULL == level) {
		goto exit;
	}

	environment = level->environment;

	if (NULL == environment) {
		goto exit;
	}

	// notify the environment that the gunk changed
	AKrEnvironment_GunkChanged();

	gq_general_array = environment->gqGeneralArray->gqGeneral;
	object_gunk = door_osd->object_gunk;

	if (NULL != object_gunk) {
		TMtIndexArray *gq_array = object_gunk->gq_array;
		UUtUns32 *indices = gq_array->indices;
		UUtUns32 count = gq_array->numIndices;
		UUtUns32 itr;

		if (OBJcDoor_HideGunk == hideShow) {
			for(itr = 0; itr < count; itr++) 
			{
				UUtUns32 gq_index = indices[itr];

				gq_general_array[gq_index].flags |= (AKcGQ_Flag_Invisible | AKcGQ_Flag_No_Collision);
			}
		}
		else {
			for(itr = 0; itr < count; itr++) 
			{
				UUtUns32 gq_index = indices[itr];

				gq_general_array[gq_index].flags &= ~(AKcGQ_Flag_Invisible | AKcGQ_Flag_No_Collision);
			}
		}
	}

exit:
	return;
}


void OBJrDoor_Open( OBJtObject *inObject, ONtCharacter *inCharacter )
{
	
	OBJtOSD_Door			*door_osd;
	UUtUns16				frame_start;
	UUtUns16				frame_stop;
	OBtAnimationContext		*anim_context;
	OBtAnimation			*anim;
	UUtBool					already_open = UUcFalse;
	UUtError				error;

	UUmAssert( inObject->object_type == OBJcType_Door );

	door_osd = (OBJtOSD_Door*) inObject->object_data;	
	if (door_osd->door_class == NULL) {
		goto exit;
	}
	
	anim = door_osd->door_class->animation;
	if (!anim) {
		goto exit;
	}

	if( door_osd->state != OBJcDoorState_Open )
	{		
		if (door_osd->flags & OBJcDoorFlag_Busy) {
			// we are opening a door that was in the process of closing
			UUmAssert(door_osd->internal_door_object[0] != NULL);
			anim_context = &door_osd->internal_door_object[0]->physics->animContext;

			anim_context->animationFrame = anim->numFrames - anim_context->animationFrame;
			if (door_osd->internal_door_object[1] != NULL) {
				door_osd->internal_door_object[1]->physics->animContext.animationFrame = anim_context->animationFrame;
			}

			already_open = (anim_context->animationFrame == anim->doorOpenFrames);
		} else {
			CreateDoorObjects(inObject);
		}

		door_osd->state	= OBJcDoorState_Open;
		door_osd->open_time_left = 0;

		if (!already_open) {
			door_osd->flags	|= OBJcDoorFlag_Busy;

			if( door_osd->internal_door_object[0] )
			{
				OBtObject		*door_object;

				door_object		= door_osd->internal_door_object[0];
				anim_context	= &door_object->physics->animContext;
			
	//			if( anim_context->animationFrame == 0 )	// Handles doors that have never animated yet
	//				anim_context->animationFrame = anim->numFrames-1;

				frame_start		= anim_context->animationFrame;
				frame_stop		= anim->doorOpenFrames;

				OBrSetAnimation( door_object, anim, frame_start, frame_stop );
	//			anim_context->animationFrame = anim->numFrames - anim_context->animationFrame - 1;
					
				OBrShow( door_object, UUcTrue );

				if( door_osd->internal_door_object[1] )
				{
					door_object		= door_osd->internal_door_object[1];
					anim_context	= &door_object->physics->animContext;
			
	//				if( anim_context->animationFrame == 0 )	// Handles doors that have never animated yet
	//					anim_context->animationFrame = anim->numFrames-1;

					frame_start		= anim_context->animationFrame;
					frame_stop		= anim->doorOpenFrames;

					OBrSetAnimation( door_object, anim, frame_start, frame_stop );
	//				anim_context->animationFrame = anim->numFrames - anim_context->animationFrame - 1;
					
					OBrShow( door_object, UUcTrue );
				}
				
			}
			else
			{
				anim_context					= &door_osd->animation_context;

	//			if( anim_context->animationFrame == 0 )	// Handles doors that have never animated yet
	//				anim_context->animationFrame = anim->numFrames-1;

				frame_start						= anim_context->animationFrame;
				frame_stop						= anim->doorOpenFrames;
				anim_context->animationStop		= frame_stop;
	//			anim_context->animationFrame	= anim->numFrames - anim_context->animationFrame - 1;
			}
		}

		// make the closed gunk disapear
		OBJrDoor_HideShowGunk(door_osd, OBJcDoor_HideGunk);

		// execute any opening events
		error = ONrEventList_Execute( &door_osd->event_list, inCharacter );
		UUmAssert( error == UUcError_None );

		// play audio sound
		if (door_osd->door_class->open_sound != NULL) {
			OSrImpulse_Play(door_osd->door_class->open_sound, &inObject->position, NULL, NULL, NULL);
		}

		// play AI sound
		if (door_osd->door_class->ai_soundtype != (UUtUns32) -1) {
			AI2rKnowledge_Sound((AI2tContactType) door_osd->door_class->ai_soundtype, &inObject->position,
								door_osd->door_class->ai_soundradius, inCharacter, NULL);
		}

		// tell the pathfinding grid that our state has changed
		if (door_osd->pathfinding_connection[0] != NULL) {
			PHrNotifyDoorStateChange(door_osd->pathfinding_connection[0]);
		}
		if (door_osd->pathfinding_connection[1] != NULL) {
			PHrNotifyDoorStateChange(door_osd->pathfinding_connection[1]);
		}
	}

exit:
	return;
}

// jammed doors can't be opened by characters, only by scripting control
void OBJrDoor_Jam(UUtUns16 inID, UUtBool inJam)
{
	OBJtObject *door = OBJrDoor_GetByID(inID);
	OBJtOSD_Door *door_osd;

	if (door == NULL) {
		return;
	}

	UUmAssert(door->object_type == OBJcType_Door);
	door_osd = (OBJtOSD_Door*) door->object_data;

	if (inJam) {
		door_osd->flags |= OBJcDoorFlag_Jammed;
	} else {
		door_osd->flags &= ~OBJcDoorFlag_Jammed;
	}
}

UUtBool OBJrDoor_CharacterProximity( OBJtObject *inObject, ONtCharacter *inCharacter)
{
	float				distance_squared;
	UUtBool				proximate;
	OBJtOSD_Door		*door_osd;
	
	UUmAssert( inObject );
	UUmAssert( inObject->object_type == OBJcType_Door );

	door_osd = (OBJtOSD_Door*) inObject->object_data;

	distance_squared = MUrPoint_Distance_Squared( &inCharacter->location, &inObject->position );

	proximate = distance_squared < door_osd->activation_radius_squared;

	return proximate;
}

static ONtCharacter* OBJiDoor_ProximityDetection( OBJtObject* inObject )
{
	UUtUns32			itr;
	UUtUns32			count;
	UUtUns8				side;
	ONtCharacter		**character_list;
	ONtCharacter		*character;

	if (OBJrDoor_NeedToCheckNearbyCharacters(inObject)) {
		// loop over all characters
		character_list	= ONrGameState_LivingCharacterList_Get();
		count			= ONrGameState_LivingCharacterList_Count();

		for(itr = 0; itr < count; itr++)
		{
			character = character_list[itr];

			UUmAssert( character->flags & ONcCharacterFlag_InUse );
			UUmAssert(!(character->flags & ONcCharacterFlag_Dead));

			if (OBJrDoor_CharacterProximity(inObject,character)) 
			{
				side = OBJrDoor_CharacterOnSide(character, inObject);
				if (OBJrDoor_CharacterCanOpen(character, inObject, side)) {
					return character;
				}
			}
		}
	} else {
		// we are locked or jammed or manual-opening, don't run proximity detection
	}

	return NULL;
}

// ==========================================================================================
// updating
// ==========================================================================================

static UUtBool OBJiDoor_CanBeBlocked(OBtObject *inDoorObject, PHtPhysicsContext *inContext)
{
	OBJtObject *door_object;

	if ((inDoorObject->flags & OBcFlags_IsDoor) == 0)
		return UUcTrue;

	door_object = (OBJtObject *) inDoorObject->owner;
	if (door_object != NULL) {
		OBJtOSD_Door *door_osd = (OBJtOSD_Door *) door_object->object_data;

		if (door_osd->flags & OBJcDoorFlag_GiveUpOnCollision) {
			// cannot be blocked by anything
			return UUcFalse;
		}
	}

	switch(inContext->callback->type) {
		case PHcCallback_Weapon:
		case PHcCallback_Powerup:
			return UUcFalse;
	}

	return UUcTrue;
}

static void OBJiDoor_UpdateAnimation( OBJtObject *inObject )
{
	OBJtOSD_Door				*door_osd;
	OBtAnimationContext			*anim_context;
	OBtAnimation				*anim;

	door_osd = (OBJtOSD_Door*)inObject->object_data;

	if (door_osd->door_class == NULL)
		return;

	anim = door_osd->door_class->animation;

	UUmAssert( anim );

	anim_context					= &door_osd->animation_context;
	anim_context->frameStartTime	= ONgGameState->gameTime;
	anim_context->animationFrame	+= anim_context->animationStep;

	if( anim_context->animationFrame >= anim->numFrames )
	{
		anim_context->animationFrame = anim->numFrames - 1;
	}
	else if( anim_context->animationStop && anim_context->animationFrame >= anim_context->animationStop )
	{
		anim_context->animationFrame = anim_context->animationStop;
	}
	
	anim_context->animation = anim;

	OBrAnim_Matrix( anim_context, &door_osd->animation_matrix);
}

static void OBJrDoor_Update( OBJtObject *inObject )
{
	OBJtOSD_Door		*door_osd;
	ONtCharacter		*character;
	UUtUns16			anim_frame;
	PHtPhysicsContext	*blocking_context;
	
	UUmAssert( inObject && inObject->object_type == OBJcType_Door );

	door_osd = (OBJtOSD_Door*) inObject->object_data;

	// ok, we skip out of this function immediately if the door is in a closed state
	if( !door_osd->internal_door_object[0] && !door_osd->door_class )
	{
		return;	
	}
	else if( door_osd->internal_door_object[0] )
	{
		anim_frame = door_osd->internal_door_object[0]->physics->animContext.animationFrame;

		if ((door_osd->internal_door_object[0]->physics->animContext.numPausedFrames > 0) ||
			((door_osd->internal_door_object[1] != NULL) &&
			 (door_osd->internal_door_object[0]->physics->animContext.numPausedFrames > 0))) {
			door_osd->blocked_frames++;

			if ((door_osd->blocked_frames >= OBcDoorTotallyStuckFrames) && (door_osd->state == OBJcDoorState_Open)) {
				// we are completely wedged against something
				door_osd->flags |= OBJcDoorFlag_GiveUpOnCollision;

			} else if (door_osd->blocked_frames >= OBcDoorReverseFrames) {
				// we are blocked by something, check to see if we can reverse in our path
				door_osd->proximity_disable_timer = OBcDoor_CollisionProxDisable;

				if (door_osd->state == OBJcDoorState_Open) {
					// can't open, close again
					OBJrDoor_Close(inObject);
				} else {
					// check to see if whatever is blocking us is a character that could
					// legitimately open the door, and if so then open
					if (door_osd->internal_door_object[0]->pausing_context != NULL) {
						blocking_context = door_osd->internal_door_object[0]->pausing_context;

					} else if ((door_osd->internal_door_object[1] != NULL) &&
								(door_osd->internal_door_object[1]->pausing_context != NULL)) {
						blocking_context = door_osd->internal_door_object[1]->pausing_context;

					} else {
						blocking_context = NULL;
					}

					if ((blocking_context != NULL) && (blocking_context->flags & PHcFlags_Physics_InUse) &&
						(blocking_context->callback->type == PHcCallback_Character)) {
						door_osd->proximity_disable_timer = OBcDoor_CollisionProxDisable;
						OBJrDoor_CharacterOpen((ONtCharacter *) blocking_context->callbackData, inObject);
					}
				}
			}

		} else {
			door_osd->blocked_frames = 0;
		}
	}
	else if( door_osd->flags & OBJcDoorFlag_TestMode )
	{
		if( door_osd->flags & OBJcDoorFlag_Busy )
		{
			OBJiDoor_UpdateAnimation( inObject );
			anim_frame = door_osd->animation_context.animationFrame;
		}
	}
	else return;

	if ( door_osd->flags & OBJcDoorFlag_Busy )				// special update for busy doors
	{
		UUmAssert(door_osd->door_class != NULL);

		if( door_osd->state == OBJcDoorState_Open )
		{
			if ( anim_frame >= door_osd->door_class->animation->doorOpenFrames )
			{
				door_osd->flags &= ~OBJcDoorFlag_GiveUpOnCollision;

				if( door_osd->open_time_left )
				{
					door_osd->open_time_left--;
					if( !door_osd->open_time_left )
						door_osd->flags &= ~OBJcDoorFlag_Busy;
				}
				else
					door_osd->open_time_left = door_osd->open_time;
			}
			// still busy opening, skip the rest of the update....
			else return;
		}
		else
		{
			if (anim_frame >= door_osd->door_class->animation->numFrames - 1)
			{
				door_osd->flags &= ~OBJcDoorFlag_GiveUpOnCollision;

				// hide the moving objects
				DeleteDoorObjects(inObject);

				OBJrDoor_HideShowGunk(door_osd, OBJcDoor_ShowGunk);

				door_osd->flags &= ~OBJcDoorFlag_Busy;
			}
			else
			{
				// check for automatic proximity detection as normal
				return;
			}
		}
	}

	if (door_osd->proximity_disable_timer > 0) {
		door_osd->proximity_disable_timer--;

	} else if ((door_osd->flags & OBJcDoorFlag_Jammed) == 0) {
		// Check for automatic proximity detection
		character = OBJiDoor_ProximityDetection(inObject);
		if (character)
		{
			if( door_osd->state == OBJcDoorState_Closed ) 
				OBJrDoor_Open(inObject, character);
		}
		else if ((door_osd->flags & OBJcDoorFlag_Busy) == 0)
		{
			if( door_osd->state == OBJcDoorState_Open ) 
				OBJrDoor_Close( inObject );
		}
	}
}

// ==========================================================================================
// manual door opening
// ==========================================================================================

typedef struct OBJtDoor_TryActionEvent_UserData
{
	ONtCharacter *character;
	M3tPoint3D char_point;

	float best_angle;
	OBJtObject *best_door;
	UUtBool best_door_canopen;

} OBJtDoor_TryActionEvent_UserData;

static UUtBool OBJiDoor_TryActionEvent_Enum( OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Door *osd;
	OBJtDoor_TryActionEvent_UserData *user_data;
	M3tVector3D delta_vec;
	float angle;
	UUtBool canopen;
	UUtUns8 character_side;

	UUmAssert(inObject->object_type == OBJcType_Door);
	osd = (OBJtOSD_Door *) inObject->object_data;

	user_data = (OBJtDoor_TryActionEvent_UserData *) inUserData;
	UUmAssertReadPtr(user_data, sizeof(*user_data));

	if (osd->state == OBJcDoorState_Open) {
		// this door is already open!
		return UUcTrue;
	}

	if (!OBJrDoor_CharacterProximity(inObject, user_data->character)) {
		// the character is too far away to open this door
		return UUcTrue;
	}

	MUmVector_Subtract(delta_vec, inObject->position, user_data->char_point);
	if (MUrVector_Normalize_ZeroTest(&delta_vec)) {
		angle = 0;
	} else {
		angle = MUrAngleBetweenVectors3D(&delta_vec, &user_data->character->facingVector);
	}

	if (angle > OBcDoorFaceAngle) {
		// the character is not facing in the right direction to open the door
		return UUcTrue;
	}

	character_side = OBJrDoor_CharacterOnSide(user_data->character, inObject);
	canopen = OBJrDoor_CharacterCanOpen(user_data->character, inObject, character_side);

	if (((osd->flags & OBJcDoorFlag_Manual) == 0) && (canopen)) {
		// automatic doors that we can open don't respond to door events, but
		// if they are locked they do (negatively)
		return UUcTrue;
	}

	if (canopen && (!user_data->best_door_canopen)) {
		// we can open this door, we prefer it

	} else if ((!canopen) && user_data->best_door_canopen) {
		// we can't open this door, don't choose it instead
		return UUcTrue;

	} else {
		// compare which door we are facing closer towards
		if (angle > user_data->best_angle) {
			// we already have a better door to open
			return UUcTrue;
		}
	}

	// store this door and keep looking
	user_data->best_angle			= angle;
	user_data->best_door			= inObject;
	user_data->best_door_canopen	= canopen;

	return UUcTrue;
}

// a character pressed the action key, check to see if there is a manual-opening door nearby
UUtBool OBJrDoor_TryDoorAction(ONtCharacter *inCharacter, OBJtObject **outClosestDoor)
{
	OBJtDoor_TryActionEvent_UserData user_data;

	user_data.character = inCharacter;
	ONrCharacter_GetPelvisPosition(user_data.character, &user_data.char_point);
	user_data.best_angle = 1e+09f;
	user_data.best_door = NULL;
	user_data.best_door_canopen = UUcFalse;

	OBJrObjectType_EnumerateObjects(OBJcType_Door, OBJiDoor_TryActionEvent_Enum, (UUtUns32) &user_data); 

	*outClosestDoor = user_data.best_door;
	return user_data.best_door_canopen;
}

// ==========================================================================================
// door jumping
// ==========================================================================================

static void OBJrDoor_JumpToDoor( UUtUns32 inIndex )
{
	LItAction					action;
	OBJtOSD_Door				*door_osd;
	OBJtObject					*object;
	CAtCamera					*camera		= ONgGameState->local.camera;
	M3tVector3D					direction	= { 0, 0, 1 };
	M3tPoint3D					position;

	object = OBJrObjectType_GetObject_ByNumber( OBJcType_Door, inIndex );

	UUmAssert( object );

	door_osd = (OBJtOSD_Door*)object->object_data;

	// place the camera into the correct mode
	if( CAgCamera.mode != CAcMode_Manual )
	{
		CArManual_Enter();
		CAgCamera.modeData.manual.useMouseLook = UUcTrue;
	}

	direction.x = 0;
	direction.y = 0;
	direction.z = 1;

	position.x = object->position.x + door_osd->center_offset.x;
	position.y = object->position.y + door_osd->center_offset.y;
	position.z = object->position.z + door_osd->center_offset.z;

	OBJrSelectedObjects_Select( object, UUcFalse );

	MUrPoint_RotateYAxis( &direction, object->rotation.y * M3cDegToRad, &direction );

	camera->viewData.viewVector = direction;

	CArManual_LookAt( &position, 40.0f );

	UUrMemory_Clear(&action, sizeof(LItAction));

	CArUpdate(0, 1, &action);
}


void OBJrDoor_JumpToNextDoor()
{
	UUtUns32		count;
	
	count = OBJrObjectType_GetNumObjects( OBJcType_Door );

	if( count == 0 )
		return;

	OBJgDoor_CurrentDoor++;
	if( OBJgDoor_CurrentDoor >= count )
		OBJgDoor_CurrentDoor = 0;

	OBJrDoor_JumpToDoor( OBJgDoor_CurrentDoor );
}

void OBJrDoor_JumpToPrevDoor()
{
	UUtUns32		count;
	
	count = OBJrObjectType_GetNumObjects( OBJcType_Door );

	if( count == 0 )
		return;
	if( OBJgDoor_CurrentDoor == 0xffff || OBJgDoor_CurrentDoor == 0 )
		OBJgDoor_CurrentDoor = count - 1;
	else
		OBJgDoor_CurrentDoor--;

	OBJrDoor_JumpToDoor( OBJgDoor_CurrentDoor );
}


// ==========================================================================================
// mechanics handlers
// ==========================================================================================

static UUtError OBJtDoor_Mechanics_Terminate( OBJtObject *inObject )
{
	return UUcError_None;
}

UUtError OBJtDoor_Reset( OBJtObject *inObject )
{
	OBJtOSD_Door*			osd;

	osd = (OBJtOSD_Door*)inObject->object_data;

	if( osd->flags & OBJcDoorFlag_InitialLocked )
		osd->flags	|= OBJcDoorFlag_Locked;
	else
		osd->flags	&= ~OBJcDoorFlag_Locked;

	osd->state		= OBJcDoorState_Closed;
	osd->proximity_disable_timer = 0;

	OBJrDoor_HideShowGunk(osd, OBJcDoor_ShowGunk);

	osd->animation_context.animationFrame	= 0;
	osd->animation_context.animationStep	= 1;

	DeleteDoorObjects(inObject);

	MUrMatrix_Identity( &osd->animation_matrix );

	OBJiDoor_UpdatePosition(inObject);

	OBJiDoor_CreateBoundingSphere(inObject, osd);

	return UUcError_None;
}

// ==========================================================================================
// initialization
// ==========================================================================================

static OBJtObject* OBJrDoor_CreateDoorFrame( AKtDoorFrame* inDoorFrame )
{
	ONtLevel*				level;
	AKtEnvironment			*env;
	AKtDoorFrame*			door_frame;
	AKtGQ_Collision			*collision;
	AKtGQ_General			*quad;
	AKtGQ_Render			*rend;
	OBJtObject				*object;
	OBJtOSD_Door*			osd;
	UUtUns32				gq_index;
	M3tPoint3D				position;
	M3tPoint3D				rotation;
	UUtError				error;
	OBJtOSD_Door			door_osd;

	UUmAssert( ONgLevel && ONgLevel->environment );

	UUrMemory_Clear(&door_osd, sizeof(door_osd));

//	OBJiDoor_SetDefaults((OBJtOSD_All*)&door_osd);

	osd									= &door_osd;

	level								= ONgLevel;
	env									= level->environment;	

	door_frame							= inDoorFrame;
	gq_index							= door_frame->gq_index;
	quad								= &env->gqGeneralArray->gqGeneral[ gq_index ];
	rend								= &env->gqRenderArray->gqRender[ gq_index ];
	collision							= &env->gqCollisionArray->gqCollision[ gq_index ];

	position.x							= door_frame->position.x;
	position.y							= door_frame->position.y - ( door_frame->height / 2.0f );
	position.z							= door_frame->position.z;

	rotation.x							= 0;
	rotation.y							= door_frame->facing * M3cRadToDeg;
	rotation.z							= 0;

	door_osd.id							= 0xFFFF;
	door_osd.flags						|= OBJcDoorFlag_InDoorFrame | OBJcDoorFlag_TestMode;
	door_osd.door_frame					= door_frame;
	door_osd.door_frame_position		= door_frame->position;

	door_osd.bBox.minPoint.x			= 0;
	door_osd.bBox.minPoint.y			= 0;
	door_osd.bBox.minPoint.z			= 0;

	door_osd.bBox.maxPoint.x			= door_frame->width;
	door_osd.bBox.maxPoint.y			= door_frame->height;
	door_osd.bBox.maxPoint.z			= 0;

	door_osd.render_quad				= rend;
	door_osd.general_quad				= quad;

	door_osd.activation_radius_squared	= UUmSQR( UUmFeetToUnits( 10 ) );

	door_osd.door_frame_position		= door_frame->position;

	error = OBJrObject_New( OBJcTemplate_DoorClass, &position, &rotation, (OBJtOSD_All*) &door_osd );
	UUmAssert( error == UUcError_None );

	object								= OBJrSelectedObjects_GetSelectedObject(0);

	object->flags						|= OBJcObjectFlag_Temporary;		
	
	OBJiDoor_CreateBoundingSphere( object, &door_osd );

	return UUcError_None;
}

static OBtObjectSetup *GetDoorSetup(UUtUns16 inID)
{
	/******************
	* obj_create id id
	*/
		
	OBtObjectSetup *setup = NULL;
	UUtUns32 i;
	
	for (i=0; i < ONgGameState->level->objectSetupArray->numObjects; i++)
	{
		OBtObjectSetup *test_setup = ONgGameState->level->objectSetupArray->objects + i;

		if (inID == test_setup->doorScriptID) {		
			setup = test_setup;
			break;
		}
	}
	
	return setup;
}

static void CreateDoorObjects(OBJtObject *inDoor)
{
	OBJtOSD_Door *door_osd	= (OBJtOSD_Door*) inDoor->object_data;
	OBtObject *object;

	if (door_osd->internal_door_object_setup[0]) {
		door_osd->internal_door_object_setup[0]->flags = OBcFlags_InUse | OBcFlags_FaceCollision;
		object = OBrList_Add(ONgGameState->objects);
		
		OBrInit(object, door_osd->internal_door_object_setup[0]);	
		
		object->flags |= OBcFlags_FlatLighting;
		object->flat_lighting_shade = door_osd->shade;

		door_osd->internal_door_object[0] = object;
	}

	if (door_osd->internal_door_object_setup[1]) {
		door_osd->internal_door_object_setup[1]->flags = OBcFlags_InUse | OBcFlags_FaceCollision;
		object = OBrList_Add(ONgGameState->objects);

		OBrInit(object, door_osd->internal_door_object_setup[1]);			

		object->flags |= OBcFlags_FlatLighting;
		object->flat_lighting_shade = door_osd->shade;

		door_osd->internal_door_object[1] = object;
	}

	// reset any door objects
	if (door_osd->internal_door_object[0]) {
		door_osd->internal_door_object[0]->flags |= (OBcFlags_IsDoor | OBcFlags_NotifyCollision);
		door_osd->internal_door_object[0]->owner = inDoor;
		door_osd->internal_door_object[0]->physics->animContext.animationFrame		= 0;

		if( door_osd->flags & OBJcDoorFlag_Mirror ) {
			door_osd->internal_door_object[0]->physics->animContext.animContextFlags	|= OBcAnimContextFlags_RotateY180;
		}
	}

	if (door_osd->internal_door_object[1]) {
		door_osd->internal_door_object[1]->flags |= (OBcFlags_IsDoor | OBcFlags_NotifyCollision);
		door_osd->internal_door_object[1]->owner = inDoor;
		door_osd->internal_door_object[1]->physics->master							= door_osd->internal_door_object[0]->physics;
		door_osd->internal_door_object[1]->physics->animContext.animationFrame		= 0;
		door_osd->internal_door_object[1]->physics->animContext.animContextFlags		|= OBcAnimContextFlags_Slave;

		if(!( door_osd->flags & OBJcDoorFlag_Mirror )) {
			door_osd->internal_door_object[1]->physics->animContext.animContextFlags	|= OBcAnimContextFlags_RotateY180;
		}
	}


	return;
}

static void DeleteDoorObjects(OBJtObject *inDoor)
{
	OBJtOSD_Door *door_osd	= (OBJtOSD_Door*) inDoor->object_data;

	if (door_osd->internal_door_object[0]) {
		OBrDelete(door_osd->internal_door_object[0]);
		door_osd->internal_door_object[0] = NULL;
	}

	if (door_osd->internal_door_object[1]) {
		OBrDelete(door_osd->internal_door_object[1]);
		door_osd->internal_door_object[1] = NULL;
	}

	return;
}


static UUtError OBJrDoor_LevelBegin( OBJtObject* object )
{
	ONtLevel*				level;
	AKtEnvironment			*env;
	AKtDoorFrame*			door_frame;
	AKtGQ_Collision			*collision;
	AKtGQ_General			*quad;
	AKtGQ_Render			*rend;
	OBJtOSD_Door*			osd;
	UUtUns32				gq_index;
	UUtUns32				tag;
	ONtObjectGunk*			gunk;		

	UUmAssert( ONgLevel && ONgLevel->environment );

	level		= ONgLevel;

	env			= level->environment;

	osd			= (OBJtOSD_Door*) object->object_data;
	
	// if this door has a frame, setup frame attachment
	if( osd->flags & OBJcDoorFlag_InDoorFrame )
	{
		door_frame	= OBJrDoor_FindDoorFrameAt( &osd->door_frame_position );
		
		if( !door_frame) {
			osd->door_frame		= NULL;
			osd->render_quad	= NULL;
			osd->general_quad	= NULL;
		}
		else {
			gq_index			= door_frame->gq_index;

			quad				= &env->gqGeneralArray->gqGeneral[ gq_index ];
			rend				= &env->gqRenderArray->gqRender[ gq_index ];
			collision			= &env->gqCollisionArray->gqCollision[ gq_index ];
			
			osd->door_frame		= door_frame;
			osd->render_quad	= rend;
			osd->general_quad	= quad;

			quad->flags			|= AKcGQ_Flag_Invisible;
		}
	}

	// grab the objects if they exist

	// no need to copy gunk list just store a pointer



	osd->internal_door_object_setup[0]			= GetDoorSetup( osd->id );
	osd->internal_door_object_setup[1]			= GetDoorSetup( osd->id | 0x1000 );

	osd->internal_door_object[0]				= NULL;
	osd->internal_door_object[1]				= NULL;

	// grab the gunk list
	tag = OBJmMakeObjectTag(OBJcTypeIndex_Door, object);
	gunk = ONrLevel_FindObjectGunk(tag);
	if (gunk) {
		osd->shade = ONrLevel_ObjectGunk_GetShade(gunk);

		if (!OBJgDoor_PopLighting) {
			ONrLevel_ObjectGunk_SetShade(gunk, osd->shade);
		}

		osd->object_gunk = gunk;
	}
	else {
		osd->shade = IMcShade_White;
		osd->object_gunk = NULL;
	}

	osd->flags |= OBJcDoorFlag_TestMode;
	
	OBJtDoor_Reset( object );

	return UUcError_None;
}

static UUtError OBJtDoor_Mechanics_LevelBegin( ONtMechanicsClass* inClass )
{
	ONtLevel*				level;
	AKtEnvironment			*env;
	UUtUns32				i;
	OBJtObject				*object;

	UUtUns32				object_count;
	OBJtObject				**object_list;

	OBJrObjectType_GetObjectList( OBJcType_Door, &object_list, &object_count );

	UUmAssert( ONgLevel && ONgLevel->environment );

	level	= ONgLevel;

	env		= level->environment;

	// init all the loaded objects
	ONrMechanics_Default_ClassMethod_LevelBegin( inClass );

	if( !OBJrObjectType_IsLocked(OBJcTemplate_DoorClass) ) 
	{
		// setup empty door frames...
		for( i = 0; i < env->door_frames->door_count; i++ )
		{
			AKtDoorFrame*	door_frame;
			door_frame		= &env->door_frames->door_frames[i];
			object			= OBJrDoor_FindDoorWithFrame( door_frame );
			// create a temporary door for this object
			if( !object )
			{
				object = OBJrDoor_CreateDoorFrame( door_frame );
			}
		}
	}
	return UUcError_None;
}

void OBJrDoor_GetExportMatricies( OBJtObject *inObject, M3tMatrix4x3 *ioMatrix1A,  M3tMatrix4x3 *ioMatrix1B , M3tMatrix4x3 *ioMatrix2A,  M3tMatrix4x3 *ioMatrix2B )
{
	M3tMatrix4x3			max_matrix;
	M3tMatrix4x3			rotation_matrix;
	OBJtOSD_Door*			osd;

	M3tVector3D				offset_pos[2];
	float					width;

	osd						= (OBJtOSD_Door*) inObject->object_data;

	if( osd->flags & OBJcDoorFlag_DoubleDoors )
	{	
		width				= OBJiDoor_GetGeometryWidth( inObject );
		offset_pos[0].x		= osd->center_offset.x + ( width / 2 );
		offset_pos[1].x		= osd->center_offset.x - ( width / 2 );
	}
	else
	{
		offset_pos[0].x		= osd->center_offset.x;
		offset_pos[1].x		= osd->center_offset.x;
	}
	offset_pos[0].y		= osd->center_offset.y;
	offset_pos[0].z		= osd->center_offset.z;
	offset_pos[1].y		= osd->center_offset.y;
	offset_pos[1].z		= osd->center_offset.z;

	// build the max coordinate conversion matrix
	MUrMatrix_BuildRotateX(M3c2Pi - M3cHalfPi,&max_matrix);

	// grab object rotation matrix
	OBJrObject_GetRotationMatrix(inObject, &rotation_matrix);

	if( ioMatrix1A )
	{
		// build door 1 opening matrix
		MUrMatrix_Identity(ioMatrix1A);
		MUrMatrixStack_Translate(ioMatrix1A,	&inObject->position);
		MUrMatrixStack_Matrix(ioMatrix1A,		&rotation_matrix);
		MUrMatrixStack_Translate(ioMatrix1A,	&offset_pos[0]);
		if( osd->flags & OBJcDoorFlag_Mirror )
			MUrMatrixStack_RotateY( ioMatrix1A,	M3cPi );
		MUrMatrixStack_Matrix(ioMatrix1A,		&max_matrix);
	}

	if( ioMatrix1B )
	{	
		// build door 1 closed matrix
		MUrMatrix_Identity(ioMatrix1B);
		MUrMatrixStack_Translate(ioMatrix1B,	&inObject->position);
		MUrMatrixStack_Matrix(ioMatrix1B,		&rotation_matrix);
		MUrMatrixStack_Translate(ioMatrix1B,	&offset_pos[0]);
		MUrMatrixStack_Matrix(ioMatrix1B,		&max_matrix);
		if( osd->flags & OBJcDoorFlag_Mirror )
			MUrMatrixStack_RotateY( ioMatrix1B,	M3cPi );
	}

	if( ioMatrix2A )
	{
		// build door 2 closed matrix
		MUrMatrix_Identity(ioMatrix2A);
		MUrMatrixStack_Translate(ioMatrix2A,	&inObject->position);	
		MUrMatrixStack_Matrix(ioMatrix2A,		&rotation_matrix);		
		MUrMatrixStack_Translate(ioMatrix2A,	&offset_pos[1]);
		if( !(osd->flags & OBJcDoorFlag_Mirror) )
			MUrMatrixStack_RotateY( ioMatrix2A,	M3cPi );
		MUrMatrixStack_Matrix(ioMatrix2A,		&max_matrix);			
	}

	if( ioMatrix2B )
	{
		// build door 2 opening matrix
		MUrMatrix_Identity(ioMatrix2B);
		MUrMatrixStack_Translate(ioMatrix2B,	&inObject->position);	
		MUrMatrixStack_Matrix(ioMatrix2B,		&rotation_matrix);		
		MUrMatrixStack_Translate(ioMatrix2B,	&offset_pos[1]);
		MUrMatrixStack_Matrix(ioMatrix2B,		&max_matrix);			
		if( !(osd->flags & OBJcDoorFlag_Mirror) )
			MUrMatrixStack_RotateY( ioMatrix2B,	M3cPi );
	}
}

UUtUns32 OBJrDoor_ComputeObstruction(OBJtObject *inObject, UUtUns32 *outDoorID, M3tPoint3D *outDoorPoints)
{
	OBJtOSD_Door				*door_osd;
	OBtAnimationContext			anim_context;
	OBtAnimation				*anim;
	M3tMatrix4x3				door_anim_matrix, doubledoor_matrix;
	M3tPoint3D					swap_point;
	float						width;
	UUtUns32					itr, num_points;
	UUtBool						ignore_door0, ignore_door1;

	UUmAssert(inObject->object_type == OBJcType_Door);
	door_osd = (OBJtOSD_Door *) inObject->object_data;

	if (outDoorID != NULL) {
		*outDoorID = door_osd->id;
	}

	if (door_osd->door_class == NULL)
		return 0;

	// calculate the animation matrix for the door's end position
	anim = door_osd->door_class->animation;
	if (anim == NULL)
		return 0;

	anim_context = door_osd->animation_context;
	anim_context.animationFrame = (door_osd->state == OBJcDoorState_Open) ? anim->doorOpenFrames : (anim->numFrames - 1);
	anim_context.animation = anim;
	OBrAnim_Matrix(&anim_context, &door_anim_matrix);

	num_points = (door_osd->flags & OBJcDoorFlag_DoubleDoors) ? 4 : 2;
	width = OBJiDoor_GetGeometryWidth(inObject);

	// set up the points at the sides of the door
	outDoorPoints[0].x = -width/2;		outDoorPoints[1].x = +width/2;
	outDoorPoints[0].y = 0;				outDoorPoints[1].y = 0;
	outDoorPoints[0].z = 0;				outDoorPoints[1].z = 0;
	if (num_points == 4) {
		outDoorPoints[2] = outDoorPoints[0];
		outDoorPoints[3] = outDoorPoints[1];
	}

	if (num_points >= 2) {
		// animate the door to its current position
		MUrMatrix_MultiplyPoints(2, &door_anim_matrix, &outDoorPoints[0], &outDoorPoints[0]);
	}
	if (num_points >= 4) {
		// animate the other door in the set to its current position... we apply a 180-degree rotation
		// about the X axis, apply the animation, then apply the reverse rotation (which is the same as
		// the rotation is a homomorphism)
		MUrMatrix_BuildRotateX(M3cPi, &doubledoor_matrix);

		MUrMatrix_MultiplyPoints(2, &doubledoor_matrix, &outDoorPoints[2], &outDoorPoints[2]); 
		MUrMatrix_MultiplyPoints(2, &door_anim_matrix, &outDoorPoints[2], &outDoorPoints[2]);
		MUrMatrix_MultiplyPoints(2, &doubledoor_matrix, &outDoorPoints[2], &outDoorPoints[2]); 
	}	

	// apply the rotation from Max space to Oni space
	for (itr = 0; itr < num_points; itr++) {
		swap_point = outDoorPoints[itr];
		outDoorPoints[itr].x =  swap_point.x;
		outDoorPoints[itr].y =  swap_point.z;
		outDoorPoints[itr].z = -swap_point.y;
	}

	// apply the door matrices for each component door
	MUrMatrix_MultiplyPoints(2, &door_osd->door_matrix[0], &outDoorPoints[0], &outDoorPoints[0]);
	if (num_points == 4) {
		MUrMatrix_MultiplyPoints(2, &door_osd->door_matrix[1], &outDoorPoints[2], &outDoorPoints[2]);
	}

	// take away the center offset to give us points at the base of the door
	for (itr = 0; itr < num_points; itr++) {
		outDoorPoints[itr].y -= door_osd->center_offset.y;
	}

	// apply the rotation matrix of the object
	MUrMatrix_MultiplyPoints(num_points, &door_osd->rotation_matrix, outDoorPoints, outDoorPoints);

	ignore_door0 = ignore_door1 = UUcFalse;
	if ((outDoorPoints[0].y >= OBcDoorOpenUpwardsHeight) && (outDoorPoints[1].y >= OBcDoorOpenUpwardsHeight)) {
		// the base of the first door is so far off the ground that it must be opening upwards, it is not
		// an obstruction
		ignore_door0 = UUcTrue;
	}
	if ((num_points < 4) || ((outDoorPoints[2].y >= OBcDoorOpenUpwardsHeight) && (outDoorPoints[3].y >= OBcDoorOpenUpwardsHeight))) {
		// the base of the second door is so far off the ground that it must be opening upwards, it is not
		// an obstruction
		ignore_door1 = UUcTrue;
	}

	if (ignore_door0 && ignore_door1) {
		// discard both doors
		return 0;
	} else if (ignore_door0 && !ignore_door1) {
		// move the second door into where the first door was
		outDoorPoints[0] = outDoorPoints[2];
		outDoorPoints[1] = outDoorPoints[3];
		num_points = 2;
	} else if (!ignore_door0 && ignore_door1) {
		// discard the second door
		num_points = 2;
	}

	// apply the translation of the object
	for (itr = 0; itr < num_points; itr++) {
		MUmVector_Add(outDoorPoints[itr], outDoorPoints[itr], inObject->position);
	}

	return num_points;
}

// ==========================================================================================
// debugging code
// ==========================================================================================

#if TOOL_VERSION
static void OBJiDoor_DumpDebugLine(char *inMsg, BFtFile *inFile)
{
	COrConsole_Print(COcPriority_Console, 0xFFFF9090, 0xFFFF3030, inMsg);
	if (inFile != NULL) {
		BFrFile_Printf(inFile, "%s\n", inMsg);
	}
}

static void OBJiDoor_DumpDebugInfo(OBJtObject *inDoor, BFtFile *inFile)
{
	OBJtOSD_Door *door_osd;
	char msg[512];

	UUmAssert(inDoor->object_type == OBJcType_Door);
	door_osd = (OBJtOSD_Door *) inDoor->object_data;

	sprintf(msg, "door %d: class %s%s, position (%f %f %f) rotation %f",
		door_osd->id, door_osd->door_class_name, (door_osd->door_class == NULL) ? " (not found)" : "",
		inDoor->position.x, inDoor->position.y, inDoor->position.z, inDoor->rotation);
	OBJiDoor_DumpDebugLine(msg, inFile);

	strcpy(msg, "  flags [");

	if (door_osd->flags & OBJcDoorFlag_InitialLocked) {
		strcat(msg, " initial-locked");
	}
	if (door_osd->flags & OBJcDoorFlag_Locked) {
		strcat(msg, " locked");
	}
	if (door_osd->flags & OBJcDoorFlag_InDoorFrame) {
		strcat(msg, " door-frame");
	}
	if (door_osd->flags & OBJcDoorFlag_Manual) {
		strcat(msg, " manual");
	}
	if (door_osd->flags & OBJcDoorFlag_Busy) {
		strcat(msg, " busy");
	}
	if (door_osd->flags & OBJcDoorFlag_DoubleDoors) {
		strcat(msg, " double");
	}
	if (door_osd->flags & OBJcDoorFlag_Mirror) {
		strcat(msg, " mirror");
	}
	if (door_osd->flags & OBJcDoorFlag_OneWay) {
		strcat(msg, " one-way");
	}
	if (door_osd->flags & OBJcDoorFlag_Reverse) {
		strcat(msg, " reverse");
	}
	if (door_osd->flags & OBJcDoorFlag_Jammed) {
		strcat(msg, " jammed");
	}
	if (door_osd->flags & OBJcDoorFlag_InitialOpen) {
		strcat(msg, " initial-open");
	}
	if (door_osd->state == OBJcDoorState_Closed) {
		strcat(msg, " closed");
	} else if (door_osd->state == OBJcDoorState_Open) {
		strcat(msg, " open");
	} else {
		strcat(msg, " unknown-state");
	}
	strcat(msg, " ]");
	OBJiDoor_DumpDebugLine(msg, inFile);
}

typedef struct OBJtDoor_DebugDumpStructure {
	BFtFile *dump_file;
	UUtBool restrict_position;
	M3tPoint3D position;
	float radius;
} OBJtDoor_DebugDumpStructure;

static UUtBool OBJiDoor_DebugDump_Enum(OBJtObject *inObject, UUtUns32 inUserData)
{
	OBJtDoor_DebugDumpStructure *dump_structure = (OBJtDoor_DebugDumpStructure *) inUserData;

	UUmAssertReadPtr(dump_structure, sizeof(*dump_structure));

	if (dump_structure->restrict_position) {
		// only dump doors near some point
		float dist_sq = MUrPoint_Distance_Squared(&inObject->position, &dump_structure->position);

		if (dist_sq > UUmSQR(dump_structure->radius)) {
			return UUcTrue;
		}
	}

	OBJiDoor_DumpDebugInfo(inObject, dump_structure->dump_file);
	return UUcTrue;
}

UUtError OBJrDoor_DumpAll(void)
{
	OBJtDoor_DebugDumpStructure dump_structure;
	BFtFileRef *file_ref;
	UUtError error;

	dump_structure.restrict_position = UUcFalse;
	dump_structure.dump_file = NULL;

	error = BFrFileRef_MakeFromName("door_debug.txt", &file_ref);
	if (error == UUcError_None) {
		error = BFrFile_Open(file_ref, "wt", &dump_structure.dump_file);
		if (error != UUcError_None) {
			dump_structure.dump_file = NULL;
		}
	}

	OBJrObjectType_EnumerateObjects(OBJcType_Door, OBJiDoor_DebugDump_Enum, (UUtUns32) &dump_structure); 

	if (dump_structure.dump_file != NULL) {
		BFrFile_Close(dump_structure.dump_file);
	}

	return UUcError_None;
}

UUtError OBJrDoor_DumpNearby(M3tPoint3D *inPoint, float inRadius)
{
	OBJtDoor_DebugDumpStructure dump_structure;

	dump_structure.restrict_position = UUcTrue;
	dump_structure.position = *inPoint;
	dump_structure.radius = inRadius;
	dump_structure.dump_file = NULL;

	OBJrObjectType_EnumerateObjects(OBJcType_Door, OBJiDoor_DebugDump_Enum, (UUtUns32) &dump_structure); 

	return UUcError_None;
}
#endif

// ==========================================================================================
// class initialization / registration
// ==========================================================================================

UUtError OBJrDoor_Initialize( )
{
	UUtError				error;
	OBJtMethods				methods;
	ONtMechanicsMethods		mechanics_methods;

	// initialize globals
	OBJgDoor_DrawDoor					= UUcTrue;	
	OBJgDoor_CurrentDoor				= 0xFFFF;
	OBJgDoor_Shade						= IMcShade_White;
	OBJgDoor_IgnoreLocks				= UUcFalse;
	OBJgDoor_ShowDebug					= UUcFalse;
	OBJgDoor_ShowActivationCircles		= UUcFalse;

	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));
	
	// set up the methods structure
	methods.rNew						= OBJiDoor_New;
	methods.rSetDefaults				= OBJiDoor_SetDefaults;
	methods.rDelete						= OBJiDoor_Delete;
	methods.rDraw						= OBJiDoor_Draw;
	methods.rEnumerate					= OBJiDoor_Enumerate;
	methods.rGetBoundingSphere			= OBJiDoor_GetBoundingSphere;
	methods.rOSDGetName					= OBJiDoor_OSDGetName;
	methods.rIntersectsLine				= OBJiDoor_IntersectsLine;
	methods.rUpdatePosition				= OBJiDoor_UpdatePosition;
	methods.rGetOSD						= OBJiDoor_GetOSD;
	methods.rGetOSDWriteSize			= OBJiDoor_GetOSDWriteSize;
	methods.rSetOSD						= OBJiDoor_SetOSD;
	methods.rRead						= OBJiDoor_Read;
	methods.rWrite						= OBJiDoor_Write;
	
	// set up the type methods
	methods.rGetClassVisible			= OBJiDoor_GetVisible;
	methods.rSearch						= OBJiDoor_Search;
	methods.rSetClassVisible			= OBJiDoor_SetVisible;
		
	// set up the mechanics methods
	mechanics_methods.rInitialize		= OBJrDoor_LevelBegin;
	mechanics_methods.rTerminate		= OBJtDoor_Mechanics_Terminate;
	mechanics_methods.rReset			= OBJtDoor_Reset;
	mechanics_methods.rUpdate			= OBJrDoor_Update;
	mechanics_methods.rGetID			= OBJrDoor_GetID;

 	mechanics_methods.rClassLevelBegin	= OBJtDoor_Mechanics_LevelBegin;
	mechanics_methods.rClassLevelEnd	= NULL;
	mechanics_methods.rClassReset		= NULL;
	mechanics_methods.rClassUpdate		= NULL;

	// register the door methods
	error = ONrMechanics_Register( OBJcType_Door, OBJcTypeIndex_Door, "Door", sizeof(OBJtOSD_Door), &methods, 0, &mechanics_methods );
	UUmError_ReturnOnError(error);
	
#if CONSOLE_DEBUGGING_COMMANDS
	error = SLrGlobalVariable_Register_Bool( "door_show_debug", "Shows debug geometry", &OBJgDoor_ShowDebug );
	UUmError_ReturnOnError(error);

	error = SLrGlobalVariable_Register_Bool( "door_ignore_locks", "Disables all door locks", &OBJgDoor_IgnoreLocks );
	UUmError_ReturnOnError(error);

	error = SLrGlobalVariable_Register_Bool( "door_show_activation", "Draws a circle where door activates", &OBJgDoor_ShowActivationCircles );
	UUmError_ReturnOnError(error);

	error = SLrGlobalVariable_Register_Bool( "door_pop_lighting", "uses bad door lighting", &OBJgDoor_PopLighting);
	UUmError_ReturnOnError(error);
#endif

	return UUcError_None;
}

void OBJrDoor_UpdateSoundPointers(void)
{
	UUtUns32 itr, num_classes;
	OBJtDoorClass *door_class;
	UUtError error;

	num_classes = TMrInstance_GetTagCount(OBJcTemplate_DoorClass);

	for (itr = 0; itr < num_classes; itr++) {
		error = TMrInstance_GetDataPtr_ByNumber(OBJcTemplate_DoorClass, itr, (void **) &door_class);
		if (error == UUcError_None) {
			door_class->open_sound  = OSrImpulse_GetByName(door_class->open_soundname);
			door_class->close_sound = OSrImpulse_GetByName(door_class->close_soundname);
		}
	}
}
