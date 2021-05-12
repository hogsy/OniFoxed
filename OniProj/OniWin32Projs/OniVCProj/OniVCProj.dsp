# Microsoft Developer Studio Project File - Name="OniVCProj" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=OniVCProj - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "OniVCProj.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OniVCProj.mak" CFG="OniVCProj - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OniVCProj - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "OniVCProj - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "OniVCProj - Win32 ReleaseWithAsserts" (based on "Win32 (x86) Application")
!MESSAGE "OniVCProj - Win32 ShippingVersion" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/BungieSource/OniProj/OniWin32Projs/OniVCProj", UHAAAAAA"
# PROP Scc_LocalPath "."
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "OniVCProj - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "OniVCProj___Win32_Release"
# PROP BASE Intermediate_Dir "OniVCProj___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "OniVCProj___Win32_Release"
# PROP Intermediate_Dir "OniVCProj___Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gr /W3 /GX /Zi /O2 /Ob2 /FI"BFW_MasterHeader.h" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0400 /D DEBUG_EXTERNAL=0 /D ONI_MAP_FILE=\"OniTool.map\" /YX"BFW_MasterHeader.h" /FD @../OniVCDirectories.txt /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib MSACM32.LIB BINKW32.LIB Ws2_32.lib dinput.lib dsound.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib imagehlp.lib /nologo /subsystem:windows /profile /map /debug /debugtype:both /machine:I386 /nodefaultlib:"LIBCMT" /out:"OniVCProj___Win32_Release/OniTool.exe" /fixed:no

!ELSEIF  "$(CFG)" == "OniVCProj - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "OniVCProj___Win32_Debug"
# PROP BASE Intermediate_Dir "OniVCProj___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "OniVCProj___Win32_Debug"
# PROP Intermediate_Dir "OniVCProj___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /Gi /GX /Zi /Od /FI"BFW_MasterHeader.h" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0400 /D DEBUGGING=1 /D ONI_MAP_FILE=\"OniDbg.map\" /YX"BFW_MasterHeader.h" /FD /GZ @../OniVCDirectories.txt /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 IMAGEHLP.lib WINMM.LIB BINKW32.LIB Ws2_32.lib dinput.lib dsound.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib msacm32.lib imagehlp.lib /nologo /subsystem:windows /profile /map /debug /debugtype:both /machine:I386 /nodefaultlib:"LIBCMT" /out:"OniVCProj___Win32_Debug/OniDbg.exe"

!ELSEIF  "$(CFG)" == "OniVCProj - Win32 ReleaseWithAsserts"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "OniVCProj___Win32_ReleaseWithAsserts"
# PROP BASE Intermediate_Dir "OniVCProj___Win32_ReleaseWithAsserts"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "OniVCProj___Win32_ReleaseWithAsserts"
# PROP Intermediate_Dir "OniVCProj___Win32_ReleaseWithAsserts"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /W3 /GX /Zi /O2 /FI"BFW.h" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0400 /D DEBUG_EXTERNAL=0 /FR /YX"BFW.h" /FD @../OniVCDirectories.txt /c
# ADD CPP /nologo /Gr /W3 /GX /Zi /Od /FI"BFW_MasterHeader.h" /D CONSOLE_DEBUGGING_COMMANDS=0 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0400 /D DEBUG_EXTERNAL=1 /D ONI_MAP_FILE=\"OniOptDbg.map\" /YX"BFW_MasterHeader.h" /FD @../OniVCDirectories.txt /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 Ws2_32.lib dsound.lib ddraw.lib dinput.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /profile /map /debug /machine:I386 /nodefaultlib:"LIBCMT"
# ADD LINK32 winmm.lib MSACM32.LIB BINKW32.LIB Ws2_32.lib dinput.lib dsound.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib imagehlp.lib /nologo /subsystem:windows /profile /map /debug /debugtype:both /machine:I386 /nodefaultlib:"LIBCMT" /out:"OniVCProj___Win32_ReleaseWithAsserts/OniOptDbg.exe"

