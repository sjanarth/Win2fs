
 /*                                                                            
  *		Copyright (c) 2001 - 2005 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\MainApp\CInfoDlg.cpp
  *                                                                            
  *		Abstract:		The Info dialog class.
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

 //////////////////////////////////////////////////////////////////////////////

 // Global funtions.

 // Constructor / Destructor.

 CInfoDlg::CInfoDlg (PARTITION_INFO* pPart):CDialog (IDD_INFODLG)
 {
	 m_pPartition = pPart;
 }

 CInfoDlg::~CInfoDlg ()
 {
 }

 /////////////////////////////////////////////////////////////////////////////

 BOOL CInfoDlg::OnInitDialog ()
 {
	CDialog::OnInitDialog ();

	m_List.SubclassDlgItem (IDC_LIST, this);
	m_List.SetExtendedStyle ( LVS_EX_FULLROWSELECT);// | LVS_EX_GRIDLINES );

	m_List.InsertColumn (0, " Property ", LVCFMT_LEFT, 100);
	m_List.InsertColumn (1, " Value ", LVCFMT_LEFT, 185);

	PopulateInfo();

    return TRUE;
 }

 /////////////////////////////////////////////////////////////////////////////

 VOID CInfoDlg::PopulateInfo ()
 {
	CString str;	
	__U8 szDevName[256];

	str = "Volume Information";
	strcpy((PCHAR)szDevName, "");

	if (0 != m_pPartition->nPartNumber)
	{
		BOOL bRet = UtilGetDeviceName (m_pPartition, szDevName);
		if (bRet)
		{
			strcpy ((PCHAR)szDevName, "???");
		}
		str.Format("%s - Volume Information", szDevName);
	}
	SetWindowText (str);

	m_List.InsertItem (0, "Label");			m_List.SetItemText (0, 1, m_pPartition->szVolumeLabel);
	m_List.InsertItem (1, "Volume");		m_List.SetItemText (1, 1, (LPCSTR) szDevName);
	m_List.InsertItem (2, "File system");	m_List.SetItemText (2, 1, (LPCSTR) UtilGetFilesystemString(m_pPartition->nFilesystemId));
	m_List.InsertItem (3, "Volume type");	

	//
	//	TODO: Handling USB, Floppy, SCSI disks.
	//

	str = "Fixed, ";	

	if (IS_PRIMARY(m_pPartition->nType))	str += "Primary";
	else									str += "Logical";

	if (IS_SYSTEM(m_pPartition->nType))		str += ", System";
	if (IS_READONLY(m_pPartition->nType))	str += ", Readonly";
	if (IS_ENCRYPTED(m_pPartition->nType))	str += ", Encrypted";
	if (IS_COMPRESSED(m_pPartition->nType))	str += ", Compressed";
	
	m_List.SetItemText (3, 1, str);

	INT nIndex = 4;
/*	if (m_pPartition->pDP != NULL)
	{
		m_List.InsertItem (4, "Sectors");
		str.Format ("%lu", m_pPartition->pDP->nSectorCount);
		m_List.SetItemText (4, 1, str);
		nIndex++;
	}
*/	
	m_List.InsertItem (nIndex, "Mount point");	
	m_List.InsertItem (nIndex+1, "Total space");
	m_List.InsertItem (nIndex+2, "Used space");
	m_List.InsertItem (nIndex+3, "Free space");

	if (isalpha (m_pPartition->chMountPoint))
	{
		str.Format ("%C:\\", m_pPartition->chMountPoint);
		m_List.SetItemText (nIndex, 1, str);

		str.Format ("%lu Mb", m_pPartition->nSize);
		m_List.SetItemText (nIndex+1, 1, str);

		str.Format ("%lu Mb", m_pPartition->nSize-m_pPartition->nFree);
		m_List.SetItemText (nIndex+2, 1, str);

		str.Format ("%lu Mb", m_pPartition->nFree);
		m_List.SetItemText (nIndex+3, 1, str);
	}
	else
	{
		for (INT i = 4; i < 9; i++)
			m_List.SetItemText (i, 1, " - ");
	}
 }