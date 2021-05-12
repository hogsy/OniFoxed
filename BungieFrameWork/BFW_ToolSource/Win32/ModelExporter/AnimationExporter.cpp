/**********************************************************************
 *
	FILE: AnimExporter.cpp

	DESCRIPTION: ONI Anim model export module

	CREATED BY: Michael Evans

	HISTORY: October 3rd, 1997

 *	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#include "ExportMain.h"
#include "ExportTools.h"

// 3D Studio Max Includes
#include "Max.h"
#include "decomp.h"

#define EXPORT_VERSION "0.1"

#include <stdarg.h>

float xytoradians(float x, float y);
Matrix3 MatrixToYRotateMatrix(const Matrix3 &matrix);
float MatrixToYRotateAngle(const Matrix3 &inMatrix);

Matrix3 GetMagicMatrix(INode **nodes, int index, TimeValue time)
{
	Matrix3 matrix;
	Matrix3 test = nodes[index]->GetObjectTM(time);
	const char *name = nodes[index]->GetName();
	bool rootNode = false;

	if ((0 == strcmp(name, "Bip01 Pelvis")) || (0 == strcmp(name, "Bip02 Pelvis")))
	{
		rootNode = true;
	}

	if ((0 == index) || (rootNode))
	{
		// get the "Bip01" node
		INode *bip01Node = nodes[index]->GetParentNode();
		Matrix3 bip01Matrix = GetNodeTM(bip01Node, time);
		bip01Matrix = StripScale(bip01Matrix);

		matrix = GetNoScaleLocalTM(nodes[index], time);
		matrix = StripScale(matrix);

		matrix = matrix * bip01Matrix;
	}
	else
	{
		Matrix3 local = GetNoScaleLocalTM(nodes[index], time);
		matrix = local;
	}

	return matrix;
}

class MatrixAnimExport : public SceneExport 
{
public:
					MatrixAnimExport()				{}
					~MatrixAnimExport()				{}
	int				ExtCount()						{ return 1; }
	const TCHAR *	Ext(int n)						{ assert(0 == n); return "anm"; }
	const TCHAR *	LongDesc()						{ return "ONI Biped Matrix Animation File"; }
	const TCHAR *	ShortDesc()						{ return "ONI Biped Matrix Animation File"; }
	const TCHAR *	AuthorName()					{ return "Michael Evans"; }
	const TCHAR *	CopyrightMessage()				{ return "Copyright Bungie Software 1997"; }
	const TCHAR *	OtherMessage1()					{ return ""; }
	const TCHAR *	OtherMessage2()					{ return ""; }
	unsigned int	Version()						{ return 100; } // 1.00
	void			ShowAbout(HWND hWnd)			{}

	int				DoExport(const char *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE);	// Export file
};

//------------------------------------------------------

class MAClassDesc:public ClassDesc {
	public:
	int 			IsPublic()						{ return 1; }
	void *			Create(BOOL loading = FALSE)	{ return new MatrixAnimExport; }
	const TCHAR *	ClassName()						{ return "ONI Biped Matrix Anim Exporter"; }
	SClass_ID		SuperClassID()					{ return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return Class_ID(0xa352d21b, 0x06a4d2ac); }
	const TCHAR* 	Category()						{ return "Scene Export";  }
};

class MatrixModelExport : public SceneExport 
{
public:
					MatrixModelExport()				{}
					~MatrixModelExport()			{}
	int				ExtCount()						{ return 1; }
	const TCHAR *	Ext(int n)						{ assert(0 == n); return "mme"; }
	const TCHAR *	LongDesc()						{ return "ONI Biped Matrix Model File"; }
	const TCHAR *	ShortDesc()						{ return "ONI Biped Matrix Model File"; }
	const TCHAR *	AuthorName()					{ return "Michael Evans"; }
	const TCHAR *	CopyrightMessage()				{ return "Copyright Bungie Software 1997"; }
	const TCHAR *	OtherMessage1()					{ return ""; }
	const TCHAR *	OtherMessage2()					{ return ""; }
	unsigned int	Version()						{ return 100; } // 1.00
	void			ShowAbout(HWND hWnd)			{}

	int				DoExport(const char *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE);	// Export file
};

//------------------------------------------------------

class MMClassDesc:public ClassDesc {
	public:
	int 			IsPublic()						{ return 1; }
	void *			Create(BOOL loading = FALSE)	{ return new MatrixModelExport; }
	const TCHAR *	ClassName()						{ return "ONI Biped Matrix Model Exporter"; }
	SClass_ID		SuperClassID()					{ return SCENE_EXPORT_CLASS_ID; }
	Class_ID		ClassID()						{ return Class_ID(0xa544d617, 0x46e4c2dc); }
	const TCHAR* 	Category()						{ return "Scene Export";  }
};


MMClassDesc		MMDesc;
MAClassDesc		MADesc;
ClassDesc		*pMMDesc = &MMDesc;
ClassDesc		*pMADesc = &MADesc;



// this is an object that can be called by the system with every node
//	AnimEnumProc myScene(ei->theScene, gi->GetTime(), gi);

static void AddNonBipedChildrenRecursive(	
	Scene *scene, 
	Matrix3 tm, 
	INode *node, 
	int count, 
	INode **list, 
	TimeValue time)
{
	int itr;


	// 1. find a child
	// 2. calculate the matrix
	// 2. add the child + offset
	// 3. recurse


	for(itr = 0; itr < count; itr++)
	{
		INode *nodeItr = list[itr];
		char *name = nodeItr->GetName();

		if (INodeIsBiped(nodeItr)) {
			continue;
		}

		if (nodeItr->GetParentNode() != node) {
			continue;
		}

		Matrix3 localTM = GetLocalTM(nodeItr, time);
		Matrix3 objectTM = GetObjectOnlyTM(nodeItr, time);
		Matrix3 transmitTM = localTM * tm;
		Matrix3 addTM = objectTM * transmitTM;

		unsigned long addOptions = 0;
		addOptions |= Scene::AddOptions_NoMatrix;

		bool normal = NormalNode(nodeItr, time);

		if (normal)
		{
			scene->AddNode(nodeItr, addTM, addOptions);
		}

		AddNonBipedChildrenRecursive(scene, transmitTM, nodeItr, count, list, time);
	}
}


int MatrixModelExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi, BOOL suppressPrompts) 
{
	IScene		*scene = ei->theScene;
	TimeValue	time   = gi->GetTime();

	// time stuff
	Interval	interval	= gi->GetAnimRange();
	TimeValue	start		= interval.Start();
	TimeValue	end			= interval.End();
	int			frameSize	= GetTicksPerFrame();
	int			numFrames	= ((end - start) / frameSize) + 1;

	gi->ProgressStart("Anim Export", TRUE, progress, 0);

	int numNodes = NodeCounter::CountNodes(scene, start, kCountCoreBiped); // kCountCoreBiped);
	INode **nodes = NodeList::GetNodeList(scene, start, kCountCoreBiped); // kCountCoreBiped);

	int fullCount = NodeCounter::CountNodes(scene, start, kCountAllNodes);
	INode **fullNodes = NodeList::GetNodeList(scene, start, kCountAllNodes);

	// step 1 open the file
	gi->ProgressUpdate(0, FALSE, "opening file");
	FILE *stream = fopen_safe(filename, "wb");

	// could not open file so bail
	if (NULL == stream) { goto DoExportBail; }

	/*

		File Format

		num parts
		#

		num frames
		#

		output of each geometry (raw)

		nodeTM
		objTM

	*/

	// write to disk
	int			nodeItr;

	MXtHeader header;
	strcpy(header.stringENVF, "ENVF");
	header.version = 0;
	header.numNodes = numNodes;
	header.time = time;
	header.nodes = NULL;

	fwrite(&header, sizeof(header), 1, stream);

	{
		Scene *exportScene = new Scene(scene, time, gi);

		// dump geometries
		for(nodeItr = 0; nodeItr < numNodes; nodeItr++) {
			int progressAmt = (100 * nodeItr) / numNodes;

			char *name = nodes[nodeItr]->GetName();
			unsigned long child = FindChild(nodes[nodeItr], nodes, numNodes);
			unsigned long sibling = FindSibling(nodeItr, nodes, numNodes);
			unsigned long addOptions = 0;
			
			addOptions |= Scene::AddOptions_NoMatrix;
			addOptions |= Scene::AddOptions_IgnoredBiped;
			
			gi->ProgressUpdate(progressAmt, FALSE, name);

			// calculate the object offset only
			Matrix3 objectOnlyTM = GetObjectOnlyTM(nodes[nodeItr], time);
			Matrix3 identityTM;
			identityTM.IdentityMatrix();

			Matrix3 nodeTM = GetNodeTM(nodes[nodeItr], time);
			Matrix3 noScaleNodeTM = StripScale(nodeTM);

			Matrix3 inheritTM = nodeTM * Inverse(noScaleNodeTM);

			// calculate the parent scale
			Matrix3 addTM = inheritTM * objectOnlyTM;

			// add this node
			exportScene->AddNode(nodes[nodeItr], addTM, addOptions);

			// add all its children
			AddNonBipedChildrenRecursive(exportScene, 
				inheritTM, 
				nodes[nodeItr],
				fullCount,
				fullNodes,
				time);

			Matrix3 magicMatrix = GetMagicMatrix(nodes, nodeItr, start);

			// convert to quads
			exportScene->FacesToQuads();
			exportScene->WriteToStream(stream, name, nodes[nodeItr]->GetParentNode()->GetName(), magicMatrix);
			exportScene->Clear();
		}

		delete exportScene;
	}

	// close the stream
	fclose(stream);

	// dispose the memory
	if (NULL != nodes)
	{
		free(nodes);
	}

	if (NULL != fullNodes)
	{
		free(fullNodes);
	}

