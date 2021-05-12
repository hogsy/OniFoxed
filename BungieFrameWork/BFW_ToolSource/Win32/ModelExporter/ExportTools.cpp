/*
 *
 *
 *
 */ 

//  coordinate system fix
//	result.x = inPoint.x;
//	result.y = inPoint.z;
//	result.z = -inPoint.y;


#include "Max.h"
#include "decomp.h"
#include "STDMAT.H"
#include "EnvFileFormat.h"

#include "ExportTools.h"

#define ONE_EPSILON ((float) 0.00001)
#define SMALL_ONE   (1 - ONE_EPSILON)
#define BIG_ONE     (1 + ONE_EPSILON)

#define SQR(x) ((x) * (x))

unsigned long gClosecount = 0;
unsigned long gDifftcount = 0;

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

static void MatToQuat(const Matrix3 &inMatrix, Quat *quat)
{
	float m[3][3];

	Point3 col0		= inMatrix.GetRow(0);
	Point3 col1		= inMatrix.GetRow(1);
	Point3 col2		= inMatrix.GetRow(2);
	Point3 col3		= inMatrix.GetRow(3);

	m[0][0] = col0.x;
	m[1][0] = col0.y;
	m[2][0] = col0.z;

	m[0][1] = col1.x;
	m[1][1] = col1.y;
	m[2][1] = col1.z;

	m[0][2] = col2.x;
	m[1][2] = col2.y;
	m[2][2] = col2.z;

	// **************************************************

	float tr, s, q[4];
	int i, j, k;

	int nxt[3]= {1, 2, 0};

	tr= m[0][0] + m[1][1] + m[2][2];

	// check the diagonal
	if (tr>0.0)
	{
		s= (float) sqrt(tr + 1.0);
		quat->w= s/2.f;
		s= 0.5f/s;
		quat->x= (m[1][2] - m[2][1]) * s;
		quat->y= (m[2][0] - m[0][2]) * s;
		quat->z= (m[0][1] - m[1][0]) * s;
	}
	else
	{
		// diagonal is negative
		i = 0;

		if (m[1][1] > m[0][0]) i = 1;
		if (m[2][2] > m[i][i]) i = 2;
		j = nxt[i];
		k = nxt[j];

		s = (float) sqrt((m[i][i] - (m[j][j] + m[k][k])) + 1.f);

		q[i] = s*0.5f;

		if (s!=0.0) s = 0.5f/s;

		q[3] = (m[j][k] - m[k][j])*s;
		q[j] = (m[i][j] + m[j][i])*s;
		q[k] = (m[i][k] + m[k][i])*s;

		quat->x = q[0];
		quat->y = q[1];
		quat->z = q[2];
		quat->w = q[3];
	}
}


void my_decomp_affine(Matrix3 A, AffineParts *parts)
{
	Quat test;

	decomp_affine(A, parts);
}

int MBoxPrintf(	UINT uType,
				char *lpCaption,
    			const char *format,
				...)
{
	int result;
	char buffer[2048];
	va_list arglist;
	int return_value;
	
	va_start(arglist, format);
	return_value= vsprintf(buffer, format, arglist);
	va_end(arglist);

	if (NULL == lpCaption) { lpCaption = "message"; }

	result = MessageBox(NULL, buffer, lpCaption, uType);

	return result;
}


void DebugLogPrintf(const char *format, ...)
{
	static FILE *debug_log = NULL;
	char buffer[2048];
	va_list arglist;
	int return_value;
	
	va_start(arglist, format);
	return_value= vsprintf(buffer, format, arglist);
	va_end(arglist);


	if (NULL == debug_log)
	{
		debug_log = fopen("f:\\export_log.txt", "w");
	}

	if (NULL == debug_log)
	{
		debug_log = fopen("export_log.txt", "w");
	}
	
	if (debug_log != NULL)
	{
		fprintf(debug_log, "%s", buffer);
		fflush(debug_log);
	}
}

static void GetUserTextureName(INode *node, char *buf)
{
	char *strippedName = node->GetName();

	while(*strippedName != ':')
	{
		if ('\0' == strippedName)
		{
			strippedName = "??badname??";
			break;
		}

		strippedName++;
	}
	// skip over the ':'
	strippedName++;

	sprintf(buf, "%s.bmp", strippedName);
}

char *GetMaxTextureName(Mtl* mtl)
{
	Texmap* texmap;
	char *mapName;

	// get material
	if (NULL == mtl) { return NULL;	}

	// get sub texture map (diffuse)
	texmap = mtl->GetSubTexmap(ID_DI);
	if (NULL == texmap) { return NULL;	}
	
	// must be BitmapTex object
	if (texmap->ClassID() != Class_ID(BMTEX_CLASS_ID, 0x00)) { return NULL; }

	mapName = ((BitmapTex *)texmap)->GetMapName();

	return mapName;
}

char *GetMaxTextureName(INode *node)
{
	assert(NULL != node);

	Mtl* mtl = node->GetMtl();

	return GetMaxTextureName(mtl);
}


void GetTextureName(INode *node, char *buf)
{
	char *name;

	name = GetMaxTextureName(node);
	if (NULL != name)
	{
		strcpy(buf, name);
	}
	else
	{
		GetUserTextureName(node, buf);
	}
}

static void CompressArray(void *array, unsigned long removed, unsigned long length, unsigned long elementSize)
{
	assert(NULL != array);
	assert(removed < length);
	
	unsigned long iArray = (unsigned long) array;
	unsigned long iSrc	= iArray + ((removed + 1) * elementSize);
	unsigned long iDst	= iArray + (removed * elementSize);
	unsigned long len	= (length - removed - 1) * elementSize;

	memmove((void *) iDst, (void *) iSrc, len);
}

DWORD WINAPI progress(LPVOID arg)
{
	return 0;
}


bool CoreBiped(INode *node)
{
	// 1. if it is not a biped, it is not a core biped
	bool biped = INodeIsBiped(node);
	if (!biped) { return false; }

	// 2. scan for evil suffix
	const int ignore_suffix_count = 7;
	char *ignore_suffix_table[ignore_suffix_count] =
	{
		"Footsteps",
		"Toe0",
		"Toe01",
		"Toe02",
		"Toe03",
		"Toe04",
		"Toe05"
	};
	char *name = node->GetName();

	int itr;
	for(itr = 0; itr < ignore_suffix_count; itr++)
	{
		bool hasSuffix;

		hasSuffix = HasSuffix(name, ignore_suffix_table[itr]);

		if (hasSuffix) {
			return false;
		}
	}

	// strip the node called Bip01 as well
	if ((0 == strcmp(name, "Bip01")) || (0 == strcmp(name, "Bip02")))
	{
		return false;
	}
	
	return true;
}

bool CountThisNode(INode *node, TimeValue time, CountType countType)
{
	char *name = node->GetName();
	bool marker = '#' == name[0];
	bool counts;

	if (kCountMarkerNodes == countType)
	{
		counts = marker;
		return counts;
	}

	if (marker) {
		return false;
	}

	switch(countType)
	{
		case kCountAllNodes:
			counts = true;
		break;

		case kCountNormalNodes:
			counts = NormalNode(node, time);
		break;

		case kCountDualTextureNormalNodes:
			counts = NormalNode(node, time);
			if (counts)
			{
//				counts &= INodeHasTexture1(node, time);
				counts &= INodeHasTexture2(node, time);
			}
		break;

		case kCountNormalPlusBiped:
			counts = NormalNode(node, time);
			counts |= CoreBiped(node);
		break;

		case kCountCoreBiped:
			counts = CoreBiped(node);
		break;

		default:
			assert(!"Should not be here");
		break;
	}

	return counts;
}

int NodeCounter::CountNodes(IScene *scene, TimeValue time, CountType countType) 
{
	NodeCounter nodeCounter; 
	nodeCounter.fCount = 0;
	nodeCounter.fCountType = countType;
	nodeCounter.fTime = time;

	scene->EnumTree(&nodeCounter);

	return nodeCounter.fCount;
};

int NodeCounter::callback(INode *node) 
{
	bool counts = CountThisNode(node, fTime, fCountType);

	if (counts) 
	{
		fCount += 1;
	}

	return TREE_CONTINUE; 
}

int inode_depth(INode *inNode)
{
	int depth = 0;

	for(depth = 0; !inNode->IsRootNode(); inNode = inNode->GetParentNode())
	{
	}

	return depth;
}

int inode_biped_order(INode *inNode)
{
	int itr;
	const char *name = inNode->GetName();
	const char *table[] = 
	{
		"Bip01 Pelvis",
		"Bip01 L Thigh",
		"Bip01 L Calf",
		"Bip01 L Foot",
		"Bip01 R Thigh",
		"Bip01 R Calf",
		"Bip01 R Foot",
		"Bip01 Spine",
		"Bip01 Spine1",
		"Bip01 Neck",
		"Bip01 Head",
		"Bip01 L Clavicle",
		"Bip01 L UpperArm",
		"Bip01 L Forearm",
		"Bip01 L Hand",
		"Bip01 R Clavicle",
		"Bip01 R UpperArm",
		"Bip01 R Forearm",
		"Bip01 R Hand",
		"NULL",
	};

	for(itr = 0; table[itr] != NULL; itr++)
	{
		if (0 == strcmp(table[itr] + strlen("Bip01 "), name + strlen("Bip01 ")))
		{
			break;
		}
	}

	return itr;
}

int sort_inodes_compare(const void *elem1, const void *elem2)
{
	INode *inode_1 = *((INode **) elem1);
	INode *inode_2 = *((INode **) elem2);
	int sort;

	if (INodeIsBiped(inode_1) && INodeIsBiped(inode_2))
	{
		sort =  inode_biped_order(inode_1) - inode_biped_order(inode_2);
	}
	else
	{
		sort =  inode_depth(inode_1) - inode_depth(inode_2);
	}

	if (0 == sort)
	{
		const char *name_1 = inode_1->GetName();
		const char *name_2 = inode_2->GetName();

		sort = strcmp(name_1, name_2);
	}

	return sort;
}

