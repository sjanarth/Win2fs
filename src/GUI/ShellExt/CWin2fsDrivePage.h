 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CWin2fsDrivePage.h
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

 #ifndef	__CWIN2FSDRIVEPAGE_H
 #define	__CWIN2FSDRIVEPAGE_H

 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "resource.h"
 #include "CWin2fsPage.h"

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

 class CWin2fsDrivePage : public CWin2fsPage
 {
	public:

		CWin2fsDrivePage(const CString& strPath, BOOL bAutoDestroy);
		virtual ~CWin2fsDrivePage();

	DECLARE_DYNAMIC(CWin2fsDrivePage)

	protected:

		virtual BOOL OnInitDialog();

		BOOL LoadSuper ();

	DECLARE_MESSAGE_MAP()
 };

 //////////////////////////////////////////////////////////////////////////////

 #endif		//	__CWIN2FSDRIVEPAGE_H