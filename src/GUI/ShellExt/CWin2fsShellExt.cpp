 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CWin2fsShellExt.cpp
  *
  *     Abstract:		The Win2fs Shell extension.
  *
  *     Author:		    Satish Kumar J
  *
  *		Date:			20-Dec-2005
  *
  *		Notes:			None.
  *
  *		Revision History:
  *
  *		Date		Version	Author				Changes
  *		------------------------------------------------------------------------
  *
  *		20-DEC-05	1.00	Satish Kumar J		Initial	Version.
  *
  *
  */

 //////////////////////////////////////////////////////////////////////////////

 // Library includes.

 #include "stdafx.h"

 //////////////////////////////////////////////////////////////////////////////

 // Private includes.

 #include "CWin2fsApp.h"
 #include "CWin2fsShellExt.h"

 #include "CWin2fsFilePage.h"
 #include "CWin2fsDrivePage.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global data.

 #ifndef IMPLEMENT_IUNKNOWN

	#define IMPLEMENT_IUNKNOWN_ADDREF(ObjectClass, InterfaceClass) \
		STDMETHODIMP_(ULONG) ObjectClass::X##InterfaceClass::AddRef(void) \
		{ \
			METHOD_PROLOGUE(ObjectClass, InterfaceClass); \
			return pThis->ExternalAddRef(); \
		}

	#define IMPLEMENT_IUNKNOWN_RELEASE(ObjectClass, InterfaceClass) \
		STDMETHODIMP_(ULONG) ObjectClass::X##InterfaceClass::Release(void) \
		{ \
			METHOD_PROLOGUE(ObjectClass, InterfaceClass); \
			return pThis->ExternalRelease(); \
		}

	#define IMPLEMENT_IUNKNOWN_QUERYINTERFACE(ObjectClass, InterfaceClass) \
		STDMETHODIMP ObjectClass::X##InterfaceClass::QueryInterface(REFIID riid, LPVOID *ppVoid) \
		{ \
			METHOD_PROLOGUE(ObjectClass, InterfaceClass); \
			return (HRESULT)pThis->ExternalQueryInterface(&riid, ppVoid); \
		}

	#define IMPLEMENT_IUNKNOWN(ObjectClass, InterfaceClass) \
		IMPLEMENT_IUNKNOWN_ADDREF(ObjectClass, InterfaceClass) \
		IMPLEMENT_IUNKNOWN_RELEASE(ObjectClass, InterfaceClass) \
		IMPLEMENT_IUNKNOWN_QUERYINTERFACE(ObjectClass, InterfaceClass)

 #endif		// IMPLEMENT_IUNKNOWN

 IMPLEMENT_DYNCREATE(CWin2fsShellExt, CCmdTarget);

 // {AA2CEE9F-6FC8-11D1-AEA2-204C4F4F5020}
 IMPLEMENT_OLECREATE(CWin2fsShellExt, "Win2fs", 
	0xaa2cee9f, 0x6fc8, 0x11d1, 0xae, 0xa2, 0x20, 0x4c, 0x4f, 0x4f, 0x50, 0x20);

 BEGIN_MESSAGE_MAP(CWin2fsShellExt, CCmdTarget)
 END_MESSAGE_MAP()

 BEGIN_DISPATCH_MAP(CWin2fsShellExt, CCmdTarget)
 END_DISPATCH_MAP()

 BEGIN_INTERFACE_MAP(CWin2fsShellExt, CCmdTarget)
	INTERFACE_PART(CWin2fsShellExt, IID_IShellExtInit, ShellExtInit)
	INTERFACE_PART(CWin2fsShellExt, IID_IShellPropSheetExt, ShellPropSheetExt)
 END_INTERFACE_MAP()

 //////////////////////////////////////////////////////////////////////////////

 // Static data.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 static HRESULT IsExt2fs (CString strPath)
 {
	 TRACE ("\nIsExt2fs: Path=%s", strPath);

	 CString strDrive = strPath.Left (3);

	 DWORD dwOmit = 0;
	 CHAR buf[MAX_PATH];

	 BOOL bRet = GetVolumeInformation (strDrive, NULL, 0, &dwOmit, &dwOmit, &dwOmit, buf, MAX_PATH);

	 if (bRet)
	 {
		 TRACE("\nIsExt2fs: FileSystemName=%s", buf);

		 if (strncmp (buf, "EXT2", 4) == 0)
		 {
			TRACE("\nIsExt2fs: returning S_OK.");

			return S_OK;
		 }
		 else
		 {
			TRACE("\nIsExt2fs: returning E_FAIL.");

			return E_FAIL;
		 }
	 }
	 else
	 {
		 TRACE("\nIsExt2fs: GetVolumeInformation failed, returning E_FAIL.");

		 return E_FAIL;
	 }
 }

 static BOOL IsDrive (CString path)
 {
	 return (3 == path.GetLength ());
 }

 //////////////////////////////////////////////////////////////////////////////

 // Restricted functions.

 // Privates'.

 // None.

 // Protected.

 // None.
 
 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 CWin2fsShellExt::CWin2fsShellExt()
 {
	
 }

 CWin2fsShellExt::~CWin2fsShellExt()
 {
 }

 void CWin2fsShellExt::OnFinalRelease()
 {
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
 }
	
 //////////////////////////////////////////////////////////////////////////////
 // 
 // IShellExtInit implementation

 IMPLEMENT_IUNKNOWN(CWin2fsShellExt, ShellExtInit)

 HRESULT CWin2fsShellExt::XShellExtInit::Initialize
	( LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID )
 {
	METHOD_PROLOGUE(CWin2fsShellExt, ShellExtInit);

    if( !pThis->m_strFile.IsEmpty())
		return E_UNEXPECTED; // Should be initialized only once

	if( !pDataObj )
		return E_UNEXPECTED; // Should be initialized only once
	
    FORMATETC fmte = { CF_HDROP,
        			   NULL,
        	           DVASPECT_CONTENT,
        	           -1,
        	           TYMED_HGLOBAL 
        	         };
    STGMEDIUM medium;
	HRESULT hRes = pDataObj->GetData( &fmte, &medium );

    if( SUCCEEDED( hRes ))
    {
        if( medium.tymed != TYMED_HGLOBAL || !medium.hGlobal)
			return E_UNEXPECTED;
        
		if( 1 == ::DragQueryFile( (HDROP)medium.hGlobal, 0xFFFFFFFF, 0, 0 ))
		{
			pThis->m_strFile.ReleaseBuffer
				( ::DragQueryFile( (HDROP)medium.hGlobal, 0, 
								   pThis->m_strFile.GetBuffer( MAX_PATH ), 
								   MAX_PATH + 1 
								 )
				);

			ASSERT( ::GetFileAttributes( pThis->m_strFile ) != 0xFFFFFFFF );

			hRes = IsExt2fs (pThis->m_strFile);
		}

		::ReleaseStgMedium( &medium );
	}
    
    return hRes;
       
	UNUSED_ALWAYS( pIDFolder );
	UNUSED_ALWAYS( hKeyID );
 }

 /////////////////////////////////////////////////////////////////////////////
 //
 // IShellPropSheetExt implementation

 IMPLEMENT_IUNKNOWN(CWin2fsShellExt, ShellPropSheetExt)

 HRESULT CWin2fsShellExt::XShellPropSheetExt::AddPages( LPFNADDPROPSHEETPAGE AddPage, LPARAM lParam )
 { 
	METHOD_PROLOGUE(CWin2fsShellExt, ShellPropSheetExt);
    
	try
	{
		HPROPSHEETPAGE hPage = NULL;

		if (IsDrive (pThis->m_strFile))
			hPage =	( new CWin2fsDrivePage( pThis->m_strFile, TRUE ))->CreatePage();     
		else
			hPage =	( new CWin2fsFilePage( pThis->m_strFile, TRUE ))->CreatePage();     

		if( hPage )
		{ 
			if( !AddPage( hPage, lParam )) 
				VERIFY( ::DestroyPropertySheetPage( hPage )); // deletes the CWin2fsXXXXPage object
		} 
		
		return NOERROR;
	}
	catch( CMemoryException* pX )
	{
		pX->Delete();
	}
	
	return E_OUTOFMEMORY;
 }

 HRESULT CWin2fsShellExt::XShellPropSheetExt::ReplacePage( UINT, LPFNADDPROPSHEETPAGE, LPARAM )
 {
	return E_NOTIMPL;
 }

 /////////////////////////////////////////////////////////////////////////////