/*
	FILE:	Oni_InputBindings.h
	
	AUTHOR:	Brent H. Pease
	
	CREATED: Sept 11, 1997
	
	PURPOSE:
	
	Copyright 1997

*/
#ifndef ONI_INPUTBINDINGS_H
#define ONI_INPUTBINDINGS_H

enum
{
	ONcBinding_MoveForward		= 0x001,
	ONcBinding_MoveBackward		= 0x002,
	ONcBinding_MoveLeft			= 0x004,
	ONcBinding_MoveRight		= 0x008,
	ONcBinding_MoveUp			= 0x010,
	ONcBinding_MoveDown			= 0x020,
	ONcBinding_SidestepLeft		= 0x040,
	ONcBinding_SidestepRight	= 0x080,
	ONcBinding_LookLeft			= 0x100,
	ONcBinding_LookRight		= 0x200,
	ONcBinding_Escape			= 0x400,
	ONcBinding_ToggleOpacity	= 0x800

};

#endif /* ONI_INPUTBINDINGS_H */
