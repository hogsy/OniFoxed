/**********************************************************************
 *<
	FILE: LightmapExporter.cpp

	DESCRIPTION: ONI enivornment export module

	CREATED BY: Michael Evans

	HISTORY: Dec 10, 1997

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/


#include "ExportMain.h"
#include "ExportTools.h"

#include "Max.h"

// oni files
#include "BFW_TemplateManager.h"
#include "BFW_FileManager.h"
#include "BFW_Motoko.h"
#include "BFW.h"


#include <stdarg.h>


//------------------------------------------------------

class LightmapExport : public SceneExport 
{
public:
	int				ExtCount()						{ return 1; }
	const TCHAR *	Ext(int n)						{ UUmAssert(0 == n); return "lit"; }
	const TCHAR *	LongDesc()						{ return "Oni Lightmap File"; }
	const TCHAR *	ShortDesc()						{ return "Oni Lightmap File"; }
	const TCHAR *	AuthorName()					{ return "Michael Evans"; }
	const TCHAR *	CopyrightMessage()				{ return "Copyright Bungie Software 1997"; }
	const TCHAR *	OtherMessage1()					{ return ""; }
	const TCHAR *	OtherMessage2()					{ return ""; }
	unsigned int	Version()						{ return 100; } // 1.00
	void			ShowAbout(HWND hWnd)			{}

	int				DoExport(const char *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts); // Export file
};

class LightmapClassDesc:public ClassDesc {
	public:
	int 			IsPublic()						{ return 1; }
	void *			Create(BOOL loading = FALSE)	{ return new LightmapExport; }
	const TCHAR *	ClassName()						{ return "ONI Lightmap Exporter"; }
	SClass_ID		SuperClassID()					{ return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return Class_ID(0xe2f39058, 0xf6d46452); }
	const TCHAR* 	Category()						{ return "Scene Export";  }
};

LightmapClassDesc	LightmapDesc;
ClassDesc			*pLightmapDesc = &LightmapDesc;

/*
 *
 * This class dumps everything to a single file.
 *
 */


int LightmapExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi, BOOL suppressPrompts) 
{
	IScene		*scene = ei->theScene;
	TimeValue	time   = gi->GetTime();

	UUtError	error;

	error = UUrInitialize(UUcFalse);	// don't initialize the sytem, max will have done this already
	UUmAssert(UUcError_None == error);

	gi->ProgressStart("Lightmap Export", TRUE, progress, 0);

	// get our list of nodes
	INode **nodeList = NodeList::GetNodeList(scene, time, kCountDualTextureNormalNodes);
	int count = NodeCounter::CountNodes(scene, time, kCountDualTextureNormalNodes);
	int nodeItr;
	int itr;
	int from;

	// open our file
	gi->ProgressUpdate(0, FALSE, "opening file");
	FILE *stream = fopen_safe(filename, "wb");

	// could not open file so bail
	UUmAssert(NULL != stream);

	fprintf(stream, "Lightmap file\r\n");
	fprintf(stream, "version\r\n");
	fprintf(stream, "1.00\r\n");
	fprintf(stream, "count\r\n");
	fprintf(stream, "%d\r\n",count);
	fprintf(stream, "\r\n");

	Point3 *verts = NULL;
	Point3 *normals = NULL;
	Point3 *uvs = NULL;
	Point3 *rgbs = NULL;

	int pointCount = 0;
	int uvCount = 0;
	int rgbCount = 0;

	typedef struct {
		UUtUns32 v[3];
	} IndexGroup;

	IndexGroup *triIndex = NULL;
	IndexGroup *uvIndex = NULL;
	IndexGroup *rgbIndex = NULL;

	int faceCount = 0;

	// iterate through our nodes building up the list
	for(nodeItr = 0; nodeItr < count; nodeItr++)
	{
		int progressAmt = (100 * nodeItr) / count;

		Scene scene(scene, time, gi);
		INode *node = nodeList[nodeItr];
		char *name = node->GetName();

		gi->ProgressUpdate(progressAmt, FALSE, name);

		Object		*object			= node->EvalWorldState(time).obj;
		UUmAssert(NULL != object);

		int			canMesh			= object->CanConvertToType(triObjectClassID);
		UUmAssert(canMesh);

		TriObject	*triangle		= (TriObject *) object->ConvertToType(time, triObjectClassID);
		UUmAssert(NULL != triangle);

		Mesh		*mesh			= &(triangle->mesh);
		UUmAssert(NULL != mesh);

		bool		deleteTriangle	= (object) != ((Object *) triangle);

		UUmAssert(NULL != mesh->tVerts);
		UUmAssert(NULL != mesh->vertCol);

		// build the normals
		mesh->buildNormals();

		int newFaceCount = faceCount + mesh->numFaces;

		// grow the index lists
		triIndex = (IndexGroup *) UUrMemory_Block_Realloc(triIndex, newFaceCount * sizeof(IndexGroup));
		uvIndex = (IndexGroup *) UUrMemory_Block_Realloc(uvIndex, newFaceCount * sizeof(IndexGroup));
		rgbIndex = (IndexGroup *) UUrMemory_Block_Realloc(rgbIndex, newFaceCount * sizeof(IndexGroup));

		for(itr = faceCount; itr < newFaceCount; itr++)
		{
			from = itr - faceCount;

			// copy + offset the triangles
			triIndex[itr].v[0] = mesh->faces[from].v[0] + pointCount;
			triIndex[itr].v[1] = mesh->faces[from].v[1] + pointCount;
			triIndex[itr].v[2] = mesh->faces[from].v[2] + pointCount; 

			// copy + offset the texture
			uvIndex[itr].v[0] = mesh->tvFace[from].t[0] + uvCount;
			uvIndex[itr].v[1] = mesh->tvFace[from].t[1] + uvCount;
			uvIndex[itr].v[2] = mesh->tvFace[from].t[2] + uvCount; 

			// copy + offset the color
			rgbIndex[itr].v[0] = mesh->tvFace[from].t[0] + rgbCount;
			rgbIndex[itr].v[1] = mesh->tvFace[from].t[1] + rgbCount;
			rgbIndex[itr].v[2] = mesh->tvFace[from].t[2] + rgbCount; 		
		}

		// do the regular triangle points
		int newPointCount = pointCount + mesh->numVerts;

		verts = (Point3 *) UUrMemory_Block_Realloc(verts, newPointCount * sizeof(Point3));
		normals = (Point3 *) UUrMemory_Block_Realloc(normals, newPointCount * sizeof(Point3));

		for(itr = pointCount; itr < newPointCount; itr++)
		{
			from = itr - pointCount;

			Point3 theVertex = mesh->verts[from];
			Point3 theNormal = mesh->getNormal(from);

			verts[itr] = theVertex;
			normals[itr] = theNormal;
		}

		// do tverts
		int newUVCount = uvCount + mesh->numTVerts;

		uvs = (Point3 *) UUrMemory_Block_Realloc(uvs, newUVCount * sizeof(Point3));

		for(itr = uvCount; itr < newUVCount; itr++)
		{
			from = itr - uvCount;

			uvs[itr] = mesh->tVerts[from];
		}

		// Color per vertex
		int newRGBCount = rgbCount + mesh->numCVerts;

		rgbs = (Point3 *) UUrMemory_Block_Realloc(rgbs, newRGBCount * sizeof(Point3));

		for(itr = rgbCount; itr < newRGBCount; itr++)
		{
			from = itr - rgbCount;

			rgbs[itr] = mesh->vertCol[from];
		}

		if (deleteTriangle) {
			UUmAssert(NULL != triangle);

			delete triangle;
		}

		faceCount = newFaceCount;
		pointCount = newPointCount;
		uvCount = newUVCount;
		rgbCount = newRGBCount;
	}

	fprintf(stream, "face count\r\n");
	fprintf(stream, "%d\r\n", faceCount);
	fprintf(stream, "p1,p2,p3, uv1, uv2, uv3, rgb1, rgb2, rgb3\r\n");

	for(itr = 0; itr < faceCount; itr++)
	{
		fprintf(stream, "%d\r\n", triIndex[itr].v[0]);
		fprintf(stream, "%d\r\n", triIndex[itr].v[1]);
		fprintf(stream, "%d\r\n", triIndex[itr].v[2]);
		fprintf(stream, "\r\n");

		fprintf(stream, "%d\r\n", uvIndex[itr].v[0]);
		fprintf(stream, "%d\r\n", uvIndex[itr].v[1]);
		fprintf(stream, "%d\r\n", uvIndex[itr].v[2]);
		fprintf(stream, "\r\n");

		fprintf(stream, "%d\r\n", rgbIndex[itr].v[0]);
		fprintf(stream, "%d\r\n", rgbIndex[itr].v[1]);
		fprintf(stream, "%d\r\n", rgbIndex[itr].v[2]);
		fprintf(stream, "\r\n");
	}

	fprintf(stream, "point count\r\n");
	fprintf(stream, "%d\r\n", pointCount);
	fprintf(stream, "x,y,z,nx,ny,nz\r\n");
	for(itr = 0; itr < pointCount; itr++)
	{
		fprintf(stream, "%f\r\n", verts[itr].x);
		fprintf(stream, "%f\r\n", verts[itr].y);
		fprintf(stream, "%f\r\n", verts[itr].z);
		fprintf(stream, "\r\n");

		fprintf(stream, "%f\r\n", normals[itr].x);
		fprintf(stream, "%f\r\n", normals[itr].y);
		fprintf(stream, "%f\r\n", normals[itr].z);
		fprintf(stream, "\r\n");
	}

	fprintf(stream, "uv count\r\n");
	fprintf(stream, "%d\r\n", uvCount);
	for(itr = 0; itr < uvCount; itr++)
	{
		fprintf(stream, "%f\r\n", uvs[itr].x);
		fprintf(stream, "%f\r\n", uvs[itr].y);
		fprintf(stream, "\r\n");
	}

	fprintf(stream, "rgb count\r\n");
	fprintf(stream, "%d\r\n", rgbCount);
	for(itr = 0; itr < rgbCount; itr++)
	{
		fprintf(stream, "%f\r\n", rgbs[itr].x);
		fprintf(stream, "%f\r\n", rgbs[itr].y);
		fprintf(stream, "%f\r\n", rgbs[itr].z);
		fprintf(stream, "\r\n");
	}

	if (NULL != verts) 
	{
		UUrMemory_Block_Delete(verts);
	}

	if (NULL != normals)
	{
		UUrMemory_Block_Delete(normals);
	}

	if (NULL != uvs)
	{
		UUrMemory_Block_Delete(uvs);
	}

	if (NULL != rgbs)
	{
		UUrMemory_Block_Delete(rgbs);
	}

	if (NULL != nodeList)
	{
		UUrMemory_Block_Delete(nodeList);
	}

	gi->ProgressUpdate(100, FALSE, "closing file");
	fclose(stream);

	gi->ProgressEnd();

	UUrTerminate();

	return 1; // no error
}
