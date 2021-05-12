/*
	FILE:	OGL_DrawGeom_Common.c
	
	AUTHOR:	Brent Pease, Kevin Armstrong, Michael Evans
	
	CREATED: January 5, 1998
	
	PURPOSE: 
	
	Copyright 1997 - 1998

*/

#ifndef __ONADA__

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Console.h"

#include "Motoko_Manager.h"

#include "OGL_DrawGeom_Common.h"
#include "OGL_DrawGeom_Platform.h"


#define GL_RGB_S3TC   (0x83A0)
#define GL_RGB4_S3TC  (0x83A1)
#define GL_RGBA_S3TC  (0x83A2) 
#define GL_RGBA4_S3TC (0x83A3) 

/*---------- S.S. */

GLint gl_error;
gl_color_4ub current_vertex_color_gl= {0L}; // used by OGLrCommon_glColor4ub()

typedef struct OGLtExtension_Function
{
	char*	functionName;
	void**	functionLocation;	// can never be NULL
	UUtBool	required;
	
} OGLtExtension_Function;

typedef struct OGLtExtension_Feature
{
	char*		featureName;
	UUtBool*	featureFlag;	// if NULL the feature is required
	
} OGLtExtension_Feature;

OGLtCommon	OGLgCommon;

typedef enum OGLtDownloadType
{
	OGLcDownloadS3,
	OGLcDownloadPacked,
	OGLcDownloadClassic,
	OGLcDownloadGeneric,
	OGLcDownloadInvalid
	
} OGLtDownloadType;


typedef struct OGLtTexelTypeInfo
{
	IMtPixelType		texelType;

	OGLtDownloadType	downloadType;
	GLenum				downloadInfo;

	GLenum				glInternalFormat;
} OGLtTexelTypeInfo;

typedef void
(*OGLtTextureMap_Proc_Download)(
	const M3tTextureMap*		inTextureMap,
	const OGLtTexelTypeInfo*	inTextureInfo,
	void*						inSrc,
	GLint						inLevel,
	GLsizei						inWidth,
	GLsizei						inHeight,
	void*						inBuffer);

OGLtExtension_Feature	OGLgExtension_Features[] =
{
	// required
	
	// optional
	{"GL_ARB_multitexture",				&OGLgCommon.ext_arb_multitexture},
	{"GL_EXT_vertex_array",				&OGLgCommon.ext_vertex_array},
	{"GL_EXT_compiled_vertex_array",	&OGLgCommon.ext_compiled_vertex_array},
	{"GL_EXT_packed_pixels",			&OGLgCommon.ext_packed_pixels},
	{"GL_EXT_abgr",						&OGLgCommon.ext_abgr},
	{"GL_EXT_bgra",						&OGLgCommon.ext_bgra},
	{"GL_EXT_clip_volume_hint",			&OGLgCommon.ext_clip_volume_hint},
	{"GL_EXT_cull_vertex",				&OGLgCommon.ext_cull_vertex},
	{"GL_EXT_point_parameters",			&OGLgCommon.ext_point_parameters},
	{"GL_EXT_stencil_wrap",				&OGLgCommon.ext_stencil_wrap},
	{"GL_EXT_texture_object",			&OGLgCommon.ext_texture_object},
	{"GL_S3_s3tc",						&OGLgCommon.ext_s3tc}, 
	
	{NULL, NULL}

};

OGLtExtension_Function	OGLgExtension_Functions[] =
{
	
	// required
	
	// optional
	{"glActiveTextureARB",			(void**) &OGLgCommon.pglActiveTextureARB, 		UUcFalse},
	{"glClientActiveTextureARB",	(void**) &OGLgCommon.pglClientActiveTextureARB,	UUcFalse},
	{"glMultiTexCoord4fARB",		(void**) &OGLgCommon.pglMultiTexCoord4fARB,		UUcFalse},
	{"glMultiTexCoord2fvARB",		(void**) &OGLgCommon.pglMultiTexCoord2fvARB,	UUcFalse},
	{"glLockArraysEXT",				(void**) &OGLgCommon.pglLockArraysEXT,			UUcFalse},
	{"glUnlockArraysEXT",			(void**) &OGLgCommon.pglUnlockArraysEXT,		UUcFalse},
	
	{NULL, NULL}
};

