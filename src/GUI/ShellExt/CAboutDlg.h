 
 /*
  *		Copyright (C) Satish Kumar Janarthanan (vsat_in@yahoo.com) 2001-2005
  *
  *		Project:		
  *
  *		Module Name:    CAboutDlg.h
  *
  *     Abstract:		Header for the About dialog class.
  *
  *     Author:		    Satish Kumar J
  *
  *		Date:			23-APR-2002
  *
  *		Notes:			None.
  *
  *		Revision History:
  *
  *		Date		Version	Author				Changes
  *		------------------------------------------------------------------------
  *
  *		23-APR-02	1.00	Satish Kumar J		Initial	Version.
  *
  */

 #ifndef	__CABOUTDLG_H
 #define	__CABOUTDLG_H

 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "StdAfx.h"
 #include "..\..\Other\CHyperLink.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global declarations.

 class CAboutDlg : public CDialog
 {
	public:

		CAboutDlg();

		afx_msg BOOL OnInitDialog();

	protected:

		CHyperLink	m_Copyright;
 };

 //////////////////////////////////////////////////////////////////////////////

 #endif		//	__CABOUTDLG_H
