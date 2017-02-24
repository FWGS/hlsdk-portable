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
#include "sprite.h"
#include "com_model.h"
#include "customweapons.h"

#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY	2700
#define BOLT_WATER_VELOCITY	2000

class CGateOfBabylonSpawner;
#define MAX_SPAWNERS 7
class CGateOfBabylon : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 0; }
	int GetItemInfo(ItemInfo *p);
	int ObjectCaps();
#ifndef CLIENT_DLL
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
#endif
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );

	virtual BOOL UseDecrement( void )
	{
//#if defined( CLIENT_WEAPONS )
//		return ;
//#else
		return false;
//#endif
	}

	CGateOfBabylonSpawner *m_pSpawners[MAX_SPAWNERS];
	bool IntersectOtherSpawner( CGateOfBabylonSpawner *spawner );

private:

	void AddSpawners( void );
	int m_iSpawnerCount;
};


class CGateOfBabylonBolt : public CBaseEntity
{
public:
	static CGateOfBabylonBolt *BoltCreate( void );

private:
	void Spawn( void );
	void Precache( void );
	void EXPORT BubbleThink( void );
	void EXPORT BoltTouch( CBaseEntity *pOther );
	void EXPORT RemoveThink();
	void Trail( void );
	float TouchGravGun( CBaseEntity *attacker, int stage );
	int m_iTrail;
};

class CGateOfBabylonSpawner : public CBaseEntity
{
public:
	static CGateOfBabylonSpawner *CreateSpawner( CGateOfBabylon *pGates, int iNumber );

private:
	void Spawn( void );
	void Precache( void );
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	void UpdatePosition( void );
	void Animate( void );
	void EXPORT FollowPlayerThink( void );
	bool FireBolts( void );

	EHBasePlayerItem m_pGates;
	Vector m_vecOffset;
	float m_flNextNPThrow;
	float m_flLastTimeAnim;
	int m_iNumber;
	int m_iMaxFrames;

	friend class CGateOfBabylon;
};

LINK_ENTITY_TO_CLASS( gateofbabylon_spawner, CGateOfBabylonSpawner );
LINK_ENTITY_TO_CLASS( weapon_gateofbabylon, CGateOfBabylon );
LINK_ENTITY_TO_CLASS( gateofbabylon_bolt, CGateOfBabylonBolt );

enum gauss_e
{
	CROWBAR_IDLE = 0,
	CROWBAR_DRAW,
	CROWBAR_HOLSTER,
	CROWBAR_ATTACK1HIT,
	CROWBAR_ATTACK1MISS,
	CROWBAR_ATTACK2MISS,
	CROWBAR_ATTACK2HIT,
	CROWBAR_ATTACK3MISS,
	CROWBAR_ATTACK3HIT
};

static float UTIL_MyWeaponTimeBase( void )
{
	return gpGlobals->time;
}

CGateOfBabylonBolt *CGateOfBabylonBolt::BoltCreate( void )
{
	// Create a new entity with CGateOfBabylonBolt private data
	CGateOfBabylonBolt *pBolt = GetClassPtr( (CGateOfBabylonBolt *)NULL );
	pBolt->pev->classname = MAKE_STRING( "gateofbabylon_bolt" );	// g-cont. enable save\restore
	pBolt->Spawn();
	return pBolt;
}

void CGateOfBabylonBolt::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;
	pev->renderamt = 255;
	pev->rendermode = kRenderTransColor;

	SET_MODEL( ENT( pev ), "models/w_crowbar.mdl" );

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	SetTouch( &CGateOfBabylonBolt::BoltTouch );
	SetThink( &CGateOfBabylonBolt::BubbleThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CGateOfBabylonBolt::Precache()
{
	PRECACHE_MODEL( "models/w_crowbar.mdl" );
	PRECACHE_SOUND( "weapons/xbow_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/xbow_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/xbow_fly1.wav" );
	PRECACHE_SOUND( "weapons/xbow_hit1.wav" );
	PRECACHE_SOUND( "fvox/beep.wav" );
	m_iTrail = PRECACHE_MODEL( "sprites/lgtning.spr" );
}

void CGateOfBabylonBolt::RemoveThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
	pev->renderamt -= 2;
	if( pev->renderamt <= 0 )
	{
		SUB_Remove();
	}
}

