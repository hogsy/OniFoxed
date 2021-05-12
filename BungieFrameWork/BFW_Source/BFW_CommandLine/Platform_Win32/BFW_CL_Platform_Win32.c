// ======================================================================
// BFW_CL_Platform_Win32.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW_CL_Platform_Win32.h"
#include "BFW_CL_Resource.h"

// ======================================================================
// defines
// ======================================================================
#define CLgMaxCommandLineLength		2048

#define MAX_ARGS					25
#define MAX_APPLICATION_NAME		256

// ======================================================================
// globals
// ======================================================================
static char			CLgCommandLine[CLgMaxCommandLineLength];

static char			argStr[CLgMaxCommandLineLength + CLgMaxCommandLineLength];
static char			*CLgArgV[MAX_ARGS + 1];
static char			CLgApplicationName[MAX_APPLICATION_NAME];

// ======================================================================
// prototypes
// ======================================================================
static UUtInt16
CLiParseArgs(
	char			*inCommandLine);
	
static BOOL CALLBACK
CLiDialogBoxProc(
	HWND			inHDlg,
	UINT			inMsg,
	WPARAM			inWParam,
	LPARAM			inLParam);

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static BOOL CALLBACK
CLiDialogBoxProc(
	HWND			inHDlg,
	UINT			inMsg,
	WPARAM			inWParam,
	LPARAM			inLParam)
{
	switch (inMsg)
	{
		case WM_INITDIALOG:
		{
			RECT			dialog_rect;
			UUtInt16		x, y, width, height;
			UUtInt16		screen_width, screen_height;
			
			// get the dialogs dimensions
			GetWindowRect(inHDlg, &dialog_rect);
			
			// calculate the width and height of the dialog
			width = (UUtInt16) (dialog_rect.right - dialog_rect.left);
			height = (UUtInt16) (dialog_rect.bottom - dialog_rect.top);
			
			// get the screen width and height
			screen_width = GetSystemMetrics(SM_CXSCREEN);
			screen_height = GetSystemMetrics(SM_CYSCREEN);
			
			// calculate the x and y of the dialog
			x = (screen_width - width) / 2;
			y = (screen_height - height) / 2;
			
			// set the dialogs position in the middle of the screen
			// flags: ignore width and height 
			SetWindowPos(inHDlg, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOREDRAW);

			SetWindowText(GetDlgItem(inHDlg, ID_EDIT1), CLgCommandLine);
						
			return TRUE;
		}
		break;
			
		case WM_COMMAND:
			switch (LOWORD(inWParam))
			{
				case IDOK:
				{
					// copy the text from the dialog into CLgCommandLine
					GetWindowText(
						GetDlgItem(inHDlg, ID_EDIT1),
						CLgCommandLine,
						CLgMaxCommandLineLength);
					EndDialog(inHDlg, IDOK);
					return TRUE;
				}
				break;
			}
		break;
	}
	
	return FALSE;
}

// ----------------------------------------------------------------------
// taken from ccomand.c
// ----------------------------------------------------------------------
static UUtInt16
CLiParseArgs(
	char			*inCommandLine)
{
	int				n;
	int				Quote;
	char			*p;
	char			*p1;
	char			c;
	
	n = 1;
	Quote = 0;
	p = inCommandLine;
	p1 = (char*)argStr;
	
	while ((c = *p++) != 0)
	{
		if (c==' ') continue;
		
		CLgArgV[n++] = p1;
		
		if (n > MAX_ARGS)
		{
			return (n-1);
		}
		
		do
		{
			if (c=='\\' && *p++) 
			{
				c = *p++;
			}
			else
			{
				if ((c=='"') || (c == '\''))
				{
					if (!Quote)
					{
						Quote = c;
						continue;
					}
					
					if (c == Quote)
					{
						Quote = 0;
						continue;
					}
				}
			}
			*p1++ = c;
		}

		while (*p && ((c = *p++) != ' ' || Quote));

		*p1++ = 0;
	}
	return n;
}

// ======================================================================
#if 0
#pragma mark -
#endif

static void makeCommandLine(int argc, const char *argv[])
{
	char *cl_ptr = CLgCommandLine;
	int itr;

	CLgCommandLine[0] = '\0';

	for(itr = 1; itr < argc; itr++)
	{
		const char *cur_arg = argv[itr];
		int length = strlen(cur_arg);

		strcpy(cl_ptr, cur_arg);
		cl_ptr += length;

		// append a " " if this is not the last argument
		if ((itr + 1) < argc) {
			cl_ptr[0] = ' ';
			cl_ptr[1] = '\0';

			cl_ptr += 1;
		}
	}

	return;
}

// ======================================================================
// ----------------------------------------------------------------------
UUtInt16
CLrPlatform_GetCommandLine(
	int				inArgc,
	const char*		inArgv[],
	char			***outArgV)
{
	HRESULT			result;
	UUtInt16		argc;
	
	// clear CLgCommandLine
	CLgCommandLine[0] = '\0';
	
	// set argc to zero
	argc = 0;
	
	makeCommandLine(inArgc, inArgv);

	if (UUrString_Substring(CLgCommandLine, "-nodialog", CLgMaxCommandLineLength + 1)) {
		// we do not want to bring up the command-line dialog box
		result = IDOK;
	} else {
		// run the dialog box
		result =
			DialogBox(
				NULL,
				MAKEINTRESOURCE(IDD_COMMANDLINE),
				NULL,
				CLiDialogBoxProc);
	}

	if (result == IDOK)
	{
		char	parse_this[CLgMaxCommandLineLength + CLgMaxCommandLineLength + 1];
		
		// clear the array
		UUrMemory_Clear(
			parse_this,
			CLgMaxCommandLineLength + CLgMaxCommandLineLength + 1);
				
		// copy the command line onto parse this
		UUrString_Cat(parse_this, CLgCommandLine, CLgMaxCommandLineLength + CLgMaxCommandLineLength + 1);
		
		// parse the command line
		argc = CLiParseArgs(parse_this);

		// copy the app_name to CLgArgV
		UUrString_Copy(CLgApplicationName, inArgv[0], MAX_APPLICATION_NAME);
		CLgArgV[0]= CLgApplicationName;
		
		*outArgV = CLgArgV;
	}
	
	return argc;
}
