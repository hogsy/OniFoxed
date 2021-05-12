// ======================================================================
// OT_Melee.c
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

#include "Oni_AI2_MeleeProfile.h"

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
OBJiMelee_Delete(
	OBJtObject				*inObject)
{
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inObject->object_data;

	if (profile->reference_count > 0) {
		// tell the AI that this profile has been deleted
		AI2rNotifyMeleeProfile(profile, UUcTrue);
	}

	if (profile->technique != NULL)
		UUrMemory_Block_Delete(profile->technique);

	if (profile->move != NULL)
		UUrMemory_Block_Delete(profile->move);
}

// ----------------------------------------------------------------------
static void
OBJiMelee_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
	return;
}

// ----------------------------------------------------------------------
static UUtError
OBJiMelee_Enumerate(
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
OBJiMelee_GetBoundingSphere(
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
OBJiMelee_OSDGetName(
	const OBJtOSD_All		*inOSD,
	char					*outName,
	UUtUns32				inNameLength)
{
	const AI2tMeleeProfile	*profile;

	profile = &inOSD->osd.melee_osd;
	
	UUrString_Copy(outName, profile->name, inNameLength);
	
	return;
}

static void
OBJiMelee_OSDSetName(
	OBJtOSD_All				*inOSD,
	const char				*outName)
{
	AI2tMeleeProfile		*profile;

	profile = &inOSD->osd.melee_osd;
	
	UUrString_Copy(profile->name, outName, sizeof(profile->name));

	return;
}



// ----------------------------------------------------------------------
static void
OBJiMelee_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	AI2tMeleeProfile		*profile;
	UUtUns32				blocksize;

	profile = (AI2tMeleeProfile *) inObject->object_data;
	
	outOSD->osd.melee_osd = *profile;

	// *** allocate and copy the variable-length arrays

	blocksize = (profile->num_actions + profile->num_reactions + profile->num_spacing) * sizeof(AI2tMeleeTechnique);
	if (blocksize == 0) {
		outOSD->osd.melee_osd.technique = NULL;
	} else {
		outOSD->osd.melee_osd.technique = UUrMemory_Block_New(blocksize);
		if (outOSD->osd.melee_osd.technique == NULL) {
			// error - but no way of reporting it! ugh, this is an ugly hack
			outOSD->osd.melee_osd.num_actions = 0;
			outOSD->osd.melee_osd.num_reactions = 0;
			outOSD->osd.melee_osd.num_spacing = 0;
		} else {
			UUrMemory_MoveFast(profile->technique, outOSD->osd.melee_osd.technique, blocksize);
		}
	}

	blocksize = profile->num_moves * sizeof(AI2tMeleeMove);
	if (blocksize == 0) {
		outOSD->osd.melee_osd.move = NULL;
	} else {
		outOSD->osd.melee_osd.move = UUrMemory_Block_New(blocksize);
		if (outOSD->osd.melee_osd.move == NULL) {
			// error - but no way of reporting it! ugh, this is an ugly hack
			outOSD->osd.melee_osd.num_moves = 0;
		} else {
			UUrMemory_MoveFast(profile->move, outOSD->osd.melee_osd.move, blocksize);
		}
	}

	outOSD->osd.melee_osd.reference_count = 0;


	return;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiMelee_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	UUtUns32 profile_size, technique_size, move_size;
	AI2tMeleeProfile *profile = (AI2tMeleeProfile *) inObject->object_data;
	
	technique_size = 0;
	technique_size += sizeof(profile->technique->name);
	technique_size += sizeof(profile->technique->user_flags);
	technique_size += sizeof(profile->technique->base_weight);
	technique_size += sizeof(profile->technique->importance);
	technique_size += sizeof(profile->technique->delay_frames);
	technique_size += sizeof(profile->technique->num_moves);
	technique_size += sizeof(profile->technique->start_index);

	move_size = 0;
	move_size += sizeof(profile->move->move);
	move_size += sizeof(profile->move->param);

	profile_size = 0;
	profile_size += sizeof(profile->id);
	profile_size += sizeof(profile->name);
	profile_size += sizeof(profile->char_classname);

	profile_size += sizeof(profile->attacknotice_percentage);
	profile_size += sizeof(profile->dodge_base_percentage);
	profile_size += sizeof(profile->dodge_additional_percentage);
	profile_size += sizeof(profile->dodge_damage_threshold);
	profile_size += sizeof(profile->blockskill_percentage);
	profile_size += sizeof(profile->blockgroup_percentage);

	profile_size += sizeof(profile->weight_notblocking_any);
	profile_size += sizeof(profile->weight_notblocking_changestance);
	profile_size += sizeof(profile->weight_blocking_unblockable);
	profile_size += sizeof(profile->weight_blocking_stagger);
	profile_size += sizeof(profile->weight_blocking_blockstun);
	profile_size += sizeof(profile->weight_blocking);

	profile_size += sizeof(profile->weight_throwdanger);

	profile_size += sizeof(profile->dazed_minframes);
	profile_size += sizeof(profile->dazed_maxframes);

	profile_size += sizeof(profile->num_actions);
	profile_size += sizeof(profile->num_reactions);
	profile_size += sizeof(profile->num_spacing);
	profile_size += sizeof(profile->num_moves);

	profile_size += technique_size * (profile->num_actions + profile->num_reactions + profile->num_spacing);
	profile_size += move_size * profile->num_moves;

	return profile_size;
}

// ----------------------------------------------------------------------
static void
OBJiMelee_DuplicateOSD(
	const OBJtOSD_All *inOSD,
	OBJtOSD_All *outOSD)
{
	UUtUns32				itr;
	OBJtObject				*prev_object;

	UUrMemory_MoveFast(&inOSD->osd.melee_osd, &outOSD->osd.melee_osd, sizeof(OBJtOSD_Melee));

	// find a name that isn't a duplicate
	for (itr = 0; itr < UUcMaxUns16; itr++)
	{
		if (itr == 0) {
			// try input name
		} else if (itr == 1) {
			sprintf(outOSD->osd.melee_osd.name, "%s_copy", inOSD->osd.melee_osd.name);
		} else {
			sprintf(outOSD->osd.melee_osd.name, "%s_copy%d", inOSD->osd.melee_osd.name, itr);
		}

		prev_object = OBJrObjectType_Search(OBJcType_Melee, OBJcSearch_MeleeName, outOSD);
		if (prev_object != NULL) { continue; }

		break;
	}
	UUmAssert(itr < UUcMaxUns16);

	// find a new ID
	for (itr = 0; itr < 1000; itr++)
	{
		outOSD->osd.melee_osd.id = inOSD->osd.melee_osd.id + itr;

		prev_object = OBJrObjectType_Search(OBJcType_Melee, OBJcSearch_MeleeID, outOSD);
		if (prev_object != NULL) { continue; }

		break;
	}
	UUmAssert(itr < 1000);

	return;
}

// ----------------------------------------------------------------------
static void
OBJiMelee_GetUniqueOSD(
	OBJtOSD_All *inOSD)
{
	UUtUns32				itr;
	OBJtObject				*prev_object;

	UUrMemory_Clear(&inOSD->osd.melee_osd, sizeof(OBJtOSD_Melee));

	for (itr = 0; itr < UUcMaxUns16; itr++)
	{
		sprintf(inOSD->osd.melee_osd.name, "melee_%02d", itr);
		inOSD->osd.melee_osd.id = (UUtUns16) itr;

		prev_object = OBJrObjectType_Search(OBJcType_Melee, OBJcSearch_MeleeName, inOSD);
		if (prev_object != NULL) { continue; }

		prev_object = OBJrObjectType_Search(OBJcType_Melee, OBJcSearch_MeleeID, inOSD);
		if (prev_object != NULL) { continue; }

		break;
	}

	return;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiMelee_IntersectsLine(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inStartPoint,
	const M3tPoint3D		*inEndPoint)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static void
OBJiMelee_SetDefaultVersion20Weights(
	OBJtOSD_Melee		*outOSD)
{
	// default weight factors
	outOSD->weight_notblocking_any			= 2.0f;
	outOSD->weight_notblocking_changestance	= 1.5f;
	outOSD->weight_blocking_unblockable		= 1.5f;
	outOSD->weight_blocking_stagger			= 1.2f;
	outOSD->weight_blocking_blockstun		= 0.7f;
	outOSD->weight_blocking					= 0.5f;
}

// ----------------------------------------------------------------------
static void
OBJiMelee_SetDefaultVersion26DazeTimes(
	OBJtOSD_Melee		*outOSD)
{
	// default daze timers
	outOSD->dazed_minframes = 60;
	outOSD->dazed_maxframes = 100;
}

// ----------------------------------------------------------------------
static void
OBJiMelee_SetDefaultVersion31Weights(
	OBJtOSD_Melee		*outOSD)
{
	// default weight factors
	outOSD->weight_throwdanger				= 2.0f;
}

// ----------------------------------------------------------------------
static UUtError
OBJiMelee_SetDefaults(
	OBJtOSD_All			*outOSD)
{
	OBJiMelee_GetUniqueOSD(outOSD);

	UUrString_Copy(outOSD->osd.melee_osd.char_classname, AI2cDefaultClassName, sizeof(outOSD->osd.melee_osd.char_classname));

	// default general abilities
	outOSD->osd.melee_osd.attacknotice_percentage = 100;
	outOSD->osd.melee_osd.blockskill_percentage = 100;
	outOSD->osd.melee_osd.blockgroup_percentage = 100;
	outOSD->osd.melee_osd.dodge_base_percentage = 50;
	outOSD->osd.melee_osd.dodge_additional_percentage = 50;
	outOSD->osd.melee_osd.dodge_damage_threshold = 30;

	OBJiMelee_SetDefaultVersion20Weights(&outOSD->osd.melee_osd);
	OBJiMelee_SetDefaultVersion26DazeTimes(&outOSD->osd.melee_osd);
	OBJiMelee_SetDefaultVersion31Weights(&outOSD->osd.melee_osd);

	// no techniques or moves
	outOSD->osd.melee_osd.num_actions = 0;
	outOSD->osd.melee_osd.num_reactions = 0;
	outOSD->osd.melee_osd.num_spacing = 0;
	outOSD->osd.melee_osd.num_moves = 0;
	outOSD->osd.melee_osd.technique = NULL;
	outOSD->osd.melee_osd.move = NULL;

	outOSD->osd.melee_osd.char_class = NULL;
	outOSD->osd.melee_osd.reference_count = 0;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiMelee_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_All				osd_all;
	UUtError				error;
	
	if (NULL == inOSD) {
		error = OBJiMelee_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);
	}
	else
	{
		OBJiMelee_DuplicateOSD(inOSD, &osd_all);
	}
	
	// set the object specific data and position
	error = OBJrObject_SetObjectSpecificData(inObject, &osd_all);
	UUmError_ReturnOnError(error);
	
	OBJrObject_UpdatePosition(inObject);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiMelee_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	AI2tMeleeProfile		*profile = (AI2tMeleeProfile *) inObject->object_data;
	AI2tMeleeTechnique		*technique;
	AI2tMeleeMove			*move;
	UUtUns32				itr, num_techniques;
	UUtUns32				bytes_read = 0;
	
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->id,						UUtUns32,				inSwapIt);
	bytes_read += OBJmGetStringFromBuffer(inBuffer, profile->name,						sizeof(profile->name),	inSwapIt);
	bytes_read += OBJmGetStringFromBuffer(inBuffer, profile->char_classname,			sizeof(profile->char_classname),inSwapIt);


	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->attacknotice_percentage,	UUtUns32,				inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->dodge_base_percentage,		UUtUns32,				inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->dodge_additional_percentage,UUtUns32,				inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->dodge_damage_threshold,	UUtUns32,				inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->blockskill_percentage,		UUtUns32,				inSwapIt);
	if (inVersion >= OBJcVersion_38) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->blockgroup_percentage,	UUtUns32,				inSwapIt);
	} else {
		profile->blockgroup_percentage = profile->blockskill_percentage;
	}

	if (inVersion >= OBJcVersion_20) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->weight_notblocking_any,		float,			inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->weight_notblocking_changestance,float,			inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->weight_blocking_unblockable,	float,			inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->weight_blocking_stagger,		float,			inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->weight_blocking_blockstun,		float,			inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->weight_blocking,				float,			inSwapIt);
	} else {
		OBJiMelee_SetDefaultVersion20Weights((OBJtOSD_Melee *) profile);
	}

	if (inVersion >= OBJcVersion_31) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->weight_throwdanger,		float,				inSwapIt);
	} else {
		OBJiMelee_SetDefaultVersion31Weights((OBJtOSD_Melee *) profile);
	}

	if (inVersion >= OBJcVersion_26) {
		bytes_read += OBJmGet2BytesFromBuffer(inBuffer, profile->dazed_minframes,		UUtUns16,				inSwapIt);
		bytes_read += OBJmGet2BytesFromBuffer(inBuffer, profile->dazed_maxframes,		UUtUns16,				inSwapIt);
	} else {
		OBJiMelee_SetDefaultVersion26DazeTimes((OBJtOSD_Melee *) profile);
	}

	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->num_actions,				UUtUns32,				inSwapIt);
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->num_reactions,				UUtUns32,				inSwapIt);
	if (inVersion >= OBJcVersion_37) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->num_spacing,			UUtUns32,				inSwapIt);
	} else {
		profile->num_spacing = 0;
	}
	bytes_read += OBJmGet4BytesFromBuffer(inBuffer, profile->num_moves,					UUtUns32,				inSwapIt);

	// allocate the dynamic arrays
	num_techniques = profile->num_actions + profile->num_reactions + profile->num_spacing;
	profile->technique	= UUrMemory_Block_NewClear(num_techniques * sizeof(AI2tMeleeTechnique));
	profile->move		= UUrMemory_Block_NewClear(profile->num_moves * sizeof(AI2tMeleeMove));
	UUmAssert((num_techniques == 0) || (profile->technique != NULL));
	UUmAssert((profile->num_moves == 0) || (profile->move != NULL));

	// read the technique array
	for (itr = 0, technique = profile->technique; itr < num_techniques; itr++, technique++) {
		bytes_read += OBJmGetStringFromBuffer(inBuffer, technique->name,				sizeof(technique->name),inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, technique->user_flags,			UUtUns32,				inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, technique->base_weight,			UUtUns32,				inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, technique->importance,			UUtUns32,				inSwapIt);
		if (inVersion >= OBJcVersion_35) {
			bytes_read += OBJmGet4BytesFromBuffer(inBuffer, technique->delay_frames,	UUtUns32,				inSwapIt);
		} else {
			technique->delay_frames = 0;
		}

		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, technique->num_moves,			UUtUns32,				inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, technique->start_index,			UUtUns32,				inSwapIt);

		UUmAssert((technique->num_moves == 0) ||
			((technique->start_index >= 0) && ((technique->start_index + technique->num_moves) <= profile->num_moves)));
	}

	// read the move array
	for (itr = 0, move = profile->move; itr < profile->num_moves; itr++, move++) {
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, move->move,						UUtUns32,				inSwapIt);

		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, move->param[0],					float,					inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, move->param[1],					float,					inSwapIt);
		bytes_read += OBJmGet4BytesFromBuffer(inBuffer, move->param[2],					float,					inSwapIt);
	}

	// set up runtime parameters
	profile->char_class = NULL;
	profile->reference_count = 0;

	// bring the object up to date
	OBJrObject_UpdatePosition(inObject);
	
	return bytes_read;
}

