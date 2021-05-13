// ======================================================================
// BFW_Materials.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Materials.h"
#include "BFW_TemplateManager.h"

// ======================================================================
// typedefs
// ======================================================================

TMtPrivateData				*MAgMaterials_PrivateData = NULL;
TMtPrivateData				*MAgImpacts_PrivateData = NULL;

static UUtUns32				MAgNumMaterials;
static MAtMaterial			**MAgMaterialArray;

static UUtUns32				MAgNumImpacts;
static MAtImpact			**MAgImpactArray;

typedef struct MAtImpactBuildData {
	MAtImpact *impact;
	struct MAtImpactBuildData *parent;
	struct MAtImpactBuildData *child;
	struct MAtImpactBuildData *sibling;
} MAtImpactBuildData;

typedef struct MAtMaterialBuildData {
	MAtMaterial *material;
	struct MAtMaterialBuildData *parent;
	struct MAtMaterialBuildData *child;
	struct MAtMaterialBuildData *sibling;
} MAtMaterialBuildData;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
MAtImpactType
MArImpactType_GetByName(
	const char					*inImpactTypeName)
{
	UUtError					error;
	MAtImpact					*impact;
	
	UUmAssert(inImpactTypeName);
	
	error = TMrInstance_GetDataPtr(MAcTemplate_Impact, inImpactTypeName, &impact);
	if (error != UUcError_None) { return MAcInvalidID; }
	
	return impact->id;
}

// ----------------------------------------------------------------------
const char*
MArImpactType_GetName(
	MAtImpactType				inImpactType)
{
	MAtImpact	*impact;

	UUmAssert((inImpactType >= 0) && (inImpactType < MAgNumImpacts));

	impact = MAgImpactArray[inImpactType];
	UUmAssertReadPtr(impact, sizeof(MAtImpact));

	return TMrInstance_GetInstanceName(impact);
}

// ----------------------------------------------------------------------
UUtUns16
MArImpactType_GetDepth(
	MAtImpactType				inImpactType)
{
	MAtImpact	*impact;

	UUmAssert((inImpactType >= 0) && (inImpactType < MAgNumImpacts));

	impact = MAgImpactArray[inImpactType];
	UUmAssertReadPtr(impact, sizeof(MAtImpact));

	return impact->depth;
}

// ----------------------------------------------------------------------
MAtImpactType
MArImpactType_GetParent(
	MAtImpactType				inImpactType)
{
	MAtImpact	*impact, *parent;

	UUmAssert((inImpactType >= 0) && (inImpactType < MAgNumImpacts));

	impact = MAgImpactArray[inImpactType];
	UUmAssertReadPtr(impact, sizeof(MAtImpact));

	parent = impact->parent;
	if (parent == NULL) {
		return MAcInvalidID;
	}
	
	UUmAssertReadPtr(parent, sizeof(MAtImpact));

	UUmAssert((parent->id >= 0) && (parent->id < MAgNumImpacts));

	return parent->id;
}