void CGateOfBabylonBolt::BoltTouch( CBaseEntity *pOther )
{
	const char *szSoundName;
	TraceResult tr = UTIL_GetGlobalTrace();

	if( pOther->pev->owner == pev->owner &&
		FClassnameIs( pOther->pev, "gateofbabylon_bolt" ) )
	{
		return; // ignore same entities from same player
	}

	SetTouch( NULL );
	SetThink( NULL );

	if( pOther->pev->takedamage )
	{
		entvars_t *pevOwner;
		int bitsDamageType = DMG_ALWAYSGIB;
		float flDamage;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		if( pOther->IsPlayer() )
		{
			flDamage = gSkillData.plrDmgCrossbowClient / 3.5f;
		}
		else
		{
			flDamage = gSkillData.plrDmgCrossbowMonster / 3.5f;
			bitsDamageType |= DMG_BULLET | DMG_ALWAYSGIB;
		}
		pOther->TraceAttack( pevOwner, flDamage, pev->velocity.Normalize(), &tr, bitsDamageType);

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = g_vecZero;
		// play body "thwack" sound
		switch( RANDOM_LONG( 0, 1 ) )
		{
		case 0: szSoundName = "weapons/xbow_hitbod1.wav"; break;
		case 1: szSoundName = "weapons/xbow_hitbod2.wav"; break;
		}
		EMIT_SOUND( ENT( pev ), CHAN_BODY, szSoundName, 1, ATTN_NORM );

		if( !g_pGameRules->IsMultiplayer() )
		{
			Killed( pev, GIB_NEVER );
		}
	}
	else
	{
		switch( RANDOM_LONG( 0, 1 ) )
		{
		case 0: szSoundName = "weapons/xbow_hit1.wav"; break;
		case 1:	szSoundName = "weapons/xbow_hit2.wav"; break;
		}

		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, szSoundName, RANDOM_FLOAT( 0.95, 1.0 ), ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 7 ) );

		float saveRoll = pev->angles.z;

		SetThink( &CGateOfBabylonBolt::RemoveThink );
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();

			DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

			UTIL_SetOrigin( pev, pev->origin - vecDir * RANDOM_FLOAT( 10, 15 ) );
			pev->angles = UTIL_VecToAngles( -vecDir );
			pev->angles.z = saveRoll;
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = g_vecZero;
			pev->avelocity.z = 0;
			pev->nextthink = gpGlobals->time + 10.0;
		}
		else if( pOther->pev->movetype == MOVETYPE_PUSH || pOther->pev->movetype == MOVETYPE_PUSHSTEP )
		{
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin( pev, pev->origin - vecDir * RANDOM_FLOAT( 10, 15 ) );
			pev->angles = UTIL_VecToAngles( -vecDir );
			pev->angles.z = saveRoll;
			pev->solid = SOLID_NOT;
			pev->velocity = g_vecZero;
			pev->avelocity.z = 0;
			pev->nextthink = gpGlobals->time + 10.0;

			// g-cont. Setup movewith feature
			if( gPhysicsInterfaceInitialized )
			{
				pev->movetype = MOVETYPE_COMPOUND;	// set movewith type
				pev->aiment = ENT( pOther->pev );	// set parent
			}
		}

		if( UTIL_PointContents( pev->origin ) != CONTENTS_WATER )
		{
			UTIL_Sparks( pev->origin );
		}
	}
}

void CGateOfBabylonBolt::Trail( void )
{
	// trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	{
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );	// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( 10 ); // life
		WRITE_BYTE( 5 );  // width
		switch( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 128 );   // r, g, b
			WRITE_BYTE( 14 );   // r, g, b
			break;
		case 1:
			WRITE_BYTE( 192 );   // r, g, b
			WRITE_BYTE( 128 );   // r, g, b
			WRITE_BYTE( 14 );   // r, g, b
			break;
		}
		WRITE_BYTE( 255 );	// brightness
	}
	MESSAGE_END();
}



void CGateOfBabylonBolt::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	Trail();

	if( pev->waterlevel != 0 )
	{
		UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
	}
}

