/*
 *
 * File: ExportTools.h
 *
 * Author: Michael Evans
 *
 * Copyright (c) Bungie Software 1997
 *
 */


// 3DS Max includes
#include "Max.h"
#include "EnvFileFormat.h"

// ANSI includes
#include <stdio.h>
#include <math.h>

#define INVALID_INDEX	0xffffffff
#define kParent_Root	0xffffffff
#define MAX_NORMAL_DISTANCE 0.001

DWORD WINAPI progress(LPVOID arg);

#define UUmMin(a,b) ((a)>(b)?(b):(a))
#define UUmMax(a,b) ((a)>(b)?(a):(b))

#define UUmFloor(n,floor) ((n)<(floor)?(floor):(n))
#define UUmCeiling(n,ceiling) ((n)>(ceiling)?(ceiling):(n))
#define UUmPin(n,floor,ceiling) ((n)<(floor) ? (floor) : UUmCeiling(n,ceiling))

#define UUcMaxInt8	((signed char) 0x7f)
#define UUcMinInt8  ((signed char) -128)

#undef assert
#define assert(x) if (!(x)) do { int macro_result = MBoxPrintf(MB_ABORTRETRYIGNORE, "file %s line %d msg %s\r\n",__FILE__,__LINE__,#x); if (IDRETRY == macro_result) __asm { int 3 } } while(0)

int MBoxPrintf(	UINT uType,
				char *lpCaption,
    			const char *format,
				...);

#define MAXIMUM_EXPORT_MATERIALS 40
typedef struct {
	Mtl *material;
	int sub_material_index;
} ExportMaterial;

#define MAXIMUM_EXPORT_MARKERS 40
typedef struct
{
	Matrix3 matrix;
	char *name;
	TSTR user_data;
} ExportMarker;

typedef struct {
	float x,y,z;			// coordinates

	float nx, ny, nz;		// normal
	float u,v;				// texture map
} ExportPoint;

typedef struct {
	long material_index;
	unsigned long index[4];		// verticies (triangle or quad)

	unsigned long	geometry;	// which geometry this face belongs to
	bool		marked;		// used internally

	float nx, ny, nz;		// face normal
} ExportFace;

typedef struct {
	INode *node;				// node this edge came from

	unsigned long	v1, v2;		// v1 to v2 edge
	bool		visable;		// is it supposed to be 

	unsigned long	f1, f2;		// used internally (faces this edge is from)
} ExportEdge;

typedef struct {
	bool		valid;		// internal, true if any points in it, false otherwise
	
	float		xmin, xmax;
	float		ymin, ymax;
	float		zmin, zmax;
} ExportBBox;

typedef struct {
	// bounding box
	ExportBBox	bbox;

	// hierarchy information
	unsigned long	parent;			// index or 0xffffffff for the parent
	bool		inTree;			// used internally

	unsigned long	numFaces;
} ExportGeometry;

typedef enum
{
	kCountAllNodes,
	kCountNormalNodes,
	kCountDualTextureNormalNodes,		
	kCountNormalPlusBiped,
	kCountCoreBiped,
	kCountMarkerNodes
} CountType;

bool CountThisNode(INode *node, CountType countType);

void GetTextureName(INode *node, char *buf);
char *GetMaxTextureName(INode *node);
unsigned long FindChild(INode *node, INode **list, unsigned long count);
unsigned long FindSibling(unsigned long index, INode **list, unsigned long count);
			   
class NodeCounter : public ITreeEnumProc
{
	public:
		static int CountNodes(IScene *scene, TimeValue time, CountType countType = kCountAllNodes);
		static int CountNormalNodes(IScene *scene);

	private:
		int	callback(INode *node);	

		CountType fCountType;
		TimeValue fTime;
		int fCount;
};

class NodeList : public ITreeEnumProc
{
	public:
		static INode **GetNodeList(IScene *scene, TimeValue time, CountType countType = kCountAllNodes);


	private:
		int	callback(INode *node);

		INode **fList;

		CountType fCountType;
		TimeValue fTime;
		int fCount;
};

