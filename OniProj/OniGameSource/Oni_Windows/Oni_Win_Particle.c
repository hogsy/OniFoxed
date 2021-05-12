#if TOOL_VERSION
// ======================================================================
// Oni_Win_Particle.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================

#include "bfw_math.h"

#include "BFW.h"
#include "BFW_WindowManager.h"
#include "BFW_Particle3.h"
#include "WM_Dialog.h"

#include "WM_DrawContext.h"
#include "WM_PartSpecification.h"
#include "WM_Scrollbar.h"
#include "WM_MenuBar.h"
#include "WM_Menu.h"
#include "WM_PopupMenu.h"
#include "WM_Picture.h"

#include "Oni.h"
#include "Oni_Windows.h"
#include "Oni_Level.h"
#include "Oni_Win_Particle.h"
#include "Oni_BinaryData.h"
#include "Oni_Sound2.h"
#include "Oni_Win_Sound2.h"
#include "Oni_Particle3.h"

// ======================================================================
// globals
// ======================================================================

// a global array for tracking open particle windows that need to be
// updated by their child windows
#define OWcParticle_MaxOpenWindows				32
UUtUns16 OWgParticle_NumOpenWindows;
WMtWindow *OWgParticle_OpenWindows[OWcParticle_MaxOpenWindows];

// for renaming variables
char *OWgRenameVarTraverseOldName;
char *OWgRenameVarTraverseNewName;

// for deleting variables
char *OWgDeleteVarTraverseName;
char OWgDeleteVarTraverseLocation[256];

// values for formatting emitters in the listbox
#define OWcParticleNumEmitterHeaderLines	5
#define OWcParticleNumEmitterSections		6

// ======================================================================
// prototypes
// ======================================================================

static UUtBool OWrParticle_DecalTextures_Callback( WMtDialog *inDialog, WMtMessage inMessage, UUtUns32 inParam1, UUtUns32 inParam2);

void OWrParticle_Value_Sound_Get(SStType inType, OWtParticle_Value_Data *inValUserData);

// ======================================================================
// functions
// ======================================================================


// initialise the particle windowing system
UUtError OWrParticle_Initialize(void)
{
	OWgParticle_NumOpenWindows = 0;

	return UUcError_None;
}

// register that a window has opened that needs its pointer tracked
// for children to access it
static void OWiParticle_RegisterOpenWindow(WMtWindow *inWindow)
{
	UUmAssert(OWgParticle_NumOpenWindows < OWcParticle_MaxOpenWindows);

	OWgParticle_OpenWindows[OWgParticle_NumOpenWindows++] = inWindow;
}

// register that a window which had its pointer tracked for children
// to access it is closing
static void OWiParticle_RegisterCloseWindow(WMtWindow *inWindow)
{
	UUtUns16 itr;

	// find the window in our array
	for (itr = 0; itr < OWgParticle_NumOpenWindows; itr++)
		if (OWgParticle_OpenWindows[itr] == inWindow)
			break;

	if (itr >= OWgParticle_NumOpenWindows) {
		// couldn't find the window. either it has already closed
		// or it was never registered. this probably shouldn't happen but
		// isn't severe enough to warrant an assertion.
		return;
	}

	// fill in this hole in the array
	if (itr < OWgParticle_NumOpenWindows - 1) {
		OWgParticle_OpenWindows[itr] = OWgParticle_OpenWindows[OWgParticle_NumOpenWindows - 1];
	}
	OWgParticle_NumOpenWindows--;
}

// a child has made a change and wants to notify its parent window.
// is this window still open?
static UUtBool OWiParticle_IsWindowOpen(WMtWindow *inWindow)
{
	UUtUns16 itr;

	// find the window in our array
	for (itr = 0; itr < OWgParticle_NumOpenWindows; itr++)
		if (OWgParticle_OpenWindows[itr] == inWindow)
			return UUcTrue;

	return UUcFalse;
}

// notify a parent window of some changes that were made by its child
static UUtBool OWiParticle_NotifyParent(WMtWindow *inParent, UUtUns32 inParam1)
{
	if (!OWiParticle_IsWindowOpen(inParent))
		return UUcFalse;

	// send a message to the parent window
	WMrMessage_Send(inParent, OWcMessage_Particle_UpdateList, inParam1, (UUtUns32) -1);

	return UUcTrue;
}

static void OWiRenameVarTraverseProc(P3tVariableReference *ioVarRef, char *inLocation)
{
	if (strcmp(ioVarRef->name, OWgRenameVarTraverseOldName) == 0) {
		strcpy(ioVarRef->name, OWgRenameVarTraverseNewName);
	}
}

static void OWiDeleteVarTraverseProc(P3tVariableReference *ioVarRef, char *inLocation)
{
	if (strcmp(ioVarRef->name, OWgDeleteVarTraverseName) == 0) {
		strcpy(OWgDeleteVarTraverseLocation, inLocation);
	}
}

static void OWiPrintShade(IMtShade inShade, char *outString)
{
	float shade_val[4];

	IMrPixel_Decompose(IMrPixel_FromShade(IMcPixelType_ARGB8888, inShade),
						IMcPixelType_ARGB8888,
						&shade_val[0], &shade_val[1], &shade_val[2], &shade_val[3]);

	sprintf(outString, "%.2f, %.2f, %.2f, %.2f",
			shade_val[0], shade_val[1], shade_val[2], shade_val[3]);
}

static UUtBool OWiParseShade(char *inString, IMtShade *outShade)
{
	float shade_val[4];
	int read_params;

	read_params = sscanf(inString, "%f, %f, %f, %f",
			&shade_val[0], &shade_val[1], &shade_val[2], &shade_val[3]);

	if (read_params == 4) {
		*outShade = IMrShade_FromPixel(IMcPixelType_ARGB8888, IMrPixel_FromARGB(IMcPixelType_ARGB8888,
										shade_val[0], shade_val[1], shade_val[2], shade_val[3]));


		return UUcTrue;
	} else {
		return UUcFalse;
	}
}

static void OWiFillVariableMenu(WMtPopupMenu *inMenu, P3tDataType inType,
						 P3tParticleDefinition *inDefinition,
						 char *select_item)
{
	UUtUns16			itr;
	WMtMenuItemData		item_data;

	UUmAssertReadPtr(inDefinition, sizeof(P3tParticleDefinition));

	// must always be at least one item
	item_data.flags = WMcMenuItemFlag_Enabled;
	item_data.id = -1;
	strcpy(item_data.title, "NONE");
	WMrPopupMenu_AppendItem(inMenu, &item_data);
	if (select_item == NULL)
		WMrPopupMenu_SetSelection(inMenu, 0);

	for (itr = 0; itr < inDefinition->num_variables; itr++) {
		if ((inDefinition->variable[itr].type & inType) == inType) {
			item_data.flags = WMcMenuItemFlag_Enabled;
			item_data.id = itr;
			strcpy(item_data.title, inDefinition->variable[itr].name);
	
			WMrPopupMenu_AppendItem(inMenu, &item_data);

			if ((select_item != NULL) && (strcmp(select_item, item_data.title) == 0))
				WMrPopupMenu_SetSelection(inMenu, itr);
		}
	}
}

// describe a value - handles some cases that BFW_Particle3 can't know about
static void OWiDescribeValue(P3tParticleClass *inClass, P3tDataType inDataType, P3tValue *inValue, char *outString)
{
	// try the particle system first
	if (P3rDescribeValue(inClass, inDataType, inValue, outString))
		return;

	/*
	 * ambient sounds
	 */
	if ((inDataType == P3cDataType_Sound_Ambient) &&
		(inValue->type == P3cValueType_String)) {
		const char *sound_name;
		SStAmbient *ambient;
		
		// get this ambient sound's name
		sound_name = inValue->u.string_const.val;
		
		// get the ambient sound
		ambient = OSrAmbient_GetByName(sound_name);
		if (ambient == NULL) {
			sprintf(outString, "ambient sound %s '<no ambient sound>'", sound_name);
			return;
		}
		
		sprintf(outString, "ambient sound '%s'", sound_name);
		
/*		UUtUns32 sound_id;
		OStCollection *ambient_collection;
		OStItem *sound_item;
		const char *sound_name;

		// get this ambient sound's name
		sound_id = inValue->u.enum_const.val;

		if (sound_id == SScInvalidID) {
			strcpy(outString, "no sound");
			return;
		}

		ambient_collection = OSrCollection_GetByType(OScCollectionType_Ambient);
		if (!ambient_collection) {
			sprintf(outString, "ambient sound %d '<no collection>'", sound_id);
			return;
		}

		sound_item = OSrCollection_Item_GetByID(ambient_collection, sound_id);
		if (!sound_item) {
			sprintf(outString, "ambient sound %d '<no item>'", sound_id);
			return;
		}

		sound_name = OSrItem_GetName(sound_item);
		if (!sound_name) {
			sprintf(outString, "ambient sound %d '<null item name>'", sound_id);
			return;
		}

		sprintf(outString, "ambient sound '%s'", sound_name);*/
		return;
	}

	/*
	 * impulse sounds
	 */
	if ((inDataType == P3cDataType_Sound_Impulse) &&
		(inValue->type == P3cValueType_String)) {
		const char *sound_name;
		SStImpulse *impulse;
		
		// get this impulse sound's name
		sound_name = inValue->u.string_const.val;
		
		// get the impulse sound
		impulse = OSrImpulse_GetByName(sound_name);
		if (impulse == NULL) {
			sprintf(outString, "impulse sound %s '<no impulse sound>'", sound_name);
			return;
		}
		
		sprintf(outString, "impulse sound '%s'", sound_name);
		
/*		UUtUns32 sound_id;
		OStCollection *impulse_collection;
		OStItem *sound_item;
		const char *sound_name;

		// get this impulse sound's name
		sound_id = inValue->u.enum_const.val;

		if (sound_id == SScInvalidID) {
			strcpy(outString, "no sound");
			return;
		}

		impulse_collection = OSrCollection_GetByType(OScCollectionType_Impulse);
		if (!impulse_collection) {
			sprintf(outString, "impulse sound %d '<no collection>'", sound_id);
			return;
		}

		sound_item = OSrCollection_Item_GetByID(impulse_collection, sound_id);
		if (!sound_item) {
			sprintf(outString, "impulse sound %d '<no item>'", sound_id);
			return;
		}

		sound_name = OSrItem_GetName(sound_item);
		if (!sound_name) {
			sprintf(outString, "impulse sound %d '<null item name>'", sound_id);
			return;
		}

		sprintf(outString, "impulse sound '%s'", sound_name);*/
	}
}

static void OWiFillEnumMenu(P3tParticleClass *inClass, WMtPopupMenu *inMenu, P3tEnumTypeInfo *inEnumInfo, UUtUns32 inSelected)
{
	UUtUns16			itr, added = 0;
	WMtMenuItemData		item_data;

	UUmAssert(!inEnumInfo->not_handled);

	for (itr = (UUtUns16) inEnumInfo->min; itr <= inEnumInfo->max; itr++) {
		if (P3rDescribeEnum(inClass, inEnumInfo->type, itr, item_data.title) == UUcTrue) {
			item_data.flags = WMcMenuItemFlag_Enabled;
			item_data.id = itr;

			WMrPopupMenu_AppendItem(inMenu, &item_data);
			added++;

			if (inSelected == item_data.id)
				WMrPopupMenu_SetSelection(inMenu, item_data.id);
		}
	}

	if (!added) {
		item_data.flags = WMcMenuItemFlag_Enabled;
		item_data.id = (UUtUns16) inEnumInfo->defaultval;
		strcpy(item_data.title, "<<error>>");

		WMrPopupMenu_AppendItem(inMenu, &item_data);
	}
}

static void OWiGetVariableFromMenu(WMtPopupMenu *inMenu, P3tParticleDefinition *inDefinition, P3tVariableReference *outVar)
{
	UUtUns16			var_index;
	P3tVariableInfo *	selected_var;

	UUmAssertReadPtr(inDefinition, sizeof(P3tParticleDefinition));

	// get the currently selected variable
	WMrPopupMenu_GetItemID(inMenu, -1, &var_index);

	if (var_index == (UUtUns16) -1) {
		// make a null variable reference
		outVar->offset = -1;
		strcpy(outVar->name, "");
		outVar->type = P3cDataType_None;
	} else {
		UUmAssert((var_index >= 0) && (var_index < inDefinition->num_variables));
		selected_var = &inDefinition->variable[var_index];

		// copy a reference to this variable
		strcpy(outVar->name, selected_var->name);
		outVar->offset = selected_var->offset;
		outVar->type = selected_var->type;
	}
}

static UUtError OWiParticle_Edit_OpenEditor(WMtWindow *inDialog)
{
	WMtWindow *						listbox;
	WMtWindow *						dialog;
	OWtParticle_Edit_Data *			particledata;
	char							particleclassname[P3cParticleClassNameLength + 1];

	// allocate the particle-editing data structure
	particledata = (OWtParticle_Edit_Data *) UUrMemory_Block_New(sizeof(OWtParticle_Edit_Data));

	// get the currently selected particle class
	listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Edit_Listbox);
	if (!listbox)
		return UUcError_Generic;

	WMrMessage_Send(listbox, LBcMessage_GetText, (UUtUns32) particleclassname, (UUtUns32)(-1));

	// find this class
	particledata->classptr = P3rGetParticleClass(particleclassname);
	if (particledata->classptr == NULL) {
		UUrDebuggerMessage("can't find this class!\n");
		UUrMemory_Block_Delete(particledata);
		return UUcError_Generic;
	}

	// can't edit particles unless you have write access to the file
	if (particledata->classptr->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Locked) {
		if (WMrDialog_MessageBox(NULL, "File is Locked",
					"You do not have write access to this particle file. Maybe you need to check it "
					"out from Visual SourceSafe first. Click OK to continue, but any changes that you "
					"make to the particle file ***WILL NOT BE SAVED***",
					WMcMessageBoxStyle_OK_Cancel) == WMcDialogItem_Cancel) {
			UUrMemory_Block_Delete(particledata);
			return UUcError_Generic;
		}
	}

	// pop up the particle dialog
	WMrDialog_Create(OWcDialog_Particle_Class, NULL, OWrParticle_Class_Callback, (UUtUns32) particledata, &dialog);

	return UUcError_None;
}

// sort comparison function for two particle classes
static int UUcExternal_Call OWiQSort_ParticleClasses(const void *pptr1, const void *pptr2)
{
	P3tParticleClass *c1 = *((P3tParticleClass **) pptr1);
	P3tParticleClass *c2 = *((P3tParticleClass **) pptr2);

	return strcmp(c1->classname, c2->classname);
}

UUtBool
OWrParticle_Edit_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	UUtUns32						modalreturn, index;
	WMtWindow *						listbox;
	P3tParticleClassIterateState	itr;
	P3tParticleClass *				classptr;
	P3tParticleClass **				classarray;
	P3tString						oldname;
	char							newname[32];
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Edit_Listbox);
			if (!listbox)
				break;

			// make an array of all particle class pointers
			classarray = (P3tParticleClass **) UUrMemory_Block_New(P3gNumClasses * sizeof(P3tParticleClass *));
			if (classarray == NULL)
				break;

			itr = P3cParticleClass_None;
			index = 0;
			while (P3rIterateClasses(&itr, &classarray[index++]) == UUcTrue) {
			}
			index--;
			UUmAssert(index == P3gNumClasses);

			// sort the particle classes by name
			qsort((void *) classarray, P3gNumClasses, sizeof(P3tParticleClass *), OWiQSort_ParticleClasses);

			// add the particle classes to the box
			for (index = 0; index < P3gNumClasses; index++) {
				WMrMessage_Send(listbox, LBcMessage_AddString, (UUtUns32) classarray[index]->classname, 0);
			}

			// delete the temporary array we allocated
			UUrMemory_Block_Delete(classarray);
		break;
		
		case WMcMessage_Destroy:
			// no destruction necessary
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
				break;

				case OWcParticle_Edit_Listbox:
					// if the user double-clicks on a particle class, edit it by
					// falling through this case statement to the edit button code
					if (UUmHighWord(inParam1) != WMcNotify_DoubleClick)
					{
						// any other interaction with the listbox does nothing
						break;
					}

				case OWcParticle_Edit_Edit:
					// if the user hits the edit button, edit the class they have selected
					OWiParticle_Edit_OpenEditor(inDialog);
				break;

				case OWcParticle_Edit_NewDecal:
					// create a new decal class
					strcpy(newname, "d_newdecal");
					WMrDialog_ModalBegin(OWcDialog_Particle_DecalTextures, inDialog, OWrParticle_DecalTextures_Callback, (UUtUns32) newname, &modalreturn);
					if(((UUtBool) modalreturn == UUcTrue))	// && (P3rNewParticleClass(newname) != NULL)) 
					{
						listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Edit_Listbox);
						if (listbox != NULL) 
						{
							index = WMrMessage_Send(listbox, LBcMessage_AddString, (UUtUns32) newname, 0);
							WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32) -1, index);
						}
					}
				break;

				case OWcParticle_Edit_New:
					// create a new, blank, particle class
					strcpy(newname, "new_class");
					WMrDialog_ModalBegin(OWcDialog_Particle_ClassName, inDialog, OWrParticle_ClassName_Callback,
										 (UUtUns32) newname, &modalreturn);
					if (((UUtBool) modalreturn == UUcTrue) && (P3rNewParticleClass(newname,UUcTrue) != NULL)) {
						listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Edit_Listbox);
						if (listbox != NULL) {
							index = WMrMessage_Send(listbox, LBcMessage_AddString, (UUtUns32) newname, 0);
							WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32) -1, index);
						}
					}
				break;

				case OWcParticle_Edit_Duplicate:
					// create a duplicate of a particle class
					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Edit_Listbox);
					if (!listbox)
						break;

					if (WMrMessage_Send(listbox, LBcMessage_GetText,
										(UUtUns32) oldname, (UUtUns32) -1) == LBcError)
						break;

					classptr = P3rGetParticleClass(oldname);
					if (classptr == NULL)
						break;

					sprintf(newname, "%s_copy", oldname);
					WMrDialog_ModalBegin(OWcDialog_Particle_ClassName, inDialog, OWrParticle_ClassName_Callback,
										 (UUtUns32) newname, &modalreturn);
					if (((UUtBool) modalreturn == UUcTrue) && (P3rDuplicateParticleClass(classptr, newname) != NULL)) {
						listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Edit_Listbox);
						if (listbox != NULL) {
							index = WMrMessage_Send(listbox, LBcMessage_AddString, (UUtUns32) newname, 0);
							WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32) -1, index);
						}
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_Variables_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	OWtParticle_Edit_Data *			user_data;
	OWtParticle_Value_Data *		val_user_data;
	OWtParticle_NewVar_Data			newvar_data;
	WMtDialog *						dialog;
	UUtUns16						itr;
	P3tVariableInfo	*				var;
	char							variablestring[256];
	char							messageboxstring[384];
	WMtWindow *						listbox;
	WMtWindow *						memory_text;
	UUtUns32						selected_var;
	UUtUns32						modaldialog_message;

	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);
			listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
			if (listbox != NULL) {
				// add each variable to the listbox
				for (itr = 0, var = user_data->classptr->definition->variable;
						itr < user_data->classptr->definition->num_variables; itr++, var++) {
					P3rDescribeVariable(user_data->classptr, var, variablestring);
					WMrMessage_Send(listbox, LBcMessage_AddString, (UUtUns32) variablestring, 0);
				}
			}

			// set up the variable size static text item
			memory_text = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_MemUsage);
			if (memory_text != NULL) {
				sprintf(variablestring, "%d bytes", user_data->classptr->particle_size);
				WMrWindow_SetTitle(memory_text, variablestring, WMcMaxTitleLength);
			}

			// register that we may need to be notified about changes
			OWiParticle_RegisterOpenWindow(inDialog);
		break;
		
		case WMcMessage_Destroy:
			// deallocate user data
			user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);

			UUrMemory_Block_Delete(user_data);

			// register that we are going away and shouldn't be notified about any further changes
			OWiParticle_RegisterCloseWindow(inDialog);
		break;

		case OWcMessage_Particle_UpdateList:
			user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);

			// a variable has changed. update the display of its value.
			listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
			UUmAssert(listbox != NULL);

			P3rDescribeVariable(user_data->classptr,
								&user_data->classptr->definition->variable[inParam1], variablestring);
			WMrMessage_Send(listbox, LBcMessage_ReplaceString, (UUtUns32) variablestring, (UUtUns32) inParam1);
			
			break;

		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case OWcParticle_Variables_New:
					// pop up a 'new variable' dialog
					WMrDialog_ModalBegin(OWcDialog_Particle_NewVar, inDialog, OWrParticle_NewVar_Callback,
						                 (UUtUns32) &newvar_data, &modaldialog_message);

					if ((UUtBool) modaldialog_message == UUcTrue) {
						user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);

						// this particle class is now dirty. NB: must be called BEFORE we make any changes.
						P3rMakeDirty(user_data->classptr, UUcTrue);

						// create a new variable in the particle class. this might change the class pointer
						// because it has to move around. fortunately we have a pointer to the OWtParticle_Edit_Data
						// which is the only way that the particle editing windows know about this. change
						// it there and it isn't a problem.
						P3rClass_NewVar(user_data->classptr, &var);

						// set up this variable
						strcpy(var->name, newvar_data.name);
						var->type = newvar_data.type;
						var->offset = -1;	// not yet calculated. needed for P3rPackVariables to know it's new
						P3rSetDefaultInitialiser(var->type, &(var->initialiser));

						// update the particle class
						P3rPackVariables(user_data->classptr, UUcTrue);

						// update the variable list
						P3rDescribeVariable(user_data->classptr, var, variablestring);
						listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
						UUmAssert(listbox != NULL);
						WMrMessage_Send(listbox, LBcMessage_AddString, (UUtUns32) variablestring, (UUtUns32) -1);

						// update the variable size static text item
						memory_text = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_MemUsage);
						if (memory_text != NULL) {
							sprintf(variablestring, "%d bytes", user_data->classptr->particle_size);
							WMrWindow_SetTitle(memory_text, variablestring, WMcMaxTitleLength);
						}

						// we may now need to autosave
						OBDrMakeDirty();
					}
					break;

				case OWcParticle_Variables_Listbox:
					// if the user double-clicks on a variable, edit it by
					// falling through this case statement to the edit button code
					if (UUmHighWord(inParam1) != WMcNotify_DoubleClick)
					{
						// any other interaction with the listbox does nothing
						break;
					}

				case OWcParticle_Variables_Edit:
					// get the currently selected variable
					user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);
					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
					if (listbox == NULL)
						break;
					selected_var = WMrMessage_Send(listbox, LBcMessage_GetSelection,
													(UUtUns32) -1, (UUtUns32) -1);

					if ((selected_var < 0) || (selected_var >= user_data->classptr->definition->num_variables))
						break;
					var = &user_data->classptr->definition->variable[selected_var];

					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &var->initialiser;
					val_user_data->variable_allowed = UUcFalse;
					sprintf(val_user_data->title, "Edit %s initial value", var->name);

					val_user_data->notify_struct.line_to_notify = (UUtUns16) selected_var;
					val_user_data->notify_struct.notify_window	= inDialog;

					switch (var->type) {
					case P3cDataType_Integer:
						WMrDialog_Create(OWcDialog_Particle_Value_Int, NULL,
										OWrParticle_Value_Int_Callback, (UUtUns32) val_user_data, &dialog);
						break;

					case P3cDataType_Float:
						WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
										OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
						break;

					case P3cDataType_String:
						WMrDialog_Create(OWcDialog_Particle_Value_String, NULL,
										OWrParticle_Value_String_Callback, (UUtUns32) val_user_data, &dialog);
						break;

					case P3cDataType_Shade:
						WMrDialog_Create(OWcDialog_Particle_Value_Shade, NULL,
										OWrParticle_Value_Shade_Callback, (UUtUns32) val_user_data, &dialog);
						break;
					
					case P3cDataType_Sound_Ambient:
						OWrParticle_Value_Sound_Get(SScType_Ambient, val_user_data);
						break;
						
					case P3cDataType_Sound_Impulse:
						OWrParticle_Value_Sound_Get(SScType_Impulse, val_user_data);
						break;

					default:
						if (var->type & P3cDataType_Enum) {
							P3rGetEnumInfo(user_data->classptr, var->type, &val_user_data->enum_info);

							if (val_user_data->enum_info.not_handled) {
/*								switch(val_user_data->enum_info.type) {
									case P3cEnumType_AmbientSoundID:
									{
										UUtError error;
										UUtUns32 sound_id;

										error = OWrSAS_Display(inDialog, &sound_id, NULL);
										if (error == UUcError_None) {
											val_user_data->valptr->type = P3cValueType_Enum;
											val_user_data->valptr->u.enum_const.val = sound_id;

											OWiParticle_NotifyParent(val_user_data->notify_struct.notify_window,
														(UUtUns32) val_user_data->notify_struct.line_to_notify);
										}
										break;
									}
									case P3cEnumType_ImpulseSoundID:
									{
										UUtError error;
										UUtUns32 sound_id;

										error = OWrSIS_Display(inDialog, &sound_id, NULL);
										if (error == UUcError_None) {
											val_user_data->valptr->type = P3cValueType_Enum;
											val_user_data->valptr->u.enum_const.val = sound_id;

											OWiParticle_NotifyParent(val_user_data->notify_struct.notify_window,
														(UUtUns32) val_user_data->notify_struct.line_to_notify);
										}
										break;
									}
									default:
										// don't know how to handle this value. do nothing.
										break;
								}*/
							} else {
								WMrDialog_Create(OWcDialog_Particle_Value_Enum, NULL,
												OWrParticle_Value_Enum_Callback, (UUtUns32) val_user_data, &dialog);
							}
						}
					}

					break;

				case OWcParticle_Variables_Rename:
					// get the currently selected variable
					user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);
					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
					if (listbox == NULL)
						break;
					selected_var = WMrMessage_Send(listbox, LBcMessage_GetSelection,
													(UUtUns32) -1, (UUtUns32) -1);

					if ((selected_var < 0) || (selected_var >= user_data->classptr->definition->num_variables))
						break;
					var = &user_data->classptr->definition->variable[selected_var];

					strcpy(variablestring, var->name);
					WMrDialog_ModalBegin(OWcDialog_Particle_RenameVar, inDialog, OWrParticle_RenameVar_Callback,
						                 (UUtUns32) variablestring, &modaldialog_message);

					if ((UUtBool) modaldialog_message == UUcTrue) {
						// this particle class is now dirty. NB: must be called BEFORE we make any changes.
						P3rMakeDirty(user_data->classptr, UUcTrue);

						// traverse all variable references in the particle class and
						// rename them to match the new name
						OWgRenameVarTraverseOldName = var->name;
						OWgRenameVarTraverseNewName = variablestring;
						P3rTraverseVarRef(user_data->classptr->definition, OWiRenameVarTraverseProc, UUcFalse);

						// rename the variable
						strcpy(var->name, variablestring);

						// this variable's description has changed
						P3rDescribeVariable(user_data->classptr, var, variablestring);

						// update the variable list
						listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
						UUmAssert(listbox != NULL);
						WMrMessage_Send(listbox, LBcMessage_ReplaceString, (UUtUns32) variablestring, selected_var);

						// we may now need to autosave
						OBDrMakeDirty();
					}
					break;

				case OWcParticle_Variables_Delete:
					// get the currently selected variable
					user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);
					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
					if (listbox == NULL)
						break;
					selected_var = WMrMessage_Send(listbox, LBcMessage_GetSelection,
													(UUtUns32) -1, (UUtUns32) -1);

					if ((selected_var < 0) || (selected_var >= user_data->classptr->definition->num_variables))
						break;
					var = &user_data->classptr->definition->variable[selected_var];

					// we can't delete a variable unless there are no references to it
					OWgDeleteVarTraverseName = var->name;
					strcpy(OWgDeleteVarTraverseLocation, "");
					P3rTraverseVarRef(user_data->classptr->definition, OWiDeleteVarTraverseProc, UUcTrue);

					// was there a reference to it?
					if (strcmp(OWgDeleteVarTraverseLocation, "") != 0) {
						sprintf(messageboxstring, "This variable is still referenced at: %s. "
							                      "It cannot be deleted while it is in use.",
							    OWgDeleteVarTraverseLocation);
						WMrDialog_MessageBox(inDialog, "Cannot delete variable!",
											messageboxstring, WMcMessageBoxStyle_OK);
						break;
					}

					// this particle class is now dirty. NB: must be called BEFORE we make any changes.
					P3rMakeDirty(user_data->classptr, UUcTrue);

					// we may now need to autosave
					OBDrMakeDirty();

					// remove this variable from the particle class. this might change the class pointer
					// if it has to move around. see rationale above attached to P3rClass_NewVar
					P3rClass_DeleteVar(user_data->classptr, (UUtUns16) selected_var);

					// update the particle class
					P3rPackVariables(user_data->classptr, UUcTrue);

					// update the variable list
					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
					UUmAssert(listbox != NULL);
					WMrMessage_Send(listbox, LBcMessage_DeleteString, (UUtUns32) -1, (UUtUns32) selected_var);

					// update the variable size static text item
					memory_text = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_MemUsage);
					if (memory_text != NULL) {
						sprintf(variablestring, "%d bytes", user_data->classptr->particle_size);
						WMrWindow_SetTitle(memory_text, variablestring, WMcMaxTitleLength);
					}
					break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

