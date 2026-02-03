// ======================================================================
// Oni_Sound.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_SoundSystem2.h"
#include "BFW_BinaryData.h"

#include "Oni_Sound.h"
#include "Oni_Sound_Private.h"
#include "Oni_Sound_Animation.h"
#include "Oni_BinaryData.h"
#include "Oni_Object.h"
#include "Oni_ImpactEffect.h"

// ======================================================================
// defines
// ======================================================================
#define OScMaxBufferSize			1024

#define OScCatTag					UUm4CharToUns32('S', 'C', 'A', 'T')
#define OScItemTag					UUm4CharToUns32('S', 'I', 'T', 'M')

#define OScAmbientWriteSize			\
	(sizeof(OStItem) + sizeof(SStAmbient) - sizeof(UUtUns32) - (sizeof(SStGroup*) * 5))
	/* OStItem + SStAmbient - SStAmbient->id - SStAmbient->SStGroup* */

#define OScImpulseWriteSize			\
	(sizeof(OStItem) + sizeof(SStImpulse) - sizeof(UUtUns32) - sizeof(SStGroup*))
	/* OStItem + SStImpulse - SStImpulse->id - SStImpulse->SStGroup* */

#define OScPermutationWriteSize		\
	(sizeof(SStPermutation) - sizeof(SStSoundData*))
	/* SStPermutation - SStSoundData* */

#define OScGroupWriteSize(num_perms)			\
	(sizeof(OStItem) + sizeof(UUtUns32) + (OScPermutationWriteSize * num_perms))
	/* OStItem + num_permutations + permutations */

// ======================================================================
// enums
// ======================================================================
enum
{
	OScCollectionFlag_None		= 0x0000,
	OScCollectionFlag_Locked	= 0x0001,
	OScCollectionFlag_Dirty		= 0x0002
};

// ======================================================================
// typedefs
// ======================================================================
struct OStItem
{
	UUtUns32					id;
	UUtUns32					category_id;
	char						name[OScMaxNameLength];

};

// ----------------------------------------------------------------------
struct OStCategory
{
	UUtUns32					id;
	UUtUns32					parent_id;
	char						name[OScMaxNameLength];
};

// ----------------------------------------------------------------------
struct OStCollection
{
	OStCollectionType			type;
	UUtUns32					flags;

	UUtMemory_Array				*categories;

	OStItemDeleteProc			item_delete_proc;
	UUtMemory_Array				*items;
};

// ----------------------------------------------------------------------
typedef struct OStPlayingAmbient
{
	const OBJtObject			*object;
	SStPlayID					play_id;

} OStPlayingAmbient;

// ----------------------------------------------------------------------
typedef struct OStPlayingMusic
{
	SStPlayID					play_id;
	char						name[OScMaxNameLength];

} OStPlayingMusic;

// ======================================================================
// globals
// ======================================================================
static UUtMemory_Array			*OSgCollections;
static UUtMemory_Array			*OSgPlayingAmbient;
static UUtMemory_Array			*OSgPlayingMusic;
static SStPlayID				OSgDialog_PlayID;

