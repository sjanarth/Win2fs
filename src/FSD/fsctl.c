/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             fsctl.c
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
#pragma alloc_text(PAGE, Ext2SetVpbFlag)
#pragma alloc_text(PAGE, Ext2ClearVpbFlag)
#pragma alloc_text(PAGE, Ext2LockVolume)
#pragma alloc_text(PAGE, Ext2UnlockVolume)
#pragma alloc_text(PAGE, Ext2UserFsRequest)
#pragma alloc_text(PAGE, Ext2MountVolume)
#pragma alloc_text(PAGE, Ext2PurgeVolume)
#pragma alloc_text(PAGE, Ext2PurgeFile)
#pragma alloc_text(PAGE, Ext2DismountVolume)
#pragma alloc_text(PAGE, Ext2IsVolumeMounted)
#pragma alloc_text(PAGE, Ext2VerifyVolume)
#pragma alloc_text(PAGE, Ext2FileSystemControl)
#endif


VOID
Ext2SetVpbFlag (IN PVPB     Vpb,
        IN USHORT   Flag )
{
    KIRQL OldIrql;
    
    IoAcquireVpbSpinLock(&OldIrql);
    
    Vpb->Flags |= Flag;
    
    IoReleaseVpbSpinLock(OldIrql);
}

VOID
Ext2ClearVpbFlag (IN PVPB     Vpb,
          IN USHORT   Flag )
{
    KIRQL OldIrql;
    
    IoAcquireVpbSpinLock(&OldIrql);
    
    Vpb->Flags &= ~Flag;
    
    IoReleaseVpbSpinLock(OldIrql);
}



NTSTATUS
Ext2LockVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB       Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    
    __try
    {
        ASSERT(IrpContext != NULL);
        
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
        
        ExAcquireResourceSharedLite(
            &Vcb->MainResource,
            TRUE            );
        
        VcbResourceAcquired = TRUE;
        
        if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED))
        {
            Ext2DbgPrint(D_FSCTL, "Ext2LockVolume: Volume is already locked.\n");
            
            Status = STATUS_ACCESS_DENIED;
            
            __leave;
        }
        
        if (Vcb->OpenFileHandleCount)
        {
            Ext2DbgPrint(D_FSCTL, "Ext2LockVolume: Open files exists.\n");
            
            Status = STATUS_ACCESS_DENIED;
            
            __leave;
        }
        
        ExReleaseResourceForThreadLite(
            &Vcb->MainResource,
            ExGetCurrentResourceThread()
            );
        
        VcbResourceAcquired = FALSE;
        
        Ext2PurgeVolume(Vcb, TRUE);
        
        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE
            );
        
        VcbResourceAcquired = TRUE;
        
        if (!IsListEmpty(&Vcb->FcbList))
        {
            Ext2DbgPrint(D_FSCTL, "Ext2LockVolume: Could not purge cached files.\n");
            
            Status = STATUS_ACCESS_DENIED;
            
            __leave;
        }
        
        SetFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
        
        Ext2SetVpbFlag(Vcb->Vpb, VPB_LOCKED);
        
        Ext2DbgPrint(D_FSCTL, "Ext2LockVolume: Volume locked.\n");
        
        Status = STATUS_SUCCESS;
    }
    __finally
    {
        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread());
        }
        
        if (!IrpContext->ExceptionInProgress)
        {
            IrpContext->Irp->IoStatus.Status = Status;
            
            Ext2CompleteRequest(
                IrpContext->Irp,
                (CCHAR)
                (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
            
            Ext2FreeIrpContext(IrpContext);
        }
    }
    
    return Status;
}

NTSTATUS
Ext2UnlockVolume (
         IN PEXT2_IRP_CONTEXT IrpContext
         )
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB       Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    
    __try
    {
        ASSERT(IrpContext != NULL);
        
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
        
        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE );
        
        VcbResourceAcquired = TRUE;
        
        if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED))
        {
            Ext2DbgPrint(D_FSCTL, ": Ext2UnlockVolume: Volume is not locked .\n");
            
            Status = STATUS_ACCESS_DENIED;
            
            __leave;
        }
        
        ClearFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
        
        Ext2ClearVpbFlag(Vcb->Vpb, VPB_LOCKED);
        
        Ext2DbgPrint(D_FSCTL, "Ext2UnlockVolume: Volume unlocked.\n");
        
        Status = STATUS_SUCCESS;
    }
    __finally
    {
        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }
        
        if (!IrpContext->ExceptionInProgress)
        {
            IrpContext->Irp->IoStatus.Status = Status;
            
            Ext2CompleteRequest(
                IrpContext->Irp,
                (CCHAR)
                (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
            
            Ext2FreeIrpContext(IrpContext);
        }
    }
    
    return Status;
}


