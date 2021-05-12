/**********************************************************************
 *<
	FILE: OpacityExporter.cpp

	DESCRIPTION: ONI file export module

	CREATED BY: Michael Evans

	HISTORY: October 2nd 1997

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

#define TEMPLATE_DAT "c:\\Program Files\\DevStudio\\MyProjects\\BungieSource\\OniProj\\GameDataFolder\\template.dat"

#define EXPORT_VERSION "0.1"

#include <stdarg.h>

static float maximum(float a, float b)
{
	if (a > b) {
		return a;
	} 
	else {
		return b;
	}
}



//------------------------------------------------------

class OpacityExport : public SceneExport 
{
public:
					OpacityExport()					{}
					~OpacityExport()				{}
	int				ExtCount()						{ return 1; }
	const TCHAR *	Ext(int n)						{ UUmAssert(0 == n); return "opc"; }
	const TCHAR *	LongDesc()						{ return "Oni Opacity File"; }
	const TCHAR *	ShortDesc()						{ return "Oni Opacity File"; }
	const TCHAR *	AuthorName()					{ return "Michael Evans"; }
	const TCHAR *	CopyrightMessage()				{ return "Copyright Bungie Software 1997"; }
	const TCHAR *	OtherMessage1()					{ return ""; }
	const TCHAR *	OtherMessage2()					{ return ""; }
	unsigned int	Version()						{ return 100; } // 1.00
	void			ShowAbout(HWND hWnd)			{}

	int				DoExport(const char *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts); // Export file
};

class OpacityClassDesc:public ClassDesc {
	public:
	int 			IsPublic()						{ return 1; }
	void *			Create(BOOL loading = FALSE)	{ return new OpacityExport; }
	const TCHAR *	ClassName()						{ return "ONI Opacity Exporter"; }
	SClass_ID		SuperClassID()					{ return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return Class_ID(0x34946297, 0x123963df); }
	const TCHAR* 	Category()						{ return "Scene Export";  }
};

OpacityClassDesc	OpacityDesc;
ClassDesc			*pOpacityDesc = &OpacityDesc;

/*
 *
 * This class dumps everything to a single file.
 *
 */

class OpacityEnumProc : public ITreeEnumProc 
{	
	public:
		OpacityEnumProc(IScene *scene, TimeValue t, Interface *i);
		~OpacityEnumProc();

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

OpacityEnumProc::OpacityEnumProc(IScene *scene, TimeValue t, Interface *i) 
{
	fScene = scene;
	fTime = t;
	fInterface = i;

	fCount = 0;
	fBuiltScene = NULL;
}



void OpacityEnumProc::DoExport(const char *inFileName)
{
	fInterface->ProgressStart("Opacity Export", TRUE, progress, 0);

	fBuiltScene = new Scene(fScene, fTime, fInterface);

	UUtError	error;

	error = UUrInitialize(UUcFalse);	// don't initialize the sytem, max will have done this already
	UUmAssert(UUcError_None == error);

	// because i am curious sometimes
	fTotalNodes = NodeCounter::CountNodes(fScene, fTime);

	// enumerate (callback is magically called back)
	fScene->EnumTree(this);

	// fix the coordinate system
	fBuiltScene->CoordinateSystemMaxToOni();

	// convert to quads and round
	fBuiltScene->FacesToQuads(Scene::QuadOptions_Exact);
	fBuiltScene->RoundPoints();
	fBuiltScene->ClearUV();
	fBuiltScene->CompressPoints();

	// variable are now built, write to disk
	FILE *stream = fopen_safe(inFileName, "wb");

	if (NULL != stream) {
		fprintf(stream, "opacity dump\r\n");
		fprintf(stream, "version\r\n");
		fprintf(stream, "100\r\n");

		UUtUns32 options = 0;
		
		options |= Scene::WriteOptions_NoVertexNormals;
		options |= Scene::WriteOptions_NoVertexUV;
		options |= Scene::WriteOptions_NoFaceNormals;

		fBuiltScene->WritePoints(stream, options);
		fBuiltScene->WriteFaces(stream, options);

		fclose(stream);
	}

	// close everything		
	fInterface->ProgressUpdate(0, FALSE, "finishing");
	delete fBuiltScene;
	fBuiltScene = NULL;
	UUrTerminate();

	fInterface->ProgressEnd();
}

OpacityEnumProc::~OpacityEnumProc() {
	UUmAssert(NULL == fBuiltScene);
}

int OpacityEnumProc::callback(INode *node) 
{
	// update the progress bar
	char buf[255];
	sprintf(buf, "extracing %s", node->GetName());
	fInterface->ProgressUpdate((100 * fCount) / fTotalNodes, FALSE, buf);

	// add this node
	fBuiltScene->AddNode(node);

	// go the next object
	fCount++;

	return TREE_CONTINUE;
}

int OpacityExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi, BOOL suppressPrompts) 
{
	// this is an object that can be called by the system with every node
	OpacityEnumProc myScene(ei->theScene, gi->GetTime(), gi);

	myScene.DoExport(filename);
	
	return 1; // no error
}
