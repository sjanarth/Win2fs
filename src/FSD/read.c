/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             read.c
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

NTSTATUS
Ext2ReadComplete (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2ReadFile (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2ReadVolume (IN PEXT2_IRP_CONTEXT IrpContext);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2CompleteIrpContext)
#pragma alloc_text(PAGE, Ext2CopyRead)
#pragma alloc_text(PAGE, Ext2Read)
#pragma alloc_text(PAGE, Ext2ReadVolume)
#pragma alloc_text(PAGE, Ext2ReadInode)
#pragma alloc_text(PAGE, Ext2ReadFile)
#pragma alloc_text(PAGE, Ext2ReadComplete)
#pragma alloc_text(PAGE, Ext2SyncUninitializeCacheMap)
#endif

/* FUNCTIONS *************************************************************/

NTSTATUS
Ext2CompleteIrpContext (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN NTSTATUS Status )
{
    PIRP Irp = IrpContext->Irp;
    
    if (NT_ERROR(Status))
    {
        Irp->IoStatus.Information = 0;
    }
    
    Irp->IoStatus.Status = Status;
    
    IoCompleteRequest( Irp, IO_DISK_INCREMENT );

    Ext2FreeIrpContext(IrpContext);

    return Status;
}


VOID
Ext2SyncUninitializeCacheMap (
    IN PFILE_OBJECT FileObject
    )
{
    CACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent;
    NTSTATUS WaitStatus;
    LARGE_INTEGER Ext2LargeZero = {0,0};


    KeInitializeEvent( &UninitializeCompleteEvent.Event,
                       SynchronizationEvent,
                       FALSE);

    CcUninitializeCacheMap( FileObject,
                            &Ext2LargeZero,
                            &UninitializeCompleteEvent );

    WaitStatus = KeWaitForSingleObject( &UninitializeCompleteEvent.Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

    ASSERT (NT_SUCCESS(WaitStatus));
}

BOOLEAN 
Ext2CopyRead(
    IN PFILE_OBJECT  FileObject,
    IN PLARGE_INTEGER  FileOffset,
    IN ULONG  Length,
    IN BOOLEAN  Wait,
    OUT PVOID  Buffer,
    OUT PIO_STATUS_BLOCK  IoStatus
    )
{

    return CcCopyRead(FileObject,
            FileOffset,
            Length,
            Wait,
            Buffer,
            IoStatus    );
/*
    PVOID Bcb = NULL;
    PVOID Buf = NULL;

    if (CcMapData(  FileObject,
                    FileOffset,
                    Length,
                    Wait,
                    &Bcb,
                    &Buf    ))
    {
        RtlCopyMemory(Buffer,  Buf, Length);
        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = Length;
        CcUnpinData(Bcb);
        return TRUE;

    }
    else
    {
        // IoStatus->Status = STATUS_
        return FALSE;
    }
*/
}


NTSTATUS
Ext2ReadVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PEXT2_VCB           Vcb;
    PEXT2_FCBVCB        FcbOrVcb;
    PFILE_OBJECT        FileObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;

    ULONG               Length;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    PUCHAR              Buffer;
    PEXT2_BDL           ext2_bdl = NULL;

    __try
    {
        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
    
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
            (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
        
        FileObject = IrpContext->FileObject;

        FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;
        
        ASSERT(FcbOrVcb);
        
        if (!(FcbOrVcb->Identifier.Type == EXT2VCB && (PVOID)FcbOrVcb == (PVOID)Vcb))
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

        Irp = IrpContext->Irp;
            
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
            
        Length = IoStackLocation->Parameters.Read.Length;
        ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;
            
        PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
        Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);
            
        if (Length == 0)
        {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            __leave;
        }
            
        if (Nocache &&
           (ByteOffset.LowPart & (SECTOR_SIZE - 1) ||
            Length & (SECTOR_SIZE - 1)))
        {
            Ext2DbgBreakPoint();

            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC))
        {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            __leave;
        }
        
        if (!PagingIo)
        {
            if (!ExAcquireResourceSharedLite(
                &Vcb->MainResource,
                IrpContext->IsSynchronous ))
            {
                Status = STATUS_PENDING;
                __leave;
            }
            
            MainResourceAcquired = TRUE;
        }
        else
        {
            if (!ExAcquireResourceSharedLite(
                &Vcb->PagingIoResource,
                IrpContext->IsSynchronous ))
            {
                Status = STATUS_PENDING;
                __leave;
            }
            
            PagingIoResourceAcquired = TRUE;
        }
        
    
        if (ByteOffset.QuadPart >=
            Vcb->PartitionInformation.PartitionLength.QuadPart  )
        {
            Irp->IoStatus.Information = 0;
            Status = STATUS_END_OF_FILE;
            __leave;
        }

        if (!Nocache)
        {
            if ((ByteOffset.QuadPart + Length) >
                    Vcb->PartitionInformation.PartitionLength.QuadPart
            )
            {
                Length = (ULONG) (
                    Vcb->PartitionInformation.PartitionLength.QuadPart -
                    ByteOffset.QuadPart);
                Length &= ~(SECTOR_SIZE - 1);
            }

            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL))
            {
                CcMdlRead(
                    Vcb->StreamObj,
                    &ByteOffset,
                    Length,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );
                
                Status = Irp->IoStatus.Status;
            }
            else
            {
            
                Buffer = Ext2GetUserBuffer(Irp);
                    
                if (Buffer == NULL)
                {
                    Status = STATUS_INVALID_USER_BUFFER;
                    __leave;
                }

                if (!CcCopyRead(
                    Vcb->StreamObj,
                    (PLARGE_INTEGER)&ByteOffset,
                    Length,
                    IrpContext->IsSynchronous,
                    Buffer,
                    &Irp->IoStatus ))
                {
                    Status = STATUS_PENDING;
                    __leave;
                }
                
                Status = Irp->IoStatus.Status;
            }
        }
        else
        {
            if ((ByteOffset.QuadPart + Length) >
                    Vcb->PartitionInformation.PartitionLength.QuadPart
            )
            {
                Length = (ULONG) (
                    Vcb->PartitionInformation.PartitionLength.QuadPart -
                    ByteOffset.QuadPart);

                Length &= ~(SECTOR_SIZE - 1);
            }

            Status = Ext2LockUserBuffer(
                IrpContext->Irp,
                Length,
                IoWriteAccess );
                
            if (!NT_SUCCESS(Status))
            {
                __leave;
            }

            ext2_bdl = ExAllocatePool(PagedPool, sizeof(EXT2_BDL));

            if (!ext2_bdl)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            ext2_bdl->Irp = NULL;
            ext2_bdl->Lba = ByteOffset.QuadPart;
            ext2_bdl->Length = Length;
            ext2_bdl->Offset = 0;

            Status = Ext2ReadWriteBlocks(IrpContext,
                                Vcb,
                                ext2_bdl,
                                Length,
                                1,
                                FALSE   );
            Irp = IrpContext->Irp;

            if (!Irp)
                __leave;
        }
    }

    __finally
    {
        if (PagingIoResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->PagingIoResource,
                ExGetCurrentResourceThread());
        }
        
        if (MainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread());
        }

        if (ext2_bdl)
            ExFreePool(ext2_bdl);

        if (!IrpContext->ExceptionInProgress)
        {
            if (Irp)
            {
                if (Status == STATUS_PENDING)
                {
                    Status = Ext2LockUserBuffer(
                        IrpContext->Irp,
                        Length,
                        IoWriteAccess );
                    
                    if (NT_SUCCESS(Status))
                    {
                        Status = Ext2QueueRequest(IrpContext);
                    }
                    else
                    {
                        IrpContext->Irp->IoStatus.Status = Status;
                        Ext2CompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
                        Ext2FreeIrpContext(IrpContext);
                    }
                }
                else
                {
                    IrpContext->Irp->IoStatus.Status = Status;
                    
                    if (SynchronousIo && !PagingIo && NT_SUCCESS(Status))
                    {
                        FileObject->CurrentByteOffset.QuadPart =
                            ByteOffset.QuadPart + Irp->IoStatus.Information;
                    }
                    
                    if (!PagingIo && NT_SUCCESS(Status))
                    {
                        FileObject->Flags &= ~FO_FILE_FAST_IO_READ;
                    }
                    
                    Ext2CompleteRequest(
                            IrpContext->Irp,
                            (CCHAR)
                            (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
                
                    Ext2FreeIrpContext(IrpContext);
                }
            }
            else
            {
                Ext2FreeIrpContext(IrpContext);
            }
        }
    }

    return Status;
}

