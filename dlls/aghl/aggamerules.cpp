//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gamerules.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#include "agglobal.h"
#include "agcommand.h"
#include "agvote.h"
#include "agclient.h"
#include "aggamerules.h"
#ifdef AGSTATS
#include "agstats.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern int gmsgTeamInfo;
extern int gmsgScoreInfo;
extern int gmsgAllowSpec;
extern int gmsgServerName;
extern int gmsgSpikeCheck;
extern int gmsgGametype;
extern int g_teamplay;
extern int gmsgGameMode;
extern int gmsgAuthID;
extern int gmsgMapList;

AgGameRules::AgGameRules()
{
  g_bPaused = false;
  m_bProxyConnected = false;
  g_bUseTeamColors = CVAR_GET_FLOAT( "mp_teamplay" ) > 0;
  m_sHostname = CVAR_GET_STRING("hostname");
  AdminCache.Load();
#ifdef AG_NO_CLIENT_DLL
  m_LocationCache.Load();
#endif
#ifdef AGSTATS
  Stats.Reset();
#endif
}

AgGameRules::~AgGameRules()
{
  AdminCache.Save();
}

bool AgGameRules::AgThink()
{
  //Check if gamerules are correct.
  if (!m_Settings.Think())
    //Dont do anything more.
    return false;
  
  //Check if game over.
  if (g_fGameOver)
    return true;
  
  //Update HUD timer and effective time.
  m_Timer.Think();
  
  //Update vote status.
  //Handled globally m_Vote.Think();
  
  if (CTF == AgGametype())
    m_CTF.Think();
  //++ muphicks
  else if (DOM == AgGametype())
    m_DOM.Think();
  //-- muphicks

  if (LMS == AgGametype())
  {
    //Arena status.
    m_LMS.Think();
  }
  else if (ARENA == AgGametype())
  {
    //Arena status.
    m_Arena.Think();
  }
  else
  {
    //Update match status.
    m_Match.Think();
    //Scorelog
    m_ScoreLog.Think();
  }
  
  //Init intermission spots.
  m_InfoInterMission.Think();

  //Check gamemode
  GameMode.Think();
  return true;
}

BOOL AgGameRules::ClientCommand(CBasePlayer* pPlayer, const char *pcmd)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return FALSE;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return FALSE;

  //First check if its a client command. weaponswith,say etc.
  if (m_Client.HandleCommand(pPlayer))
    return TRUE;

  //Server command?
  if (Command.HandleCommand(pPlayer))
    return TRUE;
  
  //Gamemode?
  if (GameMode.HandleCommand(pPlayer))
    return TRUE;

  //Vote?
  if (m_Vote.HandleCommand(pPlayer))
    return TRUE;

#ifdef AGSTATS
  if (Stats.HandleCommand(pPlayer))
    return TRUE;
#endif

  //We didn't handle it
  return FALSE;
}

void AgGameRules::Start(const AgString& sSpawn)
{
  if (ARENA != AgGametype() && LMS != AgGametype())
    m_Match.Start(sSpawn);
}


int AgGameRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
  if (ARENA == AgGametype() || ARCADE == AgGametype())
    return GR_PLR_DROP_GUN_NO;
  else
    return GR_PLR_DROP_GUN_ACTIVE;
}

int AgGameRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
  if (ARENA == AgGametype() || ARCADE == AgGametype())
    return GR_PLR_DROP_AMMO_NO;
  else
    return GR_PLR_DROP_AMMO_ACTIVE;
}

BOOL AgGameRules::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return FALSE;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return FALSE;
  
  return pPlayer->IsIngame();
}


