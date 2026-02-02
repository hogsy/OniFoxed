/*
	FILE:	Imp_Texture.c

	AUTHOR:	Brent H. Pease

	CREATED: August 14, 1997

	PURPOSE: Header file for the BMP file format

	Copyright 1997

*/

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_Motoko.h"
#include "BFW_Group.h"
#include "BFW_AppUtilities.h"
#include "BFW_FileFormat.h"
#include "BFW_Materials.h"

#include "Imp_Common.h"
#include "Imp_Texture.h"
#include "Imp_TextureFlags.h"

UUtUns16	gSizeTable[9][9] =
			{
				{0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0, 0}
			};


#define cMaxInvalidTextures 4096

UUtUns16	gNumInvalidTextures = 0;

typedef struct tInvalidTextureInfo {
	char			name[128];
	IMtPixelType	texelType;
	UUtUns16		width;
	UUtUns16		height;
} tInvalidTextureInfo;

tInvalidTextureInfo gInvalidTextureInfo[cMaxInvalidTextures];



UUtError
Imp_AddTexture(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;
	BFtFileRef*			file;
	char				mungedInstanceName[BFcMaxFileNameLength];
	tTextureFlags		flags;

	error =
		Imp_Common_GetFileRefAndModDate(
			inSourceFileRef,
			inGroup,
			"file",
			&file);
	IMPmError_ReturnOnErrorMsg(error, "Could not get file name");

	UUrString_Copy(mungedInstanceName, inInstanceName, BFcMaxFileNameLength);
	UUrString_StripExtension(mungedInstanceName);

	// setup default flags
	Imp_ClearTextureFlags(&flags);
	flags.flags = 0;
	flags.flags |= M3cTextureFlags_HasMipMap;
	flags.hasFlagFile = cTextureFlagFile_No;

	Imp_ProcessTextureFlags(inGroup, mungedInstanceName, &flags);

	error = Imp_ProcessTexture(file, 0, &flags, NULL, mungedInstanceName, IMPcIgnoreTextureRef);

	BFrFileRef_Dispose(file);

	return UUcError_None;
}

