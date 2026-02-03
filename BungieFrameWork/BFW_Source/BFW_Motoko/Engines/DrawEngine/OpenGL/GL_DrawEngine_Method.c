/*
	FILE:	GL_DrawEngine_Method.c

	AUTHOR:	Kevin Armstrong, Michael Evans

	CREATED: January 5, 1998

	PURPOSE:

	Copyright 1997 - 1998

*/

#ifndef __ONADA__

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_TemplateManager.h"
#include "BFW_BitVector.h"
#include "BFW_Console.h"
#include "BFW_ScriptLang.h"

#include "Motoko_Manager.h"

#include "OGL_DrawGeom_Common.h"

#include "GL_DC_Private.h"
#include "GL_Utility_Functions.h"
#include "GL_Draw_Functions.h"
#include "GL_Platform.h"
#include "GL_DrawEngine_Method.h"

UUtBool GLgMultiTexture = UUcFalse;
UUtBool GLgVertexArray = UUcFalse;

GLuint GLgCurTexture = 0;

#define GLcMaxVerticies 65535

UUtBool		GLgBufferClear = UUcTrue;
UUtBool		GLgDoubleBuffer = UUcTrue;
UUtBool		GLgZTrick = UUcFalse;
UUtUns32	GLgClearColor = 0x00000000;



#if 0
GL_ALPHA, GL_ALPHA4, GL_ALPHA8, GL_ALPHA12, GL_ALPHA16, GL_LUMINANCE, GL_LUMINANCE4,
GL_LUMINANCE8, GL_LUMINANCE12, GL_LUMINANCE16, GL_LUMINANCE_ALPHA, GL_LUMINANCE4_ALPHA4,
GL_LUMINANCE6_ALPHA2, GL_LUMINANCE8_ALPHA8, GL_LUMINANCE12_ALPHA4, GL_LUMINANCE12_ALPHA12,
GL_LUMINANCE16_ALPHA16, GL_INTENSITY, GL_INTENSITY4, GL_INTENSITY8, GL_INTENSITY12,
GL_INTENSITY16, GL_RGB, GL_R3_G3_B2, GL_RGB4, GL_RGB5, GL_RGB8, GL_RGB10, GL_RGB12,
GL_RGB16, GL_RGBA, GL_RGBA2, GL_RGBA4, GL_RGB5_A1, GL_RGBA8, GL_RGB10_A2, GL_RGBA12, or GL_RGBA16.
#endif

#define GLcMaxElements (40 * 1024)

GLtDrawContextPrivate	*GLgDrawContextPrivate = NULL;

M3tDrawEngineCaps		GLgDrawEngineCaps;
M3tDrawEngineMethods	GLgDrawEngineMethods;
M3tDrawContextMethods	GLgDrawContextMethods;

UUtInt32				GLgTextureBytesDownloaded;


UUtError initialize_opengl(void);
void terminate_opengl(void);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------

//typedef void (GL_CALL *GLtActiveTextureARB)(GLenum target);
//typedef void (GL_CALL *GLtClientActiveTextureARB)(GLenum target);

// ----------------------------------------------------------------------
static void
GLrDrawEngine_Method_ContextPrivateDelete(
	void)
{
	if (GLgDrawContextPrivate->compiled_vertex_array_locked) {
		UUmAssert(OGLgCommon.ext_compiled_vertex_array);
		OGLgCommon.pglUnlockArraysEXT();
		GLgDrawContextPrivate->compiled_vertex_array_locked = UUcFalse;
	}

	OGLrCommon_State_Terminate();
	terminate_opengl();

	UUrMemory_Block_Delete(GLgDrawContextPrivate->compiled_vertex_array);
	UUrMemory_Block_Delete(GLgDrawContextPrivate);
	GLgDrawContextPrivate = NULL;
}

