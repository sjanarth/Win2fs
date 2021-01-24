/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             dispatch.c
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
#pragma alloc_text(PAGE, Ext2QueueRequest)
#pragma alloc_text(PAGE, Ext2DeQueueRequest)
#pragma alloc_text(PAGE, Ext2DispatchRequest)
#pragma alloc_text(PAGE, Ext2BuildRequest)
#endif


NTSTATUS
Ext2QueueRequest (IN PEXT2_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
    
    // IsSynchronous means we can block (so we don't requeue it)
    IrpContext->IsSynchronous = TRUE;

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_QUEUED_REQ);
    
    IoMarkIrpPending(IrpContext->Irp);
    
    ExInitializeWorkItem(
        &IrpContext->WorkQueueItem,
        Ext2DeQueueRequest,
        IrpContext );
    
    ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);
    
    return STATUS_PENDING;
}


VOID
Ext2DeQueueRequest (IN PVOID Context)
{
    PEXT2_IRP_CONTEXT IrpContext;

    IrpContext = (PEXT2_IRP_CONTEXT) Context;

    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
           (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

    __try
    {
        __try
        {
            FsRtlEnterFileSystem();

            if (!IrpContext->IsTopLevel)
            {
                IoSetTopLevelIrp((PIRP) FSRTL_FSP_TOP_LEVEL_IRP);
            }

            Ext2DispatchRequest(IrpContext);
        }
        __except (Ext2ExceptionFilter(IrpContext, GetExceptionCode()))
        {
            Ext2ExceptionHandler(IrpContext);
        }
    }
    __finally
    {
        IoSetTopLevelIrp(NULL);

        FsRtlExitFileSystem();
    }
}


NTSTATUS
Ext2DispatchRequest (IN PEXT2_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
    
    switch (IrpContext->MajorFunction)
    {
    case IRP_MJ_CREATE:
        return Ext2Create(IrpContext);
        
    case IRP_MJ_CLOSE:
        return Ext2Close(IrpContext);
        
    case IRP_MJ_READ:
        return Ext2Read(IrpContext);

    case IRP_MJ_WRITE:
        return Ext2Write(IrpContext);

    case IRP_MJ_FLUSH_BUFFERS:
        return Ext2Flush(IrpContext);

    case IRP_MJ_QUERY_INFORMATION:
        return Ext2QueryInformation(IrpContext);
        
    case IRP_MJ_SET_INFORMATION:
        return Ext2SetInformation(IrpContext);
        
    case IRP_MJ_QUERY_VOLUME_INFORMATION:
        return Ext2QueryVolumeInformation(IrpContext);

    case IRP_MJ_SET_VOLUME_INFORMATION:
        return Ext2SetVolumeInformation(IrpContext);

    case IRP_MJ_DIRECTORY_CONTROL:
        return Ext2DirectoryControl(IrpContext);
        
    case IRP_MJ_FILE_SYSTEM_CONTROL:
        return Ext2FileSystemControl(IrpContext);
        
    case IRP_MJ_DEVICE_CONTROL:
        return Ext2DeviceControl(IrpContext);

    case IRP_MJ_LOCK_CONTROL:
        return Ext2LockControl(IrpContext);
        
    case IRP_MJ_CLEANUP:
        return Ext2Cleanup(IrpContext);

    case IRP_MJ_SHUTDOWN:
        return Ext2ShutDown(IrpContext);
        
    default:

        Ext2DbgPrint(D_DISPATCH, "Ext2DispatchRequest: Unexpected major function: %#x\n",
            IrpContext->MajorFunction);

        IrpContext->Irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
        Ext2CompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        Ext2FreeIrpContext(IrpContext);
        return STATUS_DRIVER_INTERNAL_ERROR;
    }
}


NTSTATUS
Ext2BuildRequest (IN PDEVICE_OBJECT   DeviceObject,
          IN PIRP             Irp)
{
    BOOLEAN             AtIrqlPassiveLevel = FALSE;
    BOOLEAN             IsTopLevelIrp = FALSE;
    PEXT2_IRP_CONTEXT   IrpContext = NULL;
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    
    __try
    {
        __try
        {
            Ext2DbgPrintCall(D_DISPATCH, DeviceObject, Irp);
            
            AtIrqlPassiveLevel = (KeGetCurrentIrql() == PASSIVE_LEVEL);
            
            if (AtIrqlPassiveLevel)
            {
                FsRtlEnterFileSystem();
            }
            
            if (!IoGetTopLevelIrp())
            {
                IsTopLevelIrp = TRUE;
                IoSetTopLevelIrp(Irp);
            }
            
            IrpContext = Ext2AllocateIrpContext(DeviceObject, Irp);
            
            if (!IrpContext)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Status = Status;
                Ext2CompleteRequest(Irp, IO_NO_INCREMENT);
            }
            else
            {
                Status = Ext2DispatchRequest(IrpContext);
            }
        }
        __except (Ext2ExceptionFilter(IrpContext, GetExceptionCode()))
        {
            Status = Ext2ExceptionHandler(IrpContext);
        }
    }
    __finally
    {
        if (IsTopLevelIrp)
        {
            IoSetTopLevelIrp(NULL);
        }
        
        if (AtIrqlPassiveLevel)
        {
            FsRtlExitFileSystem();
        }       
    }
    
    return Status;
}

