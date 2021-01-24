/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             volinfo.c
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
#pragma alloc_text(PAGE, Ext2QueryVolumeInformation)
#pragma alloc_text(PAGE, Ext2SetVolumeInformation)
#endif


NTSTATUS
Ext2QueryVolumeInformation (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB               Vcb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FS_INFORMATION_CLASS    FsInformationClass;
    ULONG                   Length;
    PVOID                   Buffer;
    BOOLEAN                 VcbResourceAcquired = FALSE;

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
        
        if (!ExAcquireResourceSharedLite(
            &Vcb->MainResource,
            IrpContext->IsSynchronous
            ))
        {
            Status = STATUS_PENDING;
            __leave;
        }
        
        VcbResourceAcquired = TRUE;
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        FsInformationClass =
            IoStackLocation->Parameters.QueryVolume.FsInformationClass;
        
        Length = IoStackLocation->Parameters.QueryVolume.Length;
        
        Buffer = Irp->AssociatedIrp.SystemBuffer;
        
        RtlZeroMemory(Buffer, Length);
        
        switch (FsInformationClass)
        {
        case FileFsVolumeInformation:
            {
                PFILE_FS_VOLUME_INFORMATION FsVolInfo;
                ULONG                       VolumeLabelLength;
                ULONG                       RequiredLength;
                
                if (Length < sizeof(FILE_FS_VOLUME_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FsVolInfo = (PFILE_FS_VOLUME_INFORMATION) Buffer;
                
                FsVolInfo->VolumeCreationTime.QuadPart = 0;
                
                FsVolInfo->VolumeSerialNumber = Vcb->Vpb->SerialNumber;

                VolumeLabelLength = Vcb->Vpb->VolumeLabelLength;
                
                FsVolInfo->VolumeLabelLength = VolumeLabelLength;
                
                // I don't know what this means
                FsVolInfo->SupportsObjects = FALSE;

                RequiredLength = sizeof(FILE_FS_VOLUME_INFORMATION)
                    + VolumeLabelLength - sizeof(WCHAR);
                
                if (Length < RequiredLength)
                {
                    Irp->IoStatus.Information =
                        sizeof(FILE_FS_VOLUME_INFORMATION);
                    Status = STATUS_BUFFER_OVERFLOW;
                    __leave;
                }

                RtlCopyMemory(FsVolInfo->VolumeLabel, Vcb->Vpb->VolumeLabel, Vcb->Vpb->VolumeLabelLength);

                Irp->IoStatus.Information = RequiredLength;
                Status = STATUS_SUCCESS;
                __leave;
            }
            
        case FileFsSizeInformation:
            {
                PFILE_FS_SIZE_INFORMATION FsSizeInfo;
                
                if (Length < sizeof(FILE_FS_SIZE_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FsSizeInfo = (PFILE_FS_SIZE_INFORMATION) Buffer;
                
                {
                    FsSizeInfo->TotalAllocationUnits.QuadPart =
                        Vcb->ext2_super_block->s_blocks_count;
                    
                    FsSizeInfo->AvailableAllocationUnits.QuadPart = 
                        Vcb->ext2_super_block->s_free_blocks_count;
                }
                
                FsSizeInfo->SectorsPerAllocationUnit =
                    Vcb->ext2_block / Vcb->DiskGeometry.BytesPerSector;
                
                FsSizeInfo->BytesPerSector =
                    Vcb->DiskGeometry.BytesPerSector;
                
                Irp->IoStatus.Information = sizeof(FILE_FS_SIZE_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }
            
        case FileFsDeviceInformation:
            {
                PFILE_FS_DEVICE_INFORMATION FsDevInfo;
                
                if (Length < sizeof(FILE_FS_DEVICE_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FsDevInfo = (PFILE_FS_DEVICE_INFORMATION) Buffer;
                
                FsDevInfo->DeviceType =
                    Vcb->TargetDeviceObject->DeviceType;
                
                FsDevInfo->Characteristics =
                    Vcb->TargetDeviceObject->Characteristics;
                
                if (FlagOn(Vcb->Flags, VCB_READ_ONLY))
                {
                    SetFlag(FsDevInfo->Characteristics,
                        FILE_READ_ONLY_DEVICE   );
                }
                
                Irp->IoStatus.Information = sizeof(FILE_FS_DEVICE_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }
            
        case FileFsAttributeInformation:
            {
                PFILE_FS_ATTRIBUTE_INFORMATION  FsAttrInfo;
                ULONG                           RequiredLength;
                
                if (Length < sizeof(FILE_FS_ATTRIBUTE_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FsAttrInfo =
                    (PFILE_FS_ATTRIBUTE_INFORMATION) Buffer;
                
                FsAttrInfo->FileSystemAttributes =
                    FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES;
                
                FsAttrInfo->MaximumComponentNameLength = EXT2_NAME_LEN;
                
                FsAttrInfo->FileSystemNameLength = 10;
                
                RequiredLength = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
                    10 - sizeof(WCHAR);
                
                if (Length < RequiredLength)
                {
                    Irp->IoStatus.Information =
                        sizeof(FILE_FS_ATTRIBUTE_INFORMATION);
                    Status = STATUS_BUFFER_OVERFLOW;
                    __leave;
                }
                
                RtlCopyMemory(
                    FsAttrInfo->FileSystemName,
                    L"EXT2\0", 8);
                
                Irp->IoStatus.Information = RequiredLength;
                Status = STATUS_SUCCESS;
                __leave;
            }

#if (_WIN32_WINNT >= 0x0500)

        case FileFsFullSizeInformation:
            {
                PFILE_FS_FULL_SIZE_INFORMATION PFFFSI;

                if (Length < sizeof(FILE_FS_FULL_SIZE_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }

                PFFFSI = (PFILE_FS_FULL_SIZE_INFORMATION) Buffer;

/*
                typedef struct _FILE_FS_FULL_SIZE_INFORMATION {
                    LARGE_INTEGER   TotalAllocationUnits;
                    LARGE_INTEGER   CallerAvailableAllocationUnits;
                    LARGE_INTEGER   ActualAvailableAllocationUnits;
                    ULONG           SectorsPerAllocationUnit;
                    ULONG           BytesPerSector;
                } FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;
*/

                {
                    PFFFSI->TotalAllocationUnits.QuadPart =
                        Vcb->ext2_super_block->s_blocks_count;

                    PFFFSI->CallerAvailableAllocationUnits.QuadPart =
                        Vcb->ext2_super_block->s_free_blocks_count;

                    /* - Vcb->ext2_super_block->s_r_blocks_count; */

                    PFFFSI->ActualAvailableAllocationUnits.QuadPart = 
                        Vcb->ext2_super_block->s_free_blocks_count;
                }

                PFFFSI->SectorsPerAllocationUnit =
                    Vcb->ext2_block / Vcb->DiskGeometry.BytesPerSector;

                PFFFSI->BytesPerSector = Vcb->DiskGeometry.BytesPerSector;

                Irp->IoStatus.Information = sizeof(FILE_FS_FULL_SIZE_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }

#endif // (_WIN32_WINNT >= 0x0500)

        default:
            Status = STATUS_INVALID_INFO_CLASS;
        }
    }

    __finally
    {
        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread() );
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

NTSTATUS
Ext2SetVolumeInformation (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB               Vcb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FS_INFORMATION_CLASS    FsInformationClass;

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

        if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
        {
            Status = STATUS_MEDIA_WRITE_PROTECTED;
            __leave;
        }
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        //Notes: SetVolume is not defined in ntddk.h of win2k ddk,
        //       But it's same to QueryVolume ....
        FsInformationClass =
            IoStackLocation->Parameters./*SetVolume*/QueryVolume.FsInformationClass;
        
        switch (FsInformationClass)
        {
        case FileFsLabelInformation:
            {
                PFILE_FS_LABEL_INFORMATION		VolLabelInfo = NULL;
			    ULONG                           VolLabelLen;


                VolLabelInfo = (PFILE_FS_LABEL_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
        
                VolLabelLen = VolLabelInfo->VolumeLabelLength;

			    if(VolLabelLen > MAXIMUM_VOLUME_LABEL_LENGTH)
			    {
                    Status = STATUS_INVALID_VOLUME_LABEL;
				    __leave;
			    }
               
                RtlCopyMemory(Vcb->Vpb->VolumeLabel, VolLabelInfo->VolumeLabel, VolLabelInfo->VolumeLabelLength);

                RtlZeroMemory(Vcb->ext2_super_block->s_volume_name, 16);

                Ext2WcharToChar(Vcb->ext2_super_block->s_volume_name,
                                    VolLabelInfo->VolumeLabel, 
                                    VolLabelLen / 2 );

                Vcb->Vpb->VolumeLabelLength = (USHORT) VolLabelLen;

                if (Ext2SaveSuper(IrpContext, Vcb))
                    Status = STATUS_SUCCESS;

                Irp->IoStatus.Information = 0;

			}
			break;

          default:
            Status = STATUS_INVALID_INFO_CLASS;
        }
    }

    __finally
    {
       
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