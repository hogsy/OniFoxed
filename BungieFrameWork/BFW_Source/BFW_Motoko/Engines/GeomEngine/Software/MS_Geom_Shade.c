/*
	FILE:	MS_Geom_Shade.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: May 21, 1997
	
	PURPOSE: Interface to the Motoko 3D engine
	
	Copyright 1997

*/

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_BitVector.h"

#include "MS_GC_Private.h"

#include "MS_Geom_Shade.h"

static UUtUns32
iComputeShade(
	M3tPoint3D*		inPoint,
	M3tVector3D*	inNormal,
	M3tPoint3D*		inCameraLocation,
	M3tTextureCoord	*outEnvMapCoord);

static UUtUns32
iComputeShade_Face(
	M3tVector3D*			inNormal,
	float					inBaseR,
	float					inBaseG,
	float					inBaseB);
	

void
MSrShade_Vertices_Gouraud(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	M3tVector3D*			inWorldVertexNormals,
	UUtUns32*				inResultScreenShades)
{
	UUtUns16				i;
	
	M3tPoint3D*				curPoint = inWorldPoints;
	M3tVector3D*			curNormal = inWorldVertexNormals;
	UUtUns32*				curScreenShade = inResultScreenShades;
	UUtBool					environmentMapping;
	M3tTextureCoord*		curEnvMapCoord = NULL;
	M3tPoint3D*				cameraLocation;
	
	cameraLocation = &MSgGeomContextPrivate->visCamera->cameraLocation;
	
	environmentMapping = (inGeometry->baseMap->flags & M3cTextureFlags_ReceivesEnvMap) != 0;
	
	MSgGeomContextPrivate->numDebugEnvMap = 0;

	UUmAssert(NULL != curPoint);
	
	if(0 || environmentMapping)
	{
		curEnvMapCoord = MSgGeomContextPrivate->envMapCoords;
	}
	
	if ((inGeometry->geometryFlags & M3cGeometryFlag_SelfIlluminent) || (inGeometry->baseMap->flags & M3cTextureFlags_SelfIlluminent))
	{
		for(i = 0; i < inGeometry->pointArray->numPoints; i++)
		{
			*curScreenShade++ = 0xFFFFFFF;
		}
	}
	else 
	{		
		for(i = 0; i < inGeometry->pointArray->numPoints; i++)
		{
			*curScreenShade = iComputeShade(curPoint, curNormal, cameraLocation, curEnvMapCoord);
			
			curPoint++;
			curNormal++;
			curScreenShade++;
			if(curEnvMapCoord != NULL) curEnvMapCoord++;
		}
	}
}

