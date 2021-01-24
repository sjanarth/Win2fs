 
 /*
  *		Copyright (c) 2001 - 2003 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *
  *		Module Name:	\Win2fs.h
  *
  *     Abstract:		Common type defintions and includes used by Win2fs.
  *
  *		Notes:			None.
  *
  *		Revision History:
  *		
  *		Date		Version		Author			Changes
  *		-------------------------------------------------------------------------
  *
  *					0.0.1		Satish Kumar	Initial Version
  *
  */
 
 #ifndef	__WIN2FS_H
 #define	__WIN2FS_H

 ////////////////////////////////////////////////////////////////////////////////

 // Include files.

 // Standard 'C' Runtime lib.

 #include <memory.h>	
 #include <string.h>

 ////////////////////////////////////////////////////////////////////////////////
 
 // Macros and typedefs'.

 // Win2fs basic types.

 #ifdef	 _MSC_VER

 #ifndef __WIN2FS_TYPES
 #define __WIN2FS_TYPES

	typedef signed	char		__S8, *__PS8;	
	typedef unsigned char		__U8, *__PU8;

	typedef signed short		__S16, *__PS16;
	typedef unsigned short		__U16, *__PU16;

	typedef signed int			__S32, *__PS32;
	typedef unsigned int		__U32, *__PU32;

	typedef signed __int64		__S64, *__PS64;
	typedef unsigned __int64	__U64, *__PU64;

 #ifndef _WINDEF_

	#ifndef DRIVER

		typedef void			VOID, *PVOID;
		typedef const __PU8		LPCTSTR;
		typedef __PU8			LPSTR, PSZ;

	#endif

	typedef unsigned char		BOOL, *PBOOL;

	#define	TRUE				1
	#define FALSE				0

 #endif		//	_WINDEF_

 #endif		//  __WIN2FS_TYPES

 #endif		//	_MSC_VER

 // Win2fs Standard Macros.

 #ifndef MAX
	#define MAX(a,b)		(((a) > (b)) ? (a) : (b))
 #endif

 #ifndef MIN
	#define MIN(a,b)		(((a) < (b)) ? (a) : (b))
 #endif

 #define MAKE16(u8u, u8l)	((__U16)(((__U8)(u8u) << 8)) | ((__U16)((__U8)(u8l))))
 #define MAKE32(u16u, u16l)	((__U32)(((__U16)(u16u) << 16)) | ((__U32)((__U16)(u16l))) )

 #define LO8(u16v)			((__U8)(u16v))
 #define HI8(u16v)			((__U8)(((__U16)(u16v) >> 8) & 0x00FF))
 #define LO16(u32v)			((__U16)(u32v))
 #define HI16(u32v)			((__U16)(((__U32)(u32v) >> 16) & 0xFFFF))
 
 #define isdigit(u8c)		(((u8c) <= '9') && ((u8c) >= '0'))
 #define isupper(u8c)		(((u8c) <= 'Z') && ((u8c) >= 'A'))
 #define islower(u8c)		(((u8c) <= 'z') && ((u8c) >= 'a'))
 #define isalpha(u8c)		(isupper(u8c) || islower(u8c))
 #define toupper(u8c)		(islower(u8c)?u8c-32:u8c);
 #define tolower(u8c)		(isupper(u8c)?u8c+32:u8c);

 // do-while Macros.

 #define BREAK_IF_EQUAL(val1,val2)		if ((val2) == (val1)) { break; }
 #define BREAK_IF_NOTEQUAL(val1,val2)	if ((val2) != (val1)) { break; }
 
 #define BREAK_IF_NULL(ptr)				BREAK_IF_EQUAL ((ptr), NULL)
 #define BREAK_IF_ZERO(val)				BREAK_IF_EQUAL ((val), 0)
 #define BREAK_IF_TRUE(val)				BREAK_IF_EQUAL ((val), TRUE)
 #define BREAK_IF_FALSE(val)			BREAK_IF_EQUAL ((val), FALSE)

 #ifdef DRIVER
	#define BREAK_IF_FAILED(NtStatus)	if (!NT_SUCCESS((NtStatus)))	{ break; }
 #else
	#define BREAK_IF_FAILED(hResult)	if (FAILED((hResult)))	{ break; }
 #endif
 
 #define ARRAYSIZE(arr)					( sizeof ( (arr) ) / sizeof ( (arr)[0] ) )
	 
 #define SAFE_DELETE(Obj)				if ((Obj) != NULL) { delete (Obj); Obj = NULL; }	 
 #define SAFE_RELEASE(Obj)				if ((Obj) != NULL) { (Obj)->Release (); }	 
 #define SAFE_DELETE_ARRAY(arr)			if (arr != NULL) { delete[] arr; arr = NULL; }

 #define SAFE_CLOSE_HANDLE(h)			if (INVALID_HANDLE_VALUE != (h)) {	\
											CloseHandle (h);				\
											h = INVALID_HANDLE_VALUE;		\
										}

 #define ISWIN98()						( ( GetVersion ( ) ) & 0x80000000 ) 

 ////////////////////////////////////////////////////////////////////////////////
 
 #pragma intrinsic (memcpy,memcmp,strcpy,strlen,strcat,strcmp,memset)

 #endif		// __WIN2FS_H
