/**********************************************************************
 *<
	FILE: HierarchyExpoter.cpp

	DESCRIPTION: ONI hierarchy export module

	CREATED BY: Michael Evans

	HISTORY: July 1st 1997

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/


#include "ExportMain.h"
#include "ExportTools.h"

#include "Max.h"

// oni files
#include "BFW_FileManager.h"
#include "BFW_Motoko.h"
#include "BFW.h"


#define EXPORT_VERSION	"0.1"

#include <stdarg.h>


//------------------------------------------------------


class HierarchyExport : public SceneExport 
{
public:
					HierarchyExport()				{}
					~HierarchyExport()				{}
	int				ExtCount()						{ return 1; }
	const TCHAR *	Ext(int n)						{ UUmAssert(0 == n); return "hie"; }
	const TCHAR *	LongDesc()						{ return "Oni Hierarchy File (2.0)"; }
	const TCHAR *	ShortDesc()						{ return "Oni Hierarchy File (2.0)"; }
	const TCHAR *	AuthorName()					{ return "Michael Evans"; }
	const TCHAR *	CopyrightMessage()				{ return "Copyright Bungie Software 1997"; }
	const TCHAR *	OtherMessage1()					{ return ""; }
	const TCHAR *	OtherMessage2()					{ return ""; }
	unsigned int	Version()						{ return 100; } // 1.00
	void			ShowAbout(HWND hWnd)			{}

	int				DoExport(const char *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE);
};

class HierarchyClassDesc:public ClassDesc {
	public:
	int 			IsPublic()						{ return 1; }
	void *			Create(BOOL loading = FALSE)	{ return new HierarchyExport; }
	const TCHAR *	ClassName()						{ return "ONI Hierarchy Exporter"; }
	SClass_ID		SuperClassID()					{ return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return Class_ID(0xa7a44a43, 0x45b32565); }
	const TCHAR* 	Category()						{ return "Scene Export";  }
};

HierarchyClassDesc	HierarchyDesc;
ClassDesc			*pHierarchyDesc = &HierarchyDesc;

/*
 *
 * This class dumps everything to a single file.
 *
 */

class HierarchyEnumProc : public ITreeEnumProc 
{	
	public:
		HierarchyEnumProc(IScene *scene, TimeValue t, Interface *i);
		~HierarchyEnumProc();

		void		DoExport(const char *inFileName);
		int			callback(INode *node);

	private:	
		IScene			*fScene;
		TimeValue		fTime;
		Interface		*fInterface;

		long			fCount;
		long			fTotalNodes;

	public:
		Scene *fBuiltScene;
};

HierarchyEnumProc::HierarchyEnumProc(IScene *scene, TimeValue t, Interface *i) 
{
	fScene = scene;
	fTime = t;
	fInterface = i;

	fCount = 0;
	fBuiltScene = NULL;
}

void HierarchyEnumProc::DoExport(const char *inFileName)
{
	fInterface->ProgressStart("Hierarchy Export", TRUE, progress, 0);

	fBuiltScene = new Scene(fScene, fTime, fInterface);

	UUtError	error;

	error = UUrInitialize(UUcFalse);	// don't initialize the sytem, max will have done this already
	UUmAssert(UUcError_None == error);

	// because i am curious sometimes
	fTotalNodes = NodeCounter::CountNodes(fScene, fTime);

	// enumerate (callback is magically called back)
	fScene->EnumTree(this);

	fBuiltScene->CoordinateSystemMaxToOni();
	fBuiltScene->FacesToQuads(Scene::QuadOptions_Exact);
	fBuiltScene->RoundPoints();
	fBuiltScene->ClearUV();
	fBuiltScene->CompressPoints();

	fBuiltScene->BuildHierarchy();

	// variable are now built, write to disk
	FILE *stream = fopen_safe(inFileName, "wb");

	if (NULL != stream) {
		fprintf(stream, "Hierarchy Dump Version 10/10/97 build 2\r\n");
		
		UUtUns32 pointOptions = 0;

		pointOptions |= Scene::WriteOptions_NoVertexNormals;
		pointOptions |= Scene::WriteOptions_NoVertexUV;

		fBuiltScene->WritePoints(stream, pointOptions);	// no vertex normals, no vertex uv
		fBuiltScene->WriteFaces(stream, Scene::WriteOptions_NoFaceNormals);				// no face normals
		fBuiltScene->WriteGeometries(stream);

		fclose(stream);
	}

	// close everything		
	fInterface->ProgressUpdate(0, FALSE, "finishing");
	delete fBuiltScene;
	fBuiltScene = NULL;
	UUrTerminate();

	fInterface->ProgressEnd();
}

HierarchyEnumProc::~HierarchyEnumProc() {
	UUmAssert(NULL == fBuiltScene);
}

int HierarchyEnumProc::callback(INode *node) 
{
	// update the progress bar
	char buf[255];
	sprintf(buf, "extracing %s", node->GetName());
	fInterface->ProgressUpdate((100 * fCount) / fTotalNodes, FALSE, buf);

	// go the next object
	fCount++;

	fBuiltScene->AddNode(node);

	return TREE_CONTINUE;
}

int HierarchyExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi, BOOL suppressPrompts)
{
	// this is an object that can be called by the system with every node
	HierarchyEnumProc myScene(ei->theScene, gi->GetTime(), gi);

	myScene.DoExport(filename);
	
	return 1; // no error
}
