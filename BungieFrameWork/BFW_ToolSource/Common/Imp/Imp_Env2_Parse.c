/*
	FILE:	Imp_Env_Parse.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 3, 1997

	PURPOSE: 
	
	Copyright 1997

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_AppUtilities.h"
#include "BFW_Group.h"
#include "BFW_TM_Construction.h"
#include "BFW_Mathlib.h"
#include "BFW_Object.h"
#include "BFW_BitVector.h"
#include "BFW_FileFormat.h"
#include "BFW_Particle2.h"
#include "BFW_Timer.h"
#include "BFW_Math.h"

#include "Imp_Common.h"
#include "Imp_Texture.h"
#include "Imp_Env2_Private.h"
#include "Imp_Model.h"
#include "Imp_AI_HHT.h"
#include "Imp_ParseEnvFile.h"
#include "EnvFileFormat.h"
#include "Imp_AI_Script.h"
#include "Imp_Sound.h"
#include "Imp_EnvParticle.h"

#include "Oni_AI_Script.h"
#include "Oni_AI_Setup.h"
#include <ctype.h>

#define PRINT_ENV_TEXTURE_LIST		0

UUtUns32 			IMPgEnv_PointIndexRemap[IMPcEnv_MaxRemapPoints];
M3tTextureCoord		IMPgEnv_TextureCoords[IMPcEnv_MaxRemapPoints];
M3tPoint3D			IMPgEnv_RawPoints[IMPcEnv_MaxRemapPoints];

const char*			IMPg_Gunk_FileName;
const char*			IMPg_Gunk_ObjName;
const char*			IMPg_Gunk_MaterialName;
char*				IMPg_Gunk_TextureName;

UUtUns32			IMPg_Gunk_Flags;
UUtUns16			IMPg_Gunk_VisFlags;
UUtUns16			IMPg_Gunk_LMFlags;
UUtUns16			IMPg_Gunk_ScriptID;
UUtUns32			IMPg_Gunk_ObjectTag;

UUtUns16			IMPg_Gunk_FileRelativeGQIndex;

UUtBool				IMPgEnv_LS = UUcFalse;
float				IMPgEnv_LS_FilterColor[3];
IMPtLS_LightType	IMPgEnv_LS_Type;
UUtUns32			IMPgEnv_LS_Intensity;
UUtUns32			IMPgEnv_LS_ParamFlags;
IMPtLS_Distribution	IMPgEnv_LS_Distribution;
float				IMPgEnv_LS_BeamAngle;
float				IMPgEnv_LS_FieldAngle;

extern GRtGroup*	gPreferencesGroup;

AUtFlagElement	IMPgLSParamFlags[] = 
{
	{
		"not_occluding",
		IMPcLS_LightParam_NotOccluding
	},
	{
		"window",
		IMPcLS_LightParam_Window
	},
	{
		"not_reflecting",
		IMPcLS_LightParam_NotReflecting
	},
	{
		NULL,
		0
	}
};

AUtFlagElement	IMPgGunkFlags[] = 
{
	{
		"door",
		AKcGQ_Flag_Door
	},
	{
		"ghost",
		AKcGQ_Flag_Ghost
	},
	{
		"stairsup",
		AKcGQ_Flag_SAT_Up
	},
	{
		"stairsdown",
		AKcGQ_Flag_SAT_Down
	},
	{
		"stairs",
		AKcGQ_Flag_Stairs
	},
	{
		"2sided",
		AKcGQ_Flag_DrawBothSides
	},
	{
		"nocollision",
		AKcGQ_Flag_No_Collision
	},
	{
		"invisible",
		AKcGQ_Flag_Invisible,
	},
	{
		"no_chr_col",
		AKcGQ_Flag_No_Character_Collision,
	},
	{
		"no_obj_col",
		AKcGQ_Flag_No_Object_Collision,
	},
	{
		"no_occl",
		AKcGQ_Flag_No_Occlusion,
	},
	{
		"danger",
		AKcGQ_Flag_AIDanger,
	},
	{
		"grid_ignore",
		AKcGQ_Flag_AIGridIgnore,
	},
	{
		"nodecal",
		AKcGQ_Flag_NoDecal,
	},
	{
		"furniture",
		AKcGQ_Flag_Furniture,
	},
	{
		"sound_transparent",
		AKcGQ_Flag_SoundTransparent,
	},
	{
		"impassable",
		AKcGQ_Flag_AIImpassable
	},
	{
		NULL,
		0
	}
};

AUtFlagElement	IMPgLMFlags[] = 
{
	{
		"forceon",
		IMPcGQ_LM_ForceOn
	},
	{
		"forceoff",
		IMPcGQ_LM_ForceOff
	},
	{
		NULL,
		0
	}
};

AUtFlagElement	IMPgTriggerFlags[] =
{
	{
		"toggle",
		AIcTriggerFlag_Toggle
	},
	{
		"once",
		AIcTriggerFlag_Once
	},
	{
		NULL,
		0
	}
};

AUtFlagElement IMPgGunkObjectFlags[] =
{
	{
		"ignorecharacters",
		OBcFlags_IgnoreCharacters
	},
	{
		"light",
		OBcFlags_LightSource
	},
	{
		"nocollision",
		OBcFlags_NoCollision
	},
	{
		"facecollision",
		OBcFlags_FaceCollision
	},
	{
		"nogravity",
		OBcFlags_NoGravity
	},
	{
		NULL,
		0
	}
};

AUtFlagElement	IMPgBNVFlags[] = 
{
	{
		"stairs",
		AKcBNV_Flag_Stairs_Standard
	},
	{
		"spiralstairs",
		AKcBNV_Flag_Stairs_Spiral
	},
	{
		"nonai",
		AKcBNV_Flag_NonAI
	},
	{
		"simple",
		AKcBNV_Flag_SimplePathfinding
	},
	{
		NULL,
		0
	}
};

static UUtUns32 IMPiEnv_Parse_GQ_ObjectTag( GRtGroup* inGroup, IMPtEnv_BuildData* inBuildData )
{
	UUtError		error;
	UUtUns32		object_tag;

	error  = GRrGroup_GetUns32(inGroup,"object_tag", &object_tag);	
	if (error == UUcError_None) 
	{
		return object_tag;
	}
	return 0xFFFFFFFF;
}

void IMPrEnv_Add_ObjectTag( IMPtEnv_BuildData* inBuildData, UUtUns32 inTag, UUtUns32 inQuadIndex )
{
	UUtError		error;
	UUtUns32*		array;
	UUtUns32		new_index;
	
	UUmAssert( inBuildData->object_tags && inBuildData->object_quads );

	error = UUrMemory_Array_GetNewElement( inBuildData->object_tags, &new_index, NULL );
	UUmAssert( error == UUcError_None );
	array = (UUtUns32*) UUrMemory_Array_GetMemory( inBuildData->object_tags );
	array[new_index] = inTag;

	error = UUrMemory_Array_GetNewElement( inBuildData->object_quads, &new_index, NULL );
	UUmAssert( error == UUcError_None );
	array = (UUtUns32*) UUrMemory_Array_GetMemory( inBuildData->object_quads );
	array[new_index] = inQuadIndex;
}

static UUtError
IMPiEnv_Parse_Point(
	IMPtEnv_BuildData*	inBuildData,
	UUtUns16			index,
	const MXtPoint*			inPoint)
{
	UUtError			error;
	
	if(index >= IMPcEnv_MaxRemapPoints)
	{
		Imp_PrintWarning("exceeded max remap indices");
		return UUcError_Generic;
	}
	
	IMPgEnv_RawPoints[index] = inPoint->point;
	AUrSharedPointArray_RoundPoint(IMPgEnv_RawPoints + index);
	
	error =
		AUrSharedPointArray_AddPoint(
			inBuildData->sharedPointArray,
			inPoint->point.x, inPoint->point.y, inPoint->point.z,
			&IMPgEnv_PointIndexRemap[index]);
	UUmError_ReturnOnErrorMsg(error, "Could not add point");
	
	IMPgEnv_TextureCoords[index] = inPoint->uv;
	
	return UUcError_None;
}

static UUtError
IMPiEnv_Parse_Point_ForFlag(
	M3tPoint3D*	inFlagPoints,
	UUtUns16	index,
	MXtPoint*	inPoint)
{
	if(index >= IMPcEnv_MaxRemapPoints)
	{
		Imp_PrintWarning("exceeded max remap indices, tell Quinn");
		return UUcError_Generic;
	}

	inFlagPoints[index] = inPoint->point;
	
	return UUcError_None;
}


static UUtError
IMPiEnv_Parse_BNV_AddQuad(
	IMPtEnv_BuildData*	inBuildData,
	UUtUns32			ri0,
	UUtUns32			ri1,
	UUtUns32			ri2,
	UUtUns32			ri3,
	M3tVector3D			*dont_use_this_normal)
{
	UUtError			error;
	UUtUns32			quadIndex;
	IMPtEnv_BNV*		curBNV;
	
	UUtUns16			curSideIndex;
	IMPtEnv_BNV_Side*	curSide;
	
	M3tPoint3D*			shared_point_list = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	M3tPoint3D*			curPoint;
	float				d;
	M3tPlaneEquation*	planeArray;
	float				sideA, sideB, sideC, sideD;
	float				nx, ny, nz;

	{
		M3tVector3D normal;

		MUrVector_NormalFromPoints(shared_point_list + ri0, shared_point_list + ri1, shared_point_list + ri2, &normal);

		if (MUrVector_DotProduct(dont_use_this_normal, &normal) < 0.f) {
			MUmVector_Negate(normal);
		}

		nx = normal.x;
		ny = normal.y;
		nz = normal.z;
	}
	
	planeArray = AUrSharedPlaneEquArray_GetList(inBuildData->sharedPlaneEquArray);

	// Add the quad
		error =
			AUrSharedQuadArray_AddQuad(
				inBuildData->sharedBNVQuadArray,
				ri0, ri1, ri2, ri3,
				&quadIndex);
		UUmError_ReturnOnErrorMsg(error, "Could not add quad");
	
	// Compute the temporary plane equation
		curPoint = AUrSharedPointArray_GetList(inBuildData->sharedPointArray) + ri0;
		
		d = -(curPoint->x * nx + curPoint->y * ny + curPoint->z * nz);
	
	
	curBNV = inBuildData->bnvList + inBuildData->numBNVs;
	
	// check for other quads on this plane
	for(curSideIndex = 0, curSide = curBNV->sides;
		curSideIndex < curBNV->numSides;
		curSideIndex++, curSide++)
	{
		AKmPlaneEqu_GetComponents(
			curSide->planeEquIndex,
			planeArray,
			sideA, sideB, sideC, sideD);
		
		if(UUmFloat_CompareEqu(sideA, nx) &&
			UUmFloat_CompareEqu(sideB, ny) &&
			UUmFloat_CompareEqu(sideC, nz) &&
			UUmFloat_CompareEqu(sideD, d))
		{
			// Already have added this side so just add the quad
			if(curSide->numBNVQuads > IMPcEnv_MaxQuadsPerSide)
			{
				UUmError_ReturnOnErrorMsg(UUcError_Generic, "Too many side BNV quads");
			}
			
			curSide->bnvQuadList[curSide->numBNVQuads++] = quadIndex;
			
			return UUcError_None;
		}
	}
	
	if(curBNV->numSides >= IMPcEnv_MaxBNVSides)
	{
		UUmError_ReturnOnErrorMsgP(
			UUcError_Generic, 
			"Exceeeded max BNV sides on file \"%s\", thank Brent.",
			(UUtUns32)curBNV->fileName, 0, 0);
	}
	
	curSide = curBNV->sides + curBNV->numSides++;
	
	curSide->numGQGhostIndices = 0;
	
	curSide->numBNVQuads = 1;
	curSide->bnvQuadList[0] = quadIndex;
	
	error =
		AUrSharedPlaneEquArray_AddPlaneEqu(
			inBuildData->sharedPlaneEquArray,
			nx, ny, nz, d,
			&curSide->planeEquIndex);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}
	
static UUtError
IMPiEnv_Parse_BNV_Quad(
	IMPtEnv_BuildData*	inBuildData,
	MXtFace*			inFace)
{
	UUtError			error;
	UUtUns32			ri0, ri1, ri2, ri3;
	
	ri0 = IMPgEnv_PointIndexRemap[inFace->indices[0]];
	ri1 = IMPgEnv_PointIndexRemap[inFace->indices[1]];
	ri2 = IMPgEnv_PointIndexRemap[inFace->indices[2]];
	ri3 = IMPgEnv_PointIndexRemap[inFace->indices[3]];
	
	error =
		IMPiEnv_Parse_BNV_AddQuad(
			inBuildData,
			ri0, ri1, ri2, ri3,
			&inFace->dont_use_this_normal);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

static UUtError
IMPiEnv_Parse_BNV_Tri(
	IMPtEnv_BuildData*	inBuildData,
	MXtFace*			inFace)
{
	UUtError			error;
	UUtUns32			ri0, ri1, ri2, ri3;
	
	float				midX, midY, midZ;
	M3tPoint3D*			sharedPoints;
	
	sharedPoints = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	
	ri0 = IMPgEnv_PointIndexRemap[inFace->indices[0]];
	ri1 = IMPgEnv_PointIndexRemap[inFace->indices[1]];
	ri2 = IMPgEnv_PointIndexRemap[inFace->indices[2]];
	
	midX = (sharedPoints[ri0].x + sharedPoints[ri2].x) * 0.5f;
	midY = (sharedPoints[ri0].y + sharedPoints[ri2].y) * 0.5f;
	midZ = (sharedPoints[ri0].z + sharedPoints[ri2].z) * 0.5f;
	
	error =
		AUrSharedPointArray_AddPoint(
			inBuildData->sharedPointArray,
			midX, midY, midZ,
			&ri3);
	UUmError_ReturnOnErrorMsg(error, "Could not add point");
	
	error =
		IMPiEnv_Parse_BNV_AddQuad(
			inBuildData,
			ri0, ri1, ri2, ri3,
			&inFace->dont_use_this_normal);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

static UUtError
IMPiEnv_Parse_BNV(
	const char*			inFileName,
	MXtHeader*			inEnvData,
	IMPtEnv_BuildData*	inBuildData)
{
	UUtError			error;
	IMPtEnv_BNV*		curBNV;
	GRtGroup_Context*	groupContext;
	GRtGroup*			group;
	GRtElementType	 	groupType;
	GRtElementArray*	groupFlags;
	UUtUns32			bnvFlags;
	MXtNode*			curNode;
	MXtPoint*			curPoint;
	MXtFace*			curFace;
	
	UUtUns16			nodeItr;
	UUtUns16			faceItr;
	UUtUns16			pointItr;

	// do some setup stuff
	for(nodeItr = 0, curNode = inEnvData->nodes; nodeItr < inEnvData->numNodes;	nodeItr++, curNode++)
	{
		UUmAssert(inBuildData->numBNVs < inBuildData->maxBNVs);
		
		if (UUrString_HasPrefix(curNode->name, IMPcGunkFlagNamePrefix) ||
			UUrString_HasPrefix(curNode->name, IMPcParticle2Prefix) ||
			UUrString_HasPrefix(curNode->name, IMPcFXPrefix))
		{
			Imp_PrintWarning("Can not have a flag or FX in a BNV FILE: \"%s\" OBJ: \"%s\"", inFileName, curNode->name);

			return UUcError_Generic;
		}

		curBNV = inBuildData->bnvList + inBuildData->numBNVs;
		
		bnvFlags = 0;
		
		// Read user data for BNVs
		if(curNode->userDataCount > 0 && *curNode->userData != 0) {
			// We have user data, so parse it
			error = GRrGroup_Context_NewFromString(curNode->userData,gPreferencesGroup,NULL,&groupContext,&group);
			UUmError_ReturnOnError(error);
			
			// Read off the BNV property flags
			error = GRrGroup_GetElement(group, "type", &groupType, &groupFlags);

			if(error == UUcError_None) {
				error = 
					AUrFlags_ParseFromGroupArray(
						IMPgBNVFlags,
						groupType,
						groupFlags,
						&bnvFlags);
			}

			GRrGroup_Context_Delete(groupContext);
		}
		
		curBNV->flags = bnvFlags;
				
		// read the world 
		
		curBNV->parent = 0xFFFFFFFF;
		curBNV->level = 0xFFFFFFFF;
		curBNV->child = 0xFFFFFFFF;
		curBNV->next = 0xFFFFFFFF;
		curBNV->numSides = 0;
		curBNV->numNodes = 0;
		curBNV->isLeaf = 0;
		curBNV->error = UUcFalse;
		curBNV->numGQs = 0;
		
		UUrString_Copy(curBNV->fileName, inFileName, IMPcEnv_MaxNameLength);
		UUrString_Copy(curBNV->objName, curNode->name, IMPcEnv_MaxNameLength);

		for(pointItr = 0, curPoint = curNode->points; pointItr < curNode->numPoints; pointItr++, curPoint++)
		{
			MUrMatrix_MultiplyPoint(&curPoint->point, &curNode->matrix,	&curPoint->point);
			
			error = IMPiEnv_Parse_Point(inBuildData, pointItr, curPoint);
			UUmError_ReturnOnError(error);

			if (0 == pointItr) {
				curBNV->minX = curBNV->maxX = curPoint->point.x;
				curBNV->minY = curBNV->maxY = curPoint->point.y;
				curBNV->minZ = curBNV->maxZ = curPoint->point.z;
			}
			else {
				UUmMinMax(curPoint->point.x, curBNV->minX, curBNV->maxX);
				UUmMinMax(curPoint->point.y, curBNV->minY, curBNV->maxY);
				UUmMinMax(curPoint->point.z, curBNV->minZ, curBNV->maxZ);
			}
		}

		for(faceItr = 0, curFace = curNode->triangles; faceItr < curNode->numTriangles;	faceItr++, curFace++)
		{
			MUrMatrix_MultiplyNormal(&curFace->dont_use_this_normal, &curNode->matrix, &curFace->dont_use_this_normal);

			error = IMPiEnv_Parse_BNV_Tri(inBuildData, curFace);
			UUmError_ReturnOnError(error);
		}
		
		for(faceItr = 0, curFace = curNode->quads; faceItr < curNode->numQuads; faceItr++, curFace++)
		{
			MUrMatrix_MultiplyNormal(&curFace->dont_use_this_normal, &curNode->matrix, &curFace->dont_use_this_normal);

			error = IMPiEnv_Parse_BNV_Quad(inBuildData, curFace);
			UUmError_ReturnOnError(error);
		}

		inBuildData->numBNVs++;
	}
		
	return UUcError_None;
}


static UUtUns16 IMPiEnv_Parse_GQ_AddTexture( IMPtEnv_BuildData* inBuildData, const char* inTextureName)
{
	UUtError				error;
	
	UUtUns16				newIndex;
	char					fileName[BFcMaxFileNameLength];
		
	UUmAssert(*inTextureName != 0);
	
	if(1 || !(IMPg_Gunk_Flags & AKcGQ_Flag_NoTextureMask))
	{
		UUtUns32 temp;

		UUrString_Copy(fileName, inTextureName, BFcMaxFileNameLength);
		UUrString_Capitalize(fileName, BFcMaxFileNameLength);
		UUrString_StripExtension(fileName);
		
		error = 
			AUrSharedStringArray_AddString(
				inBuildData->textureMapArray,
				fileName,
				0,
				&temp);

		newIndex = (UUtUns16) temp;

		if(error != UUcError_None)
		{
			UUrError_Report(UUcError_OutOfMemory, "could not add texture");
			return 0xFFFF;
		}
	}
	else
	{
		newIndex = 0xFFFF;
	}
	
	return newIndex;
}

static UUtError
IMPiEnv_Parse_GQ_AddQuadProperSize(
	IMPtEnv_BuildData*	inBuildData,
	UUtUns32			ri0,
	UUtUns32			ri1,
	UUtUns32			ri2,
	UUtUns32			ri3,
	M3tTextureCoord*	inBaseTextureCoords,
	float				nx,
	float				ny,
	float				nz,
	float				realD,
	UUtUns16			localFlags)		// Can be NULL
{
	UUtError		error;
	IMPtEnv_GQ*		curGQ;
	UUtUns16		curTextureIndex;
	M3tPoint3D*		pointArray;
	M3tPoint3D*		curPoint;
	//float			d;
	UUtUns32		swap;
	UUtUns16		i;
	UUtBool			planeProblem, tri_too_small;
	float			curDist, maxDist, testD;
	M3tVector3D		edge0, edge1, edge2;
	M3tVector3D		normal0, normal1;
	float			dot;
	
	pointArray = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	
	// Check for degenerate quads
		if(ri0 == ri1 || ri0 == ri2 || ri0 == ri3 ||
			ri1 == ri2 || ri1 == ri3)
		{
			if (localFlags & AKcGQ_Flag_Triangle) {
				IMPrEnv_LogError("degenerate tri, %s, %s, %s [vertices %d %d %d]",
								IMPg_Gunk_FileName, IMPg_Gunk_ObjName, IMPg_Gunk_TextureName, ri0, ri1, ri2);
			} else {
				IMPrEnv_LogError("degenerate quad, %s, %s, %s [vertices %d %d %d %d]",
								IMPg_Gunk_FileName, IMPg_Gunk_ObjName, IMPg_Gunk_TextureName, ri0, ri1, ri2, ri3);
			}
			return UUcError_None;
		}
			
		if(ri2 == ri3 && !(localFlags & AKcGQ_Flag_Triangle))
		{
			// this GQ should be triangular
			IMPrEnv_LogError("degenerate quad, %s, %s, %s [vertices %d %d %d %d] -> converting to triangle",
							IMPg_Gunk_FileName, IMPg_Gunk_ObjName, IMPg_Gunk_TextureName, ri0, ri1, ri2, ri3);
			localFlags |= AKcGQ_Flag_Triangle;
		}

		/*
		 * check quads for collinear points
		 */

		if ((localFlags & AKcGQ_Flag_Triangle) == 0)
		{
			tri_too_small = (MUrTriangle_Area(pointArray + ri0, pointArray + ri1, pointArray + ri2) < 0.01f);
			if (tri_too_small)
			{
				// this is a quad and it has a zero-area triangle on one side
				tri_too_small = (MUrTriangle_Area(pointArray + ri0, pointArray + ri1, pointArray + ri3) < 0.01f);
				if (tri_too_small)
				{
					// this is a zero-size quad
					IMPrEnv_LogError("quad with zero area, %s, %s, %s", IMPg_Gunk_FileName, IMPg_Gunk_ObjName, IMPg_Gunk_TextureName);
					return UUcError_None;
				}

				// otherwise, just reject the 0, 1, 2 triangle that is one side of the quad
				ri1 = ri2;
				ri2 = ri3;
				localFlags |= AKcGQ_Flag_Triangle;
				IMPrEnv_LogError("quad with zero side, converting to tri, %s, %s, %s [v %d %d %d]",
								IMPg_Gunk_FileName, IMPg_Gunk_ObjName, IMPg_Gunk_TextureName, ri0, ri1, ri2);
			}
			else
			{
				// check the quad for collinear points along the other diagonal (1 2 3)
				tri_too_small = (MUrTriangle_Area(pointArray + ri1, pointArray + ri2, pointArray + ri3) < 0.01f);
				if (tri_too_small)
				{
					// reject the 0, 1, 2 triangle that is one side of the quad
					ri1 = ri2;
					ri2 = ri3;
					localFlags |= AKcGQ_Flag_Triangle;
					IMPrEnv_LogError("quad with zero side, converting to tri, %s, %s, %s [v %d %d %d]",
									IMPg_Gunk_FileName, IMPg_Gunk_ObjName, IMPg_Gunk_TextureName, ri0, ri1, ri2);
				}
			}
		}

		/*
		 * check triangles for zero area
		 */

		if (localFlags & AKcGQ_Flag_Triangle)
		{
			tri_too_small = (MUrTriangle_Area(pointArray + ri0, pointArray + ri1, pointArray + ri2) < 0.01f);
			if (tri_too_small)
			{
				if (localFlags & AKcGQ_Flag_Triangle)
				{
					IMPrEnv_LogError("triangle with zero area, %s, %s, %s", IMPg_Gunk_FileName, IMPg_Gunk_ObjName, IMPg_Gunk_TextureName);
					return UUcError_None;
				}
			}
		}

		/*
		 * check for pretzel quads
		 */

		if(!(localFlags & AKcGQ_Flag_Triangle))
		{
			MUmVector_Subtract(edge0, pointArray[ri1], pointArray[ri0]);
			MUmVector_Subtract(edge1, pointArray[ri2], pointArray[ri0]);
			MUmVector_Subtract(edge2, pointArray[ri3], pointArray[ri0]);

			MUrVector_CrossProduct(&edge0, &edge1, &normal0);
			MUrVector_CrossProduct(&edge1, &edge2, &normal1);			

			dot = MUrVector_DotProduct(&normal0, &normal1);
			if(dot < 0)
			{
				IMPrEnv_LogError("pretzel quad, %s, %s, %s [norm012 %f %f %f, norm023 %f %f %f orig_n %f %f %f] dot %f...",
								IMPg_Gunk_FileName, IMPg_Gunk_ObjName, IMPg_Gunk_TextureName, 
								normal0.x, normal0.y, normal0.z, normal1.x, normal1.y, normal1.z,
								nx, ny, nz, dot);

				if (normal0.x * nx + normal0.y * ny + normal0.z * nz > 0.01f) {
					// the 012 triangle has the correct normal, leave 01 unchanged
					swap = ri2;
					ri2 = ri3;
					ri3 = swap;
					IMPrEnv_LogError("... norm0 dot input norm is positive, flip 2/3");

				} else if (normal1.x * nx + normal1.y * ny + normal1.z * nz > 0.01f) {
					// the 023 triangle has the correct normal, leave 03 unchanged
					swap = ri2;
					ri2 = ri1;
					ri1 = swap;
					IMPrEnv_LogError("... norm0 dot input norm is positive, flip 1/2");

				} else {
					IMPrEnv_LogError("... both normals wrong, discard quad");
					return UUcError_None;
				}
			}
		}

		// FIXME: check for concave quads?
			
		
	if(inBuildData->numGQs >= inBuildData->maxGQs)
	{
		UUmError_ReturnOnErrorMsgP(
			UUcError_Generic, 
			"Exceeeded max num GQs in file \"%s\".",
			(UUtUns32)IMPg_Gunk_FileName, 0, 0);
	}
	
	if (!(inBuildData->numGQs % 500)) Imp_PrintMessage(IMPcMsg_Important, ".");


	curGQ = inBuildData->gqList + inBuildData->numGQs;

	curGQ->object_tag = IMPg_Gunk_ObjectTag;
	
	curGQ->ghostUsed = UUcFalse;
	curGQ->stairBNVIndex = (UUtUns32) -1;

	curGQ->visibleQuad.indices[0] = ri0;
	curGQ->visibleQuad.indices[1] = ri1;
	curGQ->visibleQuad.indices[2] = ri2;
	curGQ->visibleQuad.indices[3] = ri3;
	
	UUrMemory_Clear(curGQ->gqMaterialName, IMPcEnv_MaxNameLength);
	
	curTextureIndex = IMPiEnv_Parse_GQ_AddTexture(inBuildData, IMPg_Gunk_TextureName);
	
	if(!(IMPg_Gunk_Flags & AKcGQ_Flag_NoTextureMask))
	{
		error = 
			AUrSharedTexCoordArray_AddTexCoord(
				inBuildData->sharedTextureCoordArray,
				inBaseTextureCoords[0].u,
				inBaseTextureCoords[0].v,
				&curGQ->baseMapIndices.indices[0]);
		UUmError_ReturnOnError(error);
		
		error = 
			AUrSharedTexCoordArray_AddTexCoord(
				inBuildData->sharedTextureCoordArray,
				inBaseTextureCoords[1].u,
				inBaseTextureCoords[1].v,
				&curGQ->baseMapIndices.indices[1]);
		UUmError_ReturnOnError(error);
		
		error = 
			AUrSharedTexCoordArray_AddTexCoord(
				inBuildData->sharedTextureCoordArray,
				inBaseTextureCoords[2].u,
				inBaseTextureCoords[2].v,
				&curGQ->baseMapIndices.indices[2]);
		UUmError_ReturnOnError(error);
		
		error = 
			AUrSharedTexCoordArray_AddTexCoord(
				inBuildData->sharedTextureCoordArray,
				inBaseTextureCoords[3].u,
				inBaseTextureCoords[3].v,
				&curGQ->baseMapIndices.indices[3]);
		UUmError_ReturnOnError(error);
	}
	else
	{
		error = 
			AUrSharedTexCoordArray_AddTexCoord(
				inBuildData->sharedTextureCoordArray,
				0.0f,
				0.0f,
				&curGQ->baseMapIndices.indices[0]);
		UUmError_ReturnOnError(error);
		
		error = 
			AUrSharedTexCoordArray_AddTexCoord(
				inBuildData->sharedTextureCoordArray,
				1.0f,
				0.0f,
				&curGQ->baseMapIndices.indices[1]);
		UUmError_ReturnOnError(error);
		
		error = 
			AUrSharedTexCoordArray_AddTexCoord(
				inBuildData->sharedTextureCoordArray,
				1.0f,
				1.0f,
				&curGQ->baseMapIndices.indices[2]);
		UUmError_ReturnOnError(error);
		
		error = 
			AUrSharedTexCoordArray_AddTexCoord(
				inBuildData->sharedTextureCoordArray,
				0.0f,
				1.0f,
				&curGQ->baseMapIndices.indices[3]);
		UUmError_ReturnOnError(error);
	}
			
	curGQ->textureMapIndex = curTextureIndex;
	