static void OWiParticle_Appearance_UpdateFields(WMtDialog *inDialog)
{
	OWtParticle_Edit_Data *			user_data;
	WMtWindow *						text_string;
	char							textbuf[196];
	char							valuebuf[128];

	user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);

	// set up the X scale
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_XScale);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->appearance.scale, valuebuf);
		sprintf(textbuf, "Scale: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}
	
	// set up the X offset
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_XOffset);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->appearance.x_offset, valuebuf);
		sprintf(textbuf, "X Offset: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the X shortening
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_XShorten);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->appearance.x_shorten, valuebuf);
		sprintf(textbuf, "X Shorten: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the Y scale
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_YScale);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->appearance.y_scale, valuebuf);
		sprintf(textbuf, "Y Scale: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the rotation
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_Rotation);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->appearance.rotation, valuebuf);
		sprintf(textbuf, "Rotation: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the alpha
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_Alpha);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->appearance.alpha, valuebuf);
		sprintf(textbuf, "Alpha: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the tint
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_Tint);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Shade, &user_data->classptr->definition->appearance.tint, valuebuf);
		sprintf(textbuf, "Tint: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the edge-fade
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_EdgeFadeMin);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->appearance.edgefade_min, valuebuf);
		sprintf(textbuf, "Edge Fade Minval: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_EdgeFadeMax);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->appearance.edgefade_max, valuebuf);
		sprintf(textbuf, "Edge Fade Maxval: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the max-contrail-length
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_MaxContrail);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->appearance.max_contrail, valuebuf);
		sprintf(textbuf, "Max Contrail Length: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the lensflare seethrough distance
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_LensflareDist);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->appearance.lensflare_dist, valuebuf);
		sprintf(textbuf, "Lensflare Through Dist: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the lensflare in-frames number
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_LensInFrames);
	if (text_string != NULL) {
		sprintf(textbuf, "Lensflare Fade-In Frames: %d", user_data->classptr->definition->appearance.lens_inframes);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the lensflare out-frames number
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_LensOutFrames);
	if (text_string != NULL) {
		sprintf(textbuf, "Lensflare Fade-Out Frames: %d", user_data->classptr->definition->appearance.lens_outframes);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the texture or geometry name
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_Texture);
	if (text_string != NULL) {
		sprintf(textbuf, "Tex/Geom: %s", user_data->classptr->definition->appearance.instance);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the max decals number
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_MaxDecals);
	if (text_string != NULL) {
		sprintf(textbuf, "Max Decals: %d", user_data->classptr->definition->appearance.max_decals);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the decal fade-frames number
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_DecalFadeFrames);
	if (text_string != NULL) {
		sprintf(textbuf, "Decal Fade Frames: %d", user_data->classptr->definition->appearance.decal_fadeframes);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the decal wrap angle
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_DecalWrap);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->appearance.decal_wrap, valuebuf);
		sprintf(textbuf, "Decal Wrap Angle: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}
}

UUtBool
OWrParticle_Appearance_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled, checked;
	UUtUns32						new_flags, new_flags2, selection;
	UUtUns16						selected_id;
	OWtParticle_Edit_Data *			user_data;
	OWtParticle_Number_Data *		number_user_data;
	OWtParticle_Value_Data *		val_user_data;
	OWtParticle_Texture_Data *		tex_user_data;
	WMtDialog *						dialog;
	WMtWindow *						checkbox;
	WMtPopupMenu *					menu;

	handled = UUcTrue;
	user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// update all of the fields showing the particle's appearance values
			OWiParticle_Appearance_UpdateFields(inDialog);

			// set the 'invisible' checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_Invisible);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags2 &
								P3cParticleClassFlag2_Appearance_Invisible) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the 'initially hidden' checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_InitiallyHidden);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags2 &
								P3cParticleClassFlag2_Appearance_InitiallyHidden) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the geometry/texture popup menu appropriately
			menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_DisplayType);
			if (menu != NULL) {
				if (user_data->classptr->definition->flags & P3cParticleClassFlag_Appearance_Geometry) {
					WMrPopupMenu_SetSelection(menu, OWcParticle_DisplayType_Geometry);

				} else if (user_data->classptr->definition->flags2 & P3cParticleClassFlag2_Appearance_Vector) {
					WMrPopupMenu_SetSelection(menu, OWcParticle_DisplayType_Vector);

				} else if (user_data->classptr->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal) {
					WMrPopupMenu_SetSelection(menu, OWcParticle_DisplayType_Decal);

				} else {
					WMrPopupMenu_SetSelection(menu, OWcParticle_DisplayType_Sprite);
				}
			}

			// set the Y-scale checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_YScaleEnable);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags &
								P3cParticleClassFlag_Appearance_UseSeparateYScale) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the 'scale to velocity' checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_ScaleToVelocity);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags &
								P3cParticleClassFlag_Appearance_ScaleToVelocity) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the edge-fade checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_EdgeFade);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags2 &
								P3cParticleClassFlag2_Appearance_EdgeFade) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the one-sided edge-fade checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_EdgeFade1Sided);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags2 &
								P3cParticleClassFlag2_Appearance_EdgeFade_OneSided) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the lensflare checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_LensFlare);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags2 &
								P3cParticleClassFlag2_Appearance_LensFlare) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the lensflare fading checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_LensFade);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags &
								P3cParticleClassFlag_HasLensFrames) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the texture index checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_TextureIndex);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags &
								P3cParticleClassFlag_HasTextureIndex) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the texture time checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_TextureTime);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags &
								P3cParticleClassFlag_HasTextureTime) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the decal fullbright checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_DecalFullBright);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags2 &
								P3cParticleClassFlag2_Appearance_DecalFullBright) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the sky checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_Sky);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags2 &
								P3cParticleClassFlag2_Appearance_Sky) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// set the sprite type popup menu appropriately
			menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_SpriteType);
			if (menu != NULL) {
				selection = (user_data->classptr->definition->flags &
						P3cParticleClassFlag_Appearance_SpriteTypeMask) >> P3cParticleClassFlags_SpriteTypeShift;

				WMrPopupMenu_SetSelection(menu, (UUtUns16) selection);
			}

			// register that we are here and may need to be notified about changes
			OWiParticle_RegisterOpenWindow(inDialog);
		break;
		
		case WMcMessage_Destroy:
			// deallocate user data
			UUrMemory_Block_Delete(user_data);

			// register that we are going away and shouldn't be notified about any further changes
			OWiParticle_RegisterCloseWindow(inDialog);
		break;

		case OWcMessage_Particle_UpdateList:
			// one of our variables has changed. update all of them.
			OWiParticle_Appearance_UpdateFields(inDialog);			
			break;

		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case OWcParticle_Appearance_XScale_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.scale;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s scale", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_XOffset_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.x_offset;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s X offset", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_XShorten_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.x_shorten;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s X shortening", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_YScale_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.y_scale;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s Y scale", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_Rotation_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.rotation;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s rotation", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_Alpha_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.alpha;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s alpha", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_Tint_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.tint;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s tint", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Shade, NULL,
									OWrParticle_Value_Shade_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_EdgeFadeMin_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.edgefade_min;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s edge-fade minval", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_EdgeFadeMax_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.edgefade_max;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s edge-fade maxval", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_MaxContrail_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.max_contrail;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s max contrail length", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_LensDist_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.lensflare_dist;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s lensflare seethrough distance", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_DecalWrap_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->appearance.decal_wrap;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s decal wrap angle (0-180)", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Appearance_LensIn_Set:
					// set up data for the lensflare in-frames dialog
					number_user_data = (OWtParticle_Number_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Number_Data));
					number_user_data->classptr = user_data->classptr;
					number_user_data->notify_struct.notify_window = inDialog;
					number_user_data->notify_struct.line_to_notify = 0;
					sprintf(number_user_data->title, "Edit %s lensflare fade-in frames", user_data->classptr->classname);
					strcpy(number_user_data->editmsg, "Fade frames:");
					number_user_data->is_float = UUcFalse;
					number_user_data->ptr.numberptr = &user_data->classptr->definition->appearance.lens_inframes;

					// create the window
					WMrDialog_Create(OWcDialog_Particle_Number, NULL,
									OWrParticle_Number_Callback, (UUtUns32) number_user_data, &dialog);
					break;

				case OWcParticle_Appearance_MaxDecals_Set:
					// set up data for the max-decals dialog
					number_user_data = (OWtParticle_Number_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Number_Data));
					number_user_data->classptr = user_data->classptr;
					number_user_data->notify_struct.notify_window = inDialog;
					number_user_data->notify_struct.line_to_notify = 0;
					sprintf(number_user_data->title, "Edit %s max decals", user_data->classptr->classname);
					strcpy(number_user_data->editmsg, "Max Decals:");
					number_user_data->is_float = UUcFalse;
					number_user_data->ptr.numberptr = &user_data->classptr->definition->appearance.max_decals;

					// create the window
					WMrDialog_Create(OWcDialog_Particle_Number, NULL,
									OWrParticle_Number_Callback, (UUtUns32) number_user_data, &dialog);
					break;

				case OWcParticle_Appearance_DecalFade_Set:
					// set up data for the decal fade-frames dialog
					number_user_data = (OWtParticle_Number_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Number_Data));
					number_user_data->classptr = user_data->classptr;
					number_user_data->notify_struct.notify_window = inDialog;
					number_user_data->notify_struct.line_to_notify = 0;
					sprintf(number_user_data->title, "Edit %s decal fade frames", user_data->classptr->classname);
					strcpy(number_user_data->editmsg, "Decal Fade Frames:");
					number_user_data->is_float = UUcFalse;
					number_user_data->ptr.numberptr = &user_data->classptr->definition->appearance.decal_fadeframes;

					// create the window
					WMrDialog_Create(OWcDialog_Particle_Number, NULL,
									OWrParticle_Number_Callback, (UUtUns32) number_user_data, &dialog);
					break;

				case OWcParticle_Appearance_LensOut_Set:
					// set up data for the lensflare out-frames dialog
					number_user_data = (OWtParticle_Number_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Number_Data));
					number_user_data->classptr = user_data->classptr;
					number_user_data->notify_struct.notify_window = inDialog;
					number_user_data->notify_struct.line_to_notify = 0;
					sprintf(number_user_data->title, "Edit %s lensflare fade-out frames", user_data->classptr->classname);
					strcpy(number_user_data->editmsg, "Fade frames:");
					number_user_data->is_float = UUcFalse;
					number_user_data->ptr.numberptr = &user_data->classptr->definition->appearance.lens_outframes;

					// create the window
					WMrDialog_Create(OWcDialog_Particle_Number, NULL,
									OWrParticle_Number_Callback, (UUtUns32) number_user_data, &dialog);
					break;

				case OWcParticle_Appearance_Texture_Set:
					// set up data for the texture name edit dialog
					tex_user_data = (OWtParticle_Texture_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Texture_Data));
					tex_user_data->classptr = user_data->classptr;
					tex_user_data->texture_name = user_data->classptr->definition->appearance.instance;
					sprintf(tex_user_data->title, "Select %s ", &user_data->classptr->classname);
					if (user_data->classptr->definition->flags & P3cParticleClassFlag_Appearance_Geometry) 
					{
						strcat(tex_user_data->title,	"geometry");
						strcpy(tex_user_data->editmsg,	"Geometry Instance:");
						tex_user_data->name_type = OWcParticle_NameType_Geometry;
					} 
					else
					{
						if (user_data->classptr->definition->flags2 & P3cParticleClassFlag2_Appearance_Decal) 
						{
							strcat(tex_user_data->title,	"decal");
						}
						else 
						{
							strcat(tex_user_data->title,	"sprite");
						}
						strcpy(tex_user_data->editmsg,	"Texture Name:");
						tex_user_data->name_type = OWcParticle_NameType_Texture;
					}

					tex_user_data->notify_struct.line_to_notify = 0;
					tex_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Texture, NULL,
									OWrParticle_Texture_Callback, (UUtUns32) tex_user_data, &dialog);
					break;

				case OWcParticle_Appearance_YScaleEnable:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_YScaleEnable);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags = user_data->classptr->definition->flags
							& ~P3cParticleClassFlag_Appearance_UseSeparateYScale;
						if (checked)
							new_flags |= P3cParticleClassFlag_Appearance_UseSeparateYScale;

						if (new_flags != user_data->classptr->definition->flags) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags = new_flags;
	
							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;

				case OWcParticle_Appearance_ScaleToVelocity:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_ScaleToVelocity);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags = user_data->classptr->definition->flags
							& ~P3cParticleClassFlag_Appearance_ScaleToVelocity;
						if (checked)
							new_flags |= P3cParticleClassFlag_Appearance_ScaleToVelocity;

						if (new_flags != user_data->classptr->definition->flags) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags = new_flags;
	
							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;

				case OWcParticle_Appearance_EdgeFade:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_EdgeFade);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags2 = user_data->classptr->definition->flags2
							& ~P3cParticleClassFlag2_Appearance_EdgeFade;
						if (checked)
							new_flags2 |= P3cParticleClassFlag2_Appearance_EdgeFade;

						if (new_flags2 != user_data->classptr->definition->flags2) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags2 = new_flags2;
	
							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;

				case OWcParticle_Appearance_EdgeFade1Sided:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_EdgeFade1Sided);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags2 = user_data->classptr->definition->flags2
							& ~P3cParticleClassFlag2_Appearance_EdgeFade_OneSided;
						if (checked)
							new_flags2 |= P3cParticleClassFlag2_Appearance_EdgeFade_OneSided;

						if (new_flags2 != user_data->classptr->definition->flags2) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags2 = new_flags2;
	
							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;

				case OWcParticle_Appearance_LensFlare:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_LensFlare);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags2 = user_data->classptr->definition->flags2
							& ~P3cParticleClassFlag2_Appearance_LensFlare;
						if (checked)
							new_flags2 |= P3cParticleClassFlag2_Appearance_LensFlare;

						if (new_flags2 != user_data->classptr->definition->flags2) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags2 = new_flags2;
	
							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;

				case OWcParticle_Appearance_LensFade:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_LensFade);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags = user_data->classptr->definition->flags
							& ~P3cParticleClassFlag_HasLensFrames;
						if (checked)
							new_flags |= P3cParticleClassFlag_HasLensFrames;

						if (new_flags != user_data->classptr->definition->flags) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags = new_flags;
	
							// we now have to re-pack the variables inside each particle
							P3rPackVariables(user_data->classptr, UUcTrue);

							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;

				case OWcParticle_Appearance_Invisible:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_Invisible);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags2 = user_data->classptr->definition->flags2
							& ~P3cParticleClassFlag2_Appearance_Invisible;
						if (checked)
							new_flags2 |= P3cParticleClassFlag2_Appearance_Invisible;

						if (new_flags2 != user_data->classptr->definition->flags2) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags2 = new_flags2;
	
							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;

				case OWcParticle_Appearance_InitiallyHidden:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_InitiallyHidden);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags2 = user_data->classptr->definition->flags2
							& ~P3cParticleClassFlag2_Appearance_InitiallyHidden;
						if (checked)
							new_flags2 |= P3cParticleClassFlag2_Appearance_InitiallyHidden;

						if (new_flags2 != user_data->classptr->definition->flags2) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags2 = new_flags2;
	
							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;

				case OWcParticle_Appearance_TextureIndex:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_TextureIndex);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags = user_data->classptr->definition->flags
							& ~P3cParticleClassFlag_HasTextureIndex;
						if (checked)
							new_flags |= P3cParticleClassFlag_HasTextureIndex;

						if (new_flags != user_data->classptr->definition->flags) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags = new_flags;
	
							// we now have to re-pack the variables inside each particle
							// because there is or isn't an additional texture index to store
							P3rPackVariables(user_data->classptr, UUcTrue);

							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;

				case OWcParticle_Appearance_TextureTime:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_TextureTime);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags = user_data->classptr->definition->flags
							& ~P3cParticleClassFlag_HasTextureTime;
						if (checked)
							new_flags |= P3cParticleClassFlag_HasTextureTime;

						if (new_flags != user_data->classptr->definition->flags) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags = new_flags;
	
							// we may now need to autosave
							OBDrMakeDirty();

							// we now have to re-pack the variables inside each particle
							// because there is or isn't an additional texture index to store
							P3rPackVariables(user_data->classptr, UUcTrue);
						}
					}
					break;

				case OWcParticle_Appearance_DecalFullBright:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_DecalFullBright);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags2 = user_data->classptr->definition->flags2
							& ~P3cParticleClassFlag2_Appearance_DecalFullBright;
						if (checked)
							new_flags2 |= P3cParticleClassFlag2_Appearance_DecalFullBright;

						if (new_flags2 != user_data->classptr->definition->flags2) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags2 = new_flags2;
	
							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;

				case OWcParticle_Appearance_Sky:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Appearance_Sky);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags2 = user_data->classptr->definition->flags2
							& ~P3cParticleClassFlag2_Appearance_Sky;
						if (checked)
							new_flags2 |= P3cParticleClassFlag2_Appearance_Sky;

						if (new_flags2 != user_data->classptr->definition->flags2) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags2 = new_flags2;
	
							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;
			}
		break;
		
		case WMcMessage_MenuCommand:
			menu = (WMtMenu *) inParam2;
			switch (WMrWindow_GetID(menu))
			{
			case OWcParticle_Appearance_SpriteType:
				// get the current sprite type
				WMrPopupMenu_GetItemID(menu, -1, &selected_id);

				if (selected_id != (UUtUns16) -1) {
					// check this is a valid selection
					UUmAssert((selected_id >= 0) && (selected_id < 8));
					
					new_flags = user_data->classptr->definition->flags
						& ~P3cParticleClassFlag_Appearance_SpriteTypeMask;
					new_flags |= ((UUtUns32) selected_id) << P3cParticleClassFlags_SpriteTypeShift;
					
					if (new_flags != user_data->classptr->definition->flags) {
						// this particle class is now dirty. NB: must be called BEFORE we make any changes.
						P3rMakeDirty(user_data->classptr, UUcTrue);

						user_data->classptr->definition->flags = new_flags;
						
						// we may now need to autosave
						OBDrMakeDirty();
					}
				}
				break;

			case OWcParticle_Appearance_DisplayType:
				// get the current display type
				WMrPopupMenu_GetItemID(menu, -1, &selected_id);

				new_flags = user_data->classptr->definition->flags &
					~(P3cParticleClassFlag_Appearance_Geometry | P3cParticleClassFlag_HasDecalData);
				new_flags2 = user_data->classptr->definition->flags2 &
					~(P3cParticleClassFlag2_Appearance_Decal | P3cParticleClassFlag2_Appearance_Vector);

				if (selected_id == OWcParticle_DisplayType_Geometry) 
				{
					new_flags |= P3cParticleClassFlag_Appearance_Geometry;
				} 
				else if (selected_id == OWcParticle_DisplayType_Vector) 
				{
					new_flags2 |= P3cParticleClassFlag2_Appearance_Vector;
				} 
				else if (selected_id == OWcParticle_DisplayType_Decal) 
				{
					new_flags  |= P3cParticleClassFlag_HasDecalData | P3cParticleClassFlag_Decorative;
					new_flags2 |= P3cParticleClassFlag2_Appearance_Decal;
				}

				if ((new_flags != user_data->classptr->definition->flags) || (new_flags2 != user_data->classptr->definition->flags2))
				{
					// this particle class is now dirty. NB: must be called BEFORE we make any changes.
					P3rMakeDirty(user_data->classptr, UUcTrue);

					user_data->classptr->definition->flags  = new_flags;
					user_data->classptr->definition->flags2 = new_flags2;
						
					// we may now need to autosave
					OBDrMakeDirty();

					P3rPackVariables(user_data->classptr, UUcTrue);

				}
				break;
			}
			
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

static void OWiParticle_SetCheckbox(WMtDialog *inDialog, P3tParticleClass *inClass,
									UUtUns16 inCheckBox, UUtUns32 inFlagVar, UUtUns32 inFlag)
{
	WMtWindow *checkbox;
	UUtUns32 flags = (inFlagVar == 1) ? inClass->definition->flags : inClass->definition->flags2;

	checkbox = WMrDialog_GetItemByID(inDialog, inCheckBox);
	if (checkbox != NULL) {
		WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) ((flags & inFlag) ? UUcTrue : UUcFalse), (UUtUns32) -1);
	}
}

