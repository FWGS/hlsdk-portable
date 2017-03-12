#include <stdio.h>

#ifdef AG_USE_CHEATPROTECTION

#include "AgGlobal.h"
#include "AgVersionInfo.h"

#pragma comment(lib,"version.lib")

DWORD AgVersionInfo::SetError()
{
  m_dwLastError = ::GetLastError();
  if (0 == m_dwLastError) 
    m_dwLastError = (DWORD)-1;
  return m_dwLastError;
}

DWORD AgVersionInfo::LoadVersionInfo(LPCSTR pszFileName)
{
  try
  {
    // get size of fileversion
    DWORD dwLen = ::GetFileVersionInfoSize((LPTSTR)pszFileName,&m_dwHandle);

    if (0 == dwLen)
      return SetError();

    // get data-info
    m_pszData = (char*)malloc(dwLen+16);
    BOOL bRet = ::GetFileVersionInfo((LPTSTR)pszFileName,m_dwHandle,dwLen,m_pszData);
  
    if (!bRet)
    {
      //assert(FALSE);
      return SetError();
    }

    // get VS_FIXEDFILEINFO struct

    VS_FIXEDFILEINFO* pInfo  = NULL;
    UINT              uiSize = 0;

    if (!::VerQueryValue((BYTE*)(LPCSTR)m_pszData,"\\",(LPVOID*)&pInfo,&uiSize))
    {
      //assert(FALSE);
      return SetError();
    }

    // did we get something?

    if (uiSize != sizeof(m_ffi))
    {
      //assert(FALSE);
      return SetError();
    }

    // does the structure have correct signature and version?

    if (VS_FFI_SIGNATURE != pInfo->dwSignature || VS_FFI_STRUCVERSION != pInfo->dwStrucVersion)
    {
      //assert(FALSE);
      return SetError();
    }

    // everything ok, copy to our member-struct

    memcpy(&m_ffi,pInfo,uiSize);

    m_dwLastError = 0;
    return m_dwLastError;
  }
  catch (...)
  {
    AgLog("LoadVersionInfo failed");
    return SetError();
  }
  return 0;
}

BOOL AgVersionInfo::FileVersion(int& iMajor,int& iMinor,int& iMicro,int& iState) const
{
  //assert(!HasErrors());

  if (HasErrors())
    return FALSE;
  
  iMajor = (int)(HIWORD(m_ffi.dwFileVersionMS));
  iMinor = (int)(LOWORD(m_ffi.dwFileVersionMS));
  iMicro = (int)(HIWORD(m_ffi.dwFileVersionLS));
  iState = (int)(LOWORD(m_ffi.dwFileVersionLS));

  return TRUE;
}

BOOL AgVersionInfo::ProductVersion(int& iMajor,int& iMinor,int& iMicro,int& iState) const
{
  //assert(!HasErrors());

  if (HasErrors())
    return FALSE;
  
  iMajor = (int)(HIWORD(m_ffi.dwProductVersionMS));
  iMinor = (int)(LOWORD(m_ffi.dwProductVersionMS));
  iMicro = (int)(HIWORD(m_ffi.dwProductVersionLS));
  iState = (int)(LOWORD(m_ffi.dwProductVersionLS));

  return TRUE;
}

const char* AgVersionInfo::GetTextData(LPCSTR pszParameter,DWORD dwLanguage)
{
  static char szParam[MAX_PATH];

  // must have been initialized properly

  assert(!HasErrors());

  if (HasErrors())
    return "";
  
  LPVOID pInfo;
  UINT   uiSize;

  if (-1 == dwLanguage)
  {
    if (-1 == m_dwDefaultLang)
    {
      // get translation table pointer

      pInfo  = NULL;
      uiSize = 0;

      if (!::VerQueryValue((BYTE*)(LPCSTR)m_pszData,"\\VarFileInfo\\Translation",&pInfo,&uiSize))
      {
        //SetError();
        //assert(FALSE);
        return "";
      }

      // did we get it?

      if (0 == uiSize)
      {
        SetError();
        assert(FALSE);
        return "";
      }

      m_dwDefaultLang = *((DWORD*)pInfo);
    }

    dwLanguage = m_dwDefaultLang;
  }

  // get the parameter

  pInfo  = NULL;
  uiSize = 0;

  m_dwLastError = 0;

  sprintf(szParam,"\\StringFileInfo\\%04x%04x\\%s",LOWORD(dwLanguage),HIWORD(dwLanguage),pszParameter);

  if (!::VerQueryValue((BYTE*)(LPCSTR)m_pszData,szParam,&pInfo,&uiSize))
    return ""; // the parameter is currently not defined

  // is parameter-value an empty-string?

  if (0 == uiSize)
    return "";

  // copy the value to our own string

  memcpy(szParam,(LPBYTE)pInfo,uiSize);
  return szParam;
}


BOOL AgVersionInfo::IsRequiredVersion(int iReqMajor, int iReqMinor, int iReqMicro) const
{
  assert(!HasErrors());

  if (HasErrors())
    return FALSE;

  int iMajor, iMinor, iMicro, iState;
  iMajor = iMinor = iMicro = iState = 0;
  FileVersion(iMajor, iMinor, iMicro, iState);

  if ((iMajor == iReqMajor && iMinor == iReqMinor && iMicro >= iReqMicro) || 
       iMajor == iReqMajor && iMinor >  iReqMinor ||
       iMajor >  iReqMajor)
       return TRUE;

  return FALSE;
}

#endif //AG_USE_CHEATPROTECTION