// ----------------------------------------------------------------------
static UUtError
OBJiMelee_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	AI2tMeleeProfile		*profile;
	UUtUns32				blocksize;
	
	UUmAssert(inOSD);
	
	profile = (AI2tMeleeProfile *) inObject->object_data;
	
	// *** copy the data from inOSD to the melee profile

	UUrString_Copy(profile->name, inOSD->osd.melee_osd.name, sizeof(inOSD->osd.melee_osd.name));
	profile->id = inOSD->osd.melee_osd.id;
	UUrString_Copy(profile->char_classname, inOSD->osd.melee_osd.char_classname, sizeof(inOSD->osd.melee_osd.char_classname));

	profile->attacknotice_percentage		= inOSD->osd.melee_osd.attacknotice_percentage;
	profile->blockskill_percentage			= inOSD->osd.melee_osd.blockskill_percentage;
	profile->blockgroup_percentage			= inOSD->osd.melee_osd.blockgroup_percentage;
	profile->dodge_base_percentage			= inOSD->osd.melee_osd.dodge_base_percentage;
	profile->dodge_additional_percentage	= inOSD->osd.melee_osd.dodge_additional_percentage;
	profile->dodge_damage_threshold			= inOSD->osd.melee_osd.dodge_damage_threshold;

	profile->weight_notblocking_any			= inOSD->osd.melee_osd.weight_notblocking_any;
	profile->weight_notblocking_changestance= inOSD->osd.melee_osd.weight_notblocking_changestance;
	profile->weight_blocking_unblockable	= inOSD->osd.melee_osd.weight_blocking_unblockable;
	profile->weight_blocking_stagger		= inOSD->osd.melee_osd.weight_blocking_stagger;
	profile->weight_blocking_blockstun		= inOSD->osd.melee_osd.weight_blocking_blockstun;
	profile->weight_blocking				= inOSD->osd.melee_osd.weight_blocking;

	profile->weight_throwdanger				= inOSD->osd.melee_osd.weight_throwdanger;

	profile->dazed_minframes				= inOSD->osd.melee_osd.dazed_minframes;
	profile->dazed_maxframes				= inOSD->osd.melee_osd.dazed_maxframes;

	profile->num_actions					= inOSD->osd.melee_osd.num_actions;
	profile->num_reactions					= inOSD->osd.melee_osd.num_reactions;
	profile->num_spacing					= inOSD->osd.melee_osd.num_spacing;
	profile->num_moves						= inOSD->osd.melee_osd.num_moves;

	// *** allocate and copy the variable-length arrays

	blocksize = (inOSD->osd.melee_osd.num_actions + inOSD->osd.melee_osd.num_reactions + inOSD->osd.melee_osd.num_spacing) * sizeof(AI2tMeleeTechnique);
	if (blocksize == 0) {
		profile->technique = NULL;
	} else {
		profile->technique = UUrMemory_Block_New(blocksize);
		UUmError_ReturnOnNull(profile->technique);
		UUrMemory_MoveFast(inOSD->osd.melee_osd.technique, profile->technique, blocksize);
	}

	blocksize = inOSD->osd.melee_osd.num_moves * sizeof(AI2tMeleeMove);
	if (blocksize == 0) {
		profile->move = NULL;
	} else {
		profile->move = UUrMemory_Block_New(blocksize);
		UUmError_ReturnOnNull(profile->move);
		UUrMemory_MoveFast(inOSD->osd.melee_osd.move, profile->move, blocksize);
	}

	profile->char_class = inOSD->osd.melee_osd.char_class;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiMelee_UpdatePosition(
	OBJtObject				*inObject)
{
	// nothing
}

