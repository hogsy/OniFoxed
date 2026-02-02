// ======================================================================
// OT_PowerUp.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"

#include "Oni_Object.h"
#include "Oni_Object_Private.h"

// ======================================================================
// typedefs
// ======================================================================
typedef struct OBJtPowerUpTypeNames
{
	WPtPowerupType			type;
	UUtUns32				type_chars;
	char					*type_name;

} OBJtPowerUpTypeNames;

// ======================================================================
// globals
// ======================================================================
static UUtBool				OBJgPowerUps_DrawPowerUps;

static OBJtPowerUpTypeNames	OBJgPowerUpTypeNames[] =
{
	{ WPcPowerup_AmmoBallistic,		UUm4CharToUns32('A', 'M', 'M', 'B'), 	"Ammo Ballistic" },
	{ WPcPowerup_AmmoEnergy,		UUm4CharToUns32('A', 'M', 'M', 'E'), 	"Ammo Energy" },
	{ WPcPowerup_Hypo,				UUm4CharToUns32('H', 'Y', 'P', 'O'), 	"Health Packet" },
	{ WPcPowerup_ShieldBelt,		UUm4CharToUns32('S', 'H', 'L', 'D'), 	"Shield Belt" },
	{ WPcPowerup_Invisibility,		UUm4CharToUns32('I', 'N', 'V', 'I'), 	"Invisibility" },
	{ WPcPowerup_LSI,				UUm4CharToUns32('A', 'L', 'S', 'I'), 	"LSI" },
	{ WPcPowerup_NumTypes,			UUm4CharToUns32('N', 'O', 'N', 'E'), 	NULL },
};


// ======================================================================
// prototypes
// ======================================================================
static UUtError
OBJiPowerUp_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiPowerUp_Delete(
	OBJtObject				*inObject)
{
	OBJtOSD_PowerUp			*powerup_osd;

	// get a pointer to the object osd
	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;
}

// ----------------------------------------------------------------------
static void
OBJiPowerUp_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
	OBJtOSD_PowerUp			*powerup_osd;
	M3tBoundingBox			bBox;

	// get a pointer to the object osd
	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;

	M3rMatrixStack_Push();
	M3rMatrixStack_ApplyTranslate(inObject->position);
	M3rMatrixStack_RotateZAxis(M3cHalfPi);
	M3rGeom_State_Commit();

	if ((powerup_osd->powerup) && (powerup_osd->powerup->geometry))
	{
		M3rMinMaxBBox_To_BBox(&powerup_osd->powerup->geometry->pointArray->minmax_boundingBox, &bBox);
		M3rBBox_Draw_Line(&bBox, IMcShade_White);
	}
	else
	{
		M3tBoundingBox_MinMax	min_max = {{-0.5f, 0.0f, -0.5f}, {0.5f, 0.5f, 0.5f}};
		M3rMinMaxBBox_To_BBox(&min_max, &bBox);
		M3rBBox_Draw_Line(&bBox, IMcShade_White);
	}

#if TOOL_VERSION
	// draw the bounding box if this is the selected object
	if ((inDrawFlags & OBJcDrawFlag_Selected) != 0)
	{
		M3tBoundingSphere		sphere;

		OBJrObject_GetBoundingSphere(inObject, &sphere);
		OBJrObjectUtil_DrawRotationRings(inObject, &sphere, inDrawFlags);
	}
#endif

	M3rMatrixStack_Pop();
}