NTSTATUS
Ext2UserFsRequest (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;
    ULONG               FsControlCode;
    NTSTATUS            Status;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
    
    Irp = IrpContext->Irp;
    
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    
#ifndef _GNU_NTIFS_
    FsControlCode =
        IoStackLocation->Parameters.FileSystemControl.FsControlCode;
#else
    FsControlCode = ((PEXTENDED_IO_STACK_LOCATION)
        IoStackLocation)->Parameters.FileSystemControl.FsControlCode;
#endif
    
    switch (FsControlCode)
    {
    case FSCTL_LOCK_VOLUME:
        Status = Ext2LockVolume(IrpContext);
        break;
        
    case FSCTL_UNLOCK_VOLUME:
        Status = Ext2UnlockVolume(IrpContext);
        break;
        
    case FSCTL_DISMOUNT_VOLUME:
        Status = Ext2DismountVolume(IrpContext);
        break;
        
    case FSCTL_IS_VOLUME_MOUNTED:
        Status = Ext2IsVolumeMounted(IrpContext);
        break;
        
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        IrpContext->Irp->IoStatus.Status = Status;
        Ext2CompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        Ext2FreeIrpContext(IrpContext);
    }
    
    return Status;
}

NTSTATUS
Ext2MountVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT              MainDeviceObject;
    BOOLEAN                     GlobalDataResourceAcquired = FALSE;
    PIRP                        Irp;
    PIO_STACK_LOCATION          IoStackLocation;
    PDEVICE_OBJECT              TargetDeviceObject;
    NTSTATUS                    Status = STATUS_UNRECOGNIZED_VOLUME;
    PDEVICE_OBJECT              VolumeDeviceObject = NULL;
    PEXT2_VCB                   Vcb;
    PEXT2_SUPER_BLOCK           Ext2Sb = NULL;
    
    __try
    {
        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
            (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
        MainDeviceObject = IrpContext->DeviceObject;

        //
        //  Make sure we can wait.
        //

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

        //
        // This request is only allowed on the main device object
        //
        if (MainDeviceObject != gExt2Global->DeviceObject)
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }
        
        ExAcquireResourceExclusiveLite(
            &(gExt2Global->Resource),
            TRUE );
        
        GlobalDataResourceAcquired = TRUE;
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        TargetDeviceObject =
            IoStackLocation->Parameters.MountVolume.DeviceObject;
        
        Ext2Sb = Ext2LoadSuper(TargetDeviceObject);

        if (Ext2Sb)
        {
            if (Ext2Sb->s_magic == EXT2_SUPER_MAGIC)
            {
                Ext2DbgPrint(D_FSCTL, "Win2fs: Ext2fs found.\n");
                Status = STATUS_SUCCESS;
            }
        }

        if (!NT_SUCCESS(Status))
        {
            __leave;
        }
        
        Status = IoCreateDevice(
            MainDeviceObject->DriverObject,
            sizeof(EXT2_VCB),
            NULL,
            FILE_DEVICE_DISK_FILE_SYSTEM,
            0,
            FALSE,
            &VolumeDeviceObject );
        
        if (!NT_SUCCESS(Status))
        {
            __leave;
        }
        
        VolumeDeviceObject->StackSize = (CCHAR)(TargetDeviceObject->StackSize + 1);

        ClearFlag(VolumeDeviceObject->Flags, DO_DEVICE_INITIALIZING);

        if (TargetDeviceObject->AlignmentRequirement > 
            VolumeDeviceObject->AlignmentRequirement) {

            VolumeDeviceObject->AlignmentRequirement = 
                TargetDeviceObject->AlignmentRequirement;
        }

        (IoStackLocation->Parameters.MountVolume.Vpb)->DeviceObject =
            VolumeDeviceObject;
        
        Vcb = (PEXT2_VCB) VolumeDeviceObject->DeviceExtension;

        Status = Ext2InitializeVcb(IrpContext, Vcb, Ext2Sb, TargetDeviceObject, 
                    VolumeDeviceObject, IoStackLocation->Parameters.MountVolume.Vpb);

    }

    __finally
    {
        if (GlobalDataResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &gExt2Global->Resource,
                ExGetCurrentResourceThread() );
        }

        if (!NT_SUCCESS(Status))
        {
            if (Ext2Sb)
            {
                ExFreePool(Ext2Sb);
            }

            if (VolumeDeviceObject)
            {
                IoDeleteDevice(VolumeDeviceObject);
            }
        }
        
        if (!IrpContext->ExceptionInProgress)
        {
            if (NT_SUCCESS(Status))
            {
                ClearFlag(VolumeDeviceObject->Flags, DO_DEVICE_INITIALIZING);
            }
            
            IrpContext->Irp->IoStatus.Status = Status;
            
            Ext2CompleteRequest(
                IrpContext->Irp,
                (CCHAR)
                (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
            
            Ext2FreeIrpContext(IrpContext);
        }
    }
    
    return Status;
}


