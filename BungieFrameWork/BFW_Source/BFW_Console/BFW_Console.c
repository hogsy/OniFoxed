// ======================================================================
// BFW_Console.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Console.h"
#include "BFW_TextSystem.h"
#include "BFW_ScriptLang.h"
#include "BFW_AppUtilities.h"
#include "BFW_Timer.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

// ======================================================================
// defines
// ======================================================================
#define BytesToLongs(a)				(((a) + 3) >> 2)

#if THE_DAY_IS_MINE
#define COcMaxRememberedCommands	256
#else
#define COcMaxRememberedCommands	8
#endif

#define COcCommandLineColor			IMcShade_Black
#define COcConsoleLinesColor		IMcShade_Black

#define COcInputModeSwitchTime		1

#define COcUsingTicks				0
#define COcFadeTime_Name			"co_fade_time"
#define COcImmediateFadeTime		64

#define COcInitialDisplayValue		"1"
#define COcMaxTextColors			4

#define COcDisplayConsoleName		"co_display"
#define COcDisplayMessagesName		"co_message_display"

#define COcMaxConfigFileLine		128

#define COcCaretDisplayTime			30 /* ticks */
#define COcCaretHideTime			20 /* ticks */

#define COcMaxHooks					(4)

#define COcMessageArea_NumLines			10
#define COcInGameSubtitleArea_NumLines	10
#define COcConsoleArea_FontSize			TScFontSize_Default
#define COcMessageArea_FontSize			12
#define COcInGameSubtitleArea_FontSize	TScFontSize_Default

// ======================================================================
// typedefs
// ======================================================================
// ----------------------------------------------------------------------
typedef struct COtTextEntry
{
	COtPriority			priority;
	UUtUns32			timer;						// amount of time to display the text
	char				prefix[COcMaxPrefixLength];	// prefix for the line
	char				text[COcCharsPerLine];		// text on the line
	const char			*identifier;
	IMtShade			text_color;
	IMtShade			text_shadow;
} COtTextEntry;

// ----------------------------------------------------------------------
typedef struct COtTextArea
{
	TStFontFamily		*font_family;
	TStTextContext		*text_context;				// text context to do the drawing
	TStFormat			formatting;
	UUtUns16			font_size;
	UUtBool				scale_font;
	UUtBool				bottom_justify;
	UUtBool				display_completion;
	UUtBool				fade_over_bounds;
	UUtBool				use_formatting_commands;

	UUtRect				bounds;
	UUtUns16			num_text_entries;
	UUtUns16			max_text_entries;
	COtTextEntry		*text_entries;				// an array (num_text_entries long) of COtTextEntry
	
} COtTextArea;

typedef char		COtCommandLine[COcMaxCharsPerLine];

// ----------------------------------------------------------------------
typedef struct COtHookEntry
{
	const char *prefix;
	UUtUns32 refCon;
	COtConsole_Hook hook;
} COtHookEntry;


// ======================================================================
// globals
// ======================================================================
#if SHIPPING_VERSION
static UUtInt32					COgPriority = (UUtInt32) COcPriority_Subtitle;
#else
static UUtInt32					COgPriority = (UUtInt32) COcPriority_Console;
#endif

static UUtUns16					COgNumHooks = 0;
static COtHookEntry				COgHooks[COcMaxHooks];

static UUtUns32					COgTextColor = 0;
static const IMtShade			COcTextShades[COcMaxTextColors] = {IMcShade_White, IMcShade_Black, IMcShade_Blue,		IMcShade_Red};
static const IMtShade			COcDimShades[COcMaxTextColors] = {IMcShade_Gray50, IMcShade_Gray50, 0xFF6060C0,			0xFFC00000};
static const IMtShade			COcTextShadow[COcMaxTextColors] = {IMcShade_Gray25, IMcShade_Gray75, IMcShade_Gray25,	IMcShade_Black};

static UUtBool					COgInConsole;
static UUtBool					COgActivate;
static UUtBool					COgSetInputMode;
static UUtUns32					COgInputModeSwitchTime;

static UUtUns16					COgDrawAreaWidth;
static UUtUns16					COgDrawAreaHeight;
static UUtRect					COgConsoleBounds;

static UUtUns32					COgUnique;

static UUtBool					COgConsoleDebugFile_Checked;
static BFtDebugFile				*COgConsoleDebugFile;

static COtTextArea				*COgCommandLine;
static COtTextArea				*COgConsoleLines;
static COtTextArea				*COgMessageArea;
static COtTextArea				*COgInGameSubtitleArea;

static COtCommandLine			COgRememberedCommands[COcMaxRememberedCommands];

static UUtInt32					COgRemComNum = 0;
static UUtInt32					COgRemComPos = -1;

static UUtUns32					COgUniqueIdentifier;

// ----------------------------------------------------------------------
static UUtInt32					COgFadeTimeValue = 600;
static UUtBool					COgDisplayMessages = UUcTrue;
static UUtBool					COgDisplayConsole = UUcTrue;
static UUtBool					COgConsoleTemporaryDisable = UUcFalse;
static UUtBool					COgMessagesTemporaryDisable = UUcFalse;

// ----------------------------------------------------------------------
static UUtUns32					COgCaretTime;
static UUtBool					COgCaretDisplayed;

IMtPixelType					COgTexelType;

// ----------------------------------------------------------------------
static UUtUns16					COgCurCompletionNameIndex = 0xFFFF;
static char**					COgCompletionNames;
static UUtUns16					COgNumCompletionNames;
static UUtBool					COgPerformCompletionOnTab;

IMtShade COgDefaultTextShade = IMcShade_White;
IMtShade COgDefaultTextShadowShade = IMcShade_Gray25;


// ======================================================================
// prototypes
// ======================================================================

static UUtBool
COiConsole_CallHook(
	const char *inCommandLine);

static UUtError
COrTextArea_New(
	COtTextArea				**inTextArea,
	UUtRect					*inDestination,
	IMtPixel				inLineColor,
	UUtInt16				inNumTextEntries,
	TStFontFamily			*inFontFamily,
	TStFormat				inFormatting,
	UUtBool					inUseFormattingCommands,
	UUtUns16				inFontSize,
	UUtBool					inScaleFontToScreen,
	UUtBool					inBottomJustify,
	UUtBool					inDisplayCompletion,
	UUtBool					inFadeOverBounds);

static void
COrTextArea_Clear(
	COtTextArea				*inTextArea);

static void
COrTextArea_Delete(
	COtTextArea				*inTextArea);

static void
COrTextArea_DiscardBelowPriority(
	COtTextArea				*inTextArea,
	COtPriority				inPriority);

static void
COrTextArea_DiscardByPriority(
	COtTextArea				*inTextArea,
	COtPriority				inPriority);

static void
COrTextArea_Display(
	COtTextArea				*inTextArea);
	
static void
COrTextArea_Print(
	COtTextArea				*inTextArea,
	COtPriority				inPriority,
	IMtShade				inTextColor,
	IMtShade				inTextShadow,
	const char				*inString,
	const char				*inIdentifier,
	UUtUns32				inFadeTime);
	
static UUtBool
COrTextArea_FadeByIdentifier(
	COtTextArea				*inTextArea,
	const char				*inIdentifier);

static UUtBool
COrTextArea_IdentifierVisible(
	COtTextArea				*inTextArea,
	const char				*inIdentifier);

static void
COrTextArea_RemoveEntry(
	COtTextArea				*inTextArea,
	UUtUns16				inIndex);

static UUtError
COrTextArea_Resize(
	COtTextArea				*inTextArea,
	UUtRect					*inBounds,
	UUtInt16				inNumTextEntries);

