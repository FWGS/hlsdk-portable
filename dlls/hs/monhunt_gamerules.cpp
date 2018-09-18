/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//
// monhunt_gamerules.cpp
//
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#ifndef NO_VOICEGAMEMGR
#include	"voice_gamemgr.h"
#endif
#include	"monhunt_gamerules.h"
#include	"game.h"
//#include	"mp3.h"

extern DLL_GLOBAL BOOL		g_fGameOver;
extern int gmsgScoreInfo;
extern int gmsgPlayMP3; //AJH - Killars MP3player
extern int gmsgDeathMsg;

extern cvar_t timeleft, fragsleft;

extern cvar_t mp_chattime;

void CMonsterplay::Think( void )
{
	// longest the intermission can last, in seconds
	#define MAX_INTERMISSION_TIME		120
#ifndef NO_VOICEGAMEMGR
	CVoiceGameMgr	g_VoiceGameMgr;
#endif
	float g_flIntermissionStartTime = 0;
#ifndef NO_VOICEGAMEMGR
	g_VoiceGameMgr.Update(gpGlobals->frametime);
#endif
	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	int ltime = 0; //Check to see if the times have changed

	if ( g_fGameOver )   // someone else quit the game already
	{
		// bounds check
		int time = (int)CVAR_GET_FLOAT( "mp_chattime" );
		if ( time < 1 )
			CVAR_SET_STRING( "mp_chattime", "1" );
		else if ( time > MAX_INTERMISSION_TIME )
			CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );

		m_flIntermissionEndTime = g_flIntermissionStartTime + mp_chattime.value;

		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->time )
		{
			if ( m_iEndIntermissionButtonHit  // check that someone has pressed a key, or the max intermission time is over
				|| ( ( g_flIntermissionStartTime + MAX_INTERMISSION_TIME ) < gpGlobals->time) ) 
				ChangeLevel(); // intermission is over
		}

		return;
	}

	float flTimeLimit = timelimit.value * 60;
	float flFragLimit = fraglimit.value;

	time_remaining = (int)(flTimeLimit ? ( flTimeLimit - gpGlobals->time ) : 0);

	if ( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
	{
		GoToIntermission();
		return;
	}

	if ( flFragLimit )
	{
		int bestfrags = 9999;
		int remain;

		// check if any player is over the frag limit
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && pPlayer->pev->frags >= flFragLimit )
			{
				GoToIntermission();
				return;
			}


			if ( pPlayer )
			{
				remain = flFragLimit - pPlayer->pev->frags;
				if ( remain < bestfrags )
				{
					bestfrags = remain;
				}
			}

		}
		frags_remaining = bestfrags;
	}

	// Updates when frags change
	if ( frags_remaining != last_frags )
	{
		g_engfuncs.pfnCvar_DirectSet( &fragsleft, UTIL_VarArgs( "%i", frags_remaining ) );
	}

	// Updates once per second
	if ( timeleft.value != last_time )
	{
		g_engfuncs.pfnCvar_DirectSet( &timeleft, UTIL_VarArgs( "%i", time_remaining ) );
	}

	last_frags = frags_remaining;
	last_time  = time_remaining;

	//fMonHuntNextSpawn = gpGlobals->time + 6;

	/*if (bMonsterCheck < 1)
	{
		CBaseEntity *pMonsterCheck;
		pMonsterCheck = UTIL_FindEntityByClassname(NULL, "monstermaker");
		if ( !FNullEnt( pMonsterCheck->edict() ))
			bMonsterMakers = true;

		fMonHuntNextSpawn = gpGlobals->time + 6;
		UTIL_SayTextAllHS( "MONSTER CHECKED" );
		bMonsterCheck = 1;
	}*/

	//If there is no Monster Makers.
	//if (/*!bMonsterMakers && (*/gpGlobals->time > fMonHuntNextSpawn/*)*/)
	/*{
		CBaseEntity *pMonsterSpawner;
		pMonsterSpawner = UTIL_FindEntityByClassname(NULL, "info_player_deathmatch");

		CBaseEntity *pMonster = CBaseEntity::Create("monster_barney", pMonsterSpawner->pev->origin, pMonsterSpawner->pev->angles);

		UTIL_SayTextAllHS( "CALLED" );

		fMonHuntNextSpawn = gpGlobals->time + 6; //TODO: CVAR SET FUCK
	}*/

		//if ( RANDOM_LONG(0,10) > 9)
		//{
		//	CBaseEntity *pMonsterSpawner;

		//	for ( int i = RANDOM_LONG(1,5); i > 0; i-- )
		//	pMonsterSpawner = UTIL_FindEntityByClassname(NULL, "info_player_deathmatch");

		//	CBaseEntity *pMonster = CBaseEntity::Create("monster_barney", pMonsterSpawner->pev->origin, pMonsterSpawner->pev->angles);

		//	UTIL_SayTextAllHS( "CALLED" );

		//	fMonHuntNextSpawn = gpGlobals->time + 6; //TODO: CVAR SET FUCK
		//}
}

CMonsterplay :: CMonsterplay()
{
 //Genuflect
}

BOOL CMonsterplay::IsMonster()
{
 return TRUE;
}

