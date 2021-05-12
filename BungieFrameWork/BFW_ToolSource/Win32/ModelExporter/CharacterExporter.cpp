/**********************************************************************
 *<
	FILE: CharacterExporter.cpp

	DESCRIPTION: ONI character model export module

	CREATED BY: Michael Evans

	HISTORY: October 3rd, 1997

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

#define EXPORT_VERSION "0.1"

#include <stdarg.h>

/*

We scan for these suffixes in order to find the body parts that we want.

*/


class CharacterExport : public SceneExport 
{
public:
					CharacterExport()				{}
					~CharacterExport()				{}
	int				ExtCount()						{ return 1; }
	const TCHAR *	Ext(int n)						{ UUmAssert(0 == n); return "chr"; }
	const TCHAR *	LongDesc()						{ return "ONI Character File"; }
	const TCHAR *	ShortDesc()						{ return "ONI Character File"; }
	const TCHAR *	AuthorName()					{ return "Michael Evans"; }
	const TCHAR *	CopyrightMessage()				{ return "Copyright Bungie Software 1997"; }
	const TCHAR *	OtherMessage1()					{ return ""; }
	const TCHAR *	OtherMessage2()					{ return ""; }
	unsigned int	Version()						{ return 100; } // 1.00
	void			ShowAbout(HWND hWnd)			{}

	int				DoExport(const char *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE);
};


//------------------------------------------------------

class CharacterClassDesc:public ClassDesc {
	public:
	int 			IsPublic()						{ return 1; }
	void *			Create(BOOL loading = FALSE)	{ return new CharacterExport; }
	const TCHAR *	ClassName()						{ return "ONI Character Exporter"; }
	SClass_ID		SuperClassID()					{ return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return Class_ID(0x8373de84, 0xd8e2ab39); }
	const TCHAR* 	Category()						{ return "Scene Export";  }
};

CharacterClassDesc	CharacterDesc;
ClassDesc			*pCharacterDesc = &CharacterDesc;

/*
 *
 * This class dumps everything to a single file.
 *
 */

typedef struct {
	Point3	distance;
	bool	marked;
} Link;

class CharacterEnumProc : public ITreeEnumProc 
{	
	public:
		CharacterEnumProc(IScene *scene, TimeValue t, Interface *i);
		~CharacterEnumProc();

		void		DoExport(const char *inFileName);
		int			callback(INode *node);

	private:	
		Link				fLinks[kBodyPartCount];
		Scene				*fBodyParts[kBodyPartCount];

		IScene				*fScene;
		TimeValue			fTime;
		Interface			*fInterface;

		long				fCount;
		long				fTotalNodes;
		FILE				*fStream;
};

CharacterEnumProc::CharacterEnumProc(IScene *scene, TimeValue t, Interface *i) 
{
	UUtError error;

	error = UUrInitialize(UUcFalse);	// don't initialize the sytem, max will have done this already
	UUmAssert(UUcError_None == error);

	fScene = scene;
	fTime = t;
	fInterface = i;

	fCount = 0;
	fStream = NULL;

	UUtUns32 itr;
	for(itr = 0; itr < kBodyPartCount; itr++)
	{
		fBodyParts[itr] = NULL;
	}
}

CharacterEnumProc::~CharacterEnumProc() {
	UUtUns32 itr;

	for(itr = 0; itr < kBodyPartCount; itr++) {
		if (fBodyParts[itr] != NULL) {
			delete fBodyParts[itr];
		}
	}

	// needs to happen after all BFW stuff
	UUrTerminate();
}


