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

#define	PIPE_BODYHIT_VOLUME 128
#define	PIPE_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS(weapon_pipe, CPipe);

enum pipe_e {
	PIPE_IDLE1 = 0,
	PIPE_DRAW,
	PIPE_HOLSTER,
	PIPE_ATTACK1HIT,
	PIPE_ATTACK1MISS,
	PIPE_ATTACK2MISS,
	PIPE_ATTACK2HIT,
	PIPE_ATTACK3MISS,
#ifndef CROWBAR_IDLE_ANIM
	PIPE_ATTACK3HIT
#else
	PIPE_ATTACK3HIT,
	PIPE_IDLE2,
	PIPE_IDLE3
#endif
};


void CPipe::Spawn()
{
	Precache();
	m_iId = WEAPON_PIPE;
	SET_MODEL(ENT(pev), "models/w_pipe.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CPipe::Precache(void)
{
	PRECACHE_MODEL( "models/v_pipe.mdl" );
	PRECACHE_MODEL( "models/w_pipe.mdl" );
	PRECACHE_MODEL( "models/p_pipe.mdl" );

	// PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	// PRECACHE_SOUND( "weapons/cbar_hit2.wav" );
	// PRECACHE_SOUND( "weapons/cbar_hitbod1.wav" );
	// PRECACHE_SOUND( "weapons/cbar_hitbod2.wav" );
	// PRECACHE_SOUND( "weapons/cbar_hitbod3.wav" );
	// PRECACHE_SOUND( "weapons/cbar_miss1.wav" );

	m_usPipe = PRECACHE_EVENT( 1, "events/pipe.sc" );
}

int CPipe::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_PIPE;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}

int CPipe::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CPipe::Deploy()
{
	return DefaultDeploy("models/v_pipe.mdl", "models/p_pipe.mdl", PIPE_DRAW, "pipe");
}

void CPipe::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim(PIPE_HOLSTER);
}

void CPipe::PrimaryAttack()
{
	if (!Swing(1))
	{
#ifndef CLIENT_DLL
		SetThink(&CPipe::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.5f;
#endif
	}
}

void CPipe::Smack()
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

void CPipe::SwingAgain( void )
{
	Swing( 0 );
}

int CPipe::Swing(int fFirst)
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

#ifndef CLIENT_DLL
	if (tr.flFraction >= 1.0f)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT(m_pPlayer->pev), &tr);
		if (tr.flFraction < 1.0f)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
			if (!pHit || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usPipe,
		0.0, g_vecZero, g_vecZero, 0, 0, 0,
		0.0, 0, 0.0);


	if (tr.flFraction >= 1.0f)
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay(0.5);

#ifdef CROWBAR_IDLE_ANIM
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		}
	}
	else
	{
		switch (((m_iSwing++) % 2) + 1)
		{
		case 0:
			SendWeaponAnim(PIPE_ATTACK1HIT); break;
		case 1:
			SendWeaponAnim(PIPE_ATTACK2HIT); break;
		case 2:
			SendWeaponAnim(PIPE_ATTACK3HIT); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage();

			// If building with the clientside weapon prediction system,
			// UTIL_WeaponTimeBase() is always 0 and m_flNextPrimaryAttack is >= -1.0f, thus making
			// m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() always evaluate to false.
#ifdef CLIENT_WEAPONS
			if( ( m_flNextPrimaryAttack + 1 == UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#else
			if( ( m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#endif
		{
			// first swing does full damage
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar * 2, gpGlobals->v_forward, &tr, DMG_CLUB);
		}
		else
		{
			// subsequent swings do half
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_CLUB);
		}
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// play thwack or smack sound
				switch (RANDOM_LONG(0, 2))
				{
				case 0:
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM, 0, 100); break;
				case 1:
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod2.wav", 1, ATTN_NORM, 0, 100); break;
				case 2:
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM, 0, 100); break;
				}
				m_pPlayer->m_iWeaponVolume = PIPE_BODYHIT_VOLUME;
				if (!pEntity->IsAlive())
					return TRUE;
				else
					flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer())
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 75 + RANDOM_LONG(0, 3)); // 98
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 75 + RANDOM_LONG(0, 3)); // 98
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * PIPE_WALLHIT_VOLUME;

		SetThink(&CPipe::Smack);
		pev->nextthink = UTIL_WeaponTimeBase() + 0.48f;
#endif
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5f); // 0.25f
	}
#ifdef CROWBAR_IDLE_ANIM
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
	return fDidHit;
}

#ifdef CROWBAR_IDLE_ANIM
void CPipe::WeaponIdle()
{
	if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand > 0.9f )
		{
			iAnim = PIPE_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0f / 30.0f;
		}
		else
		{
			if( flRand > 0.5f )
			{
				iAnim = PIPE_IDLE1;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0f / 30.0f;
			}
			else
			{
				iAnim = PIPE_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0f / 30.0f;
			}
		}
		SendWeaponAnim( iAnim );
	}
}
#endif
