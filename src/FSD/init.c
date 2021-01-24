
 /*                                                                            
  *		Copyright (c) 2001 - 2010 Satish Kumar Janarthanan (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\FSD\Init.c
  *                                                                            
  *		Abstract:		The main entry point for the driver.
  *
  *		Notes:			None.
  *
  *		Revision History:
  *
  *		Date		Version		Author				Change Log
  *		------------------------------------------------------------------------
  *
  *					0.0.1		Satish Kumar J		Initial Version
  */       


 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "ext2fs.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global, External declarations.

 #ifdef ALLOC_PRAGMA
	#pragma alloc_text(INIT, Ext2QueryRegistry)
	#pragma alloc_text(INIT, DriverEntry)
 #endif

 PEXT2_GLOBAL gExt2Global = NULL;

 //////////////////////////////////////////////////////////////////////////////

 // Static data.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 static USHORT GetMaxDepth ()
 {
	USHORT lRet = 16;

	ENTER (D_INIT, "GetMaxDepth");

    switch ( MmQuerySystemSize() ) 
	{
	    case MmSmallSystem:
			lRet = 16;
	        break;
	    case MmMediumSystem:
	        lRet = 64;
		    break;
	    case MmLargeSystem:
		    lRet = 256;
			break;
	}

	Ext2DbgPrint (D_INIT, "Leaving GetMaxDepth, returning %u", lRet);

	return lRet;
 }

 static VOID SetupDriverObject (IN PDRIVER_OBJECT DriverObject)
 {
	 ENTER (D_INIT, "SetupDriverObject");

	 if (DriverObject != NULL)
	 {
		Ext2DbgPrint (D_INIT, "Setting up IRP MajorFunction callbacks ...");

		DriverObject->MajorFunction[IRP_MJ_CREATE]              = Ext2BuildRequest;
		DriverObject->MajorFunction[IRP_MJ_CLOSE]               = Ext2BuildRequest;
		DriverObject->MajorFunction[IRP_MJ_READ]                = Ext2BuildRequest;
		DriverObject->MajorFunction[IRP_MJ_WRITE]               = Ext2BuildRequest;

		DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]       = Ext2BuildRequest;
		DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]	        = Ext2BuildRequest;

		DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]   = Ext2BuildRequest;
		DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]     = Ext2BuildRequest;

		DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION]    = Ext2BuildRequest;
		DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]      = Ext2BuildRequest;

		DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]   = Ext2BuildRequest;
		DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = Ext2BuildRequest;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]      = Ext2BuildRequest;
		DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]        = Ext2BuildRequest;

		DriverObject->MajorFunction[IRP_MJ_CLEANUP]             = Ext2BuildRequest;
		DriverObject->DriverUnload                              = NULL;
	 }

	 LEAVE (D_INIT, "SetupDriverObject");
 }

 static VOID SetupDeviceExtension (
		IN PDRIVER_OBJECT DriverObject,
		IN PDEVICE_OBJECT DeviceObject
	)
 {
     PEXT2FS_EXT                 DeviceExt;
	 PFAST_IO_DISPATCH           FastIoDispatch;
     PCACHE_MANAGER_CALLBACKS    CacheManagerCallbacks;

	 ENTER (D_INIT, "SetupDeviceExtension");

	 do
	 {
		BREAK_IF_NULL (DeviceObject);

		Ext2DbgPrint (D_INIT, "Setting up Device Extension ...");
		
		DeviceExt = (PEXT2FS_EXT) DeviceObject->DeviceExtension;
		RtlZeroMemory(DeviceExt, sizeof(EXT2FS_EXT));

		// Setup the gExt2Global structure.	    
		gExt2Global = &(DeviceExt->gExt2Global);
		//TODO: Need to setup gExt2Global->TimeZone here.
		gExt2Global->Identifier.Type = EXT2FGD;
		gExt2Global->Identifier.Size = sizeof(EXT2_GLOBAL);
		gExt2Global->DeviceObject = DeviceObject;
		gExt2Global->DriverObject = DriverObject;
		gExt2Global->MaxDepth = GetMaxDepth ();

		// Initialize the fast I/O entry points
		FastIoDispatch = &(gExt2Global->FastIoDispatch);
		FastIoDispatch->SizeOfFastIoDispatch        = sizeof(FAST_IO_DISPATCH);
		FastIoDispatch->FastIoCheckIfPossible       = Ext2FastIoCheckIfPossible;
	#ifdef DEBUG
		FastIoDispatch->FastIoRead                  = Ext2FastIoRead;
		FastIoDispatch->FastIoWrite                 = Ext2FastIoWrite;
	#else
		FastIoDispatch->FastIoRead                  = FsRtlCopyRead;
		FastIoDispatch->FastIoWrite                 = FsRtlCopyWrite;
	#endif
		FastIoDispatch->FastIoQueryBasicInfo        = Ext2FastIoQueryBasicInfo;
		FastIoDispatch->FastIoQueryStandardInfo     = Ext2FastIoQueryStandardInfo;
		FastIoDispatch->FastIoLock                  = Ext2FastIoLock;
		FastIoDispatch->FastIoUnlockSingle          = Ext2FastIoUnlockSingle;
		FastIoDispatch->FastIoUnlockAll             = Ext2FastIoUnlockAll;
		FastIoDispatch->FastIoUnlockAllByKey        = Ext2FastIoUnlockAllByKey;
		FastIoDispatch->FastIoQueryNetworkOpenInfo  = Ext2FastIoQueryNetworkOpenInfo;

		DriverObject->FastIoDispatch = FastIoDispatch;

		// Initialize the Cache Manager callbacks
		CacheManagerCallbacks = &(gExt2Global->CacheManagerCallbacks);
		CacheManagerCallbacks->AcquireForLazyWrite  = Ext2AcquireForLazyWrite;
		CacheManagerCallbacks->ReleaseFromLazyWrite = Ext2ReleaseFromLazyWrite;
		CacheManagerCallbacks->AcquireForReadAhead  = Ext2AcquireForReadAhead;
		CacheManagerCallbacks->ReleaseFromReadAhead = Ext2ReleaseFromReadAhead;

		gExt2Global->CacheManagerNoOpCallbacks.AcquireForLazyWrite  = Ext2NoOpAcquire;
		gExt2Global->CacheManagerNoOpCallbacks.ReleaseFromLazyWrite = Ext2NoOpRelease;
		gExt2Global->CacheManagerNoOpCallbacks.AcquireForReadAhead  = Ext2NoOpAcquire;
		gExt2Global->CacheManagerNoOpCallbacks.ReleaseFromReadAhead = Ext2NoOpRelease;

		// Initialize the global data
		InitializeListHead(&(gExt2Global->VcbList));
		ExInitializeResourceLite(&(gExt2Global->Resource));

		ExInitializeNPagedLookasideList( &(gExt2Global->Ext2IrpContextLookasideList),
										 NULL,
										 NULL,
										 0,
										 sizeof(EXT2_IRP_CONTEXT),
										 '2TXE',
										 gExt2Global->MaxDepth);

		ExInitializeNPagedLookasideList( &(gExt2Global->Ext2FcbLookasideList),
										 NULL,
										 NULL,
										 0,
										 sizeof(EXT2_FCB),
										 '2TXE',
										 gExt2Global->MaxDepth);

		ExInitializeNPagedLookasideList( &(gExt2Global->Ext2CcbLookasideList),
										 NULL,
										 NULL,
										 0,
										 sizeof(EXT2_CCB),
										 '2TXE',
										 (USHORT)(gExt2Global->MaxDepth << 1));

		ExInitializePagedLookasideList( &(gExt2Global->Ext2McbLookasideList),
										 NULL,
										 NULL,
										 0,
										 sizeof(EXT2_MCB),
										 '2TXE',
										 (USHORT)(gExt2Global->MaxDepth << 1));

	 } while (FALSE);

	 LEAVE (D_INIT, "SetupDeviceExtension");
 }

 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 NTSTATUS DriverEntry (
		IN PDRIVER_OBJECT   DriverObject,
		IN PUNICODE_STRING  RegistryPath
	)
 {
    PDEVICE_OBJECT              DeviceObject;
    UNICODE_STRING              DeviceName;
    NTSTATUS                    Status;

	Ext2DbgInitialize (DOUT_FILE, "\\??\\C:\\Win2fs.log");

    ENTER (D_INIT, "DriverEntry");

	RtlInitUnicodeString(&DeviceName, DEVICE_NAME);

	do
	{
	    Ext2DbgPrint(D_INIT, "Ext2 File System Driver.");

		Ext2DbgPrint(D_INIT, " " __DATE__ " " __TIME__
 #ifdef DEBUG
			", Checked"
 #endif
			", _WIN32_WINNT=%#x.", _WIN32_WINNT);

		Ext2DbgPrint (D_INIT, "Calling IoCreateDevice ...");

		Status = IoCreateDevice(
					DriverObject,
					sizeof(EXT2FS_EXT),
					&DeviceName,
					FILE_DEVICE_DISK_FILE_SYSTEM,
					0,
					FALSE,
					&DeviceObject );

		BREAK_IF_FAILED (Status);

		SetupDriverObject (DriverObject);

		SetupDeviceExtension (DriverObject, DeviceObject);

 #ifdef DEBUG
		gProcessNameOffset = Ext2GetProcessNameOffset();
 #endif

		Ext2DbgPrint (D_INIT, "Calling IoRegisterFileSystem ...");

	    IoRegisterFileSystem (DeviceObject);
    
	} while (FALSE);

	LEAVE3 (D_INIT, "DriverEntry");
    
    return Status;
 }