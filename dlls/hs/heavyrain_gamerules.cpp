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
// heavyrain_gamerules.cpp
//
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"heavyrain_gamerules.h"
#include	"game.h"

extern DLL_GLOBAL BOOL		g_fGameOver;
extern int gmsgScoreInfo;
extern int gmsgPlayMP3; //AJH - Killars MP3player
extern int gmsgStopMP3;

CHeavyRainplay :: CHeavyRainplay()
{
 //Genuflect
}

BOOL CHeavyRainplay::IsHeavyRain()
{
	return TRUE;
}

void CHeavyRainplay::PlayerSpawn( CBasePlayer *pPlayer )
{
	BOOL		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

	pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
	
	addDefault = TRUE;

	edict_t *pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );

	while( ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ) ) )
	{
		pWeaponEntity->Touch( pPlayer );
		addDefault = FALSE;
	}

	if ( addDefault )
	{
		pPlayer->GiveNamedItem( "weapon_jason" );
		pPlayer->GiveNamedItem( "weapon_goldengun" );
		pPlayer->GiveAmmo( 15, "gold", GOLDENGUN_MAX_CARRY );
		MESSAGE_BEGIN( MSG_ONE, gmsgPlayMP3, NULL, pPlayer->edict() );
			WRITE_STRING( "media/jayson.mp3" );
		MESSAGE_END();
	}
}

void CHeavyRainplay::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if ( pVictim->pev == pKiller )  
	{  // killed self
		pVictim->pev->frags -= 10;
		UTIL_SayText( "You sacrificed your life for Jason\nYou lose 10 Jasons for revival costs.\n", pVictim );
		return;
	}
	else if ( ktmp && ktmp->IsPlayer() )
	{
		int FUCK = pVictim->pev->frags;
		pKiller->frags += JasonsStolen(FUCK);
		pVictim->pev->frags -= JasonsStolen(FUCK);

		UTIL_SayText( "You lose your Jasons to the killer.\nKILL HIM AND GET THEM BACK! ;)\n", pVictim );

		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX(pVictim->edict()) );
			WRITE_SHORT( pVictim->pev->frags );
			WRITE_SHORT( pVictim->m_iDeaths );
			WRITE_SHORT( 0 );
			WRITE_SHORT( g_pGameRules->GetTeamIndex( pVictim->m_szTeamName ) + 1 );
			WRITE_SHORT( pVictim->m_fHSDev );
		MESSAGE_END();

	CBaseEntity *ep = CBaseEntity::Instance( pKiller );
	if ( ep && ep->Classify() == CLASS_PLAYER )
	{
		CBasePlayer *PK = (CBasePlayer*)ep;

		UTIL_SayText( "You stole your victim's Jasons and it is now added to your score.\nNo doubt hes out for blood, watch out ;)\n", PK );

		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX(PK->edict()) );
			WRITE_SHORT( PK->pev->frags );
			WRITE_SHORT( PK->m_iDeaths );
			WRITE_SHORT( 0 );
			WRITE_SHORT( GetTeamIndex( PK->m_szTeamName) + 1 );
			WRITE_SHORT( PK->m_fHSDev );
		MESSAGE_END();

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
	}
	else
	{
		// World did them in, Genuflect.
	}
	}
}

int CHeavyRainplay::JasonsStolen(int jason)
{
	return jason; //JASON! I found you!
}
