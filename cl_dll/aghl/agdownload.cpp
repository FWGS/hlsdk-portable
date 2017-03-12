// AgDownload.cpp: implementation of the AgDownload class.
//
//////////////////////////////////////////////////////////////////////


#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"
#include "agglobal.h"
#include "agdownload.h"
#include "urlmon.h"
#pragma comment(lib, "urlmon.lib")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

class AgDownloadCallBack : public IBindStatusCallback  
{
protected:
  DWORD m_dwWhenToTimeout;
public:
  AgDownloadCallBack(DWORD dwWhenToTimeout = 0)
  {
    m_dwWhenToTimeout = dwWhenToTimeout;
  }
  virtual ~AgDownloadCallBack()
  {
  };
  
  // IBindStatusCallback methods.  Note that the only method called by IE
  // is OnProgress(), so the others just return E_NOTIMPL.
  
  STDMETHOD(OnStartBinding)(DWORD dwReserved,IBinding __RPC_FAR *pib) { return E_NOTIMPL; }
  STDMETHOD(GetPriority)(/* [out] */ LONG __RPC_FAR *pnPriority)      { return E_NOTIMPL; }
  STDMETHOD(OnLowResource)(/* [in] */ DWORD reserved){ return E_NOTIMPL; }
  STDMETHOD(OnProgress)(/* [in] */ ULONG ulProgress,/* [in] */ ULONG ulProgressMax,/* [in] */ ULONG ulStatusCode,/* [in] */ LPCWSTR wszStatusText)
  {
    if (m_dwWhenToTimeout > 0 && m_dwWhenToTimeout < GetTickCount()) 
      return E_ABORT;
    return S_OK;
  }
  
  STDMETHOD(OnStopBinding)(/* [in] */ HRESULT hresult,/* [unique][in] */ LPCWSTR szError) { return E_NOTIMPL; }
  STDMETHOD(GetBindInfo)(/* [out] */ DWORD __RPC_FAR *grfBINDF,/* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo) { return E_NOTIMPL; }
  STDMETHOD(OnDataAvailable)(/* [in] */ DWORD grfBSCF,/* [in] */ DWORD dwSize,/* [in] */ FORMATETC __RPC_FAR *pformatetc,/* [in] */ STGMEDIUM __RPC_FAR *pstgmed) { return E_NOTIMPL; }
  STDMETHOD(OnObjectAvailable)(/* [in] */ REFIID riid, /* [iid_is][in] */ IUnknown __RPC_FAR *punk) { return E_NOTIMPL; }
  STDMETHOD_(ULONG,AddRef)() { return 0; }
  STDMETHOD_(ULONG,Release)() { return 0; }
  STDMETHOD(QueryInterface)( /* [in] */ REFIID riid, /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) { return E_NOTIMPL; }
};

AgDownload::AgDownload()
{
  
}

AgDownload::~AgDownload()
{
  
}

void AgDownload::DownloadFile(const char* pszURL, const char* pszSaveAs)
{
  AgDownloadCallBack callback(GetTickCount() + 7000);
  HRESULT hr = URLDownloadToFile(NULL, pszURL, pszSaveAs, 0, &callback);
  
  if (SUCCEEDED(hr))
  {
    ConsolePrint("Download completed successfully!\n");
  }
  else
  {
    char szMsg[512];
    LPTSTR lpszErrorMessage;
    
    if (FormatMessage ( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | 
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, hr, 
      MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&lpszErrorMessage, 0, NULL))
    {
      sprintf(szMsg, "Download failed. Error = 0x%08lX\n\n%s\n", (DWORD)hr, lpszErrorMessage);
      LocalFree(lpszErrorMessage);
    }
    else
    {
      sprintf(szMsg,"Download failed. Error = 0x%08lX\n\nNo message available.\n", (DWORD)hr);
    }
    
    ConsolePrint(szMsg);
  }
  
}
