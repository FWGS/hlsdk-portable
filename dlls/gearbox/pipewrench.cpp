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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

void FindHullIntersection(const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity);

#define	MELEE_WALLHIT_VOLUME 512
#define MELEE_BODYHIT_VOLUME 128

#define PIPEWRENCH_ATTACK2_MIN_DAMAGE				45
#define PIPEWRENCH_ATTACK2_MAX_DAMAGE				200

LINK_ENTITY_TO_CLASS(weapon_pipewrench, CPipeWrench)

enum pwrench_e {
	PIPEWRENCH_IDLE1 = 0,
	PIPEWRENCH_IDLE2,
	PIPEWRENCH_IDLE3,
	PIPEWRENCH_DRAW,
	PIPEWRENCH_HOLSTER,
	PIPEWRENCH_ATTACK1HIT,
	PIPEWRENCH_ATTACK1MISS,
	PIPEWRENCH_ATTACK2HIT,
	PIPEWRENCH_ATTACK2MISS,
	PIPEWRENCH_ATTACK3HIT,
	PIPEWRENCH_ATTACK3MISS,
	PIPEWRENCH_ATTACKBIGWIND,
	PIPEWRENCH_ATTACKBIGHIT,
	PIPEWRENCH_ATTACKBIGMISS,
	PIPEWRENCH_ATTACKBIGLOOP,
};


void CPipeWrench::Spawn()
{
	Precache();
	m_iId = WEAPON_PIPEWRENCH;
	SET_MODEL(ENT(pev), "models/w_pipe_wrench.mdl");
	m_iSwingMode = 0;
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CPipeWrench::Precache(void)
{
	PRECACHE_MODEL("models/v_pipe_wrench.mdl");
	PRECACHE_MODEL("models/w_pipe_wrench.mdl");
	PRECACHE_MODEL("models/p_pipe_wrench.mdl");

	PRECACHE_SOUND("weapons/pwrench_hit1.wav");
	PRECACHE_SOUND("weapons/pwrench_hit2.wav");
	PRECACHE_SOUND("weapons/pwrench_hitbod1.wav");
	PRECACHE_SOUND("weapons/pwrench_hitbod2.wav");
	PRECACHE_SOUND("weapons/pwrench_hitbod3.wav");
	PRECACHE_SOUND("weapons/pwrench_miss1.wav");
	PRECACHE_SOUND("weapons/pwrench_miss2.wav");

	PRECACHE_SOUND("weapons/pwrench_big_hitbod1.wav");
	PRECACHE_SOUND("weapons/pwrench_big_hitbod2.wav");
	PRECACHE_SOUND("weapons/pwrench_big_miss.wav");

	m_usPWrench = PRECACHE_EVENT(1, "events/pipewrench.sc");
}

int CPipeWrench::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_PIPEWRENCH;
	p->iWeight = PIPEWRENCH_WEIGHT;
	return 1;
}



BOOL CPipeWrench::Deploy()
{
	m_iSwingMode = 0;
	return DefaultDeploy("models/v_pipe_wrench.mdl", "models/p_pipe_wrench.mdl", PIPEWRENCH_DRAW, "pipewrench");
}

void CPipeWrench::Holster(int skiplocal /* = 0 */)
{
	m_iSwingMode = 0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(PIPEWRENCH_HOLSTER);
}

