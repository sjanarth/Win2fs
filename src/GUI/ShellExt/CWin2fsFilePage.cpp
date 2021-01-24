 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CWin2fsFilePage.cpp
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
 #include "CWin2fsFilePage.h"

 #include "ioctls.h"
 #include "linux\ext2_fs.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global data.

 BEGIN_MESSAGE_MAP(CWin2fsFilePage, CWin2fsPage)
 END_MESSAGE_MAP()

 #define IsBitSet(v,b)	(((b)&(v)) != 0)

 #ifndef __EXT2FS_H

	 #define S_IFMT   0x0F000            /*017 0000 */

	 #define S_IFSOCK 0x0C000            /*014 0000 */
	 #define S_IFLNK  0x0A000            /*012 0000 */
	 #define S_IFFIL  0x08000            /*010 0000 */
	 #define S_IFBLK  0x06000            /*006 0000 */
	 #define S_IFDIR  0x04000            /*004 0000 */
	 #define S_IFCHR  0x02000            /*002 0000 */
	 #define S_IFIFO  0x01000            /*001 0000 */

	 #define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)
	 #define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
	 #define S_ISFIL(m)      (((m) & S_IFMT) == S_IFFIL)
	 #define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
	 #define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
	 #define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)
	 #define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)
 
	 #define S_IPERMISSION_MASK 0x1FF 

	 #define S_IRWXU 0x01C0     /*  00700 */
	 #define S_IRUSR 0x0100     /*  00400 */
	 #define S_IWUSR 0x0080     /*  00200 */
	 #define S_IXUSR 0x0040     /*  00100 */

	 #define S_IRWXG 0x0038     /*  00070 */
	 #define S_IRGRP 0x0020     /*  00040 */
	 #define S_IWGRP 0x0010     /*  00020 */
	 #define S_IXGRP 0x0008     /*  00010 */
 
	 #define S_IRWXO 0x0007     /*  00007 */
	 #define S_IROTH 0x0004     /*  00004 */
	 #define S_IWOTH 0x0002     /*  00002 */
	 #define S_IXOTH 0x0001     /*  00001 */

 #endif		// __EXT2FS_H

 //////////////////////////////////////////////////////////////////////////////

 // Static data.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 static CString GetType (__U16 mode)
 {
	 CString str = "Unknown";

	 if (S_ISSOCK(mode))
		 str = "Socket";
	 else if (S_ISLNK(mode))
		 str = "Link";
	 else if (S_ISFIL(mode))
		 str = "Regular file";
	 else if (S_ISBLK(mode))
		 str = "Block device";
	 else if (S_ISDIR(mode))
		 str = "Directory";
	 else if (S_ISCHR(mode))
		 str = "Char device";
	 else if (S_ISFIFO(mode))
		 str = "FIFO";

	 return str;
 }

 static CString FormatSize (__U32 size)
 {
	 CString str = "Unknown";

	 __U32	oneKB	= 1000;
	 __U32	oneMB	= (1000*oneKB);
	 __U32	oneGB	= (1000*oneMB);

	 if ((size/oneGB) > 0)
	 {
		 str.Format ("%-2.2f GB", (float) ((float)size/oneGB));
	 }
	 else if ((size/oneMB) > 0)
	 {
		 str.Format ("%-2.2f MB", (float) ((float)size/oneMB));
	 }
	 else
	 {
		 str.Format ("%u bytes", size);
	 }

	 return str;
 }

 //////////////////////////////////////////////////////////////////////////////

 // Restricted functions.

 // Privates'.

 // None.

 // Protected.

 VOID CWin2fsFilePage::CheckFilePerms (ext2_inode* pi)
 {
	 if (pi != NULL)
	 {
		 CheckDlgButton (IDC_CHECK_UREAD,	IsBitSet (pi->i_mode, S_IRUSR));
		 CheckDlgButton (IDC_CHECK_UWRITE,	IsBitSet (pi->i_mode, S_IWUSR));
		 CheckDlgButton (IDC_CHECK_UEXEC,	IsBitSet (pi->i_mode, S_IXUSR));
		 CheckDlgButton (IDC_CHECK_SUID,	IsBitSet (pi->i_mode, S_IRUSR));

		 CheckDlgButton (IDC_CHECK_GREAD,	IsBitSet (pi->i_mode, S_IRGRP));
		 CheckDlgButton (IDC_CHECK_GWRITE,	IsBitSet (pi->i_mode, S_IWGRP));
		 CheckDlgButton (IDC_CHECK_GEXEC,	IsBitSet (pi->i_mode, S_IXGRP));
		 CheckDlgButton (IDC_CHECK_SGID,	IsBitSet (pi->i_mode, S_IRUSR));

		 CheckDlgButton (IDC_CHECK_OREAD,	IsBitSet (pi->i_mode, S_IROTH));
		 CheckDlgButton (IDC_CHECK_OWRITE,	IsBitSet (pi->i_mode, S_IWOTH));
		 CheckDlgButton (IDC_CHECK_OEXEC,	IsBitSet (pi->i_mode, S_IXOTH));
	 }
 }
 
 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 IMPLEMENT_DYNAMIC(CWin2fsFilePage, CWin2fsPage)

 CWin2fsFilePage::CWin2fsFilePage( const CString& strPath, BOOL bAutoDestroy ) 
			:CWin2fsPage( IDD_WIN2FS_FILEPAGE, strPath, bAutoDestroy )
 {
 }

 CWin2fsFilePage::~CWin2fsFilePage()
 {
 }

 BOOL CWin2fsFilePage::OnInitDialog() 
 {
	CWin2fsPage::OnInitDialog();

	LoadInode ();

	return TRUE;  // return TRUE unless you set the focus to a control
 }

 /////////////////////////////////////////////////////////////////////////////

 // Message handlers.

 // None.

 /////////////////////////////////////////////////////////////////////////////

 // Protected/private methods.

 BOOL CWin2fsFilePage::LoadInode ()
 {
	BOOL bRet = FALSE;

	ext2_inode* pi = NULL;

	do
	{
		pi = ReadInode ();

		BREAK_IF_NULL(pi);

		AddProperty ("Inode #",			pi->osd1.linux1.l_i_reserved1);
		AddProperty ("File type",		GetType (pi->i_mode));
		AddProperty ("Links count",		pi->i_links_count);
		AddProperty ("User ID",			((pi->osd2.linux2.l_i_uid_high<<16)| pi->i_uid));
		AddProperty ("Group ID",		((pi->osd2.linux2.l_i_gid_high<<16)| pi->i_gid));
		AddProperty ("Size",			FormatSize(pi->i_size));
		AddProperty ("Created",			ctime ((const long*)&pi->i_ctime));
		AddProperty ("Accessed",		ctime ((const long*)&pi->i_atime));
		AddProperty ("Modified",		ctime ((const long*)&pi->i_mtime));

		CheckFilePerms (pi);

	} while (FALSE);

	SAFE_DELETE (pi);

	return bRet;
 }

 /////////////////////////////////////////////////////////////////////////////