// ======================================================================
// Oni_Event.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_BinaryData.h"
#include "BFW_Timer.h"

#include "EulerAngles.h"
#include "Oni_BinaryData.h"
#include "Oni_Object.h"
#include "Oni_Object_Private.h"

#include "Oni_Weapon.h"
#include "Oni_Level.h"
#include "Oni_Camera.h"
#include "Oni_Character.h"
#include "Oni_Windows.h"
#include "Oni_AI2_Targeting.h"
#include "Oni_Event.h"
#include "Oni_Script.h"

#include "ENVFileFormat.h"

// ======================================================================
// globals
// ======================================================================

ONtCharacter		*ONgEvent_Character;

// ======================================================================
// handler prototypes
// ======================================================================

void ONrEvent_Handle_ExecuteScript( ONtEvent *inEvent );
void ONrEvent_Handle_TurretActivate( ONtEvent *inEvent );
void ONrEvent_Handle_TurretDeactivate( ONtEvent *inEvent );
void ONrEvent_Handle_ConsoleActivate( ONtEvent *inEvent );
void ONrEvent_Handle_ConsoleDeactivate( ONtEvent *inEvent );
void ONrEvent_Handle_AlarmActivate( ONtEvent *inEvent );
void ONrEvent_Handle_AlarmDeactivate( ONtEvent *inEvent );
void ONrEvent_Handle_TriggerActivate( ONtEvent *inEvent );
void ONrEvent_Handle_TriggerDeactivate( ONtEvent *inEvent );
void ONrEvent_Handle_DoorLock( ONtEvent *inEvent );
void ONrEvent_Handle_DoorUnlock( ONtEvent *inEvent );

// ======================================================================
// constants
// ======================================================================

ONtEventDescription ONcEventDescriptions[ONrEvent_NumTypes] =
{
	{	ONcEvent_Script_Execute,		"Execute Script",			ONrEvent_Handle_ExecuteScript,		ONcEventParams_Text	},
	{	ONcEvent_Turret_Activate,		"Activate Turret",			ONrEvent_Handle_TurretActivate,		ONcEventParams_ID16	},
	{	ONcEvent_Turret_Deactivate,		"Deactivate Turret",		ONrEvent_Handle_TurretDeactivate,	ONcEventParams_ID16	},
	{	ONcEvent_Console_Activate,		"Activate Console",			ONrEvent_Handle_ConsoleActivate,	ONcEventParams_ID16	},
	{	ONcEvent_Console_Deactivate,	"Deactivate Console",		ONrEvent_Handle_ConsoleDeactivate,	ONcEventParams_ID16	},
	{	ONcEvent_Alarm_Activate,		"Activate Alarm",			ONrEvent_Handle_AlarmActivate,		ONcEventParams_ID16	},
	{	ONcEvent_Alarm_Deactivate,		"Deactivate Alarm",			ONrEvent_Handle_AlarmDeactivate,	ONcEventParams_ID16	},
	{	ONcEvent_Trigger_Activate,		"Activate Trigger",			ONrEvent_Handle_TriggerActivate,	ONcEventParams_ID16	},
	{	ONcEvent_Trigger_Deactivate,	"Deactivate Trigger",		ONrEvent_Handle_TriggerDeactivate,	ONcEventParams_ID16	},
	{	ONcEvent_Door_Lock,				"Lock Door",				ONrEvent_Handle_DoorLock,			ONcEventParams_ID16	},
	{	ONcEvent_Door_Unlock,			"Unlock Door",				ONrEvent_Handle_DoorUnlock,			ONcEventParams_ID16	},
	{	ONcEvent_None,					"",							NULL,								ONcEventParams_None	}
};

// ======================================================================
// functions
// ======================================================================


UUtError ONrEventList_Initialize( ONtEventList *inEventList )
{
	inEventList->event_count	= 0;
	inEventList->events			= NULL;
	return UUcError_None;
}

UUtError ONrEventList_Destroy( ONtEventList *inEventList )
{
	inEventList->events = UUrMemory_Block_Realloc( inEventList->events, 0 );
	return UUcError_None;
}

