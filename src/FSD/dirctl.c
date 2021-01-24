/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             dirctl.c
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
#pragma alloc_text(PAGE, Ext2GetInfoLength)
#pragma alloc_text(PAGE, Ext2ProcessDirEntry)
#pragma alloc_text(PAGE, Ext2QueryDirectory)
#pragma alloc_text(PAGE, Ext2NotifyChangeDirectory)
#pragma alloc_text(PAGE, Ext2DirectoryControl)
#endif

ULONG
Ext2GetInfoLength(IN FILE_INFORMATION_CLASS  FileInformationClass)
{
    switch (FileInformationClass)
    {
    case FileDirectoryInformation:
        return sizeof(FILE_DIRECTORY_INFORMATION);
        break;
        
    case FileFullDirectoryInformation:
        return sizeof(FILE_FULL_DIR_INFORMATION);
        break;
        
    case FileBothDirectoryInformation:
        return sizeof(FILE_BOTH_DIR_INFORMATION);
        break;
        
    case FileNamesInformation:
        return sizeof(FILE_NAMES_INFORMATION);
        break;
        
    default:
        break;
    }

    return 0;
}

/*
#define FillInfo (FI, BSize, Inode, Index, NSize, pName, Single) {\
    if (!Single) \
        FI->NextEntryOffset = BSize + NSize - sizeof(WCHAR); \
    else \
            FI->NextEntryOffset = 0; \
    FI->FileIndex = Index; \
    FI->CreationTime.QuadPart = Inode.i_ctime; \
    FI->LastAccessTime.QuadPart = Inode.i_atime; \
    FI->LastWriteTime.QuadPart = Inode.i_mtime; \
    FI->ChangeTime.QuadPart = Inode.i_mtime; \
    FI->EndOfFile.QuadPart = Inode.i_size; \
    FI->AllocationSize.QuadPart = Inode.i_size; \
    FI->LastAccessTime.QuadPart = Inode.i_atime; \
    FI->FileAttributes = FILE_ATTRIBUTE_NORMAL; \
    if (S_ISDIR(Inode->i_mode)) \
        FI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY; \
    FI->FileNameLength = NSize; \
    RtlCopyMemory(FI->FileName, pName->Buffer, NSize); \
    dwBytes = BSize + NSize - sizeof(WCHAR); }
*/

ULONG
Ext2ProcessDirEntry(IN PEXT2_VCB         Vcb,
            IN FILE_INFORMATION_CLASS  FileInformationClass,
            IN ULONG         in,
            IN PVOID         Buffer,
            IN ULONG         UsedLength,
            IN ULONG         Length,
            IN ULONG         FileIndex,
            IN PUNICODE_STRING   pName,
            IN BOOLEAN       Single )
{
    EXT2_INODE inode;
    PFILE_DIRECTORY_INFORMATION FDI;
    PFILE_FULL_DIR_INFORMATION FFI;
    PFILE_BOTH_DIR_INFORMATION FBI;
    PFILE_NAMES_INFORMATION FNI;

    ULONG InfoLength = 0;
    ULONG NameLength = 0;
    ULONG dwBytes = 0;

    NameLength = pName->Length;

    if (!in)
    {
        Ext2DbgPrint(D_DIRCTL, "Ext2PricessDirEntry: ext2_dir_entry is empty.\n");
        return 0;
    }

    InfoLength = Ext2GetInfoLength(FileInformationClass);
    if (!InfoLength || InfoLength + NameLength - sizeof(WCHAR)> Length)
    {
        Ext2DbgPrint(D_DIRCTL, "Ext2PricessDirEntry: Size/Length error.\n");
        return 0;
    }

    if(!Ext2LoadInode(Vcb, in, &inode))
    {
        Ext2DbgPrint(D_DIRCTL, "Ext2PricessDirEntry: Loading inode %xh error.\n", in);
        return 0;
    }

    switch(FileInformationClass)
    {
    case FileDirectoryInformation:
        FDI = (PFILE_DIRECTORY_INFORMATION) ((PUCHAR)Buffer + UsedLength);
        if (!Single)
            FDI->NextEntryOffset = InfoLength + NameLength - sizeof(WCHAR);
        else
            FDI->NextEntryOffset = 0;
        FDI->FileIndex = FileIndex;
        FDI->CreationTime = Ext2SysTime(inode.i_ctime);
        FDI->LastAccessTime = Ext2SysTime(inode.i_atime);
        FDI->LastWriteTime = Ext2SysTime(inode.i_mtime);
        FDI->ChangeTime = Ext2SysTime(inode.i_mtime);
        FDI->EndOfFile.QuadPart = inode.i_size;
        FDI->AllocationSize.QuadPart = inode.i_size;
        FDI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

        if (FlagOn(Vcb->Flags, VCB_READ_ONLY) || Ext2IsReadOnly(inode.i_mode))
        {
            SetFlag(FDI->FileAttributes, FILE_ATTRIBUTE_READONLY);
        }

        if (S_ISDIR(inode.i_mode))
            FDI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

        FDI->FileNameLength = NameLength;
        RtlCopyMemory(FDI->FileName, pName->Buffer, NameLength);
        dwBytes = InfoLength + NameLength - sizeof(WCHAR); 
        break;
        
    case FileFullDirectoryInformation:
        FFI = (PFILE_FULL_DIR_INFORMATION) ((PUCHAR)Buffer + UsedLength);
//      FillInfo (FFI, InfoLength, inode, FileIndex, NameLength, pName, Single)
        if (!Single)
            FFI->NextEntryOffset = InfoLength + NameLength - sizeof(WCHAR);
        else
            FFI->NextEntryOffset = 0;
        FFI->FileIndex = FileIndex;
        FFI->CreationTime = Ext2SysTime(inode.i_ctime);
        FFI->LastAccessTime = Ext2SysTime(inode.i_atime);
        FFI->LastWriteTime = Ext2SysTime(inode.i_mtime);
        FFI->ChangeTime = Ext2SysTime(inode.i_mtime);
        FFI->EndOfFile.QuadPart = inode.i_size;
        FFI->AllocationSize.QuadPart = inode.i_size;
        FFI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

        if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)  || Ext2IsReadOnly(inode.i_mode))
        {
            SetFlag(FFI->FileAttributes, FILE_ATTRIBUTE_READONLY);
        }

        if (S_ISDIR(inode.i_mode))
            FFI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

        FFI->FileNameLength = NameLength;
        RtlCopyMemory(FFI->FileName, pName->Buffer, NameLength);
        dwBytes = InfoLength + NameLength - sizeof(WCHAR); 

        break;
        
    case FileBothDirectoryInformation:
        FBI = (PFILE_BOTH_DIR_INFORMATION) ((PUCHAR)Buffer + UsedLength);
