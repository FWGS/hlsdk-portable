//++ BulliT
#include "hud.h"
#include "cl_util.h"

#ifdef AG_USE_CHEATPROTECTION

#include "AgWallhack.h"
#include "AgVersionInfo.h"
#include "com_weapons.h"
#include "agbase64.h"
#include "agmapi.h"
#include "agicq.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



bool CheckHooks(const char* pszModule, const char* pszMethod, BYTE* pBytesToCheck, DWORD dwSize)
{
  bool bOK = false;
  HANDLE hProcess = ::GetCurrentProcess();
  HMODULE hModule = ::GetModuleHandle(pszModule);
  if (!hModule)
    return true; //The dll aint loaded
  LPVOID pAddress = ::GetProcAddress(hModule, pszMethod);

  // change the page-protection for the intercepted function
  DWORD dwOldProtect;
  if (!::VirtualProtectEx(hProcess, pAddress, dwSize, PAGE_EXECUTE_READ, &dwOldProtect))
    return false;

  //Read the bytes to see if someone hooked that function
  BYTE* pBytesInMem = (BYTE*)malloc(dwSize);
  DWORD dwRead = 0;
  if (::ReadProcessMemory(hProcess, pAddress, pBytesInMem, dwSize, &dwRead))
  {
    bOK = 0 != memcmp(pBytesInMem, pBytesToCheck, dwRead);

    /*
    char szAddress[_MAX_PATH];
    sprintf(szAddress, "%s::%s - at %lx - %s", pszModule, pszMethod, pAddress, bOK ? "OK" : "HACK");
    AgLog(szAddress);

    HANDLE hFile = CreateFile("c:\\temp.bin", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
    DWORD dwWritten;
    WriteFile(hFile, pBytesToCheck, dwRead, &dwWritten, NULL);
    CloseHandle(hFile);  
    */
  }

  //
  // restore page protection
  //
  VirtualProtectEx(hProcess, pAddress, dwSize, dwOldProtect, &dwOldProtect);

  free(pBytesInMem);

  return bOK;
}        



AgWallhack g_Wallhack;
static char szDisconnect[] = "disconnect\n";
static char szMicrosoft[] = "Microsoft Corporation";
static char szCDFPS[] = "cd_fps";

static char* s_szGoodDlls[] =
{
  /*
  "\\ag\\cl_dlls\\client.dll", 
  "\\ag\\dlls\\ag.dll",
  "\\ag\\dlls\\hl.dll",
  "\\aghl\\cl_dlls\\client.dll", 
  "\\aghl\\dlls\\ag.dll",
  "\\aghl\\dlls\\hl.dll",
  */
  "\\vgui.dll",
  "\\woncrypt.dll",
  "\\wonauth.dll",
  "\\hl_res.dll",
  "\\hw.dll",
  "\\sw.dll",
  "\\mss32.dll",
  "\\mssv12.dll",
  "\\mssv12.asi",
  "\\mp3dec.asi",
  "\\mssv29.asi",
  "\\steamvalidateuseridtickets.dll",
  "\\voice_miles.dll",
  "\\hl.exe",
  "\\cstrike.exe",
};


static char* s_szBadDlls[] =
{
  "glhack.dll",   //Famous glhack.
  "default.dll",  //Cracked version of famous glhack. This one is encoded... smart fellow :P
  "hl_rem.dll",   //Cracked version of famous glhack. I will stop any version by opening the dll anyway.
  "oldogl32.dll", //Famous glhack.
  "oglhack.dll",   
  "ogl.dll", 
  "sw!zz3r.dll",   
  "hookhl.dll",   
  "ogc.dll", 
};