void AgGameRules::PlayerSpawn(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
  
  if (!FPlayerCanRespawn(pPlayer) && !pPlayer->IsProxy())
  {
    if (!pPlayer->m_bDoneFirstSpawn)
    {
      pPlayer->m_bDoneFirstSpawn = true;
    }

    if (!pPlayer->IsSpectator())
    {
      //Check if we boot him.
      int iSpectators = 0;
      for ( int i = 1; i <= gpGlobals->maxClients; i++ )
      {
        CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
        if (pPlayerLoop && pPlayer != pPlayerLoop )
        {
          if (pPlayerLoop->IsSpectator())
            iSpectators++;
        }
      }
      
      if (iSpectators >= ag_max_spectators.value && ARENA != AgGametype() && LMS != AgGametype() && !pPlayer->IsAdmin())
      {
        //He has to go.
        AgConsole("To many spectators, kicking him.");
        char szCommand[128];
        sprintf(szCommand,"kick \"%s\"\n",(const char*)pPlayer->GetName());
        SERVER_COMMAND( szCommand );
        return;
      }
      
      //Go specmode
      pPlayer->Spectate_Start(true);
    }
    ClientPrint(pPlayer->pev,HUD_PRINTCENTER,"Match is running, you are not allowed to enter the game.\n");
    return;
  }
  
  if (pPlayer->IsProxy() || pPlayer->pev->flags & FL_FAKECLIENT)
    pPlayer->m_bDoneFirstSpawn = true;
  //if (1 == pPlayer->pev->iuser3)
    //pPlayer->m_bDoneFirstSpawn = true;
  if (!pPlayer->m_bDoneFirstSpawn)
  {
    pPlayer->m_bDoneFirstSpawn = true;
    pPlayer->pev->takedamage = DAMAGE_NO;
    pPlayer->pev->flags |= FL_SPECTATOR;
    pPlayer->pev->flags |= FL_NOTARGET;
    pPlayer->pev->effects |= EF_NODRAW;
    pPlayer->pev->solid = SOLID_NOT;
    pPlayer->pev->movetype = MOVETYPE_NOCLIP;
    pPlayer->pev->modelindex = 0;
    pPlayer->m_pGoalEnt = NULL;
    
    //Move player to info intermission spot
    edict_t* pSpot = m_InfoInterMission.GetRandomSpot();
    ASSERT(NULL != pSpot);
    if (pSpot)
      pPlayer->MoveToInfoIntermission(pSpot);
    
    //Display Gamemode
    pPlayer->SetDisplayGamemode(5);

    //Display greeting message
    AgDisplayGreetingMessage(pPlayer->GetAuthID());

    return;
  }
  
  BOOL		addDefault;
  CBaseEntity	*pWeaponEntity = NULL;
  
  pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
  
  addDefault = TRUE;
  
  while ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ))
  {
    pWeaponEntity->Touch( pPlayer );
    addDefault = FALSE;
  }
  
  pPlayer->m_bInSpawn = true;

  if ( addDefault )
  {
    if (pPlayer->GetSpawnFull())
    {
      //American spawn mode... Start with full load of everything.
      pPlayer->SetSpawnFull(false); 

      pPlayer->pev->health = MAX_NORMAL_BATTERY;
      pPlayer->pev->armorvalue = MAX_NORMAL_BATTERY;
      
      if (1 > ag_ban_longjump.value)
      {
        pPlayer->m_fLongJump = TRUE;
        g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "slj", "1" );
        pPlayer->OnPickupLongjump();
      }
      if (1 > ag_ban_glock.value)
        pPlayer->GiveNamedItem( "weapon_9mmhandgun" );
      if (1 > ag_ban_crowbar.value)
        pPlayer->GiveNamedItem( "weapon_crowbar" );
      if (1 > ag_ban_shotgun.value)
        pPlayer->GiveNamedItem( "weapon_shotgun" );
      if (1 > ag_ban_mp5.value)
        pPlayer->GiveNamedItem( "weapon_9mmAR" );
      if (1 > ag_ban_gauss.value)
        pPlayer->GiveNamedItem( "weapon_gauss" );
      if (1 > ag_ban_hgrenade.value)
        pPlayer->GiveNamedItem( "weapon_handgrenade" );
      if (1 > ag_ban_tripmine.value)
        pPlayer->GiveNamedItem( "weapon_tripmine" );
      if (1 > ag_ban_egon.value)
        pPlayer->GiveNamedItem( "weapon_egon" );
      if (1 > ag_ban_crossbow.value)
        pPlayer->GiveNamedItem( "weapon_crossbow" );
      if (1 > ag_ban_357.value)
        pPlayer->GiveNamedItem( "weapon_357" );
      if (1 > ag_ban_rpg.value)
        pPlayer->GiveNamedItem( "weapon_rpg" );
      if (1 > ag_ban_satchel.value)
        pPlayer->GiveNamedItem( "weapon_satchel" );
      if (1 > ag_ban_snark.value)
        pPlayer->GiveNamedItem( "weapon_snark" );
      if (1 > ag_ban_hornet.value)
        pPlayer->GiveNamedItem( "weapon_hornetgun" );
      
      if (1 > ag_ban_hgrenade.value)
        pPlayer->GiveAmmo( HANDGRENADE_MAX_CARRY, "Hand Grenade", HANDGRENADE_MAX_CARRY );
      if (1 > ag_ban_satchel.value)
        pPlayer->GiveAmmo( SATCHEL_MAX_CARRY, "Satchel Charge", SATCHEL_MAX_CARRY );
      if (1 > ag_ban_tripmine.value)
        pPlayer->GiveAmmo( TRIPMINE_MAX_CARRY, "Trip Mine", TRIPMINE_MAX_CARRY );
      if (1 > ag_ban_snark.value)
        pPlayer->GiveAmmo( SNARK_MAX_CARRY, "Snarks", SNARK_MAX_CARRY );
      if (1 > ag_ban_hornet.value)
        pPlayer->GiveAmmo( HORNET_MAX_CARRY, "Hornets", HORNET_MAX_CARRY );
      if (1 > ag_ban_m203.value)
        pPlayer->GiveAmmo( M203_GRENADE_MAX_CARRY, "ARgrenades", M203_GRENADE_MAX_CARRY );
      if (1 > ag_ban_egon.value && 1 > ag_ban_gauss.value)
        pPlayer->GiveAmmo( URANIUM_MAX_CARRY, "uranium", URANIUM_MAX_CARRY );
      if (1 > ag_ban_glock.value && 1 > ag_ban_9mmar.value)
        pPlayer->GiveAmmo( _9MM_MAX_CARRY, "9mm", _9MM_MAX_CARRY );
      if (1 > ag_ban_357.value)
        pPlayer->GiveAmmo( _357_MAX_CARRY, "357", _357_MAX_CARRY );
      if (1 > ag_ban_shotgun.value)
        pPlayer->GiveAmmo( BUCKSHOT_MAX_CARRY, "buckshot", BUCKSHOT_MAX_CARRY );
      if (1 > ag_ban_crossbow.value)
        pPlayer->GiveAmmo( BOLT_MAX_CARRY, "bolts", BOLT_MAX_CARRY );
      if (1 > ag_ban_rpg.value)
        pPlayer->GiveAmmo( ROCKET_MAX_CARRY, "rockets", ROCKET_MAX_CARRY );
    }
    else 
    {
      //Normal spawn.
      pPlayer->pev->health = ag_start_health.value;
      pPlayer->pev->armorvalue = ag_start_armour.value;
    
      if (0 < ag_start_longjump.value)
      {
        pPlayer->m_fLongJump = TRUE;
        g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "slj", "1" );
        pPlayer->OnPickupLongjump();
      }
      if (0 < ag_start_glock.value)
        pPlayer->GiveNamedItem( "weapon_9mmhandgun" );
      if (0 < ag_start_crowbar.value)
        pPlayer->GiveNamedItem( "weapon_crowbar" );
      if (0 < ag_start_shotgun.value)
        pPlayer->GiveNamedItem( "weapon_shotgun" );
      if (0 < ag_start_mp5.value)
        pPlayer->GiveNamedItem( "weapon_9mmAR" );
      if (0 < ag_start_gauss.value)
        pPlayer->GiveNamedItem( "weapon_gauss" );
      if (0 < ag_start_hgrenade.value)
        pPlayer->GiveNamedItem( "weapon_handgrenade" );
      if (0 < ag_start_tripmine.value)
        pPlayer->GiveNamedItem( "weapon_tripmine" );
      if (0 < ag_start_egon.value)
        pPlayer->GiveNamedItem( "weapon_egon" );
      if (0 < ag_start_crossbow.value)
        pPlayer->GiveNamedItem( "weapon_crossbow" );
      if (0 < ag_start_357.value)
        pPlayer->GiveNamedItem( "weapon_357" );
      if (0 < ag_start_rpg.value)
        pPlayer->GiveNamedItem( "weapon_rpg" );
      if (0 < ag_start_satchel.value)
        pPlayer->GiveNamedItem( "weapon_satchel" );
      if (0 < ag_start_snark.value)
        pPlayer->GiveNamedItem( "weapon_snark" );
      if (0 < ag_start_hornet.value)
        pPlayer->GiveNamedItem( "weapon_hornetgun" );
    
      if (0 < ag_start_hgrenade.value)
        pPlayer->GiveAmmo( ag_start_hgrenade.value, "Hand Grenade", HANDGRENADE_MAX_CARRY );
      if (0 < ag_start_satchel.value)
        pPlayer->GiveAmmo( ag_start_satchel.value, "Satchel Charge", SATCHEL_MAX_CARRY );
      if (0 < ag_start_tripmine.value)
        pPlayer->GiveAmmo( ag_start_tripmine.value, "Trip Mine", TRIPMINE_MAX_CARRY );
      if (0 < ag_start_snark.value)
        pPlayer->GiveAmmo( ag_start_snark.value, "Snarks", SNARK_MAX_CARRY );
      if (0 < ag_start_hornet.value)
        pPlayer->GiveAmmo( ag_start_hornet.value, "Hornets", HORNET_MAX_CARRY );
      if (0 < ag_start_m203.value)
        pPlayer->GiveAmmo( ag_start_m203.value, "ARgrenades", M203_GRENADE_MAX_CARRY );
      if (0 < ag_start_uranium.value)
        pPlayer->GiveAmmo( ag_start_uranium.value, "uranium", URANIUM_MAX_CARRY );
      if (0 < ag_start_9mmar.value)
        pPlayer->GiveAmmo( ag_start_9mmar.value, "9mm", _9MM_MAX_CARRY );
      if (0 < ag_start_357ammo.value)
        pPlayer->GiveAmmo( ag_start_357ammo.value, "357", _357_MAX_CARRY );
      if (0 < ag_start_bockshot.value)
        pPlayer->GiveAmmo( ag_start_bockshot.value, "buckshot", BUCKSHOT_MAX_CARRY );
      if (0 < ag_start_bolts.value)
        pPlayer->GiveAmmo( ag_start_bolts.value, "bolts", BOLT_MAX_CARRY );
      if (0 < ag_start_rockets.value)
        pPlayer->GiveAmmo( ag_start_rockets.value, "rockets", ROCKET_MAX_CARRY );
    }
  }
  pPlayer->m_bInSpawn = false;

  /*
  //Quake1 teleport splash around him.
  MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
    WRITE_BYTE( TE_TELEPORT ); 
    WRITE_COORD(pPlayer->pev->origin.x);
    WRITE_COORD(pPlayer->pev->origin.y);
    WRITE_COORD(pPlayer->pev->origin.z);
  MESSAGE_END();
  */
}

