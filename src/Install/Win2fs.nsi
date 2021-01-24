
 ;/////////////////////////////////////////////////////////////////////////////
 ;
 ;    NSIS Script For Win2fs.
 ;
 ;    Copyright (c) 2001 - 2005 Satish Kumar J (vsat_in@yahoo.com)
 ;
 ;    Project:      Win2fs
 ;
 ;    Module Name:  Win2fs.nsi
 ;
 ;    Abstract:     NSIS based Win2fs Installer script.
 ;
 ;    Notes:
 ;
 ;      o Remember to update the top level defines to suit your local config.
 ;
 ;    Revision History:
 ;
 ;    Date    Version    Author        Changes
 ;    -------------------------------------------------------------------------
 ;
 ;            0.0.1      Satish Kumar  Initial Version
 ;
 ;/////////////////////////////////////////////////////////////////////////////

 !define VERSION                    "0.0.1"
 !define BINDIR                     "E:\Projects\Win2fs\Bin"
 
 !define S_NOSUPPORT                "This version of Windows is not currently supported."
 !define S_COMPLETED                "Install Completed."
 !define S_UNINSTALL                "This will uninstall Win2fs from your system."
 !define S_REVIEWLICENSE            "Please review the following license agreement before proceeding."
 !define S_RESTART                  "It is highly recommended you restart your computer to complete the uninstall.$\n$\nDo you want to restart your computer now?"
 !define S_MUSTRESTART              "You must restart your computer for the changes to take effect.$\n$\nDo you want to restart your computer now?"

 Name                               "Win2fs"                                             ; Title Of Your Application
 BGGradient                         9995C4 000000 FFFFFF                                 ; Backgound Colors
 BrandingText                       /TRIMCENTER "Win2fs Version ${VERSION}"
 CRCCheck                           On                                                   ; Do A CRC Check
 InstProgressFlags                  smooth                                               ; Let there be smooth progress bars.
 
 LicenseData                        "${BINDIR}\GPL.TXT"                                  ; License Data
 OutFile                            "${BINDIR}\Setup.exe"                                ; Output File Name
 InstallDir                         "$PROGRAMFILES\Win2fs"                               ; The Default Installation Directory

 CompletedText                      "${S_COMPLETED}"                                     ; The text shown to indicate that setup has finished.
 UninstallText                      "${S_UNINSTALL}"
 LicenseText                        "${S_REVIEWLICENSE}"

 Section "Install"

         ; We should exit 'gracefully'.
         SetAutoClose False

         ; Activate the compression feature.
         SetCompress Auto

         ; Copy files only if they are newer.
         SetOverwrite IfNewer

         ; Identify the OS.
         call GetWindowsVersion
         DetailPrint "System: $1"

         ; Set the default folder where the files will be unpacked.
         SetOutPath $INSTDIR

         ; We support only 2000 & XP for now.
         StrCmp $0 '2000' Win2000
         StrCmp $0 'XP'   WinXP   WinNotSupported

         WinXP:
         Win2000:

                 ; Do Windows 2000+ specific stuff.
                 File ".\Win2fs.inf"
                 File "${BINDIR}\Release\Win2fs.sys"
                 File "${BINDIR}\Release\Win2fs.dll"
                 File "${BINDIR}\Release\Win2fs.cpl"
                 File "${BINDIR}\Release\Win2fs.exe"
                 
                 ; Install the driver - the Windows way.
                 DetailPrint "Installing the Filesystem driver(s) ... "
                 ExecWait "rundll32.exe setupapi.dll,InstallHinfSection DefaultInstall 132 $INSTDIR\Win2fs.inf"
                 
                 ; Install the Control Panel applet.
                 CopyFiles $INSTDIR\Win2fs.cpl $SYSDIR
                 
                 ; Cleanup quickly.
                 Delete "$INSTDIR\*.sys"
                 Delete "$INSTDIR\*.cpl"
                 ; Delete "$INSTDIR\*.inf" // We need this while uninstalling, so leave it there.
                 
                 ; Regsiter the shell extensions.
                 RegDLL $INSTDIR\Win2fs.dll

                 ; Write registry keys.
                 WriteRegStr HKLM "Software\Win2fs" "InstallDir" "$INSTDIR"

                 ; Write the uninstall keys for Windows
                 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Win2fs" "DisplayName" "Win2fs"
                 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Win2fs" "UninstallString" "$INSTDIR\Uninst.exe"
                 WriteUninstaller "Uninst.exe"
                 
                 ; Reboot if fine.
                 MessageBox MB_YESNO|MB_ICONQUESTION "${S_MUSTRESTART}" IDNO +2
                 Reboot

                 Goto Finish
   
        WinNotSupported:

                ; We are running on an Unsupported platform.
                MessageBox MB_OK|MB_ICONSTOP|MB_TOPMOST|MB_SETFOREGROUND "${S_NOSUPPORT}"
                Abort
   
        Finish:

 SectionEnd

 Section Uninstall
 
        ; Unregsiter the shell extensions.
        UnRegDLL $INSTDIR\Win2fs.dll

        ; Uninstall the driver.
        DetailPrint "Uninstalling the driver ..."
        ExecWait "rundll32.exe setupapi.dll,InstallHinfSection DefaultUninstall 132 $INSTDIR\Win2fs.inf"
        
        ; Delete the Control Panel applet.
        Delete "$SYSDIR\Win2fs.cpl"

        ; Delete Uninstaller And Unistall Registry Entries.
        Delete "$INSTDIR\Uninst.exe"
        
        ; Registry cleanup.
        DeleteRegKey HKLM "SOFTWARE\Win2fs"
        DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Win2fs"

        ; Remove the directory itself.
        RMDir /r "$INSTDIR"

        ; Reboot if fine.
        MessageBox MB_YESNO|MB_ICONQUESTION "${S_RESTART}" IDNO +2
        Reboot

 SectionEnd

 ;/////////////////////////////////////////////////////////////////////////////
 ;
 ; Helper functions.
 ;
 ;/////////////////////////////////////////////////////////////////////////////
 ; GetWindowsVersion
 ;
 ; Returns:
 ;
 ; $0 = Windows Version (One of 95, 98, ME, NT x.x, 2000, XP)
 ; $1 = Microsoft Windows 2000 Service Pack 1 (Version String)
 ;              or
 ; $0 = ''
 ; $1 = '' (No OS Info).
 ;
 ; Uses: $0, $1, $2, $9.
 ;
 ;/////////////////////////////////////////////////////////////////////////////

 Function GetWindowsVersion

          ; Are we running on an NT clone ?
          ReadRegStr $1 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" ProductName

          StrCmp $1 "" 0 WinNT

          ; We are not on NT.

          ; Read the 'Product Name' into $2.
          ReadRegStr $1 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion" ProductName

          ; Read the Version Info into $0.
          ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion" VersionNumber

          ; Copy 1 char from $0 to $9.
          StrCpy $9 $0 1

          ; If its not '4', we are in trouble.
          StrCmp $9 '4' 0 Error

          ; Copy 3 chars from $0 to $9.
          StrCpy $9 $0 3

          ; 4.0 => Windows 95
          StrCmp $9 '4.0' Win95

          ; > 4.0 <= 4.9 => Windows 98
          ; > 4.10 => Windows Me
          StrCmp $9 '4.9' WinME Win98

          Win95:

                StrCpy $0 '95'
                Goto Done

          Win98:

                StrCpy $0 '98'
                Goto Done

         WinME:

                StrCpy $0 'ME'
                Goto Done

         WinNT:

                ; We are running on some NT clone.
                ; Read the current version into $0.
                ReadRegStr $0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion

                StrCpy $9 $0 1

                ; This is the ext. OS info.
                ReadRegStr $2 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CSDVersion

                ; 3.X, 4.X => Windows NT
                StrCmp $9 '3' WinNT_X
                StrCmp $9 '4' WinNT_X

                ; 5.0 => Windows 2000
                StrCmp $0 '5.0' Win2000

                ; 5.1 => Windows XP.
                StrCmp $0 '5.1' WinXP Error

        WinNT_X:

                StrCpy $0 "NT $0" 7
                StrCmp $2 "" 0 DoExt
                Goto Done

        Win2000:

                Strcpy $0 '2000'
                StrCmp $2 "" 0 DoExt
                Goto Done

        WinXP:

                Strcpy $0 'XP'
                StrCmp $2 "" 0 DoExt
                Goto Done

        Error:

                ; We cudn't determine the windows version.
                Strcpy $0 ''
                Strcpy $1 ''
                Goto Done

        DoExt:

                StrCpy $9 $1
                StrCpy $1 "$9 ($2)"

        Done:

 FunctionEnd

 ;/////////////////////////////////////////////////////////////////////////////