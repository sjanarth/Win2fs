
 /*                                                                            
  *		Copyright (c) 2001 - 2003 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\MainApp\CMainApp.h
  *                                                                            
  *		Abstract:		Header for the application class.
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

 #ifndef	__CMAINAPP_H
 #define	__CMAINAPP_H

 /////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "StdAfx.h"

 /////////////////////////////////////////////////////////////////////////////

 // Global declarations.

 class CMainApp : public CWinApp 
 {
	protected:
		
		HANDLE m_hMutex;        

		BOOL CheckOS ();
    
	public:

		BOOL m_bWin9x;

		CMainApp ();
		virtual ~CMainApp ();
        
		virtual BOOL InitInstance ();
 };
 
 /////////////////////////////////////////////////////////////////////////////

 #endif		//	__CMAINAPP_H
 