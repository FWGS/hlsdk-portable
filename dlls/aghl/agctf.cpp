//++ BulliT - with ideas from http://www.planethalflife.com/whenitsdone/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "aggamerules.h"
#include "agglobal.h"
#include "agctf.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern int gmsgCTFSound;
extern int gmsgCTFFlag;
extern int gmsgCountdown;

enum CTFSound 
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

FILE_GLOBAL int s_iTeam1Captures;
FILE_GLOBAL int s_iTeam2Captures;

DLL_GLOBAL bool g_bTeam1FlagStolen;
DLL_GLOBAL bool g_bTeam2FlagStolen;

DLL_GLOBAL bool g_bTeam1FlagLost; 
DLL_GLOBAL bool g_bTeam2FlagLost; 
FILE_GLOBAL int	s_iPlayerFlag1;
FILE_GLOBAL int	s_iPlayerFlag2;

extern int gmsgTeamScore;

AgCTF::AgCTF()
{
  m_iTeam1Captures = 0;
  m_iTeam2Captures = 0;
	s_iTeam1Captures = s_iTeam2Captures = 0;
	g_bTeam1FlagStolen = g_bTeam2FlagStolen = false;
  g_bTeam1FlagLost = g_bTeam2FlagLost = false;
  s_iPlayerFlag1 = 0;
  s_iPlayerFlag2 = 0;
  m_iPlayerFlag1 = 0;
  m_iPlayerFlag2= 0;
	m_fNextCountdown = 0.0; 
	m_fMatchStart = 0.0;

  if (ag_ctf_roundbased.value)
    m_Status = Waiting;
  else
    m_Status = Playing;
}

AgCTF::~AgCTF()
{

}

void AgCTF::PlayerInitHud(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pPlayer->pev );
    WRITE_STRING( CTF_TEAM1_NAME);
    WRITE_SHORT( m_iTeam1Captures );
    WRITE_SHORT( 0 );
  MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pPlayer->pev );
    WRITE_STRING( CTF_TEAM2_NAME);
    WRITE_SHORT( m_iTeam2Captures );
    WRITE_SHORT( 0 );
  MESSAGE_END();
}

bool AgCTF::CaptureLimit()
{
  if (ag_ctf_capture_limit.value > 1
    && (  s_iTeam1Captures == ag_ctf_capture_limit.value
        ||s_iTeam2Captures == ag_ctf_capture_limit.value))
    return true;

  return false;
}

void AgCTF::SendCaptures(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pPlayer->pev );
    WRITE_STRING( CTF_TEAM1_NAME);
    WRITE_SHORT( s_iTeam1Captures );
    WRITE_SHORT( 0 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pPlayer->pev );
    WRITE_STRING( CTF_TEAM2_NAME);
    WRITE_SHORT( s_iTeam2Captures );
    WRITE_SHORT( 0 );
  MESSAGE_END();

}

void AgCTF::ResetScore(bool bResetCaptures)
{
    m_iTeam1Captures = -1;
    m_iTeam2Captures = -1;
	  s_iTeam1Captures = s_iTeam2Captures = 0;
    m_Status = Playing;

    if (bResetCaptures)
      ResetCaptures();
}

void AgCTF::ResetCaptures()
{
	g_bTeam1FlagStolen = g_bTeam2FlagStolen = false;

  //Reset flag carrier.
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop)
    {
      pPlayerLoop->m_bFlagTeam1 = false;
      pPlayerLoop->m_bFlagTeam2 = false;
    }
  }

  //Remove carried flag
  CBaseEntity* pEntity = NULL;	
  pEntity = NULL;
  while ((pEntity = UTIL_FindEntityByClassname( pEntity, "carried_flag_team1" )) != NULL)
    UTIL_Remove(pEntity);

  pEntity = NULL;
  while ((pEntity = UTIL_FindEntityByClassname( pEntity, "carried_flag_team2" )) != NULL)
    UTIL_Remove(pEntity);

  //Remove dropped flag
  pEntity = NULL;
  while ((pEntity = UTIL_FindEntityByClassname( pEntity, "item_flag_team1" )) != NULL)
  {
    if (((AgCTFFlag*)pEntity)->m_bDropped)
      UTIL_Remove(pEntity);
  }

  pEntity = NULL;
  while ((pEntity = UTIL_FindEntityByClassname( pEntity, "item_flag_team2" )) != NULL)
  {
    if (((AgCTFFlag*)pEntity)->m_bDropped)
      UTIL_Remove(pEntity);
  }

  //Reset base flag
  AgCTFFlag::ResetFlag(CTF_TEAM1_NAME);
  AgCTFFlag::ResetFlag(CTF_TEAM2_NAME);
}

void AgCTF::Think()
{
  if (!g_pGameRules)
    return;

  if (m_iTeam1Captures != s_iTeam1Captures
    ||m_iTeam2Captures != s_iTeam2Captures)
  {
    m_iTeam1Captures = s_iTeam1Captures;

    //Send new team score to all clients.
    MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
      WRITE_STRING( CTF_TEAM1_NAME);
      WRITE_SHORT( s_iTeam1Captures );
      WRITE_SHORT( 0 );
    MESSAGE_END();

    m_iTeam2Captures = s_iTeam2Captures;

    //Send new team score to all clients.
    MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
      WRITE_STRING( CTF_TEAM2_NAME);
      WRITE_SHORT( s_iTeam2Captures );
      WRITE_SHORT( 0 );
    MESSAGE_END();
  }

  if (m_iPlayerFlag1 != s_iPlayerFlag1
    ||m_iPlayerFlag2 != s_iPlayerFlag2)
  {
    m_iPlayerFlag1 = s_iPlayerFlag1;
    m_iPlayerFlag2 = s_iPlayerFlag2;

    //Send new flag status to all clients.
    MESSAGE_BEGIN( MSG_ALL, gmsgCTFFlag );
      WRITE_BYTE( m_iPlayerFlag1 );
      WRITE_BYTE( m_iPlayerFlag2 );
    MESSAGE_END();
  }

  m_FileItemCache.Init();

  RoundBasedThink();
}

