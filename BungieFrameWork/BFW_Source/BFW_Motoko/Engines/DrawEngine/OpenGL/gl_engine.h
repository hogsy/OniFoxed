/*
	gl_engine.h
	Thursday Aug. 10, 2000 7:46 pm
	Stefan
*/

#ifndef __GL_ENGINE__
#define __GL_ENGINE__

/*---------- headers */

#include <GL/gl.h>
#if UUmPlatform != UUmPlatform_Linux
#include "glext.h"
#else
#define GL_TEXTURE_IMAGE_SIZE_ARB GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB
#endif

#include "BFW_Motoko.h"
#include "Motoko_Manager.h"

/*---------- constants */

// define this to use the standard GL ARB multitexturing (doesn't work with current artwork)
//#define ENABLE_GL_ARB_MULTITEXTURING // guess what? even if we were to use it, this doesn't work on the Matrox G400.

// define this to enable nVidia register combiner multitexturing
#define ENABLE_NVIDIA_MULTITEXTURING

// define this to enable GL_EXT_texture_env_combine multitexturing (doesn't work with current artwork)
//#define ENABLE_GL_EXT_texture_env_combine

// define this to enable fog
#define ENABLE_GL_FOG

// define this to use S3 texture compression if available
//#define GL_USE_S3TC

// define this to spew out debugging info on texture compression
//#define REPORT_ON_COMPRESSION_GL

// define this to enable alpha fade-out code using glBlendColor()
//#define GL_ENABLE_ALPHA_FADE_CODE // this was actually never working correctly

// define this to disable fog on 3Dfx hardware
#define DISABLE_FOG_ON_3DFX

// define this to scale z-values (to make fog work on 3Dfx)
#ifndef DISABLE_FOG_ON_3DFX
#define Z_SCALE		65535.f
#endif

// define this to load GL API functions dynamically (for MS Windows)
// if not defined on MS Windows, be sure to link to opengl32.lib
#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
#define GL_LOAD_API_DYNAMICALLY
#endif

#undef TRUE
#undef FALSE
#undef NONE

#define FOG_COLOR_GL		0.25f // 0.f <= FOG_COLOR_GL <= 1.f
#define ONI_FOG_START		0.975f
#define ONI_FOG_END			1.f

#define ONE_OVER_255		0.003921568627451f

#define DEFAULT_WIN32_GL_DLL	"opengl32.dll"

enum {
	FALSE= 0,
	TRUE= 1,
	NONE= -1L,

	_error_none= 0,
	_error_no_opengl,
	_error_out_of_memory,
	_error_unknown,
	NUMBER_OF_ERROR_CODES,

	_camera_mode_2d= 0,
	_camera_mode_3d,

	_gl_pixel_download_S3= 0,
	_gl_pixel_download_packed,
	_gl_pixel_download_classic,
	_gl_pixel_download_generic,
	_gl_pixel_download_invalid= NONE,

	_geom_draw_mode_default= 0,
	_geom_draw_mode_wireframe,
	_geom_draw_mode_gourand, // diffuse color only
	_geom_draw_mode_flat, // texture only
	_geom_draw_mode_environment_map, // (env map rgb) * (base map alpha) + (base map rgb)
	_geom_draw_mode_environment_map_low_quality, // base map rgb - use constant alpha
	_geom_draw_mode_split, // diffuse color + base texture
	_geom_draw_mode_environment_map_multipass, // diffuse + texture, src blend= 1, dst blend= 0
	_geom_draw_mode_base_map_multipass, // diffuse + texture, src blend= 1, dst blend= src alpha
	NUMBER_OF_GEOMETRY_DRAW_MODES,

	_vertex_format_diffuse_color_flag= (1<<0),
	_vertex_format_base_texture_flag= (1<<1),
	_vertex_format_second_texture_flag= (1<<2),

	_gl_multitexture_none= 0, // no multitexturing available (multipass used)
	_gl_multitexture_arb, // use standard ARB extenstions for multitexturing
	_gl_multitexture_nv, // use nVidia register combiners for multitexturing
	_gl_multitexture_texture_env_combine, // use GL_EXT_texture_env_combine

	_gl_initial_sfactor= GL_ONE,
	_gl_initial_dfactor= GL_ZERO,

	MAX_COMPRESSED_TEXTURE_FORMATS= 16,
	
	// S.S. 11/09/2000 added this for texture management
	
	MAX_INTERNAL_FRAME_COUNT_VALUE= 0x00FFFFFF, // storing in the 3 pad bytes for a Motoko texture
	
	KILO= 1024,
	MEG= KILO * KILO,
	GIG= KILO * MEG,
	
	// this regulates how many OpenGL textures we will keep loaded
	// gl_macos.c contains code which regulates this based on graphics quality setting
	MAX_DEFAULT_TEXTURE_MEMORY_LIMIT= 1 * GIG, // no limit

	DESIRED_MINIMUM_FREE_TEXTURE_MEMORY= 1 * MEG, // when we exceed MAX_DEFAULT_TEXTURE_MEMORY_LIMIT, purge until we have this much free space
	MINIMUM_TEXTURE_PURGE_LIMIT= 32 // we will never purge to fewer than this many textures

};