static UUtError iIsValidTextureMapSize(UUtUns32 inWidth, UUtUns32 inHeight)
{
	UUtError result = UUcError_None;

	switch(inWidth)
	{
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

static UUtError iMakeAnimFrameFileRef(
	BFtFileRef *inDirectoryRef,
	char *inBaseName,
	UUtUns16 itr,
	char *suffixName,
	BFtFileRef **outFileRef)
{
	UUtError error;
	char frameName1[BFcMaxFileNameLength * 2];
	char frameName2[BFcMaxFileNameLength * 2];
	BFtFileRef *frameFileRef;
	UUtBool retry;
	const char*	dirLeafName;

	dirLeafName = BFrFileRef_GetLeafName(inDirectoryRef);

	do
	{
		retry = UUcFalse;

		sprintf(frameName1, "%s_%d%s",
			inBaseName,
			itr + 1,
			suffixName);

		error =
			BFrFileRef_DuplicateAndAppendName(
				inDirectoryRef,
				frameName1,
				&frameFileRef);
		UUmError_ReturnOnErrorMsgP(error, "could not create file ref for anim frame, %s, %s", (UUtUns32)dirLeafName, (UUtUns32)frameName1, 0);

		if (!BFrFileRef_FileExists(frameFileRef))
		{
			BFrFileRef_Dispose(frameFileRef);
			frameFileRef = NULL;

			sprintf(frameName2, "%s_0%d%s",
				inBaseName,
				itr + 1,
				suffixName);

			error =
				BFrFileRef_DuplicateAndAppendName(
					inDirectoryRef,
					frameName2,
					&frameFileRef);
			UUmError_ReturnOnErrorMsg(error, "could not create file ref for anim frame");
		}

		if (!BFrFileRef_FileExists(frameFileRef))
		{
			BFrFileRef_Dispose(frameFileRef);
			frameFileRef = NULL;
		}

		// possible retry if we couldn't load the file
		if (NULL == frameFileRef)
		{
			AUtMB_ButtonChoice choice = AUrMessageBox(AUcMBType_RetryCancel, "Could not find anim frame %s or %s.", frameName1, frameName2);

			if (AUcMBChoice_Retry == choice) {
				retry = UUcTrue;
			}
		}
	} while (retry);

	*outFileRef = frameFileRef;
	error = (frameFileRef != NULL) ? UUcError_None : BFcError_FileNotFound;

	return error;
}

static UUtError
Imp_ProcessTextureAnimation(
	M3tTextureMap		*inTextureMap,
	BFtFileRef*			inFileRef,
	const tTextureFlags	*inDefaultTextureFlags,
	M3tTextureMapAnimation	**outTextureAnimationRef)
{
	tTextureFlags localDefaultTextureFlags;
	UUtError	error;
	UUtUns16	itr;
	const char *leafName;
	char		animDirectoryName[BFcMaxFileNameLength * 2];
	BFtFileRef *animDirectoryRef;
	UUtBool		fileExists;
	char		suffixName[BFcMaxFileNameLength];
	M3tTextureMapAnimation *animation;

	leafName = BFrFileRef_GetLeafName(inFileRef);
	fileExists = BFrFileRef_FileExists(inFileRef);

	// create the animation structure
	error = TMrConstruction_Instance_NewUnique(
				M3cTemplate_TextureMapAnimation,
				inDefaultTextureFlags->numFrames,
				(void **)&animation);
	IMPmError_ReturnOnError(error);

	// set the field of the animation up
	animation->timePerFrame = inDefaultTextureFlags->timePerFrame;
	animation->numFrames = inDefaultTextureFlags->numFrames;
	animation->randTPF_low = inDefaultTextureFlags->timePerFrame;
	animation->randTPF_range = 0;
	animation->maps[0] = NULL; // inTextureMap; -- screws up touch recursive..

	// setup the texture flags for all our child textures
	localDefaultTextureFlags = *inDefaultTextureFlags;
	localDefaultTextureFlags.numFrames = 1;

	// get the suffix name
	suffixName[0] = '.';
	UUrString_Copy(suffixName + 1, BFrFileRef_GetSuffixName(inFileRef), BFcMaxFileNameLength);

	// open the directory
	UUrString_Copy(animDirectoryName, leafName, BFcMaxFileNameLength * 2);
	animDirectoryName[strlen(leafName) - 4] = '\0';

	error =
		BFrFileRef_DuplicateAndReplaceName(
			inFileRef,
			animDirectoryName,
			&animDirectoryRef);
	UUmError_ReturnOnErrorMsg(error, "Could not find anim directory");

	// get all the frame file refs
	for(itr = 1; itr < animation->numFrames; itr++)
	{
		BFtFileRef *frameFileRef;
		M3tTextureMap *frameTexture;

		error = iMakeAnimFrameFileRef(animDirectoryRef, animDirectoryName, itr, suffixName, &frameFileRef);
		UUmError_ReturnOnErrorMsg(error, "failed to find anim frame file!");

		error = Imp_ProcessTexture(
			frameFileRef,
			0,
			&localDefaultTextureFlags,
			NULL,
			IMPcUniqueTextureName,
			&frameTexture);
		UUmError_ReturnOnErrorMsg(error, "failed to read anim frame");

		BFrFileRef_Dispose(frameFileRef);

		animation->maps[itr] = frameTexture;
	}

	BFrFileRef_Dispose(animDirectoryRef);

	*outTextureAnimationRef = animation;

	return UUcError_None;
}


static void iUpdateTextureSizeTable(const M3tTextureMap *inTexture)
{
	UUtUns16 widthBits;
	UUtUns16 heightBits;
	UUtBool valid_texture;

	// Add it to the size global var
	switch(inTexture->width)
	{
		case 256:	widthBits = 8; break;
		case 128:	widthBits = 7; break;
		case 64:	widthBits = 6; break;
		case 32:	widthBits = 5; break;
		case 16:	widthBits = 4; break;
		case 8:		widthBits = 3; break;
		case 4:		widthBits = 2; break;
		case 2:		widthBits = 1; break;
		case 1:		widthBits = 0; break;
		default: UUmAssert(!"invalid texture size");
	}

	switch(inTexture->height)
	{
		case 256:	heightBits = 8; break;
		case 128:	heightBits = 7; break;
		case 64:	heightBits = 6; break;
		case 32:	heightBits = 5; break;
		case 16:	heightBits = 4; break;
		case 8:		heightBits = 3; break;
		case 4:		heightBits = 2; break;
		case 2:		heightBits = 1; break;
		case 1:		heightBits = 0; break;
		default: UUmAssert(!"invalid texture size");
	}

	gSizeTable[widthBits][heightBits]++;

	if (IMcPixelType_DXT1 == inTexture->texelType ) {
		valid_texture = UUcFalse;
		valid_texture |= (128 == inTexture->width) && (128 == inTexture->height);
		valid_texture |= (64 == inTexture->width) && (64 == inTexture->height);
		valid_texture |= (32 == inTexture->width) && (32 == inTexture->height);
	}
	else if (IMcPixelType_ARGB4444 == inTexture->texelType) {
		valid_texture = UUcFalse;
		valid_texture |= (256 == inTexture->width) && (256 == inTexture->height);
		valid_texture |= (128 == inTexture->width) && (128 == inTexture->height);
		valid_texture |= (128 == inTexture->width) && (64 == inTexture->height);
		valid_texture |= (64 == inTexture->width) && (128 == inTexture->height);
		valid_texture |= (64 == inTexture->width) && (64 == inTexture->height);
		valid_texture |= (32 == inTexture->width) && (32 == inTexture->height);
	}
	else {
		valid_texture = UUcFalse;
	}

	{
		if (gNumInvalidTextures < cMaxInvalidTextures) {
			tInvalidTextureInfo *new_invalid_texture;

			new_invalid_texture = gInvalidTextureInfo + gNumInvalidTextures;
			gNumInvalidTextures++;

			strcpy(new_invalid_texture->name, inTexture->debugName);

			new_invalid_texture->texelType = inTexture->texelType;
			new_invalid_texture->width = inTexture->width;
			new_invalid_texture->height = inTexture->height;
		}
	}

	return;
}


UUtError
Imp_ProcessTexture(
	BFtFileRef*			inFileRef,
	UUtUns32			inReduceAmount,
	const tTextureFlags	*inDefaultTextureFlags,
	tTextureFlags		*outNewTextureFlags,
	const char*			inInstanceName,
	M3tTextureMap		**outTextureRef)
{
	UUtError			error;
	M3tTextureMap*		textureMap;

	UUtUns16			width;
	UUtUns16			height;
	IMtMipMap			mipMap;
	void*				imageData;
	void*				separateData;

	UUtUns32			imageBytes;
	UUtUns32			totalMapBytes;

	UUtBool				buildInstance;

	const char*				leafName;
	UUtBool				fileExists;

	tTextureFlags		local_flags;
	IMtPixelType		pixel_type;

	FFtFileInfo			file_info;

	BFtFileRef*			localFrameRef;
	BFtFileRef*			ddsFrameRef = NULL;
	char				suffixName[BFcMaxFileNameLength];

	if (inInstanceName)
	{
		char*	cp;

		cp = strchr(inInstanceName, '.');
		UUmAssert(cp == NULL);
	}

	{
		const char *name = inInstanceName ? inInstanceName : "unique";

		// Imp_PrintMessage(IMPcMsg_Critical, "**** creating texture %s"UUmNL, name);
	}

	// copy the default flags
	local_flags = *inDefaultTextureFlags;

	do
	{
		leafName = BFrFileRef_GetLeafName(inFileRef);
		fileExists = BFrFileRef_FileExists(inFileRef);

		if(!fileExists)
		{
			// try to add .bmp extension first
			BFrFileRef_SetLeafNameSuffex(inFileRef, "BMP");
			fileExists = BFrFileRef_FileExists(inFileRef);
		}

		if(!fileExists)
		{
			// try to add .psd extension next
			BFrFileRef_SetLeafNameSuffex(inFileRef, "PSD");
			fileExists = BFrFileRef_FileExists(inFileRef);
		}

		if(!fileExists)
		{
			AUtMB_ButtonChoice choice =	AUrMessageBox(AUcMBType_AbortRetryIgnore, "Could not find file %s."UUmNL,	leafName);

			if (AUcMBChoice_Abort == choice) {
				IMPmError_ReturnOnErrorMsg(UUcError_Generic, "Could not find texture file");
			}

			if (AUcMBChoice_Ignore == choice) {
				return UUcError_None;
			}
		}
	} while (!fileExists);

	if(IMPcUniqueTextureName == inInstanceName)
	{
		buildInstance = UUcTrue;
	}
	else if (!TMrConstruction_Instance_CheckExists(M3cTemplate_TextureMap, inInstanceName))
	{
		buildInstance = UUcTrue;
	}
	else
	{
		buildInstance = UUcFalse;
	}


	if(buildInstance != UUcTrue)
	{
		return UUcError_None;
	}

	// process the flags file
		Imp_ProcessTextureFlags_FromFileRef(inFileRef, &local_flags);

	// Set the first frame
	localFrameRef = inFileRef;

	if((local_flags.flags & M3cTextureFlags_PixelType) && IMrPixelType_HasAlpha(local_flags.pixelType))
	{
		// get the suffix name
		UUrString_Copy(suffixName, BFrFileRef_GetSuffixName(inFileRef), BFcMaxFileNameLength);
		UUrString_Capitalize(suffixName, BFcMaxFileNameLength);

		if(strcmp(suffixName, "PSD"))
		{
			BFrFileRef_SetLeafNameSuffex(localFrameRef, "PSD");
		}
	}
	else
	{
		UUtBool compressed;

		// ok scan to see if we can use a compressed version
		char ddsFileName[BFcMaxFileNameLength];
		UUtUns32 ddsFileNameLength;
		UUrString_Copy(ddsFileName, BFrFileRef_GetLeafName(localFrameRef), BFcMaxFileNameLength);

		ddsFileNameLength = strlen(ddsFileName);
		UUmAssert(ddsFileNameLength > 4);
		ddsFileName[ddsFileNameLength - 3] = 'd';
		ddsFileName[ddsFileNameLength - 2] = 'd';
		ddsFileName[ddsFileNameLength - 1] = 's';

		error = BFrFileRef_DuplicateAndReplaceName(localFrameRef, ddsFileName, &ddsFrameRef);
		UUmError_ReturnOnErrorMsg(error, "could not duplicate and replace");

		compressed = BFrFileRef_FileExists(ddsFrameRef);

		if (compressed) {
			UUtUns32 original_mod_time;
			UUtUns32 dds_mod_time;

			error = BFrFileRef_GetModTime(localFrameRef, &original_mod_time);
			UUmError_ReturnOnErrorMsg(error, "could not get modification time");

			error = BFrFileRef_GetModTime(ddsFrameRef, &dds_mod_time);
			UUmError_ReturnOnErrorMsg(error, "could not get modification time");

			if (dds_mod_time >= original_mod_time) {
				localFrameRef = ddsFrameRef;
			}
			else {
				Imp_PrintWarning("file %s is newer then %s",
					BFrFileRef_GetLeafName(localFrameRef),
					BFrFileRef_GetLeafName(ddsFrameRef));

				compressed = UUcFalse;
			}
		}


	}

	// get the file info
	error = FFrPeek_2D(localFrameRef, &file_info);
	UUmError_ReturnOnErrorMsgP(error, "failed to get file info on %s", (UUtUns32) BFrFileRef_GetLeafName(localFrameRef), 0, 0);

	// XXX - ONE DAY MAKE SURE WE HANDLE DDS ALPHA CORRECTLY

	switch(file_info.format)
	{
		case FFcFormat_2D_PSD:
			pixel_type = IMcPixelType_ARGB4444;
			break;

		case FFcFormat_2D_BMP:
			pixel_type = IMcPixelType_RGB555;
			break;

		case FFcFormat_2D_DDS:
			pixel_type = IMcPixelType_DXT1;
			break;

		default:
			UUmAssert(!"unsupported file format");
	}

	width = file_info.width;
	height = file_info.height;

	width = width >> inReduceAmount;
	height = height >> inReduceAmount;

	// set the pixel type
	if (local_flags.flags & M3cTextureFlags_PixelType)
	{
		if (IMcPixelType_DXT1 == pixel_type) {
			const char *whine_leaf_name = BFrFileRef_GetLeafName(localFrameRef);

			Imp_PrintWarning("texture %s was DXT1 but attempted to force pixel type, ignoring", whine_leaf_name);

			local_flags.pixelType = pixel_type;
		}
		else {
			pixel_type = local_flags.pixelType;
		}
	}
	else
	{
		local_flags.pixelType = pixel_type;
	}

	if (local_flags.flags & M3cTextureFlags_HasMipMap)
	{
		totalMapBytes = IMrImage_ComputeSize(pixel_type, IMcHasMipMap, width, height);
	}
	else
	{
		totalMapBytes = IMrImage_ComputeSize(pixel_type, IMcNoMipMap, width, height);
	}

	/*
	 * Create the texture map instance
	 */
	if (IMPcUniqueTextureName == inInstanceName)
	{
		error = TMrConstruction_Instance_NewUnique(
			M3cTemplate_TextureMap,
			0,
			(void **)&textureMap);
		IMPmError_ReturnOnError(error);
		strncpy(textureMap->debugName, "imported unique texture", 128);
	}
	else
	{
		error =
			TMrConstruction_Instance_Renew(
				M3cTemplate_TextureMap,
				inInstanceName,
				0,
				(void **)&textureMap);
		IMPmError_ReturnOnError(error);
		strncpy(textureMap->debugName, inInstanceName, 128);
	}

	textureMap->flags = local_flags.flags;
	textureMap->texelType = pixel_type;
	textureMap->width = width;
	textureMap->height = height;
	textureMap->animation = NULL;
	textureMap->envMap = local_flags.envMapPlaceHolder;
	textureMap->materialType = MAcMaterial_Base;

	separateData = NULL;
	textureMap->pixels = NULL;
	textureMap->pixelStorage = TMcSeparateFileOffset_None;

#if 0
	// CB: perhaps we should use some criteria to decide whether certain textures should be permanently resident?
	textureMap->pixels = TMrConstruction_Raw_New(totalMapBytes, 16, M3cTemplate_TextureMap);
#else
	separateData = TMrConstruction_Separate_New(totalMapBytes, M3cTemplate_TextureMap);
#endif

	if (!BFrFileRef_FileExists(localFrameRef))
	{
		UUmError_ReturnOnErrorMsgP(UUcError_Generic, "File %s does not exist.",
			(UUtUns32) BFrFileRef_GetLeafName(inFileRef), 0, 0);
	}

	// read the image data
	UUrMemory_Block_VerifyList();
	error =
		FFrRead_2D_Reduce(
			localFrameRef,
			pixel_type,
			inReduceAmount,
			(M3cTextureFlags_ReceivesEnvMap & textureMap->flags) ? IMcScaleMode_IndependantAlpha : IMcScaleMode_Box,
			&width,
			&height,
			&mipMap,
			&imageData);
	UUmError_ReturnOnErrorMsgP(error, "File %s could not be read.",
		(UUtUns32) BFrFileRef_GetLeafName(localFrameRef), 0, 0);
	UUrMemory_Block_VerifyList();

	// no means no
	if (0 == (local_flags.flags & M3cTextureFlags_HasMipMap))
	{
		mipMap = IMcNoMipMap;
	}

	error = iIsValidTextureMapSize(width, height);
	if(error != UUcError_None)
	{
		Imp_PrintWarning("File %s had an invalid texture map size (%d,%d)"UUmNL"(1 to 256 and 8x1 ratio only)",
			BFrFileRef_GetLeafName(inFileRef),
			width,
			height);

		return error;
	}

	/*
	 * Copy the image into the texture map
	 */

	imageBytes = IMrImage_ComputeSize(pixel_type, mipMap, width, height);
	if (textureMap->pixels != NULL) {
		UUrMemory_MoveFast(imageData, textureMap->pixels, imageBytes);
	}
	if (separateData != NULL) {
		UUrMemory_MoveFast(imageData, separateData, imageBytes);
	}

	// if we want a mip map and our file lacks one, build it
	if ((textureMap->flags & M3cTextureFlags_HasMipMap) && (IMcNoMipMap == mipMap))
	{
		IMtScaleMode scale_mode = (textureMap->flags & M3cTextureFlags_ReceivesEnvMap) ? IMcScaleMode_IndependantAlpha : IMcScaleMode_Box;

		if (textureMap->pixels != NULL) {
			error = M3rTextureMap_BuildMipMap(scale_mode, width, height, pixel_type, textureMap->pixels);
		}
		if ((separateData != NULL) && (error == UUcError_None)) {
			error = M3rTextureMap_BuildMipMap(scale_mode, width, height, pixel_type, separateData);
		}

		if (error != UUcError_None)
		{
			textureMap->flags &= ~M3cTextureFlags_HasMipMap;
		}
	}

	UUrMemory_Block_Delete(imageData);

	if (local_flags.numFrames > 1)
	{
		M3tTextureMapAnimation *animation;

		error = Imp_ProcessTextureAnimation(
			textureMap,
			inFileRef,
			&local_flags,
			&animation);
		UUmError_ReturnOnError(error);

		textureMap->animation = animation;
	}

	// update stats
	iUpdateTextureSizeTable(textureMap);

	if (IMPcIgnoreTextureRef != outTextureRef)
	{
		UUmAssertReadPtr(outTextureRef, sizeof(*outTextureRef));
		*outTextureRef = textureMap;
	}

	if (ddsFrameRef) BFrFileRef_Dispose(ddsFrameRef);

	if(outNewTextureFlags != NULL)
	{
		*outNewTextureFlags = local_flags;
	}

	#if UUmEndian == UUmEndian_Little
	textureMap->flags |= M3cTextureFlags_LittleEndian;
	#endif

	if (textureMap->pixels != NULL) {
		textureMap->pixels = TMrConstruction_Raw_Write(textureMap->pixels);
	}
	if (separateData != NULL) {
		textureMap->pixelStorage = (TMtSeparateFileOffset) TMrConstruction_Separate_Write(separateData);
	}

	return UUcError_None;
}

#define IMPcTexture_MaxFilesInDir	(1024)

typedef enum {
	textures_in_directory_lower_case,
	textures_in_directory_upper_case,
	textures_in_directory_existing_case
} texture_in_directory_type;

static UUtError AddTexturesInDirectory(BFtFileRef *inDirectoryRef, texture_in_directory_type texture_case)
{
	UUtError				error;
	BFtFileIterator			*file_iterator;

	// create the file iterator for the directory
	error = BFrDirectory_FileIterator_New( inDirectoryRef, NULL, NULL, &file_iterator);
	IMPmError_ReturnOnError(error);

	while (1)
	{
		BFtFileRef			file_ref;
		char				name[BFcMaxFileNameLength];
		TMtPlaceHolder		texture_ref;

		// get file ref for the file
		error = BFrDirectory_FileIterator_Next(file_iterator, &file_ref);
		if (error != UUcError_None) { break; }

		// skip files that are not valid image files
		{
			const char			*file_suffix;
			UUtBool				valid_image_suffix;

			file_suffix = BFrFileRef_GetSuffixName(&file_ref);
			if (file_suffix == NULL) { continue; }
			valid_image_suffix = UUmString_IsEqual_NoCase(file_suffix, "bmp") | UUmString_IsEqual_NoCase(file_suffix, "psd");
			if (!valid_image_suffix) { continue; }
		}

		// get the file name
		UUrString_Copy(name, BFrFileRef_GetLeafName(&file_ref), BFcMaxFileNameLength);

		switch(texture_case)
		{
			case textures_in_directory_lower_case:
				UUrString_MakeLowerCase(name, BFcMaxFileNameLength);
			break;

			case textures_in_directory_upper_case:
				UUrString_Capitalize(name, BFcMaxFileNameLength);
			break;

			case textures_in_directory_existing_case:
			break;
		}

		UUrString_StripExtension(name);

		error = Imp_ProcessTexture_File(&file_ref, name, &texture_ref);
		IMPmError_ReturnOnError(error);
	}

	BFrDirectory_FileIterator_Delete(file_iterator);
	file_iterator = NULL;

	return UUcError_None;
}

UUtError
Imp_AddTextureDirectory(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName)
{
	UUtError			error;

	char*				textureFileDirName;
	BFtFileRef			textureFileDirRef = *inSourceFileRef;

	error = GRrGroup_GetString(inGroup, "file", &textureFileDirName);
	IMPmError_ReturnOnErrorMsg(error, "Could not get texture file info");

	error = BFrFileRef_SetName(&textureFileDirRef, textureFileDirName);
	IMPmError_ReturnOnErrorMsg(error, "Could not make texture file dir ref");

	AddTexturesInDirectory(&textureFileDirRef, textures_in_directory_upper_case);

	return UUcError_None;
}

void
Imp_WriteTextureSize(
	void)
{
	FILE*	file;
	UUtUns16	width, height;
	UUtUns16	curWidth, curHeight;
	UUtUns32 invalid_texture_loop;

	file = fopen("textureSize.txt", "wb");
	if(file == NULL) return;

	curWidth = 1;
	for(width = 0; width <= 8; width++)
	{
		curHeight = 1;
		for(height = width; height <= 8; height++)
		{
			UUtUns32 total;

			total = 0;
			total += gSizeTable[width][height];
			total += (height != width) ? gSizeTable[height][width] : 0;

			if (total > 0) {
				fprintf(file, "[%d][%d] = %d"UUmNL, 1 << width, 1 << height, total);
			}

			curHeight *= 2;
		}
		curWidth *= 2;
	}

	fprintf(file, "%d textures:"UUmNL, gNumInvalidTextures);

	for(invalid_texture_loop = 0; invalid_texture_loop < gNumInvalidTextures; invalid_texture_loop++)
	{
		tInvalidTextureInfo *cur_invalid_texture = gInvalidTextureInfo + invalid_texture_loop;
		const char *texture_format_string = IMrPixelTypeToString(cur_invalid_texture->texelType);

		fprintf(file, "%s is resolution = %d by %d format = %s"UUmNL,
			cur_invalid_texture->name,
			cur_invalid_texture->width,
			cur_invalid_texture->height,
			texture_format_string);
	}

	fclose(file);
}

// ----------------------------------------------------------------------
UUtError
Imp_ProcessTexture_File(
	BFtFileRef			*inFileRef,
	const char			*inInstanceName,
	TMtPlaceHolder		*outTextureRef)
{
	UUtError			error;
	UUtUns32			file_mod_date;
	tTextureFlags		flags;
	char				mungedInstanceName[BFcMaxFileNameLength];

	UUmAssert(inFileRef);
	UUmAssert(outTextureRef);
	UUmAssert(inInstanceName);

	if (!BFrFileRef_FileExists(inFileRef)) {
		Imp_PrintWarning("Could not find file %s", BFrFileRef_GetLeafName(inFileRef));
	}

	// get the modification time of the file
	error = BFrFileRef_GetModTime(inFileRef, &file_mod_date);
	IMPmError_ReturnOnErrorMsg(error, "Could not get file ref mod date");

	// set the texture flags
	Imp_ClearTextureFlags(&flags);
	flags.hasFlagFile = cTextureFlagFile_Yes;
	flags.flags = M3cTextureFlags_None;

	//strip off extension
	UUrString_Copy(mungedInstanceName, inInstanceName, BFcMaxFileNameLength);
	UUrString_StripExtension(mungedInstanceName);

	// process M3cTextureMap_Big
	error =
		Imp_ProcessTexture(
			inFileRef,
			0,
			&flags,
			NULL,
			mungedInstanceName,
			IMPcIgnoreTextureRef);
	IMPmError_ReturnOnError(error);

	// get a placeholder for the M3tTextureMap_Big instance of texture_name
	{
		M3tTextureMap *texture = M3rTextureMap_GetPlaceholder(mungedInstanceName);
		UUmAssert(NULL != texture);

		*outTextureRef = (TMtPlaceHolder) texture;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
Imp_ProcessTexture_Dir(
	BFtFileRef			*inFileRef,
	const char			*inInstanceName,
	TMtPlaceHolder		*outTextureRef)
{
	UUtError			error;
	UUtUns32			file_mod_date;
	tTextureFlags		flags;
	char				mungedInstanceName[BFcMaxFileNameLength];

	UUmAssert(inFileRef);
	UUmAssert(outTextureRef);
	UUmAssert(inInstanceName);

	// get the modification time of the file
	//error = BFrFileRef_GetModTime(inFileRef, &file_mod_date);
	//IMPmError_ReturnOnErrorMsg(error, "Could not get file ref mod date");

	file_mod_date		= 0;

	// set the texture flags
	Imp_ClearTextureFlags(&flags);
	flags.hasFlagFile	= cTextureFlagFile_Yes;
	flags.flags			= M3cTextureFlags_None;

	//strip off extension
	UUrString_Copy(mungedInstanceName, inInstanceName, BFcMaxFileNameLength);
	UUrString_StripExtension(mungedInstanceName);
	UUrString_MakeLowerCase(mungedInstanceName, BFcMaxFileNameLength);

	// process M3cTextureMap_Big
	error =
		Imp_ProcessTexture(
			inFileRef,
			0,
			&flags,
			NULL,
			mungedInstanceName,
			IMPcIgnoreTextureRef);
	IMPmError_ReturnOnError(error);

	// get a placeholder for the M3tTextureMap_Big instance of texture_name
	error =
		TMrConstruction_Instance_GetPlaceHolder(
			M3cTemplate_TextureMap,
			mungedInstanceName,
			outTextureRef);
	IMPmError_ReturnOnError(error);

	return UUcError_None;
}

UUtError Imp_AddLocalTextures( BFtFileRef *inDirectoryRef )
{
	UUtError				error;

	error = AddTexturesInDirectory(inDirectoryRef, textures_in_directory_lower_case);

	return error;
}

UUtError Imp_AddLocalTexture( BFtFileRef* inSourceFileRef, char* inTextureName, M3tTextureMap **ioTextureMap )
{
	BFtFileRef				*texture_ref;
	UUtError				error;

	error = BFrFileRef_DuplicateAndReplaceName( inSourceFileRef, inTextureName, &texture_ref );
	IMPmError_ReturnOnError(error);

	error = Imp_ProcessTexture_Dir( texture_ref, inTextureName, (TMtPlaceHolder*) ioTextureMap );
	IMPmError_ReturnOnError(error);

	BFrFileRef_Dispose( texture_ref );
	return UUcError_None;
}