NTSTATUS
Ext2ReadInode (
            IN PEXT2_IRP_CONTEXT    IrpContext,
            IN PEXT2_VCB            Vcb,
            IN PEXT2_INODE          ext2_inode,
            IN ULONG                offset,
            IN PVOID                Buffer,
            IN ULONG                size,
            OUT PULONG              dwReturn)
{
    PEXT2_BDL   ext2_bdl = NULL;
    ULONG       blocks, i;
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK IoStatus;

    blocks = Ext2BuildBDL(IrpContext, Vcb, ext2_inode, offset, size, &ext2_bdl);

    if (blocks <= 0)
    {
        Ext2DbgBreakPoint();
        goto errorout;
    }
    
    if (IrpContext)
    {
        // assume offset is aligned.
        Status = Ext2ReadWriteBlocks(IrpContext, Vcb, ext2_bdl, size, blocks, FALSE);
    }
    else
    {
        for(i = 0; i < blocks; i++)
        {
            IoStatus.Information = 0;

            Ext2CopyRead(
                    Vcb->StreamObj, 
                    (PLARGE_INTEGER)(&(ext2_bdl[i].Lba)), 
                    ext2_bdl[i].Length,
                    TRUE,
                    (PVOID)((PUCHAR)Buffer + ext2_bdl[i].Offset), 
                    &IoStatus   );

            Status = IoStatus.Status;
        }
    }

errorout:

    if (ext2_bdl)
        ExFreePool(ext2_bdl);

    return Status;
}