// ----------------------------------------------------------------------
static UUtError
OBJiMelee_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	AI2tMeleeProfile		*profile = (AI2tMeleeProfile *) inObject->object_data;
	AI2tMeleeTechnique		*technique;
	AI2tMeleeMove			*move;
	UUtUns32				itr, bytes_available, num_techniques;
	
	bytes_available = *ioBufferSize;

	OBJmWrite4BytesToBuffer(ioBuffer, profile->id,						UUtUns32,				bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, profile->name,					sizeof(profile->name),	bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, profile->char_classname,			sizeof(profile->char_classname),bytes_available, OBJcWrite_Little);

	OBJmWrite4BytesToBuffer(ioBuffer, profile->attacknotice_percentage,	UUtUns32,				bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->dodge_base_percentage,	UUtUns32,				bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->dodge_additional_percentage,UUtUns32,			bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->dodge_damage_threshold,	UUtUns32,				bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->blockskill_percentage,	UUtUns32,				bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->blockgroup_percentage,	UUtUns32,				bytes_available, OBJcWrite_Little);

	OBJmWrite4BytesToBuffer(ioBuffer, profile->weight_notblocking_any,				float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->weight_notblocking_changestance,		float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->weight_blocking_unblockable,			float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->weight_blocking_stagger,				float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->weight_blocking_blockstun,			float,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->weight_blocking,						float,		bytes_available, OBJcWrite_Little);

	OBJmWrite4BytesToBuffer(ioBuffer, profile->weight_throwdanger,					float,		bytes_available, OBJcWrite_Little);
	
	OBJmWrite2BytesToBuffer(ioBuffer, profile->dazed_minframes,			UUtUns16,				bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, profile->dazed_maxframes,			UUtUns16,				bytes_available, OBJcWrite_Little);

	OBJmWrite4BytesToBuffer(ioBuffer, profile->num_actions,				UUtUns32,				bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->num_reactions,			UUtUns32,				bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->num_spacing,				UUtUns32,				bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, profile->num_moves,				UUtUns32,				bytes_available, OBJcWrite_Little);

	// write the technique array
	num_techniques = profile->num_actions + profile->num_reactions + profile->num_spacing;
	for (itr = 0, technique = profile->technique; itr < num_techniques; itr++, technique++) {
		UUmAssert((technique->num_moves == 0) ||
			((technique->start_index >= 0) && ((technique->start_index + technique->num_moves) <= profile->num_moves)));

		OBJmWriteStringToBuffer(ioBuffer, technique->name,				sizeof(technique->name),bytes_available, OBJcWrite_Little);
		OBJmWrite4BytesToBuffer(ioBuffer, technique->user_flags,		UUtUns32,				bytes_available, OBJcWrite_Little);
		OBJmWrite4BytesToBuffer(ioBuffer, technique->base_weight,		UUtUns32,				bytes_available, OBJcWrite_Little);
		OBJmWrite4BytesToBuffer(ioBuffer, technique->importance,		UUtUns32,				bytes_available, OBJcWrite_Little);
		OBJmWrite4BytesToBuffer(ioBuffer, technique->delay_frames,		UUtUns32,				bytes_available, OBJcWrite_Little);
		OBJmWrite4BytesToBuffer(ioBuffer, technique->num_moves,			UUtUns32,				bytes_available, OBJcWrite_Little);
		OBJmWrite4BytesToBuffer(ioBuffer, technique->start_index,		UUtUns32,				bytes_available, OBJcWrite_Little);
	}

	// read the move array
	for (itr = 0, move = profile->move; itr < profile->num_moves; itr++, move++) {
		OBJmWrite4BytesToBuffer(ioBuffer, move->move,					UUtUns32,				bytes_available, OBJcWrite_Little);

		OBJmWrite4BytesToBuffer(ioBuffer, move->param[0],				float,					bytes_available, OBJcWrite_Little);
		OBJmWrite4BytesToBuffer(ioBuffer, move->param[1],				float,					bytes_available, OBJcWrite_Little);
		OBJmWrite4BytesToBuffer(ioBuffer, move->param[2],				float,					bytes_available, OBJcWrite_Little);
	}

	// set ioBufferSize to the number of bytes written to the buffer
	*ioBufferSize = *ioBufferSize - bytes_available;

	return UUcError_None;
}


