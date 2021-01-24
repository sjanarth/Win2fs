# Microsoft Developer Studio Generated NMAKE File, Based on MainApp.dsp
!IF "$(CFG)" == ""
CFG=MainApp - Win32 Debug
!MESSAGE No configuration specified. Defaulting to MainApp - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "MainApp - Win32 Release" && "$(CFG)" != "MainApp - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MainApp.mak" CFG="MainApp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MainApp - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "MainApp - Win32 Debug" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "MainApp - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\MainApp.exe" "$(OUTDIR)\MainApp.bsc"


CLEAN :
	-@erase "$(INTDIR)\CDrvSelDlg.obj"
	-@erase "$(INTDIR)\CDrvSelDlg.sbr"
	-@erase "$(INTDIR)\CInfoDlg.obj"
	-@erase "$(INTDIR)\CInfoDlg.sbr"
	-@erase "$(INTDIR)\CMainApp.obj"
	-@erase "$(INTDIR)\CMainApp.sbr"
	-@erase "$(INTDIR)\CMainDlg.obj"
	-@erase "$(INTDIR)\CMainDlg.sbr"
	-@erase "$(INTDIR)\MainApp.res"
	-@erase "$(INTDIR)\Utils.obj"
	-@erase "$(INTDIR)\Utils.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MainApp.bsc"
	-@erase "$(OUTDIR)\MainApp.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_AFXDLL" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\MainApp.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MainApp.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MainApp.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\CInfoDlg.sbr" \
	"$(INTDIR)\CMainApp.sbr" \
	"$(INTDIR)\CMainDlg.sbr" \
	"$(INTDIR)\Utils.sbr" \
	"$(INTDIR)\CDrvSelDlg.sbr"

"$(OUTDIR)\MainApp.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\MainApp.pdb" /machine:I386 /out:"$(OUTDIR)\MainApp.exe" 
LINK32_OBJS= \
	"$(INTDIR)\CInfoDlg.obj" \
	"$(INTDIR)\CMainApp.obj" \
	"$(INTDIR)\CMainDlg.obj" \
	"$(INTDIR)\MainApp.res" \
	"$(INTDIR)\Utils.obj" \
	"$(INTDIR)\CDrvSelDlg.obj"

"$(OUTDIR)\MainApp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "MainApp - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\MainApp.exe" "$(OUTDIR)\MainApp.bsc"


CLEAN :
	-@erase "$(INTDIR)\CDrvSelDlg.obj"
	-@erase "$(INTDIR)\CDrvSelDlg.sbr"
	-@erase "$(INTDIR)\CInfoDlg.obj"
	-@erase "$(INTDIR)\CInfoDlg.sbr"
	-@erase "$(INTDIR)\CMainApp.obj"
	-@erase "$(INTDIR)\CMainApp.sbr"
	-@erase "$(INTDIR)\CMainDlg.obj"
	-@erase "$(INTDIR)\CMainDlg.sbr"
	-@erase "$(INTDIR)\MainApp.res"
	-@erase "$(INTDIR)\Utils.obj"
	-@erase "$(INTDIR)\Utils.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\MainApp.bsc"
	-@erase "$(OUTDIR)\MainApp.exe"
	-@erase "$(OUTDIR)\MainApp.ilk"
	-@erase "$(OUTDIR)\MainApp.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\MainApp.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\MainApp.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\MainApp.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\CInfoDlg.sbr" \
	"$(INTDIR)\CMainApp.sbr" \
	"$(INTDIR)\CMainDlg.sbr" \
	"$(INTDIR)\Utils.sbr" \
	"$(INTDIR)\CDrvSelDlg.sbr"

"$(OUTDIR)\MainApp.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\MainApp.pdb" /debug /machine:I386 /nodefaultlib:"msvcrtd.lib mfc42d.lib" /out:"$(OUTDIR)\MainApp.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\CInfoDlg.obj" \
	"$(INTDIR)\CMainApp.obj" \
	"$(INTDIR)\CMainDlg.obj" \
	"$(INTDIR)\MainApp.res" \
	"$(INTDIR)\Utils.obj" \
	"$(INTDIR)\CDrvSelDlg.obj"

"$(OUTDIR)\MainApp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("MainApp.dep")
!INCLUDE "MainApp.dep"
!ELSE 
!MESSAGE Warning: cannot find "MainApp.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "MainApp - Win32 Release" || "$(CFG)" == "MainApp - Win32 Debug"
SOURCE=.\CDrvSelDlg.cpp

"$(INTDIR)\CDrvSelDlg.obj"	"$(INTDIR)\CDrvSelDlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CInfoDlg.cpp

"$(INTDIR)\CInfoDlg.obj"	"$(INTDIR)\CInfoDlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CMainApp.cpp

"$(INTDIR)\CMainApp.obj"	"$(INTDIR)\CMainApp.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CMainDlg.cpp

"$(INTDIR)\CMainDlg.obj"	"$(INTDIR)\CMainDlg.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\MainApp.rc

"$(INTDIR)\MainApp.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\Utils.cpp

"$(INTDIR)\Utils.obj"	"$(INTDIR)\Utils.sbr" : $(SOURCE) "$(INTDIR)"



!ENDIF 