NTSTATUS
Ext2ReadFile(IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PEXT2_VCB           Vcb;
    PEXT2_FCB           Fcb;
    PEXT2_CCB           Ccb;
    PFILE_OBJECT        FileObject;
    PFILE_OBJECT        CacheObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;

    ULONG               Length;
    ULONG               ReturnedLength;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    PUCHAR              Buffer;

    __try
    {
        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
    
        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
            (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        
        ASSERT(Fcb);
    
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
            (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        Ccb = (PEXT2_CCB) FileObject->FsContext2;

        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        Length = IoStackLocation->Parameters.Read.Length;
        ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;
        
        PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
        Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);
        
        if (Length == 0)
        {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            __leave;
        }

        if (Nocache &&
           (ByteOffset.LowPart & (SECTOR_SIZE - 1) ||
            Length & (SECTOR_SIZE - 1)))
        {
            Status = STATUS_INVALID_PARAMETER;
            Ext2DbgBreakPoint();
            __leave;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC))
        {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            Ext2DbgBreakPoint();
            __leave;
        }
        
        if (!PagingIo)
        {
            if (!ExAcquireResourceSharedLite(
                &Fcb->MainResource,
                IrpContext->IsSynchronous ))
            {
                Status = STATUS_PENDING;
                __leave;
            }
            
            MainResourceAcquired = TRUE;

            if (!FsRtlCheckLockForReadAccess(
                &Fcb->FileLockAnchor,
                Irp         ))
            {
                Status = STATUS_FILE_LOCK_CONFLICT;
                __leave;
            }
        }
        else
        {
            if (!ExAcquireResourceSharedLite(
                &Fcb->PagingIoResource,
                IrpContext->IsSynchronous ))
            {
                Status = STATUS_PENDING;
                __leave;
            }
            
            PagingIoResourceAcquired = TRUE;
        }
        
        if (!Nocache)
        {
            if ((ByteOffset.QuadPart + (LONGLONG)Length) >
                Fcb->CommonFCBHeader.FileSize.QuadPart )
            {
                if (ByteOffset.QuadPart >= (Fcb->CommonFCBHeader.FileSize.QuadPart))
                {
                    Irp->IoStatus.Information = 0;
                    Status = STATUS_END_OF_FILE;
                    __leave;
                }

                Length =
                    (ULONG)(Fcb->CommonFCBHeader.FileSize.QuadPart - ByteOffset.QuadPart);

            }

            ReturnedLength = Length;

            if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
            {
                __leave;
            }

            {
                if (FileObject->PrivateCacheMap == NULL)
                {
                    CcInitializeCacheMap(
                        FileObject,
                        (PCC_FILE_SIZES)(&Fcb->CommonFCBHeader.AllocationSize),
                        FALSE,
                        &gExt2Global->CacheManagerCallbacks,
                        Fcb );
                }

                CacheObject = FileObject;
            }

            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL))
            {
                CcMdlRead(
                    CacheObject,
                    (&ByteOffset),
                    Length,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );
                
                Status = Irp->IoStatus.Status;
            }
            else
            {
                Buffer = Ext2GetUserBuffer(Irp);
                
                if (Buffer == NULL)
                {
                    Status = STATUS_INVALID_USER_BUFFER;
                    Ext2DbgBreakPoint();
                    __leave;
                }
                
                if (!CcCopyRead(
                    CacheObject,
                    (PLARGE_INTEGER)&ByteOffset,
                    Length,
                    IrpContext->IsSynchronous,
                    Buffer,
                    &Irp->IoStatus ))
                {
                    Status = STATUS_PENDING;
                    Ext2DbgBreakPoint();
                    __leave;
                }
                
                Status = Irp->IoStatus.Status;
            }
        }
        else
        {
            if ((ByteOffset.QuadPart + (LONGLONG)Length) > Fcb->CommonFCBHeader.AllocationSize.QuadPart)
            {

                if (ByteOffset.QuadPart >= Fcb->CommonFCBHeader.AllocationSize.QuadPart)
                {
                    Irp->IoStatus.Information = 0;
                    Status = STATUS_END_OF_FILE;
                    Ext2DbgBreakPoint();
                    __leave;
                }

                Length =
                     (ULONG)(Fcb->CommonFCBHeader.AllocationSize.QuadPart- ByteOffset.QuadPart);
            }

            ReturnedLength = Length;

            Status = Ext2LockUserBuffer(
                IrpContext->Irp,
                Length,
                IoWriteAccess );
                
            if (!NT_SUCCESS(Status))
            {
                __leave;
            }

            Buffer = Ext2GetUserBuffer(Irp);
            
            if (Buffer == NULL)
            {
                Status = STATUS_INVALID_USER_BUFFER;
                Ext2DbgBreakPoint();

                __leave;
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Length;
            
            Status = 
                Ext2ReadInode(
                IrpContext,
                Vcb,
                Fcb->ext2_inode,
                (ULONG)(ByteOffset.QuadPart),
                NULL,
                Length,
                &ReturnedLength);

            Irp = IrpContext->Irp;

        }
    }

    __finally
    {
        if (PagingIoResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->PagingIoResource,
                ExGetCurrentResourceThread());
        }
        
        if (MainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread());
        }
        
        if (!IrpContext->ExceptionInProgress)
        {
            if (Irp)
            {
                if (Status == STATUS_PENDING)
                {
                    Status = Ext2LockUserBuffer(
                        IrpContext->Irp,
                        Length,
                        IoWriteAccess );
                    
                    if (NT_SUCCESS(Status))
                    {
                        Status = Ext2QueueRequest(IrpContext);
                    }
                    else
                    {
                        IrpContext->Irp->IoStatus.Status = Status;
                        Ext2CompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
                        Ext2FreeIrpContext(IrpContext);
                    }
                }
                else
                {
                    IrpContext->Irp->IoStatus.Status = Status;
                    
                    if (SynchronousIo && !PagingIo && NT_SUCCESS(Status))
                    {
                        FileObject->CurrentByteOffset.QuadPart =
                            ByteOffset.QuadPart + Irp->IoStatus.Information;
                    }
                    
                    if (!PagingIo && NT_SUCCESS(Status))
                    {
                        FileObject->Flags &= ~FO_FILE_FAST_IO_READ;
                    }
                    
                    Ext2CompleteRequest(
                            IrpContext->Irp,
                            (CCHAR)
                            (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
                    
                    Ext2FreeIrpContext(IrpContext);
                }
            }
            else
            {
                Ext2FreeIrpContext(IrpContext);
            }
        }
    }
    
    return Status;

}

