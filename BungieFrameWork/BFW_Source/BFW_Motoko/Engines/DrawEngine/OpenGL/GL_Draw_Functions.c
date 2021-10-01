/*
	FILE:	GL_DC_Method_Frame.h
	
	AUTHOR:	Michael Evans
	
	CREATED: August 10, 1999
	
	PURPOSE: 
	
	Copyright 1999

*/

#ifndef __ONADA__

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"

#include "OGL_DrawGeom_Common.h"

#include "GL_Draw_Functions.h"
#include "GL_DC_Private.h"

extern GLint gl_error;

#if 1
#define FLUSH glFlush();
#else
#define FLUSH 
#endif

void
GLrDrawContext_Method_TriSprite(
	const M3tPointScreen	*inPoints,				// points[3]
	const M3tTextureCoord	*inTextureCoords)		// UVs[3]
{
	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r,
		GLgDrawContextPrivate->constant_g,
		GLgDrawContextPrivate->constant_b,
		GLgDrawContextPrivate->constant_a);
	#endif
	
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBegin(GL_TRIANGLES);
		// point[0]
		glTexCoord2f(inTextureCoords[0].u, inTextureCoords[0].v);
		glVertex3f(inPoints[0].x, inPoints[0].y, inPoints[0].z);
		
		// point[1]
		glTexCoord2f(inTextureCoords[1].u, inTextureCoords[1].v);
		glVertex3f(inPoints[1].x, inPoints[1].y, inPoints[0].z);

		// point[2]
		glTexCoord2f(inTextureCoords[2].u, inTextureCoords[2].v);
		glVertex3f(inPoints[2].x, inPoints[2].y, inPoints[0].z);

	glEnd();

	FLUSH

	return;
}


void
GLrDrawContext_Method_Sprite(
	const M3tPointScreen	*inPoints,				// tl, br
	const M3tTextureCoord	*inTextureCoords)		// tl, tr, bl, br
{
	float z;
	float w;

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	z = inPoints[0].z;
	w = inPoints[0].invW;
	
	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r,
		GLgDrawContextPrivate->constant_g,
		GLgDrawContextPrivate->constant_b,
		GLgDrawContextPrivate->constant_a);
	#endif
	
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBegin(GL_QUADS);
		// tl
		glTexCoord2f(inTextureCoords[0].u, inTextureCoords[0].v);
		glVertex3f(inPoints[0].x, inPoints[0].y, z);
		
		// tr
		glTexCoord2f(inTextureCoords[1].u, inTextureCoords[1].v);
		glVertex3f(inPoints[1].x, inPoints[0].y, z);

		// br
		glTexCoord2f(inTextureCoords[3].u, inTextureCoords[3].v);
		glVertex3f(inPoints[1].x, inPoints[1].y, z);

		// bl
		glTexCoord2f(inTextureCoords[2].u, inTextureCoords[2].v);
		glVertex3f(inPoints[0].x, inPoints[1].y, z);

	glEnd();

	FLUSH

	return;
}


void
GLrDrawContext_Method_SpriteArray(
	const M3tPointScreen	*inPoints,				// tl, br
	const M3tTextureCoord	*inTextureCoords,		// tl, tr, bl, br
	const UUtUns32			*inColors,
	const UUtUns32			inCount )
{
	float		z;
	float		w;
	UUtUns32	i;
	UUtUns32	color;
	
	color = (UUtUns32)GLgDrawContextPrivate->statePtr[M3cDrawStateIntType_ConstantColor];

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	glBegin(GL_QUADS);
	
	i = 0;

	for( i = 0; i < inCount; i++ )
	{
		z = inPoints[0].z;
		w = inPoints[0].invW;
	
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		if( color != inColors[i] )
		{
			color = inColors[i];
			{
			UUtUns8		a = (UUtUns8) ((color & 0xff000000) >> 24);
			UUtUns8		r = (UUtUns8) ((color & 0x00ff0000) >> 16);
			UUtUns8		g = (UUtUns8) ((color & 0x0000ff00) >> 8);
			UUtUns8		b = (UUtUns8) ((color & 0x000000ff) >> 0);

			//GLgDrawContextPrivate->constant_a = a * (1.f / 255.f);
			GLgDrawContextPrivate->constant_a = a * (1.f / 255.f);
			GLgDrawContextPrivate->constant_r = r * (1.f / 255.f);
			GLgDrawContextPrivate->constant_g = g * (1.f / 255.f);
			GLgDrawContextPrivate->constant_b = b * (1.f / 255.f);

			glColor4f( GLgDrawContextPrivate->constant_r, GLgDrawContextPrivate->constant_g, GLgDrawContextPrivate->constant_b, GLgDrawContextPrivate->constant_a );
			}
		}

		// tl
		glTexCoord2f(inTextureCoords[i*4+0].u, inTextureCoords[i*4+0].v);
		glVertex3f(inPoints[i*2+0].x, inPoints[i*2+0].y, z);
		
		// tr
		glTexCoord2f(inTextureCoords[i*4+1].u, inTextureCoords[i*4+1].v);
		glVertex3f(inPoints[i*2+1].x, inPoints[i*2+0].y, z);

		// br
		glTexCoord2f(inTextureCoords[i*4+3].u, inTextureCoords[i*4+3].v);
		glVertex3f(inPoints[i*2+1].x, inPoints[i*2+1].y, z);

		// bl
		glTexCoord2f(inTextureCoords[i*4+2].u, inTextureCoords[i*4+2].v);
		glVertex3f(inPoints[i*2+0].x, inPoints[i*2+1].y, z);

	}
	glEnd();

	FLUSH

	return;
}


