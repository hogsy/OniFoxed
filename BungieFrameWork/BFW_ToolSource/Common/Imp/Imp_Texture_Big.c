// ======================================================================
// Imp_Texture_Big.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_FileFormat.h"
#include "BFW_TM_Construction.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"
#include "BFW_Motoko.h"
#include "BFW_Materials.h"

#include "Imp_Common.h"
#include "Imp_Texture.h"


#include <ctype.h>

// ======================================================================
// prototypes
// ======================================================================
static UUtError
BTiTextureMap_New(
	UUtUns16			inWidth,
	UUtUns16			inHeight,
	IMtPixelType		inTexelType,
	char				*inInstanceName,
	M3tTextureMap		**outTextureMap,
	void				**outSeparateData);

static UUtError
BTiIsValidTextureMapSize(
	UUtUns32			inWidth,
	UUtUns32			inHeight);
	
// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
BTiTextureMap_New(
	UUtUns16			inWidth,
	UUtUns16			inHeight,
	IMtPixelType		inTexelType,
	char				*inInstanceName,
	M3tTextureMap		**outTextureMap,
	void				**outSeparateData)
{
	M3tTextureMap		*new_texture_map;
	void				*separateData;
	UUtUns16			width;
	UUtUns16			height;
	UUtError			error;
	UUtUns8				size_of_texel;
	UUtUns32			total_map_bytes;
	
	// make sure that the parameters are good
	UUmAssert(inWidth > 0);
	UUmAssert(inHeight > 0);
	UUmAssertReadPtr(outTextureMap, sizeof(*outTextureMap));
	
	// calculate the width, height, and rowbytes of the texture
	M3rTextureMap_GetTextureSize(inWidth, inHeight, &width, &height);

	size_of_texel = IMrPixel_GetSize(inTexelType);
	total_map_bytes = width * height * size_of_texel;
	
	// create a new instance of the texture
	error =
		TMrConstruction_Instance_Renew(
			M3cTemplate_TextureMap,
			inInstanceName,
			0,
			&new_texture_map);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new texture.");
	
	// set up the texture map
	if (NULL != inInstanceName) { strncpy(new_texture_map->debugName, inInstanceName, 128); }
	else { strncpy(new_texture_map->debugName, "imported unique big texture", 128); }

	new_texture_map->width = width;
	new_texture_map->height = height;
	new_texture_map->texelType = inTexelType;
	new_texture_map->flags = M3cTextureFlags_ClampHoriz | M3cTextureFlags_ClampVert;
	new_texture_map->animation = NULL;
	new_texture_map->envMap = NULL;
	new_texture_map->materialType = MAcMaterial_Base;
	new_texture_map->pixels = NULL;
	new_texture_map->pixelStorage = TMcSeparateFileOffset_None;
	separateData = NULL;

#if 0
	// CB: perhaps we should use some criteria to decide whether certain textures should be permanently resident?
	new_texture_map->pixels = TMrConstruction_Raw_New(total_map_bytes, 16, M3cTemplate_TextureMap);
#else
	separateData = TMrConstruction_Separate_New(total_map_bytes, M3cTemplate_TextureMap);
#endif
	
	// save the outgoing variables
	*outTextureMap = new_texture_map;
	*outSeparateData = separateData;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
BTiIsValidTextureMapSize(
	UUtUns32			inWidth,
	UUtUns32			inHeight)
{
	UUtError result;

	// set results initial value
	result = UUcError_None;
	
	// check width and height
	switch(inWidth)
	{
		case 1600:
		case 1200:
		case 1024:
		case 800:
		case 768:
		case 512:
		case 256:
		case 128:
		case 64:
		case 32:
		case 16:
		case 8:
		case 4:
		case 2:
		case 1:
		break;

		default:
			result = UUcError_Generic;
	}

	switch(inHeight)
	{
		case 1200:
		case 1024:
		case 800:
		case 768:
		case 512:
		case 256:
		case 128:
		case 64:
		case 32:
		case 16:
		case 8:
		case 4:
		case 2:
		case 1:
		break;

		default:
			result = UUcError_Generic;
	}

	// aspect ratio
	if ((inWidth * 8) < inHeight)
	{
		result = UUcError_Generic;
	}

	// aspect ratio
	if ((inHeight * 8) < inWidth)
	{
		result = UUcError_Generic;
	}

	return result;
}

// ----------------------------------------------------------------------
static UUtError
Imp_ProcessTexture_Big(
	BFtFileRef*			inFileRef,
	const tTextureFlags	*inDefaultTextureFlags,
	const char*			inInstanceName)
{
	UUtError			error;
	UUtBool				file_exists;
	UUtBool				build_instance;
	UUtUns16			width;
	UUtUns16			height;
	IMtMipMap			mipMap;
	UUtUns16			row_bytes;
	void				*image_data;
	void				*separate_data;
	const char			*leaf_name;
	M3tTextureMap_Big	*big_texture;
	
	UUtUns16			num_x;
	UUtUns16			num_y;
	
	UUtUns16			x;
	UUtUns16			y;
	
	char				texture_group_name[BFcMaxFileNameLength * 2];
	
	BFtFileRef			*flagFileRef;
	char				flagFileLeafName[BFcMaxFileNameLength * 2];
	
	tTextureFlags		local_flags;
	IMtPixelType		pixel_type;
	
	GRtGroup_Context	*context;
	GRtGroup			*group;
	
	FFtFileInfo			file_info;
	
	// copy the default flags
	local_flags = *inDefaultTextureFlags;
	
	// find out if the file exists
	do 
	{
		// get the name of the file without the path
		leaf_name = BFrFileRef_GetLeafName(inFileRef);
		
		// see if the file exists
		file_exists = BFrFileRef_FileExists(inFileRef);
		
		if (file_exists == UUcFalse)
		{
			AUtMB_ButtonChoice choice;
			
			choice = 
				AUrMessageBox(
					AUcMBType_AbortRetryIgnore,
					"Could not find file %s."UUmNL,
					leaf_name);
			
			if (choice == AUcMBChoice_Abort)
			{
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find texture file");
			}
			else if (choice == AUcMBChoice_Ignore)
			{
				return UUcError_None;
			}
		}
	}
	while (!file_exists);
	
	// find out if the instance exists
	build_instance =
		!TMrConstruction_Instance_CheckExists(
			M3cTemplate_TextureMap_Big,
			inInstanceName);

	if (!build_instance)
	{
		return UUcError_None;
	}
	
	// get the file info
	error = FFrPeek_2D(inFileRef, &file_info);
	UUmError_ReturnOnErrorMsg(error, "Could not get file info.");
	
	// set the default pixel format
	if (file_info.format == FFcFormat_2D_PSD)
	{
		pixel_type = IMcPixelType_ARGB4444;
	}
	else
	{
		pixel_type = IMcPixelType_RGB555;
	}
	
	// process the flags file
	UUrString_Copy(flagFileLeafName, leaf_name, BFcMaxFileNameLength * 2);
	flagFileLeafName[strlen(leaf_name) - 4] = '\0';
	UUrString_Cat(flagFileLeafName, "_ins.txt", BFcMaxFileNameLength * 2);

	if (cTextureFlagFile_Yes == local_flags.hasFlagFile)
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

			Imp_ProcessTextureFlags(group, BFrFileRef_GetLeafName(flagFileRef), &local_flags);

			GRrGroup_Context_Delete(context);
		}

		BFrFileRef_Dispose(flagFileRef);
		flagFileRef = NULL;
	}

	// set the pixel type
	if (local_flags.flags & M3cTextureFlags_PixelType)
	{
		pixel_type = local_flags.pixelType;
	}
	
	// read the data from the file
	error = 
		FFrRead_2D(
			inFileRef,
			pixel_type,
			&width,
			&height,
			&mipMap,
			&image_data);
	UUmError_ReturnOnError(error);
	
	row_bytes = IMrImage_ComputeRowBytes(pixel_type, width);