static void
COrTextArea_SetPrefix(
	COtTextArea				*inTextArea,
	char					*inPrefix);

static void
COrTextArea_SetTimer(
	COtTextArea				*inTextArea,
	UUtUns32				inTimer);

static void
COrTextArea_Update(
	COtTextArea				*inTextArea,
	UUtUns32				inTicksPassed,
	UUtBool					inDisplayCaret);

// ----------------------------------------------------------------------
static void
COrProcessMouseMove(
	LItInputEvent			*input_event);

static void
COrProcessMouseDown(
	LItInputEvent			*input_event);

static void
COrProcessMouseUp(
	LItInputEvent			*input_event);

static void
COrProcessKeyDown(
	LItInputEvent			*input_event);

static void
COrProcessKeyUp(
	LItInputEvent			*input_event);

static void
COrProcessKeyRepeat(
	LItInputEvent			*input_event);

// ----------------------------------------------------------------------

static void
COrCommand_Complete(
	LItInputEvent			*input_event);
	
static void
COrCommand_Copy(
	char					*inCommandLine);

static void
COrCommand_CycleUp(
	void);

static void
COrCommand_CycleDown(
	void);
	
// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
COiCommand_Print_Callback(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns32	itr;
	
	for(itr = 0; itr < inParameterListLength; itr++)
	{
		switch(inParameterList[itr].type)
		{
			case SLcType_Int32:
				COrConsole_Printf("%d\n", inParameterList[itr].val.i);
				break;
				
			case SLcType_Float:
				COrConsole_Printf("%f\n", inParameterList[itr].val.f);
				break;
				
			case SLcType_String:
				COrConsole_Printf("%s\n", inParameterList[itr].val.str);
				break;
			
			default:
				UUmAssert(0);
		}
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
COiCommand_ToggleText_Callback(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	COgTextColor = (COgTextColor + 1) % COcMaxTextColors;

	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtBool
COrRunConfigFile(
	char*	inFilename)
{
	UUtError		error;
	BFtFileRef		configFileRef;
	BFtTextFile*	configFile;
	char*			curLine;
	
	error = BFrFileRef_Search(inFilename, &configFileRef);
	if(error != UUcError_None)
	{
		return UUcFalse;
	}
	
	if(BFrFileRef_FileExists(&configFileRef) == UUcFalse)
	{
		return UUcFalse;
	}

	error = BFrTextFile_OpenForRead(&configFileRef, &configFile);
	if(error != UUcError_None)
	{
		return UUcFalse;
	}
		
	while(1)
	{
		curLine = BFrTextFile_GetNextStr(configFile);
		if(curLine == NULL)
		{
			break;
		}
		// S.S. added these; I'm actually using a config file now :P
		if ((curLine[0] != '#') && // comment char
			(curLine[0] != '\n') &&
			(curLine[0] != '\r') &&
			(curLine[0] != '\v') &&
			(curLine[0] != '\0'))
		{
			COrCommand_Execute(curLine);
		}
	}
	
	BFrTextFile_Close(configFile);
	
	return UUcTrue;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
COiVariableChanged_Priority(
	void)
{
	UUtUns16 width, height;

	width = M3rDraw_GetWidth();
	height = M3rDraw_GetHeight();

	// set the command line bounds
	COgConsoleBounds.left = 2;
	COgConsoleBounds.right = width - 2;

	if (COgPriority > COcPriority_Console) {
		// set the bounds of the console so that it only fills the letterboxed region
		// this is dependent on ONcLetterBox_Depth in Oni_GameState.c (currently 75)
		COgConsoleBounds.bottom = height;
		COgConsoleBounds.top = COgConsoleBounds.bottom - (height * 60) / 480;
	} else {
		// set the bounds of the console so that it can fill the whole screen, but
		// leave room for the command line
		COgConsoleBounds.top = 0;
		COgConsoleBounds.bottom = height - 25;
	}

	if (COgConsoleLines != NULL) {
		COrTextArea_Resize(COgConsoleLines, &COgConsoleBounds, -1);
		COrTextArea_DiscardBelowPriority(COgConsoleLines, COgPriority);
	}
}

// ----------------------------------------------------------------------
void
COrConsole_SetPriority(
	COtPriority inPriority)
{
	COgPriority = inPriority;
	COiVariableChanged_Priority();
}

// ----------------------------------------------------------------------
UUtError
COrInitialize(
	void)
{
	UUtError				error;

	error;
	
	// set gInConsole
	COgInConsole = UUcFalse;
	
	// set initial values draw area width and height
	COgDrawAreaWidth = 0;
	COgDrawAreaHeight = 0;
	
	// register the console variables
#if CONSOLE_DEBUGGING_COMMANDS
	error = SLrGlobalVariable_Register_Int32(COcFadeTime_Name, "The fade time of the console", &COgFadeTimeValue);	
	UUmError_ReturnOnErrorMsg(error, "Unable to register console control variable");

	SLrGlobalVariable_Register("co_priority", "changes the priority of messages to display on the console",
								SLcReadWrite_ReadWrite, SLcType_Int32, (SLtValue *) &COgPriority, COiVariableChanged_Priority);

	error = SLrGlobalVariable_Register_Bool("co_display", "enables console display", &COgDisplayConsole);	
	UUmError_ReturnOnErrorMsg(error, "Unable to register console control variable");

	error = SLrGlobalVariable_Register_Bool("co_message_display", "enables text message display", &COgDisplayMessages);
	UUmError_ReturnOnErrorMsg(error, "Unable to register console control variable");
	
	// register the console commands
	error = 
		SLrScript_Command_Register_Void(
			"console_print",
			"dumps all arguments",
			"",
			COiCommand_Print_Callback);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new command.");
	
	error =
		SLrScript_Command_Register_Void(
			"co_toggle_text",
			"cycles console text color",
			"",
			COiCommand_ToggleText_Callback);
	UUmError_ReturnOnErrorMsg(error, "Unable to create a new command.");
#endif

	// set the caret flashing time
	COgCaretTime = UUrMachineTime_Sixtieths();

	COgConsoleDebugFile_Checked = UUcFalse;
	COgConsoleDebugFile = NULL;

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
COrTerminate(
	void)
{
	if (COgConsoleDebugFile != NULL) {
		BFrDebugFile_Close(COgConsoleDebugFile);
	}

	COgInConsole = UUcFalse;
	
	// delete the text areas
	if (COgCommandLine) {
		COrTextArea_Delete(COgCommandLine);
	}

	if (COgConsoleLines) {
		COrTextArea_Delete(COgConsoleLines);
	}

	if (COgMessageArea) {
		COrTextArea_Delete(COgMessageArea);
	}

	if (COgInGameSubtitleArea) {
		COrTextArea_Delete(COgInGameSubtitleArea);
	}
}

// ----------------------------------------------------------------------
void
COrClear(
	void)
{
	if (COgConsoleLines) {
		COrTextArea_Clear(COgConsoleLines);
	}

	if (COgMessageArea) {
		COrTextArea_Clear(COgMessageArea);
	}

	if (COgInGameSubtitleArea) {
		COrTextArea_Clear(COgInGameSubtitleArea);
	}
}

// ----------------------------------------------------------------------
UUtError
COrConfigure(
	void)
{
	UUtError				error;
	UUtRect					bounds;
	UUtUns16				drawAreaWidth;
	UUtUns16				drawAreaHeight;
	UUtUns16				ui_width, ui_height, line_height;
	TStFontFamily			*font_family;
	TStFont					*default_font;
	
	// get the font family
	error =
		TSrFontFamily_Get(
			TScFontFamily_Default,
			&font_family);
	UUmError_ReturnOnErrorMsg(error, "Unable to get the font family.");
	
	// get the width and height of the draw area
	drawAreaWidth = M3rDraw_GetWidth();
	drawAreaHeight = M3rDraw_GetHeight();
	
	if ((drawAreaWidth <= 0) || (drawAreaHeight <= 0))
	{
		char error_msg[128]= "";
		
		sprintf(error_msg, "drawAreaWidth= %d   drawAreaHeight= %d", drawAreaWidth, drawAreaHeight);
		UUmError_ReturnOnErrorMsg(UUcError_Generic, error_msg);
	}
	
	// store the draw area width and height
	COgDrawAreaWidth = drawAreaWidth;
	COgDrawAreaHeight = drawAreaHeight;
	
	// set the command line bounds
	bounds.left = 2;
	bounds.top = drawAreaHeight - 23;
	bounds.right = bounds.left + drawAreaWidth - 4;
	bounds.bottom = bounds.top + 21;

	// choose the texel_type
	error = M3rDrawEngine_FindGrayscalePixelType(
				&COgTexelType);
	UUmError_ReturnOnErrorMsg(error, "Unable to find a texel_type");
	
	if (COgCommandLine == NULL) {
		// create the command area
		error = COrTextArea_New(&COgCommandLine, &bounds, IMrPixel_FromShade(COgTexelType, COcCommandLineColor),
								1, font_family, TSc_HLeft, UUcFalse, COcConsoleArea_FontSize, UUcTrue, UUcFalse, UUcTrue, UUcFalse);
		UUmError_ReturnOnErrorMsg(error, "Unable to create the command line.");
	}
	else
	{
		// resize the command area
		error = COrTextArea_Resize(COgCommandLine, &bounds, 1);
		UUmError_ReturnOnErrorMsg(error, "Unable to resize the command line.");
	}
		
	// set the command line prefix
	COrTextArea_SetPrefix(COgCommandLine, "CMD: ");
	
	// set the console lines bounds
	COiVariableChanged_Priority();

	if (COgConsoleLines == NULL)
	{
		// create the console area
		error =
			COrTextArea_New(
				&COgConsoleLines,
				&COgConsoleBounds,
				IMrPixel_FromShade(COgTexelType, COcConsoleLinesColor),
				-1,
				font_family, TSc_HLeft, UUcFalse, COcConsoleArea_FontSize, UUcTrue, UUcTrue, UUcFalse, UUcTrue);
		UUmError_ReturnOnErrorMsg(error, "Unable to create the console area.");
	}
	else
	{
		// resize the console area
		error = COrTextArea_Resize(COgConsoleLines, &COgConsoleBounds, -1);
		UUmError_ReturnOnErrorMsg(error, "Unable to resize the console area.");
	}
	
	// set the message area bounds so that there is room for the in-game UI.
	// currently the in-game UI is a fixed number of pixels, if this changes then
	// this calculation should be based on the draw area's size.
	ui_width = 138;
	ui_height = 128;
	bounds.left = ui_width + 10;
	bounds.right = UUmMax(drawAreaWidth - ui_width - 10, bounds.left + 30);
	bounds.bottom = drawAreaHeight - 2;

	// the area is a fixed number of lines high
	default_font = TSrFontFamily_GetFont(font_family, TScFontSize_Default, TScStyle_Plain);
	if (default_font == NULL) {
		line_height = 23;
	} else {
		line_height = (UUtUns16) TSrFont_GetLineHeight(default_font);
	}
	if (bounds.bottom > 2 + line_height * COcMessageArea_NumLines) {
		bounds.top = bounds.bottom - line_height * COcMessageArea_NumLines;
	} else {
		bounds.top = 2;
	}
	
	if (COgMessageArea == NULL)
	{
		// create the message area
		error =
			COrTextArea_New(
				&COgMessageArea,
				&bounds,
				IMrPixel_FromShade(COgTexelType, COcConsoleLinesColor),
				COcMessageArea_NumLines + 3,
				font_family, TSc_HCenter, UUcTrue, COcMessageArea_FontSize, UUcTrue, UUcTrue, UUcFalse, UUcFalse);
		UUmError_ReturnOnErrorMsg(error, "Unable to create the message area.");
	}
	else
	{
		// resize the message area
		error = COrTextArea_Resize(COgMessageArea, &bounds, COcMessageArea_NumLines + 3);
		UUmError_ReturnOnErrorMsg(error, "Unable to resize the message area.");
	}

	// create the in-game subtitle area in the same location as the message area
	if (COgInGameSubtitleArea == NULL)
	{
		// create the message area
		error =
			COrTextArea_New(
				&COgInGameSubtitleArea,
				&bounds,
				IMrPixel_FromShade(COgTexelType, COcConsoleLinesColor),
				COcInGameSubtitleArea_NumLines + 3,
				font_family, TSc_HLeft, UUcTrue, COcInGameSubtitleArea_FontSize, UUcTrue, UUcTrue, UUcFalse, UUcFalse);
		UUmError_ReturnOnErrorMsg(error, "Unable to create the in-game subtitle area.");
	}
	else
	{
		// resize the message area
		error = COrTextArea_Resize(COgInGameSubtitleArea, &bounds, COcInGameSubtitleArea_NumLines + 3);
		UUmError_ReturnOnErrorMsg(error, "Unable to resize the in-game subtitle area.");
	}

	return UUcError_None;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtBool
COiCompletiond_BuildList_Compare_Function(
	UUtUns32	inA,
	UUtUns32	inB)
{
	UUtBool result = strcmp((char*)inA, (char*)inB) > 0;

	return result;
}

static void
COiCompletion_BuildList(
	void)
{
	UUtUns32	numNames;
	
	SLrScript_ConsoleCompletionList_Get(&numNames, &COgCompletionNames);
	COgNumCompletionNames = (UUtUns16) numNames;
		
	AUrQSort_32(COgCompletionNames, COgNumCompletionNames, COiCompletiond_BuildList_Compare_Function);	
}

// ----------------------------------------------------------------------
static UUtUns16
COiCompletion_FindPartial(
	char*		inName)
{
	UUtUns16	itr;
	UUtUns16	len;
	
	COiCompletion_BuildList();
	
	len = strlen(inName);
	
	for (itr = 0; itr < COgNumCompletionNames; itr++)
	{
		if (strncmp(COgCompletionNames[itr], inName, len) == 0)
			return itr;
	}
	
	return 0xFFFF;
}

static void
COiGetCurPrefix(
	char*	outCompletionBuffer)
{
	char		c, *cp, *commandline;
	UUtUns32	len;

	commandline = COgCommandLine->text_entries[0].text;
	len = strlen(commandline);
	outCompletionBuffer[0] = '\0';
	
	if (len == 0)
		return;
	
	cp = &commandline[len - 1];
	
	while(len > 0)
	{
		c = *cp;
		
		if(c == '"') return;
		
		if(c == ' ' || c == ',' || c == '(' || c == '=') break;
		
		len--;
		cp--;
	}
	
	UUrString_Copy(outCompletionBuffer, cp+1, COcMaxCharsPerLine);
	
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
COrConsole_Activate(
	void)
{
	if (!COgSetInputMode)
	{
		COgActivate = UUcTrue;
		COgSetInputMode = UUcTrue;
		COgInputModeSwitchTime = COcInputModeSwitchTime;
	}
}

// ----------------------------------------------------------------------
void
COrConsole_Deactivate(
	void)
{
	if (!COgSetInputMode)
	{
		COgActivate = UUcFalse;
		COgSetInputMode = UUcTrue;
		COgInputModeSwitchTime = COcInputModeSwitchTime;
	}
}
	
// ----------------------------------------------------------------------
void
COrConsole_TemporaryDisable(
	UUtBool inDisable)
{
	COgConsoleTemporaryDisable = inDisable;
}

// ----------------------------------------------------------------------
void
COrMessage_TemporaryDisable(
	UUtBool inDisable)
{
	COgMessagesTemporaryDisable = inDisable;
}

// ----------------------------------------------------------------------
void
COrConsole_Display_Lines(
	void)
{
	if (COgDisplayConsole && !COgConsoleTemporaryDisable) {
		// display the text in the console
		UUmAssert(COgConsoleLines);
		COrTextArea_Display(COgConsoleLines);
	}
}

// ----------------------------------------------------------------------
void
COrConsole_Display_Messages(
	void)
{
	if (COgDisplayMessages && !COgMessagesTemporaryDisable) {
		if (COgInGameSubtitleArea->num_text_entries > 0) {
			// there are in-game subtitles visible, display them
			UUmAssert(COgInGameSubtitleArea);
			COrTextArea_Display(COgInGameSubtitleArea);
		} else {
			// display the text in the message area
			UUmAssert(COgMessageArea);
			COrTextArea_Display(COgMessageArea);
		}
	}
}

// ----------------------------------------------------------------------
void
COrConsole_Display_Console(
	void)
{
	UUmAssert(COgCommandLine);

	// display the text on the command line
	if (COgInConsole) {
		COgCommandLine->num_text_entries = 1;
		COgCommandLine->text_entries[0].text_color = COgDefaultTextShade;
		COgCommandLine->text_entries[0].text_shadow = COgDefaultTextShadowShade;

		COrTextArea_Display(COgCommandLine);
	}

	return;
}

// ----------------------------------------------------------------------
void
COrConsole_Print(
	COtPriority				inPriority,
	IMtShade				inTextColor,
	IMtShade				inTextShadow,
	const char				*inString)
{
	if (NULL != COgConsoleLines) {
		COrTextArea_Print(COgConsoleLines, inPriority, inTextColor, inTextShadow, inString, NULL, COgFadeTimeValue);
	}

#if SUPPORT_DEBUG_FILES
	if (!COgConsoleDebugFile_Checked) {
		COgConsoleDebugFile = BFrDebugFile_Open("consoleLog.txt");
		COgConsoleDebugFile_Checked = UUcTrue;
	}
		
	if (COgConsoleDebugFile != NULL) {
		BFrDebugFile_Printf(COgConsoleDebugFile, "%s"UUmNL, inString);
	}
#endif

	return;
}

// ----------------------------------------------------------------------
void
COrConsole_PrintIdentified(
	COtPriority				inPriority,
	IMtShade				inTextColor,
	IMtShade				inTextShadow,
	const char				*inString,
	const char				*inIdentifier,
	UUtUns32				inFadeTimer)
{
	if (NULL != COgConsoleLines) {
		COrTextArea_Print(COgConsoleLines, inPriority, inTextColor, inTextShadow, inString, inIdentifier, inFadeTimer);
	}

#if SUPPORT_DEBUG_FILES
	if (!COgConsoleDebugFile_Checked) {
		COgConsoleDebugFile = BFrDebugFile_Open("consoleLog.txt");
		COgConsoleDebugFile_Checked = UUcTrue;
	}
		
	if (COgConsoleDebugFile != NULL) {
		BFrDebugFile_Printf(COgConsoleDebugFile, "%s"UUmNL, inString);
	}
#endif

	return;
}

// ----------------------------------------------------------------------
void
COrMessage_Print(
	const char				*inString,
	const char				*inIdentifier,
	UUtUns32				inFadeTimer)
{
	if (NULL != COgMessageArea) {
		COrTextArea_Print(COgMessageArea, COcPriority_Content, COgDefaultTextShade, COgDefaultTextShadowShade, inString, inIdentifier, inFadeTimer);
	}

	return;
}

// ----------------------------------------------------------------------
void
COrInGameSubtitle_Print(
	const char				*inString,
	const char				*inIdentifier,
	UUtUns32				inFadeTimer)
{
	if (NULL != COgInGameSubtitleArea) {
		COrTextArea_Print(COgInGameSubtitleArea, COcPriority_Content, COgDefaultTextShade, COgDefaultTextShadowShade, inString, inIdentifier, inFadeTimer);
	}

	return;
}

// ----------------------------------------------------------------------
UUtBool
COrConsole_Visible(
	const char				*inIdentifier)
{
	if (NULL != COgConsoleLines) {
		return COrTextArea_IdentifierVisible(COgConsoleLines, inIdentifier);
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtBool
COrConsole_Remove(
	const char				*inIdentifier)
{
	if (NULL != COgConsoleLines) {
		return COrTextArea_FadeByIdentifier(COgConsoleLines, inIdentifier);
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtBool
COrMessage_Visible(
	const char				*inIdentifier)
{
	if (NULL != COgMessageArea) {
		return COrTextArea_IdentifierVisible(COgMessageArea, inIdentifier);
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtBool
COrMessage_Remove(
	const char				*inIdentifier)
{
	if (NULL != COgMessageArea) {
		return COrTextArea_FadeByIdentifier(COgMessageArea, inIdentifier);
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtBool
COrInGameSubtitle_Visible(
	const char				*inIdentifier)
{
	if (NULL != COgInGameSubtitleArea) {
		return COrTextArea_IdentifierVisible(COgInGameSubtitleArea, inIdentifier);
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtBool
COrInGameSubtitle_Remove(
	const char				*inIdentifier)
{
	if (NULL != COgInGameSubtitleArea) {
		return COrTextArea_FadeByIdentifier(COgInGameSubtitleArea, inIdentifier);
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
UUtError
COrConsole_Update(
	UUtUns32				inTicksPassed)
{
	LItInputEvent			input_event;
	
	if (COgInputModeSwitchTime > 0)
		COgInputModeSwitchTime--;
	
	// set the input mode if necessary
	if (COgSetInputMode && (COgInputModeSwitchTime == 0))
	{
		if (COgActivate)
		{
			LIrMode_Set(LIcMode_Normal);
			COgInConsole = UUcTrue;
		}
		else
		{
			LIrMode_Set(LIcMode_Game);
			COgInConsole = UUcFalse;
		}
		COgSetInputMode = UUcFalse;
	}

	if (COgInConsole)
	{
		UUtBool gotEvent;
		UUtUns32 itr;
		#define cMaxEventsPerUpdate  10

		// get an input event
		for(itr = 0; itr < cMaxEventsPerUpdate; itr++)
		{
			gotEvent =
				LIrInputEvent_Get(
					&input_event);

			if ((!gotEvent) || (LIcInputEvent_None == input_event.type))
			{
				break;
			}
		
			// process the input event
			switch (input_event.type)
			{
				case LIcInputEvent_MouseMove:
					COrProcessMouseMove(&input_event);
					break;
				
				case LIcInputEvent_LMouseDown:
					COrProcessMouseDown(&input_event);
					break;
				
				case LIcInputEvent_LMouseUp:
					COrProcessMouseUp(&input_event);
					break;
				
				case LIcInputEvent_KeyDown:
				case LIcInputEvent_KeyRepeat:
					COrProcessKeyDown(&input_event);
					break;
				
				default:
					break;
			}
		};

		// update the command line
		COrTextArea_SetTimer(COgCommandLine, COgFadeTimeValue);
		COrTextArea_Update(COgCommandLine, inTicksPassed, UUcTrue);		
	}

	if (!COgConsoleTemporaryDisable) {
		// update the console text
		COrTextArea_Update(COgConsoleLines, inTicksPassed, UUcFalse);
	}
	
	if (!COgMessagesTemporaryDisable) {
		// update the message and in-game subtitle text
		COrTextArea_Update(COgMessageArea, inTicksPassed, UUcFalse);
		COrTextArea_Update(COgInGameSubtitleArea, inTicksPassed, UUcFalse);
	}
	
	return UUcError_None;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
COrTextArea_New(
	COtTextArea				**outTextArea,
	UUtRect					*inDestination,
	IMtPixel				inLineColor,
	UUtInt16				inNumTextEntries,
	TStFontFamily			*inFontFamily,
	TStFormat				inFormatting,
	UUtBool					inUseFormattingCommands,
	UUtUns16				inFontSize,
	UUtBool					inScaleFontToScreen,
	UUtBool					inBottomJustify,
	UUtBool					inDisplayCompletion,
	UUtBool					inFadeOverBounds)
{
	COtTextArea				*text_area;
	
	// allocate memory for the text area
	text_area = UUrMemory_Block_New(sizeof(COtTextArea));
	UUmError_ReturnOnNull(text_area);

	*outTextArea = text_area;
	text_area->font_family = inFontFamily;
	text_area->bounds = *inDestination;
	text_area->formatting = inFormatting;
	text_area->font_size = inFontSize;
	text_area->scale_font = inScaleFontToScreen;
	text_area->bottom_justify = inBottomJustify;
	text_area->display_completion = inDisplayCompletion;
	text_area->fade_over_bounds = inFadeOverBounds;
	text_area->use_formatting_commands = inUseFormattingCommands;
	text_area->num_text_entries = 0;
	text_area->max_text_entries = 0;
	text_area->text_entries = NULL;
	text_area->text_context = NULL;

	return COrTextArea_Resize(*outTextArea, inDestination, inNumTextEntries);
}

// ----------------------------------------------------------------------
static void
COrTextArea_Clear(
	COtTextArea				*inTextArea)
{
	inTextArea->num_text_entries = 0;
}

// ----------------------------------------------------------------------
static void
COrTextArea_Delete(
	COtTextArea				*inTextArea)
{
	UUmAssert(inTextArea);
	
	// delete the text context
	if (inTextArea->text_context)
	{
		TSrContext_Delete(inTextArea->text_context);
		inTextArea->text_context = 0;
	}
	
	// delete the text_entries array
	UUrMemory_Block_Delete(inTextArea->text_entries);
	
	// release the memory used by the text area
	UUrMemory_Block_Delete(inTextArea);
}

// ----------------------------------------------------------------------
static void COrTextArea_DiscardBelowPriority(COtTextArea *inTextArea, COtPriority inPriority)
{
	UUtUns32 itr;
	COtTextEntry *entry;

	itr = 0;
	entry = inTextArea->text_entries;
	while (itr < inTextArea->num_text_entries) {
		if (entry->priority < inPriority) {
			// remove this entry
			COrTextArea_RemoveEntry(inTextArea, (UUtUns16) itr);
		} else {
			// keep this entry
			itr++;
			entry++;
		}
	}
}

// ----------------------------------------------------------------------
static void COrTextArea_DiscardByPriority(COtTextArea *inTextArea, COtPriority inPriority)
{
	UUtUns32 itr;
	COtTextEntry *entry;

	itr = 0;
	entry = inTextArea->text_entries;
	while (itr < inTextArea->num_text_entries) {
		if (entry->priority == inPriority) {
			// remove this entry
			COrTextArea_RemoveEntry(inTextArea, (UUtUns16) itr);
		} else {
			// keep this entry
			itr++;
			entry++;
		}
	}
}

// ----------------------------------------------------------------------
static void COrTextArea_Display(COtTextArea	*inTextArea)
{
	UUtUns16				i, j, index;
	UUtUns16				alpha;
	UUtInt16				cur_y, y_offset, text_height;
	UUtError				error;
	IMtPoint2D				dest, offset;
	TStStringFormat			formatted_string;
	COtTextEntry			*text_entry;
	char					text[COcMaxCharsPerLine + 1];
	UUtBool					overflow;

	if (inTextArea->bottom_justify) {
		cur_y = inTextArea->bounds.bottom;
	} else {
		cur_y = inTextArea->bounds.top;
	}

	for (i = 0; i < inTextArea->num_text_entries; i++)
	{
		if (inTextArea->bottom_justify) {
			index = inTextArea->num_text_entries - 1 - i;
		} else {
			index = i;
		}

		text_entry = &inTextArea->text_entries[index];
		TSrContext_SetShade(inTextArea->text_context, text_entry->text_color);

		// put the text string together
		UUrString_Copy(text, text_entry->prefix, COcMaxCharsPerLine);
		UUrString_Cat(text, text_entry->text, COcMaxCharsPerLine);

		if ((text_entry->timer == 0) || ((text_entry->timer * 4) > M3cMaxAlpha)) {
			alpha = M3cMaxAlpha;
		} else {
			alpha = (UUtUns16)(text_entry->timer * 4);
		}

		UUmAssert(alpha <= M3cMaxAlpha);

		// get a dummy destination for the text and format it into lines
		dest.x = inTextArea->bounds.left;
		dest.y = inTextArea->bounds.top;
		error = TSrContext_FormatString(inTextArea->text_context, text, &inTextArea->bounds, &dest, &formatted_string);
		if ((error != UUcError_None) || (formatted_string.num_segments == 0)) {
			continue;
		}

		// determine how much vertical space this text requires, and where it should be placed
		text_height = (formatted_string.bounds[formatted_string.num_segments - 1].bottom - formatted_string.bounds[0].top) + 2;
		
		if (inTextArea->bottom_justify) {
			cur_y -= text_height;
			overflow = (cur_y < inTextArea->bounds.top);
			dest.y = cur_y;
		} else {
			dest.y = cur_y;
			cur_y += text_height;
			overflow = (cur_y > inTextArea->bounds.bottom);
		}

		if (overflow) {
			// this line overflows the bounds of the text area
			if (inTextArea->fade_over_bounds) {
				// make it fade out immediately
				if ((text_entry->timer == 0) || (text_entry->timer > COcImmediateFadeTime)) {
					text_entry->timer = COcImmediateFadeTime;
				}
			} else {
				// just don't draw it (or any others after it, since they would overflow too)
				break;
			}
		}

		y_offset = dest.y - inTextArea->bounds.top;
		for (j = 0; j < formatted_string.num_segments; j++) {
			formatted_string.bounds[j].top += y_offset;
			formatted_string.bounds[j].bottom += y_offset;
			formatted_string.destination[j].y += y_offset;
		}

		// draw the drop shadow and then the main text
		offset.x = +1; offset.y = +1;
		TSrContext_DrawFormattedText(inTextArea->text_context, &formatted_string, alpha, &offset, &text_entry->text_shadow);
		TSrContext_DrawFormattedText(inTextArea->text_context, &formatted_string, alpha, NULL, NULL);
		
		if((inTextArea->display_completion) && (i == 0) && (COgCurCompletionNameIndex != 0xFFFF))
		{
			UUtRect	newBounds;
			IMtPoint2D	newDest;
			
			// draw the greyed out part
			TSrContext_SetShade(inTextArea->text_context, IMcShade_Gray50);
			
			TSrContext_GetStringRect(inTextArea->text_context, text, &newBounds);
			newDest = dest;
			newDest.x += newBounds.right + 1;
			newDest.y += 1;
			
			COiGetCurPrefix(text);

			TSrContext_SetShade(inTextArea->text_context, COcTextShadow[COgTextColor]);

			TSrContext_DrawText(
					inTextArea->text_context,
					COgCompletionNames[COgCurCompletionNameIndex] + strlen(text),
					alpha,
					NULL,
					&newDest);
		
			newDest.x -= 1; newDest.y -= 1;

			TSrContext_SetShade(inTextArea->text_context, COcDimShades[COgTextColor]);

			TSrContext_DrawText(
					inTextArea->text_context,
					COgCompletionNames[COgCurCompletionNameIndex] + strlen(text),
					alpha,
					NULL,
					&newDest);
		}
	}
}

// ----------------------------------------------------------------------
static void
COrTextArea_Print(
	COtTextArea				*inTextArea,
	COtPriority				inPriority,
	IMtShade				inTextColor,
	IMtShade				inTextShadow,
	const char				*inString,
	const char				*inIdentifier,
	UUtUns32				inFadeTime)
{
	if ((((UUtInt32) inPriority) >= COgPriority) && (inString != NULL) && (inString[0] != '\0'))
	{
		if (inTextArea->num_text_entries == inTextArea->max_text_entries) {
			// the text area is full, remove the oldest entry
			COrTextArea_RemoveEntry(inTextArea, 0);
		}

		if (inTextArea->num_text_entries < inTextArea->max_text_entries) {
			COtTextEntry			*text_entry;
			UUtUns32				len;

			// write this text into the last text entry
			text_entry = &inTextArea->text_entries[inTextArea->num_text_entries++];

			text_entry->priority = inPriority;
			text_entry->identifier = inIdentifier;
			text_entry->timer = inFadeTime;
			text_entry->text_color = inTextColor;
			text_entry->text_shadow = inTextShadow;

			len = strlen(inString);
			if (len > COcCharsPerLine - 1) {
				// this line must be split up in order to be displayed - produces visual artifacts but
				// it's usually only a problem for debugging text which doesn't matter that much
				UUrString_Copy_Length(text_entry->text, inString, COcCharsPerLine, COcCharsPerLine - 1);
				COrTextArea_Print(inTextArea, inPriority, inTextColor, inTextShadow, &inString[COcCharsPerLine], inIdentifier, inFadeTime);
			} else {
				UUrString_Copy(text_entry->text, inString, COcCharsPerLine);
			}
		}
	}
}

// ----------------------------------------------------------------------
static UUtError
COrTextArea_Resize(
	COtTextArea				*inTextArea,
	UUtRect					*inBounds,
	UUtInt16				inNumTextEntries)
{
	UUtError				error;
	UUtInt16				line_height;
	UUtUns16				font_size;
	UUtInt16				text_width;
	UUtInt16				text_height;
	UUtUns16				max_text_entries;

	if (inTextArea == NULL) {
		return UUcError_None;
	}

	if (inTextArea->scale_font) {
		font_size = (inTextArea->font_size * M3rDraw_GetHeight()) / 480;
	} else {
		font_size = inTextArea->font_size;
	}

	if (inTextArea->text_context == NULL) {
		// create the text context
		error =
			TSrContext_New(
				inTextArea->font_family,
				font_size,
				TScStyle_Plain,
				inTextArea->formatting,
				inTextArea->use_formatting_commands,
				&inTextArea->text_context);
		UUmError_ReturnOnErrorMsg(error, "Unable to initialize the text context.");
	} else {
		TSrContext_SetFontSize(inTextArea->text_context, font_size);
	}

	if (inBounds != NULL) {
		inTextArea->bounds = *inBounds;
	}

	// calculate the width and height of the text area
	text_width = inTextArea->bounds.right - inTextArea->bounds.left;
	text_height = inTextArea->bounds.bottom - inTextArea->bounds.top;
	
	UUmAssert(text_width > 0);
	UUmAssert(text_height > 0);

	if (text_width > 512)
		text_width = 512;
	
	if (inNumTextEntries == -1) {
		// calculate how many text lines can be displayed
		line_height =
			(UUtInt16)TSrFont_GetLineHeight(
				TSrContext_GetFont(inTextArea->text_context, TScStyle_InUse)) + 2;
		max_text_entries = text_height / line_height;
	} else {
		max_text_entries = inNumTextEntries;
	}
	
	if (max_text_entries != inTextArea->max_text_entries) {
		if (inTextArea->text_entries == NULL) {
			// allocate memory for the text entries and clear it to zero
			inTextArea->text_entries = UUrMemory_Block_NewClear(sizeof(COtTextEntry) * max_text_entries);
			UUmError_ReturnOnNull(inTextArea->text_entries);
		} else {
			// resize the text entries array
			inTextArea->text_entries = UUrMemory_Block_Realloc(inTextArea->text_entries, sizeof(COtTextEntry) * max_text_entries);
			UUmError_ReturnOnNull(inTextArea->text_entries);

			if (max_text_entries > inTextArea->max_text_entries) {
				// clear the new memory
				UUrMemory_Clear(&inTextArea->text_entries[inTextArea->max_text_entries], 
								(max_text_entries - inTextArea->max_text_entries) * sizeof(COtTextEntry));
			}
		}
	
		inTextArea->max_text_entries = max_text_entries;
		inTextArea->num_text_entries = UUmMin(inTextArea->num_text_entries, inTextArea->max_text_entries);
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
COrTextArea_SetPrefix(
	COtTextArea				*inTextArea,
	char					*inPrefix)
{
	UUtUns16				i;

	if (inTextArea)
	{
		for (i = 0; i < inTextArea->max_text_entries; i++)
		{
			UUrString_Copy(inTextArea->text_entries[i].prefix, inPrefix, COcMaxPrefixLength);
		}
	}
}

// ----------------------------------------------------------------------
static void
COrTextArea_SetTimer(
	COtTextArea				*inTextArea,
	UUtUns32				inTimer)
{
	UUtUns16				i;
	
	if (!inTextArea)
		return;

	if (inTimer == 0) {
		COrTextArea_Clear(inTextArea);
	} else {
		for (i = 0; i < inTextArea->num_text_entries; i++) {
			inTextArea->text_entries[i].timer = inTimer;
		}
	}
}

// ----------------------------------------------------------------------
static void
COrTextArea_Update(
	COtTextArea				*inTextArea,
	UUtUns32				inTicksPassed,
	UUtBool					inDisplayCaret)
{
	UUtUns16				i;
	
	for (i = 0; i < inTextArea->num_text_entries; i++) {
		if (inTextArea->text_entries[i].timer > 0) {
			if (inTextArea->text_entries[i].timer > inTicksPassed) {
				inTextArea->text_entries[i].timer -= inTicksPassed;
			} else {
				COrTextArea_RemoveEntry(inTextArea, i);
			}
		}
	}
}

// ----------------------------------------------------------------------
static UUtBool
COrTextArea_FadeByIdentifier(
	COtTextArea				*inTextArea,
	const char				*inIdentifier)
{
	UUtUns16				i;
	COtTextEntry			*text_entry;
	UUtBool					found = UUcFalse;

	for (i = 0, text_entry = inTextArea->text_entries; i < inTextArea->num_text_entries; i++, text_entry++) {
		if (inIdentifier != NULL) {
			// only fade specific messages
			if ((text_entry->identifier == NULL) || (!UUmString_IsEqual(text_entry->identifier, inIdentifier))) {
				continue;
			}
		}

		// cause this entry to fade immediately
		if ((text_entry->timer == 0) || (text_entry->timer > COcImmediateFadeTime)) {
			text_entry->timer = COcImmediateFadeTime;
		}
		found = UUcTrue;
	}

	return found;
}

// ----------------------------------------------------------------------
static UUtBool
COrTextArea_IdentifierVisible(
	COtTextArea				*inTextArea,
	const char				*inIdentifier)
{
	UUtUns16				i;
	COtTextEntry			*text_entry;

	for (i = 0, text_entry = inTextArea->text_entries; i < inTextArea->num_text_entries; i++, text_entry++) {
		if ((text_entry->identifier != NULL) && (UUmString_IsEqual(text_entry->identifier, inIdentifier))) {
			return UUcTrue;
		}
	}

	return UUcFalse;
}

// ----------------------------------------------------------------------
static void
COrTextArea_RemoveEntry(
	COtTextArea				*inTextArea,
	UUtUns16				inIndex)
{
	UUmAssert((inIndex >= 0) && (inIndex < inTextArea->num_text_entries));

	if (inIndex + 1 < inTextArea->num_text_entries) {
		UUrMemory_MoveOverlap(&inTextArea->text_entries[inIndex + 1], &inTextArea->text_entries[inIndex],
								(inTextArea->num_text_entries - (inIndex + 1)) * sizeof(COtTextEntry));
	}
	inTextArea->num_text_entries--;
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
COrProcessMouseMove(
	LItInputEvent			*input_event)
{
}

// ----------------------------------------------------------------------
static void
COrProcessMouseDown(
	LItInputEvent			*input_event)
{
}

// ----------------------------------------------------------------------
static void
COrProcessMouseUp(
	LItInputEvent			*input_event)
{
}

// ----------------------------------------------------------------------
static void
COrProcessKeyDown(
	LItInputEvent			*input_event)
{
	UUtUns16				key;
	UUtInt16 				len;
	UUtUns16				potentialNextIndex = COgCurCompletionNameIndex;
	char					completionBuffer[COcMaxCharsPerLine];
	char					*commandline;

	key = (input_event->key & 0x00FF);
	commandline = COgCommandLine->text_entries[0].text;
	len = strlen(commandline);
	
	switch (key)
	{
		case LIcKeyCode_RightArrow:
			if(potentialNextIndex < COgNumCompletionNames)
			{
				potentialNextIndex += 1;
			
				if(strncmp(commandline, COgCompletionNames[potentialNextIndex], len) == 0)
				{
					COgCurCompletionNameIndex = potentialNextIndex;
				}
			}
			break;
			
		case LIcKeyCode_LeftArrow:
			if(potentialNextIndex > 1 && potentialNextIndex < COgNumCompletionNames)
			{
				potentialNextIndex -= 1;

				if(strncmp(commandline, COgCompletionNames[potentialNextIndex], len) == 0)
				{
					COgCurCompletionNameIndex = potentialNextIndex;
				}
			}
			break;

		case LIcKeyCode_Tab:	// tab
			COiGetCurPrefix(completionBuffer);

			COiCompletion_BuildList();

			if(COgPerformCompletionOnTab)
			{
				COgPerformCompletionOnTab = UUcFalse;
				
				if(COgCurCompletionNameIndex >= COgNumCompletionNames)
				{
					COgCurCompletionNameIndex = UUcMaxUns16;
				}
				else
				{
					UUrString_Cat(commandline, COgCompletionNames[COgCurCompletionNameIndex] + strlen(completionBuffer), COcMaxVarNameLength);
				}
			}
			else
			{
				// we need to cycle
				if(input_event->modifiers & (LIcKeyState_LShiftDown | LIcKeyState_RShiftDown))
				{
					if(COgCurCompletionNameIndex == 0 || COgCurCompletionNameIndex >= COgNumCompletionNames)
					{
						COgCurCompletionNameIndex = COgNumCompletionNames - 1;
					}
					else
					{
						COgCurCompletionNameIndex -= 1;
					}
				}
				else
				{
					if(COgCurCompletionNameIndex < COgNumCompletionNames - 1)
					{
						COgCurCompletionNameIndex += 1;
					}
					else
					{
						COgCurCompletionNameIndex = 0;
					}
				}
				
				UUmAssert(COgCurCompletionNameIndex < COgNumCompletionNames);
				
				UUrString_Copy(commandline, COgCompletionNames[COgCurCompletionNameIndex], COcMaxVarNameLength);
			}
			break;
		
		case LIcKeyCode_BackSpace:	// backspace
			COgPerformCompletionOnTab = UUcTrue;
			commandline[len - 1] = '\0';
			
			len--;
			
			if(len == 0)
			{
				COgCurCompletionNameIndex = 0xFFFF;
			}
			else
			{
				if (COgCurCompletionNameIndex == 0xFFFF || strncmp(commandline, COgCompletionNames[COgCurCompletionNameIndex], len))
				{
					COgCurCompletionNameIndex = COiCompletion_FindPartial(commandline);
				}
			}
			break;
		
		case LIcKeyCode_Return:	// return - complete the current command
			COrCommand_Execute(commandline);
			break;

		case LIcKeyCode_Grave:
		case LIcKeyCode_Escape:	// escape
			COrConsole_Deactivate();
			break;
		
		case LIcKeyCode_UpArrow:	// up arrow
			COrCommand_CycleUp();
			COgCurCompletionNameIndex = UUcMaxUns16;
			COgPerformCompletionOnTab = UUcFalse;
			break;
		
		case LIcKeyCode_DownArrow:	// down arrow
			COrCommand_CycleDown();
			COgCurCompletionNameIndex = UUcMaxUns16;
			COgPerformCompletionOnTab = UUcFalse;
			break;
		
		default:
			COgPerformCompletionOnTab = UUcTrue;
			if (len < COcCharsPerLine) {
				commandline[len++] = (input_event->key & 0x00FF);
				commandline[len] = '\0';
			}
			break;
	}

	// find the current match
	COiGetCurPrefix(completionBuffer);
	if (completionBuffer[0] != 0)
	{
		if(COgCurCompletionNameIndex == 0xFFFF ||
			strncmp(completionBuffer, COgCompletionNames[COgCurCompletionNameIndex], len))
		{
			COgCurCompletionNameIndex = COiCompletion_FindPartial(completionBuffer);
		}
	}
	else
	{
		COgCurCompletionNameIndex = 0xFFFF;
	}
}

// ----------------------------------------------------------------------
static void
COrProcessKeyUp(
	LItInputEvent			*input_event)
{
}

// ----------------------------------------------------------------------
static void
COrProcessKeyRepeat(
	LItInputEvent			*input_event)
{
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtBool
COrCommand_Execute(
	char					*inCommandLine)
{
	UUtBool					success;
	
	if(inCommandLine[0] == '+')
	{
		if(COrRunConfigFile(inCommandLine + 1) == UUcTrue)
		{
			COrCommand_Copy(inCommandLine);
		}
		
		success = UUcTrue;
	}
	else
	{
		// try a hook
		success = COiConsole_CallHook(inCommandLine);

		if (success) 
		{
			// copy command to command list - only valid commands get copied
			COrCommand_Copy(inCommandLine);

			// clear the command line
			COgCommandLine->text_entries[0].text[0] = '\0';
		}
	}		

	return success;
}

// ----------------------------------------------------------------------
static void
COrCommand_Copy(
	char					*inCommandLine)
{
	if (strlen(inCommandLine) > 5)
	{
		if (COgRemComNum >= COcMaxRememberedCommands)
		{
			UUtInt32	itr;
			// move all commands down
			for(itr = 1; itr < COcMaxRememberedCommands; itr++)
			{
				UUrString_Copy(COgRememberedCommands[itr-1], COgRememberedCommands[itr], COcMaxCharsPerLine);
			}
		}
		
		UUrString_Copy(COgRememberedCommands[COgRemComNum], inCommandLine, COcMaxCharsPerLine);
		
		COgRemComPos = COgRemComNum;
		
		if(COgRemComNum < COcMaxRememberedCommands) COgRemComNum++;
	}
}

// ----------------------------------------------------------------------
static void
COrCommand_CycleUp(
	void)
{
	if(COgRemComPos >= 0)
	{
		// copy the previous command
		COrTextArea_Print(COgCommandLine, COcPriority_Console, IMcShade_White, IMcShade_Black, COgRememberedCommands[COgRemComPos], NULL, 0);
		
		if(COgRemComPos > 0) COgRemComPos--;
				
	}
}

// ----------------------------------------------------------------------
static void
COrCommand_CycleDown(
	void)
{
	if(COgRemComPos >= 0)
	{
		if(COgRemComPos < COgRemComNum) 
		{
			COgRemComPos++;

			// copy the next command
			COrTextArea_Print(COgCommandLine, COcPriority_Console, IMcShade_White, IMcShade_Black, COgRememberedCommands[COgRemComPos], NULL, 0);
		}
		else
		{
			// clear the command line
			COgCommandLine->text_entries[0].text[0] = '\0';
		}
	}
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
#if THE_DAY_IS_MINE
void UUcArglist_Call COrConsole_Printf(const char *format, ...)
{
	char buffer[2048];
	va_list arglist;
	int return_value;
	
	va_start(arglist, format);
	return_value= vsprintf(buffer, format, arglist);
	va_end(arglist);

	COrConsole_Print(COcPriority_Console, COgDefaultTextShade, COgDefaultTextShadowShade, buffer);

	return;
}
#endif

void UUcArglist_Call COrConsole_Printf_Color(COtPriority inPriority, IMtShade inTextColor, IMtShade inTextShadow, const char *format, ...)
{
	char buffer[2048];
	va_list arglist;
	int return_value;
	
	va_start(arglist, format);
	return_value= vsprintf(buffer, format, arglist);
	va_end(arglist);

	COrConsole_Print(inPriority, inTextColor, inTextShadow, buffer);
}

// ======================================================================
#if UUmCompiler == UUmCompiler_MWerks
#pragma mark -
#endif
// ======================================================================




// ----------------------------------------------------------------------
// generic UUtUns32 callback (assumes adress is in refcon)



UUtError
COrConsole_AddHook(const char *inPrefix, UUtUns32 inRefCon, COtConsole_Hook inHook)
{
	UUmAssertReadPtr(inPrefix, sizeof(char));

	if (COcMaxHooks == COgNumHooks) {
		return UUcError_Generic;
	}

	COgHooks[COgNumHooks].prefix = inPrefix;
	COgHooks[COgNumHooks].refCon = inRefCon;
	COgHooks[COgNumHooks].hook = inHook;

	COgNumHooks++;

	return UUcError_None;
}

static UUtBool COiConsole_CallHook(const char *inCommandLine)
{
	UUtInt32 itr;
	UUtInt32 count = COgNumHooks;
		
	if (NULL == inCommandLine) {
		return UUcFalse;
	}

	UUmAssertReadPtr(inCommandLine, sizeof(char));

	for(itr = 0; itr < count; itr++)
	{
		const char *prefix = COgHooks[itr].prefix;
		UUtUns32 refCon = COgHooks[itr].refCon;
		UUtBool handled = UUcFalse;
		
		if (UUrString_HasPrefix(inCommandLine, prefix)) {
			handled = COgHooks[itr].hook(prefix, refCon, inCommandLine);
		}

		if (handled) { return UUcTrue; }
	}

	return UUcFalse;
}

void COrSubtitle_Clear(void)
{
	COrTextArea_DiscardByPriority(COgConsoleLines, COcPriority_Subtitle);
}

static COtStatusLine *COgStatusLines = NULL;
static TStTextContext*ONgStatusLineTextContext = NULL;
extern M3tTextureMap*ONgLetterboxTexture;

void COrConsole_StatusLines_Begin(COtStatusLine *inStatusLines, UUtBool *inBoolPtr, UUtUns32 inCount)
{
	UUtUns32 itr;

	UUrMemory_Clear(inStatusLines, sizeof(COtStatusLine) * inCount);

	for(itr = 0; itr < inCount; itr++)
	{
		UUtUns32 index = inCount - itr - 1;
		COtStatusLine *current_line = inStatusLines + index;

		current_line->inUse = inBoolPtr;
		COrConsole_StatusLine_Add(current_line);
	}

	return;
}

void COrConsole_StatusLine_Add(COtStatusLine *inStatusLine)
{
	UUmAssert(NULL == inStatusLine->prev);
	UUmAssert(NULL == inStatusLine->next);

	inStatusLine->prev = NULL;
	inStatusLine->next = COgStatusLines;
	COgStatusLines = inStatusLine;

	return;
}

void COrConsole_StatusLine_Remove(COtStatusLine *inStatusLine)
{
	if (inStatusLine->next != NULL) {
		inStatusLine->next->prev = inStatusLine->prev;
	}

	if (inStatusLine->prev != NULL) {
		inStatusLine->prev->next = inStatusLine->next;
	}
	else {
		COgStatusLines = inStatusLine->next;
	}

	return;
}

void COrConsole_StatusLine_Display(void)
{
	const UUtUns16 screen_width = M3rDraw_GetWidth();
	const UUtUns16 screen_height = M3rDraw_GetHeight();
	COtStatusLine *current_line;
	IMtPoint2D draw_location;
	UUtUns16 line_height = TSrFont_GetLineHeight(TSrContext_GetFont(ONgStatusLineTextContext, TScStyle_InUse));
	UUtUns16 ascending_height = TSrFont_GetAscendingHeight(TSrContext_GetFont(ONgStatusLineTextContext, TScStyle_InUse));

	if (ONgLetterboxTexture == NULL) {
		return;
	}

	draw_location.x = screen_width - 256;
	draw_location.y = 30;

	M3rDraw_State_Push();

	M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, 0x80);
	
	M3rDraw_State_SetPtr(M3cDrawStatePtrType_BaseTextureMap, ONgLetterboxTexture);

	for(current_line = COgStatusLines; current_line != NULL; current_line = current_line->next)
	{
		UUtBool *inUse = current_line->inUse;
		M3tPointScreen screen_points[2];

		screen_points[0].x = (float) (screen_width - 256);
		screen_points[0].y = (float) (draw_location.y - ascending_height);
		screen_points[0].z = 0.5f;
		screen_points[0].invW = 2.f;

		screen_points[1].x = (float) screen_width;
		screen_points[1].y = (float) (draw_location.y + line_height - ascending_height);
		screen_points[1].z = 0.5f;
		screen_points[1].invW = 2.f;

		if ((NULL == inUse) || (*inUse)) {
			M3rDraw_State_Commit();
			M3rDraw_Sprite(screen_points, sprite_uv);

			TSrContext_DrawText(ONgStatusLineTextContext, current_line->text, M3cMaxAlpha, NULL, &draw_location);
			draw_location.y += line_height;
		}
	}

	M3rDraw_State_Pop();
}

UUtError COrConsole_StatusLine_LevelBegin(void)
{
	UUtError error;
	TStFontFamily	*fontFamily;

	error = TMrInstance_GetDataPtr(TScTemplate_FontFamily, TScFontFamily_Default,	&fontFamily);
	UUmError_ReturnOnErrorMsg(error, "Could not get font family");

	error = TSrContext_New(fontFamily, TScFontSize_Default, TScStyle_Bold, TSc_HLeft, UUcFalse, &ONgStatusLineTextContext);
	UUmError_ReturnOnErrorMsg(error, "Could not create text context");

	return UUcError_None;
}

void COrConsole_StatusLine_LevelEnd(void)
{
	if (NULL != ONgStatusLineTextContext) {
		TSrContext_Delete(ONgStatusLineTextContext);
		ONgStatusLineTextContext = NULL;
	}

	return;
}