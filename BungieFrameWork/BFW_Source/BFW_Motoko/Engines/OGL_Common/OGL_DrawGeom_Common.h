/*
	FILE:	OGL_DrawGeom_Common.h

	AUTHOR:	Brent Pease, Kevin Armstrong, Michael Evans

	CREATED: January 5, 1998

	PURPOSE:

	Copyright 1997 - 1998

*/

// extensions
#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
#define GL_CALL __stdcall
#else
#define GL_CALL
#endif

typedef void (GL_CALL *GLtLockArraysEXT)(int first, GLsizei count);
typedef void (GL_CALL *GLtUnlockArraysEXT)(void);
typedef void (GL_CALL *GLtActiveTextureARB)(GLenum target);
typedef void (GL_CALL *GLtClientActiveTextureARB)(GLenum target);
typedef void (GL_CALL *GLtMultiTexCoord4fARB)(GLenum target,GLfloat s,GLfloat t,GLfloat q,GLfloat r);
typedef void (GL_CALL *GLtMultiTexCoord2fvARB)(GLenum target,GLfloat *uv);

typedef enum OGLtCameraMode
{
	OGLcCameraMode_2D,
	OGLcCameraMode_3D

} OGLtCameraMode;

// Extra GL header stuff
#define	UNSIGNED_BYTE_3_3_2			0x8032
#define	UNSIGNED_BYTE_2_3_3_REV		0x8362
#define	UNSIGNED_SHORT_5_6_5		0x8363
#define	UNSIGNED_SHORT_5_6_5_REV	0x8364
#define	UNSIGNED_SHORT_4_4_4_4		0x8033
#define	UNSIGNED_SHORT_4_4_4_4_REV	0x8365
#define	UNSIGNED_SHORT_5_5_5_1		0x8034
#define	UNSIGNED_SHORT_1_5_5_5_REV	0x8366
#define	UNSIGNED_INT_8_8_8_8		0x8035
#define	UNSIGNED_INT_8_8_8_8_REV	0x8367
#define	UNSIGNED_INT_10_10_10_2		0x8036
#define	UNSIGNED_INT_2_10_10_10_REV	0x8368

/* EXT_vertex_array */
#define GL_VERTEX_ARRAY_EXT                 0x8074
#define GL_NORMAL_ARRAY_EXT                 0x8075
#define GL_COLOR_ARRAY_EXT                  0x8076
#define GL_INDEX_ARRAY_EXT                  0x8077
#define GL_TEXTURE_COORD_ARRAY_EXT          0x8078
#define GL_EDGE_FLAG_ARRAY_EXT              0x8079
#define GL_VERTEX_ARRAY_SIZE_EXT            0x807A
#define GL_VERTEX_ARRAY_TYPE_EXT            0x807B
#define GL_VERTEX_ARRAY_STRIDE_EXT          0x807C
#define GL_VERTEX_ARRAY_COUNT_EXT           0x807D
#define GL_NORMAL_ARRAY_TYPE_EXT            0x807E
#define GL_NORMAL_ARRAY_STRIDE_EXT          0x807F
#define GL_NORMAL_ARRAY_COUNT_EXT           0x8080
#define GL_COLOR_ARRAY_SIZE_EXT             0x8081
#define GL_COLOR_ARRAY_TYPE_EXT             0x8082
#define GL_COLOR_ARRAY_STRIDE_EXT           0x8083
#define GL_COLOR_ARRAY_COUNT_EXT            0x8084
#define GL_INDEX_ARRAY_TYPE_EXT             0x8085
#define GL_INDEX_ARRAY_STRIDE_EXT           0x8086
#define GL_INDEX_ARRAY_COUNT_EXT            0x8087
#define GL_TEXTURE_COORD_ARRAY_SIZE_EXT     0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE_EXT     0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE_EXT   0x808A
#define GL_TEXTURE_COORD_ARRAY_COUNT_EXT    0x808B
#define GL_EDGE_FLAG_ARRAY_STRIDE_EXT       0x808C
#define GL_EDGE_FLAG_ARRAY_COUNT_EXT        0x808D
#define GL_VERTEX_ARRAY_POINTER_EXT         0x808E
#define GL_NORMAL_ARRAY_POINTER_EXT         0x808F
#define GL_COLOR_ARRAY_POINTER_EXT          0x8090
#define GL_INDEX_ARRAY_POINTER_EXT          0x8091
#define GL_TEXTURE_COORD_ARRAY_POINTER_EXT  0x8092
#define GL_EDGE_FLAG_ARRAY_POINTER_EXT      0x8093