!ELSEIF  "$(CFG)" == "OniVCProj - Win32 ShippingVersion"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "OniVCProj___Win32_ShippingVersion"
# PROP BASE Intermediate_Dir "OniVCProj___Win32_ShippingVersion"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "OniVCProj___Win32_ShippingVersion"
# PROP Intermediate_Dir "OniVCProj___Win32_ShippingVersion"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /W3 /GX /Zi /O2 /Ob2 /FI"BFW_MasterHeader.h" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0400 /D DEBUG_EXTERNAL=0 /YX"BFW_MasterHeader.h" /FD @../OniVCDirectories.txt /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /Gr /W3 /GX /O2 /Ob2 /FI"BFW_MasterHeader.h" /D SHIPPING_VERSION=1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0400 /D DEBUG_EXTERNAL=0 /YX"BFW_MasterHeader.h" /FD @../OniVCDirectories.txt /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 BINKW32.LIB OPENGL32.LIB glu32.lib Ws2_32.lib dsound.lib ddraw.lib dinput.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /profile /map /debug /machine:I386 /nodefaultlib:"LIBCMT" /out:"OniVCProj___Win32_Release/Oni.exe" /fixed:no
# ADD LINK32 winmm.lib MSACM32.LIB BINKW32.LIB Ws2_32.lib dinput.lib dsound.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /pdb:none /machine:I386 /nodefaultlib:"LIBCMT" /out:"OniVCProj___Win32_ShippingVersion/Oni.exe" /fixed:no

!ENDIF 

# Begin Target

# Name "OniVCProj - Win32 Release"
# Name "OniVCProj - Win32 Debug"
# Name "OniVCProj - Win32 ReleaseWithAsserts"
# Name "OniVCProj - Win32 ShippingVersion"
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

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_BinaryData.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Bink.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_BitVector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Collision.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_CommandLine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Console.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_DebuggerSymbols.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_DialogManager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Doors.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Effect.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_EnvParticle.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_ErrorCodes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_FileFormat.h
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

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_MasterHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Materials.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_MathLib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Motoko.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Noise.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Object.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Particle3.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Path.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Physics.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_ScriptLang.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_SoundSystem2.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_TemplateManager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_TextSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Timer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Totoro.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_WindowManager.h
# End Source File
# End Group
# Begin Group "BFW_Source"

# PROP Default_Filter ""
# Begin Group "BFW_FileManager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileManager\BFW_FileManager_Common.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileManager\Platform_MacOS\BFW_FileManager_MacOS.c
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

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_LocalInput\Platform_MacOS\BFW_LI_Platform_MacOS.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_LocalInput\Platform_MacOS\BFW_LI_Platform_MacOS.h
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

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_LocalInput\BFW_LI_Translators.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_LocalInput\BFW_LocalInput.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_LocalInput\BFW_LocalInput_Template.c
# End Source File
# End Group
# Begin Group "BFW_Motoko"

# PROP Default_Filter ""
# Begin Group "Engines"

# PROP Default_Filter ""
# Begin Group "DrawEngine"

# PROP Default_Filter ""
# Begin Group "StefanGL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\OpenGL\gl_code_version.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\OpenGL\gl_engine.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\OpenGL\gl_engine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\OpenGL\gl_geometry_draw_method.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\OpenGL\gl_mswindows.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\OpenGL\gl_utility.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\OpenGL\glext.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\Software\MS_DrawEngine_Method.h
# End Source File
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

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\GeomEngine\Software\MS_GC_Method_Pick.h
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

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_DefaultFunctions.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_DefaultFunctions.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Draw.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Draw.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Geom.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Geom_Camera.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Geom_Matrix.c
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

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Sort.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Sort.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_State_Draw.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_State_Geom.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Texture.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Utility.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Verify.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\Motoko_Verify.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Manager\BFW_Motoko_Template.c
# End Source File
# End Group
# Begin Group "BFW_TemplateManager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_TemplateManager\BFW_TM_Common.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_TemplateManager\BFW_TM_Construction.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_TemplateManager\BFW_TM_Construction.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_TemplateManager\BFW_TM_Game.c
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

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\bfw_cseries.h
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

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_Noise.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_Platform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\Platform_MacOS\BFW_Platform_AltiVec.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_Platform_AltiVec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\Platform_MacOS\BFW_Platform_MacOS.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\Platform_Win32\BFW_Platform_Win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_String.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_Timer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_Utility.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\lrar_cache.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Motoko\Engines\DrawEngine\3DFX_Glide\lrar_cache2.c
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

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\bfw_math.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\bfw_math.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\bfw_math_3dnow.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\bfw_math_stdc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\BFW_MathLib.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\BFW_MathLib_Matrix.c
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
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\EulerAngles.h
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

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Object\BFW_Doors.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Object\BFW_Object.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Object\BFW_Object_Templates.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Object\BFW_Physics.c
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
# End Group
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_CommandLine\BFW_CommandLine.c
# End Source File
# End Group
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

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Akira\BFW_Akira_LightMap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Akira\BFW_Akira_Render.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Akira\BFW_Akira_Template.c
# End Source File
# End Group
# Begin Group "BFW_FileFormat"

