// ======================================================================
// OT_Character.c
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

#include <ctype.h>
#include "EulerAngles.h"

// ======================================================================
// defines
// ======================================================================
#define OBJcCharacter_DefaultDrawNameDistance			150.0f
#define OBJcCharacter_DrawSize							 23.0f

// ======================================================================
// globals
// ======================================================================

#if TOOL_VERSION
// text name drawing
static UUtBool				OBJgCharacter_DrawInitialized = UUcFalse;
static UUtRect				OBJgCharacter_TextureBounds;
static TStTextContext		*OBJgCharacter_TextContext;
static M3tTextureMap		*OBJgCharacter_Texture;
static IMtPoint2D			OBJgCharacter_Dest;
static IMtPixel				OBJgCharacter_WhiteColor;
static UUtInt16				OBJgCharacter_TextureWidth;
static UUtInt16				OBJgCharacter_TextureHeight;
static float				OBJgCharacter_WidthRatio;
#endif

static UUtBool				OBJgCharacter_DrawPositions;

static float				OBJgCharacter_DrawNameDistance;

// external
const char					*OBJgCharacter_TeamName[OBJcCharacter_TeamMaxNamed]
		 = {"Konoko", "TCTF", "Syndicate", "Neutral", "SecurityGuard", "RogueKonoko",
			"Switzerland", "SyndicateAccessory"};


const IMtShade				OBJgCharacter_TeamColor[OBJcCharacter_TeamMaxNamed]
		 = {IMcShade_Purple, IMcShade_Green, IMcShade_Red, IMcShade_Blue, IMcShade_Yellow,
			IMcShade_LightBlue, IMcShade_Pink, IMcShade_Orange};

// ======================================================================
// functions
// ======================================================================
#if TOOL_VERSION
// ----------------------------------------------------------------------
UUtError
OBJrCharacter_DrawInitialize(
	void)
{
	if (!OBJgCharacter_DrawInitialized) {
		UUtError				error;
		TStFontFamily			*font_family;
		IMtPixelType			textureFormat;
		
		M3rDrawEngine_FindGrayscalePixelType(&textureFormat);
		
		OBJgCharacter_WhiteColor = IMrPixel_FromShade(textureFormat, IMcShade_White);

		error = TSrFontFamily_Get(TScFontFamily_Default, &font_family);
		if (error != UUcError_None)
		{
			goto cleanup;
		}
		
		error = TSrContext_New(font_family, TScFontSize_Default, TScStyle_Bold, TSc_SingleLine, UUcFalse, &OBJgCharacter_TextContext);
		if (error != UUcError_None)
		{
			goto cleanup;
		}
		
		OBJgCharacter_Dest.x = 2;
		OBJgCharacter_Dest.y =
			TSrFont_GetLeadingHeight(TSrContext_GetFont(OBJgCharacter_TextContext, TScStyle_InUse)) +
			TSrFont_GetAscendingHeight(TSrContext_GetFont(OBJgCharacter_TextContext, TScStyle_InUse));
		
		TSrContext_GetStringRect(OBJgCharacter_TextContext, "maximum_length_of_character_name", &OBJgCharacter_TextureBounds);
		
		error =
			M3rTextureMap_New(
				OBJgCharacter_TextureBounds.right,
				OBJgCharacter_TextureBounds.bottom,
				textureFormat,
				M3cTexture_AllocMemory,
				M3cTextureFlags_None,
				"character name texture",
				&OBJgCharacter_Texture);
		if (error != UUcError_None)
		{
			goto cleanup;
		}

		// get the REAL size of the texture that motoko allocated for us
		M3rTextureRef_GetSize((void *) OBJgCharacter_Texture,
						(UUtUns16*)&OBJgCharacter_TextureBounds.right,
						(UUtUns16*)&OBJgCharacter_TextureBounds.bottom);
		
		OBJgCharacter_TextureWidth = OBJgCharacter_TextureBounds.right;
		OBJgCharacter_TextureHeight = OBJgCharacter_TextureBounds.bottom;
		
		OBJgCharacter_WidthRatio = (float)OBJgCharacter_TextureWidth / (float)OBJgCharacter_TextureHeight;

		OBJgCharacter_DrawInitialized = UUcTrue;
	}

	return UUcError_None;

cleanup:
	if (OBJgCharacter_TextContext)
	{
		TSrContext_Delete(OBJgCharacter_TextContext);
		OBJgCharacter_TextContext = NULL;
	}

	return UUcError_Generic;
}

