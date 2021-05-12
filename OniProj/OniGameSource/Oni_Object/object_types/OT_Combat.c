// ======================================================================
// OT_Combat.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ScriptLang.h"
#include "BFW_TextSystem.h"

#include "Oni_Object.h"
#include "Oni_Character.h"
#include "Oni_Object_Private.h"

#include "Oni_AI2.h"

#include <ctype.h>
#include "EulerAngles.h"

// ======================================================================
// defines
// ======================================================================

// ======================================================================
// globals
// ======================================================================

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiCombat_Delete(
	OBJtObject				*inObject)
{
}

// ----------------------------------------------------------------------
static void
OBJiCombat_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
	return;
}

// ----------------------------------------------------------------------
static UUtError
OBJiCombat_Enumerate(
	OBJtObject						*inObject,
	OBJtEnumCallback_ObjectName		inEnumCallback,
	UUtUns32						inUserData)
{
	char							name[OBJcMaxNameLength + 1];
	
	OBJrObject_GetName(inObject, name, OBJcMaxNameLength);
	inEnumCallback(name, inUserData);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiCombat_GetBoundingSphere(
	const OBJtObject		*inObject,
	M3tBoundingSphere		*outBoundingSphere)
{
	outBoundingSphere->center.x = 0.0f;
	outBoundingSphere->center.y = 0.0f;
	outBoundingSphere->center.z = 0.0f;
	outBoundingSphere->radius = 0.f;

	return;
}

// ----------------------------------------------------------------------
static void
OBJiCombat_OSDGetName(
	const OBJtOSD_All		*inOSD,
	char					*outName,
	UUtUns32				inNameLength)
{
	const OBJtOSD_Combat		*com_osd;

	// get a pointer to the object osd
	com_osd = &inOSD->osd.combat_osd;
	
	UUrString_Copy(outName, com_osd->name, inNameLength);
	
	return;
}

static void
OBJiCombat_OSDSetName(
	OBJtOSD_All				*inOSD,
	const char				*outName)
{
	OBJtOSD_Combat		*com_osd;

	// get a pointer to the object osd
	com_osd = &inOSD->osd.combat_osd;
	
	UUrString_Copy(com_osd->name, outName, sizeof(com_osd->name));

	return;
}



// ----------------------------------------------------------------------
static void
OBJiCombat_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	OBJtOSD_Combat		*com_osd;

	// get a pointer to the object osd
	com_osd = (OBJtOSD_Combat *)inObject->object_data;
	
	outOSD->osd.combat_osd = *com_osd;

	return;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiCombat_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	UUtUns32 combat_size;
	OBJtOSD_All osd;

	osd;
	
	combat_size = 0;
	combat_size += sizeof(osd.osd.combat_osd.name);
	combat_size += sizeof(osd.osd.combat_osd.id);
	combat_size += sizeof(osd.osd.combat_osd.flags);
	combat_size += sizeof(osd.osd.combat_osd.medium_range);
	combat_size += sizeof(osd.osd.combat_osd.behavior);
	combat_size += sizeof(osd.osd.combat_osd.melee_when);
	combat_size += sizeof(osd.osd.combat_osd.no_gun);
	combat_size += sizeof(osd.osd.combat_osd.short_range);
	combat_size += sizeof(osd.osd.combat_osd.pursuit_distance);
	combat_size += sizeof(osd.osd.combat_osd.panic_melee);
	combat_size += sizeof(osd.osd.combat_osd.panic_gunfire);
	combat_size += sizeof(osd.osd.combat_osd.panic_sight);
	combat_size += sizeof(osd.osd.combat_osd.panic_hurt);
	combat_size += sizeof(osd.osd.combat_osd.alarm_find_distance);
	combat_size += sizeof(osd.osd.combat_osd.alarm_ignore_enemy_dist);
	combat_size += sizeof(osd.osd.combat_osd.alarm_chase_enemy_dist);
	combat_size += sizeof(osd.osd.combat_osd.alarm_damage_threshold);
	combat_size += sizeof(osd.osd.combat_osd.alarm_fight_timer);
	return combat_size;
}

