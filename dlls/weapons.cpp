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
/*

===== weapons.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"

ItemInfo CBasePlayerItem::ItemInfoArray[MAX_WEAPONS];
AmmoInfo CBasePlayerItem::AmmoInfoArray[MAX_AMMO_SLOTS];
MULTIDAMAGE gMultiDamage;
int giAmmoIndex;
short g_sModelIndexLaser;
short g_sModelIndexLaserDot;
short g_sModelIndexFireball;
short g_sModelIndexFireball2;
short g_sModelIndexFireball3;
short g_sModelIndexFireball4;
short g_sModelIndexSmoke;
short g_sModelIndexWExplosion;
short g_sModelIndexBubbles;
short g_sModelIndexBloodDrop;
short g_sModelIndexBloodSpray;
short g_sModelIndexC4Glow;
const char* g_pModelNameLaser = "sprites/laserbeam.spr";

extern int gEvilImpulse101;
extern int gmsgCurWeapon;


//*********************************************************
// weaponbox code:
//*********************************************************

LINK_ENTITY_TO_CLASS( weaponbox, CWeaponBox );

TYPEDESCRIPTION CBasePlayerItem::m_SaveData[] =
{
	DEFINE_FIELD( CBasePlayerItem, m_pPlayer, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayerItem, m_pNext, FIELD_CLASSPTR ),
	//DEFINE_FIELD( CBasePlayerItem, m_fKnown, FIELD_INTEGER ),Reset to zero on load
	DEFINE_FIELD( CBasePlayerItem, m_iId, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerItem, m_bIsAccessory, FIELD_BOOLEAN ),
	// DEFINE_FIELD( CBasePlayerItem, m_iIdPrimary, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayerItem, m_iIdSecondary, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CBasePlayerItem, CBaseAnimating );

TYPEDESCRIPTION CBasePlayerWeapon::m_SaveData[] =
{
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iPrimaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iSecondaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iClip, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iDefaultAmmo, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iClientClip, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iClientWeaponState, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_fInReload, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_fInSpecialReload, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iShellId, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_bDelayFire, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iDirection, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_bSecondarySilencerOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flAccuracy, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flLastFire, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iShotsFired, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_vVecAiming, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayerWeapon, model_name, FIELD_STRING ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flGlock18Shoot, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iGlock18ShotsFired, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flFamasShoot, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iFamasShotsFired, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_fBurstSpread, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iWeaponState, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextReload, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flDecreaseShotsFired, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iPlayEmptySound, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_fFireOnEmpty, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_usFireGlock18, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_usFireFamas, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_WeaponState, FIELD_INTEGER ),
};

int CBasePlayerWeapon::Save( CSave& save )
{
	if ( !CBasePlayerItem::Save( save ) )
		return 0;
	return save.WriteFields( "CBasePlayerWeapon", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
}

int CBasePlayerWeapon::Restore( CRestore& restore )
{
	if ( !CBasePlayerItem::Restore( restore ) )
		return 0;
	int result = restore.ReadFields( "CBasePlayerWeapon", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
	CBasePlayerWeapon::Spawn();
	return result;
}

TYPEDESCRIPTION CWeaponBox::m_SaveData[] =
{
	DEFINE_ARRAY( CWeaponBox, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS ),
	DEFINE_ARRAY( CWeaponBox, m_rgiszAmmo, FIELD_STRING, MAX_AMMO_SLOTS ),
	DEFINE_ARRAY( CWeaponBox, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES ),
	DEFINE_FIELD( CWeaponBox, m_cAmmoTypes, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CWeaponBox, CBaseEntity );

void ClearMultiDamage()
{
	gMultiDamage.pEntity = NULL;
	gMultiDamage.amount = 0;
	gMultiDamage.type = 0;
}

void ApplyMultiDamage( entvars_t* inflictor, entvars_t* attacker )
{
	if ( gMultiDamage.pEntity )
		gMultiDamage.pEntity->TakeDamage( inflictor, attacker, gMultiDamage.amount, gMultiDamage.type );
}


// GLOBALS USED:
//		gMultiDamage

void AddMultiDamage( entvars_t* inflictor, CBaseEntity* entity, float damage, int damageType )
{
	if ( !entity )
		return;
	gMultiDamage.type |= damageType;
	if ( entity != gMultiDamage.pEntity )
	{
		ApplyMultiDamage( inflictor, inflictor );
		gMultiDamage.pEntity = entity;
		gMultiDamage.amount = 0;
	}
	gMultiDamage.amount += damage;
}

/*
================
SpawnBlood
================
*/
void SpawnBlood( Vector spot, int bloodColor, float damage )
{
	UTIL_BloodDrips( spot, g_vecAttackDir, bloodColor, (int)damage );
}

