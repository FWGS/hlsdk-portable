//++ BulliT

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "agglobal.h"

extern int gmsgTeamInfo;
extern int gmsgSetFOV;
extern int gmsgSpectator;

void CBasePlayer::Spectate_Init()
{
  m_hObserverTarget = NULL;
}


//Spectate code with parts from Robin Walkers Tutorial.
void CBasePlayer::Spectate_Spectate()
{
  ASSERT(NULL != pev);
  if (!pev)
    return;
  
  if (IsSpectator())
    Spectate_Stop();
  else
    Spectate_Start();
}

bool CBasePlayer::Spectate_HLTV()
{
  //This is the valve proxy.
  if (g_pGameRules->IsTeamplay())
  {
    strcpy(m_szTeamName,"");
    g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", m_szTeamName );
    g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "team", m_szTeamName );
    if (g_pGameRules->IsTeamplay())
      ((CHalfLifeTeamplay*)g_pGameRules)->RecountTeams();
  } 
  g_pGameRules->UpdateGameMode(this);
  return true;
}

void CBasePlayer::Spectate_Start()
{
  ASSERT(NULL != pev);
  if (!pev)
    return;
  
  if (IsSpectator()) //Don't start it if already spectator or if its a player in the arena.
    return;

  if (IsProxy())
    return;

  //Reset.
  m_iQuakeWeapon = 0;
  Spectate_Init(); 
  
  //Set player as spectator.
  pev->iuser1 = OBS_ROAMING;
  pev->iuser2 = 0;
  m_hObserverTarget = NULL;
  
  //Remove spectators from this player
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop && this != pPlayerLoop )
    {
      if ((CBaseEntity*)pPlayerLoop->m_hObserverTarget == (CBaseEntity*)this)
      {
        //Move to next player.
        pPlayerLoop->Observer_FindNextPlayer();
      }
    }
  }
  
  // clear any clientside entities attached to this player
  MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
    WRITE_BYTE( TE_KILLPLAYERATTACHMENTS );
    WRITE_BYTE( (BYTE)entindex() );
  MESSAGE_END();  

  
  pev->health	= 1; //Remove clientside screentilt. If you find this one helpful - give me credit because I spent 3 hours a friday night figuring it out!  
  
  EnableControl(TRUE);
  
  if ( m_pTank != NULL )
  {
    m_pTank->Use( this, this, USE_OFF, 0 );
    m_pTank = NULL;
  }
  
  // clear out the suit message cache so we don't keep chattering
  SetSuitUpdate(NULL, FALSE, 0);
  
  RemoveAllItemsNoClientMessage();

  pev->deadflag = DEAD_DEAD;
  pev->flags |= FL_SPECTATOR;
  pev->flags |= FL_NOTARGET;

  ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
  ClearBits( pev->flags, FL_DUCKING );
  SetBits(m_afPhysicsFlags, PFLAG_OBSERVER);
  UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
  
  pev->fixangle = TRUE;
  pev->solid = SOLID_NOT;
  pev->takedamage = DAMAGE_NO;
  pev->movetype = MOVETYPE_NOCLIP;
  pev->effects |= EF_NODRAW;
  pev->view_ofs = g_vecZero;
  m_hObserverTarget = NULL;
  // clear attack/use commands from player
  m_afButtonPressed = 0;
  pev->button = 0;
  m_afButtonReleased = 0;
  m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_HEALTH;

  
  MESSAGE_BEGIN(MSG_ALL,gmsgSpectator);
    WRITE_BYTE( ENTINDEX(edict()) );
    WRITE_BYTE(1);
  MESSAGE_END();
  
  //Change teamname
  if (g_pGameRules->IsTeamplay())
  {
    MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
      WRITE_BYTE( entindex() );
      WRITE_STRING( TeamID() );
    MESSAGE_END();
  }
  
  
  pev->nextthink = gpGlobals->time + 0.1;
  
  //Tell clients
  UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s entered spectator mode\n",GetName()));
}


void CBasePlayer::Spectate_Stop(bool bIntermediateSpawn)
{
  ASSERT(NULL != pev);
  if (!pev)
    return;
  
  if (!IsSpectator())
    return;
  
  if (IsProxy())
    return;

  if (!g_pGameRules->FPlayerCanRespawn( this))
    return;
  
  EnableControl(TRUE);
  
  //Reset flags.
  Spectate_Init();
  pev->flags &= ~FL_SPECTATOR;
  pev->flags &= ~FL_NOTARGET;
  pev->deadflag = DEAD_RESPAWNABLE; 
  pev->button = 0;
  pev->iuser1 = OBS_NONE;
  pev->iuser2 = 0;
  pev->effects &= ~EF_NODRAW;
  pev->movetype = MOVETYPE_WALK;
  m_iRespawnFrames = 0;
  m_bHadFirstSpawn = !bIntermediateSpawn;
  m_iHideHUD &= ~HIDEHUD_WEAPONS;
  m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
  m_iHideHUD &= ~HIDEHUD_HEALTH;

  //Remove spec
  MESSAGE_BEGIN(MSG_ALL,gmsgSpectator);
    WRITE_BYTE(ENTINDEX(edict()));
    WRITE_BYTE(0);
  MESSAGE_END();
  
  //Change teamname
  
  if (g_pGameRules->IsTeamplay())
  {
    MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
      WRITE_BYTE( entindex() );
      WRITE_STRING( TeamID() );
    MESSAGE_END();
  }
  
  //Force data to be resent.
  m_fKnownItem = FALSE;   // Force weaponinit messages.

  //Tell client(s)
  UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s left spectator mode\n",GetName()));
  
  Spawn();

}

void CBasePlayer::Spectate_UpdatePosition()
{
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
	  if (pPlayerLoop && this != pPlayerLoop && pPlayerLoop->IsSpectator())
    {
      if ((CBaseEntity*)pPlayerLoop->m_hObserverTarget == (CBaseEntity*)this)
      {
		    if (pPlayerLoop->m_hObserverTarget->pev)
		    {
			    //Update the spectators position
			    UTIL_SetOrigin( pPlayerLoop->pev, pev->origin );
		    }
      }
    }
  }
}



bool CBasePlayer::Spectate_Follow(EHANDLE& pPlayer,int iMode)
{
  if (IsProxy())
    return true;

  m_hObserverTarget = pPlayer;
  Observer_SetMode(iMode);
  return true;
}

//-- Martin Webrant