// ----------------------------------------------------------------------
static void
OBJiCharacter_DrawName(
	OBJtObject				*inObject,
	M3tPoint3D				*inLocation)
{
	UUtError				error;

	error = OBJrCharacter_DrawInitialize();
	if (error == UUcError_None) {
		char					name[OBJcMaxNameLength + 1];
		UUtRect					string_bounds;
		UUtUns16				string_width;
		IMtPoint2D				text_dest;
		float					sprite_size;
		
		// erase the texture and set the text contexts shade
		M3rTextureMap_Fill(OBJgCharacter_Texture, OBJgCharacter_WhiteColor, NULL);
		TSrContext_SetShade(OBJgCharacter_TextContext, IMcShade_Black);
		
		// get the character's name
		OBJrObject_GetName(inObject, name, OBJcMaxNameLength);
		
		// work out how big the string is going to be
		TSrContext_GetStringRect(OBJgCharacter_TextContext, name, &string_bounds);
		string_width = string_bounds.right - string_bounds.left;

		text_dest = OBJgCharacter_Dest;
		text_dest.x += (OBJgCharacter_TextureWidth - string_width) / 2;

		sprite_size = ((float) string_width) / OBJgCharacter_TextureWidth;

		// draw the string to the texture
		TSrContext_DrawString(
			OBJgCharacter_TextContext,
			OBJgCharacter_Texture,
			name,
			&OBJgCharacter_TextureBounds,
			&text_dest);
		
		// draw the sprite
		M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, M3cGeomState_SpriteMode_Normal);
		M3rGeom_State_Commit();

		M3rSprite_Draw(
			OBJgCharacter_Texture,
			inLocation,
			OBJgCharacter_WidthRatio,
			1.0f,
			IMcShade_White,
			M3cMaxAlpha,
			0,
			NULL,
			NULL,
			0,
			1.0f - sprite_size, 0);
	}
	
	return;
}

// ----------------------------------------------------------------------
void
OBJrCharacter_DrawTerminate(
	void)
{
	if (OBJgCharacter_DrawInitialized) {
		if (OBJgCharacter_TextContext)
		{
			TSrContext_Delete(OBJgCharacter_TextContext);
			OBJgCharacter_TextContext = NULL;
		}

		OBJgCharacter_DrawInitialized = UUcFalse;
	}
}
#endif

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiCharacter_Delete(
	OBJtObject				*inObject)
{
}

// ----------------------------------------------------------------------
static void
OBJiCharacter_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
#if TOOL_VERSION
	OBJtOSD_Character		*char_osd;
	M3tPoint3D				camera_location;
	IMtShade				char_shade;

	// KNA: replaced size with OBJcCharacter_DrawSize so that CodeWarrior would compile
	M3tPoint3D points[8] = 
	{
		{ 0.00f * OBJcCharacter_DrawSize, 0.00f * OBJcCharacter_DrawSize,  0.0f },
		{ 0.25f * OBJcCharacter_DrawSize, 0.75f * OBJcCharacter_DrawSize,  0.0f },
		{ 0.00f * OBJcCharacter_DrawSize, 1.00f * OBJcCharacter_DrawSize,  0.0f },
		{-0.25f * OBJcCharacter_DrawSize, 0.75f * OBJcCharacter_DrawSize,  0.0f },
		{ 0.00f * OBJcCharacter_DrawSize, 0.75f * OBJcCharacter_DrawSize,  0.0f },
		{ 0.00f * OBJcCharacter_DrawSize, 0.75f * OBJcCharacter_DrawSize,  0.5f * OBJcCharacter_DrawSize },
		{ 0.05f * OBJcCharacter_DrawSize, 0.75f * OBJcCharacter_DrawSize,  0.4f * OBJcCharacter_DrawSize },
		{-0.05f * OBJcCharacter_DrawSize, 0.75f * OBJcCharacter_DrawSize,  0.4f * OBJcCharacter_DrawSize }
	};
	
	if (OBJgCharacter_DrawPositions == UUcFalse) { return; }
	
	// get a pointer to the object osd
	char_osd = (OBJtOSD_Character *)inObject->object_data;
	
	// set up the matrix stack
	M3rMatrixStack_Push();
	M3rMatrixStack_ApplyTranslate(inObject->position);
	M3rMatrixStack_RotateYAxis(char_osd->facing);
	M3rGeom_State_Commit();
	
	// draw the character position marker
	if (char_osd->team_number < OBJcCharacter_TeamMaxNamed)
		char_shade = OBJgCharacter_TeamColor[char_osd->team_number];
	else
		char_shade = IMcShade_White;

	M3rGeom_Line_Light(points + 0, points + 1, char_shade);
	M3rGeom_Line_Light(points + 1, points + 2, char_shade);
	M3rGeom_Line_Light(points + 2, points + 3, char_shade);
	M3rGeom_Line_Light(points + 3, points + 0, char_shade);

	M3rGeom_Line_Light(points + 4, points + 5, char_shade);
	M3rGeom_Line_Light(points + 5, points + 6, char_shade);
	M3rGeom_Line_Light(points + 5, points + 7, char_shade);
	
	// draw the name
	camera_location = CArGetLocation();
	if (MUrPoint_Distance(&inObject->position, &camera_location) < OBJgCharacter_DrawNameDistance)
	{
		OBJiCharacter_DrawName(inObject, points + 2);
	}
	
	// draw the rotation ring if this character is selected
	if (inDrawFlags & OBJcDrawFlag_Selected)
	{
		M3tBoundingSphere	bounding_sphere;
		
		OBJrObject_GetBoundingSphere(inObject, &bounding_sphere);
		OBJrObjectUtil_DrawRotationRings(inObject, &bounding_sphere, OBJcDrawFlag_RingY);
	}

	M3rMatrixStack_Pop();
