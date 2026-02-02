#pragma once

#ifndef ONI_MECHANICS_H
#define ONI_MECHANICS_H

#include "Oni_Object.h"
#include "Oni_Object_Private.h"

struct ONtMechanicsClass;

typedef UUtError (*ONtMechanics_Method_Initialize)	( OBJtObject *inObject );
typedef UUtError (*ONtMechanics_Method_Terminate)	( OBJtObject *inObject );
typedef UUtError (*ONtMechanics_Method_Reset)		( OBJtObject *inObject );
typedef void	 (*ONtMechanics_Method_Update)		( OBJtObject *inObject );
typedef UUtUns16 (*ONtMechanics_Method_GetID)		( OBJtObject *inObject );

typedef UUtError (*ONtMechanics_Method_ClassLevelBegin)	( struct ONtMechanicsClass *inObject );
typedef UUtError (*ONtMechanics_Method_ClassLevelEnd)	( struct ONtMechanicsClass *inObject );
typedef UUtError (*ONtMechanics_Method_ClassReset)		( struct ONtMechanicsClass *inObject );
typedef void	 (*ONtMechanics_Method_ClassUpdate)		( struct ONtMechanicsClass *inObject );

struct ONtMechanicsMethods
{
	ONtMechanics_Method_Initialize			rInitialize;
	ONtMechanics_Method_Terminate			rTerminate;
	ONtMechanics_Method_Reset				rReset;
	ONtMechanics_Method_Update				rUpdate;
	ONtMechanics_Method_GetID				rGetID;

	ONtMechanics_Method_ClassLevelBegin		rClassLevelBegin;
	ONtMechanics_Method_ClassLevelEnd		rClassLevelEnd;
	ONtMechanics_Method_ClassReset			rClassReset;
	ONtMechanics_Method_ClassUpdate			rClassUpdate;

};

struct ONtMechanicsClass
{
	UUtUns32							object_type;
	ONtMechanicsMethods					methods;
	UUtMemory_Array						*object_array;
};

// default class methods
UUtError ONrMechanics_Default_ClassMethod_LevelBegin( struct ONtMechanicsClass *inClass );
UUtError ONrMechanics_Default_ClassMethod_LevelEnd( struct ONtMechanicsClass *inClass );
void     ONrMechanics_Default_ClassMethod_Update( struct ONtMechanicsClass *inClass );
UUtError ONrMechanics_Default_ClassMethod_Reset( struct ONtMechanicsClass *inClass );

// mechanics management
UUtError ONrMechanics_Register( OBJtObjectType inObjectType, UUtUns32 inObjectTypeIndex, char *inGroupName,
							    UUtUns32 inSizeInMemory, OBJtMethods *inMethods, UUtUns32 inFlags,
								ONtMechanicsMethods* inMechanicsMethods );
OBJtObject* ONrMechanics_GetObjectWithID( OBJtObjectType inObjectType, UUtUns16 inID );

// init and reset
UUtError ONrMechanics_LevelBegin(void);
UUtError ONrMechanics_LevelEnd(void);
UUtError ONrMechanics_Reset(void);

UUtError ONrMechanics_Initialize( );
void ONrMechanics_Terminate( );
void ONrMechanics_DeleteClass( ONtMechanicsClass *inMechanicsClass );

// misc helpers
OBJtObject* ONiMechanics_GetObjectByIndex( OBJtObjectType inObjectType, UUtUns32 inIndex );
UUtUns16	ONrMechanics_GetNextID( OBJtObjectType inObjectType );
UUtError	ONrMechanics_GetObjectList( OBJtObjectType inObjectType, OBJtObject ***ioList, UUtUns32 *ioObjectCount );

// main update
void ONrMechanics_Update();

#endif

