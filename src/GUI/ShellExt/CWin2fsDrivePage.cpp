 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CWin2fsDrivePage.cpp
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

 #include "CAboutDlg.h"
 #include "CWin2fsDrivePage.h"

 #include "ioctls.h"
 #include "linux\ext2_fs.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global data.

 BEGIN_MESSAGE_MAP(CWin2fsDrivePage, CWin2fsPage)
 END_MESSAGE_MAP()

 //////////////////////////////////////////////////////////////////////////////

 // Static data.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 PCHAR GetOSString (__U32 creator)
 {
	 switch (creator)
	 {
		case EXT2_OS_LINUX:
			return "Linux";
		case EXT2_OS_HURD:	
			return "GNU Hurd";
		case EXT2_OS_MASIX:
			return "MASIX";
		case EXT2_OS_FREEBSD:
			return "FreeBSD";
		case EXT2_OS_LITES:
			return "Lites";
		case EXT2_OS_WINNT:
			return "Windows NT";
		default:
			return "Unknown";
	 }
 }

 PCHAR decode (ext2_super_block* sb, __U32 mask)
 {
	 if (EXT2_HAS_COMPAT_FEATURE(sb,mask))
		 return "Yes";
	 else if (EXT2_HAS_RO_COMPAT_FEATURE(sb,mask))
		 return "Read-only";
	 else
		 return "No";
 }

 //////////////////////////////////////////////////////////////////////////////

 // Restricted functions.

 // Privates'.

 // None.

 // Protected.

 // None.
 
 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 IMPLEMENT_DYNAMIC(CWin2fsDrivePage, CWin2fsPage)

 CWin2fsDrivePage::CWin2fsDrivePage( const CString& strPath, BOOL bAutoDestroy ) 
			:CWin2fsPage( IDD_WIN2FS_DRIVEPAGE, strPath, bAutoDestroy )
 {
 }

 CWin2fsDrivePage::~CWin2fsDrivePage()
 {
 }

 BOOL CWin2fsDrivePage::OnInitDialog() 
 {
	CWin2fsPage::OnInitDialog();

	LoadSuper ();

	return TRUE;  // return TRUE unless you set the focus to a control
 }

 /////////////////////////////////////////////////////////////////////////////

 // Message handlers.

 // None.

 /////////////////////////////////////////////////////////////////////////////

 // Protected/private methods.

 BOOL CWin2fsDrivePage::LoadSuper ()
 {
	BOOL bRet = FALSE;

	ext2_super_block* psb = NULL;

	do
	{
		psb = ReadSuper ();

		BREAK_IF_NULL (psb);

		CString sInfo;
		sInfo.Format (
					"%d bytes, %d total, %d free",
					EXT2_BLOCK_SIZE(psb),
					psb->s_blocks_count,
					psb->s_free_blocks_count
				);
		AddProperty ("Blocks", sInfo);

		sInfo.Format (
					"%d bytes, %d total, %d free",
					EXT2_INODE_SIZE(psb),
					psb->s_inodes_count,
					psb->s_free_inodes_count
				);
		AddProperty ("Inodes", sInfo);

		AddProperty ("Mount count", psb->s_mnt_count);
		AddProperty ("Creator OS", GetOSString(psb->s_creator_os));
		AddPropertyHex ("Magic", psb->s_magic);
		AddProperty ("Volume Name", psb->s_volume_name);
		AddProperty ("", "");

		AddProperty ("Directory Pre-allocation",	decode(psb, EXT2_FEATURE_COMPAT_DIR_PREALLOC));
		AddProperty ("Has Journal",					decode(psb, EXT3_FEATURE_COMPAT_HAS_JOURNAL));
		AddProperty ("Extended Attributes",			decode(psb, EXT2_FEATURE_COMPAT_EXT_ATTR));
		AddProperty ("Resizable Inodes",			decode(psb, EXT2_FEATURE_COMPAT_RESIZE_INODE));
		AddProperty ("Directory Indexing",			decode(psb, EXT2_FEATURE_COMPAT_DIR_INDEX));
		AddProperty ("Sparse super blocks",			decode(psb, EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER));
		AddProperty ("Large files support",			decode(psb, EXT2_FEATURE_RO_COMPAT_LARGE_FILE));
		AddProperty ("B-Tree Directories",			decode(psb, EXT2_FEATURE_RO_COMPAT_BTREE_DIR));

	} while (FALSE);

	SAFE_DELETE(psb);

	return bRet;
 }

 /////////////////////////////////////////////////////////////////////////////