#endif

	return;
}

// ----------------------------------------------------------------------
static UUtError
OBJiCharacter_Enumerate(
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
OBJiCharacter_GetBoundingSphere(
	const OBJtObject		*inObject,
	M3tBoundingSphere		*outBoundingSphere)
{
	outBoundingSphere->center.x = 0.0f;
	outBoundingSphere->center.y = OBJcCharacter_DrawSize / 2.0f;
	outBoundingSphere->center.z = 0.0f;
	outBoundingSphere->radius = OBJcCharacter_DrawSize / 2.0f;	// just an approximation
}

// ----------------------------------------------------------------------
static void
OBJiCharacter_OSDGetName(
	const OBJtOSD_All		*inOSD,
	char					*outName,
	UUtUns32				inNameLength)
{
	const OBJtOSD_Character	*char_osd = &inOSD->osd.character_osd;
	
	UUrString_Copy(outName, char_osd->character_name, inNameLength);

	return;
}


// ----------------------------------------------------------------------
static void
OBJiCharacter_OSDSetName(
	OBJtOSD_All				*inOSD,
	const char				*inName)
{
	OBJtOSD_Character		*char_osd = &inOSD->osd.character_osd;
	
	UUrString_Copy(char_osd->character_name, inName, sizeof(char_osd->character_name));

	return;
}


// ----------------------------------------------------------------------
static void
OBJiCharacter_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	OBJtOSD_Character		*char_osd;

	// get a pointer to the object osd
	char_osd = (OBJtOSD_Character *)inObject->object_data;
	
	outOSD->osd.character_osd = *char_osd;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiCharacter_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	UUtUns32				size;
	
	size =
		sizeof(AI2tAlertStatus) +		// initial alert
		sizeof(AI2tAlertStatus) +		// minimum alert
		sizeof(AI2tAlertStatus) +		// job-start alert
		sizeof(AI2tAlertStatus) +		// investigate alert
		4 +								// job
		2 +								// job specific id
		2 +								// combat id
		2 +								// melee id
		2 +								// neutral id
		sizeof(UUtUns32) +				// flags
		64 +							// character_class
		ONcMaxPlayerNameLength +		// character_name
		64 +							// weapon_class_name
		SLcScript_MaxNameLength +		// spawn_script
		SLcScript_MaxNameLength +		// die_script
		SLcScript_MaxNameLength +		// combat_script
		SLcScript_MaxNameLength +		// alarm_script
		SLcScript_MaxNameLength +		// hurt_script
		SLcScript_MaxNameLength +		// defeated_script
		SLcScript_MaxNameLength +		// outofammo_script
		SLcScript_MaxNameLength +		// nopath_script
		sizeof(UUtInt32) +				// hit_points
		OBJcCharInv_Max * OBJcCharSlot_Max * sizeof(UUtUns16) +		// inventory
		sizeof(UUtUns32) +				// team_number
		sizeof(UUtUns32) +				// ammo_percentage
		sizeof(UUtUns32) +				// alarm groups
		4*sizeof(AI2tPursuitMode) +		// pursuit modes
		sizeof(AI2tPursuitLostBehavior);// target lost behavior
	
	return size;
}

