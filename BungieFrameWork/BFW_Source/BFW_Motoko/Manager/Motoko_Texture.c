// ======================================================================
// Motoko_Texture.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW_Motoko.h"
#include "BFW_TextSystem.h"

#include "Motoko_Manager.h"
#include "Motoko_Private.h"

// ======================================================================
// globals
// ======================================================================

M3tManagerTextureData		M3gManager_TextureStorage;

// ======================================================================
// prototypes
// ======================================================================
static UUtUns16
M3iTextureMap_CalcNearestPowerOfTwo(
	UUtUns16				inSize);

void
M3rTextureMap_GetTextureSize(
	UUtUns16				inWidth,
	UUtUns16 				inHeight,
	UUtUns16 				*outWidth,
	UUtUns16 				*outHeight);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------

static void
M3iTextureByteSwapper(
	M3tTextureMap*			inTextureMap,
	void*					ioTextureMapData)
{
	UUtUns32*				lp;
	UUtUns16*				sp;
	UUtUns32				itr;
	UUtUns32				numTexels;
	IMtMipMap				hasMipMap = (inTextureMap->flags & M3cTextureFlags_HasMipMap) ? IMcHasMipMap : IMcNoMipMap;

#if defined(DEBUGGING) && DEBUGGING
	{
		static UUtBool debug_mipmap_swapping_problems = UUcFalse;

		if (debug_mipmap_swapping_problems && UUmString_IsEqual(inTextureMap->debugName, "HQ_CONCRETE")) {
			UUrEnterDebugger("at problem texture swapping");
		}
	}
#endif

	if (inTextureMap->texelType == IMcPixelType_DXT1) {
		// we swap in terms of DXT1 blocks which are 4x4 (2^2)
		numTexels = IMrImage_NumPixelBlocks(hasMipMap, inTextureMap->width, inTextureMap->height, 2);
	} else {
		// swap texels individually
		numTexels = IMrImage_NumPixels(hasMipMap, inTextureMap->width, inTextureMap->height);
	}

	switch(inTextureMap->texelType)
	{
		case IMcPixelType_DXT1:
			// CB: we swap 4 16-bit numbers for each DXT1 block
			sp = (UUtUns16 *) ioTextureMapData;
			for(itr = 0; itr < (numTexels << 2); itr++)
			{
				UUrSwap_2Byte(sp++);
			}
			break;

		case IMcPixelType_ARGB4444:
		case IMcPixelType_RGB555:
		case IMcPixelType_ARGB1555:
			// needs 2 byte swapping
			sp = (UUtUns16 *) ioTextureMapData;
			for(itr = 0; itr < numTexels; itr++)
			{
				UUrSwap_2Byte(sp++);
			}
			break;

		case IMcPixelType_I8:
		case IMcPixelType_I1:
		case IMcPixelType_A8:
		case IMcPixelType_A4I4:
			// does not need swapping
			break;

		case IMcPixelType_ARGB8888:
		case IMcPixelType_RGB888:
			// needs 4 byte swapping
			lp = (UUtUns32 *) ioTextureMapData;
			for(itr = 0; itr < numTexels; itr++)
			{
				UUrSwap_4Byte(lp++);
			}
			break;

		default:
			UUmAssert(!"Unknown texture format");
	}

	return;
}


void
M3rTextureMap_Prepare_Internal(
	M3tTextureMap			*ioTextureMap)
{
	if (0 == (ioTextureMap->flags & M3cTextureFlags_Prepared)) {
		if (ioTextureMap->pixels != NULL) {
			// CB: this texturemap has permanently-resident data that must be byte-swapped now
			void *offset;
			UUtBool swap;

			// CB: note that I changed this code so that it no longer sets the flag
			// M3cTextureFlags_LittleEndian after it byte-swaps a texture. this means
			// that we will incorrectly double byte-swap if someone clears the
			// M3cTextureFlags_Prepared flag and then calls this routine again (this
			// should not ever be done).
			//
			// this change was necessary because the separate-data loading code might
			// potentially have to load the same texturemap multiple times.
			#if UUmEndian == UUmEndian_Little
			swap = ((ioTextureMap->flags & M3cTextureFlags_LittleEndian) == 0);
			#else
			swap = ((ioTextureMap->flags & M3cTextureFlags_LittleEndian) > 0);
			#endif

			offset = TMrInstance_GetRawOffset(ioTextureMap);
			ioTextureMap->pixels = (void *) (((UUtUns32) ioTextureMap->pixels) + ((UUtUns32) offset));

			if (swap) {
				M3iTextureByteSwapper(ioTextureMap, ioTextureMap->pixels);
			}
		}

		ioTextureMap->flags |= M3cTextureFlags_Prepared;
	}

	return;
}

