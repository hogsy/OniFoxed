/*
	FILE:	GL_DrawContext_Private.h

	AUTHOR:	Michael Evans

	CREATED: August 10, 1999

	PURPOSE:

	Copyright 1999

*/
#ifndef GL_DRAWCONTEXT_PRIVATE_H
#define GL_DRAWCONTEXT_PRIVATE_H

#include "BFW.h"
#include "BFW_Motoko.h"

typedef struct GLtDrawContextPrivate GLtDrawContextPrivate;
typedef struct GLtTextureMapPrivate GLtTextureMapPrivate;
typedef struct GLtStatePrivate GLtStatePrivate;
typedef struct GLtVertex GLtVertex;

struct GLtVertex
{
	float pos[3];					// screenX, screenY, 0 <= z <= 1
	//float texcoord[4];				// u*oow, v*oow, 0.f, oow, tex coords still use
	// this format, they're just no longer stored within this struct
	//float color[4];					// r, g, b, a	now stored in color arrays
	float oow;
};

struct GLtDrawContextPrivate
{
	UUtInt32				width;
	UUtInt32				height;
	UUtInt32				bitDepth;

	M3tDrawContextType		contextType;

	GLtVertex*				vertexList;
	UUtUns32				vertexCount;

	M3tTextureMap*	curBaseMap;
	M3tTextureMap*	curLightMap;
	M3tTextureMap*	curEnvironmentMap;

	const UUtInt32*			stateInt;
	const void**			statePtr;

	float*					compiled_vertex_array;
	UUtBool					compiled_vertex_array_locked;

	float		constant_a;
	float		constant_r;
	float		constant_g;
	float		constant_b;

	GLint		max_texture_size;
	GLint		has_double_buffer;
	GLint		num_tmu;
};

extern GLtDrawContextPrivate	*GLgDrawContextPrivate;
extern UUtBool GLgBufferClear;
extern UUtBool GLgDoubleBuffer;
extern UUtUns32 GLgClearColor;

extern UUtBool GLgMultiTexture;
extern UUtBool GLgVertexArray;

#endif /* GL_DRAWCONTEXT_PRIVATE_H */
