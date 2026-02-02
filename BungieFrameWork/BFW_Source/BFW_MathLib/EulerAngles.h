/**** EulerAngles.h - Support for 24 angle schemes ****/
/* Ken Shoemake, 1993 */
#ifndef _H_EulerAngles
#define _H_EulerAngles

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_MathLib.h"

/*** Order type constants, constructors, extractors
There are 24 possible conventions, designated by:

	  o EulAxI = axis used initially
 	  o EulPar = parity of axis permutation
	  o EulRep = repetition of initial axis as last
	  o EulFrm = frame from which axes are taken

Axes I,J,K will be a permutation of X,Y,Z.
Axis H will be either I or K, depending on EulRep.
Frame S takes axes from initial static frame.
If ord = (AxI=X, Par=Even, Rep=No, Frm=S), then
{a,b,c,ord} means Rz(c)Ry(b)Rx(a), where Rz(c)v
rotates v around Z by c radians.
****************************************************/

#define EulFrmS	     0
#define EulFrmR	     1
#define EulFrm(ord)  ((unsigned)(ord)&1)
#define EulRepNo     0
#define EulRepYes    1
#define EulRep(ord)  (((unsigned)(ord)>>1)&1)
#define EulParEven   0
#define EulParOdd    1
#define EulPar(ord)  (((unsigned)(ord)>>2)&1)
#define EulSafe	     "\000\001\002\000"
#define EulNext	     "\001\002\000\001"
#define EulAxI(ord)  ((int)(EulSafe[(((unsigned)(ord)>>3)&3)]))
#define EulAxJ(ord)  ((int)(EulNext[EulAxI(ord)+(EulPar(ord)==EulParOdd)]))
#define EulAxK(ord)  ((int)(EulNext[EulAxI(ord)+(EulPar(ord)!=EulParOdd)]))
#define EulAxH(ord)  ((EulRep(ord)==EulRepNo)?EulAxK(ord):EulAxI(ord))
    /* EulGetOrd unpacks all useful information about order simultaneously. */
#define iEuler_GetOrder(ord,i,j,k,h,n,s,f) {unsigned o=ord;f=o&1;o>>=1;s=o&1;o>>=1;\
    n=o&1;o>>=1;i=EulSafe[o&3];j=EulNext[i+n];k=EulNext[i+1-n];h=s?k:i;}
    /* MUmEulerOrder creates an order value between 0 and 23 from 4-tuple choices. */
#define MUmEulerOrder(i,p,r,f)	   (((((((i)<<1)+(p))<<1)+(r))<<1)+(f))
    /* Static axes */
#define MUcEulerOrderXYZs    MUmEulerOrder(0,EulParEven,EulRepNo,EulFrmS)
#define MUcEulerOrderXYXs    MUmEulerOrder(0,EulParEven,EulRepYes,EulFrmS)
#define MUcEulerOrderXZYs    MUmEulerOrder(0,EulParOdd,EulRepNo,EulFrmS)
#define MUcEulerOrderXZXs    MUmEulerOrder(0,EulParOdd,EulRepYes,EulFrmS)
#define MUcEulerOrderYZXs    MUmEulerOrder(1,EulParEven,EulRepNo,EulFrmS)
#define MUcEulerOrderYZYs    MUmEulerOrder(1,EulParEven,EulRepYes,EulFrmS)
#define MUcEulerOrderYXZs    MUmEulerOrder(1,EulParOdd,EulRepNo,EulFrmS)
#define MUcEulerOrderYXYs    MUmEulerOrder(1,EulParOdd,EulRepYes,EulFrmS)
#define MUcEulerOrderZXYs    MUmEulerOrder(2,EulParEven,EulRepNo,EulFrmS)
#define MUcEulerOrderZXZs    MUmEulerOrder(2,EulParEven,EulRepYes,EulFrmS)
#define MUcEulerOrderZYXs    MUmEulerOrder(2,EulParOdd,EulRepNo,EulFrmS)
#define MUcEulerOrderZYZs    MUmEulerOrder(2,EulParOdd,EulRepYes,EulFrmS)
    /* Rotating axes */
#define MUcEulerOrderZYXr    MUmEulerOrder(0,EulParEven,EulRepNo,EulFrmR)
#define MUcEulerOrderXYXr    MUmEulerOrder(0,EulParEven,EulRepYes,EulFrmR)
#define MUcEulerOrderYZXr    MUmEulerOrder(0,EulParOdd,EulRepNo,EulFrmR)
#define MUcEulerOrderXZXr    MUmEulerOrder(0,EulParOdd,EulRepYes,EulFrmR)
#define MUcEulerOrderXZYr    MUmEulerOrder(1,EulParEven,EulRepNo,EulFrmR)
#define MUcEulerOrderYZYr    MUmEulerOrder(1,EulParEven,EulRepYes,EulFrmR)
#define MUcEulerOrderZXYr    MUmEulerOrder(1,EulParOdd,EulRepNo,EulFrmR)
#define MUcEulerOrderYXYr    MUmEulerOrder(1,EulParOdd,EulRepYes,EulFrmR)
#define MUcEulerOrderYXZr    MUmEulerOrder(2,EulParEven,EulRepNo,EulFrmR)
#define MUcEulerOrderZXZr    MUmEulerOrder(2,EulParEven,EulRepYes,EulFrmR)
#define MUcEulerOrderXYZr    MUmEulerOrder(2,EulParOdd,EulRepNo,EulFrmR)
#define MUcEulerOrderZYZr    MUmEulerOrder(2,EulParOdd,EulRepYes,EulFrmR)

#endif