void
M3rTextureMap_TemporarilyLoad_Internal(
	M3tTextureMap			*ioTextureMap,
	UUtUns32				inSkipLargeLODs)
{
	if ((ioTextureMap->pixels == NULL) && (ioTextureMap->pixelStorage != TMcSeparateFileOffset_None)){
		BFtFile *separate_data_file;
		UUtUns32 read_location;
		UUtUns32 read_size;
		UUtUns32 data_size;
		UUtUns32 skip_data_size;
		IMtMipMap hasMipMap;
		UUtError error;
		void *dest_pointer;

		separate_data_file = TMrInstance_GetSeparateFile(ioTextureMap);
		if (separate_data_file != NULL) {
			// CB: we can never have two textures loaded into temporary space at once
			UUmAssert(M3gManager_TextureStorage.temporaryTexture == NULL);

			// determine how much data we want to load
			hasMipMap = (ioTextureMap->flags & M3cTextureFlags_HasMipMap) ? IMcHasMipMap : IMcNoMipMap;
			data_size = IMrImage_ComputeSize(ioTextureMap->texelType, hasMipMap, ioTextureMap->width, ioTextureMap->height);
			UUmAssert(data_size < M3gManager_TextureStorage.temporarySize);

			skip_data_size = 0;
			if ((hasMipMap == IMcHasMipMap) && (inSkipLargeLODs > 0)) {
				UUtUns16 width = ioTextureMap->width;
				UUtUns16 height = ioTextureMap->height;

				while ((inSkipLargeLODs > 0) && (width > 1) && (height > 1)) {
					skip_data_size += IMrImage_ComputeSize(ioTextureMap->texelType, IMcNoMipMap, width, height);

					inSkipLargeLODs--;
					width >>= 1;
					height >>= 1;
					width = UUmMax(width, 1);
					height = UUmMax(height, 1);
				}
			}

			// don't bother reading any large LODs that will be skipped
			if (skip_data_size == 0) {
				read_location = ioTextureMap->pixelStorage;
				read_size = data_size;
				dest_pointer = M3gManager_TextureStorage.temporaryStorage;
			} else {
				UUmAssert(skip_data_size < data_size);
				read_location = ioTextureMap->pixelStorage + skip_data_size;
				read_size = data_size - skip_data_size;
				dest_pointer = (void *) (((UUtUns8 *) M3gManager_TextureStorage.temporaryStorage) + skip_data_size);
			}

			// read the texture's data from disk
			error = BFrFile_ReadPos(separate_data_file, read_location, read_size, dest_pointer);

			if (error == UUcError_None) {
				UUtBool swap;

				// set up the texture so that it points to the temporary storage
				ioTextureMap->pixels = (void *) M3gManager_TextureStorage.temporaryStorage;
				M3gManager_TextureStorage.temporaryTexture = ioTextureMap;

				#if UUmEndian == UUmEndian_Little
				swap = ((ioTextureMap->flags & M3cTextureFlags_LittleEndian) == 0);
				#else
				swap = ((ioTextureMap->flags & M3cTextureFlags_LittleEndian) > 0);
				#endif

				if (swap) {
					M3iTextureByteSwapper(ioTextureMap, ioTextureMap->pixels);
				}
			}
		}
	}
}

void
M3rTextureMap_UnloadTemporary(
	M3tTextureMap			*ioTextureMap)
{
	if (M3gManager_TextureStorage.temporaryTexture == ioTextureMap) {
		M3gManager_TextureStorage.temporaryTexture = NULL;
		ioTextureMap->pixels = NULL;
	}
}

