/*
	gl_macos.c
*/

#ifdef __ONADA__

/*---------- headers */

#define TARGET_API_MAC_CARBON	1

#include <agl.h>
#include <Video.h>
#include <ROMDefs.h>
#include <Devices.h>
#include <Gestalt.h>
#include <Displays.h>
#include <Movies.h>

#include "BFW.h"
#include "gl_engine.h"
#include "Oni_Platform.h"
#include "Oni_Persistance.h"
#include "Oni_Platform_Mac.h"

/*---------- constants */

enum {
	// display setting constants
	KMonoDev= 0, // false (handy definitions for gdDevType settings)
	kColorDev= 1, // true
	char_Enter= 0x03, // for filter proc
	char_Return= 0x0D
};

// requestFlags bit values in VideoRequestRec (example use: 1<<kAbsoluteRequestBit)
enum {
	kBitDepthPriorityBit= 0,	// Bit depth setting has priority over resolution
	kAbsoluteRequestBit= 1,	// Available setting must match request
	kShallowDepthBit= 2,	// Match bit depth less than or equal to request
	kMaximizeResBit= 3,	// Match screen resolution greater than or equal to request
	kAllValidModesBit= 4		// Match display with valid timing modes (may include modes which are not marked as safe)
};

// availFlags bit values in VideoRequestRec (example use: 1<<kModeValidNotSafeBit)
enum {
	kModeValidNotSafeBit		= 0		//  Available timing mode is valid but not safe (requires user confirmation of switch)
};


/*---------- structures */

typedef struct DepthInfo {
	VDSwitchInfoRec			depthSwitchInfo;			// This is the switch mode to choose this timing/depth
	VPBlock					depthVPBlock;				// VPBlock (including size, depth and format)
} DepthInfo;

typedef struct ListIteratorDataRec {
	VDTimingInfoRec			displayModeTimingInfo;		// Contains timing flags and such
	unsigned long			depthBlockCount;			// How many depths available for a particular timing
	DepthInfo				*depthBlocks;				// Array of DepthInfo
} ListIteratorDataRec;

// video request structure
typedef struct VideoRequestRec	{
	GDHandle		screenDevice;		// <in/out>	nil will force search of best device, otherwise search this device only
	short			reqBitDepth;		// <in>		requested bit depth
	short			availBitDepth;		// <out>	available bit depth
	unsigned long	reqHorizontal;		// <in>		requested horizontal resolution
	unsigned long	reqVertical;		// <in>		requested vertical resolution
	unsigned long	availHorizontal;	// <out>	available horizontal resolution
	unsigned long	availVertical;		// <out>	available vertical resolution
	unsigned long	requestFlags;		// <in>		request flags
	unsigned long	availFlags;			// <out>	available mode flags
	unsigned long	displayMode;		// <out>	mode used to set the screen resolution
	unsigned long	depthMode;			// <out>	mode used to set the depth
	VDSwitchInfoRec	switchInfo;			// <out>	DM2.0 uses this rather than displayMode/depthMode combo
} VideoRequestRec, *VideoRequestRecPtr;

/*---------- prototypes */

static boolean gl_get_current_display_setting(short *width, short *height, short *depth);
static boolean gl_test_display_setting(short width, short height, short depth);
static boolean gl_restore_display_settings_use_quicktime(void);
static boolean gl_change_display_setting_use_quicktime(short width, short height);
static boolean gl_change_display_setting(short width, short height, short depth);

static OSErr RVRequestVideoSetting(VideoRequestRecPtr requestRecPtr);
static OSErr RVGetCurrentVideoSetting(VideoRequestRecPtr requestRecPtr);
static OSErr RVSetVideoRequest (VideoRequestRecPtr requestRecPtr);
static void GetRequestTheDM2Way(VideoRequestRecPtr requestRecPtr, GDHandle walkDevice, DMDisplayModeListIteratorUPP myModeIteratorProc,
	DMListIndexType theDisplayModeCount, DMListType *theDisplayModeList);
pascal void ModeListIterator(void *userData, DMListIndexType itemIndex, DMDisplayModeListEntryPtr displaymodeInfo);
static Boolean FindBestMatch(VideoRequestRecPtr requestRecPtr, short bitDepth, unsigned long horizontal, unsigned long vertical);
static pascal Boolean ConfirmAlertFilter(DialogPtr dlg, EventRecord *evt, short *itemHit);

static UUtUns32 gl_macos_read_pref(void);
static void gl_macos_write_pref(UUtUns32 pref);

/*---------- globals */

static short original_display_width= 0;
static short original_display_height= 0;
static short original_display_depth= 0;
static VDSwitchInfoRec original_display_mode_info= {0};
static UUtUns32 available_texture_memory= 0L;
static Ptr quicktime_screen_state_ptr= NULL;

extern Handle display_state_handle; // from Oni_Platform_MacOS.c


/*---------- code */

