/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             memory.c
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
#pragma alloc_text(PAGE, Ext2AllocateIrpContext)
#pragma alloc_text(PAGE, Ext2FreeIrpContext)
#pragma alloc_text(PAGE, Ext2AllocateFcb)
#pragma alloc_text(PAGE, Ext2FreeFcb)
#pragma alloc_text(PAGE, Ext2AllocateMcb)
#pragma alloc_text(PAGE, Ext2SearchMcbTree)
#pragma alloc_text(PAGE, Ext2SearchMcb)
#pragma alloc_text(PAGE, Ext2GetFullFileName)
#pragma alloc_text(PAGE, Ext2AddMcbNode)
#pragma alloc_text(PAGE, Ext2DeleteMcbNode)
#pragma alloc_text(PAGE, Ext2GetMcbDepth)
#pragma alloc_text(PAGE, Ext2CompareMcb)
#pragma alloc_text(PAGE, Ext2FindUnusedMcb)
#pragma alloc_text(PAGE, Ext2FreeMcbTree)
#pragma alloc_text(PAGE, Ext2CheckBitmapConsistency)
#pragma alloc_text(PAGE, Ext2CheckSetBlock)
#pragma alloc_text(PAGE, Ext2InitializeVcb)
#pragma alloc_text(PAGE, Ext2FreeCcb)
#pragma alloc_text(PAGE, Ext2AllocateCcb)
#pragma alloc_text(PAGE, Ext2FreeVcb)
#pragma alloc_text(PAGE, Ext2CreateFcbFromMcb)
#endif


PEXT2_IRP_CONTEXT
Ext2AllocateIrpContext (IN PDEVICE_OBJECT   DeviceObject,
                        IN PIRP             Irp )
{
    PIO_STACK_LOCATION  IoStackLocation;
    PEXT2_IRP_CONTEXT    IrpContext;
    
    ASSERT(DeviceObject != NULL);
    ASSERT(Irp != NULL);
    
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    
    IrpContext = (PEXT2_IRP_CONTEXT) (ExAllocateFromNPagedLookasideList( &(gExt2Global->Ext2IrpContextLookasideList)));

    if (IrpContext == NULL) {

        IrpContext = ExAllocatePool( NonPagedPool, sizeof(EXT2_IRP_CONTEXT) );

        //
        //  Zero out the irp context and indicate that it is from pool and
        //  not region allocated
        //

        RtlZeroMemory(IrpContext, sizeof(EXT2_IRP_CONTEXT));

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_FROM_POOL);

    } else {

        //
        //  Zero out the irp context and indicate that it is from zone and
        //  not pool allocated
        //

        RtlZeroMemory(IrpContext, sizeof(EXT2_IRP_CONTEXT) );
    }
    
    if (!IrpContext)
    {
        return NULL;
    }
    
    IrpContext->Identifier.Type = EXT2ICX;
    IrpContext->Identifier.Size = sizeof(EXT2_IRP_CONTEXT);
    
    IrpContext->Irp = Irp;
    
    IrpContext->MajorFunction = IoStackLocation->MajorFunction;
    IrpContext->MinorFunction = IoStackLocation->MinorFunction;
    
    IrpContext->DeviceObject = DeviceObject;
    
    IrpContext->FileObject = IoStackLocation->FileObject;
    
    if (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL ||
        IrpContext->MajorFunction == IRP_MJ_DEVICE_CONTROL ||
        IrpContext->MajorFunction == IRP_MJ_SHUTDOWN)
    {
        IrpContext->IsSynchronous = TRUE;
    }
    else if (IrpContext->MajorFunction == IRP_MJ_CLEANUP ||
        IrpContext->MajorFunction == IRP_MJ_CLOSE)
    {
        IrpContext->IsSynchronous = FALSE;
    }
    else
    {
        IrpContext->IsSynchronous = IoIsOperationSynchronous(Irp);
    }
    
    //
    // Temporary workaround for a bug in close that makes it reference a
    // fileobject when it is no longer valid.
    //
    if (IrpContext->MajorFunction == IRP_MJ_CLOSE)
    {
        IrpContext->IsSynchronous = TRUE;
    }
    
    IrpContext->IsTopLevel = (IoGetTopLevelIrp() == Irp);
    
    IrpContext->ExceptionInProgress = FALSE;
    
    return IrpContext;
}

VOID
Ext2FreeIrpContext (IN PEXT2_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext != NULL);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));


    Ext2UnpinRepinnedBcbs(IrpContext);
    
    //  Return the Irp context record to the region or to pool depending on
    //  its flag

    if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FROM_POOL)) {

        ExFreePool( IrpContext );

    } else {

        ExFreeToNPagedLookasideList(&(gExt2Global->Ext2IrpContextLookasideList), IrpContext);
    }
}

VOID
Ext2RepinBcb (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PBCB Bcb
    )
{
    PEXT2_REPINNED_BCBS Repinned;
    ULONG i;

    Repinned = &IrpContext->Repinned;

    while (TRUE)
    {
        for (i = 0; i < EXT2_REPINNED_BCBS_ARRAY_SIZE; i += 1)
        {
            if (Repinned->Bcb[i] == Bcb)
            {
                return;
            }

            if (Repinned->Bcb[i] == NULL)
            {
                Repinned->Bcb[i] = Bcb;
                CcRepinBcb( Bcb );

                return;
            }
        }

        if (Repinned->Next == NULL)
        {
            Repinned->Next = FsRtlAllocatePool( PagedPool, sizeof(EXT2_REPINNED_BCBS) );
            RtlZeroMemory( Repinned->Next, sizeof(EXT2_REPINNED_BCBS) );
        }

        Repinned = Repinned->Next;
    }
}