UUtError
M3rTextureMap_New(
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	IMtPixelType			inTexelType,
	UUtUns32				inAllocMemory,
	UUtUns32				inFlags,
	const char				*inDebugName,
	M3tTextureMap			**outTextureMap)
{
	M3tTextureMap			*new_texture_map;
	UUtUns16				width;
	UUtUns16				height;
	UUtError				error;
	UUtUns32				size_of_texture;
	IMtMipMap				hasMipMap;
	TMtDynamicPool_Type		memPool= (inAllocMemory & M3cTexture_UseTempMem) ? TMcDynamicPool_Type_Temporary : TMcDynamicPool_Type_Permanent;

	// make sure that the parameters are good
	UUmAssert(inWidth > 0);
	UUmAssert(inHeight > 0);
	UUmAssertReadPtr(outTextureMap, sizeof(*outTextureMap));

	// calculate the width, height, and rowbytes of the texture
	M3rTextureMap_GetTextureSize(inWidth, inHeight, &width, &height);

	hasMipMap = (inFlags & M3cTextureFlags_HasMipMap) ? IMcHasMipMap : IMcNoMipMap;
	size_of_texture = IMrImage_ComputeSize(inTexelType, hasMipMap, width, height);

	// create a new instance of the texture
	if (inAllocMemory & M3cTexture_AllocMemory)
	{
		error =
			TMrInstance_Dynamic_New(
				memPool,
				M3cTemplate_TextureMap,
				0,
				&new_texture_map);
	}
	else
	{
		UUmAssert(0);
	}
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new texture.");

#if 0
	UUrDebuggerMessage("texture %s ptr %x %s\n",
		inDebugName,
		new_texture_map,
		(inAllocMemory & M3cTexture_UseTempMem) ? "temporary" : "permanent");
#endif

	error = (new_texture_map != NULL) ? UUcError_None : UUcError_Generic;
	UUmError_ReturnOnErrorMsg(error, "Unable to allocate the texture.");

	// set up the texture map
	strncpy(new_texture_map->debugName, inDebugName, 128);
	new_texture_map->debugName[127] = '\0';

	new_texture_map->width = width;
	new_texture_map->height = height;
	new_texture_map->texelType = inTexelType;
	new_texture_map->flags = inFlags;
	new_texture_map->animation = NULL;

	#if UUmEndian == UUmEndian_Little
	new_texture_map->flags |= M3cTextureFlags_LittleEndian;
	#endif

	new_texture_map->flags |= M3cTextureFlags_Prepared;
	new_texture_map->pixels = TMrInstancePool_Allocate(memPool, size_of_texture);
	new_texture_map->pixelStorage = TMcSeparateFileOffset_None;

	if (new_texture_map->pixels == NULL) {
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to allocate the pixels.");
	}

	// update the texture map data
	error = TMrInstance_Update(new_texture_map);
	UUmError_ReturnOnErrorMsg(error, "Unable to update the data of the texture.");

	// save the outgoing texture map
	*outTextureMap = new_texture_map;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
M3rTextureMap_Big_New(
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	IMtPixelType			inTexelType,
	UUtUns32				inAllocMemory,
	UUtUns32				inFlags,
	const char *			inDebugName,
	M3tTextureMap_Big		**outTextureMap)
{
	UUtError				error;
	M3tTextureMap_Big		*new_texture_map;

	UUtUns16				num_x;
	UUtUns16				num_y;

	UUtUns16				x;
	UUtUns16				y;

	// make sure the parameters are good
	UUmAssert(inWidth > 0);
	UUmAssert(inHeight > 0);
	UUmAssertReadPtr(outTextureMap, sizeof(*outTextureMap));

	// calculate the number of M3tTextureMaps it will take to hold the texture
	num_x = inWidth / M3cTextureMap_MaxWidth;
	num_y = inHeight / M3cTextureMap_MaxHeight;

	if ((inWidth & (M3cTextureMap_MaxWidth - 1)) > 0) num_x++;
	if ((inHeight & (M3cTextureMap_MaxHeight - 1)) > 0) num_y++;

	// create a new instance of the M3tTextureMap_Big
	error =
		TMrInstance_Dynamic_New(
			TMcDynamicPool_Type_Permanent,
			M3cTemplate_TextureMap_Big,
			num_x * num_y,
			&new_texture_map);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new texture.");

	// set up the texture map
	new_texture_map->width		= inWidth;
	new_texture_map->height		= inHeight;
	new_texture_map->texelType	= inTexelType;
	new_texture_map->num_x		= num_x;
	new_texture_map->num_y		= num_y;

	// create the M3tTextureMaps
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

			// calculate the index number of the texture
			index = x + (y * num_x);

			// calculate the texture width and height
			left			= (x * M3cTextureMap_MaxWidth);
			top				= (y * M3cTextureMap_MaxHeight);
			texture_width	= UUmMin(M3cTextureMap_MaxWidth, inWidth - left);
			texture_height	= UUmMin(M3cTextureMap_MaxHeight, inHeight - top);

			// create a new texture
			error =
				M3rTextureMap_New(
					texture_width,
					texture_height,
					inTexelType,
					inAllocMemory,
					inFlags,
					inDebugName,
					&texture);
			UUmError_ReturnOnErrorMsg(error, "Unable to create texture map");

			// save the texture pointer
			new_texture_map->textures[index] = texture;
		}
	}

	// save the new texture
	*outTextureMap = new_texture_map;

	return UUcError_None;
}

