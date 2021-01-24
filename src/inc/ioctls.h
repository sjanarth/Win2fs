
 /*                                                                            
  *		Copyright (c) 2001 - 2003 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\inc\ioctls.h
  *                                                                            
  *		Abstract:		Device IO Control defines.
  *
  *		Notes:			None  
  *
  *		Revision History:
  *
  *		Date		Version		Author				Change Log
  *		------------------------------------------------------------------------
  *
  *					0.0.1		Satish Kumar J		Initial Version
  */                          		
 								           
 #ifndef	__IOCTLS_H
 #define	__IOCTLS_H

 ///////////////////////////////////////////////////////////////////////////////

 // Includes.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 #pragma pack(1)

 #ifndef _NTDDK_
	#define FILE_DEVICE_UNKNOWN				0x00000022
	#define METHOD_NEITHER                  3 
	#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
		((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
	)
 #endif	//	_NTDDK_

 #define IOCTL_GET_SUPER	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_NEITHER, FILE_WRITE_DATA) 
 #define IOCTL_GET_INODE	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_NEITHER, FILE_WRITE_DATA) 
 #define IOCTL_SET_INODE	\
		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_NEITHER, FILE_READ_DATA|FILE_WRITE_DATA) 

 #pragma pack()

 //////////////////////////////////////////////////////////////////////////////

 #endif		// __IOCTLS_H

