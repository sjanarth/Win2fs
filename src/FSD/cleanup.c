/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             cleanup.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "ntifs.h"
#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL gExt2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2Cleanup)
#endif


NTSTATUS
Ext2Cleanup (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_SUCCESS;
    PEXT2_VCB       Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    PFILE_OBJECT    FileObject;
    PEXT2_FCB       Fcb;
    BOOLEAN         FcbResourceAcquired = FALSE;
    BOOLEAN         FcbPagingIoAcquired = FALSE;
    PEXT2_CCB       Ccb;
    PIRP            Irp;

    __try
    {
        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        if (DeviceObject == gExt2Global->DeviceObject)
        {
            Status = STATUS_SUCCESS;
            __leave;
        }
        
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
            (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        if (!IsFlagOn(Vcb->Flags, VCB_INITIALIZED))
        {
            Status = STATUS_SUCCESS;
            __leave;
        }

        if (!ExAcquireResourceExclusiveLite(
                 &Vcb->MainResource,
                 IrpContext->IsSynchronous
                 ))
        {
            Status = STATUS_PENDING;
            __leave;
        }

        VcbResourceAcquired = TRUE;
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        
        if (!Fcb)
        {
            Status = STATUS_SUCCESS;
            __leave;
        }
        
        if (Fcb->Identifier.Type == EXT2VCB)
        {
            if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED))
            {
                ClearFlag(Vcb->Flags, VCB_VOLUME_LOCKED);

                Ext2ClearVpbFlag(Vcb->Vpb, VPB_LOCKED);
            }

            ExAcquireResourceExclusiveLite(
                &Vcb->CountResource, TRUE );
        
            Vcb->OpenHandleCount--;
        
            ExReleaseResourceForThreadLite(
                &Vcb->CountResource,
                ExGetCurrentResourceThread() );


            if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY) && (!Vcb->OpenHandleCount))
                IoRemoveShareAccess(FileObject, &Vcb->ShareAccess);

            Status = STATUS_SUCCESS;
            __leave;
        }
        