# PROP Default_Filter ""
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
# Begin Group "DDS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileFormat\DDS\BFW_FF_DDS.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileFormat\DDS\BFW_FF_DDS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileFormat\DDS\BFW_FF_DDS_Priv.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_FileFormat\BFW_FileFormat.c
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

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Image\BFW_Image_Draw.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Image\BFW_Image_PixelConversion.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Image\BFW_Image_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Image\BFW_Image_Scale_Box.c
# End Source File
# End Group
# Begin Group "BFW_Particle"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Particle\BFW_Decal.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Particle\BFW_EnvParticle.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Particle\BFW_Particle3.c
# End Source File
# End Group
# Begin Group "BFW_DebuggerSymbols"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DebuggerSymbols\BFW_DebuggerSymbols_Win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DebuggerSymbols\stack_walk_windows.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_DebuggerSymbols\stack_walk_windows.h
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
# Begin Group "BFW_Effect"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Effect\BFW_Effect.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Effect\BFW_Effect_Private.h
# End Source File
# End Group
# Begin Group "BFW_WindowManager"

# PROP Default_Filter ""
# Begin Group "WM_Cursor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Cursor\WM_Cursor.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Cursor\WM_Cursor.h
# End Source File
# End Group
# Begin Group "WM_Dialog"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Dialog\WM_Dialog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Dialog\WM_Dialog.h
# End Source File
# End Group
# Begin Group "WM_DrawContext"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_DrawContext\WM_DrawContext.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_DrawContext\WM_DrawContext.h
# End Source File
# End Group
# Begin Group "WM_PartSpecification"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_PartSpecification\WM_PartSpecification.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_PartSpecification\WM_PartSpecification.h
# End Source File
# End Group
# Begin Group "WM_Utilities"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Utilities\WM_Utilities.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Utilities\WM_Utilities.h
# End Source File
# End Group
# Begin Group "WM_Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Box.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Box.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Button.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Button.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_CheckBox.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_CheckBox.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_EditField.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_EditField.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_ListBox.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_ListBox.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Menu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Menu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_MenuBar.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_MenuBar.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Picture.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Picture.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_PopupMenu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_PopupMenu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_ProgressBar.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_ProgressBar.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_RadioButton.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_RadioButton.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Scrollbar.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Scrollbar.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Slider.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Slider.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Text.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Text.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Window.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\WM_Windows\WM_Window.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\BFW_WindowManager.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\BFW_WindowManager_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_WindowManager\BFW_WindowManager_Public.h
# End Source File
# End Group
# Begin Group "BFW_ScriptLang"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Context.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Context.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Database.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Database.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Error.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Error.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Eval.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Eval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Expr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Expr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Parse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Parse.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Scheduler.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Scheduler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Token.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_ScriptLang\BFW_ScriptLang_Token.h
# End Source File
# End Group
# Begin Group "BFW_BinaryData"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_BinaryData\BFW_BinaryData.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_BinaryData\BFW_BinaryData_Template.c
# End Source File
# End Group
# Begin Group "BFW_SoundSystem2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem2\BFW_SoundSystem2.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem2\BFW_SS2_IMA.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem2\BFW_SS2_IMA.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem2\BFW_SS2_MSADPCM.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem2\BFW_SS2_MSADPCM.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem2\BFW_SS2_Platform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem2\Platform_MacOS\BFW_SS2_Platform_MacOS.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem2\Platform_Win32\BFW_SS2_Platform_Win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem2\BFW_SS2_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_SoundSystem2\BFW_SS2_RegisterTemplate.c
# End Source File
# End Group
# Begin Group "BFW_Materials"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Materials\BFW_Materials.c
# End Source File
# End Group
# Begin Group "BFW_Bink"