void
MSrShade_Vertices_GouraudActive(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	M3tVector3D*			inWorldVertexNormals,
	UUtUns32*				inResultScreenShades,
	UUtUns32*				inActiveVerticesBV)
{
	UUtUns32				i;
	
	M3tPoint3D*				curPoint = inWorldPoints;
	M3tVector3D*			curNormal = inWorldVertexNormals;
	UUtUns32*				curScreenShade = inResultScreenShades;
	UUtBool					environmentMapping;
	M3tTextureCoord*		curEnvMapCoord = NULL;
	M3tPoint3D*				cameraLocation;
        
	UUmAssertReadPtr(MSgGeomContextPrivate->visCamera, sizeof(*MSgGeomContextPrivate->visCamera));
	UUmAssertReadPtr(MSgGeomContextPrivate, sizeof(*MSgGeomContextPrivate));

	cameraLocation = &MSgGeomContextPrivate->visCamera->cameraLocation;
	
	
	environmentMapping = (inGeometry->baseMap->flags & M3cTextureFlags_ReceivesEnvMap) != 0;
	
	if(0 || environmentMapping)
	{
		curEnvMapCoord = MSgGeomContextPrivate->envMapCoords;
	}
	
	UUmAssert(NULL != curPoint);
	
	if ((inGeometry->geometryFlags & M3cGeometryFlag_SelfIlluminent) || (inGeometry->baseMap->flags & M3cTextureFlags_SelfIlluminent))
	{
		for(i = 0; i < inGeometry->pointArray->numPoints; i++)
		{
			*curScreenShade++ = 0xFFFFFFF;
		}
	}
	else if (inGeometry->baseMap->flags & (M3cTextureFlags_MagicShield_Invis | M3cTextureFlags_MagicShield_Blue | M3cTextureFlags_MagicShield_Daodan)) {

#define cShieldEffect_MaxVertices		64

		const float cShieldEffect_Speed_Base			= 0.01f;
		const float cShieldEffect_Speed_Delta			= 0.005f;
		const UUtUns8 cShieldEffect_Shield_R			= 15;
		const UUtUns8 cShieldEffect_Shield_G			= 127;
		const UUtUns8 cShieldEffect_Shield_B			= 255;
		static UUtUns8 cShieldEffect_Daodan_R			= 255;
		static UUtUns8 cShieldEffect_Daodan_G			= 32;
		static UUtUns8 cShieldEffect_Daodan_B			= 80;
		const UUtUns8 cShieldEffect_Invis_R				= 24;
		const UUtUns8 cShieldEffect_Invis_B				= 64;
		const UUtUns8 cShieldEffect_Invis_B_Subtract	= 48;
		const float cShieldEffect_Timescale_R			= 0.9f;
		const float cShieldEffect_Timescale_G			= 1.0f;
		const float cShieldEffect_Timescale_B			= 1.1f;
		static float cShieldEffect_DaodanTimescale_R		= 1.1f;
		static float cShieldEffect_DaodanTimescale_G		= 1.0f;
		static float cShieldEffect_DaodanTimescale_B		= 0.9f;
		const float cShieldEffect_InvisTimescale_R		= 1.0f;
		const float cShieldEffect_InvisTimescale_B		= 0.3f;
		const float cShieldEffect_Timescale_V			= 0.15f;

		static UUtBool computed_data = UUcFalse;
		static float vertex_speeds[cShieldEffect_MaxVertices];
		static UUtUns8 vary_table[256];

		UUtUns8 red_value, green_value, blue_value, red_max, green_max, blue_max;
		UUtUns32 time = M3rDraw_State_GetInt(M3cDrawStateIntType_Time);
		float red_time, green_time, blue_time, speed;

		if (!computed_data) {
			for (i = 0; i < cShieldEffect_MaxVertices; i++) {
				vertex_speeds[i] = cShieldEffect_Speed_Base + cShieldEffect_Speed_Delta * (float) sin((float) i);
			}
			for (i = 0; i < 256; i++) {
				vary_table[i] = (UUtUns8) MUrUnsignedSmallFloat_To_Uns_Round(255 * 0.5f * (1.0f + (float) sin((i * (float) M3c2Pi) * 0.00390625f /* S.S. / 256*/)));
			}

			computed_data = UUcTrue;
		}

		if (inGeometry->baseMap->flags & M3cTextureFlags_MagicShield_Invis) {
			red_max		= cShieldEffect_Invis_R;
			blue_max	= cShieldEffect_Invis_B;

			for(i = 0; i < inGeometry->pointArray->numPoints; i++)
			{
				speed = vertex_speeds[i % cShieldEffect_MaxVertices];

				// red and green channels are linked together for the invisibility effect
				red_time	= cShieldEffect_InvisTimescale_R * speed * time + cShieldEffect_Timescale_V * i;
				blue_time	= cShieldEffect_InvisTimescale_B * speed * time + cShieldEffect_Timescale_V * i;

				red_value	= (red_max * vary_table[MUrUnsignedSmallFloat_To_Uns_Round(red_time * 256) % 256]) >> 8;
				blue_value	= (blue_max * vary_table[MUrUnsignedSmallFloat_To_Uns_Round(blue_time * 256) % 256]) >> 8;
				if (blue_value > cShieldEffect_Invis_B_Subtract) {
					blue_value = red_value + (blue_value - cShieldEffect_Invis_B_Subtract);
				} else {
					blue_value = red_value;
				}

				*curScreenShade++ = (((UUtUns32) red_value) << 16) | (((UUtUns32) red_value) << 8) | ((UUtUns32) blue_value);
			}
		} else if (inGeometry->baseMap->flags & M3cTextureFlags_MagicShield_Blue) {
			red_max		= cShieldEffect_Shield_R;
			green_max	= cShieldEffect_Shield_G;
			blue_max	= cShieldEffect_Shield_B;

			for(i = 0; i < inGeometry->pointArray->numPoints; i++)
			{
				speed = vertex_speeds[i % cShieldEffect_MaxVertices];

				red_time	= cShieldEffect_Timescale_R * speed * time + cShieldEffect_Timescale_V * i;
				green_time	= cShieldEffect_Timescale_G * speed * time + cShieldEffect_Timescale_V * i;
				blue_time	= cShieldEffect_Timescale_B * speed * time + cShieldEffect_Timescale_V * i;

				red_value	= (red_max * vary_table[MUrUnsignedSmallFloat_To_Uns_Round(red_time * 256) % 256]) >> 8;
				green_value = (green_max * vary_table[MUrUnsignedSmallFloat_To_Uns_Round(green_time * 256) % 256]) >> 8;
				blue_value	= (blue_max * vary_table[MUrUnsignedSmallFloat_To_Uns_Round(blue_time * 256) % 256]) >> 8;

				*curScreenShade++ = (((UUtUns32) red_value) << 16) | (((UUtUns32) green_value) << 8) | ((UUtUns32) blue_value);
			}
		} else {
			red_max		= cShieldEffect_Daodan_R;
			green_max	= cShieldEffect_Daodan_G;
			blue_max	= cShieldEffect_Daodan_B;

			for(i = 0; i < inGeometry->pointArray->numPoints; i++)
			{
				speed = vertex_speeds[i % cShieldEffect_MaxVertices];

				red_time	= cShieldEffect_DaodanTimescale_R * speed * time + cShieldEffect_Timescale_V * i;
				green_time	= cShieldEffect_DaodanTimescale_G * speed * time + cShieldEffect_Timescale_V * i;
				blue_time	= cShieldEffect_DaodanTimescale_B * speed * time + cShieldEffect_Timescale_V * i;

				red_value	= (red_max * vary_table[MUrUnsignedSmallFloat_To_Uns_Round(red_time * 256) % 256]) >> 8;
				green_value = (green_max * vary_table[MUrUnsignedSmallFloat_To_Uns_Round(green_time * 256) % 256]) >> 8;
				blue_value	= (blue_max * vary_table[MUrUnsignedSmallFloat_To_Uns_Round(blue_time * 256) % 256]) >> 8;

				*curScreenShade++ = (((UUtUns32) red_value) << 16) | (((UUtUns32) green_value) << 8) | ((UUtUns32) blue_value);
			}
		}
	}
	else 
	{
		UUtUns32 numPoints = inGeometry->pointArray->numPoints;

		UUmBitVector_Loop_Begin(i, numPoints, inActiveVerticesBV)
		{
			UUmBitVector_Loop_Test
			{
				*curScreenShade = iComputeShade(curPoint, curNormal, cameraLocation, curEnvMapCoord);
			}
			
			curPoint++;
			curNormal++;
			curScreenShade++;
			if(curEnvMapCoord != NULL) curEnvMapCoord++;
		}
		UUmBitVector_Loop_End
	}
}

