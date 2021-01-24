/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             block.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "ntifs.h"
#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL g_gExt2Global;

/* DEFINITIONS *************************************************************/


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2LockUserBuffer)
#pragma alloc_text(PAGE, Ext2GetUserBuffer)
#pragma alloc_text(PAGE, Ext2ReadSync)
#pragma alloc_text(PAGE, Ext2ReadDisk)
#pragma alloc_text(PAGE, Ext2ReadDiskOverrideVerify)
#pragma alloc_text(PAGE, Ext2DiskIoControl)
#pragma alloc_text(PAGE, Ext2ReadWriteBlocks)
#pragma alloc_text(PAGE, Ext2DiskShutDown)
#endif


/* FUNCTIONS ***************************************************************/


NTSTATUS
Ext2LockUserBuffer (IN PIRP     Irp,
            IN ULONG            Length,
            IN LOCK_OPERATION   Operation)
{
    NTSTATUS Status;
    
    ASSERT(Irp != NULL);
    
    if (Irp->MdlAddress != NULL)
    {
        return STATUS_SUCCESS;
    }
    
    IoAllocateMdl(Irp->UserBuffer, Length, FALSE, FALSE, Irp);
    
    if (Irp->MdlAddress == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    __try
    {
        MmProbeAndLockPages(Irp->MdlAddress, Irp->RequestorMode, Operation);
        
        Status = STATUS_SUCCESS;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        IoFreeMdl(Irp->MdlAddress);
        
        Irp->MdlAddress = NULL;
        
        Status = STATUS_INVALID_USER_BUFFER;
    }
    
    return Status;
}

PVOID
Ext2GetUserBuffer (IN PIRP Irp )
{
    ASSERT(Irp != NULL);
    
    if (Irp->MdlAddress)
    {
#if (_WIN32_WINNT >= 0x0500)
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
#else
        return MmGetSystemAddressForMdl(Irp->MdlAddress);
#endif
    }
    else
    {
        return Irp->UserBuffer;
    }
}

typedef struct _EXT2_CONTEXT {
        PIRP        MasterIrp;
        KEVENT      Event;
        BOOLEAN     Wait;
        ULONG       Blocks;
        ULONG       Length;
} EXT2_CONTEXT, *PEXT2_CONTEXT;


NTSTATUS
Ext2ReadWriteBlockSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context    )
{
    PEXT2_CONTEXT pContext = (PEXT2_CONTEXT)Context;

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

        pContext->MasterIrp->IoStatus = Irp->IoStatus;
    }

    IoFreeMdl( Irp->MdlAddress );
    IoFreeIrp( Irp );

    if (InterlockedDecrement(&pContext->Blocks) == 0)
    {
        pContext->MasterIrp->IoStatus.Information = 0;

        if (NT_SUCCESS(pContext->MasterIrp->IoStatus.Status))
        {
            pContext->MasterIrp->IoStatus.Information =
                pContext->Length;
        }

        KeSetEvent( &pContext->Event, 0, FALSE );
    }

    UNREFERENCED_PARAMETER( DeviceObject );

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
Ext2ReadWriteBlockAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PEXT2_CONTEXT pContext = (PEXT2_CONTEXT)Context;

    if (!NT_SUCCESS( Irp->IoStatus.Status ))
    {
        pContext->MasterIrp->IoStatus = Irp->IoStatus;
    }

    if (InterlockedDecrement(&pContext->Blocks) == 0)
    {

        pContext->MasterIrp->IoStatus.Information = 0;

        if (NT_SUCCESS(pContext->MasterIrp->IoStatus.Status))
        {
            pContext->MasterIrp->IoStatus.Information =
                pContext->Length;
        }

        IoMarkIrpPending( pContext->MasterIrp );

        ExFreePool(pContext);
    }

    UNREFERENCED_PARAMETER( DeviceObject );

    return STATUS_SUCCESS;

}

