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
// teamplay_gamerules.cpp
//

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"items.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;

//=========================================================
//=========================================================
CHalfLifeRules::CHalfLifeRules( void )
{
	SERVER_COMMAND( "exec spserver.cfg\n" );
	RefreshSkillData();
}

//=========================================================
//=========================================================
void CHalfLifeRules::Think( void )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsMultiplayer( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsDeathmatch( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsCoOp( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if( !pPlayer->m_iAutoWepSwitch )
	{
		return FALSE;
	}

	if( pPlayer->m_iAutoWepSwitch == 2
	    && pPlayer->m_afButtonLast & ( IN_ATTACK | IN_ATTACK2 ) )
	{
		return FALSE;
	}

	if( !pPlayer->m_pActiveItem->CanHolster() )
	{
		return FALSE;
	}

	return TRUE;
}

//=========================================================
//=========================================================
BOOL HLGetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{
	CBasePlayerItem *pCheck;
	CBasePlayerItem *pBest;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight;
	int i;

	iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	if( !pCurrentWeapon->CanHolster() )
	{
		// can't put this gun away right now, so can't switch.
		return FALSE;
	}

	for( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		pCheck = pPlayer->m_rgpPlayerItems[i];

		while( pCheck )
		{
			if( !FBitSet( pCheck->iFlags(), ITEM_FLAG_NOAUTOSWITCHTO ))
			{
				if( pCheck->iWeight() > -1 && pCheck->iWeight() == pCurrentWeapon->iWeight() && pCheck != pCurrentWeapon )
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
				else if( pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
				{
					//ALERT ( at_console, "Considering %s\n", STRING( pCheck->pev->classname ) );
					// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
					// that the player was using. This will end up leaving the player with his heaviest-weighted
					// weapon.
					if( pCheck->CanDeploy() )
					{
						// if this weapon is useable, flag it as the best
						iBestWeight = pCheck->iWeight();
						pBest = pCheck;
					}
				}
			}

			pCheck = pCheck->m_pNext;
		}
	}

	// if we make it here, we've checked all the weapons and found no useable
	// weapon in the same catagory as the current weapon.

	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always
	// at least get the crowbar, but ya never know.
	if( !pBest )
	{
		return FALSE;
	}

	pPlayer->SwitchWeapon( pBest );

	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{
	if( pCurrentWeapon && FBitSet( pCurrentWeapon->iFlags(), ITEM_FLAG_EXHAUSTIBLE ))
		return HLGetNextBestWeapon( pPlayer, pCurrentWeapon );
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] )
{
	return TRUE;
}

void CHalfLifeRules::InitHUD( CBasePlayer *pl )
{
}

//=========================================================
//=========================================================
void CHalfLifeRules::ClientDisconnected( edict_t *pClient )
{
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerSpawn( CBasePlayer *pPlayer )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::AllowAutoTargetCrosshair( void )
{
	return ( g_iSkillLevel == SKILL_EASY );
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerThink( CBasePlayer *pPlayer )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeRules::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	DeathNotice( pVictim, pKiller, pInflictor, 0);
}

//=========================================================
// Deathnotice
//=========================================================
void CHalfLifeRules::DeathNotice( CBaseEntity *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int type )
{
	if(CVAR_GET_FLOAT( "cl_deathnotice" ) <= 0){
	return;
	}
// Work out what killed the player, and send a message to all clients about it
	CBaseEntity *Killer = CBaseEntity::Instance( pKiller );

	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	const char *victim_name = "";
	const char *killer_name = "";

	int killer_color = 0;
	int victim_color = 0;
	//killer_weapon_name = STRING( pInflictor->classname );
	
	victim_name = STRING( pVictim->pev->netname );
	//killer_name = STRING( pInflictor->classname );

		if(type & DMG_UNKNOWBLAST){
			killer_weapon_name = "mortar";
		}
		else if(type & DMG_ENERGYBEAM){
			killer_weapon_name = "energy";
		}
		else if(type & DMG_ENERGYBLAST){
			killer_weapon_name = "energyblast";
		}
		else if(type & DMG_BURN){
			killer_weapon_name = "burn";
		}
		else if(type & DMG_FREEZE){
			killer_weapon_name = "freeze";
		}
		else if(type & DMG_BLAST){
			killer_weapon_name = "blast";
		}
		else if(type & DMG_DARK){
			killer_weapon_name = "dark";
		}
		else if(type & DMG_ACID){
			killer_weapon_name = "acid";
		}
		else if(type & DMG_CRUSH){
			killer_weapon_name = "crush";
		}
		else if(type & DMG_SONIC){
			killer_weapon_name = "sonic";
		}
		else if(type & DMG_SLASH){
			killer_weapon_name = "slash";
		}
		else if(type & DMG_CONCUSSION){
			killer_weapon_name = "concussion";
		}
		else if(type & DMG_FALL){
			killer_weapon_name = "skull";
		}

	if(Killer != NULL && Killer != pVictim){

		if ( (Killer->pev->flags & FL_MONSTER) ){
			if(Killer->Classify() == CLASS_PLAYER_ALLY || Killer->Classify() == CLASS_MACHINE_BLACK){
				killer_color = 1;//�����
				if(type == DMG_SHOCK){
				killer_weapon_name = "shock";
				}
				else if(type & DMG_SLASH){
				killer_weapon_name = "cleave";
				}
			}
			else{
				if(type == DMG_SHOCK){
				killer_weapon_name = "electric";
				}
				else if(type & DMG_ENERGYBEAM){
				killer_weapon_name = "beam";
				}
				killer_color = 2;//�����
			}
			killer_name = STRING( Killer->pev->netname );
		}
		else if ( (Killer->pev->flags & FL_CLIENT) ){
			killer_name = "Kadoma";
			killer_color = 3;

			if(type == DMG_CRUSH){
			killer_weapon_name = "vehicle";
			}
			else if(type & DMG_MORTAR){
			killer_weapon_name = "mortar";
			}
			else if(type & DMG_VALVE_SWORD){
			killer_weapon_name = "holysword";
			}
			else if(type & DMG_AIR){
			killer_weapon_name = "fist";
			}
		}

	}

	if ( (pVictim->pev->flags & FL_CLIENT) ){
		victim_name = "Kadoma";
		victim_color = 3;
	}

	if ( (pVictim->pev->flags & FL_MONSTER) ){
			if(pVictim->Classify() == CLASS_PLAYER_ALLY || pVictim->Classify() == CLASS_MACHINE_BLACK){
			victim_color = 1;//�����
			}
			else{
			victim_color = 2;//�����
			}
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if ( strncmp( killer_name, "weapon_", 7 ) == 0 )
		killer_name += 7;
	else if ( strncmp( killer_name, "monster_", 8 ) == 0 )
		killer_name += 8;
	else if ( strncmp( killer_name, "func_", 5 ) == 0 )
		killer_name += 5;
	else if ( strncmp( killer_name, "env_", 4 ) == 0 )
		killer_name += 4;


	MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
		WRITE_BYTE( 0 );						// the killer
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );		// the victim
		WRITE_STRING( killer_weapon_name );		// what they were killed by (should this be a string?)
		WRITE_STRING( victim_name );
		WRITE_STRING( killer_name );
		WRITE_BYTE( killer_color );
		WRITE_BYTE( victim_color );
	MESSAGE_END();

	UTIL_LogPrintf( "\"%s\" killed with \"%s\"\n",
	STRING( pVictim->pev->netname ),
	killer_name );		
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeRules::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeRules::FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	return -1;
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeRules::FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeRules::WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	return GR_WEAPON_RESPAWN_NO;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::ItemShouldRespawn( CItem *pItem )
{
	return GR_ITEM_RESPAWN_NO;
}

//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeRules::FlItemRespawnTime( CItem *pItem )
{
	return -1;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	return GR_AMMO_RESPAWN_NO;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return -1;
}

//=========================================================
//=========================================================
Vector CHalfLifeRules::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlHealthChargerRechargeTime( void )
{
	return 0;// don't recharge
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// why would a single player in half life need this? 
	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::FAllowMonsters( void )
{
	return TRUE;
}
