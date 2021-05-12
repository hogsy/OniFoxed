// ======================================================================
// Imp_TextureFlags.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include <ctype.h>

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"
#include "BFW_TM_Construction.h"

#include "Imp_TextureFlags.h"

#define IMPcTexture_DefaultReflectivity_value	(0.25f)

// ======================================================================
// typedefs
// ======================================================================
typedef struct tFlagTable
{
	char					*flagName;
	UUtUns32				flagValue;
	
} tFlagTable;

// ======================================================================
// globals
// ======================================================================
tFlagTable gFlagTable[] =	
{
	{ "mipmap", M3cTextureFlags_HasMipMap },
	{ "trilinear", M3cTextureFlags_Trilinear },
	{ "clampHoriz", M3cTextureFlags_ClampHoriz },
	{ "clampVert", M3cTextureFlags_ClampVert },
	{ "pixelType", M3cTextureFlags_PixelType },
	{ "envmap", M3cTextureFlags_ReceivesEnvMap },
	{ "additive", M3cTextureFlags_Blend_Additive },
	{ "selfIlluminent", M3cTextureFlags_SelfIlluminent },
	{ "magicShield", M3cTextureFlags_MagicShield_Blue },
	{ "magicInvis", M3cTextureFlags_MagicShield_Invis },
	{ "magicDaodanShield", M3cTextureFlags_MagicShield_Daodan },
	{ NULL, 0}
};

tFlagTable gPixelTypeTable[] = 
{
	{ "argb4444",	IMcPixelType_ARGB4444 },
	{ "rgb555",		IMcPixelType_RGB555 },
	{ "argb1555",	IMcPixelType_ARGB1555 },
	{ "i8",			IMcPixelType_I8 },
	{ "i1",			IMcPixelType_I1 },
	{ "a8",			IMcPixelType_A8 },
	{ "a4i4",		IMcPixelType_A4I4 },
	{ "argb8888",	IMcPixelType_ARGB8888 },
	{ "rgb888",		IMcPixelType_RGB888 },
	{ NULL,			0 },
};

AUtFlagElement	IMPgAnimTextureFlags[] = 
{
	{
		"pingpong",
		M3cTextureFlags_Anim_PingPong
	},
	{
		"randomframe",
		M3cTextureFlags_Anim_RandomFrame
	},
	{
		"randomstart",
		M3cTextureFlags_Anim_RandomStart
	},
	{
		"noanimation",
		M3cTextureFlags_Anim_DontAnimate
	},
	{
		NULL,
		0
	}
};

static UUtUns16 iNameToValue(const char *inString)
{
	tFlagTable *entry;

	for(entry = gPixelTypeTable; entry->flagName != NULL; entry++) {
		if (UUmString_IsEqual_NoCase(inString, entry->flagName)) {
			UUmAssert(entry->flagValue < IMcNumPixelTypes);
			return (UUtUns16) (entry->flagValue);
		}
	}

	return IMcNumPixelTypes;
}


// ======================================================================
// functions
// ======================================================================

void
Imp_ProcessTextureFlags_FromFileRef(
	BFtFileRef*			inFileRef,
	tTextureFlags*		ioTextureFlags)
{
	UUtError			error;
	const char*			leafName;

	BFtFileRef			*flagFileRef;
	char				flagFileLeafName[BFcMaxFileNameLength * 2];
	
	GRtGroup_Context	*context;
	GRtGroup			*group;
	
	leafName = BFrFileRef_GetLeafName(inFileRef);
	
	UUrString_Copy(flagFileLeafName, leafName, BFcMaxFileNameLength * 2);
	flagFileLeafName[strlen(leafName) - 4] = '\0';
	UUrString_Cat(flagFileLeafName, "_ins.txt", BFcMaxFileNameLength * 2);

	if (cTextureFlagFile_Yes == ioTextureFlags->hasFlagFile)
	{
		error = BFrFileRef_DuplicateAndReplaceName(inFileRef, flagFileLeafName, &flagFileRef);
		if (UUcError_None != error)
		{
			flagFileRef = NULL;
		}
	}
	else 
	{
		flagFileRef = NULL;
	}

	if (flagFileRef != NULL)
	{
		if (BFrFileRef_FileExists(flagFileRef))
		{
			GRrGroup_Context_NewFromFileRef(flagFileRef, NULL, NULL, &context, &group);

			Imp_ProcessTextureFlags(group, BFrFileRef_GetLeafName(flagFileRef), ioTextureFlags);

			GRrGroup_Context_Delete(context);
		}

		BFrFileRef_Dispose(flagFileRef);
		flagFileRef = NULL;
	}
}



