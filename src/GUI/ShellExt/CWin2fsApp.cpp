 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CWin2fsApp.cpp
  *
  *     Abstract:		The Application class.
  *
  *     Author:		    Satish Kumar J
  *
  *		Date:			20-NOV-2005
  *
  *		Notes:			
  *
  *			If this DLL is dynamically linked against the MFC
  *			DLLs, any functions exported from this DLL which
  *			call into MFC must have the AFX_MANAGE_STATE macro
  *			added at the very beginning of the function.
  *
  *			For example:
  *
  *			extern "C" BOOL PASCAL EXPORT ExportedFunction()
  *			{
  *				AFX_MANAGE_STATE(AfxGetStaticModuleState());
  *				// normal function body here
  *			}
  *
  *			It is very important that this macro appear in each
  *			function, prior to any calls into MFC.  This means that
  *			it must appear as the first statement within the 
  *			function, even before any object variable declarations
  *			as their constructors may generate calls into the MFC
  *			DLL.
  *
  *			Please see MFC Technical Notes 33 and 58 for additional
  *			details.
  *
  *		Revision History:
  *
  *		Date		Version	Author				Changes
  *		------------------------------------------------------------------------
  *
  *		20-NOV-05	1.00	Satish Kumar J		Initial	Version.
  *
  *
  */

 //////////////////////////////////////////////////////////////////////////////

 // Library includes.

 #include "stdafx.h"

 //////////////////////////////////////////////////////////////////////////////

 // Private includes.

 #include "CWin2fsApp.h"
 #include "CWin2fsShellExt.h"

 #include "..\..\Other\Registry.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global data.

 #define IS_WINNT() (!(GetVersion() & 0x80000000))

 BEGIN_MESSAGE_MAP(CWin2fsApp, CWinApp)
 END_MESSAGE_MAP()

 //////////////////////////////////////////////////////////////////////////////

 // Static data.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Restricted functions.

 // Privates'.

 // None.

 // Protected.

 // None.
 
 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 CWin2fsApp::CWin2fsApp()
 {
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
 }

 BOOL CWin2fsApp::InitInstance()
 {
	// Register all OLE server (factories) as running.  This enables the
	//  OLE libraries to create objects from other applications.
	COleObjectFactory::RegisterAll();

	return TRUE;
 }
	
 /////////////////////////////////////////////////////////////////////////////
 // The one and only CWin2fsApp object

 CWin2fsApp theApp;

 /////////////////////////////////////////////////////////////////////////////
 //
 // Special entry points required for inproc servers

 STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
 {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return AfxDllGetClassObject(rclsid, riid, ppv);
 }

 STDAPI DllCanUnloadNow(void)
 {
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return AfxDllCanUnloadNow();
 }

 STDAPI DllRegisterServer()
 {
	AFX_MANAGE_STATE( AfxGetStaticModuleState());

	HRESULT hRes = RegisterShellExtension<CWin2fsShellExt> ();
	
	return hRes;
 }

 STDAPI DllUnregisterServer()
 {
	AFX_MANAGE_STATE( AfxGetStaticModuleState());

	HRESULT hRes = UnregisterShellExtension<CWin2fsShellExt> ();
	
	return hRes;
 }

 /////////////////////////////////////////////////////////////////////////////

 HRESULT RegisterShellExtension( REFGUID guidClass )
 {
	// Register the server first.
	CString strGUID;
	{
		OLECHAR pszGUID[40];
		StringFromGUID2( guidClass, pszGUID, ARRAYSIZE(pszGUID));
		strGUID = pszGUID;
	}

	// Get path to server.
	CString strServerPath;
	AfxGetModuleShortFileName( AfxGetInstanceHandle(), strServerPath );

	CRegString hServer1 (
			HKEY_CLASSES_ROOT,
			_T("CLSID\\") + strGUID ,
			""
		);
	hServer1 = _T ("Win2fs Shell Extension.");

	CRegString hServer2 (
			HKEY_CLASSES_ROOT,
			_T("CLSID\\") + strGUID + _T("\\InProcServer32"),
			""
		);
	hServer2 = strServerPath;

	CRegString hServer3 (
			HKEY_CLASSES_ROOT,
			_T("CLSID\\") + strGUID + _T("\\InProcServer32"),
			_T("\\ThreadingModel")
		);
	hServer3 = _T("Apartment");

	// Add to approved list of extensions under NT
	if(IS_WINNT())
	{
		CRegString hApproved (
			HKEY_LOCAL_MACHINE,
			_T("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved\\"),
			strGUID
		);
		hApproved = _T("Win2fs shell extensions");
	}

	// Register the extensions.
	CString sPropSheetEx = _T("\\shellex\\PropertySheetHandlers\\") + strGUID;
	PCHAR str[] = { _T("*"), _T("Drive"), _T("Folder"), NULL };
	for (INT i = 0; str[i] != NULL; i++)
	{
		CRegString hRS (
				HKEY_CLASSES_ROOT,
				str[i]+sPropSheetEx,
				""
			);
		hRS = _T("Win2fs shell extension");
	}

	return S_OK;
 }
 
 HRESULT UnregisterShellExtension( REFGUID guidClass )
 {
	// Register the server first.
	CString strGUID;
	{
		OLECHAR pszGUID[40];
		StringFromGUID2( guidClass, pszGUID, ARRAYSIZE(pszGUID));
		strGUID = pszGUID;
	}

	// The server itself.
	CRegString hServer (
			HKEY_CLASSES_ROOT,
			_T("CLSID\\") + strGUID,
			""
		);
	hServer.RemoveKey ();

	// Remove from the apporved list.
	if(IS_WINNT())
	{
		CRegString hApproved (
			HKEY_LOCAL_MACHINE,
			_T("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved\\"),
			strGUID
		);
		hApproved.RemoveValue ();
	}

	// Register the extensions.
	CString sPropSheetEx = _T("\\shellex\\PropertySheetHandlers\\") + strGUID ;
	PCHAR str[] = { _T("*"), _T("Drive"), _T("Folder"), NULL };
	for (INT i = 0; str[i] != NULL; i++)
	{
		CRegString hRS (
				HKEY_CLASSES_ROOT,
				str[i]+sPropSheetEx,
				""
			);
		hRS.RemoveKey ();
	}

	return S_OK;
 }

 //////////////////////////////////////////////////////////////////////////////