static char* s_szBadStrings[] =
{
  "Z2xoYWNr", //glhack
  "b3BlbmdsLmluaQAA", //opengl.ini
  "VFdDaGVhdAAA", //TWCheat
  "Qi50ZXJhcGh5", //B.teraphy
  "RmxhdXR6", //Flautz
  "c3chenozcgAA", //sw!zz3r
  "QU5BS2lO", //ANAKiN
  "VVBYIQAA", //UPX!
  "Yzpcb3BlbmdsMzIuZGxs", //c:\opengl32.dll
  "aGxoLmRsbAAA", //hlh.dll
  "R1JpTS1GX0gA", //GRiM-F_H
  "Q2hyb21heFMA", //ChromaxS
  "b2djLmRsbAAA", //ogc.dll
  "ZVohJDd2", //eZ!$7v  Swizz hack
  "Y29kZXJzLmRsbAAA", //coders.dll wh_beta4, wh_beta5
  "b2djLmNmZwAA", //ogc.cfg
  "eHF6MgAA",     //xqz2 - xqz2_b71
  "eHFiNgAA",     //xqb6 - xqz2_b80
  "cEBncmFt",     //p@gram - XQZ2Beta85
  "W09HQ10A",     //[OGC] - from ogc 7
  "Sm9vbHoA",     //Joolz - from ogc 8 - 
  "dGhyb3VnaCB3YWxs",  //through wall - from ogc 8 - 
  "UlNEU/s19Llq",     //RSDSû5ô¹j from 187 Wallhack
  "d2FsbGhhY2sA",     //wallhack from SyFWallHack.dll
  "W1VHQ10A",         //[UGC] from [FuRy]-immortal
  "R0xIYWNr",         //GLHack
  "XDE4N0hBQ0sA",     //\187HACK - 187 version 1.5, 2.0, xqz
  "THRmeGhvb2sA",     //Ltfxhook - version 4
  "c2VjdXJlLmluaQAA", //secure.ini - nc-secure
  "bWFkQ29kZUhvb2sA", //madCodeHookLib - www.madshi.net hooking library that some seem to use.
  "amFpbmEA", //jaina - comes from exprtl0.dll
  "bmV0LWNvZGVycwAA", //net-coders - from net coderse hack
//  "YWltaGFjawAA",     //aimhack - 187 version xqz
  "V2FsbGhhY2sA",     //Wallhack - from many hacks.
  "aG9va2VyLmRsbAAA", //hooker.dll
  "VW5ob29rZXIA",     //Unhooker
//in cheats.dat  "V2lyZWZyYW1l",     //Wireframe - from Net coders hack.
  /*
  "T0dDAAAA",	    //OGC
  "b2djAAAA",     //ogc
  
  */
  /*
http://www.zone.ee/kunnchat/
http://www.unknowncheats.com/
http://www.cheat-network.net/chnetphp/
  */
};

AgWallhack::AgWallhack()
{
#ifndef _DEBUG
#ifndef AG_TESTCHEAT
  if (IsDebuggerPresent())
	  exit(-1);
#endif
#endif

  m_dwHLAddressToValidate = 0L;
  int i = 0;
  for (i = 0; i < sizeof(s_szBadStrings)/sizeof(s_szBadStrings[0]); i++)
    AddBadString(s_szBadStrings[i]);

  for (i = 0; i < sizeof(s_szBadDlls)/sizeof(s_szBadDlls[0]); i++)
    AddBadDll(s_szBadDlls[i]);

  char szHalfLife[MAX_PATH];
  GetModuleFileName(GetModuleHandle("client.dll"),szHalfLife,sizeof(szHalfLife));
  szHalfLife[strrchr(szHalfLife,'\\') - szHalfLife] = '\0';
  szHalfLife[strrchr(szHalfLife,'\\') - szHalfLife] = '\0';
  szHalfLife[strrchr(szHalfLife,'\\') - szHalfLife] = '\0';
  strlwr(szHalfLife);
  char szDll[MAX_PATH];
  for (i = 0; i < sizeof(s_szGoodDlls)/sizeof(s_szGoodDlls[0]); i++)
  {
    sprintf(szDll,"%s%s",szHalfLife,s_szGoodDlls[i]);
    m_setGoodDlls.insert(szDll);
  }

  /*
  unsigned short usSize = 0; 
  unsigned char szSearch[256];
  AgBase64Decode("hitewalls",9,szSearch,usSize);
	OutputDebugString((char*)szSearch);
  OutputDebugString("\n");
  */
  /*

  AgBase64Encode((unsigned char*)"Wallhack",8,szDll);
	OutputDebugString(szDll);
  OutputDebugString("\n");
  */
/*
  AgBase64Encode((unsigned char*)"ownlinecheating",7,szDll);
	OutputDebugString(szDll);
  OutputDebugString("\n");
  */


  m_bDoneCheck = false;
  m_iFiles = 0;
  m_dwBytes = 0;
}

AgWallhack::~AgWallhack()
{
  m_setBadStrings.clear();
  m_setBadDlls.clear();
  m_setGoodDlls.clear();
  m_setGoodSystemDlls.clear();
}  

