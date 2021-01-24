 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CWin2fsPage.cpp
  *
  *     Abstract:		The property page extension class.
  *
  *     Author:		    Satish Kumar J
  *
  *		Date:			20-NOV-2005
  *
  *		Notes:			None.
  *
  *		Revision History:
  *
  *		Date		Version	Author				Changes
  *		------------------------------------------------------------------------
  *
  *		20-NOV-05	1.00	Satish Kumar J		Initial	Version.
  *
  *
  */

 //////////////////////////////////////////////////////////////////////////////

 // Library includes.

 #include "stdafx.h"

 //////////////////////////////////////////////////////////////////////////////

 // Private includes.

 #include "ioctls.h"

 #include "CAboutDlg.h"
 #include "CWin2fsPage.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global data.

 BEGIN_MESSAGE_MAP(CWin2fsPage, CGenericPage)
	ON_COMMAND (IDC_ABOUT, OnAbout)
 END_MESSAGE_MAP()

 //////////////////////////////////////////////////////////////////////////////

 // Static data.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Restricted functions.

 // Privates'.

 // None.

 // Protected.

 // None.
 
 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 IMPLEMENT_DYNAMIC(CWin2fsPage, CGenericPage)

 CWin2fsPage::CWin2fsPage( UINT nIdTemplate, const CString& strPath, BOOL bAutoDestroy ) 
			:CGenericPage( nIdTemplate, bAutoDestroy )
 {
	m_strPath = strPath;

	AfxOleLockApp();	// Lock the DLL into memory despite 
						//	the shell extension object's Relese()
 }

 CWin2fsPage::~CWin2fsPage()
 {
	AfxOleUnlockApp();
 }

 BOOL CWin2fsPage::OnInitDialog() 
 {
	CGenericPage::OnInitDialog();

	m_list.SubclassDlgItem (IDC_LIST, this);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
	
	// Setup the rowheight.
	CImageList imageList; 
	imageList.Create(1, 16, ILC_COLOR4, 1, 1); 
	m_list.SetImageList(&imageList, LVSIL_SMALL); 

	m_list.InsertColumn (0, "Name", LVCFMT_LEFT, 125 );
	m_list.InsertColumn (1, "Value", LVCFMT_LEFT, 208 );
	
	return TRUE;  // return TRUE unless you set the focus to a control
 }

 /////////////////////////////////////////////////////////////////////////////

 // Message handlers.

 VOID CWin2fsPage::OnAbout ()
 {
	CAboutDlg dlg;
	dlg.DoModal ();
 }

 /////////////////////////////////////////////////////////////////////////////

 // Protected/private methods.

 ext2_super_block* CWin2fsPage::ReadSuper ()
 {
	BOOL bRet = FALSE;
	ext2_super_block* psb = new ext2_super_block ();

	do
	{
		CHAR dev[32];
		sprintf (dev, "\\\\.\\%C:", m_strPath[0]);

		HANDLE hFile = INVALID_HANDLE_VALUE;

	    hFile = CreateFile(dev, 
					GENERIC_READ|GENERIC_WRITE, 0, NULL, 
					OPEN_EXISTING, 0, NULL);

		TRACE("\n\nReadSuper, CreateFile returned: 0x%08lX", GetLastError());

		BREAK_IF_EQUAL (INVALID_HANDLE_VALUE, hFile);

		DWORD dwReturn = 0;

        bRet = DeviceIoControl(hFile,
                        IOCTL_GET_SUPER, 
                        m_strPath.GetBuffer (0),
                        m_strPath.GetLength(),
                        psb, 
                        sizeof(ext2_super_block), 
                        &dwReturn, 
						NULL);

		TRACE("\n\nReadSuper, DeviceIoControl returned: 0x%08lX", GetLastError());

		CloseHandle (hFile);

	} while (FALSE);

	if (FALSE == bRet)
	{
		SAFE_DELETE (psb);
	}

	return psb;
 }

 ext2_inode* CWin2fsPage::ReadInode ()
 {
	BOOL bRet = FALSE;
	ext2_inode* pi = new ext2_inode ();

	do
	{
		TRACE ("\n\nReadInode for %s", m_strPath);

		HANDLE hFile = INVALID_HANDLE_VALUE;

		DWORD dwAttr = GetFileAttributes (m_strPath);

		if ((dwAttr&FILE_ATTRIBUTE_DIRECTORY))
		{
			CHAR dev[32];
			sprintf (dev, "\\\\.\\%C:", m_strPath[0]);

			/*
			 *	Note: HACK !!!
			 *
			 *	For some reason, Windows doesn't let you do a private DeviceIoControl 
			 *	on a directory object (even if we manage to get a CreateFile handle).
			 *
			 *	Our hack, is to open the volume object and fake a private DeviceIoControl
			 *	on the volume object for reading the inode. We pass the name of the
			 *	directory in the Input buffer and the FSD understands this to really mean
			 *	a request for the inode of the directory given in the Input buffer and not 
			 *	as a request for the inode of the volume object itself.
			 *
			 *	We can do this, as we never read, write or show the inode of the volume 
			 *	object anywhere in user mode.
			 */

		    hFile = CreateFile(dev, 
						GENERIC_READ|GENERIC_WRITE, 0, NULL, 
						OPEN_EXISTING, 0, NULL);

		    // hFile = CreateFile(m_strPath, 
			//			GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 
			//			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT, 
			//			NULL);
		}
		else
		{
		    hFile = CreateFile(m_strPath, 
						GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 
						NULL, OPEN_EXISTING, 0, NULL);
		}

		TRACE("\n\nReadInode, CreateFile returned: 0x%08lX", GetLastError());

		BREAK_IF_EQUAL (INVALID_HANDLE_VALUE, hFile);

		DWORD dwReturn = 0;

		CString sfileName = m_strPath.Mid (2);

        bRet = DeviceIoControl(hFile,
                        IOCTL_GET_INODE, 
                        sfileName.GetBuffer (0),
                        sfileName.GetLength(),
                        pi, 
                        sizeof(ext2_inode), 
                        &dwReturn, 
						NULL);

		TRACE("\n\nReadInode, DeviceIoControl returned: 0x%08lX", GetLastError());

		CloseHandle (hFile);

	} while (FALSE);

	if (FALSE == bRet)
	{
		SAFE_DELETE (pi);
	}

	return pi;
 }

 BOOL CWin2fsPage::SaveInode (ext2_inode* pi)
 {
	 return TRUE;
	 UNUSED_ALWAYS(pi);
 }

 VOID CWin2fsPage::AddProperty (CString prop, DWORD value)
 {
	 CHAR buf[MAX_PATH];
	 sprintf (buf, "%d", value);
	 AddProperty (prop, buf);
 }

 VOID CWin2fsPage::AddPropertyHex (CString prop, DWORD value)
 {
	 CHAR buf[MAX_PATH];
	 sprintf (buf, "0x%x", value);
	 AddProperty (prop, buf);
 }

 VOID CWin2fsPage::AddProperty (CString prop, CString value)
 {
	 static UINT nItem = 0;

	 m_list.InsertItem (nItem, prop);
	 m_list.SetItemText (nItem, 1, value);
	 nItem++;
 }

 /////////////////////////////////////////////////////////////////////////////