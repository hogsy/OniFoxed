/*
	FILE:	BFW_LSBuilder.cpp
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Jan 1, 1998
	
	PURPOSE: 
	
	Copyright 1998

*/

#include "LtTTypes.h"
#include "rmap.h"

#include "BFW_LSSolution.h"
#include "BFW_LSBuilder.h"
#include "BFW_AppUtilities.h"

extern UUtBool	gError;

//-----------------------------------------------------------------------
//			Class TreeInfoBuilder
//
BFW_LSInfoBuilder::BFW_LSInfoBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild)
:  LtTBaseInfoBuilder(f)
{
	m_lsData = lsData;
	m_build = inBuild;
	
}

LtTBool BFW_LSInfoBuilder::Finish()
{

	if(1)
	{
		m_lsData->contrast = GetContrast();
		m_lsData->brightness = GetBrightness();
	}

#if 0

	fprintf( stderr, "Number of Elements: %d", GetNumElements());

	fprintf( stderr, "Number of Groups: %d", GetNumGroups() );

	fprintf( stderr, "Number of Shapes: %d", GetNumShapes() );

	fprintf( stderr, "Number of Sources: %d", GetNumSources() );

	fprintf( stderr, "Number of Materials: %d", GetNumMaterials() );

	fprintf( stderr, "Numbr of Collections: %d", GetNumCollections() );

	fprintf( stderr, "File Version: %d", GetFileVersion() );

	fprintf( stderr, "Ambient: %f", GetAmbient() );

	fprintf( stderr, "Length Units: %s", whichLengthUnit().GetBuffer(5) );

	fprintf( stderr, "Intensity Units: %s", whichIntensityUnit().GetBuffer(5) );
#endif

	return TRUE;
}

//-----------------------------------------------------------------------
//			Class TreeParameterBuilder
//
BFW_LSParameterBuilder::BFW_LSParameterBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild)
:  LtTBaseParameterBuilder(f)
{

	m_lsData = lsData;
	m_build = inBuild;
}



LtTBool BFW_LSParameterBuilder::Finish()
{
	LtTProcessFlag	processFlags;
	
	processFlags = GetFlags();
	
	if(processFlags & LT_PROCESS_DAYLIGHT)
	{
		m_lsData->flags |= LScFlag_Daylight;
	}
	
	if(processFlags & LT_PROCESS_EXTERIOR)
	{
		m_lsData->flags |= LScFlag_Exterior;
	}

	return TRUE;
}

//-----------------------------------------------------------------------
//			Class BFW_LSViewBuilder
//
BFW_LSViewBuilder::BFW_LSViewBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild)
:  LtTBaseViewBuilder(f)
{
	m_lsData = lsData;
	m_build = inBuild;
}

LtTBool BFW_LSViewBuilder::Finish()
{
	return TRUE;
}



//-----------------------------------------------------------------------
//			Class BFW_LSMaterialBuilder
//
BFW_LSMaterialBuilder::BFW_LSMaterialBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild)
:  LtTBaseMaterialBuilder(f)
{

	m_lsData = lsData;
	m_build = inBuild;
}

LtTBool BFW_LSMaterialBuilder::Finish()
{

	return TRUE;
}

//-----------------------------------------------------------------------
//			Class BFW_LSLayerBuilder
//
BFW_LSLayerBuilder::BFW_LSLayerBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild)
:  LtTBaseLayerBuilder(f)
{

	m_lsData = lsData;
	m_build = inBuild;
}

LtTBool BFW_LSLayerBuilder::Finish()
{

	return TRUE;
}


//-----------------------------------------------------------------------
//			Class BFW_LSTextureBuilder
//
BFW_LSTextureBuilder::BFW_LSTextureBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild)
:  LtTBaseTextureBuilder(f)
{

	m_lsData = lsData;
	m_build = inBuild;

}

LtTBool BFW_LSTextureBuilder::Finish()
{
	return TRUE;
}

//-----------------------------------------------------------------------
//			Class BFW_LSLampBuilder
//
BFW_LSLampBuilder::BFW_LSLampBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild)
:  LtTBaseLampBuilder(f)
{

	m_lsData = lsData;
	m_build = inBuild;
}

LtTBool BFW_LSLampBuilder::Finish()
{

	return TRUE;
}

