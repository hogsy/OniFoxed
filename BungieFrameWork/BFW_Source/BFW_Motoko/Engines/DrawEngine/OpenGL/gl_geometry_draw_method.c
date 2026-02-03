/*
	gl_geometry_draw_method.c
	Tuesday Aug. 15, 2000 8:51 pm
	Stefan
*/

#ifdef __ONADA__

/*---------- includes */

#include "gl_engine.h"

/*---------- macromania */

// 09/01/2000 ripped out all the conditional vertex array code since Oni will never use it.

#if defined(GEOM_TYPE) && defined(GEOM_SPLIT_TYPE) && defined(NUM_VERTICES) && defined(GEOMETRY_MODE) && defined(FUNCTION_NAME)

#ifdef COUNTER_NAME
UUtUns32 COUNTER_NAME = 0;
#endif

#define SUBMIT_POINT(j)			GL_FXN(glVertex3f)(our_screen_points[j]->x, our_screen_points[j]->y, ZCOORD(our_screen_points[j]->z));

#define SUBMIT_COLOR(G,j) { \
    const unsigned long c= vertex_shades[G->indices[j]]; \
    GL_FXN(glColor4ub)((byte)((c&0x00FF0000)>>16), \
    (byte)((c&0x0000FF00)>>8), \
    (byte)(c&0x000000FF), \
    gl->constant_color.a); \
}

#define SUBMIT_COLOR_SPLIT(G,j)	{ \
    const unsigned long c= G->shades[j]; \
    GL_FXN(glColor4ub)((byte)((c&0x00FF0000)>>16), \
    (byte)((c&0x0000FF00)>>8), \
    (byte)(c&0x000000FF), \
    gl->constant_color.a); \
}

#define SUBMIT_BASE_QRST(G,j)	{ \
    const float oow= our_screen_points[j]->invW; \
    GL_FXN(glTexCoord4f)(base_texture_coords[G->indices[j]].u * oow, \
                         base_texture_coords[G->indices[j]].v * oow, \
                         0.f, oow); \
}

#define SUBMIT_BASE_QRST_SPLIT(G,j)	{ \
    const float oow= our_screen_points[j]->invW; \
    GL_FXN(glTexCoord4f)(base_texture_coords[G->baseUVIndices.indices[j]].u * oow, \
                         base_texture_coords[G->baseUVIndices.indices[j]].v * oow, \
                         0.f, oow); \
}

#define SUBMIT_ENV_QRST(G,j)	{ \
    const float oow= our_screen_points[j]->invW; \
    GL_FXN(glTexCoord4f)(env_texture_coords[G->indices[j]].u * oow, \
         		env_texture_coords[G->indices[j]].v * oow, \
		        0.f, oow); \
}
#define SUBMIT_BASE_QRST1(G,j)	{ \
        const float oow= our_screen_points[j]->invW; \
        GL_EXT(glMultiTexCoord4fARB)(GL_TEXTURE1_ARB, base_texture_coords[G->indices[j]].u * oow, \
              			     base_texture_coords[G->indices[j]].v * oow, \
			             0.f, oow); \
}
#define SUBMIT_ENV_QRST0(G,j)	{ \
        const float oow= our_screen_points[j]->invW; \
        GL_EXT(glMultiTexCoord4fARB)(GL_TEXTURE0_ARB, env_texture_coords[G->indices[j]].u * oow, \
        env_texture_coords[G->indices[j]].v * oow, \
        0.f, oow); \
}

/*---------- code */

