// ======================================================================
// DM_DialogCursor.h
// ======================================================================
#ifndef DM_DIALOGCURSOR_H
#define DM_DIALOGCURSOR_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"

// ======================================================================
// defines
// ======================================================================
#define DCcMaxCursorNameLength			32

#define DCcCursorCenter_X				16
#define DCcCursorCenter_Y				16

#define DCcCursor_DefaultAnimationRate	20

// ======================================================================
// typedefs
// ======================================================================
typedef UUtUns16				DCtCursorType;

enum
{
	DCcCursorType_Arrow				= 1,

	DCcNumCursorTypes

};

// ----------------------------------------------------------------------
typedef tm_struct DCtCursorTexture
{
	tm_templateref			texture_ref;

} DCtCursorTexture;

// ----------------------------------------------------------------------
#define DCcTemplate_Cursor				UUm4CharToUns32('D', 'C', 'C', '_')
typedef tm_template('D', 'C', 'C', '_', "DC Cursor List")
DCtCursor
{
	tm_pad							pad0[20];

	UUtUns16						animation_rate;

	tm_varindex	UUtUns16			num_cursors;
	tm_vararray	DCtCursorTexture	cursors[1];

} DCtCursor;

typedef struct DCtCursor_PrivateData
{
	UUtUns16				current_cursor;

	// time in ticks of next animation
	UUtUns32				next_animation;

} DCtCursor_PrivateData;

// ----------------------------------------------------------------------
typedef tm_struct DCtCursorTypePair
{
	UUtUns16				cursor_type;
	char					cursor_name[32];

} DCtCursorTypePair;

// ----------------------------------------------------------------------
#define DCcTemplate_CursorTypeList		UUm4CharToUns32('C', 'T', 'L', '_')
typedef tm_template('C', 'T', 'L', '_', "Cursor Type List")
DCtCursorTypeList
{
	tm_pad							pad0[22];

	tm_varindex UUtUns16			num_cursor_type_pairs;
	tm_vararray DCtCursorTypePair	cursor_type_pair[1];

} DCtCursorTypeList;


extern TMtPrivateData*	DMgTemplate_Cursor_PrivateData;

// ======================================================================
// functions
// ======================================================================
void
DCrCursor_Animate(
	DCtCursor				*inCursor,
	UUtUns32				inTime);

void
DCrCursor_Draw(
	DCtCursor				*inCursor,
	IMtPoint2D				*inDestination,
	float					inLayer);

UUtError
DCrCursor_Load(
	DCtCursorType			inCursorType,
	DCtCursor				**outCursorList);

UUtError
DCrCursor_ProcHandler(
	TMtTemplateProc_Message	inMessage,
	void*					inInstancePtr,
	void*					inDataPtr);

// ----------------------------------------------------------------------
UUtError
DCrInitialize(
	void);

void
DCrTerminate(
	void);

// ======================================================================
#endif /* DM_DIALOGCURSOR_H */
