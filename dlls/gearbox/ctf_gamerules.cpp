/***
*
*	Copyright (c) 2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== ctf_gamerules.cpp ========================================================

This contains all the gamerules for the Half-Life: Opposing force CTF Gamemode.

*/

#define NUM_TEAMS 2

char *sTeamNames[] =
{
	"SPECTATOR",
	"RED",
	"BLUE",
};

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"game.h"
#include	"items.h"
#include	"ctf_gamerules.h"
#include	"ctf_items.h"
#include	"ctf_powerups.h"
#include	"enginecallback.h"

extern int gmsgCTFMsgs;
extern int gmsgShowMenu;
extern int gmsgFlagStatus;
extern int gmsgRuneStatus;
extern int gmsgFlagCarrier;
extern int gmsgScoreInfo;

extern unsigned short g_usHook;
extern unsigned short g_usCable;
extern unsigned short g_usCarried;
extern unsigned short g_usFlagSpawn;


static char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
static int team_scores[MAX_TEAMS];
static int num_teams = 0;

bool g_bSpawnedRunes;
void SpawnRunes(void);

extern edict_t *EntSelectSpawnPoint(CBaseEntity *pPlayer);
#if 0
extern edict_t *EntSelectSpawnPoint(CBaseEntity *pPlayer, bool bCheckDM);
#else
edict_t *EntSelectSpawnPoint(CBaseEntity *pPlayer, bool bCheckDM)
{
	return EntSelectSpawnPoint(pPlayer);
}
#endif
extern edict_t *RuneSelectSpawnPoint(void);

#ifndef NO_VOICEGAMEMGR
class CCTFGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer(CBasePlayer *pPlayer1, CBasePlayer *pPlayer2)
	{
		return stricmp(pPlayer1->TeamID(), pPlayer2->TeamID()) == 0;
	}
};
static CCTFGameMgrHelper g_GameMgrHelper;
#endif

extern DLL_GLOBAL BOOL		g_fGameOver;

char* GetTeamName(int team)
{
	if (team < 0 || team > NUM_TEAMS)
		team = 0;

	return sTeamNames[team];
}

CCTFMultiplay::CCTFMultiplay()
{
#ifndef NO_VOICEGAMEMGR
	// CHalfLifeMultiplay already initialized it - just override its helper callback.
	m_VoiceGameMgr.SetHelper(&g_GameMgrHelper);
#endif
	m_DisableDeathMessages = FALSE;
	m_DisableDeathPenalty = FALSE;

	memset(team_names, 0, sizeof(team_names));
	memset(team_scores, 0, sizeof(team_scores));
	num_teams = 0;

	iBlueTeamScore = iRedTeamScore = 0;
	g_bSpawnedRunes = FALSE;

	// Copy over the team from the server config
	m_szTeamList[0] = 0;

	// Cache this because the team code doesn't want to deal with changing this in the middle of a game
	strncpy(m_szTeamList, teamlist.string, TEAMPLAY_TEAMLISTLENGTH);

	edict_t *pWorld = INDEXENT(0);
	if (pWorld && pWorld->v.team)
	{
		if (teamoverride.value)
		{
			const char *pTeamList = STRING(pWorld->v.team);
			if (pTeamList && strlen(pTeamList))
			{
				strncpy(m_szTeamList, pTeamList, TEAMPLAY_TEAMLISTLENGTH);
			}
		}
	}
	// Has the server set teams
	if (strlen(m_szTeamList))
		m_teamLimit = TRUE;
	else
		m_teamLimit = FALSE;

	RecountTeams();
}


BOOL CCTFMultiplay::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	return CHalfLifeMultiplay::ClientConnected(pEntity, pszName, pszAddress, szRejectReason);
}


extern cvar_t timeleft, fragsleft;

void CCTFMultiplay::Think(void)
{
#ifndef NO_VOICEGAMEMGR
	m_VoiceGameMgr.Update(gpGlobals->frametime);
#endif
	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	if (g_fGameOver)   // someone else quit the game already
	{
		CHalfLifeMultiplay::Think();
		return;
	}

	float flTimeLimit = CVAR_GET_FLOAT("mp_timelimit") * 60;

	time_remaining = (int)(flTimeLimit ? (flTimeLimit - gpGlobals->time) : 0);

	if (flTimeLimit != 0 && gpGlobals->time >= flTimeLimit)
	{
		GoToIntermission();
		return;
	}

	float flFragLimit = fraglimit.value;
	if (flFragLimit)
	{
		int bestfrags = 9999;
		int remain;

		// check if any team is over the frag limit
		for (int i = 0; i < num_teams; i++)
		{
			if (team_scores[i] >= flFragLimit)
			{
				GoToIntermission();
				return;
			}

			remain = flFragLimit - team_scores[i];
			if (remain < bestfrags)
			{
				bestfrags = remain;
			}
		}
		frags_remaining = bestfrags;
	}

	if (!g_bSpawnedRunes)
		SpawnRunes();

	if (m_flFlagStatusTime && m_flFlagStatusTime <= gpGlobals->time)
		GetFlagStatus(NULL);

	// Updates when frags change
	if (frags_remaining != last_frags)
	{
		g_engfuncs.pfnCvar_DirectSet(&fragsleft, UTIL_VarArgs("%i", frags_remaining));
	}

	// Updates once per second
	if (timeleft.value != last_time)
	{
		g_engfuncs.pfnCvar_DirectSet(&timeleft, UTIL_VarArgs("%i", time_remaining));
	}

	last_frags = frags_remaining;
	last_time = time_remaining;
}

void CCTFMultiplay::JoinTeam(CBasePlayer *pPlayer, int iTeam)
{
	if (pPlayer->pev->team == iTeam)
		return;

	if (pPlayer->m_flNextTeamChange > gpGlobals->time)
		return;

	pPlayer->m_flNextTeamChange = gpGlobals->time + 5;

	if (pPlayer->pev->team == 0)
	{
		ChangePlayerTeam(pPlayer, iTeam);
		RecountTeams();

		pPlayer->Spawn();
	}
	else
	{
		ChangePlayerTeam(pPlayer, iTeam);
		RecountTeams();
	}
}

