// ======================================================================
// WM_DrawContext.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_TextSystem.h"
#include "BFW_WindowManager.h"
#include "BFW_WindowManager_Private.h"

#include "WM_DrawContext.h"

// ======================================================================
// defines
// ======================================================================
#define DCcDC_MaxListElements			3	/* only one seems to ever get used at a time */

#define DCcDefault_Z					0.5f

// ======================================================================
// structs
// ======================================================================
struct DCtDrawContext
{
	UUtUns32				id;
	WMtWindow				*window;
	UUtRect					clip_rect;
	M3tPointScreen			screen_dest;
	UUtUns32				shade;
	UUtInt16				x_offset;
	UUtInt16				y_offset;

};

// ======================================================================
// typedefs
// ======================================================================
typedef struct DCtDC_ListElement
{
	UUtBool					inUse;
	DCtDrawContext			draw_context;

} DCtDC_ListElement;

// ======================================================================
// globals
// ======================================================================
static DCtDC_ListElement	DCgDC_List[DCcDC_MaxListElements];

static TStTextContext		*DCgTextContext;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
WMrClipRect(
	const UUtRect			*inClipRect,
	UUtRect					*ioTextureRect,
	M3tTextureCoord			*ioUVs)
{
	UUmAssert(ioTextureRect);
	UUmAssert(ioUVs);

	// don't clip if inClipRect is NULL
	if (inClipRect == NULL)
	{
		return UUcTrue;
	}

	// do trivial reject
	if ((ioTextureRect->right < inClipRect->left) ||
		(ioTextureRect->left > inClipRect->right) ||
		(ioTextureRect->bottom < inClipRect->top) ||
		(ioTextureRect->top > inClipRect->bottom))
	{
		return UUcFalse;
	}

	// clip
	if (ioTextureRect->left < inClipRect->left)
	{
		ioUVs[0].u = ioUVs[2].u =
			ioUVs[1].u -
			((ioUVs[1].u - ioUVs[0].u) *
			((float)(ioTextureRect->right - inClipRect->left) /
				(float)(ioTextureRect->right - ioTextureRect->left)));
		ioTextureRect->left = inClipRect->left;
	}

	if (ioTextureRect->top < inClipRect->top)
	{
		ioUVs[0].v = ioUVs[1].v =
			ioUVs[2].v -
			((ioUVs[2].v - ioUVs[0].v) *
			((float)(ioTextureRect->bottom - inClipRect->top) /
				(float)(ioTextureRect->bottom - ioTextureRect->top)));
		ioTextureRect->top = inClipRect->top;
	}

	if (ioTextureRect->right > inClipRect->right)
	{
		ioUVs[1].u = ioUVs[3].u =
			ioUVs[0].u +
			((ioUVs[1].u - ioUVs[0].u) *
			((float)(inClipRect->right - ioTextureRect->left) /
				(float)(ioTextureRect->right - ioTextureRect->left)));
		ioTextureRect->right = inClipRect->right;
	}

	if (ioTextureRect->bottom > inClipRect->bottom)
	{
		ioUVs[2].v = ioUVs[3].v =
			ioUVs[0].v +
			((ioUVs[2].v - ioUVs[0].v) *
			((float)(inClipRect->bottom - ioTextureRect->top) /
				(float)(ioTextureRect->bottom - ioTextureRect->top)));
		ioTextureRect->bottom = inClipRect->bottom;
	}

	return UUcTrue;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
DCtDrawContext*
DCrDraw_Begin(
	WMtWindow				*inWindow)
{
	UUtUns32				i;
	DCtDrawContext			*draw_context;

	UUmAssert(inWindow);

	// get a draw context to draw with
	draw_context = NULL;
	for (i = 0; i < DCcDC_MaxListElements; i++)
	{
		if (DCgDC_List[i].inUse == UUcFalse)
		{
			draw_context = &DCgDC_List[i].draw_context;
			break;
		}
	}

	if (draw_context != NULL)
	{
		UUtRect				window_rect;
		UUtRect				client_rect;
		UUtInt16			dest_x;
		UUtInt16			dest_y;
		UUtInt16			x_offset;
		UUtInt16			y_offset;

		// set the x and y offset
		x_offset = 0;
		y_offset = 0;

		// get the client rect of the window
		WMrWindow_GetRect(inWindow, &window_rect);
		client_rect = inWindow->client_rect;

		// if this window is a child, it has to fit inside it's parent's client rect
		if (inWindow->flags & WMcWindowFlag_Child)
		{
			WMtWindow			*window;
			UUtRect				offset_rect;

			// put inWindow's client rect in world coordinates
			IMrRect_Offset(&client_rect, window_rect.left, window_rect.top);

			// save the client_rect to calculate the x and y offset later
			offset_rect = client_rect;

			// go up the list of parents and clip client_rect to the parent's client_rect
			window = inWindow->parent;
			while (window)
			{
				UUtRect			win_rect;
				UUtRect			clip_rect;

				// get the clip rect from window and put it in global coordinates
				WMrWindow_GetRect(window, &win_rect);
				clip_rect = window->client_rect;
				IMrRect_Offset(&clip_rect, win_rect.left, win_rect.top);

				// clip the client_rect to the parent
				IMrRect_Intersect(&client_rect, &clip_rect, &client_rect);

				// stop when a top level window has been reached
				if ((window->flags & WMcWindowFlag_Child) == 0) { break; }

				// get the next parent
				window = window->parent;
			}

			// calculate the x and y offset
			x_offset = offset_rect.left - client_rect.left;
			y_offset = offset_rect.top - client_rect.top;

			dest_x = client_rect.left;
			dest_y = client_rect.top;
		}
		else
		{
			dest_x = client_rect.left + window_rect.left;
			dest_y = client_rect.top + window_rect.top;
		}

		IMrRect_Offset(&client_rect, -client_rect.left, -client_rect.top);

		// initialize the draw_context
		draw_context->id = i;
		draw_context->window = inWindow;
		draw_context->clip_rect = client_rect;
		draw_context->screen_dest.x = (float)(dest_x);
		draw_context->screen_dest.y = (float)(dest_y);
		draw_context->screen_dest.z = DCcDefault_Z;
		draw_context->screen_dest.invW = 1.0f / DCcDefault_Z;
		draw_context->shade = IMcShade_White;
		draw_context->x_offset = x_offset;
		draw_context->y_offset = y_offset;
	}

	return draw_context;
}

// ----------------------------------------------------------------------
void
DCrDraw_End(
	DCtDrawContext			*inDrawContext,
	WMtWindow				*inWindow)
{
	UUmAssert(inDrawContext);
	UUmAssert(inWindow);

	DCgDC_List[inDrawContext->id].inUse = UUcFalse;
}

// ----------------------------------------------------------------------
// only call from BFW_TextSystem
void
DCrDraw_Glyph(
	const DCtDrawContext	*inDrawContext,
	M3tPointScreen			*inScreenDest,
	M3tTextureCoord			*inUVs)
{
	UUtRect					screen_dest;

	screen_dest.left = (UUtInt16)inScreenDest[0].x;
	screen_dest.top = (UUtInt16)inScreenDest[0].y;
	screen_dest.right = (UUtInt16)inScreenDest[1].x;
	screen_dest.bottom = (UUtInt16)inScreenDest[1].y;

	// clip the inScreenDest coords to inDrawContext->clip_rect;
	if (WMrClipRect(&inDrawContext->clip_rect, &screen_dest, inUVs))
	{
		inScreenDest[0].x = (float)screen_dest.left + inDrawContext->screen_dest.x;
		inScreenDest[0].y = (float)screen_dest.top + inDrawContext->screen_dest.y;
		inScreenDest[1].x = (float)screen_dest.right + inDrawContext->screen_dest.x;
		inScreenDest[1].y = (float)screen_dest.bottom + inDrawContext->screen_dest.y;

		M3rDraw_Sprite(inScreenDest, inUVs);
	}
}

// ----------------------------------------------------------------------
void
DCrDraw_Line(
	DCtDrawContext			*inDrawContext,
	IMtPoint2D				*inPoint1,
	IMtPoint2D				*inPoint2)
{
/*	M3tPointScreen			dest[2];

	UUmAssert(inDrawContext);

	dest[1] = inDrawContext->screen_dest;
	dest[1].x += (float)inPoint1->x;
	dest[1].y += (float)inPoint1->y;

	dest[2] = inDrawContext->screen_dest;
	dest[2].x += (float)inPoint2->x;
	dest[2].y += (float)inPoint2->y;

	M3rGeometry_LineDraw2D(2, dest, inDrawContext->shade);*/
}

// ----------------------------------------------------------------------
DCtDrawContext*
DCrDraw_NC_Begin(
	WMtWindow				*inWindow)
{
	UUtUns32				i;
	DCtDrawContext			*draw_context;

	UUmAssert(inWindow);

	// get a draw context to draw with
	draw_context = NULL;
	for (i = 0; i < DCcDC_MaxListElements; i++)
	{
		if (DCgDC_List[i].inUse == UUcFalse)
		{
			draw_context = &DCgDC_List[i].draw_context;
			break;
		}
	}

	if (draw_context != NULL)
	{
		UUtRect				rect;
		UUtRect				offset_rect;
		UUtInt16			x_offset;
		UUtInt16			y_offset;

		// set the x and y offset
		x_offset = 0;
		y_offset = 0;

		// get the rect of the window
		WMrWindow_GetRect(inWindow, &rect);

		if (inWindow->flags & WMcWindowFlag_Child)
		{
			WMtWindow		*parent;

			// save rect for use when calculate the x and y offste
			offset_rect = rect;

			// go through the parents and clip to their client rects
			parent = inWindow->parent;
			while (parent)
			{
				UUtRect			win_rect;
				UUtRect			clip_rect;

				// get the clip rect from the parent and put it into world coordinates
				WMrWindow_GetRect(parent, &win_rect);
				clip_rect = parent->client_rect;
				IMrRect_Offset(&clip_rect, win_rect.left, win_rect.top);

				// clip the rect to the clip rect
				IMrRect_Intersect(&rect, &clip_rect, &rect);

				// calculate the x and y offset
				x_offset = offset_rect.left - rect.left;
				y_offset = offset_rect.top - rect.top;

				// stop when parent is no longer a child window
				if ((parent->flags & WMcWindowFlag_Child) == 0)	{ break; }

				// get the next parent
				parent = parent->parent;
			}
		}

		// initialize the draw context
		draw_context->id = i;
		draw_context->window = inWindow;
		draw_context->clip_rect = rect;
		draw_context->screen_dest.x = (float)rect.left;
		draw_context->screen_dest.y = (float)rect.top;
		draw_context->screen_dest.z = DCcDefault_Z;
		draw_context->screen_dest.invW = 1.0f / DCcDefault_Z;
		draw_context->shade = IMcShade_White;
		draw_context->x_offset = x_offset;
		draw_context->y_offset = y_offset;

		IMrRect_Offset(&draw_context->clip_rect, -rect.left, -rect.top);
	}

	return draw_context;
}

// ----------------------------------------------------------------------
void
DCrDraw_NC_End(
	DCtDrawContext			*inDrawContext,
	WMtWindow				*inWindow)
{
	UUmAssert(inDrawContext);
	UUmAssert(inWindow);

	DCgDC_List[inDrawContext->id].inUse = UUcFalse;
}

// ----------------------------------------------------------------------
void
DCrDraw_PartSpec(
	DCtDrawContext			*inDrawContext,
	PStPartSpec				*inPartSpec,
	UUtUns16				inFlags,
	IMtPoint2D				*inLocation,
	UUtInt16				inWidth,
	UUtInt16				inHeight,
	UUtUns16				inAlpha)
{
	UUtUns32				shade;
	M3tTextureMap			*texture;
	PStPartSpec_PrivateData	*private_data;
	IMtPoint2D				dest;

	UUtInt16				l_width;
	UUtInt16				m_width;
	UUtInt16				r_width;

	UUtInt16				t_height;
	UUtInt16				m_height;
	UUtInt16				b_height;

	UUtRect					bounds;
	M3tTextureCoord			uv[4];
	M3tPointScreen			screen_dest[2];

	UUtUns16				i;
	UUtUns16				j;

	UUtInt16				width;
	UUtInt16				height;

	M3tTextureCoord			*uv_list;

	UUtUns16				columns[3] =
		{PScPart_LeftColumn, PScPart_MiddleColumn, PScPart_RightColumn};

	UUtUns16				parts[3][3] = {
		{PScPart_LeftTop, PScPart_LeftMiddle, PScPart_LeftBottom},
		{PScPart_MiddleTop, PScPart_MiddleMiddle, PScPart_MiddleBottom},
		{PScPart_RightTop, PScPart_RightMiddle, PScPart_RightBottom}};

	// CB: prevents integer division by zero nastiness
	if ((inWidth == 0) || (inHeight == 0))
		return;

	UUmAssert(inDrawContext);
	UUmAssert(inPartSpec);
	UUmAssert(inLocation);

	// get the private data
	private_data = (PStPartSpec_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(PSgTemplate_PartSpec_PrivateData, inPartSpec);
	UUmAssert(private_data);

	texture = (M3tTextureMap*)inPartSpec->texture;

	// set the shade and alpha
	shade = IMcShade_White;

	// calculate the heights and widths
	if ((private_data->t_height + private_data->b_height) > inHeight)
	{
		if (private_data->t_height > inHeight)
		{
			t_height = inHeight;
		}
		else
		{
			t_height = private_data->t_height * private_data->t_height / inHeight;
		}

		if (private_data->b_height > inHeight)
		{
			b_height = inHeight;
		}
		else
		{
			b_height = private_data->b_height * private_data->b_height / inHeight;
		}
	}
	else
	{
		t_height = private_data->t_height;
		b_height = private_data->b_height;
	}
	m_height = (UUtUns16)UUmMax(0, inHeight - (t_height + b_height));

	if ((private_data->l_width + private_data->r_width) > inWidth)
	{
		if (private_data->l_width > inWidth)
		{
			l_width = inWidth;
		}
		else
		{
			l_width = private_data->l_width * private_data->l_width / inWidth;
		}

		if (private_data->r_width > inWidth)
		{
			r_width = inWidth;
		}
		else
		{
			r_width = private_data->r_width * private_data->r_width / inWidth;
		}
	}
	else
	{
		l_width = private_data->l_width;
		r_width = private_data->r_width;
	}
	m_width = (UUtUns16)UUmMax(0, inWidth - (l_width + r_width));

	if (inFlags == PScPart_MiddleMiddle)
	{
		m_height = inHeight;
		m_width = inWidth;
	}

	// set up the states
	M3rDraw_State_Push();

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Appearence,
		M3cDrawState_Appearence_Texture_Unlit);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_None);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Fill,
		M3cDrawState_Fill_Solid);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		shade);

	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Alpha,
		inAlpha);

	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_BaseTextureMap,
		texture);

	M3rDraw_State_Commit();

	for (i = 0; i < 3; i++)
	{
		// go through each column and setup the destination
		dest = *inLocation;
		dest.x += inDrawContext->x_offset;
		dest.y += inDrawContext->y_offset;

		switch (columns[i])
		{
			case PScPart_LeftColumn:
				width = l_width;
			break;

			case PScPart_MiddleColumn:
				width = m_width;

				if (inFlags & PScPart_LeftColumn)
				{
					dest.x += l_width;
				}
			break;

			case PScPart_RightColumn:
				width = r_width;

				if (inFlags & PScPart_LeftColumn)
				{
					dest.x += l_width;
				}

				if (inFlags & PScPart_MiddleColumn)
				{
					dest.x += m_width;
				}
			break;
		}

		// draw the visible parts in each column
		for (j = 0; j < 3; j++)
		{
			UUtBool			draw;

			draw = UUcTrue;

			switch (inFlags & parts[i][j])
			{
				case PScPart_LeftTop:
					height = t_height;
					uv_list = private_data->lt;
				break;

				case PScPart_LeftMiddle:
					height = m_height;
					uv_list = private_data->lm;
				break;

				case PScPart_LeftBottom:
					height = b_height;
					uv_list = private_data->lb;
				break;

				case PScPart_MiddleTop:
					height = t_height;
					uv_list = private_data->mt;
				break;

				case PScPart_MiddleMiddle:
					height = m_height;
					uv_list = private_data->mm;
				break;

				case PScPart_MiddleBottom:
					height = b_height;
					uv_list = private_data->mb;
				break;

				case PScPart_RightTop:
					height = t_height;
					uv_list = private_data->rt;
				break;

				case PScPart_RightMiddle:
					height = m_height;
					uv_list = private_data->rm;
				break;

				case PScPart_RightBottom:
					height = b_height;
					uv_list = private_data->rb;
				break;

				default:
					draw = UUcFalse;
				break;
			}

			if (draw)
			{
				bounds.left		= dest.x;
				bounds.top		= dest.y;
				bounds.right	= bounds.left + width;
				bounds.bottom	= bounds.top + height;

				uv[0] = uv_list[0];
				uv[1] = uv_list[1];
				uv[2] = uv_list[2];
				uv[3] = uv_list[3];

				if (WMrClipRect(&inDrawContext->clip_rect, &bounds, uv))
				{
					screen_dest[0] = inDrawContext->screen_dest;
					screen_dest[1] = inDrawContext->screen_dest;
					screen_dest[0].x += (float)bounds.left;
					screen_dest[0].y += (float)bounds.top;
					screen_dest[1].x += (float)bounds.right;
					screen_dest[1].y += (float)bounds.bottom;

					M3rDraw_Sprite(
						screen_dest,
						uv);
				}

				dest.y += height;
			}
		}
	}

	M3rDraw_State_Pop();

	return;
}

