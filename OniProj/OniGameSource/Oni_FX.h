/*	FILE:	Oni_FX.h
	
	AUTHOR:	Jason Duncan, Michael Evans
	
	CREATED: April 2, 1999
	
	PURPOSE: control of FX in ONI
	
	Copyright (c) 1999, 2000

*/

#ifndef _ONI_FX_H_
#define _ONI_FX_H_

UUtError FXrEffects_Initialize(void);
UUtError FXrEffects_LevelBegin(void);

void FXrDrawDot(M3tPoint3D *inPoint);
void FXrDrawLaser( M3tPoint3D *inFrom, M3tPoint3D *inTo, UUtUns32 inColor );
void FXrDrawCursorTunnel( M3tPoint3D *inFrom, M3tPoint3D *inTo, UUtUns32 inColor, M3tTextureMap *inTexture );
void FXrDrawCone( M3tPoint3D *inFrom, M3tPoint3D *inTo, UUtUns32 inColor, M3tTextureMap *inTexture );
void FXrDrawSingleCursor(M3tPoint3D *inFrom, M3tPoint3D *inTo, UUtUns32 inColor, M3tTextureMap *inTexture);

#endif
