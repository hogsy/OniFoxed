/*
	FILE:	Oni_Platform_Mac.c

	AUTHOR:	Brent H. Pease

	CREATED: April 2, 1997

	PURPOSE: Macintosh specific code

	Copyright 1997

*/

/*---------- headers */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BFW_Motoko.h"
#include "BFW_Akira.h"

#include "Oni.h"
#include "Oni_Platform.h"
#include "Oni_Mac_Keys.h"
#include "Oni_Platform_Mac.h"

// define this to use DrawSprocket
//#define USE_DRAW_SPROCKET
#ifdef USE_DRAW_SPROCKET
#include <DrawSprocket.h>
#endif

// define this if we are using a Mac menu bar
//#define USE_MAC_MENU


/*---------- constants */

#define ONcSurface_Left	0
#define ONcSurface_Top	20

enum {
	MAX_DISPLAY_DEVICES= 12 // note: limit is 2^4 - 1
};

/*---------- globals */


WindowPtr oni_mac_window= NULL; // S.S. added this as a reliable means of shielding the cursor when in the main window
static UUtBool menu_bar_is_visible= TRUE;
static ListHandle device_list= NULL;
static int device_count= 0;
static GDHandle original_device= NULL;
static DisplayIDType original_display_id= 0L;
Handle display_state_handle= NULL;
static Boolean changed_display_device= false;

#if defined(DEBUGGING) && DEBUGGING

	#define DEBUG_AKIRA 1

#endif

#if defined(DEBUG_AKIRA) && DEBUG_AKIRA

	UUtUns16*	gAkiraBaseAddr;
	UUtUns16	gAkiraRowBytes;

#endif

#ifdef USE_DRAW_SPROCKET
static DSpContextReference			ONgDSp_Context= NULL;
static DSpContextAttributes			ONgDSp_ContextAttributes= {0};
#endif

/*---------- code */

/*
	Stefan's display select code
	looks more like what you get with DirectX display device enumeration, not what DrawSprocket does
	but... beggers can't be chosers
*/