UUtError GLrDrawContext_Method_ScreenCapture(
	const UUtRect			*inRect, 
	void					*outBuffer)
{
	GLenum glError;
	GLint x = inRect->left;
	GLint y = inRect->top;
	GLsizei width = inRect->right - inRect->left + 1;
	GLsizei height = inRect->bottom - inRect->top + 1;

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	glReadBuffer(GL_FRONT);
	OGLmAssertSuccess(glError);

	glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, outBuffer);
	OGLmAssertSuccess(glError);
 
	return UUcError_None;
}


UUtBool GLrDrawContext_Method_PointVisible(
	const M3tPointScreen	*inPoint,
	float					inTolerance)
{
	GLenum glError;
	GLfloat depth_val;
	GLint x = MUrUnsignedSmallFloat_To_Uns_Round(inPoint->x);
	GLint y = OGLgCommon.height - MUrUnsignedSmallFloat_To_Uns_Round(inPoint->y);

	glReadBuffer(GL_BACK);
	OGLmAssertSuccess(glError);

	glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, (void *) &depth_val);
	OGLmAssertSuccess(glError);

	// visible if the point isn't behind the distance stord in the depth buffer
	return (depth_val > inPoint->z);
}


void GLrLine(
	UUtUns16		inVIndex0,
	UUtUns16		inVIndex1)
{
	const M3tPointScreen*	screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const M3tPointScreen*	ourScreenPoints[2];

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inVIndex0;
	ourScreenPoints[1] = screenPoints + inVIndex1;

	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r, 
		GLgDrawContextPrivate->constant_g, 
		GLgDrawContextPrivate->constant_b, 
		GLgDrawContextPrivate->constant_a);
	#endif
	
	glBegin(GL_LINES);
		glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);
		glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);
	glEnd();

	FLUSH

	return;
}

#define GLmR(x) ((UUtUns8) (((x) & 0xff0000) >> 16))
#define GLmG(x) ((UUtUns8) (((x) & 0xff00) >> 8))
#define GLmB(x) ((UUtUns8) (((x) & 0xff) >> 0))

void GLrTriangle(M3tTri *inTri)
{
	UUtBool clipped;
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const UUtUns32*			vertexShades = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenShadeArray_DC];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[3];
	UUtUns32		ourShades[3];
	const M3tTextureCoord*	ourTextureCoords[3];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inTri->indices[0];
	ourScreenPoints[1] = screenPoints + inTri->indices[1];
	ourScreenPoints[2] = screenPoints + inTri->indices[2];

	ourShades[0] = vertexShades[inTri->indices[0]];
	ourShades[1] = vertexShades[inTri->indices[1]];
	ourShades[2] = vertexShades[inTri->indices[2]];

	ourTextureCoords[0] = textureCoords + inTri->indices[0];
	ourTextureCoords[1] = textureCoords + inTri->indices[1];
	ourTextureCoords[2] = textureCoords + inTri->indices[2];

	clipped = UUmMax3(inTri->indices[0], inTri->indices[1], inTri->indices[2]) > GLgDrawContextPrivate->vertexCount;
	clipped = GLgVertexArray ? clipped : UUcTrue;
	
	if (clipped) {
		glBegin(GL_TRIANGLES);
			glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

			glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

			glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);
		glEnd();
	}
	else {
		glBegin(GL_TRIANGLES);
			glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glArrayElement(inTri->indices[0]);

			glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glArrayElement(inTri->indices[1]);

			glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glArrayElement(inTri->indices[2]);
		glEnd();
	}
	

	FLUSH

	return;
}

void GLrQuad(M3tQuad *inQuad)
{
	UUtBool clipped;
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const UUtUns32*			vertexShades = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenShadeArray_DC];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[4];
	UUtUns32		ourShades[4];
	const M3tTextureCoord*	ourTextureCoords[4];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inQuad->indices[0];
	ourScreenPoints[1] = screenPoints + inQuad->indices[1];
	ourScreenPoints[2] = screenPoints + inQuad->indices[2];
	ourScreenPoints[3] = screenPoints + inQuad->indices[3];

	ourShades[0] = vertexShades[inQuad->indices[0]];
	ourShades[1] = vertexShades[inQuad->indices[1]];
	ourShades[2] = vertexShades[inQuad->indices[2]];
	ourShades[3] = vertexShades[inQuad->indices[3]];

	ourTextureCoords[0] = textureCoords + inQuad->indices[0];
	ourTextureCoords[1] = textureCoords + inQuad->indices[1];
	ourTextureCoords[2] = textureCoords + inQuad->indices[2];
	ourTextureCoords[3] = textureCoords + inQuad->indices[3];
			
	clipped = UUmMax4(inQuad->indices[0], inQuad->indices[1], inQuad->indices[2], inQuad->indices[3]) > GLgDrawContextPrivate->vertexCount;
	clipped = GLgVertexArray ? clipped : UUcTrue;

	if (clipped) {
		glBegin(GL_QUADS);
			glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

			glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

			glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);

			glColor4ub(GLmR(ourShades[3]), GLmG(ourShades[3]), GLmB(ourShades[3]), alpha);
			glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
			glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y, ourScreenPoints[3]->z);
		glEnd();
	}
	else {
		glBegin(GL_QUADS);
			glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glArrayElement(inQuad->indices[0]);

			glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glArrayElement(inQuad->indices[1]);

			glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glArrayElement(inQuad->indices[2]);

			glColor4ub(GLmR(ourShades[3]), GLmG(ourShades[3]), GLmB(ourShades[3]), alpha);
			glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
			glArrayElement(inQuad->indices[3]);
		glEnd();
	}


	FLUSH

	return;
}



