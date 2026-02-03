// ======================================================================
// BFW_ViewUtilities.h
// ======================================================================
#ifndef BFW_VIEWUTILITIES_H
#define BFW_VIEWUTILITIES_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_ViewManager.h"

// ======================================================================
// defines
// ======================================================================
#define VUcAlpha_Enabled				M3cMaxAlpha
#define VUcAlpha_Disabled				40

extern TMtPrivateData*	DMgTemplate_PartSpec_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
void
VUrDrawPartSpecification(
	VMtPartSpec			*inPartSpec,
	UUtUns16			inFlags,
	M3tPointScreen		*inLocation,
	UUtUns16			inWidth,
	UUtUns16			inHeight,
	UUtUns16			inAlpha);

void
VUrDrawTextureRef(
	void				*inTextureRef,
	M3tPointScreen		*inLocation,
	UUtUns16			inWidth,
	UUtUns16			inHeight,
	UUtUns16			inAlpha);

// ----------------------------------------------------------------------
UUtError
VUrCreate_Texture(
	UUtUns16			inWidth,
	UUtUns16			inHeight,
	void				**outTextureRef);

UUtError
VUrCreate_StringTexture(
	char				*inString,
	UUtUns16			inFlags,
	void				**outStringTexture,
	UUtRect				*outStringBounds);

void
VUrClearTexture(
	void				*inTexture,
	UUtRect				*inBounds,
	IMtPixel			inColor);

// ======================================================================
#endif /* BFW_VIEWUTILITIES_H */
