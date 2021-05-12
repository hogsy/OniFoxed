// ======================================================================
// Oni_Obj_Tools.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"

#include "Oni_AI_Setup.h"
#include "Oni_Level.h"
#include "Oni_Obj_Tools.h"

#include "ENVFileFormat.h"

// ======================================================================
// defines
// ======================================================================
/*	OBJcObjectFileVersion needs to be incremented when:
	- new fields are added to an object's object specific data,
	- a type flag is changed ex. UUm4CharToUns32('M', 'P', 'L', 'R') to
		UUm4CharToUns32('M', 'P', 'S', 'P') */
#define OBJcObjectFileVersion	0x0000001

#define OBJcNoCharacter			0xFFFF

#define OBJcMaxNameLength		63

#define OBJcMaxBytesInBuffer	2048

#define OBJcSwap				UUm4CharToUns32('S', 'W', 'A', 'P')
#define OBJcEnd					UUm4CharToUns32('E', 'N', 'D', ' ')

// ----------------------------------------------------------------------
#define OBJmGet4BytesFromBuffer(src,dst,type,swap_it)				\
			(dst) = *((type*)(src));								\
			((UUtUns8*)(src)) += sizeof(type);						\
			if ((swap_it)) { UUrSwap_4Byte(&(dst)); }							

#define OBJmWrite4BytesToBuffer(buf,val,type,num_bytes)				\
			*((type*)(buf)) = (val);								\
			((UUtUns8*)(buf)) += sizeof(type);						\
			(num_bytes) -= sizeof(type);

// ======================================================================
// typedefs
// ======================================================================
struct OBJtObject
{
	OBJtObjectType			object_type;
	M3tPoint3D				position;
	M3tPoint3D				rotation;
	UUtUns32				object_data[];
	
};

typedef struct OBJtObjectTypeNames
{
	UUtUns32				type;
	char					*name;//[OBJcMaxNameLength + 1];
	
} OBJtObjectTypeNames;

typedef UUtError
(*OBJtDataProcessor)(
	BFtFile					*inObjectFile,
	UUtBool					inSwapIt);
	
typedef struct OBJtProcessors
{
	UUtUns32				version;
	OBJtDataProcessor		processor;
	
} OBJtProcessors;


// ======================================================================
// globals
// ======================================================================
static UUtMemory_Array		*OBJgObjectArray;	/* access only from OBJiObjectList_* functions */
static BFtFile				*OBJgObjectFile;	/* access only from OBJiLevel_* functions */

static UUtBool				OBJgDrawTriggers;

static OBJtObject			*OBJgSelectedObject;

static OBJtObjectTypeNames	OBJgFlagTypes[] =
{
	{ OBJcFlagType_CharacterStart,		"Character Start" },
	{ OBJcFlagType_MultiplayerSpawn,	"Multiplayer Spawn" },
	{ OBJcFlagType_Waypoint,			"Waypoint" },
	{ OBJcFlagType_None,				NULL }
};

static OBJtObjectTypeNames	OBJgPowerUpTypes[] =
{
	{ OBJcPowerUpType_Hypo,				"Hypo" },
	{ OBJcPowerUpType_ShieldGreen,		"Shield Green" },
	{ OBJcPowerUpType_ShieldBlue,		"Shield Blue" },
	{ OBJcPowerUpType_ShieldRed,		"Shield Red" },
	{ OBJcPowerUpType_AmmoEnergy,		"Ammo Energy" },
	{ OBJcPowerUpType_AmmoBallistic,	"Ammo Ballistic" },
	{ OBJcPowerUpType_None,				NULL }
};

// ======================================================================
// prototypes
// ======================================================================
static UUtError
OBJiObject_FillBuffer(
	OBJtObject				*inObject,
	UUtUns8					*outBuffer,
	UUtUns32				*ioNumBytes);
	
static UUtError
OBJiOSD_ReadFromFile_1(
	BFtFile					*inObjectFile,
	OBJtObjectType			inObjectType,
	UUtBool					inSwapIt,
	OBJtOSD_All				*outOSD);
	
// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiFlagToString(
	UUtUns32				inFlag,
	OBJtObjectTypeNames		*inFlagTypes,
	char					*outString)
{
	UUtUns32				i;
	
	UUmAssert(inFlagTypes);
	UUmAssert(outString);
	
	outString[0] = '\0';
	
	for (i = 0; ; i++)
	{
		if (inFlagTypes[i].name == NULL) { break; }
		
		if (inFlag == inFlagTypes[i].type)
		{
			UUrString_Copy(outString, inFlagTypes[i].name, OBJcMaxNameLength);
			break;
		}
	}
}

