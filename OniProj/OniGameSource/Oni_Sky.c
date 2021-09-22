/*	FILE:	Oni_Sky.c
	
	AUTHOR:	Jason Duncan
	
	CREATED: May, 2000
	
	PURPOSE: the sky
	
	Copyright 2000
*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Totoro.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"
#include "BFW_Physics.h"
#include "BFW_Object.h"
#include "BFW_Noise.h"
#include "Oni_Weapon.h"
#include "Oni.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Sky.h"

#include "Motoko_Manager.h"
/*#include "OGL_DrawGeom_Common.h"
#include "OG_GC_Private.h"
#include "OG_GC_Method_Geometry.h"
#include "OG_GC_Method_State.h"*/

// =========================================================================================================

UUtBool						ONgSky_Initialized = UUcFalse;
UUtBool						ONgSky_ShowSky;
UUtBool						ONgSky_ShowClouds;
UUtBool						ONgSky_ShowSkybox;
UUtBool						ONgSky_ShowStars;
UUtBool						ONgSky_ShowPlanet;
M3tPoint2D					ONgSky_SunVisiblityTestPoints[50];
UUtInt32					ONgSky_SunVisiblityTestCount;
float						ONgSky_NoiseTable[1024];
float						ONgSky_Height;

// =========================================================================================================

UUtError ONrSky_Initialize( )
{
	UUtError				error;
	UUtInt32				i;

	error;

	ONgSky_ShowSky					= UUcTrue;
	ONgSky_ShowClouds				= UUcTrue;
	ONgSky_ShowSkybox				= UUcTrue;
	ONgSky_ShowStars				= UUcTrue;
	ONgSky_ShowPlanet				= UUcTrue;
	ONgSky_SunVisiblityTestCount	= 0;
	ONgSky_Height					= 0.0f;

#if CONSOLE_DEBUGGING_COMMANDS
	error = SLrGlobalVariable_Register_Bool( "sky_show_sky",			"Toggles display of the sky",				&ONgSky_ShowSky			);
	UUmError_ReturnOnError(error);

	error = SLrGlobalVariable_Register_Bool( "sky_show_clouds",			"Toggles display of the clouds",			&ONgSky_ShowClouds		);
	UUmError_ReturnOnError(error);

	error = SLrGlobalVariable_Register_Bool( "sky_show_skybox",			"Toggles display of the skybox",			&ONgSky_ShowSkybox		);
	UUmError_ReturnOnError(error);

	error = SLrGlobalVariable_Register_Bool( "sky_show_stars",			"Toggles display of the stars",				&ONgSky_ShowStars		);
	UUmError_ReturnOnError(error);

	error = SLrGlobalVariable_Register_Bool( "sky_show_planet",			"Toggles display of the planet (sun/moon)",	&ONgSky_ShowPlanet		);
	UUmError_ReturnOnError(error);

	SLrGlobalVariable_Register_Float( "sky_height",	"",	&ONgSky_Height	);
#endif

	// build the noise table
	UUrNoise_Initialize( 1000 );

	for( i = 0; i < 1024; i++ )
	{
		ONgSky_NoiseTable[i]	= UUrNoise_1( (float) i / 8.0f );
	}

	ONgSky_Initialized			= UUcTrue;

	return UUcError_None;
}

static void ONiSky_GetYIQ( float r, float g, float b, M3tVector3D *ioYIQ )
{
	M3tPoint3D		color;
	static M3tMatrix3x3	m = 
	{ 0.30f,  0.60f,  0.21f, 0.59f, -0.28f, -0.52f, 0.11f, -0.32f, -0.31f };

	color.x		= r;
	color.y		= g;
	color.z		= b;
/*
	m.m[0][0]	=  0.30f;
	m.m[0][1]	=  0.60f;
	m.m[0][2]	=  0.21f;

	m.m[1][0]	=  0.59f;
	m.m[1][1]	= -0.28f;
	m.m[1][2]	= -0.52f;

	m.m[2][0]	=  0.11f;
	m.m[2][1]	= -0.32f;
	m.m[2][2]	= -0.31f;
*/
	MUrMatrix3x3_MultiplyPoint( &color, &m, ioYIQ );
}