// ----------------------------------------------------------------------
void
Imp_ProcessTextureFlags(
	GRtGroup*			inGroup,
	const char*			inFileName,
	tTextureFlags		*outTextureFlags)
{
	UUtError			error;
	tFlagTable			*curFlag;
	char				*string;
	UUtUns32			tempu32;
	GRtElementType		elemType;
	GRtGroup*			animGroup;
	char*				elem;
	UUtUns32			effectFlags;
	M3tColorRGB			reflectivity_color;
	
	UUmAssert(inGroup);
	UUmAssert(outTextureFlags);
	
	outTextureFlags->timePerFrame = 1;
	outTextureFlags->numFrames = 1;
	outTextureFlags->reflectivity_specified = UUcFalse;
	outTextureFlags->reflectivity_color.r = IMPcTexture_DefaultReflectivity_value;
	outTextureFlags->reflectivity_color.g = IMPcTexture_DefaultReflectivity_value;
	outTextureFlags->reflectivity_color.b = IMPcTexture_DefaultReflectivity_value;
	
	outTextureFlags->envMapPlaceHolder = NULL;
	
	error = 
		M3rGroup_GetColor(
			inGroup,
			"reflectivity",
			&reflectivity_color);
	if(error == UUcError_None)
	{
		outTextureFlags->reflectivity_specified = UUcTrue;
		outTextureFlags->reflectivity_color = reflectivity_color;
	}
	
	error = 
		GRrGroup_GetElement(
			inGroup,
			"anim",
			&elemType,
			&animGroup);
	if(error == UUcError_None)
	{
		if(elemType == GRcElementType_Group)
		{
			outTextureFlags->timePerFrame = 1;
			error = 
				GRrGroup_GetElement(
					animGroup,
					"tpf",
					&elemType,
					&elem);
			if(error == UUcError_None)
			{
				if(elemType != GRcElementType_String)
				{
					AUrMessageBox(AUcMBType_OK, "illegal anim group, tpf field");
					return;
				}
				sscanf(elem, "%d", &tempu32);
				outTextureFlags->timePerFrame = (UUtUns16)tempu32;
			}
			error = 
				GRrGroup_GetElement(
					animGroup,
					"numFrames",
					&elemType,
					&elem);
			if(error != UUcError_None || elemType != GRcElementType_String)
			{
				AUrMessageBox(AUcMBType_OK, "illegal anim group, numFrames field");
				return;
			}
			else
			{
				sscanf(elem, "%d", &tempu32);
				outTextureFlags->numFrames = (UUtUns16)tempu32;
			}
			
			error = 
				GRrGroup_GetElement(
					animGroup,
					"effect",
					&elemType,
					&elem);
			if(error == UUcError_None)
			{
				error = 
					AUrFlags_ParseFromGroupArray(
						IMPgAnimTextureFlags,
						elemType,
						elem,
						&effectFlags);
				if(error != UUcError_None)
				{
					AUrMessageBox(AUcMBType_OK, "unknown texture effect, file: %s", inFileName);
					return;
				}

				outTextureFlags->flags |= effectFlags;
			}
		}
		else
		{
			AUrMessageBox(AUcMBType_OK, "illegal anim group");
		}
	}
	
	// process all of the possible flags
	for (curFlag = gFlagTable;
		 curFlag->flagName != NULL;
		 curFlag++)
	{
		switch (curFlag->flagValue)
		{	
			case M3cTextureFlags_PixelType:
			{
				UUtUns16	value;
				
				error = GRrGroup_GetString(inGroup, curFlag->flagName, &string);
				if (error == UUcError_None)
				{
					value = iNameToValue(string);
					UUmAssert(value < IMcNumPixelTypes);

					if (value >= IMcNumPixelTypes) {
						AUrMessageBox(AUcMBType_OK, "Flag %s is not a valid pixelType", string);
					}
					else {
						outTextureFlags->flags |= curFlag->flagValue;
						outTextureFlags->pixelType = (IMtPixelType)value;
					}
				}
			}
			break;

			default:
			{
				UUtBool		bool_val;
				
				error = GRrGroup_GetBool(inGroup, curFlag->flagName, &bool_val);
				if (error == UUcError_None)
				{	
					// if the flag exists, then override the existing value
					if (bool_val)
						outTextureFlags->flags |= curFlag->flagValue;
					else
						outTextureFlags->flags &= ~curFlag->flagValue;
				}
			}
			break;

		}
	}

	error = GRrGroup_GetString(inGroup, "envmapname", &string);
	if (error == UUcError_None)
	{
		if(string[0] == 0 || isspace(string[0]))
		{
			AUrMessageBox(AUcMBType_OK, "Illegal env map name: %s", inFileName);
		}
		else
		{
			char	mungedTextureName[BFcMaxFileNameLength];
			
			UUrString_Copy(mungedTextureName, string, BFcMaxFileNameLength);
			UUrString_StripExtension(mungedTextureName);
			
			error = 
				TMrConstruction_Instance_GetPlaceHolder(
					M3cTemplate_TextureMap,
					mungedTextureName,
					(TMtPlaceHolder*)&outTextureFlags->envMapPlaceHolder);
			if(error != UUcError_None)
			{
				AUrMessageBox(AUcMBType_OK, "Could not create place holder for %s", mungedTextureName);
			}
		}
	}
}	

void Imp_ClearTextureFlags(tTextureFlags *outFlags)
{
	outFlags->flags = 0;
	outFlags->hasFlagFile = cTextureFlagFile_No;
	outFlags->numFrames = 1;
	outFlags->timePerFrame = 1;
	outFlags->pixelType = IMcPixelType_RGB555;
	outFlags->reflectivity_specified = UUcFalse;
	outFlags->reflectivity_color.r = IMPcTexture_DefaultReflectivity_value;
	outFlags->reflectivity_color.g = IMPcTexture_DefaultReflectivity_value;
	outFlags->reflectivity_color.b = IMPcTexture_DefaultReflectivity_value;
	outFlags->envMapPlaceHolder = NULL;
}