void AgCTF::RoundBasedThink()
{
  //We only update status once every second.
  if (m_fNextCountdown > gpGlobals->time)
    return;
  m_fNextCountdown = gpGlobals->time + 1.0; 

  //Handle the status
  if (Waiting == m_Status)
  {
    g_bPaused = true;
    m_Status = Countdown;
    m_fMatchStart = gpGlobals->time + 8.0;
    m_fNextCountdown = gpGlobals->time + 3.0; 

    //Write waiting message
    MESSAGE_BEGIN( MSG_ALL, gmsgCountdown);
      WRITE_BYTE( 50 );
      WRITE_BYTE( 1 );
      WRITE_STRING( m_sWinner.c_str() );
      WRITE_STRING( "" );
    MESSAGE_END();
  }
  else if (Countdown == m_Status)
  {
    if (m_fMatchStart < gpGlobals->time)
    {
      //Clear out the map
      AgResetMap();

      //Reset CTF items
      ResetCaptures();

      m_Status = Spawning;
      m_sWinner = "";

      //Time to start playing.
      for ( int i = 1; i <= gpGlobals->maxClients; i++ )
      {
        CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
        if (pPlayerLoop && pPlayerLoop->m_bReady)
        {
          if (!pPlayerLoop->IsSpectator())
            pPlayerLoop->RespawnMatch();
        }
      }
   
      m_Status = Playing;

      //Stop countdown
      MESSAGE_BEGIN( MSG_ALL, gmsgCountdown);
        WRITE_BYTE( -1 );
        WRITE_BYTE( 0 );
        WRITE_STRING( "" );
        WRITE_STRING( "" );
      MESSAGE_END();

      g_bPaused = false;
    }
    else
    {
      //Write countdown message.
      MESSAGE_BEGIN( MSG_ALL, gmsgCountdown);
        WRITE_BYTE( (int)(m_fMatchStart - gpGlobals->time) );
        WRITE_BYTE( 1 );
        WRITE_STRING( "" );
        WRITE_STRING( "" );
      MESSAGE_END();
    }
  }
}

void AgCTF::RoundOver(const char* pszWinner)
{
  m_sWinner = pszWinner;
  m_Status = Waiting;
}


void AgCTF::ClientConnected(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
}


void AgCTF::ClientDisconnected(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  PlayerDropFlag(pPlayer);
}

void AgCTF::PlayerKilled(CBasePlayer* pPlayer,entvars_t *pKiller)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  CBaseEntity* pKillerBE = CBaseEntity::Instance(pKiller);
  if (pKillerBE && CLASS_PLAYER == pKillerBE->Classify())
  {
    CBasePlayer* pKillerPlayer = ((CBasePlayer*)pKillerBE);
    AddPointsForKill(pKillerPlayer,pPlayer);
  }

  bool bReturnDirectly = (0 == strcmp(STRING(pKiller->classname),"trigger_hurt"));

  if (bReturnDirectly)
  {
    if (pPlayer->m_bFlagTeam1)
    {
      CBaseEntity* pEntity = NULL;	
      pEntity = NULL;
      while ((pEntity = UTIL_FindEntityByClassname( pEntity, "carried_flag_team1" )) != NULL)
        UTIL_Remove(pEntity);
      pPlayer->m_bFlagTeam1 = false;
      AgCTFFlag::ResetFlag(CTF_TEAM1_NAME);

		  char szText[201];
			sprintf(szText, "%s flag returned!", CTF_TEAM1_NAME);
      AgConsole(szText);
			UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );

      MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
        WRITE_BYTE( BlueFlagReturned );
      MESSAGE_END();
    }
    else if (pPlayer->m_bFlagTeam2)
    {
      CBaseEntity* pEntity = NULL;	
      pEntity = NULL;
      while ((pEntity = UTIL_FindEntityByClassname( pEntity, "carried_flag_team2" )) != NULL)
        UTIL_Remove(pEntity);
      pPlayer->m_bFlagTeam2 = false;
      AgCTFFlag::ResetFlag(CTF_TEAM2_NAME);

      char szText[201];
			sprintf(szText, "%s flag returned!", CTF_TEAM2_NAME);
      AgConsole(szText);
			UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );

      MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
        WRITE_BYTE( RedFlagReturned );
      MESSAGE_END();
    }
  }
  else
    PlayerDropFlag(pPlayer);
}

void AgCTF::AddPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled)
{
	if ( pAttacker != pKilled && g_pGameRules->PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return; //Killed his team m8.

	if (FStrEq(CTF_TEAM1_NAME, pKilled->m_szTeamName) && pKilled->m_bFlagTeam2
    ||FStrEq(CTF_TEAM2_NAME, pKilled->m_szTeamName) && pKilled->m_bFlagTeam1)
  {
    //He killed the flag carrier.
	  pAttacker->AddPoints(ag_ctf_carrierkillpoints.value, TRUE);
  }

  //Check if he is 192 units within his own flag (defending)
	edict_t* pFind = NULL;
	if (FStrEq(CTF_TEAM1_NAME, pAttacker->m_szTeamName))
		pFind = FIND_ENTITY_BY_CLASSNAME(NULL,"item_flag_team1");
	else if (FStrEq(CTF_TEAM2_NAME, pAttacker->m_szTeamName))
		pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "item_flag_team2" );
  if (pFind)
  {
    Vector vtFlag = pFind->v.origin;
    Vector vtPlayer = pKilled->pev->origin;
    float fDistance = (vtFlag - vtPlayer).Length();
    if (fDistance < 192)
      //Add points for defending.
      pAttacker->AddPoints(ag_ctf_defendpoints.value, TRUE);
  }
}


