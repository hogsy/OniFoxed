/**********************************************************************
 *<
	FILE: 3dsexp.cpp

	DESCRIPTION: ONI file export module

	CREATED BY: Michael Evans

	HISTORY: July 1st 1997

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/


#include "ExportMain.h"
#include "ExportTools.h"

#include "Max.h"

#define TEMPLATE_DAT "c:\\Program Files\\DevStudio\\MyProjects\\BungieSource\\OniProj\\GameDataFolder\\template.dat"

#define EXPORT_VERSION "0.1"

#include <stdarg.h>


// Statics

class ModelExport : public SceneExport 
{
public:
					ModelExport()					{}
					~ModelExport()					{}
	int				ExtCount()						{ return 1; }
	const TCHAR *	Ext(int n)						{ assert(0 == n); return "mdl"; }
	const TCHAR *	LongDesc()						{ return "ONI Model File"; }
	const TCHAR *	ShortDesc()						{ return "ONI Model File"; }
	const TCHAR *	AuthorName()					{ return "Michael Evans"; }
	const TCHAR *	CopyrightMessage()				{ return "Copyright Bungie Software 1997"; }
	const TCHAR *	OtherMessage1()					{ return ""; }
	const TCHAR *	OtherMessage2()					{ return ""; }
	unsigned int	Version()						{ return 100; } // 1.00
	void			ShowAbout(HWND hWnd)			{}

	int				DoExport(const char *name,ExpInterface *ei,Interface *i,BOOL suppressPrompts=FALSE);	// Export file
};

class ModelClassDesc:public ClassDesc {
	public:
	int 			IsPublic()						{ return 1; }
	void *			Create(BOOL loading = FALSE)	{ return new ModelExport; }
	const TCHAR *	ClassName()						{ return "ONI Model Exporter"; }
	SClass_ID		SuperClassID()					{ return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return Class_ID(0xd0d00b0e, 0xbeefbabe); }
	const TCHAR* 	Category()						{ return "Scene Export";  }
};

ModelClassDesc	ModelDesc;
ClassDesc		*pModelDesc = &ModelDesc;

int ModelExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi, BOOL suppressPrompts) 
{
	IScene			*scene = ei->theScene;
	TimeValue		time = gi->GetTime();
	Interface		*theInterface = gi;

	// list of max nodes
	INode			**nodeList;
	long			count;

	// export tools scene object
	Scene			*builtScene;

	// type of nodes to export
	CountType		countType = kCountAllNodes;

	// create the lists
	nodeList = NodeList::GetNodeList(scene, time, countType);
	count = NodeCounter::CountNodes(scene, time, countType);

	// build the our scene object
	builtScene = new Scene(scene, time, theInterface);

	// iterate through our nodes
	int itr;
	for(itr = 0; itr < count; itr++)
	{
		builtScene->AddNode(nodeList[itr]);
	}

	// fix the coordinate system
	builtScene->CoordinateSystemMaxToOni();

	// convert to quads
	builtScene->FacesToQuads();
	builtScene->CompressPoints();

	// variable are now built, write to disk
	FILE *stream = fopen_safe(filename, "wb");

	if (NULL != stream) {
		builtScene->WriteToStream(stream);
		fclose(stream);
	}

	// close everything		
	delete builtScene;
	builtScene = NULL;
	free(nodeList);

	return 1; // no error
}
