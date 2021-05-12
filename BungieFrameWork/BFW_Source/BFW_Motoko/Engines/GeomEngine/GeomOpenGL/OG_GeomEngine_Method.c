/*
	FILE:	OG_GeomEngine_Method.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: June 10, 1999
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997

*/

#ifndef __ONADA__

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Akira.h"
#include "BFW_BitVector.h"
#include "BFW_Console.h"
#include "BFW_TemplateManager.h"
#include "BFW_ScriptLang.h"

#include "Motoko_Manager.h"
#include "OGL_DrawGeom_Common.h"

#include "OG_GeomEngine_Method.h"
#include "OG_GC_Private.h"
#include "OG_GC_Method_Frame.h"
#include "OG_GC_Method_Camera.h"
#include "OG_GC_Method_Light.h"
#include "OG_GC_Method_State.h"
#include "OG_GC_Method_Geometry.h"
#include "OG_GC_Method_Env.h"
#include "OG_GC_Method_Pick.h"

#define OGcMaxGeomEntries		128
#define	OGcGeomEngine_OpenGL	"OpenGL"
#define OGcGeomVersion			1

OGtGeomContextPrivate	OGgGeomContextPrivate;

UUtError
OGiCache_Geometry_Load(
	void*		inInstanceData,
	void*		*outDataPtr)
{
	GLuint			newIndex;
	M3tGeometry*	geometry = (M3tGeometry*)inInstanceData;
	UUtUns32*		lastVertexPtr;
	UUtUns32*		curVertexPtr;
	UUtUns32		curIndex;
	M3tVector3D*	curTriNormal;
	
	//COrConsole_Print("Loading geometry");
	
	newIndex = glGenLists(1);
	if(newIndex == 0) UUmError_ReturnOnErrorMsg(UUcError_OutOfMemory, "OGL: out of display lists");
	
	OGLrCommon_glEnableClientState(GL_VERTEX_ARRAY);
	OGLrCommon_glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	OGLrCommon_glEnableClientState(GL_NORMAL_ARRAY);
	
	OGLrCommon_glDisableClientState(GL_COLOR_ARRAY);
	OGLrCommon_glDisableClientState(GL_INDEX_ARRAY);
	OGLrCommon_glDisableClientState(GL_EDGE_FLAG_ARRAY);

	glNewList(newIndex, GL_COMPILE);
	
	glVertexPointer(3, GL_FLOAT, 0, geometry->pointArray->points);
	glNormalPointer(GL_FLOAT, 0, geometry->vertexNormalArray->vectors);
	glTexCoordPointer(2, GL_FLOAT, 0, geometry->texCoordArray->textureCoords);
	
	curVertexPtr = geometry->triStripArray->indices;
	lastVertexPtr = curVertexPtr + geometry->triStripArray->numIndices;
	curTriNormal = geometry->triNormalArray->vectors;
	
	glBegin(GL_TRIANGLE_STRIP);
		curIndex = *curVertexPtr++ & 0x7FFFFFFF;
		glArrayElement(curIndex);
		curIndex = *curVertexPtr++;
		glArrayElement(curIndex);
		
		while(curVertexPtr < lastVertexPtr)
		{
			curIndex = *curVertexPtr++;
			
			if(curIndex & 0x80000000)
			{
				glEnd();
				glBegin(GL_TRIANGLE_STRIP);
				curIndex &= 0x7FFFFFFF;
				glArrayElement(curIndex);
				curIndex = *curVertexPtr++;
				glArrayElement(curIndex);
				curIndex = *curVertexPtr++;
			}
			
			glArrayElement(curIndex);
		}
	glEnd();
	
	glEndList();
	
	UUmAssert(glIsList(newIndex) == GL_TRUE);

	*outDataPtr = (void*)newIndex;
	
	return UUcError_None;
}

static void
OGiCache_Geometry_Unload(
	void*		inInstanceData,
	void*		inDataPtr)
{
	//COrConsole_Print("Unoading geometry");
	
	glDeleteLists((GLuint)inDataPtr, 1);
	
}

static UUtError
OGiGeomEngine_Method_ContextPrivateNew(
	M3tDrawContextDescriptor*	inDrawContextDescriptor,
	M3tGeomContextMethods*		*outGeomContextFuncs)
{
	UUtError	error;
	
	/*
	 * Initialize the methods
	 */
		OGgGeomContextPrivate.contextMethods.frameStart = OGrGeomContext_Method_Frame_Start;
		OGgGeomContextPrivate.contextMethods.frameEnd = OGrGeomContext_Method_Frame_End;
		OGgGeomContextPrivate.contextMethods.pickScreenToWorld = OGrGeomContext_Method_Pick_ScreenToWorld;
		OGgGeomContextPrivate.contextMethods.lightListAmbient = OGrGeomContext_Method_LightList_Ambient;
		OGgGeomContextPrivate.contextMethods.lightListDirectional = OGrGeomContext_Method_LightList_Directional;
		OGgGeomContextPrivate.contextMethods.lightListPoint = OGrGeomContext_Method_LightList_Point;
		OGgGeomContextPrivate.contextMethods.lightListCone = OGrGeomContext_Method_LightList_Cone;
		OGgGeomContextPrivate.contextMethods.geometryBoundingBoxDraw = OGrGeomContext_Method_Geometry_BoundingBox_Draw;
		OGgGeomContextPrivate.contextMethods.spriteDraw = OGrGeomContext_Method_Sprite_Draw;
		OGgGeomContextPrivate.contextMethods.contrailDraw = OGrGeomContext_Method_Contrail_Draw;
		OGgGeomContextPrivate.contextMethods.geometryPolyDraw = OGrGeomContext_Method_Geometry_PolyDraw;
		OGgGeomContextPrivate.contextMethods.geometryLineDraw = OGrGeomContext_Method_Geometry_LineDraw;
		OGgGeomContextPrivate.contextMethods.geometryPointDraw = OGrGeomContext_Method_Geometry_PointDraw;
		OGgGeomContextPrivate.contextMethods.envSetCamera = OGrGeomContext_Method_Env_SetCamera;
		OGgGeomContextPrivate.contextMethods.envDrawGQList = OGrGeomContext_Method_Env_DrawGQList;
		OGgGeomContextPrivate.contextMethods.geometryDraw = OGrGeomContext_Method_Geometry_Draw;
		OGgGeomContextPrivate.contextMethods.pointVisible = OGrGeomContext_Method_PointVisible;
	
	OGgGeomContextPrivate.environment = NULL;

	error = 
		TMrTemplate_Cache_Simple_New(
			M3cTemplate_Geometry,
			OGcMaxGeomEntries,
			OGiCache_Geometry_Load,
			OGiCache_Geometry_Unload,
			&OGgGeomContextPrivate.geometryCache);
	
	*outGeomContextFuncs = &OGgGeomContextPrivate.contextMethods;
	
	return UUcError_None;
}

