/*
	gl_engine.c
	Thursday Aug. 10, 2000 8:00 pm
	Stefan
*/

#ifdef __ONADA__

/*---------- includes */

#include "gl_engine.h"
#include "BFW_MathLib.h"
#include "Oni_Motoko.h" // for graphics quality

/*---------- prototypes */

static UUtError gl_context_private_new(M3tDrawContextDescriptor *in_draw_context_descriptor,
	M3tDrawContextMethods **out_draw_context_funcs, UUtBool in_full_screen, M3tDrawAPI *out_api);
static void gl_context_private_delete(void);

static void gl_texture_reset_all(void);
static UUtError gl_private_state_new(void *in_state_private);
static void gl_private_state_delete(void *in_state_private);
static UUtError gl_change_mode(M3tDisplayMode mode);
static UUtError gl_private_state_update(void *in_state_private,
	UUtUns32 in_state_int_flags, const UUtInt32 *in_state_int,
	UUtInt32 in_state_ptr_flags, void **in_state_ptr);

static UUtError gl_frame_start(UUtUns32 game_ticks_elapsed);
static UUtError gl_frame_end(UUtUns32 *out_texture_bytes_downloaded);
static UUtError gl_frame_sync(void);
static UUtError gl_screen_capture(const UUtRect *in_rect, void *out_buffer);
static UUtBool gl_point_visible(const M3tPointScreen *in_point, float in_tolerance);
static UUtBool gl_single_pass_multitexture_capable(void);
static UUtBool gl_support_depth_reads(void);

static void gl_trisprite(const M3tPointScreen *in_points, const M3tTextureCoord *in_texture_coords);
static void gl_sprite(const M3tPointScreen *in_points, const M3tTextureCoord *in_texture_coords);
static void gl_sprite_array(const M3tPointScreen *in_points, const M3tTextureCoord *in_texture_coords,
	const UUtUns32 *in_colors, const UUtUns32 in_count);
static void gl_point(M3tPointScreen *inv_coord);
static void gl_line(UUtUns16 index0, UUtUns16 index1);
static void gl_triangle(M3tTri *in_tri);
static void gl_quad(M3tQuad *in_quad);
static void gl_pent(M3tPent *in_pent);

/*---------- globals */

const gl_color_4ub gl_color_white = {0xFF, 0xFF, 0xFF, 0xFF};

/*---------- code */

UUtBool gl_draw_engine_initialize(
	void)
{
	UUtBool success;

	if ((success= opengl_available()))
	{
		UUtError error= UUcError_None;

		//gl->mode= NONE;
		//gl->vertex_count= 0;
		//gl->vertex_format_flags= NONE;
		//gl->geom_draw_mode= NONE;

		gl->draw_engine_methods.contextPrivateNew= gl_context_private_new;
		gl->draw_engine_methods.contextPrivateDelete= gl_context_private_delete;
		gl->draw_engine_methods.textureResetAll= gl_texture_reset_all;

		gl->draw_engine_methods.privateStateSize= 0;
		gl->draw_engine_methods.privateStateNew= gl_private_state_new;
		gl->draw_engine_methods.privateStateDelete= gl_private_state_delete;
		gl->draw_engine_methods.privateStateUpdate= gl_private_state_update;

		gl->draw_engine_caps.engineFlags= M3cDrawEngineFlag_3DOnly;
		UUrString_Copy(gl->draw_engine_caps.engineName, M3cDrawEngine_OpenGL, M3cMaxNameLen);
		gl->draw_engine_caps.engineDriver[0] = 0;
		gl->draw_engine_caps.engineVersion= 1;
		gl->draw_engine_caps.numDisplayDevices= 1;

		// enumerate valid display modes
                memset(gl->draw_engine_caps.displayDevices[0].displayModes, 0, sizeof(gl->draw_engine_caps.displayDevices[0].displayModes));
		gl->draw_engine_caps.displayDevices[0].numDisplayModes=
			gl_enumerate_valid_display_modes(gl->draw_engine_caps.displayDevices[0].displayModes);

		error= M3rManager_Register_DrawEngine(&gl->draw_engine_caps,
			&gl->draw_engine_methods);

		if (error != UUcError_None)
		{
			UUrError_Report(UUcError_Generic, "Could not setup engine caps");
			success= FALSE;
		}
	}
	if (!success) // the app doesn't handle failure so we do it here
	{
		AUrMessageBox(AUcMBType_OK, "There is no hardware-accelerated OpenGL present on your system; Oni will now exit.");
		exit(0);
	}

	return success;
}

