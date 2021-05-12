// ======================================================================
// BFW_Materials.h
// ======================================================================
#pragma once

#ifndef BFW_MATERIALS_H
#define BFW_MATERIALS_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// defines
// ======================================================================
#define MAcMaxNameLength			(32)

#define MAcInvalidID				0xFFFF

#define MAcImpact_Base				0
#define MAcMaterial_Base			0

// ======================================================================
// typedefs
// ======================================================================
typedef UUtUns16				MAtMaterialType;
typedef UUtUns16				MAtImpactType;

#define MAcTemplate_Material	UUm4CharToUns32('M', 't', 'r', 'l')
typedef tm_template('M', 't', 'r', 'l', "Material")
MAtMaterial
{
	UUtUns16					id;					// valid only at runtime
	UUtUns16					depth;				// valid only at runtime
	UUtBool						breakable;			// valid only at runtime
	tm_pad						pad0[3];
	tm_templateref				parent;
	
} MAtMaterial;

#define MAcTemplate_Impact		UUm4CharToUns32('I', 'm', 'p', 't')
typedef tm_template('I', 'm', 'p', 't', "Impact")
MAtImpact
{
	UUtUns16					id;					// valid only at runtime
	UUtUns16					depth;				// valid only at runtime
	UUtBool						lookup_bymaterial;	// valid only at runtime
	tm_pad						pad0[3];
	tm_templateref				parent;
	
} MAtImpact;

// ======================================================================
// prototypes
// ======================================================================
MAtImpactType
MArImpactType_GetByName(
	const char					*inImpactTypeName);
	
const char*
MArImpactType_GetName(
	MAtImpactType				inImpactType);

UUtUns16
MArImpactType_GetDepth(
	MAtImpactType				inImpactType);

MAtImpactType
MArImpactType_GetParent(
	MAtImpactType				inImpactType);

UUtBool
MArImpactType_LookupByMaterial(
	MAtImpactType				inImpactType);

// ----------------------------------------------------------------------
MAtMaterialType
MArMaterialType_GetByName(
	const char					*inMaterialTypeName);
	
const char*
MArMaterialType_GetName(
	MAtMaterialType				inMaterialType);

UUtUns16
MArMaterialType_GetDepth(
	MAtMaterialType				inMaterialType);

MAtMaterialType
MArMaterialType_GetParent(
	MAtMaterialType				inMaterialType);

// returns UUcTrue iff inDescendant descends from inMaterial
UUtBool MArMaterial_IsDescendent(
	MAtMaterialType inDescendant, 
	MAtMaterialType inMaterial);

UUtBool
MArMaterialType_IsBreakable(
	MAtMaterialType				inMaterialType);

// ----------------------------------------------------------------------
UUtUns32
MArImpacts_GetNumTypes(
	void);
	
UUtUns32
MArMaterials_GetNumTypes(
	void);

UUtError
MArMaterials_Initialize(
	void);
	
UUtError
MArMaterials_RegisterTemplates(
	void);

void
MArMaterials_Terminate(
	void);
	
// ----------------------------------------------------------------------
UUtError
MArImpacts_PreProcess(
	void);

UUtError
MArMaterials_PreProcess(
	void);

// ======================================================================
#endif /* BFW_MATERIALS_H */
