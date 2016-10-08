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

#define	SPANNER_BODYHIT_VOLUME 128
#define	SPANNER_WALLHIT_VOLUME 512

void FindHullIntersection(const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity);

LINK_ENTITY_TO_CLASS(weapon_th_spanner, CSpanner);

enum spanner_e {
	SPANNER_IDLE = 0,
	SPANNER_ATTACK1,
	SPANNER_ATTACK2,
	SPANNER_USE,
	SPANNER_DRAW,
	SPANNER_HOLSTER,
};

void CSpanner::Spawn()
{
	Precache();
	m_iId = WEAPON_SPANNER;
	SET_MODEL(ENT(pev), "models/backpack.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CSpanner::Precache(void)
{
	PRECACHE_MODEL("models/v_tfc_spanner.mdl");
	PRECACHE_MODEL("models/backpack.mdl");
	PRECACHE_MODEL("models/p_spanner.mdl");
	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");

	PRECACHE_SOUND("kelly/cbar_hitkelly1.wav");
	PRECACHE_SOUND("kelly/cbar_hitkelly2.wav");
	PRECACHE_SOUND("kelly/cbar_hitkelly3.wav");

	m_usSpanner = PRECACHE_EVENT(1, "events/spanner.sc");
}

int CSpanner::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = WEAPON_SPANNER;
	p->iWeight = SPANNER_WEIGHT;
	return 1;
}



BOOL CSpanner::Deploy()
{
	return DefaultDeploy("models/v_tfc_spanner.mdl", "models/p_spanner.mdl", SPANNER_DRAW, "spanner");
}

void CSpanner::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(SPANNER_HOLSTER);
}

void CSpanner::PrimaryAttack()
{
	if (!Swing(1))
	{
		SetThink(&CSpanner::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


int CSpanner::Swing(int fFirst)
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

	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usSpanner,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
		0.0, 0, 0.0);


	if (tr.flFraction >= 1.0)
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay(0.38); // 0.5

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		}
	}
	else
	{
		switch ((m_iSwing++) % 2)
		{
		case 0:
			SendWeaponAnim(SPANNER_ATTACK1); break;
		case 1:
			SendWeaponAnim(SPANNER_ATTACK2); break;
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
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgSpanner, gpGlobals->v_forward, &tr, DMG_CLUB);
		}
		else
		{
			// subsequent swings do half
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgSpanner / 2, gpGlobals->v_forward, &tr, DMG_CLUB);
		}
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// Skeletons make different hit sounds.
				if (pEntity->Classify() == CLASS_SKELETON)
				{
					// play thwack or smack sound
					switch (RANDOM_LONG(0, 2))
					{
					case 0:
						EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "kelly/cbar_hitkelly1.wav", 1, ATTN_NORM); break;
					case 1:
						EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "kelly/cbar_hitkelly2.wav", 1, ATTN_NORM); break;
					case 2:
						EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "kelly/cbar_hitkelly3.wav", 1, ATTN_NORM); break;
					}
				}
				else
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
				}

				m_pPlayer->m_iWeaponVolume = SPANNER_BODYHIT_VOLUME;
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
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * SPANNER_WALLHIT_VOLUME;
#endif
		m_flNextPrimaryAttack = GetNextAttackDelay(0.2); // 0.25

		SetThink(&CSpanner::Smack);
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;


	}
	return fDidHit;
}



