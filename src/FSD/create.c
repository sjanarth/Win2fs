/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             create.c
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
#pragma alloc_text(PAGE, Ext2LookupFileName)
#pragma alloc_text(PAGE, Ext2SearchFcbList)
#pragma alloc_text(PAGE, Ext2ScanDir)
#pragma alloc_text(PAGE, Ext2CreateFile)
#pragma alloc_text(PAGE, Ext2CreateVolume)
#pragma alloc_text(PAGE, Ext2Create)
#pragma alloc_text(PAGE, Ext2CreateInode)
#endif

NTSTATUS
Ext2LookupFileName (IN PEXT2_VCB    Vcb,
            IN PUNICODE_STRING      FullFileName,
            IN PEXT2_FCB            ParentFcb,
            OUT PEXT2_MCB *         Ext2Mcb,
            IN OUT PEXT2_INODE      ext2_inode)
{
    NTSTATUS        Status;
    UNICODE_STRING  FileName;
    PEXT2_MCB       ParentMcb;
    PEXT2_MCB       Mcb;

    EXT2_DIR_ENTRY2 ext2_dir;
    int             i = 0;
    BOOLEAN         bRun = TRUE;
    BOOLEAN         bParent = FALSE;
    EXT2_INODE      in;
    ULONG           off = 0;

    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    *Ext2Mcb = NULL;

    if (ParentFcb && FullFileName->Buffer[0] != L'\\')
    {
        ParentMcb = ParentFcb->Ext2Mcb;
        bParent = TRUE;
    }
    else
    {
        ParentMcb = Vcb->Ext2McbTree;
    }

    RtlZeroMemory(&ext2_dir, sizeof(EXT2_DIR_ENTRY2));

    if (FullFileName->Length == 0)
    {
        return Status;
    }

    if (FullFileName->Length == 2 && FullFileName->Buffer[0] == L'\\')
    {
        if (!Ext2LoadInode(Vcb, ParentMcb->Inode, ext2_inode))
        {
            return Status;      
        }

        *Ext2Mcb = Vcb->Ext2McbTree;

        return STATUS_SUCCESS;
    }

    while (bRun && i < FullFileName->Length/2)
    {
        int Length;
        ULONG FileAttr = FILE_ATTRIBUTE_NORMAL;

        if (bParent)
        {
            bParent = FALSE;
        }
        else
        {
            while(i < FullFileName->Length/2 && FullFileName->Buffer[i] == L'\\') i++;
        }

        Length = i;

        while(i < FullFileName->Length/2 && (FullFileName->Buffer[i] != L'\\')) i++;

        if (i - Length >0)
        {
            FileName = *FullFileName;
            FileName.Buffer += Length;
            FileName.Length = (USHORT)((i - Length) * 2);

            Mcb = Ext2SearchMcb(ParentMcb, &FileName);

            if (Mcb)
            {
                ParentMcb = Mcb;

                Status = STATUS_SUCCESS;

                if (!IsFlagOn(Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
                {
                    if (i < FullFileName->Length/2)
                    {
                        Status =  STATUS_OBJECT_NAME_NOT_FOUND;
                    }
                    break;
                }
            }
            else
            {
                if (!Ext2LoadInode(Vcb, ParentMcb->Inode, &in))
                {
                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    break;
                }

                if (!S_ISDIR(in.i_mode))
                {
                    if (i < FullFileName->Length/2)
                    {
                        Status =  STATUS_OBJECT_NAME_NOT_FOUND;
                    }
                    break;
                }

                Status = Ext2ScanDir (
                    Vcb,
                    ParentMcb,
                    &FileName,
                    &off,
                    &in,
                    &ext2_dir);

                if (!NT_SUCCESS(Status))
                {
                    bRun = FALSE;
/*
                    if (i >= FullFileName->Length/2)
                    {
                        *Ext2Mcb = ParentMcb;
                    }
*/
                }
                else
                {
                    if (IsFlagOn(Vcb->ext2_super_block->s_feature_incompat, EXT2_FEATURE_INCOMPAT_FILETYPE))
                    {
                        if (ext2_dir.file_type == EXT2_FT_DIR)
                            SetFlag(FileAttr, FILE_ATTRIBUTE_DIRECTORY);
                    }
                    else
                    {
                        if (!Ext2LoadInode(Vcb, ext2_dir.inode, &in))
                        {
                            Status = STATUS_OBJECT_NAME_NOT_FOUND;
                            break;
                        }

                        if (S_ISDIR(in.i_mode))
                        {
                            SetFlag(FileAttr, FILE_ATTRIBUTE_DIRECTORY);
                        }
                    }

					Mcb = Ext2AllocateMcb(Vcb, &FileName, FileAttr);

					if (!Mcb)
					{
						Status = STATUS_OBJECT_NAME_NOT_FOUND;
						break;
					}

                    Mcb->Inode = ext2_dir.inode;
                    Mcb->DeOffset = off;
                    Ext2AddMcbNode(ParentMcb, Mcb);
                    ParentMcb = Mcb;
                }

            }
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(Status))
    {
        *Ext2Mcb = Mcb;
        if (ext2_inode)
        {
            if (!Ext2LoadInode(Vcb, Mcb->Inode, ext2_inode))
            {
                Ext2DbgPrint(D_CREATE, "Ext2LoopupFileName: error loading Inode %xh\n", Mcb->Inode);
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    return Status;
}


NTSTATUS
Ext2ScanDir (IN PEXT2_VCB       Vcb,
         IN PEXT2_MCB           ParentMcb,
         IN PUNICODE_STRING     FileName,
         IN OUT PULONG          Index,
         IN PEXT2_INODE         ext2_inode,
         IN OUT PEXT2_DIR_ENTRY2 ext2_dir)
{
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    USHORT                  InodeFileNameLength;
    UNICODE_STRING          InodeFileName;

    PEXT2_DIR_ENTRY2        pDir = NULL;
    ULONG                   dwBytes = 0;
    BOOLEAN                 bFound = FALSE;
    PEXT2_FCB               Dcb = NULL;
    LONGLONG                Offset = 0;
    ULONG                   inode = ParentMcb->Inode;

    ULONG                   dwRet;

    __try
    {

        pDir = (PEXT2_DIR_ENTRY2) ExAllocatePool(PagedPool,
                                    sizeof(EXT2_DIR_ENTRY2));
        if (!pDir)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        InodeFileName.Buffer = ExAllocatePool(
                    PagedPool,
                    (EXT2_NAME_LEN + 1) * 2 );

        if (!InodeFileName.Buffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }


        Dcb = ParentMcb->Ext2Fcb;

        dwBytes = 0;

        while (!bFound && dwBytes < ext2_inode->i_size)
        {
            RtlZeroMemory(pDir, sizeof(EXT2_DIR_ENTRY2));

            // Reading the DCB contents
            Status = Ext2ReadInode(
                        NULL,
                        Vcb,
                        ext2_inode,
                        dwBytes,
                        (PVOID)pDir,
                        sizeof(EXT2_DIR_ENTRY2),
                        &dwRet);


            if (!NT_SUCCESS(Status))
            {
                Ext2DbgPrint(D_CREATE, "Ext2ScanDir: Reading Directory Content error.\n");
                __leave;
            }

            if (pDir->inode)
            {
                InodeFileNameLength = pDir->name_len & 0xff;
                
                InodeFileName.Length = InodeFileName.MaximumLength =
                    InodeFileNameLength * 2;

                RtlZeroMemory(InodeFileName.Buffer, InodeFileNameLength * 2 + 2);
                
                Ext2CharToWchar(
                    InodeFileName.Buffer,
                    pDir->name,
                    InodeFileNameLength );

                if (!RtlCompareUnicodeString(
                    FileName,
                    &InodeFileName,
                    TRUE ))
                {
                    bFound = TRUE;
                    *Index = dwBytes;
                    RtlCopyMemory(ext2_dir, pDir, pDir->rec_len > sizeof(EXT2_DIR_ENTRY2)
                        ? sizeof(EXT2_DIR_ENTRY2) : pDir->rec_len);
                    Status = STATUS_SUCCESS;
                    Ext2DbgPrint(D_CREATE, "Ext2ScanDir: Found: Name=%S Inode=%xh\n", InodeFileName.Buffer, pDir->inode);
                }
                
                dwBytes +=pDir->rec_len;
                Offset = (LONGLONG)dwBytes;
            }
            else
            {
                break;
            }
        }

        if (!bFound)
            Status = STATUS_NO_SUCH_FILE;
    }

    __finally
    {
        if (InodeFileName.Buffer != NULL)
        {
            ExFreePool(InodeFileName.Buffer);
        }

        if (pDir)
            ExFreePool(pDir);
    }
    
    return Status;
}

/*

PEXT2_FCB
Ext2SearchFcbList(	IN PEXT2_VCB    Vcb,
				    IN ULONG        inode )
{
	BOOLEAN				bFound = FALSE;
	PLIST_ENTRY         Link;
	PEXT2_FCB           TmpFcb;

	Link = Vcb->FcbList.Flink;
	
	while (!bFound && Link != &Vcb->FcbList)
	{
		TmpFcb = CONTAINING_RECORD(Link, EXT2_FCB, Next);
		
		if (TmpFcb && TmpFcb->Identifier.Type == FCB)
		{
#if DBG
			Ext2DbgPrint(D_CREATE, "Ext2SearchFcbList: [%s,%xh]\n", 
                TmpFcb->AnsiFileName.Buffer, TmpFcb->Inode);
#endif			
			if (TmpFcb->Inode == inode)
			{
				Ext2DbgPrint(D_CREATE, "Ext2SearchMcb: Found FCB for %xh.\n", inode);
				bFound = TRUE;
			}
		}
		Link = Link->Flink;
	}

	if (bFound)
		return TmpFcb;
	else
		return NULL;
	
}
*/

NTSTATUS
Ext2CreateFile(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION  io_stack;
    PEXT2_FCB           Fcb = NULL;
    PEXT2_FCB           pParentFcb = NULL;
    BOOLEAN             bParentFcbCreated = FALSE;
    PEXT2_CCB           Ccb = NULL;
    PEXT2_INODE         ext2_inode;
    BOOLEAN             VcbResourceAcquired = FALSE;
    BOOLEAN             bDir = FALSE;
    BOOLEAN             bFcbAllocated = FALSE;
    BOOLEAN             bCreated = FALSE;
    UNICODE_STRING      FileName;
    PEXT2_MCB           Ext2Mcb = NULL;
    PIRP                Irp;

    ULONG               Options;
    ULONG               CreateDisposition;

    BOOLEAN             OpenDirectory;
    BOOLEAN             OpenTargetDirectory;
    BOOLEAN             CreateDirectory;
    BOOLEAN             SequentialOnly;
    BOOLEAN             NoIntermediateBuffering;
    BOOLEAN             IsPagingFile;
    BOOLEAN             DirectoryFile;
    BOOLEAN             NonDirectoryFile;
    BOOLEAN             NoEaKnowledge;
    BOOLEAN             DeleteOnClose;
    BOOLEAN             TemporaryFile;
    BOOLEAN             CaseSensitive;

    ACCESS_MASK         DesiredAccess;
    ULONG               ShareAccess;


    Irp = IrpContext->Irp;
    io_stack = IoGetCurrentIrpStackLocation(Irp);

    Options  = io_stack->Parameters.Create.Options;
    
    DirectoryFile = IsFlagOn(Options, FILE_DIRECTORY_FILE);
    OpenTargetDirectory = IsFlagOn(io_stack->Flags, SL_OPEN_TARGET_DIRECTORY);

    NonDirectoryFile = IsFlagOn(Options, FILE_NON_DIRECTORY_FILE);
    SequentialOnly = IsFlagOn(Options, FILE_SEQUENTIAL_ONLY);
    NoIntermediateBuffering = IsFlagOn( Options, FILE_NO_INTERMEDIATE_BUFFERING );
    NoEaKnowledge = IsFlagOn(Options, FILE_NO_EA_KNOWLEDGE);
    DeleteOnClose = IsFlagOn(Options, FILE_DELETE_ON_CLOSE);

    CaseSensitive = IsFlagOn(io_stack->Flags, SL_CASE_SENSITIVE);

    TemporaryFile = IsFlagOn(io_stack->Parameters.Create.FileAttributes,
                                   FILE_ATTRIBUTE_TEMPORARY );

    CreateDisposition = (Options >> 24) & 0x000000ff;

    IsPagingFile = IsFlagOn(io_stack->Flags, SL_OPEN_PAGING_FILE);

    CreateDirectory = (BOOLEAN)(DirectoryFile &&
                                ((CreateDisposition == FILE_CREATE) ||
                                 (CreateDisposition == FILE_OPEN_IF)));

    OpenDirectory   = (BOOLEAN)(DirectoryFile &&
                                ((CreateDisposition == FILE_OPEN) ||
                                 (CreateDisposition == FILE_OPEN_IF)));

    DesiredAccess = io_stack->Parameters.Create.SecurityContext->DesiredAccess;
    ShareAccess   = io_stack->Parameters.Create.ShareAccess;

    FileName.Buffer = NULL;

    __try
    {
        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource, TRUE );
        
        VcbResourceAcquired = TRUE;

		if (Irp->Overlay.AllocationSize.HighPart) 
		{
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
        
        if (!(ext2_inode = ExAllocatePool   (
              PagedPool, sizeof(EXT2_INODE) ) ))
        {
            __leave;
        }

        FileName.MaximumLength = io_stack->FileObject->FileName.MaximumLength;
        FileName.Length = io_stack->FileObject->FileName.Length;

        FileName.Buffer = ExAllocatePool(PagedPool, FileName.MaximumLength);
        if (!FileName.Buffer)
        {   
            Status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlZeroMemory(FileName.Buffer, FileName.MaximumLength);
        RtlCopyMemory(FileName.Buffer, io_stack->FileObject->FileName.Buffer, FileName.Length);

        if (io_stack->FileObject->RelatedFileObject)
        {
            pParentFcb = (PEXT2_FCB)(io_stack->FileObject->RelatedFileObject->FsContext);
        }

        if ((FileName.Length > sizeof(WCHAR)) &&
            (FileName.Buffer[1] == L'\\') &&
            (FileName.Buffer[0] == L'\\')) {
            
            FileName.Length -= sizeof(WCHAR);
            
            RtlMoveMemory( &FileName.Buffer[0],
                &FileName.Buffer[1],
                FileName.Length );
            
            //
            //  Bad Name if there are still two beginning backslashes.
            //
            
            if ((FileName.Length > sizeof(WCHAR)) &&
                (FileName.Buffer[1] == L'\\') &&
                (FileName.Buffer[0] == L'\\')) {
                                
                Status = STATUS_OBJECT_NAME_INVALID;

                __leave;
            }
        }

        Ext2DbgPrint(D_CREATE, "Ext2CreateFile: %S (name len=%xh)Opt: %xh.\n",
            FileName.Buffer, FileName.Length,
            io_stack->Parameters.Create.Options);

        Status = Ext2LookupFileName(
            Vcb,
            &FileName,
            pParentFcb,
            &Ext2Mcb,
            ext2_inode );

        if (!NT_SUCCESS(Status))
        {
            UNICODE_STRING  NewName;

            NewName = FileName;

            Ext2DbgPrint(D_CREATE, "Ext2CreateFile: File Not Found.\n");

            Ext2Mcb = NULL;

            // We need to create a new one ?
            if ((CreateDisposition == FILE_CREATE ) ||
				(CreateDisposition == FILE_OPEN_IF) ||
				(CreateDisposition == FILE_OVERWRITE_IF))
            {
                if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
                {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    __leave;
                }

                if (DirectoryFile)
                {
                    if (TemporaryFile)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        __leave;
                    }
                }

                if (!pParentFcb)
                {
                    if (DirectoryFile)
                    {
                        while (NewName.Length > 0 && NewName.Buffer[NewName.Length/2 - 1] == L'\\')
                        {
                            NewName.Buffer[NewName.Length/2 - 1] = 0;
                            NewName.Length -= 2;
                        }
                    }
                    else
                    {
                        if (NewName.Buffer[NewName.Length/2 - 1] == L'\\')
                        {
                            Status = STATUS_INVALID_PARAMETER;
                            __leave;
                        }
                    }

                    while (NewName.Length > 0 && NewName.Buffer[NewName.Length/2 - 1] != L'\\')
                    {
                        NewName.Length -= 2;
                    }

                    Status = Ext2LookupFileName (
                                Vcb,
                                &NewName,
                                pParentFcb,
                                &Ext2Mcb,
                                ext2_inode      );


                    if (NT_SUCCESS(Status))
                    {
                        pParentFcb = Ext2Mcb->Ext2Fcb;

                        NewName.Buffer = (USHORT *)((UCHAR *)NewName.Buffer + NewName.Length);
                        NewName.Length = FileName.Length - NewName.Length;

                        //Here we should create the fcb.
                        if (!pParentFcb)
                        {
                            PEXT2_INODE pTmpInode = ExAllocatePool(PagedPool, sizeof(EXT2_INODE));
                            if (!pTmpInode)
                            {
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                __leave;
                            }
                            RtlCopyMemory(pTmpInode, ext2_inode, sizeof(EXT2_INODE));
                            pParentFcb = Ext2AllocateFcb(Vcb,  Ext2Mcb, pTmpInode);

                            if (!pParentFcb)
                            {
                                ExFreePool(pTmpInode);
                                Status = STATUS_INVALID_PARAMETER;
                                __leave;
                            }

                            bParentFcbCreated = TRUE;

                            ExAcquireResourceExclusiveLite(&pParentFcb->CountResource, TRUE);
                            pParentFcb->ReferenceCount++;
                            ExReleaseResourceForThreadLite(
                                    &pParentFcb->CountResource,
                                    ExGetCurrentResourceThread());

                        }
                    }

                    Ext2Mcb = NULL;
                }

                // Here we get a valid pParentFcb

                if (DirectoryFile)
                {
                    Status = Ext2CreateInode(IrpContext, Vcb, pParentFcb, EXT2_FT_DIR, io_stack->Parameters.Create.FileAttributes, &NewName);
                }
                else
                {
                    Status = Ext2CreateInode(IrpContext, Vcb, pParentFcb, EXT2_FT_REG_FILE, io_stack->Parameters.Create.FileAttributes, &NewName);
                }
                
                if (NT_SUCCESS(Status))
                {
                    bCreated = TRUE;

                    Irp->IoStatus.Information = FILE_CREATED;                    
                    Status = Ext2LookupFileName (
                                Vcb,
                                &NewName,
                                pParentFcb,
                                &Ext2Mcb,
                                ext2_inode      );

                    if (!NT_SUCCESS(Status))
                    {
                        Ext2DbgBreakPoint();
                    }
                }
                else
                {
                    Ext2DbgBreakPoint();
                }
            }
            else if (OpenTargetDirectory)
            {
                if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
                {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    __leave;
                }

                if (!pParentFcb)
                {
                    while (NewName.Length > 0 && NewName.Buffer[NewName.Length/2 - 1] == L'\\')
                    {
                         NewName.Length -= 2;
                    }

                    while (NewName.Length > 0 && NewName.Buffer[NewName.Length/2 - 1] != L'\\')
                    {
                        NewName.Length -= 2;
                    }

                    Status = Ext2LookupFileName (
                                Vcb,
                                &NewName,
                                pParentFcb,
                                &Ext2Mcb,
                                ext2_inode      );

                    if (NT_SUCCESS(Status))
                    {
                        pParentFcb = Ext2Mcb->Ext2Fcb;

                        NewName.Buffer = (USHORT *)((ULONG)(NewName.Buffer) + NewName.Length);
                        NewName.Length = FileName.Length - NewName.Length;

                        RtlZeroMemory(io_stack->FileObject->FileName.Buffer, 
                                      io_stack->FileObject->FileName.Length );

                        io_stack->FileObject->FileName.Length = NewName.Length;

                        RtlCopyMemory( io_stack->FileObject->FileName.Buffer,
                                       NewName.Buffer,
                                       NewName.Length );
                        

                        //Here we should create the fcb.
                        if (!pParentFcb)
                        {
                            PEXT2_INODE pTmpInode = ExAllocatePool(PagedPool, sizeof(EXT2_INODE));
                            if (!pTmpInode)
                            {
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                __leave;
                            }
                            RtlCopyMemory(pTmpInode, ext2_inode, sizeof(EXT2_INODE));
                            pParentFcb = Ext2AllocateFcb(Vcb,  Ext2Mcb, pTmpInode);

                            if (!pParentFcb)
                            {
                                ExFreePool(pTmpInode);
                                Status = STATUS_INVALID_PARAMETER;
                                __leave;
                            }

                            bParentFcbCreated = TRUE;

                            ExAcquireResourceExclusiveLite(&pParentFcb->CountResource, TRUE);
                            pParentFcb->ReferenceCount++;
                            ExReleaseResourceForThreadLite(
                                    &pParentFcb->CountResource,
                                    ExGetCurrentResourceThread());
                        }
                    }

                    if (!pParentFcb)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        __leave;
                    }
                }

                Irp->IoStatus.Information = FILE_DOES_NOT_EXIST;
                Status = STATUS_SUCCESS;
            }
/*
            else if ((CreateDisposition == FILE_OPEN) ||
                     (CreateDisposition == FILE_OVERWRITE))
            {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                __leave;
            }
*/
            else
            {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                __leave;
            }
        }
        else // File / Dir already exists.
        {
            if (OpenTargetDirectory)
            {
                if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
                {
                    Status = STATUS_MEDIA_WRITE_PROTECTED;
                    __leave;
                }

                Irp->IoStatus.Information = FILE_EXISTS;
                Status = STATUS_SUCCESS;

                RtlZeroMemory(io_stack->FileObject->FileName.Buffer, io_stack->FileObject->FileName.MaximumLength);
                io_stack->FileObject->FileName.Length = Ext2Mcb->ShortName.Length;

                RtlCopyMemory( io_stack->FileObject->FileName.Buffer,
                               Ext2Mcb->ShortName.Buffer,
                               Ext2Mcb->ShortName.Length );

                //Let Mcb pointer to it's parent
                Ext2Mcb = Ext2Mcb->Parent;

                goto Openit;
            }

            // We can not create if one exists
            if (CreateDisposition == FILE_CREATE)
            {
                Irp->IoStatus.Information = FILE_EXISTS;
                Status = STATUS_OBJECT_NAME_COLLISION;
                __leave;
            }

            if(IsFlagOn(Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
            {
                if ((CreateDisposition != FILE_OPEN) &&
                    (CreateDisposition != FILE_OPEN_IF))
                {

                    Status = STATUS_OBJECT_NAME_COLLISION;
                    __leave;
                }

			    if (NonDirectoryFile) 
			    {
				    Status = STATUS_FILE_IS_A_DIRECTORY;
				    __leave;
			    }

                if (Ext2Mcb->Inode == EXT2_ROOT_INO)
                {
                    if (DeleteOnClose)
                    {
                        Status = STATUS_CANNOT_DELETE;
                        __leave;
                    }

                    if (OpenTargetDirectory)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        __leave;
                    }
                }
            }

            Irp->IoStatus.Information = FILE_OPENED;
        }

Openit:
        
        if (Ext2Mcb)
        {
            Fcb = Ext2Mcb->Ext2Fcb;

            if (!Fcb)
            {
                Fcb = Ext2AllocateFcb (Vcb, Ext2Mcb, ext2_inode);
                bFcbAllocated = TRUE;
            }
        }
        
        if (Fcb)
        {
            if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED))
            {
                Status = STATUS_FILE_DELETED;
                __leave;
            }

            if (FlagOn(Fcb->Flags, FCB_DELETE_PENDING))
            {
                Status = STATUS_DELETE_PENDING;
                __leave;
            }

            // Add ./.. entries
            if (bCreated)
            {
                if (DirectoryFile)
                {
                    UNICODE_STRING EntryName;
                    USHORT  NameBuf[6];

                    RtlZeroMemory(&NameBuf, 6 * sizeof(USHORT));

                    EntryName.Length = EntryName.MaximumLength = 2;
                    EntryName.Buffer = &NameBuf[0];
                    NameBuf[0] = (USHORT)'.';

                    Ext2AddEntry(IrpContext, Vcb, Fcb, EXT2_FT_DIR, Fcb->Ext2Mcb->Inode, &EntryName);
                    Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Inode, Fcb->ext2_inode);

                    EntryName.Length = EntryName.MaximumLength = 4;
                    EntryName.Buffer = &NameBuf[0];
                    NameBuf[0] = NameBuf[1] = (USHORT)'.';

                    Ext2AddEntry(IrpContext, Vcb, Fcb, EXT2_FT_DIR, Fcb->Ext2Mcb->Parent->Inode, &EntryName);

                    //No need to inc the links count, 'cause
                    //pParentFcb->ext2_inode->i_links_count++;
                    Ext2SaveInode(IrpContext, Vcb, Fcb->Ext2Mcb->Parent->Inode, pParentFcb->ext2_inode);
                }
                else
                {
                    if (!Ext2ExpandFileAllocation(IrpContext, Vcb, Fcb, &(Irp->Overlay.AllocationSize)))
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        __leave;
                    }
                }
            }
            else    // For existing files
            {
                if (DeleteOnClose)
                {
                    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
                    {
                        Status = STATUS_MEDIA_WRITE_PROTECTED;
                        __leave;
                    }

                    SetFlag(Fcb->Flags, FCB_DELETE_ON_CLOSE);
                }
            }

            if (!IsFlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
            {
                if ((CreateDisposition == FILE_SUPERSEDE) && !IsPagingFile)
                {
                    DesiredAccess |= DELETE;
                }
                else if (((CreateDisposition == FILE_OVERWRITE) ||
                        (CreateDisposition == FILE_OVERWRITE_IF)) && !IsPagingFile)
                {
                    DesiredAccess |= FILE_WRITE_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES;
                }
            }

            if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
            {
                if (Fcb->OpenHandleCount > 0) 
                {
                    Status = IoCheckShareAccess(DesiredAccess, ShareAccess, io_stack->FileObject,
                                                &(Fcb->ShareAccess), TRUE);

                    if (!NT_SUCCESS(Status))
                    {
                        __leave;
				    }
                } 
                else 
                {
                    IoSetShareAccess(DesiredAccess, ShareAccess, io_stack->FileObject, &(Fcb->ShareAccess));
                }
            }

            Ccb = Ext2AllocateCcb();

            ExAcquireResourceExclusiveLite(&Fcb->CountResource, TRUE);
            Fcb->OpenHandleCount++;
            Fcb->ReferenceCount++;

            if (IsFlagOn(io_stack->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING))
            {
                Fcb->NonCachedOpenCount++;
            }

            ExReleaseResourceForThreadLite(
                    &Fcb->CountResource,
                    ExGetCurrentResourceThread());

            ExAcquireResourceExclusiveLite(&Vcb->CountResource, TRUE);
            Vcb->OpenFileHandleCount++;
            Vcb->ReferenceCount++;
            ExReleaseResourceForThreadLite(
                    &Vcb->CountResource,
                    ExGetCurrentResourceThread());
            
#if DBG
            Ext2DbgPrint(D_CREATE, "Ext2CreateFile: %s refercount=%xh\n", Fcb->AnsiFileName.Buffer, Fcb->ReferenceCount);
#endif

            if (!IsFlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
            {
                Fcb->CommonFCBHeader.IsFastIoPossible = FastIoIsPossible;

                if (IsFlagOn(io_stack->FileObject->Flags, FO_CACHE_SUPPORTED) &&
                    (Fcb->SectionObject.DataSectionObject != NULL))
                {
                    if (Fcb->NonCachedOpenCount == Fcb->OpenHandleCount)
                    {
                        /* IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED) */

                        if(!IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
                        {
                            CcFlushCache(&Fcb->SectionObject, NULL, 0, NULL);
                            ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);
                        }

                        CcPurgeCacheSection(&Fcb->SectionObject,
                                             NULL,
                                             0,
                                             FALSE );
                    }
                }
            }

            io_stack->FileObject->FsContext = (void*)Fcb;
            io_stack->FileObject->FsContext2 = (void*) Ccb;
            io_stack->FileObject->PrivateCacheMap = NULL;
            io_stack->FileObject->SectionObjectPointer = &(Fcb->SectionObject);
            io_stack->FileObject->Vpb = Vcb->Vpb;

            Status = STATUS_SUCCESS;
#if DBG
            Ext2DbgPrint(D_CREATE, "Ext2CreateFile: %s OpenCount: %u ReferCount: %u\n",
                Fcb->AnsiFileName.Buffer, Fcb->OpenHandleCount, Fcb->ReferenceCount);
#endif

            if (!NoIntermediateBuffering )
            {
                io_stack->FileObject->Flags |= FO_CACHE_SUPPORTED;
            }

            if (!bCreated && !IsFlagOn(Fcb->Ext2Mcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY))
            {
                if ( DeleteOnClose ||
                    (CreateDisposition == FILE_OVERWRITE) ||
                    (CreateDisposition == FILE_OVERWRITE_IF))
                {
                    if (!MmFlushImageSection( &Fcb->SectionObject,
                                              MmFlushForWrite ))
                    {

                        Status = DeleteOnClose ? STATUS_CANNOT_DELETE :
                                                 STATUS_SHARING_VIOLATION;
                        __leave;
                    }
                }

                if ((CreateDisposition == FILE_SUPERSEDE) ||
                    (CreateDisposition == FILE_OVERWRITE) ||
                    (CreateDisposition == FILE_OVERWRITE_IF))
                {
                    BOOLEAN bRet;

                    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
                    {
                        Status = STATUS_MEDIA_WRITE_PROTECTED;
                        __leave;
                    }
                    
                    Ext2SupersedeOrOverWriteFile(IrpContext, Vcb, Fcb, CreateDisposition);
                    bRet = Ext2ExpandFileAllocation(IrpContext, Vcb, Fcb, &(Irp->Overlay.AllocationSize));

                    if (!bRet)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        __leave;
                    }

                    if (CreateDisposition == FILE_SUPERSEDE)
                    {
                       Irp->IoStatus.Information = FILE_SUPERSEDED;
                    }
                    else
                    {
                        Irp->IoStatus.Information = FILE_OVERWRITTEN;
                    }
                }
            }
        }
    }

    __finally
    {
        if (FileName.Buffer)
            ExFreePool(FileName.Buffer);

        if (bParentFcbCreated)
        {
            ExAcquireResourceExclusiveLite(&pParentFcb->CountResource, TRUE);
            pParentFcb->ReferenceCount--;
            ExReleaseResourceForThreadLite(
                    &pParentFcb->CountResource,
                    ExGetCurrentResourceThread());
        }

        if (VcbResourceAcquired)
        {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread() );
        }

        if (!bFcbAllocated)
        {
            if (ext2_inode)
                ExFreePool(ext2_inode);
        }
        else
        {
            if (!Fcb && ext2_inode)
                ExFreePool(ext2_inode);
        }
    }
    
    return Status;
}

NTSTATUS
Ext2CreateVolume(PEXT2_IRP_CONTEXT IrpContext, PEXT2_VCB Vcb)
{
    PIO_STACK_LOCATION  io_stack;
    PIRP                Irp;

    NTSTATUS            Status;

    ACCESS_MASK         DesiredAccess;
    ULONG               ShareAccess;

    ULONG               Options;
    BOOLEAN             DirectoryFile;
    BOOLEAN             OpenTargetDirectory;

    ULONG               CreateDisposition;

    Irp = IrpContext->Irp;
    io_stack = IoGetCurrentIrpStackLocation(Irp);

    Options  = io_stack->Parameters.Create.Options;
    
    DirectoryFile = IsFlagOn(Options, FILE_DIRECTORY_FILE);
    OpenTargetDirectory = IsFlagOn(io_stack->Flags, SL_OPEN_TARGET_DIRECTORY);

    CreateDisposition = (Options >> 24) & 0x000000ff;

    DesiredAccess = io_stack->Parameters.Create.SecurityContext->DesiredAccess;
    ShareAccess   = io_stack->Parameters.Create.ShareAccess;

    if (DirectoryFile)
        return STATUS_NOT_A_DIRECTORY;

    if (OpenTargetDirectory)
        return STATUS_INVALID_PARAMETER;

	if ((CreateDisposition != FILE_OPEN) && (CreateDisposition != FILE_OPEN_IF)) 
	{
		return STATUS_ACCESS_DENIED;
    }

    Status = STATUS_SUCCESS;

    {
        if (Vcb->OpenHandleCount > 0) 
        {
            Status = IoCheckShareAccess(DesiredAccess, ShareAccess, io_stack->FileObject,
                                        &(Vcb->ShareAccess), TRUE);

            if (!NT_SUCCESS(Status)) 
            {
			    goto errorout;
            }
        } 
        else 
        {
            IoSetShareAccess(DesiredAccess, ShareAccess, io_stack->FileObject, &(Vcb->ShareAccess));
        }
    }

    {
            io_stack->FileObject->FsContext = Vcb;
        
            ExAcquireResourceExclusiveLite(
                &Vcb->CountResource, TRUE );
        
            Vcb->ReferenceCount++;
            Vcb->OpenHandleCount++;
        
            ExReleaseResourceForThreadLite(
                &Vcb->CountResource,
                ExGetCurrentResourceThread() );
        
            Irp->IoStatus.Information = FILE_OPENED;
    }
  
errorout:
      
    return Status;
}


NTSTATUS
Ext2Create (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT      DeviceObject;
    PIRP                Irp;
    PIO_STACK_LOCATION  io_stack;
    PEXT2_VCB           Vcb = 0;
    NTSTATUS            Status = STATUS_OBJECT_NAME_NOT_FOUND;
    PEXT2_FCBVCB        Xcb = NULL;

    DeviceObject = IrpContext->DeviceObject;

    Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
    
    Irp = IrpContext->Irp;
    
    io_stack = IoGetCurrentIrpStackLocation(Irp);

    Xcb = (PEXT2_FCBVCB) (io_stack->FileObject->FsContext);
    
    if (DeviceObject == gExt2Global->DeviceObject)
    {
        Ext2DbgPrint(D_CREATE, "Ext2Create: Create on main device object.\n");
        
        Irp->IoStatus.Information = FILE_OPENED;
        Status = STATUS_SUCCESS;
        IrpContext->Irp->IoStatus.Status = Status;
        
        Ext2CompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        
        Ext2FreeIrpContext(IrpContext);
        
        return Status;
        
    }
   
    __try
    {
        if (IsFlagOn(Vcb->Flags, VCB_VOLUME_LOCKED))
        {
            Status = STATUS_ACCESS_DENIED;
            __leave;
        }

        if (((io_stack->FileObject->FileName.Length == 0) && (io_stack->FileObject->RelatedFileObject == NULL) ) || 
                  (Xcb && Xcb->Identifier.Type == EXT2VCB) )
        {
            Status = Ext2CreateVolume(IrpContext, Vcb);
        }
        else
        {
            Status = Ext2CreateFile(IrpContext, Vcb);
        }
    }

    __finally
    {

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
Ext2CreateInode(
    PEXT2_IRP_CONTEXT   IrpContext,
    PEXT2_VCB           Vcb,
    PEXT2_FCB           pParentFcb,
    ULONG               Type,
    ULONG               FileAttr,
    PUNICODE_STRING     FileName)
{
    ULONG       Inode;
    ULONG       Group;

    EXT2_INODE  Ext2Ino;

    RtlZeroMemory(&Ext2Ino, sizeof(EXT2_INODE));

    Group = (pParentFcb->Ext2Mcb->Inode - 1) / Vcb->ext2_super_block->s_blocks_per_group;

    Ext2DbgPrint(D_CREATE, "Ext2CreateInode: %S in %S(Inode=%xh)\n", FileName->Buffer, pParentFcb->Ext2Mcb->ShortName.Buffer, pParentFcb->Ext2Mcb->Inode);

    if (Ext2NewInode(IrpContext, Vcb, Group,Type, &Inode))
    {
        if (NT_SUCCESS(Ext2AddEntry(IrpContext, Vcb, pParentFcb, Type, Inode, FileName)))
        {
            Ext2SaveInode(IrpContext, Vcb, pParentFcb->Ext2Mcb->Inode, pParentFcb->ext2_inode);

            Ext2Ino.i_ctime = pParentFcb->ext2_inode->i_mtime;
            Ext2Ino.i_mode = 0x1FF;
            Ext2Ino.i_links_count = 1;

            if (FlagOn(FileAttr, FILE_ATTRIBUTE_READONLY))
            {
                Ext2SetReadOnly(Ext2Ino.i_mode);
            }

            if (Type == EXT2_FT_DIR)
            {
                SetFlag(Ext2Ino.i_mode, S_IFDIR);
                if ((pParentFcb->Ext2Mcb->Inode == EXT2_ROOT_INO) && (FileName->Length == 0x10))
                {
                    if (memcmp(FileName->Buffer, L"Recycled\0", 0x10) == 0)
                    {
                        Ext2SetReadOnly(Ext2Ino.i_mode);
                    }
                }
            }
            else
            {
                SetFlag(Ext2Ino.i_mode, S_IFFIL);
            }

            Ext2SaveInode(IrpContext, Vcb, Inode, &Ext2Ino);

            Ext2DbgPrint(D_CREATE, "Ext2CreateInode: New Inode = %xh (Type=%xh)\n", Inode, Type);
            
            return STATUS_SUCCESS;
        }
        else
        {
            Ext2DbgBreakPoint();
            Ext2FreeInode(IrpContext, Vcb, Inode, Type);
        }
    }

    return STATUS_UNSUCCESSFUL;
}