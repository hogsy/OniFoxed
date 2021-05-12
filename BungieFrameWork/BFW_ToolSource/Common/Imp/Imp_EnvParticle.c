// ======================================================================
// Imp_EnvParticle.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"
#include "BFW_Particle3.h"

#include "Imp_Common.h"
#include "Imp_ParseEnvFile.h"
#include "Imp_EnvParticle.h"

#include "EnvFileFormat.h"

// ======================================================================
// internal globals
// ======================================================================

AUtFlagElement	IMPgEnvParticleFlags[] = 
{
	{
		"not-created",
		EPcParticleFlag_NotInitiallyCreated
	},
	{
		NULL,
		0
	}
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------

UUtError IMPrParse_EnvParticle(
	M3tMatrix4x3		*inMatrix,
	UUtUns32			inUserDataCount,
	char				*inUserData,
	char				*inMarkerName,
	char				*inNodeName,
	const char*			inFileName,
	EPtEnvParticle		*outParticle)
{
	UUtError				error;
	GRtGroup_Context		*groupContext;
	GRtGroup				*group;
	GRtElementType			groupType;
	char					*stringptr;
	UUtUns32				flag_val;
	void					*groupFlags;
	M3tVector3D				marker_z;
	M3tVector3D				particle_x, particle_y, particle_z;
	M3tVector3D				position;
	
	groupContext = NULL;

	// clear the particle to zero (default values)
	UUmAssertWritePtr(outParticle, sizeof(EPtEnvParticle));
	UUrMemory_Clear(outParticle, sizeof(EPtEnvParticle));

	// make sure there is user data to process
	if ((inUserDataCount == 0) || (inUserData[0] == '\0'))
	{
		goto cleanup;
	}
	
	// create a GRtGroup from the user data
	error =
		GRrGroup_Context_NewFromString(
			inUserData,
			gPreferencesGroup,
			NULL,
			&groupContext,
			&group);
	if (error != UUcError_None)
		goto cleanup;
	
	error = GRrGroup_GetString(group, "particle_class", &stringptr);
	if (error != UUcError_None)
		goto cleanup;
	UUrString_Copy(outParticle->classname, stringptr, P3cParticleClassNameLength + 1);

	error = GRrGroup_GetString(group, "tag", &stringptr);
	if (error == UUcError_None) {
		UUrString_Copy(outParticle->tagname, stringptr, P3cParticleClassNameLength + 1);
	}
	
	// construct an orthonormal basis from the input matrix	
	MUrMatrix_GetCol(inMatrix, 1, &particle_y);
	MUmVector_Normalize(particle_y);

	MUrMatrix_GetCol(inMatrix, 2, &marker_z);
	MUrVector_CrossProduct(&particle_y, &marker_z, &particle_x);
	MUmVector_Normalize(particle_x);

	MUrVector_CrossProduct(&particle_x, &particle_y, &particle_z);
	MUmVector_Normalize(particle_z);

	MUrMatrix3x3_FormOrthonormalBasis(&particle_x, &particle_y, &particle_z,
									(M3tMatrix3x3 *) &outParticle->matrix);

	position = MUrMatrix_GetTranslation(inMatrix);
	MUrMatrix_SetTranslation(&outParticle->matrix, &position);

	// decal X and Y scale factors
	error = GRrGroup_GetFloat(group, "decal_x", &outParticle->decal_xscale);
	if (error != UUcError_None) {
		outParticle->decal_xscale = 1.0f;
	}
	error = GRrGroup_GetFloat(group, "decal_y", &outParticle->decal_yscale);
	if (error != UUcError_None) {
		outParticle->decal_yscale = 1.0f;
	}

	// flags
	error = GRrGroup_GetElement(group, "flags", &groupType, &groupFlags);
	if (error == UUcError_None) {
		error = AUrFlags_ParseFromGroupArray(IMPgEnvParticleFlags, groupType, groupFlags, &flag_val);
		if (error != UUcError_None)
			goto cleanup;

		outParticle->flags = (UUtUns16) flag_val;
	}

	// successfully parsed particle
	error = UUcError_None;
	
cleanup:
	if (groupContext)
	{
		GRrGroup_Context_Delete(groupContext);
	}

	if (error != UUcError_None) {
		Imp_PrintWarning("Imp_EnvParticle: %s: %s: %s - couldn't parse env particle!",
						inFileName, inNodeName, inMarkerName);
	}

	return error;
}
