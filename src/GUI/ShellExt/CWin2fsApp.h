 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CWin2fsApp.h
  *
  *     Abstract:		Header for the Application class.
  *
  *     Author:		    Satish Kumar J
  *
  *		Date:			20-NOV-2005
  *
  *		Notes:			None.
  *
  *		Revision History:
  *
  *		Date		Version	Author				Changes
  *		------------------------------------------------------------------------
  *
  *		20-NOV-05	1.00	Satish Kumar J		Initial	Version.
  *
  *
  */

 #ifndef	__CWIN2FSAPP_H
 #define	__CWIN2FSAPP_H

 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "resource.h"

 //////////////////////////////////////////////////////////////////////////////

 // Macros and typedefs.
 
 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Externals.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static declarations.

 // None.

 //////////////////////////////////////////////////////////////////////////////
 
 // Global declarations.

 class CWin2fsApp : public CWinApp
 {
	public:

		CWin2fsApp();

		virtual BOOL InitInstance();

		DECLARE_MESSAGE_MAP()
 };

 /////////////////////////////////////////////////////////////////////////////
 // Registering / Unregistering code.

 HRESULT RegisterShellExtension( REFGUID guidClass );
 HRESULT UnregisterShellExtension( REFGUID guidClass );

 template<class OLECREATE> 
 HRESULT RegisterShellExtension( )
 {
	return RegisterShellExtension( OLECREATE::guid );
 }

 template<class OLECREATE> 
 HRESULT UnregisterShellExtension( )
 {
	return UnregisterShellExtension( OLECREATE::guid );
 }

 //////////////////////////////////////////////////////////////////////////////

 #endif		//	__CWIN2FSAPP_H