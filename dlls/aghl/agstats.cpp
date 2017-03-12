//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "agglobal.h"
#include "agstats.h"
#include "weapons.h"
#include "monsters.h"

DLL_GLOBAL AgStats Stats;

AgStats::AgStats()
{
}

AgStats::~AgStats()
{
	Reset();
}

void AgStats::Reset()
{
  for (AgPlayerStatsMap::iterator itrPlayerStats = m_mapPlayerStats.begin() ;itrPlayerStats != m_mapPlayerStats.end(); ++itrPlayerStats)
    delete (*itrPlayerStats).second;

  m_mapPlayerStats.clear();
}

bool AgStats::HandleCommand(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return false;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return false;
  ASSERT(NULL != g_pGameRules);
  
  if (!g_pGameRules || 0 == CMD_ARGC())
    return false;

  if (FStrEq(CMD_ARGV(0), "agstats"))
  {
    CBasePlayer* pStatsPlayer = pPlayer;
    if (CMD_ARGC() == 2)
      pStatsPlayer = AgPlayerByName(CMD_ARGV(1));

    if (pStatsPlayer)
      PrintStats(pPlayer,pStatsPlayer);
	  return true;
  }

  return false;
}

void AgStats::PrintStats(CBasePlayer* pPlayer, CBasePlayer* pStatsPlayer)
{
	  AgPlayerStats* pStats = GetStats(pStatsPlayer);

	  AgString sStats;
	  char szTempBuff[1024];
	  static char s_szPlayerStats[] = "Kills %d, Deaths %d, Team Kills %d, Damage ratio %d";
    sprintf(szTempBuff,s_szPlayerStats,pStats->m_iKills,pStats->m_iDeaths,pStats->m_iTeamKills,(int)(pStats->m_iDamageTaken ? ((float)pStats->m_iDamageGiven / (float)pStats->m_iDamageTaken * 100) : 0));
    AgConsole(szTempBuff,pPlayer);

	  for (AgPlayerStats::AgWeaponStatsMap::iterator itrWeaponStats = pStats->m_mapWeaponStats.begin() ;itrWeaponStats != pStats->m_mapWeaponStats.end(); ++itrWeaponStats)
	  {
      AgPlayerStats::AgWeaponStats* pWeaponStats = (*itrWeaponStats).second;
      static char s_szWeaponStats[] = "%s, Shots %d, Hits %d, Accuracy %d";
      int iHits = pWeaponStats->m_iHits;
      if ((*itrWeaponStats).first == "weapon_shotgun")
        iHits /= 4;
      sprintf(szTempBuff,s_szWeaponStats,(*itrWeaponStats).first.c_str(), pWeaponStats->m_iShots, iHits, (int)(pWeaponStats->m_iShots ? ((float)iHits / (float)pWeaponStats->m_iShots * 100) : 0));
      AgConsole(szTempBuff,pPlayer);
    }
}

void AgStats::OnChangeLevel()
{
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop && !pPlayerLoop->IsProxy())
      PrintStats(pPlayerLoop,pPlayerLoop);
  }
}

AgStats::AgPlayerStats* AgStats::GetStats(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return NULL;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return NULL;

  if (g_bLangame)
  {
    for (AgPlayerStatsMap::iterator itrPlayerStats = m_mapPlayerStats.begin() ;itrPlayerStats != m_mapPlayerStats.end(); ++itrPlayerStats)
    {
      if (pPlayer->GetName() == (*itrPlayerStats).second->m_sName)
        return (*itrPlayerStats).second;
    }
  }
  else
  {
    //Search for auth id.
    AgPlayerStatsMap::iterator itrPlayerStats = m_mapPlayerStats.find(pPlayer->GetAuthID());
    if (itrPlayerStats != m_mapPlayerStats.end())
    {
	    (*itrPlayerStats).second->m_sName = pPlayer->GetName();	//Update name...
      return (*itrPlayerStats).second;
    }
  }


  AgPlayerStats* pPlayerStats = new AgPlayerStats();
  pPlayerStats->m_sName = pPlayer->GetName();
  if (!g_bLangame)
    m_mapPlayerStats.insert(AgPlayerStatsMap::value_type(pPlayer->GetAuthID(),pPlayerStats));
  else
    m_mapPlayerStats.insert(AgPlayerStatsMap::value_type(pPlayer->GetName(),pPlayerStats));

  return pPlayerStats;
}