// ----------------------------------------------------------------------
static void
OBJiCharacter_GetUniqueOSD(
	OBJtOSD_All *inOSD)
{
	UUtUns32				itr;
	OBJtObject				*prev_object;
	OBJtOSD_Character		*char_osd = &inOSD->osd.character_osd;
	ONtCharacterClass		*char_class;
	UUtError				error;

	UUrMemory_Clear(char_osd, sizeof(*char_osd));

	for (itr = 0; itr < UUcMaxUns32; itr++)
	{
		sprintf(char_osd->character_name, "char_%d", itr);

		prev_object = OBJrObjectType_Search(OBJcType_Character, OBJcSearch_CharacterName, inOSD);
		if (prev_object != NULL) { continue; }

		break;
	}

	// default values
	char_osd->flags = OBJcCharFlags_NotInitiallyPresent;
	UUrString_Copy(char_osd->character_class, AI2cDefaultClassName, 64);

	char_osd->weapon_class_name[0] = '\0';
	char_osd->spawn_script[0] = '\0';
	char_osd->die_script[0] = '\0';
	char_osd->combat_script[0] = '\0';
	char_osd->alarm_script[0] = '\0';
	char_osd->hurt_script[0] = '\0';
	char_osd->defeated_script[0] = '\0';
	char_osd->outofammo_script[0] = '\0';
	char_osd->nopath_script[0] = '\0';
	char_osd->alarm_groups	 = 0;
	char_osd->hit_points = 0;				// offset from class default
	for (itr = 0; itr < OBJcCharInv_Max; itr++) {
		UUtUns32 itr2;

		for (itr2 = 0; itr2 < OBJcCharSlot_Max; itr2++) {
			char_osd->inventory[itr][itr2] = 0;
		}
	}
	char_osd->team_number = 0;
	char_osd->ammo_percentage = 100;
	char_osd->initial_alert = AI2cAlertStatus_Lull;
	char_osd->minimum_alert = AI2cAlertStatus_Lull;
	char_osd->jobstart_alert = AI2cAlertStatus_Low;
	char_osd->investigate_alert = AI2cAlertStatus_Medium;
	char_osd->combat_id = 0;
	char_osd->melee_id = 0;
	char_osd->neutral_id = 0;

/*	char_osd->pursue_strong_unseen	= AI2cPursuitMode_Move;
	char_osd->pursue_weak_unseen	= AI2cPursuitMode_Forget;
	char_osd->pursue_strong_seen	= AI2cPursuitMode_Hunt;
	char_osd->pursue_weak_seen		= AI2cPursuitMode_Look;*/
	char_osd->pursue_strong_unseen	= AI2cPursuitMode_Look;
	char_osd->pursue_weak_unseen	= AI2cPursuitMode_Forget;
	char_osd->pursue_strong_seen	= AI2cPursuitMode_Look;
	char_osd->pursue_weak_seen		= AI2cPursuitMode_Look;
	char_osd->pursue_lost = AI2cPursuitLost_Return;

	// get the default character class's default combat and melee settings
	error = TMrInstance_GetDataPtr(TRcTemplate_CharacterClass, char_osd->character_class, (void **) &char_class);
	if (error == UUcError_None) {
		char_osd->combat_id = char_class->ai2_behavior.default_combat_id;
		char_osd->melee_id = char_class->ai2_behavior.default_melee_id;
	}

	return;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiCharacter_IntersectsLine(
	const OBJtObject		*inObject,
	const M3tPoint3D		*inStartPoint,
	const M3tPoint3D		*inEndPoint)
{
	M3tBoundingSphere		sphere;

	UUrMemory_Clear(&sphere, sizeof(M3tBoundingSphere));
	
	// get the bounding sphere
	OBJrObject_GetBoundingSphere(inObject, &sphere);
	
	sphere.center.x += inObject->position.x;
	sphere.center.y += inObject->position.y;
	sphere.center.z += inObject->position.z;
	
	return CLrSphere_Line(inStartPoint, inEndPoint, &sphere);
}

// ----------------------------------------------------------------------
static UUtError
OBJiCharacter_SetDefaults(
	OBJtOSD_All				*outOSD)
{
	OBJiCharacter_GetUniqueOSD(outOSD);
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiCharacter_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	UUtError				error;
	OBJtOSD_Character		*char_osd;
	
	// get a pointer to the object osd
	char_osd = (OBJtOSD_Character *)inObject->object_data;
	
	if (inOSD == NULL)
	{
		error = OBJiCharacter_SetDefaults((OBJtOSD_All *) char_osd);
		UUmError_ReturnOnError(error);
	}
	else
	{
		*char_osd = inOSD->osd.character_osd;
	}
	
	// set the object specific data and position
	OBJrObject_UpdatePosition(inObject);

	UUrMemory_Block_VerifyList();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiCharacter_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_Character		*character = (OBJtOSD_Character *) inObject->object_data;
	UUtUns32				num_bytes = 0;
	UUtUns32				itr, itr2;
	
	// read the data
	num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->flags,				UUtUns32,				inSwapIt);
	character->flags &= ~OBJcCharFlags_Spawned;		// clear runtime-only flags

	num_bytes += OBJmGetStringFromBuffer(inBuffer, character->character_class,	64,						inSwapIt);
	num_bytes += OBJmGetStringFromBuffer(inBuffer, character->character_name,		ONcMaxPlayerNameLength,	inSwapIt);
	num_bytes += OBJmGetStringFromBuffer(inBuffer, character->weapon_class_name,	64,						inSwapIt);
	num_bytes += OBJmGetStringFromBuffer(inBuffer, character->spawn_script,		SLcScript_MaxNameLength,inSwapIt);	
	if (inVersion >= OBJcVersion_8) {
		num_bytes += OBJmGetStringFromBuffer(inBuffer, character->die_script,		SLcScript_MaxNameLength,inSwapIt);	
		num_bytes += OBJmGetStringFromBuffer(inBuffer, character->combat_script,		SLcScript_MaxNameLength,inSwapIt);	
		num_bytes += OBJmGetStringFromBuffer(inBuffer, character->alarm_script,		SLcScript_MaxNameLength,inSwapIt);	
	} else {
		character->die_script[0] = '\0';
		character->combat_script[0] = '\0';
		character->alarm_script[0] = '\0';
	}
	if (inVersion >= OBJcVersion_21) {
		num_bytes += OBJmGetStringFromBuffer(inBuffer, character->hurt_script,		SLcScript_MaxNameLength,inSwapIt);	
	} else {
		character->hurt_script[0] = '\0';
	}
	if (inVersion >= OBJcVersion_29) {
		num_bytes += OBJmGetStringFromBuffer(inBuffer, character->defeated_script,		SLcScript_MaxNameLength,inSwapIt);	
	} else {
		character->defeated_script[0] = '\0';
	}
	if (inVersion >= OBJcVersion_34) {
		num_bytes += OBJmGetStringFromBuffer(inBuffer, character->outofammo_script,		SLcScript_MaxNameLength,inSwapIt);	
		num_bytes += OBJmGetStringFromBuffer(inBuffer, character->nopath_script,		SLcScript_MaxNameLength,inSwapIt);	
	} else {
		character->outofammo_script[0] = '\0';
		character->nopath_script[0] = '\0';
	}
	num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->hit_points,			UUtInt32,				inSwapIt);

	if (inVersion >= OBJcVersion_5) {
		num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->job, UUtUns32, inSwapIt);
		num_bytes += OBJmGet2BytesFromBuffer(inBuffer, character->job_specific_id, UUtUns16, inSwapIt);
	}
	else {
		character->job = 0;
		character->job_specific_id = 0;
	}

	if (inVersion >= OBJcVersion_12) {
		num_bytes += OBJmGet2BytesFromBuffer(inBuffer, character->combat_id, UUtUns16, inSwapIt);
		num_bytes += OBJmGet2BytesFromBuffer(inBuffer, character->melee_id, UUtUns16, inSwapIt);

		if (inVersion >= OBJcVersion_23) {
			num_bytes += OBJmGet2BytesFromBuffer(inBuffer, character->neutral_id, UUtUns16, inSwapIt);
		} else {
			num_bytes += OBJmSkip2BytesFromBuffer(inBuffer);	// word-alignment padding
			character->neutral_id = 0;
		}

	} else if (inVersion >= OBJcVersion_9) {
		num_bytes += OBJmGet2BytesFromBuffer(inBuffer, character->combat_id, UUtUns16, inSwapIt);
		character->melee_id = 0;
		character->neutral_id = 0;
	}
	else if (inVersion >= OBJcVersion_5) {
		// CB: in versions 5-9 the combat behaviors were stored in the character. they aren't any more.
		for(itr = 0; itr < 5; itr++) {
			num_bytes += OBJmSkip4BytesFromBuffer(inBuffer);
		}

		character->combat_id = 0;
		character->melee_id = 0;
		character->neutral_id = 0;
	}
	else {
		character->combat_id = 0;
		character->melee_id = 0;
		character->neutral_id = 0;
	}

	for (itr = 0; itr < OBJcCharInv_Max; itr++) {
		for (itr2 = 0; itr2 < OBJcCharSlot_Max; itr2++) {
			num_bytes += OBJmGet2BytesFromBuffer(inBuffer, character->inventory[itr][itr2], UUtUns16,		inSwapIt);
		}
	}
	num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->team_number,		UUtUns32,				inSwapIt);
	num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->ammo_percentage,	UUtUns32,				inSwapIt);
		
	if (inVersion >= OBJcVersion_8) {
		num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->initial_alert,	AI2tAlertStatus,	inSwapIt);
		num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->minimum_alert,	AI2tAlertStatus,	inSwapIt);
		num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->jobstart_alert,	AI2tAlertStatus,	inSwapIt);
		num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->investigate_alert,AI2tAlertStatus,	inSwapIt);
	} else {
		character->initial_alert = AI2cAlertStatus_Lull;
		character->minimum_alert = AI2cAlertStatus_Lull;
		character->jobstart_alert = AI2cAlertStatus_Low;
		character->investigate_alert = AI2cAlertStatus_Medium;
	}

	if( inVersion >= OBJcVersion_17 )
	{
		num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->alarm_groups,	UUtUns32,	inSwapIt);
	}
	else
	{
		character->alarm_groups	= 0;
	}

	if (inVersion >= OBJcVersion_18) {
		num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->pursue_strong_unseen,	AI2tPursuitMode,	inSwapIt);
		num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->pursue_weak_unseen,	AI2tPursuitMode,	inSwapIt);
		num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->pursue_strong_seen,	AI2tPursuitMode,	inSwapIt);
		num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->pursue_weak_seen,		AI2tPursuitMode,	inSwapIt);

		num_bytes += OBJmGet4BytesFromBuffer(inBuffer, character->pursue_lost,			AI2tPursuitLostBehavior,inSwapIt);
	} else {
		// default values
/*		character->pursue_strong_unseen	= AI2cPursuitMode_Move;
		character->pursue_weak_unseen	= AI2cPursuitMode_Forget;
		character->pursue_strong_seen	= AI2cPursuitMode_Hunt;
		character->pursue_weak_seen		= AI2cPursuitMode_Look;*/
		character->pursue_strong_unseen	= AI2cPursuitMode_Look;
		character->pursue_weak_unseen	= AI2cPursuitMode_Forget;
		character->pursue_strong_seen	= AI2cPursuitMode_Look;
		character->pursue_weak_seen		= AI2cPursuitMode_Look;

		character->pursue_lost			= AI2cPursuitLost_Return;
	}

	// bring the object up to date
	OBJrObject_UpdatePosition(inObject);
	
	return num_bytes;
}

