cmake_minimum_required(VERSION 3.16.0)

project(OniAll LANGUAGES C)

include(CMakeDependentOption)
cmake_dependent_option(ONI_BINK_VIDEO "Build with Bink Video support" OFF "WIN32;CMAKE_SIZEOF_VOID_P EQUAL 4" OFF)

# MSVC 19 finally
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include_directories(
	BungieFrameWork/BFW_Headers/
	BungieFrameWork/BFW_Source/BFW_Akira/
	BungieFrameWork/BFW_Source/BFW_Object/
	BungieFrameWork/BFW_Source/BFW_Batou/
	BungieFrameWork/BFW_Source/BFW_Bink/
	BungieFrameWork/BFW_Source/BFW_Bink/BinkHeaders/
	BungieFrameWork/BFW_Source/BFW_Console/
	BungieFrameWork/BFW_Source/BFW_DialogManager/
	BungieFrameWork/BFW_Source/BFW_DialogManager/DM_Cursor/
	BungieFrameWork/BFW_Source/BFW_DialogManager/VM_Views/
	BungieFrameWork/BFW_Source/BFW_FileManager/
	BungieFrameWork/BFW_Source/BFW_FileFormat/
	BungieFrameWork/BFW_Source/BFW_FileFormat/BMP/
	BungieFrameWork/BFW_Source/BFW_FileFormat/PSD/
	BungieFrameWork/BFW_Source/BFW_FileFormat/DDS/
	BungieFrameWork/BFW_Source/BFW_Group/
	BungieFrameWork/BFW_Source/BFW_Effect/
	BungieFrameWork/BFW_Source/BFW_LocalInput/
	BungieFrameWork/BFW_Source/BFW_MathLib/
	BungieFrameWork/BFW_Source/BFW_Motoko/
	BungieFrameWork/BFW_Source/BFW_Motoko/Engines/OGL_Common/
	BungieFrameWork/BFW_Source/BFW_Motoko/Engines/
	BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/Direct3D/
	BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/Software/
	BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/3DFX_Glide/
	BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/Software/
	BungieFrameWork/BFW_Source/BFW_Motoko/Engines/GeomEngine/GeomOpenGL/
	BungieFrameWork/BFW_Source/BFW_Motoko/Manager/
	BungieFrameWork/BFW_Source/BFW_Motoko/Manager/Win32/
	BungieFrameWork/BFW_Source/BFW_NetworkManager/
	BungieFrameWork/BFW_Source/BFW_NetworkManager/NM_Queues/
	BungieFrameWork/BFW_Source/BFW_NetworkManager/NM_WinSock/
	BungieFrameWork/BFW_Source/BFW_RedPig/
	BungieFrameWork/BFW_Source/BFW_RemoteInput/
	BungieFrameWork/BFW_Source/BFW_Shared/
	BungieFrameWork/BFW_Source/BFW_SoundSystem/
	BungieFrameWork/BFW_Source/BFW_SoundSystem2/
	BungieFrameWork/BFW_Source/BFW_TemplateManager/
	BungieFrameWork/BFW_Source/BFW_TextSystem/
	BungieFrameWork/BFW_Source/BFW_Totoro/
	BungieFrameWork/BFW_Source/BFW_Utility/
	BungieFrameWork/BFW_Source/BFW_WindowManager/
	BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Cursor/
	BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Dialog/
	BungieFrameWork/BFW_Source/BFW_WindowManager/WM_DrawContext/
	BungieFrameWork/BFW_Source/BFW_WindowManager/WM_PartSpecification/
	BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Utilities/
	BungieFrameWork/BFW_Source/BFW_WindowManager/WM_Windows/
	BungieFrameWork/BFW_ToolSource/Common/LSSolutionFileIO/
	BungieFrameWork/BFW_ToolSource/Common/LSSolutionFileIO/LSReader
	BungieFrameWork/BFW_ToolSource/Common/TemplateExtractor
	BungieFrameWork/BFW_ToolSource/Win32/ModelExporter
	BungieFrameWork/BFW_Source/BFW_CommandLine/
	OniProj/OniGameSource/
	OniProj/OniGameSource/Oni_AI/
	OniProj/OniGameSource/Oni_Networking/
	OniProj/OniGameSource/Oni_Windows/
	OniProj/OniGameSource/Oni_Object/
	OniProj/OniGameSource/Oni_BinaryData/
	OniProj/OniGameSource/Oni_OutGameUI/
	BungieFrameWork/BFW_ToolSource/Common/Imp
	BungieFrameWork/BFW_Source/BFW_Motoko/Engines/DrawEngine/OpenGL/
)
if(WIN32)
	include_directories(
		BungieFrameWork/BFW_Source/BFW_LocalInput/Platform_Win32/
		BungieFrameWork/BFW_Source/BFW_SoundSystem/Platform_Win32/
		BungieFrameWork/BFW_Source/BFW_SoundSystem2/Platform_Win32/
		BungieFrameWork/BFW_Source/BFW_CommandLine/Platform_Win32/
		OniProj/OniGameSource/Platform_Win32/
	)
elseif(APPLE)
	include_directories(
		BungieFrameWork/BFW_Source/BFW_LocalInput/Platform_MacOS/
		BungieFrameWork/BFW_Source/BFW_SoundSystem/Platform_MacOS/
		BungieFrameWork/BFW_Source/BFW_SoundSystem2/Platform_MacOS/
		BungieFrameWork/BFW_Source/BFW_CommandLine/Platform_MacOS/
		OniProj/OniGameSource/Platform_MacOS/
	)
endif()

if(ONI_BINK_VIDEO)
	add_subdirectory(OniProj/OniCMakeProjs/BinkProj/)
endif()
add_subdirectory(OniProj/OniCMakeProjs/OniProj/)
