
 /*                                                                            
  *		Copyright (c) 2001 - 2003 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\CPL\CPL.cpp
  *                                                                            
  *		Abstract:		The Win2fs Control Panel Applet.
  *
  *		Environment:	Win32 DLL operating from User space.
  *	
  *		Notes:			None  
  *
  *		Revision History:
  *
  *		Date		Version		Author				Change Log
  *		------------------------------------------------------------------------
  *
  *		03-APR-02	0.0.1		Satish Kumar J		Initial Version
  */                          		

 /////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include <stdio.h>  
 #include <windows.h>

 #include "CPLApp.h"
 #include "resource.h"

 /////////////////////////////////////////////////////////////////////////////

 // Global declarations.

 #define STR_REGKEY			TEXT("SOFTWARE\\Win2fs")
 #define STR_INSTALLDIR		TEXT("InstallDir")
 #define STR_BINFILENAME	TEXT("\\Win2fs.exe")
 #define STR_ERRORMSG		TEXT("Error retrieving installation directory.")

 class CWin2fsApplet : public CCPLEx 
 {
    public :

        CWin2fsApplet () : CCPLEx (IDI_ICON, IDM_NAMESTRING, IDM_DESCSTRING) {}

        BOOL OnDoubleClick (HWND hWnd, LONG appletData);      
 };

 CWin2fsApplet theApplet;

 /////////////////////////////////////////////////////////////////////////////

 BOOL 
	CWin2fsApplet::OnDoubleClick ( 
		HWND hWnd, 
		LONG appletData	
	)
 {
    HKEY	hKey;
    DWORD	dwRetLen;
	DWORD	dwRet;
    CHAR	pszInstallDir[256];

	hKey = NULL;
	dwRetLen = 0;
	dwRet = 0;
	ZeroMemory ( pszInstallDir, sizeof ( pszInstallDir ) );

	do
	{
		dwRet = RegOpenKeyEx ( HKEY_LOCAL_MACHINE, 
							   STR_REGKEY, 
							   0, 
							   KEY_QUERY_VALUE, 
							   &hKey );  

		if ( ERROR_SUCCESS != dwRet )
		{
			MessageBox ( 0, STR_ERRORMSG, 0, 0 );  

			break;
		}

        dwRetLen = 256;
		dwRet = RegQueryValueEx ( hKey, 
								  STR_INSTALLDIR,
								  NULL, 
								  NULL, 
								  (LPBYTE) pszInstallDir, 
								  &dwRetLen );    

		RegCloseKey ( hKey );

		if ( ERROR_SUCCESS != dwRet )
		{
			MessageBox ( 0, STR_ERRORMSG, 0, 0 );  

			break;
		}

		strcat ( pszInstallDir, STR_BINFILENAME );

        WinExec ( pszInstallDir, 1 );

    } while ( FALSE );
   
	return TRUE;
 }

 /////////////////////////////////////////////////////////////////////////////
