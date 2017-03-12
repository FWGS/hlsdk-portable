//++ BulliT
#if !defined(AFX_AGWALLHACK_H__72F3428F_5B58_4681_A572_92EAAE5B2F91__INCLUDED_)
#define AFX_AGWALLHACK_H__72F3428F_5B58_4681_A572_92EAAE5B2F91__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef AG_USE_CHEATPROTECTION

// ToolHelp Function Pointers so that it will work on NT4
#include <tlhelp32.h>
typedef HANDLE (WINAPI* CREATETOOLHELP32SNAPSHOT)(DWORD,DWORD);
typedef BOOL   (WINAPI* MODULE32FIRST)(HANDLE,LPMODULEENTRY32);
typedef BOOL   (WINAPI* MODULE32NEXT)(HANDLE,LPMODULEENTRY32);
typedef BOOL   (WINAPI* PROCESS32FIRST)(HANDLE,LPPROCESSENTRY32);
typedef BOOL   (WINAPI* PROCESS32NEXT)(HANDLE,LPPROCESSENTRY32);

class AgWallhack  
{
  CREATETOOLHELP32SNAPSHOT m_lpfCreateToolhelp32Snapshot;
  MODULE32FIRST            m_lpfModule32First;
  MODULE32NEXT             m_lpfModule32Next;
  PROCESS32FIRST           m_lpfProcess32First;
  PROCESS32NEXT            m_lpfProcess32Next;
  HINSTANCE                m_hKernel32;

  bool m_bDoneCheck;
  bool InitToolHelp32();
  int InternalCheck();

  AgStringSet m_setBadStrings;
  AgStringSet m_setBadDlls;
  AgStringSet m_setGoodDlls;
  AgStringSet m_setGoodSystemDlls;
  AgString    m_sDll;
  int m_iFiles;
  DWORD m_dwBytes;

  DWORD m_dwHLAddressToValidate;

public:
  
	AgWallhack();
	virtual ~AgWallhack();

  void AddBadDll(const char* pszDll);
  void AddBadString(const char* pszString);
  void AddBadStrings(const char* pszStrings);

  bool Check();

  void SetHLAddressToValidate(DWORD dwHLAddressToValidate)
  {
    m_dwHLAddressToValidate = dwHLAddressToValidate;
  }

  void SendMessageToIRC(const char* szMessage);
};

extern AgWallhack g_Wallhack;

#endif //AG_USE_CHEATPROTECTION

#endif // !defined(AFX_AGWALLHACK_H__72F3428F_5B58_4681_A572_92EAAE5B2F91__INCLUDED_)

//-- Martin Webrant
