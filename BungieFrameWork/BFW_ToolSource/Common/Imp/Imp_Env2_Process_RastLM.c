/*
	FILE:	Imp_Env_Process_RastLM.c

	AUTHOR:	Brent H. Pease

	CREATED: Sept 3, 1997

	PURPOSE: 
	
	Copyright 1997

*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "BFW.h"
#include "BFW_FileManager.h"
#include "BFW_Motoko.h"
#include "BFW_AppUtilities.h"
#include "BFW_TM_Construction.h"
#include "BFW_MathLib.h"
#include "BFW_LSSolution.h"
#include "BFW_Collision.h"
#include "BFW_Shared_TriRaster.h"

#include "Imp_Env2_Private.h"



// Globals and constants for colour reduction
#define jitterx(x,y,s) (uranx[((x+(y<<2))+irand[(x+s)&IMPcLS_JitterMask])&IMPcLS_JitterMask])
#define jittery(x,y,s) (urany[((y+(x<<2))+irand[(y+s)&IMPcLS_JitterMask])&IMPcLS_JitterMask])
#define IMPcLS_JitterTableSize	1024
#define IMPcLS_JitterMask		(IMPcLS_JitterTableSize-1)
#define IMPcLS_JitterRandRange	1024

UUtUns16	irand[IMPcLS_JitterTableSize];	// Jitter look-up tables
double		uranx[IMPcLS_JitterTableSize];
double		urany[IMPcLS_JitterTableSize];
UUtBool		haveJitterTables = UUcFalse;


// Globals and constants for magic square dithering
#define DMAP(v,x,y)	(modN[v]>magic[x][y] ? divN[v] + 1 : divN[v])

int			grgbmap[256*256*256][3];
int			gdivn[256],gmodn[256],gmagic[16][16];
static int magic4x4[4][4] =  {
 	 0, 14,  3, 13,
	11,  5,  8,  6,
	12,  2, 15,  1,
	 7,  9,  4, 10
};
UUtBool		haveMagicSquare = UUcFalse;


// Other globals
UUtBool triVisible;
extern UUtBool renderLightmap;


static void iRGB2HSV(float r, float g, float b, float* h, float *s, float *v)
{
	float maxv,minv,diff,rd,gd,bd;
	
	maxv = UUmMax3(r,g,b);
	minv = UUmMin3(r,g,b);
	diff = maxv-minv;
	if (diff==0.0f) diff = 0.00001f;
	
	*v = maxv;
	if (maxv != 0.0f) *s = diff/maxv;
	else
	{
		*h = *s = *v = 0;
		return;
	}
	
	rd = (maxv-r)/diff;
	gd = (maxv-g)/diff;
	bd = (maxv-b)/diff;
	if (r == maxv) *h = bd-gd;
	else if (g == maxv)
	{
		*h = 2.0f + rd - bd;
	}
	else if (b == maxv)
	{
		*h = 4.0f + gd - rd;
	}
	
	*h *= 60.0f;
	if (*h < 0) *h += 360.0f;
	
	UUmAssert(*h >=0 && *h<=360.0f);
}

static void iHSV2RGB(float h, float s, float v, float *r, float *g, float *b)
{
	float f,p,q,t;
	UUtUns32 i;
	
	h = UUmPin(h,0.0f,360.0f);
	if (s==0.0f)
	{
		*r = v;
		*g = v;
		*b = v;
	}
	else
	{
		if (h==360.0f) h = 0.0f;
		h/=60.0f;
		i = (UUtUns32)h;
		f = h - (float)i;
		p = v * (1.0f - s);
		q = v * (1.0f - (s*f));
		t = v * (1.0f - (s*(1.0f - f)));
		switch (i)
		{
			case 0:
				*r = v;
				*g = t;
				*b = p;
				break;
			case 1:
				*r = q;
				*g = v;
				*b = p;
				break;
			case 2:
				*r = p;
				*g = v;
				*b = t;
				break;
			case 3:
				*r = p;
				*g = q;
				*b = v;
				break;
			case 4:
				*r = t;
				*g = p;
				*b = v;
				break;
			case 5:
				*r = v;
				*g = p;
				*b = q;
				break;
			default:
				UUmAssert(0);
		}
	}
}

static void iRGB5552HSV(UUtUns8 r, UUtUns8 g, UUtUns8 b, float *h, float *s, float *v)
{
	float fr,fg,fb;
	
	fr = ((float)(r<<3)) / 255.0f;
	fg = ((float)(g<<3)) / 255.0f;
	fb = ((float)(b<<3)) / 255.0f;
	iRGB2HSV(fr,fg,fb,h,s,v);
}

static void iHSV2RGBFilter(float h, float s, float v, UUtUns8 *r, UUtUns8 *g, UUtUns8 *b)
{
	float fr,fg,fb,minD,d,fh,fs,fv;
	UUtUns8 ir,ig,ib,minr,ming,minb;
	
	iHSV2RGB(h,s,v,&fr,&fg,&fb);
	fr*=255.0;
	fg*=255.0;
	fb*=255.0;
	fr = UUmPin(fr,0.0f,255.0f);
	fg = UUmPin(fg,0.0f,255.0f);
	fb = UUmPin(fb,0.0f,255.0f);
	
	ir = ((UUtUns8)fr) >>3;
	ig = ((UUtUns8)fg) >>3;
	ib = ((UUtUns8)fb) >>3;
	minD = 1e9;
	
	iRGB5552HSV(ir,ig,ib,&fh,&fs,&fv);											// 000
	d = UUmFabs(fh-h);
	if (UUmFabs(fh-h) < minD) { minD = d; minr = ir; ming = ig; minb = ib; }
	ir ^= 0x01;																	// 100
	iRGB5552HSV(ir,ig,ib,&fh,&fs,&fv);
	d = UUmFabs(fh-h);
	if (UUmFabs(fh-h) < minD) { minD = d; minr = ir; ming = ig; minb = ib; }
	ig ^= 0x01;																	// 110
	iRGB5552HSV(ir,ig,ib,&fh,&fs,&fv);
	d = UUmFabs(fh-h);
	if (UUmFabs(fh-h) < minD) { minD = d; minr = ir; ming = ig; minb = ib; }
	ib ^= 0x01;																	// 111
	iRGB5552HSV(ir,ig,ib,&fh,&fs,&fv);
	d = UUmFabs(fh-h);
	if (UUmFabs(fh-h) < minD) { minD = d; minr = ir; ming = ig; minb = ib; }
	ir ^= 0x01;																	// 011
	iRGB5552HSV(ir,ig,ib,&fh,&fs,&fv);
	d = UUmFabs(fh-h);
	if (UUmFabs(fh-h) < minD) { minD = d; minr = ir; ming = ig; minb = ib; }
	ig ^= 0x01;																	// 001
	iRGB5552HSV(ir,ig,ib,&fh,&fs,&fv);
	d = UUmFabs(fh-h);
	if (UUmFabs(fh-h) < minD) { minD = d; minr = ir; ming = ig; minb = ib; }
	ib ^= 0x01; ig ^= 0x01;														// 010
	iRGB5552HSV(ir,ig,ib,&fh,&fs,&fv);
	d = UUmFabs(fh-h);
	if (UUmFabs(fh-h) < minD) { minD = d; minr = ir; ming = ig; minb = ib; }
	ir ^= 0x01; ib ^= 0x01;														// 101
	iRGB5552HSV(ir,ig,ib,&fh,&fs,&fv);
	d = UUmFabs(fh-h);
	if (UUmFabs(fh-h) < minD) { minD = d; minr = ir; ming = ig; minb = ib; }
	
	*r = minr <<3;
	*g = ming <<3;
	*b = minb <<3;
}

	
/*****************************************************************
 * TAG( make_square )
 * 
 * Build the magic square for a given number of levels.
 * Inputs:
 * 	N:		Pixel values per level (255.0 / levels).
 * Outputs:
 * 	divN:		Integer value of pixval / N
 *	modN:		Integer remainder between pixval and divN[pixval]*N
 *	magic:		Magic square for dithering to N sublevels.
 * Assumptions:
 * 	
 * Algorithm:
 *	divN[pixval] = (int)(pixval / N) maps pixval to its appropriate level.
 *	modN[pixval] = pixval - (int)(N * divN[pixval]) maps pixval to
 *	its sublevel, and is used in the dithering computation.
 *	The magic square is computed as the (modified) outer product of
 *	a 4x4 magic square with itself.
 *	magic[4*k + i][4*l + j] = (magic4x4[i][j] + magic4x4[k][l]/16.0)
 *	multiplied by an appropriate factor to get the correct dithering
 *	range.
 */
