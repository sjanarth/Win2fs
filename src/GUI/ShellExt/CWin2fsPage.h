 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CWin2fsPage.h
  *
  *     Abstract:		Header for the base class of all out property classes.
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

 #ifndef	__CWIN2FSPAGE_H
 #define	__CWIN2FSPAGE_H

 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "resource.h"
 #include "CGenericPage.h"
 
 #include "linux\ext2_fs.h"

 //////////////////////////////////////////////////////////////////////////////

 // Macros and typedefs.
 
 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Externals.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static declarations.

 // None.

 //////////////////////////////////////////////////////////////////////////////
 
 // Global declarations.

 class CWin2fsPage : public CGenericPage
 {
	public:

		CWin2fsPage(UINT nIdTemplate, const CString& strPath, BOOL bAutoDestroy);
		virtual ~CWin2fsPage();

		virtual BOOL OnInitDialog();

		afx_msg VOID OnAbout();

	DECLARE_DYNAMIC(CWin2fsPage)

	protected:

		ext2_super_block*	ReadSuper ();
		ext2_inode*			ReadInode ();
		BOOL				SaveInode (ext2_inode*);

		VOID AddProperty (CString, DWORD);
		VOID AddProperty (CString, CString);
		VOID AddPropertyHex (CString, DWORD);

		CString		m_strPath;
		CListCtrl	m_list;

	DECLARE_MESSAGE_MAP()
 };

 //////////////////////////////////////////////////////////////////////////////

 #endif		//	__CWIN2FSPAGE_H