static UUtBool OWiParticle_UpdateCheckbox(WMtDialog *inDialog, P3tParticleClass *inClass,
										UUtUns16 inCheckBox, UUtUns32 inFlagVar, UUtUns32 inFlag, UUtBool inRepack)
{
	WMtWindow *checkbox;
	UUtUns32 new_flags, *flagptr;
	UUtBool checked;

	flagptr = (inFlagVar == 1) ? &inClass->definition->flags : &inClass->definition->flags2;

	checkbox = WMrDialog_GetItemByID(inDialog, inCheckBox);
	if (checkbox != NULL) {
		checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue, (UUtUns32) -1, (UUtUns32) -1);

		new_flags = (*flagptr) & ~inFlag;
		if (checked)
			new_flags |= inFlag;

		if (new_flags != *flagptr) {
			// this particle class is now dirty. NB: must be called BEFORE we make any changes.
			P3rMakeDirty(inClass, UUcTrue);

			*flagptr = new_flags;
	
			// we may now need to autosave
			OBDrMakeDirty();

			if (inRepack) {
				// we now have to re-pack the variables inside each particle
				// because there is or isn't an additional variable to store
				P3rPackVariables(inClass, UUcTrue);
			}

			return UUcTrue;
		}
	}

	return UUcFalse;
}

static void OWiParticle_Attractor_UpdateFields(WMtDialog *inDialog)
{
	OWtParticle_Edit_Data *			user_data;
	WMtWindow *						text_string;
	char							textbuf[196];
	char							valuebuf[128];

	user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);

	// set up the attractor name
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Attractor_Name);
	if (text_string != NULL) {
		sprintf(textbuf, "Class/Tag Name: %s", user_data->classptr->definition->attractor.attractor_name);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}
	
	// set up the max distance
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Attractor_Max_Distance);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->attractor.max_distance, valuebuf);
		sprintf(textbuf, "Max Distance: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the max angle
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Attractor_Max_Angle);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->attractor.max_angle, valuebuf);
		sprintf(textbuf, "Max Angle: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the min angle for angle-selection
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Attractor_AngleSelect_Min);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->attractor.select_thetamin, valuebuf);
		sprintf(textbuf, "AngleSelect Min: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the max angle for angle-selection
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Attractor_AngleSelect_Max);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->attractor.select_thetamax, valuebuf);
		sprintf(textbuf, "AngleSelect Max: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// set up the weight for angle-selection
	text_string = WMrDialog_GetItemByID(inDialog, OWcParticle_Attractor_AngleSelect_Weight);
	if (text_string != NULL) {
		OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->attractor.select_thetaweight, valuebuf);
		sprintf(textbuf, "AngleSelect Weight: %s", valuebuf);
		WMrWindow_SetTitle(text_string, textbuf, WMcMaxTitleLength);
	}

	// recalculate the attractor pointer for this class
	P3rSetupAttractorPointers(user_data->classptr);
}

UUtBool
OWrParticle_Attractor_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled, checked;
	UUtUns16						selected_id, itr;
	UUtUns32						new_flags2;
	OWtParticle_Edit_Data *			user_data;
	OWtParticle_Value_Data *		val_user_data;
	OWtParticle_Texture_Data *		tex_user_data;
	WMtDialog *						dialog;
	WMtWindow *						checkbox;
	WMtPopupMenu *					menu;

	handled = UUcTrue;
	user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// update all of the fields showing the particle's attractor values
			OWiParticle_Attractor_UpdateFields(inDialog);

			// fill the iterator and selector type menus
			menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Attractor_Iterator);
			for (itr = 0; itr < P3cAttractorIterator_Max; itr++) {
				WMrPopupMenu_AppendItem_Light(menu, itr, P3gAttractorIteratorTable[itr].name);
			}
			WMrPopupMenu_SetSelection(menu, (UUtUns16) user_data->classptr->definition->attractor.iterator_type);

			menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Attractor_Selector);
			for (itr = 0; itr < P3cAttractorSelector_Max; itr++) {
				WMrPopupMenu_AppendItem_Light(menu, itr, P3gAttractorSelectorTable[itr].name);
			}
			WMrPopupMenu_SetSelection(menu, (UUtUns16) user_data->classptr->definition->attractor.selector_type);

			// set the check-walls checkbox appropriately
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Attractor_CheckWalls);
			if (checkbox != NULL) {
				WMrMessage_Send(checkbox, CBcMessage_SetCheck,
					(UUtUns32) ((user_data->classptr->definition->flags2 &
								P3cParticleClassFlag2_Attractor_CheckEnvironment) ? UUcTrue : UUcFalse),
					(UUtUns32) -1);
			}

			// register that we are here and may need to be notified about changes
			OWiParticle_RegisterOpenWindow(inDialog);
		break;
		
		case WMcMessage_Destroy:
			// deallocate user data
			UUrMemory_Block_Delete(user_data);

			// register that we are going away and shouldn't be notified about any further changes
			OWiParticle_RegisterCloseWindow(inDialog);
		break;

		case OWcMessage_Particle_UpdateList:
			// one of our variables has changed. update all of them.
			OWiParticle_Attractor_UpdateFields(inDialog);			
			break;

		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case OWcParticle_Attractor_Max_Distance_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->attractor.max_distance;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s attractor max distance", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Attractor_Max_Angle_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->attractor.max_angle;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s attractor max angle", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Attractor_AngleSelect_Min_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->attractor.select_thetamin;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s attractor angle-select min angle", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Attractor_AngleSelect_Max_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->attractor.select_thetamax;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s attractor angle-select max angle", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Attractor_AngleSelect_Weight_Set:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->attractor.select_thetaweight;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s attractor angle-select weight", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Attractor_Name_Set:
					// set up data for the class name edit dialog
					tex_user_data = (OWtParticle_Texture_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Texture_Data));
					tex_user_data->classptr = user_data->classptr;
					tex_user_data->texture_name = user_data->classptr->definition->attractor.attractor_name;
					sprintf(tex_user_data->title, "Select %s attractor ", &user_data->classptr->classname);
					if (user_data->classptr->definition->attractor.iterator_type == P3cAttractorIterator_ParticleClass)
					{
						strcat(tex_user_data->title,	"class");
						strcpy(tex_user_data->editmsg,	"Class Name:");
						tex_user_data->name_type = OWcParticle_NameType_ParticleClass;
					} 
					else if (user_data->classptr->definition->attractor.iterator_type == P3cAttractorIterator_ParticleTag)
					{
						strcat(tex_user_data->title,	"tag");
						strcpy(tex_user_data->editmsg,	"Attractor Tag:");
						tex_user_data->name_type = OWcParticle_NameType_String;
					}
					else
					{
						// this button does nothing
						UUrMemory_Block_Delete(tex_user_data);
						break;
					}

					tex_user_data->notify_struct.line_to_notify = 0;
					tex_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Texture, NULL,
									OWrParticle_Texture_Callback, (UUtUns32) tex_user_data, &dialog);
					break;

				case OWcParticle_Attractor_CheckWalls:
					// get the current check value
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Attractor_CheckWalls);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);

						new_flags2 = user_data->classptr->definition->flags2
							& ~P3cParticleClassFlag2_Attractor_CheckEnvironment;
						if (checked)
							new_flags2 |= P3cParticleClassFlag2_Attractor_CheckEnvironment;

						if (new_flags2 != user_data->classptr->definition->flags2) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							user_data->classptr->definition->flags2 = new_flags2;
	
							// we may now need to autosave
							OBDrMakeDirty();
						}
					}
					break;
			}
		break;
		
		case WMcMessage_MenuCommand:
			menu = (WMtMenu *) inParam2;
			switch (WMrWindow_GetID(menu))
			{
			case OWcParticle_Attractor_Iterator:
				// get the current sprite type
				WMrPopupMenu_GetItemID(menu, -1, &selected_id);

				if (selected_id != (UUtUns16) -1) {
					// check this is a valid selection
					UUmAssert((selected_id >= 0) && (selected_id < P3cAttractorIterator_Max));
					
					if (selected_id != user_data->classptr->definition->attractor.iterator_type) {
						// this particle class is now dirty. NB: must be called BEFORE we make any changes.
						P3rMakeDirty(user_data->classptr, UUcTrue);

						user_data->classptr->definition->attractor.iterator_type = selected_id;
						
						// we may now need to autosave
						OBDrMakeDirty();
					}
				}
				break;

			case OWcParticle_Attractor_Selector:
				// get the current sprite type
				WMrPopupMenu_GetItemID(menu, -1, &selected_id);

				if (selected_id != (UUtUns16) -1) {
					// check this is a valid selection
					UUmAssert((selected_id >= 0) && (selected_id < P3cAttractorSelector_Max));
					
					if (selected_id != user_data->classptr->definition->attractor.selector_type) {
						// this particle class is now dirty. NB: must be called BEFORE we make any changes.
						P3rMakeDirty(user_data->classptr, UUcTrue);

						user_data->classptr->definition->attractor.selector_type = selected_id;
						
						// we may now need to autosave
						OBDrMakeDirty();
					}
				}
				break;
			}
			
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_Class_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	UUtUns16						selection;
	UUtUns32						new_flags2;
	OWtParticle_Edit_Data			*user_data, *new_user_data;
	OWtParticle_Emitter_Data *		emitter_user_data;
	WMtDialog *						dialog;
	WMtPopupMenu *					menu;
	WMtWindow 						*text;
	char							title[64], tempstring[64];
	OWtParticle_Value_Data *		val_user_data;
	OWtParticle_Number_Data *		number_user_data;
	SStImpulse *					impulse_sound;

	handled = UUcTrue;
	user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// set the dialog title
			sprintf(title, "Edit %s", user_data->classptr->classname);
			WMrWindow_SetTitle(inDialog, title, WMcMaxTitleLength);

			// set the lifetime description text
			text = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_LifetimeText);
			if (text != NULL) {
				OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->lifetime, tempstring);
				sprintf(title, "Lifetime: %s", tempstring);
				WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
			}

			// set the collision radius description text
			text = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_RadiusText);
			if (text != NULL) {
				OWiDescribeValue(user_data->classptr, P3cDataType_Float,
								&user_data->classptr->definition->collision_radius, tempstring);
				sprintf(title, "Collision Radius: %s", tempstring);
				WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
			}

			// set the dodge radius description text
			text = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_DodgeText);
			if (text != NULL) {
				sprintf(title, "Dodge Radius: %.1f", user_data->classptr->definition->dodge_radius);
				WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
			}

			// set the alert radius description text
			text = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_AlertText);
			if (text != NULL) {
				sprintf(title, "Alert Radius: %.1f", user_data->classptr->definition->alert_radius);
				WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
			}

			// set the flyby sound description text
			text = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_FlybyText);
			if (text != NULL) {
				sprintf(title, "Flyby Sound: %s", user_data->classptr->definition->flyby_soundname);
				WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
			}

			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasVelocity,			1,	P3cParticleClassFlag_HasVelocity);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasOrientation,		1,	P3cParticleClassFlag_HasOrientation);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasOffsetPos,			1,	P3cParticleClassFlag_HasOffsetPos);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasDynamicMatrix,		1,	P3cParticleClassFlag_HasDynamicMatrix);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_DeltaPosCollision,		1,	P3cParticleClassFlag_HasPrevPosition);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasContrailData,		1,	P3cParticleClassFlag_HasContrailData);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_Decorative,			1,	P3cParticleClassFlag_Decorative);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_CollideEnv,			1,	P3cParticleClassFlag_Physics_CollideEnv);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_CollideChar,			1,	P3cParticleClassFlag_Physics_CollideChar);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasDamageOwner,		1,	P3cParticleClassFlag_HasOwner);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_IsAttracted,			1,	P3cParticleClassFlag_HasAttractor);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_ExpireOnCutscene,		2,	P3cParticleClassFlag2_ExpireOnCutscene);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_DieOnCutscene,			2,	P3cParticleClassFlag2_DieOnCutscene);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_GlobalTint,			2,	P3cParticleClassFlag2_UseGlobalTint);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_LockToLinked,			2,	P3cParticleClassFlag2_LockToLinkedParticle);
			OWiParticle_SetCheckbox(inDialog, user_data->classptr, OWcParticle_Class_IsContrailEmitter,		2,	P3cParticleClassFlag2_ContrailEmitter);

			menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWcParticle_Class_Disable);
			if (menu != NULL) {
				if (user_data->classptr->definition->flags2 & P3cParticleClassFlag2_MediumDetailDisable) {
					selection = OWcParticle_Disable_MediumDetail;
				} else if (user_data->classptr->definition->flags2 & P3cParticleClassFlag2_LowDetailDisable) {
					selection = OWcParticle_Disable_LowDetail;
				} else {
					selection = OWcParticle_Disable_Never;
				}

				WMrPopupMenu_SetSelection(menu, selection);
			}

			// register that we are here and may need to be notified about changes
			OWiParticle_RegisterOpenWindow(inDialog);
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);

			// register that we are going away and shouldn't be notified about any further changes
			OWiParticle_RegisterCloseWindow(inDialog);
		break;
		
		case OWcMessage_Particle_UpdateList:
			// set the lifetime description text
			text = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_LifetimeText);
			if (text != NULL) {
				OWiDescribeValue(user_data->classptr, P3cDataType_Float, &user_data->classptr->definition->lifetime, tempstring);
				sprintf(title, "Lifetime: %s", tempstring);
				WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
			}

			// set the collision radius description text
			text = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_RadiusText);
			if (text != NULL) {
				OWiDescribeValue(user_data->classptr, P3cDataType_Float,
								&user_data->classptr->definition->collision_radius, tempstring);
				sprintf(title, "Collision Radius: %s", tempstring);
				WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
			}

			// set the dodge radius description text
			text = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_DodgeText);
			if (text != NULL) {
				sprintf(title, "Dodge Radius: %.1f", user_data->classptr->definition->dodge_radius);
				WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
			}

			// set the alert radius description text
			text = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_AlertText);
			if (text != NULL) {
				sprintf(title, "Alert Radius: %.1f", user_data->classptr->definition->alert_radius);
				WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
			}

			// set the flyby sound description text
			text = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_FlybyText);
			if (text != NULL) {
				sprintf(title, "Flyby Sound: %s", user_data->classptr->definition->flyby_soundname);
				WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
			}

			break;

		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case OWcParticle_Class_Save:
					if (P3rIsDirty(user_data->classptr)) {
						OWrSave_Particle(user_data->classptr);
					}
					break;

				case OWcParticle_Class_Revert:
					if (P3rIsDirty(user_data->classptr)) {
						OWrRevert_Particle(user_data->classptr);
						WMrWindow_Delete(inDialog);
					}
					break;

				case OWcParticle_Class_Variables:
					new_user_data = (OWtParticle_Edit_Data *) UUrMemory_Block_New(sizeof(OWtParticle_Edit_Data));

					// pop up the variable editing dialog
					new_user_data->classptr = user_data->classptr;

					WMrDialog_Create(OWcDialog_Particle_Variables, NULL, OWrParticle_Variables_Callback,
									(UUtUns32) new_user_data, &dialog);
					break;

				case OWcParticle_Class_Events:
					new_user_data = (OWtParticle_Edit_Data *) UUrMemory_Block_New(sizeof(OWtParticle_Edit_Data));

					// pop up the event list dialog
					new_user_data->classptr = user_data->classptr;

					WMrDialog_Create(OWcDialog_Particle_Events, NULL, OWrParticle_EventList_Callback,
									(UUtUns32) new_user_data, &dialog);
					break;

				case OWcParticle_Class_Appearance:
					new_user_data = (OWtParticle_Edit_Data *) UUrMemory_Block_New(sizeof(OWtParticle_Edit_Data));

					// pop up the appearance editing dialog
					new_user_data->classptr = user_data->classptr;

					WMrDialog_Create(OWcDialog_Particle_Appearance, NULL, OWrParticle_Appearance_Callback,
									(UUtUns32) new_user_data, &dialog);
					break;

				case OWcParticle_Class_Emitters:
					emitter_user_data = (OWtParticle_Emitter_Data *) UUrMemory_Block_New(sizeof(OWtParticle_Emitter_Data));

					// pop up the emitters editing dialog
					emitter_user_data->classptr = user_data->classptr;

					WMrDialog_Create(OWcDialog_Particle_Emitters, NULL, OWrParticle_Emitters_Callback,
									(UUtUns32) emitter_user_data, &dialog);
					break;

				case OWcParticle_Class_Attractor:
					new_user_data = (OWtParticle_Edit_Data *) UUrMemory_Block_New(sizeof(OWtParticle_Edit_Data));

					// pop up the appearance editing dialog
					new_user_data->classptr = user_data->classptr;

					WMrDialog_Create(OWcDialog_Particle_Attractor, NULL, OWrParticle_Attractor_Callback,
									(UUtUns32) new_user_data, &dialog);
					break;

				case OWcParticle_Class_HasVelocity:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasVelocity,	1,	P3cParticleClassFlag_HasVelocity,	UUcTrue);
					break;

				case OWcParticle_Class_HasOrientation:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasOrientation, 1,	P3cParticleClassFlag_HasOrientation, UUcTrue);
					break;

				case OWcParticle_Class_HasOffsetPos:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasOffsetPos,	1,	P3cParticleClassFlag_HasOffsetPos,	UUcTrue);
					break;

				case OWcParticle_Class_HasDynamicMatrix:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasDynamicMatrix,1,	P3cParticleClassFlag_HasDynamicMatrix,UUcTrue);
					break;

				case OWcParticle_Class_HasContrailData:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasContrailData, 1,	P3cParticleClassFlag_HasContrailData,UUcTrue);
					break;

				case OWcParticle_Class_DeltaPosCollision:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_DeltaPosCollision,1,	P3cParticleClassFlag_HasPrevPosition,UUcTrue);
					break;

				case OWcParticle_Class_Decorative:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_Decorative,		1,	P3cParticleClassFlag_Decorative,		UUcTrue);
					break;

				case OWcParticle_Class_CollideEnv:
					// CB: note that environment collision now requires an environment cache
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_CollideEnv,		1,
												(P3cParticleClassFlag_Physics_CollideEnv | P3cParticleClassFlag_HasEnvironmentCache), UUcTrue);						
					break;

				case OWcParticle_Class_CollideChar:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_CollideChar,	1,	P3cParticleClassFlag_Physics_CollideChar,UUcFalse);
					break;

				case OWcParticle_Class_HasDamageOwner:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_HasDamageOwner, 1,	P3cParticleClassFlag_HasOwner,UUcTrue);
					break;

				case OWcParticle_Class_IsAttracted:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_IsAttracted,	1,	P3cParticleClassFlag_HasAttractor,UUcTrue);
					break;

				case OWcParticle_Class_ExpireOnCutscene:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_ExpireOnCutscene,	2,	P3cParticleClassFlag2_ExpireOnCutscene,UUcTrue);
					break;

				case OWcParticle_Class_DieOnCutscene:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_DieOnCutscene,	2,	P3cParticleClassFlag2_DieOnCutscene,UUcTrue);
					break;

				case OWcParticle_Class_GlobalTint:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_GlobalTint,		2,	P3cParticleClassFlag2_UseGlobalTint,UUcFalse);
					break;

				case OWcParticle_Class_LockToLinked:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_LockToLinked,	2,	P3cParticleClassFlag2_LockToLinkedParticle,UUcFalse);
					break;

				case OWcParticle_Class_IsContrailEmitter:
					OWiParticle_UpdateCheckbox(inDialog, user_data->classptr, OWcParticle_Class_IsContrailEmitter, 2,	P3cParticleClassFlag2_ContrailEmitter,UUcFalse);
					break;

				case OWcParticle_Class_LifetimeButton:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->lifetime;
					val_user_data->variable_allowed = UUcFalse;
					sprintf(val_user_data->title, "Edit %s lifetime", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 0;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Class_RadiusButton:
					// set up data for the value edit dialog
					val_user_data = (OWtParticle_Value_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
					val_user_data->classptr = user_data->classptr;
					val_user_data->valptr = &user_data->classptr->definition->collision_radius;
					val_user_data->variable_allowed = UUcTrue;
					sprintf(val_user_data->title, "Edit %s collision radius", &user_data->classptr->classname);

					val_user_data->notify_struct.line_to_notify = 1;
					val_user_data->notify_struct.notify_window	= inDialog;

					WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
					break;

				case OWcParticle_Class_DodgeButton:
					// set up data for the dodge radius dialog
					number_user_data = (OWtParticle_Number_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Number_Data));
					number_user_data->classptr = user_data->classptr;
					number_user_data->notify_struct.notify_window = inDialog;
					number_user_data->notify_struct.line_to_notify = 2;
					sprintf(number_user_data->title, "Edit %s dodge radius", user_data->classptr->classname);
					strcpy(number_user_data->editmsg, "Dodge Radius:");
					number_user_data->is_float = UUcTrue;
					number_user_data->ptr.floatptr = &user_data->classptr->definition->dodge_radius;

					// create the window
					WMrDialog_Create(OWcDialog_Particle_Number, NULL,
									OWrParticle_Number_Callback, (UUtUns32) number_user_data, &dialog);
					break;

				case OWcParticle_Class_AlertButton:
					// set up data for the dodge radius dialog
					number_user_data = (OWtParticle_Number_Data *)
						UUrMemory_Block_New(sizeof(OWtParticle_Number_Data));
					number_user_data->classptr = user_data->classptr;
					number_user_data->notify_struct.notify_window = inDialog;
					number_user_data->notify_struct.line_to_notify = 3;
					sprintf(number_user_data->title, "Edit %s alert radius", user_data->classptr->classname);
					strcpy(number_user_data->editmsg, "Alert Radius:");
					number_user_data->is_float = UUcTrue;
					number_user_data->ptr.floatptr = &user_data->classptr->definition->alert_radius;

					// create the window
					WMrDialog_Create(OWcDialog_Particle_Number, NULL,
									OWrParticle_Number_Callback, (UUtUns32) number_user_data, &dialog);
					break;

				case OWcParticle_Class_FlybyButton:
				{
					OWtSelectResult	result;
					
					// select the impulse sound
					result = OWrSelect_ImpulseSound(&impulse_sound);
					if (result == OWcSelectResult_Cancel) {
						break;
					}

					P3rMakeDirty(user_data->classptr, UUcTrue);

					if (impulse_sound == NULL) {
						user_data->classptr->definition->flyby_soundname[0] = '\0';
						user_data->classptr->flyby_sound = NULL;
					} else {
						UUrString_Copy_Length(user_data->classptr->definition->flyby_soundname, impulse_sound->impulse_name,
												P3cStringVarSize, P3cStringVarSize);
						user_data->classptr->flyby_sound = OSrImpulse_GetByName(user_data->classptr->definition->flyby_soundname);
					}

					OBDrMakeDirty();

					// set the flyby sound description text
					text = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_FlybyText);
					if (text != NULL) {
						sprintf(title, "Flyby Sound: %s", user_data->classptr->definition->flyby_soundname);
						WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
					}
					break;
				}
			}
		break;
		
		case WMcMessage_MenuCommand:
			switch (WMrWindow_GetID((WMtWindow *) inParam2))
			{
				case OWcParticle_Class_Disable:
				{
					// get the current disable level
					menu = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWcParticle_Class_Disable);
					if (menu == NULL) {
						break;
					}

					WMrPopupMenu_GetItemID(menu, -1, &selection);

					new_flags2 = user_data->classptr->definition->flags2 &
						~(P3cParticleClassFlag2_MediumDetailDisable | P3cParticleClassFlag2_LowDetailDisable);

					if (selection == OWcParticle_Disable_MediumDetail) 
					{
						new_flags2 |= (P3cParticleClassFlag2_MediumDetailDisable | P3cParticleClassFlag2_LowDetailDisable);
					} 
					else if (selection == OWcParticle_Disable_LowDetail) 
					{
						new_flags2 |= P3cParticleClassFlag2_LowDetailDisable;
					} 

					if (new_flags2 != user_data->classptr->definition->flags2)
					{
						// this particle class is now dirty. NB: must be called BEFORE we make any changes.
						P3rMakeDirty(user_data->classptr, UUcTrue);

						user_data->classptr->definition->flags2 = new_flags2;
							
						// we may now need to autosave
						OBDrMakeDirty();
					}

					ONrParticle3_UpdateGraphicsQuality();
					break;
				}
			}
		break;

		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