// WARNING! This function is also called for resolution switching
UUtBool gl_platform_initialize(
	void)
{
	UUtBool success= FALSE;
	OSErr err= noErr;
	AGLPixelFormat pixel_format;
	GLint pixel_attributes_32bit[]= {
		AGL_ACCELERATED,
		AGL_RGBA,
		AGL_DOUBLEBUFFER,
		AGL_DEPTH_SIZE,
		32,
		AGL_NONE
	};
	GLint pixel_attributes_16bit[]= {
		AGL_ACCELERATED,
		AGL_RGBA,
		AGL_DOUBLEBUFFER,
		AGL_DEPTH_SIZE,
		16,
		AGL_NONE
	};
	static short window_width= 0;
	static short window_height= 0;
	static short window_depth= 0;
	
	if (aglChoosePixelFormat == NULL)
	{
		AUrMessageBox(AUcMBType_OK, "Oni requires OpenGL to run; please install OpenGL and try again.");
		exit(0);
	}
	
	if (original_display_width == 0)
	{
		boolean got_settings;
		OSErr err= noErr;
		unsigned long value= 0L;
		
		// save initial display settings
		got_settings= gl_get_current_display_setting(&original_display_width, &original_display_height, &original_display_depth);
		UUrStartupMessage("original display settings= %dx%dx%d : %s", original_display_width, original_display_height, original_display_depth,
			(got_settings ? "SUCCESS" : "FAILED"));
		err= DMGetDisplayMode(GetMainDevice(), &original_display_mode_info);
		if (err != noErr)
		{
			UUrStartupMessage("DMGetDisplayMode() returned err= %d", err);
		}
	}
	
	UUmAssert(ONgPlatformData.gameWindow);
	
	if (gl->context == NULL)
	{
		if (gl->display_mode.bitDepth == 32)
		{
			pixel_format= aglChoosePixelFormat(NULL, 0, pixel_attributes_32bit);
		}
		else
		{
			pixel_format= aglChoosePixelFormat(NULL, 0, pixel_attributes_16bit);
		}
		
		if (pixel_format == NULL)
		{
			AUtMB_ButtonChoice response;
			UUtUns32 gl_macos_pref;
			UUtBool option_key_down;
			
			gl_macos_pref= gl_macos_read_pref();
			option_key_down= LIrPlatform_TestKey(LIcKeyCode_LeftOption, 0);
			if ((gl_macos_pref & _mac_always_search_any_opengl_pref_flag) && (option_key_down == UUcFalse))
			{
				response= AUcMBChoice_Yes;
			}
			else
			{
				response= AUrMessageBox(AUcMBType_YesNo, "Unable to initialize hardware accelerated OpenGL on your system; do you want to continue trying to initialize OpenGL? (note: this could result in poor performace)");
				if (response == AUcMBChoice_Yes)
				{
					gl_macos_pref|= _mac_always_search_any_opengl_pref_flag;
				}
				else
				{
					gl_macos_pref&= (~((UUtUns32)_mac_always_search_any_opengl_pref_flag));
				}
				gl_macos_write_pref(gl_macos_pref);
			}
			
			if (response == AUcMBChoice_Yes)
			{
				GLint pixel_attributes_any32bit[]= {
					AGL_ALL_RENDERERS,
					AGL_RGBA,
					AGL_DOUBLEBUFFER,
					AGL_DEPTH_SIZE,
					32,
					AGL_NONE
				};
				GLint pixel_attributes_any16bit[]= {
					AGL_ALL_RENDERERS,
					AGL_RGBA,
					AGL_DOUBLEBUFFER,
					AGL_DEPTH_SIZE,
					16,
					AGL_NONE
				};
				
				if (gl->display_mode.bitDepth == 32)
				{
					pixel_format= aglChoosePixelFormat(NULL, 0, pixel_attributes_any32bit);
				}
				else
				{
					pixel_format= aglChoosePixelFormat(NULL, 0, pixel_attributes_any16bit);
				}
			}
		}
		
		if (pixel_format != NULL)
		{
			if (M3gResolutionSwitch)
			{
				success= gl_change_display_setting(gl->display_mode.width, gl->display_mode.height, gl->display_mode.bitDepth);
				if (!success)
				{
					UUrStartupMessage("changed display settings to: %dx%dx%d : FAILED", gl->display_mode.width, gl->display_mode.height, gl->display_mode.bitDepth);
					M3gResolutionSwitch= UUcFalse;
					success= TRUE;
				}
				else
				{
					UUrStartupMessage("changed display settings to: %dx%dx%d : SUCCESS", gl->display_mode.width, gl->display_mode.height, gl->display_mode.bitDepth);
				}

			}
			else
			{
				success= TRUE;
			}
			
			if (success)
			{
				gl->context= aglCreateContext(pixel_format, NULL);
				if (gl->context != NULL)
				{
					SizeWindow(ONgPlatformData.gameWindow, gl->display_mode.width, gl->display_mode.height, false);
					success= aglSetDrawable((AGLContext)gl->context, GetWindowPort(ONgPlatformData.gameWindow));
					if (success)
					{
						ONtGraphicsQuality quality_setting= ONrPersist_GetGraphicsQuality();
						UUtUns32 texture_memory_based_on_quality[5]= {
							4 * MEG,
							6 * MEG,
							8 * MEG,
							16 * MEG,
							32 * MEG};
						
						quality_setting= UUmMin(quality_setting, 4);
						available_texture_memory= texture_memory_based_on_quality[quality_setting];
						
						success= aglSetCurrentContext((AGLContext)gl->context);
						aglDestroyPixelFormat(pixel_format);
						window_width= gl->display_mode.width;
						window_height= gl->display_mode.height;
						window_depth= gl->display_mode.bitDepth;
					}
				}
			}
		}
		else // the app doesn't handle failure so we do it here
		{
			AUrMessageBox(AUcMBType_OK, "Unable to initialize OpenGL on your system; Oni will now exit.");
			exit(0);
		}
	}
	else if ((window_width != gl->display_mode.width) || (window_height != gl->display_mode.height) || (window_depth != gl->display_mode.bitDepth))
	{
		// changing screen res from within the game
		// probably need some DrawSprocket code here if we end up using DrawSprocket
		if (M3gResolutionSwitch)
		{
			success= gl_change_display_setting(gl->display_mode.width, gl->display_mode.height, gl->display_mode.bitDepth);
		}
		else
		{
			success= TRUE;
		}
		
		if (success)
		{
			SizeWindow(ONgPlatformData.gameWindow, gl->display_mode.width, gl->display_mode.height, false);
			window_width= gl->display_mode.width;
			window_height= gl->display_mode.height;
			
			if (!aglUpdateContext((AGLContext)gl->context))
			{
				UUrDebuggerMessage("aglUpdateContext() failed after resizing the display");
			}
		}
	}
	else
	{
		// redundant call (also occurs on resolution changing)
		success= TRUE;
	}
	
	if (success)
	{
		GLbyte *renderer_string, *vendor_string;
		
		// punt if we end up in one of Apple's software renderers
		renderer_string= GL_FXN(glGetString)(GL_RENDERER);
		vendor_string= GL_FXN(glGetString)(GL_VENDOR);
		if (renderer_string && vendor_string &&
			(strstr(renderer_string, "APPLE") || strstr(renderer_string, "Apple") || strstr(renderer_string, "apple") ||
			strstr(vendor_string, "APPLE") || strstr(vendor_string, "Apple") || strstr(vendor_string, "apple")))
		{
			AUrMessageBox(AUcMBType_OK, "Unable to initialize hardware accelerated OpenGL on your system with the current settings; Oni will now exit.");
			gl_platform_dispose();
			exit(0);
		}
		
		// cursor won't stay hidden until we are done messing around with display settings
		mac_hide_cursor();
	}

	return success;
}

