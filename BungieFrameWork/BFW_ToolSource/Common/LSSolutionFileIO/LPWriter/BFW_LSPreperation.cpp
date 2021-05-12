/*
	FILE:	BFW_LSPreperation.cpp
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Nov 20, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_MathLib.h"
#include "BFW_AppUtilities.h"

#include "BFW_LSSolution.h"

#include "IMP_Env2_Private.h"
#include "BFW_LSPreperation.h"

//#define IMPmLS_BreakIntoTris

static void
iHSV2RGB(
	float h,
	float s,
	float v,
	float *r,
	float *g,
	float *b)
{
	float		f,p,q,t;
	UUtUns32	i;
	
	h = UUmPin(h, 0.0f, 360.0f);

	if (s == 0.0f)
	{
		*r = v;
		*g = v;
		*b = v;
	}
	else
	{
		if (h==360.0f) h = 0.0f;
		h/=60.0f;
		i = (UUtUns32)h;
		f = h - (float)i;
		p = v * (1.0f - s);
		q = v * (1.0f - (s*f));
		t = v * (1.0f - (s*(1.0f - f)));
		switch (i)
		{
			case 0:
				*r = v;
				*g = t;
				*b = p;
				break;
			case 1:
				*r = q;
				*g = v;
				*b = p;
				break;
			case 2:
				*r = p;
				*g = v;
				*b = t;
				break;
			case 3:
				*r = p;
				*g = q;
				*b = v;
				break;
			case 4:
				*r = t;
				*g = p;
				*b = v;
				break;
			case 5:
				*r = v;
				*g = p;
				*b = q;
				break;
			default:
				UUmAssert(0);
		}
	}
}

static UUtBool
iDontExportGQ(
	IMPtEnv_GQ*	inGQ)
{
	if(inGQ->isLuminaire) return UUcFalse;
	if(inGQ->lmFlags & IMPcGQ_LM_ForceOn) return UUcFalse;
	if(inGQ->lmFlags & IMPcGQ_LM_ForceOff) return UUcTrue;
	if(inGQ->flags & AKcGQ_Flag_DontLight) return UUcTrue;
	if(inGQ->flags & AKcGQ_Flag_Invisible) return UUcTrue;
	
	return UUcFalse;
}

static float
iSpotIntensityToFlux(
    float intensity,
    float beamDegress,
    float fieldDegrees)
{
	float	beam = beamDegress * (M3cPi / 180.0f);
	float	field = fieldDegrees * (M3cPi / 180.0f);
	float	cosBeam = cosf(beam);
	float	cosField = cosf(field);
	
    float	spotFlux = (logf(0.5f) / logf(cosBeam)) + 1.0f;
    
    if (cosField <= 0.0f)
    {
		spotFlux = (float)(2.0 * M3cPi / spotFlux);
	}
    else
    {
		spotFlux = (float)((2.0 * M3cPi / spotFlux) * (1.0 - powf(cosField, spotFlux)));
    }
    
    return intensity * spotFlux;
}

BFW_LPWriter::BFW_LPWriter(
	const char*			inGQFileName,
	IMPtEnv_BuildData*	inBuildData)
{
	gqFileName = inGQFileName;
	buildData = inBuildData;
}

LtTBool BFW_LPWriter::OnWriteDefaults(LtTPrepDefaultsApi& writer)
{
    writer.SetLengthConversion(LT_LENGTH_M, 0.1f);

    writer.SetAngleConversion(LT_ANGLE_DEGREES, 1.0f);

    writer.SetLengthUnits(LT_LENGTH_M);
	
	writer.SetIntensityConversion(LT_INTENS_SI);

	writer.SetTexturePath("F:\\BungieDevelopment\\BungieGameData\\OniGameData\\Environment\\SharedTextures\\Textures");

    // Set the model extents
    LtTPoint minPoint(-100.0f, -100.0f, -100.0f), maxPoint(100.0f, 100.0f, 100.0f);
    writer.SetModelExtents(minPoint, maxPoint);

    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteDisplay(LtTPrepDisplayApi& writer)
{

    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteReceiver(LtTPrepReceiverApi& writer)
{
    calcTotalArea();
    params.SetMeshing(writer);
    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteSource(LtTPrepSourceApi& writer)
{
    calcTotalArea();
    params.SetMeshing(writer);
    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteShadow(LtTPrepShadowApi& writer)
{
    calcTotalArea();
    params.SetMeshing(writer);
    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteFog(LtTPrepFogApi& writer)
{
    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteNatLight(LtTPrepNatLightApi& writer)
{
    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteView(LtTPrepViewApi& writer)
{
    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteTextures(LtTPrepTextureApi& writer)
{
	UUtUns16	gqItr;
	IMPtEnv_GQ*	curGQ;
	AUtSharedString*	textureArray;
	char*				textureName;
	UUtUns32*			indexList;
	UUtUns32			num;
	AUtSharedStringArray*	newArray;
	UUtUns32			newIndex;
	UUtUns32			itr;	
	//char				temp[128];

	newArray = AUrSharedStringArray_New();

	textureArray = AUrSharedStringArray_GetList(
						buildData->textureMapArray);

	for(gqItr = 0, curGQ = buildData->gqList;
		gqItr < buildData->numGQs;
		gqItr++, curGQ++)
	{
		if(iDontExportGQ(curGQ)) continue;

		if(gqFileName != NULL && strcmp(curGQ->fileName, gqFileName))
		{
			continue;
		}

		textureName = (textureArray + curGQ->textureMapIndex)->string;

		error =
			AUrSharedStringArray_AddString(
				newArray,
				textureName,
				0,
				&newIndex);
	}

	num = AUrSharedStringArray_GetNum(newArray);
	indexList = AUrSharedStringArray_GetSortedIndexList(newArray);
	textureArray = AUrSharedStringArray_GetList(newArray);

	for(itr = 0; itr < num; itr++)
	{

		// We have a valid name. Set the name in the writer
		UUrString_MakeLowerCase(textureArray[indexList[itr]].string, AUcMaxStringLength);
		
		writer.SetName(textureArray[indexList[itr]].string, TRUE);
		writer.SetFilterMethods(LT_TEXTURE_MIN_BILINEAR_MM, LT_TEXTURE_MAG_LINEAR);

		// Finish the texture
		if (!writer.Finish())
			return FALSE;

    }
	
	AUrSharedStringArray_Delete(newArray);

    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteLayers(LtTPrepLayerApi& writer)
{
	writer.SetName("oni_layer");
	writer.SetIsActive(1);
	writer.Finish();

    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteLamps(LtTPrepLampApi& writer)
{
	UUtUns16		gqItr;
	IMPtEnv_GQ*		curGQ;
	char			lampName[256];
	LtTRGBColor		color;
	float			r, g, b;

	for(gqItr = 0, curGQ = buildData->gqList;
		gqItr < buildData->numGQs;
		gqItr++, curGQ++)
	{
		if(iDontExportGQ(curGQ)) continue;

		if(gqFileName != NULL && strcmp(curGQ->fileName, gqFileName))
		{
			continue;
		}
		
		if(!curGQ->isLuminaire) continue;
		
		sprintf(
			lampName,
			"l:%s:%s:%05d",
			curGQ->objName,
			curGQ->fileName,
			curGQ->fileRelativeGQIndex);
		
		writer.SetName(lampName);
		
		switch(curGQ->lightType)
		{
			case IMPcLS_LightType_Point:
				writer.SetType(LT_SOURCE_POINT);
				break;
				
			case IMPcLS_LightType_Linear:
				writer.SetType(LT_SOURCE_LINEAR);
				break;
				
			case IMPcLS_LightType_Area:
				writer.SetType(LT_SOURCE_AREA);
				break;
			
			default:
				UUmAssert(0);
		}
		
		iHSV2RGB(curGQ->filterColor[0], curGQ->filterColor[1], curGQ->filterColor[2], &r, &g, &b);
		color.SetRGB(r, g, b);
		writer.SetFilterColor(color);
		
		if(curGQ->distribution == IMPcLS_Distribution_Diffuse)
		{
			writer.SetDistribution(LT_LAMP_DIFFUSE);
			writer.SetFlux((float)curGQ->intensity * M3cPi);
		}
		else if(curGQ->distribution == IMPcLS_Distribution_Spot)
		{
			writer.SetDistribution(LT_LAMP_SPOTLIGHT);
			writer.SetFlux(iSpotIntensityToFlux(
							(float)curGQ->intensity,
							curGQ->beamAngle * 0.5f,
							curGQ->fieldAngle * 0.5f));
		}
		else
		{
			UUmAssert(0);
		}
		
		writer.SetBeamAngle(curGQ->beamAngle * 0.5f);
		writer.SetFieldAngle(curGQ->fieldAngle * 0.5f);
		
		
		if (!writer.Finish())
		{
			UUtInt32	error = GetError();
			
			fprintf(stderr, "error=%d"UUmNL, error);

			UUmAssert(0);

			return FALSE;
		}
	}
	
    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteMaterials(LtTPrepMaterialApi& writer)
{
	UUtUns16	gqItr;
	IMPtEnv_GQ*	curGQ;
	AUtSharedString*	materialArray;
	AUtSharedString*	textureArray;
	UUtUns32*			indexList;
	UUtUns32			num;
	AUtSharedStringArray*	newArray;
	UUtUns32			newIndex;
	UUtUns32			itr;
	char*				textureName;
	//char				temp[128];

	newArray = AUrSharedStringArray_New();

	for(gqItr = 0, curGQ = buildData->gqList;
		gqItr < buildData->numGQs;
		gqItr++, curGQ++)
	{
		if(iDontExportGQ(curGQ)) continue;

		if(gqFileName != NULL && strcmp(curGQ->fileName, gqFileName))
		{
			continue;
		}

		error =
			AUrSharedStringArray_AddString(
				newArray,
				curGQ->materialName,
				(UUtUns32)curGQ->textureMapIndex,
				&newIndex);
	}

	num = AUrSharedStringArray_GetNum(newArray);
	indexList = AUrSharedStringArray_GetSortedIndexList(newArray);
	materialArray = AUrSharedStringArray_GetList(newArray);
	textureArray = AUrSharedStringArray_GetList(buildData->textureMapArray);

	for(itr = 0; itr < num; itr++)
	{
		textureName = (textureArray + materialArray[indexList[itr]].data)->string;
		
		//sprintf(temp, "%s.bmp", textureName);
		UUrString_MakeLowerCase(textureName, AUcMaxStringLength);

		// We have a valid name. Set the name in the writer
		writer.SetName(materialArray[indexList[itr]].string);

		writer.SetTextureName(textureName);
		
		IMPtEnv_Texture* envTexture = buildData->envTextureList + materialArray[indexList[itr]].data;

		LtTRGBColor color;

		if(envTexture->flags.reflectivity_specified || !buildData->reflectivity_specified) {
			color.SetRGB(envTexture->flags.reflectivity_color.r, envTexture->flags.reflectivity_color.g, envTexture->flags.reflectivity_color.b);
		}
		else {
			color.SetRGB(buildData->reflectivity_color.r, buildData->reflectivity_color.g, buildData->reflectivity_color.b);
		}
		
		writer.SetReflectivity(color);

		if (!writer.Finish())
		{
			UUtInt32	error = GetError();
			
			fprintf(stderr, "error=%d"UUmNL, error);

			UUmAssert(0);

			return FALSE;
		}
    }

	AUrSharedStringArray_Delete(newArray);

    return TRUE;
}

LtTBool BFW_LPWriter::OnWriteBlocks(LtTPrepBlockApi& blocks, LtTPrepEntityApi& entities)
{
	UUtUns16	gqItr;
	IMPtEnv_GQ*	curGQ;

	char		blockName[256];
	char		lampName[256];
	
	LtTPoint	p0(1.0, 0.0, 0.0),
				p1(0.0, 1.0, 0.0),
				p2(0.0, 0.0, 1.0),
				p3(1.0, 1.0, 0.0);
	LtTPoint	uv0, uv1, uv2, uv3;

	M3tTextureCoord*	textCoordArray;
	M3tPoint3D*			pointArray;
	M3tPoint3D*			curPoint;
	M3tPlaneEquation*	planeArray;
	float				a, b, c, d;

	float		fa[3];

	UUtUns16	count = 0;

	float	m[3][3];
			
	pointArray = AUrSharedPointArray_GetList(buildData->sharedPointArray);
	planeArray = AUrSharedPlaneEquArray_GetList(buildData->sharedPlaneEquArray);
	textCoordArray = AUrSharedTexCoordArray_GetList(buildData->sharedTextureCoordArray);

	for(gqItr = 0, curGQ = buildData->gqList;
		gqItr < buildData->numGQs;
		gqItr++, curGQ++)
	{
		if(iDontExportGQ(curGQ)) continue;

		if(gqFileName != NULL && strcmp(curGQ->fileName, gqFileName))
		{
			continue;
		}
		
		count++;

		//if(count >= 0 && count <= 40) continue;
		//if(count >= 41 && count <= 80) continue;
		//if(count != 1) continue;

		sprintf(
			blockName,
			"%s:%d:%s:%05d:%s",
			LScPrepVersion,
			curGQ->planeEquIndex,
			curGQ->fileName,
			curGQ->fileRelativeGQIndex,
			LScPrepEndToken);

		blocks.SetBlockName(blockName);
		
		entities.SetLayer(blockName);
		entities.SetMaterial(curGQ->materialName);

		if(curGQ->isLuminaire)
		{
			AKmPlaneEqu_GetComponents(
				curGQ->planeEquIndex,
				planeArray,
				a, b, c, d);
			
			M3tVector3D	from, to;
			M3tMatrix4x3	matrix;
			
			sprintf(
				lampName,
				"l:%s:%s:%05d",
				curGQ->objName,
				curGQ->fileName,
				curGQ->fileRelativeGQIndex);
			
			from.x = a;
			from.y = -c;
			from.z = b;
			
			to.x = 0.0f;
			to.y = 0.0f;
			to.z = 1.0f;
			
			MUrMatrix_Alignment(
				&from,
				&to,
				&matrix);
			
			m[0][0] = matrix.m[0][0];
			m[1][0] = matrix.m[1][0];
			m[2][0] = matrix.m[2][0];
			m[0][1] = matrix.m[0][1];
			m[1][1] = matrix.m[1][1];
			m[2][1] = matrix.m[2][1];
			m[0][2] = matrix.m[0][2];
			m[1][2] = matrix.m[1][2];
			m[2][2] = matrix.m[2][2];

			blocks.SetTransform(m);
			blocks.SetLampName(lampName);
			
		}
		
		LtTEntityFlags flags = LT_POLY_NONE;
		
		if(curGQ->lightParams & IMPcLS_LightParam_NotOccluding)
		{
			flags |= LT_POLY_NOT_AN_OBSTACLE;
		}
		if(curGQ->lightParams & IMPcLS_LightParam_Window)
		{
			flags |= LT_POLY_WINDOW;
		}
		if(curGQ->lightParams & IMPcLS_LightParam_NotReflecting)
		{
			flags |= LT_POLY_NOT_A_REFLECTOR;
		}
		if(curGQ->flags & AKcGQ_Flag_Transparent)
		{
			flags |= LT_POLY_WINDOW | LT_POLY_TWO_SIDED;
		}
		
		entities.SetFlags(flags);

		curPoint = pointArray + curGQ->visibleQuad.indices[0];
		fa[0] = curPoint->x;
		fa[1] = -curPoint->z;
		fa[2] = curPoint->y;

		p0 = fa;

		curPoint = pointArray + curGQ->visibleQuad.indices[1];
		fa[0] = curPoint->x;
		fa[1] = -curPoint->z;
		fa[2] = curPoint->y;

		p1 = fa;

		curPoint = pointArray + curGQ->visibleQuad.indices[2];
		fa[0] = curPoint->x;
		fa[1] = -curPoint->z;
		fa[2] = curPoint->y;

		p2 = fa;

		uv0 = (float*)(textCoordArray + curGQ->baseMapIndices.indices[0]);
		uv1 = (float*)(textCoordArray + curGQ->baseMapIndices.indices[1]);
		uv2 = (float*)(textCoordArray + curGQ->baseMapIndices.indices[2]);
		
		if(curGQ->flags & AKcGQ_Flag_Triangle)
		{
			entities.SetVertices(p0, p1, p2);
			entities.SetUVs(uv0, uv1, uv2);
			
			if(curGQ->isLuminaire)
			{
				if(curGQ->lightType == IMPcLS_LightType_Area)
				{
					blocks.SetAreaLightPosition(p0, p1, p2);
				}
				else if(curGQ->lightType == IMPcLS_LightType_Point)
				{
					LtTPoint p(
								(p0.GetX() + p1.GetX() + p2.GetX()) / 3.0f,
								(p0.GetY() + p1.GetY() + p2.GetY()) / 3.0f,
								(p0.GetZ() + p1.GetZ() + p2.GetZ()) / 3.0f);

					blocks.SetPointLightPosition(p);
				}
				else
				{
					fprintf(stderr, "Linear light not supported yet\n");
				}
			}
		}
		else
		{
			
			curPoint = pointArray + curGQ->visibleQuad.indices[3];
			fa[0] = curPoint->x;
			fa[1] = -curPoint->z;
			fa[2] = curPoint->y;

			p3 = fa;
			
			uv3 = (float*)(textCoordArray + curGQ->baseMapIndices.indices[3]);

			entities.SetVertices(p0, p1, p2, p3);
			entities.SetUVs(uv0, uv1, uv2, uv3);
			
			if(curGQ->isLuminaire)
			{
				if(curGQ->lightType == IMPcLS_LightType_Area)
				{
					blocks.SetAreaLightPosition(p0, p1, p2, p3);
				}
				else if(curGQ->lightType == IMPcLS_LightType_Point)
				{
					LtTPoint p(
								(p0.GetX() + p1.GetX() + p2.GetX() + p3.GetX()) / 4.0f,
								(p0.GetY() + p1.GetY() + p2.GetY() + p3.GetY()) / 4.0f,
								(p0.GetZ() + p1.GetZ() + p2.GetZ() + p3.GetZ()) / 4.0f);

					blocks.SetPointLightPosition(p);
				}
				else
				{
					fprintf(stderr, "Linear light not supported yet\n");
				}
			}
		}
		
		if(!entities.Finish())
		{
			UUtInt32	error = GetError();
			
			if (error != LT_WRITE_DEGENERATE_POLY &&
				error != LT_WRITE_NONPLANAR_QUAD)
			{
				fprintf(stderr, "error=%d"UUmNL, error);

				UUmAssert(0);
				
				return FALSE;
			}
		}

		if (!blocks.Finish())
		{
			UUtInt32	error = GetError();

			fprintf(stderr, "error=%d"UUmNL, error);

			UUmAssert(0);

			return FALSE;
		}
	}

	fprintf(stderr, "%d"UUmNL, count);

    return TRUE;
}

LtTBool BFW_LPWriter ::OnWriteEntities(LtTPrepEntityApi& writer)
{
	UUtUns16	gqItr;

	IMPtEnv_GQ*	curGQ;
	char		blockName[256];
	float		identity[3][3] = 
				{
					{1.0f, 0.0f, 0.0f},
					{0.0f, 1.0f, 0.0f},
					{0.0f, 0.0f, 1.0f}
				};
	
	UUtUns16	count = 0;

	for(gqItr = 0, curGQ = buildData->gqList;
		gqItr < buildData->numGQs;
		gqItr++, curGQ++)
	{
		if(iDontExportGQ(curGQ)) continue;

		if(gqFileName != NULL && strcmp(curGQ->fileName, gqFileName))
		{
			continue;
		}
		
		count++;

		//if(count >= 0 && count < 40) continue;
		//if(count >= 41 && count <= 80) continue;
		//if(count != 1) continue;

		sprintf(
			blockName,
			"%s:%d:%s:%05d:%s",
			LScPrepVersion,
			curGQ->planeEquIndex,
			curGQ->fileName,
			curGQ->fileRelativeGQIndex,
			LScPrepEndToken);

		writer.SetBlockName(blockName);
		//wrtier.SetLayer(blockName);

		writer.SetInsertionPoint(LtTPoint(0.0f, 0.0f, 0.0f));

		writer.SetTransform(identity);
		
		if (!writer.Finish())
		{
			UUtInt32	error = GetError();

			fprintf(stderr, "error=%d"UUmNL, error);

			UUmAssert(0);

			return FALSE;
		}
		
		#if defined(IMPmLS_BreakIntoTris) && IMPmLS_BreakIntoTris
		
			if(!(curGQ->flags & AKcGQ_Flag_Triangle))
			{
				sprintf(
					blockName,
					"%s:%d:%s:%05d:%s:a",
					LScPrepVersion,
					curGQ->planeEquIndex,
					curGQ->fileName,
					curGQ->fileRelativeGQIndex,
					LScPrepEndToken);

				writer.SetBlockName(blockName);
				//wrtier.SetLayer(blockName);

				writer.SetInsertionPoint(LtTPoint(0.0f, 0.0f, 0.0f));

				writer.SetTransform(identity);

				if (!writer.Finish())
				{
					UUtInt32	error = GetError();

					fprintf(stderr, "error=%d"UUmNL, error);

					UUmAssert(0);

					return FALSE;
				}
			}
			
		#endif
	}
	
	//fprintf(stderr, "%d"UUmNL, count);

    return TRUE;
}

// Calculate total area for model
void BFW_LPWriter::calcTotalArea()
{
	
	return;

	UUtUns16	gqItr;
	IMPtEnv_GQ*	curGQ;

	M3tPoint3D*	pointArray;
	
	float		totalArea = 0.0f;

	pointArray = AUrSharedPointArray_GetList(buildData->sharedPointArray);

	for(gqItr = 0, curGQ = buildData->gqList;
		gqItr < buildData->numGQs;
		gqItr++, curGQ++)
	{
		if(iDontExportGQ(curGQ)) continue;

		if(gqFileName != NULL && strcmp(curGQ->fileName, gqFileName))
		{
			continue;
		}
		
		totalArea += 
			MUrTriangle_Area(
				pointArray + curGQ->visibleQuad.indices[0],
				pointArray + curGQ->visibleQuad.indices[1],
				pointArray + curGQ->visibleQuad.indices[2]);
		
		if(!(curGQ->flags & AKcGQ_Flag_Triangle))
		{
			totalArea += 
				MUrTriangle_Area(
					pointArray + curGQ->visibleQuad.indices[0],
					pointArray + curGQ->visibleQuad.indices[2],
					pointArray + curGQ->visibleQuad.indices[3]);
		}
	}

	params.SetQuality(2);
	params.SetTotalArea(totalArea);

}

#define LScTag	"Model File: "

UUtError
LSrPreperationFile_Create(
	IMPtEnv_BuildData*	inBuildData,
	const char*				inGQFileName,
	const char*				inLPFilePath)
{
	BFW_LPWriter	lpWriter(inGQFileName, inBuildData);
	
	//if(strcmp(inGQFileName, "MP_Gunk_01_cd.env")) return UUcError_None;

	fprintf(stderr, "%s"UUmNL, inGQFileName);

    if (!LtPrepfileExport(inLPFilePath, lpWriter))
	{
		UUmError_ReturnOnError(UUcError_Generic);
    }
	
	// work around lightscape bug
	{
		UUtError		error;
		BFtFileRef*		textFileRef;
		BFtTextFile*	textFile;
		FILE*			outFile;
		char*			str;

		error = BFrFileRef_MakeFromName(inLPFilePath, &textFileRef);
		UUmError_ReturnOnError(error);
		
		error = BFrTextFile_OpenForRead(textFileRef, &textFile);
		UUmError_ReturnOnError(error);

		BFrFile_Delete(textFileRef);

		outFile = fopen(inLPFilePath, "w");
		UUmError_ReturnOnNull(outFile);

		while(1)
		{
			str = BFrTextFile_GetNextStr(textFile);
			if(str == NULL) break;

			if(!strncmp(str, LScTag, strlen(LScTag)))
			{
				fprintf(outFile, LScTag"AvoidLightscapeBug\n");
			}
			else
			{
				fprintf(outFile, "%s\n", str);
			}
		}
		
		fclose(outFile);

		BFrFileRef_Dispose(textFileRef);
		BFrTextFile_Close(textFile);
	}

	return UUcError_None;
}

