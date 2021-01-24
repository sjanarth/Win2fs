# Microsoft Developer Studio Project File - Name="Win2fs" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Win2fs - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Win2fs.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Win2fs.mak" CFG="Win2fs - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win2fs - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Win2fs - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Win2fs - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f Win2fs.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Win2fs.exe"
# PROP BASE Bsc_Name "Win2fs.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "ddkbuild -W2K free ."
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2fs.sys"
# PROP Bsc_Name ".\Win2fs.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Win2fs - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f Win2fs.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Win2fs.exe"
# PROP BASE Bsc_Name "Win2fs.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "ddkbuild -W2K checked ."
# PROP Rebuild_Opt "-c"
# PROP Target_File "Win2fs.sys"
# PROP Bsc_Name ".\Win2fs.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Win2fs - Win32 Release"
# Name "Win2fs - Win32 Debug"

!IF  "$(CFG)" == "Win2fs - Win32 Release"

!ELSEIF  "$(CFG)" == "Win2fs - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\block.c
# End Source File
# Begin Source File

SOURCE=.\cleanup.c
# End Source File
# Begin Source File

SOURCE=.\close.c
# End Source File
# Begin Source File

SOURCE=.\cmcb.c
# End Source File
# Begin Source File

SOURCE=.\create.c
# End Source File
# Begin Source File

SOURCE=.\debug.c
# End Source File
# Begin Source File

SOURCE=.\devctl.c
# End Source File
# Begin Source File

SOURCE=.\dirctl.c
# End Source File
# Begin Source File

SOURCE=.\dispatch.c
# End Source File
# Begin Source File

SOURCE=.\except.c
# End Source File
# Begin Source File

SOURCE=.\ext2.c
# End Source File
# Begin Source File

SOURCE=.\fastio.c
# End Source File
# Begin Source File

SOURCE=.\fileinfo.c
# End Source File
# Begin Source File

SOURCE=.\flush.c
# End Source File
# Begin Source File

SOURCE=.\fsctl.c
# End Source File
# Begin Source File

SOURCE=.\init.c
# End Source File
# Begin Source File

SOURCE=.\lock.c
# End Source File
# Begin Source File

SOURCE=.\memory.c
# End Source File
# Begin Source File

SOURCE=.\read.c
# End Source File
# Begin Source File

SOURCE=.\shutdown.c
# End Source File
# Begin Source File

SOURCE=.\util.c
# End Source File
# Begin Source File

SOURCE=.\volinfo.c
# End Source File
# Begin Source File

SOURCE=.\Win2fs.rc
# End Source File
# Begin Source File

SOURCE=.\write.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
