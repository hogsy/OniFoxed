// ======================================================================
// BFW_ViewUtilities.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewUtilities.h"
#include "BFW_TextSystem.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
void
VUrDrawPartSpecification(
	VMtPartSpec				*inPartSpec,
	UUtUns16				inFlags,
	M3tPointScreen			*inLocation,
	UUtUns16				inWidth,
	UUtUns16				inHeight,
	UUtUns16				inAlpha)
{
	UUtUns32				shade;
	M3tTextureMap			*texture;
	VMtPartSpec_PrivateData	*private_data;
	M3tPointScreen			dest;
	
	UUtUns16				t_height;
	UUtUns16				m_height;
	UUtUns16				b_height;

	UUtUns16				l_width;
	UUtUns16				m_width;
	UUtUns16				r_width;
	
	// get the private data
	private_data = (VMtPartSpec_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(DMgTemplate_PartSpec_PrivateData, inPartSpec);
	UUmAssert(private_data);
	
	texture = (M3tTextureMap*)inPartSpec->texture;
	
	// set the shade and alpha
	shade = IMcShade_White;
	
	// calculate the heights and widths
	t_height = UUmABS(inPartSpec->part_matrix_br[0][0].y - inPartSpec->part_matrix_tl[0][0].y);
	b_height = UUmABS(inPartSpec->part_matrix_br[0][2].y - inPartSpec->part_matrix_tl[0][2].y);
	if ((t_height + b_height) > inHeight)
	{
		t_height = t_height * t_height / inHeight;
		b_height = b_height * b_height / inHeight;
	}
	m_height = (UUtUns16)UUmMax(0, inHeight - (t_height + b_height));
	
	l_width = UUmABS(inPartSpec->part_matrix_br[0][0].x - inPartSpec->part_matrix_tl[0][0].x);
	r_width = UUmABS(inPartSpec->part_matrix_br[2][0].x - inPartSpec->part_matrix_tl[2][0].x);
	if ((l_width + r_width) > inWidth)
	{
		l_width = l_width * l_width / inWidth;
		r_width = r_width * r_width / inWidth;
	}
	m_width = (UUtUns16)UUmMax(0, inWidth - (l_width + r_width));
	
	if (inFlags == VMcPart_MiddleMiddle)
	{
		m_height = inHeight;
		m_width = inWidth;
	}
	
	// draw the left column
	dest = *inLocation;

	if (inFlags & VMcPart_LeftTop)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->lt,
			&dest,
			l_width,
			t_height,
			shade,
			inAlpha);
		dest.y += t_height;
	}

	if (inFlags & VMcPart_LeftMiddle)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->lm,
			&dest,
			l_width,
			m_height,
			shade,
			inAlpha);
		dest.y += m_height;
	}

	if (inFlags & VMcPart_LeftBottom)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->lb,
			&dest,
			l_width,
			b_height,
			shade,
			inAlpha);
	}
	
	// draw the middle column
	dest = *inLocation;
	if (inFlags & VMcPart_LeftColumn)
	{
		dest.x += l_width;
	}

	if (inFlags & VMcPart_MiddleTop)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->mt,
			&dest,
			m_width,
			t_height,
			shade,
			inAlpha);
		dest.y += t_height;
	}

	if (inFlags & VMcPart_MiddleMiddle)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->mm,
			&dest,
			m_width,
			m_height,
			shade,
			inAlpha);
		dest.y += m_height;
	}

	if (inFlags & VMcPart_MiddleBottom)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->mb,
			&dest,
			m_width,
			b_height,
			shade,
			inAlpha);
	}
	
	// draw the right column
	dest = *inLocation;
	if (inFlags & VMcPart_MiddleColumn)
	{
		dest.x += l_width + m_width;
	}
	
	if (inFlags & VMcPart_RightTop)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->rt,
			&dest,
			r_width,
			t_height,
			shade,
			inAlpha);
		dest.y += t_height;
	}

	if (inFlags & VMcPart_RightMiddle)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->rm,
			&dest,
			r_width,
			m_height,
			shade,
			inAlpha);
		dest.y += m_height;
	}

	if (inFlags & VMcPart_RightBottom)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->rb,
			&dest,
			r_width,
			b_height,
			shade,
			inAlpha);
	}
}