// ----------------------------------------------------------------------
static void
OBJiCombat_DuplicateOSD(
	const OBJtOSD_All *inOSD,
	OBJtOSD_All *outOSD)
{
	UUtUns32				itr;
	OBJtObject				*prev_object;

	UUrMemory_MoveFast(&inOSD->osd.combat_osd, &outOSD->osd.combat_osd, sizeof(OBJtOSD_Combat));

	// find a name that isn't a duplicate
	for (itr = 0; itr < UUcMaxUns16; itr++)
	{
		if (itr == 0) {
			// try input name
		} else if (itr == 1) {
			sprintf(outOSD->osd.combat_osd.name, "%s_copy", inOSD->osd.combat_osd.name);
		} else {
			sprintf(outOSD->osd.combat_osd.name, "%s_copy%d", inOSD->osd.combat_osd.name, itr);
		}

		prev_object = OBJrObjectType_Search(OBJcType_Combat, OBJcSearch_CombatName, outOSD);
		if (prev_object != NULL) { continue; }

		break;
	}
	UUmAssert(itr < UUcMaxUns16);

	// find a new ID
	for (itr = 0; itr < 1000; itr++)
	{
		outOSD->osd.combat_osd.id = inOSD->osd.combat_osd.id + (UUtUns16) itr;

		prev_object = OBJrObjectType_Search(OBJcType_Combat, OBJcSearch_CombatID, outOSD);
		if (prev_object != NULL) { continue; }

		break;
	}
	UUmAssert(itr < 1000);

	return;
}