int DamageDecal( CBaseEntity* entity, int damageType )
{
	return entity ? entity->DamageDecal( damageType ) : RANDOM_LONG( 0, 4 );
}

void EjectBrass( const Vector& origin, const Vector& velocity, float rotation, int model, int soundType )
{
	// FIX: when the player shoots, their gun isn't in the same position as it is on the model other players see.

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, origin );
		WRITE_BYTE( TE_MODEL );
		WRITE_COORD( origin.x );
		WRITE_COORD( origin.y );
		WRITE_COORD( origin.z );
		WRITE_COORD( velocity.x );
		WRITE_COORD( velocity.y );
		WRITE_COORD( velocity.z );
		WRITE_ANGLE( rotation );
		WRITE_SHORT( model );
		WRITE_BYTE( soundType );
		WRITE_BYTE( 25 ); // 2.5 seconds
	MESSAGE_END();
}

int MaxAmmoCarry( int name )
{
	for ( int i = 0; i < MAX_WEAPONS; ++i )
	{
		ItemInfo& info = CBasePlayerItem::ItemInfoArray[i];
		if ( info.pszAmmo1 && !strcmp( STRING( name ), info.pszAmmo1 ) )
			return info.iMaxAmmo1;
		if ( info.pszAmmo2 && !strcmp( STRING( name ), info.pszAmmo2 ) )
			return info.iMaxAmmo2;
	}
	ALERT( at_console, "MaxAmmoCarry() doesn't recognize '%s'!\n", STRING( name ) );
	return -1;
}

void AddAmmoNameToAmmoRegistry( const char* name )
{
	// make sure it's not already in the registry
	for ( int i = 0; i < MAX_AMMO_SLOTS; ++i )
	{
		if ( CBasePlayerItem::AmmoInfoArray[i].pszName &&
			!_stricmp( CBasePlayerItem::AmmoInfoArray[i].pszName, name ) )
			return;
	}
	if ( ++giAmmoIndex > MAX_AMMO_SLOTS - 1 )
		giAmmoIndex = 0;
	CBasePlayerItem::AmmoInfoArray[giAmmoIndex].pszName = name;
	CBasePlayerItem::AmmoInfoArray[giAmmoIndex].iId = giAmmoIndex;   // yes, this info is redundant
}


// Precaches the weapon and queues the weapon info for sending to clients
void UTIL_PrecacheOtherWeapon( const char* classname )
{
	edict_t* entity = CREATE_NAMED_ENTITY( MAKE_STRING( classname ) );
	if ( FNullEnt( entity ) )
	{
		ALERT( at_console, "NULL Ent in UTIL_PrecacheOtherWeapon\n" );
		return;
	}
	CBasePlayerItem* item = (CBasePlayerItem*)CBaseEntity::Instance( VARS( entity ) );
	if ( item )
	{
		item->Precache();
		ItemInfo info;
		memset( &info, 0, sizeof( info ) );
		if ( item->GetItemInfo( &info ) )
		{
			CBasePlayerItem::ItemInfoArray[info.iId + ( info.bIsAccessory ? 32 : 0 )] = info;
			if ( info.pszAmmo1 && *info.pszAmmo1 )
				AddAmmoNameToAmmoRegistry( info.pszAmmo1 );
			if ( info.pszAmmo2 && *info.pszAmmo2 )
				AddAmmoNameToAmmoRegistry( info.pszAmmo2 );
		}
	}
	REMOVE_ENTITY( entity );
}

