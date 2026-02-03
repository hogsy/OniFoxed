// ======================================================================
// OT_Particles.c
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

#include <ctype.h>
#include "EulerAngles.h"

// ======================================================================
// globals
// ======================================================================
static UUtBool				OBJgParticle_DrawParticles;
static UUtBool				OBJgParticle_AlwaysDrawTriangle;

static UUtError
OBJiParticle_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD);


// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiParticle_UpdateObjectPosition(
	OBJtObject *inObject)
{
	OBJtOSD_Particle		*particle_osd;
	float					rot_x;
	float					rot_y;
	float					rot_z;
	MUtEuler				euler;

	// get a pointer to the particle_osd
	particle_osd = (OBJtOSD_Particle*)inObject->object_data;

	// convert the rotation to radians
	rot_x = inObject->rotation.x * M3cDegToRad;
	rot_y = inObject->rotation.y * M3cDegToRad;
	rot_z = inObject->rotation.z * M3cDegToRad;

	// update the rotation matrix
	euler.order = MUcEulerOrderXYZs;
	euler.x = rot_x;
	euler.y = rot_y;
	euler.z = rot_z;
	MUrEulerToMatrix(&euler, &particle_osd->particle.matrix);

	MUrMatrix_SetTranslation(&particle_osd->particle.matrix, &inObject->position);

	// the particle's position needs to be updated
	if (particle_osd->particle.particle != NULL) {
		particle_osd->particle.particle->header.flags |= P3cParticleFlag_PositionChanged;
	}
	particle_osd->particle.dynamic_matrix = NULL;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiParticle_Delete(
	OBJtObject				*inObject)
{
	OBJtOSD_Particle *particle_osd = (OBJtOSD_Particle*)inObject->object_data;

	EPrDelete(&particle_osd->particle);
}

// ----------------------------------------------------------------------
static void
OBJiParticle_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
#if TOOL_VERSION
	// note - the actual position markers are drawn by BFW_EnvParticle

	if (inDrawFlags & OBJcDrawFlag_Selected)
	{
		OBJtOSD_Particle *particle_osd = (OBJtOSD_Particle*)inObject->object_data;
		M3tBoundingSphere bounding_sphere;

		// we are selected, we need to calculate whether or not we are a decal, because
		// this changes the way that the user interacts with us
		particle_osd->is_decal = ((particle_osd->particle.particle_class != NULL) &&
							(particle_osd->particle.particle_class->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal));

		// setup the matrix stack
		M3rMatrixStack_Push();
		M3rMatrixStack_Multiply(&particle_osd->particle.matrix);
		M3rGeom_State_Commit();

		OBJrObject_GetBoundingSphere(inObject, &bounding_sphere);
		OBJrObjectUtil_DrawRotationRings(inObject, &bounding_sphere, inDrawFlags);

		M3rMatrixStack_Pop();
	}
#endif

	return;
}

