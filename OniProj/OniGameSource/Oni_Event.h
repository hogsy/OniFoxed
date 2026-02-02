// ======================================================================
// Oni_Event.h
// ======================================================================
#pragma once

#ifndef ONI_EVENT_H
#define ONI_EVENT_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Motoko.h"
#include "BFW_ScriptLang.h"
#include "BFW_Particle3.h"
#include "BFW_SoundSystem2.h"

// ======================================================================
// defines
// ======================================================================

#define ONcEventParams_MaxTextLength		32

// ======================================================================
// enums
// ======================================================================

typedef enum ONtEvent_Type
{
	ONcEvent_None						= 0,
	ONcEvent_Script_Execute,
	ONcEvent_Turret_Activate,
	ONcEvent_Turret_Deactivate,
	ONcEvent_Console_Activate,
	ONcEvent_Console_Deactivate,
	ONcEvent_Alarm_Activate,
	ONcEvent_Alarm_Deactivate,
	ONcEvent_Trigger_Activate,
	ONcEvent_Trigger_Deactivate,
	ONcEvent_Door_Lock,
	ONcEvent_Door_Unlock,
	ONrEvent_NumTypes
} ONtEvent_Type;

typedef enum ONtEventParams_Type
{
	ONcEventParams_None					= 0,
	ONcEventParams_ID16					= 1,
	ONcEventParams_ID32					= 2,
	ONcEventParams_Text					= 3
} ONtEventParams_Type;

typedef struct ONtEventParams_ID16
{
		UUtUns16					id;
} ONtEventParams_ID16;

typedef struct ONtEventParams_ID32
{
		UUtUns32					id;
} ONtEventParams_ID32;

typedef struct ONtEventParams_Text
{
		char						text[ONcEventParams_MaxTextLength];
} ONtEventParams_Text;


typedef struct ONtEvent
{
	ONtEvent_Type					event_type;
	union
	{
		ONtEventParams_ID16			params_id16;
		ONtEventParams_ID32			params_id32;
		ONtEventParams_Text			params_text;
	} params;
} ONtEvent;

typedef struct ONtEventList
{
	UUtUns16				event_count;
	ONtEvent				*events;
} ONtEventList;

// ======================================================================
// enumerator
// ======================================================================

typedef UUtBool (*ONtEvent_EnumCallback_TypeName)(	const char *inName, UUtUns32 inUserData );

UUtUns32 ONrEvent_EnumerateTypes( ONtEvent_EnumCallback_TypeName inEnumCallback, UUtUns32 inUserData );

// ======================================================================
// event handler
// ======================================================================

typedef void (*ONtEvent_Handler)( ONtEvent *inEvent );

// ======================================================================
// event descriptions
// ======================================================================

typedef struct ONtEventDescription
{
	ONtEvent_Type					type;
	char							name[320];
	ONtEvent_Handler				handler;
	ONtEventParams_Type				params_type;
} ONtEventDescription;

// ======================================================================
// event list managment
// ======================================================================

UUtError ONrEventList_Initialize( ONtEventList *inEventList );
UUtError ONrEventList_Destroy( ONtEventList *inEventList );
UUtError ONrEventList_DeleteEvent( ONtEventList *inEventList, UUtUns32 inIndex );
UUtError ONrEventList_AddEvent( ONtEventList *inEventList, ONtEvent *inEvent );
UUtError ONrEventList_Copy( ONtEventList *inSource, ONtEventList *inDest );
UUtError ONrEventList_Execute( ONtEventList *inEventList, ONtCharacter *inCharacter );

// ======================================================================
// event list serialization
// ======================================================================

UUtUns32 ONrEventList_Read( ONtEventList *inEventList, UUtUns16	inVersion, UUtBool inSwapIt, UUtUns8 *inBuffer );
UUtError ONrEventList_Write( ONtEventList *inEventList, UUtUns8 *ioBuffer, UUtUns32 *ioBufferSize );
UUtUns32 ONrEventList_GetWriteSize( ONtEventList *inEventList );

// ======================================================================
// event managment
// ======================================================================

UUtError ONrEvent_GetName( ONtEvent *inEvent, char* ioDescription, UUtUns32 inMaxLength );

ONtEventDescription* ONrEvent_GetDescription( ONtEvent_Type inType );

UUtError ONrEventList_FillListBox( ONtEventList *inEventList, WMtWindow* inWindow );

ONtEvent_Type ONrEvent_GetTypeFromName( char* inName );

extern ONtEventDescription ONcEventDescriptions[ONrEvent_NumTypes];


#endif