BOOL AgGameRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return FALSE;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return FALSE;

  ASSERT(NULL != pWeapon);
  if (!pWeapon)
    return FALSE;
  
  if ( !pWeapon->CanDeploy() )
  {
    // that weapon can't deploy anyway.
    return FALSE;
  }
  
  if ( !pPlayer->m_pActiveItem )
  {
    // player doesn't have an active item!
    return TRUE;
  }
  
  if ( !pPlayer->m_pActiveItem->CanHolster() )
  {
    // can't put away the active item.
    return FALSE;
  }
  
  if (pPlayer->ShouldWeaponSwitch() && (pPlayer->GetWeaponWeight(pWeapon) > pPlayer->GetWeaponWeight(pPlayer->m_pActiveItem)))
  {
    return TRUE;
  }
  return FALSE;
}


BOOL AgGameRules::GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return FALSE;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return FALSE;

  ASSERT(NULL != pCurrentWeapon);
  if (!pCurrentWeapon)
    return FALSE;
  

	CBasePlayerItem *pCheck;
	CBasePlayerItem *pBest;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight;
	int i;

	iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	if ( !pCurrentWeapon->CanHolster() )
	{
		// can't put this gun away right now, so can't switch.
		return FALSE;
	}

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pCheck = pPlayer->m_rgpPlayerItems[ i ];

		while ( pCheck )
		{
			if ( pPlayer->GetWeaponWeight(pCheck) > -1 && pPlayer->GetWeaponWeight(pCheck) == pPlayer->GetWeaponWeight(pCurrentWeapon) && pCheck != pCurrentWeapon )
			{
				// this weapon is from the same category. 
				if ( pCheck->CanDeploy() )
				{
					if ( pPlayer->SwitchWeapon( pCheck ) )
					{
						return TRUE;
					}
				}
			}
			else if ( pPlayer->GetWeaponWeight(pCheck) > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
			{
				//ALERT ( at_console, "Considering %s\n", STRING( pCheck->pev->classname ) );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted 
				// weapon. 
				if ( pCheck->CanDeploy() )
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pPlayer->GetWeaponWeight(pCheck);
					pBest = pCheck;
				}
			}

			pCheck = pCheck->m_pNext;
		}
	}

	// if we make it here, we've checked all the weapons and found no useable 
	// weapon in the same catagory as the current weapon. 
	
	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
	// at least get the crowbar, but ya never know.
	if ( !pBest )
	{
		return FALSE;
	}

	pPlayer->SwitchWeapon( pBest );

	return TRUE;
}

