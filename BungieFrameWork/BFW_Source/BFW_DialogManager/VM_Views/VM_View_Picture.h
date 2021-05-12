// ======================================================================
// VM_View_Picture.h
// ======================================================================
#ifndef VM_VIEW_PICTURE_H
#define VM_VIEW_PICTURE_H

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_ViewManager.h"

// ======================================================================
// typedefs
// ======================================================================
#define VMcTemplate_View_Picture			UUm4CharToUns32('V', 'M', '_', 'P')
typedef tm_template('V', 'M', '_', 'P', "VM View Picture")
VMtView_Picture
{
	
	VMtTextureList			*b_textures;	// background textures
	VMtPartSpec				*partspec;
	
} VMtView_Picture;

typedef struct VMtView_Picture_PrivateData
{
	UUtUns16				unused;
	
} VMtView_Picture_PrivateData;

// ======================================================================
// prototypes
// ======================================================================
UUtUns32
VMrView_Picture_Callback(
	VMtView				*inView,
	VMtMessage			inMessage,
	UUtUns32			inParam1,
	UUtUns32			inParam2);

// ======================================================================
#endif /* VM_VIEW_PICTURE_H */