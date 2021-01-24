
 /*                                                                            
  *		Copyright (c) Satish Kumar Janarthanan (vsat_in@yahoo.com) 2004 - 2005
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\MainApp\Utils.h
  *                                                                            
  *		Abstract:		Header for the utility functions.
  *
  *		Notes:			None.
  *
  *		Revision History:
  *
  *		Date		Version		Author				Change Log
  *		------------------------------------------------------------------------
  *
  *		03-DEC-05	0.0.1		Satish Kumar J		Initial Version
  */                          		

 #ifndef	__UTILS_H
 #define	__UTILS_H

 /////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "StdAfx.h"

 /////////////////////////////////////////////////////////////////////////////

 // Macros and typedefs.

 // Pseudo codes for partition classifications.

 #define PT_PRIMARY			0x04
 #define PT_SYSTEM			0x08
 #define PT_READONLY		0x10
 #define PT_ENCRYPTION		0x20
 #define PT_COMPRESSION		0x40

 #define IS_PRIMARY(t)		(PT_PRIMARY == ((t)&PT_PRIMARY))
 #define IS_SYSTEM(t)		(PT_SYSTEM == ((t)&PT_SYSTEM))
 #define IS_READONLY(t)		(PT_READONLY == ((t)&PT_READONLY))
 #define IS_ENCRYPTED(t)	(PT_ENCRYPTION == ((t)&PT_ENCRYPTION))
 #define IS_COMPRESSED(t)	(PT_COMPRESSION == ((t)&PT_COMPRESSION))

 // Maximums we can handle.

 #define MAX_IDE_DISK 			    4
 #define MAX_SCSI_DISK 			    8	
 #define MAX_PARTITION_PER_DISK	    16
 #define MAX_PART_RECORDS			(MAX_PARTITION_PER_DISK * (MAX_IDE_DISK + MAX_SCSI_DISK))

 // Check if a given FS Id represents an extended partition.

 #define IS_EXTENDED_PARTITION(p)	((0x05 == (p)) || (0x0F == (p)) || (0x85 == (p)))

 // Values for the type parameter of UtilLogMessage.

 #define MT_INFO					(MB_OK|MB_ICONINFORMATION)
 #define MT_WARNING					(MB_OK|MB_ICONWARNING)
 #define MT_ERROR					(MB_OK|MB_ICONERROR)

 #pragma pack(1)

 struct SPartitionInfo
 {
	__U8	nDiskNumber;		// 0, 1, ...
	__U8	nPartNumber;		// 0, 1, ... (for this disk)
	__U8	nFilesystemId;		// 0x83, 0x0B, ...
	__U8	chMountPoint;		// A, C, D ... (if mounted)
	CHAR	szVolumeLabel[11];	// DOS volume label.
	__U32	nSize;				// The size of the partition in Mb.											
	__U32	nFree;				// The free space available in the volume in Mb.											
	__U8	nType;				// Primary, System, etc
	PVOID	pReserved;		
 };

 typedef struct SPartitionInfo PARTITION_INFO;

 #pragma pack()

 /////////////////////////////////////////////////////////////////////////////

 // Global declarations.

 BOOL	UtilGetPartitionTable ( PARTITION_INFO PartitionTable[MAX_PART_RECORDS] );

 __PU8	UtilGetFilesystemString ( __U8 FSId );

 BOOL	UtilGetFreeDiskSpace ( __U8 MntPt, __PU32 Free );

 BOOL   UtilGetVolumeLabel ( __U8 MntPt, CHAR szVolName[11] );

 BOOL	UtilGetDeviceName (PARTITION_INFO* Partition, __U8 szDevName[64]);

 BOOL	UtilMount ( __U32 nDisk, __U32 nPartition, __U8 nDriveLetter, BOOL bPreserve );
 
 BOOL	UtilUnMount ( __U8  nDriveLetter );

 BOOL	UtilCheckIfAdmin ();

 BOOL	UtilCheckDriverStatus ();

 VOID	UtilShowMessage (LPCTSTR sMsg, DWORD dwType = MT_INFO, DWORD dwErrCode = ERROR_SUCCESS );
 
 /////////////////////////////////////////////////////////////////////////////
 
 #endif		// __UTILS_H