void AgCTF::PlayerDropFlag(CBasePlayer* pPlayer, bool bPlayerDrop)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

	//When carrying a flag, drop it!
	if (pPlayer->m_bFlagTeam1 || pPlayer->m_bFlagTeam2)
	{
		char szText [201];

		CBaseEntity *pEnt = NULL;

    if (bPlayerDrop)
		  UTIL_MakeVectors ( pPlayer->pev->angles ); 

		if (pPlayer->m_bFlagTeam1)
		{
      pEnt = CBaseEntity::Create( "item_flag_team1", bPlayerDrop? (pPlayer->pev->origin + gpGlobals->v_forward * 10) : pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict() );
			sprintf(szText, "%s lost the %s flag!", STRING(pPlayer->pev->netname), CTF_TEAM1_NAME);
      AgConsole(szText);
			UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );
      g_bTeam1FlagLost = true;
		}
		else if (pPlayer->m_bFlagTeam2)
		{
			pEnt = CBaseEntity::Create( "item_flag_team2", bPlayerDrop? (pPlayer->pev->origin + gpGlobals->v_forward * 10) : pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict() );
			sprintf(szText, "%s lost the %s flag!", STRING(pPlayer->pev->netname), CTF_TEAM2_NAME);
      AgConsole(szText);
			UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );
      g_bTeam2FlagLost = true;
		}

    if (bPlayerDrop)
    {
      pEnt->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;
 			pEnt->pev->angles.z = 0;
    }
    else
		  pEnt->pev->velocity = pPlayer->pev->velocity * 1.2;
		pEnt->pev->angles.x = 0;

		AgCTFFlag *pFlag = (AgCTFFlag *)pEnt;

    if (bPlayerDrop)
      pFlag->m_fNextTouch = gpGlobals->time + 0.5; //Gotta give the flag a bit of time to fly away from player before it can be picked up again.
		pFlag->m_bDropped = true;

	  pFlag->m_fNextReset = gpGlobals->time + ag_ctf_flag_resettime.value;

		pPlayer->m_bFlagTeam1 = false;
		pPlayer->m_bFlagTeam2 = false;
  }
}


extern int gmsgItemPickup;

enum Flag_Animations 
{ 
  ON_GROUND = 0, 
  NOT_CARRIED,
  CARRIED,
  WAVE_IDLE,
  FLAG_POSITION
};

void AgCTFFlag::Spawn ( void )
{
  m_fNextTouch = 0;

	Precache( );
	SET_MODEL(ENT(pev), "models/flag.mdl");
		
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));

	SetThink( &AgCTFFlag::Think );
	SetTouch( &AgCTFFlag::FlagTouch );

	pev->nextthink = gpGlobals->time + 0.1;
		
	if (FStrEq(CTF_TEAM1_NAME, m_szTeamName))
		pev->skin = 1;
	else if (FStrEq(CTF_TEAM2_NAME, m_szTeamName))
		pev->skin = 2;

	m_bDropped = false;

	pev->sequence = NOT_CARRIED;
	pev->framerate = 1.0;

	if (FStrEq(CTF_TEAM1_NAME, m_szTeamName))
	{
		pev->rendercolor.x = 0;
		pev->rendercolor.y = 0;
		pev->rendercolor.z = 128;			
	}
	else if (FStrEq(CTF_TEAM2_NAME, m_szTeamName))
	{
		pev->rendercolor.x = 128;
		pev->rendercolor.y = 0;
		pev->rendercolor.z = 0;
	}
	pev->renderamt = 50;
	pev->renderfx = kRenderFxGlowShell;
}

void AgCTFFlag::Precache( void )
{
	PRECACHE_MODEL ("models/flag.mdl");
}

void AgCTFFlag::Capture(CBasePlayer *pPlayer, const char *m_szTeamName)
{
	char szText[201];

	sprintf(szText, "%s captured the %s flag!", STRING(pPlayer->pev->netname), m_szTeamName);
  AgConsole(szText);
	UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );

	//Give the player the points
	pPlayer->AddPoints(ag_ctf_capturepoints.value, TRUE);
	pPlayer->AddPointsToTeam(ag_ctf_teamcapturepoints.value, TRUE);

	//And give the team a capture
	if (FStrEq(CTF_TEAM1_NAME, m_szTeamName))
  {
    MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
      WRITE_BYTE( RedScores );
    MESSAGE_END();

		s_iTeam2Captures++;
    UTIL_LogPrintf("Team \"%s\" triggered \"Capture\" (%s \"%d\") (%s \"%d\")\n",CTF_TEAM2_NAME,CTF_TEAM1_NAME,s_iTeam1Captures,CTF_TEAM2_NAME,s_iTeam2Captures);

    if (ag_ctf_roundbased.value)
      g_pGameRules->m_CTF.RoundOver(CTF_TEAM2_NAME);
  }
	else if (FStrEq(CTF_TEAM2_NAME, m_szTeamName))
  {
    MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
      WRITE_BYTE( BlueScores );
    MESSAGE_END();

		s_iTeam1Captures++;
    UTIL_LogPrintf("Team \"%s\" triggered \"Capture\" (%s \"%d\") (%s \"%d\")\n",CTF_TEAM1_NAME,CTF_TEAM1_NAME,s_iTeam1Captures,CTF_TEAM2_NAME,s_iTeam2Captures);

    if (ag_ctf_roundbased.value)
      g_pGameRules->m_CTF.RoundOver(CTF_TEAM1_NAME);
  }

	ResetFlag( m_szTeamName );
}

void AgCTFFlag::ResetFlag()
{
  AgCTFFlag::ResetFlag(m_szTeamName);

	char szText[201];
	sprintf(szText, "%s flag returned!", m_szTeamName);
  AgConsole(szText);
	UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );

	if (FStrEq(CTF_TEAM1_NAME, m_szTeamName))
	{
    MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
      WRITE_BYTE( BlueFlagReturned );
    MESSAGE_END();
	}
	else if (FStrEq(CTF_TEAM2_NAME, m_szTeamName))
	{
    MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
      WRITE_BYTE( RedFlagReturned );
    MESSAGE_END();
	}

  if (m_bDropped)
	  UTIL_Remove( this );
}