UUtError ONrEventList_AddEvent( ONtEventList *inEventList, ONtEvent *inEvent )
{
	inEventList->events = UUrMemory_Block_Realloc( inEventList->events, sizeof(ONtEvent) * (inEventList->event_count + 1));
	inEventList->events[inEventList->event_count] = *inEvent;
	inEventList->event_count++;
	return UUcError_None;
}

UUtError ONrEventList_DeleteEvent( ONtEventList *inEventList, UUtUns32 inIndex )
{
	UUtUns32		i;
	ONtEvent		*new_events;

	if( inEventList->event_count <= 0 || inIndex >= inEventList->event_count )
		return UUcError_Generic;

	new_events = UUrMemory_Block_Realloc( NULL, sizeof(ONtEvent) * (inEventList->event_count - 1));

	for( i = 0; i < inEventList->event_count; i++ )
	{
		if( i == inIndex )
		{
			for( i++; i < inEventList->event_count; i++ )
			{
				UUrMemory_MoveFast( &inEventList->events[i], &new_events[i-1], sizeof(ONtEvent));
			}
		}
		else
		{
			UUrMemory_MoveFast( &inEventList->events[i], &new_events[i], sizeof(ONtEvent));
		}
	}

	UUrMemory_Block_Realloc( inEventList->events, 0 );
	inEventList->events = new_events;
	inEventList->event_count--;

	return UUcError_None;
}

UUtError ONrEventList_Copy( ONtEventList *inSource, ONtEventList *inDest )
{

	inDest->event_count		= inSource->event_count;
	inDest->events			= UUrMemory_Block_Realloc( NULL, sizeof(ONtEvent) * inSource->event_count );
	if( inDest->events )
	{
		UUrMemory_MoveFast( inSource->events, inDest->events, sizeof(ONtEvent) * inSource->event_count );
	}
	return UUcError_None;
}

// ----------------------------------------------------------------------
UUtUns32 ONrEventList_Read( ONtEventList *inEventList, UUtUns16	inVersion, UUtBool inSwapIt, UUtUns8 *inBuffer )
{
	UUtUns32					num_bytes;
	UUtUns32					i;
	ONtEvent*					event;
	ONtEventDescription			*desc;

	OBJmGet2BytesFromBuffer(inBuffer, inEventList->event_count, UUtUns16,	inSwapIt);

	num_bytes = sizeof(UUtUns16);

	inEventList->events = UUrMemory_Block_Realloc( inEventList->events, sizeof(ONtEvent) * inEventList->event_count );
	for( i = 0; i < inEventList->event_count; i++ )
	{
		UUtUns16					event_type;
		
		event = &inEventList->events[i];

		OBJmGet2BytesFromBuffer( inBuffer, event_type, UUtUns16, inSwapIt );
		event->event_type = event_type;

		num_bytes += sizeof(UUtUns16);

		desc = ONrEvent_GetDescription( event->event_type );
		
		UUmAssert( desc );

		switch( desc->params_type )
		{
			case ONcEventParams_None:
				break;
			case ONcEventParams_ID16:
				OBJmGet2BytesFromBuffer(inBuffer, event->params.params_id16.id, UUtUns16, inSwapIt);
				num_bytes += sizeof( UUtUns16 );
				break;
			case ONcEventParams_ID32:
				OBJmGet4BytesFromBuffer(inBuffer, event->params.params_id32.id, UUtUns32, inSwapIt);
				num_bytes += sizeof( UUtUns32 );
				break;
			case ONcEventParams_Text:
				OBJmGetStringFromBuffer(inBuffer, event->params.params_text.text, ONcEventParams_MaxTextLength, inSwapIt);
				num_bytes += ONcEventParams_MaxTextLength;
				break;
			default:
				UUmAssert( UUcFalse );
		}
	}
	return num_bytes;
}