// ----------------------------------------------------------------------
void
DCrDraw_SetLineColor(
	DCtDrawContext			*inDrawContext,
	UUtUns16				inShade)
{
	UUmAssert(inDrawContext);

	inDrawContext->shade = inShade;
}

// ----------------------------------------------------------------------
void
DCrDraw_String2(
	DCtDrawContext			*inDrawContext,
	const char				*inString,
	const UUtRect			*inBounds,
	const IMtPoint2D		*inDestination,
	UUtRect					*outStringRect)
{
	UUtRect					bounds;

	UUmAssert(inDrawContext);
	UUmAssert(inString);
	UUmAssert(inDestination);

	// handle inBounds of NULL
	if (inBounds == NULL)
	{
		WMrWindow_GetClientRect(inDrawContext->window, &bounds);
	}
	else
	{
		bounds = *inBounds;
	}

	// do a trivial reject
	if (((bounds.right - bounds.left) <= 0) ||
		((bounds.bottom - bounds.top) <= 0))
	{
		return;
	}

	if (IMrRect_PointIn(&bounds, inDestination) == UUcFalse)
	{
		return;
	}

	// draw the string
	TSrContext_DrawText_DC(
		DCgTextContext,
		inString,
		&bounds,
		inDestination,
		inDrawContext,
		outStringRect);
}