AgStats::AgPlayerStats::AgWeaponStats* AgStats::AgPlayerStats::GetWeaponStats(const char* pszItem)
{
  AgWeaponStatsMap::iterator itrWeaponStats = m_mapWeaponStats.find(pszItem);
  if (itrWeaponStats != m_mapWeaponStats.end())
    return (*itrWeaponStats).second;

  AgWeaponStats* pWeaponStats = new AgWeaponStats();
  m_mapWeaponStats.insert(AgWeaponStatsMap::value_type(pszItem,pWeaponStats));
  return pWeaponStats;
}

void AgStats::FireShot(CBasePlayer* pPlayer, const char* pszItem)
{
	GetStats(pPlayer)->GetWeaponStats(pszItem)->m_iShots++;
}

void AgStats::FireHit(CBasePlayer* pPlayer, int iDamage, entvars_t* pevAttacker)
{
  CBasePlayer* pInflictor = NULL;
  AgString strItem = STRING(pevAttacker->classname);

  if (strItem == "player")
  {
    strItem = "";
    pInflictor = (CBasePlayer*)CBaseEntity::Instance(pevAttacker);
    if (pInflictor && pInflictor->m_pActiveItem)
      strItem = CBasePlayerItem::ItemInfoArray[pInflictor->m_pActiveItem->m_iId].pszName;
  }
  else if (strItem == "rpg_rocket")
  {
    pInflictor = (CBasePlayer*)CBaseEntity::Instance(pevAttacker->owner);
    strItem = "weapon_rpg";
  }
  /*
  else if (strItem == "bolt")
  {
    pInflictor = (CBasePlayer*)CBaseEntity::Instance(pevAttacker->owner);
    strItem = "weapon_crossbow";
  }
  */
  else if (strItem == "hornet")
  {
    pInflictor = (CBasePlayer*)CBaseEntity::Instance(pevAttacker->owner);
    strItem = "weapon_hornet";
  }
  else if (strItem == "grenade")
  {
    strItem = "";
  }
  /*
//  if (pInflictor == pTarget)
  //  return; //heh count self hit as a miss.

  AgString strItem;
  if (0 == strncmp(pszItem, "weapon_", 7 ))
    strItem = pszItem;
  else if (0 == strcmp(pszItem,"rpg_rocket"))
    strItem = "weapon_rpg";
  else if (0 == strcmp(pszItem,"hornet"))
    strItem = "weapon_hornet";
  else if (0 == strcmp(pszItem,"bolt"))
    strItem = "weapon_crossbow"; 
  else if (0 == strcmp(pszItem,"grenade"))
	  strItem = "";
  else if (0 == strcmp(pszItem,"player") && pInflictor->m_pActiveItem)
    strItem = CBasePlayerItem::ItemInfoArray[pInflictor->m_pActiveItem->m_iId].pszName ;
    */

  if (strItem.size() && pInflictor && pInflictor != pPlayer)
  {
    AgPlayerStats* pPlayerStats = GetStats(pInflictor);
	  AgPlayerStats::AgWeaponStats* pWeaponStats = pPlayerStats->GetWeaponStats(strItem.c_str());
    pPlayerStats->m_iDamageGiven += iDamage;
	  pWeaponStats->m_iHits++;
	  GetStats(pPlayer)->m_iDamageTaken += iDamage;
  }
}

void AgStats::PlayerKilled(CBasePlayer* pInflictor, CBasePlayer* pKilled)
{
	if (GR_TEAMMATE == g_pGameRules->PlayerRelationship(pInflictor, pKilled))
		GetStats(pInflictor)->m_iTeamKills++;
	else
		GetStats(pInflictor)->m_iKills++;
	GetStats(pKilled)->m_iDeaths++;
}

//-- Martin Webrant
