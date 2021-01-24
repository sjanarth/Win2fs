
 /*
  *		Copyright (C) 2003 - 2004 Satish Kumar J (vsat_in@yahoo.com)  
  *
  *		Project:		
  *
  *		Module Name:	Registry.cpp
  *
  *     Abstract:		A set of classes for accessing the Windows Registry.
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
  *		27-MAR-2003	1.04	Satish Kumar J		Removed the Write in the destrs.
  *												Changed the = overloads to write even if the values are the same.		
  *												Changed the typecasts to read even if the values are the same.		
  *												Removed the Force option.
  *												Removed the caching option.
  *
  *		03-APR-2003	1.05	Satish Kumar J		- Nil -
  *
  */

 //////////////////////////////////////////////////////////////////////////////

 // Includes.

 #include "StdAfx.h"
 #include "Registry.h"

 //////////////////////////////////////////////////////////////////////////////

 // Global functions.

 // CRegBase.

 CRegBase::CRegBase () 
 {
	m_hKey = NULL;
	m_hBase = DEFAULT_BASE_KEY;

	m_strProp = "";
	m_strKey = "";

	m_bKeyPresent = FALSE; 
	m_bPropPresent = FALSE;

	m_nCurSubKey = 0;
	m_nCurProp = 0;
 }

 CRegBase::CRegBase (HKEY base, CString key, CString property)
 {
	m_hKey = NULL;
	m_hBase = base;

	key.TrimLeft ("\\");
	key.TrimRight ("\\");

	property.TrimLeft ("\\");
	property.TrimRight ("\\");

	m_strKey = key;
	m_strProp = property;
	
	m_bKeyPresent = FALSE; 
	m_bPropPresent = FALSE;

	m_nCurSubKey = 0;
	m_nCurProp = 0;
 }

 CRegBase::~CRegBase ()
 {
 }

 BOOL CRegBase::Open(REGSAM sam)
 {
	 BOOL bRet = FALSE;

	 do
	 {
		DWORD dwRet = RegOpenKeyEx (m_hBase, m_strKey, 0, sam, &m_hKey);

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

		bRet = TRUE;

	 } while (FALSE);

	 return bRet;
 }

 DWORD CRegBase::RemoveKey ()
 {
	DWORD dwRet;

	dwRet = 0;

	do
	{
		BOOL bRet = Open(KEY_WRITE);

		BREAK_IF_FALSE(bRet);
		
		dwRet = SHDeleteKey (m_hBase, (LPCTSTR) m_strKey);

	} while (FALSE);

	return dwRet;
 }

 LONG CRegBase::RemoveValue ()
 {
	 DWORD dwRet;

	 dwRet = 0;

	 do
	 {
		BOOL bRet = Open(KEY_WRITE);

		BREAK_IF_FALSE(bRet);

		dwRet = RegDeleteValue (m_hKey, (LPCTSTR) m_strProp); 

	 } while (FALSE);

	 return (LONG) dwRet;
 }

 BOOL CRegBase::IsKeyPresent ()
 {
	BOOL bRet;
	DWORD dwRet;

	dwRet = 0;
	bRet = FALSE;

	do
	{
		 if (m_bKeyPresent)
		 {
			bRet = TRUE;
								
			break;
		 }

		 bRet = Open(KEY_QUERY_VALUE);

		 BREAK_IF_FALSE(bRet);

		 m_bKeyPresent = TRUE;

	} while (FALSE);

	SAFE_CLOSE_KEY (m_hKey);

	return bRet;
 }

 BOOL CRegBase::IsPropPresent ()
 {
	 BOOL bRet;
	 DWORD dwRet;
	 DWORD dwType;

	 dwRet = 0;
	 dwType = 0;
	 bRet = FALSE;

	 do
	 {
		 if (m_bPropPresent)
		 {
			 bRet = TRUE;

			 break;
		 }

		 if (!IsKeyPresent ())
		 {
			 break;
		 }

		 bRet = Open(KEY_QUERY_VALUE);

		 BREAK_IF_FALSE(bRet);

		 dwRet = RegQueryValueEx (m_hKey, m_strProp, NULL, &dwType, NULL, NULL);

		 BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

		 bRet = TRUE;

		 m_bPropPresent = TRUE;

	 } while (FALSE);

	 SAFE_CLOSE_KEY (m_hKey);

	 return bRet;
 }

 CString CRegBase::GetNextSubKey()
 {
	 CString sKeyName = "";

	 do
	 {
		 BOOL bRet = Open(KEY_ENUMERATE_SUB_KEYS);

		 BREAK_IF_FALSE(bRet);

		 CHAR szKeyName[512];

		 LONG lRet = RegEnumKey(m_hKey, m_nCurSubKey, szKeyName, 512);

		 BREAK_IF_UNEQUAL(lRet, ERROR_SUCCESS);

		 sKeyName = szKeyName;

		 m_nCurSubKey++;

	 } while (FALSE);

	 SAFE_CLOSE_KEY (m_hKey);

	 return sKeyName;
 }

 CString CRegBase::GetNextProperty()
 {
	 CString sPropName = "";

	 do
	 {
		 BOOL bRet = Open(KEY_QUERY_VALUE);

		 BREAK_IF_FALSE(bRet);

		 CHAR szPropName[512];
		 DWORD dwSize = 512;

		 LONG lRet = RegEnumValue(m_hKey, m_nCurProp, szPropName, &dwSize, NULL, NULL, NULL, NULL);

		 BREAK_IF_UNEQUAL(lRet, ERROR_SUCCESS);

		 sPropName = szPropName;

		 m_nCurProp++;

	 } while (FALSE);

	 SAFE_CLOSE_KEY (m_hKey);

	 return sPropName;
 }

 CRegDWORD::CRegDWORD ()
 {
	m_Value = 0;
	m_DefValue = 0;
 }

 CRegDWORD::CRegDWORD (HKEY base, CString key, CString prop, DWORD def)
 {					  
	m_Value = 0;
	m_hBase = base;
	m_DefValue = def;
	
	key.TrimLeft ("\\");
	key.TrimRight ("\\");

	prop.TrimLeft ("\\");
	prop.TrimRight ("\\");
	
	m_strKey = key;
	m_strProp = prop;
	
	Read ();
 }

 CRegDWORD::~CRegDWORD ()
 {
 }

 DWORD CRegDWORD::Read ()
 {
	DWORD dwRet;
	DWORD dwSize;
	DWORD dwType;
	DWORD dwValue;

	dwRet = 0;
	dwSize = 0;
	dwType = 0;
	dwValue = m_DefValue;

	do
	{
		ASSERT (m_strProp != "");

		BOOL bRet = Open(KEY_EXECUTE);

		BREAK_IF_FALSE(bRet);

		dwSize = sizeof (m_Value);

		dwRet = RegQueryValueEx (m_hKey, 
								 m_strProp, 
								 NULL, &dwType, (PBYTE) &m_Value, &dwSize);

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

		ASSERT (REG_DWORD == dwType);
			
		dwValue = m_Value;

	} while (FALSE);

	SAFE_CLOSE_KEY (m_hKey);	

	return dwValue;
 }

 VOID CRegDWORD::Write ()
 {
	DWORD dwRet;
	DWORD dwDisp;

	dwRet = 0;
	dwDisp = 0;

	do
	{
		ASSERT (m_strProp != "");

		dwRet = RegCreateKeyEx (m_hBase, 
								m_strKey, 
								0, 
								"", 
								REG_OPTION_NON_VOLATILE, 
								KEY_WRITE, NULL, &m_hKey, &dwDisp);

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

		dwRet = RegSetValueEx (m_hKey, 
							   m_strProp, 
							   0, 
							   REG_DWORD,
							   (const PBYTE) &m_Value, sizeof (m_Value));

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

	} while (FALSE);

	SAFE_CLOSE_KEY (m_hKey);
 }

 CRegDWORD::operator DWORD ()
 {
	return Read ();
 }
 
 CRegDWORD& CRegDWORD::operator = (DWORD d)
 {
	m_Value = d;

	Write ();

	return *this;
 }

 //////////////////////////////////////////////////////////////////////////////////////////////

 CRegString::CRegString ()
 {
	m_Value = "";
	m_DefValue = "";
 }

 CRegString::CRegString (HKEY base, CString key, CString prop, CString def)
 {
	m_Value = "";
	m_hBase = base;
	m_DefValue = def;

	key.TrimLeft ("\\");
	key.TrimRight ("\\");

	prop.TrimLeft ("\\");
	prop.TrimRight ("\\");
	
	m_strKey = key;
	m_strProp = prop;

	Read ();
 }

 CRegString::~CRegString ()
 {
 }

 CString CRegString::Read ()
 {
	DWORD dwRet;
	DWORD dwSize;
	DWORD dwType;
	CString strRet;

	dwRet = 0;
	dwType = 0;
	dwSize = 0;
	strRet = m_DefValue;

	do
	{
		// This can be NULL (the "(Default)" property).
		// ASSERT (m_strProp != "");

		BOOL bRet = Open(KEY_EXECUTE);

		BREAK_IF_FALSE(bRet);

		dwSize = sizeof (m_Value);

		dwRet = RegQueryValueEx (m_hKey, m_strProp, NULL, &dwType, NULL, &dwSize);

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

		PCHAR pStr = new CHAR[dwSize];

		dwRet = RegQueryValueEx (m_hKey, 
								 m_strProp, 
								 NULL, 
								 &dwType, (PBYTE) pStr, &dwSize);

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

		ASSERT ((REG_SZ == dwType) || (REG_MULTI_SZ == dwType) || (REG_EXPAND_SZ == dwType));

		if (REG_MULTI_SZ == dwType)
		{
			// Concatenate all the strings.

			DWORD dwLen = strlen (pStr);

			while ((dwLen+1) < dwSize)
			{
				pStr[dwLen] = ' ';

				dwLen = strlen (pStr);
			}
		}

 		m_Value = CString (pStr);

		strRet = m_Value;

		delete[] pStr;
		
	} while (FALSE);

	SAFE_CLOSE_KEY (m_hKey);	
	
	return strRet;
 }

 VOID CRegString::Write ()
 {
	DWORD dwRet;
	DWORD dwDisp;

	dwRet = 0;
	dwDisp = 0;

	do
	{
		// This can be NULL (the "(Default)" property).		
		// ASSERT (m_strProp != "");

		dwRet = RegCreateKeyEx (m_hBase, 
								m_strKey, 
								0, 
								"", 
								REG_OPTION_NON_VOLATILE, 
								KEY_WRITE, NULL, &m_hKey, &dwDisp);

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

		dwRet = RegSetValueEx (m_hKey, 
							   m_strProp, 
							   0, 
							   REG_SZ,
							   (const PBYTE) (LPCTSTR)m_Value, 
							   m_Value.GetLength ()+1);

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

	} while (FALSE);

	SAFE_CLOSE_KEY (m_hKey);
 }

 CRegString::operator CString ()
 {
	return Read ();
 }

 CRegString& CRegString::operator = (CString s)
 {
	m_Value = s;

	Write ();
	
	return *this;
 }

 //////////////////////////////////////////////////////////////////////////////////////////////

 CRegBinary::CRegBinary ()
 {
	m_Value = NULL;
	m_dwCount = 0;
 }

 CRegBinary::~CRegBinary()
 {
	 if(m_Value != NULL)
	 {
		 delete m_Value;
	 }
 }

 CRegBinary::CRegBinary (HKEY base, CString key, CString prop)
 {
	m_Value = NULL;
	m_hBase = base;

	key.TrimLeft ("\\");
	key.TrimRight ("\\");

	prop.TrimLeft ("\\");
	prop.TrimRight ("\\");
	
	m_strKey = key;
	m_strProp = prop;
 }

 PBYTE CRegBinary::Read (LPDWORD pdwRead)
 {
	DWORD dwRet;
	DWORD dwType;

	dwRet = 0;
	dwType = 0;

	do
	{
		// This can be NULL (the "(Default)" property).
		// ASSERT (m_strProp != "");

		BOOL bRet = Open(KEY_EXECUTE);

		BREAK_IF_FALSE(bRet);

		m_dwCount = 1;

		dwRet = RegQueryValueEx (m_hKey, m_strProp, NULL, &dwType, NULL, &m_dwCount);

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

		if(m_Value != NULL)
		{
			if(m_dwCount > sizeof(m_Value))
			{
				delete m_Value;
				m_Value = NULL;
			}
		}

		if(m_Value == NULL)
		{
			m_Value = new BYTE[m_dwCount];
		}

		dwRet = RegQueryValueEx (m_hKey, m_strProp, NULL, &dwType, m_Value, &m_dwCount);

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

		ASSERT (REG_BINARY == dwType);

		if(pdwRead)
		{
			(*pdwRead) = m_dwCount;
		}

	} while (FALSE);

	SAFE_CLOSE_KEY (m_hKey);	
	
	return m_Value;
 }

 VOID CRegBinary::Write (PBYTE pbuf, DWORD dwCount)
 {
	DWORD dwRet;
	DWORD dwDisp;

	dwRet = 0;
	dwDisp = 0;

	do
	{
		// This can be NULL (the "(Default)" property).		
		// ASSERT (m_strProp != "");

		dwRet = RegCreateKeyEx (m_hBase, m_strKey, 0, "", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &m_hKey, &dwDisp);

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

		dwRet = RegSetValueEx (m_hKey, m_strProp, 0, REG_BINARY, (const PBYTE) pbuf, dwCount);

		BREAK_IF_UNEQUAL (dwRet, ERROR_SUCCESS);

	} while (FALSE);

	SAFE_CLOSE_KEY (m_hKey);
 }

 //////////////////////////////////////////////////////////////////////////////////////////////