// this function sorts the list so that the roots are at the 
// front of the list
static void SortINodes(INode **list, unsigned long count)
{
	unsigned long itr;
	unsigned long rootCount = 0;
	INode *root;
	INode *temp;

	qsort(list, count, sizeof(INode *), sort_inodes_compare);

	// step 1 root at front
	for(itr = 0; itr < count; itr++)
	{
		bool isRoot = list[itr]->IsRootNode() > 0;

		if (isRoot) {
			temp = list[rootCount];

			list[rootCount] = list[itr];
			list[itr] = temp;
			
			rootCount += 1;
		}
	}

	if (0 == rootCount) {
		root = NULL;
	}
	else {
		root = list[0];
	}


	// step 2 roots children next (important for biped)
	for(itr = rootCount; itr < count; itr++)
	{
		bool rootChild = list[itr]->GetParentNode() == root;

		if (rootChild) {
			temp = list[rootCount];

			list[rootCount] = list[itr];
			list[itr] = temp;
			
			rootCount += 1;
		}
	}
}

INode **NodeList::GetNodeList(IScene *scene, TimeValue time, CountType countType)
{
	NodeList nodeList;

	int numNodes = NodeCounter::CountNodes(scene, countType);

	nodeList.fList = (INode **) malloc(sizeof(INode **) * numNodes);
	nodeList.fCount = 0;
	nodeList.fCountType = countType;
	nodeList.fTime = time;

	scene->EnumTree(&nodeList);

	// sort so that there is a root at the top
	SortINodes(nodeList.fList, nodeList.fCount);

	return nodeList.fList;
}

int NodeList::callback(INode *node)
{
	bool counts = CountThisNode(node, fTime, fCountType);

	if (counts) 
	{
		fList[fCount] = node;
		fCount += 1;
	}

	return TREE_CONTINUE;
}



/*

  a normal node is a visable geometry object with a non zero
  point count

 */

int CountNodePoints(INode *node, TimeValue time)
{
	Object		*object			= node->EvalWorldState(time).obj;
	assert(NULL != object);

	int			canMesh			= object->CanConvertToType(triObjectClassID);
	assert(canMesh);

	TriObject	*triangle		= (TriObject *) object->ConvertToType(time, triObjectClassID);
	assert(NULL != triangle);

	Mesh		*mesh			= &(triangle->mesh);
	assert(NULL != mesh);

	bool		deleteTriangle	= (object) != ((Object *) triangle);
	
	int			numVerts		= mesh->numVerts;

	if (deleteTriangle) {
		assert(NULL != triangle);

		delete triangle;
	}

	return numVerts;
}

bool INodeHasTexture1(INode *node, TimeValue time)
{
	Object		*object			= node->EvalWorldState(time).obj;
	assert(NULL != object);

	int			canMesh			= object->CanConvertToType(triObjectClassID);
	assert(canMesh);

	TriObject	*triangle		= (TriObject *) object->ConvertToType(time, triObjectClassID);
	assert(NULL != triangle);

	Mesh		*mesh			= &(triangle->mesh);
	assert(NULL != mesh);

	bool		deleteTriangle	= (object) != ((Object *) triangle);
	
	bool		hasTexture1 = NULL != mesh->tVerts;

	if (deleteTriangle) {
		assert(NULL != triangle);

		delete triangle;
	}

	return hasTexture1;
}

bool INodeHasTexture2(INode *node, TimeValue time)
{
	Object		*object			= node->EvalWorldState(time).obj;
	assert(NULL != object);

	int			canMesh			= object->CanConvertToType(triObjectClassID);
	assert(canMesh);

	TriObject	*triangle		= (TriObject *) object->ConvertToType(time, triObjectClassID);
	assert(NULL != triangle);

	Mesh		*mesh			= &(triangle->mesh);
	assert(NULL != mesh);

	bool		deleteTriangle	= (object) != ((Object *) triangle);
	
	bool		hasTexture2 = NULL != mesh->vertCol;

	if (deleteTriangle) {
		assert(NULL != triangle);

		delete triangle;
	}

	return hasTexture2;
}

bool NormalNode(INode *node, TimeValue time)
{
	char *name		= node->GetName();
	int hidden		= node->IsHidden();
	bool geometry	= INodeIsGeomObject(node);
	Object *object	= node->EvalWorldState(time).obj;
	int canMesh		= object->CanConvertToType(triObjectClassID);
	bool biped		= INodeIsBiped(node);

	if (hidden) {
		return false;
	}

	if (!geometry) {
		return false;
	}

	if (!canMesh) {
		return false;
	}

	if (biped) {
		return false;
	}

	int numVerts	= CountNodePoints(node, time);

	if (0 == numVerts) {
		return false;
	}

	// passed all the tests

	return true;
}

unsigned long FindChild(INode *node, INode **list, unsigned long count)
{
	unsigned long itr;

	for(itr = 0; itr < count; itr++)
	{
		INode *parent;

		parent = list[itr]->GetParentNode();

		if (node == parent)
		{
			return itr;
		}
	}

	// didnt find anyone who has me as the parent
	return 0;
}

unsigned long FindSibling(unsigned long index, INode **list, unsigned long count)
{
	unsigned long itr;
	INode *myParent = list[index]->GetParentNode();

	for(itr = index + 1; itr < count; itr++)
	{
		INode *parent;

		parent = list[itr]->GetParentNode();

		if (myParent == parent)
		{
			return itr;
		}
	}

	// didn't find anyone after me who shared my parent
	return 0;
}

//--------------------------------------------------------------
Scene::Scene(IScene *scene, TimeValue time, Interface *i)
{
	fScene		= scene;
	fTime		= time;
	fInterface	= i;

	fUserData	= NULL;
	fPoints		= NULL;
	fFaces		= NULL;
	fEdges		= NULL;
	fGeometries	= NULL;
	
	Clear();
}

/*
 
Given a mesh and an index into that mesh copies the verticies and the normals.

does not change the texture mapping for that vertex

*/

void Scene::DoExportPoint(Mesh *mesh, Matrix3 &tm, int i, ExportPoint *outPoint)
{
	assert(NULL != mesh);
	assert(NULL != outPoint);
	assert(i < mesh->numVerts);

	Point3 translatedPoint = TranslatePoint(mesh->verts[i], tm);
	Point3 normal = mesh->getNormal(i);
	Point3 translatedNormal = TranslateNormal(normal, tm);

	outPoint->x = translatedPoint.x;
	outPoint->y = translatedPoint.y;
	outPoint->z = translatedPoint.z;

	outPoint->nx = translatedNormal.x;
	outPoint->ny = translatedNormal.y;
	outPoint->nz = translatedNormal.z;
} 

Point3 Scene::TranslateNormal(Point3 normal, Matrix3 tm)
{

	float pre_length = Length(normal);

	if ((pre_length < SMALL_ONE) || (pre_length > BIG_ONE))
	{
		if (0 != pre_length) {
			normal /= pre_length;
		}
		else {
			normal.x = 1;
			normal.y = 0;
			normal.z = 0;
		}
	}

	// remove the transation from the matrix
	tm.NoTrans();

	// multiply through (this may include some non unit scale)
	// so we have to normalize
	Point3 translatedNormal = normal * tm;
	float post_length = Length(translatedNormal);
	translatedNormal /= post_length;

	// assert that the normal is approximately a unit normal
	post_length = Length(translatedNormal);
	assert((post_length >= SMALL_ONE) && (post_length <= BIG_ONE));

	return translatedNormal;
}

Point3 Scene::TranslatePoint(Point3 point, Matrix3 tm)
{
	Point3 translatedPoint = point * tm;

	return translatedPoint;
}

void Scene::GetMapName(Mtl *material, int subTexmapType, char *outName, float *amount)
{
	Texmap* texmap;

	strcpy(outName, "<none>");
	*amount = 0.f;

	if (NULL == material) return;

	texmap= material->GetSubTexmap(subTexmapType); 
	if (texmap && texmap->ClassID()==Class_ID(BMTEX_CLASS_ID, 0x00))
	{
		char *texture_name= ((BitmapTex *)texmap)->GetMapName();

		while (strchr(texture_name, '\\')) texture_name= strchr(texture_name, '\\') + 1;
		strcpy(outName, texture_name);
		// texture_name= strtok(outName, ".");
		*amount= ((StdMat *)material)->GetTexmapAmt(subTexmapType, fTime);
	}
}

void Scene::WriteUserData(FILE *stream)
{
//	TSTR user_prop_buffer;
//	node->GetUserPropBuffer(user_prop_buffer);

	fprintf(stream,	"user defined (num lines, data)\n");
	PrintLines(stream, fUserData);
	fprintf(stream, "\n");
}

void Scene::WriteMarkers(FILE *stream)
{

	// dump markers
	{
		short marker_index;
		
		for (marker_index= 0; marker_index<fNumMarkers; ++marker_index)
		{
			ExportMarker *thisMarker = fMarkers + marker_index;
			MXtMarker writeMarker;

			strcpy(writeMarker.name, thisMarker->name+1);
			MXrMatrix(thisMarker->matrix, writeMarker.matrix);
			writeMarker.userDataCount = strlen(thisMarker->user_data) + 1;
			writeMarker.userData = NULL;

			fwrite(&writeMarker, sizeof(writeMarker), 1, stream);
			fwrite(thisMarker->user_data, sizeof(char), strlen(thisMarker->user_data) + 1, stream);
		}
	}
}