NTSTATUS
Ext2VerifyVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PEXT2_SUPER_BLOCK       ext2_sb = NULL;
    PEXT2_VCB               Vcb;
    BOOLEAN                 VcbResourceAcquired = FALSE;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    
    __try
    {
        ASSERT(IrpContext != NULL);
        
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
        
        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE );
        
        VcbResourceAcquired = TRUE;
        
        if (!FlagOn(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME))
        {
            Status = STATUS_SUCCESS;
            __leave;
        }
        
        Irp = IrpContext->Irp;

        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        ext2_sb = Ext2LoadSuper(Vcb->TargetDeviceObject);

        if (ext2_sb && ext2_sb->s_magic == EXT2_SUPER_MAGIC)
        {
            ClearFlag(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME);
                
            Ext2DbgPrint(D_FSCTL, "Ext2VerifyVolume: Volume verify succeeded.\n");

            Status = STATUS_SUCCESS;

            __leave;
        }
        else
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()
                );
            
            VcbResourceAcquired = FALSE;
            
            Ext2PurgeVolume(Vcb, FALSE);
            
            ExAcquireResourceExclusiveLite(
                &Vcb->MainResource,
                TRUE
                );
            
            VcbResourceAcquired = TRUE;
            
            SetFlag(Vcb->Flags, VCB_DISMOUNT_PENDING);
            
            ClearFlag(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME);
            
            Ext2DbgPrint(D_FSCTL, "Ext2VerifyVolume: Volume verify failed\n");
            
            __leave;
        }

        __leave;
    }

    __finally
    {
        if (ext2_sb)
            ExFreePool(ext2_sb);

        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }
        
        if (!IrpContext->ExceptionInProgress)
        {
            IrpContext->Irp->IoStatus.Status = Status;
            
            Ext2CompleteRequest(
                IrpContext->Irp,
                (CCHAR)
                (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
            
            Ext2FreeIrpContext(IrpContext);
        }
    }
    
    return Status;
}


NTSTATUS
Ext2IsVolumeMounted (IN PEXT2_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    return Ext2VerifyVolume(IrpContext);
}