static void
OGiGeomEngine_Method_ContextPrivateDelete(
	void)
{
	TMrTemplate_Cache_Simple_Delete(OGgGeomContextPrivate.geometryCache);
}

static UUtError
OGiGeomEngine_Method_ContextSetEnvironment(
	struct AKtEnvironment*		inEnvironment)
{
	OGgGeomContextPrivate.environment = inEnvironment;
	
	return UUcError_None;
}

// This lets the engine allocate a new private state structure
static UUtError
OGiGeomEngine_Method_PrivateState_New(
	void*	inState_Private)
{
	
	return UUcError_None;
}

// This lets the engine delete a new private state structure
static void 
OGiGeomEngine_Method_PrivateState_Delete(
	void*	inState_Private)
{

}

// This lets the engine update the state according to the update flags
static UUtError
OGiGeomEngine_Method_State_Update(
	void*			inState_Private,
	UUtUns16		inState_IntFlags,
	const UUtInt32*	inState_Int)
{
	
	if(inState_IntFlags & (1 << M3cGeomStateIntType_Alpha))
	{
		//float rgba[4];
		
		//glGetFloatV(GL_CURRENT_COLOR, f);
		glColor4f(1.0, 1.0f, 1.0f, inState_Int[M3cGeomStateIntType_Alpha] * (1.0f / 255.0f));
		
	}
	
	return UUcError_None;
}

static void
OGiGeomEngine_Method_Camera_View_Update(
	void)
{
	OGLgCommon.update3DCamera_ViewData = UUcTrue;
}

static void
OGiGeomEngine_Method_Camera_Static_Update(
	void)
{
	OGLgCommon.update3DCamera_StaticData = UUcTrue;
}

void
OGrGeomEngine_Initialize(
	void)
{
	UUtError				error;
	M3tGeomEngineMethods	geomEngineMethods;
	M3tGeomEngineCaps		geomEngineCaps;
	UUtUns16				numDrawEngines;
	UUtUns16				itrDrawEngine;
	M3tDrawEngineCaps*		drawEngineCaps;
	
	geomEngineMethods.contextPrivateNew = OGiGeomEngine_Method_ContextPrivateNew;
	geomEngineMethods.contextPrivateDelete = OGiGeomEngine_Method_ContextPrivateDelete;
	geomEngineMethods.contextSetEnvironment = OGiGeomEngine_Method_ContextSetEnvironment;
	
	geomEngineMethods.privateStateSize = 0;
	geomEngineMethods.privateStateNew = OGiGeomEngine_Method_PrivateState_New;
	geomEngineMethods.privateStateDelete = OGiGeomEngine_Method_PrivateState_Delete;
	geomEngineMethods.privateStateUpdate = OGiGeomEngine_Method_State_Update;
	
	geomEngineMethods.cameraViewUpdate = OGiGeomEngine_Method_Camera_View_Update;
	geomEngineMethods.cameraStaticUpdate = OGiGeomEngine_Method_Camera_Static_Update;
	
	geomEngineCaps.engineFlags = M3tGeomEngineFlag_None;
	
	UUrString_Copy(geomEngineCaps.engineName, OGcGeomEngine_OpenGL, M3cMaxNameLen);
	geomEngineCaps.engineDriver[0] = 0;
	
	geomEngineCaps.engineVersion = OGcGeomVersion;
	
	// Find my compatable opengl draw engine
		numDrawEngines = M3rDrawEngine_GetNumber();
		for(itrDrawEngine = 0; itrDrawEngine < numDrawEngines; itrDrawEngine++)
		{
			drawEngineCaps = M3rDrawEngine_GetCaps(itrDrawEngine);
			if(!strcmp(drawEngineCaps->engineName, M3cDrawEngine_OpenGL)) break;
		}
	
	// bail if we did not find the open gl draw engine
	if(itrDrawEngine >= numDrawEngines) return;
	
	geomEngineCaps.compatibleDrawEngineBV = 1 << itrDrawEngine;
	
	error = M3rManager_Register_GeomEngine(&geomEngineCaps, &geomEngineMethods);
	if(error != UUcError_None)
	{
		UUrError_Report(error, "Could not register motoko opengl geometry engine");
	}
	
}

void
OGrGeomEngine_Terminate(
	void)
{

}




#endif // __ONADA__
