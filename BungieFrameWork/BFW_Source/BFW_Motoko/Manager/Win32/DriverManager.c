// ======================================================================
// DM_Devices.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "DriverManager.h"

#include <DDraw.h>
#include <D3D.h>

// ======================================================================
// defines
// ======================================================================

#define DMcMaxNameLength					31
#define DMcMaxDescLength					63


// DRVRMGR Flags
#define DMcDRVRMGR_Initialized				0x00000001

// DD Driver Flags
#define DMcDDDriver_Valid					0x00000001
#define DMcDDDriver_Primary					0x00000002
#define DMcDDDriver_D3D						0x00000004

#define DMcDDDriver_Modes_Loaded			0x00000008
#define DMcDDDriver_Devices_Loaded			0x00000010

// D3D Device Flags
#define DMcD3DDevice_Valid					0x00000001
#define DMcD3DDevice_Formats_Loaded			0x00000002


// ======================================================================
// typedefs
// ======================================================================
// ----------------------------------------------------------------------
typedef struct DMtDD_CallbackInfo
{
	UUtBool					result;				// success or failure
	UUtInt32				count;				// current number of drivers
	void					*data;				// current driver/device/etc.
	DMtModeDescription		*mode;				// mode description
} DMtDD_CallbackInfo;

// ----------------------------------------------------------------------
struct DMtDD_SurfaceInfo
{
	// mode info
	DDSURFACEDESC				surface_desc;	// Complete surface descriptrion

	// list info
	struct DMtDD_SurfaceInfo	*next;			// next surface info in list

};

// ----------------------------------------------------------------------
struct DMtD3D_DeviceInfo
{
	// D3D driver info
	UUtUns32				flags;				// info flags

	// D3D info
	GUID					guid;				// GUID
	char					name[DMcMaxNameLength+1];	// name of driver
	char					desc[DMcMaxDescLength+1];	// description of driver
	D3DDEVICEDESC			HALdesc;			// HAL description
	D3DDEVICEDESC			HELdesc;			// HEL description

	// texture formats
	UUtInt32					num_formats;	// number of texture formats
	struct DMtDD_SurfaceInfo	*format_list;	// list of texture formats

	// list info
	struct DMtD3D_DeviceInfo	*next;			// next device info in list

};

// ----------------------------------------------------------------------
struct DMtDD_DriverInfo
{
	// structure info
	UUtUns32				flags;				// info flags

	// driver info
	GUID					guid;				// guid for the device
	char					name[DMcMaxNameLength+1];	// name of driver
	char					desc[DMcMaxDescLength+1];	// description of driver

	// driver caps
	DDCAPS					HALcaps;			// HAL caps
	DDCAPS					HELcaps;			// HEL caps

	// mode info
	UUtInt32					num_modes;			// number of modes
	struct DMtDD_SurfaceInfo	*mode_list;			// list of modes

	// D3D info
	UUtInt32					num_d3d_devices;	// number of devices
	struct DMtD3D_DeviceInfo	*d3d_device_list;	// list of d3d devices

	// list info
	struct DMtDD_DriverInfo		*next;				// next driver info in list

};

// ----------------------------------------------------------------------
typedef struct DMtDRVRMGR
{
	// data
	UUtUns32					flags;					// info flags

	// DD driver info
	UUtInt32					num_dd_drivers;			// number of DD drivers
	struct DMtDD_DriverInfo		*dd_driver_list;		// list of DD drivers

	// configuration info
	struct DMtDD_DriverInfo		*current_dd_driver;		// DD driver being used
	struct DMtDD_SurfaceInfo	*current_mode;			// current mode in DD driver
	struct DMtD3D_DeviceInfo	*current_d3d_device;	// current d3d device
	struct DMtDD_SurfaceInfo	*current_texture_format;// current texture format

} DMtDRVRMGR;

// ======================================================================
// globals
// ======================================================================
static DMtDRVRMGR		DMgDRVRMGR;
static UUtBool			DMgUseDirect3D;

// ======================================================================
// prototypes
// ======================================================================
static UUtBool
DMiDRVRMGR_isInitialized(
	DMtDRVRMGR					*inDriverManager);

static UUtBool
DMiDRVRMGR_isInitialized(
	DMtDRVRMGR					*inDriverManager);

static void
DMiDRVRMGR_initOn(
	DMtDRVRMGR					*inDriverManager);

static void
DMiDRVRMGR_initOff(
	DMtDRVRMGR					*inDriverManager);

static UUtError
DMiDRVRMGR_LoadDrivers(
	DMtDRVRMGR					*inDriverManager,
	DMtModeDescription			*inModeDescription);

static void
DMiDRVRMGR_UnloadDrivers(
	DMtDRVRMGR					*inDriverManager);

static UUtError
DMiDRVRMGR_AddDriver(
	DMtDRVRMGR					*inDriverManager,
	DMtDD_DriverInfo			*inDriverInfo);

// ----------------------------------------------------------------------
static DMtDD_DriverInfo*
DMiDD_DI_New(
	LPGUID						inGuid,
	LPSTR						inName,
	LPSTR						inDescription,
	DMtModeDescription			*inModeDescription);

static void
DMiDD_DI_Delete(
	DMtDD_DriverInfo			*inDriverInfo);

static UUtError
DMiDD_DI_LoadModes(
	DMtDD_DriverInfo			*inDriverInfo,
	LPDIRECTDRAW				inDirectDraw,
	DMtModeDescription			*inModeDescription);

static void
DMiDD_DI_UnloadModes(
	DMtDD_DriverInfo			*inDriverInfo);

static UUtError
DMiDD_DI_AddMode(
	DMtDD_DriverInfo			*inDriverInfo,
	DMtDD_SurfaceInfo			*inModeInfo);

static UUtError
DMiDD_DI_LoadDevices(
	DMtDD_DriverInfo			*inDriverInfo,
	LPDIRECT3D2					inDirect3D2);

static void
DMiDD_DI_UnloadDevices(
	DMtDD_DriverInfo			*inDriverInfo);

static UUtError
DMiDD_DI_AddDevice(
	DMtDD_DriverInfo			*inDriverInfo,
	DMtD3D_DeviceInfo			*inD3DDeviceInfo);

static UUtBool
DMiDD_DI_isValid(
	DMtDD_DriverInfo			*inDriverInfo);

static void
DMiDD_DI_validOn(
	DMtDD_DriverInfo			*inDriverInfo);

static void
DMiDD_DI_validOff(
	DMtDD_DriverInfo			*inDriverInfo);

static UUtBool
DMiDD_DI_isPrimary(
	DMtDD_DriverInfo			*inDriverInfo);

static void
DMiDD_DI_primaryOn(
	DMtDD_DriverInfo			*inDriverInfo);

static void
DMiDD_DI_primaryOff(
	DMtDD_DriverInfo			*inDriverInfo);

static UUtBool
DMiDD_DI_devicesLoaded(
	DMtDD_DriverInfo			*inDriverInfo);

static void
DMiDD_DI_devicesLoadedOn(
	DMtDD_DriverInfo			*inDriverInfo);

static void
DMiDD_DI_devicesLoadedOff(
	DMtDD_DriverInfo			*inDriverInfo);

static UUtBool
DMiDD_DI_modesLoaded(
	DMtDD_DriverInfo			*inDriverInfo);

static void
DMiDD_DI_modesLoadedOn(
	DMtDD_DriverInfo			*inDriverInfo);

static void
DMiDD_DI_modesLoadedOff(
	DMtDD_DriverInfo			*inDriverInfo);

// ----------------------------------------------------------------------
static DMtDD_SurfaceInfo*
DMiDD_SI_New(
	LPDDSURFACEDESC				inSurfaceDesc,
	DMtModeDescription			*inModeDescription);

