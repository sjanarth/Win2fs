# Microsoft Developer Studio Generated NMAKE File, Based on CPL.dsp
!IF "$(CFG)" == ""
CFG=CPL - Win32 Debug
!MESSAGE No configuration specified. Defaulting to CPL - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "CPL - Win32 Release" && "$(CFG)" != "CPL - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CPL.mak" CFG="CPL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CPL - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "CPL - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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

!IF  "$(CFG)" == "CPL - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\Win2fs.cpl" "$(OUTDIR)\CPL.bsc"


CLEAN :
	-@erase "$(INTDIR)\CPL.obj"
	-@erase "$(INTDIR)\CPL.res"
	-@erase "$(INTDIR)\CPL.sbr"
	-@erase "$(INTDIR)\CPLApp.obj"
	-@erase "$(INTDIR)\CPLApp.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\CPL.bsc"
	-@erase "$(OUTDIR)\Win2fs.cpl"
	-@erase "$(OUTDIR)\Win2fs.exp"
	-@erase "$(OUTDIR)\Win2fs.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CPL_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\CPL.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\CPL.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\CPL.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\CPL.sbr" \
	"$(INTDIR)\CPLApp.sbr"

"$(OUTDIR)\CPL.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\Win2fs.pdb" /machine:I386 /def:".\CPLEx.def" /out:"$(OUTDIR)\Win2fs.cpl" /implib:"$(OUTDIR)\Win2fs.lib" 
DEF_FILE= \
	".\CPLEx.def"
LINK32_OBJS= \
	"$(INTDIR)\CPL.obj" \
	"$(INTDIR)\CPLApp.obj" \
	"$(INTDIR)\CPL.res"

"$(OUTDIR)\Win2fs.cpl" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "CPL - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\Win2fs.cpl" "$(OUTDIR)\CPL.bsc"


CLEAN :
	-@erase "$(INTDIR)\CPL.obj"
	-@erase "$(INTDIR)\CPL.res"
	-@erase "$(INTDIR)\CPL.sbr"
	-@erase "$(INTDIR)\CPLApp.obj"
	-@erase "$(INTDIR)\CPLApp.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\CPL.bsc"
	-@erase "$(OUTDIR)\Win2fs.cpl"
	-@erase "$(OUTDIR)\Win2fs.exp"
	-@erase "$(OUTDIR)\Win2fs.ilk"
	-@erase "$(OUTDIR)\Win2fs.lib"
	-@erase "$(OUTDIR)\Win2fs.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CPL_EXPORTS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\CPL.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\CPL.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\CPL.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\CPL.sbr" \
	"$(INTDIR)\CPLApp.sbr"

"$(OUTDIR)\CPL.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\Win2fs.pdb" /debug /machine:I386 /def:".\CPLEx.def" /out:"$(OUTDIR)\Win2fs.cpl" /implib:"$(OUTDIR)\Win2fs.lib" /pdbtype:sept 
DEF_FILE= \
	".\CPLEx.def"
LINK32_OBJS= \
	"$(INTDIR)\CPL.obj" \
	"$(INTDIR)\CPLApp.obj" \
	"$(INTDIR)\CPL.res"

"$(OUTDIR)\Win2fs.cpl" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("CPL.dep")
!INCLUDE "CPL.dep"
!ELSE 
!MESSAGE Warning: cannot find "CPL.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "CPL - Win32 Release" || "$(CFG)" == "CPL - Win32 Debug"
SOURCE=.\CPL.cpp

"$(INTDIR)\CPL.obj"	"$(INTDIR)\CPL.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CPLApp.cpp

"$(INTDIR)\CPLApp.obj"	"$(INTDIR)\CPLApp.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CPL.rc

"$(INTDIR)\CPL.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