/* ARB_multitexture */
#define GL_ACTIVE_TEXTURE_ARB               0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB        0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB            0x84E2
#define GL_TEXTURE0_ARB                     0x84C0
#define GL_TEXTURE1_ARB                     0x84C1
#define GL_TEXTURE2_ARB                     0x84C2
#define GL_TEXTURE3_ARB                     0x84C3
#define GL_TEXTURE4_ARB                     0x84C4
#define GL_TEXTURE5_ARB                     0x84C5
#define GL_TEXTURE6_ARB                     0x84C6
#define GL_TEXTURE7_ARB                     0x84C7
#define GL_TEXTURE8_ARB                     0x84C8
#define GL_TEXTURE9_ARB                     0x84C9
#define GL_TEXTURE10_ARB                    0x84CA
#define GL_TEXTURE11_ARB                    0x84CB
#define GL_TEXTURE12_ARB                    0x84CC
#define GL_TEXTURE13_ARB                    0x84CD
#define GL_TEXTURE14_ARB                    0x84CE
#define GL_TEXTURE15_ARB                    0x84CF
#define GL_TEXTURE16_ARB                    0x84D0
#define GL_TEXTURE17_ARB                    0x84D1
#define GL_TEXTURE18_ARB                    0x84D2
#define GL_TEXTURE19_ARB                    0x84D3
#define GL_TEXTURE20_ARB                    0x84D4
#define GL_TEXTURE21_ARB                    0x84D5
#define GL_TEXTURE22_ARB                    0x84D6
#define GL_TEXTURE23_ARB                    0x84D7
#define GL_TEXTURE24_ARB                    0x84D8
#define GL_TEXTURE25_ARB                    0x84D9
#define GL_TEXTURE26_ARB                    0x84DA
#define GL_TEXTURE27_ARB                    0x84DB
#define GL_TEXTURE28_ARB                    0x84DC
#define GL_TEXTURE29_ARB                    0x84DD
#define GL_TEXTURE30_ARB                    0x84DE
#define GL_TEXTURE31_ARB                    0x84DF

/*---------- structures */

typedef union gl_color_4ub { // S.S.
	struct _bytes_4ub { // gcc won't like this unamed struct
	#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
		UUtUns8 b, g, r, a;
	#elif defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
		UUtUns8 a, r, g, b;
	#else
		#error "struct not setup for this platform"
	#endif
	};
	UUtUns32 argb;
} gl_color_4ub;

typedef M3tTextureCoord gl_texture_uv;
typedef M3tPointScreen gl_vertex_xyzw;

/*---------- constants */
enum {
	VERTEX_ARRAY_SIZE_OGL= 1024
};

/*---------- globals */

extern gl_color_4ub *gl_vertex_colors;
extern gl_texture_uv *gl_texture_uvs0;
extern gl_texture_uv *gl_texture_uvs1;
extern gl_vertex_xyzw *gl_vertices;