void AgWallhack::AddBadDll(const char* pszDll)
{
  if (pszDll && 0 != strlen(pszDll))
    m_setBadDlls.insert(pszDll);
}

void AgWallhack::AddBadString(const char* pszString)
{
  if (pszString && 0 != strlen(pszString))
    m_setBadStrings.insert(pszString);
}

void AgWallhack::AddBadStrings(const char* pszStrings)
{
  char* pszStringsTemp = strdup(pszStrings);

  char* pszCheatString = strtok( pszStringsTemp, "\n");
  while (pszCheatString != NULL)
  {
    AgString strCheatString = pszCheatString;
    AgTrim(strCheatString);
    if (strCheatString.length())
      AddBadString(strCheatString.c_str());
    pszCheatString = strtok( NULL, "\n");
  }

  free(pszStringsTemp);
}


bool AgWallhack::InitToolHelp32()
{
  if (m_hKernel32 && m_lpfCreateToolhelp32Snapshot && m_lpfModule32First && m_lpfModule32Next)
    return true;

  m_hKernel32 = ::LoadLibrary("kernel32.dll");
  if (NULL == m_hKernel32)
    return false;

  m_lpfCreateToolhelp32Snapshot = (CREATETOOLHELP32SNAPSHOT) ::GetProcAddress(m_hKernel32,"CreateToolhelp32Snapshot");
  m_lpfModule32First            = (MODULE32FIRST)            ::GetProcAddress(m_hKernel32,"Module32First");
  m_lpfModule32Next             = (MODULE32NEXT)             ::GetProcAddress(m_hKernel32,"Module32Next");
  m_lpfProcess32First           = (PROCESS32FIRST)           ::GetProcAddress(m_hKernel32,"Process32First");
  m_lpfProcess32Next            = (PROCESS32NEXT)            ::GetProcAddress(m_hKernel32,"Process32Next");
  
  if (NULL == m_lpfCreateToolhelp32Snapshot ||
      NULL == m_lpfModule32First  ||
      NULL == m_lpfModule32Next   ||
      NULL == m_lpfProcess32First ||
      NULL == m_lpfProcess32Next)
  {
    ::FreeLibrary(m_hKernel32);
    m_hKernel32 = NULL;
    return false;
  }
  return true;
}

bool AgWallhack::Check()
{
#ifdef _DEBUG
  //return true;
  //m_bDoneCheck = true;
  //HMODULE x1 = LoadLibrary("E:/Dev/cheats/nC 2.1/nC-Hack.dll");
#endif 
  if (m_bDoneCheck)
    return true;


  if (!InitToolHelp32())
  {
    char szMessage[] = "Cheat check: This version of Windows is not supported\n";
    ServerCmd("say <AG Mod> this version of Windows is not supported.");
    AgLog(szMessage);
    ConsolePrint(szMessage);
    ClientCmd(szDisconnect);
    return false;
  }

  int iCheck = InternalCheck();
#ifdef _DEBUG
  char szChecked[128];
  sprintf(szChecked,"Checked for %d cheats in %d files with total data of %ld bytes\n",(int)(m_setBadStrings.size() + m_setBadDlls.size()),m_iFiles,m_dwBytes);
  ConsolePrint(szChecked);
#endif

  if (0 > iCheck)
  {
    char szMessage[512];
    sprintf(szMessage,"say <AG Mod> Autodisconnected, error in installation.\n");
    ServerCmd(szMessage);
    sprintf(szMessage,"Error in installation %s. (Error = %d)\n",m_sDll.c_str(),iCheck);
    AgLog(szMessage);
    ConsolePrint(szMessage);
    ClientCmd(szDisconnect);
    return false;
  }

  if (0 != iCheck)
  {
    char szMessage[512];
    sprintf(szMessage,"say <AG Mod> Disconnected for using invalid module %s.\n",m_sDll.c_str());
    ServerCmd(szMessage);
    sprintf(szMessage,"Cheat check: %s is not allowed in AG. (Code = %d)\n",m_sDll.c_str(),iCheck);
    AgLog(szMessage);
    ConsolePrint(szMessage);
    //AgSendICQ(szMessage);
    //AgSendMail(szMessage);
#ifndef _DEBUG
    SendMessageToIRC(szMessage);
    ClientCmd(szDisconnect);
    return false;
#endif
  }

//  m_bDoneCheck = true;
  return true;
}

