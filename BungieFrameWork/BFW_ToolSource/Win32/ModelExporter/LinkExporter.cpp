/**********************************************************************
 *
	FILE: LinkExporter.cpp

  DESCRIPTION: ONI Link model export module

	CREATED BY: Michael Evans

	HISTORY: October 3rd, 1997

 *	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#include "ExportMain.h"
#include "ExportTools.h"

// 3D Studio Max Includes
#include "Max.h"
#include "decomp.h"

// oni files
#include "BFW_TemplateManager.h"
#include "BFW_FileManager.h"
#include "BFW_Motoko.h"
#include "BFW.h"

#define EXPORT_VERSION "0.1"

#include <stdarg.h>

class LinkExport : public SceneExport 
{
public:
					LinkExport()					{}
					~LinkExport()					{}
	int				ExtCount()						{ return 1; }
	const TCHAR *	Ext(int n)						{ UUmAssert(0 == n); return "tlk"; }
	const TCHAR *	LongDesc()						{ return "Oni Link File"; }
	const TCHAR *	ShortDesc()						{ return "Oni Link File"; }
	const TCHAR *	AuthorName()					{ return "Michael Evans"; }
	const TCHAR *	CopyrightMessage()				{ return "Copyright Bungie Software 1997"; }
	const TCHAR *	OtherMessage1()					{ return ""; }
	const TCHAR *	OtherMessage2()					{ return ""; }
	unsigned int	Version()						{ return 100; } // 1.00
	void			ShowAbout(HWND hWnd)			{}

	int				DoExport(const char *name,ExpInterface *ei,Interface *i,BOOL suppressPrompts=FALSE);	// Export file
};


class IKAnimExport : public SceneExport 
{
public:
					IKAnimExport()					{}
					~IKAnimExport()					{}
	int				ExtCount()						{ return 1; }
	const TCHAR *	Ext(int n)						{ UUmAssert(0 == n); return "ika"; }
	const TCHAR *	LongDesc()						{ return "Oni IK Anim File"; }
	const TCHAR *	ShortDesc()						{ return "Oni IK Anim File"; }
	const TCHAR *	AuthorName()					{ return "Michael Evans"; }
	const TCHAR *	CopyrightMessage()				{ return "Copyright Bungie Software 1997"; }
	const TCHAR *	OtherMessage1()					{ return ""; }
	const TCHAR *	OtherMessage2()					{ return ""; }
	unsigned int	Version()						{ return 100; } // 1.00
	void			ShowAbout(HWND hWnd)			{}

	int				DoExport(const char *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE);	// Export file
};

//------------------------------------------------------

class LinkClassDesc:public ClassDesc {
	public:
	int 			IsPublic()						{ return 1; }
	void *			Create(BOOL loading = FALSE)	{ return new LinkExport; }
	const TCHAR *	ClassName()						{ return "ONI Link Exporter"; }
	SClass_ID		SuperClassID()					{ return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return Class_ID(0x5a7f251b, 0x02d7ff86); }
	const TCHAR* 	Category()						{ return "Scene Export";  }
};

LinkClassDesc	LinkDesc;
ClassDesc		*pLinkDesc = &LinkDesc;

class IKAnimClassDesc:public ClassDesc {
	public:
	int 			IsPublic()						{ return 1; }
	void *			Create(BOOL loading = FALSE)	{ return new IKAnimExport; }
	const TCHAR *	ClassName()						{ return "ONI Link Exporter"; }
	SClass_ID		SuperClassID()					{ return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return Class_ID(0x38743624, 0x23929312); }
	const TCHAR* 	Category()						{ return "Scene Export";  }
};

IKAnimClassDesc	IKAnimDesc;
ClassDesc		*pIKAnimDesc = &IKAnimDesc;

typedef struct {
	INode				*node;
	Scene				*scene;

	Matrix3				matrix;

	UUtUns32			parentIndex;

	UUtUns32			childIndex;
	UUtUns32			siblingIndex;
} LinkNode;


/*
 *
 * This class dumps everything to a single file.
 *
 */

class IKEnumProc : public ITreeEnumProc 
{	
	public:
		IKEnumProc(IScene *scene, TimeValue t, Interface *i);
		~IKEnumProc();