void W_Precache()
{
	memset( CBasePlayerItem::ItemInfoArray, 0, sizeof( CBasePlayerItem::ItemInfoArray ) );
	memset( CBasePlayerItem::AmmoInfoArray, 0, sizeof( CBasePlayerItem::AmmoInfoArray ) );
	giAmmoIndex = 0;
	UTIL_PrecacheOther( "weaponbox" );
	UTIL_PrecacheOtherWeapon( "weapon_awp" );
	UTIL_PrecacheOtherWeapon( "weapon_g3sg1" );
	UTIL_PrecacheOtherWeapon( "weapon_ak47" );
	UTIL_PrecacheOtherWeapon( "weapon_scout" );
	UTIL_PrecacheOtherWeapon( "weapon_m60" );
	UTIL_PrecacheOtherWeapon( "weapon_m249" );
	UTIL_PrecacheOtherWeapon( "weapon_m4a1" );
	UTIL_PrecacheOtherWeapon( "weapon_sg552" );
	UTIL_PrecacheOtherWeapon( "weapon_aug" );
	UTIL_PrecacheOtherWeapon( "weapon_sg550" );
	UTIL_PrecacheOtherWeapon( "weapon_m3" );
	UTIL_PrecacheOtherWeapon( "weapon_xm1014" );
	UTIL_PrecacheOtherWeapon( "weapon_usp" );
	UTIL_PrecacheOtherWeapon( "weapon_mac10" );
	UTIL_PrecacheOtherWeapon( "weapon_ump45" );
	UTIL_PrecacheOtherWeapon( "weapon_fiveseven" );
	UTIL_PrecacheOtherWeapon( "weapon_p90" );
	UTIL_PrecacheOtherWeapon( "weapon_deagle" );
	UTIL_PrecacheOtherWeapon( "weapon_p228" );
	UTIL_PrecacheOtherWeapon( "weapon_knife" );
	UTIL_PrecacheOtherWeapon( "weapon_glock18" );
	UTIL_PrecacheOtherWeapon( "weapon_mp5navy" );
	UTIL_PrecacheOtherWeapon( "weapon_tmp" );
	UTIL_PrecacheOtherWeapon( "weapon_elite" );
	UTIL_PrecacheOtherWeapon( "weapon_flashbang" );
	UTIL_PrecacheOtherWeapon( "weapon_hegrenade" );
	UTIL_PrecacheOtherWeapon( "weapon_smokegrenade" );
	UTIL_PrecacheOtherWeapon( "weapon_galil" );
	UTIL_PrecacheOtherWeapon( "weapon_famas" );
	UTIL_PrecacheOtherWeapon( "weapon_shieldgun" );
	UTIL_PrecacheOtherWeapon( "weapon_camera" );
	UTIL_PrecacheOtherWeapon( "weapon_blowtorch" );
	UTIL_PrecacheOtherWeapon( "weapon_radiocontrolledbomb" );
	UTIL_PrecacheOtherWeapon( "weapon_laws" );
	UTIL_PrecacheOtherWeapon( "weapon_briefcase" );
	UTIL_PrecacheOtherWeapon( "weapon_zipline" );
	UTIL_PrecacheOtherWeapon( "weapon_radio" );
	UTIL_PrecacheOtherWeapon( "weapon_fiberopticcamera" );
	g_sModelIndexFireball = PRECACHE_MODEL( "sprites/zerogxplode.spr" ); // fireball
	g_sModelIndexWExplosion = PRECACHE_MODEL( "sprites/WXplo1.spr" ); // underwater fireball
	g_sModelIndexSmoke = PRECACHE_MODEL( "sprites/steam1.spr" );
	g_sModelIndexBloodSpray = PRECACHE_MODEL( "sprites/bloodspray.spr" ); // initial blood
	g_sModelIndexBloodDrop = PRECACHE_MODEL( "sprites/blood.spr" ); // splattered blood 
	g_sModelIndexLaser = PRECACHE_MODEL( (char*)g_pModelNameLaser );
	g_sModelIndexLaserDot = PRECACHE_MODEL( "sprites/laserdot.spr" );


	// used by explosions
	PRECACHE_MODEL( "models/grenade.mdl" );
	PRECACHE_MODEL( "sprites/explode1.spr" );
	PRECACHE_SOUND( "weapons/debris1.wav" ); // explosion aftermaths
	PRECACHE_SOUND( "weapons/debris2.wav" ); // explosion aftermaths
	PRECACHE_SOUND( "weapons/debris3.wav" ); // explosion aftermaths
	PRECACHE_SOUND( "weapons/grenade_hit1.wav" ); //grenade
	PRECACHE_SOUND( "weapons/grenade_hit2.wav" ); //grenade
	PRECACHE_SOUND( "weapons/grenade_hit3.wav" ); //grenade
	PRECACHE_SOUND( "weapons/bullet_hit1.wav" );	// hit by bullet
	PRECACHE_SOUND( "weapons/bullet_hit2.wav" );	// hit by bullet
	PRECACHE_SOUND( "items/weapondrop1.wav" ); // weapon falls to the ground
}

BOOL CanAttack( float attackTime, float currentTime, BOOL predicted )
{
	return currentTime >= attackTime;
}

int CBasePlayerItem::AddToPlayer( CBasePlayer* player )
{
	m_pPlayer = player;
	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, player->pev );
		WRITE_BYTE( m_iId );
		WRITE_BYTE( m_bIsAccessory );
	MESSAGE_END();
	return TRUE;
}

void CBasePlayerItem::DestroyItem()
{
	if ( m_pPlayer )
		m_pPlayer->RemovePlayerItem( this );
	Kill();
}

void CBasePlayerItem::Materialize()
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin ); // link into world.
	SetTouch( &CBasePlayerItem::DefaultTouch );
	SetThink( NULL );
}

void CBasePlayerItem::CheckRespawn()
{
	if ( g_pGameRules->WeaponShouldRespawn( this ) == GR_WEAPON_RESPAWN_YES )
		Respawn();
}