// ----------------------------------------------------------------------
UUtBool
MArImpactType_LookupByMaterial(
	MAtImpactType				inImpactType)
{
	MAtImpact	*impact;

	if (inImpactType == MAcInvalidID)
		return UUcFalse;

	UUmAssert((inImpactType >= 0) && (inImpactType < MAgNumImpacts));

	impact = MAgImpactArray[inImpactType];
	UUmAssertReadPtr(impact, sizeof(MAtImpact));

	return impact->lookup_bymaterial;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
MAtMaterialType
MArMaterialType_GetByName(
	const char					*inMaterialTypeName)
{
	UUtError					error;
	MAtMaterial					*material;
	
	UUmAssert(inMaterialTypeName);
	
	error = TMrInstance_GetDataPtr(MAcTemplate_Material, inMaterialTypeName, &material);
	if (error != UUcError_None) { return MAcInvalidID; }
	
	return material->id;
}

// ----------------------------------------------------------------------
const char*
MArMaterialType_GetName(
	MAtMaterialType				inMaterialType)
{
	MAtMaterial	*material;

	UUmAssert((inMaterialType >= 0) && (inMaterialType < MAgNumMaterials));

	material = MAgMaterialArray[inMaterialType];
	UUmAssertReadPtr(material, sizeof(MAtMaterial));

	return TMrInstance_GetInstanceName(material);
}

// ----------------------------------------------------------------------
UUtUns16
MArMaterialType_GetDepth(
	MAtMaterialType				inMaterialType)
{
	MAtMaterial	*material;

	UUmAssert((inMaterialType >= 0) && (inMaterialType < MAgNumMaterials));

	material = MAgMaterialArray[inMaterialType];
	UUmAssertReadPtr(material, sizeof(MAtMaterial));

	return material->depth;
}

// ----------------------------------------------------------------------
MAtMaterialType
MArMaterialType_GetParent(
	MAtMaterialType				inMaterialType)
{
	MAtMaterial	*material, *parent;

	UUmAssert((inMaterialType >= 0) && (inMaterialType < MAgNumMaterials));

	material = MAgMaterialArray[inMaterialType];
	UUmAssertReadPtr(material, sizeof(MAtMaterial));

	parent = material->parent;
	if (parent == NULL) {
		return MAcInvalidID;
	}

	UUmAssertReadPtr(parent, sizeof(MAtMaterial));

	UUmAssert((parent->id >= 0) && (parent->id < MAgNumMaterials));

	return parent->id;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
MAiImpacts_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void					*inPrivateData)
{
	MAtImpact				*impact;
	
	impact = (MAtImpact*)inInstancePtr;
	
	switch (inMessage)
	{
		case TMcTemplateProcMessage_LoadPostProcess:
		{
			impact->id = MAcInvalidID;
		}
		break;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
MAiMaterials_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void					*inPrivateData)
{
	MAtMaterial				*material;
	
	material = (MAtMaterial*)inInstancePtr;
	
	switch (inMessage)
	{
		case TMcTemplateProcMessage_LoadPostProcess:
		{
			material->id = MAcInvalidID;
		}
		break;
	}
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
MArImpacts_GetNumTypes(
	void)
{
	return MAgNumImpacts;
}

// ----------------------------------------------------------------------
UUtUns32
MArMaterials_GetNumTypes(
	void)
{
	return MAgNumMaterials;
}

// ----------------------------------------------------------------------
UUtError
MArMaterials_Initialize(
	void)
{
	UUtError					error;
	
	// initialize the globals
	MAgNumImpacts = 0;
	MAgImpactArray = NULL;
	
	MAgNumMaterials = 0;
	MAgMaterialArray = NULL;
	
	error = MArMaterials_RegisterTemplates();
	UUmError_ReturnOnError(error);

	// install the proc handlers
	error =
		TMrTemplate_PrivateData_New(
			MAcTemplate_Material,
			0,
			MAiMaterials_ProcHandler,
			&MAgMaterials_PrivateData);
	UUmError_ReturnOnError(error);
		
	error =
		TMrTemplate_PrivateData_New(
			MAcTemplate_Impact,
			0,
			MAiImpacts_ProcHandler,
			&MAgImpacts_PrivateData);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
MArMaterials_RegisterTemplates(
	void)
{
	UUtError					error;
	
	error = TMrTemplate_Register(MAcTemplate_Material, sizeof(MAtMaterial), TMcFolding_Forbid);
	UUmError_ReturnOnError(error);
	
	error = TMrTemplate_Register(MAcTemplate_Impact, sizeof(MAtImpact), TMcFolding_Forbid);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
MArMaterials_Terminate(
	void)
{
	if (MAgImpactArray) {
		UUrMemory_Block_Delete(MAgImpactArray);
	}
	MAgNumImpacts = 0;
	MAgImpactArray = NULL;

	if (MAgMaterialArray) {
		UUrMemory_Block_Delete(MAgMaterialArray);
	}
	MAgNumMaterials = 0;
	MAgMaterialArray = NULL;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
MAiImpacts_Traverse(
	MAtImpactBuildData *	inImpactData,
	UUtUns32 *				inIndex,
	UUtUns32				inDepth,
	UUtBool					inLookupByMaterial)
{
	char *impact_name;
	MAtImpact *impact;
	UUtBool lookup_bymaterial;

	if (inImpactData == NULL)
		return;

	lookup_bymaterial = inLookupByMaterial;
	impact = inImpactData->impact;
	impact_name = TMrInstance_GetInstanceName(impact);
	if ((impact_name != NULL) && (UUmString_IsEqual(impact_name, "Blunt"))) {
		lookup_bymaterial = UUcTrue;
	}

	// store this impact
	UUmAssert((*inIndex >= 0) && (*inIndex < MAgNumImpacts));
	impact->id					= (MAtImpactType) *inIndex;
	impact->depth				= (UUtUns16) inDepth;
	impact->lookup_bymaterial	= lookup_bymaterial;
	MAgImpactArray[*inIndex] = inImpactData->impact;
	*inIndex += 1;

#if defined(DEBUGGING) && DEBUGGING
	{
		// very important: check that order is preserved in the hierarchy!
		MAtImpact *parent = impact->parent;
		if (parent != NULL) {
			UUmAssert(parent->id < impact->id);
		}
	}
#endif

	// depth-first traverse
	MAiImpacts_Traverse(inImpactData->child,	inIndex, inDepth + 1,	lookup_bymaterial);
	MAiImpacts_Traverse(inImpactData->sibling,	inIndex, inDepth,		inLookupByMaterial);
}

// ----------------------------------------------------------------------
UUtError
MArImpacts_PreProcess(
	void)
{
	UUtUns32 itr, traverse_id, num_impacts, num_passes, num_added;
	MAtImpactBuildData *impact_array, *parent_build, *impact_build, *sibling, *prev_sibling, *base;
	MAtImpact *impact, *parent;
	char *impact_name, *sibling_name;
	UUtError error;

	num_impacts = TMrInstance_GetTagCount(MAcTemplate_Impact);

	if (num_impacts == 0) {
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "no impact types!");
	}

	// set all impacts' IDs to invalid
	for (itr = 0; itr < num_impacts; itr++) {
		error = TMrInstance_GetDataPtr_ByNumber(MAcTemplate_Impact, itr, &impact);
		UUmError_ReturnOnError(error);
		
		impact->id = MAcInvalidID;
	}

	num_passes = 0;
	base = NULL;
	MAgNumImpacts = 0;
	impact_array = UUrMemory_Block_NewClear(num_impacts * sizeof(MAtImpactBuildData));
	UUmError_ReturnOnNull(impact_array);

	do {
		// loop over all impacts and hierarchically order them
		num_added = 0;
		for (itr = 0; itr < num_impacts; itr++) {
			error = TMrInstance_GetDataPtr_ByNumber(MAcTemplate_Impact, itr, &impact);
			UUmError_ReturnOnError(error);

			if (impact->id != MAcInvalidID)
				continue;

			parent = impact->parent;
			if (parent == NULL) {
				if (base != NULL) {
					if (num_passes == 0) {
						// only scream once
						UUrDebuggerMessage("MArImpacts_PreProcess: multiple base impacts (%s and %s have parent 'None')\n",
								TMrInstance_GetInstanceName(base->impact), TMrInstance_GetInstanceName(impact));
					}
					continue;
				} else {
					UUmAssert(num_passes == 0);
					parent_build = NULL;
				}
			} else {
				UUmAssertReadPtr(parent, sizeof(MAtImpact));
				
				if (parent->id == MAcInvalidID) {
					// we can't add the impact because its parent has not yet been added.
					continue;
				} else {
					UUmAssert((parent->id >= 0) && (parent->id < MAgNumImpacts));
					parent_build = &impact_array[parent->id];
				}
			}

			UUmAssert(MAgNumImpacts < num_impacts);
			impact->id = (MAtImpactType) MAgNumImpacts++;

			impact_build = &impact_array[impact->id];
			impact_build->impact = impact;
			impact_build->parent = parent_build;
			impact_build->child = NULL;
			impact_build->sibling = NULL;
			num_added++;

			if (parent_build == NULL) {
				base = impact_build;
			} else {
				// find whereabouts we fit in the parent's child chain (sorted alphabetically)
				impact_name = TMrInstance_GetInstanceName(impact);
				prev_sibling = NULL;
				sibling = parent_build->child;
				while (sibling != NULL) {
					UUmAssert(sibling->parent == parent_build);

					sibling_name = TMrInstance_GetInstanceName(sibling->impact);
					if (strcmp(impact_name, sibling_name) < 0) {
						break;
					}
					prev_sibling = sibling;
					sibling = sibling->sibling;
				}

				impact_build->sibling = sibling;
				if (prev_sibling == NULL) {
					parent_build->child = impact_build;
				} else {
					prev_sibling->sibling = impact_build;
				}
			}
		}

		num_passes++;
	} while ((num_added > 0) && (num_passes < 1000));

	if (num_passes >= 1000) {
		UUrDebuggerMessage("MArImpacts_PreProcess: infinite loop detected, bailing early\n");
	}

	// check that we have added all impacts
	if (MAgNumImpacts < num_impacts) {
		UUrDebuggerMessage("MArImpacts_PreProcess: there were %d orphan impact types (out of %d total)...\n",
							num_impacts - MAgNumImpacts, num_impacts);

		for (itr = 0; itr < num_impacts; itr++) {
			error = TMrInstance_GetDataPtr_ByNumber(MAcTemplate_Impact, itr, &impact);
			UUmError_ReturnOnError(error);
			
			if (impact->id == MAcInvalidID) {
				UUrDebuggerMessage("  - %s (parent %s)\n", TMrInstance_GetInstanceName(impact),
								(impact->parent == NULL) ? "None" : TMrInstance_GetInstanceName(impact->parent));
			}
		}
	}

	// allocate the global impact array
	MAgImpactArray = (MAtImpact **) UUrMemory_Block_New(MAgNumImpacts * sizeof(MAtImpact *));
	UUmError_ReturnOnNull(MAgImpactArray);

	// do a depth-first traverse and add all impacts to the global array
	traverse_id = 0;
	MAiImpacts_Traverse(base, &traverse_id, 0, UUcFalse);

	UUmAssert(traverse_id == MAgNumImpacts);

	UUrMemory_Block_Delete(impact_array);
	
	return UUcError_None;
}


// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
MAiMaterials_Traverse(
	MAtMaterialBuildData *	inMaterialData,
	UUtUns32 *				inIndex,
	UUtUns32				inDepth,
	UUtBool					inParentBreakable)
{
	MAtMaterial *material;
	char *material_name;
	UUtBool breakable;

	if (inMaterialData == NULL)
		return;

	material = inMaterialData->material;

	// store this material
	UUmAssert((*inIndex >= 0) && (*inIndex < MAgNumMaterials));
	material->id = (MAtMaterialType) *inIndex;
	material->depth = (UUtUns16) inDepth;
	MAgMaterialArray[*inIndex] = inMaterialData->material;
	*inIndex += 1;

#if defined(DEBUGGING) && DEBUGGING
	{
		// very important: check that order is preserved in the hierarchy!
		MAtMaterial *parent = material->parent;
		if (parent != NULL) {
			UUmAssert(parent->id < material->id);
		}
	}
#endif

	material_name = TMrInstance_GetInstanceName(inMaterialData->material);
	if (material_name != NULL) {
		// work out if this material is breakable
		if (UUmString_IsEqual(material_name, "Glass")) {
			breakable = UUcTrue;
		} else if (UUmString_IsEqual(material_name, "Unbreak_Glass")) {
			breakable = UUcFalse;
		} else {
			breakable = inParentBreakable;
		}
	}
	inMaterialData->material->breakable = breakable;

	// depth-first traverse
	MAiMaterials_Traverse(inMaterialData->child,	inIndex, inDepth + 1, breakable);
	MAiMaterials_Traverse(inMaterialData->sibling,	inIndex, inDepth,     inParentBreakable);
}

// ----------------------------------------------------------------------
UUtError
MArMaterials_PreProcess(
	void)
{
	UUtUns32 itr, traverse_id, num_materials, num_passes, num_added;
	MAtMaterialBuildData *material_array, *parent_build, *material_build, *sibling, *prev_sibling, *base;
	MAtMaterial *material, *parent;
	char *material_name, *sibling_name;
	UUtError error;

	num_materials = TMrInstance_GetTagCount(MAcTemplate_Material);

	if (num_materials == 0) {
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "no material types!");
	}

	// set all materials' IDs to invalid
	for (itr = 0; itr < num_materials; itr++) {
		error = TMrInstance_GetDataPtr_ByNumber(MAcTemplate_Material, itr, &material);
		UUmError_ReturnOnError(error);
		
		material->id = MAcInvalidID;
	}

	num_passes = 0;
	base = NULL;
	MAgNumMaterials = 0;
	material_array = UUrMemory_Block_NewClear(num_materials * sizeof(MAtMaterialBuildData));
	UUmError_ReturnOnNull(material_array);

	do {
		// loop over all materials and hierarchically order them
		num_added = 0;
		for (itr = 0; itr < num_materials; itr++) {
			error = TMrInstance_GetDataPtr_ByNumber(MAcTemplate_Material, itr, &material);
			UUmError_ReturnOnError(error);

			if (material->id != MAcInvalidID)
				continue;

			parent = material->parent;
			if (parent == NULL) {
				if (base != NULL) {
					if (num_passes == 0) {
						// only scream once
						UUrDebuggerMessage("MArMaterials_PreProcess: multiple base materials (%s and %s have parent 'None')\n",
								TMrInstance_GetInstanceName(base->material), TMrInstance_GetInstanceName(material));
					}
					continue;
				} else {
					UUmAssert(num_passes == 0);
					parent_build = NULL;
				}
			} else {
				UUmAssertReadPtr(parent, sizeof(MAtMaterial));
				
				if (parent->id == MAcInvalidID) {
					// we can't add the material because its parent has not yet been added.
					continue;
				} else {
					UUmAssert((parent->id >= 0) && (parent->id < MAgNumMaterials));
					parent_build = &material_array[parent->id];
				}
			}

			UUmAssert(MAgNumMaterials < num_materials);
			material->id = (MAtMaterialType) MAgNumMaterials++;

			material_build = &material_array[material->id];
			material_build->material = material;
			material_build->parent = parent_build;
			material_build->child = NULL;
			material_build->sibling = NULL;
			num_added++;

			if (parent_build == NULL) {
				base = material_build;
			} else {
				// find whereabouts we fit in the parent's child chain (sorted alphabetically)
				material_name = TMrInstance_GetInstanceName(material);
				prev_sibling = NULL;
				sibling = parent_build->child;
				while (sibling != NULL) {
					UUmAssert(sibling->parent == parent_build);

					sibling_name = TMrInstance_GetInstanceName(sibling->material);
					if (strcmp(material_name, sibling_name) < 0) {
						break;
					}
					prev_sibling = sibling;
					sibling = sibling->sibling;
				}

				material_build->sibling = sibling;
				if (prev_sibling == NULL) {
					parent_build->child = material_build;
				} else {
					prev_sibling->sibling = material_build;
				}
			}
		}

		num_passes++;
	} while ((num_added > 0) && (num_passes < 1000));

	if (num_passes >= 1000) {
		UUrDebuggerMessage("MArMaterials_PreProcess: infinite loop detected, bailing early\n");
	}

	// check that we have added all materials
	if (MAgNumMaterials < num_materials) {
		UUrDebuggerMessage("MArMaterials_PreProcess: there were %d orphan material types (out of %d total)...\n",
							num_materials - MAgNumMaterials, num_materials);

		for (itr = 0; itr < num_materials; itr++) {
			error = TMrInstance_GetDataPtr_ByNumber(MAcTemplate_Material, itr, &material);
			UUmError_ReturnOnError(error);
			
			if (material->id == MAcInvalidID) {
				UUrDebuggerMessage("  - %s (parent %s)\n", TMrInstance_GetInstanceName(material),
								(material->parent == NULL) ? "None" : TMrInstance_GetInstanceName(material->parent));
			}
		}
	}

	// allocate the global material array
	MAgMaterialArray = (MAtMaterial **) UUrMemory_Block_New(MAgNumMaterials * sizeof(MAtMaterial *));
	UUmError_ReturnOnNull(MAgMaterialArray);

	// do a depth-first traverse and add all materials to the global array. this also determines which materials
	// are breakable
	traverse_id = 0;
	MAiMaterials_Traverse(base, &traverse_id, 0, UUcFalse);

	UUmAssert(traverse_id == MAgNumMaterials);

	UUrMemory_Block_Delete(material_array);
	
	return UUcError_None;
}

UUtBool MArMaterial_IsDescendant(MAtMaterialType inDescendant, MAtMaterialType inMaterial)
{
	MAtMaterialType material_itr;
	UUtBool is_descendant = UUcFalse;

	if ((inDescendant == MAcInvalidID) || (inMaterial == MAcInvalidID))
		return UUcFalse;

	UUmAssert((inDescendant >= 0) && (inDescendant < MAgNumMaterials));
	UUmAssert((inMaterial >= 0) && (inMaterial < MAgNumMaterials));

	for(material_itr = inDescendant; material_itr != MAcInvalidID; material_itr = MArMaterialType_GetParent(material_itr))
	{
		if (material_itr == inMaterial) {			
			is_descendant = UUcTrue;
			break;
		}
	}

	return is_descendant;
}

UUtBool
MArMaterialType_IsBreakable(
	MAtMaterialType				inMaterialType)
{
	MAtMaterial	*material;

	if (inMaterialType == MAcInvalidID)
		return UUcFalse;

	UUmAssert((inMaterialType >= 0) && (inMaterialType < MAgNumMaterials));
	if( inMaterialType >= MAgNumMaterials )
		return UUcFalse;

	material = MAgMaterialArray[inMaterialType];
	UUmAssertReadPtr(material, sizeof(MAtMaterial));

	return material->breakable;
}
