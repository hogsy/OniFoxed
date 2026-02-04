project(TEVCProj)

add_executable(TEVCProj
        ../../../BungieFrameWork/BFW_Source/BFW_AppUtilities/BFW_AppUtilities.c
        ../../../BungieFrameWork/BFW_Source/BFW_FileManager/BFW_FileManager_Common.c
        ../../../BungieFrameWork/BFW_Source/BFW_Group/BFW_Group.c
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/BFW_MathLib.c
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/BFW_MathLib_Matrix.c
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/Decompose.c
        ../../../BungieFrameWork/BFW_Source/BFW_MathLib/EulerAngles.c
        ../../../BungieFrameWork/BFW_Source/BFW_TemplateManager/BFW_TM_Common.c
        ../../../BungieFrameWork/BFW_Source/BFW_TemplateManager/BFW_TM_Construction.c
        ../../../BungieFrameWork/BFW_Source/BFW_TemplateManager/BFW_TM_Game.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_BitVector.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Error.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Memory.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_String.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Timer.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/BFW_Utility.c
        ../../../BungieFrameWork/BFW_ToolSource/Common/TemplateExtractor/TemplateExtractor.c
        ../../../BungieFrameWork/BFW_ToolSource/Common/TemplateExtractor/TE_Extract.c
        ../../../BungieFrameWork/BFW_ToolSource/Common/TemplateExtractor/TE_Parser.c
        ../../../BungieFrameWork/BFW_ToolSource/Common/TemplateExtractor/TE_Symbol.c

        # shoved here for now
        ../../OniWin32Projs/ImpConsole/templatechecksum.c
)

target_precompile_headers(TEVCProj PRIVATE ../../../BungieFrameWork/BFW_Headers/BFW_MasterHeader.h)

if(LINUX)
    endif()

if (Platform_SDL)
    target_sources(TEVCProj PRIVATE
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/Platform_SDL/BFW_Platform_SDL.c
    )
    target_compile_definitions(TEVCProj PRIVATE UUmSDL=1)
    target_link_libraries(TEVCProj PRIVATE
        SDL2::SDL2
    )
endif()

if (LINUX)
    target_sources(TEVCProj PRIVATE
        ../../../BungieFrameWork/BFW_Source/BFW_FileManager/Platform_Linux/BFW_FileManager_Linux.c
        ../../../BungieFrameWork/BFW_Source/BFW_DebuggerSymbols/BFW_DebuggerSymbols_Win32.c
    )
    target_link_libraries(TEVCProj PRIVATE m)
elseif (WIN32)
    target_sources(TEVCProj PRIVATE
        ../../../BungieFrameWork/BFW_Source/BFW_DebuggerSymbols/BFW_DebuggerSymbols_Win32.c
        ../../../BungieFrameWork/BFW_Source/BFW_FileManager/Platform_Win32/BFW_FileManager_Win32.c
        ../../../BungieFrameWork/BFW_Source/BFW_Utility/Platform_Win32/BFW_Platform_Win32.c
    )
endif()

if (MSVC)
    target_compile_options(TEVCProj
            PRIVATE
            /FI../../../BungieFrameWork/BFW_Headers/BFW_MasterHeader.h
    )
endif ()

set(TEVCWorkingDirectory "${CMAKE_SOURCE_DIR}/OniProj/OniCMakeProjs/TEVCProj")

if (TARGET TEVCProj AND NOT CMAKE_CROSSCOMPILING)
    message("Found TEVCProj under output directory, attempting to update templatechecksum.c")
    execute_process(
            COMMAND TEVCProj
            WORKING_DIRECTORY "${TEVCWorkingDirectory}")

    # check if anything was spat out
    if (EXISTS "${TEVCWorkingDirectory}/templatechecksum.c")
        message("Overwriting templatechecksum.c with new copy...")
        # remove it, overwrite is unreliable/unsupported with cmake?
        file(REMOVE "${CMAKE_SOURCE_DIR}/OniProj/OniWin32Projs/ImpConsole/templatechecksum.c")
        # and now copy the new file over
        file(COPY_FILE
                "${TEVCWorkingDirectory}/templatechecksum.c"
                "${CMAKE_SOURCE_DIR}/OniProj/OniWin32Projs/ImpConsole/templatechecksum.c"
                RESULT error
        )
        if (NOT ${error} EQUAL 0)
            message(FATAL_ERROR "Failed to copy templatechecksum.c to destination!")
        endif ()
    else ()
        message("No output found, likely checksum match.")
    endif ()
else ()
    message(" Failed to find TEVCProj under output directory (${CMAKE_RUNTIME_OUTPUT_DIRECTORY}) !")
endif ()