void CBasePlayerItem::DefaultTouch( CBaseEntity* other )
{
	// if it's not a player, ignore
	if ( !other->IsPlayer() )
		return;
	CBasePlayer* player = (CBasePlayer*)other;
	if ( g_pGameRules->CanHavePlayerItem( player, this ) )
	{
		if ( player->AddPlayerItem( this ) )
		{
			AttachToPlayer( player );
			if ( !player->isEquippingSilently() )
				EMIT_SOUND( player->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
		}
		SUB_UseTargets( other, USE_TOGGLE, 0 );
	}
	else if ( gEvilImpulse101 )
		UTIL_Remove( this );
}

void CBasePlayerItem::AttachToPlayer( CBasePlayer* player )
{
	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->aiment = player->edict();
	pev->effects = EF_NODRAW; // ??
	pev->modelindex = 0; // server won't send down to clients if modelindex == 0
	pev->model = iStringNull;
	pev->owner = player->edict();
	pev->nextthink = gpGlobals->time + 0.1;
	SetTouch( NULL );
}

void CBasePlayerItem::AttemptToMaterialize()
{
	float delay = g_pGameRules->FlWeaponTryRespawn( this );
	if ( delay == 0 )
		Materialize();
	else
		pev->nextthink = gpGlobals->time + delay;
}

void CBasePlayerItem::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector( -24, -24, 0 );
	pev->absmax = pev->origin + Vector( 24, 24, 16 );
}

void CBasePlayerItem::Drop()
{
	SetTouch( NULL );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBasePlayerItem::Kill()
{
	SetTouch( NULL );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBasePlayerItem::Holster( int skiplocal )
{
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
}

CBaseEntity* CBasePlayerItem::Respawn()
{
	CBaseEntity* item = CBaseEntity::Create( (char*)STRING( pev->classname ),
		g_pGameRules->VecWeaponRespawnSpot( this ), pev->angles, pev->owner );
	if ( item )
	{
		item->pev->effects |= EF_NODRAW;
		item->SetTouch( NULL );
		item->SetThink( &CBasePlayerItem::AttemptToMaterialize );
		DROP_TO_FLOOR( ENT( pev ) );

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		item->pev->nextthink = g_pGameRules->FlWeaponRespawnTime( this );
	}
	else
		ALERT( at_console, "Respawn failed to create %s!\n", STRING( pev->classname ) );
	return item;
}

void CBasePlayerItem::FallInit()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	SetTouch( &CBasePlayerItem::DefaultTouch );
	SetThink( &CBasePlayerItem::FallThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBasePlayerItem::FallThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	if ( pev->flags & FL_ONGROUND )
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if ( !FNullEnt( pev->owner ) )
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "items/weapondrop1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 0, 29 ) + 95 );

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;
		Materialize();
	}
}

void CBasePlayerWeapon::Spawn()
{
	if ( CVAR_GET_FLOAT( "lightmultiplier" ) >= 0.1f )
	{
		pev->renderfx = kRenderFxLightMultiplier;
		pev->iuser4 = (int)CVAR_GET_FLOAT( "lightmultiplier" );
	}
}

int CBasePlayerWeapon::AddToPlayer( CBasePlayer* player )
{
	m_pPlayer = player;
	if ( m_bIsAccessory )
		player->m_iAccessoryBits |= 1 << m_iId;
	else
		player->pev->weapons |= 1 << m_iId;
	if ( !m_iPrimaryAmmoType )
	{
		m_iPrimaryAmmoType = CBasePlayer::GetAmmoIndex( pszAmmo1() );
		m_iSecondaryAmmoType = CBasePlayer::GetAmmoIndex( pszAmmo2() );
	}
	if ( !AddWeapon() )
		return FALSE;
	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, player->pev );
		WRITE_BYTE( m_iId );
		WRITE_BYTE( m_bIsAccessory );
	MESSAGE_END();
	return TRUE;
}

int CBasePlayerWeapon::AddDuplicate( CBasePlayerItem* original )
{
	if ( m_iDefaultAmmo )
		return ExtractAmmo( (CBasePlayerWeapon*)original );
	return ExtractClipAmmo( (CBasePlayerWeapon*)original );
}

int CBasePlayerWeapon::ExtractAmmo( CBasePlayerWeapon* weapon )
{
	int result = FALSE;
	if ( pszAmmo1() )
	{
		// blindly call with m_iDefaultAmmo. It's either going to be a value or zero. If it is zero,
		// we only get the ammo in the weapon's clip, which is what we want. 
		result = weapon->AddPrimaryAmmo( m_iDefaultAmmo, (char*)pszAmmo1(), iMaxClip(), iMaxAmmo1() );
		m_iDefaultAmmo = 0;
	}
	if ( pszAmmo2() )
		result = weapon->AddSecondaryAmmo( 0, (char*)pszAmmo2(), iMaxAmmo2() );
	return result;
}