static void
iMakeMagicSquare(double N, int divN[256], int modN[256], int magic[16][16] )
{
	// Build magic square for dithering
    register int i, j, k, l;
    double magicfact;

    for ( i = 0; i < 256; i++ )
    {
	divN[i] = (int)(i / N);
	modN[i] = i - (int)(N * divN[i]);
    }
    modN[255] = 0;		/* always */

    /*
     * Expand 4x4 dither pattern to 16x16.  4x4 leaves obvious patterning,
     * and doesn't give us full intensity range (only 17 sublevels).
     * 
     * magicfact is (N - 1)/16 so that we get numbers in the matrix from 0 to
     * N - 1: mod N gives numbers in 0 to N - 1, don't ever want all
     * pixels incremented to the next level (this is reserved for the
     * pixel value with mod N == 0 at the next level).
     */
    magicfact = (N - 1) / 16.;
    for ( i = 0; i < 4; i++ )
	for ( j = 0; j < 4; j++ )
	    for ( k = 0; k < 4; k++ )
		for ( l = 0; l < 4; l++ )
		    magic[4*k+i][4*l+j] =
			(int)(0.5 + magic4x4[i][j] * magicfact +
			      (magic4x4[k][l] / 16.) * magicfact);
}


	
static void
iDitherMap(int levels, double gamma, int rgbmap[][3], int divN[256], int modN[256], int magic[16][16] )
{
	// Create the colour look-up map for dithered colour approximations
    double N;
    register int i;
    int levelsq, levelsc;
    int gammamap[256];
    
    for ( i = 0; i < 256; i++ )
	gammamap[i] = (int)(0.5 + 255 * pow( i / 255.0, 1.0/gamma ));

    levelsq = levels*levels;	/* squared */
    levelsc = levels*levelsq;	/* and cubed */

    N = 255.0 / (levels - 1);    /* Get size of each step */

    /* 
     * Set up the color map entries.
     */
    for(i = 0; i < levelsc; i++) {
	rgbmap[i][0] = gammamap[(int)(0.5 + (i%levels) * N)];
	rgbmap[i][1] = gammamap[(int)(0.5 + ((i/levels)%levels) * N)];
	rgbmap[i][2] = gammamap[(int)(0.5 + ((i/levelsq)%levels) * N)];
    }

    iMakeMagicSquare( N, divN, modN, magic );
    haveMagicSquare = UUcTrue;
}


static int iDitherRGB(int x, int y, int r, int g, int b, int levels, int divN[256], int modN[256], int magic[16][16] )
{
	// Perform a magic square dither of an RGB pixel
    int col = x % 16, row = y % 16;

	if (!haveMagicSquare) iDitherMap(256,0.5,grgbmap,gdivn,gmodn,gmagic);
	
    return DMAP(r, col, row) +
	DMAP(g, col, row) * levels +
	    DMAP(b, col, row) * levels*levels;
}




static void iCreateJitterTables(void)
{
	int i;

	// Initialize colour reduction jitter tables
	for (i = 0; i < IMPcLS_JitterTableSize; ++i)
	    uranx[i] = (double)(rand() % IMPcLS_JitterRandRange) / (double)IMPcLS_JitterRandRange;
	for (i = 0; i < IMPcLS_JitterTableSize; ++i)
	    urany[i] = (double)(rand() % IMPcLS_JitterRandRange) / (double)IMPcLS_JitterRandRange;
	for (i = 0; i < IMPcLS_JitterTableSize; ++i)
	    irand[i] = (int)((double)IMPcLS_JitterTableSize
	        * ((double)(rand() % IMPcLS_JitterRandRange) / (double)IMPcLS_JitterRandRange));
}