void CMonsterplay::PlayerSpawn( CBasePlayer *pPlayer )
{
	BOOL		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

	pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
	
	addDefault = TRUE;
	
	//entvars_t *pev = &pEntity->v;

//	CBasePlayer	*pPlayer;

	edict_t *pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );

	while( ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ) ) )
	{
		pWeaponEntity->Touch( pPlayer );
		addDefault = FALSE;
	}

	if ( addDefault )
	{
		pPlayer->GiveNamedItem( "weapon_fotn" );
		pPlayer->GiveNamedItem( "weapon_9mmhandgun" );
		pPlayer->GiveNamedItem( "ammo_9mmclip" );
		pPlayer->GiveNamedItem( "weapon_shotgun" );
		pPlayer->GiveNamedItem( "ammo_buckshot" );
		pPlayer->GiveNamedItem( "weapon_9mmAR" );
		pPlayer->GiveNamedItem( "ammo_9mmAR" );
		pPlayer->GiveNamedItem( "ammo_ARgrenades" );
		pPlayer->GiveNamedItem( "weapon_handgrenade" );
		pPlayer->GiveNamedItem( "weapon_tripmine" );
		pPlayer->GiveNamedItem( "weapon_rpg" );
		pPlayer->GiveNamedItem( "ammo_rpgclip" );
		pPlayer->GiveNamedItem( "weapon_satchel" );
		pPlayer->GiveNamedItem( "weapon_snark" );
		pPlayer->GiveNamedItem( "weapon_soda" );
		pPlayer->GiveNamedItem( "weapon_dosh" );
		pPlayer->GiveNamedItem( "weapon_beamkatana" );
		pPlayer->GiveNamedItem( "weapon_ak47" );
		pPlayer->GiveNamedItem( "weapon_bow" );
		pPlayer->GiveNamedItem( "weapon_jason" );
		pPlayer->GiveNamedItem( "weapon_jihad" );
		pPlayer->GiveNamedItem( "weapon_jackal" );
		pPlayer->GiveNamedItem( "weapon_nstar" );
		pPlayer->GiveNamedItem( "weapon_mw2" );
		pPlayer->GiveNamedItem( "weapon_zapper" );
		pPlayer->GiveNamedItem( "weapon_goldengun" );
		pPlayer->GiveNamedItem( "weapon_boombox" );
		pPlayer->GiveNamedItem( "weapon_scientist" );
		pPlayer->GiveNamedItem( "weapon_modman" );
	}
}

void CMonsterplay::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
		CBaseEntity *pKira = CBaseEntity::Instance(pKiller);

		if (pKira->Classify() != CLASS_PLAYER && pKira->pev->flags & FL_MONSTER)
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
				WRITE_BYTE( -1 );																			// the killer
				WRITE_BYTE( ENTINDEX(pVictim->edict()) );													// the victim
				WRITE_STRING( PrepareMonsterName( STRING(pKiller->classname) ) );							// what they were killed by (should this be a string?)
			MESSAGE_END();

			pVictim->m_iDeaths += 1;

			// update the scores
			// killed scores
			MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
				WRITE_BYTE( ENTINDEX(pVictim->edict()) );
				WRITE_SHORT( pVictim->pev->frags );
				WRITE_SHORT( pVictim->m_iDeaths );
				WRITE_SHORT( 0 );
				WRITE_SHORT( GetTeamIndex( pVictim->m_szTeamName ) + 1 );
				WRITE_SHORT( pVictim->m_fHSDev );
			MESSAGE_END();
		}
		else
			return CHalfLifeMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);
}

int CMonsterplay::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() )
		return GR_NOTTEAMMATE;

	// monster hunt has only teammates
	return GR_TEAMMATE;
}

BOOL CMonsterplay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if ( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		// my teammate hit me.
		if ( (friendlyfire.value == 0) && (pAttacker != pPlayer) )
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return FALSE;
		}
	}

	return CHalfLifeMultiplay::FPlayerCanTakeDamage( pPlayer, pAttacker );
}

BOOL CMonsterplay::FAllowMonsters( void )
{
		return TRUE;
}

int CMonsterplay::iKillforMonster(const char *classname)
{
	if ( !strcmp( classname, "Gay Glenn" ) )
		return 1;
	else if ( !strcmp( classname, "Barney" ) )
		return 2;
	else if ( !strcmp( classname, "X-Mas Tree" ) )
		return 5;
	else if ( !strcmp( classname, "Scientist" ) )
		return 1;
	else if ( !strcmp( classname, "Sinistar" ) )
		return 3;
	else if ( !strcmp( classname, "Chris-Chan" ) )
		return 0;
	else if ( !strcmp( classname, "Zombie" ) )
		return 2;
	else if ( !strcmp( classname, "snark" ) )
		return 0;
	else
		return 1;
}

//What for? To strip "monster_" and give it a proper name.
const char *CMonsterplay::PrepareMonsterName( const char *monster_name )
{
	// Hack to fix name change
			const char *gayglenn = "Gay Glenn";
			const char *barney = "Barney";
			const char *scientist = "Scientist";
			const char *xmast = "X-Mas Tree";
			const char *sinistar = "Sinistar";
			const char *cwc = "Chris-Chan";
			const char *zombie = "Zombie";
			const char *controller = "Alien Controller";
			const char *bullsquid = "Bull Squid";
			const char *grinman = "Grinman";

			monster_name += 8;

			// replace the code names with the 'real' names
			if ( !strcmp( monster_name, "gay" ) )
				monster_name = gayglenn;
			else if ( !strcmp( monster_name, "barney" ) )
				monster_name = barney;
			else if ( !strcmp( monster_name, "xmast" ) )
				monster_name = xmast;
			else if ( !strcmp( monster_name, "scientist" ) )
				monster_name = scientist;
			else if ( !strcmp( monster_name, "sinistar" ) )
				monster_name = sinistar;
			else if ( !strcmp( monster_name, "chrischan" ) )
				monster_name = cwc;
			else if ( !strcmp( monster_name, "zombie" ) )
				monster_name = zombie;
			else if ( !strcmp( monster_name, "alien_controller") )
				monster_name = controller;
			else if ( !strcmp( monster_name, "bullsquid") )
				monster_name = bullsquid;
			else if ( !strcmp( monster_name, "grinman") )
				monster_name = grinman;

			return monster_name;
}
