//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "game.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"

#include "aggamerules.h"
#include "agclient.h"

#include "voice_gamemgr.h"
extern CVoiceGameMgr g_VoiceGameMgr;

#ifdef AGMSGSTAT
#include "agmsgstat.h"
#endif 

extern cvar_t timeleft, fragsleft;
extern int gmsgSayText;
extern int gmsgLocation;
extern int gmsgAuthID;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern int gmsgPlaySound;
extern int gmsgTeamInfo;
extern int g_teamplay;

AgClient::AgClient()
{
  
}

AgClient::~AgClient()
{
}

FILE_GLOBAL char* s_szCommands[] =
{
  "hud_spectatebar <1,0> - Spectator bar with information.",
  "hud_timer <0-3> - Timer mode, 0 off, 1 = Time remaining, 2 = Effective time, 3 clock.",
  "hud_playerid <1,0> - Player id.",
  "hud_settings <1,0> - Show server settings.",
  "cl_matchreport <1,0> - Save matchreport in /report directory.",
  "cl_autowepswitch <2,1,0> - Switch to better weapon when you walk over it. 2 = Ag switch",
  "cl_scores <0 - 32> - Amount of scores to draw on hud.",
  "cl_only_team_talk <1,0> - Only print team talk.",
  "auth <name> <pass> - Authenticate you as an admin.",
  "newpass <oldpass> <newpass> - Change password.",
  "settings - Shows server settings.",
  "timeleft - Shows time left.",
  "timeout - Call 1 minute timeout.",
  "play_team <wavefile> - Players wave file to m8's.",
  "play_close <wavefile> - Players wave file to m8's that are close.",
  "say enhancements - %%h health, %%a armour, %%q ammo, %%w weapon, %%l location", 
  "more say - %%p longjump status %%s your score",
  "say_close - Says a message to team m8's close to you.",
  "spectate - Leave/enter spectator mode.",
  "ready/notready - Set ready mode.",
  "winamp play/pause/stop/forward/back/next/prev/close/shuffle/repeat/volume 0-255",
  "dropitems - Drops flag in CTF mode",
};