int CCTFMultiplay::TeamWithFewestPlayers(void)
{

	CBaseEntity *pPlayer = NULL;

	int iNumRed, iNumBlue;

	int iTeam;

	// Initialize the player counts..
	iNumRed = iNumBlue = 0;

	pPlayer = UTIL_FindEntityByClassname(pPlayer, "player");

	while ((pPlayer != NULL) && (!FNullEnt(pPlayer->edict())))
	{
		if (pPlayer->pev->flags != FL_DORMANT)
		{
			pPlayer = GetClassPtr((CBasePlayer *)pPlayer->pev);


			if (pPlayer->pev->team == RED)
				iNumRed += 1;

			else if (pPlayer->pev->team == BLUE)
				iNumBlue += 1;

		}
		pPlayer = UTIL_FindEntityByClassname(pPlayer, "player");
	}

	if (iNumRed == iNumBlue)
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0: iTeam = RED; break;
		case 1: iTeam = BLUE; break;
		}
	}
	else if (iNumRed == 0 && iNumBlue == 0)
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0: iTeam = RED; break;
		case 1: iTeam = BLUE; break;
		}
	}

	else if (iNumRed > iNumBlue)
		iTeam = BLUE;

	else if (iNumRed < iNumBlue)
		iTeam = RED;

	return iTeam;

}

void DropRune(CBasePlayer *pPlayer);
//=========================================================
// ClientCommand
// the user has typed a command which is unrecognized by everything else;
// this check to see if the gamerules knows anything about the command
//=========================================================
BOOL CCTFMultiplay::ClientCommand(CBasePlayer *pPlayer, const char *pcmd)
{
#ifndef NO_VOICEGAMEMGR
	if (m_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return TRUE;
#endif
	if (FStrEq(pcmd, "menuselect"))
	{
		if (CMD_ARGC() < 2)
			return TRUE;

		int slot = atoi(CMD_ARGV(1));

		// select the item from the current menu
		switch (pPlayer->m_iMenu)
		{
		case Team_Menu:

			switch (slot)
			{
			case 1:
				JoinTeam(pPlayer, RED);
				break;
			case 2:
				JoinTeam(pPlayer, BLUE);
				break;
			case 5:
				JoinTeam(pPlayer, TeamWithFewestPlayers());
				break;
			}

			break;

		case Team_Menu_IG:

			switch (slot)
			{
			case 1:
				JoinTeam(pPlayer, RED);
				break;
			case 2:
				JoinTeam(pPlayer, BLUE);
				break;
			case 5:
				JoinTeam(pPlayer, TeamWithFewestPlayers());
				break;
			default:
				return TRUE;
			}

			break;
		}

		return TRUE;
	}
	else if (FStrEq(pcmd, "droprune"))
	{
		DropRune(pPlayer);

		return TRUE;
	}
	else if (FStrEq(pcmd, "changeteam"))
	{
		if (pPlayer->pev->team != 0)
		{
			pPlayer->ShowMenu(1 + 2 + 16 + 512, -1, FALSE, "#Team_Menu_Join_IG");
			pPlayer->m_iMenu = Team_Menu_IG;
		}

		return TRUE;
	}

	return FALSE;
}

extern int gmsgGameMode;
extern int gmsgSayText;
extern int gmsgTeamInfo;

void CCTFMultiplay::UpdateGameMode(CBasePlayer *pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, NULL, pPlayer->edict());
	WRITE_BYTE(1);  // game mode teamplay
	MESSAGE_END();
}

edict_t *CCTFMultiplay::GetPlayerSpawnSpot(CBasePlayer *pPlayer)
{
	edict_t *pentSpawnSpot;

	if (FBitSet(pPlayer->m_afPhysicsFlags, PFLAG_OBSERVER) || pPlayer->pev->team == 0)
		pentSpawnSpot = EntSelectSpawnPoint(pPlayer, FALSE);
	else
	{
		if (RANDOM_LONG(1, 7) < 3)
			pentSpawnSpot = EntSelectSpawnPoint(pPlayer, TRUE);
		else
			pentSpawnSpot = EntSelectSpawnPoint(pPlayer, FALSE);
	}

	if (IsMultiplayer() && pentSpawnSpot->v.target)
	{
		FireTargets(STRING(pentSpawnSpot->v.target), pPlayer, pPlayer, USE_TOGGLE, 0);
	}

	pPlayer->pev->origin = VARS(pentSpawnSpot)->origin + Vector(0, 0, 1);
	pPlayer->pev->v_angle = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = VARS(pentSpawnSpot)->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = TRUE;

	return pentSpawnSpot;
}

void CCTFMultiplay::PlayerTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker)
{
	if (!pAttacker->IsPlayer())
		return;

	if (pPlayer->pev->team == pAttacker->pev->team)
		return;

	if (pPlayer->m_bHasFlag)
	{
		pPlayer->pCarrierHurter = (CBasePlayer *)pAttacker;
		pPlayer->m_flCarrierHurtTime = gpGlobals->time + TEAM_CAPTURE_CARRIER_DANGER_PROTECT_TIMEOUT;
	}

}

void CCTFMultiplay::PlayerSpawn(CBasePlayer *pPlayer)
{
	BOOL		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

	if (pPlayer->pev->team == 0)
	{
		pPlayer->pev->takedamage = DAMAGE_NO;
		pPlayer->pev->solid = SOLID_NOT;
		pPlayer->pev->movetype = MOVETYPE_NOCLIP;
		pPlayer->pev->effects |= EF_NODRAW;
		pPlayer->pev->flags |= FL_NOTARGET;
		pPlayer->m_afPhysicsFlags |= PFLAG_OBSERVER;
		pPlayer->m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_HEALTH;

		pPlayer->m_flFlagStatusTime = gpGlobals->time + 0.1;
	}
	else
	{
		pPlayer->pev->weapons |= (1 << WEAPON_SUIT);

		addDefault = TRUE;

		while (pWeaponEntity = UTIL_FindEntityByClassname(pWeaponEntity, "game_player_equip"))
		{
			pWeaponEntity->Touch(pPlayer);
			addDefault = FALSE;
		}

		if (addDefault)
		{
			pPlayer->m_bHasFlag = FALSE;

			pPlayer->m_iHideHUD &= ~HIDEHUD_WEAPONS;
			pPlayer->m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
			pPlayer->m_iHideHUD &= ~HIDEHUD_HEALTH;
			pPlayer->m_afPhysicsFlags &= ~PFLAG_OBSERVER;

#if 0
			// Start with init ammoload
			pPlayer->m_iAmmoShells = 25;

			// Start with shotgun and axe
			pPlayer->GiveNamedItem("weapon_quakegun");
			pPlayer->m_iQuakeItems |= (IT_SHOTGUN | IT_AXE | IT_EXTRA_WEAPON);
			pPlayer->m_iQuakeWeapon = pPlayer->W_BestWeapon();
			pPlayer->W_SetCurrentAmmo();
#endif

			pPlayer->m_flFlagStatusTime = gpGlobals->time + 0.1;
		}
	}

	/*	MESSAGE_BEGIN( MSG_ONE, gmsgRuneStatus, NULL, pPlayer->pev);
	WRITE_BYTE( pPlayer->m_iRuneStatus );
	MESSAGE_END();*/
}