int CountNodePoints(INode *node, TimeValue time);
bool NormalNode(INode *node, TimeValue time);
bool INodeHasTexture1(INode *node, TimeValue time);
bool INodeHasTexture2(INode *node, TimeValue time);

/*
 *
 * This parses the node that is is given and builds up
 * a parsd list.  Then with WriteToStream you can dump
 * that structure to a file.
 *
 */

class Scene 
{
	public:
		Scene(IScene *scene, TimeValue t, Interface *i);
		~Scene();
		void Clear(void);

		/*
			we want to be able to ignore certain parts
			of a decomposed node tm

			TRTM = translate & rotate

		 */

		enum
		{
			AddOptions_IgnoreNodeRTTM		= 0x00000001,
			AddOptions_SupportNonGeometry	= 0x00000002,
			AddOptions_IgnoredBiped			= 0x00000004,
			AddOptions_AddHidden			= 0x00000008,
			AddOptions_NoMatrix				= 0x00000010,
			AddOptions_AddMarkers			= 0x00000020
		};

		void	AddNode(INode *node, unsigned long options = 0);
		void	AddNode(INode *node, Matrix3 tm, unsigned long options = 0);
		void	AddScene(Scene *scene);

		// multiplies everythin by this matrix
		void	Multiply(Matrix3 &m);

		bool	IsEmpty(void) { return (0 == fNumPoints); }

		enum
		{
			WriteOptions_TrianglesOnly		= 0x00000008,
			WriteOptions_QuadsOnly			= 0x00000010
		};

		void GetMapName(Mtl *material, int subTexmapType, char *outName, float *amount);

		static void MXrMatrix(const Matrix3 &m, MUtMatrix3 &outMatrix);

		void WriteUserData(FILE *stream);
		void WriteMaterials(FILE *stream);
		void WriteMarkers(FILE *stream);
		void WriteToStream(FILE *stream, const char *name, const char *parent_name, Matrix3 tm);
		void WritePoints(FILE *stream);

		void WriteFaces(FILE *stream,		unsigned long options = 0);
		void WriteTriangles(FILE *stream,	unsigned long options = 0);
		void WriteQuads(FILE *stream,		unsigned long options = 0);

		void WriteGeometries(FILE *stream,	unsigned long options = 0);

	private:
		void AddMarkersForNode(INode *node);
		void AddThisMarker(INode *node);

		int GetExportMaterialIndex(Mtl *material, int sub_material_index);
		unsigned long	CountTriangles(void);
		unsigned long	CountQuads(void);

	private:
		IScene		*fScene;
		TimeValue	fTime;
		Interface	*fInterface;

	public:
		char		*fNodeName;
		char		*fUserData;

		ExportMarker fMarkers[MAXIMUM_EXPORT_MARKERS];
		long fNumMarkers;

		ExportMaterial fMaterials[MAXIMUM_EXPORT_MATERIALS];
		long fNumMaterials;

		ExportGeometry	*fGeometries;
		unsigned long	fNumGeometries;

		ExportPoint *fPoints;
		unsigned long	fNumPoints;
		 
		ExportFace *fFaces;
		unsigned long	fNumFaces;

		ExportEdge *fEdges;
		unsigned long	fNumEdges;
		unsigned long	fAllocatedEdges;

		bool		fWarnNonUnitNormals;
		bool		fWarnIdenticalUV;			
		bool		fQuads;
		bool		fWarnMapping;

		float		fMaxNormalDistance;
		unsigned long	fMaxNormalDistanceCount;
		char		*fMaxNormalDistanceName;

	public:
		// given a face and an offset to our internal representation of 
		// verts it will add this edge to our edge list
		void		AddEdges(INode *node, Face *inFace, unsigned long faceIndex, const unsigned long vertOffset);
		void		AddEdgeIfUnique(ExportEdge &edge, unsigned long faceIndex);
		ExportFace	MergeFaces(ExportEdge edge);
		void		BuildHierarchy(void);
		void		OffsetPoints(Point3 offset);	// translates all the points
		Point3		FindCenter(void);

	private:
		// these functions grow the memory buffers
		void GrowPointList(unsigned long newTotal);
		void GrowFaceList(unsigned long newTotal);
		void GrowEdgeList(unsigned long newTotal);

