//++ BulliT

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

extern int gmsgTeamInfo;
extern int gmsgSetFOV;
extern int gmsgSplash;

extern int gmsgGamemode;
extern int g_teamplay;

void CBasePlayer::Spectate_Init()
{
  m_fSpectateTime = AgTime();
  m_iSpot = 0;
  m_hSpectateTarget = NULL;
  m_iSpectateWeapon = 0;
  m_iSpectateAmmoClip = 0;
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
  g_pGameRules->m_bProxyConnected = true;
  //This is the valve proxy.
  if (g_pGameRules->IsTeamplay())
  {
    strcpy(m_szTeamName,"");
    g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", m_szTeamName );
    g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "team", m_szTeamName );
    g_pGameRules->RecountTeams();
  } 
  g_pGameRules->UpdateGameMode(this);
  g_pGameRules->ResendScoreBoard();
  return true;
}

void CBasePlayer::Spectate_Start(bool bResetScore)
{
  ASSERT(NULL != pev);
  if (!pev)
    return;
  
  if (IsSpectator()) //Don't start it if already spectator or if its a player in the arena.
    return;

  if (IsProxy())
    return;
  
  //CTF
  if (CTF == AgGametype())
    g_pGameRules->m_CTF.PlayerDropFlag(this);

  //Reset.
  Spectate_Init(); 
  
  //Set player as spectator.
  pev->iuser1 = OBS_ROAMING;
  pev->iuser2 = 0;
  m_hSpectateTarget = NULL;
  
  //Remove spectators from this player
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop && this != pPlayerLoop )
    {
      if ((CBaseEntity*)pPlayerLoop->m_hSpectateTarget == (CBaseEntity*)this)
      {
        //Move to next player.
        pPlayerLoop->Spectate_Nextplayer(false);
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
  m_hSpectateTarget = NULL;
  // clear attack/use commands from player
  m_afButtonPressed = 0;
  pev->button = 0;
  m_afButtonReleased = 0;
  m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_HEALTH;
  
  // reset FOV
  pev->fov = m_iFOV = m_iClientFOV = 0;
  MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
    WRITE_BYTE(0);
  MESSAGE_END();

  
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
  
  //Reset score
  if (bResetScore)
    ResetScore();
  
  pev->nextthink = gpGlobals->time + 0.1;
  
  //Tell clients
  UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s entered spectator mode\n",GetName()));

  //Put up splash screen for 10 secs :)
#ifndef AG_NO_CLIENT_DLL
  MESSAGE_BEGIN( MSG_ONE_UNRELIABLE, gmsgSplash, NULL, pev );
    WRITE_BYTE(10);
  MESSAGE_END();
#endif
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
  m_bDoneFirstSpawn = !bIntermediateSpawn;
  m_iHideHUD &= ~HIDEHUD_WEAPONS;
  m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
  m_iHideHUD &= ~HIDEHUD_HEALTH;

  //Reset fov
  pev->fov = m_iFOV = m_iClientFOV = 0;
  MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
    WRITE_BYTE(0);
  MESSAGE_END();
  
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

#ifndef AG_NO_CLIENT_DLL
  //Remove splash screen
  MESSAGE_BEGIN( MSG_ONE, gmsgSplash, NULL, pev );
    WRITE_BYTE(0);
  MESSAGE_END();
#endif
}

// Find the next client in the game for this player to spectate
void CBasePlayer::Spectate_Nextplayer( bool bReverse )
{
  ASSERT(NULL != pev);
  if (!pev)
    return;

  if (IsProxy())
    return;
  
  // MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
  //				only a subset of the players. e.g. Make it check the target's team.
  
  int		iStart;
  if ( m_hSpectateTarget )
    iStart = ENTINDEX( m_hSpectateTarget->edict() );
  else
    iStart = ENTINDEX( edict() );
  
  int	    iCurrent = iStart;
  m_hSpectateTarget = NULL;
  int iDir = bReverse ? -1 : 1; 
  
  do
  {
    iCurrent += iDir;
    
    // Loop through the clients
    if (iCurrent > gpGlobals->maxClients)
      iCurrent = 1;
    if (iCurrent < 1)
      iCurrent = gpGlobals->maxClients;
    
    CBaseEntity *pEnt = AgPlayerByIndex( iCurrent );
    if ( !pEnt )
      continue;
    if ( pEnt == this )
      continue;

    // Don't spec observers or invisible players
    if ( ((CBasePlayer*)pEnt)->IsSpectator() || (pEnt->pev->effects == EF_NODRAW) )
      continue;

    if (0 < ag_spec_enable_disable.value &&  ((CBasePlayer*)pEnt)->DisableSpecs())
        continue;

    m_hSpectateTarget = pEnt;
    break;
  } 
  while ( iCurrent != iStart );
  
  // Did we find a target?
  if (m_hSpectateTarget != NULL && m_hSpectateTarget->pev != NULL)
  {
    // Store the target in pev so the physics DLL can get to it
    pev->iuser2 = ENTINDEX( m_hSpectateTarget->edict() );
    // Move to the target
    UTIL_SetOrigin( pev, m_hSpectateTarget->pev->origin );
    
    //ClientPrint(m_hSpectateTarget->pev, HUD_PRINTNOTIFY,  UTIL_VarArgs("You are being watched by %s\n",STRING(pev->netname)));
  }
  else
  {
    //Go roaming.
    Spectate_SetMode(OBS_ROAMING);
 }
}


// Handle buttons in spectate mode
void CBasePlayer::Spectate_HandleButtons()
{
  ASSERT(NULL != pev);
  if (!pev)
    return;
  
  if (IsProxy())
    return;

  // Slow down mouse clicks
  if ( m_fSpectateTime > AgTime() )
    return;

  pev->impulse = 0;
  
  // Jump changes from modes: Chase to Roaming
  if ( m_afButtonPressed & IN_JUMP )
  {
    //Reset fov
    pev->fov = m_iFOV = m_iClientFOV = 0;
    MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
      WRITE_BYTE(0);
    MESSAGE_END();

    if ( pev->iuser1 == OBS_ROAMING )
      Spectate_SetMode( OBS_IN_EYE );
    else if ( pev->iuser1 == OBS_IN_EYE )
      Spectate_SetMode( OBS_CHASE_LOCKED );
    else if ( pev->iuser1 == OBS_CHASE_LOCKED )
	  Spectate_SetMode( OBS_CHASE_FREE );
    else if ( pev->iuser1 == OBS_CHASE_FREE )
      Spectate_SetMode( OBS_MAP_CHASE );
	else if ( pev->iuser1 == OBS_MAP_CHASE)
      Spectate_SetMode( OBS_MAP_FREE );
	else if ( pev->iuser1 == OBS_MAP_FREE )
	  Spectate_SetMode( OBS_ROAMING );
    else
      Spectate_SetMode( OBS_ROAMING );
  }
  
  // Attack moves to the next player
  if ( m_afButtonPressed & IN_ATTACK)
  {
    //Reset fov
    pev->fov = m_iFOV = m_iClientFOV = 0;
    MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
      WRITE_BYTE(0);
    MESSAGE_END();

    if (pev->iuser2 != 0)
      Spectate_Nextplayer(false);
    else
      Spectate_Nextspot(false);
    
    m_fSpectateTime = AgTime() + 0.2;
  }
  
  // Attack2 moves to the prev player
  if ( m_afButtonPressed & IN_ATTACK2)
  {
    //Reset fov
    pev->fov = m_iFOV = m_iClientFOV = 0;
    MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
      WRITE_BYTE(0);
    MESSAGE_END();

    if (pev->iuser2 != 0)
      Spectate_Nextplayer(true);
    else
      Spectate_Nextspot(true);
    
    m_fSpectateTime = AgTime() + 0.2;
  }
}

void CBasePlayer::Spectate_UpdatePosition()
{
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
	  if (pPlayerLoop && this != pPlayerLoop && pPlayerLoop->IsSpectator())
    {
      if ((CBaseEntity*)pPlayerLoop->m_hSpectateTarget == (CBaseEntity*)this)
      {
		    if (pPlayerLoop->m_hSpectateTarget->pev)
		    {
			    //Update the spectators position
			    //pPlayerLoop->m_hSpectateTarget = (CBaseEntity*)this;
			    UTIL_SetOrigin( pPlayerLoop->pev, pev->origin );
		    }
      }
    }
  }
}