void CBasePlayer::ShowMenu(int bitsValidSlots, int nDisplayTime, BOOL fNeedMore, char *pszText)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pev);
	WRITE_SHORT(bitsValidSlots);
	WRITE_CHAR(nDisplayTime);
	WRITE_BYTE(fNeedMore);
	WRITE_STRING(pszText);
	MESSAGE_END();
}

//=========================================================
// InitHUD
//=========================================================
void CCTFMultiplay::InitHUD(CBasePlayer *pPlayer)
{
	CHalfLifeMultiplay::InitHUD(pPlayer);

	int clientIndex = pPlayer->entindex();
	// update this player with all the other players team info
	// loop through all active players and send their team info to the new client
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *plr = UTIL_PlayerByIndex(i);
		if (plr)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict());
			WRITE_BYTE(plr->entindex());
			WRITE_STRING(plr->TeamID());
			MESSAGE_END();

			if (((CBasePlayer*)plr)->m_bHasFlag)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgFlagCarrier, NULL, pPlayer->edict());
				WRITE_BYTE(plr->entindex());
				WRITE_BYTE(1);
				MESSAGE_END();
			}
		}
	}

	//Remove Rune icon if we have one.
	MESSAGE_BEGIN(MSG_ONE, gmsgRuneStatus, NULL, pPlayer->pev);
	WRITE_BYTE(0);
	MESSAGE_END();

	if (pPlayer->pev->team == 0)
	{
		pPlayer->ShowMenu(1 + 2 + 16, -1, FALSE, "#Team_Menu_Join");
		pPlayer->m_iMenu = Team_Menu;
	}
}


void CCTFMultiplay::ChangePlayerTeam(CBasePlayer *pPlayer, int iTeam)
{
	int damageFlags = DMG_GENERIC;
	int clientIndex = pPlayer->entindex();

	if (pPlayer->pev->team != 0)
	{
		damageFlags |= DMG_ALWAYSGIB;

		// kill the player,  remove a death,  and let them start on the new team
		m_DisableDeathMessages = TRUE;
		m_DisableDeathPenalty = TRUE;

		entvars_t *pevWorld = VARS(INDEXENT(0));
		pPlayer->TakeDamage(pevWorld, pevWorld, 900, damageFlags);

		m_DisableDeathMessages = FALSE;
		m_DisableDeathPenalty = FALSE;
	}

	int oldTeam = pPlayer->pev->team;
	pPlayer->pev->team = iTeam;

	if (pPlayer->pev->team == RED)
	{
		strncpy(pPlayer->m_szTeamName, "RED", TEAM_NAME_LENGTH);
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", "red");
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "topcolor", UTIL_VarArgs("%d", 255));
	}
	else if (pPlayer->pev->team == BLUE)
	{
		strncpy(pPlayer->m_szTeamName, "BLUE", TEAM_NAME_LENGTH);
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", "blue");
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "topcolor", UTIL_VarArgs("%d", 153));
	}

	// notify everyone's HUD of the team change
	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
	WRITE_BYTE(clientIndex);
	WRITE_STRING(pPlayer->m_szTeamName);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(ENTINDEX(pPlayer->edict()));
	WRITE_SHORT(pPlayer->pev->frags);
	WRITE_SHORT(pPlayer->m_iDeaths);
	WRITE_SHORT(pPlayer->pev->team);
	MESSAGE_END();

	// log the change
	UTIL_LogPrintf("\"%s<%i><%s><%s>\" joined team \"%s\"\n",
		STRING(pPlayer->pev->netname),
		GETPLAYERUSERID(pPlayer->edict()),
		GETPLAYERAUTHID(pPlayer->edict()),
		GetTeamName(oldTeam),
		pPlayer->m_szTeamName);
}


//=========================================================
// ClientUserInfoChanged
//=========================================================
void CCTFMultiplay::ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer)
{

	int clientIndex = pPlayer->entindex();

	if (pPlayer->pev->team == RED)
	{
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "topcolor", UTIL_VarArgs("%d", 255));
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", "red");
	}
	else if (pPlayer->pev->team == BLUE)
	{
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "topcolor", UTIL_VarArgs("%d", 153));
		g_engfuncs.pfnSetClientKeyValue(clientIndex, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", "blue");
	}

}

extern int gmsgDeathMsg;

//=========================================================
// Deathnotice. 
//=========================================================
void CCTFMultiplay::DeathNotice(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor)
{
	if (m_DisableDeathMessages)
		return;

	if (pVictim && pKiller && pKiller->flags & FL_CLIENT)
	{
		CBasePlayer *pk = (CBasePlayer*)CBaseEntity::Instance(pKiller);

		if (pk)
		{
			if ((pk != pVictim) && (PlayerRelationship(pVictim, pk) == GR_TEAMMATE))
			{
				MESSAGE_BEGIN(MSG_ALL, gmsgDeathMsg);
				WRITE_BYTE(ENTINDEX(ENT(pKiller)));		// the killer
				WRITE_BYTE(ENTINDEX(pVictim->edict()));	// the victim
				WRITE_STRING("teammate");		// flag this as a teammate kill
				MESSAGE_END();
				return;
			}
		}
	}

	CHalfLifeMultiplay::DeathNotice(pVictim, pKiller, pevInflictor);
}

