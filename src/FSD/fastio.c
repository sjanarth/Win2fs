/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             fastio.c
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
#if DBG
#pragma alloc_text(PAGE, Ext2FastIoRead)
#pragma alloc_text(PAGE, Ext2FastIoWrite)
#endif
#pragma alloc_text(PAGE, Ext2FastIoCheckIfPossible)
#pragma alloc_text(PAGE, Ext2FastIoQueryBasicInfo)
#pragma alloc_text(PAGE, Ext2FastIoQueryStandardInfo)
#pragma alloc_text(PAGE, Ext2FastIoQueryNetworkOpenInfo)
#pragma alloc_text(PAGE, Ext2FastIoLock)
#pragma alloc_text(PAGE, Ext2FastIoUnlockSingle)
#pragma alloc_text(PAGE, Ext2FastIoUnlockAll)
#pragma alloc_text(PAGE, Ext2FastIoUnlockAll)
#endif

BOOLEAN
Ext2FastIoCheckIfPossible (
              IN PFILE_OBJECT         FileObject,
              IN PLARGE_INTEGER       FileOffset,
              IN ULONG                Length,
              IN BOOLEAN              Wait,
              IN ULONG                LockKey,
              IN BOOLEAN              CheckForReadOperation,
              OUT PIO_STATUS_BLOCK    IoStatus,
              IN PDEVICE_OBJECT       DeviceObject
              )
{
    BOOLEAN          Status = FALSE;
    PEXT2_FCB        Fcb;
    LARGE_INTEGER    lLength;
    
    lLength.QuadPart = Length;
    
    __try
    {
        __try
        {
            if (DeviceObject == gExt2Global->DeviceObject)
            {
                Status = FALSE;
                __leave;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == EXT2VCB)
            {
                Status = FALSE;
                __leave;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
            
            if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
            {
                Status = FALSE;
                __leave;
            }
            
            FsRtlEnterFileSystem();
            
            if (CheckForReadOperation)
            {
                Status = FsRtlFastCheckLockForRead(
                    &Fcb->FileLockAnchor,
                    FileOffset,
                    &lLength,
                    LockKey,
                    FileObject,
                    PsGetCurrentProcess());
            }
            else
            {
                if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
                {
                    Status = FALSE;
                }
                else
                {
                    Status = FsRtlFastCheckLockForWrite(
                        &Fcb->FileLockAnchor,
                        FileOffset,
                        &lLength,
                        LockKey,
                        FileObject,
                        PsGetCurrentProcess());
                }
            }
 #if DBG
            Ext2DbgPrint(D_FASTIO,"Ext2FastIIOCheckPossible: %-16.16s %-31s %s\n",
                Ext2DbgGetCurrentProcessName(),
                "FASTIO_CHECK_IF_POSSIBLE",
                Fcb->AnsiFileName.Buffer
                );
 #endif
            Ext2DbgPrint(D_FASTIO,"Ext2FastIIOCheckPossible: Offset: %I64u Length: %u Key: %u %s %s\n",
                FileOffset->QuadPart,
                Length,
                LockKey,
                (CheckForReadOperation ? "CheckForReadOperation:" : "CheckForWriteOperation:"),
                (Status ? "Succeeded" : "Failed"));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            Status = FALSE;
        }
    }
    __finally
    {
        FsRtlExitFileSystem();
    }
    
    return Status;
}


#if DBG
BOOLEAN
Ext2FastIoRead (IN PFILE_OBJECT         FileObject,
           IN PLARGE_INTEGER       FileOffset,
           IN ULONG                Length,
           IN BOOLEAN              Wait,
           IN ULONG                LockKey,
           OUT PVOID               Buffer,
           OUT PIO_STATUS_BLOCK    IoStatus,
           IN PDEVICE_OBJECT       DeviceObject)
{
    BOOLEAN     Status;
    PEXT2_FCB    Fcb;
    
    Fcb = (PEXT2_FCB) FileObject->FsContext;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
        (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    Ext2DbgPrint(D_FASTIO,"Ext2FastIoRead: %-16.16s %-31s %s\n",
        Ext2DbgGetCurrentProcessName(),
        "FASTIO_READ",
        Fcb->AnsiFileName.Buffer     );

    Ext2DbgPrint(D_FASTIO,"Ext2FastIoRead: Offset: %I64u Length: %u Key: %u\n",
        FileOffset->QuadPart,
        Length,
        LockKey       );

    Status = FsRtlCopyRead (
        FileObject, FileOffset, Length, Wait,
        LockKey, Buffer, IoStatus, DeviceObject);
    
    return Status;
}

BOOLEAN
Ext2FastIoWrite (
           IN PFILE_OBJECT         FileObject,
           IN PLARGE_INTEGER       FileOffset,
           IN ULONG                Length,
           IN BOOLEAN              Wait,
           IN ULONG                LockKey,
           OUT PVOID               Buffer,
           OUT PIO_STATUS_BLOCK    IoStatus,
           IN PDEVICE_OBJECT       DeviceObject)
{
    BOOLEAN     Status;
    PEXT2_FCB    Fcb;
    
    Fcb = (PEXT2_FCB) FileObject->FsContext;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
        (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

    Ext2DbgPrint(D_FASTIO, "Ext2FastIoWrite: %-16.16s %-31s %s\n",
        Ext2DbgGetCurrentProcessName(),
        "FASTIO_WRITE",
        Fcb->AnsiFileName.Buffer     );

    Ext2DbgPrint(D_FASTIO, "Ext2FastIoWrite: Offset: %I64xh Length: %xh Key: %xh\n",
        FileOffset->QuadPart,
        Length,
        LockKey       );

    if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
    {
        return FALSE;
    }

    Status = FsRtlCopyWrite (
        FileObject, FileOffset, Length, Wait,
        LockKey, Buffer, IoStatus, DeviceObject);
    
    return Status;
}

#endif /* DBG */


BOOLEAN
Ext2FastIoQueryBasicInfo (IN PFILE_OBJECT             FileObject,
              IN BOOLEAN                  Wait,
              OUT PFILE_BASIC_INFORMATION Buffer,
              OUT PIO_STATUS_BLOCK        IoStatus,
              IN PDEVICE_OBJECT           DeviceObject)
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;
    BOOLEAN     FcbMainResourceAcquired = FALSE;
    
    __try
    {
        __try
        {
            if (DeviceObject == gExt2Global->DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == EXT2VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
#if DBG         
            Ext2DbgPrint(D_FASTIO, "Ext2FastIoQueryBasicInfo: %-16.16s %-31s %s\n",
                Ext2DbgGetCurrentProcessName(),
                "FASTIO_QUERY_BASIC_INFO",
                Fcb->AnsiFileName.Buffer
                );
#endif          
            FsRtlEnterFileSystem();
            
            if (!ExAcquireResourceSharedLite(
                &Fcb->MainResource,
                Wait))
            {
                Status = FALSE;
                __leave;
            }
            
            FcbMainResourceAcquired = TRUE;
            
            RtlZeroMemory(Buffer, sizeof(FILE_BASIC_INFORMATION));
            
            /*
            typedef struct _FILE_BASIC_INFORMATION {
            LARGE_INTEGER   CreationTime;
            LARGE_INTEGER   LastAccessTime;
            LARGE_INTEGER   LastWriteTime;
            LARGE_INTEGER   ChangeTime;
            ULONG           FileAttributes;
            } FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;
            */

            Buffer->CreationTime = Ext2SysTime(Fcb->ext2_inode->i_ctime);
            Buffer->LastAccessTime = Ext2SysTime(Fcb->ext2_inode->i_atime);
            Buffer->LastWriteTime = Ext2SysTime(Fcb->ext2_inode->i_mtime);
            Buffer->ChangeTime = Ext2SysTime(Fcb->ext2_inode->i_mtime);
            
            
            Buffer->FileAttributes = Fcb->Ext2Mcb->FileAttr;
            
            IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);
            
            IoStatus->Status = STATUS_SUCCESS;
            
            Status =  TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }

    __finally
    {
        if (FcbMainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }
        
        FsRtlExitFileSystem();
    }
    
    
    if (Status == FALSE)
    {
#if DBG
        Ext2DbgPrint(D_FASTIO, "Ext2FastIoQueryBasicInfo: %-16.16s %-31s *** Status: FALSE ***\n",
            Ext2DbgGetCurrentProcessName(),
            "FASTIO_QUERY_BASIC_INFO"
            );
#endif
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        Ext2DbgPrint(D_FASTIO, "Ext2FastIoQueryBasicInfo: %-16.16s %-31s *** Status: %s (%#x) ***\n",
            Ext2FastIoQueryBasicInfo,
            "FASTIO_QUERY_BASIC_INFO",
            Ext2DbgNtStatusToString(IoStatus->Status),
            IoStatus->Status
            );
    }
    
    return Status;
}

BOOLEAN
Ext2FastIoQueryStandardInfo (
                IN PFILE_OBJECT                 FileObject,
                IN BOOLEAN                      Wait,
                OUT PFILE_STANDARD_INFORMATION  Buffer,
                OUT PIO_STATUS_BLOCK            IoStatus,
                IN PDEVICE_OBJECT               DeviceObject
                )
{
    
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;
    BOOLEAN     FcbMainResourceAcquired = FALSE;
    
    __try
    {
        __try
        {
            if (DeviceObject == gExt2Global->DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == EXT2VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
#if DBG         
            Ext2DbgPrint(D_FASTIO, "Ext2FastIoQueryStandardInfo: %-16.16s %-31s %s\n",
                Ext2DbgGetCurrentProcessName(),
                "FASTIO_QUERY_STANDARD_INFO",
                Fcb->AnsiFileName.Buffer );
#endif          
            FsRtlEnterFileSystem();
            
            if (!ExAcquireResourceSharedLite(
                &Fcb->MainResource,
                Wait        ))
            {
                Status = FALSE;
                __leave;
            }
            
            FcbMainResourceAcquired = TRUE;
            
            RtlZeroMemory(Buffer, sizeof(FILE_STANDARD_INFORMATION));
            
            /*
            typedef struct _FILE_STANDARD_INFORMATION {
            LARGE_INTEGER   AllocationSize;
            LARGE_INTEGER   EndOfFile;
            ULONG           NumberOfLinks;
            BOOLEAN         DeletePending;
            BOOLEAN         Directory;
            } FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;
            */
            
            Buffer->AllocationSize.QuadPart =
                (LONGLONG)(Fcb->ext2_inode->i_size);
            Buffer->EndOfFile.QuadPart =
                (LONGLONG)(Fcb->ext2_inode->i_size);            
            Buffer->NumberOfLinks = Fcb->ext2_inode->i_links_count;
            
            if (IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
                Buffer->DeletePending = FALSE;
            else
                Buffer->DeletePending = IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING);
            
            if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
            {
                Buffer->Directory = TRUE;
            }
            else
            {
                Buffer->Directory = FALSE;
            }
            
            IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);
            
            IoStatus->Status = STATUS_SUCCESS;
            
            Status =  TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }

    __finally
    {
        if (FcbMainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }
        
        FsRtlExitFileSystem();
    }

#if DBG
    if (Status == FALSE)
    {
        Ext2DbgPrint(D_FASTIO, "Ext2FastIoQueryStandardInfo: %-16.16s %-31s *** Status: FALSE ***\n",
            Ext2DbgGetCurrentProcessName(),
            "FASTIO_QUERY_STANDARD_INFO"            );
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        Ext2DbgPrint(D_FASTIO, "Ext2FastIoQueryStandardInfo: %-16.16s %-31s *** Status: %s (%#x) ***\n",
            Ext2DbgGetCurrentProcessName(),
            "FASTIO_QUERY_STANDARD_INFO",
            Ext2DbgNtStatusToString(IoStatus->Status),
            IoStatus->Status            );
    }
#endif
    
    return Status;
}

BOOLEAN
Ext2FastIoQueryNetworkOpenInfo (
     IN PFILE_OBJECT                     FileObject,
     IN BOOLEAN                          Wait,
     OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer,
     OUT PIO_STATUS_BLOCK                IoStatus,
     IN PDEVICE_OBJECT                   DeviceObject )
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;
    BOOLEAN     FcbMainResourceAcquired = FALSE;
    
    __try
    {
        __try
        {
            if (DeviceObject == gExt2Global->DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == EXT2VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
#if DBG         
            Ext2DbgPrint(D_FASTIO,
                DRIVER_NAME ": %-16.16s %-31s %s\n",
                PsGetCurrentProcess()->ImageFileName,
                "FASTIO_QUERY_NETWORK_OPEN_INFO",
                Fcb->AnsiFileName.Buffer
                );
#endif          
            FsRtlEnterFileSystem();
            
            if (!ExAcquireResourceSharedLite(
                &Fcb->MainResource,
                Wait
                ))
            {
                Status = FALSE;
                __leave;
            }
            
            FcbMainResourceAcquired = TRUE;
            
            RtlZeroMemory(Buffer, sizeof(FILE_NETWORK_OPEN_INFORMATION));
            
            /*
            typedef struct _FILE_NETWORK_OPEN_INFORMATION {
            LARGE_INTEGER   CreationTime;
            LARGE_INTEGER   LastAccessTime;
            LARGE_INTEGER   LastWriteTime;
            LARGE_INTEGER   ChangeTime;
            LARGE_INTEGER   AllocationSize;
            LARGE_INTEGER   EndOfFile;
            ULONG           FileAttributes;
            } FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;
            */
            
            Buffer->CreationTime = Ext2SysTime(Fcb->ext2_inode->i_ctime);
            Buffer->LastAccessTime = Ext2SysTime(Fcb->ext2_inode->i_atime);
            Buffer->LastWriteTime = Ext2SysTime(Fcb->ext2_inode->i_mtime);
            Buffer->ChangeTime = Ext2SysTime(Fcb->ext2_inode->i_mtime);
            Buffer->FileAttributes = Fcb->Ext2Mcb->FileAttr;
            Buffer->AllocationSize.QuadPart =
                (LONGLONG)(Fcb->ext2_inode->i_size);
            Buffer->EndOfFile.QuadPart =
                (LONGLONG)(Fcb->ext2_inode->i_size);            
            
            Buffer->FileAttributes = Fcb->Ext2Mcb->FileAttr;
            
            IoStatus->Information = sizeof(FILE_NETWORK_OPEN_INFORMATION);
            
            IoStatus->Status = STATUS_SUCCESS;
            
            Status =  TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }
    __finally
    {
        if (FcbMainResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }
        
        FsRtlExitFileSystem();
    }
    
    
    
    if (Status == FALSE)
    {
        Ext2DbgPrint(D_FASTIO,
            DRIVER_NAME ": %-16.16s %-31s *** Status: FALSE ***\n",
            PsGetCurrentProcess()->ImageFileName,
            "FASTIO_QUERY_NETWORK_OPEN_INFO"
            );
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        Ext2DbgPrint(D_FASTIO,
            DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
            PsGetCurrentProcess()->ImageFileName,
            "FASTIO_QUERY_NETWORK_OPEN_INFO",
            Ext2DbgNtStatusToString(IoStatus->Status),
            IoStatus->Status
            );
    }
    
    return Status;
}


BOOLEAN
Ext2FastIoLock (
           IN PFILE_OBJECT         FileObject,
           IN PLARGE_INTEGER       FileOffset,
           IN PLARGE_INTEGER       Length,
           IN PEPROCESS            Process,
           IN ULONG                Key,
           IN BOOLEAN              FailImmediately,
           IN BOOLEAN              ExclusiveLock,
           OUT PIO_STATUS_BLOCK    IoStatus,
           IN PDEVICE_OBJECT       DeviceObject
           )
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;
    
    __try
    {
        __try
        {
            if (DeviceObject == gExt2Global->DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == EXT2VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
            
            if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }
#if DBG         
            Ext2DbgPrint(D_FASTIO, "Ext2FastIoLock: %-16.16s %-31s %s\n",
                Ext2DbgGetCurrentProcessName(),
                "FASTIO_LOCK",
                Fcb->AnsiFileName.Buffer        );
#endif          
            Ext2DbgPrint(D_FASTIO, "Ext2FastIoLock: Offset: %I64u Length: %I64u Key: %u %s%s\n",
                FileOffset->QuadPart,
                Length->QuadPart,
                Key,
                (FailImmediately ? "FailImmediately " : ""),
                (ExclusiveLock ? "ExclusiveLock " : "") );
            
            if (Fcb->CommonFCBHeader.IsFastIoPossible != FastIoIsQuestionable)
            {
#if DBG
                Ext2DbgPrint(D_FASTIO, "Ext2FastIoLock: %-16.16s %-31s %s\n",
                    (PUCHAR) Process + gProcessNameOffset,
                    "FastIoIsQuestionable",
                    Fcb->AnsiFileName.Buffer        );
#endif              
                Fcb->CommonFCBHeader.IsFastIoPossible = FastIoIsQuestionable;
            }
            
            FsRtlEnterFileSystem();
            
            Status = FsRtlFastLock(
                &Fcb->FileLockAnchor,
                FileObject,
                FileOffset,
                Length,
                Process,
                Key,
                FailImmediately,
                ExclusiveLock,
                IoStatus,
                NULL,
                FALSE);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }

    __finally
    {
        FsRtlExitFileSystem();
    }

#if DBG 
    if (Status == FALSE)
    {
        Ext2DbgPrint(D_FASTIO,"Ext2FastIoLock: %-16.16s %-31s *** Status: FALSE ***\n",
            (PUCHAR) Process + gProcessNameOffset,
            "FASTIO_LOCK"
            );
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        Ext2DbgPrint(D_FASTIO, "Ext2FastIoLock: %-16.16s %-31s *** Status: %s (%#x) ***\n",
            (PUCHAR) Process + gProcessNameOffset,
            "FASTIO_LOCK",
            Ext2DbgNtStatusToString(IoStatus->Status),
            IoStatus->Status
            );
    }
#endif
    
    return Status;
}

BOOLEAN
Ext2FastIoUnlockSingle (
               IN PFILE_OBJECT         FileObject,
               IN PLARGE_INTEGER       FileOffset,
               IN PLARGE_INTEGER       Length,
               IN PEPROCESS            Process,
               IN ULONG                Key,
               OUT PIO_STATUS_BLOCK    IoStatus,
               IN PDEVICE_OBJECT       DeviceObject
               )
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;
    
    __try
    {
        __try
        {
            if (DeviceObject == gExt2Global->DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == EXT2VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
            
            if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }
#if DBG         
            Ext2DbgPrint(D_FASTIO,"Ext2FastIoUnlockSingle: %-16.16s %-31s %s\n",
                (PUCHAR) Process + gProcessNameOffset,
                "FASTIO_UNLOCK_SINGLE",
                Fcb->AnsiFileName.Buffer        );
#endif          
            Ext2DbgPrint(D_FASTIO,"Ext2FastIoUnlockSingle: Offset: %I64u Length: %I64u Key: %u\n",
                FileOffset->QuadPart,
                Length->QuadPart,
                Key     );
            
            FsRtlEnterFileSystem();
            
            IoStatus->Status = FsRtlFastUnlockSingle(
                &Fcb->FileLockAnchor,
                FileObject,
                FileOffset,
                Length,
                Process,
                Key,
                NULL,
                FALSE);                      
            
            IoStatus->Information = 0;
            
            Status =  TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }
    __finally
    {
        FsRtlExitFileSystem();
    }

#if DBG 
    if (Status == FALSE)
    {
        Ext2DbgPrint(D_FASTIO, "Ext2FastIoUnlockSingle: %-16.16s %-31s *** Status: FALSE ***\n",
            (PUCHAR) Process + gProcessNameOffset,
            "FASTIO_UNLOCK_SINGLE"          );
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        Ext2DbgPrint(D_FASTIO, "Ext2FastIoUnlockSingle: %-16.16s %-31s *** Status: %s (%#x) ***\n",
            (PUCHAR) Process + gProcessNameOffset,
            "FASTIO_UNLOCK_SINGLE",
            Ext2DbgNtStatusToString(IoStatus->Status),
            IoStatus->Status            );
    }
#endif  
    return Status;
}

BOOLEAN
Ext2FastIoUnlockAll (
            IN PFILE_OBJECT         FileObject,
            IN PEPROCESS            Process,
            OUT PIO_STATUS_BLOCK    IoStatus,
            IN PDEVICE_OBJECT       DeviceObject)
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;
    
    __try
    {
        __try
        {
            if (DeviceObject == gExt2Global->DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == EXT2VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
            
            if (FlagOn(Fcb->Ext2Mcb->FileAttr ,FILE_ATTRIBUTE_DIRECTORY))
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }
#if DBG         
            Ext2DbgPrint(D_FASTIO, "Ext2FastIoUnlockSingle: %-16.16s %-31s %s\n",
                (PUCHAR) Process + gProcessNameOffset,
                "FASTIO_UNLOCK_ALL",
                Fcb->AnsiFileName.Buffer
                );
#endif          
            FsRtlEnterFileSystem();
            
            IoStatus->Status = FsRtlFastUnlockAll(
                &Fcb->FileLockAnchor,
                FileObject,
                Process,
                NULL        );
            
            IoStatus->Information = 0;
            
            Status =  TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }
    __finally
    {
        FsRtlExitFileSystem();
    }

#if DBG 
    if (Status == FALSE)
    {
        Ext2DbgPrint(D_FASTIO, "Ext2FastIoUnlockSingle: %-16.16s %-31s *** Status: FALSE ***\n",
            (PUCHAR) Process + gProcessNameOffset,
            "FASTIO_UNLOCK_ALL"
            );
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        Ext2DbgPrint(D_FASTIO, "Ext2FastIoUnlockSingle: %-16.16s %-31s *** Status: %s (%#x) ***\n",
            (PUCHAR) Process + gProcessNameOffset,
            "FASTIO_UNLOCK_ALL",
            Ext2DbgNtStatusToString(IoStatus->Status),
            IoStatus->Status
            );
    }
#endif  
    return Status;
}

BOOLEAN
Ext2FastIoUnlockAllByKey (
             IN PFILE_OBJECT         FileObject,
             IN PEPROCESS            Process,
             IN ULONG                Key,
             OUT PIO_STATUS_BLOCK    IoStatus,
             IN PDEVICE_OBJECT       DeviceObject
             )
{
    BOOLEAN     Status = FALSE;
    PEXT2_FCB   Fcb;
    
    __try
    {
        __try
        {
            if (DeviceObject == gExt2Global->DeviceObject)
            {
                IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
                Status = TRUE;
                __leave;
            }
            
            Fcb = (PEXT2_FCB) FileObject->FsContext;
            
            ASSERT(Fcb != NULL);
            
            if (Fcb->Identifier.Type == EXT2VCB)
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }
            
            ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
                (Fcb->Identifier.Size == sizeof(EXT2_FCB)));
            
            if (FlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
            {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
                Status = TRUE;
                __leave;
            }
#if DBG         
            Ext2DbgPrint(D_FASTIO, "Ext2FastIoUnlockAllByKey: %-16.16s %-31s %s\n",
                (PUCHAR) Process + gProcessNameOffset,
                "FASTIO_UNLOCK_ALL_BY_KEY",
                Fcb->AnsiFileName.Buffer
                );
#endif          
            Ext2DbgPrint(D_FASTIO, "Ext2FastIoUnlockAllByKey: Key: %u\n",
                Key
                );
            
            FsRtlEnterFileSystem();
            
            IoStatus->Status = FsRtlFastUnlockAllByKey(
                &Fcb->FileLockAnchor,
                FileObject,
                Process,
                Key,
                NULL
                );  
            
            IoStatus->Information = 0;
            
            Status =  TRUE;
        }

        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoStatus->Status = GetExceptionCode();
            Status = TRUE;
        }
    }

    __finally
    {
        FsRtlExitFileSystem();
    }
#if DBG 
    if (Status == FALSE)
    {
        Ext2DbgPrint(D_FASTIO, "Ext2FastIoUnlockAllByKey: %-16.16s %-31s *** Status: FALSE ***\n",
            (PUCHAR) Process + gProcessNameOffset,
            "FASTIO_UNLOCK_ALL_BY_KEY"
            );
    }
    else if (IoStatus->Status != STATUS_SUCCESS)
    {
        Ext2DbgPrint(D_FASTIO, "Ext2FastIoUnlockAllByKey: %-16.16s %-31s *** Status: %s (%#x) ***\n",
            (PUCHAR) Process + gProcessNameOffset,
            "FASTIO_UNLOCK_ALL_BY_KEY",
            Ext2DbgNtStatusToString(IoStatus->Status),
            IoStatus->Status
            );
    }
#endif  
    return Status;
}
