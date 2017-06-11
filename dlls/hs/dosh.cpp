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
#if !defined( OEM_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
//#include "gamerules.h"

#include "soundent.h"
#include "gamerules.h"

enum dosh_e {
	DOSH_IDLE = 0,
	DOSH_FIDGET,
	DOSH_PINPULL,
	DOSH_THROW1,		// to empty
	DOSH_THROW2,	// loaded
	DOSH_THROW3,		// loaded
};

LINK_ENTITY_TO_CLASS( weapon_dosh, CDosh );

#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS( dosh_rocket, CDoshRocket );

//=========================================================
//=========================================================
CDoshRocket *CDoshRocket::CreateDoshRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CDosh *pLauncher )
{
	CDoshRocket *pRocket = GetClassPtr( (CDoshRocket *)NULL );

	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch( &CDoshRocket::RocketTouch );
	pRocket->m_pLauncher = pLauncher;// remember what RPG fired me. 
	pRocket->m_pLauncher->m_cActiveRockets++;// register this missile as active for the launcher
	pRocket->pev->owner = pOwner->edict();

	return pRocket;
}

//=========================================================
//=========================================================
void CDoshRocket :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	//SET_MODEL(ENT(pev), "models/doshrocket.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	pev->classname = MAKE_STRING("dosh_rocket");

	SetThink( &CDoshRocket::IgniteThink );
	SetTouch( &CDoshRocket::DoshTouch );

	pev->angles.x -= 30;
	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -(pev->angles.x + 30);

	pev->velocity = gpGlobals->v_forward * 250;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.4;

	pev->dmg = gSkillData.plrDmgDosh;
}

//=========================================================
//=========================================================
void CDoshRocket :: RocketTouch ( CBaseEntity *pPlayer )
{
	if ( m_pLauncher )
	{
		// my launcher is still around, tell it I'm dead.
		m_pLauncher->m_cActiveRockets--;
	}

	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/doshget.wav", 1, 0.5 );
	ExplodeTouch2( pPlayer );
}

void CDoshRocket::DoshTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		pev->velocity = Vector( 0, 0, 0 );

		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/doshget.wav", 1, ATTN_NORM);

		UTIL_Remove( this );
	}
	else
	{
		SetThink( &CBaseEntity::SUB_Remove );

		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if ( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize( );
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
			//pev->angles = UTIL_VecToAngles( vecDir );
			pev->angles = Vector( 0, 0, 0 ); //Should reset it upright.
			//pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.y = 100; //Simulate the Killing Floor spinning
			pev->solid = SOLID_BBOX;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			//pev->angles.x = RANDOM_LONG(0,360);
			pev->nextthink = gpGlobals->time + 10.0;
		}
	}
}

//=========================================================
//=========================================================
void CDoshRocket :: Precache( void )
{
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND ("weapons/doshget.wav");
}


void CDoshRocket :: IgniteThink( void  )
{

	pev->movetype = MOVETYPE_FLY;
	m_flIgniteTime = gpGlobals->time;
}

#endif
void CDosh::Reload( void )
{
		return;
}


void CDosh::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DOSH;

	SET_MODEL(ENT(pev), "models/w_dosh.mdl");

	FallInit();// get ready to fall down.
}


void CDosh::Precache( void )
{
	PRECACHE_MODEL("models/w_dosh.mdl");
	PRECACHE_MODEL("models/v_dosh.mdl");
	PRECACHE_MODEL("models/p_dosh.mdl");
	PRECACHE_MODEL("models/dosh.mdl");	//Jason!

	PRECACHE_SOUND("items/9mmclip1.wav");

//	UTIL_PrecacheOther( "laser_spot" );
	UTIL_PrecacheOther( "dosh_rocket" );

	PRECACHE_SOUND("weapons/dosh1.wav"); //HERES SOME CASH GUYS SOMEONE TAKE IT
	PRECACHE_SOUND("weapons/dosh2.wav"); //HERES SOME CASH GUYS SOMEONE TAKE IT
	PRECACHE_SOUND("weapons/dosh3.wav"); //HERES SOME CASH GUYS SOMEONE TAKE IT
	PRECACHE_SOUND("weapons/dosh4.wav"); //HERES SOME CASH GUYS SOMEONE TAKE IT
	PRECACHE_SOUND("weapons/dosh5.wav"); //HERES SOME CASH GUYS SOMEONE TAKE IT
	//PRECACHE_SOUND("weapons/dosh6.wav"); //Did I miss the dosh party?
	PRECACHE_SOUND("weapons/doshget.wav"); //HERES SOME CASH GUYS SOMEONE TAKE IT

	m_usDosh = PRECACHE_EVENT ( 1, "events/dosh.sc" );
	m_usDosh2 = PRECACHE_EVENT( 1, "events/dosh2.sc" );
}


int CDosh::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = DOSH_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_DOSH;
	p->iFlags = 0;
	p->iWeight = DOSH_WEIGHT;

	return 1;
}

int CDosh::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CDosh::Deploy( )
{
	if ( m_iClip == 0 )
	{
		return DefaultDeploy( "models/v_dosh.mdl", "models/p_dosh.mdl", DOSH_IDLE, "crowbar" );
	}

	return DefaultDeploy( "models/v_dosh.mdl", "models/p_dosh.mdl", DOSH_IDLE, "crowbar" );
}


BOOL CDosh::CanHolster( void )
{

	return TRUE;
}

void CDosh::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	
	SendWeaponAnim( DOSH_THROW3 );

#ifndef CLIENT_DLL
	
#endif

}

void CDosh::PrimaryAttack()
{
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

 	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// we don't add in player velocity anymore.
	CGrenade::ShootContact2( m_pPlayer->pev, 
							m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 60, 
							gpGlobals->v_forward * 512 );

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usDosh ); 

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;// idle pretty soon after shooting.
}

void CDosh::WeaponIdle( void )
{
	//UpdateSpot( );

	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.75 || m_fSpotActive)
		{
				iAnim = DOSH_IDLE;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 15.0;
		}
		else
		{
				iAnim = DOSH_FIDGET;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;
		}

		SendWeaponAnim( iAnim );
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
	}
}

#endif