/*
        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread());

            VcbResourceAcquired = FALSE;
        }
*/

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
            (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY) && !IsFlagOn(Fcb->Flags, FCB_PAGE_FILE))
        {
            if (!ExAcquireResourceExclusiveLite(
                     &Fcb->MainResource,
                     IrpContext->IsSynchronous
                     ))
            {
                Status = STATUS_PENDING;
                __leave;
            }

            FcbResourceAcquired = TRUE;
        }
        
        Ccb = (PEXT2_CCB) FileObject->FsContext2;

        if (!Ccb)
        {
            Status = STATUS_SUCCESS;
            __leave;
        }
        
        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));        
        Irp = IrpContext->Irp;

        ExAcquireResourceExclusiveLite(&Fcb->CountResource, TRUE);
        Fcb->OpenHandleCount--;
        if (!IsFlagOn(FileObject->Flags, FO_CACHE_SUPPORTED ))
        {
            Fcb->NonCachedOpenCount--;
        }
        ExReleaseResourceForThreadLite(
                &Fcb->CountResource,
                ExGetCurrentResourceThread());

        ExAcquireResourceExclusiveLite(&Vcb->CountResource, TRUE);
        Vcb->OpenFileHandleCount--;
        ExReleaseResourceForThreadLite(
                &Vcb->CountResource,
                ExGetCurrentResourceThread());

        if (IsFlagOn(Fcb->Flags, FCB_DELETE_ON_CLOSE))
        {
            SetFlag(Fcb->Flags, FCB_DELETE_PENDING);

            FsRtlNotifyFullChangeDirectory( Vcb->NotifySync,
                                                &Vcb->NotifyList,
                                                Fcb,
                                                NULL,
                                                FALSE,
                                                FALSE,
                                                0,
                                                NULL,
                                                NULL,
                                                NULL );


        }

        if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        {
            FsRtlNotifyCleanup(
                Vcb->NotifySync,
                &Vcb->NotifyList,
                Ccb   );
        }
        else
        {
            //
            // Drop any byte range locks this process may have on the file.
            //
            FsRtlFastUnlockAll(
                &Fcb->FileLockAnchor,
                FileObject,
                IoGetRequestorProcess(Irp),
                NULL  );

            //
            // If there are no byte range locks owned by other processes on the
            // file the fast I/O read/write functions doesn't have to check for
            // locks so we set IsFastIoPossible to FastIoIsPossible again.
            //
            if (!FsRtlGetNextFileLock(&Fcb->FileLockAnchor, TRUE))
            {
                if (Fcb->CommonFCBHeader.IsFastIoPossible != FastIoIsPossible)
                {
#if DBG
                    Ext2DbgPrint(D_CLEANUP, 
                        DRIVER_NAME ": %-16.16s %-31s %s\n",
                        Ext2DbgGetCurrentProcessName(),
                        "FastIoIsPossible",
                        Fcb->AnsiFileName.Buffer
                        );
#endif
                    Fcb->CommonFCBHeader.IsFastIoPossible = FastIoIsPossible;
                }
            }
        }

        if (!IsFlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        {
            if (IsFlagOn(FileObject->Flags, FO_CACHE_SUPPORTED) &&
                (Fcb->SectionObject.DataSectionObject != NULL))
            {
/*
                if (Fcb->NonCachedOpenCount != 0 && 
                    Fcb->NonCachedOpenCount == Fcb->OpenHandleCount)
*/
                {

                    if(!IsFlagOn(Vcb->Flags, VCB_READ_ONLY) &&
                          (IsFlagOn(Fcb->Flags, FCB_FILE_MODIFIED) || IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED) ))
                    {
                        CcFlushCache(&Fcb->SectionObject, NULL, 0, NULL);
                        ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                    }

                    CcPurgeCacheSection(&Fcb->SectionObject,
                                         NULL,
                                         0,
                                         FALSE );
                }
            }

            if (FileObject->PrivateCacheMap)
            {
                CcUninitializeCacheMap(FileObject,
                (PLARGE_INTEGER)(&(Fcb->CommonFCBHeader.FileSize)),
                NULL );
            }
        }

        if (IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING))
        {
			if (!Fcb->OpenHandleCount && Fcb->ReferenceCount <= 1)
			{
				//
				//	Have to delete this file...
				//

                if (!ExAcquireResourceExclusiveLite(
                         &Fcb->PagingIoResource,
                         IrpContext->IsSynchronous
                         ))
                {
                    Status = STATUS_PENDING;
                    __leave;
                }

                FcbPagingIoAcquired = TRUE;

				if (Ext2DeleteFile(IrpContext, Vcb, Fcb))
                {
/*
                    UNICODE_STRING FullFileName;

                    if (Ext2GetFullFileName(Fcb->Ext2Mcb, &FullFileName))
                    {
                        FsRtlNotifyFullReportChange((PNOTIFY_SYNC)(Vcb->NotifySync),
                                                    (PLIST_ENTRY)&(Vcb->NotifyList),
                                                    (PSTRING)(&FullFileName),
                                                    (USHORT)(FullFileName.Length - Fcb->Ext2Mcb->ShortName.Length),
                                                    (PSTRING)NULL,
                                                    (PSTRING)NULL,
                                                    IsFlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY) ?
                                                    ((ULONG)FILE_NOTIFY_CHANGE_DIR_NAME) :
                                                    ((ULONG)FILE_NOTIFY_CHANGE_FILE_NAME),
                                                    (ULONG)FILE_ACTION_REMOVED,
                                                    (PVOID)0 );


                        ExFreePool(FullFileName.Buffer);
                    }
*/
                }

                if (FcbPagingIoAcquired)
                {
                    ExReleaseResourceForThreadLite(
                        &Fcb->PagingIoResource,
                        ExGetCurrentResourceThread() );

                    FcbPagingIoAcquired = FALSE;
                }
			}
        }

       if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY) && !Fcb->OpenHandleCount)
            IoRemoveShareAccess(FileObject, &Fcb->ShareAccess);


#if DBG
        Ext2DbgPrint(D_CLEANUP, "Ext2Cleanup: OpenCount: %u ReferCount: %-7u %s\n",
            Fcb->OpenHandleCount,
            Fcb->ReferenceCount,
            Fcb->AnsiFileName.Buffer );
#endif
        Status = STATUS_SUCCESS;
    }

    __finally
    {
        if (IrpContext->FileObject)
        {
            SetFlag(IrpContext->FileObject->Flags, FO_CLEANUP_COMPLETE);
        }
        
        if (FcbPagingIoAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->PagingIoResource,
                ExGetCurrentResourceThread() );
        }

        if (FcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread() );
        }
        
        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread());
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
                    (CCHAR)
                    (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
                
                Ext2FreeIrpContext(IrpContext);
            }
        }
    }
    
    return Status;
}
