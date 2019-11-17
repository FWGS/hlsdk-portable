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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "shake.h"

#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY	2000
#define BOLT_WATER_VELOCITY	1000

extern BOOL gPhysicsInterfaceInitialized;

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
// 
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CCrossbowBolt : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	void EXPORT BubbleThink( void );
	void EXPORT BoltTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );

	int m_iTrail;

public:
	static CCrossbowBolt *BoltCreate( void );
};

LINK_ENTITY_TO_CLASS( crossbow_bolt, CCrossbowBolt )

CCrossbowBolt *CCrossbowBolt::BoltCreate( void )
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = GetClassPtr( (CCrossbowBolt *)NULL );
	pBolt->pev->classname = MAKE_STRING( "crossbow_bolt" );	// g-cont. enable save\restore
	pBolt->Spawn();

	return pBolt;
}

void CCrossbowBolt::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5f;

	SET_MODEL( ENT( pev ), "models/crossbow_bolt.mdl" );

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	SetTouch( &CCrossbowBolt::BoltTouch );
	SetThink( &CCrossbowBolt::BubbleThink );
	pev->nextthink = gpGlobals->time + 0.2f;
}

void CCrossbowBolt::Precache()
{
	PRECACHE_MODEL( "models/crossbow_bolt.mdl" );
	PRECACHE_SOUND( "weapons/xbow_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/xbow_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/xbow_fly1.wav" );
	PRECACHE_SOUND( "weapons/xbow_hit1.wav" );
	PRECACHE_SOUND( "fvox/beep.wav" );
	m_iTrail = PRECACHE_MODEL( "sprites/streak.spr" );
}

int CCrossbowBolt::Classify( void )
{
	return CLASS_NONE;
}

void CCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if( pOther->pev->takedamage )
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t *pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		if( pOther->IsPlayer() )
		{
			pOther->TraceAttack( pevOwner, gSkillData.plrDmgCrossbowClient, pev->velocity.Normalize(), &tr, DMG_NEVERGIB ); 
		}
		else
		{
			pOther->TraceAttack( pevOwner, gSkillData.plrDmgCrossbowMonster, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB ); 
		}

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = g_vecZero;
		// play body "thwack" sound
		switch( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			EMIT_SOUND( ENT( pev ), CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT( pev ), CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM );
			break;
		}

		if( !g_pGameRules->IsMultiplayer() )
		{
			Killed( pev, GIB_NEVER );
		}
	}
	else
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/xbow_hit1.wav", RANDOM_FLOAT( 0.95f, 1.0f ), ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 7 ) );

		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12.0f );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = g_vecZero;
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG( 0, 360 );
			pev->nextthink = gpGlobals->time + 10.0f;
		}
		else if( pOther->pev->movetype == MOVETYPE_PUSH || pOther->pev->movetype == MOVETYPE_PUSHSTEP )
		{
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12.0f );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->velocity = g_vecZero;
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG( 0, 360 );
			pev->nextthink = gpGlobals->time + 10.0f;			

			if (gPhysicsInterfaceInitialized) {
				// g-cont. Setup movewith feature
				pev->movetype = MOVETYPE_COMPOUND;	// set movewith type
				pev->aiment = ENT( pOther->pev );	// set parent
			}
		}

		if( UTIL_PointContents( pev->origin ) != CONTENTS_WATER )
		{
			UTIL_Sparks( pev->origin );
		}
	}

	if( g_pGameRules->IsMultiplayer() )
	{
		SetThink( &CCrossbowBolt::ExplodeThink );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CCrossbowBolt::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if( pev->waterlevel == 0 )
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1f, pev->origin, 1 );
}

void CCrossbowBolt::ExplodeThink( void )
{
	int iContents = UTIL_PointContents( pev->origin );
	int iScale;

	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if( iContents != CONTENTS_WATER )
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( iScale ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	entvars_t *pevOwner;

	if( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB );

	UTIL_Remove( this );
}
#endif

enum crossbow_e
{
	CROSSBOW_IDLE1 = 0,	// full
	CROSSBOW_IDLE2,		// empty
	CROSSBOW_FIDGET1,	// full
	CROSSBOW_FIDGET2,	// empty
	CROSSBOW_FIRE1,		// full
	CROSSBOW_RELOAD,	// reload
	CROSSBOW_DRAWBACK,		// empty
	CROSSBOW_UNDRAW,	// from empty
	CROSSBOW_DRAW1,		// full
	CROSSBOW_DRAW2,		// empty
	CROSSBOW_HOLSTER1,	// full
	CROSSBOW_HOLSTER2	// empty
};

LINK_ENTITY_TO_CLASS( weapon_cmlwbr, CCrossbow )

void CCrossbow::Spawn()
{
	Precache();
	m_iId = WEAPON_CROSSBOW;
	SET_MODEL( ENT( pev ), "models/w_crossbow.mdl" );

	m_iDefaultAmmo = CROSSBOW_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

int CCrossbow::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CCrossbow::Precache( void )
{
	PRECACHE_MODEL( "models/w_crossbow.mdl" );
	PRECACHE_MODEL( "models/v_cmlwbr.mdl" );
	PRECACHE_MODEL( "models/p_crossbow.mdl" );

	PRECACHE_SOUND( "weapons/cmlwbr_fire.wav" );
	PRECACHE_SOUND( "weapons/cmlwbr_reload.wav" );
	PRECACHE_SOUND( "weapons/cmlwbr_drawback.wav" );
	PRECACHE_SOUND( "weapons/cmlwbr_undraw.wav" );
	PRECACHE_SOUND( "weapons/cmlwbr_zoom.wav" );

	UTIL_PrecacheOther( "crossbow_bolt" );
}

int CCrossbow::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "bolts";
	p->iMaxAmmo1 = BOLT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CROSSBOW_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iId = WEAPON_CROSSBOW;
	p->iFlags = 0;
	p->iWeight = CROSSBOW_WEIGHT;
	return 1;
}

BOOL CCrossbow::Deploy()
{
	return DefaultDeploy( "models/v_cmlwbr.mdl", "models/p_crossbow.mdl", m_iClip ? CROSSBOW_DRAW1 : CROSSBOW_DRAW2 , "bow" );
}

void CCrossbow::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if( m_fInZoom )
	{
		ZoomOut();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( m_iClip ? CROSSBOW_HOLSTER1 : CROSSBOW_HOLSTER2 );
}

void CCrossbow::PrimaryAttack( void )
{
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2f;
		return;
	}

	FireBolt();
}

