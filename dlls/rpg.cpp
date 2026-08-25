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
#if !OEM_BUILD

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "decals.h"

enum rpg_e 
{
	RPG_IDLE = 0,
	RPG_IDLE_UL,		// unloaded idle
	RPG_FIDGET,
	RPG_FIDGET_UL,	// unloaded fidget
	RPG_FIRE2,		// to empty
	RPG_RELOAD,		// to reload
	RPG_DRAW1,	// loaded
	RPG_DRAW_UL,	// unloaded
	RPG_HOLSTER1,	// loaded
	RPG_HOLSTER2,	// unloaded
};

LINK_ENTITY_TO_CLASS( weapon_rpg, CRpg )

#if !CLIENT_DLL

class CChainsaw : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void Explode(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT BoltThink();
	int sawhealth = 100;
	Vector Direction;

	int m_iTrail;

public:
	static CChainsaw *BoltCreate(Vector dir);
};
LINK_ENTITY_TO_CLASS(chainsaw, CChainsaw);

CChainsaw *CChainsaw::BoltCreate(Vector dir)
{
	// Create a new entity with CCrossbowBolt private data
	CChainsaw *pBolt = GetClassPtr((CChainsaw *)NULL);
	pBolt->pev->classname = MAKE_STRING("chainsaw");
	pBolt->Direction = dir;
	pBolt->Spawn();
	pBolt->sawhealth = 350;

	return pBolt;
}

void CChainsaw::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCEMISSILE;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0;

	SET_MODEL(ENT(pev), "models/w_rpgammo.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, -1), Vector(16, 16, 1));

	SetTouch(&CChainsaw::BoltTouch);
	SetThink(&CChainsaw::BoltThink);
	pev->nextthink = gpGlobals->time + 0.05;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/saw.wav", 1, 0.5);
}


void CChainsaw::Precache()
{
	PRECACHE_MODEL("models/w_rpgammo.mdl");
	PRECACHE_SOUND("weapons/saw.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CChainsaw::Classify(void)
{
	return	CLASS_NONE;
}

void CChainsaw::BoltTouch(CBaseEntity *pOther)
{
	if (pOther->edict() == pev->owner)
		return;

	if (pev->flags & FL_ONGROUND)
		Explode();

	if (pOther->IsBSPModel())
		Explode();
	
	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t	*pevOwner;

		pevOwner = VARS(pev->owner);

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		pOther->pev->velocity = pOther->pev->velocity * 0.1;
		if (pOther->pev->health >= sawhealth)
		{
			pOther->TraceAttack(pevOwner, pOther->pev->health, pev->velocity.Normalize(), &tr, DMG_ALWAYSGIB);
			Explode();
		}
		else
		{
			//pOther->TraceAttack(pevOwner, pOther->pev->health * 2, pev->velocity.Normalize(), &tr, DMG_ALWAYSGIB);
			//sawhealth -= pOther->pev->health;
			pOther->TraceAttack(pevOwner, 6, pev->velocity.Normalize(), &tr, DMG_ALWAYSGIB);
			sawhealth -= 1;

			pev->origin = pev->origin + Direction * 1;
			pev->velocity = Direction * 1;
			pev->speed = 1;
		}

		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/cbar_hitbod1.wav", 0.9, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/cbar_hitbod2.wav", 0.9, ATTN_NORM); break;
		}

		ApplyMultiDamage(pev, pevOwner);
	}
	else
	{
		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(pev->origin);
		}
	}

}

void CChainsaw::BoltThink(void)
{
	pev->nextthink = gpGlobals->time + 0.2;
	//pev->origin = pev->origin + Direction;
	pev->velocity = Direction * 1400;
	pev->speed = 1400;
	pev->avelocity.y = -1200;
	//pev->nextthink = gpGlobals->time + 0.01;
	//pev->origin = pev->origin + Direction * 5;
	//pev->speed = 1600;
}

void CChainsaw::Explode(void)
{
	STOP_SOUND(edict(), CHAN_VOICE, "weapons/saw.wav");

	int iContents = UTIL_PointContents(pev->origin);
	int iScale = 50;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	if (iContents != CONTENTS_WATER)
	{
		WRITE_SHORT(g_sModelIndexFireball);
	}
	else
	{
		WRITE_SHORT(g_sModelIndexWExplosion);
	}
	WRITE_BYTE(iScale); // scale * 10
	WRITE_BYTE(15); // framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	entvars_t *pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage(pev->origin, pev, pevOwner, 200, 200, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB);	

	UTIL_Remove(this);
}