static void ONiSky_TextureCoords( float inU, float inV, M3tTextureCoord *ioCoord )
{
	inU = UUmPin( inU, -1000.0f, 1000.0f );
	inV = UUmPin( inV, -1000.0f, 1000.0f );
	
	ioCoord->u	=  (( inU + 1000.0f ) / 2000.0f);
	ioCoord->v	= -(( inV - 1000.0f ) / 2000.0f);
}

UUtBool ONiSky_LinePlaneIntersection(
	M3tPoint3D *inLineA,
	M3tPoint3D *inLineB,
	M3tPlaneEquation *inPlane,
	M3tPoint3D *outIntersection)
{
	float Xm,Ym,Zm,denom,t;
	M3tPoint3D newPoint;
	
	Xm = (inLineB->x - inLineA->x);
	Ym = (inLineB->y - inLineA->y);
	Zm = (inLineB->z - inLineA->z);
	
	denom = inPlane->a * Xm +inPlane->b * Ym + inPlane->c * Zm;
	
	if (denom == 0.0f)
	{
		return UUcFalse;
	}
	
	t = -(inPlane->a * inLineA->x + inPlane->b * inLineA->y + inPlane->c * inLineA->z + inPlane->d) / denom;

	if (t < 0.0f || t > 1.0f)
	{
		return UUcFalse;
	}
	
	newPoint.x = Xm * t + inLineA->x;
	newPoint.y = Ym * t + inLineA->y;
	newPoint.z = Zm * t + inLineA->z;
	
	if (outIntersection) *outIntersection = newPoint;
	return UUcTrue;
}

static void ONiSky_SkyboxIntercept( M3tPoint3D *inPoint, M3tPoint3D *inCenter, M3tSkyboxData *inSkybox, UUtUns32 *ioPlaneIndex, M3tTextureCoord *ioCoords )
{
	M3tVector3D			ray;
	float				theta_horiz;
	float				theta_vert;
	float				d;
	float				quarter_pi;
	float				dist_top;
	float				dist_side;
	UUtUns32			plane;
	float				size;
	UUtBool				hit;
	M3tPlaneEquation	side_plane;
	M3tPoint3D			hit_point;
	M3tPoint3D			top_hit_point;
	M3tPoint3D			point_to;

	size				= 1000.0f;

	// calc a ray from the origin to the star
	MUmVector_Subtract( ray, *inPoint, *inCenter );
	MUrNormalize( &ray );

	theta_horiz		= MUrATan2( ray.z, ray.x );
	d				= MUrSqrt( UUmSQR( ray.x ) + UUmSQR( ray.z ) );
	theta_vert		= MUrATan2( ray.y, d );

	UUmTrig_Clip( theta_horiz );

	quarter_pi				= M3cPi / 4.0f;
	
	point_to.x				= inPoint->x + ray.x * 10000.0f;
	point_to.y				= inPoint->y + ray.y * 10000.0f;
	point_to.z				= inPoint->z + ray.z * 10000.0f;

	point_to					= *inPoint;
	
	side_plane.a				=  0;
	side_plane.b				=  0;
	side_plane.c				=  0;
	side_plane.d				=  1000;

	// right - east
	if( theta_horiz <= quarter_pi || theta_horiz > M3c2Pi - quarter_pi )
	{
		plane						= M3cSkyRight;
		side_plane.a				= -1;
	}
	// front - north
	else if( theta_horiz <= quarter_pi * 3.0f )
	{
		plane						= M3cSkyFront;
		side_plane.c				= -1;
	}
	// left - west
	else if( theta_horiz <= quarter_pi * 5.0f )
	{
		plane						= M3cSkyLeft;
		side_plane.a				= 1;
	}
	// back - south
	else
	{
		plane						= M3cSkyBack;
		side_plane.c				= 1;
	}

	ONiSky_LinePlaneIntersection( inCenter, &point_to, &side_plane, &hit_point );

	dist_side				= MUmVector_GetDistance( *inCenter, hit_point );

	// top - top
	side_plane.a			=  0.0f;
	side_plane.b			= -1.0f;
	side_plane.c			=  0.0f;
	side_plane.d			=  1000.0f;
	hit						=  ONiSky_LinePlaneIntersection( inCenter, &point_to, &side_plane, &top_hit_point );
	if( hit )
	{
		dist_top				= MUmVector_GetDistance( *inCenter, top_hit_point );
		if( dist_side > dist_top )
		{
			hit_point			= top_hit_point;
			plane				= M3cSkyTop;	
		}
	}

	switch( plane )
	{
		case M3cSkyLeft:	// west
			ONiSky_TextureCoords( -hit_point.z, hit_point.y, ioCoords );
			break;
		case M3cSkyRight:	// east
			ONiSky_TextureCoords(  hit_point.z, hit_point.y, ioCoords );
			break; 
		case M3cSkyFront:	// north
			ONiSky_TextureCoords( -hit_point.x, hit_point.y, ioCoords );
			break;
		case M3cSkyBack:	// south
			ONiSky_TextureCoords(  hit_point.x, hit_point.y, ioCoords );
			break;
		case M3cSkyTop:		// up 
			ONiSky_TextureCoords(  hit_point.x, hit_point.z, ioCoords );
			break;
	}
	*ioPlaneIndex = plane;
}

