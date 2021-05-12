/*	FILE:	Oni_Sky.h
	
	AUTHOR:	Quinn Dunki
	
	CREATED: April 2, 1999
	
	PURPOSE: control of sky in ONI
	
	Copyright 1998

*/

#ifndef _ONI_SKY_H_
#define _ONI_SKY_H_

#define ONcMaxSkyName			64		// don't change without changing references below
#define ONcSky_MaxPlanets		8		// don't change without changing references below

#define ONcTemplate_SkyClass UUm4CharToUns32('O','N','S','K')
typedef tm_template('O','N','S','K',"Oni Sky class") ONtSkyClass
{

	M3tTextureMap		*skybox[6];
	M3tTextureMap		*planet_tex[8];
	M3tTextureMap		*lens_flare;
	M3tTextureMap		*star_textures[5];

	UUtUns32			num_planets;
	UUtUns32			planet_with_lensflare;
	float				planet_xsize[8];
	float				planet_ysize[8];
	float				planet_azimuth[8];
	float				planet_bearing[8];

	float				lensflare_scale;
	float				lensflare_alpha;
	UUtUns32			star_count;
	UUtUns32			star_seed;
	UUtUns32			star_texture_count;
} ONtSkyClass;

typedef struct ONtSkyFlare
{
	M3tPoint3D			draw_position;
	M3tPoint3D			actual_position;
	M3tVector3D			direction;
	float				cosine_cutoff_angle;
	float				cosine_falloff_angle;
	float				angle_falloff_a;
	float				angle_falloff_b;
	float				cutoff_angle;
	float				falloff_angle;
	float				alpha;
	UUtUns32			last_update_tick;
} ONtSkyFlare;

typedef struct ONtSky
{
	ONtSkyClass			*sky_class;
	
	M3tPoint3D			planet_position[ONcSky_MaxPlanets];

	M3tSpriteArray		*star_sprite_array;
	M3tSkyboxData		skybox;

	UUtUns16			*twinkle_indices;
	UUtUns32			twinkle_count;

	ONtSkyFlare			flare;
} ONtSky;

UUtError ONrSky_Initialize( );
UUtError ONrSky_LevelBegin( ONtSky *inSky, ONtSkyClass *inSkyClass );
UUtError ONrSky_LevelEnd( ONtSky *inSky );

void ONrSky_Update( ONtSky *inSky );

void ONrSky_Draw( ONtSky *inSky );

void ONrSky_DrawLensFlare( ONtSky *inSky );

#endif