static void OWiParticle_Value_Int_InitDialog(WMtDialog *inDialog)
{
	WMtPopupMenu *					menu;
	UUtUns16						active_radio;
	char							editstring[64];
	char *							selected_var;
	WMtWindow *						edit_field;
	OWtParticle_Value_Data *		user_data;

	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	WMrWindow_SetTitle(inDialog, user_data->title, WMcMaxTitleLength);

	if (user_data->valptr->type == P3cValueType_Variable) {
		selected_var = user_data->valptr->u.variable.name;
	} else {
		selected_var = NULL;
	}

	// initialise the popup menu
	menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Int_VarPopup);
	if (menu != NULL) {
		OWiFillVariableMenu(menu, P3cDataType_Integer, user_data->classptr->definition,
							selected_var);
	}

	switch(user_data->valptr->type) {
	case P3cValueType_Integer:
		active_radio = OWcParticle_Value_Int_Const;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Int_ConstVal);
		sprintf(editstring, "%d", user_data->valptr->u.integer_const.val);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);
		break;

	case P3cValueType_IntegerRange:
		active_radio = OWcParticle_Value_Int_Range;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Int_RangeLow);
		sprintf(editstring, "%d", user_data->valptr->u.integer_range.val_low);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Int_RangeHi);
		sprintf(editstring, "%d", user_data->valptr->u.integer_range.val_hi);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);
		break;

	case P3cValueType_Variable:
		UUmAssert(user_data->variable_allowed);
		active_radio = OWcParticle_Value_Int_Variable;
		break;

	default:
		UUmAssert(!"editing non-integer value using integer edit dialog");
	}

	WMrDialog_RadioButtonCheck(inDialog, OWcParticle_Value_Int_Const,
								OWcParticle_Value_Int_Variable, active_radio);
}

static void OWiParticle_Value_Int_UpdateParameters(WMtDialog *inDialog)
{
	WMtPopupMenu *					menu;
	UUtUns16						active_radio;
	char							editstring[64];
	WMtWindow *						edit_field;
	WMtWindow *						radio_button;
	P3tValue						new_val;
	UUtBool							val_changed;
	OWtParticle_Value_Data *		user_data;

	// read which radio button is selected
	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	active_radio = 0;
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Int_Const);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			active_radio = OWcParticle_Value_Int_Const;
			new_val.type = P3cValueType_Integer;
		}
	}
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Int_Range);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			active_radio = OWcParticle_Value_Int_Range;
			new_val.type = P3cValueType_IntegerRange;
		}
	}
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Int_Variable);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			// if variables are not allowed, this can never be selected
			UUmAssert(user_data->variable_allowed);
			active_radio = OWcParticle_Value_Int_Variable;
			new_val.type = P3cValueType_Variable;
		}
	}
	
	// is this a different type to the initial one?
	val_changed = (new_val.type != user_data->valptr->type);
	
	// read the modified value from whichever field is selected
	switch(active_radio) {
	case OWcParticle_Value_Int_Const:
		new_val.u.integer_const.val = 0;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Int_ConstVal);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
		if ((sscanf(editstring, "%hd", &new_val.u.integer_const.val) == 1) && (!val_changed)) {
			val_changed = (new_val.u.integer_const.val != user_data->valptr->u.integer_const.val);
		}
		break;
		
	case OWcParticle_Value_Int_Range:
		new_val.u.integer_range.val_low = 0;
		new_val.u.integer_range.val_hi	= 0;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Int_RangeLow);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
		if ((sscanf(editstring, "%hd", &new_val.u.integer_range.val_low) == 1) && (!val_changed)) {
			val_changed = (new_val.u.integer_range.val_low !=
				user_data->valptr->u.integer_range.val_low);
		}

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Int_RangeHi);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
		if ((sscanf(editstring, "%hd", &new_val.u.integer_range.val_hi) == 1) && (!val_changed)) {
			val_changed = (new_val.u.integer_range.val_hi !=
					user_data->valptr->u.integer_range.val_hi);
		}
		break;

	case OWcParticle_Value_Int_Variable:
		menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Int_VarPopup);
		UUmAssert(menu != NULL);

		OWiGetVariableFromMenu(menu, user_data->classptr->definition, &new_val.u.variable);
		if (!val_changed) {
			val_changed = (new_val.u.variable.offset != user_data->valptr->u.variable.offset);
		}
		break;
	}

	if (val_changed) {
		// this particle class is now dirty. NB: must be called BEFORE we make any changes.
		P3rMakeDirty(user_data->classptr, UUcTrue);

		// set the new value
		*(user_data->valptr) = new_val;

		// notify the parent window to update itself
		OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
						(UUtUns32) user_data->notify_struct.line_to_notify);

		// we may now need to autosave
		OBDrMakeDirty();
	}
}

static void OWiParticle_Value_Float_InitDialog(WMtDialog *inDialog)
{
	WMtPopupMenu *					menu;
	UUtUns16						active_radio;
	char							editstring[64];
	char *							selected_var;
	WMtWindow *						edit_field;
	OWtParticle_Value_Data *		user_data;

	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	WMrWindow_SetTitle(inDialog, user_data->title, WMcMaxTitleLength);

	if (user_data->valptr->type == P3cValueType_Variable) {
		selected_var = user_data->valptr->u.variable.name;
	} else {
		selected_var = NULL;
	}

	// initialise the popup menu
	menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_VarPopup);
	if (menu != NULL) {
		OWiFillVariableMenu(menu, P3cDataType_Float, user_data->classptr->definition,
							selected_var);
	}

	switch(user_data->valptr->type) {
	case P3cValueType_Float:
		active_radio = OWcParticle_Value_Float_Const;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_ConstVal);
		sprintf(editstring, "%.3f", user_data->valptr->u.float_const.val);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);
		break;

	case P3cValueType_FloatRange:
		active_radio = OWcParticle_Value_Float_Range;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_RangeLow);
		sprintf(editstring, "%.3f", user_data->valptr->u.float_range.val_low);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_RangeHi);
		sprintf(editstring, "%.3f", user_data->valptr->u.float_range.val_hi);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);
		break;

	case P3cValueType_FloatBellcurve:
		active_radio = OWcParticle_Value_Float_Bellcurve;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_BellMean);
		sprintf(editstring, "%.3f", user_data->valptr->u.float_bellcurve.val_mean);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_BellStddev);
		sprintf(editstring, "%.3f", user_data->valptr->u.float_bellcurve.val_stddev);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);
		break;

	case P3cValueType_FloatTimeBased:
		active_radio = OWcParticle_Value_Float_Cycle;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_CycleLength);
		sprintf(editstring, "%.3f", user_data->valptr->u.float_timebased.cycle_length);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_CycleMax);
		sprintf(editstring, "%.3f", user_data->valptr->u.float_timebased.cycle_max);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);
		break;

	case P3cValueType_Variable:
		UUmAssert(user_data->variable_allowed);
		active_radio = OWcParticle_Value_Float_Variable;
		break;

	default:
		UUmAssert(!"editing non-float value using float edit dialog");
	}

	WMrDialog_RadioButtonCheck(inDialog, OWcParticle_Value_Float_Const,
								OWcParticle_Value_Float_Cycle, active_radio);
}

static void OWiParticle_Value_Float_UpdateParameters(WMtDialog *inDialog)
{
	WMtPopupMenu *					menu;
	UUtUns16						active_radio;
	char							editstring[64];
	WMtWindow *						edit_field;
	WMtWindow *						radio_button;
	P3tValue						new_val;
	UUtBool							val_changed;
	OWtParticle_Value_Data *		user_data;
	float							delta_val;

	// read which radio button is selected
	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	active_radio = 0;
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_Const);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			active_radio = OWcParticle_Value_Float_Const;
			new_val.type = P3cValueType_Float;
		}
	}
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_Range);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			active_radio = OWcParticle_Value_Float_Range;
			new_val.type = P3cValueType_FloatRange;
		}
	}
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_Bellcurve);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			active_radio = OWcParticle_Value_Float_Bellcurve;
			new_val.type = P3cValueType_FloatBellcurve;
		}
	}
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_Cycle);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			active_radio = OWcParticle_Value_Float_Cycle;
			new_val.type = P3cValueType_FloatTimeBased;
		}
	}
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_Variable);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			// if variables are not allowed, this can never be selected
			UUmAssert(user_data->variable_allowed);
			active_radio = OWcParticle_Value_Float_Variable;
			new_val.type = P3cValueType_Variable;
		}
	}
	
	// is this a different type to the initial one?
	val_changed = (new_val.type != user_data->valptr->type);
	
	// read the modified value from whichever field is selected
	switch(active_radio) {
	case OWcParticle_Value_Float_Const:
		new_val.u.float_const.val		= 0;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_ConstVal);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
		if ((sscanf(editstring, "%f", &new_val.u.float_const.val) == 1) && (!val_changed)) {
			delta_val = new_val.u.float_const.val - user_data->valptr->u.float_const.val;
			val_changed = (fabs(delta_val) > 1e-06f);
		}
		break;
		
	case OWcParticle_Value_Float_Range:
		new_val.u.float_range.val_low	= 0;
		new_val.u.float_range.val_hi	= 0;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_RangeLow);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
		if ((sscanf(editstring, "%f", &new_val.u.float_range.val_low) == 1) && (!val_changed)) {
			delta_val = new_val.u.float_range.val_low - user_data->valptr->u.float_range.val_low;
			val_changed = (fabs(delta_val) > 1e-06f);
		}

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_RangeHi);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
		if ((sscanf(editstring, "%f", &new_val.u.float_range.val_hi) == 1) && (!val_changed)) {
			delta_val = new_val.u.float_range.val_hi - user_data->valptr->u.float_range.val_hi;
			val_changed = (fabs(delta_val) > 1e-06f);
		}
		break;

	case OWcParticle_Value_Float_Bellcurve:
		new_val.u.float_bellcurve.val_mean	= 0;
		new_val.u.float_bellcurve.val_stddev= 0;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_BellMean);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
		if ((sscanf(editstring, "%f", &new_val.u.float_bellcurve.val_mean) == 1) && (!val_changed)) {
			delta_val = new_val.u.float_bellcurve.val_mean - user_data->valptr->u.float_bellcurve.val_mean;
			val_changed = (fabs(delta_val) > 1e-06f);
		}

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_BellStddev);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
		if ((sscanf(editstring, "%f", &new_val.u.float_bellcurve.val_stddev) == 1) && (!val_changed)) {
			delta_val = new_val.u.float_bellcurve.val_stddev - user_data->valptr->u.float_bellcurve.val_stddev;
			val_changed = (fabs(delta_val) > 1e-06f);
		}
		break;

	case OWcParticle_Value_Float_Cycle:
		new_val.u.float_timebased.cycle_length = 1.0f;
		new_val.u.float_timebased.cycle_max    = 0;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_CycleLength);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
		if ((sscanf(editstring, "%f", &new_val.u.float_timebased.cycle_length) == 1) && (!val_changed)) {
			delta_val = new_val.u.float_timebased.cycle_length - user_data->valptr->u.float_timebased.cycle_length;
			val_changed = (fabs(delta_val) > 1e-06f);
		}

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_CycleMax);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
		if ((sscanf(editstring, "%f", &new_val.u.float_timebased.cycle_max) == 1) && (!val_changed)) {
			delta_val = new_val.u.float_timebased.cycle_max - user_data->valptr->u.float_timebased.cycle_max;
			val_changed = (fabs(delta_val) > 1e-06f);
		}
		break;

	case OWcParticle_Value_Float_Variable:
		menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_VarPopup);
		UUmAssert(menu != NULL);

		OWiGetVariableFromMenu(menu, user_data->classptr->definition, &new_val.u.variable);
		if (!val_changed) {
			val_changed = (new_val.u.variable.offset != user_data->valptr->u.variable.offset);
		}
		break;
	}

	if (val_changed) {
		// this particle class is now dirty. NB: must be called BEFORE we make any changes.
		P3rMakeDirty(user_data->classptr, UUcTrue);

		// set the new value
		*(user_data->valptr) = new_val;

		// notify the parent window to update itself
		OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
						(UUtUns32) user_data->notify_struct.line_to_notify);

		// we may now need to autosave
		OBDrMakeDirty();
	}
}

static void OWiParticle_Value_String_InitDialog(WMtDialog *inDialog)
{
	WMtPopupMenu *					menu;
	UUtUns16						active_radio;
	char *							selected_var;
	WMtWindow *						edit_field;
	OWtParticle_Value_Data *		user_data;

	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	WMrWindow_SetTitle(inDialog, user_data->title, WMcMaxTitleLength);

	if (user_data->valptr->type == P3cValueType_Variable) {
		selected_var = user_data->valptr->u.variable.name;
	} else {
		selected_var = NULL;
	}

	// initialise the popup menu
	menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_String_VarPopup);
	if (menu != NULL) {
		OWiFillVariableMenu(menu, P3cDataType_String, user_data->classptr->definition,
							selected_var);
	}

	edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_String_ConstVal);
	WMrMessage_Send(edit_field, EFcMessage_SetMaxChars, P3cStringVarSize, 0);

	switch(user_data->valptr->type) {
	case P3cValueType_String:
		active_radio = OWcParticle_Value_String_Const;

		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) user_data->valptr->u.string_const.val, 0);
		break;

	case P3cValueType_Variable:
		UUmAssert(user_data->variable_allowed);
		active_radio = OWcParticle_Value_String_Variable;
		break;

	default:
		UUmAssert(!"editing non-string value using string edit dialog");
	}

	WMrDialog_RadioButtonCheck(inDialog, OWcParticle_Value_String_Const,
								OWcParticle_Value_String_Variable, active_radio);
}

static void OWiParticle_Value_String_UpdateParameters(WMtDialog *inDialog)
{
	WMtPopupMenu *					menu;
	UUtUns16						active_radio;
	WMtWindow *						edit_field;
	WMtWindow *						radio_button;
	P3tValue						new_val;
	UUtBool							val_changed;
	OWtParticle_Value_Data *		user_data;

	// read which radio button is selected
	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	active_radio = 0;
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_String_Const);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			active_radio = OWcParticle_Value_String_Const;
			new_val.type = P3cValueType_String;
		}
	}
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_String_Variable);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			// if variables are not allowed, this can never be selected
			UUmAssert(user_data->variable_allowed);
			active_radio = OWcParticle_Value_String_Variable;
			new_val.type = P3cValueType_Variable;
		}
	}
	
	// is this a different type to the initial one?
	val_changed = (new_val.type != user_data->valptr->type);
	
	// read the modified value from whichever field is selected
	switch(active_radio) {
	case OWcParticle_Value_String_Const:
		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_String_ConstVal);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) new_val.u.string_const.val, P3cStringVarSize + 1);
		if (!val_changed) {
			val_changed = strcmp(new_val.u.string_const.val,
						user_data->valptr->u.string_const.val);
		}
		break;
		
	case OWcParticle_Value_String_Variable:
		menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_String_VarPopup);
		UUmAssert(menu != NULL);

		OWiGetVariableFromMenu(menu, user_data->classptr->definition, &new_val.u.variable);
		if (!val_changed) {
			val_changed = (new_val.u.variable.offset != user_data->valptr->u.variable.offset);
		}
		break;
	}

	if (val_changed) {
		// this particle class is now dirty. NB: must be called BEFORE we make any changes.
		P3rMakeDirty(user_data->classptr, UUcTrue);

		// set the new value
		*(user_data->valptr) = new_val;

		// notify the parent window to update itself
		OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
						(UUtUns32) user_data->notify_struct.line_to_notify);

		// we may now need to autosave
		OBDrMakeDirty();
	}
}

static void OWiParticle_Value_Shade_InitDialog(WMtDialog *inDialog)
{
	WMtPopupMenu *					menu;
	UUtUns16						active_radio;
	char							editstring[64];
	char *							selected_var;
	WMtWindow *						edit_field;
	OWtParticle_Value_Data *		user_data;

	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	WMrWindow_SetTitle(inDialog, user_data->title, WMcMaxTitleLength);

	if (user_data->valptr->type == P3cValueType_Variable) {
		selected_var = user_data->valptr->u.variable.name;
	} else {
		selected_var = NULL;
	}

	// initialise the popup menu
	menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_VarPopup);
	if (menu != NULL) {
		OWiFillVariableMenu(menu, P3cDataType_Shade, user_data->classptr->definition,
							selected_var);
	}

	switch(user_data->valptr->type) {
	case P3cValueType_Shade:
		active_radio = OWcParticle_Value_Shade_Const;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_ConstVal);
		OWiPrintShade(user_data->valptr->u.shade_const.val, editstring);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);
		break;

	case P3cValueType_ShadeRange:
		active_radio = OWcParticle_Value_Shade_Range;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_RangeLow);
		OWiPrintShade(user_data->valptr->u.shade_range.val_low, editstring);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_RangeHi);
		OWiPrintShade(user_data->valptr->u.shade_range.val_hi, editstring);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);
		break;

	case P3cValueType_ShadeBellcurve:
		active_radio = OWcParticle_Value_Shade_Bellcurve;

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_BellMean);
		OWiPrintShade(user_data->valptr->u.shade_bellcurve.val_mean, editstring);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_BellStddev);
		OWiPrintShade(user_data->valptr->u.shade_bellcurve.val_stddev, editstring);
		WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);
		break;

	case P3cValueType_Variable:
		UUmAssert(user_data->variable_allowed);
		active_radio = OWcParticle_Value_Float_Variable;
		break;

	default:
		UUmAssert(!"editing non-shade value using shade edit dialog");
	}

	WMrDialog_RadioButtonCheck(inDialog, OWcParticle_Value_Shade_Const,
								OWcParticle_Value_Shade_Variable, active_radio);
}

static UUtBool OWiParticle_Value_Shade_UpdateParameters(WMtDialog *inDialog, char *outString)
{
	WMtPopupMenu *					menu;
	UUtUns16						active_radio;
	char							editstring[64];
	WMtWindow *						edit_field;
	WMtWindow *						radio_button;
	P3tValue						new_val;
	UUtBool							val_changed;
	OWtParticle_Value_Data *		user_data;

	// read which radio button is selected
	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	active_radio = 0;
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_Const);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			active_radio = OWcParticle_Value_Shade_Const;
			new_val.type = P3cValueType_Shade;
		}
	}
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_Range);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			active_radio = OWcParticle_Value_Shade_Range;
			new_val.type = P3cValueType_ShadeRange;
		}
	}
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_Bellcurve);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			active_radio = OWcParticle_Value_Shade_Bellcurve;
			new_val.type = P3cValueType_ShadeBellcurve;
		}
	}
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_Variable);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			// if variables are not allowed, this can never be selected
			UUmAssert(user_data->variable_allowed);
			active_radio = OWcParticle_Value_Shade_Variable;
			new_val.type = P3cValueType_Variable;
		}
	}
	
	// is this a different type to the initial one?
	val_changed = (new_val.type != user_data->valptr->type);
	
	// read the modified value from whichever field is selected
	switch(active_radio) {
	case OWcParticle_Value_Shade_Const:
		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Float_ConstVal);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);

		if (OWiParseShade(editstring, &new_val.u.shade_const.val) == UUcFalse) {
			strcpy(outString, editstring);
			return UUcFalse;
		}
		if (!val_changed) {
			val_changed = (new_val.u.shade_const.val != user_data->valptr->u.shade_const.val);
		}
		break;
		
	case OWcParticle_Value_Shade_Range:
		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_RangeLow);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);

		if (OWiParseShade(editstring, &new_val.u.shade_range.val_low) == UUcFalse) {
			strcpy(outString, editstring);
			return UUcFalse;
		}
		if (!val_changed) {
			val_changed = (new_val.u.shade_range.val_low != user_data->valptr->u.shade_range.val_low);
		}

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_RangeHi);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);

		if (OWiParseShade(editstring, &new_val.u.shade_range.val_hi) == UUcFalse) {
			strcpy(outString, editstring);
			return UUcFalse;
		}
		if (!val_changed) {
			val_changed = (new_val.u.shade_range.val_hi != user_data->valptr->u.shade_range.val_hi);
		}
		break;

	case OWcParticle_Value_Shade_Bellcurve:
		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_BellMean);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);

		if (OWiParseShade(editstring, &new_val.u.shade_bellcurve.val_mean) == UUcFalse) {
			strcpy(outString, editstring);
			return UUcFalse;
		}
		if (!val_changed) {
			val_changed = (new_val.u.shade_bellcurve.val_mean != user_data->valptr->u.shade_bellcurve.val_mean);
		}

		edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_BellStddev);
		UUmAssert(edit_field != NULL);
		WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);

		if (OWiParseShade(editstring, &new_val.u.shade_bellcurve.val_stddev) == UUcFalse) {
			strcpy(outString, editstring);
			return UUcFalse;
		}
		if (!val_changed) {
			val_changed = (new_val.u.shade_bellcurve.val_stddev != user_data->valptr->u.shade_bellcurve.val_stddev);
		}
		break;

	case OWcParticle_Value_Shade_Variable:
		menu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Shade_VarPopup);
		UUmAssert(menu != NULL);

		OWiGetVariableFromMenu(menu, user_data->classptr->definition, &new_val.u.variable);
		if (!val_changed) {
			val_changed = (new_val.u.variable.offset != user_data->valptr->u.variable.offset);
		}
		break;
	}

	if (val_changed) {
		// this particle class is now dirty. NB: must be called BEFORE we make any changes.
		P3rMakeDirty(user_data->classptr, UUcTrue);

		// set the new value
		*(user_data->valptr) = new_val;

		// notify the parent window to update itself
		OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
						(UUtUns32) user_data->notify_struct.line_to_notify);

		// we may now need to autosave
		OBDrMakeDirty();
	}

	return UUcTrue;
}

static void OWiParticle_Value_Enum_InitDialog(WMtDialog *inDialog)
{
	WMtPopupMenu					*varmenu, *constmenu;
	char *							selected_var;
	UUtInt32						current_enum;
	UUtUns16						active_radio;
	OWtParticle_Value_Data *		user_data;

	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	WMrWindow_SetTitle(inDialog, user_data->title, WMcMaxTitleLength);

	if (user_data->valptr->type == P3cValueType_Variable) {
		selected_var = user_data->valptr->u.variable.name;
		current_enum = -1;
	} else if (user_data->valptr->type == P3cValueType_Enum) {
		selected_var = NULL;
		current_enum = user_data->valptr->u.enum_const.val;
	} else {
		selected_var = NULL;
		current_enum = -1;
	}

	// initialise the variable popup menu
	varmenu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Enum_VarPopup);
	if (varmenu != NULL) {
		OWiFillVariableMenu(varmenu, P3cDataType_Enum | user_data->enum_info.type, user_data->classptr->definition,
							selected_var);
	}

	// initialise the enumerated-type popup menu
	constmenu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Enum_ConstPopup);
	if (constmenu != NULL) {
		OWiFillEnumMenu(user_data->classptr, constmenu, &user_data->enum_info, current_enum);
	}

	switch(user_data->valptr->type) {
	case P3cValueType_Enum:
		active_radio = OWcParticle_Value_Enum_Const;
		break;

	case P3cValueType_Variable:
		UUmAssert(user_data->variable_allowed);
		active_radio = OWcParticle_Value_Enum_Variable;
		break;

	default:
		UUmAssert(!"editing non-enum value using enum edit dialog");
	}

	WMrDialog_RadioButtonCheck(inDialog, OWcParticle_Value_Enum_Const,
								OWcParticle_Value_Enum_Variable, active_radio);
}

static void OWiParticle_Value_Enum_UpdateParameters(WMtDialog *inDialog)
{
	WMtPopupMenu					*varmenu, *constmenu;
	UUtUns16						active_radio, enumtype_index;
	WMtWindow *						radio_button;
	P3tValue						new_val;
	UUtBool							val_changed;
	P3tEnumTypeInfo *				enuminfo;
	OWtParticle_Value_Data *		user_data;

	// read which radio button is selected
	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	active_radio = 0;
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Enum_Const);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			active_radio = OWcParticle_Value_Enum_Const;
			new_val.type = P3cValueType_Enum;
		}
	}
	
	radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Enum_Variable);
	if (radio_button != NULL) {
		if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
			(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
			// if variables are not allowed, this can never be selected
			UUmAssert(user_data->variable_allowed);
			active_radio = OWcParticle_Value_Enum_Variable;
			new_val.type = P3cValueType_Variable;
		}
	}
	
	// is this a different type to the initial one?
	val_changed = (new_val.type != user_data->valptr->type);
	
	enumtype_index = (UUtUns16) (user_data->enum_info.type >> P3cEnumShift);

	UUmAssert((enumtype_index >= 0) && (enumtype_index < (P3cEnumType_Max >> P3cEnumShift)));
	enuminfo = &P3gEnumTypeInfo[enumtype_index];

	// read the modified value from whichever field is selected
	switch(active_radio) {
	case OWcParticle_Value_Enum_Const:
		constmenu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_Enum_ConstPopup);
		UUmAssert(constmenu != NULL);

		// get the currently selected enumeration
		WMrPopupMenu_GetItemID(constmenu, -1, &enumtype_index);
		UUmAssert((enumtype_index >= enuminfo->min) &&
				  (enumtype_index <= enuminfo->max));

		new_val.u.enum_const.val = enumtype_index;

		if (!val_changed) {
			val_changed = (new_val.u.enum_const.val != user_data->valptr->u.enum_const.val);
		}
		break;
		
	case OWcParticle_Value_Enum_Variable:
		varmenu = WMrDialog_GetItemByID(inDialog, OWcParticle_Value_String_VarPopup);
		UUmAssert(varmenu != NULL);

		OWiGetVariableFromMenu(varmenu, user_data->classptr->definition, &new_val.u.variable);
		if (!val_changed) {
			val_changed = (new_val.u.variable.offset != user_data->valptr->u.variable.offset);
		}
		break;
	}

	if (val_changed) {
		// this particle class is now dirty. NB: must be called BEFORE we make any changes.
		P3rMakeDirty(user_data->classptr, UUcTrue);

		// set the new value
		*(user_data->valptr) = new_val;

		// notify the parent window to update itself
		OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
						(UUtUns32) user_data->notify_struct.line_to_notify);

		// we may now need to autosave
		OBDrMakeDirty();
	}
}