# PROP Default_Filter ""
# Begin Group "BinkHeaders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Bink\BinkHeaders\bink.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Bink\BinkHeaders\Rad.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Bink\BFW_Bink.c
# End Source File
# End Group
# End Group
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

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Alarm.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Alarm.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Alert.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Alert.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Combat.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Combat.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Error.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Error.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Executor.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Executor.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Fight.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Fight.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Guard.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Guard.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Idle.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Idle.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Knowledge.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Knowledge.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_LocalPath.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_LocalPath.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Maneuver.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Maneuver.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Melee.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Melee.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_MeleeProfile.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Movement.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Movement.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_MovementState.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_MovementState.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_MovementStub.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_MovementStub.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Neutral.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Neutral.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Panic.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Panic.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Path.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Path.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Patrol.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Patrol.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Pursuit.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Pursuit.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Script.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Script.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Targeting.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_Targeting.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_TeamBattle.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI2_TeamBattle.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Action.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Activity.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Behaviour.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Group.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Profile.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Script.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_AI\Oni_AI_Script.h
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
# Begin Group "Oni_Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_AI.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_AI.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_AI_Character.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_AI_Combat.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_AI_Combat.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_AI_Melee.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_AI_Melee.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_AI_Neutral.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_AI_Neutral.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_AI_Path.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_ImpactEffects.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_ImpactEffects.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_Particle.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_Particle.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_Settings.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_Sound2.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_Sound2.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_TextureMaterials.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_TextureMaterials.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_Tools.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows\Oni_Win_Tools.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Windows2.c
# End Source File
# End Group
# Begin Group "Oni_Objects"

# PROP Default_Filter ""
# Begin Group "Oni_ObjectTypes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Character.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Combat.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Console.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Door.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Flags.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Furniture.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Melee.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Neutral.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Particle.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\OT_PatrolPoint.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_PowerUps.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Sound.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Trigger.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_TriggerVolume.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Turret.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\object_types\OT_Weapon.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\Oni_Object.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\Oni_Object.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\Oni_Object_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\Oni_Object_RegisterTemplates.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Object\Oni_Object_Utils.c
# End Source File
# End Group
# Begin Group "Oni_BinaryData"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_BinaryData\Oni_BinaryData.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_BinaryData\Oni_BinaryData.h
# End Source File
# End Group
# Begin Group "Oni_OutGameUI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_OutGameUI\Oni_OutGameUI.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_OutGameUI\Oni_OutGameUI.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\OniGameSource\Oni.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Aiming.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Aiming.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Bink.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Bink.h
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

SOURCE=..\..\OniGameSource\Oni_Character_Animation.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Cinematics.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Cinematics.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Combos.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Combos.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Event.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Event.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Film.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Film.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_FX.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_FX.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_GameSettings.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_GameSettings.h
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

SOURCE=..\..\OniGameSource\Oni_ImpactEffect.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_ImpactEffect.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_InGameUI.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_InGameUI.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_KeyBindings.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_KeyBindings.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Level.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Level.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Mechanics.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Mechanics.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Motoko.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Motoko.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Particle3.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Particle3.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Path.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Path.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Persistance.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Persistance.h
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

SOURCE=..\..\OniGameSource\Oni_Script.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Script.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Sky.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Sky.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Sound2.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Sound2.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Sound_Animation.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Sound_Animation.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Sound_Private.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Speech.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Speech.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Templates.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Templates.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_TextureMaterials.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_TextureMaterials.h
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Weapon.c
# End Source File
# Begin Source File

SOURCE=..\..\OniGameSource\Oni_Weapon.h
# End Source File
# End Group
# Begin Group "OniResources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\OniResources\oni.ico
# End Source File
# Begin Source File

SOURCE=..\..\OniResources\OniResources.rc
# End Source File
# Begin Source File

SOURCE=..\..\OniResources\resource.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\OniVCDirectories.txt
# End Source File
# Begin Source File

SOURCE=..\impconsole\templatechecksum.c
# End Source File
# End Target
# End Project