static float ONiSky_TexturePixel( M3tTextureMap *inText, M3tTextureCoord *ioCoords )
{
	float			x;
	float			y;
	IMtPixel		pixel;
	float			r, g, b, a;
	float			val;
//	M3tVector3D		yiq;

	if( !inText )
		return 0.0f;

	// CB: now that textures' data is not necessarily memory-resident all the time this will fail.
	// but we're not using it anyway, so no harm done
	if (inText->pixels == NULL)
		return 0.0f;

	x = ioCoords->u * inText->width;
	y = ioCoords->v * inText->height;

	pixel = IMrPixel_Get( inText->pixels, inText->texelType,(UUtUns16) x,(UUtUns16) y,inText->width,inText->height);

	IMrPixel_Decompose( pixel, inText->texelType, &a, &r, &g, &b );

//	ONiSky_GetYIQ( r, g, b, &yiq );
//	val = (float) fabs( yiq.x );

	val = UUmMax( UUmMax( r, g ), b );

	return val;
}

UUtError ONrSky_LevelBegin( ONtSky *inSky, ONtSkyClass *inSkyClass )
{
	UUtUns32				i;
	float					view_distance_far;
	float					view_distance_near;
	float					radius;
	float					height;
	M3tSpriteInstance		*star_sprites;
	UUtUns32				star_sprite_count;

	if( !ONgSky_Initialized )
		ONrSky_Initialize();

	UUmAssert( ONgLevel );

	//Init the sky for a level
	UUrMemory_Clear(inSky,sizeof(ONtSky));

	inSky->sky_class		= inSkyClass;
	inSky->twinkle_indices	= NULL;
	inSky->twinkle_count	= 0;

	if( !inSky->sky_class || !ONgLevel )
		return UUcError_None;

	// grab view distance
	M3rCamera_GetStaticData( ONgActiveCamera, NULL, NULL, &view_distance_near, &view_distance_far);
	
	radius					= view_distance_far * 0.5f;
//	inSky->skyClass->sunglow->baseMap->flags |= M3cTextureFlags_Blend_Additive;
//	TMrInstance_GetDataPtr(M3cTemplate_TextureMap, "caution01", (void **) &inSky->skyClass->skydome->baseMap);

	M3rDraw_CreateSkybox( &inSky->skybox, inSkyClass->skybox );

	ONgSky_Height = ONgLevel->sky_height;
	
	if( inSkyClass->star_count )
	{
		if( inSkyClass->star_seed )
			UUrRandom_SetSeed( inSkyClass->star_seed );

		star_sprites			= (M3tSpriteInstance*) UUrMemory_Block_New( sizeof(M3tSpriteInstance) * inSkyClass->star_count );
		star_sprite_count		= 0;

		for( i = 0; i < inSkyClass->star_count; i++ )
		{
			UUtUns32			plane;
			M3tTextureCoord		coords;
			M3tTextureMap		*text;
			float				pixel;
			float				alpha;
			M3tPoint3D			position;
			M3tPoint3D			center = { 0,0,0 };
			float				scale			= UUmRandomRangeFloat( 10.0f, 40.0f );

			P3rGetRandomUnitVector3D(&position);
			
			if( position.y < 0.0f )
				position.y = -position.y;

			position.x		*= radius;
			position.y		*= radius;
			position.z		*= radius;

			height			= ONgSky_Height * 1000.0f;
			
			//center.y		+= height;
			//position.y		+= height;

			ONiSky_SkyboxIntercept( &position, &center, &inSky->skybox, &plane, &coords );

			switch( plane )
			{
				case M3cSkyLeft:	// west
					text	= inSky->skybox.textures[M3cSkyLeft];
					break;
				case M3cSkyRight:	// east
					text	= inSky->skybox.textures[M3cSkyRight];
					break;
				case M3cSkyFront:	// north
					text	= inSky->skybox.textures[M3cSkyBack];
					break;
				case M3cSkyBack:	// south
					text	= inSky->skybox.textures[M3cSkyFront];
					break;
				case M3cSkyTop:
					text	= inSky->skybox.textures[M3cSkyTop];
					break;
			}

			pixel	= ONiSky_TexturePixel( text, &coords );

			if( pixel <= 0.05f )
			{
				pixel											= 1.0f - ( pixel / 0.05f );
				alpha											= M3cMaxAlpha * pixel;
				//alpha											= M3cMaxAlpha * ( 1.0f - ( pixel / 0.1f ) ) ;
				star_sprites[star_sprite_count].position		= position;
				star_sprites[star_sprite_count].scale			= scale;
				star_sprites[star_sprite_count].max_alpha		= (UUtUns8)alpha;
				star_sprites[star_sprite_count].alpha			= (UUtUns8)alpha;
				star_sprite_count++;
			}
		}   

		inSky->star_sprite_array				= (M3tSpriteArray*)UUrMemory_Block_New( sizeof(M3tSpriteArray) + sizeof(M3tSpriteInstance) * (star_sprite_count-1) );
		inSky->star_sprite_array->texture_map	= inSkyClass->star_textures[0];
		inSky->star_sprite_array->sprite_count	= star_sprite_count;
		
		for( i = 0; i < star_sprite_count; i++ )
		{
			inSky->star_sprite_array->sprites[i].position		= star_sprites[i].position;
			inSky->star_sprite_array->sprites[i].scale			= star_sprites[i].scale;
			inSky->star_sprite_array->sprites[i].max_alpha		= star_sprites[i].max_alpha;
			inSky->star_sprite_array->sprites[i].alpha			= star_sprites[i].alpha;
		}

		UUrMemory_Block_Delete( star_sprites );

		inSky->twinkle_count	= star_sprite_count;	
		inSky->twinkle_indices	= UUrMemory_Block_New( sizeof( UUtUns16 ) * inSky->twinkle_count );
		for( i = 0; i < inSky->twinkle_count; i++ )
		{
			inSky->twinkle_indices[i] = UUmRandomRange( 0, (1024 * 16) - 1 );
		}
	}
	
	for (i = 0; i < inSkyClass->num_planets; i++) {
		float		planet_distance;

		inSky->planet_position[i].y = MUrSin(inSkyClass->planet_azimuth[i]) * radius;
		planet_distance				= MUrCos(inSkyClass->planet_azimuth[i]) * radius;
		inSky->planet_position[i].x	= MUrSin(inSkyClass->planet_bearing[i]) * planet_distance;
		inSky->planet_position[i].z	= MUrCos(inSkyClass->planet_bearing[i]) * planet_distance;

		if ((inSkyClass->lens_flare != NULL) && (inSkyClass->planet_with_lensflare == i)) {
			M3tVector3D						offset_vector;

			offset_vector					= inSky->planet_position[i];

			MUmVector_Normalize( offset_vector );
	
			inSky->flare.cutoff_angle				= 0.6f;
			inSky->flare.falloff_angle				= 0.3f;
			inSky->flare.cosine_cutoff_angle		= (float) cos( inSky->flare.cutoff_angle );
			inSky->flare.cosine_falloff_angle		= (float) cos( inSky->flare.falloff_angle );
			inSky->flare.angle_falloff_a			= 1.0f / ( inSky->flare.cosine_falloff_angle - inSky->flare.cosine_cutoff_angle );
			inSky->flare.angle_falloff_b			= -inSky->flare.cosine_cutoff_angle / ( inSky->flare.cosine_falloff_angle - inSky->flare.cosine_cutoff_angle );

			inSky->flare.draw_position.x			= offset_vector.x * view_distance_near * 10.0f;
			inSky->flare.draw_position.y			= offset_vector.y * view_distance_near * 10.0f;
			inSky->flare.draw_position.z			= offset_vector.z * view_distance_near * 10.0f;

			inSky->flare.actual_position.x			= offset_vector.x * radius;
			inSky->flare.actual_position.y			= offset_vector.y * radius;
			inSky->flare.actual_position.z			= offset_vector.z * radius;

			inSky->flare.alpha						= 0.0f;
			inSky->flare.last_update_tick			= 0;

			inSky->flare.direction					= offset_vector;

			inSkyClass->lens_flare->flags			|= M3cTextureFlags_Blend_Additive;

			// FIXME: find better points 
			// build 2d visiblity test points
			{
				int			j;
				int			x, y;
				float		dist;
				float		radius_squared = UUmSQR(20);

				j = 0;
				for( y = -3; y <= 3; y++ )
				{
					for( x = -3; x <= 3; x++ )
					{
						float	xx = x * 8.0f + (float) UUmRandomRangeFloat( -4.0f, 4.0f );
						float	yy = y * 8.0f + (float) UUmRandomRangeFloat( -4.0f, 4.0f );

						dist = UUmSQR(xx) + UUmSQR(yy);
						if( dist < radius_squared )
						{
							ONgSky_SunVisiblityTestPoints[j].x	= xx;
							ONgSky_SunVisiblityTestPoints[j].z	= yy;
							j++;
						}
					}
				}
				ONgSky_SunVisiblityTestCount	= j;
			}
		}
	}

	return UUcError_None;
}