void gl_platform_dispose(
	void)
{
	if (gl->context)
	{
		aglSetCurrentContext(NULL);
		aglSetDrawable((AGLContext)gl->context, NULL);
		aglDestroyContext((AGLContext)gl->context);
		gl->context= NULL;
	}
	
	// new; since we no longer play bink thru OpenGL we need to grow the window back up to full-screen size
	// on the chance that the end movie is going to play after we exit this function
	if (ONgPlatformData.gameWindow &&
		(original_display_width >= 640) &&
		(original_display_height >= 480))
	{
		SizeWindow(ONgPlatformData.gameWindow, original_display_width, original_display_height, false);
	}
	
	if (M3gResolutionSwitch)
	{
		short width, height, depth;
		
		// restore previous display settings if we messed with them
		if (quicktime_screen_state_ptr)
		{
			gl_restore_display_settings_use_quicktime();
		}
		else if ((gl->display_mode.width != original_display_width) ||
			(gl->display_mode.height != original_display_height) ||
			(gl->display_mode.bitDepth != original_display_depth))
		{
			boolean success;
			
			UUrStartupMessage("resetting display settings to: %dx%dx%d", original_display_width, original_display_height, original_display_depth);
			success= gl_change_display_setting(original_display_width, original_display_height, original_display_depth);
		}
		
		if (gl_get_current_display_setting(&width, &height, &depth))
		{
			UUrStartupMessage("actual exiting display settings= %dx%dx%d", width, height, depth);
			if ((width != original_display_width) ||
				(original_display_height != height) ||
				(original_display_depth != depth))
			{
				UUrStartupMessage("failed to restore original display settings... sorry");
			}
		}
	}
	
	return;
}

void gl_display_back_buffer(
	void)
{
	UUmAssert(gl->context);
	aglSwapBuffers((AGLContext)gl->context);
	
	return;
}

void gl_matrox_display_back_buffer(
	void)
{
	UUmAssert(gl->context);
	aglSwapBuffers((AGLContext)gl->context);
	
	return;
}