//=========================================================
//=========================================================
void CCTFMultiplay::ClientDisconnected(edict_t *pClient)
{
	if (pClient)
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pClient);

		if (pPlayer)
		{
			//We have the flag, spawn it
			if (pPlayer->m_bHasFlag)
			{
				CBaseEntity *pEnt;

				//We have the BLUE flag, Spawn it
				if (pPlayer->pev->team == RED)
				{
					pEnt = CBaseEntity::Create("item_flag_team2", pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict());

					UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Dropped_Blue_Flag\"\n",
						STRING(pPlayer->pev->netname),
						GETPLAYERUSERID(pPlayer->edict()),
						GETPLAYERAUTHID(pPlayer->edict()),
						GetTeamName(pPlayer->pev->team));
				}
				//We have the RED flag, Spawn it
				else if (pPlayer->pev->team == BLUE)
				{
					pEnt = CBaseEntity::Create("item_flag_team1", pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict());

					UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Dropped_Red_Flag\"\n",
						STRING(pPlayer->pev->netname),
						GETPLAYERUSERID(pPlayer->edict()),
						GETPLAYERAUTHID(pPlayer->edict()),
						GetTeamName(pPlayer->pev->team));
				}

				pEnt->pev->velocity = pPlayer->pev->velocity * 1.2;
				pEnt->pev->angles.x = 0;

				CItemFlag *pFlag = (CItemFlag *)pEnt;
				pFlag->Dropped = TRUE;
				pFlag->m_flDroppedTime = gpGlobals->time + TEAM_CAPTURE_FLAG_RETURN_TIME;

#if 0
				PLAYBACK_EVENT_FULL(FEV_GLOBAL | FEV_RELIABLE,
					pPlayer->edict(), g_usCarried, 0, (float *)&g_vecZero, (float *)&g_vecZero,
					0.0, 0.0, pPlayer->entindex(), pPlayer->pev->team, 1, 0);
#endif

				MESSAGE_BEGIN(MSG_ALL, gmsgCTFMsgs, NULL);
				if (pPlayer->pev->team == RED)
					WRITE_BYTE(BLUE_FLAG_LOST);
				else if (pPlayer->pev->team == BLUE)
					WRITE_BYTE(RED_FLAG_LOST);

				WRITE_STRING(STRING(pPlayer->pev->netname));
				MESSAGE_END();

				m_flFlagStatusTime = gpGlobals->time + 0.1;

				pPlayer->m_bHasFlag = FALSE;
			}

			// drop any runes the player has
			CBaseEntity *pRune;
			char * runeName;

			switch (pPlayer->m_iRuneStatus)
			{
			case IT_CTF_ACCELERATOR_FLAG:

				pRune = CBaseEntity::Create(IT_CTF_ACCELERATOR_CLASSNAME, pPlayer->pev->origin, pPlayer->pev->angles, NULL);

				pRune->pev->velocity = pPlayer->pev->velocity * 1.5;
				pRune->pev->angles.x = 0;
				((CPowerupAccelerator*)pRune)->dropped = true;

				runeName = "AcceleratorRune";

				break;

			case IT_CTF_BACKPACK_FLAG:

				pRune = CBaseEntity::Create(IT_CTF_BACKPACK_CLASSNAME, pPlayer->pev->origin, pPlayer->pev->angles, NULL);

				pRune->pev->velocity = pPlayer->pev->velocity * 1.5;
				pRune->pev->angles.x = 0;
				((CPowerupBackpack*)pRune)->dropped = true;

				runeName = "BackpackRune";

				break;

			case IT_CTF_LONGJUMP_FLAG:

				pRune = CBaseEntity::Create(IT_CTF_LONGJUMP_CLASSNAME, pPlayer->pev->origin, pPlayer->pev->angles, NULL);

				pRune->pev->velocity = pPlayer->pev->velocity * 1.5;
				pRune->pev->angles.x = 0;
				((CPowerupJumppack*)pRune)->dropped = true;

				runeName = "JumppackRune";

				break;

			case IT_CTF_PORTABLEHEV_FLAG:

				pRune = CBaseEntity::Create(IT_CTF_PORTABLEHEV_CLASSNAME, pPlayer->pev->origin, pPlayer->pev->angles, NULL);

				pRune->pev->velocity = pPlayer->pev->velocity * 1.5;
				pRune->pev->angles.x = 0;
				((CPowerupPorthev*)pRune)->dropped = true;

				runeName = "PortableHEVRune";

				break;

			case IT_CTF_REGEN_FLAG:

				pRune = CBaseEntity::Create(IT_CTF_REGEN_CLASSNAME, pPlayer->pev->origin, pPlayer->pev->angles, NULL);

				pRune->pev->velocity = pPlayer->pev->velocity * 1.5;
				pRune->pev->angles.x = 0;
				((CPowerupRegen*)pRune)->dropped = true;

				runeName = "RegenRune";

				break;

			default:

				runeName = "Unknown";

				break;
			}

			if (pPlayer->m_iRuneStatus)
			{
				pPlayer->m_iRuneStatus = 0;

				UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Dropped_%s\"\n",
					STRING(pPlayer->pev->netname),
					GETPLAYERUSERID(pPlayer->edict()),
					GETPLAYERAUTHID(pPlayer->edict()),
					pPlayer->m_szTeamName,
					runeName);
			}

			FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);

			UTIL_LogPrintf("\"%s<%i><%s><%s>\" disconnected\n",
				STRING(pPlayer->pev->netname),
				GETPLAYERUSERID(pPlayer->edict()),
				GETPLAYERAUTHID(pPlayer->edict()),
				GetTeamName(pPlayer->pev->team));

			pPlayer->RemoveAllItems(TRUE);// destroy all of the players weapons and items
		}
	}
}