int CBasePlayerWeapon::ExtractClipAmmo( CBasePlayerWeapon* weapon )
{
	int count = m_iClip == WEAPON_NOCLIP ? 0 : m_iClip;
	return weapon->m_pPlayer->GiveAmmo( count, (char*)pszAmmo1(), iMaxAmmo1() );
}

//=========================================================
// RetireWeapon - no more ammo for this gun, put it away.
//=========================================================
void CBasePlayerWeapon::RetireWeapon()
{
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
	//m_pPlayer->pev->viewmodelindex = NULL;

	g_pGameRules->GetNextBestWeapon( m_pPlayer, this );
}

BOOL CBasePlayerWeapon::DefaultDeploy( const char* viewModel, const char* weaponModel, int animation, const char* extension, int skiplocal )
{
	if ( !CanDeploy() )
		return FALSE;
	m_pPlayer->TabulateAmmo();
	m_pPlayer->pev->viewmodel = viewModel ? MAKE_STRING( viewModel ) : 0;
	m_pPlayer->pev->weaponmodel = weaponModel ? MAKE_STRING( weaponModel ) : 0;
	model_name = m_pPlayer->pev->viewmodel;
	strcpy( m_pPlayer->m_szAnimExtention, extension );
	SendWeaponAnim( animation, skiplocal );
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.75;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
	m_flDecreaseShotsFired = gpGlobals->time;
	m_pPlayer->m_iFOV = 90;
	m_pPlayer->pev->fov = 90;
	m_pPlayer->m_bResumeZoom = false;
	m_pPlayer->m_iLastZoom = 90;
	m_pPlayer->m_iIncognito = 0;
	m_pPlayer->FlashlightTurnOff();
	return TRUE;
}

BOOL CBasePlayerWeapon::DefaultReload( int clipSize, int animation, float delay )
{
	int ammo = m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
	if ( ammo <= 0 )
		return FALSE;
	int count = clipSize - m_iClip;
	if ( ammo >= count && count == 0 )
		return FALSE;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + delay;
	SendWeaponAnim( animation, UseDecrement() );
	m_fInReload = TRUE;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	return TRUE;
}

void CBasePlayerWeapon::Holster( int skiplocal )
{
	m_fInReload = FALSE; // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
}

BOOL CBasePlayerWeapon::AddPrimaryAmmo( int count, char* name, int maxClip, int maxCarry )
{
	int index;
	if ( maxClip <= 0 )
	{
		m_iClip = WEAPON_NOCLIP;
		index = m_pPlayer->GiveAmmo( count, name, maxCarry );
	}
	else if ( m_iClip != 0 )
		index = m_pPlayer->GiveAmmo( count, name, maxCarry );
	else
	{
		int clip = min( count, maxClip );
		m_iClip = clip;
		index = m_pPlayer->GiveAmmo( count - clip, name, maxCarry );
	}
	if ( index <= 0 )
		return FALSE;
	m_iPrimaryAmmoType = index;
	if ( m_pPlayer->HasPlayerItem( this ) )
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
	return TRUE;
}

BOOL CBasePlayerWeapon::AddSecondaryAmmo( int count, char* name, int maxCarry )
{
	int index = m_pPlayer->GiveAmmo( count, name, maxCarry );
	if ( index <= 0 )
		return FALSE;
	m_iSecondaryAmmoType = index;
	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
	return TRUE;
}

BOOL CBasePlayerWeapon::PlayEmptySound()
{
	if ( m_iPlayEmptySound )
	{
		EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM );
		m_iPlayEmptySound = FALSE;
	}
	return FALSE;
}

void CBasePlayerWeapon::ResetEmptySound()
{
	m_iPlayEmptySound = TRUE;
}

void CBasePlayerWeapon::SendWeaponAnim( int animation, int skiplocal )
{
	m_pPlayer->pev->weaponanim = animation;
	MESSAGE_BEGIN( MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->pev );
		WRITE_BYTE( animation );						// sequence number
		WRITE_BYTE( pev->body );					// weaponmodel bodygroup.
	MESSAGE_END();
}

void CBasePlayerWeapon::EjectBrassLate()
{
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector forward = gpGlobals->v_forward * 25;
	Vector up = gpGlobals->v_up * RANDOM_FLOAT( 100, 150 );
	Vector velocity = m_pPlayer->pev->velocity + gpGlobals->v_right * RANDOM_FLOAT( 50, 70 ) + up + forward;
	Vector origin = pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -9 + gpGlobals->v_forward * 16 + gpGlobals->v_right * 9;
	EjectBrass( origin, velocity, pev->angles.y, m_iShellId, TE_BOUNCE_SHELL );
}

