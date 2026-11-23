/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// teamplay_gamerules.cpp
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "skill.h"
#include "items.h"

CHalfLifeRules::CHalfLifeRules()
{
	RefreshSkillData();
	m_iHostagesRescued = 0;
	m_iHostagesTouched = 0;
	m_bMapHasRescueZone = false;
	m_fTeamCount = 0;
	m_iAccountCT = 0;
	m_iAccountTerrorist = 0;
}

//=========================================================
//=========================================================
void CHalfLifeRules::Think()
{
	CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( 1 );
	if ( pPlayer )
		pPlayer->ResetMaxSpeed();
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsMultiplayer()
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsDeathmatch()
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsCoOp()
{
	return FALSE;
}

BOOL CHalfLifeRules::FShouldSwitchWeapon( CBasePlayer* pPlayer, CBasePlayerItem* )
{
	if ( !pPlayer->m_pActiveItem )
		return TRUE;
	if ( !pPlayer->m_pActiveItem->CanHolster() )
		return FALSE;
	return CVAR_GET_FLOAT( "autoswitch" ) != 0;
}

BOOL CHalfLifeRules::GetNextBestWeapon( CBasePlayer* pPlayer, CBasePlayerItem* pCurrentWeapon )
{
	if ( !pCurrentWeapon->CanHolster() )
		return FALSE;

	if ( pPlayer->SelectLastItem() && pPlayer->m_pActiveItem->IsWeapon()
		&& ( (CBasePlayerWeapon*)pPlayer->m_pActiveItem )->m_iClip > 0 )
		return TRUE;

	int iBestWeight = -1;
	CBasePlayerItem* pBest = NULL;
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		for ( CBasePlayerItem* pItem = pPlayer->m_rgpPlayerItems[i]; pItem; pItem = pItem->m_pNext )
		{
			if ( pItem->iWeight() > iBestWeight && pItem != pCurrentWeapon && pItem->CanDeploy()
				&& ( (CBasePlayerWeapon*)pItem )->m_iClip > 0 )
			{
				iBestWeight = pItem->iWeight();
				pBest = pItem;
			}
		}
	}
	if ( !pBest )
		return FALSE;

	pPlayer->SwitchWeapon( pBest );
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::ClientConnected( edict_t*, const char*, const char*, char[128] )
{
	return TRUE;
}

void CHalfLifeRules::InitHUD( CBasePlayer* )
{
}

//=========================================================
//=========================================================
void CHalfLifeRules::ClientDisconnected( edict_t* )
{
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerFallDamage( CBasePlayer* pPlayer )
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * 100.0f / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED );
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerSpawn( CBasePlayer* pPlayer )
{
	pPlayer->m_bFirstConnected = true;
}

BOOL CHalfLifeRules::AllowAutoTargetCrosshair()
{
	return g_iSkillLevel == SKILL_EASY;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerThink( CBasePlayer* )
{
}


//=========================================================
//=========================================================
BOOL CHalfLifeRules::FPlayerCanRespawn( CBasePlayer* )
{
	return TRUE;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerSpawnTime( CBasePlayer* )
{
	return gpGlobals->time; //now!
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeRules::IPointsForKill( CBasePlayer*, CBasePlayer* )
{
	return 1;
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeRules::PlayerKilled( CBasePlayer*, entvars_t*, entvars_t* )
{
}

//=========================================================
// Deathnotice
//=========================================================
void CHalfLifeRules::DeathNotice( CBasePlayer*, entvars_t*, entvars_t* )
{
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeRules::PlayerGotWeapon( CBasePlayer*, CBasePlayerItem* )
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeRules::FlWeaponRespawnTime( CBasePlayerItem* )
{
	return -1;
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeRules::FlWeaponTryRespawn( CBasePlayerItem* )
{
	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecWeaponRespawnSpot( CBasePlayerItem* pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeRules::WeaponShouldRespawn( CBasePlayerItem* )
{
	return GR_WEAPON_RESPAWN_NO;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::CanHaveItem( CBasePlayer*, CItem* )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotItem( CBasePlayer*, CItem* )
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::ItemShouldRespawn( CItem* )
{
	return GR_ITEM_RESPAWN_NO;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeRules::FlItemRespawnTime( CItem* )
{
	return -1;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecItemRespawnSpot( CItem* pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsAllowedToSpawn( CBaseEntity* )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotAmmo( CBasePlayer*, char*, int )
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::AmmoShouldRespawn( CBasePlayerAmmo* )
{
	return GR_AMMO_RESPAWN_NO;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlAmmoRespawnTime( CBasePlayerAmmo* )
{
	return -1;
}

//=========================================================
//=========================================================
Vector CHalfLifeRules::VecAmmoRespawnSpot( CBasePlayerAmmo* pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlHealthChargerRechargeTime()
{
	return 0; // don't recharge
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerWeapons( CBasePlayer* )
{
	return GR_PLR_DROP_GUN_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerAmmo( CBasePlayer* )
{
	return GR_PLR_DROP_AMMO_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::PlayerRelationship( CBaseEntity*, CBaseEntity* )
{
	// why would a single player in half life need this? 
	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::FAllowMonsters()
{
	return TRUE;
}
