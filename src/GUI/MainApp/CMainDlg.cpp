
 /*                                                                            
  *		Copyright (c) 2001 - 2005 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\MainApp\CMainDlg.cpp
  *                                                                            
  *		Abstract:		The Main dialog.
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

 #include "CInfoDlg.h"
 #include "CDrvSelDlg.h"
 #include "CMainDlg.h"
 #include "CAboutDlg.h"

 /////////////////////////////////////////////////////////////////////////////////////////////

 // Static data.

 BEGIN_MESSAGE_MAP (CMainDlg, CDialog)
	 ON_WM_PAINT ()
	 ON_COMMAND (IDC_BUTTON_ABOUT, OnClickAbout)
	 ON_COMMAND (IDC_BUTTON_MOUNT, OnClickMount)
	 ON_COMMAND (IDC_BUTTON_REFRESH, OnClickRefresh)
	 ON_COMMAND (IDC_BUTTON_INFO, OnClickInfo)
	 ON_COMMAND (IDC_BUTTON_CLOSE, OnClickClose)
	 ON_NOTIFY	(LVN_ITEMCHANGED, IDC_LIST_PART, OnNotify)
 END_MESSAGE_MAP ()

 /////////////////////////////////////////////////////////////////////////////////////////////

 // Static functions.

 static BOOL GetVolumeType (CHAR chMountPt, __PU8 type)
 {
	BOOL bRet;
	DWORD dwflags;

	bRet = FALSE;
	dwflags = 0;

	do
	{
		CHAR  buf[16];
		sprintf(buf, "%C:\\", chMountPt );

		bRet = GetVolumeInformation (buf, NULL, 0, NULL, NULL, &dwflags, NULL, 0);

		BREAK_IF_FALSE (bRet);

		if (dwflags&FS_VOL_IS_COMPRESSED)
			(*type) |= PT_COMPRESSION;

	} while (FALSE);

	return bRet;
 }

 /////////////////////////////////////////////////////////////////////////////////////////////

 // Protected members.

 VOID CMainDlg::PopulateListControl ()
 {
	 __U32 i, j;
	 __U8  chBuf[32];
	 
	 ZeroMemory ( chBuf, sizeof ( chBuf ) );

	 // We might be called to 'Refresh' the contents.
	 m_ListCtrl.DeleteAllItems ();

	 for ( i = 0, j = 0; 
			(m_Partitions[i].nSize != 0) || 
			(m_Partitions[i].chMountPoint != 0); 
			i++, j++ )
	 {
		 // Insert a blank row to separate disks.
		 if ( i > 0 )
		 {
			if ( m_Partitions[i].nDiskNumber != m_Partitions[i-1].nDiskNumber )
			{
				m_ListCtrl.InsertItem (j, "");

				j++;
			}
		 }

		 // The device name of the form /dev/...
		 UtilGetDeviceName ( &m_Partitions[i], chBuf );

		 m_ListCtrl.InsertItem ( j, (LPCTSTR) chBuf );

		 // The filesystem common name.
		 m_ListCtrl.SetItemText ( j, 1, (LPCSTR) UtilGetFilesystemString ( m_Partitions[i].nFilesystemId ) );

		 // The filesystem Id itself.
		 sprintf ( (PCHAR) chBuf, " %02X ", m_Partitions[i].nFilesystemId );
		 m_ListCtrl.SetItemText (j, 2, (LPCSTR) chBuf );

		 if ( isalpha ( m_Partitions[i].chMountPoint ) )
		 {
			// The mount point if one exists.
			sprintf ( (PCHAR) chBuf, " %C:\\ ", m_Partitions[i].chMountPoint );
			m_ListCtrl.SetItemText ( j, 3, (LPCSTR) chBuf );

			// The Volume label.
			m_ListCtrl.SetItemText ( j, 4, (LPCSTR) m_Partitions[i].szVolumeLabel);
		 }

		 // The size of the partition.
		 double dsize = (double) m_Partitions[i].nSize;
		 if (dsize > 1024)
		 {
			dsize = (double) (dsize / 1024);
			sprintf ( (PCHAR) chBuf, " %.2f GB", dsize);
		 }
		 else
		 {
			sprintf ( (PCHAR) chBuf, " %.2f MB", dsize);
		 }

		 m_ListCtrl.SetItemText ( j, 5, (LPCSTR) chBuf );

		 // Store the partition record as the row's data.
		 m_ListCtrl.SetItemData ( j, (DWORD) &m_Partitions[i] );

		 Sleep(50);
	 }
 }

 BOOL CMainDlg::InitControls ()
 {
	 BOOL bRet;

	 bRet = FALSE;

	 do
	 {
		 m_ListCtrl.SubclassDlgItem  ( IDC_LIST_PART, this );
		 m_ListCtrl.SetExtendedStyle ( LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES );
     
		 m_ListCtrl.InsertColumn ( 0, " Device ",	LVCFMT_LEFT, 65 );
		 m_ListCtrl.InsertColumn ( 1, " Fileystem ",LVCFMT_LEFT, 130 );
		 m_ListCtrl.InsertColumn ( 2, " ID ",		LVCFMT_LEFT, 35 );
		 m_ListCtrl.InsertColumn ( 3, " ",			LVCFMT_LEFT, 35 );
		 m_ListCtrl.InsertColumn ( 4, " Label ",	LVCFMT_LEFT, 70 );
		 m_ListCtrl.InsertColumn ( 5, " Size ",		LVCFMT_RIGHT, 80 );

		 // Disable at first.
		 m_ListCtrl.EnableWindow ( FALSE );
		 
		 bRet = UtilGetPartitionTable (m_Partitions);

		 BREAK_IF_FALSE ( bRet );

		 // Populate the control. 
		 PopulateListControl ( );

	     // Enable now.		 
		 m_ListCtrl.EnableWindow ( TRUE );

		 // Disable the "Mount" and "Info" buttons.
		 GetDlgItem(IDC_BUTTON_INFO)->EnableWindow(FALSE);
		 GetDlgItem(IDC_BUTTON_MOUNT)->EnableWindow(FALSE);

	 } while (FALSE);

	 return bRet;
 }

 __U32 CMainDlg::OnListSelectionChange ()
 {
	__U32 nSel;
	POSITION pos;
	PARTITION_INFO* pPartition;

	nSel = -1;
	pos = NULL;
	pPartition = NULL;

	do
	{
		pos = m_ListCtrl.GetFirstSelectedItemPosition ();

		BREAK_IF_NULL (pos);

		nSel = m_ListCtrl.GetNextSelectedItem (pos);
		pPartition = (PARTITION_INFO*) m_ListCtrl.GetItemData (nSel);

		// For those empty slots.
		BREAK_IF_NULL (pPartition);

		// Ext2 ?
		if (0x83 == pPartition->nFilesystemId)
		{
			((CWnd*)GetDlgItem(IDC_BUTTON_MOUNT))->EnableWindow (TRUE);

			// Mounted ?
			if ( isalpha ( pPartition->chMountPoint ) )
			{
				((CWnd*)GetDlgItem(IDC_BUTTON_MOUNT))->SetWindowText ("&UnMount");
				break;
			}

			((CWnd*)GetDlgItem(IDC_BUTTON_MOUNT))->SetWindowText ("&Mount");

			break;
		}

		// Something else.
		GetDlgItem(IDC_BUTTON_INFO)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_MOUNT)->EnableWindow(FALSE);

	} while (FALSE);

	return nSel;
 }

 BOOL CMainDlg::OnMount ()
 {
	 BOOL  bRet;
	 CDrvSelDlg *pDlg;
	 PARTITION_INFO* pi;

	 pi = NULL;
	 pDlg = NULL;
	 bRet = FALSE;

	 do
	 {
		 __S32 nSel = OnListSelectionChange ();

		 BREAK_IF_EQUAL (nSel, -1);

		 pi = (PARTITION_INFO*) m_ListCtrl.GetItemData (nSel);

		 // For those empty slots.
		 BREAK_IF_NULL (pi);

		 pDlg = new CDrvSelDlg ();

		 __U8 nRet = pDlg->DoModal ();

		 BREAK_IF_NOTEQUAL (nRet, IDOK);

		 __U8 drv = pDlg->GetSelectedDrive ();

		 BOOL bPersist = pDlg->GetPersistentFlag ();

		 bRet = UtilMount (pi->nDiskNumber, pi->nPartNumber, drv, bPersist);

		 CString str;
		 __U8 szDevName[128];

		 UtilGetDeviceName (pi, szDevName);

		 if (FALSE == bRet)
		 {
			 str.Format ("Failed to Mount %s on %C:\\ ", szDevName,	drv);

			 UtilShowMessage (str, MT_ERROR, GetLastError () ) ;

			 break;
		 }

		 str.Format ("Successfully Mounted %s on %C:\\", szDevName, drv);
		 UtilShowMessage (str, MT_INFO);

		 pi->chMountPoint = drv;
		 UtilGetFreeDiskSpace(pi->chMountPoint, &pi->nFree);
		 UtilGetVolumeLabel(pi->chMountPoint, pi->szVolumeLabel);

		 OnClickRefresh ();

	 } while (FALSE);

	 SAFE_DELETE (pDlg);

	 return bRet;
 }

 BOOL CMainDlg::OnUnMount ()
 {
	 BOOL  bRet;
	 __U32 nSel;
	 PARTITION_INFO* pi;

	 pi = NULL;
	 nSel = -1;
	 bRet = FALSE;

	 do
	 {
		 nSel = OnListSelectionChange ();

		 BREAK_IF_EQUAL (nSel, -1);

		 pi = (PARTITION_INFO*) m_ListCtrl.GetItemData (nSel);

		 // For those empty slots.
		 BREAK_IF_NULL (pi);

		 // Check if its really mounted.
		 if ( !isalpha (pi->chMountPoint) )
		 {
			 break;
		 }

		 bRet = UtilUnMount (pi->chMountPoint);

		 CString str;
		 __U8 szDevName[128];

		 UtilGetDeviceName (pi, szDevName);

		 if (FALSE == bRet)
		 {
			 CString str;

			 str.Format ("Failed to UnMount %s from %C:\\ ", szDevName, pi->chMountPoint);
			 
			 UtilShowMessage (str, MT_ERROR, GetLastError());
			 
			 break;
		 }

		 str.Format ("Successfully UnMounted %s from %C:\\", szDevName, pi->chMountPoint);
		 UtilShowMessage (str, MT_INFO);
		 
		 pi->chMountPoint = 0;
		 pi->nFree = 0;
		 ZeroMemory( pi->szVolumeLabel, sizeof ( pi->szVolumeLabel ));

		 OnClickRefresh ();

	 } while (FALSE);

	 return bRet;
 }

 /////////////////////////////////////////////////////////////////////////////////////////////

 // Global functions.

 // Construtor / Destructor.

 CMainDlg::CMainDlg ():CDialog (IDD_MAINDLG)
 {
	ZeroMemory (&m_Partitions, sizeof (m_Partitions));

	m_hIcon = AfxGetApp()->LoadIcon (IDI_DLGFRAME);
 }

 CMainDlg::~CMainDlg ()
 {
 }

 // Publics'.

 BOOL CMainDlg::OnInitDialog ()
 {
     CDialog::OnInitDialog();

     SetIcon (m_hIcon, TRUE);			
     SetIcon (m_hIcon, FALSE);			

	 InitControls ();

     return TRUE;
 }

 VOID CMainDlg::OnPaint()
 {
	 CPaintDC dc(this);

	// Draw the lines.
	CRect rectWnd;
    GetWindowRect (rectWnd);

	dc.Draw3dRect (
		4, 3,							// upper-left point
        rectWnd.Width()-15, 2,          // width & height
        ::GetSysColor(COLOR_3DSHADOW),
        ::GetSysColor(COLOR_3DHIGHLIGHT));

	dc.Draw3dRect (
		4, 210,							// upper-left point
        rectWnd.Width()-15, 2,          // width & height
        ::GetSysColor(COLOR_3DSHADOW),
        ::GetSysColor(COLOR_3DHIGHLIGHT));
 }

 BOOL CMainDlg::OnNotify (NMHDR* phdr, LRESULT* pRes)
 {
 	 OnListSelectionChange ();

	 return FALSE;
 }

 VOID CMainDlg::OnClose ()
 {
	 EndDialog (IDOK);

	 PostQuitMessage (0);
 }

 VOID CMainDlg::OnClickClose ()
 {
	 OnClose ();
 }

 VOID CMainDlg::OnClickInfo ()
 {
	 __S32 nSel;

	 do
	 {
		nSel = OnListSelectionChange ();

		BREAK_IF_EQUAL (nSel, -1);

		PARTITION_INFO* pi = (PARTITION_INFO*) m_ListCtrl.GetItemData (nSel);

		// For those empty slots.
		BREAK_IF_NULL (pi);

		CInfoDlg *pDlg = new CInfoDlg (pi);
		
		pDlg->DoModal ();

		SAFE_DELETE(pDlg);

	 } while (FALSE);
 }

 VOID CMainDlg::OnClickAbout ()
 {
	 CAboutDlg dlg;

	 dlg.DoModal ();
 }

 VOID CMainDlg::OnClickMount ()
 {
	 CString szText;

	 GetDlgItemText (IDC_BUTTON_MOUNT, szText);
	 
	 if (0 == szText.CompareNoCase ("&Mount"))
	 {
		 OnMount ();
	 }
	 else
	 {
		 OnUnMount ();
	 }
 }

 VOID CMainDlg::OnClickRefresh ()
 {
	 BOOL bRet;

	 bRet = FALSE;

	 do
	 {
		 // Disable at first.
		 m_ListCtrl.EnableWindow ( FALSE );
		 
		 bRet = UtilGetPartitionTable (m_Partitions) ;

		 BREAK_IF_FALSE ( bRet );

		 // Populate the control.
		 PopulateListControl ( );

		 // Enable now.
		 m_ListCtrl.EnableWindow ( TRUE );

	 } while (FALSE);
 }

 /////////////////////////////////////////////////////////////////////////////////////////////
