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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"


// set the weapon sounds
// set the weapon slot
// set the weapon crosshairs

#define CHARM_PRIMARY_FIRE_VOLUME	450// how loud charm is when cast

class CCharmedMonster : public CBaseEntity
{
public:
	void Spawn( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void	Initialise(CBaseMonster* pCharmedMonster);
	void	EXPORT CharmThink(void);

	int				m_iOriginalClass;
	CBaseMonster*	m_pCharmedMonster;	// this is the thing being charmed
};

TYPEDESCRIPTION CCharmedMonster::m_SaveData[] =
{
	DEFINE_FIELD( CCharmedMonster, m_iOriginalClass, FIELD_INTEGER),
	DEFINE_FIELD( CCharmedMonster, m_pCharmedMonster, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE( CCharmedMonster, CBaseEntity );

LINK_ENTITY_TO_CLASS(charmedmonster, CCharmedMonster);

void CCharmedMonster::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->classname = MAKE_STRING( "charmedmonster" );
	
	pev->solid = SOLID_NOT;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	DontThink();
}

void CCharmedMonster::Initialise(CBaseMonster* pCharmedMonster)
{
	m_pCharmedMonster = pCharmedMonster;

	m_iOriginalClass = m_pCharmedMonster->Classify();
	m_pCharmedMonster->m_iClass = CLASS_PLAYER_ALLY;

	m_pCharmedMonster->m_hEnemy = NULL;
	
	SetThink(CharmThink);

	SetNextThink(3000.0 / (float)m_pCharmedMonster->pev->max_health);
}

void CCharmedMonster::CharmThink()
{
	m_pCharmedMonster->m_iClass = m_iOriginalClass;

	UTIL_Remove(this);
}


/////////////////////////////////////////////////////////////////////////////////////

enum charm_e {
	CHARM_OPEN = 0,
	CHARM_IDLE1,
	CHARM_IDLE2,
	CHARM_IDLE3,
	CHARM_CAST,
	CHARM_CLOSE
};

class CCharm : public CBasePlayerWeapon
{
public:

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0  );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );
	void ZapPowerUp( void );
	void ZapShoot( void );
	void ZapDone( void );
	
	int m_fInAttack;

	void ClearBeam( void );
	void ZapBeam();

	BOOL IsMonsterCharmed (CBaseEntity* pEntity);

	virtual BOOL UseDecrement( void )
	{ 
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	
	// the beam effect
	CBeam* m_pBeam;
};

LINK_ENTITY_TO_CLASS( weapon_charm, CCharm );

TYPEDESCRIPTION	CCharm::m_SaveData[] = 
{
	DEFINE_FIELD( CCharm, m_fInAttack, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CCharm, CBasePlayerWeapon );

void CCharm::Spawn( )
{
	Precache( );
	m_iId = WEAPON_CHARM;
	SET_MODEL(ENT(pev), "models/w_charm.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CCharm::Precache( void )
{
	PRECACHE_MODEL("models/w_charm.mdl");
	PRECACHE_MODEL("models/v_charm.mdl");
	//PRECACHE_MODEL("models/p_charm.mdl");

	PRECACHE_MODEL( "sprites/xbeam3.spr" ); 

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/gauss2.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("weapons/electro5.wav");
	PRECACHE_SOUND("weapons/electro6.wav");
	PRECACHE_SOUND("ambience/pulsemachine.wav");
}

int CCharm::AddToPlayer( CBasePlayer *pPlayer )
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

int CCharm::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 5;
	p->iId = m_iId = WEAPON_CHARM;
	p->iFlags = 0;
	p->iWeight = CHARM_WEIGHT;

	return 1;
}

BOOL CCharm::Deploy( )
{
	return DefaultDeploy( "models/v_charm.mdl", "", CHARM_OPEN, "charm" );
}

void CCharm::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	
	SendWeaponAnim( CHARM_CLOSE );
	m_fInAttack = 0;
}


void CCharm::PrimaryAttack()
{
	if (m_fInAttack != 0)
	{
		if ( m_fInAttack == 1 )
		{
			ZapShoot();
		}
		else // == 2
		{
			ZapDone();
		}
		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	/*
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 1)
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}
	*/

	ZapPowerUp();
}

void CCharm::SecondaryAttack()
{
}

void CCharm::ZapPowerUp()
{
	m_fInAttack = 1;

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 150 );

	m_flTimeWeaponIdle = gpGlobals->time + 0.2;
}


void CCharm::ZapShoot()
{
	ClearBeam( );

	UTIL_MakeAimVectors( pev->angles );

	ZapBeam();

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );

	m_flTimeWeaponIdle = gpGlobals->time + 0.2;
	m_flNextPrimaryAttack = gpGlobals->time + 0.2;

	m_fInAttack = 2;
}

void CCharm::ZapDone()
{
	ClearBeam( );

	m_fInAttack = 0;
	m_flTimeWeaponIdle = gpGlobals->time + 0.4;
	m_flNextPrimaryAttack = gpGlobals->time + 0.4;
}

void CCharm::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	if (m_fInAttack != 0)
	{
		if ( m_fInAttack == 1 )
		{
			ZapShoot();
		}
		else // == 2
		{
			ZapDone();
		}
	}
	else
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.4)
		{
			iAnim = CHARM_IDLE1;
			m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT(2,3);
		}
		else if (flRand <= 0.75)
		{
			iAnim = CHARM_IDLE2;
			m_flTimeWeaponIdle = gpGlobals->time + 3;
		}
		else
		{
			iAnim = CHARM_IDLE3;
			m_flTimeWeaponIdle = gpGlobals->time + 3;
		}

		return;
		SendWeaponAnim( iAnim );
	}
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CCharm :: ZapBeam()
{
	TraceResult tr;
	CBaseEntity *pEntity;

	// this should be attachment 0 (???)
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecDest = vecSrc + vecAiming * 2048;
	UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, m_pPlayer->edict(), &tr );

	m_pBeam = CBeam::BeamCreate( "sprites/xbeam3.spr", 20 );
	if (!m_pBeam)
		return;

	m_pBeam->PointEntInit( tr.vecEndPos, m_pPlayer->entindex( ) );
	m_pBeam->SetEndAttachment( 1 );
	m_pBeam->SetColor( 255, 180, 96 );
	m_pBeam->SetBrightness( 255 );
	m_pBeam->SetNoise( 0 );
	m_pBeam->SetScrollRate( 20 );
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;	// Flag these to be destroyed on save/restore or level transition

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL)
	{
		// is it a monster
		if (pEntity->pev->flags & FL_MONSTER)
		{
			// we cannot charm a monster who is already charmed
			if (!IsMonsterCharmed(pEntity))
			{
				// create a charmed monster entity
				CCharmedMonster* pCharm = (CCharmedMonster*)CBaseEntity::Create( "charmedmonster", pEntity->pev->origin, pEntity->pev->angles, pEntity->edict() );

				// set the monster
				pCharm->Initialise((CBaseMonster*)pEntity);
			}
		}
	}
	UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}

BOOL CCharm :: IsMonsterCharmed(CBaseEntity* pEntity)
{
	CCharmedMonster* pCharm = NULL;
	pCharm = (CCharmedMonster*)UTIL_FindEntityByClassname(pCharm, "charmedmonster");
	while (pCharm != NULL)
	{
		if (pCharm->m_pCharmedMonster == pEntity) return TRUE;

		pCharm = (CCharmedMonster*)UTIL_FindEntityByClassname(pCharm, "charmedmonster");
	}

	return FALSE;
}

void CCharm :: ClearBeam( )
{
	if (m_pBeam)
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}

	STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
}


#endif