LINK_ENTITY_TO_CLASS( laser_spot, CLaserSpot )

//=========================================================
//Create a transparent turrent laserspot for the wrench gun
//=========================================================
CLaserSpot *CLaserSpot::CreateSpotTurret(void)
{
	CLaserSpot *pSpot = GetClassPtr((CLaserSpot *)NULL);
	pSpot->SpawnTurret();

	pSpot->pev->classname = MAKE_STRING("laser_spot");

	return pSpot;
}

//=========================================================
//=========================================================
void CLaserSpot::SpawnTurret(void)
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/sentry.mdl");
	pev->rendermode = kRenderTransAlpha;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 155;

	UTIL_SetOrigin(pev, pev->origin);
};

//=========================================================
//=========================================================
CLaserSpot *CLaserSpot::CreateSpot( void )
{
	CLaserSpot *pSpot = GetClassPtr( (CLaserSpot *)NULL );
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING( "laser_spot" );

	return pSpot;
}

//=========================================================
//=========================================================
void CLaserSpot::Spawn( void )
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;

	SET_MODEL( ENT( pev ), "sprites/laserdot.spr" );
	UTIL_SetOrigin( pev, pev->origin );
}

//=========================================================
// Suspend- make the laser sight invisible. 
//=========================================================
void CLaserSpot::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;

	SetThink( &CLaserSpot::Revive );
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CLaserSpot::Revive( void )
{
	pev->effects &= ~EF_NODRAW;

	SetThink( NULL );
}

void CLaserSpot::Precache( void )
{
	PRECACHE_MODEL( "sprites/laserdot.spr" );
}

LINK_ENTITY_TO_CLASS( rpg_rocket, CRpgRocket )

//=========================================================
//=========================================================
CRpgRocket *CRpgRocket::CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher )
{
	CRpgRocket *pRocket = GetClassPtr( (CRpgRocket *)NULL );

	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch( &CRpgRocket::RocketTouch );
	pRocket->m_hLauncher = pLauncher;// remember what RPG fired me. 
	pLauncher->m_cActiveRockets++;// register this missile as active for the launcher
	pRocket->pev->owner = pOwner->edict();

	return pRocket;
}

void CRpgRocket::Explode( TraceResult *pTrace, int bitsDamageType )
{
	if( CRpg *pLauncher = GetLauncher())
	{
		// my launcher is still around, tell it I'm dead.
		pLauncher->m_cActiveRockets--;
		m_hLauncher = 0;
	}

	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav" );

	CGrenade::Explode( pTrace, bitsDamageType );
}

CRpg *CRpgRocket::GetLauncher( void )
{
	return (CRpg*)( (CBaseEntity*)m_hLauncher );
}

//=========================================================
//=========================================================
void CRpgRocket::Spawn( void )
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/rpgrocket.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->classname = MAKE_STRING( "rpg_rocket" );

	SetThink( &CRpgRocket::IgniteThink );
	SetTouch( &CGrenade::ExplodeTouch );

	pev->angles.x -= 30.0f;
	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -( pev->angles.x + 30.0f );

	pev->velocity = gpGlobals->v_forward * 250.0f;
	pev->gravity = 0.5f;

	pev->nextthink = gpGlobals->time + 0.4f;

	pev->dmg = gSkillData.plrDmgRPG;
}

//=========================================================
//=========================================================
void CRpgRocket::RocketTouch( CBaseEntity *pOther )
{
	if( CRpg *pLauncher = GetLauncher())
	{
		// my launcher is still around, tell it I'm dead.
		pLauncher->m_cActiveRockets--;
		m_hLauncher = 0;
	}

	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav" );
	ExplodeTouch( pOther );
}

