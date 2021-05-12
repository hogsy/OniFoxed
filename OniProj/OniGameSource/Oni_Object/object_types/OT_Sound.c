// ======================================================================
// OT_Sounds.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ScriptLang.h"
#include "BFW_TextSystem.h"

#include "Oni_Object.h"
#include "Oni_Object_Private.h"
#include "Oni_BinaryData.h"
#include "Oni_Sound2.h"

#include <ctype.h>
#include "EulerAngles.h"

// ======================================================================
// defines
// ======================================================================
#define OBJcNumPointsInRing			16

// ======================================================================
// globals
// ======================================================================
static UUtBool				OBJgSound_DrawSounds;
static UUtBool				OBJgSound_AlwaysDraw;
static UUtBool				OBJgSound_AlwaysDrawSpheres;
static UUtBool				OBJgSound_AlwaysDrawVolumes;

#if TOOL_VERSION
// sound diamond points
static M3tPoint3D			*OBJgSound_Diamond;
#endif

static UUtError OBJiSound_SetOSD(OBJtObject *inObject, const OBJtOSD_All *inOSD);
static void OBJiSound_UpdatePosition(OBJtObject *inObject);

#if TOOL_VERSION
// text name drawing
static UUtBool				OBJgSound_DrawInitialized = UUcFalse;
static UUtRect				OBJgSound_TextureBounds;
static M3tTextureMap		*OBJgSound_Texture;
static IMtPoint2D			OBJgSound_Dest;
static IMtPixel				OBJgSound_WhiteColor;
static UUtInt16				OBJgSound_TextureWidth;
static UUtInt16				OBJgSound_TextureHeight;
static float				OBJgSound_WidthRatio;
static float				OBJgSound_HeightRatio;
static TStFontFamily		*OBJgSound_FontFamily;
#endif

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiSound_CalcBoundingVolume(
	OBJtObject				*inObject,
	OBJtOSD_Sound			*inSoundOSD)
{
	M3tMatrix4x3			matrix;
	M3tBoundingVolume		*volume;
	M3tPoint3D				*points;
	M3tBoundingBox			bbox;
	UUtInt32				itr;
	
	volume = &inSoundOSD->u.bvolume.bvolume;
	points = volume->worldPoints;
	
	// calculate the offset matrix
	MUrMatrix_Identity(&matrix);
	MUrMatrixStack_Translate(&matrix, &inObject->position);
	
	// expand into bounding box, then copy
	M3rMinMaxBBox_To_BBox(&inSoundOSD->u.bvolume.bbox, &bbox);
	UUrMemory_MoveFast(&bbox, points, sizeof(M3tPoint3D) * M3cNumBoundingPoints);
	MUrMatrix_MultiplyPoints(8, &matrix, points, points);

	// compute faces
	UUrMemory_MoveFast(M3gBBox_QuadList, volume->faces, sizeof(M3tQuad) * M3cNumBoundingFaces);

	// compute normals	
	for (itr = 0; itr < M3cNumBoundingFaces; itr++)
	{
		MUrVector_NormalFromPoints(
			volume->worldPoints + volume->faces[itr].indices[0],
			volume->worldPoints + volume->faces[itr].indices[1],
			volume->worldPoints + volume->faces[itr].indices[2],
			volume->normals + itr);
	}

	// compute planes
	for (itr = 0; itr < M3cNumBoundingFaces; itr++)
	{
		M3tVector3D normal;
		M3tPlaneEquation *plane;
		M3tPoint3D *point;

		normal = volume->normals[itr];
	
		point = volume->worldPoints + volume->faces[itr].indices[0];
		plane = volume->curPlanes + itr;

		plane->a = normal.x;
		plane->b = normal.y;
		plane->c = normal.z;
		plane->d = -(normal.x*point->x + normal.y*point->y + normal.z*point->z);

		volume->curProjections[itr] = (UUtUns16) 
			CLrQuad_FindProjection(volume->worldPoints,	volume->faces + itr);

		// degenerate quads should be culled at import time
		UUmAssert(volume->curProjections[itr] != CLcProjection_Unknown);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiSound_DrawSphere(
	float					inRadius,
	M3tPoint3D				*inDest,
	IMtShade				inShade)
{
	// one times cache line size should be plenty but why risk it
	UUtUns8					block_XZ[(sizeof(M3tPoint3D) * (OBJcNumPointsInRing + 1)) + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8					block_XY[(sizeof(M3tPoint3D) * (OBJcNumPointsInRing + 1)) + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8					block_YZ[(sizeof(M3tPoint3D) * (OBJcNumPointsInRing + 1)) + (2 * UUcProcessor_CacheLineSize)];
	M3tPoint3D				*ring_XZ = UUrAlignMemory(block_XZ);
	M3tPoint3D				*ring_XY = UUrAlignMemory(block_XY);
	M3tPoint3D				*ring_YZ = UUrAlignMemory(block_YZ);
	UUtUns32				itr;
	
	for(itr = 0; itr < OBJcNumPointsInRing; itr++)
	{
		float		theta;
		float		cos_theta_radius;
		float		sin_theta_radius;
		
		theta = M3c2Pi * (((float) itr) / OBJcNumPointsInRing);
		cos_theta_radius = MUrCos(theta) * inRadius;
		sin_theta_radius = MUrSin(theta) * inRadius;
		
		ring_XZ[itr].x = cos_theta_radius + inDest->x;
		ring_XZ[itr].y = inDest->y;
		ring_XZ[itr].z = sin_theta_radius + inDest->z;
		
		ring_XY[itr].x = cos_theta_radius + inDest->x;
		ring_XY[itr].y = sin_theta_radius + inDest->y;
		ring_XY[itr].z = inDest->z;

		ring_YZ[itr].x = inDest->x;
		ring_YZ[itr].y = cos_theta_radius + inDest->y;
		ring_YZ[itr].z = sin_theta_radius + inDest->z;
	}

	// close the rings
	ring_XZ[OBJcNumPointsInRing] = ring_XZ[0];
	ring_XY[OBJcNumPointsInRing] = ring_XY[0];
	ring_YZ[OBJcNumPointsInRing] = ring_YZ[0];
	
	// draw the rings
	M3rGeometry_LineDraw((OBJcNumPointsInRing + 1), ring_XZ, inShade);
	M3rGeometry_LineDraw((OBJcNumPointsInRing + 1), ring_XY, inShade);
	M3rGeometry_LineDraw((OBJcNumPointsInRing + 1), ring_YZ, inShade);
}

#if TOOL_VERSION
// ----------------------------------------------------------------------
UUtError
OBJrSound_DrawInitialize(
	void)
{
	if (!OBJgSound_DrawInitialized) {
		IMtPixelType			textureFormat;
		
		// create the diamond
		OBJgSound_Diamond = (M3tPoint3D*)UUrMemory_Block_New(sizeof(M3tPoint3D) * 13);
		UUmError_ReturnOnNull(OBJgSound_Diamond);
		
		MUmVector_Set(OBJgSound_Diamond[0], 0.0f, 1.0f, 0.0f);
		MUmVector_Set(OBJgSound_Diamond[1], -1.0f, 0.0f, 1.0f);
		MUmVector_Set(OBJgSound_Diamond[2], 1.0f, 0.0f, 1.0f);
		MUmVector_Set(OBJgSound_Diamond[3], 0.0f, -1.0f, 0.0f);
		MUmVector_Set(OBJgSound_Diamond[4], -1.0f, 0.0f, 1.0f);
		MUmVector_Set(OBJgSound_Diamond[5], -1.0f, 0.0f, -1.0f);
		MUmVector_Set(OBJgSound_Diamond[6], 0.0f, 1.0f, 0.0f);
		MUmVector_Set(OBJgSound_Diamond[7], 1.0f, 0.0f, -1.0f);
		MUmVector_Set(OBJgSound_Diamond[8], -1.0f, 0.0f, -1.0f);
		MUmVector_Set(OBJgSound_Diamond[9],	0.0f, -1.0f, 0.0f);
		MUmVector_Set(OBJgSound_Diamond[10], 1.0f, 0.0f, -1.0f);
		MUmVector_Set(OBJgSound_Diamond[11], 1.0f, 0.0f, 1.0f);
		MUmVector_Set(OBJgSound_Diamond[12], 0.0f, 1.0f, 0.0f);
			
		// create the name texture
		M3rDrawEngine_FindGrayscalePixelType(&textureFormat);
		OBJgSound_WhiteColor = IMrPixel_FromShade(textureFormat, IMcShade_White);
		
		TSrFontFamily_Get(TScFontFamily_Default, &OBJgSound_FontFamily);
		DCrText_SetFontFamily(OBJgSound_FontFamily);
		DCrText_SetSize(TScFontSize_Default);
		DCrText_SetStyle(TScFontStyle_Default);
		
		OBJgSound_Dest.x = 2;
		OBJgSound_Dest.y = DCrText_GetLeadingHeight() + DCrText_GetAscendingHeight();
		
		DCrText_GetStringRect("01234567890123456789012345 p0.00 v0.00", &OBJgSound_TextureBounds);
		
		OBJgSound_TextureWidth = OBJgSound_TextureBounds.right;
		OBJgSound_TextureHeight = OBJgSound_TextureBounds.bottom;
		OBJgSound_WidthRatio = (float)OBJgSound_TextureWidth / (float)OBJgSound_TextureHeight;
		OBJgSound_HeightRatio = (float)OBJgSound_TextureHeight / (float)OBJgSound_TextureWidth;
		
		M3rTextureMap_New(
			OBJgSound_TextureWidth,
			OBJgSound_TextureHeight,
			textureFormat,
			M3cTexture_AllocMemory,
			M3cTextureFlags_None,
			"flag texture",
			&OBJgSound_Texture);

		OBJgSound_DrawInitialized = UUcTrue;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiSound_DrawName(
	OBJtObject				*inObject,
	OBJtOSD_Sound			*inSoundOSD,
	M3tPoint3D				*inLocation)
{
	char					name[128];
	
	if (OBJgSound_Texture == NULL) { return; }
	
	// erase the texture and set the text contexts shade
	M3rTextureMap_Fill(OBJgSound_Texture, OBJgSound_WhiteColor, NULL);
	
	// draw the string to the texture
	DCrText_SetFontFamily(OBJgSound_FontFamily);
	DCrText_SetSize(TScFontSize_Default);
	DCrText_SetStyle(TScFontStyle_Default);
	DCrText_SetShade(IMcShade_Black);
	DCrText_SetFormat(TSc_HCenter | TSc_VCenter);
	
	sprintf(name, "%s p%1.2f v%1.2f", inSoundOSD->ambient_name, inSoundOSD->pitch, inSoundOSD->volume);
	DCrText_DrawString(OBJgSound_Texture, name, &OBJgSound_TextureBounds, &OBJgSound_Dest);
	
	M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, M3cGeomState_SpriteMode_Normal);
	M3rGeom_State_Commit();
	
	// draw the sprite
	M3rSimpleSprite_Draw(
		OBJgSound_Texture,
		inLocation,
		OBJgSound_WidthRatio,
		1.0f,
		IMcShade_White,
		M3cMaxAlpha);
}

// ----------------------------------------------------------------------
void
OBJrSound_DrawTerminate(
	void)
{
	if (OBJgSound_DrawInitialized) {
		if (OBJgSound_Diamond != NULL) {
			UUrMemory_Block_Delete(OBJgSound_Diamond);
			OBJgSound_Diamond = NULL;
		}

		OBJgSound_DrawInitialized = UUcFalse;
	}
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiSound_Delete(
	OBJtObject				*inObject)
{
	// nothings to do here
}

// ----------------------------------------------------------------------
static void
OBJiSound_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
#if TOOL_VERSION
	UUtError				error;

	error = OBJrSound_DrawInitialize();
	if (error == UUcError_None) {
		OBJtOSD_Sound			*sound_osd;
		M3tPoint3D				dest;
		M3tPoint3D				camera_location;
			
		// get a pointer to the object osd
		sound_osd = (OBJtOSD_Sound*)inObject->object_data;
		
		// setup the matrix stack
		M3rMatrixStack_Push();
		M3rMatrixStack_ApplyTranslate(inObject->position);
		M3rGeom_State_Commit();
		
		// draw the diamond
		M3rGeometry_LineDraw(13, OBJgSound_Diamond, IMcShade_Yellow);
		
		MUmVector_Set(dest, 0.0f, 0.0f, 0.0f);

		// draw the appropriate type
		switch (sound_osd->type)
		{
			case OBJcSoundType_Spheres:
				if ((OBJgSound_AlwaysDraw == UUcTrue) ||
					(OBJgSound_AlwaysDrawSpheres == UUcTrue) ||
					((inDrawFlags & OBJcDrawFlag_Selected) != 0))
				{
					// draw the Max Volume Sphere
					MUmVector_Set(dest, 0.0f, 0.0f, 0.0f);
					OBJiSound_DrawSphere(
						sound_osd->u.spheres.max_volume_distance * UUmFeet(1),
						&dest,
						IMcShade_Red);
					
					// draw the Min Volume Sphere
					MUmVector_Set(dest, 0.0f, 0.0f, 0.0f);
					OBJiSound_DrawSphere(
						sound_osd->u.spheres.min_volume_distance * UUmFeet(1),
						&dest,
						IMcShade_Green);
				}
			break;
			
			case OBJcSoundType_BVolume:
				if ((OBJgSound_AlwaysDraw == UUcTrue) ||
					(OBJgSound_AlwaysDrawVolumes == UUcTrue) ||
					((inDrawFlags & OBJcDrawFlag_Selected) != 0))
				{
					M3tBoundingBox			bBox;
					IMtShade				shade;
					
					shade = ((inDrawFlags & OBJcDrawFlag_Selected) != 0) ? IMcShade_Green : IMcShade_Red;
					
					// draw the bounding box if this is the selected object
					M3rMinMaxBBox_To_BBox(&sound_osd->u.bvolume.bbox, &bBox);
					M3rBBox_Draw_Line(&bBox, shade);
					
					M3rGeom_Line_Light(&bBox.localPoints[0], &bBox.localPoints[7], shade);
					M3rGeom_Line_Light(&bBox.localPoints[1], &bBox.localPoints[6], shade);
					M3rGeom_Line_Light(&bBox.localPoints[2], &bBox.localPoints[5], shade);
					M3rGeom_Line_Light(&bBox.localPoints[3], &bBox.localPoints[4], shade);
				}
			break;
		}
		
		// draw the name
		camera_location = CArGetLocation();
		if (MUrPoint_Distance(&inObject->position, &camera_location) < 150.0f)
		{
			dest.y += 1.0f;
			OBJiSound_DrawName(inObject, sound_osd, &dest);
		}
		
		// draw the rotation rings if this is the selected object
		if (inDrawFlags & OBJcDrawFlag_Selected)
		{
			M3tBoundingSphere		bounding_sphere;
			
			OBJrObject_GetBoundingSphere(inObject, &bounding_sphere);
			OBJrObjectUtil_DrawRotationRings(inObject, &bounding_sphere, inDrawFlags);
		}
		
		M3rMatrixStack_Pop();
	}
#endif

	return;
}

// ----------------------------------------------------------------------
static UUtError
OBJiSound_Enumerate(
	OBJtObject						*inObject,
	OBJtEnumCallback_ObjectName		inEnumCallback,
	UUtUns32						inUserData)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiSound_GetBoundingSphere(
	const OBJtObject		*inObject,
	M3tBoundingSphere		*outBoundingSphere)
{
	MUmVector_Set(outBoundingSphere->center, 0.0f, 0.0f, 0.0f);
	outBoundingSphere->radius = 1.5f;
}

// ----------------------------------------------------------------------
static void
OBJiSound_OSDGetName(
	const OBJtOSD_All		*inObject,
	char					*outName,
	UUtUns32				inNameLength)
{
}

// ----------------------------------------------------------------------
static void
OBJiSound_OSDSetName(
	OBJtOSD_All		*inObject,
	const char		*outName)
{
}

// ----------------------------------------------------------------------
static void
OBJiSound_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	OBJtOSD_All				*sound_osd;
	
	sound_osd = (OBJtOSD_All*)inObject->object_data;
	
	outOSD->osd.sound_osd = sound_osd->osd.sound_osd;

	return;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiSound_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	UUtUns32				result;
	
	result =
		BFcMaxFileNameLength +					/* ambient name */
		sizeof(OBJtSoundType) +					/* sound type */
		UUmMax(									/* sound spheres or volumes */
			sizeof(OBJtOSD_Sound_Spheres),
			sizeof(M3tBoundingBox_MinMax)) +
		sizeof(float) +							/* pitch */
		sizeof(float);							/* volume */
		
	return result;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiSound_IntersectsLine(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inStartPoint,
	const M3tPoint3D		*inEndPoint)
{
	M3tBoundingSphere		sphere;

	UUrMemory_Clear(&sphere, sizeof(M3tBoundingSphere));
	
	// get the bounding sphere
	OBJrObject_GetBoundingSphere(inObject, &sphere);
	
	sphere.center.x += inObject->position.x;
	sphere.center.y += inObject->position.y;
	sphere.center.z += inObject->position.z;
	
	return CLrSphere_Line(inStartPoint, inEndPoint, &sphere);
}

// ----------------------------------------------------------------------
static UUtError
OBJiSound_SetDefaults(
	OBJtOSD_All			*outOSD)
{
	outOSD->osd.sound_osd.type = OBJcSoundType_Spheres;
	outOSD->osd.sound_osd.ambient_name[0] = '\0';
	outOSD->osd.sound_osd.ambient = NULL;
	outOSD->osd.sound_osd.pitch = 1.0f;
	outOSD->osd.sound_osd.volume = 1.0f;
	outOSD->osd.sound_osd.u.spheres.max_volume_distance = 5.0f;
	outOSD->osd.sound_osd.u.spheres.min_volume_distance = 10.0f;
			
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiSound_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_All				osd_all;
	UUtError				error;
	
	if (inOSD == NULL)
	{
		error = OBJiSound_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);
		
		// send osd_all to OBJmObject_SetOSD()
		inOSD = &osd_all;
	}
	
	// set the object specific data and position
	OBJiSound_SetOSD(inObject, inOSD);
	OBJiSound_UpdatePosition(inObject);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiSound_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_Sound			*sound_osd;
	UUtUns32				bytes_read;
	
	// get a pointer to the object osd
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	UUrMemory_Clear(sound_osd, sizeof(OBJtOSD_Sound));
	
	// read the sound data
	OBDmGetStringFromBuffer(inBuffer, sound_osd->ambient_name, BFcMaxFileNameLength, inSwapIt);
	
	if (inVersion < OBJcVersion_30)
	{
		sound_osd->type = OBJcSoundType_Spheres;
		OBDmGet4BytesFromBuffer(inBuffer, sound_osd->u.spheres.max_volume_distance, float, inSwapIt);
		OBDmGet4BytesFromBuffer(inBuffer, sound_osd->u.spheres.min_volume_distance, float, inSwapIt);
		
		sound_osd->pitch = 1.0f;
		sound_osd->volume = 1.0f;
		
		// set the number of bytes read
		bytes_read =
			BFcMaxFileNameLength +		// name
			sizeof(float) +				// max volume distance
			sizeof(float);				// min volume distance
	}
	else
	{
		OBDmGet4BytesFromBuffer(inBuffer, sound_osd->type, OBJtSoundType, inSwapIt);
		
		switch (sound_osd->type)
		{
			case OBJcSoundType_Spheres:
				OBDmGet4BytesFromBuffer(inBuffer, sound_osd->u.spheres.max_volume_distance, float, inSwapIt);
				OBDmGet4BytesFromBuffer(inBuffer, sound_osd->u.spheres.min_volume_distance, float, inSwapIt);
			break;
			
			case OBJcSoundType_BVolume:
				OBDmGet4BytesFromBuffer(inBuffer, sound_osd->u.bvolume.bbox.minPoint.x, float, inSwapIt);
				OBDmGet4BytesFromBuffer(inBuffer, sound_osd->u.bvolume.bbox.minPoint.y, float, inSwapIt);
				OBDmGet4BytesFromBuffer(inBuffer, sound_osd->u.bvolume.bbox.minPoint.z, float, inSwapIt);
				
				OBDmGet4BytesFromBuffer(inBuffer, sound_osd->u.bvolume.bbox.maxPoint.x, float, inSwapIt);
				OBDmGet4BytesFromBuffer(inBuffer, sound_osd->u.bvolume.bbox.maxPoint.y, float, inSwapIt);
				OBDmGet4BytesFromBuffer(inBuffer, sound_osd->u.bvolume.bbox.maxPoint.z, float, inSwapIt);
			break;
		}
		
		if (inVersion < OBJcVersion_33)
		{
			sound_osd->pitch = 1.0f;
			sound_osd->volume = 1.0f;
			
			bytes_read =
				BFcMaxFileNameLength +					/* ambient name */
				sizeof(OBJtSoundType) +					/* sound type */
				UUmMax(									/* sound spheres or volumes */
					sizeof(OBJtOSD_Sound_Spheres),
					sizeof(M3tBoundingBox_MinMax));
		}
		else
		{
			OBDmGet4BytesFromBuffer(inBuffer, sound_osd->pitch, float, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, sound_osd->volume, float, inSwapIt);

			// set the number of bytes read
			bytes_read = OBJiSound_GetOSDWriteSize(inObject);
		}
	}
	
	// set the osd and update the position
	OBJiSound_SetOSD(inObject, (OBJtOSD_All*)inObject->object_data);
	OBJrObject_UpdatePosition(inObject);
	
	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtError
OBJiSound_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_Sound			*sound_osd;
	
	// get a pointer to the sound_osd
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	
	// copy the data from inOSD into sound_osd
	*sound_osd = inOSD->osd.sound_osd;
	
	// set the pointer to the ambient sound
	sound_osd->ambient = OSrAmbient_GetByName(sound_osd->ambient_name);
		
	if (sound_osd->type == OBJcSoundType_BVolume)
	{
		// recalculate the bounding volume
		OBJiSound_CalcBoundingVolume(inObject, sound_osd);
	}

UUrMemory_Block_VerifyList();
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiSound_UpdatePosition(
	OBJtObject				*inObject)
{
	OBJtOSD_Sound			*sound_osd;
	
	// no rotation
	inObject->rotation.x = 0.0f;
	inObject->rotation.y = 0.0f;
	inObject->rotation.z = 0.0f;
	
	// get a pointer to the sound_osd
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	
	if (sound_osd->type == OBJcSoundType_BVolume)
	{
		// recalculate the bounding volume
		OBJiSound_CalcBoundingVolume(inObject, sound_osd);
	}
}

// ----------------------------------------------------------------------
static UUtError
OBJiSound_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	OBJtOSD_Sound			*sound_osd;
	UUtUns32				bytes_available;
		
	// get a pointer to the sound_osd
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	
	// set the number of bytes available
	bytes_available = *ioBufferSize;
			
	// read the data from the buffer
	OBDmWriteStringToBuffer(ioBuffer, sound_osd->ambient_name, BFcMaxFileNameLength, bytes_available, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(ioBuffer, sound_osd->type, OBJtSoundType, bytes_available, OBJcWrite_Little);
	
	switch (sound_osd->type)
	{
		case OBJcSoundType_Spheres:
			OBDmWrite4BytesToBuffer(ioBuffer, sound_osd->u.spheres.max_volume_distance, float, bytes_available, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, sound_osd->u.spheres.min_volume_distance, float, bytes_available, OBJcWrite_Little);
		break;
		
		case OBJcSoundType_BVolume:
			OBDmWrite4BytesToBuffer(ioBuffer, sound_osd->u.bvolume.bbox.minPoint.x, float, bytes_available, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, sound_osd->u.bvolume.bbox.minPoint.y, float, bytes_available, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, sound_osd->u.bvolume.bbox.minPoint.z, float, bytes_available, OBJcWrite_Little);

			OBDmWrite4BytesToBuffer(ioBuffer, sound_osd->u.bvolume.bbox.maxPoint.x, float, bytes_available, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, sound_osd->u.bvolume.bbox.maxPoint.y, float, bytes_available, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, sound_osd->u.bvolume.bbox.maxPoint.z, float, bytes_available, OBJcWrite_Little);
		break;
	}
		
	OBDmWrite4BytesToBuffer(ioBuffer, sound_osd->pitch, float, bytes_available, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(ioBuffer, sound_osd->volume, float, bytes_available, OBJcWrite_Little);

	// set ioBufferSize to the number of bytes written to the buffer
	*ioBufferSize = OBJiSound_GetOSDWriteSize(inObject); // *ioBufferSize - bytes_available;
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OBJiSound_GetVisible(
	void)
{
	return OBJgSound_DrawSounds;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiSound_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static void
OBJiSound_SetVisible(
	UUtBool					inIsVisible)
{
	OBJgSound_DrawSounds = inIsVisible;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
SStAmbient*
OBJrSound_GetAmbient(
	const OBJtObject		*inObject)
{
	OBJtOSD_Sound			*sound_osd;
	
	// get a pointer to the sound osd
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	
	if (sound_osd->ambient == NULL)
	{
		sound_osd->ambient = OSrAmbient_GetByName(sound_osd->ambient_name);
	}
	
	return sound_osd->ambient;
}

// ----------------------------------------------------------------------
UUtBool
OBJrSound_GetMinMaxDistances(
	const OBJtObject		*inObject,
	float					*outMaxVolumeDistance,
	float					*outMinVolumeDistance)
{
	OBJtOSD_Sound			*sound_osd;
	
	// get a pointer to the sound osd
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	if (sound_osd->type != OBJcSoundType_Spheres) { return UUcFalse; }
	
	*outMaxVolumeDistance = sound_osd->u.spheres.max_volume_distance;
	*outMinVolumeDistance = sound_osd->u.spheres.min_volume_distance;
	
	return UUcTrue;
}

// ----------------------------------------------------------------------
float
OBJrSound_GetPitch(
	const OBJtObject		*inObject)
{
	OBJtOSD_Sound			*sound_osd;
	
	// get a pointer to the sound osd
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	return sound_osd->pitch;
}

// ----------------------------------------------------------------------
OBJtSoundType
OBJrSound_GetType(
	const OBJtObject		*inObject)
{
	OBJtOSD_Sound			*sound_osd;
	
	// get a pointer to the sound osd
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	return sound_osd->type;
}

// ----------------------------------------------------------------------
float
OBJrSound_GetVolume(
	const OBJtObject		*inObject)
{
	OBJtOSD_Sound			*sound_osd;
	
	// get a pointer to the sound osd
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	return sound_osd->volume;
}

// ----------------------------------------------------------------------
UUtError
OBJrSound_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;
	
	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));
	
	// set up the methods structure
	methods.rNew				= OBJiSound_New;
	methods.rSetDefaults		= OBJiSound_SetDefaults;
	methods.rDelete				= OBJiSound_Delete;
	methods.rDraw				= OBJiSound_Draw;
	methods.rEnumerate			= OBJiSound_Enumerate;
	methods.rGetBoundingSphere	= OBJiSound_GetBoundingSphere;
	methods.rOSDGetName			= OBJiSound_OSDGetName;
	methods.rOSDSetName			= OBJiSound_OSDSetName;
	methods.rIntersectsLine		= OBJiSound_IntersectsLine;
	methods.rUpdatePosition		= OBJiSound_UpdatePosition;
	methods.rGetOSD				= OBJiSound_GetOSD;
	methods.rGetOSDWriteSize	= OBJiSound_GetOSDWriteSize;
	methods.rSetOSD				= OBJiSound_SetOSD;
	methods.rRead				= OBJiSound_Read;
	methods.rWrite				= OBJiSound_Write;
	
	// set up the type methods
	methods.rGetClassVisible	= OBJiSound_GetVisible;
	methods.rSearch				= OBJiSound_Search;
	methods.rSetClassVisible	= OBJiSound_SetVisible;
	
	// register the furniture methods
	error =
		OBJrObjectGroup_Register(
			OBJcType_Sound,
			OBJcTypeIndex_Sound,
			"Sound",
			sizeof(OBJtOSD_Sound),
			&methods,
			OBJcObjectGroupFlag_None);
	UUmError_ReturnOnError(error);
	
#if CONSOLE_DEBUGGING_COMMANDS
	// register the variables
	error =
		SLrGlobalVariable_Register_Bool(
			"show_sound_all",
			"Sound rings will always be drawn, not just when a sound is selected.",
			&OBJgSound_AlwaysDraw);
	UUmError_ReturnOnError(error);

	error =
		SLrGlobalVariable_Register_Bool(
			"show_sound_spheres",
			"Sound spheres will always be drawn, not just when a sound sphere is selected.",
			&OBJgSound_AlwaysDrawSpheres);
	UUmError_ReturnOnError(error);

	error =
		SLrGlobalVariable_Register_Bool(
			"show_sound_rectangles",
			"Sound rectangles will always be drawn, not just when a sound Rectangle is selected.",
			&OBJgSound_AlwaysDrawVolumes);
	UUmError_ReturnOnError(error);
#endif

	OBJgSound_DrawSounds = UUcFalse;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
OBJrSound_PointIn(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inPoint)
{
	OBJtOSD_Sound			*sound_osd;
	UUtBool					point_in_object;
	
	// get a pointer to the sound osd
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	
	point_in_object = UUcFalse;
	switch (sound_osd->type)
	{
		case OBJcSoundType_Spheres:
		{
			float					distance;
			
			distance = MUrPoint_Distance_Squared(inPoint, &inObject->position) * UUmSQR(SScFootToDist);
			point_in_object = (distance <= UUmSQR(sound_osd->u.spheres.min_volume_distance));
		}
		break;
		
		case OBJcSoundType_BVolume:
			point_in_object =
				PHrCollision_Volume_Point_Inside(&sound_osd->u.bvolume.bvolume, inPoint);
		break;
	}
	
	return point_in_object;
}