// ----------------------------------------------------------------------
static void
OBJiCombat_GetUniqueOSD(
	OBJtOSD_All *inOSD)
{
	UUtUns32				itr;
	OBJtObject				*prev_object;

	UUrMemory_Clear(&inOSD->osd.combat_osd, sizeof(OBJtOSD_Combat));

	for (itr = 0; itr < UUcMaxUns16; itr++)
	{
		sprintf(inOSD->osd.combat_osd.name, "combat_%02d", itr);
		inOSD->osd.combat_osd.id = (UUtUns16) itr;

		prev_object = OBJrObjectType_Search(OBJcType_Combat, OBJcSearch_CombatName, inOSD);
		if (prev_object != NULL) { continue; }

		prev_object = OBJrObjectType_Search(OBJcType_Combat, OBJcSearch_CombatID, inOSD);
		if (prev_object != NULL) { continue; }

		break;
	}

	return;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiCombat_IntersectsLine(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inStartPoint,
	const M3tPoint3D		*inEndPoint)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtError
OBJiCombat_SetDefaults(
	OBJtOSD_All			*outOSD)
{
	UUtUns32				itr;
	
	OBJiCombat_GetUniqueOSD(outOSD);

	outOSD->osd.combat_osd.flags = 0;
	outOSD->osd.combat_osd.medium_range = AI2cCombatSettings_DefaultMediumRange;
	for (itr = 0; itr < 5; itr++) {
		outOSD->osd.combat_osd.behavior[itr] = AI2cCombatSettings_DefaultBehavior;
	}
	outOSD->osd.combat_osd.melee_when = AI2cCombatSettings_DefaultMeleeWhen;
	outOSD->osd.combat_osd.no_gun = AI2cCombatSettings_DefaultNoGun;
	outOSD->osd.combat_osd.short_range = AI2cCombatSettings_DefaultShortRange;
	outOSD->osd.combat_osd.pursuit_distance = AI2cCombatSettings_DefaultPursuitDistance;

	outOSD->osd.combat_osd.panic_melee = AI2cPanic_DefaultMelee;
	outOSD->osd.combat_osd.panic_gunfire = AI2cPanic_DefaultGunfire;
	outOSD->osd.combat_osd.panic_sight = AI2cPanic_DefaultSight;
	outOSD->osd.combat_osd.panic_hurt = AI2cPanic_DefaultHurt;

	outOSD->osd.combat_osd.alarm_find_distance = AI2cAlarmSettings_DefaultFindDistance;
	outOSD->osd.combat_osd.alarm_ignore_enemy_dist = AI2cAlarmSettings_DefaultIgnoreEnemy;
	outOSD->osd.combat_osd.alarm_chase_enemy_dist = AI2cAlarmSettings_DefaultChaseEnemy;
	outOSD->osd.combat_osd.alarm_damage_threshold = AI2cAlarmSettings_DefaultDamageThreshold;
	outOSD->osd.combat_osd.alarm_fight_timer = AI2cAlarmSettings_DefaultFightTimer;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiCombat_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_All				osd_all;
	UUtError				error;
	
	if (NULL == inOSD) {
		error = OBJiCombat_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);
	}
	else
	{
		OBJiCombat_DuplicateOSD(inOSD, &osd_all);
	}
	
	// set the object specific data and position
	error = OBJrObject_SetObjectSpecificData(inObject, &osd_all);
	UUmError_ReturnOnError(error);
	
	OBJrObject_UpdatePosition(inObject);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiCombat_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_Combat			*com_osd = (OBJtOSD_Combat *) inObject->object_data;
	UUtUns32				itr;
	UUtUns32				bytes_read = 0;
	
	bytes_read += OBJmGetStringFromBuffer(inBuffer, com_osd->name, sizeof(com_osd->name), inSwapIt);
	bytes_read += OBJmGet2BytesFromBuffer(inBuffer, com_osd->id, UUtUns16, inSwapIt);

	if (inVersion >= OBJcVersion_36) {
		bytes_read += OBJmGet2BytesFromBuffer(inBuffer, com_osd->flags, UUtUns16, inSwapIt);
	} else {
		com_osd->flags = 0;
	}

	for(itr = 0; itr < 5; itr++) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->behavior[itr], UUtUns32, inSwapIt);
	}

	if (inVersion >= OBJcVersion_10) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->medium_range, float, inSwapIt);
	} else {
		com_osd->medium_range = AI2cCombatSettings_DefaultMediumRange;
	}

	if (inVersion >= OBJcVersion_11) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->melee_when,	UUtUns32, inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->no_gun,		UUtUns32, inSwapIt);
	} else {
		com_osd->melee_when = AI2cCombatSettings_DefaultMeleeWhen;
		com_osd->no_gun = AI2cCombatSettings_DefaultNoGun;
	}

	if (inVersion >= OBJcVersion_15) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->short_range, float, inSwapIt);
	} else {
		com_osd->short_range = AI2cCombatSettings_DefaultShortRange;
	}

	if (inVersion >= OBJcVersion_22) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->pursuit_distance, float, inSwapIt);
	} else {
		com_osd->pursuit_distance = AI2cCombatSettings_DefaultPursuitDistance;
	}

	if (inVersion >= OBJcVersion_39) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->panic_melee, UUtUns32, inSwapIt);
	} else {
		com_osd->panic_melee = AI2cPanic_DefaultMelee;
	}
	if (inVersion >= OBJcVersion_28) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->panic_gunfire, UUtUns32, inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->panic_sight, UUtUns32, inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->panic_hurt, UUtUns32, inSwapIt);
	} else {
		com_osd->panic_gunfire = AI2cPanic_DefaultGunfire;
		com_osd->panic_sight = AI2cPanic_DefaultSight;
		com_osd->panic_hurt = AI2cPanic_DefaultHurt;
	}

	if (inVersion >= OBJcVersion_35) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->alarm_find_distance, float, inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->alarm_ignore_enemy_dist, float, inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->alarm_chase_enemy_dist, float, inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->alarm_damage_threshold, UUtUns32, inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, com_osd->alarm_fight_timer, UUtUns32, inSwapIt);
	} else {
		com_osd->alarm_find_distance = AI2cAlarmSettings_DefaultFindDistance;
		com_osd->alarm_ignore_enemy_dist = AI2cAlarmSettings_DefaultIgnoreEnemy;
		com_osd->alarm_chase_enemy_dist = AI2cAlarmSettings_DefaultChaseEnemy;
		com_osd->alarm_damage_threshold = AI2cAlarmSettings_DefaultDamageThreshold;
		com_osd->alarm_fight_timer = AI2cAlarmSettings_DefaultFightTimer;
	}

	// bring the object up to date
	OBJrObject_UpdatePosition(inObject);
	
	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtError
OBJiCombat_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_Combat			*com_osd;
	UUtUns32				itr;
	
	UUmAssert(inOSD);
	
	// get a pointer to the object osd
	com_osd = (OBJtOSD_Combat *)inObject->object_data;
	
	// copy the data from inOSD to char_osd
	UUrString_Copy(com_osd->name, inOSD->osd.combat_osd.name, sizeof(inOSD->osd.combat_osd.name));

	com_osd->id = inOSD->osd.combat_osd.id;
	com_osd->flags = inOSD->osd.combat_osd.flags;

	for (itr = 0; itr < 5; itr++) {
		com_osd->behavior[itr] = inOSD->osd.combat_osd.behavior[itr];
	}

	com_osd->medium_range = inOSD->osd.combat_osd.medium_range;

	com_osd->melee_when = inOSD->osd.combat_osd.melee_when;
	com_osd->no_gun = inOSD->osd.combat_osd.no_gun;

	com_osd->short_range = inOSD->osd.combat_osd.short_range;
	com_osd->pursuit_distance = inOSD->osd.combat_osd.pursuit_distance;
	
	com_osd->panic_melee = inOSD->osd.combat_osd.panic_melee;
	com_osd->panic_gunfire = inOSD->osd.combat_osd.panic_gunfire;
	com_osd->panic_sight = inOSD->osd.combat_osd.panic_sight;
	com_osd->panic_hurt = inOSD->osd.combat_osd.panic_hurt;

	com_osd->alarm_find_distance = inOSD->osd.combat_osd.alarm_find_distance;
	com_osd->alarm_ignore_enemy_dist = inOSD->osd.combat_osd.alarm_ignore_enemy_dist;
	com_osd->alarm_chase_enemy_dist = inOSD->osd.combat_osd.alarm_chase_enemy_dist;
	com_osd->alarm_damage_threshold = inOSD->osd.combat_osd.alarm_damage_threshold;
	com_osd->alarm_fight_timer = inOSD->osd.combat_osd.alarm_fight_timer;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiCombat_UpdatePosition(
	OBJtObject				*inObject)
{
	// nothing
}

// ----------------------------------------------------------------------
static UUtError
OBJiCombat_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	OBJtOSD_Combat		*com_osd;
	UUtUns32			itr, bytes_available;
	
	com_osd = (OBJtOSD_Combat *) inObject->object_data;
	bytes_available = *ioBufferSize;

	OBJmWriteStringToBuffer(ioBuffer, com_osd->name,			sizeof(com_osd->name), bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, com_osd->id,				UUtUns16,	bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, com_osd->flags,			UUtUns16,	bytes_available, OBJcWrite_Little);

	for(itr = 0; itr < 5; itr++) {
		OBJmWrite4BytesToBuffer(ioBuffer, com_osd->behavior[itr], UUtUns32, bytes_available, OBJcWrite_Little);
	}

	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->medium_range,			float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->melee_when,				UUtUns32,	bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->no_gun,					UUtUns32,	bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->short_range,				float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->pursuit_distance,		float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->panic_melee,				UUtUns32,	bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->panic_gunfire,			UUtUns32,	bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->panic_sight,				UUtUns32,	bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->panic_hurt,				UUtUns32,	bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->alarm_find_distance,		float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->alarm_ignore_enemy_dist,	float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->alarm_chase_enemy_dist,	float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->alarm_damage_threshold,	UUtUns32,	bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, com_osd->alarm_fight_timer,		UUtUns32,	bytes_available, OBJcWrite_Little);

	// set ioBufferSize to the number of bytes written to the buffer
	*ioBufferSize = *ioBufferSize - bytes_available;
	
	return UUcError_None;
}


