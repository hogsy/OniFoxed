#if TOOL_VERSION
// ======================================================================
// Oni_Win_ImpactEffects.c
// ======================================================================

// ======================================================================
// includes
// ======================================================================
#include "BFW.h"
#include "BFW_Materials.h"
#include "BFW_SoundSystem2.h"
#include "BFW_WindowManager.h"
#include "WM_PopupMenu.h"
#include "WM_EditField.h"
#include "WM_CheckBox.h"

#include "Oni_ImpactEffect.h"
#include "Oni_Sound2.h"
#include "Oni_Windows.h"
#include "Oni_Win_ImpactEffects.h"
#include "Oni_Win_Sound2.h"
#include "Oni_Character.h"
#include "Oni_AI2_Knowledge.h"

// ======================================================================
// defines
// ======================================================================
#define OWcIEListBox_IndentDepth		3
#define OWcEntry_DoesNotBelong			(1 << 31)

// ======================================================================
// typedefs
// ======================================================================
typedef struct OWtIEprop
{
	UUtUns32					entry_index;
	UUtUns32					selected_particle;
	
} OWtIEprop;

typedef struct OWtIEdata
{
	MAtMaterialType				cur_material_type;
	MAtImpactType				cur_impact_type;
	UUtUns16					cur_mod_type;

	UUtUns16					num_entries;
	UUtUns32					entry_baseindex;

	UUtUns32					found_entries[ONcIEComponent_Max];
	
	UUtBool						clipboard_full;
} OWtIEdata;

// ======================================================================
// constants
// ======================================================================
const char *OWcParticleLocationNames[P3cEffectLocationType_Max] =
{"Default", "Offset", "<unused>", "Find Wall", "Decal", "Attached To Object"};

const char *OWcParticleOrientationNames[P3cEnumCollisionOrientation_Max] =
{"Unchanged", "Reversed", "Reflected", "Perpendicular"};

// ======================================================================
// functions
// ======================================================================
static UUtBool
OWiIEprop_EmptySound(
	ONtIESound					*inSound)
{
/*	if (inSound->impulse_id != SScInvalidID)
		return UUcFalse;*/
	if (inSound->sound_name[0] != '\0')
		return UUcFalse;
	
	if (inSound->flags & ONcIESoundFlag_AICanHear)
		return UUcFalse;

	return UUcTrue;
}

static void
OWiIEprop_SetupParticleControls(
	WMtDialog					*inDialog)
{
	OWtIEprop					*properties;
	WMtEditField				*ef_classname = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWcIEprop_EF_ParticleClass);
	WMtPopupMenu				*pm_orientation = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWcIEprop_PM_ParticleOrientation);
	WMtPopupMenu				*pm_location = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWcIEprop_PM_ParticleLocation);
	WMtEditField				*ef_params[5];
	WMtWindow					*txt_params[5];
	WMtCheckBox					*cb_params[5];
	UUtUns32					itr, num_particles;
	ONtIEParticle				*particle, *sel_particle;

	properties = (OWtIEprop*)WMrDialog_GetUserData(inDialog);
	if (properties == NULL)
	{
		WMrDialog_ModalEnd(inDialog, 0);
		return;
	}

	for (itr = 0; itr < 5; itr++) {
		ef_params[itr] = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWcIEprop_EF_Param0 + (UUtUns16) itr);
		txt_params[itr] = WMrDialog_GetItemByID(inDialog, OWcIEprop_Txt_Param0 + (UUtUns16) itr);
		cb_params[itr] = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWcIEprop_CB_Check0 + (UUtUns16) itr);
	}

	particle = ONrImpactEffect_GetParticles(properties->entry_index, &num_particles);
	if ((properties->selected_particle < 0) || (properties->selected_particle >= num_particles)) {
		// disable all particle controls
		WMrEditField_SetText(ef_classname, "");
		WMrWindow_SetEnabled(ef_classname, UUcFalse);
		WMrWindow_SetEnabled(pm_orientation, UUcFalse);
		WMrWindow_SetEnabled(pm_location, UUcFalse);

		for (itr = 0; itr < 5; itr++) {
			WMrWindow_SetVisible(ef_params[itr], UUcFalse);
			WMrWindow_SetVisible(txt_params[itr], UUcFalse);
			WMrWindow_SetVisible(cb_params[itr], UUcFalse);
		}
	} else {
		sel_particle = &particle[properties->selected_particle];

		// enable controls as appropriate
		WMrWindow_SetEnabled(ef_classname, UUcTrue);
		WMrEditField_SetText(ef_classname, sel_particle->particle_class_name);

		WMrWindow_SetEnabled(pm_orientation, UUcTrue);
		WMrPopupMenu_SetSelection(pm_orientation, (UUtUns16) sel_particle->effect_spec.collision_orientation);

		WMrWindow_SetEnabled(pm_location, UUcTrue);
		WMrPopupMenu_SetSelection(pm_location, (UUtUns16) sel_particle->effect_spec.location_type);

		switch(sel_particle->effect_spec.location_type) {
			case P3cEffectLocationType_Default:
				// no parameters
				for (itr = 0; itr < 5; itr++) {
					WMrWindow_SetVisible(ef_params[itr], UUcFalse);
					WMrWindow_SetVisible(txt_params[itr], UUcFalse);
					WMrWindow_SetVisible(cb_params[itr], UUcFalse);
				}
			break;

			case P3cEffectLocationType_CollisionOffset:
				// only one parameter - collision offset
				WMrWindow_SetVisible(txt_params[0], UUcTrue);
				WMrWindow_SetTitle(txt_params[0], "Offset:", WMcMaxTitleLength);
				WMrWindow_SetVisible(ef_params[0], UUcTrue);
				WMrEditField_SetFloat(ef_params[0], sel_particle->effect_spec.location_data.collisionoffset.offset);

				// the rest are invisible
				WMrWindow_SetVisible(cb_params[0], UUcFalse);
				for (itr = 1; itr < 5; itr++) {
					WMrWindow_SetVisible(ef_params[itr], UUcFalse);
					WMrWindow_SetVisible(txt_params[itr], UUcFalse);
					WMrWindow_SetVisible(cb_params[itr], UUcFalse);
				}
			break;

			case P3cEffectLocationType_FindWall:
				// only one parameter - find wall distance
				WMrWindow_SetVisible(txt_params[0], UUcTrue);
				WMrWindow_SetTitle(txt_params[0], "Radius:", WMcMaxTitleLength);
				WMrWindow_SetVisible(ef_params[0], UUcTrue);
				WMrEditField_SetFloat(ef_params[0], sel_particle->effect_spec.location_data.findwall.radius);

				// one checkbox
				WMrWindow_SetVisible(cb_params[0], UUcTrue);
				WMrWindow_SetTitle(cb_params[0], "Look Along +Y", WMcMaxTitleLength);
				WMrCheckBox_SetCheck(cb_params[0], sel_particle->effect_spec.location_data.findwall.look_along_orientation);

				// the rest are invisible
				for (itr = 1; itr < 5; itr++) {
					WMrWindow_SetVisible(ef_params[itr], UUcFalse);
					WMrWindow_SetVisible(txt_params[itr], UUcFalse);
					WMrWindow_SetVisible(cb_params[itr], UUcFalse);
				}
			break;

			case P3cEffectLocationType_Decal:
				// all decals created from impact effects are dynamic and have random rotation and default size
				sel_particle->effect_spec.location_data.decal.static_decal = UUcFalse;
				sel_particle->effect_spec.location_data.decal.random_rotation = UUcTrue;
				for (itr = 0; itr < 5; itr++) 
				{
					WMrWindow_SetVisible(ef_params[itr], UUcFalse);
					WMrWindow_SetVisible(txt_params[itr], UUcFalse);
					WMrWindow_SetVisible(cb_params[itr], UUcFalse);
				}
			break;

			default:
				UUmAssert(!"OWiIEprop_SetupParticleControls: unknown location type");
			break;
		}
	}

	if (ONgImpactEffect_Locked) {
		// use the visibility status we calculated earlier, but disable everything
		WMrWindow_SetEnabled(ef_classname, UUcFalse);
		WMrWindow_SetEnabled(pm_orientation, UUcFalse);
		WMrWindow_SetEnabled(pm_location, UUcFalse);

		for (itr = 0; itr < 5; itr++) {
			WMrWindow_SetEnabled(ef_params[itr], UUcFalse);
			WMrWindow_SetEnabled(txt_params[itr], UUcFalse);
			WMrWindow_SetEnabled(cb_params[itr], UUcFalse);
		}
	}
}