void AgCTFFlag::ResetFlag(const char *szTeamName)
{
	edict_t *pFind;
	
	if (FStrEq(CTF_TEAM1_NAME, szTeamName))
	{
		pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "item_flag_team1" );
		g_bTeam1FlagStolen = false;
    g_bTeam1FlagLost = false;
	}
	else if (FStrEq(CTF_TEAM2_NAME, szTeamName))
	{
		pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "item_flag_team2" );
		g_bTeam2FlagStolen = false;
    g_bTeam2FlagLost = false;
	}
	else 
    return;

	while ( !FNullEnt( pFind ) )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( pFind );
		AgCTFFlag *pFlag = (AgCTFFlag *)pEnt;

		pFlag->Materialize( );

		if (FStrEq(CTF_TEAM1_NAME, szTeamName))
			pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "item_flag_team1" );
		else if (FStrEq(CTF_TEAM2_NAME, szTeamName))
			pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "item_flag_team2" );
	}
}

void AgCTFFlag::FlagTouch( CBaseEntity *pOther )
{
  if (m_fNextTouch > gpGlobals->time)
    return;

	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
		return;

  if ( !pOther->IsAlive() )
    return;


	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (MyTouch( pPlayer ))
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );
		SetThink( NULL );
		
		//if it's a dropped flag, remove. Else make it invisible
		if (m_bDropped)
			UTIL_Remove( this );
		else
			pev->effects |= EF_NODRAW;
	}
}

void AgCTFFlag::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( &AgCTFFlag::FlagTouch );
	SetThink( &AgCTFFlag::Think );
}

BOOL AgCTFFlag::MyTouch( CBasePlayer *pPlayer )
{
	char szText[201];

	// Can only carry one flag and can not pickup own flag
	if (FStrEq(CTF_TEAM1_NAME, m_szTeamName))
	{
		//if client has other teams flag and it isn't a dropped flag, then there is a capture!
		if ( pPlayer->m_bFlagTeam2 && !m_bDropped)
		{
			pPlayer->m_bFlagTeam2 = false;
			
      UTIL_SendDirectorMessage( pPlayer->edict(), this->edict(), 10 | DRC_FLAG_DRAMATIC);
			Capture(pPlayer, CTF_TEAM2_NAME);
			return FALSE;
		}
		else if ( pPlayer->m_bFlagTeam1 )
		{
			return FALSE;
		}
		else if (FStrEq(pPlayer->m_szTeamName, CTF_TEAM1_NAME))
		{
			//if dropped, return flag
			if (m_bDropped)
			{
				ResetFlag(CTF_TEAM1_NAME);
				sprintf(szText, "%s returned the %s flag!", STRING(pPlayer->pev->netname), m_szTeamName);
        AgConsole(szText);
				UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );
				UTIL_Remove( this );

        pPlayer->AddPoints(ag_ctf_returnpoints.value, TRUE);

        MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
          WRITE_BYTE( BlueFlagReturned );
        MESSAGE_END();
			}
			//but don't pick it up!
			return FALSE;
		}
	}
	else if (FStrEq(CTF_TEAM2_NAME, m_szTeamName))
	{
		//if client has other teams flag and it isn't a dropped flag, then there is a capture!
		if ( pPlayer->m_bFlagTeam1 && !m_bDropped)
		{
			pPlayer->m_bFlagTeam1 = false;

      UTIL_SendDirectorMessage( pPlayer->edict(), this->edict(), 10 | DRC_FLAG_DRAMATIC);
			Capture(pPlayer, CTF_TEAM1_NAME);

			return FALSE;
		}
		else if ( pPlayer->m_bFlagTeam2 )
		{
			return FALSE;
		}
		else if (FStrEq(pPlayer->m_szTeamName, CTF_TEAM2_NAME))
		{
			//if dropped, return flag
			if (m_bDropped)
			{
				ResetFlag(CTF_TEAM2_NAME);
				sprintf(szText, "%s returned the %s flag!", STRING(pPlayer->pev->netname), m_szTeamName);
        AgConsole(szText);
				UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );
				UTIL_Remove( this );

        MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
          WRITE_BYTE( RedFlagReturned );
        MESSAGE_END();

        pPlayer->AddPoints(ag_ctf_returnpoints.value, TRUE);
			}
			//but don't pick it up!
			return FALSE;
		}
	}

	if ( ( pPlayer->pev->weapons & (1<<WEAPON_SUIT) ) )
	{
		if (FStrEq(CTF_TEAM1_NAME, m_szTeamName))
		{
			pPlayer->m_bFlagTeam1 = true;
			// player is now carrying the flag of team1, so give him the flag

			CBaseEntity *pEnt = CBaseEntity::Create( "carried_flag_team1", pev->origin, pev->angles, pPlayer->edict() );
			AgCTFPlayerFlag *pCarriedFlag = (AgCTFPlayerFlag *)pEnt;
			pCarriedFlag->m_pOwner = pPlayer;
			s_iPlayerFlag1 = pPlayer->entindex();

      /*
      //Glow blue
			pCarriedFlag->pev->renderfx = kRenderFxGlowShell;
			pCarriedFlag->pev->rendercolor = Vector( 0, 0, 255 );	// RGB
			pCarriedFlag->pev->renderamt = 100;	// Shell size
      */
		}
		else if (FStrEq(CTF_TEAM2_NAME, m_szTeamName))
		{
			pPlayer->m_bFlagTeam2 = true;
			// player is now carrying the flag of team2, so give him the flag
			CBaseEntity *pEnt = CBaseEntity::Create( "carried_flag_team2", pev->origin, pev->angles, pPlayer->edict() );
			AgCTFPlayerFlag *pCarriedFlag = (AgCTFPlayerFlag *)pEnt;
			pCarriedFlag->m_pOwner = pPlayer;
			s_iPlayerFlag2 = pPlayer->entindex();
      /*
      //Glow red
			pCarriedFlag->pev->renderfx = kRenderFxGlowShell;
			pCarriedFlag->pev->rendercolor = Vector( 255, 0, 0 );	// RGB
			pCarriedFlag->pev->renderamt = 100;	// Shell size
      */
		}
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
		MESSAGE_END();

		//Let all players hear and read that the flag is gone
		char szText[201];

		if (FStrEq(CTF_TEAM1_NAME, m_szTeamName))
    {
      if (!g_bTeam1FlagLost)
        pPlayer->AddPoints(ag_ctf_stealpoints.value, TRUE);
			sprintf(szText, "%s got the %s flag!\n", STRING(pPlayer->pev->netname), CTF_TEAM1_NAME);
    }
		else if (FStrEq(CTF_TEAM2_NAME, m_szTeamName))
    {
      if (!g_bTeam2FlagLost)
        pPlayer->AddPoints(ag_ctf_stealpoints.value, TRUE);
			sprintf(szText, "%s got the %s flag!\n", STRING(pPlayer->pev->netname), CTF_TEAM2_NAME);
    }

    AgConsole(szText);
		UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );

    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
      CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
      if (pPlayerLoop)
      {
        if (pPlayer != pPlayerLoop)
        {
          if (pPlayerLoop->IsSpectator() || pPlayerLoop->IsProxy())
          {
            if (!m_bDropped)
            {
              if (FStrEq(m_szTeamName, CTF_TEAM1_NAME))
              {
	              MESSAGE_BEGIN( MSG_ONE, gmsgCTFSound, NULL, pPlayerLoop->pev );
                  WRITE_BYTE(BlueFlagStolen);
                MESSAGE_END();
              }
              else
              {
	              MESSAGE_BEGIN( MSG_ONE, gmsgCTFSound, NULL, pPlayerLoop->pev );
                  WRITE_BYTE(RedFlagStolen);
                MESSAGE_END();
              }
            }
          }
          else if (FStrEq(pPlayerLoop->m_szTeamName, m_szTeamName))
          {
	          MESSAGE_BEGIN( MSG_ONE, gmsgCTFSound, NULL, pPlayerLoop->pev );
              WRITE_BYTE(EnemyHaveFlag);
            MESSAGE_END();
          }
          else 
          {
	          MESSAGE_BEGIN( MSG_ONE, gmsgCTFSound, NULL, pPlayerLoop->pev );
              WRITE_BYTE(TeamHaveFlag);
            MESSAGE_END();
          }
        }
        else
        {
	        MESSAGE_BEGIN( MSG_ONE, gmsgCTFSound, NULL, pPlayerLoop->pev );
            WRITE_BYTE(YouHaveFlag);
          MESSAGE_END();
        }
      }
    }

    int iPowerUp = 0;

		if (FStrEq(CTF_TEAM1_NAME, m_szTeamName))
    {
      g_bTeam1FlagStolen = true;
      g_bTeam1FlagLost = false;


      //Glow red
			pPlayer->pev->renderfx = kRenderFxGlowShell;
			pPlayer->pev->rendercolor = Vector( 128, 0, 0 );	// RGB
			pPlayer->pev->renderamt = 50;	// Shell size

			iPowerUp = 2;
    }
		else if (FStrEq(CTF_TEAM2_NAME, m_szTeamName))
    {
			g_bTeam2FlagStolen = true;
      g_bTeam2FlagLost = false;

      //Glow blue
			pPlayer->pev->renderfx = kRenderFxGlowShell;
			pPlayer->pev->rendercolor = Vector( 0, 0, 128 );	// RGB
			pPlayer->pev->renderamt = 50;	// Shell size

			iPowerUp = 2;
    }

    UTIL_SendDirectorMessage( pPlayer->edict(), this->edict(), 8 | DRC_FLAG_DRAMATIC);

		return TRUE;		
	}
	return FALSE;
}