// ----------------------------------------------------------------------
static UUtError
OBJiPowerUp_Enumerate(
	OBJtObject						*inObject,
	OBJtEnumCallback_ObjectName		inEnumCallback,
	UUtUns32						inUserData)
{
	OBJtPowerUpTypeNames			*type_name;

	for (type_name = OBJgPowerUpTypeNames; type_name->type_name != NULL; type_name++)
	{
		inEnumCallback(type_name->type_name, inUserData);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiPowerUp_GetBoundingSphere(
	const OBJtObject		*inObject,
	M3tBoundingSphere		*outBoundingSphere)
{
	OBJtOSD_PowerUp			*powerup_osd;

	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;
	if ((powerup_osd->powerup != NULL) && (powerup_osd->powerup->geometry != NULL))
	{
		*outBoundingSphere = powerup_osd->powerup->geometry->pointArray->boundingSphere;
	}
	else
	{
		MUmVector_Set(outBoundingSphere->center, 0.0f, 0.0f, 0.0f);
		outBoundingSphere->radius = 0.0f;
	}
}

// ----------------------------------------------------------------------
static void
OBJiPowerUp_OSDGetName(
	const OBJtOSD_All		*inOSD,
	char					*outName,
	UUtUns32				inNameLength)
{
	// return the name of the powerup type
	OBJtPowerUpTypeNames	*type_name;

	outName[0] = '\0';

	for (type_name = OBJgPowerUpTypeNames; type_name->type_name != NULL; type_name++)
	{
		if (type_name->type != inOSD->osd.powerup_osd.powerup_type) { continue; }

		UUrString_Copy(outName, type_name->type_name, inNameLength);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OBJiPowerUp_OSDSetName(
	OBJtOSD_All				*inOSD,
	const char				*inName)
{
}

// ----------------------------------------------------------------------
static void
OBJiPowerUp_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	OBJtOSD_PowerUp			*powerup_osd;

	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;

	outOSD->osd.powerup_osd = *powerup_osd;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiPowerUp_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	UUtUns32				size;
	OBJtOSD_PowerUp			*powerup_osd;

	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;

	// save the powerup type
	size =
		(sizeof(UUtUns8) * 4);				/* powerup type */

	return size;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiPowerUp_IntersectsLine(
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

	// do the fast test to see if the line is colliding with the bounding sphere
	return CLrSphere_Line(inStartPoint, inEndPoint, &sphere);
}

// ----------------------------------------------------------------------
static UUtError
OBJiPowerUp_SetDefaults(
	OBJtOSD_All				*outOSD)
{
	// clear the osd
	UUrMemory_Clear(&outOSD->osd.powerup_osd, sizeof(OBJtOSD_PowerUp));

	outOSD->osd.powerup_osd.powerup_type = WPcPowerup_AmmoBallistic;
	outOSD->osd.powerup_osd.powerup = NULL;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiPowerUp_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	UUtError				error;
	OBJtOSD_All				osd_all;

	if (inOSD == NULL)
	{
		error = OBJiPowerUp_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);

		// send osd_all to OBJiPowerUp_SetOSD()
		inOSD = &osd_all;
	}

	// set the object specific data and position
	OBJiPowerUp_SetOSD(inObject, inOSD);
	OBJrObject_UpdatePosition(inObject);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiPowerUp_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_PowerUp			*powerup_osd;
	UUtUns32				read_bytes;
	UUtUns8					chars[4];
	UUtUns32				i;
	UUtUns32				type;

	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;

	read_bytes = 0;

	// read the powerup type
	read_bytes += BDmGet4CharsFromBuffer(inBuffer, chars[0], chars[1], chars[2], chars[3]);
	type = UUm4CharToUns32(chars[0], chars[1], chars[2], chars[3]);

	for (i = 0; i < WPcPowerup_NumTypes; i++)
	{
		if (type != OBJgPowerUpTypeNames[i].type_chars) { continue; }

		powerup_osd->powerup_type = OBJgPowerUpTypeNames[i].type;
	}

	// set the osd and update the position
	OBJiPowerUp_SetOSD(inObject, (OBJtOSD_All*)inObject->object_data);
	OBJrObject_UpdatePosition(inObject);

	return read_bytes;
}

// ----------------------------------------------------------------------
static UUtError
OBJiPowerUp_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_PowerUp			*powerup_osd;

	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;

	powerup_osd->powerup_type = inOSD->osd.powerup_osd.powerup_type;
	powerup_osd->powerup = inOSD->osd.powerup_osd.powerup;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiPowerUp_UpdatePosition(
	OBJtObject				*inObject)
{
	OBJtOSD_PowerUp			*powerup_osd;

	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;
	if (powerup_osd->powerup)
	{
		powerup_osd->powerup->position = inObject->position;
		if (powerup_osd->powerup->geometry != NULL) {
			// move the powerup up so that the bottom of its geometry is sitting on the floor
			powerup_osd->powerup->position.y -= powerup_osd->powerup->geometry->pointArray->minmax_boundingBox.minPoint.y;
		}
		powerup_osd->powerup->rotation = inObject->rotation.x * M3cDegToRad;
	}

	// powerups don't do Y or Z rotation
	inObject->rotation.y = 0.0f;
	inObject->rotation.z = 0.0f;
}

// ----------------------------------------------------------------------
static UUtError
OBJiPowerUp_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	OBJtOSD_PowerUp			*powerup_osd;
	UUtUns32				bytes_available;
	OBJtPowerUpTypeNames	*type_name;

	powerup_osd = (OBJtOSD_PowerUp*)inObject->object_data;

	// set the number of bytes available
	bytes_available = *ioBufferSize;

	for (type_name = OBJgPowerUpTypeNames; type_name->type_name != NULL; type_name++)
	{
		char					chars[4];

		if (type_name->type != powerup_osd->powerup_type) { continue; }

		chars[0] = (UUtUns8)(type_name->type_chars >> 24);
		chars[1] = (UUtUns8)(type_name->type_chars >> 16);
		chars[2] = (UUtUns8)(type_name->type_chars >> 8);
		chars[3] = (UUtUns8)(type_name->type_chars);

		BDmWrite4CharsToBuffer(ioBuffer, chars[0], chars[1], chars[2], chars[3], bytes_available);
		break;
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
OBJiPowerUp_GetVisible(
	void)
{
	return OBJgPowerUps_DrawPowerUps;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiPowerUp_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static void
OBJiPowerUp_SetVisible(
	UUtBool					inIsVisible)
{
	OBJgPowerUps_DrawPowerUps = inIsVisible;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBJrPowerUp_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;

	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));

	// set up the methods structure
	methods.rNew				= OBJiPowerUp_New;
	methods.rSetDefaults		= OBJiPowerUp_SetDefaults;
	methods.rDelete				= OBJiPowerUp_Delete;
	methods.rDraw				= OBJiPowerUp_Draw;
	methods.rEnumerate			= OBJiPowerUp_Enumerate;
	methods.rGetBoundingSphere	= OBJiPowerUp_GetBoundingSphere;
	methods.rOSDGetName			= OBJiPowerUp_OSDGetName;
	methods.rOSDSetName			= OBJiPowerUp_OSDSetName;
	methods.rIntersectsLine		= OBJiPowerUp_IntersectsLine;
	methods.rUpdatePosition		= OBJiPowerUp_UpdatePosition;
	methods.rGetOSD				= OBJiPowerUp_GetOSD;
	methods.rGetOSDWriteSize	= OBJiPowerUp_GetOSDWriteSize;
	methods.rSetOSD				= OBJiPowerUp_SetOSD;
	methods.rRead				= OBJiPowerUp_Read;
	methods.rWrite				= OBJiPowerUp_Write;
	methods.rGetClassVisible	= OBJiPowerUp_GetVisible;
	methods.rSearch				= OBJiPowerUp_Search;
	methods.rSetClassVisible	= OBJiPowerUp_SetVisible;

	// register the furniture methods
	error =
		OBJrObjectGroup_Register(
			OBJcType_PowerUp,
			OBJcTypeIndex_PowerUp,
			"PowerUp",
			sizeof(OBJtOSD_PowerUp),
			&methods,
			OBJcObjectGroupFlag_None);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
WPtPowerupType
OBJrPowerUp_NameToType(
	const char				*inPowerUpName)
{
	OBJtPowerUpTypeNames	*type_name;
	WPtPowerupType			type;

	type = WPcPowerup_None;
	for (type_name = OBJgPowerUpTypeNames; type_name->type_name != NULL; type_name++)
	{
		UUtInt32			result;

		result = UUrString_Compare_NoCase(inPowerUpName, type_name->type_name);
		if (result == 0)
		{
			type = type_name->type;
			break;
		}
	}

	return type;
}