UUtBool
OWrParticle_Value_Int_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	OWtParticle_Value_Data *		user_data;
	UUtUns16						active_radio;
	
	handled = UUcTrue;
	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiParticle_Value_Int_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case WMcDialogItem_OK:
					OWiParticle_Value_Int_UpdateParameters(inDialog);
					WMrWindow_Delete(inDialog);
					break;

				case OWcParticle_Value_Int_Apply:
					OWiParticle_Value_Int_UpdateParameters(inDialog);
					break;

				case OWcParticle_Value_Int_Variable:
					if (!user_data->variable_allowed)
						break;
				case OWcParticle_Value_Int_Const:
				case OWcParticle_Value_Int_Range:
					if (UUmHighWord(inParam1) == WMcNotify_Click)
					{
						active_radio = (UUtUns16) UUmLowWord(inParam1);

						WMrDialog_RadioButtonCheck(inDialog, OWcParticle_Value_Int_Const,
										OWcParticle_Value_Int_Variable, active_radio);
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_Value_Float_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	OWtParticle_Value_Data *		user_data;
	UUtUns16						active_radio;
	
	handled = UUcTrue;
	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiParticle_Value_Float_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case WMcDialogItem_OK:
					OWiParticle_Value_Float_UpdateParameters(inDialog);
					WMrWindow_Delete(inDialog);
					break;

				case OWcParticle_Value_Float_Apply:
					OWiParticle_Value_Float_UpdateParameters(inDialog);
					break;

				case OWcParticle_Value_Float_Variable:
					if (!user_data->variable_allowed)
						break;
				case OWcParticle_Value_Float_Const:
				case OWcParticle_Value_Float_Range:
				case OWcParticle_Value_Float_Bellcurve:
				case OWcParticle_Value_Float_Cycle:
					if (UUmHighWord(inParam1) == WMcNotify_Click)
					{
						active_radio = (UUtUns16) UUmLowWord(inParam1);

						WMrDialog_RadioButtonCheck(inDialog, OWcParticle_Value_Float_Const,
										OWcParticle_Value_Float_Cycle, active_radio);
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_Value_String_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	OWtParticle_Value_Data *		user_data;
	UUtUns16						active_radio;
	
	handled = UUcTrue;
	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiParticle_Value_String_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case WMcDialogItem_OK:
					OWiParticle_Value_String_UpdateParameters(inDialog);
					WMrWindow_Delete(inDialog);
					break;

				case OWcParticle_Value_String_Apply:
					OWiParticle_Value_String_UpdateParameters(inDialog);
					break;

				case OWcParticle_Value_String_Variable:
					if (!user_data->variable_allowed)
						break;
				case OWcParticle_Value_String_Const:
					if (UUmHighWord(inParam1) == WMcNotify_Click)
					{
						active_radio = (UUtUns16) UUmLowWord(inParam1);

						WMrDialog_RadioButtonCheck(inDialog, OWcParticle_Value_String_Const,
										OWcParticle_Value_String_Variable, active_radio);
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_Value_Enum_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	OWtParticle_Value_Data *		user_data;
	UUtUns16						active_radio;
	
	handled = UUcTrue;
	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiParticle_Value_Enum_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case WMcDialogItem_OK:
					OWiParticle_Value_Enum_UpdateParameters(inDialog);
					WMrWindow_Delete(inDialog);
					break;

				case OWcParticle_Value_Enum_Apply:
					OWiParticle_Value_Enum_UpdateParameters(inDialog);
					break;

				case OWcParticle_Value_Enum_Variable:
					if (!user_data->variable_allowed)
						break;
				case OWcParticle_Value_Enum_Const:
					if (UUmHighWord(inParam1) == WMcNotify_Click)
					{
						active_radio = (UUtUns16) UUmLowWord(inParam1);

						WMrDialog_RadioButtonCheck(inDialog, OWcParticle_Value_Enum_Const,
										OWcParticle_Value_Enum_Variable, active_radio);
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_Value_Shade_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	OWtParticle_Value_Data *		user_data;
	UUtUns16						active_radio;
	char							value_error[64];
	char							errormsg[256];
	
	handled = UUcTrue;
	user_data = (OWtParticle_Value_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiParticle_Value_Shade_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case WMcDialogItem_OK:
					if (OWiParticle_Value_Shade_UpdateParameters(inDialog, value_error) == UUcFalse) {
						// pop up error dialog, don't close window
						sprintf(errormsg, "Can't understand shade specification '%s'. Format should be "
											"like this: 1.0, 0.3, 0.9, 0.4 (in the order alpha, red, green, blue).",
								value_error);
						WMrDialog_MessageBox(NULL, "Error reading shade", errormsg, WMcMessageBoxStyle_OK);
						break;
					} else {
						// successful, close window
						WMrWindow_Delete(inDialog);
					}
					break;

				case OWcParticle_Value_Shade_Apply:
					if (OWiParticle_Value_Shade_UpdateParameters(inDialog, value_error) == UUcFalse) {
						// pop up error dialog
						sprintf(errormsg, "Can't understand shade specification '%s'. Format should be "
											"like this: 1.0, 0.3, 0.9, 0.4 (in the order alpha, red, green, blue).",
								value_error);
						WMrDialog_MessageBox(NULL, "Error reading shade", errormsg, WMcMessageBoxStyle_OK);
						break;
					}
					break;

				case OWcParticle_Value_Shade_Variable:
					if (!user_data->variable_allowed)
						break;
				case OWcParticle_Value_Shade_Const:
				case OWcParticle_Value_Shade_Range:
				case OWcParticle_Value_Shade_Bellcurve:
					if (UUmHighWord(inParam1) == WMcNotify_Click)
					{
						active_radio = (UUtUns16) UUmLowWord(inParam1);

						WMrDialog_RadioButtonCheck(inDialog, OWcParticle_Value_Shade_Const,
										OWcParticle_Value_Shade_Variable, active_radio);
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_NewVar_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	OWtParticle_NewVar_Data *		user_data;
	WMtWindow *						edit_field;
	WMtWindow *						radio_button;
	WMtPopupMenu *					enum_popup;
	UUtUns16						active_radio, itr, selected_enumtype;
	char							typedname[64];
	char							messageboxstring[128];
	WMtMenuItemData					item_data;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			WMrDialog_RadioButtonCheck(inDialog, OWcParticle_NewVar_Int,
										OWcParticle_NewVar_Enum, OWcParticle_NewVar_Int);

			enum_popup = WMrDialog_GetItemByID(inDialog, OWcParticle_NewVar_EnumPopup);
			UUmAssert(enum_popup != NULL);
			for (itr = 0; itr < P3cEnumType_Max >> P3cEnumShift; itr++) {
				item_data.flags = WMcMenuItemFlag_Enabled;
				item_data.id = P3gEnumTypeInfo[itr].type;
				strcpy(item_data.title, P3gEnumTypeInfo[itr].name);
	
				WMrPopupMenu_AppendItem(enum_popup, &item_data);
			}
		break;
		
		case WMcMessage_Destroy:
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, (UUtUns32) UUcFalse);
					break;

				case WMcDialogItem_OK:
					user_data = (OWtParticle_NewVar_Data *) WMrDialog_GetUserData(inDialog);

					// get the variable name
					edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_NewVar_Name);
					UUmAssert(edit_field != NULL);
					WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) typedname, 64);

					// can't exceed the fixed size of variable names
					if ((strlen(typedname) < 1) || (strlen(typedname) > P3cStringVarSize)) {
						sprintf(messageboxstring, "Sorry, variable names must be 1-%d letters long.",
							    P3cStringVarSize);
						WMrDialog_MessageBox(inDialog, "Error",
											messageboxstring, WMcMessageBoxStyle_OK);
						break;
					}
					strcpy(user_data->name, typedname);
				
					// find the selected data type
					user_data->type = P3cDataType_Integer;

					radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_NewVar_Int);
					if (radio_button != NULL) {
						if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
											(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
							user_data->type = P3cDataType_Integer;
						}
					}
	
					radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_NewVar_Float);
					if (radio_button != NULL) {
						if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
											(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
							user_data->type = P3cDataType_Float;
						}
					}
	
					radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_NewVar_String);
					if (radio_button != NULL) {
						if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
											(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
							user_data->type = P3cDataType_String;
						}
					}
	
					radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_NewVar_Shade);
					if (radio_button != NULL) {
						if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
											(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
							user_data->type = P3cDataType_Shade;
						}
					}
	
					radio_button = WMrDialog_GetItemByID(inDialog, OWcParticle_NewVar_Enum);
					if (radio_button != NULL) {
						if (WMrMessage_Send(radio_button, WMcMessage_GetValue,
											(UUtUns32) -1, (UUtUns32) -1) == (UUtUns32) UUcTrue) {
							user_data->type = P3cDataType_Enum;

							// get the selected kind of enum
							enum_popup = WMrDialog_GetItemByID(inDialog, OWcParticle_NewVar_EnumPopup);
							UUmAssert(enum_popup != NULL);
							WMrPopupMenu_GetItemID(enum_popup, -1, &selected_enumtype);

							user_data->type |= selected_enumtype;
						}
					}
						
					WMrDialog_ModalEnd(inDialog, (UUtUns32) UUcTrue);
					break;

				case OWcParticle_NewVar_Int:
				case OWcParticle_NewVar_Float:
				case OWcParticle_NewVar_String:
				case OWcParticle_NewVar_Shade:
				case OWcParticle_NewVar_Enum:
					if (UUmHighWord(inParam1) == WMcNotify_Click)
					{
						active_radio = (UUtUns16) UUmLowWord(inParam1);

						WMrDialog_RadioButtonCheck(inDialog, OWcParticle_NewVar_Int,
													OWcParticle_NewVar_Enum, active_radio);
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}


UUtBool
OWrParticle_RenameVar_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool			handled;
	char *			newvar_name;
	char			messageboxstring[64];
	WMtWindow *		edit_field;
	
	handled = UUcTrue;
	newvar_name = (char *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// set up the initial variable name
			edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_RenameVar_Name);
			UUmAssert(edit_field != NULL);
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) newvar_name, 0);
		break;
		
		case WMcMessage_Destroy:
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					WMrDialog_ModalEnd(inDialog, (UUtUns32) UUcFalse);
					break;

				case WMcDialogItem_OK:
					edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_RenameVar_Name);
					UUmAssert(edit_field != NULL);
					WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) newvar_name, 64);

					// can't exceed the fixed size of variable names
					if ((strlen(newvar_name) < 1) || (strlen(newvar_name) > P3cStringVarSize)) {
						sprintf(messageboxstring, "Sorry, variable names must be 1-%d letters long.",
							    P3cStringVarSize);
						WMrDialog_MessageBox(inDialog, "Error",
											messageboxstring, WMcMessageBoxStyle_OK);
						break;
					}

					WMrDialog_ModalEnd(inDialog, (UUtUns32) UUcTrue);
					break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_Texture_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool						handled, close_window, invalid;
	char						texturename[32], errormsg[256];
	WMtWindow					*edit_field, *textfield;
	OWtParticle_Texture_Data *	user_data;
	M3tTextureMap *				textureptr;
	M3tGeometry *				geomptr;
	P3tParticleClass *			classptr;
	UUtError					error;
	
	handled = UUcTrue;
	user_data = (OWtParticle_Texture_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// set our dialog's title
			WMrWindow_SetTitle(inDialog, user_data->title, WMcMaxTitleLength);

			// set the textfield
			textfield = WMrDialog_GetItemByID(inDialog, OWcParticle_Texture_Prompt);
			if (textfield != NULL) {
				WMrWindow_SetTitle(textfield, user_data->editmsg, WMcMaxTitleLength);
			}

			// set up the initial texture name
			edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Texture_Name);
			UUmAssert(edit_field != NULL);
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) user_data->texture_name, 0);
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case WMcDialogItem_OK:
					close_window = UUcTrue;	// fall through to apply case
				case OWcParticle_Texture_Apply:
					edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Texture_Name);
					UUmAssert(edit_field != NULL);
					WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) texturename, 32);

					// make sure that this is a valid instance name
					invalid = UUcFalse;
					switch(user_data->name_type) {
						case OWcParticle_NameType_Geometry:
							// make sure that this is a valid geometry instance
							error = TMrInstance_GetDataPtr(M3cTemplate_Geometry, texturename,
															&geomptr);
							if (error != UUcError_None) {
								sprintf(errormsg, "Could not find any geometry instances named '%s'. Are you sure this is correct?", texturename);
								if (WMrDialog_MessageBox(NULL, "Unknown Geometry", errormsg,
														 WMcMessageBoxStyle_OK_Cancel) != WMcDialogItem_OK) {
									invalid = UUcTrue;
								}
							}
						break;

						case OWcParticle_NameType_Texture:
							// make sure that this is a valid texture
							error = TMrInstance_GetDataPtr(M3cTemplate_TextureMap, texturename,
														&textureptr);
							if (error != UUcError_None) {
								sprintf(errormsg, "Could not find any textures named '%s'. Are you sure this is correct?", texturename);
								if (WMrDialog_MessageBox(NULL, "Unknown Texture", errormsg,
														WMcMessageBoxStyle_OK_Cancel) != WMcDialogItem_OK) {
									invalid = UUcTrue;
								}
							}
						break;

						case OWcParticle_NameType_ParticleClass:
							// make sure that this is a valid texture
							classptr = P3rGetParticleClass(texturename);
							if (classptr == NULL) {
								sprintf(errormsg, "Could not find any particle classes named '%s'. Are you sure this is correct?", texturename);
								if (WMrDialog_MessageBox(NULL, "Unknown Particle Class", errormsg,
														WMcMessageBoxStyle_OK_Cancel) != WMcDialogItem_OK) {
									invalid = UUcTrue;
								}
							}
						break;

						case OWcParticle_NameType_String:
						default:
						break;
					}

					if (invalid) {
						// keep editing
						break;
					}

					if (strcmp(user_data->texture_name, texturename) != 0) {
						// this particle class is now dirty. NB: must be called BEFORE we make any changes.
						P3rMakeDirty(user_data->classptr, UUcTrue);

						// set the new value
						strcpy(user_data->texture_name, texturename);

						// notify the parent window to update itself
						OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
										(UUtUns32) user_data->notify_struct.line_to_notify);

						// we may now need to autosave
						OBDrMakeDirty();
					}

					if (close_window)
						WMrWindow_Delete(inDialog);
					break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_Number_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool						handled, close_window;
	char						input_text[64];
	WMtWindow					*edit_field, *textfield;
	OWtParticle_Number_Data *	user_data;
	UUtUns16					new_number;
	float						new_float;
	
	handled = UUcTrue;
	user_data = (OWtParticle_Number_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// set our dialog's title
			WMrWindow_SetTitle(inDialog, user_data->title, WMcMaxTitleLength);

			// set the textfield
			textfield = WMrDialog_GetItemByID(inDialog, OWcParticle_Number_Text);
			if (textfield != NULL) {
				WMrWindow_SetTitle(textfield, user_data->editmsg, WMcMaxTitleLength);
			}

			// set up the initial number
			edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Number_EditField);
			UUmAssert(edit_field != NULL);
			if (user_data->is_float) {
				sprintf(input_text, "%.3f", *(user_data->ptr.floatptr));
			} else {
				sprintf(input_text, "%d", *(user_data->ptr.numberptr));
			}
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) input_text, 0);
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case WMcDialogItem_OK:
					close_window = UUcTrue;	// fall through to apply case
				case OWcParticle_Number_Apply:
					edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Number_EditField);
					UUmAssert(edit_field != NULL);
					WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) input_text, 32);

					if (user_data->is_float) {
						// read the number
						if (sscanf(input_text, "%f", &new_float) != 1) {
							WMrDialog_MessageBox(NULL, "Invalid Entry", "Please enter a valid floating point number.",
								                 WMcMessageBoxStyle_OK);
							break;
						}

						if (!UUmFloat_CompareEqu(new_float, *(user_data->ptr.floatptr))) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							// set the new value
							*(user_data->ptr.floatptr) = new_float;

							// notify the parent window to update itself
							OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
											(UUtUns32) user_data->notify_struct.line_to_notify);

							// we may now need to autosave
							OBDrMakeDirty();
						}
					} else {
						// read the number
						if (sscanf(input_text, "%hd", &new_number) != 1) {
							WMrDialog_MessageBox(NULL, "Invalid Entry", "Please enter a valid number.",
								                 WMcMessageBoxStyle_OK);
							break;
						}

						if (new_number != *(user_data->ptr.numberptr)) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							// set the new value
							*(user_data->ptr.numberptr) = new_number;

							// notify the parent window to update itself
							OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
											(UUtUns32) user_data->notify_struct.line_to_notify);

							// we may now need to autosave
							OBDrMakeDirty();
						}
					}

					if (close_window)
						WMrWindow_Delete(inDialog);
					break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_NewAction_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	WMtWindow *						menu;
	WMtMenuItemData					item_data;
	P3tActionTemplate **			output_template;
	UUtUns16						itr, act_index;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			menu = WMrDialog_GetItemByID(inDialog, OWcParticle_NewAction_Type);
			UUmAssert(menu != NULL);

			// set up the action type popup menu
			for (itr = 0; itr < P3cNumActionTypes; itr++) {
				strcpy(item_data.title, P3gActionInfo[itr].name);
				if (strcmp(item_data.title, "-") == 0) {
					// this is a divider and not a real action type
					item_data.flags = WMcMenuItemFlag_Divider;
					item_data.id = -1;
				} else if (strcmp(item_data.title, "") == 0) {
					// this is a blank space in the action table, don't add to menu
					continue;
				} else {
					// this is an action type
					item_data.flags = WMcMenuItemFlag_Enabled;
					item_data.id = itr;
				}
	
				WMrPopupMenu_AppendItem(menu, &item_data);
			}
			WMrPopupMenu_SetSelection(menu, 0);
		break;
		
		case WMcMessage_Destroy:
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, (UUtUns32) UUcFalse);
					break;

				case WMcDialogItem_OK:
					// get the selected action type
					menu = WMrDialog_GetItemByID(inDialog, OWcParticle_NewAction_Type);
					UUmAssert(menu != NULL);

					WMrPopupMenu_GetItemID(menu, -1, &act_index);
					UUmAssert((act_index >= 0) && (act_index < P3cNumActionTypes));

					output_template = (P3tActionTemplate **) WMrDialog_GetUserData(inDialog);
					*output_template = &P3gActionInfo[act_index];

					WMrDialog_ModalEnd(inDialog, (UUtUns32) UUcTrue);
					break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_VarRef_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled, kill_window;
	OWtParticle_VarRef_Data *		user_data;
	char *							selected_var;
	UUtUns16						old_offset;
	WMtWindow *						menu;
	
	kill_window = UUcFalse;
	handled = UUcTrue;
	user_data = (OWtParticle_VarRef_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			selected_var = (user_data->varptr->offset == -1) ? NULL : user_data->varptr->name;

			// initialise the popup menu
			menu = WMrDialog_GetItemByID(inDialog, OWcParticle_VarRef_Popup);
			if (menu != NULL) {
				OWiFillVariableMenu(menu, user_data->allowed_types, user_data->classptr->definition,
									selected_var);
			}

			// set the title
			WMrWindow_SetTitle(inDialog, user_data->title, WMcMaxTitleLength);
		break;
		
		case WMcMessage_Destroy:

			UUrMemory_Block_Delete(user_data);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case WMcDialogItem_OK:
					// fall through to apply
					kill_window = UUcTrue;

				case OWcParticle_VarRef_Apply:
					menu = WMrDialog_GetItemByID(inDialog, OWcParticle_VarRef_Popup);
					UUmAssert(menu != NULL);

					old_offset = user_data->varptr->offset;
					OWiGetVariableFromMenu(menu, user_data->classptr->definition, user_data->varptr);

					if (user_data->varptr->offset != old_offset) {
						// this particle class is now dirty. NB: must be called BEFORE we make any changes.
						P3rMakeDirty(user_data->classptr, UUcTrue);

						// notify the parent window to update itself
						OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
										(UUtUns32) user_data->notify_struct.line_to_notify);

						// we may now need to autosave
						OBDrMakeDirty();
					}

					if (kill_window == UUcTrue) {
						// only if the OK button was hit
						WMrWindow_Delete(inDialog);
					}
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_ActionList_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled, changed;
	OWtParticle_ActionList_Data *	user_data;
	UUtUns16						orig_val;
	WMtWindow *						edit_field;
	char							editstring[64];
	
	handled = UUcTrue;
	user_data = (OWtParticle_ActionList_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_ActionList_Start);
			if (edit_field != NULL) {
				sprintf(editstring, "%d", user_data->actionlistptr->start_index);
				WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);
			}

			edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_ActionList_End);
			if (edit_field != NULL) {
				sprintf(editstring, "%d", user_data->actionlistptr->end_index);
				WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) editstring, 0);
			}

			// set the title
			WMrWindow_SetTitle(inDialog, user_data->title, WMcMaxTitleLength);
		break;
		
		case WMcMessage_Destroy:
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, (UUtUns32) UUcFalse);
				break;

				case WMcDialogItem_OK:
					changed = UUcFalse;

					edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_ActionList_Start);
					UUmAssert(edit_field != NULL);

					orig_val = user_data->actionlistptr->start_index;
					WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
					if ((sscanf(editstring, "%hd", &user_data->actionlistptr->start_index) != 1) ||
						(user_data->actionlistptr->start_index < 0) ||
						(user_data->actionlistptr->start_index >= user_data->max_actions)) {
						char errmsg[256];

						// print an error message
						sprintf(errmsg, "Please enter a start index that is either -1 (no actions) or an index "
										"between 0 and %d.", user_data->max_actions - 1);
						WMrDialog_MessageBox(NULL, "Invalid start index", errmsg, WMcMessageBoxStyle_OK);
						break;
					}

					if (user_data->actionlistptr->start_index != orig_val) {
						changed = UUcTrue;
					}

					edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_ActionList_End);
					UUmAssert(edit_field != NULL);

					orig_val = user_data->actionlistptr->end_index;
					WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) editstring, 64);
					if ((sscanf(editstring, "%hd", &user_data->actionlistptr->end_index) != 1) ||
						(user_data->actionlistptr->end_index < 0) ||
						(user_data->actionlistptr->end_index >= user_data->max_actions) ||
						(user_data->actionlistptr->end_index < user_data->actionlistptr->start_index)) {
						char errmsg[256];

						// print an error message
						sprintf(errmsg, "Please enter an end index that is either -1 (no actions) or an index "
										"between 0 and %d. The end index must not be smaller than the start index.",
								user_data->max_actions - 1);
						WMrDialog_MessageBox(NULL, "Invalid end index", errmsg, WMcMessageBoxStyle_OK);
						break;
					}

					if (user_data->actionlistptr->end_index != orig_val) {
						changed = UUcTrue;
					}

					WMrDialog_ModalEnd(inDialog, (UUtUns32) changed);
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

