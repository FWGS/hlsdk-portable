//++ muphicks  
// AGDomination mode
// Based on the AG CTF code by BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "aggamerules.h"
#include "agglobal.h"
#include "agdom.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
// Use the existing CTF message for now since I don't really want
// to have to make client changes at this time.. Once I get the new
// flag model from aljosa@lovercic@siol.net then I will probably
// make a new message if its required and add some new sounds.
extern int gmsgCTFSound;
enum DOMSound 
{
	YouHaveFlag = 0,
	TeamHaveFlag,
	EnemyHaveFlag,
	BlueFlagReturned,
	RedFlagReturned,
	BlueScores,
	RedScores,
  BlueFlagStolen,
  RedFlagStolen,
  //not used...
  BlueLeads,
	RedLeads,
	TeamsTied,
	SuddenDeath,
  Stolen,
  Capture,
};

extern int gmsgTeamScore;

FILE_GLOBAL int s_iTeam1Score;
FILE_GLOBAL int s_iTeam2Score;

AgDOM::AgDOM()
{
  m_iTeam1Score = 0;
  m_iTeam2Score = 0;
  s_iTeam1Score = 0;
  s_iTeam2Score = 0;
}

AgDOM::~AgDOM()
{
}

typedef list<int> boo;

void AgDOM::PlayerInitHud(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pPlayer->pev );
    WRITE_STRING( DOM_TEAM1_NAME);
    WRITE_SHORT( m_iTeam1Score );
    WRITE_SHORT( 0 );
  MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pPlayer->pev );
    WRITE_STRING( DOM_TEAM2_NAME);
    WRITE_SHORT( m_iTeam2Score );
    WRITE_SHORT( 0 );
  MESSAGE_END();
}


void AgDOM::SendControlScores(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pPlayer->pev );
    WRITE_STRING( DOM_TEAM1_NAME);
    WRITE_SHORT( s_iTeam1Score );
    WRITE_SHORT( 0 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pPlayer->pev );
    WRITE_STRING( DOM_TEAM2_NAME);
    WRITE_SHORT( s_iTeam2Score );
    WRITE_SHORT( 0 );
  MESSAGE_END();

}

bool AgDOM::ScoreLimit(void)
{
  if (ag_dom_scorelimit.value > 1
    && (  s_iTeam1Score >= ag_dom_scorelimit.value
        ||s_iTeam2Score >= ag_dom_scorelimit.value))
    return true;

  return false;
}

void AgDOM::ResetControlPoints(void)
{
  // Looping through entity finds !
  edict_t *pFind;
  
  // Grab a list of control points
  pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "item_dom_controlpoint" );

  while ( !FNullEnt( pFind ) )
  {
    // reset each one back to the NEUTRAL team ie uncaptured state
    CBaseEntity *pEnt = CBaseEntity::Instance( pFind );
    AgDOMControlPoint *pControlPoint = (AgDOMControlPoint *)pEnt;
    pControlPoint->Reset();
    pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "object_world" );
  }

  // reset scores here??
  m_iTeam1Score = 0;
  m_iTeam2Score = 0;
  s_iTeam1Score = 0;
  s_iTeam2Score = 0;
}

void AgDOM::Think()
{
  if (!g_pGameRules)
    return;

  // Has there been a new capture? Yes then we'd best see if one
  // team controls all the capture points or not.

  // Play BLUE/RED TEAM DOMINATE 

  // send update of scores
  if (m_iTeam1Score != s_iTeam1Score
    ||m_iTeam2Score != s_iTeam2Score)
  {
    m_iTeam1Score = s_iTeam1Score;

    //Send new team score to all clients.
    MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
      WRITE_STRING( DOM_TEAM1_NAME);
      WRITE_SHORT( s_iTeam1Score );
      WRITE_SHORT( 0 );
    MESSAGE_END();

    m_iTeam2Score = s_iTeam2Score;

    //Send new team score to all clients.
    MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
      WRITE_STRING( CTF_TEAM2_NAME);
      WRITE_SHORT( s_iTeam2Score );
      WRITE_SHORT( 0 );
    MESSAGE_END();

    // Could check whether a team has all the control points and if so play DOMINATION sound?
  }

  
  m_FileItemCache.Init();
}

void AgDOM::ClientConnected(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
}


void AgDOM::ClientDisconnected(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  // remove player from any active scoring controls
  edict_t *pFind;
  
  // Grab a list of control points
  pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "item_dom_controlpoint" );

  while ( !FNullEnt( pFind ) )
  {
    // reset each one back to the NEUTRAL team ie uncaptured state
    CBaseEntity *pEnt = CBaseEntity::Instance( pFind );
    AgDOMControlPoint *pControlPoint = (AgDOMControlPoint *)pEnt;
    pControlPoint->ClientDisconnected( pPlayer );
    pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "item_dom_controlpoint" );
  }
}