// ----------------------------------------------------------------------
static void
OBJiStringToFlag(
	char					*inString,
	OBJtObjectTypeNames		*inFlagTypes,
	UUtUns32				*outFlag)
{
	UUtUns32				i;
	
	UUmAssert(inString);
	UUmAssert(inFlagTypes);
	UUmAssert(outFlag);
	
	*outFlag = 0;
	
	for (i = 0; ; i++)
	{
		if (inFlagTypes[i].name == NULL) { break; }
		
		if (strcmp(inFlagTypes[i].name, inString) == 0)
		{
			*outFlag = inFlagTypes[i].type;
			break;
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
OBJiProcessor_1(
	BFtFile					*inObjectFile,
	UUtBool					inSwapIt)
{
	UUtError				error;
	UUtBool					done;
	UUtUns8					data[OBJcMaxBytesInBuffer];	/* XXX - may need to align */
	UUtUns8					*buffer;
	
	done = UUcFalse;
	do
	{
		OBJtObjectType		object_type;
		M3tPoint3D			position;
		M3tPoint3D			rotation;
		OBJtOSD_All			osd_all;
		
		// get the object type
		error = BFrFile_Read(inObjectFile, sizeof(UUtUns32), data);
		UUmError_ReturnOnError(error);
		buffer = data;
		
		OBJmGet4BytesFromBuffer(buffer, object_type, OBJtObjectType, inSwapIt);
		if (object_type == OBJcEnd)
		{
			done = UUcTrue;
			continue;
		}
		
		// read the object position into memory
		error = BFrFile_Read(inObjectFile, sizeof(M3tPoint3D), data);
		UUmError_ReturnOnError(error);
		buffer = data;
		
		// get the position
		OBJmGet4BytesFromBuffer(buffer, position.x, float, inSwapIt);
		OBJmGet4BytesFromBuffer(buffer, position.y, float, inSwapIt);
		OBJmGet4BytesFromBuffer(buffer, position.z, float, inSwapIt);
		
		// read the object rotation into memory
		error = BFrFile_Read(inObjectFile, sizeof(M3tPoint3D), data);
		UUmError_ReturnOnError(error);
		buffer = data;
		
		// get the rotation
		OBJmGet4BytesFromBuffer(buffer, rotation.x, float, inSwapIt);
		OBJmGet4BytesFromBuffer(buffer, rotation.y, float, inSwapIt);
		OBJmGet4BytesFromBuffer(buffer, rotation.z, float, inSwapIt);
		
		// read the OSD
		UUrMemory_Clear(&osd_all, sizeof(OBJtOSD_All));
		error = OBJiOSD_ReadFromFile_1(inObjectFile, object_type, inSwapIt, &osd_all);
		UUmError_ReturnOnError(error);
		
		// create a new object
		error = OBJrObject_New(object_type, &position, &rotation, (void*)&osd_all);
		UUmError_ReturnOnError(error);
	}
	while (!done);
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OBJiObjectType_IsValid(
	OBJtObjectType			inObjectType)
{
	UUtBool					result;
	
	switch (inObjectType)
	{
		case OBJcType_Character:
		case OBJcType_Flag:
		case OBJcType_Furniture:
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
		case OBJcType_PowerUp:
		case OBJcType_Sound:
		case OBJcType_TriggerVolume:
		case OBJcType_Weapon:
			result = UUcTrue;
		break;
		
		default:
			UUmAssert(!"invalid object type");
			result = UUcFalse;
		break;
	}
	
	return result;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OBJiObjectList_AddObject(
	OBJtObject				*inObject)
{
	UUtError				error;
	UUtUns32				index;
	UUtBool					mem_moved;
	OBJtObject				**object_list;
	
	// if the object list hasn't been created yet, then create it
	if (OBJgObjectArray == NULL)
	{
		OBJgObjectArray =
			UUrMemory_Array_New(
				sizeof(OBJtObject*),
				sizeof(OBJtObject*),
				0,
				0);
		UUmError_ReturnOnNull(OBJgObjectArray);
	}
	
	// create a new element in the array
	error = UUrMemory_Array_GetNewElement(OBJgObjectArray, &index, &mem_moved);
	UUmError_ReturnOnError(error);
	
	// get a pointer to the memory array
	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(OBJgObjectArray);
	if (object_list == NULL) { return UUcError_Generic; }
	
	object_list[index] = inObject;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObjectList_DeleteObject(
	UUtUns32				inIndex)
{
	if (OBJgObjectArray)
	{
		OBJtObject			**object_list;
		
		object_list = (OBJtObject**)UUrMemory_Array_GetMemory(OBJgObjectArray);
		if (object_list)
		{
			// delete the object
			UUrMemory_Block_Delete(object_list[inIndex]);
			
			// delete the array entry
			UUrMemory_Array_DeleteElement(OBJgObjectArray, inIndex);
		}
	}
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiObjectList_GetNumObjects(
	void)
{
	if (OBJgObjectArray == NULL) { return 0; }
	
	return UUrMemory_Array_GetUsedElems(OBJgObjectArray);
}

// ----------------------------------------------------------------------
static OBJtObject*
OBJiObjectList_GetObject(
	UUtUns32				inIndex)
{
	OBJtObject				*object;
	UUtUns32				num_objects;
	
	object = NULL;
	
	if (OBJgObjectArray)
	{
		// get the number of objects in the array and make sure
		// inIndex is in range
		num_objects = UUrMemory_Array_GetUsedElems(OBJgObjectArray);
		if ((num_objects > 0) && (inIndex < num_objects))
		{
			OBJtObject		**object_list;
			
			// get the object from the object list
			object_list = (OBJtObject**)UUrMemory_Array_GetMemory(OBJgObjectArray);
			if (object_list)
			{
				object = object_list[inIndex];
			}
		}
	}
	
	return object;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectList_Initialize(
	void)
{
	UUmAssert(OBJgObjectArray == NULL);
	
	OBJgObjectArray = NULL;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObjectList_Terminate(
	void)
{
	UUtUns32				num_objects;
	OBJtObject				**object_list;
	UUtUns32				i;
	
	if (OBJgObjectArray == NULL) { return; }
	
	// delete the objects in the array
	num_objects = UUrMemory_Array_GetUsedElems(OBJgObjectArray);
	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(OBJgObjectArray);
	
	for (i = 0; i < num_objects; i++)
	{
		UUrMemory_Block_Delete(object_list[i]);
	}
		
	// delete the array
	UUrMemory_Array_Delete(OBJgObjectArray);
	OBJgObjectArray = NULL;

}

// ----------------------------------------------------------------------
void
OBJrObjectList_DrawObjects(
	void)
{
	UUtUns32				num_objects;
	OBJtObject				**object_list;
	UUtUns32				i;
	
	if (OBJgObjectArray == NULL) { return; }
	
	// draw the objects in the array
	num_objects = UUrMemory_Array_GetUsedElems(OBJgObjectArray);
	object_list = (OBJtObject**)UUrMemory_Array_GetMemory(OBJgObjectArray);
	
	for (i = 0; i < num_objects; i++)
	{
		OBJrObject_Draw(object_list[i]);
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiObjectFile_Close(
	BFtFile					*inObjectFile)
{
	// close the object file
	BFrFile_Close(inObjectFile);
}

// ----------------------------------------------------------------------
static OBJtProcessors		OBJgProcessors[] =
{
	{ 0x0000001,	OBJiProcessor_1 },
	{ 0x0000000,	NULL }
};

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_CreateObjectsFromFile(
	BFtFile					*inObjectFile)
{
	UUtError				error;
	UUtUns8					data[OBJcMaxBytesInBuffer];	/* XXX - may need to align */
	UUtUns8					*buffer;
	UUtBool					swap_it;
	UUtUns32				version;
	UUtUns32				i;
	
	UUmAssert(inObjectFile);
	
	// read the swap flag from file
	error = BFrFile_Read(inObjectFile, sizeof(UUtUns32), data);
	UUmError_ReturnOnError(error);
	buffer = data;
	
	// determine if the file needs to be swapped
	swap_it = UUcFalse;
	if (*(UUtUns32*)(buffer) != OBJcSwap)
	{
		UUrSwap_4Byte(buffer);
		if (*(UUtUns32*)(buffer) != OBJcSwap) { return UUcError_Generic; }
		
		swap_it = UUcTrue;
	}
	
	// read the version flag from the file
	error = BFrFile_Read(inObjectFile, sizeof(UUtUns32), data);
	UUmError_ReturnOnError(error);
	buffer = data;
	
	OBJmGet4BytesFromBuffer(buffer, version, UUtUns32, swap_it);
	
	// process the rest of the file
	for (i = 0; ; i++)
	{
		if (OBJgProcessors[i].processor == NULL)
		{
			UUmAssert(!"Unknown object file version");
			error = UUcError_Generic;
			break;
		}
		
		if (OBJgProcessors[i].version == version)
		{
			error = OBJgProcessors[i].processor(inObjectFile, swap_it);
			break;
		}
	}
	UUmError_ReturnOnError(error);
	
	// make sure no objects are selected
	OBJrObject_Select(NULL);
		
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiObjectFile_FileExists(
	BFtFileRef				*inObjectFileRef)
{
	return BFrFileRef_FileExists(inObjectFileRef);
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_MakeFileRef(
	UUtUns16				inLevelNum,
	BFtFileRef				**outObjectFileRef)
{
	UUtError				error;
	char					filename[128];
	BFtFileRef				*object_file_ref;
	
	UUmAssert(outObjectFileRef);
	
	*outObjectFileRef = NULL;
	
	// find the appropriate directory
	
	// make the filename
	sprintf(filename, "Level%d_Objects.txt", inLevelNum);
	
	// make the file ref
	error = BFrFileRef_MakeFromName(filename, &object_file_ref);
	UUmError_ReturnOnError(error);
	
	*outObjectFileRef = object_file_ref;
	
	return UUcError_None; 
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_New(
	BFtFileRef				*inObjectFileRef)
{
	UUtError				error;
	
	// create the object file
	error = BFrFile_Create(inObjectFileRef);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_Open(
	BFtFileRef				*inObjectFileRef,
	BFtFile					**outObjectFile)
{
	UUtError				error;
	BFtFile					*object_file;
	
	// open the object file
	error = BFrFile_Open(inObjectFileRef, "rw", &object_file);
	UUmError_ReturnOnError(error);
	
	*outObjectFile = object_file;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObjectFile_SaveObjectsToFile(
	BFtFile					*inObjectFile)
{
	UUtError				error;
	UUtUns32				num_objects;
	UUtUns32				i;
	UUtUns32				num_bytes;
	UUtUns8					buffer[OBJcMaxBytesInBuffer];	/* XXX - may need to align */
	
	// set the pos to 0
	error = BFrFile_SetPos(inObjectFile, 0);
	UUmError_ReturnOnError(error);
	
	// put swap flag into the file
	*(UUtUns32*)(buffer) = OBJcSwap;
	error = BFrFile_Write(inObjectFile, sizeof(UUtUns32), buffer);
	UUmError_ReturnOnError(error);
	
	// put the version number into the file
	*(UUtUns32*)(buffer) = OBJcObjectFileVersion;
	error = BFrFile_Write(inObjectFile, sizeof(UUtUns32), buffer);
	UUmError_ReturnOnError(error);
	
	// get the number of objects to save
	num_objects = OBJiObjectList_GetNumObjects();
	for (i = 0; i < num_objects; i++)
	{
		OBJtObject			*object;
		
		// get the object from the object list
		object = OBJiObjectList_GetObject(i);
		if (object == NULL) { continue; }
		
		// produce the necessary data to contain the object
		num_bytes = OBJcMaxBytesInBuffer;
		error = OBJiObject_FillBuffer(object, buffer, &num_bytes);
		UUmError_ReturnOnError(error);
		
		// save the object to the file
		error = BFrFile_Write(inObjectFile, num_bytes, buffer);
		UUmError_ReturnOnError(error);
	}
	
	// put end flag into the file
	*(UUtUns32*)(buffer) = OBJcEnd;
	error = BFrFile_Write(inObjectFile, sizeof(UUtUns32), buffer);
	UUmError_ReturnOnError(error);
	
	// if EOF needs to be set, try BFrFile_GetPos(inObjectFile, &pos), BFrFile_SetPos(inObjectFile, pos);
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiObject_Character_Delete(
	OBJtObject				*ioObject)
{
	OBJtOSD_Character		*character_osd;
	ONtCharacter			*character_list;
	ONtCharacter			*character;

	// get a pointer to the object osd
	character_osd = (OBJtOSD_Character*)ioObject->object_data;
	if (character_osd->character_index == OBJcNoCharacter) { return; }

	// get a pointer to the character 
	character_list = ONrGameState_GetCharacterList();
	character = &character_list[character_osd->character_index];
	
	ONrGameState_DeleteCharacter(character);
	character_osd->character_index = OBJcNoCharacter;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Character_FillBuffer(
	OBJtObject				*inObject,
	UUtUns8					**ioBuffer,
	UUtUns32				*ioNumBytes)
{
	OBJtOSD_Character		*character_osd;
	
	character_osd = (OBJtOSD_Character*)inObject->object_data;
	
	if (*ioNumBytes < sizeof(OBJtOSD_Character)) { return UUcError_Generic; }
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_Character_GetOSD(
	OBJtObject				*inObject,
	OBJtOSD_All				*outObjectSpecificData)
{
	OBJtOSD_Character		*character_osd;
	
	character_osd = (OBJtOSD_Character*)inObject->object_data;
	
	outObjectSpecificData->character_osd = *character_osd;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Character_New(
	OBJtObject				*inObject)
{
	UUtError				error;
	OBJtOSD_Character		*character_osd;
	AItCharacterSetup		character_setup;
	ONtCharacterClass		*character_class;
	
	// get a pointer to the object osd
	character_osd = (OBJtOSD_Character*)inObject->object_data;
	
	// init some fields of the character_osd
	UUrString_Copy(character_osd->character_class, "konoko_generic", 64);
	UUrString_Copy(character_osd->character_name, "none", ONcMaxPlayerNameLength);
	character_osd->character_index = OBJcNoCharacter;
	
	// get the character class
	error =
		TMrInstance_GetDataPtr(
			TRcTemplate_CharacterClass,
			character_osd->character_class,
			&character_class);
	UUmError_ReturnOnError(error);
	
	// set the hitpoints
	character_osd->hit_points = character_class->maxHitPoints;

	// init the character setup
	UUrString_Copy(character_setup.playerName, character_osd->character_name, ONcMaxPlayerNameLength);
	character_setup.defaultScriptID = 0;
	character_setup.defaultFlagID = 0;
	character_setup.flags = 0;
	character_setup.teamNumber = 0;
	character_setup.characterClass = character_class;
	character_setup.variable[0] = '\0';
	character_setup.scripts.spawn_name[0] = '\0';
	character_setup.weapon = NULL;
	character_setup.ammo = 0;
	character_setup.leaveAmmo = 0;
	character_setup.leaveClips = 0;
	character_setup.leaveHypos = 0;
	character_setup.leaveCells = 0;
	
	// create the new character
	error = ONrGameState_NewCharacter(&character_setup, NULL, &character_osd->character_index);
	UUmError_ReturnOnError(error);
		
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Character_ReadFromFile_1(
	BFtFile					*inFile,
	OBJtOSD_All				*outOSD)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Character_SetOSD(
	OBJtObject				*ioObject,
	OBJtOSD_All				*inObjectSpecificData)
{
	UUtError				error;
	OBJtOSD_Character		*character_osd;
	ONtCharacter			*character_list;
	ONtCharacter			*character;
	UUtUns16				character_index;
	ONtCharacterClass		*character_class;
	
	// get a pointer to the object osd
	character_osd = (OBJtOSD_Character*)ioObject->object_data;
	
	// set OSD
	*character_osd = inObjectSpecificData->character_osd;
	
	// restore the character index
	character_osd->character_index = character_index;
	
	// ------------------------------
	// update the character
	// ------------------------------	
	// get a pointer to the character 
	character_list = ONrGameState_GetCharacterList();
	character = &character_list[character_osd->character_index];
		
	// set the character class
	error =
		TMrInstance_GetDataPtr(
			TRcTemplate_CharacterClass,
			character_osd->character_class,
			&character_class);
	UUmError_ReturnOnError(error);
	
	character->characterClass = character_class;
	
	// set the character name
	UUrString_Copy(character->player_name, character_osd->character_name, ONcMaxPlayerNameLength);
	character->hitPoints = character_osd->hit_points;
	character->teamNumber = character_osd->team_number;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_Character_SetPosition(
	OBJtObject				*inObject)
{
	OBJtOSD_Character		*character_osd;
	ONtCharacter			*character;
	ONtCharacter			*character_list;
	
	// get a pointer to the object osd
	character_osd = (OBJtOSD_Character*)inObject->object_data;
	
	// get the character
	character_list = ONrGameState_GetCharacterList();
	character = &character_list[character_osd->character_index];
	
	// set the character's position
	character->location = character->physics->position = inObject->position;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiObject_Flag_Delete(
	OBJtObject				*ioObject)
{
	OBJtOSD_Flag			*flag_osd;
	
	flag_osd = (OBJtOSD_Flag*)ioObject->object_data;
	
	ONrLevel_DeleteFlag(flag_osd->flag_index);
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Flag_FillBuffer(
	OBJtObject				*inObject,
	UUtUns8					**ioBuffer,
	UUtUns32				*ioNumBytes)
{
	OBJtOSD_Flag			*flag_osd;
	
	flag_osd = (OBJtOSD_Flag*)inObject->object_data;
	
	if (*ioNumBytes < sizeof(UUtUns32)) { return UUcError_Generic; }
	OBJmWrite4BytesToBuffer(*ioBuffer, flag_osd->flag_type, UUtUns32, *ioNumBytes);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_Flag_GetOSD(
	OBJtObject				*inObject,
	OBJtOSD_All				*outObjectSpecificData)
{
	OBJtOSD_Flag			*flag_osd;
	
	flag_osd = (OBJtOSD_Flag*)inObject->object_data;
	
	outObjectSpecificData->flag_osd = *flag_osd;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Flag_New(
	OBJtObject				*ioObject)
{
	ONtFlag					flag;
	UUtInt16				flag_id;
	OBJtOSD_Flag			*flag_osd;
	
	M3tPoint3D				rotPoint;
	M3tMatrix4x3			rotMatrix;
	
	// setup the flag
	flag.location	= ioObject->position;
	flag.idNumber	= 0;
	flag.deleted	= UUcFalse;
	flag.maxFlag	= UUcFalse;
	
	MUrMatrix_Identity(&flag.matrix);
	MUrMatrixStack_Translate(&flag.matrix, (M3tVector3D*)&ioObject->position);

	// calculate the matrix and rotation
	rotPoint.x = 1.0f;
	rotPoint.y = 0.0f;
	rotPoint.z = 0.0f;
	
	MUrMatrix_To_RotationMatrix(&flag.matrix, &rotMatrix);
	MUrMatrix_MultiplyPoint(&rotPoint, &rotMatrix, &rotPoint);

	if (UUmFloat_CompareEqu(0.0f, rotPoint.x) && UUmFloat_CompareEqu(0.0f, rotPoint.y))
	{
		flag.rotation = 0.0f;
	}
	else
	{
		flag.rotation = MUrATan2(rotPoint.x, rotPoint.z);
		UUmTrig_ClipLow(flag.rotation);
	}

	// get an unused flag id number
	flag_id = 0;
	while (ONrLevel_Flag_ID_To_Pointer(flag_id) != NULL)
	{
		flag_id++;
	}
	flag.idNumber = flag_id;
	
	ONrLevel_AddFlag(&flag);

	// set the flag index
	flag_osd = (OBJtOSD_Flag*)ioObject->object_data;
	flag_osd->flag_index = ONrLevel_Flag_ID_To_Index(flag.idNumber);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Flag_ReadFromFile_1(
	BFtFile					*inObjectFile,
	UUtBool					inSwapIt,
	OBJtOSD_All				*outOSD)
{
	UUtError				error;
	UUtUns8					data[sizeof(OBJtOSD_All)];
	UUtUns8					*buffer;

	// read the data from the file
	error = BFrFile_Read(inObjectFile, sizeof(UUtUns32), data);
	UUmError_ReturnOnError(error);
	buffer = data;
	
	// copy the data into the osd
	OBJmGet4BytesFromBuffer(buffer, outOSD->flag_osd.flag_type, OBJtFlagType, inSwapIt);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Flag_SetOSD(
	OBJtObject				*ioObject,
	OBJtOSD_All				*inObjectSpecificData)
{
	OBJtOSD_Flag			*flag_osd;
	
	// get a pointer to the object osd
	flag_osd = (OBJtOSD_Flag*)ioObject->object_data;
	
	// set the flag type
	flag_osd->flag_type = inObjectSpecificData->flag_osd.flag_type;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_Flag_SetPosition(
	OBJtObject				*inObject)
{
	OBJtOSD_Flag			*flag_osd;
	
	// get a pointer to the object osd
	flag_osd = (OBJtOSD_Flag*)inObject->object_data;
	
	// set the location of the flag
	ONrLevel_Flag_SetLocation(
		ONrLevel_Flag_Index_To_Pointer(flag_osd->flag_index),
		&inObject->position);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiObject_Furniture_Delete(
	OBJtObject				*ioObject)
{
}

// ----------------------------------------------------------------------
static void
OBJiObject_Furniture_Draw(
	OBJtObject				*inObject)
{
	OBJtOSD_Furniture		*furniture_osd;
	M3tBoundingBox			bBox;
	
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	if (furniture_osd->geometry == NULL) { return; }
	
	
	M3rMatrixStack_Push();
	M3rMatrixStack_ApplyTranslate(inObject->position);
	M3rMatrixStack_Multiply(&furniture_osd->rotation_matrix);
	M3rGeom_State_Commit();
	
	// draw the bounding box if this is the selected object
	if (inObject == OBJgSelectedObject)
	{
		M3rMinMaxBBox_To_BBox(&furniture_osd->geometry->pointArray->minmax_boundingBox, &bBox);
		M3rBBox_Draw_Line(&bBox, IMcShade_White);
	}
		
	M3rGeometry_Draw(furniture_osd->geometry);

	M3rMatrixStack_Pop();
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Furniture_FillBuffer(
	OBJtObject				*inObject,
	UUtUns8					**ioBuffer,
	UUtUns32				*ioNumBytes)
{
	OBJtOSD_Furniture		*furniture_osd;
	
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	
	if (*ioNumBytes < BFcMaxFileNameLength) { return UUcError_Generic; }
	
	// put the geometry name in the buffer
	UUrString_Copy((char*)*ioBuffer, furniture_osd->geometry_name, *ioNumBytes);
	((UUtUns8*)(*ioBuffer)) += BFcMaxFileNameLength;
	*ioNumBytes -= BFcMaxFileNameLength;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_Furniture_GetOSD(
	OBJtObject				*inObject,
	OBJtOSD_All				*outObjectSpecificData)
{
	OBJtOSD_Furniture		*furniture_osd;
	
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	
	outObjectSpecificData->furniture_osd = *furniture_osd;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Furniture_New(
	OBJtObject				*inObject)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Furniture_ReadFromFile_1(
	BFtFile					*inObjectFile,
	UUtBool					inSwapIt,
	OBJtOSD_All				*outOSD)
{
	UUtError				error;
	UUtUns8					data[sizeof(OBJtOSD_All)];
	UUtUns8					*buffer;
	
	// read the data from the file
	error = BFrFile_Read(inObjectFile, BFcMaxFileNameLength, data);
	UUmError_ReturnOnError(error);
	buffer = data;
	
	// copy the data into the osd
	UUrString_Copy(outOSD->furniture_osd.geometry_name, (char*)buffer, BFcMaxFileNameLength);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Furniture_SetOSD(
	OBJtObject				*ioObject,
	OBJtOSD_All				*inObjectSpecificData)
{
	UUtError				error;
	OBJtOSD_Furniture		*furniture_osd;
	
	UUmAssert(inObjectSpecificData);
	
	// get a pointer to the object osd
	furniture_osd = (OBJtOSD_Furniture*)ioObject->object_data;
	
	// set OSD
	UUrString_Copy(
		furniture_osd->geometry_name,
		inObjectSpecificData->furniture_osd.geometry_name,
		BFcMaxFileNameLength);
	
	// get a pointer to the geometry
	if (furniture_osd->geometry_name[0] != '\0')
	{
		error =
			TMrInstance_GetDataPtr(
				M3cTemplate_Geometry,
				furniture_osd->geometry_name,
				&furniture_osd->geometry);
		UUmError_ReturnOnError(error);
	}
	else
	{
		furniture_osd->geometry = NULL;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_Furniture_SetPosition(
	OBJtObject				*inObject)
{
	OBJtOSD_Furniture		*furniture_osd;
	M3tMatrix3x3			rotation_matrix;
	float					rot_x;
	float					rot_y;
	float					rot_z;

	// get a pointer to the object osd
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	
	// convert the rotation to radians
	rot_x = inObject->rotation.x * M3cDegToRad;
	rot_y = inObject->rotation.y * M3cDegToRad;
	rot_z = inObject->rotation.z * M3cDegToRad;
	
	// update the rotation matrix
	MUrMatrix3x3_Identity(&rotation_matrix);
	MUrMatrix3x3_RotateX(&rotation_matrix, rot_x);
	MUrMatrix3x3_RotateY(&rotation_matrix, rot_y);
	MUrMatrix3x3_RotateZ(&rotation_matrix, rot_z);
	
	MUrMatrix4x3_BuildFrom3x3(&furniture_osd->rotation_matrix, &rotation_matrix);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiObject_PowerUp_Delete(
	OBJtObject				*ioObject)
{
	OBJtOSD_PowerUp			*powerup_osd;
	
	// get a pointer to the object osd
	powerup_osd = (OBJtOSD_PowerUp*)ioObject->object_data;
	
	if (powerup_osd->powerup)
	{
		WPrPowerup_Delete(powerup_osd->powerup);
		powerup_osd->powerup = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OBJiObject_PowerUp_Draw(
	OBJtObject				*inObject)
{
	OBJtOSD_PowerUp			*powerup_osd;
	M3tBoundingBox			bBox;
	
	// get a pointer to the object osd
	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;

	// draw the bounding box if this is the selected object
	if (inObject == OBJgSelectedObject)
	{
		M3rMatrixStack_Push();
		M3rMatrixStack_ApplyTranslate(powerup_osd->powerup->position);
		M3rMatrixStack_RotateZAxis(M3cHalfPi);
		M3rGeom_State_Commit();
		
		M3rMinMaxBBox_To_BBox(&powerup_osd->powerup->geometry->pointArray->minmax_boundingBox, &bBox);
		M3rBBox_Draw_Line(&bBox, IMcShade_White);
		
		M3rMatrixStack_Pop();
	}
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_PowerUp_FillBuffer(
	OBJtObject				*inObject,
	UUtUns8					**ioBuffer,
	UUtUns32				*ioNumBytes)
{
	OBJtOSD_PowerUp			*powerup_osd;
	
	// get a pointer to the object osd
	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;
	
	if (*ioNumBytes < sizeof(UUtUns32)) { return UUcError_Generic; }
	OBJmWrite4BytesToBuffer(*ioBuffer, powerup_osd->powerup_type, UUtUns32, *ioNumBytes);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_PowerUp_GetOSD(
	OBJtObject				*inObject,
	OBJtOSD_All				*outObjectSpecificData)
{
	OBJtOSD_PowerUp			*powerup_osd;
	
	// get a pointer to the object osd
	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;
	
	outObjectSpecificData->powerup_osd = *powerup_osd;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_PowerUp_New(
	OBJtObject				*inObject)
{
	OBJtOSD_PowerUp			*powerup_osd;
	WPtPowerup				*powerup;
	WPtPowerupType			type;
	
	// get a pointer to the object osd
	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;
	
	switch (powerup_osd->powerup_type)
	{
		case OBJcPowerUpType_Hypo:
			type = WPcPowerup_Hypo;
		break;
		
		case OBJcPowerUpType_ShieldGreen:
			type = WPcPowerup_ShieldGreen;
		break;
		
		case OBJcPowerUpType_ShieldBlue:
			type = WPcPowerup_ShieldBlue;
		break;
		
		case OBJcPowerUpType_ShieldRed:
			type = WPcPowerup_ShieldRed;
		break;
		
		case OBJcPowerUpType_AmmoEnergy:
			type = WPcPowerup_AmmoEnergy;
		break;
		
		case OBJcPowerUpType_AmmoBallistic:
			type = WPcPowerup_AmmoBallistic;
		break;
	}
	
	powerup = WPrPowerup_New(type, &inObject->position, 0.0f);
	UUmError_ReturnOnNull(powerup);
	
	powerup_osd->powerup = powerup;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_PowerUp_ReadFromFile_1(
	BFtFile					*inObjectFile,
	UUtBool					inSwapIt,
	OBJtOSD_All				*outOSD)
{
	UUtError				error;
	UUtUns8					data[sizeof(OBJtOSD_All)];
	UUtUns8					*buffer;
	
	// read the data from the file
	error = BFrFile_Read(inObjectFile, sizeof(UUtUns32), data);
	UUmError_ReturnOnError(error);
	buffer = data;
	
	// copy the data into the osd
	OBJmGet4BytesFromBuffer(buffer, outOSD->powerup_osd.powerup_type, OBJtPowerUpType, inSwapIt);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_PowerUp_SetOSD(
	OBJtObject				*ioObject,
	OBJtOSD_All				*inObjectSpecificData)
{
	UUtError				error;
	OBJtOSD_PowerUp			*powerup_osd;
	WPtPowerupType			type;
	
	UUmAssert(inObjectSpecificData);
	
	// get a pointer to the object osd
	powerup_osd = (OBJtOSD_PowerUp*)ioObject->object_data;
	
	// set OSD
	powerup_osd->powerup_type = inObjectSpecificData->powerup_osd.powerup_type;
	
	// set the objects data
	switch (powerup_osd->powerup_type)
	{
		case OBJcPowerUpType_Hypo:
			type = WPcPowerup_Hypo;
		break;
		
		case OBJcPowerUpType_ShieldGreen:
			type = WPcPowerup_ShieldGreen;
		break;
		
		case OBJcPowerUpType_ShieldBlue:
			type = WPcPowerup_ShieldBlue;
		break;
		
		case OBJcPowerUpType_ShieldRed:
			type = WPcPowerup_ShieldRed;
		break;
		
		case OBJcPowerUpType_AmmoEnergy:
			type = WPcPowerup_AmmoEnergy;
		break;
		
		case OBJcPowerUpType_AmmoBallistic:
			type = WPcPowerup_AmmoBallistic;
		break;
	}
	
	error = WPrPowerup_SetType(powerup_osd->powerup, type);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_PowerUp_SetPosition(
	OBJtObject				*inObject)
{
	OBJtOSD_PowerUp			*powerup_osd;
	
	// get a pointer to the object osd
	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;
	
	// set the position
	powerup_osd->powerup->position = inObject->position;
	
	// set the orientation
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiObject_Sound_Delete(
	OBJtObject				*ioObject)
{
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Sound_FillBuffer(
	OBJtObject				*inObject,
	UUtUns8					**ioBuffer,
	UUtUns32				*ioNumBytes)
{
	OBJtOSD_Sound			*sound_osd;
	
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	
	if (*ioNumBytes < sizeof(UUtUns32)) { return UUcError_Generic; }
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_Sound_GetOSD(
	OBJtObject				*inObject,
	OBJtOSD_All				*outObjectSpecificData)
{
	OBJtOSD_Sound			*sound_osd;
	
	sound_osd = (OBJtOSD_Sound*)inObject->object_data;
	
	outObjectSpecificData->sound_osd = *sound_osd;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Sound_New(
	OBJtObject				*inObject)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Sound_ReadFromFile_1(
	BFtFile					*inObjectFile,
	UUtBool					inSwapIt,
	OBJtOSD_All				*outOSD)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Sound_SetOSD(
	OBJtObject				*ioObject,
	OBJtOSD_All				*inObjectSpecificData)
{
	OBJtOSD_Sound			*sound_osd;
	
	UUmAssert(inObjectSpecificData);
	
	// get a pointer to the object osd
	sound_osd = (OBJtOSD_Sound*)ioObject->object_data;
	
	// set OSD
	*sound_osd = inObjectSpecificData->sound_osd;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_Sound_SetPosition(
	OBJtObject				*inObject)
{
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiObject_TrigVol_Delete(
	OBJtObject				*ioObject)
{
}

// ----------------------------------------------------------------------
static void
OBJiObject_TrigVol_Draw(
	OBJtObject				*inObject)
{
	OBJtOSD_TrigVol			*trigvol_osd;
	
	trigvol_osd = (OBJtOSD_TrigVol*)inObject->object_data;
	if (trigvol_osd->trigger == NULL) { return; }
	
	M3rMatrixStack_Push();
	
	// draw the bounding box if this is the selected object
	if (inObject == OBJgSelectedObject)
	{
		ONrLevel_Trigger_Display(trigvol_osd->trigger, IMcShade_White);
	}
	else
	{
		ONrLevel_Trigger_Display(trigvol_osd->trigger, IMcShade_Blue);
	}
	
	M3rMatrixStack_Pop();
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_TrigVol_FillBuffer(
	OBJtObject				*inObject,
	UUtUns8					**ioBuffer,
	UUtUns32				*ioNumBytes)
{
	OBJtOSD_TrigVol			*trigvol_osd;
	
	// get a pointer to the object osd
	trigvol_osd = (OBJtOSD_TrigVol*)inObject->object_data;
	
	if (*ioNumBytes < (sizeof(M3tPoint3D) /*+ sizeof(M3tQuaternion)*/ + sizeof(UUtInt32))) { return UUcError_Generic; }
	
	OBJmWrite4BytesToBuffer(*ioBuffer, trigvol_osd->scale.x, float, *ioNumBytes);
	OBJmWrite4BytesToBuffer(*ioBuffer, trigvol_osd->scale.y, float, *ioNumBytes);
	OBJmWrite4BytesToBuffer(*ioBuffer, trigvol_osd->scale.z, float, *ioNumBytes);
/*	OBJmWrite4BytesToBuffer(*ioBuffer, trigvol_osd->orientation.x, float, *ioNumBytes);
	OBJmWrite4BytesToBuffer(*ioBuffer, trigvol_osd->orientation.y, float, *ioNumBytes);
	OBJmWrite4BytesToBuffer(*ioBuffer, trigvol_osd->orientation.z, float, *ioNumBytes);
	OBJmWrite4BytesToBuffer(*ioBuffer, trigvol_osd->orientation.w, float, *ioNumBytes);*/
	OBJmWrite4BytesToBuffer(*ioBuffer, trigvol_osd->id, UUtInt32, *ioNumBytes);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_TrigVol_GetBoundingSphere(
	OBJtObject				*inObject,
	M3tBoundingSphere		*outBoundingSphere)
{
	OBJtOSD_TrigVol			*trigvol_osd;
	float					radius;
	
	// get a pointer to the object osd
	trigvol_osd = (OBJtOSD_TrigVol*)inObject->object_data;
	
	radius = UUmMax(trigvol_osd->scale.x, trigvol_osd->scale.y);
	radius = UUmMax(radius, trigvol_osd->scale.z);
	
	outBoundingSphere->center.x = 0;
	outBoundingSphere->center.y = 0;
	outBoundingSphere->center.z = 0;
	outBoundingSphere->radius = radius;
}

// ----------------------------------------------------------------------
static void
OBJiObject_TrigVol_GetOSD(
	OBJtObject				*inObject,
	OBJtOSD_All				*outObjectSpecificData)
{
	OBJtOSD_TrigVol			*trigvol_osd;
	
	// get a pointer to the object osd
	trigvol_osd = (OBJtOSD_TrigVol*)inObject->object_data;
	
	outObjectSpecificData->trigvol_osd = *trigvol_osd;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_TrigVol_New(
	OBJtObject				*inObject)
{
	OBJtOSD_TrigVol			*trigvol_osd;
	
	// get a pointer to the object osd
	trigvol_osd = (OBJtOSD_TrigVol*)inObject->object_data;
	
	trigvol_osd->trigger	= ONrLevel_Trigger_New();
	UUmError_ReturnOnNull(trigvol_osd->trigger);

	trigvol_osd->scale.x = 5.0f;
	trigvol_osd->scale.y = 5.0f;
	trigvol_osd->scale.z = 5.0f;

	trigvol_osd->trigger->location		= inObject->position;
	trigvol_osd->trigger->orientation	= MUgZeroQuat;
	trigvol_osd->trigger->scale			= trigvol_osd->scale;
	trigvol_osd->trigger->idNumber		= -1;
	
	ONrLevel_Trigger_TransformPoints(trigvol_osd->trigger);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_TrigVol_ReadFromFile_1(
	BFtFile					*inObjectFile,
	UUtBool					inSwapIt,
	OBJtOSD_All				*outOSD)
{
	UUtError				error;
	UUtUns8					data[sizeof(OBJtOSD_All)];
	UUtUns8					*buffer;
	
	// read the data from the file
	error = BFrFile_Read(inObjectFile, sizeof(M3tPoint3D) /*+ sizeof(M3tQuaternion)*/ + sizeof(UUtInt32), data);
	UUmError_ReturnOnError(error);
	buffer = data;
	
	// copy the data into the osd
	OBJmGet4BytesFromBuffer(buffer, outOSD->trigvol_osd.scale.x, float, inSwapIt);
	OBJmGet4BytesFromBuffer(buffer, outOSD->trigvol_osd.scale.y, float, inSwapIt);
	OBJmGet4BytesFromBuffer(buffer, outOSD->trigvol_osd.scale.z, float, inSwapIt);
	
/*	OBJmGet4BytesFromBuffer(buffer, outOSD->trigvol_osd.orientation.x, float, inSwapIt);
	OBJmGet4BytesFromBuffer(buffer, outOSD->trigvol_osd.orientation.y, float, inSwapIt);
	OBJmGet4BytesFromBuffer(buffer, outOSD->trigvol_osd.orientation.z, float, inSwapIt);
	OBJmGet4BytesFromBuffer(buffer, outOSD->trigvol_osd.orientation.w, float, inSwapIt);*/
	
	OBJmGet4BytesFromBuffer(buffer, outOSD->trigvol_osd.id, UUtInt32, inSwapIt);	
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_TrigVol_SetOSD(
	OBJtObject				*ioObject,
	OBJtOSD_All				*inObjectSpecificData)
{
	OBJtOSD_TrigVol			*trigvol_osd;
	
	UUmAssert(inObjectSpecificData);
	
	// get a pointer to the object osd
	trigvol_osd = (OBJtOSD_TrigVol*)ioObject->object_data;
	
	// set OSD
	trigvol_osd->scale			= inObjectSpecificData->trigvol_osd.scale;
//	trigvol_osd->orientation	= inObjectSpecificData->trigvol_osd.orientation;
	trigvol_osd->id				= inObjectSpecificData->trigvol_osd.id;
	
	// update the trigger
	trigvol_osd->trigger->scale			= trigvol_osd->scale;
//	trigvol_osd->trigger->orientation	= trigvol_osd->orientation;
	trigvol_osd->trigger->idNumber		= trigvol_osd->id;
	
	ONrLevel_Trigger_TransformPoints(trigvol_osd->trigger);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_TrigVol_SetPosition(
	OBJtObject				*inObject)
{
	OBJtOSD_TrigVol			*trigvol_osd;
	
	// get a pointer to the object osd
	trigvol_osd = (OBJtOSD_TrigVol*)inObject->object_data;
	
	// update the trigger
	trigvol_osd->trigger->location = inObject->position;

	ONrLevel_Trigger_TransformPoints(trigvol_osd->trigger);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiObject_Weapon_Delete(
	OBJtObject				*ioObject)
{
	OBJtOSD_Weapon			*weapon_osd;
	
	// get a pointer to the object osd
	weapon_osd = (OBJtOSD_Weapon*)ioObject->object_data;
	
	if (weapon_osd->weapon)
	{
		WPrDelete(weapon_osd->weapon);
		weapon_osd->weapon = NULL;
	}
}

// ----------------------------------------------------------------------
static void
OBJiObject_Weapon_Draw(
	OBJtObject				*inObject)
{
	OBJtOSD_Weapon			*weapon_osd;
	M3tBoundingBox			bBox;
	
	// get a pointer to the object osd
	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;
	
	if (weapon_osd->weapon == NULL) { return; }
	
	// draw the bounding box if this is the selected object
	if (inObject == OBJgSelectedObject)
	{
		M3rMatrixStack_Push();
		M3rMatrixStack_ApplyTranslate(WPrGetPhysics(weapon_osd->weapon)->position);
		M3rGeom_State_Commit();
		
		M3rMinMaxBBox_To_BBox(&WPrGetClass(weapon_osd->weapon)->geometry->pointArray->minmax_boundingBox, &bBox);
		M3rBBox_Draw_Line(&bBox, IMcShade_White);
		
		M3rMatrixStack_Pop();
	}
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Weapon_FillBuffer(
	OBJtObject				*inObject,
	UUtUns8					**ioBuffer,
	UUtUns32				*ioNumBytes)
{
	OBJtOSD_Weapon			*weapon_osd;
	
	// get a pointer to the object osd
	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;
	
	if (*ioNumBytes < BFcMaxFileNameLength) { return UUcError_Generic; }
	
	// put the weapon class name in the buffer
	UUrString_Copy((char*)*ioBuffer, weapon_osd->weapon_class_name, *ioNumBytes);
	((UUtUns8*)(*ioBuffer)) += BFcMaxFileNameLength;
	*ioNumBytes -= BFcMaxFileNameLength;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_Weapon_GetOSD(
	OBJtObject				*inObject,
	OBJtOSD_All				*outObjectSpecificData)
{
	OBJtOSD_Weapon			*weapon_osd;
	
	// get a pointer to the object osd
	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;
	
	outObjectSpecificData->weapon_osd = *weapon_osd;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Weapon_New(
	OBJtObject				*inObject)
{
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Weapon_ReadFromFile_1(
	BFtFile					*inObjectFile,
	UUtBool					inSwapIt,
	OBJtOSD_All				*outOSD)
{
	UUtError				error;
	UUtUns8					data[sizeof(OBJtOSD_All)];
	UUtUns8					*buffer;
	
	// read the data from the file
	error = BFrFile_Read(inObjectFile, BFcMaxFileNameLength, data);
	UUmError_ReturnOnError(error);
	buffer = data;
	
	// copy the data into the osd
	UUrString_Copy(outOSD->weapon_osd.weapon_class_name, (char*)buffer, BFcMaxFileNameLength);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_Weapon_SetOSD(
	OBJtObject				*ioObject,
	OBJtOSD_All				*inObjectSpecificData)
{
	UUtError				error;
	OBJtOSD_Weapon			*weapon_osd;
	WPtWeaponClass			*weapon_class;
	
	UUmAssert(inObjectSpecificData);
	
	// get a pointer to the object osd
	weapon_osd = (OBJtOSD_Weapon*)ioObject->object_data;
	
	// delete the existing weapon
	if (weapon_osd->weapon)
	{
		WPrDelete(weapon_osd->weapon);
		weapon_osd->weapon = NULL;
	}
	
	// set OSD
	UUrString_Copy(
		weapon_osd->weapon_class_name,
		inObjectSpecificData->weapon_osd.weapon_class_name,
		BFcMaxFileNameLength);
	
	// get the weapon class
	error =
		TMrInstance_GetDataPtr(
			WPcTemplate_WeaponClass,
			weapon_osd->weapon_class_name,
			&weapon_class);
	UUmError_ReturnOnError(error);
	
	// create a weapon using the weapon class
	weapon_osd->weapon = WPrNew(weapon_class);
	UUmError_ReturnOnNull(weapon_osd->weapon);
	
	// set the weapon's position
	OBJrObject_SetPosition(ioObject, &ioObject->position, &ioObject->rotation);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiObject_Weapon_SetPosition(
	OBJtObject				*inObject)
{
	OBJtOSD_Weapon			*weapon_osd;
	
	// get a pointer to the object osd
	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;
	
	WPrSetPosition(weapon_osd->weapon, &inObject->position);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OBJiObject_EnumerateTemplate(
	TMtTemplateTag			inTemplateTag,
	OBJtObjectType			inObjectType,
	OBJtObjectEnumCallback	inObjectEnumCallback,
	UUtUns32				inUserData)
{
#define cMaxInstances		1024
	
	UUtError				error;
	void					*instances[cMaxInstances];
	UUtUns32				num_instances;
	UUtUns32				i;
	UUtBool					result;

	// get a list of instances of the class
	error =
		TMrInstance_GetDataPtr_List(
			inTemplateTag,
			cMaxInstances,
			&num_instances,
			instances);
	UUmError_ReturnOnError(error);
	
	for (i = 0; i < num_instances; i++)
	{
		result =
			inObjectEnumCallback(
				inObjectType,
				TMrInstance_GetInstanceName(instances[i]),
				inUserData);
		if (result == UUcFalse) { break; }
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_EnumerateTypeNames(
	OBJtObjectTypeNames		*inObjectTypeNames,
	OBJtObjectType			inObjectType,
	OBJtObjectEnumCallback	inObjectEnumCallback,
	UUtUns32				inUserData)
{
	UUtUns32				i;
	UUtBool					result;

	for (i = 0; ; i++)
	{
		if (inObjectTypeNames[i].type == OBJcFlagType_None) { break; }
		
		result =
			inObjectEnumCallback(
				inObjectType,
				inObjectTypeNames[i].name,
				inUserData);
		if (result == UUcFalse) { break; }
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiObject_FillBuffer(
	OBJtObject				*inObject,
	UUtUns8					*outBuffer,
	UUtUns32				*ioNumBytes)
{
	UUtError				error;
	UUtUns8					*buffer;
	
	UUmAssert(inObject);
	UUmAssert(outBuffer);
	UUmAssert(ioNumBytes && (*ioNumBytes > sizeof(OBJtObject)));
	
	buffer = outBuffer;
	
	// make sure there is enough room for the basics
	if (*ioNumBytes < sizeof(OBJtObject)) { return UUcError_Generic; }
	
	// fill in the basic data
	OBJmWrite4BytesToBuffer(buffer, inObject->object_type, UUtUns32, *ioNumBytes);
	OBJmWrite4BytesToBuffer(buffer, inObject->position.x, float, *ioNumBytes);
	OBJmWrite4BytesToBuffer(buffer, inObject->position.y, float, *ioNumBytes);
	OBJmWrite4BytesToBuffer(buffer, inObject->position.z, float, *ioNumBytes);
	OBJmWrite4BytesToBuffer(buffer, inObject->rotation.x, float, *ioNumBytes);
	OBJmWrite4BytesToBuffer(buffer, inObject->rotation.y, float, *ioNumBytes);
	OBJmWrite4BytesToBuffer(buffer, inObject->rotation.z, float, *ioNumBytes);
	
	// fill in the type specific data
	switch (inObject->object_type)
	{
		case OBJcType_Character:
			error = OBJiObject_Character_FillBuffer(inObject, &buffer, ioNumBytes);
		break;
		
		case OBJcType_Flag:
			error = OBJiObject_Flag_FillBuffer(inObject, &buffer, ioNumBytes);
		break;
		
		case OBJcType_Furniture:
			error = OBJiObject_Furniture_FillBuffer(inObject, &buffer, ioNumBytes);
		break;
		
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
			error = UUcError_None;
		break;
		
		case OBJcType_PowerUp:
			error = OBJiObject_PowerUp_FillBuffer(inObject, &buffer, ioNumBytes);
		break;
		
		case OBJcType_Sound:
			error = OBJiObject_Sound_FillBuffer(inObject, &buffer, ioNumBytes);
		break;
		
		case OBJcType_TriggerVolume:
			error = OBJiObject_TrigVol_FillBuffer(inObject, &buffer, ioNumBytes);;
		break;
		
		case OBJcType_Weapon:
			error = OBJiObject_Weapon_FillBuffer(inObject, &buffer, ioNumBytes);
		break;
		
		default:
			UUmAssert(!"invalid object type");
			error = UUcError_None;
		break;
	}
	UUmError_ReturnOnError(error);
	
	// set ioNumBytes to the number of bytes written into the buffer
	*ioNumBytes = buffer - outBuffer;
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OBJrObject_Draw(
	OBJtObject				*inObject)
{
	UUmAssert(inObject);
	
	switch (inObject->object_type)
	{
		case OBJcType_Character:		/* drawn elsewhere */
		break;
		
		case OBJcType_Flag:
		break;
		
		case OBJcType_Furniture:
			OBJiObject_Furniture_Draw(inObject);
		break;
		
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
		break;
		
		case OBJcType_PowerUp:
			OBJiObject_PowerUp_Draw(inObject);
		break;
		
		case OBJcType_Sound:
		break;
		
		case OBJcType_TriggerVolume:
			if (OBJgDrawTriggers)
			{
				OBJiObject_TrigVol_Draw(inObject);
			}
		break;
		
		case OBJcType_Weapon:			/* drawn elsewhere */
			OBJiObject_Weapon_Draw(inObject);
		break;
	}
}

// ----------------------------------------------------------------------
UUtError
OBJrObject_Enumerate(
	OBJtObjectType			inObjectType,
	OBJtObjectEnumCallback	inObjectEnumCallback,
	UUtUns32				inUserData)
{
	UUtError				error;
	
	if (OBJiObjectType_IsValid(inObjectType) == UUcFalse) { return UUcError_Generic; }
	
	switch (inObjectType)
	{
		case OBJcType_Character:
			error =	OBJiObject_EnumerateTemplate(TRcTemplate_CharacterClass, inObjectType, inObjectEnumCallback, inUserData);
		break;
		
		case OBJcType_Flag:
			error = OBJiObject_EnumerateTypeNames(OBJgFlagTypes, inObjectType, inObjectEnumCallback, inUserData);
		break;
		
		case OBJcType_Furniture:
			error =	OBJiObject_EnumerateTemplate(M3cTemplate_Geometry, inObjectType, inObjectEnumCallback, inUserData);
		break;
		
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
		break;
		
		case OBJcType_PowerUp:
			error = OBJiObject_EnumerateTypeNames(OBJgPowerUpTypes, inObjectType, inObjectEnumCallback, inUserData);
		break;
		
		case OBJcType_Sound:
//			error = OBJiObject_EnumerateTypeNames(OBJgSoundTypes, inObjectType, inObjectEnumCallback, inUserData);
		break;
		
		case OBJcType_TriggerVolume:
			error = UUcError_None;
		break;
		
		case OBJcType_Weapon:
			error = OBJiObject_EnumerateTemplate(WPcTemplate_WeaponClass, inObjectType, inObjectEnumCallback, inUserData);
		break;
	}
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrObject_GetObjectSpecificData(
	OBJtObject				*inObject,
	OBJtOSD_All				*outObjectSpecificData)
{
	UUmAssert(inObject);
	UUmAssert(outObjectSpecificData);
	
	// clear the object specific data structure
	UUrMemory_Clear(outObjectSpecificData, sizeof(OBJtOSD_All));
	
	switch (inObject->object_type)
	{
		case OBJcType_Character:
			OBJiObject_Character_GetOSD(inObject, outObjectSpecificData);
		break;
		
		case OBJcType_Flag:
			OBJiObject_Flag_GetOSD(inObject, outObjectSpecificData);
		break;
		
		case OBJcType_Furniture:
			OBJiObject_Furniture_GetOSD(inObject, outObjectSpecificData);
		break;
		
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
		break;
		
		case OBJcType_PowerUp:
			OBJiObject_PowerUp_GetOSD(inObject, outObjectSpecificData);
		break;
		
		case OBJcType_Sound:
			OBJiObject_Sound_GetOSD(inObject, outObjectSpecificData);
		break;
		
		case OBJcType_TriggerVolume:
			OBJiObject_TrigVol_GetOSD(inObject, outObjectSpecificData);
		break;
		
		case OBJcType_Weapon:
			OBJiObject_Weapon_GetOSD(inObject, outObjectSpecificData);
		break;
	}
}

// ----------------------------------------------------------------------
void
OBJrObject_GetPosition(
	OBJtObject				*inObject,
	M3tPoint3D				*outPosition,
	M3tPoint3D				*outRotation)
{
	UUmAssert(inObject);
	UUmAssert(outPosition);
	UUmAssert(outRotation);
	
	*outPosition = inObject->position;
	*outRotation = inObject->rotation;
}

// ----------------------------------------------------------------------
OBJtObject*
OBJrObject_GetSelectedObject(
	void)
{
	return OBJgSelectedObject;
}

// ----------------------------------------------------------------------
OBJtObjectType
OBJrObject_GetType(
	OBJtObject				*inObject)
{
	UUmAssert(inObject);
	
	return inObject->object_type;
}

// ----------------------------------------------------------------------
UUtBool
OBJrObject_IntersectsLine(
	OBJtObject				*inObject,
	M3tPoint3D				*inStartPoint,
	M3tPoint3D				*inEndPoint)
{
	M3tBoundingSphere		sphere;
	OBJtOSD_All				*osd_all;
	
	UUmAssert(inObject);
	UUmAssert(inStartPoint);
	UUmAssert(inEndPoint);

	UUrMemory_Clear(&sphere, sizeof(M3tBoundingSphere));
	
	// get at pointer to the osd
	osd_all = (OBJtOSD_All*)inObject->object_data;
	
	switch (inObject->object_type)
	{
		case OBJcType_Character:
		break;
		
		case OBJcType_Flag:
			sphere.center.x = 0.0f;
			sphere.center.y = 0.0f;
			sphere.center.z = 0.0f;
			sphere.radius = 5.0f;
		break;
		
		case OBJcType_Furniture:
			sphere = osd_all->furniture_osd.geometry->pointArray->boundingSphere;
		break;
		
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
		break;
		
		case OBJcType_PowerUp:
			sphere = osd_all->powerup_osd.powerup->geometry->pointArray->boundingSphere;
		break;
		
		case OBJcType_Sound:
		break;
		
		case OBJcType_TriggerVolume:
			OBJiObject_TrigVol_GetBoundingSphere(inObject, &sphere);
		break;
		
		case OBJcType_Weapon:
			WPrGetPosition(osd_all->weapon_osd.weapon, &inObject->position);
			sphere = WPrGetClass(osd_all->weapon_osd.weapon)->geometry->pointArray->boundingSphere;
		break;
	}
	
	sphere.center.x += inObject->position.x;
	sphere.center.y += inObject->position.y;
	sphere.center.z += inObject->position.z;
	
	return CLrSphere_Line(inStartPoint, inEndPoint, &sphere);
}

// ----------------------------------------------------------------------
UUtError
OBJrObject_New(
	OBJtObjectType			inObjectType,
	M3tPoint3D				*inPosition,
	M3tPoint3D				*inRotation,
	OBJtOSD_All				*inObjectSpecificData)
{
	UUtError				error;
	OBJtObject				*object;
	UUtUns32				object_size;
	
	// make sure the object type is valid
	if (OBJiObjectType_IsValid(inObjectType) == UUcFalse) { return UUcError_Generic; }
	
	// calculate the amount of memory needed for the object
	object_size = sizeof(OBJtObject);
	switch (inObjectType)
	{
		case OBJcType_Character:
			object_size += sizeof(OBJtOSD_Character);
		break;
		
		case OBJcType_Flag:
			object_size += sizeof(OBJtOSD_Flag);
		break;
		
		case OBJcType_Furniture:
			object_size += sizeof(OBJtOSD_Furniture);
		break;
		
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
		break;
		
		case OBJcType_PowerUp:
			object_size += sizeof(OBJtOSD_PowerUp);
		break;
		
		case OBJcType_Sound:
			object_size += sizeof(OBJtOSD_Sound);
		break;
		
		case OBJcType_TriggerVolume:
			object_size += sizeof(OBJtOSD_TrigVol);
		break;
		
		case OBJcType_Weapon:
			object_size += sizeof(OBJtOSD_Weapon);
		break;
	}
	
	// allocate memory for the new object
	object = (OBJtObject*)UUrMemory_Block_NewClear(object_size);
	UUmError_ReturnOnNull(object);
	
	// add the object to the object list
	error = OBJiObjectList_AddObject(object);
	UUmError_ReturnOnError(error);
	
	// initialize the object
	object->object_type		= inObjectType;
	object->position		= *inPosition;
	object->rotation		= *inRotation;
	
	switch (object->object_type)
	{
		case OBJcType_Character:
			error = OBJiObject_Character_New(object);
		break;
		
		case OBJcType_Flag:
			error = OBJiObject_Flag_New(object);
		break;
		
		case OBJcType_Furniture:
			error = OBJiObject_Furniture_New(object);
		break;
		
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
			error = UUcError_None;
		break;
		
		case OBJcType_PowerUp:
			error = OBJiObject_PowerUp_New(object);
		break;
		
		case OBJcType_Sound:
			error = OBJiObject_Sound_New(object);
		break;
		
		case OBJcType_TriggerVolume:
			error = OBJiObject_TrigVol_New(object);;
		break;
		
		case OBJcType_Weapon:
			error = OBJiObject_Weapon_New(object);
		break;
	}
	UUmError_ReturnOnError(error);
	
	// set the object specific data
	if (inObjectSpecificData != NULL)
	{
		error = OBJrObject_SetObjectSpecificData(object, inObjectSpecificData);
		UUmError_ReturnOnError(error);
	}
	
	// go through the set position code 
	OBJrObject_SetPosition(object, inPosition, inRotation);

	// select the newly created object
	OBJrObject_Select(object);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrObject_Delete(
	OBJtObject				*inObject)
{
	UUtUns32				i;
	UUtUns32				num_objects;
	
	UUmAssert(inObject);
	
	// get the number of objects to save
	num_objects = OBJiObjectList_GetNumObjects();
	if (num_objects == 0) { return; }
	
	// find the index of the object to delete
	for (i = 0; i < num_objects; i++)
	{
		OBJtObject			*object;
		
		// get the object
		object = OBJiObjectList_GetObject(i);
		if (object != inObject) { continue; }
		
		switch (object->object_type)
		{
			case OBJcType_Character:
				OBJiObject_Character_Delete(inObject);
			break;
			
			case OBJcType_Flag:
				OBJiObject_Flag_Delete(inObject);
			break;
			
			case OBJcType_Furniture:
				OBJiObject_Furniture_Delete(inObject);
			break;
			
			case OBJcType_Machine:
			case OBJcType_LevelSpecificItem:
			break;
			
			case OBJcType_PowerUp:
				OBJiObject_PowerUp_Delete(inObject);
			break;
			
			case OBJcType_Sound:
				OBJiObject_Sound_Delete(inObject);
			break;
			
			case OBJcType_TriggerVolume:
				OBJiObject_TrigVol_Delete(inObject);
			break;
			
			case OBJcType_Weapon:
				OBJiObject_Weapon_Delete(inObject);
			break;
		}
		
		// delete the object
		OBJiObjectList_DeleteObject(i);
		
		// set the selected object to NULL if it was the one deleted
		if (inObject == OBJgSelectedObject)
		{
			OBJgSelectedObject = NULL;
		}
		
		break;
	}
}

// ----------------------------------------------------------------------
void
OBJrObject_DeleteSelected(
	void)
{
	if (OBJgSelectedObject != NULL)
	{
		OBJrObject_Delete(OBJgSelectedObject);
		OBJgSelectedObject = NULL;
	}
}

// ----------------------------------------------------------------------
UUtError
OBJrObject_Select(
	OBJtObject				*inObject)
{
	OBJgSelectedObject = inObject;
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrObject_SetObjectSpecificData(
	OBJtObject				*ioObject,
	OBJtOSD_All				*inObjectSpecificData)
{
	UUtError				error;
	
	UUmAssert(ioObject);
	UUmAssert(inObjectSpecificData);
	
	switch (ioObject->object_type)
	{
		case OBJcType_Character:
			error = OBJiObject_Character_SetOSD(ioObject, inObjectSpecificData);
		break;
		
		case OBJcType_Flag:
			error = OBJiObject_Flag_SetOSD(ioObject, inObjectSpecificData);
		break;
		
		case OBJcType_Furniture:
			error = OBJiObject_Furniture_SetOSD(ioObject, inObjectSpecificData);
		break;
		
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
			error = UUcError_None;
		break;
		
		case OBJcType_PowerUp:
			error = OBJiObject_PowerUp_SetOSD(ioObject, inObjectSpecificData);
		break;
		
		case OBJcType_Sound:
			error = OBJiObject_Sound_SetOSD(ioObject, inObjectSpecificData);
		break;
		
		case OBJcType_TriggerVolume:
			error = OBJiObject_TrigVol_SetOSD(ioObject, inObjectSpecificData);;
		break;
		
		case OBJcType_Weapon:
			error = OBJiObject_Weapon_SetOSD(ioObject, inObjectSpecificData);
		break;
		
		default:
			UUmAssert(!"invalid object type");
			error = UUcError_Generic;
		break;
	}
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrObject_SetPosition(
	OBJtObject				*ioObject,
	M3tPoint3D				*inPosition,
	M3tPoint3D				*inRotation)
{
	UUmAssert(ioObject);
	UUmAssert(inPosition);
	UUmAssert(inRotation);
	
	ioObject->position = *inPosition;
	ioObject->rotation = *inRotation;
	
	switch (ioObject->object_type)
	{
		case OBJcType_Character:
			OBJiObject_Character_SetPosition(ioObject);
		break;
		
		case OBJcType_Flag:
			OBJiObject_Flag_SetPosition(ioObject);
		break;
		
		case OBJcType_Furniture:
			OBJiObject_Furniture_SetPosition(ioObject);
		break;
		
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
		break;
		
		case OBJcType_PowerUp:
			OBJiObject_PowerUp_SetPosition(ioObject);
		break;
		
		case OBJcType_Sound:
			OBJiObject_Sound_SetPosition(ioObject);
		break;
		
		case OBJcType_TriggerVolume:
			OBJiObject_TrigVol_SetPosition(ioObject);
		break;
		
		case OBJcType_Weapon:
			OBJiObject_Weapon_SetPosition(ioObject);
		break;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OBJiOSD_ReadFromFile_1(
	BFtFile					*inObjectFile,
	OBJtObjectType			inObjectType,
	UUtBool					inSwapIt,
	OBJtOSD_All				*outOSD)
{
	UUtError				error;
	
	UUmAssert(inObjectFile);
	UUmAssert(outOSD);
	
	switch (inObjectType)
	{
		case OBJcType_Character:
			error = OBJiObject_Flag_ReadFromFile_1(inObjectFile, inSwapIt, outOSD);
		break;
		
		case OBJcType_Flag:
			error = OBJiObject_Flag_ReadFromFile_1(inObjectFile, inSwapIt, outOSD);
		break;
		
		case OBJcType_Furniture:
			error = OBJiObject_Furniture_ReadFromFile_1(inObjectFile, inSwapIt, outOSD);
		break;
		
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
		break;
		
		case OBJcType_PowerUp:
			error = OBJiObject_PowerUp_ReadFromFile_1(inObjectFile, inSwapIt, outOSD);
		break;
		
		case OBJcType_Sound:
			error = OBJiObject_Sound_ReadFromFile_1(inObjectFile, inSwapIt, outOSD);
		break;
		
		case OBJcType_TriggerVolume:
			error = OBJiObject_TrigVol_ReadFromFile_1(inObjectFile, inSwapIt, outOSD);
		break;
		
		case OBJcType_Weapon:
			error = OBJiObject_Weapon_ReadFromFile_1(inObjectFile, inSwapIt, outOSD);
		break;
	}
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrOSD_FlagToName(
	UUtUns32				inFlag,
	OBJtObjectType			inObjectType,
	char					*outName)
{
	UUmAssert(outName);
	
	if (OBJiObjectType_IsValid(inObjectType) == UUcFalse) { return; }
	
	switch(inObjectType)
	{
		case OBJcType_Character:
		break;
		
		case OBJcType_Flag:
			OBJiFlagToString(inFlag, OBJgFlagTypes, outName);
		break;
		
		case OBJcType_Furniture:
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
		break;
		
		case OBJcType_PowerUp:
			OBJiFlagToString(inFlag, OBJgPowerUpTypes, outName);
		break;
		
		case OBJcType_Sound:
		case OBJcType_TriggerVolume:
		case OBJcType_Weapon:
		break;
	}
}

// ----------------------------------------------------------------------
void
OBJrOSD_NameToFlag(
	char					*inName,
	OBJtObjectType			inObjectType,
	UUtUns32				*outFlag)
{
	UUmAssert(inName);
	UUmAssert(outFlag);
	
	if (OBJiObjectType_IsValid(inObjectType) == UUcFalse) { return; }
	
	switch(inObjectType)
	{
		case OBJcType_Character:
		break;
		
		case OBJcType_Flag:
			OBJiStringToFlag(inName, OBJgFlagTypes, outFlag);
		break;
		
		case OBJcType_Furniture:
		case OBJcType_Machine:
		case OBJcType_LevelSpecificItem:
		break;
		
		case OBJcType_PowerUp:
			OBJiStringToFlag(inName, OBJgPowerUpTypes, outFlag);
		break;
		
		case OBJcType_Sound:
		case OBJcType_TriggerVolume:
		case OBJcType_Weapon:
		break;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBJrLevel_Load(
	UUtUns16				inLevelNum)
{
	UUtError				error;
	BFtFileRef				*object_file_ref;
	BFtFile					*object_file;
	
	// no objects are selected
	OBJrObject_Select(NULL);

	// initialize the object list
	error = OBJiObjectList_Initialize();
	UUmError_ReturnOnError(error);
	
	// initialize object file
	OBJgObjectFile = NULL;
	
	// create the object file ref
	error = OBJiObjectFile_MakeFileRef(inLevelNum, &object_file_ref);
	UUmError_ReturnOnError(error);
	
	// find the object file associated with this level
	if (OBJiObjectFile_FileExists(object_file_ref))
	{
		// open the object file
		error = OBJiObjectFile_Open(object_file_ref, &object_file);
		UUmError_ReturnOnError(error);
		
		// parse the file and create the objects it contains
		error = OBJiObjectFile_CreateObjectsFromFile(object_file);
		UUmError_ReturnOnError(error);
		
		OBJgObjectFile = object_file;
	}
	
	// delete the file ref
	BFrFileRef_Dispose(object_file_ref);
	object_file_ref = NULL;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrLevel_SaveObjects(
	void)
{
	UUtError				error;
	UUtUns16				level_num;
	
	// no objects are selected
	OBJrObject_Select(NULL);

	// create the object file if it doesn't already exist
	if (OBJgObjectFile == NULL)
	{
		BFtFileRef				*object_file_ref;
		BFtFile					*object_file;
		
		// get the current level number
		level_num = ONrLevel_GetCurrentLevel();
		if (level_num == 0) { return UUcError_Generic; }
		
		// create the object file ref
		error = OBJiObjectFile_MakeFileRef(level_num, &object_file_ref);
		UUmError_ReturnOnError(error);
		
		// if the file doesn't exist, create it
		if (OBJiObjectFile_FileExists(object_file_ref) == UUcFalse)
		{
			// create the object file
			error = OBJiObjectFile_New(object_file_ref);
			UUmError_ReturnOnError(error);
		}
		
		// open the object file
		error = OBJiObjectFile_Open(object_file_ref, &object_file);
		UUmError_ReturnOnError(error);
		
		OBJgObjectFile = object_file;
		
		// delete the file ref
		BFrFileRef_Dispose(object_file_ref);
		object_file_ref = NULL;
		
		if (OBJgObjectFile == NULL) { return UUcError_Generic; }
	}
	
	error = OBJiObjectFile_SaveObjectsToFile(OBJgObjectFile);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OBJrLevel_Unload(
	void)
{
	// save the objects to the object file
	OBJrLevel_SaveObjects();
	
	// close the object file
	OBJiObjectFile_Close(OBJgObjectFile);
	
	// terminate the object list
	OBJiObjectList_Terminate();	
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
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
	
	// get the active camera
	M3rCamera_GetActive(&camera);
	UUmAssert(camera);
	
	// get the point on the near clip plane where the user clicked
	M3rPick_ScreenToWorld(camera, (UUtUns16)inPoint->x, (UUtUns16)inPoint->y, &world_point);
	
	// get the camera position
	CArGetPosition(&camera_position);
	
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
	
	// loop through each object and find the one closest to the camera that
	// collides with the line
	num_objects = OBJiObjectList_GetNumObjects();
	for (i = 0; i < num_objects; i++)
	{
		OBJtObject				*object;
		
		object = OBJiObjectList_GetObject(i);
		
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
	
	return closest_to_camera;
}

// ----------------------------------------------------------------------
void
OBJrDrawTriggers(
	UUtBool					inDrawTriggers)
{
	OBJgDrawTriggers = inDrawTriggers;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns16
OBJiENVFile_GetNumGunkObjects(
	void)
{
	UUtUns32				num_objects;
	UUtUns32				i;
	UUtUns16				num_gunk_objects;
	
	num_objects = OBJiObjectList_GetNumObjects();
	num_gunk_objects = 0;
	
	for (i = 0; i < num_objects; i++)
	{
		OBJtObject			*object;
		
		object = OBJiObjectList_GetObject(i);
		if (object == NULL) { continue; }
		
		if (object->object_type == OBJcType_Furniture)
		{
			num_gunk_objects++;
		}
	}
	
	return num_gunk_objects;
}

// ----------------------------------------------------------------------
static UUtError
OBJiENVFile_WriteNode(
	BFtFile					*inFile,
	OBJtObject				*inObject)
{
	UUtError				error;
	MXtNode					*node;
	MXtPoint				*points;
	MXtFace					*tri_faces;
	MXtMaterial				*materials;
	OBJtOSD_Furniture		*furniture_osd;
	M3tGeometry				*geometry;
	UUtUns16				i;
	UUtUns32				points_size;
	UUtUns32				tri_faces_size;
	UUtUns32				materials_size;
	
	UUtUns16				index0;
	UUtUns16				index1;
	UUtUns16				index2;
	UUtUns16*				curIndexPtr;

	// get a pointer to the geometry
	furniture_osd = (OBJtOSD_Furniture*)inObject->object_data;
	geometry = furniture_osd->geometry;
	
	// ------------------------------
	// allocate memory for the node
	node = (MXtNode*)UUrMemory_Block_NewClear(sizeof(MXtNode));
	
	// copy the name and parentName into the node
	UUrString_Copy(node->name, "", MXcMaxName);
	UUrString_Copy(node->parentName, "", MXcMaxName);
	
	// set the number of points
	node->numPoints = geometry->pointArray->numPoints;
	node->numTriangles = geometry->triNormalArray->numVectors;
	node->numMaterials = 1;
	
#if (UUmEndian == UUmEndian_Big)
	UUrSwap_2Byte(&node->numPoints);
	UUrSwap_2Byte(&node->numTriangles);
	UUrSwap_2Byte(&node->numMaterials);
#endif
	
	// write the node
	error = BFrFile_Write(inFile, sizeof(MXtNode), node);
	UUmError_ReturnOnError(error);
	
	UUrMemory_Block_Delete(node);
	node = NULL;
	
	// ------------------------------
	// make the points
	points_size = sizeof(MXtPoint) * geometry->pointArray->numPoints;
	points = (MXtPoint*)UUrMemory_Block_NewClear(points_size);
	
	for (i = 0; i < geometry->pointArray->numPoints; i++)
	{
		points[i].point		= geometry->pointArray->points[i];
		points[i].normal	= geometry->vertexNormalArray->vectors[i];
		points[i].uv		= geometry->texCoordArray->textureCoords[i];
		
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
	tri_faces_size = sizeof(MXtFace) * geometry->triNormalArray->numVectors;
	tri_faces = (MXtFace*)UUrMemory_Block_NewClear(tri_faces_size);

	curIndexPtr = geometry->triStripArray->indices;
	
	for (i = 0; i < geometry->triNormalArray->numVectors; i++)
	{
		index2 = *curIndexPtr++;
		
		if (index2 & 0x8000)
		{
			index0 = (index2 & 0x7FFF);
			index1 = *curIndexPtr++;
			index2 = *curIndexPtr++;
		}
		
		tri_faces[i].indicies[0]	= index0;
		tri_faces[i].indicies[1]	= index1;
		tri_faces[i].indicies[2]	= index2;
		tri_faces[i].indicies[3]	= 0;		
		tri_faces[i].normal			= geometry->triNormalArray->vectors[i];
		tri_faces[i].material		= 0;
		tri_faces[i].pad			= 0;

#if (UUmEndian == UUmEndian_Big)
		UUrSwap_2Byte(&tri_faces[i].indicies[0]);
		UUrSwap_2Byte(&tri_faces[i].indicies[1]);
		UUrSwap_2Byte(&tri_faces[i].indicies[2]);
		UUrSwap_2Byte(&tri_faces[i].indicies[3]);
		UUrSwap_4Byte(&tri_faces[i].normal.x);
		UUrSwap_4Byte(&tri_faces[i].normal.y);
		UUrSwap_4Byte(&tri_faces[i].normal.z);
		UUrSwap_2Byte(&tri_faces[i].material);
#endif

		index0 = index1;
		index1 = index2;
	}
	
	// ------------------------------
	// make the materials
	materials_size = sizeof(MXtMaterial);
	materials = (MXtMaterial*)UUrMemory_Block_NewClear(materials_size);
	
	UUrString_Copy(materials->name, TMrInstance_GetInstanceName(geometry->baseMap), MXcMaxName);
	materials->alpha = 1.0f;
	
	for (i = 0; i < MXcMapping_Count; i++)
	{
		UUrString_Copy(materials->maps[i].name, "<none>", MXcMaxName);
	}
	
	UUrString_Copy(materials->maps[1].name, TMrInstance_GetInstanceName(geometry->baseMap), MXcMaxName);
	materials->maps[1].amount = 1.0f;
	
#if (UUmEndian == UUmEndian_Big)
	UUrSwap_4Byte(&materials->alpha);
#endif
	
	// ------------------------------
	// write user data
	
	// ------------------------------
	// write points
	error = BFrFile_Write(inFile, points_size, points);
	UUmError_ReturnOnError(error);
	
	// ------------------------------
	// write triangles
	error = BFrFile_Write(inFile, tri_faces_size, tri_faces);
	UUmError_ReturnOnError(error);
	
	// ------------------------------
	// write quads
	
	// ------------------------------
	// write markers
	
	// ------------------------------
	// write materials
	error = BFrFile_Write(inFile, materials_size, materials);
	UUmError_ReturnOnError(error);
	
	// ------------------------------
	// free the memory
	UUrMemory_Block_Delete(points);
	UUrMemory_Block_Delete(tri_faces);
	UUrMemory_Block_Delete(materials);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrENVFile_Write(
	void)
{
	UUtError				error;
	UUtUns16				level_num;
	BFtFileRef				*env_file_ref;
	BFtFile					*env_file;
	char					filename[128];
	MXtHeader				*header;
	UUtUns32				i;
	
	// get the current level number
	level_num = ONrLevel_GetCurrentLevel();
	if (level_num == 0) { return UUcError_Generic; }
	
	// create the env file ref
	sprintf(filename, "Level%d_Objects.ENV", level_num);
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
	
	// create the MXtHeader
	header = (MXtHeader*)UUrMemory_Block_NewClear(sizeof(MXtHeader));
	UUmError_ReturnOnNull(header);
	
	UUrString_Copy(header->stringENVF, "ENVF", 5);
	header->version = 0;
	header->numNodes = OBJiENVFile_GetNumGunkObjects();
	header->time = 0;
	header->nodes = NULL;
	
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
	
	// create the MXtNodes
	for (i = 0; i < OBJiObjectList_GetNumObjects(); i++)
	{
		OBJtObject			*object;
		
		object = OBJiObjectList_GetObject(i);
		if ((object == NULL) || (object->object_type != OBJcType_Furniture)) { continue; }
		
		error = OBJiENVFile_WriteNode(env_file, object);
		if (error != UUcError_None) { break; }
	}
	
	// close the file
	BFrFile_Close(env_file);
	env_file = NULL;
	
	// delete the file ref
	BFrFileRef_Dispose(env_file_ref);
	env_file_ref = NULL;
	
	return UUcError_None;
}