void CCTFMultiplay::PlayerThink(CBasePlayer *pPlayer)
{
	if (g_fGameOver)
	{
		// check for button presses
		if (pPlayer->m_afButtonPressed & (IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP))
			m_iEndIntermissionButtonHit = TRUE;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}

	if (pPlayer->pFlagCarrierKiller)
	{
		if (pPlayer->m_flFlagCarrierKillTime <= gpGlobals->time)
			pPlayer->pFlagCarrierKiller = NULL;
	}

	if (pPlayer->pFlagReturner)
	{
		if (pPlayer->m_flFlagReturnTime <= gpGlobals->time)
			pPlayer->pFlagReturner = NULL;
	}

	if (pPlayer->pCarrierHurter)
	{
		if (pPlayer->m_flCarrierHurtTime <= gpGlobals->time)
			pPlayer->pCarrierHurter = NULL;
	}

	// If the player has backpack powerup, replenish his ammunition.
	if (pPlayer->m_iRuneStatus == IT_CTF_BACKPACK_FLAG)
	{
		if (pPlayer->m_flRegenTime <= gpGlobals->time)
		{
			int i;
			ItemInfo II;
			CBasePlayerItem *item;
			CBasePlayerWeapon* weapon;
			for (i = 0; i < MAX_ITEM_TYPES; i++)
			{
				item = pPlayer->m_rgpPlayerItems[i];

				if (item)
				{
					memset(&II, 0, sizeof(II));

					if (item->GetWeaponPtr() && ((CBasePlayerItem*)item)->GetItemInfo(&II))
					{
						weapon = (CBasePlayerWeapon *)item->GetWeaponPtr();

						if (weapon && weapon->m_iClip < II.iMaxClip)
						{
							weapon->AddPrimaryAmmo(
								weapon->m_iDefaultAmmo, 
								(char *)weapon->pszAmmo1(),
								weapon->iMaxClip(),
								weapon->iMaxAmmo1());

							pPlayer->m_flRegenTime = gpGlobals->time + 1;

							EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "ctf/pow_backpack_charge.wav", 1, ATTN_NORM);
						}
					}
				}
			}
		}
	}

	// If the player has portable HEV powerup, recharge his suit.
	if (pPlayer->m_iRuneStatus == IT_CTF_PORTABLEHEV_FLAG)
	{
		if (pPlayer->m_flRegenTime <= gpGlobals->time)
		{
			if (pPlayer->pev->armorvalue < PLAYER_MAX_ARMOR_VALUE && pPlayer->pev->armorvalue)
			{
				pPlayer->pev->armorvalue += 5;

				if (pPlayer->pev->armorvalue > PLAYER_MAX_ARMOR_VALUE)
					pPlayer->pev->armorvalue = PLAYER_MAX_ARMOR_VALUE;

				pPlayer->m_flRegenTime = gpGlobals->time + 1;

				EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "ctf/pow_armor_charge.wav", 1, ATTN_NORM);
			}
		}
	}

	// If the player has regeneration powerup, regenerate his health.
	if (pPlayer->m_iRuneStatus == IT_CTF_REGEN_FLAG)
	{
		if (pPlayer->m_flRegenTime <= gpGlobals->time)
		{

			if (pPlayer->pev->health < PLAYER_MAX_HEALTH_VALUE)
			{
				pPlayer->pev->health += 5;

				if (pPlayer->pev->health > PLAYER_MAX_HEALTH_VALUE)
					pPlayer->pev->health = PLAYER_MAX_HEALTH_VALUE;

				pPlayer->m_flRegenTime = gpGlobals->time + 1;

				EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "ctf/pow_health_charge.wav", 1, ATTN_NORM);
			}
		}
	}


#if 0
	if (pPlayer->m_bOn_Hook)
		pPlayer->Service_Grapple();
#endif

	if (pPlayer->m_flFlagStatusTime && pPlayer->m_flFlagStatusTime <= gpGlobals->time)
		GetFlagStatus(pPlayer);
}

//=========================================================
//=========================================================
void CCTFMultiplay::PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor)
{
	CBasePlayer *pk = NULL;

	if (pKiller)
	{
		CBaseEntity *pTemp = CBaseEntity::Instance(pKiller);

		if (pTemp->IsPlayer())
			pk = (CBasePlayer*)pTemp;
	}

	//Only award a bonus if the Flag carrier had the flag for more than 2 secs
	//Prevents from people waiting for the flag carrier to grab the flag and then killing him
	//Instead of actually defending the flag.
	if (pVictim->m_bHasFlag)
	{
		if (pk)
		{
			if (pVictim->pev->team != pk->pev->team)
			{
				if (pVictim->m_flCarrierPickupTime <= gpGlobals->time)
					pk->AddPoints(TEAM_CAPTURE_FRAG_CARRIER_BONUS, TRUE);

				if (pk->pev->team == RED)
				{
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pk->pev->netname));
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, " fragged ");
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "BLUE");
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "'s flag carrier!\n");

					UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Killed_Enemy_Flag_Carrier\"\n",
						STRING(pk->pev->netname),
						GETPLAYERUSERID(pk->edict()),
						GETPLAYERAUTHID(pk->edict()),
						GetTeamName(pk->pev->team));

					if (iBlueFlagStatus == BLUE_FLAG_STOLEN)
					{
						for (int i = 1; i <= gpGlobals->maxClients; i++)
						{
							CBasePlayer *pTeamMate = (CBasePlayer *)UTIL_PlayerByIndex(i);

							if (pTeamMate)
							{
								if (pTeamMate->m_bHasFlag)
								{
									pTeamMate->pFlagCarrierKiller = pk;
									pTeamMate->m_flFlagCarrierKillTime = gpGlobals->time + TEAM_CAPTURE_FRAG_CARRIER_ASSIST_TIMEOUT;
								}
							}
						}
					}
				}

				if (pk->pev->team == BLUE)
				{
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pk->pev->netname));
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, " fragged ");
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "RED");
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "'s flag carrier!\n");

					UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Killed_Enemy_Flag_Carrier\"\n",
						STRING(pk->pev->netname),
						GETPLAYERUSERID(pk->edict()),
						GETPLAYERAUTHID(pk->edict()),
						GetTeamName(pk->pev->team));

					if (iRedFlagStatus == RED_FLAG_STOLEN)
					{
						for (int i = 1; i <= gpGlobals->maxClients; i++)
						{
							CBasePlayer *pTeamMate = (CBasePlayer *)UTIL_PlayerByIndex(i);

							if (pTeamMate)
							{
								if (pTeamMate->m_bHasFlag)
								{
									pTeamMate->pFlagCarrierKiller = pk;
									pTeamMate->m_flFlagCarrierKillTime = gpGlobals->time + TEAM_CAPTURE_FRAG_CARRIER_ASSIST_TIMEOUT;
								}
							}
						}
					}
				}
			}
		}


		CBaseEntity *pEnt;

		//We have the BLUE flag, Spawn it
		if (pVictim->pev->team == RED)
		{
			pEnt = CBaseEntity::Create("item_flag_team2", pVictim->pev->origin, pVictim->pev->angles, pVictim->edict());

			UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Dropped_Blue_Flag\"\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GetTeamName(pVictim->pev->team));
		}
		else if (pVictim->pev->team == BLUE)
		{
			pEnt = CBaseEntity::Create("item_flag_team1", pVictim->pev->origin, pVictim->pev->angles, pVictim->edict());

			UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Dropped_Red_Flag\"\n",
				STRING(pVictim->pev->netname),
				GETPLAYERUSERID(pVictim->edict()),
				GETPLAYERAUTHID(pVictim->edict()),
				GetTeamName(pVictim->pev->team));
		}

		pEnt->pev->velocity = pVictim->pev->velocity * 1.2;
		pEnt->pev->angles.x = 0;

		CItemFlag *pFlag = (CItemFlag *)pEnt;
		pFlag->Dropped = TRUE;