VOID
Ext2UnpinRepinnedBcbs (
    IN PEXT2_IRP_CONTEXT IrpContext
    )
{
    IO_STATUS_BLOCK RaiseIosb;
    PEXT2_REPINNED_BCBS Repinned;
    BOOLEAN WriteThroughToDisk;
    PFILE_OBJECT FileObject = NULL;
    BOOLEAN ForceVerify = FALSE;
    ULONG i;

    Repinned = &IrpContext->Repinned;
    RaiseIosb.Status = STATUS_SUCCESS;

    WriteThroughToDisk = (BOOLEAN) (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH) ||
                                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FLOPPY));

    while (Repinned != NULL)
    {
        for (i = 0; i < EXT2_REPINNED_BCBS_ARRAY_SIZE; i += 1)
        {
            if (Repinned->Bcb[i] != NULL)
            {
                IO_STATUS_BLOCK Iosb;

                if ( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FLOPPY) )
                {
                    FileObject = CcGetFileObjectFromBcb( Repinned->Bcb[i] );
                }

                CcUnpinRepinnedBcb( Repinned->Bcb[i],
                                    WriteThroughToDisk,
                                    &Iosb );

                if ( !NT_SUCCESS(Iosb.Status) )
                {
                    if (RaiseIosb.Status == STATUS_SUCCESS)
                    {
                        RaiseIosb = Iosb;
                    }

                    if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FLOPPY) &&
                        (IrpContext->MajorFunction != IRP_MJ_CLEANUP) &&
                        (IrpContext->MajorFunction != IRP_MJ_FLUSH_BUFFERS) &&
                        (IrpContext->MajorFunction != IRP_MJ_SET_INFORMATION))
                    {

                        CcPurgeCacheSection( FileObject->SectionObjectPointer,
                                             NULL,
                                             0,
                                             FALSE );

                        ForceVerify = TRUE;
                    }
                }

                Repinned->Bcb[i] = NULL;

            }
            else
            {
                break;
            }
        }

        if (Repinned != &IrpContext->Repinned)
        {

            PEXT2_REPINNED_BCBS Saved;

            Saved = Repinned->Next;
            ExFreePool( Repinned );
            Repinned = Saved;

        }
        else
        {

            Repinned = Repinned->Next;
            IrpContext->Repinned.Next = NULL;
        }
    }

    if (!NT_SUCCESS(RaiseIosb.Status))
    {
        if (ForceVerify && FileObject)
        {
            SetFlag(FileObject->DeviceObject->Flags, DO_VERIFY_VOLUME);
            IoSetHardErrorOrVerifyDevice( IrpContext->Irp,
                                          FileObject->DeviceObject );
        }

        IrpContext->Irp->IoStatus = RaiseIosb;
        Ext2NormalizeAndRaiseStatus(IrpContext, RaiseIosb.Status );
    }

    return;
}


PEXT2_FCB
Ext2AllocateFcb (IN PEXT2_VCB   Vcb,
         IN PEXT2_MCB           Ext2Mcb,
         IN PEXT2_INODE         ext2_inode )
{
    PEXT2_FCB Fcb;

    Fcb = (PEXT2_FCB) (ExAllocateFromNPagedLookasideList( &(gExt2Global->Ext2FcbLookasideList)));

    if (Fcb == NULL)
    {
        Fcb = (PEXT2_FCB)ExAllocatePool(NonPagedPool, sizeof(EXT2_FCB));

        RtlZeroMemory(Fcb, sizeof(EXT2_FCB));

        SetFlag(Fcb->Flags, FCB_FROM_POOL);
    }
    else
    {
        RtlZeroMemory(Fcb, sizeof(EXT2_FCB));
    }

    if (!Fcb)
    {
        return NULL;
    }
    
    Fcb->Identifier.Type = EXT2FCB;
    Fcb->Identifier.Size = sizeof(EXT2_FCB);

    FsRtlInitializeFileLock (
        &Fcb->FileLockAnchor,
        NULL,
        NULL );
    
    Fcb->OpenHandleCount = 0;
    Fcb->ReferenceCount = 0;
    
    Fcb->Vcb = Vcb;

#if DBG    

    Fcb->AnsiFileName.Length = Ext2Mcb->ShortName.Length / sizeof(WCHAR);

    Fcb->AnsiFileName.MaximumLength = Ext2Mcb->ShortName.Length / sizeof(WCHAR) + 1;

    Fcb->AnsiFileName.Buffer = (PUCHAR) ExAllocatePool(
        PagedPool,
        Ext2Mcb->ShortName.Length / sizeof(WCHAR) + 1 );

    if (!Fcb->AnsiFileName.Buffer)
    {
        goto errorout;
    }

    Ext2WcharToChar(
        Fcb->AnsiFileName.Buffer,
        Ext2Mcb->ShortName.Buffer,
        Ext2Mcb->ShortName.Length / sizeof(WCHAR)
        );

    Fcb->AnsiFileName.Buffer[Ext2Mcb->ShortName.Length / sizeof(WCHAR)] = 0;

#endif

    Ext2Mcb->FileAttr = FILE_ATTRIBUTE_NORMAL;
    
    if (S_ISDIR(ext2_inode->i_mode))
    {
        SetFlag(Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY);
    }

    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY) || Ext2IsReadOnly(ext2_inode->i_mode))
    {
        SetFlag(Ext2Mcb->FileAttr, FILE_ATTRIBUTE_READONLY);
    }
    
    Fcb->ext2_inode = ext2_inode;

    Fcb->Ext2Mcb = Ext2Mcb;
    Ext2Mcb->Ext2Fcb = Fcb;
    
    RtlZeroMemory(&Fcb->CommonFCBHeader, sizeof(FSRTL_COMMON_FCB_HEADER));
    
    Fcb->CommonFCBHeader.NodeTypeCode = (USHORT) EXT2FCB;
    Fcb->CommonFCBHeader.NodeByteSize = sizeof(EXT2_FCB);
    Fcb->CommonFCBHeader.IsFastIoPossible = FastIoIsNotPossible;
    Fcb->CommonFCBHeader.Resource = &(Fcb->MainResource);
    Fcb->CommonFCBHeader.PagingIoResource = &(Fcb->PagingIoResource);

    Fcb->CommonFCBHeader.AllocationSize.QuadPart = (LONGLONG)(Fcb->ext2_inode->i_blocks * SECTOR_SIZE);
    Fcb->CommonFCBHeader.FileSize.QuadPart = (LONGLONG)(Fcb->ext2_inode->i_size);
    Fcb->CommonFCBHeader.ValidDataLength.QuadPart = (LONGLONG)(0x7fffffffffffffff);
    
    Fcb->SectionObject.DataSectionObject = NULL;
    Fcb->SectionObject.SharedCacheMap = NULL;
    Fcb->SectionObject.ImageSectionObject = NULL;

    ExInitializeResourceLite(&(Fcb->MainResource));
    ExInitializeResourceLite(&(Fcb->PagingIoResource));

    ExInitializeResourceLite(&(Fcb->CountResource));

    InsertTailList(&Vcb->FcbList, &Fcb->Next);

