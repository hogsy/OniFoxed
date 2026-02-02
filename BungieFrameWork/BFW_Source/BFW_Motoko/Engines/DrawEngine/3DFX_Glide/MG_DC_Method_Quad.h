/*
	FILE:	MG_DC_Method_Quad.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 18, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef MG_DC_METHOD_QUAD_H
#define MG_DC_METHOD_QUAD_H

// Quad functions
	void
	MGrQuad_Point(
		M3tQuad*		inQuad);

	void
	MGrQuad_Line_InterpNone(
		M3tQuad*		inQuad);

	void
	MGrQuad_Line_InterpVertex(
		M3tQuad*		inQuad);

	void
	MGrQuad_Solid_Gouraud_InterpNone(
		M3tQuad*		inQuad);

	void
	MGrQuad_Solid_Gouraud_InterpVertex(
		M3tQuad*		inQuad);

	void
	MGrQuad_Solid_TextureLit_InterpNone(
		M3tQuad*		inQuad);

	void
	MGrQuad_Solid_TextureLit_InterpVertex(
		M3tQuad*		inQuad);

	void
	MGrQuad_Solid_TextureLit_EnvMap_InterpVertex(
		M3tQuad*		inQuad);

	void
	MGrQuad_Solid_TextureUnlit(
		M3tQuad*		inQuad);

// Quadsplit functions

	// texture lit

		// interp none

		void
		MGrQuadSplit_Solid_TextureLit_InterpNone_LMOff(
			M3tQuadSplit*		inQuad);

		// interp vertex

		void
		MGrQuadSplit_Solid_TextureLit_InterpVertex_LMOff(
			M3tQuadSplit*		inQuad);

	// texture unlit

		void
		MGrQuadSplit_Solid_TextureUnlit_LMOff(
			M3tQuadSplit*		inQuad);


#endif /* MG_DC_METHOD_QUAD_H */
