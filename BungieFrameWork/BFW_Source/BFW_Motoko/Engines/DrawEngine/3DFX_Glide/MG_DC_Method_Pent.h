/*
	FILE:	MG_DC_Method_Pent.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 18, 1997
	
	PURPOSE: 
	
	Copyright 1997

*/
#ifndef MG_DC_METHOD_PENT_H
#define MG_DC_METHOD_PENT_H

// Pent functions
	void 
	MGrPent_Point(
		M3tPent*		inPent);

	void
	MGrPent_Line_InterpNone(
		M3tPent*		inPent);

	void
	MGrPent_Line_InterpVertex(
		M3tPent*		inPent);

	void
	MGrPent_Solid_Gouraud_InterpNone(
		M3tPent*		inPent);

	void
	MGrPent_Solid_Gouraud_InterpVertex(
		M3tPent*		inPent);

	void
	MGrPent_Solid_TextureLit_InterpNone(
		M3tPent*		inPent);

	void
	MGrPent_Solid_TextureLit_InterpVertex(
		M3tPent*		inPent);

	void
	MGrPent_Solid_TextureLit_EnvMap_InterpVertex(
		M3tPent*		inPent);

	void
	MGrPent_Solid_TextureUnlit(
		M3tPent*		inPent);

// Pentsplit functions
	
	// texture lit
	
		// interp none
		
		void
		MGrPentSplit_Solid_TextureLit_InterpNone_LMOff(
			M3tPentSplit*		inPent);
			
		
		// interp vertex
		
		void
		MGrPentSplit_Solid_TextureLit_InterpVertex_LMOff(
			M3tPentSplit*		inPent);
	

	// texture unlit
	
		void
		MGrPentSplit_Solid_TextureUnlit_LMOff(
			M3tPentSplit*		inPent);


#endif /* MG_DC_METHOD_PENT_H */