int CBasePlayerWeapon::UpdateClientData( CBasePlayer* player )
{
	int state = 0;
	BOOL send = !player->m_fWeapon;
	if ( player->m_pActiveItem == this )
	{
		send = TRUE;
		state = player->m_fOnTarget ? WEAPON_IS_ONTARGET : 1;
	}
	else if ( player->m_pClientActiveItem == this )
		send = TRUE;
	if ( m_iClip != m_iClientClip || m_iClientWeaponState != state || player->m_iFOV != player->m_iClientFOV || send )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, player->pev );
			WRITE_BYTE( state );
			WRITE_BYTE( m_iId );
			WRITE_BYTE( m_iClip );
			WRITE_BYTE( m_bIsAccessory );
		MESSAGE_END();
		m_iClientWeaponState = state;
		m_iClientClip = m_iClip;
		player->m_fWeapon = TRUE;
	}
	if ( m_pNext )
		m_pNext->UpdateClientData( player );
	return TRUE;
}

void CBasePlayerWeapon::KickBack( float upBase, float lateralBase, float upModifier, float lateralModifier,
	float upMax, float lateralMax, int directionChange )
{
	float up = upBase;
	float lateral = lateralBase;
	if ( m_iShotsFired != 1 )
	{
		up += upModifier * m_iShotsFired;
		lateral += lateralModifier * m_iShotsFired;
	}
	m_pPlayer->pev->punchangle.x -= up;
	if ( m_pPlayer->pev->punchangle.x < upMax * -1 )
		m_pPlayer->pev->punchangle.x = upMax * -1;
	if ( m_iDirection == 1 )
	{
		m_pPlayer->pev->punchangle.y += lateral;
		if ( m_pPlayer->pev->punchangle.y > lateralMax )
			m_pPlayer->pev->punchangle.y = lateralMax;
	}
	else
	{
		m_pPlayer->pev->punchangle.y -= lateral;
		if ( m_pPlayer->pev->punchangle.y < lateralMax * -1 )
			m_pPlayer->pev->punchangle.y = lateralMax * -1;
	}
	if ( RANDOM_LONG( 0, directionChange ) == 0 )
		m_iDirection = 1 - m_iDirection;
}

BOOL CBasePlayerWeapon::IsUseable()
{
	if ( m_iClip > 0 )
		return TRUE;
	if ( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0 )
		return TRUE;
	return iMaxAmmo1() == -1;
}

void CBasePlayerWeapon::FireRemaining( int& shotsFired, float& shootTime, BOOL isGlock18 )
{
	if ( --m_iClip < 0 )
	{
		m_iClip = 0;
		shotsFired = 3;
		shootTime = 0;
		return;
	}
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector source = m_pPlayer->GetGunPosition();
	Vector direction;
	if ( isGlock18 )
	{
		direction = m_pPlayer->FireBullets3( source, gpGlobals->v_forward, 0.05f, 8192, 1,
			BULLET_PLAYER_9MM, 18, 0.9f, m_pPlayer->pev, TRUE, m_pPlayer->random_seed );
		--m_pPlayer->ammo_9mm;
		int y = (int)( m_pPlayer->pev->punchangle.y * 10000 );
		int x = (int)( m_pPlayer->pev->punchangle.x * 10000 );
		PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireGlock18, 0,
			(float*)&g_vecZero, (float*)&g_vecZero, direction.x, direction.y, x, y, m_iClip == 0, 0 );
	}
	else
	{
		direction = m_pPlayer->FireBullets3( source, gpGlobals->v_forward, m_fBurstSpread, 8192, 2,
			BULLET_PLAYER_556MM, 30, 0.96f, m_pPlayer->pev, FALSE, m_pPlayer->random_seed );
		--m_pPlayer->ammo_556nato;
		int y = (int)( m_pPlayer->pev->punchangle.y * 10000000 );
		int x = (int)( m_pPlayer->pev->punchangle.x * 10000000 );
		PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireFamas, 0,
			(float*)&g_vecZero, (float*)&g_vecZero, direction.x, direction.y, x, y, 0, 0 );
	}
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	if ( ++shotsFired == 3 )
		shootTime = 0;
	else
		shootTime = gpGlobals->time + 0.1f;
}

