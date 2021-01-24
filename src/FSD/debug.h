 /*                                                                            
  *		Copyright (c) 2001 - 2010 Satish Kumar Janarthanan (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\FSD\Debug.h
  *                                                                            
  *		Abstract:		Header for the debugging routines.
  *
  *		Notes:			
  *
  *			o	Each Ext2DbgPrint call must be on a single line.
  *
  *			o	The DEBUG flag must be defined for these routines to be compiled.
  *
  *		Revision History:
  *
  *		Date		Version		Author				Change Log
  *		------------------------------------------------------------------------
  *
  *					0.0.1		Satish Kumar J		Initial Version
  */                          		

 #ifndef	__DEBUG_H
 #define	__DEBUG_H

 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "ntifs.h"
 #include "Win2fs.h"

 //////////////////////////////////////////////////////////////////////////////

 // Macros and typedefs.
 
 #define DOUT_CONSOLE   1		    // Dumps to console. 
 #define DOUT_FILE	    2	        // dumps to a file.

 // Any changes made here should also be reflected in debug.c

 #define D_ALWAYS		0x00000000
 #define D_ERROR		0x00000001
 
 #define D_BLOCK		0x00000002
 #define D_CLEANUP		0x00000004
 #define D_CLOSE		0x00000008
 #define D_CMCB			0x00000010	
 #define D_CREATE		0x00000020
 #define D_DEVCTL		0x00000040
 #define D_DIRCTL		0x00000080
 #define D_DISPATCH		0x00000100
 #define D_EXCEPT		0x00000200
 #define D_EXT2			0x00000400
 #define D_FASTIO		0x00000800
 #define D_FILEINFO		0x00001000
 #define D_FSCTL		0x00002000
 #define D_INIT			0x00004000
 #define D_LOCK			0x00008000
 #define D_MEMORY		0x00010000
 #define D_READ			0x00020000
 #define D_SHUTDOWN		0x00040000
 #define D_UTIL			0x00080000
 #define D_VOLINFO		0x00100000
 #define D_WRITE		0x00200000

 #define D_END			D_WRITE
 #define D_ALL		    0xFFFFFFFF

 //////////////////////////////////////////////////////////////////////////////

 // Externals.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static declarations.

 // None.

 //////////////////////////////////////////////////////////////////////////////
 
 // Global declarations.

 #ifdef DBG

	#define	ENTER(Flag,API)		Ext2DbgPrint(Flag, "ENTER %s", API);
	#define LEAVE(Flag,API)		Ext2DbgPrint(Flag, "LEAVE %s", API);
	#define LEAVE2(Flag,API)	Ext2DbgPrint(Flag, "LEAVE %s (returning %s)",		\
													API, (bRet?:"true":"false"));
	#define LEAVE3(Flag,API)	Ext2DbgPrint(Flag, "LEAVE %s (returning %08X, %s)", \
													API, Status, Ext2DbgNtStatusToString(Status));

	#define Ext2DbgGetCurrentProcessName() (					\
		 (PUCHAR) PsGetCurrentProcess() + gProcessNameOffset	\
	)

	extern	__U32 gProcessNameOffset;

    BOOL	Ext2DbgInitialize		( __U32 out, PSZ debugFileName );
    __U32	Ext2DbgSetOut			( __U32 newOut, PSZ debugFileName );
    __U32   Ext2DbgPrint			( __U32 flag, PSZ format, ... );
	__U32	Ext2DbgPrintCall		( __U32 flag, PDEVICE_OBJECT DeviceObject, PIRP Irp );
	__U32	Ext2DbgPrintComplete	( PIRP Irp );	
	PSZ		Ext2DbgNtStatusToString ( NTSTATUS Status );
    VOID	Ext2DbgBreakPoint		( );
    VOID	Ext2DbgCleanup			( );

 #else  

	#define	ENTER(Flag,API)					//
	#define LEAVE(Flag,API)					//
	#define LEAVE2(Flag,API)				//
	#define LEAVE3(Flag,API)				//

	#define Ext2DbgGetCurrentProcessName	//

    #define Ext2DbgInitialize				//
    #define Ext2DbgSetOut  					//
    #define Ext2DbgPrint					//
	#define	Ext2DbgPrintCall				//
	#define Ext2DbgNtStatusToString			//
	#define Ext2DbgBreakPoint				//
    #define Ext2DbgCleanup					//

 #endif
 
 //////////////////////////////////////////////////////////////////////////////

 #endif		// __DEBUG_H 