NTSTATUS
Ext2ReadWriteBlocks(
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB        Vcb,
    IN PEXT2_BDL        Ext2BDL,
    IN ULONG            Length,
    IN ULONG            Count,
    IN BOOLEAN          bVerify )
{
    PMDL                Mdl;
    PIRP                Irp;
    PIRP                MasterIrp = IrpContext->Irp;
    PIO_STACK_LOCATION  IrpSp;
    NTSTATUS            Status = STATUS_SUCCESS;
    PEXT2_CONTEXT       pContext = NULL;
    ULONG               i;

    ASSERT(MasterIrp);

    __try
    {

        pContext = ExAllocatePool(NonPagedPool, sizeof(EXT2_CONTEXT));

        if (!pContext)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlZeroMemory(pContext, sizeof(EXT2_CONTEXT));

        pContext->Wait = IrpContext->IsSynchronous;
        pContext->MasterIrp = MasterIrp;
        pContext->Blocks = Count;
        pContext->Length = 0;

        if (pContext->Wait)
                KeInitializeEvent(&(pContext->Event), NotificationEvent, FALSE);

        for (i = 0; i < Count; i++)
        {

            Irp = IoMakeAssociatedIrp( MasterIrp,
                                     (CCHAR)(Vcb->TargetDeviceObject->StackSize + 1) );
            if (!Irp)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            Mdl = IoAllocateMdl( (PCHAR)MasterIrp->UserBuffer +
                    Ext2BDL[i].Offset,
                    Ext2BDL[i].Length,
                    FALSE,
                    FALSE,
                    Irp );

            if (!Mdl)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }
            
            IoBuildPartialMdl( MasterIrp->MdlAddress,
                        Mdl,
                        (PCHAR)MasterIrp->UserBuffer + Ext2BDL[i].Offset,
                        Ext2BDL[i].Length );
                
            IoSetNextIrpStackLocation( Irp );
            IrpSp = IoGetCurrentIrpStackLocation( Irp );
                
            
            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Parameters.Read.Length = Ext2BDL[i].Length;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = Ext2BDL[i].Lba;

            IoSetCompletionRoutine(     Irp,
                                        IrpContext->IsSynchronous ?
                                        &Ext2ReadWriteBlockSyncCompletionRoutine :
                                        &Ext2ReadWriteBlockAsyncCompletionRoutine,
                                        (PVOID) pContext,
                                        TRUE,
                                        TRUE,
                                        TRUE );

            IrpSp = IoGetNextIrpStackLocation( Irp );

            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Parameters.Read.Length = Ext2BDL[i].Length;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = Ext2BDL[i].Lba;

            if ( bVerify )
            {
                SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );
            }

            Ext2BDL[i].Irp = Irp;
        }

        MasterIrp->AssociatedIrp.IrpCount = Count;

        if (IrpContext->IsSynchronous) {

            MasterIrp->AssociatedIrp.IrpCount += 1;
        }

        pContext->Length = Length;

        for (i = 0; i < Count; i++)
        {
            Status = IoCallDriver ( Vcb->TargetDeviceObject,
                                    Ext2BDL[i].Irp);
        }

        if (IrpContext->IsSynchronous)
        {
            KeWaitForSingleObject( &(pContext->Event),
                                   Executive, KernelMode, FALSE, NULL );

            KeClearEvent( &(pContext->Event) );
        }
    }

    __finally
    {
        if (IrpContext->IsSynchronous)
        {
            if (MasterIrp)
                Status = MasterIrp->IoStatus.Status;

            if (pContext)
                ExFreePool(pContext);

        }
        else
        {
            IrpContext->Irp = NULL;
            Status = STATUS_PENDING;
        }

        if ( AbnormalTermination() )
        {
            for (i = 0; i < Count; i++)
            {
                if (Ext2BDL[i].Irp != NULL )
                {
                    if ( Ext2BDL[i].Irp->MdlAddress != NULL )
                    {
                        IoFreeMdl( Ext2BDL[i].Irp->MdlAddress );
                    }
                    IoFreeIrp( Ext2BDL[i].Irp );
                }
            }
        }
    }

    return Status;
}


NTSTATUS
Ext2ReadSync(
    IN PDEVICE_OBJECT   DeviceObject,
    IN LONGLONG         Offset,
    IN ULONG            Length,
    OUT PVOID           Buffer,
    BOOLEAN             bVerify
    )
{
    KEVENT          Event;
    PIRP            Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status;


    ASSERT(DeviceObject != NULL);
    ASSERT(Buffer != NULL);
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_READ,
        DeviceObject,
        Buffer,
        Length,
        (PLARGE_INTEGER)(&Offset),
        &Event,
        &IoStatus
        );

    if (!Irp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (bVerify)
        SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
            &Event,
            Suspended,
            KernelMode,
            FALSE,
            NULL
            );
        Status = IoStatus.Status;
    }

    return Status;
}