void CBasePlayerWeapon::ItemPostFrame()
{
	if ( m_flGlock18Shoot != 0 )
		FireRemaining( m_iGlock18ShotsFired, m_flGlock18Shoot, TRUE );
	else if ( m_flFamasShoot != 0 )
		FireRemaining( m_iFamasShotsFired, m_flFamasShoot, FALSE );

	if ( UTIL_WeaponTimeBase() >= m_flNextPrimaryAttack && m_pPlayer->m_bResumeZoom )
	{
		m_pPlayer->m_iFOV = m_pPlayer->m_iLastZoom;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV;
		if ( m_pPlayer->m_iFOV == m_pPlayer->m_iLastZoom )
			m_pPlayer->m_bResumeZoom = false;
	}
	if ( m_pPlayer->m_flEjectBrass != 0 && gpGlobals->time >= m_pPlayer->m_flEjectBrass )
	{
		m_pPlayer->m_flEjectBrass = 0;
		EjectBrassLate();
	}
	if ( m_fInReload && UTIL_WeaponTimeBase() >= m_pPlayer->m_flNextAttack )
	{
		int count = min( iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] );
		m_iClip += count;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= count;
		m_pPlayer->TabulateAmmo();
		m_fInReload = FALSE;
	}

	if ( ( m_pPlayer->pev->button & IN_ATTACK2 ) && UTIL_WeaponTimeBase() >= m_flNextSecondaryAttack )
	{
		if ( pszAmmo2() && m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] == 0 )
			m_fFireOnEmpty = TRUE;
		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ( ( m_pPlayer->pev->button & IN_ATTACK ) && UTIL_WeaponTimeBase() >= m_flNextPrimaryAttack )
	{
		if ( ( m_iClip == 0 && pszAmmo1() ) ||
			( iMaxClip() == WEAPON_NOCLIP && m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] == 0 ) )
			m_fFireOnEmpty = TRUE;
		m_pPlayer->TabulateAmmo();
		PrimaryAttack();
	}
	else if ( ( m_pPlayer->pev->button & IN_RELOAD ) && iMaxClip() != WEAPON_NOCLIP && !m_fInReload &&
		UTIL_WeaponTimeBase() > m_flNextPrimaryAttack && m_flFamasShoot == 0 && m_flGlock18Shoot == 0 )
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if ( !( m_pPlayer->pev->button & ( IN_ATTACK | IN_ATTACK2 ) ) )
	{
		if ( m_bDelayFire )
		{
			m_bDelayFire = false;
			if ( m_iShotsFired > 15 )
				m_iShotsFired = 15;
			m_flDecreaseShotsFired = gpGlobals->time + 0.4;
		}
		// no fire buttons down

		m_fFireOnEmpty = FALSE;
		BOOL pistol = !m_bIsAccessory && ( m_iId == WEAPON_P228 || m_iId == WEAPON_ELITE ||
			m_iId == WEAPON_FIVESEVEN || m_iId == WEAPON_USP || m_iId == WEAPON_GLOCK18 ||
			m_iId == WEAPON_DEAGLE || m_iId == WEAPON_SHIELDGUN );
		if ( pistol )
			m_iShotsFired = 0;
		else if ( m_iShotsFired > 0 && gpGlobals->time > m_flDecreaseShotsFired )
		{
			m_flDecreaseShotsFired = gpGlobals->time + 0.0225;
			--m_iShotsFired;
		}
		if ( !IsUseable() && UTIL_WeaponTimeBase() > m_flNextPrimaryAttack )
		{
			WeaponIdle();
			return;
		}
		if ( m_iClip == 0 && !( iFlags() & ITEM_FLAG_NOAUTORELOAD ) &&
			UTIL_WeaponTimeBase() > m_flNextPrimaryAttack && m_flFamasShoot == 0 && m_flGlock18Shoot == 0 )
		{
			Reload();
			return;
		}
		WeaponIdle();
		return;
	}

	// catch all
	if ( ShouldWeaponIdle() )
		WeaponIdle();
}

void CWeaponBox::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	SET_MODEL( ENT( pev ), "models/w_weaponbox.mdl" );
}

void CWeaponBox::Precache()
{
	PRECACHE_MODEL( "models/w_weaponbox.mdl" );
}

void CWeaponBox::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector( -16, -16, 0 );
	pev->absmax = pev->origin + Vector( 16, 16, 16 );
}

BOOL CWeaponBox::HasWeapon( CBasePlayerItem* item )
{
	for ( CBasePlayerItem* weapon = m_rgpPlayerItems[item->iItemSlot()]; weapon; weapon = weapon->m_pNext )
	{
		if ( FClassnameIs( weapon->pev, STRING( item->pev->classname ) ) )
			return TRUE;
	}
	return FALSE;
}

BOOL CWeaponBox::PackWeapon( CBasePlayerItem* item )
{
	if ( HasWeapon( item ) )
		return FALSE;
	if ( item->m_pPlayer )
	{
		if ( item == item->m_pPlayer->m_pActiveItem )
			item->Holster();
		if ( !item->m_pPlayer->RemovePlayerItem( item ) )
			return FALSE;
	}
	int slot = item->iItemSlot();
	item->m_pNext = m_rgpPlayerItems[slot];
	m_rgpPlayerItems[slot] = item;
	item->pev->spawnflags |= SF_NORESPAWN;
	item->pev->movetype = MOVETYPE_NONE;
	item->pev->solid = SOLID_NOT;
	item->pev->effects = EF_NODRAW;
	item->pev->modelindex = 0;
	item->pev->model = iStringNull;
	item->pev->owner = edict();
	item->SetThink( NULL );
	item->SetTouch( NULL );
	item->m_pPlayer = NULL;
	return TRUE;
}

