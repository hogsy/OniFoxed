// ======================================================================
// OT_Flags.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ScriptLang.h"
#include "BFW_TextSystem.h"

#include "Oni_Object.h"
#include "Oni_Object_Private.h"
#include "Oni_BinaryData.h"

#include <ctype.h>
#include "EulerAngles.h"

// ======================================================================
// defines
// ======================================================================
#define OBJcFlag_DefaultDrawNameDistance			150.0f

// ======================================================================
// globals
// ======================================================================
#if TOOL_VERSION
// text name drawing
static UUtBool				OBJgFlag_DrawingInitialized = UUcFalse;
static UUtRect				OBJgFlag_TextureBounds;
static TStTextContext		*OBJgFlag_TextContext;
static M3tTextureMap		*OBJgFlag_Texture;
static IMtPoint2D			OBJgFlag_Dest;
static IMtPixel				OBJgFlag_WhiteColor;
static UUtInt16				OBJgFlag_TextureWidth;
static UUtInt16				OBJgFlag_TextureHeight;
static float				OBJgFlag_WidthRatio;
#endif

static IMtShade				OBJgFlag_Shade;
static UUtUns16				OBJgFlag_Prefix;
static UUtInt32				OBJgFlag_ID;

static UUtBool				OBJgFlag_DrawFlags;

static float				OBJgFlag_DrawNameDistance;

static UUtUns16				OBJgFlag_ViewPrefix;

static UUtError
OBJiFlag_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD);


// ======================================================================
// functions
// ======================================================================
#if TOOL_VERSION
// ----------------------------------------------------------------------
UUtError
OBJrFlag_DrawInitialize(
	void)
{
	if (!OBJgFlag_DrawingInitialized) {
		UUtError				error;
		TStFontFamily			*font_family;
		IMtPixelType			textureFormat;
		
		M3rDrawEngine_FindGrayscalePixelType(&textureFormat);
		
		OBJgFlag_WhiteColor = IMrPixel_FromShade(textureFormat, IMcShade_White);

		error = TSrFontFamily_Get(TScFontFamily_Default, &font_family);
		if (error != UUcError_None)
		{
			goto cleanup;
		}
		
		error = TSrContext_New(font_family, TScFontSize_Default, TScStyle_Bold, TSc_HLeft, UUcFalse, &OBJgFlag_TextContext);
		if (error != UUcError_None)
		{
			goto cleanup;
		}
		
		OBJgFlag_Dest.x = 2;
		OBJgFlag_Dest.y =
			TSrFont_GetLeadingHeight(TSrContext_GetFont(OBJgFlag_TextContext, TScStyle_InUse)) +
			TSrFont_GetAscendingHeight(TSrContext_GetFont(OBJgFlag_TextContext, TScStyle_InUse));
		
		TSrContext_GetStringRect(OBJgFlag_TextContext, "XX-XXXX", &OBJgFlag_TextureBounds);
		
		OBJgFlag_TextureWidth = OBJgFlag_TextureBounds.right;
		OBJgFlag_TextureHeight = OBJgFlag_TextureBounds.bottom;
		
		OBJgFlag_WidthRatio = (float)OBJgFlag_TextureWidth / (float)OBJgFlag_TextureHeight;
		
		error =
			M3rTextureMap_New(
				OBJgFlag_TextureWidth,
				OBJgFlag_TextureHeight,
				textureFormat,
				M3cTexture_AllocMemory,
				M3cTextureFlags_None,
				"flag texture",
				&OBJgFlag_Texture);
		if (error != UUcError_None)
		{
			goto cleanup;
		}
	
		OBJgFlag_DrawingInitialized = UUcTrue;
	}

	return UUcError_None;

cleanup:
	if (OBJgFlag_TextContext)
	{
		TSrContext_Delete(OBJgFlag_TextContext);
		OBJgFlag_TextContext = NULL;
	}

	return UUcError_Generic;
}