//=========================================================
//=========================================================
void CRpgRocket::Precache( void )
{
	PRECACHE_MODEL( "models/rpgrocket.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
}

void CRpgRocket::IgniteThink( void )
{
	// pev->movetype = MOVETYPE_TOSS;

	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5f );

	// rocket trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );	// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( 40 ); // life
		WRITE_BYTE( 5 );  // width
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	m_flIgniteTime = gpGlobals->time;

	// set to follow laser spot
	SetThink( &CRpgRocket::FollowThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CRpgRocket::FollowThink( void )
{
	CBaseEntity *pOther = NULL;
	Vector vecTarget;
	Vector vecDir;
	float flDist, flMax, flDot;
	TraceResult tr;

	UTIL_MakeAimVectors( pev->angles );

	vecTarget = gpGlobals->v_forward;
	flMax = 4096;
	
	// Examine all entities within a reasonable radius
	while( ( pOther = UTIL_FindEntityByClassname( pOther, "laser_spot" ) ) != NULL )
	{
		UTIL_TraceLine( pev->origin, pOther->pev->origin, dont_ignore_monsters, ENT( pev ), &tr );
		// ALERT( at_console, "%f\n", tr.flFraction );
		if( tr.flFraction >= 0.9f )
		{
			vecDir = pOther->pev->origin - pev->origin;
			flDist = vecDir.Length();
			vecDir = vecDir.Normalize();
			flDot = DotProduct( gpGlobals->v_forward, vecDir );
			if( ( flDot > 0 ) && ( flDist * ( 1 - flDot ) < flMax ) )
			{
				flMax = flDist * ( 1 - flDot );
				vecTarget = vecDir;
			}
		}
	}

	pev->angles = UTIL_VecToAngles( vecTarget );

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();
	if( gpGlobals->time - m_flIgniteTime < 1.0f )
	{
		pev->velocity = pev->velocity * 0.2f + vecTarget * ( flSpeed * 0.8f + 400.0f );
		if( pev->waterlevel == 3 )
		{
			// go slow underwater
			if( pev->velocity.Length() > 300.0f )
			{
				pev->velocity = pev->velocity.Normalize() * 300.0f;
			}
			UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1f, pev->origin, 4 );
		} 
		else 
		{
			if( pev->velocity.Length() > 2000.0f )
			{
				pev->velocity = pev->velocity.Normalize() * 2000.0f;
			}
		}
	}
	else
	{
		if( pev->effects & EF_LIGHT )
		{
			pev->effects = 0;
			STOP_SOUND( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav" );
		}
		pev->velocity = pev->velocity * 0.2f + vecTarget * flSpeed * 0.798f;
		if( pev->waterlevel == 0 && pev->velocity.Length() < 1500.0f )
			Detonate();
	}
	// ALERT( at_console, "%.0f\n", flSpeed );

	if( CRpg *pLauncher = GetLauncher())
	{
		if( ( pev->origin - pLauncher->pev->origin ).Length() > 8192 || gpGlobals->time - m_flIgniteTime > 6.0f )
		{
			// my launcher is still around, tell it I'm dead.
			pLauncher->m_cActiveRockets--;
			m_hLauncher = 0;
		}
	}

	if( UTIL_PointContents( pev->origin ) == CONTENTS_SKY )
		Detonate();

	pev->nextthink = gpGlobals->time + 0.1f;
}
#endif

void CRpg::Reload( void )
{
	int iResult = 0;

	// don't bother with any of this if don't need to reload.
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == RPG_MAX_CLIP )
		return;

	// because the RPG waits to autoreload when no missiles are active while  the LTD is on, the
	// weapons code is constantly calling into this function, but is often denied because 
	// a) missiles are in flight, but the LTD is on
	// or
	// b) player is totally out of ammo and has nothing to switch to, and should be allowed to
	//    shine the designator around
	//
	// Set the next attack time into the future so that WeaponIdle will get called more often
	// than reload, allowing the RPG LTD to be updated
	
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.5f );

	if( m_cActiveRockets && m_fSpotActive )
	{
		// no reloading when there are active missiles tracking the designator.
		// ward off future autoreload attempts by setting next attack time into the future for a bit. 
		return;
	}

#if !CLIENT_DLL
	if( m_pSpot && m_fSpotActive )
	{
		m_pSpot->Suspend( 1.8f );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.8f;
	}
#endif

	if( m_iClip == 0 )
		iResult = DefaultReload( RPG_MAX_CLIP, RPG_RELOAD, 1.8 );

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/chainsaw.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));

	if( iResult )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CRpg::Spawn()
{
	Precache();
	m_iId = WEAPON_RPG;

	SET_MODEL( ENT( pev ), "models/w_rpg.mdl" );
	m_fSpotActive = false;

#if CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
	{
		// more default ammo in multiplay. 
		m_iDefaultAmmo = RPG_DEFAULT_GIVE * 2;
	}
	else
	{
		m_iDefaultAmmo = RPG_DEFAULT_GIVE;
	}

	FallInit();// get ready to fall down.
}