/*	curGQ->adjGQIndices[0] = 0xFFFFFFFF;
	curGQ->adjGQIndices[1] = 0xFFFFFFFF;
	curGQ->adjGQIndices[2] = 0xFFFFFFFF;
	curGQ->adjGQIndices[3] = 0xFFFFFFFF;*/
	
	curPoint = pointArray + curGQ->visibleQuad.indices[0];
	//d = -(curPoint->x * nx + curPoint->y * ny + curPoint->z * nz);
	error = 
		AUrSharedPlaneEquArray_AddPlaneEqu(
			inBuildData->sharedPlaneEquArray,
			nx, ny, nz, realD,
			&curGQ->planeEquIndex);
	UUmError_ReturnOnError(error);
	
	curGQ->flags		= IMPg_Gunk_Flags | localFlags;
	curGQ->lmFlags		= IMPg_Gunk_LMFlags;
	curGQ->scriptID		= IMPg_Gunk_ScriptID;

	curGQ->projection = CLrQuad_FindProjection(
			pointArray,
			&curGQ->visibleQuad);

	if (CLcProjection_Unknown == curGQ->projection) {
		IMPrEnv_LogError("tiny quad, %s, %s, %s: (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)",
						IMPg_Gunk_FileName, IMPg_Gunk_ObjName, IMPg_Gunk_TextureName,
						pointArray[curGQ->visibleQuad.indices[0]].x,
						pointArray[curGQ->visibleQuad.indices[0]].y,
						pointArray[curGQ->visibleQuad.indices[0]].z,
						pointArray[curGQ->visibleQuad.indices[1]].x,
						pointArray[curGQ->visibleQuad.indices[1]].y,
						pointArray[curGQ->visibleQuad.indices[1]].z,
						pointArray[curGQ->visibleQuad.indices[2]].x,
						pointArray[curGQ->visibleQuad.indices[2]].y,
						pointArray[curGQ->visibleQuad.indices[2]].z,
						pointArray[curGQ->visibleQuad.indices[3]].x,
						pointArray[curGQ->visibleQuad.indices[3]].y,
						pointArray[curGQ->visibleQuad.indices[3]].z);
		return UUcError_None;
	}
	
	UUrString_Copy(curGQ->fileName, IMPg_Gunk_FileName, IMPcEnv_MaxNameLength);
	UUrString_Copy(curGQ->objName, IMPg_Gunk_ObjName, IMPcEnv_MaxNameLength);
	UUrString_Copy(curGQ->materialName, IMPg_Gunk_MaterialName, IMPcEnv_MaxNameLength);
	
	// Add LS info
	curGQ->fileRelativeGQIndex	= IMPg_Gunk_FileRelativeGQIndex++;
	
	curGQ->isLuminaire = IMPgEnv_LS;
	if(curGQ->isLuminaire)
	{
		curGQ->filterColor[0] = IMPgEnv_LS_FilterColor[0];
		curGQ->filterColor[1] = IMPgEnv_LS_FilterColor[1];
		curGQ->filterColor[2] = IMPgEnv_LS_FilterColor[2];
		
		curGQ->lightType = IMPgEnv_LS_Type;
		
		curGQ->intensity = IMPgEnv_LS_Intensity;
		
		curGQ->distribution = IMPgEnv_LS_Distribution;
		curGQ->beamAngle = IMPgEnv_LS_BeamAngle;
		curGQ->fieldAngle = IMPgEnv_LS_FieldAngle;
	}
	curGQ->lightParams = IMPgEnv_LS_ParamFlags;
	
	// testing
	if(0 && inBuildData->numGQs == 0)
	{
		curGQ->isLuminaire = UUcTrue;
		curGQ->filterColor[0] = 0.2f;
		curGQ->filterColor[1] = 0.3f;
		curGQ->filterColor[2] = 0.4f;
		curGQ->lightType = IMPcLS_LightType_Area;
		curGQ->intensity = 3800000;
	}
		
	// check for nonplanar problems
	planeProblem = UUcFalse;
	maxDist = -1e13f;
	for(i = 1; i < 4; i++)
	{
		curPoint = pointArray + curGQ->visibleQuad.indices[i];
		
		testD = -(curPoint->x * nx + curPoint->y * ny + curPoint->z * nz);
		
		curDist = (float)fabs(realD - testD);
		if(curDist > 0.01f)
		{
			planeProblem = UUcTrue;
			if(curDist > maxDist) maxDist = curDist;
		}
	}
	
	if(planeProblem)
	{
		IMPrEnv_LogError(
			"GQ[%d], File: %s, Obj: %s: This GQ has planar problems, non-planar by %f",
			inBuildData->numGQs,
			curGQ->fileName,
			curGQ->objName,
			maxDist);
		for(i = 0; i < 4; i++)
		{
			curPoint = pointArray + curGQ->visibleQuad.indices[i];
			IMPrEnv_LogError(
				"\t%d: (%f, %f, %f)", i, curPoint->x, curPoint->y, curPoint->z);
		}
	}

	inBuildData->numGQs++;

	return UUcError_None;
}

