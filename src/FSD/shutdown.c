 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \FSD\shutdown.c
  *
  *     Abstract:		Code to handle IRP_MJ_SHUTDOWN.
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
	#pragma alloc_text(PAGE, Ext2ShutDown)
 #endif

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Global functions.
 
 NTSTATUS
 Ext2ShutDown (
	IN PEXT2_IRP_CONTEXT IrpContext)
 {
    NTSTATUS                Status		= STATUS_UNSUCCESSFUL;

    PKEVENT                 Event		= NULL;
    PIRP                    Irp			= NULL;
    PIO_STACK_LOCATION      IrpSp		= NULL;
    PEXT2_VCB               Vcb			= NULL;
    PLIST_ENTRY             ListEntry	= NULL;

    BOOLEAN                 GlobalResourceAcquired = FALSE;

    __try
    {
        ASSERT(IrpContext);
    
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        Status = STATUS_SUCCESS;

        Irp = IrpContext->Irp;
    
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        if (!ExAcquireResourceExclusiveLite(
                &gExt2Global->Resource,
                IrpContext->IsSynchronous )
			)
        {
            Status = STATUS_PENDING;

            __leave;
        }
            
        GlobalResourceAcquired = TRUE;

        Event = ExAllocatePool( NonPagedPool, sizeof(KEVENT));
        KeInitializeEvent(Event, NotificationEvent, FALSE );

        for (ListEntry = gExt2Global->VcbList.Flink;
             ListEntry != &(gExt2Global->VcbList);
             ListEntry = ListEntry->Flink )
        {
            Vcb = CONTAINING_RECORD(ListEntry, EXT2_VCB, Next);

            if (ExAcquireResourceExclusiveLite(
					&Vcb->MainResource,
					TRUE )
				)
            {

                Status = Ext2FlushVolume(Vcb, TRUE);
                
                CcPurgeCacheSection(&Vcb->SectionObject,
                                    NULL,
                                    0,
                                    FALSE );

                Ext2DiskShutDown(Vcb);

                ExReleaseResourceForThreadLite(
						&Vcb->MainResource,
						ExGetCurrentResourceThread()
					);
            }
        }
    }
    __finally
    {
        if (GlobalResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
					&gExt2Global->Resource,
					ExGetCurrentResourceThread() 
				);
        }

        if (!IrpContext->ExceptionInProgress)
        {
            if (Status == STATUS_PENDING)
            {
                Ext2QueueRequest(IrpContext);
            }
            else
            {
                IrpContext->Irp->IoStatus.Status = Status;
                
                Ext2CompleteRequest(
						IrpContext->Irp,
						(CCHAR) (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT)
					);
                
                Ext2FreeIrpContext(IrpContext);
            }
        }
    }

    return Status;
 }

 //////////////////////////////////////////////////////////////////////////////