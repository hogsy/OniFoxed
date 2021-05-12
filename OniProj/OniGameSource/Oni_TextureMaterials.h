// ======================================================================
// Oni_TextureMaterials.h
// ======================================================================
#pragma once

#ifndef ONI_TEXTUREMATERIALS_H
#define ONI_TEXTUREMATERIALS_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Materials.h"

// ======================================================================
// defines
// ======================================================================
#define ONcTMBinaryDataClass			UUm4CharToUns32('T', 'M', 'B', 'D')

// ======================================================================
// prototypes
// ======================================================================
UUtError
ONrTextureMaterialList_Save(
	UUtBool						inAutoSave);

UUtError
ONrTextureMaterialList_TextureMaterial_Set(
	const char					*inTextureName,
	MAtMaterialType				inMaterialType);
	
// ----------------------------------------------------------------------
UUtError
ONrTextureMaterials_Initialize(
	void);

void
ONrTextureMaterials_Terminate(
	void);

// ----------------------------------------------------------------------
void
ONrTextureMaterials_PreProcess(
	void);

// ----------------------------------------------------------------------
void
ONrTextureMaterials_LevelLoad(
	void);

// ======================================================================
#endif /* ONI_TEXTUREMATERIALS_H */