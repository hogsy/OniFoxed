// ======================================================================
// Oni_Object_Utils.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#include "Oni_Object.h"
#include "Oni_Object_Private.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
OBJrObjectUtil_DrawAxes(
	const float				inLength)
{
	UUtUns8		block_XYZ[(sizeof(M3tPoint3D) * 2) + (2 * UUcProcessor_CacheLineSize)];
	M3tPoint3D	*axes_XYZ = UUrAlignMemory(block_XYZ);
	
	axes_XYZ[0].x = 0.0f;
	axes_XYZ[0].y = 0.0f;
	axes_XYZ[0].z = 0.0f;
	
	axes_XYZ[1].x = inLength;
	axes_XYZ[1].y = 0.0f;
	axes_XYZ[1].z = 0.0f;
	M3rGeometry_LineDraw(2, axes_XYZ, IMcShade_Green);
	
	axes_XYZ[1].x = 0.0f;
	axes_XYZ[1].y = inLength;
	axes_XYZ[1].z = 0.0f;
	M3rGeometry_LineDraw(2, axes_XYZ, IMcShade_Red);
	
	axes_XYZ[1].x = 0.0f;
	axes_XYZ[1].y = 0.0f;
	axes_XYZ[1].z = inLength;
	M3rGeometry_LineDraw(2, axes_XYZ, IMcShade_Yellow);
}