float	gRTable[32] =
{
	1.0f / 32.0f,
	1.0f / 4.0f,
	1.0f / 10.0f,
	1.0f / 29.0f,
	1.0f / 21.0f,
	1.0f / 19.0f,
	1.0f / 25.0f,
	1.0f / 24.0f,
	1.0f / 22.0f,
	1.0f / 27.0f,
	1.0f / 19.0f,
	1.0f / 31.0f,
	1.0f / 18.0f,
	1.0f / 26.0f,
	1.0f / 3.0f,
	1.0f / 17.0f,
	1.0f / 14.0f,
	1.0f / 12.0f,
	1.0f / 30.0f,
	1.0f / 7.0f,
	1.0f / 11.0f,
	1.0f / 15.0f,
	1.0f / 28.0f,
	1.0f / 6.0f,
	1.0f / 23.0f,
	1.0f / 20.0f,
	1.0f / 13.0f,
	1.0f / 19.0f,
	1.0f / 16.0f,
	1.0f / 14.0f,
	1.0f / 17.0f,
	1.0f / 16.0f,
};

float	gGTable[32] =
{
	1.0f / 14.0f,
	1.0f / 21.0f,
	1.0f / 8.0f,
	1.0f / 4.0f,
	1.0f / 2.0f,
	1.0f / 1.0f,
	1.0f / 30.0f,
	1.0f / 7.0f,
	1.0f / 11.0f,
	1.0f / 15.0f,
	1.0f / 28.0f,
	1.0f / 10.0f,
	1.0f / 6.0f,
	1.0f / 5.0f,
	1.0f / 16.0f,
	1.0f / 32.0f,
	1.0f / 26.0f,
	1.0f / 9.0f,
	1.0f / 25.0f,
	1.0f / 24.0f,
	1.0f / 12.0f,
	1.0f / 23.0f,
	1.0f / 22.0f,
	1.0f / 20.0f,
	1.0f / 13.0f,
	1.0f / 27.0f,
	1.0f / 19.0f,
	1.0f / 31.0f,
	1.0f / 18.0f,
	1.0f / 3.0f,
	1.0f / 17.0f,
	1.0f / 29.0f,
};