// ----------------------------------------------------------------------
static void
OWiIEprop_DisplaySound(
	WMtDialog					*inDialog)
{
	OWtIEprop					*properties;
	WMtWindow					*text;
	WMtWindow					*edit_button;
	WMtWindow					*delete_button;
	WMtWindow					*set_button;
	WMtCheckBox					*ai_canhear;
	WMtPopupMenu				*ai_type;
	WMtEditField				*ai_radius;
//	char						title[WMcMaxTitleLength];
	UUtBool						valid = UUcFalse;
	ONtIESound					*sound;

	// get a pointer to the properties
	properties = (OWtIEprop *) WMrDialog_GetUserData(inDialog);
	if (properties == NULL)
	{
		WMrDialog_ModalEnd(inDialog, 0);
		return;
	}

	text = WMrDialog_GetItemByID(inDialog, OWcIEprop_Txt_SoundName);
	edit_button = WMrDialog_GetItemByID(inDialog, OWcIEprop_Btn_SoundEdit);
	delete_button = WMrDialog_GetItemByID(inDialog, OWcIEprop_Btn_SoundDelete);
	set_button = WMrDialog_GetItemByID(inDialog, OWcIEprop_Btn_SoundSet);
	ai_canhear = (WMtCheckBox *) WMrDialog_GetItemByID(inDialog, OWcIEprop_CB_AISound);
	ai_type = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWcIEprop_PM_AISoundType);
	ai_radius = (WMtEditField *) WMrDialog_GetItemByID(inDialog, OWcIEprop_EF_AISoundRadius);

	sound = ONrImpactEffect_GetSound(properties->entry_index);

	// set the sound title
/*	if (sound == NULL) {
		WMrWindow_SetTitle(text, "(none)", WMcMaxTitleLength);
	} else if (sound->sound_name[0] == '\0') {
		WMrWindow_SetTitle(text, "(none)", WMcMaxTitleLength);
	} else if (sound->impulse_id == SScInvalidID) {
		sprintf(title, "%s (not found)", sound->sound_name);
		WMrWindow_SetTitle(text, title, WMcMaxTitleLength);
	} else {
		WMrWindow_SetTitle(text, sound->sound_name, WMcMaxTitleLength);
		valid = UUcTrue;
	}*/
	if (sound == NULL) {
		WMrWindow_SetTitle(text, "(none)", WMcMaxTitleLength);
	} else {
		WMrWindow_SetTitle(text, sound->sound_name, WMcMaxTitleLength);
		valid = UUcTrue;
	}

	// can only edit or delete real sounds
	WMrWindow_SetEnabled(edit_button, valid);
	WMrWindow_SetEnabled(delete_button, valid);

	if (sound == NULL) {
		WMrCheckBox_SetCheck(ai_canhear, UUcFalse);
		WMrWindow_SetEnabled(ai_radius, UUcFalse);
		WMrWindow_SetEnabled(ai_type, UUcFalse);		
	} else {
		// set the AI parameters
		if (sound->flags & ONcIESoundFlag_AICanHear) {
			WMrCheckBox_SetCheck(ai_canhear, UUcTrue);
			WMrWindow_SetEnabled(ai_radius, UUcTrue);
			WMrWindow_SetEnabled(ai_type, UUcTrue);		

			WMrEditField_SetFloat(ai_radius, sound->ai_soundradius);
			if ((sound->ai_soundtype >= AI2cContactType_Sound_Ignore) && (sound->ai_soundtype <= AI2cContactType_Sound_Gunshot)) {
				WMrPopupMenu_SetSelection(ai_type, sound->ai_soundtype);
			} else {
				WMrPopupMenu_SetSelection(ai_type, AI2cContactType_Sound_Ignore);
			}
		} else {
			WMrCheckBox_SetCheck(ai_canhear, UUcFalse);
			WMrWindow_SetEnabled(ai_radius, UUcFalse);
			WMrWindow_SetEnabled(ai_type, UUcFalse);		
		}
	}

	if (ONgImpactEffect_Locked) {
		// disable everything
		WMrWindow_SetEnabled(edit_button, UUcFalse);
		WMrWindow_SetEnabled(delete_button, UUcFalse);
		WMrWindow_SetEnabled(set_button, UUcFalse);
		WMrWindow_SetEnabled(ai_canhear, UUcFalse);
		WMrWindow_SetEnabled(ai_radius, UUcFalse);
		WMrWindow_SetEnabled(ai_type, UUcFalse);
	}
}

// ----------------------------------------------------------------------
static void
OWiIEprop_DisplayParticles(
	WMtDialog					*inDialog)
{
	OWtIEprop					*properties;
	WMtWindow					*listbox;
	UUtUns32					itr, num_particles;
	ONtIEParticle				*particle;

	// get a pointer to the properties
	properties = (OWtIEprop *) WMrDialog_GetUserData(inDialog);
	if (properties == NULL)
	{
		WMrDialog_ModalEnd(inDialog, 0);
		return;
	}

	listbox = WMrDialog_GetItemByID(inDialog, OWcIEprop_LB_Particles);
	if (listbox == NULL) {
		return;
	}

	particle = ONrImpactEffect_GetParticles(properties->entry_index, &num_particles);
	WMrMessage_Send(listbox, LBcMessage_Reset, 0, 0);
	for (itr = 0; itr < num_particles; itr++) {
		WMrMessage_Send(listbox, LBcMessage_AddString, (UUtUns32) particle[itr].particle_class_name, 0);
	}
	WMrMessage_Send(listbox, LBcMessage_SetSelection, UUcFalse, properties->selected_particle);

	if (ONgImpactEffect_Locked) {
		// disable the buttons
		WMrWindow_SetEnabled(WMrDialog_GetItemByID(inDialog, OWcIEprop_Btn_ParticleNew), UUcFalse);
		WMrWindow_SetEnabled(WMrDialog_GetItemByID(inDialog, OWcIEprop_Btn_ParticleDelete), UUcFalse);
	}

	OWiIEprop_SetupParticleControls(inDialog);
}