const char* AgGameRules::GetIPAddress(edict_t *pEntity)
{
	if (0 == m_mapIPAddress.size())
		return "";
  AgIPAddress::iterator itrIPAddress = m_mapIPAddress.find(ENTINDEX(pEntity));
  if (itrIPAddress != m_mapIPAddress.end())
    return (*itrIPAddress).second.c_str();
  return "";

}

BOOL AgGameRules :: ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{
  if (0 != strcmp("127.0.0.1", pszAddress))
  {
	  if (0 == m_mapIPAddress.size())
	  {
		  m_mapIPAddress.insert(AgIPAddress::value_type(ENTINDEX(pEntity), pszAddress));
		  return TRUE;
	  }

		AgIPAddress::iterator itrIPAddress = m_mapIPAddress.find(ENTINDEX(pEntity));
		if (itrIPAddress == m_mapIPAddress.end())
			m_mapIPAddress.insert(AgIPAddress::value_type(ENTINDEX(pEntity), pszAddress));
		else
			(*itrIPAddress).second = ENTINDEX(pEntity);
  }
  return TRUE;
}

void AgGameRules::ClientDisconnected( edict_t *pClient )
{
  ASSERT(NULL != pClient);
  if ( pClient )
  {
    CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
    ASSERT(NULL != pPlayer);
    if (pPlayer)
    {
      ASSERT(NULL != pPlayer->pev);
      if (!pPlayer->pev)
        return;

      UTIL_LogPrintf("\"%s<%d><%s><%s>\" disconnected (score \"%.0f\")\n", 
                     pPlayer->GetName(), GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ), pPlayer->TeamID(), 
                     pPlayer->pev->frags );
      
      // Tell all clients this player isn't a spectator anymore
      MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );  
        WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
        WRITE_BYTE( 0 );
      MESSAGE_END();

      for ( int i = 1; i <= gpGlobals->maxClients; i++ )
      {
        CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
        if (pPlayerLoop && pPlayer != pPlayerLoop )
        {
          //Send score to others.
          ClientPrint(pPlayerLoop->pev, HUD_PRINTNOTIFY, UTIL_VarArgs("%s left with score %.0f\n",pPlayer->GetName(),pPlayer->pev->frags));

          if ((CBaseEntity*)pPlayerLoop->m_hSpectateTarget == pPlayer)
          {
            //Move to next player.
            pPlayerLoop->Spectate_Nextplayer(false);
          }
        }
      }
      
      if (ARENA == AgGametype())
        m_Arena.ClientDisconnected(pPlayer);
      else if (LMS == AgGametype())
        m_LMS.ClientDisconnected(pPlayer);
      else if (CTF == AgGametype())
        m_CTF.ClientDisconnected(pPlayer);
      //++ muphicks
      else if (DOM == AgGametype())
        m_DOM.ClientDisconnected(pPlayer);
      //-- muphicks
    }
  }
}



int AgGameRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
  ASSERT(NULL != pAttacker);
  ASSERT(NULL != pKilled);
  if (!pAttacker || !pKilled)
    return 1;
  
  if (ARCADE == AgGametype() || INSTAGIB == AgGametype())
  {
    if (pAttacker && pKilled && pAttacker != pKilled && pAttacker->IsAlive() && pAttacker->pev)
    {
      //As a reward for your kill you get full health and ammo.
      pAttacker->pev->armorvalue = MAX_NORMAL_BATTERY;
      pAttacker->pev->health = MAX_NORMAL_BATTERY;
      
      //Fill clip in all weapons weapon
      for (int i = 0 ; i < MAX_ITEM_TYPES; i++)
      {
        if (pAttacker->m_rgpPlayerItems[i])
        {
          CBasePlayerItem* pPlayerItem = pAttacker->m_rgpPlayerItems[i];
          while (pPlayerItem)
          {
            CBasePlayerWeapon* pWeapon = (CBasePlayerWeapon*)pPlayerItem->GetWeaponPtr();
            if (pWeapon)
              pWeapon->m_iClip = pWeapon->iMaxClip();
            pPlayerItem = pPlayerItem->m_pNext;
          }
        }
      }
      
      if (0 < ag_start_hgrenade.value)
        pAttacker->GiveAmmo( ag_start_hgrenade.value, "Hand Grenade", HANDGRENADE_MAX_CARRY );
      if (0 < ag_start_satchel.value)
        pAttacker->GiveAmmo( ag_start_satchel.value, "Satchel Charge", SATCHEL_MAX_CARRY );
      if (0 < ag_start_tripmine.value)
        pAttacker->GiveAmmo( ag_start_tripmine.value, "Trip Mine", TRIPMINE_MAX_CARRY );
      if (0 < ag_start_snark.value)
        pAttacker->GiveAmmo( ag_start_snark.value, "Snarks", SNARK_MAX_CARRY );
      if (0 < ag_start_hornet.value)
        pAttacker->GiveAmmo( ag_start_hornet.value, "Hornets", HORNET_MAX_CARRY );
      if (0 < ag_start_m203.value)
        pAttacker->GiveAmmo( ag_start_m203.value, "ARgrenades", M203_GRENADE_MAX_CARRY );
      if (0 < ag_start_uranium.value)
        pAttacker->GiveAmmo( ag_start_uranium.value, "uranium", URANIUM_MAX_CARRY );
      if (0 < ag_start_9mmar.value)
        pAttacker->GiveAmmo( ag_start_9mmar.value, "9mm", _9MM_MAX_CARRY );
      if (0 < ag_start_357ammo.value)
        pAttacker->GiveAmmo( ag_start_357ammo.value, "357", _357_MAX_CARRY );
      if (0 < ag_start_bockshot.value)
        pAttacker->GiveAmmo( ag_start_bockshot.value, "buckshot", BUCKSHOT_MAX_CARRY );
      if (0 < ag_start_bolts.value)
        pAttacker->GiveAmmo( ag_start_bolts.value, "bolts", BOLT_MAX_CARRY );
      if (0 < ag_start_rockets.value)
        pAttacker->GiveAmmo( ag_start_rockets.value, "rockets", ROCKET_MAX_CARRY );
    }
  }
  
  return 1;
}

void AgGameRules::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor)
{
  if (!pVictim || !pKiller || !pInflictor)
    return;
  
  CBaseEntity* pKillerBE = CBaseEntity::Instance(pKiller);
  CBasePlayer* pKillerPlayer = NULL;
  if (pKillerBE && CLASS_PLAYER == pKillerBE->Classify())
    pKillerPlayer = ((CBasePlayer*)pKillerBE);

#ifdef AGSTATS
  if (pKillerPlayer && pVictim)
    Stats.PlayerKilled(pKillerPlayer, pVictim);
#endif

  //Update the score cache.
  m_ScoreCache.UpdateScore(pVictim);
  if (pKillerPlayer && pVictim != pKillerPlayer )
    m_ScoreCache.UpdateScore(pKillerPlayer);
}


struct BANWEAPON
{
  char szWeapon[20];
  cvar_t* pBan;
};
FILE_GLOBAL BANWEAPON s_Bans[] = 
{
  "weapon_crowbar",&ag_ban_crowbar,
  "weapon_9mmhandgun",&ag_ban_glock,
  "weapon_shotgun",&ag_ban_357,
  "weapon_9mmAR",&ag_ban_mp5,
  "weapon_shotgun",&ag_ban_shotgun,
  "weapon_crossbow",&ag_ban_crossbow,
  "weapon_357",&ag_ban_357,
  "weapon_rpg",&ag_ban_rpg,
  "weapon_gauss",&ag_ban_gauss,
  "weapon_egon",&ag_ban_egon,
  "weapon_hornetgun",&ag_ban_hornet,
  "weapon_handgrenade",&ag_ban_hgrenade,
  "weapon_satchel",&ag_ban_satchel,
  "weapon_tripmine",&ag_ban_tripmine,
  "weapon_snark",&ag_ban_snark,
  "item_longjump",&ag_ban_longjump,
  "ammo_ARgrenades",&ag_ban_m203,
  "ammo_9mmclip",&ag_ban_9mmar,
  "ammo_9mmAR",&ag_ban_9mmar,
  "ammo_mp5clip",&ag_ban_9mmar,
  "ammo_9mmbox",&ag_ban_9mmar,
  "ammo_buckshot",&ag_ban_bockshot,
  "ammo_gaussclip",&ag_ban_uranium,
  "ammo_crossbow",&ag_ban_bolts,
  "ammo_rpgclip",&ag_ban_rockets,
  "ammo_357",&ag_ban_357ammo,
  "item_battery",&ag_ban_armour,
  "item_healthkit",&ag_ban_health,
};


