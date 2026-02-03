// ======================================================================
// Oni_Cinematics.h
// ======================================================================
#pragma once

#ifndef ONI_CINEMATICS_H
#define ONI_CINEMATICS_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// defines
// ======================================================================
#define OCcInvalidCinematic					UUcMaxUns32

// ======================================================================
// prototypes
// ======================================================================
void
OCrCinematic_Draw(
	void);

void
OCrCinematic_Start(
	char						*inTextureName,
	UUtInt16					inDrawWidth,
	UUtInt16					inDrawHeight,
	UUtUns8						inStart,
	UUtUns8						inEnd,
	float						inVelocity,
	UUtBool						inMirror);

void
OCrCinematic_Stop(
	char						*inTextureName,
	UUtUns8						inEnd,
	float						inVelocity);

void
OCrCinematic_DeleteAll(
	void);

void
OCrCinematic_Update(
	void);

// ----------------------------------------------------------------------
UUtError
OCrInitialize(
	void);

void
OCrTerminate(
	void);

// ======================================================================
#endif /* ONI_CINEMATICS_H */