void DumpToFile(MODULEENTRY32* pME)
{
  char	szFile[MAX_PATH];
  sprintf(szFile,"%s/filedump.txt",AgGetDirectory());
  FILE* pFile = fopen(szFile,"a+");
  if (!pFile)
  {
    return;
  }
  
  fwrite(pME->modBaseAddr,sizeof(BYTE),pME->modBaseSize,pFile);
  fflush(pFile);
  fclose(pFile);
}

int AgWallhack::InternalCheck()
{
#ifndef _DEBUG
#ifndef AG_TESTCHEAT
  if (IsDebuggerPresent())
  {
    m_sDll = "debugger";
    return -1;
  }
#endif
#endif


  char szSystemDir[MAX_PATH];
  GetSystemDirectory(szSystemDir,sizeof(szSystemDir));
  strlwr(szSystemDir);
  
  m_sDll = "";
#ifdef _DEBUG
  DWORD dwTime = GetTickCount();
#endif
  unsigned char szSearch[256];
  szSearch[0] = '\0';

  HANDLE hSnapShot = m_lpfCreateToolhelp32Snapshot(TH32CS_SNAPMODULE,0);
  if (INVALID_HANDLE_VALUE == hSnapShot)
  {
    m_sDll = "toolhelp was not found";
    return -2;
  }

  MODULEENTRY32 me;
  me.dwSize = sizeof(MODULEENTRY32);
  HMODULE hCurrent = ::GetModuleHandle("client.dll");

  if (!GetModuleHandle("hl.exe") && !GetModuleHandle("cstrike.exe"))
  {
    m_sDll = "hl.exe or cstrike.exe was not found";
    return -3;
  }

  BYTE byHokoHack[1] = {0xE8}; 
  BYTE byRegularJumpHack[1] = {0xE9}; 
  if ( !CheckHooks("opengl32.dll", "glBegin", byHokoHack, sizeof(byHokoHack))
    || !CheckHooks("opengl32.dll", "glBegin", byRegularJumpHack, sizeof(byRegularJumpHack))
    )
  {
    m_sDll = "opengl32.dll (patched)";
    return 11;
  }

  m_iFiles = 0;
  m_dwBytes = 0;

  bool bCorrectAddress = false;
  if (m_lpfModule32First(hSnapShot,&me))
  {
    do
    {
      m_iFiles++;
      m_dwBytes += me.modBaseSize;

      if (hCurrent != me.hModule)
      {
#ifdef _DEBUG 
        char szTime[MAX_PATH];
        if ("" == m_sDll)
        {
          sprintf(szTime,"%s %d ms\n",m_sDll.c_str(),int((GetTickCount() - dwTime)));
          AgLog(szTime);
          dwTime = GetTickCount();
        }
#endif //_DEBUG    

        char szFileName[MAX_PATH];
        strcpy(szFileName,me.szExePath);
        strlwr(szFileName);
        m_sDll = szFileName;

        DWORD dwAddressInModule = (DWORD)me.modBaseAddr;
        if (m_dwHLAddressToValidate >= dwAddressInModule
         && m_dwHLAddressToValidate <= (dwAddressInModule + me.modBaseSize))
        {
          bCorrectAddress = (0 == stricmp(&szFileName[strlen(szFileName)-6],"hl.exe")) || (0 == stricmp(&szFileName[strlen(szFileName)-11],"cstrike.exe"));
        }

        bool bOpenGL = 0;
        if (strlen(szFileName) > 11)
          bOpenGL = 0 == strcmp(&szFileName[strlen(szFileName)-12],"opengl32.dll");

        //Extract version resource.
        AgVersionInfo vi;
        vi.LoadVersionInfo(szFileName);

        //Check if microsoft dll. 
        if (!vi.HasErrors())
        {
          if (0 == strcmp(szMicrosoft, vi.CompanyName()))
          {
            if (!bOpenGL && me.modBaseSize > 150000L) //Most cheat dlls are usually less than 150000kb.
              continue;
          }
        }

        if (0 == strcmp(&szFileName[strlen(szFileName)-11],"shimeng.dll"))
          continue;
        
        //Skip over some HL dll's
        bool bGood = false;
        AgStringSet::iterator itrGoodDlls;
        for (itrGoodDlls = m_setGoodDlls.begin() ;itrGoodDlls != m_setGoodDlls.end() && !bGood; ++itrGoodDlls)
        {
          if (0 == strcmp(m_sDll.c_str(),(*itrGoodDlls).c_str()))
            bGood = true;
        }
        
        if (bGood)
          continue;
        
#ifndef AG_TESTCHEAT
        //Check bad dll's - practicly worthless :P -  the rutine to open and scan the dll inside below is much better.
        AgStringSet::iterator itrWallhackDlls;
        for (itrWallhackDlls = m_setBadDlls.begin() ;itrWallhackDlls != m_setBadDlls.end(); ++itrWallhackDlls)
        {
          if (0 == strcmp(m_sDll.c_str(),(*itrWallhackDlls).c_str()))
          {
            AgLog(("Wallhack found in " + m_sDll + "\n").c_str());
            return 1; //He used an obvious cheat.
          }
        }
#endif
        
#ifdef _DEBUG 
        bool bDump = false;
        if (bDump)
          DumpToFile(&me);
#endif //_DEBUG   
        
        if (bOpenGL)
        {
          //Check if dll is in windows system directory. Hacks sometimes put the passthru in c:/
          if (NULL == strstr(m_sDll.c_str(),szSystemDir))
            return 2; //The opengl32.dll driver wasn't in windows system directory.
          
          //Check file size. Under 600k is wrong.
          //Check http://support.microsoft.com/servicedesks/fileversion/dllinfo.asp?sd=MSDN
          if (600000 > me.modBaseSize)
            return 3;  //This dll is way to small to be the standard opengl32.dll. 
          
          //Extract version resource.
          if (vi.HasErrors())
            return 4; //Typically a passthruu dll's aint got any version resource.
          
          //Check the version info for microsoft string. Can easily be done by hackers though
          if (0 != strcmp(szMicrosoft, vi.CompanyName()))
            return 5; //Should be Microsoft Corporation.
          
          //Check the productversion.
          int iMajor, iMinor, iMicro, iState;
          vi.ProductVersion(iMajor, iMinor, iMicro, iState);
          if (iMajor < 4)
            return 6; //Should be 4 or more. Cant do any more serious check than this because of the different flavors of windows...
        }
        
#ifdef _DEBUG
        AgLog(m_sDll.c_str());
#endif
        //Oki - brute force method to check for hacks... its easy to rename dll's you know...
        AgStringSet::iterator itrWallhackStrings;
        unsigned long i = 0;
        unsigned short x = 0;
        bool bFoundHack = true;
        unsigned char* pBuffer = NULL;
        for (itrWallhackStrings = m_setBadStrings.begin() ;itrWallhackStrings != m_setBadStrings.end(); ++itrWallhackStrings)
        {
          unsigned short usSize = 0; 
          AgBase64Decode((*itrWallhackStrings).c_str(),(*itrWallhackStrings).size(),szSearch,usSize);
          const unsigned char* pszSearch = szSearch;
          i = 0;
          unsigned long lCount = me.modBaseSize - usSize;
          pBuffer = me.modBaseAddr;
          do 
          {
            //bFoundHack = FastCmp(pBuffer,pszSearch,usSize);
            bFoundHack = true;
            x = 0;
            do
            {
              bFoundHack = *(pBuffer+x) == *(pszSearch+x);
              ++x;
            }
            while (bFoundHack && x < usSize);
            
            if (bFoundHack)
            {
              AgLog(("Wallhack found in " + m_sDll + "\n").c_str());
              return 7;
            }
            ++pBuffer;
            ++i;
          }
          while (i < lCount);
        }
      }
      me.dwSize = sizeof(MODULEENTRY32);
    } 
    while (m_lpfModule32Next(hSnapShot,&me));
    
    ::CloseHandle(hSnapShot);
  }
  
  if (!bCorrectAddress && !gEngfuncs.pfnGetCvarPointer(szCDFPS))
  {
    m_sDll = "ogc type hooking library";
    return 10;
  }
  return 0;
  /*


  HANDLE hSnapShotProcess = m_lpfCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
  if (INVALID_HANDLE_VALUE == hSnapShotProcess)
    return -1;

  PROCESSENTRY32 pe;
  pe.dwSize = sizeof(PROCESSENTRY32);
  DWORD th32ProcessID = GetCurrentProcessId();

  unsigned char* pFileBuffer = NULL;
  DWORD dwBufferSize = 0;
  if (m_lpfProcess32First(hSnapShotProcess,&pe))
  {
    do
    {
      m_sDll = pe.szExeFile;
      m_iFiles++;
      if (th32ProcessID != pe.th32ProcessID)
      {
        AgVersionInfo vi;
        vi.LoadVersionInfo(pe.szExeFile);

        //Check if microsoft dll. Let's hope that no hacker reads this source :P
        if (!vi.HasErrors())
        {
          AgString sCompanyName = vi.CompanyName();
          AgTrim(sCompanyName);
          if ("Microsoft Corporation" == vi.CompanyName()) 
              continue;
        }

        //Load the dll into a buffer so it's possible to scan the contence.
        HANDLE hFile = CreateFile(pe.szExeFile,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,0);
        if (!hFile)
          return 9; //Could not load the exe.

        DWORD dwSize = GetFileSize(hFile,NULL);
        m_dwBytes += dwSize;

        if (dwBufferSize < dwSize)
        {
          if (pFileBuffer)
            free(pFileBuffer);
          dwBufferSize = dwSize;
          pFileBuffer = (unsigned char*)malloc(dwBufferSize);
        }

        DWORD dwRead = 0;
        if (ReadFile(hFile,(void*)pFileBuffer,dwSize,&dwRead,NULL))
        {
          if (dwRead == dwSize)
          {
            //Oki - brute force method to check for hacks... its easy to rename dll's you know...
            AgStringSet::iterator itrWallhackStrings;
            unsigned long i = 0;
            unsigned short x = 0;
            bool bFoundHack = true;
            unsigned char* pBuffer = NULL;
            for (itrWallhackStrings = m_setBadStrings.begin() ;itrWallhackStrings != m_setBadStrings.end(); ++itrWallhackStrings)
            {
              unsigned short usSize = 0; 
              AgBase64Decode((*itrWallhackStrings).c_str(),(*itrWallhackStrings).size(),szSearch,usSize);
              const unsigned char* pszSearch = szSearch;
              i = 0;
              unsigned long lCount = dwSize - usSize;
              pBuffer = pFileBuffer;
              do 
              {
                //bFoundHack = FastCmp(pBuffer,pszSearch,usSize);
                bFoundHack = true;
                x = 0;
                do
                {
                  bFoundHack = *(pBuffer+x) == *(pszSearch+x);
                  ++x;
                }
                while (bFoundHack && x < usSize);

                if (bFoundHack)
                {
                  AgLog(("Wallhack found in " + m_sDll + "\n").c_str());
                  return 7;
                }
                ++pBuffer;
                ++i;
              }
              while (i < lCount);
            }

          }
          else
          {
          }
          
        }
        else
        {
        }

        CloseHandle(hFile);
      }
      
      pe.dwSize = sizeof(PROCESSENTRY32);
    } 
    while (TRUE == m_lpfProcess32Next(hSnapShotProcess,&pe));

    ::CloseHandle(hSnapShotProcess);
  }
  free(pFileBuffer);
  */
}