int gl_enumerate_valid_display_modes(
	M3tDisplayMode display_mode_list[M3cMaxDisplayModes])
{
	int i, j, n;
	M3tDisplayMode desired_display_mode_list[]= {
			{640, 480, 16, 0},
			{800, 600, 16, 0},
			{1024, 768, 16, 0},
			{1152, 864,	16, 0},
			//{1280, 1024, 16, 0},
			{1600, 1200, 16, 0},
			{1920, 1080, 16, 0},
			//{1920, 1200, 16, 0},
			{640, 480, 32, 0},
			{800, 600, 32, 0},
			{1024, 768, 32, 0},
			{1152, 864, 32, 0},
			//{1280, 1024, 32, 0},
			{1600, 1200, 32, 0},
			{1920, 1080, 32, 0}
			/*{1920, 1200, 32, 0}*/};
	
	n= sizeof(desired_display_mode_list)/sizeof(desired_display_mode_list[0]);
	j= 0;
	for (i= 0; i<n; i++)
	{
		// 11/21/2000 -S.S.
		// only enumerate safe modes
		if (gl_test_display_setting(desired_display_mode_list[i].width,
			desired_display_mode_list[i].height,
			desired_display_mode_list[i].bitDepth))
		{
			display_mode_list[j]= desired_display_mode_list[i];
			++j;
		}
	}
	
	if (j == 0)
	{
		display_mode_list[0]= desired_display_mode_list[0]; // 640x480x16
		display_mode_list[1]= desired_display_mode_list[6]; // 640x480x32
		j= 2;
	}
	
	return j;
}

#define LOAD_GL_EXT(name)	else if (strcmp(string, #name) == 0) { function= name; }
void * aglGetProcAddress(
	const char *string)
{
	void *function;
	
	if (0) {}
	// GL_ARB_multitexture
	LOAD_GL_EXT(glActiveTextureARB)
	LOAD_GL_EXT(glClientActiveTextureARB)
	LOAD_GL_EXT(glMultiTexCoord4fARB)
	LOAD_GL_EXT(glMultiTexCoord2fvARB)
	// GL_ARB_imaging / GL_EXT_blend_color
	LOAD_GL_EXT(glBlendColorEXT)
	// nVidia GL_NV_register_combiners
	/*
	LOAD_GL_EXT(glCombinerParameteriNV)
	LOAD_GL_EXT(glCombinerInputNV)
	LOAD_GL_EXT(glCombinerOutputNV)
	LOAD_GL_EXT(glFinalCombinerInputNV)
	*/
	// GL_ARB_texture_compression
	/*
	LOAD_GL_EXT(glCompressedTexImage3DARB)
	LOAD_GL_EXT(glCompressedTexImage2DARB)
	LOAD_GL_EXT(glCompressedTexImage1DARB)
	LOAD_GL_EXT(glCompressedTexSubImage3DARB)
	LOAD_GL_EXT(glCompressedTexSubImage2DARB)
	LOAD_GL_EXT(glCompressedTexSubImage1DARB)
	LOAD_GL_EXT(glGetCompressedTexImageARB)
	*/
	else
	{
		function= NULL;
	}
		
	return function;
}

UUtUns32 gl_get_mac_texture_memory(
	void)
{
	return available_texture_memory;
}

static boolean gl_get_current_display_setting(
	short *width,
	short *height,
	short *depth)
{
	boolean success= FALSE;
	GDHandle device;
	
	device= GetMainDevice();
	if (device)
	{
		*width= (*(*device)->gdPMap)->bounds.right - (*(*device)->gdPMap)->bounds.left;
		*height= (*(*device)->gdPMap)->bounds.bottom - (*(*device)->gdPMap)->bounds.top;
		*depth= (*((*device)->gdPMap))->pixelSize;
		success= TRUE;
	}
	
	return success;
}

static boolean gl_test_display_setting(
	short width,
	short height,
	short depth)
{
	VideoRequestRec	request= {0};
	boolean success= FALSE;
	
	request.screenDevice= GetMainDevice();
	request.reqHorizontal= width;
	request.reqVertical= height;
	request.reqBitDepth= depth;
	request.displayMode= nil; // must init to nil
	request.depthMode= nil; // must init to nil
	request.requestFlags= 0;						
	if ((RVRequestVideoSetting(&request) == noErr) &&
		(request.reqHorizontal == request.availHorizontal) &&
		(request.reqVertical == request.availVertical) &&
		(request.reqBitDepth == request.availBitDepth))
	{
		success= TRUE;
	}
	
	return success;
}

/* Using QuickTime to go full-screen works on OS X beta, whereas the Display Manager does not */

static boolean gl_change_display_setting_use_quicktime(
	short width,
	short height)
{
	OSErr err;
	RGBColor black_color= {0,0,0};
	unsigned long flags= fullScreenDontChangeMenuBar | fullScreenHideCursor;
	WindowPtr fullscreen_window= NULL;
	short original_width= width;
	short original_height= height;
	
	err= BeginFullScreen(&quicktime_screen_state_ptr, NULL, &width, &height, &fullscreen_window, &black_color, flags);
	if (err != noErr)
	{
		UUrDebuggerMessage("BeginFullScreen() returned err= %d", err);
		quicktime_screen_state_ptr= NULL;
	}
	else
	{
		short dx= 0, dy= 0;
		
		SizeWindow(ONgPlatformData.gameWindow, original_width, original_height, false);
		BringToFront(ONgPlatformData.gameWindow);
		
		if ((width > original_width) || (height > original_height))
		{
			dx= (width - original_width)>>1;
			dy= (height - original_height)>>1;
		}
		
		MoveWindow(ONgPlatformData.gameWindow, dx, dy, true);
	}
	
	return (err == noErr);
}