typedef struct OGLtCommon
{
	const char*	vendor;
	const char* renderer;
	const char*	version;
	const char*	extensions;

	// only optional extension are here

	UUtBool		ext_arb_multitexture;
	UUtBool		ext_vertex_array;
	UUtBool		ext_compiled_vertex_array;
	UUtBool		ext_packed_pixels;
	UUtBool		ext_abgr;
	UUtBool		ext_bgra;
	UUtBool		ext_clip_volume_hint;
	UUtBool		ext_cull_vertex;
	UUtBool		ext_point_parameters;
	UUtBool		ext_stencil_wrap;
	UUtBool		ext_texture_object;
	UUtBool		ext_s3tc;

	GLtLockArraysEXT			pglLockArraysEXT;
	GLtUnlockArraysEXT			pglUnlockArraysEXT;
	GLtActiveTextureARB			pglActiveTextureARB;
	GLtClientActiveTextureARB	pglClientActiveTextureARB;
	GLtMultiTexCoord4fARB		pglMultiTexCoord4fARB;
	GLtMultiTexCoord2fvARB		pglMultiTexCoord2fvARB;

	OGLtCameraMode				cameraMode;

	UUtBool						update3DCamera_ViewData;
	UUtBool						update3DCamera_StaticData;

	UUtUns16					width;
	UUtUns16					height;

	TMtPrivateData*				texturePrivateData;
	void*						convertedDataBuffer;

	// fog params
	GLfloat fog_color[4];
	GLfloat fog_density;
	GLfloat fog_start;
	GLfloat fog_end;
	GLint fog_mode;
} OGLtCommon;

// S.S.
#ifndef NONE
#define NONE (-1L)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#if defined(DEBUGGING) && DEBUGGING
#define OGLmAssertSuccess(glError) do { for((glError) = glGetError(); (glError) != GL_NO_ERROR; (glError) = glGetError()) { UUmAssert(GL_NO_ERROR == glError); } } while(0)
#else
#define OGLmAssertSuccess(glError) do { glError; } while(0)
#endif

// This returns true of open gl is available and can be used
UUtError
OGLrCommon_Initialize_OneTime(
	void);

UUtError
OGLrCommon_State_Initialize(
	UUtUns16	inWidth,
	UUtUns16	inHeight);

void
OGLrCommon_State_Terminate(
	void);

void
OGLrCommon_Camera_Update(
	OGLtCameraMode	inCameraMode);

void
OGLrCommon_Matrix4x3_OniToGL(
	M3tMatrix4x3*	inMatrix,
	float			*outGLMatrix);

void
OGLrCommon_Matrix4x4_OniToGL(
	M3tMatrix4x4*	inMatrix,
	float			*outGLMatrix);

void
OGLrCommon_TextureMap_Select(
	M3tTextureMap*	inTextureMap,
	GLenum			inTMU);

UUtBool
OGLrCommon_TextureMap_TextureFormatAvailable(
	IMtPixelType		inTexelType);

void
OGLrCommon_DepthMode_Set(
	UUtBool inRead,
	UUtBool inWrite);

void
OGLrCommon_DepthBias_Set(
	float inBias );

/*---------- S.S. */

/*---------- prototypes */

void OGLrCommon_glFogEnable(void);

/*---------- wrappers */

void OGLrCommon_glFlush(void);
void OGLrCommon_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void OGLrCommon_glBindTexture(GLenum target, GLuint texture);
void OGLrCommon_glBlendFunc(GLenum src_factor, GLenum dst_factor);
void OGLrCommon_glEnable(GLenum cap);
void OGLrCommon_glDisable(GLenum cap);
void OGLrCommon_glEnableClientState(GLenum array);
void OGLrCommon_glDisableClientState(GLenum array);


/*---------- globals */

extern GLint gl_error;
extern gl_color_4ub current_vertex_color_gl;

/*---------- macros */

#define OGLrCommon_glColor4ub(color) { \
	if (color.argb != current_vertex_color_gl.argb) { \
		glColor4ub(color.r, color.g, color.b, color.a); \
		current_vertex_color_gl.argb= color.argb; \
	} }


//
extern OGLtCommon	OGLgCommon;