const OGLtTexelTypeInfo OGLgTexInfoTable[] = 
{
{	IMcPixelType_ARGB4444,		OGLcDownloadPacked,		UNSIGNED_SHORT_4_4_4_4,		GL_RGBA4 },
{	IMcPixelType_RGB555,		OGLcDownloadPacked,		UNSIGNED_SHORT_5_5_5_1,		GL_RGB5 },
{	IMcPixelType_ARGB1555,		OGLcDownloadPacked,		UNSIGNED_SHORT_5_5_5_1,		GL_RGB5_A1 },
{	IMcPixelType_I8,			OGLcDownloadClassic,	GL_LUMINANCE,				GL_LUMINANCE8 },
{	IMcPixelType_I1,			OGLcDownloadInvalid,	0,							GL_LUMINANCE8 },
{	IMcPixelType_A8,			OGLcDownloadGeneric,	0,							GL_ALPHA8 },
{	IMcPixelType_A4I4,			OGLcDownloadGeneric,	0,							GL_LUMINANCE4_ALPHA4 },
{	IMcPixelType_ARGB8888,		OGLcDownloadGeneric,	0,							GL_RGBA },
{	IMcPixelType_RGB888,		OGLcDownloadGeneric,	0,							GL_RGB },
{	IMcPixelType_DXT1,			OGLcDownloadS3,			UNSIGNED_SHORT_5_5_5_1,		GL_RGB5_A1 },
{	IMcPixelType_RGB_Bytes,		OGLcDownloadClassic,	GL_RGB,						GL_RGB },
{	IMcPixelType_RGBA_Bytes,	OGLcDownloadClassic,	GL_RGBA,					GL_RGBA },
{	IMcPixelType_RGBA5551,		OGLcDownloadPacked,		UNSIGNED_SHORT_5_5_5_1,		GL_RGB5_A1 },
{	IMcPixelType_RGBA4444,		OGLcDownloadPacked,		UNSIGNED_SHORT_4_4_4_4,		GL_RGBA4 },
{	IMcPixelType_RGB565,		OGLcDownloadPacked,		UNSIGNED_SHORT_5_6_5,		GL_RGB5 },
};


