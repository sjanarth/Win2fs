 
 /*
  *		Copyright (C) 2003 - 2004 Satish Kumar J (vsat_in@yahoo.com)  
  *
  *		Project:		
  *
  *		Module Name:	.\CHyperLink.h	
  *
  *     Abstract:		Header for the CHyperLink class.
  *
  *     Author:		    Satish Kumar J
  *
  *		Date:			23-SEP-2004
  *
  *		Notes:			None.
  *
  *		Revision History:
  *
  *		Date		Version	Author				Changes
  *		------------------------------------------------------------------------
  *
  *		23-SEP-04	1.00	Satish Kumar J		Initial	Version.
  *
  */

 #ifndef	__CHYPERLINK_H
 #define	__CHYPERLINK_H

 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "..\StdAfx.h"

 //////////////////////////////////////////////////////////////////////////////
 
 // Macros and typedefs

 #define TOOLTIP_ID 1

 //////////////////////////////////////////////////////////////////////////////

 // Global declarations.

 class CHyperLink : public CStatic
 {
	public:

		CHyperLink();
		virtual ~CHyperLink();

		enum UnderLineOptions { ulHover = -1, ulNone = 0, ulAlways = 1};

		VOID SetURL(CString strURL);
		CString GetURL() const;

		VOID SetColours(COLORREF crLinkColour, 
						COLORREF crVisitedColour, COLORREF crHoverColour = -1);
		COLORREF GetLinkColour() const;
		COLORREF GetVisitedColour() const;
		COLORREF GetHoverColour() const;

		VOID SetVisited(BOOL bVisited = TRUE);
		BOOL GetVisited() const;

		VOID SetLinkCursor(HCURSOR hCursor);
		HCURSOR GetLinkCursor() const;

		VOID SetUnderline(int nUnderline = ulHover);
		INT  GetUnderline() const;

		VOID SetAutoSize(BOOL bAutoSize = TRUE);
		BOOL GetAutoSize() const;

	protected:

		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual BOOL DestroyWindow();
		virtual VOID PreSubclassWindow();

		afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
		afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		afx_msg VOID OnMouseMove(UINT nFlags, CPoint point);
		afx_msg VOID OnTimer(UINT nIDEvent);
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg VOID OnClicked();

		DECLARE_MESSAGE_MAP()
    
		HINSTANCE GotoURL(LPCTSTR url, INT showcmd);
		VOID ReportError(INT nError);
		LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata);
		VOID PositionWindow();
		VOID SetDefaultCursor();

		COLORREF m_crLinkColour, m_crVisitedColour;     // Hyperlink colours
		COLORREF m_crHoverColour;                       // Hover colour
		BOOL     m_bOverControl;                        // cursor over control?
		BOOL     m_bVisited;                            // Has it been visited?
		int      m_nUnderline;                          // underline hyperlink?
		BOOL     m_bAdjustToFit;                        // Adjust window size to fit text?
		CString  m_strURL;                              // hyperlink URL
		CFont    m_UnderlineFont;                       // Font for underline display
		CFont    m_StdFont;                             // Standard font
		HCURSOR  m_hLinkCursor;                         // Cursor for hyperlink
		CToolTipCtrl m_ToolTip;                         // The tooltip
		UINT     m_nTimerID;
 };

 //////////////////////////////////////////////////////////////////////////////

 #endif		//	__CHYPERLINK_H