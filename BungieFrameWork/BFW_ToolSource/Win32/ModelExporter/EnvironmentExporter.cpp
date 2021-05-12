/**********************************************************************
 *<
	FILE: EnvironmentExporter.cpp

	DESCRIPTION: ONI enivornment export module

	CREATED BY: Michael Evans

	HISTORY: Dec 10, 1997

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/


#include "ExportMain.h"
#include "ExportTools.h"
#include "EnvFileFormat.h"

#include "Max.h"

#include <stdarg.h>

static char *GetLocalFileName(char *filename)
{
	char *output = filename;
	assert(NULL != filename);

	for(; *filename != '\0'; filename++)
	{
		if ('\\' == *filename)
		{
			output = filename + 1;
		}
	}

	return output;
}

//------------------------------------------------------

class EnvironmentExport : public SceneExport 
{
public:
	int				ExtCount()						{ return 1; }
	const TCHAR *	Ext(int n)						{ assert(0 == n); return "env"; }
	const TCHAR *	LongDesc()						{ return "Oni Environment File"; }
	const TCHAR *	ShortDesc()						{ return "Oni Emvironment File"; }
	const TCHAR *	AuthorName()					{ return "Michael Evans"; }
	const TCHAR *	CopyrightMessage()				{ return "Copyright Bungie Software 1997"; }
	const TCHAR *	OtherMessage1()					{ return ""; }
	const TCHAR *	OtherMessage2()					{ return ""; }
	unsigned int	Version()						{ return 100; } // 1.00
	void			ShowAbout(HWND hWnd)			{}

	int				DoExport(const char *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts); // Export file
};

class EnvironmentClassDesc:public ClassDesc {
	public:
	int 			IsPublic()						{ return 1; }
	void *			Create(BOOL loading = FALSE)	{ return new EnvironmentExport; }
	const TCHAR *	ClassName()						{ return "ONI Environment Exporter"; }
	SClass_ID		SuperClassID()					{ return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return Class_ID(0xa6546297, 0xa2f9d3cc); }
	const TCHAR* 	Category()						{ return "Scene Export";  }
};

EnvironmentClassDesc	EnvironmentDesc;
ClassDesc				*pEnvironmentDesc = &EnvironmentDesc;


bool has_object_undesrcore_prefix(char *string)
{
	char *object_underscore = "object_";
	bool match;

	match = 0 == strncmp(string, object_underscore, strlen(object_underscore));

	return match;
}

// finds the center of an INode's local space points from its constructed bounding box

int EnvironmentExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi, BOOL suppressPrompts) 
{
	INode *node;
	TimeValue	time   = gi->GetTime();
	Scene scene(ei->theScene, time, gi);

	gi->ProgressStart("Environment Export", TRUE, progress, 0);

	// get our list of nodes
	INode **nodeList = NodeList::GetNodeList(ei->theScene, time, kCountNormalNodes);
	int count = NodeCounter::CountNodes(ei->theScene, time, kCountNormalNodes);
	char **object_names = (char **) malloc(sizeof(char *) * count);
	int itr;

	MXtHeader header;

	// open our file
	gi->ProgressUpdate(0, FALSE, "opening file");
	FILE *stream = fopen_safe(filename, "wb");

	// could not open file so bail
	if (NULL == stream) { goto DoExportBail; }

	strcpy(header.stringENVF, "ENVF");
	header.version = 0;
	header.numNodes = count;
	header.time = time;
	header.nodes = NULL;

	fwrite(&header, sizeof(header), 1, stream);
	
	// iterate through our nodes
	for(itr = 0; itr < count; itr++)
	{
		int progressAmt = (100 * itr) / count;

		Scene scene(ei->theScene, time, gi);
		node = nodeList[itr];
		char *name = node->GetName();
		bool duplicateObject;
		Point3 center;

		object_names[itr] = name;

		gi->ProgressUpdate(progressAmt, FALSE, name);

		TSTR user_prop_buffer;
		node->GetUserPropBuffer(user_prop_buffer);

		// ============== construct the internal geometry ==============
		unsigned long addOptions = 0;
		addOptions |= Scene::AddOptions_NoMatrix;

		scene.AddNode(node, addOptions);
		unsigned long checksum = scene.Checksum();

		center = scene.FindCenter();
		scene.OffsetPoints(-center);
		scene.FindCenter();

		// ============== matrix ==============
		Matrix3 tm;
		Matrix3 fix = Scene::CoordinateFix();

		tm = GetObjectTM(node, time);	// object is total world space
		tm = TransMatrix(center) * tm;
		tm = tm * fix;
		
		duplicateObject = false;

		if (has_object_undesrcore_prefix(name)) {
			int duplicateItr;

			for(duplicateItr = 0; duplicateItr < itr; duplicateItr++) {
				if (0 == strcmp(object_names[duplicateItr], name)) {
					duplicateObject = true;
					break;
				}
			}
		}

		if (duplicateObject) {
			// delete all the geometry
		}

		// ============== geometry ==============
		scene.FacesToQuads();

		scene.WriteToStream(stream, node->GetName(), node->GetParentNode()->GetName(), tm);

		scene.Clear();

		// ============== match ==============
	}

	if (NULL != nodeList)
	{
		free(nodeList);
	}

	scene.Clear();

	gi->ProgressUpdate(100, FALSE, "closing file");
	fclose(stream);

	gi->ProgressEnd();

DoExportBail:

	return 1; // no error
}
