project(Oni)

set(BFW_SOURCE
        #Bungie Framework
        ../../../BungieFrameWork/BFW_Headers/BFW.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Akira.h
        ../../../BungieFrameWork/BFW_Headers/BFW_AppUtilities.h
        ../../../BungieFrameWork/BFW_Headers/BFW_BinaryData.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Bink.h
        ../../../BungieFrameWork/BFW_Headers/BFW_BitVector.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Collision.h
        ../../../BungieFrameWork/BFW_Headers/BFW_CommandLine.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Console.h
        ../../../BungieFrameWork/BFW_Headers/BFW_DebuggerSymbols.h
        ../../../BungieFrameWork/BFW_Headers/BFW_DialogManager.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Doors.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Effect.h
        ../../../BungieFrameWork/BFW_Headers/BFW_EnvParticle.h
        ../../../BungieFrameWork/BFW_Headers/BFW_ErrorCodes.h
        ../../../BungieFrameWork/BFW_Headers/BFW_FileFormat.h
        ../../../BungieFrameWork/BFW_Headers/BFW_FileManager.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Group.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Image.h
        ../../../BungieFrameWork/BFW_Headers/BFW_LocalInput.h
        ../../../BungieFrameWork/BFW_Headers/BFW_MasterHeader.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Materials.h
        ../../../BungieFrameWork/BFW_Headers/BFW_MathLib.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Motoko.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Noise.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Object.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Particle3.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Path.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Physics.h
        ../../../BungieFrameWork/BFW_Headers/BFW_ScriptLang.h
        ../../../BungieFrameWork/BFW_Headers/BFW_SoundSystem2.h
        ../../../BungieFrameWork/BFW_Headers/BFW_TemplateManager.h
        ../../../BungieFrameWork/BFW_Headers/BFW_TextSystem.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Timer.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Totoro.h
        ../../../BungieFrameWork/BFW_Headers/BFW_Types.h
        ../../../BungieFrameWork/BFW_Headers/BFW_WindowManager.h

        ../../../BungieFrameWork/BFW_Source/BFW_FileManager/BFW_FileManager_Common.c
        ../../../BungieFrameWork/BFW_Source/BFW_FileManager/Platform_MacOS/BFW_FileManager_MacOS.c
        ../../../BungieFrameWork/BFW_Source/BFW_FileManager/Platform_Win32/BFW_FileManager_Win32.c

        ../../../BungieFrameWork/BFW_Source/BFW_LocalInput/BFW_LI_Platform.h
        ../../../BungieFrameWork/BFW_Source/BFW_LocalInput/Platform_MacOS/BFW_LI_Platform_MacOS.c
        ../../../BungieFrameWork/BFW_Source/BFW_LocalInput/Platform_MacOS/BFW_LI_Platform_MacOS.h
        ../../../BungieFrameWork/BFW_Source/BFW_LocalInput/Platform_Win32/BFW_LI_Platform_Win32.c
        ../../../BungieFrameWork/BFW_Source/BFW_LocalInput/Platform_Win32/BFW_LI_Platform_Win32.h
        ../../../BungieFrameWork/BFW_Source/BFW_LocalInput/BFW_LI_Private.h
        ../../../BungieFrameWork/BFW_Source/BFW_LocalInput/BFW_LI_Translators.c
        ../../../BungieFrameWork/BFW_Source/BFW_LocalInput/BFW_LocalInput.c
        ../../../BungieFrameWork/BFW_Source/BFW_LocalInput/BFW_LocalInput_Template.c

        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/OpenGL/gl_code_version.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/OpenGL/gl_engine.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/OpenGL/gl_engine.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/OpenGL/gl_geometry_draw_method.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/OpenGL/gl_mswindows.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/OpenGL/gl_utility.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/OpenGL/glext.h

        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/Software/MS_DrawEngine_Method.h

        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Env_Clip.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Env_Clip.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Env.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Env.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Frame.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Frame.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Geometry.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Geometry.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Light.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Light.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Matrix.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Matrix.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Pick.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_Pick.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_State.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Method_State.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GC_Private.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_Geom_Clip.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_Geom_Clip.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_Geom_Shade.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_Geom_Shade.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_Geom_Transform.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_Geom_Transform.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GeomEngine_Method.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/MS_GeomEngine_Method.h

        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_DefaultFunctions.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_DefaultFunctions.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Draw.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Draw.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Geom.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Geom_Camera.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Geom_Matrix.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Manager.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Manager.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Win32/Motoko_Platform_Win32.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Win32/Motoko_Platform_Win32.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Private.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Sort.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Sort.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_State_Draw.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_State_Geom.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Texture.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Utility.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Verify.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Motoko_Verify.h

        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Manager/BFW_Motoko_Template.c

        ../../../BungieFrameWork/BFW_Source/BFW_TemplateManager/BFW_TM_Common.c
        ../../../BungieFrameWork/BFW_Source/BFW_TemplateManager/BFW_TM_Construction.c
        ../../../BungieFrameWork/BFW_Source/BFW_TemplateManager/BFW_TM_Construction.h
        ../../../BungieFrameWork/BFW_Source/BFW_TemplateManager/BFW_TM_Game.c
        ../../../BungieFrameWork/BFW_Source/BFW_TemplateManager/BFW_TM_Private.h

        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_BitVector.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/3DFX_Glide/bfw_cseries.h
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Error.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Error.h
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Memory.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Memory.h
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Noise.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Platform.h
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/Platform_MacOS/BFW_Platform_AltiVec.c
        ../../../BungieFrameWork/BFW_Headers/BFW_Platform_AltiVec.h
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/Platform_MacOS/BFW_Platform_MacOS.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/Platform_Win32/BFW_Platform_Win32.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_String.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Timer.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Utility.c
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/3DFX_Glide/lrar_cache.h
        ../../../BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/3DFX_Glide/lrar_cache2.c

        ../../../BungieFrameWork/BFW_Source/BFW_Totoro/BFW_Totoro.c
        ../../../BungieFrameWork/BFW_Source/BFW_Totoro/BFW_Totoro_Private.h
        ../../../BungieFrameWork/BFW_Source/BFW_Totoro/BFW_Totoro_Template.c

        ../../../BungieFrameWork/BFW_Source/BFW_TextSystem/BFW_TextSystem.c
        ../../../BungieFrameWork/BFW_Source/BFW_TextSystem/BFW_TextSystem_Template.c
        ../../../BungieFrameWork/BFW_Source/BFW_TextSystem/BFW_TS_Private.h

        ../../../BungieFrameWork/BFW_Source/BFW_Console/BFW_Console.c

        ../../../BungieFrameWork/BFW_Source/BFW_Group/BFW_Group.c

        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/bfw_math.c
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/bfw_math.h
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/bfw_math_3dnow.c
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/bfw_math_stdc.c
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/BFW_MathLib.c
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/BFW_MathLib_Matrix.c
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/Decompose.c
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/Decompose.h
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/EulerAngles.c
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/EulerAngles.h

        ../../../BungieFrameWork/BFW_Source/BFW_AppUtilities/BFW_AppUtilities.c

        ../../../BungieFrameWork/BFW_Source/BFW_Object/BFW_Doors.c
        ../../../BungieFrameWork/BFW_Source/BFW_Object/BFW_Object.c
        ../../../BungieFrameWork/BFW_Source/BFW_Object/BFW_Object_Templates.h
        ../../../BungieFrameWork/BFW_Source/BFW_Object/BFW_Physics.c

        ../../../BungieFrameWork/BFW_Source/BFW_Path/BFW_Path.c

        ../../../BungieFrameWork/BFW_Source/BFW_Collision/BFW_Collision.c

        ../../../BungieFrameWork/BFW_Source/BFW_CommandLine/Platform_Win32/BFW_CL_Platform_Win32.c
        ../../../BungieFrameWork/BFW_Source/BFW_CommandLine/Platform_Win32/BFW_CL_Platform_Win32.h

        ../../../BungieFrameWork/BFW_Source/BFW_CommandLine/BFW_CommandLine.c

        ../../../BungieFrameWork/BFW_Source/BFW_Akira/Akira_Private.h
        ../../../BungieFrameWork/BFW_Source/BFW_Akira/BFW_Akira.c
        ../../../BungieFrameWork/BFW_Source/BFW_Akira/BFW_Akira_Collision.c
        ../../../BungieFrameWork/BFW_Source/BFW_Akira/BFW_Akira_LightMap.c
        ../../../BungieFrameWork/BFW_Source/BFW_Akira/BFW_Akira_Render.c
        ../../../BungieFrameWork/BFW_Source/BFW_Akira/BFW_Akira_Template.c

        ../../../BungieFrameWork/BFW_Source/BFW_FileFormat/BMP/BFW_FF_BMP.c
        ../../../BungieFrameWork/BFW_Source/BFW_FileFormat/BMP/BFW_FF_BMP.h
        ../../../BungieFrameWork/BFW_Source/BFW_FileFormat/BMP/BFW_FF_BMP_Priv.h

        ../../../BungieFrameWork/BFW_Source/BFW_FileFormat/PSD/BFW_FF_PSD.c
        ../../../BungieFrameWork/BFW_Source/BFW_FileFormat/PSD/BFW_FF_PSD.h
        ../../../BungieFrameWork/BFW_Source/BFW_FileFormat/PSD/BFW_FF_PSD_Priv.h

        ../../../BungieFrameWork/BFW_Source/BFW_FileFormat/DDS/BFW_FF_DDS.c
        ../../../BungieFrameWork/BFW_Source/BFW_FileFormat/DDS/BFW_FF_DDS.h
        ../../../BungieFrameWork/BFW_Source/BFW_FileFormat/DDS/BFW_FF_DDS_Priv.h

        ../../../BungieFrameWork/BFW_Source/BFW_FileFormat/BFW_FileFormat.c

        ../../../BungieFrameWork/BFW_Source/BFW_Image/BFW_Image.c
        ../../../BungieFrameWork/BFW_Source/BFW_Image/BFW_Image_Dither.c
        ../../../BungieFrameWork/BFW_Source/BFW_Image/BFW_Image_Draw.c
        ../../../BungieFrameWork/BFW_Source/BFW_Image/BFW_Image_PixelConversion.c
        ../../../BungieFrameWork/BFW_Source/BFW_Image/BFW_Image_Private.h
        ../../../BungieFrameWork/BFW_Source/BFW_Image/BFW_Image_Scale_Box.c

        ../../../BungieFrameWork/BFW_Source/BFW_Particle/BFW_Decal.c
        ../../../BungieFrameWork/BFW_Source/BFW_Particle/BFW_EnvParticle.c
        ../../../BungieFrameWork/BFW_Source/BFW_Particle/BFW_Particle3.c

        ../../../BungieFrameWork/BFW_Source/BFW_DebuggerSymbols/BFW_DebuggerSymbols_Win32.c
        ../../../BungieFrameWork/BFW_Source/BFW_DebuggerSymbols/stack_walk_windows.c
        ../../../BungieFrameWork/BFW_Source/BFW_DebuggerSymbols/stack_walk_windows.h

        ../../../BungieFrameWork/BFW_Source/BFW_Shared/BFW_Shared_Camera.h
        ../../../BungieFrameWork/BFW_Source/BFW_Shared/BFW_Shared_Clip.c
        ../../../BungieFrameWork/BFW_Source/BFW_Shared/BFW_Shared_Clip.h
        ../../../BungieFrameWork/BFW_Source/BFW_Shared/BFW_Shared_ClipCompPoint_c.h
        ../../../BungieFrameWork/BFW_Source/BFW_Shared/BFW_Shared_ClipPoly_c.h
        ../../../BungieFrameWork/BFW_Source/BFW_Shared/BFW_Shared_Math.c
        ../../../BungieFrameWork/BFW_Source/BFW_Shared/BFW_Shared_Math.h
        ../../../BungieFrameWork/BFW_Source/BFW_Shared/BFW_Shared_TriRaster.h
        ../../../BungieFrameWork/BFW_Source/BFW_Shared/BFW_Shared_TriRaster_c.h

        ../../../BungieFrameWork/BFW_Source/BFW_Effect/BFW_Effect.c
        ../../../BungieFrameWork/BFW_Source/BFW_Effect/BFW_Effect_Private.h

        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Cursor/WM_Cursor.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Cursor/WM_Cursor.h

        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Dialog/WM_Dialog.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Dialog/WM_Dialog.h

        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_DrawContext/WM_DrawContext.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_DrawContext/WM_DrawContext.h

        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_PartSpecification/WM_PartSpecification.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_PartSpecification/WM_PartSpecification.h

        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Utilities/WM_Utilities.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Utilities/WM_Utilities.h

        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Box.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Box.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Button.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Button.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_CheckBox.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_CheckBox.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_EditField.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_EditField.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_ListBox.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_ListBox.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Menu.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Menu.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_MenuBar.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_MenuBar.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Picture.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Picture.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_PopupMenu.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_PopupMenu.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_ProgressBar.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_ProgressBar.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_RadioButton.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_RadioButton.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Scrollbar.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Scrollbar.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Slider.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Slider.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Text.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Text.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Window.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/WM_Window.h

        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/BFW_WindowManager.c
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/BFW_WindowManager_Private.h
        ../../../BungieFrameWork/BFW_Source/BFW_WindowManager/BFW_WindowManager_Public.h

        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang.c
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Context.c
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Context.h
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Database.c
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Database.h
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Error.c
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Error.h
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Eval.c
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Eval.h
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Expr.c
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Expr.h
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Parse.c
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Parse.h
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Private.h
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Scheduler.c
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Scheduler.h
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Token.c
        ../../../BungieFrameWork/BFW_Source/BFW_ScriptLang/BFW_ScriptLang_Token.h

        ../../../BungieFrameWork/BFW_Source/BFW_BinaryData/BFW_BinaryData.c
        ../../../BungieFrameWork/BFW_Source/BFW_BinaryData/BFW_BinaryData_Template.c

        ../../../BungieFrameWork/BFW_Source/BFW_SoundSystem2/BFW_SoundSystem2.c
        ../../../BungieFrameWork/BFW_Source/BFW_SoundSystem2/BFW_SS2_IMA.c
        ../../../BungieFrameWork/BFW_Source/BFW_SoundSystem2/BFW_SS2_IMA.h
        ../../../BungieFrameWork/BFW_Source/BFW_SoundSystem2/BFW_SS2_MSADPCM.c
        ../../../BungieFrameWork/BFW_Source/BFW_SoundSystem2/BFW_SS2_MSADPCM.h
        ../../../BungieFrameWork/BFW_Source/BFW_SoundSystem2/BFW_SS2_Platform.h
        ../../../BungieFrameWork/BFW_Source/BFW_SoundSystem2/Platform_MacOS/BFW_SS2_Platform_MacOS.c
        ../../../BungieFrameWork/BFW_Source/BFW_SoundSystem2/Platform_Win32/BFW_SS2_Platform_Win32.c
        ../../../BungieFrameWork/BFW_Source/BFW_SoundSystem2/BFW_SS2_Private.h
        ../../../BungieFrameWork/BFW_Source/BFW_SoundSystem2/BFW_SS2_RegisterTemplate.c

        ../../../BungieFrameWork/BFW_Source/BFW_Materials/BFW_Materials.c

        ../../../BungieFrameWork/BFW_Source/BFW_Bink/BinkHeaders/bink.h
        ../../../BungieFrameWork/BFW_Source/BFW_Bink/BinkHeaders/Rad.h

        ../../../BungieFrameWork/BFW_Source/BFW_Bink/BFW_Bink.c)