bool AgClient::HandleCommand(CBasePlayer* pPlayer)
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

  if (FStrEq(CMD_ARGV(0), "_special"))
  {
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "say"))
  {
    Say(pPlayer, All);
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "say_team"))
  {
    Say(pPlayer, Team);
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "say_close"))
  {
    Say(pPlayer, Close);
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "spectate" ))
  {
	  if (pPlayer->IsProxy())
		  return pPlayer->Spectate_HLTV();

    //Dont spectate if player is in game in arena.
    //Use sound and text flood checks, We dont want lamers to change to often.
    if (pPlayer->IsIngame() && ARENA == AgGametype() || pPlayer->FloodCheck() || pPlayer->FloodSound())
      return true;

    if (2 == CMD_ARGC() && 1 == atoi(CMD_ARGV(1)))
      pPlayer->Spectate_Start();
    else if (2 == CMD_ARGC() && 0 == atoi(CMD_ARGV(1)))
      pPlayer->Spectate_Stop();
    else
      pPlayer->Spectate_Spectate();
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "follownext") && 2 == CMD_ARGC())
  {
    bool bReverse = 1 == atoi(CMD_ARGV(1));
    pPlayer->Spectate_Nextplayer(bReverse);
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "spec_mode") && 2 == CMD_ARGC())
  {
    int iSpecMode = atoi(CMD_ARGV(1));
    pPlayer->Spectate_SetMode(iSpecMode);
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "jointeam" ) && 2 == CMD_ARGC())
  {
	  if (pPlayer->IsProxy())
      return true;

    if (g_teamplay)
    {
      const char* pszTeamName = "";
      const char* pszTeamNumber = CMD_ARGV(1);
      int iTeam = atoi(pszTeamNumber)-1;
      if (5 == iTeam && g_teamplay)
      {
        if (!g_pGameRules->IsValidTeam(pPlayer->TeamID()))
        {
          ((CHalfLifeTeamplay*)g_pGameRules)->RecountTeams();
          pszTeamName = ((CHalfLifeTeamplay*)g_pGameRules)->TeamWithFewestPlayers();
        }          
      }
      else
        pszTeamName = g_pGameRules->GetIndexedTeamName(iTeam);
      if (strlen(pszTeamName))
        g_pGameRules->ChangePlayerTeam(pPlayer,pszTeamName,TRUE,TRUE);
    }

    if (pPlayer->IsSpectator())
      pPlayer->Spectate_Stop();
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "timeleft" ))
  {
    if (timelimit.value && timeleft.value)
      AgConsole(UTIL_VarArgs("timeleft = %.0f",timeleft.value),pPlayer);
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "timeout" ))
  {
    if (ARENA != AgGametype() || LMS != AgGametype())
      g_pGameRules->m_Timeout.Timeout(pPlayer);
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "ready" ))
  {	
    if (ARENA == AgGametype())
    {
      g_pGameRules->m_Arena.Ready(pPlayer);
      return true;
    }
    else if (LMS == AgGametype())
    {
      ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "Changed mode to READY.\n");
      if (!pPlayer->m_bReady)
      {
        pPlayer->m_bReady = true;
        //Send new team name 
        if (g_pGameRules->IsTeamplay())
        {
          MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
            WRITE_BYTE( pPlayer->entindex() );
            WRITE_STRING( pPlayer->TeamID() );
          MESSAGE_END();
        }
      }

      return true;
    }
  }
  else if (FStrEq(CMD_ARGV(0), "notready" ))
  {	
    if (ARENA == AgGametype())
    {
      g_pGameRules->m_Arena.NotReady(pPlayer);
      return true;
    }
    else if (LMS == AgGametype())
    {
      ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "Changed mode to NOT READY.\n");
      if (pPlayer->m_bReady)
      {
        pPlayer->m_bReady = false;
        pPlayer->SetIngame(false); //Cant respawn
        if(!pPlayer->IsSpectator())
        {
          pPlayer->Spectate_Start(true);
          pPlayer->Spectate_SetMode(OBS_IN_EYE);
        }
        else
        {
          //Send new team name (blank for specs)
          if (g_pGameRules->IsTeamplay())
          {
            MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
              WRITE_BYTE( pPlayer->entindex() );
              WRITE_STRING( pPlayer->TeamID() );
            MESSAGE_END();
          }
        }
      }
      
      return true;
    }
  }
  else if (FStrEq(CMD_ARGV(0), "play_team" ))
  {	
    if (2 == CMD_ARGC())
    {	
			Play(pPlayer, Team, CMD_ARGV(1));
    }
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "play_close" ))
  {	
    if (2 == CMD_ARGC())
    {	
			Play(pPlayer, Close, CMD_ARGV(1));
    }
    return true;
  }  else if (FStrEq(CMD_ARGV(0), "auth" ))
  {	
    if (3 == CMD_ARGC())
    {	
      AdminCache.Auth(CMD_ARGV(1), CMD_ARGV(2), pPlayer);
    }
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "newpass" ))
  {	
    if (3 == CMD_ARGC())
    {	
      AdminCache.Newpass(CMD_ARGV(1), CMD_ARGV(2), pPlayer);
    }
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "help"))
  {
    for (int i = 0; i < sizeof(s_szCommands)/sizeof(s_szCommands[0]); i++)
      AgConsole(s_szCommands[i],pPlayer);
    
    //Continue with more help commands.
    return false;
  }
  else if (FStrEq(CMD_ARGV(0), "settings"))
  {
    pPlayer->SetDisplayGamemode(0);
    return true;
  }
#ifndef AG_NO_CLIENT_DLL
  else if (FStrEq(CMD_ARGV(0), "dropitems") && CTF == AgGametype())
  {
    g_pGameRules->m_CTF.PlayerDropFlag(pPlayer,true);
    return true;
  }
#ifdef _DEBUG
  else if (FStrEq(CMD_ARGV(0), "addctfitem"))
#else
  else if (FStrEq(CMD_ARGV(0), "addctfitem") && g_bLangame)
#endif
  {	
    if (2 == CMD_ARGC())
    {
      g_pGameRules->m_CTF.m_FileItemCache.Add(CMD_ARGV(1), pPlayer);
    }
    return true;
  }
#ifdef _DEBUG
  else if (FStrEq(CMD_ARGV(0), "dellastctfitem"))
#else
  else if (FStrEq(CMD_ARGV(0), "dellastctfitem") && g_bLangame)
#endif
  {	
    if (1 == CMD_ARGC())
    {
      g_pGameRules->m_CTF.m_FileItemCache.Del(pPlayer);
    }
    return true;
  }
#ifdef _DEBUG
  else if (FStrEq(CMD_ARGV(0), "listctfitem"))
#else
  else if (FStrEq(CMD_ARGV(0), "listctfitem") && g_bLangame)
