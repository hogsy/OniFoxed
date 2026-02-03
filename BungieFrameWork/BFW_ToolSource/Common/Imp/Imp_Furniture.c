// ======================================================================
// Imp_Furniture.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"

#include "Imp_Common.h"
#include "Imp_ParseEnvFile.h"
#include "Imp_Env2.h"
#include "Imp_Env2_Private.h"
#include "Imp_Model.h"
#include "Imp_Furniture.h"
#include "Imp_Texture.h"
#include "Imp_EnvParticle.h"

#include "EnvFileFormat.h"

#include "Oni_Object_Private.h"

// ======================================================================
// constants
// ======================================================================

#define IMPcFurniture_MaxParticles		32

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
IMPrFurniture_GetLightData(
	MXtNode					*inNode,
	UUtUns32				inIndex,
	OBJtFurnGeom			*ioFurnGeom)
{
	UUtError				error;
	GRtGroup_Context		*groupContext;
	GRtGroup				*group;
	GRtElementType			groupType;
	GRtGroup				*ls_group;

	IMPtLS_LightType		light_type;
	IMPtLS_Distribution		light_distribution;

	// init
	groupContext = NULL;

	// make sure there is user data to process
	if ((inNode->userDataCount == 0) || (inNode->userData[0] == '\0'))
	{
		goto cleanup;
	}

	// create a GRtGroup from the user data
	error =
		GRrGroup_Context_NewFromString(
			inNode->userData,
			gPreferencesGroup,
			NULL,
			&groupContext,
			&group);
	if (error != UUcError_None) { goto cleanup; }

	// get the ls group from the user data group
	error =
		GRrGroup_GetElement(
			group,
			"ls",
			&groupType,
			&ls_group);
	if (error != UUcError_None) { goto cleanup; }

	// create the OBJtLSData instance
	error =
		TMrConstruction_Instance_Renew(
			OBJcTemplate_LSData,
			inNode->name,
			0,
			&ioFurnGeom->ls_data);
	if (error != UUcError_None) { goto cleanup; }

	// init
	ioFurnGeom->ls_data->index = inIndex;
	ioFurnGeom->ls_data->light_flags = OBJcLightFlag_HasLight;

	// process the user data
	error =
		IMPrEnv_GetLSData(
			ls_group,
			ioFurnGeom->ls_data->filter_color,
			&light_type,
			&light_distribution,
			&ioFurnGeom->ls_data->light_intensity,
			&ioFurnGeom->ls_data->beam_angle,
			&ioFurnGeom->ls_data->field_angle);
	if (error != UUcError_None) { goto cleanup; }

	// set the light type flags
	if (light_type == IMPcLS_LightType_Area)
	{
		ioFurnGeom->ls_data->light_flags |= OBJcLightFlag_Type_Area;
	}
	else if (light_type == IMPcLS_LightType_Linear)
	{
		ioFurnGeom->ls_data->light_flags |= OBJcLightFlag_Type_Linear;
	}
	else if (light_type == IMPcLS_LightType_Point)
	{
		ioFurnGeom->ls_data->light_flags |= OBJcLightFlag_Type_Point;
	}

	// set the light distribution flags
	if (light_distribution == IMPcLS_Distribution_Diffuse)
	{
		ioFurnGeom->ls_data->light_flags |= OBJcLightFlag_Dist_Diffuse;
	}
	else if (light_distribution == IMPcLS_Distribution_Spot)
	{
		ioFurnGeom->ls_data->light_flags |= OBJcLightFlag_Dist_Spot;
	}

cleanup:
	if (groupContext)
	{
		GRrGroup_Context_Delete(groupContext);
	}
}