float CGateOfBabylonBolt::TouchGravGun(CBaseEntity *attacker, int stage)
{
	if( stage >= 2 )
	{
		pev->movetype = MOVETYPE_FLY;
		pev->solid = SOLID_BBOX;
		pev->nextthink = gpGlobals->time + 60.0;
		SetTouch( &CGateOfBabylonBolt::BoltTouch );
		UTIL_MakeVectors( attacker->pev->v_angle + attacker->pev->punchangle);
		pev->angles = UTIL_VecToAngles(-gpGlobals->v_forward);
		UTIL_SetOrigin( pev, pev->origin );
		SetThink( &CGateOfBabylonBolt::BubbleThink );
	}
	return 2000;
}
#endif

void CGateOfBabylonSpawner::Spawn( void )
{
	Precache();

	SET_MODEL( ENT(pev), "sprites/tele1.spr" );
	m_iMaxFrames = MODEL_FRAMES( pev->modelindex ) - 1;

	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;

	pev->scale = RANDOM_FLOAT( 0.25, 0.4 );
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->framerate = 10.0f;

	m_flNextNPThrow = 0;
	m_flLastTimeAnim = 0;
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	SetTouch( NULL );
	pev->nextthink = gpGlobals->time + 0.001;
}

void CGateOfBabylonSpawner::Precache( void )
{
	PRECACHE_MODEL( "sprites/tele1.spr" );
}

void CGateOfBabylonSpawner::UpdatePosition( void )
{
	UTIL_MakeVectors( m_pGates->m_pPlayer->pev->angles );

	pev->origin = m_pGates->m_pPlayer->pev->origin +
				  gpGlobals->v_right * m_vecOffset.x +
				  gpGlobals->v_up    * m_vecOffset.y;
	UTIL_SetOrigin( pev, pev->origin );

	Vector angles = m_pGates->m_pPlayer->pev->angles;
	angles.x = 0;
	pev->angles = angles;
}

void CGateOfBabylonSpawner::Animate( void )
{
	pev->frame += pev->framerate * (gpGlobals->time - m_flLastTimeAnim);
	if( pev->frame > m_iMaxFrames )
	{
		if( m_iMaxFrames > 0 )
			pev->frame = fmod( pev->frame, m_iMaxFrames );
	}

	m_flLastTimeAnim = gpGlobals->time;
}

void CGateOfBabylonSpawner::FollowPlayerThink( void )
{
	pev->nextthink = gpGlobals->time + 0.01; // update at 100 fps

	if( !m_pGates )
		return; // wait for gates

	if( !m_pGates->m_pPlayer || !m_pGates->m_pPlayer->m_pActiveItem || m_pGates->m_pPlayer->pev->deadflag > DEAD_NO )
	{
		SetThink( &CBaseEntity::SUB_Remove );
		return;
	}

	// don't draw if player changed weapon
	if( m_pGates->m_pPlayer->m_pActiveItem->m_iId != WEAPON_GATEOFBABYLON )
	{
		if( !(pev->effects & EF_NODRAW) )
			pev->effects |= EF_NODRAW;
	}
	else
	{
		// restore drawing
		if( pev->effects & EF_NODRAW )
			pev->effects &= ~EF_NODRAW;

		UpdatePosition();
		Animate();
	}
}

CGateOfBabylonSpawner *CGateOfBabylonSpawner::CreateSpawner(
		CGateOfBabylon *pGates,
		int iNumber )
{
	// Create a new entity with CGateOfBabylonBolt private data
	CGateOfBabylonSpawner *pBolt = GetClassPtr( (CGateOfBabylonSpawner *)NULL );
	pBolt->pev->classname = MAKE_STRING( "gateofbabylon_spawner" );	// g-cont. enable save\r/estore
	pBolt->m_pGates = pGates;
	pBolt->m_iNumber = iNumber;

	int i = 0;
	do
	{
		pBolt->m_vecOffset.x = RANDOM_FLOAT( 10, 50 ) * (iNumber & 1 ? 1: -1) ;
		pBolt->m_vecOffset.y = RANDOM_FLOAT( 10, 60 );
	} while( pGates->IntersectOtherSpawner( pBolt ) && i++ < 10 ); // don't go into infinite loop


	pBolt->SetThink( &CGateOfBabylonSpawner::FollowPlayerThink );

	pBolt->Spawn();

	return pBolt;
}