///////////////////////////////////////////////////////////////////////////////////
//
// AG DOM Control Point 
//
///////////////////////////////////////////////////////////////////////////////////

// most of these are not used, but may as well have them for later :)
enum Flag_Animations 
{ 
  ON_GROUND = 0, 
  NOT_CARRIED,
  CARRIED,
  WAVE_IDLE,
  FLAG_POSITION
};

void AgDOMControlPoint::Capture(CBasePlayer *pPlayer, const char *szTeamName)
{
  // if players team already controls this area don't recapture
  if (FStrEq(m_szTeamName, szTeamName))
     return;

  ChangeControllingTeam( szTeamName );

  m_fCaptureTime = gpGlobals->time + ag_dom_mincontroltime.value;
  m_iConsecutiveScores = 0;  // reset score count
  pCapturingPlayer = pPlayer;

  if ( 0 == strcmp(DOM_TEAM1_NAME,szTeamName))
  {
    MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
      WRITE_BYTE( RedFlagStolen );
    MESSAGE_END();
  }
  else if ( 0 == strcmp(DOM_TEAM2_NAME,szTeamName))
  {
    MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
      WRITE_BYTE( BlueFlagStolen );
    MESSAGE_END();
  }

  // Inform HLTV of this aweinspiring event :P Since there will be many more captures
  // happening in DOM than in CTF don't bother with slow mo for now.
  UTIL_SendDirectorMessage( pPlayer->edict(), this->edict(), 10 | DRC_FLAG_DRAMATIC );

  // Inform all players that zone has been taken control of by playername	
  // really need identifiers for each control point! 
  char szText[300];
  sprintf(szText, "%s captures CP at %s!", STRING(pPlayer->pev->netname), m_szLocation);
  AgConsole(szText);
	UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );
}

void AgDOMControlPoint::ChangeControllingTeam( const char *szTeamName )
{
 
	if (FStrEq(DOM_TEAM1_NAME, szTeamName))
		pev->skin = 1;
	else if (FStrEq(DOM_TEAM2_NAME, szTeamName))
		pev->skin = 2;
  else 
    pev->skin = 3; 


	if (FStrEq(DOM_TEAM1_NAME, szTeamName))
	{
		pev->rendercolor.x = 0;
		pev->rendercolor.y = 0;
		pev->rendercolor.z = 128;			
    pev->renderamt = 50;
	}
	else if (FStrEq(DOM_TEAM2_NAME, szTeamName))
	{
		pev->rendercolor.x = 128;
		pev->rendercolor.y = 0;
		pev->rendercolor.z = 0;
    pev->renderamt = 50;
	}
  else if (FStrEq(DOM_NEUTRAL_NAME, szTeamName))
	{
		pev->rendercolor.x = 0;  // R
		pev->rendercolor.y = 128; // G
		pev->rendercolor.z = 0;  // B
    pev->renderamt = 100;
	}

  // Change the owner of the control point
  strncpy( m_szTeamName, szTeamName, sizeof(m_szTeamName) );
}

void AgDOMControlPoint::Spawn ( void )
{
  m_fNextTouch = 0;

	Precache( );
	SET_MODEL(ENT(pev), "models/flag.mdl");
		
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));

	SetThink( &AgDOMControlPoint::Think );
	SetTouch( &AgDOMControlPoint::Touch );

	pev->nextthink = gpGlobals->time + 0.1;
	
  // spawn by default under no teams control
  ChangeControllingTeam( DOM_NEUTRAL_NAME );

  pev->sequence = NOT_CARRIED;
	pev->framerate = 1;

	pev->renderamt = 50;
	pev->renderfx = kRenderFxGlowShell;
}

void AgDOMControlPoint::Reset( void )
{
  ChangeControllingTeam( DOM_NEUTRAL_NAME ); 
  m_fCaptureTime = -1;
  m_iConsecutiveScores = 0;  // reset score count
  pCapturingPlayer = NULL;

  // Inform HLTV of this event, although its more exciting than an empty room its not that exciting :(
  UTIL_SendDirectorMessage( this->edict(), NULL, 1 );
}

void AgDOMControlPoint::Precache( void )
{
	PRECACHE_MODEL ("models/flag.mdl");
}