void M3rTextureMap_Big_Unload(M3tTextureMap_Big *inTextureMap)
{
	UUtUns32 itr;
	UUtUns32 count;

	if (NULL == inTextureMap) {
		goto exit;
	}

	count = inTextureMap->num_textures;

	for(itr = 0; itr < count; itr++)
	{
		M3tTextureMap *texture = inTextureMap->textures[itr];

		if (NULL == texture) {
			continue;
		}

		M3rDraw_Texture_Unload(texture);
	}

exit:
	return;
}

// ----------------------------------------------------------------------

void
M3rTextureMap_Fill(
	M3tTextureMap*	inTextureMap,
	IMtPixel		inColor,
	const UUtRect*	inBounds)
{
	if (inTextureMap->pixels == NULL) {
		UUmAssert(!"M3rTextureMap_Fill: cannot fill texture, not loaded");

	} else {
		IMrImage_Fill(
			inColor,
			inTextureMap->texelType,
			inTextureMap->width,
			inTextureMap->height,
			inBounds,
			inTextureMap->pixels);
	}

	return;
}

void
M3rTextureMap_Fill_Shade(
	M3tTextureMap*		inTextureMap,
	IMtShade			inShade,
	const UUtRect*		inBounds)
{
	IMtPixel color = IMrPixel_FromShade(inTextureMap->texelType, inShade);

	M3rTextureMap_Fill(inTextureMap, color, inBounds);

	return;
}

// ----------------------------------------------------------------------
void
M3rTextureMap_Big_Fill(
	M3tTextureMap_Big	*inTextureMap,
	IMtPixel			inColor,
	const UUtRect 		*inBounds)
{
	UUtUns16			x;
	UUtUns16			y;

	for (y = 0; y < inTextureMap->num_y; y++)
	{
		for (x = 0; x < inTextureMap->num_x; x++)
		{
			UUtRect 		bounds;
			UUtRect 		*bounds_ptr;
			UUtUns16		index;
			M3tTextureMap	*texture;

			// calculate the index of the texture
			index = x + (y * inTextureMap->num_x);

			// get a pointer to the current texture
			texture = inTextureMap->textures[index];

			if (inBounds == M3cFillEntireMap)
			{
				// set bounds ptr
				bounds_ptr = M3cFillEntireMap;
			}
			else
			{
				UUtRect			texture_rect;

				// set bounds ptr
				bounds_ptr = &bounds;

				// set the texture_rect
				texture_rect.left	= x * M3cTextureMap_MaxWidth;
				texture_rect.top	= y * M3cTextureMap_MaxHeight;
				texture_rect.right	= texture_rect.left + texture->width;
				texture_rect.bottom	= texture_rect.top + texture->height;

				// calculate the bounds to fill
				IMrRect_Intersect(inBounds, &texture_rect, &bounds);

				// offset the rect to the origin of the bitmap
				IMrRect_Offset(&bounds, -texture_rect.left, -texture_rect.top);
			}

			// fill in the texture map
			M3rTextureMap_Fill(
				texture,
				inColor,
				bounds_ptr);
		}
	}
}