static UUtError
IMPiEnv_Parse_GQ_AddQuad(
	IMPtEnv_BuildData*	inBuildData,
	UUtUns32			ri0,
	UUtUns32			ri1,
	UUtUns32			ri2,
	UUtUns32			ri3,
	M3tTextureCoord*	inBaseTextureCoords,
	float				nx,
	float				ny,
	float				nz,
	float				realD,
	UUtUns16			localFlags)
{
	UUtError		error;

	error = 
		IMPiEnv_Parse_GQ_AddQuadProperSize(
			inBuildData,
			ri0,
			ri1,
			ri2,
			ri3,
			inBaseTextureCoords,
			nx,
			ny,
			nz,
			realD,
			localFlags);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

static void IMPiEnv_Parse_GQ_Tri_BuildNormal(
	IMPtEnv_BuildData*	inBuildData,
	UUtUns32			ri0,
	UUtUns32			ri1,
	UUtUns32			ri2,
	M3tTextureCoord*	inBaseTextureCoords,
	M3tVector3D			*inDontUseThisNormal)
{
	M3tVector3D normal;
	float d;
	M3tPoint3D *point_list = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	M3tTextureCoord texture_coords[4];

	MUrVector_NormalFromPoints(point_list + ri0, point_list + ri1, point_list + ri2, &normal);

	d = -(point_list[ri0].x * normal.x + point_list[ri0].y * normal.y + point_list[ri0].z * normal.z);

	if (MUrVector_DotProduct(inDontUseThisNormal, &normal) < 0.f) {
		MUmVector_Negate(normal);
		d = -d;

		texture_coords[2] = inBaseTextureCoords[0];
		texture_coords[1] = inBaseTextureCoords[1];
		texture_coords[0] = inBaseTextureCoords[2];
		texture_coords[3] = inBaseTextureCoords[0];

		IMPiEnv_Parse_GQ_AddQuad(inBuildData, ri2, ri1, ri0, ri0, texture_coords, normal.x, normal.y, normal.z, d,	AKcGQ_Flag_Triangle);
	}
	else {
		IMPiEnv_Parse_GQ_AddQuad(inBuildData, ri0, ri1, ri2, ri2, inBaseTextureCoords, normal.x, normal.y, normal.z, d,	AKcGQ_Flag_Triangle);
	}

	return;
}

static void IMPiEnv_Parse_GQ_Quad_BuildNormal(
	IMPtEnv_BuildData*	inBuildData,
	UUtUns32			ri0,
	UUtUns32			ri1,
	UUtUns32			ri2,
	UUtUns32			ri3,
	M3tTextureCoord*	inBaseTextureCoords,
	M3tVector3D			*inDontUseThisNormal)
{
	M3tVector3D normal;
	float d;
	M3tPoint3D *point_list = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	M3tTextureCoord texture_coords[4];

	MUrVector_NormalFromPoints(point_list + ri0, point_list + ri1, point_list + ri2, &normal);

	// step 1: test is this a 'pretzel quad'?
	{
		M3tVector3D pretzel_normal;

		MUrVector_NormalFromPoints(point_list + ri0, point_list + ri1, point_list + ri3, &pretzel_normal);

		if (MUrVector_DotProduct(&normal, &pretzel_normal) < 0.8f) {

			texture_coords[0] = inBaseTextureCoords[0];
			texture_coords[1] = inBaseTextureCoords[1];
			texture_coords[2] = inBaseTextureCoords[2];
			texture_coords[2] = inBaseTextureCoords[2];

			IMPiEnv_Parse_GQ_Tri_BuildNormal(inBuildData, ri0, ri1, ri2, texture_coords, inDontUseThisNormal);

			texture_coords[0] = inBaseTextureCoords[1];
			texture_coords[1] = inBaseTextureCoords[2];
			texture_coords[2] = inBaseTextureCoords[3];
			texture_coords[3] = inBaseTextureCoords[3];

			IMPiEnv_Parse_GQ_Tri_BuildNormal(inBuildData, ri1, ri2, ri3, texture_coords, inDontUseThisNormal);

			goto exit;
		}
	}

	d = -(point_list[ri0].x * normal.x + point_list[ri0].y * normal.y + point_list[ri0].z * normal.z);

	if (MUrVector_DotProduct(inDontUseThisNormal, &normal) < 0.f) {
		MUmVector_Negate(normal);
		d = -d;

		texture_coords[2] = inBaseTextureCoords[0];
		texture_coords[1] = inBaseTextureCoords[1];
		texture_coords[0] = inBaseTextureCoords[2];
		texture_coords[3] = inBaseTextureCoords[3];

		IMPiEnv_Parse_GQ_AddQuad(inBuildData, ri2, ri1, ri0, ri3, texture_coords, normal.x, normal.y, normal.z, d,	0);
	}
	else {
		IMPiEnv_Parse_GQ_AddQuad(inBuildData, ri0, ri1, ri2, ri3, inBaseTextureCoords, normal.x, normal.y, normal.z, d,	0);
	}

exit:
	return;
}

	

static UUtError
IMPiEnv_Parse_GQ_Quad(
	IMPtEnv_BuildData*	inBuildData,
	MXtNode*			inNode,
	MXtFace*			inFace)
{
	UUtUns32		ri0, ri1, ri2, ri3;
	M3tPoint3D*		curPoint;
	M3tTextureCoord	gqTexCoords[4];
	
	curPoint = IMPgEnv_RawPoints + inFace->indices[0];

	ri0 = IMPgEnv_PointIndexRemap[inFace->indices[0]];
	ri1 = IMPgEnv_PointIndexRemap[inFace->indices[1]];
	ri2 = IMPgEnv_PointIndexRemap[inFace->indices[2]];
	ri3 = IMPgEnv_PointIndexRemap[inFace->indices[3]];

	gqTexCoords[0] = IMPgEnv_TextureCoords[inFace->indices[0]];
	gqTexCoords[1] = IMPgEnv_TextureCoords[inFace->indices[1]];
	gqTexCoords[2] = IMPgEnv_TextureCoords[inFace->indices[2]];
	gqTexCoords[3] = IMPgEnv_TextureCoords[inFace->indices[3]];
	
	
	if(!(IMPg_Gunk_Flags & AKcGQ_Flag_NoTextureMask))
	{
		IMPg_Gunk_MaterialName = inNode->materials[inFace->material].name;

		IMPg_Gunk_TextureName = inNode->materials[inFace->material].maps[MXcMapping_DI].name;
		
		if(!strcmp(IMPg_Gunk_TextureName, "<none>"))
		{
			IMPg_Gunk_TextureName = "NONE";
		}
		else if(!strcmp(IMPg_Gunk_MaterialName, "<none>"))
		{
			IMPg_Gunk_TextureName = "NONE";
		}
	}
	else
	{
		IMPg_Gunk_TextureName = "BLUEGRID02.PSD";
		IMPg_Gunk_MaterialName = "BLUEGRID02.PSD";
	}
	
	UUrString_Capitalize(IMPg_Gunk_TextureName, BFcMaxFileNameLength);
					
	IMPiEnv_Parse_GQ_Quad_BuildNormal(inBuildData, ri0, ri1, ri2, ri3, gqTexCoords, &inFace->dont_use_this_normal);
	
	return UUcError_None;
}

static UUtError
IMPiEnv_Parse_GQ_Tri(
	IMPtEnv_BuildData*	inBuildData,
	MXtNode*			inNode,
	MXtFace*			inFace)
{
	UUtUns32			ri0, ri1, ri2;
	M3tPoint3D*			curPoint;
	M3tTextureCoord		gqTexCoords[4];
	
	curPoint = IMPgEnv_RawPoints + inFace->indices[0];

	gqTexCoords[0] = IMPgEnv_TextureCoords[inFace->indices[0]];
	gqTexCoords[1] = IMPgEnv_TextureCoords[inFace->indices[1]];
	gqTexCoords[2] = IMPgEnv_TextureCoords[inFace->indices[2]];
	gqTexCoords[3] = IMPgEnv_TextureCoords[inFace->indices[2]];

	ri0 = IMPgEnv_PointIndexRemap[inFace->indices[0]];
	ri1 = IMPgEnv_PointIndexRemap[inFace->indices[1]];
	ri2 = IMPgEnv_PointIndexRemap[inFace->indices[2]];

	
	if(!(IMPg_Gunk_Flags & AKcGQ_Flag_NoTextureMask))
	{
		IMPg_Gunk_MaterialName = inNode->materials[inFace->material].name;

		IMPg_Gunk_TextureName = inNode->materials[inFace->material].maps[MXcMapping_DI].name;

		if(!strcmp(IMPg_Gunk_TextureName, "<none>"))
		{		
			IMPg_Gunk_TextureName = "NONE";
		}
		else if(!strcmp(IMPg_Gunk_MaterialName, "<none>"))
		{
			IMPg_Gunk_TextureName = "NONE";
		}
	}
	else
	{
		IMPg_Gunk_TextureName = "BLUEGRID02.PSD";
		IMPg_Gunk_MaterialName = "BLUEGRID02.PSD";
	}
	
	UUrString_Capitalize(IMPg_Gunk_TextureName, BFcMaxFileNameLength);
		
	IMPiEnv_Parse_GQ_Tri_BuildNormal(inBuildData, ri0, ri1, ri2, gqTexCoords, &inFace->dont_use_this_normal);
	
	return UUcError_None;
}

static UUtError
IMPiEnv_Parse_GQ_Flag(
	IMPtEnv_BuildData*	inBuildData,
	MXtNode*			inNode,
	BFtFileRef			*inFileRef,
	const char*			inFileName)
{
	UUtError			error;

	IMPtEnv_Flag		*pFlag;

	// Get the user data for the flag
	pFlag = inBuildData->flagList + inBuildData->numFlags;

	error = UUrString_To_Int32(inNode->name + strlen(IMPcGunkFlagNamePrefix), &pFlag->idNumber);

	if (error) {
		Imp_PrintWarning("failed to parse flag %s in file %s", 
			inNode->name, 
			BFrFileRef_GetLeafName(inFileRef));

		//UUmError_ReturnOnError(error);
		return UUcError_None;
	}

	pFlag->matrix = inNode->matrix;
					
	inBuildData->numFlags += 1;
			
	return UUcError_None;
}

static UUtError
IMPiEnv_Parse_GQ_Object_GeometryList(
	IMPtEnv_BuildData		*inBuildData,
	IMPtEnv_Object			*setupO,
	MXtNode*				inNode)
{
	// Reads the list of deformed geometries for this object
	UUtError 			error;
	char				*textureName;
	char*				geometryFileDirName;
	char				geometryFileDirNameMunged[128];
	BFtFileRef*			geometryFileDirRef;
	BFtFileRef*			geometryFileRefArray[IMPcEnv_MaxFilesInDir];
	UUtUns16			numGeometryFiles;
	UUtUns16			curEnvGeometryIndex;
	M3tGeometry*		*curEnvGeometry;
	char				tempFileName[128],munge[64];
	GRtGroup			*group;
	GRtGroup_Context	*groupContext;
	M3tGeometry			*objectGeometry;
	MXtHeader*			geomData;
	MXtNode*			curNode;
	
	// Determine where to find the geometries
	error = GRrGroup_GetString(inBuildData->environmentGroup, "propsDirectory", &geometryFileDirName);

	if (UUcError_None != error)  {
		// CB: fail quietly!
		// AUrMessageBox(AUcMBType_OK, "The main environment _ins is missing $propsDirectory, should work for now.");

		setupO->numDefGeometry = 0;
		return UUcError_None;
	}
	
	UUrString_Copy(munge, inNode->name + strlen(IMPcGunkObjectNamePrefix), 64);
	
	sprintf(geometryFileDirNameMunged,"%s\\%s", geometryFileDirName, munge);
	
	UUrString_NukeNumSuffix(geometryFileDirNameMunged, 128);
	
	// Fetch the list of geometries
	UUmError_ReturnOnErrorMsg(error, "Could not get props texture file info");
	error = BFrFileRef_MakeFromName(geometryFileDirNameMunged, &geometryFileDirRef);
	UUmError_ReturnOnErrorMsg(error, "Could not make props texture file dir ref");
	
	error = 
		BFrDirectory_GetFileList(
			geometryFileDirRef,
			setupO->defGeomPrefix,
			".env",
			IMPcEnv_MaxFilesInDir,
			&numGeometryFiles,
			geometryFileRefArray);

	BFrFileRef_Dispose(geometryFileDirRef);

	if(error != UUcError_None)
	{
		setupO->numDefGeometry = 0;
		return UUcError_None;
	}
		
	// Process and store the geometries
	for(curEnvGeometryIndex = 0, curEnvGeometry = setupO->defGeomList;
		curEnvGeometryIndex < numGeometryFiles;
		curEnvGeometryIndex++, curEnvGeometry++)
	{
		const char *leafName = BFrFileRef_GetLeafName(geometryFileRefArray[curEnvGeometryIndex]);
		Imp_PrintMessage(IMPcMsg_Cosmetic, "%s"UUmNL, leafName);

		UUrString_Copy(tempFileName, leafName, BFcMaxFileNameLength);
		tempFileName[strlen(tempFileName)-5] = '\0'; // blow away the .env
		
		// Open the geometry file
		error = Imp_ParseEnvFile(geometryFileRefArray[curEnvGeometryIndex], &geomData);
		if (error != UUcError_None)
		{
			Imp_PrintWarning("Unable to find deformed geometry %s for object %s"UUmNL,tempFileName,inNode->name);
			continue;
		}
		
		curNode = geomData->nodes;
					
		if (curNode->userDataCount > 0 && *curNode->userData != 0)
		{
			error = GRrGroup_Context_NewFromString(curNode->userData,gPreferencesGroup,NULL,&groupContext,&group);
			IMPmError_ReturnOnError(error);
			GRrGroup_Context_Delete(groupContext);
		}

		textureName = inNode->materials->maps[MXcMapping_DI].name;
		UUrString_Capitalize(textureName,strlen(textureName)+1);

		if(!strcmp(inNode->materials->name, "<none>"))
		{
			textureName = "NONE";
		}
		
		if(textureName == NULL || *textureName == 0)
		{
			textureName = "NONE";
		}
		
		
		// Check to see if we have this geometry yet
		if (IMPgConstructing && !TMrConstruction_Instance_CheckExists(M3cTemplate_Geometry,tempFileName))
		{
			M3tMatrix4x3 identity = {	1.0f,0,0,0,
									0,1.0f,0,0,
									0,0,1.0f,0 };
									
			error = TMrConstruction_Instance_Renew(M3cTemplate_Geometry,tempFileName,0,&objectGeometry);
			UUmError_ReturnOnError(error);
			
			// Parse the geometry
			error = Imp_Node_To_Geometry(geomData->nodes, objectGeometry);
			UUmError_ReturnOnError(error);
			
			objectGeometry->geometryFlags = M3cGeometryFlag_None;			

			// Add the texture placeholder
			UUmAssert(strchr(textureName, '.') == NULL);

			objectGeometry->baseMap = M3rTextureMap_GetPlaceholder(textureName);
			UUmAssert(objectGeometry->baseMap);
		}
		
		Imp_EnvFile_Delete(geomData);
		
		// Create the geometry placeholder
		if(IMPgConstructing)
		{
			error = 
				TMrConstruction_Instance_GetPlaceHolder(
					M3cTemplate_Geometry,
					tempFileName,
					(TMtPlaceHolder*)curEnvGeometry);
		}
		
		// Make a note of the new deformed geometry entry	
		setupO->numDefGeometry++;
		if (setupO->numDefGeometry > IMPcEnv_MaxDefGeometry) break;
	}

	return UUcError_None;
}


static UUtError
IMPiEnv_Parse_GQ_Object(
	IMPtEnv_BuildData*	inBuildData,
	MXtNode*			inNode,
	const char*			inFileName)
{
	UUtError			error;
	GRtGroup_Context*	groupContext;
	GRtGroup*			group;
	GRtElementType		groupType;
	GRtElementArray*	groupFlags;
	UUtUns32			gunkFlags = 0;
	UUtUns32 			flagResults = 0;
	char 				*string,*textureName,animName[OBcMaxObjectName],*objectName,*objectSuffix,junk[128];
	M3tPoint3D			*pointList = NULL;
	M3tMatrix4x3		strip, particle_matrix;
	IMPtEnv_Object 		*setupO;
	IMPtEnv_Object 		*setup_original;
	M3tGeometry			*objectGeometry;
	UUtBool				storeGeom = UUcTrue, skipFX, skipParticle;
	int					scanned,door;
	UUtUns32			tag;
	UUtInt16			node_index = -1;
	char				geometry_name[255];
	UUtUns16			markerItr;
	MXtMarker*			curMarker;
	EPtEnvParticle *	new_particle;

	setupO					= &inBuildData->objectList[inBuildData->numObjects];

	UUrMemory_Clear(setupO,sizeof(IMPtEnv_Object));

	setupO->lifeSpan		= OBcInfinity;
	setupO->bounces			= OBcInfinity;
	setupO->hitpoints		= OBcInfinity;
	setupO->scriptID		= OBcInfinity;
	
	animName[0] = '\0';
	objectName = inNode->name+strlen(IMPcGunkObjectNamePrefix);

	// Store debugging names
	UUrString_Copy(setupO->fileName,	inFileName,		OBcMaxObjectName);
	UUrString_Copy(setupO->objName,		inNode->name,	OBcMaxObjectName);

	// Get the user data for the object
	if (inNode->userDataCount > 0 && *inNode->userData != 0)
	{
		// We have user data, so parse it
		error = GRrGroup_Context_NewFromString(inNode->userData,gPreferencesGroup,NULL,&groupContext,&group);
		IMPmError_ReturnOnError(error);

		// Parse the type field (if any)
		error = GRrGroup_GetElement(group, "type", &groupType, &groupFlags);
		if (error == UUcError_None)
		{
			error = AUrFlags_ParseFromGroupArray(IMPgGunkObjectFlags,groupType,groupFlags,&flagResults);
		}

		// grab the object tag if it exits...
		tag = IMPiEnv_Parse_GQ_ObjectTag( group, inBuildData );
		IMPrEnv_Add_ObjectTag( inBuildData, tag, 0xFFFFFFFF );
		
		// Read the optional fields
		error = GRrGroup_GetElement(group, "lifespan", &groupType, &string);
		if (UUcError_None == error) {
			Imp_PrintMessage(IMPcMsg_Important, "object had obsolete field lifespan");
		}
		
		error = GRrGroup_GetElement(group, "bouncelimit", &groupType, &string);
		if (UUcError_None == error) {
			Imp_PrintMessage(IMPcMsg_Important, "object had obsolete field bounce limit");
		}
		
		error = GRrGroup_GetElement(group, "hitpoints", &groupType, &string);
		if (UUcError_None == error) {
			Imp_PrintMessage(IMPcMsg_Important, "object had obsolete field hitpoints");
		}
		
		error = GRrGroup_GetElement(group, "damage", &groupType, &string);
		if (UUcError_None == error) {
			Imp_PrintMessage(IMPcMsg_Important, "object had obsolete field damage");
		}
		
		error = GRrGroup_GetElement(group, "physics", &groupType, &string);
		if (error == UUcError_None)  sscanf(string,"%u",&setupO->physicsLevel);
		
		error = GRrGroup_GetElement(group, "ref", &groupType, &string);
		if (error == UUcError_None)  sscanf(string,"%u",&setupO->scriptID);
		
		error = GRrGroup_GetString(group, "anim", &string);
		if (error == UUcError_None) UUrString_Copy(animName,string,OBcMaxObjectName);
		
		error = GRrGroup_GetElement(group, "node_index", &groupType, &string);
		if (error == UUcError_None) sscanf(string,"%u",&node_index);
	
		// Read the deformation fields
		error = GRrGroup_GetElement(group,"deftex",&groupType,&string);
		if (error == UUcError_None) UUrString_Copy(setupO->defTexturePrefix,string,IMPcEnv_MaxPrefixLength);

		error = GRrGroup_GetElement(group,"defgeom",&groupType,&string);
		if (error == UUcError_None) UUrString_Copy(setupO->defGeomPrefix,string,IMPcEnv_MaxPrefixLength);
		
		IMPiEnv_Parse_GQ_Object_GeometryList(inBuildData,setupO,inNode);
		
		// Close off the user data
		GRrGroup_Context_Delete(groupContext);
	}
	//else IMPrEnv_LogError("Object %s in file %s has no user data",inNode->name,inFileName);
	
	// Intelligently set the temp and bounce flags
			
	setupO->flags = flagResults;
		
	// find the original setup object if it exists
	setup_original = NULL;

	if( node_index != -1 )
	{
		UUtUns32			i;
		for( i = 0; i < inBuildData->numObjects; i++ )
		{
			if( !UUrString_Compare_NoCase( inBuildData->objectList[i].objName, inNode->name ) )
			{
				setup_original = &inBuildData->objectList[i];
			}			
		}
	}
	
	if( setup_original )
	{
		setupO = setup_original;
	}
	else
	{
		inBuildData->numObjects++;
		if (inBuildData->numObjects > IMPcEnv_MaxObjects)
		{
			inBuildData->numObjects--;
			IMPrEnv_LogError("Warning- too many objects in one level (file %s)"UUmNL,inFileName);
		}
		// Set door specific stuff
		if( UUrString_HasPrefix( objectName, IMPcGunkDoorNameInfix ) )
		{
			objectSuffix			= objectName + strlen(IMPcGunkDoorNameInfix);
			string					= junk;
			while (isdigit(*objectSuffix))	*string++ = *objectSuffix++;
			*string					= '\0';
			scanned					= sscanf(junk,"%d",&door);
			setupO->door_id			= door;
			if (!scanned)			return UUcError_Generic;
			if (!(setupO->flags & OBcFlags_NoCollision)) 
				setupO->flags |= OBcFlags_FaceCollision;
		}
		
		// Check to see if we have geometry for this object type yet
		if( node_index == -1 )
		{
			if (!IMPgConstructing || TMrConstruction_Instance_CheckExists(M3cTemplate_Geometry,objectName))
				storeGeom = UUcFalse;
		}
	}
		
	if (storeGeom)
	{
		// This object has geometry exported for it, so parse it
		if( node_index == -1 )
			sprintf( geometry_name, "%s", objectName );
		else
			sprintf( geometry_name, "%s_%d", objectName, node_index );

		error = TMrConstruction_Instance_Renew(M3cTemplate_Geometry, geometry_name, 0, &objectGeometry);
		UUmError_ReturnOnError(error);
		objectGeometry->geometryFlags = M3cGeometryFlag_None;			

		error = Imp_Node_To_Geometry(inNode, objectGeometry);
		UUmError_ReturnOnError(error);		

		textureName = inNode->materials->maps[MXcMapping_DI].name;
		
		if(!strcmp(inNode->materials->name, "<none>"))
		{
			textureName = "NONE";
		}
		
		if(textureName == NULL || *textureName == 0)
		{
			textureName = "NONE";
		}
		
		UUrString_Capitalize(textureName, BFcMaxFileNameLength);
		
		// Add the texture placeholder and note the texture for later creation
		UUmAssert(strchr(textureName, '.') == NULL);
		
		objectGeometry->baseMap = M3rTextureMap_GetPlaceholder(textureName);
		UUmAssert(objectGeometry->baseMap);
		
		if ((!textureName) || (textureName[0] == '\0')) 
			IMPg_Gunk_Flags = AKcGQ_Flag_NoTextureMask;
		else 
			IMPg_Gunk_Flags = 0;

		IMPiEnv_Parse_GQ_AddTexture(inBuildData,textureName);
	}
	else
	{
		sprintf( geometry_name, "%s", objectName );
	}
	
	if(IMPgConstructing)
	{
		// Create the geometry placeholder
		error = TMrConstruction_Instance_GetPlaceHolder( M3cTemplate_Geometry, geometry_name, (TMtPlaceHolder*)&setupO->geometry_list[setupO->geometry_count++] );
	}
	
	// Position the object
	strip = inNode->matrix;
	setupO->debugOrigMatrix = strip;
	setupO->position = MUrMatrix_GetTranslation(&strip);
	setupO->position.y += AKcPointCollisionFudge;

	error = MUrMatrix_GetUniformScale(&strip, &setupO->scale);

	if (error != UUcError_None) 
	{ 
		Imp_PrintMessage(IMPcMsg_Critical, "Detected non uniform scale in %s", inNode->name);
		setupO->scale = 1.0f;
	}

	MUrMatrix_StripScale(&strip,&strip);
	MUrMatrixToQuat(&strip,&setupO->orientation);
	
	// Check for animation
	if (*animName && IMPgConstructing)
	{
		error = TMrConstruction_Instance_GetPlaceHolder(
			OBcTemplate_Animation,
			animName,
			(TMtPlaceHolder *)&setupO->animation);
		if (error != UUcError_None) setupO->animation = NULL;
		else setupO->physicsLevel = PHcPhysics_Animate;
	}
	
	// if this node has any markers traverse them
	setupO->numParticles = 0;
	skipFX = UUcFalse;
	skipParticle = UUcFalse;
	for(markerItr = 0, curMarker = inNode->markers;
		markerItr < inNode->numMarkers;
		markerItr++, curMarker++)
	{

		if ((!skipParticle) && (UUrString_HasPrefix(curMarker->name, IMPcEnvParticleMarkerPrefix)))
		{
			if (setupO->numParticles >= IMPcEnv_MaxObjectParticles)
			{
				Imp_PrintWarning("Object %s ran out of particles",setupO->objName);
				skipParticle = UUcTrue;
				continue;
			}

			// work out where the particle is relative to the node
			MUrMatrix_Inverse(&inNode->matrix, &particle_matrix);
			MUrMatrix_Multiply(&particle_matrix, &curMarker->matrix, &particle_matrix);

			new_particle = setupO->particles + setupO->numParticles;
			error = IMPrParse_EnvParticle(&particle_matrix, curMarker->userDataCount, curMarker->userData,
											curMarker->name, inNode->name, inFileName, new_particle);
			if (error == UUcError_None) {
				setupO->numParticles++;
			}
		}
	}
	
	return UUcError_None;
}




UUtUns32 IMPrEnv_GetNodeFlags(MXtNode *inNode)
{
	UUtError error;
	UUtUns32 gunkFlags = 0;
	GRtGroup_Context*	groupContext = NULL;
	GRtGroup*			group;
	GRtElementArray*	groupFlags;
	GRtElementType		groupType;

	if ((0 == inNode->userDataCount) || ('\0' == inNode->userData[0])) {
		goto exit;
	}

	// We have user data, so parse it
	error = GRrGroup_Context_NewFromString(inNode->userData,gPreferencesGroup,NULL,&groupContext,&group);
	if (error != UUcError_None) {
		Imp_PrintWarning("failed to parse env flags for %s", inNode->name);
		goto exit;
	}

	error = GRrGroup_GetElement(group, "type", &groupType, &groupFlags);
	if (error != UUcError_None) {
		goto exit;
	}

	error = 
		AUrFlags_ParseFromGroupArray(
			IMPgGunkFlags,
			groupType,
			groupFlags,
			&gunkFlags);
		
	if (UUcError_None != error) {
		Imp_PrintWarning("gunk in file %s, node %s had an invalid flag", IMPg_Gunk_FileName, inNode->name);
	}

exit:
	if (NULL != groupContext) {
		GRrGroup_Context_Delete(groupContext);
	}

	return gunkFlags;
}

UUtError
IMPrEnv_GetLSData(
	GRtGroup				*inLSGroup,
	float					*outFilterColor,
	IMPtLS_LightType		*outLightType,
	IMPtLS_Distribution		*outDistribution,
	UUtUns32				*outIntensity,
	float					*outBeamAngle,
	float					*outFieldAngle)
{
	UUtError				error;
	GRtElementType			groupType;
	GRtElementType			elemType;
	
	GRtElementArray*		filterColor;
	char*					color;
	char*					type;
	char*					intensity;
	UUtUns16				itr;
	
	// initialize
	outFilterColor[0] = 0.0f;
	outFilterColor[1] = 0.0f;
	outFilterColor[2] = 0.0f;
	*outLightType = IMPcLS_LightType_Point;
	*outDistribution = IMPcLS_Distribution_Diffuse;
	*outIntensity = 0;
	*outBeamAngle = 120.0f;
	*outFieldAngle = 120.0f;
	
	// process group
	error = GRrGroup_GetElement(inLSGroup, "filterColor", &groupType, &filterColor);
	IMPmError_ReturnOnErrorMsg(error, "Could not parse filter color");
	
	if (GRrGroup_Array_GetLength(filterColor) != 3)
	{
		IMPmError_ReturnOnErrorMsg(
			UUcError_Generic,
			"Filter Color must only have 3 floats");
	}
	
	for(itr = 0; itr < 3; itr++)
	{
		error = GRrGroup_Array_GetElement(filterColor, itr, &elemType, &color);
		IMPmError_ReturnOnErrorMsg(error, "Could not parse filter color");
		
		sscanf(color, "%f", &outFilterColor[itr]);
	}
	
	error = GRrGroup_GetElement(inLSGroup, "type", &elemType, &type);
	IMPmError_ReturnOnErrorMsg(error, "Could not ls light type");
	
	if(!strcmp(type, "point"))
	{
		*outLightType = IMPcLS_LightType_Point;
	}
	else if(!strcmp(type, "linear"))
	{
		*outLightType = IMPcLS_LightType_Linear;
	}
	else if(!strcmp(type, "area"))
	{
		*outLightType = IMPcLS_LightType_Area;
	}
	else
	{
		IMPmError_ReturnOnErrorMsg(UUcError_Generic, "unknown ls light type");
	}
	
	error = GRrGroup_GetElement(inLSGroup, "distribution", &elemType, &type);
	if(error == UUcError_None)
	{
		if(!strcmp(type, "diffuse"))
		{
			*outDistribution = IMPcLS_Distribution_Diffuse;
		}
		else if(!strcmp(type, "spot"))
		{
			*outDistribution = IMPcLS_Distribution_Spot;
		}
		else
		{
			IMPmError_ReturnOnErrorMsg(UUcError_Generic, "unknown ls light distribution");
		}
	}

	error = GRrGroup_GetElement(inLSGroup, "intensity", &elemType, &intensity);
	if (error == UUcError_None)
	{
		sscanf(intensity, "%d", outIntensity);
	}
	
	error = GRrGroup_GetElement(inLSGroup, "beamAngle", &elemType, &intensity);
	if(error == UUcError_None)
	{
		sscanf(intensity, "%f", outBeamAngle);
	}
	
	error = GRrGroup_GetElement(inLSGroup, "fieldAngle", &elemType, &intensity);
	if(error == UUcError_None)
	{
		sscanf(intensity, "%f", outFieldAngle);
	}
	
	return UUcError_None;
}


static UUtError
IMPiEnv_Parse_GQ(
	IMPtEnv_BuildData*	inBuildData,
	const char*			inFileName,
	MXtHeader*			inEnvData,
	BFtFileRef*			inFileRef)
{
	UUtError			error;
	GRtGroup_Context*	groupContext;
	GRtGroup*			group;
	GRtElementType		groupType;
	GRtElementArray*	groupFlags;
	UUtUns32			gunkFlags;
	UUtUns32			lmFlags;
	GRtGroup*			lsGroup;
	
	MXtNode*			curNode;
	MXtPoint*			curPoint;
	MXtFace*			curFace;
	MXtMarker*			curMarker;
	
	UUtUns16			nodeItr;
	UUtUns16			faceItr;
	UUtUns16			pointItr;
	UUtUns16			markerItr;
	UUtUns16			scriptID;
	UUtUns32			gq_index;

	char				*materialName,nameStorage[IMPcEnv_MaxNameLength];
	IMPtEnv_GQ			*lastGQ,*itrGQ;
	EPtEnvParticle		*new_particle;

	IMPg_Gunk_FileName	= inFileName;

	for(nodeItr = 0, curNode = inEnvData->nodes;
		nodeItr < inEnvData->numNodes;
		nodeItr++, curNode++)
	{
		materialName = NULL;

		// Check for whitespace prefix
		if (isspace(curNode->name[0]))
		{
			Imp_PrintWarning("Warning: Object %s has a space in front of the name",curNode->name);
		}

		// Check for special gunk types based on name
		scriptID = 0xFFFF;
		if (UUrString_HasPrefix(curNode->name, IMPcGunkFlagNamePrefix))
		{
			// We have a flag
			IMPiEnv_Parse_GQ_Flag(inBuildData, curNode, inFileRef, inFileName);
			continue;
		}
		else if (UUrString_HasPrefix(curNode->name,IMPcGunkObjectNamePrefix))
		{
			// We have an object
			
			// hack- Parse non physics objects as gunk for Dec 1/98 deadline
			// 07/99 - update- artists requested this hack be left in, and it doesn't hurt anything, so...
			if (curNode->userDataCount > 0 && *curNode->userData != 0)
			{
				IMPiEnv_Parse_GQ_Object(inBuildData, curNode, inFileName);
				continue;
			}
		}
		
		gunkFlags = 0;
		lmFlags = 0;

		IMPgEnv_LS				= UUcFalse;
		IMPg_Gunk_ObjectTag		= 0xFFFFFFFF;
		IMPgEnv_LS_ParamFlags	= IMPcLS_LightParam_None;

		if (curNode->userDataCount > 0 && *curNode->userData != 0)
		{
			// We have user data, so parse it
			error = GRrGroup_Context_NewFromString(curNode->userData,gPreferencesGroup,NULL,&groupContext,&group);
			UUmError_ReturnOnError(error);
			
			error  = GRrGroup_GetUns16(group,"ref",&scriptID);
			if (error != UUcError_None) scriptID = 0xFFFF;
			
			error  = GRrGroup_GetString(group,"material",&materialName);
			if (error != UUcError_None) materialName = NULL;
			else
			{
				UUrString_Copy(nameStorage,materialName,IMPcEnv_MaxNameLength);
				materialName = nameStorage;
			}

			gunkFlags = IMPrEnv_GetNodeFlags(curNode);

			error = GRrGroup_GetElement(group, "lm", &groupType, &groupFlags);
			if(error == UUcError_None)
			{
				error = 
					AUrFlags_ParseFromGroupArray(
						IMPgLMFlags,
						groupType,
						groupFlags,
						&lmFlags);
			}
			
			error = GRrGroup_GetElement(group, "lsparams", &groupType, &groupFlags);
			if(error == UUcError_None)
			{
				error = 
					AUrFlags_ParseFromGroupArray(
						IMPgLSParamFlags,
						groupType,
						groupFlags,
						&IMPgEnv_LS_ParamFlags);
			}

			// get the ls info if any
			error = GRrGroup_GetElement(group, "ls", &groupType, &lsGroup);
			if (error == UUcError_None)
			{
				error =
					IMPrEnv_GetLSData(
						lsGroup,
						IMPgEnv_LS_FilterColor,
						&IMPgEnv_LS_Type,
						&IMPgEnv_LS_Distribution,
						&IMPgEnv_LS_Intensity,
						&IMPgEnv_LS_BeamAngle,
						&IMPgEnv_LS_FieldAngle);
				if (error == UUcError_None)
				{
					IMPgEnv_LS = UUcTrue;
				}
			}
			
			// grab the object tag if it exits...
			IMPg_Gunk_ObjectTag = IMPiEnv_Parse_GQ_ObjectTag( group, inBuildData );
			GRrGroup_Context_Delete(groupContext);
		}
		
		IMPg_Gunk_FileName	= inFileName;
		IMPg_Gunk_ObjName	= curNode->name;
		IMPg_Gunk_Flags		= gunkFlags;
		IMPg_Gunk_LMFlags	= (UUtUns16) lmFlags;
		IMPg_Gunk_ScriptID	= scriptID;
		
		for(pointItr = 0, curPoint = curNode->points;
			pointItr < curNode->numPoints;
			pointItr++, curPoint++)
		{
			MUrMatrix_MultiplyPoint(
				&curPoint->point,
				&curNode->matrix,
				&curPoint->point);
			
			error = 
				IMPiEnv_Parse_Point(
					inBuildData,
					pointItr,
					curPoint);
			UUmError_ReturnOnError(error);
		}

		for(faceItr = 0, curFace = curNode->triangles;
			faceItr < curNode->numTriangles;
			faceItr++, curFace++)
		{
			MUrMatrix_MultiplyNormal(
				&curFace->dont_use_this_normal,
				&curNode->matrix,
				&curFace->dont_use_this_normal);

			error = 
				IMPiEnv_Parse_GQ_Tri(
					inBuildData,
					curNode,
					curFace);
			UUmError_ReturnOnError(error);
		}
		
		gq_index = inBuildData->numGQs;
		lastGQ = inBuildData->gqList + inBuildData->numGQs;
		for(faceItr = 0, curFace = curNode->quads; faceItr < curNode->numQuads; faceItr++, curFace++)
		{
			MUrMatrix_MultiplyNormal(
				&curFace->dont_use_this_normal,
				&curNode->matrix,
				&curFace->dont_use_this_normal);

			error = IMPiEnv_Parse_GQ_Quad(inBuildData, curNode, curFace);
			UUmError_ReturnOnError(error);

			// After previous statement, some indeterminate number of quads were added.
			// We need to set the material and object id for each of them
			for (itrGQ = lastGQ; itrGQ < inBuildData->gqList + inBuildData->numGQs; itrGQ++)
			{
				if (materialName)
				{
					UUrString_Copy(itrGQ->gqMaterialName,materialName,IMPcEnv_MaxNameLength);
				}
				else itrGQ->gqMaterialName[0] = '\0';
			}
		}		

		// look for environmental-particle markers.
		for (markerItr = 0, curMarker = curNode->markers; markerItr < curNode->numMarkers; markerItr++, curMarker++)
		{
			if (inBuildData->numEnvParticles >= IMPcEnv_MaxEnvParticles)
				break;

			if (!UUrString_HasPrefix(curMarker->name, IMPcEnvParticleMarkerPrefix)) {
				continue;
			}

			new_particle = inBuildData->envParticles + inBuildData->numEnvParticles;
			error = IMPrParse_EnvParticle(&curMarker->matrix, curMarker->userDataCount, curMarker->userData,
											curMarker->name, curNode->name, inFileName, new_particle);
			if (error == UUcError_None) {
				inBuildData->numEnvParticles++;

				if (inBuildData->numEnvParticles >= IMPcEnv_MaxEnvParticles) {
					Imp_PrintWarning("IMPiEnv_Parse_GQ: too many environmental particles (max %d)",
									IMPcEnv_MaxEnvParticles);
				}
			}
		}
	}
	
	// eventually dispose envData

	return UUcError_None;
}


static UUtUns32 IMPiEnv_Parse_ComputeNumGQsNeeded(MXtNode*	inNode)
{
	UUtUns32	max_gqs_needed = inNode->numTriangles + (inNode->numQuads * 2); // quads might be subdivided into 2 triangles
	
	return max_gqs_needed;
}

static void IMPiEnv_Add_Required_Textures(BFtFileRef *inFileRef, IMPtEnv_BuildData *inBuildData)
{
	UUtError error;
	BFtFileRef required_textures_ref = *inFileRef;

	BFrFileRef_AppendName(&required_textures_ref, "required_textures.txt");

	if (BFrFileRef_FileExists(&required_textures_ref)) {
		BFtTextFile *text_file;
		// ok we have a required_textures.txt file

		error = BFrTextFile_OpenForRead(&required_textures_ref, &text_file);

		if (UUcError_None == error) {
			char *current_line;

			Imp_PrintMessage(IMPcMsg_Important, UUmNL"adding required textures"UUmNL);

			for(current_line = BFrTextFile_GetNextStr(text_file); current_line != NULL; current_line = BFrTextFile_GetNextStr(text_file))
			{
				UUtUns32 ignored_new_index;
				char upper_case_extension_stripped_string[512];

				if (0 == UUrString_Compare_NoCase_NoSpace(current_line, "")) {
					continue;
				}

				strcpy(upper_case_extension_stripped_string, current_line);

				UUrString_Capitalize(upper_case_extension_stripped_string, BFcMaxFileNameLength);
				UUrString_StripExtension(upper_case_extension_stripped_string);

				AUrSharedStringArray_AddString(inBuildData->textureMapArray, upper_case_extension_stripped_string, 0, &ignored_new_index);
			}
		}

		BFrTextFile_Close(text_file);
	}

	return;
}

static void IMPiEnv_Add_Skip_Textures(BFtFileRef *inFileRef, IMPtEnv_BuildData *inBuildData)
{
	UUtError error;
	BFtFileRef skipped_textures_ref = *inFileRef;
	AUtSharedString *shared_strings = AUrSharedStringArray_GetList(inBuildData->textureMapArray);

	BFrFileRef_AppendName(&skipped_textures_ref, "skip_textures.txt");

	if (BFrFileRef_FileExists(&skipped_textures_ref)) {
		BFtTextFile *text_file;
		// ok we have a required_textures.txt file

		error = BFrTextFile_OpenForRead(&skipped_textures_ref, &text_file);

		if (UUcError_None == error) {
			char *current_line;

			Imp_PrintMessage(IMPcMsg_Important, UUmNL"adding required textures"UUmNL);

			for(current_line = BFrTextFile_GetNextStr(text_file); current_line != NULL; current_line = BFrTextFile_GetNextStr(text_file))
			{
				UUtUns32 index;
				char upper_case_extension_stripped_string[512];

				if (0 == UUrString_Compare_NoCase_NoSpace(current_line, "")) {
					continue;
				}

				strcpy(upper_case_extension_stripped_string, current_line);

				UUrString_Capitalize(upper_case_extension_stripped_string, BFcMaxFileNameLength);
				UUrString_StripExtension(upper_case_extension_stripped_string);

				if (AUrSharedStringArray_GetIndex(inBuildData->textureMapArray, upper_case_extension_stripped_string, &index)) {
					shared_strings[index].data = 1;

					Imp_PrintMessage(IMPcMsg_Important, "skip_texture %s"UUmNL, upper_case_extension_stripped_string);
				}
			}

			BFrTextFile_Close(text_file);
		}
	}

	return;
}
static UUtError
IMPrEnv_Parse_Textures(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inModTime,
	GRtGroup*			inGroup,
	IMPtEnv_BuildData*	inBuildData,
	UUtBool				*ioBuildInstance)
{
	UUtError			error;
	
	char*				textureFileDirName;
	BFtFileRef*			textureFileDirRef;
	BFtFileRef*			textureFileRefArray[IMPcEnv_MaxTextures];
	UUtUns16			numTextureFiles;
	UUtUns16			curTextureIndex;
	
	UUtUns16			curFileIndex;
	
	char				tempFileName[BFcMaxFileNameLength];
	char				suffix[4];

	UUtUns32			numTextures;
	AUtSharedString*	curEnvTextureRef;
	IMPtEnv_Texture*	curEnvTexture;
	
	
	AUtSharedString*		envTextureList;
	AUtSharedString*		textureDirectoryList;
	AUtSharedStringArray*	textureDirectoryArray;
	UUtUns32				textureIndex;
	
	UUtUns32*			envTextureSortedIndices;
	
	const char*			textureRefSuffix;
	//char*				string;

	
	
	UUtInt64 time = UUrMachineTime_High();

	Imp_PrintMessage(IMPcMsg_Important, UUmNL"Environment Texturemap");

	error = GRrGroup_GetString(inGroup, "textureDirectory", &textureFileDirName);
	UUmError_ReturnOnErrorMsg(error, "Could not get texture file info");

	error = BFrFileRef_DuplicateAndReplaceName(inSourceFileRef, textureFileDirName, &textureFileDirRef);
	UUmError_ReturnOnErrorMsg(error, "Could not make texture file dir ref");

	// add the textures we require and the textures we will skip (just make placeholders for)
	IMPiEnv_Add_Required_Textures(textureFileDirRef, inBuildData);
	IMPiEnv_Add_Skip_Textures(textureFileDirRef, inBuildData);
		
	error = 
		BFrDirectory_GetFileList(
			textureFileDirRef,
			NULL,
			NULL,
			IMPcEnv_MaxTextures,
			&numTextureFiles,
			textureFileRefArray);
	UUmError_ReturnOnErrorMsg(error, "Could not get files in bnv dir");
	
	BFrFileRef_Dispose(textureFileDirRef);
	
	textureDirectoryArray = AUrSharedStringArray_New();
	UUmError_ReturnOnNull(textureDirectoryArray);
			
	for(curTextureIndex = 0; curTextureIndex < numTextureFiles; curTextureIndex++)
	{
		textureRefSuffix = BFrFileRef_GetSuffixName(textureFileRefArray[curTextureIndex]);
		if(textureRefSuffix == NULL) continue;

		if( strlen(textureRefSuffix) > 3 )		continue;

		UUrString_Copy(
			suffix,
			textureRefSuffix,
			4);
		
		UUrString_Capitalize(suffix, 4);
		
		if(strncmp(suffix, "BMP", 3) && strncmp(suffix, "PSD", 3)) continue;
		
		UUrString_Copy(
			tempFileName,
			BFrFileRef_GetLeafName(textureFileRefArray[curTextureIndex]),
			BFcMaxFileNameLength);
		
		UUrString_Capitalize(tempFileName, BFcMaxFileNameLength);
		UUrString_StripExtension(tempFileName);
		
		error = 
			AUrSharedStringArray_AddString(
				textureDirectoryArray,
				tempFileName,
				curTextureIndex,
				NULL);
		UUmError_ReturnOnError(error);
	}
	
	numTextures = AUrSharedStringArray_GetNum(inBuildData->textureMapArray);
	
	textureDirectoryList = AUrSharedStringArray_GetList(textureDirectoryArray);
	
	for(curTextureIndex = 0,
			curEnvTextureRef = AUrSharedStringArray_GetList(inBuildData->textureMapArray),
			curEnvTexture = inBuildData->envTextureList;
		curTextureIndex < numTextures;
		curTextureIndex++, curEnvTextureRef++, curEnvTexture++)
	{
#if PRINT_ENV_TEXTURE_LIST
		Imp_PrintMessage(IMPcMsg_Important, "%s\n", curEnvTextureRef->string);
#else
		if((curTextureIndex % 10) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}
#endif

		Imp_ClearTextureFlags(&curEnvTexture->flags);
		curEnvTexture->flags.hasFlagFile = cTextureFlagFile_Yes;
		curEnvTexture->flags.flags = M3cTextureFlags_HasMipMap;

		curEnvTexture->texture = NULL;
		
		if(AUrSharedStringArray_GetIndex(textureDirectoryArray, curEnvTextureRef->string, &textureIndex))
		{
			if(IMPgConstructing)
			{
				if (curEnvTextureRef->data) {
					Imp_PrintMessage(IMPcMsg_Important, "texture %s not created"UUmNL, curEnvTextureRef->string);
					curEnvTexture->texture = M3rTextureMap_GetPlaceholder(curEnvTextureRef->string);
					UUmAssert(curEnvTexture->texture);
				}
				else {
					error = 
						Imp_ProcessTexture(
							textureFileRefArray[textureDirectoryList[textureIndex].data],
							IMPgLowMemory ? 2 : 0,
							&curEnvTexture->flags,
							&curEnvTexture->flags,
							curEnvTextureRef->string,
							IMPcIgnoreTextureRef);
					IMPmError_ReturnOnErrorMsg(error, "Could not make texture");
					
					UUmAssert(strchr(curEnvTextureRef->string, '.') == NULL);

					curEnvTexture->texture = M3rTextureMap_GetPlaceholder(curEnvTextureRef->string);
					UUmAssert(curEnvTexture->texture);
				}
			}
			else
			{
				Imp_ProcessTextureFlags_FromFileRef(
					textureFileRefArray[textureDirectoryList[textureIndex].data],
					&curEnvTexture->flags);
			}
		}
		else if(IMPgConstructing)
		{
			IMPrEnv_LogError(
				"Could not find texture %s",
				curEnvTextureRef->string);
				
			curEnvTexture->texture = M3rTextureMap_GetPlaceholder("NONE");
			UUmAssert(curEnvTexture->texture);
		}
	}
	
	Imp_PrintMessage(IMPcMsg_Important, UUmNL);
	fprintf(IMPgEnv_StatsFile, "**Texture Map stats"UUmNL);
	
	envTextureSortedIndices = AUrSharedStringArray_GetSortedIndexList(inBuildData->textureMapArray);
	envTextureList = AUrSharedStringArray_GetList(inBuildData->textureMapArray);
	
	for(curTextureIndex = 0;
		curTextureIndex < numTextures;
		curTextureIndex++)
	{
		fprintf(IMPgEnv_StatsFile, "\t%s"UUmNL, 
			envTextureList[envTextureSortedIndices[curTextureIndex]].string);
	}

	AUrSharedStringArray_Delete(textureDirectoryArray);

	for(curFileIndex = 0; curFileIndex < numTextureFiles; curFileIndex++)
	{
		BFrFileRef_Dispose(textureFileRefArray[curFileIndex]);
	}
			
	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));

	return UUcError_None;
}