// ----------------------------------------------------------------------
void
VUrDrawTextureRef(
	void				*inTextureRef,
	M3tPointScreen		*inLocation,
	UUtUns16			inWidth,
	UUtUns16			inHeight,
	UUtUns16			inAlpha)
{
	TMtTemplateTag		template_tag;
	UUtUns32			shade;
	UUtUns16			width;
	UUtUns16			height;
	
	UUmAssert(inLocation);
	
	if (inTextureRef == NULL) return;
	
	// set the shade and alpha
	shade = IMcShade_White;

	// get the template tag of the current background
	template_tag = TMrInstance_GetTemplateTag(inTextureRef);
	
	// set the width and height
	width = inWidth;
	height = inHeight;
		
	switch (template_tag)
	{
		case M3cTemplate_TextureMap:
		{
			M3tTextureMap			*texture;
			
			// get a a pointer to the texture
			texture = inTextureRef;
			
			if (inWidth == (UUtUns16)(-1)) width = texture->width;
			if (inHeight == (UUtUns16)(-1)) height = texture->height;
			
			// draw background
			M3rDraw_Bitmap(
				texture,
				inLocation,
				width,
				height,
				shade,
				inAlpha);
		}
		break;
		
		case M3cTemplate_TextureMap_Big:
		{
			M3tTextureMap_Big		*texture_big;
			
			// get a pointer to the texture
			texture_big = inTextureRef;
			
			if (inWidth == (UUtUns16)(-1)) width = texture_big->width;
			if (inHeight == (UUtUns16)(-1)) height = texture_big->height;
			
			// draw background
			M3rDraw_BigBitmap(
				texture_big,
				inLocation,
				width,
				height,
				shade,
				inAlpha);
		}
		break;
		
		default:
			UUmAssert(!"Unknown texture map template tag.");
		break;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
VUrCreate_Texture(
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
				VMcControl_PixelType,
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
				VMcControl_PixelType,
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
VUrCreate_StringTexture(
	char				*inString,
	UUtUns16			inFlags,
	void				**outStringTexture,
	UUtRect				*outStringBounds)
{
	UUtError			error;
	
	IMtPixel			background_color;
	IMtPixel			text_color;

	UUtRect				bounds;
	TStTextContext		*text_context;
	TStFontFamily		*font_family;
	char				*desired_font_family;
	TStFontStyle		desired_font_style;
	UUtUns16			desired_font_format;
	
	IMtPoint2D			dest;

	*outStringTexture = NULL;
	outStringBounds->left	= 0;
	outStringBounds->top	= 0;
	outStringBounds->right	= 0;
	outStringBounds->bottom	= 0;
	
	// ------------------------------
	// set the colors to draw with
	// ------------------------------
	background_color = IMrPixel_FromShade(VMcControl_PixelType, VMcColor_Background);
	text_color = IMrPixel_FromShade(VMcControl_PixelType, VMcColor_Text);
	
	// ------------------------------
	// create the text context and draw the string
	// ------------------------------
	// interpret the flags
	if (inFlags & VMcCommonFlag_Text_Small)
		desired_font_family = TScFontFamily_RomanSmall;
	else
		desired_font_family = TScFontFamily_Roman;
	
	if (inFlags & VMcCommonFlag_Text_Bold)
		desired_font_style = TScStyle_Bold;
	else if (inFlags & VMcCommonFlag_Text_Italic)
		desired_font_style = TScStyle_Italic;
	else
		desired_font_style = TScStyle_Plain;
	
	if (inFlags & VMcCommonFlag_Text_HCenter)
		desired_font_format = TSc_HCenter;
	else if (inFlags & VMcCommonFlag_Text_HRight)
		desired_font_format = TSc_HRight;
	else
		desired_font_format = TSc_HLeft;
	
	desired_font_format |= TSc_VCenter;
		
	// get the font family
	error =
		TMrInstance_GetDataPtr(
			TScTemplate_FontFamily,
			desired_font_family,
			&font_family);
	UUmError_ReturnOnErrorMsg(error, "Unable to load font family");
	
	// create a new text context
	error =
		TSrContext_New(
			font_family,
			desired_font_style,
			desired_font_format,
			&text_context);
	UUmError_ReturnOnErrorMsg(error, "Unable to create text context");
	
	// set the color of the text
	error = TSrContext_SetColor(text_context, text_color);
	UUmError_ReturnOnErrorMsg(error, "Unable to set the text color");

	// get the bounding rect for the string
	TSrContext_GetStringRect(text_context, inString, &bounds);

	// ------------------------------
	// create the appropriate texture map
	// ------------------------------
	
	// create the texture
	error = VUrCreate_Texture(bounds.right, bounds.bottom, outStringTexture);
	UUmError_ReturnOnErrorMsg(error, "Unable to create texture map");
	
	// fill in the texture
	VUrClearTexture(*outStringTexture, NULL, background_color);
	
	// ------------------------------
	// draw the text on the texture
	// ------------------------------
	
	// set the dest
	dest.x = 0;
	dest.y = 0;
	
	// draw the text into the texture map
	error =
		TSrContext_DrawString(
			text_context,
			*outStringTexture,
			(char*)inString,
			&bounds,
			&dest);
	UUmError_ReturnOnErrorMsg(error, "Unable to draw string in texture");

	// dispose of the text context
	TSrContext_Delete(text_context);
	text_context = NULL;
	
	*outStringBounds = bounds;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
void
VUrClearTexture(
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
	