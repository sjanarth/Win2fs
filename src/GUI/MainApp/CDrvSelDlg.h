
 /*                                                                            
  *		Copyright (c) 2001 - 2003 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\MainApp\CDrvSelDlg.h
  *                                                                            
  *		Abstract:		Header for the Drive Selection dialog class.
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

 #ifndef __CDRVSELDLG_H
 #define __CDRVSELDLG_H

 /////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "StdAfx.h"

 /////////////////////////////////////////////////////////////////////////////

 // Global declarations.

 class CDrvSelDlg : public CDialog
 {
    public:

        CDrvSelDlg ();
        virtual ~CDrvSelDlg();
        
		afx_msg VOID OnOK ();
		afx_msg BOOL OnInitDialog ();

		__U8 GetSelectedDrive ();
		BOOL GetPersistentFlag ();

	protected:

		__U8	m_SelDrv;
		
		BOOL	m_bPersist;	

		CComboBox m_DrvList;
 };
 
 /////////////////////////////////////////////////////////////////////////////
 
 #endif		// __CDRVSELDLG_H