/*
	gl_utility.c
	Thursday Aug. 10, 2000 8:02 pm
	Stefan
*/

#ifdef __ONADA__

/*---------- includes */

#include "gl_engine.h"
#include "BFW_ScriptLang.h" // for registering console variables
#include "Oni_Motoko.h" // for graphics quality

/*---------- structures */

struct gl_texel_type_info
{
	IMtPixelType motoko_texel_type;
	GLenum download_type;
	GLenum download_info;
	GLenum gl_internal_format;
};

typedef void (*gl_texture_map_download_proc)(
	const M3tTextureMap *texture_map,
	const struct gl_texel_type_info *texture_info,
	void *src, GLint level, GLsizei width, GLsizei height, void *buffer);

/*---------- prototypes */

static boolean gl_query_extension(char *extension);
static void *load_gl_extension_routine(char *extension_name);

/*static void gl_invert_texture_alpha_kludge(unsigned long texel_type,
	int width, int height, void *pixels, boolean env_map_fudge);*/

static boolean gl_enumerate_compressed_texture_formats(GLint format_array[],
	int num_array_elements, GLint *num_compressed_texture_formats);
static boolean gl_is_compressed_texture_format_available(GLint format);
static void gl_compress_texture_format(IMtPixelType oni_pixel_type, GLenum gl_internal_format);
static boolean gl_is_format_compressed(GLenum format);
static int gl_texture_bytes(IMtPixelType oni_format, int width, int height, boolean include_mapmaps);
static void gl_report_compression(const M3tTextureMap *texture_map,
	const struct gl_texel_type_info *texture_info, int level, int width, int height);
static char *gl_oni_texture_format_to_string(IMtPixelType oni_format);
static char *gl_texture_format_to_string(GLenum format);

static void gl_texture_map_download_generic(const M3tTextureMap *texture_map,
	const struct gl_texel_type_info *texture_info,
	void *src, GLint level, GLsizei width, GLsizei height, void *buffer);
static void gl_texture_map_download_classic(const M3tTextureMap *texture_map,
	const struct gl_texel_type_info *texture_info,
	void *src, GLint level, GLsizei width, GLsizei height, void *buffer);
static void gl_texture_map_download_DXT1(const M3tTextureMap *texture_map,
	const struct gl_texel_type_info *texture_info,
	void *src, GLint level, GLsizei width, GLsizei height, void *buffer);
static void gl_texture_map_download_packed_pixels(const M3tTextureMap *texture_map,
	const struct gl_texel_type_info *texture_info,
	void *src, GLint level, GLsizei width, GLsizei height, void *buffer);
static boolean gl_texture_map_reload(M3tTextureMap *texture_map);
static void gl_setup_multitexturing(void);
static void gl_setup_nv_combiners_for_env_maps(void);
static void gl_setup_texture_env_combine_for_env_maps(void);

static boolean gl_load_library(void);
static boolean gl_library_is_loaded(void);

static UUtError gl_fog_start_changeto_func(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue);
static UUtError gl_fog_end_changeto_func(SLtErrorContext *inErrorContext, UUtUns32 inParameterListLength,
										SLtParameter_Actual *inParameterList, UUtUns32 *outTicksTillCompletion,
										UUtBool *outStall, SLtParameter_Actual *ioReturnValue);

/*---------- globals */

