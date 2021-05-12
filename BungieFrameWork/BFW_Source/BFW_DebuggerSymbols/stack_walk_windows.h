/*
	stack_walk_windows.h
*/

#ifndef __STACK_WALK_WINDOWS__
#define __STACK_WALK_WINDOWS__

#if defined(UUmPlatform) && (UUmPlatform == UUmPlatform_Win32)

/*---------- prototypes */

void stack_walk_initialize(void);
void stack_walk(short levels_to_ignore);
void stack_walk_with_context(short levels_to_ignore, CONTEXT *context);
int handle_exception(LPEXCEPTION_POINTERS exception_data);

#endif // windows

#endif // __STACK_WALK_WINDOWS__