// ----------------------------------------------------------------------
static void
OBJiFlag_DrawName(
	OBJtObject				*inObject,
	M3tPoint3D				*inLocation)
{
	UUtError				error;

	error = OBJrFlag_DrawInitialize();
	if (error == UUcError_None) {
		char					name[OBJcMaxNameLength + 1];
		
		// erase the texture and set the text contexts shade
		M3rTextureMap_Fill(OBJgFlag_Texture, OBJgFlag_WhiteColor, NULL);
		TSrContext_SetShade(OBJgFlag_TextContext, IMcShade_Black);
		
		// get the flag's title
		OBJrObject_GetName(inObject, name, OBJcMaxNameLength);
		
		// draw the string to the texture
		TSrContext_DrawString(
			OBJgFlag_TextContext,
			OBJgFlag_Texture,
			name,
			&OBJgFlag_TextureBounds,
			&OBJgFlag_Dest);
		
		M3rGeom_State_Set(M3cGeomStateIntType_SpriteMode, M3cGeomState_SpriteMode_Normal);
		M3rGeom_State_Commit();

		// draw the sprite
		M3rSimpleSprite_Draw(
			OBJgFlag_Texture,
			inLocation,
			OBJgFlag_WidthRatio,
			1.0f,
			IMcShade_White,
			M3cMaxAlpha);
	}
	
	return;
}

// ----------------------------------------------------------------------
void
OBJrFlag_DrawTerminate(
	void)
{
	if (OBJgFlag_DrawingInitialized) {

		if (OBJgFlag_TextContext)
		{
			TSrContext_Delete(OBJgFlag_TextContext);
			OBJgFlag_TextContext = NULL;
		}

		OBJgFlag_DrawingInitialized = UUcFalse;
	}
}
#endif

// ----------------------------------------------------------------------
UUtUns16
OBJrFlag_GetUniqueID(
	void)
{
	ONtFlag					flag;
	UUtBool					found;
	
	OBJgFlag_ID++;
	
	// make sure the id_number isn't a duplicate
	found = ONrLevel_Flag_ID_To_Flag((UUtUns16)OBJgFlag_ID, &flag);
	while (found)
	{
		OBJgFlag_ID++;
		if (OBJgFlag_ID > UUcMaxInt16)
		{
			OBJgFlag_ID = 0;
		}

		found = ONrLevel_Flag_ID_To_Flag((UUtUns16)OBJgFlag_ID, &flag);
	}

	return (UUtUns16)OBJgFlag_ID;
}

// ----------------------------------------------------------------------
static void
OBJiFlag_SetFlagID(
	UUtUns16				inFlagID)
{
	OBJgFlag_ID = (UUtInt32)inFlagID;
}

// ----------------------------------------------------------------------
static UUtUns16
OBJiFlag_GetFlagPrefix(
	void)
{
	return OBJgFlag_Prefix;
}

// ----------------------------------------------------------------------
static void
OBJiFlag_SetFlagPrefix(
	UUtUns16				inFlagPrefix)
{
	OBJgFlag_Prefix = inFlagPrefix;
}

// ----------------------------------------------------------------------
static IMtShade
OBJiFlag_GetFlagShade(
	void)
{
	return OBJgFlag_Shade;
}