void OWrParticle_Value_Sound_Get(SStType inType, OWtParticle_Value_Data *inValUserData)
{
	P3tString						name;
	OWtSelectResult					result;

	UUrString_Copy(name, "", P3cStringVarSize + 1);
	
	switch(inType)
	{
		case SScType_Ambient:
		{
			SStAmbient *ambient;
			result = OWrSelect_AmbientSound(&ambient);
			if (result == OWcSelectResult_Cancel) {
				break;
			}

			P3rMakeDirty(inValUserData->classptr, UUcTrue);

			if (ambient == NULL) {
				inValUserData->valptr->u.string_const.val[0] = '\0';
			} else {
				UUrString_Copy_Length(inValUserData->valptr->u.string_const.val, ambient->ambient_name,
										P3cStringVarSize, P3cStringVarSize);
			}

			OBDrMakeDirty();
			break;
		}
		case SScType_Impulse:
		{
			SStImpulse *impulse;
			result = OWrSelect_ImpulseSound(&impulse);
			if (result == OWcSelectResult_Cancel) {
				break;
			}

			P3rMakeDirty(inValUserData->classptr, UUcTrue);

			if (impulse == NULL) {
				inValUserData->valptr->u.string_const.val[0] = '\0';
			} else {
				UUrString_Copy_Length(inValUserData->valptr->u.string_const.val, impulse->impulse_name,
										P3cStringVarSize, P3cStringVarSize);
			}

			OBDrMakeDirty();
			break;
		}
	}
	
	inValUserData->valptr->type = P3cValueType_String;
	
	OWiParticle_NotifyParent(inValUserData->notify_struct.notify_window,
				(UUtUns32) inValUserData->notify_struct.line_to_notify);
}

// --------------------------------------------------------------------

UUtError OWrSave_Particles(UUtBool inAutosave)
{
	P3tParticleClassIterateState	itr;
	P3tParticleClass *				classptr;
	UUtUns8 *						binaryClass;
	UUtUns16						binarySize;
	UUtBool							swapbytes, needs_saving;
	UUtError						error;

	if (P3rAnyDirty() == UUcFalse)
		return UUcError_None;

#if (UUmEndian == UUmEndian_Big)
	swapbytes = UUcTrue;
#else
	swapbytes = UUcFalse;
#endif

	itr = P3cParticleClass_None;
	while(P3rIterateClasses(&itr, &classptr) == UUcTrue) {
		if (classptr->non_persistent_flags & (P3cParticleClass_NonPersistentFlag_Locked | P3cParticleClass_NonPersistentFlag_DoNotSave))
			continue;

		needs_saving = ((classptr->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Dirty) > 0);
		if (!inAutosave) {
			needs_saving = needs_saving || (classptr->non_persistent_flags & P3cParticleClass_NonPersistentFlag_OldVersion);
		}

		if (!needs_saving)
			continue;
		
		// save the particle as a binary file
		binaryClass = P3rBuildBinaryData(swapbytes, classptr->definition, &binarySize);
		error = OBDrBinaryData_Save(P3cBinaryDataClass, classptr->classname,
									binaryClass, binarySize, 0, inAutosave);
		if (error) {
			COrConsole_Printf("can't save particle as binary data file");
			return error;
		}

		// if this is a 'real' save then we don't need to revert any more
		if (!inAutosave) {
			// free the memory used while saving
			UUrMemory_Block_Delete((void *) binaryClass);

			// this particle is no longer dirty
			P3rMakeDirty(classptr, UUcFalse);
			classptr->non_persistent_flags &= ~P3cParticleClass_NonPersistentFlag_OldVersion;
		}
	}

	if (!inAutosave) {
		P3rUpdateDirty();
	}
	return UUcError_None;
}

UUtError OWrSave_Particle(P3tParticleClass *inClassPtr)
{
	UUtUns8 *						binaryClass;
	UUtUns16						binarySize;
	UUtBool							swapbytes;
	UUtError						error;

#if (UUmEndian == UUmEndian_Big)
	swapbytes = UUcTrue;
#else
	swapbytes = UUcFalse;
#endif

	if (inClassPtr->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Locked)
		return UUcError_None;

	// save the particle as a binary file. note that this is not an autosave
	// (autosaves save all particles at once)
	binaryClass = P3rBuildBinaryData(swapbytes, inClassPtr->definition, &binarySize);
	error = OBDrBinaryData_Save(P3cBinaryDataClass, inClassPtr->classname, 
								binaryClass, binarySize, 0, UUcFalse);
	if (error) {
		COrConsole_Printf("can't save particle as binary data file");
		return error;
	}

	// free the memory used while saving
	UUrMemory_Block_Delete((void *) binaryClass);

	// this particle is no longer dirty
	P3rMakeDirty(inClassPtr, UUcFalse);
	P3rUpdateDirty();

	return UUcError_None;
}

UUtError OWrRevert_Particles(void)
{
	P3tParticleClassIterateState	itr;
	P3tParticleClass *				classptr;

	if (P3rAnyDirty() == UUcFalse)
		return UUcError_None;

	itr = P3cParticleClass_None;
	while(P3rIterateClasses(&itr, &classptr) == UUcTrue) {
		if (classptr->non_persistent_flags & P3cParticleClass_NonPersistentFlag_Dirty) {
			P3rRevert(classptr);
		}
	}

	P3rUpdateDirty();
	return UUcError_None;
}

UUtError OWrRevert_Particle(P3tParticleClass *inClassPtr)
{
	if (!P3rRevert(inClassPtr)) {
		// this particle class cannot be reverted, because it
		// wasn't loaded from disk
		WMrDialog_MessageBox(NULL, "Error",
							 "Sorry, this particle class was not loaded from disk and so cannot be reverted.", WMcMessageBoxStyle_OK);
		return UUcError_Generic;
	}

	P3rUpdateDirty();

	return UUcError_None;
}

static UUtBool OWiSelectAction(OWtParticle_Action_Data *inUserData, UUtUns16 inLine, UUtUns16 *outIndex)
{
	UUtUns16 itr;
	P3tActionTemplate *t;

	// which action is the first _after_ this point?
	for (itr = 0; itr < inUserData->num_actions; itr++) {
		if (inUserData->action[itr].actionpos > inLine)
			break;
	}

	// go back to the last action before this point
	if (itr == 0)
		return UUcFalse;

	itr--;
	if (itr == inUserData->num_actions - 1) {
		// this is the last action. check that we don't fall off the end
		t = inUserData->action[itr].actiontemplate;
		if (inLine - inUserData->action[itr].actionpos > 1 + t->num_variables + t->num_parameters)
			return UUcFalse;
	}

	*outIndex = itr;
	return UUcTrue;
}

// Clear listbox and add all actions to it.
static void OWiSetup_Listbox_Actions(WMtWindow *inListbox, OWtParticle_Action_Data *inUserData)
{
	UUtUns32						actionmask;
	UUtBool							can_disable;
	UUtUns16						listpos;
	UUtUns16						itr, itr2;
	P3tActionInstance *				act;
	P3tActionTemplate *				act_template;
	char							tempstring[64];
	char							actionstring[256];
	P3tActionList *					actionlist;

	UUmAssert((inUserData->listindex >= 0) && (inUserData->listindex < P3cMaxNumEvents));
	actionlist = &inUserData->classptr->definition->eventlist[inUserData->listindex];

	UUmAssert((actionlist->start_index >= 0) &&
			  (actionlist->end_index <= inUserData->classptr->definition->num_actions));
	UUmAssert(inUserData->num_actions == actionlist->end_index - actionlist->start_index);

	// clear the listbox
	WMrMessage_Send(inListbox, LBcMessage_Reset, 0, 0);

	// add each action to the inListbox
	listpos = 0;

	if (inUserData->listindex == P3cEvent_Update) {
		can_disable = UUcTrue;
		actionmask = inUserData->classptr->definition->actionmask;
	} else {
		can_disable = UUcFalse;
		actionmask = 0;
	}

	for (itr = 0, act = &inUserData->classptr->definition->action[actionlist->start_index];
		itr < inUserData->num_actions; itr++, act++) {
		// get the template of the action
		if (P3rGetActionInfo(act, &act_template) != UUcError_None)
			continue;

		if (itr > 0) {
			// blank line
			WMrMessage_Send(inListbox, LBcMessage_AddString, (UUtUns32) "", 0);
			listpos++;
		}

		// set up the action-entry
		inUserData->action[itr].actionpos = listpos;
		inUserData->action[itr].actiontemplate = act_template;

		/*
		 * write the description into the inListbox
 		 */
		
		// action's name
		sprintf(actionstring, "%s", act_template->name);
		if (can_disable) {
			strcat(actionstring, " - initially ");
			strcat(actionstring, (actionmask & (1 << itr)) ? "OFF" : "ON");
		}

		WMrMessage_Send(inListbox, LBcMessage_AddString, (UUtUns32) actionstring, 0);
		listpos++;
		
		// action's variables
		for (itr2 = 0; itr2 < act_template->num_variables; itr2++) {
			if (act->action_var[itr2].name[0] == '\0') {
				// this is a null variable reference
				sprintf(actionstring, "  %s: NONE",
					act_template->info[itr2].name);
			} else {
				sprintf(actionstring, "  %s: %s", 
					act_template->info[itr2].name, act->action_var[itr2].name);
			}
			WMrMessage_Send(inListbox, LBcMessage_AddString, (UUtUns32) actionstring, 0);
			listpos++;
		}
		
		// action's parameters
		for (itr2 = 0; itr2 < act_template->num_parameters; itr2++) {
			OWiDescribeValue(inUserData->classptr,
							 act_template->info[itr2 + act_template->num_variables].datatype,
							 &act->action_value[itr2], tempstring);
			sprintf(actionstring, "  %s: %s", act_template->info[itr2 + act_template->num_variables].name,
					tempstring);

			WMrMessage_Send(inListbox, LBcMessage_AddString, (UUtUns32) actionstring, 0);
			listpos++;
		}
	}
}

UUtBool
OWrParticle_Actions_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	OWtParticle_Action_Data *		user_data;
	P3tActionInstance *				act;
	P3tActionTemplate *				act_template;
	OWtParticle_Action_Entry *		act_entry;
	char							tempstring[64];
	char							actionstring[256];
	UUtUns16						orig_index, item_index, act_index, start_index;
	WMtWindow *						listbox;
	OWtParticle_Value_Data *		val_user_data;
	OWtParticle_VarRef_Data *		var_user_data;
	WMtDialog *						dialog;
	UUtUns32						modaldialog_message;
	WMtWindow *						list_text;
	P3tDataType						datatype;
	P3tActionList *					actionlist;

	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			user_data = (OWtParticle_Action_Data *) WMrDialog_GetUserData(inDialog);
			actionlist = &user_data->classptr->definition->eventlist[user_data->listindex];

			sprintf(actionstring, "Edit Actions (%s '%s')", user_data->classptr->classname,
					P3gEventName[user_data->listindex]);
			WMrWindow_SetTitle(inDialog, actionstring, WMcMaxTitleLength);

			// set up the action-info array in our user data
			user_data->num_actions = actionlist->end_index - actionlist->start_index;
			user_data->action = (OWtParticle_Action_Entry *) UUrMemory_Block_New(user_data->num_actions
																		* sizeof(OWtParticle_Action_Entry));

			listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Actions_Listbox);
			if (listbox != NULL)
				OWiSetup_Listbox_Actions(listbox, user_data);

			// set up the action list static text item
			list_text = WMrDialog_GetItemByID(inDialog, OWcParticle_Actions_IndexText);
			if (list_text != NULL) {
				sprintf(tempstring, "Action list is #%d - #%d",
						actionlist->start_index, actionlist->end_index);
				WMrWindow_SetTitle(list_text, tempstring, WMcMaxTitleLength);
			}

			// register that we are here and may need to be notified about changes
			OWiParticle_RegisterOpenWindow(inDialog);
		break;
		
		case WMcMessage_Destroy:
			// deallocate user data
			user_data = (OWtParticle_Action_Data *) WMrDialog_GetUserData(inDialog);

			// delete our allocated array and user data
			if (user_data->action != NULL) {
				UUrMemory_Block_Delete(user_data->action);
			}
			UUrMemory_Block_Delete(user_data);

			// register that we are going away and shouldn't be notified about any further changes
			OWiParticle_RegisterCloseWindow(inDialog);
		break;

		case OWcMessage_Particle_UpdateList:
			user_data = (OWtParticle_Action_Data *) WMrDialog_GetUserData(inDialog);
			actionlist = &user_data->classptr->definition->eventlist[user_data->listindex];

			if (OWiSelectAction(user_data, (UUtUns16) inParam1, &act_index) == UUcFalse)
				break;

			act = &user_data->classptr->definition->action[act_index + actionlist->start_index];
			act_entry = &user_data->action[act_index];

			listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
			if (listbox == NULL)
				break;

			// is this a variable or a parameter?
			item_index = (UUtUns16) (inParam1 - act_entry->actionpos - 1);
			if (item_index >= act_entry->actiontemplate->num_variables) {
				// this is a parameter
				item_index -= act_entry->actiontemplate->num_variables;
				UUmAssert((item_index >= 0) && (item_index < act_entry->actiontemplate->num_parameters));

				// form the line for this parameter
				datatype = act_entry->actiontemplate->info[item_index +
								act_entry->actiontemplate->num_variables].datatype;
				OWiDescribeValue(user_data->classptr, datatype, &act->action_value[item_index], tempstring);

				sprintf(actionstring, "  %s: %s", 
						act_entry->actiontemplate->info[act_entry->actiontemplate->num_variables
														+ item_index].name, tempstring);

				// write this line
				WMrMessage_Send(listbox, LBcMessage_ReplaceString, (UUtUns32) actionstring,
								act_entry->actionpos + 1 + item_index
								+ act_entry->actiontemplate->num_variables);
			} else {
				// this is a variable
				UUmAssert((item_index >= 0) && (item_index < act_entry->actiontemplate->num_variables));

				// form the line for this variable
				sprintf(actionstring, "  %s: %s", act_entry->actiontemplate->info[item_index].name,
						act->action_var[item_index].name);

				// write this line
				WMrMessage_Send(listbox, LBcMessage_ReplaceString, (UUtUns32) actionstring,
								act_entry->actionpos + 1 + item_index);
			}

			// the action has been changed and may need to update its private data
			P3rUpdateActionData(user_data->classptr, act);
			break;

		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case OWcParticle_Actions_Insert:
					user_data = (OWtParticle_Action_Data *) WMrDialog_GetUserData(inDialog);
					actionlist = &user_data->classptr->definition->eventlist[user_data->listindex];

					if ((actionlist->start_index == 0) && (actionlist->end_index >= 32)) {
						WMrDialog_MessageBox(NULL, "Too Many Actions",
											"Sorry, you can have at most 32 actions in the update list.",
											WMcMessageBoxStyle_OK);
						break;
					}

					// pop up a 'new action' dialog
					WMrDialog_ModalBegin(OWcDialog_Particle_NewAction, inDialog, OWrParticle_NewAction_Callback,
						                 (UUtUns32) &act_template, &modaldialog_message);

					if ((UUtBool) modaldialog_message == UUcTrue) {
						UUmAssertReadPtr(act_template, sizeof(P3tActionTemplate));

						// this particle class is now dirty. NB: must be called BEFORE we make any changes.
						P3rMakeDirty(user_data->classptr, UUcTrue);

						// which action is selected? if none, select the end of the list
						listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
						if (listbox == NULL)
							break;
						orig_index = (UUtUns16) WMrMessage_Send(listbox, LBcMessage_GetSelection,
															(UUtUns32) -1, (UUtUns32) -1);
						if (OWiSelectAction(user_data, orig_index, &act_index) == UUcFalse)
							act_index = user_data->num_actions;

						start_index = actionlist->start_index;

						// create a new action in the particle class, at the location of the selected
						// action.
						P3rClass_NewAction(user_data->classptr, user_data->listindex, act_index + start_index);

						// set up this action with default values
						act = &user_data->classptr->definition->action[act_index + start_index];
						act->action_type = act_template->type;
						P3rSetActionDefaults(act);

						// allocate space for this action in our user data
						user_data->num_actions++;
						user_data->action = (OWtParticle_Action_Entry *)
								UUrMemory_Block_Realloc((void *) user_data->action,
											user_data->num_actions * sizeof(OWtParticle_Action_Entry));

						// update the changes into the listbox
						listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
						if (listbox != NULL)
							OWiSetup_Listbox_Actions(listbox, user_data);

						// select the action
						WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32) -1,
										(UUtUns32) user_data->action[act_index].actionpos);

						// we may now need to autosave
						OBDrMakeDirty();

						// tell our parent eventlist window that our number of actions has changed
						OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
												 user_data->notify_struct.line_to_notify);
					}
					break;

				case OWcParticle_Actions_Listbox:
					// if the user double-clicks on a variable, edit it by
					// falling through this case statement to the edit button code
					if (UUmHighWord(inParam1) != WMcNotify_DoubleClick)
					{
						// any other interaction with the listbox does nothing
						break;
					}

				case OWcParticle_Actions_Edit:
					// get the currently selected action
					user_data = (OWtParticle_Action_Data *) WMrDialog_GetUserData(inDialog);
					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
					if (listbox == NULL)
						break;

					orig_index = (UUtUns16) WMrMessage_Send(listbox, LBcMessage_GetSelection,
															(UUtUns32) -1, (UUtUns32) -1);
					if (OWiSelectAction(user_data, orig_index, &act_index) == UUcFalse)
						break;

					start_index = user_data->classptr->definition->eventlist[user_data->listindex].start_index;
					act = &user_data->classptr->definition->action[act_index + start_index];

					// is this a parameter or a variable?
					item_index = orig_index - user_data->action[act_index].actionpos - 1;
					act_template = user_data->action[act_index].actiontemplate;
					if ((item_index < 0) || (item_index > act_template->num_variables + act_template->num_parameters))
						break;

					if (item_index >= act_template->num_variables) {
						item_index -= act_template->num_variables;

						UUmAssert((item_index >= 0) && (item_index < act_template->num_parameters));

						// set up data for the value edit dialog
						val_user_data = (OWtParticle_Value_Data *)
							UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
						val_user_data->classptr = user_data->classptr;
						val_user_data->valptr = &act->action_value[item_index];
						val_user_data->variable_allowed = UUcTrue;
						sprintf(val_user_data->title, "%s: parameter '%s'", 
								act_template->name, act_template->info[act_template->num_variables
																		+ item_index].name);

						val_user_data->notify_struct.line_to_notify = (UUtUns16) orig_index;
						val_user_data->notify_struct.notify_window	= inDialog;

						datatype = act_template->info[act_template->num_variables + item_index].datatype;
						switch (datatype) {
						case P3cDataType_Integer:
							WMrDialog_Create(OWcDialog_Particle_Value_Int, NULL,
											OWrParticle_Value_Int_Callback, (UUtUns32) val_user_data, &dialog);
							break;

						case P3cDataType_Float:
							WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
											OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
							break;

						case P3cDataType_String:
							WMrDialog_Create(OWcDialog_Particle_Value_String, NULL,
											OWrParticle_Value_String_Callback, (UUtUns32) val_user_data, &dialog);
							break;

						case P3cDataType_Shade:
							WMrDialog_Create(OWcDialog_Particle_Value_Shade, NULL,
											OWrParticle_Value_Shade_Callback, (UUtUns32) val_user_data, &dialog);
							break;
						
						case P3cDataType_Sound_Ambient:
							OWrParticle_Value_Sound_Get(SScType_Ambient, val_user_data);
							break;
							
						case P3cDataType_Sound_Impulse:
							OWrParticle_Value_Sound_Get(SScType_Impulse, val_user_data);
							break;
						
						default:
							if (datatype & P3cDataType_Enum) {
								P3rGetEnumInfo(user_data->classptr, datatype, &val_user_data->enum_info);

								if (val_user_data->enum_info.not_handled) {
/*									switch(val_user_data->enum_info.type) {
										case P3cEnumType_AmbientSoundID:
										{
											UUtError error;
											UUtUns32 sound_id;

											error = OWrSAS_Display(inDialog, &sound_id, NULL);
											if (error == UUcError_None) {
												val_user_data->valptr->type = P3cValueType_Enum;
												val_user_data->valptr->u.enum_const.val = sound_id;

												OWiParticle_NotifyParent(val_user_data->notify_struct.notify_window,
															(UUtUns32) val_user_data->notify_struct.line_to_notify);
											}
											break;
										}
										case P3cEnumType_ImpulseSoundID:
										{
											UUtError error;
											UUtUns32 sound_id;

											error = OWrSIS_Display(inDialog, &sound_id, NULL);
											if (error == UUcError_None) {
												val_user_data->valptr->type = P3cValueType_Enum;
												val_user_data->valptr->u.enum_const.val = sound_id;

												OWiParticle_NotifyParent(val_user_data->notify_struct.notify_window,
															(UUtUns32) val_user_data->notify_struct.line_to_notify);
											}
											break;
										}
										default:
											// don't know how to handle this value. do nothing.
											break;
									}*/
								} else {
									WMrDialog_Create(OWcDialog_Particle_Value_Enum, NULL,
													OWrParticle_Value_Enum_Callback, (UUtUns32) val_user_data, &dialog);
								}
							} else {
								UUmAssert(!"non-atomic data type for action template parameter!");
							}
						}
					} else {
						UUmAssert((item_index >= 0) && (item_index < act_template->num_variables));

						// set up data for the variable reference dialog
						var_user_data = (OWtParticle_VarRef_Data *)
							UUrMemory_Block_New(sizeof(OWtParticle_VarRef_Data));
						var_user_data->classptr = user_data->classptr;
						var_user_data->varptr = &act->action_var[item_index];
						sprintf(var_user_data->title, "%s: variable '%s'", 
								act_template->name, act_template->info[item_index].name);
						var_user_data->allowed_types = act_template->info[item_index].datatype;

						var_user_data->notify_struct.line_to_notify = (UUtUns16) orig_index;
						var_user_data->notify_struct.notify_window	= inDialog;

						WMrDialog_Create(OWcDialog_Particle_VarRef, NULL, OWrParticle_VarRef_Callback,
										(UUtUns32) var_user_data, &dialog);
					}

					break;

				case OWcParticle_Actions_Delete:
					// get the currently selected action
					user_data = (OWtParticle_Action_Data *) WMrDialog_GetUserData(inDialog);
					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
					if (listbox == NULL)
						break;

					item_index = (UUtUns16) WMrMessage_Send(listbox, LBcMessage_GetSelection,
															(UUtUns32) -1, (UUtUns32) -1);
					if (OWiSelectAction(user_data, item_index, &act_index) == UUcFalse)
						break;

					// pop up a dialog box
					sprintf(actionstring, "Really delete action '%s'?",
							user_data->action[act_index].actiontemplate->name);
					if (WMrDialog_MessageBox(NULL, "Are you sure?",
											actionstring, WMcMessageBoxStyle_Yes_No) != WMcDialogItem_Yes)
						break;

					// this particle class is now dirty. NB: must be called BEFORE we make any changes.
					P3rMakeDirty(user_data->classptr, UUcTrue);

					// we may now need to autosave
					OBDrMakeDirty();

					// remove this variable from the particle class. see rationale above attached to P3rClass_NewVar
					start_index = user_data->classptr->definition->eventlist[user_data->listindex].start_index;
					P3rClass_DeleteAction(user_data->classptr, user_data->listindex, act_index + start_index);

					// update our action array
					user_data->num_actions--;

					// re-set up our listbox
					if (listbox != NULL)
						OWiSetup_Listbox_Actions(listbox, user_data);

					// tell our parent eventlist window that our number of actions has changed
					OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
											 user_data->notify_struct.line_to_notify);
					break;

				case OWcParticle_Actions_Up:
					// get the currently selected action
					user_data = (OWtParticle_Action_Data *) WMrDialog_GetUserData(inDialog);
					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
					if (listbox == NULL)
						break;

					item_index = (UUtUns16) WMrMessage_Send(listbox, LBcMessage_GetSelection,
															(UUtUns32) -1, (UUtUns32) -1);
					if (OWiSelectAction(user_data, item_index, &act_index) == UUcFalse) {
						break;
					}

					// can't move action zero up
					if (act_index == 0) {
						break;
					}

					// this particle class is now dirty. NB: must be called BEFORE we make any changes.
					P3rMakeDirty(user_data->classptr, UUcTrue);

					// we may now need to autosave
					OBDrMakeDirty();

					// swap the actions in the particle class
					start_index = user_data->classptr->definition->eventlist[user_data->listindex].start_index;
					P3rSwapTwoActions(user_data->classptr, start_index + act_index - 1, start_index + act_index);
					
					// re-set up our listbox
					if (listbox != NULL)
						OWiSetup_Listbox_Actions(listbox, user_data);

					// select the action
					WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32) -1,
									(UUtUns32) user_data->action[act_index - 1].actionpos);
					break;

				case OWcParticle_Actions_Down:
					// get the currently selected action
					user_data = (OWtParticle_Action_Data *) WMrDialog_GetUserData(inDialog);
					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
					if (listbox == NULL)
						break;

					item_index = (UUtUns16) WMrMessage_Send(listbox, LBcMessage_GetSelection,
															(UUtUns32) -1, (UUtUns32) -1);
					if (OWiSelectAction(user_data, item_index, &act_index) == UUcFalse) {
						break;
					}

					// can't move last action down
					if (act_index == user_data->num_actions - 1) {
						break;
					}

					// this particle class is now dirty. NB: must be called BEFORE we make any changes.
					P3rMakeDirty(user_data->classptr, UUcTrue);

					// we may now need to autosave
					OBDrMakeDirty();

					// swap the actions in the particle class
					start_index = user_data->classptr->definition->eventlist[user_data->listindex].start_index;
					P3rSwapTwoActions(user_data->classptr, start_index + act_index + 1, start_index + act_index);
					
					// re-set up our listbox
					if (listbox != NULL)
						OWiSetup_Listbox_Actions(listbox, user_data);

					// select the action
					WMrMessage_Send(listbox, LBcMessage_SetSelection, (UUtUns32) -1,
									(UUtUns32) user_data->action[act_index + 1].actionpos);
					break;

				case OWcParticle_Actions_Toggle:
					// get the currently selected action
					user_data = (OWtParticle_Action_Data *) WMrDialog_GetUserData(inDialog);
					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Variables_Listbox);
					if (listbox == NULL)
						break;

					if (user_data->listindex != P3cEvent_Update) {
						WMrDialog_MessageBox(inDialog, "Cannot disable action!", "Only actions in the 'update' list can be disabled. Sorry.", WMcMessageBoxStyle_OK);
						break;
					}

					item_index = (UUtUns16) WMrMessage_Send(listbox, LBcMessage_GetSelection,
															(UUtUns32) -1, (UUtUns32) -1);
					if (OWiSelectAction(user_data, item_index, &act_index) == UUcFalse) {
						break;
					}

					UUmAssert((act_index >= 0) && (act_index < user_data->classptr->definition->num_actions));

					// this particle class is now dirty. NB: must be called BEFORE we make any changes.
					P3rMakeDirty(user_data->classptr, UUcTrue);

					// we may now need to autosave
					OBDrMakeDirty();

					// toggle the actionmask for this action
					user_data->classptr->definition->actionmask ^= (1 << act_index);

					// re-set up our listbox
					if (listbox != NULL)
						OWiSetup_Listbox_Actions(listbox, user_data);

					break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// Clear listbox and add all emitters to it.
