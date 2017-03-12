//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "parsemsg.h"
#include "agglobal.h"
#include "aghudlocation.h"


DECLARE_MESSAGE(m_Location, Location );
DECLARE_MESSAGE(m_Location, InitLoc );
DECLARE_COMMAND(m_Location, OpenLocation );
DECLARE_COMMAND(m_Location, CloseLocation );
DECLARE_COMMAND(m_Location, AddLocation );
DECLARE_COMMAND(m_Location, DeleteLocation );
DECLARE_COMMAND(m_Location, ShowLocations );

Vector vPlayerLocations[MAX_PLAYERS+1];

int AgHudLocation::Init(void)
{
  m_fAt = 0;
  m_fNear = 0;
	m_iFlags = 0;
  m_szMap[0] = '\0';

  for (int i = 1; i <= MAX_PLAYERS; i++)
    vPlayerLocations[i] = Vector(0,0,0);

	gHUD.AddHudElem(this);

  HOOK_MESSAGE( Location );
  HOOK_MESSAGE( InitLoc );
#ifdef _DEBUG
  HOOK_COMMAND("+location",OpenLocation);
  HOOK_COMMAND("-location",CloseLocation);
#endif
  HOOK_COMMAND("agaddloc",AddLocation);
  HOOK_COMMAND("agdelloc",DeleteLocation);
  HOOK_COMMAND("aglistloc",ShowLocations);
	return 1;
};

AgHudLocation::~AgHudLocation()
{
  //Delete all.
  for (AgLocationList::iterator itrLocations = m_lstLocations.begin() ;itrLocations != m_lstLocations.end(); ++itrLocations)
    delete *itrLocations;
  m_lstLocations.clear();
}

int AgHudLocation::VidInit(void)
{
	return 1;
};

void AgHudLocation::Reset(void)
{
  m_iFlags &= ~HUD_ACTIVE;
}

int AgHudLocation::Draw(float fTime)
{
	cl_entity_t* pPlayer = gEngfuncs.GetLocalPlayer();	// Get the local player's index
  int r, g, b;
  UnpackRGB(r,g,b, RGB_GREENISH);
  int iPos = 5;
  for (int i = 1; i < gEngfuncs.GetMaxClients(); i++ )
  {
    if (i == pPlayer->index)
      continue;

    if ('\0' != g_PlayerExtraInfo[i].teamname[0] &&
        '\0' != g_PlayerExtraInfo[i].teamname[pPlayer->index] &&
        0 == stricmp(g_PlayerExtraInfo[i].teamname, g_PlayerExtraInfo[pPlayer->index].teamname))
    {
      //Show his location - it's a team m8.
      char szText[128];
      sprintf(szText,"%s [%s]",g_PlayerInfoList[i].name,Location(vPlayerLocations[i]).c_str());
      gHUD.DrawHudString(ScreenWidth/20, ScreenHeight/8 + gHUD.m_scrinfo.iCharHeight*iPos,ScreenWidth,szText,r,g,b);
      iPos++;
    }
  }

  return 5 != iPos;
}


void AgHudLocation::UserCmd_OpenLocation()
{
  m_iFlags |= HUD_ACTIVE;
}

void AgHudLocation::UserCmd_CloseLocation()
{
  m_iFlags &= ~HUD_ACTIVE;
}

void AgHudLocation::UserCmd_AddLocation()
{
  if (2 == gEngfuncs.Cmd_Argc())
  {
    AgString sLocation = gEngfuncs.Cmd_Argv(1);
    if (0 == sLocation.size())
      return;
  
    AgLocation* pLocation = new AgLocation;
    pLocation->m_sLocation = sLocation;
    //pLocation->m_vPosition = vPlayerLocations[gEngfuncs.GetLocalPlayer()->index];
    pLocation->m_vPosition = 	gEngfuncs.GetLocalPlayer()->attachment[0];
  
    m_lstLocations.push_back(pLocation);
    pLocation->Show();
  
    Save();
  
    char szMsg[128];
    sprintf(szMsg,"Added Location %s.\n",(const char*)sLocation.c_str());
    ConsolePrint(szMsg);
  }
}

