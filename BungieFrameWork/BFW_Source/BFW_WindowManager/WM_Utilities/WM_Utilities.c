// ======================================================================
// WM_Utilities.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TextSystem.h"
#include "BFW_WindowManager.h"

#include "WM_Utilities.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrCreate_Texture(
	UUtUns16			inWidth,
	UUtUns16			inHeight,
	void				**outTextureRef)
{
	UUtError			error;

	*outTextureRef = NULL;

	// ------------------------------
	// create the appropriate texture map
	// ------------------------------
	if ((inWidth > M3cTextureMap_MaxWidth) ||
		(inHeight > M3cTextureMap_MaxHeight))
	{
		M3tTextureMap_Big		*texture_big;

		// create a big texture map
		error =
			M3rTextureMap_Big_New(
				inWidth,
				inHeight,
				WMcPrefered_PixelType,
				M3cTexture_AllocMemory,
				M3cTextureFlags_None,
				"VUrCreate_Texture Big",
				&texture_big);
		UUmError_ReturnOnErrorMsg(error, "Unable to create texture map");

		*outTextureRef = (void*)texture_big;
	}
	else
	{
		M3tTextureMap			*texture;

		// create a texture map
		error =
			M3rTextureMap_New(
				inWidth,
				inHeight,
				WMcPrefered_PixelType,
				M3cTexture_AllocMemory,
				M3cTextureFlags_None,
				"VUrCreate_Texture",
				&texture);
		UUmError_ReturnOnErrorMsg(error, "Unable to create texture map");

		*outTextureRef = (void*)texture;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
WMrCreate_StringTexture(
	char				*inString,
	TStFontStyle		inStyle,
	TStFormat			inFormat,
	void				**outStringTexture,
	UUtRect				*outStringBounds)
{
	UUtError			error;

	IMtPixel			background_color;

	UUtRect				bounds;
	IMtPoint2D			dest;

	UUmAssert(inString);
	UUmAssert(outStringTexture);
	UUmAssert(outStringBounds);

	*outStringTexture = NULL;
	outStringBounds->left	= 0;
	outStringBounds->top	= 0;
	outStringBounds->right	= 0;
	outStringBounds->bottom	= 0;

	// ------------------------------
	// set the colors to draw with
	// ------------------------------
	background_color = IMrPixel_FromShade(WMcPrefered_PixelType, WMcColor_Background);

	DCrText_SetShade(WMcColor_Text);

	// ------------------------------
	// set the style and format of the text
	// ------------------------------
	// set the style
	DCrText_SetStyle(inStyle);

	// set the format
	DCrText_SetFormat(inFormat);

	// ------------------------------
	// create the appropriate texture map
	// ------------------------------
	// get the bounds of the string
	DCrText_GetStringRect(inString, &bounds);

	// create the texture
	error = WMrCreate_Texture(bounds.right, bounds.bottom, outStringTexture);
	UUmError_ReturnOnErrorMsg(error, "Unable to create texture map");

	// clear the texture
	WMrClearTexture(*outStringTexture, NULL, background_color);

	// ------------------------------
	// draw the text on the texture
	// ------------------------------
	// set the dest
	dest.x = 0;
	dest.y = 0;

	// draw the text into the texture map
	DCrText_DrawString(
		*outStringTexture,
		inString,
		&bounds,
		&dest);

	*outStringBounds = bounds;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
WMrClearTexture(
	void				*inTexture,
	UUtRect				*inBounds,
	IMtPixel			inColor)
{
	TMtTemplateTag		tag;
	UUtRect				bounds;

	tag = TMrInstance_GetTemplateTag(inTexture);
	if (tag == M3cTemplate_TextureMap)
	{
		M3tTextureMap		*texture;

		texture = (M3tTextureMap*)inTexture;

		if (inBounds == NULL)
		{
			bounds.left = 0;
			bounds.top = 0;
			bounds.right = texture->width;
			bounds.bottom = texture->height;
		}
		else
		{
			bounds = *inBounds;
		}

		// fill the texture map with the background color
		M3rTextureMap_Fill(texture, inColor, &bounds);
	}
	else if (tag == M3cTemplate_TextureMap_Big)
	{
		M3tTextureMap_Big	*texture_big;

		texture_big = (M3tTextureMap_Big*)inTexture;

		if (inBounds == NULL)
		{
			bounds.left = 0;
			bounds.top = 0;
			bounds.right = texture_big->width;
			bounds.bottom = texture_big->height;
		}
		else
		{
			bounds = *inBounds;
		}

		// fill the texture map with the background color
		M3rTextureMap_Big_Fill(texture_big, inColor, &bounds);
	}
}