static struct gl_texel_type_info gl_texel_type_info_table[] = {
	{IMcPixelType_ARGB4444, _gl_pixel_download_packed, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4},
	{IMcPixelType_RGB555, _gl_pixel_download_packed, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5},
	{IMcPixelType_ARGB1555, _gl_pixel_download_packed, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1},
	{IMcPixelType_I8, _gl_pixel_download_classic, GL_LUMINANCE, GL_LUMINANCE8},
	{IMcPixelType_I1, _gl_pixel_download_invalid, 0, GL_LUMINANCE8},
//#if UUmPlatform	== UUmPlatform_Mac
#if 0
	{IMcPixelType_A8, _gl_pixel_download_packed, GL_UNSIGNED_SHORT_4_4_4_4, // CB: this produces black text on ATI Macs */GL_ALPHA8 /*GL_RGBA4*/ },
//#else
	{IMcPixelType_A8, _gl_pixel_download_classic, GL_ALPHA, GL_ALPHA8 },
#endif
	{IMcPixelType_A8, _gl_pixel_download_packed, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4},
	
	{IMcPixelType_A4I4, _gl_pixel_download_generic, 0, GL_LUMINANCE4_ALPHA4},
	{IMcPixelType_ARGB8888, _gl_pixel_download_generic, 0, GL_RGBA},
	{IMcPixelType_RGB888, _gl_pixel_download_generic, 0, GL_RGB},
	{IMcPixelType_DXT1, _gl_pixel_download_S3, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5},
	{IMcPixelType_RGB_Bytes, _gl_pixel_download_classic, GL_RGB, GL_RGB},
	{IMcPixelType_RGBA_Bytes, _gl_pixel_download_classic, GL_RGBA, GL_RGBA},
	{IMcPixelType_RGBA5551, _gl_pixel_download_packed, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1},
	{IMcPixelType_RGBA4444, _gl_pixel_download_packed, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4},
	{IMcPixelType_RGB565, _gl_pixel_download_packed, GL_UNSIGNED_SHORT_5_6_5, GL_RGB5}};

struct gl_state_global *gl= NULL;
struct gl_api *gl_api= NULL;

/*---------- code */

boolean opengl_available(
	void)
{
	boolean success= FALSE;

	if ((gl == NULL) && (gl_api == NULL))
	{
		gl= (struct gl_state_global *)UUrMemory_Block_New(sizeof(struct gl_state_global));
		gl_api= (struct gl_api *)UUrMemory_Block_New(sizeof(struct gl_api));

		if ((gl != NULL) && (gl_api != NULL))
		{
			UUrMemory_Clear(gl, sizeof(struct gl_state_global));
			UUrMemory_Clear(gl_api, sizeof(struct gl_api));

			success= gl_load_library();
		}
	}
	else
	{
		success= gl_library_is_loaded();
	}

	return success;
}

boolean initialize_opengl(
	void)
{
	boolean success= TRUE;
	UUtError error= _error_none;
	
	UUrStartupMessage("OpenGL platform initialization");
	success= gl_platform_initialize();

	if ((success == TRUE) && (gl->engine_initialized == UUcFalse))
	{
		gl->vendor= (char *)GL_FXN(glGetString)(GL_VENDOR);
		gl->renderer= (char *)GL_FXN(glGetString)(GL_RENDERER);
		gl->version= (char *)GL_FXN(glGetString)(GL_VERSION);
		gl->extensions= (char *)GL_FXN(glGetString)(GL_EXTENSIONS);

		UUrStartupMessage("OpenGL vendor = %s", gl->vendor);
		UUrStartupMessage("OpenGL renderer = %s", gl->renderer);
		UUrStartupMessage("OpenGL version = %s", gl->version);
		UUrStartupMessage("OpenGL extensions = %s", gl->extensions);

		// vendor-specific code :(
		if (strstr(gl->renderer, "Matrox"))
		{
			gl->gl_display_back_buffer= gl_matrox_display_back_buffer;
		}
		else
		{
			gl->gl_display_back_buffer= gl_display_back_buffer;
		}

		GL_FXN(glGetIntegerv)(GL_MAX_TEXTURE_UNITS_ARB, (int*)&gl->num_tmu);
		GL_FXN(glGetIntegerv)(GL_MAX_TEXTURE_SIZE, (int*)&gl->max_texture_size);
		GL_FXN(glGetIntegerv)(GL_DOUBLEBUFFER, &gl->has_double_buffer);

		// init multitexturing extensions
		if (gl->num_tmu > 1 &&
			gl_query_extension("GL_ARB_multitexture") &&
			(GL_EXT(glActiveTextureARB)= load_gl_extension_routine("glActiveTextureARB")) &&
			(GL_EXT(glClientActiveTextureARB)= load_gl_extension_routine("glClientActiveTextureARB")) &&
			(GL_EXT(glMultiTexCoord2fvARB)= load_gl_extension_routine("glMultiTexCoord2fvARB")) &&
			(GL_EXT(glMultiTexCoord4fARB)= load_gl_extension_routine("glMultiTexCoord4fARB")))
		{
			UUrStartupMessage("multitexturing is available ...");
			GL_EXT(glActiveTextureARB)(GL_TEXTURE0_ARB);
			GL_EXT(glClientActiveTextureARB)(GL_TEXTURE0_ARB);
		#ifdef ENABLE_GL_ARB_MULTITEXTURING
			if (1)
		#else
			if (0)
		#endif
			{
				gl->multitexture= _gl_multitexture_arb;
				UUrStartupMessage("OpenGL ARB multitexturing being used");
			}
		#ifdef ENABLE_NVIDIA_MULTITEXTURING
			else if (gl_query_extension("GL_NV_register_combiners") &&
				(GL_EXT(glCombinerParameteriNV)= load_gl_extension_routine("glCombinerParameteriNV")) &&
				(GL_EXT(glCombinerInputNV)= load_gl_extension_routine("glCombinerInputNV")) &&
				(GL_EXT(glCombinerOutputNV)= load_gl_extension_routine("glCombinerOutputNV")) &&
				(GL_EXT(glFinalCombinerInputNV)= load_gl_extension_routine("glFinalCombinerInputNV")))
			{
				gl->multitexture= _gl_multitexture_nv;
				GL_FXN(glGetIntegerv)(GL_MAX_GENERAL_COMBINERS_NV, &gl->max_nvidia_general_combiners);
				UUrStartupMessage("nVidia register combiner multitexturing being used");
			}
		#endif
		#ifdef ENABLE_GL_EXT_texture_env_combine
			else if (gl_query_extension("GL_EXT_texture_env_combine") || gl_query_extension("GL_ARB_texture_env_combine"))
			{
				gl->multitexture= _gl_multitexture_texture_env_combine;
				UUrStartupMessage("GL_EXT_texture_env_combine being used");
			}
		#endif
			else
			{
				gl->multitexture= _gl_multitexture_none;
				UUrStartupMessage("multipass being used");
			}
		}
		else
		{
			gl->multitexture= _gl_multitexture_none;
			UUrStartupMessage("multipass being used");
		}

		UUrStartupMessage("OpenGL supports %d texturing units", gl->num_tmu);

		// init color blending extension
		GL_EXT(glBlendColor)= load_gl_extension_routine("glBlendColor"); // OpenGL 1.2
		if (GL_EXT(glBlendColor) == NULL)
		{
			// pre-1.2 extension
			GL_EXT(glBlendColor)= load_gl_extension_routine("glBlendColorEXT");
		}
		if (GL_EXT(glBlendColor))
		{
			UUrStartupMessage("glBlendColor() available");
		}

		// look for texture compression support
		// gl->compress_textures needs to be initialized before calling
		// gl_enumerate_compressed_texture_formats() since it does some magic.
		// gl->compress_textures should be user-configurable
		gl->compress_textures= TRUE;
		GL_EXT(glCompressedTexImage3DARB)= load_gl_extension_routine("glCompressedTexImage3DARB");
		GL_EXT(glCompressedTexImage2DARB)= load_gl_extension_routine("glCompressedTexImage2DARB");
		GL_EXT(glCompressedTexImage1DARB)= load_gl_extension_routine("glCompressedTexImage1DARB");
		GL_EXT(glCompressedTexSubImage3DARB)= load_gl_extension_routine("glCompressedTexSubImage3DARB");
		GL_EXT(glCompressedTexSubImage2DARB)= load_gl_extension_routine("glCompressedTexSubImage2DARB");
		GL_EXT(glCompressedTexSubImage1DARB)= load_gl_extension_routine("glCompressedTexSubImage1DARB");
		GL_EXT(glGetCompressedTexImageARB)= load_gl_extension_routine("glGetCompressedTexImageARB");
		if (gl_query_extension("GL_ARB_texture_compression") &&
			gl_enumerate_compressed_texture_formats(gl->compressed_texture_formats,
			MAX_COMPRESSED_TEXTURE_FORMATS, &gl->number_of_compressed_texture_formats))
		{
			UUrStartupMessage("OpenGL texture compression available");
		}
		else
		{
			gl->compress_textures= FALSE;
		}

		gl_sync_to_vtrace(TRUE);

		// initialize OpenGL state

		GL_FXN(glHint)(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		GL_FXN(glDisable)(GL_CULL_FACE);
		GL_FXN(glEnableClientState)(GL_VERTEX_ARRAY);
		GL_FXN(glEnable)(GL_BLEND);
		GL_FXN(glBlendFunc)(_gl_initial_sfactor, _gl_initial_dfactor);

		//GL_FXN(glEnable)(GL_POLYGON_OFFSET_FILL);
		//GL_FXN(glPolygonOffset)(0.f, 0.f);

		// set pixel packing
		GL_FXN(glPixelStorei)(GL_UNPACK_ALIGNMENT,1);
		GL_FXN(glPixelStorei)(GL_PACK_ALIGNMENT,1);

		gl->clear_color.r= 0.f;
		gl->clear_color.g= 0.f;
		gl->clear_color.b= 0.f;
		gl->clear_color.a= 1.f;

		// debugging switches
		gl->mipmap_offset = 0;
		gl->packed_pixels_disabled = FALSE;
		gl->dxt1_textures_disabled = FALSE;

		// fog
		gl->fog_color.r= FOG_COLOR_GL;
		gl->fog_color.g= FOG_COLOR_GL;
		gl->fog_color.b= FOG_COLOR_GL;
		gl->fog_color.a= 0.f;

		gl->fog_start= ONI_FOG_START;
		gl->fog_end= ONI_FOG_END;

	#ifdef ENABLE_GL_FOG
		
		GL_FXN(glFogi)(GL_FOG_MODE, GL_LINEAR);
		GL_FXN(glFogfv)(GL_FOG_COLOR, (float *)&gl->fog_color);
	#ifdef Z_SCALE
		GL_FXN(glFogf)(GL_FOG_START, gl->fog_start * Z_SCALE);
		GL_FXN(glFogf)(GL_FOG_END, gl->fog_end * Z_SCALE);
	#else
		GL_FXN(glFogf)(GL_FOG_START, gl->fog_start);
		GL_FXN(glFogf)(GL_FOG_END, gl->fog_end);
	#endif

		SLrGlobalVariable_Register_Float("gl_fog_end", "fog end", &gl->fog_end);
		SLrGlobalVariable_Register_Float("gl_fog_start", "fog start", &gl->fog_start);
		SLrGlobalVariable_Register_Float("gl_fog_red", "fog red", &gl->fog_color.r);
		SLrGlobalVariable_Register_Float("gl_fog_green", "fog green", &gl->fog_color.g);
		SLrGlobalVariable_Register_Float("gl_fog_blue", "fog blue", &gl->fog_color.b);

		SLrScript_Command_Register_Void("gl_fog_start_changeto", "changes the fog start distance smoothly", "start_val:float [frames:int | ]", gl_fog_start_changeto_func);
		SLrScript_Command_Register_Void("gl_fog_end_changeto", "changes the fog end distance smoothly", "end_val:float [frames:int | ]", gl_fog_end_changeto_func);

#if THE_DAY_IS_MINE
		SLrGlobalVariable_Register_Bool("gl_disable_depth_reads", "disable_depth_reads", &gl->depth_buffer_reads_disabled);
		SLrGlobalVariable_Register_Bool("gl_disable_packed_pixels", "lets you disable packed-pixels support", &gl->packed_pixels_disabled);
		SLrGlobalVariable_Register_Bool("gl_disable_dxt1", "lets you disable use of DXT1", &gl->dxt1_textures_disabled);
		SLrGlobalVariable_Register_Int32("gl_mipmap_offset", "lets you disable miplevels (-ve = disable big, +ve = disable small)", &gl->mipmap_offset);
#endif

	#ifdef DISABLE_FOG_ON_3DFX
	
		if (strstr(gl->renderer, "3Dfx") || strstr(gl->renderer, "3dfx"))
		{
		// 3Dfx cards on Mac handle fog correctly (also, vendor string reported as "3dfx" instead of "3Dfx" but we don't want to rely on that)
		#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
			gl->fog_disabled_3Dfx= TRUE;
			gl->depth_buffer_reads_disabled= TRUE;
			SLrGlobalVariable_Register_Bool("gl_fog_disabled", "fog disabled", &gl->fog_disabled_3Dfx);
		#endif
		}
		else if (strstr(gl->renderer, "formac")) // Mac card that doesn't handle our fog correctly
		{
			gl->fog_disabled_3Dfx= TRUE;
			SLrGlobalVariable_Register_Bool("gl_fog_disabled", "fog disabled", &gl->fog_disabled_3Dfx);
		}
		else
	#endif
		{
			GL_FXN(glEnable)(GL_FOG);
			gl->fog_enabled= TRUE;
		}

	#endif
	
	#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
		// glReadPixels() has proven to be prohibitively slow on current generation ATI
		// and 3Dfx hardware (ie, the Mac video card market)
		gl->depth_buffer_reads_disabled= TRUE;
	#endif
			
		GL_FXN(glClearColor)(gl->clear_color.r, gl->clear_color.g, gl->clear_color.b, gl->clear_color.a);
		GL_FXN(glClear)(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
		gl_depth_mode_set(TRUE, TRUE);

		GL_FXN(glViewport)(0, 0, gl->display_mode.width, gl->display_mode.height);
		GL_FXN(glDepthRange)(0.f, 1.f);
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return success;
}

boolean gl_query_extension(
	char *extension)
{	
	UUmAssert(gl->extensions);
	UUmAssert(extension && extension[0] != '\0');

	return (strstr(gl->extensions, extension) != NULL);
}

static void *load_gl_extension_routine(
	char *extension_name)
{
	void *extension_routine= NULL;

	UUmAssert(extension_name);

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
	extension_routine= WGL_FXN(wglGetProcAddress)(extension_name);
#elif defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
	extension_routine = AGL_FXN(aglGetProcAddress)(extension_name);
#else
	#error "unknown platform"
#endif
	
	UUmAssert(gl_GetError() == GL_NO_ERROR);
	
	return extension_routine;
}

void gl_reset_fog_parameters(
	void)
{
#ifdef ENABLE_GL_FOG
	//if (gl->fog_enabled)
	gl->fog_color.r= FOG_COLOR_GL;
	gl->fog_color.g= FOG_COLOR_GL;
	gl->fog_color.b= FOG_COLOR_GL;
	gl->fog_color.a= 0.f;

	gl->fog_start= ONI_FOG_START;
	gl->fog_end= ONI_FOG_END;

	gl->fog_start_changing = FALSE;
	gl->fog_end_changing = FALSE;

	GL_FXN(glFogfv)(GL_FOG_COLOR, (float *)&gl->fog_color);
#ifdef Z_SCALE
	GL_FXN(glFogf)(GL_FOG_START, gl->fog_start * Z_SCALE);
	GL_FXN(glFogf)(GL_FOG_END, gl->fog_end * Z_SCALE);
#else
	GL_FXN(glFogf)(GL_FOG_START, gl->fog_start);
	GL_FXN(glFogf)(GL_FOG_END, gl->fog_end);
#endif

#endif
	
	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

static UUtError gl_fog_start_changeto_func(
	SLtErrorContext *inErrorContext,
	UUtUns32 inParameterListLength,
	SLtParameter_Actual *inParameterList,
	UUtUns32 *outTicksTillCompletion,
	UUtBool *outStall,
	SLtParameter_Actual *ioReturnValue)
{
	int frames= 0;
	float fog_val= inParameterList[0].val.f;

	if (inParameterListLength >= 2)
	{
		frames= inParameterList[1].val.i;
	}

	if (frames <= 0)
	{
		// set fog directly
		gl->fog_start= fog_val;
	}
	else
	{
		// set up fog interpolation
		gl->fog_start_desired= fog_val;
		gl->fog_start_delta= (gl->fog_start_desired - gl->fog_start) / frames;
		gl->fog_start_changing= TRUE;
	}

	return UUcError_None;
}

static UUtError gl_fog_end_changeto_func(
	SLtErrorContext *inErrorContext,
	UUtUns32 inParameterListLength,
	SLtParameter_Actual *inParameterList,
	UUtUns32 *outTicksTillCompletion,
	UUtBool *outStall,
	SLtParameter_Actual *ioReturnValue)
{
	int frames= 0;
	float fog_val= inParameterList[0].val.f;

	if (inParameterListLength >= 2)
	{
		frames= inParameterList[1].val.i;
	}

	if (frames <= 0)
	{
		// set fog directly
		gl->fog_end = fog_val;
	}
	else
	{
		// set up fog interpolation
		gl->fog_end_desired= fog_val;
		gl->fog_end_delta= (gl->fog_end_desired - gl->fog_end) / frames;
		gl->fog_end_changing= TRUE;
	}

	return UUcError_None;
}

void gl_fog_update(
	int frames)
{
#ifdef ENABLE_GL_FOG
	if (gl->fog_start_changing)
	{
		float delta= gl->fog_start_delta * frames;

		if (delta > 0)
		{
			// we are ramping up the fog_start
			if (gl->fog_start < gl->fog_start_desired - delta)
			{
				gl->fog_start+= delta;
			}
			else
			{
				gl->fog_start= gl->fog_start_desired;
				gl->fog_start_changing= FALSE;
			}

		}
		else if (delta < 0)
		{
			// we are ramping down the fog_start
			if (gl->fog_start > gl->fog_start_desired - delta)
			{
				gl->fog_start+= delta;
			}
			else
			{
				gl->fog_start= gl->fog_start_desired;
				gl->fog_start_changing= FALSE;
			}
		}
	}

	if (gl->fog_end_changing)
	{
		float delta= gl->fog_start_delta * frames;

		if (delta > 0)
		{
			// we are ramping up the fog_end
			if (gl->fog_end < gl->fog_end_desired - delta)
			{
				gl->fog_end+= delta;
			}
			else
			{
				gl->fog_end= gl->fog_end_desired;
				gl->fog_end_changing= FALSE;
			}

		} 
		else if (delta < 0)
		{
			// we are ramping down the fog_end
			if (gl->fog_end > gl->fog_end_desired - delta)
			{
				gl->fog_end+= delta;
			}
			else
			{
				gl->fog_end= gl->fog_end_desired;
				gl->fog_end_changing= FALSE;
			}
		}
	}
#endif

	return;
}

void gl_camera_update(
	word camera_mode)
{
	if (camera_mode == _camera_mode_2d && gl->camera_mode == _camera_mode_2d)
	{
		// nothing to do
	}
	else if(camera_mode == _camera_mode_3d)
	{
		//M3tManager_GeomCamera *active_camera;

		UUmAssert(0); // uncharted territory ahead ...
/*		
		M3rCamera_GetActive((M3tGeomCamera**)&active_camera);
		M3rManager_Camera_UpdateMatrices(active_camera);
		
		if (gl->camera_mode != _camera_mode_3d)
		{
			gl->camera_mode= camera_mode;
			gl->update_camera_static_data= TRUE;
			gl->update_camera_view_data= TRUE;
		}

		if (gl->update_camera_view_data)
		{
			glLoadIdentity();
			gluLookAt(
				active_camera->cameraLocation.x,
				active_camera->cameraLocation.y,
				ZCOORD(active_camera->cameraLocation.z),
				active_camera->cameraLocation.x + active_camera->viewVector.x,
				active_camera->cameraLocation.y + active_camera->viewVector.y,
				ZCOORD(active_camera->cameraLocation.z + active_camera->viewVector.z),
				active_camera->upVector.x,
				active_camera->upVector.y,
				ZCOORD(active_camera->upVector.z));
		}
		
		if (gl->update_camera_static_data)
		{
			#define _180_over_pi 57.295779513082320876798154814105
			glLoadIdentity();
			gluPerspective(
				active_camera->fovy * _180_over_pi,
				active_camera->aspect,
				ZCOORD(active_camera->zNear),
				ZCOORD(active_camera->zFar));
		}*/
	}
	else
	{
		gl->camera_mode= _camera_mode_2d;

		GL_FXN(glMatrixMode)(GL_PROJECTION);
		GL_FXN(glLoadIdentity)();
	#ifdef Z_SCALE
		GL_FXN(glOrtho)(0.f, gl->display_mode.width, gl->display_mode.height, 0.f, 0.f, (Z_SCALE));
	#else
		GL_FXN(glOrtho)(0.f, gl->display_mode.width, gl->display_mode.height, 0.f, 0.f, 1.f);
	#endif
		GL_FXN(glMatrixMode)(GL_MODELVIEW);
		GL_FXN(glLoadIdentity)();
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

void gl_matrix4x3_oni_to_gl(
	M3tMatrix4x3 *in_matrix,
	float *m)
{
	float **inM= (float **)in_matrix->m;

	*m++ = inM[0][0]; *m++ = inM[0][1]; *m++ = inM[0][2]; *m++ = 0.0f;
	*m++ = inM[1][0]; *m++ = inM[1][1]; *m++ = inM[1][2]; *m++ = 0.0f;
	*m++ = inM[2][0]; *m++ = inM[2][1]; *m++ = inM[2][2]; *m++ = 0.0f;
	*m++ = inM[3][0]; *m++ = inM[3][1]; *m++ = inM[3][2]; *m++ = 1.0f;
	
	return;
}

void gl_matrix4x4_oni_to_gl(
	M3tMatrix4x4 *in_matrix,
	float *m)
{
	float **inM= (float **)in_matrix->m;
	
	*m++ = inM[0][0]; *m++ = inM[0][1]; *m++ = inM[0][2]; *m++ = inM[0][3];
	*m++ = inM[1][0]; *m++ = inM[1][1]; *m++ = inM[1][2]; *m++ = inM[1][3];
	*m++ = inM[2][0]; *m++ = inM[2][1]; *m++ = inM[2][2]; *m++ = inM[2][3];
	*m++ = inM[3][0]; *m++ = inM[3][1]; *m++ = inM[3][2]; *m++ = inM[3][3];
	
	return;
}

UUtBool gl_texture_map_format_available(
	IMtPixelType texel_type)
{
	// this is totally meaningless
	return (gl_texel_type_info_table[texel_type].download_info != _gl_pixel_download_invalid);
}

void gl_depth_mode_set(
	boolean read,
	boolean write)
{
	static int depth_enabled= FALSE;
	static int depth_read= FALSE;
	static int depth_write= FALSE;

	if (read || write)
	{
		if (!depth_enabled)
		{
			depth_enabled= TRUE;
			/* when re-enabling, force updates */
			depth_read= !read;
			depth_write= !write;
		}
		if ((read && !depth_read) || (!read && depth_read))
		{
			GL_FXN(glEnable)(GL_DEPTH_TEST);
			GL_FXN(glDepthFunc)(read ? GL_LEQUAL : GL_ALWAYS);
			depth_read= read;
		}
		if ((write && !depth_write) || (!write && depth_write))
		{
			GL_FXN(glDepthMask)(write ? GL_TRUE : GL_FALSE);
			depth_write= write;
		}
	}
	else
	{
		if (depth_enabled)
		{
			GL_FXN(glDisable)(GL_DEPTH_TEST);
			GL_FXN(glDepthFunc)(GL_ALWAYS);
			GL_FXN(glDepthMask)(GL_FALSE);
			depth_enabled= FALSE;
			depth_read= FALSE;
			depth_write= FALSE;
		}
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

void gl_depth_bias_set(
	float bias)
{
	if (bias != gl->depth_bias)
	{
		GL_FXN(glPolygonOffset)(1.f, bias);
		gl->depth_bias= bias;

		if (bias != 0.f)
		{
			GL_FXN(glEnable)(GL_POLYGON_OFFSET_FILL);
		}
		else
		{
			GL_FXN(glDisable)(GL_POLYGON_OFFSET_FILL);
		}
	}
	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

// S.S. 11/09/2000 added for texture management on the Mac
static void gl_set_texture_timestamp(
	M3tTextureMap *texture_map)
{
	UUmAssert(texture_map);
	UUmAssert(gl);
	
	texture_map->pad1[0]= (UUtUns8)(gl->frame_count & 0x000000FF);
	texture_map->pad1[1]= (UUtUns8)((gl->frame_count & 0x0000FF00) >> 8);
	texture_map->pad1[2]= (UUtUns8)((gl->frame_count & 0x00FF0000) >> 16);
	
	return;
}

static UUtUns32 gl_get_texture_timestamp(
	M3tTextureMap *texture_map)
{
	UUtUns32 timestamp;
	
	UUmAssert(texture_map);
	UUmAssert(gl);
	
	timestamp= texture_map->pad1[0] |
		(((UUtUns32)texture_map->pad1[1] << 8) & 0x0000FF00) |
		(((UUtUns32)texture_map->pad1[2] << 16) & 0x00FF0000);
	
	return timestamp;
}

static int UUcExternal_Call gl_sort_textures_by_timestamp_proc(
	const void *t0,
	const void *t1)
{
	// sort so that least recently used textures are at the front of the list; most recently used at the tail end
	UUtUns32 timestamp0, timestamp1;
	int ret;
	
	timestamp0= gl_get_texture_timestamp((M3tTextureMap *)t0);
	timestamp1= gl_get_texture_timestamp((M3tTextureMap *)t1);
	
	if (timestamp0 < timestamp1)
	{
		ret= -1;
	}
	else if (timestamp0 > timestamp1)
	{
		ret= 1;
	}
	else
	{
		ret= 0;
	}
	
	return ret;
}

static void gl_sort_textures_by_timestamp(
	void)
{
	if (gl && gl->loaded_texture_array && (gl->num_loaded_textures > 0))
	{
		qsort(gl->loaded_texture_array, gl->num_loaded_textures, sizeof(M3tTextureMap *), gl_sort_textures_by_timestamp_proc);
	}
	
	return;
}

static void gl_purge_old_textures_with_memory_limit(UUtUns32 texture_memory_limit)
{
	if (gl->current_texture_memory > texture_memory_limit)
	{
		UUtUns32 desired_resident_texture_memory_size;
		UUtUns32 start_index= 0;
		
		gl_sort_textures_by_timestamp();
		
                
                if (texture_memory_limit > DESIRED_MINIMUM_FREE_TEXTURE_MEMORY)
                    desired_resident_texture_memory_size= texture_memory_limit - DESIRED_MINIMUM_FREE_TEXTURE_MEMORY;
                else
                    desired_resident_texture_memory_size= texture_memory_limit;
                    
		while ((gl->current_texture_memory > desired_resident_texture_memory_size) &&
			(gl->num_loaded_textures > MINIMUM_TEXTURE_PURGE_LIMIT) &&
			gl->loaded_texture_array[start_index])
		{
			// never purge offscreen textures
			while ((gl->loaded_texture_array[start_index]->flags & M3cTextureFlags_Offscreen))
			{
				if (++start_index >= gl->num_loaded_textures) break;
			}
			if (start_index >= gl->num_loaded_textures) break;
			// purge the oldest texture
			if (gl->loaded_texture_array[start_index])
			{
				if (gl_texture_map_delete(gl->loaded_texture_array[start_index]) == UUcFalse)
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
	}
}

static void gl_purge_old_textures_as_needed(
	void)
{
	static UUtUns32 texture_memory_limit= 0L;
	// texture_memory_limit is a desired upper limit for the amount of OpenGL textures to keep in memory
	// (can be on the card or in RAM)
	
	if (texture_memory_limit == 0L)
	{
	#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
		extern UUtUns32 gl_get_mac_texture_memory(void); // gl_macos.c

		texture_memory_limit= gl_get_mac_texture_memory();
	#else
		texture_memory_limit= MAX_DEFAULT_TEXTURE_MEMORY_LIMIT;
	#endif
	}
	
	gl_purge_old_textures_with_memory_limit(texture_memory_limit);
}

static boolean gl_texture_list_add(
	M3tTextureMap *texture_map)
{
	boolean success= FALSE;
	
	UUmAssert(texture_map);
	UUmAssert(texture_map->opengl_texture_name);
	
	if (gl->loaded_texture_array == NULL)
	{
		gl->loaded_texture_array= (M3tTextureMap **)UUrMemory_Block_NewClear(KILO * sizeof(M3tTextureMap *));
		if (gl->loaded_texture_array)
		{
			gl->loaded_texture_array_length= KILO;
		}
	}
	
	if (gl->loaded_texture_array)
	{
		if (gl->num_loaded_textures < gl->loaded_texture_array_length)
		{
			gl->loaded_texture_array[gl->num_loaded_textures]= texture_map;
			++gl->num_loaded_textures;
			success= TRUE;
		}
		else // need to grow array
		{
			M3tTextureMap **temp= (M3tTextureMap **) UUrMemory_Block_NewClear((KILO + gl->loaded_texture_array_length) * sizeof(M3tTextureMap *));
			
			if (temp)
			{
				UUrMemory_MoveFast(gl->loaded_texture_array, temp, gl->loaded_texture_array_length * sizeof(M3tTextureMap *));
				UUrMemory_Block_Delete(gl->loaded_texture_array);
				gl->loaded_texture_array= temp;
				gl->loaded_texture_array_length+= KILO;
				gl->loaded_texture_array[gl->num_loaded_textures]= texture_map;
				++gl->num_loaded_textures;
				success= TRUE;
			}
		}
	}
	
	if (success)
	{
		gl_set_texture_timestamp(texture_map);
		gl->current_texture_memory+= gl_texture_bytes(texture_map->texelType, texture_map->width, texture_map->height,
			((texture_map->flags&M3cTextureFlags_HasMipMap) ? TRUE : FALSE) /* mipmap */);
	}

	return success;
}

static boolean gl_texture_list_remove(
	M3tTextureMap *texture_map)
{
	boolean success= FALSE;
	UUtUns32 i= 0;
	
	UUmAssert(texture_map);
	UUmAssert(texture_map->opengl_texture_name);
	
	while (i < gl->num_loaded_textures)
	{
		if (gl->loaded_texture_array[i] == texture_map)
		{
			if (i < (gl->num_loaded_textures - 1)) // keep the array packed
			{
				UUtUns32 size= (gl->num_loaded_textures - (i+1)) * sizeof(M3tTextureMap *);
				M3tTextureMap **src= gl->loaded_texture_array+i+1;
				M3tTextureMap **dst= gl->loaded_texture_array+i;
				
				UUrMemory_MoveOverlap(src, dst, size);
			}
			--gl->num_loaded_textures;
			gl->loaded_texture_array[gl->num_loaded_textures]= NULL;
			gl->current_texture_memory-= gl_texture_bytes(texture_map->texelType, texture_map->width, texture_map->height,
				((texture_map->flags&M3cTextureFlags_HasMipMap) ? TRUE : FALSE) /* mipmap */);
			success= TRUE;
			break;
		}
		++i;
	}

	return success;
}

UUtBool gl_texture_map_create(
	M3tTextureMap *texture_map)
{
	const struct gl_texel_type_info *texture_info;
	gl_texture_map_download_proc download_proc;
	boolean mipmap;
	UUtBool success= TRUE;
	unsigned long lod;
	word width, height, minsize;
	int disable_large_lods, disable_small_lods;
	void *base;
	UUtBool supports_packed_pixels = ((gl->packed_pixels_disabled == UUcFalse) &&
		//  these appear to be functionally equivalent
		(gl_query_extension("GL_EXT_packed_pixels") || gl_query_extension("GL_APPLE_packed_pixel")));

	UUmAssert(texture_map);
	UUmAssert(texture_map->opengl_dirty == TRUE);

	download_proc= gl_texture_map_download_generic;
	texture_info= &gl_texel_type_info_table[texture_map->texelType];
	
	gl_purge_old_textures_as_needed();
		
	switch (texture_info->download_type)
	{
		case _gl_pixel_download_S3:
			// is S3 supported?
			if ((!gl->dxt1_textures_disabled) &&
				gl_is_compressed_texture_format_available(GL_COMPRESSED_RGB_S3TC_DXT1_EXT))
			{
				download_proc= gl_texture_map_download_DXT1;
			}
			else if (supports_packed_pixels)
			{
				download_proc= gl_texture_map_download_packed_pixels;
			}
			break;
		case _gl_pixel_download_packed:
			if (supports_packed_pixels)
			{
				download_proc= gl_texture_map_download_packed_pixels;
			}
			break;
		case _gl_pixel_download_classic:
			download_proc= gl_texture_map_download_classic;
			break;
		case _gl_pixel_download_generic:
			break;
		default:
			success= FALSE;
			break;
	}
	
	mipmap= (texture_map->flags&M3cTextureFlags_HasMipMap) ? TRUE : FALSE;
        
	// generate opengl name
	if (texture_map->opengl_texture_name == 0)
	{
		GL_FXN(glGenTextures)(1, (unsigned int*)&texture_map->opengl_texture_name);
		success= (texture_map->opengl_texture_name == 0) ? FALSE : TRUE;
	}

	if (success)
	{
		disable_large_lods = disable_small_lods = minsize = 0;

		if (mipmap) {
			// determine which lods we want to load
			if (gl->mipmap_offset > 0) {
				disable_large_lods = gl->mipmap_offset;
			} else if (gl->mipmap_offset < 0) {
				disable_small_lods = -gl->mipmap_offset;
			}

			if (!ONrMotoko_GraphicsQuality_SupportHighQualityTextures()) {
				// the first lod only gets loaded if graphics quality is higher than minimum setting
				disable_large_lods = UUmMax(1, disable_large_lods);
			}

			if (disable_small_lods > 0) {
				minsize = (1 << disable_small_lods);
			}
		}

		// CB: load the texture if it isn't already
		M3rTextureMap_TemporarilyLoad(texture_map, disable_large_lods);
		if (texture_map->pixels != NULL) {
			// we have the texture, nothing from here on can fail
			GL_FXN(glBindTexture)(GL_TEXTURE_2D, texture_map->opengl_texture_name);

			if (texture_map->flags&M3cTextureFlags_ClampHoriz)
			{
				GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			}
			else
			{
				GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			}
			if (texture_map->flags&M3cTextureFlags_ClampVert)
			{
				GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			else
			{
				GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}

			GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			
			if (mipmap)
			{
				static boolean trilinear_mipmapping_supported= TRUE;

				if (trilinear_mipmapping_supported && 
					// no trilinear filtering if graphics quality is minimum setting
					ONrMotoko_GraphicsQuality_SupportTrilinear())
				{
					GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					if (gl_GetError() != GL_NO_ERROR) // trilinear filtering not supported
					{
						trilinear_mipmapping_supported= FALSE;
						GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
					}
				}
				else
				{
					GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}
			}
			else
			{
				GL_FXN(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			}
			UUmAssert(gl_GetError() == GL_NO_ERROR);

			lod = 0;
			width= texture_map->width;
			height= texture_map->height;
			base= texture_map->pixels;

			while ((width > 0) && (height > 0))
			{
				if ((disable_large_lods > 0) && (width > 1) && (height > 1)) {
					// don't load this lod, we are discarding high-end textures
					disable_large_lods--;

				} else if ((lod > 0) && ((width <= minsize) || (height <= minsize))) {
					// don't load this lod, we are discarding small textures (and we have
					// already loaded at least one lod)
					break;

				} else {
					// load the lod
					download_proc(texture_map, texture_info, base, lod,
						width, height, gl->converted_data_buffer);
					lod++;

					if (!mipmap) {
						// we only want one lod anyway
						break;
					}
				}

				base= ((char *)base)+IMrImage_ComputeSize(texture_map->texelType, IMcNoMipMap, width, height);
				width>>= 1;
				height>>= 1;

				if ((width > 0) || (height > 0))
				{
					width= UUmMax(width, 1);
					height= UUmMax(height, 1);
				}
			}
			
			UUmAssert(GL_FXN(glIsTexture)(texture_map->opengl_texture_name) == GL_TRUE);
			UUmAssert(gl_GetError() == GL_NO_ERROR);
			
			texture_map->opengl_dirty= FALSE;
			success= gl_texture_list_add(texture_map);
			UUmAssert(success);
		}
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	// CB: if this texturemap was loaded into temporary storage for the conversion
	// process, unload it now
	M3rTextureMap_UnloadTemporary(texture_map);

	return success;
}

static void gl_texture_map_download_generic(
	const M3tTextureMap *texture_map,
	const struct gl_texel_type_info *texture_info,
	void *src,
	GLint level,
	GLsizei width,
	GLsizei height,
	void *buffer)
{
	UUtError error;
	IMtPixelType new_pixel_type;
	GLenum format;

	if (IMrPixelType_HasAlpha(texture_map->texelType))
	{
		new_pixel_type= IMcPixelType_RGBA_Bytes;
		format= GL_RGBA;
	}
	else
	{
		new_pixel_type= IMcPixelType_RGB_Bytes;
		format= GL_RGB;
	}

	error= IMrImage_ConvertPixelType(IMcDitherMode_Off, 
		width, height, IMcNoMipMap, texture_map->texelType, src,
		new_pixel_type, buffer);
	UUmAssert(error == UUcError_None);

	if (error == UUcError_None)
	{
		GL_FXN(glTexImage2D)(GL_TEXTURE_2D, level,
			texture_info->gl_internal_format, // GL format
			width, height, 0, format, GL_UNSIGNED_BYTE, buffer);
	#ifdef REPORT_ON_COMPRESSION_GL
		gl_report_compression(texture_map, texture_info, level, width, height);
	#endif
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

static void gl_texture_map_download_packed_pixels(
	const M3tTextureMap *texture_map,
	const struct gl_texel_type_info *texture_info,
	void *src,
	GLint level,
	GLsizei width,
	GLsizei height,
	void *buffer)
{
	UUtError error = UUcError_None;
	IMtPixelType dst_pixel_type;
	UUtBool in_place = UUcTrue;
	void *tex_image_2d_buffer = src;

	switch(texture_map->texelType)
	{
		case IMcPixelType_A8:
		case IMcPixelType_ARGB4444:
			dst_pixel_type = IMcPixelType_RGBA4444;
		break;

		case IMcPixelType_ARGB1555:
		case IMcPixelType_RGB555:
			dst_pixel_type = IMcPixelType_RGBA5551;
		break;
		
		case IMcPixelType_DXT1:
			dst_pixel_type = IMcPixelType_RGBA5551;
		break;

		case IMcPixelType_RGBA5551:
		case IMcPixelType_RGBA4444:
			dst_pixel_type = texture_map->texelType;
		break;

		case IMcPixelType_RGB565:
			dst_pixel_type = texture_map->texelType;
		break;

		default:
		break;
	}

	if (dst_pixel_type != texture_map->texelType) {
		error = IMrImage_ConvertPixelType(
			IMcDitherMode_Off, 
			width,
			height,
			IMcNoMipMap,
			texture_map->texelType,
			src,
			dst_pixel_type,
			buffer);
		UUmAssert(UUcError_None == error);

		tex_image_2d_buffer = buffer;
	}

	GL_FXN(glTexImage2D)(
		GL_TEXTURE_2D,
		level,
		texture_info->gl_internal_format,	// requested format to store on the card or 0 (GLenum)
		width,								// width
		height,								// height
		0,									// border {?} (GLint)
		GL_RGBA,							// format of the pixel data (GLenum)
		texture_info->download_info,		// packed format (GLenum) 
		tex_image_2d_buffer);

	{
		UUtInt32 error_code_test = GL_FXN(glGetError)();
		UUtInt32 breakpoint_here = 0;
	}
	
	return;
}

static void gl_texture_map_download_classic(
	const M3tTextureMap *texture_map,
	const struct gl_texel_type_info *texture_info,
	void *src,
	GLint level,
	GLsizei width,
	GLsizei height,
	void *buffer)
{
	switch(texture_map->texelType)
	{
		case IMcPixelType_A8:
			GL_FXN(glPixelTransferf)(GL_RED_BIAS, 1.f);
			GL_FXN(glPixelTransferf)(GL_GREEN_BIAS, 1.f);
			GL_FXN(glPixelTransferf)(GL_BLUE_BIAS, 1.f);
		break;
                default:
                        break;
	}

	GL_FXN(glTexImage2D)(GL_TEXTURE_2D, level,
		texture_info->gl_internal_format, // GL format
		width, height, 0, texture_info->download_info,	
		GL_UNSIGNED_BYTE, src);
	#ifdef REPORT_ON_COMPRESSION_GL
		gl_report_compression(texture_map, texture_info, level, width, height);
	#endif

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	switch(texture_map->texelType)
	{
		case IMcPixelType_A8:
			GL_FXN(glPixelTransferf)(GL_RED_BIAS, 0.f);
			GL_FXN(glPixelTransferf)(GL_GREEN_BIAS, 0.f);
			GL_FXN(glPixelTransferf)(GL_BLUE_BIAS, 0.f);
		break;
                default:
                        break;
	}

	return;
}

static void gl_texture_map_download_DXT1(
	const M3tTextureMap *texture_map,
	const struct gl_texel_type_info *texture_info,
	void *src,
	GLint level,
	GLsizei width,
	GLsizei height,
	void *buffer)
{
	if (gl_is_compressed_texture_format_available(GL_COMPRESSED_RGB_S3TC_DXT1_EXT) &&
		(GL_EXT(glCompressedTexImage2DARB) != NULL))
	{
		// upload the pre-compressed texture data directly
		enum { dxt1_block_size= 8 };
		int size= ((width+3)>>2)*((height+3)>>2)*dxt1_block_size;

		UUmAssert(texture_info->motoko_texel_type == IMcPixelType_DXT1);
		UUmAssert(texture_map->texelType == IMcPixelType_DXT1);

		GL_EXT(glCompressedTexImage2DARB)(GL_TEXTURE_2D, level, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
			width, height, 0/*border*/, size, src);
		UUmAssert(gl_GetError() == GL_NO_ERROR);
	}
	else
	{
		// convert to a generic format & then upload
		gl_texture_map_download_generic(texture_map, texture_info, src, level, width, height, buffer);
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);
	
	return;
}

#define REPORT_FORMAT(GLFMT) \
	if (gl_is_compressed_texture_format_available(GLFMT)) UUrStartupMessage(#GLFMT);
#define COMPRESS_FORMAT(ONIFMT, GLFMT) \
	if (gl_is_compressed_texture_format_available(GLFMT)) \
		gl_compress_texture_format(ONIFMT, GLFMT);
// this function will automatically cause texture compression to be used if support for it is found
static boolean gl_enumerate_compressed_texture_formats(
	GLint format_array[],
	int num_array_elements,
	GLint *num_compressed_texture_formats)
{
	boolean success= FALSE;
	boolean compression_algorithm_selected= FALSE;

	GL_FXN(glGetIntegerv)(GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, num_compressed_texture_formats);
	
	if ((*num_compressed_texture_formats > 0) &&
		(*num_compressed_texture_formats < num_array_elements))
	{
		GL_FXN(glGetIntegerv)(GL_COMPRESSED_TEXTURE_FORMATS_ARB, format_array);

		if (gl_query_extension("GL_ARB_texture_compression"))
		{
			REPORT_FORMAT(GL_COMPRESSED_ALPHA_ARB)
			REPORT_FORMAT(GL_COMPRESSED_LUMINANCE_ARB)
			REPORT_FORMAT(GL_COMPRESSED_LUMINANCE_ALPHA_ARB)
			REPORT_FORMAT(GL_COMPRESSED_INTENSITY_ARB)
			REPORT_FORMAT(GL_COMPRESSED_RGB_ARB)
			REPORT_FORMAT(GL_COMPRESSED_RGBA_ARB)
		}
		if (gl_query_extension("GL_EXT_texture_compression_s3tc"))
		{
			REPORT_FORMAT(GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
			REPORT_FORMAT(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
			REPORT_FORMAT(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
			REPORT_FORMAT(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
		#ifdef GL_USE_S3TC
			COMPRESS_FORMAT(IMcPixelType_ARGB4444, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
			COMPRESS_FORMAT(IMcPixelType_RGB555, GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
			COMPRESS_FORMAT(IMcPixelType_ARGB1555, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
			COMPRESS_FORMAT(IMcPixelType_ARGB8888, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
			COMPRESS_FORMAT(IMcPixelType_RGB888, GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
			COMPRESS_FORMAT(IMcPixelType_RGB_Bytes, GL_COMPRESSED_RGB_S3TC_DXT1_EXT) // never used?
			COMPRESS_FORMAT(IMcPixelType_RGBA_Bytes, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) // never used?
			COMPRESS_FORMAT(IMcPixelType_DXT1, GL_COMPRESSED_RGB_S3TC_DXT1_EXT) // really a GL_RGB5_A1
		#endif
			compression_algorithm_selected= TRUE;
		}
		if (gl_query_extension("GL_3DFX_texture_compression_FXT1"))
		{
			REPORT_FORMAT(GL_COMPRESSED_RGB_FXT1_3DFX)
			REPORT_FORMAT(GL_COMPRESSED_RGBA_FXT1_3DFX)
		}

		success= TRUE;
	}
	else // 3dfx's drivers don't report that they support texture compression even when they really do
	{
		// so let's see if this is the case...
		*num_compressed_texture_formats= 0;

		if (gl_query_extension("GL_3DFX_texture_compression_FXT1"))
		{
			UUrStartupMessage("FXT1 found even though the driver said GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB = 0");
			format_array[(*num_compressed_texture_formats)++]= GL_COMPRESSED_RGB_FXT1_3DFX;
			format_array[(*num_compressed_texture_formats)++]= GL_COMPRESSED_RGBA_FXT1_3DFX;
			if (!compression_algorithm_selected)
			{
		/* disabled this code; it works, but looks mmm... like a colostomy bag exploded.
			// ARGB4444 textures go to hell under FXT1
			//	COMPRESS_FORMAT(IMcPixelType_ARGB4444, GL_COMPRESSED_RGBA_FXT1_3DFX)
				COMPRESS_FORMAT(IMcPixelType_RGB555, GL_COMPRESSED_RGB_FXT1_3DFX)
				COMPRESS_FORMAT(IMcPixelType_ARGB1555, GL_COMPRESSED_RGBA_FXT1_3DFX)
			// I'm assuming the same would happen for ARGB8888
			//	COMPRESS_FORMAT(IMcPixelType_ARGB8888, GL_COMPRESSED_RGBA_FXT1_3DFX)
				COMPRESS_FORMAT(IMcPixelType_RGB888, GL_COMPRESSED_RGB_FXT1_3DFX)
				COMPRESS_FORMAT(IMcPixelType_RGB_Bytes, GL_COMPRESSED_RGB_FXT1_3DFX) // never used?
				COMPRESS_FORMAT(IMcPixelType_RGBA_Bytes, GL_COMPRESSED_RGBA_FXT1_3DFX) // never used?
				COMPRESS_FORMAT(IMcPixelType_DXT1, GL_COMPRESSED_RGB_FXT1_3DFX) // really a GL_RGB5_A1
				
				compression_algorithm_selected= TRUE;
		*/
			}
		}
	}

	if (gl_GetError() != GL_NO_ERROR)
	{
		*num_compressed_texture_formats= 0;
	}
	
	return success;
}

static void gl_compress_texture_format(
	IMtPixelType oni_pixel_type,
	GLenum gl_internal_format)
{
	int i, n= sizeof(gl_texel_type_info_table)/sizeof(struct gl_texel_type_info);

	for (i=0; i<n; i++)
	{
		if (gl_texel_type_info_table[i].motoko_texel_type == oni_pixel_type)
		{
			// modify gl_texel_type_info_table[] so that it will do compression on the fly when
			// any texture matching this format is loaded
			gl_texel_type_info_table[i].gl_internal_format= gl_internal_format;
			break;
		}
	}

	return;
}

static boolean gl_is_format_compressed(
	GLenum format)
{
	return (
		// GL_ARB_texture_compression
		(format == GL_COMPRESSED_ALPHA_ARB) ||
		(format == GL_COMPRESSED_LUMINANCE_ARB) ||
		(format == GL_COMPRESSED_LUMINANCE_ALPHA_ARB) ||
		(format == GL_COMPRESSED_INTENSITY_ARB) ||
		(format == GL_COMPRESSED_RGB_ARB) ||
		(format == GL_COMPRESSED_RGBA_ARB) ||
		// GL_EXT_texture_compression_s3tc
		(format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) ||
		(format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ||
		(format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT) ||
		(format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) ||
		// GL_3DFX_texture_compression_FXT1
		(format == GL_COMPRESSED_RGB_FXT1_3DFX) ||
		(format == GL_COMPRESSED_RGBA_FXT1_3DFX));
}

static boolean gl_is_compressed_texture_format_available(
	GLint format)
{
	boolean available= FALSE;
	int i;

	for (i=0; i<gl->number_of_compressed_texture_formats; i++)
	{
		if (gl->compressed_texture_formats[i] == format)
		{
			available= TRUE;
			break;
		}
	}

	return available;
}

static int gl_texture_bytes(
	IMtPixelType oni_format,
	int width,
	int height,
	boolean include_mipmaps)
{
	int size, bpp;

	switch (oni_format)
	{
		case IMcPixelType_ARGB4444: bpp= 2; break;
		case IMcPixelType_RGB555: bpp= 2; break;
		case IMcPixelType_ARGB1555: bpp= 2; break;
		case IMcPixelType_I8: bpp= 1; break;
		case IMcPixelType_I1: bpp= 1; break;
		case IMcPixelType_A8: bpp= 1; break;
		case IMcPixelType_A4I4: bpp= 1; break;
		case IMcPixelType_ARGB8888: bpp= 4; break;
		case IMcPixelType_RGB888: bpp= 3; break;
		case IMcPixelType_DXT1: bpp= 2; break;
		case IMcPixelType_RGB_Bytes: bpp= 3; break;
		case IMcPixelType_RGBA_Bytes: bpp= 4; break;
		case IMcPixelType_RGBA5551: bpp= 2; break;
		case IMcPixelType_RGBA4444: bpp= 2; break;
		case IMcPixelType_RGB565: bpp= 2; break;
		default: bpp= 0;
	}

	size= bpp * width * height;
	
	if (include_mipmaps)
	{
		do
		{
			width>>= 1;
			height>>= 1;
			size+= (bpp * width * height);
		} while (width && height);
	}

	return size;
}

static void gl_report_compression(
	const M3tTextureMap *texture_map,
	const struct gl_texel_type_info *texture_info,
	int level,
	int width,
	int height)
{
	if (gl->number_of_compressed_texture_formats > 0)
	{
		if (gl_is_format_compressed(texture_info->gl_internal_format))
		{
			GLint iscompressed, compressed_size;
			
			GL_FXN(glGetTexLevelParameteriv)(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_ARB, &iscompressed);
			if (iscompressed)
			{
				GL_FXN(glGetTexLevelParameteriv)(GL_TEXTURE_2D, level, GL_TEXTURE_IMAGE_SIZE_ARB, &compressed_size);
				UUrDebuggerMessage("compressed texture '%s', %s ==> %s, %d ==> %d bytes",
					texture_map->debugName,
					gl_oni_texture_format_to_string(texture_info->motoko_texel_type),
					gl_texture_format_to_string(texture_info->gl_internal_format),
					gl_texture_bytes(texture_info->motoko_texel_type, width, height, FALSE), compressed_size);
			}
			else
			{
				UUrDebuggerMessage("tried to compress texture '%s', but it didn't get compressed", texture_map->debugName);
			}
		}
		else
		{
			UUrDebuggerMessage("texture '%s' (%s) not compressed; I don't compress this format", texture_map->debugName,
				gl_oni_texture_format_to_string(texture_info->motoko_texel_type));
		}
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

static char *gl_oni_texture_format_to_string(
	IMtPixelType oni_format)
{
	static char *string;

	switch (oni_format)
	{
		case IMcPixelType_ARGB4444: string= "IMcPixelType_ARGB4444"; break;
		case IMcPixelType_RGB555: string= "IMcPixelType_RGB555"; break;
		case IMcPixelType_ARGB1555: string= "IMcPixelType_ARGB1555"; break;
		case IMcPixelType_I8: string= "IMcPixelType_I8"; break;
		case IMcPixelType_I1: string= "IMcPixelType_I1"; break;
		case IMcPixelType_A8: string= "IMcPixelType_A8"; break;
		case IMcPixelType_A4I4: string= "IMcPixelType_A4I4"; break;
		case IMcPixelType_ARGB8888: string= "IMcPixelType_ARGB8888"; break;
		case IMcPixelType_RGB888: string= "IMcPixelType_RGB888"; break;
		case IMcPixelType_DXT1: string= "IMcPixelType_DXT1"; break;
		case IMcPixelType_RGB_Bytes: string= "IMcPixelType_RGB_Bytes"; break;
		case IMcPixelType_RGBA_Bytes: string= "IMcPixelType_RGBA_Bytes"; break;
		case IMcPixelType_RGBA5551: string= "IMcPixelType_RGBA5551"; break;
		case IMcPixelType_RGBA4444: string= "IMcPixelType_RGBA4444"; break;
		case IMcPixelType_RGB565: string= "IMcPixelType_RGB565"; break;
		default: string= "unknown format";
	}

	return string;
}

static char *gl_texture_format_to_string(
	GLenum format)
{
	static char *string;

	switch (format)
	{
		// uncompressed
		case 1: string= "GL 1"; break;
		case 2: string= "GL 2"; break;
		case 3: string= "GL 3"; break;
		case 4: string= "GL 4"; break;
		case GL_ALPHA4: string= "GL_ALPHA4"; break;
		case GL_ALPHA8: string= "GL_ALPHA8"; break;
		case GL_ALPHA12: string= "GL_ALPHA12"; break;
		case GL_ALPHA16: string= "GL_ALPHA16"; break;
		case GL_LUMINANCE4: string= "GL_LUMINANCE4"; break;
		case GL_LUMINANCE8: string= "GL_LUMINANCE8"; break;
		case GL_LUMINANCE12: string= "GL_LUMINANCE12"; break;
		case GL_LUMINANCE16: string= "GL_LUMINANCE16"; break;
		case GL_LUMINANCE4_ALPHA4: string= "GL_LUMINANCE4_ALPHA4"; break;
		case GL_LUMINANCE6_ALPHA2: string= "GL_LUMINANCE6_ALPHA2"; break;
		case GL_LUMINANCE8_ALPHA8: string= "GL_LUMINANCE8_ALPHA8"; break;
		case GL_LUMINANCE12_ALPHA4: string= "GL_LUMINANCE12_ALPHA4"; break;
		case GL_LUMINANCE12_ALPHA12: string= "GL_LUMINANCE12_ALPHA12"; break;
		case GL_LUMINANCE16_ALPHA16: string= "GL_LUMINANCE16_ALPHA16"; break;
		case GL_INTENSITY: string= "GL_INTENSITY"; break;
		case GL_INTENSITY4: string= "GL_INTENSITY4"; break;
		case GL_INTENSITY8: string= "GL_INTENSITY8"; break;
		case GL_INTENSITY12: string= "GL_INTENSITY12"; break;
		case GL_INTENSITY16: string= "GL_INTENSITY16"; break;
		case GL_R3_G3_B2: string= "GL_R3_G3_B2"; break;
		case GL_RGB4: string= "GL_RGB4"; break;
		case GL_RGB5: string= "GL_RGB5"; break;
		case GL_RGB8: string= "GL_RGB8"; break;
		case GL_RGB10: string= "GL_RGB10"; break;
		case GL_RGB12: string= "GL_RGB12"; break;
		case GL_RGB16: string= "GL_RGB16"; break;
		case GL_RGBA2: string= "GL_RGBA2"; break;
		case GL_RGBA4: string= "GL_RGBA4"; break;
		case GL_RGB5_A1: string= "GL_RGB5_A1"; break;
		case GL_RGBA8: string= "GL_RGBA8"; break;
		case GL_RGB10_A2: string= "GL_RGB10_A2"; break;
		case GL_RGBA12: string= "GL_RGBA12"; break;
		case GL_RGBA16: string= "GL_RGBA16"; break;
		// GL_ARB_texture_compression
		case GL_COMPRESSED_ALPHA_ARB: string= "GL_COMPRESSED_ALPHA_ARB"; break;
		case GL_COMPRESSED_LUMINANCE_ARB: string= "GL_COMPRESSED_LUMINANCE_ARB"; break;
		case GL_COMPRESSED_LUMINANCE_ALPHA_ARB: string= "GL_COMPRESSED_LUMINANCE_ALPHA_ARB"; break;
		case GL_COMPRESSED_INTENSITY_ARB: string= "GL_COMPRESSED_INTENSITY_ARB"; break;
		case GL_COMPRESSED_RGB_ARB: string= "GL_COMPRESSED_RGB_ARB"; break;
		case GL_COMPRESSED_RGBA_ARB: string= "GL_COMPRESSED_RGBA_ARB"; break;
		// GL_EXT_texture_compression_s3tc
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: string= "GL_COMPRESSED_RGB_S3TC_DXT1_EXT"; break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: string= "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT"; break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: string= "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT"; break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: string= "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT"; break;
		// GL_3DFX_texture_compression_FXT1
		case GL_COMPRESSED_RGB_FXT1_3DFX: string= "GL_COMPRESSED_RGB_FXT1_3DFX"; break;
		case GL_COMPRESSED_RGBA_FXT1_3DFX: string= "GL_COMPRESSED_RGBA_FXT1_3DFX"; break;
		//
		default: string= "unknown format";
	}

	return string;
}

UUtBool gl_texture_map_delete(
	M3tTextureMap *texture_map)
{
	UUtBool success= TRUE;

	UUmAssert(texture_map);

	if (texture_map->flags&M3cTextureFlags_Offscreen)
	{
		// don't delete
		texture_map->opengl_dirty= TRUE;
	}
	else if (texture_map->opengl_texture_name != 0)
	{
		success= gl_texture_list_remove(texture_map);
		GL_FXN(glDeleteTextures)(1, (unsigned int*)&texture_map->opengl_texture_name);
		texture_map->opengl_texture_name= 0;
		texture_map->opengl_dirty= TRUE;

		UUmAssert(success && (gl_GetError() == GL_NO_ERROR));
	}

	return success;
}

static boolean gl_texture_map_reload(
	M3tTextureMap *texture_map)
{
	boolean success;

	UUmAssert(texture_map);
	if (texture_map->flags&M3cTextureFlags_Offscreen)
	{
		success= TRUE;
		texture_map->opengl_dirty= FALSE; // S.S. 11/09/2000
	}
	else
	{
		success= gl_texture_map_delete(texture_map);
		UUmAssert(success);
		if (success)
		{
			success= gl_texture_map_create(texture_map);
			UUmAssert(success);
		}
	}

	return success;
}

UUtError gl_texture_map_proc_handler(
	TMtTemplateProc_Message message,
	void *instance_ptr,
	void *private_data)
{
	M3tTextureMap *texture_map= instance_ptr;
	
	switch (message)
	{
		case TMcTemplateProcMessage_NewPostProcess:
			texture_map->flags|= M3cTextureFlags_Offscreen;
			texture_map->debugName[0]= '\0';
			texture_map->opengl_texture_name= 0;
			texture_map->opengl_dirty= TRUE;
			break;
		case TMcTemplateProcMessage_LoadPostProcess:
			M3rTextureMap_Prepare(texture_map);			
			texture_map->opengl_texture_name= 0;
			texture_map->opengl_dirty= TRUE;
			break;
		case TMcTemplateProcMessage_DisposePreProcess:
			gl_texture_map_delete(texture_map);
			break;
		case TMcTemplateProcMessage_Update:
			texture_map->opengl_dirty= TRUE;
			break;
		case TMcTemplateProcMessage_PrepareForUse:
			break;
		default:
			break;
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return UUcError_None;
}

void gl_set_textures(
	M3tTextureMap *texture0,
	M3tTextureMap *texture1,
	GLenum sfactor, // sfactor, dfactor valid only if texture0 && !texture1
	GLenum dfactor) // otherwise sfactor & dfactor must == NONE
{
	static M3tTextureMap *current_texture0= NULL;
	static M3tTextureMap *current_texture1= NULL;
	static GLenum current_sfactor= _gl_initial_sfactor;
	static GLenum current_dfactor= _gl_initial_dfactor;
	static boolean fog_was_forced_off= FALSE;
	static boolean constant_alpha_blend= FALSE;
	boolean blend_factors_changed, fog_needs_reactivating, success= TRUE;

	UUmAssert(gl_GetError() == GL_NO_ERROR);
	// if we forcibly disabled fog last time we were here, turn it back on
	if (fog_was_forced_off)
	{
		fog_needs_reactivating= TRUE;
		fog_was_forced_off= FALSE;
	}
	else
	{
		fog_needs_reactivating= FALSE;
	}

	if (texture1)
	{
		UUmAssert(texture0);
		UUmAssert(gl->multitexture);
	}

#ifdef GL_ENABLE_ALPHA_FADE_CODE
	// detect fade-out condition (frame buffer blend w/ constant alpha)
	if (gl->blend_to_frame_buffer_with_constant_alpha && GL_EXT(glBlendColor))
	{
		sfactor= GL_CONSTANT_ALPHA;
		dfactor= GL_ONE_MINUS_CONSTANT_ALPHA; //GL_ONE_MINUS_SRC_ALPHA GL_ONE_MINUS_CONSTANT_ALPHA
		GL_EXT(glBlendColor)(0.f, 0.f, 0.f, gl->constant_color.a * ONE_OVER_255);
		constant_alpha_blend= TRUE;
		UUmAssert(gl_GetError() == GL_NO_ERROR);
	}
	else if (constant_alpha_blend)
	{
		constant_alpha_blend= FALSE;
	}
#endif
	
	// handle blend factors
	if (texture0)
	{
		if (texture0->opengl_dirty)
		{
			success= gl_texture_map_reload(texture0);
		}

		if (success)
		{
			if (sfactor != NONE)
			{
				UUmAssert(dfactor != NONE);
			}
			else 
			{
				UUmAssert(dfactor == NONE);
				if (texture1 == NULL)
				{
					if (texture0->flags&M3cTextureFlags_Blend_Additive)
					{
						sfactor= GL_SRC_ALPHA;
						dfactor= GL_ONE;

						// force fog off!
						if (gl->fog_enabled)
						{
							GL_FXN(glDisable)(GL_FOG);
							fog_was_forced_off= TRUE;
						}
					}
					else // default alpha transparency
					{
						sfactor= GL_SRC_ALPHA;
						dfactor= GL_ONE_MINUS_SRC_ALPHA;
					}
				}
				else
				{
					sfactor= GL_ONE;
					dfactor= GL_ZERO;
				}
			}
		}
	}

	blend_factors_changed= ((sfactor != current_sfactor) || (dfactor != current_dfactor));

	// setup first texture stage
	if ((texture0 != current_texture0) && success)
	{
		// select & activate desired texture unit
		if (gl->multitexture)
		{
			GL_EXT(glActiveTextureARB)(GL_TEXTURE0_ARB);
			GL_EXT(glClientActiveTextureARB)(GL_TEXTURE0_ARB);
		}
		if (texture0 == NULL)
		{
			GL_FXN(glBindTexture)(GL_TEXTURE_2D, 0);
			GL_FXN(glDisable)(GL_TEXTURE_2D);
		}
		else
		{
			GL_FXN(glEnable)(GL_TEXTURE_2D);
			GL_FXN(glBindTexture)(GL_TEXTURE_2D, texture0->opengl_texture_name);
			
			gl_set_texture_timestamp(texture0);
		}
		current_texture0= texture0;
	}
	// setup second texture stage
	if ((texture1 != current_texture1) && success)
	{
		// select & activate desired texture unit
		GL_EXT(glActiveTextureARB)(GL_TEXTURE1_ARB);
		GL_EXT(glClientActiveTextureARB)(GL_TEXTURE1_ARB);
		
		if (texture1 == NULL)
		{
			GL_FXN(glBindTexture)(GL_TEXTURE_2D, 0);
			GL_FXN(glDisable)(GL_TEXTURE_2D);

		#ifdef ENABLE_NVIDIA_MULTITEXTURING
			// if we were using the NV regiser combiners previously, turn them off now
			if ((gl->multitexture & _gl_multitexture_nv) && gl->nv_register_combiners_enabled)
			{
				GL_FXN(glDisable)(GL_REGISTER_COMBINERS_NV);
				gl->nv_register_combiners_enabled= FALSE;
			}
		#endif
		}
		else
		{
			gl_setup_multitexturing();

			if (texture1->opengl_dirty)
			{
				success= gl_texture_map_reload(texture1);
			}
			if (success)
			{
				GL_FXN(glEnable)(GL_TEXTURE_2D);
				GL_FXN(glBindTexture)(GL_TEXTURE_2D, texture1->opengl_texture_name);
			}
			
			gl_set_texture_timestamp(texture1);
		}
		current_texture1= texture1;

		// make GL_TEXTURE0_ARB the active texture unit again
		GL_EXT(glActiveTextureARB)(GL_TEXTURE0_ARB);
		GL_EXT(glClientActiveTextureARB)(GL_TEXTURE0_ARB);
	}

	if (blend_factors_changed && (sfactor != NONE) && (dfactor != NONE))
	{
		GL_FXN(glBlendFunc)(sfactor, dfactor);
		current_sfactor= sfactor;
		current_dfactor= dfactor;

	}

	if (fog_needs_reactivating && !fog_was_forced_off && gl->fog_enabled && !gl->fog_disabled_3Dfx)
	{
		GL_FXN(glEnable)(GL_FOG);
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);
	
	return;
}

void gl_sync_to_vtrace(
	boolean enable)
{

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
	if (enable && !gl->gl_vsync_enabled)
	{
		if (!GL_EXT(wglSwapIntervalEXT))
		{
			GL_EXT(wglSwapIntervalEXT)= load_gl_extension_routine("wglSwapIntervalEXT");
		}

		if (GL_EXT(wglSwapIntervalEXT))
		{
			gl->gl_vsync_enabled= GL_EXT(wglSwapIntervalEXT)(1);
			UUrStartupMessage("wglSwapIntervalEXT supported; vsync= %d", gl->gl_vsync_enabled);
		}
	}
#endif
	
	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

static void gl_setup_multitexturing(
	void)
{
	UUmAssert(gl->geom_draw_mode == _geom_draw_mode_environment_map);

	if (gl->multitexture == _gl_multitexture_nv)
	{
		gl_setup_nv_combiners_for_env_maps();
	}
	else if (gl->multitexture == _gl_multitexture_texture_env_combine)
	{
		gl_setup_texture_env_combine_for_env_maps();
	}
	else if (gl->multitexture == _gl_multitexture_arb)
	{
		GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	}
	else
	{
		UUmAssert(0);
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

static void gl_setup_nv_combiners_for_env_maps(
	void)
{
	UUmAssert(gl->multitexture == _gl_multitexture_nv);

	GL_EXT(glCombinerParameteriNV)(GL_NUM_GENERAL_COMBINERS_NV, 1);
	// glCombinerInputNV(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage)
	// A= reflection map
	GL_EXT(glCombinerInputNV)(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_A_NV, GL_TEXTURE0_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB); 
	// B= base texture alpha
	GL_EXT(glCombinerInputNV)(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_B_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA); 
	// C= base texture rgb
	GL_EXT(glCombinerInputNV)(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_C_NV, GL_TEXTURE1_ARB, GL_UNSIGNED_IDENTITY_NV, GL_RGB); 
	// D= 1 (0 inverted == 1)
	GL_EXT(glCombinerInputNV)(GL_COMBINER0_NV, GL_RGB, GL_VARIABLE_D_NV, GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_RGB); 
	// output: spare0= (reflection map)*(base texture alpha) + (base texture rgb)
	//glCombinerOutputNV(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum)
	//                     stage            portion  abOutput       cdOutput       sumOutput     scale     bias    abDot     cdDot     muxSum
	GL_EXT(glCombinerOutputNV)(GL_COMBINER0_NV, GL_RGB, GL_DISCARD_NV, GL_DISCARD_NV, GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE);

	GL_EXT(glFinalCombinerInputNV)(GL_VARIABLE_A_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	GL_EXT(glFinalCombinerInputNV)(GL_VARIABLE_B_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	GL_EXT(glFinalCombinerInputNV)(GL_VARIABLE_C_NV, GL_ZERO, GL_UNSIGNED_IDENTITY_NV, GL_RGB);
	// write the results as the final texturing output
	GL_EXT(glFinalCombinerInputNV)(GL_VARIABLE_D_NV, GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_RGB);

	if (!gl->nv_register_combiners_enabled)
	{
		GL_FXN(glEnable)(GL_REGISTER_COMBINERS_NV);
		gl->nv_register_combiners_enabled= TRUE;
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);

	return;
}

static void gl_setup_texture_env_combine_for_env_maps(
	void)
{
#ifdef ENABLE_GL_EXT_texture_env_combine
	static boolean first_time= TRUE;

	// we want: result= (reflection map)*(base texture alpha) + (base texture rgb)
	// setup tmu 0
	GL_EXT(glActiveTextureARB)(GL_TEXTURE0_ARB);
	GL_EXT(glClientActiveTextureARB)(GL_TEXTURE0_ARB);
	GL_FXN(glTexEnvf)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);

	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_MODULATE);

	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE); // reflection map rgb; must be GL_TEXTURE
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT); // prev = null
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_CONSTANT_EXT); // junk

	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_OPERAND2_RGB_EXT, GL_SRC_ALPHA);

	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE); // reflection map alpha (none); must be GL_TEXTURE
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT);

	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_EXT, GL_SRC_ALPHA);
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_SRC_ALPHA);

	if (first_time)
	{
		GL_FXN(glTexEnvf)(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 1.0f);
		GL_FXN(glTexEnvf)(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1.0f);
	}

	// setup tmu 1
	GL_EXT(glActiveTextureARB)(GL_TEXTURE1_ARB);
	GL_EXT(glClientActiveTextureARB)(GL_TEXTURE1_ARB);
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);

	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_ADD); // (env map rgb) + (base map rgb)
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE); // reflection map has no alpha

	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE); // base map rgb; must be GL_TEXTURE
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT); // env map rgb
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_CONSTANT_EXT); // junk

	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_OPERAND2_RGB_EXT, GL_SRC_ALPHA);

	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE); // must be GL_TEXTURE
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT);

	// here is where we are screwed.
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_EXT, GL_ONE_MINUS_SRC_ALPHA); // need this to be 1!!! It's cryin' time ...
	GL_FXN(glTexEnvi)(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_EXT, GL_SRC_ALPHA);

	if (first_time)
	{
		GL_FXN(glTexEnvf)(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 1.0f);
		GL_FXN(glTexEnvf)(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1.0f);
	}

	UUmAssert(gl_GetError() == GL_NO_ERROR);
	first_time= FALSE;

#endif

	return;
}

/* OpenGL wrappers */

#define GL_ERROR_TO_STRING(err)	case err: gl->error_string= #err; break;
GLenum gl_GetError(
	void)
{
	gl->error_code= GL_FXN(glGetError)();
	switch (gl->error_code)
	{
		GL_ERROR_TO_STRING(GL_NO_ERROR)
		GL_ERROR_TO_STRING(GL_INVALID_ENUM)
		GL_ERROR_TO_STRING(GL_INVALID_VALUE)
		GL_ERROR_TO_STRING(GL_INVALID_OPERATION)
		GL_ERROR_TO_STRING(GL_STACK_OVERFLOW)
		GL_ERROR_TO_STRING(GL_STACK_UNDERFLOW)
		GL_ERROR_TO_STRING(GL_OUT_OF_MEMORY)
		default: gl->error_string= "<unknown error>"; break;
	}

	return gl->error_code;
}

static boolean gl_load_library(
	void)
{
	boolean success= TRUE;

#ifdef GL_LOAD_API_DYNAMICALLY
	memset(gl_api, 0, sizeof(struct gl_api));
	#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
	success= gl_load_opengl_dll();
	#endif
		
	/*
		this marks the end of standard API functions that are required to run
		optional extensions are loaded as needed
	*/
	gl_api->end_standard_api= (void *)NONE;

	if (success)
	{
		success= gl_library_is_loaded();
	}
#endif // GL_LOAD_API_DYNAMICALLY

	return success;
}

static boolean gl_library_is_loaded(
	void)
{
	boolean loaded= TRUE;
#ifdef GL_LOAD_API_DYNAMICALLY
	int i, n;

	n= sizeof(struct gl_api)/sizeof(void *);
	for (i=0; i<n; i++)
	{
		void **api_ptr= (void **)gl_api;

		if (api_ptr[i] == (void *)NONE) // gl_api->end_standard_api
		{
			break;
		}
		if (api_ptr[i] == NULL)
		{
			loaded= FALSE;
			break;
		}
	}
#endif
	return loaded;
}

float gl_calculate_fog_factor(
	M3tPoint3D *point)
{
	float fog_factor;

#if SHIPPING_VERSION == 0
	static boolean gl_fade_particles_by_fog= TRUE;
	static boolean first_time= TRUE;

	if (first_time)
	{
		SLrGlobalVariable_Register_Bool("gl_fade_particles_by_fog", "gl_fade_particles_by_fog on", &gl_fade_particles_by_fog);
		first_time= FALSE;
	}
	if (gl_fade_particles_by_fog == FALSE) return 0.f;
#endif

	// assumes GL_FOG_MODE == GL_LINEAR
	// given a depth value 'z', calculate what the fog factor would be given the current GL state
	if ((gl->fog_start == gl->fog_end) ||
		(gl->fog_start >= 1.f) ||
		(gl->fog_end <= 0.f))
	{
		fog_factor= 0.f;
	}
	else
	{
		// z must be in screen space coordinates
		// oh the pain
		extern int MSrTransform_PointListToFrustumScreen( // MS_Geom_Transform.c (returns an enumerated type)
			UUtUns32			inNumVertices,
			const M3tPoint3D*	inPointList,
			M3tPoint4D			*outFrustumPoints,
			M3tPointScreen		*outScreenPoints,
			UUtUns8				*outClipCodeList);
		// transformation buffers
		static UUtUns8 screen_buf[sizeof(M3tPointScreen) + (2 * UUcProcessor_CacheLineSize)];
		static UUtUns8 frustum_buf[sizeof(M3tPoint4D) + (2 * UUcProcessor_CacheLineSize)];
		static UUtUns8 world_buf[sizeof(M3tPoint3D) + (2 * UUcProcessor_CacheLineSize)];
		static M3tPointScreen *screen_point= NULL;
		static M3tPoint4D *frustum_point= NULL;
		static M3tPoint3D *world_point= NULL;
		UUtUns8 clip_code;

		if (screen_point == NULL)
		{
			// set up the memory-aligned pointers
			screen_point= UUrAlignMemory(screen_buf);
			frustum_point= UUrAlignMemory(frustum_buf);
			world_point= UUrAlignMemory(world_buf);
		}
		*world_point= *point;
	
		MSrTransform_PointListToFrustumScreen(1, world_point, frustum_point, screen_point, &clip_code);
		if (screen_point->z <= gl->fog_start)
		{
			fog_factor= 0.f;
		}
		else if (screen_point->z >= gl->fog_end)
		{
			fog_factor= 1.f;
		}
		else
		{
			fog_factor= (gl->fog_end - screen_point->z)/(gl->fog_end - gl->fog_start);
		}
	}
	UUmAssert((fog_factor >= 0.f) && (fog_factor <= 1.f));

	return fog_factor;
}

UUtBool gl_voodoo_card_full_screen(
	void)
{
	UUtBool voodoo_fullscreen= UUcFalse;
	
	if ((gl != NULL) && (gl->renderer != NULL) && (strstr(gl->renderer, "3Dfx") || strstr(gl->renderer, "3dfx")) && M3gResolutionSwitch)
	{
		voodoo_fullscreen= UUcTrue;
	}

	return voodoo_fullscreen;
}

UUtBool gl_s3_crappy_card_full_screen(
	void)
{
	UUtBool s3_card_fullscreen= UUcFalse;

	if ((gl != NULL) && strstr(gl->vendor, "S3") && M3gResolutionSwitch)
	{
		s3_card_fullscreen= UUcTrue;
	}

	return s3_card_fullscreen;
}

#endif // __ONADA__