// ----------------------------------------------------------------------
void
DCrDraw_String(
	DCtDrawContext			*inDrawContext,
	const char				*inString,
	const UUtRect			*inBounds,
	const IMtPoint2D		*inDestination)
{
	DCrDraw_String2(inDrawContext, inString, inBounds, inDestination, NULL);
}

// ----------------------------------------------------------------------
void
DCrDraw_TextureRef(
	DCtDrawContext			*inDrawContext,
	void					*inTextureRef,
	IMtPoint2D				*inLocation,
	UUtInt16				inWidth,
	UUtInt16				inHeight,
	UUtBool					inScale,
	UUtUns16				inAlpha)
{
	TMtTemplateTag			template_tag;
	UUtUns32				shade;
	UUtInt16				draw_width;
	UUtInt16				draw_height;
	M3tTextureMap			*texture;
	M3tTextureMap_Big		*texture_big;
	IMtPoint2D				offset_location;

	UUmAssert(inDrawContext);
	UUmAssert(inTextureRef);
	UUmAssert(inLocation);

	if (inTextureRef == NULL) return;

	// get the template tag of the current background
	template_tag = TMrInstance_GetTemplateTag(inTextureRef);

	// set the shade and alpha
	shade = IMcShade_White;

	// offset the location
	offset_location.x = inLocation->x + inDrawContext->x_offset;
	offset_location.y = inLocation->y + inDrawContext->y_offset;

	switch (template_tag)
	{
		case M3cTemplate_TextureMap:
		{
			M3tPointScreen			dest[2];
			M3tTextureCoord			uv[4];
			UUtRect					texture_rect;

			// get a pointer to the texture
			texture = inTextureRef;

			if (inWidth == -1)
				draw_width = texture->width;
			else
				draw_width = inWidth;

			if (inHeight == -1)
				draw_height = texture->height;
			else
				draw_height = inHeight;

			// set UVs
			// br
			if (inScale)
			{
				uv[3].u = 0.999f;
				uv[3].v = 0.999f;
			}
			else
			{
				uv[3].u = (float)draw_width / (float)texture->width;
				uv[3].v = (float)draw_height / (float)texture->height;
			}

			// tl
			uv[0].u = 0.f;
			uv[0].v = 0.f;

			// tr
			uv[1].u = uv[3].u;
			uv[1].v = 0.f;

			// bl
			uv[2].u = 0.f;
			uv[2].v = uv[3].v;

			// build the texture rect
			texture_rect.left	= offset_location.x;
			texture_rect.top	= offset_location.y;
			texture_rect.right	= texture_rect.left + draw_width;
			texture_rect.bottom	= texture_rect.top + draw_height;

			// clip the rect
			if (WMrClipRect(&inDrawContext->clip_rect, &texture_rect, uv) == UUcFalse)
			{
				return;
			}

			// set dest
			dest[0] = inDrawContext->screen_dest;
			dest[1] = inDrawContext->screen_dest;

			dest[0].x += (float)texture_rect.left;
			dest[0].y += (float)texture_rect.top;
			dest[1].x += (float)texture_rect.right;
			dest[1].y += (float)texture_rect.bottom;

			M3rDraw_State_Push();

			M3rDraw_State_SetInt(
				M3cDrawStateIntType_Appearence,
				M3cDrawState_Appearence_Texture_Unlit);

			M3rDraw_State_SetInt(
				M3cDrawStateIntType_Interpolation,
				M3cDrawState_Interpolation_None);

			M3rDraw_State_SetInt(
				M3cDrawStateIntType_Fill,
				M3cDrawState_Fill_Solid);

			M3rDraw_State_SetInt(
				M3cDrawStateIntType_ConstantColor,
				shade);

			M3rDraw_State_SetInt(
				M3cDrawStateIntType_Alpha,
				inAlpha);

			M3rDraw_State_SetPtr(
				M3cDrawStatePtrType_BaseTextureMap,
				texture);

			M3rDraw_State_Commit();

			M3rDraw_Sprite(
				dest,
				uv);

			M3rDraw_State_Pop();
		}
		break;

		case M3cTemplate_TextureMap_Big:
		{
			UUtUns16				x;
			UUtUns16				y;

			// get a pointer to the texture
			texture_big = inTextureRef;

			if (inWidth == -1)
				draw_width = texture_big->width;
			else
				draw_width = inWidth;

			if (inHeight == -1)
				draw_height = texture_big->height;
			else
				draw_height = inHeight;

			// draw the subtextures that are visible
			for (y = 0; y < texture_big->num_y; y++)
			{
				for (x = 0; x < texture_big->num_x; x++)
				{
					UUtUns16			index;
					IMtPoint2D			texture_dest;
					UUtUns16			width;
					UUtUns16			height;

					UUtInt16			temp;
					UUtInt16			x_times_maxwidth;
					UUtInt16			y_times_maxheight;

					// get a pointer to the texture to be drawn
					index = x + (y * texture_big->num_x);
					texture = texture_big->textures[index];

					x_times_maxwidth = x * M3cTextureMap_MaxWidth;
					y_times_maxheight = y * M3cTextureMap_MaxHeight;

					// calculate the dest_point of the texture
					texture_dest.x = offset_location.x + x_times_maxwidth;
					texture_dest.y = offset_location.y + y_times_maxheight;

					// calculate the width and height
					temp = (UUtInt16)UUmMax(0, draw_width - x_times_maxwidth);
					width = UUmMin(M3cTextureMap_MaxWidth, temp);
					temp = (UUtInt16)UUmMax(0, draw_height - y_times_maxheight);
					height = UUmMin(M3cTextureMap_MaxHeight, temp);

					// draw the sub_texture
					DCrDraw_TextureRef(
						inDrawContext,
						texture,
						&texture_dest,
						width,
						height,
						UUcFalse,
						inAlpha);
				}
			}
		}
		break;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
DCrText_DrawString(
	void					*inTextureRef,
	char					*inString,
	UUtRect					*inBounds,
	IMtPoint2D				*inDestination)
{
	UUtRect					bounds;
	IMtPoint2D				dest;

	UUmAssert(DCgTextContext);

	if (inBounds == NULL)
	{
		TSrContext_GetStringRect(
			DCgTextContext,
			inString,
			&bounds);
	}
	else
	{
		bounds = *inBounds;
	}

	if (inDestination == NULL)
	{
		dest.x = 0;
		dest.y = DCrText_GetAscendingHeight();
	}
	else
	{
		dest = *inDestination;
	}

	TSrContext_DrawString(
		DCgTextContext,
		inTextureRef,
		inString,
		&bounds,
		&dest);
}

// ----------------------------------------------------------------------
void
DCrText_DrawText(
	char			*inString,
	UUtRect			*inBounds,
	IMtPoint2D		*inDestination)
{
	TSrContext_DrawText(DCgTextContext, inString, M3cMaxAlpha, inBounds, inDestination);
}

// ----------------------------------------------------------------------
UUtUns16
DCrText_GetAscendingHeight(
	void)
{
	UUtUns16				descending_height;

	UUmAssert(DCgTextContext);

	descending_height =
		TSrFont_GetAscendingHeight(TSrContext_GetFont(DCgTextContext, TScStyle_InUse));

	return descending_height;
}

// ----------------------------------------------------------------------
const TStTextContext*
DCrText_GetTextContext(
	void)
{
	return DCgTextContext;
}

// ----------------------------------------------------------------------
UUtUns16
DCrText_GetDescendingHeight(
	void)
{
	UUtUns16				descending_height;

	UUmAssert(DCgTextContext);

	descending_height =
		TSrFont_GetDescendingHeight(TSrContext_GetFont(DCgTextContext, TScStyle_InUse));

	return descending_height;
}

// ----------------------------------------------------------------------
TStFont*
DCrText_GetFont(
	void)
{
	return TSrContext_GetFont(DCgTextContext, TScStyle_InUse);
}

// ----------------------------------------------------------------------
UUtUns16
DCrText_GetLeadingHeight(
	void)
{
	UUtUns16				leading_height;

	UUmAssert(DCgTextContext);

	leading_height = TSrFont_GetLeadingHeight(TSrContext_GetFont(DCgTextContext, TScStyle_InUse));

	return leading_height;
}

// ----------------------------------------------------------------------
UUtUns16
DCrText_GetLineHeight(
	void)
{
	UUtUns16				line_height;

	UUmAssert(DCgTextContext);

	line_height = TSrFont_GetLineHeight(TSrContext_GetFont(DCgTextContext, TScStyle_InUse));

	return line_height;
}

// ----------------------------------------------------------------------
void
DCrText_GetStringRect(
	char					*inString,
	UUtRect					*outRect)
{
	UUmAssert(inString);
	UUmAssert(outRect);

	TSrContext_GetStringRect(DCgTextContext, inString, outRect);
}

// ----------------------------------------------------------------------
UUtError
DCrText_Initialize(
	void)
{
	UUtError				error;
	TStFontFamily			*font_family;

	DCgTextContext = NULL;

	// get the font family
	error =
		TMrInstance_GetDataPtr(
			TScTemplate_FontFamily,
			TScFontFamily_Default,
			&font_family);
	UUmError_ReturnOnErrorMsg(error, "Unable to load font family");

	// create a new text context
	error =
		TSrContext_New(
			font_family,
			TScFontSize_Default,
			TScStyle_Plain,
			TSc_HLeft,
			UUcFalse,
			&DCgTextContext);
	UUmError_ReturnOnErrorMsg(error, "Unable to create text context");

	// set the default color
	error = TSrContext_SetShade(DCgTextContext, IMcShade_Black);
	UUmError_ReturnOnErrorMsg(error, "Unable to set text context color");

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
DCrText_SetFontFamily(
	TStFontFamily			*inFontFamily)
{
	UUmAssert(DCgTextContext);
	UUmAssert(inFontFamily);

	TSrContext_SetFontFamily(DCgTextContext, inFontFamily);
}

// ----------------------------------------------------------------------
void
DCrText_SetFontInfo(
	TStFontInfo				*inFontInfo)
{
	UUmAssert(DCgTextContext);
	UUmAssert(inFontInfo);
	UUmAssert(inFontInfo->font_family);

	TSrContext_SetFontFamily(DCgTextContext, inFontInfo->font_family);
	TSrContext_SetFontSize(DCgTextContext, inFontInfo->font_size);
	TSrContext_SetFontStyle(DCgTextContext, inFontInfo->font_style);
	TSrContext_SetShade(DCgTextContext, inFontInfo->font_shade);
}

// ----------------------------------------------------------------------
void
DCrText_SetFormat(
	TStFormat				inFormat)
{
	UUmAssert(DCgTextContext);

	TSrContext_SetFormat(DCgTextContext, inFormat);
}

// ----------------------------------------------------------------------
void
DCrText_SetShade(
	UUtUns32				inShade)
{
	UUmAssert(DCgTextContext);

	TSrContext_SetShade(DCgTextContext, inShade);
}

// ----------------------------------------------------------------------
void
DCrText_SetSize(
	UUtUns16				inSize)
{
	UUmAssert(DCgTextContext);

	TSrContext_SetFontSize(DCgTextContext, inSize);
}

// ----------------------------------------------------------------------
void
DCrText_SetStyle(
	TStFontStyle			inStyle)
{
	UUmAssert(DCgTextContext);

	TSrContext_SetFontStyle(DCgTextContext, inStyle);
}

// ----------------------------------------------------------------------
void
DCrText_Terminate(
	void)
{
	if (DCgTextContext)
	{
		TSrContext_Delete(DCgTextContext);
		DCgTextContext = NULL;
	}
}
