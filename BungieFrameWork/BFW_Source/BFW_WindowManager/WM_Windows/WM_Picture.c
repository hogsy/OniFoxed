// ======================================================================
// WM_Picture.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_WindowManager.h"

#include "WM_Picture.h"

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiPicture_Create(
	WMtPicture				*inPicture)
{
	UUtError				error;
	UUtUns32				style;
	void					*texture_ref;

	texture_ref = NULL;

	style = WMrWindow_GetStyle(inPicture);
	if ((style & WMcPictureStyle_SetAtRuntime) == 0)
	{
		// the pictures title is the instance name of the texture
		error =
			TMrInstance_GetDataPtr(
				M3cTemplate_TextureMap,
				WMrWindow_GetTitlePtr(inPicture),
				&texture_ref);
		if (error != UUcError_None)
		{
			error =
				TMrInstance_GetDataPtr(
					M3cTemplate_TextureMap_Big,
					WMrWindow_GetTitlePtr(inPicture),
					&texture_ref);
		}

		WMrWindow_SetLong(inPicture, 0, (UUtUns32)texture_ref);
	}
	else
	{
		WMrWindow_SetLong(inPicture, 0, (UUtUns32)texture_ref);
	}

	return WMcResult_Handled;
}

// ----------------------------------------------------------------------
static void
WMiPicture_HandleSetPartSpec(
	WMtPicture				*inPicture,
	UUtUns32				inParam1)
{
	TMtTemplateTag			template_tag;

	// get the template tag
	template_tag = TMrInstance_GetTemplateTag((void*)inParam1);
	if (template_tag != PScTemplate_PartSpecification)
	{
		return;
	}

	WMrWindow_SetLong(inPicture, 0, inParam1);
}

// ----------------------------------------------------------------------
static void
WMiPicture_HandleSetPicture(
	WMtPicture				*inPicture,
	UUtUns32				inParam1)
{
	TMtTemplateTag			template_tag;

	// get the template tag
	template_tag = TMrInstance_GetTemplateTag((void*)inParam1);
	if ((template_tag != M3cTemplate_TextureMap) &&
		(template_tag != M3cTemplate_TextureMap_Big))
	{
		return;
	}

	WMrWindow_SetLong(inPicture, 0, inParam1);
}

// ----------------------------------------------------------------------
static void
WMiPicture_Paint(
	WMtPicture				*inPicture)
{
	DCtDrawContext			*draw_context;
	IMtPoint2D				dest;
	UUtInt16				width;
	UUtInt16				height;
	void					*template_ref;
	TMtTemplateTag			template_tag;
	UUtUns32				style;

	// get the template ref stored in the long
	template_ref = (void*)WMrWindow_GetLong(inPicture, 0);
	if (template_ref == NULL) { return; }

	draw_context = DCrDraw_Begin(inPicture);

	// get the width and height of the window
	WMrWindow_GetSize(inPicture, &width, &height);

	dest.x = 0;
	dest.y = 0;

	template_tag = TMrInstance_GetTemplateTag(template_ref);
	switch (template_tag)
	{
		case M3cTemplate_TextureMap:
		case M3cTemplate_TextureMap_Big:
			// get the picture style
			style = WMrWindow_GetStyle(inPicture);

			if ((style & WMcPictureStyle_Scale) == 0)
			{
				UUtUns16			texture_width;
				UUtUns16			texture_height;

				M3rTextureRef_GetSize(template_ref, &texture_width, &texture_height);

				width = UUmMin(width, texture_width);
				height = UUmMin(height, texture_height);
			}

			// draw the texture ref
			DCrDraw_TextureRef(
				draw_context,
				template_ref,
				&dest,
				width,
				height,
				UUcFalse,
				M3cMaxAlpha);
		break;

		case PScTemplate_PartSpecification:
			// draw the part spec
			DCrDraw_PartSpec(
				draw_context,
				template_ref,
				PScPart_All,
				&dest,
				width,
				height,
				M3cMaxAlpha);
		break;
	}

	DCrDraw_End(draw_context, inPicture);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtUns32
WMiPicture_Callback(
	WMtPicture				*inPicture,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtUns32				result;

	switch(inMessage)
	{
		case WMcMessage_Create:
			result = WMiPicture_Create(inPicture);
		return result;

		case WMcMessage_Paint:
			WMiPicture_Paint(inPicture);
		return WMcResult_Handled;

		case PTcMessage_SetPartSpec:
			WMiPicture_HandleSetPartSpec(inPicture, inParam1);
		return WMcResult_Handled;

		case PTcMessage_SetPicture:
			WMiPicture_HandleSetPicture(inPicture, inParam1);
		return WMcResult_Handled;
	}

	return WMrWindow_DefaultCallback(inPicture, inMessage, inParam1, inParam2);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
WMrPicture_Initialize(
	void)
{
	UUtError				error;
	WMtWindowClass			window_class;

	// register the window class
	UUrMemory_Clear(&window_class, sizeof(WMtWindowClass));
	window_class.type = WMcWindowType_Picture;
	window_class.callback = WMiPicture_Callback;
	window_class.private_data_size = sizeof(UUtUns32);

	error = WMrWindowClass_Register(&window_class);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
WMrPicture_SetPartSpec(
	WMtPicture				*inPicture,
	void					*inPartSpec)
{
	UUmAssert(inPicture);
	UUmAssert(inPartSpec);

	WMrMessage_Send(inPicture, PTcMessage_SetPartSpec, (UUtUns32)inPartSpec, 0);
}

// ----------------------------------------------------------------------
void
WMrPicture_SetPicture(
	WMtPicture				*inPicture,
	void					*inTexture)
{
	UUmAssert(inPicture);
	UUmAssert(inTexture);

	WMrMessage_Send(inPicture, PTcMessage_SetPicture, (UUtUns32)inTexture, 0);
}
