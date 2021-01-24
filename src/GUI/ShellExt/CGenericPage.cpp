 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CGenericPage.cpp
  *
  *     Abstract:		Header for the grand base class of all our property pages.
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

 #include <StdAfx.h>

 //////////////////////////////////////////////////////////////////////////////

 // Private includes.

 #include "CGenericPage.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global data.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static data.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static functions.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Restricted functions.

 // Privates'.

 // None.

 // Protected.

 // None.
 
 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 #ifdef _AFXDLL

 struct CCreatePageHook : public CNoTrackObject
 {
	HHOOK	m_hOldCbtHook;
	CGenericPage* m_pPageInit;

	CCreatePageHook()
			:m_hOldCbtHook( NULL ),	m_pPageInit( NULL )
	{
	}

	~CCreatePageHook()
	{
		if( m_hOldCbtHook )
		{
			::UnhookWindowsHookEx( m_hOldCbtHook );
			m_hOldCbtHook  = NULL;
		}
	}
 };

 CThreadLocal<CCreatePageHook> hookCreatePage;

 LRESULT CALLBACK CGenericPage::PreCreateHook( int code, WPARAM wParam, LPARAM lParam )
 {
	ASSERT( hookCreatePage->m_hOldCbtHook );
	ASSERT( hookCreatePage->m_pPageInit );

	if( code == HCBT_CREATEWND )
	{
		_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
		CGenericPage* pPage = hookCreatePage->m_pPageInit;

		if( pThreadState->m_pWndInit == pPage )
		{
			ASSERT( AfxGetModuleState() != pPage->m_pModuleState );
			AFX_MANAGE_STATE( pPage->m_pModuleState );

			// The following call is performed in the context of the RIGHT state.
			LRESULT lRes = CallNextHookEx( hookCreatePage->m_hOldCbtHook, code, wParam, lParam );

			ASSERT( !pThreadState->m_pWndInit ); // Already removed
			ASSERT( pPage == CWnd::FromHandlePermanent( pPage->m_hWnd )); // Attached

			::UnhookWindowsHookEx( hookCreatePage->m_hOldCbtHook );
			hookCreatePage->m_hOldCbtHook = NULL;

			hookCreatePage->m_pPageInit = NULL;

			return lRes;
		}
	}

	return CallNextHookEx( hookCreatePage->m_hOldCbtHook, code, wParam, lParam );
 }

 UINT CALLBACK AfxPropPageCallback(HWND, UINT message, LPPROPSHEETPAGE pPropPage);

 UINT CALLBACK CGenericPage::PropPageCallback( HWND hWnd, UINT message, LPPROPSHEETPAGE pPropPage )
 {
	UINT uRes = AfxPropPageCallback( hWnd, message, pPropPage );

	CGenericPage* pPage = STATIC_DOWNCAST( CGenericPage, (CObject*)pPropPage->lParam );

	switch( message )
	{
		case PSPCB_CREATE:
		{
			if( !uRes )
				return FALSE;

			{
				AFX_MANAGE_STATE( pPage->m_pModuleState );

				if( !pPage->OnCreatePage())
					return FALSE;
			}
				
			if( AfxGetModuleState() != pPage->m_pModuleState )
			{
				// AfxPropPageCallback sets Subclassing CBT hook on window creation.
				//	We need to overlay it with another hook so that we can set correct module state.
				
				_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
				ASSERT( pThreadState->m_pWndInit == pPage );
				ASSERT( pThreadState->m_hHookOldCbtFilter );

				if( !hookCreatePage->m_hOldCbtHook )
				{
					hookCreatePage->m_hOldCbtHook = ::SetWindowsHookEx( WH_CBT, PreCreateHook, NULL, ::GetCurrentThreadId());
					if( !hookCreatePage->m_hOldCbtHook )
						AfxThrowMemoryException(); // Just like std AfxHookWindowCreate behaves
				}
				else
					ASSERT( FALSE );

				ASSERT( !hookCreatePage->m_pPageInit );
				hookCreatePage->m_pPageInit = pPage;
			}
		}
		break;
	
		case PSPCB_RELEASE:
		{
			AFX_MANAGE_STATE( pPage->m_pModuleState );

			pPage->OnReleasePage();
		}
		break;
	}

	return uRes;
 }

 #endif // _AFXDLL

 //////////////////////////////////////////////////////////////////////////////

 IMPLEMENT_DYNAMIC(CGenericPage, CPropertyPage)

 CGenericPage::CGenericPage( UINT idTemplate )
			  :CPropertyPage( idTemplate ), m_bAutoDestroy( FALSE ),m_bChanged( FALSE )
 {
 #ifdef _AFXDLL
	m_psp.pfnCallback = PropPageCallback;
 #endif
 }

 CGenericPage::CGenericPage( UINT idTemplate, BOOL bAutoDestroy ) 
			  :CPropertyPage( idTemplate ), m_bAutoDestroy( bAutoDestroy ), m_bChanged( FALSE )
 {
 #ifdef _AFXDLL
	m_psp.pfnCallback = PropPageCallback;
 #endif
 }

 CGenericPage::~CGenericPage()
 {
 }

 //////////////////////////////////////////////////////////////////////////////

 HPROPSHEETPAGE CGenericPage::CreatePage()
 {
	PROPSHEETPAGE psp; memcpy( &psp, &m_psp, sizeof( psp ));
	PreProcessPageTemplate( psp, FALSE );
	
	HPROPSHEETPAGE hpgRes = ::CreatePropertySheetPage( &psp );

	if( !hpgRes )
		OnReleasePage();

	return hpgRes;
 }

 VOID CGenericPage::SetModified (BOOL bChanged /*=TRUE*/)
 { 
	m_bChanged = bChanged; 
	CPropertyPage::SetModified( bChanged );
 }

 BOOL CGenericPage::IsModified() const
 { 
	 return m_bChanged; 
 }

 CPropertySheet* CGenericPage::GetParentSheet() 
 { 
	return STATIC_DOWNCAST( CPropertySheet, GetParent()); 
 }

 /////////////////////////////////////////////////////////////////////////////
 // CGenericPage message handlers

 BOOL CGenericPage::OnCreatePage()
 {
	return TRUE;
 }

 VOID CGenericPage::OnReleasePage()
 {
	if( m_bAutoDestroy )
		delete this; // must be created on heap
 }

 VOID CGenericPage::OnModified()
 { 
	 SetModified(); 
	 UpdateDialogControls( this, FALSE ); 
 }

 VOID CGenericPage::OnModified(UINT)
 { 
	 OnModified ();
 }

 /////////////////////////////////////////////////////////////////////////////