bool CGateOfBabylonSpawner::FireBolts( void )
{
	TraceResult tr;
	CBasePlayer *pPlayer = m_pGates->m_pPlayer;

	if( m_flNextNPThrow > gpGlobals->time )
	{
		return false;
	}

	m_flNextNPThrow = gpGlobals->time + RANDOM_FLOAT( 0.5, 1.0 );

	Vector anglesAim = pPlayer->pev->v_angle;
	UTIL_MakeVectors( anglesAim );

	Vector vecSrc	= pev->origin + gpGlobals->v_up * 2;
	Vector vecDir;
	UTIL_TraceLine( vecSrc, pPlayer->GetGunPosition() + gpGlobals->v_forward * 8192,
					dont_ignore_monsters, ignore_glass, pPlayer->edict(), &tr );


	if( tr.fStartSolid )
		return false;

	if( tr.flFraction != 1.0f )
	{
		vecDir = (pPlayer->GetGunPosition() + gpGlobals->v_forward * ( tr.vecEndPos - vecSrc ).Length()) - vecSrc;
		vecDir = vecDir.Normalize();
	}
	else
	{
		return false;
	}

	vecDir = vecDir + gpGlobals->v_right * RANDOM_FLOAT( 0, 0.02 ) -
			 gpGlobals->v_up    * RANDOM_FLOAT( 0, 0.02 );

#ifndef CLIENT_DLL
	CGateOfBabylonBolt *pBolt = CGateOfBabylonBolt::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = UTIL_VecToAngles( -gpGlobals->v_forward );
	pBolt->pev->angles.z = RANDOM_FLOAT(0, 360);
	pBolt->pev->owner = pPlayer->edict();

	if( pPlayer->pev->waterlevel == 3 )
	{
		pBolt->pev->velocity = vecDir * BOLT_WATER_VELOCITY;
		pBolt->pev->speed = BOLT_WATER_VELOCITY;
	}
	else
	{
		pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
		pBolt->pev->speed = BOLT_AIR_VELOCITY;
	}
	pBolt->pev->avelocity.z = 10;
#endif

	return true;
}

TYPEDESCRIPTION CGateOfBabylonSpawner::m_SaveData[] =
{
	DEFINE_FIELD( CGateOfBabylonSpawner, m_iNumber, FIELD_INTEGER ),
	DEFINE_FIELD( CGateOfBabylonSpawner, m_iMaxFrames, FIELD_INTEGER ),
	DEFINE_FIELD( CGateOfBabylonSpawner, m_vecOffset, FIELD_VECTOR ),
	DEFINE_FIELD( CGateOfBabylonSpawner, m_flNextNPThrow, FIELD_TIME ),
	DEFINE_FIELD( CGateOfBabylonSpawner, m_flLastTimeAnim, FIELD_TIME ),
	DEFINE_FIELD( CGateOfBabylonSpawner, m_pGates, FIELD_CLASSPTR )
};

IMPLEMENT_SAVERESTORE( CGateOfBabylonSpawner, CBaseEntity );

TYPEDESCRIPTION	CGateOfBabylon::m_SaveData[] =
{
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_FIELD( CGateOfBabylon, m_iSpawnerCount, FIELD_INTEGER ),
	DEFINE_ARRAY( CGateOfBabylon, m_pSpawners, FIELD_CLASSPTR, MAX_SPAWNERS )
};

IMPLEMENT_SAVERESTORE( CGateOfBabylon, CBasePlayerWeapon );


int CGateOfBabylon::ObjectCaps()
{
	return FCAP_ACROSS_TRANSITION;
}

void CGateOfBabylon::Spawn()
{
	Precache();
	m_iId = WEAPON_GATEOFBABYLON;
	//m_iSpawnerCount = 0;
	SET_MODEL( ENT( pev ), "models/w_crowbar.mdl" );

	FallInit();// get ready to fall down.
}

int CGateOfBabylon::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();

		m_iSpawnerCount = 0;

		//AddSpawners();

		return TRUE;
	}
	return FALSE;
}

void CGateOfBabylon::Precache( void )
{
	PRECACHE_MODEL( "models/w_crowbar.mdl" );
	PRECACHE_MODEL( "models/v_crowbar.mdl" );
	PRECACHE_MODEL( "models/p_crowbar.mdl" );

	PRECACHE_SOUND( "weapons/xbow_fire1.wav" );
	PRECACHE_SOUND( "weapons/xbow_reload1.wav" );

	UTIL_PrecacheOther( "gateofbabylon_bolt" );
	UTIL_PrecacheOther( "gateofbabylon_spawner" );
}