// ----------------------------------------------------------------------
static UUtError
OBJiCharacter_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_Character		*char_osd;
	ONtCharacterClass		*char_class;
	UUtUns16				prev_default_combat, prev_default_melee;
	UUtError				error;

	UUmAssert(inOSD);
	
	// get a pointer to the object osd
	char_osd = (OBJtOSD_Character *)inObject->object_data;

	// get the previous character class's default combat and melee settings
	error = TMrInstance_GetDataPtr(TRcTemplate_CharacterClass, char_osd->character_class, (void **) &char_class);
	if (error == UUcError_None) {
		prev_default_combat = char_class->ai2_behavior.default_combat_id;
		prev_default_melee  = char_class->ai2_behavior.default_melee_id;
	} else {
		prev_default_combat = (UUtUns16) -1;
		prev_default_melee  = (UUtUns16) -1;
	}

	UUrMemory_MoveFast(&inOSD->osd.character_osd, char_osd, sizeof(OBJtOSD_Character));
	
	UUrMemory_Block_VerifyList();

	// get the new character class's default combat and melee settings
	error = TMrInstance_GetDataPtr(TRcTemplate_CharacterClass, char_osd->character_class, (void **) &char_class);
	if (error == UUcError_None) {
		// if the combat settings are different for this class, use the default
		if (char_class->ai2_behavior.default_combat_id != prev_default_combat) {
			char_osd->combat_id = char_class->ai2_behavior.default_combat_id;
		}

		// if the melee settings are different for this class, use the default
		if (char_class->ai2_behavior.default_melee_id != prev_default_melee) {
			char_osd->melee_id = char_class->ai2_behavior.default_melee_id;
		}
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiCharacter_UpdatePosition(
	OBJtObject				*inObject)
{
	OBJtOSD_Character		*char_osd;
	
	// get a pointer to the object osd
	char_osd = (OBJtOSD_Character *)inObject->object_data;
	
	// convert the rotation to radians
	char_osd->facing = inObject->rotation.y * M3cDegToRad;
}

// ----------------------------------------------------------------------
static UUtError
OBJiCharacter_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	OBJtOSD_Character		*char_osd;
	UUtUns32				bytes_available, itr, itr2;

	// get a pointer to the object osd
	char_osd = (OBJtOSD_Character *)inObject->object_data;
	
	// set the number of bytes available
	bytes_available = *ioBufferSize;
	
	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->flags,				UUtUns32,				bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, char_osd->character_class,	64,						bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, char_osd->character_name,		ONcMaxPlayerNameLength,	bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, char_osd->weapon_class_name,	64,						bytes_available, OBJcWrite_Little);
	OBJmWriteStringToBuffer(ioBuffer, char_osd->spawn_script,		SLcScript_MaxNameLength,bytes_available, OBJcWrite_Little);	
	OBJmWriteStringToBuffer(ioBuffer, char_osd->die_script,			SLcScript_MaxNameLength,bytes_available, OBJcWrite_Little);	
	OBJmWriteStringToBuffer(ioBuffer, char_osd->combat_script,		SLcScript_MaxNameLength,bytes_available, OBJcWrite_Little);	
	OBJmWriteStringToBuffer(ioBuffer, char_osd->alarm_script,		SLcScript_MaxNameLength,bytes_available, OBJcWrite_Little);	
	OBJmWriteStringToBuffer(ioBuffer, char_osd->hurt_script,		SLcScript_MaxNameLength,bytes_available, OBJcWrite_Little);	
	OBJmWriteStringToBuffer(ioBuffer, char_osd->defeated_script,	SLcScript_MaxNameLength,bytes_available, OBJcWrite_Little);	
	OBJmWriteStringToBuffer(ioBuffer, char_osd->outofammo_script,	SLcScript_MaxNameLength,bytes_available, OBJcWrite_Little);	
	OBJmWriteStringToBuffer(ioBuffer, char_osd->nopath_script,		SLcScript_MaxNameLength,bytes_available, OBJcWrite_Little);	
	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->hit_points,			UUtInt32,				bytes_available, OBJcWrite_Little);

	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->job,				UUtUns32,				bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, char_osd->job_specific_id,	UUtUns16,				bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, char_osd->combat_id,			UUtUns16,				bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, char_osd->melee_id,			UUtUns16,				bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, char_osd->neutral_id,			UUtUns16,				bytes_available, OBJcWrite_Little);

	for (itr = 0; itr < OBJcCharInv_Max; itr++) {
		for (itr2 = 0; itr2 < OBJcCharSlot_Max; itr2++) {
			OBJmWrite2BytesToBuffer(ioBuffer, char_osd->inventory[itr][itr2], UUtUns16,		bytes_available, OBJcWrite_Little);
		}
	}

	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->team_number,		UUtUns32,				bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->ammo_percentage,	UUtUns32,				bytes_available, OBJcWrite_Little);

	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->initial_alert,		AI2tAlertStatus,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->minimum_alert,		AI2tAlertStatus,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->jobstart_alert,		AI2tAlertStatus,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->investigate_alert,	AI2tAlertStatus,		bytes_available, OBJcWrite_Little);

	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->alarm_groups,		UUtUns32,				bytes_available, OBJcWrite_Little);

	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->pursue_strong_unseen,AI2tPursuitMode,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->pursue_weak_unseen,	AI2tPursuitMode,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->pursue_strong_seen,	AI2tPursuitMode,		bytes_available, OBJcWrite_Little);
	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->pursue_weak_seen,	AI2tPursuitMode,		bytes_available, OBJcWrite_Little);

	OBJmWrite4BytesToBuffer(ioBuffer, char_osd->pursue_lost,		AI2tPursuitLostBehavior,bytes_available, OBJcWrite_Little);

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
OBJiCharacter_GetVisible(
	void)
{
	return OBJgCharacter_DrawPositions;
}

