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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


#define	CROWBAR_BODYHIT_VOLUME 128
#define	CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS(weapon_scimitar, CScimitar);



enum scimitar_e {
	SCIMITAR_IDLE = 0,
	SCIMITAR_DRAW,
	SCIMITAR_ATTACK
};


void CScimitar::Spawn()
{
	Precache();
	m_iId = WEAPON_SCIMITAR;
	SET_MODEL(ENT(pev), "models/w_runescimitar.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CScimitar::Precache(void)
{
	PRECACHE_MODEL("models/v_runescimitar.mdl");
	PRECACHE_MODEL("models/w_runescimitar.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("weapons/scimitar_hitbod.wav");
	PRECACHE_SOUND("weapons/scimitar_hitwall.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
	PRECACHE_MODEL("sprites/osrs_dmgs.spr");
	PRECACHE_MODEL("sprites/osrs_dmg0.spr");
	PRECACHE_SOUND("weapons/scimitar_kill.wav");
	UTIL_PrecacheOther("item_healthkit");

	m_usScimitar = PRECACHE_EVENT(1, "events/scimitar.sc");
}

int CScimitar::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_SCIMITAR;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}



BOOL CScimitar::Deploy()
{
	return DefaultDeploy("models/v_runescimitar.mdl", "models/p_crowbar.mdl", SCIMITAR_DRAW, "scimitar");
}

void CScimitar::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
}


void FindHullIntersection_Scimmy(const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity)
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = { mins, maxs };
	TraceResult tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc) * 2);
	UTIL_TraceLine(vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace);
	if (tmpTrace.flFraction < 1.0)
	{
		tr = tmpTrace;
		return;
	}

	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			for (k = 0; k < 2; k++)
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace);
				if (tmpTrace.flFraction < 1.0)
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if (thisDistance < distance)
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}


void CScimitar::PrimaryAttack()
{
	if (!Swing(1))
	{
		SetThink(&CScimitar::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


void CScimitar::Smack()
{
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}


void CScimitar::SwingAgain(void)
{
	Swing(0);
}


int CScimitar::Swing(int fFirst)
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
				FindHullIntersection_Scimmy(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usScimitar,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
		0.0, 0, 0.0);


	if (tr.flFraction >= 1.0)
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
			SendWeaponAnim(SCIMITAR_ATTACK);
		}
	}
	else
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		SendWeaponAnim(SCIMITAR_ATTACK);

#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage();

		int DMG = RANDOM_LONG(0, 32);

		pEntity->TraceAttack(m_pPlayer->pev, DMG*2.5, gpGlobals->v_forward, &tr, DMG_SCIMMY);

		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				
				CSprite *scimmy_damage;


				if (DMG)
					scimmy_damage = CSprite::SpriteCreate("sprites/osrs_dmgs.spr", pev->origin, TRUE);
				else
					scimmy_damage = CSprite::SpriteCreate("sprites/osrs_dmg0.spr", pev->origin, TRUE);

				if (scimmy_damage)
				{
					if (DMG != 0)
						DMG--;

					scimmy_damage->pev->frame = DMG;
					scimmy_damage->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone);
					scimmy_damage->pev->origin = (pEntity->pev->origin - gpGlobals->v_forward * 20 + gpGlobals->v_up * (pEntity->pev->size.z * 0.8));
					//scimmy_damage->pev->origin = tr2.vecEndPos + (gpGlobals->v_up * (pEntity->pev->size.z / 2));
					//scimmy_damage->pev->origin = tr.vecEndPos;
					scimmy_damage->SetScale(0.2);
					//scimmy_damage->pev->framerate = 10.0;
					//scimmy_damage->TurnOn();
					scimmy_damage->LiveForTime(1.0);
				}

				// play thwack or smack sound
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scimitar_hitbod.wav", 1, ATTN_NORM);

				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;


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

			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scimitar_hitwall.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;

		SetThink(&CScimitar::Smack);
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;


	}
	return fDidHit;
}





