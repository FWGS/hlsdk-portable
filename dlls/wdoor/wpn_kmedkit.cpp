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
#include "hornet.h"
#include "gamerules.h"
#include "shake.h"

enum kmedkit_e {
	KM_IDLE1 = 0,
	KM_IDLE2,
	KM_IDLE3,
	KM_USE1,
	KM_USE2,
	KM_DOWN,
	KM_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_medkit, Ckmedkit );
LINK_ENTITY_TO_CLASS( weapon_medkit_full, Ckmedkit );

BOOL Ckmedkit::IsUseable( void )
{
	return TRUE;
}

void Ckmedkit::Spawn( )
{
	Precache( );
	m_iId = WEAPON_KMEDKIT;
	SET_MODEL(ENT(pev), "models/w_all_items1.mdl");
	pev->body = 0;

	m_iDefaultAmmo = 1;

	#ifndef CLIENT_DLL
	if ( FClassnameIs( pev, "weapon_medkit_full" ) ){
	m_iDefaultAmmo = 5;
	}
	#endif

	pev->classname = MAKE_STRING("weapon_medkit"); // hack to allow for old names

	FallInit();// get ready to fall down.
}


void Ckmedkit::Precache( void )
{
	PRECACHE_MODEL("models/v_kmedkit.mdl");
	PRECACHE_SOUND("rmxp/105-Heal01.wav");
}

int Ckmedkit::AddToPlayer( CBasePlayer *pPlayer )
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

int Ckmedkit::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "kmedkit";
	p->iMaxAmmo1 = MEDKIT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_KMEDKIT;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = HORNETGUN_WEIGHT;

	return 1;
}


BOOL Ckmedkit::Deploy( )
{
	m_pPlayer->m_newcross_active = 0;

	return DefaultDeploy( "models/v_kmedkit.mdl", 0, KM_DRAW, "trip" );
}

void Ckmedkit::Holster( int skiplocal /* = 0 */ )
{
	if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_KMEDKIT);
		SetThink( &Ckmedkit::DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( KM_DOWN );
}


void Ckmedkit::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		return;
	}
	if (m_pPlayer->pev->health >= m_pPlayer->pev->max_health)
	{
		return;
	}

	#ifndef CLIENT_DLL
	FX_Explosion( m_pPlayer->Center(), EXPLOSION_MEDKIT);
	if (g_iSkillLevel == SKILL_EASY){
	m_pPlayer->TakeHealth( m_pPlayer->pev->max_health * 0.25, DMG_GENERIC );
	}
	else{
	m_pPlayer->TakeHealth( m_pPlayer->pev->max_health * 0.2, DMG_GENERIC );
	}
//	if(m_pPlayer->m_darkposion > 0){
//	m_pPlayer->m_darkposion = 0;//����ȡ��
//	}
//	m_pPlayer->m_needleheal2 += (int)m_pPlayer->pev->max_health * 0.2;
//	m_pPlayer->m_flVelocityModifier = 0;
	#endif
	
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	SendWeaponAnim( 7,623 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.0;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
}


void FindHullIntersection5( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace );
	if ( tmpTrace.flFraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if ( tmpTrace.flFraction < 1.0 )
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

void Ckmedkit::SecondaryAttack( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		return;
	}
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 64;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );
	
#ifndef CLIENT_DLL
	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection5( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif


	if ( tr.flFraction >= 1.0 )
	{
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}
	else
	{

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
#ifndef CLIENT_DLL
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if ( pEntity->pev->flags & (FL_MONSTER) ){
			if(pEntity->pev->health < pEntity->pev->max_health && 
			(pEntity->Classify() == CLASS_PLAYER_ALLY || pEntity->Classify() == CLASS_HUMAN_PASSIVE) ){
				CBaseMonster *pEnemyMonster;
				pEnemyMonster = pEntity->MyMonsterPointer();
				if(pEnemyMonster){
					if(pEnemyMonster->m_lovehate == 810 || pEnemyMonster->m_lovehate <= 0
					|| pEnemyMonster->m_rpgms_inteam == 0 || pEntity->pev->deadflag != DEAD_NO){
					return;
					}
					//if(pEntity->pev->deadflag != DEAD_NO && pEntity->pev->health >= -pEntity->pev->max_health * 0.2){
					//����������ҽ��
					//FX_Explosion( pEntity->pev->origin, EXPLOSION_MEDKIT);
					//pEnemyMonster->Spawn();
					//pEnemyMonster->pev->health = 0;
					//}
					pEnemyMonster->m_lovehate += 5;//ˢ�øж�
					FX_Explosion( pEntity->Center(), EXPLOSION_MEDKIT);
					m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
					m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.5;
					m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
					SendWeaponAnim( 4 );
					if (g_iSkillLevel == SKILL_EASY){
					pEntity->TakeHealth( pEntity->pev->max_health * 0.25, DMG_GENERIC );
					}
					else{
					pEntity->TakeHealth( pEntity->pev->max_health * 0.2, DMG_GENERIC );
					}
				}
			}
		}
#endif

		
	}
	return;
}

void Ckmedkit::WeaponIdle( void )
{

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	if (flRand <= 0.75)
	{
		iAnim = KM_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.875)
	{
		iAnim = KM_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
	}
	else
	{
		iAnim = KM_IDLE3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 35.0 / 16.0;
	}

	if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] ){
		RetireWeapon();
		return;
	}

	SendWeaponAnim( iAnim );
}

#endif