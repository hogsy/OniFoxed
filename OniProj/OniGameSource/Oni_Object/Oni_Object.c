// ======================================================================
// Oni_ObjectGroup.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_BinaryData.h"
#include "BFW_Timer.h"

#include "EulerAngles.h"
#include "Oni_BinaryData.h"
#include "Oni_Object.h"
#include "Oni_Object_Private.h"
#include "WM_Menu.h"
#include "Oni_Win_Tools.h"
#include "Oni_GameStatePrivate.h"

#include "ENVFileFormat.h"
#include "Oni_Mechanics.h"

UUtInt32 OBJgNodeCount = 0;
UUtInt32 OBJgNodeActual = 0;

UUtUns32 OBJrObject_GetNextAvailableID();
// ======================================================================
// defines
// ======================================================================
#define OBJcMaxBufferSize				16 * 1024
#define OBJcParticleMarkerUserDataMax	1024

#define OBJcAutoSaveDelta				(1 * 60 * 60)	/* minutes in ticks */
#define OBJcObjectGenericWriteSize											\
		(																	\
		sizeof(OBJtObjectType) +					/* object type */		\
		sizeof(UUtUns32) +							/* object ID */			\
		sizeof(UUtUns32) +							/* object flags */		\
		sizeof(M3tPoint3D) +						/* object position */	\
		sizeof(M3tPoint3D)							/* object rotation */	\
		)

// ======================================================================
// enums
// ======================================================================
enum
{
	OBJcObjectGroupFlag_Dirty			= 0x0001,
	OBJcObjectGroupFlag_Locked			= 0x0002
	
};

// ======================================================================
// typedefs
// ======================================================================
typedef struct OBJtObjectGroup
{
	UUtMemory_Array				*object_list;
	OBJtObjectType				object_type;
	UUtUns32					object_typeindex;
	UUtUns32					flags;
	UUtUns32					osd_size_in_memory;
	OBJtMethods					methods;
	ONtMechanicsClass			*mechanics_class;
	char						group_name[OBJcMaxNameLength];

} OBJtObjectGroup;


typedef enum OBJtRotateAxis
{
	OBJcRotate_None,
	OBJcRotate_XAxis,
	OBJcRotate_YAxis,
	OBJcRotate_ZAxis
	
} OBJtRotateAxis;

// ======================================================================
// globals
// ======================================================================
static UUtUns32					OBJgNextObjectID;

static UUtMemory_Array			*OBJgObjectGroups;

static UUtMemory_Array			*OBJgSelectedObjects;

static OBJtRotateAxis			OBJgRotateAxis;

static AUtFlagElement			OBJgGunkFlags[] = 
{
	{	"ghost",				AKcGQ_Flag_Ghost	},
	{	"stairsup",				AKcGQ_Flag_SAT_Up	},
	{	"stairsdown",			AKcGQ_Flag_SAT_Down	},
	{	"stairs",				AKcGQ_Flag_Stairs	},
	{	"2sided",				AKcGQ_Flag_DrawBothSides	},
	{	"nocollision",			AKcGQ_Flag_No_Collision	},
	{	"invisible",			AKcGQ_Flag_Invisible	},
	{	"no_chr_col",			AKcGQ_Flag_No_Character_Collision,	},
	{	"no_obj_col",			AKcGQ_Flag_No_Object_Collision,	},
	{	"no_occl",				AKcGQ_Flag_No_Occlusion	},
	{	"danger",				AKcGQ_Flag_AIDanger },
	{	"grid_ignore",			AKcGQ_Flag_AIGridIgnore },
	{	"nodecal",				AKcGQ_Flag_NoDecal },
	{	"furniture",			AKcGQ_Flag_Furniture },
	{	"sound_transparent",	AKcGQ_Flag_SoundTransparent },
	{	"impassable",			AKcGQ_Flag_AIImpassable },
	{	NULL,					0	}
};

static UUtUns32					OBJgMoveMode;
static UUtUns32					OBJgRotateMode;

static UUtBool					OBJgSelectedObjects_Locked;

static UUtUns32					OBJgAutoSaveTime;

// ======================================================================
// prototypes
// ======================================================================
static UUtBool
OBJiObjectGroup_IsLocked(
	OBJtObjectGroup				*inObjectGroup);
	
static void
OBJiObjectGroup_RemoveObject(
	OBJtObjectGroup				*inObjectGroup,
	OBJtObject					*inObject);
	
static void
OBJiObjectGroup_SetDirty(
	OBJtObjectGroup				*inObjectGroup,
	UUtBool						inLocked);
	
// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OBJiObjectGroup_AddObject(
	OBJtObjectGroup				*inObjectGroup,
	OBJtObject					*inObject)
{
	UUtError					error;
	UUtUns32					index;
	UUtBool						mem_moved;
	OBJtObject					**object_list;
	
	UUmAssert(inObjectGroup);
	
	if (OBJiObjectGroup_IsLocked(inObjectGroup)) { 

#if defined(DEBUGGING) && DEBUGGING		
		UUrDebuggerMessage("failed to add object (object group %s was locked)", inObjectGroup->group_name);
#endif

		UUmAssert(!"blam");
		return UUcError_Generic; 
	}
	
	// create the object list if it doesn't already exist
	if (inObjectGroup->object_list == NULL)
	{
		inObjectGroup->object_list
			= UUrMemory_Array_New(
				sizeof(OBJtObjectType),
				5,
				0,
				1);
	}
	
	// get a new element in the object group's object list
	error = UUrMemory_Array_GetNewElement(inObjectGroup->object_list, &index, &mem_moved);
	UUmAssert(!error);
	UUmError_ReturnOnError(error);
	
	// get a pointer to the object list
	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(inObjectGroup->object_list);
	UUmAssert(object_list);
	
	// add the object to the object list
	object_list[index] = inObject;
	
	// the object group is now dirty
	OBJiObjectGroup_SetDirty(inObjectGroup, UUcTrue);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObjectGroup_DeleteAllObjects(
	OBJtObjectGroup				*inObjectGroup)
{
	// delete the object array
	if (inObjectGroup->object_list)
	{
		// delete the objects in the object array
		while (UUrMemory_Array_GetUsedElems(inObjectGroup->object_list))
		{
			OBJtObject		**object_list;
			OBJtObject		*delete_me;
			
			object_list = (OBJtObject**)UUrMemory_Array_GetMemory(inObjectGroup->object_list);
			if (object_list == NULL) { break; }
			
			delete_me = object_list[0];
			if (delete_me == NULL) { continue; }
			
			// remove the object from the object group
			OBJiObjectGroup_RemoveObject(inObjectGroup, delete_me);

			// delete the object specific data
			delete_me->methods->rDelete(delete_me);
			
			// release the memory used by the object
			UUrMemory_Block_Delete(delete_me);
		}

		// delete the object list
		UUrMemory_Array_Delete(inObjectGroup->object_list);
		inObjectGroup->object_list = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OBJiObjectGroup_Delete(
	OBJtObjectGroup				*inObjectGroup)
{
	OBJtObjectGroup				**object_group_list;
	UUtUns32					num_groups;
	UUtUns32					i;
	
	// delete the objects
	OBJiObjectGroup_DeleteAllObjects(inObjectGroup);
	
	// find the object group in the list
	object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
	num_groups = UUrMemory_Array_GetUsedElems(OBJgObjectGroups);
	for (i = 0; i < num_groups; i++)
	{
		if (object_group_list[i] == inObjectGroup)
		{
			UUrMemory_Array_DeleteElement(OBJgObjectGroups, i);
		}
	}
	
	// delete the object group
	if( inObjectGroup->mechanics_class )
		ONrMechanics_DeleteClass( inObjectGroup->mechanics_class );
	UUrMemory_Block_Delete(inObjectGroup);
}

// ----------------------------------------------------------------------
static void
OBJiObjectGroup_Draw(
	OBJtObjectGroup				*inObjectGroup)
{
	UUtUns32					num_objects;
	UUtUns32					i;
	OBJtObject					**object_list;
	
	if (inObjectGroup->object_list == NULL) { return; }
	
	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(inObjectGroup->object_list);
	if (object_list == NULL) { return; }
	
	num_objects = UUrMemory_Array_GetUsedElems(inObjectGroup->object_list);
	for (i = 0; i < num_objects; i++)
	{
		OBJrObject_Draw(object_list[i]);
	}
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiObjectGroup_EnumerateObjects(
	OBJtObjectGroup				*inObjectGroup,
	OBJtEnumCallback_Object		inEnumCallback,
	UUtUns32					inUserData)
{
	UUtUns32					num_objects;
	UUtUns32					i;
	OBJtObject					**object_list;
	
	UUmAssert(inObjectGroup);
	
	if (inObjectGroup->object_list == NULL) { return 0; }

	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(inObjectGroup->object_list);
	if (object_list == NULL) { return 0; }
	
	num_objects = UUrMemory_Array_GetUsedElems(inObjectGroup->object_list);

	if (NULL != inEnumCallback) {
		for (i = 0; i < num_objects; i++)
		{
			UUtBool					result;
			
			result = inEnumCallback(object_list[i], inUserData);
			if (result == UUcFalse) { break; }
		}
	}

	return num_objects;
}

// ----------------------------------------------------------------------
static OBJtObjectGroup*
OBJiObjectGroup_GetByName(
	const char					*inGroupName)
{
	OBJtObjectGroup				*object_group;
	OBJtObjectGroup				**object_group_list;
	
	UUmAssert(inGroupName);
	
	// intialize
	object_group = NULL;
	
	// make sure the object group list exists
	object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
	if (object_group_list)
	{
		UUtUns32				num_groups;
		UUtUns32				i;
		
		// go through all the groups and find the object group that
		// has inObjectType associated with it
		num_groups = UUrMemory_Array_GetUsedElems(OBJgObjectGroups);
		for (i = 0; i < num_groups; i++)
		{
			if (strcmp(object_group_list[i]->group_name, inGroupName) == 0)
			{
				object_group = object_group_list[i];
				break;
			}
		}
	}
	
	return object_group;
}

// ----------------------------------------------------------------------
static OBJtObjectGroup*
OBJiObjectGroup_GetByType(
	OBJtObjectType				inObjectType)
{
	OBJtObjectGroup				*object_group;
	OBJtObjectGroup				**object_group_list;
	
	// intialize
	object_group = NULL;
	
	// make sure the object group list exists
	object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
	if (object_group_list)
	{
		UUtUns32				num_groups;
		UUtUns32				i;
		
		// go through all the groups and find the object group that
		// has an object type of inObjectType
		num_groups = UUrMemory_Array_GetUsedElems(OBJgObjectGroups);
		for (i = 0; i < num_groups; i++)
		{
			if (object_group_list[i]->object_type == inObjectType)
			{
				object_group = object_group_list[i];
				break;
			}
		}
	}
	
	return object_group;
}

// ----------------------------------------------------------------------
UUtUns32 OBJrObjectType_GetOSDSize(
	OBJtObjectType				inObjectType)
{
	OBJtObjectGroup				*object_group;

	object_group = OBJiObjectGroup_GetByType(inObjectType);
	if (object_group == NULL) {
		return 0;
	} else {
		return object_group->osd_size_in_memory;
	}
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiObjectGroup_GetNumObjects(
	OBJtObjectGroup				*inObjectGroup)
{
	UUtUns32					num_objects;
	
	UUmAssert(inObjectGroup);
	
	if (inObjectGroup->object_list)
	{
		num_objects = UUrMemory_Array_GetUsedElems(inObjectGroup->object_list);
	}
	else
	{
		num_objects = 0;
	}

	return num_objects;
}

// ----------------------------------------------------------------------
static OBJtObject*
OBJiObjectGroup_GetObject(
	OBJtObjectGroup				*inObjectGroup,
	UUtUns32					inIndex)
{
	OBJtObject					**object_list;
	UUtUns32					num_objects;
	
	UUmAssert(inObjectGroup);
	
	num_objects = UUrMemory_Array_GetUsedElems(inObjectGroup->object_list);
	UUmAssert(inIndex < num_objects);
	
	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(inObjectGroup->object_list);
	if (object_list == NULL) { return NULL; }
	
	return object_list[inIndex];
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiObjectGroup_GetOSDSize(
	OBJtObjectGroup				*inObjectGroup)
{
	UUmAssert(inObjectGroup);
	
	return inObjectGroup->osd_size_in_memory;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiObjectGroup_GetWriteSize(
	OBJtObjectGroup				*inObjectGroup)
{
	UUtUns32					size;
	UUtUns32					i;
	UUtUns32					count;
	
	size = 0;
	count = OBJiObjectGroup_GetNumObjects(inObjectGroup);
	for (i = 0; i < count; i++)
	{
		OBJtObject				*object;
		UUtUns32				osd_write_size;
		UUtUns32				chunk_size;
		
		object = OBJiObjectGroup_GetObject(inObjectGroup, i);
		if (object == NULL) { continue; }
		
		osd_write_size = OBJrObject_GetOSDWriteSize(object);
		chunk_size = osd_write_size + OBJcObjectGenericWriteSize;
		chunk_size = UUmMakeMultiple(chunk_size, 4);
		
		size += chunk_size;
	}
	
	return size;
}

// ----------------------------------------------------------------------
static OBJtObjectType
OBJiObjectGroup_GetType(
	OBJtObjectGroup				*inObjectGroup)
{
	return inObjectGroup->object_type;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiObjectGroup_IsDirty(
	OBJtObjectGroup				*inObjectGroup)
{
	return ((inObjectGroup->flags & OBJcObjectGroupFlag_Dirty) != 0);
}

// ----------------------------------------------------------------------
static UUtBool OBJiObjectGroup_IsLocked(OBJtObjectGroup *inObjectGroup)
{
	UUtBool is_locked = (inObjectGroup->flags & OBJcObjectGroupFlag_Locked) > 0;

	return is_locked;
}

// ----------------------------------------------------------------------
static void
OBJiObjectGroup_VerifyMethods(
	OBJtMethods					*inMethods)
{
#if 0
#if defined(DEBUGGING) && DEBUGGING
	void						**methods;
	UUtUns32					i;
	
	UUmAssert(inMethods->rNew);
	UUmAssert(inMethods->rDelete);
	UUmAssert(inMethods->rDraw);
	UUmAssert(inMethods->rEnumerate);
	UUmAssert(inMethods->rGetBoundingSphere);
	UUmAssert(inMethods->rGetName);
	UUmAssert(inMethods->rUpdatePosition);
	UUmAssert(inMethods->rGetOSD);
	UUmAssert(inMethods->rSetOSD);
	UUmAssert(inMethods->rWrite);
	UUmAssert(inMethods->rRead);

	methods = (void*)inMethods;
	for (i = 0; i < (sizeof(OBJtMethods) >> 2); i++)
	{
		if (methods[i] == NULL) { UUmAssert(!"The methods are not complete"); }
	}
#endif
#endif
}

// ----------------------------------------------------------------------

UUtError
OBJrObjectGroup_Register(
	OBJtObjectType				inObjectType,
	UUtUns32					inObjectTypeIndex,
	char						*inGroupName,
	UUtUns32					inOSDSizeInMemory,
	OBJtMethods					*inMethods,
	UUtUns32					inFlags)
{
	UUtError					error;
	UUtUns32					index;
	UUtBool						mem_moved;
	OBJtObjectGroup				*object_group;
	OBJtObjectGroup				**object_group_list;
	
	UUmAssert(inGroupName);
	UUmAssert(inGroupName[0] != '\0');
	
	// see if inObjectType is already in use
	if (OBJiObjectGroup_GetByType(inObjectType) != NULL)
	{
		// this object type has already been registered
		return UUcError_Generic;
	}
	
	// see if inName is already in use
	if (OBJiObjectGroup_GetByName(inGroupName) != NULL)
	{
		// this object type has already been registered
		return UUcError_Generic;
	}
	
	// index must be a single nonzero byte
	if ((inObjectTypeIndex <= 0) || (inObjectTypeIndex >= 256)) {
		return UUcError_Generic;
	}

	OBJiObjectGroup_VerifyMethods(inMethods);
	
	// create a new group entry in the object groups array
	error = UUrMemory_Array_GetNewElement(OBJgObjectGroups, &index, &mem_moved);
	UUmError_ReturnOnError(error);
	
	// allocate memory for the group
	object_group = (OBJtObjectGroup*)UUrMemory_Block_New(sizeof(OBJtObjectGroup));
	UUmError_ReturnOnNull(object_group);
	
	// initialize this group
	object_group->object_list = NULL;
	object_group->object_type = inObjectType;
	object_group->object_typeindex = inObjectTypeIndex;
	object_group->flags = inFlags;
	object_group->osd_size_in_memory = inOSDSizeInMemory;
	object_group->methods = *inMethods;
	object_group->mechanics_class = NULL;
	UUrString_Copy(object_group->group_name, inGroupName, OBJcMaxNameLength);
		
	// add the object_group to the object group array
	object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
	UUmAssert(object_group_list);
	object_group_list[index] = object_group;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObjectGroup_RemoveObject(
	OBJtObjectGroup				*inObjectGroup,
	OBJtObject					*inObject)
{
	OBJtObject					**object_list;
	UUtUns32					i;
	UUtUns32					num_objects;
	
	UUmAssert(inObjectGroup);
	UUmAssert(inObject);
	
	// get a pointer to the object list
	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(inObjectGroup->object_list);
	if (object_list == NULL) { return; }
	
	// find the object in the object list and delete it
	num_objects = UUrMemory_Array_GetUsedElems(inObjectGroup->object_list);
	for (i = 0; i < num_objects; i++)
	{
		if (object_list[i] != inObject) { continue; }
		
		// delete the object from the list
		UUrMemory_Array_DeleteElement(inObjectGroup->object_list, i);
		
		// the object group is now dirty
		OBJiObjectGroup_SetDirty(inObjectGroup, UUcTrue);
		
		break;
	}
}

// ----------------------------------------------------------------------
static void
OBJiObjectGroup_SetDirty(
	OBJtObjectGroup				*inObjectGroup,
	UUtBool						inLocked)
{
	if (inLocked)
	{
		inObjectGroup->flags |= OBJcObjectGroupFlag_Dirty;
	}
	else
	{
		inObjectGroup->flags &= ~OBJcObjectGroupFlag_Dirty;
	}
}

// ----------------------------------------------------------------------
static void
OBJiObjectGroup_SetLocked(
	OBJtObjectGroup				*inObjectGroup,
	UUtBool						inLocked)
{
	if (inLocked)
	{
		inObjectGroup->flags |= OBJcObjectGroupFlag_Locked;
	}
	else
	{
		inObjectGroup->flags &= ~OBJcObjectGroupFlag_Locked;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiObjectGroups_Draw(
	void)
{
	UUtUns32					i;
	UUtUns32					num_groups;
	OBJtObjectGroup				**object_group_list;
	
	UUmAssert(OBJgObjectGroups);

	object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
	if (object_group_list == NULL) { return; }
	
	num_groups = UUrMemory_Array_GetUsedElems(OBJgObjectGroups);
	for (i = 0; i < num_groups; i++)
	{
		if (OBJrObjectType_GetVisible(object_group_list[i]->object_type))
		{
			OBJiObjectGroup_Draw(object_group_list[i]);
		}
	}
}

// ----------------------------------------------------------------------
static void
OBJiObjectGroups_Enumerate(
	OBJtEnumCallback_ObjectType	inEnumCallback,
	UUtUns32					inUserData)
{
	UUtUns32					i;
	UUtUns32					num_groups;
	OBJtObjectGroup				**object_group_list;
	
	UUmAssert(OBJgObjectGroups);
	UUmAssert(inEnumCallback);
	
	object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
	if (object_group_list == NULL) { return; }
	
	num_groups = UUrMemory_Array_GetUsedElems(OBJgObjectGroups);
	for (i = 0; i < num_groups; i++)
	{
		UUtBool					result;
		
		result =
			inEnumCallback(
				object_group_list[i]->object_type,
				object_group_list[i]->group_name,
				inUserData);
		if (result == UUcFalse) { break; }
	}
}

// ----------------------------------------------------------------------
static OBJtObjectGroup*
OBJiObjectGroups_GetGroupByIndex(
	UUtUns32					inGroupIndex)
{
	OBJtObjectGroup				**object_group_list;
	UUtUns32					num_groups;
	
	UUmAssert(OBJgObjectGroups);
	
	num_groups = UUrMemory_Array_GetUsedElems(OBJgObjectGroups);
	UUmAssert(inGroupIndex < num_groups);
	
	object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
	UUmAssert(object_group_list);
	
	return object_group_list[inGroupIndex];
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiObjectGroups_GetNumGroups(
	void)
{
	UUmAssert(OBJgObjectGroups);
	
	return UUrMemory_Array_GetUsedElems(OBJgObjectGroups);
}


// ----------------------------------------------------------------------
static UUtError
OBJiObjectGroups_Initialize(
	void)
{
	UUmAssert(OBJgObjectGroups == NULL);

	OBJgNextObjectID = 1;

	// create the object group array
	OBJgObjectGroups =
		UUrMemory_Array_New(
			sizeof(OBJtObjectGroup*),
			1,
			0,
			0);
	UUmError_ReturnOnNull(OBJgObjectGroups);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------------------------------------------------
static UUtError
OBJiObjectGroups_LevelLoad(
	void)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObjectGroups_LevelUnload(
	void)
{
	OBJtObjectGroup			**object_group_list;
	UUtUns32				num_groups;
	UUtUns32				i;
	
	if (OBJgObjectGroups == NULL) { return; }
	
	// delete the objects in the object groups
	object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
	num_groups = UUrMemory_Array_GetUsedElems(OBJgObjectGroups);
	for (i = 0; i < num_groups; i++)
	{
		// don't unload those common to all levels
		if (object_group_list[i]->flags & OBJcObjectGroupFlag_CommonToAllLevels)
			continue;

		// delete the objects in the object group
		OBJiObjectGroup_DeleteAllObjects(object_group_list[i]);
		
		// clear the object groups locked status
		OBJiObjectGroup_SetLocked(object_group_list[i], UUcFalse);
	}
}

// ----------------------------------------------------------------------
static void
OBJiObjectGroups_Terminate(
	void)
{
	if (OBJgObjectGroups)
	{
		// delete the object groups in the object group array
		while (UUrMemory_Array_GetUsedElems(OBJgObjectGroups))
		{
			OBJtObjectGroup			**object_group_list;

			object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
			OBJiObjectGroup_Delete(object_group_list[0]);
		}
		
		// delete the object group array
		UUrMemory_Array_Delete(OBJgObjectGroups);
		OBJgObjectGroups = NULL;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
OBJrObjectType_EnumerateObjects(
	OBJtObjectType					inObjectType,
	OBJtEnumCallback_Object			inEnumCallback,
	UUtUns32						inUserData)
{
	OBJtObjectGroup					*object_group;
	UUtUns32						num_objects;
		
	object_group = OBJiObjectGroup_GetByType(inObjectType);
	if (object_group == NULL) { return 0; }
	num_objects = OBJiObjectGroup_EnumerateObjects(object_group, inEnumCallback, inUserData);

	return num_objects;
}

// ----------------------------------------------------------------------
OBJtObject *OBJrObjectType_GetObject_ByNumber(
	OBJtObjectType inObjectType,
	UUtUns32 inIndex)
{
	OBJtObjectGroup				*object_group;
//	UUtUns32					num_objects;
//	OBJtObject					**object_list;
	
	object_group = OBJiObjectGroup_GetByType(inObjectType);
	return OBJiObjectGroup_GetObject(object_group, inIndex);
	
/*	if (NULL == object_group) { return NULL; }
	if (NULL == object_group->object_list) { return NULL; }

	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(object_group->object_list);

	if (NULL == object_list) { return NULL; }

	num_objects = UUrMemory_Array_GetUsedElems(object_group->object_list);

	if (inIndex >= num_objects) { return NULL; }

	return object_list[inIndex];*/
}

// ----------------------------------------------------------------------
const char *
OBJrObjectType_GetName(
	OBJtObjectType					inObjectType)
{
	OBJtObjectGroup					*object_group;
	
	object_group = OBJiObjectGroup_GetByType(inObjectType);

	if (object_group == NULL) {
		return "<error>";
	} else {
		return object_group->group_name;
	}
}

// ----------------------------------------------------------------------
void
OBJrObjectType_GetUniqueOSD(
	OBJtObjectType					inObjectType,
	OBJtOSD_All						*outOSD)
{
	OBJtObjectGroup					*object_group;
	
	object_group = OBJiObjectGroup_GetByType(inObjectType);
	if (object_group == NULL) { return; }
	
	object_group->methods.rGetUniqueOSD(outOSD);

	return;
}

// ----------------------------------------------------------------------
UUtBool
OBJrObjectType_GetVisible(
	OBJtObjectType					inObjectType)
{
	OBJtObjectGroup					*object_group;
	
	object_group = OBJiObjectGroup_GetByType(inObjectType);
	if (object_group == NULL) { return UUcFalse; }
	
	return object_group->methods.rGetClassVisible();
}

// ----------------------------------------------------------------------
UUtBool
OBJrObjectType_IsLocked(
	OBJtObjectType					inObjectType)
{
	OBJtObjectGroup					*object_group;
	UUtBool							is_locked = UUcFalse;
	
	object_group = OBJiObjectGroup_GetByType(inObjectType);
	if (object_group == NULL) { goto exit; }
	
	is_locked = OBJiObjectGroup_IsLocked(object_group);

exit:
	return is_locked;
}

// ----------------------------------------------------------------------
OBJtObject*
OBJrObjectType_Search(
	OBJtObjectType					inObjectType,
	UUtUns32						inSearchType,
	const OBJtOSD_All				*inSearchParams)
{
	OBJtObjectGroup					*object_group;
	OBJtObject						*found_object;
	UUtUns32						i;
	UUtUns32						num_objects;
	
	object_group = OBJiObjectGroup_GetByType(inObjectType);
	if (object_group == NULL) { return NULL; }
	
	// find the object in the list
	found_object = NULL;
	num_objects = OBJiObjectGroup_GetNumObjects(object_group);
	for (i = 0; i < num_objects; i++)
	{
		UUtBool						found;
		OBJtObject					*object;
		
		object = OBJiObjectGroup_GetObject(object_group, i);
		if (object == NULL) { continue; }
		
		found =
			object_group->methods.rSearch(
				object,
				inSearchType,
				inSearchParams);
		if (found)
		{
			found_object = object;
			break;
		}
	}
	
	return found_object;
}

// ----------------------------------------------------------------------
void
OBJrObjectType_SetVisible(
	OBJtObjectType					inObjectType,
	UUtBool							inIsVisible)
{
	OBJtObjectGroup					*object_group;
	
	object_group = OBJiObjectGroup_GetByType(inObjectType);
	if (object_group == NULL) { return; }
	
	object_group->methods.rSetClassVisible(inIsVisible);
}

// ----------------------------------------------------------------------
void
OBJrObjectTypes_Enumerate(
	OBJtEnumCallback_ObjectType		inEnumCallback,
	UUtUns32						inUserData)
{
	UUmAssert(inEnumCallback);
	OBJiObjectGroups_Enumerate(inEnumCallback, inUserData);
}

// ----------------------------------------------------------------------
// mechanics helpers
// ----------------------------------------------------------------------

UUtError OBJrObjectType_RegisterMechanics( OBJtObjectType inObjectType, ONtMechanicsClass *inMechanicsClass )
{
	OBJtObjectGroup				*object_group;
	
	if (!(object_group = OBJiObjectGroup_GetByType(inObjectType)))
		return UUcError_Generic;

	object_group->mechanics_class	= inMechanicsClass;

	return UUcError_None;
}

ONtMechanicsClass* OBJrObjectType_GetMechanicsByType( OBJtObjectType inObjectType )
{
	OBJtObjectGroup				*object_group;
	
	if (!(object_group = OBJiObjectGroup_GetByType(inObjectType)))
		return NULL;

	return object_group->mechanics_class;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
OBJiObject_Read_1(
	OBJtObject				*inObject,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	UUtUns32				num_bytes;
	
	num_bytes = 0;
	
	// read the position
	OBDmGet4BytesFromBuffer(inBuffer, inObject->position.x, float, inSwapIt);
	OBDmGet4BytesFromBuffer(inBuffer, inObject->position.y, float, inSwapIt);
	OBDmGet4BytesFromBuffer(inBuffer, inObject->position.z, float, inSwapIt);
	num_bytes += sizeof(float) + sizeof(float) + sizeof(float);

	// read the rotation
	OBDmGet4BytesFromBuffer(inBuffer, inObject->rotation.x, float, inSwapIt);
	OBDmGet4BytesFromBuffer(inBuffer, inObject->rotation.y, float, inSwapIt);
	OBDmGet4BytesFromBuffer(inBuffer, inObject->rotation.z, float, inSwapIt);
	num_bytes += sizeof(float) + sizeof(float) + sizeof(float);

	return num_bytes;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiObject_Read_3(
	OBJtObject				*inObject,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	UUtUns32				num_bytes;
	
	num_bytes = 0;
	
	// read the flags
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->flags, UUtUns32, inSwapIt);
	
	// read the position
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->position.x, float, inSwapIt);
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->position.y, float, inSwapIt);
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->position.z, float, inSwapIt);

	// read the rotation
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->rotation.x, float, inSwapIt);
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->rotation.y, float, inSwapIt);
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->rotation.z, float, inSwapIt);

	return num_bytes;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiObject_Read_7(
	OBJtObject				*inObject,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	UUtUns32				num_bytes;
	
	num_bytes = 0;
	
	// read the ID
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->object_id, UUtUns32, inSwapIt);
	
	// read the flags
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->flags, UUtUns32, inSwapIt);
	
	// read the position
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->position.x, float, inSwapIt);
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->position.y, float, inSwapIt);
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->position.z, float, inSwapIt);

	// read the rotation
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->rotation.x, float, inSwapIt);
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->rotation.y, float, inSwapIt);
	num_bytes += OBDmGet4BytesFromBuffer(inBuffer, inObject->rotation.z, float, inSwapIt);

	return num_bytes;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError OBJrObject_OSD_SetDefaults(OBJtObjectType inType, OBJtOSD_All *outOSD)
{
	OBJtObjectGroup					*object_group;
	
	object_group = OBJiObjectGroup_GetByType(inType);
	UUmAssert(object_group != NULL); 
	if (object_group == NULL) {
		return UUcError_Generic;
	}

	return object_group->methods.rSetDefaults(outOSD);
}

// ----------------------------------------------------------------------
void OBJrObject_OSD_GetName(OBJtObjectType inType, const OBJtOSD_All *inOSD, char *outName, UUtUns32 inNameLength)
{
	OBJtObjectGroup					*object_group;
	
	object_group = OBJiObjectGroup_GetByType(inType);
	UUmAssert(object_group != NULL); 
	if (object_group == NULL) { return; }

	object_group->methods.rOSDGetName(inOSD, outName, inNameLength);

	return;
}

// ----------------------------------------------------------------------
void OBJrObject_OSD_SetName(OBJtObjectType inType, OBJtOSD_All *inOSD, const char *inName)
{
	OBJtObjectGroup					*object_group;
	
	object_group = OBJiObjectGroup_GetByType(inType);
	UUmAssert(object_group != NULL); 
	if (object_group == NULL) { return; }

	object_group->methods.rOSDSetName(inOSD, inName);

	return;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBJrObject_CreateFromBuffer(
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer,
	UUtUns32				*outNumBytesRead)
{
	UUtError				error;
	OBJtObjectType			object_type;
	UUtUns32				object_size;
	OBJtObject				*object;
	UUtUns32				num_bytes;
	UUtUns32				total_bytes;
	OBJtObjectGroup			*object_group;
	
	// init the returning values
	*outNumBytesRead = 0;
	total_bytes = 0;
	
	// read the object type
	OBDmGet4BytesFromBuffer(inBuffer, object_type, OBJtObjectType, inSwapIt);
	num_bytes = sizeof(OBJtObjectType);
	total_bytes += num_bytes;
	
	// make sure the object type is valid
	object_group = OBJiObjectGroup_GetByType(object_type);
	if (object_group == NULL) { return UUcError_Generic; }
	
	// allocate memory for the new object
	object_size = sizeof(OBJtObject) + OBJiObjectGroup_GetOSDSize(object_group);
	object = (OBJtObject*)UUrMemory_Block_NewClear(object_size);
	UUmError_ReturnOnNull(object);
	
	// set the objects methods
	object->object_type = object_type;
	object->methods = &object_group->methods;
	object->mechanics_class		= object_group->mechanics_class;

	// mark the ID as not used
	object->object_id = 0;

	UUmAssert(inVersion <= OBJcCurrentVersion);

	// process the common object data
	if (inVersion >= 7) 
	{
		num_bytes = OBJiObject_Read_7(object, inSwapIt, inBuffer);
	} 
	else if (inVersion >= 3) 
	{
		num_bytes = OBJiObject_Read_3(object, inSwapIt, inBuffer);
	} 
	else if (inVersion >= 1) 
	{
		num_bytes = OBJiObject_Read_1(object, inSwapIt, inBuffer);
	} 
	else 
	{
		UUmAssert(!"Unknown Version file version");
	}

	inBuffer += num_bytes;
	total_bytes += num_bytes;
	
	// process the object specific data
	num_bytes = object->methods->rRead(object, inVersion, inSwapIt, inBuffer);
	num_bytes = UUmMakeMultiple(num_bytes, 4);

	inBuffer += num_bytes;
	total_bytes += num_bytes;

	// set the object id if one does not exist
	if( object->object_id == 0 )
	{
		object->object_id = OBJrObject_GetNextAvailableID();
	}
	else
	{
		// make sure our next available is higher than it.....
		if( object->object_id >= OBJgNextObjectID )
			OBJgNextObjectID = object->object_id + 1;
	}
	
	// add the object to the object list
	error = OBJiObjectGroup_AddObject(object_group, object);
	if (error != UUcError_None) {
		UUmAssert(!"OBJrObject_CreateFromBuffer failed");
		UUrDebuggerMessage("OBJrObject_CreateFromBuffer: unable to add object to group %s", object_group->group_name);
	}
	
	// return the number of bytes read
	*outNumBytesRead = total_bytes;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrObject_Delete(
	OBJtObject					*inObject)
{
	OBJtObjectGroup				*object_group;
	
	UUmAssert(inObject);
	
	// make sure the object isn't locked
	if (OBJrObject_IsLocked(inObject)) { return; }
	
	// get the object group that inObject belongs to
	object_group = OBJiObjectGroup_GetByType(inObject->object_type);
	if (object_group == NULL) { return; }
	if (OBJiObjectGroup_IsLocked(object_group)) { return; }
	
	// remove the object from the object group
	OBJiObjectGroup_RemoveObject(object_group, inObject);
	
	// delete the object specific data
	inObject->methods->rDelete(inObject);
	
	// make sure this object isn't selected
	if (OBJrSelectedObjects_IsObjectSelected(inObject))
	{
		OBJrSelectedObjects_Unselect(inObject);
	}
	
	// release the memory used by inObject
	UUrMemory_Block_Delete(inObject);
	UUrMemory_Block_VerifyList();
}

// ----------------------------------------------------------------------
void
OBJrObject_Draw(
	OBJtObject					*inObject)
{
	UUtUns32					draw_flags;
	
	UUmAssert(inObject);
	
	draw_flags = 0;
	if (OBJrSelectedObjects_IsObjectSelected(inObject))
	{
		draw_flags |= OBJcDrawFlag_Selected;
	}
	
	if (inObject->flags & OBJcObjectFlag_Locked)
	{
		draw_flags |= OBJcDrawFlag_Locked;
	}
	
	switch (OBJgRotateMode)
	{
		case OBJcRotateMode_XYZ:
			draw_flags |= (OBJcDrawFlag_RingX | OBJcDrawFlag_RingY | OBJcDrawFlag_RingZ);
		break;
		
		case OBJcRotateMode_XY:
			draw_flags |= OBJcDrawFlag_RingZ;
		break;
		
		case OBJcRotateMode_XZ:
			draw_flags |= OBJcDrawFlag_RingY;
		break;
		
		case OBJcRotateMode_YZ:
			draw_flags |= OBJcDrawFlag_RingX;
		break;
	}
	
	inObject->methods->rDraw(inObject, draw_flags);
}

// ----------------------------------------------------------------------
void
OBJrObject_GetPosition(
	const OBJtObject		*inObject,
	M3tPoint3D				*outPosition,
	M3tPoint3D				*outRotation)
{
	UUmAssert(inObject);
	
	if (outPosition)
	{
		*outPosition = inObject->position;
	}
	
	if (outRotation)
	{
		*outRotation = inObject->rotation;
	}
}

// ----------------------------------------------------------------------
void
OBJrObject_GetRotationMatrix(
	const OBJtObject		*inObject,
	M3tMatrix4x3			*outMatrix)
{
	MUtEuler				euler;

	euler.order = MUcEulerOrderXYZs;
	euler.x = inObject->rotation.x * M3cDegToRad;
	euler.y = inObject->rotation.y * M3cDegToRad;
	euler.z = inObject->rotation.z * M3cDegToRad;

	MUrEulerToMatrix(&euler, outMatrix);
}

// ----------------------------------------------------------------------
UUtBool
OBJrObject_IsLocked(
	const OBJtObject			*inObject)
{
	return ((inObject->flags & OBJcObjectFlag_Locked) != 0);
}

// ----------------------------------------------------------------------
UUtError
OBJrObject_New(
	const OBJtObjectType		inObjectType,
	const M3tPoint3D			*inPosition,
	const M3tPoint3D			*inRotation,
	const OBJtOSD_All			*inObjectSpecificData)
{
	UUtError					error;
	UUtUns32					object_size;
	OBJtObject					*object;
	OBJtObjectGroup				*object_group;
	
	// get the object group that the object will be added to
	object_group = OBJiObjectGroup_GetByType(inObjectType);
	if (object_group == NULL) { return UUcError_Generic; }
	if (OBJiObjectGroup_IsLocked(object_group)) { return UUcError_Generic; }
	
	// allocate memory for the new object
	object_size = sizeof(OBJtObject) + OBJiObjectGroup_GetOSDSize(object_group);
	object = (OBJtObject*)UUrMemory_Block_NewClear(object_size);
	UUmError_ReturnOnNull(object);
	
	// initialize the object
	object->object_type = inObjectType;
	object->object_id = OBJrObject_GetNextAvailableID();
	object->flags = OBJcObjectFlag_None;
	object->position = *inPosition;
	object->rotation = *inRotation;
	object->methods = &object_group->methods;
	object->mechanics_class = object_group->mechanics_class;
	
	error = object->methods->rNew(object, inObjectSpecificData);
	UUmError_ReturnOnError(error);
	
	// add the object to the object group
	error = OBJiObjectGroup_AddObject(object_group, object);
	UUmError_ReturnOnError(error);
	
	// select the newly created object
	OBJrSelectedObjects_Select(object, UUcFalse);

UUrMemory_Block_VerifyList();
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrObject_LevelBegin(
	void)
{
	OBJtObjectGroup			**object_group_list;
	UUtUns32				num_groups;
	UUtUns32				i;
	UUtError				error;
	
	UUmAssert( ONgLevel );

	if (OBJgObjectGroups == NULL) { return UUcError_Generic; }

	// delete the objects in the object groups
	object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
	num_groups = UUrMemory_Array_GetUsedElems(OBJgObjectGroups);

	UUmAssert(object_group_list);

	for (i = 0; i < num_groups; i++)
	{	
		OBJtObjectGroup				*object_group;
		UUtUns32					num_objects;
		UUtUns32					j, tag;
		OBJtObject					**object_list;
		OBJtObject					*object;
		
		object_group = object_group_list[i];

		UUmAssert(object_group);

		// call levelbegin on this group, if we have a method for it
		if( object_group->methods.rLevelBegin ) {
			error = object_group->methods.rLevelBegin();
			UUmError_ReturnOnError(error);
		}

		if (object_group->object_list == NULL) continue;

 		object_list = (OBJtObject**)UUrMemory_Array_GetMemory(object_group->object_list);
		if (object_list == NULL) continue;
		
		num_objects = UUrMemory_Array_GetUsedElems(object_group->object_list);

		for (j = 0; j < num_objects; j++)
		{
			// grab the object
			object = object_list[j];

			// see if this object has been imported as gunk, and set the flag....
			tag = OBJmMakeObjectTag(object_group->object_typeindex, object);
			if( ONrLevel_FindObjectTag(tag) )
				object->flags |= OBJcObjectFlag_Gunk;
			else
				object->flags &= ~OBJcObjectFlag_Gunk;
		}
	}

	return UUcError_None;
}

UUtError OBJrObject_LevelEnd(void)
{
	OBJtObjectGroup			**object_group_list;
	UUtUns32				num_groups;
	UUtUns32				i;
	UUtError				error;
	
	UUmAssert( ONgLevel );

	// clear the selected objects
	OBJgSelectedObjects_Locked = UUcFalse;
	OBJrSelectedObjects_UnselectAll();

	if (OBJgObjectGroups == NULL) { return UUcError_Generic; }

	// delete the objects in the object groups
	object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
	num_groups = UUrMemory_Array_GetUsedElems(OBJgObjectGroups);

	UUmAssert(object_group_list);

	for (i = 0; i < num_groups; i++)
	{	
		OBJtObjectGroup				*object_group;
		
		object_group = object_group_list[i];

		UUmAssert(object_group);

		// call levelend on this group, if we have a method for it
		if( object_group->methods.rLevelEnd ) {
			error = object_group->methods.rLevelEnd();
			UUmError_ReturnOnError(error);
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrObject_MouseMove(
	OBJtObject				*inObject,
	IMtPoint2D				*inStartPoint,
	IMtPoint2D				*inEndPoint)
{
	static OBJtObject		*last_object = NULL;
	OBJtObjectGroup			*object_group;
	UUtBool					set_rotation;
	M3tPoint3D				new_object_position;
	M3tVector3D				new_object_rotation;
	float					delta_x;
	float					delta_y;
	
	// make sure the object isn't locked
	if (OBJrObject_IsLocked(inObject)) { return; }
	
	// make sure the object group isn't locked
	object_group = OBJiObjectGroup_GetByType(inObject->object_type);
	if (object_group == NULL) { return; }
	if (OBJiObjectGroup_IsLocked(object_group)) { return; }
	
	// make sure the start point and end point aren't the same
	if ((inStartPoint->x == inEndPoint->x) &&
		(inStartPoint->y == inEndPoint->y))
	{
		return;
	}

	if (last_object != inObject) {
		char move_name_string[1024];

		OBJrObject_GetName(inObject, move_name_string, sizeof(move_name_string));

		COrConsole_Printf("moving object %s", move_name_string);

		last_object = inObject;
	}
	
	// calculate the delta between the points
	delta_x = (float)(inEndPoint->x - inStartPoint->x);
	delta_y = (float)(inStartPoint->y - inEndPoint->y);
	
	set_rotation = UUcFalse;
	new_object_position = inObject->position;
	new_object_rotation = inObject->rotation;
	
	// restrict left/right or forward/backward movement
	if ((OBJgMoveMode & OBJcMoveMode_LR) == 0)
	{
		delta_x = 0.0f;
	}
	else if ((OBJgMoveMode & (OBJcMoveMode_FB | OBJcMoveMode_UD)) == 0)
	{
		delta_y = 0.0f;
	}

	if (OBJgMoveMode & OBJcMoveMode_Wall)
	{
		float				object_dist, dist_x, dist_y, fov_x, fov_y, aspect, plane_dist;
		M3tPoint3D			camera_pos;
		M3tVector3D			fwd, up, right, ray_dir, delta_pos, normal, plus_Y = {0, 1, 0};
		M3tGeomCamera *		camera;
		AKtEnvironment *	environment;
		M3tMatrix4x3		rotate_matrix;
		UUtBool				found_collision;
		MUtEuler			euler_angles;
		AKtGQ_Collision *	gq_collision;

		/*
		 * move the object around on a wall
		 */
		M3rCamera_GetActive(&camera);
		M3rCamera_GetViewData(camera, &camera_pos, &fwd, &up);
		M3rCamera_GetViewData_VxU(camera, &right);

		// calculate normalized view directions
		MUmVector_Normalize(fwd);
		MUmVector_Normalize(up);
		MUmVector_Normalize(right);

		// how far is the object currently from the camera?
		MUmVector_Subtract(delta_pos, inObject->position, camera_pos);
		object_dist = MUrVector_DotProduct(&delta_pos, &fwd);

		// determine how far this mouse movement is in worldspace
		M3rCamera_GetStaticData(camera, &fov_y, &aspect, NULL, NULL);
		fov_x = fov_y * aspect;
		dist_x = delta_x * fov_x * object_dist / M3rDraw_GetWidth();
		dist_y = delta_y * fov_y * object_dist / M3rDraw_GetHeight();

		// increment the object's position by this movement amount
		MUmVector_ScaleIncrement(new_object_position, dist_x, right);
		MUmVector_ScaleIncrement(new_object_position, dist_y, up);

		// snap object to wall if we can
		environment = ONrGameState_GetEnvironment();
		MUmVector_Subtract(ray_dir, new_object_position, camera_pos);
		MUmVector_Scale(ray_dir, 10.0f);
		found_collision = AKrCollision_Point(environment, &camera_pos, &ray_dir,
								 AKcGQ_Flag_Obj_Col_Skip | AKcGQ_Flag_Invisible | AKcGQ_Flag_Jello, AKcMaxNumCollisions);
		if (found_collision) {
			// get the position and orientation of this collision
			new_object_position = AKgCollisionList[0].collisionPoint;
			gq_collision = environment->gqCollisionArray->gqCollision + AKgCollisionList[0].gqIndex;
			AKmPlaneEqu_GetComponents(gq_collision->planeEquIndex, environment->planeArray->planes, normal.x, normal.y, normal.z, plane_dist);

			// get our current rotation matrix
			euler_angles.order = MUcEulerOrderXYZs;
			euler_angles.x = inObject->rotation.x * M3cDegToRad;
			euler_angles.y = inObject->rotation.y * M3cDegToRad;
			euler_angles.z = inObject->rotation.z * M3cDegToRad;
			MUrEulerToMatrix(&euler_angles, &rotate_matrix);

			// determine the rotation matrix that has +Y normal to the collision, but
			// remains pointing in this general direction
			up = MUrMatrix_GetZAxis(&rotate_matrix);
			MUrVector_CrossProduct(&normal, &up, &right);
			if (MUrVector_Normalize_ZeroTest(&right)) {
				// the normal of the collision is oriented along +Z of our previous rotation matrix... use a Z of the previous -Y
				right = MUrMatrix_GetYAxis(&rotate_matrix);
				MUmVector_Negate(right);
				// uhh? S.S. UUmAssert((float)fabs(MUrVector_DotProduct(&normal, &right) < 0.001f));
			}
			MUrVector_CrossProduct(&right, &normal, &up);
			MUmVector_Normalize(up);
			MUrMatrix3x3_FormOrthonormalBasis(&right, &normal, &up, (M3tMatrix3x3 *) &rotate_matrix);

			// calculate euler angles for this new rotation matrix
			euler_angles = MUrMatrixToEuler(&rotate_matrix, MUcEulerOrderXYZs);

			// set the object to this rotation
			new_object_rotation.x = euler_angles.x * M3cRadToDeg;
			new_object_rotation.y = euler_angles.y * M3cRadToDeg;
			new_object_rotation.z = euler_angles.z * M3cRadToDeg;
			set_rotation = UUcTrue;
		}
	}
	else if (OBJgMoveMode & OBJcMoveMode_UD)
	{
		M3tVector3D			y_vector;
		
		// move up and down
		MUmVector_Set(y_vector, 0.0f, 1.0f, 0.0f);
		MUmVector_Scale(y_vector, delta_y * 0.1f);
		MUmVector_Increment(new_object_position, y_vector);
	}
	else
	{
		M3tVector3D			up;
		M3tVector3D			cross;
		M3tVector3D			view_vector;
		
		// get the vector in the direction the camera is looking
		view_vector = CArGetFacing();
		MUrNormalize(&view_vector);
		
		// get the vector which is orthogonal to the view vector and
		// is in the same xz plane
		MUmVector_Copy(up, view_vector);
		up.y += 1.0f;
		MUrVector_CrossProduct(&up, &view_vector, &cross);
		MUrNormalize(&cross);
		
		// no y movement
		view_vector.y = 0.0f;
		cross.y = 0.0f;
		
		// scale the vectors by the movement of the mouse
		MUmVector_Scale(view_vector, delta_y * 0.2f);
		MUmVector_Negate(cross);
		MUmVector_Scale(cross, delta_x * 0.1f);
		
		// add the movement vectors to the object's position
		MUmVector_Increment(new_object_position, view_vector);
		MUmVector_Increment(new_object_position, cross);
	}
	
	// set the object's new position
	OBJrObject_SetPosition(inObject, &new_object_position, (set_rotation) ? &new_object_rotation : NULL);
}

// ----------------------------------------------------------------------
void
OBJrObject_MouseRotate_Begin(
	OBJtObject				*inObject,
	IMtPoint2D				*inMousePosition)
{
	M3tGeomCamera			*camera;
	M3tPoint3D				camera_position;
	M3tPoint3D				start_world_point;
	
	M3tVector3D				obj_center;
	M3tVector3D				cam_to_obj;
	M3tVector3D				cam_to_start;
	M3tPoint3D				cam_through_start;
	
	float					cam_to_obj_dist;
	
	M3tVector3D				origin;
	M3tVector3D				x_vector;
	M3tVector3D				y_vector;
	M3tVector3D				z_vector;
	
	M3tVector3D				obj_origin;
	M3tVector3D				obj_x_vector;
	M3tVector3D				obj_y_vector;
	M3tVector3D				obj_z_vector;
	
	M3tPlaneEquation		xy_plane;
	M3tPlaneEquation		xz_plane;
	M3tPlaneEquation		yz_plane;
	
	M3tMatrix4x3			matrix;
	
	M3tPoint3D				xy_start_intersect;
	M3tPoint3D				xz_start_intersect;
	M3tPoint3D				yz_start_intersect;
	
	M3tBoundingSphere		bounding_sphere;
	
	M3tVector3D				xy_dist_vector;
	M3tVector3D				xz_dist_vector;
	M3tVector3D				yz_dist_vector;
	
	float					xy_distance;
	float					xz_distance;
	float					yz_distance;
	
	UUtBool					xy_int;
	UUtBool					xz_int;
	UUtBool					yz_int;
	
	OBJtObjectGroup			*object_group;
	
	// make sure the object isn't locked
	if (OBJrObject_IsLocked(inObject)) { return; }
	
	// make sure the object group isn't locked
	object_group = OBJiObjectGroup_GetByType(inObject->object_type);
	if (object_group == NULL) { return; }
	if (OBJiObjectGroup_IsLocked(object_group)) { return; }
	
	// get the active camera
	M3rCamera_GetActive(&camera);
	UUmAssert(camera);
	
	// get the camera position
	camera_position = CArGetLocation();
	
	// get the point on the near clip plane where the user clicked
	M3rPick_ScreenToWorld(
		camera,
		(UUtUns16)inMousePosition->x,
		(UUtUns16)inMousePosition->y,
		&start_world_point);
	
	// get the bounding sphere
	OBJrObject_GetBoundingSphere(inObject, &bounding_sphere);
	
	// calculate the object center	
	MUmVector_Add(obj_center, bounding_sphere.center, inObject->position);
	
	// calculate the distance from the camera to the object
	MUmVector_Subtract(cam_to_obj, obj_center, camera_position); 
	cam_to_obj_dist = MUrVector_Length(&cam_to_obj);
	
	// get a point on the line from the camera through the start point to a point
	// that doesn't quite reach the other side of the bounding sphere
	MUmVector_Subtract(cam_to_start, start_world_point, camera_position);
	MUrNormalize(&cam_to_start);
	MUmVector_Scale(cam_to_start, cam_to_obj_dist + (bounding_sphere.radius / 2));
	MUmVector_Add(cam_through_start, cam_to_start, camera_position);
		
	// initialize unit vectors
	MUmVector_Set(origin, 0.0f, 0.0f, 0.0f);
	MUmVector_Set(x_vector, 1.0f, 0.0f, 0.0f);
	MUmVector_Set(y_vector, 0.0f, 1.0f, 0.0f);
	MUmVector_Set(z_vector, 0.0f, 0.0f, 1.0f);
	
	// rotate the unit vectors
	MUrMatrix_Identity(&matrix);
	MUrMatrix_RotateX(&matrix, (inObject->rotation.x * M3cDegToRad));
	MUrMatrix_RotateY(&matrix, (inObject->rotation.y * M3cDegToRad));
	MUrMatrix_RotateZ(&matrix, (inObject->rotation.z * M3cDegToRad));
	
	MUrMatrix_MultiplyPoint(&origin, &matrix, &obj_origin);
	MUrMatrix_MultiplyPoint(&x_vector, &matrix, &obj_x_vector);
	MUrMatrix_MultiplyPoint(&y_vector, &matrix, &obj_y_vector);
	MUrMatrix_MultiplyPoint(&z_vector, &matrix, &obj_z_vector);
	
	// move the rotated unit vectors to the object center
	MUmVector_Increment(obj_origin, obj_center);
	MUmVector_Increment(obj_x_vector, obj_center);
	MUmVector_Increment(obj_y_vector, obj_center);
	MUmVector_Increment(obj_z_vector, obj_center);
	
	// build plane equations
	MUrVector_PlaneFromEdges(&obj_origin, &obj_x_vector, &obj_origin, &obj_y_vector, &xy_plane);
	MUrVector_PlaneFromEdges(&obj_origin, &obj_x_vector, &obj_origin, &obj_z_vector, &xz_plane);
	MUrVector_PlaneFromEdges(&obj_origin, &obj_y_vector, &obj_origin, &obj_z_vector, &yz_plane);
	
	// intersect points
	xy_int = CLrLine_Plane(&camera_position, &cam_through_start, &xy_plane, &xy_start_intersect);
	xz_int = CLrLine_Plane(&camera_position, &cam_through_start, &xz_plane, &xz_start_intersect);
	yz_int = CLrLine_Plane(&camera_position, &cam_through_start, &yz_plane, &yz_start_intersect);
	
	// calculate distance from bounding sphere edge to intersecting point
	xy_distance = UUcFloat_Max;
	xz_distance = UUcFloat_Max;
	yz_distance = UUcFloat_Max;

	if (xy_int)
	{
		MUmVector_Subtract(xy_dist_vector, xy_start_intersect, obj_origin);
		xy_distance = (float)fabs(MUrVector_Length(&xy_dist_vector) - bounding_sphere.radius);
	}
	
	if (xz_int)
	{
		MUmVector_Subtract(xz_dist_vector, xz_start_intersect, obj_origin);
		xz_distance = (float)fabs(MUrVector_Length(&xz_dist_vector) - bounding_sphere.radius);
	}
	
	if (yz_int)
	{
		MUmVector_Subtract(yz_dist_vector, yz_start_intersect, obj_origin);
		yz_distance = (float)fabs(MUrVector_Length(&yz_dist_vector) - bounding_sphere.radius);
	}
	
	// determine which axis to rotate
	OBJgRotateAxis = OBJcRotate_None;
	if ((OBJgRotateMode & OBJcRotateMode_XYZ) == OBJcRotateMode_XYZ)
	{
		if (xy_distance < xz_distance)
		{
			if (xy_distance < yz_distance)
			{
				OBJgRotateAxis = OBJcRotate_ZAxis;
			}
			else
			{
				OBJgRotateAxis = OBJcRotate_XAxis;
			}
		}
		else if (xz_distance < yz_distance)
		{
			OBJgRotateAxis = OBJcRotate_YAxis;
		}
		else
		{
			OBJgRotateAxis = OBJcRotate_XAxis;
		}
	}
	else if (OBJgRotateMode & OBJcRotateMode_XY)
	{
		OBJgRotateAxis = OBJcRotate_ZAxis;
	}
	else if (OBJgRotateMode & OBJcRotateMode_XZ)
	{
		OBJgRotateAxis = OBJcRotate_YAxis;
	}
	else if (OBJgRotateMode & OBJcRotateMode_YZ)
	{
		OBJgRotateAxis = OBJcRotate_XAxis;
	}
}

// ----------------------------------------------------------------------
void
OBJrObject_MouseRotate_Update(
	OBJtObject				*inObject,
	IMtPoint2D				*inPositionDelta)
{
	M3tMatrix4x3 old_rotation_matrix;
	M3tMatrix4x3 delta_rotation_matrix;
	M3tMatrix4x3 new_rotation_matrix;
	M3tPoint3D axis_for_rotation;
	float radians;
	float mouse_units_to_radians = ((float) M3cDegToRad);	// 1 mouse unit = 1 degree works well

	OBJtObjectGroup			*object_group;
	
	// make sure the object isn't locked
	if (OBJrObject_IsLocked(inObject)) { return; }
	
	// make sure the object group isn't locked
	object_group = OBJiObjectGroup_GetByType(inObject->object_type);
	if (object_group == NULL) { return; }
	if (OBJiObjectGroup_IsLocked(object_group)) { return; }
	
	// get the axis to rotate about
	axis_for_rotation = MUgZeroPoint;

	switch (OBJgRotateAxis)
	{
		case OBJcRotate_ZAxis:
			axis_for_rotation.z = 1.0f;
		break;
		
		case OBJcRotate_YAxis:
			axis_for_rotation.y = 1.0f;
		break;
		
		case OBJcRotate_XAxis:
			axis_for_rotation.x = 1.0f;
		break;
	}

	// turn that axis to local axis
	OBJrObject_GetRotationMatrix(inObject, &old_rotation_matrix);
	MUrMatrix_MultiplyPoint(&axis_for_rotation, &old_rotation_matrix, &axis_for_rotation);

	// how many radians do we need to rotate ?
	radians = inPositionDelta->x * mouse_units_to_radians;
	radians = UUmPin(radians, -M3cPi, M3cPi);	// pinning simplifies the clip
	UUmTrig_Clip(radians);
	
	// build the rotation matrix for the amount of rotation this mouse movement caused
	MUrMatrix_BuildRotate(radians, axis_for_rotation.x, axis_for_rotation.y, axis_for_rotation.z, &delta_rotation_matrix);

	// multiply the delta matrix by the old matrix
	MUrMatrix_Multiply(&delta_rotation_matrix, &old_rotation_matrix, &new_rotation_matrix);

	// set the new rotation matrix
	OBJrObject_SetRotationMatrix(inObject, &new_rotation_matrix);
}

// ----------------------------------------------------------------------
void
OBJrObject_MouseRotate_End(
	void)
{
	OBJgRotateAxis = OBJcRotate_None;
}

// ----------------------------------------------------------------------
void
OBJrObject_SetLocked(
	OBJtObject				*ioObject,
	UUtBool					inIsLocked)
{
	OBJtObjectGroup			*object_group;

	UUmAssert(ioObject);

	// make sure the object group isn't locked
	object_group = OBJiObjectGroup_GetByType(ioObject->object_type);
	if (object_group == NULL) { return; }
	if (OBJiObjectGroup_IsLocked(object_group)) { return; }
	
	if (inIsLocked)
	{
		ioObject->flags |= OBJcObjectFlag_Locked;
	}
	else
	{
		ioObject->flags &= ~OBJcObjectFlag_Locked;
	}
	
	// the object group is now dirty
	OBJiObjectGroup_SetDirty(object_group, UUcTrue);
}

// ----------------------------------------------------------------------
UUtError
OBJrObject_SetObjectSpecificData(
	OBJtObject				*ioObject,
	const OBJtOSD_All		*inObjectSpecificData)
{
	UUtError				error;
	OBJtObjectGroup			*object_group;
	
	UUmAssert(ioObject);
	UUmAssert(inObjectSpecificData);
	
	// make sure the object group isn't locked
	object_group = OBJiObjectGroup_GetByType(ioObject->object_type);
	if (object_group == NULL) { return UUcError_Generic; }
	if (OBJiObjectGroup_IsLocked(object_group)) { return UUcError_Generic; }
	
	error = ioObject->methods->rSetOSD(ioObject, inObjectSpecificData);
	UUmError_ReturnOnError(error);
	
	// the object group is now dirty
	OBJiObjectGroup_SetDirty(object_group, UUcTrue);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrObject_SetPosition(
	OBJtObject				*ioObject,
	const M3tPoint3D		*inPosition,
	const M3tPoint3D		*inRotation)
{
	OBJtObjectGroup			*object_group;
	
	UUmAssert(ioObject);
	
	// make sure the object isn't locked
	if (OBJrObject_IsLocked(ioObject)) { return; }
	
	// make sure the object group isn't locked
	object_group = OBJiObjectGroup_GetByType(ioObject->object_type);
	if (object_group == NULL) { return; }
	if (OBJiObjectGroup_IsLocked(object_group)) { return; }
	
	if (inPosition != NULL)
	{
		ioObject->position = *inPosition;
	}
	
	if (inRotation != NULL)
	{
		ioObject->rotation = *inRotation;
		
		while (ioObject->rotation.x < 0.0f) { ioObject->rotation.x += 360.0f; }
		while (ioObject->rotation.x >= 360.0f) { ioObject->rotation.x -= 360.0f; }
		
		while (ioObject->rotation.y < 0.0f) { ioObject->rotation.y += 360.0f; }
		while (ioObject->rotation.y >= 360.0f) { ioObject->rotation.y -= 360.0f; }

		while (ioObject->rotation.z < 0.0f) { ioObject->rotation.z += 360.0f; }
		while (ioObject->rotation.z >= 360.0f) { ioObject->rotation.z -= 360.0f; }
	}
	
	OBJrObject_UpdatePosition(ioObject);

	// clear the gunk flag
	ioObject->flags &= ~OBJcObjectFlag_Gunk;

	// the object group is now dirty
	OBJiObjectGroup_SetDirty(object_group, UUcTrue);
}

// ----------------------------------------------------------------------
void
OBJrObject_SetRotationMatrix(
	OBJtObject				*ioObject,
	M3tMatrix4x3			*inMatrix)
{
	MUtEuler				euler;
	M3tPoint3D				rotation;

	// make sure the object isn't locked
	if (OBJrObject_IsLocked(ioObject)) { return; }
	
	// construct the euler
	euler.order = MUcEulerOrderXYZs;
	euler = MUrMatrixToEuler(inMatrix, MUcEulerOrderXYZs);
	rotation.x = euler.x * M3cRadToDeg;
	rotation.y = euler.y * M3cRadToDeg;
	rotation.z = euler.z * M3cRadToDeg;

	// set the rotation back to the object
	OBJrObject_SetPosition(ioObject, NULL, &rotation);

	// clear the gunk flag
	ioObject->flags &= ~OBJcObjectFlag_Gunk;
}

// ----------------------------------------------------------------------
void
OBJrObject_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioNumBytes)
{
	UUtUns32				bytes_available;
	UUtUns32				bytes_used;
	OBJtObjectGroup			*object_group;
	
	// get the object group
	object_group = OBJiObjectGroup_GetByType(inObject->object_type);	
	
	// don't write into the buffer if there isn't enough space
	bytes_available = *ioNumBytes;
	bytes_used = OBJcObjectGenericWriteSize + OBJrObject_GetOSDWriteSize(inObject);

	if (bytes_available < bytes_used)
	{
		// set the number of bytes written to the buffer
		*ioNumBytes = 0;
		
		return;
	}
	
	// write the object type
	OBDmWrite4BytesToBuffer(ioBuffer, inObject->object_type, UUtUns32, bytes_available, OBJcWrite_Little);
	
	// write the object ID
	OBDmWrite4BytesToBuffer(ioBuffer, inObject->object_id, UUtUns32, bytes_available, OBJcWrite_Little);
	
	// write the flag
	OBDmWrite4BytesToBuffer(ioBuffer, inObject->flags, UUtUns32, bytes_available, OBJcWrite_Little);
	
	// write the position
	OBDmWrite4BytesToBuffer(ioBuffer, inObject->position.x, float, bytes_available, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(ioBuffer, inObject->position.y, float, bytes_available, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(ioBuffer, inObject->position.z, float, bytes_available, OBJcWrite_Little);
	
	// write the rotation
	OBDmWrite4BytesToBuffer(ioBuffer, inObject->rotation.x, float, bytes_available, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(ioBuffer, inObject->rotation.y, float, bytes_available, OBJcWrite_Little);
	OBDmWrite4BytesToBuffer(ioBuffer, inObject->rotation.z, float, bytes_available, OBJcWrite_Little);
	
	// save the number of bytes used so far
	bytes_used = *ioNumBytes - bytes_available;
	
	// write the object specific data
	inObject->methods->rWrite(inObject, ioBuffer, &bytes_available);
	
	// bytes_available is now the number of bytes written into
	// the buffer by OBJmObject_Write()
	
	// return the number of bytes written into the buffer
	*ioNumBytes = bytes_used + bytes_available;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OBJiBinaryData_Load(
	const char				*inIdentifier,
	BDtData					*ioBinaryData,
	UUtBool					inSwapIt,
	UUtBool					inLocked,
	UUtBool					inAllocated)
{
	UUtUns8					*buffer;
	UUtUns32				version;
	UUtUns32				num_bytes;
	OBJtObjectGroup			*object_group;
	
	UUmAssert(inIdentifier);
	UUmAssert(ioBinaryData);
	
	object_group = OBJiObjectGroup_GetByName(inIdentifier);

	if (NULL == object_group) {
		UUmError_ReturnOnErrorMsgP(UUcError_Generic, "failed to find object group %s", (UUtUns32) inIdentifier, 0, 0);
	}

	OBJiObjectGroup_DeleteAllObjects(object_group);
	
	buffer = ioBinaryData->data;
	
	// read the version number
	OBDmGet4BytesFromBuffer(buffer, version, UUtUns32, inSwapIt);
	
	// get the size of the data chunk
	OBDmGet4BytesFromBuffer(buffer, num_bytes, UUtUns32, inSwapIt);
	while (num_bytes > 0)
	{
		UUtUns32			read_bytes;
		
		// process the data chunk
		OBJrObject_CreateFromBuffer((UUtUns16)version, inSwapIt, buffer, &read_bytes);
		
		// advance to next data chunk
		buffer += num_bytes;
		
		// get the size of the data chunk
		OBDmGet4BytesFromBuffer(buffer, num_bytes, UUtUns32, inSwapIt);
	}
	
	// set the locked and dirty flags
	OBJiObjectGroup_SetLocked(object_group, inLocked);
	OBJiObjectGroup_SetDirty(object_group, UUcFalse);
	
	if (inAllocated) {
		UUrMemory_Block_Delete(ioBinaryData);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiBinaryData_Register(
	void)
{
	UUtError				error;
	BDtMethods				methods;
	
	methods.rLoad = OBJiBinaryData_Load;
	
	error =
		BDrRegisterClass(
			OBJcBinaryDataClass,
			&methods);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
#if TOOL_VERSION
static void
OBJiBinaryData_Save(
	UUtUns32				inObjectType,
	UUtBool					inAutoSave)
{
	UUtUns32				i;
	UUtUns16				level_id;
	UUtUns8					block[OBJcMaxBufferSize + (2 * UUcProcessor_CacheLineSize)];
	UUtUns8					*temp_buffer = UUrAlignMemory(block);

	UUrMemory_Block_VerifyList();
	
	// save the objects
	for (i = 0; i < OBJiObjectGroups_GetNumGroups(); i++)
	{
		OBJtObjectGroup		*object_group;
		UUtUns32			j;
		UUtUns32			num_objects;
		UUtUns32			data_size;
		UUtUns8				*data;
		UUtUns8				*buffer;
		UUtUns32			num_bytes;
		
		object_group = OBJiObjectGroups_GetGroupByIndex(i);
		if (object_group == NULL)								{ continue; }
		if (OBJiObjectGroup_IsLocked(object_group) == UUcTrue)	{ continue; }
		if (OBJiObjectGroup_IsDirty(object_group) == UUcFalse)	{ continue; }
		if ((inObjectType != OBJcType_None) && (OBJiObjectGroup_GetType(object_group) != inObjectType))
			{ continue; }
		
		num_objects = OBJiObjectGroup_GetNumObjects(object_group);
			
		data_size =
			sizeof(UUtUns32) +								// version number 
			(sizeof(UUtUns32) * num_objects) +				// sizes of the object data 
			OBJiObjectGroup_GetWriteSize(object_group) +	// object data 
			sizeof(UUtUns32);								// zero at end of file 
		
		data = UUrMemory_Block_NewClear(data_size);
		if (data == NULL) { UUmAssert(!"Unable to allocate memory to save objects"); continue; }
		
		// set write variables
		buffer = data;
		num_bytes = data_size;
		
		// write the version number into the buffer
		OBDmWrite4BytesToBuffer(buffer, OBJcCurrentVersion, UUtUns32, num_bytes, OBJcWrite_Little);
		
		for (j = 0; j < num_objects; j++)
		{
			OBJtObject		*object;
			UUtUns32		temp_num_bytes;
			
			UUrMemory_Clear(temp_buffer, OBJcMaxBufferSize);
			UUrMemory_Block_VerifyList();
				
			object = OBJiObjectGroup_GetObject(object_group, j);
			if (object == NULL)								continue;
			if (object->flags & OBJcObjectFlag_Temporary)	continue;
			
			// write the object into the temp buffer
			temp_num_bytes = OBJcMaxBufferSize;
			OBJrObject_Write(object, temp_buffer, &temp_num_bytes);
			UUmAssert(temp_num_bytes == (OBJcObjectGenericWriteSize + OBJrObject_GetOSDWriteSize(object)));
			
			// align
			temp_num_bytes = UUmMakeMultiple(temp_num_bytes, 4);

			// write the number of bytes
			OBDmWrite4BytesToBuffer(buffer, temp_num_bytes, UUtUns32, num_bytes, OBJcWrite_Little);
			UUmAssert(temp_num_bytes <= num_bytes);
			
			// write the temp_buffer
			UUrMemory_MoveFast(temp_buffer, buffer, temp_num_bytes);
						
			buffer += temp_num_bytes;
			num_bytes -= temp_num_bytes;

			UUrMemory_Block_VerifyList();
		}
		
/*		for (j = 0; j < num_objects; j++)
		{
			OBJtObject		*object;
			UUtUns32		*size_ptr;
			UUtUns32		length_of_chunk;

			UUrMemory_Block_VerifyList();
						
			object = OBJiObjectGroup_GetObject(object_group, j);
			if (object == NULL) { continue; }

			// get a pointer to where the size goes
			size_ptr = (UUtUns32 *) buffer;
			buffer += 4;
			UUmAssert(num_bytes > 4);
			num_bytes -= 4;
			
			// write the object into the buffer
			length_of_chunk = num_bytes;
			OBJrObject_Write(object, buffer, &length_of_chunk);

			UUmAssert(length_of_chunk == (OBJcObjectGenericWriteSize + OBJmObject_GetOSDWriteSize(object)));

			// align the chunk size
			length_of_chunk = UUmMakeMultiple(length_of_chunk, 4);

			// record the size and advance the buffer pointer
			UUmAssert(num_bytes >= length_of_chunk);
			*size_ptr = length_of_chunk;
			buffer += length_of_chunk;
			num_bytes -= length_of_chunk;

			UUrMemory_Block_VerifyList();
		}*/
		
		UUrMemory_Block_VerifyList();

		// there are no more objects for this file, write a size of zero, use j to hold the zero
		j = 0;
		OBDmWrite4BytesToBuffer(buffer, j, UUtUns32, num_bytes, OBJcWrite_Little);
		
		UUrMemory_Block_VerifyList();
		
		level_id = (object_group->flags & OBJcObjectGroupFlag_CommonToAllLevels) ? 0 : ONrLevel_GetCurrentLevel();

		// write the buffer to the binary datafile
		OBDrBinaryData_Save(
			OBJcBinaryDataClass,
			object_group->group_name,
			data,
			(data_size - num_bytes),
			level_id,
			inAutoSave);

		UUrMemory_Block_VerifyList();
		
		// dispose of the buffer
		UUrMemory_Block_Delete(data);
		
		// the object group is no longer dirty if this wasn't an autosave
		if (inAutoSave == UUcFalse)
		{
			OBJiObjectGroup_SetDirty(object_group, UUcFalse);
		}

		UUrMemory_Block_VerifyList();
	}
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBJrLevel_Load(
	UUtUns16				inLevelNumber)
{
	UUtError				error;
	
	// initialize the object list
	error = OBJiObjectGroups_LevelLoad();
	UUmError_ReturnOnError(error);
	
	OBJrSelectedObjects_Select(NULL, UUcFalse);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrLevel_Unload(
	void)
{
#if TOOL_VERSION
	// save the objects
	OBJiBinaryData_Save(OBJcType_None, UUcFalse);
#endif
	
	OBJiObjectGroups_LevelUnload();

	return;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// =====================================================================

// ----------------------------------------------------------------------
static UUtUns32
OBJiENVFile_GetNumNodes(
	void)
{
	OBJtObject				*object;
	OBJtObjectGroup			*object_group;
	UUtUns32				num_objects;
	UUtUns32				i;
	UUtUns32				num_nodes;
	
	num_nodes = 0;

	////////////////////////////////////////////////////////////////////////////////////////////////
	//Furniture
	object_group = OBJiObjectGroup_GetByType(OBJcType_Furniture);
	if (object_group == NULL) { return 0; }
	
	num_objects = OBJiObjectGroup_GetNumObjects(object_group);
	
	for (i = 0; i < num_objects; i++)
	{
		OBJtOSD_Furniture	*furniture_osd;
		
		object = OBJiObjectGroup_GetObject(object_group, i);
		if( !object )									return 0;
		if( object->object_type != OBJcType_Furniture) { continue; }
		
		furniture_osd = (OBJtOSD_Furniture*)object->object_data;
		num_nodes += (furniture_osd->furn_geom_array != NULL) ? furniture_osd->furn_geom_array->num_furn_geoms : 0;
	}

	//COrConsole_Printf("furniture %d", num_nodes);

	////////////////////////////////////////////////////////////////////////////////////////////////
	//Turrets
	object_group = OBJiObjectGroup_GetByType(OBJcType_Turret);
	num_objects = OBJiObjectGroup_GetNumObjects(object_group);	

	for (i = 0; i < num_objects; i++)
	{
		object = OBJiObjectGroup_GetObject(object_group, i);

		if ((object == NULL) || (object->object_type != OBJcType_Turret)) { continue; }
		if( object->flags & OBJcObjectFlag_Temporary )	continue;

		num_nodes++;
	}

	//COrConsole_Printf("turrets %d", num_nodes);

	////////////////////////////////////////////////////////////////////////////////////////////////
	//Triggers
	object_group = OBJiObjectGroup_GetByType(OBJcType_Trigger);
	num_objects = OBJiObjectGroup_GetNumObjects(object_group);	

	for (i = 0; i < num_objects; i++)
	{
		object = OBJiObjectGroup_GetObject(object_group, i);

		if ((object == NULL) || (object->object_type != OBJcType_Trigger)) { continue; }
		if( object->flags & OBJcObjectFlag_Temporary )			continue;

		num_nodes++;
	}

	//COrConsole_Printf("triggers %d", num_nodes);

	////////////////////////////////////////////////////////////////////////////////////////////////
	//Consoles
	object_group = OBJiObjectGroup_GetByType(OBJcType_Console);
	if (object_group == NULL) { return 0; }
	
	num_objects = OBJiObjectGroup_GetNumObjects(object_group);
	
	for (i = 0; i < num_objects; i++)
	{
		OBJtOSD_Console		*console_osd;
		
		object = OBJiObjectGroup_GetObject(object_group, i);
		if( !object )										continue;
		if( object->object_type != OBJcType_Console) { continue; }
		if( object->flags & OBJcObjectFlag_Temporary )	continue;
		
		console_osd = (OBJtOSD_Console*) object->object_data;
		num_nodes += console_osd->console_class->geometry_array->num_furn_geoms;
	}
	//COrConsole_Printf("consoles %d", num_nodes);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// The Doors

	object_group = OBJiObjectGroup_GetByType(OBJcType_Door);
	if (object_group == NULL) { return 0; }
	
	num_objects = OBJiObjectGroup_GetNumObjects(object_group);
	
	for (i = 0; i < num_objects; i++)
	{
		OBJtOSD_Door		*door_osd;
		
		object = OBJiObjectGroup_GetObject(object_group, i);
		if( !object )									continue;
		if( object->object_type != OBJcType_Door)		continue;
		if( object->flags & OBJcObjectFlag_Temporary )	continue;
		
		door_osd = (OBJtOSD_Door*) object->object_data;

		if (door_osd->door_class != NULL) {
			num_nodes += door_osd->door_class->geometry_array[0]->num_furn_geoms * 2;
			if( door_osd->flags & OBJcDoorFlag_DoubleDoors )
			{
				if( door_osd->door_class->geometry_array[1] ) {
					num_nodes += door_osd->door_class->geometry_array[1]->num_furn_geoms * 2;
				}
				else {
					num_nodes += door_osd->door_class->geometry_array[0]->num_furn_geoms * 2;
				}
			}
		}
	}
	//COrConsole_Printf("doors %d", num_nodes);

	
	return num_nodes;
}

// ----------------------------------------------------------------------
static UUtError
OBJiENVFile_WriteNode_Geometry(
	BFtFile					*inFile,
	M3tMatrix4x3			*inMatrix,
	M3tGeometry				*inGeometry,
	UUtUns32				inGQFlags,
	OBJtLSData				*inLsData,
	char					*inName,
	char					*inUserData,
	M3tMatrix4x3			*inDeformMatrix,
	UUtUns32				inParticleCount,
	EPtEnvParticle			*inParticleArray,
	M3tTextureMap			*inTextureMap,
	BFtFile					*inDebugFile)
{
	UUtError				error;
	char					object_name[255];	
	char					*geom_name;
	MXtNode					*node;
	MXtNode					*optimized_node;
	MXtPoint				*points;
	MXtFace					*tri_faces;
	MXtMaterial				*materials;
	MXtMarker				*markers;
	M3tGeometry				*geometry;
	UUtUns16				i;
#if (UUmEndian == UUmEndian_Big)
	UUtUns16				j;
#endif
	UUtUns32				points_size;
	UUtUns32				tri_faces_size;
	UUtUns32				materials_size;
	UUtUns32				markers_size;
	
	UUtUns32				index0;
	UUtUns32				index1;
	UUtUns32				index2;
	UUtUns32				*curIndexPtr;

	EPtEnvParticle			*particle;

	M3tMatrix4x3			deform_matrix;
	
	char					user_data[2048];
	UUtUns32				user_data_length;

	char					*particle_user_data_buf;
	char					**particle_user_data;
	UUtUns32				*particle_user_data_length;
		
	if( inName )
		UUrString_Copy( object_name, inName, 255 );
	else
		object_name[0] = 0;

	// get a pointer to the geometry being written as a node
	geometry = inGeometry;

	// get the name for this geometry
	geom_name = TMrInstance_GetInstanceName(geometry);
	if( !geom_name )
	{
		geom_name = object_name;
	}
	
	if( inDeformMatrix )
		deform_matrix = *inDeformMatrix;
	else
		MUrMatrix_Identity(&deform_matrix);


	// ------------------------------
	// build the user data
	UUrMemory_Clear(user_data, 2048);
	user_data_length = 0;
	if (inGQFlags != 0)
	{
		strcat(user_data, "$type = ["UUmNL);
		
		for (i = 0; i < 32; i++)
		{
			if (inGQFlags & (1 << i))
			{
				const char *flag_name = AUrFlags_GetTextName(OBJgGunkFlags, (1 << i));

				if (NULL != flag_name) {
					strcat(user_data, flag_name);
					strcat(user_data, UUmNL);
				}
			}
		}
		
		strcat(user_data, "]"UUmNL);
	}
	
	if (inLsData)
	{
		OBJtLSData			*ls_data;
		char				temp_buffer[128];
		ls_data = inLsData;
		// find the ls_data corresponding to this geometry
		if (ls_data != NULL)
		{			
			// write the light data
			strcat(user_data, "$ls = {"UUmNL);
			
			// write the light filter color
			strcat(user_data, "$filterColor = [ ");
			sprintf(
				temp_buffer,
				"%5.3f, %5.3f, %5.3f ]"UUmNL,
				ls_data->filter_color[0],
				ls_data->filter_color[1],
				ls_data->filter_color[2]);
			strcat(user_data, temp_buffer);
			
			// write the light type
			strcat(user_data, "$type = ");
			if (ls_data->light_flags & OBJcLightFlag_Type_Area)
			{
				sprintf(temp_buffer, "area"UUmNL);
			}
			else if (ls_data->light_flags & OBJcLightFlag_Type_Linear)
			{
				sprintf(temp_buffer, "linear"UUmNL);
			}
			else
			{
				sprintf(temp_buffer, "point"UUmNL);
			}
			strcat(user_data, temp_buffer);
			
			// write the light distribution
			strcat(user_data, "$distribution = ");
			if (ls_data->light_flags & OBJcLightFlag_Dist_Diffuse)
			{
				sprintf(temp_buffer, "diffuse"UUmNL);
			}
			else
			{
				sprintf(temp_buffer, "spot"UUmNL);
			}
			strcat(user_data, temp_buffer);
			
			// write the light intensity
			sprintf(temp_buffer, "$intensity = %f"UUmNL, ls_data->light_intensity);
			strcat(user_data, temp_buffer);
			
			// write the light beam angle
			sprintf(temp_buffer, "$beamAngle = %f"UUmNL, ls_data->beam_angle);
			strcat(user_data, temp_buffer);
			
			// write the light field angle
			sprintf(temp_buffer, "$fieldAngle = %f"UUmNL, ls_data->field_angle);
			strcat(user_data, temp_buffer);

			strcat(user_data, "}"UUmNL);
		}
		else
		{
			UUmAssert(!"furn_geom with ls_data didn't have corresponding furn_osd->ls_data");
		}
	}
	if( inUserData )
		strcat( user_data, inUserData );
	strcat(user_data, "\0");
	
	user_data_length = strlen(user_data) + 1;
	
	// ------------------------------
	// allocate memory for the node
	node = (MXtNode*)UUrMemory_Block_NewClear(sizeof(MXtNode));
	UUmError_ReturnOnNull(node);

	// copy the name and parentName into the node
	UUrString_Copy(node->name,		 geom_name, MXcMaxName);
	UUrString_Copy(node->parentName, "",		MXcMaxName);
	
	// set the node data
	node->numPoints		= (UUtUns16) geometry->pointArray->numPoints;
	node->numTriangles	= (UUtUns16) geometry->triNormalIndexArray->numIndices;
	node->numMaterials	= 1;
	node->numMarkers	= (UUtUns16) inParticleCount;
	node->userDataCount = user_data_length;
	node->matrix		= *inMatrix;

#if (UUmEndian == UUmEndian_Big)
	UUrSwap_2Byte(&node->numPoints);
	UUrSwap_2Byte(&node->numTriangles);
	UUrSwap_2Byte(&node->numMaterials);
	UUrSwap_2Byte(&node->numMarkers);
	UUrSwap_4Byte(&node->userDataCount);
	
	for (i = 0; i < 4; i++)
	{
		UUrSwap_4Byte(&node->matrix.m[i][0]);
		UUrSwap_4Byte(&node->matrix.m[i][1]);
		UUrSwap_4Byte(&node->matrix.m[i][2]);
	}
#endif
		
	// ------------------------------
	// make the points
	points_size = sizeof(MXtPoint) * geometry->pointArray->numPoints;
	points = (MXtPoint*)UUrMemory_Block_NewClear(points_size);
	UUmError_ReturnOnNull(points);
	
	for (i = 0; i < geometry->pointArray->numPoints; i++)
	{
#if 1
		MUrMatrix_MultiplyPoint(  &geometry->pointArray->points[i],			&deform_matrix, &points[i].point );
#else
		MUrMatrix_MultiplyNormal( &geometry->vertexNormalArray->vectors[i], &deform_matrix, &points[i].normal );
		points[i].point		= geometry->pointArray->points[i];
#endif
		points[i].normal	= geometry->vertexNormalArray->vectors[i];
		points[i].uv		= geometry->texCoordArray->textureCoords[i];
		
		points[i].uv.v = 1.0f - points[i].uv.v;
		
#if (UUmEndian == UUmEndian_Big)
		UUrSwap_4Byte(&points[i].point.x);
		UUrSwap_4Byte(&points[i].point.y);
		UUrSwap_4Byte(&points[i].point.z);
		UUrSwap_4Byte(&points[i].normal.x);
		UUrSwap_4Byte(&points[i].normal.y);
		UUrSwap_4Byte(&points[i].normal.z);
		UUrSwap_4Byte(&points[i].uv.u);
		UUrSwap_4Byte(&points[i].uv.v);
#endif
	}
	
	// ------------------------------
	// make the triangles
	tri_faces_size = sizeof(MXtFace) * geometry->triNormalIndexArray->numIndices;
	tri_faces = (MXtFace*)UUrMemory_Block_NewClear(tri_faces_size);
	UUmError_ReturnOnNull(tri_faces);
	
	curIndexPtr = geometry->triStripArray->indices;
	
	for (i = 0; i < geometry->triNormalIndexArray->numIndices; i++)
	{
		UUtUns32				normal_index;
		M3tVector3D				v1;
		M3tVector3D				v2;
		
		index2 = *curIndexPtr++;
		
		if (index2 & 0x80000000)
		{
			index0 = index2 & 0x7FFFFFFF;
			index1 = *curIndexPtr++;
			index2 = *curIndexPtr++;
			
			UUmAssert((index0 & 0x80000000) != 0x80000000);
			UUmAssert((index1 & 0x80000000) != 0x80000000);
			UUmAssert((index2 & 0x80000000) != 0x80000000);
		}
		
		normal_index = geometry->triNormalIndexArray->indices[i];
		UUmAssert((normal_index & 0x80000000) != 0x80000000);
		
		UUmAssert(index0 < UUcMaxUns16);
		UUmAssert(index1 < UUcMaxUns16);
		UUmAssert(index2 < UUcMaxUns16);
		
		MUmVector_Subtract(v1, geometry->pointArray->points[index1], geometry->pointArray->points[index0]);
		MUmVector_Subtract(v2, geometry->pointArray->points[index2], geometry->pointArray->points[index0]);
				
		tri_faces[i].indices[0]	= (UUtUns16) index0;
		tri_faces[i].indices[1]	= (UUtUns16) index1;
		tri_faces[i].indices[2]	= (UUtUns16) index2;
		tri_faces[i].indices[3]	= 0;		
		tri_faces[i].dont_use_this_normal			= geometry->triNormalArray->vectors[normal_index];
		tri_faces[i].material		= 0;
		tri_faces[i].pad			= 0;

#if (UUmEndian == UUmEndian_Big)
		UUrSwap_2Byte(&tri_faces[i].indices[0]);
		UUrSwap_2Byte(&tri_faces[i].indices[1]);
		UUrSwap_2Byte(&tri_faces[i].indices[2]);
		UUrSwap_2Byte(&tri_faces[i].indices[3]);
		UUrSwap_4Byte(&tri_faces[i].dont_use_this_normal.x);
		UUrSwap_4Byte(&tri_faces[i].dont_use_this_normal.y);
		UUrSwap_4Byte(&tri_faces[i].dont_use_this_normal.z);
		UUrSwap_2Byte(&tri_faces[i].material);
#endif

		index0 = index1;
		index1 = index2;
	}
	
	// ------------------------------
	// make the materials
	materials_size	= sizeof(MXtMaterial);
	materials		= (MXtMaterial*)UUrMemory_Block_NewClear(materials_size);
	UUmError_ReturnOnNull(materials);
	
	{
		M3tTextureMap		*texture;
		char				texture_name[ MXcMaxName];
				
		if( inTextureMap )
		{
			texture = inTextureMap;
		}
		else
		{
			texture = geometry->baseMap;
		}

		if( !texture ) 
		{
			UUrString_Copy( texture_name, "null_material", MXcMaxName);
		} 
		else 
		{
			UUrString_Copy( texture_name, TMrInstance_GetInstanceName(texture), MXcMaxName);
		}
		UUrString_Copy(materials->name, texture_name, MXcMaxName);

		materials->alpha = 1.0f;
		
		for (i = 0; i < MXcMapping_Count; i++)
		{
			UUrString_Copy(materials->maps[i].name, "<none>", MXcMaxName);
		}
		
		UUrString_Copy(materials->maps[MXcMapping_DI].name, texture_name, MXcMaxName);
		materials->maps[MXcMapping_DI].amount = 1.0f;
	}
	
#if (UUmEndian == UUmEndian_Big)
	UUrSwap_4Byte(&materials->alpha);
	UUrSwap_4Byte(&materials->maps[MXcMapping_DI].amount);
#endif
	
	if (inParticleCount > 0) 
	{
		char temp_buffer[256];

		// ------------------------------
		// make the markers
		markers_size = sizeof(MXtMarker) * inParticleCount;
		markers = (MXtMarker *) UUrMemory_Block_NewClear(markers_size);
		UUmError_ReturnOnNull(markers);

		particle_user_data_buf = (char *) UUrMemory_Block_NewClear(inParticleCount * OBJcParticleMarkerUserDataMax);
		particle_user_data = (char **) UUrMemory_Block_New(inParticleCount * sizeof(char *));
		particle_user_data_length = (UUtUns32 *) UUrMemory_Block_New(inParticleCount * sizeof(UUtUns32));

		for (i = 0, particle = inParticleArray; i < inParticleCount; i++, particle++) {
			// set the marker's data
			sprintf(markers[i].name, "envp_%s", particle->tagname);

			MUrMatrix_Multiply(inMatrix, &particle->matrix, &markers[i].matrix);

#if (UUmEndian == UUmEndian_Big)
			for (j = 0; j < 4; j++)
			{
				UUrSwap_4Byte(&markers[i].matrix.m[j][0]);
				UUrSwap_4Byte(&markers[i].matrix.m[j][1]);
				UUrSwap_4Byte(&markers[i].matrix.m[j][2]);
			}
#endif
			// set up the buffer pointer
			particle_user_data[i] = particle_user_data_buf + i * OBJcParticleMarkerUserDataMax;

			markers[i].userDataCount = 0;
			markers[i].userData = NULL;

			// form the marker's user data
			sprintf(temp_buffer, "$particle_class = %s"UUmNL, particle->classname);
			strcat(particle_user_data[i], temp_buffer);

			sprintf(temp_buffer, "$tag = %s"UUmNL, particle->tagname);
			strcat(particle_user_data[i], temp_buffer);

			strcat(particle_user_data[i], "$flags = [ ");
			if (particle->flags & EPcParticleFlag_NotInitiallyCreated) {
				strcat(particle_user_data[i], "not-created ");
			}
			strcat(particle_user_data[i], "]"UUmNL);

			sprintf(temp_buffer, "$decal_x = %f"UUmNL, particle->decal_xscale);
			strcat(particle_user_data[i], temp_buffer);

			sprintf(temp_buffer, "$decal_y = %f"UUmNL, particle->decal_yscale);
			strcat(particle_user_data[i], temp_buffer);

			particle_user_data_length[i] = strlen(particle_user_data[i]) + 1;

			markers[i].userDataCount = particle_user_data_length[i];
#if (UUmEndian == UUmEndian_Big)
			UUrSwap_4Byte(&markers[i].userDataCount);
#endif
		}
	}

	node->points = points;
	node->quads = NULL;
	node->triangles = tri_faces;

	optimized_node = M3rOptimizeNode(node, inDebugFile);

	node->points = optimized_node->points;
	node->numPoints = optimized_node->numPoints;

	node->triangles = optimized_node->triangles;
	node->numTriangles = optimized_node->numTriangles;

	node->quads = optimized_node->quads;
	node->numQuads = optimized_node->numQuads;

	// write the node
	error = BFrFile_Write(inFile, sizeof(MXtNode), node);
	UUmError_ReturnOnError(error);
	
	// ------------------------------
	// write user data
	BFrFile_Write(inFile, user_data_length, user_data);
			
	// ------------------------------
	// write points
	error = BFrFile_Write(inFile, sizeof(MXtPoint) * node->numPoints, node->points);
	UUmError_ReturnOnError(error);
	
	// ------------------------------
	// write triangles
	error = BFrFile_Write(inFile, sizeof(MXtFace) * node->numTriangles, node->triangles);
	UUmError_ReturnOnError(error);
	
	// ------------------------------
	// write quads
	error = BFrFile_Write(inFile, sizeof(MXtFace) * node->numQuads, node->quads);
	UUmError_ReturnOnError(error);

	// ------------------------------
	// write markers
	for (i = 0; i < inParticleCount; i++) {
		error = BFrFile_Write(inFile, sizeof(MXtMarker), &markers[i]);
		UUmError_ReturnOnError(error);
	
		error = BFrFile_Write(inFile, particle_user_data_length[i], particle_user_data[i]);
		UUmError_ReturnOnError(error);
	}

	// ------------------------------
	// write materials
	error = BFrFile_Write(inFile, materials_size, materials);
	UUmError_ReturnOnError(error);
	
	// ------------------------------
	// free the memory
	UUrMemory_Block_Delete(node);
	UUrMemory_Block_Delete(optimized_node);
	UUrMemory_Block_Delete(points);
	UUrMemory_Block_Delete(tri_faces);
	UUrMemory_Block_Delete(materials);
	if (inParticleCount > 0) {
		UUrMemory_Block_Delete(markers);
		UUrMemory_Block_Delete(particle_user_data_buf);
		UUrMemory_Block_Delete(particle_user_data_length);
		UUrMemory_Block_Delete(particle_user_data);
	}

	OBJgNodeActual++;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError OBJiENVFile_WriteNode_GeometryArray(
	BFtFile					*inFile,
	M3tMatrix4x3			*inMatrix,
	OBJtFurnGeomArray		*inFurnGeomArray,
	OBJtLSData				*inLsDataArray,
	char					*inName,
	char					*inUserData,
	M3tMatrix4x3			*inDeformMatrix,
	UUtUns32				inParticleCount,
	EPtEnvParticle			*inParticleArray,
	UUtUns32				inGQFlags_TurnOn,
	UUtUns32				inGQFlags_TurnOff,
	UUtUns32				inTestFlags_Require,
	UUtUns32				inTestFlags_Skip,
	UUtUns32				inTestFlags_TurnOn,
	UUtUns32				inTestFlags_TurnOff,
	M3tTextureMap			*inTextureMap,
	BFtFile					*inDebugFile )
{
	UUtError				error;
	UUtUns32				g;
	char					user_data[255];
	UUtUns32				j;
	OBJtLSData				*ls_data;
	UUtUns32				new_flags;

	if (NULL == inFurnGeomArray) {
		return UUcError_None;
	}

	for( g = 0; g < inFurnGeomArray->num_furn_geoms; g++ )
	{
		// get a pointer to the geometry being written as a node
		ls_data = NULL;
		if( inLsDataArray )
		{
			// find the ls_data corresponding to this geometry
			for (j = 0; j < inFurnGeomArray->num_furn_geoms; j++)
			{
				ls_data = inLsDataArray + j;
				if (ls_data->index == g) { break; }
				ls_data = NULL;
			}
		}
		
		sprintf( user_data, "$node_index = %d"UUmNL, g );
		if( inUserData )
			strcat( user_data, inUserData );
		
		new_flags = inFurnGeomArray->furn_geom[g].gq_flags;

		if (((new_flags & inTestFlags_Require) == inTestFlags_Require) &&
			((new_flags & inTestFlags_Skip) == 0)) {
			new_flags = (new_flags & ~inTestFlags_TurnOff) | inTestFlags_TurnOn;
		}
		new_flags = ((new_flags & ~inGQFlags_TurnOff) | inGQFlags_TurnOn);
		
		error = OBJiENVFile_WriteNode_Geometry( inFile, inMatrix, inFurnGeomArray->furn_geom[g].geometry,
			new_flags, ls_data, inName, user_data, inDeformMatrix,
			inParticleCount, inParticleArray, inTextureMap, inDebugFile );
		UUmError_ReturnOnError( error );

		// write all particles in the first node
		inParticleCount = 0;
		inParticleArray = NULL;
	}
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError OBJrENVFile_Write(UUtBool inDebug)
{
	UUtError				error;
	UUtUns16				level_num;
	BFtFileRef				*env_file_ref, *debug_file_ref;
	BFtFile					*env_file, *debug_file;
	char					filename[128];
	MXtHeader				*header;
	UUtUns32				i;
	UUtUns32				num_objects;
	OBJtObjectGroup			*object_group;
	OBJtObject				*object;
	char					user_data[100];
	char					object_name[100];
	M3tMatrix4x3			matrix;
	M3tMatrix4x3			m1, m2;
	M3tMatrix4x3			m1b, m2b;
	UUtUns32				flags, tag;
	
	// get the current level number
	level_num = ONrLevel_GetCurrentLevel();
	if (level_num == 0) { return UUcError_Generic; }
	
	// create the env file ref
	sprintf(filename, "L%d_Gunk.ENV", level_num);
	error = BFrFileRef_MakeFromName(filename, &env_file_ref);
	UUmError_ReturnOnError(error);
	
	// create the .ENV file if it doesn't already exist
	if (BFrFileRef_FileExists(env_file_ref) == UUcFalse)
	{
		// create the object.env file
		error = BFrFile_Create(env_file_ref);
		UUmError_ReturnOnError(error);
	}
	
	// open the object.env file
	error = BFrFile_Open(env_file_ref, "rw", &env_file);
	UUmError_ReturnOnError(error);
	
	// set the position to 0
	error = BFrFile_SetPos(env_file, 0);
	UUmError_ReturnOnError(error);

	if (inDebug) {
		// create the debug file ref
		sprintf(filename, "L%d_GunkDebug.txt", level_num);
		error = BFrFileRef_MakeFromName(filename, &debug_file_ref);
		UUmError_ReturnOnError(error);
		
		// create the debug file if it doesn't already exist
		if (BFrFileRef_FileExists(debug_file_ref) == UUcFalse)
		{
			// create the debug file
			error = BFrFile_Create(debug_file_ref);
			UUmError_ReturnOnError(error);
		}
		
		// open the debug file
		error = BFrFile_Open(debug_file_ref, "w", &debug_file);
		UUmError_ReturnOnError(error);	
		
		BFrFile_Printf(debug_file, "debug info for level %d gunk file..."UUmNL, level_num);
		BFrFile_Printf(debug_file, UUmNL);
	} else {
		debug_file_ref = NULL;
		debug_file = NULL;
	}
	
	// create the MXtHeader
	header = (MXtHeader*)UUrMemory_Block_NewClear(sizeof(MXtHeader));
	UUmError_ReturnOnNull(header);
	
	UUrString_Copy(header->stringENVF, "ENVF", 5);
	header->version = 0;
	header->numNodes = (UUtUns16) OBJiENVFile_GetNumNodes();
	header->time = 0;
	header->nodes = NULL;

	OBJgNodeCount = OBJiENVFile_GetNumNodes();
	OBJgNodeActual = 0;
		
#if (UUmEndian == UUmEndian_Big)
	UUrSwap_4Byte(&header->version);
	UUrSwap_2Byte(&header->numNodes);
	UUrSwap_2Byte(&header->time);	
#endif
	
	// write the header
	error = BFrFile_Write(env_file, sizeof(MXtHeader), header);
	UUmError_ReturnOnError(error);
	
	// dispose of the memory
	UUrMemory_Block_Delete(header);
	header = NULL;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Furniture
	object_group = OBJiObjectGroup_GetByType(OBJcType_Furniture);
	if (object_group == NULL) { return UUcError_Generic; }

	if (debug_file) {
		BFrFile_Printf(debug_file, "FURNITURE"UUmNL);
		BFrFile_Printf(debug_file, "========="UUmNL);
	}

	num_objects = OBJiObjectGroup_GetNumObjects(object_group);
	for (i = 0; i < num_objects; i++)
	{
		OBJtOSD_Furniture	*furniture_osd;

		object = OBJiObjectGroup_GetObject(object_group, i);
		if ((object == NULL) || (object->object_type != OBJcType_Furniture)) { continue; }
		if( object->flags & OBJcObjectFlag_Temporary )			continue;
		
		furniture_osd = (OBJtOSD_Furniture*) object->object_data;

		tag = OBJmMakeObjectTag(object_group->object_typeindex, object);

		sprintf( object_name,	"ob_%d_%d",				object_group->object_typeindex, object->object_id );
		sprintf( user_data,		"$object_tag = %d"UUmNL, tag );

		OBJrObject_GetRotationMatrix(object, &m1);
		MUrMatrix_Identity(&matrix);
		MUrMatrixStack_Translate(&matrix, &object->position);
		MUrMatrixStack_Matrix(&matrix, &m1);
		
		if (debug_file) {
			BFrFile_Printf(debug_file, "furniture #%d geom %s furntag %s, typeindex %d objID %d -> tag %d"UUmNL,
				i, furniture_osd->furn_geom_name, furniture_osd->furn_tag, object_group->object_typeindex, object->object_id, tag);
		}

		// write out furniture... set the no-occlusion and furniture flags on. for any quads which are no-obj-collision and invisible
		// and which characters can collide with, these are likely to be invisible collision volumes.
		// make sure that they get tagged as AI impassable.
		error = OBJiENVFile_WriteNode_GeometryArray(env_file, &matrix, furniture_osd->furn_geom_array, furniture_osd->ls_data_array,
					object_name, user_data, NULL, furniture_osd->num_particles, furniture_osd->particle,
					AKcGQ_Flag_No_Occlusion | AKcGQ_Flag_Furniture, 0, 
					AKcGQ_Flag_No_Object_Collision | AKcGQ_Flag_Invisible, AKcGQ_Flag_No_Collision | AKcGQ_Flag_No_Character_Collision, 
					AKcGQ_Flag_AIImpassable, 0, NULL, debug_file);
		if (error != UUcError_None) { break; }
	}

	//COrConsole_Printf("actual");
	//COrConsole_Printf("furniture %d", OBJgNodeActual);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//Turrets
	object_group = OBJiObjectGroup_GetByType(OBJcType_Turret);
	if (object_group == NULL) { return UUcError_Generic; }
	
	if (debug_file) {
		BFrFile_Printf(debug_file, "TURRETS"UUmNL);
		BFrFile_Printf(debug_file, "======="UUmNL);
		BFrFile_Printf(debug_file, UUmNL);
	}

	num_objects = OBJiObjectGroup_GetNumObjects(object_group);
	for (i = 0; i < num_objects; i++)
	{
		OBJtOSD_Turret		*turret_osd;

		object = OBJiObjectGroup_GetObject(object_group, i);
		if ((object == NULL) || (object->object_type != OBJcType_Turret)) { continue; }
		if( object->flags & OBJcObjectFlag_Temporary )			continue;
		
		turret_osd = (OBJtOSD_Turret*) object->object_data;

		tag = OBJmMakeObjectTag(object_group->object_typeindex, object);

		sprintf( object_name,	"ob_%d_%d",				object_group->object_typeindex, object->object_id );
		sprintf( user_data,		"$object_tag = %d"UUmNL, tag );

		OBJrObject_GetRotationMatrix(object, &m1);
 		MUrMatrix_Identity(&matrix);
		MUrMatrixStack_Translate(&matrix, &object->position);
		MUrMatrixStack_Matrix(&matrix, &m1);
		
		// always set the no occlusion flag
		flags = (turret_osd->turret_class->base_gq_flags | AKcGQ_Flag_No_Occlusion);
		
		if (debug_file) {
			BFrFile_Printf(debug_file, "turret #%d class %s ID %d, typeindex %d objID %d -> tag %d"UUmNL,
								i, turret_osd->turret_class_name, turret_osd->id, object_group->object_typeindex, object->object_id, tag);
		}

		error = OBJiENVFile_WriteNode_Geometry(env_file, &matrix, turret_osd->turret_class->base_geometry, flags, turret_osd->base_ls_data, object_name, user_data, NULL, 0, NULL, NULL, debug_file);
		if (error != UUcError_None) { break; }
	}

	//COrConsole_Printf("turrets %d", OBJgNodeActual);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Triggers
	object_group = OBJiObjectGroup_GetByType(OBJcType_Trigger);
	if (object_group == NULL) { return UUcError_Generic; }
	
	if (debug_file) {
		BFrFile_Printf(debug_file, "TRIGGERS"UUmNL);
		BFrFile_Printf(debug_file, "========"UUmNL);
		BFrFile_Printf(debug_file, UUmNL);
	}

	num_objects = OBJiObjectGroup_GetNumObjects(object_group);
	for (i = 0; i < num_objects; i++)
	{
		OBJtOSD_Trigger		*trigger_osd;

		object = OBJiObjectGroup_GetObject(object_group, i);
		if ((object == NULL) || (object->object_type != OBJcType_Trigger)) { continue; }
		if( object->flags & OBJcObjectFlag_Temporary )			continue;
		
		trigger_osd = (OBJtOSD_Trigger*) object->object_data;

		tag = OBJmMakeObjectTag(object_group->object_typeindex, object);

		sprintf( object_name,	"ob_%d_%d",				object_group->object_typeindex, object->object_id );
		sprintf( user_data,		"$object_tag = %d"UUmNL, tag );

		OBJrObject_GetRotationMatrix(object, &m1);
 		MUrMatrix_Identity(&matrix);
		MUrMatrixStack_Translate(&matrix, &object->position);
		MUrMatrixStack_Matrix(&matrix, &m1);

		// always set the no occlusion flag
		flags = (trigger_osd->trigger_class->base_gq_flags | AKcGQ_Flag_No_Occlusion);
		
		if (debug_file) {
			BFrFile_Printf(debug_file, "trigger #%d class %s ID %d, typeindex %d objID %d -> tag %d"UUmNL,
				i, trigger_osd->trigger_class_name, trigger_osd->id, object_group->object_typeindex, object->object_id, tag);
		}

		error = OBJiENVFile_WriteNode_Geometry(env_file, &matrix, trigger_osd->trigger_class->base_geometry, flags, NULL, object_name, user_data, NULL, 0, NULL, NULL, debug_file);
		if (error != UUcError_None) { break; }
	}

	//COrConsole_Printf("triggers %d", OBJgNodeActual);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Consoles
	object_group = OBJiObjectGroup_GetByType(OBJcType_Console);
	if (object_group == NULL) { return UUcError_Generic; }

	if (debug_file) {
		BFrFile_Printf(debug_file, "CONSOLES"UUmNL);
		BFrFile_Printf(debug_file, "========"UUmNL);
		BFrFile_Printf(debug_file, UUmNL);
	}

	num_objects = OBJiObjectGroup_GetNumObjects(object_group);
	for (i = 0; i < num_objects; i++)
	{
		OBJtOSD_Console		*console_osd;

		object = OBJiObjectGroup_GetObject(object_group, i);
		if ((object == NULL) || (object->object_type != OBJcType_Console)) { continue; }
		if( object->flags & OBJcObjectFlag_Temporary )			continue;
		
		console_osd = (OBJtOSD_Console*) object->object_data;

		OBJrObject_GetRotationMatrix(object, &m1);
 		MUrMatrix_Identity(&matrix);
		MUrMatrixStack_Translate(&matrix, &object->position);
		MUrMatrixStack_Matrix(&matrix, &m1);

		tag = OBJmMakeObjectTag(object_group->object_typeindex, object);

		sprintf( object_name,	"ob_%d_%d",				object_group->object_typeindex, object->object_id );
		sprintf( user_data,		"$object_tag = %d"UUmNL, tag );

		if (debug_file) {
			BFrFile_Printf(debug_file, "console #%d class %s ID %d, typeindex %d objID %d -> tag %d"UUmNL,
				i, console_osd->console_class_name, console_osd->id, object_group->object_typeindex, object->object_id, tag);
		}

		error = OBJiENVFile_WriteNode_GeometryArray(env_file, &matrix, console_osd->console_class->geometry_array, console_osd->ls_data_array, object_name, user_data, NULL, 0, NULL, AKcGQ_Flag_No_Occlusion, 0, 0, 0, 0, 0, NULL, debug_file);
		if (error != UUcError_None) { break; }
	}

	//COrConsole_Printf("consoles %d", OBJgNodeActual);
	

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// The Doors
	object_group = OBJiObjectGroup_GetByType(OBJcType_Door);
	if (object_group == NULL) { return UUcError_Generic; }

	if (debug_file) {
		BFrFile_Printf(debug_file, "DOORS"UUmNL);
		BFrFile_Printf(debug_file, "====="UUmNL);
		BFrFile_Printf(debug_file, UUmNL);
	}

	num_objects = OBJiObjectGroup_GetNumObjects(object_group);
	for (i = 0; i < num_objects; i++)
	{
		OBJtOSD_Door		*door_osd;
		UUtUns32			gq_flags;

		object = OBJiObjectGroup_GetObject(object_group, i);
		if( !object || (object->object_type != OBJcType_Door))	continue;
		if( object->flags & OBJcObjectFlag_Temporary )			continue;
		
		door_osd = (OBJtOSD_Door*) object->object_data;
		
		if (door_osd->door_class == NULL)
			continue;

		OBJrDoor_GetExportMatricies( object, &m1, &m1b, &m2, &m2b );

		tag = OBJmMakeObjectTag(object_group->object_typeindex, object);

		sprintf( user_data,		"$object_tag = %d"UUmNL,		tag );

		// CB: note... we write out doors as no-collision so that they aren't rasterized into
		// the pathfinding grid. this flag will be cleared at runtime when the door is reset, so don't worry,
		// characters won't walk through doors.
		gq_flags = AKcGQ_Flag_No_Collision | AKcGQ_Flag_NoDecal;

		if (debug_file) {
			BFrFile_Printf(debug_file, "door #%d class %s ID %d, typeindex %d objID %d -> tag %d"UUmNL,
				i, door_osd->door_class_name, door_osd->id, object_group->object_typeindex, object->object_id, tag);
		}

		sprintf( object_name,	"door_%d",					door_osd->id );
		error = OBJiENVFile_WriteNode_GeometryArray(env_file, &m1, door_osd->door_class->geometry_array[0], NULL, object_name, user_data, NULL, 0, NULL, gq_flags, 0, 0, 0, 0, 0, door_osd->door_texture_ptr[0], debug_file );
		if (error != UUcError_None) { break; }

		sprintf( object_name,	"object_door_%d",			door_osd->id );
		error = OBJiENVFile_WriteNode_GeometryArray(env_file, &m1b, door_osd->door_class->geometry_array[0], NULL, object_name, user_data, NULL, 0, NULL, gq_flags, 0, 0, 0, 0, 0, door_osd->door_texture_ptr[0], debug_file );
		if (error != UUcError_None) { break; }

		if( door_osd->flags & OBJcDoorFlag_DoubleDoors )
		{
			sprintf( object_name,	"door_%d",					door_osd->id | 0x1000 );
			if (door_osd->door_class->geometry_array[1]) {
				error = OBJiENVFile_WriteNode_GeometryArray(env_file, &m2, door_osd->door_class->geometry_array[1], NULL, object_name, user_data, NULL, 0, NULL, gq_flags, 0, 0, 0, 0, 0, door_osd->door_texture_ptr[1], debug_file );
			}
			else {
				error = OBJiENVFile_WriteNode_GeometryArray(env_file, &m2, door_osd->door_class->geometry_array[0], NULL, object_name, user_data, NULL, 0, NULL, gq_flags, 0, 0, 0, 0, 0, door_osd->door_texture_ptr[1], debug_file );
			}
			if (error != UUcError_None) { break; }

			sprintf( object_name,	"object_door_%d",			door_osd->id | 0x1000 );
			if (door_osd->door_class->geometry_array[1]) {
				error = OBJiENVFile_WriteNode_GeometryArray(env_file, &m2b, door_osd->door_class->geometry_array[1], NULL, object_name, user_data, NULL, 0, NULL, gq_flags, 0, 0, 0, 0, 0, door_osd->door_texture_ptr[1], debug_file );
			}
			else {
				error = OBJiENVFile_WriteNode_GeometryArray(env_file, &m2b, door_osd->door_class->geometry_array[0], NULL, object_name, user_data, NULL, 0, NULL, gq_flags, 0, 0, 0, 0, 0, door_osd->door_texture_ptr[1], debug_file );
			}
			if (error != UUcError_None) { break; }
			
		}
	}

	//COrConsole_Printf("doors %d", OBJgNodeActual);
	
	// set the end of the file
	BFrFile_SetEOF(env_file);
	
	// close the file
	BFrFile_Close(env_file);
	env_file = NULL;
	
	// delete the file ref
	BFrFileRef_Dispose(env_file_ref);
	env_file_ref = NULL;

	if (debug_file) {
		// delete the debug file
		BFrFile_Printf(debug_file, "ENV file written, %d nodes (predicted: %d)"UUmNL, OBJgNodeCount, OBJgNodeActual);
		BFrFile_Close(debug_file);
		debug_file = NULL;
	}

	if (debug_file_ref) {
		BFrFileRef_Dispose(debug_file_ref);
		debug_file_ref = NULL;
	}

	if (OBJgNodeCount != OBJgNodeActual) {
		COrConsole_Printf("FAILED TO WRITE OUT ENV FILE, predicted %d wrote %d", OBJgNodeCount, OBJgNodeActual);
	}
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
#if TOOL_VERSION
UUtError
OBJrFlagTXTFile_Write(
	void)
{
	UUtError				error;
	UUtUns16				level_num;
	BFtFileRef				*flag_file_ref;
	BFtFile					*flag_file;
	char					filename[128];
	UUtUns32				i;
	UUtUns32				num_objects;
	OBJtObjectGroup			*object_group;
	
	// get the object group
	object_group = OBJiObjectGroup_GetByType(OBJcType_Flag);
	if (object_group == NULL) { return UUcError_Generic; }
	
	// get the current level number
	level_num = ONrLevel_GetCurrentLevel();
	if (level_num == 0) { return UUcError_Generic; }
	
	// create the env file ref
	sprintf(filename, "L%d_Flags.txt", level_num);
	error = BFrFileRef_MakeFromName(filename, &flag_file_ref);
	UUmError_ReturnOnError(error);
	
	// create the .TXT file if it doesn't already exist
	if (BFrFileRef_FileExists(flag_file_ref) == UUcFalse)
	{
		// create the object.env file
		error = BFrFile_Create(flag_file_ref);
		UUmError_ReturnOnError(error);
	}
	
	// open the object.env file
	error = BFrFile_Open(flag_file_ref, "rw", &flag_file);
	UUmError_ReturnOnError(error);
	
	// set the position to 0
	error = BFrFile_SetPos(flag_file, 0);
	UUmError_ReturnOnError(error);
	
	// write the flags
	num_objects = OBJiObjectGroup_GetNumObjects(object_group);
	for (i = 0; i < num_objects; i++)
	{
		OBJtObject			*object;
		OBJtOSD_All			osd_all;
		char				string[1024];
		
		object = OBJiObjectGroup_GetObject(object_group, i);
		if (object == NULL) { continue; }
		
		OBJrObject_GetObjectSpecificData(object, &osd_all);
		
		// format each line as:
		// flag_name <tab> flag_note
		OBJrObject_GetName(object, string, 1024);
		strcat(string, "\t");
		strcat(string, osd_all.osd.flag_osd.note);
		strcat(string, UUmNL);
		
		BFrFile_Write(flag_file, strlen(string), string);
	}
	
	// set the end of the file
	BFrFile_SetEOF(flag_file);
	
	// close the file
	BFrFile_Close(flag_file);
	flag_file = NULL;
	
	// delete the file ref
	BFrFileRef_Dispose(flag_file_ref);
	flag_file_ref = NULL;
	
	return UUcError_None;
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OBJrSelectedObjects_Delete(
	void)
{
	UUtUns32				num_objects;
	UUtUns32				i;
	
	// delete all of the selected objects pointers
	num_objects = UUrMemory_Array_GetUsedElems(OBJgSelectedObjects);
	for (i = 0; i < num_objects; i++)
	{
		OBJtObject			*object;
		
		// get a currently selected object
		object = OBJrSelectedObjects_GetSelectedObject(0);
		if (object == NULL) { continue; }
		
		// delete the object
		OBJrObject_Delete(object);
	}
	
	// there are no longer any objects selected so the selection can't be locked
	OBJrSelectedObjects_Lock_Set(UUcFalse);
}

// ----------------------------------------------------------------------
UUtUns32
OBJrSelectedObjects_GetNumSelected(
	void)
{
	return UUrMemory_Array_GetUsedElems(OBJgSelectedObjects);
}	

// ----------------------------------------------------------------------
OBJtObject*
OBJrSelectedObjects_GetSelectedObject(
	UUtUns32				inIndex)
{
	OBJtObject				**object_list;
	UUtUns32				num_objects;
	
	num_objects = OBJrSelectedObjects_GetNumSelected();
	if ((inIndex >= num_objects) || (num_objects == 0))
	{
		return NULL;
	}
	
	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(OBJgSelectedObjects);
	
	return object_list[inIndex];
}

// ----------------------------------------------------------------------
static UUtError
OBJiSelectedObjects_Initialize(
	void)
{
	// allocate memory for the selected objects list
	OBJgSelectedObjects =
		UUrMemory_Array_New(
			sizeof(OBJtObject*),
			1,
			0,
			10);
	UUmError_ReturnOnNull(OBJgSelectedObjects);
	
	OBJrSelectedObjects_Lock_Set(UUcFalse);
	
	return UUcError_None;
}

void
OBJrSelectedObjects_MoveCameraToSelection(
	void)
{
	LItAction					action;
	OBJtObject					*object;
	CAtCamera					*camera		= ONgGameState->local.camera;
	M3tPoint3D					position_sum;
	UUtUns32					num_objects, num_found, itr;

	// get the center of the selection
	num_found = 0;
	MUmVector_Set(position_sum, 0, 0, 0);
	num_objects = OBJrSelectedObjects_GetNumSelected();

	for (itr = 0; itr < num_objects; itr++) {
		object = OBJrSelectedObjects_GetSelectedObject(itr);
		if (object == NULL)
			continue;

		MUmVector_Add(position_sum, position_sum, object->position);
		num_found++;
	}
	
	if (num_found == 0) {
		return;
	}

	// place the camera into the correct mode
	if( CAgCamera.mode != CAcMode_Manual )
	{
		CArManual_Enter();
		CAgCamera.modeData.manual.useMouseLook = UUcTrue;
	}

	CArManual_LookAt(&position_sum, 80.0f);

	UUrMemory_Clear(&action, sizeof(LItAction));
	CArUpdate(0, 1, &action);
}

// ----------------------------------------------------------------------
UUtBool
OBJrSelectedObjects_IsObjectSelected(
	const OBJtObject		*inObject)
{
	OBJtObject				**object_list;
	UUtUns32				num_objects;
	UUtUns32				i;
	UUtBool					found;
	
	// find inObject in the selected objects list
	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(OBJgSelectedObjects);
	num_objects = OBJrSelectedObjects_GetNumSelected();
	
	found = UUcFalse;
	for (i = 0; i < num_objects; i++)
	{
		if (object_list[i] == inObject)
		{
			found = UUcTrue;
			break;
		}
	}
	
	return found;
}

// ----------------------------------------------------------------------
UUtBool
OBJrSelectedObjects_Lock_Get(
	void)
{
	return OBJgSelectedObjects_Locked;
}

// ----------------------------------------------------------------------
void
OBJrSelectedObjects_Lock_Set(
	UUtBool					inLock)
{
	OBJgSelectedObjects_Locked = inLock;
}

// ----------------------------------------------------------------------
void
OBJrSelectedObjects_Select(
	OBJtObject				*inObject,
	UUtBool					inAddToList)
{
	UUtError				error;
	UUtUns32				index;
	UUtBool					mem_moved;
	OBJtObject				**object_list;
	
	if (OBJgSelectedObjects_Locked == UUcTrue) { return; }
	
	// clear the list if the item isn't being added to the current list
	if (inAddToList == UUcFalse)
	{
		OBJrSelectedObjects_UnselectAll();
 	}
 	
 	// if inObject is null then there is nothing to add so leave
 	if (inObject == NULL) { return; }
 	
 	// make room in the list for a new element
 	error = UUrMemory_Array_GetNewElement(OBJgSelectedObjects, &index, &mem_moved);
 	if (error != UUcError_None) { return; }
 	
 	// add the element to the list
 	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(OBJgSelectedObjects);
 	object_list[index] = inObject;
}

// ----------------------------------------------------------------------
static void
OBJiSelectedObjects_Terminate(
	void)
{
	UUrMemory_Array_Delete(OBJgSelectedObjects);
	OBJgSelectedObjects = NULL;
}

// ----------------------------------------------------------------------
void
OBJrSelectedObjects_Unselect(
	OBJtObject				*inObject)
{
	UUtUns32				i;
	OBJtObject				**object_list;
	
	if (OBJgSelectedObjects_Locked == UUcTrue) { return; }

	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(OBJgSelectedObjects);
	
	for (i = 0; i < OBJrSelectedObjects_GetNumSelected(); i++)
	{
		if (object_list[i] == inObject)
		{
			UUrMemory_Array_DeleteElement(OBJgSelectedObjects, i);
			break;
		}
	}
}

// ----------------------------------------------------------------------
void
OBJrSelectedObjects_UnselectAll(
	void)
{
	if (OBJgSelectedObjects_Locked == UUcTrue) { return; }

	while (OBJrSelectedObjects_GetNumSelected() > 0)
	{
		UUrMemory_Array_DeleteElement(OBJgSelectedObjects, 0);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtUns32
OBJrMoveMode_Get(
	void)
{
	return OBJgMoveMode;
}

// ----------------------------------------------------------------------
void
OBJrMoveMode_Set(
	UUtUns32				inMoveMode)
{
	OBJgMoveMode = inMoveMode;
}

// ----------------------------------------------------------------------
UUtUns32
OBJrRotateMode_Get(
	void)
{
	return OBJgRotateMode;
}

// ----------------------------------------------------------------------
void
OBJrRotateMode_Set(
	UUtUns32				inRotateMode)
{
	if ((inRotateMode & OBJcRotateMode_XYZ) == OBJcRotateMode_XYZ)
	{
		OBJgRotateMode = OBJcRotateMode_XYZ;
	}
	else if ((inRotateMode & OBJcRotateMode_XY) == OBJcRotateMode_XY)
	{
		OBJgRotateMode = OBJcRotateMode_XY;
	}
	else if ((inRotateMode & OBJcRotateMode_XZ) == OBJcRotateMode_XZ)
	{
		OBJgRotateMode = OBJcRotateMode_XZ;
	}
	else if ((inRotateMode & OBJcRotateMode_YZ) == OBJcRotateMode_YZ)
	{
		OBJgRotateMode = OBJcRotateMode_YZ;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OBJrDrawObjects(
	void)
{
	UUtStallTimer draw_objects_timer;

	UUrStallTimer_Begin(&draw_objects_timer);
	OBJiObjectGroups_Draw();
	UUrStallTimer_End(&draw_objects_timer, "OBJrDrawObjects - OBJiObjectGroups_Draw");

	// autosave
#if TOOL_VERSION	
	if (OBJgAutoSaveTime < UUrMachineTime_Sixtieths())
	{
		UUrStallTimer_Begin(&draw_objects_timer);

		UUrMemory_Block_VerifyList();

		OBJiBinaryData_Save(OBJcType_None, UUcTrue);
		OBJgAutoSaveTime = UUrMachineTime_Sixtieths() + OBJcAutoSaveDelta;

		UUrStallTimer_End(&draw_objects_timer, "OBJrDrawObjects - auto save");
	}
#endif

	return;
}

// ----------------------------------------------------------------------
OBJtObject*
OBJrGetObjectUnderPoint(
	IMtPoint2D				*inPoint)
{
	M3tGeomCamera			*camera;
	M3tPoint3D				world_point;
	M3tPoint3D				camera_position;
	M3tVector3D				intersect_vector;
	OBJtObject				*closest_to_camera;
	
	UUtUns32				i;
	UUtUns32				num_objects;
	
	float					min_distance;
	M3tPoint3D				start_point;
	M3tPoint3D				end_point;
	
	UUtUns32				j;
	UUtUns32				num_groups;
	
	// get the active camera
	M3rCamera_GetActive(&camera);
	UUmAssert(camera);
	
	// get the point on the near clip plane where the user clicked
	M3rPick_ScreenToWorld(camera, (UUtUns16)inPoint->x, (UUtUns16)inPoint->y, &world_point);
	
	// get the camera position
	camera_position = CArGetLocation();
	
	// build a vector from the camera to the point on the near clip plane
	intersect_vector.x = world_point.x - camera_position.x;
	intersect_vector.y = world_point.y - camera_position.y;
	intersect_vector.z = world_point.z - camera_position.z;
	
	// normalize
	MUrNormalize(&intersect_vector);
	
	// multiply by some large number (5,000) to get a long line
	MUrVector_SetLength(&intersect_vector, 5000.0f);
	
	// set up vars for collision detection
	closest_to_camera = NULL;
	min_distance = 1000000.0f;
	start_point = end_point = camera_position;
	end_point.x += intersect_vector.x;
	end_point.y += intersect_vector.y;
	end_point.z += intersect_vector.z;
	
	// go through all of the object groups
	num_groups = OBJiObjectGroups_GetNumGroups();
	for (j = 0; j < num_groups; j++)
	{
		OBJtObjectGroup			*object_group;
		
		// get the object group
		object_group = OBJiObjectGroups_GetGroupByIndex(j);
		
		// don't select objects which aren't visible
		if (OBJrObjectType_GetVisible(OBJiObjectGroup_GetType(object_group)) == UUcFalse)
		{
			continue;
		}
		
		// loop through each object and find the one closest to the camera that
		// collides with the line
		num_objects = OBJiObjectGroup_GetNumObjects(object_group);
		for (i = 0; i < num_objects; i++)
		{
			OBJtObject				*object;
			
			object = OBJiObjectGroup_GetObject(object_group, i);
			
			// if the object collides with the line, see if it is the closest to the camera
			if (OBJrObject_IntersectsLine(object, &start_point, &end_point))
			{
				M3tVector3D			camera_to_object;
				float				distance_to_object;
				
				// make the vector from the camera to the object
				camera_to_object.x = object->position.x - camera_position.x;
				camera_to_object.y = object->position.y - camera_position.y;
				camera_to_object.z = object->position.z - camera_position.z;
				
				// calculate the length from the camera to the object
				distance_to_object = MUrVector_Length(&camera_to_object);
				
				if (distance_to_object < min_distance)
				{
					// save closest object to camera
					min_distance = distance_to_object;
					closest_to_camera = object;
				}
			}
		}
	}
	
	return closest_to_camera;
}

// ----------------------------------------------------------------------
UUtError
OBJrInitialize(
	void)
{
	UUtError					error;

	// Initialize game Mechanics
	error = ONrMechanics_Initialize();
	UUmError_ReturnOnError(error);
	
	OBJiSelectedObjects_Initialize();
	
	// initialize the binary data class
	error = OBJiBinaryData_Register();
	UUmError_ReturnOnError(error);
	
	// initialize the object groups
	error = OBJiObjectGroups_Initialize();
	UUmError_ReturnOnError(error);
	
	// initialize the object types
	error = OBJrCharacter_Initialize();
	UUmError_ReturnOnError(error);

	error = OBJrCombat_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrConsole_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrDoor_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrFlag_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrFurniture_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrMelee_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrNeutral_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrParticle_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrPatrolPath_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrPowerUp_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrSound_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrTrigger_Initialize();
	UUmError_ReturnOnError(error);

	error = OBJrTriggerVolume_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrTurret_Initialize();
	UUmError_ReturnOnError(error);
	
	error = OBJrWeapon_Initialize();
	UUmError_ReturnOnError(error);
	
	// init vars
	OBJrMoveMode_Set(OBJcMoveMode_All);
	OBJrRotateMode_Set(OBJcRotateMode_XYZ);
	OBJgAutoSaveTime = UUrMachineTime_Sixtieths() + OBJcAutoSaveDelta;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrSaveObjects(
	UUtUns32				inObjectType)
{
#if TOOL_VERSION
	// save the objects
	OBJiBinaryData_Save(inObjectType, UUcFalse);
#endif
}

// ----------------------------------------------------------------------
void
OBJrTerminate(
	void)
{
#if TOOL_VERSION
	OBJrSound_DrawTerminate();
	OBJrFlag_DrawTerminate();
	OBJrCharacter_DrawTerminate();
	OBJrPatrolPath_DrawTerminate();
	OBJrDoor_DrawTerminate();
#endif

	OBJiSelectedObjects_Terminate();
	
	// terminate the object groups
	OBJiObjectGroups_Terminate();

	// destroy game mechanics
	ONrMechanics_Terminate();

	UUrMemory_Block_VerifyList();
}



static UUtBool OWiCompare_Object_By_Name(UUtUns32 inA, UUtUns32 inB)
{
	OBJtObject *object_a = (OBJtObject *) inA;
	OBJtObject *object_b = (OBJtObject *) inB;

	char name_a[128];
	char name_b[128];

	UUtBool a_less_than_b;

	OBJrObject_GetName(object_a, name_a, 128);
	OBJrObject_GetName(object_b, name_b, 128);

	a_less_than_b = UUrString_Compare_NoCase(name_a, name_b) > 0;

	return a_less_than_b;
}

void OBJrObjectType_BuildListBox(OBJtObjectType inObjectType, WMtWindow *ioListBox, UUtBool inAllowNone)
{
	// construct the initial state of the listbox 
	OBJtObject **list = NULL;
	UUtUns32 count;
	UUtUns32 itr, base;
	OBJtObject *old_selected_object;

	old_selected_object = (OBJtObject *) WMrMessage_Send(ioListBox, LBcMessage_GetItemData, 0, -1);

	WMrMessage_Send(ioListBox, LBcMessage_Reset, 0, 0);

	if (inAllowNone) {
		base = 1;
		WMrMessage_Send(ioListBox, LBcMessage_AddString, (UUtUns32) "<none>", 0);
		WMrMessage_Send(ioListBox, LBcMessage_SetItemData, (UUtUns32) NULL, 0);
	} else {
		base = 0;
	}

	count = OBJrObjectType_EnumerateObjects(inObjectType, NULL, 0);
	list = UUrMemory_Block_New(sizeof(OBJtObject *) * count);

	for(itr = 0; itr < count; itr++)
	{
		list[itr] = OBJrObjectType_GetObject_ByNumber(inObjectType, itr);
	}

	AUrQSort_32(list, count, OWiCompare_Object_By_Name);

	for(itr = 0; itr < count; itr++)
	{
		UUtUns32 new_index;
		char name[128];

		OBJrObject_GetName(list[itr], name, 128);

		new_index = WMrMessage_Send(ioListBox, LBcMessage_AddString, (UUtUns32) name, 0);
		UUmAssert(new_index == itr + base);

		WMrMessage_Send(ioListBox, LBcMessage_SetItemData, (UUtUns32) list[itr], itr + base);

		if (list[itr] == old_selected_object) {
			WMrMessage_Send(ioListBox, LBcMessage_SetSelection, UUcFalse, itr + base);
		}
	}

	if (NULL != list) {
		UUrMemory_Block_Delete(list);
	}
}

// ----------------------------------------------------------------------

UUtUns32 OBJrObject_GetNextAvailableID(void)
{
	UUtUns32				next_id;
	next_id = OBJgNextObjectID;
	OBJgNextObjectID++;
	return next_id;
}

// ----------------------------------------------------------------------

typedef struct OBJtObjectSearchRequest
{
	UUtUns32				id;
	OBJtObject				*object;
} OBJtObjectSearchRequest;

static UUtBool OBJiObject_Enum_ObjectType( OBJtObjectType inObjectType, const char *inName, UUtUns32 inUserData )
{
	return UUcTrue;
}
	
static UUtBool OBJiObject_Enum_SearchRequest( OBJtObject *inObject, UUtUns32 inUserData )
{
	OBJtObjectSearchRequest			*request;

	request = (OBJtObjectSearchRequest*) inUserData;
	if( inObject->object_id == request->id )
	{
		request->object = (OBJtObject*) inObject;
		return UUcFalse;
	}
	return UUcTrue;
}


OBJtObject* OBJrObject_FindByID( UUtUns32 inID )
{
	OBJtObjectSearchRequest			request;
	OBJtObjectGroup					**object_group_list;
	UUtUns32						num_groups;
	UUtUns32						i;
	
	// intialize
	request.object = NULL;
	request.id = inID;
	
	// make sure the object group list exists
	object_group_list = (OBJtObjectGroup**)UUrMemory_Array_GetMemory(OBJgObjectGroups);
	if (object_group_list)
	{
		num_groups = UUrMemory_Array_GetUsedElems(OBJgObjectGroups);
		for (i = 0; i < num_groups; i++)
		{
			OBJiObjectGroup_EnumerateObjects( object_group_list[i], OBJiObject_Enum_SearchRequest, (UUtUns32) &request );
			if( request.object )
				return request.object;
		}
	}
	
	return NULL;
}

UUtError StringToBitMask( UUtUns32 *ioMask, char* inString )
{
	char*		str;
	char*		internal;
	UUtUns32	mask;
	UUtUns32	value;
	UUtUns32	bit;
	UUtError	error;
	
	internal	= NULL;
	mask		= 0;

	str = UUrString_Tokenize( inString, ",", &internal );
	while( str )
	{
		error = UUrString_To_Uns32( str, &value );
		if( error != UUcError_None )
			return error;
		
		bit = 1 << value;
		mask = mask | bit;

		str = UUrString_Tokenize( NULL, ",", &internal );
	}

	*ioMask = mask;

	return UUcError_None;
}

UUtError BitMaskToString( UUtUns32 inMask, char* ioString, UUtUns32 inStringSize )
{
	UUtUns32		mask;
	UUtUns16		index;
	char			temp[20];
	char			string[ 100 ];

	UUrMemory_Clear(string, 100 );

	mask = 1;
	for( index = 1; index <= 32; index++ )
	{
		if( ( inMask & mask ) == mask )
		{
			sprintf( temp, "%d", index - 1 );
			if( string[0] )
				strcat( string, "," );
			strcat( string, temp );
		}
		mask = mask << 1;
	}

	UUrString_Copy( ioString, string, inStringSize );

	return UUcError_None;
}

static UUtBool OWrOSD_ChooseName(OBJtObjectType inObjectType, OBJtOSD_All *inOSD, char *inPrompt)
{
	OBJtObjectGroup *object_group = OBJiObjectGroup_GetByType(inObjectType);
	char new_src_string[128];
	char new_dst_string[128];
	UUtBool success = UUcTrue;

	if (object_group->flags & OBJcObjectGroupFlag_CanSetName) {
		char *gotten_string;
		OBJrObject_OSD_GetName(inObjectType, inOSD, new_src_string, sizeof(new_src_string));

		gotten_string = WMrDialog_GetString(
			NULL, 
			inPrompt, 
			inPrompt, 
			new_src_string, 
			new_dst_string, 
			SLcScript_MaxNameLength, 
			NULL);

		if (gotten_string != NULL) {
			OBJrObject_OSD_SetName(inObjectType, inOSD, new_dst_string);
		}
		else {
			success = UUcFalse;
		}
	}

	return success;
}

typedef struct OBJtSelectObjectInternals
{
	OBJtObjectType object_type;
	OBJtObject *selected_object;
	UUtBool allow_none;
	UUtBool allow_goto;
} OBJtSelectObjectInternals;

static UUtBool
OWiChooseObject_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	enum
	{
		cChooseObject_Listbox = 100,
		cChooseObject_New = 101,
		cChooseObject_Delete = 102,
		cChooseObject_Edit = 103,
		cChooseObject_Duplicate = 104,
		cChooseObject_GoTo = 105
	};

	OBJtSelectObjectInternals *internals = (OBJtSelectObjectInternals *) WMrDialog_GetUserData(inDialog);
	UUtBool handled = UUcTrue;
	UUtError error;
	WMtWindow *listbox = WMrDialog_GetItemByID(inDialog, cChooseObject_Listbox);
	UUtBool finish_dialog = UUcFalse;
	UUtBool cancel_dialog = UUcFalse;
	OBJtObjectType object_type = internals->object_type;
	OBJtObjectGroup *object_group = OBJiObjectGroup_GetByType(object_type);

	UUtUns32 selected_index = LBcError;
	OBJtObject *selected_object = NULL;
	char titlebuf[WMcMaxTitleLength];

	UUtBool edit_selected_object = UUcFalse;
	UUtBool rebuild_list_box = UUcFalse;

	UUmAssert(NULL != object_group);

	if (listbox != NULL) {
		selected_index = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
	}

	if (LBcError != selected_index) {
		selected_object	= (OBJtObject *) WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
	}

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			sprintf(titlebuf, "Select %s", object_group->group_name);
			WMrWindow_SetTitle(inDialog, titlebuf, WMcMaxTitleLength);

			OBJrObjectType_BuildListBox(object_type, listbox, internals->allow_none);
			selected_object = internals->selected_object;

			rebuild_list_box = UUcTrue;
		break;
		
		case WMcMessage_Destroy:
		break;

		case WMcMessage_MenuCommand:
			switch (WMrWindow_GetID((WMtMenu *) inParam2))
			{
			}
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_OK:
					finish_dialog = UUcTrue;
				break;

				case WMcDialogItem_Cancel:
					cancel_dialog = UUcTrue;
				break;

				case cChooseObject_New:
					{
						UUtBool got_new_name;
						OBJtOSD_All new_object_data;

						// CB: this ensures that there are no uninitialized variables
						// in the OSD!
						error = OBJrObject_OSD_SetDefaults(object_type, &new_object_data);
//						OBJrObjectType_GetUniqueOSD(object_type, &new_object_data);
						if (error == UUcError_None) {
							got_new_name = OWrOSD_ChooseName(object_type, &new_object_data, "New Name");					

							if (got_new_name) {
								error = OWrTools_CreateObject(object_type, &new_object_data);
								if (error == UUcError_None) {
									selected_object = OBJrSelectedObjects_GetSelectedObject(0);
									rebuild_list_box = UUcTrue;
								}
							}					
						}
					}
				break;

				case cChooseObject_Duplicate:
					if (NULL != selected_object)
					{
						OBJtOSD_All duplicate_object;
						UUtBool got_duplicate_name;

						M3tPoint3D duplicate_position;
						M3tPoint3D duplicate_rotation;

						OBJrObject_GetObjectSpecificData(selected_object, &duplicate_object);
						OBJrObject_GetPosition(selected_object, &duplicate_position, &duplicate_rotation);

						got_duplicate_name = OWrOSD_ChooseName(object_type, &duplicate_object, "Duplicate's Name");

						if (got_duplicate_name) {
							OBJrObject_New(object_type, &duplicate_position, &duplicate_rotation, &duplicate_object);							
							selected_object = OBJrSelectedObjects_GetSelectedObject(0);
							rebuild_list_box = UUcTrue;
						}					
					}					
				break;

				case cChooseObject_Delete:
					if (NULL != selected_object) {
						OBJrObject_Delete(selected_object);
						rebuild_list_box = UUcTrue;
					}				 
				break;

				case cChooseObject_GoTo:
					if (NULL != selected_object) {
						OBJrSelectedObjects_Select(selected_object, UUcFalse);
						OBJrSelectedObjects_MoveCameraToSelection();
					}
				break;

				case cChooseObject_Edit:
					edit_selected_object = UUcTrue;
				break;

				case cChooseObject_Listbox:
					if (WMcNotify_DoubleClick == UUmHighWord(inParam1)) {
						edit_selected_object = UUcTrue;
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}

	if ((edit_selected_object) && (NULL != selected_object)) {
		OWrProp_Display(selected_object);
		rebuild_list_box = UUcTrue;
	}

	if (rebuild_list_box) {
		OBJrObjectType_BuildListBox(object_type, listbox, internals->allow_none);

		{
			UUtUns32 scan_itr;
			UUtUns32 scan_count = WMrMessage_Send(listbox, LBcMessage_GetNumLines, 0, 0);
			OBJtObject *this_object;

			for(scan_itr = 0; scan_itr < scan_count; scan_itr++)
			{
				this_object = (OBJtObject *) WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, scan_itr);

				if (selected_object == this_object) {
					WMrMessage_Send(listbox, LBcMessage_SetSelection, UUcFalse, scan_itr);
					break;
				}
			}
		}
	}

	if (finish_dialog || cancel_dialog) {
		UUtUns32 result_object = (UUtUns32) internals->selected_object;

		if (!cancel_dialog) {
			if ((internals->allow_none) || (selected_object != NULL)) {
				result_object = (UUtUns32) selected_object;
			}
		}

		WMrDialog_ModalEnd(inDialog, result_object);
	}
	
	return handled;
}

OBJtObject *OWrSelectObject(OBJtObjectType inObjectType, OBJtObject *inObject, UUtBool inAllowNone, UUtBool inAllowGoto)
{
	OBJtSelectObjectInternals select_object_internals;
	OBJtObject *out_selected_object;

	select_object_internals.object_type = inObjectType;
	select_object_internals.selected_object = inObject;
	select_object_internals.allow_none = inAllowNone;
	select_object_internals.allow_goto = inAllowGoto;

	WMrDialog_ModalBegin(OWcDialog_AI_ChooseCombat, NULL, OWiChooseObject_Callback, (UUtUns32) &select_object_internals, (UUtUns32 *) &out_selected_object);

	return out_selected_object;
}

// ----------------------------------------------------------------------------------------------------

UUtUns32 OBJrObjectType_GetNumObjects( OBJtObjectType inObjectType )
{
	OBJtObjectGroup		*group;
	group = OBJiObjectGroup_GetByType( inObjectType );			UUmAssert( group );
	if( !group )
		return 0;
	return OBJiObjectGroup_GetNumObjects( group );
}

UUtError OBJrObjectType_GetObjectList( OBJtObjectType inObjectType, OBJtObject ***ioList, UUtUns32 *ioObjectCount )
{
	OBJtObjectGroup		*group;
	OBJtObject			**object_list;

	group = OBJiObjectGroup_GetByType( inObjectType );			UUmAssert( group );
	UUmError_ReturnOnNull( group );

	if( !group->object_list ) 
	{
		*ioObjectCount = 0;
		*ioList = NULL;
	}
	else
	{
		object_list = (OBJtObject**) UUrMemory_Array_GetMemory( group->object_list );
		if( !object_list )
		{
			*ioObjectCount = 0;
			*ioList = NULL;
		}
		else
		{
			*ioObjectCount = UUrMemory_Array_GetUsedElems( group->object_list );
			*ioList = object_list;
		}
	}

	return UUcError_None;
}
