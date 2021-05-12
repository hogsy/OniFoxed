/*
	FILE:	MG_DrawEngine_Method_Ptrs.c
	
	AUTHOR:	Brent Pease
	
	CREATED: July 31, 1999
	
	PURPOSE: 
	
	Copyright 1997 - 1999

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "Motoko_Manager.h"

#include "MG_DC_Private.h"
#include "MG_DC_Method_Triangle.h"
#include "MG_DC_Method_Quad.h"
#include "MG_DC_Method_Pent.h"
#include "MG_DC_Method_LinePoint.h"
#include "MG_DC_Mode.h"
#include "MG_DC_CreateVertex.h"

M3tDrawContextMethod_Line		MGgLineFuncs[M3cDrawState_Interpolation_Num] =
{
	// M3cDrawState_Interpolation_None
		MGrLine_InterpNone,
		
	// M3cDrawState_Interpolation_Vertex
		MGrLine_InterpVertex
};

MGtDrawTri			MGgTriangleFuncs_VertexUnified
									[M3cDrawState_Fill_Num]
									[M3cDrawState_Appearence_Num]
									[M3cDrawState_Interpolation_Num] =
{
	// M3cDrawState_Fill_Point
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Point
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Point
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Point
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Point
		}
	},
	
	// M3cDrawState_Fill_Line
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Line_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Line_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Line_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Line_InterpNone
		}
		
	},
	
	// M3cDrawState_Fill_Solid
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Solid_Gouraud_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Solid_Gouraud_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Solid_TextureLit_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Solid_TextureLit_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Solid_TextureLit_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Solid_TextureLit_EnvMap_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			MGrTriangle_Solid_TextureUnlit,
			
			// M3cDrawState_Interpolation_Vertex
			MGrTriangle_Solid_TextureUnlit
		}
	}
};

MGtDrawTriSplit	MGgTriangleFuncs_VertexSplit
									[M3cDrawState_Fill_Num]
									[M3cDrawState_Appearence_Num]
									[M3cDrawState_Interpolation_Num]
									[MGcDrawState_TMU_Num] =
{
	// M3cDrawState_Fill_Point
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Point,	(MGtDrawTriSplit)MGrTriangle_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Point,	(MGtDrawTriSplit)MGrTriangle_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Point,	(MGtDrawTriSplit)MGrTriangle_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Point,	(MGtDrawTriSplit)MGrTriangle_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Point,	(MGtDrawTriSplit)MGrTriangle_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Point,	(MGtDrawTriSplit)MGrTriangle_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Point,	(MGtDrawTriSplit)MGrTriangle_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Point,	(MGtDrawTriSplit)MGrTriangle_Point
			}
		}
		
	},
	
	// M3cDrawState_Fill_Line
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Line_InterpNone,	(MGtDrawTriSplit)MGrTriangle_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Line_InterpVertex,	(MGtDrawTriSplit)MGrTriangle_Line_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Line_InterpNone,	(MGtDrawTriSplit)MGrTriangle_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Line_InterpVertex,	(MGtDrawTriSplit)MGrTriangle_Line_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Line_InterpNone,	(MGtDrawTriSplit)MGrTriangle_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Line_InterpVertex,	(MGtDrawTriSplit)MGrTriangle_Line_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Line_InterpNone,	(MGtDrawTriSplit)MGrTriangle_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawTriSplit)MGrTriangle_Line_InterpVertex,	(MGtDrawTriSplit)MGrTriangle_Line_InterpVertex
			}
		}
	},
	
	// M3cDrawState_Fill_Solid
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU								2 TMU
				(MGtDrawTriSplit)MGrTriangle_Solid_Gouraud_InterpNone,	(MGtDrawTriSplit)MGrTriangle_Solid_Gouraud_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU								2 TMU
				(MGtDrawTriSplit)MGrTriangle_Solid_Gouraud_InterpVertex,	(MGtDrawTriSplit)MGrTriangle_Solid_Gouraud_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrTriSplit_Solid_TextureLit_InterpNone_LMOff,	MGrTriSplit_Solid_TextureLit_InterpNone_LMOff
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrTriSplit_Solid_TextureLit_InterpVertex_LMOff,	MGrTriSplit_Solid_TextureLit_InterpVertex_LMOff
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrTriSplit_Solid_TextureLit_InterpNone_LMOff,	MGrTriSplit_Solid_TextureLit_InterpNone_LMOff
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrTriSplit_Solid_TextureLit_InterpVertex_LMOff,	MGrTriSplit_Solid_TextureLit_InterpVertex_LMOff
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrTriSplit_Solid_TextureUnlit_LMOff,	MGrTriSplit_Solid_TextureUnlit_LMOff
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrTriSplit_Solid_TextureUnlit_LMOff,	MGrTriSplit_Solid_TextureUnlit_LMOff
			}
		}
	}
};

MGtDrawQuad	MGgQuadFuncs_VertexUnified
									[M3cDrawState_Fill_Num]
									[M3cDrawState_Appearence_Num]
									[M3cDrawState_Interpolation_Num] =
{
	// M3cDrawState_Fill_Point
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Point
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Point
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Point
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Point
		}
		
	},
	
	// M3cDrawState_Fill_Line
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Line_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Line_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Line_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Line_InterpNone
		}
		
	},
	
	// M3cDrawState_Fill_Solid
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Solid_Gouraud_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Solid_Gouraud_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Solid_TextureLit_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Solid_TextureLit_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Solid_TextureLit_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Solid_TextureLit_EnvMap_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			MGrQuad_Solid_TextureUnlit,
			
			// M3cDrawState_Interpolation_Vertex
			MGrQuad_Solid_TextureUnlit
		}
	}
};


MGtDrawQuadSplit	MGgQuadFuncs_VertexSplit
									[M3cDrawState_Fill_Num]
									[M3cDrawState_Appearence_Num]
									[M3cDrawState_Interpolation_Num]
									[MGcDrawState_TMU_Num] =
{
	// M3cDrawState_Fill_Point
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Point,	(MGtDrawQuadSplit)MGrQuad_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Point,	(MGtDrawQuadSplit)MGrQuad_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Point,	(MGtDrawQuadSplit)MGrQuad_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Point,	(MGtDrawQuadSplit)MGrQuad_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Point,	(MGtDrawQuadSplit)MGrQuad_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Point,	(MGtDrawQuadSplit)MGrQuad_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Point,	(MGtDrawQuadSplit)MGrQuad_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Point,	(MGtDrawQuadSplit)MGrQuad_Point
			}
		}
		
	},
	
	// M3cDrawState_Fill_Line
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Line_InterpNone,	(MGtDrawQuadSplit)MGrQuad_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Line_InterpVertex,	(MGtDrawQuadSplit)MGrQuad_Line_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Line_InterpNone,	(MGtDrawQuadSplit)MGrQuad_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Line_InterpVertex,	(MGtDrawQuadSplit)MGrQuad_Line_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Line_InterpNone,	(MGtDrawQuadSplit)MGrQuad_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Line_InterpVertex,	(MGtDrawQuadSplit)MGrQuad_Line_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Line_InterpNone,	(MGtDrawQuadSplit)MGrQuad_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawQuadSplit)MGrQuad_Line_InterpVertex,	(MGtDrawQuadSplit)MGrQuad_Line_InterpVertex
			}
		}
	},
	
	// M3cDrawState_Fill_Solid
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU								2 TMU
				(MGtDrawQuadSplit)MGrQuad_Solid_Gouraud_InterpNone,	(MGtDrawQuadSplit)MGrQuad_Solid_Gouraud_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU								2 TMU
				(MGtDrawQuadSplit)MGrQuad_Solid_Gouraud_InterpVertex,	(MGtDrawQuadSplit)MGrQuad_Solid_Gouraud_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrQuadSplit_Solid_TextureLit_InterpNone_LMOff,		MGrQuadSplit_Solid_TextureLit_InterpNone_LMOff
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrQuadSplit_Solid_TextureLit_InterpVertex_LMOff,	MGrQuadSplit_Solid_TextureLit_InterpVertex_LMOff
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrQuadSplit_Solid_TextureLit_InterpNone_LMOff,		MGrQuadSplit_Solid_TextureLit_InterpNone_LMOff
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrQuadSplit_Solid_TextureLit_InterpVertex_LMOff,	MGrQuadSplit_Solid_TextureLit_InterpVertex_LMOff
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrQuadSplit_Solid_TextureUnlit_LMOff,			MGrQuadSplit_Solid_TextureUnlit_LMOff
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrQuadSplit_Solid_TextureUnlit_LMOff,			MGrQuadSplit_Solid_TextureUnlit_LMOff
			}
		}
	}
};

MGtDrawPent	MGgPentFuncs_VertexUnified
									[M3cDrawState_Fill_Num]
									[M3cDrawState_Appearence_Num]
									[M3cDrawState_Interpolation_Num] =
{
	// M3cDrawState_Fill_Point
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Point
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Point
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Point
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Point
		}
		
	},
	
	// M3cDrawState_Fill_Line
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Line_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Line_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Line_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Line_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Line_InterpNone
		}
		
	},
	
	// M3cDrawState_Fill_Solid
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Solid_Gouraud_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Solid_Gouraud_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Solid_TextureLit_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Solid_TextureLit_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Solid_TextureLit_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Solid_TextureLit_EnvMap_InterpVertex
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			MGrPent_Solid_TextureUnlit,
			
			// M3cDrawState_Interpolation_Vertex
			MGrPent_Solid_TextureUnlit
		}
	}
};


MGtDrawPentSplit	MGgPentFuncs_VertexSplit
									[M3cDrawState_Fill_Num]
									[M3cDrawState_Appearence_Num]
									[M3cDrawState_Interpolation_Num]
									[MGcDrawState_TMU_Num] =
{
	// M3cDrawState_Fill_Point
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawPentSplit)MGrPent_Point,	(MGtDrawPentSplit)MGrPent_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawPentSplit)MGrPent_Point,	(MGtDrawPentSplit)MGrPent_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			// M3cDrawState_LightMapMode_Off
			{
					(MGtDrawPentSplit)MGrPent_Point,	(MGtDrawPentSplit)MGrPent_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawPentSplit)MGrPent_Point,	(MGtDrawPentSplit)MGrPent_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawPentSplit)MGrPent_Point,	(MGtDrawPentSplit)MGrPent_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawPentSplit)MGrPent_Point,	(MGtDrawPentSplit)MGrPent_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawPentSplit)MGrPent_Point,	(MGtDrawPentSplit)MGrPent_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				(MGtDrawPentSplit)MGrPent_Point,	(MGtDrawPentSplit)MGrPent_Point
			}
		}
		
	},
	
	// M3cDrawState_Fill_Line
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawPentSplit)MGrPent_Line_InterpNone,	(MGtDrawPentSplit)MGrPent_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawPentSplit)MGrPent_Line_InterpVertex,	(MGtDrawPentSplit)MGrPent_Line_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawPentSplit)MGrPent_Line_InterpNone,	(MGtDrawPentSplit)MGrPent_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawPentSplit)MGrPent_Line_InterpVertex,	(MGtDrawPentSplit)MGrPent_Line_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawPentSplit)MGrPent_Line_InterpNone,	(MGtDrawPentSplit)MGrPent_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawPentSplit)MGrPent_Line_InterpVertex,	(MGtDrawPentSplit)MGrPent_Line_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawPentSplit)MGrPent_Line_InterpNone,	(MGtDrawPentSplit)MGrPent_Line_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				(MGtDrawPentSplit)MGrPent_Line_InterpVertex,	(MGtDrawPentSplit)MGrPent_Line_InterpVertex
			}
		}
	},
	
	// M3cDrawState_Fill_Solid
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU								2 TMU
				(MGtDrawPentSplit)MGrPent_Solid_Gouraud_InterpNone,	(MGtDrawPentSplit)MGrPent_Solid_Gouraud_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU								2 TMU
				(MGtDrawPentSplit)MGrPent_Solid_Gouraud_InterpVertex,	(MGtDrawPentSplit)MGrPent_Solid_Gouraud_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrPentSplit_Solid_TextureLit_InterpNone_LMOff,	MGrPentSplit_Solid_TextureLit_InterpNone_LMOff
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrPentSplit_Solid_TextureLit_InterpVertex_LMOff,	MGrPentSplit_Solid_TextureLit_InterpVertex_LMOff
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrPentSplit_Solid_TextureLit_InterpNone_LMOff,	MGrPentSplit_Solid_TextureLit_InterpNone_LMOff
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrPentSplit_Solid_TextureLit_InterpVertex_LMOff,	MGrPentSplit_Solid_TextureLit_InterpVertex_LMOff
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrPentSplit_Solid_TextureUnlit_LMOff,	MGrPentSplit_Solid_TextureUnlit_LMOff
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrPentSplit_Solid_TextureUnlit_LMOff,	MGrPentSplit_Solid_TextureUnlit_LMOff
				}
		}
	}
};

MGtModeFunction	MGgModeFuncs_VertexUnified
	[M3cDrawState_Fill_Num]
	[M3cDrawState_Appearence_Num]
	[M3cDrawState_Interpolation_Num] =
{
	// M3cDrawState_Fill_Point
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Point
		},
			
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Point
		},
			
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Point
		},
			
		// M3cDrawState_Appearence_TextureUnlit
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Point,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Point
		}
	},
	
	// M3cDrawState_Fill_Line
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Line_Gouraud_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Line_Gouraud_InterpVertex
		},
			
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Line_Gouraud_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Line_Gouraud_InterpVertex
		},
			
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Line_Gouraud_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Line_Gouraud_InterpVertex
		},
			
		// M3cDrawState_Appearence_TextureUnlit
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Line_Gouraud_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Line_Gouraud_InterpVertex
		}
	},
	
	
	// M3cDrawState_Fill_Solid
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Solid_Gouraud_InterpNone,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Solid_Gouraud_InterpVertex
		},
			
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Solid_TextureLit_InterpNone_LMOn_1TMU,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Solid_TextureLit_InterpVertex_LMOn_1TMU
		},
			
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Solid_TextureLit_InterpNone_LMOn_1TMU,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Solid_TextureLit_InterpVertex_LMOn_1TMU
		},
			
		// M3cDrawState_Appearence_TextureUnlit
		{
			// M3cDrawState_Interpolation_None
			MGrMode_Solid_TextureUnlit_InterpNone_LMOn_1TMU,
			
			// M3cDrawState_Interpolation_Vertex
			MGrMode_Solid_TextureUnlit_InterpNone_LMOn_1TMU
		}
	}	

};

MGtModeFunction	MGgModeFuncs_VertexSplit
	[M3cDrawState_Fill_Num]
	[M3cDrawState_Appearence_Num]
	[M3cDrawState_Interpolation_Num]
	[MGcDrawState_TMU_Num] =
{
	// M3cDrawState_Fill_Point
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				MGrMode_Point,	MGrMode_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				MGrMode_Point,	MGrMode_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				MGrMode_Point,	MGrMode_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				MGrMode_Point,	MGrMode_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				MGrMode_Point,	MGrMode_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				MGrMode_Point,	MGrMode_Point
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				MGrMode_Point,	MGrMode_Point
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU			// 2 TMU
				MGrMode_Point,	MGrMode_Point
			}
		}
		
	},
	
	// M3cDrawState_Fill_Line
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				MGrMode_Line_Gouraud_InterpNone,	MGrMode_Line_Gouraud_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				MGrMode_Line_Gouraud_InterpVertex,	MGrMode_Line_Gouraud_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				MGrMode_Line_Gouraud_InterpNone,	MGrMode_Line_Gouraud_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				MGrMode_Line_Gouraud_InterpVertex,	MGrMode_Line_Gouraud_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				MGrMode_Line_Gouraud_InterpNone,	MGrMode_Line_Gouraud_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				MGrMode_Line_Gouraud_InterpVertex,	MGrMode_Line_Gouraud_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			// M3cDrawState_LightMapMode_Off
			{
					MGrMode_Line_Gouraud_InterpNone,	MGrMode_Line_Gouraud_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU						// 2 TMU
				MGrMode_Line_Gouraud_InterpVertex,	MGrMode_Line_Gouraud_InterpVertex
			}
		}
	},
	
	// M3cDrawState_Fill_Solid
	{
		// M3cDrawState_Appearence_Gouraud
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU								2 TMU
				MGrMode_Solid_Gouraud_InterpNone,	MGrMode_Solid_Gouraud_InterpNone
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1 TMU								2 TMU
				MGrMode_Solid_Gouraud_InterpVertex,	MGrMode_Solid_Gouraud_InterpVertex
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrMode_Solid_TextureLit_InterpNone_LMOff_1TMU,	MGrMode_Solid_TextureLit_InterpNone_LMOff_2TMU
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrMode_Solid_TextureLit_InterpVertex_LMOff_1TMU,	MGrMode_Solid_TextureLit_InterpVertex_LMOff_2TMU
			}
		},
		
		// M3cDrawState_Appearence_Texture_Lit_EnvMap
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrMode_Solid_TextureLit_InterpNone_LMOff_1TMU,	MGrMode_Solid_TextureLit_InterpNone_LMOff_2TMU
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrMode_Solid_TextureLit_InterpVertex_LMOff_1TMU,	MGrMode_Solid_TextureLit_InterpVertex_LMOff_2TMU
			}
		},
		
		// M3cDrawState_Appearence_Texture_Unlit
		{
			// M3cDrawState_Interpolation_None
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrMode_Solid_TextureUnlit_InterpNone_LMOff_1TMU,	MGrMode_Solid_TextureUnlit_InterpNone_LMOff_2TMU
			},
			
			// M3cDrawState_Interpolation_Vertex
			{
				// M3cDrawState_LightMapMode_Off
				// 1TMU												// 2 TMU
				MGrMode_Solid_TextureUnlit_InterpVertex_LMOff_1TMU,	MGrMode_Solid_TextureUnlit_InterpVertex_LMOff_2TMU
			}
		}
	}
};

MGtVertexCreateFunction	MGgVertexCreateFuncs_Unified
	[M3cDrawState_Appearence_Num]
	[M3cDrawState_Interpolation_Num] = 
{
	// M3cDrawState_Appearence_Gouraud
	{
		// M3cDrawState_Interpolation_None
		MGrVertexCreate_XYZ,
		
		// M3cDrawState_Interpolation_Vertex
		MGrVertexCreate_XYZ_RGB
	},
	
	// M3cDrawState_Appearence_Texture_Lit
	{
		// M3cDrawState_Interpolation_None
		MGrVertexCreate_XYZ_BaseUV,
		
		// M3cDrawState_Interpolation_Vertex
		MGrVertexCreate_XYZ_RGB_BaseUV
	},
	
	// M3cDrawState_Appearence_Texture_Lit_EnvMap
	{
		// M3cDrawState_Interpolation_None
		MGrVertexCreate_XYZ_BaseUV,
		
		// M3cDrawState_Interpolation_Vertex
		MGrVertexCreate_XYZ_RGB_BaseUV_EnvUV
	},
	
	// M3cDrawState_Appearence_TextureUnlit
	{
		// M3cDrawState_Interpolation_None
		MGrVertexCreate_XYZ_BaseUV,
		
		// M3cDrawState_Interpolation_Vertex
		MGrVertexCreate_XYZ_BaseUV
	}
};

MGtVertexCreateFunction	MGgVertexCreateFuncs_Split
	[M3cDrawState_Appearence_Num]
	[M3cDrawState_Interpolation_Num]
	[MGcDrawState_TMU_Num] =
{
	// M3cDrawState_Appearence_Gouraud
	{
		// M3cDrawState_Interpolation_None
		{
			// M3cDrawState_LightMapMode_Off
			// 1 TMU			// 2 TMU
			MGrVertexCreate_XYZ,	MGrVertexCreate_XYZ
		},
		
		// M3cDrawState_Interpolation_Vertex
		{
			// M3cDrawState_LightMapMode_Off
			// 1 TMU			// 2 TMU
			MGrVertexCreate_XYZ_RGB,	MGrVertexCreate_XYZ_RGB
		}
	},
	
	// M3cDrawState_Appearence_Texture_Lit
	{
		// M3cDrawState_Interpolation_None
		{
			// M3cDrawState_LightMapMode_Off
			// 1 TMU			// 2 TMU
			MGrVertexCreate_XYZ,	MGrVertexCreate_XYZ
		},
		
		// M3cDrawState_Interpolation_Vertex
		{
			// M3cDrawState_LightMapMode_Off
			// 1 TMU			// 2 TMU
			MGrVertexCreate_XYZ_RGB,	MGrVertexCreate_XYZ_RGB
		}
	},
	
	// M3cDrawState_Appearence_Texture_Lit_EnvMap
	{
		// M3cDrawState_Interpolation_None
		{
			// M3cDrawState_LightMapMode_Off
			// 1 TMU			// 2 TMU
			MGrVertexCreate_XYZ,	MGrVertexCreate_XYZ
		},
		
		// M3cDrawState_Interpolation_Vertex
		{
			// M3cDrawState_LightMapMode_Off
			// 1 TMU			// 2 TMU
			MGrVertexCreate_XYZ_RGB,	MGrVertexCreate_XYZ_RGB
		}
	},
	
	// M3cDrawState_Appearence_Texture_Unlit
	{
		// M3cDrawState_Interpolation_None
		{
			// M3cDrawState_LightMapMode_Off
			// 1 TMU			// 2 TMU
			MGrVertexCreate_XYZ,	MGrVertexCreate_XYZ
		},
		
		// M3cDrawState_Interpolation_Vertex
		{
			// M3cDrawState_LightMapMode_Off
			// 1 TMU			// 2 TMU
			MGrVertexCreate_XYZ,	MGrVertexCreate_XYZ
		}
	}
};
