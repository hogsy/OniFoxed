/*
	FILE:	Oni_Motoko.c
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 28, 1998
	
	PURPOSE: Motoko stuff for Oni
	
	Copyright 1998

*/

#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Console.h"
#include "BFW_ScriptLang.h"

#include "Oni.h"
#include "Oni_GameState.h"
#include "Oni_Platform.h"
#include "Oni_Motoko.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Persistance.h"

#include "Motoko_Manager.h"
#include "Motoko_Private.h"


UUtInt32 ONgMotoko_ClearColor = 0;
UUtBool ONgMotoko_ShadeVertex = UUcTrue;
UUtBool ONgMotoko_FillSolid = UUcTrue;
UUtBool ONgMotoko_Texture = UUcTrue;
UUtBool ONgMotoko_ZCompareOn = UUcTrue;
UUtBool ONgMotoko_DoubleBuffer = UUcTrue;
UUtBool ONgMotoko_BufferClear = UUcTrue;

float	ONgMotoko_FieldOfView = (45.f * (M3c2Pi / 360.f));
float	ONgMotoko_FarPlane;

UUtUns16	ONgMotoko_ActiveGeomEngineIndex = 0;
UUtUns16	ONgMotoko_ActiveDrawEngineIndex = 0;
UUtUns16	ONgMotoko_ActiveDeviceIndex = 0;
UUtUns16	ONgMotoko_ActiveModeIndex = 0;

static UUtError
ONiMotoko_DrawEngine_List(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16			numEngines;
	UUtUns16			itr;
	M3tDrawEngineCaps*	curCaps;
	
	numEngines = M3rDrawEngine_GetNumber();
	
	for(itr = 0; itr < numEngines; itr++)
	{
		curCaps = M3rDrawEngine_GetCaps(itr);
		
		COrConsole_Printf("%s%d: name: %s", ONgMotoko_ActiveDrawEngineIndex == itr ? "*" : "" ,itr, curCaps->engineName);
		COrConsole_Printf("%s%d: driver: %s", ONgMotoko_ActiveDrawEngineIndex == itr ? "*" : "", itr, curCaps->engineDriver);
		COrConsole_Printf("%s%d: version: %d", ONgMotoko_ActiveDrawEngineIndex == itr ? "*" : "", itr, curCaps->engineVersion);
		COrConsole_Printf("%s%d: numDevices: %d", ONgMotoko_ActiveDrawEngineIndex == itr ? "*" : "", itr, curCaps->numDisplayDevices);
	}
	
	return UUcError_None;
}

static UUtError
ONiMotoko_Engine_Set(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16			newGeomEngineIndex;
	UUtUns16			newDrawEngineIndex;
	M3tGeomEngineCaps*	curCaps;
	
	newGeomEngineIndex = (UUtUns16)inParameterList[0].val.i;
	newDrawEngineIndex = (UUtUns16)inParameterList[1].val.i;
	
	if(newGeomEngineIndex < M3rGeomEngine_GetNumber() && newDrawEngineIndex < M3rDrawEngine_GetNumber())
	{
	
		// make sure that the draw and geom engine are compatable
			curCaps = M3rGeomEngine_GetCaps(newGeomEngineIndex);
			if(((1 << newDrawEngineIndex) & curCaps->compatibleDrawEngineBV) == 0)
			{
				COrConsole_Printf("draw engine and geom engine are not compatable");
				return UUcError_None;
			}
		
		ONgMotoko_ActiveGeomEngineIndex = newGeomEngineIndex;
		ONgMotoko_ActiveDrawEngineIndex = newDrawEngineIndex;
		
		UUrMemory_Leak_ForceGlobal_Begin();
		
		M3rGeomContext_SetEnvironment(
			NULL);
		ONrMotoko_TearDownDrawing();
		ONrMotoko_SetupDrawing(&ONgPlatformData);
		M3rGeomContext_SetEnvironment(
			ONgGameState->level->environment);

		UUrMemory_Leak_ForceGlobal_End();
	}

	return UUcError_None;
}

