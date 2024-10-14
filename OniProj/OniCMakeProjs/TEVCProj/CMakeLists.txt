project(TEVCProj)

set(BFW_SOURCE_FILES
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
)

if (WIN32)
    add_executable(TEVCProj
            ${BFW_SOURCE_FILES}
            ../../../BungieFrameWork/BFW_Source/BFW_DebuggerSymbols/BFW_DebuggerSymbols_Win32.c
            ../../../BungieFrameWork/BFW_Source/BFW_FileManager/Platform_Win32/BFW_FileManager_Win32.c
            ../../../BungieFrameWork/BFW_Source/BFW_Utility/Platform_Win32/BFW_Platform_Win32.c

            # shoved here for now
            ../../OniWin32Projs/ImpConsole/templatechecksum.c
    )
endif ()

target_precompile_headers(TEVCProj PRIVATE ../../../BungieFrameWork/BFW_Headers/BFW_MasterHeader.h)

if (MSVC)
    target_compile_options(TEVCProj
            PRIVATE
            /FI../../../BungieFrameWork/BFW_Headers/BFW_MasterHeader.h
    )
endif ()

set(TEVCProjExecutable "")
if (WIN32)
    set(TEVCProjExecutable "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/TEVCProj.exe")
else ()
    set(TEVCProjExecutable "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/TEVCProj")
endif ()

if (EXISTS "${TEVCProjExecutable}")
    message("Found TEVCProj under output directory, attempting to update templatechecksum.c")
    execute_process(
            COMMAND "${TEVCProjExecutable}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/OniProj/OniCMakeProjs/TEVCProj/")
else ()
    message("Failed to find TEVCProj under output directory (${CMAKE_RUNTIME_OUTPUT_DIRECTORY})!")
endif ()