static UUtUns32 macos_read_display_pref(
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

static void macos_write_display_pref(
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

static pascal Boolean device_list_filter(
	DialogPtr dialog,
	EventRecord *event,
	short *item_hit)
{
	Rect temp_rect;
	Boolean event_handled= false;
	GrafPtr old_port;

	// get the current port, set the port to this dialog
	GetPort(&old_port);
	SetPort(GetDialogPort(dialog));

	// was this an update event for this dialog?
	if ((event->what == updateEvt) && (event->message == (UInt32)dialog))
	{
		Cell item_cell;

		BeginUpdate((WindowPtr)dialog);

		LUpdate(GetPortVisibleRegion(GetDialogPort(dialog), NULL), device_list);

		item_cell.h= 0;
		item_cell.v= 0;

		while (item_cell.v < device_count)
		{
			LDraw(item_cell, device_list);
			item_cell.v++;
		}
		// get the list rectangle
		temp_rect= (*device_list)->rView;
		// push it outwards one pixel and frame the list
		InsetRect(&temp_rect, -1, -1);
		FrameRect(&temp_rect);

        EndUpdate((WindowPtr)dialog);
	}
	else
	{
		if (event->what == mouseDown)
		{
			Point point;

			// we set the port to the dialog on entry to the filter, so a GetMouse will work
            GetMouse(&point);
			temp_rect= (*device_list)->rView;
			// add the scroll bar back in for hit testing (remember we took it out earlier)
			temp_rect.right+= 16;

			// See if they clicked in our list!
			if (PtInRect(point, &temp_rect))
			{
				Boolean clicked= LClick(point, nil, device_list);

				// if they double-clicked the list, return 1, as if the OK button had been pressed
				*item_hit= clicked ? _button_ok : _list_item;
				event_handled= true;
			}
		}
	}

	SetPort(old_port);

	return event_handled;
}

// create a list of display devices & let the user select one (returns main device if it's the only one)
static GDHandle mac_enumerate_display_devices(
	void)
{
	GDHandle display_devices[MAX_DISPLAY_DEVICES]= {NULL}, selected_device;
	Rect list_rect= {0,0,0,1};
	Cell item_cell= {0,0};
	DialogPtr dialog= NULL;
	short item_hit= 0;
	DialogItemType item_type;
	Rect item_rect;
	Handle item_handle;
	OSErr err= noErr;

	selected_device= DMGetFirstScreenDevice(true);
	dialog= GetNewDialog(MAC_DISPLAY_SELECT_RSRC_ID, nil, (WindowPtr)-1L);

	if (dialog)
	{
		//ModalFilterUPP modal_filter_UPP= (ModalFilterUPP)NewModalFilterProc(device_list_filter);
		ModalFilterUPP modal_filter_UPP= (ModalFilterUPP)device_list_filter;

		GetDialogItem(dialog, _list_item, &item_hit, &item_handle, &item_rect);
		item_rect.right-= 16; // make room for scroll bar

		device_list= LNew(&item_rect, &list_rect, item_cell, nil, (WindowPtr)dialog, false, false, false, true);
		if (device_list)
		{
			LSetDrawingMode(true, device_list);

			display_devices[0]= GetDeviceList();

			while (display_devices[device_count] && (device_count < MAX_DISPLAY_DEVICES) && (err == noErr))
			{
				DisplayIDType display_id;

				err= DMGetDisplayIDByGDevice(display_devices[device_count], &display_id, false);
				if (err == noErr)
				{
					Str255 display_name= "\p";

					err= DMGetNameByAVID(display_id, 0L, display_name);
					if (err == noErr)
					{
						int list_count= LAddRow(1, device_count, device_list);

						UUmAssert(list_count == device_count);
						item_cell.h= 0;
						item_cell.v= list_count;
						LAddToCell(&display_name[1], display_name[0], item_cell, device_list);
					}
				}
				++device_count;
				display_devices[device_count]= DMGetNextScreenDevice(display_devices[device_count-1], true);
			}
			// only throw up the dialog if there is more than 1 device to chose from
			if ((err == noErr) && (device_count > 1))
			{
				UUtUns32 display_pref;
				int saved_display_index; // will be saved as a base-1 integer
				UUtBool option_key_down;

				display_pref= macos_read_display_pref();
				saved_display_index= (display_pref >> _mac_saved_display_index_pref_shift);
				option_key_down= LIrPlatform_TestKey(LIcKeyCode_LeftOption, 0);

				if (saved_display_index && (saved_display_index <= device_count) && (option_key_down == UUcFalse))
				{
					// use saved display index
					selected_device= display_devices[saved_display_index-1];
				}
				else
				{
					SetDialogDefaultItem(dialog, _button_ok);
					SetDialogCancelItem(dialog, _button_cancel);
					SetPort(GetDialogPort(dialog));
					ShowWindow((WindowPtr)dialog);
					DrawDialog(dialog);

					item_cell.h= 0;
					item_cell.v= 0;

					while (item_cell.v < device_count)
					{
						LDraw(item_cell, device_list);
						item_cell.v++;
					}

					do
					{
						ModalDialog(modal_filter_UPP, &item_hit);

						if (item_hit == _save_device_check_box)
						{
							GetDialogItem(dialog, _save_device_check_box, &item_type, &item_handle, &item_rect);
							if (item_handle)
							{
								SetControlValue((ControlHandle)item_handle, (GetControlValue((ControlHandle)item_handle) == 0 ? 1 : 0));
							}
						}
					} while ((item_hit != _button_ok) && (item_hit != _button_cancel));

					if (item_hit == _button_ok)
					{
						item_cell.h= 0;
						item_cell.v= 0;

						if (LGetSelect(true, &item_cell, device_list))
						{
							if (display_devices[item_cell.v] != NULL)
							{
								UUtUns32 save_device= 0;

								selected_device= display_devices[item_cell.v];

								// do we need to save the selected display index?
								GetDialogItem(dialog, _save_device_check_box, &item_type, &item_handle, &item_rect);
								if (item_handle)
								{
									save_device= GetControlValue((ControlHandle)item_handle);
								}
								if (save_device)
								{
									saved_display_index= item_cell.v + 1; // save as base-1 integer
									saved_display_index= ((saved_display_index << _mac_saved_display_index_pref_shift) & _mac_saved_display_index_pref_mask);
									display_pref&= (~((UUtUns32)_mac_saved_display_index_pref_mask));
									display_pref|= saved_display_index;
									macos_write_display_pref(display_pref);
								}
							}
						}
					}
				}
			}

			LDispose(device_list);
		}

		DisposeModalFilterUPP(modal_filter_UPP);
		DisposeDialog(dialog);
	}

	if (selected_device == NULL)
	{
		selected_device= GetMainDevice();
	}

	return selected_device;
}

static void mac_restore_original_display_device(
	void)
{
	if (changed_display_device)
	{
		OSErr err;

		// restore original device
		err= DMGetGDeviceByDisplayID(original_display_id, &original_device, true);
		if (err == noErr)
		{
			err= DMSetMainDisplay(original_device, NULL);
			UUmAssert(err == noErr);
		}
		/* it is now safe to allow DM to muck with desktop settings */
		err= DMEndConfigureDisplays(display_state_handle);
		UUmAssert(err == noErr);
	}

	return;
}

static Boolean mac_change_display_device(
	GDHandle new_device)
{
	Boolean success= false;

	if (original_device == NULL)
	{
		original_device= GetMainDevice();
	}

	if (new_device && (new_device != original_device))
	{
		OSErr err;

		// save and change main device
		err= DMGetDisplayIDByGDevice(original_device, &original_display_id, true);
		if (err == noErr)
		{
			/* call this to notify DM that we don't want it mucking with desktop settings
			  since we are going to be responsible citizens and set things back (hehe!) */
			err= DMBeginConfigureDisplays(&display_state_handle);
			if (err == noErr)
			{
				err= DMSetMainDisplay(new_device, NULL);
				if (err == noErr)
				{
					changed_display_device= true;
					success= true;
				}
			}
		}
	}
	else
	{
		success= true;
	}

	return success;
}

// ----------------------------------------------------------------------
static Boolean
ONiDSpCallback(
	EventRecord				*inEvent)
{
	return UUcFalse;
}

UUtBool ONrPlatform_IsForegroundApp(
	void)
{
	return (FrontWindow() == oni_mac_window);
}

static void mac_hide_menu_bar(
	void)
{
	if (HideMenuBar != NULL)
	{
		HideMenuBar();
		menu_bar_is_visible= FALSE;
	}

	return;
}

static void mac_show_menu_bar(
	void)
{
	if ((ShowMenuBar != NULL) && (menu_bar_is_visible == FALSE))
	{
		ShowMenuBar();
	}

	return;
}

UUtError
ONrPlatform_Initialize(
	ONtPlatformData			*outPlatformData)
{
	OSStatus status= noErr;
	BitMap screen_bits;
#ifdef USE_DRAW_SPROCKET
	Boolean					select_possible;
	GDHandle				device;
	DisplayIDType			displayID;
	DSpContextAttributes	attributes;
#endif
	short width, height, depth;
	GDHandle selected_display_device;

	selected_display_device= mac_enumerate_display_devices();
	UUmAssert(selected_display_device);

	if (mac_change_display_device(selected_display_device) == false)
	{
		AUrMessageBox(AUcMBType_OK, "Whoa, something bad happened while trying to change the display device. Try playing Oni from your main monitor next time.");
		exit(0);
	}

	depth= 16;
	if (GetQDGlobalsScreenBits(&screen_bits) != NULL)
	{
		width= screen_bits.bounds.right - screen_bits.bounds.left;
		height= screen_bits.bounds.bottom - screen_bits.bounds.top;
	}
	else
	{
		width= 640;
		height= 480;
	}

#ifdef USE_DRAW_SPROCKET
	// start draw sprocket
	status = DSpStartup();
	if (status != noErr)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to start Draw Sprocket");
	}

	// initialize the draw sprocket attributes
	ONgDSp_ContextAttributes.frequency				= 0;
	ONgDSp_ContextAttributes.displayWidth			= width; //ONgCommandLine.width;
	ONgDSp_ContextAttributes.displayHeight			= height; //ONgCommandLine.height;
	ONgDSp_ContextAttributes.reserved1				= 0;
	ONgDSp_ContextAttributes.reserved2				= 0;
	ONgDSp_ContextAttributes.colorNeeds				= kDSpColorNeeds_Require;
	ONgDSp_ContextAttributes.colorTable				= 0;
	ONgDSp_ContextAttributes.contextOptions			= 0;
	ONgDSp_ContextAttributes.backBufferDepthMask	= (depth == 16 ? kDSpDepthMask_16 : kDSpDepthMask_32);
	ONgDSp_ContextAttributes.displayDepthMask		= (depth == 16 ? kDSpDepthMask_16 : kDSpDepthMask_32);
	ONgDSp_ContextAttributes.backBufferBestDepth	= depth; //32;
	ONgDSp_ContextAttributes.displayBestDepth		= depth; //32;
	ONgDSp_ContextAttributes.pageCount				= 1;
	ONgDSp_ContextAttributes.gameMustConfirmSwitch	= false;
	ONgDSp_ContextAttributes.reserved3[0]			= 0;
	ONgDSp_ContextAttributes.reserved3[1]			= 0;
	ONgDSp_ContextAttributes.reserved3[2]			= 0;
	ONgDSp_ContextAttributes.reserved3[3]			= 0;

	// create the draw sprocket context
	status = DSpCanUserSelectContext(&ONgDSp_ContextAttributes, &select_possible);
	if (status != noErr)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Problem with Draw Sprockets");
	}

	if (select_possible)
	{
		device = DMGetFirstScreenDevice(UUcTrue);
		if (device == NULL)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "No Screen devices available");
		}

		status = DMGetDisplayIDByGDevice(device, &displayID, UUcTrue);
		if (status != noErr)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get Display ID from GDevice");
		}

		status =
			DSpUserSelectContext(
				&ONgDSp_ContextAttributes,
				displayID,
				ONiDSpCallback,
				&ONgDSp_Context);
		if (status != noErr)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "User unable to select context");
		}