// ----------------------------------------------------------------------
static void
OWiIEprop_InitDialog(
	WMtDialog					*inDialog)
{
	OWtIEprop					*properties;
	WMtWindow					*mod_popup, *com_popup;
	WMtPopupMenu				*orient_popup, *location_popup;
	char						title[WMcMaxTitleLength];
	ONtImpactEntry				*entry;

	// get a pointer to the properties
	properties = (OWtIEprop *) WMrDialog_GetUserData(inDialog);
	if (properties == NULL)
	{
		WMrDialog_ModalEnd(inDialog, 0);
		return;
	}

	entry = ONrImpactEffect_GetEntry(properties->entry_index);

	// build the title
	sprintf(title, "Edit Impact Entry for %s vs %s", MArImpactType_GetName(entry->impact_type),
			MArMaterialType_GetName(entry->material_type));
	WMrWindow_SetTitle(inDialog, title, WMcMaxTitleLength);

	// set up the popup menus
	mod_popup = WMrDialog_GetItemByID(inDialog, OWcIEprop_PM_ModType);
	WMrPopupMenu_SetSelection(mod_popup, entry->modifier);
	
	com_popup = WMrDialog_GetItemByID(inDialog, OWcIEprop_PM_Component);
	WMrPopupMenu_SetSelection(com_popup, entry->component);

	orient_popup = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWcIEprop_PM_ParticleOrientation);
	WMrPopupMenu_AppendStringList(orient_popup, P3cEnumCollisionOrientation_Max, OWcParticleOrientationNames);

	location_popup = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWcIEprop_PM_ParticleLocation);
	WMrPopupMenu_AppendStringList(location_popup, P3cEffectLocationType_Max, OWcParticleLocationNames);

	// get the sound; set the name
	OWiIEprop_DisplaySound(inDialog);

	// get the particles and set up the listbox (will set up particle controls in doing so)
	properties->selected_particle = (UUtUns32) -1;
	OWiIEprop_DisplayParticles(inDialog);

	if (ONgImpactEffect_Locked) {
		// disable the popup menus
		WMrWindow_SetEnabled(mod_popup, UUcFalse);
		WMrWindow_SetEnabled(com_popup, UUcFalse);
	}
}

