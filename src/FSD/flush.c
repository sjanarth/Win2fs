 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \FSD\flush.c
  *
  *     Abstract:		Volume and File flush code.
  *
  *     Author:		    Satish Kumar J
  *
  *		Date:			09-JUL-2004
  *
  *		Notes:			None.
  *
  *		Revision History:
  *
  *		Date		Version	Author				Changes
  *		------------------------------------------------------------------------
  *
  *		09-JUL-04	1.00	Satish Kumar J		Initial	Version.
  *
  *
  */

 //////////////////////////////////////////////////////////////////////////////

 // Include files.

 #include "ntifs.h"
 #include "ext2fs.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global data.

 extern PEXT2_GLOBAL gExt2Global;

 //////////////////////////////////////////////////////////////////////////////

 // Static data.

 #ifdef ALLOC_PRAGMA
	#pragma alloc_text(PAGE, Ext2FlushFile)
	#pragma alloc_text(PAGE, Ext2FlushVolume)
	#pragma alloc_text(PAGE, Ext2Flush)
 #endif

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 NTSTATUS
 Ext2FlushCompletionRoutine (
    IN PDEVICE_OBJECT	DeviceObject,
    IN PIRP				Irp,
    IN PVOID			Contxt )
 {
    if (Irp->PendingReturned)
	{
        IoMarkIrpPending(Irp);
	}

    if (Irp->IoStatus.Status == STATUS_INVALID_DEVICE_REQUEST)
	{
        Irp->IoStatus.Status = STATUS_SUCCESS;
	}

    return STATUS_SUCCESS;
 }

 NTSTATUS
 Ext2FlushVolume (
	IN PEXT2_VCB	Vcb, 
	BOOLEAN			bShutDown)
 {
    IO_STATUS_BLOCK		IoStatus;

    PEXT2_FCB			Fcb			= NULL;
    PLIST_ENTRY			ListEntry	= NULL;
    
    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
    {
        return STATUS_SUCCESS;
    }

    // Flush all Fcbs in Vcb list queue.
    {
        for (ListEntry = Vcb->FcbList.Flink;
             ListEntry != &Vcb->FcbList;
             ListEntry = ListEntry->Flink )
        {
            Fcb = CONTAINING_RECORD(ListEntry, EXT2_FCB, Next);
            
            if (ExAcquireResourceExclusiveLite(
						&Fcb->MainResource,
						TRUE ))
            {

                if (bShutDown)
				{
                    IoStatus.Status = Ext2PurgeFile(Fcb, TRUE);
				}
                else
				{
                    IoStatus.Status = Ext2FlushFile(Fcb);
				}

                ExReleaseResourceForThreadLite(
						&Fcb->MainResource,
						ExGetCurrentResourceThread()
					);
            }
        }
    }

    CcFlushCache(&(Vcb->SectionObject), NULL, 0, &IoStatus);

    return IoStatus.Status;       
 }

 NTSTATUS
 Ext2FlushFile (
	IN PEXT2_FCB Fcb)
 {
    IO_STATUS_BLOCK    IoStatus;

    ASSERT(Fcb != NULL);
        
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
		   (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    if (IsFlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
	{
        return STATUS_SUCCESS;
	}

    CcFlushCache(&(Fcb->SectionObject), NULL, 0, &IoStatus);
    ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);

    return IoStatus.Status;
 }

 NTSTATUS
 Ext2Flush (
	IN PEXT2_IRP_CONTEXT	IrpContext)
 {
    NTSTATUS                Status				= STATUS_UNSUCCESSFUL;

    PIRP                    Irp					= NULL;
    PIO_STACK_LOCATION      IrpSp				= NULL;
    PEXT2_VCB               Vcb					= NULL;
    PEXT2_FCBVCB            FcbOrVcb			= NULL;
    PFILE_OBJECT            FileObject			= NULL;
    PDEVICE_OBJECT          DeviceObject		= NULL;

    BOOLEAN                 MainResourceAcquired = FALSE;

    __try
    {
        ASSERT(IrpContext);
    
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        
        //
        // This request is not allowed on the main device object
        //

        if (DeviceObject == gExt2Global->DeviceObject)
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;

            __leave;
        }
        
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
       
        ASSERT(Vcb != NULL);

        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
			   (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
        {
            Status =  STATUS_SUCCESS;

            __leave;
        }

        Irp = IrpContext->Irp;
    
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        FileObject = IrpContext->FileObject;
        
        FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;
        
        ASSERT(FcbOrVcb != NULL);

        if (!ExAcquireResourceExclusiveLite(
                &FcbOrVcb->MainResource,
                IrpContext->IsSynchronous ))
        {
            Status = STATUS_PENDING;

            __leave;
        }
            
        MainResourceAcquired = TRUE;

        if (FcbOrVcb->Identifier.Type == EXT2VCB)
        {
            Status = Ext2FlushVolume((PEXT2_VCB)(FcbOrVcb), FALSE);

            if (NT_SUCCESS(Status) && IsFlagOn(Vcb->StreamObj->Flags, FO_FILE_MODIFIED))
            {
                ClearFlag(Vcb->StreamObj->Flags, FO_FILE_MODIFIED);
            }
                
        }
        else if (FcbOrVcb->Identifier.Type == EXT2FCB)
        {
            Status = Ext2FlushFile((PEXT2_FCB)(FcbOrVcb));

            if (NT_SUCCESS(Status) && IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED))
            {
                ClearFlag(FileObject->Flags, FO_FILE_MODIFIED);
            }
        }
    }
    __finally
    {
        if (MainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &FcbOrVcb->MainResource,
                ExGetCurrentResourceThread() );
        }

        if (!IrpContext->ExceptionInProgress)
        {
            if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
            {
                // Call the disk driver to flush the physial media.
                NTSTATUS DriverStatus;
                PIO_STACK_LOCATION NextIrpSp;

                NextIrpSp = IoGetNextIrpStackLocation(Irp);

                *NextIrpSp = *IrpSp;

                IoSetCompletionRoutine( Irp,
                                        Ext2FlushCompletionRoutine,
                                        NULL,
                                        TRUE,
                                        TRUE,
                                        TRUE );

                DriverStatus = IoCallDriver(Vcb->TargetDeviceObject, Irp);

                Status = (DriverStatus == STATUS_INVALID_DEVICE_REQUEST) ?
									Status : 
									DriverStatus;

                IrpContext->Irp = Irp = NULL;
            }
                
            Ext2FreeIrpContext(IrpContext);
        }
    }

    return Status;
 }

 //////////////////////////////////////////////////////////////////////////////