//      FillInfo (FBI, InfoLength, inode, FileIndex, NameLength, pName, Single)
        if (!Single)
            FBI->NextEntryOffset = InfoLength + NameLength - sizeof(WCHAR);
        else
            FBI->NextEntryOffset = 0;
        FBI->CreationTime = Ext2SysTime(inode.i_ctime);
        FBI->LastAccessTime = Ext2SysTime(inode.i_atime);
        FBI->LastWriteTime = Ext2SysTime(inode.i_mtime);
        FBI->ChangeTime = Ext2SysTime(inode.i_mtime);

/*
        FBI->CreationTime.QuadPart = inode.i_ctime;
        FBI->LastAccessTime.QuadPart = inode.i_atime;
        FBI->LastWriteTime.QuadPart = inode.i_mtime;
        FBI->ChangeTime.QuadPart = inode.i_mtime;
*/
        FBI->EndOfFile.QuadPart = inode.i_size;
        FBI->AllocationSize.QuadPart = inode.i_size;
        FBI->FileAttributes = FILE_ATTRIBUTE_NORMAL;

        if (FlagOn(Vcb->Flags, VCB_READ_ONLY)  || Ext2IsReadOnly(inode.i_mode))
        {
            SetFlag(FBI->FileAttributes, FILE_ATTRIBUTE_READONLY);
        }

        if (S_ISDIR(inode.i_mode))
            FBI->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        FBI->FileNameLength = NameLength;
        RtlCopyMemory(FBI->FileName, pName->Buffer, NameLength);
        dwBytes = InfoLength + NameLength - sizeof(WCHAR); 

        break;
        
    case FileNamesInformation:
        FNI = (PFILE_NAMES_INFORMATION) ((PUCHAR)Buffer + UsedLength);
        if (!Single)
            FNI->NextEntryOffset = InfoLength + NameLength - sizeof(WCHAR);
        else
            FNI->NextEntryOffset = 0;
        FNI->FileNameLength = NameLength;
        RtlCopyMemory(FNI->FileName, pName->Buffer, NameLength);
        dwBytes = InfoLength + NameLength - sizeof(WCHAR); 

        break;
        
    default:
        break;
    }

    return dwBytes;
}


