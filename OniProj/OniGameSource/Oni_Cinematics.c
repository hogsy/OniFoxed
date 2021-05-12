// ======================================================================
// Oni_Cinematics.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_Mathlib.h"
#include "BFW_ScriptLang.h"
#include "WM_PartSpecification.h"

#include "Oni_Cinematics.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"

// ======================================================================
// defines
// ======================================================================
#define OCcNumStartPositions	25

// ======================================================================
// enums
// ======================================================================
typedef enum OCtPart
{
	OCcPart_None				= 0x0000,

	OCcPart_Top					= 0x0001,
	OCcPart_Row_Center			= 0x0002,
	OCcPart_Bottom				= 0x0004,

	OCcPart_Left				= 0x0010,
	OCcPart_Col_Center			= 0x0020,
	OCcPart_Right				= 0x0040,

	OCcPart_TopLeft				= (OCcPart_Top | OCcPart_Left),
	OCcPart_TopCenter			= (OCcPart_Top | OCcPart_Col_Center),
	OCcPart_TopRight			= (OCcPart_Top | OCcPart_Right),
	
	OCcPart_CenterLeft			= (OCcPart_Row_Center | OCcPart_Left),
	OCcPart_CenterCenter		= (OCcPart_Row_Center | OCcPart_Col_Center),
	OCcPart_CenterRight			= (OCcPart_Row_Center | OCcPart_Right),

	OCcPart_BottomLeft			= (OCcPart_Bottom | OCcPart_Left),
	OCcPart_BottomCenter		= (OCcPart_Bottom | OCcPart_Col_Center),
	OCcPart_BottomRight			= (OCcPart_Bottom | OCcPart_Right)

} OCtPart;

enum
{
	OCcCinematicFlag_None		= 0x0000,
	OCcCinematicFlag_Stop		= 0x0001,
	OCcCinematicFlag_Delete		= 0x0002,
	OCcCinematicFlag_AtEnd		= 0x0004,
	OCcCinematicFlag_Mirror		= 0x0010
};

#define OCcCinematic_ScreenWidth	1024
#define OCcCinematic_ScreenHeight	768

// CB: cinematics do not use the whole screen so that they don't obscure
// subtitles on the bottom letterboxed area
#define OCcCinematic_Width			OCcCinematic_ScreenWidth
#define OCcCinematic_Height			(OCcCinematic_ScreenHeight - 40)

// ======================================================================
// typedefs
// ======================================================================
typedef struct OCtCinematic
{
	UUtUns32					flags;
	
	M3tTextureMap				*texture;
	UUtInt16					draw_width;
	UUtInt16					draw_height;
	
	M3tPointScreen				position;
	M3tPoint3D					delta;
	UUtUns32					steps;
	UUtUns8						start;
	UUtUns8						end;
		
} OCtCinematic;

typedef enum OCtMod
{
	OCcMod_None,
	OCcMod_Add,
	OCcMod_Sub

} OCtMod;

typedef struct OCtIndexPositions
{
	OCtMod						x_mod;
	OCtMod						y_mod;
	UUtBool						add_screen_width;
	UUtBool						add_screen_height;
	OCtPart						part;
	
} OCtIndexPositions;

// ======================================================================
// globals
// ======================================================================
static UUtMemory_Array			*ONgCinematics;
static UUtUns32					ONgCinematicRef;

static PStPartSpec				*OCgBorder;