void AgHudLocation::UserCmd_DeleteLocation()
{
  if (2 == gEngfuncs.Cmd_Argc())
  {
    AgString sLocation = gEngfuncs.Cmd_Argv(1);
    if (0 == sLocation.size())
      return;
  
    for (AgLocationList::iterator itrLocations = m_lstLocations.begin() ;itrLocations != m_lstLocations.end(); ++itrLocations)
    {
      if (0 == stricmp((*itrLocations)->m_sLocation.c_str(),sLocation.c_str()))
      {
        char szMsg[128];
        sprintf(szMsg,"Deleted Location %s.\n",(const char*)sLocation.c_str());
        ConsolePrint(szMsg);
      
        AgLocation* pLocation = *itrLocations; 
        m_lstLocations.erase(itrLocations);
        delete pLocation;
        Save();
        break;
      } 
    }
  }
}

void AgHudLocation::UserCmd_ShowLocations()
{
  for (AgLocationList::iterator itrLocations = m_lstLocations.begin() ;itrLocations != m_lstLocations.end(); ++itrLocations)
  {
    char szMsg[128];
    sprintf(szMsg,"%s\n",(const char*)(*itrLocations)->m_sLocation.c_str());
    ConsolePrint(szMsg);
    (*itrLocations)->Show();
  }
}


void AgHudLocation::InitDistances()
{	
  if (2 > m_lstLocations.size())
    return; //No use.
  
  float fMinDistance = -1;
  float fMaxdistance = 0;
  
  //Calculate max/min distance between all locations.
  for (AgLocationList::iterator itrLocations = m_lstLocations.begin() ;itrLocations != m_lstLocations.end(); ++itrLocations)
  {
    for (AgLocationList::iterator itr = m_lstLocations.begin() ;itr != m_lstLocations.end(); ++itr)
    {
      if (*itrLocations != *itr)
      {
        float fDistance = ((*itr)->m_vPosition - (*itrLocations)->m_vPosition).Length();
        
        if (fDistance < fMinDistance || -1 == fMinDistance)
          fMinDistance = fDistance;
        
        if (fDistance > fMaxdistance)
          fMaxdistance = fDistance;
      }
      
    }
  }
  
  //Now calculate when you are at/near a location or at atleast when its closest.
  m_fAt = fMinDistance / 2; //You are at a location if you are one fourth between to locations.
  m_fNear = fMinDistance / 1.1; //Over halfway of the mindistance you are at the "side".
}

bool AgHudLocation::NearestLocation(const Vector& vPosition, AgLocation*& pLocation, float& fNearestdistance)
{	
  fNearestdistance = -1;
  pLocation = NULL;
  
  for (AgLocationList::iterator itrLocations = m_lstLocations.begin() ;itrLocations != m_lstLocations.end(); ++itrLocations)
  {
    float fDistance = (vPosition - (*itrLocations)->m_vPosition).Length();
    
    if (fDistance < fNearestdistance || -1 == fNearestdistance)
    {
      fNearestdistance = fDistance;
      pLocation = *itrLocations;
    }
  }
  
  return NULL != pLocation;
}

extern cvar_t* g_pcl_location_keywords;

AgString AgHudLocation::Location(const Vector& vLocation)
{
  AgString sLocation;
  if (2 > m_lstLocations.size())
    return sLocation;
  
  AgLocation* pLocation = NULL;
  float fNearestDistance = 0;
  
  if (NearestLocation(vLocation, pLocation, fNearestDistance))
  {
    if (pLocation)
    {
      /*
#ifdef _DEBUG
      pLocation->Show();
#endif
      */
      if (fNearestDistance < m_fAt || g_pcl_location_keywords->value < 1)
      {
        sLocation = pLocation->m_sLocation;
      }
      else if (fNearestDistance < m_fNear)
      {
        sLocation.append("Near ");
        sLocation.append(pLocation->m_sLocation);
      }
      else 
      {
        sLocation.append(pLocation->m_sLocation);
        sLocation.append(" Side");
      }
    }
  }
  
  return sLocation;
}