void GLrPent(M3tPent *inPent)
{
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const UUtUns32*			vertexShades = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenShadeArray_DC];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[5];
	UUtUns32		ourShades[5];
	const M3tTextureCoord*	ourTextureCoords[5];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inPent->indices[0];
	ourScreenPoints[1] = screenPoints + inPent->indices[1];
	ourScreenPoints[2] = screenPoints + inPent->indices[2];
	ourScreenPoints[3] = screenPoints + inPent->indices[3];
	ourScreenPoints[4] = screenPoints + inPent->indices[4];

	ourShades[0] = vertexShades[inPent->indices[0]];
	ourShades[1] = vertexShades[inPent->indices[1]];
	ourShades[2] = vertexShades[inPent->indices[2]];
	ourShades[3] = vertexShades[inPent->indices[3]];
	ourShades[4] = vertexShades[inPent->indices[4]];

	ourTextureCoords[0] = textureCoords + inPent->indices[0];
	ourTextureCoords[1] = textureCoords + inPent->indices[1];
	ourTextureCoords[2] = textureCoords + inPent->indices[2];
	ourTextureCoords[3] = textureCoords + inPent->indices[3];
	ourTextureCoords[4] = textureCoords + inPent->indices[4];
			
	glBegin(GL_POLYGON);
		glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
		glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
		glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

		glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
		glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
		glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

		glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
		glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
		glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);

		glColor4ub(GLmR(ourShades[3]), GLmG(ourShades[3]), GLmB(ourShades[3]), alpha);
		glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
		glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y, ourScreenPoints[3]->z);

		glColor4ub(GLmR(ourShades[4]), GLmG(ourShades[4]), GLmB(ourShades[4]), alpha);
		glTexCoord4f(ourTextureCoords[4]->u * ourScreenPoints[4]->invW, ourTextureCoords[4]->v * ourScreenPoints[4]->invW, 0.f, ourScreenPoints[4]->invW);
		glVertex3f(ourScreenPoints[4]->x, ourScreenPoints[4]->y, ourScreenPoints[4]->z);
	glEnd();


	FLUSH

	return;
}



void GLrPoint(M3tPointScreen*	invCoord)
{


	FLUSH

	return;
}


void GLrTriangle_Flat(M3tTri *inTri)
{
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[5];
	const M3tTextureCoord*	ourTextureCoords[5];

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inTri->indices[0];
	ourScreenPoints[1] = screenPoints + inTri->indices[1];
	ourScreenPoints[2] = screenPoints + inTri->indices[2];

	ourTextureCoords[0] = textureCoords + inTri->indices[0];
	ourTextureCoords[1] = textureCoords + inTri->indices[1];
	ourTextureCoords[2] = textureCoords + inTri->indices[2];

	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r,
		GLgDrawContextPrivate->constant_g,
		GLgDrawContextPrivate->constant_b,
		GLgDrawContextPrivate->constant_a);
	#endif
	
	glBegin(GL_TRIANGLES);
		glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
		glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

		glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
		glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

		glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
		glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);
	glEnd();


	FLUSH

	return;
}

void GLrQuad_Flat(M3tQuad *inQuad)
{
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[5];
	const M3tTextureCoord*	ourTextureCoords[5];

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inQuad->indices[0];
	ourScreenPoints[1] = screenPoints + inQuad->indices[1];
	ourScreenPoints[2] = screenPoints + inQuad->indices[2];
	ourScreenPoints[3] = screenPoints + inQuad->indices[3];

	ourTextureCoords[0] = textureCoords + inQuad->indices[0];
	ourTextureCoords[1] = textureCoords + inQuad->indices[1];
	ourTextureCoords[2] = textureCoords + inQuad->indices[2];
	ourTextureCoords[3] = textureCoords + inQuad->indices[3];
	
	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r,
		GLgDrawContextPrivate->constant_g,
		GLgDrawContextPrivate->constant_b,
		GLgDrawContextPrivate->constant_a);
	#endif
	
	glBegin(GL_QUADS);
		glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
		glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

		glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
		glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

		glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
		glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);

		glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
		glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y, ourScreenPoints[3]->z);
	glEnd();


	FLUSH

	return;
}

void GLrPent_Flat(M3tPent *inPent)
{
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[5];
	const M3tTextureCoord*	ourTextureCoords[5];

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inPent->indices[0];
	ourScreenPoints[1] = screenPoints + inPent->indices[1];
	ourScreenPoints[2] = screenPoints + inPent->indices[2];
	ourScreenPoints[3] = screenPoints + inPent->indices[3];
	ourScreenPoints[4] = screenPoints + inPent->indices[4];

	ourTextureCoords[0] = textureCoords + inPent->indices[0];
	ourTextureCoords[1] = textureCoords + inPent->indices[1];
	ourTextureCoords[2] = textureCoords + inPent->indices[2];
	ourTextureCoords[3] = textureCoords + inPent->indices[3];
	ourTextureCoords[4] = textureCoords + inPent->indices[4];
	
	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r,
		GLgDrawContextPrivate->constant_g,
		GLgDrawContextPrivate->constant_b,
		GLgDrawContextPrivate->constant_a);
	#endif
	
	glBegin(GL_POLYGON);
		glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
		glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

		glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
		glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

		glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
		glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);

		glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
		glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y, ourScreenPoints[3]->z);

		glTexCoord4f(ourTextureCoords[4]->u * ourScreenPoints[4]->invW, ourTextureCoords[4]->v * ourScreenPoints[4]->invW, 0.f, ourScreenPoints[4]->invW);
		glVertex3f(ourScreenPoints[4]->x, ourScreenPoints[4]->y, ourScreenPoints[4]->z);
	glEnd();


	FLUSH

	return;
}


