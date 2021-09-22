#include "BFW.h"
#include "BFW_Memory.h"

#include "Oni_Camera.h"
#include "Oni_GameState.h"
#include "Oni_GameStatePrivate.h"
#include "Oni_Object.h"
#include "Oni_Mechanics.h"

static UUtMemory_Array			*ONgMechanicsClasses;

// ==================================================================================
// object management
// ==================================================================================
UUtUns16 ONrMechanics_GetNextID( OBJtObjectType inObjectType )
{
	UUtUns16				id;
	UUtUns16				i;
	UUtUns32				*bit_vector;
	ONtMechanicsClass*		mechanics_class;
	UUtUns32				object_count;
	OBJtObject				**object_list;
	UUtError				error;
	
	mechanics_class = OBJrObjectType_GetMechanicsByType( inObjectType );		UUmAssert( mechanics_class );
	if( !mechanics_class )
		return 0xFFFF;

	error = OBJrObjectType_GetObjectList( inObjectType, &object_list, &object_count );
	UUmAssert( error == UUcError_None );

	bit_vector = UUrBitVector_New(UUcMaxUns16);
	for( i = 0; i < object_count; i++ )
	{
		UUmAssert( object_list[i] );
		id = mechanics_class->methods.rGetID( object_list[i] );
		UUrBitVector_SetBit( bit_vector, id );
	}

	for( id = 1; id < UUcMaxUns16; id++ )
	{
		if( !UUrBitVector_TestBit( bit_vector, id ) )
		{
			break;
		}
	}
	UUrBitVector_Dispose(bit_vector);
	return id;
}
// =====================================================================================
// default class methods
// =====================================================================================

UUtError ONrMechanics_Default_ClassMethod_LevelBegin( struct ONtMechanicsClass *inClass )
{
	UUtUns32					i;
	UUtUns32					object_count;
	OBJtObject					**object_list;
	OBJtObject					*object;
	UUtError					error;
	
	UUmAssert( inClass );

	error = OBJrObjectType_GetObjectList( inClass->object_type, &object_list, &object_count );	UUmAssert( error == UUcError_None );

	UUmAssert((0 == object_count) || (NULL != object_list));

	for( i = 0; i < object_count; i++ )
	{
		object = object_list[i];

		UUmAssert( object );
		
		inClass->methods.rInitialize( object );
	}
	return UUcError_None;
}

UUtError ONrMechanics_Default_ClassMethod_LevelEnd( struct ONtMechanicsClass *inClass )
{
	UUtUns32					i;
	UUtUns32					object_count;
	OBJtObject					**object_list;
	OBJtObject					*object;
	UUtError					error;
	
	UUmAssert( inClass );

	error = OBJrObjectType_GetObjectList( inClass->object_type, &object_list, &object_count );	UUmAssert( error == UUcError_None );

	UUmAssert((0 == object_count) || (NULL != object_list));

	for( i = 0; i < object_count; i++ )
	{
		object = object_list[i];

		UUmAssert( object );
		
		inClass->methods.rTerminate( object );
	}
	return UUcError_None;
}

UUtError ONrMechanics_Default_ClassMethod_Reset( struct ONtMechanicsClass *inClass )
{
	UUtUns32					i;
	UUtUns32					object_count;
	OBJtObject					**object_list;
	OBJtObject					*object;
	UUtError					error;
	
	UUmAssert( inClass );

	error = OBJrObjectType_GetObjectList( inClass->object_type, &object_list, &object_count );	UUmAssert( error == UUcError_None );

	UUmAssert((0 == object_count) || (NULL != object_list));

	for( i = 0; i < object_count; i++ )
	{
		object = object_list[i];

		UUmAssert( object );
		
		inClass->methods.rReset( object );
	}
	return UUcError_None;
}

void ONrMechanics_Default_ClassMethod_Update( struct ONtMechanicsClass *inClass )
{
	UUtUns32					i;
	UUtUns32					object_count;
	OBJtObject					**object_list;
	OBJtObject					*object;
	UUtError					error;
	
	UUmAssert( inClass );

	error = OBJrObjectType_GetObjectList( inClass->object_type, &object_list, &object_count );	UUmAssert( error == UUcError_None );

	UUmAssert((0 == object_count) || (NULL != object_list));

	for( i = 0; i < object_count; i++ )
	{
		object = object_list[i];

		UUmAssert( object );
		
		inClass->methods.rUpdate( object );
	}
}