void CRpg::Precache( void )
{
	PRECACHE_MODEL( "models/w_rpg.mdl" );
	PRECACHE_MODEL( "models/v_rpg.mdl" );
	PRECACHE_MODEL( "models/p_rpg.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND("weapons/chainsaw.wav");

	UTIL_PrecacheOther( "laser_spot" );
	UTIL_PrecacheOther( "rpg_rocket" );
	UTIL_PrecacheOther("chainsaw");

	PRECACHE_SOUND( "weapons/rocketfire1.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" ); // alternative fire sound

	m_usRpg = PRECACHE_EVENT( 1, "events/rpg.sc" );
}

int CRpg::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "rockets";
	p->iMaxAmmo1 = ROCKET_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = RPG_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_RPG;
	p->iFlags = 0;
	p->iWeight = RPG_WEIGHT;

	return 1;
}

int CRpg::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CRpg::Deploy()
{
	if( m_iClip == 0 )
	{
		return DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW_UL, "rpg" );
	}

	return DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW1, "rpg" );
}

BOOL CRpg::CanHolster( void )
{
	if( m_fSpotActive && m_cActiveRockets )
	{
		// can't put away while guiding a missile.
		return FALSE;
	}

	return TRUE;
}

void CRpg::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	SendWeaponAnim( RPG_HOLSTER1 );

#if !CLIENT_DLL
	if( m_pSpot )
	{
		m_pSpot->Killed( NULL, GIB_NEVER );
		m_pSpot = NULL;
	}
#endif
}

void CRpg::PrimaryAttack()
{
	if( m_iClip )
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

#if !CLIENT_DLL
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
		UTIL_MakeVectors(anglesAim);
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 24;
		Vector vecDir = gpGlobals->v_forward;

		//CRpgRocket *pRocket = CRpgRocket::CreateRpgRocket(vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this);

		//UTIL_MakeVectors(m_pPlayer->pev->v_angle);// RpgRocket::Create stomps on globals, so remake.
		//pRocket->pev->velocity = pRocket->pev->velocity + gpGlobals->v_forward * DotProduct(m_pPlayer->pev->velocity, gpGlobals->v_forward);

		CChainsaw *pBolt = CChainsaw::BoltCreate(vecDir);
		pBolt->pev->owner = m_pPlayer->edict();
		pBolt->pev->origin = vecSrc;
		pBolt->pev->angles = anglesAim;
		pBolt->pev->velocity = vecDir * 1000;
		pBolt->pev->speed = 1000;
		//pBolt->pev->avelocity.x = -900;
		pBolt->pev->angles.z = 90;
		pBolt->pev->avelocity.y = -1200;
		//pBolt->pev->avelocity.z = -900;
#endif

		// firing RPG no longer turns on the designator. ALT fire is a toggle switch for the LTD.
		// Ken signed up for this as a global change (sjb)

		int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usRpg );

		m_iClip--; 

		m_flNextPrimaryAttack = GetNextAttackDelay( 1.5f );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5f;

		ResetEmptySound();
	}
	else
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2f;
	}
	UpdateSpot();
}

void CRpg::SecondaryAttack()
{
	m_fSpotActive = false;

#if !CLIENT_DLL
	if( !m_fSpotActive && m_pSpot )
	{
		m_pSpot->Killed( NULL, GIB_NORMAL );
		m_pSpot = NULL;
	}
#endif
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2f;
}

void CRpg::WeaponIdle( void )
{
	UpdateSpot();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		ResetEmptySound();

		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
		if( flRand <= 0.75f || m_fSpotActive )
		{
			if( m_iClip == 0 )
				iAnim = RPG_IDLE_UL;
			else
				iAnim = RPG_IDLE;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0f / 15.0f;
		}
		else
		{
			if( m_iClip == 0 )
				iAnim = RPG_FIDGET_UL;
			else
				iAnim = RPG_FIDGET;
#if WEAPONS_ANIMATION_TIMES_FIX
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.1f;
#else
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
#endif
		}

		SendWeaponAnim( iAnim );
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	}
}

void CRpg::UpdateSpot( void )
{
#if !CLIENT_DLL
	if( m_fSpotActive )
	{
		if (m_pPlayer->pev->viewmodel == 0)
			return;

		if( !m_pSpot )
		{
			m_pSpot = CLaserSpot::CreateSpot();
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 8192.0f, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

		UTIL_SetOrigin( m_pSpot->pev, tr.vecEndPos );
	}
#endif
}

class CRpgAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/w_rpgammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_rpgammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int iGive;
#if CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
		{
			// hand out more ammo per rocket in multiplayer.
			iGive = AMMO_RPGCLIP_GIVE * 2;
		}
		else
		{
			iGive = AMMO_RPGCLIP_GIVE;
		}

		if( pOther->GiveAmmo( iGive, "rockets", ROCKET_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_rpgclip, CRpgAmmo )
#endif