/*---------- macros */

// all calls to an OpenGL or WinGDI function MUST be wrapped by these macros!!
#ifdef GL_LOAD_API_DYNAMICALLY
#define GL_FXN(GL_FUNCTION_NAME)	gl_api->GL_FUNCTION_NAME
#define WGL_FXN(WGL_FUNCTION_NAME)	gl_api->WGL_FUNCTION_NAME
#else
#define GL_FXN(GL_FUNCTION_NAME)	GL_FUNCTION_NAME
#define WGL_FXN(WGL_FUNCTION_NAME)	WGL_FUNCTION_NAME
#endif

#define GL_EXT(EXT_FUNCTION_NAME)	gl_api->EXT_FUNCTION_NAME
#define GDI_FXN(GDI_FUNCTION_NAME)	GDI_FUNCTION_NAME // wrapped because it is conceivable that we might want to load these ourselves
#define AGL_FXN(AGL_FUNCTION_NAME)	AGL_FUNCTION_NAME // feel free to ignore this one

// ZCOORD wraps all z values
// note that I invert our z values (and setup the GL view matrix accordingly);
// this is because on certain video cards (ATI) fog will not work if we feed
// in negative z-values, which is how the Oni coordinate system gives them.
// So everyone suffers :)
#ifdef Z_SCALE
	#define ZCOORD(z)	(-(z) * Z_SCALE)
#else
	#define ZCOORD(z)	(-(z))
#endif

/*---------- structures */

typedef unsigned char byte;
typedef unsigned short word;

#ifndef boolean
typedef byte boolean;
#endif

typedef struct gl_vertex
{
	float x;
	float y;
	float z;
} gl_vertex;

typedef struct gl_color_4ub {
	byte r, g, b, a; // MUST be this byte order! Don't change!
} gl_color_4ub;

typedef struct gl_color_4f {
	float r, g, b, a;
} gl_color_4f;

typedef union gl_texture_coord {
	struct {
		float u, v;
	} uv;
	struct {
		float q, r, s, t;
	} qrst;
} gl_texture_coord;

struct gl_api { // this is so we can load the apropriate OpenGL DLL on Windows, also for extension routines

#ifdef GL_LOAD_API_DYNAMICALLY

/*#ifdef _WINGDI_
	int (WINAPI *ChoosePixelFormat)(HDC, CONST PIXELFORMATDESCRIPTOR *);
	int (WINAPI *DescribePixelFormat)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
	BOOL (WINAPI *SetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
	int (WINAPI *GetPixelFormat)(HDC);
	BOOL (WINAPI *SwapBuffers)(HDC);

	BOOL (WINAPI *CopyContext)(HGLRC, HGLRC, UINT);
	HGLRC (WINAPI *CreateContext)(HDC);
	HGLRC (WINAPI *CreateLayerContext)(HDC, int);
	BOOL (WINAPI *DeleteContext)(HGLRC);
	HGLRC (WINAPI *GetCurrentContext)(VOID);
	HDC (WINAPI *GetCurrentDC)(VOID);
	PROC (WINAPI *GetProcAddress)(LPCSTR);
	BOOL (WINAPI *MakeCurrent)(HDC, HGLRC);
	BOOL (WINAPI *ShareLists)(HGLRC, HGLRC);
	BOOL (WINAPI *UseFontBitmaps)(HDC, DWORD, DWORD, DWORD);

	BOOL (WINAPI *UseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT);

	BOOL (WINAPI *DescribeLayerPlane)(HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR);
	int (WINAPI *SetLayerPaletteEntries)(HDC, int, int, int, CONST COLORREF *);
	int (WINAPI *GetLayerPaletteEntries)(HDC, int, int, int, COLORREF *);
	BOOL (WINAPI *RealizeLayerPalette)(HDC, int, BOOL);
	BOOL (WINAPI *SwapLayerBuffers)(HDC, UINT);
#endif*/