#if 0
		PLAYBACK_EVENT_FULL(FEV_GLOBAL | FEV_RELIABLE,
			pVictim->edict(), g_usCarried, 0, (float *)&g_vecZero, (float *)&g_vecZero,
			0.0, 0.0, pVictim->entindex(), pVictim->pev->team, 1, 0);
#endif

		pFlag->m_flDroppedTime = gpGlobals->time + TEAM_CAPTURE_FLAG_RETURN_TIME;

		MESSAGE_BEGIN(MSG_ALL, gmsgCTFMsgs, NULL);
		if (pVictim->pev->team == RED)
			WRITE_BYTE(BLUE_FLAG_LOST);
		else if (pVictim->pev->team == BLUE)
			WRITE_BYTE(RED_FLAG_LOST);

		WRITE_STRING(STRING(pVictim->pev->netname));
		MESSAGE_END();

		pVictim->m_bHasFlag = FALSE;

		m_flFlagStatusTime = gpGlobals->time + 0.1;
	}
	else
	{
		if (pk)
		{
			if (pk->pev->team == RED)
			{
				if (iBlueFlagStatus == BLUE_FLAG_STOLEN)
				{
					for (int i = 1; i <= gpGlobals->maxClients; i++)
					{
						CBasePlayer *pTeamMate = (CBasePlayer *)UTIL_PlayerByIndex(i);

						if (pTeamMate && pTeamMate != pk)
						{
							if (pTeamMate->pev->team == pk->pev->team)
							{
								if (pTeamMate->m_bHasFlag)
								{
									if (pTeamMate->pCarrierHurter)
									{
										if (pTeamMate->pCarrierHurter == pVictim)
										{
											if (pTeamMate->m_flCarrierHurtTime > gpGlobals->time)
											{
												UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pk->pev->netname));
												UTIL_ClientPrintAll(HUD_PRINTNOTIFY, " defends ");
												UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "RED");
												UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "'s flag carrier against an agressive enemy\n");

												pk->AddPoints(TEAM_CAPTURE_CARRIER_DANGER_PROTECT_BONUS, TRUE);
											}
										}
									}
								}
							}
						}
					}
				}
			}

			if (pk->pev->team == BLUE)
			{
				if (iRedFlagStatus == RED_FLAG_STOLEN)
				{
					for (int i = 1; i <= gpGlobals->maxClients; i++)
					{
						CBasePlayer *pTeamMate = (CBasePlayer *)UTIL_PlayerByIndex(i);

						if (pTeamMate && pTeamMate != pk)
						{
							if (pTeamMate->pev->team == pk->pev->team)
							{
								if (pTeamMate->m_bHasFlag)
								{
									if (pTeamMate->pCarrierHurter)
									{
										if (pTeamMate->pCarrierHurter == pVictim)
										{
											if (pTeamMate->m_flCarrierHurtTime > gpGlobals->time)
											{
												UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pk->pev->netname));
												UTIL_ClientPrintAll(HUD_PRINTNOTIFY, " defends ");
												UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "BLUE");
												UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "'s flag carrier against an agressive enemy\n");

												pk->AddPoints(TEAM_CAPTURE_CARRIER_DANGER_PROTECT_BONUS, TRUE);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}

	}

	// Find if this guy is near our flag or our flag carrier
	CBaseEntity *ent = NULL;
	float Dist;

	if (pk)
	{
		if (pk->pev->team == RED)
		{
			while ((ent = UTIL_FindEntityByClassname(ent, "item_flag_team1")) != NULL)
			{
				//Do not defend a invisible flag
				if (ent->pev->effects & EF_NODRAW)
					break;

				Dist = (pk->pev->origin - ent->pev->origin).Length();

				if (Dist <= TEAM_CAPTURE_TARGET_PROTECT_RADIUS)
				{
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pk->pev->netname));
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, " defends the ");
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "RED");
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, " flag\n");

					pk->AddPoints(TEAM_CAPTURE_FLAG_DEFENSE_BONUS, TRUE);
					break;
				}
			}

			if (iBlueFlagStatus == BLUE_FLAG_STOLEN)
			{
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					CBasePlayer *pTeamMate = (CBasePlayer *)UTIL_PlayerByIndex(i);

					if (pTeamMate && pTeamMate != pk)
					{
						if (pTeamMate->pev->team == pk->pev->team)
						{
							if (pTeamMate->m_bHasFlag)
							{
								Dist = (pk->pev->origin - pTeamMate->pev->origin).Length();

								if (Dist <= TEAM_CAPTURE_TARGET_PROTECT_RADIUS)
								{
									UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pk->pev->netname));
									UTIL_ClientPrintAll(HUD_PRINTNOTIFY, " defends ");
									UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "RED");
									UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "'s flag carrier\n");

									pk->AddPoints(TEAM_CAPTURE_CARRIER_PROTECT_BONUS, TRUE);
								}
							}
						}
					}
				}
			}

		}
		else if (pk->pev->team == BLUE)
		{
			while ((ent = UTIL_FindEntityByClassname(ent, "item_flag_team2")) != NULL)
			{
				//Do not defend a invisible flag
				if (ent->pev->effects & EF_NODRAW)
					break;

				Dist = (pk->pev->origin - ent->pev->origin).Length();

				if (Dist <= TEAM_CAPTURE_TARGET_PROTECT_RADIUS)
				{
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pk->pev->netname));
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, " defends the ");
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "RED");
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, " flag\n");

					pk->AddPoints(TEAM_CAPTURE_FLAG_DEFENSE_BONUS, TRUE);
					break;
				}
			}

			if (iRedFlagStatus == RED_FLAG_STOLEN)
			{
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					CBasePlayer *pTeamMate = (CBasePlayer *)UTIL_PlayerByIndex(i);

					if (pTeamMate && pTeamMate != pk)
					{
						if (pTeamMate->pev->team == pk->pev->team)
						{
							if (pTeamMate->m_bHasFlag)
							{
								Dist = (pk->pev->origin - pTeamMate->pev->origin).Length();

								if (Dist <= TEAM_CAPTURE_TARGET_PROTECT_RADIUS)
								{
									UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pk->pev->netname));
									UTIL_ClientPrintAll(HUD_PRINTNOTIFY, " defends ");
									UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "RED");
									UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "'s flag carrier\n");

									pk->AddPoints(TEAM_CAPTURE_CARRIER_PROTECT_BONUS, TRUE);
								}
							}
						}
					}
				}
			}

		}
	}

	CBaseEntity *pRune;
	char * runeName;

	switch (pVictim->m_iRuneStatus)
	{
	case IT_CTF_ACCELERATOR_FLAG:

		pRune = CBaseEntity::Create(IT_CTF_ACCELERATOR_CLASSNAME, pVictim->pev->origin, pVictim->pev->angles, NULL);

		pRune->pev->velocity = pVictim->pev->velocity * 1.5;
		pRune->pev->angles.x = 0;
		((CPowerupAccelerator*)pRune)->dropped = true;

		runeName = "AcceleratorRune";

		break;

	case IT_CTF_BACKPACK_FLAG:

		pRune = CBaseEntity::Create(IT_CTF_BACKPACK_CLASSNAME, pVictim->pev->origin, pVictim->pev->angles, NULL);

		pRune->pev->velocity = pVictim->pev->velocity * 1.5;
		pRune->pev->angles.x = 0;
		((CPowerupBackpack*)pRune)->dropped = true;

		runeName = "BackpackRune";

		break;

	case IT_CTF_LONGJUMP_FLAG:

		pRune = CBaseEntity::Create(IT_CTF_LONGJUMP_CLASSNAME, pVictim->pev->origin, pVictim->pev->angles, NULL);

		pRune->pev->velocity = pVictim->pev->velocity * 1.5;
		pRune->pev->angles.x = 0;
		((CPowerupJumppack*)pRune)->dropped = true;

		runeName = "JumppackRune";

		break;

	case IT_CTF_PORTABLEHEV_FLAG:

		pRune = CBaseEntity::Create(IT_CTF_PORTABLEHEV_CLASSNAME, pVictim->pev->origin, pVictim->pev->angles, NULL);

		pRune->pev->velocity = pVictim->pev->velocity * 1.5;
		pRune->pev->angles.x = 0;
		((CPowerupPorthev*)pRune)->dropped = true;

		runeName = "PortableHEVRune";

		break;

	case IT_CTF_REGEN_FLAG:

		pRune = CBaseEntity::Create(IT_CTF_REGEN_CLASSNAME, pVictim->pev->origin, pVictim->pev->angles, NULL);

		pRune->pev->velocity = pVictim->pev->velocity * 1.5;
		pRune->pev->angles.x = 0;
		((CPowerupRegen*)pRune)->dropped = true;

		runeName = "RegenRune";

		break;

	default:

		runeName = "Unknown";

		break;
	}

	if (pVictim->m_iRuneStatus)
	{
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Dropped_%s\"\n",
			STRING(pVictim->pev->netname),
			GETPLAYERUSERID(pVictim->edict()),
			GETPLAYERAUTHID(pVictim->edict()),
			pVictim->m_szTeamName,
			runeName);
	}