// ----------------------------------------------------------------------
static UUtError
OWiIEprop_NewSound(
	OWtIEprop					*inProperties,
	ONtIESound					**outSound)
{
	UUtError error;

	error = ONrImpactEffect_NewSound(inProperties->entry_index, outSound);
	if (error != UUcError_None) {
		return error;
	}

	UUrString_Copy((*outSound)->sound_name, "", BFcMaxFileNameLength);
//	(*outSound)->impulse_id = SScInvalidID;
	(*outSound)->impulse_ptr = NULL;
	(*outSound)->flags = 0;
	(*outSound)->ai_soundradius = 0;
	(*outSound)->ai_soundtype = AI2cContactType_Sound_Ignore;
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OWiIEprop_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	WMtWindow					*inControl)
{
	UUtError					error;
	OWtIEprop					*properties;
//	OStItem						*item;
	ONtIESound					*sound;
	ONtIEParticle				*particle, *sel_particle;
	ONtImpactEntry				*entry;
//	OStCollection				*collection;
	UUtUns32					num_particles, field_index;
	char						new_classname[P3cParticleClassNameLength + 1];
	UUtBool						checked;

	properties = (OWtIEprop*)WMrDialog_GetUserData(inDialog);
	
	switch (UUmLowWord(inParam1))
	{
		case OWcIEprop_Btn_OK:
			WMrDialog_ModalEnd(inDialog, UUcTrue);
		break;

		case OWcIEprop_LB_Particles:
			if (UUmHighWord(inParam1) == LBcNotify_SelectionChanged) {
				properties->selected_particle = WMrListBox_GetSelection((WMtListBox *) inControl);
				OWiIEprop_DisplayParticles(inDialog);
			}
		break;

		case OWcIEprop_Btn_SoundSet:
		{
			SStImpulse			*impulse;
			OWtSelectResult		result;
			
			sound = ONrImpactEffect_GetSound(properties->entry_index);
			if (sound == NULL) {
				error = OWiIEprop_NewSound(properties, &sound);
				if (error != UUcError_None) {
					WMrDialog_MessageBox(inDialog, "Internal Error", "Cannot add sound to entry!", WMcMessageBoxStyle_OK);
					break;
				}
				UUrString_Copy(sound->sound_name, "", BFcMaxFileNameLength);
				sound->impulse_ptr = NULL;
			}

			result = OWrSelect_ImpulseSound(&impulse);
			if (result == OWcSelectResult_Cancel) { break; }
			
			sound->impulse_ptr = impulse;
			
			if (sound->impulse_ptr == NULL) {
				UUrString_Copy(sound->sound_name, "", SScMaxNameLength);
			} else {
				UUrString_Copy(
					sound->sound_name,
					sound->impulse_ptr->impulse_name,
					SScMaxNameLength);
			}
			
			if (OWiIEprop_EmptySound(sound)) {
				// delete this empty sound entry
				entry = ONrImpactEffect_GetEntry(properties->entry_index);
				error = ONrImpactEffect_DeleteSound(UUcFalse, entry->sound_index);
				if (error != UUcError_None) {
					WMrDialog_MessageBox(inDialog, "Internal Error", "Cannot delete unused sound from entry!", WMcMessageBoxStyle_OK);
					break;
				}
			} else {
				// store the name of the sound
				UUrString_Copy(sound->sound_name, sound->impulse_ptr->impulse_name, BFcMaxFileNameLength);
			}

			error = OWrImpactEffect_MakeDirty();
			if (error != UUcError_None) {
				WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
				break;
			}

			// update the dialog
			OWiIEprop_DisplaySound(inDialog);
		}
		break;

		case OWcIEprop_Btn_SoundEdit:
		{
			sound = ONrImpactEffect_GetSound(properties->entry_index);
			if (sound == NULL) {
				break;
			}
			
			if (sound->impulse_ptr == NULL)
			{
				sound->impulse_ptr = OSrImpulse_GetByName(sound->sound_name);
				if (sound->impulse_ptr == NULL)
				{
					WMrDialog_MessageBox(
						inDialog,
						"Error",
						"Unable to find the impulse sound for this Impact Effect.  "
						"Set a new sound, or recreate the one that no longer seems to exist.",
						WMcMessageBoxStyle_OK);
					break;
				}
			}
			
			// edit this sound
			OWrImpulseProperties_Display(inDialog, sound->impulse_ptr);

			// update the title as necessary
			UUrString_Copy(sound->sound_name, sound->impulse_ptr->impulse_name, BFcMaxFileNameLength);

			error = OWrImpactEffect_MakeDirty();
			if (error != UUcError_None) {
				WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
				break;
			}

			OWiIEprop_DisplaySound(inDialog);
			break;
		}

		case OWcIEprop_Btn_SoundDelete:
			entry = ONrImpactEffect_GetEntry(properties->entry_index);
			sound = ONrImpactEffect_GetSound(properties->entry_index);
			if (sound == NULL) {
				break;
			}

			sound->impulse_ptr = NULL;
			sound->sound_name[0] = '\0';

			if (OWiIEprop_EmptySound(sound)) {
				// delete this now-unused sound entry
				error = ONrImpactEffect_DeleteSound(UUcFalse, entry->sound_index);
				if (error != UUcError_None) {
					WMrDialog_MessageBox(inDialog, "Internal Error", "Cannot delete sound!", WMcMessageBoxStyle_OK);
					break;
				}
			}

			OWiIEprop_DisplaySound(inDialog);
		break;

		case OWcIEprop_Btn_ParticleNew:
			error = ONrImpactEffect_AddParticleToEntry(properties->entry_index, &particle);
			if (error != UUcError_None) {
				WMrDialog_MessageBox(inDialog, "Internal Error", "Cannot add new particle!", WMcMessageBoxStyle_OK);
				break;
			}

			// we could set up proper defaults here if desired
			UUrMemory_Clear(&particle->effect_spec, sizeof(P3tEffectSpecification));
			UUrString_Copy(particle->particle_class_name, "new_particle", P3cParticleClassNameLength + 1);
			
			// select the new particle
			ONrImpactEffect_GetParticles(properties->entry_index, &num_particles);
			properties->selected_particle = num_particles - 1;
			OWiIEprop_DisplayParticles(inDialog);
		break;

		case OWcIEprop_Btn_ParticleDelete:
			if (properties->selected_particle == (UUtUns32) -1) {
				break;
			}

			error = ONrImpactEffect_RemoveParticleFromEntry(UUcFalse, properties->entry_index, properties->selected_particle);
			if (error != UUcError_None) {
				WMrDialog_MessageBox(inDialog, "Internal Error", "Cannot delete particle!", WMcMessageBoxStyle_OK);
				break;
			}

			// no particle is now selected
			properties->selected_particle = (UUtUns32) -1;
			OWiIEprop_DisplayParticles(inDialog);
		break;

		case OWcIEprop_EF_ParticleClass:
			if (properties->selected_particle == (UUtUns32) -1) {
				break;
			}
		
			WMrEditField_GetText((WMtEditField *) inControl, new_classname, P3cParticleClassNameLength + 1);

			particle = ONrImpactEffect_GetParticles(properties->entry_index, &num_particles);
			UUmAssert((properties->selected_particle >= 0) && (properties->selected_particle < num_particles));
			sel_particle = particle + properties->selected_particle;

			if (strcmp(sel_particle->particle_class_name, new_classname) != 0) {
				UUrString_Copy(sel_particle->particle_class_name, new_classname, P3cParticleClassNameLength + 1);

				error = OWrImpactEffect_MakeDirty();
				if (error != UUcError_None) {
					WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
					break;
				}

				OWrImpactEffect_UpdateParticleClassPtr(properties->entry_index, properties->selected_particle);
				OWiIEprop_DisplayParticles(inDialog);
			}
		break;

		case OWcIEprop_EF_Param0:
		case OWcIEprop_EF_Param1:
		case OWcIEprop_EF_Param2:
		case OWcIEprop_EF_Param3:
		case OWcIEprop_EF_Param4:
			if (properties->selected_particle == (UUtUns32) -1) {
				break;
			}
		
			particle = ONrImpactEffect_GetParticles(properties->entry_index, &num_particles);
			UUmAssert((properties->selected_particle >= 0) && (properties->selected_particle < num_particles));
			sel_particle = particle + properties->selected_particle;

			field_index = UUmLowWord(inParam1) - OWcIEprop_EF_Param0;
			switch (sel_particle->effect_spec.location_type) {
				case P3cEffectLocationType_Default:
					// no parameters should be enabled - don't change anything!
				break;

				case P3cEffectLocationType_CollisionOffset:
					if (field_index == 0) {
						// collision offset
						sel_particle->effect_spec.location_data.collisionoffset.offset =
							WMrEditField_GetFloat((WMtEditField *) inControl);
					}
				break;

				case P3cEffectLocationType_FindWall:
					if (field_index == 0) {
						// find-wall distance
						sel_particle->effect_spec.location_data.findwall.radius =
							WMrEditField_GetFloat((WMtEditField *) inControl);
					}
				break;
				
				default:
					UUmAssert(!"OWiIEprop_HandleCommand: selected particle has unknown location type");
				break;
			}

			error = OWrImpactEffect_MakeDirty();
			if (error != UUcError_None) {
				WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
				break;
			}

		break;

		case OWcIEprop_CB_Check0:
		case OWcIEprop_CB_Check1:
		case OWcIEprop_CB_Check2:
		case OWcIEprop_CB_Check3:
		case OWcIEprop_CB_Check4:
			if (properties->selected_particle == (UUtUns32) -1) {
				break;
			}
		
			particle = ONrImpactEffect_GetParticles(properties->entry_index, &num_particles);
			UUmAssert((properties->selected_particle >= 0) && (properties->selected_particle < num_particles));
			sel_particle = particle + properties->selected_particle;

			field_index = UUmLowWord(inParam1) - OWcIEprop_CB_Check0;
			switch (sel_particle->effect_spec.location_type) {
				case P3cEffectLocationType_Default:
					// no checkboxes should be enabled - don't change anything!
				break;

				case P3cEffectLocationType_CollisionOffset:
					// no checkboxes should be enabled - don't change anything!
				break;

				case P3cEffectLocationType_FindWall:
					if (field_index == 0) {
						// look along Y
						sel_particle->effect_spec.location_data.findwall.look_along_orientation =
							WMrCheckBox_GetCheck((WMtCheckBox *) inControl);
					}
				break;
				
				default:
					UUmAssert(!"OWiIEprop_HandleCommand: selected particle has unknown location type");
				break;
			}

			error = OWrImpactEffect_MakeDirty();
			if (error != UUcError_None) {
				WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
				break;
			}

		break;

		case OWcIEprop_CB_AISound:
			sound = ONrImpactEffect_GetSound(properties->entry_index);

			checked = WMrCheckBox_GetCheck((WMtCheckBox *) inControl);
			if (checked) {
				if (sound == NULL) {
					error = OWiIEprop_NewSound(properties, &sound);
					if (error != UUcError_None) {
						WMrDialog_MessageBox(inDialog, "Internal Error", "Cannot add sound to entry!", WMcMessageBoxStyle_OK);
						break;
					}
				}

				sound->flags |= ONcIESoundFlag_AICanHear;

				error = OWrImpactEffect_MakeDirty();
				if (error != UUcError_None) {
					WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
					break;
				}

				OWiIEprop_DisplaySound(inDialog);
			} else {
				if (sound != NULL) {
					sound->flags &= ~ONcIESoundFlag_AICanHear;

					if (OWiIEprop_EmptySound(sound)) {
						// delete this now-unused sound entry
						entry = ONrImpactEffect_GetEntry(properties->entry_index);
						error = ONrImpactEffect_DeleteSound(UUcFalse, entry->sound_index);
						if (error != UUcError_None) {
							WMrDialog_MessageBox(inDialog, "Internal Error", "Cannot delete sound!", WMcMessageBoxStyle_OK);
							break;
						}
					}

					error = OWrImpactEffect_MakeDirty();
					if (error != UUcError_None) {
						WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
						break;
					}

					OWiIEprop_DisplaySound(inDialog);
				}
			}
		break;
	
		case OWcIEprop_EF_AISoundRadius:
			sound = ONrImpactEffect_GetSound(properties->entry_index);

			if (sound != NULL) {
				sound->ai_soundradius = WMrEditField_GetFloat((WMtEditField *) inControl);

				error = OWrImpactEffect_MakeDirty();
				if (error != UUcError_None) {
					WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
					break;
				}
			}
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiIEprop_HandleMenuCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inPopupMenu,
	UUtUns16					inItemID)
{
	OWtIEprop					*properties;
	ONtImpactEntry				*entry;
	ONtIEParticle				*particle, *sel_particle;
	ONtIESound					*sound;
	UUtUns32					num_particles;
	UUtError					error;
	
	properties = (OWtIEprop*)WMrDialog_GetUserData(inDialog);
	
	switch (WMrWindow_GetID(inPopupMenu))
	{
		case OWcIEprop_PM_ModType:
			entry = ONrImpactEffect_GetEntry(properties->entry_index);
			if (entry->modifier != inItemID) {
				entry->modifier = inItemID;

				error = OWrImpactEffect_MakeDirty();
				if (error != UUcError_None) {
					WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
					break;
				}
			}
		break;

		case OWcIEprop_PM_Component:
			entry = ONrImpactEffect_GetEntry(properties->entry_index);
			if (entry->component != inItemID) {
				entry->component = inItemID;

				error = OWrImpactEffect_MakeDirty();
				if (error != UUcError_None) {
					WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
					break;
				}
			}
		break;

		case OWcIEprop_PM_ParticleOrientation:
			if (properties->selected_particle == (UUtUns32) -1) {
				break;
			}
		
			particle = ONrImpactEffect_GetParticles(properties->entry_index, &num_particles);
			UUmAssert((properties->selected_particle >= 0) && (properties->selected_particle < num_particles));
			sel_particle = particle + properties->selected_particle;

			if (sel_particle->effect_spec.collision_orientation != (P3tEnumCollisionOrientation) inItemID) {
				sel_particle->effect_spec.collision_orientation = (P3tEnumCollisionOrientation) inItemID;

				error = OWrImpactEffect_MakeDirty();
				if (error != UUcError_None) {
					WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
					break;
				}
			}
		break;

		case OWcIEprop_PM_ParticleLocation:
			if (properties->selected_particle == (UUtUns32) -1) {
				break;
			}
		
			particle = ONrImpactEffect_GetParticles(properties->entry_index, &num_particles);
			UUmAssert((properties->selected_particle >= 0) && (properties->selected_particle < num_particles));
			sel_particle = particle + properties->selected_particle;

			if (sel_particle->effect_spec.location_type != (P3tEffectLocationType) inItemID) {
				sel_particle->effect_spec.location_type = (P3tEffectLocationType) inItemID;

				OWiIEprop_SetupParticleControls(inDialog);

				error = OWrImpactEffect_MakeDirty();
				if (error != UUcError_None) {
					WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
					break;
				}
			}

		break;

		case OWcIEprop_PM_AISoundType:
			sound = ONrImpactEffect_GetSound(properties->entry_index);

			if (sound != NULL) {
				sound->ai_soundtype = inItemID;

				error = OWrImpactEffect_MakeDirty();
				if (error != UUcError_None) {
					WMrDialog_MessageBox(inDialog, "Internal Error", "Unable to set dirty flag on impact effects!", WMcMessageBoxStyle_OK);
					break;
				}
			}
		break;	
	}
}

// ----------------------------------------------------------------------
static UUtBool
OWiIEprop_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiIEprop_InitDialog(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiIEprop_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;
		
		case WMcMessage_MenuCommand:
			OWiIEprop_HandleMenuCommand(inDialog, (WMtWindow*)inParam2, UUmLowWord(inParam1));
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	return handled;
}

// ======================================================================
#if 0
#pragma mark -
#endif
// ======================================================================
// ----------------------------------------------------------------------
void
OWrIE_FillImpactListBox(
	WMtDialog					*inDialog,
	WMtListBox					*inListBox)
{
	UUtUns32					indent;
	UUtUns32					num_types;
	UUtUns16					i;
	const char *				item_name;
	char						tempstring[64];
	
	WMrMessage_Send(inListBox, LBcMessage_Reset, 0, 0);
	num_types = MArImpacts_GetNumTypes();

	for (i = 0; i < num_types; i++)
	{
		item_name	= MArImpactType_GetName(i);
		indent = OWcIEListBox_IndentDepth * MArImpactType_GetDepth(i);

		if (indent > 0) {
			UUrMemory_Set8(tempstring, ' ', indent);
			UUrString_Copy(tempstring + indent, item_name, sizeof(tempstring) - indent);
			WMrMessage_Send(inListBox, LBcMessage_AddString, (UUtUns32) tempstring, 0);
		} else {
			WMrMessage_Send(inListBox, LBcMessage_AddString, (UUtUns32) item_name, 0);
		}
	}
}

// ----------------------------------------------------------------------
void
OWrIE_FillMaterialListBox(
	WMtDialog					*inDialog,
	WMtListBox					*inListBox)
{
	UUtUns32					indent;
	UUtUns32					num_types;
	UUtUns16					i;
	const char *				item_name;
	char						tempstring[64];
	
	WMrMessage_Send(inListBox, LBcMessage_Reset, 0, 0);
	num_types = MArMaterials_GetNumTypes();

	for (i = 0; i < num_types; i++)
	{
		item_name	= MArMaterialType_GetName(i);
		indent = OWcIEListBox_IndentDepth * MArMaterialType_GetDepth(i);

		if (indent > 0) {
			UUrMemory_Set8(tempstring, ' ', indent);
			UUrString_Copy(tempstring + indent, item_name, sizeof(tempstring) - indent);
			WMrMessage_Send(inListBox, LBcMessage_AddString, (UUtUns32) tempstring, 0);
		} else {
			WMrMessage_Send(inListBox, LBcMessage_AddString, (UUtUns32) item_name, 0);
		}
	}
}

// ----------------------------------------------------------------------
static void
OWiIE_FillEntryListBoxes(
	WMtDialog					*inDialog)
{
	UUtBool						belong;
	UUtUns16					num_entries;
	UUtUns32					base_index, i, index;
	OWtIEdata					*data;
	ONtImpactEntry				*entry;
	WMtWindow					*text;
	WMtListBox					*listbox, *button;
	char						textbuf[256];
	MAtMaterialType				material_type, parent_material_type;
	MAtMaterialType				new_material_type;
	
	data = (OWtIEdata*)WMrDialog_GetUserData(inDialog);
	UUmAssert(data);
	
	// fill the attached entry list
	listbox = WMrDialog_GetItemByID(inDialog, OWcIE_LB_AttachedEntries);
	WMrMessage_Send(listbox, LBcMessage_Reset, 0, 0);

	material_type = data->cur_material_type;
	data->num_entries = 0;
	data->entry_baseindex = 0;
	do {
		ONrImpactEffect_GetImpactEntries(data->cur_impact_type, material_type, &new_material_type, &num_entries, &base_index);
		if (new_material_type == MAcInvalidID) {
			// no more entries, stop
			break;
		}

		if (new_material_type == data->cur_material_type) {
			// these entries belong to the currently viewed node
			data->num_entries = num_entries;
			data->entry_baseindex = base_index;
			belong = UUcTrue;
		} else {
			belong = UUcFalse;

			// is the found material type one of our children?
			parent_material_type = new_material_type;
			while ((parent_material_type != data->cur_material_type) && (parent_material_type != MAcInvalidID)) {
				parent_material_type = MArMaterialType_GetParent(parent_material_type);
			}
			if (parent_material_type == MAcInvalidID) {
				// this material type is not a child of the current material type, break out
				break;
			}
		}

		// add the entries found for this material type
		for (i = 0; i < num_entries; i++) {
			index = base_index + i;
			entry = ONrImpactEffect_GetEntry(index);
			if ((entry->modifier != data->cur_mod_type) && (entry->modifier != ONcIEModType_Any) &&
				(data->cur_mod_type != ONcIEModType_Any)) {
				// we don't want to see this entry, it is for a different modifier type
				continue;
			}

			if (!belong) {
				index |= OWcEntry_DoesNotBelong;
			}

			WMrMessage_Send(listbox, LBcMessage_AddString, index, 0);
		}
		
		material_type = new_material_type + 1;
	} while (material_type < MArMaterials_GetNumTypes());
	
	// perform an impact lookup on the currently viewed node
	ONrImpactEffect_Lookup(data->cur_impact_type, data->cur_material_type, data->cur_mod_type, data->found_entries);

	// set the prompt for the lookup
	text = WMrDialog_GetItemByID(inDialog, OWcIE_Txt_Lookup);
	sprintf(textbuf, "Effects found for Impact %s on Material %s with Modifier:",
		MArImpactType_GetName(data->cur_impact_type), MArMaterialType_GetName(data->cur_material_type));
	WMrWindow_SetTitle(text, textbuf, WMcMaxTitleLength);

	// fill the found entry list
	listbox = WMrDialog_GetItemByID(inDialog, OWcIE_LB_FoundEntries);
	WMrMessage_Send(listbox, LBcMessage_Reset, 0, 0);
	for (i = 0; i < ONcIEComponent_Max; i++) {
		if (data->found_entries[i] == (UUtUns32) -1) {
			continue;
		}

		WMrMessage_Send(listbox, LBcMessage_AddString, data->found_entries[i], 0);
	}

	// disable delete and copy buttons, nothing is selected
	button = WMrDialog_GetItemByID(inDialog, OWcIE_Btn_Delete);
	WMrWindow_SetEnabled(button, UUcFalse);
	button = WMrDialog_GetItemByID(inDialog, OWcIE_Btn_Copy);
	WMrWindow_SetEnabled(button, UUcFalse);
}

// ----------------------------------------------------------------------
static void
OWiIE_HandleDelete(
	WMtDialog					*inDialog)
{
	WMtWindow					*listbox;
	UUtUns32					index, selected, result;
	UUtError					error;
	char						*errmsg;

	if (ONgImpactEffect_Locked) {
		WMrDialog_MessageBox(inDialog, "File Is Locked",
							"Sorry, you cannot delete this entry because the impact effects .bin file is locked.",
							WMcMessageBoxStyle_OK);
		return;
	}

	listbox = WMrDialog_GetItemByID(inDialog, OWcIE_LB_AttachedEntries);

	result = 
		WMrDialog_MessageBox(
			inDialog,
			"Confirm Delete",
			"Are you sure you want to delete this impact effect?",
			WMcMessageBoxStyle_Yes_No);
	if (result == WMcDialogItem_No) {
		return;
	}
	
	selected = WMrMessage_Send(listbox, LBcMessage_GetSelection, 0, 0);
	if (selected == (UUtUns32) -1) {
		// nothing to delete!
		return;
	}

	index = WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
	UUmAssert((index & OWcEntry_DoesNotBelong) == 0);

	error = ONrImpactEffect_DeleteEntry(UUcFalse, index);
	if (error == UUcError_OutOfMemory) {
		errmsg = "Could not delete entry; out of memory!";
	} else {
		errmsg = "Internal error deleting entry; see debugger.txt";
	}

	if (error != UUcError_None) {
		WMrDialog_MessageBox(inDialog, "Error", errmsg, WMcMessageBoxStyle_OK);
		return;
	}

	// re-fill the listboxes
	OWiIE_FillEntryListBoxes(inDialog);
}

// ----------------------------------------------------------------------
static UUtError
OWiIE_HandleEdit(
	WMtDialog					*inDialog)
{
	ONtImpactEntry				*entry;
	OWtIEdata					*data;
	WMtWindow					*listbox;
	UUtUns32					index;
	OWtIEprop					properties;
	UUtError					error;
	UUtUns32					message;
	UUtBool						doesnt_belong;
	
	data = (OWtIEdata*)WMrDialog_GetUserData(inDialog);
	UUmAssert(data);

	listbox = WMrDialog_GetItemByID(inDialog, OWcIE_LB_AttachedEntries);
	index = WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
	
	if (index == (UUtUns32) -1) {
		// nothing selected
		return UUcError_None;
	}

	doesnt_belong = ((index & OWcEntry_DoesNotBelong) > 0);
	index &= ~OWcEntry_DoesNotBelong;

	if (doesnt_belong) {
		// take the dialog to the correct material type
		entry = ONrImpactEffect_GetEntry(index);
		listbox = WMrDialog_GetItemByID(inDialog, OWcIE_LB_Material);
		WMrListBox_SetSelection(listbox, UUcTrue, entry->material_type);
	} else {
		properties.entry_index = index;
			
		// edit the properties
		error = 
			WMrDialog_ModalBegin(
				OWcDialog_Impact_Effect_Prop,
				inDialog,
				OWiIEprop_Callback,
				(UUtUns32)&properties,
				&message);
		UUmError_ReturnOnError(error);
		
		// update the listboxes
		OWiIE_FillEntryListBoxes(inDialog);
	}

	return UUcError_None;
}

// ----------------------------------------------------------------------
static UUtError
OWiIE_HandleNew(
	WMtDialog					*inDialog)
{
	OWtIEdata					*data;
	UUtError					error;
	UUtUns32					entry_index;
	char *						errmsg;

	data = (OWtIEdata *) WMrDialog_GetUserData(inDialog);
	UUmAssert(data);

	if (ONgImpactEffect_Locked) {
		WMrDialog_MessageBox(inDialog, "File Is Locked",
							"Sorry, you cannot add a new entry because the impact effects .bin file is locked.",
							WMcMessageBoxStyle_OK);
		return UUcError_None;
	}
	
	error = ONrImpactEffect_CreateImpactEntry(data->cur_impact_type, data->cur_material_type, &entry_index);
	if (error == UUcError_OutOfMemory) {
		errmsg = "Could not add new entry; out of memory!";
	} else {
		errmsg = "Internal error creating entry; see debugger.txt";
	}

	if (error != UUcError_None) {
		WMrDialog_MessageBox(inDialog, "Error", errmsg, WMcMessageBoxStyle_OK);
		return error;
	}

	// re-fill the listboxes
	OWiIE_FillEntryListBoxes(inDialog);
	
	return UUcError_None;
}

// ----------------------------------------------------------------------
static void
OWiIE_InitDialog(
	WMtDialog					*inDialog)
{
	WMtWindow					*listbox, *button;
	WMtPopupMenu				*modifier_popup;
	OWtIEdata					*data;

	// allocate memory
	data = (OWtIEdata *) UUrMemory_Block_NewClear(sizeof(OWtIEdata));
	if (data == NULL) {
		WMrDialog_ModalEnd(inDialog, 0);
		return;
	}

	WMrDialog_SetUserData(inDialog, (UUtUns32) data);

	// can't paste until we copy something
	button = WMrDialog_GetItemByID(inDialog, OWcIE_Btn_Paste);
	WMrWindow_SetEnabled(button, UUcFalse);

	// set up the impact and material listboxes, and select the base types
	listbox = WMrDialog_GetItemByID(inDialog, OWcIE_LB_Impact);
	OWrIE_FillImpactListBox(inDialog, listbox);
	WMrListBox_SetSelection(listbox, UUcFalse, MAcImpact_Base);
	
	listbox = WMrDialog_GetItemByID(inDialog, OWcIE_LB_Material);
	OWrIE_FillMaterialListBox(inDialog, listbox);
	WMrListBox_SetSelection(listbox, UUcFalse, MAcMaterial_Base);

	modifier_popup = (WMtPopupMenu *) WMrDialog_GetItemByID(inDialog, OWcIE_PM_Modifier);
	WMrPopupMenu_SetSelection(modifier_popup, (UUtUns16) ONcIEModType_Any);

	// fill the entry listboxes
	OWiIE_FillEntryListBoxes(inDialog);

	// start a timer with a 1 minute frequency
	WMrTimer_Start(0, (60 * 60), inDialog);
}

// ----------------------------------------------------------------------
static void
OWiIE_HandleCopy(
	WMtDialog					*inDialog)
{
	OWtIEdata					*data;
	WMtWindow					*listbox, *button;
	UUtUns32					index;
	UUtError					error;
	char						*errmsg;

	data = (OWtIEdata *) WMrDialog_GetUserData(inDialog);
	UUmAssert(data);

	listbox = WMrDialog_GetItemByID(inDialog, OWcIE_LB_AttachedEntries);
	index = WMrMessage_Send(listbox, LBcMessage_GetItemData, 0, (UUtUns32)-1);
	
	if (index == (UUtUns32) -1) {
		// nothing selected
		return;
	}

	error = ONrImpactEffect_Copy(index);
	if (error == UUcError_OutOfMemory) {
		errmsg = "Could not copy entry; out of memory!";
	} else {
		errmsg = "Internal error copying entry; see debugger.txt";
	}

	if (error != UUcError_None) {
		WMrDialog_MessageBox(inDialog, "Error", errmsg, WMcMessageBoxStyle_OK);
		return;
	}

	data->clipboard_full = UUcTrue;
	button = WMrDialog_GetItemByID(inDialog, OWcIE_Btn_Paste);
	WMrWindow_SetEnabled(button, UUcTrue);
}

// ----------------------------------------------------------------------
static void
OWiIE_HandlePaste(
	WMtDialog					*inDialog)
{
	OWtIEdata					*data;
	UUtUns32					index;
	UUtError					error;
	char						*errmsg = "<internal error>";

	if (ONgImpactEffect_Locked) {
		WMrDialog_MessageBox(inDialog, "File Is Locked",
							"Sorry, you cannot paste this entry because the impact effects .bin file is locked.",
							WMcMessageBoxStyle_OK);
		return;
	}

	data = (OWtIEdata *) WMrDialog_GetUserData(inDialog);
	UUmAssert(data);

	if (!data->clipboard_full)
		return;

	error = ONrImpactEffect_Paste(data->cur_impact_type, data->cur_material_type, &index);
	if (error != UUcError_None) {
		if (error == UUcError_OutOfMemory) {
			errmsg = "Could not paste entry; out of memory!";
		} else {
			errmsg = "Internal error pasting entry; see debugger.txt";
		}

		WMrDialog_MessageBox(inDialog, "Error", errmsg, WMcMessageBoxStyle_OK);
		return;
	}
	
	// fill the entry listboxes
	OWiIE_FillEntryListBoxes(inDialog);	
}

// ----------------------------------------------------------------------
static void
OWiIE_Destroy(
	WMtDialog					*inDialog)
{
	OWtIEdata					*data;
	
	data = (OWtIEdata *) WMrDialog_GetUserData(inDialog);
	if (data != NULL) {
		UUrMemory_Block_Delete(data);
	}
	
	// stop the timer and save the data
	WMrTimer_Stop(0, inDialog);
	ONrImpactEffects_Save(UUcFalse);
}

// ----------------------------------------------------------------------
static void
OWiIE_HandleMenuCommand(
	WMtDialog					*inDialog,
	WMtWindow					*inPopupMenu,
	UUtUns16					inItemID)
{
	OWtIEdata					*data;

	data = (OWtIEdata *) WMrDialog_GetUserData(inDialog);
	UUmAssert(data);

	switch (WMrWindow_GetID(inPopupMenu))
	{
		case OWcIE_PM_Modifier:
			data->cur_mod_type = inItemID;
			OWiIE_FillEntryListBoxes(inDialog);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiIE_HandleCommand(
	WMtDialog					*inDialog,
	UUtUns32					inParam1,
	WMtWindow					*inControl)
{
	OWtIEdata					*data;
	WMtListBox					*button;
	UUtUns32					selected_item;

	data = (OWtIEdata *) WMrDialog_GetUserData(inDialog);
	UUmAssert(data);

	switch (UUmLowWord(inParam1))
	{
		case WMcDialogItem_Cancel:
			WMrDialog_ModalEnd(inDialog, 0);
		break;
		
		case OWcIE_LB_Impact:
			data->cur_impact_type = (MAtImpactType) WMrListBox_GetSelection((WMtListBox *) inControl);
			OWiIE_FillEntryListBoxes(inDialog);
		break;

		case OWcIE_LB_Material:
			data->cur_material_type = (MAtMaterialType) WMrListBox_GetSelection((WMtListBox *) inControl);
			OWiIE_FillEntryListBoxes(inDialog);
		break;

		case OWcIE_LB_AttachedEntries:
			if (UUmHighWord(inParam1) == WMcNotify_DoubleClick) {
				OWiIE_HandleEdit(inDialog);
			}

			selected_item = WMrMessage_Send(inControl, LBcMessage_GetSelection, 0, 0);
			
			// disable delete and copy buttons if nothing is selected
			button = WMrDialog_GetItemByID(inDialog, OWcIE_Btn_Delete);
			WMrWindow_SetEnabled(button, (selected_item != (UUtUns32) -1));
			button = WMrDialog_GetItemByID(inDialog, OWcIE_Btn_Copy);
			WMrWindow_SetEnabled(button, (selected_item != (UUtUns32) -1));
		break;
		
		case OWcIE_LB_FoundEntries:
			WMrMessage_Send(inControl, LBcMessage_SetSelection, UUcFalse, (UUtUns32) -1);
		break;

		case OWcIE_Btn_Delete:
			OWiIE_HandleDelete(inDialog);
		break;
		
		case OWcIE_Btn_Copy:
			OWiIE_HandleCopy(inDialog);
		break;
		
		case OWcIE_Btn_Paste:
			OWiIE_HandlePaste(inDialog);
		break;
		
		case OWcIE_Btn_Edit:
			OWiIE_HandleEdit(inDialog);
		break;
		
		case OWcIE_Btn_New:
			OWiIE_HandleNew(inDialog);
		break;
	}
}

// ----------------------------------------------------------------------
static void
OWiIE_HandleDrawItem(
	WMtDialog					*inDialog,
	WMtDrawItem					*inDrawItem)
{
	IMtPoint2D					dest;
	PStPartSpecUI				*partspec_ui;
	UUtInt16					line_width;
	UUtInt16					line_height;
	UUtUns32					index;
	UUtBool						doesnt_belong;
	ONtImpactEntry				*entry;
	const char					*name;
	char						description[512];

	doesnt_belong = ((inDrawItem->data & OWcEntry_DoesNotBelong) > 0);
	index = inDrawItem->data & ~OWcEntry_DoesNotBelong;

	// get a pointer to the partspec_ui
	partspec_ui = PSrPartSpecUI_GetActive();
	UUmAssert(partspec_ui);
	
	// set the dest
	dest.x = inDrawItem->rect.left;
	dest.y = inDrawItem->rect.top;
	
	// calc the width and height of the line
	line_width = inDrawItem->rect.right - inDrawItem->rect.left;
	line_height = inDrawItem->rect.bottom - inDrawItem->rect.top;
	
	if (inDrawItem->rect.top > 5)
	{
		// draw a divider
		DCrDraw_PartSpec(
			inDrawItem->draw_context,
			partspec_ui->divider,
			PScPart_All,
			&dest,
			line_width,
			2,
			M3cMaxAlpha);
	}
	
	// draw the Text
	DCrText_SetFormat(TSc_HLeft | TSc_VCenter);
	DCrText_SetStyle(TScStyle_Plain);
	DCrText_SetShade(doesnt_belong ? IMcShade_Gray50 : IMcShade_Black);
	
	dest.x = 5;
	dest.y += 1;

	// get the impact entry
	entry = ONrImpactEffect_GetEntry(index);
	
	// draw the impact type name
	name = MArImpactType_GetName(entry->impact_type);
	DCrDraw_String(inDrawItem->draw_context, name, &inDrawItem->rect, &dest);
	dest.x += 100;
		
	// draw the material type name
	name = MArMaterialType_GetName(entry->material_type);
	DCrDraw_String(inDrawItem->draw_context, name, &inDrawItem->rect, &dest);
	dest.x += 100;

	// draw the component name
	name = ONrIEComponent_GetName(entry->component);
	DCrDraw_String(inDrawItem->draw_context, name, &inDrawItem->rect, &dest);
	dest.x += 60;
	
	// draw the modifier name
	name = ONrIEModType_GetName(entry->modifier);
	DCrDraw_String(inDrawItem->draw_context, name, &inDrawItem->rect, &dest);	
	dest.x += 60;
	
	// get a description of the entry
	ONrImpactEffect_GetDescription(index, description);
	DCrDraw_String(inDrawItem->draw_context, description, &inDrawItem->rect, &dest);	
}

// ----------------------------------------------------------------------
static UUtBool
OWiIE_Callback(
	WMtDialog					*inDialog,
	WMtMessage					inMessage,
	UUtUns32					inParam1,
	UUtUns32					inParam2)
{
	UUtBool						handled;
	
	handled = UUcTrue;
	
	UUrMemory_Block_VerifyList();
	ONrImpactEffect_VerifyStructure(UUcTrue);

	switch (inMessage)
	{
		case WMcMessage_InitDialog:
			OWiIE_InitDialog(inDialog);
		break;
		
		case WMcMessage_Destroy:
			OWiIE_Destroy(inDialog);
		break;
		
		case WMcMessage_Command:
			OWiIE_HandleCommand(inDialog, inParam1, (WMtWindow*)inParam2);
		break;
		
		case WMcMessage_MenuCommand:
			OWiIE_HandleMenuCommand(inDialog, (WMtWindow*)inParam2, UUmLowWord(inParam1));
		break;
		
		case WMcMessage_Timer:
			ONrImpactEffects_Save(UUcTrue);
		break;
		
		case WMcMessage_DrawItem:
			OWiIE_HandleDrawItem(inDialog, (WMtDrawItem*)inParam2);
		break;
		
		default:
			handled = UUcFalse;
		break;
	}
	
	UUrMemory_Block_VerifyList();
	ONrImpactEffect_VerifyStructure(UUcTrue);

	return handled;
}

// ----------------------------------------------------------------------
UUtError
OWrImpactEffect_Display(
	void)
{
	UUtError					error;
	
	error =
		WMrDialog_ModalBegin(
			OWcDialog_Impact_Effect,
			NULL,
			OWiIE_Callback,
			0,
			NULL);
	UUmError_ReturnOnError(error);
	
	return UUcError_None;
}
#endif