/*
    if (dir_inode != inode)
    {
        ParentFcb = Ext2SearchMcb(Vcb, dir_inode);

        if (ParentFcb)
        {
            Fcb->ParentFcb = ParentFcb;
            ExAcquireResourceExclusiveLite(&ParentFcb->CountResource, TRUE);
            ParentFcb->ReferenceCount++;
            ExReleaseResourceForThreadLite(
                    &ParentFcb->CountResource,
                    ExGetCurrentResourceThread());

        }
    }
*/

#if DBG

    ExAcquireResourceExclusiveLite(
        &gExt2Global->Resource,
        TRUE );

    gExt2Global->FcbAllocated++;

    ExReleaseResourceForThreadLite(
        &gExt2Global->Resource,
        ExGetCurrentResourceThread() );
#endif
    
    return Fcb;

#if DBG
errorout:
#endif

    if (Fcb)
    {

#if DBG
        if (Fcb->AnsiFileName.Buffer)
            ExFreePool(Fcb->AnsiFileName.Buffer);
#endif
        
        if (FlagOn(Fcb->Flags, FCB_FROM_POOL)) {
            
            ExFreePool( Fcb );
            
        } else {
            
            ExFreeToNPagedLookasideList(&(gExt2Global->Ext2FcbLookasideList), Fcb);
        }
        
    }

    return NULL;
}

VOID
Ext2FreeFcb (IN PEXT2_FCB Fcb)
{
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
        (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    FsRtlUninitializeFileLock(&Fcb->FileLockAnchor);

    ExDeleteResourceLite(&Fcb->CountResource);

    ExDeleteResourceLite(&Fcb->MainResource);
    
    ExDeleteResourceLite(&Fcb->PagingIoResource);
    
    RemoveEntryList(&Fcb->Next);

	Fcb->Ext2Mcb->Ext2Fcb = NULL;

    if(IsFlagOn(Fcb->Flags, FCB_FILE_DELETED))
    {
        if (Fcb->Ext2Mcb)
        {
            Ext2DeleteMcbNode(Fcb->Ext2Mcb->Parent, Fcb->Ext2Mcb);
            Ext2FreeMcb(Fcb->Ext2Mcb);
        }
    }

#if DBG    
    ExFreePool(Fcb->AnsiFileName.Buffer);
#endif

    ExFreePool(Fcb->ext2_inode);

/*

    if ( Fcb->ParentFcb )
    {
        ASSERT((Fcb->ParentFcb->Identifier.Type == FCB) &&
            (Fcb->ParentFcb->Identifier.Size == sizeof(EXT2_FCB)));
        
        ExAcquireResourceExclusiveLite(&Fcb->ParentFcb->CountResource, TRUE);
        Fcb->ParentFcb->ReferenceCount -- ;
        ExReleaseResourceForThreadLite(
            &Fcb->ParentFcb->CountResource,
            ExGetCurrentResourceThread());
        
        if (!Fcb->ParentFcb->ReferenceCount)
            Ext2FreeFcb(Fcb->ParentFcb);
    }
*/
    if (FlagOn(Fcb->Flags, FCB_FROM_POOL)) {

        ExFreePool( Fcb );

    } else {

        ExFreeToNPagedLookasideList(&(gExt2Global->Ext2FcbLookasideList), Fcb);
    }

#if DBG

    ExAcquireResourceExclusiveLite(
        &gExt2Global->Resource,
        TRUE );

    gExt2Global->FcbAllocated--;

    ExReleaseResourceForThreadLite(
        &gExt2Global->Resource,
        ExGetCurrentResourceThread() );
#endif

}

PEXT2_CCB
Ext2AllocateCcb (VOID)
{
    PEXT2_CCB Ccb;
    
    Ccb = (PEXT2_CCB) (ExAllocateFromNPagedLookasideList( &(gExt2Global->Ext2CcbLookasideList)));

    if (Ccb == NULL)
    {
        Ccb = (PEXT2_CCB)ExAllocatePool(NonPagedPool, sizeof(EXT2_CCB));

        RtlZeroMemory(Ccb, sizeof(EXT2_CCB));

        SetFlag(Ccb->Flags, CCB_FROM_POOL);
    }
    else
    {
        RtlZeroMemory(Ccb, sizeof(EXT2_CCB));
    }

    if (!Ccb)
    {
        return NULL;
    }
    
    Ccb->Identifier.Type = EXT2CCB;
    Ccb->Identifier.Size = sizeof(EXT2_CCB);
    
    Ccb->CurrentByteOffset = 0;
    
    Ccb->DirectorySearchPattern.Length = 0;
    Ccb->DirectorySearchPattern.MaximumLength = 0;
    Ccb->DirectorySearchPattern.Buffer = 0;
    
    return Ccb;
}

VOID
Ext2FreeCcb (IN PEXT2_CCB Ccb)
{
    ASSERT(Ccb != NULL);
    
    ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
        (Ccb->Identifier.Size == sizeof(EXT2_CCB)));
    
    if (Ccb->DirectorySearchPattern.Buffer != NULL)
    {
        ExFreePool(Ccb->DirectorySearchPattern.Buffer);
    }

    if (FlagOn(Ccb->Flags, CCB_FROM_POOL)) {

        ExFreePool( Ccb );

    } else {

        ExFreeToNPagedLookasideList(&(gExt2Global->Ext2CcbLookasideList), Ccb);
    }
}