static UUtError
ONiMotoko_GeomEngine_List(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16			numEngines;
	UUtUns16			itr;
	M3tGeomEngineCaps*	curCaps;
	
	numEngines = M3rGeomEngine_GetNumber();
	
	for(itr = 0; itr < numEngines; itr++)
	{
		curCaps = M3rGeomEngine_GetCaps(itr);
		
		COrConsole_Printf("%s%d: name: %s", ONgMotoko_ActiveGeomEngineIndex == itr ? "*" : "" ,itr, curCaps->engineName);
		COrConsole_Printf("%s%d: driver: %s", ONgMotoko_ActiveGeomEngineIndex == itr ? "*" : "", itr, curCaps->engineDriver);
		COrConsole_Printf("%s%d: version: %d", ONgMotoko_ActiveGeomEngineIndex == itr ? "*" : "", itr, curCaps->engineVersion);
		COrConsole_Printf("%s%d: compatableBV: %d", ONgMotoko_ActiveGeomEngineIndex == itr ? "*" : "", itr, curCaps->compatibleDrawEngineBV);
	}

	return UUcError_None;
}

static UUtError
ONiMotoko_Display_List(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16			itrDevice;
	UUtUns16			itrMode;
	M3tDrawEngineCaps*	curCaps;

	curCaps = M3rDrawEngine_GetCaps(ONgMotoko_ActiveDrawEngineIndex);

	for(itrDevice = 0; itrDevice < curCaps->numDisplayDevices; itrDevice++)
	{
		COrConsole_Printf("Device %d: numModes = %d", itrDevice, curCaps->displayDevices[itrDevice].numDisplayModes);
		
		for(itrMode = 0; itrMode < curCaps->displayDevices[itrDevice].numDisplayModes; itrMode++)
		{
			COrConsole_Printf(
				"Device %d: mode[%d]: width: %d, height: %d, bitdepth: %d",
				itrDevice,
				itrMode,
				curCaps->displayDevices[itrDevice].displayModes[itrMode].width,
				curCaps->displayDevices[itrDevice].displayModes[itrMode].height,
				curCaps->displayDevices[itrDevice].displayModes[itrMode].bitDepth);
		}
	}

	return UUcError_None;
}

static UUtError
ONiMotoko_Display_Set(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool				*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	UUtUns16	newDeviceIndex;
	UUtUns16	newModeIndex;
	M3tDrawEngineCaps*	curCaps;
	
	newDeviceIndex = (UUtUns16)inParameterList[0].val.i;
	newModeIndex = (UUtUns16)inParameterList[1].val.i;
	
	curCaps = M3rDrawEngine_GetCaps(ONgMotoko_ActiveDrawEngineIndex);

	if(newDeviceIndex < curCaps->numDisplayDevices &&
		newModeIndex < curCaps->displayDevices[newDeviceIndex].numDisplayModes)
	{
		ONgMotoko_ActiveDeviceIndex = newDeviceIndex;
		ONgMotoko_ActiveModeIndex = newModeIndex;

		UUrMemory_Leak_ForceGlobal_Begin();

		M3rGeomContext_SetEnvironment(
			NULL);
		ONrMotoko_TearDownDrawing();
		ONrMotoko_SetupDrawing(&ONgPlatformData);
		M3rGeomContext_SetEnvironment(
			ONgGameState->level->environment);

		UUrMemory_Leak_ForceGlobal_End();
	}

	return UUcError_None;
}

static UUtError
ONiMotoko_Quality_Set(
	SLtErrorContext*	inErrorContext,
	UUtUns32			inParameterListLength,
	SLtParameter_Actual*		inParameterList,
	UUtUns32			*outTicksTillCompletion,
	UUtBool				*outStall,				
	SLtParameter_Actual		*ioReturnValue)
{
	typedef struct {
		char *name;
		ONtGraphicsQuality quality;
	} tGraphicsQualityName;

	static tGraphicsQualityName quality_table[] = {
		{"superlow",	ONcGraphicsQuality_SuperLow},
		{"low",			ONcGraphicsQuality_Low},
		{"medium",		ONcGraphicsQuality_Medium},
		{"high",		ONcGraphicsQuality_High},
		{"superhigh",	ONcGraphicsQuality_SuperHigh},
		{NULL,			0}
	};
	tGraphicsQualityName *tableptr;

	for (tableptr = quality_table; tableptr->name != NULL; tableptr++) {
		if (UUmString_IsEqual(tableptr->name, inParameterList[0].val.str)) {
			if ((tableptr->quality >= ONcGraphicsQuality_Min) && (tableptr->quality <= ONcGraphicsQuality_Max)) {
				break;
			}
		}
	}
	if (tableptr->name == NULL) {
		COrConsole_Printf("can't find graphics quality setting '%s'", inParameterList[0].val.str);
	} else {
		COrConsole_Printf("graphics quality changed to %s", tableptr->name);
		ONrPersist_SetGraphicsQuality(tableptr->quality);
	}

	return UUcError_None;
}