	public:
		// debugging sorts of things
		void VerifyFaces(void);
		void VerifyFace(ExportFace *edge);

		void VerifyEdges(void);
		void VerifyEdge(ExportEdge *edge);
		bool FaceHasVertex(ExportFace *face, unsigned long vertex);

		enum 
		{
			QuadOptions_Exact			= 0x00000001
		};

		void FacesToQuads(unsigned long options = 0);
		void RoundPoints(void);			// rounds all the points to their nearest integer
		void ClearUV(void);				// clears all the texture coordinates
		void CompressPoints(void);		// finds duplicate points and removes them
	
	private:
		void DoExportPoint(Mesh *mesh, Matrix3 &tm, int i, ExportPoint *outPoint);

	public:

static	Matrix3 CoordinateFix(void);
static	Point3	TranslateNormal(Point3 normal, Matrix3 tm);
static	Point3	TranslatePoint(Point3 point, Matrix3 tm);			

	// changes coordinate system from max to oni
		void CoordinateSystemMaxToOni(void);	

// static	Point3	&FixCoordinates(Point3 inPoint);	
// static  Matrix3 &FixMatrix(Matrix3 inMatrix);

		// functions related to point compression
	private:
		void RemoveDuplicatesOfPoint(unsigned long original);

	// functions related to hierarchy construction
	private:
		void GrowBBox(ExportBBox *geometry, unsigned long pointIndex);
		unsigned long FindNodeThatEnclosesNoUnmarkedGeometries();
		bool EnclosesUnmarkedGeometries(unsigned long geomIndex);
		bool GeometryEncloses(ExportGeometry *enclosing, ExportGeometry *test);
		void AttachGeometry(unsigned long index);

	public:
		unsigned long Checksum(void);
};


/*
 *
 * Matrix helper functions
 *
 */


		// local is just our matrix in the hierarchy
Matrix3	GetLocalTM(INode *node, TimeValue t);

		// gets the no scale local
Matrix3 GetNoScaleLocalTM(INode *node, TimeValue time);

		// matrix is the sum of ours and our parents and so on up the chain
Matrix3 GetNodeTM(INode *node, TimeValue t);

		// offset is the total world space tm
Matrix3	GetObjectTM(INode *node, TimeValue t);

		// this is only the tm for our object (node tm removed)
Matrix3 GetObjectOnlyTM(INode *node, TimeValue t);



/*
 *
 * General Matrix operations
 *
 */

Matrix3	StripScale(Matrix3 inMatrix);
Matrix3	StripRotate(Matrix3 inMatrix);
Matrix3 StripTranslate(Matrix3 inMatrix);
Matrix3 StripRotTrans(Matrix3 inMatrix);
Matrix3 MatrixToRotateMatrix(const Matrix3 &matrix);

/*
 *
 * handy stdio help.
 *
 */

FILE *fopen_safe(const char *name, const char *mode);

void PrintLines(FILE *stream, const char *buf);
void PrintFloat(FILE *stream, float f);
void PrintMatrix(FILE *stream, const Matrix3 &matrix);

void PrintMatrixEuler(FILE *stream, const Matrix3 &matrix);
void PrintMatrixRotate(FILE *stream, const Matrix3 &matrix);
void PrintMatrixTranslate(FILE *stream, const Matrix3 &matrix);

void PrintDecomposedMatrix(FILE *stream, const Matrix3 &matrix);
void PrintBBox(FILE *stream, ExportBBox &bbox);
void PrintPoint(FILE *stream, const Point3 &point);
void PrintAngleAxis(FILE *stream, const Point3 &axis, float angle);
void PrintAngleAxis(FILE *stream, Quat q);
void PrintQAsAngleAxis(FILE *stream, Quat q);
void PrintQuaternion(FILE *stream, Quat q);
void PrintEuler(FILE *stream, float euler[3]);

void PrintSeperator(FILE *stream);
void PrintLineBreak(FILE *stream);

bool	INodeIsBiped(INode *node);
bool	INodeIsGeomObject(INode *node);
bool	HasSuffix(char *string, char *suffix);