NTSTATUS
Ext2QueryDirectory (IN PEXT2_IRP_CONTEXT IrpContext)
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
    PUNICODE_STRING         FileName;
    ULONG                   FileIndex;
    BOOLEAN                 RestartScan;
    BOOLEAN                 ReturnSingleEntry;
    BOOLEAN                 IndexSpecified;
    PUCHAR                  Buffer;
    BOOLEAN                 FirstQuery;
    PEXT2_INODE             Inode = NULL;
    BOOLEAN                 FcbResourceAcquired = FALSE;
    ULONG                   UsedLength = 0;
    USHORT                  InodeFileNameLength;
    UNICODE_STRING          InodeFileName;
    PEXT2_DIR_ENTRY2        pDir = NULL;
    ULONG                   dwBytes;
    ULONG                   dwTemp = 0;
    ULONG                   dwSize = 0;
    ULONG                   dwReturn = 0;
    BOOLEAN                 bRun = TRUE;
    ULONG                   ByteOffset;

    InodeFileName.Buffer = NULL;
    
    __try
    {
        ASSERT(IrpContext);
        
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
        
        ASSERT(Fcb);
        
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
        
        if (!FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }
        
        Ccb = (PEXT2_CCB) FileObject->FsContext2;
        
        ASSERT(Ccb);
        
        ASSERT((Ccb->Identifier.Type == EXT2CCB) &&
            (Ccb->Identifier.Size == sizeof(EXT2_CCB)));
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
#ifndef _GNU_NTIFS_
        
        FileInformationClass =
            IoStackLocation->Parameters.QueryDirectory.FileInformationClass;
        
        Length = IoStackLocation->Parameters.QueryDirectory.Length;
        
        FileName = IoStackLocation->Parameters.QueryDirectory.FileName;
        
        FileIndex = IoStackLocation->Parameters.QueryDirectory.FileIndex;
        
#else // _GNU_NTIFS_
        
        FileInformationClass = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.FileInformationClass;
        
        Length = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.Length;
        
        FileName = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.FileName;
        
        FileIndex = ((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Parameters.QueryDirectory.FileIndex;
        
#endif // _GNU_NTIFS_
        
        RestartScan = FlagOn(((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Flags, SL_RESTART_SCAN);
        ReturnSingleEntry = FlagOn(((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Flags, SL_RETURN_SINGLE_ENTRY);
        IndexSpecified = FlagOn(((PEXTENDED_IO_STACK_LOCATION)
            IoStackLocation)->Flags, SL_INDEX_SPECIFIED);
/*
        if (!Irp->MdlAddress && Irp->UserBuffer)
        {
            ProbeForWrite(Irp->UserBuffer, Length, 1);
        }
*/
        Buffer = Ext2GetUserBuffer(Irp);

        if (Buffer == NULL)
        {
            Status = STATUS_INVALID_USER_BUFFER;
            __leave;
        }
        
        if (!IrpContext->IsSynchronous)
        {
            Status = STATUS_PENDING;
            __leave;
        }
        
        if (!ExAcquireResourceSharedLite(
                 &Fcb->MainResource,
                 IrpContext->IsSynchronous
                 ))
        {
            Status = STATUS_PENDING;
            __leave;
        }

        FcbResourceAcquired = TRUE;
        
        if (FileName != NULL)
        {
    
            if (Ccb->DirectorySearchPattern.Buffer != NULL)
            {
                FirstQuery = FALSE;
            }
            else
            {
                FirstQuery = TRUE;
                
                Ccb->DirectorySearchPattern.Length =
                    Ccb->DirectorySearchPattern.MaximumLength =
                    FileName->Length;
                
                Ccb->DirectorySearchPattern.Buffer =
                    ExAllocatePool(PagedPool, FileName->Length);
                
                if (Ccb->DirectorySearchPattern.Buffer == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    __leave;
                }

                Status = RtlUpcaseUnicodeString(
                    &(Ccb->DirectorySearchPattern),
                    FileName,
                    FALSE);

                if (!NT_SUCCESS(Status))
                    __leave;
            }
        }
        else if (Ccb->DirectorySearchPattern.Buffer != NULL)
        {
            FirstQuery = FALSE;
            FileName = &Ccb->DirectorySearchPattern;
        }
        else
        {
            FirstQuery = TRUE;
            
            Ccb->DirectorySearchPattern.Length =
                Ccb->DirectorySearchPattern.MaximumLength = 2;
            
            Ccb->DirectorySearchPattern.Buffer =
                ExAllocatePool(PagedPool, 2);
            
            if (Ccb->DirectorySearchPattern.Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }
            
            RtlCopyMemory(
                Ccb->DirectorySearchPattern.Buffer,
                L"*\0", 2);
        }
        
        if (!IndexSpecified)
        {
            if (RestartScan || FirstQuery)
            {
                FileIndex = Fcb->Ext2Mcb->DeOffset = 0;
            }
            else
            {
                FileIndex = Ccb->CurrentByteOffset;
            }
        }
        
        Inode = (PEXT2_INODE) ExAllocatePool(
            PagedPool,
            sizeof(EXT2_INODE));
        
        if (Inode == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }
        
        RtlZeroMemory(Buffer, Length);

        if (Fcb->ext2_inode->i_size <= FileIndex)
        {
            Status = STATUS_NO_MORE_FILES;
            __leave;
        }
        
        pDir = ExAllocatePool(PagedPool,
                sizeof(EXT2_DIR_ENTRY2));
        if (!pDir)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }
        
        dwBytes = 0;
        dwSize = Fcb->ext2_inode->i_size - FileIndex - (sizeof(EXT2_DIR_ENTRY2) - EXT2_NAME_LEN + 1);

        ByteOffset = FileIndex;

        dwTemp = 0;
        
        while (bRun && UsedLength < Length  && dwBytes < dwSize)
        {
            RtlZeroMemory(pDir, sizeof(EXT2_DIR_ENTRY2));

            Status = Ext2ReadInode(
                        NULL,
                        Vcb,
                        Fcb->ext2_inode,
                        ByteOffset,
                        (PVOID)pDir,
                        sizeof(EXT2_DIR_ENTRY2),
                        &dwReturn);

            if (!NT_SUCCESS(Status))
            {
                __leave;
            }

            if (!pDir->inode)
            {
                bRun = FALSE;
                break;
            }

            InodeFileNameLength = pDir->name_len & 0xff;
            
            InodeFileName.Length = InodeFileName.MaximumLength =
                InodeFileNameLength * 2;
            
            if (InodeFileName.Length <= 0)
                break;

            InodeFileName.Buffer = ExAllocatePool(
                PagedPool,
                InodeFileNameLength * 2 + 2);

            if (!InodeFileName.Buffer)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            RtlZeroMemory(InodeFileName.Buffer, InodeFileNameLength * 2 + 2);
            
            Ext2CharToWchar(
                InodeFileName.Buffer,
                pDir->name,
                InodeFileNameLength );

            if (FsRtlDoesNameContainWildCards(&(Ccb->DirectorySearchPattern)) ?
                FsRtlIsNameInExpression(
                    &(Ccb->DirectorySearchPattern),
                    &InodeFileName,
                    TRUE,
                    NULL) :
                !RtlCompareUnicodeString(
                    &(Ccb->DirectorySearchPattern),
                    &InodeFileName,
                    TRUE)           )
            {
                dwReturn = Ext2ProcessDirEntry(
                    Vcb, FileInformationClass,
                    pDir->inode, Buffer, UsedLength, Length - UsedLength,
                    dwBytes, &InodeFileName,
                    ReturnSingleEntry );

                if (dwReturn <= 0)
                {
                    bRun = FALSE;
                }
                else
                {
                    dwTemp = UsedLength;
                    UsedLength += dwReturn;
                }
            }
            
            if (InodeFileName.Buffer != NULL)
            {
                ExFreePool(InodeFileName.Buffer);
                InodeFileName.Buffer = NULL;
            }
            
            if (bRun)
            {
                dwBytes +=pDir->rec_len;
                Ccb->CurrentByteOffset = FileIndex + dwBytes;
            }

            if (UsedLength && ReturnSingleEntry)
            {
                Status = STATUS_SUCCESS;
                __leave;
            }

            ByteOffset = FileIndex + dwBytes;
        }

        FileIndex += dwBytes;

        ((PULONG)((PUCHAR)Buffer + dwTemp)) [0] = 0;

        if (!UsedLength)
        {
            if (FirstQuery)
            {
                Status = STATUS_NO_SUCH_FILE;
            }
            else
            {
                Status = STATUS_NO_MORE_FILES;
            }
        }
        else
        {
            Status = STATUS_SUCCESS;
        }
    }

    __finally
    {
    
        if (FcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread() );
        }
        
        if (Inode != NULL)
        {
            ExFreePool(Inode);
        }
        
        if (pDir != NULL)
        {
            ExFreePool(pDir);
            pDir = NULL;
        }
        
        if (InodeFileName.Buffer != NULL)
        {
            ExFreePool(InodeFileName.Buffer);
        }
        
        if (!IrpContext->ExceptionInProgress)
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
                IrpContext->Irp->IoStatus.Information = UsedLength;
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
Ext2NotifyChangeDirectory (
    IN PEXT2_IRP_CONTEXT IrpContext
    )
{
    PDEVICE_OBJECT      DeviceObject;
    BOOLEAN             CompleteRequest;
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB           Vcb;
    PFILE_OBJECT        FileObject;
    PEXT2_FCB           Fcb;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    ULONG               CompletionFilter;
    BOOLEAN             WatchTree;

    __try
    {
        ASSERT(IrpContext);

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        //
        //  Always set the wait flag in the Irp context for the original request.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

        DeviceObject = IrpContext->DeviceObject;

        if (DeviceObject == gExt2Global->DeviceObject)
        {
            CompleteRequest = TRUE;
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;

        ASSERT(Vcb != NULL);

        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        FileObject = IrpContext->FileObject;

        Fcb = (PEXT2_FCB) FileObject->FsContext;

        ASSERT(Fcb);

        if (Fcb->Identifier.Type == EXT2VCB)
        {
            CompleteRequest = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

        if (!FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
        {
            CompleteRequest = TRUE;
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        Irp = IrpContext->Irp;

        IrpSp = IoGetCurrentIrpStackLocation(Irp);

#ifndef _GNU_NTIFS_

        CompletionFilter =
            IrpSp->Parameters.NotifyDirectory.CompletionFilter;

#else // _GNU_NTIFS_

        CompletionFilter = ((PEXTENDED_IO_STACK_LOCATION)
            IrpSp)->Parameters.NotifyDirectory.CompletionFilter;

#endif // _GNU_NTIFS_

        WatchTree = IsFlagOn(IrpSp->Flags, SL_WATCH_TREE);

        CompleteRequest = FALSE;

        Status = STATUS_PENDING;

        {
            UNICODE_STRING FullFileName;

            if (Ext2GetFullFileName(Fcb->Ext2Mcb, &FullFileName))
            {
                FsRtlNotifyFullChangeDirectory(Vcb->NotifySync,
                                        &Vcb->NotifyList,
                                        FileObject->FsContext2,
                                        (PSTRING)&FullFileName,
                                        WatchTree,
                                        FALSE,
                                        CompletionFilter,
                                        Irp,
                                        NULL,
                                        NULL );


                ExFreePool(FullFileName.Buffer);
            }
        }

/*
    Currently the driver is read-only but here is an example on how to use the
    FsRtl-functions to report a change:

    ANSI_STRING TestString;
    USHORT      FileNamePartLength;

    RtlInitAnsiString(&TestString, "\\ntifs.h");

    FileNamePartLength = 7;

    FsRtlNotifyReportChange(
        Vcb->NotifySync,            // PNOTIFY_SYNC NotifySync
        &Vcb->NotifyList,           // PLIST_ENTRY  NotifyList
        &TestString,                // PSTRING      FullTargetName
        &FileNamePartLength,        // PUSHORT      FileNamePartLength
        FILE_NOTIFY_CHANGE_NAME     // ULONG        FilterMatch
        );

    or

    ANSI_STRING TestString;

    RtlInitAnsiString(&TestString, "\\ntifs.h");

    FsRtlNotifyFullReportChange(
        Vcb->NotifySync,            // PNOTIFY_SYNC NotifySync
        &Vcb->NotifyList,           // PLIST_ENTRY  NotifyList
        &TestString,                // PSTRING      FullTargetName
        1,                          // USHORT       TargetNameOffset
        NULL,                       // PSTRING      StreamName OPTIONAL
        NULL,                       // PSTRING      NormalizedParentName OPTIONAL
        FILE_NOTIFY_CHANGE_NAME,    // ULONG        FilterMatch
        0,                          // ULONG        Action
        NULL                        // PVOID        TargetContext
        );
*/

    }
    __finally
    {
        if (!IrpContext->ExceptionInProgress)
        {
            if (CompleteRequest)
            {
                IrpContext->Irp->IoStatus.Status = Status;

                Ext2CompleteRequest(
                    IrpContext->Irp,
                    (CCHAR)
                    (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT)
                    );
            }

            Ext2FreeIrpContext(IrpContext);
        }
    }

    return Status;
}



NTSTATUS
Ext2DirectoryControl (IN PEXT2_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
    
    switch (IrpContext->MinorFunction)
    {
    case IRP_MN_QUERY_DIRECTORY:
        Status = Ext2QueryDirectory(IrpContext);
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
        Status = Ext2NotifyChangeDirectory(IrpContext);
        break;
        
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        IrpContext->Irp->IoStatus.Status = Status;
        Ext2CompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        Ext2FreeIrpContext(IrpContext);
    }
    
    return Status;
}
