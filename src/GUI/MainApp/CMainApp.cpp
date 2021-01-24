
 /*                                                                            
  *		Copyright (c) 2001 - 2005 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\MainApp\CMainApp.cpp
  *                                                                            
  *		Abstract:		The Win2fs application class.
  *
  *		Notes:			None  
  *
  *		Revision History:
  *
  *		Date		Version		Author				Change Log
  *		------------------------------------------------------------------------
  *
  *		03-DEC-05	0.0.1		Satish Kumar J		Initial Version
  */                          		

 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "StdAfx.h"

 #include "Utils.h"
 #include "CMainApp.h"
 #include "CMainDlg.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global data.

 CMainApp theApp;

 #define MUTEX_NAME		"WIN2FS_MUTEX"  

 /////////////////////////////////////////////////////////////////////////////

 // Protected functions.

 BOOL CMainApp::CheckOS ()
 {
	BOOL bRet;
	OSVERSIONINFO osvi;

	bRet = FALSE;
	ZeroMemory (&osvi, sizeof (OSVERSIONINFO));

	do
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

		if(!GetVersionEx(&osvi)) 
		{
			TRACE ("\n GetVersionEx failed, Errcode=0x%08lx, returning FALSE.", GetLastError ( ) );

			break;
		}

        if (VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId)  
		{
            m_bWin9x = TRUE;
		}
        else
		{
            m_bWin9x = FALSE;
		}

		bRet = TRUE;

    } while (FALSE);
    
	return bRet;
 }
 
 //////////////////////////////////////////////////////////////////////////////

 // Public functions.

 CMainApp::CMainApp ()
 {
	 m_bWin9x = FALSE;
	 m_hMutex = INVALID_HANDLE_VALUE;
 }

 CMainApp::~CMainApp ()
 {
	 SAFE_CLOSE_HANDLE (m_hMutex);
 }

 BOOL CMainApp::InitInstance ()
 {
    BOOL bRet;

	bRet = FALSE;

	do
	{
	    //
		//	1. Allow only a single instance to be active.
		//

		m_hMutex = CreateMutex (NULL, TRUE, MUTEX_NAME);

		BREAK_IF_EQUAL(m_hMutex, INVALID_HANDLE_VALUE);

		if (ERROR_ALREADY_EXISTS == GetLastError ())
		{
			TRACE ("\n Another instance found active, bailing out ...");

			CWnd *pWnd = CWnd::FindWindow (NULL, "Win2fs"); 

			ASSERT (pWnd);

			pWnd->CenterWindow ();
			pWnd->SetActiveWindow ();

			break;
		}

		//
		//	2. Check the target OS.
		//

		bRet = CheckOS ();

		if (FALSE == bRet)  
		{
			AfxMessageBox ( "Error: Unable to determine Windows Version." );

			break;
		}

		if (!m_bWin9x)
		{
			if (!UtilCheckIfAdmin ())
			{
				AfxMessageBox ("Sorry, but you do not have sufficient privileges to use this program.\r\n\r\n                 Please contact your system administrator.");

				break;
			}
		}

		//
		// 3. Check if the Win2fs service is started.
		//

		bRet = UtilCheckDriverStatus ();

		if (FALSE == bRet)
		{
			AfxMessageBox ("Error: Could not load the Win2fs driver.");

//			break;
		}

		//
		//  4. Create the dialog and start the show.
		//

		CMainDlg dlg;

		m_pMainWnd = &dlg;
 
		dlg.DoModal ();

	} while (FALSE);

    return FALSE;
 }

 /////////////////////////////////////////////////////////////////////////////