PEXT2_MCB
Ext2AllocateMcb (PEXT2_VCB Vcb, PUNICODE_STRING FileName, ULONG FileAttr)
{
    PEXT2_MCB   Mcb = NULL;

    ULONG       i, Extra = 0;

    if (gExt2Global->McbAllocated > (gExt2Global->MaxDepth << 1))
        Extra = gExt2Global->McbAllocated - (gExt2Global->MaxDepth << 1) + gExt2Global->MaxDepth;
#if DBG
    Ext2DbgPrint(D_MEMORY, "Ext2AllocateMcb: CurrDepth=%xh/%xh/%xh FileName=%S\n", 
            gExt2Global->McbAllocated,
            gExt2Global->MaxDepth << 1,
            gExt2Global->FcbAllocated,
            FileName->Buffer);
#endif

    for (i = 0; i < Extra; i++)
    {
        Ext2FindUnusedMcb(Vcb->Ext2McbTree, &Mcb);

        if (Mcb && (Mcb->Inode != 2))
        {
            Ext2DbgPrint(D_MEMORY, "Ext2AllocateMcb: Mcb %S will be freed.\n", Mcb->ShortName.Buffer);

            if (Ext2DeleteMcbNode(Vcb->Ext2McbTree, Mcb))
                Ext2FreeMcb(Mcb);

            Mcb = NULL;
        }
        else
        {
            Ext2DbgPrint(D_MEMORY, "Ext2AllocateMcb: (%xh/%xh) Could not find free mcbs.\n", i, Extra); 

            break;
        }
    }
    
    Mcb = (PEXT2_MCB) (ExAllocateFromPagedLookasideList( &(gExt2Global->Ext2McbLookasideList)));
    
    if (Mcb == NULL)
    {
        Mcb = (PEXT2_MCB)ExAllocatePool(PagedPool, sizeof(EXT2_MCB));
        
        RtlZeroMemory(Mcb, sizeof(EXT2_MCB));
        
        SetFlag(Mcb->Flags, MCB_FROM_POOL);
    }
    else
    {
        RtlZeroMemory(Mcb, sizeof(EXT2_MCB));
    }
    
    if (!Mcb)
    {
        return NULL;
    }

    Mcb->Identifier.Type = EXT2MCB;
    Mcb->Identifier.Size = sizeof(EXT2_MCB);

    if (FileName && FileName->Length)
    {
        Mcb->ShortName.Length = FileName->Length;
        Mcb->ShortName.MaximumLength = Mcb->ShortName.Length + 2;

        Mcb->ShortName.Buffer = ExAllocatePool(PagedPool, Mcb->ShortName.MaximumLength);

        if (!Mcb->ShortName.Buffer)
            goto errorout;

        RtlZeroMemory(Mcb->ShortName.Buffer, Mcb->ShortName.MaximumLength);
        RtlCopyMemory(Mcb->ShortName.Buffer, FileName->Buffer, Mcb->ShortName.Length);
    } 

    Mcb->FileAttr = FileAttr;

    KeQuerySystemTime((PLARGE_INTEGER) (&Mcb->TickerCount));

    ExAcquireResourceExclusiveLite(
        &gExt2Global->Resource,
        TRUE );

    gExt2Global->McbAllocated++;

    ExReleaseResourceForThreadLite(
        &gExt2Global->Resource,
        ExGetCurrentResourceThread() );

    return Mcb;

errorout:

    if (Mcb)
    {
        if (Mcb->ShortName.Buffer)
            ExFreePool(Mcb->ShortName.Buffer);

        if (FlagOn(Mcb->Flags, MCB_FROM_POOL)) {

            ExFreePool( Mcb );

        } else {

            ExFreeToPagedLookasideList(&(gExt2Global->Ext2McbLookasideList), Mcb);
        }
    }

    return NULL;
}

VOID
Ext2FreeMcb (IN PEXT2_MCB Mcb)
{
    ASSERT(Mcb != NULL);
    
    ASSERT((Mcb->Identifier.Type == EXT2MCB) &&
        (Mcb->Identifier.Size == sizeof(EXT2_MCB)));

    Ext2DbgPrint(D_MEMORY, "Ext2FreeMcb: Mcb %S will be freed.\n", Mcb->ShortName.Buffer);

    if (Mcb->ShortName.Buffer)
        ExFreePool(Mcb->ShortName.Buffer);
    
    if (FlagOn(Mcb->Flags, MCB_FROM_POOL)) {

        ExFreePool( Mcb );

    } else {

        ExFreeToPagedLookasideList(&(gExt2Global->Ext2McbLookasideList), Mcb);
    }

    ExAcquireResourceExclusiveLite(
        &gExt2Global->Resource,
        TRUE );

    gExt2Global->McbAllocated--;

    ExReleaseResourceForThreadLite(
        &gExt2Global->Resource,
        ExGetCurrentResourceThread() );

}

PEXT2_FCB
Ext2CreateFcbFromMcb(PEXT2_VCB Vcb, PEXT2_MCB Mcb)
{
    PEXT2_FCB       Fcb = NULL;
    EXT2_INODE      Ext2Ino;

    if (Mcb->Ext2Fcb)
        return Mcb->Ext2Fcb;

    if (Ext2LoadInode(Vcb, Mcb->Inode, &Ext2Ino))
    {
        PEXT2_INODE pTmpInode = ExAllocatePool(PagedPool, sizeof(EXT2_INODE));
        if (!pTmpInode)
        {
            goto errorout;
        }

        RtlCopyMemory(pTmpInode, &Ext2Ino, sizeof(EXT2_INODE));
        Fcb = Ext2AllocateFcb(Vcb, Mcb, pTmpInode);
        if (!Fcb)
        {
            ExFreePool(pTmpInode);
        }
    }

errorout:

    return Fcb;
}

BOOLEAN
Ext2GetFullFileName(PEXT2_MCB Mcb, PUNICODE_STRING FileName)
{
    USHORT          Length = 0;
    PEXT2_MCB       TmpMcb = Mcb;
    PUNICODE_STRING FileNames[256];
    SHORT           Count = 0 , i = 0, j = 0;

    while(TmpMcb && Count < 256)
    {
        if (TmpMcb->Inode == EXT2_ROOT_INO)
            break;

        FileNames[Count++] = &TmpMcb->ShortName;
        Length += (2 + TmpMcb->ShortName.Length);

        TmpMcb = TmpMcb->Parent;
    }

    if (Count >= 256)
        return FALSE;

    if (Count ==0)
        Length = 2;
    
    FileName->Length = Length;
    FileName->MaximumLength = Length + 2;
    FileName->Buffer = ExAllocatePool(PagedPool, Length + 2);
    if (!FileName->Buffer)
        return FALSE;

    RtlZeroMemory(FileName->Buffer, FileName->MaximumLength);

    if (Count ==0)
    {
        FileName->Buffer[0] = L'\\';
        return  TRUE;
    }

    for (i = Count - 1; i >= 0 && j < (SHORT)(FileName->MaximumLength); i--)
    {
        FileName->Buffer[j++] = L'\\';

        RtlCopyMemory(&(FileName->Buffer[j]), FileNames[i]->Buffer, FileNames[i]->Length);
        j += FileNames[i]->Length / 2;
    }
    
    return TRUE;
}

PEXT2_MCB
Ext2SearchMcbTree(PEXT2_MCB Ext2Mcb, ULONG Inode)
{
    PEXT2_MCB TmpMcb = NULL;

    if (!Ext2Mcb)
        return NULL;

    if (Ext2Mcb->Inode == Inode)
        return Ext2Mcb;

    TmpMcb = Ext2SearchMcbTree(Ext2Mcb->Next, Inode);

    if (TmpMcb)
        return TmpMcb;

    TmpMcb = Ext2SearchMcbTree(Ext2Mcb->Child, Inode);

    return TmpMcb;
}