NTSTATUS
Ext2ReadDisk(IN PDEVICE_OBJECT pDeviceObject,
         IN ULONG       lba,
         IN ULONG       offset,
         IN ULONG       Size,
         IN PVOID       Buffer)
{
    ULONG       Sectors;
    NTSTATUS    Status;
    PUCHAR      Buf;
    ULONG       off = 0;

    if (offset >= SECTOR_SIZE)
        lba += offset / SECTOR_SIZE;

    off = offset % SECTOR_SIZE;
    Sectors = (off + Size + SECTOR_SIZE - 1) / SECTOR_SIZE;


    Buf = ExAllocatePool(PagedPool, Sectors * SECTOR_SIZE);
    if (!Buf)
    {
        Ext2DbgPrint(D_BLOCK, "Ext2ReadDisk: no enough memory.\n");
        return STATUS_UNSUCCESSFUL;
    }

    Status = Ext2ReadSync(pDeviceObject, (LONGLONG)lba * SECTOR_SIZE, Sectors*SECTOR_SIZE, Buf, FALSE);

    if (!NT_SUCCESS(Status))
    {
        Ext2DbgPrint(D_BLOCK, "Ext2ReadDisk: Read Block Device error.\n");
                ExFreePool(Buf);
        return Status;
    }

    RtlCopyMemory(Buffer, &Buf[off], Size);

    ExFreePool(Buf);

    return Status;
}


NTSTATUS
Ext2ReadDiskOverrideVerify (IN PDEVICE_OBJECT pDeviceObject,
                IN ULONG    DiskSector, 
                IN ULONG    SectorCount,
                IN OUT PUCHAR Buffer)
{
    LARGE_INTEGER   offset;
    NTSTATUS        Status;
    ULONG           Length;

    offset.u.LowPart = DiskSector << 9;
    offset.u.HighPart = DiskSector >> 23;
    
    Length = SECTOR_SIZE * SectorCount;
    
    Status = Ext2ReadSync(pDeviceObject, offset.QuadPart, Length, Buffer, TRUE);
    
    if (!NT_SUCCESS (Status))
    {
        Ext2DbgPrint(D_BLOCK, "Ext2ReadDiskOverrideVerify: IO failed! Error code: %x\n", Status);
        return (Status);
    }

    return Status;
}


NTSTATUS 
Ext2DiskIoControl (IN PDEVICE_OBJECT   pDeviceObject,
           IN ULONG            IoctlCode,
           IN PVOID            InputBuffer,
           IN ULONG            InputBufferSize,
           IN OUT PVOID        OutputBuffer,
           IN OUT PULONG       OutputBufferSize)
{
    ULONG           OutBufferSize = 0;
    KEVENT          Event;
    PIRP            Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status;
    
    ASSERT(pDeviceObject != NULL);
    
    if (OutputBufferSize)
    {
        OutBufferSize = *OutputBufferSize;
    }
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    
    Irp = IoBuildDeviceIoControlRequest(
        IoctlCode,
        pDeviceObject,
        InputBuffer,
        InputBufferSize,
        OutputBuffer,
        OutBufferSize,
        FALSE,
        &Event,
        &IoStatus
        );
    
    if (Irp == NULL)
    {
        Ext2DbgPrint(D_BLOCK, "Ext2DiskIoControl: Building IRQ error!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = IoCallDriver(pDeviceObject, Irp);
    
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }
    
    if (OutputBufferSize)
    {
        *OutputBufferSize = IoStatus.Information;
    }
    
    return Status;
}


NTSTATUS
Ext2DiskShutDown(PEXT2_VCB Vcb)
{
    PIRP                Irp;
    KEVENT              Event;

    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatus;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_SHUTDOWN,
                                       Vcb->TargetDeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);

    if (Irp)
    {
        Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            Status = IoStatus.Status;
        }
    }
    else
    {
        Status = IoStatus.Status;
    }

    return Status;
}