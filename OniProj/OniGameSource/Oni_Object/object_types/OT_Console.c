// ======================================================================
// OT_Console.c
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
#include "Oni_Mechanics.h"

// ======================================================================
// defines
// ======================================================================

// ======================================================================
// globals
// ======================================================================

static UUtBool					OBJgConsole_DrawConsole;

// ======================================================================
// prototypes
// ======================================================================

static UUtError OBJrConsole_CalculateMarker( const OBJtObject *inObject );
static UUtError OBJiConsole_SetOSD( OBJtObject *inObject, const OBJtOSD_All *inOSD);
static UUtError OBJiConsole_Reset( OBJtObject *inObject );

// ======================================================================
// functions
// ======================================================================
static void _DrawDot(M3tPoint3D *inPoint)
{
	UUtError error;
	M3tTextureMap *dotTexture;
	float scale = .003f;
	float cam_sphere_dist;
	M3tPoint3D cameraLocation;

	M3rCamera_GetViewData(ONgActiveCamera, &cameraLocation, NULL, NULL);

	error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "dot",  (void **) &dotTexture);
	UUmAssert(UUcError_None == error);

	if (error) 	return;

	cam_sphere_dist = MUmVector_GetDistance( cameraLocation, (*inPoint));

	//if (cam_sphere_dist > .001f)
	//{
		//scale *= cam_sphere_dist;
		//M3rSprite_Draw( dotTexture, inPoint, scale, scale,  IMcShade_Red, M3cMaxAlpha, 0, NULL, NULL, 0, 0, 0);
	//}

	return;
}
// ----------------------------------------------------------------------
static void OBJiConsole_CreateBoundingSphere( OBJtObject *inObject, OBJtOSD_Console	*inOSD)
{
	UUtUns32				i;
	M3tBoundingSphere		*sphere;
	M3tPoint3D				new_center;
	float					new_radius;

	new_center.x = 0.0f;
	new_center.y = 0.0f;
	new_center.z = 0.0f;

	if( !inOSD->console_class )
		return;

	// calculate the center of the sphere
	for (i = 0; i < inOSD->console_class->geometry_array->num_furn_geoms; i++)
	{
		sphere = &inOSD->console_class->geometry_array->furn_geom[i].geometry->pointArray->boundingSphere;
		MUmVector_Increment(new_center, sphere->center);
	}

	new_center.x /= inOSD->console_class->geometry_array->num_furn_geoms;
	new_center.y /= inOSD->console_class->geometry_array->num_furn_geoms;
	new_center.z /= inOSD->console_class->geometry_array->num_furn_geoms;

	// caculate the new radius
	new_radius = 0.0f;
	for (i = 0; i < inOSD->console_class->geometry_array->num_furn_geoms; i++)
	{
		M3tVector3D			vector;
		float				temp_radius;

		sphere = &inOSD->console_class->geometry_array->furn_geom[i].geometry->pointArray->boundingSphere;
		MUmVector_Subtract(vector, new_center, sphere->center);
		temp_radius = MUrVector_Length(&vector) + sphere->radius;

		new_radius = UUmMax(new_radius, temp_radius);
	}
	// set the bounding sphere
	inOSD->bounding_sphere.center = new_center;
	inOSD->bounding_sphere.radius = new_radius;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void OBJiConsole_Delete( OBJtObject *inObject)
{
	OBJtOSD_Console		*console_osd;

	// get a pointer to the object osd
	console_osd = (OBJtOSD_Console*)inObject->object_data;

	// delete the memory allocated for the ls_data_array
	if (console_osd->ls_data_array)
	{
		UUrMemory_Block_Delete(console_osd->ls_data_array);
		console_osd->ls_data_array = NULL;
		console_osd->num_ls_datas = 0;
	}

	ONrEventList_Destroy(&console_osd->event_list);

	return;
}


// ----------------------------------------------------------------------
static void OBJiConsole_Draw( OBJtObject *inObject, UUtUns32 inDrawFlags)
{
	OBJtOSD_Console			*console_osd;
	M3tMatrix4x3			console_matrix;

	if (OBJgConsole_DrawConsole == UUcFalse) { return; }
	// don't draw when the non-occluding geometry is hidden
	if (AKgDraw_Occl == UUcTrue) { return; }

	console_osd = (OBJtOSD_Console*)inObject->object_data;
	if (console_osd->console_class == NULL) { return; }
	if (console_osd->console_class->geometry_array == NULL) { return; }

	MUrMatrix_BuildTranslate(inObject->position.x, inObject->position.y, inObject->position.z, &console_matrix);
	MUrMatrixStack_Matrix(&console_matrix, &console_osd->rotation_matrix);

#if TOOL_VERSION
	// dont draw the main geometry if its been gunkified
	if(!(inObject->flags & OBJcObjectFlag_Gunk))
	{
		M3tBoundingBox			bBox;
		UUtUns32				i;

		for (i = 0; i < console_osd->console_class->geometry_array->num_furn_geoms; i++)
		{
			UUtUns32			flags;

			// check the visibility flags
			flags = console_osd->console_class->geometry_array->furn_geom[i].gq_flags;
			if (((flags & AKcGQ_Flag_Invisible) != 0) && (AKgDraw_Invis == UUcFalse))	continue;

			// draw the bounding box if this is the selected object
			if (inDrawFlags & OBJcDrawFlag_Selected)
			{
				M3rMinMaxBBox_To_BBox( &console_osd->console_class->geometry_array->furn_geom[i].geometry->pointArray->minmax_boundingBox, &bBox);
				MUrMatrix_MultiplyPoints(8, &console_matrix, bBox.localPoints, bBox.localPoints);

				M3rBBox_Draw_Line(&bBox, IMcShade_White);
			}


			AKrEnvironment_DrawIfVisible(console_osd->console_class->geometry_array->furn_geom[i].geometry, &console_matrix);
		}
	}
#endif


	// draw the screen
	UUmAssert( console_osd->console_class->screen_geometry );
	{
		M3tTextureMap*		old_texture;
		M3tTextureMap*		cur_texture;

		if (console_osd->flags & OBJcConsoleFlag_Active)
		{
			if( console_osd->flags & OBJcConsoleFlag_Triggered ) {
				cur_texture = console_osd->triggered_screen_texture;
			}
			else {
				cur_texture = console_osd->active_screen_texture;
			}
		}
		else {
			cur_texture = console_osd->inactive_screen_texture;
		}

		old_texture = console_osd->console_class->screen_geometry->baseMap;
		console_osd->console_class->screen_geometry->baseMap = cur_texture;
		AKrEnvironment_DrawIfVisible(console_osd->console_class->screen_geometry, &console_matrix);
		console_osd->console_class->screen_geometry->baseMap = old_texture;
	}

#if TOOL_VERSION
	// draw the rotation rings if this is the selected object
	if (inDrawFlags & OBJcDrawFlag_Selected)
	{
		M3tPoint3D laserFrom;
		M3tPoint3D laserTo;
		M3tVector3D laserVector;

		laserFrom = console_osd->console_class->action_position;
		laserVector = console_osd->console_class->action_vector;
		MUmVector_Add( laserTo, laserFrom, laserVector );
		M3rGeom_Line_Light(&laserFrom, &laserTo, 0xf00000);

		M3rMatrixStack_Push();
			M3rMatrixStack_Multiply(&console_matrix);
			OBJrObjectUtil_DrawRotationRings(inObject, &console_osd->bounding_sphere, inDrawFlags);
		M3rMatrixStack_Pop();
	}

	// _DrawDot( &console_osd->action_marker->position );
#endif

	return;
}

// ----------------------------------------------------------------------
static UUtError OBJiConsole_Enumerate( OBJtObject *inObject, OBJtEnumCallback_ObjectName		inEnumCallback, UUtUns32 inUserData)
{
	return OBJrObjectUtil_EnumerateTemplate( "", OBJcTemplate_ConsoleClass, inEnumCallback, inUserData);
}

// ----------------------------------------------------------------------
static void OBJiConsole_GetBoundingSphere( const OBJtObject *inObject, M3tBoundingSphere *outBoundingSphere )
{
	OBJtOSD_Console		*console_osd;

	console_osd = (OBJtOSD_Console*)inObject->object_data;

	*outBoundingSphere = console_osd->bounding_sphere;
}

// ----------------------------------------------------------------------
static void OBJiConsole_OSDGetName( const OBJtOSD_All *inOSD, char *outName, UUtUns32 inNameLength )
{
	const OBJtOSD_Console		*console_osd;

	console_osd = &inOSD->osd.console_osd;

	sprintf(outName, "%s_%d", console_osd->console_class_name, console_osd->id);
}

// ----------------------------------------------------------------------
static void OBJiConsole_OSDSetName(OBJtOSD_All *inOSD, const char *inName)
{
	return;
}

// ----------------------------------------------------------------------
static void OBJiConsole_GetOSD( const OBJtObject *inObject, OBJtOSD_All *outOSD)
{
	OBJtOSD_Console		*console_osd;

	console_osd = (OBJtOSD_Console*)inObject->object_data;

	UUrMemory_MoveFast( console_osd, &outOSD->osd.console_osd, sizeof(OBJtOSD_Console) );

	ONrEventList_Copy( &console_osd->event_list, &outOSD->osd.console_osd.event_list );
}


// ----------------------------------------------------------------------
static UUtBool OBJiConsole_IntersectsLine( const OBJtObject	*inObject, const M3tPoint3D	*inStartPoint, const M3tPoint3D *inEndPoint)
{
	M3tBoundingSphere		sphere;
	UUtBool					result;

	UUrMemory_Clear(&sphere, sizeof(M3tBoundingSphere));

	// get the bounding sphere
	OBJrObject_GetBoundingSphere(inObject, &sphere);

	sphere.center.x += inObject->position.x;
	sphere.center.y += inObject->position.y;
	sphere.center.z += inObject->position.z;

	// do the fast test to see if the line is colliding with the bounding sphere
	result = CLrSphere_Line(inStartPoint, inEndPoint, &sphere);
	if (result) {
		OBJtOSD_Console		*console_osd;
		UUtUns32				i;
		M3tPoint3D				start_point;
		M3tPoint3D				end_point;
		M3tMatrix4x3			inverse_matrix;
		M3tVector3D				neg_position;

		result = UUcFalse;

		// because the line collided with the bounding sphere, test to see if the line
		// collides with the bounding box of the geometries
		console_osd = (OBJtOSD_Console*)inObject->object_data;
		if (console_osd->console_class == NULL) { return UUcFalse; }
		if (console_osd->console_class->geometry_array == NULL) { return UUcFalse; }

		// calculate the inverse of the rotation matrix
		MUrMatrix_Inverse(&console_osd->rotation_matrix, &inverse_matrix);
		neg_position = inObject->position;
		MUmVector_Negate(neg_position);
		MUrMatrix_Translate(&inverse_matrix, &neg_position);

		// calculate a start point and an end poing in object space
		MUrMatrix_MultiplyPoint(inStartPoint, &inverse_matrix, &start_point);
		MUrMatrix_MultiplyPoint(inEndPoint, &inverse_matrix, &end_point);

		// check the bounding box of each geometry to see if the line intersects the geometry
		for (i = 0; i < console_osd->console_class->geometry_array->num_furn_geoms; i++)
		{
			result =
				CLrBox_Line(
					&console_osd->console_class->geometry_array->furn_geom[i].geometry->pointArray->minmax_boundingBox,
					&start_point,
					&end_point);
			if (result == UUcTrue) { break; }
		}

		return result;
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtError OBJiConsole_SetDefaults(OBJtOSD_All *outOSD)
{
	UUtError				error;
	void					*instances[OBJcMaxInstances];
	UUtUns32				num_instances;
	char					*instance_name;

	// clear the osd
	UUrMemory_Clear(&outOSD->osd.console_osd, sizeof(OBJtOSD_Console));

	// get a list of instances of the class
	error = TMrInstance_GetDataPtr_List( OBJcTemplate_ConsoleClass, OBJcMaxInstances, &num_instances, instances);
	UUmError_ReturnOnError(error);

	// error out if we have no console instances
	if(!num_instances)			return UUcError_Generic;

	// copy the name of the first console template name
	instance_name = TMrInstance_GetInstanceName(instances[0]);

	UUrString_Copy( outOSD->osd.console_osd.console_class_name, instance_name, OBJcMaxNameLength);

	// default initial data
	outOSD->osd.console_osd.id					= ONrMechanics_GetNextID( OBJcType_Console );
	outOSD->osd.console_osd.flags				= OBJcConsoleFlag_InitialActive | OBJcConsoleFlag_Active;

	// initialize the event list
	ONrEventList_Initialize( &outOSD->osd.console_osd.event_list );

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError OBJiConsole_New(OBJtObject	*inObject, const OBJtOSD_All *inOSD)
{
	OBJtOSD_All				osd_all;
	UUtError				error;

	if (inOSD == NULL)
	{
		error = OBJiConsole_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);

		inOSD = &osd_all;
	}

	// set the object specific data and position
	OBJiConsole_SetOSD(inObject, inOSD);
	OBJrObject_UpdatePosition(inObject);

	return UUcError_None;
}


// ----------------------------------------------------------------------
static UUtError OBJiConsole_SetOSD( OBJtObject *inObject, const OBJtOSD_All *inOSD)
{
	UUtError				error;
	OBJtOSD_Console			*console_osd;
	OBJtConsoleClass		*console;
	UUtUns32				size;
	UUtUns32				i;

	UUmAssert(inOSD);

	// get a pointer to the object osd
	console_osd = (OBJtOSD_Console*)inObject->object_data;

	// save persistant data
	UUrString_Copy( console_osd->console_class_name,	inOSD->osd.console_osd.console_class_name,		OBJcMaxNameLength );
	UUrString_Copy( console_osd->screen_inactive,		inOSD->osd.console_osd.screen_inactive,			OBJcMaxNameLength );
	UUrString_Copy( console_osd->screen_active,			inOSD->osd.console_osd.screen_active,			OBJcMaxNameLength );
	UUrString_Copy( console_osd->screen_triggered,		inOSD->osd.console_osd.screen_triggered,		OBJcMaxNameLength );
	console_osd->id				= inOSD->osd.console_osd.id;

	//console_osd->flags			= (console_osd->flags & OBJcConsoleFlag_Persist);
	//console_osd->flags			|= inOSD->osd.console_osd.flags & (OBJcConsoleFlag_Persist|OBJcConsoleFlag_Triggered);

	console_osd->flags			= inOSD->osd.console_osd.flags;

	ONrEventList_Copy( &((OBJtOSD_All*)inOSD)->osd.console_osd.event_list, &console_osd->event_list );

	// get a pointer to the console template
	error = TMrInstance_GetDataPtr( OBJcTemplate_ConsoleClass, inOSD->osd.console_osd.console_class_name, &console );
	if( error != UUcError_None )
	{
		COrConsole_Printf("failed to find console class %s", inOSD->osd.console_osd.console_class_name);

		error = TMrInstance_GetDataPtr_ByNumber(OBJcTemplate_ConsoleClass, 0, &console);

		UUmError_ReturnOnErrorMsg(error, "failed to find any console class");

		inObject->flags				|= OBJcObjectFlag_Temporary;
		console	= NULL;
	}

	console_osd->console_class = console;

	// delete the old ls_data_array
	if (console_osd->ls_data_array)
	{
		UUrMemory_Block_Delete(console_osd->ls_data_array);
		console_osd->ls_data_array = NULL;
		console_osd->num_ls_datas = 0;
	}

	// calculate the size for the ls_data_array and the number of OBJtLSData's in it
	size = 0;
	console_osd->num_ls_datas	= 0;
	if( console )
	{
		for (i = 0; i < console_osd->console_class->geometry_array->num_furn_geoms; i++)
		{
			if (console_osd->console_class->geometry_array->furn_geom[i].ls_data == NULL) { continue; }

			size += sizeof(OBJtLSData);
			console_osd->num_ls_datas++;
		}

		if (size > 0)
		{
			// allocate memory for the ls_data_array
			console_osd->ls_data_array = (OBJtLSData*)UUrMemory_Block_NewClear(size);
			UUmError_ReturnOnNull(console_osd->ls_data_array);
			// if inOSD has a ls_data_array, then copy the data from it into the console->ls_data_array
			// otherwise copy the ones from the geometry_array
			if (inOSD->osd.console_osd.ls_data_array != NULL)
			{
				// copy the data from the inOSD ls_data_array
				UUrMemory_MoveFast( inOSD->osd.console_osd.ls_data_array, console_osd->ls_data_array, size);
			}
			else
			{
				UUtUns32		j;
				// copy the data from the OBJtLSData template instance in the furn_geom
				for (i = 0, j = 0; i < console_osd->console_class->geometry_array->num_furn_geoms; i++)
				{
					if (console_osd->console_class->geometry_array->furn_geom[i].ls_data == NULL) { continue; }
					UUrMemory_MoveFast( console_osd->console_class->geometry_array->furn_geom[i].ls_data, (console_osd->ls_data_array + j), sizeof(OBJtLSData));
					j++;
				}
			}
		}

		// create action marker
		if( ONgLevel && !console_osd->action_marker )
		{
			console_osd->action_marker = ONrLevel_ActionMarker_New( );

			UUmAssert( console_osd->action_marker );

			console_osd->action_marker->object = (UUtUns32) inObject;
		}

		console_osd->active_screen_texture = NULL;
		console_osd->inactive_screen_texture = NULL;
		console_osd->triggered_screen_texture = NULL;

		UUrString_Copy( console_osd->screen_inactive,	inOSD->osd.console_osd.screen_inactive,		OBJcMaxNameLength );
		UUrString_Copy( console_osd->screen_active,		inOSD->osd.console_osd.screen_active,		OBJcMaxNameLength );
		UUrString_Copy( console_osd->screen_triggered,	inOSD->osd.console_osd.screen_triggered,	OBJcMaxNameLength );

		UUmAssert( console_osd->console_class->screen_inactive );
		UUmAssert( console_osd->console_class->screen_active );
		UUmAssert( console_osd->console_class->screen_triggered );

		if (console_osd->screen_inactive[0]) {
			console_osd->inactive_screen_texture = M3rTextureMap_GetFromName_UpperCase(console_osd->screen_inactive);
		}
		else {
			console_osd->inactive_screen_texture = M3rTextureMap_GetFromName_UpperCase(console_osd->console_class->screen_inactive);
		}

		if (console_osd->screen_active[0]) {
			console_osd->active_screen_texture = M3rTextureMap_GetFromName_UpperCase(console_osd->screen_active);
		}
		else {
			console_osd->active_screen_texture = M3rTextureMap_GetFromName_UpperCase(console_osd->console_class->screen_active);
		}

		if (console_osd->screen_triggered[0]) {
			console_osd->triggered_screen_texture = M3rTextureMap_GetFromName_UpperCase(console_osd->screen_triggered);
		}
		else {
			console_osd->triggered_screen_texture = M3rTextureMap_GetFromName_UpperCase(console_osd->console_class->screen_triggered);
		}

		if (NULL == console_osd->inactive_screen_texture) {
			COrConsole_Printf("failed to find inactive_screen_texture");
			console_osd->inactive_screen_texture = M3rTextureMap_GetFromName("NONE");
		}

		if (NULL == console_osd->active_screen_texture) {
			COrConsole_Printf("failed to find console_osd->active_screen_texture");
			console_osd->active_screen_texture = M3rTextureMap_GetFromName("NONE");
		}

		if (NULL == console_osd->triggered_screen_texture) {
			COrConsole_Printf("failed to find triggered_screen_texture");
			console_osd->triggered_screen_texture = M3rTextureMap_GetFromName("NONE");
		}
	}

	// create the bounding sphere
	OBJiConsole_CreateBoundingSphere(inObject, console_osd);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void OBJiConsole_UpdatePosition( OBJtObject *inObject)
{
	OBJtOSD_Console			*console_osd;
	float					rot_x;
	float					rot_y;
	float					rot_z;
	MUtEuler				euler;

	// get a pointer to the object osd
	console_osd = (OBJtOSD_Console*)inObject->object_data;

	// convert the rotation to radians
	rot_x = inObject->rotation.x * M3cDegToRad;
	rot_y = inObject->rotation.y * M3cDegToRad;
	rot_z = inObject->rotation.z * M3cDegToRad;

	// update the rotation matrix
	euler.order = MUcEulerOrderXYZs;
	euler.x = rot_x;
	euler.y = rot_y;
	euler.z = rot_z;
	MUrEulerToMatrix(&euler, &console_osd->rotation_matrix);

	if( console_osd->action_marker )
		OBJrConsole_CalculateMarker( inObject );
}

// ----------------------------------------------------------------------
static UUtUns32 OBJiConsole_Read( OBJtObject *inObject, UUtUns16 inVersion, UUtBool inSwapIt, UUtUns8 *inBuffer)
{
	OBJtOSD_Console			*console_osd;
	OBJtOSD_All				osd_all;
	UUtUns32				num_bytes;
	UUtUns32				i;

	num_bytes = 0;

	console_osd = &osd_all.osd.console_osd;

	OBJmGetStringFromBuffer(inBuffer, console_osd->console_class_name,		OBJcMaxNameLength, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, console_osd->id,						UUtUns16, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, console_osd->flags,					UUtUns16, inSwapIt);
	OBJmGetStringFromBuffer(inBuffer, console_osd->screen_inactive,			OBJcMaxNameLength, inSwapIt);
	OBJmGetStringFromBuffer(inBuffer, console_osd->screen_active,			OBJcMaxNameLength, inSwapIt);
	OBJmGetStringFromBuffer(inBuffer, console_osd->screen_triggered,		OBJcMaxNameLength, inSwapIt);

	console_osd->flags &= OBJcConsoleFlag_Persist;

	num_bytes =
		OBJcMaxNameLength +
		sizeof(UUtUns16) +
		sizeof(UUtUns16) +
		OBJcMaxNameLength +
		OBJcMaxNameLength +
		OBJcMaxNameLength;

	// initialize and read the event list
	ONrEventList_Initialize( &console_osd->event_list );
	num_bytes += ONrEventList_Read( &console_osd->event_list, inVersion, inSwapIt, inBuffer );

	// bring the object up to date
	OBJiConsole_SetOSD(inObject, &osd_all);

	ONrEventList_Destroy(&console_osd->event_list);

	// get a pointer to the object osd
	console_osd = (OBJtOSD_Console*) inObject->object_data;

	// read the light data
	for (i = 0; i < console_osd->num_ls_datas; i++)
	{
		OBJtLSData			*ls_data;

		// get a pointer to the ls_data
		ls_data = console_osd->ls_data_array + i;

		// read the data from the buffer
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->index,				UUtUns32, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->light_flags,			UUtUns32, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->filter_color[0],		float, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->filter_color[1],		float, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->filter_color[2],		float, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->light_intensity,		UUtUns32, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->beam_angle,			float, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->field_angle,			float, inSwapIt);

		// set the number of bytes read
		num_bytes += ((sizeof(UUtUns32) * 3) + (sizeof(float) * 5));
	}

	OBJrObject_UpdatePosition(inObject);

	return num_bytes;
}

// ----------------------------------------------------------------------
static UUtError OBJiConsole_Write( OBJtObject *inObject, UUtUns8 *ioBuffer, UUtUns32 *ioBufferSize )
{
	OBJtOSD_Console			*console_osd;
	UUtUns32				bytes_available;
	UUtUns32				i;

	console_osd = (OBJtOSD_Console*)inObject->object_data;

	// set the number of bytes available
	bytes_available = *ioBufferSize;

	// put the geometry name in the buffer
	OBJmWriteStringToBuffer(ioBuffer, console_osd->console_class_name,			OBJcMaxNameLength, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, console_osd->id,							UUtInt16, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, console_osd->flags & OBJcConsoleFlag_Persist,	UUtUns16, bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, console_osd->screen_inactive,			OBJcMaxNameLength, bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, console_osd->screen_active,			OBJcMaxNameLength, bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, console_osd->screen_triggered,		OBJcMaxNameLength, bytes_available, OBJcWrite_Little);

	ONrEventList_Write( &console_osd->event_list, ioBuffer, &bytes_available );

	// put the light data into the buffer
	for (i = 0; i < console_osd->num_ls_datas; i++)
	{
		OBJtLSData			*ls_data;

		ls_data = console_osd->ls_data_array + i;

		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->index, UUtUns32, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->light_flags, UUtUns32, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->filter_color[0], float, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->filter_color[1], float, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->filter_color[2], float, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->light_intensity, UUtUns32, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->beam_angle, float, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->field_angle, float, bytes_available, OBJcWrite_Little);
	}

	// set ioBufferSize to the number of bytes written to the buffer
	*ioBufferSize = *ioBufferSize - bytes_available;

	return UUcError_None;
}
// ----------------------------------------------------------------------
static UUtUns32
OBJiConsole_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	OBJtOSD_Console		*console_osd;
	UUtUns32				size;

	// get a pointer to the console_osd
	console_osd = (OBJtOSD_Console*)inObject->object_data;

	// calculate the number of bytes needed to save the OSD
	size =
		OBJcMaxNameLength +
		sizeof(UUtUns16) +
		sizeof(UUtUns16) +
		OBJcMaxNameLength +
		OBJcMaxNameLength +
		OBJcMaxNameLength +
		(sizeof(OBJtLSData) * console_osd->num_ls_datas);

	size += ONrEventList_GetWriteSize( &console_osd->event_list );

	return size;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OBJiConsole_GetVisible(
	void)
{
	return OBJgConsole_DrawConsole;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiConsole_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static void
OBJiConsole_SetVisible(
	UUtBool					inIsVisible)
{
	OBJgConsole_DrawConsole = inIsVisible;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
static UUtError OBJrConsole_CalculateMarker( const OBJtObject *inObject )
{
	OBJtOSD_Console			*console_osd;
	M3tVector3D				pos;
	M3tVector3D				dir_vector;
	float					dir_angle;
	M3tMatrix4x3			matrix;
	ONtActionMarker*		marker;

	UUmAssert( inObject );
	UUmAssert( ((OBJtOSD_Console*) inObject->object_data)->action_marker );

	// grab the OSD
	console_osd = (OBJtOSD_Console*) inObject->object_data;

	// calculate object position & orientation matrix
	matrix = console_osd->rotation_matrix;
	MUrMatrix_SetTranslation(&matrix, &inObject->position);

	// calculate action vector
	MUrMatrix_MultiplyPoint( &console_osd->console_class->action_vector, &console_osd->rotation_matrix, &dir_vector );

	// calculate action position
	pos.x = console_osd->console_class->action_position.x;
	pos.y = console_osd->console_class->action_position.y;
	pos.z = console_osd->console_class->action_position.z;

	// place position and direction into world space
	MUrMatrix_MultiplyPoint( &pos, &matrix, &pos );

	// get desired facing angle in world radians
	dir_angle = MUrATan2( -dir_vector.x, -dir_vector.z );
	if( dir_angle < 0 )
		dir_angle += M3c2Pi;

	marker = console_osd->action_marker;
	marker->position = pos;
	marker->direction = dir_angle;

	return UUcError_None;
}

static UUtError OBJrConsole_HandleCharacterAction( OBJtObject *inObject, ONtCharacter *inCharacter )
{
	return UUcError_None;
}

static UUtUns16 OBJiConsole_GetID( OBJtObject *inObject )
{
	OBJtOSD_Console			*osd;
	UUmAssert( inObject && inObject->object_data );
	osd = (OBJtOSD_Console*)inObject->object_data;
	return osd->id;
}


UUtError OBJrConsole_OnActivate( OBJtObject *inObject, ONtCharacter* inCharacter )
{
	OBJtOSD_Console			*console_osd;
	UUtError				error;
	UUtBool					console_is_active;
	UUtBool					console_is_triggered;
	M3tPoint3D sound_location;

	UUmAssert( inObject->object_type == OBJcType_Console );

	console_osd = (OBJtOSD_Console*) inObject->object_data;

	console_is_active = (console_osd->flags & OBJcConsoleFlag_Active) ? UUcTrue : UUcFalse;
	console_is_triggered = (console_osd->flags & OBJcConsoleFlag_Triggered) ? UUcTrue : UUcFalse;

	MUmVector_Add(sound_location, console_osd->action_marker->position, inCharacter->actual_position);
	MUmVector_Scale(sound_location, 0.5f);

	if (!console_is_active) {
		SSrImpulse_Play_Simple("console_locked", &sound_location);
	}
	else if (console_is_triggered) {
		SSrImpulse_Play_Simple("console_already_used", &sound_location);
	}
	else {
		SSrImpulse_Play_Simple("console_activate", &sound_location);
		console_osd->flags |= OBJcConsoleFlag_Triggered;

		error = ONrEventList_Execute( &console_osd->event_list, inCharacter );
		UUmAssert( error == UUcError_None );
	}

	return UUcError_None;
}

UUtError OBJrConsole_AddEvent( OBJtOSD_Console *inConsole_osd, ONtEvent *inEvent )
{
	ONrEventList_AddEvent( &inConsole_osd->event_list, inEvent );
	return UUcError_None;
}

UUtError OBJrConsole_DeleteEvent( OBJtOSD_Console *inConsole_osd, UUtUns32 inIndex )
{
	ONrEventList_DeleteEvent( &inConsole_osd->event_list, inIndex );
	return UUcError_None;
}

// ==================================================================================
// activation / deactivation
// ==================================================================================

UUtError OBJrConsole_Activate( OBJtObject *inObject )
{
	OBJtOSD_Console			*console_osd;

	UUmAssert( inObject->object_type == OBJcType_Console );

	console_osd = (OBJtOSD_Console*) inObject->object_data;

	console_osd->flags |= OBJcConsoleFlag_Active;

	return UUcError_None;
}

UUtError OBJrConsole_Deactivate( OBJtObject *inObject )
{
	OBJtOSD_Console			*console_osd;

	UUmAssert( inObject->object_type == OBJcType_Console );

	console_osd = (OBJtOSD_Console*) inObject->object_data;

	console_osd->flags &= ~OBJcConsoleFlag_Active;

	return UUcError_None;
}

// ==================================================================================
// Event handlers
// ==================================================================================

typedef struct OBJtConsoleFindID
{
	UUtUns16 id;
	OBJtObject *object;
} OBJtConsoleFindID;

static UUtBool OBJrConsole_Find_ID_Enum(OBJtObject *inObject, UUtUns32 inUserData)
{
	OBJtOSD_Console *console_osd;
	OBJtConsoleFindID *find_struct = (OBJtConsoleFindID *) inUserData;

	UUmAssert(inObject->object_type == OBJcType_Console);
	UUmAssertReadPtr(find_struct, sizeof(OBJtConsoleFindID));

	console_osd = (OBJtOSD_Console *) inObject->object_data;
	if (console_osd->id == find_struct->id) {
		// found a console with the desired ID
		find_struct->object = inObject;
		return UUcFalse;
	} else {
		// keep looking
		return UUcTrue;
	}
}

OBJtObject *OBJrConsole_GetByID(UUtUns16 inID)
{
	OBJtConsoleFindID find_struct;

	find_struct.id = inID;
	find_struct.object = NULL;
	OBJrObjectType_EnumerateObjects(OBJcType_Console, OBJrConsole_Find_ID_Enum, (UUtUns32) &find_struct);

	return find_struct.object;
}

static UUtBool OBJrConsole_Activate_ID_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Console *console_osd;

	UUmAssert( inObject->object_type == OBJcType_Console );

	console_osd = (OBJtOSD_Console*) inObject->object_data;

	if( inUserData == 0xffff || console_osd->id == inUserData )
	{
		OBJrConsole_Activate( (OBJtObject*) inObject );
	}

	return UUcTrue;
}

void OBJrConsole_Activate_ID( UUtUns16 inID )
{
	if (inID == (UUtUns16) -1) {
		inID = 0xffff;
	}

	OBJrObjectType_EnumerateObjects( OBJcType_Console, OBJrConsole_Activate_ID_Enum, (UUtUns32) inID );

	return;
}

static UUtBool OBJrConsole_Deactivate_ID_Enum(OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtOSD_Console *Console_osd;

	UUmAssert( inObject->object_type == OBJcType_Console );

	Console_osd = (OBJtOSD_Console*) inObject->object_data;

	if( inUserData == 0xffff || Console_osd->id == inUserData )
	{
		OBJrConsole_Deactivate( (OBJtObject*) inObject );
	}

	return UUcTrue;
}

void OBJrConsole_Deactivate_ID( UUtUns16 inID )
{
	if (inID == (UUtUns16) -1) {
		inID = 0xffff;
	}

	OBJrObjectType_EnumerateObjects( OBJcType_Console, OBJrConsole_Deactivate_ID_Enum, (UUtUns32) inID );

	return;
}

static UUtBool OBJrConsole_Reset_ID_Enum(OBJtObject *inObject, UUtUns32 inUserData)
{
	OBJtOSD_Console *console_osd;

	UUmAssert( inObject->object_type == OBJcType_Console );

	console_osd = (OBJtOSD_Console*) inObject->object_data;

	if ((inUserData == 0xffff) || (console_osd->id == inUserData)) {
		OBJiConsole_Reset(inObject);
	}

	return UUcTrue;
}

void OBJrConsole_Reset_ID(UUtUns16 inID)
{
	if (inID == (UUtUns16) -1) {
		inID = 0xffff;
	}

	OBJrObjectType_EnumerateObjects( OBJcType_Console, OBJrConsole_Reset_ID_Enum, (UUtUns32) inID );

	return;
}


// ==================================================================================
// Mechanics handlers
// ==================================================================================

static UUtError OBJiConsole_Reset( OBJtObject *inObject )
{
	OBJtOSD_Console			*console_osd;

	UUmAssert( inObject->object_type == OBJcType_Console );

	console_osd = (OBJtOSD_Console*) inObject->object_data;

	console_osd->flags &= ~OBJcConsoleFlag_Triggered;

	if (console_osd->flags & OBJcConsoleFlag_InitialActive) {
		console_osd->flags |= OBJcConsoleFlag_Active;
	}
	else {
		console_osd->flags &= ~OBJcConsoleFlag_Active;
	}

	return UUcError_None;
}

static UUtError OBJiConsole_LevelEnd( OBJtObject *inObject )
{

	return UUcError_None;
}

static UUtError OBJiConsole_LevelBegin( OBJtObject *inObject )
{
	OBJtOSD_Console			*console_osd;

	UUmAssert( inObject->object_type == OBJcType_Console );

	console_osd = (OBJtOSD_Console*) inObject->object_data;

	if (NULL == console_osd->console_class) {
		COrConsole_Printf("issues with console class %s", console_osd->console_class_name);
		goto exit;
	}

	UUmAssert( !console_osd->action_marker );

	console_osd->action_marker = ONrLevel_ActionMarker_New( );

	UUmAssert( console_osd->action_marker );

	console_osd->action_marker->object = (UUtUns32) inObject;

	OBJrConsole_CalculateMarker( inObject );

	OBJiConsole_Reset( inObject );

exit:
	return UUcError_None;
}


// ----------------------------------------------------------------------
UUtError OBJrConsole_Initialize(void)
{
	UUtError					error;
	OBJtMethods					methods;
	ONtMechanicsMethods			mechanics_methods;

	// initialize globals
	OBJgConsole_DrawConsole				= UUcTrue;

	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));

	// set up the methods structure
	methods.rNew						= OBJiConsole_New;
	methods.rSetDefaults				= OBJiConsole_SetDefaults;
	methods.rDelete						= OBJiConsole_Delete;
	methods.rDraw						= OBJiConsole_Draw;
	methods.rEnumerate					= OBJiConsole_Enumerate;
	methods.rGetBoundingSphere			= OBJiConsole_GetBoundingSphere;
	methods.rOSDGetName					= OBJiConsole_OSDGetName;
	methods.rOSDSetName					= OBJiConsole_OSDSetName;
	methods.rIntersectsLine				= OBJiConsole_IntersectsLine;
	methods.rUpdatePosition				= OBJiConsole_UpdatePosition;
	methods.rGetOSD						= OBJiConsole_GetOSD;
	methods.rGetOSDWriteSize			= OBJiConsole_GetOSDWriteSize;
	methods.rSetOSD						= OBJiConsole_SetOSD;
	methods.rRead						= OBJiConsole_Read;
	methods.rWrite						= OBJiConsole_Write;

	// class methods
	methods.rSearch						= OBJiConsole_Search;
	methods.rGetClassVisible			= OBJiConsole_GetVisible;
	methods.rSetClassVisible			= OBJiConsole_SetVisible;

	// mechanics methods
	mechanics_methods.rInitialize		= OBJiConsole_LevelBegin;
	mechanics_methods.rTerminate		= OBJiConsole_LevelEnd;
	mechanics_methods.rReset			= OBJiConsole_Reset;
	mechanics_methods.rUpdate			= NULL;
	mechanics_methods.rGetID			= OBJiConsole_GetID;

	mechanics_methods.rClassLevelBegin 	= NULL;
	mechanics_methods.rClassLevelEnd 	= NULL;
	mechanics_methods.rClassReset		= NULL;
	mechanics_methods.rClassUpdate		= NULL;

	// register the console methods
	error = ONrMechanics_Register( OBJcType_Console, OBJcTypeIndex_Console, "Console", sizeof(OBJtOSD_Console), &methods, OBJcObjectGroupFlag_CanSetName, &mechanics_methods );
	UUmError_ReturnOnError(error);

	// console is initially visible
	OBJiConsole_SetVisible(UUcTrue);

	return UUcError_None;
}