void CWeaponBox::Kill()
{
	for ( int slot = 0; slot < MAX_ITEM_TYPES; ++slot )
	{
		for ( CBasePlayerItem* item = m_rgpPlayerItems[slot]; item; item = item->m_pNext )
		{
			item->SetThink( &CBaseEntity::SUB_Remove );
			item->pev->nextthink = gpGlobals->time + 0.1;
		}
	}

	// remove the box
	UTIL_Remove( this );
}

BOOL CWeaponBox::IsEmpty()
{
	for ( int slot = 0; slot < MAX_ITEM_TYPES; ++slot )
	{
		if ( m_rgpPlayerItems[slot] )
			return FALSE;
	}
	for ( int ammo = 0; ammo < MAX_AMMO_SLOTS; ++ammo )
	{
		if ( m_rgiszAmmo[ammo] )
			return FALSE;
	}
	return TRUE;
}

int CWeaponBox::GiveAmmo( int count, const char* name, int maxCarry, int* index )
{
	int slot;
	for ( slot = 1; slot < MAX_AMMO_SLOTS; ++slot )
	{
		if ( !m_rgiszAmmo[slot] )
		{
			if ( index )
				*index = slot;
			m_rgiszAmmo[slot] = MAKE_STRING( name );
			m_rgAmmo[slot] = count;
			return slot;
		}
		if ( !_stricmp( name, STRING( m_rgiszAmmo[slot] ) ) )
		{
			if ( index )
				*index = slot;
			int add = min( count, maxCarry - m_rgAmmo[slot] );
			if ( count != 0 && add <= 0 )
				return -1;
			m_rgAmmo[slot] += add;
			return slot;
		}
	}
	ALERT( at_console, "out of named ammo slots\n" );
	return slot;
}

BOOL CWeaponBox::PackAmmo( int name, int count )
{
	if ( !name )
	{
		// error here
		ALERT( at_console, "NULL String in PackAmmo!\n" );
		return FALSE;
	}
	int maxCarry = MaxAmmoCarry( name );
	if ( maxCarry == -1 || count <= 0 )
		return FALSE;
	GiveAmmo( count, (char*)STRING( name ), maxCarry );
	return TRUE;
}

void CWeaponBox::KeyValue( KeyValueData* data )
{
	if ( m_cAmmoTypes <= MAX_AMMO_SLOTS - 1 )
	{
		PackAmmo( ALLOC_STRING( data->szKeyName ), atoi( data->szValue ) );
		++m_cAmmoTypes;
		data->fHandled = TRUE;
	}
	else
		ALERT( at_console, "WeaponBox too full! only %d ammotypes allowed\n", MAX_AMMO_SLOTS );
}

void CWeaponBox::Touch( CBaseEntity* other )
{
	if ( !( pev->flags & FL_ONGROUND ) || !other->IsPlayer() || !other->IsAlive() )
		return;
	CBasePlayer* player = (CBasePlayer*)other;
	if ( player->m_bIsVIP == TRUE )
		return;
	BOOL remove = TRUE;
	BOOL sound = FALSE;
	for ( int slot = 0; slot < MAX_ITEM_TYPES; ++slot )
	{
		CBasePlayerItem* head = m_rgpPlayerItems[slot];
		CBasePlayerItem* item = head;
		while ( item )
		{
			if ( ( slot == 1 || slot == 2 ) && player->m_rgpPlayerItems[slot] )
			{
				item = head->m_pNext;
				remove = FALSE;
			}
			else
			{
				if ( player->AddPlayerItem( item ) )
					item->AttachToPlayer( player );
				sound = TRUE;
				item = m_rgpPlayerItems[slot]->m_pNext;
				head = item;
				m_rgpPlayerItems[slot] = item;
			}
		}
	}
	if ( remove )
	{
		for ( int ammo = 0; ammo < MAX_AMMO_SLOTS; ++ammo )
		{
			if ( m_rgiszAmmo[ammo] )
			{
				player->GiveAmmo( m_rgAmmo[ammo], (char*)STRING( m_rgiszAmmo[ammo] ), MaxAmmoCarry( m_rgiszAmmo[ammo] ) );
				m_rgiszAmmo[ammo] = 0;
				m_rgAmmo[ammo] = 0;
			}
		}
	}
	if ( sound )
		EMIT_SOUND( player->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
	if ( remove )
	{
		SetTouch( NULL );
		UTIL_Remove( this );
	}
}