void Scene::WriteMaterials(FILE *stream)
{
	char *foo = "";
	int material_index;

	for (material_index= 0; material_index<fNumMaterials; ++material_index)
	{
		Mtl *material;
		MXtMaterial writeMaterial;
		
		char *material_name= "<none>";

		char texture_name[300];
		float texture_amount;

		float alpha= 1.f;

		float self_illumination= 0.f;
		float shininess= 0.f, shininess_strength= 0.f;

		ULONG requirements;

		material= fMaterials[material_index].material;
		if (material)
		{
			if (fMaterials[material_index].sub_material_index>=0) {
				material= material->GetSubMtl(fMaterials[material_index].sub_material_index);
			}

			requirements = fMaterials[material_index].material->Requirements(fMaterials[material_index].sub_material_index);
		}

		if (material)
		{
			material_name= material->GetName();

			alpha= 1.f-material->GetXParency();
			if (alpha<0.f) alpha= 0.f;
			if (alpha>1.f) alpha= 1.f;

			self_illumination= material->GetSelfIllum();
			shininess= material->GetShininess();
			shininess_strength= material->GetShinStr();
		}

		// material name
		// fprintf(stream, "%s\r\n", material_name);
		strcpy(writeMaterial.name, material_name);

		// material alpha
		// fprintf(stream, "%f\r\n", alpha);
		writeMaterial.alpha = alpha;

		// material self-illumination
		// fprintf(stream, "%f\r\n", self_illumination);
		writeMaterial.selfIllumination = self_illumination;

		// material shininess and shininess strength
		// fprintf(stream, "%f\t%f\r\n", shininess, shininess_strength);
		writeMaterial.shininess = shininess;
		writeMaterial.shininessStrength = shininess_strength;

		// the material's three colors
		Color ambient;
		Color diffuse;
		Color specular;

		ambient.Black();
		diffuse.Black();
		specular.Black();

		if (fMaterials[material_index].material)
		{
			ambient= fMaterials[material_index].material->GetAmbient(fMaterials[material_index].sub_material_index);
			diffuse= fMaterials[material_index].material->GetDiffuse(fMaterials[material_index].sub_material_index);
			specular= fMaterials[material_index].material->GetSpecular(fMaterials[material_index].sub_material_index);
		}

		//fprintf(stream, "%f\t%f\t%f\r\n", ambient.r, ambient.g, ambient.b);
		//fprintf(stream, "%f\t%f\t%f\r\n", diffuse.r, diffuse.g, diffuse.b);
		//fprintf(stream, "%f\t%f\t%f\r\n", specular.r, specular.g, specular.b);
		writeMaterial.ambient[0] = ambient.r;
		writeMaterial.ambient[1] = ambient.g;
		writeMaterial.ambient[2] = ambient.b;

		writeMaterial.diffuse[0] = diffuse.r;
		writeMaterial.diffuse[1] = diffuse.g;
		writeMaterial.diffuse[2] = diffuse.b;

		writeMaterial.specular[0] = specular.r;
		writeMaterial.specular[1] = specular.g;
		writeMaterial.specular[2] = specular.b;

		int subTexmapType;

		for(subTexmapType = ID_AM; subTexmapType <= ID_RR; subTexmapType++) {
			GetMapName(material, subTexmapType, texture_name, &texture_amount);
			// fprintf(stream, "%f\t%s\n", texture_amount, texture_name);

			strcpy(writeMaterial.maps[subTexmapType].name, texture_name);
			writeMaterial.maps[subTexmapType].amount = texture_amount;
		}

		// flags: MTLREQ_2SIDE, MTLREQ_WIRE, MTLREQ_WIRE_ABS, MTLREQ_TRANSP, MTLREQ_UV ...
		//fprintf(stream, "%x\n", requirements);
		writeMaterial.requirements = requirements;

		//fprintf(stream, "\n");
		fwrite(&writeMaterial, sizeof(writeMaterial), 1, stream);
	}

	return;
}

void Scene::WriteToStream(FILE *stream, const char *name, const char *parent_name, Matrix3 tm)
{
//	Matrix3 fix = Scene::CoordinateFix();
	MXtNode currentNode;

	strcpy(currentNode.name, name);
	strcpy(currentNode.parentName, parent_name);

	MXrMatrix(tm, currentNode.matrix);

	currentNode.sibling = 0;
	currentNode.child = 0;
	currentNode.parentNode = NULL;

	currentNode.numPoints = (UUtUns16) fNumPoints;
	currentNode.pad1 = 0;
	currentNode.points = NULL;

	currentNode.numTriangles = (UUtUns16) CountTriangles();
	currentNode.pad2 = 0;
	currentNode.triangles = NULL;

	currentNode.numQuads = (UUtUns16) CountQuads();
	currentNode.pad3 = 0;
	currentNode.quads = NULL;

	currentNode.numMarkers = (UUtUns16) fNumMarkers;
	currentNode.pad4 = 0;
	currentNode.markers = NULL;

	currentNode.numMaterials = (UUtUns16) fNumMaterials;
	currentNode.pad5 = 0;
	currentNode.materials = NULL;

	currentNode.userDataCount = strlen(fUserData) + 1;
	currentNode.userData = NULL;

	fwrite(&currentNode, sizeof(currentNode), 1, stream);
	fwrite(fUserData, sizeof(char), strlen(fUserData) + 1, stream);

	WritePoints(stream);
	
	WriteFaces(stream, WriteOptions_TrianglesOnly);
	WriteFaces(stream, WriteOptions_QuadsOnly);

	WriteMarkers(stream);
	WriteMaterials(stream);
}

void Scene::WritePoints(FILE *stream)
{
	unsigned long pointItr;
	for(pointItr = 0; pointItr < fNumPoints; pointItr++)
	{
		ExportPoint *thisPoint = fPoints + pointItr;
		MXtPoint writePoint;

		writePoint.point.x = thisPoint->x;
		writePoint.point.y = thisPoint->y;
		writePoint.point.z = thisPoint->z;
		writePoint.normal.x = thisPoint->nx;
		writePoint.normal.y = thisPoint->ny;
		writePoint.normal.z = thisPoint->nz;
		writePoint.uv.u = thisPoint->u;
		writePoint.uv.v = thisPoint->v;

		fwrite(&writePoint, sizeof(writePoint), 1, stream);
	}
}

void Scene::WriteFaces(FILE *stream, unsigned long options)
{
	bool trianglesOnly	= (options & WriteOptions_TrianglesOnly) > 0;
	bool quadsOnly		= (options & WriteOptions_QuadsOnly) > 0;

	unsigned long faceItr;
	for(faceItr = 0; faceItr < fNumFaces; faceItr++)
	{
		MXtFace writeFace;
		ExportFace *thisFace = fFaces + faceItr;
		bool		triangle = thisFace->index[3] == INVALID_INDEX;

		if (triangle && quadsOnly) { continue; }
		if (!triangle && trianglesOnly) { continue; }

		writeFace.indices[0] = (UUtUns16) thisFace->index[0];
		writeFace.indices[1] = (UUtUns16) thisFace->index[1];
		writeFace.indices[2] = (UUtUns16) thisFace->index[2];

		if (triangle) {
			writeFace.indices[3] = (UUtUns16) thisFace->index[0];
		}
		else {
			writeFace.indices[3] = (UUtUns16) thisFace->index[3];
		}

		writeFace.normal.x = thisFace->nx;
		writeFace.normal.y = thisFace->ny;
		writeFace.normal.z = thisFace->nz;
		writeFace.material = (UUtUns16) thisFace->material_index;
		writeFace.pad = 0;

		fwrite(&writeFace, sizeof(writeFace), 1, stream);
	}
}

unsigned long Scene::CountTriangles(void)
{
	unsigned long itr;
	unsigned long count = 0;

	for(itr = 0; itr < fNumFaces; itr++)
	{
		if (INVALID_INDEX == fFaces[itr].index[3]) {
			count++;
		}
	}

	return count;
}

unsigned long Scene::CountQuads(void)
{
	unsigned long total = fNumFaces - CountTriangles();

	return total;
}

Scene::~Scene()
{
	Clear();

	assert(NULL == fPoints);
	assert(NULL == fFaces);
	assert(NULL == fEdges);

	gClosecount = 0;
	gDifftcount = 0;

	DebugLogPrintf("************************** DONE EXPORT ************************\r\n");
}

void Scene::Clear(void)
{
	fNumMarkers = 0;
	fNumMaterials = 0;

	fNodeName = "";

	fWarnNonUnitNormals = false;	// turning this off
	fWarnIdenticalUV = true;
	fQuads = false;
	fWarnMapping = true;

	fMaxNormalDistance = 0;
	fMaxNormalDistanceCount = 0;
	fMaxNormalDistanceName = "";

	if (NULL != fUserData) {
		free(fUserData);
		fUserData = NULL;
	}

	if (NULL != fPoints) {
		free(fPoints);	
		fPoints = NULL;
	}

	if (NULL != fFaces) { 
		free(fFaces);
		fFaces = NULL;
	}

	if (NULL != fEdges) {
		free(fEdges);
		fEdges = NULL;
	}

	if (NULL != fGeometries) { 
		free(fGeometries);
		fGeometries = NULL;
	}

	fNumPoints = 0;
	fNumFaces = 0;
	fNumEdges = 0;
	fNumGeometries = 0;
	fAllocatedEdges = 0;

	assert(NULL == fPoints);
	assert(NULL == fFaces);
	assert(NULL == fEdges);
}

typedef struct
{
	unsigned long vertIndex;
	unsigned long tVertIndex;

	ExportPoint point;
} UniquePoint;