static boolean gl_restore_display_settings_use_quicktime(
	void)
{
	OSErr err= noErr;
	
	if (quicktime_screen_state_ptr)
	{
		err= EndFullScreen(quicktime_screen_state_ptr, nil);
		if (err != noErr)
		{
			UUrDebuggerMessage("EndFullScreen() returned err= %d", err);
		}
		else
		{
			quicktime_screen_state_ptr= NULL;
		}
	}
	
	return (err == noErr);
}

static boolean gl_change_display_setting(
	short width,
	short height,
	short depth)
{
	boolean success= FALSE, osx= FALSE;
	OSErr err;
	unsigned long value;
	
	err= Gestalt(gestaltSystemVersion, &value);
	if (err == noErr)
	{
		unsigned long major_version;
		
		// value will look like this: 0x00000904 (OS 9.0.4)
		major_version= (value & 0x0000FF00)>>8;
		if (major_version >= 10)
		{
			osx= TRUE;
		}
	}
	
	
	if (osx || quicktime_screen_state_ptr)
	{
		success= gl_change_display_setting_use_quicktime(width, height);
	}
	else
	{
		VideoRequestRec	request= {0};
		short current_width;
		short current_height;
		short current_bitdepth;
		
		if (gl_get_current_display_setting(&current_width, &current_height, &current_bitdepth))
		{
			request.screenDevice= GetMainDevice();
			request.reqHorizontal= width; //(*(*(request.screenDevice))->gdPMap)->bounds.right;	// main screen is always zero offset (bounds.left == 0)
			request.reqVertical= height; //(*(*(requestRec.screenDevice))->gdPMap)->bounds.bottom;	// main screen is always zero offset (bounds.top == 0)
			request.reqBitDepth= depth;
			request.displayMode= nil; // must init to nil
			request.depthMode= nil; // must init to nil
			request.requestFlags= 0; //1<<kAllValidModesBit;					
			if (RVRequestVideoSetting(&request) == noErr)
			{
				if (RVSetVideoRequest(&request) == noErr)
				{
					short result;
					ModalFilterUPP confirmFilterUPP;
			
					if (request.availFlags & 1<<kModeValidNotSafeBit) // new mode is valid but not safe, so ask user to confirm
					{
						UUrStartupMessage("attempted display mode is unsafe");
						
						mac_show_cursor();
		
						//confirmFilterUPP= (ModalFilterUPP)NewModalFilterProc (ConfirmAlertFilter);	// create a new modal filter proc UPP
						confirmFilterUPP= (ModalFilterUPP)ConfirmAlertFilter;	// create a new modal filter proc UPP
						result= Alert(rConfirmSwtchAlrt, confirmFilterUPP);	// alert the user
						DisposeModalFilterUPP(confirmFilterUPP);
				
						mac_hide_cursor();
				
						if (result != iConfirmItem)
						{
							// switch things back
							UUrStartupMessage("reverting to previous display settings");
							UUrMemory_Clear(&request, sizeof(request));
							request.screenDevice= GetMainDevice();
							request.reqHorizontal= current_width;
							request.reqVertical= current_height;
							request.reqBitDepth= current_bitdepth;
							request.displayMode= nil; // must init to nil
							request.depthMode= nil; // must init to nil
							request.requestFlags= 0;	
							
							success= ((RVRequestVideoSetting(&request) == noErr) && (RVSetVideoRequest(&request) == noErr)) ? TRUE : FALSE;
						}
						else
						{
							UUrStartupMessage("using these display settings");
							success= TRUE;
						}
					}
					else
					{
						success= TRUE;
					}
				}
				else
				{
					UUrDebuggerMessage("RVSetVideoRequest() failed");
					success= FALSE;
				}
			}
			else
			{
				UUrDebuggerMessage("RVRequestVideoSetting() failed");
				success= FALSE;
			}
		}
		else
		{
			UUrDebuggerMessage("gl_get_current_display_setting() failed");
			success= FALSE;
		}
		
		if (success == FALSE) // try using QuickTime (odds are slim to none that things would come to this, but just in case...)
		{
			UUrDebuggerMessage("attempting to use QuickTime to change display settings...");
			success= gl_change_display_setting_use_quicktime(width, height);
		}
		else
		{
			MoveWindow(ONgPlatformData.gameWindow, 0, 0, true);
		}
	}
	
	return success;
}

// unsupported gamma stuff
void M3rSetGamma(
	float inGamma)
{
	return;
}

static void RestoreGammaTable(
	void)
{
	return;
}

static void SetupGammaTable(
	void)
{
	return;

}

// from Apple's sample Display Manager code