add_executable(Oni WIN32
        ${BFW_SOURCE}

        ###############################################################################

        ../../OniGameSource/Oni_AI/Oni_AI.c
        ../../OniGameSource/Oni_AI/Oni_AI.h
        ../../OniGameSource/Oni_AI/Oni_AI2.c
        ../../OniGameSource/Oni_AI/Oni_AI2.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Alarm.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Alarm.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Alert.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Alert.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Combat.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Combat.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Error.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Error.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Executor.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Executor.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Fight.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Fight.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Guard.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Guard.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Idle.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Idle.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Knowledge.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Knowledge.h
        ../../OniGameSource/Oni_AI/Oni_AI2_LocalPath.c
        ../../OniGameSource/Oni_AI/Oni_AI2_LocalPath.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Maneuver.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Maneuver.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Melee.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Melee.h
        ../../OniGameSource/Oni_AI/Oni_AI2_MeleeProfile.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Movement.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Movement.h
        ../../OniGameSource/Oni_AI/Oni_AI2_MovementState.c
        ../../OniGameSource/Oni_AI/Oni_AI2_MovementState.h
        ../../OniGameSource/Oni_AI/Oni_AI2_MovementStub.c
        ../../OniGameSource/Oni_AI/Oni_AI2_MovementStub.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Neutral.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Neutral.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Panic.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Panic.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Path.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Path.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Patrol.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Patrol.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Pursuit.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Pursuit.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Script.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Script.h
        ../../OniGameSource/Oni_AI/Oni_AI2_Targeting.c
        ../../OniGameSource/Oni_AI/Oni_AI2_Targeting.h
        ../../OniGameSource/Oni_AI/Oni_AI2_TeamBattle.c
        ../../OniGameSource/Oni_AI/Oni_AI2_TeamBattle.h
        ../../OniGameSource/Oni_AI/Oni_AI_Action.h
        ../../OniGameSource/Oni_AI/Oni_AI_Activity.h
        ../../OniGameSource/Oni_AI/Oni_AI_Behaviour.h
        ../../OniGameSource/Oni_AI/Oni_AI_Group.h
        ../../OniGameSource/Oni_AI/Oni_AI_Profile.h
        ../../OniGameSource/Oni_AI/Oni_AI_Script.c
        ../../OniGameSource/Oni_AI/Oni_AI_Script.h
        ../../OniGameSource/Oni_AI/Oni_AI_Setup.h
        ../../OniGameSource/Oni_AI/Oni_AStar.c
        ../../OniGameSource/Oni_AI/Oni_AStar.h

        ../../OniGameSource/Oni_Windows/Oni_Win_AI.c
        ../../OniGameSource/Oni_Windows/Oni_Win_AI.h
        ../../OniGameSource/Oni_Windows/Oni_Win_AI_Character.c
        ../../OniGameSource/Oni_Windows/Oni_Win_AI_Combat.c
        ../../OniGameSource/Oni_Windows/Oni_Win_AI_Combat.h
        ../../OniGameSource/Oni_Windows/Oni_Win_AI_Melee.c
        ../../OniGameSource/Oni_Windows/Oni_Win_AI_Melee.h
        ../../OniGameSource/Oni_Windows/Oni_Win_AI_Neutral.c
        ../../OniGameSource/Oni_Windows/Oni_Win_AI_Neutral.h
        ../../OniGameSource/Oni_Windows/Oni_Win_AI_Path.c
        ../../OniGameSource/Oni_Windows/Oni_Win_ImpactEffects.c
        ../../OniGameSource/Oni_Windows/Oni_Win_ImpactEffects.h
        ../../OniGameSource/Oni_Windows/Oni_Win_Particle.c
        ../../OniGameSource/Oni_Windows/Oni_Win_Particle.h
        ../../OniGameSource/Oni_Windows/Oni_Win_Settings.c
        ../../OniGameSource/Oni_Windows/Oni_Win_Sound2.c
        ../../OniGameSource/Oni_Windows/Oni_Win_Sound2.h
        ../../OniGameSource/Oni_Windows/Oni_Win_TextureMaterials.c
        ../../OniGameSource/Oni_Windows/Oni_Win_TextureMaterials.h
        ../../OniGameSource/Oni_Windows/Oni_Win_Tools.c
        ../../OniGameSource/Oni_Windows/Oni_Win_Tools.h
        ../../OniGameSource/Oni_Windows.c
        ../../OniGameSource/Oni_Windows.h
        ../../OniGameSource/Oni_Windows2.c

        ../../OniGameSource/Oni_Object/object_types/OT_Character.c
        ../../OniGameSource/Oni_Object/object_types/OT_Combat.c
        ../../OniGameSource/Oni_Object/object_types/OT_Console.c
        ../../OniGameSource/Oni_Object/object_types/OT_Door.c
        ../../OniGameSource/Oni_Object/object_types/OT_Flags.c
        ../../OniGameSource/Oni_Object/object_types/OT_Furniture.c
        ../../OniGameSource/Oni_Object/object_types/OT_Melee.c
        ../../OniGameSource/Oni_Object/object_types/OT_Neutral.c
        ../../OniGameSource/Oni_Object/object_types/OT_Particle.c
        ../../OniGameSource/Oni_Object/OT_PatrolPoint.c
        ../../OniGameSource/Oni_Object/object_types/OT_PowerUps.c
        ../../OniGameSource/Oni_Object/object_types/OT_Sound.c
        ../../OniGameSource/Oni_Object/object_types/OT_Trigger.c
        ../../OniGameSource/Oni_Object/object_types/OT_TriggerVolume.c
        ../../OniGameSource/Oni_Object/object_types/OT_Turret.c
        ../../OniGameSource/Oni_Object/object_types/OT_Weapon.c

        ../../OniGameSource/Oni_Object/Oni_Object.c
        ../../OniGameSource/Oni_Object/Oni_Object.h
        ../../OniGameSource/Oni_Object/Oni_Object_Private.h
        ../../OniGameSource/Oni_Object/Oni_Object_RegisterTemplates.c
        ../../OniGameSource/Oni_Object/Oni_Object_Utils.c

        ../../OniGameSource/Oni_BinaryData/Oni_BinaryData.c
        ../../OniGameSource/Oni_BinaryData/Oni_BinaryData.h

        ../../OniGameSource/Oni_OutGameUI/Oni_OutGameUI.c
        ../../OniGameSource/Oni_OutGameUI/Oni_OutGameUI.h

        ../../OniGameSource/Oni.c
        ../../OniGameSource/Oni.h
        ../../OniGameSource/Oni_Aiming.c
        ../../OniGameSource/Oni_Aiming.h
        ../../OniGameSource/Oni_Bink.c
        ../../OniGameSource/Oni_Bink.h
        ../../OniGameSource/Oni_Camera.c
        ../../OniGameSource/Oni_Camera.h
        ../../OniGameSource/Oni_Camera_Templates.c
        ../../OniGameSource/ONI_Character.c
        ../../OniGameSource/ONI_Character.h
        ../../OniGameSource/Oni_Character_Animation.h
        ../../OniGameSource/Oni_Cinematics.c
        ../../OniGameSource/Oni_Cinematics.h
        ../../OniGameSource/Oni_Combos.c
        ../../OniGameSource/Oni_Combos.h
        ../../OniGameSource/Oni_Event.c
        ../../OniGameSource/Oni_Event.h
        ../../OniGameSource/Oni_Film.c
        ../../OniGameSource/Oni_Film.h
        ../../OniGameSource/Oni_FX.c
        ../../OniGameSource/Oni_FX.h
        ../../OniGameSource/Oni_GameSettings.c
        ../../OniGameSource/Oni_GameSettings.h
        ../../OniGameSource/Oni_GameState.c
        ../../OniGameSource/Oni_GameState.h
        ../../OniGameSource/Oni_GameStatePrivate.h
        ../../OniGameSource/Oni_ImpactEffect.c
        ../../OniGameSource/Oni_ImpactEffect.h
        ../../OniGameSource/Oni_InGameUI.c
        ../../OniGameSource/Oni_InGameUI.h
        ../../OniGameSource/Oni_KeyBindings.c
        ../../OniGameSource/Oni_KeyBindings.h
        ../../OniGameSource/Oni_Level.c
        ../../OniGameSource/Oni_Level.h
        ../../OniGameSource/Oni_Mechanics.c
        ../../OniGameSource/Oni_Mechanics.h
        ../../OniGameSource/Oni_Motoko.c
        ../../OniGameSource/Oni_Motoko.h
        ../../OniGameSource/Oni_Particle3.c
        ../../OniGameSource/Oni_Particle3.h
        ../../OniGameSource/Oni_Path.c
        ../../OniGameSource/Oni_Path.h
        ../../OniGameSource/Oni_Persistance.c
        ../../OniGameSource/Oni_Persistance.h
        ../../OniGameSource/Oni_Platform.h
        ../../OniGameSource/Platform_Win32/Oni_Platform_Win32.c
        ../../OniGameSource/Platform_Win32/Oni_Platform_Win32.h
        ../../OniGameSource/Oni_Script.c
        ../../OniGameSource/Oni_Script.h
        ../../OniGameSource/Oni_Sky.c
        ../../OniGameSource/Oni_Sky.h
        ../../OniGameSource/Oni_Sound2.c
        ../../OniGameSource/Oni_Sound2.h
        ../../OniGameSource/Oni_Sound_Animation.c
        ../../OniGameSource/Oni_Sound_Animation.h
        ../../OniGameSource/Oni_Sound_Private.h
        ../../OniGameSource/Oni_Speech.c
        ../../OniGameSource/Oni_Speech.h
        ../../OniGameSource/Oni_Templates.c
        ../../OniGameSource/Oni_Templates.h
        ../../OniGameSource/Oni_TextureMaterials.c
        ../../OniGameSource/Oni_TextureMaterials.h
        ../../OniGameSource/Oni_Weapon.c
        ../../OniGameSource/Oni_Weapon.h

        ../../OniResources/OniResources.rc
        ../../OniResources/resource.h)

if(WIN32)
target_link_libraries(Oni winmm
        MSACM32.LIB
        BINKW32.LIB
        Ws2_32
        dinput
        dsound
        dxguid
        odbc32
        odbccp32
        imagehlp)
endif()