void CCrossbow::FireBolt()
{
	TraceResult tr;

	if( m_iClip == 0 || !m_fInAttack )
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	m_iClip--;

	EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cmlwbr_fire.wav", RANDOM_FLOAT( 0.95, 1.0 ), ATTN_NORM, 0, 93 + RANDOM_LONG( 0, 0xF ) );
	SendWeaponAnim( CROSSBOW_FIRE1 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );

	anglesAim.x	= -anglesAim.x;

#ifndef CLIENT_DLL
	Vector vecSrc	= m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2.0f;
	Vector vecDir	= gpGlobals->v_forward;

	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();

	if( m_pPlayer->pev->waterlevel == 3 )
	{
		pBolt->pev->velocity = vecDir * BOLT_WATER_VELOCITY;
		pBolt->pev->speed = BOLT_WATER_VELOCITY;
	}
	else
	{
		pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
		pBolt->pev->speed = BOLT_AIR_VELOCITY;
	}
	pBolt->pev->avelocity.z = 10.0f;
#endif

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;

	m_pPlayer->pev->punchangle.x -= 2.0f;
}

void CCrossbow::ZoomOut()
{
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
	m_fInZoom = 0;
	m_pPlayer->pev->iuser4 = m_fInZoom;
	m_pPlayer->pev->viewmodel = MAKE_STRING( "models/v_cmlwbr.mdl" );
	UTIL_ScreenFade( m_pPlayer, g_vecZero, 0.2, 0.1, 255, FFADE_IN );
}

void CCrossbow::SecondaryAttack()
{
	if( m_fInZoom == 2 )	
	{
		ZoomOut();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7;
	}
	else
	{
		m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 20 * ( 2 - m_fInZoom );
		++m_fInZoom;
		m_pPlayer->pev->iuser4 = m_fInZoom;
		m_pPlayer->pev->viewmodel = 0;
		UTIL_ScreenFade( m_pPlayer, g_vecZero, 0.1, 0.1, 255, FFADE_IN );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
	EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cmlwbr_zoom.wav", RANDOM_FLOAT( 0.95, 1.0 ), ATTN_NORM, 0, 93 + RANDOM_LONG( 0, 0xF ) );	
}

void CCrossbow::ToggleDrawn()
{
	SetThink( NULL );
	m_fInAttack = ( m_fInAttack == 0 );
	if( m_fInSpecialReload )
	{
		m_fInSpecialReload = 0;
		Reload();
	}
}

void CCrossbow::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == CROSSBOW_MAX_CLIP )
		return;

	if( m_fInZoom != 0 )
	{
		ZoomOut();
	}

	if( m_fInAttack )
	{
		if(!m_fInSpecialReload )
		{
			SetThink( &CCrossbow::ToggleDrawn );
			pev->nextthink = gpGlobals->time + 1.4f;
			SendWeaponAnim( CROSSBOW_UNDRAW );
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cmlwbr_undraw.wav", RANDOM_FLOAT( 0.95, 1.0 ), ATTN_NORM, 0, 93 + RANDOM_LONG( 0, 0xF ) );
			m_fInSpecialReload = 1;
		}
		return;
	}

	if( DefaultReload( CROSSBOW_MAX_CLIP, CROSSBOW_RELOAD, 4.5f ) )
	{
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cmlwbr_reload.wav", RANDOM_FLOAT( 0.95, 1.0 ), ATTN_NORM, 0, 93 + RANDOM_LONG( 0, 0xF ) );
	}
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.5f;
}

void CCrossbow::WeaponIdle( void )
{
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	ResetEmptySound();
	
	if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		int iAnim;
		if( m_fInAttack || !m_iClip )
		{
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if( flRand <= 0.75 || m_fInZoom )
			{
				iAnim = m_fInAttack ? CROSSBOW_IDLE1 : CROSSBOW_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.1f;
			}
			else
			{
				iAnim = m_fInAttack ? CROSSBOW_FIDGET1 : CROSSBOW_FIDGET2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.8f;
			}
		}
		else
		{
			iAnim = CROSSBOW_DRAWBACK;
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cmlwbr_drawback.wav", RANDOM_FLOAT( 0.9, 1.0 ), ATTN_NORM, 0, 93 + RANDOM_LONG( 0, 0xF ) );
			m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.3f;
			SetThink( &CCrossbow::ToggleDrawn );
			pev->nextthink = gpGlobals->time + 1.2f;
		}
		SendWeaponAnim( iAnim );
	}
}

class CCrossbowAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_crossbow_clip.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_crossbow_clip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{ 
		if( pOther->GiveAmmo( AMMO_CROSSBOWCLIP_GIVE, "bolts", BOLT_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_bolts, CCrossbowAmmo )
#endif