static char						*OSgPriority_Name[] =
{
	"Low",
	"Normal",
	"High",
	"Highest",
	NULL
};

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
OSiCollection_GetNextCategoryID(
	const OStCollection			*inCollection)
{
	UUtUns32					num_categories;
	OStCategory					*categories;
	UUtUns32					i;
	UUtUns32					next_category_id;

	// get the number of categories
	num_categories = UUrMemory_Array_GetUsedElems(inCollection->categories);

	// get a pointer to the categories
	categories = (OStCategory*)UUrMemory_Array_GetMemory(inCollection->categories);

	// search for the next available category id
	next_category_id = 1;
	while(1)
	{
		// try to find a category with an id equal to next_category_id
		for (i = 0; i < num_categories; i++)
		{
			if (next_category_id == categories[i].id)
			{
				break;
			}
		}

		// if all the category ids were checked and none matched next_category_id
		// then stop looking
		if (i == num_categories)
		{
			break;
		}

		// increment next_category_id and search again
		next_category_id++;
	}

	return next_category_id;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
static void
OSiImpulseSound_Delete(
	UUtUns32					inItemID)
{
	// notify the impact effects that this impulse sound has been deleted
	ONrImpactEffects_UpdateImpulseIDs(inItemID);

	// actually delete the sound
	SSrImpulse_Delete(inItemID);
}

// ----------------------------------------------------------------------
static UUtError
OSiCollections_AddType(
	OStCollectionType			inType,
	OStItemDeleteProc			inItemDeleteProc)
{
	UUtError					error;
	UUtUns32					index;
	UUtBool						mem_moved;
	OStCollection				*collections;
	UUtUns32					i;
	UUtUns32					num_elements;

	// get a pointer to the array memory
	collections = (OStCollection*)UUrMemory_Array_GetMemory(OSgCollections);
	if (collections != NULL)
	{
		// make sure there isn't a collection of the same type already in the list
		num_elements = UUrMemory_Array_GetUsedElems(OSgCollections);
		for (i = 0; i < num_elements; i++)
		{
			if (collections[i].type == inType) { return UUcError_Generic; }
		}
	}

	// get a new element from the array
	error =	UUrMemory_Array_GetNewElement(OSgCollections, &index, &mem_moved);
	UUmError_ReturnOnError(error);

	// if the memory moved, get another pointer to the collections
	if (mem_moved)
	{
		collections = (OStCollection*)UUrMemory_Array_GetMemory(OSgCollections);
	}

	// initialize the new collection
	collections[index].type = inType;
	collections[index].flags = OScCollectionFlag_None;
	collections[index].categories =
		UUrMemory_Array_New(
			sizeof(OStCategory),
			1,
			0,
			5);
	collections[index].item_delete_proc = inItemDeleteProc;
	collections[index].items =
		UUrMemory_Array_New(
			sizeof(OStItem),
			5,
			0,
			0);

	UUmError_ReturnOnNull(collections[index].categories);
	UUmError_ReturnOnNull(collections[index].items);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiCollections_Initialize(
	void)
{
	// create the collections array
	OSgCollections =
		UUrMemory_Array_New(
			sizeof(OStCollection),
			1,
			0,
			3);
	UUmError_ReturnOnNull(OSgCollections);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OSiCollections_Terminate(
	void)
{
	// delete the collections array
	if (OSgCollections)
	{
		OStCollection			*collections;

		// get a pointer to the collections
		collections = UUrMemory_Array_GetMemory(OSgCollections);
		if (collections != NULL)
		{
			UUtUns32				i;
			UUtUns32				num_collections;

			num_collections = UUrMemory_Array_GetUsedElems(OSgCollections);
			for (i = 0; i < num_collections; i++)
			{
				OStCollection			*collection;
				OStItem					*items;
				UUtUns32				j;
				UUtUns32				num_items;

				// get a pointer to the collection
				collection = &collections[i];

				// delete the categories array
				UUrMemory_Array_Delete(collection->categories);

				// delete all of the items' data
				items = (OStItem*)UUrMemory_Array_GetMemory(collection->items);
				num_items = UUrMemory_Array_GetUsedElems(collection->items);
				for (j = 0; j < num_items; j++)
				{
					collection->item_delete_proc(items[j].id);
				}

				// delte the items array
				UUrMemory_Array_Delete(collection->items);
			}
		}

		// delete the collection array
		UUrMemory_Array_Delete(OSgCollections);
		OSgCollections = NULL;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
OSrItem_GetCategoryID(
	const OStItem				*inItem)
{
	UUmAssert(inItem);
	return inItem->category_id;
}

// ----------------------------------------------------------------------
UUtUns32
OSrItem_GetID(
	const OStItem				*inItem)
{
	UUmAssert(inItem);
	return inItem->id;
}

// ----------------------------------------------------------------------
const char*
OSrItem_GetName(
	const OStItem				*inItem)
{
	UUmAssert(inItem);
	return inItem->name;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
OSrCategory_GetID(
	const OStCategory			*inCategory)
{
	UUmAssert(inCategory);
	return inCategory->id;
}

// ----------------------------------------------------------------------
const char*
OSrCategory_GetName(
	const OStCategory			*inCategory)
{
	UUmAssert(inCategory);
	return inCategory->name;
}

// ----------------------------------------------------------------------
UUtUns32
OSrCategory_GetParentCategoryID(
	const OStCategory			*inCategory)
{
	UUmAssert(inCategory);
	return inCategory->parent_id;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
OSiCollection_CalcMemoryUsed(
	const OStCollection			*inCollection)
{
	UUtUns32					num_items;
	UUtUns32					bytes_used;
	UUtUns32					i;

	bytes_used = 0;

	// Add the categories
	bytes_used +=
		((sizeof(UUtUns8) * 4) + sizeof(UUtUns32) + sizeof(OStCategory))*
		UUrMemory_Array_GetUsedElems(inCollection->categories);

	// Add the items
	num_items = UUrMemory_Array_GetUsedElems(inCollection->items);
	bytes_used += ((sizeof(UUtUns8) * 4) + sizeof(UUtUns32)) * num_items;
	switch (OSrCollection_GetType(inCollection))
	{
		case OScCollectionType_Group:
		{
			for (i = 0; i < num_items; i++)
			{
				OStItem					*item;
				SStGroup				*group;

				item = OSrCollection_Item_GetByIndex(inCollection, i);
				group = SSrGroup_GetByID(OSrItem_GetID(item));

				bytes_used += OScGroupWriteSize(UUrMemory_Array_GetUsedElems(group->permutations));
			}
		}
		break;

		case OScCollectionType_Impulse:
			for (i = 0; i < num_items; i++)
			{
				bytes_used += OScImpulseWriteSize;
			}
		break;

		case OScCollectionType_Ambient:
			for (i = 0; i < num_items; i++)
			{
				bytes_used += OScAmbientWriteSize;
			}
		break;
	}

	return bytes_used;
}

// ----------------------------------------------------------------------
static UUtError
OSiCollection_Category_CreateFromBuffer(
	OStCollection				*ioCollection,
	UUtUns32					inVersion,
	UUtBool						inSwapIt,
	UUtUns8						*inBuffer,
	UUtUns32					*outNumBytesRead)
{
	UUtError					error;
	UUtUns32					id;
	UUtUns32					parent_id;
	char						*name;
	UUtUns32					temp_id;
	OStCategory					*category;

	// init the returning value
	*outNumBytesRead = 0;

	// read the OStCategory
	OBDmGet4BytesFromBuffer(inBuffer, id, UUtUns32, inSwapIt);
	OBDmGet4BytesFromBuffer(inBuffer, parent_id, UUtUns32, inSwapIt);
	name = (char*)inBuffer; inBuffer += OScMaxNameLength;

	// make sure no other categories exist with this id
	category = OSrCollection_Category_GetByID(ioCollection, id);
	if (category != NULL) { return UUcError_Generic; }

	// add the category to the collection
	error =
		OSrCollection_Category_New(
			ioCollection,
			parent_id,
			name,
			&temp_id);
	if (error != UUcError_None)
	{
		OSrCollection_Category_Delete(ioCollection, temp_id);
	}

	// get a pointer to the new category
	category = OSrCollection_Category_GetByID(ioCollection, temp_id);
	if (category == NULL)
	{
		UUmAssert(!"This should never happen");
		return UUcError_Generic;
	}

	// set the category's id
	category->id = id;

	// set the number of bytes read
	*outNumBytesRead = sizeof(OStCategory);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrCollection_Category_Delete(
	OStCollection				*ioCollection,
	UUtUns32					inCategoryID)
{
	OStCategory					*categories;
	OStItem						*items;
	UUtUns32					i;
	UUtUns32					num_categories;
	UUtUns32					num_items;

	if ((ioCollection->flags & OScCollectionFlag_Locked) != 0) { return; }

	// delete the category with inCategoryID
	categories = (OStCategory*)UUrMemory_Array_GetMemory(ioCollection->categories);
	num_categories = UUrMemory_Array_GetUsedElems(ioCollection->categories);
	for (i = (num_categories - 1); i < num_categories; i--)
	{
		if (categories[i].id == inCategoryID)
		{
			UUrMemory_Array_DeleteElement(ioCollection->categories, i);
		}
		else if (categories[i].parent_id == inCategoryID)
		{
			OSrCollection_Category_Delete(ioCollection, categories[i].id);
		}
	}

	// delete the items in the category
	items = (OStItem*)UUrMemory_Array_GetMemory(ioCollection->items);
	num_items = UUrMemory_Array_GetUsedElems(ioCollection->items);
	for (i = (num_items - 1); i < num_items ; i--)
	{
		if (items[i].category_id == inCategoryID)
		{
			// delete the item's data
			ioCollection->item_delete_proc(items[i].id);

			// delete the item from the collection
			UUrMemory_Array_DeleteElement(ioCollection->items, i);
		}
	}

	// the collection is now dirty
	ioCollection->flags |= OScCollectionFlag_Dirty;
}

// ----------------------------------------------------------------------
OStCategory*
OSrCollection_Category_GetByID(
	OStCollection				*inCollection,
	UUtUns32					inCategoryID)
{
	OStCategory					*categories;
	OStCategory					*category;
	UUtUns32					i;
	UUtUns32					num_elements;

	UUmAssert(inCollection);

	// get the category array
	categories = (OStCategory*)UUrMemory_Array_GetMemory(inCollection->categories);
	if (categories == NULL) { return NULL; }

	// find the category with inCategoryID in the array
	category = NULL;
	num_elements = UUrMemory_Array_GetUsedElems(inCollection->categories);
	for (i = 0; i < num_elements; i++)
	{
		if (categories[i].id == inCategoryID)
		{
			category = &categories[i];
			break;
		}
	}

	return category;
}

// ----------------------------------------------------------------------
OStCategory*
OSrCollection_Category_GetByIndex(
	OStCollection				*inCollection,
	UUtUns32					inCategoryIndex)
{
	OStCategory					*categories;

	UUmAssert(inCollection);

	// make sure inCategoryIndex is in range
	if (inCategoryIndex >= UUrMemory_Array_GetUsedElems(inCollection->categories))
	{
		return NULL;
	}

	// get a pointer to the categories array
	categories = (OStCategory*)UUrMemory_Array_GetMemory(inCollection->categories);
	if (categories == NULL) { return NULL; }

	// return the categery
	return &categories[inCategoryIndex];
}

// ----------------------------------------------------------------------
UUtError
OSrCollection_Category_New(
	OStCollection				*ioCollection,
	UUtUns32					inParentCategoryID,
	const char					*inCategoryName,
	UUtUns32					*outCategoryID)
{
	UUtError					error;
	OStCategory					*categories;
	UUtUns32					i;
	UUtUns32					index;
	UUtBool						mem_moved;
	UUtUns32					num_categories;

	UUmAssert(ioCollection);
	UUmAssert(inCategoryName && (inCategoryName[0] != '\0'));

	if ((ioCollection->flags & OScCollectionFlag_Locked) != 0) { return UUcError_Generic; }

	*outCategoryID = UUcMaxUns32;

	num_categories = UUrMemory_Array_GetUsedElems(ioCollection->categories);
	if (num_categories > 0)
	{
		// get a pointer to the array's memory
		categories = (OStCategory*)UUrMemory_Array_GetMemory(ioCollection->categories);
		UUmAssert(categories);

		// if this category name already exists in the parent category, then
		// return its id number
		for (i = 0; i < num_categories; i++)
		{
			if ((categories[i].parent_id == inParentCategoryID) &&
				(UUrString_Compare_NoCase(categories[i].name, inCategoryName) == 0))
			{
				return UUcError_Generic;
			}
		}
	}

	// add the item to the categories array
	error = UUrMemory_Array_GetNewElement(ioCollection->categories, &index, &mem_moved);
	UUmError_ReturnOnError(error);

	// get a pointer to the array's memory
	categories = (OStCategory*)UUrMemory_Array_GetMemory(ioCollection->categories);
	UUmAssert(categories);

	// initialize the category
	categories[index].parent_id = inParentCategoryID;
	categories[index].id = OSiCollection_GetNextCategoryID(ioCollection);
	UUrString_Copy(categories[index].name, inCategoryName, BFcMaxFileNameLength);

	*outCategoryID = categories[index].id;

	// the collection is now dirty
	ioCollection->flags |= OScCollectionFlag_Dirty;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OSrCollection_Category_SetName(
	OStCollection				*ioCollection,
	UUtUns32					inCategoryID,
	const char					*inCategoryName)
{
	OStCategory					*category;
	UUtUns32					num_categories;

	UUmAssert(ioCollection);
	UUmAssert(inCategoryName);

	// make sure the item can be changed
	if ((ioCollection->flags & OScCollectionFlag_Locked) != 0) { return UUcError_Generic; }

	// get a pointer to the category
	category = OSrCollection_Category_GetByID(ioCollection, inCategoryID);
	if (category == NULL) { return UUcError_None; }

	// make sure that no other categories with this name exist
	num_categories = UUrMemory_Array_GetUsedElems(ioCollection->categories);
	if (num_categories > 0)
	{
		OStCategory				*categories;
		UUtUns32				i;

		// get a pointer to the array's memory
		categories = (OStCategory*)UUrMemory_Array_GetMemory(ioCollection->categories);
		UUmAssert(categories);

		// if this category name already exists in the parent category, then
		// return its id number
		for (i = 0; i < num_categories; i++)
		{
			if (category == &categories[i]) { continue; }

			if ((categories[i].parent_id == category->parent_id) &&
				(UUrString_Compare_NoCase(categories[i].name, inCategoryName) == 0))
			{
				return UUcError_Generic;
			}
		}
	}

	// change the category name
	UUrString_Copy(category->name, inCategoryName, OScMaxNameLength);

	// the collection is now dirty
	ioCollection->flags |= OScCollectionFlag_Dirty;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OSiCollection_Category_WriteByIndex(
	OStCollection				*ioCollection,
	UUtUns32					inCategoryIndex,
	UUtUns8						*ioBuffer,
	UUtUns32					*ioNumBytes)
{
	OStCategory					*category;
	UUtUns32					bytes_avail;
	UUtUns32					bytes_used;

	// make sure that inItemIndex is in range
	if (inCategoryIndex > UUrMemory_Array_GetUsedElems(ioCollection->categories))
	{
		// set the number of bytes written
		*ioNumBytes = 0;
		return;
	}

	// make sure there is enough room to write the category
	bytes_avail = *ioNumBytes;
	bytes_used = sizeof(OStCategory);
	if (bytes_avail < bytes_used)
	{
		// set the number of bytes written
		*ioNumBytes = 0;
		return;
	}

	// get a pointer to the category
	category = OSrCollection_Category_GetByIndex(ioCollection, inCategoryIndex);

	// write the category
	OBDmWrite4BytesToBuffer(ioBuffer, category->id, UUtUns32, bytes_avail, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(ioBuffer, category->parent_id, UUtUns32, bytes_avail, OBJcWrite_Little);
	UUrString_Copy((char*)ioBuffer, category->name, OScMaxNameLength);
	ioBuffer += OScMaxNameLength;
	bytes_avail -= OScMaxNameLength;

	// calculate the number of bytes written
	*ioNumBytes = *ioNumBytes - bytes_avail;
	UUmAssert(*ioNumBytes == bytes_used);
}

// ----------------------------------------------------------------------
static UUtError
OSiCollection_DeleteAll(
	OStCollection				*ioCollection)
{
	OStItem						*item_array;
	UUtUns32					i;
	UUtUns32					num_elements;

	// delete all of the items
	item_array = (OStItem*)UUrMemory_Array_GetMemory(ioCollection->items);
	num_elements = UUrMemory_Array_GetUsedElems(ioCollection->items);
	for (i = 0; i < num_elements; i++)
	{
		ioCollection->item_delete_proc(item_array[i].id);
	}

	// delete the arrays
	UUrMemory_Array_Delete(ioCollection->categories);
	UUrMemory_Array_Delete(ioCollection->items);

	// allocate new arrays
	ioCollection->categories =
		UUrMemory_Array_New(
			sizeof(OStCategory),
			1,
			0,
			5);
	ioCollection->items =
		UUrMemory_Array_New(
			sizeof(OStItem),
			5,
			0,
			0);

	UUmError_ReturnOnNull(ioCollection->categories);
	UUmError_ReturnOnNull(ioCollection->items);

	return UUcError_None;
}

// ----------------------------------------------------------------------
OStCollection*
OSrCollection_GetByType(
	OStCollectionType			inCollectionType)
{
	OStCollection				*collections;
	UUtUns32					i;
	UUtUns32					num_elements;

	// get a pointer to the collections
	collections = (OStCollection*)UUrMemory_Array_GetMemory(OSgCollections);
	if (collections == NULL) { return NULL; }

	// find a collection where type == inType
	num_elements = UUrMemory_Array_GetUsedElems(OSgCollections);
	for (i = 0; i < num_elements; i++)
	{
		if (collections[i].type == inCollectionType) { return &collections[i]; }
	}

	return NULL;
}

// ----------------------------------------------------------------------
UUtUns32
OSrCollection_GetNumCategories(
	const OStCollection			*inCollection)
{
	UUmAssert(inCollection);
	return UUrMemory_Array_GetUsedElems(inCollection->categories);
}

// ----------------------------------------------------------------------
UUtUns32
OSrCollection_GetNumItems(
	const OStCollection			*inCollection)
{
	UUmAssert(inCollection);
	return UUrMemory_Array_GetUsedElems(inCollection->items);
}

// ----------------------------------------------------------------------
OStCollectionType
OSrCollection_GetType(
	const OStCollection			*inCollection)
{
	UUmAssert(inCollection);
	return inCollection->type;
}

// ----------------------------------------------------------------------
UUtBool
OSrCollection_IsDirty(
	const OStCollection			*inCollection)
{
	return ((inCollection->flags & OScCollectionFlag_Dirty) != 0);
}

// ----------------------------------------------------------------------
UUtBool
OSrCollection_IsLocked(
	const OStCollection			*inCollection)
{
	return ((inCollection->flags & OScCollectionFlag_Locked) != 0);
}

// ----------------------------------------------------------------------
static UUtError
OSiCollection_Item_CreateFromBuffer(
	OStCollection				*ioCollection,
	UUtUns32					inVersion,
	UUtBool						inSwapIt,
	UUtUns8						*inBuffer,
	UUtUns32					*outNumBytesRead)
{
	UUtError					error;
	UUtUns32					id;
	UUtUns32					category_id;
	char						*name;
	UUtUns32					bytes_read;

	// init the outgoing value
	*outNumBytesRead = 0;

	// read the OStItem
	OBDmGet4BytesFromBuffer(inBuffer, id, UUtUns32, inSwapIt);
	OBDmGet4BytesFromBuffer(inBuffer, category_id, UUtUns32, inSwapIt);
	name = (char*)inBuffer; inBuffer += OScMaxNameLength;

	bytes_read = sizeof(OStItem);

	// create the OStItem
	error = OSrCollection_Item_New(ioCollection, category_id, id, name);
	UUmError_ReturnOnError(error);

	// read the SSt____ data
	switch (OSrCollection_GetType(ioCollection))
	{
		case OScCollectionType_Group:
		{
			UUtUns32			num_permutations;
			UUtMemory_Array		*permutations;
			SStPermutation		*perm_array;
			UUtUns32			i;
			SStGroup			*group;

			// get the number of permutations
			OBDmGet4BytesFromBuffer(inBuffer, num_permutations, UUtUns32, inSwapIt);
			bytes_read += sizeof(UUtUns32);

			// create a memory array to hold the permutations
			permutations =
				UUrMemory_Array_New(
					sizeof(SStPermutation),
					1,
					num_permutations,
					num_permutations);
			UUmError_ReturnOnNull(permutations);

			// read the permutations
			perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(permutations);
			for (i = 0; i < num_permutations; i++)
			{
				// read the permutation data into the array
				OBDmGet4BytesFromBuffer(inBuffer, perm_array[i].weight, UUtUns32, inSwapIt);
				OBDmGet4BytesFromBuffer(inBuffer, perm_array[i].min_volume_percent, float, inSwapIt);
				OBDmGet4BytesFromBuffer(inBuffer, perm_array[i].max_volume_percent, float, inSwapIt);
				OBDmGet4BytesFromBuffer(inBuffer, perm_array[i].min_pitch_percent, float, inSwapIt);
				OBDmGet4BytesFromBuffer(inBuffer, perm_array[i].max_pitch_percent, float, inSwapIt);
				UUrString_Copy(perm_array[i].sound_data_name, (char*)inBuffer, BFcMaxFileNameLength);
				inBuffer += BFcMaxFileNameLength;

				// get a pointer to the sound buffer
				perm_array[i].sound_data = NULL;
				perm_array[i].sound_data =
					SSrSoundData_GetByName(
						perm_array[i].sound_data_name,
						UUcTrue);

				bytes_read += (sizeof(UUtUns32) * 5) + BFcMaxFileNameLength;
			}

			// create a new SStGroup
			error = SSrGroup_New(NULL, &group);
			UUmError_ReturnOnError(error);

			// dispose of the permutations array in the new group
			UUrMemory_Array_Delete(group->permutations);

			// set the fields of the group
			group->id = id;
//			group->played_sound = 0;
			group->num_channels = 0;
			group->permutations = permutations;
			if ((num_permutations > 0) && (perm_array[0].sound_data != NULL))
			{
				group->num_channels = perm_array[0].sound_data->num_channels;
			}
		}
		break;

		case OScCollectionType_Impulse:
		{
			UUtUns32			impulse_sound_id;
			SStPriority2		priority;
			float				max_vol_dist;
			float				min_vol_dist;
			float				max_vol_angle;
			float				min_vol_angle;
			float				min_angle_attn;
			SStImpulse			*impulse;

			// read the OStImpulse data
			OBDmGet4BytesFromBuffer(inBuffer, impulse_sound_id, UUtUns32, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, priority, SStPriority2, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, max_vol_dist, float, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, min_vol_dist, float, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, max_vol_angle, float, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, min_vol_angle, float, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, min_angle_attn, float, inSwapIt);

			bytes_read += sizeof(UUtUns32) + sizeof(SStPriority2) + (sizeof(float) * 5);

			// create a new SStImpulse
			error = SSrImpulse_New(NULL, &impulse);
			UUmError_ReturnOnError(error);

			// set the fields of the impulse sound
			impulse->id = id;
			impulse->impulse_sound = NULL;
			impulse->impulse_sound_id = impulse_sound_id;
			impulse->priority = priority;
			impulse->max_volume_distance = max_vol_dist;
			impulse->min_volume_distance = min_vol_dist;
			impulse->max_volume_angle = max_vol_angle;
			impulse->min_volume_angle = min_vol_angle;
			impulse->min_angle_attenuation = min_angle_attn;
		}
		break;

		case OScCollectionType_Ambient:
		{
			SStPriority2		priority;
			UUtUns32			flags;
			float				sphere_radius;
			float				min_detail_time;
			float				max_detail_time;
			float				max_volume_distance;
			float				min_volume_distance;
			UUtUns32			detail_id;
			UUtUns32			base_track1_id;
			UUtUns32			base_track2_id;
			UUtUns32			in_sound_id;
			UUtUns32			out_sound_id;
			SStAmbient			*ambient;

			// read the OStAmbient from the buffer
			OBDmGet4BytesFromBuffer(inBuffer, priority, SStPriority2, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, flags, UUtUns32, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, sphere_radius, float, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, min_detail_time, float, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, max_detail_time, float, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, max_volume_distance, float, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, min_volume_distance, float, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, detail_id, UUtUns32, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, base_track1_id, UUtUns32, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, base_track2_id, UUtUns32, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, in_sound_id, UUtUns32, inSwapIt);
			OBDmGet4BytesFromBuffer(inBuffer, out_sound_id, UUtUns32, inSwapIt);

			bytes_read +=
				(sizeof(SStPriority2) +
				sizeof(UUtUns32) +
				(sizeof(float) * 5) +
				(sizeof(UUtUns32) * 5));

			// make a new SStAmbient
			error = SSrAmbient_New(NULL, &ambient);
			UUmError_ReturnOnError(error);

			// set the fields of the ambient sound
			ambient->id = id;
			ambient->priority = priority;
			ambient->flags = flags;
			ambient->sphere_radius = sphere_radius;
			ambient->min_detail_time = min_detail_time;
			ambient->max_detail_time = max_detail_time;
			ambient->max_volume_distance = max_volume_distance;
			ambient->min_volume_distance = min_volume_distance;
			ambient->detail_id = detail_id;
			ambient->base_track1_id = base_track1_id;
			ambient->base_track2_id = base_track2_id;
			ambient->in_sound_id = in_sound_id;
			ambient->out_sound_id = out_sound_id;
		}
		break;
	}

	// set the outgoing size
	*outNumBytesRead = bytes_read;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrCollection_Item_Delete(
	OStCollection				*ioCollection,
	UUtUns32					inItemID)
{
	UUtUns32					i;
	OStItem						*items;
	UUtUns32					num_elements;

	UUmAssert(ioCollection);

	// get a pointer to the items
	items = (OStItem*)UUrMemory_Array_GetMemory(ioCollection->items);
	if (items == NULL) { return; }

	// find the item in the items array and delete it
	num_elements = UUrMemory_Array_GetUsedElems(ioCollection->items);
	for (i = 0; i < num_elements; i++)
	{
		if (items[i].id == inItemID)
		{
			// delete the item
			ioCollection->item_delete_proc(inItemID);

			// delete the item from the items array
			UUrMemory_Array_DeleteElement(ioCollection->items, i);

			break;
		}
	}

	// the collection is now dirty
	ioCollection->flags |= OScCollectionFlag_Dirty;
}

// ----------------------------------------------------------------------
OStItem*
OSrCollection_Item_GetByID(
	const OStCollection			*inCollection,
	UUtUns32					inItemID)
{
	UUtUns32					i;
	OStItem						*items;
	UUtUns32					num_elements;

	UUmAssert(inCollection);

	// get a pointer to the items
	items = (OStItem*)UUrMemory_Array_GetMemory(inCollection->items);
	if (items == NULL) { return NULL; }

	// find the itme in the items array
	num_elements = UUrMemory_Array_GetUsedElems(inCollection->items);
	for (i = 0; i < num_elements; i++)
	{
		if (items[i].id == inItemID)
		{
			return &items[i];
		}
	}

	return NULL;
}

// ----------------------------------------------------------------------
OStItem*
OSrCollection_Item_GetByIndex(
	const OStCollection			*inCollection,
	UUtUns32					inItemIndex)
{
	OStItem						*items;

	UUmAssert(inCollection);

	// make sure inItemIndex is in range
	if (inItemIndex >= UUrMemory_Array_GetUsedElems(inCollection->items))
	{
		return NULL;
	}

	// get a pointer to the items array
	items = (OStItem*)UUrMemory_Array_GetMemory(inCollection->items);
	if (items == NULL) { return NULL; }

	// return the item
	return &items[inItemIndex];
}

// ----------------------------------------------------------------------
OStItem*
OSrCollection_Item_GetByName(
	const OStCollection			*inCollection,
	const char					*inItemName)
{
	UUtUns32					i;
	UUtUns32					num_items;
	OStItem						*item_array;
	OStItem						*found_item;

	found_item = NULL;
	num_items = UUrMemory_Array_GetUsedElems(inCollection->items);
	item_array = (OStItem*)UUrMemory_Array_GetMemory(inCollection->items);
	for (i = 0; i < num_items; i++)
	{
		UUtInt32				result;

		result = UUrString_Compare_NoCase(item_array[i].name, inItemName);
		if (result == 0)
		{
			found_item = &item_array[i];
			break;
		}
	}

	return found_item;
}

// ----------------------------------------------------------------------
UUtError
OSrCollection_Item_New(
	OStCollection				*ioCollection,
	UUtUns32					inCategoryID,
	UUtUns32					inItemID,
	char						*inItemName)
{
	UUtError					error;
	OStItem						*items;
	UUtUns32					i;
	UUtUns32					index;
	UUtBool						mem_moved;
	UUtUns32					num_items;

	UUmAssert(ioCollection);
	UUmAssert(inItemName);

	// check for an existing item with the same name
	num_items = UUrMemory_Array_GetUsedElems(ioCollection->items);
	if (num_items > 0)
	{
		// get a pointer to the items array
		items = (OStItem*)UUrMemory_Array_GetMemory(ioCollection->items);
		UUmAssert(items);

		// if the item name already exists in the parent category, then return its id number
		for (i = 0; i < num_items; i++)
		{
			if ((items[i].category_id == inCategoryID) &&
				(UUrString_Compare_NoCase(items[i].name, inItemName) == 0))
			{
				return UUcError_Generic;
			}
		}
	}

	// add the item to the items array
	error = UUrMemory_Array_GetNewElement(ioCollection->items, &index, &mem_moved);
	UUmError_ReturnOnError(error);

	// get a pointer to the items array
	items = (OStItem*)UUrMemory_Array_GetMemory(ioCollection->items);
	UUmAssert(items);

	// initialize the item
	items[index].id = inItemID;
	items[index].category_id = inCategoryID;
	UUrString_Copy(items[index].name, inItemName, OScMaxNameLength);

	// the collection is now dirty
	ioCollection->flags |= OScCollectionFlag_Dirty;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OSrCollection_Item_SetName(
	OStCollection				*ioCollection,
	UUtUns32					inItemID,
	const char					*inItemName)
{
	OStItem						*item;

	UUmAssert(ioCollection);
	UUmAssert(inItemName);

	// make sure the item can be changed
	if ((ioCollection->flags & OScCollectionFlag_Locked) != 0) { return UUcError_Generic; }

	// get a pointer to the item
	item = OSrCollection_Item_GetByID(ioCollection, inItemID);
	if (item == NULL) { return UUcError_None; }

	// change the item name
	UUrString_Copy(item->name, inItemName, OScMaxNameLength);

	// the collection is now dirty
	ioCollection->flags |= OScCollectionFlag_Dirty;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OSiCollection_Item_WriteByIndex(
	OStCollection				*ioCollection,
	UUtUns32					inItemIndex,
	UUtUns8						*ioBuffer,
	UUtUns32					*ioNumBytes)
{
	OStItem						*item;
	UUtUns32					bytes_avail;
	UUtUns32					bytes_used;

	// make sure that inItemIndex is in range
	if (inItemIndex > UUrMemory_Array_GetUsedElems(ioCollection->items))
	{
		// set the number of bytes written
		*ioNumBytes = 0;
		return;
	}

	// get a pointer to the item
	item = OSrCollection_Item_GetByIndex(ioCollection, inItemIndex);

	// write the SSt_____ data
	switch (OSrCollection_GetType(ioCollection))
	{
		case OScCollectionType_Group:
		{
			SStGroup					*group;
			UUtUns32					num_permutations;
			SStPermutation				*perm_array;
			UUtUns32					i;

			// get a pointer to the group
			group = SSrGroup_GetByID(item->id);

			// get the number of permutations
			num_permutations = UUrMemory_Array_GetUsedElems(group->permutations);

			// don't write into the buffer if there isn't enough space
			bytes_avail = *ioNumBytes;
			bytes_used = OScGroupWriteSize(num_permutations);
			if (bytes_avail < bytes_used)
			{
				// set the number of bytes written
				*ioNumBytes = 0;
				return;
			}

			// write the OStItem
			OBDmWrite4BytesToBuffer(ioBuffer, item->id, UUtUns32, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, item->category_id, UUtUns32, bytes_avail, OBJcWrite_Little);
			UUrString_Copy((char*)ioBuffer, item->name, OScMaxNameLength);
			ioBuffer += OScMaxNameLength;
			bytes_avail -= OScMaxNameLength;

			// write the number of permutations
			OBDmWrite4BytesToBuffer(ioBuffer, num_permutations, UUtUns32, bytes_avail, OBJcWrite_Little);

			// write the permutations
			perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(group->permutations);
			for (i = 0; i < num_permutations; i++)
			{
				SStPermutation			*perm;

				perm = &perm_array[i];

				// write the permutation
				OBDmWrite4BytesToBuffer(ioBuffer, perm->weight, UUtUns32, bytes_avail, OBJcWrite_Little);
				OBDmWrite4BytesToBuffer(ioBuffer, perm->min_volume_percent, float, bytes_avail, OBJcWrite_Little);
				OBDmWrite4BytesToBuffer(ioBuffer, perm->max_volume_percent, float, bytes_avail, OBJcWrite_Little);
				OBDmWrite4BytesToBuffer(ioBuffer, perm->min_pitch_percent, float, bytes_avail, OBJcWrite_Little);
				OBDmWrite4BytesToBuffer(ioBuffer, perm->max_pitch_percent, float, bytes_avail, OBJcWrite_Little);

				// write the SStSoundData's name
				UUrString_Copy(
					(char*)ioBuffer,
					perm->sound_data_name,
					BFcMaxFileNameLength);
				ioBuffer += BFcMaxFileNameLength;
				bytes_avail -= BFcMaxFileNameLength;
			}
		}
		break;

		case OScCollectionType_Impulse:
		{
			SStImpulse					*impulse;

			impulse = SSrImpulse_GetByID(item->id);

			// don't write into the buffer if there isn't enough space
			bytes_avail = *ioNumBytes;
			bytes_used = OScImpulseWriteSize;
			if (bytes_avail < bytes_used)
			{
				// set the number of bytes written
				*ioNumBytes = 0;
				return;
			}

			// write the item data
			OBDmWrite4BytesToBuffer(ioBuffer, item->id, UUtUns32, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, item->category_id, UUtUns32, bytes_avail, OBJcWrite_Little);
			UUrString_Copy((char*)ioBuffer, item->name, OScMaxNameLength);
			ioBuffer += OScMaxNameLength;
			bytes_avail -= OScMaxNameLength;

			// write the impulse sound
			OBDmWrite4BytesToBuffer(ioBuffer, impulse->impulse_sound_id, UUtUns32, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, impulse->priority, SStPriority2, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, impulse->max_volume_distance, float, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, impulse->min_volume_distance, float, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, impulse->max_volume_angle, float, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, impulse->min_volume_angle, float, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, impulse->min_angle_attenuation, float, bytes_avail, OBJcWrite_Little);
		}
		break;

		case OScCollectionType_Ambient:
		{
			SStAmbient					*ambient;

			ambient = SSrAmbient_GetByID(item->id);

			// don't write into the buffer if there isn't enough space
			bytes_avail = *ioNumBytes;
			bytes_used = OScAmbientWriteSize;
			if (bytes_avail < bytes_used)
			{
				// set the number of bytes written
				*ioNumBytes = 0;
				return;
			}

			// write the item data
			OBDmWrite4BytesToBuffer(ioBuffer, item->id, UUtUns32, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, item->category_id, UUtUns32, bytes_avail, OBJcWrite_Little);
			UUrString_Copy((char*)ioBuffer, item->name, OScMaxNameLength);
			ioBuffer += OScMaxNameLength;
			bytes_avail -= OScMaxNameLength;

			// write the ambient sound
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->priority, SStPriority2, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->flags, UUtUns32, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->sphere_radius, float, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->min_detail_time, float, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->max_detail_time, float, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->max_volume_distance, float, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->min_volume_distance, float, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->detail_id, UUtUns32, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->base_track1_id, UUtUns32, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->base_track2_id, UUtUns32, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->in_sound_id, UUtUns32, bytes_avail, OBJcWrite_Little);
			OBDmWrite4BytesToBuffer(ioBuffer, ambient->out_sound_id, UUtUns32, bytes_avail, OBJcWrite_Little);
		}
		break;
	}

	*ioNumBytes = *ioNumBytes - bytes_avail;
	UUmAssert(*ioNumBytes == bytes_used);
}

// ----------------------------------------------------------------------
void
OSrCollection_SetDirty(
	OStCollection				*ioCollection,
	UUtBool						inDirty)
{
	if (inDirty == UUcTrue)
	{
		ioCollection->flags |= OScCollectionFlag_Dirty;
	}
	else
	{
		ioCollection->flags &= ~OScCollectionFlag_Dirty;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OSiBinaryData_Load(
	const char				*inIdentifier,
	BDtData					*ioBinaryData,
	UUtBool					inSwapIt,
	UUtBool					inLocked,
	UUtBool					inAllocated)
{
	UUtError				error;
	OStCollectionType		collection_type;
	OStCollection			*collection;
	UUtUns8					*buffer;
	UUtUns32				buffer_size;
	UUtUns32				version;
	UUtUns32				num_categories;
	UUtUns32				num_items;
	UUtUns32				i;
	UUtUns32				read_bytes;

	UUmAssert(inIdentifier);
	UUmAssert(ioBinaryData);

	buffer = ioBinaryData->data;
	buffer_size = ioBinaryData->header.data_size;

	// read the version number
	OBDmGet4BytesFromBuffer(buffer, version, UUtUns32, inSwapIt);

	// read the collection type
	OBDmGet4BytesFromBuffer(buffer, collection_type, OStCollectionType, inSwapIt);
	collection = OSrCollection_GetByType(collection_type);
	if (collection == NULL) { return UUcError_Generic; }

	// delete all current items in the collection
	OSiCollection_DeleteAll(collection);

	// read the number of categories
	OBDmGet4BytesFromBuffer(buffer, num_categories, UUtUns32, inSwapIt);

	// read the number of items
	OBDmGet4BytesFromBuffer(buffer, num_items, UUtUns32, inSwapIt);

	// process the categories
	for (i = 0; i < num_categories; i++)
	{
		UUtUns8					*cat_data;
		UUtUns32				cat_data_length;

		// find the tags
		error =
			UUrFindTagData(
				buffer,
				buffer_size,
				UUcFile_LittleEndian,
				OScCatTag,
				&cat_data,
				&cat_data_length);
		UUmError_ReturnOnError(error);

		// process the data chunk
		error =
			OSiCollection_Category_CreateFromBuffer(
				collection,
				version,
				inSwapIt,
				cat_data,
				&read_bytes);
		UUmError_ReturnOnError(error);
		UUmAssert(read_bytes == cat_data_length);

		// advance to next data chunk
		buffer = cat_data + cat_data_length;
	}

	// process the items
	for (i = 0; i < num_items; i++)
	{
		UUtUns8					*item_data;
		UUtUns32				item_data_length;

		// find the tags
		error =
			UUrFindTagData(
				buffer,
				buffer_size,
				UUcFile_LittleEndian,
				OScItemTag,
				&item_data,
				&item_data_length);
		UUmError_ReturnOnError(error);

		// process the data chunk
		error =
			OSiCollection_Item_CreateFromBuffer(
				collection,
				version,
				inSwapIt,
				item_data,
				&read_bytes);
		UUmError_ReturnOnError(error);
		UUmAssert(read_bytes == item_data_length);

		// advance to next data chunk
		buffer = item_data + item_data_length;
	}

	// update all of the pointers for the groups
	SSrImpulse_UpdateGroupPointers(SScInvalidID);
	SSrAmbient_UpdateGroupPointers(SScInvalidID);

	// set the locked and dirty flags
	collection->flags = OScCollectionFlag_None;
	if (inLocked) { collection->flags |= OScCollectionFlag_Locked; }

	if (inAllocated)
	{
		UUrMemory_Block_Delete(ioBinaryData);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiBinaryData_Register(
	void)
{
	UUtError				error;
	BDtMethods				methods;

	methods.rLoad = OSiBinaryData_Load;

	error =	BDrRegisterClass(OScBinaryDataClass, &methods);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiBinaryData_Save(
	void)
{
	UUtUns32						i;
	OStCollection					*collection_array;
	UUtUns32						num_elements;

	// save all of the collections
	collection_array = (OStCollection*)UUrMemory_Array_GetMemory(OSgCollections);
	num_elements = UUrMemory_Array_GetUsedElems(OSgCollections);
	for (i = 0; i < num_elements; i++)
	{
		OStCollection					*collection;
		UUtUns32						num_categories;
		UUtUns32						num_items;
		UUtUns32						data_size;
		UUtUns8							*data;
		UUtUns8							*buffer;
		UUtUns32						num_bytes;
		UUtUns32						j;
		UUtUns8							temp_buffer[OScMaxBufferSize];	/* probably not 4 bytes aligned */
		UUtUns32						temp_num_bytes;
		UUtUns32						bytes_written;
		char							*name;

		collection = &collection_array[i];
		if (OSrCollection_IsLocked(collection) == UUcTrue) { continue; }
		if (OSrCollection_IsDirty(collection) == UUcFalse) { continue; }

		// get the number of categories and items
		num_categories = UUrMemory_Array_GetUsedElems(collection->categories);
		num_items = UUrMemory_Array_GetUsedElems(collection->items);

		// determine the number of bytes to write
		data_size =
			sizeof(UUtUns32) +							/* version number */
			sizeof(UUtUns32) +							/* collection type */
			sizeof(UUtUns32) +							/* number of categories */
			sizeof(UUtUns32) +							/* number of items */
			OSiCollection_CalcMemoryUsed(collection);	/* size of categories and items */

		data = UUrMemory_Block_NewClear(data_size);
		if (data == NULL)
		{
			UUmAssert(!"Unable to allocate memory to save sounds");
			continue;
		}

		// set write variables
		buffer = data;
		num_bytes = data_size;

		// write the version
		OBDmWrite4BytesToBuffer(buffer, OScCurrentVersion, UUtUns32, num_bytes, OBJcWrite_Little);

		// write the collection type
		OBDmWrite4BytesToBuffer(
			buffer,
			OSrCollection_GetType(collection),
			OStCollectionType,
			num_bytes,
			OBJcWrite_Little);

		// write the number of categories
		OBDmWrite4BytesToBuffer(buffer, num_categories, UUtUns32, num_bytes, OBJcWrite_Little);

		// write the number of items
		OBDmWrite4BytesToBuffer(buffer, num_items, UUtUns32, num_bytes, OBJcWrite_Little);

		// write the categories
		for (j = 0; j < num_categories; j++)
		{
			UUrMemory_Clear(temp_buffer, OScMaxBufferSize);
			temp_num_bytes = OScMaxBufferSize;

			// write the category
			OSiCollection_Category_WriteByIndex(
				collection,
				j,
				temp_buffer,
				&temp_num_bytes);
			temp_num_bytes = ((temp_num_bytes + 3) & ~3);

			// write the item marker
			bytes_written =
				UUrWriteTagDataToBuffer(
					buffer,
					num_bytes,
					OScCatTag,
					temp_buffer,
					temp_num_bytes,
					UUcFile_LittleEndian);

			// advance the buffer
			buffer += bytes_written;
			num_bytes -= bytes_written;
		}

		// write the items
		for (j = 0; j < num_items; j++)
		{
			UUrMemory_Clear(temp_buffer, OScMaxBufferSize);
			temp_num_bytes = OScMaxBufferSize;

			// write the item
			OSiCollection_Item_WriteByIndex(
				collection,
				j,
				temp_buffer,
				&temp_num_bytes);
			temp_num_bytes = ((temp_num_bytes + 3) & ~3);

			// write the item marker
			bytes_written =
				UUrWriteTagDataToBuffer(
					buffer,
					num_bytes,
					OScItemTag,
					temp_buffer,
					temp_num_bytes,
					UUcFile_LittleEndian);

			// advance the buffer
			buffer += bytes_written;
			num_bytes -= bytes_written;
		}

		switch (OSrCollection_GetType(collection))
		{
			case OScCollectionType_Group:
				name = "Group";
			break;

			case OScCollectionType_Ambient:
				name = "Ambient";
			break;

			case OScCollectionType_Impulse:
				name = "Impulse";
			break;
		}

		// write the buffer to the binary datafile
		OBDrBinaryData_Save(
			OScBinaryDataClass,
			name,
			data,
			(data_size - num_bytes),
			0,
			UUcFalse);

		// dispose of the buffer
		UUrMemory_Block_Delete(data);

		// the collection is no longer dirty
		OSrCollection_SetDirty(collection, UUcFalse);
	}

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OSiDialog_Play(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	OStCollection			*collection;
	OStItem					*item;
	SStAmbient				*ambient;

	// don't play a dialog if other dialog is already playing
	if (OSgDialog_PlayID == SScInvalidID)
	{
		// get the collection
		collection = OSrCollection_GetByType(OScCollectionType_Ambient);
		if (collection == NULL) { return UUcError_None; }

		// get the ambient sound
		item = OSrCollection_Item_GetByName(collection, inParameterList[0].val.str);
		if (item == NULL) { return UUcError_None; }

		ambient = SSrAmbient_GetByID(item->id);
		if (ambient == NULL) { return UUcError_None; }

		// start the ambient sound playing
		OSgDialog_PlayID = SSrAmbient_Start(ambient, NULL, NULL, NULL, 0.0f, 0.0f);
	}

//	*outTicksTillCompletion = 0;
//	*outStall = UUcFalse;

//	ioReturnValue->type = SLcType_Void;
//	ioReturnValue->val.i = 0;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiDialog_Play_Blocked(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	if (OSgDialog_PlayID == SScInvalidID)
	{
		OStCollection			*collection;
		OStItem					*item;
		SStAmbient				*ambient;

		// get the collection
		collection = OSrCollection_GetByType(OScCollectionType_Ambient);
		if (collection == NULL) { return UUcError_None; }

		// get the ambient sound
		item = OSrCollection_Item_GetByName(collection, inParameterList[0].val.str);
		if (item == NULL) { return UUcError_None; }

		ambient = SSrAmbient_GetByID(item->id);
		if (ambient == NULL) { return UUcError_None; }

		// start the ambient sound playing
		OSgDialog_PlayID = SSrAmbient_Start(ambient, NULL, NULL, NULL, 0.0f, 0.0f);

//		*outTicksTillCompletion = 0;
		*outStall = UUcFalse;
	}
	else
	{
//		*outTicksTillCompletion = 2;
		*outStall = UUcTrue;
	}

//	ioReturnValue->type = SLcType_Void;
//	ioReturnValue->val.i = 0;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiMusic_Start(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	OStCollection			*collection;
	OStItem					*item;
	SStAmbient				*ambient;
	UUtError				error;
	OStPlayingMusic			*playing_music_array;
	UUtUns32				index;

	// get the collection
	collection = OSrCollection_GetByType(OScCollectionType_Ambient);
	if (collection == NULL) { return UUcError_None; }

	// get the ambient sound
	item = OSrCollection_Item_GetByName(collection, inParameterList[0].val.str);
	if (item == NULL) { return UUcError_None; }

	// start the music
	ambient = SSrAmbient_GetByID(item->id);
	if (ambient == NULL) { return UUcError_None; }

	// add an element to the play list
	error = UUrMemory_Array_GetNewElement(OSgPlayingMusic, &index, NULL);
	if (error != UUcError_None) { return UUcError_None; }

	playing_music_array = (OStPlayingMusic*)UUrMemory_Array_GetMemory(OSgPlayingMusic);
	UUmAssert(playing_music_array);

	UUrString_Copy(
		playing_music_array[index].name,
		inParameterList[0].val.str,
		OScMaxNameLength);
	playing_music_array[index].play_id = SSrAmbient_Start(ambient, NULL, NULL, NULL, 0.0f, 0.0f);

/*	*outTicksTillCompletion = 0;
	*outStall = UUcFalse;
	ioReturnValue->type = SLcType_Void;
	ioReturnValue->val.i = 0;*/

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiMusic_Stop(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	OStPlayingMusic			*playing_music_array;
	UUtUns32				num_elements;
	UUtUns32				i;

	playing_music_array = (OStPlayingMusic*)UUrMemory_Array_GetMemory(OSgPlayingMusic);
	num_elements = UUrMemory_Array_GetUsedElems(OSgPlayingMusic);
	for (i = 0; i < num_elements; i++)
	{
		UUtInt32			result;

		result = UUrString_Compare_NoCase(playing_music_array[i].name, inParameterList[0].val.str);
		if (result != 0) { continue; }

		// stop the music
		SSrAmbient_Stop(playing_music_array[i].play_id);

		UUrMemory_Array_DeleteElement(OSgPlayingMusic, i);
		break;
	}

/*	*outTicksTillCompletion = 0;
	*outStall = UUcFalse;
	ioReturnValue->type = SLcType_Void;
	ioReturnValue->val.i = 0;*/

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiScriptCommands_Initialize(
	void)
{
	UUtError					error;

	// initialize the globals
	OSgDialog_PlayID = SScInvalidID;

	OSgPlayingMusic =
		UUrMemory_Array_New(
			sizeof(OStPlayingMusic*),
			5,
			0,
			1);
	UUmError_ReturnOnNull(OSgPlayingMusic);

	// initialize the commands
	error =
		SLrScript_Command_Register_Void(
			"sound_music_start",
			"function to start music playing",
			"name:string",
			OSiMusic_Start);
	UUmError_ReturnOnError(error);

	error =
		SLrScript_Command_Register_Void(
			"sound_music_stop",
			"function to start music playing",
			"name:string",
			OSiMusic_Stop);
	UUmError_ReturnOnError(error);

	error =
		SLrScript_Command_Register_Void(
			"sound_dialog_play",
			"function to start character dialog playing",
			"name:string",
			OSiDialog_Play);
	UUmError_ReturnOnError(error);

	error =
		SLrScript_Command_Register_Void(
			"sound_dialog_play_block",
			"function to start character dialog playing after the current dialog finishes",
			"name:string",
			OSiDialog_Play_Blocked);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OSiScriptCommands_Terminate(
	void)
{
	if (OSgPlayingMusic)
	{
		UUrMemory_Array_Delete(OSgPlayingMusic);
		OSgPlayingMusic = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OSiScriptCommands_Update(
	void)
{
	if (OSgDialog_PlayID != SScInvalidID)
	{
		UUtBool					active;

		active = SSrAmbient_Update(OSgDialog_PlayID, NULL, NULL, NULL);
		if (active == UUcFalse)
		{
			OSgDialog_PlayID = SScInvalidID;
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OSiSoundObject_BuildPlayList(
	OBJtObject					*inObject,
	UUtUns32					inUserData)
{
	UUtError					error;
	OBJtOSD_All					osd_all;
	UUtUns32					index;
	OStPlayingAmbient			*playing_ambient_array;

	// get the object specific data
	OBJrObject_GetObjectSpecificData(inObject, &osd_all);

	// get the ambient sound
	error = UUrMemory_Array_GetNewElement(OSgPlayingAmbient, &index, NULL);
	if (error != UUcError_None) { return UUcFalse; }

	// get a pointer to the memory array
	playing_ambient_array = (OStPlayingAmbient*)UUrMemory_Array_GetMemory(OSgPlayingAmbient);
	if (playing_ambient_array == NULL) { return UUcFalse; }

	// initialize the array element
	playing_ambient_array[index].object = inObject;
	playing_ambient_array[index].play_id = SScInvalidID;

	return UUcTrue;
}

// ----------------------------------------------------------------------
UUtError
OSrLevel_Load(
	UUtUns32					inLevelNumber)
{
	UUtError					error;

	if (inLevelNumber == 0)
	{
		error = OSrVariantList_Initialize();
		UUmError_ReturnOnError(error);
	}
	else
	{
		// create the ambient sound play list
		OSgPlayingAmbient =
			UUrMemory_Array_New(
				sizeof(OStPlayingAmbient),
				1,
				0,
				10);
		UUmError_ReturnOnNull(OSgPlayingAmbient);

		// build a list of the spatial and evironmental ambient sounds in the level
		OBJrObjectType_EnumerateObjects(
			OBJcType_Sound,
			OSiSoundObject_BuildPlayList,
			0);

		// update the variant list's sound animation pointers
		OSrVariantList_LevelLoad();
	}

	// set up all of the impact effect's impulse IDs
	ONrImpactEffects_UpdateImpulseIDs(SScInvalidID);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrLevel_Unload(
	void)
{
	UUtUns32					i;
	OStPlayingAmbient			*playing_ambient_array;
	UUtUns32					num_elements;

	// go through the ambient sound play list and stop the playing sounds
	playing_ambient_array = (OStPlayingAmbient*)UUrMemory_Array_GetMemory(OSgPlayingAmbient);
	num_elements = UUrMemory_Array_GetUsedElems(OSgPlayingAmbient);
	for (i = 0; i < num_elements; i++)
	{
		if (playing_ambient_array[i].play_id == SScInvalidID) { continue; }

		SSrAmbient_Stop(playing_ambient_array[i].play_id);
		playing_ambient_array[i].play_id = SScInvalidID;
	}

	// delete the ambient sound play list
	UUrMemory_Array_Delete(OSgPlayingAmbient);
	OSgPlayingAmbient = NULL;

	// unload the variant list
	OSrVariantList_LevelUnload();
}

// ----------------------------------------------------------------------
void
OSrUpdate(
	const M3tPoint3D			*inPosition,
	const M3tPoint3D			*inFacing)
{
	UUtUns32					i;
	OStPlayingAmbient			*playing_ambient_array;
	UUtUns32					num_elements;

	// update the script commands
	OSiScriptCommands_Update();

	// set the listener's position
	SSrListener_SetPosition(inPosition, inFacing);

	// go through the ambient sound play list and update the playing sounds
	playing_ambient_array = (OStPlayingAmbient*)UUrMemory_Array_GetMemory(OSgPlayingAmbient);
	num_elements = UUrMemory_Array_GetUsedElems(OSgPlayingAmbient);
	for (i = 0; i < num_elements; i++)
	{
		M3tPoint3D				position;
		OBJtOSD_All				osd_all;
		SStAmbient				*ambient;
		M3tVector3D				velocity;
		float					distance;

		// calculate the distance from the object to the listener
		OBJrObject_GetPosition(playing_ambient_array[i].object, &position, NULL);
		OBJrObject_GetObjectSpecificData(playing_ambient_array[i].object, &osd_all);

		// see if the ambient sound is in range to be played
		distance = MUrPoint_Distance(inPosition, &position) * SScFootToDist;
		if ((distance < osd_all.sound_osd.min_volume_distance) == UUcFalse)
		{
			// if the ambient sound was playing, then stop it
			if (playing_ambient_array[i].play_id != SScInvalidID)
			{
				SSrAmbient_Stop(playing_ambient_array[i].play_id);
				playing_ambient_array[i].play_id = SScInvalidID;
			}

			continue;
		}

		// if the ambient sound is already playing then go to the next sound
		if (playing_ambient_array[i].play_id != SScInvalidID) { continue; }

		// play the amient sound
		ambient = SSrAmbient_GetByID(osd_all.sound_osd.ambient_id);

		if (NULL != ambient)
		{
			MUmVector_Set(velocity, 0.0f, 0.0f, 0.0f);
			playing_ambient_array[i].play_id =
				SSrAmbient_Start(
					ambient,
					&position,
					inFacing,
					&velocity,
					osd_all.sound_osd.max_volume_distance,
					osd_all.sound_osd.min_volume_distance);
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OSiTextFile_WriteAmbient(
	BFtFile					*inFile)
{
	OStCollection			*ambient_collection;
	OStCollection			*group_collection;
	UUtUns32				num_items;
	UUtUns32				i;
	char					string[2048];

	// get the ambient collection
	ambient_collection = OSrCollection_GetByType(OScCollectionType_Ambient);
	if (ambient_collection == NULL) { return UUcError_None; }

	// get the group collection
	group_collection = OSrCollection_GetByType(OScCollectionType_Group);
	if (group_collection == NULL) { return UUcError_None; }

	// write the header
	sprintf(string, "Name\tGroup\tPriority\tIn Sound\tOut Sound\tBase Track 1\tBase Track 2\tDetail Track\tMin Detail Time\tMax Detail Time\tSphere Radius\tMax Volume Distance\tMin Volume Distance\tInterrupt On Stop\tPlay Once\n");
	BFrFile_Write(inFile, strlen(string), string);

	// write the items
	num_items = OSrCollection_GetNumItems(ambient_collection);
	for (i = 0; i < num_items; i++)
	{
		OStItem				*item;
		OStCategory			*category;
		SStAmbient			*ambient;
		OStItem				*group;
		char				number[128];

		item = OSrCollection_Item_GetByIndex(ambient_collection, i);
		if (item == NULL) { continue; }

		category = OSrCollection_Category_GetByID(ambient_collection, OSrItem_GetCategoryID(item));

		ambient = SSrAmbient_GetByID(item->id);
		if (ambient == NULL) { continue; }

		string[0] = '\0';

		strcat(string, OSrItem_GetName(item)); strcat(string, "\t");
		if (category) { strcat(string, OSrCategory_GetName(category)); } strcat(string, "\t");

		strcat(string, OSgPriority_Name[ambient->priority]); strcat(string, "\t");

		group = OSrCollection_Item_GetByID(group_collection, ambient->in_sound_id);
		if (group) { strcat(string, OSrItem_GetName(group)); }
		strcat(string, "\t");

		group = OSrCollection_Item_GetByID(group_collection, ambient->out_sound_id);
		if (group) { strcat(string, OSrItem_GetName(group)); }
		strcat(string, "\t");

		group = OSrCollection_Item_GetByID(group_collection, ambient->base_track1_id);
		if (group) { strcat(string, OSrItem_GetName(group)); }
		strcat(string, "\t");

		group = OSrCollection_Item_GetByID(group_collection, ambient->base_track2_id);
		if (group) { strcat(string, OSrItem_GetName(group)); }
		strcat(string, "\t");

		group = OSrCollection_Item_GetByID(group_collection, ambient->detail_id);
		if (group) { strcat(string, OSrItem_GetName(group)); }
		strcat(string, "\t");

		sprintf(number, "5.3f", ambient->min_detail_time); strcat(string, number); strcat(string, "\t");
		sprintf(number, "5.3f", ambient->max_detail_time); strcat(string, number); strcat(string, "\t");
		sprintf(number, "5.3f", ambient->sphere_radius); strcat(string, number); strcat(string, "\t");
		sprintf(number, "5.3f", ambient->max_volume_distance); strcat(string, number); strcat(string, "\t");
		sprintf(number, "5.3f", ambient->min_volume_distance); strcat(string, number); strcat(string, "\t");

		if ((ambient->flags & SScAmbientFlag_InterruptOnStop) != 0) { strcat(string, "Y"); }
		strcat(string, "\t");

		if ((ambient->flags & SScAmbientFlag_PlayOnce) != 0) { strcat(string, "Y"); }

		strcat(string, "\n");

		BFrFile_Write(inFile, strlen(string), string);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiTextFile_WriteImpulse(
	BFtFile					*inFile)
{
	OStCollection			*impulse_collection;
	OStCollection			*group_collection;
	UUtUns32				num_items;
	UUtUns32				i;
	char					string[2048];

	// get the impulse collection
	impulse_collection = OSrCollection_GetByType(OScCollectionType_Impulse);
	if (impulse_collection == NULL) { return UUcError_None; }

	// get the group collection
	group_collection = OSrCollection_GetByType(OScCollectionType_Group);
	if (group_collection == NULL) { return UUcError_None; }

	// write the header
	sprintf(string, "Name\tGroup\tPriority\tSound\tMax Volume Distance\tMin Volume Distance\n");
	BFrFile_Write(inFile, strlen(string), string);

	// write the items
	num_items = OSrCollection_GetNumItems(impulse_collection);
	for (i = 0; i < num_items; i++)
	{
		OStItem				*item;
		OStCategory			*category;
		SStImpulse			*impulse;
		OStItem				*group;
		char				number[128];

		item = OSrCollection_Item_GetByIndex(impulse_collection, i);
		if (item == NULL) { continue; }

		category = OSrCollection_Category_GetByID(impulse_collection, OSrItem_GetCategoryID(item));

		impulse = SSrImpulse_GetByID(item->id);
		if (impulse == NULL) { continue; }

		string[0] = '\0';

		strcat(string, OSrItem_GetName(item)); strcat(string, "\t");
		if (category) { strcat(string, OSrCategory_GetName(category)); } strcat(string, "\t");

		strcat(string, OSgPriority_Name[impulse->priority]); strcat(string, "\t");

		group = OSrCollection_Item_GetByID(group_collection, impulse->impulse_sound_id);
		if (group) { strcat(string, OSrItem_GetName(group)); }
		strcat(string, "\t");

		sprintf(number, "5.3f", impulse->min_volume_distance); strcat(string, number); strcat(string, "\t");
		sprintf(number, "5.3f", impulse->max_volume_distance); strcat(string, number);

		strcat(string, "\n");

		BFrFile_Write(inFile, strlen(string), string);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiTextFile_WriteGroup(
	BFtFile					*inFile)
{
	OStCollection			*group_collection;
	UUtUns32				num_items;
	UUtUns32				i;
	char					string[2048];

	// get the group collection
	group_collection = OSrCollection_GetByType(OScCollectionType_Group);
	if (group_collection == NULL) { return UUcError_None; }

	// write the header
	sprintf(string, "Name\tGroup\tFile Name\tWeight\tMin Volume Percent\tMax Volume Percent\tMin Pitch Percent\tMax Pitch Percent\n");
	BFrFile_Write(inFile, strlen(string), string);

	// write the items
	num_items = OSrCollection_GetNumItems(group_collection);
	for (i = 0; i < num_items; i++)
	{
		OStItem				*item;
		OStCategory			*category;
		SStGroup			*group;
		SStPermutation		*perm_array;
		UUtUns32			num_permutations;

		item = OSrCollection_Item_GetByIndex(group_collection, i);
		if (item == NULL) { continue; }

		category = OSrCollection_Category_GetByID(group_collection, OSrItem_GetCategoryID(item));

		group = SSrGroup_GetByID(item->id);
		if (group == NULL) { continue; }

		string[0] = '\0';

		strcat(string, OSrItem_GetName(item)); strcat(string, "\t");
		if (category) { strcat(string, OSrCategory_GetName(category)); } strcat(string, "\n");

		BFrFile_Write(inFile, strlen(string), string);

		perm_array = (SStPermutation*)UUrMemory_Array_GetMemory(group->permutations);
		num_permutations = UUrMemory_Array_GetUsedElems(group->permutations);
		for (i = 0; i < num_permutations; i++)
		{
			SStPermutation		*perm;

			perm = &perm_array[i];
			sprintf(
				string,
				"\t\t%s\t%d\t%5.3f\t%5.3f\t%5.3f\t%5.3f\n",
				perm->sound_data_name,
				perm->weight,
				perm->min_volume_percent,
				perm->max_volume_percent,
				perm->min_pitch_percent,
				perm->max_pitch_percent);
			BFrFile_Write(inFile, strlen(string), string);
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OSiTextFile_Write(
	OStCollectionType		inCollectionType)
{
	UUtError				error;
	char					filename[128];
	BFtFileRef				*file_ref;
	BFtFile					*file;

	// create the env file ref
	switch (inCollectionType)
	{
		case OScCollectionType_Ambient:
			sprintf(filename, "Sound_Ambient.txt");
		break;

		case OScCollectionType_Impulse:
			sprintf(filename, "Sound_Impulse.txt");
		break;

		case OScCollectionType_Group:
			sprintf(filename, "Sound_Group.txt");
		break;
	}
	error = BFrFileRef_MakeFromName(filename, &file_ref);
	UUmError_ReturnOnError(error);

	// create the .TXT file if it doesn't already exist
	if (BFrFileRef_FileExists(file_ref) == UUcFalse)
	{
		// create the object.env file
		error = BFrFile_Create(file_ref);
		UUmError_ReturnOnError(error);
	}

	// open the file
	error = BFrFile_Open(file_ref, "rw", &file);
	UUmError_ReturnOnError(error);

	// set the position to 0
	error = BFrFile_SetPos(file, 0);
	UUmError_ReturnOnError(error);

	// write the items
	switch (inCollectionType)
	{
		case OScCollectionType_Ambient:
			OSiTextFile_WriteAmbient(file);
		break;

		case OScCollectionType_Impulse:
			OSiTextFile_WriteImpulse(file);
		break;

		case OScCollectionType_Group:
			OSiTextFile_WriteGroup(file);
		break;
	}

	// set the end of the file
	BFrFile_SetEOF(file);

	// close the file
	BFrFile_Close(file);
	file = NULL;

	// delete the file ref
	BFrFileRef_Dispose(file_ref);
	file_ref = NULL;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OSrTextFile_Write(
	void)
{
	UUtError					error;

	error = OSiTextFile_Write(OScCollectionType_Ambient);
	UUmError_ReturnOnError(error);

	error = OSiTextFile_Write(OScCollectionType_Impulse);
	UUmError_ReturnOnError(error);

	error = OSiTextFile_Write(OScCollectionType_Group);
	UUmError_ReturnOnError(error);

	return error;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OSrInitialize(
	void)
{
	UUtError					error;

	// intialize the collections
	error = OSiCollections_Initialize();
	UUmError_ReturnOnError(error);

	// initialize the sound animations
	error = OSrSA_Initialize();
	UUmError_ReturnOnError(error);

	// initialize the script commands
	error = OSiScriptCommands_Initialize();
	UUmError_ReturnOnError(error);

	// initialize the categories
	error =
		OSiCollections_AddType(
			OScCollectionType_Group,
			(OStItemDeleteProc)SSrGroup_Delete);
	UUmError_ReturnOnError(error);

	error =
		OSiCollections_AddType(
			OScCollectionType_Ambient,
			(OStItemDeleteProc)SSrAmbient_Delete);
	UUmError_ReturnOnError(error);

	// CB: OSiImpulseSound_Delete is a wrapper for SSrImpulse_Delete that informs Oni_ImpactEffect
	// that the impulse sound is being deleted
	error =
		OSiCollections_AddType(
			OScCollectionType_Impulse,
			(OStItemDeleteProc)OSiImpulseSound_Delete);
	UUmError_ReturnOnError(error);

	// initialize the binary data class
	error = OSiBinaryData_Register();
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OSrSave(
	void)
{
	OSiBinaryData_Save();
}

// ----------------------------------------------------------------------
void
OSrTerminate(
	void)
{
	OSiCollections_Terminate();
	OSrVariantList_Terminate();
	OSiScriptCommands_Terminate();
}