static OCtIndexPositions		ONgIndexPositions[26] =
{
	{ OCcMod_None,	OCcMod_None,	UUcFalse,	UUcFalse,	OCcPart_None },			// 0
	
	{ OCcMod_Add,	OCcMod_Add,		UUcFalse,	UUcFalse,	OCcPart_TopLeft },		// 1
	{ OCcMod_None,	OCcMod_Add,		UUcFalse,	UUcFalse,	OCcPart_TopCenter },	// 2
	{ OCcMod_Sub,	OCcMod_Add,		UUcTrue,	UUcFalse,	OCcPart_TopRight },		// 3
	{ OCcMod_Add,	OCcMod_None,	UUcFalse,	UUcFalse,	OCcPart_CenterLeft },	// 4
	{ OCcMod_None,	OCcMod_None,	UUcFalse,	UUcFalse,	OCcPart_CenterCenter },	// 5
	{ OCcMod_Sub,	OCcMod_None,	UUcTrue,	UUcFalse,	OCcPart_CenterRight },	// 6
	{ OCcMod_Add,	OCcMod_Sub,		UUcFalse,	UUcTrue,	OCcPart_BottomLeft },	// 7
	{ OCcMod_None,	OCcMod_Sub,		UUcFalse,	UUcTrue,	OCcPart_BottomCenter },	// 8
	{ OCcMod_Sub,	OCcMod_Sub,		UUcTrue,	UUcTrue,	OCcPart_BottomRight },	// 9
	
	{ OCcMod_Sub,	OCcMod_Sub,		UUcFalse,	UUcFalse,	OCcPart_BottomRight },	// 10
	{ OCcMod_Add,	OCcMod_Sub,		UUcFalse,	UUcFalse,	OCcPart_BottomLeft },	// 11
	{ OCcMod_None,	OCcMod_Sub,		UUcFalse,	UUcFalse,	OCcPart_BottomCenter },	// 12
	{ OCcMod_Sub,	OCcMod_Sub,		UUcTrue,	UUcFalse,	OCcPart_BottomRight },	// 13
	{ OCcMod_Add,	OCcMod_Sub,		UUcTrue,	UUcFalse,	OCcPart_BottomLeft },	// 14
	
	{ OCcMod_Sub,	OCcMod_Add,		UUcFalse,	UUcFalse,	OCcPart_TopRight },		// 15
	{ OCcMod_Add,	OCcMod_Add,		UUcTrue,	UUcFalse,	OCcPart_TopLeft },		// 16
	
	{ OCcMod_Sub,	OCcMod_None,	UUcFalse,	UUcFalse,	OCcPart_CenterRight },	// 17
	{ OCcMod_Add,	OCcMod_None,	UUcTrue,	UUcFalse,	OCcPart_CenterLeft },	// 18
	
	{ OCcMod_Sub,	OCcMod_Sub,		UUcFalse,	UUcTrue,	OCcPart_BottomRight },	// 19
	{ OCcMod_Add,	OCcMod_Sub,		UUcTrue,	UUcTrue,	OCcPart_BottomLeft },	// 20
	
	{ OCcMod_Sub,	OCcMod_Add,		UUcFalse,	UUcTrue,	OCcPart_TopRight },		// 21
	{ OCcMod_Add,	OCcMod_Add,		UUcFalse,	UUcTrue,	OCcPart_TopLeft },		// 22
	{ OCcMod_None,	OCcMod_Add,		UUcFalse,	UUcTrue,	OCcPart_TopCenter },	// 23
	{ OCcMod_Sub,	OCcMod_Add,		UUcTrue,	UUcTrue,	OCcPart_TopRight },		// 24
	{ OCcMod_Add,	OCcMod_Add,		UUcTrue,	UUcTrue,	OCcPart_TopLeft },		// 25
};

static float					ONgXOffset;
static float					ONgYOffset;