BOOL AgGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem )
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return FALSE;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return FALSE;
  ASSERT(NULL != pItem);
  if (!pItem)
    return FALSE;
  if (!pPlayer->IsAlive())
    return FALSE;
  
  if (ARENA == AgGametype() && m_Arena.CanHaveItem())
    return TRUE;

  const char* pszClass = STRING(pItem->pev->classname);
  if (0 == strncmp( pszClass, "weapon_", 7 ) || 
    0 == strncmp( pszClass, "ammo_", 5 ) || 
    0 == strncmp( pszClass, "item_", 5 ) )
  {
    int iCount = sizeof(s_Bans)/sizeof(s_Bans[0]);
    for (int i = 0; i < iCount; i++)
    {
      if (FStrEq(pszClass,s_Bans[i].szWeapon))
      {
        if (1 > s_Bans[i].pBan->value)
          return CGameRules::CanHavePlayerItem(pPlayer,pItem);
        else
          return FALSE;
      }
    }
  }
  
  return CGameRules::CanHavePlayerItem(pPlayer,pItem);
}

BOOL AgGameRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return FALSE;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return FALSE;
  ASSERT(NULL != pItem);
  if (!pItem)
    return FALSE;
  if (!pPlayer->IsAlive())
    return FALSE;
  
  if (ARENA == AgGametype() && m_Arena.CanHaveItem())
    return TRUE;

  const char* pszClass = STRING(pItem->pev->classname);
  if (0 == strncmp( pszClass, "item_", 5 ) )
  {
    int iCount = sizeof(s_Bans)/sizeof(s_Bans[0]);
    for (int i = 0; i < iCount; i++)
    {
      if (FStrEq(pszClass,s_Bans[i].szWeapon))
        return 1 > s_Bans[i].pBan->value;
    }
  }
  return TRUE;
}

BOOL AgGameRules::IsAllowedToSpawn( const char* pszClass )
{
  //Check if item is banned.
  if (0 == strncmp( pszClass, "weapon_", 7 ) || 
    0 == strncmp( pszClass, "ammo_", 5 ) || 
    0 == strncmp( pszClass, "item_", 5 ) )
  {
    int iCount = sizeof(s_Bans)/sizeof(s_Bans[0]);
    for (int i = 0; i < iCount; i++)
    {
      if (FStrEq(pszClass,s_Bans[i].szWeapon))
        return 1 > s_Bans[i].pBan->value;
    }
  }
  
  return TRUE;
}

BOOL AgGameRules::IsAllowedToSpawn( CBaseEntity *pEntity )
{
  ASSERT(NULL != pEntity);
  if (!pEntity)
    return FALSE;

  if (ARENA == AgGametype() && m_Arena.CanHaveItem())
    return TRUE;
 
  return IsAllowedToSpawn(STRING(pEntity->pev->classname));
}


void AgGameRules::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
}

void AgGameRules::ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer )
{
#ifdef AG_USE_CHEATPROTECTION
  const char* pszModel = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" );
  if (pszModel && strlen(pszModel) && 0 != strcmp(pPlayer->m_szModel,pszModel))
  {
		strncpy(pPlayer->m_szModel, pszModel, sizeof(pPlayer->m_szModel) - 1);
		pPlayer->m_szModel[sizeof(pPlayer->m_szModel) - 1] = '\0';

    MESSAGE_BEGIN( MSG_ALL, gmsgSpikeCheck);
		  WRITE_STRING(pszModel);  
	  MESSAGE_END();
  }
#endif //AG_USE_CHEATPROTECTION

  char* pszAutoWepSwitch = g_engfuncs.pfnInfoKeyValue( infobuffer, "cl_autowepswitch");
  if (strlen(pszAutoWepSwitch))
    pPlayer->m_iAutoWepSwitch = atoi(pszAutoWepSwitch);

  char* pszDisableSpecs = g_engfuncs.pfnInfoKeyValue( infobuffer, "cl_disablespecs");
  if (strlen(pszDisableSpecs))
    pPlayer->m_iDisableSpecs = atoi(pszDisableSpecs);
	
  /*
	char* pszWeaponWeights = g_engfuncs.pfnInfoKeyValue( infobuffer, "cl_weaponweights" );
	if (strlen(pszWeaponWeights))
    pPlayer->SetWeaponWeights(pszWeaponWeights);
  */
}

void AgGameRules::InitHUD( CBasePlayer *pPlayer )
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;
  
  //Send server name.
  MESSAGE_BEGIN( MSG_ONE, gmsgServerName, NULL, pPlayer->edict() );
    WRITE_STRING( m_sHostname.substr(0,30).c_str() );
  MESSAGE_END();

#ifndef AG_NO_CLIENT_DLL
  MESSAGE_BEGIN( MSG_ONE, gmsgGametype, NULL, pPlayer->edict() );
    WRITE_BYTE( AgGametype());
  MESSAGE_END();  

  MESSAGE_BEGIN( MSG_ALL, gmsgAuthID);
    WRITE_BYTE( pPlayer->entindex() );
		WRITE_STRING(pPlayer->GetAuthID());  
	MESSAGE_END();
#endif