static unsigned long FindUniquePoint(Mesh *mesh, unsigned long vertIndex, unsigned long tVertIndex, UniquePoint *list, long count)
{
	Point3 inVertex = mesh->verts[vertIndex];
	Point3 inTVertex(0, 0, 0);
	const float tvert_eps = 0.002f;
	const float vert_eps  = 0.000001f;
	long itr;

	DebugLogPrintf("%f\n", mesh->verts[vertIndex].x);
	DebugLogPrintf("%f\n", mesh->verts[vertIndex].y);
	DebugLogPrintf("%f\n", mesh->verts[vertIndex].z);


	if (mesh->tVerts != NULL)
	{
		inTVertex = mesh->tVerts[tVertIndex];
	}

	DebugLogPrintf("%f\n", inTVertex.x);
	DebugLogPrintf("%f\n", inTVertex.y);
	DebugLogPrintf("%f\n", inTVertex.z);

	DebugLogPrintf("\n");

	for(itr = 0; itr < count; itr++)
	{
		bool reject;
		Point3 curVertex = mesh->verts[list[itr].vertIndex];
		Point3 curTVertex(0, 0, 0);

		if (mesh->tVerts)
		{
			curTVertex = mesh->tVerts[list[itr].tVertIndex];
		}

		if ((tVertIndex != list[itr].tVertIndex) && (vertIndex == list[itr].vertIndex))
		{
		}

		reject = false;

		if (curVertex.x + vert_eps <= inVertex.x) continue;
		if (curVertex.y + vert_eps <= inVertex.y) continue;
		if (curVertex.z + vert_eps <= inVertex.z) continue;
		if (curVertex.x - vert_eps >= inVertex.x) continue;
		if (curVertex.y - vert_eps >= inVertex.y) continue;
		if (curVertex.z - vert_eps >= inVertex.z) continue;

		if (mesh->tVerts != NULL) {
			if ((inTVertex.x + tvert_eps) <= curTVertex.x) reject = true;
			if ((inTVertex.y + tvert_eps) <= curTVertex.y) reject = true;
			if ((inTVertex.x - tvert_eps) >= curTVertex.x) reject = true;
			if ((inTVertex.y - tvert_eps) >= curTVertex.y) reject = true;
		}

		if (reject) 
		{
			gDifftcount++;
			DebugLogPrintf("FindUniquePoint [reject] (%f, %f, %f) [%f, %f]\r\nout (%f %f %f) [%f %f]\r\n%d,%d v %d,%d count=%d\r\n", 
						inVertex.x, inVertex.y, inVertex.z, inTVertex.x, inTVertex.y,
						curVertex.x, curVertex.y, curVertex.z, curTVertex.x, curTVertex.y,
						vertIndex, tVertIndex, list[itr].vertIndex, list[itr].tVertIndex, gDifftcount);

			continue;
		}

		if ((tVertIndex != list[itr].tVertIndex) || (vertIndex != list[itr].vertIndex))
		{
			gClosecount += 1;
			DebugLogPrintf("FindUniquePoint [accept] (%f, %f, %f) [%f, %f]\r\nout (%f %f %f) [%f %f]\r\n%d,%d v %d,%d count=%d\r\n", 
						inVertex.x, inVertex.y, inVertex.z, inTVertex.x, inTVertex.y,
						curVertex.x, curVertex.y, curVertex.z, curTVertex.x, curTVertex.y,
						vertIndex, tVertIndex, list[itr].vertIndex, list[itr].tVertIndex, gClosecount);
		}

		return itr;
	}

	return 0xffffffff;
}

static void AddUniquePoint(Mesh *mesh, Matrix3 tm, unsigned long vertIndex, unsigned long tVertIndex, UniquePoint **ioList, long *ioCount)
{
	unsigned long index;
	long count = *ioCount;
	UniquePoint *list = *ioList;



	// step 1 scan for the point
	index = FindUniquePoint(mesh, vertIndex, tVertIndex, list, count);

	// if we found it then return
	if (0xffffffff != index) { return; }

	// step 2 add if not found
	list = (UniquePoint *) realloc(list, (count + 1) * sizeof(UniquePoint));
	assert(NULL != list);	// failed to allocate

	list[count].vertIndex = vertIndex;
	list[count].tVertIndex = tVertIndex;

	Point3 vertex = mesh->verts[vertIndex];
	vertex = Scene::TranslatePoint(vertex, tm);

	list[count].point.x = vertex.x;
	list[count].point.y = vertex.y;
	list[count].point.z = vertex.z;

	Point3 normal = mesh->getNormal(vertIndex);
	normal = Scene::TranslateNormal(normal, tm);

	list[count].point.nx = normal.x;
	list[count].point.ny = normal.y; 
	list[count].point.nz = normal.z;

	if (NULL == mesh->tVerts)
	{
		list[count].point.u = 0;
		list[count].point.v = 0;
	}
	else
	{
		list[count].point.u = mesh->tVerts[tVertIndex].x;
		list[count].point.v = mesh->tVerts[tVertIndex].y;
	}

	count += 1;

	// setup the return values
	*ioCount = count;
	*ioList = list;
}

int Scene::GetExportMaterialIndex(
	Mtl *material,
	int sub_material_index)
{
	int result= -1;

	for (int i= 0; i<fNumMaterials; ++i)
	{
		if (fMaterials[i].material==material &&
			fMaterials[i].sub_material_index==sub_material_index)
		{
			result= i;
			break;
		}
	}

	if (result==-1)
	{
		if (fNumMaterials<MAXIMUM_EXPORT_MATERIALS)
		{
			fMaterials[fNumMaterials].material= material;
			fMaterials[fNumMaterials].sub_material_index= sub_material_index;
			result= fNumMaterials;

			fNumMaterials+= 1;
		}
	}

	return result;
}

void Scene::AddNode(INode *node, unsigned long options)
{
	Matrix3 tm;

	tm.IdentityMatrix();

	AddNode(node, tm, options);
}

void Scene::AddNode(INode *node, Matrix3 inTM, unsigned long options)
{
	bool useNodeTM		= (options & AddOptions_IgnoreNodeRTTM) == 0;
	bool geometryOnly	= (options & AddOptions_SupportNonGeometry) == 0;
	bool ignoreBiped	= (options & AddOptions_IgnoredBiped) > 0;
	bool ignoreHidden	= !(options & AddOptions_AddHidden);
	bool noMatrix		= (options & AddOptions_NoMatrix) > 0;

	bool isBiped		= INodeIsBiped(node);
	bool isGeometry		= INodeIsGeomObject(node);
	bool isHidden		= node->IsHidden() > 0;
	
	if ((isBiped) && (ignoreBiped)) 
	{
		return;
	}

	if ((!isGeometry) && (geometryOnly)) 
	{
		return;
	}

	if (ignoreHidden && isHidden) 
	{
		return;
	}
	
	fNodeName = node->GetName();
	assert(fNodeName[0] != '#');

	Matrix3 tm;
	Matrix3 fix = CoordinateFix();

	if (noMatrix)
	{
		tm.IdentityMatrix();
	}
	else if (useNodeTM)
	{
		tm = GetObjectTM(node, fTime);	// object is total world space
		tm = tm * fix;
	}
	else 
	{
		Matrix3 tm1 = node->GetNodeTM(fTime);
		Matrix3 tm2 = GetObjectOnlyTM(node, fTime);
		Matrix3 objTM = GetObjectTM(node, fTime);

		tm1 = StripRotTrans(tm1);

		tm = tm1 * tm2;
		tm = tm * fix;
	}

	tm = inTM * tm;

	TSTR user_prop_buffer;
	node->GetUserPropBuffer(user_prop_buffer);
	
	if (fUserData != NULL) {
		fUserData = (char *) realloc(fUserData, strlen(fUserData) + user_prop_buffer.Length() + 2);
		switch (fUserData[strlen(fUserData)])
		{
			case '\n':
			case '\r':
			case '\0':
			break;

			default:
				strcat(fUserData, "\n");
			break;
		}
		strcat(fUserData, user_prop_buffer);
	}
	else { 
		fUserData = (char *) realloc(fUserData, user_prop_buffer.Length() + 1);
		strcpy(fUserData, user_prop_buffer);
	}

	// we should not have converted to quads yet
	assert(false == fQuads);

	// get the object
	Object *object = node->EvalWorldState(fTime).obj;

	// if we cannot be a triangle mesh then we can't add this node
	// this happens silently
	if (!object->CanConvertToType(triObjectClassID)) {
		return;
	}

	unsigned long thisGeometry = fNumGeometries;
	fNumGeometries += 1;

	TriObject	*triangle		= (TriObject *) object->ConvertToType(fTime, triObjectClassID);
	Mesh		*mesh			= &(triangle->mesh);
	bool		deleteTriangle	= (object) != ((Object *) triangle);

	assert(NULL != triangle);
	assert(NULL != mesh);

	boolean		isTextureMapping = NULL != mesh->tVerts;

	long vertItr; 
	long faceItr;
	UniquePoint *pointList = NULL;
	long newPoints = 0;

	// build normals
	mesh->buildNormals();

	Mtl* base_material= node->GetMtl();
	short sub_material_count= base_material ? base_material->NumSubMtls() : 0;

	// construct the entire list of points

	// iterate through every face adding unique pairs of vert,tvert
	// each unqiue pair represents a single vertext in our engine
	for(faceItr = 0; faceItr < mesh->numFaces; faceItr++)
	{
		Face *face		= mesh->faces + faceItr;
		TVFace *tvFace	= NULL;
			
		if (isTextureMapping)
		{
			tvFace = mesh->tvFace + faceItr;
		}

		DebugLogPrintf("face:");
		for(vertItr = 0; vertItr < 3; vertItr++)
		{
			unsigned long tVert = 0;

			if (isTextureMapping) 
			{
				tVert = tvFace->t[vertItr];
			}

			AddUniquePoint(mesh, tm, face->v[vertItr], tVert, &pointList, &newPoints);
		}
	}

	// grow the list of points by the max of texture or regular verts
	unsigned long vertOffset			= fNumPoints;

	fNumPoints += newPoints;
	fPoints = (ExportPoint *) realloc(fPoints, fNumPoints * sizeof(ExportPoint));
	assert((0 == fPoints) || (NULL != fPoints));	// failed to allocate

	// copy the points from the unique point list into the point list
	for(vertItr = 0; vertItr < newPoints; vertItr++)
	{
		fPoints[vertItr + vertOffset] = pointList[vertItr].point;
	}

	// grow the list of faces
	unsigned long faceOffset = fNumFaces;
	fNumFaces += mesh->numFaces;

	fFaces = (ExportFace *) realloc(fFaces, fNumFaces * sizeof(ExportFace));
	assert((0 == fFaces) || (NULL != fFaces));	// failed to allocate

	// copy the faces
	for(faceItr = 0; faceItr < mesh->numFaces; faceItr++)
	{	
		// find the max faces that make this face
		Face *face		= mesh->faces + faceItr;
		TVFace *tvFace	= NULL;

		if (isTextureMapping) 
		{
			tvFace = mesh->tvFace + faceItr;
		}

		// find the face to write to
		ExportFace *outFace	= fFaces + faceItr + faceOffset;

		// geometry
		outFace->geometry = thisGeometry;

		// normals
		Point3 faceNormal = mesh->getFaceNormal(faceItr);
		faceNormal = TranslateNormal(faceNormal, tm);

		outFace->nx = faceNormal.x;
		outFace->ny = faceNormal.y;
		outFace->nz = faceNormal.z;

		outFace->material_index= GetExportMaterialIndex(base_material,
			sub_material_count ? (face->getMatID()%sub_material_count) : -1);

		// vertex indices
		for(vertItr = 0; vertItr < 3; vertItr++)
		{
			unsigned long tVert = 0;
			unsigned long vert = face->v[vertItr];

			if (isTextureMapping) 
			{
				tVert = tvFace->t[vertItr];
			}

			unsigned long foundPoint = FindUniquePoint(mesh, vert, tVert, pointList, newPoints);
			assert(0xffffffff != foundPoint);

			outFace->index[vertItr] = foundPoint + vertOffset;
		}
		outFace->index[3] = INVALID_INDEX;
		// edges
		AddEdges(node, face, faceItr + faceOffset, vertOffset);
	}

	VerifyEdges();

	// free our unique list
	if (NULL != pointList)
	{
		free(pointList); 
	}

	// free up max space if required
	if (deleteTriangle) {
		assert(NULL != triangle);

		delete triangle;
	}

	AddMarkersForNode(node);
}