void
iReduceColourSpace(
	UUtUns8 *ioR,
	UUtUns8 *ioG,
	UUtUns8 *ioB,
	UUtUns16 inNoise,
	UUtUns16 inPixelX,
	UUtUns16 inPixelY)
{
	UUtInt32 i, p, q;
	UUtUns8 *rgb = ioR;
	
	// Initialize
	if (!haveJitterTables) iCreateJitterTables();
	 
	if (inNoise == 0)
    {
    	*ioR &= 0xF8;
    	*ioG &= 0xF8;
    	*ioB &= 0xF8;
	    return;
    }

	for (i = 0; i <= 2; ++i)
    {
    	if (i==1) rgb = ioG;
    	else if (i==2) rgb = ioB;
    	
    	if (*rgb < 248)
        {
	        p = *rgb % 8;
	        q = (UUtInt32)(jitterx(inPixelX, inPixelY, i) * 9.0);
	        if (p <= q) *rgb += 8;

	        // Add some noise
	        q = 8 * ((UUtInt32)((jittery(inPixelX, inPixelY, i)
	            * (double)(2 * inNoise)) + 0.5)
	            - inNoise);
	        q += (UUtInt32)(*rgb);

	        // Make sure resulting intensity is within allowable range
	        if (q >= 0 && q <= 255) *rgb = (UUtUns8)q;

	        // Shift
	        *rgb &= 0xF8;//>>= 3;
        }
    }
}


