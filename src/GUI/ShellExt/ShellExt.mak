# Microsoft Developer Studio Generated NMAKE File, Based on ShellExt.dsp
!IF "$(CFG)" == ""
CFG=ShellExt - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ShellExt - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ShellExt - Win32 Release" && "$(CFG)" != "ShellExt - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ShellExt.mak" CFG="ShellExt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ShellExt - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ShellExt - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ShellExt - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\Win2fs.dll" "$(OUTDIR)\ShellExt.bsc"


CLEAN :
	-@erase "$(INTDIR)\CAboutDlg.obj"
	-@erase "$(INTDIR)\CAboutDlg.sbr"
	-@erase "$(INTDIR)\CGenericPage.obj"
	-@erase "$(INTDIR)\CGenericPage.sbr"
	-@erase "$(INTDIR)\CHyperLink.obj"
	-@erase "$(INTDIR)\CHyperLink.sbr"
	-@erase "$(INTDIR)\CWin2fsApp.obj"
	-@erase "$(INTDIR)\CWin2fsApp.sbr"
	-@erase "$(INTDIR)\CWin2fsDrivePage.obj"
	-@erase "$(INTDIR)\CWin2fsDrivePage.sbr"
	-@erase "$(INTDIR)\CWin2fsFilePage.obj"
	-@erase "$(INTDIR)\CWin2fsFilePage.sbr"
	-@erase "$(INTDIR)\CWin2fsFolderPage.obj"
	-@erase "$(INTDIR)\CWin2fsFolderPage.sbr"
	-@erase "$(INTDIR)\CWin2fsShellExt.obj"
	-@erase "$(INTDIR)\CWin2fsShellExt.sbr"
	-@erase "$(INTDIR)\Registry.obj"
	-@erase "$(INTDIR)\Registry.sbr"
	-@erase "$(INTDIR)\ShellExt.pch"
	-@erase "$(INTDIR)\ShellExt.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\ShellExt.bsc"
	-@erase "$(OUTDIR)\Win2fs.dll"
	-@erase "$(OUTDIR)\Win2fs.exp"
	-@erase "$(OUTDIR)\Win2fs.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ShellExt.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ShellExt.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ShellExt.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\CAboutDlg.sbr" \
	"$(INTDIR)\CGenericPage.sbr" \
	"$(INTDIR)\CWin2fsApp.sbr" \
	"$(INTDIR)\CWin2fsDrivePage.sbr" \
	"$(INTDIR)\CWin2fsFilePage.sbr" \
	"$(INTDIR)\CWin2fsFolderPage.sbr" \
	"$(INTDIR)\CWin2fsShellExt.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\CHyperLink.sbr" \
	"$(INTDIR)\Registry.sbr"

"$(OUTDIR)\ShellExt.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=shlwapi.lib /nologo /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\Win2fs.pdb" /machine:I386 /def:".\ShellExt.def" /out:"$(OUTDIR)\Win2fs.dll" /implib:"$(OUTDIR)\Win2fs.lib" 
DEF_FILE= \
	".\ShellExt.def"
LINK32_OBJS= \
	"$(INTDIR)\CAboutDlg.obj" \
	"$(INTDIR)\CGenericPage.obj" \
	"$(INTDIR)\CWin2fsApp.obj" \
	"$(INTDIR)\CWin2fsDrivePage.obj" \
	"$(INTDIR)\CWin2fsFilePage.obj" \
	"$(INTDIR)\CWin2fsFolderPage.obj" \
	"$(INTDIR)\CWin2fsShellExt.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\CHyperLink.obj" \
	"$(INTDIR)\Registry.obj" \
	"$(INTDIR)\ShellExt.res"

"$(OUTDIR)\Win2fs.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ShellExt - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\Win2fs.dll" "$(OUTDIR)\ShellExt.bsc"