DoExportBail:  	// close everything
	gi->ProgressUpdate(0, FALSE, "finishing");
	gi->ProgressEnd();

	return 1;
}

/*

  prints an angle that would be the approximate radian amount of Y Axis rotation
  needed to approximate that matrix

 */


float xytoradians(float x, float y)
{
	float result;

	result = (float) acos(x);

	if (y < 0)
	{
		result = TWOPI - result;
	}


	return result;
}

float MatrixToYRotateAngle(const Matrix3 &inMatrix)
{
	Matrix3 workMatrix = MatrixToRotateMatrix(inMatrix);
	Point3 pointAtZero(1,0,0);
	Point3 pointResult;
	float  radians;

	pointResult = pointAtZero * workMatrix;
	pointResult.y = 0;
	pointResult = Normalize(pointResult);

	radians = xytoradians(pointResult.x, -1.f * pointResult.z);

	return radians;
}

Matrix3 MatrixToYRotateMatrix(const Matrix3 &inMatrix)
{
	float radians = MatrixToYRotateAngle(inMatrix);
	Matrix3 result = RotateYMatrix(radians);

	return result;
}

static void PrintMatrixYRotateAngle(FILE *stream, const Matrix3 &inMatrix)
{
	float  radians = MatrixToYRotateAngle(inMatrix);

	fprintf(stream, "%f\r\n",radians);
}


