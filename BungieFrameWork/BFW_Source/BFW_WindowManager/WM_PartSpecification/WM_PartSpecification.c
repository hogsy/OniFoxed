// ======================================================================
// WM_PartSpecification.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "WM_PartSpecification.h"
#include "WM_DrawContext.h"

// ======================================================================
// globals
// ======================================================================
TMtPrivateData*				PSgTemplate_PartSpec_PrivateData;

static PStPartSpecUI		*PSgPartSpecUI_Active;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
PSiPartSpec_CalcUV(
	PStPartSpec				*inPartSpec,
	M3tTextureCoord			*ioUVs,
	UUtUns16				inColumn,
	UUtUns16				inRow)
{
	M3tTextureMap			*texture;
	float					invTextureWidth;
	float					invTextureHeight;
	float					lu;
	float					ru;
	float					tv;
	float					bv;

	texture = (M3tTextureMap*)inPartSpec->texture;

	invTextureWidth = 1.0f / (float)texture->width;
	invTextureHeight = 1.0f / (float)texture->height;

	lu = (float)inPartSpec->part_matrix_tl[inColumn][inRow].x * invTextureWidth;
	tv = (float)inPartSpec->part_matrix_tl[inColumn][inRow].y * invTextureHeight;

	bv = (float)inPartSpec->part_matrix_br[inColumn][inRow].y * invTextureHeight;
	ru = (float)inPartSpec->part_matrix_br[inColumn][inRow].x * invTextureWidth;

	// tl
	ioUVs[0].u = lu;
	ioUVs[0].v = tv;

	// tr
	ioUVs[1].u = ru;
	ioUVs[1].v = tv;

	// bl
	ioUVs[2].u = lu;
	ioUVs[2].v = bv;

	// br
	ioUVs[3].u = ru;
	ioUVs[3].v = bv;
}