float	gBTable[32] =
{
	1.0f / 28.0f,
	1.0f / 5.0f,
	1.0f / 16.0f,
	1.0f / 10.0f,
	1.0f / 29.0f,
	1.0f / 32.0f,
	1.0f / 26.0f,
	1.0f / 9.0f,
	1.0f / 25.0f,
	1.0f / 24.0f,
	1.0f / 22.0f,
	1.0f / 20.0f,
	1.0f / 6.0f,
	1.0f / 13.0f,
	1.0f / 14.0f,
	1.0f / 12.0f,
	1.0f / 30.0f,
	1.0f / 7.0f,
	1.0f / 11.0f,
	1.0f / 15.0f,
	1.0f / 23.0f,
	1.0f / 27.0f,
	1.0f / 19.0f,
	1.0f / 31.0f,
	1.0f / 18.0f,
	1.0f / 3.0f,
	1.0f / 17.0f,
	1.0f / 21.0f,
	1.0f / 8.0f,
	1.0f / 4.0f,
	1.0f / 2.0f,
	1.0f / 1.0f
};


void
MSrShade_Faces_Gouraud(
	M3tGeometry*			inGeometry,
	M3tPoint3D*				inWorldPoints,
	UUtUns32				inNumFaces,
	M3tVector3D*			inWorldFaceNormals,
	UUtUns32*				inResultFaceScreenShades)
{
	UUtUns32				i;
	
	M3tVector3D*			curFaceNormal;
	UUtUns32*				curFaceScreenShade;	
	UUtUns16	count = 0;
	UUtUns16	stripIndexCount = 2;
	
	curFaceNormal = inWorldFaceNormals;
	curFaceScreenShade = inResultFaceScreenShades;

	if ((inGeometry->geometryFlags & M3cGeometryFlag_SelfIlluminent) || (inGeometry->baseMap->flags & M3cTextureFlags_SelfIlluminent))
	{
		for(i = 0; i < inNumFaces; i++)
		{
			*curFaceScreenShade++ = 0xFFFFFFF;
		}
	}
	else 
	{		
		for(i = 0; i < inNumFaces; i++)
		{
			if(inGeometry->triStripArray->indices[stripIndexCount++] & 0x80000000)
			{
				count++;
				if(count >= 32) count = 0;
				stripIndexCount += 2;
			}
		
			*curFaceScreenShade =
				iComputeShade_Face(
					curFaceNormal,
					gRTable[count],
					gGTable[count],
					gBTable[count]);
			
			curFaceNormal++;
			curFaceScreenShade++;
		}
	}
}

#if 0
static void render_list_compute_reflection_texture_uvs(
	struct view_data *view,
	struct render_list *list,
	short map_index)
{
	// set texture uvs of map
	{
		struct uv_pair *texture_uvs= list->maps[map_index].texture_uvs;
		real_vector3d *world_normals= list->world_normals;
		real_point3d *world_points= list->world_points;

		short vertex_count= list->vertex_count;

		while (vertex_count-->0)
		{
			// if this vertex is extant
			{
				real_vector3d incident;
				real_vector3d reflection;

				real one_over_magnitude;

				vector_from_points3d(&view->position, world_points, &incident);
				reflect_vector3d(&incident, world_normals, &reflection);

				one_over_magnitude= 0.5f*reciprocal_square_root(reflection.i*reflection.i + reflection.j*reflection.j + (1.f + reflection.k)*(1.f + reflection.k));
				texture_uvs->u= 0.5f + reflection.i*one_over_magnitude;
				texture_uvs->v= 0.5f + reflection.j*one_over_magnitude;
			}

			world_normals+= 1;
			world_points+= 1;
			texture_uvs+= 1;
		}
	}

	// fix triangles which have (u,v) coordinates which wrap in the "wrong" direction
	{
		struct uv_pair *texture_uvs= list->maps[map_index].texture_uvs;
		short *vertex_indices= list->triangle_strip_vertex_indices;
		short triangle_strip_count= list->triangle_strip_count;

		while (triangle_strip_count-->0)
		{
			short vertex_count= *vertex_indices++;

			real u0;
			real u1= texture_uvs[*vertex_indices++].u;

			while (--vertex_count>0)
			{
				real du;

				u0= u1, u1= texture_uvs[*vertex_indices++].u;
				du= u1-u0;

				if (du>0.5f)
				{
					// u0 near zero degrees, u1 near 360 degrees: adjust u1 down
					texture_uvs[vertex_indices[-2]].u+= 0.5f;
				}
				else if (du<-0.5f)
				{
					// u1 near zero degrees, u0 near 360 degrees: adjust u1 up
					texture_uvs[vertex_indices[-1]].u+= 0.5f;
				}
			}
		}
	}

	return;
}
#endif

