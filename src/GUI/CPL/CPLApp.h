
 /*                                                                            
  *		Copyright (c) 2001 - 2003 Satish Kumar J (vsat_in@yahoo.com)
  *
  *		Project:		Win2fs
  *                                                                            
  *		Module Name:	\GUI\CPL\CPLApp.h
  *                                                                            
  *		Abstract:		Header for a Control panel applet framework.
  *
  *		Notes:			None  
  *
  *		Revision History:
  *
  *		Date		Version		Author				Change Log
  *		------------------------------------------------------------------------
  *
  *		03-APR-02	0.0.1		Satish Kumar J		Initial Version
  */                          		
 								           
 #ifndef	__CPLAPP_H
 #define	__CPLAPP_H

 ///////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "CPL.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global declarations.

 BOOL WINAPI DllMain (HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved);
 LONG CALLBACK CPlApplet (HWND hWnd, UINT uMsg, LONG lParam1, LONG lParam2);
					   
 class CCPLEx 
 {
    public :

      // Contruction & Destruction.
	   
	  CCPLEx (INT nIconID, INT nNameID, INT nDescID);
      virtual ~CCPLEx ();

      // Applet message handling.

      virtual BOOL OnInit ();
      virtual BOOL OnInquire (LPCPLINFO lpCPlInfo);
      virtual BOOL OnNewInquire (LPNEWCPLINFO lpNewCPlInfo);
      virtual BOOL OnDoubleClick (HWND hWnd, LONG appletData) = 0;
      virtual BOOL OnStartWithParams (HWND hWnd, LPSTR params);
      virtual BOOL OnStop (LONG appletData);
      virtual BOOL OnExit ();
  
   private :

      // Applet data.

      INT m_nIconID;
      INT m_nNameID;
      INT m_nDescID;
      CCPLEx *m_pNext;

      // Static access functions to manipulate all applets in this DLL.

      friend BOOL WINAPI ::DllMain (HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved);
      friend LONG CALLBACK ::CPlApplet (HWND hWnd, UINT uMsg, LONG lParam1, LONG lParam2);

      static void SetInstanceHandle (HINSTANCE hInstance);

      static LONG Init ();
      static LONG GetCount ();
      static LONG Inquire (LONG appletIndex, LPCPLINFO lpCPlInfo);
      static LONG NewInquire (LONG appletIndex, LPNEWCPLINFO lpCPlInfo);
      static LONG DoubleClick (HWND hWnd, LONG lParam1, LONG lParam2);
      static LONG StartWithParams (HWND hWnd, LONG lParam1, LPSTR lParam2);
      static LONG Stop (LPARAM lParam1, LPARAM lParam2);
      static LONG Exit ();
      
      // Private helper function.

      static CCPLEx *GetAppletByIndex (LONG index);

      // Static applet data. 
      
      static HINSTANCE s_hInstance;
      static CCPLEx *s_pListHead;

      // Disable copying: Do not implement these functions ...

      CCPLEx (const CCPLEx &rhs);
      CCPLEx &operator = (const CCPLEx &rhs);
 };

 ///////////////////////////////////////////////////////////////////////////////

 #endif		// __CPLAPP_H