void gl_draw_engine_terminate(
	void)
{
	extern void gl_unload_opengl_dll(void); // platform-dependant

	gl_context_private_delete();
	if (gl_api)
	{
		UUrMemory_Block_Delete(gl_api);
		gl_api= NULL;
	}
	if (gl)
	{
		if (gl->loaded_texture_array)
		{
			UUrMemory_Block_Delete(gl->loaded_texture_array);
		}
		UUrMemory_Block_Delete(gl);
		gl= NULL;
	}

	UUrStartupMessage("OpenGL disposed properly");

	return;
}

static UUtError gl_context_private_new(
	M3tDrawContextDescriptor *in_draw_context_descriptor,
	M3tDrawContextMethods **out_draw_context_funcs,
	UUtBool in_full_screen,
	M3tDrawAPI *out_api)
{
	UUtError error= UUcError_None;
	word active_draw_engine;
	word active_device;
	word active_mode;
	M3tDrawEngineCaps *active_draw_engine_caps;

	UUrStartupMessage("creating new OpenGL context");
	*out_api= M3cDrawAPI_OpenGL;

	// get the active draw engine info
	M3rManager_GetActiveDrawEngine(&active_draw_engine, &active_device, &active_mode);
	active_draw_engine_caps= M3rDrawEngine_GetCaps(active_draw_engine);

	// initialize draw context methods
	gl->draw_context_methods.frameStart= gl_frame_start;
	gl->draw_context_methods.frameEnd= gl_frame_end;
	gl->draw_context_methods.frameSync= gl_frame_sync;
	gl->draw_context_methods.triangle= (M3tDrawContextMethod_Triangle)gl_triangle;
	gl->draw_context_methods.quad= (M3tDrawContextMethod_Quad)gl_quad;
	gl->draw_context_methods.pent= (M3tDrawContextMethod_Pent)gl_pent;
	gl->draw_context_methods.line= (M3tDrawContextMethod_Line)gl_line;
	gl->draw_context_methods.point= (M3tDrawContextMethod_Point)gl_point;
	gl->draw_context_methods.triSprite= (M3tDrawContextMethod_TriSprite)gl_trisprite;
	gl->draw_context_methods.sprite= (M3tDrawContextMethod_Sprite)gl_sprite;
	gl->draw_context_methods.spriteArray= (M3tDrawContextMethod_SpriteArray)gl_sprite_array;
	gl->draw_context_methods.screenCapture= gl_screen_capture;
	gl->draw_context_methods.pointVisible= gl_point_visible;
	gl->draw_context_methods.textureFormatAvailable= gl_texture_map_format_available;
	gl->draw_context_methods.changeMode= gl_change_mode;
	gl->draw_context_methods.resetFog= gl_reset_fog_parameters;
	gl->draw_context_methods.loadTexture= gl_texture_map_create;
	gl->draw_context_methods.unloadTexture= gl_texture_map_delete;
	gl->draw_context_methods.supportSinglePassMultitexture= gl_single_pass_multitexture_capable;
	gl->draw_context_methods.supportPointVisible= gl_support_depth_reads;

	*out_draw_context_funcs= &gl->draw_context_methods;

	gl->context_type= in_draw_context_descriptor->type;
	gl->display_mode= active_draw_engine_caps->displayDevices[active_device].displayModes[active_mode];

	if (!initialize_opengl())
	{
		error= _error_no_opengl;
	}
	else
	{
		gl->engine_initialized= TRUE;
	}

	if (error == _error_none)
	{
		if (gl->converted_data_buffer == NULL)
		{
			gl->converted_data_buffer= UUrMemory_Block_New(256*256*sizeof(long));
		}
		if (gl->converted_data_buffer == NULL)
		{
			error= _error_out_of_memory;
		}

		// force update of the camera
		gl->camera_mode= _camera_mode_3d;
		gl_camera_update(_camera_mode_2d);

		// create the texture private array
		if (gl->texture_private_data == NULL)
		{
			error= TMrTemplate_PrivateData_New(M3cTemplate_TextureMap,
				0, gl_texture_map_proc_handler, &gl->texture_private_data);
		}
	}

	return error;
}