static OSErr RVSetVideoRequest(
	VideoRequestRecPtr requestRecPtr)
{
	OSErr err= -1; // generic error code
	long value= 0;

	if (requestRecPtr->displayMode && requestRecPtr->depthMode)
	{
		SetDeviceAttribute(requestRecPtr->screenDevice, gdDevType, kColorDev);		
		
		err= DMSetDisplayMode(requestRecPtr->screenDevice,	// GDevice
			requestRecPtr->displayMode,						// DM1.0 uses this
			&requestRecPtr->depthMode,						// DM1.0 uses this
			(unsigned long)&(requestRecPtr->switchInfo),	// DM2.0 uses this rather than displayMode/depthMode combo; Carbon docs say to pass in NULL here tho ...
			display_state_handle);
		if (noErr == err)
		{
			
		}
		else if (kDMDriverNotDisplayMgrAwareErr == err)
		{
			UUrDebuggerMessage("DMSetDisplayMode() failed with error kDMDriverNotDisplayMgrAwareErr");
		}
		else
		{
			UUrDebuggerMessage("DMSetDisplayMode() failed with error %d", err);
		}
	}
	else
	{
		UUrDebuggerMessage("bad display change request", err);
	}
	
	return err;
}

static pascal Boolean ConfirmAlertFilter(
	DialogPtr theDialog,
	EventRecord *theEvent,
	short *itemHit)
{
	char charCode;
	Boolean enterORreturn;
	Boolean returnValue = false;

	if (0 == GetWRefCon(theDialog))
		SetWRefCon(theDialog,TickCount());
	else
	{
		if (GetWRefCon(theDialog) + kSecondsToConfirm * 60 < TickCount())
		{
			returnValue= true;
			theEvent->what= nullEvent;
			*itemHit= iRevertItem;
		}
		else
		{
			if (theEvent->what == keyDown)
			{
				charCode = (char)theEvent->message & charCodeMask;
				enterORreturn = (charCode == (char)char_Return) || (charCode == (char)char_Enter);
				if (enterORreturn)
				{
					theEvent->what = nullEvent;
					returnValue = true;
					*itemHit= 1; // OK
					if (enterORreturn && (0 != (theEvent->modifiers & optionKey)))
					{
						*itemHit= 2; // cancel
					}
				}
			}
		}
	}
	return (returnValue);
}

static OSErr RVRequestVideoSetting(
	VideoRequestRecPtr requestRecPtr)
{
	short							iCount = 0;					// just a counter of GDevices we have seen
	DMDisplayModeListIteratorUPP	myModeIteratorProc = nil;	// for DM2.0 searches
	Boolean							suppliedGDevice;	
	DisplayIDType					theDisplayID;				// for DM2.0 searches
	DMListIndexType					theDisplayModeCount;		// for DM2.0 searches
	DMListType						theDisplayModeList;			// for DM2.0 searches
	GDHandle						walkDevice = nil;			// for everybody
	OSErr							err= noErr;

	// init the needed data before we start
	if (requestRecPtr->screenDevice)							// user wants a specifc device?
	{
		walkDevice = requestRecPtr->screenDevice;
		suppliedGDevice = true;
	}
	else
	{
		walkDevice = DMGetFirstScreenDevice (dmOnlyActiveDisplays);			// for everybody
		suppliedGDevice = false;
	}
	
	//myModeIteratorProc = (DMDisplayModeListIteratorUPP)NewDMDisplayModeListIteratorProc(ModeListIterator);	// for DM2.0 searches
	myModeIteratorProc = (DMDisplayModeListIteratorUPP)(ModeListIterator);	// for DM2.0 searches
	
	// Note that we are hosed if somebody changes the gdevice list behind our backs while we are iterating....
	// ...now do the loop if we can start
	if( walkDevice && myModeIteratorProc) do // start the search
	{
		iCount++;		// GDevice we are looking at (just a counter)
		if((err= DMGetDisplayIDByGDevice( walkDevice, &theDisplayID, true)) == noErr )	// DM1.0 does not need this, but it fits in the loop
		{
			theDisplayModeCount = 0;	// for DM2.0 searches
			if ((err= DMNewDisplayModeList(theDisplayID, 0, 0, &theDisplayModeCount, &theDisplayModeList)) == noErr )
			{
				// search NuBus & PCI the new kool way through Display Manager 2.0
				GetRequestTheDM2Way (requestRecPtr, walkDevice, myModeIteratorProc, theDisplayModeCount, &theDisplayModeList);
				DMDisposeList(theDisplayModeList);	// now toss the lists for this gdevice and go on to the next one
			}
			else
			{
				// search NuBus only the old disgusting way through the slot manager
				//GetRequestTheDM1Way (requestRecPtr, walkDevice);
				UUrDebuggerMessage("DMNewDisplayModeList() returned err= %d", err);
				break; // f*ck that
			}
		}
		else
		{
			UUrDebuggerMessage("DMGetDisplayIDByGDevice() returned err= %d", err);
		}
	} while ( !suppliedGDevice && nil != (walkDevice = DMGetNextScreenDevice ( walkDevice, dmOnlyActiveDisplays )) );	// go until no more gdevices
	if( myModeIteratorProc )
		DisposeDMNotificationUPP(myModeIteratorProc);
	
	return (noErr);	// we were able to get the look for a match
}