		// constructs and trims an animation hierarchy
		void		BuildHierarchy(void);

		// writes out the built format as a link file
		void		DoWriteLinkFile(const char *inFileName);
		void		DoWriteAnimFile(const char *inFileName);

		int			callback(INode *node);

	private:	
		IScene				*fScene;
		TimeValue			fTime;
		Interface			*fInterface;

	// this section is the datastructure we built up
	// that define this kinematic hierarchy

	private:
		LinkNode			*fNodes;
		UUtUns32			fNumNodes;

		void BuildParents(void);		// builds the parentIndex goo
		void BuildMatricies(void);
		void BuildScenes(void);

		void BuildExtraRelatives(void);	// builds children and siblings

		bool IsLeafNode(UUtUns32 index);
		void RemoveNode(UUtUns32 index);
};

Matrix3 ComputeDeltaMatrix(INode *node, TimeValue oldTime, TimeValue newTime)
{
	Matrix3		oldTM	= node->GetNodeTM(oldTime);
	Matrix3		newTM	= node->GetNodeTM(newTime);
	Matrix3		deltaTM	= newTM * Inverse(oldTM);

	// for debugging lets decompose it
	AffineParts	parts;
	decomp_affine(deltaTM, &parts);

	return deltaTM;
}

IKEnumProc::IKEnumProc(IScene *scene, TimeValue t, Interface *i) 
{
	UUtError error;

	error = UUrInitialize(UUcFalse);	// don't initialize the sytem, max will have done this already
	UUmAssert(UUcError_None == error);

	fScene		= scene;
	fTime		= t;
	fInterface	= i;

	fNodes		= NULL;

	fNumNodes	= 0;
}

IKEnumProc::~IKEnumProc() 
{
	UUtUns32 itr;

	for(itr = 0; itr < fNumNodes; itr++) {
		UUmAssert(NULL != fNodes);

		delete fNodes[itr].scene;
		fNodes[itr].scene = NULL;
	}

	if (fNumNodes > 0) {
		UUrMemory_Block_Delete(fNodes);
	}

	UUrTerminate();
}

bool IKEnumProc::IsLeafNode(UUtUns32 index)
{
	UUtUns32 itr;

	for(itr = 0; itr < fNumNodes; itr++)
	{
		if (index == fNodes[itr].parentIndex) {
			return false;
		}
	}

	return true;
}

void IKEnumProc::RemoveNode(UUtUns32 index)
{
	// for debugging here is the node and the name
	INode *node	= fNodes[index].node;
	char *name	= fNodes[index].node->GetName();

	UUtUns32 itr;

	// step 0 toast my scene object
	// step 1 remember my parent
	// step 2 move the list down
	// step 3 patch up all my children through to my parent
	// step 4 patch up all parent references

	delete fNodes[index].scene;
	fNodes[index].scene = NULL;

	UUtUns32 deletedParentIndex = fNodes[index].parentIndex;

	for(itr = index; itr < (fNumNodes - 1); itr++)
	{
		fNodes[itr] = fNodes[itr + 1];
	}

	fNumNodes -= 1;

	for(itr = 0; itr < fNumNodes; itr++)
	{
		LinkNode *thisNode = fNodes + itr;

		// if this was a child of the deleted node, pass index through
		if (thisNode->parentIndex == index) {
			thisNode->parentIndex = deletedParentIndex;
		}

		// points to root, ignore
		if (thisNode->parentIndex == INVALID_INDEX) {
			continue;
		}

		// before the shift, ignore
		if (thisNode->parentIndex < index) {
			continue;
		}

		// slide it down
		thisNode->parentIndex -= 1;
	}
}

void IKEnumProc::BuildParents(void)
{
	UUtUns32 childItr, parentItr;

	for(childItr = 0; childItr < fNumNodes; childItr++)
	{
		const INode *parentNode = 	fNodes[childItr].node->GetParentNode();
	
		// assume no parent
		fNodes[childItr].parentIndex = 0xffffffff;

		// search for parent, halt when found
		for(parentItr = 0; parentItr < fNumNodes; parentItr++)
		{	
			if (parentNode == fNodes[parentItr].node) {
				fNodes[childItr].parentIndex = parentItr;
				break;
			}
		}
	}
}