static void gl_context_private_delete(
	void)
{
	if (gl != NULL)
	{
		if (gl->converted_data_buffer)
		{
			UUrMemory_Block_Delete(gl->converted_data_buffer);
			gl->converted_data_buffer= NULL;
		}

		if (gl->texture_private_data)
		{
			TMrTemplate_PrivateData_Delete(gl->texture_private_data);
			gl->texture_private_data= NULL;
		}

		if (gl->loaded_texture_array)
		{
			UUrMemory_Block_Delete(gl->loaded_texture_array);
			gl->loaded_texture_array= NULL;
		}

		if (gl->engine_initialized)
		{
			gl_sync_to_vtrace(FALSE);
			gl_platform_dispose();
			gl->engine_initialized= FALSE;
		}

		memset(gl, 0, sizeof(struct gl_state_global));
	}

	return;
}

static void gl_texture_reset_all(
	void)
{
	return;
}

static UUtError gl_private_state_new(
	void *in_state_private)
{
	return UUcError_None;
}

static void gl_private_state_delete(
	void *in_state_private)
{
	return;
}

static UUtError gl_change_mode(
        M3tDisplayMode mode)
{
	UUtError error= UUcError_None;
    M3tDisplayMode oldMode= gl->display_mode;

	gl->display_mode= mode;

	// this relies on gl_platform_initialize() doing the resolution
	// switch for us without messing up anything else!!
	if (gl_platform_initialize() == FALSE)
	{
		gl->display_mode= oldMode;
		error= (gl_platform_initialize() == TRUE ? UUcError_None : UUcError_Generic);
	}
	else
	{
		GL_FXN(glViewport)(0, 0, gl->display_mode.width, gl->display_mode.height);
		// force an update of the view
		gl->camera_mode= _camera_mode_3d;
		gl_camera_update(_camera_mode_2d);
	}


	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return error;
}