void
iRasterize_LightMap16(
	IMPtEnv_LM_Vertex*	inVertex0,
	IMPtEnv_LM_Vertex*	inVertex1,
	IMPtEnv_LM_Vertex*	inVertex2,
	UUtUns16*			inMapMemory,
	UUtUns16			inRowBytes,
	UUtInt16			inWidth,
	UUtInt16			inHeight,
	float				inBrightness,
	float				inContrast,
	UUtBool				inDaylightOn,
	UUtBool				inIsExterior)
{
	
	IMPtEnv_LM_Vertex*	vertex_min;
	IMPtEnv_LM_Vertex*	vertex_mid;
	IMPtEnv_LM_Vertex*	vertex_max;
	
	/*
	 * Key to variable names
	 * 
	 * ddx = d/dx, horizontal incremental change
	 * ddy = d/dy, vertical incremental change
	 *
	 * The type is appended to ease the scheduling of integer and floating point variables
	 */
	
	
	/*
	 * x left and right variables
	 */
		UUtInt32	u_longEdge_int,			u_longEdge_ddv_int;
		UUtInt32	u_leftEdge_int,			u_leftEdge_ddv_int;
		UUtInt32	u_rightEdge_int,		u_rightEdge_ddv_int;
		UUtInt32	u_shortEdgeBottom_int,	u_shortEdgeBottom_ddv_int;
		UUtInt32	u_shortEdgeTop_int,		u_shortEdgeTop_ddv_int;
		
	/*
	 * These variables have double duty. They are used to store the int version of the 
	 * min, mid, max rgb values and the variable interpolation values for rgb
	 */
		float	r_max_ddv_float, g_max_ddv_float, b_max_ddv_float;
		float	r_mid_ddu_float, g_mid_ddu_float, b_mid_ddu_float;
		float	r_min_longEdge_float, g_min_longEdge_float, b_min_longEdge_float;
	
	/*
	 * top, middle, and bottom scanlines
	 */
		UUtInt32	scanline_min_int, scanline_mid_int, scanline_max_int;
		
	/*
	 * Pointers into the buffers and the rowBytes
	 */
	 	
		UUtUns16*	imageScanlineLeft_ptr;
		UUtUns16	imageRowBytes_uns;
		
	/*
	 * Used to indicate whether scanline traversal is left->right or right->left
	 */
		UUtInt16	spanDirection_int;
	
	/*
	 * min, mid, and max variables for x and y
	 */
		float	v_min_float, v_mid_float, v_max_float;
		float	u_min_float, u_mid_float, u_max_float;
		float	v0_float, v1_float, v2_float;
	
	/*
	 * Plane equation variables
	 */
		float		m10_float, m11_float, m12_float;
		float		invDet_float;
		float		v_maxMinusMin_float, v_midMinusMin_float;
		float		u_maxMinusMin_float, u_midMinusMin_float;
	
	/*
	 * Error variables
	 */
		float	v_minError_float, v_midError_float;
	
	/*
	 * Temporary variables
	 */
		float			temp_float;
		float			r_temp_float, g_temp_float, b_temp_float;
				
	/*
	 * floating point constants
	 */
		
		float	u_scale_float = MSmFractOne * MSmSubPixelUnits;
		float	oneHalf_float = 0.5F;
		float	one_float = 1.0F;
	
	/*
	 * Scanline variables
	 */
		UUtInt32	scanline_int;
		
		UUtInt32	ptr_ddu_int;
		
		UUtUns16	*image_ptr;
		
	/*
	 * Pixel & lightmap variables
	 */		
		UUtInt32	u_pixelLeft_int, u_pixelRight_int;
		UUtInt32	i;
		float		a, b, c;
		float		intensity, f;
		float		red, green, blue;
		UUtUns8 	intR,intG,intB;
					
					
	/*
	 * DDA variables
	 */
		float		invHeight_float;
		
	
	// Set up irradiance coefficients	
	{
		float mag, power, step, stepsize, res, powerBot;
		float orders_of_mag = 5.0f;

		b = inBrightness < 0.0f ? 0.0f : inBrightness * 0.7f;

		powerBot = inDaylightOn && inIsExterior ? 4.0f : 2.0f;

		res = 100.0f / orders_of_mag;
		stepsize = 9.0f / res;
		mag = (float) floor(50.0f / res);
		step = 50.0f - mag * res;
		power = b / 20.0f - orders_of_mag - powerBot + mag;
		b = (float)(pow(10.0f, power) * (1 + stepsize * step));

		c = 0.02f * inContrast;
		a = b * (1.0f + c);
		b *= (float)M3cPi;
	}

	/*
	 * Step 1. Sort the vertices according to v. Sort in place to save registers
	 */
		
		triVisible = UUcFalse;
		
		v0_float = inVertex0->v;
		v1_float = inVertex1->v;
		v2_float = inVertex2->v;
		
		if (v0_float < v1_float)
		{
			v_min_float = v0_float;
			v_mid_float = v1_float;

			vertex_min = inVertex0;
			vertex_mid = inVertex1;
		}
		else
		{
			v_min_float = v1_float;
			v_mid_float = v0_float;
			
			vertex_min = inVertex1;
			vertex_mid = inVertex0;
		}
		
		if (v2_float < v_min_float)
		{
			v_max_float = v_mid_float;
			v_mid_float = v_min_float;
			v_min_float = v2_float;
			
			vertex_max = vertex_mid;
			vertex_mid = vertex_min;
			vertex_min = inVertex2;
		}
		else if (v2_float < v_mid_float)
		{
			v_max_float = v_mid_float;
			v_mid_float = v2_float;
						
			vertex_max = vertex_mid;
			vertex_mid = inVertex2;
		}
		else
		{
			v_max_float = v2_float;
			vertex_max = inVertex2;
		}
	
		UUmAssert(v_max_float >= v_mid_float);
		UUmAssert(v_mid_float >= v_min_float);

	/*
	 * Step 2. Compute plane equations and interwind load/store operations
	 */
		
		/* prefetch x mid, min, max */
		u_min_float = vertex_min->u;
		u_mid_float = vertex_mid->u;
		u_max_float = vertex_max->u;
		
	
		// Vertex irradiance setup
		#define colour_convert(ir,ig,ib,dr,dg,db)	\
			intensity = UUmFabs(ir) * 0.263f +	\
				UUmFabs(ig) * 0.655f +	\
				UUmFabs(ib) * 0.082f;	\
			\
			f = (float)(a / (b * intensity + c)) * 10.0f;	\
			\
			red		= (ir * f) / M3cPi;	\
			green	= (ig * f) / M3cPi;	\
			blue	= (ib * f) / M3cPi;	\
			\
			red = UUmPin(red, 0.0f, 1.0f);	\
			green = UUmPin(green, 0.0f, 1.0f);	\
			blue = UUmPin(blue, 0.0f, 1.0f);	\
			iRGB2HSV(red,green,blue,&dr,&dg,&db);
			
	
		colour_convert(vertex_min->r,vertex_min->g,vertex_min->b,r_min_longEdge_float,g_min_longEdge_float,b_min_longEdge_float);
		colour_convert(vertex_mid->r,vertex_mid->g,vertex_mid->b,r_mid_ddu_float,g_mid_ddu_float,b_mid_ddu_float);
		colour_convert(vertex_max->r,vertex_max->g,vertex_max->b,r_max_ddv_float,g_max_ddv_float,b_max_ddv_float);
		
		/* Calc plane equation stuff */
		v_maxMinusMin_float = v_max_float - v_min_float;
		u_midMinusMin_float = u_mid_float - u_min_float;
		v_midMinusMin_float = v_mid_float - v_min_float;
		u_maxMinusMin_float = u_max_float - u_min_float;
		invDet_float = v_maxMinusMin_float * u_midMinusMin_float - v_midMinusMin_float * u_maxMinusMin_float;
		invDet_float = one_float / invDet_float;
		
		if (invDet_float > 0.0f)
		{
			spanDirection_int = 1;
		}
		else
		{
			invDet_float = -invDet_float;
			spanDirection_int = -1;
		}
		
		/* Compute plane equation */
		m10_float = (v_mid_float - v_max_float) * invDet_float;
		m11_float = v_maxMinusMin_float * invDet_float;
		m12_float = -v_midMinusMin_float * invDet_float;
		
	/*
	 * Step 4. compute ddx parameters and the first, mid and bottom scanlines
	 * Step 5. Set up the vertical DDA
	 */
		
		scanline_min_int = (UUtInt32)(v_min_float + oneHalf_float);
		scanline_mid_int = (UUtInt32)(v_mid_float + oneHalf_float);
		v_minError_float = ((float)scanline_min_int + oneHalf_float) - v_min_float;
		v_midError_float = ((float)scanline_mid_int + oneHalf_float) - v_mid_float;
		
		/* start using r_mid_ddx_int as the ddx instead of the mid */
		r_mid_ddu_float = m11_float * r_mid_ddu_float;					
		g_mid_ddu_float = m11_float * g_mid_ddu_float;					
		b_mid_ddu_float = m11_float * b_mid_ddu_float;					
		
		temp_float = v_max_float - oneHalf_float;
		invHeight_float = u_scale_float / v_midMinusMin_float;
		u_min_float = u_min_float * u_scale_float;
		
		/* If the triangle does not hit the first scanline on the screen bail */
		if(temp_float < 0.5f)
		{
			return;
		}

		scanline_max_int = (UUtInt32)temp_float;

		/*
		 * This code section sets up the vertical DDA for all three edges.
		 * Begin by computing the inverse of the height of the short top edge.
		 */
		
		r_mid_ddu_float += m10_float * r_min_longEdge_float;
		g_mid_ddu_float += m10_float * g_min_longEdge_float;
		b_mid_ddu_float += m10_float * b_min_longEdge_float;
		
		temp_float = u_midMinusMin_float * invHeight_float;
		
		r_mid_ddu_float += m12_float * r_max_ddv_float;
		g_mid_ddu_float += m12_float * g_max_ddv_float;
		b_mid_ddu_float += m12_float * b_max_ddv_float;
		
		u_shortEdgeTop_int = (UUtInt32)(u_min_float + v_minError_float * temp_float);
		u_shortEdgeTop_ddv_int = (UUtInt32)temp_float;
		
	/*
	 * Compute the inverse of the height of the long edge (and do a bit more
	 * pointer setup).
	 */
		invHeight_float = one_float / v_maxMinusMin_float;
		
		imageRowBytes_uns = inRowBytes;
		
		imageScanlineLeft_ptr = (UUtUns16 *)((char *)inMapMemory +
								(scanline_min_int * imageRowBytes_uns));
	
	/*
	 * Compute the initial value and forward difference for all interpolated parameters
	 * along the long edge.
	 */
	
	temp_float = u_maxMinusMin_float * u_scale_float * invHeight_float;
	u_longEdge_int = (UUtInt32)(u_min_float + v_minError_float * temp_float);
	u_longEdge_ddv_int = (UUtInt32)temp_float;


	r_max_ddv_float = (r_max_ddv_float - r_min_longEdge_float) * invHeight_float;	
	g_max_ddv_float = (g_max_ddv_float - g_min_longEdge_float) * invHeight_float;
	b_max_ddv_float = (b_max_ddv_float - b_min_longEdge_float) * invHeight_float;
	
	invHeight_float = u_scale_float / (v_maxMinusMin_float - v_midMinusMin_float);
	
	temp_float = (u_max_float - u_mid_float) * invHeight_float;
	
	r_min_longEdge_float += (r_max_ddv_float * v_minError_float);
	g_min_longEdge_float += (g_max_ddv_float * v_minError_float);
	b_min_longEdge_float += (b_max_ddv_float * v_minError_float);
	
	
	/*
	 * Compute the inital value and forward difference of X for the bottom short edge.
	 */
	
	// XXX - Fucking VisC++ bug
	{
		float t2;
		t2 = (u_mid_float * u_scale_float + v_midError_float * temp_float);

		u_shortEdgeBottom_int = (UUtInt32)t2;
	}
	u_shortEdgeBottom_ddv_int = (UUtInt32)temp_float;

	/*
	 * Assign the xLong and xShortTop DDA information to xLeft and xRight, as
	 * indicated by spanDirection. The spanDirection test also indicates whether
	 * the bottom short edge is left or right, so add the appropriate sub-pixel bias
	 * to xShortBottom as well.
	 */
	
	if (spanDirection_int > 0)
	{
		/*
		 * vMin-vMax (long) is left edge. vMin-vMid (shortTop) is the top right edge,
		 * and vMid-vMax is the bottom right edge.
		 */
		
		u_leftEdge_int = u_longEdge_int;
		u_leftEdge_ddv_int = u_longEdge_ddv_int;
		
		u_rightEdge_int = u_shortEdgeTop_int;
		u_rightEdge_ddv_int = u_shortEdgeTop_ddv_int;
		
		u_shortEdgeBottom_int -= (MSmHalfSubPixel + 1) << MSmFractBits;
	}
	else
	{
		/*
		 * vMin-vMax (short) is right edge, vMin-vMid (shortTop) is the top left edge,
		 * and vMid-vMax is the bottom left edge.
		 */
		
		u_rightEdge_int = u_longEdge_int;
		u_rightEdge_ddv_int = u_longEdge_ddv_int;
		
		u_leftEdge_int = u_shortEdgeTop_int;
		u_leftEdge_ddv_int = u_shortEdgeTop_ddv_int;
		
		u_shortEdgeBottom_int += (MSmHalfSubPixel - 1) << MSmFractBits;
	}
	
	/*
	 * Add in the sub-pixel coverage bias to xLeft and xRight.
	 */
	
	u_leftEdge_int += (MSmHalfSubPixel - 1) << MSmFractBits;
	u_rightEdge_int -= (MSmHalfSubPixel + 1) << MSmFractBits;
	
	for (scanline_int = scanline_min_int;
		scanline_int <= scanline_max_int;
		++scanline_int)
	{
		if(scanline_int >= inHeight)
		{
			break;
		}
		
		if (scanline_int == scanline_mid_int)
		{
			if (spanDirection_int > 0)
			{
				/*
				 * Right vMin-vMid leg has ended, replace with vMid-vMax.
				 * Set xRight to the exact subpixel-accurate value that was computed
				 * earlier.
				 */
				
				u_rightEdge_ddv_int = u_shortEdgeBottom_ddv_int;
				u_rightEdge_int = u_shortEdgeBottom_int;
			}
			else
			{
				/*
				 * Left vMin-vMid leg has ended, replace with vMid-vMax.
				 * Set xLeft to the exact subpixel-accurate value that was computed
				 * earlier.
				 */
				
				u_leftEdge_ddv_int = u_shortEdgeBottom_ddv_int;
				u_leftEdge_int = u_shortEdgeBottom_int;
			}
		}
		
		/*
		 * Render a horizontal span. Note that the sub-pixel coverage bias has
		 * already been added into xLeft and xRight.
		 */
		
		u_pixelLeft_int = u_leftEdge_int >> (MSmSubPixelBits + MSmFractBits);
		u_pixelRight_int = u_rightEdge_int >> (MSmSubPixelBits + MSmFractBits);
		
		if (spanDirection_int > 0)
		{
			/*
			 * We are rasterizing from left to right. Compute the pointers to the first
			 * pixel in the span.
			 */
				
				image_ptr = imageScanlineLeft_ptr + u_pixelLeft_int;
				
				ptr_ddu_int = 1;
				
		}
		else
		{
			/*
			 * We are rasterizing from right to left.
			 */
				
				image_ptr = imageScanlineLeft_ptr + u_pixelRight_int;
				
				ptr_ddu_int = -1;
				

		}
		
		r_temp_float = r_min_longEdge_float;
		g_temp_float = g_min_longEdge_float;
		b_temp_float = b_min_longEdge_float;
		
		/*
		 * Horizontal interpolation loop.
		 */
		if(scanline_int >= 0)
		{
			for (i = 1 + u_pixelRight_int - u_pixelLeft_int; i-- > 0; /* Nothing */)
			{
				if(image_ptr >= imageScanlineLeft_ptr && image_ptr < (UUtUns16*)((char *)imageScanlineLeft_ptr + imageRowBytes_uns))
				{
					iHSV2RGBFilter(r_temp_float,g_temp_float,b_temp_float,&intR,&intG,&intB);
					
					*image_ptr = 
						((UUtUns8)(intR >> 3) << 10) |
						((UUtUns8)(intG >> 3) << 5) |
						((UUtUns8)(intB >> 3) << 0);
					triVisible = UUcTrue;
				}
				
				image_ptr += ptr_ddu_int;
				
				r_temp_float += r_mid_ddu_float;
				g_temp_float += g_mid_ddu_float;
				b_temp_float += b_mid_ddu_float;
			}
		}

		/*
		 * Advance to next scanline
		 */
		
		u_leftEdge_int += u_leftEdge_ddv_int;
		u_rightEdge_int += u_rightEdge_ddv_int;
		
		imageScanlineLeft_ptr = (UUtUns16 *)((char *)imageScanlineLeft_ptr + imageRowBytes_uns);
	
		r_min_longEdge_float += r_max_ddv_float;
		g_min_longEdge_float += g_max_ddv_float;
		b_min_longEdge_float += b_max_ddv_float;
	}
	
	//if (renderLightmap) UUmAssert(triVisible == UUcTrue);
}