void IKEnumProc::BuildExtraRelatives(void)
{
	UUtUns32 itr;

	for(itr = 0; itr < fNumNodes; itr++)
	{
		fNodes[itr].childIndex = 0;
		fNodes[itr].siblingIndex = 0;
	}

	// build our children
	for(itr = 0; itr < fNumNodes; itr++)
	{
		UUtUns32 childItr;
		UUtUns32 lastChild;

		lastChild = 0;

		for(childItr = 0; childItr < fNumNodes; childItr++)
		{
			// if this node is our child
			if (itr == fNodes[childItr].parentIndex)
			{
				if (0 == lastChild) 
				{
					// first one is the one the parent points to
					fNodes[itr].childIndex = childItr;
				}
				else
				{
					// the rest get pointed to by the previous node
					fNodes[lastChild].siblingIndex = childItr;
				}

				// remember it to be marked if we find another
				lastChild = childItr;			
			}
		}
	}
}


void IKEnumProc::BuildMatricies(void)
{
	UUtUns32 itr;
	
	for(itr = 0; itr < fNumNodes; itr++)
	{
		INode *node = fNodes[itr].node;
		bool root = 0xffffffff == fNodes[itr].parentIndex;

		// construct and add the matrix
		Matrix3 parentTM	= node->GetParentTM(fTime);
		Matrix3 localTM		= GetLocalTM(node, fTime);

		Matrix3 noScaleLocalTM				= StripScale(localTM);
		Matrix3 scaleOnlyParentTM			= StripRotTrans(parentTM);
		Matrix3 localPlusScaleTM			= noScaleLocalTM * scaleOnlyParentTM;
		Matrix3 strippedLocalPlusScaleTM	= StripScale(localPlusScaleTM);

		if (root) {
			// apply the coordinate system fix

			// x = x
			// y = z
			// z = -y

			// + 0 0 0
			// 0 0 - 0
			// 0 + 0 0

			Point3 row0(1,0,0);
			Point3 row1(0,1,0);
			Point3 row2(0,0,1);
			Point3 row3(0,0,0);

			Matrix3 fixer;

			fixer.SetRow(0, row0);
			fixer.SetRow(1, row1);
			fixer.SetRow(2, row2);
			fixer.SetRow(3, row3);	

			// pre multiply
			strippedLocalPlusScaleTM = RotateYMatrix(0.75f * (2 * M3cPi)) * fixer *  strippedLocalPlusScaleTM;
		}

		fNodes[itr].matrix = strippedLocalPlusScaleTM;
	}
}


void IKEnumProc::BuildScenes(void)
{
	UUtUns32 itr;

	for(itr = 0; itr < fNumNodes; itr++)
	{
		INode *node = fNodes[itr].node;
		bool root = 0xffffffff == fNodes[itr].parentIndex;
		char *name = node->GetName();
	
		// add the scene
		fNodes[itr].scene = new Scene(fScene, fTime, fInterface);

		UUtUns32 addOptions = 0;
		
		addOptions |= Scene::AddOptions_IgnoreNodeRTTM;
		addOptions |= Scene::AddOptions_IgnoredBiped;

		fNodes[itr].scene->AddNode(node, addOptions);
	}
}

