
 /*
  *		Copyright (C) 2003 - 2004 Satish Kumar J (vsat_in@yahoo.com)  
  *
  *		Project:		
  *
  *		Module Name:	Registry.h
  *
  *     Abstract:		Header for the Registry classes.
  *
  *     Author:		    Satish Kumar J
  *
  *		Date:			21-MAR-2003
  *
  *		Notes:			None.
  *
  *		Revision History:
  *
  *		Date		Version	Author				Changes
  *		------------------------------------------------------------------------
  *
  *		25-MAR-2003	1.03	Satish Kumar J		Initial Version
  *
  *		27-MAR-2003	1.04	Satish Kumar J		Removed the force, cache options.
  *												Removed the CRegRect and CRegPoint classes.
  *
  *		03-APR-2003	1.05	Satish Kumar J		- Nil -
  *
  */

 #ifndef	__REGISTRY_H
 #define	__REGISTRY_H

 //////////////////////////////////////////////////////////////////////////////

 // Include files.

 #include <afxwin.h>
 #include <shlwapi.h>

 ////////////////////////////////////////////////////////////////////////////////

 // Macros and typedefs'.

 #define DEFAULT_BASE_KEY				HKEY_LOCAL_MACHINE

 #define SAFE_CLOSE_KEY(hKey)			if (hKey) {						\
											RegCloseKey (hKey); 		\
											hKey = NULL; 				\
										} 

 #define BREAK_IF_UNEQUAL(val1, val2)	if (val1 != val2) { break; }	
  
 //////////////////////////////////////////////////////////////////////////////

 // Global declarations.

 class CRegBase
 {
	public:	

		CRegBase ();
		CRegBase (HKEY base, CString key, CString property);
		virtual ~CRegBase ();

		DWORD RemoveKey ();
		LONG RemoveValue (); 

		BOOL IsKeyPresent ();
		BOOL IsPropPresent ();

		CString GetNextSubKey();
		CString GetNextProperty();

	protected:

		BOOL Open(REGSAM);

		UINT m_nCurSubKey;
		UINT m_nCurProp;

		HKEY m_hKey;
		HKEY m_hBase;

		CString m_strKey;
		CString m_strProp;

		BOOL m_bKeyPresent;
		BOOL m_bPropPresent;
 };

 class CRegDWORD : public CRegBase
 {
	public:

		CRegDWORD ();
		CRegDWORD (HKEY base, CString key, CString prop, DWORD def = 0);
		virtual ~CRegDWORD ();

		DWORD Read ();	
		VOID  Write ();	

		operator DWORD ();

		CRegDWORD& operator = (DWORD d);
		CRegDWORD& operator += (DWORD d) { return *this = *this + d; }
		CRegDWORD& operator -= (DWORD d) { return *this = *this - d; }
		CRegDWORD& operator *= (DWORD d) { return *this = *this * d; }
		CRegDWORD& operator /= (DWORD d) { return *this = *this / d; }
		CRegDWORD& operator %= (DWORD d) { return *this = *this % d; }
		CRegDWORD& operator &= (DWORD d) { return *this = *this & d; }
		CRegDWORD& operator |= (DWORD d) { return *this = *this | d; }
		CRegDWORD& operator ^= (DWORD d) { return *this = *this ^ d; }
		CRegDWORD& operator <<= (DWORD d) { return *this = *this << d; }
		CRegDWORD& operator >>= (DWORD d) { return *this = *this >> d; }
	
	protected:

		DWORD	m_Value;	
		DWORD	m_DefValue;
 };

 class CRegString : public CRegBase
 {
	public:
	
		CRegString ();
		CRegString (HKEY base, CString key, CString prop, CString def = "");
		virtual ~CRegString ();
	
		CString Read ();
		VOID	Write ();
		
		operator CString ();
		CRegString& operator = (CString s);
		CRegString& operator += (CString s) { return *this = (CString)*this + s; }
	
	protected:

		CString	m_Value;
		CString	m_DefValue;
 };

 class CRegBinary : public CRegBase
 {
	public:
	
		CRegBinary ();
		CRegBinary (HKEY base, CString key, CString prop);
		virtual ~CRegBinary ();
	
		PBYTE Read (PDWORD pdwRead);
		VOID  Write (PBYTE pbuf, DWORD dwCount);
		
	protected:

		PBYTE m_Value;
		DWORD m_dwCount;
 };

 //////////////////////////////////////////////////////////////////////////////

 #endif		//	__REGISTRY_H

