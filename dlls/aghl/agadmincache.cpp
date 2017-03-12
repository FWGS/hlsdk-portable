//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

#include "agadmin.h"
#include "agadmincache.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DLL_GLOBAL AgAdminCache AdminCache; //The one and only

AgAdminCache::AgAdminCache()
{

}

AgAdminCache::~AgAdminCache()
{
  //Delete all.
  for (AgAdminList::iterator itrAdmins = m_lstAdmins.begin() ;itrAdmins != m_lstAdmins.end(); ++itrAdmins)
    delete *itrAdmins;
  m_lstAdmins.clear();
}

void AgAdminCache::RestoreAdmin(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
  if (g_bLangame)
	return;

  for (AgAdminList::iterator itrAdmins = m_lstAdmins.begin() ;itrAdmins != m_lstAdmins.end(); ++itrAdmins)
  {
    if ((*itrAdmins)->m_sAuthID == pPlayer->GetAuthID())
    {
      //Set admin flag.
      pPlayer->SetIsAdmin(true);
      break;
    }
  }
}

void AgAdminCache::Load()
{
  if (0 != m_lstAdmins.size())
    return; //Already loaded.
  
  char	szFile[MAX_PATH];
  char	szData[4096];
  sprintf(szFile, "%s/admin.txt", AgGetDirectory());
	FILE* pFile = fopen(szFile,"r");
	if (!pFile)
	{
		return;
	}

  enum enumParseState { User, Pass, AuthID };
  enumParseState ParseState = User;
  AgAdmin* pAdmin = new AgAdmin;
  int iRead = fread(szData,sizeof(char),sizeof(szData)-2,pFile);
  if (0 >= iRead)
    return;
  szData[iRead] = '\0';

  char* pszParse = NULL;
  pszParse = strtok(szData, "#");
  if (pszParse)
  {
    while (pszParse)
    {
      if (User == ParseState)
      {
        pAdmin->m_sAdmin = pszParse; 
        ParseState = Pass;
      }
      else if (Pass == ParseState)
      {
        pAdmin->m_sPassword = pszParse; 
        ParseState = AuthID;
      }
      else if (AuthID == ParseState)
      {
        pAdmin->m_sAuthID = pszParse;
        m_lstAdmins.push_back(pAdmin);
        pAdmin = new AgAdmin;
        ParseState = User;
      }
      pszParse = strtok(NULL, "#");
    }
  }
  delete pAdmin; 

  fclose(pFile);
}


void AgAdminCache::Save(CBasePlayer* pPlayer)
{
  if (0 == m_lstAdmins.size())
    return;

	char	szFile[MAX_PATH];
  sprintf(szFile, "%s/admin.txt", AgGetDirectory());
	FILE* pFile = fopen(szFile,"wb");
	if (!pFile)
	{
    AgConsole(UTIL_VarArgs("Couldn't create/save %s.",szFile),pPlayer);
		return;
	}

  //Loop and write the file.
  for (AgAdminList::iterator itrAdmins = m_lstAdmins.begin() ;itrAdmins != m_lstAdmins.end(); ++itrAdmins)
  {
    //Append.
    AgAdmin* pAdmin = *itrAdmins;
    fprintf(pFile,"%s#%s#%s#",pAdmin->m_sAdmin.c_str(),pAdmin->m_sPassword.c_str(),pAdmin->m_sAuthID.c_str());
  }

  fflush(pFile);
  fclose(pFile);
}

void AgAdminCache::AddAdmin(const AgString& sAdmin, const AgString& sPassword, CBasePlayer* pPlayer)
{
  AgAdmin* pAdmin = NULL;
  //Check if same name is used before.
  bool bNameAlreadyInUse = false;

  for (AgAdminList::iterator itrAdmins = m_lstAdmins.begin() ;itrAdmins != m_lstAdmins.end(); ++itrAdmins)
  {
    if ((*itrAdmins)->m_sAdmin == sAdmin)
    {
      bNameAlreadyInUse = true;
      break;
    }
  }

  if (bNameAlreadyInUse)
  {
    AgConsole(UTIL_VarArgs("Admin \"%s\" already exist.",sAdmin.c_str()),pPlayer);
  }
  else
  {
    //Name was ok.
    pAdmin = new AgAdmin;
    pAdmin->m_sAdmin = sAdmin;
    pAdmin->m_sPassword = sPassword;
    pAdmin->m_sAuthID = "";
    m_lstAdmins.push_back(pAdmin);
    AgConsole(UTIL_VarArgs("Added admin %s/%s.",pAdmin->m_sAdmin.c_str(),pAdmin->m_sPassword.c_str()),pPlayer);
    Save(pPlayer);
  }
}

void AgAdminCache::ListAdmins(CBasePlayer* pPlayer)
{
  if (0 == m_lstAdmins.size())
  {
    AgConsole("There are no admins.",pPlayer);
  }
  else
  {
    for (AgAdminList::iterator itrAdmins = m_lstAdmins.begin() ;itrAdmins != m_lstAdmins.end(); ++itrAdmins)
    {
      AgConsole(UTIL_VarArgs("%s",(*itrAdmins)->m_sAdmin.c_str()),pPlayer);
    }
  }
}

void AgAdminCache::DelAdmin(const AgString& sAdmin,CBasePlayer* pPlayer)
{
  if (0 == m_lstAdmins.size())
  {
    AgConsole("There are no admins.",pPlayer);
  }
  else
  {
    for (AgAdminList::iterator itrAdmins = m_lstAdmins.begin() ;itrAdmins != m_lstAdmins.end(); ++itrAdmins)
    {
      AgAdmin* pAdmin = *itrAdmins; 
      if (pAdmin->m_sAdmin == sAdmin)
      {
        AgConsole(UTIL_VarArgs("Deleted admin %s.",sAdmin.c_str()),pPlayer);
        m_lstAdmins.erase(itrAdmins);
        delete pAdmin;
        Save(pPlayer);
        break;
      }
    }
  }
}


void AgAdminCache::Auth(const AgString& sAdmin, const AgString& sPassword, CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  for (AgAdminList::iterator itrAdmins = m_lstAdmins.begin() ;itrAdmins != m_lstAdmins.end(); ++itrAdmins)
  {
    AgAdmin* pAdmin = *itrAdmins; 
 
    if (pAdmin->m_sAdmin == sAdmin && pAdmin->m_sPassword == sPassword)
    {
      //Set as admin
      pPlayer->SetIsAdmin(true);
      //Save autoid - used for auto authentication.
      pAdmin->m_sAuthID = pPlayer->GetAuthID();

      AgConsole("You are now authenticated as an admin.",pPlayer);
      Save(pPlayer);
      break;
    }
  }
}

void AgAdminCache::Newpass(const AgString& sOldpass, const AgString& sPassword, CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  if (!pPlayer->IsAdmin() || 0 == sPassword.size())
    return;

  for (AgAdminList::iterator itrAdmins = m_lstAdmins.begin() ;itrAdmins != m_lstAdmins.end(); ++itrAdmins)
  {
    AgAdmin* pAdmin = *itrAdmins; 
 
    if (pAdmin->m_sAuthID == pPlayer->GetAuthID() && pAdmin->m_sPassword == sOldpass)
    {
      //Set new pass
      pAdmin->m_sPassword = sPassword;

      //Save auth - used for auto authentication.
       pAdmin->m_sAuthID = pPlayer->GetAuthID();

      AgConsole("Changed password.",pPlayer);
      Save(pPlayer);
      break;
    }
  }
}

//-- Martin Webrant
