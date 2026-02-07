project(TEVCProj)

set(templatechecksum_path "${OniAll_SOURCE_DIR}/OniProj/OniWin32Projs/ImpConsole/templatechecksum.c")

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
        ${templatechecksum_path}
)

target_precompile_headers(TEVCProj PRIVATE ../../../BungieFrameWork/BFW_Headers/BFW_MasterHeader.h)

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

if (TARGET TEVCProj AND NOT CMAKE_CROSSCOMPILING)
    set(TEVCWorkingDirectory "${OniAll_SOURCE_DIR}/OniProj/OniCMakeProjs/TEVCProj")
    set(intermediate_templatechecksum_path "${TEVCWorkingDirectory}/templatechecksum.c")

    # Create list of headers contributing to templates
    set(path_template_file_list_txt "${OniAll_SOURCE_DIR}/OniProj/TemplateFileList.txt")
    file(STRINGS "${path_template_file_list_txt}" header_paths)
    list(TRANSFORM header_paths REPLACE "[\\]" "/")
    list(TRANSFORM header_paths PREPEND "${TEVCWorkingDirectory}/")

    # Automatically regenerate templatechecksum.c
    add_custom_command(
        OUTPUT "${intermediate_templatechecksum_path}"
        COMMAND TEVCProj
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${intermediate_templatechecksum_path}" "${templatechecksum_path}"
        DEPENDS TEVCProj "${path_template_file_list_txt}" ${header_paths}
        WORKING_DIRECTORY "${TEVCWorkingDirectory}")
    # Add manual regenerate-templatechecksum target to force regeneration
    add_custom_target(regenerate-templatechecksum
        COMMAND TEVCProj
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${intermediate_templatechecksum_path}" "${templatechecksum_path}"
        DEPENDS TEVCProj
        WORKING_DIRECTORY "${TEVCWorkingDirectory}")
else ()
    message(STATUS "TEVCProj not available: templatechecksum.c will not be regenerated")
endif ()