// ======================================================================
// functions
// ======================================================================
// ----------------------------------------------------------------------
static void
OCiGetIndexPosition(
	UUtUns8						inIndex,
	M3tPoint3D					*outPosition)
{
	switch (ONgIndexPositions[inIndex].x_mod)
	{
		case OCcMod_None:
			outPosition->x = 0.0f;
		break;

		case OCcMod_Add:
			outPosition->x = ONgXOffset;
		break;

		case OCcMod_Sub:
			outPosition->x = -ONgXOffset;
		break;
	}
	
	switch (ONgIndexPositions[inIndex].y_mod)
	{
		case OCcMod_None:
			outPosition->y = 0.0f;
		break;

		case OCcMod_Add:
			outPosition->y = ONgYOffset;
		break;

		case OCcMod_Sub:
			outPosition->y = -ONgYOffset;
		break;
	}

	outPosition->z = 0.0f;
	
	// move to the right or bottom of the screen if necessary
	if (ONgIndexPositions[inIndex].add_screen_width)
	{
		outPosition->x += (float)OCcCinematic_Width;
	}

	if (ONgIndexPositions[inIndex].add_screen_height)
	{
		outPosition->y += (float)OCcCinematic_Height;
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static void
OCiCinematic_CalcPositionFromIndex(
	OCtCinematic				*ioCinematic,
	UUtUns8						inIndex,
	M3tPoint3D					*outPosition)
{
	// get the position of inIndex
	OCiGetIndexPosition(inIndex, outPosition);
	
	// offset the position based on the part to set the position to
	switch (ONgIndexPositions[inIndex].part & (OCcPart_Top | OCcPart_Row_Center | OCcPart_Bottom))
	{
		case OCcPart_Top:
		break;
		
		case OCcPart_Row_Center:
			outPosition->y =
				(float)(OCcCinematic_Height - ioCinematic->draw_height) / 2.0f;
		break;
		
		case OCcPart_Bottom:
			outPosition->y -= (float)ioCinematic->draw_height;
		break;
	}
	
	switch (ONgIndexPositions[inIndex].part & (OCcPart_Left | OCcPart_Col_Center | OCcPart_Right))
	{
		case OCcPart_Left:
		break;
		
		case OCcPart_Col_Center:
			outPosition->x =
				(float)(OCcCinematic_Width - ioCinematic->draw_width) / 2.0f;
		break;
		
		case OCcPart_Right:
			outPosition->x -= (float)ioCinematic->draw_width;
		break;
	}
}

// ----------------------------------------------------------------------
static void
OCiCinematic_Draw(
	OCtCinematic				*inCinematic)
{
	M3tPointScreen				dest[2];
	M3tTextureCoord				uv[4];
	float						width_scale = M3rDraw_GetWidth() / ((float) OCcCinematic_ScreenWidth);
	float						height_scale = M3rDraw_GetHeight() / ((float) OCcCinematic_ScreenHeight);

	UUtInt32					draw_width;
	UUtInt32					draw_height;
	
	// set UVs
	if (inCinematic->flags & OCcCinematicFlag_Mirror)
	{
		// tl
		uv[0].u = 1.0f;
		uv[0].v = 0.0f;

		// tr
		uv[1].u = 0.0f;
		uv[1].v = 0.0f;
		
		// bl
		uv[2].u = 1.0f;
		uv[2].v = 1.0f;

		// br
		uv[3].u = 0.0f;
		uv[3].v = 1.0f;
	}
	else
	{
		// tl
		uv[0].u = 0.0f;
		uv[0].v = 0.0f;
		
		// tr
		uv[1].u = 1.0f;
		uv[1].v = 0.0f;
		
		// bl
		uv[2].u = 0.0f;
		uv[2].v = 1.0f;

		// br
		uv[3].u = 1.0f;
		uv[3].v = 1.0f;
	}
	
	// set the dest
	dest[0] = inCinematic->position;
	dest[1] = inCinematic->position;
	
	dest[1].x += (float)inCinematic->draw_width;
	dest[1].y += (float)inCinematic->draw_height;

	dest[0].x = (float) MUrFloat_Round_To_Int(dest[0].x * width_scale);
	dest[0].y = (float) MUrFloat_Round_To_Int(dest[0].y * height_scale);
	dest[1].x = (float) MUrFloat_Round_To_Int(dest[1].x * width_scale);
	dest[1].y = (float) MUrFloat_Round_To_Int(dest[1].y * height_scale);

	draw_width = MUrFloat_Round_To_Int(dest[1].x - dest[0].x);
	draw_height = MUrFloat_Round_To_Int(dest[1].y - dest[0].y);
	
	M3rDraw_State_Push();
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Appearence,
		M3cDrawState_Appearence_Texture_Unlit);
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Interpolation,
		M3cDrawState_Interpolation_None);
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Fill,
		M3cDrawState_Fill_Solid);
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_ConstantColor,
		IMcShade_White);
	
	M3rDraw_State_SetInt(
		M3cDrawStateIntType_Alpha,
		M3cMaxAlpha);
	
	M3rDraw_State_SetPtr(
		M3cDrawStatePtrType_BaseTextureMap,
		inCinematic->texture);
	
	M3rDraw_State_Commit();
	
	M3rDraw_Sprite(
		dest,
		uv);
	
	M3rDraw_State_Pop();

	// draw the border
	dest[0].x -= 3.0f;
	dest[0].y -= 3.0f;
	
	// get a pointer to the border if one doesn't already exist
	if (OCgBorder == NULL)
	{
		TMrInstance_GetDataPtr(
			PScTemplate_PartSpecification,
			"cinematic_border",
			&OCgBorder);
	}
	
	PSrPartSpec_Draw(
		OCgBorder,
		PScPart_All,
		&dest[0],
		draw_width + 6,
		draw_height + 6,
		M3cMaxAlpha);

	return;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OCrCinematic_Draw(
	void)
{
	UUtUns32					num_cinematics;
	UUtUns32					i;
	OCtCinematic				*cinematics;

	if (ONrGameState_IsSkippingCutscene()) {
		return;
	}
	
	// get a pointer to the cinematics array
	cinematics = (OCtCinematic*)UUrMemory_Array_GetMemory(ONgCinematics);
	if (cinematics == NULL) { return; }
	
	// draw the cinematics
	num_cinematics = UUrMemory_Array_GetUsedElems(ONgCinematics);
	for (i = 0; i < num_cinematics; i++)
	{
		OCiCinematic_Draw(&cinematics[i]);
	}
}