void Scene::AddMarkersForNode(INode *node)
{
	INode **markerList = NodeList::GetNodeList(fScene, fTime, kCountMarkerNodes);
	int count = NodeCounter::CountNodes(fScene, fTime, kCountMarkerNodes);
	int itr;


	for(itr = 0; itr < count; itr++)
	{
		if (markerList[itr]->GetParentNode() != node) {
			continue;
		}

		AddThisMarker(markerList[itr]);
	}

	return;
}

void Scene::AddThisMarker(INode *node)
{
	char *name = node->GetName();
	Matrix3 tm;
	Matrix3 fix = Scene::CoordinateFix();

	tm = GetObjectTM(node, fTime);
	tm = tm * fix;

	TSTR user_prop_buffer;
	node->GetUserPropBuffer(user_prop_buffer);
	
	if (fNumMarkers<MAXIMUM_EXPORT_MARKERS)
	{
		fMarkers[fNumMarkers].name= name;
		fMarkers[fNumMarkers].matrix= tm;
		fMarkers[fNumMarkers].user_data = user_prop_buffer;

		fNumMarkers+= 1;
	}
}


void Scene::AddScene(Scene *pScene)
{
	assert(this != pScene);

	this->VerifyFaces();
	this->VerifyEdges();

	pScene->VerifyFaces();
	pScene->VerifyEdges();

	unsigned long oldNumPoints	= fNumPoints;
	unsigned long oldNumFaces	= fNumFaces;
	unsigned long oldNumEdges	= fNumEdges;

	unsigned long newNumPoints	= oldNumPoints + pScene->fNumPoints;
	unsigned long newNumFaces	= oldNumFaces + pScene->fNumFaces;
	unsigned long newNumEdges	= oldNumEdges + pScene->fNumEdges;

	// mark the counts up
	fNumPoints = newNumPoints;
	fNumFaces = newNumFaces;
	fNumEdges = newNumEdges;
		
	// grow the lists
	GrowPointList(newNumPoints);
	GrowFaceList(newNumFaces);
	GrowEdgeList(newNumEdges);

	// copy those lists over & patch up the references
	unsigned long itr;
	
	for(itr = oldNumPoints; itr < newNumPoints; itr++) {
		ExportPoint *dstPoint = fPoints + itr;
		ExportPoint *srcPoint = pScene->fPoints + (itr - oldNumPoints);

		*dstPoint = *srcPoint;
	}

	for(itr = oldNumFaces; itr < newNumFaces; itr++) {
		ExportFace *dstFace = fFaces + itr;
		ExportFace *srcFace = pScene->fFaces + (itr - oldNumFaces);
	
		// copy the entire face
		*dstFace = *srcFace;

		// fixup the indices tothe points
		unsigned long fixup;
		for(fixup = 0; fixup < 4; fixup++) {
			if (INVALID_INDEX == dstFace->index[fixup]) {
				continue;
			}

			dstFace->index[fixup] += oldNumPoints;
		}
	}

	VerifyFaces();

	for(itr = oldNumEdges; itr < newNumEdges; itr++) {
		ExportEdge *dstEdge = fEdges + itr;
		ExportEdge *srcEdge = pScene->fEdges + (itr - oldNumEdges);

		pScene->VerifyEdge(srcEdge);

		// copy the entire edge
		*dstEdge = *srcEdge;

		dstEdge->v1 += oldNumPoints;
		dstEdge->v2 += oldNumPoints;

		if (INVALID_INDEX != dstEdge->f1) {
			dstEdge->f1 += oldNumFaces;
		}

		if (INVALID_INDEX != dstEdge->f2) {
			dstEdge->f2 += oldNumFaces;
		}

		VerifyEdge(dstEdge);
	}

	VerifyEdges();
}


void Scene::GrowPointList(unsigned long newTotal)
{
	fPoints	= (ExportPoint *) realloc(fPoints, newTotal * sizeof(ExportPoint));
}

void Scene::GrowFaceList(unsigned long newTotal)
{
	fFaces	= (ExportFace *) realloc(fFaces, newTotal * sizeof(ExportFace));

	// clear the unused area to try to track down a memory bug
	memset(fFaces + fNumFaces, 0, sizeof(ExportFace) * (newTotal - fNumFaces));
}

void Scene::GrowEdgeList(unsigned long newTotal)
{
	if (newTotal > fAllocatedEdges)
	{
		// at least allocate the right amount
		fAllocatedEdges = newTotal;

		if (newTotal < 2000) {
			fAllocatedEdges += 1000;
		} else {
			fAllocatedEdges += newTotal / 2;
		}

		fEdges = (ExportEdge *) realloc(fEdges, fAllocatedEdges * sizeof(ExportEdge));
		assert(NULL != fEdges);
	}
}

void Scene::Multiply(Matrix3 &tm)
{
	// 1.	fixup the verticies
	//		fixup the vertex normals

	// 2.	fixup the face normals

	// 3.	assertion about bounding boxes perhaps ?


	unsigned long itr;

	for(itr = 0; itr < fNumPoints; itr++)
	{
		ExportPoint *thisPoint = fPoints + itr;

		Point3 point(thisPoint->x, thisPoint->y, thisPoint->z);
		Point3 normal(thisPoint->nx, thisPoint->ny, thisPoint->nz);

		point	= TranslatePoint(point, tm);
		normal	= TranslateNormal(normal, tm);

		thisPoint->x = point.x;
		thisPoint->y = point.y;
		thisPoint->z = point.z;

		thisPoint->nx = normal.x;
		thisPoint->ny = normal.y;
		thisPoint->nz = normal.z;
	}

	for(itr = 0; itr < fNumFaces; itr++)
	{
		ExportFace *thisFace = fFaces + itr;

		Point3 normal(thisFace->nx, thisFace->ny, thisFace->nz);

		normal = TranslateNormal(normal, tm);

		thisFace->nx = normal.x;
		thisFace->ny = normal.y;
		thisFace->nz = normal.z;
	}
}


// local TM is the matrix that this node
// will pass on to its child that is in addition
// to its parent (like you would use in a matrix
// stack).
Matrix3	GetLocalTM(INode *node, TimeValue time)
{
	assert(NULL != node);

	Matrix3 nodeTM		= node->GetNodeTM(time);
	Matrix3 parentTM	= node->GetParentTM(time);
	Matrix3 localTM		= nodeTM * Inverse(parentTM);

	AffineParts nodeParts;
	AffineParts parentParts;
	AffineParts localParts;

	my_decomp_affine(nodeTM, &nodeParts);
	my_decomp_affine(parentTM, &parentParts);
	my_decomp_affine(localTM, &localParts);	

	return localTM;
}


Matrix3 GetNoScaleLocalTM(INode *node, TimeValue time)
{
	Matrix3 nodeTM		= node->GetNodeTM(time);
	Matrix3 parentTM	= node->GetParentTM(time);

	parentTM = StripScale(parentTM);
	nodeTM = StripScale(nodeTM);

	Matrix3 localTM		= nodeTM * Inverse(parentTM);

	return localTM;
}


// nodeTM is all the matrixes that apply to this
// node that will be inherited
Matrix3	GetNodeTM(INode *node, TimeValue time)
{
	assert(NULL != node);

	Matrix3 nodeTM		= node->GetNodeTM(time);
	
	return nodeTM;
}


