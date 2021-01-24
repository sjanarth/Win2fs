 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CWin2fsFilePage.h
  *
  *     Abstract:		Header for the property page class.
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

 #ifndef	__CWIN2FSFILEPAGE_H
 #define	__CWIN2FSFILEPAGE_H

 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "resource.h"
 #include "CWin2fsPage.h"

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

 class CWin2fsFilePage : public CWin2fsPage
 {
	public:

		CWin2fsFilePage(const CString& strPath, BOOL bAutoDestroy);
		virtual ~CWin2fsFilePage();

	DECLARE_DYNAMIC(CWin2fsFilePage)

	protected:

		virtual BOOL OnInitDialog();

		BOOL LoadInode ();
		BOOL SaveInode ();
		VOID CheckFilePerms (ext2_inode*);

	DECLARE_MESSAGE_MAP()

 };

 //////////////////////////////////////////////////////////////////////////////

 #endif		//	__CWIN2FSFILEPAGE_H