void IKEnumProc::BuildHierarchy(void)
{
	// enumerate (callback is magically called back)
	fScene->EnumTree(this);

	// step 1 fill in the nodes
	BuildParents();
	BuildMatricies();
	BuildScenes();

		// step 2 construct all the scene object

	// step 3 compress
	// until no progress loop through the nodes 
	//
	// if biped node skip
	// if !leaf node skip
	//
	// if empty delete
	// else add to parent (apply matrix first)

	// write to disk

	bool progress;

	do
	{
		UUrMemory_Block_VerifyList();

		progress = false;

		UUtUns32 itr;
		
		for(itr = 0; itr < fNumNodes; itr++)
		{
			char *name		= fNodes[itr].node->GetName();
			bool biped		= INodeIsBiped(fNodes[itr].node);
			bool leaf		= IsLeafNode(itr);
			bool footsteps	= HasSuffix(name, "Footsteps");

			// strip off the footsteps node
			if (footsteps) {
				UUmAssert(biped);
				UUmAssert(leaf);
			}
		
			if ((biped) && (!footsteps)) { continue; }
			if (!leaf) { continue; }
			
			bool root = 0xffffffff == fNodes[itr].parentIndex;

			if ((!root) && (!fNodes[itr].scene->IsEmpty())) {
				UUrMemory_Block_VerifyList();

				LinkNode *parent = fNodes + fNodes[itr].parentIndex;

				// multiply through by my matrix
				fNodes[itr].scene->Multiply(fNodes[itr].matrix);
				
				// add this node to its parent
				parent->scene->AddScene(fNodes[itr].scene);
			}

			// delete this baby
			RemoveNode(itr);

			progress = true;
			break;
		}
	} while(progress);

	// final step is to strip off the root biped node which is extra
	// currently this will only work with one biped per scene
	UUtUns32 itr;

	for(itr = 0; itr < fNumNodes; itr++) {
		char *name		= fNodes[itr].node->GetName();
		bool biped		= INodeIsBiped(fNodes[itr].node);
		bool root		= 0xffffffff == fNodes[itr].parentIndex;

		if (!biped) { continue; }
		if (!root) { continue; }

		// premultiply all my children by my matrix
		UUtUns32 child;
		for(child = 0; child < fNumNodes; child++) {
			if (fNodes[child].parentIndex != itr) { continue; }

			fNodes[child].matrix = fNodes[child].matrix * fNodes[itr].matrix;
		}

		RemoveNode(itr);
		break;
	}

	// turn into quads
	for(itr = 0; itr < fNumNodes; itr++)
	{
		fNodes[itr].scene->FacesToQuads();
	}

	// build the extra relatives last
	BuildExtraRelatives();
}

void IKEnumProc::DoWriteLinkFile(const char *inFileName)
{
	fInterface->ProgressStart("Link Export", TRUE, progress, 0);

	// step 1 open the file
	fInterface->ProgressUpdate(0, FALSE, "opening file");
	FILE *stream = fopen_safe(inFileName, "wb");

	// could not open file so bail
	if (NULL == stream) {
		fInterface->ProgressUpdate(0, FALSE, "finishing");
		fInterface->ProgressEnd();
	}

	char *version	= "1.00";
	char *timestamp = __TIMESTAMP__;

	fprintf(stream, "ONI IK Link File\r\n");
	fprintf(stream, "version\r\n");
	fprintf(stream, "%s\r\n",version);
	fprintf(stream, "timestamp\r\n");
	fprintf(stream, "%s\r\n",timestamp);
	fprintf(stream, "node count =\r\n");
	fprintf(stream, "%d\r\n", fNumNodes);

	UUtUns32 itr;

	for(itr = 0; itr < fNumNodes; itr++)
	{
		// name
		// parent index
		// local tm

		// points
		// faces
		
		char *name = fNodes[itr].node->GetName();

		fprintf(stream, "name\r\n%s\r\n", name);
		fprintf(stream, "index\r\n%d\r\n", itr);
		fprintf(stream, "parent\r\n%d\r\n", fNodes[itr].parentIndex);
		fprintf(stream, "child\r\n%d\r\n", fNodes[itr].childIndex);
		fprintf(stream, "sibling\r\n%d\r\n", fNodes[itr].siblingIndex);
	
		fprintf(stream, "local matrix\r\n");
		PrintDecomposedMatrix(stream, fNodes[itr].matrix);
	
		fNodes[itr].scene->WritePoints(stream);
		fNodes[itr].scene->WriteFaces(stream, Scene::WriteOptions_TrianglesOnly);
		fNodes[itr].scene->WriteFaces(stream, Scene::WriteOptions_QuadsOnly);
	}

	// close the stream
	fclose(stream);

	fInterface->ProgressUpdate(0, FALSE, "finishing");
	fInterface->ProgressEnd();
}