// ----------------------------------------------------------------------
static UUtError
PSiPartSpec_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void					*inInstancePtr,
	void*					inPrivateData)
{
	PStPartSpec				*part_spec;
	PStPartSpec_PrivateData	*private_data;

	UUmAssert(inInstancePtr);

	// get a pointer to the part spec
	part_spec = (PStPartSpec*)inInstancePtr;

	// get a pointer to the view's private data
	private_data = (PStPartSpec_PrivateData*)inPrivateData;
	UUmAssert(private_data);

	switch(inMessage)
	{
		case TMcTemplateProcMessage_NewPostProcess:
		case TMcTemplateProcMessage_LoadPostProcess:
			// precalculate all of the UVs
			PSiPartSpec_CalcUV(part_spec, private_data->lt, 0, 0);
			PSiPartSpec_CalcUV(part_spec, private_data->lm, 0, 1);
			PSiPartSpec_CalcUV(part_spec, private_data->lb, 0, 2);

			PSiPartSpec_CalcUV(part_spec, private_data->mt, 1, 0);
			PSiPartSpec_CalcUV(part_spec, private_data->mm, 1, 1);
			PSiPartSpec_CalcUV(part_spec, private_data->mb, 1, 2);

			PSiPartSpec_CalcUV(part_spec, private_data->rt, 2, 0);
			PSiPartSpec_CalcUV(part_spec, private_data->rm, 2, 1);
			PSiPartSpec_CalcUV(part_spec, private_data->rb, 2, 2);

			private_data->l_width =
				UUmABS(
					(UUtInt16)(part_spec->part_matrix_br[0][0].x -
					part_spec->part_matrix_tl[0][0].x));

			private_data->r_width =
				UUmABS(
					(UUtInt16)(part_spec->part_matrix_br[2][0].x -
					part_spec->part_matrix_tl[2][0].x));

			private_data->t_height =
				UUmABS(
					(UUtInt16)(part_spec->part_matrix_br[0][0].y -
					part_spec->part_matrix_tl[0][0].y));

			private_data->b_height =
				UUmABS(
					(UUtInt16)(part_spec->part_matrix_br[0][2].y -
					part_spec->part_matrix_tl[0][2].y));

		break;

		case TMcTemplateProcMessage_DisposePreProcess:
		break;

		case TMcTemplateProcMessage_Update:
		break;

		default:
			UUmAssert(!"Illegal message");
		break;
	}

	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
PSrPartSpec_Draw(
	PStPartSpec				*inPartSpec,
	UUtUns16				inFlags,
	M3tPointScreen			*inLocation,
	UUtInt16				inWidth,
	UUtInt16				inHeight,
	UUtUns16				inAlpha)
{
	UUtUns32				shade;
	M3tTextureMap			*texture;
	PStPartSpec_PrivateData	*private_data;
	M3tPointScreen			dest;

	UUtInt16				l_width;
	UUtInt16				m_width;
	UUtInt16				r_width;

	UUtInt16				t_height;
	UUtInt16				m_height;
	UUtInt16				b_height;


	// get the private data
	private_data = (PStPartSpec_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(PSgTemplate_PartSpec_PrivateData, inPartSpec);
	UUmAssert(private_data);

	texture = (M3tTextureMap*)inPartSpec->texture;

	// set the shade and alpha
	shade = IMcShade_White;

	// calculate the heights and widths
	if ((private_data->t_height + private_data->b_height) > inHeight)
	{
		t_height = private_data->t_height * private_data->t_height / inHeight;
		b_height = private_data->b_height * private_data->b_height / inHeight;
	}
	else
	{
		t_height = private_data->t_height;
		b_height = private_data->b_height;
	}
	m_height = (UUtUns16)UUmMax(0, inHeight - (t_height + b_height));

	if ((private_data->l_width + private_data->r_width) > inWidth)
	{
		l_width = private_data->l_width * private_data->l_width / inWidth;
		r_width = private_data->r_width * private_data->r_width / inWidth;
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

	// draw the left column
	dest = *inLocation;

	if (inFlags & PScPart_LeftTop)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->lt,
			&dest,
			private_data->l_width,
			private_data->t_height,
			shade,
			inAlpha);
		dest.y += (float)private_data->t_height;
	}

	if (inFlags & PScPart_LeftMiddle)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->lm,
			&dest,
			private_data->l_width,
			m_height,
			shade,
			inAlpha);
		dest.y += (float)m_height;
	}

	if (inFlags & PScPart_LeftBottom)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->lb,
			&dest,
			private_data->l_width,
			private_data->b_height,
			shade,
			inAlpha);
	}

	// draw the middle column
	dest = *inLocation;
	if (inFlags & PScPart_LeftColumn)
	{
		dest.x += private_data->l_width;
	}

	if (inFlags & PScPart_MiddleTop)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->mt,
			&dest,
			m_width,
			private_data->t_height,
			shade,
			inAlpha);
		dest.y += (float)private_data->t_height;
	}

	if (inFlags & PScPart_MiddleMiddle)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->mm,
			&dest,
			m_width,
			m_height,
			shade,
			inAlpha);
		dest.y += (float)m_height;
	}

	if (inFlags & PScPart_MiddleBottom)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->mb,
			&dest,
			m_width,
			private_data->b_height,
			shade,
			inAlpha);
	}

	// draw the right column
	dest = *inLocation;
	if (inFlags & PScPart_MiddleColumn)
	{
		dest.x += private_data->l_width + m_width;
	}

	if (inFlags & PScPart_RightTop)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->rt,
			&dest,
			private_data->r_width,
			private_data->t_height,
			shade,
			inAlpha);
		dest.y += (float)private_data->t_height;
	}

	if (inFlags & PScPart_RightMiddle)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->rm,
			&dest,
			private_data->r_width,
			m_height,
			shade,
			inAlpha);
		dest.y += (float)m_height;
	}

	if (inFlags & PScPart_RightBottom)
	{
		M3rDraw_BitmapUV(
			inPartSpec->texture,
			private_data->rb,
			&dest,
			private_data->r_width,
			private_data->b_height,
			shade,
			inAlpha);
	}
}