// ----------------------------------------------------------------------
void
OCrCinematic_Start(
	char						*inTextureName,
	UUtInt16					inDrawWidth,
	UUtInt16					inDrawHeight,
	UUtUns8						inStart,
	UUtUns8						inEnd,
	float						inVelocity,
	UUtBool						inMirror)
{
	UUtError					error;
	UUtUns32					index;
	UUtBool						mem_moved;
	OCtCinematic				*cinematics;
	M3tTextureMap				*texture;

	M3tPoint3D					start;
	M3tPoint3D					end;
		
	// make sure the parameters are valid
	if ((inTextureName == NULL) || (inTextureName[0] == '\0')) { return; }
	if (inStart > OCcNumStartPositions) { return; }
	if (inEnd > OCcNumStartPositions) { return; }
	if ((inDrawWidth < -1) || (inDrawHeight < -1)) { return; }
	if ((inStart != inEnd) && (UUmFloat_CompareLE(inVelocity, 0.0f)))
	{
		return;
	}
		
	// find the texture
	error =	TMrInstance_GetDataPtr(M3cTemplate_TextureMap, inTextureName, &texture);
	if (error != UUcError_None) { return; }
	
	if (inDrawWidth == -1) { inDrawWidth = texture->width; }
	if (inDrawHeight == -1) { inDrawHeight = texture->height; }
	
	// add the cinematic to the array
	error = UUrMemory_Array_GetNewElement(ONgCinematics, &index, &mem_moved);
	if (error != UUcError_None) { return; }
	
	cinematics = UUrMemory_Array_GetMemory(ONgCinematics);
	if (cinematics == NULL) { return; }
	
	// initialize the cinematic
	cinematics[index].flags = inMirror ? OCcCinematicFlag_Mirror : OCcCinematicFlag_None;
	cinematics[index].texture = texture;
	cinematics[index].draw_width = inDrawWidth * (OCcCinematic_ScreenWidth / 640);
	cinematics[index].draw_height = inDrawHeight * (OCcCinematic_ScreenHeight / 480);
	cinematics[index].position.x = 0.0f;
	cinematics[index].position.y = 0.0f;
	cinematics[index].position.z = 0.5f;
	cinematics[index].position.invW = 2.0f;
	cinematics[index].steps = 0;
	cinematics[index].start = inStart;
	cinematics[index].end = inEnd;
	
	// set the position based on the start
	OCiCinematic_CalcPositionFromIndex(
		&cinematics[index],
		cinematics[index].start,
		&start);
	cinematics[index].position.x = start.x;
	cinematics[index].position.y = start.y;
	
	if (inEnd == inStart)
	{
		MUmVector_Set(cinematics[index].delta, 0.0f, 0.0f, 0.0f);
	}
	else
	{
		float					distance;
		
		// get the end position
		OCiCinematic_CalcPositionFromIndex(&cinematics[index], inEnd, &end);
		
		// calculate the movement delta
		MUmVector_Subtract(cinematics[index].delta, end, start);
		distance = MUrVector_Length(&cinematics[index].delta);
		MUrNormalize(&cinematics[index].delta);
		MUmVector_Scale(cinematics[index].delta, inVelocity);
		
		// calculate the number of steps needed to read the end
		cinematics[index].steps = (UUtUns32)(distance/inVelocity);
	}
}


