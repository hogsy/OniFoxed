// GME2DAT.c
#include "BFW.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_FileManager.h"
#include "BFW_TemplateManager.h"
#include "BFW_TM_Construction.h"
#include "BFW_Akira.h"
#include "BFW_Path.h"
#include "BFW_AppUtilities.h"
#include "BFW_CommandLine.h"
#include "BFW_Image.h"
#include "BFW_FileFormat.h"
#include "BFW_BitVector.h"

#include "Oni_GameState.h"

#include "TemplateExtractor.h"
#include "Imp_Common.h"
#include "Imp_BatchFile.h"
#include "Imp_Texture.h"
#include "Imp_Character.h"
#include "Imp_Model.h"

#define IMPcMaxNumLevels	(200)

UUtBool IMPgGreen = UUcFalse;
UUtBool IMPgDebugFootsteps = UUcFalse;
UUtBool	gDrawGrid = UUcFalse;
UUtBool IMPgShowHHTs = UUcFalse;
UUtBool IMPgLightmaps = UUcTrue;
UUtBool IMPgBNVDebug = UUcFalse;
UUtBool IMPgForceANDRebuild = UUcFalse;
UUtBool	IMPgLightmap_Output = UUcFalse;
UUtBool	IMPgLightmap_OI = UUcFalse;
UUtBool	IMPgConstructing = UUcTrue;
UUtBool	IMPgLightmap_OutputPrepFile = UUcFalse;
UUtBool	IMPgLightmap_OutputPrepFileOne = UUcFalse;
UUtBool	IMPgBuildTextFiles = UUcFalse;
UUtBool	IMPgLightMap_HighQuality = UUcTrue;
UUtBool IMPgLowMemory = UUcFalse;
UUtBool	IMPgDoPathfinding = UUcTrue;
UUtBool IMPgSuppressErrors = UUcFalse;
UUtUns32 IMPgSuppressedWarnings = 0;
UUtUns32 IMPgSuppressedErrors = 0;
UUtBool IMPgBuildPathDebugInfo = UUcFalse;
UUtBool IMPgPrintFootsteps = UUcFalse;
char *IMPgName = NULL;
UUtBool IMPgNoSound = UUcFalse;
float	IMPgAnimAngle = 1.f;
UUtBool IMPgLightingFromLSFiles = UUcFalse;
UUtBool IMPgSkipAnyKey = UUcFalse;
UUtBool IMPgEnvironmentDebug = UUcFalse;

UUtUns32	IMPgCurLevel;
UUtWindow	gDebugWindow = NULL;

char command_line[2048];

char			*gGameDataFolder = ".\\GameDataFolder";
char			*gAlternateGameDataFolder = "OniEngine\\GameDataFolder";

static void makeCommandLine(int argc, char *argv[])
{
	char *cl_ptr = command_line;
	int itr;

	command_line[0] = '\0';

	for(itr = 1; itr < argc; itr++)
	{
		char *cur_arg = argv[itr];
		int length = strlen(cur_arg);

		strcpy(cl_ptr, cur_arg);
		cl_ptr += length;

		cl_ptr[0] = ' ';
		cl_ptr[1] = '\0';

		cl_ptr += 1;
	}

	return;
}

static void
iParseLevel(
	char*		inArg,
	UUtUns32*	inLevelBV)
{
	UUtError error;

	error = AUrParseNumericalRangeString(inArg,inLevelBV,IMPcMaxNumLevels);
	if (error != UUcError_None) fprintf(stderr,"ERROR: Unable to parse level numbers");
}

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)
extern int handle_exception(LPEXCEPTION_POINTERS);
extern void stack_walk_initialize(void);
#endif

int
main(
	int		argc,
	char*	argv[])