static const char *OBJiCombat_IsInvalid(
	OBJtObject *inObject)
{
	// construct the initial state of the listbox 
	UUtUns32 count;
	UUtUns32 itr;

	OBJtOSD_Combat *in_combat_osd = (OBJtOSD_Combat *)inObject->object_data;
	OBJtObjectType object_type = inObject->object_type;

	count = OBJrObjectType_EnumerateObjects(object_type, NULL, 0);

	for(itr = 0; itr < count; itr++)
	{
		OBJtObject *test_object = OBJrObjectType_GetObject_ByNumber(object_type, itr);

		if (test_object != inObject) {
			OBJtOSD_Combat *test_combat_osd = (OBJtOSD_Combat *)test_object->object_data;
			static const char *duplicate_name = "Duplicate Name";
			static const char *duplicate_id = "Duplicate ID";

			if (UUmString_IsEqual(in_combat_osd->name, test_combat_osd->name)) {
				return duplicate_name;
			}

			if (in_combat_osd->id == test_combat_osd->id) {
				return duplicate_id;
			}
		}
	}

	return NULL;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OBJiCombat_GetVisible(
	void)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static void
OBJiCombat_SetVisible(
	UUtBool					inIsVisible)
{
	return;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiCombat_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	OBJtOSD_Combat		*com_osd;
	UUtBool				found;
	
	// get a pointer to the object osd
	com_osd = (OBJtOSD_Combat *)inObject->object_data;
	
	// perform the check
	switch (inSearchType)
	{
		case OBJcSearch_CombatName:
			found = UUmString_IsEqual(com_osd->name, inSearchParams->osd.combat_osd.name);
		break;

		case OBJcSearch_CombatID:
			found = com_osd->id == inSearchParams->osd.combat_osd.id;
		break;

		default:
			found = UUcFalse;
		break;
	}
	
	return found;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBJrCombat_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;
	
	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));
	
	// set up the methods structure
	methods.rNew				= OBJiCombat_New;
	methods.rSetDefaults		= OBJiCombat_SetDefaults;
	methods.rDelete				= OBJiCombat_Delete;
	methods.rDraw				= OBJiCombat_Draw;
	methods.rEnumerate			= OBJiCombat_Enumerate;
	methods.rGetBoundingSphere	= OBJiCombat_GetBoundingSphere;
	methods.rOSDGetName			= OBJiCombat_OSDGetName;
	methods.rOSDSetName			= OBJiCombat_OSDSetName;
	methods.rIntersectsLine		= OBJiCombat_IntersectsLine;
	methods.rUpdatePosition		= OBJiCombat_UpdatePosition;
	methods.rGetOSD				= OBJiCombat_GetOSD;
	methods.rGetOSDWriteSize	= OBJiCombat_GetOSDWriteSize;
	methods.rSetOSD				= OBJiCombat_SetOSD;
	methods.rRead				= OBJiCombat_Read;
	methods.rWrite				= OBJiCombat_Write;
	methods.rGetClassVisible	= OBJiCombat_GetVisible;
	methods.rSearch				= OBJiCombat_Search;
	methods.rSetClassVisible	= OBJiCombat_SetVisible;
	methods.rGetUniqueOSD		= OBJiCombat_GetUniqueOSD;
	
	// register the furniture methods
	error =
		OBJrObjectGroup_Register(
			OBJcType_Combat,
			OBJcTypeIndex_Combat,
			"Combat",
			sizeof(OBJtOSD_Combat),
			&methods,
			OBJcObjectGroupFlag_CanSetName | OBJcObjectGroupFlag_CommonToAllLevels);
	UUmError_ReturnOnError(error);
		
	return UUcError_None;
}