// Attempt to change the spectate mode
void CBasePlayer::Spectate_SetMode( int iMode )
{
  ASSERT(NULL != pev);
  if (!pev)
    return;
  
  if (IsProxy() || !IsSpectator())
    return;

  // Just abort if we're changing to the mode we're already in
  if ( iMode == pev->iuser1 )
    return;

  // Changing to Roaming or Map Free?
  if ( iMode == OBS_ROAMING || iMode == OBS_MAP_FREE)
  {
    // MOD AUTHORS: If you don't want to allow roaming observers at all in your mod, just abort here.
    pev->iuser1 = iMode;
    pev->iuser2 = 0;
    m_hSpectateTarget = NULL;

    pev->weapons &= ~WEAPON_ALLWEAPONS;
    m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_HEALTH;
    return;
  }
  
  // Changing to Chase Lock, Chase Freelook, Map Chase?
  if (iMode == OBS_CHASE_FREE  || iMode == OBS_MAP_CHASE || iMode == OBS_CHASE_LOCKED)
  {
    // If changing from Roaming, or starting observing, make sure there is a target
    if ( m_hSpectateTarget == NULL )
      Spectate_Nextplayer( false );
    
    if (m_hSpectateTarget)
    {
      pev->iuser1 = iMode;
      pev->iuser2 = ENTINDEX( m_hSpectateTarget->edict() );
      pev->maxspeed = 0;

      pev->weapons |= (1<<WEAPON_SUIT);
      m_iHideHUD |= HIDEHUD_WEAPONS;
      m_iHideHUD |= HIDEHUD_FLASHLIGHT;
      m_iHideHUD &= ~HIDEHUD_HEALTH;
    }
    else
    {
      Spectate_SetMode(OBS_ROAMING);
    }
    
    return;
  }

  // Changing to in-eye?
  if ( iMode == OBS_IN_EYE)
  {
    // If changing from Roaming, or starting observing, make sure there is a target
    if ( m_hSpectateTarget == NULL )
      Spectate_Nextplayer( false );
    
    if (m_hSpectateTarget)
    {
      pev->iuser1 = iMode;
      pev->iuser2 = ENTINDEX( m_hSpectateTarget->edict() );
      pev->maxspeed = 0;

      pev->weapons |= (1<<WEAPON_SUIT);
      m_iHideHUD &= ~HIDEHUD_WEAPONS;
      m_iHideHUD |= HIDEHUD_FLASHLIGHT;
      m_iHideHUD &= ~HIDEHUD_HEALTH;
    }
    else
    {
      Spectate_SetMode(OBS_ROAMING);
    }
    
    return;
  }
}