int MatrixAnimExport::DoExport(const TCHAR *filename,ExpInterface *ei,Interface *gi, BOOL suppressPrompts)
{
	IScene		*scene = ei->theScene;
	TimeValue	time   = gi->GetTime();

	// time stuff
	Interval	interval	= gi->GetAnimRange();
	TimeValue	start		= interval.Start();
	TimeValue	end			= interval.End();
	int			frameSize	= GetTicksPerFrame();
	int			numFrames	= ((end - start) / frameSize) + 1;

	float		totalProgress;
	float		currentProgress;

	gi->ProgressStart("Anim Export", TRUE, progress, 0);

	int numNodes = NodeCounter::CountNodes(scene, start, kCountCoreBiped); // kCountCoreBiped);
	INode **nodes = NodeList::GetNodeList(scene, start, kCountCoreBiped); // kCountCoreBiped);

	// step 1 open the file
	gi->ProgressUpdate(0, FALSE, "opening file");
	FILE *stream = fopen_safe(filename, "wb");

	// could not open file so bail
	if (NULL == stream) { goto DoExportBail; }

	/*

		File Format

		num parts
		#

		num frames
		#

		output of each geometry (raw)

		nodeTM
		objTM

	*/

	// write to disk
	int			nodeItr;
	TimeValue	timeItr;

	fprintf(stream, "Animation File\r\n");

	fprintf(stream, "Version\r\n");
	fprintf(stream, "1.4\r\n");

	fprintf(stream, "comments\r\n");
	fprintf(stream, "%d\r\n", 0);				// number of lines of comments

	fprintf(stream, "Timestamp\r\n");
	fprintf(stream, "%s\r\n",__TIMESTAMP__);

	fprintf(stream, "node count =\r\n");
	fprintf(stream, "%d\r\n", numNodes)
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
	fprintf(stream, "\r\n");

	totalProgress = (float) (((end - start + 1) / frameSize) * numNodes);

	// dump matricies
	for(timeItr = start; timeItr <= end; timeItr += frameSize) {
		fprintf(stream, "time\r\n");
		fprintf(stream, "%d\r\n", timeItr);

		for(nodeItr = 0; nodeItr < numNodes; nodeItr++) {
			Matrix3 t0matrix = GetMagicMatrix(nodes, nodeItr, start);
			Matrix3 matrix = GetMagicMatrix(nodes, nodeItr, timeItr);
			Matrix3 nextMatrix = GetMagicMatrix(nodes, nodeItr, UUmMin(timeItr + frameSize, end));
			
			fprintf(stream, "name\r\n");
			fprintf(stream, "%s\r\n", nodes[nodeItr]->GetName());

			// print the matrix
			PrintMatrix(stream, matrix);
			
			currentProgress = ((float)((timeItr - start + 1)) / frameSize) * numNodes + nodeItr;
			gi->ProgressUpdate((int) ((100.f * currentProgress) / totalProgress));
		}

		fprintf(stream, "\r\n");
	}

	// close the stream
	fclose(stream);

	// dispose the memory
	if (NULL != nodes) 
	{
		free(nodes);
	}

DoExportBail:  	// close everything
	gi->ProgressUpdate(0, FALSE, "finishing");
	gi->ProgressEnd();

	return 1;
}