int CGateOfBabylon::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = WEAPON_NOCLIP;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 3;
	p->iId = WEAPON_GATEOFBABYLON;
	p->iWeight = 9999; // superimba, switch everytime
	p->iFlags = 0;
	return 1;
}

BOOL CGateOfBabylon::Deploy()
{
	return DefaultDeploy( "models/v_crowbar.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar" );
}

void CGateOfBabylon::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_MyWeaponTimeBase() + 0.01;
	SendWeaponAnim( CROWBAR_HOLSTER );
}

void CGateOfBabylon::PrimaryAttack( void )
{
	BOOL fire = FALSE;
	int j = 0;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	if( !m_iSpawnerCount )
	{
		PlayEmptySound();
		return;
	}

	do
	{
		int i = RANDOM_LONG( 0, m_iSpawnerCount - 1 );

		if( m_pSpawners[i] )
		{
			fire = m_pSpawners[i]->FireBolts();
		}
	} while( !fire && j++ < m_iSpawnerCount); // give up after some retries

	if( fire )
	{
		SendWeaponAnim( RANDOM_LONG( CROWBAR_ATTACK1HIT, CROWBAR_ATTACK3HIT ) );

		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}
	else
	{
		PlayEmptySound();
	}


	m_flNextPrimaryAttack = UTIL_MyWeaponTimeBase() + 0.1;
	m_flNextSecondaryAttack = UTIL_MyWeaponTimeBase() + 1;
}

void CGateOfBabylon::SecondaryAttack()
{
	if( m_iSpawnerCount == MAX_SPAWNERS )
	{
		// remove them all!
		for( m_iSpawnerCount = MAX_SPAWNERS - 1; m_iSpawnerCount >= 0; m_iSpawnerCount-- )
		{
			UTIL_Remove( m_pSpawners[m_iSpawnerCount] );
			m_pSpawners[m_iSpawnerCount] = NULL;
		}
		m_iSpawnerCount = 0;
	}
	else
	{
		UTIL_Remove( m_pSpawners[m_iSpawnerCount]);

		m_pSpawners[m_iSpawnerCount] = CGateOfBabylonSpawner::CreateSpawner( this, m_iSpawnerCount );
		m_iSpawnerCount++;
	}

	pev->nextthink = UTIL_MyWeaponTimeBase() + 1;
	m_flNextSecondaryAttack = UTIL_MyWeaponTimeBase() + 1;
}

void CGateOfBabylon::AddSpawners( void )
{
#if 0
	CBaseEntity *pEnt = NULL;
	CGateOfBabylonSpawner *pSpawner;
	int iSpawnerCount = 0;

	while( ( pEnt = UTIL_FindEntityInSphere( pEnt, m_pPlayer->pev->origin, 100 ) ) )
	{
		if( !FClassnameIs( pEnt->pev, "gateofbabylon_spawner" ) )
			continue;

		pSpawner = static_cast<CGateOfBabylonSpawner*>( pEnt );

		UTIL_Remove( m_pSpawners[iSpawnerCount] );

		m_pSpawners[iSpawnerCount] = pSpawner;
		pSpawner->m_iNumber = iSpawnerCount;
		pSpawner->m_pGates = this;
		pSpawner->SetThink( &CGateOfBabylonSpawner::FollowPlayerThink );
		iSpawnerCount++;
	}

	m_iSpawnerCount = iSpawnerCount;
#endif
}

void CGateOfBabylon::Reload( void )
{
}

void CGateOfBabylon::WeaponIdle( void )
{
	ResetEmptySound();

	if( m_flTimeWeaponIdle >= UTIL_MyWeaponTimeBase() )
		return;

	m_flTimeWeaponIdle = UTIL_MyWeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	SendWeaponAnim( CROWBAR_IDLE );
}

bool CGateOfBabylon::IntersectOtherSpawner( CGateOfBabylonSpawner *spawner )
{
	for( int i = 0; i < m_iSpawnerCount; i++ )
	{
		CGateOfBabylonSpawner *o = m_pSpawners[i];

		if( spawner == o || !o )
			continue;

		if( abs( spawner->m_vecOffset.x - o->m_vecOffset.x ) < 16 * pev->scale )
			return true;

		if( abs( spawner->m_vecOffset.y - o->m_vecOffset.y ) < 16 * pev->scale )
			return true;
	}

	return false;
}

#endif
