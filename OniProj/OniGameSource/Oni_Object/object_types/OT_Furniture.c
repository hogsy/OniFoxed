// ======================================================================
// OT_Furniture.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_EnvParticle.h"
#include "EulerAngles.h"
#include "Oni_Object.h"
#include "Oni_Object_Private.h"
#include "Oni_BinaryData.h"

// ======================================================================
// defines
// ======================================================================
//#define OBJcFurniturePrefix			"F_"
#define OBJcFurniturePrefix			""

// ======================================================================
// globals
// ======================================================================
static UUtBool				OBJgFurniture_DrawFurniture = UUcTrue;

static UUtError
OBJiFurniture_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD);


// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiFurniture_CreateBoundingSphere(
	OBJtObject				*inObject,
	OBJtOSD_Furniture		*inOSD)
{
	UUtUns32				i;
	M3tBoundingSphere		*sphere;
	M3tPoint3D				new_center;
	float					new_radius;
	
	new_center.x = 0.0f;
	new_center.y = 0.0f;
	new_center.z = 0.0f;
	
	// calculate the center of the sphere
	for (i = 0; i < inOSD->furn_geom_array->num_furn_geoms; i++)
	{
		sphere = &inOSD->furn_geom_array->furn_geom[i].geometry->pointArray->boundingSphere;
		MUmVector_Increment(new_center, sphere->center);
	}
	
	new_center.x /= inOSD->furn_geom_array->num_furn_geoms;
	new_center.y /= inOSD->furn_geom_array->num_furn_geoms;
	new_center.z /= inOSD->furn_geom_array->num_furn_geoms;
	
	// caculate the new radius
	new_radius = 0.0f;
	for (i = 0; i < inOSD->furn_geom_array->num_furn_geoms; i++)
	{
		M3tVector3D			vector;			
		float				temp_radius;
		
		sphere = &inOSD->furn_geom_array->furn_geom[i].geometry->pointArray->boundingSphere;
		MUmVector_Subtract(vector, new_center, sphere->center);
		temp_radius = MUrVector_Length(&vector) + sphere->radius;
		
		new_radius = UUmMax(new_radius, temp_radius);
	}
	
	// set the bounding sphere
	inOSD->bounding_sphere.center = new_center;
	inOSD->bounding_sphere.radius = new_radius;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiFurniture_Read_Version1(
	OBJtObject				*inObject,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_Furniture		*furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;

	// copy the data into the osd
	UUrString_Copy_Length(
		furniture_osd->furn_geom_name,
		(char*)inBuffer,
		BFcMaxFileNameLength,
		BFcMaxFileNameLength);

	return BFcMaxFileNameLength;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiFurniture_Read_Version2(
	OBJtObject				*inObject,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_Furniture		*furniture_osd;
	UUtUns32				bytes_read;
	UUtUns32				i;
	
	// read the version 1 data
	bytes_read = OBJiFurniture_Read_Version1(inObject, inSwapIt, inBuffer);
	
	// move to the beginning of the version 2 data
	inBuffer += bytes_read;
	
	// get a pointer to the object osd
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;

	// read the light data
	for (i = 0; i < furniture_osd->num_ls_datas; i++)
	{
		OBJtLSData			*ls_data;
		
		// get a pointer to the ls_data
		ls_data = furniture_osd->ls_data_array + i;
		
		// read the data from the buffer
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->index, UUtUns32, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->light_flags, UUtUns32, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->filter_color[0], float, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->filter_color[1], float, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->filter_color[2], float, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->light_intensity, UUtUns32, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->beam_angle, float, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, ls_data->field_angle, float, inSwapIt);
		
		// set the number of bytes read
		bytes_read += ((sizeof(UUtUns32) * 3) + (sizeof(float) * 5));
	}
	
	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiFurniture_Read_Version14(
	OBJtObject				*inObject,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_Furniture		*furniture_osd;
	UUtUns32				bytes_read;
	
	// read the version 2 data
	bytes_read = OBJiFurniture_Read_Version2(inObject, inSwapIt, inBuffer);
	
	// move to the beginning of the version 14 data
	inBuffer += bytes_read;
	
	// get a pointer to the object osd
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;

	// read the tag name
	bytes_read += OBDmGetStringFromBuffer(inBuffer, furniture_osd->furn_tag, EPcParticleTagNameLength + 1, inSwapIt);
	
	return bytes_read;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiFurniture_Delete(
	OBJtObject				*inObject)
{
	OBJtOSD_Furniture		*furniture_osd;
	EPtEnvParticle			*particle;
	UUtUns32				itr;

	// get a pointer to the object osd
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	
	// delete the memory allocated for the ls_data_array
	if (furniture_osd->ls_data_array)
	{
		UUrMemory_Block_Delete(furniture_osd->ls_data_array);
		furniture_osd->ls_data_array = NULL;
		furniture_osd->num_ls_datas = 0;
	}

	// delete any particles
	if (furniture_osd->num_particles > 0) {
		for (itr = 0, particle = furniture_osd->particle; itr < furniture_osd->num_particles; itr++, particle++) {
			if (particle->flags & EPcParticleFlag_InUse) {
				EPrDelete(particle);
			}
		}
		UUrMemory_Block_Delete(furniture_osd->particle);
	}
}

// ----------------------------------------------------------------------
void OBJrFurniture_DrawArray( OBJtFurnGeomArray* inGeometryArray, UUtUns32 inDrawFlags )
{
	UUtUns32				flags;
	M3tBoundingBox			bBox;
	UUtUns32				i;

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
		M3rGeometry_Draw( inGeometryArray->furn_geom[i].geometry );
	}

	return;
}

// ----------------------------------------------------------------------
static void
OBJiFurniture_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
#if TOOL_VERSION
	OBJtOSD_Furniture		*furniture_osd;
	M3tBoundingBox			bBox;
	UUtUns32				i;
	
	if (OBJgFurniture_DrawFurniture == UUcFalse) { return; }
	
	// don't draw when the non-occluding geometry is hidden
	if (AKgDraw_Occl == UUcTrue) { return; }
	
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	
	if (furniture_osd->furn_geom_array == NULL)		return;
	
	if (!(inObject->flags & OBJcObjectFlag_Gunk))
	{
		for (i = 0; i < furniture_osd->furn_geom_array->num_furn_geoms; i++)
		{
			UUtUns32			flags;
			
			// check the visibility flags
			flags = furniture_osd->furn_geom_array->furn_geom[i].gq_flags;
			if (((flags & AKcGQ_Flag_Invisible) != 0) && (AKgDraw_Invis == UUcFalse)) { continue; }
			
			// draw the geometry
			AKrEnvironment_DrawIfVisible(furniture_osd->furn_geom_array->furn_geom[i].geometry, &furniture_osd->matrix);
		}
	}
	
	// draw the rotation rings if this is the selected object
	if (inDrawFlags & OBJcDrawFlag_Selected)
	{
		M3rMatrixStack_Push();
		M3rMatrixStack_Multiply(&furniture_osd->matrix);
		M3rGeom_State_Commit();
		
		for (i = 0; i < furniture_osd->furn_geom_array->num_furn_geoms; i++)
		{
			// draw the bounding box if this is the selected object
			M3rMinMaxBBox_To_BBox(
				&furniture_osd->furn_geom_array->furn_geom[i].geometry->pointArray->minmax_boundingBox,
				&bBox);
			
			M3rBBox_Draw_Line(&bBox, IMcShade_White);
		}
		
		OBJrObjectUtil_DrawRotationRings(inObject, &furniture_osd->bounding_sphere, inDrawFlags);
		
		M3rMatrixStack_Pop();
	}
#endif

	return;
}

// ----------------------------------------------------------------------
static UUtError
OBJiFurniture_Enumerate(
	OBJtObject						*inObject,
	OBJtEnumCallback_ObjectName		inEnumCallback,
	UUtUns32						inUserData)
{
	UUtError						error;
	
	error =
		OBJrObjectUtil_EnumerateTemplate(
//			inObject->object_type,
			OBJcFurniturePrefix,
			OBJcTemplate_FurnGeomArray,
			inEnumCallback,
			inUserData);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiFurniture_GetBoundingSphere(
	const OBJtObject		*inObject,
	M3tBoundingSphere		*outBoundingSphere)
{
	OBJtOSD_Furniture		*furniture_osd;
	
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;

	*outBoundingSphere = furniture_osd->bounding_sphere;
}

// ----------------------------------------------------------------------
static void
OBJiFurniture_OSDGetName(
	const OBJtOSD_All		*inOSD,
	char					*outName,
	UUtUns32				inNameLength)
{
	const OBJtOSD_Furniture		*furniture_osd = &inOSD->osd.furniture_osd;

	UUrString_Copy(outName, furniture_osd->furn_geom_name, inNameLength);	
	if (furniture_osd->furn_tag[0] != '\0') {
		UUrString_Cat(outName, "_", inNameLength);
		UUrString_Cat(outName, furniture_osd->furn_tag, inNameLength);
	}

	return;
}

// ----------------------------------------------------------------------
static void
OBJiFurniture_OSDSetName(
	OBJtOSD_All		*inOSD,
	const char		*inName)
{
//	OBJtOSD_Furniture *furniture_osd = &inOSD->osd.furniture_osd;

//	UUrString_Copy(furniture_osd->furn_geom_name, inName, sizeof(inNameLength));	

	return;
}

// ----------------------------------------------------------------------
static void
OBJiFurniture_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	OBJtOSD_Furniture		*furniture_osd;
	
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	
	outOSD->osd.furniture_osd = *furniture_osd;

	// copies of the OSD have no particles in them
	outOSD->osd.furniture_osd.num_particles = 0;
	outOSD->osd.furniture_osd.particle = NULL;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiFurniture_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	OBJtOSD_Furniture		*furniture_osd;
	UUtUns32				size;
	
	// get a pointer to the furniture_osd
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	
	// calculate the number of bytes needed to save the OSD
	size = BFcMaxFileNameLength + (sizeof(OBJtLSData) * furniture_osd->num_ls_datas) + (EPcParticleTagNameLength + 1);
		
	return size;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiFurniture_IntersectsLine(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inStartPoint,
	const M3tPoint3D		*inEndPoint)
{
	M3tBoundingSphere		sphere;
	UUtBool					result;
	OBJtOSD_Furniture		*furniture_osd;

	UUrMemory_Clear(&sphere, sizeof(M3tBoundingSphere));
	
	// if the line collides with the bounding sphere, test to see if the line
	// collides with the bounding box of the geometries
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	if (furniture_osd->furn_geom_array == NULL) { return UUcFalse; }
	
	// get the bounding sphere
	OBJrObject_GetBoundingSphere(inObject, &sphere);
	
	// put the sphere at the proper place in the world
	MUrMatrix_MultiplyPoint(&sphere.center, &furniture_osd->matrix, &sphere.center);
	
	// do the fast test to see if the line is colliding with the bounding sphere
	result = CLrSphere_Line(inStartPoint, inEndPoint, &sphere);
	if (result == UUcTrue)
	{
		UUtUns32				i;
		M3tPoint3D				start_point;
		M3tPoint3D				end_point;
		M3tMatrix4x3			inverse_matrix;
		
		result = UUcFalse;
		
		// calculate the inverse of the location matrix
		MUrMatrix_Inverse(&furniture_osd->matrix, &inverse_matrix);
		
		// calculate a start point and an end poing in object space
		MUrMatrix_MultiplyPoint(inStartPoint, &inverse_matrix, &start_point);
		MUrMatrix_MultiplyPoint(inEndPoint, &inverse_matrix, &end_point);
		
		// check the bounding box of each geometry to see if the line intersects the geometry
		for (i = 0; i < furniture_osd->furn_geom_array->num_furn_geoms; i++)
		{
			result =
				CLrBox_Line(
					&furniture_osd->furn_geom_array->furn_geom[i].geometry->pointArray->minmax_boundingBox,
					&start_point,
					&end_point);
			if (result == UUcTrue) { break; }
		}
		
		return result;
	}
	
	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtError
OBJiFurniture_SetDefaults(
	OBJtOSD_All			*outOSD)
{
	UUtError				error;
	void				*instances[OBJcMaxInstances];
	UUtUns32			num_instances;
	UUtUns32			prefix_length;
	UUtUns32			i;
		
	// clear the osd
	UUrMemory_Clear(&outOSD->osd.furniture_osd, sizeof(OBJtOSD_Furniture));
	
	// get a list of instances of the class
	error =
		TMrInstance_GetDataPtr_List(
			OBJcTemplate_FurnGeomArray,
			OBJcMaxInstances,
			&num_instances,
			instances);
	UUmError_ReturnOnError(error);
	
	prefix_length = strlen(OBJcFurniturePrefix);
	
	// copy the name of the first furniture geometry array instance into 
	// the osd_all.
	for (i = 0; i < num_instances; i++)
	{
		char			*instance_name;
		
		instance_name = TMrInstance_GetInstanceName(instances[i]);
		
		if (strncmp(instance_name, OBJcFurniturePrefix, prefix_length) == 0)
		{
			UUrString_Copy(
				outOSD->osd.furniture_osd.furn_geom_name,
				(instance_name + prefix_length),
				BFcMaxFileNameLength);
			break;
		}
	}
	
	// no tag name
	outOSD->osd.furniture_osd.furn_tag[0] = '\0';

	if (i == num_instances)
	{
		return UUcError_Generic;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiFurniture_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_All				osd_all;
	OBJtOSD_Furniture		*furn_osd = (OBJtOSD_Furniture *) inObject->object_data;
	UUtError				error;
	
	if (inOSD == NULL)
	{
		error = OBJiFurniture_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);
				
		// send osd_all to OBJiFurniture_SetOSD()
		inOSD = &osd_all;
	}
	
	// set the object specific data and position
	OBJiFurniture_SetOSD(inObject, inOSD);
	OBJrObject_UpdatePosition(inObject);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiFurniture_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	UUtUns32				read_bytes;
	
	if(inVersion >= OBJcVersion_14) {
		read_bytes = OBJiFurniture_Read_Version14(inObject, inSwapIt, inBuffer);

	} else if (inVersion >= 2) {
		read_bytes = OBJiFurniture_Read_Version2(inObject, inSwapIt, inBuffer);

	} else {
		read_bytes = OBJiFurniture_Read_Version1(inObject, inSwapIt, inBuffer);
	}

	// set the osd and update the position
	OBJiFurniture_SetOSD(inObject, (OBJtOSD_All *) inObject->object_data);
	OBJrObject_UpdatePosition(inObject);

	return read_bytes;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiFurniture_CreateParticles(
	OBJtObject				*inObject,
	UUtUns32				inUserData)
{
	OBJtOSD_Furniture		*furniture_osd;
	char					particle_tag[2 * EPcParticleTagNameLength + 2];
	EPtEnvParticleArray		*particle_array;
	EPtEnvParticle			*particle;
	UUtUns32				current_time, itr;

	UUmAssert(inObject->object_type == OBJcType_Furniture);
	furniture_osd = (OBJtOSD_Furniture *) inObject->object_data;

	furniture_osd->num_particles = 0;
	furniture_osd->particle = NULL;

	if (furniture_osd->furn_geom_array == NULL) {
		goto exit;
	}

	particle_array = furniture_osd->furn_geom_array->particle_array;
	if (particle_array == NULL) {
		goto exit;
	}

	furniture_osd->num_particles = particle_array->numParticles;
	if (furniture_osd->num_particles == 0) {
		goto exit;
	}

	furniture_osd->particle = UUrMemory_Block_New(furniture_osd->num_particles * sizeof(EPtEnvParticle));
	if (furniture_osd->particle == NULL) {
		furniture_osd->num_particles = 0;
		goto exit;
	}

	current_time = ONrGameState_GetGameTime();

	// copy the particle array
	UUrMemory_MoveFast(particle_array->particle, furniture_osd->particle,
						furniture_osd->num_particles * sizeof(EPtEnvParticle));
	for (itr = 0, particle = furniture_osd->particle; itr < furniture_osd->num_particles; itr++, particle++) {
		particle->owner_type = EPcParticleOwner_Furniture;
		particle->owner = (void *) furniture_osd;
		particle->dynamic_matrix = &furniture_osd->matrix;

		if (furniture_osd->furn_tag[0] != '\0') {
			// prepend the furniture's tag name to the particle... truncate if necessary to fit
			sprintf(particle_tag, "%s_%s", furniture_osd->furn_tag, particle->tagname);
			UUrString_Copy(particle->tagname, particle_tag, EPcParticleTagNameLength + 1);
		}

		// don't actually register particles for those furniture objects that have been
		// dumped out as gunk. this prevents them from being created, and from showing up
		// in tag-based commands
		if ((inObject->flags & OBJcObjectFlag_Gunk) == 0) {
			EPrNew(particle, current_time);
		}
	}

exit:
	// keep enumerating
	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtError
OBJiFurniture_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	UUtError				error;
	OBJtOSD_Furniture		*furniture_osd;
	OBJtFurnGeomArray		*furn_geom_array;
	char					furn_geom_name[BFcMaxFileNameLength + 1];
	EPtEnvParticle			*particle;
	UUtUns32				size;
	UUtUns32				i, itr;
	
	UUmAssert(inOSD);
	
	// get a pointer to the object osd
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;

	// get the new furn_geom_array name
	UUrString_Copy(
		furn_geom_name,
		OBJcFurniturePrefix,
		BFcMaxFileNameLength);

	UUrString_Cat(
		furn_geom_name,
		inOSD->osd.furniture_osd.furn_geom_name,
		BFcMaxFileNameLength);
	
	// get a pointer to the new furn_geom_array
	error =
		TMrInstance_GetDataPtr(
			OBJcTemplate_FurnGeomArray,
			furn_geom_name,
			&furn_geom_array);
	if (error != UUcError_None) {
		UUrDebuggerMessage("failed to locate instance %s\n", (UUtUns32) furn_geom_name, 0, 0);
		return error;
	}
	
	// don't update anything if the values haven't changed
	if ((furn_geom_array == furniture_osd->furn_geom_array) &&
		(UUmString_IsEqual(furniture_osd->furn_tag, inOSD->osd.furniture_osd.furn_tag))) {
		return UUcError_None;
	}
	
	// save the pointer to the new furn_geom_array
	furniture_osd->furn_geom_array = furn_geom_array;
	
	// save the new furn_geom_name
	UUrString_Copy(
		furniture_osd->furn_geom_name,
		inOSD->osd.furniture_osd.furn_geom_name,
		BFcMaxFileNameLength);
	
	// save the new furn_tag
	UUrString_Copy(
		furniture_osd->furn_tag,
		inOSD->osd.furniture_osd.furn_tag,
		EPcParticleTagNameLength + 1);
	
	// delete the old ls_data_array
	if (furniture_osd->ls_data_array)
	{
		UUrMemory_Block_Delete(furniture_osd->ls_data_array);
		furniture_osd->ls_data_array = NULL;
		furniture_osd->num_ls_datas = 0;
	}
	
	// delete the old particle array
	if (furniture_osd->num_particles > 0)
	{
		for (itr = 0, particle = furniture_osd->particle; itr < furniture_osd->num_particles; itr++, particle++) {
			if (particle->flags & EPcParticleFlag_InUse) {
				EPrDelete(particle);
			}
		}
		UUrMemory_Block_Delete(furniture_osd->particle);
		furniture_osd->particle = NULL;
	}

	// calculate the size for the ls_data_array and the number of OBJtLSData's in it
	size = 0;
	furniture_osd->num_ls_datas = 0;
	for (i = 0; i < furniture_osd->furn_geom_array->num_furn_geoms; i++)
	{
		if (furniture_osd->furn_geom_array->furn_geom[i].ls_data == NULL) { continue; }
		
		size += sizeof(OBJtLSData);
		furniture_osd->num_ls_datas++;
	}
	
	if (size > 0)
	{
		// allocate memory for the ls_data_array
		furniture_osd->ls_data_array = (OBJtLSData*)UUrMemory_Block_NewClear(size);
		UUmError_ReturnOnNull(furniture_osd->ls_data_array);
	
		// if inOSD has a ls_data_array, then copy the data from it into the furniture->ls_data_array
		// otherwise copy the ones from the furn_geom_array
		if (inOSD->osd.furniture_osd.ls_data_array != NULL)
		{
			// copy the data from the inOSD ls_data_array
			UUrMemory_MoveFast(
				inOSD->osd.furniture_osd.ls_data_array,
				furniture_osd->ls_data_array,
				size);
		}
		else
		{
			UUtUns32		j;
			
			// copy the data from the OBJtLSData template instance in the furn_geom
			for (i = 0, j = 0; i < furniture_osd->furn_geom_array->num_furn_geoms; i++)
			{
				if (furniture_osd->furn_geom_array->furn_geom[i].ls_data == NULL) { continue; }
			
				UUrMemory_MoveFast(
					furniture_osd->furn_geom_array->furn_geom[i].ls_data,
					(furniture_osd->ls_data_array + j),
					sizeof(OBJtLSData));
					
				j++;
			}
		}
	}

	// allocate a new particle array from the furniture geom array, if one exists.
	// don't do this if we have been turned into gunk!
	if ((furniture_osd->furn_geom_array->particle_array == NULL) || (!EPgCreatedParticles)) {
		furniture_osd->num_particles = 0;
		furniture_osd->particle = NULL;
	} else {
		OBJiFurniture_CreateParticles(inObject, 0);
	}

	// create the bounding sphere
	OBJiFurniture_CreateBoundingSphere(inObject, furniture_osd);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiFurniture_UpdatePosition(
	OBJtObject				*inObject)
{
	OBJtOSD_Furniture		*furniture_osd;
	float					rot_x;
	float					rot_y;
	float					rot_z;
	MUtEuler				euler;
	
	// get a pointer to the object osd
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	
	// convert the rotation to radians
	rot_x = inObject->rotation.x * M3cDegToRad;
	rot_y = inObject->rotation.y * M3cDegToRad;
	rot_z = inObject->rotation.z * M3cDegToRad;
	
	// update the rotation component of the matrix
	euler.order = MUcEulerOrderXYZs;
	euler.x = rot_x;
	euler.y = rot_y;
	euler.z = rot_z;
	MUrEulerToMatrix(&euler, &furniture_osd->matrix);
	
	// set the translation component of the matrix
	MUrMatrix_SetTranslation(&furniture_osd->matrix, &inObject->position);
}

// ----------------------------------------------------------------------
static UUtError
OBJiFurniture_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	OBJtOSD_Furniture		*furniture_osd;
	UUtUns32				bytes_available;
	UUtUns32				i;
	
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	
	// set the number of bytes available
	bytes_available = *ioBufferSize;
		
	// put the geometry name in the buffer
	UUrString_Copy_Length(
		(char*)ioBuffer,
		furniture_osd->furn_geom_name,
		*ioBufferSize,
		BFcMaxFileNameLength);
	ioBuffer += BFcMaxFileNameLength;
	bytes_available -= BFcMaxFileNameLength;
	
	// put the light data into the buffer
	for (i = 0; i < furniture_osd->num_ls_datas; i++)
	{
		OBJtLSData			*ls_data;
		
		ls_data = furniture_osd->ls_data_array + i;
		
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->index, UUtUns32, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->light_flags, UUtUns32, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->filter_color[0], float, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->filter_color[1], float, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->filter_color[2], float, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->light_intensity, UUtUns32, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->beam_angle, float, bytes_available, OBJcWrite_Little);
		OBDmWrite4BytesToBuffer(ioBuffer, ls_data->field_angle, float, bytes_available, OBJcWrite_Little);
	}
	
	// write the tag name
	OBDmWriteStringToBuffer(ioBuffer, furniture_osd->furn_tag, (EPcParticleTagNameLength + 1), bytes_available, OBJcWrite_Little);

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
OBJiFurniture_GetVisible(
	void)
{
	return OBJgFurniture_DrawFurniture;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiFurniture_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static void
OBJiFurniture_SetVisible(
	UUtBool					inIsVisible)
{
	OBJgFurniture_DrawFurniture = inIsVisible;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBJrFurniture_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;
	
	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));
	
	// set up the methods structure
	methods.rNew				= OBJiFurniture_New;
	methods.rSetDefaults		= OBJiFurniture_SetDefaults;
	methods.rDelete				= OBJiFurniture_Delete;
	methods.rDraw				= OBJiFurniture_Draw;
	methods.rEnumerate			= OBJiFurniture_Enumerate;
	methods.rGetBoundingSphere	= OBJiFurniture_GetBoundingSphere;
	methods.rOSDGetName			= OBJiFurniture_OSDGetName;
	methods.rOSDSetName			= OBJiFurniture_OSDSetName;
	methods.rIntersectsLine		= OBJiFurniture_IntersectsLine;
	methods.rUpdatePosition		= OBJiFurniture_UpdatePosition;
	methods.rGetOSD				= OBJiFurniture_GetOSD;
	methods.rGetOSDWriteSize	= OBJiFurniture_GetOSDWriteSize;
	methods.rSetOSD				= OBJiFurniture_SetOSD;
	methods.rRead				= OBJiFurniture_Read;
	methods.rWrite				= OBJiFurniture_Write;
	methods.rGetClassVisible	= OBJiFurniture_GetVisible;
	methods.rSearch				= OBJiFurniture_Search;
	methods.rSetClassVisible	= OBJiFurniture_SetVisible;
	
	// register the furniture methods
	error =
		OBJrObjectGroup_Register(
			OBJcType_Furniture,
			OBJcTypeIndex_Furniture,
			"Furniture",
			sizeof(OBJtOSD_Furniture),
			&methods,
			OBJcObjectGroupFlag_None);
	UUmError_ReturnOnError(error);
	
	// furniture is initially visible
	OBJiFurniture_SetVisible(UUcTrue);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrFurniture_CreateParticles(
	void)
{
	OBJrObjectType_EnumerateObjects(OBJcType_Furniture, OBJiFurniture_CreateParticles, 0);

	return UUcError_None;
}