static void
OGLiTextureMap_Download_Generic_NoAlpha(
	const M3tTextureMap*		inTextureMap,
	const OGLtTexelTypeInfo*	inTextureInfo,
	void*						inSrc,
	GLint						inLevel,
	GLsizei						inWidth,
	GLsizei						inHeight,
	void*						inBuffer)
{
	UUtError		error;
	IMtPixelType	newPixelType;
	GLenum			format;

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	if (IMrPixelType_HasAlpha(inTextureMap->texelType))
	{
		newPixelType = IMcPixelType_RGBA_Bytes;
		format= GL_RGBA;
	}
	else
	{
		newPixelType = IMcPixelType_RGB_Bytes;
		format= GL_RGB;
	}

	error = IMrImage_ConvertPixelType(
		IMcDitherMode_Off, 
		inWidth,
		inHeight,
		IMcNoMipMap,
		inTextureMap->texelType,
		inSrc,
		newPixelType,
		inBuffer);
	UUmAssert(UUcError_None == error);

	{
		UUtInt32	count = inTextureMap->width * inTextureMap->height;
		UUtInt32	loop;
		UUtUns8*	reset_buf = inBuffer;

		reset_buf += 3;

		for(loop = 0; loop < count; loop++)
		{
			*reset_buf = 0xff;
			reset_buf += 4;
		}
	}

	glTexImage2D(
		GL_TEXTURE_2D,
		inLevel,
		inTextureInfo->glInternalFormat,	// requested format to store on the card or 0 (GLenum)
		inWidth,							// width
		inHeight,							// height
		0,									// border {?} (GLint)
		format,								// format of the pixel data (GLenum)
		GL_UNSIGNED_BYTE,					// packed format (GLenum) 
		inBuffer);

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

static void
OGLiTextureMap_Download_Generic(
	const M3tTextureMap*		inTextureMap,
	const OGLtTexelTypeInfo*	inTextureInfo,
	void*						inSrc,
	GLint						inLevel,
	GLsizei						inWidth,
	GLsizei						inHeight,
	void*						inBuffer)
{
	UUtError		error;
	IMtPixelType	newPixelType;
	GLenum			format;

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);
	
	if (IMrPixelType_HasAlpha(inTextureMap->texelType))
	{
		newPixelType = IMcPixelType_RGBA_Bytes;
		format= GL_RGBA;
	}
	else
	{
		newPixelType = IMcPixelType_RGB_Bytes;
		format= GL_RGB;
	}

	error = IMrImage_ConvertPixelType(
		IMcDitherMode_Off, 
		inWidth,
		inHeight,
		IMcNoMipMap,
		inTextureMap->texelType,
		inSrc,
		newPixelType,
		inBuffer);
	UUmAssert(UUcError_None == error);

	glTexImage2D(
		GL_TEXTURE_2D,
		inLevel,
		inTextureInfo->glInternalFormat,	// requested format to store on the card or 0 (GLenum)
		inWidth,							// width
		inHeight,							// height
		0,									// border {?} (GLint)
		format,								// format of the pixel data (GLenum)
		GL_UNSIGNED_BYTE,					// packed format (GLenum) 
		inBuffer);

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

static void
OGLiTextureMap_Download_Classic(
	const M3tTextureMap*		inTextureMap,
	const OGLtTexelTypeInfo*	inTextureInfo,
	void*						inSrc,
	GLint						inLevel,
	GLsizei						inWidth,
	GLsizei						inHeight,
	void*						inBuffer)
{
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	glTexImage2D(
		GL_TEXTURE_2D,
		inLevel,
		inTextureInfo->glInternalFormat,
		inWidth,				
		inHeight,				
		0,						
		inTextureInfo->downloadInfo,	
		GL_UNSIGNED_BYTE,		
		inSrc);
	
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

static IMtPixelType
OGLiTextureMap_PackedTypeToPixelType(GLenum inPackedType)
{
	IMtPixelType result;

	switch(inPackedType)
	{
		case UNSIGNED_SHORT_4_4_4_4:
			result = IMcPixelType_RGBA4444;
		break;

		case UNSIGNED_SHORT_5_5_5_1:
			result = IMcPixelType_RGBA5551;
		break;

		default:
			UUmAssert(!"invalid case in GLrPackedTypeToPixelType");
	}

	return result;
}


static void
OGLiTextureMap_Download_S3(
	const M3tTextureMap*		inTextureMap,
	const OGLtTexelTypeInfo*	inTextureInfo,
	void*						inSrc,
	GLint						inLevel,
	GLsizei						inWidth,
	GLsizei						inHeight,
	void*						inBuffer)
{
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	glTexImage2D(
		GL_TEXTURE_2D,
		inLevel,
		GL_RGB4_S3TC,							// requested format to store on the card or 0 (GLenum)
		inWidth,							// width
		inHeight,							// height
		0,									// border {?} (GLint)
		GL_RGB4_S3TC,						// format of the pixel data (GLenum)
		inTextureInfo->downloadInfo,		// packed format (GLenum) 
		inBuffer);
	
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}


static void
OGLiTextureMap_Download_PackedPixels(
	const M3tTextureMap*		inTextureMap,
	const OGLtTexelTypeInfo*	inTextureInfo,
	void*						inSrc,
	GLint						inLevel,
	GLsizei						inWidth,
	GLsizei						inHeight,
	void*						inBuffer)
{
	UUtError error = UUcError_None;
	IMtPixelType pixelType;

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	pixelType = OGLiTextureMap_PackedTypeToPixelType(inTextureInfo->downloadInfo);

	error = IMrImage_ConvertPixelType(
		IMcDitherMode_Off, 
		inWidth,
		inHeight,
		IMcNoMipMap,
		inTextureMap->texelType,
		inSrc,
		pixelType,
		inBuffer);
	UUmAssert(UUcError_None == error);

	glTexImage2D(
		GL_TEXTURE_2D,
		inLevel,
		inTextureInfo->glInternalFormat,	// requested format to store on the card or 0 (GLenum)
		inWidth,							// width
		inHeight,							// height
		0,									// border {?} (GLint)
		GL_RGBA,							// format of the pixel data (GLenum)
		inTextureInfo->downloadInfo,		// packed format (GLenum) 
		inBuffer);
	
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

static UUtError
OGLiTextureMap_Create(
	M3tTextureMap*	inTextureMap)
{
	const OGLtTexelTypeInfo*		textureInfo;
	OGLtTextureMap_Proc_Download	downloadProc;
	UUtBool							hasMipMap = UUcFalse;
	UUtInt32						lod;
	UUtUns16						width;
	UUtUns16						height;
	void*							base;
	void*							buffer = OGLgCommon.convertedDataBuffer;

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);
	UUmAssert(inTextureMap->opengl_dirty == UUcTrue);
	
	inTextureMap->opengl_dirty = UUcFalse;
	
	downloadProc = OGLiTextureMap_Download_Generic;
	
	textureInfo = OGLgTexInfoTable + inTextureMap->texelType;
		
	switch(textureInfo->downloadType)
	{
		case OGLcDownloadS3:
			if(0 && OGLgCommon.ext_s3tc) {
				downloadProc = OGLiTextureMap_Download_S3;
			}
			else if(OGLgCommon.ext_packed_pixels) {
				downloadProc = OGLiTextureMap_Download_PackedPixels;
			}

			break;
		case OGLcDownloadPacked:
			if(OGLgCommon.ext_packed_pixels) {
				downloadProc = OGLiTextureMap_Download_PackedPixels;
			}
			break;

		case OGLcDownloadClassic:
			downloadProc = OGLiTextureMap_Download_Classic;
			break;

		case OGLcDownloadGeneric:
			break;

		default:
			UUmAssert(!"blam");
	}
	
	if(inTextureMap->flags & M3cTextureFlags_HasMipMap)
	{
		hasMipMap = UUcTrue;
	}
	
	// generate opengl name
	if(inTextureMap->opengl_texture_name == 0)
	{
		glGenTextures(1, (unsigned int*)&inTextureMap->opengl_texture_name);
		if(inTextureMap->opengl_texture_name == 0) return UUcError_Generic;
	}
	
	OGLrCommon_glBindTexture(GL_TEXTURE_2D, inTextureMap->opengl_texture_name);

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);
	//glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND); //GL_MODULATE, GL_DECAL

	if (inTextureMap->flags & M3cTextureFlags_ClampHoriz)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	}

	if (inTextureMap->flags & M3cTextureFlags_ClampVert)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);
		
	if (hasMipMap)
	{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
		UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);
	}

	lod = 0;
	width = inTextureMap->width;
	height = inTextureMap->height;
	base = inTextureMap->pixels;

	while((width > 0) && (height > 0))
	{
		downloadProc(inTextureMap, textureInfo, base, lod, width, height, buffer);

		if (!hasMipMap) break;

		lod++;
		base = ((char *) base) + IMrImage_ComputeSize(inTextureMap->texelType, IMcNoMipMap, width, height); 
		width >>= 1;
		height >>= 1;

		if ((width > 0) || (height > 0))
		{
			width = UUmMax(width, 1);
			height = UUmMax(height, 1);
		}
	}
	
	UUmAssert(glIsTexture(inTextureMap->opengl_texture_name) == GL_TRUE);
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return UUcError_None;
}