static void FUNCTION_NAME (
	GEOM_TYPE *in_geom)
{
	const M3tPointScreen *screen_points= gl->state_ptr[M3cDrawStatePtrType_ScreenPointArray];
	const UUtUns32 *vertex_shades= gl->state_ptr[M3cDrawStatePtrType_ScreenShadeArray_DC];
	const M3tPointScreen *our_screen_points[NUM_VERTICES];
	const M3tTextureCoord *base_texture_coords= gl->state_ptr[M3cDrawStatePtrType_TextureCoordArray];
	int i;

	/* because we know that we will always be in 2D mode, we can avoid calling this
	gl_camera_update(_camera_mode_2d);*/

#ifdef COUNTER_NAME
        COUNTER_NAME++;
#endif

	for (i=0; i<NUM_VERTICES; i++)
	{
		our_screen_points[i]= screen_points + in_geom->indices[i];
	}

	switch (gl->geom_draw_mode)
	{
		case _geom_draw_mode_default:
			// diffuse color + base texture
			gl_set_textures(gl->texture0, NULL, NONE, NONE); // this will flush buffers if needed
			GL_FXN(glBegin)(GEOMETRY_MODE);
			for (i=0; i<NUM_VERTICES; i++)
			{
				SUBMIT_COLOR(in_geom, i)
				SUBMIT_BASE_QRST(in_geom, i)
				SUBMIT_POINT(i)
			}
			GL_FXN(glEnd)();
			break;

		case _geom_draw_mode_wireframe:
			gl_set_textures(NULL, NULL, GL_ONE, GL_ZERO); // this will flush buffers if needed
			GL_FXN(glBegin)(GL_LINE_STRIP);
			for (i=0; i<NUM_VERTICES; i++)
			{
				SUBMIT_POINT(i)
			}
			SUBMIT_POINT(0)
			GL_FXN(glEnd)();
			break;

		case _geom_draw_mode_gourand:
			// diffuse color only
			gl_set_textures(NULL, NULL, GL_ONE, GL_ZERO); // this will flush buffers if needed
			GL_FXN(glBegin)(GEOMETRY_MODE);
			for (i=0; i<NUM_VERTICES; i++)
			{
				SUBMIT_COLOR(in_geom, i)
				SUBMIT_POINT(i)
			}
			GL_FXN(glEnd)();
			break;

		case _geom_draw_mode_flat:
			// single texture only
			gl_set_textures(gl->texture0, NULL, NONE, NONE); // this will flush buffers if needed
			GL_FXN(glBegin)(GEOMETRY_MODE);
			for (i=0; i<NUM_VERTICES; i++)
			{
				SUBMIT_BASE_QRST(in_geom, i)
				SUBMIT_POINT(i)
			}
			GL_FXN(glEnd)();
			break;

		case _geom_draw_mode_split:
			// diffuse color + base texture
			{
				GEOM_SPLIT_TYPE *in_geom_split= (GEOM_SPLIT_TYPE *)in_geom;

				gl_set_textures(gl->texture0, NULL, NONE, NONE); // this will flush buffers if needed
				GL_FXN(glBegin)(GEOMETRY_MODE);
				for (i=0; i<NUM_VERTICES; i++)
				{
					SUBMIT_COLOR_SPLIT(in_geom_split, i)
					SUBMIT_BASE_QRST_SPLIT(in_geom_split, i)
					SUBMIT_POINT(i)
				}
				GL_FXN(glEnd)();
			}
			break;

		case _geom_draw_mode_environment_map_multipass:
			// diffuse + texture, src blend= 1, dst blend= 0
			{
				const M3tTextureCoord *env_texture_coords= gl->state_ptr[M3cDrawStatePtrType_EnvTextureCoordArray];

				gl_set_textures(gl->texture0, NULL, GL_ONE, GL_ZERO); // this will flush buffers if needed
				GL_FXN(glBegin)(GEOMETRY_MODE);
				for (i=0; i<NUM_VERTICES; i++)
				{
					SUBMIT_COLOR(in_geom, i)
					SUBMIT_ENV_QRST(in_geom, i)
					SUBMIT_POINT(i)
				}
				GL_FXN(glEnd)();
			}
			break;

		case _geom_draw_mode_base_map_multipass:
			// diffuse + texture, src blend= 1, dst blend= src alpha
			gl_set_textures(gl->texture1, NULL, GL_ONE, GL_SRC_ALPHA); // this will flush buffers if needed
			GL_FXN(glBegin)(GEOMETRY_MODE);
			for (i=0; i<NUM_VERTICES; i++)
			{
				SUBMIT_COLOR(in_geom, i)
				SUBMIT_BASE_QRST(in_geom, i)
				SUBMIT_POINT(i)
			}
			GL_FXN(glEnd)();
			break;

		case _geom_draw_mode_environment_map:
			// diffuse color, base texture & enviroment map texture
			{
				const M3tTextureCoord *env_texture_coords= gl->state_ptr[M3cDrawStatePtrType_EnvTextureCoordArray];

				if (gl->multitexture)
				{
					gl_set_textures(gl->texture0, gl->texture1, NONE, NONE); // this will flush buffers if needed
					GL_FXN(glBegin)(GEOMETRY_MODE);
					for (i=0; i<NUM_VERTICES; i++)
					{
						SUBMIT_COLOR(in_geom, i)
						SUBMIT_ENV_QRST0(in_geom, i)
						SUBMIT_BASE_QRST1(in_geom, i)
						SUBMIT_POINT(i)
					}
					GL_FXN(glEnd)();
				}
				else // no multitexture; multipass per-poly, yuck (should be using the above multipass modes)
				{
					UUmAssert(0); // why are you not using the above multipass modes?

					// render first pass
					gl_set_textures(gl->texture0, NULL, GL_ONE, GL_ZERO); // this will flush buffers if needed
					GL_FXN(glBegin)(GEOMETRY_MODE);
					for (i=0; i<NUM_VERTICES; i++)
					{
						SUBMIT_COLOR(in_geom, i)
						SUBMIT_ENV_QRST(in_geom, i)
						SUBMIT_POINT(i)
					}
					GL_FXN(glEnd)();
					// render second pass
					gl_set_textures(gl->texture1, NULL, GL_ONE, GL_SRC_ALPHA); // this will flush buffers if needed
					GL_FXN(glBegin)(GEOMETRY_MODE);
					for (i=0; i<NUM_VERTICES; i++)
					{
						SUBMIT_COLOR(in_geom, i)
						SUBMIT_BASE_QRST(in_geom, i)
						SUBMIT_POINT(i)
					}
					GL_FXN(glEnd)();
				}
			}
			break;

		case _geom_draw_mode_environment_map_low_quality:
			// base map rgb & ignore alpha channel
			gl_set_textures(gl->texture1, NULL, GL_ONE, GL_ZERO); // this will flush buffers if needed
			GL_FXN(glBegin)(GEOMETRY_MODE);
			for (i=0; i<NUM_VERTICES; i++)
			{
				SUBMIT_COLOR(in_geom, i)
				SUBMIT_BASE_QRST(in_geom, i)
				SUBMIT_POINT(i)
			}
			GL_FXN(glEnd)();
			break;

		default:
			UUmAssert(0);
			break;
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

#endif // defined(GEOM_TYPE) && (GEOM_SPLIT_TYPE) && (NUM_VERTICES) && (GEOMETRY_MODE) && (FUNCTION_NAME)

#undef GEOM_TYPE
#undef GEOM_SPLIT_TYPE
#undef NUM_VERTICES
#undef GEOMETRY_MODE
#undef FUNCTION_NAME
#undef COUNTER_NAME

#endif // __ONADA__

