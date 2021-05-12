# Microsoft Developer Studio Project File - Name="ModelExporter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ModelExporter - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ModelExporter.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ModelExporter.mak" CFG="ModelExporter - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ModelExporter - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ModelExporter - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ModelExporter - Win32 Hybrid" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/BungieSource/BungieFrameWork/BFW_ToolSource/Win32/MaxModelExporter/ModelExporter", HPAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ModelExporter - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Zp4 /MT /W3 /GX /Zi /O2 /I "d:\maxsdk\include" /FI"Max.h" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "MODEL_EXPORTER" /FR /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /out:"d:\3dsmax2\plugins\ModelExporter.dle"

!ELSEIF  "$(CFG)" == "ModelExporter - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Zp4 /MTd /W3 /Gm /GX /ZI /Od /I "d:\maxsdk\include" /FI"Max.h" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D DEBUGGING=1 /D "MODEL_EXPORTER" /FR /FD @../OniVCDirectories.txt /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /profile /debug /machine:I386 /out:"d:\3dsmax2\plugins\ModelExporter.dle"

!ELSEIF  "$(CFG)" == "ModelExporter - Win32 Hybrid"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Hybrid"
# PROP BASE Intermediate_Dir ".\Hybrid"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Hybrid"
# PROP Intermediate_Dir ".\Hybrid"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Zp4 /MTd /W3 /Gm /GX /ZI /Od /I "d:\maxsdk\include" /FI"BFW.h" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D DEBUGGING=1 /FR /YX"BFW.h" /FD /c
# SUBTRACT CPP /WX
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"d:\3dsmax2\plugins\ModelExporter.dle"

!ENDIF 

# Begin Target

# Name "ModelExporter - Win32 Release"
# Name "ModelExporter - Win32 Debug"
# Name "ModelExporter - Win32 Hybrid"
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Win32\ModelExporter\EnvFileFormat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Win32\ModelExporter\ExportMain.h
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Win32\ModelExporter\ExportTools.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Win32\ModelExporter\ModelExporter.def
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Win32\ModelExporter\resource.h
# End Source File
# End Group
# Begin Group "Libraries"

# PROP Default_Filter "lib;LIB"
# Begin Source File

SOURCE=D:\3DSMAX2\MAXSDK\LIB\CORE.LIB
# End Source File
# Begin Source File

SOURCE=D:\3DSMAX2\MAXSDK\LIB\GEOM.LIB
# End Source File
# Begin Source File

SOURCE=D:\3DSMAX2\MAXSDK\LIB\MESH.LIB
# End Source File
# Begin Source File

SOURCE=D:\3DSMAX2\MAXSDK\LIB\UTIL.LIB
# End Source File
# End Group
# Begin Group "MaxHeaders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\ACOLOR.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\ANIMTBL.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\ASSERT1.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\BEZFONT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\BITARRAY.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\BITMAP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\BMMLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\BOX2.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\BOX3.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\BUILDVER.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\CAPTYPES.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\CHANNELS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\CLIENT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\CMDMODE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\COLOR.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\CONTROL.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\COREEXP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\COREGEN.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\CUSTCONT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\DBGPRINT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\DECOMP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\DPOINT3.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\DRIVER.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\DUMMY.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\EVROUTER.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\EVUSER.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\EXPORT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\EXPR.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\EXPRLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\FBWIN.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\FILTERS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\FLTAPI.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\FLTLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GAMMA.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GCOMM.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GCOMMLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GENCAM.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GENHIER.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GENLIGHT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GENSHAPE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GEOM.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GEOMLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GFX.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GFXLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\GPORT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\HITDATA.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\HOLD.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\HOTCHECK.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\HSV.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\IMPAPI.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\IMPEXP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\IMTL.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\INODE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\INTERPIK.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\INTERVAL.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\IOAPI.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\IPARAMB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\IPARAMM.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\IPOINT2.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\IPOINT3.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\ISTDPLUG.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\KEYREDUC.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\LINKLIST.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\LINSHAPE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\LOCKID.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MATRIX2.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MATRIX3.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MATRIX33.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MAX.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MAXAPI.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MAXCOM.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MAXTYPES.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MESH.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MESHADJ.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MESHLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MODSTACK.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MOUSEMAN.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\MTL.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\NAMETAB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\NOTETRCK.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\NOTIFY.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\OBJECT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\OBJMODE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\PARTCLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\PARTICLE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\PATCH.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\PATCHCAP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\PATCHLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\PATCHOBJ.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\PIXELBUF.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\PLUGAPI.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\PLUGIN.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\POINT2.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\POINT3.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\POLYSHP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\PROPS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\PTRVEC.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\QUAT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\REF.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\RENDER.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\RTCLICK.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SCENEAPI.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SHAPE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SHPHIER.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SHPSELS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SIMPMOD.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SIMPOBJ.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SIMPSHP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SIMPSPL.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SNAP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SOUNDOBJ.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SPLINE3D.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\SPLSHAPE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\STACK.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\STACK3.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\STDMAT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\STRBASIC.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\STRCLASS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\TAB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\TCBGRAPH.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\TCP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\TEMPLT.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\TEXUTIL.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\TRIG.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\TRIOBJ.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\UNITS.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\UTILAPI.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\UTILEXP.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\UTILLIB.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\VEDGE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\VIEWFILE.H
# End Source File
# Begin Source File

SOURCE=..\..\..\..\ExternalLibraries\Win32\MAXSDK\INCLUDE\WINUTIL.H
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Win32\ModelExporter\AnimationExporter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Win32\ModelExporter\EnvironmentExporter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Win32\ModelExporter\ExportMain.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\BungieFrameWork\BFW_ToolSource\Win32\ModelExporter\ExportTools.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\OniVCDirectories.txt
# End Source File
# End Target
# End Project
