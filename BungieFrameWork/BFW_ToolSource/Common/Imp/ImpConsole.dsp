# Microsoft Developer Studio Project File - Name="ImpConsole" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ImpConsole - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ImpConsole.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ImpConsole.mak" CFG="ImpConsole - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ImpConsole - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ImpConsole - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/BungieSource/OniProj/OniWin32Projs/ImpConsole", LLAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ImpConsole - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ImpConsole___Win32_Release"
# PROP BASE Intermediate_Dir "ImpConsole___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ImpConsole___Win32_Release"
# PROP Intermediate_Dir "ImpConsole___Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /FI"BFW.h" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D _WIN32_WINNT=0x0400 /YX"BFW.h" /FD @../OniVCDirectories.txt /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 Ws2_32.lib dsound.lib ddraw.lib dinput.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib lvsios.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"LIBCMT"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ImpConsole - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ImpConsole___Win32_Debug0"
# PROP BASE Intermediate_Dir "ImpConsole___Win32_Debug0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ImpConsole___Win32_Debug0"
# PROP Intermediate_Dir "ImpConsole___Win32_Debug0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /FI"BFW.h" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D _WIN32_WINNT=0x0400 /D DEBUGGING=1 /FR /YX"BFW.h" /FD /GZ @../OniVCDirectories.txt /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Ws2_32.lib dsound.lib ddraw.lib dinput.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib lvsios.lib /nologo /subsystem:console /profile /debug /machine:I386 /nodefaultlib:"LIBCMT"

!ENDIF 

# Begin Target

# Name "ImpConsole - Win32 Release"
# Name "ImpConsole - Win32 Debug"
# Begin Group "BungieFrameWork"

# PROP Default_Filter ""
# Begin Group "BFW_Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Akira.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_AppUtilities.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Batou.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_BitVector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Collision.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Console.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_ErrorCodes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_FileManager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Group.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Image.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_LocalInput.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_MathLib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Motoko.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_NetworkManager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Object.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_RedPig.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_RemoteInput.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_SoundSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_TemplateManager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_TextSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Totoro.h
# End Source File
# End Group
# Begin Group "BFW_Source"

# PROP Default_Filter ""
# Begin Group "BFW_Akira"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Akira\Akira_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Akira\BFW_Akira.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Akira\BFW_Akira_Collision.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Akira\BFW_Akira_Render.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Akira\BFW_Akira_Template.c
# End Source File
# End Group
# Begin Group "BFW_Batou"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Batou\BFW_Batou.c
# End Source File
# End Group
# Begin Group "BFW_FileManager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileManager\BFW_FileManager_Common.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileManager\Platform_Win32\BFW_FileManager_Win32.c
# End Source File
# End Group
# Begin Group "BFW_LocalInput"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_LocalInput\BFW_LI_Platform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_LocalInput\Platform_Win32\BFW_LI_Platform_Win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_LocalInput\Platform_Win32\BFW_LI_Platform_Win32.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_LocalInput\BFW_LI_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_LocalInput\BFW_LocalInput.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_LocalInput\BFW_LocalInput_Template.c
# End Source File
# End Group
# Begin Group "BFW_RemoteInput"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_RemoteInput\BFW_RemoteInput.c
# End Source File
# End Group
# Begin Group "BFW_Shared"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Shared\BFW_Shared_Camera.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Shared\BFW_Shared_Clip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Shared\BFW_Shared_Clip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Shared\BFW_Shared_ClipCompPoint_c.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Shared\BFW_Shared_ClipPoly_c.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Shared\BFW_Shared_Math.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Shared\BFW_Shared_Math.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Shared\BFW_Shared_TriRaster.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Shared\BFW_Shared_TriRaster_c.h
# End Source File
# End Group
# Begin Group "BFW_TemplateManager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_TemplateManager\BFW_TemplateManager.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_TemplateManager\BFW_TM_Construction.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_TemplateManager\BFW_TM_Private.h
# End Source File
# End Group
# Begin Group "BFW_Utility"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_BitVector.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_Error.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_Error.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_Memory.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_Memory.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_Platform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\Platform_Win32\BFW_Platform_Win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_String.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_Utility.c
# End Source File
# End Group
# Begin Group "BFW_Totoro"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Totoro\BFW_Totoro.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Totoro\BFW_Totoro_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Totoro\BFW_Totoro_Template.c
# End Source File
# End Group
# Begin Group "BFW_RedPig"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_RedPig\BFW_RedPig.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_RedPig\BFW_RedPig_Template.c
# End Source File
# End Group
# Begin Group "BFW_TextSystem"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_TextSystem\BFW_TextSystem.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_TextSystem\BFW_TextSystem_Template.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_TextSystem\BFW_TS_Private.h
# End Source File
# End Group
# Begin Group "BFW_Console"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Console\BFW_Console.c
# End Source File
# End Group
# Begin Group "BFW_Group"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Group\BFW_Group.c
# End Source File
# End Group
# Begin Group "BFW_MathLib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\BFW_MathLib.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\Decompose.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\Decompose.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\EulerAngles.c
# End Source File
# End Group
# Begin Group "BFW_SoundSystem"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem\BFW_SoundSystem.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem\BFW_SoundSystem_Template.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem\BFW_SS_Platform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem\Platform_Win32\BFW_SS_Platform_Win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem\Platform_Win32\BFW_SS_Platform_Win32.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem\BFW_SS_Private.h
# End Source File
# End Group
# Begin Group "BFW_AppUtilities"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_AppUtilities\BFW_AppUtilities.c
# End Source File
# End Group
# Begin Group "BFW_Object"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Object\BFW_Object.c
# End Source File
# End Group
# Begin Group "BFW_Path"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Path\BFW_Path.c
# End Source File
# End Group
# Begin Group "BFW_Collision"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Collision\BFW_Collision.c
# End Source File
# End Group
# Begin Group "BFW_NetworkManager"