static void OWiSetup_Listbox_Emitters(WMtWindow *inListbox, OWtParticle_Emitter_Data *inUserData)
{
	UUtUns16						listpos;
	UUtUns16						itr, itr2, type_itr, offset;
	UUtUns32						paramtype, *paramptr;
	P3tEmitterSettingDescription *	classdesc;
	P3tEmitterSettingDescription *	paramdesc;
	P3tEmitter *					emitter;
	char							tempstring[64];
	char							listbox_line[256];

	UUmAssert(inUserData->num_emitters == inUserData->classptr->definition->num_emitters);

	// clear the listbox
	WMrMessage_Send(inListbox, LBcMessage_Reset, 0, 0);

	// add each action to the inListbox
	listpos = 0;

	for (itr = 0, emitter = inUserData->classptr->definition->emitter;
		itr < inUserData->num_emitters; itr++, emitter++) {

//		UUrMemory_Block_VerifyList();

		inUserData->emitter_pos[(OWcParticleNumEmitterSections + 1)*itr + 0] = listpos;

		/*
		 * write the description into the inListbox
 		 */
		
		// first line: particle class, flags, etc
		sprintf(listbox_line, "#%d: emits '%s' [", itr, emitter->classname);

		if (emitter->flags & P3cEmitterFlag_InitiallyActive) {
			strcat(listbox_line, " initially-on");
		} else {
			strcat(listbox_line, " initially-off");
		}

		if (emitter->flags & P3cEmitterFlag_IncreaseEmitCount) {
			strcat(listbox_line, " count-particles");
		}

		if (emitter->flags & P3cEmitterFlag_DeactivateWhenEmitCountThreshold) {
			strcat(listbox_line, " off-at-threshold");
		} else if (emitter->flags & P3cEmitterFlag_ActivateWhenEmitCountThreshold) {
			strcat(listbox_line, " on-at-threshold");
		}

		if (emitter->flags & P3cEmitterFlag_AddParentVelocity) {
			strcat(listbox_line, " add-velocity");
		}

		if (emitter->flags & P3cEmitterFlag_SameDynamicMatrix) {
			strcat(listbox_line, " inherit-matrix");
		}

		if (emitter->flags & P3cEmitterFlag_OrientToVelocity) {
			strcat(listbox_line, " orient-to-vel");
		}

		if (emitter->flags & P3cEmitterFlag_InheritTint) {
			strcat(listbox_line, " inherit-tint");
		}

		if (emitter->flags & P3cEmitterFlag_OnePerAttractor) {
			strcat(listbox_line, " one-per-attractor");
		}

		if (emitter->flags & P3cEmitterFlag_AtLeastOne) {
			strcat(listbox_line, " at-least-one");
		}

		if (emitter->flags & P3cEmitterFlag_RotateAttractors) {
			strcat(listbox_line, " cycle-attractors");
		}

		strcat(listbox_line, " ]");
		WMrMessage_Send(inListbox, LBcMessage_AddString, (UUtUns32) listbox_line, 0);
		listpos++;

		// second line: emit chance
		sprintf(listbox_line, "emit chance: %.3f", (100.0f * emitter->emit_chance) / UUcMaxUns16);
		WMrMessage_Send(inListbox, LBcMessage_AddString, (UUtUns32) listbox_line, 0);
		listpos++;

		// third line: burst size
		sprintf(listbox_line, "burst count: %.3f", emitter->burst_size);
		WMrMessage_Send(inListbox, LBcMessage_AddString, (UUtUns32) listbox_line, 0);
		listpos++;

		// fourth line: particle limit
		sprintf(listbox_line, "particle limit: %d", emitter->particle_limit);
		WMrMessage_Send(inListbox, LBcMessage_AddString, (UUtUns32) listbox_line, 0);
		listpos++;

		// fifth line: emitter link-type
		P3rDescribeEmitterLinkType(inUserData->classptr, emitter, emitter->link_type, tempstring);
		sprintf(listbox_line, "link type: %s", tempstring);
		WMrMessage_Send(inListbox, LBcMessage_AddString, (UUtUns32) listbox_line, 0);
		listpos++;

		for (type_itr = 0; type_itr < OWcParticleNumEmitterSections; type_itr++) {
			// store the position of this in the list (for use when determining what
			// part of which emitter is selected)
			inUserData->emitter_pos[(OWcParticleNumEmitterSections + 1)*itr + type_itr + 1] = listpos;

			// get data about this subsection of the emitter's properties (rate, position, etc)
//			UUrMemory_Block_VerifyList();
			P3rEmitterSubsection(type_itr, emitter, &paramptr, &classdesc, &offset, tempstring);
			paramtype = *paramptr;

			// write the line describing this type
			paramdesc = &classdesc[paramtype];
			UUmAssert(paramdesc->setting_enum == paramtype);

//			UUrMemory_Block_VerifyList();
			sprintf(listbox_line, "  %s: %s", tempstring, paramdesc->setting_name);
			WMrMessage_Send(inListbox, LBcMessage_AddString, (UUtUns32) listbox_line, 0);
			listpos++;
//			UUrMemory_Block_VerifyList();

			// write the variables
			for (itr2 = 0; itr2 < paramdesc->num_params; itr2++) {
				OWiDescribeValue(inUserData->classptr, paramdesc->param_desc[itr2].data_type,
								&emitter->emitter_value[offset + itr2], tempstring);
				sprintf(listbox_line, "    %s: %s", paramdesc->param_desc[itr2].param_name, tempstring);
				WMrMessage_Send(inListbox, LBcMessage_AddString, (UUtUns32) listbox_line, 0);
				listpos++;
			}
		}
	}
}

UUtBool
OWrParticle_Emitters_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	OWtParticle_Emitter_Data *		user_data;
	WMtWindow *						listbox;
	UUtUns16						itr, itr2, emitter_index, section_index, offset_index, value_offset;
	UUtUns16						orig_index;
	P3tEmitter *					emitter;
	UUtUns32						*current_param, new_choice;
	P3tEmitterSettingDescription *	class_desc;
	char							section_name[64], errmsg[128];
	OWtParticle_Value_Data *		val_user_data;
	OWtParticle_EmitterFlags_Data *	flags_user_data;
	OWtParticle_EmitterChoice_Data *choice_user_data;
	OWtParticle_Number_Data *		number_user_data;
	OWtParticle_LinkType_Data *		linktype_user_data;
	WMtDialog *						dialog;
	P3tDataType						datatype;
	UUtError						error;
	UUtUns32						modaldialog_message;

	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			user_data = (OWtParticle_Emitter_Data *) WMrDialog_GetUserData(inDialog);

			// set up the emitter-info array in our user data
			user_data->num_emitters = user_data->classptr->definition->num_emitters;
			user_data->emitter_pos = (UUtUns16 *) UUrMemory_Block_New((OWcParticleNumEmitterSections + 1) * user_data->num_emitters * sizeof(UUtUns16));
			user_data->set_percentage_chance = UUcFalse;
			user_data->temp_percentage_chance = 100.0f;
			user_data->store_percentage_chance = NULL;

			listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Emitters_Listbox);
			if (listbox != NULL)
				OWiSetup_Listbox_Emitters(listbox, user_data);

			// register that we are here and may need to be notified about changes
			OWiParticle_RegisterOpenWindow(inDialog);
		break;
		
		case WMcMessage_Destroy:
			// deallocate allocated array and user data
			user_data = (OWtParticle_Emitter_Data *) WMrDialog_GetUserData(inDialog);

			if (user_data->emitter_pos != NULL) {
				UUrMemory_Block_Delete(user_data->emitter_pos);
			}
			UUrMemory_Block_Delete(user_data);

			// register that we are going away and shouldn't be notified about any further changes
			OWiParticle_RegisterCloseWindow(inDialog);
		break;

		case OWcMessage_Particle_UpdateList:
			// just update the entire listbox again
			user_data = (OWtParticle_Emitter_Data *) WMrDialog_GetUserData(inDialog);

			if (user_data->set_percentage_chance) {
				UUmAssert(user_data->store_percentage_chance != NULL);
				*(user_data->store_percentage_chance) = (UUtUns16) 
					MUrUnsignedSmallFloat_To_Uns_Round(UUcMaxUns16 * user_data->temp_percentage_chance / 100.0f);
			}

			listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Emitters_Listbox);
			if (listbox != NULL)
				OWiSetup_Listbox_Emitters(listbox, user_data);
			break;

		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case OWcParticle_Emitters_Insert:
					user_data = (OWtParticle_Emitter_Data *) WMrDialog_GetUserData(inDialog);

					// this particle class is now dirty. NB: must be called BEFORE we make any changes.
					P3rMakeDirty(user_data->classptr, UUcTrue);

					// we may now need to autosave
					OBDrMakeDirty();

					// add a new emitter, with default parameters
					error = P3rClass_NewEmitter(user_data->classptr, &emitter);
					if (error != UUcError_None)
						break;

//					UUmAssertWritePtr(emitter, sizeof(P3tEmitter));
//					UUrMemory_Block_VerifyList();

					// re-pack the particles' variables - more emitter data must now be stored per particle
					// NB: this initialises each currently existing particle's newly added emitter to 0x00 hex
					// which will be active. they will remain active even if the "initially off" flag is later
					// selected.
					P3rPackVariables(user_data->classptr, UUcTrue);
//					UUrMemory_Block_VerifyList();

					// resize our private data
					user_data->num_emitters = user_data->classptr->definition->num_emitters;
					user_data->emitter_pos = (UUtUns16 *) UUrMemory_Block_Realloc(user_data->emitter_pos,
																(OWcParticleNumEmitterSections + 1) * user_data->num_emitters * sizeof(UUtUns16));

					strcpy(emitter->classname, "");
					emitter->emittedclass = NULL;
					emitter->burst_size = 0.0f;
					emitter->particle_limit = 0;
					emitter->flags = 0;

					emitter->rate_type = 0;
					emitter->position_type = 0;
					emitter->direction_type = 0;
					emitter->speed_type = 0;

					for (itr = 0; itr < OWcParticleNumEmitterSections; itr++) {
						P3rEmitterSubsection(itr, emitter, &current_param, &class_desc,
											&value_offset, section_name);

						UUmAssert(class_desc[*current_param].setting_enum == *current_param);
						UUmAssert((value_offset >= 0) && (value_offset + class_desc[*current_param].num_params < P3cEmitterValue_NumVals));

						for (itr2 = 0; itr2 < class_desc[*current_param].num_params; itr2++) {
							P3rSetDefaultInitialiser(class_desc[*current_param].param_desc[itr2].data_type,
													&emitter->emitter_value[value_offset + itr2]);
						}
					}

					// make sure that we aren't creating an emitter with an infinite
					// emission rate.
					emitter->rate_type = P3cEmitterRate_Continuous;
					emitter->emitter_value[P3cEmitterValue_RateValOffset + 0].type = P3cValueType_Float;
					emitter->emitter_value[P3cEmitterValue_RateValOffset + 0].u.float_const.val = 0.5f;