static const char *OBJiMelee_IsInvalid(
	OBJtObject *inObject)
{
	UUtUns32 count;
	UUtUns32 itr;

	OBJtOSD_Melee *in_melee_osd = (OBJtOSD_Melee *)inObject->object_data;
	OBJtObjectType object_type = inObject->object_type;

	count = OBJrObjectType_EnumerateObjects(object_type, NULL, 0);

	for(itr = 0; itr < count; itr++)
	{
		OBJtObject *test_object = OBJrObjectType_GetObject_ByNumber(object_type, itr);

		if (test_object != inObject) {
			OBJtOSD_Melee *test_melee_osd = (OBJtOSD_Melee *)test_object->object_data;
			static const char *duplicate_name = "Duplicate Name";
			static const char *duplicate_id = "Duplicate ID";

			if (UUmString_IsEqual(in_melee_osd->name, test_melee_osd->name)) {
				return duplicate_name;
			}

			if (in_melee_osd->id == test_melee_osd->id) {
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
OBJiMelee_GetVisible(
	void)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static void
OBJiMelee_SetVisible(
	UUtBool					inIsVisible)
{
	return;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiMelee_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	OBJtOSD_Melee		*melee_osd;
	UUtBool				found;
	
	// get a pointer to the object osd
	melee_osd = (OBJtOSD_Melee *)inObject->object_data;
	
	// perform the check
	switch (inSearchType)
	{
		case OBJcSearch_MeleeName:
			found = UUmString_IsEqual(melee_osd->name, inSearchParams->osd.melee_osd.name);
		break;

		case OBJcSearch_MeleeID:
			found = melee_osd->id == inSearchParams->osd.melee_osd.id;
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
static UUtBool
OBJiMelee_LevelBeginEnumCallback(
	OBJtObject		*inObject,
	UUtUns32				inUserData)
{
	OBJtOSD_Melee *melee_osd = (OBJtOSD_Melee *) inObject->object_data;

	melee_osd->reference_count = 0;
	melee_osd->char_class = NULL;

	return UUcTrue;
}

// ----------------------------------------------------------------------
static UUtError
OBJiMelee_LevelBegin(
	void)
{
	OBJrObjectType_EnumerateObjects(OBJcType_Melee, OBJiMelee_LevelBeginEnumCallback, (UUtUns32) -1);
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrMelee_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;
	
	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));
	
	// set up the methods structure
	methods.rNew				= OBJiMelee_New;
	methods.rSetDefaults		= OBJiMelee_SetDefaults;
	methods.rDelete				= OBJiMelee_Delete;
	methods.rDraw				= OBJiMelee_Draw;
	methods.rEnumerate			= OBJiMelee_Enumerate;
	methods.rGetBoundingSphere	= OBJiMelee_GetBoundingSphere;
	methods.rOSDGetName			= OBJiMelee_OSDGetName;
	methods.rOSDSetName			= OBJiMelee_OSDSetName;
	methods.rIntersectsLine		= OBJiMelee_IntersectsLine;
	methods.rUpdatePosition		= OBJiMelee_UpdatePosition;
	methods.rGetOSD				= OBJiMelee_GetOSD;
	methods.rGetOSDWriteSize	= OBJiMelee_GetOSDWriteSize;
	methods.rSetOSD				= OBJiMelee_SetOSD;
	methods.rRead				= OBJiMelee_Read;
	methods.rWrite				= OBJiMelee_Write;
	methods.rGetClassVisible	= OBJiMelee_GetVisible;
	methods.rSearch				= OBJiMelee_Search;
	methods.rSetClassVisible	= OBJiMelee_SetVisible;
	methods.rGetUniqueOSD		= OBJiMelee_GetUniqueOSD;
	methods.rLevelBegin			= OBJiMelee_LevelBegin;
	
	// register the furniture methods
	error =
		OBJrObjectGroup_Register(
			OBJcType_Melee,
			OBJcTypeIndex_Melee,
			"Melee Profile",
			sizeof(OBJtOSD_Melee),
			&methods,
			OBJcObjectGroupFlag_CanSetName | OBJcObjectGroupFlag_CommonToAllLevels);
	UUmError_ReturnOnError(error);
		
	return UUcError_None;
}

