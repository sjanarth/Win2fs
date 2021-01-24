/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             fileinfo.c
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
#pragma alloc_text(PAGE, Ext2QueryInformation)
#pragma alloc_text(PAGE, Ext2SetInformation)
#pragma alloc_text(PAGE, Ext2ExpandFileAllocation)
#pragma alloc_text(PAGE, Ext2TruncateFileAllocation)
#pragma alloc_text(PAGE, Ext2SetDispositionInfo)
#pragma alloc_text(PAGE, Ext2SetRenameInfo)
#endif


NTSTATUS
Ext2QueryInformation (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT            FileObject;
    PEXT2_FCB               Fcb;
    PEXT2_CCB               Ccb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FILE_INFORMATION_CLASS  FileInformationClass;
    ULONG                   Length;
    PVOID                   Buffer;
    BOOLEAN                 FcbResourceAcquired = FALSE;
    
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
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        
        ASSERT(Fcb != NULL);
        
        //
        // This request is not allowed on volumes
        //
        if (Fcb->Identifier.Type == EXT2VCB)
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }
        
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
            (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
        
        if (!IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY) && !FlagOn(Fcb->Flags, FCB_PAGE_FILE))
        {
            if (!ExAcquireResourceSharedLite(
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
        
        ASSERT(Ccb != NULL);
        
        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        FileInformationClass =
            IoStackLocation->Parameters.QueryFile.FileInformationClass;
        
        Length = IoStackLocation->Parameters.QueryFile.Length;
        
        Buffer = Irp->AssociatedIrp.SystemBuffer;
        
        RtlZeroMemory(Buffer, Length);
        
        switch (FileInformationClass)
        {
        case FileBasicInformation:
            {
                PFILE_BASIC_INFORMATION FileBasicInformation;
                
                if (Length < sizeof(FILE_BASIC_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FileBasicInformation = (PFILE_BASIC_INFORMATION) Buffer;
                
                FileBasicInformation->CreationTime = Ext2SysTime(Fcb->ext2_inode->i_ctime);
                
                FileBasicInformation->LastAccessTime = Ext2SysTime(Fcb->ext2_inode->i_atime);
                
                FileBasicInformation->LastWriteTime = Ext2SysTime(Fcb->ext2_inode->i_mtime);
                
                FileBasicInformation->ChangeTime = Ext2SysTime(Fcb->ext2_inode->i_mtime);
                
                FileBasicInformation->FileAttributes = Fcb->Ext2Mcb->FileAttr;
                
                Irp->IoStatus.Information = sizeof(FILE_BASIC_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }

#if (_WIN32_WINNT >= 0x0500)

        case FileAttributeTagInformation:
            {
                PFILE_ATTRIBUTE_TAG_INFORMATION FATI;
                
                if (Length < sizeof(FILE_ATTRIBUTE_TAG_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FATI = (PFILE_ATTRIBUTE_TAG_INFORMATION) Buffer;
                
                FATI->FileAttributes = Fcb->Ext2Mcb->FileAttr;
                FATI->ReparseTag = 0;
                
                Irp->IoStatus.Information = sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }
#endif // (_WIN32_WINNT >= 0x0500)


        case FileStandardInformation:
            {
                PFILE_STANDARD_INFORMATION FileStandardInformation;
                
                if (Length < sizeof(FILE_STANDARD_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FileStandardInformation = (PFILE_STANDARD_INFORMATION) Buffer;
                
                FileStandardInformation->AllocationSize.QuadPart =
                    (LONGLONG)(Fcb->ext2_inode->i_size);
                
                FileStandardInformation->EndOfFile.QuadPart =
                    (LONGLONG)(Fcb->ext2_inode->i_size);
                
                FileStandardInformation->NumberOfLinks = Fcb->ext2_inode->i_links_count;
                
                if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
                    FileStandardInformation->DeletePending = FALSE;
                else
                    FileStandardInformation->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);                
                
                if (Fcb->Ext2Mcb->FileAttr & FILE_ATTRIBUTE_DIRECTORY)
                {
                    FileStandardInformation->Directory = TRUE;
                }
                else
                {
                    FileStandardInformation->Directory = FALSE;
                }
                
                Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }
            
        case FileInternalInformation:
            {
                PFILE_INTERNAL_INFORMATION FileInternalInformation;
                
                if (Length < sizeof(FILE_INTERNAL_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FileInternalInformation = (PFILE_INTERNAL_INFORMATION) Buffer;
                
                // The "inode number"
                FileInternalInformation->IndexNumber.QuadPart = (LONGLONG)Fcb->Ext2Mcb->Inode;
                
                Irp->IoStatus.Information = sizeof(FILE_INTERNAL_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }
            
        case FileEaInformation:
            {
                PFILE_EA_INFORMATION FileEaInformation;
                
                if (Length < sizeof(FILE_EA_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FileEaInformation = (PFILE_EA_INFORMATION) Buffer;
                
                // Romfs doesn't have any extended attributes
                FileEaInformation->EaSize = 0;
                
                Irp->IoStatus.Information = sizeof(FILE_EA_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }
            
        case FileNameInformation:
            {
                PFILE_NAME_INFORMATION FileNameInformation;
                
                if (Length < sizeof(FILE_NAME_INFORMATION) +
                    Fcb->Ext2Mcb->ShortName.Length - sizeof(WCHAR))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FileNameInformation = (PFILE_NAME_INFORMATION) Buffer;
                
                FileNameInformation->FileNameLength = Fcb->Ext2Mcb->ShortName.Length;
                
                RtlCopyMemory(
                    FileNameInformation->FileName,
                    Fcb->Ext2Mcb->ShortName.Buffer,
                    Fcb->Ext2Mcb->ShortName.Length );
                
                Irp->IoStatus.Information = sizeof(FILE_NAME_INFORMATION) +
                    Fcb->Ext2Mcb->ShortName.Length - sizeof(WCHAR);
                Status = STATUS_SUCCESS;
                __leave;
            }
            
        case FilePositionInformation:
            {
                PFILE_POSITION_INFORMATION FilePositionInformation;
                
                if (Length < sizeof(FILE_POSITION_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FilePositionInformation = (PFILE_POSITION_INFORMATION) Buffer;
                
                FilePositionInformation->CurrentByteOffset =
                    FileObject->CurrentByteOffset;
                
                Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);
                Status = STATUS_SUCCESS;
                __leave;
            }
            
        case FileAllInformation:
            {
                PFILE_ALL_INFORMATION       FileAllInformation;
                PFILE_BASIC_INFORMATION     FileBasicInformation;
                PFILE_STANDARD_INFORMATION  FileStandardInformation;
                PFILE_INTERNAL_INFORMATION  FileInternalInformation;
                PFILE_EA_INFORMATION        FileEaInformation;
                PFILE_POSITION_INFORMATION  FilePositionInformation;
                PFILE_NAME_INFORMATION      FileNameInformation;
                
                if (Length < sizeof(FILE_ALL_INFORMATION))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    __leave;
                }
                
                FileAllInformation = (PFILE_ALL_INFORMATION) Buffer;
                
                FileBasicInformation =
                    &FileAllInformation->BasicInformation;
                
                FileStandardInformation =
                    &FileAllInformation->StandardInformation;
                
                FileInternalInformation =
                    &FileAllInformation->InternalInformation;
                
                FileEaInformation =
                    &FileAllInformation->EaInformation;
                
                FilePositionInformation =
                    &FileAllInformation->PositionInformation;
                
                FileNameInformation =
                    &FileAllInformation->NameInformation;
                
                FileBasicInformation->CreationTime = Ext2SysTime(Fcb->ext2_inode->i_ctime);
                
                FileBasicInformation->LastAccessTime = Ext2SysTime(Fcb->ext2_inode->i_atime);
                
                FileBasicInformation->LastWriteTime = Ext2SysTime(Fcb->ext2_inode->i_mtime);
                
                FileBasicInformation->ChangeTime = Ext2SysTime(Fcb->ext2_inode->i_mtime);
                
                FileBasicInformation->FileAttributes = Fcb->Ext2Mcb->FileAttr;
                
                FileStandardInformation->AllocationSize.QuadPart =
                    (LONGLONG)(Fcb->ext2_inode->i_size);
                
                FileStandardInformation->EndOfFile.QuadPart =
                    (LONGLONG)(Fcb->ext2_inode->i_size);
                
                FileStandardInformation->NumberOfLinks = Fcb->ext2_inode->i_links_count;

                if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
                    FileStandardInformation->DeletePending = FALSE;
                else
                    FileStandardInformation->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);
                
                if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
                {
                    FileStandardInformation->Directory = TRUE;
                }
                else
                {
                    FileStandardInformation->Directory = FALSE;
                }
                
                // The "inode number"
                FileInternalInformation->IndexNumber.QuadPart = (LONGLONG)Fcb->Ext2Mcb->Inode;
                
                // Romfs doesn't have any extended attributes
                FileEaInformation->EaSize = 0;
                
                FilePositionInformation->CurrentByteOffset =
                    FileObject->CurrentByteOffset;
                
                if (Length < sizeof(FILE_ALL_INFORMATION) +
                    Fcb->Ext2Mcb->ShortName.Length - sizeof(WCHAR))
                {
                    Irp->IoStatus.Information = sizeof(FILE_ALL_INFORMATION);
                    Status = STATUS_BUFFER_OVERFLOW;
                    __leave;
                }
                
                FileNameInformation->FileNameLength = Fcb->Ext2Mcb->ShortName.Length;
                
                RtlCopyMemory(
                    FileNameInformation->FileName,
                    Fcb->Ext2Mcb->ShortName.Buffer,
                    Fcb->Ext2Mcb->ShortName.Length
                    );
                
                Irp->IoStatus.Information = sizeof(FILE_ALL_INFORMATION) +
                    Fcb->Ext2Mcb->ShortName.Length - sizeof(WCHAR);
                Status = STATUS_SUCCESS;
                __leave;
            }
        
        /*
        case FileAlternateNameInformation:
            {
        // TODO: Handle FileAlternateNameInformation
        
          // Here we would like to use RtlGenerate8dot3Name but I don't
          // know how to use the argument PGENERATE_NAME_CONTEXT
          }
          */
          
        case FileNetworkOpenInformation:
        {
            PFILE_NETWORK_OPEN_INFORMATION FileNetworkOpenInformation;
            
            if (Length < sizeof(FILE_NETWORK_OPEN_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                __leave;
            }
            
            FileNetworkOpenInformation =
                (PFILE_NETWORK_OPEN_INFORMATION) Buffer;
            
            FileNetworkOpenInformation->CreationTime = Ext2SysTime(Fcb->ext2_inode->i_ctime);
            
            FileNetworkOpenInformation->LastAccessTime = Ext2SysTime(Fcb->ext2_inode->i_atime);
            
            FileNetworkOpenInformation->LastWriteTime = Ext2SysTime(Fcb->ext2_inode->i_mtime);
            
            FileNetworkOpenInformation->ChangeTime = Ext2SysTime(Fcb->ext2_inode->i_mtime);
            
            FileNetworkOpenInformation->AllocationSize.QuadPart =
                (LONGLONG)(Fcb->ext2_inode->i_size);
            
            FileNetworkOpenInformation->EndOfFile.QuadPart =
                (LONGLONG)(Fcb->ext2_inode->i_size);
            
            FileNetworkOpenInformation->FileAttributes =
                Fcb->Ext2Mcb->FileAttr;
            
            Irp->IoStatus.Information =
                sizeof(FILE_NETWORK_OPEN_INFORMATION);
            Status = STATUS_SUCCESS;
            __leave;
        }
        
        default:
        Status = STATUS_INVALID_INFO_CLASS;
        }
    }
    __finally
    {
        if (FcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
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
                    (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT)
                    );
                
                Ext2FreeIrpContext(IrpContext);
            }
        }
    }
    
    return Status;
}


NTSTATUS
Ext2SetInformation (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB               Vcb;
    PFILE_OBJECT            FileObject;
    PEXT2_FCB               Fcb;
    PEXT2_CCB               Ccb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FILE_INFORMATION_CLASS  FileInformationClass;
    ULONG                   Length;
    PVOID                   Buffer;
    BOOLEAN                 FcbMainResourceAcquired = FALSE;

    BOOLEAN                 VcbResourceAcquired = FALSE;
    BOOLEAN                 FcbPagingIoResourceAcquired = FALSE;


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
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PEXT2_FCB) FileObject->FsContext;
        
        ASSERT(Fcb != NULL);
        
        //
        // This request is not allowed on volumes
        //
        if (Fcb->Identifier.Type == EXT2VCB)
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }
        
        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
            (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED))
        {
            Status = STATUS_FILE_DELETED;
            __leave;
        }

        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        
        ASSERT(Ccb != NULL);
        
        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        FileInformationClass =
            IoStackLocation->Parameters.SetFile.FileInformationClass;
        
        Length = IoStackLocation->Parameters.SetFile.Length;
        
        Buffer = Irp->AssociatedIrp.SystemBuffer;

        if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
        {
            if (FileInformationClass == FileDispositionInformation ||
                FileInformationClass == FileRenameInformation ||
                FileInformationClass == FileLinkInformation)
            {
                if (!ExAcquireResourceExclusiveLite(
                    &Vcb->MainResource,
                    IrpContext->IsSynchronous ))
                {
                    Status = STATUS_PENDING;
                    __leave;
                }
            
                VcbResourceAcquired = TRUE;
            }
        }
        
        if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY) && !FlagOn(Fcb->Flags, FCB_PAGE_FILE))
        {
            if (!ExAcquireResourceExclusiveLite(
                &Fcb->MainResource,
                IrpContext->IsSynchronous ))
            {
                Status = STATUS_PENDING;
                __leave;
            }
            
            FcbMainResourceAcquired = TRUE;
        }
        
        if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
        {
            if (FileInformationClass != FilePositionInformation)
            {
                Status = STATUS_MEDIA_WRITE_PROTECTED;
                __leave;
            }
        }

        if (FileInformationClass == FileDispositionInformation ||
            FileInformationClass == FileRenameInformation ||
            FileInformationClass == FileLinkInformation ||
            FileInformationClass == FileAllocationInformation ||
            FileInformationClass == FileEndOfFileInformation)
        {
            if (!ExAcquireResourceExclusiveLite(
                &Fcb->PagingIoResource,
                IrpContext->IsSynchronous ))
            {
                Status = STATUS_PENDING;
                __leave;
            }
            
            FcbPagingIoResourceAcquired = TRUE;
        }
       
/*        
        if (FileInformationClass != FileDispositionInformation 
            && FlagOn(Fcb->Flags, FCB_DELETE_PENDING))
        {
            Status = STATUS_DELETE_PENDING;
            __leave;
        }
*/
        switch (FileInformationClass)
        {
        case FileBasicInformation:
            {
                PFILE_BASIC_INFORMATION FBI = (PFILE_BASIC_INFORMATION) Buffer;               
                PEXT2_INODE Ext2Inode = Fcb->ext2_inode;

			    if( FBI->CreationTime.QuadPart )
			    {
				    Ext2Inode->i_ctime = (ULONG)(Ext2InodeTime(FBI->CreationTime));
			    }

			    if( FBI->LastAccessTime.QuadPart )
			    {
                    Ext2Inode->i_atime = (ULONG)(Ext2InodeTime(FBI->LastAccessTime));
			    }

			    if( FBI->LastWriteTime.QuadPart )
			    {
                    Ext2Inode->i_mtime = (ULONG)(Ext2InodeTime(FBI->LastWriteTime));
			    }

			    if (IsFlagOn(FBI->FileAttributes, FILE_ATTRIBUTE_READONLY)) 
			    {
                    Ext2SetReadOnly(Fcb->ext2_inode->i_mode);
                    SetFlag(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_READONLY);
			    }
                else
                {
                    Ext2SetWritable(Fcb->ext2_inode->i_mode);
                    ClearFlag(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_READONLY);
                }

			    if(Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, Ext2Inode))
                    Status = STATUS_SUCCESS;

                if (FBI->FileAttributes & FILE_ATTRIBUTE_TEMPORARY) 
                {
                    SetFlag(FileObject->Flags, FO_TEMPORARY_FILE);
                } 
                else 
                {
                    ClearFlag(FileObject->Flags, FO_TEMPORARY_FILE);
                }
                
                Status = STATUS_SUCCESS;
            }
            break;

        case FileAllocationInformation:
            {
                PFILE_ALLOCATION_INFORMATION FAI = (PFILE_ALLOCATION_INFORMATION)Buffer;

                if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
                {
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    __leave;
                }

                if (FAI->AllocationSize.QuadPart == Fcb->CommonFCBHeader.AllocationSize.QuadPart)
                {
                    Status = STATUS_SUCCESS;
                }
                else if (FAI->AllocationSize.QuadPart > Fcb->CommonFCBHeader.AllocationSize.QuadPart)
                {
                    if(Ext2ExpandFileAllocation(IrpContext, Vcb, Fcb, &(FAI->AllocationSize)))
                    {
                        if (Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, Fcb->ext2_inode))
                            Status = STATUS_SUCCESS;
                    }
                    else
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else
                {
                    if (MmCanFileBeTruncated(&(Fcb->SectionObject), &(FAI->AllocationSize))) 
                    {
                        LARGE_INTEGER EndOfFile;

                        EndOfFile.QuadPart = FAI->AllocationSize.QuadPart + (LONGLONG)(Vcb->ext2_block - 1);

			            if(Ext2TruncateFileAllocation(IrpContext, Vcb, Fcb, &(EndOfFile)))
                        {
                            if (FAI->AllocationSize.QuadPart < Fcb->CommonFCBHeader.FileSize.QuadPart)
                                Fcb->CommonFCBHeader.FileSize.QuadPart = FAI->AllocationSize.QuadPart;

                            Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, Fcb->ext2_inode);

				            Status = STATUS_SUCCESS;
                        }
                    }
                    else
			        {
				        Status = STATUS_USER_MAPPED_FILE;
                        __leave;
			        }
                }

                if (NT_SUCCESS(Status))
                {
                    CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->CommonFCBHeader.AllocationSize)));
                    SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                }
                
            }
            break;

        case FileEndOfFileInformation:
            {
                PFILE_END_OF_FILE_INFORMATION FEOFI = (PFILE_END_OF_FILE_INFORMATION) Buffer;

                BOOLEAN CacheInitialized = FALSE;

                if (IsFlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
                {
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    __leave;
                }

                if (FEOFI->EndOfFile.HighPart != 0)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    __leave;
                }


                if (IoStackLocation->Parameters.SetFile.AdvanceOnly)
                {
                    Status = STATUS_SUCCESS;
                    __leave;
                }

                if ((FileObject->SectionObjectPointer->DataSectionObject != NULL) &&
                    (FileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
                    !FlagOn(Irp->Flags, IRP_PAGING_IO)) {

                    ASSERT( !FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) );

                    CcInitializeCacheMap( FileObject,
                                          (PCC_FILE_SIZES)&(Fcb->CommonFCBHeader.AllocationSize),
                                          FALSE,
                                          &(gExt2Global->CacheManagerNoOpCallbacks),
                                          Fcb );

                    CacheInitialized = TRUE;
                }

                if (FEOFI->EndOfFile.QuadPart == Fcb->CommonFCBHeader.AllocationSize.QuadPart)
                {
                    Status = STATUS_SUCCESS;
                }
                else if (FEOFI->EndOfFile.QuadPart > Fcb->CommonFCBHeader.AllocationSize.QuadPart)
                {
                    LARGE_INTEGER   FileSize = Fcb->CommonFCBHeader.FileSize;

                    if(Ext2ExpandFileAllocation(IrpContext, Vcb, Fcb, &(FEOFI->EndOfFile)))
                    {
                        {
                            Fcb->CommonFCBHeader.FileSize.QuadPart = FEOFI->EndOfFile.QuadPart;
                            Fcb->ext2_inode->i_size = (ULONG)FEOFI->EndOfFile.QuadPart;
                            Fcb->CommonFCBHeader.ValidDataLength.QuadPart = (LONGLONG)(0x7fffffffffffffff);
                        }

                        if (Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, Fcb->ext2_inode))
                            Status = STATUS_SUCCESS;
                    }
                    else
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }


                    if (NT_SUCCESS(Status))
                    {
                        CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->CommonFCBHeader.AllocationSize)));
                        SetFlag(FileObject->Flags, FO_FILE_MODIFIED);

                        Ext2ZeroHoles(IrpContext, Vcb, FileObject, FileSize.QuadPart, Fcb->CommonFCBHeader.AllocationSize.QuadPart - FileSize.QuadPart);
                    }
                }
                else
                {
                    if (MmCanFileBeTruncated(&(Fcb->SectionObject), &(FEOFI->EndOfFile))) 
                    {
                        LARGE_INTEGER EndOfFile = FEOFI->EndOfFile;

                        EndOfFile.QuadPart = EndOfFile.QuadPart + (LONGLONG)(Vcb->ext2_block - 1);

			            if(Ext2TruncateFileAllocation(IrpContext, Vcb, Fcb, &(EndOfFile)))
                        {
                            Fcb->CommonFCBHeader.FileSize.QuadPart = FEOFI->EndOfFile.QuadPart;
                            Fcb->ext2_inode->i_size = (ULONG)FEOFI->EndOfFile.QuadPart;

                            Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, Fcb->ext2_inode);

				            Status = STATUS_SUCCESS;
                        }
                    }
                    else
			        {
				        Status = STATUS_USER_MAPPED_FILE;
                        __leave;
			        }

                    if (NT_SUCCESS(Status))
                    {
                        CcSetFileSizes(FileObject, (PCC_FILE_SIZES)(&(Fcb->CommonFCBHeader.AllocationSize)));
                        SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                    }
                }
            }

            break;

        case FileDispositionInformation:
            {
                PFILE_DISPOSITION_INFORMATION FDI = (PFILE_DISPOSITION_INFORMATION)Buffer;
                
                Status = Ext2SetDispositionInfo(IrpContext, Vcb, Fcb, FDI->DeleteFile);
            }

            break;

        case FileRenameInformation:
            {
                Status = Ext2SetRenameInfo(IrpContext, Vcb, Fcb);
            }

            break;

            //
            // This is the only set file information request supported on read
            // only file systems
            //
        case FilePositionInformation:
            {
                PFILE_POSITION_INFORMATION FilePositionInformation;
                
                if (Length < sizeof(FILE_POSITION_INFORMATION))
                {
                    Status = STATUS_INVALID_PARAMETER;
                    __leave;
                }
                
                FilePositionInformation = (PFILE_POSITION_INFORMATION) Buffer;
                
                if ((FlagOn(FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING)) &&
                    (FilePositionInformation->CurrentByteOffset.LowPart &
                    DeviceObject->AlignmentRequirement) )
                {
                    Status = STATUS_INVALID_PARAMETER;
                    __leave;
                }
                
                FileObject->CurrentByteOffset =
                    FilePositionInformation->CurrentByteOffset;
                
                Status = STATUS_SUCCESS;
                __leave;
            }

            break;
            
        default:
            Status = STATUS_INVALID_INFO_CLASS;
        }
    }
    __finally
    {
        
        if (FcbPagingIoResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->PagingIoResource,
                ExGetCurrentResourceThread() );
        }
        
        if (FcbMainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread() );
        }
        
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