void AgHudLocation::Load()
{
  for (AgLocationList::iterator itrLocations = m_lstLocations.begin() ;itrLocations != m_lstLocations.end(); ++itrLocations)
    delete *itrLocations;
  m_lstLocations.clear();
  
  char	szData[8196];

  char	szFile[MAX_PATH];
  sprintf(szFile,"%s/locs/%s.loc",AgGetDirectory(),m_szMap);
  FILE* pFile = fopen(szFile,"r");
  if (!pFile)
  {
    // file error
    char szMsg[128];
    sprintf(szMsg,"Couldn't open location file %s.\n",szFile);
    ConsolePrint(szMsg);
    return;
  }
  
  enum enumParseState { Location, X, Y, Z };
  enumParseState ParseState = Location;
  AgLocation* pLocation = new AgLocation;
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
      if (Location == ParseState)
      {
        pLocation->m_sLocation = pszParse; 
        ParseState = X;
      }
      else if (X == ParseState)
      {
        pLocation->m_vPosition.x = atof(pszParse); 
        ParseState = Y;
      }
      else if (Y == ParseState)
      {
        pLocation->m_vPosition.y = atof(pszParse); 
        ParseState = Z;
      }
      else if (Z == ParseState)
      {
        pLocation->m_vPosition.z = atof(pszParse); 
        m_lstLocations.push_back(pLocation);
        pLocation = new AgLocation;
        ParseState = Location;
      }
      pszParse = strtok(NULL, "#");
    }
  }
  delete pLocation; 
  
  fclose(pFile);

  InitDistances();
}

void AgHudLocation::Save()
{
  InitDistances();

  if (0 == m_lstLocations.size())
    return;
  
  char	szFile[MAX_PATH];
  sprintf(szFile,"%s/locs/%s.loc",AgGetDirectory(),m_szMap);
  FILE* pFile = fopen(szFile,"wb");
  if (!pFile)
  {
    // file error
    char szMsg[128];
    sprintf(szMsg,"Couldn't create/save location file %s.\n",szFile);
    ConsolePrint(szMsg);
    return;
  }
  
  //Loop and write the file.
  for (AgLocationList::iterator itrLocations = m_lstLocations.begin() ;itrLocations != m_lstLocations.end(); ++itrLocations)
  {
    //Append.
    AgLocation* pLocation = *itrLocations;
    fprintf(pFile,"%s#%f#%f#%f#",(const char*)pLocation->m_sLocation.c_str(),pLocation->m_vPosition.x,pLocation->m_vPosition.y,pLocation->m_vPosition.z);
  }
  
  fflush(pFile);
  fclose(pFile);
}


void AgHudLocation::ParseAndEditSayString(char* pszSay, int iPlayer)
{
  //Make backup
  char* pszText = (char*)malloc(strlen(pszSay)+1);
  char* pszSayTemp = pszText;
  strcpy(pszText,pszSay);
  //Now parse for %L and edit it.

  while (*pszSayTemp) //Dont overflow the string. Stop when its 200 chars.
  {
    if ('%' == *pszSayTemp)
    {
      pszSayTemp++;
      if ('l' == *pszSayTemp || 'L' == *pszSayTemp || 'd' == *pszSayTemp || 'D' == *pszSayTemp )
      {
        //Location files.
        pszSay = pszSay + sprintf(pszSay,Location(vPlayerLocations[iPlayer]).c_str());
        pszSayTemp++;
        continue;
      }
      else
      {
        pszSay[0] = '%';
        pszSay++;
        continue;
      }
    }
    
    *pszSay = *pszSayTemp;
    pszSay++;
    pszSayTemp++;
  }
  *pszSay = '\0';

  free(pszText);
}


int AgHudLocation::MsgFunc_Location(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	vec3_t origin;
  int iPlayer = READ_BYTE();
	for ( int i = 0 ; i < 3 ; i++)
		vPlayerLocations[iPlayer][i] = READ_COORD();
	return 1;
}

int AgHudLocation::MsgFunc_InitLoc(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
  strcpy(m_szMap,READ_STRING());
  Load();
	return 1;
}

//-- Martin Webrant