#if 0
	if (pVictim->m_ppHook)
		((CGrapple *)pVictim->m_ppHook)->Reset_Grapple();
#endif

	pVictim->m_iRuneStatus = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgRuneStatus, NULL, pVictim->pev);
	WRITE_BYTE(pVictim->m_iRuneStatus);
	MESSAGE_END();

	if (!m_DisableDeathPenalty)
	{
		CHalfLifeMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);
		RecountTeams();
	}
}


//=========================================================
// IsTeamplay
//=========================================================
BOOL CCTFMultiplay::IsTeamplay(void)
{
	return TRUE;
}

BOOL CCTFMultiplay::FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker)
{
	if (pAttacker && PlayerRelationship(pPlayer, pAttacker) == GR_TEAMMATE)
	{
		// my teammate hit me.
		if ((CVAR_GET_FLOAT("mp_friendlyfire") == 0) && (pAttacker != pPlayer))
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return FALSE;
		}
	}

	return CHalfLifeMultiplay::FPlayerCanTakeDamage(pPlayer, pAttacker);
}

//=========================================================
//=========================================================
int CCTFMultiplay::PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if (!pPlayer || !pTarget || !pTarget->IsPlayer())
		return GR_NOTTEAMMATE;

	//As simple as this
	if (pPlayer->pev->team == pTarget->pev->team)
	{
		return GR_TEAMMATE;
	}

	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
BOOL CCTFMultiplay::ShouldAutoAim(CBasePlayer *pPlayer, edict_t *target)
{
	// always autoaim, unless target is a teammate
	CBaseEntity *pTgt = CBaseEntity::Instance(target);
	if (pTgt && pTgt->IsPlayer())
	{
		if (PlayerRelationship(pPlayer, pTgt) == GR_TEAMMATE)
			return FALSE; // don't autoaim at teammates
	}

	return CHalfLifeMultiplay::ShouldAutoAim(pPlayer, target);
}

//=========================================================
//=========================================================
int CCTFMultiplay::IPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled)
{
	if (!pKilled)
		return 0;

	if (!pAttacker)
		return 1;

	if (pAttacker != pKilled && PlayerRelationship(pAttacker, pKilled) == GR_TEAMMATE)
		return -1;

	return 1;
}

//=========================================================
//=========================================================
const char *CCTFMultiplay::GetTeamID(CBaseEntity *pEntity)
{
	if (pEntity == NULL || pEntity->pev == NULL)
		return "";

	// return their team name
	return pEntity->TeamID();
}