CLEAN :
	-@erase "$(INTDIR)\CAboutDlg.obj"
	-@erase "$(INTDIR)\CAboutDlg.sbr"
	-@erase "$(INTDIR)\CGenericPage.obj"
	-@erase "$(INTDIR)\CGenericPage.sbr"
	-@erase "$(INTDIR)\CHyperLink.obj"
	-@erase "$(INTDIR)\CHyperLink.sbr"
	-@erase "$(INTDIR)\CWin2fsApp.obj"
	-@erase "$(INTDIR)\CWin2fsApp.sbr"
	-@erase "$(INTDIR)\CWin2fsDrivePage.obj"
	-@erase "$(INTDIR)\CWin2fsDrivePage.sbr"
	-@erase "$(INTDIR)\CWin2fsFilePage.obj"
	-@erase "$(INTDIR)\CWin2fsFilePage.sbr"
	-@erase "$(INTDIR)\CWin2fsFolderPage.obj"
	-@erase "$(INTDIR)\CWin2fsFolderPage.sbr"
	-@erase "$(INTDIR)\CWin2fsShellExt.obj"
	-@erase "$(INTDIR)\CWin2fsShellExt.sbr"
	-@erase "$(INTDIR)\Registry.obj"
	-@erase "$(INTDIR)\Registry.sbr"
	-@erase "$(INTDIR)\ShellExt.pch"
	-@erase "$(INTDIR)\ShellExt.res"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\StdAfx.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\ShellExt.bsc"
	-@erase "$(OUTDIR)\Win2fs.dll"
	-@erase "$(OUTDIR)\Win2fs.exp"
	-@erase "$(OUTDIR)\Win2fs.ilk"
	-@erase "$(OUTDIR)\Win2fs.lib"
	-@erase "$(OUTDIR)\Win2fs.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W4 /WX /Gm /GX /ZI /Od /I "..\..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ShellExt.pch" /Yu"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ShellExt.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ShellExt.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\CAboutDlg.sbr" \
	"$(INTDIR)\CGenericPage.sbr" \
	"$(INTDIR)\CWin2fsApp.sbr" \
	"$(INTDIR)\CWin2fsDrivePage.sbr" \
	"$(INTDIR)\CWin2fsFilePage.sbr" \
	"$(INTDIR)\CWin2fsFolderPage.sbr" \
	"$(INTDIR)\CWin2fsShellExt.sbr" \
	"$(INTDIR)\StdAfx.sbr" \
	"$(INTDIR)\CHyperLink.sbr" \
	"$(INTDIR)\Registry.sbr"

"$(OUTDIR)\ShellExt.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=shlwapi.lib /nologo /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\Win2fs.pdb" /debug /machine:I386 /def:".\ShellExt.def" /out:"$(OUTDIR)\Win2fs.dll" /implib:"$(OUTDIR)\Win2fs.lib" /pdbtype:sept 
DEF_FILE= \
	".\ShellExt.def"
LINK32_OBJS= \
	"$(INTDIR)\CAboutDlg.obj" \
	"$(INTDIR)\CGenericPage.obj" \
	"$(INTDIR)\CWin2fsApp.obj" \
	"$(INTDIR)\CWin2fsDrivePage.obj" \
	"$(INTDIR)\CWin2fsFilePage.obj" \
	"$(INTDIR)\CWin2fsFolderPage.obj" \
	"$(INTDIR)\CWin2fsShellExt.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\CHyperLink.obj" \
	"$(INTDIR)\Registry.obj" \
	"$(INTDIR)\ShellExt.res"

"$(OUTDIR)\Win2fs.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("ShellExt.dep")
!INCLUDE "ShellExt.dep"
!ELSE 
!MESSAGE Warning: cannot find "ShellExt.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ShellExt - Win32 Release" || "$(CFG)" == "ShellExt - Win32 Debug"
SOURCE=.\CAboutDlg.cpp

"$(INTDIR)\CAboutDlg.obj"	"$(INTDIR)\CAboutDlg.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ShellExt.pch"


SOURCE=.\CGenericPage.cpp

"$(INTDIR)\CGenericPage.obj"	"$(INTDIR)\CGenericPage.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ShellExt.pch"


SOURCE=.\CWin2fsApp.cpp

"$(INTDIR)\CWin2fsApp.obj"	"$(INTDIR)\CWin2fsApp.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ShellExt.pch"


SOURCE=.\CWin2fsDrivePage.cpp

"$(INTDIR)\CWin2fsDrivePage.obj"	"$(INTDIR)\CWin2fsDrivePage.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ShellExt.pch"


SOURCE=.\CWin2fsFilePage.cpp

"$(INTDIR)\CWin2fsFilePage.obj"	"$(INTDIR)\CWin2fsFilePage.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ShellExt.pch"


SOURCE=.\CWin2fsFolderPage.cpp

"$(INTDIR)\CWin2fsFolderPage.obj"	"$(INTDIR)\CWin2fsFolderPage.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ShellExt.pch"


SOURCE=.\CWin2fsShellExt.cpp

"$(INTDIR)\CWin2fsShellExt.obj"	"$(INTDIR)\CWin2fsShellExt.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ShellExt.pch"


SOURCE=.\ShellExt.rc

"$(INTDIR)\ShellExt.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "ShellExt - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /GX /O2 /I "..\..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ShellExt.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\ShellExt.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ShellExt - Win32 Debug"

CPP_SWITCHES=/nologo /MDd /W4 /WX /Gm /GX /ZI /Od /I "..\..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\ShellExt.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\StdAfx.sbr"	"$(INTDIR)\ShellExt.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\Other\CHyperLink.cpp

"$(INTDIR)\CHyperLink.obj"	"$(INTDIR)\CHyperLink.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ShellExt.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Other\Registry.cpp

"$(INTDIR)\Registry.obj"	"$(INTDIR)\Registry.sbr" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\ShellExt.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

