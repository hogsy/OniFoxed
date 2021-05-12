#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_AppUtilities.h"
#include "BFW_Group.h"
#include "BFW_TM_Construction.h"
#include "BFW_Mathlib.h"
#include "BFW_Object.h"
#include "BFW_BitVector.h"
#include "BFW_FileFormat.h"
#include "BFW_Effect.h"

#include "ONI_Sky.h"

#include "Imp_Common.h"
#include "Imp_Texture.h"
#include "Imp_Env2_Private.h"
#include "Imp_Model.h"
#include "Imp_ParseEnvFile.h"
#include "Imp_Sky.h"
#include "EnvFileFormat.h"
#include "Imp_Sound.h"

//UUtError Imp_GenerateHorizonCylinder( GRtGroup* inGroup, M3tGeometry **ioGeometry );

UUtError Imp_AddSky(
	BFtFileRef*			inSourceFile,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;
	ONtSkyClass			*sky;
	char				*temp_name;
	//float				cloudSpeed;
	char				item_name[100];
	UUtUns32			i;

	error = TMrConstruction_Instance_Renew( ONcTemplate_SkyClass, inInstanceName, 0, &sky );
	IMPmError_ReturnOnError(error);

	sky->star_count			= 0;
	sky->star_seed			= 0;
	sky->star_texture_count	= 0;

	// skybox

	error = GRrGroup_GetString(inGroup, "top", &temp_name);
	if( error == UUcError_None )
	{
		sky->skybox[M3cSkyTop] = M3rTextureMap_GetPlaceholder(temp_name);
		UUmAssert(sky->skybox[M3cSkyTop]);
	}

	error = GRrGroup_GetString(inGroup, "left", &temp_name);
	if( error == UUcError_None )
	{
		sky->skybox[M3cSkyLeft] = M3rTextureMap_GetPlaceholder(temp_name);
		UUmAssert(sky->skybox[M3cSkyLeft]);
	}

	error = GRrGroup_GetString(inGroup, "right", &temp_name);
	if( error == UUcError_None )
	{
		sky->skybox[M3cSkyRight] = M3rTextureMap_GetPlaceholder(temp_name);
		UUmAssert(sky->skybox[M3cSkyRight]);
	}

	error = GRrGroup_GetString(inGroup, "front", &temp_name);
	if( error == UUcError_None )
	{
		sky->skybox[M3cSkyFront] = M3rTextureMap_GetPlaceholder(temp_name);
		UUmAssert(sky->skybox[M3cSkyFront]);
	}	

	error = GRrGroup_GetString(inGroup, "back", &temp_name);
	if( error == UUcError_None )
	{
		sky->skybox[M3cSkyBack] = M3rTextureMap_GetPlaceholder(temp_name);
		UUmAssert(sky->skybox[M3cSkyBack]);
	}

	// CB: read in planets (sun, moon, buildings, etc) ... this more general code implemented 03 September, 2000
	sky->num_planets = 0;
	for (i = 0; i < ONcSky_MaxPlanets; i++) {
		sprintf(item_name, "planet%d_tex", i + 1);
		error = GRrGroup_GetString(inGroup, item_name, &temp_name);
		if (error != UUcError_None) {
			break;
		}

		sky->planet_tex[i] = M3rTextureMap_GetPlaceholder(temp_name);
		UUmAssert(sky->planet_tex[i]);

		sky->num_planets++;

		// set up default planet parameters (copied from jason duncan's sky code)
		sky->planet_xsize[i] = 100.0f;
		sky->planet_ysize[i] = 100.0f;
		sky->planet_azimuth[i] = 22.5f;
		sky->planet_bearing[i] = 11.25f;

		sprintf(item_name, "planet%d_size_x", sky->num_planets);
		GRrGroup_GetFloat(inGroup, item_name, &sky->planet_xsize[i]);

		sprintf(item_name, "planet%d_size_y", sky->num_planets);
		GRrGroup_GetFloat(inGroup, item_name, &sky->planet_ysize[i]);

		sprintf(item_name, "planet%d_azimuth", sky->num_planets);
		GRrGroup_GetFloat(inGroup, item_name, &sky->planet_azimuth[i]);
		sky->planet_azimuth[i] *= M3cDegToRad;

		sprintf(item_name, "planet%d_bearing", sky->num_planets);
		GRrGroup_GetFloat(inGroup, item_name, &sky->planet_bearing[i]);
		sky->planet_bearing[i] *= M3cDegToRad;
	}

	// lens flare
	sky->planet_with_lensflare = (UUtUns32) -1;
	sky->lens_flare = NULL;
	error = GRrGroup_GetString(inGroup, "lens_flare", &temp_name);
	if( error == UUcError_None )
	{
		error = GRrGroup_GetUns32(inGroup, "planet_with_lens_flare", &sky->planet_with_lensflare);
		if ((error != UUcError_None) || (sky->planet_with_lensflare < 1) || (sky->planet_with_lensflare > sky->num_planets)) {
			Imp_PrintWarning("sky must have a 'planet_with_lens_flare' which is index to a valid planet... disabling lensflare");
			sky->planet_with_lensflare = (UUtUns32) -1;
			sky->lens_flare = NULL;
		} else {
			// convert from human-readable index [1..n] to computer index [0..n-1]
			sky->planet_with_lensflare--;

			sky->lens_flare = M3rTextureMap_GetPlaceholder(temp_name);
			UUmAssert(sky->lens_flare);
		}
	}

	error = GRrGroup_GetFloat(inGroup, "lensflare_scale", &sky->lensflare_scale);
	if (error != UUcError_None) sky->lensflare_scale = 4.0f;

	error = GRrGroup_GetFloat(inGroup, "lensflare_alpha", &sky->lensflare_alpha);
	if (error != UUcError_None) sky->lensflare_alpha = 1.0f;

	// stars
	sky->star_texture_count = 0;
	for( i = 0; i < 5; i++ )
	{
		sprintf( item_name, "star_texture%d", i );
		error = GRrGroup_GetString(inGroup, item_name, &temp_name);
		if( error == UUcError_None )
		{
			sky->star_textures[sky->star_texture_count++] = M3rTextureMap_GetPlaceholder(temp_name);
		}
	}

	error = GRrGroup_GetUns32( inGroup, "star_count", &sky->star_count);
	if( error != UUcError_None )
	{
		sky->star_count		= 0;
	}

	error = GRrGroup_GetUns32( inGroup, "star_seed", &sky->star_seed);
	if( error != UUcError_None )
	{
		sky->star_seed		= 0;
	}

	return UUcError_None;
}