#endif
  {	
    if (1 == CMD_ARGC())
    {
      g_pGameRules->m_CTF.m_FileItemCache.List(pPlayer);
    }
    return true;
  }
//++ muphicks
#ifdef _DEBUG
  else if (FStrEq(CMD_ARGV(0), "adddomitem"))
#else
  else if (FStrEq(CMD_ARGV(0), "adddomitem") && g_bLangame)
#endif
  {	
    if (2 == CMD_ARGC())
    {
      g_pGameRules->m_DOM.m_FileItemCache.Add(CMD_ARGV(1), pPlayer);
    }
    return true;
  }
#ifdef _DEBUG
  else if (FStrEq(CMD_ARGV(0), "dellastdomitem"))
#else
  else if (FStrEq(CMD_ARGV(0), "dellastdomitem") && g_bLangame)
#endif
  {	
    if (1 == CMD_ARGC())
    {
      g_pGameRules->m_DOM.m_FileItemCache.Del(pPlayer);
    }
    return true;
  }
#ifdef _DEBUG
  else if (FStrEq(CMD_ARGV(0), "listdomitem"))
#else
  else if (FStrEq(CMD_ARGV(0), "listdomitem") && g_bLangame)
#endif
  {	
    if (1 == CMD_ARGC())
    {
      g_pGameRules->m_DOM.m_FileItemCache.List(pPlayer);
    }
    return true;
  }
//-- muphicks
#endif
  else if (FStrEq(CMD_ARGV(0), "changeteam"))
  {
    #define MENU_TEAM 					2
    pPlayer->ShowVGUI(MENU_TEAM);
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "fullupdate"))
  {
    pPlayer->ForceClientDllUpdate();
    return true;
  }
  else if (FStrEq(CMD_ARGV(0), "agosinfo"))
  {
    AgConsole(AgOSVersion(),pPlayer);
    return true;
  }
#ifndef AG_NO_CLIENT_DLL
  else if (FStrEq(CMD_ARGV(0), "maplist"))
  {
    g_pGameRules->SendMapListToClient(pPlayer,true);
    return true;
  }
#endif
#ifdef AGMSGSTAT
  else if (FStrEq(CMD_ARGV(0), "agmsgstat"))
  {
	  g_MsgStat.DumpStats();
    return true;
  }
#endif

  /*
  else if (FStrEq(CMD_ARGV(0), "botme"))
  {
    	edict_t* pEntity = g_engfuncs.pfnCreateFakeClient( "botme" );
    	entvars_t *pev = &pEntity->v;
	    CBasePlayer* pBot = GetClassPtr((CBasePlayer*)pev); //Link bot object to the edict

      pBot->Init();
      pev->flags |= FL_FAKECLIENT; // bot is fakeclient
	    pBot->Spawn();
	    pev->flags |= FL_FAKECLIENT; // bot is fakeclient
      pBot = (CBasePlayer *)CBasePlayer::Instance(pEntity);
      g_pGameRules->PlayerThink( pBot);
	g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex(),
		g_engfuncs.pfnGetInfoKeyBuffer(pEntity), "model", "xxx");
  }
  */
    
  return false;
}

CBaseEntity* FindEntityForward( CBasePlayer* pMe )
{
	TraceResult tr;
	UTIL_MakeVectors(pMe->pev->v_angle);
	UTIL_TraceLine(pMe->pev->origin + pMe->pev->view_ofs,pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 2048,missile, pMe->edict(), &tr );
	if ( tr.flFraction != 1.0 && !FNullEnt( tr.pHit) )
	{
		CBaseEntity* pHit = CBaseEntity::Instance( tr.pHit );
		return pHit;
	}
	return NULL;
}