void IKEnumProc::DoWriteAnimFile(const char *inFileName)
{
	fInterface->ProgressStart("Link Export", TRUE, progress, 0);

	// step 1 open the file
	fInterface->ProgressUpdate(0, FALSE, "opening file");
	FILE *stream = fopen_safe(inFileName, "wb");

	// could not open file so bail
	if (NULL == stream) {
		fInterface->ProgressUpdate(0, FALSE, "finishing");
		fInterface->ProgressEnd();
	}

	char *version	= "1.00";
	char *timestamp = __TIMESTAMP__;

	// time stuff
	Interval	interval	= fInterface->GetAnimRange();
	TimeValue	start		= interval.Start();
	TimeValue	end			= interval.End();
	int			frameSize	= GetTicksPerFrame();
	int			numFrames	= ((end - start) / frameSize) + 1;

	fprintf(stream, "ONI IK Anim File\r\n");

	fprintf(stream, "version\r\n");
	fprintf(stream, "%s\r\n",version);

	fprintf(stream, "timestamp\r\n");
	fprintf(stream, "%s\r\n",timestamp);

	fprintf(stream, "node count =\r\n");
	fprintf(stream, "%d\r\n", fNumNodes)
		;
	fprintf(stream, "frame count=\r\n");
	fprintf(stream, "%d\r\n",numFrames);

	fprintf(stream, "start frame\r\n");
	fprintf(stream, "%d\r\n",start / frameSize);

	fprintf(stream, "end frame\r\n");
	fprintf(stream, "%d\r\n",end / frameSize);

	fprintf(stream, "ticks per frame\r\n");
	fprintf(stream, "%d\r\n",frameSize);

	fprintf(stream, "start time\r\n");
	fprintf(stream, "%d\r\n",start);

	fprintf(stream, "end time\r\n");
	fprintf(stream, "%d\r\n",end);

	PrintLineBreak(stream);

	TimeValue timeItr;

	for(timeItr = start; timeItr <= end; timeItr += frameSize) {
		fprintf(stream, "time = \r\n%d\r\n", timeItr);
		fprintf(stream, "frame = \r\n%d\r\n", timeItr / frameSize);
		fprintf(stream, "\r\n");

		UUtUns32 itr;
		for(itr = 0; itr < fNumNodes; itr++)
		{
			char *name = fNodes[itr].node->GetName();

			fprintf(stream, "name\r\n%s\r\n", name);
			fprintf(stream, "index\r\n%d\r\n", itr);

			// compute the current matrix
			Matrix3 time0Matrix = fNodes[itr].matrix;
			Matrix3 deltaMatrix = ComputeDeltaMatrix(fNodes[itr].node, start, timeItr);
			Matrix3 matrix		= deltaMatrix * time0Matrix;

			AffineParts parts;
			decomp_affine(matrix, &parts);

			fprintf(stream, "local rotation\r\n");
			PrintQuaternion(stream, parts.q);

			fprintf(stream, "\r\n");
		}
	}

	// close the stream
	fclose(stream);

	fInterface->ProgressUpdate(0, FALSE, "finishing");
	fInterface->ProgressEnd();
}


int IKEnumProc::callback(INode *node) 
{
	// step 1 grow the list of fLinks and fBodyParts
	// step 2 construct the links and the nodes

	fNodes = (LinkNode *) UUrMemory_Block_Realloc(fNodes, (fNumNodes + 1) * sizeof(LinkNode));
	fNodes[fNumNodes].node = node;

	fNumNodes += 1;

	return TREE_CONTINUE;
}

int LinkExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *i, BOOL suppressPrompts)
{
	// this is an object that can be called by the system with every node
	IKEnumProc myScene(ei->theScene, i->GetTime(), i);

	myScene.BuildHierarchy();
	myScene.DoWriteLinkFile(filename);
	
	return 1; // no error
}

int IKAnimExport::DoExport(const char *filename,ExpInterface *ei,Interface *i,BOOL suppressPrompts)
{
	// this is an object that can be called by the system with every node
	IKEnumProc myScene(ei->theScene, i->GetTime(), i);

	myScene.BuildHierarchy();
	myScene.DoWriteAnimFile(filename);
	
	return 1; // no error
}