// ----------------------------------------------------------------------
static UUtError
GLrDrawEngine_Method_ContextPrivateNew(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	M3tDrawContextMethods*		*outDrawContextFuncs,
	UUtBool						inFullScreen,
	M3tDrawAPI*					outAPI)
{
	UUtError				errorCode;
	UUtUns16				activeDrawEngine;
	UUtUns16				activeDevice;
	UUtUns16				activeMode;
	M3tDrawEngineCaps*		drawEngineCaps;

	UUrStartupMessage("creating new opengl context");

	*outAPI = M3cDrawAPI_OpenGL;

	GLgDrawContextPrivate = UUrMemory_Block_NewClear(sizeof(*GLgDrawContextPrivate));
	UUmError_ReturnOnNull(GLgDrawContextPrivate);

	GLgDrawContextPrivate->compiled_vertex_array = UUrMemory_Block_New(sizeof(float) * 3 * GLcMaxVerticies);
	UUmError_ReturnOnNull(GLgDrawContextPrivate->compiled_vertex_array);

	GLgDrawContextPrivate->constant_r = 1.f;
	GLgDrawContextPrivate->constant_g = 1.f;
	GLgDrawContextPrivate->constant_b = 1.f;
	GLgDrawContextPrivate->constant_a = 1.f;


	/*
	 * Get the active draw engine info
	 */

		M3rManager_GetActiveDrawEngine(
			&activeDrawEngine,
			&activeDevice,
			&activeMode);

	 	drawEngineCaps = M3rDrawEngine_GetCaps(activeDrawEngine);

	/*
	 * initialize methods
	 */
		GLgDrawContextMethods.frameStart = GLrDrawContext_Method_Frame_Start;
		GLgDrawContextMethods.frameEnd = GLrDrawContext_Method_Frame_End;
		GLgDrawContextMethods.frameSync = GLrDrawContext_Method_Frame_Sync;
		GLgDrawContextMethods.triangle = (M3tDrawContextMethod_Triangle)GLrTriangle;
		GLgDrawContextMethods.quad = (M3tDrawContextMethod_Quad)GLrQuad;
		GLgDrawContextMethods.pent = (M3tDrawContextMethod_Pent)GLrPent;
		GLgDrawContextMethods.line = (M3tDrawContextMethod_Line)GLrLine;
		GLgDrawContextMethods.point = (M3tDrawContextMethod_Point)GLrPoint;
		GLgDrawContextMethods.triSprite = (M3tDrawContextMethod_TriSprite)GLrDrawContext_Method_TriSprite;
		GLgDrawContextMethods.sprite = (M3tDrawContextMethod_Sprite)GLrDrawContext_Method_Sprite;
		GLgDrawContextMethods.spriteArray = (M3tDrawContextMethod_SpriteArray)GLrDrawContext_Method_SpriteArray;
		GLgDrawContextMethods.screenCapture = GLrDrawContext_Method_ScreenCapture;
		GLgDrawContextMethods.pointVisible = GLrDrawContext_Method_PointVisible;
		GLgDrawContextMethods.textureFormatAvailable = OGLrCommon_TextureMap_TextureFormatAvailable;

		*outDrawContextFuncs = &GLgDrawContextMethods;

	GLgDrawContextPrivate->contextType = inDrawContextDescriptor->type;

	GLgDrawContextPrivate->width= drawEngineCaps->displayDevices[activeDevice].displayModes[activeMode].width;
	GLgDrawContextPrivate->height= drawEngineCaps->displayDevices[activeDevice].displayModes[activeMode].height;
	GLgDrawContextPrivate->bitDepth= drawEngineCaps->displayDevices[activeDevice].displayModes[activeMode].bitDepth;

	// Init some state
	GLgDrawContextPrivate->vertexList = NULL;
	GLgDrawContextPrivate->curBaseMap = NULL;
	GLgDrawContextPrivate->curLightMap = NULL;
	GLgDrawContextPrivate->curEnvironmentMap = NULL;


	initialize_opengl();

	return UUcError_None;

//failure:

	GLrDrawEngine_Method_ContextPrivateDelete();

	return errorCode;
}

static void
GLrDrawEngine_Method_Texture_ResetAll(
	void)
{
}

// ----------------------------------------------------------------------
static UUtError
GLrDrawEngine_Method_PrivateState_New(
	void*	inState_Private)
{

	return UUcError_None;
}

// This lets the engine delete a new private state structure
static void
GLrDrawEngine_Method_PrivateState_Delete(
	void*	inState_Private)
{
}