void AgClient::Say(CBasePlayer* pPlayer, say_type Type )
{
  char* pSayText = (char*)CMD_ARGS(); 
  
  if ( CMD_ARGC() < 2  || 128 < strlen(pSayText) || !pPlayer || !g_pGameRules || 1 > strlen(pSayText))
    return;
  
  //Check if the user is flooding us with nonsens.
  if ((All == Type || pPlayer->IsSpectator()) && pPlayer->FloodCheck() || pPlayer->FloodText())
    return;
  
  bool bSendLocation = false;
  bool bSendDeathLocation = false;
  
  char szText[256];
  char* pText = szText;
  
  if (pPlayer->IsSpectator() && Team == Type)
    pText = pText + sprintf(pText,"%c(ST) %s: ", 2, pPlayer->GetName());
  else if (pPlayer->IsSpectator())
    pText = pText + sprintf(pText,"%c(S) %s: ", 2, pPlayer->GetName());
  else if (Team == Type)
    pText = pText + sprintf(pText,"%c(T) %s: ", 2, pPlayer->GetName());
  else if (Close == Type)
    pText = pText + sprintf(pText,"%c(C) %s: ", 2, pPlayer->GetName());
  else
    pText = pText + sprintf(pText,"%c(A) %s: ", 2, pPlayer->GetName());
  
  //Remove quotes 
  if (*pSayText == '"')
  {
    pSayText++;
    pSayText[strlen(pSayText)-1] = 0;
  }
  
  if (1 > strlen(pSayText))
    return;
  
  //Check AG teambind
  while (*pSayText && (pText - szText) < 200) //Dont overflow the string. Stop when its 200 chars.
  {
    if ('%' == *pSayText)
    {
      pSayText++;
      if ('h' == *pSayText || 'H' == *pSayText)
      {
        //Health.
        if (pPlayer && pPlayer->IsAlive() && !pPlayer->IsSpectator())
          pText = pText + sprintf(pText,"%.0f",pPlayer->pev->health);
        pSayText++;
        continue;
      }
      else if ('a' == *pSayText || 'A' == *pSayText)
      {
        //Armour.
        if (pPlayer && pPlayer->IsAlive() && !pPlayer->IsSpectator())
          pText = pText + sprintf(pText,"%.0f",pPlayer->pev->armorvalue);
        pSayText++;
        continue;
      }
      else if ('q' == *pSayText || 'Q' == *pSayText)
      {
        //Ammo.
        if (pPlayer && pPlayer->IsAlive() && !pPlayer->IsSpectator() && pPlayer->m_pActiveItem && pPlayer->m_pActiveItem->m_iId < MAX_AMMO_SLOTS)
        {
          CBasePlayerWeapon* pWeapon = (CBasePlayerWeapon*)pPlayer->m_pActiveItem->GetWeaponPtr();
          if (pWeapon)
          {
            int	iAmmoIndex1 = -1;
            int	iAmmoIndex2 = -1;
            
            if (pWeapon->pszAmmo1())
              iAmmoIndex1 = pPlayer->GetAmmoIndex( pWeapon->pszAmmo1() ); 
            if (pWeapon->pszAmmo2())
              iAmmoIndex2 = pPlayer->GetAmmoIndex( pWeapon->pszAmmo2() ); 
            
            if (iAmmoIndex1 != -1 && iAmmoIndex2 != -1)
              pText = pText + sprintf(pText,"%d/%d",(int)(pPlayer->m_rgAmmo[iAmmoIndex1] + pWeapon->m_iClip),(int)(pPlayer->m_rgAmmo[iAmmoIndex2]));
            else if (iAmmoIndex1 != -1)
              pText = pText + sprintf(pText,"%d",(int)(pPlayer->m_rgAmmo[iAmmoIndex1] + pWeapon->m_iClip));
            else if (iAmmoIndex2 != -1)
              pText = pText + sprintf(pText,"%d",(int)(pPlayer->m_rgAmmo[iAmmoIndex2] + pWeapon->m_iClip));
            else 
              pText = pText + sprintf(pText,"0");
          }
 	      }      
        pSayText++;
        continue;
      }
      else if ('w' == *pSayText || 'W' == *pSayText)
      {
        //Weapon
        if (pPlayer && pPlayer->IsAlive() && !pPlayer->IsSpectator() && pPlayer->m_pActiveItem && pPlayer->m_pActiveItem->m_iId < MAX_WEAPONS)
        {
          char* pWeapon = strstr(pPlayer->m_pActiveItem->pszName(),"weapon_");
          if (pWeapon)
          {
            pText = pText + sprintf(pText,"%s",&pWeapon[7]);
          }
        }
        pSayText++;
        continue;
      }
      else if ('p' == *pSayText || 'P' == *pSayText)
      {
        //LJ Status
        if (pPlayer && pPlayer->IsAlive() && !pPlayer->IsSpectator())
        {
          if (pPlayer->m_fLongJump)
            pText = pText + sprintf(pText,"got LJ");
          else
            pText = pText + sprintf(pText,"no LJ");
        }
        pSayText++;
        continue;
      }
	  /*
      else if ('o' == *pSayText || 'o' == *pSayText)
      {
        //Looking at.
        if (pPlayer && pPlayer->IsAlive() && !pPlayer->IsSpectator())
        {
			CBaseEntity* pEntity = FindEntityForward(pPlayer);
			if (pEntity && pEntity->pev)
				pText = pText + sprintf(pText,STRING(pEntity->pev->classname));
        }
        pSayText++;
        continue;
      }
	  */
      else if ('s' == *pSayText || 'S' == *pSayText)
      {
        //Score
        if (pPlayer && pPlayer->IsAlive() && !pPlayer->IsSpectator())
          pText = pText + sprintf(pText,"%.0f/%d",pPlayer->pev->frags,pPlayer->m_iDeaths);
        pSayText++;
        continue;
      }
      else if ('f' == *pSayText || 'F' == *pSayText)
      {
        //Flag info
        if (pPlayer && pPlayer->IsAlive() && !pPlayer->IsSpectator())
          if (pPlayer->m_bFlagTeam1)
            pText = pText + sprintf(pText,CTF_TEAM1_NAME);
          else if (pPlayer->m_bFlagTeam2)
            pText = pText + sprintf(pText,CTF_TEAM2_NAME);
        pSayText++;
        continue;
      }
      else if ('l' == *pSayText || 'L' == *pSayText)
      {
#ifdef AG_NO_CLIENT_DLL
		pText = pText + sprintf(pText,g_pGameRules->m_LocationCache.Location(pPlayer->pev->origin).c_str());
        pSayText++;
        continue;
#else
		bSendLocation = true;
        pText[0] = '%';
        pText++;
#endif
      }
      else if ('d' == *pSayText || 'D' == *pSayText)
      {
#ifdef AG_NO_CLIENT_DLL
		pText = pText + sprintf(pText,g_pGameRules->m_LocationCache.Location(pPlayer->GetKilledPosition()).c_str());
        pSayText++;
        continue;
#else
        bSendDeathLocation = true;
        pText[0] = '%';
        pText++;
#endif
      }
      else
      {
        pText[0] = '%';
        pText++;
        continue;
      }
    }
    
    *pText = *pSayText;
    pText++;
    pSayText++;
  }
  *pText = '\0';
#ifdef AG_NO_CLIENT_DLL
	AgStripColors(szText);
#endif
  
	// team match?
	char * temp;
	if (All == Type)
		temp = "say";
	else
    temp = "say_team";
		
	if ( g_teamplay )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" %s \"%s\"\n", 
			STRING( pPlayer->edict()->v.netname ), 
			GETPLAYERUSERID( pPlayer->edict() ),
			GETPLAYERAUTHID( pPlayer->edict() ),
			g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model" ),
			temp,
			szText );
	}
	else
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%i>\" %s \"%s\"\n", 
			STRING( pPlayer->edict()->v.netname ), 
			GETPLAYERUSERID( pPlayer->edict() ),
			GETPLAYERAUTHID( pPlayer->edict() ),
			GETPLAYERUSERID( pPlayer->edict() ),
			temp,
			szText );
	}
  
  if (pPlayer->IsSpectator())
  {
    //Spectators can talk to team m8
    //Loop through all players
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
      CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
      if (pPlayerLoop && pPlayerLoop->IsNetClient())
      {
		// can the receiver hear the sender? or has he muted him?
		if ( g_VoiceGameMgr.PlayerHasBlockedPlayer( pPlayerLoop, pPlayer ) )
			continue;

        //Sort team messages. Only talk to team m8's that are spectators
        if ( (Team == Type || Close == Type )  
          && !pPlayerLoop->IsSpectator() && pPlayer != pPlayerLoop)
          continue;
        
        //Dont let specs talk to all if spectalk = 0 if it aint an admin on a match
        if (0 == ag_spectalk.value && !pPlayer->IsAdmin() && All == Type && 0 < ag_match_running.value && pPlayer != pPlayerLoop)
          continue;
        
#ifndef AG_NO_CLIENT_DLL
        if (bSendLocation)
        {
          MESSAGE_BEGIN( MSG_ONE, gmsgLocation, NULL, pPlayerLoop->edict() );  
            WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
  	        WRITE_COORD( pPlayer->pev->origin[0]);
		        WRITE_COORD( pPlayer->pev->origin[1]);
		        WRITE_COORD( pPlayer->pev->origin[2]);
          MESSAGE_END();
        }

        if (bSendDeathLocation)
        {
          MESSAGE_BEGIN( MSG_ONE, gmsgLocation, NULL, pPlayerLoop->edict() );  
            WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
  	        WRITE_COORD( pPlayer->GetKilledPosition()[0]);
		        WRITE_COORD( pPlayer->GetKilledPosition()[1]);
		        WRITE_COORD( pPlayer->GetKilledPosition()[2]);
          MESSAGE_END();
        }
#endif

        //Ok to send text.
        MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayerLoop->edict() );
	        WRITE_BYTE( pPlayer->entindex() );
          WRITE_STRING( UTIL_VarArgs("%s\n", szText) );
        MESSAGE_END();
      }
    }
  }
  else
  {
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
      CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
      if (pPlayerLoop && pPlayerLoop->IsNetClient())
      {
		// can the receiver hear the sender? or has he muted him?
		if ( g_VoiceGameMgr.PlayerHasBlockedPlayer( pPlayerLoop, pPlayer ) )
			continue;

        if (Close == Type && pPlayer != pPlayerLoop)
        {
          //Check if team m8 is close enough.
          vec3_t vDist = pPlayerLoop->pev->origin - pPlayer->pev->origin;
          if (vDist .Length () > 700) 
            continue;
        }
        
        // for team or close we only say to our own team
        if ( (Team == Type || Close == Type ) && 
              g_pGameRules->PlayerRelationship(pPlayerLoop, pPlayer) != GR_TEAMMATE  && 
               pPlayer != pPlayerLoop)
          continue;

        //Dont let spectators read your team talk.
        if ( (Team == Type || Close == Type ) && pPlayerLoop->IsSpectator())
          continue;
        
#ifndef AG_NO_CLIENT_DLL
        if (bSendLocation)
        {
          MESSAGE_BEGIN( MSG_ONE, gmsgLocation, NULL, pPlayerLoop->edict() );  
            WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
  	        WRITE_COORD( pPlayer->pev->origin[0]);
		        WRITE_COORD( pPlayer->pev->origin[1]);
		        WRITE_COORD( pPlayer->pev->origin[2]);
          MESSAGE_END();
        }

        if (bSendDeathLocation)
        {
          MESSAGE_BEGIN( MSG_ONE, gmsgLocation, NULL, pPlayerLoop->edict() );  
            WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
  	        WRITE_COORD( pPlayer->GetKilledPosition()[0]);
		        WRITE_COORD( pPlayer->GetKilledPosition()[1]);
		        WRITE_COORD( pPlayer->GetKilledPosition()[2]);
          MESSAGE_END();
        }
#endif

        //Ok to send text.
        MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayerLoop->edict() );
	        WRITE_BYTE( pPlayer->entindex() );
          WRITE_STRING( UTIL_VarArgs("%s\n", szText) );
        MESSAGE_END();
      }
    }    
  }
  
  // echo to server console
  AgConsole(szText);
}

