
 /*                                                                            
  *		Copyright (c) 2001 - 2003 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\MainApp\CMainDlg.h
  *                                                                            
  *		Abstract:		Header for the dialog class.
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

 #ifndef	__CMAINDLG_H
 #define	__CMAINDLG_H

 /////////////////////////////////////////////////////////////////////////////

 // Includes.
 
 #include "StdAfx.h"

 #include "Utils.h"

 /////////////////////////////////////////////////////////////////////////////

 // Global declarations.

 class CMainDlg : public CDialog
 {
    public:

        CMainDlg ();
        virtual ~CMainDlg ();
        
		afx_msg VOID OnClose ();
		afx_msg VOID OnPaint ();
		afx_msg BOOL OnInitDialog ();
		afx_msg BOOL OnNotify(NMHDR*, LRESULT*); 

		afx_msg VOID OnClickAbout ();
		afx_msg VOID OnClickMount ();
		afx_msg VOID OnClickRefresh ();
		afx_msg VOID OnClickInfo ();
		afx_msg VOID OnClickClose ();

	protected:

        HICON			m_hIcon;
		CListCtrl		m_ListCtrl;
		PARTITION_INFO	m_Partitions[MAX_PART_RECORDS];

		BOOL  OnMount ();
		BOOL  OnUnMount ();
		BOOL  InitControls ();
		VOID  PopulateListControl ();
		__U32 OnListSelectionChange ();

	DECLARE_MESSAGE_MAP ()
 };

 /////////////////////////////////////////////////////////////////////////////
 
 #endif		// __CMAINDLG_H
