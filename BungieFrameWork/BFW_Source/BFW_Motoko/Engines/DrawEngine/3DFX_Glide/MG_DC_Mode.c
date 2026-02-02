/*
	FILE:	MG_DC_Mode.c

	AUTHOR:	Brent Pease

	CREATED: July 31, 1999

	PURPOSE:

	Copyright 1997 - 1999

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "Motoko_Manager.h"

#include "MG_DC_Private.h"
#include "MG_DC_Mode.h"

#include "rasterizer_3dfx.h"

void
MGrMode_Point(
	void)
{
	MGrSet_AlphaCombine(MGcAlphaCombine_NoAlpha);
	MGrSet_ColorCombine(MGcColorCombine_ConstantColor);

}

void
MGrMode_Line_Gouraud_InterpNone(
	void)
{
	MGrSet_AlphaCombine(MGcAlphaCombine_NoAlpha);
	MGrSet_ColorCombine(MGcColorCombine_ConstantColor);

}

void
MGrMode_Line_Gouraud_InterpVertex(
	void)
{
	MGrSet_AlphaCombine(MGcAlphaCombine_NoAlpha);
	MGrSet_ColorCombine(MGcColorCombine_Gouraud);

}

void
MGrMode_Solid_Gouraud_InterpNone(
	void)
{
	MGrSet_AlphaCombine(MGcAlphaCombine_ConstantAlpha);
	MGrSet_ColorCombine(MGcColorCombine_ConstantColor);

}

void
MGrMode_Solid_Gouraud_InterpVertex(
	void)
{
	MGrSet_AlphaCombine(MGcAlphaCombine_ConstantAlpha);
	MGrSet_ColorCombine(MGcColorCombine_Gouraud);

}

void
MGrMode_Solid_TextureLit_InterpNone_LMOff_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureLit_InterpNone_LMOn_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureLit_InterpNone_LMOnly_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureLit_InterpVertex_LMOff_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureGouraud);

}

void
MGrMode_Solid_TextureLit_InterpVertex_LMOn_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureGouraud);

}

void
MGrMode_Solid_TextureLit_InterpVertex_LMOnly_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureGouraud);

}

void
MGrMode_Solid_TextureUnlit_InterpNone_LMOff_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureUnlit_InterpNone_LMOn_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureUnlit_InterpNone_LMOnly_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureUnlit_InterpVertex_LMOff_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureUnlit_InterpVertex_LMOn_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureUnlit_InterpVertex_LMOnly_1TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

//
void
MGrMode_Solid_TextureLit_InterpNone_LMOff_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureLit_InterpNone_LMOn_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureLit_InterpNone_LMOnly_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureLit_InterpVertex_LMOff_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureGouraud);

}

void
MGrMode_Solid_TextureLit_InterpVertex_LMOn_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureGouraud);

}

void
MGrMode_Solid_TextureLit_InterpVertex_LMOnly_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureGouraud);

}

void
MGrMode_Solid_TextureUnlit_InterpNone_LMOff_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureUnlit_InterpNone_LMOn_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureUnlit_InterpNone_LMOnly_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureUnlit_InterpVertex_LMOff_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureUnlit_InterpVertex_LMOn_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

void
MGrMode_Solid_TextureUnlit_InterpVertex_LMOnly_2TMU(
	void)
{
	MGrSet_ColorCombine(MGcColorCombine_TextureFlat);

}