static void
OGLiTextureMap_Delete(
	M3tTextureMap*			inTextureMap)
{
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	if (inTextureMap->flags & M3cTextureFlags_Offscreen)
	{
		return;
	}

	if (0 != inTextureMap->opengl_texture_name)
	{
		glDeleteTextures(1, (unsigned int*)&inTextureMap->opengl_texture_name);
		inTextureMap->opengl_texture_name = 0;
	}

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

static UUtError
OGLiTextureMap_Update(
	M3tTextureMap*			inTextureMap)
{	
	inTextureMap->opengl_dirty = UUcTrue;
	
	return UUcError_None;
}

static UUtError
OGLiTextureMap_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void*					inInstancePtr,
	void*					inPrivateData)
{
	UUtError				error;
	M3tTextureMap*			inTextureMap = inInstancePtr;
	
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
			inTextureMap->flags |= M3cTextureFlags_Offscreen;
			inTextureMap->debugName[0] = '\0';

			inTextureMap->opengl_texture_name = 0;
			inTextureMap->opengl_dirty = UUcTrue;
			break;

		case TMcTemplateProcMessage_LoadPostProcess:
			M3rTextureMap_Prepare(inTextureMap);
			
			inTextureMap->opengl_texture_name = 0;
			inTextureMap->opengl_dirty = UUcTrue;
			break;
			
		case TMcTemplateProcMessage_DisposePreProcess:
			OGLiTextureMap_Delete(inTextureMap);
			break;
			
		case TMcTemplateProcMessage_Update:
			error = OGLiTextureMap_Update(inTextureMap);
			UUmError_ReturnOnError(error);
			break;
			
		case TMcTemplateProcMessage_PrepareForUse:
			break;
	}

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return UUcError_None;
}