UUtError ONrEventList_Write( ONtEventList *inEventList, UUtUns8 *ioBuffer, UUtUns32 *ioBufferSize )
{
	UUtUns32					bytes_available;
	UUtUns32					i;
	ONtEvent					*event;
	ONtEventDescription			*desc;
	
	// set the number of bytes available
	bytes_available = *ioBufferSize;

	OBJmWrite2BytesToBuffer( ioBuffer, inEventList->event_count, UUtUns16,	bytes_available, OBJcWrite_Little);
	
	for( i = 0; i < inEventList->event_count; i++ )
	{
		event = &inEventList->events[i];

		OBJmWrite2BytesToBuffer(ioBuffer, event->event_type, UUtInt16, bytes_available, OBJcWrite_Little);

		desc = ONrEvent_GetDescription( event->event_type );
		
		UUmAssert( desc );

		switch( desc->params_type )
		{
			case ONcEventParams_None:
				break;
			case ONcEventParams_ID16:
				OBJmWrite2BytesToBuffer(ioBuffer, event->params.params_id16.id, UUtInt16, bytes_available, OBJcWrite_Little);
				break;
			case ONcEventParams_ID32:
				OBJmWrite4BytesToBuffer(ioBuffer, event->params.params_id32.id, UUtInt32, bytes_available, OBJcWrite_Little);
				break;
			case ONcEventParams_Text:
				OBJmWriteStringToBuffer(ioBuffer, event->params.params_text.text, ONcEventParams_MaxTextLength, bytes_available, OBJcWrite_Little);
				break;
			default:
				UUmAssert( UUcFalse );
		}
	}

	*ioBufferSize = bytes_available;

	return UUcError_None;
}

UUtUns32 ONrEventList_GetWriteSize( ONtEventList *inEventList )
{
	UUtUns32					num_bytes;
	UUtUns32					i;
	ONtEvent*					event;
	ONtEventDescription			*desc;

	num_bytes	= sizeof( UUtUns16 );

	for( i = 0; i < inEventList->event_count; i++ )
	{
		event = &inEventList->events[i];

		num_bytes += sizeof(UUtUns16);

		desc = ONrEvent_GetDescription( event->event_type );
		
		UUmAssert( desc );

		switch( desc->params_type )
		{
			case ONcEventParams_None:
				break;
			case ONcEventParams_ID16:
				num_bytes += sizeof( UUtUns16 );
				break;
			case ONcEventParams_ID32:
				num_bytes += sizeof( UUtUns32 );
				break;
			case ONcEventParams_Text:
				num_bytes += ONcEventParams_MaxTextLength;
				break;
			default:
				UUmAssert( UUcFalse );
		}
	}

	return num_bytes;
}

// ======================================================================
// event execution
// ======================================================================

static UUtError ONrEvent_Execute( ONtEvent *inEvent )
{
	ONtEventDescription			*desc;
	desc = ONrEvent_GetDescription( inEvent->event_type );
	if( desc && desc->handler )
	{
		desc->handler( inEvent );
		return UUcError_None;
	}
	return UUcError_Generic;
}

UUtError ONrEventList_Execute( ONtEventList *inEventList, ONtCharacter *inCharacter )
{
	ONtEvent					*event;
	UUtUns32					i;
	
	ONgEvent_Character = inCharacter;
	
	for( i = 0; i < inEventList->event_count; i++ )
	{
		event = &inEventList->events[i];
		ONrEvent_Execute( event );
	}

	return UUcError_None;
}

// ======================================================================
// handlers
// ======================================================================

void ONrEvent_Handle_ExecuteScript( ONtEvent *inEvent )
{
	if( inEvent->params.params_text.text ) {
		ONrCharacter_Defer_Script(inEvent->params.params_text.text, (ONgEvent_Character == NULL) ? "none" : ONgEvent_Character->player_name, "eventlist");
	}
}
void ONrEvent_Handle_TurretActivate( ONtEvent *inEvent )
{
	OBJrTurret_Activate_ID( inEvent->params.params_id16.id );
}
void ONrEvent_Handle_TurretDeactivate( ONtEvent *inEvent )
{
	OBJrTurret_Deactivate_ID( inEvent->params.params_id16.id );
}
void ONrEvent_Handle_ConsoleActivate( ONtEvent *inEvent )
{
	OBJrConsole_Activate_ID( inEvent->params.params_id16.id );
}
void ONrEvent_Handle_ConsoleDeactivate( ONtEvent *inEvent )
{
	OBJrConsole_Deactivate_ID( inEvent->params.params_id16.id );
}
void ONrEvent_Handle_TriggerActivate( ONtEvent *inEvent )
{
	OBJrTrigger_Activate_ID( inEvent->params.params_id16.id );
}
void ONrEvent_Handle_TriggerDeactivate( ONtEvent *inEvent )
{
	OBJrTrigger_Deactivate_ID( inEvent->params.params_id16.id );
}
void ONrEvent_Handle_AlarmActivate( ONtEvent *inEvent )
{
	UUtUns8			alarm_id;
	UUmAssert( inEvent );
	alarm_id		= (UUtUns8) inEvent->params.params_id16.id;
	AI2rKnowledge_AlarmTripped( alarm_id, ONgEvent_Character );
}
void ONrEvent_Handle_AlarmDeactivate( ONtEvent *inEvent )
{
}