/*		status = DSpContext_GetAttributes(ONgDSp_Context, &attributes);
		if (status != noErr)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get context attributes");
		}*/
	}
	else
	{
		status = DSpFindBestContext(&ONgDSp_ContextAttributes, &ONgDSp_Context);
		if (status != noErr)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to find a suitable device");
		}
	}

	status = DSpContext_Reserve(ONgDSp_Context, &ONgDSp_ContextAttributes);
	if (status != noErr)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to create the display");
	}

	status = DSpContext_GetAttributes(ONgDSp_Context, &attributes);
	if (status != noErr)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to get context attributes");
	}

	status = DSpContext_FadeGammaOut(kDSpEveryContext, NULL);
	if (status != noErr)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to fade the display");
	}

	status = DSpContext_SetState(ONgDSp_Context, kDSpContextState_Active);
	if ((status != noErr) && (status != kDSpConfirmSwitchWarning))
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to set the display");
	}

	status = DSpContext_FadeGammaIn(kDSpEveryContext, NULL);
	if (status != noErr)
	{
		UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to fade the display");
	}
#endif // DrawSprocket setup
	// create a window
	{
		Point origin= {0,0};
		Rect win_rect;
		WindowPtr window;
		const RGBColor black_color= {0,0,0};
		unsigned long value= 0;
		OSErr err= noErr;

	#ifdef USE_MAC_MENU
		Handle menu_bar= NULL;

		menu_bar= GetNewMBar(MAC_MBAR_RSRC_ID); // the memory for this is LEAKED.
		if(menu_bar)
		{
			SetMenuBar(menu_bar);
		}
	#endif

	#ifdef USE_DRAW_SPROCKET
		if (ONgDSp_Context)
		{
			DSpContext_LocalToGlobal(ONgDSp_Context, &origin);
		}
	#endif

		win_rect.left= origin.h;
		win_rect.top= origin.v;
		win_rect.right= win_rect.left + width;
		win_rect.bottom= win_rect.top + height;

		window = NewCWindow(NULL, &win_rect, "\p", 0, plainDBox, (WindowPtr)-1, 0, 0);
		if (window == NULL)
		{
			UUmError_ReturnOnErrorMsg(UUcError_Generic, "Unable to create a window");
		}

		// set the context color of the window to black to avoid the white flash when the
		// window appears
		SetWindowContentColor(window, &black_color);
		SetPort(GetWindowPort(window));
		// hide the menu bar and cursor if we are full-screen
		if (M3gResolutionSwitch)
		{
			mac_hide_menu_bar();
			mac_hide_cursor();
			atexit(mac_show_menu_bar);
		}
		//else
		{
			short h= 0;
			short v= GetMBarHeight();

			MoveWindow(window, h, v, true);
		}
		ShowWindow(window);

	#ifdef USE_DRAW_SPROCKET
		if (ONgDSp_Context)
		{
			SetWRefCon(window, (UUtUns32)ONgDSp_Context);
		}
	#endif
		outPlatformData->gameWindow= window;
		oni_mac_window= window;
	}

	return UUcError_None;
}