void
iRasterize_LightMap32(
	IMPtEnv_LM_Vertex*	inVertex0,
	IMPtEnv_LM_Vertex*	inVertex1,
	IMPtEnv_LM_Vertex*	inVertex2,
	UUtUns32*			inMapMemory,
	UUtUns16			inRowBytes,
	UUtInt16			inWidth,
	UUtInt16			inHeight,
	float				inBrightness,
	float				inContrast,
	UUtBool				inDaylightOn,
	UUtBool				inIsExterior)
{
	
	IMPtEnv_LM_Vertex*	vertex_min;
	IMPtEnv_LM_Vertex*	vertex_mid;
	IMPtEnv_LM_Vertex*	vertex_max;
	
	/*
	 * Key to variable names
	 * 
	 * ddx = d/dx, horizontal incremental change
	 * ddy = d/dy, vertical incremental change
	 *
	 * The type is appended to ease the scheduling of integer and floating point variables
	 */
	
	
	/*
	 * x left and right variables
	 */
		UUtInt32	u_longEdge_int,			u_longEdge_ddv_int;
		UUtInt32	u_leftEdge_int,			u_leftEdge_ddv_int;
		UUtInt32	u_rightEdge_int,		u_rightEdge_ddv_int;
		UUtInt32	u_shortEdgeBottom_int,	u_shortEdgeBottom_ddv_int;
		UUtInt32	u_shortEdgeTop_int,		u_shortEdgeTop_ddv_int;
		
	/*
	 * These variables have double duty. They are used to store the int version of the 
	 * min, mid, max rgb values and the variable interpolation values for rgb
	 */
		float	r_max_ddv_float, g_max_ddv_float, b_max_ddv_float;
		float	r_mid_ddu_float, g_mid_ddu_float, b_mid_ddu_float;
		float	r_min_longEdge_float, g_min_longEdge_float, b_min_longEdge_float;
	
	/*
	 * top, middle, and bottom scanlines
	 */
		UUtInt32	scanline_min_int, scanline_mid_int, scanline_max_int;
		
	/*
	 * Pointers into the buffers and the rowBytes
	 */
	 	
		UUtUns32*	imageScanlineLeft_ptr;
		UUtUns16	imageRowBytes_uns;
		
	/*
	 * Used to indicate whether scanline traversal is left->right or right->left
	 */
		UUtInt16	spanDirection_int;
	
	/*
	 * min, mid, and max variables for x and y
	 */
		float	v_min_float, v_mid_float, v_max_float;
		float	u_min_float, u_mid_float, u_max_float;
		float	v0_float, v1_float, v2_float;
	
	/*
	 * Plane equation variables
	 */
		float		m10_float, m11_float, m12_float;
		float		invDet_float;
		float		v_maxMinusMin_float, v_midMinusMin_float;
		float		u_maxMinusMin_float, u_midMinusMin_float;
	
	/*
	 * Error variables
	 */
		float	v_minError_float, v_midError_float;
	
	/*
	 * Temporary variables
	 */
		float			temp_float;
		float			r_temp_float, g_temp_float, b_temp_float;
				
	/*
	 * floating point constants
	 */
		
		float	u_scale_float = MSmFractOne * MSmSubPixelUnits;
		float	oneHalf_float = 0.5F;
		float	one_float = 1.0F;
	
	/*
	 * Scanline variables
	 */
		UUtInt32	scanline_int;
		
		UUtInt32	ptr_ddu_int;
		
		UUtUns32	*image_ptr;
		
	/*
	 * Pixel & lightmap variables
	 */		
		UUtInt32	u_pixelLeft_int, u_pixelRight_int;
		UUtInt32	i;
					
	/*
	 * DDA variables
	 */
		float		invHeight_float;
	
	// Irradience stuff
		float		a, b, c;
		float		intensity, f;

	// Set up irradiance coefficients	
	{
		float mag, power, step, stepsize, res, powerBot;
		float orders_of_mag = 5.0f;

		b = inBrightness < 0.0f ? 0.0f : inBrightness * 0.7f;

		powerBot = inDaylightOn && inIsExterior ? 4.0f : 2.0f;

		res = 100.0f / orders_of_mag;
		stepsize = 9.0f / res;
		mag = (float) floor(50.0f / res);
		step = 50.0f - mag * res;
		power = b / 20.0f - orders_of_mag - powerBot + mag;
		b = (float)(pow(10.0f, power) * (1 + stepsize * step));

		c = 0.02f * inContrast;
		a = b * (1.0f + c);
		b *= (float)M3cPi;
	}
	/*
	 * Step 1. Sort the vertices according to v. Sort in place to save registers
	 */
		
		triVisible = UUcFalse;
		
		v0_float = inVertex0->v;
		v1_float = inVertex1->v;
		v2_float = inVertex2->v;
		
		if (v0_float < v1_float)
		{
			v_min_float = v0_float;
			v_mid_float = v1_float;

			vertex_min = inVertex0;
			vertex_mid = inVertex1;
		}
		else
		{
			v_min_float = v1_float;
			v_mid_float = v0_float;
			
			vertex_min = inVertex1;
			vertex_mid = inVertex0;
		}
		
		if (v2_float < v_min_float)
		{
			v_max_float = v_mid_float;
			v_mid_float = v_min_float;
			v_min_float = v2_float;
			
			vertex_max = vertex_mid;
			vertex_mid = vertex_min;
			vertex_min = inVertex2;
		}
		else if (v2_float < v_mid_float)
		{
			v_max_float = v_mid_float;
			v_mid_float = v2_float;
						
			vertex_max = vertex_mid;
			vertex_mid = inVertex2;
		}
		else
		{
			v_max_float = v2_float;
			vertex_max = inVertex2;
		}
	
		UUmAssert(v_max_float >= v_mid_float);
		UUmAssert(v_mid_float >= v_min_float);

	/*
	 * Step 2. Compute plane equations and interwind load/store operations
	 */
		
		/* prefetch x mid, min, max */
		u_min_float = vertex_min->u;
		u_mid_float = vertex_mid->u;
		u_max_float = vertex_max->u;
		
		
		// Vertex irradiance setup
		#undef colour_convert
		#define colour_convert(ir,ig,ib,dr,dg,db)	\
			intensity = UUmFabs(ir) * 0.263f +	\
				UUmFabs(ig) * 0.655f +	\
				UUmFabs(ib) * 0.082f;	\
			\
			f = (float)(a / (b * intensity + c)) * 10.0f;	\
			\
			dr	= (ir * f) / M3cPi;	\
			dg	= (ig * f) / M3cPi;	\
			db	= (ib * f) / M3cPi;	\
			\
			dr = UUmPin(dr, 0.0f, 1.0f) * 255.9f;	\
			dg = UUmPin(dg, 0.0f, 1.0f) * 255.9f;	\
			db = UUmPin(db, 0.0f, 1.0f) * 255.9f;
			
	
		colour_convert(vertex_min->r,vertex_min->g,vertex_min->b,r_min_longEdge_float,g_min_longEdge_float,b_min_longEdge_float);
		colour_convert(vertex_mid->r,vertex_mid->g,vertex_mid->b,r_mid_ddu_float,g_mid_ddu_float,b_mid_ddu_float);
		colour_convert(vertex_max->r,vertex_max->g,vertex_max->b,r_max_ddv_float,g_max_ddv_float,b_max_ddv_float);
		
		/* Calc plane equation stuff */
		v_maxMinusMin_float = v_max_float - v_min_float;
		u_midMinusMin_float = u_mid_float - u_min_float;
		v_midMinusMin_float = v_mid_float - v_min_float;
		u_maxMinusMin_float = u_max_float - u_min_float;
		invDet_float = v_maxMinusMin_float * u_midMinusMin_float - v_midMinusMin_float * u_maxMinusMin_float;
		invDet_float = one_float / invDet_float;
		
		if (invDet_float > 0.0f)
		{
			spanDirection_int = 1;
		}
		else
		{
			invDet_float = -invDet_float;
			spanDirection_int = -1;
		}
		
		/* Compute plane equation */
		m10_float = (v_mid_float - v_max_float) * invDet_float;
		m11_float = v_maxMinusMin_float * invDet_float;
		m12_float = -v_midMinusMin_float * invDet_float;
		
	/*
	 * Step 4. compute ddx parameters and the first, mid and bottom scanlines
	 * Step 5. Set up the vertical DDA
	 */
		
		scanline_min_int = (UUtInt32)(v_min_float + oneHalf_float);
		scanline_mid_int = (UUtInt32)(v_mid_float + oneHalf_float);
		v_minError_float = ((float)scanline_min_int + oneHalf_float) - v_min_float;
		v_midError_float = ((float)scanline_mid_int + oneHalf_float) - v_mid_float;
		
		/* start using r_mid_ddx_int as the ddx instead of the mid */
		r_mid_ddu_float = m11_float * r_mid_ddu_float;					
		g_mid_ddu_float = m11_float * g_mid_ddu_float;					
		b_mid_ddu_float = m11_float * b_mid_ddu_float;					
		
		temp_float = v_max_float - oneHalf_float;
		invHeight_float = u_scale_float / v_midMinusMin_float;
		u_min_float = u_min_float * u_scale_float;
		
		/* If the triangle does not hit the first scanline on the screen bail */
		if(temp_float < 0.5f)
		{
			return;
		}

		scanline_max_int = (UUtInt32)temp_float;

		/*
		 * This code section sets up the vertical DDA for all three edges.
		 * Begin by computing the inverse of the height of the short top edge.
		 */
		
		r_mid_ddu_float += m10_float * r_min_longEdge_float;
		g_mid_ddu_float += m10_float * g_min_longEdge_float;
		b_mid_ddu_float += m10_float * b_min_longEdge_float;
		
		temp_float = u_midMinusMin_float * invHeight_float;
		
		r_mid_ddu_float += m12_float * r_max_ddv_float;
		g_mid_ddu_float += m12_float * g_max_ddv_float;
		b_mid_ddu_float += m12_float * b_max_ddv_float;
		
		u_shortEdgeTop_int = (UUtInt32)(u_min_float + v_minError_float * temp_float);
		u_shortEdgeTop_ddv_int = (UUtInt32)temp_float;
		
	/*
	 * Compute the inverse of the height of the long edge (and do a bit more
	 * pointer setup).
	 */
		invHeight_float = one_float / v_maxMinusMin_float;
		
		imageRowBytes_uns = inRowBytes;
		
		imageScanlineLeft_ptr = (UUtUns32 *)((char *)inMapMemory +
								(scanline_min_int * imageRowBytes_uns));
	
	/*
	 * Compute the initial value and forward difference for all interpolated parameters
	 * along the long edge.
	 */
	
	temp_float = u_maxMinusMin_float * u_scale_float * invHeight_float;
	u_longEdge_int = (UUtInt32)(u_min_float + v_minError_float * temp_float);
	u_longEdge_ddv_int = (UUtInt32)temp_float;


	r_max_ddv_float = (r_max_ddv_float - r_min_longEdge_float) * invHeight_float;	
	g_max_ddv_float = (g_max_ddv_float - g_min_longEdge_float) * invHeight_float;
	b_max_ddv_float = (b_max_ddv_float - b_min_longEdge_float) * invHeight_float;
	
	invHeight_float = u_scale_float / (v_maxMinusMin_float - v_midMinusMin_float);
	
	temp_float = (u_max_float - u_mid_float) * invHeight_float;
	
	r_min_longEdge_float += (r_max_ddv_float * v_minError_float);
	g_min_longEdge_float += (g_max_ddv_float * v_minError_float);
	b_min_longEdge_float += (b_max_ddv_float * v_minError_float);
	
	
	/*
	 * Compute the inital value and forward difference of X for the bottom short edge.
	 */
	
	// XXX - Fucking VisC++ bug
	{
		float t2;
		t2 = (u_mid_float * u_scale_float + v_midError_float * temp_float);

		u_shortEdgeBottom_int = (UUtInt32)t2;
	}
	u_shortEdgeBottom_ddv_int = (UUtInt32)temp_float;

	/*
	 * Assign the xLong and xShortTop DDA information to xLeft and xRight, as
	 * indicated by spanDirection. The spanDirection test also indicates whether
	 * the bottom short edge is left or right, so add the appropriate sub-pixel bias
	 * to xShortBottom as well.
	 */
	
	if (spanDirection_int > 0)
	{
		/*
		 * vMin-vMax (long) is left edge. vMin-vMid (shortTop) is the top right edge,
		 * and vMid-vMax is the bottom right edge.
		 */
		
		u_leftEdge_int = u_longEdge_int;
		u_leftEdge_ddv_int = u_longEdge_ddv_int;
		
		u_rightEdge_int = u_shortEdgeTop_int;
		u_rightEdge_ddv_int = u_shortEdgeTop_ddv_int;
		
		u_shortEdgeBottom_int -= (MSmHalfSubPixel + 1) << MSmFractBits;
	}
	else
	{
		/*
		 * vMin-vMax (short) is right edge, vMin-vMid (shortTop) is the top left edge,
		 * and vMid-vMax is the bottom left edge.
		 */
		
		u_rightEdge_int = u_longEdge_int;
		u_rightEdge_ddv_int = u_longEdge_ddv_int;
		
		u_leftEdge_int = u_shortEdgeTop_int;
		u_leftEdge_ddv_int = u_shortEdgeTop_ddv_int;
		
		u_shortEdgeBottom_int += (MSmHalfSubPixel - 1) << MSmFractBits;
	}
	
	/*
	 * Add in the sub-pixel coverage bias to xLeft and xRight.
	 */
	
	u_leftEdge_int += (MSmHalfSubPixel - 1) << MSmFractBits;
	u_rightEdge_int -= (MSmHalfSubPixel + 1) << MSmFractBits;
	
	for (scanline_int = scanline_min_int;
		scanline_int <= scanline_max_int;
		++scanline_int)
	{
		if(scanline_int >= inHeight)
		{
			break;
		}
		
		if (scanline_int == scanline_mid_int)
		{
			if (spanDirection_int > 0)
			{
				/*
				 * Right vMin-vMid leg has ended, replace with vMid-vMax.
				 * Set xRight to the exact subpixel-accurate value that was computed
				 * earlier.
				 */
				
				u_rightEdge_ddv_int = u_shortEdgeBottom_ddv_int;
				u_rightEdge_int = u_shortEdgeBottom_int;
			}
			else
			{
				/*
				 * Left vMin-vMid leg has ended, replace with vMid-vMax.
				 * Set xLeft to the exact subpixel-accurate value that was computed
				 * earlier.
				 */
				
				u_leftEdge_ddv_int = u_shortEdgeBottom_ddv_int;
				u_leftEdge_int = u_shortEdgeBottom_int;
			}
		}
		
		/*
		 * Render a horizontal span. Note that the sub-pixel coverage bias has
		 * already been added into xLeft and xRight.
		 */
		
		u_pixelLeft_int = u_leftEdge_int >> (MSmSubPixelBits + MSmFractBits);
		u_pixelRight_int = u_rightEdge_int >> (MSmSubPixelBits + MSmFractBits);
		
		if (spanDirection_int > 0)
		{
			/*
			 * We are rasterizing from left to right. Compute the pointers to the first
			 * pixel in the span.
			 */
				
				image_ptr = imageScanlineLeft_ptr + u_pixelLeft_int;
				
				ptr_ddu_int = 1;
				
		}
		else
		{
			/*
			 * We are rasterizing from right to left.
			 */
				
				image_ptr = imageScanlineLeft_ptr + u_pixelRight_int;
				
				ptr_ddu_int = -1;
				

		}
		
		r_temp_float = r_min_longEdge_float;
		g_temp_float = g_min_longEdge_float;
		b_temp_float = b_min_longEdge_float;
		
		/*
		 * Horizontal interpolation loop.
		 */
		if(scanline_int >= 0)
		{
			for (i = 1 + u_pixelRight_int - u_pixelLeft_int; i-- > 0; /* Nothing */)
			{
				if(image_ptr >= imageScanlineLeft_ptr && image_ptr < (UUtUns32*)((char *)imageScanlineLeft_ptr + imageRowBytes_uns))
				{
					*image_ptr = 
						((UUtUns8)r_temp_float << 16) |
						((UUtUns8)g_temp_float << 8) |
						((UUtUns8)b_temp_float << 0);

					triVisible = UUcTrue;
				}
				
				image_ptr += ptr_ddu_int;
				
				r_temp_float += r_mid_ddu_float;
				g_temp_float += g_mid_ddu_float;
				b_temp_float += b_mid_ddu_float;
			}
		}

		/*
		 * Advance to next scanline
		 */
		
		u_leftEdge_int += u_leftEdge_ddv_int;
		u_rightEdge_int += u_rightEdge_ddv_int;
		
		imageScanlineLeft_ptr = (UUtUns32 *)((char *)imageScanlineLeft_ptr + imageRowBytes_uns);
	
		r_min_longEdge_float += r_max_ddv_float;
		g_min_longEdge_float += g_max_ddv_float;
		b_min_longEdge_float += b_max_ddv_float;
	}
	
	//if (renderLightmap) UUmAssert(triVisible == UUcTrue);
}


