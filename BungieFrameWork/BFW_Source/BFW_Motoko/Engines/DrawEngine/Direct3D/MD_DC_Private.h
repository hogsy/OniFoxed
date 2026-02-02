/*
	FILE:	MD_DrawContext_Private.h

	AUTHOR:	Brent H. Pease

	CREATED: Sept 17, 1997

	PURPOSE:

	Copyright 1997

*/
// ======================================================================
// includes
// ======================================================================
#ifndef MD_DRAWCONTEXT_PRIVATE_H
#define MD_DRAWCONTEXT_PRIVATE_H

#include "BFW.h"
#include "BFW_Motoko.h"

#include <D3D.h>

// ======================================================================
// typedefs
// ======================================================================

// ----------------------------------------------------------------------
typedef struct MDtDrawContextPrivate
{
	UUtInt32					width;					// height of front buffer
	UUtInt32					height;					// width of front buffer
	UUtInt32					bpp;					// bits per pixel
	UUtInt32					zbpp;					// bits per pixel of zBuffer

	UUtBool						isHardware;				// d3d_device is a hardware device

	M3tDrawContextType			contextType;
	M3tDrawContextScreenFlags	screenFlags;

	// DirectX data
	LPDIRECTDRAW				dd;						// DirectDraw device
	LPDIRECTDRAW2				dd2;					// DirectDraw 2 device
	LPDIRECT3D2					d3d2;					// Direct3D2
	LPDIRECT3DDEVICE2			d3d_device2;			// Direct3D Device2
	LPDIRECT3DVIEWPORT2			d3d_viewport2;			// Direct3D Viewport2

	// Draw data
	LPDIRECTDRAWSURFACE			frontBuffer;			// buffer displayed on the screen
	LPDIRECTDRAWSURFACE			backBuffer;				// buffer to draw into
	LPDIRECTDRAWSURFACE			zBuffer;				// zBuffer is attached to backBuffer
	RECT						rect;
	DWORD						top;
	DWORD						left;

	// Array data
	void*						statePtr[M3cDrawStatePtrType_NumTypes];
	UUtInt32					stateInt[M3cDrawStateIntType_NumTypes];

} MDtDrawContextPrivate;

// ----------------------------------------------------------------------
enum
{
	MDcTextureChanged			= 0
};

typedef struct MDtTextureMapPrivate
{
	UUtUns32					flags;

//	LPDIRECT3DTEXTURE2			texture;
	D3DTEXTUREHANDLE			texture_handle;
	LPDIRECTDRAWSURFACE3		memory_surface;
	LPDIRECTDRAWSURFACE3		device_surface;

} MDtTextureMapPrivate;

// ======================================================================
// prototypes
// ======================================================================
void
MDrUseTexture(
	MDtDrawContextPrivate		*inDrawContextPrivate,
	M3tTextureMap				*inTextureMap);

UUtBool
MDrTextureFormatAvailable(
	LPDDPIXELFORMAT				inDDPixelFormat);


// ======================================================================
#endif /* MD_DRAWCONTEXT_PRIVATE_H */