void AgClient::Play(CBasePlayer* pPlayer, say_type Type, const char* pszWave)
{
	//Check for flooding if spectator
	if (pPlayer->IsSpectator() && pPlayer->FloodCheck() || !g_pGameRules->IsTeamplay() || pPlayer->FloodSound())
		return;
	
	//Play sound to teammates.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
		// for team or close we only play to our own team
		if ( (Team == Type || Close == Type) && !(pPlayerLoop == pPlayer || g_pGameRules->PlayerRelationship( pPlayer, pPlayerLoop ) == GR_TEAMMATE))
			continue;
		
		vec3_t vDist = pPlayerLoop->pev->origin - pPlayer->pev->origin;
		if (Type == Close && vDist.Length () > 700) 
			continue;
		
		//Dont let spectators hear your team talk.
		if ( (Team == Type || Close == Type ) && pPlayerLoop->IsSpectator())
			continue;
  		
#ifdef AG_NO_CLIENT_DLL
		AgSound(pPlayerLoop, pszWave);
#else
		MESSAGE_BEGIN( MSG_ONE_UNRELIABLE, gmsgPlaySound, NULL, pPlayerLoop->pev );
		  WRITE_BYTE( pPlayer->entindex());
			WRITE_COORD( pPlayer->pev->origin[0]);
			WRITE_COORD( pPlayer->pev->origin[1]);
			WRITE_COORD( pPlayer->pev->origin[2]);
			WRITE_STRING(pszWave);
		MESSAGE_END();
#endif
			
      /*
      MESSAGE_BEGIN( MSG_ONE, gmsgPlaySound, NULL, pPlayerLoop->pev );
		    WRITE_SHORT( pPlayer->entindex());
        WRITE_STRING(CMD_ARGV(1));
				MESSAGE_END();
      */
	}
}
//-- Martin Webrant