UUtError ONrSky_LevelEnd( ONtSky *inSky )
{
	UUmAssert( inSky );

	if(!inSky->sky_class)
		return UUcError_None;

	if( inSky->star_sprite_array )
	{
		UUrMemory_Block_Delete( inSky->twinkle_indices );
		UUrMemory_Block_Delete( inSky->star_sprite_array );
		inSky->star_sprite_array	= NULL;
		inSky->twinkle_indices		= NULL;
	}

	M3rDraw_DestroySkybox( &inSky->skybox );

	return UUcError_None;
}

// FIXME: This function is sort of obsolete with the new sky system... may want to axe...
void ONrSky_Update( ONtSky *inSky )
{
	if (!inSky->sky_class) return;
}

void ONrSky_Draw( ONtSky *inSky )
{
	ONtSkyClass				*sky_class;
	M3tVector3D				direction;
	float					horiz;
	float					vert;
	float					delta;
	UUtUns32				i;
	UUtUns32				current_tick;
	UUtUns16				noise_index;
	UUtUns8					max_alpha;

	if( !inSky || !inSky->sky_class || !ONgSky_ShowSky ) return;

	sky_class			= inSky->sky_class;
	current_tick		= ONgGameState->gameTime; // S.S. unreliable-> M3rDraw_State_GetInt(M3cDrawStateIntType_Time);

	for( i = 0; i < inSky->twinkle_count; i++ )
	{
		noise_index		= inSky->twinkle_indices[i];
		noise_index		= (UUtUns16) (( noise_index + current_tick ) >> ( noise_index & 0x0F )) & 0x03FF;
		max_alpha		= inSky->star_sprite_array->sprites[i].max_alpha;

		inSky->star_sprite_array->sprites[i].alpha	= (UUtUns8) MUrUnsignedSmallFloat_To_Uns_Round( (max_alpha*0.25f) + (ONgSky_NoiseTable[noise_index] * (max_alpha*0.75f)));
	}
		
	
	inSky->skybox.height	= ONgSky_Height;
	inSky->skybox.height	= 0.0f;

	direction			= CAgCamera.viewData.viewVector;

	// find facing angles
	horiz				= MUrATan2(direction.z, direction.x);
	delta				= MUrSqrt(UUmSQR(direction.x) + UUmSQR(direction.z));
	vert				= MUrATan2(direction.y, delta);

	// clip
	UUmTrig_Clip( vert );
	UUmTrig_Clip( horiz );

	M3rMatrixStack_Push();
	{
		M3tPoint3D		pos;

		// move origin to camera location
		pos				= CAgCamera.viewData.location;
//		pos.y			+= ONgSky_Height * 1000.0f;

		M3rMatrixStack_ApplyTranslate(pos);
		M3rGeom_State_Push();
		{
			M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_Off);
			M3rGeom_State_Commit();

			// skybox
			if( ONgSky_ShowSkybox )
			{
				M3rDraw_Skybox( &inSky->skybox );
			}

			// stars
			if( inSky->star_sprite_array && ONgSky_ShowStars )
				M3rSpriteArray_Draw( inSky->star_sprite_array );

			// planets (sun / moon / buildings)
			if (ONgSky_ShowPlanet && (sky_class->num_planets > 0)) {
				M3tVector3D upvector = {0.0f, 1.0f, 0.0f};

				M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, M3cGeomState_SpriteMode_Rotated);
				M3rGeom_State_Commit();

				for (i = 0; i < sky_class->num_planets; i++) {
					if (sky_class->planet_tex[i] == NULL) {
						continue;
					}

					M3rSprite_Draw(sky_class->planet_tex[i], &inSky->planet_position[i], sky_class->planet_xsize[i], sky_class->planet_ysize[i],
									IMcShade_White, M3cMaxAlpha, 0.0f, &upvector, NULL, 0.0f, 0.0f, 0.0f);
				}
			}
		}
		M3rGeom_State_Pop();
	}
	M3rMatrixStack_Pop();
}