// ----------------------------------------------------------------------
static void
OBJiCharacter_SetVisible(
	UUtBool					inIsVisible)
{
	OBJgCharacter_DrawPositions = inIsVisible;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiCharacter_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	OBJtOSD_Character		*char_osd;
	UUtBool					found;
	
	// get a pointer to the object osd
	char_osd = (OBJtOSD_Character *)inObject->object_data;
	
	// perform the check
	found = UUcFalse;
	switch (inSearchType)
	{
		case OBJcSearch_CharacterName:
			if (strcmp(char_osd->character_name, inSearchParams->osd.character_osd.character_name) == 0)
			{
				found = UUcTrue;
			}
		break;

		case OBJcSearch_CharacterFlags:
			if ((char_osd->flags & inSearchParams->osd.character_osd.flags) == inSearchParams->osd.character_osd.flags)
			{
				found = UUcTrue;
			}
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
OBJrCharacter_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;
	
	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));
	
	// set up the methods structure
	methods.rNew				= OBJiCharacter_New;
	methods.rSetDefaults		= OBJiCharacter_SetDefaults;
	methods.rDelete				= OBJiCharacter_Delete;
	methods.rDraw				= OBJiCharacter_Draw;
	methods.rEnumerate			= OBJiCharacter_Enumerate;
	methods.rGetBoundingSphere	= OBJiCharacter_GetBoundingSphere;
	methods.rOSDGetName			= OBJiCharacter_OSDGetName;
	methods.rOSDSetName			= OBJiCharacter_OSDSetName;
	methods.rIntersectsLine		= OBJiCharacter_IntersectsLine;
	methods.rUpdatePosition		= OBJiCharacter_UpdatePosition;
	methods.rGetOSD				= OBJiCharacter_GetOSD;
	methods.rGetOSDWriteSize	= OBJiCharacter_GetOSDWriteSize;
	methods.rSetOSD				= OBJiCharacter_SetOSD;
	methods.rRead				= OBJiCharacter_Read;
	methods.rWrite				= OBJiCharacter_Write;
	methods.rGetClassVisible	= OBJiCharacter_GetVisible;
	methods.rSearch				= OBJiCharacter_Search;
	methods.rSetClassVisible	= OBJiCharacter_SetVisible;
	methods.rGetUniqueOSD		= OBJiCharacter_GetUniqueOSD;
	
	// register the furniture methods
	error =
		OBJrObjectGroup_Register(
			OBJcType_Character,
			OBJcTypeIndex_Character,
			"Character",
			sizeof(OBJtOSD_Character),
			&methods,
			OBJcObjectGroupFlag_CanSetName);
	UUmError_ReturnOnError(error);
	
#if CONSOLE_DEBUGGING_COMMANDS
	error =
		SLrGlobalVariable_Register_Bool(
			"show_characters",
			"Enables the display of character starting positions",
			&OBJgCharacter_DrawPositions);
	UUmError_ReturnOnError(error);
	
	error =
		SLrGlobalVariable_Register_Float(
			"character_name_distance",
			"Specifies the distance from the camera that character names no longer draw.",
			&OBJgCharacter_DrawNameDistance);
	UUmError_ReturnOnError(error);
#endif
	
	// intialize the globals
	OBJgCharacter_DrawNameDistance = OBJcCharacter_DefaultDrawNameDistance;
	
	return UUcError_None;
}