void ONrEvent_Handle_DoorLock( ONtEvent *inEvent )
{
	UUtUns16			id;

	UUmAssert( inEvent );

	id			= (UUtUns16) inEvent->params.params_id16.id;
	
	OBJrDoor_Lock_ID( id );

}

void ONrEvent_Handle_DoorUnlock( ONtEvent *inEvent )
{
	UUtUns16			id;

	UUmAssert( inEvent );

	id			= (UUtUns16) inEvent->params.params_id16.id;

	OBJrDoor_Unlock_ID( id );
}

// ======================================================================
// enumerator
// ======================================================================

UUtUns32 ONrEvent_EnumerateTypes( ONtEvent_EnumCallback_TypeName inEnumCallback, UUtUns32 inUserData )
{
	ONtEventDescription			*desc;
	UUtUns32					num_types;
	UUtBool						result;

	UUmAssert( inEnumCallback );

	num_types = 0;

	for( desc = ONcEventDescriptions; desc->type != ONcEvent_None; desc++ )
	{
		num_types++;
		result = inEnumCallback( desc->name, inUserData );
		if( !result )
			return num_types;
	}
	return num_types;
}

ONtEventDescription* ONrEvent_GetDescription( ONtEvent_Type inType )
{
	ONtEventDescription			*desc;
	for( desc = ONcEventDescriptions; desc->type != ONcEvent_None; desc++ )
	{
		if( desc->type == inType )
			return desc;
	}
	return NULL;
}

UUtError ONrEvent_GetName( ONtEvent *inEvent, char* ioName, UUtUns32 inMaxLength )
{
	ONtEventDescription			*desc;
	char						temp_name[100];

	desc = ONrEvent_GetDescription( inEvent->event_type );
	
	UUmAssert( desc );
	
	switch( desc->params_type )
	{
		case ONcEventParams_None:
			sprintf( temp_name, "%s", desc->name );
			break;
		case ONcEventParams_ID16:
			sprintf( temp_name, "%s: %d", desc->name, inEvent->params.params_id16.id );
			break;
		case ONcEventParams_ID32:
			sprintf( temp_name, "%s: %d", desc->name, inEvent->params.params_id32.id );
			break;
		case ONcEventParams_Text:
			sprintf( temp_name, "%s: %s", desc->name, inEvent->params.params_text.text );
			break;
		default:
			UUmAssert( UUcFalse );
			temp_name[0] = '\0';
	}

	UUrString_Copy( ioName, temp_name, inMaxLength );

	return UUcError_None;
}

UUtError ONrEventList_FillListBox( ONtEventList *inEventList, WMtWindow* inWindow )
{
	char				event_name[32];
	ONtEvent			*event;
	UUtUns32			i;

	UUmAssert( inEventList && inWindow );

	for( i = 0; i < inEventList->event_count; i++ )
	{
		event = &inEventList->events[i];
		ONrEvent_GetName( event, event_name, 32 );
		WMrMessage_Send( inWindow, LBcMessage_AddString, (UUtUns32) event_name, 0);
	}

	return UUcError_None;
}

ONtEvent_Type ONrEvent_GetTypeFromName( char* inName )
{
	ONtEventDescription			*desc;
	for( desc = ONcEventDescriptions; desc->type != ONcEvent_None; desc++ )
	{
		if( !UUrString_Compare_NoCase( inName, desc->name ) )
			return desc->type;
	}
	return ONcEvent_None;
}
