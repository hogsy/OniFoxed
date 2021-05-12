// ======================================================================
// OT_Neutral.c
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
OBJiNeutral_Delete(
	OBJtObject				*inObject)
{
}

// ----------------------------------------------------------------------
static void
OBJiNeutral_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
	return;
}

// ----------------------------------------------------------------------
static UUtError
OBJiNeutral_Enumerate(
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
OBJiNeutral_GetBoundingSphere(
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
OBJiNeutral_OSDGetName(
	const OBJtOSD_All		*inOSD,
	char					*outName,
	UUtUns32				inNameLength)
{
	const OBJtOSD_Neutral		*neutral_osd;

	// get a pointer to the object osd
	neutral_osd = &inOSD->osd.neutral_osd;
	
	UUrString_Copy(outName, neutral_osd->name, inNameLength);
	
	return;
}

static void
OBJiNeutral_OSDSetName(
	OBJtOSD_All				*inOSD,
	const char				*outName)
{
	OBJtOSD_Neutral		*neutral_osd;

	// get a pointer to the object osd
	neutral_osd = &inOSD->osd.neutral_osd;
	
	UUrString_Copy(neutral_osd->name, outName, sizeof(neutral_osd->name));

	return;
}



// ----------------------------------------------------------------------
static void
OBJiNeutral_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	OBJtOSD_Neutral		*neutral_osd;

	// get a pointer to the object osd
	neutral_osd = (OBJtOSD_Neutral *) inObject->object_data;
	
	outOSD->osd.neutral_osd = *neutral_osd;

	return;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiNeutral_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	UUtUns32 neutral_size, itr;
	OBJtOSD_Neutral *neutral_osd = (OBJtOSD_Neutral *) inObject->object_data;

	neutral_size = 0;
	neutral_size += sizeof(neutral_osd->name);

	neutral_size += sizeof(neutral_osd->id);
	neutral_size += sizeof(neutral_osd->num_lines);
	neutral_size += sizeof(neutral_osd->flags);

	neutral_size += sizeof(neutral_osd->trigger_range);
	neutral_size += sizeof(neutral_osd->conversation_range);
	neutral_size += sizeof(neutral_osd->follow_range);
	neutral_size += sizeof(neutral_osd->abort_enemy_range);
	neutral_size += sizeof(neutral_osd->trigger_sound);
	neutral_size += sizeof(neutral_osd->abort_sound);
	neutral_size += sizeof(neutral_osd->enemy_sound);
	neutral_size += sizeof(neutral_osd->end_script);

	neutral_size += sizeof(neutral_osd->give_weaponclass);
	neutral_size += sizeof(neutral_osd->give_ballistic_ammo);
	neutral_size += sizeof(neutral_osd->give_energy_ammo);
	neutral_size += sizeof(neutral_osd->give_hypos);
	neutral_size += sizeof(neutral_osd->give_flags);

	for (itr = 0; itr < neutral_osd->num_lines; itr++) {
		neutral_size += sizeof(neutral_osd->lines[itr].flags);
		neutral_size += 2;	// padding
		neutral_size += sizeof(neutral_osd->lines[itr].anim_type);
		neutral_size += sizeof(neutral_osd->lines[itr].other_anim_type);
		neutral_size += sizeof(neutral_osd->lines[itr].sound);
	}

	return neutral_size;
}

// ----------------------------------------------------------------------
static void
OBJiNeutral_DuplicateOSD(
	const OBJtOSD_All *inOSD,
	OBJtOSD_All *outOSD)
{
	UUtUns32				itr;
	OBJtObject				*prev_object;

	UUrMemory_MoveFast(&inOSD->osd.neutral_osd, &outOSD->osd.neutral_osd, sizeof(OBJtOSD_Neutral));

	// find a name that isn't a duplicate
	for (itr = 0; itr < UUcMaxUns16; itr++)
	{
		if (itr == 0) {
			// try input name
		} else if (itr == 1) {
			sprintf(outOSD->osd.neutral_osd.name, "%s_copy", inOSD->osd.neutral_osd.name);
		} else {
			sprintf(outOSD->osd.neutral_osd.name, "%s_copy%d", inOSD->osd.neutral_osd.name, itr);
		}

		prev_object = OBJrObjectType_Search(OBJcType_Neutral, OBJcSearch_NeutralName, outOSD);
		if (prev_object != NULL) { continue; }

		break;
	}
	UUmAssert(itr < UUcMaxUns16);

	// find a new ID
	for (itr = 0; itr < 1000; itr++)
	{
		outOSD->osd.neutral_osd.id = inOSD->osd.neutral_osd.id + (UUtUns16) itr;

		prev_object = OBJrObjectType_Search(OBJcType_Neutral, OBJcSearch_NeutralID, outOSD);
		if (prev_object != NULL) { continue; }

		break;
	}
	UUmAssert(itr < 1000);

	return;
}

// ----------------------------------------------------------------------
static void
OBJiNeutral_GetUniqueOSD(
	OBJtOSD_All *inOSD)
{
	UUtUns32				itr;
	OBJtObject				*prev_object;

	UUrMemory_Clear(&inOSD->osd.neutral_osd, sizeof(OBJtOSD_Neutral));

	// NB: ID #0 is reserved, don't use it
	for (itr = 1; itr < UUcMaxUns16; itr++)
	{
		sprintf(inOSD->osd.neutral_osd.name, "neutral_%02d", itr);
		inOSD->osd.neutral_osd.id = (UUtUns16) itr;

		prev_object = OBJrObjectType_Search(OBJcType_Neutral, OBJcSearch_NeutralName, inOSD);
		if (prev_object != NULL) { continue; }

		prev_object = OBJrObjectType_Search(OBJcType_Neutral, OBJcSearch_NeutralID, inOSD);
		if (prev_object != NULL) { continue; }

		break;
	}

	return;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiNeutral_IntersectsLine(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inStartPoint,
	const M3tPoint3D		*inEndPoint)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static UUtError
OBJiNeutral_SetDefaults(
	OBJtOSD_All			*outOSD)
{
	OBJiNeutral_GetUniqueOSD(outOSD);

	outOSD->osd.neutral_osd.num_lines = 1;
	outOSD->osd.neutral_osd.lines[0].flags = 0;
	outOSD->osd.neutral_osd.lines[0].anim_type = ONcAnimType_Act_Talk;
	outOSD->osd.neutral_osd.lines[0].other_anim_type = ONcAnimType_None;
	UUrString_Copy(outOSD->osd.neutral_osd.lines[0].sound, "talking", sizeof(outOSD->osd.neutral_osd.lines[0].sound));

	outOSD->osd.neutral_osd.flags = 0;
	outOSD->osd.neutral_osd.trigger_range		= 100.0f;
	outOSD->osd.neutral_osd.conversation_range	= 40.0f;
	outOSD->osd.neutral_osd.follow_range		= 200.0f;
	outOSD->osd.neutral_osd.abort_enemy_range	= 70.0f;

	UUrString_Copy(outOSD->osd.neutral_osd.trigger_sound, "hey_konoko", sizeof(outOSD->osd.neutral_osd.trigger_sound));
	UUrString_Copy(outOSD->osd.neutral_osd.abort_sound, "see_you_later", sizeof(outOSD->osd.neutral_osd.abort_sound));
	UUrString_Copy(outOSD->osd.neutral_osd.enemy_sound, "what_the", sizeof(outOSD->osd.neutral_osd.enemy_sound));
	outOSD->osd.neutral_osd.end_script[0] = '\0';

	outOSD->osd.neutral_osd.give_weaponclass[0] = '\0';
	outOSD->osd.neutral_osd.give_ballistic_ammo = 0;
	outOSD->osd.neutral_osd.give_energy_ammo = 0;
	outOSD->osd.neutral_osd.give_hypos = 0;
	outOSD->osd.neutral_osd.give_flags = 0;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiNeutral_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_All				osd_all;
	UUtError				error;
	
	if (NULL == inOSD) {
		error = OBJiNeutral_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);
	}
	else
	{
		OBJiNeutral_DuplicateOSD(inOSD, &osd_all);
	}
	
	// set the object specific data and position
	error = OBJrObject_SetObjectSpecificData(inObject, &osd_all);
	UUmError_ReturnOnError(error);
	
	OBJrObject_UpdatePosition(inObject);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiNeutral_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_Neutral			*neutral_osd = (OBJtOSD_Neutral *) inObject->object_data;
	UUtUns32				itr;
	UUtUns32				bytes_read = 0;

	bytes_read += OBJmGetStringFromBuffer(inBuffer, neutral_osd->name,					sizeof(neutral_osd->name), inSwapIt);

	bytes_read += OBJmGet2BytesFromBuffer(inBuffer, neutral_osd->id,					UUtUns16,					inSwapIt);
	bytes_read += OBJmGet2BytesFromBuffer(inBuffer, neutral_osd->num_lines,				UUtUns16,					inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, neutral_osd->flags,					UUtUns32,					inSwapIt);

	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, neutral_osd->trigger_range,			float,						inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, neutral_osd->conversation_range,	float,						inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, neutral_osd->follow_range,			float,						inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, neutral_osd->abort_enemy_range,		float,						inSwapIt);
	bytes_read += OBJmGetStringFromBuffer(inBuffer, neutral_osd->trigger_sound,			sizeof(neutral_osd->trigger_sound), inSwapIt);
	bytes_read += OBJmGetStringFromBuffer(inBuffer, neutral_osd->abort_sound,			sizeof(neutral_osd->abort_sound), inSwapIt);
	if (inVersion >= OBJcVersion_25) {
		bytes_read += OBJmGetStringFromBuffer(inBuffer, neutral_osd->enemy_sound,		sizeof(neutral_osd->enemy_sound), inSwapIt);
	} else {
		UUrString_Copy(neutral_osd->enemy_sound, "what_the", sizeof(neutral_osd->enemy_sound));
	}
	bytes_read += OBJmGetStringFromBuffer(inBuffer, neutral_osd->end_script,			sizeof(neutral_osd->end_script), inSwapIt);

	bytes_read += OBJmGetStringFromBuffer(inBuffer, neutral_osd->give_weaponclass,		sizeof(neutral_osd->give_weaponclass), inSwapIt);
	bytes_read += OBJmGetByteFromBuffer  (inBuffer, neutral_osd->give_ballistic_ammo,	UUtUns8,					inSwapIt);
	bytes_read += OBJmGetByteFromBuffer  (inBuffer, neutral_osd->give_energy_ammo,		UUtUns8,					inSwapIt);
	bytes_read += OBJmGetByteFromBuffer  (inBuffer, neutral_osd->give_hypos,			UUtUns8,					inSwapIt);
	bytes_read += OBJmGetByteFromBuffer  (inBuffer, neutral_osd->give_flags,			UUtUns8,					inSwapIt);

	for(itr = 0; itr < neutral_osd->num_lines; itr++) {
		bytes_read += OBJmGet2BytesFromBuffer(inBuffer, neutral_osd->lines[itr].flags,		UUtUns16, inSwapIt);
		if (inVersion >= OBJcVersion_24) {
			OBJmSkip2BytesFromBuffer(inBuffer);
		}
		bytes_read += OBJmGet2BytesFromBuffer(inBuffer, neutral_osd->lines[itr].anim_type,	UUtUns16, inSwapIt);
		if (inVersion >= OBJcVersion_24) {
			bytes_read += OBJmGet2BytesFromBuffer(inBuffer, neutral_osd->lines[itr].other_anim_type,	UUtUns16, inSwapIt);
		}
		bytes_read += OBJmGetStringFromBuffer(inBuffer, neutral_osd->lines[itr].sound,		sizeof(neutral_osd->lines[itr].sound), inSwapIt);
	}

	// bring the object up to date
	OBJrObject_UpdatePosition(inObject);
	
	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtError
OBJiNeutral_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_Neutral			*neutral_osd;
	UUtUns32				itr;
	
	UUmAssert(inOSD);
	
	// get a pointer to the object osd
	neutral_osd = (OBJtOSD_Neutral *) inObject->object_data;
	
	// copy the data from inOSD to char_osd
	UUrString_Copy(neutral_osd->name, inOSD->osd.neutral_osd.name, sizeof(inOSD->osd.neutral_osd.name));

	neutral_osd->id					= inOSD->osd.neutral_osd.id;
	neutral_osd->num_lines			= inOSD->osd.neutral_osd.num_lines;
	neutral_osd->flags				= inOSD->osd.neutral_osd.flags;

	neutral_osd->trigger_range		= inOSD->osd.neutral_osd.trigger_range;
	neutral_osd->conversation_range	= inOSD->osd.neutral_osd.conversation_range;
	neutral_osd->follow_range		= inOSD->osd.neutral_osd.follow_range;
	neutral_osd->abort_enemy_range	= inOSD->osd.neutral_osd.abort_enemy_range;
	UUrString_Copy(neutral_osd->trigger_sound, inOSD->osd.neutral_osd.trigger_sound, sizeof(inOSD->osd.neutral_osd.trigger_sound));
	UUrString_Copy(neutral_osd->abort_sound, inOSD->osd.neutral_osd.abort_sound, sizeof(inOSD->osd.neutral_osd.abort_sound));
	UUrString_Copy(neutral_osd->enemy_sound, inOSD->osd.neutral_osd.enemy_sound, sizeof(inOSD->osd.neutral_osd.enemy_sound));
	UUrString_Copy(neutral_osd->end_script, inOSD->osd.neutral_osd.end_script, sizeof(inOSD->osd.neutral_osd.end_script));

	UUrString_Copy(neutral_osd->give_weaponclass, inOSD->osd.neutral_osd.give_weaponclass, sizeof(inOSD->osd.neutral_osd.give_weaponclass));
	neutral_osd->give_ballistic_ammo= inOSD->osd.neutral_osd.give_ballistic_ammo;
	neutral_osd->give_energy_ammo	= inOSD->osd.neutral_osd.give_energy_ammo;
	neutral_osd->give_hypos			= inOSD->osd.neutral_osd.give_hypos;
	neutral_osd->give_flags			= inOSD->osd.neutral_osd.give_flags;

	for (itr = 0; itr < inOSD->osd.neutral_osd.num_lines; itr++) {
		neutral_osd->lines[itr].flags			= inOSD->osd.neutral_osd.lines[itr].flags;
		neutral_osd->lines[itr].anim_type		= inOSD->osd.neutral_osd.lines[itr].anim_type;
		neutral_osd->lines[itr].other_anim_type	= inOSD->osd.neutral_osd.lines[itr].other_anim_type;
		UUrString_Copy(neutral_osd->lines[itr].sound, inOSD->osd.neutral_osd.lines[itr].sound,
						sizeof(inOSD->osd.neutral_osd.lines[itr].sound));
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiNeutral_UpdatePosition(
	OBJtObject				*inObject)
{
	// nothing
}

// ----------------------------------------------------------------------
static UUtError
OBJiNeutral_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	OBJtOSD_Neutral		*neutral_osd;
	UUtUns32			itr, bytes_available;
	
	neutral_osd = (OBJtOSD_Neutral *) inObject->object_data;
	bytes_available = *ioBufferSize;

	OBJmWriteStringToBuffer(ioBuffer, neutral_osd->name,				sizeof(neutral_osd->name),	bytes_available, OBJcWrite_Little);

	OBJmWrite2BytesToBuffer(ioBuffer, neutral_osd->id,					UUtUns16,					bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, neutral_osd->num_lines,			UUtUns16,					bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, neutral_osd->flags,				UUtUns32,					bytes_available, OBJcWrite_Little);

	OBJmWrite4BytesToBuffer(ioBuffer, neutral_osd->trigger_range,		float,						bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, neutral_osd->conversation_range,	float,						bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, neutral_osd->follow_range,		float,						bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, neutral_osd->abort_enemy_range,	float,						bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, neutral_osd->trigger_sound,		sizeof(neutral_osd->trigger_sound),	bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, neutral_osd->abort_sound,			sizeof(neutral_osd->abort_sound),	bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, neutral_osd->enemy_sound,			sizeof(neutral_osd->enemy_sound),	bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, neutral_osd->end_script,			sizeof(neutral_osd->end_script),	bytes_available, OBJcWrite_Little);

	OBJmWriteStringToBuffer(ioBuffer, neutral_osd->give_weaponclass,	sizeof(neutral_osd->give_weaponclass),	bytes_available, OBJcWrite_Little);
	OBJmWriteByteToBuffer  (ioBuffer, neutral_osd->give_ballistic_ammo, UUtUns8,					bytes_available, OBJcWrite_Little);
	OBJmWriteByteToBuffer  (ioBuffer, neutral_osd->give_energy_ammo,	UUtUns8,					bytes_available, OBJcWrite_Little);
	OBJmWriteByteToBuffer  (ioBuffer, neutral_osd->give_hypos,			UUtUns8,					bytes_available, OBJcWrite_Little);
	OBJmWriteByteToBuffer  (ioBuffer, neutral_osd->give_flags,			UUtUns8,					bytes_available, OBJcWrite_Little);

	for(itr = 0; itr < neutral_osd->num_lines; itr++) {
		OBJmWrite2BytesToBuffer(ioBuffer, neutral_osd->lines[itr].flags,			UUtUns16,		bytes_available, OBJcWrite_Little);
		OBJmWrite2BytesToBuffer(ioBuffer, 0,										UUtUns16,		bytes_available, OBJcWrite_Little);		// padding
		OBJmWrite2BytesToBuffer(ioBuffer, neutral_osd->lines[itr].anim_type,		UUtUns16,		bytes_available, OBJcWrite_Little);
		OBJmWrite2BytesToBuffer(ioBuffer, neutral_osd->lines[itr].other_anim_type,	UUtUns16,		bytes_available, OBJcWrite_Little);
		OBJmWriteStringToBuffer(ioBuffer, neutral_osd->lines[itr].sound,			sizeof(neutral_osd->lines[itr].sound),	bytes_available, OBJcWrite_Little);
	}

	// set ioBufferSize to the number of bytes written to the buffer
	*ioBufferSize = *ioBufferSize - bytes_available;
	
	return UUcError_None;
}


// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
OBJiNeutral_GetVisible(
	void)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static void
OBJiNeutral_SetVisible(
	UUtBool					inIsVisible)
{
	return;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiNeutral_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	OBJtOSD_Neutral		*neutral_osd;
	UUtBool				found;
	
	// get a pointer to the object osd
	neutral_osd = (OBJtOSD_Neutral *) inObject->object_data;
	
	// perform the check
	switch (inSearchType)
	{
		case OBJcSearch_NeutralName:
			found = UUmString_IsEqual(neutral_osd->name, inSearchParams->osd.neutral_osd.name);
		break;

		case OBJcSearch_NeutralID:
			found = (neutral_osd->id == inSearchParams->osd.neutral_osd.id);
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
OBJrNeutral_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;
	
	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));
	
	// set up the methods structure
	methods.rNew				= OBJiNeutral_New;
	methods.rSetDefaults		= OBJiNeutral_SetDefaults;
	methods.rDelete				= OBJiNeutral_Delete;
	methods.rDraw				= OBJiNeutral_Draw;
	methods.rEnumerate			= OBJiNeutral_Enumerate;
	methods.rGetBoundingSphere	= OBJiNeutral_GetBoundingSphere;
	methods.rOSDGetName			= OBJiNeutral_OSDGetName;
	methods.rOSDSetName			= OBJiNeutral_OSDSetName;
	methods.rIntersectsLine		= OBJiNeutral_IntersectsLine;
	methods.rUpdatePosition		= OBJiNeutral_UpdatePosition;
	methods.rGetOSD				= OBJiNeutral_GetOSD;
	methods.rGetOSDWriteSize	= OBJiNeutral_GetOSDWriteSize;
	methods.rSetOSD				= OBJiNeutral_SetOSD;
	methods.rRead				= OBJiNeutral_Read;
	methods.rWrite				= OBJiNeutral_Write;
	methods.rGetClassVisible	= OBJiNeutral_GetVisible;
	methods.rSearch				= OBJiNeutral_Search;
	methods.rSetClassVisible	= OBJiNeutral_SetVisible;
	methods.rGetUniqueOSD		= OBJiNeutral_GetUniqueOSD;
	
	// register the neutral behavior methods
	error =
		OBJrObjectGroup_Register(
			OBJcType_Neutral,
			OBJcTypeIndex_Neutral,
			"Neutral",
			sizeof(OBJtOSD_Neutral),
			&methods,
			OBJcObjectGroupFlag_CanSetName);
	UUmError_ReturnOnError(error);
		
	return UUcError_None;
}

