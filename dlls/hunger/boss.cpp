/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "apache.h"

#define SF_WAITFORTRIGGER	(0x04 | 0x40) // UNDONE: Fix!
#define SF_NOWRECKAGE		0x08

class CEinarBoss : public CApache
{
public:
	void Spawn();
	void Precache();

	BOOL FireGun();
};

LINK_ENTITY_TO_CLASS( monster_th_boss, CEinarBoss )

void CEinarBoss::Spawn()
{
	if( !pev->model )
		pev->model = MAKE_STRING( "models/boss.mdl" );

	if( !pev->health )
		pev->health = gSkillData.apacheHealth * 2.0f;

	CApache::Spawn();
}

void CEinarBoss::Precache()
{
	if( !pev->model )
		pev->model = MAKE_STRING( "models/boss.mdl" );

	CApache::Precache();
}

BOOL CEinarBoss::FireGun()
{
	UTIL_MakeAimVectors( pev->angles );

	Vector posGun, angGun;
	GetAttachment( 1, posGun, angGun );

	Vector vecTarget = ( m_posTarget - posGun ).Normalize();

	Vector vecOut;

	vecOut.x = DotProduct( -gpGlobals->v_right, vecTarget );
	vecOut.y = -DotProduct( gpGlobals->v_forward, vecTarget );
	vecOut.z = DotProduct( gpGlobals->v_up, vecTarget );

	Vector angles = UTIL_VecToAngles( vecOut );

	angles.x = -angles.x;
	if( angles.y > 180 )
		angles.y = angles.y - 360;
	if( angles.y < -180 )
		angles.y = angles.y + 360;
	if( angles.x > 180 )
		angles.x = angles.x - 360;
	if( angles.x < -180 )
		angles.x = angles.x + 360;

	if( angles.x > m_angGun.x )
		m_angGun.x = Q_min( angles.x, m_angGun.x + 12 );
	if( angles.x < m_angGun.x )
		m_angGun.x = Q_max( angles.x, m_angGun.x - 12 );
	if( angles.y > m_angGun.y )
		m_angGun.y = Q_min( angles.y, m_angGun.y + 12 );
	if( angles.y < m_angGun.y )
		m_angGun.y = Q_max( angles.y, m_angGun.y - 12 );

	m_angGun.y = SetBoneController( 0, m_angGun.y );
	m_angGun.x = SetBoneController( 1, m_angGun.x );

	Vector posBarrel, angBarrel;
	GetAttachment( 0, posBarrel, angBarrel );
	Vector vecGun = (posBarrel - posGun).Normalize();

	if( DotProduct( vecGun, vecTarget ) > 0.98f && !( ( ++m_iShots ) % 3 ) )
	{
		FireBullets( 1, posGun, vecGun, VECTOR_CONE_4DEGREES, 8192, BULLET_MONSTER_9MM, 1 );
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.3 );
		return TRUE;
	}

	if( m_iShots % 3 )
		++m_iShots;
	else
		pev->sequence = 0;

	return FALSE;
}
