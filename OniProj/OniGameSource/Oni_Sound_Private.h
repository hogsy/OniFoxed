// ======================================================================
// Oni_Sound_Private.h
// ======================================================================
#pragma once

#ifndef ONI_SOUND_PRIVATE_H
#define ONI_SOUND_PRIVATE_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// typedefs
// ======================================================================
typedef struct OStTypeName
{
	char					*name;
	UUtUns32				type;

} OStTypeName;

// ======================================================================
// prototypes
// ======================================================================
UUtError
OSrVariantList_Initialize(
	void);

UUtError
OSrVariantList_LevelLoad(
	void);

void
OSrVariantList_LevelUnload(
	void);

void
OSrVariantList_Terminate(
	void);

// ======================================================================
#endif /* ONI_SOUND_PRIVATE_H */