void ONrMechanics_DeleteClass( ONtMechanicsClass *inMechanicsClass )
{
	ONtMechanicsClass			**class_list;
	UUtUns32					num_classes;
	UUtUns32					i;
	
	class_list = (ONtMechanicsClass**) UUrMemory_Array_GetMemory(ONgMechanicsClasses);

	UUmAssert(class_list);
	
	num_classes = UUrMemory_Array_GetUsedElems(ONgMechanicsClasses);	
	for (i = 0; i < num_classes; i++)
	{
		if (class_list[i] == inMechanicsClass)
		{
			UUrMemory_Array_Delete(inMechanicsClass->object_array);
			inMechanicsClass->object_array = NULL;
			
			UUrMemory_Array_DeleteElement(ONgMechanicsClasses, i);
			break;
		}
	}
	UUrMemory_Block_Delete(inMechanicsClass);
	UUrMemory_Block_VerifyList();
}

// =====================================================================================
// class registration
// =====================================================================================

UUtError ONrMechanics_Register( OBJtObjectType inObjectType, UUtUns32 inObjectTypeIndex, char *inGroupName,
							    UUtUns32 inSizeInMemory, OBJtMethods *inObjectMethods, UUtUns32 inFlags,
								ONtMechanicsMethods* inMechanicsMethods )
{
	UUtError					error;
	UUtUns32					index;
	ONtMechanicsClass			**class_list;
	ONtMechanicsClass			*new_class;

	UUmAssert( inMechanicsMethods );

	// create a new group entry in the mechanics class array
	error = UUrMemory_Array_GetNewElement(ONgMechanicsClasses, &index, NULL);
	UUmError_ReturnOnError(error);
	
	// allocate memory for the class
	new_class = (ONtMechanicsClass*)UUrMemory_Block_NewClear(sizeof(ONtMechanicsClass));
	UUmError_ReturnOnNull(new_class);
	
	// initialize this class
	new_class->object_type		= inObjectType;
	new_class->methods			= *inMechanicsMethods;

	// place defaults
	if( !new_class->methods.rClassLevelBegin && new_class->methods.rInitialize )	new_class->methods.rClassLevelBegin = ONrMechanics_Default_ClassMethod_LevelBegin;
	if( !new_class->methods.rClassLevelEnd	 && new_class->methods.rTerminate )		new_class->methods.rClassLevelEnd	= ONrMechanics_Default_ClassMethod_LevelEnd;
	if( !new_class->methods.rClassReset		 && new_class->methods.rReset )			new_class->methods.rClassReset		= ONrMechanics_Default_ClassMethod_Reset;
	if( !new_class->methods.rClassUpdate	 && new_class->methods.rUpdate )		new_class->methods.rClassUpdate		= ONrMechanics_Default_ClassMethod_Update;

	// setup the object array
	new_class->object_array = UUrMemory_Array_New( sizeof(OBJtObject*), 1, 0, 0);

	// add the class to the class array
	class_list = (ONtMechanicsClass**) UUrMemory_Array_GetMemory(ONgMechanicsClasses);
	UUmAssert(class_list);
	class_list[index] = new_class;

	// register the object group
	error = OBJrObjectGroup_Register( inObjectType, inObjectTypeIndex, inGroupName, inSizeInMemory, inObjectMethods, inFlags );
	UUmError_ReturnOnError(error);

	// register mechanics with group
	error = OBJrObjectType_RegisterMechanics( inObjectType, new_class );
	UUmError_ReturnOnError(error);
		
	return UUcError_None;
}

// =====================================================================================
// initialization and destruction
// =====================================================================================

UUtError ONrMechanics_Initialize( )
{
	ONgMechanicsClasses = UUrMemory_Array_New( sizeof(ONtMechanicsClass*), 1, 0, 0);
	UUmError_ReturnOnNull(ONgMechanicsClasses);
	
	return UUcError_None;
}

void ONrMechanics_Terminate( )
{
	ONtMechanicsClass			**class_list;
	if (ONgMechanicsClasses)
	{
		while (UUrMemory_Array_GetUsedElems(ONgMechanicsClasses))
		{
			class_list = (ONtMechanicsClass**)UUrMemory_Array_GetMemory(ONgMechanicsClasses);
			ONrMechanics_DeleteClass(class_list[0]);
		}
		// delete the object group array
		UUrMemory_Array_Delete(ONgMechanicsClasses);
		ONgMechanicsClasses = NULL;
	}
	UUrMemory_Block_VerifyList();
}