static void
DMiDD_SI_Delete(
	DMtDD_SurfaceInfo			*inSurfaceInfo);

static UUtBool
DMiDD_SI_MatchPixelFormat(
	DMtDD_SurfaceInfo			*inSurfaceInfo,
	LPDDPIXELFORMAT				inDDPixelFormat);

// ----------------------------------------------------------------------
static DMtD3D_DeviceInfo*
DMiD3D_DI_New(
	LPGUID						inGuid,
	LPSTR						inName,
	LPSTR						inDescription,
	LPD3DDEVICEDESC				inHALDesc,
	LPD3DDEVICEDESC				inHELDesc);

static void
DMiD3D_DI_Delete(
	DMtD3D_DeviceInfo			*inDeviceInfo);

static void
DMiD3D_DI_UnloadFormats(
	DMtD3D_DeviceInfo			*inDeviceInfo);

static UUtError
DMiD3D_DI_AddFormat(
	DMtD3D_DeviceInfo			*inDeviceInfo,
	DMtDD_SurfaceInfo			*inFormatInfo);

static UUtBool
DMiD3D_DI_isValid(
	DMtD3D_DeviceInfo			*inDeviceInfo);

static void
DMiD3D_DI_validOn(
	DMtD3D_DeviceInfo			*inDeviceInfo);

static void
DMiD3D_DI_validOff(
	DMtD3D_DeviceInfo			*inDeviceInfo);

static UUtBool
DMiD3D_DI_formatsLoaded(
	DMtD3D_DeviceInfo			*inDeviceInfo);

static void
DMiD3D_DI_formatsLoadedOn(
	DMtD3D_DeviceInfo			*inDeviceInfo);

static void
DMiD3D_DI_formatsLoadedOff(
	DMtD3D_DeviceInfo			*inDeviceInfo);

// ----------------------------------------------------------------------
static BOOL PASCAL
DMiDD_DriverEnumCallback(
	GUID						*inGuid,
	LPSTR						inDescription,
	LPSTR						inName,
	LPVOID						inData);

static BOOL PASCAL
DDiDD_ModeEnumCallback(
	LPDDSURFACEDESC				inSurfaceDesc,
	LPVOID						inData);

static HRESULT PASCAL
DMiD3D_DeviceEnumCallback(
	LPGUID						inGuid,
	LPSTR						inName,
	LPSTR						inDescription,
	LPD3DDEVICEDESC				inHALDesc,
	LPD3DDEVICEDESC				inHELDesc,
	LPVOID						inData);

static HRESULT PASCAL
DMiD3D_TextureFormatEnumCallback(
	LPDDSURFACEDESC				inTextureFormat,
	LPVOID						inData);

// ======================================================================
// functions
// ======================================================================