void GLrTriangle_Gourand(M3tTri *inTri)
{
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const UUtUns32*			vertexShades = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenShadeArray_DC];
	const M3tPointScreen*	ourScreenPoints[3];
	UUtUns32		ourShades[3];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inTri->indices[0];
	ourScreenPoints[1] = screenPoints + inTri->indices[1];
	ourScreenPoints[2] = screenPoints + inTri->indices[2];

	ourShades[0] = vertexShades[inTri->indices[0]];
	ourShades[1] = vertexShades[inTri->indices[1]];
	ourShades[2] = vertexShades[inTri->indices[2]];
			
	glBegin(GL_TRIANGLES);
		glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
		glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

		glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
		glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

		glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
		glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);
	glEnd();


	FLUSH

	return;
}

void GLrQuad_Gourand(M3tQuad *inQuad)
{
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const UUtUns32*			vertexShades = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenShadeArray_DC];
	const M3tPointScreen*	ourScreenPoints[4];
	UUtUns32		ourShades[4];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inQuad->indices[0];
	ourScreenPoints[1] = screenPoints + inQuad->indices[1];
	ourScreenPoints[2] = screenPoints + inQuad->indices[2];
	ourScreenPoints[3] = screenPoints + inQuad->indices[3];

	ourShades[0] = vertexShades[inQuad->indices[0]];
	ourShades[1] = vertexShades[inQuad->indices[1]];
	ourShades[2] = vertexShades[inQuad->indices[2]];
	ourShades[3] = vertexShades[inQuad->indices[3]];
	
	glBegin(GL_QUADS);
		glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
		glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

		glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
		glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

		glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
		glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);

		glColor4ub(GLmR(ourShades[3]), GLmG(ourShades[3]), GLmB(ourShades[3]), alpha);
		glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y, ourScreenPoints[3]->z);
	glEnd();


	FLUSH

	return;
}

void GLrPent_Gourand(M3tPent *inPent)
{
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const UUtUns32*			vertexShades = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenShadeArray_DC];
	const M3tPointScreen*	ourScreenPoints[5];
	UUtUns32		ourShades[5];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inPent->indices[0];
	ourScreenPoints[1] = screenPoints + inPent->indices[1];
	ourScreenPoints[2] = screenPoints + inPent->indices[2];
	ourScreenPoints[3] = screenPoints + inPent->indices[3];
	ourScreenPoints[4] = screenPoints + inPent->indices[4];

	ourShades[0] = vertexShades[inPent->indices[0]];
	ourShades[1] = vertexShades[inPent->indices[1]];
	ourShades[2] = vertexShades[inPent->indices[2]];
	ourShades[3] = vertexShades[inPent->indices[3]];
	ourShades[4] = vertexShades[inPent->indices[4]];
			
	glBegin(GL_POLYGON);
		glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
		glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

		glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
		glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

		glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
		glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);

		glColor4ub(GLmR(ourShades[3]), GLmG(ourShades[3]), GLmB(ourShades[3]), alpha);
		glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y, ourScreenPoints[3]->z);

		glColor4ub(GLmR(ourShades[4]), GLmG(ourShades[4]), GLmB(ourShades[4]), alpha);
		glVertex3f(ourScreenPoints[4]->x, ourScreenPoints[4]->y, ourScreenPoints[4]->z);
	glEnd();


	FLUSH

	return;
}

void GLrTriangle_Wireframe(M3tTri *inTri)
{
	UUtBool clipped;
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const M3tPointScreen*	ourScreenPoints[3];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inTri->indices[0];
	ourScreenPoints[1] = screenPoints + inTri->indices[1];
	ourScreenPoints[2] = screenPoints + inTri->indices[2];
	
	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r, 
		GLgDrawContextPrivate->constant_g, 
		GLgDrawContextPrivate->constant_b, 
		GLgDrawContextPrivate->constant_a);
	#endif
	
	clipped = UUmMax3(inTri->indices[0], inTri->indices[1], inTri->indices[2]) > GLgDrawContextPrivate->vertexCount;
	clipped = GLgVertexArray ? clipped : UUcTrue;
	
	if (clipped) {
		glBegin(GL_LINE_LOOP);
			glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);
			glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);
			glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);
		glEnd();
	}
	else {
		glBegin(GL_LINE_LOOP);
			glArrayElement(inTri->indices[0]);
			glArrayElement(inTri->indices[1]);
			glArrayElement(inTri->indices[2]);
		glEnd();
	}
	

	FLUSH

	return;
}


void GLrQuad_Wireframe(M3tQuad *inQuad)
{
	UUtBool clipped;
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const M3tPointScreen*	ourScreenPoints[4];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inQuad->indices[0];
	ourScreenPoints[1] = screenPoints + inQuad->indices[1];
	ourScreenPoints[2] = screenPoints + inQuad->indices[2];
	ourScreenPoints[3] = screenPoints + inQuad->indices[3];
	
	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r, 
		GLgDrawContextPrivate->constant_g, 
		GLgDrawContextPrivate->constant_b, 
		GLgDrawContextPrivate->constant_a);
	#endif
	
	clipped = UUmMax4(inQuad->indices[0], inQuad->indices[1], inQuad->indices[2], inQuad->indices[3]) > GLgDrawContextPrivate->vertexCount;
	clipped = GLgVertexArray ? clipped : UUcTrue;
	
	if (clipped) {
		glBegin(GL_LINE_LOOP);
			glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);
			glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);
			glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);
			glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y, ourScreenPoints[3]->z);
		glEnd();
	}
	else {
		glBegin(GL_LINE_LOOP);
			glArrayElement(inQuad->indices[0]);
			glArrayElement(inQuad->indices[1]);
			glArrayElement(inQuad->indices[2]);
			glArrayElement(inQuad->indices[3]);
		glEnd();
	}
	

	FLUSH

	return;
}

