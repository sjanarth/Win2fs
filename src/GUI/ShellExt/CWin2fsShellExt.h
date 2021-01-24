 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CWin2fsShellExt.h
  *
  *     Abstract:		Header for the Win2fs Shell Extention class.
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

 #ifndef	__CWIN2FSSHELLEX_H
 #define	__CWIN2FSSHELLEX_H

 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include <ShlObj.h> // Shell extension interfaces

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

 class CWin2fsShellExt : public CCmdTarget
 {
	public:

		virtual void OnFinalRelease();

	protected:

		CWin2fsShellExt();
		virtual ~CWin2fsShellExt();

		CString m_strFile;

	DECLARE_DYNCREATE(CWin2fsShellExt)
	DECLARE_OLECREATE(CWin2fsShellExt)
	
	DECLARE_MESSAGE_MAP()
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()    
	
	BEGIN_INTERFACE_PART(ShellExtInit, IShellExtInit)
        STDMETHOD(Initialize)(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID );
	END_INTERFACE_PART(ShellExtInit)
    
	BEGIN_INTERFACE_PART(ShellPropSheetExt, IShellPropSheetExt)
		STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
		STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);
	END_INTERFACE_PART(ShellPropSheetExt)
 };

 //////////////////////////////////////////////////////////////////////////////

 #endif		//	 __CWIN2FSSHELLEX_H