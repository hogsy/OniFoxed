/*
	FILE:	MG_DC_Method_Triangle.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 18, 1997

	PURPOSE:

	Copyright 1997

*/
#ifndef MG_DC_METHOD_TRIANGLE_H
#define MG_DC_METHOD_TRIANGLE_H

// Triangle functions
	void
	MGrTriangle_Point(
		M3tTri*		inTri);

	void
	MGrTriangle_Line_InterpNone(
		M3tTri*		inTri);

	void
	MGrTriangle_Line_InterpVertex(
		M3tTri*		inTri);

	void
	MGrTriangle_Solid_Gouraud_InterpNone(
		M3tTri*		inTri);

	void
	MGrTriangle_Solid_Gouraud_InterpVertex(
		M3tTri*		inTri);

	void
	MGrTriangle_Solid_TextureLit_InterpNone(
		M3tTri*		inTri);

	void
	MGrTriangle_Solid_TextureLit_InterpVertex(
		M3tTri*		inTri);

	void
	MGrTriangle_Solid_TextureLit_EnvMap_InterpVertex(
		M3tTri*		inTri);

	void
	MGrTriangle_Solid_TextureUnlit(
		M3tTri*		inTri);

// Trisplit functions

	// texture lit

		// interp none

		void
		MGrTriSplit_Solid_TextureLit_InterpNone_LMOff(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureLit_InterpNone_LMOn_1TMU(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureLit_InterpNone_LMOn_2TMU(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureLit_InterpNone_LMOnly_1TMU(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureLit_InterpNone_LMOnly_2TMU(
			M3tTriSplit*		inTri);


		// interp vertex

		void
		MGrTriSplit_Solid_TextureLit_InterpVertex_LMOff(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureLit_InterpVertex_LMOn_1TMU(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureLit_InterpVertex_LMOn_2TMU(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureLit_InterpVertex_LMOnly_1TMU(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureLit_InterpVertex_LMOnly_2TMU(
			M3tTriSplit*		inTri);

	// texture unlit

		void
		MGrTriSplit_Solid_TextureUnlit_LMOff(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureUnlit_LMOn_1TMU(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureUnlit_LMOn_2TMU(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureUnlit_LMOnly_1TMU(
			M3tTriSplit*		inTri);

		void
		MGrTriSplit_Solid_TextureUnlit_LMOnly_2TMU(
			M3tTriSplit*		inTri);



#endif /* MG_DC_METHOD_TRIANGLE_H */