static UUtBool ONrMotoko_SetResolution_Internal(M3tDisplayMode *inResolution)
{
	M3tGeomEngineCaps*	geomEngineCaps;
	M3tDrawEngineCaps*	drawEngineCaps;
	UUtUns16			modeItr;
	M3tDisplayMode*		curDisplayMode;
	UUtUns16			itrDrawEngine;
	UUtBool				resolution_set = UUcFalse;
	
	geomEngineCaps = M3rGeomEngine_GetCaps(ONgMotoko_ActiveGeomEngineIndex);
	
	for(itrDrawEngine = 0; itrDrawEngine < M3cMaxNumEngines; itrDrawEngine++)
	{
		if((1 << itrDrawEngine) & geomEngineCaps->compatibleDrawEngineBV) break;
	}
	
	UUmAssert(itrDrawEngine < M3cMaxNumEngines);
	
	ONgMotoko_ActiveDrawEngineIndex = itrDrawEngine;
	
	drawEngineCaps = M3rDrawEngine_GetCaps(ONgMotoko_ActiveDrawEngineIndex);
	
	for(modeItr = 0; modeItr < drawEngineCaps->displayDevices[0].numDisplayModes; modeItr++)
	{
		curDisplayMode = &drawEngineCaps->displayDevices[0].displayModes[modeItr];
		if ((curDisplayMode->bitDepth == inResolution->bitDepth) && (curDisplayMode->width == inResolution->width) && (curDisplayMode->height == inResolution->height))
		{
			resolution_set = UUcTrue;
			break;
		}
	}
	
	if (modeItr < drawEngineCaps->displayDevices[0].numDisplayModes)
	{
		ONgMotoko_ActiveModeIndex = modeItr;
	}

	M3rDrawEngine_MakeActive(ONgMotoko_ActiveDrawEngineIndex, ONgMotoko_ActiveDeviceIndex, UUcFalse, ONgMotoko_ActiveModeIndex);
	
	return resolution_set;
}

UUtBool ONrMotoko_SetResolution(M3tDisplayMode *inResolution)
{
	UUtBool changed;

	changed = ONrMotoko_SetResolution_Internal(inResolution);

	if (changed) {
		M3rDraw_SetResolution(*inResolution);
	}

	return changed;
}

