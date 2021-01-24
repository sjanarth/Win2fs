# Win2fs
Win2fs is an Ext2 file system driver for Microsoft Windows family of operating systems. Win2fs allows Linux Ext2 file systems to be "mounted" and used on a Microsoft Windows system much like the native FAT/NT file systems available on these platforms. Unlike other Windows apps for the Ext2 file system, Win2fs integrates these file systems well with the Windows OS itself, there by bringing out the best out of both.

The problem of sharing files across the Linux and Windows operating systems has been around for quite sometime. A number of solutions (like Samba, NFS, etc) have been put forth for addressing this. But it should be noted that most of these solutions are of no help to the large number of casual home users who use a single computing system with both Linux and Windows installed on the same machine. Such users have been forced to rely on third party software and voluntary Windows programs (like Win2fs) to access files stored on their Linux file-systems on Windows.

With Win2fs installed, ext2 volumes are assigned drive letters like G:, etc (also called 'mount points') and all running applications can directly access the files on these file-systems. Files and directories appear in all the file open/save dialogs of all applications and user don't have to copy files back and forth between Linux and Windows file-systems.

Win2fs started as a university project in late 2001 (originally hosted at http://win2fs.sourceforge.net/) and has since grown into a stable product fully integrated with Windows Storage subsystems.