// ----------------------------------------------------------------------
void
PSrPartSpec_GetSize(
	PStPartSpec				*inPartSpec,
	UUtUns16				inPart,
	UUtInt16				*outWidth,
	UUtInt16				*outHeight)
{
	PStPartSpec_PrivateData	*private_data;
	UUtInt16				width;
	UUtInt16				height;

	UUmAssert(inPartSpec);

	// get the private data
	private_data = (PStPartSpec_PrivateData*)TMrTemplate_PrivateData_GetDataPtr(PSgTemplate_PartSpec_PrivateData, inPartSpec);
	UUmAssert(private_data);

	width = 0;
	height = 0;

	switch (inPart)
	{
		case PScPart_LeftTop:
			width = private_data->l_width;
			height = private_data->t_height;
		break;

		case PScPart_LeftBottom:
			width = private_data->l_width;
			height = private_data->b_height;
		break;

		case PScPart_RightTop:
			width = private_data->r_width;
			height = private_data->t_height;
		break;

		case PScPart_RightBottom:
			width = private_data->r_width;
			height = private_data->b_height;
		break;
	}

	if (outWidth)
	{
		*outWidth = width;
	}

	if (outHeight)
	{
		*outHeight = height;
	}
}

// ----------------------------------------------------------------------
PStPartSpec*
PSrPartSpec_LoadByType(
	PStPartSpecType			inPartSpecType)
{
	UUtError				error;
	PStPartSpecList			*partspec_list;
	UUtUns32				i;

	UUrMemory_Block_VerifyList();

	// get the partspec list
	error =
		TMrInstance_GetDataPtr(
			PScTemplate_PartSpecList,
			"partspec_list",
			&partspec_list);
	if (error != UUcError_None)
	{
		return NULL;
	}

	UUrMemory_Block_VerifyList();

	// find the desired partspec in the partspec list
	for (i = 0; i < partspec_list->num_partspec_descs; i++)
	{
		if (partspec_list->partspec_descs[i].partspec_type == inPartSpecType)
		{
			return partspec_list->partspec_descs[i].partspec;
		}
	}

	return NULL;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
PStPartSpecUI*
PSrPartSpecUI_GetActive(
	void)
{
	if (PSgPartSpecUI_Active == NULL)
	{
		PSgPartSpecUI_Active = PSrPartSpecUI_GetByName("psui_g206");
	}
	return PSgPartSpecUI_Active;
}

// ----------------------------------------------------------------------
PStPartSpecUI*
PSrPartSpecUI_GetByName(
	const char				*inName)
{
	return TMrInstance_GetFromName(PScTemplate_PartSpecUI, inName);
}

// ----------------------------------------------------------------------
void
PSrPartSpecUI_SetActive(
	PStPartSpecUI			*inPartSpecUI)
{
	PSgPartSpecUI_Active = inPartSpecUI;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
PSrInitialize(
	void)
{
	UUtError				error;

	PSgPartSpecUI_Active = NULL;

	error = PSrRegisterTemplates();
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_PrivateData_New(
			PScTemplate_PartSpecification,
			sizeof(PStPartSpec_PrivateData),
			PSiPartSpec_ProcHandler,
			&PSgTemplate_PartSpec_PrivateData);
	UUmError_ReturnOnErrorMsg(error, "Could not install the proc handler");

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
PSrRegisterTemplates(
	void)
{
	UUtError				error;

	error =
		TMrTemplate_Register(
			PScTemplate_PartSpecification,
			sizeof(PStPartSpec),
			TMcFolding_Forbid);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			PScTemplate_PartSpecList,
			sizeof(PStPartSpecList),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	error =
		TMrTemplate_Register(
			PScTemplate_PartSpecUI,
			sizeof(PStPartSpecUI),
			TMcFolding_Allow);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}
