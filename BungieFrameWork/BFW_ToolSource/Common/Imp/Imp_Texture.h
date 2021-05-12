/*
	FILE:	Imp_Texture.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: August 14, 1997
	
	PURPOSE: Header file for the BMP file format
	
	Copyright 1997

*/
#ifndef IMP_TEXTURE_H
#define IMP_TEXTURE_H

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_Motoko.h"
#include "BFW_Group.h"

#include "Imp_TextureFlags.h"

UUtError
Imp_AddTexture(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

UUtError
Imp_AddTexture_Big(
	BFtFileRef			*inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup			*inGroup,
	char				*inInstanceName);
	
UUtError
Imp_AddTextureDirectory(
	BFtFileRef*			inSourceFileRef,
	UUtUns32			inSourceFileModDate,
	GRtGroup*			inGroup,
	char*				inInstanceName);

#define IMPcUniqueTextureName	NULL
#define IMPcIgnoreTextureRef	NULL

UUtError
Imp_ProcessTexture(
	BFtFileRef*			inFileRef,
	UUtUns32			inReduceAmount,
	const tTextureFlags	*inDefaultTextureFlags,
	tTextureFlags		*outNewTextureFlags,
	const char*			inInstanceName,
	M3tTextureMap		**outTextureRef);

void iBuildMipMap(M3tTextureMap *inTextureMap, UUtUns16 width, UUtUns16 height, UUtUns16 *src, UUtUns16 *dst);

void
Imp_WriteTextureSize(
	void);
	
UUtError
Imp_ProcessTexture_File(
	BFtFileRef			*inFileRef,
	const char			*inInstanceName,
	TMtPlaceHolder		*outTextureRef);

UUtError
Imp_ProcessTexture_Big_File(
	BFtFileRef			*inBMPFileRef,
	const char			*inInstanceName,
	TMtPlaceHolder		*outTextureRef);


UUtError Imp_AddLocalTexture( BFtFileRef* inSourceFileRef, char* inTextureName, M3tTextureMap **ioTextureMap );
UUtError Imp_AddLocalTextures( BFtFileRef *inDirectoryRef );
	
#endif /* IMP_TEXTURE_H */