PEXT2_MCB
Ext2SearchMcb(PEXT2_MCB Parent, PUNICODE_STRING FileName)
{
    PEXT2_MCB TmpMcb = Parent->Child;

    while (TmpMcb)
    {
        if (!RtlCompareUnicodeString(
					&(TmpMcb->ShortName),
            		FileName, TRUE ))
            break;

        TmpMcb = TmpMcb->Next;
    }

    return TmpMcb;
}

VOID
Ext2AddMcbNode(PEXT2_MCB Parent, PEXT2_MCB Child)
{
    PEXT2_MCB TmpMcb = Parent->Child;

    if(IsFlagOn(Child->Flags, MCB_IN_TREE))
    {
        Ext2DbgPrint(D_MEMORY, "Ext2AddMcbNode: Child Mcb is alreay in the tree.\n");
        return;
    }

    if (TmpMcb)
    {
        while (TmpMcb->Next)
        {
            TmpMcb = TmpMcb->Next;
        }

        TmpMcb->Next = Child;
        Child->Parent = TmpMcb->Parent;
        Child->Next = NULL;
    }
    else
    {
        Parent->Child = Child;
        Child->Parent = Parent;
        Child->Next = NULL;
    }

    SetFlag(Child->Flags, MCB_IN_TREE);
}

BOOLEAN
Ext2DeleteMcbNode(PEXT2_MCB Ext2McbTree, PEXT2_MCB Ext2Mcb)
{
    PEXT2_MCB   TmpMcb;

    if(!IsFlagOn(Ext2Mcb->Flags, MCB_IN_TREE))
    {
        Ext2DbgPrint(D_MEMORY, "Ext2DeleteMcbNode: Child Mcb is not in the tree.\n");
        return TRUE;
    }

    if (Ext2Mcb->Parent)
    {
        if (Ext2Mcb->Parent->Child == Ext2Mcb)
        {
            Ext2Mcb->Parent->Child = Ext2Mcb->Next;
        }
        else
        {
            TmpMcb = Ext2Mcb->Parent->Child;

            while (TmpMcb && TmpMcb->Next != Ext2Mcb)
                TmpMcb = TmpMcb->Next;

            if (TmpMcb)
            {
                TmpMcb->Next = Ext2Mcb->Next;
            }
            else
            {
                // error
                return FALSE;
            }
        }
    }
    else if (Ext2Mcb->Child)
    {
        return FALSE;
    }

    ClearFlag(Ext2Mcb->Flags, MCB_IN_TREE);

    return TRUE;
}

int Ext2GetMcbDepth(PEXT2_MCB Ext2Mcb)
{
    int i=0;
    PEXT2_MCB TmpMcb = Ext2Mcb;

    while (TmpMcb)
    {
        TmpMcb = TmpMcb->Parent;
        i++;
    }

    return i;
}