void AgDOMControlPoint::Think( void )
{
  // Has the flag been under control long enough to score a team point?
  if( !FStrEq( m_szTeamName, DOM_NEUTRAL_NAME ))
    if( m_fCaptureTime <= gpGlobals->time )
    {
      // Team scores
      if (FStrEq( m_szTeamName, DOM_TEAM1_NAME ))
        s_iTeam1Score += ag_dom_controlpoints.value;
      else if (FStrEq( m_szTeamName, DOM_TEAM2_NAME ))
        s_iTeam2Score += ag_dom_controlpoints.value;

      //Give the player the points
      if (pCapturingPlayer && FStrEq(pCapturingPlayer->m_szTeamName,m_szTeamName) )
        pCapturingPlayer->AddPoints(ag_dom_controlpoints.value, TRUE);
	      //pCapturingPlayer->AddPointsToTeam(ag_dom_controlpoints.value, TRUE);  // is this required?
      else if (pCapturingPlayer)
        pCapturingPlayer = NULL;

      // Increase score count
      m_iConsecutiveScores++;

      // reset score timer
      m_fCaptureTime = gpGlobals->time + ag_dom_mincontroltime.value;
    }

  // Have we pased the max capture score limit, if so return control of this flag to 
  // a neutral state. 
  if ( m_iConsecutiveScores >= ag_dom_resetscorelimit.value ){
    char szText[201];
    sprintf(szText, "Neutral CP available at %s", m_szLocation);
    AgConsole(szText);
    UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );
    Reset();
  }

  // animate the control point
	pev->frame += pev->framerate;
	if (pev->frame < 0.0 || pev->frame >= 256.0) 
	{
		pev->frame -= (int)(pev->frame / 256.0) * 256.0;
	}
	pev->nextthink = gpGlobals->time + 0.1;

}

void AgDOMControlPoint::Touch( CBaseEntity *pOther )
{
  // prevent CP changing owner too quickly
  if (m_fNextTouch > gpGlobals->time)
    return;
  else m_fNextTouch = gpGlobals->time + 0.5;

	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
		return;

  if ( !pOther->IsAlive() )
    return;


	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
  
  if (FStrEq(pPlayer->m_szTeamName, DOM_TEAM1_NAME))
    Capture( pPlayer, DOM_TEAM1_NAME ); 
  else if (FStrEq(pPlayer->m_szTeamName, DOM_TEAM2_NAME))
    Capture( pPlayer, DOM_TEAM2_NAME ); 	
}

void AgDOMControlPoint::ClientDisconnected(CBasePlayer* pPlayer)
{
  // Is there a better way to handle this? eg before adding on player score
  // checking if player still exists?
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
 
  if (pPlayer == pCapturingPlayer)
    pCapturingPlayer = NULL;
}


#ifndef AG_NO_CLIENT_DLL
LINK_ENTITY_TO_CLASS( item_dom_controlpoint, AgDOMControlPoint );
#endif


///////////////////////////////////////////////////////////////////////////////////
//
// AG Item Cache - Allows maps that are none DOM specific to be used as DOM maps
//
///////////////////////////////////////////////////////////////////////////////////


#include "vector.h"

class AgDOMFileItemCache;


AgDOMFileItem::AgDOMFileItem()
{
  m_vOrigin = Vector(0,0,0);
  m_vAngles = Vector(0,0,0);
  m_szName[0] = '\0';
  m_szData1[0] = '\0';
}

AgDOMFileItem::~AgDOMFileItem()
{

} 

void AgDOMFileItem::Show()
{
 	CLaserSpot* pSpot = CLaserSpot::CreateSpot();
	UTIL_SetOrigin( pSpot->pev, m_vOrigin );
	pSpot->LiveForTime(5.0);
}



AgDOMFileItemCache::AgDOMFileItemCache()
{
  m_bInitDone = false;
  Load();
}

AgDOMFileItemCache::~AgDOMFileItemCache()
{
  //Delete all.
  for (AgDOMFileItemList::iterator itrFileItems = m_lstFileItems.begin() ;itrFileItems != m_lstFileItems.end(); ++itrFileItems)
    delete *itrFileItems;
  m_lstFileItems.clear();
}

void AgDOMFileItemCache::Add(const AgString& sFileItem,CBasePlayer* pPlayer)
{ 
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
  
  if (0 == sFileItem.size())
    return;
  
  AgDOMFileItem* pFileItem = new AgDOMFileItem;
  strcpy(pFileItem->m_szName,sFileItem.c_str());
  pFileItem->m_vOrigin = pPlayer->pev->origin;
  pFileItem->m_vAngles = pPlayer->pev->angles;
  
  m_lstFileItems.push_back(pFileItem);
  pFileItem->Show();
  
  Save(pPlayer);
  
  AgConsole(UTIL_VarArgs("Added item %s.",(const char*)sFileItem.c_str()),pPlayer);
}

void AgDOMFileItemCache::Del(CBasePlayer* pPlayer)
{ 
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
  
  if (0 == m_lstFileItems.size())
    return;
  
  AgDOMFileItem* pFileItem = m_lstFileItems.back(); 
  AgConsole(UTIL_VarArgs("Deleted last item - %s.",pFileItem->m_szName,pPlayer));
  m_lstFileItems.pop_back();
  Save(pPlayer);
}