void CharacterEnumProc::DoExport(const char *inFileName)
{
	fInterface->ProgressStart("Character Export", TRUE, progress, 0);

	// because i am curious sometimes
	fTotalNodes = NodeCounter::CountNodes(fScene, fTime);

	// step 1 open the file
	fInterface->ProgressUpdate(0, FALSE, "opening file");
	fStream = fopen_safe(inFileName, "wb");

	// could not open file so bail
	if (NULL == fStream) { goto DoExportBail; }

	// enumerate (callback is magically called back)
	fScene->EnumTree(this);

	// we should have all the body parts collected so write them all out to disk
	fprintf(fStream, "ONI Character Export\r\n");
	fprintf(fStream, "version\r\n");
	fprintf(fStream, "1.00\r\n");
	fprintf(fStream, "\r\n");

	fprintf(fStream, "Links:\r\n");
	fprintf(fStream, "\r\n");

	UUtUns32 itr;

	for(itr = 0; itr < kBodyPartCount; itr++)
	{
		fprintf(fStream, "%s\r\n", kBodySuffix[itr]);

		if (fLinks[itr].marked) {
			fprintf(fStream, "%f\r\n", fLinks[itr].distance.x);
			fprintf(fStream, "%f\r\n", fLinks[itr].distance.y);
			fprintf(fStream, "%f\r\n", fLinks[itr].distance.z);
		}
		else {
			fprintf(fStream, "no link found\r\n");
		}

		fprintf(fStream, "\r\n");
	}

	fprintf(fStream, "Body Parts:\r\n");
	fprintf(fStream, "\r\n");

	for(itr = 0; itr < kBodyPartCount; itr++)
	{
		fprintf(fStream, "suffix\r\n");
		fprintf(fStream, "%s\r\n",kBodySuffix[itr]);
		
		if (NULL == fBodyParts[itr]) {
			char msg_buf[255];

			sprintf(msg_buf, "could not find a body part for %s", kBodySuffix[itr]);
			MessageBox(NULL, msg_buf, "warning", MB_OK);
		
			// make an empty part
			fBodyParts[itr] = new Scene(fScene, fTime, fInterface);
		}

		fBodyParts[itr]->WritePoints(fStream);
		fBodyParts[itr]->WriteFaces(fStream);
		fprintf(fStream, "\r\n");
	}

	// close the stream
	fclose(fStream);

DoExportBail:  	// close everything
	fInterface->ProgressUpdate(0, FALSE, "finishing");
	fInterface->ProgressEnd();
}

int CharacterEnumProc::callback(INode *node) 
{
	// update the progress bar
	char buf[255];
	sprintf(buf, "extracing %s", node->GetName());
	fInterface->ProgressUpdate((100 * fCount) / fTotalNodes, FALSE, buf);

	// go the next object
	fCount++;

	// 0. scan for the suffix
	// 1. remove bipeds (but store the link value)
	// 2. extract bone length and normalize out rotation
	// 3. remove some rotation and write to disk
	// 4. add this node

	// 0. scan for the suffix
	char *name = node->GetName();
	int index = FindSuffix(name);
	if (-1 == index) { return TREE_CONTINUE; }

	// 1. remove bipeds (but store the link value)
	if (INodeIsBiped(node)) {
		// our link is the distance from our parent to us
		Matrix3 scaleTM = GetNodeTM(node, 0);	// node TM for scale
		Matrix3 transTM	= GetLocalTM(node, 0);	// local only for translation

		// turn scale TM into scale only
		scaleTM.NoTrans();
		scaleTM.NoRot();

		// calculate the distance and mark as true
		fLinks[index].distance	= transTM.GetTrans() * scaleTM;
		fLinks[index].marked	= true;

		return TREE_CONTINUE; 
	}

	// add this geometry to the proper place in the node array
	UUmAssert(NULL == fBodyParts[index]);
	fBodyParts[index] = new Scene(fScene, fTime, fInterface);

	// add this node, convert to quads and compress
	fBodyParts[index]->AddNode(node, Scene::AddOptions_IgnoreNodeRTTM);
	fBodyParts[index]->FacesToQuads();
	fBodyParts[index]->CompressPoints();
	
	return TREE_CONTINUE;
}

int CharacterExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi, BOOL suppressPrompts)
{
	// this is an object that can be called by the system with every node
	CharacterEnumProc myScene(ei->theScene, gi->GetTime(), gi);

	myScene.DoExport(filename);
	
	return 1; // no error
}