// ----------------------------------------------------------------------
static UUtError
IMPiAddFurniture(
	BFtFileRef				*inDirectoryRef)
{
	UUtError				error;
	BFtFileIterator			*file_iterator;

	// create the file iterator for the directory
	error =
		BFrDirectory_FileIterator_New(
			inDirectoryRef,
			NULL,
			".ENV",
			&file_iterator);
	IMPmError_ReturnOnError(error);

	while (1)
	{
		MXtHeader			*header;
		MXtMarker			*curMarker;
		BFtFileRef			file_ref;
		OBJtFurnGeomArray	*furn_geom_array;
		char				name[BFcMaxFileNameLength];
		char				*textureName;
		char				mungedTextureName[BFcMaxFileNameLength];
		UUtUns32			i;
		UUtUns32			num_geoms, markerItr;
		UUtUns32			geom_index;
		UUtUns32			m;
		UUtUns32			num_particles;
		EPtEnvParticle		particle[IMPcFurniture_MaxParticles];

		// get file ref for the file
		error = BFrDirectory_FileIterator_Next(file_iterator, &file_ref);
		if (error != UUcError_None) { break; }

		// init
		header = NULL;
		furn_geom_array = NULL;
		num_particles = 0;

		// parse the .env file
		error = Imp_ParseEnvFile(&file_ref, &header);
		IMPmError_ReturnOnError(error);

		// copy the file name into name
		UUrString_Copy(name, BFrFileRef_GetLeafName(&file_ref), BFcMaxFileNameLength);
		UUrString_StripExtension(name);

		// calculate the number of geometries
		num_geoms = 0;
		for (i = 0; i < header->numNodes; i++)
		{
			num_geoms += header->nodes[i].numMaterials;
		}

		// build the geometry array instance
		error =
			TMrConstruction_Instance_Renew(
				OBJcTemplate_FurnGeomArray,
				name,
				num_geoms,
				&furn_geom_array);
		IMPmError_ReturnOnError(error);

		// initialize the furn_geoms and get the flag and light data
		// yes this is a bit inneficient, but it is import time and won't be that big
		// of a deal
		for (i = 0, geom_index = 0; i < header->numNodes; i++)
		{
			for (m = 0; m < header->nodes[i].numMaterials; m++, geom_index++)
			{
				// initialize the furn_geom
				furn_geom_array->furn_geom[geom_index].gq_flags = AKcGQ_Flag_None;
				furn_geom_array->furn_geom[geom_index].geometry = NULL;
				furn_geom_array->furn_geom[geom_index].ls_data = NULL;

				// get the GQ flags
				furn_geom_array->furn_geom[geom_index].gq_flags =
					IMPrEnv_GetNodeFlags(&header->nodes[i]);

				// get the light data
				IMPrFurniture_GetLightData(
					&header->nodes[i],
					i,
					&furn_geom_array->furn_geom[geom_index]);
			}
		}

		for (i = 0, geom_index = 0; i < header->numNodes; i++)
		{
			for (m = 0; m < header->nodes[i].numMaterials; m++, geom_index++)
			{
				char			geom_name[BFcMaxFileNameLength];
				M3tGeometry		*geometry;

				// set up the name
				sprintf(geom_name, "%s_%d", name, geom_index);

				// build the geometry instance
				error =
					TMrConstruction_Instance_Renew(
						M3cTemplate_Geometry,
						geom_name,
						0,
						&geometry);
				IMPmError_ReturnOnError(error);

				geometry->animation = NULL;

				// put the tris and quads associated with material m into a geometry
				Imp_NodeMaterial_To_Geometry_ApplyMatrix(&header->nodes[i], (UUtUns16)m, geometry);

				// set the geometry
				furn_geom_array->furn_geom[geom_index].geometry = geometry;

				// get the texture name
				textureName = header->nodes[i].materials[m].maps[MXcMapping_DI].name;

				// strip off the extension
				UUrString_Copy(mungedTextureName, textureName, BFcMaxFileNameLength);
				UUrString_Capitalize(mungedTextureName, BFcMaxFileNameLength);
				UUrString_StripExtension(mungedTextureName);
				UUmAssert(strchr(mungedTextureName, '.') == NULL);

				// get a placeholder to the texture map
				geometry->baseMap = M3rTextureMap_GetPlaceholder(mungedTextureName);
				UUmAssert(geometry->baseMap);

				// look for environmental-particle markers.
				for (markerItr = 0, curMarker = header->nodes[i].markers; markerItr < header->nodes[i].numMarkers;
					markerItr++, curMarker++)
				{
					if (num_particles >= IMPcFurniture_MaxParticles)
						break;

					if (!UUrString_HasPrefix(curMarker->name, IMPcEnvParticleMarkerPrefix)) {
						continue;
					}

					error = IMPrParse_EnvParticle(&curMarker->matrix, curMarker->userDataCount, curMarker->userData,
													curMarker->name, header->nodes[i].name, name, &particle[num_particles]);
					if (error == UUcError_None) {
						num_particles++;

						if (num_particles >= IMPcFurniture_MaxParticles) {
							Imp_PrintWarning("IMPiAddFurniture: %s exceeds max number of particles (%d)",
											name, IMPcFurniture_MaxParticles);
						}
					}
				}
			}

			IMPrEnv_CreateInstance_EnvParticles(num_particles, particle, &furn_geom_array->particle_array);
		}

		// delete the header
		Imp_EnvFile_Delete(header);
		header = NULL;
	}

	BFrDirectory_FileIterator_Delete(file_iterator);
	file_iterator = NULL;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
IMPiAddFurnitureTextures(
	BFtFileRef			*inDirectoryRef)
{
	UUtError				error;
	BFtFileIterator			*file_iterator;

	// create the file iterator for the directory
	error =
		BFrDirectory_FileIterator_New(
			inDirectoryRef,
			NULL,
			NULL,
			&file_iterator);
	IMPmError_ReturnOnError(error);

	while (1)
	{
		BFtFileRef			file_ref;
		char				name[BFcMaxFileNameLength];
		TMtPlaceHolder		texture_ref;

		// get file ref for the file
		error = BFrDirectory_FileIterator_Next(file_iterator, &file_ref);
		if (error != UUcError_None) { break; }

		// skip files that are not valid image files
		{
			const char			*file_suffix;
			UUtBool				valid_image_suffix;

			file_suffix = BFrFileRef_GetSuffixName(&file_ref);
			if (file_suffix == NULL) { continue; }
			valid_image_suffix = UUmString_IsEqual_NoCase(file_suffix, "bmp") || UUmString_IsEqual_NoCase(file_suffix, "psd");
			if (!valid_image_suffix) { continue;  }
		}

		// get the file name
		UUrString_Copy(name, BFrFileRef_GetLeafName(&file_ref), BFcMaxFileNameLength);
		UUrString_Capitalize(name, BFcMaxFileNameLength);
		UUrString_StripExtension(name);

		error = Imp_ProcessTexture_File(&file_ref, name, &texture_ref);
		IMPmError_ReturnOnError(error);
	}

	BFrDirectory_FileIterator_Delete(file_iterator);
	file_iterator = NULL;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_AddFurniture(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;
	BFtFileRef			*furniture_dir;
	BFtFileRef			*texture_dir;
	char				*dir_name;

	// get the dir_name the files are in
	error = GRrGroup_GetString(inGroup, "path", &dir_name);
	UUmError_ReturnOnError(error);

	// create the furniture directory file ref
	error =
		BFrFileRef_DuplicateAndReplaceName(
			inSourceFileRef,
			dir_name,
			&furniture_dir);
	IMPmError_ReturnOnError(error);

	// create the texture directory file ref
	error =
		BFrFileRef_DuplicateAndAppendName(
			furniture_dir,
			"textures",
			&texture_dir);
	IMPmError_ReturnOnError(error);

	// import the furniture
	error = IMPiAddFurniture(furniture_dir);
	IMPmError_ReturnOnError(error);

	// import the textures
	error = IMPiAddFurnitureTextures(texture_dir);
	IMPmError_ReturnOnError(error);

	BFrFileRef_Dispose(furniture_dir);
	BFrFileRef_Dispose(texture_dir);

	return UUcError_None;
}