// ----------------------------------------------------------------------
void ONrPlatform_Terminate(
	void)
{
#ifdef USE_DRAW_SPROCKET
	if (ONgDSp_Context)
	{
		// restore the gamma
		ONrPlatform_SetGamma(100);

		// fade to black
		DSpContext_FadeGammaOut(kDSpEveryContext, NULL);
	}
#endif
	// dispose of the window
	if (ONgPlatformData.gameWindow)
	{
		DisposeWindow(ONgPlatformData.gameWindow);
	}
#ifdef USE_DRAW_SPROCKET
	// fade back to normal
	if (ONgDSp_Context)
	{
		DSpContext_SetState(ONgDSp_Context, kDSpContextState_Inactive);
		DSpContext_FadeGammaIn(kDSpEveryContext, NULL);
		DSpContext_Release(ONgDSp_Context);
		ONgDSp_Context = NULL;
	}
	DSpShutdown();
#endif

	mac_restore_original_display_device();

	return;
}

// ----------------------------------------------------------------------
void
ONrPlatform_CopyAkiraToScreen(
	UUtUns16	inBufferWidth,
	UUtUns16	inBufferHeight,
	UUtUns16	inRowBytes,
	UUtUns16*	inBaseAdddr);

// ----------------------------------------------------------------------
void
ONrPlatform_CopyAkiraToScreen(
	UUtUns16	inBufferWidth,
	UUtUns16	inBufferHeight,
	UUtUns16	inRowBytes,
	UUtUns16*	inBaseAdddr)
{
	#if 0//defined(DEBUG_AKIRA) && DEBUG_AKIRA

		UUtUns16	x, y;

		double*	srcPtr, *dstPtr;

		for(y = 0; y < inBufferHeight; y++)
		{
			srcPtr = (double*)((char*)inBaseAdddr + y * inRowBytes);
			dstPtr = (double*)((char*)gAkiraBaseAddr + y * gAkiraRowBytes);

			for(x = 0; x < inBufferWidth >> 2; x++)
			{
				*dstPtr++ = *srcPtr++;
			}

		}

	#endif
}