// This lets the engine update the state according to the update flags
static UUtError
GLrDrawEngine_Method_State_Update(
	void*			inState_Private,
	UUtUns32		inState_IntFlags,
	const UUtInt32*	inState_Int,
	UUtInt32		inState_PtrFlags,
	const void**	inState_Ptr)
{
	UUtBool				updateRegular = UUcFalse;
	UUtBool				updateSplit = UUcFalse;

	GLgDrawContextPrivate->stateInt = inState_Int;
	GLgDrawContextPrivate->statePtr = inState_Ptr;

	GLgDrawContextPrivate->curEnvironmentMap = (void *) inState_Ptr[M3cDrawStatePtrType_EnvTextureMap];

	if(inState_IntFlags & (1 << M3cDrawStateIntType_NumRealVertices)) {
		const UUtUns32 *bit_vector = inState_Ptr[M3cDrawStatePtrType_VertexBitVector];
		const M3tPointScreen *src = inState_Ptr[M3cDrawStatePtrType_ScreenPointArray];
		GLgDrawContextPrivate->vertexCount = inState_Int[M3cDrawStateIntType_NumRealVertices];

		if (bit_vector && src && GLgVertexArray && (inState_Int[M3cDrawStateIntType_NumRealVertices] > 0))
		{
			if (GLgDrawContextPrivate->compiled_vertex_array_locked) {
				OGLgCommon.pglUnlockArraysEXT();
				GLgDrawContextPrivate->compiled_vertex_array_locked = UUcFalse;
			}

			glVertexPointer(3, GL_FLOAT, 16, src);

			if (OGLgCommon.ext_compiled_vertex_array) {
				if (inState_Int[M3cDrawStateIntType_NumRealVertices] > 0) {
					OGLgCommon.pglLockArraysEXT(0, inState_Int[M3cDrawStateIntType_NumRealVertices]);
					GLgDrawContextPrivate->compiled_vertex_array_locked = UUcTrue;
				}
			}
		}
	}

	if( inState_IntFlags & (1 << M3cDrawStateIntType_ZBias ))
	{
		UUtInt32	value = inState_Int[M3cDrawStateIntType_ZBias];
		float		zbias = (float) value;
		OGLrCommon_DepthBias_Set( zbias );
	}

	if ((inState_IntFlags & (1 << M3cDrawStateIntType_ZCompare)) ||
		(inState_IntFlags & (1 << M3cDrawStateIntType_ZWrite)))
	{
		UUtBool zread = inState_Int[M3cDrawStateIntType_ZCompare] == M3cDrawState_ZCompare_On;
		UUtBool zwrite = inState_Int[M3cDrawStateIntType_ZWrite] == M3cDrawState_ZWrite_On;

		OGLrCommon_DepthMode_Set(zread, zwrite);
	}

	if((inState_Int[M3cDrawStateIntType_Appearence] != M3cDrawState_Appearence_Gouraud) &&
		(inState_Int[M3cDrawStateIntType_Fill] == M3cDrawState_Fill_Solid))
	{
		if(inState_PtrFlags & (1 << M3cDrawStatePtrType_BaseTextureMap)) {

			M3tTextureMap* textureMap = (M3tTextureMap*)inState_Ptr[M3cDrawStatePtrType_BaseTextureMap];

			GLgDrawContextPrivate->curBaseMap = textureMap;

			if (NULL == textureMap)
			{
				OGLrCommon_TextureMap_Select(NULL, GL_TEXTURE0_ARB);
			}
			else
			{
				OGLrCommon_TextureMap_Select(textureMap, GL_TEXTURE0_ARB);
			}
		}
	}
	else
	{
		GLgDrawContextPrivate->curBaseMap = NULL;
		OGLrCommon_TextureMap_Select(NULL, GL_TEXTURE0_ARB);
	}

	if (OGLgCommon.ext_arb_multitexture)
	{
		OGLrCommon_TextureMap_Select(NULL, GL_TEXTURE1_ARB);
	}
	else
	{
		GLgDrawContextPrivate->curLightMap = NULL;
	}

	if (!GLgMultiTexture)
	{
		OGLrCommon_TextureMap_Select(NULL, GL_TEXTURE1_ARB);
	}

	if (inState_IntFlags & (1 << M3cDrawStateIntType_Alpha)) {
		UUtUns32 	alpha = inState_Int[M3cDrawStateIntType_Alpha];

		GLgDrawContextPrivate->constant_a = alpha * (1.f / 255.f);
	}

	if (inState_IntFlags & ((1 << M3cDrawStateIntType_ConstantColor)))
	{
		UUtUns32	color = inState_Int[M3cDrawStateIntType_ConstantColor];
		UUtUns8		r = (UUtUns8) ((color & 0x00ff0000) >> 16);
		UUtUns8		g = (UUtUns8) ((color & 0x0000ff00) >> 8);
		UUtUns8		b = (UUtUns8) ((color & 0x000000ff) >> 0);

		GLgDrawContextPrivate->constant_r = r * (1.f / 255.f);
		GLgDrawContextPrivate->constant_g = g * (1.f / 255.f);
		GLgDrawContextPrivate->constant_b = b * (1.f / 255.f);
	}

	if(inState_IntFlags & ((1 << M3cDrawStateIntType_Alpha) | (1 << M3cDrawStateIntType_ConstantColor)))
	{
		glColor4f(
			GLgDrawContextPrivate->constant_r,
			GLgDrawContextPrivate->constant_g,
			GLgDrawContextPrivate->constant_b,
			GLgDrawContextPrivate->constant_a);
	}

	if(inState_IntFlags & (
			(1 << M3cDrawStateIntType_Appearence) |
			(1 << M3cDrawStateIntType_Interpolation) |
			(1 << M3cDrawStateIntType_Fill)))
	{
	}

	switch(inState_Int[M3cDrawStateIntType_Fill])
	{
		case M3cDrawState_Fill_Point:
		case M3cDrawState_Fill_Line:
			GLgDrawContextMethods.triangle = (M3tDrawContextMethod_Triangle)GLrTriangle_Wireframe;
			GLgDrawContextMethods.quad = (M3tDrawContextMethod_Quad)GLrQuad_Wireframe;
			GLgDrawContextMethods.pent = (M3tDrawContextMethod_Pent)GLrPent_Wireframe;
		break;

		case M3cDrawState_Fill_Solid:
		default:
			switch(inState_Int[M3cDrawStateIntType_Appearence])
			{
				case M3cDrawState_Appearence_Gouraud:
					GLgDrawContextMethods.triangle = (M3tDrawContextMethod_Triangle)GLrTriangle_Gourand;
					GLgDrawContextMethods.quad = (M3tDrawContextMethod_Quad)GLrQuad_Gourand;
					GLgDrawContextMethods.pent = (M3tDrawContextMethod_Pent)GLrPent_Gourand;
				break;

				case M3cDrawState_Appearence_Texture_Lit:
				case M3cDrawState_Appearence_Texture_Lit_EnvMap:
				case M3cDrawState_Appearence_Texture_Unlit:
					switch(inState_Int[M3cDrawStateIntType_VertexFormat])
					{
						case M3cDrawStateVertex_Unified:
							switch(inState_Int[M3cDrawStateIntType_Interpolation])
							{
								case M3cDrawState_Interpolation_None:
									GLgDrawContextMethods.triangle = (M3tDrawContextMethod_Triangle)GLrTriangle_Flat;
									GLgDrawContextMethods.quad = (M3tDrawContextMethod_Quad)GLrQuad_Flat;
									GLgDrawContextMethods.pent = (M3tDrawContextMethod_Pent)GLrPent_Flat;
								break;

								case M3cDrawState_Interpolation_Vertex:
									if ((GLgDrawContextPrivate->curEnvironmentMap != NULL) && (1.f == GLgDrawContextPrivate->constant_a)) {
										GLgDrawContextMethods.triangle = (M3tDrawContextMethod_Triangle)GLrTriangle_EnvironmentMap_1TMU;
										GLgDrawContextMethods.quad = (M3tDrawContextMethod_Quad)GLrQuad_EnvironmentMap_1TMU;
										GLgDrawContextMethods.pent = (M3tDrawContextMethod_Pent)GLrPent_EnvironmentMap_1TMU;
									}
									else {
										GLgDrawContextMethods.triangle = (M3tDrawContextMethod_Triangle)GLrTriangle;
										GLgDrawContextMethods.quad = (M3tDrawContextMethod_Quad)GLrQuad;
										GLgDrawContextMethods.pent = (M3tDrawContextMethod_Pent)GLrPent;
									}
								break;
							}
						break;

						case M3cDrawStateVertex_Split:
							GLgDrawContextMethods.triangle = (M3tDrawContextMethod_Triangle)GLrTriangleSplit_VertexLighting;
							GLgDrawContextMethods.quad = (M3tDrawContextMethod_Quad)GLrQuadSplit_VertexLighting;
							GLgDrawContextMethods.pent = (M3tDrawContextMethod_Pent)GLrPentSplit_VertexLighting;
						break;
					}
				break;
			}
	}

	GLgBufferClear = (UUtBool) inState_Int[M3cDrawStateIntType_BufferClear];
	GLgClearColor = inState_Int[M3cDrawStateIntType_ClearColor];
	GLgDoubleBuffer = (UUtBool) inState_Int[M3cDrawStateIntType_DoubleBuffer];

	return UUcError_None;
}