# PROP Default_Filter ""
# Begin Group "NM_Queues"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_NetworkManager\NM_Queues\NM_Queues.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_NetworkManager\NM_Queues\NM_Queues.h
# End Source File
# End Group
# Begin Group "NM_WinSock"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_NetworkManager\NM_WinSock\NM_WinSock.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_NetworkManager\NM_WinSock\NM_WinSock.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_NetworkManager\NM_WinSock\NM_WinSock_Private.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_NetworkManager\BFW_NetworkManager.c
# End Source File
# End Group
# Begin Group "BFW_FileFormat"

# PROP Default_Filter ""
# Begin Group "PSD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileFormat\PSD\BFW_FF_PSD.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileFormat\PSD\BFW_FF_PSD.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileFormat\PSD\BFW_FF_PSD_Priv.h
# End Source File
# End Group
# Begin Group "BMP"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileFormat\BMP\BFW_FF_BMP.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileFormat\BMP\BFW_FF_BMP.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileFormat\BMP\BFW_FF_BMP_Priv.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileFormat\BFW_FileFormat.c
# End Source File
# End Group
# Begin Group "BFW_CommandLine"

# PROP Default_Filter ""
# Begin Group "Platform_Win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_CommandLine\Platform_Win32\BFW_CL_Platform_Win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_CommandLine\Platform_Win32\BFW_CL_Platform_Win32.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_CommandLine\Platform_Win32\BFW_CL_Resource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_CommandLine\Platform_Win32\BFW_CL_Resource.rc
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_CommandLine\BFW_CommandLine.c
# End Source File
# End Group
# Begin Group "BFW_Ranma"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Ranma\BFW_Ranma.c
# End Source File
# End Group
# Begin Group "BFW_Motoko"

# PROP Default_Filter ""
# Begin Group "Engines"

# PROP Default_Filter ""
# Begin Group "DrawEngine"

# PROP Default_Filter ""
# Begin Group "3DFX_Glide"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\bfw_cseries.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\lrar_cache.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\lrar_cache.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_Bitmap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_Bitmap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_Frame.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_LinePoint.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_LinePoint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_Pent.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_Pent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_Quad.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_Quad.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_Query.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_Query.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_SmallQuad.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_SmallQuad.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_State.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_State.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_Triangle.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Method_Triangle.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DC_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DrawEngine_Method.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DrawEngine_Method.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_DrawEngine_Platform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_Polygon.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\MG_Polygon.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\rasterizer_3dfx.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\rasterizer_3dfx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\rasterizer_3dfx_windows.c
# End Source File
# End Group
# End Group
# Begin Group "GeomEngine"