UUtError
ONrMotoko_Initialize(
	void)
{
	UUtError			error;

	error;

#if CONSOLE_DEBUGGING_COMMANDS
	error = SLrGlobalVariable_Register_Int32("m3_clear_color", "color to clear the back buffer to", &ONgMotoko_ClearColor);
	error =	SLrGlobalVariable_Register_Bool("m3_shade_vertex", "Enables vertex interpolation for the triangle", &ONgMotoko_ShadeVertex);
	error =	SLrGlobalVariable_Register_Bool("m3_fill_solid", "Toggles wireframe vs filled triangles", &ONgMotoko_FillSolid);
	error =	SLrGlobalVariable_Register_Bool("m3_texture", "Toggles texture mapping or gouraud", &ONgMotoko_Texture);
	error =	SLrGlobalVariable_Register_Bool("m3_zcompareon", "true if zcompare is on", &ONgMotoko_ZCompareOn);
	error =	SLrGlobalVariable_Register_Bool("m3_double_buffer", "Toggles double buffer mode", &ONgMotoko_DoubleBuffer);
	error =	SLrGlobalVariable_Register_Bool("m3_buffer_clear", "Toggles buffer clear", &ONgMotoko_BufferClear);

	error = 
		SLrScript_Command_Register_Void(
			"m3_draw_engine_list",
			"lists all the engines",
			"",
			ONiMotoko_DrawEngine_List);
	UUmError_ReturnOnError(error);

	error = 
		SLrScript_Command_Register_Void(
			"m3_geom_engine_list",
			"lists all the engines",
			"",
			ONiMotoko_GeomEngine_List);
	UUmError_ReturnOnError(error);

	error = 
		SLrScript_Command_Register_Void(
			"m3_engine_set",
			"sets the active engine",
			"geom_engine:int draw_engine:int",
			ONiMotoko_Engine_Set);
	UUmError_ReturnOnError(error);
	
	error = 
		SLrScript_Command_Register_Void(
			"m3_display_list",
			"lists all the display modes",
			"",
			ONiMotoko_Display_List);
	UUmError_ReturnOnError(error);
	
	error = 
		SLrScript_Command_Register_Void(
			"m3_display_set",
			"sets the active display mode",
			"device_index:int mode_index:int",
			ONiMotoko_Display_Set);
	UUmError_ReturnOnError(error);
	
	error = 
		SLrScript_Command_Register_Void(
			"m3_quality_set",
			"sets the current graphics quality",
			"quality:string",
			ONiMotoko_Quality_Set);
	UUmError_ReturnOnError(error);
#endif

	{
		M3tDisplayMode resolution = ONrPersist_GetResolution();
		UUtBool set_resolution;

		set_resolution = ONrMotoko_SetResolution_Internal(&resolution);

		if (!set_resolution) {
                        memset(&resolution, 0, sizeof(resolution));
			resolution.bitDepth = 16;
			resolution.width = 640;
			resolution.height = 480;
                            

			ONrMotoko_SetResolution_Internal(&resolution);
		}
	}
	
	return UUcError_None;
}

void
ONrMotoko_Terminate(
	void)
{

}

// XXX - This still needs lots o work
UUtError
ONrMotoko_SetupDrawing(
	ONtPlatformData*	inPlatformData)
{
	UUtError					error;
	M3tDrawContextDescriptor	contextDescriptor;
	M3tDrawEngineCaps*			drawEngineCaps;
	UUtUns16					targetHeight;
	UUtUns16					targetWidth;
	M3tDisplayMode*				targetDisplayMode;
	
	UUrStartupMessage("setting up 3d engine...");
	
	drawEngineCaps = M3rDrawEngine_GetCaps(ONgMotoko_ActiveDrawEngineIndex);
	targetDisplayMode = &drawEngineCaps->displayDevices[ONgMotoko_ActiveDeviceIndex].displayModes[ONgMotoko_ActiveModeIndex];
	targetWidth = targetDisplayMode->width;
	targetHeight = targetDisplayMode->height;
	
	M3rDrawEngine_MakeActive(ONgMotoko_ActiveDrawEngineIndex, ONgMotoko_ActiveDeviceIndex, UUcFalse, ONgMotoko_ActiveModeIndex);
	M3rGeomEngine_MakeActive(ONgMotoko_ActiveGeomEngineIndex);
	
	contextDescriptor.type = M3cDrawContextType_OnScreen;
	
	contextDescriptor.drawContext.onScreen.appInstance = inPlatformData->appInstance;
	contextDescriptor.drawContext.onScreen.window = inPlatformData->gameWindow;
	contextDescriptor.drawContext.onScreen.rect.top = 20;
	contextDescriptor.drawContext.onScreen.rect.left = 0;
	contextDescriptor.drawContext.onScreen.rect.bottom = 20 + targetHeight;
	contextDescriptor.drawContext.onScreen.rect.right = targetWidth;
	
	error = M3rGeomContext_New(&contextDescriptor);
	UUmError_ReturnOnError(error);

	M3rDraw_State_SetInt(M3cDrawStateIntType_Appearence, M3cDrawState_Appearence_Gouraud);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Interpolation,	M3cDrawState_Interpolation_None);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Fill, M3cDrawState_Fill_Solid);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZCompare, M3cDrawState_ZCompare_On);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ZWrite, M3cDrawState_ZWrite_On);
	M3rDraw_State_SetInt(M3cDrawStateIntType_NumRealVertices, 0);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ConstantColor, IMcShade_White);
	M3rDraw_State_SetInt(M3cDrawStateIntType_SubmitMode, M3cDrawState_SubmitMode_Normal);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Alpha, 0xff);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Time, 0);
	M3rDraw_State_SetInt(M3cDrawStateIntType_BufferClear, M3cDrawState_BufferClear_On);
	M3rDraw_State_SetInt(M3cDrawStateIntType_DoubleBuffer, M3cDrawState_DoubleBuffer_On);
	M3rDraw_State_SetInt(M3cDrawStateIntType_ClearColor, IMcShade_Black);
	M3rDraw_State_SetInt(M3cDrawStateIntType_Fog, M3cDrawStateFogEnable); // S.S.

	M3rDraw_State_Commit();

	ONrGameState_PrepareGeometryEngine();
	
	COrConfigure();
	
	return UUcError_None;
}

