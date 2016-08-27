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


#define	HEATERPIPE_BODYHIT_VOLUME 128
#define	HEATERPIPE_WALLHIT_VOLUME 512

#define HEATERPIPE_FIRE_RATE_MIN	0.5f
#define HEATERPIPE_FIRE_RATE_MAX	2.0f

#define HEATERPIPE_FIRE_RATE_RATIO	0.75f

void FindHullIntersection(const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity);

LINK_ENTITY_TO_CLASS(weapon_heaterpipe, CHeaterPipe);


enum heaterpipe_e {
	HEATERPIPE_IDLE = 0,
	HEATERPIPE_DRAW,
	HEATERPIPE_HOLSTER,
	HEATERPIPE_ATTACK1HIT,
	HEATERPIPE_ATTACK1MISS,
	HEATERPIPE_ATTACK2MISS,
	HEATERPIPE_ATTACK2HIT,
	HEATERPIPE_ATTACK3MISS,
	HEATERPIPE_ATTACK3HIT,
	HEATERPIPE_IDLE2,
	HEATERPIPE_IDLE3,
};


void CHeaterPipe::Spawn()
{
	Precache();
	m_iId = WEAPON_HEATERPIPE;
	SET_MODEL(ENT(pev), "models/w_heaterpipe.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CHeaterPipe::Precache(void)
{
	PRECACHE_MODEL("models/v_heaterpipe.mdl");
	PRECACHE_MODEL("models/w_heaterpipe.mdl");
	PRECACHE_MODEL("models/p_heaterpipe.mdl");
	PRECACHE_SOUND("weapons/pipe_hit1.wav");
	PRECACHE_SOUND("weapons/pipe_hit2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/pipe_miss.wav");

	m_usHeaterPipe = PRECACHE_EVENT(1, "events/heaterpipe.sc");
}

int CHeaterPipe::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_HEATERPIPE;
	p->iWeight = HEATERPIPE_WEIGHT;
	return 1;
}



BOOL CHeaterPipe::Deploy()
{
	return DefaultDeploy("models/v_heaterpipe.mdl", "models/p_heaterpipe.mdl", HEATERPIPE_DRAW, "heaterpipe");
}

void CHeaterPipe::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(HEATERPIPE_HOLSTER);

	STOP_SOUND(ENT(pev), CHAN_ITEM, "player/breathe2.wav");
}

void CHeaterPipe::PrimaryAttack()
{
	if (!Swing(1))
	{
		SetThink(&CHeaterPipe::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


void CHeaterPipe::Smack()
{
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}


void CHeaterPipe::SwingAgain(void)
{
	Swing(0);
}


void CHeaterPipe::WeaponIdle(void)
{
	STOP_SOUND(ENT(pev), CHAN_ITEM, "player/breathe2.wav");
}

int CHeaterPipe::Swing(int fFirst)
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

#ifndef CLIENT_DLL
	if (tr.flFraction >= 1.0)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT(m_pPlayer->pev), &tr);
		if (tr.flFraction < 1.0)
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

	if (m_pPlayer->m_iExertLevel < PLAYER_EXERT_LEVEL_MAX)
	{
		m_pPlayer->m_iExertLevel++;
	}

	if (m_pPlayer->m_iExertLevel > PLAYER_BREATHE_LEVEL)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, PLAYER_BREATHE_SOUND, PLAYER_BREATHE_VOLUME_MAX, ATTN_NORM);
	}

	float flNextAttackTime = HEATERPIPE_FIRE_RATE_MIN;

	if (m_pPlayer->m_iExertLevel >= 4)
	{
		float flMinFireRate = HEATERPIPE_FIRE_RATE_MIN;
		float flLongestFireRate = HEATERPIPE_FIRE_RATE_MAX - flMinFireRate;
		flNextAttackTime = flNextAttackTime + (((m_pPlayer->m_iExertLevel - 4) * flLongestFireRate) / (PLAYER_EXERT_LEVEL_MAX - 4));
	}

	m_pPlayer->m_flExertUpdateStart = gpGlobals->time;
	m_pPlayer->m_flExertRate = flNextAttackTime;

#ifndef CLIENT_DLL
	// ALERT(at_console, "HeaterPipe fire rate: %f\n", flNextAttackTime);
#endif


	if (tr.flFraction >= 1.0)
	{
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usHeaterPipe,
			0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
			0.0, 0, 0.0);

		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay(flNextAttackTime);

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		}
	}
	else
	{
		switch (((m_iSwing++) % 3))
		{
		case 0:
			SendWeaponAnim(HEATERPIPE_ATTACK1HIT, 0); break;
		case 1:
			SendWeaponAnim(HEATERPIPE_ATTACK2HIT, 0); break;
		case 2:
			SendWeaponAnim(HEATERPIPE_ATTACK3HIT, 0); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage();

		if ((m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase()) || g_pGameRules->IsMultiplayer())
		{
			// first swing does full damage
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_CLUB);
		}
		else
		{
			// subsequent swings do half
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar / 2, gpGlobals->v_forward, &tr, DMG_CLUB);
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
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = HEATERPIPE_BODYHIT_VOLUME;
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
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pipe_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pipe_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * HEATERPIPE_WALLHIT_VOLUME;
#endif
		m_flNextPrimaryAttack = GetNextAttackDelay(flNextAttackTime * HEATERPIPE_FIRE_RATE_RATIO);

		SetThink(&CHeaterPipe::Smack);
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;


	}
	return fDidHit;
}



