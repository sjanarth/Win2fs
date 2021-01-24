/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             devctl.c
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
Ext2DeviceControlCompletion (IN PDEVICE_OBJECT   DeviceObject,
                IN PIRP             Irp,
                IN PVOID            Context);


#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, Ext2DeviceControlCompletion)
#pragma alloc_text(PAGE, Ext2DeviceControl)
#pragma alloc_text(PAGE, Ext2DeviceControlNormal)
#pragma alloc_text(PAGE, Ext2DeviceControl)
#endif


NTSTATUS
Ext2DeviceControlCompletion (IN PDEVICE_OBJECT   DeviceObject,
                 IN PIRP             Irp,
                 IN PVOID            Context)
{
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

#if DBG 
    Ext2DbgPrint(D_DEVCTL, DRIVER_NAME ": %-16.16s %-31s *** Status: %s (%#x) ***\n",
        PsGetCurrentProcess()->ImageFileName,
        "IRP_MJ_DEVICE_CONTROL",
        Ext2DbgNtStatusToString(Irp->IoStatus.Status),
        Irp->IoStatus.Status );
#endif
    
    return STATUS_SUCCESS;
}


NTSTATUS
Ext2DeviceControlNormal (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    BOOLEAN         CompleteRequest = TRUE;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;

    PEXT2_VCB       Vcb;
    PEXT2_CCB       Ccb;

    PIRP            Irp;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;

    PDEVICE_OBJECT  TargetDeviceObject;
    
    __try
    {
		__try
		{
			ASSERT(IrpContext != NULL);
        
			ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
				(IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
        
			CompleteRequest = TRUE;

			DeviceObject = IrpContext->DeviceObject;
    
			if (DeviceObject == gExt2Global->DeviceObject)
			{
				Status = STATUS_INVALID_DEVICE_REQUEST;
            
				__leave;
			}
        
			Irp = IrpContext->Irp;
			IrpSp = IoGetCurrentIrpStackLocation(Irp);

			Vcb = (PEXT2_VCB) IrpSp->FileObject->FsContext;
			Ccb = (PEXT2_CCB) IrpSp->FileObject->FsContext2;

			if (!(Vcb && 
				  ( (Vcb->Identifier.Type == EXT2VCB) && (Vcb->Identifier.Size == sizeof(EXT2_VCB))) &&
				  Ccb) )
			{

				Status = STATUS_INVALID_PARAMETER;
				__leave;
			}
        
			TargetDeviceObject = Vcb->TargetDeviceObject;
        
			//
			// Pass on the IOCTL to the driver below
			//
        
			CompleteRequest = FALSE;

			NextIrpSp = IoGetNextIrpStackLocation( Irp );
			*NextIrpSp = *IrpSp;

			IoSetCompletionRoutine(
				Irp,
				Ext2DeviceControlCompletion,
				NULL,
				FALSE,
				TRUE,
				TRUE );
        
			Status = IoCallDriver(TargetDeviceObject, Irp);
		}	
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			Status = FALSE;
		}
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

            if (IrpContext)
			{
                Ext2FreeIrpContext(IrpContext);
			}
        }
    }
    
    return Status;
}

