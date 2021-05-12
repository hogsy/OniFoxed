# Microsoft Developer Studio Project File - Name="TEVCProj" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=TEVCProj - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "TEVCProj.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "TEVCProj.mak" CFG="TEVCProj - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TEVCProj - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "TEVCProj - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/BungieSource/TEVCProj", KEAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TEVCProj - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /FI"BFW.h" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX"BFW.h" /FD /c
# SUBTRACT CPP /WX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "TEVCProj - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /FI"BFW.h" /D "_CONSOLE" /D "_MBCS" /D "WIN32" /D "_DEBUG" /D DEBUGGING=1 /FR /YX"BFW.h" /FD @../OniVCDirectories.txt /c
# SUBTRACT CPP /WX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "TEVCProj - Win32 Release"
# Name "TEVCProj - Win32 Debug"
# Begin Group "BungieFrameWork"

# PROP Default_Filter ""
# Begin Group "BFW_Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_FileManager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Headers\BFW_TemplateManager.h
# End Source File
# End Group
# Begin Group "BFW_Source"

# PROP Default_Filter ""
# Begin Group "BFW_Utility"

# PROP Default_Filter ""
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

SOURCE=\
..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\Platform_Win32\BFW_Platform_Win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_Utility\BFW_Utility.c
# End Source File
# End Group
# Begin Group "BFW_FileManager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=\
..\..\..\BungieFrameWork\BFW_Source\BFW_FileManager\BFW_FileManager_Common.c
# End Source File
# Begin Source File

SOURCE=\
..\..\..\BungieFrameWork\BFW_Source\BFW_FileManager\Platform_Win32\BFW_FileManager_Win32.c
# End Source File
# End Group
# Begin Group "BFW_TemplateManager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=\
..\..\..\BungieFrameWork\BFW_Source\BFW_TemplateManager\BFW_TemplateManager.c
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_TemplateManager\BFW_TM_Private.h
# End Source File
# End Group
# Begin Group "BFW_MathLib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_Source\BFW_MathLib\BFW_MathLib.c
# End Source File
# End Group
# End Group
# End Group
# Begin Group "TemplateExtractor"

# PROP Default_Filter ""
# Begin Source File

SOURCE=\
..\..\..\BungieFrameWork\BFW_ToolSource\Common\TemplateExtractor\TE_Parser.c
# End Source File
# Begin Source File

SOURCE=\
..\..\..\BungieFrameWork\BFW_ToolSource\Common\TemplateExtractor\TE_Parser.h
# End Source File
# Begin Source File

SOURCE=\
..\..\..\BungieFrameWork\BFW_ToolSource\Common\TemplateExtractor\TE_Private.h
# End Source File
# Begin Source File

SOURCE=\
..\..\..\BungieFrameWork\BFW_ToolSource\Common\TemplateExtractor\TE_Symbol.c
# End Source File
# Begin Source File

SOURCE=\
..\..\..\BungieFrameWork\BFW_ToolSource\Common\TemplateExtractor\TE_Symbol.h
# End Source File
# Begin Source File

SOURCE=\
..\..\..\BungieFrameWork\BFW_ToolSource\Common\TemplateExtractor\TemplateExtractor.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\OniVCDirectories.txt
# End Source File
# End Target
# End Project