void
ONrMotoko_TearDownDrawing(
	void)
{
	ONrGameState_TearDownGeometryEngine();
	M3rGeomContext_Delete();
}

UUtInt32 ONrMotoko_GraphicsQuality_CharacterPolygonCount(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtInt32 polygon_count;
	UUtInt32 polygon_count_table[5] = 
	{
		1500,
		2200,
		3000,
		3400,
		4000		
	};

	quality = UUmPin(quality, 0, 4);

	polygon_count = polygon_count_table[quality];

	return polygon_count;
}

UUtInt32 ONrMotoko_GraphicsQuality_RayCastCount(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtInt32 raycast_count;
	UUtInt32 raycast_count_table[5] = 
	{
		1,
		1,
		1,
		1,
		2		
	};

	quality = UUmPin(quality, 0, 4);

	raycast_count = raycast_count_table[quality];

	return raycast_count;
}

UUtInt32 ONrMotoko_GraphicsQuality_NumDirectionalLights(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtInt32 directional_light_count;
	UUtInt32 directional_light_count_table[5] = 
	{
		1,
		1,
		2,
		2,
		2		
	};

	quality = UUmPin(quality, 0, 4);

	directional_light_count = directional_light_count_table[quality];

	return directional_light_count;
}


UUtInt32 ONrMotoko_GraphicsQuality_RayCount(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtInt32 ray_count;
	UUtInt32 ray_count_table[5] = 
	{
		16,
		18,
		20,
		20,
		20		
	};

	quality = UUmPin(quality, 0, 4);

	ray_count = ray_count_table[quality];

	return ray_count;
}

UUtBool ONrMotoko_GraphicsQuality_SupportShadows(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtBool support_shadows = quality >= ONcGraphicsQuality_Low;

	return support_shadows;
}

UUtBool ONrMotoko_GraphicsQuality_SupportTrilinear(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtBool support_trilinear = quality >= ONcGraphicsQuality_Medium;

	return support_trilinear;
}

UUtBool ONrMotoko_GraphicsQuality_SupportReflectionMapping(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtBool support_reflection_mapping = quality >= ONcGraphicsQuality_Low;

	return support_reflection_mapping;
}

UUtBool ONrMotoko_GraphicsQuality_SupportHighQualityTextures(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtBool support_high_quality_textures = quality >= ONcGraphicsQuality_Medium;

	return support_high_quality_textures;
}

UUtBool ONrMotoko_GraphicsQuality_SupportCosmeticCorpses(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtBool support_cosmetic_corpses = quality >= ONcGraphicsQuality_Low;

	return support_cosmetic_corpses;
}

UUtBool ONrMotoko_GraphicsQuality_SupportHighQualityCorpses(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtBool support_high_quality_corpses = quality >= ONcGraphicsQuality_High;

	return support_high_quality_corpses;
}

UUtBool ONrMotoko_GraphicsQuality_NeverUseSuperLow(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtBool never_use_super_low = quality >= ONcGraphicsQuality_SuperHigh;

	return never_use_super_low;
}

UUtBool ONrMotoko_GraphicsQuality_HardwareBink(void)
{
	ONtGraphicsQuality quality = ONrPersist_GetGraphicsQuality();
	UUtBool hardware_bink = quality >= ONcGraphicsQuality_SuperHigh;

	return hardware_bink;
}

