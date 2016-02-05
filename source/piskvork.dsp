# Microsoft Developer Studio Project File - Name="piskvork" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=piskvork - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "piskvork.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "piskvork.mak" CFG="piskvork - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "piskvork - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "piskvork - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "piskvork - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /O1 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "HAVE_UNZIP" /Yu"hdr.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /d "NDEBUG"
# ADD RSC /l 0x405 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 wsock32.lib mswsock.lib version.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib winmm.lib /nologo /subsystem:windows /map /machine:I386 /out:"..\piskvork.exe"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=c:\_petr\cw\hotkeyp\hotkeyp.exe -close window Piskvork.exe
# End Special Build Tool

!ELSEIF  "$(CFG)" == "piskvork - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /Fr /Yu"hdr.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /d "_DEBUG"
# ADD RSC /l 0x405 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 mswsock.lib wsock32.lib version.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"..\piskvorkDBG.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=c:\_petr\cw\hotkeyp\hotkeyp.exe -close window Piskvorkdbg.exe
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "piskvork - Win32 Release"
# Name "piskvork - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\game.cpp

!IF  "$(CFG)" == "piskvork - Win32 Release"

# ADD CPP /Yu"hdr.h"

!ELSEIF  "$(CFG)" == "piskvork - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lang.cpp
# ADD CPP /Yc"hdr.h"
# End Source File
# Begin Source File

SOURCE=.\netgame.cpp

!IF  "$(CFG)" == "piskvork - Win32 Release"

# ADD CPP /Yu"hdr.h"

!ELSEIF  "$(CFG)" == "piskvork - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\nettur.cpp

!IF  "$(CFG)" == "piskvork - Win32 Release"

# ADD CPP /Yu"hdr.h"

!ELSEIF  "$(CFG)" == "piskvork - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PISKVORK.cpp

!IF  "$(CFG)" == "piskvork - Win32 Release"

# ADD CPP /Yu"hdr.h"

!ELSEIF  "$(CFG)" == "piskvork - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Piskvork.rc
# End Source File
# Begin Source File

SOURCE=.\protocol.cpp

!IF  "$(CFG)" == "piskvork - Win32 Release"

# ADD CPP /Yu"hdr.h"

!ELSEIF  "$(CFG)" == "piskvork - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\renju.cpp
# End Source File
# Begin Source File

SOURCE=.\taskbarprogress.cpp

!IF  "$(CFG)" == "piskvork - Win32 Release"

# ADD CPP /Yu

!ELSEIF  "$(CFG)" == "piskvork - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\hdr.h
# End Source File
# Begin Source File

SOURCE=.\lang.h
# End Source File
# Begin Source File

SOURCE=.\piskvork.h
# End Source File
# Begin Source File

SOURCE=.\taskbarprogress.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\PISKVORK.ICO
# End Source File
# Begin Source File

SOURCE=..\skins\pisq.bmp
# End Source File
# Begin Source File

SOURCE=.\toolbar.bmp
# End Source File
# End Group
# End Target
# End Project