NTSTATUS
Ext2GetSuperBlock (IN PEXT2_IRP_CONTEXT IrpContext)
{
	PIRP                Irp					= NULL;
	PIO_STACK_LOCATION  IoStackLocation		= NULL;
	PDEVICE_OBJECT      DeviceObject		= NULL;
	PEXT2_VCB			Vcb					= NULL;

    PCHAR				pOutputBuffer		= NULL;
	ULONG				dwDataWritten		= 0;
    ULONG				dwDataSize			= sizeof(EXT2_SUPER_BLOCK);
    NTSTATUS			Status				= STATUS_UNSUCCESSFUL;

	ENTER (D_DEVCTL, "Ext2GetSuperBlock");    
    
	__try	
	{
		__try
		{
			ASSERT(IrpContext != NULL);
			ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
					(IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

			Irp = IrpContext->Irp;
			DeviceObject = IrpContext->DeviceObject;

			ASSERT (Irp != NULL);
			ASSERT (DeviceObject != NULL);

			//
			// This request is not allowed on the main device object
			//
			if (DeviceObject == gExt2Global->DeviceObject)
			{
				Ext2DbgPrint(D_DEVCTL, "Cannot allow on the main device object, returning.");
				Status = STATUS_INVALID_DEVICE_REQUEST;
				__leave;
			}

			//
			//	It should be a VCB.
			//        
			Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        
			ASSERT(Vcb != NULL);
			ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
				(Vcb->Identifier.Size == sizeof(EXT2_VCB)));

			//
			// Get the User mode memmory where we need to dump the super block.
			//
			pOutputBuffer = (PCHAR) Irp->UserBuffer;

			if(pOutputBuffer == NULL)
			{
				Ext2DbgPrint(D_DEVCTL, "Output buffer NULL, returning.");

				Status = STATUS_INVALID_PARAMETER;

				__leave;
			}

			IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

			ProbeForWrite(
					pOutputBuffer, 
					IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength,
					1
				);

			if(IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength >= dwDataSize)
			{	
				Ext2DbgPrint(D_DEVCTL, "Copying data.");
				RtlCopyMemory(pOutputBuffer, Vcb->ext2_super_block, dwDataSize);
				dwDataWritten = dwDataSize;
				Status = STATUS_SUCCESS;
			}
			else
			{
				Ext2DbgPrint(D_DEVCTL, "Output buffer too small (need %d bytes, got %d bytes), returning.",
							dwDataSize,
							IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);

				dwDataWritten = dwDataSize;

				Status = STATUS_BUFFER_TOO_SMALL;
			}
		}	
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			Status = FALSE;
		}
	}
	__finally
	{
		if (!IrpContext->ExceptionInProgress)
		{
			IrpContext->Irp->IoStatus.Status = Status;
			IrpContext->Irp->IoStatus.Information = dwDataWritten;

			Ext2CompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        
			Ext2FreeIrpContext(IrpContext);
		}
	}

	LEAVE3 (D_DEVCTL, "Ext2GetSuperBlock");

	return Status;
}

NTSTATUS
Ext2GetInodeForFile (
	IN		PEXT2_IRP_CONTEXT	IrpContext,
	IN		PFILE_OBJECT		FileObject,
	IN OUT	ULONG*				dwDataWritten
	)
{
	PIRP                Irp					= NULL;
	PIO_STACK_LOCATION  IoStackLocation		= NULL;
	PEXT2_FCB			Fcb					= NULL;

    PCHAR				pOutputBuffer		= NULL;
    ULONG				dwDataSize			= sizeof(EXT2_INODE);
    NTSTATUS			Status				= STATUS_UNSUCCESSFUL;

	ENTER (D_DEVCTL, "Ext2GetInodeForFile");

	__try
	{
		ASSERT (IrpContext != NULL);
		ASSERT (FileObject != NULL);

		//	It should be a FCB.

        Fcb = (PEXT2_FCB) FileObject->FsContext;
        
        ASSERT(Fcb != NULL);

        ASSERT((Fcb->Identifier.Type == EXT2FCB) &&
               (Fcb->Identifier.Size == sizeof(EXT2_FCB)));

		// Get the User mode memmory where we need to dump the super block.

		Irp = IrpContext->Irp;

		ASSERT (Irp != NULL);

		pOutputBuffer = (PCHAR) Irp->UserBuffer;

		if(pOutputBuffer == NULL)
		{
			Ext2DbgPrint(D_DEVCTL, "Output buffer NULL, returning.");

			Status = STATUS_INVALID_PARAMETER;

			__leave;
		}

		IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

		ProbeForWrite(
				pOutputBuffer, 
				IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength,
			    1
			);

		if(IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength >= dwDataSize)
		{	
			Ext2DbgPrint(D_DEVCTL, "Copying data.");

			// Over loading the reserved field with the inode number.

			Fcb->ext2_inode->osd1.linux1.l_i_reserved1 = Fcb->Ext2Mcb->Inode ;

			RtlCopyMemory(pOutputBuffer, Fcb->ext2_inode, dwDataSize);

			if (dwDataWritten != NULL)
			{
				(*dwDataWritten) = dwDataSize;
			}

			Status = STATUS_SUCCESS;
		}
		else
		{
			Ext2DbgPrint(D_DEVCTL, "Output buffer too small (need %d bytes, got %d bytes), returning.",
						dwDataSize,
						IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);

			if (dwDataWritten != NULL)
			{
				(*dwDataWritten) = dwDataSize;
			}

			Status = STATUS_BUFFER_TOO_SMALL;
		}
	}
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
		Status = FALSE;
    }

	LEAVE3 (D_DEVCTL, "Ext2GetInodeForFile");

	return Status;
}