// ----------------------------------------------------------------------
void
M3rTextureMap_Copy(
	void				*inSrcTexture,
	UUtRect 			*inSrcRect,
	void				*inDstTexture,
	UUtRect 			*inDstRect)
{
	TMtTemplateTag		src_template_tag;
	IMtPoint2D			src_location;
	UUtUns16			src_width;
	UUtUns16			src_height;
//	UUtUns16			src_x;
//	UUtUns16			src_y;

	TMtTemplateTag		dst_template_tag;
	IMtPoint2D			dst_location;
//	UUtUns16			dst_width;
//	UUtUns16			dst_height;
	UUtUns16			dst_x;
	UUtUns16			dst_y;

	// get the template tags
	src_template_tag = TMrInstance_GetTemplateTag(inSrcTexture);
	dst_template_tag = TMrInstance_GetTemplateTag(inDstTexture);

	if (src_template_tag == M3cTemplate_TextureMap)
	{
		M3tTextureMap	*src_texture = (M3tTextureMap*)inSrcTexture;

		M3rTextureMap_TemporarilyLoad(src_texture, 0);

		// calculate the source width and height
		src_width = (inSrcRect->right - inSrcRect->left);
		src_height = (inSrcRect->bottom - inSrcRect->top);

		// set the source location
		src_location.x = inSrcRect->left;
		src_location.y = inSrcRect->top;

		if (dst_template_tag == M3cTemplate_TextureMap)
		{
			M3tTextureMap	*dst_texture = (M3tTextureMap*)inDstTexture;

			if ((src_texture->pixels == NULL) || (dst_texture->pixels == NULL)) {
				// CB: we cannot copy texturemaps that are not loaded (i.e. templated
				// data, not allocated data)
				UUmAssert(!"M3rTextureMap_Copy: cannot copy texture, not loaded");
				if (dst_texture->pixels != NULL) {
					IMtMipMap hasMipMap = (dst_texture->flags & M3cTextureFlags_HasMipMap) ? IMcHasMipMap : IMcNoMipMap;
					UUtUns32 size = IMrImage_ComputeSize(dst_texture->texelType, hasMipMap, dst_texture->width, dst_texture->height);

					UUrMemory_Clear(dst_texture->pixels, size);
				}

			} else {
				// set the destination location
				dst_location.x = inDstRect->left;
				dst_location.y = inDstRect->top;

				// copy the data
				IMrImage_Copy(
					src_width,
					src_height,
					src_texture->width,
					src_texture->height,
					src_texture->texelType,
					src_texture->pixels,
					&src_location,
					dst_texture->width,
					dst_texture->height,
					dst_texture->texelType,
					dst_texture->pixels,
					&dst_location);
			}
		}
		else if (dst_template_tag == M3cTemplate_TextureMap_Big)
		{
			M3tTextureMap_Big	*dst_texture = (M3tTextureMap_Big*)inDstTexture;

			UUmAssert_Untested();

			for (dst_y = 0; dst_y < dst_texture->num_y; dst_y++)
			{
				for (dst_x = 0; dst_x < dst_texture->num_x; dst_x++)
				{
					M3tTextureMap	*texture;
					UUtUns16		index;
					UUtRect			dst_bounds;
					UUtRect			dst_rect;

					// get the texture[x][y]
					index = dst_x + (dst_y * dst_texture->num_x);
					texture = dst_texture->textures[index];

					if ((src_texture->pixels == NULL) || (texture->pixels == NULL)) {
						// CB: we cannot copy texturemaps that are not loaded (i.e. templated
						// data, not allocated data)
						UUmAssert(!"M3rTextureMap_Copy: cannot copy texture, not loaded");
						if (texture->pixels != NULL) {
							IMtMipMap hasMipMap = (texture->flags & M3cTextureFlags_HasMipMap) ? IMcHasMipMap : IMcNoMipMap;
							UUtUns32 size = IMrImage_ComputeSize(texture->texelType, hasMipMap, dst_texture->width, dst_texture->height);

							UUrMemory_Clear(texture->pixels, size);
						}

					} else {
						// set the bounds rect
						dst_bounds.left = dst_x * M3cTextureMap_MaxWidth;
						dst_bounds.top = dst_y * M3cTextureMap_MaxHeight;
						dst_bounds.right = dst_bounds.left + texture->width;
						dst_bounds.bottom = dst_bounds.top + texture->height;

						// calculate the intersection
						IMrRect_Intersect(&dst_bounds, inDstRect, &dst_rect);

						// check to see if this texture map contains pixels that need
						// to be copied
						if (((dst_rect.right - dst_rect.left) == 0) ||
							((dst_rect.bottom - dst_rect.top) == 0)) continue;

						// move the rect toward the origin
						IMrRect_Offset(&dst_rect, -dst_bounds.left, -dst_bounds.top);

						// set the destination locatoin
						dst_location.x = dst_rect.left;
						dst_location.y = dst_rect.top;

						// copy the data
						IMrImage_Copy(
							src_width,
							src_height,
							src_texture->width,
							src_texture->height,
							src_texture->texelType,
							src_texture->pixels,
							&src_location,
							texture->width,
							texture->height,
							texture->texelType,
							texture->pixels,
							&dst_location);
					}
				}
			}
		}
		else
		{
			UUmAssert(!"Unknwon template type");
		}

		M3rTextureMap_UnloadTemporary(src_texture);
	}
	else if (src_template_tag == M3cTemplate_TextureMap_Big)
	{
		M3tTextureMap_Big	*src_texture = (M3tTextureMap_Big*)inSrcTexture;

		if (dst_template_tag == M3cTemplate_TextureMap)
		{
			UUmAssert(!"unimplemented");
		}
		else if (dst_template_tag == M3cTemplate_TextureMap_Big)
		{
			UUmAssert(!"unimplemented");
		}
		else
		{
			UUmAssert(!"Unknwon template type");
		}
	}
	else
	{
		UUmAssert(!"Unknown template type");
	}
}