# PROP Default_Filter ""
# Begin Group "Software No. 1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Env_Clip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Env_Clip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Camera.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Camera.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Env.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Env.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Frame.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Geometry.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Geometry.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Light.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Light.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Matrix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Pick.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_State.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_State.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_Geom_Clip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_Geom_Clip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_Geom_Math.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_Geom_Math.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_Geom_Shade.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_Geom_Shade.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_Geom_Transform.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_Geom_Transform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GeomEngine_Method.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GeomEngine_Method.h
# End Source File
# End Group
# End Group
# End Group
# Begin Group "Manager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Draw.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Draw.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Manager.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Manager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Win32\Motoko_Platform_Win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Win32\Motoko_Platform_Win32.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Texture.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Visibility.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Visibility.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\BFW_Motoko_Template.c
# End Source File
# End Group
# Begin Group "BFW_Image"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Image\BFW_Image.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Image\BFW_Image_Dither.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Image\BFW_Image_PixelConversion.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Image\BFW_Image_Scale_Box.c
# End Source File
# End Group
# Begin Group "BFW_Particle"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Particle\BFW_Particle.c
# End Source File
# End Group
# Begin Group "BFW_DialogManager"

# PROP Default_Filter ""
# Begin Group "DM_Control"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Controls\DM_Control.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Controls\DM_Control.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Controls\DM_Control_Button.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Controls\DM_Control_Button.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Controls\DM_Control_EditField.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Controls\DM_Control_EditField.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Controls\DM_Control_Picture.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Controls\DM_Control_Picture.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Controls\DM_Control_Text.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Controls\DM_Control_Text.h
# End Source File
# End Group
# Begin Group "DM_Cursor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Cursor\DM_DialogCursor.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\DM_Cursor\DM_DialogCursor.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\BFW_DialogManager.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\BFW_DialogManager_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\BFW_DialogManager_Template.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\BFW_DialogManager_Utilities.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DialogManager\BFW_DialogManager_Utilities.h
# End Source File
# End Group
# End Group
# End Group
# Begin Group "Importer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\LSSolutionFileIO\BFW_LSSolution.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_AI_HHT.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_AI_HHT.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_AI_Script.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_AI_Script.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_ANSI.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_BatchFile.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_BatchFile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Character.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Character.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Common.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Dialog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Dialog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Dialog_Controls.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Dialog_Cursor.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Env2.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Env2.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Env2_CreateInst.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Env2_LightMaps.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Env2_OctTree.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Env2_Parse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Env2_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Env2_Process.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Font.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Font.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Input.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Input.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Model.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Model.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_ParseEnvFile.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_ParseEnvFile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Particle.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Particle.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Path.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Path.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Sound.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Sound.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Texture.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Texture.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Texture_Big.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_TextureFlags.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_TextureFlags.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_WaterSystem.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_WaterSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Weapon.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\Imp\Imp_Weapon.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\TemplateExtractor\TE_Parser.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\TemplateExtractor\TE_Symbol.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\TemplateExtractor\TemplateExtractor.c
# End Source File
# End Group
# Begin Group "OniGameSource"

# PROP Default_Filter ""
# Begin Group "Oni_AI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Action.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Activity.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Behaviour.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Behaviour.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Group.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Group.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Profile.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Script.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Script.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Setup.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Setup.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AStar.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AStar.h
# End Source File
# End Group
# Begin Group "Oni_Networking"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Networking\Oni_Net_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Networking\Oni_Net_Support.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Networking\Oni_Net_Support.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Networking\Oni_Networking.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Networking\Oni_Networking.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\OniGameSource\Oni.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Action.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Aiming.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Aiming.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Camera.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Camera.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Camera_Templates.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\ONI_Character.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\ONI_Character.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Combos.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Command.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Command.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Dialogs.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Game.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Game.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_GameState.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_GameState.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_GameStatePrivate.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Motoko.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Motoko.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Path.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Path.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Performance.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Performance.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Platform.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Platform_Win32\Oni_Platform_Win32.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Platform_Win32\Oni_Platform_Win32.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Templates.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Templates.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Testbed.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Testbed.h
# End Source File
# End Group
# Begin Group "LSPreperation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\LSSolutionFileIO\LPWriter\BFW_LSPreperation.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\LSSolutionFileIO\LPWriter\BFW_LSPreperation.h
# End Source File
# End Group
# Begin Group "LSSolution"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\LSSolutionFileIO\LSReader\BFW_LSBuilder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\LSSolutionFileIO\LSReader\BFW_LSBuilder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\LSSolutionFileIO\LSReader\BFW_LSBuilderFactory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Common\LSSolutionFileIO\LSReader\BFW_LSFileReader.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\OniVCDirectories.txt
# End Source File
# End Target
# End Project