void GLrPent_Wireframe(M3tPent *inPent)
{
	UUtBool clipped;
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const M3tPointScreen*	ourScreenPoints[5];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inPent->indices[0];
	ourScreenPoints[1] = screenPoints + inPent->indices[1];
	ourScreenPoints[2] = screenPoints + inPent->indices[2];
	ourScreenPoints[3] = screenPoints + inPent->indices[3];
	ourScreenPoints[4] = screenPoints + inPent->indices[4];
	
	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r, 
		GLgDrawContextPrivate->constant_g, 
		GLgDrawContextPrivate->constant_b, 
		GLgDrawContextPrivate->constant_a);
	#endif
	
	clipped = UUmMax5(inPent->indices[0], inPent->indices[1], inPent->indices[2], inPent->indices[3], inPent->indices[4]) > GLgDrawContextPrivate->vertexCount;
	clipped = GLgVertexArray ? clipped : UUcTrue;
	
	if (clipped) {
		glBegin(GL_LINE_LOOP);
			glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);
			glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);
			glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);
			glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y, ourScreenPoints[3]->z);
			glVertex3f(ourScreenPoints[4]->x, ourScreenPoints[4]->y, ourScreenPoints[4]->z);
		glEnd();
	}
	else {
		glBegin(GL_LINE_LOOP);
			glArrayElement(inPent->indices[0]);
			glArrayElement(inPent->indices[1]);
			glArrayElement(inPent->indices[2]);
			glArrayElement(inPent->indices[3]);
			glArrayElement(inPent->indices[4]);
		glEnd();
	}
	

	FLUSH

	return;
}