// ----------------------------------------------------------------------
MAtMaterialType
M3rTextureMap_GetMaterialType(
	const M3tTextureMap	*inTextureMap)
{
	UUmAssert(inTextureMap);
	UUmAssert((inTextureMap->materialType == MAcInvalidID) ||
			  ((inTextureMap->materialType >= 0) && (inTextureMap->materialType < MArMaterials_GetNumTypes())));
	return (MAtMaterialType) inTextureMap->materialType;
}

// ----------------------------------------------------------------------
void
M3rTextureMap_SetMaterialType(
	M3tTextureMap		*ioTextureMap,
	MAtMaterialType		inMaterialType)
{
	ioTextureMap->materialType = (UUtUns32) inMaterialType;
}

// ----------------------------------------------------------------------
void
M3rTextureRef_GetSize(
	void				*inTextureRef,
	UUtUns16			*outWidth,
	UUtUns16			*outHeight)
{
	TMtTemplateTag		template_tag;
	UUtUns16			width;
	UUtUns16			height;

	template_tag = TMrInstance_GetTemplateTag(inTextureRef);

	if (template_tag == M3cTemplate_TextureMap)
	{
		M3tTextureMap	*texture;

		texture = (M3tTextureMap*)inTextureRef;

		width = texture->width;
		height = texture->height;
	}
	else if (template_tag == M3cTemplate_TextureMap_Big)
	{
		M3tTextureMap_Big	*texture_big;

		texture_big = (M3tTextureMap_Big*)inTextureRef;

		width = texture_big->width;
		height = texture_big->height;
	}

	if (outWidth)
	{
		*outWidth = width;
	}

	if (outHeight)
	{
		*outHeight = height;
	}
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns16
M3iTextureMap_CalcNearestPowerOfTwo(
	UUtUns16			inSize)
{
	UUtInt16			num_bits = 15;

	if (inSize == 0) return 0;

	while(!(inSize & (1 << num_bits))) num_bits--;

	if (inSize > (1 << num_bits))
		num_bits++;

	return (1 << num_bits);
}

// ----------------------------------------------------------------------
void
M3rTextureMap_GetTextureSize(
	UUtUns16	inWidth,
	UUtUns16 	inHeight,
	UUtUns16 	*outWidth,
	UUtUns16 	*outHeight)
{
	UUtUns16 	width;
	UUtUns16 	height;

	UUmAssertWritePtr(outWidth, sizeof(*outWidth));
	UUmAssertWritePtr(outHeight, sizeof(*outHeight));

	width = M3iTextureMap_CalcNearestPowerOfTwo(inWidth);
	height = M3iTextureMap_CalcNearestPowerOfTwo(inHeight);

	if (height < (width >> 3/* S.S. / 8*/))
	{
		height = width >> 3 /* S.S. / 8*/;
	}

	if (width < (height >> 3 /* S.S. / 8*/))
	{
		width = height >> 3 /* S.S. / 8*/;
	}

	*outHeight = height;
	*outWidth = width;

	UUmAssert(*outWidth >= inWidth);
	UUmAssert(*outHeight >= inHeight);
}

static void iBuildMipMapRGB555(M3tTextureMap *inTextureMap, UUtUns16 width, UUtUns16 height, UUtUns16 *src, UUtUns16 *dst)
{
	UUtUns16 i,j;

	for(j = 0; j < (height >> 1/* S.S. / 2*/); j++)
	{
		for(i = 0; i < (width >> 1/* S.S. / 2*/); i++)
		{
			UUtUns16 boxFilter;
			UUtUns16 a,b,c,d;
			UUtUns16 red,grn,blu;
			UUtUns16 rowBytes = width * 2;

			a = *((UUtUns16 *) (((UUtUns32) (src)) + (j * 2 + 0) * rowBytes + (i * 2 + 0) * 2));
			b = *((UUtUns16 *) (((UUtUns32) (src)) + (j * 2 + 0) * rowBytes + (i * 2 + 1) * 2));
			c = *((UUtUns16 *) (((UUtUns32) (src)) + (j * 2 + 1) * rowBytes + (i * 2 + 0) * 2));
			d = *((UUtUns16 *) (((UUtUns32) (src)) + (j * 2 + 1) * rowBytes + (i * 2 + 1) * 2));

			red = 0;
			grn = 0;
			blu = 0;

			blu += (UUtUns16) ((a & (0x1f << 0)) >> 0);
			blu += (UUtUns16) ((b & (0x1f << 0)) >> 0);
			blu += (UUtUns16) ((c & (0x1f << 0)) >> 0);
			blu += (UUtUns16) ((d & (0x1f << 0)) >> 0);
			blu += 2;
			blu >>= 2; // S.S. /= 4;

			grn += (UUtUns16) ((a & (0x1f << 5)) >> 5);
			grn += (UUtUns16) ((b & (0x1f << 5)) >> 5);
			grn += (UUtUns16) ((c & (0x1f << 5)) >> 5);
			grn += (UUtUns16) ((d & (0x1f << 5)) >> 5);
			grn += 2;
			grn >>= 2; // S.S. /= 4;

			red += (UUtUns16) ((a & (0x1f << 10)) >> 10);
			red += (UUtUns16) ((b & (0x1f << 10)) >> 10);
			red += (UUtUns16) ((c & (0x1f << 10)) >> 10);
			red += (UUtUns16) ((d & (0x1f << 10)) >> 10);
			red += 2;
			red >>= 2; // S.S. /= 4;

			boxFilter = (1 << 15) | (red << 10) | (grn << 5) | (blu << 0);

			*dst++ = boxFilter;
		}
	}
}

UUtError
M3rTextureMap_BuildMipMap(
	IMtScaleMode	inScaleMode,
	UUtUns16		inWidth,
	UUtUns16		inHeight,
	IMtPixelType	inPixelType,
	void*			inBaseMap)
{
	UUtError	error;
	UUtUns16	width;
	UUtUns16	height;
	UUtUns16	texelSize = 2;
	void*		srcPtr;
	void*		dstPtr;
	UUtBool		fast = UUcFalse;

	UUmAssertWritePtr(inBaseMap, sizeof(void*));

	// can we build a mipmap for this texture map size
	switch(inPixelType)
	{
		case IMcPixelType_RGB555:
		case IMcPixelType_ARGB1555:
		case IMcPixelType_ARGB4444:
		break;

		default:
			return UUcError_Generic;
	}

	// build the mip maps
	width = inWidth;
	height = inHeight;
	srcPtr = inBaseMap;

	while((width > 1) || (height > 1))
	{
		UUtUns16 dst_width = UUmMax((width >> 1)/* S.S. / 2*/, 1);
		UUtUns16 dst_height = UUmMax((height >> 1)/* S.S. / 2*/, 1);
		dstPtr = (void *) ((UUtUns32) srcPtr + (width * height * texelSize));

		if (fast) {
			error = IMrImage_Scale(
				inScaleMode,
				width,
				height,
				inPixelType,
				srcPtr,
				dst_width,
				dst_height,
				dstPtr);
		}
		else {
			error = IMrImage_Scale(
				inScaleMode,
				inWidth,
				inHeight,
				inPixelType,
				inBaseMap,
				dst_width,
				dst_height,
				dstPtr);
		}

		UUmError_ReturnOnError(error);

		width /= 2;
		height /= 2;

		width = UUmMax(width, 1);
		height = UUmMax(height, 1);

		srcPtr = dstPtr;
	}

	return UUcError_None;
}

void
M3rTextureCoord_FindMinMax(
	UUtUns32			inNumCoords,
	M3tTextureCoord*	inTextureCoords,
	float				*outMinU,
	float				*outMinV,
	float				*outMaxU,
	float				*outMaxV)
{
	UUtUns32	itr;
	float		minU, minV;
	float		maxU, maxV;
	float		curU, curV;

	UUmAssertReadPtr(outMinU, sizeof(float));
	UUmAssertReadPtr(outMinV, sizeof(float));
	UUmAssertReadPtr(outMaxU, sizeof(float));
	UUmAssertReadPtr(outMaxV, sizeof(float));

	minU = minV = 1e9;
	maxU = maxV = -1e9;

	for(itr = 0; itr < inNumCoords; itr++)
	{
		curU = inTextureCoords[itr].u;
		curV = inTextureCoords[itr].v;

		if(curU < minU) minU = curU;
		if(curV < minV) minV = curV;

		if(curU > maxU) maxU = curU;
		if(curV > maxV) maxV = curV;
	}

	*outMinU = minU;
	*outMinV = minV;
	*outMaxU = maxU;
	*outMaxV = maxV;
}


void
M3rDraw_Sprite_Debug(
	const M3tTextureMap*		inTextureMap,
	const M3tPointScreen*		inTopLeft,
	UUtInt32					inStartX,
	UUtInt32					inStartY,
	UUtInt32					inWidth,
	UUtInt32					inHeight)
{
	// one times cache line size should be plenty but why risk it
	UUtUns8 block[sizeof(M3tPointScreen) * 4 + 2 * UUcProcessor_CacheLineSize];
	M3tPointScreen *points = UUrAlignMemory(block);
	UUtInt32 cell_size = 5;
	UUtInt32 endX = inStartX + inWidth;
	UUtInt32 endY = inStartY + inHeight;
	UUtInt32 x;
	UUtInt32 y;

	if (inTextureMap->pixels == NULL) {
		// CB: we cannot display non-memory-resident texturemaps like this
		UUmAssert(!"M3rDraw_Sprite_Debug: cannot display texture, not loaded");
		return;
	}

	M3rDraw_State_Push();

	for(y = inStartY; y < endY; y++)
	{
		if (y >= inTextureMap->height) {
			break;
		}

		for(x = inStartX; x < endX; x++)
		{
			UUtInt32 screen_off_y = (y - inStartY) * cell_size;
			UUtInt32 screen_off_x = (x - inStartX) * cell_size;
			IMtPixel pixel;
			IMtShade shade;

			if (x >= inTextureMap->width) {
				break;
			}

			pixel = IMrPixel_Get(
				inTextureMap->pixels,
				inTextureMap->texelType,
				(UUtUns16) x,
				(UUtUns16) y,
				inTextureMap->width,
				inTextureMap->height);

			shade = IMrShade_FromPixel(
				inTextureMap->texelType,
				pixel);

			if (0 == (shade & 0xff000000)) { continue; }

			M3rDraw_State_SetInt(M3cDrawStateIntType_Appearence, M3cDrawState_Appearence_Gouraud);
			M3rDraw_State_SetInt(M3cDrawStateIntType_Interpolation,	M3cDrawState_Interpolation_None);
			M3rDraw_State_SetInt(M3cDrawStateIntType_Fill, M3cDrawState_Fill_Solid);
			M3rDraw_State_SetInt(M3cDrawStateIntType_NumRealVertices, 0);
			M3rDraw_State_SetInt(M3cDrawStateIntType_SubmitMode, M3cDrawState_SubmitMode_Normal);
			M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, 0xff);
			M3rDraw_State_SetInt(M3cDrawStateIntType_Time, 0);
			M3rDraw_State_SetPtr(M3cDrawStatePtrType_ScreenPointArray, points);
			M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, shade);
			M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, shade >> 24);
			M3rDraw_State_Commit();

			points[0] = *inTopLeft;
			points[0].x += screen_off_x;
			points[0].y += screen_off_y;

			points[1] = points[2] = points[3] = points[0];

			// tl

			// tr
			points[1].x += 5;

			// bl
			points[2].y += 5;

			// br
			points[3].x += 5;
			points[3].y += 5;

			M3rDraw_Line(0, 3);	// tl - br
			M3rDraw_Line(1, 2);	// tr - bl
		}
	}

	M3rDraw_State_Pop();
}

UUtError
M3rManager_Texture_Initialize(
	void)
{
	M3gManager_TextureStorage.temporaryTexture = NULL;

	M3gManager_TextureStorage.temporarySize = M3cTexture_TemporaryStorageSize;
	M3gManager_TextureStorage.temporaryStorage = UUrMemory_Block_New(M3gManager_TextureStorage.temporarySize);

	UUmError_ReturnOnNullMsg(M3gManager_TextureStorage.temporaryStorage, "unable to allocate temporary texture storage");

	return UUcError_None;
}

void
M3rManager_Texture_Terminate(
	void)
{
	if (M3gManager_TextureStorage.temporaryStorage != NULL) {
		UUrMemory_Block_Delete(M3gManager_TextureStorage.temporaryStorage);
	}

	M3gManager_TextureStorage.temporarySize = 0;
	M3gManager_TextureStorage.temporaryStorage = NULL;
}

