// ======================================================================
// Imp_EnvParticle.h
// ======================================================================
#ifndef IMP_ENVPARTICLE_H
#define IMP_ENVPARTICLE_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

#include "BFW_EnvParticle.h"

// ======================================================================
// prototypes
// ======================================================================
UUtError IMPrParse_EnvParticle(
	M3tMatrix4x3		*inMatrix,
	UUtUns32			inUserDataCount,
	char				*inUserData,
	char				*inMarkerName,
	char				*inNodeName,
	const char*			inFileName,
	EPtEnvParticle		*outParticle);

// ======================================================================
#endif /* IMP_ENVPARTICLE_H */