BOOLEAN
Ext2ExpandFileAllocation(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb, PEXT2_FCB Fcb, PLARGE_INTEGER AllocationSize)
{
    ULONG   dwRet = 0;
    BOOLEAN bRet = TRUE;

    if (AllocationSize->QuadPart <= Fcb->CommonFCBHeader.AllocationSize.QuadPart)
        return TRUE;

    if (((LONGLONG)Vcb->ext2_super_block->s_free_blocks_count) * Vcb->ext2_block <= (AllocationSize->QuadPart - Fcb->CommonFCBHeader.AllocationSize.QuadPart))
    {
        Ext2DbgPrint(D_FILEINFO, "Ext2ExpandFileAllocation: There is no enough disk space available.\n");
        return FALSE;
    }

    while (bRet && (AllocationSize->QuadPart > Fcb->CommonFCBHeader.AllocationSize.QuadPart))
    {
        bRet = Ext2ExpandInode(IrpContext, Vcb, Fcb, &dwRet);
    }
    
    return bRet;
}

BOOLEAN
Ext2TruncateFileAllocation(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb, PEXT2_FCB Fcb, PLARGE_INTEGER AllocationSize)
{
    BOOLEAN bRet = TRUE;
    
    while (bRet && (AllocationSize->QuadPart < Fcb->CommonFCBHeader.AllocationSize.QuadPart))
    {
        bRet = Ext2TruncateInode(IrpContext, Vcb, Fcb);
    }

    return bRet;
}

