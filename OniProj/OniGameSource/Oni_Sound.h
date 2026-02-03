// ======================================================================
// Oni_Sound.h
// ======================================================================
#pragma once

#ifndef ONI_SOUND_H
#define ONI_SOUND_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"

// ======================================================================
// defines
// ======================================================================
#define OScMaxNameLength				(32)
#define OScBinaryDataClass				UUm4CharToUns32('O', 'S', 'B', 'D')

// ======================================================================
// enums
// ======================================================================
enum
{
	OScVersion_1						= 1,

	OScCurrentVersion					= OScVersion_1
};

typedef enum OStCollectionType
{
	OScCollectionType_Group				= UUm4CharToUns32('C', 'T', 'G', 'R'),
	OScCollectionType_Ambient			= UUm4CharToUns32('C', 'T', 'A', 'M'),
	OScCollectionType_Impulse			= UUm4CharToUns32('C', 'T', 'I', 'M')

} OStCollectionType;


// ======================================================================
// typedefs
// ======================================================================
typedef struct OStItem			OStItem;
typedef struct OStCategory		OStCategory;
typedef struct OStCollection	OStCollection;

typedef void
(*OStItemDeleteProc)(
	UUtUns32					inItemID);

// ======================================================================
// prototypes
// ======================================================================
// ----------------------------------------------------------------------
// Playing Sounds
// ----------------------------------------------------------------------
UUtError
OSrLevel_Load(
	UUtUns32					inLevelNumber);

void
OSrLevel_Unload(
	void);

void
OSrUpdate(
	const M3tPoint3D			*inPosition,
	const M3tPoint3D			*inFacing);

// ----------------------------------------------------------------------
// Tool Related
// ----------------------------------------------------------------------
UUtUns32
OSrItem_GetCategoryID(
	const OStItem				*inItem);

UUtUns32
OSrItem_GetID(
	const OStItem				*inItem);

const char*
OSrItem_GetName(
	const OStItem				*inItem);

// ----------------------------------------------------------------------
UUtUns32
OSrCategory_GetID(
	const OStCategory			*inCategory);

const char*
OSrCategory_GetName(
	const OStCategory			*inCategory);

UUtUns32
OSrCategory_GetParentCategoryID(
	const OStCategory			*inCategory);

// ----------------------------------------------------------------------
void
OSrCollection_Category_Delete(
	OStCollection				*ioCollection,
	UUtUns32					inCategoryID);

OStCategory*
OSrCollection_Category_GetByID(
	OStCollection				*inCollection,
	UUtUns32					inCategoryID);

OStCategory*
OSrCollection_Category_GetByIndex(
	OStCollection				*inCollection,
	UUtUns32					inCategoryIndex);

UUtError
OSrCollection_Category_New(
	OStCollection				*ioCollection,
	UUtUns32					inParentCategoryID,
	const char					*inCategoryName,
	UUtUns32					*outCategoryID);

UUtError
OSrCollection_Category_SetName(
	OStCollection				*ioCollection,
	UUtUns32					inCategoryID,
	const char					*inCategoryName);

OStCollection*
OSrCollection_GetByType(
	OStCollectionType			inCollectionType);

UUtUns32
OSrCollection_GetNumCategories(
	const OStCollection			*inCollection);

UUtUns32
OSrCollection_GetNumItems(
	const OStCollection			*inCollection);

OStCollectionType
OSrCollection_GetType(
	const OStCollection			*inCollection);

UUtBool
OSrCollection_IsDirty(
	const OStCollection			*inCollection);

UUtBool
OSrCollection_IsLocked(
	const OStCollection			*inCollection);

void
OSrCollection_Item_Delete(
	OStCollection				*ioCollection,
	UUtUns32					inItemID);

OStItem*
OSrCollection_Item_GetByID(
	const OStCollection			*inCollection,
	UUtUns32					inItemIndex);

OStItem*
OSrCollection_Item_GetByIndex(
	const OStCollection			*inCollection,
	UUtUns32					inItemIndex);

OStItem*
OSrCollection_Item_GetByName(
	const OStCollection			*inCollection,
	const char					*inItemName);

UUtError
OSrCollection_Item_New(
	OStCollection				*ioCollection,
	UUtUns32					inCategoryID,
	UUtUns32					inItemID,
	char						*inItemName);

void
OSrCollection_SetDirty(
	OStCollection				*ioCollection,
	UUtBool						inDirty);

UUtError
OSrCollection_Item_SetName(
	OStCollection				*ioCollection,
	UUtUns32					inItemID,
	const char					*inItemName);

// ----------------------------------------------------------------------
UUtError
OSrTextFile_Write(
	void);

// ----------------------------------------------------------------------
// Master
// ----------------------------------------------------------------------
UUtError
OSrInitialize(
	void);

void
OSrSave(
	void);

void
OSrTerminate(
	void);

// ======================================================================
#endif /* ONI_SOUND_H */