	void (APIENTRY *glAccum)(GLenum op, GLfloat value);
	void (APIENTRY *glAlphaFunc)(GLenum func, GLclampf ref);
	GLboolean (APIENTRY *glAreTexturesResident)(GLsizei n, const GLuint *textures, GLboolean *residences);
	void (APIENTRY *glArrayElement)(GLint i);
	void (APIENTRY *glBegin)(GLenum mode);
	void (APIENTRY *glBindTexture)(GLenum target, GLuint texture);
	void (APIENTRY *glBitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
	void (APIENTRY *glBlendFunc)(GLenum sfactor, GLenum dfactor);
	void (APIENTRY *glCallList)(GLuint list);
	void (APIENTRY *glCallLists)(GLsizei n, GLenum type, const GLvoid *lists);
	void (APIENTRY *glClear)(GLbitfield mask);
	void (APIENTRY *glClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void (APIENTRY *glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void (APIENTRY *glClearDepth)(GLclampd depth);
	void (APIENTRY *glClearIndex)(GLfloat c);
	void (APIENTRY *glClearStencil)(GLint s);
	void (APIENTRY *glClipPlane)(GLenum plane, const GLdouble *equation);
	void (APIENTRY *glColor3b)(GLbyte red, GLbyte green, GLbyte blue);
	void (APIENTRY *glColor3bv)(const GLbyte *v);
	void (APIENTRY *glColor3d)(GLdouble red, GLdouble green, GLdouble blue);
	void (APIENTRY *glColor3dv)(const GLdouble *v);
	void (APIENTRY *glColor3f)(GLfloat red, GLfloat green, GLfloat blue);
	void (APIENTRY *glColor3fv)(const GLfloat *v);
	void (APIENTRY *glColor3i)(GLint red, GLint green, GLint blue);
	void (APIENTRY *glColor3iv)(const GLint *v);
	void (APIENTRY *glColor3s)(GLshort red, GLshort green, GLshort blue);
	void (APIENTRY *glColor3sv)(const GLshort *v);
	void (APIENTRY *glColor3ub)(GLubyte red, GLubyte green, GLubyte blue);
	void (APIENTRY *glColor3ubv)(const GLubyte *v);
	void (APIENTRY *glColor3ui)(GLuint red, GLuint green, GLuint blue);
	void (APIENTRY *glColor3uiv)(const GLuint *v);
	void (APIENTRY *glColor3us)(GLushort red, GLushort green, GLushort blue);
	void (APIENTRY *glColor3usv)(const GLushort *v);
	void (APIENTRY *glColor4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
	void (APIENTRY *glColor4bv)(const GLbyte *v);
	void (APIENTRY *glColor4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
	void (APIENTRY *glColor4dv)(const GLdouble *v);
	void (APIENTRY *glColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void (APIENTRY *glColor4fv)(const GLfloat *v);
	void (APIENTRY *glColor4i)(GLint red, GLint green, GLint blue, GLint alpha);
	void (APIENTRY *glColor4iv)(const GLint *v);
	void (APIENTRY *glColor4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha);
	void (APIENTRY *glColor4sv)(const GLshort *v);
	void (APIENTRY *glColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
	void (APIENTRY *glColor4ubv)(const GLubyte *v);
	void (APIENTRY *glColor4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha);
	void (APIENTRY *glColor4uiv)(const GLuint *v);
	void (APIENTRY *glColor4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha);
	void (APIENTRY *glColor4usv)(const GLushort *v);
	void (APIENTRY *glColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
	void (APIENTRY *glColorMaterial)(GLenum face, GLenum mode);
	void (APIENTRY *glColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void (APIENTRY *glCopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
	void (APIENTRY *glCopyTexImage1D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
	void (APIENTRY *glCopyTexImage2D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
	void (APIENTRY *glCopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
	void (APIENTRY *glCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void (APIENTRY *glCullFace)(GLenum mode);
	void (APIENTRY *glDeleteLists)(GLuint list, GLsizei range);
	void (APIENTRY *glDeleteTextures)(GLsizei n, const GLuint *textures);
	void (APIENTRY *glDepthFunc)(GLenum func);
	void (APIENTRY *glDepthMask)(GLboolean flag);
	void (APIENTRY *glDepthRange)(GLclampd zNear, GLclampd zFar);
	void (APIENTRY *glDisable)(GLenum cap);
	void (APIENTRY *glDisableClientState)(GLenum array);
	void (APIENTRY *glDrawArrays)(GLenum mode, GLint first, GLsizei count);
	void (APIENTRY *glDrawBuffer)(GLenum mode);
	void (APIENTRY *glDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
	void (APIENTRY *glDrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
	void (APIENTRY *glEdgeFlag)(GLboolean flag);
	void (APIENTRY *glEdgeFlagPointer)(GLsizei stride, const GLvoid *pointer);
	void (APIENTRY *glEdgeFlagv)(const GLboolean *flag);
	void (APIENTRY *glEnable)(GLenum cap);
	void (APIENTRY *glEnableClientState)(GLenum array);
	void (APIENTRY *glEnd)(void);
	void (APIENTRY *glEndList)(void);
	void (APIENTRY *glEvalCoord1d)(GLdouble u);
	void (APIENTRY *glEvalCoord1dv)(const GLdouble *u);
	void (APIENTRY *glEvalCoord1f)(GLfloat u);
	void (APIENTRY *glEvalCoord1fv)(const GLfloat *u);
	void (APIENTRY *glEvalCoord2d)(GLdouble u, GLdouble v);
	void (APIENTRY *glEvalCoord2dv)(const GLdouble *u);
	void (APIENTRY *glEvalCoord2f)(GLfloat u, GLfloat v);
	void (APIENTRY *glEvalCoord2fv)(const GLfloat *u);
	void (APIENTRY *glEvalMesh1)(GLenum mode, GLint i1, GLint i2);
	void (APIENTRY *glEvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
	void (APIENTRY *glEvalPoint1)(GLint i);
	void (APIENTRY *glEvalPoint2)(GLint i, GLint j);
	void (APIENTRY *glFeedbackBuffer)(GLsizei size, GLenum type, GLfloat *buffer);
	void (APIENTRY *glFinish)(void);
	void (APIENTRY *glFlush)(void);
	void (APIENTRY *glFogf)(GLenum pname, GLfloat param);
	void (APIENTRY *glFogfv)(GLenum pname, const GLfloat *params);
	void (APIENTRY *glFogi)(GLenum pname, GLint param);
	void (APIENTRY *glFogiv)(GLenum pname, const GLint *params);
	void (APIENTRY *glFrontFace)(GLenum mode);
	void (APIENTRY *glFrustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	GLuint (APIENTRY *glGenLists)(GLsizei range);
	void (APIENTRY *glGenTextures)(GLsizei n, GLuint *textures);
	void (APIENTRY *glGetBooleanv)(GLenum pname, GLboolean *params);
	void (APIENTRY *glGetClipPlane)(GLenum plane, GLdouble *equation);
	void (APIENTRY *glGetDoublev)(GLenum pname, GLdouble *params);
	GLenum (APIENTRY *glGetError)(void);
	void (APIENTRY *glGetFloatv)(GLenum pname, GLfloat *params);
	void (APIENTRY *glGetIntegerv)(GLenum pname, GLint *params);
	void (APIENTRY *glGetLightfv)(GLenum light, GLenum pname, GLfloat *params);
	void (APIENTRY *glGetLightiv)(GLenum light, GLenum pname, GLint *params);
	void (APIENTRY *glGetMapdv)(GLenum target, GLenum query, GLdouble *v);
	void (APIENTRY *glGetMapfv)(GLenum target, GLenum query, GLfloat *v);
	void (APIENTRY *glGetMapiv)(GLenum target, GLenum query, GLint *v);
	void (APIENTRY *glGetMaterialfv)(GLenum face, GLenum pname, GLfloat *params);
	void (APIENTRY *glGetMaterialiv)(GLenum face, GLenum pname, GLint *params);
	void (APIENTRY *glGetPixelMapfv)(GLenum map, GLfloat *values);
	void (APIENTRY *glGetPixelMapuiv)(GLenum map, GLuint *values);
	void (APIENTRY *glGetPixelMapusv)(GLenum map, GLushort *values);
	void (APIENTRY *glGetPointerv)(GLenum pname, GLvoid* *params);
	void (APIENTRY *glGetPolygonStipple)(GLubyte *mask);
	const GLubyte * const (APIENTRY *glGetString)(GLenum name);
	void (APIENTRY *glGetTexEnvfv)(GLenum target, GLenum pname, GLfloat *params);
	void (APIENTRY *glGetTexEnviv)(GLenum target, GLenum pname, GLint *params);
	void (APIENTRY *glGetTexGendv)(GLenum coord, GLenum pname, GLdouble *params);
	void (APIENTRY *glGetTexGenfv)(GLenum coord, GLenum pname, GLfloat *params);
	void (APIENTRY *glGetTexGeniv)(GLenum coord, GLenum pname, GLint *params);
	void (APIENTRY *glGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
	void (APIENTRY *glGetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
	void (APIENTRY *glGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
	void (APIENTRY *glGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
	void (APIENTRY *glGetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
	void (APIENTRY *glHint)(GLenum target, GLenum mode);
	void (APIENTRY *glIndexMask)(GLuint mask);
	void (APIENTRY *glIndexPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
	void (APIENTRY *glIndexd)(GLdouble c);
	void (APIENTRY *glIndexdv)(const GLdouble *c);
	void (APIENTRY *glIndexf)(GLfloat c);
	void (APIENTRY *glIndexfv)(const GLfloat *c);
	void (APIENTRY *glIndexi)(GLint c);
	void (APIENTRY *glIndexiv)(const GLint *c);
	void (APIENTRY *glIndexs)(GLshort c);
	void (APIENTRY *glIndexsv)(const GLshort *c);
	void (APIENTRY *glIndexub)(GLubyte c);
	void (APIENTRY *glIndexubv)(const GLubyte *c);
	void (APIENTRY *glInitNames)(void);
	void (APIENTRY *glInterleavedArrays)(GLenum format, GLsizei stride, const GLvoid *pointer);
	GLboolean (APIENTRY *glIsEnabled)(GLenum cap);
	GLboolean (APIENTRY *glIsList)(GLuint list);
	GLboolean (APIENTRY *glIsTexture)(GLuint texture);
	void (APIENTRY *glLightModelf)(GLenum pname, GLfloat param);
	void (APIENTRY *glLightModelfv)(GLenum pname, const GLfloat *params);
	void (APIENTRY *glLightModeli)(GLenum pname, GLint param);
	void (APIENTRY *glLightModeliv)(GLenum pname, const GLint *params);
	void (APIENTRY *glLightf)(GLenum light, GLenum pname, GLfloat param);
	void (APIENTRY *glLightfv)(GLenum light, GLenum pname, const GLfloat *params);
	void (APIENTRY *glLighti)(GLenum light, GLenum pname, GLint param);
	void (APIENTRY *glLightiv)(GLenum light, GLenum pname, const GLint *params);
	void (APIENTRY *glLineStipple)(GLint factor, GLushort pattern);
	void (APIENTRY *glLineWidth)(GLfloat width);
	void (APIENTRY *glListBase)(GLuint base);
	void (APIENTRY *glLoadIdentity)(void);
	void (APIENTRY *glLoadMatrixd)(const GLdouble *m);
	void (APIENTRY *glLoadMatrixf)(const GLfloat *m);
	void (APIENTRY *glLoadName)(GLuint name);
	void (APIENTRY *glLogicOp)(GLenum opcode);
	void (APIENTRY *glMap1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
	void (APIENTRY *glMap1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
	void (APIENTRY *glMap2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
	void (APIENTRY *glMap2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
	void (APIENTRY *glMapGrid1d)(GLint un, GLdouble u1, GLdouble u2);
	void (APIENTRY *glMapGrid1f)(GLint un, GLfloat u1, GLfloat u2);
	void (APIENTRY *glMapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
	void (APIENTRY *glMapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
	void (APIENTRY *glMaterialf)(GLenum face, GLenum pname, GLfloat param);
	void (APIENTRY *glMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
	void (APIENTRY *glMateriali)(GLenum face, GLenum pname, GLint param);
	void (APIENTRY *glMaterialiv)(GLenum face, GLenum pname, const GLint *params);
	void (APIENTRY *glMatrixMode)(GLenum mode);
	void (APIENTRY *glMultMatrixd)(const GLdouble *m);
	void (APIENTRY *glMultMatrixf)(const GLfloat *m);
	void (APIENTRY *glNewList)(GLuint list, GLenum mode);
	void (APIENTRY *glNormal3b)(GLbyte nx, GLbyte ny, GLbyte nz);
	void (APIENTRY *glNormal3bv)(const GLbyte *v);
	void (APIENTRY *glNormal3d)(GLdouble nx, GLdouble ny, GLdouble nz);
	void (APIENTRY *glNormal3dv)(const GLdouble *v);
	void (APIENTRY *glNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
	void (APIENTRY *glNormal3fv)(const GLfloat *v);
	void (APIENTRY *glNormal3i)(GLint nx, GLint ny, GLint nz);
	void (APIENTRY *glNormal3iv)(const GLint *v);
	void (APIENTRY *glNormal3s)(GLshort nx, GLshort ny, GLshort nz);
	void (APIENTRY *glNormal3sv)(const GLshort *v);
	void (APIENTRY *glNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
	void (APIENTRY *glOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	void (APIENTRY *glPassThrough)(GLfloat token);
	void (APIENTRY *glPixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat *values);
	void (APIENTRY *glPixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint *values);
	void (APIENTRY *glPixelMapusv)(GLenum map, GLsizei mapsize, const GLushort *values);
	void (APIENTRY *glPixelStoref)(GLenum pname, GLfloat param);
	void (APIENTRY *glPixelStorei)(GLenum pname, GLint param);
	void (APIENTRY *glPixelTransferf)(GLenum pname, GLfloat param);
	void (APIENTRY *glPixelTransferi)(GLenum pname, GLint param);
	void (APIENTRY *glPixelZoom)(GLfloat xfactor, GLfloat yfactor);
	void (APIENTRY *glPointSize)(GLfloat size);
	void (APIENTRY *glPolygonMode)(GLenum face, GLenum mode);
	void (APIENTRY *glPolygonOffset)(GLfloat factor, GLfloat units);
	void (APIENTRY *glPolygonStipple)(const GLubyte *mask);
	void (APIENTRY *glPopAttrib)(void);
	void (APIENTRY *glPopClientAttrib)(void);
	void (APIENTRY *glPopMatrix)(void);
	void (APIENTRY *glPopName)(void);
	void (APIENTRY *glPrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities);
	void (APIENTRY *glPushAttrib)(GLbitfield mask);
	void (APIENTRY *glPushClientAttrib)(GLbitfield mask);
	void (APIENTRY *glPushMatrix)(void);
	void (APIENTRY *glPushName)(GLuint name);
	void (APIENTRY *glRasterPos2d)(GLdouble x, GLdouble y);
	void (APIENTRY *glRasterPos2dv)(const GLdouble *v);
	void (APIENTRY *glRasterPos2f)(GLfloat x, GLfloat y);
	void (APIENTRY *glRasterPos2fv)(const GLfloat *v);
	void (APIENTRY *glRasterPos2i)(GLint x, GLint y);
	void (APIENTRY *glRasterPos2iv)(const GLint *v);
	void (APIENTRY *glRasterPos2s)(GLshort x, GLshort y);
	void (APIENTRY *glRasterPos2sv)(const GLshort *v);
	void (APIENTRY *glRasterPos3d)(GLdouble x, GLdouble y, GLdouble z);
	void (APIENTRY *glRasterPos3dv)(const GLdouble *v);
	void (APIENTRY *glRasterPos3f)(GLfloat x, GLfloat y, GLfloat z);
	void (APIENTRY *glRasterPos3fv)(const GLfloat *v);
	void (APIENTRY *glRasterPos3i)(GLint x, GLint y, GLint z);
	void (APIENTRY *glRasterPos3iv)(const GLint *v);
	void (APIENTRY *glRasterPos3s)(GLshort x, GLshort y, GLshort z);
	void (APIENTRY *glRasterPos3sv)(const GLshort *v);
	void (APIENTRY *glRasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void (APIENTRY *glRasterPos4dv)(const GLdouble *v);
	void (APIENTRY *glRasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void (APIENTRY *glRasterPos4fv)(const GLfloat *v);
	void (APIENTRY *glRasterPos4i)(GLint x, GLint y, GLint z, GLint w);
	void (APIENTRY *glRasterPos4iv)(const GLint *v);
	void (APIENTRY *glRasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w);
	void (APIENTRY *glRasterPos4sv)(const GLshort *v);
	void (APIENTRY *glReadBuffer)(GLenum mode);
	void (APIENTRY *glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
	void (APIENTRY *glRectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
	void (APIENTRY *glRectdv)(const GLdouble *v1, const GLdouble *v2);
	void (APIENTRY *glRectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
	void (APIENTRY *glRectfv)(const GLfloat *v1, const GLfloat *v2);
	void (APIENTRY *glRecti)(GLint x1, GLint y1, GLint x2, GLint y2);
	void (APIENTRY *glRectiv)(const GLint *v1, const GLint *v2);
	void (APIENTRY *glRects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
	void (APIENTRY *glRectsv)(const GLshort *v1, const GLshort *v2);
	GLint (APIENTRY *glRenderMode)(GLenum mode);
	void (APIENTRY *glRotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
	void (APIENTRY *glRotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void (APIENTRY *glScaled)(GLdouble x, GLdouble y, GLdouble z);
	void (APIENTRY *glScalef)(GLfloat x, GLfloat y, GLfloat z);
	void (APIENTRY *glScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
	void (APIENTRY *glSelectBuffer)(GLsizei size, GLuint *buffer);
	void (APIENTRY *glShadeModel)(GLenum mode);
	void (APIENTRY *glStencilFunc)(GLenum func, GLint ref, GLuint mask);
	void (APIENTRY *glStencilMask)(GLuint mask);
	void (APIENTRY *glStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
	void (APIENTRY *glTexCoord1d)(GLdouble s);
	void (APIENTRY *glTexCoord1dv)(const GLdouble *v);
	void (APIENTRY *glTexCoord1f)(GLfloat s);
	void (APIENTRY *glTexCoord1fv)(const GLfloat *v);
	void (APIENTRY *glTexCoord1i)(GLint s);
	void (APIENTRY *glTexCoord1iv)(const GLint *v);
	void (APIENTRY *glTexCoord1s)(GLshort s);
	void (APIENTRY *glTexCoord1sv)(const GLshort *v);
	void (APIENTRY *glTexCoord2d)(GLdouble s, GLdouble t);
	void (APIENTRY *glTexCoord2dv)(const GLdouble *v);
	void (APIENTRY *glTexCoord2f)(GLfloat s, GLfloat t);
	void (APIENTRY *glTexCoord2fv)(const GLfloat *v);
	void (APIENTRY *glTexCoord2i)(GLint s, GLint t);
	void (APIENTRY *glTexCoord2iv)(const GLint *v);
	void (APIENTRY *glTexCoord2s)(GLshort s, GLshort t);
	void (APIENTRY *glTexCoord2sv)(const GLshort *v);
	void (APIENTRY *glTexCoord3d)(GLdouble s, GLdouble t, GLdouble r);
	void (APIENTRY *glTexCoord3dv)(const GLdouble *v);
	void (APIENTRY *glTexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
	void (APIENTRY *glTexCoord3fv)(const GLfloat *v);
	void (APIENTRY *glTexCoord3i)(GLint s, GLint t, GLint r);
	void (APIENTRY *glTexCoord3iv)(const GLint *v);
	void (APIENTRY *glTexCoord3s)(GLshort s, GLshort t, GLshort r);
	void (APIENTRY *glTexCoord3sv)(const GLshort *v);
	void (APIENTRY *glTexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
	void (APIENTRY *glTexCoord4dv)(const GLdouble *v);
	void (APIENTRY *glTexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
	void (APIENTRY *glTexCoord4fv)(const GLfloat *v);
	void (APIENTRY *glTexCoord4i)(GLint s, GLint t, GLint r, GLint q);
	void (APIENTRY *glTexCoord4iv)(const GLint *v);
	void (APIENTRY *glTexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q);
	void (APIENTRY *glTexCoord4sv)(const GLshort *v);
	void (APIENTRY *glTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void (APIENTRY *glTexEnvf)(GLenum target, GLenum pname, GLfloat param);
	void (APIENTRY *glTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
	void (APIENTRY *glTexEnvi)(GLenum target, GLenum pname, GLint param);
	void (APIENTRY *glTexEnviv)(GLenum target, GLenum pname, const GLint *params);
	void (APIENTRY *glTexGend)(GLenum coord, GLenum pname, GLdouble param);
	void (APIENTRY *glTexGendv)(GLenum coord, GLenum pname, const GLdouble *params);
	void (APIENTRY *glTexGenf)(GLenum coord, GLenum pname, GLfloat param);
	void (APIENTRY *glTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);
	void (APIENTRY *glTexGeni)(GLenum coord, GLenum pname, GLint param);
	void (APIENTRY *glTexGeniv)(GLenum coord, GLenum pname, const GLint *params);
	void (APIENTRY *glTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	void (APIENTRY *glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	void (APIENTRY *glTexParameterf)(GLenum target, GLenum pname, GLfloat param);
	void (APIENTRY *glTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
	void (APIENTRY *glTexParameteri)(GLenum target, GLenum pname, GLint param);
	void (APIENTRY *glTexParameteriv)(GLenum target, GLenum pname, const GLint *params);
	void (APIENTRY *glTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
	void (APIENTRY *glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
	void (APIENTRY *glTranslated)(GLdouble x, GLdouble y, GLdouble z);
	void (APIENTRY *glTranslatef)(GLfloat x, GLfloat y, GLfloat z);
	void (APIENTRY *glVertex2d)(GLdouble x, GLdouble y);
	void (APIENTRY *glVertex2dv)(const GLdouble *v);
	void (APIENTRY *glVertex2f)(GLfloat x, GLfloat y);
	void (APIENTRY *glVertex2fv)(const GLfloat *v);
	void (APIENTRY *glVertex2i)(GLint x, GLint y);
	void (APIENTRY *glVertex2iv)(const GLint *v);
	void (APIENTRY *glVertex2s)(GLshort x, GLshort y);
	void (APIENTRY *glVertex2sv)(const GLshort *v);
	void (APIENTRY *glVertex3d)(GLdouble x, GLdouble y, GLdouble z);
	void (APIENTRY *glVertex3dv)(const GLdouble *v);
	void (APIENTRY *glVertex3f)(GLfloat x, GLfloat y, GLfloat z);
	void (APIENTRY *glVertex3fv)(const GLfloat *v);
	void (APIENTRY *glVertex3i)(GLint x, GLint y, GLint z);
	void (APIENTRY *glVertex3iv)(const GLint *v);
	void (APIENTRY *glVertex3s)(GLshort x, GLshort y, GLshort z);
	void (APIENTRY *glVertex3sv)(const GLshort *v);
	void (APIENTRY *glVertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	void (APIENTRY *glVertex4dv)(const GLdouble *v);
	void (APIENTRY *glVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void (APIENTRY *glVertex4fv)(const GLfloat *v);
	void (APIENTRY *glVertex4i)(GLint x, GLint y, GLint z, GLint w);
	void (APIENTRY *glVertex4iv)(const GLint *v);
	void (APIENTRY *glVertex4s)(GLshort x, GLshort y, GLshort z, GLshort w);
	void (APIENTRY *glVertex4sv)(const GLshort *v);
	void (APIENTRY *glVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void (APIENTRY *glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
	
	// WGL
	BOOL (WINAPI *wglCopyContext)(HGLRC, HGLRC, UINT);
	HGLRC (WINAPI *wglCreateContext)(HDC);
	HGLRC (WINAPI *wglCreateLayerContext)(HDC, int);
	BOOL (WINAPI *wglDeleteContext)(HGLRC);
	HGLRC (WINAPI *wglGetCurrentContext)(VOID);
	HDC (WINAPI *wglGetCurrentDC)(VOID);
	PROC (WINAPI *wglGetProcAddress)(LPCSTR);
	BOOL (WINAPI *wglMakeCurrent)(HDC, HGLRC);
	BOOL (WINAPI *wglShareLists)(HGLRC, HGLRC);

#endif // windows

#endif // GL_LOAD_API_DYNAMICALLY

	void *end_standard_api; // this marks the end of standard API routines that are required to run

	// extensions

	// GL_ARB_multitexture
	void (APIENTRY *glActiveTextureARB)(GLenum texture);
	void (APIENTRY *glClientActiveTextureARB)(GLenum texture);
	void (APIENTRY *glMultiTexCoord4fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
	void (APIENTRY *glMultiTexCoord2fvARB)(GLenum target, const GLfloat *v);

	// GL_ARB_imaging / GL_EXT_blend_color
	void (APIENTRY *glBlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);

	// nVidia GL_NV_register_combiners
	void (APIENTRY *glCombinerParameteriNV)(GLenum pname, GLint param);
	void (APIENTRY *glCombinerInputNV)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
	void (APIENTRY *glCombinerOutputNV)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
	void (APIENTRY *glFinalCombinerInputNV)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);

	// GL_ARB_texture_compression
	void (APIENTRY *glCompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
	void (APIENTRY *glCompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
	void (APIENTRY *glCompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
	void (APIENTRY *glCompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
	void (APIENTRY *glCompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
	void (APIENTRY *glCompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);
	void (APIENTRY *glGetCompressedTexImageARB)(GLenum target, GLint level, void *img);

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
	// WGL_EXT_swap_control (MS Windows only)
	int (APIENTRY *wglSwapIntervalEXT)(int);

	// WGL_3DFX_gamma_control (MS Windows only)
	BOOL (APIENTRY *wglSetDeviceGammaRamp3DFX)(HDC, LPVOID);
	BOOL (APIENTRY *wglGetDeviceGammaRamp3DFX)(HDC, LPVOID);
#endif
};

struct gl_state_global {
	boolean engine_initialized;
	
	M3tDrawEngineCaps draw_engine_caps;
	M3tDrawEngineMethods draw_engine_methods;
	M3tDrawContextMethods draw_context_methods;

        M3tDisplayMode display_mode;

	M3tDrawContextType context_type;

/* 09/01/2000 we will never be using vertex arrays for oni	
	gl_vertex *vertices;
	gl_color_4ub *vertex_colors;
	gl_texture_coord *uvs0;
	gl_texture_coord *uvs1;
	unsigned long vertex_count;
	unsigned long vertex_format_flags;
	GLenum mode; // GL_POINTS, etc...
	GLint tex_coord_dimension; // must be either 2 or 4 */

	M3tTextureMap *texture0;
	M3tTextureMap *texture1;

	word camera_mode;
	boolean update_camera_view_data;
	boolean update_camera_static_data;

	UUtInt32 *state_int;
	void **state_ptr;

	GLfloat depth_bias;

	gl_color_4ub constant_color;

	unsigned long max_texture_size;
	int has_double_buffer;
	unsigned long num_tmu;

	char *vendor;
	char *renderer;
	char *version;
	char *extensions;
	
	int multitexture;
	int mipmap_offset;
	GLint max_nvidia_general_combiners;
	boolean nv_register_combiners_enabled;

	boolean depth_buffer_reads_disabled;
	boolean fog_disabled_3Dfx;
	boolean packed_pixels_disabled;
	boolean dxt1_textures_disabled;

	// texture compression goodness
	GLint compressed_texture_formats[MAX_COMPRESSED_TEXTURE_FORMATS];
	GLint number_of_compressed_texture_formats; // this being 0 means there is no texture compression
	boolean compress_textures; // if TRUE and number_of_compressed_texture_formats > 0, compress textures

	// fog params
	float fog_start, fog_end;
	boolean fog_start_changing, fog_end_changing;
	float fog_start_desired, fog_start_delta;
	float fog_end_desired, fog_end_delta;
	gl_color_4f fog_color;
	boolean fog_enabled;
	gl_color_4f clear_color;

	GLint error_code;
	char *error_string;

	word geom_draw_mode;
	boolean buffer_clear;
	boolean double_buffer;

	TMtPrivateData *texture_private_data;
	void *converted_data_buffer;

	// platform-specific parameters
#if UUmSDL
	SDL_GLContext context;
#elif defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
	HDC device_context;
	HGLRC render_context;
	boolean gl_vsync_enabled;
#elif defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Mac)
	void *context; // AGLContext
#else
	#error "unknown platform!"
#endif

#ifdef GL_ENABLE_ALPHA_FADE_CODE
	boolean blend_to_frame_buffer_with_constant_alpha;
#endif

	// S.S. 11/09/2000 added this for texture management
	UUtUns32 frame_count; // internal frame counter used for timestamping textures
	UUtUns32 current_texture_memory;
	UUtUns32 maximum_allowed_texture_memory; // regulates how much texture memory we will use before we start discarding old textures
	M3tTextureMap **loaded_texture_array;
	UUtUns32 loaded_texture_array_length;
	UUtUns32 num_loaded_textures;

	void (*gl_display_back_buffer)(void); // need vendor specific code on the PC, sadly.
};

/*---------- macros */

#undef assert
#define assert(x) UUmAssert(x)

/*---------- globals */

extern struct gl_state_global *gl;
extern struct gl_api *gl_api;


/*---------- prototypes */

// gl_engine.c

UUtBool gl_draw_engine_initialize(void);

UUtBool gl_platform_initialize(void);
void gl_platform_dispose(void);

// gl_utilty.c

boolean opengl_available(void);
boolean initialize_opengl(void);

UUtBool gl_texture_map_create(M3tTextureMap *texture_map);
UUtBool gl_texture_map_delete(M3tTextureMap *texture_map);

void gl_reset_fog_parameters(void);
void gl_fog_update(int frames);

void gl_camera_update(word camera_mode);
void gl_matrix4x3_oni_to_gl(M3tMatrix4x3 *in_matrix, float *out_gl_matrix);
void gl_matrix4x4_oni_to_gl(M3tMatrix4x4 *in_matrix, float *out_gl_matrix);
UUtBool gl_texture_map_format_available(IMtPixelType in_texel_type);
void gl_depth_mode_set(boolean read, boolean write);
void gl_depth_bias_set(float bias);

UUtError gl_texture_map_proc_handler(TMtTemplateProc_Message message,
	void *instance_ptr, void *private_data);

// no vertex arrays void gl_render_buffered_geometry(void);
void gl_set_textures(M3tTextureMap *texture0, M3tTextureMap *texture1,
	GLenum sfactor, GLenum dfactor);

void gl_sync_to_vtrace(boolean enable);

GLenum gl_GetError(void);

// implementation of these is platform-specific
void gl_display_back_buffer(void);
void gl_matrox_display_back_buffer(void);
int gl_enumerate_valid_display_modes(M3tDisplayMode display_mode_list[M3cMaxDisplayModes]);

boolean gl_load_opengl_dll(void); // MS Windows

// gl_macos.c
void* aglGetProcAddress(const char *inString);

#endif // __GL_ENGINE__

