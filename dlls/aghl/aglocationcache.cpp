//++ BulliT

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"gamerules.h"

#ifdef AG_NO_CLIENT_DLL

#include	"aglocationcache.h"

AgLocationCache::AgLocationCache()
{
};

AgLocationCache::~AgLocationCache()
{
  //Delete all.
  for (AgLocationList::iterator itrLocations = m_lstLocations.begin() ;itrLocations != m_lstLocations.end(); ++itrLocations)
    delete *itrLocations;
  m_lstLocations.clear();
}


bool AgLocationCache::NearestLocation(const Vector& vPosition, AgLocation*& pLocation, float& fNearestdistance)
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

AgString AgLocationCache::Location(const Vector& vLocation)
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
        sLocation = pLocation->m_sLocation;
    }
  }
  
  return sLocation;
}

void AgLocationCache::Load()
{
  for (AgLocationList::iterator itrLocations = m_lstLocations.begin() ;itrLocations != m_lstLocations.end(); ++itrLocations)
    delete *itrLocations;
  m_lstLocations.clear();
  
  char	szData[8196];

  char	szFile[MAX_PATH];
  sprintf(szFile,"%s/locs/%s.loc",AgGetDirectory(),STRING(gpGlobals->mapname));
  FILE* pFile = fopen(szFile,"r");
  if (!pFile)
  {
    // file error
    char szMsg[128];
    sprintf(szMsg,"Couldn't open location file %s.",szFile);
    AgConsole(szMsg);
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
}

#endif

//-- Martin Webrant