NTSTATUS
Ext2GetInodeForDir (
	IN		PEXT2_IRP_CONTEXT	IrpContext,
	IN OUT	ULONG*				dwDataWritten
	)
{
	PIRP                Irp					= NULL;
	PIO_STACK_LOCATION  IoStackLocation		= NULL;
    PEXT2_VCB           Vcb					= NULL;
    PEXT2_MCB           Ext2Mcb				= NULL;
    PEXT2_INODE         ext2_inode			= NULL;
	BOOLEAN				FreeStrings			= FALSE;

	PCHAR				pInputBuffer		= NULL;    
	PCHAR				pOutputBuffer		= NULL;
    ULONG				dwDataSize			= sizeof(EXT2_INODE);
    NTSTATUS			Status				= STATUS_UNSUCCESSFUL;

	ANSI_STRING			FileName;
	UNICODE_STRING		UnicodeFileName;

	ENTER (D_DEVCTL, "Ext2GetInodeForDir");

	__try
	{
		__try
		{
			ASSERT (IrpContext != NULL);

			Irp = IrpContext->Irp;

			ASSERT (Irp != NULL);

			IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

			pInputBuffer = IoStackLocation->Parameters.DeviceIoControl.Type3InputBuffer;

			if (pInputBuffer == NULL)
			{
				Ext2DbgPrint(D_DEVCTL, "Input buffer NULL, returning.");

				Status = STATUS_INVALID_PARAMETER;

				__leave;
			}

			Ext2DbgPrint (D_DEVCTL, "Ext2GetInodeForDir, pInputBuffer=%s\r\n\r\n", pInputBuffer);	

			pOutputBuffer = (PCHAR) Irp->UserBuffer;

			if (pOutputBuffer == NULL)
			{
				Ext2DbgPrint(D_DEVCTL, "\r\n		Output buffer NULL, returning.");

				Status = STATUS_INVALID_PARAMETER;

				__leave;
			}

			if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < dwDataSize)
			{
				Ext2DbgPrint(D_DEVCTL, "Output buffer too small (need %d bytes, got %d bytes), returning.",
							dwDataSize,
							IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);

				if (dwDataWritten != NULL)
				{
					(*dwDataWritten) = dwDataSize;
				}

				Status = STATUS_BUFFER_TOO_SMALL;

				__leave;
			}

			Ext2DbgPrint (D_DEVCTL, "Input and output buffers validated.");

			ASSERT (IrpContext->DeviceObject != NULL);

			Vcb = (PEXT2_VCB) IrpContext->DeviceObject->DeviceExtension;

			ASSERT (Vcb != NULL);

			RtlInitAnsiString (&FileName, pInputBuffer);
			RtlAnsiStringToUnicodeString (&UnicodeFileName, &FileName, TRUE);

			FreeStrings = TRUE;

			ext2_inode = (PEXT2_INODE) ExAllocatePool (PagedPool, sizeof(EXT2_INODE));

			if (NULL == ext2_inode)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;

				__leave;
			}

			Ext2DbgPrint (D_DEVCTL, "Calling Ext2LookupFileName ... ");

			Status = Ext2LookupFileName(
							Vcb,
							&UnicodeFileName,
							NULL,
							&Ext2Mcb,
							ext2_inode 
						);

			if (!NT_SUCCESS(Status))
			{
				Ext2DbgPrint (D_DEVCTL, "Ext2LookupFileName returned %s, returning", Ext2DbgNtStatusToString(Status));
				
				__leave;
			}

			Ext2DbgPrint (D_DEVCTL, "Got the inode.");

			// Ok, we got the Inode.

			ProbeForWrite(
					pOutputBuffer, 
					IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength,
					1
				);

			Ext2DbgPrint (D_DEVCTL, "Copying data.");

			// Over loading the reserved field with the inode number.

			ext2_inode->osd1.linux1.l_i_reserved1 = Ext2Mcb->Inode ;

			RtlCopyMemory(pOutputBuffer, ext2_inode, dwDataSize);

			if (dwDataWritten != NULL)
			{
				(*dwDataWritten) = dwDataSize;
			}

			Status = STATUS_SUCCESS;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			Status = FALSE;
		}
	}
	__finally
	{
		Ext2DbgPrint (D_DEVCTL, "Ext2GetInodeForDir returning with 0x%08lX.", Status);

		if (ext2_inode != NULL)
		{
			ExFreePool (ext2_inode);
		}

		if (FreeStrings)
		{
			RtlFreeAnsiString (&FileName);
			RtlFreeUnicodeString (&UnicodeFileName);
		}
	}

	LEAVE3 (D_DEVCTL, "Ext2GetInodeForDir");

	return Status;
}