static pascal void ModeListIterator(void *userData, DMListIndexType, DMDisplayModeListEntryPtr displaymodeInfo)
{
	unsigned long			depthCount;
	short					iCount;
	ListIteratorDataRec		*myIterateData		= (ListIteratorDataRec*) userData;
	DepthInfo				*myDepthInfo;
	
	// set user data in a round about way
	myIterateData->displayModeTimingInfo		= *displaymodeInfo->displayModeTimingInfo;
	
	// now get the DMDepthInfo info into memory we own
	depthCount = displaymodeInfo->displayModeDepthBlockInfo->depthBlockCount;
	myDepthInfo = (DepthInfo*)NewPtrClear(depthCount * sizeof(DepthInfo));

	// set the info for the caller
	myIterateData->depthBlockCount = depthCount;
	myIterateData->depthBlocks = myDepthInfo;

	// and fill out all the entries
	if (depthCount) for (iCount=0; iCount < depthCount; iCount++)
	{
		myDepthInfo[iCount].depthSwitchInfo = 
			*displaymodeInfo->displayModeDepthBlockInfo->depthVPBlock[iCount].depthSwitchInfo;
		myDepthInfo[iCount].depthVPBlock = 
			*displaymodeInfo->displayModeDepthBlockInfo->depthVPBlock[iCount].depthVPBlock;
	}
}

static void GetRequestTheDM2Way(VideoRequestRecPtr requestRecPtr,
	GDHandle walkDevice,
	DMDisplayModeListIteratorUPP myModeIteratorProc,
	DMListIndexType theDisplayModeCount,
	DMListType *theDisplayModeList)
{
	short					jCount;
	short					kCount;
	ListIteratorDataRec		searchData;

	searchData.depthBlocks = nil;
	// get the mode lists for this GDevice
	for (jCount=0; jCount<theDisplayModeCount; jCount++)		// get info on all the resolution timings
	{
		DMGetIndexedDisplayModeFromList(*theDisplayModeList, jCount, 0, myModeIteratorProc, &searchData);
		
		// for all the depths for this resolution timing (mode)...
		if (searchData.depthBlockCount) for (kCount = 0; kCount < searchData.depthBlockCount; kCount++)
		{
			// only if the mode is valid and is safe or we override it with the kAllValidModesBit request flag
			if	(	searchData.displayModeTimingInfo.csTimingFlags & 1<<kModeValid && 
					(	searchData.displayModeTimingInfo.csTimingFlags & 1<<kModeSafe ||
						requestRecPtr->requestFlags & 1<<kAllValidModesBit
					)
				)
			{
				if (FindBestMatch (	requestRecPtr,
									searchData.depthBlocks[kCount].depthVPBlock.vpPixelSize,
									searchData.depthBlocks[kCount].depthVPBlock.vpBounds.right,
									searchData.depthBlocks[kCount].depthVPBlock.vpBounds.bottom))
				{
					requestRecPtr->screenDevice = walkDevice;
					requestRecPtr->availBitDepth = searchData.depthBlocks[kCount].depthVPBlock.vpPixelSize;
					requestRecPtr->availHorizontal = searchData.depthBlocks[kCount].depthVPBlock.vpBounds.right;
					requestRecPtr->availVertical = searchData.depthBlocks[kCount].depthVPBlock.vpBounds.bottom;
					
					// now set the important info for DM to set the display
					requestRecPtr->depthMode = searchData.depthBlocks[kCount].depthSwitchInfo.csMode;
					requestRecPtr->displayMode = searchData.depthBlocks[kCount].depthSwitchInfo.csData;
					requestRecPtr->switchInfo = searchData.depthBlocks[kCount].depthSwitchInfo;
					if (searchData.displayModeTimingInfo.csTimingFlags & 1<<kModeSafe)
						requestRecPtr->availFlags = 0;							// mode safe
					else requestRecPtr->availFlags = 1<<kModeValidNotSafeBit;	// mode valid but not safe, requires user validation of mode switch
	
				}
			}

		}
	
		if (searchData.depthBlocks)
		{
			DisposePtr ((Ptr)searchData.depthBlocks);	// toss for this timing mode of this gdevice
			searchData.depthBlocks = nil;				// init it just so we know
		}
	}
}

