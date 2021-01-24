
 /*                                                                            
  *		Copyright (c) 2001 - 2003 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\MainApp\CInfoDlg.h
  *                                                                            
  *		Abstract:		Header for the Info dialog class.
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

 #ifndef	__CINFODLG_H
 #define	__CINFODLG_H

 /////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "StdAfx.h"
 #include "Utils.h"

 /////////////////////////////////////////////////////////////////////////////

 // Global declarations.

 class CInfoDlg : public CDialog
 {
    public:

        CInfoDlg (PARTITION_INFO*);
        virtual ~CInfoDlg();
        
		afx_msg BOOL OnInitDialog ();

	protected:

		VOID PopulateInfo();

		CListCtrl			m_List;
		PARTITION_INFO*		m_pPartition;
 };
 
 /////////////////////////////////////////////////////////////////////////////
 
 #endif		// __CINFODLG_H