bool CBasePlayer::Spectate_Think()
{
  ASSERT(NULL != pev);
  if (!pev)
    return false;

  if (IsProxy())
	  return false;
  
  if (IsSpectator())
  {
    Spectate_HandleButtons();
    return true;
  }
  return false;
}


// Find the next info intermission spot
void CBasePlayer::Spectate_Nextspot(bool bReverse)
{
  ASSERT(NULL != pev);
  if (!pev || !g_pGameRules)
    return;
  
  if (IsProxy())
    return;

  m_iSpot += bReverse ? -1 : 1;
  
  //Check if out of bounds.
  if (0 > m_iSpot)
    m_iSpot = g_pGameRules->m_InfoInterMission.GetCount() - 1;
  else if (m_iSpot >= g_pGameRules->m_InfoInterMission.GetCount())
    m_iSpot = 0;
  
  //Move the dude
  edict_t* pSpot = g_pGameRules->m_InfoInterMission.GetSpot(m_iSpot);
  if (pSpot)
    MoveToInfoIntermission(pSpot);
}

bool CBasePlayer::Spectate_Follow(EHANDLE& pPlayer,int iMode)
{
  if (IsProxy())
    return true;

  m_hSpectateTarget = pPlayer;
  Spectate_SetMode(iMode);
  return true;
}

//-- Martin Webrant