// ----------------------------------------------------------------------
UUtError
DMrDRVRMGR_Initialize(
	DMtModeDescription			*inModeDescription)
{
	UUtError		error;

	// if the driver manager has already been initialized, terminate it
	// and then initialize it
	if (DMiDRVRMGR_isInitialized(&DMgDRVRMGR))
	{
		DMrDRVRMGR_Terminate();
	}

	// it isn't so clear the data
	UUrMemory_Clear(&DMgDRVRMGR, sizeof(DMtDRVRMGR));

	// build the driver list
	error = DMiDRVRMGR_LoadDrivers(&DMgDRVRMGR, inModeDescription);
	UUmError_ReturnOnErrorMsg(error, "Unable to load DD drivers.");

	DMiDRVRMGR_initOn(&DMgDRVRMGR);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
DMrDRVRMGR_Terminate(
	void)
{
	// make sure that the driver manager is initialized
	if (DMiDRVRMGR_isInitialized(&DMgDRVRMGR))
	{
		// delete all the data associated with the drivers
		DMiDRVRMGR_UnloadDrivers(&DMgDRVRMGR);

		// DRVRMGR is no longer initialized
		DMiDRVRMGR_initOff(&DMgDRVRMGR);
	}
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
DMrDRVRMGR_FindDriver(
	M3tDrawContextDescriptor		*inDrawContextDescriptor,
	UUtBool							inDirect3D)
{
#if 0
	DMtDD_DriverInfo				*curr;
	UUtError						error;

	// Make sure the DRVRMGR has been initialized
	if (!DMiDRVRMGR_isInitialized(&DMgDRVRMGR))
	{
		UUrError_Report(UUcError_Generic, "The Driver manager is not initialized.");
		return UUcError_Generic;
	}

	// find the driver with the correct name
	for (curr = DMgDRVRMGR.dd_driver_list; curr != NULL; curr = curr->next)
	{
		if (strcmp(inDrawContextDescriptor->drawContext.onScreen.driver_name,
				curr->name) == 0)
		{
			break;
		}
	}

	if (curr == NULL)
	{
		UUrError_Report(UUcError_Generic, "Unable to find driver.");
		return UUcError_Generic;
	}

	// set the driver
	DMgDRVRMGR.current_dd_driver = curr;

	// Find the d3d device
	if (inDirect3D)
	{
		error = DMrDD_DI_FindD3DDevice(curr, inDrawContextDescriptor);
		UUmError_ReturnOnErrorMsg(error, "Unable to find a d3d device.");
	}
#endif

	return UUcError_None;
}

// ----------------------------------------------------------------------
DMtDD_DriverInfo*
DMrDRVRMGR_GetCurrentDDDriver(
	void)
{
	return DMgDRVRMGR.current_dd_driver;
}

// ----------------------------------------------------------------------
DMtD3D_DeviceInfo*
DMrDRVRMGR_GetCurrentD3DDevice(
	void)
{
	return DMgDRVRMGR.current_d3d_device;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
DMiDRVRMGR_LoadDrivers(
	DMtDRVRMGR					*inDriverManager,
	DMtModeDescription			*inModeDescription)
{
	DMtDD_CallbackInfo			cb_info;
	HRESULT						result;

	// initialize all valid drivers in system
	cb_info.result = UUcTrue;
	cb_info.count = 0;
	cb_info.data = (void*)inDriverManager;
	cb_info.mode = inModeDescription;

	// enumerate the drivers
	result = DirectDrawEnumerate(DMiDD_DriverEnumCallback, &cb_info);
	if (result != DD_OK)
	{
		UUrError_Report(
			UUcError_DirectDraw,
			DMrGetErrorMsg(result));
		return UUcError_DirectDraw;
	}

	// double check the count
	if (	(!cb_info.result) ||
			(cb_info.count == 0) ||
			(cb_info.count != inDriverManager->num_dd_drivers))
	{
		return UUcError_Generic;
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
DMiDRVRMGR_UnloadDrivers(
	DMtDRVRMGR					*inDriverManager)
{
	DMtDD_DriverInfo			*curr;

	// walk the driver list and destroy all D3D devices
	curr = inDriverManager->dd_driver_list;
	while (curr)
	{
		// remove curr from the list
		inDriverManager->dd_driver_list = curr->next;

		// destroy curr
		DMiDD_DI_Delete(curr);

		// move on to next driver in the list
		curr = inDriverManager->dd_driver_list;
	}
}

// ----------------------------------------------------------------------
static UUtError
DMiDRVRMGR_AddDriver(
	DMtDRVRMGR					*inDriverManager,
	DMtDD_DriverInfo			*inDriverInfo)
{
	// make sure inDriverInfo is valid
	if ((inDriverManager == NULL) || (inDriverInfo == NULL))
	{
		UUrError_Report(UUcError_Generic, "Parameter error.");
		return UUcError_Generic;
	}

	// Add new mode to end of list
	if (inDriverManager->dd_driver_list == NULL)
	{
		inDriverManager->dd_driver_list = inDriverInfo;
	}
	else
	{
		DMtDD_DriverInfo		*curr;

		// find the end of the list
		curr = inDriverManager->dd_driver_list;
		while (curr->next)
		{
			curr = curr->next;
		}

		// insert inDriverInfo at the end of the list
		curr->next = inDriverInfo;

		// make sure list ends
		inDriverInfo->next = NULL;
	}

	// update count
	inDriverManager->num_dd_drivers++;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtBool
DMiDRVRMGR_isInitialized(
	DMtDRVRMGR					*inDriverManager)
{
	return (inDriverManager->flags & DMcDRVRMGR_Initialized);
}

// ----------------------------------------------------------------------
static void
DMiDRVRMGR_initOn(
	DMtDRVRMGR					*inDriverManager)
{
	inDriverManager->flags |= DMcDRVRMGR_Initialized;
}

// ----------------------------------------------------------------------
static void
DMiDRVRMGR_initOff(
	DMtDRVRMGR					*inDriverManager)
{
	inDriverManager->flags &= ~DMcDRVRMGR_Initialized;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
LPGUID
DMrDD_DI_GetGUID(
	DMtDD_DriverInfo				*inDriverInfo)
{
	return &inDriverInfo->guid;
}

// ----------------------------------------------------------------------
UUtError
DMrDD_DI_FindD3DDevice(
	DMtDD_DriverInfo				*inDriverInfo,
	M3tDrawContextDescriptor		*inDrawContextDescriptor)
{
#if 0
	DMtD3D_DeviceInfo				*curr;

	// find the device with the correct name
	for (	curr = inDriverInfo->d3d_device_list;
			curr != NULL;
			curr = curr->next)
	{
		if (strcmp(inDrawContextDescriptor->drawContext.onScreen.device_name,
				curr->name) == 0)
		{
			break;
		}
	}

	if (curr == NULL)
	{
		UUrError_Report(UUcError_Generic, "Unable to find the device.");
		return UUcError_Generic;
	}

	// set the device
	DMgDRVRMGR.current_d3d_device = curr;
#endif

	return UUcError_None;
}

// ----------------------------------------------------------------------
static DMtDD_DriverInfo*
DMiDD_DI_New(
	LPGUID						inGuid,
	LPSTR						inName,
	LPSTR						inDescription,
	DMtModeDescription			*inModeDescription)
{
	DMtDD_DriverInfo			*driverInfo;
	LPDIRECTDRAW				dd;
	LPDIRECT3D2					d3d2;
	HRESULT						result;
	UUtError					error;
	char						*errorMsg;

	dd = NULL;
	d3d2 = NULL;
	result = DD_OK;
	error = UUcError_None;
	errorMsg = "\0";

	// allocate memory for the structure
	driverInfo = (DMtDD_DriverInfo*)UUrMemory_Block_New(sizeof(DMtDD_DriverInfo));
	if (driverInfo == NULL)
	{
		UUrError_Report(UUcError_OutOfMemory, "Unable to allocate Driver Info.\0");
		return NULL;
	}
	UUrMemory_Clear(driverInfo, sizeof(DMtDD_DriverInfo));

	// copy GUID
	if (!inGuid)
	{
		DMiDD_DI_primaryOn(driverInfo);
	}
	else
	{
		driverInfo->guid = *inGuid;
	}

	// copy the name and description
	strncpy(driverInfo->name, inName, DMcMaxNameLength);
	if (inDescription)
	{
		strncpy(driverInfo->desc, inDescription, DMcMaxDescLength);
	}
	else
	{
		strcpy(driverInfo->desc, "Unknown\0");
	}

	// create the DirectDraw object
	result = DirectDrawCreate(inGuid, &dd, NULL);
	if (result != DD_OK)
	{
		error = UUcError_DirectDraw;
		UUrError_Report(
			UUcError_DirectDraw,
			DMrGetErrorMsg(result));
		goto cleanup;
	}

	// get the driver caps
	driverInfo->HALcaps.dwSize = sizeof(DDCAPS);
	driverInfo->HELcaps.dwSize = sizeof(DDCAPS);

	result =
		IDirectDraw_GetCaps(
			dd,
			&driverInfo->HALcaps,
			&driverInfo->HELcaps);
	if (result != DD_OK)
	{
		error = UUcError_DirectDraw;
		UUrError_Report(
			UUcError_DirectDraw,
			DMrGetErrorMsg(result));
		goto cleanup;
	}

	// enumerate all modes for this DD driver
	driverInfo->num_modes = 0;
	DMiDD_DI_modesLoadedOff(driverInfo);

	error = DMiDD_DI_LoadModes(driverInfo, dd, inModeDescription);
	if (error != UUcError_None)
	{
		errorMsg = "Unable to load DirectDraw modes.\0";
		UUrError_Report(error, errorMsg);
		goto cleanup;
	}

	// mark as valid driver
	DMiDD_DI_validOn(driverInfo);

	// --------------------------------------------------
	// get the Direct3D interface, if it is available
	// --------------------------------------------------
	if (inModeDescription->use_Direct3D)
	{
		result = IDirectDraw_QueryInterface(dd, &IID_IDirect3D2, &d3d2);
		if (result != DD_OK)
		{
			error = UUcError_DirectDraw;
			UUrError_Report(
				UUcError_DirectDraw,
				DMrGetErrorMsg(result));
			goto cleanup;
		}

		// enumerate all D3D devices for the DD driver
		driverInfo->num_d3d_devices = 0;
		DMiDD_DI_devicesLoadedOff(driverInfo);

		error = DMiDD_DI_LoadDevices(driverInfo, d3d2);
		if (error != UUcError_None)
		{
			errorMsg = "Unable to load Direct3D devices.\0";
			UUrError_Report(error, errorMsg);
			goto cleanup;
		}
	}
	// --------------------------------------------------
cleanup:
	// cleanup before leaving
	if (d3d2)
	{
		IDirect3D2_Release(d3d2);
		d3d2 = NULL;
	}

	if (dd)
	{
		IDirectDraw_Release(dd);
		dd = NULL;
	}

	if (error != UUcError_None)
	{
		UUrError_Report(error, errorMsg);
	}

	return driverInfo;
}

// ----------------------------------------------------------------------
static void
DMiDD_DI_Delete(
	DMtDD_DriverInfo			*inDriverInfo)
{
	// destroy all the devices
	DMiDD_DI_UnloadDevices(inDriverInfo);

	// destroy all the modes
	DMiDD_DI_UnloadModes(inDriverInfo);

	// delete the structure
	UUrMemory_Block_Delete(inDriverInfo);
}

// ----------------------------------------------------------------------
static UUtError
DMiDD_DI_LoadModes(
	DMtDD_DriverInfo			*inDriverInfo,
	LPDIRECTDRAW				inDirectDraw,
	DMtModeDescription			*inModeDescription)
{
	// check the parameters
	if ((inDriverInfo == NULL) || (inDirectDraw == NULL))
	{
		UUrError_Report(UUcError_Generic, "Parameter Error.");
		return UUcError_Generic;
	}

	// Have the modes already been loaded?
	if (!DMiDD_DI_modesLoaded(inDriverInfo))
	{
		DMtDD_CallbackInfo		cb_info;
		HRESULT					result;

		// enumerte all modes for this driver
		cb_info.result = UUcTrue;
		cb_info.count = 0;
		cb_info.data = (void*)inDriverInfo;
		cb_info.mode = inModeDescription;

		result =
			IDirectDraw_EnumDisplayModes(
				inDirectDraw,
				0,
				NULL,
				&cb_info,
				DDiDD_ModeEnumCallback);
		if (result != DD_OK)
		{
			UUrError_Report(
				UUcError_DirectDraw,
				DMrGetErrorMsg(result));
			return UUcError_DirectDraw;
		}

		// double check the count
		if (	(!cb_info.result) ||
				(cb_info.count == 0) ||
				(cb_info.count != inDriverInfo->num_modes))
		{
			return UUcError_Generic;
		}

		// mark modes as loaded
		DMiDD_DI_modesLoadedOn(inDriverInfo);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
DMiDD_DI_UnloadModes(
	DMtDD_DriverInfo			*inDriverInfo)
{
	DMtDD_SurfaceInfo			*mode_info;

	mode_info = inDriverInfo->mode_list;

	// walk the linked list and destroy all d3d device nodes
	while (mode_info)
	{
		// remove d3d_device_info from the list
		inDriverInfo->mode_list = mode_info->next;

		// delete the d3d_device_info structure
		DMiDD_SI_Delete(mode_info);

		// move to the next node
		mode_info = inDriverInfo->mode_list;
	}
}

// ----------------------------------------------------------------------
static UUtError
DMiDD_DI_AddMode(
	DMtDD_DriverInfo			*inDriverInfo,
	DMtDD_SurfaceInfo			*inModeInfo)
{
	// make sure the parameters are valid
	if ((inDriverInfo == NULL) || (inModeInfo == NULL))
	{
		UUrError_Report(UUcError_Generic, "Parameter error.\0");
		return UUcError_Generic;
	}

	// Add new mode to end of list
	if (inDriverInfo->mode_list == NULL)
	{
		inDriverInfo->mode_list = inModeInfo;
	}
	else
	{
		DMtDD_SurfaceInfo		*curr;

		// find the end of the list
		curr = inDriverInfo->mode_list;
		while (curr->next)
		{
			curr = curr->next;
		}

		// insert inModeInfo at the end of the list
		curr->next = inModeInfo;

		// make sure list ends
		inModeInfo->next = NULL;
	}

	// update count
	inDriverInfo->num_modes++;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
DMiDD_DI_LoadDevices(
	DMtDD_DriverInfo			*inDriverInfo,
	LPDIRECT3D2					inDirect3D2)
{
	// check the parameters
	if ((inDriverInfo == NULL) || (inDirect3D2 == NULL))
	{
		UUrError_Report(UUcError_Generic, "Parameter error.");
		return UUcError_Generic;
	}

	// have the devices already been loaded?
	if (!DMiDD_DI_devicesLoaded(inDriverInfo))
	{
		DMtDD_CallbackInfo		cb_info;
		HRESULT					result;

		// enumerte all devices for this driver
		cb_info.result = UUcTrue;
		cb_info.count = 0;
		cb_info.data = (void*)inDriverInfo;

		result =
			IDirect3D2_EnumDevices(
				inDirect3D2,
				DMiD3D_DeviceEnumCallback,
				&cb_info);
		if (result != DD_OK)
		{
			UUrError_Report(UUcError_DirectDraw, "Direct3D Error.");
			return UUcError_DirectDraw;
		}

		// double check the count
		if (	(!cb_info.result) ||
				(cb_info.count == 0) ||
				(cb_info.count != inDriverInfo->num_d3d_devices))
		{
			return UUcError_Generic;
		}

	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
DMiDD_DI_UnloadDevices(
	DMtDD_DriverInfo			*inDriverInfo)
{
	DMtD3D_DeviceInfo			*d3d_device_info;

	d3d_device_info = inDriverInfo->d3d_device_list;

	// walk the linked list and destroy all d3d device nodes
	while (d3d_device_info)
	{
		// remove d3d_device_info from the list
		inDriverInfo->d3d_device_list = d3d_device_info->next;

		// delete the d3d_device_info structure
		DMiD3D_DI_Delete(d3d_device_info);

		// move to the next node
		d3d_device_info = inDriverInfo->d3d_device_list;
	}
}

// ----------------------------------------------------------------------
static UUtError
DMiDD_DI_AddDevice(
	DMtDD_DriverInfo			*inDriverInfo,
	DMtD3D_DeviceInfo			*inD3DDeviceInfo)
{
	// make sure the parameters are valid
	if ((inDriverInfo == NULL) || (inD3DDeviceInfo == NULL))
	{
		UUrError_Report(UUcError_Generic, "Parameter error.\0");
		return UUcError_Generic;
	}

	// Add new mode to end of list
	if (inDriverInfo->d3d_device_list == NULL)
	{
		inDriverInfo->d3d_device_list = inD3DDeviceInfo;
	}
	else
	{
		DMtD3D_DeviceInfo		*curr;

		// find the end of the list
		curr = inDriverInfo->d3d_device_list;
		while (curr->next)
		{
			curr = curr->next;
		}

		// insert inD3DDeviceInfo at the end of the list
		curr->next = inD3DDeviceInfo;

		// make sure the list ends
		inD3DDeviceInfo->next = NULL;
	}

	// update count
	inDriverInfo->num_d3d_devices++;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtBool
DMiDD_DI_isValid(
	DMtDD_DriverInfo			*inDriverInfo)
{
	return (inDriverInfo->flags & DMcDDDriver_Valid);
}

// ----------------------------------------------------------------------
static void
DMiDD_DI_validOn(
	DMtDD_DriverInfo			*inDriverInfo)
{
	inDriverInfo->flags |= DMcDDDriver_Valid;
}

// ----------------------------------------------------------------------
static void
DMiDD_DI_validOff(
	DMtDD_DriverInfo			*inDriverInfo)
{
	inDriverInfo->flags &= ~DMcDDDriver_Valid;
}

// ----------------------------------------------------------------------
static UUtBool
DMiDD_DI_isPrimary(
	DMtDD_DriverInfo			*inDriverInfo)
{
	return (inDriverInfo->flags & DMcDDDriver_Primary);
}

// ----------------------------------------------------------------------
static void
DMiDD_DI_primaryOn(
	DMtDD_DriverInfo			*inDriverInfo)
{
	inDriverInfo->flags |= DMcDDDriver_Primary;
}

// ----------------------------------------------------------------------
static void
DMiDD_DI_primaryOff(
	DMtDD_DriverInfo			*inDriverInfo)
{
	inDriverInfo->flags &= ~DMcDDDriver_Primary;
}

// ----------------------------------------------------------------------
static UUtBool
DMiDD_DI_devicesLoaded(
	DMtDD_DriverInfo			*inDriverInfo)
{
	return (inDriverInfo->flags & DMcDDDriver_Devices_Loaded);
}

// ----------------------------------------------------------------------
static void
DMiDD_DI_devicesLoadedOn(
	DMtDD_DriverInfo			*inDriverInfo)
{
	inDriverInfo->flags |= DMcDDDriver_Devices_Loaded;
}

// ----------------------------------------------------------------------
static void
DMiDD_DI_devicesLoadedOff(
	DMtDD_DriverInfo			*inDriverInfo)
{
	inDriverInfo->flags &= ~DMcDDDriver_Devices_Loaded;
}

// ----------------------------------------------------------------------
static UUtBool
DMiDD_DI_modesLoaded(
	DMtDD_DriverInfo			*inDriverInfo)
{
	return (inDriverInfo->flags & DMcDDDriver_Modes_Loaded);
}

// ----------------------------------------------------------------------
static void
DMiDD_DI_modesLoadedOn(
	DMtDD_DriverInfo			*inDriverInfo)
{
	inDriverInfo->flags |= DMcDDDriver_Modes_Loaded;
}

// ----------------------------------------------------------------------
static void
DMiDD_DI_modesLoadedOff(
	DMtDD_DriverInfo			*inDriverInfo)
{
	inDriverInfo->flags &= ~DMcDDDriver_Modes_Loaded;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static DMtDD_SurfaceInfo*
DMiDD_SI_New(
	LPDDSURFACEDESC				inSurfaceDesc,
	DMtModeDescription			*inModeDescription)
{
	DMtDD_SurfaceInfo			*surface_desc;

	if (inModeDescription)
	{
		// if the mode capabilities meet or exceed the capabilities of
		// the mode description, then this is a good surface
		if ((inModeDescription->width > inSurfaceDesc->dwWidth) ||
			(inModeDescription->height > inSurfaceDesc->dwHeight) ||
			(inModeDescription->bitdepth > inSurfaceDesc->ddpfPixelFormat.dwRGBBitCount))
		{
			return NULL;
		}

		if (inModeDescription->use_zbuffer &&
			(inModeDescription->zbuffer_bitdepth <=
				DMrFlagsToBitDepth(inSurfaceDesc->dwZBufferBitDepth)))
		{
			return NULL;
		}
	}

	// allocate memory for the surface desc
	surface_desc = UUrMemory_Block_New(sizeof(DMtDD_SurfaceInfo));
	if (surface_desc == NULL)
	{
		UUrError_Report(UUcError_OutOfMemory, "Unable to allocate DDSURFACEDESC.\0");
		return NULL;
	}
	UUrMemory_Clear(surface_desc, sizeof(DMtDD_SurfaceInfo));

	// copy the surface desc data
	surface_desc->surface_desc = *inSurfaceDesc;

	// return the pointer to the new surface description
	return surface_desc;
}

// ----------------------------------------------------------------------
static void
DMiDD_SI_Delete(
	DMtDD_SurfaceInfo			*inSurfaceInfo)
{
	UUrMemory_Block_Delete(inSurfaceInfo);
}

// ----------------------------------------------------------------------
static UUtBool
DMiDD_SI_MatchPixelFormat(
	DMtDD_SurfaceInfo			*inSurfaceInfo,
	LPDDPIXELFORMAT				inDDPixelFormat)
{
	LPDDPIXELFORMAT				ddpf;

	UUmAssert(inSurfaceInfo);
	UUmAssert(inDDPixelFormat);

	ddpf = &inSurfaceInfo->surface_desc.ddpfPixelFormat;

	// check to see that the important parts are the same
	if ((ddpf->dwFlags & DDPF_RGB) == (inDDPixelFormat->dwFlags & DDPF_RGB))
	{
		if ((ddpf->dwRGBBitCount == inDDPixelFormat->dwRGBBitCount) &&
			(ddpf->dwRBitMask == inDDPixelFormat->dwRBitMask) &&
			(ddpf->dwGBitMask == inDDPixelFormat->dwGBitMask) &&
			(ddpf->dwBBitMask == inDDPixelFormat->dwBBitMask))
		{
			if (inDDPixelFormat->dwFlags & DDPF_ALPHAPIXELS)
			{
				if ((ddpf->dwRGBAlphaBitMask == inDDPixelFormat->dwRGBAlphaBitMask))
					return UUcTrue;
			}
			else
			{
				return UUcTrue;
			}
		}
	}

	return UUcFalse;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
LPGUID
DMrD3D_DI_GetGUID(
	DMtD3D_DeviceInfo				*inDeviceInfo)
{
	return &inDeviceInfo->guid;
}

// ----------------------------------------------------------------------
UUtBool
DMrD3D_DI_isHardware(
	DMtD3D_DeviceInfo				*inDeviceInfo)
{
	// No corrct way of doing this, but if the hardware caps
	// don't specify a shading model then it probably isn't hardware
	if (inDeviceInfo->HALdesc.dcmColorModel)
		return UUcTrue;
	return UUcFalse;
}

// ----------------------------------------------------------------------
LPD3DDEVICEDESC
DMrD3D_DI_GetHALDesc(
	DMtD3D_DeviceInfo				*inDeviceInfo)
{
	return &inDeviceInfo->HALdesc;
}

// ----------------------------------------------------------------------
LPD3DDEVICEDESC
DMrD3D_DI_GetHELDesc(
	DMtD3D_DeviceInfo				*inDeviceInfo)
{
	return &inDeviceInfo->HELdesc;
}

// ----------------------------------------------------------------------
UUtError
DMrD3D_DI_LoadFormats(
	DMtD3D_DeviceInfo				*inDeviceInfo,
	LPDIRECT3DDEVICE2				inD3DDevice2)
{
	// check the parameters
	if (inDeviceInfo == NULL)
	{
		UUrError_Report(UUcError_Generic, "Parameter Error.");
		return UUcError_Generic;
	}

	// Have the formats already been loaded?
	if (!DMiD3D_DI_formatsLoaded(inDeviceInfo))
	{
		DMtDD_CallbackInfo		cb_info;
		HRESULT					result;

		// enumerte all modes for this driver
		cb_info.result = UUcTrue;
		cb_info.count = 0;
		cb_info.data = (void*)inDeviceInfo;
		cb_info.mode = NULL;

		result =
			IDirect3DDevice2_EnumTextureFormats(
				inD3DDevice2,
				DMiD3D_TextureFormatEnumCallback,
				&cb_info);
		if (result != D3D_OK)
		{
			UUrError_Report(
				UUcError_DirectDraw,
				DMrGetErrorMsg(result));
			return UUcError_DirectDraw;
		}

		// double check the count
		if (	(!cb_info.result) ||
				(cb_info.count == 0) ||
				(cb_info.count != inDeviceInfo->num_formats))
		{
			return UUcError_Generic;
		}

		// mark formats as loaded
		DMiD3D_DI_formatsLoadedOn(inDeviceInfo);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
DMiD3D_DI_UnloadFormats(
	DMtD3D_DeviceInfo				*inDeviceInfo)
{
	DMtDD_SurfaceInfo				*format_info;

	format_info = inDeviceInfo->format_list;

	// walk the linked list and destroy all the texture formats
	while (format_info)
	{
		// remove format_info from the list
		inDeviceInfo->format_list = format_info->next;

		// delete the format info structure
		DMiDD_SI_Delete(format_info);

		// move to the next format
		format_info = inDeviceInfo->format_list;
	}
}

// ----------------------------------------------------------------------
DMtDD_SurfaceInfo*
DMrD3D_DI_FindFormat(
	DMtD3D_DeviceInfo			*inDeviceInfo,
	LPDDPIXELFORMAT				inDDPixelFormat)
{
	DMtDD_SurfaceInfo			*curr;

	// search the list for the mode that matches the pixel format
	for (	curr = inDeviceInfo->format_list;
			curr != NULL;
			curr = curr->next)
	{
		if (DMiDD_SI_MatchPixelFormat(curr, inDDPixelFormat))
			break;
	}

	return curr;
}

// ----------------------------------------------------------------------
static UUtError
DMiD3D_DI_AddFormat(
	DMtD3D_DeviceInfo			*inDeviceInfo,
	DMtDD_SurfaceInfo			*inFormatInfo)
{
	// make sure the parameters are valid
	if ((inDeviceInfo == NULL) || (inFormatInfo == NULL))
	{
		UUrError_Report(UUcError_Generic, "Parameter error.\0");
		return UUcError_Generic;
	}

	// Add new format to end of list
	if (inDeviceInfo->format_list == NULL)
	{
		inDeviceInfo->format_list = inFormatInfo;
	}
	else
	{
		DMtDD_SurfaceInfo		*curr;

		// find the end of the list
		curr = inDeviceInfo->format_list;
		while (curr->next)
		{
			curr = curr->next;
		}

		// insert inFormatInfo at the end of the list
		curr->next = inFormatInfo;

		// make sure list ends
		inFormatInfo->next = NULL;
	}

	// update count
	inDeviceInfo->num_formats++;

	return UUcError_None;
}

// ----------------------------------------------------------------------
static DMtD3D_DeviceInfo*
DMiD3D_DI_New(
	LPGUID						inGuid,
	LPSTR						inName,
	LPSTR						inDescription,
	LPD3DDEVICEDESC				inHALDesc,
	LPD3DDEVICEDESC				inHELDesc)
{
	DMtD3D_DeviceInfo			*d3d_device_info;

	// allocate memory for the D3D device info structure
	d3d_device_info = UUrMemory_Block_New(sizeof(DMtD3D_DeviceInfo));
	if (d3d_device_info == NULL)
	{
		UUrError_Report(
			UUcError_OutOfMemory,
			"Unable to allocate Direct3D device info.");
		return NULL;
	}
	UUrMemory_Clear(d3d_device_info, sizeof(DMtD3D_DeviceInfo));

	// copy the in data to the fields of the structure
	d3d_device_info->guid = *inGuid;
	d3d_device_info->HALdesc = *inHALDesc;
	d3d_device_info->HELdesc = *inHELDesc;

	strncpy(d3d_device_info->name, inName, DMcMaxNameLength);
	if (inDescription)
	{
		strncpy(d3d_device_info->desc, inDescription, DMcMaxDescLength);
	}
	else
	{
		strcpy(d3d_device_info->desc, "Unknown.");
	}

	// set the nubmer of texture formats to 0 and mark the list as not loaded
	d3d_device_info->num_formats = 0;
	d3d_device_info->format_list = NULL;

	DMiD3D_DI_formatsLoadedOff(d3d_device_info);

	// mark as valid
	DMiD3D_DI_validOn(d3d_device_info);

	// return pointer to new d3d device info structure
	return d3d_device_info;
}

// ----------------------------------------------------------------------
static void
DMiD3D_DI_Delete(
	DMtD3D_DeviceInfo			*inDeviceInfo)
{
	UUrMemory_Block_Delete(inDeviceInfo);
}

// ----------------------------------------------------------------------
static UUtBool
DMiD3D_DI_isValid(
	DMtD3D_DeviceInfo			*inDeviceInfo)
{
	return (inDeviceInfo->flags & DMcD3DDevice_Valid);
}

// ----------------------------------------------------------------------
static void
DMiD3D_DI_validOn(
	DMtD3D_DeviceInfo			*inDeviceInfo)
{
	inDeviceInfo->flags |= DMcD3DDevice_Valid;
}

// ----------------------------------------------------------------------
static void
DMiD3D_DI_validOff(
	DMtD3D_DeviceInfo			*inDeviceInfo)
{
	inDeviceInfo->flags &= ~DMcD3DDevice_Valid;
}

// ----------------------------------------------------------------------
static UUtBool
DMiD3D_DI_formatsLoaded(
	DMtD3D_DeviceInfo			*inDeviceInfo)
{
	return (inDeviceInfo->flags & DMcD3DDevice_Formats_Loaded);
}

// ----------------------------------------------------------------------
static void
DMiD3D_DI_formatsLoadedOn(
	DMtD3D_DeviceInfo			*inDeviceInfo)
{
	inDeviceInfo->flags |= DMcD3DDevice_Formats_Loaded;
}

// ----------------------------------------------------------------------
static void
DMiD3D_DI_formatsLoadedOff(
	DMtD3D_DeviceInfo			*inDeviceInfo)
{
	inDeviceInfo->flags &= ~DMcD3DDevice_Formats_Loaded;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static BOOL PASCAL
DMiDD_DriverEnumCallback(
	LPGUID						inGuid,
	LPSTR						inDescription,
	LPSTR						inName,
	LPVOID						inData)
{
	DMtDRVRMGR					*drvr_mgr;
	DMtDD_CallbackInfo			*cb_info;
	DMtDD_DriverInfo			*dd_driver_info;
	UUtError					error;

	// if inData is null, something went terribly wrong
	if (inData == NULL)
	{
		return DDENUMRET_OK;
	}

	// get a pointer to the callback info
	cb_info = (DMtDD_CallbackInfo*)inData;
	drvr_mgr = (DMtDRVRMGR*)cb_info->data;

	// create a new driver info structure
	dd_driver_info = DMiDD_DI_New(inGuid, inName, inDescription, cb_info->mode);
	if (dd_driver_info == NULL)
	{
		// not enough memory, go on to next one
		return DDENUMRET_OK;
	}

	// add the driver to the driver list
	error = DMiDRVRMGR_AddDriver(drvr_mgr, dd_driver_info);
	if (error != UUcError_None)
	{
		// unable to add to the driver list, go on to next one
		return DDENUMRET_OK;
	}

	// increment the driver count
	cb_info->count++;

	// continue processing
	return DDENUMRET_OK;
}

// ----------------------------------------------------------------------
static BOOL PASCAL
DDiDD_ModeEnumCallback(
	LPDDSURFACEDESC				inSurfaceDesc,
	LPVOID						inData)
{
	DMtDD_CallbackInfo			*cb_info;
	DMtDD_SurfaceInfo			*dd_mode_info;
	DMtDD_DriverInfo			*dd_driver_info;
	UUtError					error;

	// if inData is NULL, something went terribly wrong
	if (inData == NULL)
	{
		return DDENUMRET_OK;
	}

	// if the surface description is null something went wrong
	if (inSurfaceDesc == NULL)
	{
		return DDENUMRET_CANCEL;
	}

	// get pointers to the data
	cb_info = (DMtDD_CallbackInfo*)inData;
	dd_driver_info = (DMtDD_DriverInfo*)cb_info->data;

	// make sure dd_driver_info is good
	if (dd_driver_info == NULL)
	{
		return DDENUMRET_CANCEL;
	}

	// double check structure size
	if (inSurfaceDesc->dwSize != sizeof(DDSURFACEDESC))
	{
		return DDENUMRET_CANCEL;
	}

	// Create the mode structure
	dd_mode_info = DMiDD_SI_New(inSurfaceDesc, cb_info->mode);
	if (dd_mode_info == NULL)
	{
		// unable to allocate space, go on
		return DDENUMRET_OK;
	}

	// Add Mode to Driver Mode list
	error = DMiDD_DI_AddMode(dd_driver_info, dd_mode_info);
	if (error != UUcError_None)
	{
		return DDENUMRET_OK;
	}

	// increment mode count
	cb_info->count++;

	// continue processing
	return DDENUMRET_OK;
}

// ----------------------------------------------------------------------
static HRESULT PASCAL
DMiD3D_DeviceEnumCallback(
	LPGUID						inGuid,
	LPSTR						inName,
	LPSTR						inDescription,
	LPD3DDEVICEDESC				inHALDesc,
	LPD3DDEVICEDESC				inHELDesc,
	LPVOID						inData)
{
	DMtDD_CallbackInfo			*cb_info;
	DMtDD_DriverInfo			*dd_driver_info;
	DMtD3D_DeviceInfo			*d3d_device_info;
	UUtError					error;

	// if inData is NULL, something went terribly wrong
	if (inData == NULL)
	{
		return DDENUMRET_OK;
	}

	// get pointers to the data
	cb_info = (DMtDD_CallbackInfo*)inData;
	dd_driver_info = (DMtDD_DriverInfo*)cb_info->data;

	// make sure dd_driver_info is good
	if (dd_driver_info == NULL)
	{
		return DDENUMRET_CANCEL;
	}

	// create a D3D device info structure
	d3d_device_info =
		DMiD3D_DI_New(
			inGuid,
			inName,
			inDescription,
			inHALDesc,
			inHELDesc);
	if (d3d_device_info == NULL)
	{
		// unable to allocate space, go on
		return DDENUMRET_OK;
	}

	// Add device to list
	error = DMiDD_DI_AddDevice(dd_driver_info, d3d_device_info);
	if (error != UUcError_None)
	{
		UUrError_Report(error, "Unable to add Direct3D device to list.");
		return DDENUMRET_OK;
	}

	// increment device count
	cb_info->count++;

	// continue processing
	return DDENUMRET_OK;
}

// ----------------------------------------------------------------------
static HRESULT PASCAL
DMiD3D_TextureFormatEnumCallback(
	LPDDSURFACEDESC				inTextureFormat,
	LPVOID						inData)
{
	DMtDD_CallbackInfo			*cb_info;
	DMtD3D_DeviceInfo			*d3d_device_info;
	DMtDD_SurfaceInfo			*format_info;
	UUtError					error;


	// if inData is NULL, something went terribly wrong
	if (inData == NULL)
	{
		return DDENUMRET_OK;
	}

	// get pointers to the data
	cb_info = (DMtDD_CallbackInfo*)inData;
	d3d_device_info = (DMtD3D_DeviceInfo*)cb_info->data;

	// make sure d3d_device_info is good
	if (d3d_device_info == NULL)
	{
		return DDENUMRET_CANCEL;
	}

	// create a Texture Format info structure
	format_info = DMiDD_SI_New(inTextureFormat, NULL);
	if (format_info == NULL)
	{
		// unable to allocate space, go on
		return DDENUMRET_OK;
	}

	// Add format to list
	error = DMiD3D_DI_AddFormat(d3d_device_info, format_info);
	if (error != UUcError_None)
	{
		UUrError_Report(error, "Unable to add Texture format to list.");
		return DDENUMRET_OK;
	}

	// increment device count
	cb_info->count++;

	// continue processing
	return DDENUMRET_OK;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtInt32
DMrFlagsToBitDepth(
	UUtUns32					inFlags)
{
	if (inFlags & DDBD_1)
		return 1;
	else if (inFlags & DDBD_2)
		return 2;
	else if (inFlags & DDBD_4)
		return 4;
	else if (inFlags & DDBD_8)
		return 8;
	else if (inFlags & DDBD_16)
		return 16;
	else if (inFlags & DDBD_24)
		return 24;
	else if (inFlags & DDBD_32)
		return 32;
	else
		return 0;
}

// ----------------------------------------------------------------------
char*
DMrGetErrorMsg(HRESULT error)
{
	switch(error)
	{
		case DD_OK:
			return "No error.\0";
		case DDERR_ALREADYINITIALIZED:
			return "This object is already initialized.\0";
		case DDERR_BLTFASTCANTCLIP:
			return "Return if a clipper object is attached to the source surface passed into a BltFast call.\0";
		case DDERR_CANNOTATTACHSURFACE:
			return "This surface can not be attached to the requested surface.\0";
		case DDERR_CANNOTDETACHSURFACE:
			return "This surface can not be detached from the requested surface.\0";
		case DDERR_CANTCREATEDC:
			return "Windows can not create any more DCs.\0";
		case DDERR_CANTDUPLICATE:
			return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.\0";
		case DDERR_CLIPPERISUSINGHWND:
			return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.\0";
		case DDERR_COLORKEYNOTSET:
			return "No src color key specified for this operation.\0";
		case DDERR_CURRENTLYNOTAVAIL:
			return "Support is currently not available.\0";
		case DDERR_DIRECTDRAWALREADYCREATED:
			return "A DirectDraw object representing this driver has already been created for this process.\0";
		case DDERR_EXCEPTION:
			return "An exception was encountered while performing the requested operation.\0";
		case DDERR_EXCLUSIVEMODEALREADYSET:
			return "An attempt was made to set the cooperative level when it was already set to exclusive.\0";
		case DDERR_GENERIC:
			return "Generic failure.\0";
		case DDERR_HEIGHTALIGN:
			return "Height of rectangle provided is not a multiple of reqd alignment.\0";
		case DDERR_HWNDALREADYSET:
			return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.\0";
		case DDERR_HWNDSUBCLASSED:
			return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.\0";
		case DDERR_IMPLICITLYCREATED:
			return "This surface can not be restored because it is an implicitly created surface.\0";
		case DDERR_INCOMPATIBLEPRIMARY:
			return "Unable to match primary surface creation request with existing primary surface.\0";
		case DDERR_INVALIDCAPS:
			return "One or more of the caps bits passed to the callback are incorrect.\0";
		case DDERR_INVALIDCLIPLIST:
			return "DirectDraw does not support the provided cliplist.\0";
		case DDERR_INVALIDDIRECTDRAWGUID:
			return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.\0";
		case DDERR_INVALIDMODE:
			return "DirectDraw does not support the requested mode.\0";
		case DDERR_INVALIDOBJECT:
			return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.\0";
		case DDERR_INVALIDPARAMS:
			return "One or more of the parameters passed to the function are incorrect.\0";
		case DDERR_INVALIDPIXELFORMAT:
			return "The pixel format was invalid as specified.\0";
		case DDERR_INVALIDPOSITION:
			return "Returned when the position of the overlay on the destination is no longer legal for that destination.\0";
		case DDERR_INVALIDRECT:
			return "Rectangle provided was invalid.\0";
		case DDERR_LOCKEDSURFACES:
			return "Operation could not be carried out because one or more surfaces are locked.\0";
		case DDERR_NO3D:
			return "There is no 3D present.\0";
		case DDERR_NOALPHAHW:
			return "Operation could not be carried out because there is no alpha accleration hardware present or available.\0";
		case DDERR_NOBLTHW:
			return "No blitter hardware present.\0";
		case DDERR_NOCLIPLIST:
			return "No cliplist available.\0";
		case DDERR_NOCLIPPERATTACHED:
			return "No clipper object attached to surface object.\0";
		case DDERR_NOCOLORCONVHW:
			return "Operation could not be carried out because there is no color conversion hardware present or available.\0";
		case DDERR_NOCOLORKEY:
			return "Surface doesn't currently have a color key\0";
		case DDERR_NOCOLORKEYHW:
			return "Operation could not be carried out because there is no hardware support of the destination color key.\0";
		case DDERR_NOCOOPERATIVELEVELSET:
			return "Create function called without DirectDraw object method SetCooperativeLevel being called.\0";
		case DDERR_NODC:
			return "No DC was ever created for this surface.\0";
		case DDERR_NODDROPSHW:
			return "No DirectDraw ROP hardware.\0";
		case DDERR_NODIRECTDRAWHW:
			return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.\0";
		case DDERR_NOEMULATION:
			return "Software emulation not available.\0";
		case DDERR_NOEXCLUSIVEMODE:
			return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.\0";
		case DDERR_NOFLIPHW:
			return "Flipping visible surfaces is not supported.\0";
		case DDERR_NOGDI:
			return "There is no GDI present.\0";
		case DDERR_NOHWND:
			return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.\0";
		case DDERR_NOMIRRORHW:
			return "Operation could not be carried out because there is no hardware present or available.\0";
		case DDERR_NOOVERLAYDEST:
			return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.\0";
		case DDERR_NOOVERLAYHW:
			return "Operation could not be carried out because there is no overlay hardware present or available.\0";
		case DDERR_NOPALETTEATTACHED:
			return "No palette object attached to this surface.\0";
		case DDERR_NOPALETTEHW:
			return "No hardware support for 16 or 256 color palettes.\0";
		case DDERR_NORASTEROPHW:
			return "Operation could not be carried out because there is no appropriate raster op hardware present or available.\0";
		case DDERR_NOROTATIONHW:
			return "Operation could not be carried out because there is no rotation hardware present or available.\0";
		case DDERR_NOSTRETCHHW:
			return "Operation could not be carried out because there is no hardware support for stretching.\0";
		case DDERR_NOT4BITCOLOR:
			return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.\0";
		case DDERR_NOT4BITCOLORINDEX:
			return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.\0";
		case DDERR_NOT8BITCOLOR:
			return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.\0";
		case DDERR_NOTAOVERLAYSURFACE:
			return "Returned when an overlay member is called for a non-overlay surface.\0";
		case DDERR_NOTEXTUREHW:
			return "Operation could not be carried out because there is no texture mapping hardware present or available.\0";
		case DDERR_NOTFLIPPABLE:
			return "An attempt has been made to flip a surface that is not flippable.\0";
		case DDERR_NOTFOUND:
			return "Requested item was not found.\0";
		case DDERR_NOTLOCKED:
			return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.\0";
		case DDERR_NOTPALETTIZED:
			return "The surface being used is not a palette-based surface.\0";
		case DDERR_NOVSYNCHW:
			return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.\0";
		case DDERR_NOZBUFFERHW:
			return "Operation could not be carried out because there is no hardware support for zbuffer blitting.\0";
		case DDERR_NOZOVERLAYHW:
			return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.\0";
		case DDERR_OUTOFCAPS:
			return "The hardware needed for the requested operation has already been allocated.\0";
		case DDERR_OUTOFMEMORY:
			return "DirectDraw does not have enough memory to perform the operation.\0";
		case DDERR_OUTOFVIDEOMEMORY:
			return "DirectDraw does not have enough memory to perform the operation.\0";
		case DDERR_OVERLAYCANTCLIP:
			return "The hardware does not support clipped overlays.\0";
		case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
			return "Can only have ony color key active at one time for overlays.\0";
		case DDERR_OVERLAYNOTVISIBLE:
			return "Returned when GetOverlayPosition is called on a hidden overlay.\0";
		case DDERR_PALETTEBUSY:
			return "Access to this palette is being refused because the palette is already locked by another thread.\0";
		case DDERR_PRIMARYSURFACEALREADYEXISTS:
			return "This process already has created a primary surface.\0";
		case DDERR_REGIONTOOSMALL:
			return "Region passed to Clipper::GetClipList is too small.\0";
		case DDERR_SURFACEALREADYATTACHED:
			return "This surface is already attached to the surface it is being attached to.\0";
		case DDERR_SURFACEALREADYDEPENDENT:
			return "This surface is already a dependency of the surface it is being made a dependency of.\0";
		case DDERR_SURFACEBUSY:
			return "Access to this surface is being refused because the surface is already locked by another thread.\0";
		case DDERR_SURFACEISOBSCURED:
			return "Access to surface refused because the surface is obscured.\0";
		case DDERR_SURFACELOST:
			return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.\0";
		case DDERR_SURFACENOTATTACHED:
			return "The requested surface is not attached.\0";
		case DDERR_TOOBIGHEIGHT:
			return "Height requested by DirectDraw is too large.\0";
		case DDERR_TOOBIGSIZE:
			return "Size requested by DirectDraw is too large, but the individual height and width are OK.\0";
		case DDERR_TOOBIGWIDTH:
			return "Width requested by DirectDraw is too large.\0";
		case DDERR_UNSUPPORTED:
			return "Action not supported.\0";
		case DDERR_UNSUPPORTEDFORMAT:
			return "FOURCC format requested is unsupported by DirectDraw.\0";
		case DDERR_UNSUPPORTEDMASK:
			return "Bitmask in the pixel format requested is unsupported by DirectDraw.\0";
		case DDERR_VERTICALBLANKINPROGRESS:
			return "Vertical blank is in progress.\0";
		case DDERR_WASSTILLDRAWING:
			return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.\0";
		case DDERR_WRONGMODE:
			return "This surface can not be restored because it was created in a different mode.\0";
		case DDERR_XALIGN:
			return "Rectangle provided was not horizontally aligned on required boundary.\0";
		case D3DERR_BADMAJORVERSION:
			return "D3DERR_BADMAJORVERSION\0";
		case D3DERR_BADMINORVERSION:
			return "D3DERR_BADMINORVERSION\0";
		case D3DERR_EXECUTE_LOCKED:
			return "D3DERR_EXECUTE_LOCKED\0";
		case D3DERR_EXECUTE_NOT_LOCKED:
			return "D3DERR_EXECUTE_NOT_LOCKED\0";
		case D3DERR_EXECUTE_CREATE_FAILED:
			return "D3DERR_EXECUTE_CREATE_FAILED\0";
		case D3DERR_EXECUTE_DESTROY_FAILED:
			return "D3DERR_EXECUTE_DESTROY_FAILED\0";
		case D3DERR_EXECUTE_LOCK_FAILED:
			return "D3DERR_EXECUTE_LOCK_FAILED\0";
		case D3DERR_EXECUTE_UNLOCK_FAILED:
			return "D3DERR_EXECUTE_UNLOCK_FAILED\0";
		case D3DERR_EXECUTE_FAILED:
			return "D3DERR_EXECUTE_FAILED\0";
		case D3DERR_EXECUTE_CLIPPED_FAILED:
			return "D3DERR_EXECUTE_CLIPPED_FAILED\0";
		case D3DERR_TEXTURE_NO_SUPPORT:
			return "D3DERR_TEXTURE_NO_SUPPORT\0";
		case D3DERR_TEXTURE_NOT_LOCKED:
			return "D3DERR_TEXTURE_NOT_LOCKED\0";
		case D3DERR_TEXTURE_LOCKED:
			return "D3DERR_TEXTURELOCKED\0";
		case D3DERR_TEXTURE_CREATE_FAILED:
			return "D3DERR_TEXTURE_CREATE_FAILED\0";
		case D3DERR_TEXTURE_DESTROY_FAILED:
			return "D3DERR_TEXTURE_DESTROY_FAILED\0";
		case D3DERR_TEXTURE_LOCK_FAILED:
			return "D3DERR_TEXTURE_LOCK_FAILED\0";
		case D3DERR_TEXTURE_UNLOCK_FAILED:
			return "D3DERR_TEXTURE_UNLOCK_FAILED\0";
		case D3DERR_TEXTURE_LOAD_FAILED:
			return "D3DERR_TEXTURE_LOAD_FAILED\0";
		case D3DERR_MATRIX_CREATE_FAILED:
			return "D3DERR_MATRIX_CREATE_FAILED\0";
		case D3DERR_MATRIX_DESTROY_FAILED:
			return "D3DERR_MATRIX_DESTROY_FAILED\0";
		case D3DERR_MATRIX_SETDATA_FAILED:
			return "D3DERR_MATRIX_SETDATA_FAILED\0";
		case D3DERR_SETVIEWPORTDATA_FAILED:
			return "D3DERR_SETVIEWPORTDATA_FAILED\0";
		case D3DERR_MATERIAL_CREATE_FAILED:
			return "D3DERR_MATERIAL_CREATE_FAILED\0";
		case D3DERR_MATERIAL_DESTROY_FAILED:
			return "D3DERR_MATERIAL_DESTROY_FAILED\0";
		case D3DERR_MATERIAL_SETDATA_FAILED:
			return "D3DERR_MATERIAL_SETDATA_FAILED\0";
		case D3DERR_LIGHT_SET_FAILED:
			return "D3DERR_LIGHT_SET_FAILED\0";
		default:
			return "Unrecognized error value.\0";
	}
}

