//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "../engine/shake.h"
#include "agglobal.h"
#include "gamerules.h"

extern DLL_GLOBAL g_fGameOver;

//TITLES FOR HALF-LIFE
// Position command $position x y 
// x & y are from 0 to 1 to be screen resolution independent
// -1 means center in each dimension
// Effect command $effect <effect number>
// effect 0 is fade in/fade out
// effect 1 is flickery credits
// effect 2 is write out (training room)
// Text color r g b command $color
// fadein time fadeout time / hold time
// $fadein (message fade in time - per character in effect 2)
// $fadeout (message fade out time)
// $holdtime (stay on the screen for this long)
void AgSay(CBasePlayer* pPlayer,const AgString& sText, float* pfFloodProtected, float fHoldTime, float x, float y, int iChannel)
{
  if (g_fGameOver)
    return;         
  
  if (pfFloodProtected)
  {
    if (*pfFloodProtected > gpGlobals->time)
      return;
    
    *pfFloodProtected = gpGlobals->time + fHoldTime;
  }
  

  hudtextparms_t     hText;
  memset(&hText, 0, sizeof(hText));
  hText.channel = iChannel;
  // These X and Y coordinates are just above
  //  the health meter.
  hText.x = x;
  hText.y = y;
  
  hText.r1 = hText.g1 = hText.b1 = 180;
  hText.a1 = 0;
  
  hText.r2 = hText.g2 = hText.b2 = 0;
  hText.a2 = 0;
  
  hText.holdTime = fHoldTime - 0.30;
  
  hText.effect = 2;    // Fade in/out
  hText.fadeinTime = 0.01;
  hText.fadeoutTime = fHoldTime/5;
  hText.fxTime = 0.25;
  
  if (pPlayer)
  {
    UTIL_HudMessage(pPlayer,hText, sText.c_str());
  }
  else
  {
    for (int i = 1; i <= gpGlobals->maxClients; i++)
    {
      CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
      if (pPlayerLoop)
        UTIL_HudMessage(pPlayerLoop,hText, sText.c_str());
    }
  }
}

void AgConsole(const AgString& sText, CBasePlayer* pPlayer)
{
  if (pPlayer && pPlayer->pev)
  {
    ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, UTIL_VarArgs("%s\n", sText.c_str()));
    ClientPrint(pPlayer->pev, HUD_PRINTCENTER , UTIL_VarArgs("%s\n", sText.c_str()));
  }
  else
  {
    g_engfuncs.pfnServerPrint(UTIL_VarArgs("%s\n", sText.c_str()));
  }
}

CBasePlayer* AgPlayerByIndex(int iPlayerIndex )
{
  CBasePlayer* pPlayer = NULL;
  
  if ( iPlayerIndex > 0 && iPlayerIndex <= gpGlobals->maxClients )
  {
    edict_t *pPlayerEdict = INDEXENT( iPlayerIndex );
    if ( pPlayerEdict && !pPlayerEdict->free && pPlayerEdict->pvPrivateData)
    {
      CBaseEntity* pEnt = (CBaseEntity *)CBaseEntity::Instance( pPlayerEdict );
      if (pEnt && pEnt->pev && CLASS_PLAYER == pEnt->Classify())
      {
        if (pEnt->pev->netname && 0 != STRING(pEnt->pev->netname)[0])
        {
          pPlayer = (CBasePlayer*)pEnt;
        }
      }
    }
  }
  
  return pPlayer;
}

void AgResetMap()
{
  CBaseEntity* pEntity = NULL;
  
  edict_t* pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
  for ( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
  {	
    pEntity = CBaseEntity::Instance( pEdict );
    if (pEntity && pEntity->pev)
    {
      const char* pszClass = STRING(pEntity->pev->classname);
    
      if (pszClass && '\0' != pszClass[0])
      {
        if (0 == strncmp( pszClass, "weapon_", 7 ) || 
            0 == strncmp( pszClass, "ammo_", 5 ) || 
            0 == strncmp( pszClass, "item_", 5 ) )
        {
          pEntity->pev->nextthink = gpGlobals->time;
        }
      }
    }
  }
  
  pEntity = NULL;	
  while ((pEntity = UTIL_FindEntityByClassname( pEntity, "weaponbox" )) != NULL)
    UTIL_Remove(pEntity);
  
  pEntity = NULL;	
  while ((pEntity = UTIL_FindEntityByClassname( pEntity, "monster_satchel" )) != NULL)
    UTIL_Remove(pEntity);
  
  pEntity = NULL;	
  while ((pEntity = UTIL_FindEntityByClassname( pEntity, "monster_tripmine" )) != NULL)
    UTIL_Remove(pEntity);

  pEntity = NULL;	
  while ((pEntity = UTIL_FindEntityByClassname( pEntity, "monster_snark" )) != NULL)
    UTIL_Remove(pEntity);
  
  pEntity = NULL;	
  while ((pEntity = UTIL_FindEntityByClassname( pEntity, "beam" )) != NULL)
    UTIL_Remove(pEntity);

  pEntity = NULL;	
  while ((pEntity = UTIL_FindEntityByClassname( pEntity, "rpg_rocket" )) != NULL)
    pEntity->pev->dmg = 0;

  pEntity = NULL;	
  while ((pEntity = UTIL_FindEntityByClassname( pEntity, "grenade" )) != NULL)
    pEntity->pev->dmg = 0;
}


#define ADD_SERVER_COMMAND	( *g_engfuncs.pfnAddServerCommand )

void  AgStart(void)
{
  //Loop through all active players, reset Score and respawn.
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop)
    {
      if (pPlayerLoop->IsSpectator())
      {
        //Regular spectators aint spawned when match is restarted.
        pPlayerLoop->ResetScore();      //Reset the score.
        continue;
      }
      else
      {
        pPlayerLoop->ResetScore();      //Reset the score.
        pPlayerLoop->RespawnMatch();    //Now spawn the sucker :-)
      }
    }
  }
  g_pGameRules->m_fGameStart = gpGlobals->time;
  AgResetMap();
}

DLL_GLOBAL cvar_t	ag_version  = {"ag_version","0.1", FCVAR_SERVER }; 
DLL_GLOBAL cvar_t	ag_url      = {"ag_url","www.planethalflife.com/agmod", FCVAR_SERVER }; 

void AgInitGame()
{
  CVAR_REGISTER(&ag_version);
  CVAR_REGISTER(&ag_url);
  ADD_SERVER_COMMAND("agstart",AgStart);
}

//-- Martin Webrant