BOOLEAN
Ext2CompareMcb(PEXT2_MCB Ext2Mcb1, PEXT2_MCB Ext2Mcb2)
{
    if (Ext2Mcb2 && Ext2Mcb2->Ext2Fcb == NULL && Ext2Mcb2->Child == NULL)
    {
        if (Ext2Mcb1 == NULL)
        {
            return TRUE;
        }
        else
        {
            if (!FlagOn(Ext2Mcb2->FileAttr, FILE_ATTRIBUTE_DIRECTORY) &&
                 FlagOn(Ext2Mcb1->FileAttr, FILE_ATTRIBUTE_DIRECTORY)  )
            {
                return TRUE;
            }
            else if (Ext2Mcb2->TickerCount < Ext2Mcb1->TickerCount)
            {
                   return TRUE;
            }
            else
            {
                int Depth1, Depth2;

                Depth1 = Ext2GetMcbDepth(Ext2Mcb1);
                Depth2 = Ext2GetMcbDepth(Ext2Mcb2);

                if (Depth2 > Depth1)
                {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

VOID
Ext2FindUnusedMcb (PEXT2_MCB Ext2McbTree, PEXT2_MCB* Ext2Mcb)
{
    if (!Ext2McbTree)
        return;

    if (Ext2CompareMcb(*Ext2Mcb, Ext2McbTree))
        *Ext2Mcb = Ext2McbTree;

    //Ext2DbgPrint(D_MEMORY, "Ext2FindUnusedMcb: Mcb->Inode=%xh Name=%S\n", Ext2McbTree->Inode, Ext2McbTree->ShortName.Buffer);

    Ext2FindUnusedMcb(Ext2McbTree->Child, Ext2Mcb);
    Ext2FindUnusedMcb(Ext2McbTree->Next,  Ext2Mcb);
}

VOID Ext2FreeMcbTree(PEXT2_MCB McbTree)
{
    if (!McbTree)
        return;

    if (McbTree->Child)
        Ext2FreeMcbTree(McbTree->Child);

    if (McbTree->Next)
        Ext2FreeMcbTree(McbTree->Next);

    Ext2FreeMcb(McbTree);
}

BOOLEAN
Ext2CheckSetBlock(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb, ULONG Block)
{
    ULONG           Group, dwBlk, Length;

    RTL_BITMAP      BlockBitmap;
    PVOID           BitmapCache;
    PBCB            BitmapBcb;

    LARGE_INTEGER   Offset;

    BOOLEAN         bModified = FALSE;


    Group = (Block - EXT2_FIRST_DATA_BLOCK) / (Vcb->ext2_super_block->s_blocks_per_group);

    dwBlk = (Block - EXT2_FIRST_DATA_BLOCK) % Vcb->ext2_super_block->s_blocks_per_group;


    Offset.QuadPart = (LONGLONG) Vcb->ext2_block;
    Offset.QuadPart = Offset.QuadPart * Vcb->ext2_group_desc[Group].bg_block_bitmap;

    if (Group == Vcb->ext2_groups - 1)
        Length = Vcb->ext2_super_block->s_blocks_count % Vcb->ext2_super_block->s_blocks_per_group;
    else
        Length = Vcb->ext2_super_block->s_blocks_per_group;

    if (dwBlk >= Length)
        return FALSE;
        
    if (!CcPinRead( Vcb->StreamObj,
                        &Offset,
                        Vcb->ext2_block,
                        TRUE,
                        &BitmapBcb,
                        &BitmapCache ) )
    {
        Ext2DbgPrint(D_MEMORY, "Ext2DeleteBlock: PinReading error ...\n");
        return FALSE;
    }

    RtlInitializeBitMap( &BlockBitmap,
                         BitmapCache,
                         Length );

    if (RtlCheckBit(&BlockBitmap, dwBlk) == 0)
    {
        Ext2DbgBreakPoint();
        RtlSetBits(&BlockBitmap, dwBlk, 1);
        bModified = TRUE;
    }

    if (bModified)
    {
	    CcSetDirtyPinnedData(BitmapBcb, NULL );

        Ext2RepinBcb(IrpContext, BitmapBcb);

        Ext2AddMcbEntry(Vcb, Offset.QuadPart, (LONGLONG)Vcb->ext2_block);
    }

    {
        CcUnpinData(BitmapBcb);
        BitmapBcb = NULL;
        BitmapCache = NULL;

        RtlZeroMemory(&BlockBitmap, sizeof(RTL_BITMAP));
    }
      
    return (!bModified);
}

BOOLEAN
Ext2CheckBitmapConsistency(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb)
{
    ULONG i, j, InodeBlocks;

    for (i = 0; i < Vcb->ext2_groups; i++)
    {
        Ext2CheckSetBlock(IrpContext, Vcb, Vcb->ext2_group_desc[i].bg_block_bitmap);
        Ext2CheckSetBlock(IrpContext, Vcb, Vcb->ext2_group_desc[i].bg_inode_bitmap);
        
        
        if (i == Vcb->ext2_groups - 1)
        {
            InodeBlocks = ((Vcb->ext2_super_block->s_inodes_count % Vcb->ext2_super_block->s_inodes_per_group) * sizeof(EXT2_INODE) + Vcb->ext2_block - 1) / (Vcb->ext2_block);
        }
        else
        {
            InodeBlocks = (Vcb->ext2_super_block->s_inodes_per_group * sizeof(EXT2_INODE) + Vcb->ext2_block - 1) / (Vcb->ext2_block);
        }

        for (j = 0; j < InodeBlocks; j++ )
            Ext2CheckSetBlock(IrpContext, Vcb, Vcb->ext2_group_desc[i].bg_inode_table + j);
    }

    return TRUE;
}

NTSTATUS
Ext2InitializeVcb(IN PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb, 
            PEXT2_SUPER_BLOCK Ext2Sb, PDEVICE_OBJECT TargetDevice,
            PDEVICE_OBJECT VolumeDevice, PVPB Vpb)
{
    BOOLEAN                     VcbResourceInitialized = FALSE;
    USHORT                      VolumeLabelLength;
    ULONG                       IoctlSize;
    BOOLEAN                     NotifySyncInitialized = FALSE;
    LONGLONG                    DiskSize;
    LONGLONG                    PartSize;
    NTSTATUS                    Status = STATUS_UNSUCCESSFUL;
    UNICODE_STRING              RootNode;
    USHORT                      Buffer[2];

    __try
    {
        if (!Vpb)
        {
            Status = STATUS_DEVICE_NOT_READY;
            __leave;
        }

        RtlZeroMemory(Vcb, sizeof(EXT2_VCB));
        
        Vcb->Identifier.Type = EXT2VCB;
        Vcb->Identifier.Size = sizeof(EXT2_VCB);

        Buffer[0] = L'\\';
        Buffer[1] = 0;

        RootNode.Buffer = Buffer;
        RootNode.MaximumLength = RootNode.Length = 2;
        Vcb->Ext2McbTree = Ext2AllocateMcb(Vcb, &RootNode, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_NORMAL);
        if (!Vcb->Ext2McbTree)
            __leave;

#if EXT2_READ_ONLY
        SetFlag(Vcb->Flags, VCB_READ_ONLY);
#endif //READ_ONLY

        if (FlagOn(gExt2Global->Flags, EXT2_SUPPORT_WRITING))
            ClearFlag(Vcb->Flags, VCB_READ_ONLY);

        if (FlagOn(Ext2Sb->s_feature_incompat, EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) ||
            FlagOn(Ext2Sb->s_feature_incompat, EXT3_FEATURE_INCOMPAT_RECOVER) ||
            FlagOn(Ext2Sb->s_feature_compat, EXT3_FEATURE_COMPAT_HAS_JOURNAL) )
        {
            SetFlag(Vcb->Flags, VCB_READ_ONLY);
        }

        ExInitializeResourceLite(&Vcb->MainResource);
        ExInitializeResourceLite(&Vcb->PagingIoResource);

        ExInitializeResourceLite(&Vcb->CountResource);
        
        VcbResourceInitialized = TRUE;

        Vcb->Ext2McbTree->Inode = EXT2_ROOT_INO;

        Vcb->Vpb = Vpb;

        Vpb->DeviceObject = VolumeDevice;

        {
            VolumeLabelLength = 16;
            while( (VolumeLabelLength > 0) &&
                   ((Ext2Sb->s_volume_name[VolumeLabelLength-1] == 0x00) ||
                    (Ext2Sb->s_volume_name[VolumeLabelLength-1] == 0x20)) )
                VolumeLabelLength--;

            Ext2CharToWchar(
                    Vcb->Vpb->VolumeLabel,
                    Ext2Sb->s_volume_name,
                    VolumeLabelLength );
            Vpb->VolumeLabelLength = VolumeLabelLength * 2;
        }

		Vpb->SerialNumber = ((ULONG*)Ext2Sb->s_uuid)[0] + ((ULONG*)Ext2Sb->s_uuid)[1] +
                            ((ULONG*)Ext2Sb->s_uuid)[2] + ((ULONG*)Ext2Sb->s_uuid)[3];


        Vcb->StreamObj = IoCreateStreamFileObject( NULL, Vcb->Vpb->RealDevice);

        if (Vcb->StreamObj)
        {
            Vcb->StreamObj->SectionObjectPointer = &(Vcb->SectionObject);
            Vcb->StreamObj->Vpb = Vcb->Vpb;
            Vcb->StreamObj->ReadAccess = TRUE;
            if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
            {
                Vcb->StreamObj->WriteAccess = TRUE;
                Vcb->StreamObj->DeleteAccess = TRUE;
            }
            else
            {
                Vcb->StreamObj->WriteAccess = TRUE;
                Vcb->StreamObj->DeleteAccess = TRUE;
            }
            Vcb->StreamObj->FsContext = (PVOID) Vcb;
            Vcb->StreamObj->FsContext2 = NULL;
            Vcb->StreamObj->Vpb = Vcb->Vpb;

            SetFlag(Vcb->StreamObj->Flags, FO_NO_INTERMEDIATE_BUFFERING);
        }
        else
        {
            __leave;
        }

        InitializeListHead(&Vcb->FcbList);

        InitializeListHead(&Vcb->NotifyList);

        FsRtlNotifyInitializeSync(&Vcb->NotifySync);

        NotifySyncInitialized = TRUE;

        Vcb->DeviceObject = VolumeDevice;
        
        Vcb->TargetDeviceObject = TargetDevice;
        
        Vcb->OpenFileHandleCount = 0;
        
        Vcb->ReferenceCount = 0;
        
        Vcb->ext2_super_block = Ext2Sb;
        
        Vcb->CommonFCBHeader.NodeTypeCode = (USHORT) EXT2VCB;
        Vcb->CommonFCBHeader.NodeByteSize = sizeof(EXT2_VCB);
        Vcb->CommonFCBHeader.IsFastIoPossible = FastIoIsNotPossible;
        Vcb->CommonFCBHeader.Resource = &(Vcb->MainResource);
        Vcb->CommonFCBHeader.PagingIoResource = &(Vcb->PagingIoResource);

        Vcb->Vpb->SerialNumber = 'MATT';

        IoctlSize = sizeof(DISK_GEOMETRY);
        
        Status = Ext2DiskIoControl(
            TargetDevice,
            IOCTL_DISK_GET_DRIVE_GEOMETRY,
            NULL,
            0,
            &Vcb->DiskGeometry,
            &IoctlSize );
        
        if (!NT_SUCCESS(Status))
        {
            __leave;
        }
        
        DiskSize =
            Vcb->DiskGeometry.Cylinders.QuadPart *
            Vcb->DiskGeometry.TracksPerCylinder *
            Vcb->DiskGeometry.SectorsPerTrack *
            Vcb->DiskGeometry.BytesPerSector;

        IoctlSize = sizeof(PARTITION_INFORMATION);
        
        Status = Ext2DiskIoControl(
            TargetDevice,
            IOCTL_DISK_GET_PARTITION_INFO,
            NULL,
            0,
            &Vcb->PartitionInformation,
            &IoctlSize );

        PartSize = Vcb->PartitionInformation.PartitionLength.QuadPart;
        
        if (!NT_SUCCESS(Status))
        {
            Vcb->PartitionInformation.StartingOffset.QuadPart = 0;
            
            Vcb->PartitionInformation.PartitionLength.QuadPart =
                DiskSize;

            PartSize = DiskSize;
            
            Status = STATUS_SUCCESS;
        }

        Vcb->CommonFCBHeader.AllocationSize.QuadPart =
        Vcb->CommonFCBHeader.FileSize.QuadPart = PartSize;

        Vcb->CommonFCBHeader.ValidDataLength.QuadPart = 
            (LONGLONG)(0x7fffffffffffffff);
/*
        Vcb->CommonFCBHeader.AllocationSize.QuadPart = (LONGLONG)(ext2_super_block->s_blocks_count - ext2_super_block->s_free_blocks_count)
            * (EXT2_MIN_BLOCK << ext2_super_block->s_log_block_size);
        Vcb->CommonFCBHeader.FileSize.QuadPart = Vcb->CommonFCBHeader.AllocationSize.QuadPart;
        Vcb->CommonFCBHeader.ValidDataLength.QuadPart = Vcb->CommonFCBHeader.AllocationSize.QuadPart;
*/
        {
                CC_FILE_SIZES FileSizes;

                FileSizes.AllocationSize.QuadPart =
                FileSizes.FileSize.QuadPart =
                    Vcb->CommonFCBHeader.AllocationSize.QuadPart;

                FileSizes.ValidDataLength.QuadPart= (LONGLONG)(0x7fffffffffffffff);

                CcInitializeCacheMap( Vcb->StreamObj,
                                      &FileSizes,
                                      TRUE,
                                      &(gExt2Global->CacheManagerNoOpCallbacks),
                                      Vcb );
        }

        Vcb->ext2_group_desc = Ext2LoadGroup(Vcb); 

        if (!Vcb->ext2_group_desc)
        {
            Status = STATUS_UNSUCCESSFUL;
            __leave;
        }

        FsRtlInitializeLargeMcb(&(Vcb->DirtyMcbs), PagedPool);

#if 0
        // Understanding Mcb Routines
        {
            LONGLONG	        ByteOffset;
            LONGLONG            DirtyVba;
            LONGLONG            DirtyLba;
            LONGLONG            DirtyLength;
            int		            i;

            Ext2AddMcbEntry(Vcb, 0x00000000, (LONGLONG)0x400);
            Ext2AddMcbEntry(Vcb, 0x00001000, (LONGLONG)0x400);
            FsRtlAddLargeMcbEntry (&(Vcb->DirtyMcbs), (LONGLONG)0x3000, (LONGLONG)0x30003000FFFF0000, (LONGLONG)0x1000);
            Ext2AddMcbEntry(Vcb, 0x00010000, (LONGLONG)0x400);
            Ext2AddMcbEntry(Vcb, 0x00100000, (LONGLONG)0x400);

            Ext2DbgPrint(D_MEMORY, "Ext2InitializeVcb: initial mcb lists:\n");
            for (i = 0; FsRtlGetNextLargeMcbEntry (&(Vcb->DirtyMcbs), i, &DirtyVba, &DirtyLba, &DirtyLength); i++)
            {
	            Ext2DbgPrint(D_MEMORY, "DirtyVba = %I64xh\n", DirtyVba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLba = %I64xh\n", DirtyLba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLen = %I64xh\n", DirtyLength);
            }

            ByteOffset = 0x100;
            Ext2LookupMcbEntry(Vcb, ByteOffset, &DirtyLba, &DirtyLength,
                                                (PLONGLONG)NULL, (PLONGLONG)NULL, (PULONG)NULL);

            {
	            Ext2DbgPrint(D_MEMORY, "Lookup DirtyVba = %I64xh:\n", ByteOffset);
	            Ext2DbgPrint(D_MEMORY, "DirtyLba = %I64xh\n", DirtyLba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLen = %I64xh\n", DirtyLength);
            }


            ByteOffset = 0x1000;
            Ext2LookupMcbEntry(Vcb, ByteOffset, &DirtyLba, &DirtyLength,
                                                (PLONGLONG)NULL, (PLONGLONG)NULL, (PULONG)NULL);
            {
	            Ext2DbgPrint(D_MEMORY, "Lookup DirtyVba = %I64xh:\n", ByteOffset);
	            Ext2DbgPrint(D_MEMORY, "DirtyLba = %I64xh\n", DirtyLba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLen = %I64xh\n", DirtyLength);
            }

            ByteOffset = 0x2000;
            Ext2LookupMcbEntry(Vcb, ByteOffset, &DirtyLba, &DirtyLength,
                                                (PLONGLONG)NULL, (PLONGLONG)NULL, (PULONG)NULL);
            {
	            Ext2DbgPrint(D_MEMORY, "Lookup DirtyVba = %I64xh:\n", ByteOffset);
	            Ext2DbgPrint(D_MEMORY, "DirtyLba = %I64xh\n", DirtyLba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLen = %I64xh\n", DirtyLength);
            }

            ByteOffset = 0x110000;
            Ext2LookupMcbEntry(Vcb, ByteOffset, &DirtyLba, &DirtyLength,
                                                (PLONGLONG)NULL, (PLONGLONG)NULL, (PULONG)NULL);
            {
	            Ext2DbgPrint(D_MEMORY, "Lookup DirtyVba = %I64xh:\n", ByteOffset);
	            Ext2DbgPrint(D_MEMORY, "DirtyLba = %I64xh\n", DirtyLba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLen = %I64xh\n", DirtyLength);
            }

            Ext2RemoveMcbEntry(Vcb, 0x00003000, (LONGLONG)0x400);

            Ext2DbgPrint(D_MEMORY, "Ext2InitializeVcb: After Removing 0x300(0x400):\n");

            for (i = 0; FsRtlGetNextLargeMcbEntry (&(Vcb->DirtyMcbs), i, &DirtyVba, &DirtyLba, &DirtyLength); i++)
            {
	            Ext2DbgPrint(D_MEMORY, "DirtyVba = %I64xh\n", DirtyVba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLba = %I64xh\n", DirtyLba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLen = %I64xh\n", DirtyLength);
            }

            Ext2RemoveMcbEntry(Vcb, 0x00003000, (LONGLONG)0x1000);

            Ext2DbgPrint(D_MEMORY, "Ext2InitializeVcb: After Removing 0x300(0x1000):\n");
            for (i = 0; FsRtlGetNextLargeMcbEntry (&(Vcb->DirtyMcbs), i, &DirtyVba, &DirtyLba, &DirtyLength); i++)
            {
	            Ext2DbgPrint(D_MEMORY, "DirtyVba = %I64xh\n", DirtyVba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLba = %I64xh\n", DirtyLba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLen = %I64xh\n", DirtyLength);
            }

            Ext2RemoveMcbEntry(Vcb, 0x00000000, (LONGLONG)0x400);
            Ext2RemoveMcbEntry(Vcb, 0x00001000, (LONGLONG)0x400);
            Ext2RemoveMcbEntry(Vcb, 0x00010000, (LONGLONG)0x400);
            Ext2RemoveMcbEntry(Vcb, 0x00100000, (LONGLONG)0x400);

        }
#endif

        InsertTailList(&(gExt2Global->VcbList), &Vcb->Next);

        if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
            Ext2CheckBitmapConsistency(IrpContext, Vcb);

        SetFlag(Vcb->Flags, VCB_INITIALIZED);
    }

    __finally
    {
        if (!NT_SUCCESS(Status))
        {
            if (NotifySyncInitialized)
            {
                FsRtlNotifyUninitializeSync(&Vcb->NotifySync);
            }
            
            if (Vcb->ext2_group_desc)
            {
                ExFreePool(Vcb->ext2_group_desc);
            }
            
            if (VcbResourceInitialized)
            {
                ExDeleteResourceLite(&Vcb->CountResource);
                ExDeleteResourceLite(&Vcb->MainResource);
                ExDeleteResourceLite(&Vcb->PagingIoResource);
            }
        }
    }

    return Status;
}


VOID
Ext2FreeVcb (IN PEXT2_VCB Vcb )
{
    ASSERT(Vcb != NULL);
    
    ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
        (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
    
    Ext2ClearVpbFlag(Vcb->Vpb, VPB_MOUNTED);

    FsRtlNotifyUninitializeSync(&Vcb->NotifySync);

    ExAcquireResourceExclusiveLite(
        &gExt2Global->Resource,
        TRUE );

    if (Vcb->StreamObj)
    {
        if (Vcb->StreamObj->PrivateCacheMap)
            Ext2SyncUninitializeCacheMap(Vcb->StreamObj);

        ObDereferenceObject(Vcb->StreamObj);
        Vcb->StreamObj = NULL;
    }
    
    RemoveEntryList(&Vcb->Next);
    
    ExReleaseResourceForThreadLite(
        &gExt2Global->Resource,
        ExGetCurrentResourceThread() );
    
    ExDeleteResourceLite(&Vcb->CountResource);

    ExDeleteResourceLite(&Vcb->MainResource);
    
    ExDeleteResourceLite(&Vcb->PagingIoResource);
    
    if (Vcb->ext2_super_block)
    {
        ExFreePool(Vcb->ext2_super_block);
    }

    if (Vcb->ext2_group_desc)
    {
        ExFreePool(Vcb->ext2_group_desc);
    }

#if DBG
    if (FsRtlNumberOfRunsInLargeMcb(&(Vcb->DirtyMcbs)) != 0)
    {
        LONGLONG            DirtyVba;
        LONGLONG            DirtyLba;
        LONGLONG            DirtyLength;
        int		            i;

        for (i = 0; FsRtlGetNextLargeMcbEntry (&(Vcb->DirtyMcbs), i, &DirtyVba, &DirtyLba, &DirtyLength); i++)
        {
	            Ext2DbgPrint(D_MEMORY, "DirtyVba = %I64xh\n", DirtyVba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLba = %I64xh\n", DirtyLba);
	            Ext2DbgPrint(D_MEMORY, "DirtyLen = %I64xh\n\n", DirtyLength);
        }

        Ext2DbgBreakPoint();
   }
#endif

    FsRtlUninitializeLargeMcb(&(Vcb->DirtyMcbs));

    Ext2FreeMcbTree(Vcb->Ext2McbTree);
    
    IoDeleteDevice(Vcb->DeviceObject);
}