static M3tVector3D*
reflect_vector3d(
	const M3tVector3D *incident,
	const M3tVector3D *normal,
	M3tVector3D *reflection)
{
	float incident_dot_normal= MUrVector_DotProduct(incident, normal);
	float twice_incident_dot_normal= incident_dot_normal + incident_dot_normal;

	reflection->x= incident->x - twice_incident_dot_normal*normal->x;
	reflection->y= incident->y - twice_incident_dot_normal*normal->y;
	reflection->z= incident->z - twice_incident_dot_normal*normal->z;

	return reflection;
}

static UUtUns32
iComputeShade(
	M3tPoint3D*				inPoint,
	M3tVector3D*			inNormal,
	M3tPoint3D*				inCameraLocation,
	M3tTextureCoord*		inEnvMapCoord)
{
	UUtUns16				itr;

	float					curR;
	float					curG;
	float					curB;

	//float					attenuation;

	UUtUns8					intR, intG, intB;
	float					dotProduct;
	float					lightX,lightY,lightZ;
	//float					inverseDistance, distanceSquared;
	MStLight_Directional*	curDirectionalLight;
	//M3tLight_Point*		curPointLight;
	
	UUtUns32				outShade;
		float normalX, normalY, normalZ;
	
	normalX = inNormal->x;
	normalY = inNormal->y;
	normalZ = inNormal->z;
	
	curR = MSgGeomContextPrivate->light_Ambient.color.r;
	curG = MSgGeomContextPrivate->light_Ambient.color.g;
	curB = MSgGeomContextPrivate->light_Ambient.color.b;
	
	// compute reflection vector if needed
	if(inEnvMapCoord != NULL)
	{
		M3tVector3D incident;
		M3tVector3D reflection;

		float one_over_magnitude;

		// incident = world_points - view->position 
		MUmVector_Subtract(incident, *inPoint, *inCameraLocation);
		reflect_vector3d(&incident, inNormal, &reflection);

		one_over_magnitude= 0.5f*MUrOneOverSqrt(reflection.x*reflection.x + reflection.y*reflection.y + (1.f + reflection.z)*(1.f + reflection.z));
		inEnvMapCoord->u= 0.5f + reflection.x*one_over_magnitude;
		inEnvMapCoord->v= 0.5f + reflection.y*one_over_magnitude;
	}
	
	// directional
	for(itr = 0, curDirectionalLight = MSgGeomContextPrivate->light_DirectionalList;
		itr < MSgGeomContextPrivate->light_NumDirectionalLights;
		itr++, curDirectionalLight++)
	{
		//UUmAssert((curDirectionalLight->color.r >= 0) && (curDirectionalLight->color.r <= 1));
		//UUmAssert((curDirectionalLight->color.g >= 0) && (curDirectionalLight->color.g <= 1));
		//UUmAssert((curDirectionalLight->color.b >= 0) && (curDirectionalLight->color.b <= 1));

		lightX = -curDirectionalLight->normalizedDirection.x;
		lightY = -curDirectionalLight->normalizedDirection.y;
		lightZ = -curDirectionalLight->normalizedDirection.z;

		dotProduct = normalX * lightX + normalY * lightY + normalZ * lightZ;
		
		if (dotProduct > 0.0f)
		{
			curR += dotProduct * curDirectionalLight->color.r;
			curG += dotProduct * curDirectionalLight->color.g;
			curB += dotProduct * curDirectionalLight->color.b;
		}
	}
	
	// turn color into a shade
	UUmAssert(curR >= 0.f);
	UUmAssert(curG >= 0.f);
	UUmAssert(curB >= 0.f);

	intR = (UUtUns8) MUrUnsignedSmallFloat_To_Uns_Round(UUmMin(curR * 255.0f, 255.0f));
	intG = (UUtUns8) MUrUnsignedSmallFloat_To_Uns_Round(UUmMin(curG * 255.0f, 255.0f));
	intB = (UUtUns8) MUrUnsignedSmallFloat_To_Uns_Round(UUmMin(curB * 255.0f, 255.0f));

	outShade = (intR << 16) | (intG << 8) | (intB);

	return outShade;
}