void CPipeWrench::PrimaryAttack()
{
	if (!m_iSwingMode && !Swing(1))
	{
#if !CLIENT_DLL
		SetThink(&CPipeWrench::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
#endif
	}
}

void CPipeWrench::SecondaryAttack(void)
{
	if (m_iSwingMode != 1)
	{
		SendWeaponAnim(PIPEWRENCH_ATTACKBIGWIND);
		m_flBigSwingStart = gpGlobals->time;
	}
	m_iSwingMode = 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.3;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1;
}

void CPipeWrench::Smack()
{
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}


void CPipeWrench::SwingAgain(void)
{
	Swing(0);
}

int CPipeWrench::Swing(int fFirst)
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#if !CLIENT_DLL
	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif
	if (fFirst)
	{
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usPWrench,
			0.0, (float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 1,
			0, 0, 0 );
	}

	if ( tr.flFraction >= 1.0 )
	{
		// miss
		if ( fFirst ) {
			m_flNextPrimaryAttack = GetNextAttackDelay(0.7);
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.7;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( ((m_iSwing++) % 2) + 1 )
		{
		case 0:
			SendWeaponAnim( PIPEWRENCH_ATTACK1HIT );
			break;
		case 1:
			SendWeaponAnim( PIPEWRENCH_ATTACK2HIT );
			break;
		case 2:
			SendWeaponAnim( PIPEWRENCH_ATTACK3HIT );
			break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if !CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if( pEntity )
		{
			ClearMultiDamage();
			float flDamage;
#if CLIENT_WEAPONS
			if( ( m_flNextPrimaryAttack + 1 == UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#else
			if( ( m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#endif
			{
				// first swing does full damage
				flDamage = gSkillData.plrDmgPWrench;
			}
			else
			{
				// subsequent swings do half
				flDamage = gSkillData.plrDmgPWrench / 2;
			}
			// Send trace attack to player.
			pEntity->TraceAttack(m_pPlayer->pev, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB);

			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hitbod1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hitbod2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hitbod3.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = MELEE_BODYHIT_VOLUME;
				if ( !pEntity->IsAlive() )
				{
					m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
					return TRUE;
				}
				else
					  flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if( fHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR );

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play pipe wrench strike
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			case 1:
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = (int)( flVol * MELEE_WALLHIT_VOLUME );

		SetThink( &CPipeWrench::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;
#endif
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	return fDidHit;
}

void CPipeWrench::BigSwing(void)
{
	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#if !CLIENT_DLL
	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usPWrench,
		0.0,
		(float*)&g_vecZero,
		(float*)&g_vecZero,
		0, 0, 0, 0, 0, 0 );

	//EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pwrench_big_miss.wav", 0.8, ATTN_NORM);

	m_pPlayer->pev->punchangle.x -= 2;
	if ( tr.flFraction >= 1.0 )
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}
	else
	{
		SendWeaponAnim( PIPEWRENCH_ATTACKBIGHIT );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if !CLIENT_DLL

		// hit
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		if( pEntity )
		{
			ClearMultiDamage();
			float flDamage = (gpGlobals->time - m_flBigSwingStart) * gSkillData.plrDmgPWrench + 25.0f;
			if (flDamage > PIPEWRENCH_ATTACK2_MAX_DAMAGE) {
				flDamage = PIPEWRENCH_ATTACK2_MAX_DAMAGE;
			}
			pEntity->TraceAttack(m_pPlayer->pev, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB);

			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
		}

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,1) )
				{
				case 0:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_big_hitbod1.wav", 1, ATTN_NORM);
					break;
				case 1:
					EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_big_hitbod2.wav", 1, ATTN_NORM);
					break;
				}
				m_pPlayer->m_iWeaponVolume = MELEE_BODYHIT_VOLUME;
				if ( !pEntity->IsAlive() )
					  return;
				else
					  flVol = 0.1;

				fHitWorld = false;
			}
		}

		// play texture hit sound
		if( fHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR );

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			case 1:
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = (int)( flVol * MELEE_WALLHIT_VOLUME );
#endif
	}
}

void CPipeWrench::WeaponIdle(void)
{
	if ( m_iSwingMode == 1 )
	{
		if ( gpGlobals->time > m_flBigSwingStart + 1.0 )
		{
			m_iSwingMode = 2;
		}
	}
	else if (m_iSwingMode == 2)
	{
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.1;
		BigSwing();
		m_iSwingMode = 0;
		return;
	}
	else
	{
		m_iSwingMode = 0;
		if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
			return;
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if ( flRand <= 0.3 )
		{
			iAnim = PIPEWRENCH_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
		}
		else if ( flRand <= 0.6 )
		{
			iAnim = PIPEWRENCH_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;
		}
		else
		{
			iAnim = PIPEWRENCH_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;
		}
		SendWeaponAnim( iAnim );
	}
}
