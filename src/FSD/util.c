 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \FSD\misc.c
  *
  *     Abstract:		Miscellaneous code.
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
	#pragma alloc_text(PAGE, Ext2SysTime)
	#pragma alloc_text(PAGE, Ext2InodeTime)
	#pragma alloc_text(PAGE, Ext2CharToWchar)
	#pragma alloc_text(PAGE, Ext2WcharToChar)
 #endif

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 LARGE_INTEGER
 Ext2SysTime (
	IN ULONG i_time)
 {
    LARGE_INTEGER SysTime;

    SysTime.QuadPart = (LONGLONG)(i_time) * 10000000;
    SysTime.QuadPart += gExt2Global->TimeZone.QuadPart;

    return SysTime;
 }

 ULONG
 Ext2InodeTime (
	IN LARGE_INTEGER SysTime)
 {
    return ((ULONG)((SysTime.QuadPart - gExt2Global->TimeZone.QuadPart) / 10000000));
 }

 NTSTATUS
 Ext2CharToWchar (
	IN OUT PWCHAR   Destination,
    IN PCHAR        Source,
    IN ULONG        Length)
 {
    NTSTATUS    Status	= STATUS_UNSUCCESSFUL;
    ULONG		Index	= 0;

    ANSI_STRING     AnsiString;
    UNICODE_STRING  UniString;

    ASSERT(Destination != NULL);
    ASSERT(Source != NULL);

    AnsiString.Length = AnsiString.MaximumLength = (USHORT)(Length);
    UniString.Length  = 0;
    UniString.MaximumLength  = (USHORT)(Length * 2 + 2);

    UniString.Buffer  = Destination;
    AnsiString.Buffer = Source;

    Status = RtlAnsiStringToUnicodeString(&UniString, &AnsiString, FALSE);

    if (!NT_SUCCESS(Status))
    {
        for (Index = 0; Index < Length; Index++)
        {
            Destination[Index] = (WCHAR) Source[Index];
        }

        Status = STATUS_SUCCESS;
    }

    return Status;
 }

 NTSTATUS
 Ext2WcharToChar (
	IN OUT PCHAR    Destination,
    IN PWCHAR       Source,
    IN ULONG        Length)
 {
    ULONG           Index	= 0;
    NTSTATUS        Status	= STATUS_UNSUCCESSFUL;

    ANSI_STRING     AnsiString;
    UNICODE_STRING  UniString;

    ASSERT(Destination != NULL);
    ASSERT(Source != NULL);

    AnsiString.Length = 0;
    AnsiString.MaximumLength = (USHORT)(Length + 1);
    UniString.Length  = UniString.MaximumLength  = (USHORT)(Length * 2);

    UniString.Buffer  = Source;
    AnsiString.Buffer = Destination;

    Status = RtlUnicodeStringToAnsiString(&AnsiString, &UniString, FALSE);
    
    if (!NT_SUCCESS(Status))
    {
        Status = STATUS_SUCCESS;

        for (Index = 0, Status = STATUS_SUCCESS; Index < Length; Index++)
        {
            Destination[Index] = (CHAR) Source[Index];
        
            //
            // Check that the wide character fits in a normal character
            // but continue with the conversion anyway
            //

            if ( ((WCHAR) Destination[Index]) != Source[Index] )
            {
                Status = STATUS_OBJECT_NAME_INVALID;
            }
        }

    }
    
    return Status;
 }

 //////////////////////////////////////////////////////////////////////////////