// ----------------------------------------------------------------------
void ONrPlatform_Update(
	void)
{
}

// ----------------------------------------------------------------------
void ONrPlatform_ErrorHandler(
	UUtError			theError,
	char				*debugDescription,
	UUtInt32			userDescriptionRef,
	char				*message)
{
	unsigned char	pascalStr[256];

	sprintf((char *)pascalStr+1,"%s (%s)", message, debugDescription);
	pascalStr[0] = strlen((char *)pascalStr+1);

	DebugStr(pascalStr);
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
#define ONcMinGamma			50
#define ONcMaxGamma			150

// ----------------------------------------------------------------------
UUtError
ONrPlatform_GetGammaParams(
	UUtInt32				*outMinGamma,
	UUtInt32				*outMaxGamma,
	UUtInt32				*outCurrentGamma)
{
	*outMinGamma = ONcMinGamma;
	*outMaxGamma = ONcMaxGamma;
	*outCurrentGamma = 100;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtError
ONrPlatform_SetGamma(
	UUtInt32				inNewGamma)
{
#ifdef USE_DRAW_SPROCKET
	DSpContextReference		dsp_context;
	RGBColor				black;

	UUmAssert((inNewGamma >= ONcMinGamma) && (inNewGamma <= ONcMaxGamma));

	// set the color
	black.red = 0;
	black.green = 0;
	black.blue = 0;

	// get the Draw Sprocket context
	dsp_context = (DSpContextReference)GetWRefCon(ONgPlatformData.gameWindow);
	if (dsp_context == NULL) return UUcError_Generic;

	// set the gamma of the context
	DSpContext_FadeGamma(dsp_context, inNewGamma, &black);
#endif

	return UUcError_None;
}
