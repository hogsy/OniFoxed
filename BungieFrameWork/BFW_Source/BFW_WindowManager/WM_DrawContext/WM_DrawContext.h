// ======================================================================
// WM_DrawContext.h
// ======================================================================
#ifndef WM_DRAWCONTEXT_H
#define WM_DRAWCONTEXT_H

#pragma once

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TextSystem.h"
#include "BFW_WindowManager.h"
#include "WM_PartSpecification.h"

// ======================================================================
// typedefs
// ======================================================================
typedef struct DCtDrawContext		DCtDrawContext;

// ======================================================================
// prototypes
// ======================================================================
UUtBool
WMrClipRect(
	const UUtRect			*inClipRect,
	UUtRect					*ioTextureRect,
	M3tTextureCoord			*ioUVs);

// ----------------------------------------------------------------------
DCtDrawContext*
DCrDraw_Begin(
	WMtWindow				*inWindow);

void
DCrDraw_End(
	DCtDrawContext			*inDrawContext,
	WMtWindow				*inWindow);

void
DCrDraw_Glyph(
	const DCtDrawContext	*inDrawContext,
	M3tPointScreen			*inScreenDest,
	M3tTextureCoord			*inUVs);
	
void
DCrDraw_Line(
	DCtDrawContext			*inDrawContext,
	IMtPoint2D				*inPoint1,
	IMtPoint2D				*inPoint2);

DCtDrawContext*
DCrDraw_NC_Begin(
	WMtWindow				*inWindow);
	
void
DCrDraw_NC_End(
	DCtDrawContext			*inDrawContext,
	WMtWindow				*inWindow);
	
void
DCrDraw_PartSpec(
	DCtDrawContext			*inDrawContext,
	PStPartSpec				*inPartSpec,
	UUtUns16				inFlags,
	IMtPoint2D				*inLocation,
	UUtInt16				inWidth,
	UUtInt16				inHeight,
	UUtUns16				inAlpha);

void
DCrDraw_SetLineColor(
	DCtDrawContext			*inDrawContext,
	UUtUns16				inShade);

void
DCrDraw_String2(
	DCtDrawContext			*inDrawContext,
	const char				*inString,
	const UUtRect			*inBounds,
	const IMtPoint2D		*inDestination,
	UUtRect					*outStringRect);
	
void
DCrDraw_String(
	DCtDrawContext			*inDrawContext,
	const char				*inString,
	const UUtRect			*inBounds,
	const IMtPoint2D		*inDestination);
	
void
DCrDraw_TextureRef(
	DCtDrawContext			*inDrawContext,
	void					*inTextureRef,
	IMtPoint2D				*inLocation,
	UUtInt16				inWidth,
	UUtInt16				inHeight,
	UUtBool					inScale,
	UUtUns16				inAlpha);

// ----------------------------------------------------------------------
void
DCrText_DrawString(
	void					*inTextureRef,
	char					*inString,
	UUtRect					*inBounds,
	IMtPoint2D				*inDestination);
	
void
DCrText_DrawText(
	char			*inString,
	UUtRect			*inBounds,
	IMtPoint2D		*inDestination);
	
UUtUns16
DCrText_GetAscendingHeight(
	void);
	
const TStTextContext*
DCrText_GetTextContext(
	void);
	
UUtUns16
DCrText_GetDescendingHeight(
	void);
	
TStFont*
DCrText_GetFont(
	void);
	
UUtUns16
DCrText_GetLeadingHeight(
	void);
	
UUtUns16
DCrText_GetLineHeight(
	void);

void
DCrText_GetStringRect(
	char					*inString,
	UUtRect					*outRect);

UUtError
DCrText_Initialize(
	void);

void
DCrText_SetFontFamily(
	TStFontFamily			*inFontFamily);
	
void
DCrText_SetFontInfo(
	TStFontInfo				*inFontInfo);
	
void
DCrText_SetFormat(
	TStFormat				inFormat);
	
void
DCrText_SetShade(
	UUtUns32				inShade);
	
void
DCrText_SetSize(
	UUtUns16				inSize);
	
void
DCrText_SetStyle(
	TStFontStyle			inStyle);
	
void
DCrText_Terminate(
	void);

// ======================================================================
#endif /* WM_DRAWCONTEXT_H */