static UUtUns32
iComputeShade_Face(
	M3tVector3D*			inNormal,
	float					inBaseR,
	float					inBaseG,
	float					inBaseB)
{
	UUtUns16				itr;

	float					curR;
	float					curG;
	float					curB;

	//float					attenuation;

	UUtUns8					intR, intG, intB;
	float					dotProduct;
	float					lightX,lightY,lightZ;
	//float					inverseDistance, distanceSquared;
	MStLight_Directional*	curDirectionalLight;
	//M3tLight_Point*		curPointLight;
	
	UUtUns32				outShade;
	float					normalX, normalY, normalZ;
	
	normalX = inNormal->x;
	normalY = inNormal->y;
	normalZ = inNormal->z;
	
	curR = MSgGeomContextPrivate->light_Ambient.color.r;
	curG = MSgGeomContextPrivate->light_Ambient.color.g;
	curB = MSgGeomContextPrivate->light_Ambient.color.b;
	
	
	// directional
	for(itr = 0, curDirectionalLight = MSgGeomContextPrivate->light_DirectionalList;
		itr < MSgGeomContextPrivate->light_NumDirectionalLights;
		itr++, curDirectionalLight++)
	{
		//UUmAssert((curDirectionalLight->color.r >= 0) && (curDirectionalLight->color.r <= 1));
		//UUmAssert((curDirectionalLight->color.g >= 0) && (curDirectionalLight->color.g <= 1));
		//UUmAssert((curDirectionalLight->color.b >= 0) && (curDirectionalLight->color.b <= 1));

		lightX = -curDirectionalLight->normalizedDirection.x;
		lightY = -curDirectionalLight->normalizedDirection.y;
		lightZ = -curDirectionalLight->normalizedDirection.z;

		dotProduct = normalX * lightX + normalY * lightY + normalZ * lightZ;
		
		if (dotProduct > 0.0f)
		{
			curR += dotProduct * curDirectionalLight->color.r;
			curG += dotProduct * curDirectionalLight->color.g;
			curB += dotProduct * curDirectionalLight->color.b;
		}
	}

	#if 0
	// point
	for(itr = 0, curPointLight = inGeomPrivate->light_PointList;
		itr < inGeomPrivate->light_NumPointLights;
		itr++, curPointLight++)
	{
		UUmAssert((curPointLight->color.r >= 0) && (curPointLight->color.r <= 1));
		UUmAssert((curPointLight->color.g >= 0) && (curPointLight->color.g <= 1));
		UUmAssert((curPointLight->color.b >= 0) && (curPointLight->color.b <= 1));

		lightX = curPointLight->location.x - pointX;
		lightY = curPointLight->location.y - pointY;
		lightZ = curPointLight->location.z - pointZ;
		
		distanceSquared = UUmSQR(lightX) + UUmSQR(lightY) + UUmSQR(lightZ);
		if (0 == distanceSquared) continue;

		inverseDistance = MUrOneOverSqrt(distanceSquared);
		
		lightX *= inverseDistance;
		lightY *= inverseDistance;
		lightZ *= inverseDistance;
		
		dotProduct = normalX * lightX + normalY * lightY + normalZ * lightZ;

		if (dotProduct > 0.0f) 
		{
			attenuation = curPointLight->a * inverseDistance * inverseDistance;
			attenuation += curPointLight->b * inverseDistance;
			attenuation += curPointLight->c;
			
			dotProduct *= attenuation;
			curR += dotProduct * curPointLight->color.r;
			curG += dotProduct * curPointLight->color.g;
			curB += dotProduct * curPointLight->color.b;
		}
	}
	#endif
	
	// turn color into a shade
	UUmAssert(curR >= 0.f);
	UUmAssert(curG >= 0.f);
	UUmAssert(curB >= 0.f);
	
	curR = inBaseR;
	curG = inBaseG;
	curB = inBaseB;
	
	intR = (UUtUns8) MUrUnsignedSmallFloat_To_Uns_Round(UUmMin(curR * 255.0f, 255.0f));
	intG = (UUtUns8) MUrUnsignedSmallFloat_To_Uns_Round(UUmMin(curG * 255.0f, 255.0f));
	intB = (UUtUns8) MUrUnsignedSmallFloat_To_Uns_Round(UUmMin(curB * 255.0f, 255.0f));

	outShade = (intR << 16) | (intG << 8) | (intB);

	return outShade;
}