NTSTATUS
Ext2DismountVolume (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB       Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    
    __try
    {
        ASSERT(IrpContext != NULL);
        
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
        
        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE );
        
        VcbResourceAcquired = TRUE;
        
        if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED))
        {
            Ext2DbgPrint(D_FSCTL, "Ext2Dismount: Volume is not locked.\n");
            
            Status = STATUS_ACCESS_DENIED;
            
            __leave;
        }
        
        SetFlag(Vcb->Flags, VCB_DISMOUNT_PENDING);
        
        Ext2DbgPrint(D_FSCTL, "Ext2Dismount: Volume dismount pending.\n");
        
        Status = STATUS_SUCCESS;
    }
    __finally
    {
        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }
        
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
Ext2PurgeVolume (IN PEXT2_VCB Vcb,
         IN BOOLEAN  FlushBeforePurge )
{
    BOOLEAN         VcbResourceAcquired = FALSE;
    PEXT2_FCB       Fcb;
    LIST_ENTRY      FcbList;
    PLIST_ENTRY     ListEntry;
    PFCB_LIST_ENTRY FcbListEntry;

    BOOLEAN         Flush = FlushBeforePurge;
    
    __try
    {
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
            (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
        
        if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
        {
            Flush = FALSE;
        }

        ExAcquireResourceSharedLite(
            &Vcb->MainResource,
            TRUE );
        
        VcbResourceAcquired = TRUE;
        
        InitializeListHead(&FcbList);
        
        for (ListEntry = Vcb->FcbList.Flink;
             ListEntry != &Vcb->FcbList;
             ListEntry = ListEntry->Flink  )
        {
            Fcb = CONTAINING_RECORD(ListEntry, EXT2_FCB, Next);
            
            ExAcquireResourceExclusiveLite(
                &Fcb->CountResource,
                TRUE );
            
            Fcb->ReferenceCount++;

            ExReleaseResourceForThreadLite(
                &Fcb->CountResource,
                ExGetCurrentResourceThread());

#if DBG
            Ext2DbgPrint(D_FSCTL, "Ext2PurgeVolume: %s refercount=%xh\n", Fcb->AnsiFileName.Buffer, Fcb->ReferenceCount);
#endif  
            
            FcbListEntry = ExAllocatePool(PagedPool, sizeof(FCB_LIST_ENTRY));

            if (!FcbListEntry)
            {
                Ext2DbgPrint(D_FSCTL, "Ext2PurgeVolume: Error allocating FcbListEntry ...\n");
            }
            
            FcbListEntry->Fcb = Fcb;
            
            InsertTailList(&FcbList, &FcbListEntry->Next);
        }
        
        ExReleaseResourceForThreadLite(
            &Vcb->MainResource,
            ExGetCurrentResourceThread() );
        
        VcbResourceAcquired = FALSE;
        
        while (!IsListEmpty(&FcbList))
        {
            ListEntry = RemoveHeadList(&FcbList);
            
            FcbListEntry = CONTAINING_RECORD(ListEntry, FCB_LIST_ENTRY, Next);
            
            Fcb = FcbListEntry->Fcb;
            
            Ext2PurgeFile(Fcb, FlushBeforePurge);
            
            if (!Fcb->OpenHandleCount && Fcb->ReferenceCount == 1)
            {
#if DBG
                Ext2DbgPrint(D_FSCTL, "Ext2FreeFcb %s.\n", Fcb->AnsiFileName.Buffer);
#endif
                Ext2FreeFcb(Fcb);
            }
            
            ExFreePool(FcbListEntry);
        }
        
        Ext2DbgPrint(D_FSCTL, "Ext2PurgeVolume: Volume flushed and purged.\n");
    }
    __finally
    {
        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread() );
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
Ext2PurgeFile (IN PEXT2_FCB Fcb,
           IN BOOLEAN  FlushBeforePurge )
{
    IO_STATUS_BLOCK    IoStatus;

    ASSERT(Fcb != NULL);
        
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
        (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    
    if(!IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY) && FlushBeforePurge)
    {
#if DBG
        Ext2DbgPrint(D_FSCTL, "Ext2PurgeFile: CcFlushCache on %s.\n", Fcb->AnsiFileName.Buffer);
#endif        
        CcFlushCache(&Fcb->SectionObject, NULL, 0, &IoStatus);
        ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);
    }
    
    if (Fcb->SectionObject.ImageSectionObject)
    {

#if DBG
        Ext2DbgPrint(D_FSCTL, "Ext2PurgeFile: MmFlushImageSection on %s.\n", Fcb->AnsiFileName.Buffer);
#endif
    
        MmFlushImageSection(&Fcb->SectionObject, MmFlushForWrite);
    }
    
    if (Fcb->SectionObject.DataSectionObject)
    {
#if DBG
        Ext2DbgPrint(D_FSCTL, "Ext2PurgeFile: CcPurgeCacheSection on %s.\n", Fcb->AnsiFileName.Buffer);
#endif
        CcPurgeCacheSection(&Fcb->SectionObject, NULL, 0, FALSE);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
Ext2FileSystemControl (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS    Status;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
    
    switch (IrpContext->MinorFunction)
    {
    case IRP_MN_USER_FS_REQUEST:
        Status = Ext2UserFsRequest(IrpContext);
        break;
        
    case IRP_MN_MOUNT_VOLUME:
        Status = Ext2MountVolume(IrpContext);
        break;
        
    case IRP_MN_VERIFY_VOLUME:
        Status = Ext2VerifyVolume(IrpContext);
        break;
        
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        IrpContext->Irp->IoStatus.Status = Status;
        Ext2CompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        Ext2FreeIrpContext(IrpContext);
    }
    
    return Status;
}
