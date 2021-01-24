 
 /*
  *		Copyright (C) 2003 - 2004 Satish Kumar J (vsat_in@yahoo.com)  
  *
  *		Project:		
  *
  *		Module Name:    CAboutDlg.cpp
  *
  *     Abstract:		The About dialog class.
  *
  *     Author:		    Satish Kumar J
  *
  *		Date:			23-SEP-2004
  *
  *		Notes:			None.
  *
  *		Revision History:
  *
  *		Date		Version	Author				Changes
  *		------------------------------------------------------------------------
  *
  *		23-SEP-04	1.00	Satish Kumar J		Initial	Version.
  *
  *
  */

 //////////////////////////////////////////////////////////////////////////////

 // Include files.

 #include "StdAfx.h"

 #include "resource.h"
 #include "CAboutDlg.h"
 #include "Version.h" 

 //////////////////////////////////////////////////////////////////////////////

 // Global data.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Restricted functions.

 // None.
 
 //////////////////////////////////////////////////////////////////////////////

 CAboutDlg::CAboutDlg() 
	 : CDialog(IDD_ABOUTDLG)
 {
 }

 BOOL CAboutDlg::OnInitDialog()
 {
	// Setup the hyperlinks.
	m_Copyright.SubclassDlgItem(IDC_STATIC_URL, this);
	m_Copyright.SetURL("http://groups.yahoo.com/group/win2fs/");
	m_Copyright.SetUnderline(CHyperLink::ulAlways);

	SetDlgItemText (IDC_STATIC_VERSION_EX, PRODUCT_VERSION_EX);
	SetDlgItemText (IDC_STATIC_COPYRIGHT, PRODUCT_COPYRIGHT);

	return TRUE;
 }

 //////////////////////////////////////////////////////////////////////////////


