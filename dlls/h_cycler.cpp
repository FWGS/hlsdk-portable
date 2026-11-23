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

===== h_cycler.cpp ========================================================

  The Halflife Cycler Monsters

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "animation.h"

class CCycler : public CBaseMonster
{
public:
	void GenericCyclerSpawn( const char* model, Vector mins, Vector maxs );
	int ObjectCaps() override { return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;
	void Spawn() override;
	void Think() override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	BOOL IsAlive() override { return FALSE; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	int m_animate;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

TYPEDESCRIPTION CCycler::m_SaveData[] =
{
	DEFINE_FIELD( CCycler, m_animate, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( CCycler, CBaseMonster );

void CCycler::GenericCyclerSpawn( const char* model, Vector mins, Vector maxs )
{
	if ( !model || !model[0] )
	{
		ALERT( at_error, "cycler at %.0f %.0f %0.f missing modelname",
			pev->origin.x, pev->origin.y, pev->origin.z );
		REMOVE_ENTITY( edict() );
		return;
	}
	pev->classname = MAKE_STRING( "cycler" );
	PRECACHE_MODEL( model );
	SET_MODEL( edict(), model );
	CCycler::Spawn();
	UTIL_SetSize( pev, mins, maxs );
}

void CCycler::Spawn()
{
	InitBoneControllers();
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_YES;
	pev->effects = 0;
	pev->health = 80000; // no cycler should die
	pev->yaw_speed = 5;
	pev->ideal_yaw = pev->angles.y;
	ChangeYaw( 360 );
	m_flFrameRate = 75;
	m_flGroundSpeed = 0;
	pev->nextthink += 1.0;
	ResetSequenceInfo();
	if ( pev->sequence != 0 || pev->frame != 0 )
	{
		m_animate = 0;
		pev->framerate = 0;
	}
	else
		m_animate = 1;
}




//
// cycler think
//
void CCycler::Think()
{
	pev->nextthink = gpGlobals->time + 0.1;
	if ( m_animate )
		StudioFrameAdvance();
	if ( m_fSequenceFinished && !m_fSequenceLoops )
	{
		// ResetSequenceInfo();
		// hack to avoid reloading model every frame
		pev->animtime = gpGlobals->time;
		pev->framerate = 1;
		m_fSequenceFinished = FALSE;
		m_flLastEventCheck = gpGlobals->time;
		pev->frame = 0;
		if ( !m_animate )
			pev->framerate = 0;	// FIX: don't reset framerate
	}
}

void CCycler::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	m_animate = !m_animate;
	pev->framerate = m_animate ? 1 : 0;
}

//
// CyclerPain , changes sequences when shot
//
//void CCycler :: Pain( float flDamage )
int CCycler::TakeDamage( entvars_t*, entvars_t*, float, int )
{
	if ( m_animate )
	{
		pev->sequence++;
		ResetSequenceInfo();
		if ( m_flFrameRate == 0 )
		{
			pev->sequence = 0;
			ResetSequenceInfo();
		}
		pev->frame = 0;
	}
	else
	{
		pev->framerate = 1;
		StudioFrameAdvance( 0.1f );
		pev->framerate = 0;
		ALERT( at_console, "sequence: %d, frame %.0f\n", pev->sequence, pev->frame );
	}
	return 0;
}


//
// we should get rid of all the other cyclers and replace them with this.
//
class CGenericCycler : public CCycler
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS( cycler, CGenericCycler );

void CGenericCycler::Spawn()
{
	GenericCyclerSpawn( (char*)STRING( pev->model ), Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );
}



// Probe droid imported for tech demo compatibility
//
// PROBE DROID
//
class CCyclerProbe : public CCycler
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS( cycler_prdroid, CCyclerProbe );

void CCyclerProbe::Spawn()
{
	pev->origin = pev->origin + Vector( 0, 0, 16 );
	GenericCyclerSpawn( "models/prdroid.mdl", Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) );
}

class CCyclerSprite : public CBaseEntity
{
public:
	void Spawn() override;
	void Think() override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;
	int ObjectCaps() override { return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE | FCAP_DONT_SAVE; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	void Animate( float frames );

	static TYPEDESCRIPTION m_SaveData[];
	int		m_animate;
	float	m_lastTime;
	float	m_maxFrame;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( cycler_sprite, CCyclerSprite );

TYPEDESCRIPTION CCyclerSprite::m_SaveData[] =
{
	DEFINE_FIELD( CCyclerSprite, m_animate, FIELD_INTEGER ),
	DEFINE_FIELD( CCyclerSprite, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CCyclerSprite, m_maxFrame, FIELD_FLOAT )
};

IMPLEMENT_SAVERESTORE( CCyclerSprite, CBaseEntity );

void CCyclerSprite::Spawn()
{
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_YES;
	pev->effects = 0;
	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1;
	m_animate = 1;
	m_lastTime = gpGlobals->time;
	PRECACHE_MODEL( (char*)STRING( pev->model ) );
	SET_MODEL( edict(), STRING( pev->model ) );
	m_maxFrame = MODEL_FRAMES( pev->modelindex ) - 1;
}

void CCyclerSprite::Think()
{
	if ( m_animate && m_maxFrame > 1 )
		Animate( pev->framerate * ( gpGlobals->time - m_lastTime ) );
	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

void CCyclerSprite::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	m_animate = !m_animate;
	ALERT( at_console, "Sprite: %s\n", STRING( pev->model ) );
}

int CCyclerSprite::TakeDamage( entvars_t*, entvars_t*, float, int )
{
	if ( m_maxFrame > 1 )
		Animate( 1 );
	return 1;
}

void CCyclerSprite::Animate( float frames )
{
	pev->frame += frames;
	if ( m_maxFrame > 0 )
		pev->frame = fmod( pev->frame, m_maxFrame );
}

class CWeaponCycler : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	int iItemSlot() override { return 1; }
	int GetItemInfo( ItemInfo* info ) override { return 0; }
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;

	int m_iszModel;
	int m_iModel;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( cycler_weapon, CWeaponCycler );

void CWeaponCycler::Spawn()
{
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	PRECACHE_MODEL( (char*)STRING( pev->model ) );
	SET_MODEL( edict(), STRING( pev->model ) );
	m_iszModel = pev->model;
	m_iModel = pev->modelindex;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	SetTouch( &CWeaponCycler::DefaultTouch );
}

BOOL CWeaponCycler::Deploy()
{
	m_pPlayer->pev->viewmodel = m_iszModel;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	SendWeaponAnim( 0, 0 );
	m_iClip = 0;
	return TRUE;
}

void CWeaponCycler::Holster( int )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CWeaponCycler::PrimaryAttack()
{
	SendWeaponAnim( pev->sequence, 0 );
	m_flNextPrimaryAttack = gpGlobals->time + 0.3;
}

void CWeaponCycler::SecondaryAttack()
{
	pev->sequence = ( pev->sequence + 1 ) % 8;
	pev->modelindex = m_iModel;
	float frameRate, groundSpeed;
	GetSequenceInfo( GET_MODEL_PTR( edict() ), pev, &frameRate, &groundSpeed );
	pev->modelindex = 0;
	if ( frameRate == 0 )
		pev->sequence = 0;
	SendWeaponAnim( pev->sequence, 0 );
	m_flNextSecondaryAttack = gpGlobals->time + 0.3;
}



// Flaming Wreakage
class CWreckage : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void Think() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	int m_flStartTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( cycler_wreckage, CWreckage );

TYPEDESCRIPTION CWreckage::m_SaveData[] =
{
	DEFINE_FIELD( CWreckage, m_flStartTime, FIELD_TIME )
};

IMPLEMENT_SAVERESTORE( CWreckage, CBaseMonster );

void CWreckage::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->effects = 0;
	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1;
	if ( pev->model )
	{
		PRECACHE_MODEL( (char*)STRING( pev->model ) );
		SET_MODEL( edict(), STRING( pev->model ) );
	}
	m_flStartTime = (int)gpGlobals->time;
}

void CWreckage::Precache()
{
	if ( pev->model )
		PRECACHE_MODEL( (char*)STRING( pev->model ) );
}

void CWreckage::Think()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.2;
	if ( pev->dmgtime )
	{
		if ( gpGlobals->time > pev->dmgtime )
		{
			UTIL_Remove( this );
			return;
		}
		if ( !( RANDOM_FLOAT( 0, pev->dmgtime - m_flStartTime ) <= pev->dmgtime - gpGlobals->time ) )
			return;
	}
	Vector source;
	source.x = RANDOM_FLOAT( pev->absmin.x, pev->absmax.x );
	source.y = RANDOM_FLOAT( pev->absmin.y, pev->absmax.y );
	source.z = RANDOM_FLOAT( pev->absmin.z, pev->absmax.z );
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, source );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( source.x );
		WRITE_COORD( source.y );
		WRITE_COORD( source.z );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( RANDOM_LONG( 0, 49 ) + 50 ); // scale * 10
		WRITE_BYTE( RANDOM_LONG( 0, 3 ) + 8 ); // framerate
	MESSAGE_END();
}
