 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \FSD\except.c
  *
  *     Abstract:		Exception handler code.
  *
  *     Author:		    Satish Kumar J
  *
  *		Date:			09-JUL-2004
  *
  *		Notes:			None.
  *
  *		Revision History:
  *
  *		Date		Version	Author				Changes
  *		------------------------------------------------------------------------
  *
  *		09-JUL-04	1.00	Satish Kumar J		Initial	Version.
  *
  *
  */

 //////////////////////////////////////////////////////////////////////////////

 // Include files.

 #include "ntifs.h"
 #include "ext2fs.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global data.

 extern PEXT2_GLOBAL gExt2Global;

 //////////////////////////////////////////////////////////////////////////////

 // Static data.

 #ifdef ALLOC_PRAGMA
	#pragma alloc_text(PAGE, Ext2ExceptionFilter)
	#pragma alloc_text(PAGE, Ext2ExceptionHandler)
 #endif

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 NTSTATUS
 Ext2ExceptionFilter (
	IN PEXT2_IRP_CONTEXT    IrpContext,
    IN NTSTATUS             ExceptionCode)
 {
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    //
    // Only use a valid IrpContext
    //

    if (IrpContext)
    {
        if ((IrpContext->Identifier.Type != EXT2ICX) ||
            (IrpContext->Identifier.Size != sizeof(EXT2_IRP_CONTEXT)))
        {
            IrpContext = NULL;
        }
    }

    //
    //  For the purposes of processing this exception, let's mark this
    //  request as being able to wait, and neither write through nor on
    //  removable media if we aren't posting it.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    //
    // If the exception is expected execute our handler
    //

    if (FsRtlIsNtstatusExpected(ExceptionCode))
    {
        Ext2DbgPrint(D_EXCEPT, "Ext2ExceptionFilter: Catching exception %#x\n",  ExceptionCode);
        
        Status = EXCEPTION_EXECUTE_HANDLER;
        
        if (IrpContext)
        {
            IrpContext->ExceptionInProgress = TRUE;
            IrpContext->ExceptionCode = ExceptionCode;
        }
    }

    //
    // else continue search for an higher level exception handler
    //

    else
    {
        Ext2DbgPrint(D_EXCEPT, "Ext2ExceptionFilter: Passing on exception %#x\n", ExceptionCode);
        
        Status = EXCEPTION_CONTINUE_SEARCH;
        
        if (IrpContext)
        {
            Ext2FreeIrpContext(IrpContext);
        }
    }
    
    return Status;
 }

 NTSTATUS 
 Ext2ExceptionHandler (
	IN PEXT2_IRP_CONTEXT IrpContext)
 {
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    
    if (IrpContext)
    {
        Status = IrpContext->ExceptionCode;
        
        if (IrpContext->Irp)
        {
            IrpContext->Irp->IoStatus.Status = Status;
            
            Ext2CompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
        }
        
        Ext2FreeIrpContext(IrpContext);
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return Status;
 }

 //////////////////////////////////////////////////////////////////////////////