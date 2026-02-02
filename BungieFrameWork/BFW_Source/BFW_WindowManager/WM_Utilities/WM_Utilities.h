// ======================================================================
// WM_Utilities.h
// ======================================================================
#ifndef WM_UTILITIES_H
#define WM_UTILITIES_H

#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"

// ======================================================================
// prototypes
// ======================================================================
UUtError
WMrCreate_Texture(
	UUtUns16			inWidth,
	UUtUns16			inHeight,
	void				**outTextureRef);

UUtError
WMrCreate_StringTexture(
	char				*inString,
	TStFontStyle		inStyle,
	TStFormat			inFormat,
	void				**outStringTexture,
	UUtRect				*outStringBounds);

void
WMrClearTexture(
	void				*inTexture,
	UUtRect				*inBounds,
	IMtPixel			inColor);

// ======================================================================
#endif /* WM_UTILITIES_H */
