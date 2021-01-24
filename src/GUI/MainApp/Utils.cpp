
 /*                                                                            
  *		Copyright (c) Satish Kumar Janarthanan (vsat_in@yahoo.com) 2004 - 2005
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\MainApp\Utils.cpp
  *                                                                            
  *		Abstract:		Misc. utility functions.
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
 #include "Utils.h"
 
 #include "..\..\Other\Registry.h"

 //////////////////////////////////////////////////////////////////////////////

 // Static data.

 #define STR_SERVICE_NAME	"Win2fs"

 // The NT object name for disk drives.

 static LPCSTR gs_szVolName = (LPCSTR) "\\\\.\\PhysicalDrive%lu";

 struct SFilesystem
 {
	 __U8	nFSId;
	 __S8	szFilesys[20];
 };

 typedef struct SFilesystem	FILESYSTEM ;

 static FILESYSTEM gs_FilesystemTable[] = 
 {
    { 0x01, TEXT("MSDOS FAT12        ") },
    { 0x04, TEXT("MSDOS FAT16        ") },
    { 0x05, TEXT("Windows Extended   ") },
    { 0x06, TEXT("Windows FAT16      ") }, 
    { 0x07, TEXT("Windows NTFS       ") }, 
    { 0x0B, TEXT("Windows FAT32      ") }, 
    { 0x0C, TEXT("Large Windows FAT32") }, 
    { 0x0E, TEXT("LBA VFAT (16-Bit)  ") },
    { 0x0F, TEXT("LBA VFAT Extended  ") },
    { 0x11, TEXT("Hidden FAT12       ") },
    { 0x14, TEXT("Hidden DOS FAT16   ") },
    { 0x16, TEXT("Hidden FAT16       ") },
    { 0x17, TEXT("Hidden NTFS        ") },
    { 0x1B, TEXT("Hidden FAT32       ") },
    { 0x1C, TEXT("Hidden FAT32 LBA   ") },
    { 0x1E, TEXT("Hidden FAT16 LBA   ") },
    { 0x78, TEXT("XOSL Filesystem    ") },
    { 0x80, TEXT("Minix Filesystem   ") },
    { 0x81, TEXT("Old Linux          ") },
    { 0x82, TEXT("Linux Swap         ") }, 
    { 0x83, TEXT("Linux Ext2fs       ") }, 
    { 0x85, TEXT("Linux Extended     ") },
    { 0x00,	TEXT("Unknown            ") }  
 };                               

 struct SPartitionRecord
 {
	__U8	nDiskNumber;		//	1, 2, ...
	__U8	nPartNumber;		//  1, 2, ... (for this disk).
	__U8    nVolumeNumber;		//  1, 2, ... (Across all disks).
	__U32	nSectorCount;		//	The number of 512 byte sectors.
	__U32	nStartSector;		//	The starting sector number.
	__U8	nDriveLetter;		//	The driver leter (if mounted).
	__U8	nId;			    //	The type of the partition.
	PVOID	pDCB;			    //	Pointer to the device handle.
 };

 typedef struct SPartitionRecord PARTITION_RECORD ;

 struct SDriveLayoutInfo
 {
	 __U32	PartitionCount;
	 __U32	Signature;
	 PARTITION_INFORMATION	PartitionEntry[MAX_PART_RECORDS];
 };

 typedef struct SDriveLayoutInfo DRIVE_LAYOUT_INFO ;
 
 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 static __U32 UtilComputeFreeSpace (DWORD dwFree, DWORD dwSPC, DWORD dwBPS)
 {
	 __U32 nRet = 0;

	 if (dwFree >= 1024)
	 {
		 dwFree = dwFree / 1024;

		 if (dwSPC >= 2)
		 {
			 dwSPC = (dwSPC * dwBPS) / 1024;
			 nRet = dwFree * dwSPC;
		 }
		 else
		 {
			 nRet = (dwFree * dwSPC * dwBPS) / 1024;
		 }
	 }
	 else
	 {
		 if (dwSPC >= 2)
		 {
			 dwSPC = (dwSPC * dwBPS) / 1024;
			 nRet = (dwFree * dwSPC) / 1024;
		 }
		 else
		 {
			 nRet = (dwFree * dwSPC * dwBPS) / 1024;
		 }
	 }

	 return nRet;
 }

 static __U8 UtilFindVolumeMountPoint ( PARTITION_RECORD* Part )
 {
	__U8  nPos;
	__U8  chMountPt;
	BOOL  bCheck2;

	CHAR  szBuf[16];
	CHAR  szVolName1[128];
	CHAR  szVolName2[128];

	nPos = 2;
	chMountPt = 255;
	bCheck2 = FALSE;
	ZeroMemory (szBuf, sizeof (szBuf));
	ZeroMemory (szVolName1, sizeof (szVolName1));
	ZeroMemory (szVolName2, sizeof (szVolName2));

	// Volume to be queried.
	sprintf ( szVolName1, "\\Device\\HarddiskVolume%lu", Part->nVolumeNumber );

	CheckAgain:

	while (nPos <= 26)
	{
		sprintf (szBuf, "%C:", nPos+'A');
		if (QueryDosDevice (szBuf, szVolName2, 128))	
		{
			if (0 == strcmpi (szVolName1, szVolName2))
			{
				chMountPt = nPos ;

				break;
			}
		}
		nPos++;
	}

	if ( !isalpha (chMountPt+'A') && (!bCheck2) )
	{
		// Sometimes, volume names follow a different naming convention.
		sprintf ( 
				szVolName1, 
				"\\Device\\Harddisk%d\\Partition%d", 
				Part->nDiskNumber, 
				Part->nPartNumber+1
			);

		nPos = 2;
		bCheck2 = TRUE;

		goto CheckAgain;
	}

	return chMountPt;
 }

 static BOOL UtilGetPartitionRecords ( PARTITION_RECORD Partitions[MAX_PART_RECORDS] )
 {
	__U32	i;
	BOOL	bRet;
	HANDLE	hDev;				 
	__U32	nSize;
	__U32	nCount;
	__U32	nLastVolume;
	__U32	nVolumeNumber;
	__U32	nBytesReturned;
	CHAR	szDeviceName[512];

	DRIVE_LAYOUT_INFO DriveLayout;

	i = 0;
	nCount = 0;
	bRet = FALSE;
	nLastVolume = 0;
	nVolumeNumber = 0;
	nBytesReturned = 0;
	hDev = INVALID_HANDLE_VALUE;
	nSize = MAX_PART_RECORDS * sizeof ( PARTITION_RECORD );

	ZeroMemory ( &DriveLayout, sizeof ( DRIVE_LAYOUT_INFO ) );
	ZeroMemory ( szDeviceName, sizeof ( szDeviceName ) );

	for ( i = 0; i < (MAX_IDE_DISK+MAX_SCSI_DISK); i++ )
	{
		sprintf ( (PCHAR)szDeviceName, gs_szVolName, i );

		hDev = CreateFileA ( szDeviceName,
							 GENERIC_READ, 
							 FILE_SHARE_READ, 
							 NULL, OPEN_EXISTING, 0, NULL );

		BREAK_IF_EQUAL ( hDev, INVALID_HANDLE_VALUE );

		bRet = DeviceIoControl (hDev, 
								IOCTL_DISK_GET_DRIVE_LAYOUT,
								NULL,
								0, 
								&DriveLayout, 
								sizeof (DriveLayout),
								(PULONG) &nBytesReturned, 
								NULL);

		BREAK_IF_FALSE ( bRet ); 

		for ( nCount = 0, nLastVolume = 0; nCount < DriveLayout.PartitionCount; nCount++ )
		{
			if ( nCount > MAX_PART_RECORDS )
			{
				break;
			}

			Partitions->nDiskNumber = i;
			Partitions->nPartNumber = nCount;
			Partitions->nVolumeNumber = nVolumeNumber+DriveLayout.PartitionEntry[nCount].PartitionNumber;
			Partitions->nDriveLetter = UtilFindVolumeMountPoint (Partitions);
			Partitions->nId = DriveLayout.PartitionEntry[nCount].PartitionType;
			// Had to hack this !.
			__int64 t = DriveLayout.PartitionEntry[nCount].StartingOffset.QuadPart/512;			
			Partitions->nStartSector = (__U32) t;
			t = DriveLayout.PartitionEntry[nCount].PartitionLength.QuadPart/512;			
			Partitions->nSectorCount = (__U32) t;
			Partitions++;

			if ( nLastVolume < DriveLayout.PartitionEntry[nCount].PartitionNumber )
			{
				nLastVolume = DriveLayout.PartitionEntry[nCount].PartitionNumber;
			}
		}

		nVolumeNumber += nLastVolume;

		SAFE_CLOSE_HANDLE ( hDev );
	}

	SAFE_CLOSE_HANDLE ( hDev );

	return bRet;
 }

 static BOOL UtilSetupDriver2AutoStart (BOOL bYes)
 {
	 CRegDWORD dwStart (
			HKEY_LOCAL_MACHINE, 
			"SYSTEM\\CurrentControlSet\\Services\\Win2fs",
			"Start"
		);

	 if (bYes)
	 {
		 dwStart = 1;	// System start.
	 }
	 else
	 {
		 dwStart = 3;	// Manual start.	
	 }

	 return TRUE;
 }

 static BOOL UtilSetAutoMount (__U32 nDisk, __U32 nPartition, __U8 chDriveLetter)
 {
	 BOOL bRet = FALSE;

	 if (isalpha(chDriveLetter))
	 {
		 bRet = UtilSetupDriver2AutoStart (TRUE);

		 if (bRet)
		 {
			CHAR mp[32];
			sprintf (mp, "%C:", chDriveLetter);

			CHAR dev[128];
			sprintf (dev, "\\Device\\Harddisk%d\\Partition%d", nDisk, nPartition);

			CRegString rsDD(	
					HKEY_LOCAL_MACHINE, 
					"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\DOS Devices",
					mp
				);

			rsDD = dev;

			bRet = TRUE;
		 }
	 }

	 return bRet;
 }

 static BOOL UtilRemoveAutoMount (__U8 chDriveLetter)
 {
	 BOOL bRet = FALSE;

	 if (isalpha(chDriveLetter))
	 {
		CHAR mp[32];
		sprintf (mp, "%C:", chDriveLetter);

		CRegString rsDD(	
				HKEY_LOCAL_MACHINE, 
				"SYSTEM\\CurrenrControlSet\\Control\\Session Manager\\DOS Devices",
				mp
			);

		rsDD.RemoveValue ();

		bRet = TRUE;
	 }

	 return bRet;
 }

 //////////////////////////////////////////////////////////////////////////////

 // Global funtions.

 BOOL UtilGetPartitionTable ( PARTITION_INFO PartitionTable[MAX_PART_RECORDS] )
 {
	 __U32		i, j;
	 __U32		nVol;
	 BOOL		bRet;
	 BOOL		bFirstExtended;

	 PARTITION_RECORD PartitionRecords[MAX_PART_RECORDS];

	 bRet			= FALSE;
	 bFirstExtended = TRUE;
	 ZeroMemory ( PartitionRecords, sizeof ( PartitionRecords ) );

	 do
	 {
		 // Get the Partition records first.

		 bRet = UtilGetPartitionRecords ( PartitionRecords );

		 BREAK_IF_FALSE ( bRet );

		 // Map them to PARTITION_INFO records.

		 ZeroMemory ( PartitionTable, sizeof ( PartitionTable ) );

		 for ( i = 0, j = 0, nVol = 1; i < MAX_PART_RECORDS; i++ )
		 {
			 //
			 //	Win2000 inserts empty partition records, 
			 // to indicate the MBR of extended partitions.
			 // we must adjust for that.
			 //

			 if ( 0 == PartitionRecords[i].nSectorCount )
			 { 				 
				 continue;
			 }

			 //
			 // Leave out the 'inner' extended partitions themselves, 
			 // we are interested only in the logical drives they contain.
			 //

			 if ( IS_EXTENDED_PARTITION ( PartitionRecords[i].nId ) )
			 {
				 if ( ! bFirstExtended )
				 {
	 				continue;					 
				 }

				 bFirstExtended = FALSE;
			 }

			 //
			 // Are we done with the previous disk ?
			 //

			 if ( i > 0 )
			 {
				 if ( PartitionRecords[i].nDiskNumber != PartitionRecords[i-1].nDiskNumber )
				 {
					 nVol = 1;
				 }
			 }

			 PartitionTable[j].nDiskNumber = PartitionRecords[i].nDiskNumber;
			 PartitionTable[j].nPartNumber = nVol;
			 PartitionTable[j].nFilesystemId = PartitionRecords[i].nId;
			 
			 // 0 => A, 1 => B, etc

			 PartitionTable[j].chMountPoint = PartitionRecords[i].nDriveLetter + 'A';	

			 // In Mb.

			 PartitionTable[j].nSize = PartitionRecords[i].nSectorCount/2048;

			 if (isalpha(PartitionTable[j].chMountPoint))
			 {
				UtilGetFreeDiskSpace(PartitionTable[j].chMountPoint, &PartitionTable[j].nFree);
				UtilGetVolumeLabel(PartitionTable[j].chMountPoint, PartitionTable[j].szVolumeLabel);
			 }

			 PartitionTable[j].pReserved = &(PartitionRecords[i]);

			 // Move to the next slot.
			 j++;
			 nVol++;
		 }

	 } while ( FALSE );

	 return bRet;
 }

 __PU8 UtilGetFilesystemString ( __U8 FSId )
 {
    __U32 i;
	__U32 nCount;
	__PS8 szFileSys;	
		
	szFileSys = (__PS8) "Unknown";
	nCount = ARRAYSIZE (gs_FilesystemTable);
    
	for ( i = 0; i < nCount; i++)
	{
        if (gs_FilesystemTable[i].nFSId == FSId)  
		{
            return (__PU8) gs_FilesystemTable[i].szFilesys ;
        }
    }

	return (__PU8) "Unknown";
 }
 
 BOOL UtilGetFreeDiskSpace ( __U8 MntPt, __PU32 Free )
 {
	BOOL  bRet;
	DWORD dwSPC;
	DWORD dwBPS;
	DWORD dwFree;
	DWORD dwTotal;
	CHAR  buf[16];
	__U32 nRet;

	nRet = 0;
	dwSPC = 0;
	dwBPS = 0;
	dwFree = 0;
	dwTotal = 0;
	bRet = FALSE;
	ZeroMemory (buf, sizeof ( buf ) );

	sprintf(buf, "%C:\\", MntPt );
	bRet = GetDiskFreeSpace (buf, &dwSPC, &dwBPS, &dwFree, &dwTotal);

	if ( bRet )
	{
		(*Free) = UtilComputeFreeSpace (dwFree, dwSPC, dwBPS);
	}

	return bRet;
 }

 BOOL UtilGetVolumeLabel ( __U8 MntPt, CHAR szVolName[11] )
 {
	BOOL  bRet;
	CHAR  buf[16];

	bRet = FALSE;
	ZeroMemory (buf, sizeof ( buf ) );

	sprintf(buf, "%C:\\", MntPt );
	bRet = GetVolumeInformation (buf, szVolName, 11, NULL, NULL, NULL, NULL, 0);

	return bRet;
 }

 BOOL UtilGetDeviceName (PARTITION_INFO* Partition, __U8 szDevName[64])
 {
	sprintf ((PCHAR)szDevName, "/dev/hd%c%lu", Partition->nDiskNumber+'a', Partition->nPartNumber);	

	return TRUE;
 }

 BOOL UtilMount ( __U32 nDisk, __U32 nPartition, __U8  nDriveLetter, BOOL bPreserve )
 {
	BOOL bRet;
	CHAR szDevName[128];
	CHAR szMountPoint[32];
	
    sprintf (szDevName, "\\Device\\Harddisk%d\\Partition%d", nDisk, nPartition);

	sprintf (szMountPoint, "%C:", nDriveLetter);

	bRet = DefineDosDevice (DDD_RAW_TARGET_PATH, szMountPoint, szDevName);

	if (bRet)
	{
		// Do we need to make this mount persist across system restarts ?

		if (bPreserve)
		{
			bRet = UtilSetAutoMount (nDisk, nPartition, nDriveLetter);
		}
		else
		{
			bRet = UtilRemoveAutoMount (nDriveLetter);
		}
	}

	return bRet;
 }

 BOOL UtilUnMount ( __U8 nDriveLetter )
 {
	BOOL	bRet;
	HANDLE	hDev;
	DWORD   dwBytes;
	CHAR	szDevName[128];

	dwBytes = 0;
	bRet = FALSE;
	hDev = INVALID_HANDLE_VALUE;
	ZeroMemory ( szDevName, sizeof ( szDevName ) );

	do
	{
		sprintf ( szDevName, "\\\\.\\%C:", nDriveLetter );

		hDev = CreateFile(
					szDevName,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,	NULL );

		BREAK_IF_EQUAL (hDev, INVALID_HANDLE_VALUE);

		bRet = DeviceIoControl(
					hDev,
					FSCTL_LOCK_VOLUME,
					NULL, 0, NULL, 0, &dwBytes, NULL );

		BREAK_IF_FALSE (bRet);

		bRet = DeviceIoControl(
					hDev,
					FSCTL_DISMOUNT_VOLUME,
					NULL, 0, NULL, 0, &dwBytes,	NULL);

		BREAK_IF_FALSE (bRet);

		sprintf ( szDevName, "%C:", nDriveLetter );

		bRet = DefineDosDevice(
					DDD_REMOVE_DEFINITION,
					szDevName, NULL );

		if (bRet)
		{
			UtilRemoveAutoMount (nDriveLetter);
		}

	} while (FALSE);

	SAFE_CLOSE_HANDLE(hDev);

	return bRet;
 }

 //
 // Taken from Microsoft KB: Q118626.
 //

 // 
 // Make up some private access rights.
 // 
 
 #define ACCESS_READ  1
 #define ACCESS_WRITE 2

 BOOL UtilCheckIfAdmin () 
 {
	HANDLE hToken;
	DWORD  dwStatus;
	DWORD  dwAccessMask;
	DWORD  dwAccessDesired;
	DWORD  dwACLSize;
	PACL   pACL            = NULL;
	PSID   psidAdmin       = NULL;
	BOOL   bReturn         = FALSE;
	DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);

	PRIVILEGE_SET   ps;
	GENERIC_MAPPING GenericMapping;

	PSECURITY_DESCRIPTOR     psdAdmin           = NULL;
	SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

	do
	{
		// AccessCheck() requires an impersonation token.
		
		ImpersonateSelf (SecurityImpersonation);

		if (!OpenThreadToken (GetCurrentThread (), 
							  TOKEN_QUERY, 
							  FALSE, 
							  &hToken)) 
		{
			if (ERROR_NO_TOKEN != GetLastError ())
			{
				break;
			}

			//
			// If the thread does not have an access token, we'll 
			// examine the access token associated with the process.
			//

			if (!OpenProcessToken (GetCurrentProcess (), 
								   TOKEN_QUERY, 
								   &hToken))
			{
				break;
			}
		}

	    if (!AllocateAndInitializeSid (&SystemSidAuthority, 
									   2, 
									   SECURITY_BUILTIN_DOMAIN_RID, 
									   DOMAIN_ALIAS_RID_ADMINS,
									   0, 0, 0, 0, 0, 0, &psidAdmin))
		{
			break;
		}

		psdAdmin = LocalAlloc (LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

		BREAK_IF_NULL ( psdAdmin );

		if (!InitializeSecurityDescriptor (psdAdmin,
										   SECURITY_DESCRIPTOR_REVISION))
		{
			break;
		}
  
		//
		// Compute size needed for the ACL.
		//	

		dwACLSize = sizeof(ACL) + 
					sizeof(ACCESS_ALLOWED_ACE) +
					GetLengthSid(psidAdmin) - sizeof(DWORD);

		//
		// Allocate memory for ACL.
		//

		pACL = (PACL) LocalAlloc (LPTR, dwACLSize);

		BREAK_IF_NULL ( pACL );

		//
		// Initialize the new ACL.
		//

		if (!InitializeAcl(pACL, dwACLSize, ACL_REVISION2))
		{
			break;
		}		

		dwAccessMask = ACCESS_READ | ACCESS_WRITE;
	      
		//
		// Add the access-allowed ACL to the DACL.
		//

		if (!AddAccessAllowedAce (pACL, 
								  ACL_REVISION2,
								  dwAccessMask, 
								  psidAdmin))
		{
			break;
		}

		//
		// Set our DACL to the SD.
		//

		if (!SetSecurityDescriptorDacl (psdAdmin, 
										TRUE, 
										pACL, 
										FALSE))
		{
			break;
		}

		//
		// AccessCheck is sensitive about what is in the SD; set
		// the group and owner.
		//

		SetSecurityDescriptorGroup (psdAdmin, psidAdmin, FALSE);
		SetSecurityDescriptorOwner (psdAdmin, psidAdmin, FALSE);

		if (!IsValidSecurityDescriptor (psdAdmin))
		{
			break;
		}

		dwAccessDesired = ACCESS_READ;

		// 
		// Initialize GenericMapping structure even though we
		// won't be using generic rights.
		// 

		GenericMapping.GenericRead    = ACCESS_READ;
		GenericMapping.GenericWrite   = ACCESS_WRITE;
		GenericMapping.GenericExecute = 0;
		GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;

		if (!AccessCheck(psdAdmin, 
						 hToken, 
					     dwAccessDesired, 
					     &GenericMapping, 
					     &ps, 
					     &dwStructureSize, &dwStatus, &bReturn)) 
		{
			break;
		}

		RevertToSelf ();
   
	} while (FALSE);

	// Cleanup. 

	if (pACL)		LocalFree (pACL);
	if (psdAdmin)	LocalFree (psdAdmin);  
	if (psidAdmin)	FreeSid (psidAdmin);

	return bReturn;
 }

 BOOL UtilCheckDriverStatus ()
 {
	 BOOL bRet;
	 SC_HANDLE hSCM;
	 SC_HANDLE hService;
	 SERVICE_STATUS srvStatus;

	 bRet = FALSE;
	 hSCM = NULL;
	 hService = NULL;
	 ZeroMemory (&srvStatus, sizeof (srvStatus));

	 do
	 {
		 // Open a handle to the Service Control Manager.
		 hSCM = OpenSCManager ( NULL, NULL, SC_MANAGER_ALL_ACCESS);

		 BREAK_IF_NULL (hSCM);

		 // Open a handle to the FSD service.
		 hService = OpenService ( hSCM, 
								  STR_SERVICE_NAME, 
								  SERVICE_QUERY_STATUS|SERVICE_START);

		 BREAK_IF_NULL (hService);

		 // Check the status of the service.
		 bRet = QueryServiceStatus ( hService, &srvStatus );

		 BREAK_IF_FALSE (bRet);

		 if ( SERVICE_RUNNING == srvStatus.dwCurrentState )
		 {
			 bRet = TRUE;

			 break;
		 }

		 // Try starting the service.
		 bRet = StartService ( hService, 0, NULL );

		 // StartService may also fail if the servcie is already running.
		 if (FALSE == bRet)
		 {
			 if (ERROR_SERVICE_ALREADY_RUNNING == GetLastError())
			 {
				 bRet = TRUE;
			 }
		 }

	 } while (FALSE);

	 if (NULL != hService)
	 {
		 CloseServiceHandle (hService);
	 }

	 if (NULL != hSCM)
	 {
		 CloseServiceHandle (hSCM);
	 }

	 return bRet;
 }

 VOID UtilShowMessage (LPCTSTR sErrMsg, DWORD dwStyle, DWORD dwErrCode )
 {
	 LPVOID lpMsgBuf;

	 if (ERROR_SUCCESS != dwErrCode)
	 {
		FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, dwErrCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	
				(LPTSTR) &lpMsgBuf,	0, NULL 
			);

		CString str;

		str.Format ("%s\r\n\r\nError Code:       0x%08lX\r\n\r\nError Message: %s", sErrMsg, dwErrCode, lpMsgBuf );
	 
		AfxMessageBox (str, dwStyle);
	 
		LocalFree (lpMsgBuf);
	 }
	 else
	 {
		AfxMessageBox (sErrMsg, dwStyle);
	 }
 }

 /////////////////////////////////////////////////////////////////////////////