void GLrTriangleSplit_VertexLighting(M3tTriSplit *inTri)
{
	UUtBool clipped;
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[3];
	const M3tTextureCoord*	ourTextureCoords[3];
	const UUtUns32*			vertexShades = inTri->shades;
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);
	
	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r, 
		GLgDrawContextPrivate->constant_g, 
		GLgDrawContextPrivate->constant_b, 
		GLgDrawContextPrivate->constant_a);
	#endif
	
	ourScreenPoints[0] = screenPoints + inTri->vertexIndices.indices[0];
	ourScreenPoints[1] = screenPoints + inTri->vertexIndices.indices[1];
	ourScreenPoints[2] = screenPoints + inTri->vertexIndices.indices[2];

	ourTextureCoords[0] = textureCoords + inTri->baseUVIndices.indices[0];
	ourTextureCoords[1] = textureCoords + inTri->baseUVIndices.indices[1];
	ourTextureCoords[2] = textureCoords + inTri->baseUVIndices.indices[2];

	//glColor3ub(0xff, 0xff, 0xff);

	clipped = UUmMax3(inTri->vertexIndices.indices[0], inTri->vertexIndices.indices[1], inTri->vertexIndices.indices[2]) > GLgDrawContextPrivate->vertexCount;
	clipped = GLgVertexArray ? clipped : UUcTrue;
	
	if (clipped) {
		glBegin(GL_TRIANGLES);
			glColor4ub(GLmR(vertexShades[0]), GLmG(vertexShades[0]), GLmB(vertexShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y,ourScreenPoints[0]->z);

			glColor4ub(GLmR(vertexShades[1]), GLmG(vertexShades[1]), GLmB(vertexShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y,ourScreenPoints[1]->z);

			glColor4ub(GLmR(vertexShades[2]), GLmG(vertexShades[2]), GLmB(vertexShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y,ourScreenPoints[2]->z);
		glEnd();
	}
	else {
		glBegin(GL_TRIANGLES);
			glColor4ub(GLmR(vertexShades[0]), GLmG(vertexShades[0]), GLmB(vertexShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glArrayElement(inTri->vertexIndices.indices[0]);

			glColor4ub(GLmR(vertexShades[1]), GLmG(vertexShades[1]), GLmB(vertexShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glArrayElement(inTri->vertexIndices.indices[1]);

			glColor4ub(GLmR(vertexShades[2]), GLmG(vertexShades[2]), GLmB(vertexShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glArrayElement(inTri->vertexIndices.indices[2]);
		glEnd();
	}

	FLUSH

	return;
}

void GLrQuadSplit_VertexLighting(M3tQuadSplit *inQuad)
{
	UUtBool clipped;
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[4];
	const M3tTextureCoord*	ourTextureCoords[4];
	const UUtUns32*			vertexShades = inQuad->shades;
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);
	
	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r, 
		GLgDrawContextPrivate->constant_g, 
		GLgDrawContextPrivate->constant_b, 
		GLgDrawContextPrivate->constant_a);
	#endif
	
	ourScreenPoints[0] = screenPoints + inQuad->vertexIndices.indices[0];
	ourScreenPoints[1] = screenPoints + inQuad->vertexIndices.indices[1];
	ourScreenPoints[2] = screenPoints + inQuad->vertexIndices.indices[2];
	ourScreenPoints[3] = screenPoints + inQuad->vertexIndices.indices[3];

	ourTextureCoords[0] = textureCoords + inQuad->baseUVIndices.indices[0];
	ourTextureCoords[1] = textureCoords + inQuad->baseUVIndices.indices[1];
	ourTextureCoords[2] = textureCoords + inQuad->baseUVIndices.indices[2];
	ourTextureCoords[3] = textureCoords + inQuad->baseUVIndices.indices[3];

	//glColor3ub(0xff, 0xff, 0xff);
	clipped = UUmMax4(inQuad->vertexIndices.indices[0], inQuad->vertexIndices.indices[1], inQuad->vertexIndices.indices[2], inQuad->vertexIndices.indices[3]) > GLgDrawContextPrivate->vertexCount;
	clipped = GLgVertexArray ? clipped : UUcTrue;

	if (clipped) {
		glBegin(GL_QUADS);
			glColor4ub(GLmR(vertexShades[0]), GLmG(vertexShades[0]), GLmB(vertexShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y,ourScreenPoints[0]->z);

			glColor4ub(GLmR(vertexShades[1]), GLmG(vertexShades[1]), GLmB(vertexShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y,ourScreenPoints[1]->z);

			glColor4ub(GLmR(vertexShades[2]), GLmG(vertexShades[2]), GLmB(vertexShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y,ourScreenPoints[2]->z);

			glColor4ub(GLmR(vertexShades[3]), GLmG(vertexShades[3]), GLmB(vertexShades[3]), alpha);
			glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
			glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y,ourScreenPoints[3]->z);
		glEnd();
	}
	else {
		glBegin(GL_QUADS);
			glColor4ub(GLmR(vertexShades[0]), GLmG(vertexShades[0]), GLmB(vertexShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glArrayElement(inQuad->vertexIndices.indices[0]);

			glColor4ub(GLmR(vertexShades[1]), GLmG(vertexShades[1]), GLmB(vertexShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glArrayElement(inQuad->vertexIndices.indices[1]);

			glColor4ub(GLmR(vertexShades[2]), GLmG(vertexShades[2]), GLmB(vertexShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glArrayElement(inQuad->vertexIndices.indices[2]);

			glColor4ub(GLmR(vertexShades[3]), GLmG(vertexShades[3]), GLmB(vertexShades[3]), alpha);
			glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
			glArrayElement(inQuad->vertexIndices.indices[3]);
		glEnd();
	}


	FLUSH

	return;
}

void GLrPentSplit_VertexLighting(M3tPentSplit *inPent)
{
	UUtBool clipped;
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[5];
	const M3tTextureCoord*	ourTextureCoords[5];
	const UUtUns32*			vertexShades = inPent->shades;
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);
	
	#if 0
	glColor4f(
		GLgDrawContextPrivate->constant_r, 
		GLgDrawContextPrivate->constant_g, 
		GLgDrawContextPrivate->constant_b, 
		GLgDrawContextPrivate->constant_a);
	#endif
	
	ourScreenPoints[0] = screenPoints + inPent->vertexIndices.indices[0];
	ourScreenPoints[1] = screenPoints + inPent->vertexIndices.indices[1];
	ourScreenPoints[2] = screenPoints + inPent->vertexIndices.indices[2];
	ourScreenPoints[3] = screenPoints + inPent->vertexIndices.indices[3];
	ourScreenPoints[4] = screenPoints + inPent->vertexIndices.indices[4];

	ourTextureCoords[0] = textureCoords + inPent->baseUVIndices.indices[0];
	ourTextureCoords[1] = textureCoords + inPent->baseUVIndices.indices[1];
	ourTextureCoords[2] = textureCoords + inPent->baseUVIndices.indices[2];
	ourTextureCoords[3] = textureCoords + inPent->baseUVIndices.indices[3];
	ourTextureCoords[4] = textureCoords + inPent->baseUVIndices.indices[4];

	//glColor3ub(0xff, 0xff, 0xff);

	clipped = UUmMax5(inPent->vertexIndices.indices[0], inPent->vertexIndices.indices[1], inPent->vertexIndices.indices[2], inPent->vertexIndices.indices[3], inPent->vertexIndices.indices[4]) > GLgDrawContextPrivate->vertexCount;
	clipped = GLgVertexArray ? clipped : UUcTrue;

	if (clipped) {
		glBegin(GL_POLYGON);
			glColor4ub(GLmR(vertexShades[0]), GLmG(vertexShades[0]), GLmB(vertexShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y,ourScreenPoints[0]->z);

			glColor4ub(GLmR(vertexShades[1]), GLmG(vertexShades[1]), GLmB(vertexShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y,ourScreenPoints[1]->z);

			glColor4ub(GLmR(vertexShades[2]), GLmG(vertexShades[2]), GLmB(vertexShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y,ourScreenPoints[2]->z);

			glColor4ub(GLmR(vertexShades[3]), GLmG(vertexShades[3]), GLmB(vertexShades[3]), alpha);
			glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
			glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y,ourScreenPoints[3]->z);

			glColor4ub(GLmR(vertexShades[4]), GLmG(vertexShades[4]), GLmB(vertexShades[4]), alpha);
			glTexCoord4f(ourTextureCoords[4]->u * ourScreenPoints[4]->invW, ourTextureCoords[4]->v * ourScreenPoints[4]->invW, 0.f, ourScreenPoints[4]->invW);
			glVertex3f(ourScreenPoints[4]->x, ourScreenPoints[4]->y,ourScreenPoints[4]->z);
		glEnd();
	}
	else {
		glBegin(GL_POLYGON);
			glColor4ub(GLmR(vertexShades[0]), GLmG(vertexShades[0]), GLmB(vertexShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glArrayElement(inPent->vertexIndices.indices[0]);

			glColor4ub(GLmR(vertexShades[1]), GLmG(vertexShades[1]), GLmB(vertexShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glArrayElement(inPent->vertexIndices.indices[1]);

			glColor4ub(GLmR(vertexShades[2]), GLmG(vertexShades[2]), GLmB(vertexShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glArrayElement(inPent->vertexIndices.indices[2]);

			glColor4ub(GLmR(vertexShades[3]), GLmG(vertexShades[3]), GLmB(vertexShades[3]), alpha);
			glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
			glArrayElement(inPent->vertexIndices.indices[3]);

			glColor4ub(GLmR(vertexShades[4]), GLmG(vertexShades[4]), GLmB(vertexShades[4]), alpha);
			glTexCoord4f(ourTextureCoords[4]->u * ourScreenPoints[4]->invW, ourTextureCoords[4]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[4]->invW);
			glArrayElement(inPent->vertexIndices.indices[4]);
		glEnd();
	}


	FLUSH

	return;
}

void GLrTriangle_EnvironmentMap_Only(M3tTri *inTri)
{
	UUtBool clipped;
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const UUtUns32*			vertexShades = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenShadeArray_DC];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_EnvTextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[3];
	UUtUns32		ourShades[3];
	const M3tTextureCoord*	ourTextureCoords[3];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inTri->indices[0];
	ourScreenPoints[1] = screenPoints + inTri->indices[1];
	ourScreenPoints[2] = screenPoints + inTri->indices[2];

	ourShades[0] = vertexShades[inTri->indices[0]];
	ourShades[1] = vertexShades[inTri->indices[1]];
	ourShades[2] = vertexShades[inTri->indices[2]];

	ourTextureCoords[0] = textureCoords + inTri->indices[0];
	ourTextureCoords[1] = textureCoords + inTri->indices[1];
	ourTextureCoords[2] = textureCoords + inTri->indices[2];

	clipped = UUmMax3(inTri->indices[0], inTri->indices[1], inTri->indices[2]) > GLgDrawContextPrivate->vertexCount;
	clipped = GLgVertexArray ? clipped : UUcTrue;
	
	if (clipped) {
		glBegin(GL_TRIANGLES);
			glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

			glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

			glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);
		glEnd();
	}
	else {
		glBegin(GL_TRIANGLES);
			glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glArrayElement(inTri->indices[0]);

			glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glArrayElement(inTri->indices[1]);

			glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glArrayElement(inTri->indices[2]);
		glEnd();
	}
	

	FLUSH

	return;
}

void GLrQuad_EnvironmentMap_Only(M3tQuad *inQuad)
{
	UUtBool clipped;
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const UUtUns32*			vertexShades = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenShadeArray_DC];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[4];
	UUtUns32		ourShades[4];
	const M3tTextureCoord*	ourTextureCoords[4];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inQuad->indices[0];
	ourScreenPoints[1] = screenPoints + inQuad->indices[1];
	ourScreenPoints[2] = screenPoints + inQuad->indices[2];
	ourScreenPoints[3] = screenPoints + inQuad->indices[3];

	ourShades[0] = vertexShades[inQuad->indices[0]];
	ourShades[1] = vertexShades[inQuad->indices[1]];
	ourShades[2] = vertexShades[inQuad->indices[2]];
	ourShades[3] = vertexShades[inQuad->indices[3]];

	ourTextureCoords[0] = textureCoords + inQuad->indices[0];
	ourTextureCoords[1] = textureCoords + inQuad->indices[1];
	ourTextureCoords[2] = textureCoords + inQuad->indices[2];
	ourTextureCoords[3] = textureCoords + inQuad->indices[3];
			
	clipped = UUmMax4(inQuad->indices[0], inQuad->indices[1], inQuad->indices[2], inQuad->indices[3]) > GLgDrawContextPrivate->vertexCount;
	clipped = GLgVertexArray ? clipped : UUcTrue;

	if (clipped) {
		glBegin(GL_QUADS);
			glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

			glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

			glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);

			glColor4ub(GLmR(ourShades[3]), GLmG(ourShades[3]), GLmB(ourShades[3]), alpha);
			glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
			glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y, ourScreenPoints[3]->z);
		glEnd();
	}
	else {
		glBegin(GL_QUADS);
			glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
			glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
			glArrayElement(inQuad->indices[0]);

			glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
			glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
			glArrayElement(inQuad->indices[1]);

			glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
			glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
			glArrayElement(inQuad->indices[2]);

			glColor4ub(GLmR(ourShades[3]), GLmG(ourShades[3]), GLmB(ourShades[3]), alpha);
			glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
			glArrayElement(inQuad->indices[3]);
		glEnd();
	}


	FLUSH

	return;
}



void GLrPent_EnvironmentMap_Only(M3tPent *inPent)
{
	const M3tPointScreen*		screenPoints = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenPointArray];
	const UUtUns32*			vertexShades = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_ScreenShadeArray_DC];
	const M3tTextureCoord*	textureCoords = GLgDrawContextPrivate->statePtr[M3cDrawStatePtrType_TextureCoordArray];
	const M3tPointScreen*	ourScreenPoints[5];
	UUtUns32		ourShades[5];
	const M3tTextureCoord*	ourTextureCoords[5];
	UUtUns8 alpha = (UUtUns8) MUrUnsignedSmallFloat_To_Int_Round(GLgDrawContextPrivate->constant_a * 0xff);

	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	ourScreenPoints[0] = screenPoints + inPent->indices[0];
	ourScreenPoints[1] = screenPoints + inPent->indices[1];
	ourScreenPoints[2] = screenPoints + inPent->indices[2];
	ourScreenPoints[3] = screenPoints + inPent->indices[3];
	ourScreenPoints[4] = screenPoints + inPent->indices[4];

	ourShades[0] = vertexShades[inPent->indices[0]];
	ourShades[1] = vertexShades[inPent->indices[1]];
	ourShades[2] = vertexShades[inPent->indices[2]];
	ourShades[3] = vertexShades[inPent->indices[3]];
	ourShades[4] = vertexShades[inPent->indices[4]];

	ourTextureCoords[0] = textureCoords + inPent->indices[0];
	ourTextureCoords[1] = textureCoords + inPent->indices[1];
	ourTextureCoords[2] = textureCoords + inPent->indices[2];
	ourTextureCoords[3] = textureCoords + inPent->indices[3];
	ourTextureCoords[4] = textureCoords + inPent->indices[4];
			
	glBegin(GL_POLYGON);
		glColor4ub(GLmR(ourShades[0]), GLmG(ourShades[0]), GLmB(ourShades[0]), alpha);
		glTexCoord4f(ourTextureCoords[0]->u * ourScreenPoints[0]->invW, ourTextureCoords[0]->v * ourScreenPoints[0]->invW, 0.f, ourScreenPoints[0]->invW);
		glVertex3f(ourScreenPoints[0]->x, ourScreenPoints[0]->y, ourScreenPoints[0]->z);

		glColor4ub(GLmR(ourShades[1]), GLmG(ourShades[1]), GLmB(ourShades[1]), alpha);
		glTexCoord4f(ourTextureCoords[1]->u * ourScreenPoints[1]->invW, ourTextureCoords[1]->v * ourScreenPoints[1]->invW, 0.f, ourScreenPoints[1]->invW);
		glVertex3f(ourScreenPoints[1]->x, ourScreenPoints[1]->y, ourScreenPoints[1]->z);

		glColor4ub(GLmR(ourShades[2]), GLmG(ourShades[2]), GLmB(ourShades[2]), alpha);
		glTexCoord4f(ourTextureCoords[2]->u * ourScreenPoints[2]->invW, ourTextureCoords[2]->v * ourScreenPoints[2]->invW, 0.f, ourScreenPoints[2]->invW);
		glVertex3f(ourScreenPoints[2]->x, ourScreenPoints[2]->y, ourScreenPoints[2]->z);

		glColor4ub(GLmR(ourShades[3]), GLmG(ourShades[3]), GLmB(ourShades[3]), alpha);
		glTexCoord4f(ourTextureCoords[3]->u * ourScreenPoints[3]->invW, ourTextureCoords[3]->v * ourScreenPoints[3]->invW, 0.f, ourScreenPoints[3]->invW);
		glVertex3f(ourScreenPoints[3]->x, ourScreenPoints[3]->y, ourScreenPoints[3]->z);

		glColor4ub(GLmR(ourShades[4]), GLmG(ourShades[4]), GLmB(ourShades[4]), alpha);
		glTexCoord4f(ourTextureCoords[4]->u * ourScreenPoints[4]->invW, ourTextureCoords[4]->v * ourScreenPoints[4]->invW, 0.f, ourScreenPoints[4]->invW);
		glVertex3f(ourScreenPoints[4]->x, ourScreenPoints[4]->y, ourScreenPoints[4]->z);
	glEnd();


	FLUSH

	return;
}

void GLrTriangle_EnvironmentMap_1TMU(M3tTri *inTri)
{
	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	OGLrCommon_TextureMap_Select(GLgDrawContextPrivate->curEnvironmentMap, GL_TEXTURE0_ARB);
	glBlendFunc(GL_ONE, GL_ZERO);
	GLrTriangle_EnvironmentMap_Only(inTri);

	OGLrCommon_TextureMap_Select(GLgDrawContextPrivate->curBaseMap, GL_TEXTURE0_ARB);
	glBlendFunc(GL_ONE, GL_SRC_ALPHA);
	GLrTriangle(inTri);

	FLUSH

	return;
}

void GLrQuad_EnvironmentMap_1TMU(M3tQuad *inQuad)
{
	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	OGLrCommon_TextureMap_Select(GLgDrawContextPrivate->curEnvironmentMap, GL_TEXTURE0_ARB);
	glBlendFunc(GL_ONE, GL_ZERO);
	GLrQuad_EnvironmentMap_Only(inQuad);

	OGLrCommon_TextureMap_Select(GLgDrawContextPrivate->curBaseMap, GL_TEXTURE0_ARB);
	glBlendFunc(GL_ONE, GL_SRC_ALPHA);
	GLrQuad(inQuad);

	FLUSH

	return;
}

void GLrPent_EnvironmentMap_1TMU(M3tPent *inPent)
{
	OGLrCommon_Camera_Update(OGLcCameraMode_2D);

	OGLrCommon_TextureMap_Select(GLgDrawContextPrivate->curEnvironmentMap, GL_TEXTURE0_ARB);
	glBlendFunc(GL_ONE, GL_ZERO);
	GLrPent_EnvironmentMap_Only(inPent);

	OGLrCommon_TextureMap_Select(GLgDrawContextPrivate->curBaseMap, GL_TEXTURE0_ARB);
	glBlendFunc(GL_ONE, GL_SRC_ALPHA);
	GLrPent(inPent);

	FLUSH

	return;
}

#endif // #ifndef __ONADA__