//-----------------------------------------------------------------------
//			Class TreeMeshBuilder
//

BFW_LSMeshBuilder::BFW_LSMeshBuilder( LtTBuilderFactory *f, LStData* lsData, UUtBool inBuild)
:  LtTBaseMeshBuilder(f)
{

	m_lsData = lsData;
	m_build = inBuild;

}

BFW_LSMeshBuilder::~BFW_LSMeshBuilder()
{
}


M3tPoint3D lightscape_to_oni_point(const LtTPoint *inPoint)
{
	M3tPoint3D result;

	result.x = 10.f * inPoint->GetX();
	result.y = 10.f * inPoint->GetZ();
	result.z = -10.f * inPoint->GetY();

	return result;
}

LtTBool BFW_LSMeshBuilder::Finish()
{
	const LtTPoint*		points;
	UUtInt32			vertex_count;
	UUtInt32			face_count;
	const LtTPoint*		curPoint;
	const LtTRGBColor*	colors;
	
	LStPoint*			new_point;
	UUtInt32			curVertexIndex;
	//LStPatch*			curBFWPatch;
	LtTUnitVector		normal;

	
	if (gError) {
		return UUcFalse;
	}
	
	if (m_build)
	{						
		M3tVector3D patch_normal;
		float expected_plane_result;

		rmap.SetBrightness(m_lsData->brightness);
		rmap.SetContrast(m_lsData->contrast);
		rmap.SetDaylight(m_lsData->flags & LScFlag_Daylight);
		rmap.SetExterior(m_lsData->flags & LScFlag_Exterior);

		points = GetVertices();
		colors = GetIrradiances();
		vertex_count = GetVertexCount();
		face_count = GetFaceCount();
		GetPlane(normal);
						
		patch_normal.x = normal.GetX();
		patch_normal.y = normal.GetZ();
		patch_normal.z = -normal.GetY();
			
		curPoint = points;
		new_point = m_lsData->the_point_list + m_lsData->num_points;

		{
			M3tPoint3D a_point = lightscape_to_oni_point(curPoint);

			expected_plane_result = (patch_normal.x * a_point.x) + (patch_normal.y * a_point.y) + (patch_normal.z * a_point.z);
		}

		for(curVertexIndex = 0; curVertexIndex < vertex_count; curVertexIndex++)
		{
			new_point->location = lightscape_to_oni_point(curPoint);

			{
				float plane_result = (patch_normal.x * new_point->location.x) + (patch_normal.y * new_point->location.y) + (patch_normal.z * new_point->location.z) - expected_plane_result;
				UUmAssert(UUmFloat_LooseCompareZero(plane_result));
			}

			{
				LtTRGBColor			irradiance;
				float r, g, b;

				UUtUns32 integer_r;
				UUtUns32 integer_g;
				UUtUns32 integer_b;

				irradiance = GetIrradiances()[ curVertexIndex ];

				r = (float)(irradiance.GetR()) * 3.0f;
				g = (float)(irradiance.GetG()) * 3.0f;
				b = (float)(irradiance.GetB()) * 3.0f;
				
				r /= (float)M_PI;
				g /= (float)M_PI;
				b /= (float)M_PI;

				rmap.ToColor(r, g, b);

				integer_r = MUrUnsignedSmallFloat_To_Uns_Round(UUmPin(r * 255.f, 0, 255.f));
				integer_g = MUrUnsignedSmallFloat_To_Uns_Round(UUmPin(g * 255.f, 0, 255.f));
				integer_b = MUrUnsignedSmallFloat_To_Uns_Round(UUmPin(b * 255.f, 0, 255.f));

				new_point->shade = (integer_r) << 16 | (integer_g) << 8 | (integer_b << 0);
			}

			new_point->normal = patch_normal;
			new_point->used = 0;

			UUmMinMax(new_point->location.x, m_lsData->bbox.minPoint.x, m_lsData->bbox.maxPoint.x);
			UUmMinMax(new_point->location.y, m_lsData->bbox.minPoint.y, m_lsData->bbox.maxPoint.y);
			UUmMinMax(new_point->location.z, m_lsData->bbox.minPoint.z, m_lsData->bbox.maxPoint.z);

			new_point++;
			curPoint++;
		}
	}

	m_lsData->num_points += GetVertexCount();
	
	return TRUE;
}
