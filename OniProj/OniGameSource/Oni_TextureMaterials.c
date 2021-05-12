// ======================================================================
// Oni_TextureMaterials.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Materials.h"
#include "BFW_Motoko.h"

#include "Oni_BinaryData.h"
#include "Oni_TextureMaterials.h"

// ======================================================================
// defines
// ======================================================================
#define ONcMaxNameLength		(32)

// ======================================================================
// enums
// ======================================================================
enum
{
	ONcTMVersion_1				= 1,
	
	ONcTMCurrentVersion			= ONcTMVersion_1
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct ONtTextureMaterial
{
	MAtMaterialType				type;
	const char					*material_name;		// only valid between load and preprocess
	M3tTextureMap				*texture;
	char						texture_name[ONcMaxNameLength];
	
} ONtTextureMaterial;

// ======================================================================
// globals
// ======================================================================
static UUtMemory_Array			*ONgTextureMaterials;
static UUtBool					ONgTextureMaterialsBinaryDataAllocated;
static BDtData					*ONgTextureMaterialsBinaryData;

// ======================================================================
// prototypes
// ======================================================================
static void
ONiTextureMaterialList_SortByTexture(
	void);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static ONtTextureMaterial*
ONiTextureMaterial_GetByTextureName(
	const char					*inTextureName)
{
	ONtTextureMaterial			*found_texture_material;
	ONtTextureMaterial			*materials_array;
	UUtUns32					num_elements;
	UUtUns32					i;
	
	found_texture_material = NULL;
	
	// get a pointer to the materials array
	materials_array = (ONtTextureMaterial*)UUrMemory_Array_GetMemory(ONgTextureMaterials);
	if (materials_array == NULL) { return NULL; }
	
	// search the array for the texture name
	num_elements = UUrMemory_Array_GetUsedElems(ONgTextureMaterials);
	for (i = 0; i < num_elements; i++)
	{
		if (strcmp(materials_array[i].texture_name, inTextureName) == 0)
		{
			found_texture_material = &materials_array[i];
			break;
		}
	}
	
	return found_texture_material;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
ONiTMBinaryData_Load(
	const char					*inIdentifier,
	BDtData						*ioBinaryData,
	UUtBool						inSwapIt,
	UUtBool						inLocked,
	UUtBool						inAllocated)
{
	UUtError					error;
	UUtUns8						*buffer;
	UUtUns32					buffer_size;
	UUtUns32					version;
	UUtUns32					num_elements;
	ONtTextureMaterial			*materials_array;
	UUtUns32					i;
	
	UUmAssert(inIdentifier);
	UUmAssert(ioBinaryData);
	
	buffer = ioBinaryData->data;
	buffer_size = ioBinaryData->header.data_size;
	
	// read the version number
	buffer_size -= OBDmGet4BytesFromBuffer(buffer, version, UUtUns32, inSwapIt);
	
	// read the number of elements
	buffer_size -= OBDmGet4BytesFromBuffer(buffer, num_elements, UUtUns32, inSwapIt);
	
	// is there enough data
	UUmAssert(((ONcMaxNameLength + ONcMaxNameLength) * num_elements) == buffer_size); 
	
	// set the number of elements in the array
	error = UUrMemory_Array_SetUsedElems(ONgTextureMaterials, num_elements, NULL);
	UUmError_ReturnOnError(error);
	
	// get a pointer to the array
	materials_array = (ONtTextureMaterial*)UUrMemory_Array_GetMemory(ONgTextureMaterials);
	UUmAssert((materials_array != NULL) || (0 == num_elements));
	
	// process the elements
	for (i = 0; i < num_elements; i++)
	{
		char					*material_name;
		char					*texture_name;
	
		// get a pointer to the material name
		material_name = (char*)buffer;
		buffer += ONcMaxNameLength;
		
		// get a pointer to the texture name
		texture_name = (char*)buffer;
		buffer += ONcMaxNameLength;
		
		// initialize the element
		materials_array[i].material_name = material_name;
		materials_array[i].type = MAcInvalidID;		// will be set up from name in a preprocess phase
													// after we know for sure that materials have loaded
		UUrString_Copy(materials_array[i].texture_name, texture_name, ONcMaxNameLength);
		materials_array[i].texture = NULL;			// will be set up at level load time
	}
	
	// CB: we do not dispose of allocated memory here because we still have pointers to it
	// in the form of material_name in each texturematerial. these pointers will be used and
	// removed in ONrTextureMaterials_PreProcess. instead store the memory.
	ONgTextureMaterialsBinaryDataAllocated = inAllocated;
	ONgTextureMaterialsBinaryData = ioBinaryData;

	return UUcError_None;
}	

// ----------------------------------------------------------------------
static UUtError
ONiTMBinaryData_Register(
	void)
{
	UUtError					error;
	BDtMethods					methods;
	
	methods.rLoad = ONiTMBinaryData_Load;
	
	error =	BDrRegisterClass(ONcTMBinaryDataClass, &methods);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiTMBinaryData_Save(
	UUtBool						inAutoSave)
{
	UUtError					error;
	UUtUns32					num_elements;
	UUtUns32					data_size;
	UUtUns8						*data;
	UUtUns8						*buffer;
	UUtUns32					num_bytes;
	ONtTextureMaterial			*materials_array;
	UUtUns32					i;
	UUtUns8						*num_elements_ptr;
	UUtUns32					num_elements_written;
	
	// get the number of elements that need to be saved
	num_elements = UUrMemory_Array_GetUsedElems(ONgTextureMaterials);
	
	// calculate the size of the data
	data_size =
		sizeof(UUtUns32) +										/* version */
		sizeof(UUtUns32) +										/* number of elements */
		(ONcMaxNameLength + ONcMaxNameLength) * num_elements;	/* elements */
	
	// allocate memory to hold that many elements
	data = (UUtUns8*)UUrMemory_Block_NewClear(data_size);
	
	// init the temp pointers
	buffer = data;
	num_bytes = data_size;
	
	// write the version
	OBDmWrite4BytesToBuffer(buffer, ONcTMCurrentVersion, UUtUns32, num_bytes, OBJcWrite_Little);
	
	// save the num_elements pointer
	num_elements_ptr = buffer;
	buffer += sizeof(UUtUns32);
	
	// get a pointer to the materials array
	materials_array = (ONtTextureMaterial*)UUrMemory_Array_GetMemory(ONgTextureMaterials);
	
	// write the elements
	num_elements_written = 0;
	for (i = 0; i < num_elements; i++)
	{
		if (materials_array[i].type == MAcInvalidID) { continue; }
		
		// write the material instance name
		UUrString_Copy(
			(char*)buffer,
			MArMaterialType_GetName(materials_array[i].type), //ONgMaterialTypeHierarchy[materials_array[i].type].name,
			ONcMaxNameLength);
		buffer += ONcMaxNameLength;
		num_bytes -= ONcMaxNameLength;
		
		// write the texture name
		UUrString_Copy((char*)buffer, materials_array[i].texture_name, ONcMaxNameLength);
		buffer += ONcMaxNameLength;
		num_bytes -= ONcMaxNameLength;
		
		// increment the number of elements written
		num_elements_written++;
	}
	
	// write the number of elements
	OBDmWrite4BytesToBuffer(num_elements_ptr, num_elements_written, UUtUns32, num_bytes, OBJcWrite_Little);
	
	// save the material data
	error = 
		OBDrBinaryData_Save(
			ONcTMBinaryDataClass,
			"TextureMaterials",
			data,
			(data_size - num_bytes),
			0,
			inAutoSave);
	if (error != UUcError_None) {
		UUrDebuggerMessage("ONiTMBinaryData_Save: WARNING - could not save texture materials binary file!\n");
	}
	
	// dispose of memory
	UUrMemory_Block_Delete(data);
	data = NULL;
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static int UUcExternal_Call
ONiCompareTextures(
	const void					*inA,
	const void					*inB)
{
	ONtTextureMaterial			*a;
	ONtTextureMaterial			*b;
	
	a = (ONtTextureMaterial*)inA;
	b = (ONtTextureMaterial*)inB;
	
	return ((int)a->texture - (int)b->texture);
}

// ----------------------------------------------------------------------
static UUtError
ONiTextureMaterialList_Initialize(
	void)
{
	// allocate memory for the texture material list
	ONgTextureMaterials =
		UUrMemory_Array_New(
			sizeof(ONtTextureMaterial),
			5,
			0,
			0);
	UUmError_ReturnOnNull(ONgTextureMaterials);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
ONiTextureMaterialList_LevelLoad(
	void)
{
	UUtError					error;
	UUtUns32					i;
	ONtTextureMaterial			*materials_array;
	UUtUns32					num_elements;
	
	// update the pointers to all the textures in the level
	materials_array = (ONtTextureMaterial*)UUrMemory_Array_GetMemory(ONgTextureMaterials);
	num_elements = UUrMemory_Array_GetUsedElems(ONgTextureMaterials);
	for (i = 0; i < num_elements; i++)
	{
		M3tTextureMap			*texture;
		
		error =
			TMrInstance_GetDataPtr(
				M3cTemplate_TextureMap,
				materials_array[i].texture_name,
				&texture);
		if (error == UUcError_None)
		{
			materials_array[i].texture = texture;
			M3rTextureMap_SetMaterialType(texture, materials_array[i].type);
		}
		else
		{
			materials_array[i].texture = NULL;
			M3rTextureMap_SetMaterialType(texture, MAcMaterial_Base);
		}
	}
	
	// sort the materials
	ONiTextureMaterialList_SortByTexture();
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrTextureMaterialList_Save(
	UUtBool						inAutoSave)
{
	return ONiTMBinaryData_Save(inAutoSave);
}

// ----------------------------------------------------------------------
static void
ONiTextureMaterialList_SortByTexture(
	void)
{
	ONtTextureMaterial			*materials_array;
	
	// get a pointer to the material array
	materials_array = (ONtTextureMaterial*)UUrMemory_Array_GetMemory(ONgTextureMaterials);
	if (materials_array == NULL) { return; }
	
	// sort the materials array
	qsort(
		materials_array,
		UUrMemory_Array_GetUsedElems(ONgTextureMaterials),
		sizeof(ONtTextureMaterial),
		ONiCompareTextures);
}

// ----------------------------------------------------------------------
static void
ONiTextureMaterialList_Terminate(
	void)
{
	UUrMemory_Array_Delete(ONgTextureMaterials);
	ONgTextureMaterials = NULL;
}

// ----------------------------------------------------------------------
UUtError
ONrTextureMaterialList_TextureMaterial_Set(
	const char					*inTextureName,
	MAtMaterialType				inMaterialType)
{
	ONtTextureMaterial			*texture_material;
	UUtError					error;
	ONtTextureMaterial			*materials_array;
	
	// find the texture in the list
	texture_material = ONiTextureMaterial_GetByTextureName(inTextureName);
	if ((texture_material == NULL) && (inMaterialType != MAcInvalidID))
	{
		UUtUns32					index;
		
		// get a new element from the array
		error = UUrMemory_Array_GetNewElement(ONgTextureMaterials, &index, NULL);
		UUmError_ReturnOnError(error);
		
		// get a pointer to the material array
		materials_array = (ONtTextureMaterial*)UUrMemory_Array_GetMemory(ONgTextureMaterials);
		UUmAssert(materials_array);
		
		// set the texture material
		texture_material = &materials_array[index];
	}
	
	if (texture_material != NULL)
	{
		// re-set up the texture material
		texture_material->material_name = NULL;		// this pointer not valid here
		texture_material->type = inMaterialType;
		UUrString_Copy(texture_material->texture_name, inTextureName, ONcMaxNameLength);

		texture_material->texture = NULL;
		TMrInstance_GetDataPtr(M3cTemplate_TextureMap, texture_material->texture_name, &texture_material->texture);
		if (texture_material->texture != NULL) {
			M3rTextureMap_SetMaterialType(texture_material->texture, texture_material->type);
		}
		
		// sort the materials
		ONiTextureMaterialList_SortByTexture();
	}

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
ONrTextureMaterials_Initialize(
	void)
{
	UUtError					error;
	
	ONgTextureMaterialsBinaryDataAllocated = UUcFalse;
	ONgTextureMaterialsBinaryData = NULL;

	error = ONiTMBinaryData_Register();
	UUmError_ReturnOnError(error);
	
	error = ONiTextureMaterialList_Initialize();
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
ONrTextureMaterials_Terminate(
	void)
{
	ONiTextureMaterialList_Terminate();
}

void ONrTextureMaterials_LevelLoad(void)
{
	ONtTextureMaterial			*materials_array;
	UUtUns32					num_elements;
	UUtUns32					i;
	
	num_elements = UUrMemory_Array_GetUsedElems(ONgTextureMaterials);
	materials_array = (ONtTextureMaterial*) UUrMemory_Array_GetMemory(ONgTextureMaterials);
	
	// find the texture for each element, and set up 
	for (i = 0; i < num_elements; i++)
	{
		ONtTextureMaterial *material = materials_array + i;

		material->texture = NULL;
		TMrInstance_GetDataPtr(M3cTemplate_TextureMap, material->texture_name, &material->texture);

		if (material->texture != NULL) {
			M3rTextureMap_SetMaterialType(material->texture, material->type);
		}
	}
	

	return;
}

void
ONrTextureMaterials_PreProcess(
	void)
{
	ONtTextureMaterial			*materials_array;
	UUtUns32					num_elements;
	UUtUns32					i;
	
	num_elements = UUrMemory_Array_GetUsedElems(ONgTextureMaterials);

	if (num_elements == 0) {
		return;
	} else {
		// we must have loaded binary data in order for this to be true
		UUmAssert(ONgTextureMaterialsBinaryData != NULL);
	}
	
	materials_array = (ONtTextureMaterial*) UUrMemory_Array_GetMemory(ONgTextureMaterials);

	// set up the material type for each element from the stored material name pointer
	for (i = 0; i < num_elements; i++)
	{
		ONtTextureMaterial *material = materials_array + i;

		// get the material type
		material->type = MArMaterialType_GetByName(material->material_name);
		material->material_name = NULL;		// clear the stored string pointer

		if (material->type == MAcInvalidID) {
			// the material was not found!
			material->type = MAcMaterial_Base;
		}	
	}

	// now that we have finshed using the stored string pointers, we can
	// dispose of memory allocated at load time, if any
	if (ONgTextureMaterialsBinaryData) {
		if (ONgTextureMaterialsBinaryDataAllocated) {
			UUrMemory_Block_Delete(ONgTextureMaterialsBinaryData);
			UUrMemory_Block_VerifyList();
		}
		ONgTextureMaterialsBinaryData = NULL;
	}	
}

