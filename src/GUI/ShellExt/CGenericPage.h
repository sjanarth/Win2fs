 
 /*
  *     Copyright (C) Satish kumar Janarthanan (vsat_in@yahoo.com.), 2004-2005
  *
  *		Project:		Win2fs
  *
  *		Module Name:    \GUI\ShellExt\CGenericPage.h
  *
  *     Abstract:		Header for the grand base class of all our property pages.
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

 #ifndef	__CGENERICPAGE_H
 #define	__CGENERICPAGE_H

 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Macros and typedefs.
 
 #pragma warning ( disable : 4097 )  // typedef-name '...' used as synonym for class-name '...'

 //////////////////////////////////////////////////////////////////////////////

 // Externals.

 // None.

 //////////////////////////////////////////////////////////////////////////////

 // Static declarations.

 // None.

 //////////////////////////////////////////////////////////////////////////////
 
 // Global declarations.

 class CGenericPage : public CPropertyPage
 {
	public:

		BOOL m_bAutoDestroy;

		CGenericPage (UINT idTemplate);
		CGenericPage (UINT idTemplate, BOOL bAutoDestroy);
		virtual ~CGenericPage ();

		virtual HPROPSHEETPAGE CreatePage ();

		VOID SetModified (BOOL bChanged = TRUE);
		BOOL IsModified () const;
		CPropertySheet* GetParentSheet ();

		inline VOID SetWizardButtons (DWORD dwFlags);

	DECLARE_DYNAMIC(CGenericPage)

	protected:

		virtual BOOL OnCreatePage ();
		virtual VOID OnReleasePage ();

		BOOL m_bChanged;

		afx_msg VOID OnModified ();
		afx_msg VOID OnModified (UINT);

 #ifdef _AFXDLL

	private:

		static UINT CALLBACK PropPageCallback( HWND hWnd, UINT message, LPPROPSHEETPAGE pPropPage );
		static LRESULT CALLBACK PreCreateHook( int code, WPARAM wParam, LPARAM lParam );

 #endif

		friend class CGenericSheet;
 };

 inline VOID CGenericPage::SetWizardButtons( DWORD dwFlags ) 
 {
	CPropertySheet* pSheet = GetParentSheet();
	
	ASSERT( pSheet && pSheet->m_hWnd );
	ASSERT( pSheet->m_psh.dwFlags & PSH_WIZARD );
		
	pSheet->SetWizardButtons( dwFlags );
 }

 //////////////////////////////////////////////////////////////////////////////

 #endif		//	 __CGENERICPAGE_H