/*	error = BTiIsValidTextureMapSize(width, height);
	if(error != UUcError_None)
	{
		Imp_PrintWarning(
			"File %s had an invalid texture map size (%d, %d)"UUmNL"(1 to 256 and 8x1 ratio only)", 
			BFrFileRef_GetLeafName(inFileRef),
			width,
			height);
		
		return error;
	}*/
	
	// calculate the number of M3tTextureMaps it will take to hold the texture
	num_x = width / M3cTextureMap_MaxWidth;
	num_y = height / M3cTextureMap_MaxHeight;
	
	if ((width & (M3cTextureMap_MaxWidth - 1)) > 0) num_x++;
	if ((height & (M3cTextureMap_MaxHeight - 1)) > 0) num_y++;
	
	// Create the texture map instance
	error =
		TMrConstruction_Instance_Renew(
			M3cTemplate_TextureMap_Big,
			inInstanceName,
			num_x * num_y,
			(void **)&big_texture);
	IMPmError_ReturnOnError(error);
	
	big_texture->texelType	= pixel_type;
	big_texture->width		= width;
	big_texture->height		= height;
	big_texture->num_x		= num_x;
	big_texture->num_y		= num_y;
	
	// set the texture_group_name
	UUrString_Copy(texture_group_name, leaf_name, BFcMaxFileNameLength * 2);
	texture_group_name[strlen(leaf_name) - 4] = '\0';
	
	// Copy the image into the texture maps
	for (y = 0; y < num_y; y++)
	{
		for (x = 0; x < num_x; x++)
		{
			UUtUns16			left;
			UUtUns16			top;
			UUtUns16			texture_width;
			UUtUns16			texture_height;
			
			UUtUns16			index;
			
			M3tTextureMap		*texture;
			char				texture_name[BFcMaxFileNameLength * 2];
			
			// calculate the index number of the texture
			index = x + (y * num_x);
						
			// calculate the texture width and height
			left			= (x * M3cTextureMap_MaxWidth);
			top				= (y * M3cTextureMap_MaxHeight);
			texture_width	= UUmMin(M3cTextureMap_MaxWidth, width - left);
			texture_height	= UUmMin(M3cTextureMap_MaxHeight, height - top);
			
			// set the texture_name
			sprintf(texture_name, "%s_%d", texture_group_name, index);
			
			// create a new texture
			error =
				BTiTextureMap_New(
					texture_width,
					texture_height,
					big_texture->texelType,
					texture_name,
					&texture,
					&separate_data);
			IMPmError_ReturnOnErrorMsg(error, "Unable to create texture map");
			
			// save the texture pointer
			big_texture->textures[index] = texture;
						
			// copy the data from the image_data to the texture
			{
				UUtUns16			w;
				UUtUns16			h;
				
				UUtUns16			texel_size;
			
				UUtUns8				*src;
				UUtUns32			src_row_bytes;
				
				UUtUns8				*dst1, *dst2;
				UUtUns32			dst_row_bytes;
					
				// get the texel size
				texel_size = (UUtUns16)IMrPixel_GetSize(big_texture->texelType);
				
				// set the src
				src_row_bytes = row_bytes;
				src = 
					(UUtUns8*)image_data +
					(top * src_row_bytes) +
					(left * texel_size);
				
				// set the dst
				dst_row_bytes = texture->width * texel_size;
				dst1 = (UUtUns8*)texture->pixels;
				dst2 = (UUtUns8*)separate_data;
				
				w = texture_width * texel_size;
				h = texture_height;
				
				while (h--)
				{
					if (dst1 != NULL) {
						UUrMemory_MoveFast(src, dst1, w);
						dst1 += dst_row_bytes;
					}

					if (dst2 != NULL) {
						UUrMemory_MoveFast(src, dst2, w);
						dst2 += dst_row_bytes;
					}
					
					// advance to next line
					src += src_row_bytes;
				}
			}

			if (texture->pixels != NULL) {
				texture->pixels = TMrConstruction_Raw_Write(texture->pixels);
			}
			if (separate_data != NULL) {
				texture->pixelStorage = (TMtSeparateFileOffset) TMrConstruction_Separate_Write(separate_data);
			}

			#if UUmEndian == UUmEndian_Little
			texture->flags |= M3cTextureFlags_LittleEndian;
			#endif
		}
	}
	
	// dispose of the image_data
	UUrMemory_Block_Delete(image_data);
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
Imp_AddTexture_Big(
	BFtFileRef			*inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup			*inGroup,
	char				*inInstanceName)
{
	UUtError			error;
	BFtFileRef*			file;
	tTextureFlags		flags;
	
	// get a reference to the file
	error =
		Imp_Common_GetFileRefAndModDate(
			inSourceFileRef,
			inGroup,
			"file",
			&file);
	IMPmError_ReturnOnErrorMsg(error, "Could not get file name");
	
	// set the flags
	Imp_ClearTextureFlags(&flags);
	flags.hasFlagFile = cTextureFlagFile_No;
	flags.flags = M3cTextureFlags_None;
	
	// process the texture
	error =
		Imp_ProcessTexture_Big(
			file,
			&flags,
			inInstanceName);
	
	// dispose of the file reference
	BFrFileRef_Dispose(file);

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
Imp_ProcessTexture_Big_File(
	BFtFileRef			*inFileRef,
	const char			*inInstanceName,
	TMtPlaceHolder		*outTextureRef)
{
	UUtError			error;
	UUtUns32			file_mod_date;
	tTextureFlags		flags;
	
	UUmAssert(inFileRef);
	UUmAssert(outTextureRef);
	UUmAssert(inInstanceName);
	
	// get the modification time of the file
	error = BFrFileRef_GetModTime(inFileRef, &file_mod_date);
	IMPmError_ReturnOnErrorMsg(error, "Could not get file ref mod date");
		
	// set the texture flags
	Imp_ClearTextureFlags(&flags);
	flags.hasFlagFile = cTextureFlagFile_Yes;
	flags.flags = M3cTextureFlags_None;
	
	// process M3cTextureMap_Big
	error =
		Imp_ProcessTexture_Big(
			inFileRef,
			&flags,
			inInstanceName);
	IMPmError_ReturnOnError(error);
	
	// get a placeholder for the M3tTextureMap_Big instance of texture_name
	error =
		TMrConstruction_Instance_GetPlaceHolder(
			M3cTemplate_TextureMap_Big,
			inInstanceName,
			outTextureRef);
	IMPmError_ReturnOnError(error);
	
	return UUcError_None;
}