# Win2fs
Win2fs is an Ext2 file system driver for Microsoft Windows family of operating systems. Win2fs allows Linux Ext2 file systems to be "mounted" and used on a Microsoft Windows system much like the native FAT/NT file systems available on these platforms. Unlike other Windows apps for the Ext2 file system, Win2fs integrates these file systems well with the Windows OS itself, there by bringing out the best out of both.

The problem of sharing files across the Linux and Windows operating systems has been around for quite sometime. A number of solutions (like Samba, NFS, etc) have been put forth for addressing this. But it should be noted that most of these solutions are of no help to the large number of casual home users who use a single computing system with both Linux and Windows installed on the same machine. Such users have been forced to rely on third party software and voluntary Windows programs (like Win2fs) to access files stored on their Linux file-systems on Windows.

With Win2fs installed, ext2 volumes are assigned drive letters like G:, etc (also called 'mount points') and all running applications can directly access the files on these file-systems. Files and directories appear in all the file open/save dialogs of all applications and user don't have to copy files back and forth between Linux and Windows file-systems.

Win2fs started as a university project in late 2001 (originally hosted at http://win2fs.sourceforge.net/) and has since grown into a stable product fully integrated with Windows Storage subsystems.

## Features
	Support for most Windows operating systems
	Complete read and write support to almost all releases of ext2/3/4fs
	Supports Windows style Fast IO (better caching and hence better performance)
	Support for USB disk drives
	Completely automated install and uninstall tools
	Option to format and check the volume for consistency
	Option to automatically mount selected volumes
	Support for all block sizes
	Handling of symbolic links
	Support for the traditional UNIX style three tier file permissions
	Keeping track of file-system state
	Support for the Large File (4GB+) API

## Screenshots
<p align="center">
  <img alt="Main Window" src="http://github.com/sjanarth/win2fs/tree/master/images/mainapp.jpg" />
</p>
<p align="center">
  <img alt="Properties Dialog" src="http://github.com/sjanarth/win2fs/tree/master/images/proppage.jpg" />
</p>
<p align="center">
  <img alt="Windows File Explorer" src="http://github.com/sjanarth/win2fs/tree/master/images/explorer.jpg" />
</p>