// objectTM is the total local to world space transformation matrix
Matrix3	GetObjectTM(INode *node, TimeValue time)
{
	assert(NULL != node);

	Matrix3 objectTM	= node->GetObjectTM(time);
	
	return objectTM;
}


		// this is only the tm for our object (node tm removed)
Matrix3 GetObjectOnlyTM(INode *node, TimeValue time)
{
	assert(NULL != node);

	Matrix3 objectTM		= node->GetObjectTM(time);
	Matrix3 nodeTM			= node->GetNodeTM(time);
	Matrix3 objectOnlyTM	= objectTM * Inverse(nodeTM);

	return objectOnlyTM;
}

typedef enum {
	kKeepScale,
	kStripScale
} tStripScale;

typedef enum {
	kKeepRotate,
	kStripRotate
} tStripRotate;

typedef enum {
	kKeepTranslate,
	kStripTranslate
} tStripTranslate;

static Matrix3 MatrixStripper(const Matrix3 inMatrix, tStripScale stripScale, tStripRotate stripRotate, tStripTranslate stripTranslate)
{
	Matrix3		outMatrix;
	Matrix3		testMatrix;
	AffineParts inParts;
	AffineParts outParts;
	AffineParts testParts;
	
	my_decomp_affine(inMatrix, &inParts);

	// translate
	Matrix3 translateTM = TransMatrix(inParts.t);

	// rotate 
	Matrix3	rotateTM;
	inParts.q.MakeMatrix(rotateTM);

	// stretch rotate
	Matrix3 stretchRotateTM;
	inParts.u.MakeMatrix(stretchRotateTM);

	// stretch
	Matrix3 stretchTM = ScaleMatrix(inParts.k);

	// inverse stretch rotate
	Matrix3 invStretchRotateTM;
	Quat invU = inParts.u;
	invU.MakeMatrix(invStretchRotateTM);

	// this should just be original matrix
	testMatrix = (stretchRotateTM * stretchTM * invStretchRotateTM) * rotateTM * translateTM;
	my_decomp_affine(testMatrix, &testParts);

	// ============ strip the parts we don't need ============
	if (kStripScale == stripScale) {
		stretchRotateTM.IdentityMatrix();
		stretchTM.IdentityMatrix();
		invStretchRotateTM.IdentityMatrix();
	}

	if (kStripRotate == stripRotate) {
		rotateTM.IdentityMatrix();
	}

	if (kStripTranslate == stripTranslate) {
		translateTM.IdentityMatrix();
	}

	outMatrix = (stretchRotateTM * stretchTM * invStretchRotateTM) * rotateTM * translateTM;

	my_decomp_affine(outMatrix, &outParts);

	return outMatrix;
}

Matrix3	StripScale(Matrix3 inMatrix)
{
	Matrix3 result;

	result = MatrixStripper(inMatrix, kStripScale, kKeepRotate, kKeepTranslate);

	return result;
}

Matrix3	StripRotate(Matrix3 inMatrix)
{
	Matrix3 result;

	result = MatrixStripper(inMatrix, kKeepScale, kStripRotate, kKeepTranslate);

	return result;
}

Matrix3 StripTranslate(Matrix3 inMatrix)
{
	Matrix3 result;

	result = MatrixStripper(inMatrix, kKeepScale, kKeepRotate, kStripTranslate);
	
	return result;
}

Matrix3 StripRotTrans(Matrix3 inMatrix)
{
	Matrix3 result;
	Matrix3 rottrans;
	Matrix3 test;
	AffineParts testParts;

	result = MatrixStripper(inMatrix, kKeepScale, kStripRotate, kStripTranslate);

	rottrans = StripScale(inMatrix);
	test = result * rottrans;		// test involving reversing the order

	my_decomp_affine(test, &testParts);
	
	return result;
}


/*

AddEdges

inFace 

*/

void Scene::AddEdges(INode *node, Face *inMaxFace, unsigned long faceIndex, const unsigned long vertOffset)
{
	assert(NULL != inMaxFace);
	assert(vertOffset < 0x10000000);

	unsigned long itr;
	ExportFace *face = fFaces + faceIndex;

	for(itr = 0; itr < 3; itr++) {
		int visability	= inMaxFace->getEdgeVis(itr);
		int v1			= face->index[itr];	
		int v2			= face->index[((itr + 1) % 3)];

		assert(v1 >= 0);
		assert(v2 >= 0);
		assert((unsigned long) v1 < fNumPoints);
		assert((unsigned long) v2 < fNumPoints);
		assert(FaceHasVertex(face, v1));
		assert(FaceHasVertex(face, v2));

		ExportEdge edge;

		assert((0 == visability) || (EDGE_A == visability) || (EDGE_B == visability) || (EDGE_C == visability));

		edge.node = node;
		edge.v1 = v1;
		edge.v2 = v2;
		edge.visable = visability != 0;
		edge.f1 = faceIndex;
		edge.f2 = INVALID_INDEX;

		VerifyEdge(&edge);
		AddEdgeIfUnique(edge, faceIndex);
	}
}

void Scene::AddEdgeIfUnique(ExportEdge &newEdge, unsigned long faceIndex)
{
	unsigned long itr;

	for(itr = 0; itr < fNumEdges; itr++) {
		assert(NULL != fEdges);

		ExportEdge *thisEdge = fEdges + itr;

		if (((thisEdge->v1 == newEdge.v1) && (thisEdge->v2 == newEdge.v2)) ||
			 ((thisEdge->v1 == newEdge.v2) && (thisEdge->v2 == newEdge.v1))) 
		{				
			// if old edge is invisable and new edge is not
			// make this edge visiable as well
			if ((newEdge.visable) && (!thisEdge->visable)) {
				thisEdge->visable = true;
			}

			// an invisable edge should be only be shared by two faces
			if (INVALID_INDEX != thisEdge->f2)
			{
				char buf[512];
				sprintf(buf, "Node %s has an invisiable edge shared by multiple faces.", thisEdge->node->GetName());
				MessageBox(NULL, buf, "WARNING", MB_OK);

				break;
			}
			thisEdge->f2 = faceIndex;

			VerifyEdge(thisEdge);
			return;
		}
	}

	// no duplicate found, so add this edge
	if ((fNumEdges + 1) > fAllocatedEdges)
	{
		if (fNumEdges < 2000) {
			fAllocatedEdges += 1000;
		} else {
			fAllocatedEdges += fNumEdges / 2;
		}

		fEdges = (ExportEdge *) realloc(fEdges, fAllocatedEdges * sizeof(ExportEdge));
		assert(NULL != fEdges);
	}

	fEdges[fNumEdges] = newEdge;
	VerifyEdge(&(fEdges[fNumEdges]));
	fNumEdges += 1;
}

static float sqr(float x) 
{
	return x * x;
}

static float NormalDistance(const ExportFace *f1, const ExportFace *f2)
{
	float distance = sqr(f1->nx - f2->nx) + sqr(f1->ny - f2->ny) + sqr(f1->nz - f2->nz);
	distance = (float) sqrt(distance);

	return distance;
}

ExportFace Scene::MergeFaces(ExportEdge edge)
{
	VerifyEdge(&edge);

	ExportFace result = fFaces[edge.f1];
	unsigned long itr;
	ExportFace *f1 = fFaces + edge.f1;
	ExportFace *f2 = fFaces + edge.f2;
	
	float distance = NormalDistance(f1, f2);

	if (distance > fMaxNormalDistance)
	{
		fMaxNormalDistance = distance;
	}

	if (distance > MAX_NORMAL_DISTANCE)
	{
		fMaxNormalDistanceCount += 1;
	}

	// step 1 find shared span in the first triangle
	// step 2 add first triangle so that the span is the trailing edge (2..0)
	// step 3 add point from remaining triangle

	// step 1 find shared span in the first triangle

	// starting at vertex 0 of the triangle, walk
	// around the triangle untill we have hit both edges
	// on the span in a row, then stop we are now
	// at the begiinning of the non span part

	unsigned long thisIndex = 0;
	unsigned long nextIndex = 1;
	unsigned long spanEnd;

	while(1)
	{
		unsigned long thisPointIndex = f1->index[thisIndex];
		unsigned long nextPointIndex = f1->index[nextIndex];

		if (((thisPointIndex == edge.v1) && (nextPointIndex == edge.v2)) || 
			((thisPointIndex == edge.v2) && (nextPointIndex == edge.v1)))
		{
			// ok we found the span
			spanEnd = nextIndex;
			break;
		}		

		thisIndex = (thisIndex + 1) % 3;		// increment mod 3
		nextIndex = (nextIndex + 1) % 3;

		bool infiniteLoop = (0 == thisIndex);
		assert(!infiniteLoop);
	}

	// step 2 add first triangle so that the span is the trailing edge (2..0)
	unsigned long fromItr = spanEnd;
	for(itr = 0; itr < 3; itr++)
	{
		result.index[itr] = f1->index[fromItr];

		fromItr = (fromItr + 1) % 3;
	}

	// step 3 add point from remaining triangle
	for(itr = 0; itr < 3; itr++)
	{
		unsigned long pointIndex = f2->index[itr];

		if ((pointIndex != edge.v1) && (pointIndex != edge.v2))
		{
			result.index[3] = pointIndex;
		}
	}

	return result;
}

