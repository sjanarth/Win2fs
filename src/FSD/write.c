/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             write.c
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
Ext2WriteComplete (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2WriteFile (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2WriteVolume (IN PEXT2_IRP_CONTEXT IrpContext);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2ZeroHoles)

#pragma alloc_text(PAGE, Ext2Write)

#pragma alloc_text(PAGE, Ext2WriteVolume)
#pragma alloc_text(PAGE, Ext2WriteInode)
#pragma alloc_text(PAGE, Ext2WriteFile)
#pragma alloc_text(PAGE, Ext2WriteComplete)

#pragma alloc_text(PAGE, Ext2SupersedeOrOverWriteFile)
#pragma alloc_text(PAGE, Ext2IsDirectoryEmpty)
#pragma alloc_text(PAGE, Ext2DeleteFile)
#endif

/* FUNCTIONS *************************************************************/

BOOLEAN
Ext2ZeroHoles (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN LONGLONG Offset,
    IN LONGLONG Count
    )
{
    LARGE_INTEGER StartAddr = {0,0};
    LARGE_INTEGER EndAddr = {0,0};

    StartAddr.QuadPart = (Offset + (SECTOR_SIZE - 1)) & ~(SECTOR_SIZE - 1);

    EndAddr.QuadPart = (Offset + Count + (SECTOR_SIZE - 1)) & ~(SECTOR_SIZE - 1);

    if (StartAddr.QuadPart < EndAddr.QuadPart)
    {
        return CcZeroData( FileObject,
                       &StartAddr,
                       &EndAddr,
                       IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );
    }

    return TRUE;
}


NTSTATUS
Ext2WriteVolume (IN PEXT2_IRP_CONTEXT IrpContext)
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
            
        Length = IoStackLocation->Parameters.Write.Length;
        ByteOffset = IoStackLocation->Parameters.Write.ByteOffset;
            
        PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
        Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);

        Ext2DbgPrint(D_WRITE, "Ext2WriteVolume: Len=%xh Off=%I64x Paging=%xh Nocache=%xh\n", Length, ByteOffset.QuadPart, PagingIo, Nocache);

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
            __leave;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC))
        {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            __leave;
        }
        
        if (ByteOffset.QuadPart >=
            Vcb->PartitionInformation.PartitionLength.QuadPart  )
        {
            Irp->IoStatus.Information = 0;
            Status = STATUS_END_OF_FILE;
            __leave;
        }

		if (Nocache && !PagingIo && (Vcb->SectionObject.DataSectionObject != NULL)) 
		{

            ExAcquireResourceExclusive(&Vcb->MainResource, TRUE);
            MainResourceAcquired = TRUE;

            ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
            ExReleaseResource(&Vcb->PagingIoResource);

			CcFlushCache( &(Vcb->SectionObject),
                          &ByteOffset,
                          Length,
                          &(Irp->IoStatus));

			if (!NT_SUCCESS(Irp->IoStatus.Status)) 
			{
                Status = Irp->IoStatus.Status;
				__leave;
			}

            ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
            ExReleaseResource(&Vcb->PagingIoResource);

			CcPurgeCacheSection( &(Vcb->SectionObject),
                                 (PLARGE_INTEGER)&(ByteOffset),
								 Length,
                                 FALSE );

            ExReleaseResource(&Vcb->MainResource);
            MainResourceAcquired = FALSE;
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

                CcPrepareMdlWrite (
                    Vcb->StreamObj,
                    &ByteOffset,
                    Length,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );
                
                Status = Irp->IoStatus.Status;
            }
            else
            {
                PBCB    Bcb;
                PVOID   Buf;

                Buffer = Ext2GetUserBuffer(Irp);
                    
                if (Buffer == NULL)
                {
                    Status = STATUS_INVALID_USER_BUFFER;
                    __leave;
                }

				CcPreparePinWrite( 
					Vcb->StreamObj,
					&ByteOffset,
					Length,
					FALSE,
					TRUE,
					&Bcb,
					&Buf );

                RtlCopyMemory(Buf, Buffer, Length);

                CcUnpinData(Bcb);

                Status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(Status))
                Irp->IoStatus.Information = Length;
        }
        else
        {
            PEXT2_BDL           ext2_bdl = NULL;
            ULONG               Blocks = 0;

            LONGLONG            DirtyStart;
            LONGLONG            DirtyLba;
            LONGLONG            DirtyLength;
            LONGLONG            RemainLength;

            if ((ByteOffset.QuadPart + Length) >
                    Vcb->PartitionInformation.PartitionLength.QuadPart )
            {
                Length = (ULONG) (
                    Vcb->PartitionInformation.PartitionLength.QuadPart -
                    ByteOffset.QuadPart);

                Length &= ~(SECTOR_SIZE - 1);
            }

            Status = Ext2LockUserBuffer(
                IrpContext->Irp,
                Length,
                IoReadAccess );
                
            if (!NT_SUCCESS(Status))
            {
                __leave;
            }

            ext2_bdl = ExAllocatePool(PagedPool, (Length / Vcb->ext2_block) * sizeof(EXT2_BDL));

            if (!ext2_bdl)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            DirtyLba = ByteOffset.QuadPart;
            RemainLength = (LONGLONG) Length;

            while (RemainLength > 0)
            {
                DirtyStart = DirtyLba;

                if (Ext2LookupMcbEntry(Vcb, DirtyStart, &DirtyLba, &DirtyLength,
                                        (PLONGLONG)NULL, (PLONGLONG)NULL, (PULONG)NULL))
                {

                    if (DirtyLba == -1)
                    {
                        DirtyLba = DirtyStart + DirtyLength;

                        RemainLength = ByteOffset.QuadPart + (LONGLONG)Length - DirtyLba;
                        continue;
                    }
            
                    ext2_bdl[Blocks].Irp = NULL;
                    ext2_bdl[Blocks].Lba = DirtyLba;
                    ext2_bdl[Blocks].Offset = (ULONG)((LONGLONG)Length - RemainLength - (DirtyLba - DirtyStart));

                    if (DirtyLba + DirtyLength > DirtyStart + RemainLength)
                    {
                        ext2_bdl[Blocks].Length = (ULONG)(DirtyStart + RemainLength - DirtyLba);
                        RemainLength = 0;
                    }
                    else
                    {
                        ext2_bdl[Blocks].Length = (ULONG)DirtyLength;
                        RemainLength = (DirtyStart + RemainLength) - (DirtyLba + DirtyLength);
                    }

                    DirtyLba = DirtyStart + DirtyLength;
                    Blocks++;
                }
                else
                {
                    if (Blocks == 0)
                    {
                        if (ext2_bdl)
                            ExFreePool(ext2_bdl);

                        // Lookup fails at the first time, ie. no dirty blocks in the run
                        if (RemainLength == (LONGLONG)Length)
                            Status = STATUS_SUCCESS;
                        else
                            Status = STATUS_UNSUCCESSFUL;

                        __leave;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            if (Blocks > 0)
            {
                Status = Ext2ReadWriteBlocks(IrpContext,
                                    Vcb,
                                    ext2_bdl,
                                    Length,
                                    Blocks,
                                    FALSE   );
                Irp = IrpContext->Irp;

                if (NT_SUCCESS(Status))
                {
                    ULONG   i;

                    for (i=0; i<Blocks;i++)
                        Ext2RemoveMcbEntry(Vcb, ext2_bdl[i].Lba, ext2_bdl[i].Length);
                }

                if (ext2_bdl)
                    ExFreePool(ext2_bdl);

                if (!Irp)
                    __leave;

            }
            else
            {
                if (ext2_bdl)
                    ExFreePool(ext2_bdl);

                Status = STATUS_SUCCESS;
                __leave;
            }
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

        if (!IrpContext->ExceptionInProgress)
        {
            if (Irp)
            {
                if (Status == STATUS_PENDING)
                {
                    Status = Ext2LockUserBuffer(
                        IrpContext->Irp,
                        Length,
                        IoReadAccess );
                    
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
                        SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
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
Ext2WriteInode (
            IN PEXT2_IRP_CONTEXT    IrpContext,
            IN PEXT2_VCB            Vcb,
            IN PEXT2_INODE          ext2_inode,
            IN ULONG                offset,
            IN PVOID                Buffer,
            IN ULONG                size,
            IN BOOLEAN              bWriteToDisk,
            OUT PULONG              dwReturn)
{
    PEXT2_BDL   ext2_bdl = NULL;
    ULONG       blocks, i;
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;

    blocks = Ext2BuildBDL(IrpContext, Vcb, ext2_inode, offset, size, &ext2_bdl);

    if (blocks <= 0)
        return  Status;

#if DBG
    {
        ULONG   dwTotal = 0;
        Ext2DbgPrint(D_WRITE, "Ext2WriteInode: BDLCount = %xh Size=%xh Off=%xh\n", blocks, size, offset);
        for(i=0;i<blocks;i++)
        {
            Ext2DbgPrint(D_WRITE, "Ext2WriteInode: Lba=%I64xh Len=%xh Off=%xh\n", ext2_bdl[i].Lba, ext2_bdl[i].Length, ext2_bdl[i].Offset);
            dwTotal += ext2_bdl[i].Length;
        }

        if (dwTotal != size)
        {
            Ext2DbgBreakPoint();
        }
        Ext2DbgPrint(D_WRITE, "Ext2WriteInode: Total = %xh (WriteToDisk=%x)\n", dwTotal, bWriteToDisk);
    }
#endif
    
    if (bWriteToDisk)
    {

#if 0
        for(i=0; i<blocks; i++)
        {
            {
                CcFlushCache(   &(Vcb->SectionObject),
                                (PLARGE_INTEGER)&(ext2_bdl[i].Lba),
                                ext2_bdl[i].Length,
                                NULL);

                if (Vcb->SectionObject.DataSectionObject != NULL)
                {
                    ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
                    ExReleaseResource(&Vcb->PagingIoResource);
                           
   			        CcPurgeCacheSection( &(Vcb->SectionObject),
                                         (PLARGE_INTEGER)&(ext2_bdl[i].Lba),
								         ext2_bdl[i].Length,
                                         FALSE );
                }
            }
        }
#endif

        // assume offset is aligned.
        Status = Ext2ReadWriteBlocks(IrpContext, Vcb, ext2_bdl, size, blocks, FALSE);
    }
    else
    {
        for(i = 0; i < blocks; i++)
        {
            if(!Ext2SaveBuffer(IrpContext, Vcb, ext2_bdl[i].Lba, ext2_bdl[i].Length, (PVOID)((PUCHAR)Buffer + ext2_bdl[i].Offset)))
                goto errorout;
        }

        Status = STATUS_SUCCESS;
    }

errorout:

    if (ext2_bdl)
        ExFreePool(ext2_bdl);

    return Status;
}

NTSTATUS
Ext2WriteFile(IN PEXT2_IRP_CONTEXT IrpContext)
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

    BOOLEAN             bNeedExtending = FALSE;
    BOOLEAN             bAppendFile = FALSE;

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
        
        Length = IoStackLocation->Parameters.Write.Length;
        ByteOffset = IoStackLocation->Parameters.Write.ByteOffset;
        
        PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
        Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);

        Ext2DbgPrint(D_WRITE, "Ext2WriteFile: Len=%xh Off=%I64x Paging=%xh Nocache=%xh\n", Length, ByteOffset.QuadPart, PagingIo, Nocache);
        
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
            __leave;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC))
        {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            __leave;
        }

        if (IsEndOfFile(ByteOffset))
        {
            bAppendFile = TRUE;
            ByteOffset.QuadPart = Fcb->CommonFCBHeader.FileSize.QuadPart;
        }

        if ( FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY) && !PagingIo)
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

		//
		//	Do flushing for such cases
		//
		if (Nocache && !PagingIo && (Fcb->SectionObject.DataSectionObject != NULL)) 
		{
            ExAcquireResourceExclusive(&Fcb->MainResource, IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT));
            MainResourceAcquired = TRUE;

            ExAcquireSharedStarveExclusive( &Fcb->PagingIoResource, TRUE);
            ExReleaseResource(&Fcb->PagingIoResource);

			CcFlushCache( &(Fcb->SectionObject),
                          &ByteOffset,
                          Length,
                          &(Irp->IoStatus));
            ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);

			if (!NT_SUCCESS(Irp->IoStatus.Status)) 
			{
                Status = Irp->IoStatus.Status;
				__leave;
			}

            ExAcquireSharedStarveExclusive( &Fcb->PagingIoResource, TRUE);
            ExReleaseResource(&Fcb->PagingIoResource);

			CcPurgeCacheSection( &(Fcb->SectionObject),
                                 (PLARGE_INTEGER)&(ByteOffset),
								 Length,
                                 FALSE );

            ExReleaseResource(&Fcb->MainResource);
            MainResourceAcquired = FALSE;
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
        
        if (!PagingIo)
        {
            if (!FsRtlCheckLockForWriteAccess(
                &Fcb->FileLockAnchor,
                Irp         ))
            {
                Status = STATUS_FILE_LOCK_CONFLICT;
                __leave;
            }
        }

        if (Nocache)
        {
            if (ByteOffset.QuadPart + Length > Fcb->CommonFCBHeader.AllocationSize.QuadPart)
            {
                if (ByteOffset.QuadPart >= Fcb->CommonFCBHeader.AllocationSize.QuadPart)
                {
                    Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = 0;
                    __leave;
                }
                else
                {
                    if (Length > (ULONG)(Fcb->CommonFCBHeader.AllocationSize.QuadPart - ByteOffset.QuadPart))
                        Length = (ULONG)(Fcb->CommonFCBHeader.AllocationSize.QuadPart - ByteOffset.QuadPart);
                }
            }
        }

        if (!Nocache)
        {
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

                    CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY);
                    CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->CommonFCBHeader.AllocationSize)));
                }

                CacheObject = FileObject;
            }

            //
            //  Need extending the size of inode ?
            //
            if (bAppendFile || ((ULONG)(ByteOffset.QuadPart + Length) >
                (ULONG)(Fcb->CommonFCBHeader.FileSize.QuadPart)))
            {

                LARGE_INTEGER   ExtendSize;
                LARGE_INTEGER   FileSize;

                bNeedExtending = TRUE;
                FileSize = Fcb->CommonFCBHeader.FileSize;
                ExtendSize.QuadPart = (LONGLONG)(ByteOffset.QuadPart + Length);

                if (ExtendSize.QuadPart > Fcb->CommonFCBHeader.AllocationSize.QuadPart)
                {
                    if (!Ext2ExpandFileAllocation(IrpContext, Vcb, Fcb, &ExtendSize))
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        __leave;
                    }
                }

                {
                    Fcb->CommonFCBHeader.FileSize.QuadPart = ExtendSize.QuadPart;
                    Fcb->ext2_inode->i_size = (ULONG) ExtendSize.QuadPart;
                }

                if (FileObject->PrivateCacheMap)
                {
                    CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->CommonFCBHeader.AllocationSize)));

                    if (ByteOffset.QuadPart > FileSize.QuadPart)
                    {
                        Ext2ZeroHoles(IrpContext, Vcb, FileObject, FileSize.QuadPart, ByteOffset.QuadPart - FileSize.QuadPart);
                    }

                    if (Fcb->CommonFCBHeader.AllocationSize.QuadPart > ExtendSize.QuadPart)
                    {
                        Ext2ZeroHoles(IrpContext, Vcb, FileObject, ExtendSize.QuadPart, Fcb->CommonFCBHeader.AllocationSize.QuadPart - ExtendSize.QuadPart);
                    }
                }

                if (Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, Fcb->ext2_inode))
                    Status = STATUS_SUCCESS;
            }


            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL))
            {
                CcPrepareMdlWrite(
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
                    __leave;
                }
                
                if (!CcCopyWrite(
                    CacheObject,
                    (PLARGE_INTEGER)&ByteOffset,
                    Length,
                    IrpContext->IsSynchronous,
                    Buffer  ))
                {
                    Status = STATUS_PENDING;
                    __leave;
                }
                
                Status = Irp->IoStatus.Status;
            }

            if (NT_SUCCESS(Status))
                Irp->IoStatus.Information = Length;

        }
        else
        {
            ReturnedLength = Length;

            Status = Ext2LockUserBuffer(
                IrpContext->Irp,
                Length,
                IoReadAccess );
                
            if (!NT_SUCCESS(Status))
            {
                __leave;
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Length;
            
            Status = 
                Ext2WriteInode(
                IrpContext,
                Vcb,
                Fcb->ext2_inode,
                (ULONG)(ByteOffset.QuadPart),
                NULL,
                Length,
                TRUE,
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
                        IoReadAccess );
                    
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
                        SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                        SetFlag(Fcb->Flags, FCB_FILE_MODIFIED);
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
Ext2WriteComplete (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PIRP            Irp;
    PIO_STACK_LOCATION IrpSp;
    
    __try
    {
        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        FileObject = IrpContext->FileObject;
        
        Irp = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        
        CcMdlWriteComplete(FileObject, &(IrpSp->Parameters.Write.ByteOffset), Irp->MdlAddress);
        
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
Ext2Write (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status;
    PEXT2_FCBVCB        FcbOrVcb;
    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;
    PEXT2_VCB           Vcb;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    __try
    {
        if (FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE))
        {
            Status =  Ext2WriteComplete(IrpContext);
        }
        else
        {
            DeviceObject = IrpContext->DeviceObject;

            if (DeviceObject == gExt2Global->DeviceObject)
            {
                Status = Ext2CompleteIrpContext(IrpContext, STATUS_INVALID_DEVICE_REQUEST);
                __leave;
            }

            Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;

            if (Vcb->Identifier.Type != EXT2VCB ||
                Vcb->Identifier.Size != sizeof(EXT2_VCB) )
            {
                 Status = Ext2CompleteIrpContext(IrpContext, STATUS_INVALID_PARAMETER);
                __leave;
            }

            if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
            {
                Status = STATUS_MEDIA_WRITE_PROTECTED;
                __leave;
            }

            FileObject = IrpContext->FileObject;
            
            FcbOrVcb = (PEXT2_FCBVCB) FileObject->FsContext;

            if (FcbOrVcb->Identifier.Type == EXT2VCB)
            {
                Status = Ext2WriteVolume(IrpContext);
            }
            else if (FcbOrVcb->Identifier.Type == EXT2FCB)
            {
                Status = Ext2WriteFile(IrpContext);
            }
            else
            {
                Status = Ext2CompleteIrpContext(IrpContext, STATUS_INVALID_PARAMETER);
            }
        }
    }

    __finally
    {

    }
    
    return Status;
}


BOOLEAN
Ext2SupersedeOrOverWriteFile(
        PEXT2_IRP_CONTEXT IrpContext,
        PEXT2_VCB Vcb,
        PEXT2_FCB Fcb,
        ULONG     Disposition)
{
	LARGE_INTEGER   CurrentTime;
    LARGE_INTEGER   AllocationSize;
    
    BOOLEAN         bRet = FALSE;

	KeQuerySystemTime(&CurrentTime);

    AllocationSize.QuadPart = (LONGLONG)0;

    bRet = Ext2TruncateFileAllocation(IrpContext, Vcb, Fcb, &AllocationSize);

    if (bRet)
    {
        Fcb->CommonFCBHeader.AllocationSize.QuadPart = 
        Fcb->CommonFCBHeader.FileSize.QuadPart =  (LONGLONG) 0;

        Fcb->ext2_inode->i_size = 0;

        if (Disposition == FILE_SUPERSEDE)
            Fcb->ext2_inode->i_ctime = Ext2InodeTime(CurrentTime);

        Fcb->ext2_inode->i_atime =
        Fcb->ext2_inode->i_mtime = Ext2InodeTime(CurrentTime);
    }
    else
    {
        if (Fcb->ext2_inode->i_size > (Fcb->ext2_inode->i_blocks * SECTOR_SIZE))
            Fcb->ext2_inode->i_size = (Fcb->ext2_inode->i_blocks * SECTOR_SIZE);
    
        Fcb->CommonFCBHeader.AllocationSize.QuadPart = (LONGLONG)(Fcb->ext2_inode->i_blocks * SECTOR_SIZE);
        Fcb->CommonFCBHeader.FileSize.QuadPart =  (LONGLONG) Fcb->ext2_inode->i_size;

    }

    Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, Fcb->ext2_inode);

    return bRet;
}

BOOLEAN Ext2IsDirectoryEmpty (
        PEXT2_VCB Vcb,
        PEXT2_FCB Dcb )
{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;

    PEXT2_DIR_ENTRY2        pTarget = NULL;

    ULONG                   dwBytes = 0;
    ULONG                   dwRet;

    BOOLEAN                 bRet = TRUE;

    if (!IsFlagOn(Dcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        return TRUE;

    __try
    {
        pTarget = (PEXT2_DIR_ENTRY2) ExAllocatePool(PagedPool,
                                     EXT2_DIR_REC_LEN(EXT2_NAME_LEN));
        if (!pTarget)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }
        
        dwBytes = 0;


        while ((LONGLONG)dwBytes < Dcb->CommonFCBHeader.AllocationSize.QuadPart)
        {
            RtlZeroMemory(pTarget, EXT2_DIR_REC_LEN(EXT2_NAME_LEN));

            Status = Ext2ReadInode(
                        NULL,
                        Vcb,
                        Dcb->ext2_inode,
                        dwBytes,
                        (PVOID)pTarget,
                        EXT2_DIR_REC_LEN(EXT2_NAME_LEN),
                        &dwRet);

            if (!NT_SUCCESS(Status))
            {
                Ext2DbgPrint(D_WRITE, "Ext2RemoveEntry: Reading Directory Content error.\n");
                __leave;
            }

            if (pTarget->inode)
            {
                if (pTarget->name_len == 1 && pTarget->name[0] == '.')
                {
                }
                else if (pTarget->name_len == 2 && pTarget->name[0] == '.' && 
                         pTarget->name[1] == '.')
                {
                }
                else
                {
                    bRet = FALSE;
                    break;
                }
            }
            else
            {
                break;
            }

            dwBytes += pTarget->rec_len;
        }
    }

    __finally
    {
        if (pTarget != NULL)
        {
            ExFreePool(pTarget);
        }
    }
    
    return bRet;
}


BOOLEAN
Ext2DeleteFile(
        PEXT2_IRP_CONTEXT IrpContext,
        PEXT2_VCB Vcb,
        PEXT2_FCB Fcb )
{
    BOOLEAN         bRet = FALSE;
    LARGE_INTEGER   AllocationSize;
    PEXT2_FCB       Dcb = NULL;

    NTSTATUS        Status;

    Ext2DbgPrint(D_WRITE, "Ext2DeleteFile: File %S (%xh) will be deleted!\n", Fcb->Ext2Mcb->ShortName.Buffer, Fcb->Ext2Mcb->Inode);

    if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED))
        return TRUE;

    if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
    {
        if (!Ext2IsDirectoryEmpty(Vcb, Fcb))
        {
            ClearFlag(Fcb->Flags, FCB_DELETE_PENDING);
            
            return FALSE;
        }
    }

    Ext2DbgPrint(D_WRITE, "Ext2DeleteFile: EXT2SB->S_FREE_BLOCKS = %xh .\n", Vcb->ext2_super_block->s_free_blocks_count);

    Status = STATUS_UNSUCCESSFUL;

    {
        if (Fcb->Ext2Mcb->Parent->Ext2Fcb)
        {
            Status = Ext2RemoveEntry(IrpContext, Vcb, Fcb->Ext2Mcb->Parent->Ext2Fcb, Fcb->Ext2Mcb->Inode);
        }
        else
        {
            Dcb = Ext2CreateFcbFromMcb(Vcb, Fcb->Ext2Mcb->Parent);
            if (Dcb)
            {
                Status = Ext2RemoveEntry(IrpContext, Vcb, Dcb, Fcb->Ext2Mcb->Inode);
            }
        }
    }

    if (NT_SUCCESS(Status))
    {
        Fcb->ext2_inode->i_links_count--;

        if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        {
            if (Fcb->ext2_inode->i_links_count <= 1)
            {
                bRet = TRUE;
            }
        }
        else
        {
            if (Fcb->ext2_inode->i_links_count == 0)
            {
                bRet = TRUE;
            }
        }
    }

    if (bRet)
    {
        AllocationSize.QuadPart = (LONGLONG)0;
        bRet = Ext2TruncateFileAllocation(IrpContext, Vcb, Fcb, &AllocationSize);
        
        if (bRet)
        {
            if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
                bRet = Ext2FreeInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, EXT2_FT_DIR);
            else
                bRet = Ext2FreeInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, EXT2_FT_REG_FILE);

            SetFlag(Fcb->Flags, FCB_FILE_DELETED);
            Ext2DeleteMcbNode(Fcb->Ext2Mcb->Parent, Fcb->Ext2Mcb);

            {
                LARGE_INTEGER   SysTime;
                KeQuerySystemTime(&SysTime);

                Fcb->ext2_inode->i_dtime = Ext2InodeTime(SysTime);

                Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, Fcb->ext2_inode);
            }
        }
    }

    Ext2DbgPrint(D_WRITE, "Ext2DeleteFile: Succeed... EXT2SB->S_FREE_BLOCKS = %xh .\n", Vcb->ext2_super_block->s_free_blocks_count);

    return bRet;
}