NTSTATUS
Ext2SetDispositionInfo(
            PEXT2_IRP_CONTEXT IrpContext,
            PEXT2_VCB Vcb,
            PEXT2_FCB Fcb,
            BOOLEAN bDelete)
{
    PIRP    Irp = IrpContext->Irp;
    PIO_STACK_LOCATION IrpSp;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Ext2DbgPrint(D_FILEINFO, "Ext2SetDispositionInfo: bDelete=%x\n", bDelete);
    
    if (bDelete)
    {
        if (!MmFlushImageSection( &Fcb->SectionObject,
                                  MmFlushForDelete ))
        {
            return STATUS_CANNOT_DELETE;
        }

        if (Fcb->Ext2Mcb->Inode == EXT2_ROOT_INO)
        {
            return STATUS_CANNOT_DELETE;
        }

        if (IsFlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        {
            if (!Ext2IsDirectoryEmpty(Vcb, Fcb))
            {
                return STATUS_DIRECTORY_NOT_EMPTY;
            }
        }

        SetFlag(Fcb->Flags, FCB_DELETE_PENDING);
        IrpSp->FileObject->DeletePending = TRUE;

        if (IsFlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        {
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
    }
    else
    {
        ClearFlag(Fcb->Flags, FCB_DELETE_PENDING);
        IrpSp->FileObject->DeletePending = FALSE;
    }

    return STATUS_SUCCESS;
}  

NTSTATUS
Ext2SetRenameInfo(
            PEXT2_IRP_CONTEXT IrpContext,
            PEXT2_VCB Vcb,
            PEXT2_FCB Fcb)
{
    PEXT2_FCB               TargetDcb;
    PEXT2_MCB               Mcb;

    PEXT2_MCB               TargetMcb;
    EXT2_INODE              TargetIno;

    UNICODE_STRING          FileName;
    
    NTSTATUS                Status;

    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;

    PFILE_OBJECT            FileObject;
    PFILE_OBJECT            TargetObject;
    BOOLEAN                 ReplaceIfExists;

    PFILE_RENAME_INFORMATION    FRI;

    if (Fcb->Ext2Mcb->Inode == EXT2_ROOT_INO)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto errorout;
    }

    Irp = IrpContext->Irp;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    FileObject = IrpSp->FileObject;
    TargetObject = IrpSp->Parameters.SetFile.FileObject;
    ReplaceIfExists = IrpSp->Parameters.SetFile.ReplaceIfExists;

    FRI = (PFILE_RENAME_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

    if (TargetObject == NULL)
    {
        UNICODE_STRING  NewName;

        NewName.Buffer = FRI->FileName;
        NewName.MaximumLength = NewName.Length = (USHORT)FRI->FileNameLength;

        while (NewName.Length > 0 && NewName.Buffer[NewName.Length/2 - 1] == L'\\')
        {
            NewName.Buffer[NewName.Length/2 - 1] = 0;
            NewName.Length -= 2;
        }

        while (NewName.Length > 0 && NewName.Buffer[NewName.Length/2 - 1] != L'\\')
        {
            NewName.Length -= 2;
        }

        NewName.Buffer = (USHORT *)((UCHAR *)NewName.Buffer + NewName.Length);
        NewName.Length = (USHORT)(FRI->FileNameLength - NewName.Length);

        FileName = NewName;

        TargetDcb = NULL;
        Mcb = Fcb->Ext2Mcb->Parent;
        
        if (FileName.Length >= EXT2_NAME_LEN*sizeof(USHORT))
        {
            Status = STATUS_OBJECT_NAME_INVALID;
            goto errorout;
        }
    }
    else
    {
        TargetDcb = (PEXT2_FCB)(TargetObject->FsContext);
        if (!TargetDcb || TargetDcb->Vcb != Vcb)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto errorout;
        }

        Mcb = TargetDcb->Ext2Mcb;

        FileName = TargetObject->FileName;
    }

    if (Mcb->Inode == Fcb->Ext2Mcb->Parent->Inode)
    {
        if (FsRtlAreNamesEqual( &FileName,
                                &(Fcb->Ext2Mcb->ShortName),
                                FALSE,
                                NULL ))
        {
            Status = STATUS_SUCCESS;
            goto errorout;
        }
    }

    TargetDcb = Mcb->Ext2Fcb;

    if (!TargetDcb)
        TargetDcb = Ext2CreateFcbFromMcb(Vcb, Mcb);

    if (Mcb->Inode != Fcb->Ext2Mcb->Parent->Inode)
        Ext2CreateFcbFromMcb(Vcb, Fcb->Ext2Mcb->Parent);

    if (!TargetDcb || !(Fcb->Ext2Mcb->Parent->Ext2Fcb))
    {
        Status = STATUS_UNSUCCESSFUL;

        goto errorout;
    }

    TargetMcb = NULL;
    Status = Ext2LookupFileName(
                Vcb,
                &FileName,
                TargetDcb,
                &TargetMcb,
                &TargetIno ); 

    if (NT_SUCCESS(Status))   
    {
        if ( (!ReplaceIfExists) ||
             (IsFlagOn(TargetMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)) ||
             (IsFlagOn(TargetMcb->FileAttr, FILE_ATTRIBUTE_READONLY)))
        {
            Status = STATUS_OBJECT_NAME_COLLISION;
            goto errorout;
        }

        if (ReplaceIfExists)
        {
            Status = STATUS_NOT_IMPLEMENTED;
            goto errorout;
        }
    }

    Status = Ext2RemoveEntry(IrpContext, Vcb, Fcb->Ext2Mcb->Parent->Ext2Fcb, Fcb->Ext2Mcb->Inode);
 
    if (IsFlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        Status = Ext2AddEntry(IrpContext, Vcb, TargetDcb, EXT2_FT_DIR, Fcb->Ext2Mcb->Inode, &FileName);
    else
        Status = Ext2AddEntry(IrpContext, Vcb, TargetDcb, EXT2_FT_REG_FILE, Fcb->Ext2Mcb->Inode, &FileName);

    if (NT_SUCCESS(Status))
    {
        if (Fcb->Ext2Mcb->ShortName.MaximumLength < (FileName.Length + 2))
        {
            ExFreePool(Fcb->Ext2Mcb->ShortName.Buffer);
            Fcb->Ext2Mcb->ShortName.Buffer = 
                ExAllocatePool(PagedPool, FileName.Length + 2);

            if (!Fcb->Ext2Mcb->ShortName.Buffer)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto errorout;
            }

            Fcb->Ext2Mcb->ShortName.MaximumLength = FileName.Length + 2;
        }

        {
            RtlCopyMemory(Fcb->Ext2Mcb->ShortName.Buffer, FileName.Buffer, FileName.Length);
            Fcb->Ext2Mcb->ShortName.Length = FileName.Length;
        }
    
#if DBG    
        if (Fcb->AnsiFileName.Length < (FileName.Length / 2 + 1))
        {
            ExFreePool(Fcb->AnsiFileName.Buffer);
            Fcb->AnsiFileName.Buffer = 
                ExAllocatePool(PagedPool, FileName.Length / 2 + 1);

            if (!Fcb->AnsiFileName.Buffer)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto errorout;
            }

            Fcb->AnsiFileName.MaximumLength = FileName.Length / 2 + 1;
        }

        Fcb->AnsiFileName.Length = FileName.Length / 2;

        Ext2WcharToChar(
            Fcb->AnsiFileName.Buffer,
            FileName.Buffer,
            FileName.Length / 2 );

        Fcb->AnsiFileName.Buffer[FileName.Length / sizeof(WCHAR)] = 0;

#endif

        Ext2DeleteMcbNode(Fcb->Ext2Mcb->Parent, Fcb->Ext2Mcb);
        Ext2AddMcbNode(Mcb, Fcb->Ext2Mcb);
    }

errorout:

    return Status;
}