#ifdef AG_USE_CHEATPROTECTION
  const char* pszModel = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model" );
  if (pszModel && strlen(pszModel))
  {
    MESSAGE_BEGIN( MSG_ALL, gmsgSpikeCheck);
		  WRITE_STRING(pszModel);  
	  MESSAGE_END();
  }
#endif //AG_USE_CHEATPROTECTION
}

void AgGameRules::GoToIntermission()
{
  //Log scores
  if (ARENA != AgGametype() && LMS != AgGametype())
  {
    m_ScoreLog.End();
    CVAR_SET_FLOAT("sv_ag_match_running",0);
    CVAR_SET_FLOAT("sv_ag_show_gibs",1);
    CVAR_SET_FLOAT("ag_spectalk",1);
  }
}

BOOL AgGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return FALSE;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return FALSE;

  if (!g_bPaused && INSTAGIB == AgGametype())
  {
    ASSERT(NULL != pAttacker);
    if (!pAttacker)
      return FALSE;
    ASSERT(NULL != pAttacker->pev);
    if (!pAttacker->pev)
      return FALSE;

    if (CLASS_PLAYER == pAttacker->Classify())
    {
      //This dude is instant dead!
      pPlayer->pev->health = -200;
      pPlayer->Killed(pAttacker->pev, GIB_NEVER);
      return FALSE;
    }
  }

  if (ARENA == AgGametype())
    return (m_Arena.CanTakeDamage() && !g_bPaused);
  else if (LMS == AgGametype())
    return (m_LMS.CanTakeDamage() && !g_bPaused);

  return !g_bPaused;
}

void AgGameRules::ChangeNextLevel()
{
  m_Settings.ChangeNextLevel();
}

void AgGameRules::ResendScoreBoard()
{
  if (g_pGameRules->IsTeamplay())
  {
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
      CBasePlayer* plr = AgPlayerByIndex( i );
      if ( plr )
      {
        //Team
        if ( plr && g_pGameRules->IsValidTeam( plr->TeamID() ) )
        {
          MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
	          WRITE_BYTE( plr->entindex() );
            WRITE_STRING( plr->TeamID() );
          MESSAGE_END();
        }
      }
    }
  }
  
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* plr = AgPlayerByIndex( i );
    if ( plr )
    {
      //Score
      MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
        WRITE_BYTE( ENTINDEX(plr->edict()) );
        WRITE_SHORT( plr->pev->frags );
        WRITE_SHORT( plr->m_iDeaths );
        //++ BulliT
 		    WRITE_SHORT( g_teamplay );
		    WRITE_SHORT( g_pGameRules->GetTeamIndex( plr->m_szTeamName ) + 1 );
       //-- Martin Webrant
      MESSAGE_END();

      //Spectator
      MESSAGE_BEGIN(MSG_ALL,gmsgSpectator);
        WRITE_BYTE(ENTINDEX(plr->edict()));
        WRITE_BYTE(plr->IsSpectator() ? 1 : 0);
      MESSAGE_END();
    }
  }
}

void AgGameRules::RefreshSkillData( void )
{
  // load all default values
	CGameRules::RefreshSkillData();

// override some values for multiplay.
	// suitcharger
	gSkillData.suitchargerCapacity = 30;

  // Crowbar whack
	gSkillData.plrDmgCrowbar = ag_dmg_crowbar.value;

	// Glock Round
	gSkillData.plrDmg9MM = ag_dmg_glock.value;

	// 357 Round
	gSkillData.plrDmg357 = ag_dmg_357.value;

	// MP5 Round
	gSkillData.plrDmgMP5 = ag_dmg_mp5.value;

	// M203 grenade
	gSkillData.plrDmgM203Grenade = ag_dmg_m203.value;

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot =  ag_dmg_shotgun.value;

	// Crossbow
	gSkillData.plrDmgCrossbowClient = ag_dmg_crossbow.value;
  gSkillData.plrDmgCrossbowMonster = ag_dmg_bolts.value;
  
	// RPG
	gSkillData.plrDmgRPG = ag_dmg_rpg.value;

	// Egon
	gSkillData.plrDmgEgonWide = ag_dmg_egon_wide.value;
	gSkillData.plrDmgEgonNarrow = ag_dmg_egon_narrow.value;

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = ag_dmg_hgrenade.value;

	// Satchel Charge
	gSkillData.plrDmgSatchel = ag_dmg_satchel.value;

	// Tripmine
	gSkillData.plrDmgTripmine = ag_dmg_tripmine.value;

	// hornet
	gSkillData.plrDmgHornet = ag_dmg_hornet.value;

  // Head
  gSkillData.plrHead = ag_headshot.value;
}


