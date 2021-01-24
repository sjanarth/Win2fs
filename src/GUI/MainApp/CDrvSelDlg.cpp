
 /*                                                                            
  *		Copyright (c) 2001 - 2005 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\MainApp\CDrvSelDlg.cpp
  *                                                                            
  *		Abstract:		The drive selction dialog class.
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
 
 #include "resource.h" 
 #include "CDrvSelDlg.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global funtions.

 // Constructor / Destructor.

 CDrvSelDlg::CDrvSelDlg ():CDialog (IDD_DRVSELDLG)
 {
	 m_SelDrv = 0;
	 m_bPersist = FALSE;
 }	

 CDrvSelDlg::~CDrvSelDlg ()
 {
 }

 /////////////////////////////////////////////////////////////////////////////

 BOOL CDrvSelDlg::OnInitDialog ()
 {
	CDialog::OnInitDialog ();

	m_DrvList.SubclassDlgItem (IDC_COMBO_DRIVE, this);

	DWORD dwUsed = GetLogicalDrives ();

	// Skip to 'C:\'.
	dwUsed >>= 2;

	CString str;
	__U8 nPos = 67;
	__U8 nItem = 0;

	while (dwUsed)
	{
		if (0 == (dwUsed&1))
		{
			str.Format ("%C:", nPos);
			m_DrvList.AddString (str);
			m_DrvList.SetItemData (nItem, nPos);
			nItem++;
		}

		nPos++;
		dwUsed >>= 1;
	}

	// Add the remaining drives.
	while (nPos <= 'Z')
	{
		str.Format ("%C:", nPos);
		m_DrvList.AddString (str);
		m_DrvList.SetItemData (nItem, nPos);
		nItem++;
		nPos++;
	}

	m_DrvList.SetCurSel (0);

    return TRUE;
 }

 VOID CDrvSelDlg::OnOK ()
 {
	__S8 nSel = m_DrvList.GetCurSel ();

	if (nSel != -1)
	{
		m_SelDrv = (__U8) m_DrvList.GetItemData (nSel);

		m_bPersist = (BOOL) IsDlgButtonChecked (IDC_CHECK_PERSIST);
	}

	CDialog::OnOK ();
 }

 __U8 CDrvSelDlg::GetSelectedDrive ()
 {
	return m_SelDrv;
 }

 BOOL CDrvSelDlg::GetPersistentFlag ()
 {
	 return m_bPersist;
 }

 /////////////////////////////////////////////////////////////////////////////