// ----------------------------------------------------------------------
void
OCrCinematic_Stop(
	char						*inTextureName,
	UUtUns8						inEnd,
	float						inVelocity)
{
	UUtError					error;
	OCtCinematic				*cinematics;
	M3tTextureMap				*texture;
	M3tPoint3D					start;
	M3tPoint3D					end;
	UUtUns32					i;
	UUtUns32					index;
	
	// get a pointer to the cinematics array
	cinematics = (OCtCinematic*)UUrMemory_Array_GetMemory(ONgCinematics);
	if (cinematics == NULL) { return; }
	
	// find the texture
	error =	TMrInstance_GetDataPtr(M3cTemplate_TextureMap, inTextureName, &texture);
	if (error != UUcError_None) { return; }

	// find the cinematic in the list
	index = OCcInvalidCinematic;
	for (i = 0; i < UUrMemory_Array_GetUsedElems(ONgCinematics); i++)
	{
		if (cinematics[i].texture == texture)
		{
			index = i;
			break;
		}
	}
	if (index == OCcInvalidCinematic) { return; }
	
	// clear out all the flags other than the mirror flag
	cinematics[index].flags &= OCcCinematicFlag_Mirror;
	
	// set the needed fields
	cinematics[index].flags |= OCcCinematicFlag_Stop;
	cinematics[index].steps = 0;
	cinematics[index].start = cinematics[index].end;
	cinematics[index].end = inEnd;
	
	// set the position based on the start
	OCiCinematic_CalcPositionFromIndex(
		&cinematics[index],
		cinematics[index].start,
		&start);
	cinematics[index].position.x = start.x;
	cinematics[index].position.y = start.y;
	
	if (inEnd == cinematics[index].start)
	{
		MUmVector_Set(cinematics[index].delta, 0.0f, 0.0f, 0.0f);
	}
	else
	{
		float					distance;
		
		// get the end position
		OCiCinematic_CalcPositionFromIndex(&cinematics[index], inEnd, &end);
		
		// calculate the movement delta
		MUmVector_Subtract(cinematics[index].delta, end, start);
		distance = MUrVector_Length(&cinematics[index].delta);
		MUrNormalize(&cinematics[index].delta);
		MUmVector_Scale(cinematics[index].delta, inVelocity);
		
		// calculate the number of steps needed to read the end
		cinematics[index].steps = (UUtUns32)(distance/inVelocity);
	}
}

// ----------------------------------------------------------------------
void
OCrCinematic_DeleteAll(
	void)
{
	// delete all elements in the cinematic array
	UUrMemory_Array_SetUsedElems(ONgCinematics, 0, NULL);
}