static void
OGLiTexture_EnsureLoaded(
	M3tTextureMap*	inTextureMap)
{
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	if(inTextureMap->opengl_dirty)
	{
		OGLiTextureMap_Delete(inTextureMap);
		OGLiTextureMap_Create(inTextureMap);
		//COrConsole_Printf("made texture %d", inPrivateTextureData->texture_name);
	}
	
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

UUtError
OGLrCommon_Initialize_OneTime(
	void)
{
	UUtBool	present;

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	// S.S.
	memset(&OGLgCommon, 0, sizeof(OGLgCommon));

	OGLgCommon.vendor= (void *) glGetString(GL_VENDOR);
	OGLgCommon.renderer= (void *) glGetString(GL_RENDERER);
	OGLgCommon.version= (void *) glGetString(GL_VERSION);
	OGLgCommon.extensions= (void *) glGetString(GL_EXTENSIONS);

	UUrStartupMessage("opengl vendor = %s", OGLgCommon.vendor);
	UUrStartupMessage("opengl renderer = %s", OGLgCommon.renderer);
	UUrStartupMessage("opengl version = %s", OGLgCommon.version);
	UUrStartupMessage("opengl extensions = %s", OGLgCommon.extensions);
	
	// get the feature extensions
	{
		OGLtExtension_Feature*	curFeature = OGLgExtension_Features;
		
		while(curFeature->featureName != NULL)
		{
			present = strstr(OGLgCommon.extensions, curFeature->featureName) ? UUcTrue : UUcFalse;
			
			if(curFeature->featureFlag == NULL)
			{
				if(present == UUcFalse)
				{
					UUmError_ReturnOnErrorMsgP(UUcError_Generic, "opengl is missing extension %s", (UUtUns32)curFeature->featureName, 0, 0);
				}
			}
			else
			{
				*curFeature->featureFlag = present;
			}
			
			curFeature++;
		}
	}

	// get function extensions
	{
		OGLtExtension_Function*	curFunction = OGLgExtension_Functions;
		void*					functionAddress;
		
		while(curFunction->functionName != NULL)
		{
			functionAddress = (void*)OGLrCommon_Platform_GetFunction(curFunction->functionName);
			
			if(curFunction->required == UUcTrue)
			{
				if(functionAddress == NULL)
				{
					UUmError_ReturnOnErrorMsgP(UUcError_Generic, "opengl is missing function %s", (UUtUns32)curFunction->functionName, 0, 0);
				}
			}

			*curFunction->functionLocation = functionAddress;
			
			curFunction++;
		}
	}
	
	if(OGLgCommon.ext_compiled_vertex_array == UUcTrue)
	{
		OGLgCommon.ext_vertex_array = UUcTrue;
	}
	
/* S.S.	if(OGLgCommon.ext_vertex_array == UUcFalse)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "opengl is missing extension vertex arrays");
	}*/
	
	// verify compiled vertex arrays
	if ((NULL == OGLgCommon.pglLockArraysEXT) || (NULL == OGLgCommon.pglUnlockArraysEXT))
	{
		OGLgCommon.ext_compiled_vertex_array = UUcFalse;

		OGLgCommon.pglLockArraysEXT = NULL;
		OGLgCommon.pglUnlockArraysEXT = NULL;
	}

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return UUcError_None;
}

UUtError
OGLrCommon_State_Initialize(
	UUtUns16	inWidth,
	UUtUns16	inHeight)
{
	UUtError	error;

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	OGLgCommon.width = inWidth;
	OGLgCommon.height = inHeight;
	
	OGLrCommon_glEnableClientState(GL_VERTEX_ARRAY);
//	OGLrCommon_glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//	OGLrCommon_glEnableClientState(GL_NORMAL_ARRAY);
	
	OGLrCommon_glDisableClientState(GL_COLOR_ARRAY);
	OGLrCommon_glDisableClientState(GL_INDEX_ARRAY);
	OGLrCommon_glDisableClientState(GL_EDGE_FLAG_ARRAY);

	//glDepthRange(0,1);
	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glLoadIdentity();

	OGLrCommon_glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);

	glViewport(0, 0, OGLgCommon.width, OGLgCommon.height);
	
	OGLgCommon.convertedDataBuffer = UUrMemory_Block_New(256 * 256 * 4);	
	UUmError_ReturnOnNull(OGLgCommon.convertedDataBuffer);

	// force update of the camera
		OGLgCommon.cameraMode = OGLcCameraMode_3D;
		OGLrCommon_Camera_Update(OGLcCameraMode_2D);
	
	// create the texture private array
		error = 
			TMrTemplate_PrivateData_New(
				M3cTemplate_TextureMap,
				0,
				OGLiTextureMap_ProcHandler,
				&OGLgCommon.texturePrivateData);
		UUmError_ReturnOnError(error);
	
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return UUcError_None;
}

