/**********************************************************************
 *<
	FILE: SimpleExporter.cpp

	DESCRIPTION: ONI Simple model export module

	CREATED BY: Michael Evans

	HISTORY: Nov 9th, 1997

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#include "ExportMain.h"
#include "ExportTools.h"

#include "Max.h"

// oni files
#include "BFW_FileManager.h"
#include "BFW_Motoko.h"
#include "BFW.h"

#define EXPORT_VERSION "0.1"

#include <stdarg.h>

class SimpleExport : public SceneExport 
{
public:
					SimpleExport()					{			}
					~SimpleExport()					{			}
	int				ExtCount()						{ return 1; }			
	const TCHAR *	Ext(int n)						{ UUmAssert(0 == n); return "smp"; }	
	const TCHAR *	LongDesc()						{ return "Oni Simple Animation File";	}
	const TCHAR *	ShortDesc()						{ return "Oni Simple Animation File";	}
	const TCHAR *	AuthorName()					{ return "Michael Evans"; }
	const TCHAR *	CopyrightMessage()				{ return "Copyright Bungie Software 1997"; }
	const TCHAR *	OtherMessage1()					{ return ""; }
	const TCHAR *	OtherMessage2()					{ return ""; }
	unsigned int	Version()						{ return 100; } // 1.00
	void			ShowAbout(HWND hWnd)			{			}
	int				DoExport(const char *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts);	// Export file
};


//------------------------------------------------------

class SimpleClassDesc:public ClassDesc {
	public:
	int 			IsPublic()						{ return 1; }
	void *			Create(BOOL loading = FALSE)	{ return new SimpleExport; }
	const TCHAR *	ClassName()						{ return "ONI Simple Exporter"; }
	SClass_ID		SuperClassID()					{ return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return Class_ID(0x39872341, 0x451d3b73); }
	const TCHAR* 	Category()						{ return "Scene Export";  }
};

SimpleClassDesc	SimpleDesc;
ClassDesc			*pSimpleDesc = &SimpleDesc;

class SimpleEnumProc : public ITreeEnumProc 
{	
	public:
		SimpleEnumProc(IScene *scene, TimeValue t, Interface *i);
		~SimpleEnumProc();

		void		DoExport(const char *inFileName);
		int			callback(INode *node);

	private:	
		INode				*fBodyParts[kBodyPartCount];

		IScene				*fScene;
		TimeValue			fTime;
		Interface			*fInterface;

		long				fCount;
		long				fTotalNodes;
};

SimpleEnumProc::SimpleEnumProc(IScene *scene, TimeValue t, Interface *i) 
{
	UUtError error;

	error = UUrInitialize(UUcFalse);	// don't initialize the sytem, max will have done this already
	UUmAssert(UUcError_None == error);

	fScene = scene;
	fTime = t;
	fInterface = i;

	fCount = 0;

	UUtUns32 itr;
	for(itr = 0; itr < kBodyPartCount; itr++)
	{
		fBodyParts[itr] = NULL;
	}
}

SimpleEnumProc::~SimpleEnumProc() {
	UUtUns32 itr;

	for(itr = 0; itr < kBodyPartCount; itr++) {
		if (fBodyParts[itr] != NULL) {
		}
	}

	// needs to happen after all BFW stuff
	UUrTerminate();
}


void SimpleEnumProc::DoExport(const char *inFileName)
{
	fInterface->ProgressStart("Simple Export", TRUE, progress, 0);

	// because i am curious sometimes
	fTotalNodes = NodeCounter::CountNodes(fScene, fTime);

	// step 1 open the file
	fInterface->ProgressUpdate(0, FALSE, "opening file");
	FILE *stream = fopen_safe(inFileName, "wb");

	// could not open file so bail
	if (NULL == stream) { 
		fInterface->ProgressUpdate(0, FALSE, "finishing");
		fInterface->ProgressEnd();
	}

	// enumerate (callback is magically called back)
	fScene->EnumTree(this);

	// time stuff
	Interval	interval	= fInterface->GetAnimRange();
	TimeValue	start		= interval.Start();
	TimeValue	end			= interval.End();
	int			frameSize	= GetTicksPerFrame();
	int			numFrames	= ((end - start) / frameSize) + 1;

	// we should have all the body parts collected so write them all out to disk
	fprintf(stream, "ONI Simple Animation Export\r\n");
	fprintf(stream, "version\r\n");
	fprintf(stream, "1.00\r\n");
	fprintf(stream, "\r\n");

	fprintf(stream ,"num frames\r\n");
	fprintf(stream ,"%d\r\n", numFrames);
	fprintf(stream ,"\r\n");
	
	TimeValue	timeItr;
	UUtUns32	nodeIndexItr;

	for(timeItr = start; timeItr <= end; timeItr += frameSize) {
		Scene *pScenes[kBodyPartCount];
		UUtUns32 geometryCount = 0;

		for(nodeIndexItr = 0; nodeIndexItr < kBodyPartCount; nodeIndexItr++)
		{
			pScenes[nodeIndexItr] = new Scene(fScene, timeItr, fInterface);

			// what node
			UUmAssert(NULL != fBodyParts[nodeIndexItr]);

			// construct
			pScenes[nodeIndexItr]->AddNode(fBodyParts[nodeIndexItr]);
			pScenes[nodeIndexItr]->FacesToQuads();
			pScenes[nodeIndexItr]->CompressPoints();

			// if empty, delete
			if ((0 == pScenes[nodeIndexItr]->fNumPoints) || (0 == pScenes[nodeIndexItr]->fNumFaces)) {
				delete pScenes[nodeIndexItr];
				pScenes[nodeIndexItr] = NULL;
			}
			else {
				geometryCount++;
			}
		}

		fprintf(stream, "frame = \r\n%d\r\n", timeItr / frameSize);
		fprintf(stream, "num geometries = \r\n%d\r\n", geometryCount);
		fprintf(stream, "\r\n");

		for(nodeIndexItr = 0; nodeIndexItr < kBodyPartCount; nodeIndexItr++)
		{
			if (NULL == pScenes[nodeIndexItr]) { continue; }

			fprintf(stream, "suffix = %s\r\n\r\n", kBodySuffix[nodeIndexItr]);

			// what node
			UUmAssert(NULL != fBodyParts[nodeIndexItr]);

			// write
			pScenes[nodeIndexItr]->WriteToStream(stream, Scene::WriteOptions_NoHeader);

			// delete
			delete pScenes[nodeIndexItr];
		}
	}

	// close the stream
	fclose(stream);

	fInterface->ProgressUpdate(0, FALSE, "finishing");
	fInterface->ProgressEnd();
}

int SimpleEnumProc::callback(INode *node) 
{
	// here we are just building a list of the INodes we are going to export

	// update the progress bar
	char buf[255];
	sprintf(buf, "extracing %s", node->GetName());
	fInterface->ProgressUpdate((100 * fCount) / fTotalNodes, FALSE, buf);
	fCount++;

	// ignore biped nodes
	if (INodeIsBiped(node)) { return TREE_CONTINUE; }

	// scan for the suffix
	char *name = node->GetName();
	int index = FindSuffix(name);
	if (-1 == index) { return TREE_CONTINUE; }

	// store the inode in the list
	fBodyParts[index] = node;

	return TREE_CONTINUE;
}

int SimpleExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi, BOOL suppressPrompts) 
{
	// this is an object that can be called by the system with every node
	SimpleEnumProc myScene(ei->theScene, gi->GetTime(), gi);

	myScene.DoExport(filename);
	
	return 1; // no error
}