void AgCTFFlag::Think( void )
{
	if (m_bDropped && m_fNextReset <= gpGlobals->time)
	{
		//Let all players know that the flag has been returned
		char szText[201];

		if (FStrEq(CTF_TEAM1_NAME, m_szTeamName))
		{
			sprintf(szText, "The %s flag has returned.\n", CTF_TEAM1_NAME);
			ResetFlag(CTF_TEAM1_NAME);

      MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
        WRITE_BYTE( BlueFlagReturned );
      MESSAGE_END();
		}
		else if (FStrEq(CTF_TEAM2_NAME, m_szTeamName))
		{
			sprintf(szText, "The %s flag has returned.\n", CTF_TEAM2_NAME);
			ResetFlag(CTF_TEAM2_NAME);

      MESSAGE_BEGIN( MSG_ALL, gmsgCTFSound );
        WRITE_BYTE( RedFlagReturned );
      MESSAGE_END();
		}

    AgConsole(szText);
		UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );
		UTIL_Remove( this );
		return;
	}
	pev->frame += pev->framerate;
	if (pev->frame < 0.0 || pev->frame >= 256.0) 
	{
		pev->frame -= (int)(pev->frame / 256.0) * 256.0;
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

class AgCTFFlagTeam1 : public AgCTFFlag
{
	void Spawn( void )
	{
		pev->classname = MAKE_STRING("item_flag_team1"); //CCTF map compatibility hack
		strcpy( m_szTeamName, CTF_TEAM1_NAME );

		AgCTFFlag::Spawn( );
	}
};

#ifndef AG_NO_CLIENT_DLL
LINK_ENTITY_TO_CLASS( item_flag_team1, AgCTFFlagTeam1 );
LINK_ENTITY_TO_CLASS( ctf_blueflag, AgCTFFlagTeam1 ); //CCTF map compatibility hack
#endif

class AgCTFFlagTeam2 : public AgCTFFlag
{
	void Spawn( void )
	{
		pev->classname = MAKE_STRING("item_flag_team2"); //CCTF map compatibility hack
		strcpy( m_szTeamName, CTF_TEAM2_NAME );

		AgCTFFlag::Spawn( );
	}
};

#ifndef AG_NO_CLIENT_DLL
LINK_ENTITY_TO_CLASS( item_flag_team2, AgCTFFlagTeam2 );
LINK_ENTITY_TO_CLASS( ctf_redflag, AgCTFFlagTeam2 ); //CCTF map compatibility hack
#endif

class AgCTFFlagTeamOP4 : public AgCTFFlag
{
  int m_iTeam;
  void KeyValue( KeyValueData *pkvd )
  {
	  if (FStrEq(pkvd->szKeyName, "goal_no"))
    {
      m_iTeam = atoi(pkvd->szValue); 
		  pkvd->fHandled = TRUE;
    }
    else if (FStrEq(pkvd->szKeyName, "ctf_flag")) //HLE map compatibility hack
    {
      if (1 == atoi(pkvd->szValue))
        m_iTeam = 2; 
      else if (2 == atoi(pkvd->szValue))
        m_iTeam = 1;
      
		  pkvd->fHandled = TRUE;
    }
    else
		  AgCTFFlag::KeyValue( pkvd );
  }

	void Spawn( void )
	{
    if (1 == m_iTeam) 
    {
		    pev->classname = MAKE_STRING("item_flag_team1");
		    strcpy( m_szTeamName, CTF_TEAM1_NAME );
    }
    else if (2 == m_iTeam)
    {
		    pev->classname = MAKE_STRING("item_flag_team2");
		    strcpy( m_szTeamName, CTF_TEAM2_NAME );
    }

		AgCTFFlag::Spawn( );
	}
};
#ifndef AG_NO_CLIENT_DLL
LINK_ENTITY_TO_CLASS( item_ctfflag, AgCTFFlagTeamOP4 ); //OP4CTF map compatibility hack.
LINK_ENTITY_TO_CLASS( info_flag_ctf, AgCTFFlagTeamOP4 ); //HLE map compatibility hack
#endif
  


//=========================================================
// Carried Flag
//
// This is a complete new entity because it doesn't behave
// as a flag. It just sits on the back of the player and
// removes itself at the right time. (When player is gone
// or death or lost the flag.)
//=========================================================
void AgCTFPlayerFlag ::Spawn( )
{
	Precache( );

	SET_MODEL(ENT(pev), "models/flag.mdl");
	UTIL_SetOrigin( pev, pev->origin ); 

	pev->movetype = MOVETYPE_NONE; 
	pev->solid = SOLID_NOT;

	//HACKHACKHACK: Overcome the attachment with no owner yet model "hop" by making it invisible
	pev->effects |= EF_NODRAW;

	pev->sequence = WAVE_IDLE;
	pev->framerate = 1.0;

	if (FStrEq(CTF_TEAM1_NAME, m_szTeamName))
		pev->skin = 1;
	else if (FStrEq(CTF_TEAM2_NAME, m_szTeamName))
		pev->skin = 2;

  SetThink( &AgCTFPlayerFlag::Think );
	pev->nextthink = gpGlobals->time + 0.1;
}

void AgCTFPlayerFlag::UpdateOnRemove( void )
{
	if (FStrEq(CTF_TEAM1_NAME, m_szTeamName))
		s_iPlayerFlag1 = 0;
	else if (FStrEq(CTF_TEAM2_NAME, m_szTeamName))
		s_iPlayerFlag2 = 0;
}


void AgCTFPlayerFlag::Precache( )
{
	PRECACHE_MODEL ("models/flag.mdl");
}

void AgCTFPlayerFlag::Think( )
{
	//Make it visible
	pev->effects &= ~EF_NODRAW;

	//And let if follow
	pev->aiment = ENT(m_pOwner->pev);
	pev->movetype = MOVETYPE_FOLLOW;

	//Remove if owner is death
	if (!m_pOwner->IsAlive())
		UTIL_Remove( this );

	//If owner lost flag, remove
	if ( !m_pOwner->m_bFlagTeam1 && !m_pOwner->m_bFlagTeam2)
	{
		UTIL_Remove( this );
	}
	else
	{
		//If owners speed is low, go in idle mode
		if (m_pOwner->pev->velocity.Length() <= 75 && pev->sequence != WAVE_IDLE)
		{
			pev->sequence = WAVE_IDLE;
		}
		//Else let the flag go wild
		else if (m_pOwner->pev->velocity.Length() >= 75 && pev->sequence != CARRIED)
		{
			pev->sequence = CARRIED;
		}
		pev->frame += pev->framerate;
		if (pev->frame < 0.0 || pev->frame >= 256.0) 
		{
			pev->frame -= (int)(pev->frame / 256.0) * 256.0;
		}
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

class AgCTFPlayerFlagTeam1 : public AgCTFPlayerFlag
{
	void Spawn( void )
	{
		strcpy( m_szTeamName, CTF_TEAM1_NAME );

		AgCTFPlayerFlag::Spawn( );
	}
};

#ifndef AG_NO_CLIENT_DLL
LINK_ENTITY_TO_CLASS( carried_flag_team1, AgCTFPlayerFlagTeam1 );
#endif

class AgCTFPlayerFlagTeam2 : public AgCTFPlayerFlag
{
	void Spawn( void )
	{
		strcpy( m_szTeamName, CTF_TEAM2_NAME );

		AgCTFPlayerFlag::Spawn( );
	}
};

#ifndef AG_NO_CLIENT_DLL
LINK_ENTITY_TO_CLASS( carried_flag_team2, AgCTFPlayerFlagTeam2 );
#endif

class AgCTFDetect : public CBaseEntity
{
	void Spawn( void )
	{
		UTIL_SetOrigin( pev, pev->origin );
		pev->solid = SOLID_NOT;
		pev->effects = EF_NODRAW;		

    AgString sGametype = CVAR_GET_STRING("sv_ag_gametype");
    if (sGametype != "ctf")
      CVAR_SET_STRING("sv_ag_gamemode","ctf");
	}
	void KeyValue( KeyValueData* pkvd)
  { 
    pkvd->fHandled = FALSE; 
  }
};
#ifndef AG_NO_CLIENT_DLL
LINK_ENTITY_TO_CLASS( info_hmctfdetect, AgCTFDetect );
LINK_ENTITY_TO_CLASS( info_ctfdetect, AgCTFDetect );   //OP4 CTF detect
LINK_ENTITY_TO_CLASS( game_mode_ctf, AgCTFDetect ); //HLE CTF detect
#endif

/*
============
EntSelectCTFSpawnPoint

Returns the CTF entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/
extern CBaseEntity	*g_pLastSpawn;
BOOL IsSpawnPointValid( CBaseEntity *pPlayer, CBaseEntity *pSpot);
inline int FNullEnt( CBaseEntity *ent ) { return (ent == NULL) || FNullEnt( ent->edict() ); }


edict_t *EntSelectCTFSpawnPoint( CBaseEntity *pPlayer )
{
	CBaseEntity *pSpot;
	edict_t		*player;

	player = pPlayer->edict();
	CBasePlayer *cbPlayer = (CBasePlayer *)pPlayer; //we need a CBasePlayer

	pSpot = g_pLastSpawn;
	// Randomize the start spot
	for ( int i = RANDOM_LONG(1,5); i > 0; i-- )
	{
		if (FStrEq(cbPlayer->m_szTeamName, CTF_TEAM1_NAME))
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_team1" );
		else if (FStrEq(cbPlayer->m_szTeamName, CTF_TEAM2_NAME))
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_team2" );
		else
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
	}
	if ( FNullEnt( pSpot ) )  // skip over the null point
	{
		if (FStrEq(cbPlayer->m_szTeamName, CTF_TEAM1_NAME))
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_team1" );
		else if (FStrEq(cbPlayer->m_szTeamName, CTF_TEAM2_NAME))
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_team2" );
		else
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
	}

	CBaseEntity *pFirstSpot = pSpot;

	do 
	{
		if ( pSpot )
		{
			// check if pSpot is valid
			if ( IsSpawnPointValid( pPlayer, pSpot ) )
			{
				if ( pSpot->pev->origin == Vector( 0, 0, 0 ) )
				{
					if (FStrEq(cbPlayer->m_szTeamName, CTF_TEAM1_NAME))
						pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_team1" );
					else if (FStrEq(cbPlayer->m_szTeamName, CTF_TEAM2_NAME))
						pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_team2" );
					else
						pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
					continue;
				}

				// valid pSpot, so it can be returned
				goto ReturnSpot;
			}
		}
		// increment pSpot
		if (FStrEq(cbPlayer->m_szTeamName, CTF_TEAM1_NAME))
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_team1" );
		else if (FStrEq(cbPlayer->m_szTeamName, CTF_TEAM2_NAME))
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_team2" );
		else
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
	} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

	// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
	if ( !FNullEnt( pSpot ) )
	{
		CBaseEntity *ent = NULL;
		while ( (ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 128 )) != NULL )
		{
			// if ent is a client, kill em (unless they are ourselves)
			if ( ent->IsPlayer() && !(ent->edict() == player) )
				ent->TakeDamage( VARS(INDEXENT(0)), VARS(INDEXENT(0)), 300, DMG_GENERIC );
		}
		goto ReturnSpot;
	}

ReturnSpot:
	if ( FNullEnt( pSpot ) )
	{
		ALERT(at_error, "PutClientInServer: no info_player_team1,info_player_team2 on level");
    ClientPrint(pPlayer->pev,HUD_PRINTCENTER, "This is not a valid CTF map!" );
    pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
    if (FNullEnt( pSpot ))
		  return INDEXENT(0);
	}

	g_pLastSpawn = pSpot;
	return pSpot->edict();
}


class AgCTFSpawn : public CPointEntity
{
  int m_iTeam;
public:
	void		KeyValue( KeyValueData *pkvd );
	BOOL		IsTriggered( CBaseEntity *pEntity );
  void    Spawn( void );

private:
};

void AgCTFSpawn::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "master"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
  //OP4 map compatibility
	else if (FStrEq(pkvd->szKeyName, "team_no"))
  {
    m_iTeam = atoi(pkvd->szValue); 
		pkvd->fHandled = TRUE;
  }
  else
		CPointEntity::KeyValue( pkvd );
}

BOOL AgCTFSpawn::IsTriggered( CBaseEntity *pEntity )
{
	BOOL master = UTIL_IsMasterTriggered( pev->netname, pEntity );

	return master;
}

void AgCTFSpawn :: Spawn( void )
{
	//CCTF map compatibility
	if (FStrEq(STRING(pev->classname), "ctf_bluespawn"))
		pev->classname = MAKE_STRING("info_player_team1");
	else if (FStrEq(STRING(pev->classname), "ctf_redspawn"))
		pev->classname = MAKE_STRING("info_player_team2");
	//HLE map compatibility
	else if (FStrEq(STRING(pev->classname), "info_player_ctf_blue"))
		pev->classname = MAKE_STRING("info_player_team1");
	else if (FStrEq(STRING(pev->classname), "info_player_ctf_red"))
		pev->classname = MAKE_STRING("info_player_team2");
  //OP4 map compatibility
  else if (1 == m_iTeam)
		  pev->classname = MAKE_STRING("info_player_team1");
  else if (2 == m_iTeam)
		  pev->classname = MAKE_STRING("info_player_team2");
	pev->solid = SOLID_NOT;

  
}

#ifndef AG_NO_CLIENT_DLL
LINK_ENTITY_TO_CLASS(info_player_team1,AgCTFSpawn);
LINK_ENTITY_TO_CLASS(ctf_bluespawn,AgCTFSpawn); //CCTF map compatibility
LINK_ENTITY_TO_CLASS(info_player_team2,AgCTFSpawn);
LINK_ENTITY_TO_CLASS(ctf_redspawn,AgCTFSpawn); //CCTF map compatibility
LINK_ENTITY_TO_CLASS(info_ctfspawn,AgCTFSpawn); //OP4 CTF map compatibility
LINK_ENTITY_TO_CLASS(info_player_ctf_blue,AgCTFSpawn); //HLE CTF map compatibility
LINK_ENTITY_TO_CLASS(info_player_ctf_red,AgCTFSpawn); //HLE CTF map compatibility
#endif



#include "vector.h"

class AgCTFFileItemCache;


AgCTFFileItem::AgCTFFileItem()
{
  m_vOrigin = Vector(0,0,0);
  m_vAngles = Vector(0,0,0);
  m_szName[0] = '\0';
}

AgCTFFileItem::~AgCTFFileItem()
{

} 

void AgCTFFileItem::Show()
{
 	CLaserSpot* pSpot = CLaserSpot::CreateSpot();
	UTIL_SetOrigin( pSpot->pev, m_vOrigin );
	pSpot->LiveForTime(5.0);
}



AgCTFFileItemCache::AgCTFFileItemCache()
{
  m_bInitDone = false;
  Load();
}

AgCTFFileItemCache::~AgCTFFileItemCache()
{
  //Delete all.
  for (AgCTFFileItemList::iterator itrFileItems = m_lstFileItems.begin() ;itrFileItems != m_lstFileItems.end(); ++itrFileItems)
    delete *itrFileItems;
  m_lstFileItems.clear();
}

void AgCTFFileItemCache::Add(const AgString& sFileItem,CBasePlayer* pPlayer)
{ 
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
  
  if (0 == sFileItem.size())
    return;
  
  AgCTFFileItem* pFileItem = new AgCTFFileItem;
  strcpy(pFileItem->m_szName,sFileItem.c_str());
  pFileItem->m_vOrigin = pPlayer->pev->origin;
  pFileItem->m_vAngles = pPlayer->pev->angles;
  
  m_lstFileItems.push_back(pFileItem);
  pFileItem->Show();
  
  Save(pPlayer);
  
  AgConsole(UTIL_VarArgs("Added item %s.",(const char*)sFileItem.c_str()),pPlayer);
}

void AgCTFFileItemCache::Del(CBasePlayer* pPlayer)
{ 
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
  
  if (0 == m_lstFileItems.size())
    return;
  
  AgCTFFileItem* pFileItem = m_lstFileItems.back(); 
  AgConsole(UTIL_VarArgs("Deleted last item - %s.",pFileItem->m_szName,pPlayer));
  m_lstFileItems.pop_back();
  Save(pPlayer);
}

void AgCTFFileItemCache::List(CBasePlayer* pPlayer)
{ 
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
  
  for (AgCTFFileItemList::iterator itrFileItems = m_lstFileItems.begin() ;itrFileItems != m_lstFileItems.end(); ++itrFileItems)
  {
    AgConsole(UTIL_VarArgs("%s",(const char*)(*itrFileItems)->m_szName),pPlayer);
    (*itrFileItems)->Show();
  }
}

void AgCTFFileItemCache::Load(CBasePlayer* pPlayer)
{
  for (AgCTFFileItemList::iterator itrFileItems = m_lstFileItems.begin() ;itrFileItems != m_lstFileItems.end(); ++itrFileItems)
    delete *itrFileItems;
  m_lstFileItems.clear();
  

  char	szFile[MAX_PATH];
  char	szData[20000];
  sprintf(szFile, "%s/ctf/%s.ctf", AgGetDirectory(),STRING(gpGlobals->mapname));
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
    AgCTFFileItem* pFileItem = new AgCTFFileItem;
    sscanf(pszCTFString,"%s %f %f %f %f %f %f\n",pFileItem->m_szName,&pFileItem->m_vOrigin.x,&pFileItem->m_vOrigin.y,&pFileItem->m_vOrigin.z,&pFileItem->m_vAngles.x,&pFileItem->m_vAngles.y,&pFileItem->m_vAngles.z);
    m_lstFileItems.push_back(pFileItem);
    pszCTFString = strtok( NULL, "\n");
  }
}

void AgCTFFileItemCache::Save(CBasePlayer* pPlayer)
{
  if (0 == m_lstFileItems.size())
    return;
  
  char	szFile[MAX_PATH];
  sprintf(szFile, "%s/ctf/%s.ctf", AgGetDirectory(),STRING(gpGlobals->mapname));
  FILE* pFile = fopen(szFile,"wb");
  if (!pFile)
  {
    // file error
    AgConsole(UTIL_VarArgs("Couldn't create/save FileItem file %s.",szFile),pPlayer);
    return;
  }
  
  //Loop and write the file.
  for (AgCTFFileItemList::iterator itrFileItems = m_lstFileItems.begin() ;itrFileItems != m_lstFileItems.end(); ++itrFileItems)
  {
    //Append.
    AgCTFFileItem* pFileItem = *itrFileItems;
    fprintf(pFile,"%s %f %f %f %f %f %f\n",pFileItem->m_szName,pFileItem->m_vOrigin.x,pFileItem->m_vOrigin.y,pFileItem->m_vOrigin.z,pFileItem->m_vAngles.x,pFileItem->m_vAngles.y,pFileItem->m_vAngles.z);
  }
  
  fflush(pFile);
  fclose(pFile);
}


void AgCTFFileItemCache::Init()
{
  if (m_bInitDone)
    return;
  m_bInitDone = true;

  for (AgCTFFileItemList::iterator itrFileItems = m_lstFileItems.begin() ;itrFileItems != m_lstFileItems.end(); ++itrFileItems)
  {
    AgCTFFileItem* pFileItem = *itrFileItems;

    if (g_pGameRules->IsAllowedToSpawn(pFileItem->m_szName))
      CBaseEntity::Create(pFileItem->m_szName, pFileItem->m_vOrigin, pFileItem->m_vAngles, INDEXENT(0));
  }
}

//-- Martin Webrant