void Scene::FacesToQuads(unsigned long options)
{
	bool exact		= QuadOptions_Exact == (options & QuadOptions_Exact);

	// step 1 mark the faces as unmarked
	// step 2 iterate through the invisible edges
	//				- add their merged faces to the quad list
	//				- mark them
	// step 3 iterate through the visible faces
	//				- move them to the merged list

	ExportFace *outQuads = NULL;
	unsigned long numQuads = 0;

	unsigned long itr;

	for(itr = 0; itr < fNumFaces; itr++) {
		fFaces[itr].marked = false;
	}

	for(itr = 0; itr < fNumEdges; itr++) {
		if (fEdges[itr].visable) { continue; }

		// take the quad made by these two faces and
		unsigned long faceIndex1 = fEdges[itr].f1;
		unsigned long faceIndex2 = fEdges[itr].f2;

		// this can happen in the odd  texture mapping case
		if ((faceIndex1 == INVALID_INDEX) || (faceIndex2 == INVALID_INDEX)) {
			continue;
		}

		assert(faceIndex1 != INVALID_INDEX);
		assert(faceIndex2 != INVALID_INDEX);

		ExportFace *face1 = fFaces + faceIndex1;
		ExportFace *face2 = fFaces + faceIndex2;

		if ((face1->marked) || (face2->marked)) {
			continue;
		}
		
		float normalDistance = NormalDistance(face1, face2);
		if (normalDistance > MAX_NORMAL_DISTANCE) { continue; }

		face1->marked = true;
		face2->marked = true;

		// merge these two faces
		ExportFace newFace = MergeFaces(fEdges[itr]);

		// add them to the outFaces list (and grow it)
		outQuads = (ExportFace *) realloc(outQuads, (numQuads + 1) * sizeof(ExportFace));
		assert(NULL != outQuads);	// failed to allocate
		outQuads[numQuads] = newFace;
		numQuads += 1;
	}

	
	// now we tag all the triangles to the end of the list of quads
	for(itr = 0; itr < fNumFaces; itr++) {
		if (fFaces[itr].marked) { continue; }

		ExportFace triangle = fFaces[itr];

		outQuads = (ExportFace *) realloc(outQuads, (numQuads + 1)* sizeof(ExportFace));
		assert(NULL != outQuads);

		outQuads[numQuads] = triangle;
		numQuads += 1;
	}

	// this message is obsolete, but it is left here because the root cause
	// needs to be fixed
	if (fMaxNormalDistance > MAX_NORMAL_DISTANCE) {
		char msg2[255];
		sprintf(msg2, "Found some normals that were quite far apart (%d > %f max of %f).", 
			fMaxNormalDistanceCount, MAX_NORMAL_DISTANCE, fMaxNormalDistance);
		MessageBox(NULL, msg2, "WARNING", MB_OK);
	}

	// replace the old face list with the current working list
	if (fFaces != NULL) { 
		free(fFaces); 
	}

	fFaces = outQuads;
	fNumFaces = numQuads;
	fQuads = true;
}


void Scene::RoundPoints(void)
{
	assert(0);
}

void Scene::ClearUV(void)
{
	unsigned long itr;

	for(itr = 0; itr < fNumPoints; itr++)
	{
		fPoints[itr].u = 0;
		fPoints[itr].v = 0;
	}
}

void Scene::RemoveDuplicatesOfPoint(unsigned long original)
{
	unsigned long duplicate = original + 1;

	while(duplicate < fNumPoints) 
	{
		if ((fPoints[duplicate].x != fPoints[original].x) || 
			(fPoints[duplicate].y != fPoints[original].y) ||
			(fPoints[duplicate].z != fPoints[original].z) ||
			(fPoints[duplicate].u != fPoints[original].u) ||
			(fPoints[duplicate].v != fPoints[original].v))
		{
				// this wasn't a duplicate, is the next one ?
				duplicate += 1;
				continue;
		}

		// if we got to here then we did find a duplicate
		// no we need to replace all references to duplicate with original
		// and shift everything > original down by 1

		unsigned long faceItr;
		for(faceItr = 0; faceItr < fNumFaces; faceItr++) {
			unsigned long indexItr;

			for(indexItr = 0; indexItr < 4; indexItr++) {
				if (INVALID_INDEX == fFaces[faceItr].index[indexItr]) {
					continue;
				}

				if (fFaces[faceItr].index[indexItr] == duplicate) {
					fFaces[faceItr].index[indexItr] = original;
				}
				else if (fFaces[faceItr].index[indexItr] > duplicate) {
					fFaces[faceItr].index[indexItr] -= 1;
				}
			}
		}

		CompressArray(fPoints, duplicate, fNumPoints, sizeof(ExportPoint));
		fNumPoints--;

		// note we want to retry with the same duplicate since everything
		// got shifted down (dont increment duplicate)
	}
}

void Scene::CompressPoints(void)
{
	unsigned long i;

	for(i = 0; i < fNumPoints; i++)
	{
		RemoveDuplicatesOfPoint(i);
	}
}

void Scene::GrowBBox(ExportBBox *bbox, unsigned long pointIndex)
{
	if (INVALID_INDEX == pointIndex) { return; }

	assert(pointIndex < fNumPoints);

	ExportPoint *pPoint = fPoints + pointIndex;

	if (bbox->valid) {
		bbox->xmin = UUmMin(pPoint->x, bbox->xmin);
		bbox->xmax = UUmMax(pPoint->x, bbox->xmax);

		bbox->ymin = UUmMin(pPoint->y, bbox->ymin);
		bbox->ymax = UUmMax(pPoint->y, bbox->ymax);

		bbox->zmin = UUmMin(pPoint->z, bbox->zmin);
		bbox->zmax = UUmMax(pPoint->z, bbox->zmax);
	}
	else {
		bbox->xmin = pPoint->x;
		bbox->xmax = pPoint->x;

		bbox->ymin = pPoint->y;
		bbox->ymax = pPoint->y;

		bbox->zmin = pPoint->z;
		bbox->zmax = pPoint->z;

		bbox->valid = true;
	}
}

bool Scene::FaceHasVertex(ExportFace *face, unsigned long vertex)
{
	unsigned long itr;
	bool found = false;
	for(itr = 0; itr < 4; itr++)
	{
		if (vertex == face->index[itr]) {
			found = true;
			break;
		}
	}

	return found;
}

void Scene::VerifyFace(ExportFace *face)
{
	assert(face->index[0] < fNumPoints);
	assert(face->index[1] < fNumPoints);
	assert(face->index[2] < fNumPoints);
	assert((face->index[3] < fNumPoints) || (face->index[3] == INVALID_INDEX));
}

void Scene::VerifyFaces(void)
{
	unsigned long itr;

	for(itr = 0; itr < fNumFaces; itr++) {
		ExportFace *thisFace = fFaces + itr;

		VerifyFace(thisFace);
	}
}

void Scene::VerifyEdge(ExportEdge *edge)
{
	unsigned long v1 = edge->v1;
	unsigned long v2 = edge->v2;

	if (INVALID_INDEX != edge->f1) {
		ExportFace *face1 = fFaces + edge->f1;

		VerifyFace(face1);

		assert(FaceHasVertex(face1, v1));
		assert(FaceHasVertex(face1, v2));
	}

	if (INVALID_INDEX != edge->f2) {
		ExportFace *face2 = fFaces + edge->f2;

		VerifyFace(face2);

		assert(FaceHasVertex(face2, v1));
		assert(FaceHasVertex(face2, v2));
	}
}
					

void Scene::VerifyEdges(void)
{
	unsigned long itr;

	for(itr = 0; itr < fNumEdges; itr++)
	{
		ExportEdge *edge = fEdges + itr;

		VerifyEdge(edge);
	}
}

Matrix3 Scene::CoordinateFix(void)
{
	// apply the coordinate system fix

	// x = x
	// y = z
	// z = -y

	// i guess the result is:
	// 0 0 1
	// 0 1 0
	// - 0 0

	Point3 row0(0,0,1);
	Point3 row1(0,1,0);
	Point3 row2(-1,0, 0);
	Point3 row3(0,0,0);

	Matrix3 tm;

	tm.SetRow(0, row0);
	tm.SetRow(1, row1);
	tm.SetRow(2, row2);
	tm.SetRow(3, row3);	

	tm = tm * RotateYMatrix(0.25f * (2 * PI));
	tm = tm * RotateXMatrix(0.75f * (2 * PI));

	return tm;
}

// translates all the points
void Scene::OffsetPoints(Point3 offset)
{
	unsigned int itr;

	for(itr = 0; itr < fNumPoints; itr++)
	{
		fPoints[itr].x += offset.x;
		fPoints[itr].y += offset.y;
		fPoints[itr].z += offset.z;
	}
}

Point3 Scene::FindCenter(void)
{
	unsigned int itr;
	Point3 center(0,0,0);

	if (0 == fNumPoints) { return center; }

	float min_x = fPoints[0].x;
	float min_y = fPoints[0].y;
	float min_z = fPoints[0].z;

	float max_x = fPoints[0].x;
	float max_y = fPoints[0].y;
	float max_z = fPoints[0].z;

	for(itr = 0; itr < fNumPoints; itr++)
	{
		min_x = min(min_x, fPoints[itr].x);
		min_y = min(min_y, fPoints[itr].y);
		min_z = min(min_z, fPoints[itr].z);

		max_x = max(max_x, fPoints[itr].x);
		max_y = max(max_y, fPoints[itr].y);
		max_z = max(max_z, fPoints[itr].z);
	}

	center.x = (min_x + max_x) / 2;
	center.y = (min_y + max_y) / 2;
	center.z = (min_z + max_z) / 2;

	return center;
}


FILE *fopen_safe(const char *name, const char *mode)
{
	FILE *stream;
	int result;

	do
	{
		stream = fopen(name , mode);

		if (NULL != stream) { break; }

		char msg[255];
		sprintf(msg, "Failed to open file %s (mode %s).", name, mode);

		result = MessageBox(NULL, msg, "warning", MB_RETRYCANCEL);
	} while(IDCANCEL != result);

	return stream;
}

