
 /*                                                                            
  *		Copyright (c) 2001 - 2003 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\CPL\CPLApp.cpp
  *                                                                            
  *		Abstract:		A Control panel applet framework.
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

 #include <windows.h>

 #include "CPLApp.h"

 /////////////////////////////////////////////////////////////////////////////
 
 // Global functions.

 BOOL 
	WINAPI DllMain (
		HINSTANCE hInstance, 
		DWORD dwReason, 
		LPVOID lpvReserved
	)
 {
	if ( DLL_PROCESS_ATTACH == dwReason )	
    {
		CCPLEx::SetInstanceHandle ( hInstance );
	}

	return TRUE;
 }

 // This is the main entry point for the CPL.

 LONG 
	CALLBACK CPlApplet (
		HWND hWnd, 
		UINT uMsg, 
		LONG lParam1, 
		LONG lParam2
	)
 {
	LONG lResult = 0;

	switch ( uMsg )   
	{
		// Applet initialisation. 

		case CPL_INIT:                

			lResult = CCPLEx::Init ( );
		 
			break;

		case CPL_GETCOUNT:       // How many applets in the DLL?          

			lResult = CCPLEx::GetCount ( );

			break;

		case CPL_INQUIRE:		 //  Tell Control Panel about this applet.

			lResult = CCPLEx::Inquire ( lParam1, (LPCPLINFO) lParam2 );
		 
			break;

		case CPL_NEWINQUIRE:	 //  Tell Control Panel about this applet.

			lResult = CCPLEx::NewInquire ( lParam1, (LPNEWCPLINFO) lParam2 );
		 
			break;

		case CPL_DBLCLK:         //  Double click notification.

			lResult = CCPLEx::DoubleClick ( hWnd, lParam1, lParam2 );

			break;

 #if (WINVER >= 0x0400)

		case CPL_STARTWPARMS:   // Started with RUNDLL.       
         
			lResult = CCPLEx::StartWithParams ( hWnd, lParam1, (LPSTR) lParam2 );
		 
			break;
 
 #endif		// WINVER >= 0x4000

		case CPL_STOP:			// Applet shutdown.

			lResult = CCPLEx::Stop ( lParam1, lParam2 );
	     
			break;

		case CPL_EXIT :			// DLL shutdown.

			lResult = CCPLEx::Exit ( );
		 
			break;

		default:
		 
			break;
	}

	return lResult;
 }

 /////////////////////////////////////////////////////////////////////////////

 //	Static members.

 CCPLEx *CCPLEx::s_pListHead = 0;

 HINSTANCE CCPLEx::s_hInstance = (HINSTANCE) INVALID_HANDLE_VALUE;

 VOID 
	CCPLEx::SetInstanceHandle (
		HINSTANCE hInstance
	)
 {
	s_hInstance = hInstance;
 }

 LONG CCPLEx::Init ( )
 {
	BOOL bOk = TRUE;
   
	CCPLEx* pApplet = s_pListHead;

	while ( ( pApplet ) && ( bOk ) )    
	{
		bOk = pApplet->OnInit ( );

		pApplet = pApplet->m_pNext;
	}

	return bOk;
 }

 LONG CCPLEx::GetCount ( )
 {
	LONG numApplets = 0;

	CCPLEx* pApplet = s_pListHead;

	while ( pApplet )   
	{
		numApplets++;
		
		pApplet = pApplet->m_pNext;
	}

	return numApplets;
 }

 CCPLEx*
	CCPLEx::GetAppletByIndex (
		LONG index
	)
 {
	LONG numApplets = 0;

	CCPLEx* pApplet = s_pListHead;

	while ( ( pApplet ) && ( numApplets < index ) )   
	{
		numApplets++;

		pApplet = pApplet->m_pNext;
	}
   
	return pApplet;
 }

 LONG 
	CCPLEx::Inquire (
		LONG appletIndex, 
		LPCPLINFO lpCPlInfo
	)
 {
	LONG lResult = 1;

	CCPLEx* pApplet = GetAppletByIndex ( appletIndex );
   
	if ( ( pApplet ) && pApplet->OnInquire ( lpCPlInfo ) )   
	{
		lResult = 0;
	}

	return lResult;
 }

 LONG 
	CCPLEx::NewInquire (
		LONG appletIndex, 
		LPNEWCPLINFO lpCPlInfo
	)
 {
	LONG lResult = 1;
   
	CCPLEx *pApplet = GetAppletByIndex ( appletIndex );
   
	if ( ( pApplet ) && pApplet->OnNewInquire ( lpCPlInfo ) )
	{
		lResult = 0;
	}

	return lResult;
 }

 LONG 
	CCPLEx::DoubleClick (
		HWND hWnd, 
		LONG appletIndex, 
		LONG appletData
	)
 {
	LONG lResult = 1;
   
	CCPLEx *pApplet = GetAppletByIndex ( appletIndex );
   
	if ( ( pApplet ) && pApplet->OnDoubleClick ( hWnd, appletData ) )   
	{
		lResult = 0;
	}

	return lResult;
 }

 LONG 
	CCPLEx::StartWithParams (
		HWND hWnd, 
		LONG appletIndex, 
		LPSTR params
	)
 {
	LONG lResult = 1;
   
	CCPLEx* pApplet = GetAppletByIndex ( appletIndex );
   
	if ( ( pApplet ) && pApplet->OnStartWithParams ( hWnd, params ) )   
	{
		lResult = 0;
	}

	return lResult;
 }

 LONG 
	CCPLEx::Stop (
		LONG appletIndex, 
		LONG appletData
	)
 {
	LONG lResult = 1;
   
	CCPLEx* pApplet = GetAppletByIndex ( appletIndex );
   
	if ( ( pApplet ) && pApplet->OnStop ( appletData ) )   
	{
		lResult = 0;
	}

	return lResult;
 }

 LONG CCPLEx::Exit ( )
 {
	LONG lResult = 0;
   
	CCPLEx* pApplet = s_pListHead;

	while ( pApplet )   
	{
		if ( ! pApplet->OnExit ( ))      
		{
			lResult = 1;
		}

	    pApplet = pApplet->m_pNext;
	}

	return lResult;
 }

 BOOL CCPLEx::OnInit ( )
 {
	return TRUE;
 }

 BOOL 
	CCPLEx::OnInquire ( 
		LPCPLINFO lpCPlInfo 
	)
 {
	lpCPlInfo->lData = 0;
	lpCPlInfo->idIcon = m_nIconID;
	lpCPlInfo->idName = m_nNameID;
	lpCPlInfo->idInfo = m_nDescID;
   
	return TRUE;
 }

 BOOL 
	CCPLEx::OnNewInquire ( 
		LPNEWCPLINFO lpNewCPlInfo
	)
 {
	BOOL bOk = FALSE;

	if ( sizeof ( NEWCPLINFO ) == lpNewCPlInfo->dwSize )   
	{
		lpNewCPlInfo->dwSize = (DWORD) sizeof(NEWCPLINFO);
		lpNewCPlInfo->dwFlags = 0;
		lpNewCPlInfo->dwHelpContext = 0;
		lpNewCPlInfo->lData = 0;
		lpNewCPlInfo->hIcon = LoadIcon ( s_hInstance, MAKEINTRESOURCE ( m_nIconID ) );
		lpNewCPlInfo->szHelpFile[0] = '\0';
	   
		LoadString ( s_hInstance, m_nNameID, lpNewCPlInfo->szName, 32 );
		LoadString ( s_hInstance, m_nDescID, lpNewCPlInfo->szInfo, 64 );

		bOk = TRUE;
	}

	return bOk;
 }

 BOOL 
	CCPLEx::OnStartWithParams (
		HWND hWnd, 
		LPSTR /* params */
	)
 {
	return OnDoubleClick ( hWnd, 0 );
 }

 BOOL 
	CCPLEx::OnStop (
		LONG /* appletData */
	)
 {
	return TRUE;
 }

 BOOL CCPLEx::OnExit ( )
 {
	return TRUE;
 }

 ////////////////////////////////////////////////////////////////////////////

 // Public functions.

 //	Construction and destruction.

 CCPLEx::CCPLEx (INT nIconID, INT nNameID, INT nDescID): 
				  m_nIconID (nIconID), 
				  m_nNameID (nNameID), 
				  m_nDescID (nDescID)
 {
    m_pNext = s_pListHead;
   
	s_pListHead = this;
 }

 CCPLEx::~CCPLEx()
 {
    // Remove ourselves from the list of all Applet objects.

    CCPLEx *pApplet = s_pListHead;
    CCPLEx *pPrevious = NULL;
   
    // First find ourselves in the list...

	while ( ( pApplet ) && ( this != s_pListHead ) )   
	{
		pPrevious = pApplet;

		pApplet = pApplet->m_pNext;
	}

	// Sanity check...   

	if ( pApplet )      
	{ 
		// Remove ourselves

		if ( pPrevious )      
		{
			pPrevious = pApplet->m_pNext;
		}
		else			      
		{
			s_pListHead = pApplet->m_pNext;
		}
	}

	m_pNext = NULL;   // Clear our link to the next object. 
 }

 /////////////////////////////////////////////////////////////////////////////
