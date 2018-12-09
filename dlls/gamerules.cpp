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
//=========================================================
// GameRules.cpp
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"teamplay_gamerules.h"
#include	"skill.h"
#include	"game.h"

extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

DLL_GLOBAL CGameRules *g_pGameRules = NULL;
extern DLL_GLOBAL BOOL g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgMOTD;

int g_teamplay = 0;

//=========================================================
//=========================================================
BOOL CGameRules::CanHaveAmmo( CBasePlayer *pPlayer, const char *pszAmmoName, int iMaxCarry )
{
	int iAmmoIndex;

	if( pszAmmoName )
	{
		iAmmoIndex = pPlayer->GetAmmoIndex( pszAmmoName );

		if( iAmmoIndex > -1 )
		{
			if( pPlayer->AmmoInventory( iAmmoIndex ) < iMaxCarry )
			{
				// player has room for more of this type of ammo
				return TRUE;
			}
		}
	}

	return FALSE;
}

//=========================================================
//=========================================================
edict_t *CGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = EntSelectSpawnPoint( pPlayer );

	pPlayer->pev->origin = VARS( pentSpawnSpot )->origin + Vector( 0, 0, 1 );
	pPlayer->pev->v_angle  = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = VARS( pentSpawnSpot )->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = TRUE;

	return pentSpawnSpot;
}

//=========================================================
//=========================================================
BOOL CGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	// only living players can have items
	if( pPlayer->pev->deadflag != DEAD_NO )
		return FALSE;

	if( pWeapon->pszAmmo1() )
	{
		if( !CanHaveAmmo( pPlayer, pWeapon->pszAmmo1(), pWeapon->iMaxAmmo1() ) )
		{
			// we can't carry anymore ammo for this gun. We can only 
			// have the gun if we aren't already carrying one of this type
			if( pPlayer->HasPlayerItem( pWeapon ) )
			{
				return FALSE;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if( pPlayer->HasPlayerItem( pWeapon ) )
		{
			return FALSE;
		}
	}

	// note: will fall through to here if GetItemInfo doesn't fill the struct!
	return TRUE;
}

//=========================================================
// load the SkillData struct with the proper values based on the skill level.
//=========================================================
void CGameRules::RefreshSkillData ( void )
{
	//Turret
	gSkillData.turretHealth = 1000.0f;

	// MiniTurret
	gSkillData.miniturretHealth = 1000.0f;

	// Sentry Turret
	gSkillData.sentryHealth = 1000.0f;

	// PLAYER WEAPONS

	// Crowbar whack
	gSkillData.plrDmgCrowbar = 25.0f;

	// Glock Round
	gSkillData.plrDmg9MM = 12.0f;

	// 357 Round
	gSkillData.plrDmg357 = 40.0f;

	// MP5 Round
	gSkillData.plrDmgMP5 = 12.0f;

	// M203 grenade
	gSkillData.plrDmgM203Grenade = 100.0f;

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = 20.0f;

	// Crossbow
	gSkillData.plrDmgCrossbowClient = 20.0f;

	// RPG
	gSkillData.plrDmgRPG = 120.0f;

	// Egon Gun
	gSkillData.plrDmgEgonNarrow = 10.0f;
	gSkillData.plrDmgEgonWide = 20.0f;

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = 100.0f;

	// Satchel Charge
	gSkillData.plrDmgSatchel = 120.0f;

	// Tripmine
	gSkillData.plrDmgTripmine = 150.0f;

	// MONSTER WEAPONS
	gSkillData.monDmg12MM = 8.0f;
	gSkillData.monDmgMP5 = 3.0f;
	gSkillData.monDmg9MM = 5.0f;

	// PLAYER HORNET
	gSkillData.plrDmgHornet = 10.0f;

	// HEALTH/CHARGE
	gSkillData.suitchargerCapacity = 75.0f;
	gSkillData.batteryCapacity = 15.0f;
	gSkillData.healthchargerCapacity = 15.0f;
	gSkillData.healthkitCapacity = 25.0f;

	// monster damage adj
	gSkillData.monHead = 3.0f;
	gSkillData.monChest = 1.0f;
	gSkillData.monStomach = 1.0f;
	gSkillData.monLeg = 1.0f;
	gSkillData.monArm = 1.0f;

	// player damage adj
	gSkillData.plrHead = 3.0f;
	gSkillData.plrChest = 1.0f;
	gSkillData.plrStomach = 1.0f;
	gSkillData.plrLeg = 1.0f;
	gSkillData.plrArm = 1.0f;
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
/*
CGameRules *InstallGameRules( void )
{
	SERVER_COMMAND( "exec game.cfg\n" );
	SERVER_EXECUTE();

	if( !gpGlobals->deathmatch )
	{
		// generic half-life
		g_teamplay = 0;
		return new CHalfLifeRules;
	}
	else
	{
		if( teamplay.value > 0 )
		{
			// teamplay
			g_teamplay = 1;
			return new CHalfLifeTeamplay;
		}
		if( (int)gpGlobals->deathmatch == 1 )
		{
			// vanilla deathmatch
			g_teamplay = 0;
			return new CHalfLifeMultiplay;
		}
		else
		{
			// vanilla deathmatch??
			g_teamplay = 0;
			return new CHalfLifeMultiplay;
		}
	}
}*/