// ----------------------------------------------------------------------
static UUtError
OBJiParticle_Enumerate(
	OBJtObject						*inObject,
	OBJtEnumCallback_ObjectName		inEnumCallback,
	UUtUns32						inUserData)
{
	P3tParticleClassIterateState	itr;

	itr = P3cParticleClass_None;
	while (1)
	{
		UUtBool						result;
		P3tParticleClass			*particle_class;

		// iterate over all particle classes and pass their names to the
		// callback function (for storage in a listbox)
		result = P3rIterateClasses(&itr, &particle_class);
		if (result == UUcFalse) { break; }

		result = inEnumCallback(particle_class->classname, inUserData);
		if (result == UUcFalse) { break; }
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiParticle_GetBoundingSphere(
	const OBJtObject		*inObject,
	M3tBoundingSphere		*outBoundingSphere)
{
	MUmVector_Set(outBoundingSphere->center, 0.0f, 0.0f, 0.0f);
	outBoundingSphere->radius = 2.0f;
}

// ----------------------------------------------------------------------
static void
OBJiParticle_OSDGetName(
	const OBJtOSD_All		*inOSD,
	char					*outName,
	UUtUns32				inNameLength)
{
	const OBJtOSD_Particle *particle_osd = &inOSD->osd.particle_osd;

	UUrString_Copy(outName, particle_osd->particle.classname, inNameLength);
	if (particle_osd->particle.tagname[0] != '\0') {
		UUrString_Cat(outName, "_", inNameLength);
		UUrString_Cat(outName, particle_osd->particle.tagname, inNameLength);
	}

	return;
}

// ----------------------------------------------------------------------
static void
OBJiParticle_OSDSetName(
	OBJtOSD_All		*inOSD,
	const char		*inName)
{
	OBJtOSD_Particle *particle_osd = &inOSD->osd.particle_osd;

	if (strcmp(particle_osd->particle.classname, inName) != 0) {
		// set up the new name
		UUrString_Copy(particle_osd->particle.classname, inName,
							(P3cParticleClassNameLength + 1));

		EPrNotifyClassChange(&particle_osd->particle, ONrGameState_GetGameTime());
	}
}

// ----------------------------------------------------------------------
static void
OBJiParticle_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	OBJtOSD_Particle		*particle_osd;

	particle_osd = &((OBJtOSD_All *)inObject->object_data)->osd.particle_osd;

	UUrMemory_MoveFast(particle_osd, &outOSD->osd.particle_osd, sizeof(OBJtOSD_Particle));
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiParticle_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	UUtUns32				result;

	result = (P3cParticleClassNameLength + 1);			/* particle class name */
	result += (EPcParticleTagNameLength + 1);			/* particle tag name */
	result += sizeof(UUtUns16);							/* flags */
	result += 2 * sizeof(float);						/* decal x, y scale */

	return result;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiParticle_IntersectsLine(
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
OBJiParticle_SetDefaults(
	OBJtOSD_All			*outOSD)
{
	// clear the osd for the new particle point
	UUrMemory_Clear(&outOSD->osd.particle_osd, sizeof(OBJtOSD_Particle));

	outOSD->osd.particle_osd.particle.classname[0] = '\0';
	outOSD->osd.particle_osd.particle.tagname[0] = '\0';
	outOSD->osd.particle_osd.particle.flags = 0;
	outOSD->osd.particle_osd.particle.decal_xscale = 1.0f;
	outOSD->osd.particle_osd.particle.decal_yscale = 1.0f;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiParticle_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_Particle		*new_osd;
	UUtError				error;

	new_osd = (OBJtOSD_Particle *) inObject->object_data;

	// the new OSD has been allocated as blank... set its particle up and add
	// it to any appropriate lists. important: this must be done before we
	// set its OSD to be the input OSD.
	error = OBJiParticle_SetDefaults((OBJtOSD_All *) new_osd);
	UUmError_ReturnOnError(error);

	new_osd->particle.owner_type = EPcParticleOwner_InEngine;
	new_osd->particle.owner = (void *) &new_osd;

	EPrNew(&new_osd->particle, ONrGameState_GetGameTime());

	// set the particle's position
	OBJrObject_UpdatePosition(inObject);

	if (inOSD != NULL) {
		// copy the persistent fields from the input OSD; this
		// is only class, tag and flags.
		OBJiParticle_SetOSD(inObject, inOSD);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiParticle_Read_Version1(
	OBJtObject				*inObject,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_All				*osd_all;
	UUtUns32				bytes_read;

	// get a pointer to the object osd
	osd_all = (OBJtOSD_All*)inObject->object_data;
	bytes_read = 0;

	// read the Particle data
	UUrString_Copy(
		osd_all->osd.particle_osd.particle.classname,
		(char*)inBuffer,
		(P3cParticleClassNameLength + 1));
	inBuffer += (P3cParticleClassNameLength + 1);
	bytes_read += (P3cParticleClassNameLength + 1);

	// these values did not exist in version 1
	osd_all->osd.particle_osd.particle.tagname[0] = '\0';
	osd_all->osd.particle_osd.particle.flags = 0;
	osd_all->osd.particle_osd.particle.decal_xscale = 1.0f;
	osd_all->osd.particle_osd.particle.decal_yscale = 1.0f;

	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiParticle_Read_Version6(
	OBJtObject				*inObject,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_All				*osd_all;
	UUtUns32				bytes_read, flags;

	// get a pointer to the object osd
	osd_all = (OBJtOSD_All*)inObject->object_data;

	// read the version 1 data and advance past it
	bytes_read = OBJiParticle_Read_Version1(inObject, inSwapIt, inBuffer);
	inBuffer += bytes_read;

	/*
	 * read the version 6 data
	 */

	// velocity is now obsolete
	bytes_read += BDmSkip4BytesFromBuffer(inBuffer);

	// copy the tagname, which was only 32 characters long back then
	UUrString_Copy(
		osd_all->osd.particle_osd.particle.tagname,
		(char*)inBuffer,
		(OBJcParticleTagNameLength + 1));
	inBuffer += (OBJcParticleTagNameLength + 1);
	bytes_read += (OBJcParticleTagNameLength + 1);

	// get the flags and convert them from the old 32-bit format to the new 16-bit format
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, flags, UUtUns32, inSwapIt);

	osd_all->osd.particle_osd.particle.flags = 0;
	if (flags & OBJcParticleFlag_NotInitiallyCreated)
		osd_all->osd.particle_osd.particle.flags |= EPcParticleFlag_NotInitiallyCreated;

	// these values did not exist in version 6
	osd_all->osd.particle_osd.particle.decal_xscale = 1.0f;
	osd_all->osd.particle_osd.particle.decal_yscale = 1.0f;

	// set the OSD
	OBJiParticle_SetOSD(inObject, osd_all);

	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiParticle_Read_Version13(
	OBJtObject				*inObject,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_All				*osd_all;
	UUtUns32				bytes_read;

	// get a pointer to the object osd
	osd_all = (OBJtOSD_All*)inObject->object_data;

	/*
	 * read the version 13 data
	 */

	bytes_read = 0;

	// read the particle class name
	UUrString_Copy(
		osd_all->osd.particle_osd.particle.classname,
		(char*)inBuffer,
		(P3cParticleClassNameLength + 1));
	inBuffer += (P3cParticleClassNameLength + 1);
	bytes_read += (P3cParticleClassNameLength + 1);

	// read the tag name (all 48 chars of it)
	UUrString_Copy(
		osd_all->osd.particle_osd.particle.tagname,
		(char*)inBuffer,
		(EPcParticleTagNameLength + 1));
	inBuffer += (EPcParticleTagNameLength + 1);
	bytes_read += (EPcParticleTagNameLength + 1);

	// read the 16-bit flags
	bytes_read += OBJmGet2BytesFromBuffer(inBuffer, osd_all->osd.particle_osd.particle.flags, UUtUns16, inSwapIt);

	// these values did not exist in version 13
	osd_all->osd.particle_osd.particle.decal_xscale = 1.0f;
	osd_all->osd.particle_osd.particle.decal_yscale = 1.0f;

	// set the OSD
	OBJiParticle_SetOSD(inObject, osd_all);

	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiParticle_Read_Version32(
	OBJtObject				*inObject,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_All				*osd_all;
	UUtUns32				bytes_read;

	// get a pointer to the object osd
	osd_all = (OBJtOSD_All*)inObject->object_data;

	/*
	 * read the version 32 data
	 */

	bytes_read = 0;

	// read the particle class name
	UUrString_Copy(
		osd_all->osd.particle_osd.particle.classname,
		(char*)inBuffer,
		(P3cParticleClassNameLength + 1));
	inBuffer += (P3cParticleClassNameLength + 1);
	bytes_read += (P3cParticleClassNameLength + 1);

	// read the tag name (all 48 chars of it)
	UUrString_Copy(
		osd_all->osd.particle_osd.particle.tagname,
		(char*)inBuffer,
		(EPcParticleTagNameLength + 1));
	inBuffer += (EPcParticleTagNameLength + 1);
	bytes_read += (EPcParticleTagNameLength + 1);

	// read the 16-bit flags
	bytes_read += OBJmGet2BytesFromBuffer(inBuffer, osd_all->osd.particle_osd.particle.flags, UUtUns16, inSwapIt);

	// read the decal X and Y scales
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, osd_all->osd.particle_osd.particle.decal_xscale, float, inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, osd_all->osd.particle_osd.particle.decal_yscale, float, inSwapIt);

	// set the OSD
	OBJiParticle_SetOSD(inObject, osd_all);

	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiParticle_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	UUtUns32 bytes_read;
	OBJtOSD_Particle *particle_osd;

	if (inVersion >= OBJcVersion_32) {
		bytes_read = OBJiParticle_Read_Version32(inObject, inSwapIt, inBuffer);
	} else if (inVersion >= OBJcVersion_13) {
		bytes_read = OBJiParticle_Read_Version13(inObject, inSwapIt, inBuffer);
	} else if (inVersion >= OBJcVersion_6) {
		bytes_read = OBJiParticle_Read_Version6(inObject, inSwapIt, inBuffer);
	} else {
		bytes_read = OBJiParticle_Read_Version1(inObject, inSwapIt, inBuffer);
	}

	// get a pointer to the object osd
	particle_osd = (OBJtOSD_Particle*)inObject->object_data;
	particle_osd->owner = (OBJtObject *) inObject;

	// set up the ownership of our particle
	particle_osd->particle.owner_type = EPcParticleOwner_InEngine;
	particle_osd->particle.owner = (void *) particle_osd;

	// clear flags that don't persist
	particle_osd->particle.flags &= EPcParticleFlag_PersistentMask;

	// notify the env particle system that this particle exists
	EPrNew(&particle_osd->particle, 0);

	// set the particle's position matrices
	OBJiParticle_UpdateObjectPosition(inObject);
	EPrNotifyMatrixChange(&particle_osd->particle, ONrGameState_GetGameTime(), UUcFalse);

	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtError
OBJiParticle_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_Particle		*particle_osd;

	// get a pointer to the particle_osd
	particle_osd = (OBJtOSD_Particle*)inObject->object_data;
	particle_osd->owner = inObject;

	if (strcmp(particle_osd->particle.classname, inOSD->osd.particle_osd.particle.classname) != 0) {
		// the particle class has changed.
		UUrString_Copy(particle_osd->particle.classname, inOSD->osd.particle_osd.particle.classname,
							(P3cParticleClassNameLength + 1));

		EPrNotifyClassChange(&particle_osd->particle, ONrGameState_GetGameTime());
	}

	if (strcmp(particle_osd->particle.tagname, inOSD->osd.particle_osd.particle.tagname) != 0) {
		// the tag name has changed.
		EPrPreNotifyTagChange(&particle_osd->particle);

		UUrString_Copy(particle_osd->particle.tagname, inOSD->osd.particle_osd.particle.tagname,
							(EPcParticleTagNameLength + 1));

		EPrPostNotifyTagChange(&particle_osd->particle);

		// attractors which reference particle tags must be set up again
		P3rSetupAttractorPointers(NULL);
	}

	particle_osd->particle.flags = inOSD->osd.particle_osd.particle.flags;
	particle_osd->particle.decal_xscale = inOSD->osd.particle_osd.particle.decal_xscale;
	particle_osd->particle.decal_yscale = inOSD->osd.particle_osd.particle.decal_yscale;

	if ((particle_osd->particle.flags & EPcParticleFlag_NotInitiallyCreated) == 0) {
		if (particle_osd->particle.particle_class != NULL) {
			if (particle_osd->particle.particle == NULL) {
				// create a new particle
				EPrNewParticle(&particle_osd->particle, ONrGameState_GetGameTime());
			}
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiParticle_UpdatePosition(
	OBJtObject				*inObject)
{
	OBJtOSD_Particle *particle_osd = (OBJtOSD_Particle*) inObject->object_data;

	OBJiParticle_UpdateObjectPosition(inObject);
	EPrNotifyMatrixChange(&particle_osd->particle, ONrGameState_GetGameTime(), UUcFalse);
}

// ----------------------------------------------------------------------
static UUtError
OBJiParticle_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	OBJtOSD_Particle		*particle_osd;
	UUtUns32				bytes_available;

	// get a pointer to the particle_osd
	particle_osd = (OBJtOSD_Particle*)inObject->object_data;

	// get the number of bytes available
	bytes_available = *ioBufferSize;

	// write the data into the buffer
	UUrString_Copy((char*)ioBuffer, particle_osd->particle.classname, P3cParticleClassNameLength + 1);
	bytes_available -= (P3cParticleClassNameLength + 1);
	ioBuffer += (P3cParticleClassNameLength + 1);

	UUrString_Copy((char*)ioBuffer, particle_osd->particle.tagname, EPcParticleTagNameLength + 1);
	bytes_available -= (EPcParticleTagNameLength + 1);
	ioBuffer += (EPcParticleTagNameLength + 1);

	OBJmWrite2BytesToBuffer(ioBuffer, particle_osd->particle.flags, UUtUns16, bytes_available, OBJcWrite_Little);

	OBJmWrite4BytesToBuffer(ioBuffer, particle_osd->particle.decal_xscale, float, bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, particle_osd->particle.decal_yscale, float, bytes_available, OBJcWrite_Little);

	// set ioBufferSize to the number of bytes written to the buffer
	*ioBufferSize = *ioBufferSize - bytes_available;

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OBJiParticle_GetVisible(
	void)
{
	return EPgDrawParticles;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiParticle_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static void
OBJiParticle_SetVisible(
	UUtBool					inIsVisible)
{
	EPgDrawParticles = inIsVisible;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBJrParticle_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;

	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));

	// set up the methods structure
	methods.rNew				= OBJiParticle_New;
	methods.rSetDefaults		= OBJiParticle_SetDefaults;
	methods.rDelete				= OBJiParticle_Delete;
	methods.rDraw				= OBJiParticle_Draw;
	methods.rEnumerate			= OBJiParticle_Enumerate;
	methods.rGetBoundingSphere	= OBJiParticle_GetBoundingSphere;
	methods.rOSDGetName			= OBJiParticle_OSDGetName;
	methods.rOSDSetName			= OBJiParticle_OSDSetName;
	methods.rIntersectsLine		= OBJiParticle_IntersectsLine;
	methods.rUpdatePosition		= OBJiParticle_UpdatePosition;
	methods.rGetOSD				= OBJiParticle_GetOSD;
	methods.rGetOSDWriteSize	= OBJiParticle_GetOSDWriteSize;
	methods.rSetOSD				= OBJiParticle_SetOSD;
	methods.rRead				= OBJiParticle_Read;
	methods.rWrite				= OBJiParticle_Write;
	methods.rGetClassVisible	= OBJiParticle_GetVisible;
	methods.rSearch				= OBJiParticle_Search;
	methods.rSetClassVisible	= OBJiParticle_SetVisible;

	// register the particles methods
	error =
		OBJrObjectGroup_Register(
			OBJcType_Particle,
			OBJcTypeIndex_Particle,
			"Particle",
			sizeof(OBJtOSD_Particle),
			&methods,
			OBJcObjectGroupFlag_CanSetName);
	UUmError_ReturnOnError(error);

	// particle placement points are not initially visible
	OBJiParticle_SetVisible(UUcFalse);

	return UUcError_None;
}

