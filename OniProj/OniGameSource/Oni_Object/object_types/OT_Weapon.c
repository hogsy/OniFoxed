// ======================================================================
// OT_Weapon.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"

#include "Oni_Object.h"
#include "Oni_Object_Private.h"
#include "Oni_Weapon.h"


// ======================================================================
// globals
// ======================================================================
static UUtBool				OBJgWeapons_DrawWeapons;

// ======================================================================
// prototypes
// ======================================================================
static UUtError
OBJiWeapon_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
OBJiWeapon_Delete(
	OBJtObject				*inObject)
{
/*	OBJtOSD_Weapon			*weapon_osd;

	// get a pointer to the object osd
	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;
	if (weapon_osd->weapon)
	{
		WPrDelete(weapon_osd->weapon);
		weapon_osd->weapon = NULL;
	}*/
}

// ----------------------------------------------------------------------
static void
OBJiWeapon_Draw(
	OBJtObject				*inObject,
	UUtUns32				inDrawFlags)
{
	OBJtOSD_Weapon			*weapon_osd;
	M3tBoundingBox			bBox;
	M3tGeometry				*geometry;

	// get a pointer to the object osd
	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;

	M3rMatrixStack_Push();
	M3rMatrixStack_ApplyTranslate(inObject->position);
	M3rGeom_State_Commit();

	if (weapon_osd->weapon_class)
	{
		geometry = weapon_osd->weapon_class->geometry;
		M3rMinMaxBBox_To_BBox(&geometry->pointArray->minmax_boundingBox, &bBox);
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
OBJiWeapon_Enumerate(
	OBJtObject						*inObject,
	OBJtEnumCallback_ObjectName		inEnumCallback,
	UUtUns32						inUserData)
{
	UUtError						error;
	UUtUns32						num_weapon_classes;
	WPtWeaponClass					*weapon_class_list[128];
	UUtUns32						i;

	error =
		TMrInstance_GetDataPtr_List(
			WPcTemplate_WeaponClass,
			128,
			&num_weapon_classes,
			weapon_class_list);
	UUmError_ReturnOnError(error);

	for (i = 0; i < num_weapon_classes; i++)
	{
		UUtBool						result;

		result =
			inEnumCallback(
				TMrInstance_GetInstanceName(weapon_class_list[i]),
				inUserData);
		if (result == UUcFalse) { break; }
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiWeapon_GetBoundingSphere(
	const OBJtObject		*inObject,
	M3tBoundingSphere		*outBoundingSphere)
{
	OBJtOSD_Weapon			*weapon_osd;

	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;
	if (weapon_osd->weapon_class != NULL)
	{
		M3tGeometry				*geometry;

		geometry = weapon_osd->weapon_class->geometry;
		*outBoundingSphere = geometry->pointArray->boundingSphere;
	}
	else
	{
		MUmVector_Set(outBoundingSphere->center, 0.0f, 0.0f, 0.0f);
		outBoundingSphere->radius = 2.5f;
	}
}

// ----------------------------------------------------------------------
static void
OBJiWeapon_OSDGetName(
	const OBJtOSD_All		*inOSD,
	char					*outName,
	UUtUns32				inNameLength)
{
	// return the name of the weapon class
	UUrString_Copy(outName, inOSD->osd.weapon_osd.weapon_class_name, inNameLength);
}

// ----------------------------------------------------------------------
static void
OBJiWeapon_OSDSetName(
	OBJtOSD_All				*inOSD,
	const char				*inName)
{
}

// ----------------------------------------------------------------------
static void
OBJiWeapon_GetOSD(
	const OBJtObject		*inObject,
	OBJtOSD_All				*outOSD)
{
	OBJtOSD_Weapon			*weapon_osd;

	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;

	outOSD->osd.weapon_osd = *weapon_osd;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiWeapon_GetOSDWriteSize(
	const OBJtObject		*inObject)
{
	UUtUns32				size;
	OBJtOSD_Weapon			*weapon_osd;

	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;

	// save the weapon type
	size =
		WPcMaxWeaponName;				/* weapon class name */

	return size;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiWeapon_IntersectsLine(
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
OBJiWeapon_SetDefaults(
	OBJtOSD_All				*outOSD)
{
	// clear the osd
	UUrMemory_Clear(&outOSD->osd.weapon_osd, sizeof(OBJtOSD_Weapon));

	UUrString_Copy(outOSD->osd.weapon_osd.weapon_class_name, "w1_tap", WPcMaxWeaponName);
//	outOSD->osd.weapon_osd.weapon = NULL;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OBJiWeapon_New(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	UUtError				error;
	OBJtOSD_All				osd_all;

	if (inOSD == NULL)
	{
		error = OBJiWeapon_SetDefaults(&osd_all);
		UUmError_ReturnOnError(error);

		// send osd_all to OBJiWeapon_SetOSD()
		inOSD = &osd_all;
	}

	// set the object specific data and position
	OBJiWeapon_SetOSD(inObject, inOSD);
	OBJrObject_UpdatePosition(inObject);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtUns32
OBJiWeapon_Read(
	OBJtObject				*inObject,
	UUtUns16				inVersion,
	UUtBool					inSwapIt,
	UUtUns8					*inBuffer)
{
	OBJtOSD_Weapon			*weapon_osd;
	UUtUns32				read_bytes;

	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;

	read_bytes = 0;

	// read the weapon type
	UUrString_Copy(weapon_osd->weapon_class_name, (char*)inBuffer, WPcMaxWeaponName);
	inBuffer += WPcMaxWeaponName;
	read_bytes += WPcMaxWeaponName;

	// set the osd and update the position
	OBJiWeapon_SetOSD(inObject, (OBJtOSD_All*)inObject->object_data);
	OBJrObject_UpdatePosition(inObject);

	return read_bytes;
}

// ----------------------------------------------------------------------
static UUtError
OBJiWeapon_SetOSD(
	OBJtObject				*inObject,
	const OBJtOSD_All		*inOSD)
{
	OBJtOSD_Weapon			*weapon_osd;
//	WPtWeaponClass			*weapon_class;
	UUtError				error;

	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;
/*	if (weapon_osd->weapon)
	{
		// delete the old weapon
		WPrDelete(weapon_osd->weapon);
		weapon_osd->weapon = NULL;
	}*/

	UUrString_Copy(
		weapon_osd->weapon_class_name,
		inOSD->osd.weapon_osd.weapon_class_name,
		WPcMaxWeaponName);

	error =
		TMrInstance_GetDataPtr(
			WPcTemplate_WeaponClass,
			weapon_osd->weapon_class_name,
			&weapon_osd->weapon_class);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OBJiWeapon_UpdatePosition(
	OBJtObject				*inObject)
{
	OBJtOSD_Weapon			*weapon_osd;

	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;

	// weapons don't do Y or Z rotation
	inObject->rotation.y = 0.0f;
	inObject->rotation.z = 0.0f;
}

// ----------------------------------------------------------------------
static UUtError
OBJiWeapon_Write(
	OBJtObject				*inObject,
	UUtUns8					*ioBuffer,
	UUtUns32				*ioBufferSize)
{
	OBJtOSD_Weapon			*weapon_osd;
	UUtUns32				bytes_available;
//	WPtWeaponClass			*weapon_class;
//	char					*weapon_class_name;

	weapon_osd = (OBJtOSD_Weapon*)inObject->object_data;

	// set the number of bytes available
	bytes_available = *ioBufferSize;

//	weapon_class = WPrGetClass(weapon_osd->weapon);
//	weapon_class_name = TMrInstance_GetInstanceName(weapon_class);

	UUrString_Copy((char*)ioBuffer, weapon_osd->weapon_class_name, WPcMaxWeaponName);
	ioBuffer += WPcMaxWeaponName;
	bytes_available -= WPcMaxWeaponName;

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
OBJiWeapon_GetVisible(
	void)
{
	return OBJgWeapons_DrawWeapons;
}

// ----------------------------------------------------------------------
static UUtBool
OBJiWeapon_Search(
	const OBJtObject		*inObject,
	const UUtUns32			inSearchType,
	const OBJtOSD_All		*inSearchParams)
{
	return UUcFalse;
}

// ----------------------------------------------------------------------
static void
OBJiWeapon_SetVisible(
	UUtBool					inIsVisible)
{
	OBJgWeapons_DrawWeapons = inIsVisible;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OBJrWeapon_Initialize(
	void)
{
	UUtError				error;
	OBJtMethods				methods;

	// clear the methods structure
	UUrMemory_Clear(&methods, sizeof(OBJtMethods));

	// set up the methods structure
	methods.rNew				= OBJiWeapon_New;
	methods.rSetDefaults		= OBJiWeapon_SetDefaults;
	methods.rDelete				= OBJiWeapon_Delete;
	methods.rDraw				= OBJiWeapon_Draw;
	methods.rEnumerate			= OBJiWeapon_Enumerate;
	methods.rGetBoundingSphere	= OBJiWeapon_GetBoundingSphere;
	methods.rOSDGetName			= OBJiWeapon_OSDGetName;
	methods.rOSDSetName			= OBJiWeapon_OSDSetName;
	methods.rIntersectsLine		= OBJiWeapon_IntersectsLine;
	methods.rUpdatePosition		= OBJiWeapon_UpdatePosition;
	methods.rGetOSD				= OBJiWeapon_GetOSD;
	methods.rGetOSDWriteSize	= OBJiWeapon_GetOSDWriteSize;
	methods.rSetOSD				= OBJiWeapon_SetOSD;
	methods.rRead				= OBJiWeapon_Read;
	methods.rWrite				= OBJiWeapon_Write;
	methods.rGetClassVisible	= OBJiWeapon_GetVisible;
	methods.rSearch				= OBJiWeapon_Search;
	methods.rSetClassVisible	= OBJiWeapon_SetVisible;

	// register the furniture methods
	error =
		OBJrObjectGroup_Register(
			OBJcType_Weapon,
			OBJcTypeIndex_Weapon,
			"Weapon",
			sizeof(OBJtOSD_Weapon),
			&methods,
			OBJcObjectGroupFlag_None);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