// ----------------------------------------------------------------------

void
GLrDrawEngine_Initialize(
	void)
{
	UUtInt32 itr;
	M3tDisplayMode displayModeList[16] =
	{
		{ 640,		480,	16,		0 },
		{ 800,		600,	16,		0 },
		{ 1024,		768,	16,		0 },
		{ 1152,		864,	16,		0 },
		{ 1280,		1024,	16,		0 },
		{ 1600,		1200,	16,		0 },
		{ 1920,		1080,	16,		0 },
		{ 1920,		1200,	16,		0 },

		{ 640,		480,	32,		0 },
		{ 800,		600,	32,		0 },
		{ 1024,		768,	32,		0 },
		{ 1152,		864,	32,		0 },
		{ 1280,		1024,	32,		0 },
		{ 1600,		1200,	32,		0 },
		{ 1920,		1080,	32,		0 },
		{ 1920,		1200,	32,		0 }
	};


	UUtError error;

	if(glGetString == NULL) return;

	GLgDrawEngineMethods.contextPrivateNew = GLrDrawEngine_Method_ContextPrivateNew;
	GLgDrawEngineMethods.contextPrivateDelete = GLrDrawEngine_Method_ContextPrivateDelete;
	GLgDrawEngineMethods.textureResetAll = GLrDrawEngine_Method_Texture_ResetAll;

	GLgDrawEngineMethods.privateStateSize = 0;
	GLgDrawEngineMethods.privateStateNew = GLrDrawEngine_Method_PrivateState_New;
	GLgDrawEngineMethods.privateStateDelete = GLrDrawEngine_Method_PrivateState_Delete;
	GLgDrawEngineMethods.privateStateUpdate = GLrDrawEngine_Method_State_Update;

	GLgDrawEngineCaps.engineFlags = M3cDrawEngineFlag_3DOnly;

	UUrString_Copy(GLgDrawEngineCaps.engineName, M3cDrawEngine_OpenGL, M3cMaxNameLen);
	GLgDrawEngineCaps.engineDriver[0] = 0;

	GLgDrawEngineCaps.engineVersion = 1;

	GLgDrawEngineCaps.numDisplayDevices = 1;

	GLgDrawEngineCaps.displayDevices[0].numDisplayModes = 16;

	for(itr = 0; itr < 16; itr++) {
		// NOTE: this should be built on the fly at some point
		GLgDrawEngineCaps.displayDevices[0].displayModes[itr] = displayModeList[itr];
	}

	error =
		M3rManager_Register_DrawEngine(
			&GLgDrawEngineCaps,
			&GLgDrawEngineMethods);
	if(error != UUcError_None)
	{
		UUrError_Report(UUcError_Generic, "Could not setup engine caps");
		return;
	}

	SLrGlobalVariable_Register_Bool("gl_multitexture", "enable opengl multitexturing", &GLgMultiTexture);
	SLrGlobalVariable_Register_Bool("gl_vertex_array", "enable opengl vertex arrays", &GLgVertexArray);

}