UUtError
IMPrEnv_Parse(
	BFtFileRef*			inSourceFileRef,
	GRtGroup*			inGroup,
	IMPtEnv_BuildData*	inBuildData,
	UUtBool				*ioBuildInstance)
{
	UUtError			error;

	//char*				bnvFileDirName;
	//char*				gqFileDirName;
	
	
	UUtUns16			numBNVfiles;
	BFtFileRef*			bnvFileDirRef;
	BFtFileRef*			bnvFileRefArray[IMPcEnv_MaxFilesInDir];
	MXtHeader*			bnvEnvDataArray[IMPcEnv_MaxFilesInDir];
	
	UUtUns16			numGQfiles;
	BFtFileRef*			gqFileDirRef;
	BFtFileRef*			gqFileRefArray[IMPcEnv_MaxFilesInDir];
	MXtHeader*			gqEnvDataArray[IMPcEnv_MaxFilesInDir];
		
	UUtUns16			curFileIndex;
	
	UUtBool				buildInstance;
	UUtUns32			createDate = 0;
	UUtUns32			curModTime;

	
	M3tPoint3D			*pointList;
	
	
	
	UUtUns16			nodeItr;
	MXtNode*			curNode;
	const char*			leafName;
	//char*				string;

	UUtUns32			newIndex;
	
	M3tColorRGB			reflectivity_color;
	
	UUtUns32			numGQsNeeded;
	UUtInt64			time;
	
	AUrSharedStringArray_Reset(inBuildData->textureMapArray);
	
	// setup default door frame texture
	error = AUrSharedStringArray_AddString( inBuildData->textureMapArray, "_DOOR_FRAME", 0, &newIndex);
	UUmAssert( error == UUcError_None );
	inBuildData->door_frame_texture = (UUtUns16) newIndex;

	// Get the file refs
	error =
		BFrFileRef_DuplicateAndReplaceName(
			inSourceFileRef,
			"Conversion_Files\\BNVs",
			&bnvFileDirRef);
	UUmError_ReturnOnError(error);

	error =
		BFrFileRef_DuplicateAndReplaceName(
			inSourceFileRef,
			"Conversion_Files\\Gunk",
			&gqFileDirRef);
	UUmError_ReturnOnError(error);
	
	// Get the default reflectivity
	inBuildData->reflectivity_specified = UUcFalse;
	
	error = 
		M3rGroup_GetColor(
			inGroup,
			"reflectivity",
			&reflectivity_color);
	if(error == UUcError_None)
	{
		inBuildData->reflectivity_specified = UUcTrue;
		inBuildData->reflectivity_color = reflectivity_color;

		Imp_PrintMessage(IMPcMsg_Important, "reflectivity = %f,%f,%f"UUmNL, inBuildData->reflectivity_color.r, inBuildData->reflectivity_color.g, inBuildData->reflectivity_color.b);
	}
		
	
	// Get the file lists
	error = 
		BFrDirectory_GetFileList(
			bnvFileDirRef,
			NULL,
			".env",
			IMPcEnv_MaxFilesInDir,
			&numBNVfiles,
			bnvFileRefArray);
	UUmError_ReturnOnErrorMsg(error, "Could not get files in bnv dir");
	
	error = 
		BFrDirectory_GetFileList(
			gqFileDirRef,
			NULL,
			".env",
			IMPcEnv_MaxFilesInDir,
			&numGQfiles,
			gqFileRefArray);
	UUmError_ReturnOnErrorMsg(error, "Could not get files in gq dir");
			
		
	buildInstance = *ioBuildInstance;
	
	for(curFileIndex = 0; curFileIndex < numBNVfiles && !buildInstance; curFileIndex++)
	{
		error = BFrFileRef_GetModTime(bnvFileRefArray[curFileIndex], &curModTime);
		UUmError_ReturnOnErrorMsg(error, "Could not get mod time");

		if(createDate < curModTime) {
			buildInstance = UUcTrue;
		}
	}

	for(curFileIndex = 0; curFileIndex < numGQfiles && !buildInstance; curFileIndex++)
	{
		error = BFrFileRef_GetModTime(gqFileRefArray[curFileIndex], &curModTime);
		UUmError_ReturnOnErrorMsg(error, "Could not get mod time");

		if(createDate < curModTime) {
			buildInstance = UUcTrue;
		}
	}

	if(!buildInstance) {
		return UUcError_None;
	}
	// Collect all the dirtmap file names
	inBuildData->maxBNVs = 0;
		
	time = UUrMachineTime_High();
	Imp_PrintMessage(IMPcMsg_Important, "Parsing BNVs");
	
	for(curFileIndex = 0; curFileIndex < numBNVfiles; curFileIndex++)
	{
		if((curFileIndex % 5) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}
		
		leafName = BFrFileRef_GetLeafName(bnvFileRefArray[curFileIndex]);
		Imp_PrintMessage(IMPcMsg_Cosmetic, "%s"UUmNL, leafName);

		error = Imp_ParseEnvFile(bnvFileRefArray[curFileIndex], &bnvEnvDataArray[curFileIndex]);
		UUmError_ReturnOnError(error);
		
		inBuildData->maxBNVs += bnvEnvDataArray[curFileIndex]->numNodes;
	}
	
	inBuildData->bnvList = UUrMemory_Block_New(sizeof(IMPtEnv_BNV) * inBuildData->maxBNVs);
	UUmError_ReturnOnNull(inBuildData->bnvList);
	
	for(curFileIndex = 0; curFileIndex < numBNVfiles; curFileIndex++)
	{
		if((curFileIndex % 5) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}
		error =
			IMPiEnv_Parse_BNV(
				BFrFileRef_GetLeafName(bnvFileRefArray[curFileIndex]),
				bnvEnvDataArray[curFileIndex],
				inBuildData);
		UUmError_ReturnOnError(error);
	}
	
	for(curFileIndex = 0; curFileIndex < numBNVfiles; curFileIndex++)
	{
		Imp_EnvFile_Delete(bnvEnvDataArray[curFileIndex]);
	}

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));
	
	time = UUrMachineTime_High();
	Imp_PrintMessage(IMPcMsg_Important, UUmNL"GQs");

	inBuildData->maxGQs = 0;
	inBuildData->numGQFiles = numGQfiles;
	
	numGQsNeeded = 0;
	
	inBuildData->numMarkers = 0;
	for(curFileIndex = 0; curFileIndex < numGQfiles; curFileIndex++)
	{
		if((curFileIndex % 5) == 0)
		{
			Imp_PrintMessage(IMPcMsg_Important, ".");
		}
		leafName = BFrFileRef_GetLeafName(gqFileRefArray[curFileIndex]);
		Imp_PrintMessage(IMPcMsg_Cosmetic, "%s"UUmNL, leafName);

		error = Imp_ParseEnvFile(gqFileRefArray[curFileIndex], &gqEnvDataArray[curFileIndex]);
		UUmError_ReturnOnError(error);
		
		for(nodeItr = 0, curNode = gqEnvDataArray[curFileIndex]->nodes;
			nodeItr < gqEnvDataArray[curFileIndex]->numNodes;
			nodeItr++, curNode++)
		{
			inBuildData->maxGQs += curNode->numTriangles + curNode->numQuads;
			
			numGQsNeeded += IMPiEnv_Parse_ComputeNumGQsNeeded(curNode);
			
			for (newIndex=0; newIndex<curNode->numMarkers; newIndex++)
			{
				if (inBuildData->numMarkers >= IMPcEnv_MaxMarkerNodes) {
					Imp_PrintWarning("Discarding %d marker nodes from ENV file %s - IMPcEnv_MaxMarkerNodes in a level is %d",
						curNode->numMarkers - newIndex, leafName, IMPcEnv_MaxMarkerNodes);
					break;
				}

				inBuildData->markerList[inBuildData->numMarkers] = curNode->markers[newIndex];
				inBuildData->numMarkers++;
			}
		}
		
		UUrString_Copy(inBuildData->gqFileNames[curFileIndex], BFrFileRef_GetLeafName(gqFileRefArray[curFileIndex]), BFcMaxFileNameLength);
	}

	time = UUrMachineTime_High() - time;
	Imp_PrintMessage(IMPcMsg_Important, UUmNL "total time = %f" UUmNL UUmNL, UUrMachineTime_High_To_Seconds(time));
	

	inBuildData->maxGQs = numGQsNeeded + (numGQsNeeded >> 1);

	if(inBuildData->maxGQs < 4000) inBuildData->maxGQs = 4000;
	
	Imp_PrintMessage(IMPcMsg_Important, UUmNL"max gqs = %d"UUmNL, inBuildData->maxGQs);
			
	inBuildData->gqList = UUrMemory_Block_New(sizeof(IMPtEnv_GQ) * inBuildData->maxGQs);
	UUmError_ReturnOnNull(inBuildData->gqList);
	
	inBuildData->gqTakenBV = UUrBitVector_New(inBuildData->maxGQs);
	UUmError_ReturnOnNull(inBuildData->gqTakenBV);

	for(curFileIndex = 0; curFileIndex < numGQfiles; curFileIndex++)
	{
		IMPg_Gunk_FileRelativeGQIndex = 0;
		
		error =
			IMPiEnv_Parse_GQ(
				inBuildData,
				BFrFileRef_GetLeafName(gqFileRefArray[curFileIndex]),
				gqEnvDataArray[curFileIndex],
				gqFileRefArray[curFileIndex]);
		UUmError_ReturnOnError(error);
	}
	
	Imp_PrintMessage(IMPcMsg_Important,UUmNL"%d found"UUmNL,inBuildData->numGQs);
	pointList = AUrSharedPointArray_GetList(inBuildData->sharedPointArray);
	

	IMPrEnv_Parse_Textures(inSourceFileRef, curModTime, inGroup, inBuildData, ioBuildInstance);
		
	for(curFileIndex = 0; curFileIndex < numGQfiles; curFileIndex++)
	{
		BFrFileRef_Dispose(gqFileRefArray[curFileIndex]);
	}
	
	for(curFileIndex = 0; curFileIndex < numBNVfiles; curFileIndex++)
	{
		BFrFileRef_Dispose(bnvFileRefArray[curFileIndex]);
	}

	for(curFileIndex = 0; curFileIndex < numGQfiles; curFileIndex++)
	{
		Imp_EnvFile_Delete(gqEnvDataArray[curFileIndex]);
	}
	
	BFrFileRef_Dispose(gqFileDirRef);
	BFrFileRef_Dispose(bnvFileDirRef);
	
	*ioBuildInstance = buildInstance;
	
	return UUcError_None;
}