NTSTATUS
Ext2GetInode (IN PEXT2_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT      DeviceObject		= NULL;
	PFILE_OBJECT        FileObject			= NULL;

	ULONG				dwDataWritten		= 0;
    NTSTATUS			Status				= STATUS_UNSUCCESSFUL;

	ENTER (D_DEVCTL, "Ext2GetInode");    
    
	__try	
	{
		__try
		{
			ASSERT(IrpContext != NULL);
			ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
				   (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

			DeviceObject = IrpContext->DeviceObject;

			ASSERT (DeviceObject != NULL);

			//
			// This request is not allowed on the main device object
			//

			if (DeviceObject == gExt2Global->DeviceObject)
			{
				Ext2DbgPrint(D_DEVCTL, "Cannot allow on the main device object, returning.");

				Status = STATUS_INVALID_DEVICE_REQUEST;

				__leave;
			}

			FileObject = IrpContext->FileObject;

			ASSERT (FileObject != NULL);

			if (FileObject->FileName.Length != 0)
			{
				Ext2DbgPrint(D_DEVCTL, "FileName: %S (Calling Ext2GetInodeForFile) \r\n", FileObject->FileName.Buffer);

				Status = Ext2GetInodeForFile (IrpContext, FileObject, &dwDataWritten);

				__leave;
			}

			Ext2DbgPrint(D_DEVCTL, "FileName is null, calling Ext2GetInodeForDir \r\n");

			Status = Ext2GetInodeForDir (IrpContext, &dwDataWritten);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			Status = FALSE;
		}
	}
	__finally
	{
		if (!IrpContext->ExceptionInProgress)
		{
			IrpContext->Irp->IoStatus.Status = Status;
			IrpContext->Irp->IoStatus.Information = dwDataWritten;

			Ext2CompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        
			Ext2FreeIrpContext(IrpContext);
		}
	}

	LEAVE3 (D_DEVCTL, "Ext2GetInode");    

	return Status;
}

NTSTATUS
Ext2SetInode (IN PEXT2_IRP_CONTEXT IrpContext)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

	ENTER (D_DEVCTL, "Ext2SetInode");  

	//
	// TODO: Need to implement this fully.
	// 
	
	ASSERT (IrpContext != NULL);

	IrpContext->Irp->IoStatus.Status = Status;
	IrpContext->Irp->IoStatus.Information = 0;;

	Ext2CompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);

	Ext2FreeIrpContext(IrpContext);

	LEAVE3 (D_DEVCTL, "Ext2SetInode");  

	return Status;
}

NTSTATUS
Ext2DeviceControl (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;
    ULONG               IoControlCode;
    NTSTATUS            Status;

	ENTER (D_DEVCTL, "Ext2DeviceControl");
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
        (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));
    
    Irp = IrpContext->Irp;
    
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    
    IoControlCode =
        IoStackLocation->Parameters.DeviceIoControl.IoControlCode;

	Ext2DbgPrint (D_DEVCTL, "IoControlCode=0x%08lX", IoControlCode);
    
    switch (IoControlCode)
    {
		case IOCTL_GET_SUPER:
			Ext2DbgPrint (D_DEVCTL, "Ext2DeviceControl: Calling Ext2GetSuperBlock");
			Status = Ext2GetSuperBlock (IrpContext);
			break;
		case IOCTL_GET_INODE:
			Ext2DbgPrint (D_DEVCTL, "Ext2DeviceControl: Calling Ext2GetInode");
			Status = Ext2GetInode (IrpContext);
			break;
		case IOCTL_SET_INODE:
			Ext2DbgPrint (D_DEVCTL, "Ext2DeviceControl: Calling Ext2SetInode");
			Status = Ext2SetInode (IrpContext);
			break;
		default:
			Ext2DbgPrint (D_DEVCTL, "Ext2DeviceControl: Calling Ext2DeviceControlNormal");
			Status = Ext2DeviceControlNormal(IrpContext);
    }
    
	LEAVE3 (D_DEVCTL, "Ext2DeviceControl");

    return Status;
}