int CCTFMultiplay::GetTeamIndex(const char *pTeamName)
{
	if (pTeamName && *pTeamName != 0)
	{
		// try to find existing team
		for (int tm = 0; tm < num_teams; tm++)
		{
			if (!stricmp(team_names[tm], pTeamName))
				return tm;
		}
	}

	return -1;	// No match
}


const char *CCTFMultiplay::GetIndexedTeamName(int teamIndex)
{
	if (teamIndex < 0 || teamIndex >= num_teams)
		return "";

	return team_names[teamIndex];
}


BOOL CCTFMultiplay::IsValidTeam(const char *pTeamName)
{
	if (!m_teamLimit)	// Any team is valid if the teamlist isn't set
		return TRUE;

	return (GetTeamIndex(pTeamName) != -1) ? TRUE : FALSE;
}


void CCTFMultiplay::GetFlagStatus(CBasePlayer *pPlayer)
{
	CBaseEntity *pFlag = NULL;
	int iFoundCount = 0;
	int iDropped = 0;

	while ((pFlag = UTIL_FindEntityByClassname(pFlag, "carried_flag_team1")) != NULL)
	{
		if (pFlag && !FBitSet(pFlag->pev->flags, FL_KILLME))
			iFoundCount++;
	}

	if (iFoundCount >= 1)
		iRedFlagStatus = RED_FLAG_STOLEN;

	if (!iFoundCount)
	{
		while ((pFlag = UTIL_FindEntityByClassname(pFlag, "item_flag_team1")) != NULL)
		{
			if (pFlag)
			{
				if (((CItemFlag *)pFlag)->Dropped)
					iDropped++;

				iFoundCount++;
			}
		}

		if (iFoundCount > 1 && iDropped == 1)
			iRedFlagStatus = RED_FLAG_DROPPED;
		else if (iFoundCount >= 1 && iDropped == 0)
			iRedFlagStatus = RED_FLAG_ATBASE;
	}

	iDropped = iFoundCount = 0;

	while ((pFlag = UTIL_FindEntityByClassname(pFlag, "carried_flag_team2")) != NULL)
	{
		if (pFlag && !FBitSet(pFlag->pev->flags, FL_KILLME))
			iFoundCount++;
	}

	if (iFoundCount >= 1)
		iBlueFlagStatus = BLUE_FLAG_STOLEN;

	if (!iFoundCount)
	{

		while ((pFlag = UTIL_FindEntityByClassname(pFlag, "item_flag_team2")) != NULL)
		{
			if (pFlag)
			{
				if (((CItemFlag *)pFlag)->Dropped)
					iDropped++;

				iFoundCount++;
			}
		}

		if (iFoundCount > 1 && iDropped == 1)
			iBlueFlagStatus = BLUE_FLAG_DROPPED;
		else if (iFoundCount >= 1 && iDropped == 0)
			iBlueFlagStatus = BLUE_FLAG_ATBASE;
	}

	if (pPlayer)
	{
		if (pPlayer->pev->team == 0)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgFlagStatus, NULL, pPlayer->edict());
			WRITE_BYTE(0);
			WRITE_BYTE(iRedFlagStatus);
			WRITE_BYTE(iBlueFlagStatus);
			WRITE_BYTE(iRedTeamScore);
			WRITE_BYTE(iBlueTeamScore);
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgFlagStatus, NULL, pPlayer->edict());
			WRITE_BYTE(1);
			WRITE_BYTE(iRedFlagStatus);
			WRITE_BYTE(iBlueFlagStatus);
			WRITE_BYTE(iRedTeamScore);
			WRITE_BYTE(iBlueTeamScore);
			MESSAGE_END();
		}

		pPlayer->m_flFlagStatusTime = 0.0;
	}
	else
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgFlagStatus, NULL);
		WRITE_BYTE(1);
		WRITE_BYTE(iRedFlagStatus);
		WRITE_BYTE(iBlueFlagStatus);
		WRITE_BYTE(iRedTeamScore);
		WRITE_BYTE(iBlueTeamScore);
		MESSAGE_END();

		m_flFlagStatusTime = 0.0;
	}

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *plr = UTIL_PlayerByIndex(i);
		if (plr)
		{
			if (((CBasePlayer *)plr)->m_bHasFlag)
			{
				MESSAGE_BEGIN(MSG_ALL, gmsgFlagCarrier, NULL);
				WRITE_BYTE(plr->entindex());
				WRITE_BYTE(1);
				MESSAGE_END();
			}
			else
			{
				MESSAGE_BEGIN(MSG_ALL, gmsgFlagCarrier, NULL);
				WRITE_BYTE(plr->entindex());
				WRITE_BYTE(0);
				MESSAGE_END();
			}
		}
	}
}


//=========================================================
//=========================================================
void CCTFMultiplay::RecountTeams(void)
{
	char	*pName;
	char	teamlist[TEAMPLAY_TEAMLISTLENGTH];

	// loop through all teams, recounting everything
	num_teams = 0;

	// Copy all of the teams from the teamlist
	// make a copy because strtok is destructive
	strcpy(teamlist, m_szTeamList);
	pName = teamlist;
	pName = strtok(pName, ";");
	while (pName != NULL && *pName)
	{
		if (GetTeamIndex(pName) < 0)
		{
			strcpy(team_names[num_teams], pName);
			num_teams++;
		}
		pName = strtok(NULL, ";");
	}

	if (num_teams < 2)
	{
		num_teams = 0;
		m_teamLimit = FALSE;
	}

	// Sanity check
	memset(team_scores, 0, sizeof(team_scores));

	// loop through all clients
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *plr = UTIL_PlayerByIndex(i);

		if (plr)
		{
			const char *pTeamName = plr->TeamID();
			// try add to existing team
			int tm = GetTeamIndex(pTeamName);

			if (tm < 0) // no team match found
			{
				if (!m_teamLimit)
				{
					// add to new team
					tm = num_teams;
					num_teams++;
					team_scores[tm] = 0;
					strncpy(team_names[tm], pTeamName, MAX_TEAMNAME_LENGTH);
				}
			}

			if (tm >= 0)
			{
				team_scores[tm] += plr->pev->frags;
			}
		}
	}
}