// ----------------------------------------------------------------------
static void
OBJiFlag_SetFlagShade(
	IMtShade				inFlagShade)
{
	OBJgFlag_Shade = inFlagShade;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiFlag_Delete(
	OBJtObject				*inObject)
{
}

// ----------------------------------------------------------------------
static void
OBJiFlag_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
#if TOOL_VERSION
	OBJtOSD_Flag			*flag_osd;
	M3tPoint3D				camera_location;
	
	M3tPoint3D points[4] = 
	{
		{ 0.0f,  0.0f,  0.0f },
		{ 0.0f, 10.0f,  0.0f },
		{ 0.0f,  8.0f,  4.0f },
		{ 0.0f,  6.0f,  0.0f }
	};
	
	if (OBJgFlag_DrawFlags == UUcFalse) { return; }
	
	// get a pointer to the object osd
	flag_osd = (OBJtOSD_Flag*)inObject->object_data;
	
	if ((OBJgFlag_ViewPrefix != 0) && (OBJgFlag_ViewPrefix != flag_osd->id_prefix)) { return; }
	
	// set up the matrix stack
	M3rMatrixStack_Push();
	M3rMatrixStack_ApplyTranslate(inObject->position);
	M3rMatrixStack_Multiply(&flag_osd->rotation_matrix);
	M3rGeom_State_Commit();
	
	// draw the flag
	M3rGeom_Line_Light(points + 0, points + 1, flag_osd->shade);
	M3rGeom_Line_Light(points + 1, points + 2, flag_osd->shade);
	M3rGeom_Line_Light(points + 2, points + 3, flag_osd->shade);
	
	// draw the name
	camera_location = CArGetLocation();
	if (MUrPoint_Distance(&inObject->position, &camera_location) < OBJgFlag_DrawNameDistance)
	{
		OBJiFlag_DrawName(inObject, points + 1);
	}
	
	// draw the rotation ring if this flag is selected
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
OBJiFlag_Enumerate(
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
OBJiFlag_GetBoundingSphere(
	const OBJtObject		*inObject,
	M3tBoundingSphere		*outBoundingSphere)
{
	outBoundingSphere->center.x = 0.0f;
	outBoundingSphere->center.y = 0.0f;
	outBoundingSphere->center.z = 0.0f;
	outBoundingSphere->radius = 5.0f;
}

// ----------------------------------------------------------------------
static void
OBJiFlag_OSDGetName(
	const OBJtOSD_All		*inOSD,
	char					*outName,
	UUtUns32				inNameLength)
{
	char buffer[256];
	const OBJtOSD_Flag			*flag_osd;
	char					char_1;
	char					char_2;

	// get a pointer to the object osd
	flag_osd = &inOSD->osd.flag_osd;
	
	// put the name into outName
	char_1 = UUmHighByte(flag_osd->id_prefix);
	char_2 = UUmLowByte(flag_osd->id_prefix);
	
	if ((char_1 == 0) && (char_2 == 0))
	{
		sprintf(buffer, "%d", flag_osd->id_number);
	}
	else if (char_1 == 0)
	{
		sprintf(buffer, "%c-%d", char_2, flag_osd->id_number);
	}
	else if (char_2 == 0)
	{
		sprintf(buffer, "%c-%d", char_1, flag_osd->id_number);
	}
	else
	{
		sprintf(buffer, "%c%c-%d", char_1, char_2, flag_osd->id_number);
	}

	UUrString_Copy(outName, buffer, inNameLength);

	return;
}

// ----------------------------------------------------------------------
static void
OBJiFlag_OSDSetName(
	OBJtOSD_All		*inOSD,
	const char		*inName)
{

	return;
}


// ----------------------------------------------------------------------
static void
OBJiFlag_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	OBJtOSD_Flag			*flag_osd;

	// get a pointer to the object osd
	flag_osd = (OBJtOSD_Flag*)inObject->object_data;
	
	outOSD->osd.flag_osd = *flag_osd;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiFlag_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	UUtUns32				size;
	
	size =
		sizeof(IMtShade) +
		sizeof(UUtUns16) +
		sizeof(UUtUns16) +
		(OBJcMaxNoteChars + 1);
	
	return size;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiFlag_IntersectsLine(
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



static void
OBJiFlag_GetUniqueOSD(
	OBJtOSD_All *inOSD)

{
	// initialize osd_all
	inOSD->osd.flag_osd.shade = OBJiFlag_GetFlagShade();
	inOSD->osd.flag_osd.id_prefix = OBJiFlag_GetFlagPrefix();
	inOSD->osd.flag_osd.id_number = OBJrFlag_GetUniqueID();
#if TOOL_VERSION
	inOSD->osd.flag_osd.note[0] = '\0';
#endif

	return;
}

// ----------------------------------------------------------------------
static UUtError
OBJiFlag_SetDefaults(
	OBJtOSD_All				*outOSD)
{
	OBJiFlag_GetUniqueOSD(outOSD);
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiFlag_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_All				osd_all;
	OBJtOSD_Flag			*flag_osd;
	UUtError				error;
	
	// get a pointer to the object osd
	flag_osd = (OBJtOSD_Flag*)inObject->object_data;
	
	if (inOSD == NULL)
	{
		error = OBJiFlag_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);
	}
	else
	{
		osd_all = *inOSD;
	}
	
	// set the object specific data and position
	error = OBJiFlag_SetOSD(inObject, &osd_all);
//	UUmError_ReturnOnError(error);
	
	OBJrObject_UpdatePosition(inObject);

	UUrMemory_Block_VerifyList();

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiFlag_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_All				osd_all;
	UUtUns32				num_bytes;
	
	// read the data
	OBJmGet4BytesFromBuffer(inBuffer, osd_all.osd.flag_osd.shade, UUtUns32, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, osd_all.osd.flag_osd.id_prefix, UUtUns16, inSwapIt);
	OBJmGet2BytesFromBuffer(inBuffer, osd_all.osd.flag_osd.id_number, UUtUns16, inSwapIt);
#if TOOL_VERSION
	OBJmGetStringFromBuffer(inBuffer, osd_all.osd.flag_osd.note, OBJcMaxNoteChars + 1, inSwapIt);
#endif

	num_bytes = sizeof(UUtUns32) + sizeof(UUtUns16) + sizeof(UUtUns16) + (OBJcMaxNoteChars + 1);
	
	// bring the object up to date
	OBJiFlag_SetOSD(inObject, &osd_all);
	OBJrObject_UpdatePosition(inObject);
	
	return num_bytes;
}

// ----------------------------------------------------------------------
static UUtError
OBJiFlag_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_Flag			*flag_osd;
	
	UUmAssert(inOSD);
	
	// get a pointer to the object osd
	flag_osd = (OBJtOSD_Flag*)inObject->object_data;
	
	// copy the from inOSD to flag_osd
	flag_osd->shade			= inOSD->osd.flag_osd.shade;
	flag_osd->id_prefix		= inOSD->osd.flag_osd.id_prefix;
	flag_osd->id_number		= inOSD->osd.flag_osd.id_number;
#if TOOL_VERSION
	UUrString_Copy(flag_osd->note, inOSD->osd.flag_osd.note, OBJcMaxNoteChars);
#endif
	
	// save the last used id number
	OBJiFlag_SetFlagShade(flag_osd->shade);
	OBJiFlag_SetFlagPrefix(flag_osd->id_prefix);
	OBJiFlag_SetFlagID(flag_osd->id_number);
	
	UUrMemory_Block_VerifyList();
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiFlag_UpdatePosition(
	OBJtObject				*inObject)
{
	OBJtOSD_Flag			*flag_osd;
	float					rot_x;
	float					rot_y;
	float					rot_z;
	MUtEuler				euler;
	
	// get a pointer to the object osd
	flag_osd = (OBJtOSD_Flag*)inObject->object_data;
	
	// convert the rotation to radians
	rot_x = inObject->rotation.x * M3cDegToRad;
	rot_y = inObject->rotation.y * M3cDegToRad;
	rot_z = inObject->rotation.z * M3cDegToRad;
	
	// update the rotation matrix
	euler.order = MUcEulerOrderXYZs;
	euler.x = rot_x;
	euler.y = rot_y;
	euler.z = rot_z;
	MUrEulerToMatrix(&euler, &flag_osd->rotation_matrix);
}

// ----------------------------------------------------------------------
static UUtError
OBJiFlag_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	OBJtOSD_Flag			*flag_osd;
	UUtUns32				bytes_available;

	// get a pointer to the object osd
	flag_osd = (OBJtOSD_Flag*)inObject->object_data;
	
	// set the number of bytes available
	bytes_available = *ioBufferSize;
	
	// put the shade, id_prefix, id_number, and note
	OBJmWrite4BytesToBuffer(ioBuffer, flag_osd->shade, UUtUns32, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, flag_osd->id_prefix, UUtUns16, bytes_available, OBJcWrite_Little);
	OBJmWrite2BytesToBuffer(ioBuffer, flag_osd->id_number, UUtInt16, bytes_available, OBJcWrite_Little);
#if TOOL_VERSION
	OBJmWriteStringToBuffer(ioBuffer, flag_osd->note, OBJcMaxNoteChars + 1, bytes_available, OBJcWrite_Little);
#else
	OBJmWriteStringToBuffer(ioBuffer, "", OBJcMaxNoteChars + 1, bytes_available, OBJcWrite_Little);
#endif
	
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
OBJiFlag_GetVisible(
	void)
{
	return OBJgFlag_DrawFlags;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiFlag_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	OBJtOSD_Flag			*flag_osd;
	UUtBool					found;
	
	// get a pointer to the object osd
	flag_osd = (OBJtOSD_Flag*)inObject->object_data;
	
	// perform the check
	found = UUcFalse;
	switch (inSearchType)
	{
		case OBJcSearch_FlagID:
			if (flag_osd->id_number == inSearchParams->osd.flag_osd.id_number)
			{
				found = UUcTrue;
			}
		break;
	}
	
	return found;
}

// ----------------------------------------------------------------------
static void
OBJiFlag_SetVisible(
	UUtBool					inIsVisible)
{
	OBJgFlag_DrawFlags = inIsVisible;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OBJiFlag_SetViewPrefix(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns8					char_1;
	UUtUns8					char_2;
	
	if (inParameterListLength > 1) { return UUcError_Generic; }
	if (inParameterList[0].type != SLcType_String) { return UUcError_Generic; }
	
	if (inParameterList[0].val.str == NULL)
	{
		OBJgFlag_ViewPrefix = 0;
	}
	else
	{
		char_1 = inParameterList[0].val.str[0];
		char_2 = inParameterList[0].val.str[1];
		
		if (char_2 == 0)
		{
			OBJgFlag_ViewPrefix = char_1;
		}
		else
		{
			OBJgFlag_ViewPrefix = UUmMakeShort(char_1, char_2);
		}
	}
	
	*outTicksTillCompletion = 0;
	*outStall = UUcFalse;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
OBJrFlag_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;
	
	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));
	
	// set up the methods structure
	methods.rNew				= OBJiFlag_New;
	methods.rSetDefaults		= OBJiFlag_SetDefaults;
	methods.rDelete				= OBJiFlag_Delete;
	methods.rDraw				= OBJiFlag_Draw;
	methods.rEnumerate			= OBJiFlag_Enumerate;
	methods.rGetBoundingSphere	= OBJiFlag_GetBoundingSphere;
	methods.rOSDGetName			= OBJiFlag_OSDGetName;
	methods.rOSDSetName			= OBJiFlag_OSDSetName;
	methods.rIntersectsLine		= OBJiFlag_IntersectsLine;
	methods.rUpdatePosition		= OBJiFlag_UpdatePosition;
	methods.rGetOSD				= OBJiFlag_GetOSD;
	methods.rGetOSDWriteSize	= OBJiFlag_GetOSDWriteSize;
	methods.rSetOSD				= OBJiFlag_SetOSD;
	methods.rRead				= OBJiFlag_Read;
	methods.rWrite				= OBJiFlag_Write;
	methods.rGetClassVisible	= OBJiFlag_GetVisible;
	methods.rSearch		= OBJiFlag_Search;
	methods.rSetClassVisible	= OBJiFlag_SetVisible;
	methods.rGetUniqueOSD		= OBJiFlag_GetUniqueOSD;
	
	// register the furniture methods
	error =
		OBJrObjectGroup_Register(
			OBJcType_Flag,
			OBJcTypeIndex_Flag,
			"Flag",
			sizeof(OBJtOSD_Flag),
			&methods,
			OBJcObjectGroupFlag_None);
	UUmError_ReturnOnError(error);
	
#if CONSOLE_DEBUGGING_COMMANDS
	// register the id set function
	error =
		SLrGlobalVariable_Register_Int32(
			"flag_new_id",
			"Specifies the id for a new flag",
			&OBJgFlag_ID);
	UUmError_ReturnOnError(error);

	error =
		SLrGlobalVariable_Register_Bool(
			"show_flags",
			"Enables the display of flags",
			&OBJgFlag_DrawFlags);
	UUmError_ReturnOnError(error);
	
	error =
		SLrGlobalVariable_Register_Float(
			"flag_name_distance",
			"Specifies the distance from the camera that flag names no longer draw.",
			&OBJgFlag_DrawNameDistance);
	UUmError_ReturnOnError(error);
	
	error =
		SLrScript_Command_Register_Void(
			"flag_view_prefix",
			"View flags with a specific prefix",
			"prefix:string",
			&OBJiFlag_SetViewPrefix);
	UUmError_ReturnOnError(error);
#endif

	// intialize the globals
	OBJgFlag_Shade = IMcShade_White;
	OBJgFlag_Prefix = 0;
	OBJgFlag_ID = 0;
	OBJgFlag_DrawNameDistance = OBJcFlag_DefaultDrawNameDistance;
	OBJgFlag_ViewPrefix = 0;
	
	return UUcError_None;
}