static Boolean FindBestMatch(
	VideoRequestRecPtr requestRecPtr,
	short bitDepth,
	unsigned long horizontal,
	unsigned long vertical)
{
	// •• do the big comparison ••
	// first time only if	(no mode yet) and
	//						(bounds are greater/equal or kMaximizeRes not set) and
	//						(depth is less/equal or kShallowDepth not set) and
	//						(request match or kAbsoluteRequest not set)
	if	(	nil == requestRecPtr->displayMode
			&&
			(	(horizontal >= requestRecPtr->reqHorizontal &&
				vertical >= requestRecPtr->reqVertical)
				||														
				!(requestRecPtr->requestFlags & 1<<kMaximizeResBit)	
			)
			&&
			(	bitDepth <= requestRecPtr->reqBitDepth ||	
				!(requestRecPtr->requestFlags & 1<<kShallowDepthBit)		
			)
			&&
			(	(horizontal == requestRecPtr->reqHorizontal &&	
				vertical == requestRecPtr->reqVertical &&
				bitDepth == requestRecPtr->reqBitDepth)
				||
				!(requestRecPtr->requestFlags & 1<<kAbsoluteRequestBit)	
			)
		)
		{
			// go ahead and set the new values
			return (true);
		}
	else	// can we do better than last time?
	{
		// if	(kBitDepthPriority set and avail not equal req) and
		//		((depth is greater avail and depth is less/equal req) or kShallowDepth not set) and
		//		(avail depth less reqested and new greater avail)
		//		(request match or kAbsoluteRequest not set)
		if	(	(	requestRecPtr->requestFlags & 1<<kBitDepthPriorityBit && 
					requestRecPtr->availBitDepth != requestRecPtr->reqBitDepth
				)
				&&
				(	(	bitDepth > requestRecPtr->availBitDepth &&
						bitDepth <= requestRecPtr->reqBitDepth
					)
					||
					!(requestRecPtr->requestFlags & 1<<kShallowDepthBit)	
				)
				&&
				(	requestRecPtr->availBitDepth < requestRecPtr->reqBitDepth &&
					bitDepth > requestRecPtr->availBitDepth	
				)
				&&
				(	(horizontal == requestRecPtr->reqHorizontal &&	
					vertical == requestRecPtr->reqVertical &&
					bitDepth == requestRecPtr->reqBitDepth)
					||
					!(requestRecPtr->requestFlags & 1<<kAbsoluteRequestBit)	
				)
			)
		{
			// go ahead and set the new values
			return (true);
		}
		else
		{
			// match resolution: minimize Δh & Δv
			if	(	abs((requestRecPtr->reqHorizontal - horizontal)) <=
					abs((requestRecPtr->reqHorizontal - requestRecPtr->availHorizontal)) &&
					abs((requestRecPtr->reqVertical - vertical)) <=
					abs((requestRecPtr->reqVertical - requestRecPtr->availVertical))
				)
			{
				// now we have a smaller or equal delta
				//	if (h or v greater/equal to request or kMaximizeRes not set) 
				if (	(horizontal >= requestRecPtr->reqHorizontal &&
						vertical >= requestRecPtr->reqVertical)
						||
						!(requestRecPtr->requestFlags & 1<<kMaximizeResBit)
					)
				{
					// if	(depth is equal or kBitDepthPriority not set) and
					//		(depth is less/equal or kShallowDepth not set) and
					//		([h or v not equal] or [avail depth less reqested and new greater avail] or depth equal avail) and
					//		(request match or kAbsoluteRequest not set)
					if	(	(	requestRecPtr->availBitDepth == bitDepth ||			
								!(requestRecPtr->requestFlags & 1<<kBitDepthPriorityBit)
							)
							&&
							(	bitDepth <= requestRecPtr->reqBitDepth ||	
								!(requestRecPtr->requestFlags & 1<<kShallowDepthBit)		
							)
							&&
							(	(requestRecPtr->availHorizontal != horizontal ||
								requestRecPtr->availVertical != vertical)
								||
								(requestRecPtr->availBitDepth < requestRecPtr->reqBitDepth &&
								bitDepth > requestRecPtr->availBitDepth)
								||
								(bitDepth == requestRecPtr->reqBitDepth)
							)
							&&
							(	(horizontal == requestRecPtr->reqHorizontal &&	
								vertical == requestRecPtr->reqVertical &&
								bitDepth == requestRecPtr->reqBitDepth)
								||
								!(requestRecPtr->requestFlags & 1<<kAbsoluteRequestBit)	
							)
						)
					{
						// go ahead and set the new values
						return (true);
					}
				}
			}
		}
	}
	return (false);
}

static UUtUns32 gl_macos_read_pref(
	void)
{
	UUtUns32 pref= 0L;
	Handle pref_handle;
	
	pref_handle= GetResource('pref', MAC_PREF_RSRC_ID);
	if (pref_handle && (GetHandleSize(pref_handle) == sizeof(UUtUns32)))
	{
		HLock(pref_handle);
		pref= **((UUtUns32 **)pref_handle);
		HUnlock(pref_handle);
		ReleaseResource(pref_handle);
	}
	
	return pref;
}

static void gl_macos_write_pref(
	UUtUns32 pref)
{
	Handle pref_handle;
	
	pref_handle= GetResource('pref', MAC_PREF_RSRC_ID);
	if (pref_handle)
	{
		SetHandleSize(pref_handle, sizeof(UUtUns32));
		HLock(pref_handle);
		BlockMoveData(&pref, *pref_handle, sizeof(UUtUns32));
		HUnlock(pref_handle);
		ChangedResource(pref_handle);
		WriteResource(pref_handle);
		ReleaseResource(pref_handle);
	}
	
	return;
}


#endif // __ONADA__
