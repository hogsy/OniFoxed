// ======================================================================
// DM_Devices.h
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include <d3d.h>

// ======================================================================
// defines
// ======================================================================
#define RELEASE(p)	if (p) (p)->lpVtbl->Release(p);

// ======================================================================
// typedefs
// ======================================================================
typedef struct DMtDD_SurfaceInfo		DMtDD_SurfaceInfo;
typedef struct DMtD3D_DeviceInfo		DMtD3D_DeviceInfo;
typedef struct DMtDD_DriverInfo			DMtDD_DriverInfo;

// ----------------------------------------------------------------------
typedef struct DMtModeDescription
{
	UUtBool							use_Direct3D;
	UUtBool							use_hardware;
	UUtBool							use_zbuffer;
	UUtUns16						width;
	UUtUns16						height;
	UUtUns16						bitdepth;
	UUtUns16						zbuffer_bitdepth;
} DMtModeDescription;
// ======================================================================
// prototypes
// ======================================================================
UUtError
DMrDRVRMGR_Initialize(
	DMtModeDescription				*inModeDescription);

void
DMrDRVRMGR_Terminate(
	void);

// ----------------------------------------------------------------------
UUtError
DMrDRVRMGR_FindDriver(
	M3tDrawContextDescriptor		*inDrawContextDescriptor,
	UUtBool							inDirect3D);

DMtDD_DriverInfo*
DMrDRVRMGR_GetCurrentDDDriver(
	void);

DMtD3D_DeviceInfo*
DMrDRVRMGR_GetCurrentD3DDevice(
	void);

UUtError
DMrD3D_DI_LoadFormats(
	DMtD3D_DeviceInfo				*inDeviceInfo,
	LPDIRECT3DDEVICE2				inD3DDevice2);

// ----------------------------------------------------------------------
LPGUID
DMrDD_DI_GetGUID(
	DMtDD_DriverInfo				*inDriverInfo);

UUtError
DMrDD_DI_FindD3DDevice(
	DMtDD_DriverInfo				*inDriverInfo,
	M3tDrawContextDescriptor		*inDrawContextDescriptor);

// ----------------------------------------------------------------------
LPGUID
DMrD3D_DI_GetGUID(
	DMtD3D_DeviceInfo				*inDeviceInfo);

UUtBool
DMrD3D_DI_isHardware(
	DMtD3D_DeviceInfo				*inDeviceInfo);

LPD3DDEVICEDESC
DMrD3D_DI_GetHALDesc(
	DMtD3D_DeviceInfo				*inDeviceInfo);

LPD3DDEVICEDESC
DMrD3D_DI_GetHELDesc(
	DMtD3D_DeviceInfo				*inDeviceInfo);

DMtDD_SurfaceInfo*
DMrD3D_DI_FindFormat(
	DMtD3D_DeviceInfo				*inDeviceInfo,
	LPDDPIXELFORMAT					inDDPixelFormat);

// ----------------------------------------------------------------------
UUtInt32
DMrFlagsToBitDepth(
	UUtUns32					inFlags);

char*
DMrGetErrorMsg(
	HRESULT error);