#if 0
{
	UUtUns32 itr;
	FILE *stream;



	for(itr = 1; itr <= 17; itr++)
	{
		char number_string[128];
		char file_name[128];


		if (itr <= 9) {
			sprintf(number_string, "0%d", itr);
		}
		else {
			sprintf(number_string, "%d", itr);
		}



		sprintf(file_name, "level_%s_pictures_bat.txt", number_string);

		stream = fopen(file_name, "w");
		fprintf(stream, "begin\\level%s_intro_ins.txt\n", number_string);
		fprintf(stream, "fail\\level%s_fail_ins.txt\n", number_string);
		fprintf(stream, "win\\level%s_win_ins.txt\n", number_string);
		fclose(stream);
	}
}
#else
{
	UUtError			error;
	BFtFileRef*			instanceFileRef;

	char				fileName[256];
	const char*				leafName;
	char*				temp;
	char				targetName[256];

	BFtFileRef			prefFileRef;
	GRtGroup_Context*	prefGroupContext;
	GRtGroup*			prefGroup;

	char				*dataFileDir;
	char				batchDirFileName[256];

	BFtFileRef*			batchDirFileRef;

	UUtBool				processFinal = UUcFalse;
	UUtBool				addDebugFile = UUcFalse;
	UUtBool				processFinalOnly = UUcFalse;
	UUtBool				dumpStuff = UUcFalse;
	UUtBool				stopAfterTE = UUcFalse;

	UUtBool				runTE = UUcFalse;

	UUtBool				convertToNativeEndiean = UUcFalse;

	UUtUns16			i;
	BFtFileIterator*	fileIterator;

	BFtFileRef			gameDataRef;

	UUtUns32*			targetLevelBV;
	UUtUns32			itr;
	UUtBool				gotLevel = UUcFalse;

	#if defined(UUmCompiler) && (UUmCompiler == UUmCompiler_VisC)
	#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32) && defined(SHIPPING_VERSION) && (SHIPPING_VERSION != 1)
	__try {
	stack_walk_initialize();
	#endif
	#endif

	/*
	 * Initialize the Universal Utilities. This does a base level platform init. So if
	 * this succedes we can bring up error dialogs
	 */
		error =
			UUrInitialize(
				UUcTrue);	// Init basic platform also
		if(error != UUcError_None)
		{
			UUmAssert(!"This is really bad - should never happen");
			exit(1);
		}

	#if UUmPlatform == UUmPlatform_Mac

		runTE = UUcTrue;

		UUgError_HaltOnError = UUcTrue;

	#else

		UUgError_PrintWarning = UUcTrue;

	#endif

	targetLevelBV = UUrBitVector_New(IMPcMaxNumLevels);
	if(targetLevelBV == NULL) return 1;

	#if defined(PROFILE) && PROFILE
		UUrProfile_Initialize();
		UUrProfile_State_Set(UUcProfile_State_Off);
	#endif

	argc = CLrGetCommandLine(argc, argv, &argv);

	UUrProfile_State_Set(UUcProfile_State_On);

	for(i = 1; i < argc; i++) {
		Imp_PrintMessage(IMPcMsg_Important, "%s ", argv[i]);
	}

	Imp_PrintMessage(IMPcMsg_Important, "" UUmNL);

	/*
	 * Parse the command line
	 */

		for(i = 1; i < argc; i++)
		{
			if(strcmp(argv[i], "-final") == 0)
			{
				processFinal = UUcTrue;
			}
			else if (strcmp(argv[i], "-skipanykey") == 0)
			{
				IMPgSkipAnyKey= UUcTrue;
			}
			else if (strcmp(argv[i], "-green") == 0)
			{
				IMPgGreen = UUcTrue;
			}
			else if (strcmp(argv[i], "-debug_footsteps") == 0)
			{
				IMPgDebugFootsteps = UUcTrue;
			}
			else if(strcmp(argv[i], "-txt") == 0)
			{
				IMPgBuildTextFiles = UUcTrue;
			}
			else if(strcmp(argv[i], "-finalOnly") == 0)
			{
				processFinalOnly = UUcTrue;
				processFinal = UUcTrue;
			}
			else if(strcmp(argv[i], "-debug") == 0)
			{
				addDebugFile = UUcTrue;
			}
			else if(strcmp(argv[i], "-grid") == 0)
			{
				gDrawGrid = UUcTrue;
			}
			else if(strcmp(argv[i], "-dumpStuff") == 0)
			{
				dumpStuff = UUcTrue;
			}
			else if(strcmp(argv[i], "-t") == 0)
			{
				runTE = UUcTrue;
			}
			else if(strcmp(argv[i], "-tOnly") == 0)
			{
				runTE = UUcTrue;
				stopAfterTE = UUcTrue;
			}
			else if(strcmp(argv[i], "-hht") == 0)
			{
				IMPgShowHHTs = UUcTrue;
			}
			else if(strcmp(argv[i],"-bnv") == 0)
			{
				IMPgBNVDebug = UUcTrue;
			}
			else if(strcmp(argv[i],"-rebuildAND") == 0)
			{
				IMPgForceANDRebuild = UUcTrue;
			}
			else if(strcmp(argv[i],"-footsteps") == 0)
			{
				IMPgPrintFootsteps = UUcTrue;
			}
			else if(strcmp(argv[i], "-level") == 0)
			{
				i++;
				if((i >= argc) || !isdigit(argv[i][0]))
				{
					goto ohYeaBaby;
				}

				iParseLevel(argv[i], targetLevelBV);

				gotLevel = UUcTrue;
			}
			else if (strcmp(argv[i], "-anim_angle") == 0)
			{
				i++;

				if (i >= argc) {
					goto ohYeaBaby;
				}

				error = UUrString_To_Float(argv[i], &IMPgAnimAngle);

				if (UUcError_None == error) {
					Imp_PrintMessage(IMPcMsg_Important, "animation angle = %f", IMPgAnimAngle);
				}
				else {
					goto ohYeaBaby;
				}

			}
			else if (0 == strcmp(argv[i], "-ehalt"))
			{
				UUgError_HaltOnError = UUcTrue;
			}
			else if (0 == strcmp(argv[i], "-lp1"))
			{
				#if UUmPlatform == UUmPlatform_Mac

					Imp_PrintWarning("-lp is only supported on the PC");
					goto ohYeaBaby;

				#endif

				IMPgLightmap_OutputPrepFile = UUcTrue;
				IMPgLightmap_OutputPrepFileOne = UUcTrue;
				IMPgConstructing = UUcFalse;
				IMPgLightmaps = UUcTrue;
				IMPgLightmap_Output = UUcTrue;
			}
			else if (0 == strcmp(argv[i], "-lmfast"))
			{
				IMPgLightMap_HighQuality = UUcFalse;
			}
			else if (0 == strcmp(argv[i], "-lm"))
			{
				IMPgLightingFromLSFiles = UUcTrue;
			}
			else if (0 == strcmp(argv[i], "-quiet"))
			{
				IMPgMin_Importance = IMPcMsg_Critical;
			}
			else if (0 == strcmp(argv[i], "-noisy"))
			{
				IMPgMin_Importance = IMPcMsg_Cosmetic;
			}
			else if (0 == strcmp(argv[i], "-ignore"))
			{
				IMPgSuppressErrors = UUcTrue;
			}
			else if (0 == strcmp(argv[i], "-convert"))
			{
				convertToNativeEndiean = UUcTrue;
			}
			else if (0 == strcmp(argv[i], "-select"))
			{
				char *internal;
				char *cur_select;

				i++;

				Imp_BatchFile_Activate_All(UUcFalse);

				for(cur_select = UUrString_Tokenize(argv[i], ",", &internal);
					cur_select != NULL;
					cur_select = UUrString_Tokenize(NULL, ",", &internal))
				{
					error = Imp_BatchFile_Activate_Type(cur_select, UUcTrue);

					if (error != UUcError_None) {
						Imp_PrintWarning("invalid template type %s.", cur_select);
					}
				}
			}
			else if (0 == strcmp(argv[i], "-unselect"))
			{
				char *internal;
				char *cur_select;

				i++;

				for(cur_select = UUrString_Tokenize(argv[i], ",", &internal);
					cur_select != NULL;
					cur_select = UUrString_Tokenize(NULL, ",", &internal))
				{
					error = Imp_BatchFile_Activate_Type(cur_select, UUcFalse);

					if (error != UUcError_None) {
						Imp_PrintWarning("invalid template type %s.", cur_select);
					}
				}
			}
			else if (0 == strcmp(argv[i], "-name"))
			{
				i++;

				IMPgName = argv[i];
			}
			else if (0 == strcmp(argv[i], "-anim_compress"))
			{
				i++;
				error = UUrString_To_Uns8(argv[i], &IMPgDefaultCompressionSize);

				if (UUcError_None != error) {
					Imp_PrintWarning("could not parse anim compression value %s", argv[i]);
				}

				switch(IMPgDefaultCompressionSize)
				{
					case 4:
					case 6:
					case 8:
					case 16:
					break;

					default:
						Imp_PrintWarning("could not parse anim compression size %d (4,6,8,16 are valid) ", IMPgDefaultCompressionSize);
						IMPgDefaultCompressionSize = 8;
					break;
				}
			}
			else if (0 == strcmp(argv[i], "-lowmem")) {
				IMPgLowMemory = UUcTrue;
			}
			else if (0 == strcmp(argv[i], "-noai")) {
				IMPgDoPathfinding = UUcFalse;
			}
			else if (0 == strcmp(argv[i], "-nosound")) {
				IMPgNoSound = UUcTrue;
			}
			else if (0 == strcmp(argv[i], "-pathdebug")) {
				IMPgBuildPathDebugInfo = UUcTrue;
			}
			else if (0 == strcmp(argv[i], "-nodialog")) {
				// this parameter is handled by BFW_CommandLine at a prior point
			}
			else if (0 == strcmp(argv[i], "-macsound")) {
				SSgCompressionMode = SScCompressionMode_Mac;
			}
			else if (0 == strcmp(argv[i], "-envdebug")) {
				IMPgEnvironmentDebug = UUcTrue;
			}
			else
			{
				goto ohYeaBaby;
			}
		}

		goto iMissBasic;
ohYeaBaby:
			Imp_PrintWarning("%s [-nodialog] [-lightmaps] [-final] [-finalOnly] [-debug] [-grid] [-dumpStuff] [-t] [-tOnly] [-level num]"UUmNL, argv[0]);
			fprintf(stdout, "done, press any key that is labeled enter");
			fgetc(stdin);
			return -1;

iMissBasic:

	/*
	 * Initialize the template management system
	 */
		Imp_PrintMessage(IMPcMsg_Cosmetic, "Finding the game data folder"UUmNL);
		error =
			BFrFileRef_Search(
				gGameDataFolder,
				&gameDataRef);
		if (error != UUcError_None) {
			// CB: could not find gamedatafolder under working directory, maybe we are running
			// from inside the development environment and have our working directory set elsewhere.
			// try a recursive search for OniEngine\GameDataFolder           - 20 september 2000
			error =
				BFrFileRef_Search(
					gAlternateGameDataFolder,
					&gameDataRef);
			UUmError_ReturnOnErrorMsg(error, "Could not find GameDataFolder directory");
		}

		error = TMrInitialize(UUcFalse, &gameDataRef);
		UUmError_ReturnOnError(error);

	if(convertToNativeEndiean)
	{
		TMrConstruction_ConvertToNativeEndian();
		goto doneConverting;
	}

	if(gotLevel == UUcFalse)
	{
		UUrBitVector_SetBitAll(targetLevelBV, IMPcMaxNumLevels);
	}

	if(runTE)
	{
		Imp_PrintMessage(IMPcMsg_Cosmetic, "Running the template extractor"UUmNL);
		error = TErRun();
		if (UUcError_None != error) {
			Imp_PrintWarning("Errors with the template extractor");
		}

		if (stopAfterTE) exit(0);
	}

	Imp_PrintMessage(IMPcMsg_Cosmetic, "Imp_Initialize"UUmNL);
	error = Imp_Initialize();
	UUmError_ReturnOnError(error);

	if(0)
	{
		UUtRect	rect;
		rect.top = 20;
		rect.left = 20;
		rect.bottom = rect.top + 1024;
		rect.right = rect.left + 1024;

		gDebugWindow = AUrWindow_New(&rect);
	}

	Imp_PrintMessage(IMPcMsg_Cosmetic, "Finding Preferences.txt"UUmNL);
	error = BFrFileRef_Search("Preferences.txt", &prefFileRef);
	UUmError_ReturnOnErrorMsg(error, "Could not find preferences file");

	Imp_PrintMessage(IMPcMsg_Cosmetic, "Making the preferences group"UUmNL);
	error =
		GRrGroup_Context_NewFromFileRef(
			&prefFileRef,
			NULL,
			NULL,
			&prefGroupContext,
			&prefGroup);
	UUmError_ReturnOnErrorMsg(error, "Could not make preferences group");

	gPreferencesGroup = prefGroup;

	Imp_PrintMessage(IMPcMsg_Cosmetic, "Looking up DataFileDir"UUmNL);
	error = GRrGroup_GetString(prefGroup, "DataFileDir", &dataFileDir);
	UUmError_ReturnOnErrorMsg(error, "Could not find DataFileDir in preferences.txt");
	sprintf(batchDirFileName, "%s\\BatchFiles", dataFileDir);

	Imp_PrintMessage(IMPcMsg_Cosmetic, "Finding the batch file directory"UUmNL);
	error = BFrFileRef_MakeFromName(batchDirFileName, &batchDirFileRef);
	UUmError_ReturnOnErrorMsg(error, "Could not find OniBatchFiles directory");

	/*
	 * Initialize importers that need it
	 */
		error = Imp_Character_Initialize();
		UUmError_ReturnOnError(error);


	/*
	 * Loop through all the _bat files
	 */
	Imp_PrintMessage(IMPcMsg_Cosmetic, "Building the batch file directory iterator"UUmNL);
	if (!BFrFileRef_FileExists(batchDirFileRef)) {
		Imp_PrintWarning("The batch file directory (%s) could not be found", BFrFileRef_GetLeafName(batchDirFileRef));
	}

	// tell user what levels will be processed
		Imp_PrintMessage(IMPcMsg_Important, "Building levels: ");
		for(itr = 0; itr < IMPcMaxNumLevels; itr++)
		{
			if(UUrBitVector_TestBit(targetLevelBV, itr))
			{
				Imp_PrintMessage(IMPcMsg_Important, "%d ", itr);
			}
		}
		Imp_PrintMessage(IMPcMsg_Important, UUmNL);

	error =
		BFrDirectory_FileIterator_New(
			batchDirFileRef,
			"Level",
			"_bat.txt",
			&fileIterator);
	UUmError_ReturnOnErrorMsg(error, "Could not create iterator");

	Imp_PrintMessage(IMPcMsg_Cosmetic, "Looping through the batch files"UUmNL);
	while(1)
	{
		BFtFileRef batchFileRef;

		error = BFrDirectory_FileIterator_Next(fileIterator, &batchFileRef);
		if(error != UUcError_None)
		{
			break;
		}

		leafName = BFrFileRef_GetLeafName(&batchFileRef);
		Imp_PrintMessage(IMPcMsg_Cosmetic, "batch file %s"UUmNL, leafName);

		temp = strchr(leafName, '_');
		if(temp == NULL)
		{
			fprintf(stderr, "WARNING: Illegal batch file name: %s\n", leafName);
			continue;
		}

		sscanf(leafName + 5, "%d", &IMPgCurLevel);

		if(IMPgCurLevel >= TMcMaxLevelNum)
		{
			fprintf(stderr, "ERROR: Illegal level number: %s\n", leafName);
			continue;
		}

		UUrString_Copy(targetName, temp + 1, 256);

		temp = strrchr(targetName, '_');
		if(temp == NULL)
		{
			fprintf(stderr, "WARNING: Illegal batch file name: %s\n", leafName);
			continue;
		}

		*temp = 0;

		if(!UUrBitVector_TestBit(targetLevelBV, IMPgCurLevel))
		{
			continue;
		}

		if(strcmp(targetName, "Final") == 0)
		{
			if(processFinal == UUcFalse)
			{
				continue;
			}
		}
		else if(processFinalOnly == UUcTrue)
		{
			continue;
		}

		/*
		 * Load the .dat instance file, create if needed
		 */

			if (IMPgName != NULL) {
				sprintf(fileName, "level%d_%s.dat", IMPgCurLevel, IMPgName);
			}
			else {
				sprintf(fileName, "level%d_%s.dat", IMPgCurLevel, targetName);
			}

			Imp_PrintMessage(IMPcMsg_Important, "Starting construction on %s."UUmNL, fileName);

			error =
				BFrFileRef_DuplicateAndAppendName(
					&gameDataRef,
					fileName,
					&instanceFileRef);
			UUmError_ReturnOnError(error);

			if(IMPgLightmap_OI)
			{
				IMPgLightmap_Output = UUcTrue;
				IMPgConstructing = UUcFalse;
			}

goAgain:

			if(IMPgConstructing)
			{
				error = TMrConstruction_Start(instanceFileRef);
				UUmError_ReturnOnError(error);
			}

		/*
		 * Process the final instances
		 */
			Imp_PrintMessage(IMPcMsg_Important, "processing"UUmNL);

			error = Imp_BatchFile_Process(&batchFileRef);
			UUmError_ReturnOnError(error);

		/*
		 * Save and dispose
		 */
			Imp_PrintMessage(IMPcMsg_Important, "stopping and disposing"UUmNL);

			if(IMPgConstructing)
			{
				TMrConstruction_Stop(dumpStuff);
			}

		// go again if we need to for lightmaps
			if(IMPgLightmap_OI == UUcTrue && IMPgLightmap_Output == UUcTrue)
			{
				IMPgLightmap_Output = UUcFalse;
				IMPgConstructing = UUcTrue;
				goto goAgain;
			}

			BFrFileRef_Dispose(instanceFileRef);
	}

	BFrDirectory_FileIterator_Delete(fileIterator);

	Imp_PrintMessage(IMPcMsg_Important, "done looping "UUmNL, leafName);

	Imp_WriteTextureSize();

	GRrGroup_Context_Delete(prefGroupContext);

	BFrFileRef_Dispose(batchDirFileRef);

	if (IMPgSuppressedWarnings || IMPgSuppressedErrors) {
		Imp_PrintMessage(IMPcMsg_Important, "********** %d warnings and %d errors were suppressed, check import_log.txt",
							IMPgSuppressedWarnings, IMPgSuppressedErrors);
	}

	Imp_Terminate();

doneConverting:

	TMrTerminate();

	if(gDebugWindow != NULL) AUrWindow_Delete(gDebugWindow);

	#if defined(PROFILE) && PROFILE
		UUrProfile_State_Set(UUcProfile_State_Off);
		UUrProfile_Dump("ImpProfile");
		UUrProfile_Terminate();
	#endif

	UUrBitVector_Dispose(targetLevelBV);

	IMrTerminate();

	UUrTerminate();

	#if ((UUmPlatform == UUmPlatform_Win32) && (UUmCompiler != UUmCompiler_MWerks))

		fprintf(stdout, "done, press any key that is labeled enter");
		if (!IMPgSkipAnyKey)  {
			fgetc(stdin);
		}
	#endif

	#if defined(UUmCompiler) && (UUmCompiler == UUmCompiler_VisC)
	#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32) && defined(SHIPPING_VERSION) && (SHIPPING_VERSION != 1)
	} // end of try block
	__except (handle_exception(GetExceptionInformation())) {}
	#endif
	#endif

	return 0;
}
#endif

#if 0
int WINAPI WinMain(
	HINSTANCE	hInstance,
	HINSTANCE	hPrevInstance,
	PSTR		ssCmdLine,
	int			iCmdShow)
{
//	MSG	msg;
#define iMaxArguments 20
	int argc;
	char *argv[iMaxArguments + 1];

	argv[0] = "";

	AllocConsole();

	AUrBuildArgumentList(ssCmdLine, iMaxArguments, (UUtUns32*)&argc, argv + 1);
	main(argc + 1, argv);

	return 0;
}
#endif

// this is in Oni.c, and called in Oni_OutGameUI.c, but we can't include Oni.c in the importer project
void OniExit(
	void)
{
	return;
}