#ifdef DEBUG
#define IRC_CHANNEL "#aghl.beta"
#else
#define IRC_CHANNEL "#aghl"
#endif

#ifdef AG_TESTCHEAT
#undef IRC_CHANNEL
#define IRC_CHANNEL "#aghl.beta"
#endif

void AgWallhack::SendMessageToIRC(const char* szMessage)
{
  char szCommand[512];
  sprintf(szCommand,"irc_nick \"AGHL|CHEATER%0d\"",gEngfuncs.pfnRandomLong(0,99));
  ClientCmd(szCommand);

  sprintf(szCommand,"irc_server \"irc.quakenet.eu.org\"");
  ClientCmd(szCommand);

  sprintf(szCommand,"irc_port \"6667\"");
  ClientCmd(szCommand);

  sprintf(szCommand,"irc_autojoin \"%s\"",IRC_CHANNEL);
  ClientCmd(szCommand);
 
  int iPlayer = gEngfuncs.GetLocalPlayer()->index;	// Get the local player's index

  sprintf(szCommand,"irc_autocommand2 \"/msg %s %s (Authid=%s) %s\"", IRC_CHANNEL, gEngfuncs.PlayerInfo_ValueForKey(iPlayer,"name"), AgGetAuthID(iPlayer).c_str(), "was caught with a cheat. For more info see message below.");
  ClientCmd(szCommand);

  sprintf(szCommand,"irc_autocommand3 \"/msg %s %s\"", IRC_CHANNEL, szMessage);
  ClientCmd(szCommand);

  ClientCmd("ircconnect2");
} 

#endif //AG_USE_CHEATPROTECTION
//-- Martin Webrant