//					UUrMemory_Block_VerifyList();

					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Emitters_Listbox);
					if (listbox == NULL)
						break;

					OWiSetup_Listbox_Emitters(listbox, user_data);
					break;

				case OWcParticle_Emitters_Listbox:
					// if the user double-clicks on a variable, edit it by
					// falling through this case statement to the edit button code
					if (UUmHighWord(inParam1) != WMcNotify_DoubleClick)
					{
						// any other interaction with the listbox does nothing
						break;
					}

				case OWcParticle_Emitters_Edit:
					// get the currently selected line
					user_data = (OWtParticle_Emitter_Data *) WMrDialog_GetUserData(inDialog);

					if (user_data->num_emitters == 0)
						break;

					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Emitters_Listbox);
					if (listbox == NULL)
						break;

					orig_index = (UUtUns16) WMrMessage_Send(listbox, LBcMessage_GetSelection,
															(UUtUns32) -1, (UUtUns32) -1);

					// find out where this lies in the list of emitters
					for (itr = 0; itr < (OWcParticleNumEmitterSections + 1) * user_data->num_emitters; itr++) {
						if (user_data->emitter_pos[itr] > orig_index)
							break;
					}
					UUmAssert(itr > 0);
					itr--;
					emitter_index = itr / (OWcParticleNumEmitterSections + 1);
					section_index = itr % (OWcParticleNumEmitterSections + 1);
					offset_index = orig_index - user_data->emitter_pos[itr];

					UUmAssert((emitter_index >= 0) && (emitter_index < user_data->num_emitters));
					emitter = &user_data->classptr->definition->emitter[emitter_index];

					if (section_index == 0) {
						// selected one of the topmost lines
						UUmAssert((offset_index >= 0) && (offset_index < OWcParticleNumEmitterHeaderLines));

						if (offset_index == 0) {
							// selected the title line, pop up the emitter flags dialog
							flags_user_data = (OWtParticle_EmitterFlags_Data *)
								UUrMemory_Block_New(sizeof(OWtParticle_EmitterFlags_Data));
							flags_user_data->classptr = user_data->classptr;
							flags_user_data->emitter = emitter;
							flags_user_data->notify_struct.notify_window = inDialog;
							flags_user_data->notify_struct.line_to_notify = orig_index;

							// create the window
							WMrDialog_Create(OWcDialog_Particle_EmitterFlags, NULL,
											OWrParticle_EmitterFlags_Callback, (UUtUns32) flags_user_data, &dialog);

						} else if (offset_index == 1) {
							// selected the emit-chance line, pop up a number editing dialog
							user_data->set_percentage_chance = UUcTrue;
							user_data->temp_percentage_chance = (100.0f * emitter->emit_chance) / UUcMaxUns16;
							user_data->store_percentage_chance = &emitter->emit_chance;

							number_user_data = (OWtParticle_Number_Data *)
								UUrMemory_Block_New(sizeof(OWtParticle_Number_Data));
							number_user_data->classptr = user_data->classptr;
							number_user_data->notify_struct.notify_window = inDialog;
							number_user_data->notify_struct.line_to_notify = orig_index;
							sprintf(number_user_data->title, "Edit %s emitter %d chance to emit",
									user_data->classptr->classname, emitter_index);
							strcpy(number_user_data->editmsg, "Percentage chance:");
							number_user_data->is_float = UUcTrue;
							number_user_data->ptr.floatptr = &user_data->temp_percentage_chance;

							// create the window
							WMrDialog_Create(OWcDialog_Particle_Number, NULL,
											OWrParticle_Number_Callback, (UUtUns32) number_user_data, &dialog);

						} else if (offset_index == 2) {
							// selected the burst-count line, pop up a number editing dialog
							number_user_data = (OWtParticle_Number_Data *)
								UUrMemory_Block_New(sizeof(OWtParticle_Number_Data));
							number_user_data->classptr = user_data->classptr;
							number_user_data->notify_struct.notify_window = inDialog;
							number_user_data->notify_struct.line_to_notify = orig_index;
							sprintf(number_user_data->title, "Edit %s emitter %d burst size",
									user_data->classptr->classname, emitter_index);
							strcpy(number_user_data->editmsg, "Burst size:");
							number_user_data->is_float = UUcTrue;
							number_user_data->ptr.floatptr = &emitter->burst_size;

							// create the window
							WMrDialog_Create(OWcDialog_Particle_Number, NULL,
											OWrParticle_Number_Callback, (UUtUns32) number_user_data, &dialog);
						} else if (offset_index == 3) {
							// selected the particle-limit line, pop up a number editing dialog
							number_user_data = (OWtParticle_Number_Data *)
								UUrMemory_Block_New(sizeof(OWtParticle_Number_Data));
							number_user_data->classptr = user_data->classptr;
							number_user_data->notify_struct.notify_window = inDialog;
							number_user_data->notify_struct.line_to_notify = orig_index;
							sprintf(number_user_data->title, "Edit %s emitter %d particle limit",
									user_data->classptr->classname, emitter_index);
							strcpy(number_user_data->editmsg, "Particle limit:");
							number_user_data->is_float = UUcFalse;
							number_user_data->ptr.numberptr = &emitter->particle_limit;

							// create the window
							WMrDialog_Create(OWcDialog_Particle_Number, NULL,
											OWrParticle_Number_Callback, (UUtUns32) number_user_data, &dialog);
						} else {
							// selected the link-type line, pop up a link-type choice dialog
							linktype_user_data = (OWtParticle_LinkType_Data *)
								UUrMemory_Block_New(sizeof(OWtParticle_LinkType_Data));
							linktype_user_data->classptr = user_data->classptr;
							linktype_user_data->notify_struct.notify_window = inDialog;
							linktype_user_data->notify_struct.line_to_notify = orig_index;
							sprintf(linktype_user_data->title, "Edit %s emitter %d link type",
									user_data->classptr->classname, emitter_index);
							strcpy(linktype_user_data->editmsg, "Link Type:");
							linktype_user_data->emitter = emitter;

							// create the window
							WMrDialog_Create(OWcDialog_Particle_EmitterChoice, NULL,
											OWrParticle_LinkType_Callback, (UUtUns32) linktype_user_data, &dialog);
						}
					} else {
						// selected a value somewhere in the emitter's properties
						section_index--;
						P3rEmitterSubsection(section_index, emitter, &current_param, &class_desc,
											&value_offset, section_name);

						if (offset_index == 0) {
							// create a modal dialog window that lets us change the type of
							// this section
							choice_user_data = (OWtParticle_EmitterChoice_Data *)
								UUrMemory_Block_New(sizeof(OWtParticle_EmitterChoice_Data));
							choice_user_data->classptr = user_data->classptr;
							choice_user_data->emitter = emitter;
							choice_user_data->classname = section_name;
							choice_user_data->class_desc = class_desc;
							choice_user_data->current_choice = *current_param;
							choice_user_data->new_choice = &new_choice;

							WMrDialog_ModalBegin(OWcDialog_Particle_EmitterChoice, inDialog,
												OWrParticle_EmitterChoice_Callback, (UUtUns32) choice_user_data,
												&modaldialog_message);

							if ((modaldialog_message == (UUtUns32) UUcFalse) || (new_choice == *current_param)) {
								// no changes were made
								break;
							}
	
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							// set up default values for any parameters that are changing types
							for (itr = 0; itr < class_desc[new_choice].num_params; itr++) {
								// if this parameter didn't exist in the old setting, or if it has
								// changed types, we need to reset it.
								if ((itr >= class_desc[*current_param].num_params) ||
									(class_desc[new_choice].param_desc[itr].data_type !=
									 class_desc[*current_param].param_desc[itr].data_type)) {
									// set up the default initialiser for this value
									P3rSetDefaultInitialiser(class_desc[new_choice].param_desc[itr].data_type,
															&emitter->emitter_value[value_offset + itr]);
								}
							}

							// set the new type of component
							*current_param = new_choice;

							// we may now need to autosave
							OBDrMakeDirty();

							// re-set up our listbox
							OWiSetup_Listbox_Emitters(listbox, user_data);
						} else {
							// this is a value that can be changed
							offset_index--;
							UUmAssert((offset_index >= 0) && (offset_index < class_desc[*current_param].num_params));

							// set up data for the value edit dialog
							val_user_data = (OWtParticle_Value_Data *)
								UUrMemory_Block_New(sizeof(OWtParticle_Value_Data));
							val_user_data->classptr = user_data->classptr;
							val_user_data->valptr = &emitter->emitter_value[offset_index + value_offset];							
							val_user_data->variable_allowed = UUcTrue;
							sprintf(val_user_data->title, "emitter #%d %s (%s): parameter '%s'", 
									emitter_index, section_name, class_desc[*current_param].setting_name,
									class_desc[*current_param].param_desc[offset_index].param_name);

							val_user_data->notify_struct.line_to_notify = (UUtUns16) orig_index;
							val_user_data->notify_struct.notify_window	= inDialog;

							datatype = class_desc[*current_param].param_desc[offset_index].data_type;

							switch (datatype) {
							case P3cDataType_Integer:
								WMrDialog_Create(OWcDialog_Particle_Value_Int, NULL,
									OWrParticle_Value_Int_Callback, (UUtUns32) val_user_data, &dialog);
								break;
								
							case P3cDataType_Float:
								WMrDialog_Create(OWcDialog_Particle_Value_Float, NULL,
									OWrParticle_Value_Float_Callback, (UUtUns32) val_user_data, &dialog);
								break;
								
							case P3cDataType_String:
								WMrDialog_Create(OWcDialog_Particle_Value_String, NULL,
									OWrParticle_Value_String_Callback, (UUtUns32) val_user_data, &dialog);
								break;
								
							case P3cDataType_Shade:
								WMrDialog_Create(OWcDialog_Particle_Value_Shade, NULL,
									OWrParticle_Value_Shade_Callback, (UUtUns32) val_user_data, &dialog);
								break;
						
							case P3cDataType_Sound_Ambient:
								OWrParticle_Value_Sound_Get(SScType_Ambient, val_user_data);
								break;
								
							case P3cDataType_Sound_Impulse:
								OWrParticle_Value_Sound_Get(SScType_Impulse, val_user_data);
								break;
							
							default:
								if (datatype & P3cDataType_Enum) {
									P3rGetEnumInfo(user_data->classptr, datatype, &val_user_data->enum_info);
									
									if (val_user_data->enum_info.not_handled) {
/*										switch(val_user_data->enum_info.type) {
											case P3cEnumType_AmbientSoundID:
											{
												UUtError error;
												UUtUns32 sound_id;

												error = OWrSAS_Display(inDialog, &sound_id, NULL);
												if (error == UUcError_None) {
													val_user_data->valptr->type = P3cValueType_Enum;
													val_user_data->valptr->u.enum_const.val = sound_id;

													OWiParticle_NotifyParent(val_user_data->notify_struct.notify_window,
																(UUtUns32) val_user_data->notify_struct.line_to_notify);
												}
												break;
											}
											case P3cEnumType_ImpulseSoundID:
											{
												UUtError error;
												UUtUns32 sound_id;

												error = OWrSIS_Display(inDialog, &sound_id, NULL);
												if (error == UUcError_None) {
													val_user_data->valptr->type = P3cValueType_Enum;
													val_user_data->valptr->u.enum_const.val = sound_id;

													OWiParticle_NotifyParent(val_user_data->notify_struct.notify_window,
																(UUtUns32) val_user_data->notify_struct.line_to_notify);
												}
												break;
											}
											default:
												// don't know how to handle this value. do nothing.
												break;
										}*/
									} else {
										WMrDialog_Create(OWcDialog_Particle_Value_Enum, NULL,
											OWrParticle_Value_Enum_Callback, (UUtUns32) val_user_data, &dialog);
									}
								} else {
									UUmAssert(!"non-atomic data type for action template parameter!");
								}
							}
						}
					}
					break;

				case OWcParticle_Emitters_Delete:
					// get the currently selected action
					user_data = (OWtParticle_Emitter_Data *) WMrDialog_GetUserData(inDialog);

					if (user_data->num_emitters == 0)
						break;

					listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Emitters_Listbox);
					if (listbox == NULL)
						break;

					orig_index = (UUtUns16) WMrMessage_Send(listbox, LBcMessage_GetSelection,
															(UUtUns32) -1, (UUtUns32) -1);

					// find out where this lies in the list of emitters
					for (itr = 0; itr < (OWcParticleNumEmitterSections + 1) * user_data->num_emitters; itr++) {
						if (user_data->emitter_pos[itr] > orig_index)
							break;
					}
					UUmAssert(itr > 0);
					itr--;
					emitter_index = itr / (OWcParticleNumEmitterSections + 1);

					// pop up a dialog box
					sprintf(errmsg, "Really delete emitter '%d'?", emitter_index);
					if (WMrDialog_MessageBox(NULL, "Are you sure?",
											errmsg, WMcMessageBoxStyle_Yes_No) != WMcDialogItem_Yes)
						break;

					// this particle class is now dirty. NB: must be called BEFORE we make any changes.
					P3rMakeDirty(user_data->classptr, UUcTrue);

					// we may now need to autosave
					OBDrMakeDirty();

					// remove this variable from the particle class. see rationale above attached to P3rClass_NewVar
					P3rClass_DeleteEmitter(user_data->classptr, emitter_index);

					// re-pack the particles' variables - less emitter data must now be stored per particle
					P3rPackVariables(user_data->classptr, UUcTrue);

					// update our emitter array (don't bother resizing)
					user_data->num_emitters--;

					// re-set up our listbox
					OWiSetup_Listbox_Emitters(listbox, user_data);

					break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_EmitterFlags_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool								handled, close_window, has_changed, checked;
	char								classname[P3cParticleClassNameLength + 1], errormsg[256];
	WMtWindow 							*edit_field, *checkbox;
	OWtParticle_EmitterFlags_Data *		user_data;
	P3tParticleClass *					new_classptr;
	UUtUns32							new_flags;
	
	handled = UUcTrue;
	user_data = (OWtParticle_EmitterFlags_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// set up the initial particle class name
			edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_ClassName);
			UUmAssert(edit_field != NULL);
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) user_data->emitter->classname, 0);

			// set up the checkboxes
			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_InitiallyOn);
			if (checkbox != NULL) {
				checked = (user_data->emitter->flags & P3cEmitterFlag_InitiallyActive) ? UUcTrue : UUcFalse;

				WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);				
			}

			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_IncreaseCount);
			if (checkbox != NULL) {
				checked = (user_data->emitter->flags & P3cEmitterFlag_IncreaseEmitCount) ? UUcTrue : UUcFalse;

				WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);				
			}

			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_OffAtThreshold);
			if (checkbox != NULL) {
				checked = (user_data->emitter->flags & P3cEmitterFlag_DeactivateWhenEmitCountThreshold) ? UUcTrue : UUcFalse;

				WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);				
			}

			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_OnAtThreshold);
			if (checkbox != NULL) {
				checked = (user_data->emitter->flags & P3cEmitterFlag_ActivateWhenEmitCountThreshold) ? UUcTrue : UUcFalse;

				WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);				
			}

			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_ParentVelocity);
			if (checkbox != NULL) {
				checked = (user_data->emitter->flags & P3cEmitterFlag_AddParentVelocity) ? UUcTrue : UUcFalse;

				WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);				
			}

			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_DynamicMatrix);
			if (checkbox != NULL) {
				checked = (user_data->emitter->flags & P3cEmitterFlag_SameDynamicMatrix) ? UUcTrue : UUcFalse;

				WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);				
			}

			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_OrientToVelocity);
			if (checkbox != NULL) {
				checked = (user_data->emitter->flags & P3cEmitterFlag_OrientToVelocity) ? UUcTrue : UUcFalse;

				WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);				
			}

			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_InheritTint);
			if (checkbox != NULL) {
				checked = (user_data->emitter->flags & P3cEmitterFlag_InheritTint) ? UUcTrue : UUcFalse;

				WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);				
			}

			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_OnePerAttractor);
			if (checkbox != NULL) {
				checked = (user_data->emitter->flags & P3cEmitterFlag_OnePerAttractor) ? UUcTrue : UUcFalse;

				WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);				
			}

			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_AtLeastOne);
			if (checkbox != NULL) {
				checked = (user_data->emitter->flags & P3cEmitterFlag_AtLeastOne) ? UUcTrue : UUcFalse;

				WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);				
			}

			checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_RotateAttractors);
			if (checkbox != NULL) {
				checked = (user_data->emitter->flags & P3cEmitterFlag_RotateAttractors) ? UUcTrue : UUcFalse;

				WMrMessage_Send(checkbox, CBcMessage_SetCheck, (UUtUns32) checked, (UUtUns32) -1);				
			}
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);
		break;
		
		case WMcMessage_Command:
			close_window = UUcFalse;

			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				case WMcDialogItem_OK:
					close_window = UUcTrue;	// fall through to apply case
				case OWcParticle_EmitterFlags_Apply:
					edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_ClassName);
					UUmAssert(edit_field != NULL);
					WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) classname, P3cParticleClassNameLength + 1);

					// is this a real particle class?
					new_classptr = P3rGetParticleClass(classname);
					if (new_classptr == NULL) {
						sprintf(errormsg, "Could not find particle class '%s'. Do you want to store this name anyway?",
								classname);
						if (WMrDialog_MessageBox(NULL, "Unknown Particle Class", errormsg,
												WMcMessageBoxStyle_OK_Cancel) == WMcDialogItem_Cancel) {
							// user has hit cancel, ignore this update request
							break;
						}
					}

					has_changed = UUcFalse;
					if (strcmp(user_data->emitter->classname, classname) != 0) {
						if (!has_changed) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);
						}

						// this is a new name
						has_changed = UUcTrue;

						// update the particle class
						strcpy(user_data->emitter->classname, classname);
						user_data->emitter->emittedclass = new_classptr;
					}

					// read the flag checkboxes
					new_flags = 0;

					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_InitiallyOn);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);
						if (checked)
							new_flags |= P3cEmitterFlag_InitiallyActive;
					}
					
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_IncreaseCount);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);
						if (checked)
							new_flags |= P3cEmitterFlag_IncreaseEmitCount;
					}
					
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_OffAtThreshold);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);
						if (checked)
							new_flags |= P3cEmitterFlag_DeactivateWhenEmitCountThreshold;
					}
					
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_OnAtThreshold);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);
						if (checked)
							new_flags |= P3cEmitterFlag_ActivateWhenEmitCountThreshold;
					}
					
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_ParentVelocity);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);
						if (checked)
							new_flags |= P3cEmitterFlag_AddParentVelocity;
					}
					
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_DynamicMatrix);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);
						if (checked)
							new_flags |= P3cEmitterFlag_SameDynamicMatrix;
					}
					
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_OrientToVelocity);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);
						if (checked)
							new_flags |= P3cEmitterFlag_OrientToVelocity;
					}
					
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_InheritTint);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);
						if (checked)
							new_flags |= P3cEmitterFlag_InheritTint;
					}
					
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_OnePerAttractor);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);
						if (checked)
							new_flags |= P3cEmitterFlag_OnePerAttractor;
					}
					
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_AtLeastOne);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);
						if (checked)
							new_flags |= P3cEmitterFlag_AtLeastOne;
					}
					
					checkbox = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterFlags_RotateAttractors);
					if (checkbox != NULL) {
						checked = (UUtBool) WMrMessage_Send(checkbox, WMcMessage_GetValue,
															(UUtUns32) -1, (UUtUns32) -1);
						if (checked)
							new_flags |= P3cEmitterFlag_RotateAttractors;
					}
					
					// do we need to update the emitter?
					if (new_flags != user_data->emitter->flags) {
						if (!has_changed) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);
						}

						user_data->emitter->flags = new_flags;
						has_changed = UUcTrue;
					}

					if (has_changed) {
						// notify the parent window to update itself
						OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
										(UUtUns32) user_data->notify_struct.line_to_notify);

						// we may now need to autosave
						OBDrMakeDirty();
					}

					if (close_window)
						WMrWindow_Delete(inDialog);
					break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_LinkType_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool						handled;
	WMtWindow *					text_field;
	WMtMenuItemData				item_data;
	WMtPopupMenu *				menu;
	OWtParticle_LinkType_Data *	user_data;
	UUtUns32					itr;
	UUtUns16					new_linktype;
	
	handled = UUcTrue;
	user_data = (OWtParticle_LinkType_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// set our dialog's title
			WMrWindow_SetTitle(inDialog, user_data->title, WMcMaxTitleLength);

			// set the textfield
			text_field = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterChoice_Text);
			if (text_field != NULL) {
				WMrWindow_SetTitle(text_field, user_data->editmsg, WMcMaxTitleLength);
			}

			menu = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterChoice_Popup);
			if (menu != NULL) {
				// set up the items in the popup menu
				for (itr = 0; itr < P3cEmitterLinkType_Max; itr++) {
					// don't set up invalid items unless they're the current choice
					if ((!P3rDescribeEmitterLinkType(user_data->classptr, user_data->emitter, itr, item_data.title)) &&
						(itr != (UUtUns32) user_data->emitter->link_type))
						continue;

					item_data.flags = WMcMenuItemFlag_Enabled;
					item_data.id = (UUtUns16) itr;

					WMrPopupMenu_AppendItem(menu, &item_data);
				}
			}

			WMrPopupMenu_SetSelection(menu, (UUtUns16) user_data->emitter->link_type);			
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
				break;

				case WMcDialogItem_OK:
					// get the current menu selection
					menu = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterChoice_Popup);
					if (menu != NULL) {
						WMrPopupMenu_GetItemID(menu, -1, &new_linktype);

						if (new_linktype != (UUtUns16) user_data->emitter->link_type) {
							// this particle class is now dirty. NB: must be called BEFORE we make any changes.
							P3rMakeDirty(user_data->classptr, UUcTrue);

							// set the new value
							user_data->emitter->link_type = (P3tEmitterLinkType) new_linktype;

							// notify the parent window to update itself
							OWiParticle_NotifyParent(user_data->notify_struct.notify_window,
											(UUtUns32) user_data->notify_struct.line_to_notify);

							// we may now need to autosave
							OBDrMakeDirty();
						}
					}

					WMrWindow_Delete(inDialog);
				break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_EmitterChoice_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool								handled;
	OWtParticle_EmitterChoice_Data *	user_data;
	P3tEmitterSettingDescription *		setting_desc;
	WMtWindow *							text_field;
	WMtPopupMenu *						menu;
	char								tempstring[64];
	UUtUns16							itr, selected_id;
	WMtMenuItemData						item_data;
	
	handled = UUcTrue;
	user_data = (OWtParticle_EmitterChoice_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// set up our dialog's title
			sprintf(tempstring, "Set Emitter #%d %s",
						(user_data->emitter - user_data->classptr->definition->emitter), user_data->classname);
			WMrWindow_SetTitle(inDialog, tempstring, WMcMaxTitleLength);
			
			// set up the static text item
			text_field = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterChoice_Text);
			if (text_field != NULL) {
				sprintf(tempstring, "%s:", user_data->classname);
				WMrWindow_SetTitle(text_field, tempstring, WMcMaxTitleLength);
			}

			// set up the items in the popup menu
			menu = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterChoice_Popup);
			if (menu != NULL) {
				for (itr = 0, setting_desc = user_data->class_desc; ; itr++, setting_desc++) {
					// watch for the sentinel
					if (setting_desc->setting_enum == (UUtUns32) -1)
						break;

					UUmAssert(setting_desc->setting_enum == itr);

					item_data.flags = WMcMenuItemFlag_Enabled;
					item_data.id = (UUtUns16) setting_desc->setting_enum;
					strcpy(item_data.title, setting_desc->setting_name);

					WMrPopupMenu_AppendItem(menu, &item_data);
				}
			}

			WMrPopupMenu_SetSelection(menu, (UUtUns16) user_data->current_choice);			
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, (UUtUns32) UUcFalse);
					break;

				case WMcDialogItem_OK:
					// get the current menu selection
					menu = WMrDialog_GetItemByID(inDialog, OWcParticle_EmitterChoice_Popup);
					if (menu != NULL) {
						WMrPopupMenu_GetItemID(menu, -1, &selected_id);

						*(user_data->new_choice) = (UUtUns32) selected_id;
					}
					WMrDialog_ModalEnd(inDialog, (UUtUns32) UUcTrue);
					break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_EventList_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool							handled;
	OWtParticle_Edit_Data			*user_data;
	OWtParticle_Action_Data *		action_user_data;
	WMtDialog *						dialog;
	WMtWindow *						text;
	char							title[64];
	UUtUns16						itr, num_actions;
	P3tActionList *					actionlist;
	
	handled = UUcTrue;
	user_data = (OWtParticle_Edit_Data *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// set the dialog title
			sprintf(title, "Edit %s Events", user_data->classptr->classname);
			WMrWindow_SetTitle(inDialog, title, WMcMaxTitleLength);

			for (itr = 0; itr < P3cEvent_NumEvents; itr++) {
				// set the event name text
				text = WMrDialog_GetItemByID(inDialog, OWcParticle_EventList_TextBase + itr);
				if (text != NULL) {
					sprintf(title, "%s:", P3gEventName[itr]);
					WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
				}

				// set the event description text
				text = WMrDialog_GetItemByID(inDialog, OWcParticle_EventList_DescBase + itr);
				if (text != NULL) {
					actionlist = &user_data->classptr->definition->eventlist[itr];
					num_actions = actionlist->end_index - actionlist->start_index;
					sprintf(title, "[%d action", num_actions);
					if (num_actions != 1) {
						strcat(title, "s");
					}
					strcat(title, "]");
					WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
				}
			}

			// register that we are here and may need to be notified about changes
			OWiParticle_RegisterOpenWindow(inDialog);
		break;
		
		case WMcMessage_Destroy:
			UUrMemory_Block_Delete(user_data);

			// register that we are going away and shouldn't be notified about any further changes
			OWiParticle_RegisterCloseWindow(inDialog);
		break;
		
		case OWcMessage_Particle_UpdateList:
			// set the event description text for whichever event has been updated
			text = WMrDialog_GetItemByID(inDialog, OWcParticle_EventList_DescBase + (UUtUns16) inParam1);
			if (text != NULL) {
				actionlist = &user_data->classptr->definition->eventlist[inParam1];
				num_actions = actionlist->end_index - actionlist->start_index;
				sprintf(title, "[%d action", num_actions);
				if (num_actions != 1) {
					strcat(title, "s");
				}
				strcat(title, "]");
				WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
			}
			break;

		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrWindow_Delete(inDialog);
					break;

				default:
					itr = UUmLowWord(inParam1) - OWcParticle_EventList_ButtonBase;

					if ((itr >= 0) && (itr < P3cEvent_NumEvents)) {
						action_user_data = (OWtParticle_Action_Data *) UUrMemory_Block_New(sizeof(OWtParticle_Action_Data));

						// pop up an action editing dialog for this event
						action_user_data->classptr = user_data->classptr;
						action_user_data->listindex = itr;
						action_user_data->notify_struct.notify_window = inDialog;
						action_user_data->notify_struct.line_to_notify = itr;

						WMrDialog_Create(OWcDialog_Particle_Actions, NULL, OWrParticle_Actions_Callback,
										(UUtUns32) action_user_data, &dialog);
					}
					break;

			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

UUtBool
OWrParticle_ClassName_Callback(
	WMtDialog				*inDialog,
	WMtMessage				inMessage,
	UUtUns32				inParam1,
	UUtUns32				inParam2)
{
	UUtBool						handled;
	char						errormsg[256], *classname;
	WMtWindow *					edit_field;
	
	handled = UUcTrue;
	classname = (char *) WMrDialog_GetUserData(inDialog);
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			// set up the initial texture name
			edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_Name);
			UUmAssert(edit_field != NULL);
			WMrMessage_Send(edit_field, EFcMessage_SetText, (UUtUns32) classname, 0);
		break;
		
		case WMcMessage_Destroy:
			// nothing to delete
		break;
		
		case WMcMessage_Command:
			switch (UUmLowWord(inParam1))
			{
				case WMcDialogItem_Cancel:
					WMrDialog_ModalEnd(inDialog, (UUtUns32) UUcFalse);
					break;

				case WMcDialogItem_OK:
					edit_field = WMrDialog_GetItemByID(inDialog, OWcParticle_Class_Name);
					UUmAssert(edit_field != NULL);
					WMrMessage_Send(edit_field, EFcMessage_GetText, (UUtUns32) classname, 32);

					// can't exceed the fixed size of particle class names
					if ((strlen(classname) < 1) || (strlen(classname) > P3cParticleClassNameLength)) {
						sprintf(errormsg, "Sorry, class names must be 1-%d letters long.",
							    P3cParticleClassNameLength);
						WMrDialog_MessageBox(inDialog, "Error",
											errormsg, WMcMessageBoxStyle_OK);
						break;
					}

					// check that we don't overwrite another class
					if (P3rGetParticleClass(classname) != NULL) {
						sprintf(errormsg, "A particle class already exists with the name '%s'.",
							    classname);
						WMrDialog_MessageBox(inDialog, "Error",
											errormsg, WMcMessageBoxStyle_OK);
						break;
					}

					WMrDialog_ModalEnd(inDialog, (UUtUns32) UUcTrue);
					break;
			}
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}


//================================================================================================================
// decal textures 
//================================================================================================================

static char			*OWgParticle_DecalTexture_Name;

static UUtError OWrParticle_DecalTextures_InitDialog( WMtDialog *inDialog )
{
	M3tTextureMap				**texture_list;
	UUtUns32					num_textures;
	UUtError					error;
	UUtUns32					i;
	WMtWindow					*listbox;
	
	// get the number of textures
	num_textures = TMrInstance_GetTagCount(M3cTemplate_TextureMap);
	if (num_textures == 0) { return UUcError_Generic; }
	
	// allocate memory to hold the list of textures
	texture_list = (M3tTextureMap**)UUrMemory_Block_New(sizeof(M3tTextureMap*) * num_textures);
	if (texture_list == NULL) { return UUcError_Generic; }
	
	OWgParticle_DecalTexture_Name = (char*) WMrDialog_GetUserData(inDialog);

	UUmAssert( OWgParticle_DecalTexture_Name );

	WMrDialog_SetUserData(inDialog, (UUtUns32)texture_list);
	
	// get a list of the textures
	error = TMrInstance_GetDataPtr_List( M3cTemplate_TextureMap, num_textures, &num_textures, texture_list );
	UUmError_ReturnOnError(error);
	
	// put the textures into the listbox
	listbox = WMrDialog_GetItemByID(inDialog, OWcParticle_Decal_Textures);
	for (i = 0; i < num_textures; i++)
	{
		WMrMessage_Send(listbox, LBcMessage_AddString, i, 0);
	}
	
	// select the first item in the list
	WMrListBox_SetSelection(listbox, UUcTrue, 0);
	
	// set the focus
	WMrWindow_SetFocus(listbox);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void OWrParticle_DecalTextures_Destroy( WMtDialog *inDialog )
{
	M3tTextureMap				**texture_list;
	
	texture_list = (M3tTextureMap**)WMrDialog_GetUserData(inDialog);
	
	// delete the texture list
	UUrMemory_Block_Delete(texture_list);
	texture_list = NULL;
}

// ----------------------------------------------------------------------
static UUtError OWrParticle_DecalTextures_HandleCommand( WMtDialog *inDialog, UUtUns32 inParam1, WMtWindow *inControl)
{
	switch (UUmLowWord(inParam1))
	{
		case OWcParticle_Decal_Textures:
		{
			M3tTextureMap		**texture_list;
			UUtUns32			index;
			M3tTextureMap		*texture;
			
			if (UUmHighWord(inParam1) != LBcNotify_SelectionChanged) { break; }
			
			// get a pointer to the texture
			texture_list	= (M3tTextureMap**)WMrDialog_GetUserData(inDialog);
			index			= WMrListBox_GetSelection(inControl);
			texture			= texture_list[index];
			
			WMrPicture_SetPicture(WMrDialog_GetItemByID(inDialog, OWcParticle_Decal_Picture),texture);
		}
		break;

		case OWcParticle_Decal_Select:
			// create the decal class
			{
				WMtListBox			*listbox;
				UUtUns32			index;
				M3tTextureMap		**texture_list;
				M3tTextureMap		*texture;
				P3tParticleClass	*new_class;
				char				class_name[256];
				char				texture_name[256];
				UUtUns32			modalreturn;
				
				listbox			= WMrDialog_GetItemByID(inDialog, OWcParticle_Decal_Textures);
				texture_list	= (M3tTextureMap**)WMrDialog_GetUserData(inDialog);
				index			= WMrListBox_GetSelection(listbox);
				texture			= texture_list[index];

				sprintf( texture_name, TMrInstance_GetInstanceName(texture) );

				sprintf( class_name, "d_%s", texture_name );

				if( strlen(class_name) > P3cStringVarSize )
					class_name[P3cStringVarSize+1] = 0;
				
				WMrDialog_ModalBegin(OWcDialog_Particle_ClassName, inDialog, OWrParticle_ClassName_Callback, (UUtUns32) class_name, &modalreturn );			
				if( (UUtBool) modalreturn == UUcTrue ) 
				{
					new_class = P3rNewParticleClass(class_name,UUcTrue);

					UUmAssert( new_class );
					
					if( new_class )
					{
						new_class->definition->flags								|= P3cParticleClassFlag_HasDecalData | P3cParticleClassFlag_Decorative;
						new_class->definition->flags2								|= P3cParticleClassFlag2_Appearance_Decal;
						new_class->definition->lifetime.type						= P3cValueType_Float;
						new_class->definition->lifetime.u.float_const.val			= 0.0f;
						new_class->definition->appearance.scale.type				= P3cValueType_Float;
						new_class->definition->appearance.scale.u.float_const.val	= 1.0;
						new_class->definition->appearance.y_scale.type				= P3cValueType_Float;
						new_class->definition->appearance.y_scale.u.float_const.val	= 1.0;

						P3rPackVariables( new_class, UUcTrue );

						strcpy( new_class->definition->appearance.instance, texture_name );

						strcpy( OWgParticle_DecalTexture_Name, new_class->classname );

						WMrDialog_ModalEnd(inDialog, 1);
						break;
					}
				}
				// otherwise fall through to cancel....
			}
		case OWcParticle_Decal_Cancel:
			OWgParticle_DecalTexture_Name[0]	= 0;					
			WMrDialog_ModalEnd(inDialog, 0);
		break;
	}
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError OWrParticle_DecalTextures_HandleMenuCommand( WMtDialog *inDialog, WMtWindow *inPopupMenu, UUtUns32 inItemID)
{
	WMtListBox					*listbox;
	M3tTextureMap				**texture_list;
	UUtUns32					index;
	M3tTextureMap				*texture;
	
	// get a pointer to the texture
	listbox			= WMrDialog_GetItemByID(inDialog, OWcParticle_Decal_Textures);
	texture_list	= (M3tTextureMap**)WMrDialog_GetUserData(inDialog);
	index			= WMrListBox_GetSelection(listbox);
	texture			= texture_list[index];
	
	// set the focus
	WMrWindow_SetFocus(listbox);

	return UUcError_None;
}

// ----------------------------------------------------------------------
static void OWrParticle_DecalTextures_HandleDrawItem( WMtDialog *inDialog, WMtDrawItem *inDrawItem)
{
	IMtPoint2D					dest;
//	PStPartSpecUI				*partspec_ui;
	UUtInt16					line_width;
	UUtInt16					line_height;
	M3tTextureMap				**texture_list;
	M3tTextureMap				*texture;
	
	texture_list = (M3tTextureMap**)WMrDialog_GetUserData(inDialog);
	UUmAssert(texture_list);
	
	// set the dest
	dest.x = inDrawItem->rect.left;
	dest.y = inDrawItem->rect.top;
	
	// calc the width and height of the line
	line_width	= inDrawItem->rect.right  - inDrawItem->rect.left;
	line_height = inDrawItem->rect.bottom - inDrawItem->rect.top;
		
	dest.x = 5;
	
	// draw the texture name
	texture = texture_list[inDrawItem->data];

	DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
	DCrDraw_String(inDrawItem->draw_context,TMrInstance_GetInstanceName(texture),&inDrawItem->rect,&dest);
}

// ----------------------------------------------------------------------
static UUtBool OWrParticle_DecalTextures_Callback( WMtDialog *inDialog, WMtMessage inMessage, UUtUns32 inParam1, UUtUns32 inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWrParticle_DecalTextures_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWrParticle_DecalTextures_Destroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWrParticle_DecalTextures_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;
		
		case WMcMessage_MenuCommand:
			OWrParticle_DecalTextures_HandleMenuCommand(inDialog, (WMtWindow*)inParam2, UUmLowWord(inParam1));
		break;
		
		case WMcMessage_DrawItem:
			OWrParticle_DecalTextures_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

#endif