void AgGameRules::HLTV_ResendScoreBoard()
{
  if (!m_bProxyConnected)
    return;

  if (g_pGameRules->IsTeamplay())
  {
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
      CBasePlayer* plr = AgPlayerByIndex( i );
      if ( plr )
      {
        //Team
        if ( plr && g_pGameRules->IsValidTeam( plr->TeamID() ) )
        {
          MESSAGE_BEGIN( MSG_SPEC, gmsgTeamInfo );
	          WRITE_BYTE( plr->entindex() );
            WRITE_STRING( plr->TeamID() );
          MESSAGE_END();
        }
      }
    }
  }
  
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* plr = AgPlayerByIndex( i );
    if ( plr )
    {
      //Score
      MESSAGE_BEGIN( MSG_SPEC, gmsgScoreInfo );
        WRITE_BYTE( ENTINDEX(plr->edict()) );
        WRITE_SHORT( plr->pev->frags );
        WRITE_SHORT( plr->m_iDeaths );
        //++ BulliT
 		    WRITE_SHORT( g_teamplay );
		    WRITE_SHORT( g_pGameRules->GetTeamIndex( plr->m_szTeamName ) + 1 );
        //-- Martin Webrant
      MESSAGE_END();

      //Spectator
      MESSAGE_BEGIN(MSG_SPEC,gmsgSpectator);
        WRITE_BYTE(ENTINDEX(plr->edict()));
        WRITE_BYTE(plr->IsSpectator() ? 1 : 0);
      MESSAGE_END();
    }
  }

  MESSAGE_BEGIN(MSG_SPEC,gmsgGameMode);
    WRITE_BYTE(g_teamplay);
  MESSAGE_END();
}

/*
class AgTeam
{
public:
  AgTeam(const char* pszTeam)
  {
    m_strName = pszTeam;
    m_iCount = 0;
  }

  AgString m_strName;

  int m_iCount;
};

struct lessagteam : binary_function<AgTeam, AgTeam, bool> {
    bool operator()(const AgTeam& x, const AgTeam& y) const 
    { 
      if (x.m_strName.compare(y.m_strName) < 0)
          return true;
      else
          return false;
    }
};

AgString CHalfLifeTeamplay::GetTeamWithFewestPlayers()
{
//++ BulliT
  if (CTF == AgGametype())
  {
    if (GetTeamIndex(CTF_TEAM1_NAME) == -1)
      return CTF_TEAM1_NAME;
    else if (GetTeamIndex(CTF_TEAM2_NAME) == -1)
      return CTF_TEAM2_NAME; 
  }
//-- Martin Webrant

	char	*pName;
	char	szTeamList[TEAMPLAY_TEAMLISTLENGTH];
  typedef set<AgTeam, lessagteam > AgTeamSet;

  AgTeamSet setTeams;

	// Copy all of the teams from the teamlist
	// make a copy because strtok is destructive
	strcpy( szTeamList, m_szTeamList );
	pName = szTeamList;
	pName = strtok( pName, ";" );
	while ( pName != NULL && *pName )
	{
    setTeams.insert(AgTeam(pName));
		pName = strtok( NULL, ";" );
	}

	int i;
	int minPlayers = MAX_TEAMS;
	int teamCount[ MAX_TEAMS ];
	char *pTeamName = NULL;

	memset( teamCount, 0, MAX_TEAMS * sizeof(int) );
	
	// loop through all clients, count number of players on each team
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop)
    {
      AgTeamSet::iterator itrTeams = setTeams.find(pPlayerLoop->m_szTeamName);
      (*itrTeams).m_iCount++;
		}
	}

  AgTeamSet::iterator itrTeams;
  int iMinPlayers = MAX_TEAMS;
  AgString strTeam;
  for (itrTeams = setTeams.begin() ;itrTeams != setTeams.end(); ++itrTeams)
  {
    if ((*itrTeams).m_iCount < iMinPlayers)
    {
      iMinPlayers = (*itrTeams).m_iCount;
      strTeam = (*itrTeams).m_strName;
    }
  }

	return strTeam;
}
*/

#define MAX_MAPLIST_CHUNK	  190

AgString g_sMapList;
void AgGameRules::SendMapListToClient(CBasePlayer* pPlayer, bool bStart)
{
  if (-1 == pPlayer->m_iMapListSent)
  {
    if (bStart)
      pPlayer->m_iMapListSent = 0;
    else
      return;
  }
    
  if (0 == g_sMapList.size())
  {
    char	szDirAG[MAX_PATH];
    char	szDirVALVE[MAX_PATH];
	sprintf(szDirAG, "%s/maps", AgGetDirectory());
	sprintf(szDirVALVE, "%s/maps", AgGetDirectoryValve());

    AgStringSet setFiles;
    AgStringSet::iterator itrFiles;

    AgDirList(szDirAG,setFiles);
    AgDirList(szDirVALVE,setFiles);

    for (itrFiles = setFiles.begin() ;itrFiles != setFiles.end(); ++itrFiles)
    {
      AgString sFile = *itrFiles;
      AgToLower(sFile);
      if (!strstr(sFile.c_str(),".bsp"))
        continue;
      sFile = sFile.substr(0,sFile.size()-4);
      AgTrim(sFile);
      g_sMapList += sFile + "#";
    }

	}

  //Send it over in chunks.
  int iDataToSend = min((int)g_sMapList.size()-pPlayer->m_iMapListSent,MAX_MAPLIST_CHUNK);
  AgString sChunk = g_sMapList.substr(pPlayer->m_iMapListSent,iDataToSend);
  pPlayer->m_iMapListSent += iDataToSend;
	MESSAGE_BEGIN( MSG_ONE, gmsgMapList, NULL, pPlayer->edict() );
		WRITE_BYTE( pPlayer->m_iMapListSent < (int)g_sMapList.size() );
		WRITE_STRING( sChunk.c_str() );
	MESSAGE_END();

  if (pPlayer->m_iMapListSent >= (int)g_sMapList.size())
    pPlayer->m_iMapListSent = -1;
}


//-- Martin Webrant