void AgDOMFileItemCache::List(CBasePlayer* pPlayer)
{ 
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
  
  for (AgDOMFileItemList::iterator itrFileItems = m_lstFileItems.begin() ;itrFileItems != m_lstFileItems.end(); ++itrFileItems)
  {
    AgConsole(UTIL_VarArgs("%s",(const char*)(*itrFileItems)->m_szName),pPlayer);
    (*itrFileItems)->Show();
  }
}

void AgDOMFileItemCache::Load(CBasePlayer* pPlayer)
{
  for (AgDOMFileItemList::iterator itrFileItems = m_lstFileItems.begin() ;itrFileItems != m_lstFileItems.end(); ++itrFileItems)
    delete *itrFileItems;
  m_lstFileItems.clear();
  

  char	szFile[MAX_PATH];
  char	szData[20000];
  sprintf(szFile, "%s/dom/%s.dom", AgGetDirectory(),STRING(gpGlobals->mapname));
	FILE* pFile = fopen(szFile,"r");
	if (!pFile)
  {
    // file error
    return;
  }

  int iRead = fread(szData,sizeof(char),sizeof(szData)-2,pFile);
  fclose(pFile);
  if (0 >= iRead)
    return;
  szData[iRead] = '\0';

  char* pszCTFString = strtok( szData, "\n");
  while (pszCTFString != NULL)
  {
    AgDOMFileItem* pFileItem = new AgDOMFileItem;
    sscanf(pszCTFString,"%s %f %f %f %f %f %f %s\n",pFileItem->m_szName,&pFileItem->m_vOrigin.x,&pFileItem->m_vOrigin.y,&pFileItem->m_vOrigin.z,
                                                    &pFileItem->m_vAngles.x,&pFileItem->m_vAngles.y,&pFileItem->m_vAngles.z,
                                                    pFileItem->m_szData1 );
    m_lstFileItems.push_back(pFileItem);
    pszCTFString = strtok( NULL, "\n");
  }
}

void AgDOMFileItemCache::Save(CBasePlayer* pPlayer)
{
  if (0 == m_lstFileItems.size())
    return;
  
  char	szFile[MAX_PATH];
  sprintf(szFile, "%s/dom/%s.dom", AgGetDirectory(),STRING(gpGlobals->mapname));
  FILE* pFile = fopen(szFile,"wb");
  if (!pFile)
  {
    // file error
    AgConsole(UTIL_VarArgs("Couldn't create/save FileItem file %s.",szFile),pPlayer);
    return;
  }
  
  //Loop and write the file.
  for (AgDOMFileItemList::iterator itrFileItems = m_lstFileItems.begin() ;itrFileItems != m_lstFileItems.end(); ++itrFileItems)
  {
    //Append.
    AgDOMFileItem* pFileItem = *itrFileItems;
    fprintf(pFile,"%s %f %f %f %f %f %f %s\n",pFileItem->m_szName,pFileItem->m_vOrigin.x,pFileItem->m_vOrigin.y,pFileItem->m_vOrigin.z,
                                              pFileItem->m_vAngles.x,pFileItem->m_vAngles.y,pFileItem->m_vAngles.z,
                                              pFileItem->m_szData1);
  }
  
  fflush(pFile);
  fclose(pFile);
}


void AgDOMFileItemCache::Init()
{
  if (m_bInitDone)
    return;
  m_bInitDone = true;
  CBaseEntity *pEnt = NULL;

  for (AgDOMFileItemList::iterator itrFileItems = m_lstFileItems.begin() ;itrFileItems != m_lstFileItems.end(); ++itrFileItems)
  {
    AgDOMFileItem* pFileItem = *itrFileItems;

    if (g_pGameRules->IsAllowedToSpawn(pFileItem->m_szName))
      pEnt = CBaseEntity::Create(pFileItem->m_szName, pFileItem->m_vOrigin, pFileItem->m_vAngles, INDEXENT(0));

    // In addition to the generic entity creation params we wish to load a location param for ControlPoints,
    // we parse this here. Unless you can tell me a better place or more generic method in which case I'll use it :)
    // An alternative is to add info_dom_location items with a string name for the location however this would
    // still involve parsing the datafile for strings so unless we change the save/load routine there is no point
    // EG parse item name then deal with special cases or generic case - May change to this later :P
    if (FStrEq( "item_dom_controlpoint", pFileItem->m_szName) && pEnt)
	{
      AgDOMControlPoint *pCP = (AgDOMControlPoint*)pEnt;
      strncpy( pCP->m_szLocation, pFileItem->m_szData1, sizeof(pCP->m_szLocation) );
    }

  }
}