// ----------------------------------------------------------------------
void
OBJrObjectUtil_DrawRotationRings(
	const OBJtObject		*inObject,
	const M3tBoundingSphere	*inBoundingSphere,
	UUtUns32				inDrawRings)
{
	IMtShade				shade;
	
	#define cNumPoints 24

	// one times cache line size should be plenty but why risk it
	UUtUns8					block_XZ[(sizeof(M3tPoint3D) * (cNumPoints + 1)) + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8					block_XY[(sizeof(M3tPoint3D) * (cNumPoints + 1)) + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8					block_YZ[(sizeof(M3tPoint3D) * (cNumPoints + 1)) + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8					block_ticks[(sizeof(M3tPoint3D) * (2)) + (2 * UUcProcessor_CacheLineSize)];
	
	M3tPoint3D				*ring_XZ = UUrAlignMemory(block_XZ);
	M3tPoint3D				*ring_XY = UUrAlignMemory(block_XY);
	M3tPoint3D				*ring_YZ = UUrAlignMemory(block_YZ);
	M3tPoint3D				*ticks = UUrAlignMemory(block_ticks);
	UUtUns32				itr;

	for(itr = 0; itr < cNumPoints; itr++)
	{
		float		theta;
		float		cos_theta_radius;
		float		sin_theta_radius;
		
		theta = M3c2Pi * (((float) itr) / cNumPoints);
		cos_theta_radius = MUrCos(theta) * inBoundingSphere->radius;
		sin_theta_radius = MUrSin(theta) * inBoundingSphere->radius;
		
		ring_XZ[itr].x = cos_theta_radius + inBoundingSphere->center.x;
		ring_XZ[itr].y = inBoundingSphere->center.y;
		ring_XZ[itr].z = sin_theta_radius + inBoundingSphere->center.z;
		
		ring_XY[itr].x = cos_theta_radius + inBoundingSphere->center.x;
		ring_XY[itr].y = sin_theta_radius + inBoundingSphere->center.y;
		ring_XY[itr].z = inBoundingSphere->center.z;

		ring_YZ[itr].x = inBoundingSphere->center.x;
		ring_YZ[itr].y = cos_theta_radius + inBoundingSphere->center.y;
		ring_YZ[itr].z = sin_theta_radius + inBoundingSphere->center.z;
	}
	
	ring_XZ[cNumPoints] = ring_XZ[0];
	ring_XY[cNumPoints] = ring_XY[0];
	ring_YZ[cNumPoints] = ring_YZ[0];
	
	if (inDrawRings & OBJcDrawFlag_RingY)
	{
		shade = (inDrawRings & OBJcDrawFlag_Locked) ? shade = IMcShade_Gray50 : IMcShade_Red;
		M3rGeometry_LineDraw(cNumPoints + 1, ring_XZ, shade);
	}
	
	if (inDrawRings & OBJcDrawFlag_RingZ)
	{
		shade = (inDrawRings & OBJcDrawFlag_Locked) ? shade = IMcShade_Gray50 : IMcShade_Yellow;
		M3rGeometry_LineDraw(cNumPoints + 1, ring_XY, shade);
	}
	
	if (inDrawRings & OBJcDrawFlag_RingX)
	{
		shade = (inDrawRings & OBJcDrawFlag_Locked) ? shade = IMcShade_Gray50 : IMcShade_Green;
		M3rGeometry_LineDraw(cNumPoints + 1, ring_YZ, shade);
	}
	
	for (itr = 0; itr < cNumPoints; itr += 2)
	{
		ticks[0] = ring_XZ[itr];
		MUmVector_Subtract(ticks[1], inBoundingSphere->center, ticks[0]);
		MUmVector_Scale(ticks[1], 0.1f);
		MUmVector_Add(ticks[1], ticks[1], ticks[0]);
		
		if (inDrawRings & OBJcDrawFlag_RingY)
		{
			shade = (inDrawRings & OBJcDrawFlag_Locked) ? shade = IMcShade_Gray50 : IMcShade_Red;
			M3rGeometry_LineDraw(2, ticks, shade);
		}
		
		ticks[0] = ring_XY[itr];
		MUmVector_Subtract(ticks[1], inBoundingSphere->center, ticks[0]);
		MUmVector_Scale(ticks[1], 0.1f);
		MUmVector_Add(ticks[1], ticks[1], ticks[0]);
		
		if (inDrawRings & OBJcDrawFlag_RingZ)
		{
			shade = (inDrawRings & OBJcDrawFlag_Locked) ? shade = IMcShade_Gray50 : IMcShade_Yellow;
			M3rGeometry_LineDraw(2, ticks, shade);
		}
		
		ticks[0] = ring_YZ[itr];
		MUmVector_Subtract(ticks[1], inBoundingSphere->center, ticks[0]);
		MUmVector_Scale(ticks[1], 0.1f);
		MUmVector_Add(ticks[1], ticks[1], ticks[0]);
		
		if (inDrawRings & OBJcDrawFlag_RingX)
		{
			shade = (inDrawRings & OBJcDrawFlag_Locked) ? shade = IMcShade_Gray50 : IMcShade_Green;
			M3rGeometry_LineDraw(2, ticks, shade);
		}
	}
}
// ----------------------------------------------------------------------

UUtError
OBJrObjectUtil_EnumerateTemplate(
	char							*inPrefix,
	TMtTemplateTag					inTemplateTag,
	OBJtEnumCallback_ObjectName		inEnumCallback,
	UUtUns32						inUserData)
{
	UUtError						error;
	void							*instances[OBJcMaxInstances];
	UUtUns32						num_instances;
	UUtUns32						i;
	UUtUns32						prefix_length;

	// get a list of instances of the class
	error =
		TMrInstance_GetDataPtr_List(
			inTemplateTag,
			OBJcMaxInstances,
			&num_instances,
			instances);
	UUmError_ReturnOnError(error);
	
	if (inPrefix)
	{
		prefix_length = strlen(inPrefix);
	}
	else
	{
		prefix_length = 0;
	}
	
	for (i = 0; i < num_instances; i++)
	{
		char				*instance_name;
		UUtBool				result = UUcTrue;
		
		// get the name of the template instance
		instance_name = TMrInstance_GetInstanceName(instances[i]);
		
		// send the information to the callback
		if (prefix_length == 0)
		{
			result = inEnumCallback(instance_name, inUserData);
		}
		else
		{
			// check for the prefix
			if (strncmp(instance_name, inPrefix, prefix_length) == 0)
			{
				result =
					inEnumCallback(
						(instance_name + prefix_length),
						inUserData);
			}
		}

		// stop if the result of the callback is false
		if (!result) { break; }
	}
	
	return UUcError_None;
}