// =====================================================================================
// level begin & end
// =====================================================================================

UUtError ONrMechanics_LevelBegin( )
{
	UUtUns32					i;
	UUtUns32					class_count;
	ONtMechanicsClass			**class_list;
	ONtMechanicsClass			*mechanics_class;

	UUmAssert( ONgMechanicsClasses );

	class_list = (ONtMechanicsClass**) UUrMemory_Array_GetMemory(ONgMechanicsClasses);
	UUmAssert(class_list);

	class_count = UUrMemory_Array_GetUsedElems(	ONgMechanicsClasses );

	for( i = 0; i < class_count; i++ )
	{
		mechanics_class = class_list[i];
		
		UUmAssert( mechanics_class );
		
		if( mechanics_class->methods.rClassLevelBegin )
			mechanics_class->methods.rClassLevelBegin( mechanics_class );
	}
	return UUcError_None;
}

UUtError ONrMechanics_LevelEnd( )
{
	UUtUns32					i;
	UUtUns32					class_count;
	ONtMechanicsClass			**class_list;
	ONtMechanicsClass			*mechanics_class;

	UUmAssert( ONgMechanicsClasses );

	class_list = (ONtMechanicsClass**) UUrMemory_Array_GetMemory(ONgMechanicsClasses);
	UUmAssert(class_list);

	class_count = UUrMemory_Array_GetUsedElems(	ONgMechanicsClasses );

	for( i = 0; i < class_count; i++ )
	{
		mechanics_class = class_list[i];
		
		UUmAssert( mechanics_class );

		if( mechanics_class->methods.rClassLevelEnd )
			mechanics_class->methods.rClassLevelEnd( mechanics_class );
	}
	return UUcError_None;
}

// =====================================================================================
// reset all mechanics
// =====================================================================================

UUtError ONrMechanics_Reset( )
{
	UUtUns32					i;
	UUtUns32					class_count;
	ONtMechanicsClass			**class_list;
	ONtMechanicsClass			*mechanics_class;

	UUmAssert( ONgMechanicsClasses );

	class_list = (ONtMechanicsClass**) UUrMemory_Array_GetMemory(ONgMechanicsClasses);
	UUmAssert(class_list);

	class_count = UUrMemory_Array_GetUsedElems(	ONgMechanicsClasses );

	for( i = 0; i < class_count; i++ )
	{
		mechanics_class = class_list[i];
		
		UUmAssert( mechanics_class );
		
		if( mechanics_class->methods.rClassReset )
			mechanics_class->methods.rClassReset( mechanics_class );
	}
	return UUcError_None;
}

// =====================================================================================
// main update for all mechanics
// =====================================================================================

void ONrMechanics_Update()
{
	UUtUns32					i;
	UUtUns32					class_count;
	ONtMechanicsClass			**class_list;
	ONtMechanicsClass			*mechanics_class;

	UUmAssert( ONgMechanicsClasses );

	class_list = (ONtMechanicsClass**) UUrMemory_Array_GetMemory(ONgMechanicsClasses);
	UUmAssert(class_list);

	class_count = UUrMemory_Array_GetUsedElems(	ONgMechanicsClasses );

	for( i = 0; i < class_count; i++ )
	{
		mechanics_class = class_list[i];
		UUmAssert( mechanics_class );
		if( mechanics_class->methods.rClassUpdate )
			mechanics_class->methods.rClassUpdate( mechanics_class );
	}
}


OBJtObject* ONrMechanics_GetObjectWithID( OBJtObjectType inObjectType, UUtUns16 inID )
{
	OBJtObject				*object;
	ONtMechanicsClass		*mechanics_class;
	UUtUns32				object_count;
	OBJtObject				**object_list;
	UUtUns16				id;
	UUtUns32				i;
	UUtError				error;
	
	mechanics_class = OBJrObjectType_GetMechanicsByType( inObjectType );
	UUmAssert( mechanics_class );
	if( !mechanics_class )
		return NULL;

	error = OBJrObjectType_GetObjectList( inObjectType, &object_list, &object_count );
	UUmAssert( error == UUcError_None );
	if( error != UUcError_None )
		return NULL;

	for( i = 0; i < object_count; i++ )
	{
		object = object_list[i];
		UUmAssert( object );
		id = mechanics_class->methods.rGetID( object );
		if( id == inID )
			return object;
	}
	return NULL;
}

