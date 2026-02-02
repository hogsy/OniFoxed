// ======================================================================
// Imp_TextureFlags.h
// ======================================================================
#ifndef IMP_TEXTUREFLAGS_H
#define IMP_TEXTUREFLAGS_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Group.h"

// ======================================================================
// typedefs
// ======================================================================
typedef enum
{
	cTextureFlagFile_No,
	cTextureFlagFile_Yes
} tTextureFlagFile;

typedef struct tTextureFlags
{
	tTextureFlagFile	hasFlagFile;
	UUtUns32			flags;
	IMtPixelType		pixelType;
	UUtUns16			timePerFrame;
	UUtUns16			numFrames;
	UUtBool				reflectivity_specified;
	M3tColorRGB			reflectivity_color;
	M3tTextureMap*		envMapPlaceHolder;

} tTextureFlags;

// ======================================================================
// prototypes
// ======================================================================
void
Imp_ClearTextureFlags(
	tTextureFlags *outFlags);

void
Imp_ProcessTextureFlags(
	GRtGroup*			inGroup,
	const char*			inFileName,
	tTextureFlags		*outTextureFlags);

void
Imp_ProcessTextureFlags_FromFileRef(
	BFtFileRef*			inFileRef,
	tTextureFlags*		ioTextureFlags);

// ======================================================================
#endif /* IMP_TEXTUREFLAGS_H */