void
OGLrCommon_State_Terminate(
	void)
{
	TMrTemplate_PrivateData_Delete(OGLgCommon.texturePrivateData);
	UUrMemory_Block_Delete(OGLgCommon.convertedDataBuffer);
}

void
OGLrCommon_Camera_Update(
	OGLtCameraMode	inCameraMode)
{
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);
	
	if(inCameraMode == OGLcCameraMode_2D && OGLgCommon.cameraMode == OGLcCameraMode_2D) return;
	
	if(inCameraMode == OGLcCameraMode_3D)
	{
		M3tManager_GeomCamera*	activeCamera;
		
		M3rCamera_GetActive((M3tGeomCamera**)&activeCamera);
		
		M3rManager_Camera_UpdateMatrices(activeCamera);
		
		if(OGLgCommon.cameraMode != OGLcCameraMode_3D)
		{
			OGLgCommon.cameraMode = inCameraMode;
			OGLgCommon.update3DCamera_ViewData = UUcTrue;
			OGLgCommon.update3DCamera_StaticData = UUcTrue;
		}
		
		if(OGLgCommon.update3DCamera_ViewData)
		{
			OGLgCommon.update3DCamera_ViewData = UUcFalse;
			
			glLoadIdentity();
			
			#if 0
				gluLookAt(
					activeCamera->cameraLocation.x,
					activeCamera->cameraLocation.y,
					activeCamera->cameraLocation.z,
					activeCamera->cameraLocation.x + activeCamera->viewVector.x,
					activeCamera->cameraLocation.y + activeCamera->viewVector.y,
					activeCamera->cameraLocation.z + activeCamera->viewVector.z,
					activeCamera->upVector.x,
					activeCamera->upVector.y,
					activeCamera->upVector.z);
			#else
			{
				float	m[16];
				
				OGLrCommon_Matrix4x4_OniToGL(
					&activeCamera->matrix_worldToView,
					m);
				
				glMultMatrixf(m);
				
			}
			#endif
		}
		
		if(OGLgCommon.update3DCamera_StaticData)
		{
			OGLgCommon.update3DCamera_StaticData = UUcFalse;
		
			glMatrixMode(GL_PROJECTION);
			
			glLoadIdentity();
			
			#if 0
			gluPerspective(
				activeCamera->fovy * 180.0f / M3cPi,
				activeCamera->aspect,
				activeCamera->zNear,
				activeCamera->zFar);
			#else
			{
				float	m[16];
				
				OGLrCommon_Matrix4x4_OniToGL(
					&activeCamera->matrix_viewToFrustum,
					m);
				
				glMultMatrixf(m);
				
			}
			#endif
			
			glMatrixMode(GL_MODELVIEW);
		}
	}
	else
	{
		// set the ogl camera to 2d mode
		OGLgCommon.cameraMode = OGLcCameraMode_2D;
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(
			0.f, OGLgCommon.width,
			OGLgCommon.height, 0.f,
			0.f, -1.f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
	}

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

void
OGLrCommon_Matrix4x3_OniToGL(
	M3tMatrix4x3*	inMatrix,
	float			*outGLMatrix)
{
	float*	m = outGLMatrix;
	
	*m++ = inMatrix->m[0][0];
	*m++ = inMatrix->m[0][1];
	*m++ = inMatrix->m[0][2];
	*m++ = 0.0f;
	
	*m++ = inMatrix->m[1][0];
	*m++ = inMatrix->m[1][1];
	*m++ = inMatrix->m[1][2];
	*m++ = 0.0f;
	
	*m++ = inMatrix->m[2][0];
	*m++ = inMatrix->m[2][1];
	*m++ = inMatrix->m[2][2];
	*m++ = 0.0f;
	
	*m++ = inMatrix->m[3][0];
	*m++ = inMatrix->m[3][1];
	*m++ = inMatrix->m[3][2];
	*m++ = 1.0f;
	
	return;
}

void
OGLrCommon_Matrix4x4_OniToGL(
	M3tMatrix4x4*	inMatrix,
	float			*outGLMatrix)
{
	float*	m = outGLMatrix;
	
	*m++ = inMatrix->m[0][0];
	*m++ = inMatrix->m[0][1];
	*m++ = inMatrix->m[0][2];
	*m++ = inMatrix->m[0][3];
	
	*m++ = inMatrix->m[1][0];
	*m++ = inMatrix->m[1][1];
	*m++ = inMatrix->m[1][2];
	*m++ = inMatrix->m[1][3];
	
	*m++ = inMatrix->m[2][0];
	*m++ = inMatrix->m[2][1];
	*m++ = inMatrix->m[2][2];
	*m++ = inMatrix->m[2][3];
	
	*m++ = inMatrix->m[3][0];
	*m++ = inMatrix->m[3][1];
	*m++ = inMatrix->m[3][2];
	*m++ = inMatrix->m[3][3];
	
	return;
}

void
OGLrCommon_TextureMap_Select(
	M3tTextureMap*	inTextureMap,
	GLenum					inTMU)
{
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	if (OGLgCommon.ext_arb_multitexture)
	{
		OGLgCommon.pglActiveTextureARB(inTMU);
		OGLgCommon.pglClientActiveTextureARB(inTMU);
	}
	else if (inTMU != GL_TEXTURE0_ARB)
	{
		return;
	}

	if (NULL == inTextureMap)
	{
		OGLrCommon_glBindTexture(GL_TEXTURE_2D, 0);
		OGLrCommon_glDisable(GL_TEXTURE_2D);
		
		return;
	}

	if(inTMU == GL_TEXTURE0_ARB)
	{
		if(inTextureMap->flags & M3cTextureFlags_Blend_Additive)
		{
			OGLrCommon_glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}
		else
		{
			OGLrCommon_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}
		
	OGLiTexture_EnsureLoaded(inTextureMap);
	
	glEnable(GL_TEXTURE_2D);
	
	UUmAssert(glIsTexture(inTextureMap->opengl_texture_name) == GL_TRUE);
		
	OGLrCommon_glBindTexture(GL_TEXTURE_2D, inTextureMap->opengl_texture_name);
	
	#if 0
	if (GL_TEXTURE1_ARB == inTMU)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	}
	#endif

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

UUtBool
OGLrCommon_TextureMap_TextureFormatAvailable(
	IMtPixelType		inTexelType)
{
	if (OGLcDownloadInvalid == OGLgTexInfoTable[inTexelType].downloadInfo)
	{
		return UUcFalse;
	}

	return UUcTrue;
}

void
OGLrCommon_DepthBias_Set(
	float inBias )
{
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	OGLrCommon_glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset( 1.0, inBias );

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

void
OGLrCommon_DepthMode_Set(
	UUtBool inRead,
	UUtBool inWrite)
{
	static int depth_enabled = UUcFalse;
	static int depth_read = UUcFalse;
	static int depth_write = UUcFalse;

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	//inRead = UUcTrue;
	//inWrite = UUcTrue;
	
	if (inRead || inWrite)
	{
		if (!depth_enabled)
		{
			depth_enabled = UUcTrue;
			/* when re-enabling, force updates */
			depth_read = !inRead;
			depth_write = !inWrite;
		}
		
		if ((inRead && !depth_read) ||
			(!inRead && depth_read))
		{
			OGLrCommon_glEnable(GL_DEPTH_TEST);
			glDepthFunc(inRead ? GL_LEQUAL : GL_ALWAYS);
			depth_read = inRead;
		}
		
		if ((inWrite && !depth_write) ||
			(!inWrite && depth_write))
		{
			glDepthMask(inWrite ? GL_TRUE : GL_FALSE);
			depth_write = inWrite;
		}
	}
	else
	{
		if (depth_enabled)
		{
			OGLrCommon_glDisable(GL_DEPTH_TEST);
			glDepthFunc(GL_ALWAYS);
			glDepthMask(GL_FALSE);
			//pgl
			//pglDepthMask(GL_FALSE);
			//pglDepthFunc(GL_ALWAYS);
			depth_enabled= UUcFalse;
			depth_read= UUcFalse;
			depth_write= UUcFalse;
		}
	}

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

// S.S.

void OGLrCommon_glFogEnable(
	void)
{
	#include "Oni_Motoko.h" // for near & far plane

	static int first_time= 1;
	static float fog_start= -1.f;
	static float fog_end= -1.f;
	static float fog_density= -1.f;

	if (first_time)
	{
		OGLgCommon.fog_color[0]= 0.f;
		OGLgCommon.fog_color[1]= 0.f;
		OGLgCommon.fog_color[2]= 0.f;
		OGLgCommon.fog_color[3]= 1.f;
		OGLgCommon.fog_density= 0.5f;
		OGLgCommon.fog_mode= GL_EXP2;
	
//		OGLrCommon_glEnable(GL_FOG);
	
		glFogi(GL_FOG_MODE, OGLgCommon.fog_mode);
//		glHint(GL_FOG_HINT, GL_DONT_CARE);

		first_time= 0;
	}

	OGLgCommon.fog_start= ONcMotoko_NearPlane;
	OGLgCommon.fog_end= 100.f; //0.75f * ONgMotoko_FarPlane;

	if (OGLgCommon.fog_density != fog_density)
	{
		glFogf(GL_FOG_DENSITY, OGLgCommon.fog_density);
		fog_density= OGLgCommon.fog_density;
	}
	if (OGLgCommon.fog_start != fog_start)
	{
		glFogf(GL_FOG_START, OGLgCommon.fog_start);
		fog_start= OGLgCommon.fog_start;
	}
	if (OGLgCommon.fog_end != fog_end)
	{
		glFogf(GL_FOG_END, OGLgCommon.fog_end);
		fog_end= OGLgCommon.fog_end;
	}

	glFogfv(GL_FOG_COLOR, OGLgCommon.fog_color);

	glClearColor(OGLgCommon.fog_color[0], OGLgCommon.fog_color[1],
		OGLgCommon.fog_color[2], OGLgCommon.fog_color[3]);
	
	return;
}

// S.S. - OpenGL wrappers

void OGLrCommon_glFlush(
	void)
{
	glFlush();

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

void OGLrCommon_glDrawElements(
	GLenum mode,
	GLsizei count,
	GLenum type,
	const GLvoid *indices)
{
	glDrawElements(mode, count, type, indices);

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

void OGLrCommon_glBindTexture(
	GLenum target,
	GLuint texture)
{
	static GLuint current_texture= 0;

	UUmAssert(target == GL_TEXTURE_2D);
	if (texture != current_texture)
	{
		current_texture= texture;
		glBindTexture(GL_TEXTURE_2D, texture);
	}
	/*else
	{
		int a= 5;
	}*/

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

void OGLrCommon_glBlendFunc(
	GLenum src_factor,
	GLenum dst_factor)
{
	glBlendFunc(src_factor, dst_factor);

	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

void OGLrCommon_glEnable(
	GLenum cap)
{
	glEnable(cap);
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

void OGLrCommon_glDisable(
	GLenum cap)
{
	glDisable(cap);
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

void OGLrCommon_glEnableClientState(
	GLenum array)
{
	glEnableClientState(array);
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}

void OGLrCommon_glDisableClientState(
	GLenum array)
{
	glDisableClientState(array);
	UUmAssert((gl_error= glGetError()) == GL_NO_ERROR);

	return;
}



#endif // __ONADA__