NTSTATUS
Ext2ReadComplete (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PIRP            Irp;
    
    __try
    {
        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        FileObject = IrpContext->FileObject;
        
        Irp = IrpContext->Irp;
        
        CcMdlReadComplete(FileObject, Irp->MdlAddress);
        
        Irp->MdlAddress = NULL;
        
        Status = STATUS_SUCCESS;
    }

    __finally
    {
        if (!IrpContext->ExceptionInProgress)
        {
            IrpContext->Irp->IoStatus.Status = Status;
            
            Ext2CompleteRequest(
                IrpContext->Irp,
                (CCHAR)
                (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT)
                );
            
            Ext2FreeIrpContext(IrpContext);
        }
    }
    
    return Status;
}


NTSTATUS
Ext2Read (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status;
    PEXT2_FCBVCB        FcbOrVcb;
    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
    __try
    {
        if (FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE))
        {
            Status =  Ext2ReadComplete(IrpContext);
        }
        else
        {
            DeviceObject = IrpContext->DeviceObject;

            if (DeviceObject == gExt2Global->DeviceObject)
            {
                Status = Ext2CompleteIrpContext(IrpContext, STATUS_INVALID_DEVICE_REQUEST);
                __leave;
            }

            FileObject = IrpContext->FileObject;
            
            FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;

            if (FcbOrVcb->Identifier.Type == EXT2VCB)
            {
                Status = Ext2ReadVolume(IrpContext);
            }
            else if (FcbOrVcb->Identifier.Type == EXT2FCB)
            {
                Status = Ext2ReadFile(IrpContext);
            }
            else
            {
                Ext2DbgPrint(D_READ, "Ext2Read: INVALID PARAMETER ... \n");
                Ext2DbgBreakPoint();

                Status = Ext2CompleteIrpContext(IrpContext, STATUS_INVALID_PARAMETER);
            }
        }
    }

    __finally
    {
    }
    
    return Status;
}