static int gl_save_geom_draw_mode= NONE;
static UUtError gl_private_state_update(
	void *in_state_private,
	UUtUns32 in_state_int_flags,
	const UUtInt32 *in_state_int,
	UUtInt32 in_state_ptr_flags,
	void **in_state_ptr)
{
	GLenum gl_fill_mode;
	boolean constant_color_changed= FALSE;

	gl->state_int= (long *)in_state_int;
	gl->state_ptr= in_state_ptr;

#ifdef ENABLE_GL_FOG
	// update fogging
	if (in_state_int_flags & (1<<M3cDrawStateIntType_Fog))
	{
		if ((in_state_int[M3cDrawStateIntType_Fog] == M3cDrawStateFogDisable) &&
			gl->fog_enabled)
		{
			// disable fog
			GL_FXN(glDisable)(GL_FOG);
			gl->fog_enabled= FALSE;
		}
		else if ((in_state_int[M3cDrawStateIntType_Fog] == M3cDrawStateFogEnable) &&
			!gl->fog_enabled && !gl->fog_disabled_3Dfx)
		{
			GL_FXN(glEnable)(GL_FOG);
			gl->fog_enabled= TRUE;
		}
	}
#endif

#ifdef GL_ENABLE_ALPHA_FADE_CODE
	// special frame buffer blend mode (for the invisibility powerup)
	if (in_state_int_flags & (1<<M3cDrawStateIntType_FrameBufferBlendWithConstantAlpha))
	{
		if (in_state_int[M3cDrawStateIntType_FrameBufferBlendWithConstantAlpha])
		{
			gl->blend_to_frame_buffer_with_constant_alpha= TRUE;
		}
		else
		{
			gl->blend_to_frame_buffer_with_constant_alpha= FALSE;
		}
	}
#endif

	// set depth bias if required
	/* nothing uses this - disabling 10/26/2000 S.S.
	if ((in_state_int_flags & (1<<M3cDrawStateIntType_ZBias)) &&
		(in_state_int[M3cDrawStateIntType_ZBias] != 0))
	{
		gl_depth_bias_set((float)in_state_int[M3cDrawStateIntType_ZBias]);
	}
	else if (gl->depth_bias != 0.f)
	{
		gl_depth_bias_set(0.f);
	}*/

	// set depth mode if required
	if ((in_state_int_flags & (1<<M3cDrawStateIntType_ZCompare)) ||
		(in_state_int_flags & (1<<M3cDrawStateIntType_ZWrite)))
	{
		gl_depth_mode_set(
			((UUtBool)(in_state_int[M3cDrawStateIntType_ZCompare]) == M3cDrawState_ZCompare_On),
			((UUtBool)(in_state_int[M3cDrawStateIntType_ZWrite]) == M3cDrawState_ZWrite_On));
	}

	// setup texture maps
	gl->texture0= gl->texture1= NULL;

	// set base texture map
	if ((in_state_int[M3cDrawStateIntType_Appearence] != M3cDrawState_Appearence_Gouraud) &&
		(in_state_int[M3cDrawStateIntType_Fill] == M3cDrawState_Fill_Solid) &&
		(in_state_ptr_flags & (1 << M3cDrawStatePtrType_BaseTextureMap)))
	{
		// set texture0 to this base map
		gl->texture0= (M3tTextureMap*)in_state_ptr[M3cDrawStatePtrType_BaseTextureMap];
	}
	else
	{
		gl->texture0= NULL;
	}

	if (in_state_int_flags & (1<<M3cDrawStateIntType_Alpha))
	{
		if (gl->constant_color.a != (UUtUns8)in_state_int[M3cDrawStateIntType_Alpha])
		{
			gl->constant_color.a= (UUtUns8)in_state_int[M3cDrawStateIntType_Alpha];
			constant_color_changed= TRUE;
		}
	}

	if (in_state_int_flags & ((1<<M3cDrawStateIntType_ConstantColor)))
	{
		gl->constant_color.r= (byte)((in_state_int[M3cDrawStateIntType_ConstantColor]&0x00FF0000)>>16);
		gl->constant_color.g= (byte)((in_state_int[M3cDrawStateIntType_ConstantColor]&0x0000FF00)>>8);
		gl->constant_color.b= (byte)(in_state_int[M3cDrawStateIntType_ConstantColor]&0x000000FF);
		constant_color_changed= TRUE;
	}

	if (constant_color_changed)
	{
		GL_FXN(glColor4ub)(gl->constant_color.r, gl->constant_color.g, gl->constant_color.b, gl->constant_color.a);
	}

	// what does this mean?
	/*if (in_state_int_flags & ((1<<M3cDrawStateIntType_Appearence) |
		(1<<M3cDrawStateIntType_Interpolation) |
		(1<<M3cDrawStateIntType_Fill)))
	{
	}*/

	// determine the current geometry drawing mode
	switch (in_state_int[M3cDrawStateIntType_Fill])
	{
		case M3cDrawState_Fill_Point:
		case M3cDrawState_Fill_Line:
			gl->geom_draw_mode= _geom_draw_mode_wireframe;
			gl_fill_mode= GL_LINE;
			break;
		case M3cDrawState_Fill_Solid:
		default:
			gl_fill_mode= GL_FILL;
			switch (in_state_int[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					gl->geom_draw_mode= _geom_draw_mode_gourand;
					break;
				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Lit_EnvMap:
				case M3cDrawState_Appearence_Texture_Unlit:
					switch (in_state_int[M3cDrawStateIntType_VertexFormat])
					{
						case M3cDrawStateVertex_Unified:
							switch (in_state_int[M3cDrawStateIntType_Interpolation])
							{
								case M3cDrawState_Interpolation_None:
									gl->geom_draw_mode= _geom_draw_mode_flat;
									break;
								case M3cDrawState_Interpolation_Vertex:
									if ((((M3tTextureMap*)in_state_ptr[M3cDrawStatePtrType_EnvTextureMap]) != NULL) &&
										(gl->constant_color.a == 0xFF))
									{
										gl->texture0= (M3tTextureMap*)in_state_ptr[M3cDrawStatePtrType_EnvTextureMap];
										gl->texture1= (M3tTextureMap*)in_state_ptr[M3cDrawStatePtrType_BaseTextureMap];
										UUmAssert(gl->texture1);

										if (gl_save_geom_draw_mode == NONE)
										{
											if (ONrMotoko_GraphicsQuality_SupportReflectionMapping() == FALSE)
											{
												gl->geom_draw_mode= _geom_draw_mode_environment_map_low_quality;
											}
											else // single-pass multitexture (or multipass per-poly, yuck)
											{
												gl->geom_draw_mode= _geom_draw_mode_environment_map;
											}
										}
										else
										{
											boolean blam= TRUE;
										}
									}
									else
									{
										gl->geom_draw_mode= _geom_draw_mode_default;
									}
									break;
							}
							break;
						case M3cDrawStateVertex_Split:
							gl->geom_draw_mode= _geom_draw_mode_split;
							break;
					}
					break;
				default:
					break;
			}
			break;
	}

	// account for fill mode changes
	{
		static GLenum current_fill_mode= GL_FILL;

		if (gl_fill_mode != current_fill_mode)
		{
			GL_FXN(glPolygonMode)(GL_FRONT_AND_BACK, gl_fill_mode);
			UUmAssert(gl_GetError() == GL_NO_ERROR);
			current_fill_mode= gl_fill_mode;
		}
	}

	if ((in_state_int_flags & (1<<M3cDrawStateIntType_BufferClear)) &&
		(gl->buffer_clear= (boolean)in_state_int[M3cDrawStateIntType_BufferClear]))
	{
		gl->clear_color.a= (float)(((in_state_int[M3cDrawStateIntType_ClearColor]&0xFF000000)>>24) * ONE_OVER_255);
		gl->clear_color.r= (float)(((in_state_int[M3cDrawStateIntType_ClearColor]&0x00FF0000)>>16) * ONE_OVER_255);
		gl->clear_color.g= (float)(((in_state_int[M3cDrawStateIntType_ClearColor]&0x0000FF00)>>8) * ONE_OVER_255);
		gl->clear_color.b= (float)((in_state_int[M3cDrawStateIntType_ClearColor]&0x000000FF) * ONE_OVER_255);
		GL_FXN(glClearColor)(gl->clear_color.r, gl->clear_color.g, gl->clear_color.b, gl->clear_color.a);
	}

	if (in_state_int_flags & (1<<M3cDrawStateIntType_DoubleBuffer))
	{
		gl->double_buffer= (boolean)in_state_int[M3cDrawStateIntType_DoubleBuffer];
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return UUcError_None;
}

void gl_prepare_multipass_env_map(
	void)
{
	UUmAssert(gl_save_geom_draw_mode == NONE);
	UUmAssert(gl->multitexture == _gl_multitexture_none); // Use The Force, Luke
	gl_save_geom_draw_mode= gl->geom_draw_mode;
	gl->geom_draw_mode= _geom_draw_mode_environment_map_multipass;

	return;
}

void gl_prepare_multipass_base_map(
	void)
{
	UUmAssert(gl->multitexture == _gl_multitexture_none); // Use The Force, Luke
	gl->geom_draw_mode= _geom_draw_mode_base_map_multipass;

	return;
}

void gl_finish_multipass(
	void)
{
	gl->geom_draw_mode= gl_save_geom_draw_mode;
	gl_save_geom_draw_mode= NONE;

	return;
}

static UUtError gl_frame_start(
	UUtUns32 game_ticks_elapsed)
{
	if (gl->double_buffer)
	{
		GL_FXN(glDrawBuffer)(GL_BACK);
	}
	else
	{
		GL_FXN(glDrawBuffer)(GL_FRONT);
	}

	if (gl->buffer_clear)
	{
		GL_FXN(glClearColor)(gl->clear_color.r, gl->clear_color.g, gl->clear_color.b, gl->clear_color.a);
		GL_FXN(glClear)(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// update fog if required
#ifdef ENABLE_GL_FOG
	{
		static float fog_start= 0.f;
		static float fog_end= 0.f;
		static gl_color_4f fog_color= {FOG_COLOR_GL, FOG_COLOR_GL, FOG_COLOR_GL, 0.f};
		boolean fog_color_changed= FALSE;
		static boolean fog_was_disabled_3Dfx= FALSE;

		if (game_ticks_elapsed > 0) {
			gl_fog_update(game_ticks_elapsed);
		}

		if (gl->fog_start != fog_start)
		{
		#ifdef Z_SCALE
			GL_FXN(glFogf)(GL_FOG_START, gl->fog_start * Z_SCALE);
		#else
			GL_FXN(glFogf)(GL_FOG_START, gl->fog_start);
		#endif
			fog_start= gl->fog_start;
		}
		if (gl->fog_end != fog_end)
		{
		#ifdef Z_SCALE
			GL_FXN(glFogf)(GL_FOG_END, gl->fog_end * Z_SCALE);
		#else
			GL_FXN(glFogf)(GL_FOG_END, gl->fog_end);
		#endif
			fog_end= gl->fog_end;
		}
		if (gl->fog_color.r != fog_color.r)
		{
			fog_color.r= gl->fog_color.r;
			fog_color_changed= TRUE;
		}
		if (gl->fog_color.g != fog_color.g)
		{
			fog_color.g= gl->fog_color.g;
			fog_color_changed= TRUE;
		}
		if (gl->fog_color.b != fog_color.b)
		{
			fog_color.b= gl->fog_color.b;
			fog_color_changed= TRUE;
		}
		if (fog_color_changed)
		{
			GL_FXN(glFogfv)(GL_FOG_COLOR, (float *)&gl->fog_color);
		}
		// can toggle fog on 3Dfx boards for demonstrative purposes
		if (fog_was_disabled_3Dfx != gl->fog_disabled_3Dfx)
		{
			if (gl->fog_disabled_3Dfx == FALSE)
			{
				if (gl->fog_enabled)
				{
					GL_FXN(glEnable)(GL_FOG);
					gl->fog_enabled= TRUE;
				}
				else
				{
					GL_FXN(glDisable)(GL_FOG);
					gl->fog_enabled= FALSE;
				}
			}
			else
			{
				GL_FXN(glDisable)(GL_FOG);
			}
		}
		fog_was_disabled_3Dfx= gl->fog_disabled_3Dfx;
	}
#endif

	if (++gl->frame_count == MAX_INTERNAL_FRAME_COUNT_VALUE)
	{
		gl->frame_count= 0;
	}

//	COrConsole_Printf_Color(2, 0xFF30FF30, 0xFF307030, "texture memory used= %ld bytes", gl->current_texture_memory);

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return UUcError_None;
}

static UUtError gl_frame_end(
	UUtUns32 *out_texture_bytes_downloaded)
{
	*out_texture_bytes_downloaded = 0;

	if (gl->double_buffer)
	{
		gl->gl_display_back_buffer();
	}
	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return UUcError_None;
}

static UUtError gl_frame_sync(
	void)
{
	return UUcError_None;
}

static UUtError gl_screen_capture(
	const UUtRect *in_rect,
	void *out_buffer)
{
	GLint x= in_rect->left;
	GLint y= in_rect->top;
	GLsizei width= in_rect->right - in_rect->left + 1;
	GLsizei height= in_rect->bottom - in_rect->top + 1;

	// unnecessary gl_camera_update(_camera_mode_2d);

	GL_FXN(glReadBuffer)(GL_FRONT);
	GL_FXN(glReadPixels)(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, out_buffer);

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return UUcError_None;
}

static UUtBool gl_point_visible(
	const M3tPointScreen *in_point,
	float in_tolerance)
{
	GLfloat depth_val;
	GLint x= MUrUnsignedSmallFloat_To_Uns_Round(in_point->x);
	GLint y= gl->display_mode.height - MUrUnsignedSmallFloat_To_Uns_Round(in_point->y);

	GL_FXN(glReadBuffer)(GL_BACK);
	GL_FXN(glReadPixels)(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, (void *)&depth_val);
	UUmAssert(gl_GetError() == GL_NO_ERROR);

	// visible if the point isn't behind the distance stored in the depth buffer
	return (depth_val > in_point->z);
}

static UUtBool gl_single_pass_multitexture_capable(
	void)
{
	return (gl->multitexture != _gl_multitexture_none);
}

static UUtBool gl_support_depth_reads(
	void)
{
	return (!gl->depth_buffer_reads_disabled);
}

// gl_triangle()
#define GEOM_TYPE M3tTri
#define GEOM_SPLIT_TYPE M3tTriSplit
#define NUM_VERTICES 3
#define GEOMETRY_MODE GL_TRIANGLES
#define FUNCTION_NAME gl_triangle
#define COUNTER_NAME triCounter
#include "gl_geometry_draw_method.c"

// gl_quad()
#define GEOM_TYPE M3tQuad
#define GEOM_SPLIT_TYPE M3tQuadSplit
#define NUM_VERTICES 4
#define GEOMETRY_MODE GL_TRIANGLE_FAN //GL_QUADS
#define FUNCTION_NAME gl_quad
#define COUNTER_NAME quadCounter
#include "gl_geometry_draw_method.c"

// gl_pent()
#define GEOM_TYPE M3tPent
#define GEOM_SPLIT_TYPE M3tPentSplit
#define NUM_VERTICES 5
#define GEOMETRY_MODE GL_POLYGON
#define FUNCTION_NAME gl_pent
#define COUNTER_NAME pentCounter
#include "gl_geometry_draw_method.c"

static void gl_trisprite(
	const M3tPointScreen *in_points,
	const M3tTextureCoord *in_texture_coords)
{
	// unneccesary gl_camera_update(_camera_mode_2d);

	gl_set_textures(gl->texture0, NULL, NONE, NONE); // this will flush buffers if needed
	GL_FXN(glBegin)(GL_TRIANGLES);
		GL_FXN(glTexCoord2f)(in_texture_coords[0].u, in_texture_coords[0].v);
		GL_FXN(glVertex3f)(in_points[0].x, in_points[0].y, ZCOORD(in_points[0].z));
		GL_FXN(glTexCoord2f)(in_texture_coords[1].u, in_texture_coords[1].v);
		GL_FXN(glVertex3f)(in_points[1].x, in_points[1].y, ZCOORD(in_points[0].z));
		GL_FXN(glTexCoord2f)(in_texture_coords[2].u, in_texture_coords[2].v);
		GL_FXN(glVertex3f)(in_points[2].x, in_points[2].y, ZCOORD(in_points[0].z));
	GL_FXN(glEnd)();

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

static void gl_sprite(
	const M3tPointScreen *in_points,
	const M3tTextureCoord *in_texture_coords)
{
	// unneccesary gl_camera_update(_camera_mode_2d);

	gl_set_textures(gl->texture0, NULL, NONE, NONE); // this will flush buffers if needed
	GL_FXN(glBegin)(GL_QUADS);
		GL_FXN(glTexCoord2f)(in_texture_coords[0].u, in_texture_coords[0].v);
		GL_FXN(glVertex3f)(in_points[0].x, in_points[0].y, ZCOORD(in_points[0].z));
		GL_FXN(glTexCoord2f)(in_texture_coords[1].u, in_texture_coords[1].v);
		GL_FXN(glVertex3f)(in_points[1].x, in_points[0].y, ZCOORD(in_points[0].z));
		GL_FXN(glTexCoord2f)(in_texture_coords[3].u, in_texture_coords[3].v);
		GL_FXN(glVertex3f)(in_points[1].x, in_points[1].y, ZCOORD(in_points[0].z));
		GL_FXN(glTexCoord2f)(in_texture_coords[2].u, in_texture_coords[2].v);
		GL_FXN(glVertex3f)(in_points[0].x, in_points[1].y, ZCOORD(in_points[0].z));
	GL_FXN(glEnd)();

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

static void gl_sprite_array(
	const M3tPointScreen *in_points,
	const M3tTextureCoord *in_texture_coords,
	const UUtUns32 *in_colors,
	const UUtUns32 in_count)
{
	unsigned long i;

	// unneccesary gl_camera_update(_camera_mode_2d);

	for (i= 0; i<in_count; i++)
	{
		long i2= i*2, i4= i*4;

		gl_set_textures(gl->texture0, NULL, NONE, NONE); // this will flush buffers if needed
		GL_FXN(glBegin)(GL_QUADS);
			GL_FXN(glTexCoord2f)(in_texture_coords[i4].u, in_texture_coords[i4].v);
			GL_FXN(glVertex3f)(in_points[i2].x, in_points[i2].y, ZCOORD(in_points[0].z));
			GL_FXN(glTexCoord2f)(in_texture_coords[i4+1].u, in_texture_coords[i4+1].v);
			GL_FXN(glVertex3f)(in_points[i2+1].x, in_points[i2].y, ZCOORD(in_points[0].z));
			GL_FXN(glTexCoord2f)(in_texture_coords[i4+3].u, in_texture_coords[i4+3].v);
			GL_FXN(glVertex3f)(in_points[i2+1].x, in_points[i2+1].y, ZCOORD(in_points[0].z));
			GL_FXN(glTexCoord2f)(in_texture_coords[i4+2].u, in_texture_coords[i4+2].v);
			GL_FXN(glVertex3f)(in_points[i2].x, in_points[i2+1].y, ZCOORD(in_points[0].z));
		GL_FXN(glEnd)();
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

static void gl_point(
	M3tPointScreen *inv_coord)
{
	// unneccesary gl_camera_update(_camera_mode_2d);

	gl_set_textures(NULL, NULL, GL_ONE, GL_ZERO); // set blend mode

	GL_FXN(glBegin)(GL_POINTS);
		GL_FXN(glVertex3f)(inv_coord->x, inv_coord->y, ZCOORD(inv_coord->z));
	GL_FXN(glEnd)();

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

static void gl_line(
	UUtUns16 index0,
	UUtUns16 index1)
{
	const M3tPointScreen *screen_points= gl->state_ptr[M3cDrawStatePtrType_ScreenPointArray];
	const M3tPointScreen *our_screen_points[2];

	// unneccesary gl_camera_update(_camera_mode_2d);

	gl_set_textures(NULL, NULL, GL_ONE, GL_ZERO); // set blend mode

	our_screen_points[0]= screen_points + index0;
	our_screen_points[1]= screen_points + index1;

	GL_FXN(glBegin)(GL_LINES);
		GL_FXN(glVertex3f)(our_screen_points[0]->x, our_screen_points[0]->y, ZCOORD(our_screen_points[0]->z));
		GL_FXN(glVertex3f)(our_screen_points[1]->x, our_screen_points[1]->y, ZCOORD(our_screen_points[1]->z));
	GL_FXN(glEnd)();

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

#endif // __ONADA__