void ONrSky_DrawLensFlare( ONtSky* inSky )
{
	float				alpha, intensity, visible_scale, direction_dot_radius;
	M3tVector3D			eye_to_light_vector;
	UUtUns32			ticks, current_tick;
	UUtUns16			int_alpha;

	UUmAssert( inSky );

	if( !inSky || !inSky->sky_class || !inSky->sky_class->lens_flare )
		return;

	current_tick			= ONgGameState->gameTime; // S.S. unreliable-> M3rDraw_State_GetInt(M3cDrawStateIntType_Time);

	ticks					= current_tick - inSky->flare.last_update_tick;

	// color
	eye_to_light_vector		= inSky->flare.direction;

	MUmVector_Normalize( eye_to_light_vector );

	direction_dot_radius	= MUrVector_DotProduct( &CAgCamera.viewData.viewVector, &eye_to_light_vector );	

	if (direction_dot_radius < inSky->flare.cosine_cutoff_angle) {
		alpha = 0;

	} else {
		if (direction_dot_radius < inSky->flare.cosine_falloff_angle) {
			intensity = inSky->flare.angle_falloff_a * direction_dot_radius + inSky->flare.angle_falloff_b;
		} else {
			intensity = 1.0f;
		}

		visible_scale = M3rPointVisibleScale( &inSky->flare.actual_position, ONgSky_SunVisiblityTestPoints, ONgSky_SunVisiblityTestCount );
		visible_scale = 1 - UUmSQR(1 - visible_scale);

		alpha = M3cMaxAlpha * inSky->sky_class->lensflare_alpha * intensity * visible_scale;
	}


	if (alpha != inSky->flare.alpha)
	{
		float					delta;
		float					delta_per_tick;
		delta					= alpha - inSky->flare.alpha;
		if( delta > 0 ) 
		{
			delta_per_tick	= 75.0f;
			alpha			= UUmMin( alpha,	inSky->flare.alpha + delta_per_tick * ticks );
		}
		else
		{
			delta_per_tick	= 10.0f;
			alpha = UUmMax( alpha, inSky->flare.alpha - delta_per_tick * ticks );
		}
	}

	int_alpha = (UUtUns16) MUrUnsignedSmallFloat_To_Uns_Round(alpha);
	if (int_alpha > 0) {
		M3rMatrixStack_Push();
		M3rMatrixStack_ApplyTranslate(CAgCamera.viewData.location);
		M3rGeom_State_Push();
		{
			static M3tVector3D dir = { 0.0f, 1.0f, 0.0f };

			M3rDraw_State_SetInt( M3cDrawStateIntType_ZWrite,	M3cDrawState_ZWrite_Off );
			M3rGeom_State_Set( M3cGeomStateIntType_SpriteMode,	M3cGeomState_SpriteMode_Rotated );
			M3rGeom_State_Commit();
			M3rSprite_Draw(inSky->sky_class->lens_flare, &inSky->flare.draw_position, inSky->sky_class->lensflare_scale, inSky->sky_class->lensflare_scale,
							IMcShade_White, int_alpha, 0.0f, &dir, NULL, 0.0f, 0.0f, 0.0f );
		}
		M3rGeom_State_Pop();
		M3rMatrixStack_Pop();
	}
	inSky->flare.alpha					= alpha;
	inSky->flare.last_update_tick		= current_tick;
}

