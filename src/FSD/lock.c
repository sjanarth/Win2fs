 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \FSD\lock.c
  *
  *     Abstract:		Lock control API.
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
	#pragma alloc_text(PAGE, Ext2LockControl)
 #endif

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 NTSTATUS
 Ext2LockControl (
	IN PEXT2_IRP_CONTEXT IrpContext)
 {
    NTSTATUS        Status			= STATUS_UNSUCCESSFUL;
    BOOLEAN         CompleteRequest	= FALSE;

    PDEVICE_OBJECT  DeviceObject	= NULL;
    PFILE_OBJECT    FileObject		= NULL;
    PEXT2_FCB       Fcb				= NULL;
    PIRP            Irp				= NULL;
    
    __try
    {
        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        if (DeviceObject == gExt2Global->DeviceObject)
        {
            CompleteRequest = TRUE;

            Status = STATUS_INVALID_DEVICE_REQUEST;

            __leave;
        }
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        
        ASSERT(Fcb != NULL);
        
        if (Fcb->Identifier.Type == EXT2VCB)
        {
            CompleteRequest = TRUE;

            Status = STATUS_INVALID_PARAMETER;

            __leave;
        }
        
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
        
        if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        {
            CompleteRequest = TRUE;

            Status = STATUS_INVALID_PARAMETER;

            __leave;
        }
        
        Irp = IrpContext->Irp;
        
        //
        // While the file has any byte range locks we set IsFastIoPossible to
        // FastIoIsQuestionable so that the FastIoCheckIfPossible function is
        // called to check the locks for any fast I/O read/write operation.
        //

        if (Fcb->CommonFCBHeader.IsFastIoPossible != FastIoIsQuestionable)
        {
 #if DBG
            Ext2DbgPrint(D_LOCK, "Ext2LockControl: %-16.16s %-31s %s\n",
					Ext2DbgGetCurrentProcessName(),
					"FastIoIsQuestionable",
					Fcb->AnsiFileName.Buffer
                );
 #endif          
            Fcb->CommonFCBHeader.IsFastIoPossible = FastIoIsQuestionable;
        }
        
        //
        // FsRtlProcessFileLock acquires FileObject->FsContext->Resource while
        // modifying the file locks and calls IoCompleteRequest when it's done.
        //
        
        CompleteRequest = FALSE;
        
        Status = FsRtlProcessFileLock(
            &Fcb->FileLockAnchor,
            Irp,
            NULL        );
        
        if (Status != STATUS_SUCCESS)
        {
 #if DBG
            Ext2DbgPrint(D_LOCK, "Ext2LockControl: %-16.16s %-31s *** Status: %s (%#x) ***\n",
					Ext2DbgGetCurrentProcessName(),
					"IRP_MJ_LOCK_CONTROL",
					Ext2DbgNtStatusToString(Status),
					Status          
				);
 #endif
        }
    }
    __finally
    {
        if (!IrpContext->ExceptionInProgress)
        {
            if (CompleteRequest)
            {
                IrpContext->Irp->IoStatus.Status = Status;
                
                Ext2CompleteRequest(
						IrpContext->Irp,
						(CCHAR)	(NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT)
                    );
            }
            
            Ext2FreeIrpContext(IrpContext);
        }
    }
    
    return Status;
 }

 //////////////////////////////////////////////////////////////////////////////