// ----------------------------------------------------------------------
void
OCrCinematic_Update(
	void)
{
	OCtCinematic				*cinematics;
	UUtUns32					num_cinematics;
	UUtInt32					i;
	
	// make sure there are cinematics to draw
	num_cinematics = UUrMemory_Array_GetUsedElems(ONgCinematics);
	if (num_cinematics == 0) { return; }
	
	// get a pointer to the cinematics array
	cinematics = (OCtCinematic*)UUrMemory_Array_GetMemory(ONgCinematics);
	
	// update the cinematics
	for (i = 0; i < (UUtInt32)num_cinematics; i++)
	{
		if (cinematics[i].steps > 0)
		{
			// update the cinematic
			cinematics[i].steps--;
			cinematics[i].position.x += cinematics[i].delta.x;
			cinematics[i].position.y += cinematics[i].delta.y;
		}
		else if ((cinematics[i].flags & OCcCinematicFlag_AtEnd) == 0)
		{
			M3tPoint3D			position;
			
			// put the cinematic exactly at the desired position
			OCiCinematic_CalcPositionFromIndex(
				&cinematics[i],
				cinematics[i].end,
				&position);
			cinematics[i].position.x = position.x;
			cinematics[i].position.y = position.y;
			
			cinematics[i].flags |= OCcCinematicFlag_AtEnd;
			
			// if stopping delete the element
			if ((cinematics[i].flags & OCcCinematicFlag_Stop) != 0)
			{
				cinematics[i].flags |= OCcCinematicFlag_Delete;
			}
		}
	}
	
	// delete the cinematics that are marked for deletion
	for (i = (UUtInt32)(UUrMemory_Array_GetUsedElems(ONgCinematics)) - 1; i >= 0; i--)
	{
		if ((cinematics[i].flags & OCcCinematicFlag_Delete) != 0)
		{
			// delete the element
			UUrMemory_Array_DeleteElement(ONgCinematics, (UUtUns32)i);
			
			// the delete element call may have moved the array
			cinematics = (OCtCinematic*)UUrMemory_Array_GetMemory(ONgCinematics);
		}
	}
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
static UUtError
OCiCinematic_ScriptStart(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	// verify the parameter list
	if (inParameterListLength != 7) { return UUcError_Generic; }
	if (inParameterList[0].type != SLcType_String) { return UUcError_Generic; }
	if (inParameterList[1].type != SLcType_Int32) { return UUcError_Generic; }
	if (inParameterList[2].type != SLcType_Int32) { return UUcError_Generic; }
	if (inParameterList[3].type != SLcType_Int32) { return UUcError_Generic; }
	if (inParameterList[4].type != SLcType_Int32) { return UUcError_Generic; }
	if (inParameterList[5].type != SLcType_Float) { return UUcError_Generic; }
	if (inParameterList[6].type != SLcType_Bool) { return UUcError_Generic; }
	
	OCrCinematic_Start(
		(char*)inParameterList[0].val.str,
		(UUtInt16)inParameterList[1].val.i,
		(UUtInt16)inParameterList[2].val.i,
		(UUtUns8)inParameterList[3].val.i,
		(UUtUns8)inParameterList[4].val.i,
		(float)inParameterList[5].val.f,
		(UUtBool)inParameterList[6].val.b);
	
	*outTicksTillCompletion = 0;
	*outStall = UUcFalse;
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OCiCinematic_ScriptStop(
	SLtErrorContext*		inErrorContext,
	UUtUns32				inParameterListLength,
	SLtParameter_Actual*	inParameterList,
	UUtUns32				*outTicksTillCompletion,	// This returns the number of ticks the caller should be slept
	UUtBool					*outStall,					// This means the caller should be slept and this call made again
	SLtParameter_Actual		*ioReturnValue)
{
	if (inParameterListLength != 3) { return UUcError_Generic; }
	if (inParameterList[0].type != SLcType_String) { return UUcError_Generic; }
	if (inParameterList[1].type != SLcType_Int32) { return UUcError_Generic; }
	if (inParameterList[2].type != SLcType_Float) { return UUcError_Generic; }
	
	OCrCinematic_Stop(
		(char*)inParameterList[0].val.str,
		(UUtUns8)inParameterList[1].val.i,
		(float)inParameterList[2].val.f);
	
	*outTicksTillCompletion = 0;
	*outStall = UUcFalse;
	
	return UUcError_None;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
UUtError
OCrInitialize(
	void)
{
	UUtError					error;
	
	ONgCinematicRef = 0;
	ONgXOffset = 20.0f;
	ONgYOffset = 65.0f;
	
	// allocate memory for the cinematics array
	ONgCinematics =
		UUrMemory_Array_New(
			sizeof(OCtCinematic),
			1,
			0,
			5);
	UUmError_ReturnOnNull(ONgCinematics);

#if CONSOLE_DEBUGGING_COMMANDS
	// register the variables
	error =
		SLrGlobalVariable_Register_Float(
			"cinematic_xoffset",
			"Offset from the vertical edge of the screen for the cinematic insert.",
			&ONgXOffset);
	UUmError_ReturnOnError(error);

	error =
		SLrGlobalVariable_Register_Float(
			"cinematic_yoffset",
			"Offset from the horizontal edge of the screen for the cinematic insert.",
			&ONgYOffset);
	UUmError_ReturnOnError(error);
#endif
	
	// register the script commands
	error =
		SLrScript_Command_Register_Void(
			"cinematic_start",
			"start the display of a cinematic insert",
			"bitmap_name:string draw_width:int draw_height:int start:int end:int velocity:float mirror:bool",
			OCiCinematic_ScriptStart);
	UUmError_ReturnOnError(error);
	
	error =
		SLrScript_Command_Register_Void(
			"cinematic_stop",
			"stop the display of a cinematic insert",
			"bitmap_name:string end:int velocity:float",
			OCiCinematic_ScriptStop);
	UUmError_ReturnOnError(error);

	return UUcError_None;
}

// ----------------------------------------------------------------------
void
OCrTerminate(
	void)
{
	if (ONgCinematics != NULL)
	{
		UUrMemory_Array_Delete(ONgCinematics);
		ONgCinematics = NULL;
	}
}