void PrintMatrix(FILE *stream, const Matrix3 &inMatrix)
{
	Matrix3 matrix	= inMatrix;
//	Point4 row0		= matrix.GetColumn(0);
//	Point4 row1		= matrix.GetColumn(1);
//	Point4 row2		= matrix.GetColumn(2);

	Point3 col0		= matrix.GetRow(0);
	Point3 col1		= matrix.GetRow(1);
	Point3 col2		= matrix.GetRow(2);
	Point3 col3		= matrix.GetRow(3);

	fprintf(stream, "%f\t%f\t%f\t%f\r\n", col0.x, col1.x, col2.x, col3.x);
	fprintf(stream, "%f\t%f\t%f\t%f\r\n", col0.y, col1.y, col2.y, col3.y);
	fprintf(stream, "%f\t%f\t%f\t%f\r\n", col0.z, col1.z, col2.z, col3.z);
}

void PrintMatrixEuler(FILE *stream, const Matrix3 &matrix)
{
	AffineParts parts;
	float euler[3];

	my_decomp_affine(matrix, &parts);
	QuatToEuler(parts.q, euler);

	PrintEuler(stream, euler);
}

void PrintMatrixRotate(FILE *stream, const Matrix3 &matrix)
{
	AffineParts parts;
	float euler[3];

	my_decomp_affine(matrix, &parts);
	QuatToEuler(parts.q, euler);

	PrintQuaternion(stream, parts.q);
}

void PrintMatrixTranslate(FILE *stream, const Matrix3 &matrix)
{
	AffineParts parts;
	my_decomp_affine(matrix, &parts);

	PrintPoint(stream, parts.t);
}

Matrix3 MatrixToRotateMatrix(const Matrix3 &matrix)
{
	AffineParts parts;
	Matrix3 result;

	my_decomp_affine(matrix, &parts);

	parts.q.MakeMatrix(result);

	return result;
}


void PrintDecomposedMatrix(FILE *stream, const Matrix3 &matrix)
{
	AffineParts parts;
	my_decomp_affine(matrix, &parts);

	fprintf(stream, "matrix\r\n");
	PrintMatrix(stream, matrix);
	PrintLineBreak(stream);

	// translate rotate matrix stuff
	Matrix3 transTM;
	Matrix3 rotTM;
	Matrix3 trTM;
	
	transTM = TransMatrix(parts.t);
	parts.q.MakeMatrix(rotTM);
	trTM = rotTM * transTM;

	AffineParts parts2;
	my_decomp_affine(trTM, &parts2);

	fprintf(stream, "translate matrix\r\n");
	PrintMatrix(stream, transTM);
	PrintLineBreak(stream);

	fprintf(stream, "rotate matrix\r\n");
	PrintMatrix(stream, rotTM);
	PrintLineBreak(stream);

	fprintf(stream, "translate-rotate matrix\r\n");
	PrintMatrix(stream, trTM);
	PrintLineBreak(stream);

	fprintf(stream, "translation\r\n");
	PrintPoint(stream, parts.t);
	PrintLineBreak(stream);

	fprintf(stream, "essential rotation\r\n");
	PrintQuaternion(stream, parts.q);
	PrintLineBreak(stream);

	fprintf(stream, "stretch rotation\r\n");
	PrintQuaternion(stream, parts.u);
	PrintLineBreak(stream);

	fprintf(stream, "stretch factors\r\n");
	PrintPoint(stream, parts.k);
	PrintLineBreak(stream);

	fprintf(stream, "sign of the determinate\r\n");
	PrintFloat(stream, parts.f);
	PrintLineBreak(stream);
}

void PrintBBox(FILE *stream, ExportBBox &bbox)
{
	assert(bbox.valid);

	fprintf(stream, "%f\r\n", bbox.xmin);
	fprintf(stream, "%f\r\n", bbox.xmax);
	fprintf(stream, "%f\r\n", bbox.ymin);
	fprintf(stream, "%f\r\n", bbox.ymax);

	fprintf(stream, "%f\r\n", bbox.zmin);
	fprintf(stream, "%f\r\n", bbox.zmax);
}

void PrintPoint(FILE *stream, const Point3 &point)
{
	fprintf(stream, "%f\t%f\t%f\r\n", point[0], point[1], point[2]);
}

void PrintAngleAxis(FILE *stream, const Point3 &axis, float angle)
{
	fprintf(stream, "%f\t%f\t%f\t%f\r\n", axis[0], axis[1], axis[2], angle);
}

void PrintSeperator(FILE *stream)
{
	fprintf(stream, "-------------------------------------------\r\n");
}

void PrintLineBreak(FILE *stream)
{
	fprintf(stream, "\r\n");
}

void PrintFloat(FILE *stream, float f)
{
	fprintf(stream, "%f\r\n");
}

void PrintAngleAxis(FILE *stream, Quat q)
{
	PrintQAsAngleAxis(stream, q);
}

void PrintQAsAngleAxis(FILE *stream, Quat q)
{
	Point3	axis;
	float	angle;
	Quat	confirm;

	AngAxisFromQ(q, &angle, axis);
	confirm = QFromAngAxis(angle, axis);

	PrintAngleAxis(stream, axis, angle);
}

void PrintQuaternion(FILE *stream, Quat q)
{
	fprintf(stream, "%f\t%f\t%f\t%f\r\n", q.x, q.y, q.z, q.w);
}

void PrintEuler(FILE *stream, float euler[3])
{
	fprintf(stream, "%f\t%f\t%f\r\n", euler[0], euler[1], euler[2]);
}

bool INodeIsBiped(INode * node)
{
	char *name					= node->GetName();
	TimeValue	time			= 0;
	Object *	object			= node->EvalWorldState(time).obj;
	SClass_ID	superClassID	= object->SuperClassID();
	Class_ID	classID			= object->ClassID();
	ULONG		classIDA		= classID.PartA();
	ULONG		classIDB		= classID.PartB();

	bool		isBiped			= false;

	if (superClassID != GEOMOBJECT_CLASS_ID) {
		return false;
	}

	switch (classIDA)
	{
		case 0x9125:
			isBiped = true;
		break;
	}

	if (strlen(name) > strlen("Bip0"))
	{
		char prefix[5];
		strncpy(prefix, name, 5);
		prefix[4] = 0;

		if (0 == strcmp(prefix, "Bip0")) {
			isBiped = true;
		}
	}

	return isBiped;
}

bool INodeIsGeomObject(INode *node)
{
	TimeValue	time			= 0;
	Object *	object			= node->EvalWorldState(time).obj;
	SClass_ID	superClassID	= object->SuperClassID();

	bool		isGeomObject	= GEOMOBJECT_CLASS_ID == superClassID;

	return isGeomObject;
}

bool HasSuffix(char *string, char *suffix)
{
	int string_length	= strlen(string);
	int suffix_length	= strlen(suffix);

	if (suffix_length > string_length) { return false; }

	string -= 1;	// now 1 is first element
	suffix -= 1;	

	while(suffix_length > 0) {
		int stringChar = tolower(string[string_length]);
		int suffixChar = tolower(suffix[suffix_length]);

		if (stringChar != suffixChar) {
			return false;
		}

		string_length -= 1;
		suffix_length -= 1;
	}

	return true;
}

/*
 *
 * This class dumps everything to a single file.
 *
 */

void PrintLines(FILE *stream, const char *buf)
{
	assert(NULL != stream);

	if (NULL == buf) {
		fprintf(stream, "0\n");
		return;
	}
	
	if (0 == strlen(buf)) {
		fprintf(stream, "0\n");
		return;
	}

	int itr;
	int count = 0;
	int length = strlen(buf);

	for(itr = 0; itr < length; itr++)
	{
		if ('\n' == buf[itr]) {
			count += 1;
		}
	}

	count += 1;	// final \0

	fprintf(stream, "%d\n", count);

	for(itr = 0; itr < length; itr++)
	{
		switch (buf[itr])
		{
			case '\0': assert(0); return;
			case '\n': fprintf(stream, "\n"); break;
			case '\r': break;
			default: fprintf(stream, "%c", buf[itr]); break;
		}
	}

	fprintf(stream, "\n");
}

void Scene::MXrMatrix(const Matrix3 &m, MUtMatrix3 &outMatrix)
{
	Point3 col0		= m.GetRow(0);
	Point3 col1		= m.GetRow(1);
	Point3 col2		= m.GetRow(2);
	Point3 col3		= m.GetRow(3);

	outMatrix.m[0][0] = col0.x;
	outMatrix.m[0][1] = col0.y;
	outMatrix.m[0][2] = col0.z;

	outMatrix.m[1][0] = col1.x;
	outMatrix.m[1][1] = col1.y;
	outMatrix.m[1][2] = col1.z;

	outMatrix.m[2][0] = col2.x;
	outMatrix.m[2][1] = col2.y;
	outMatrix.m[2][2] = col2.z;

	outMatrix.m[3][0] = col3.x;
	outMatrix.m[3][1] = col3.y;
	outMatrix.m[3][2] = col3.z;

	return;
}


unsigned long Scene::Checksum(void)
{
	unsigned long checksum = 0;
	unsigned long itr;
	unsigned long *pLong;

	for(itr = 0; itr < fNumPoints; itr++)
	{
		ExportPoint *curPoint = fPoints + itr;
		
		pLong = (unsigned long *) &(curPoint->nx);
		checksum += *pLong;

		pLong = (unsigned long *) &(curPoint->ny);
		checksum += *pLong;

		pLong = (unsigned long *) &(curPoint->nz);
		checksum += *pLong;

		pLong = (unsigned long *) &(curPoint->u);
		checksum += *pLong;

		pLong = (unsigned long *) &(curPoint->v);
		checksum += *pLong;

		pLong = (unsigned long *) &(curPoint->x);
		checksum += *pLong;

		pLong = (unsigned long *) &(curPoint->y);
		checksum += *pLong;

		pLong = (unsigned long *) &(curPoint->z);
		checksum += *pLong;
	}

	checksum += fNumFaces;
	checksum += fNumPoints;

	return checksum;
}