// ----------------------------------------------------------------------
void
GLrDrawEngine_Terminate(
	void)
{
}

// ----------------------------------------------------------------------

void terminate_opengl(void)
{
	// stop OpenGL
	GLrPlatform_Dispose();

	return;
}

UUtError initialize_opengl(void)
{
	UUtError error;
	GLfloat fog_color[4]= {0.5, 0.5, 0.5, 1.0};

	UUrStartupMessage("opengl platform initialization");
	error = GLrPlatform_Initialize();
	UUmError_ReturnOnErrorMsg(error, "could not initialize opengl");

	error = OGLrCommon_Initialize_OneTime();
	UUmError_ReturnOnError(error);	// XXX - At some point inform user that open gl did not have needed extensions

	GLgDrawContextPrivate->num_tmu = 1;

	if (OGLgCommon.ext_arb_multitexture) {
		OGLgCommon.pglActiveTextureARB(GL_TEXTURE0_ARB);
		OGLgCommon.pglClientActiveTextureARB(GL_TEXTURE0_ARB);
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &GLgDrawContextPrivate->num_tmu);
	}
	else {
		GLgDrawContextPrivate->num_tmu = 1;
	}

	UUrStartupMessage("supports %d texturing units", GLgDrawContextPrivate->num_tmu);

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &GLgDrawContextPrivate->max_texture_size);
	glGetIntegerv(GL_DOUBLEBUFFER, &GLgDrawContextPrivate->has_double_buffer);

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_PACK_ALIGNMENT,1);


	/* the depth range is set by the set_frame_parameters code */

	OGLrCommon_DepthMode_Set(UUcTrue,UUcTrue);

	OGLrCommon_State_Initialize((UUtUns16)GLgDrawContextPrivate->width, (UUtUns16)GLgDrawContextPrivate->height);

	// glFogfv(GL_FOG_COLOR,GLgDrawContextPrivate->fog_color);

	//set_opengl_texture(NULL);
	/* check for OpenGL Errors here */

	return UUcTrue;
}

#endif // #ifndef __ONADA__
