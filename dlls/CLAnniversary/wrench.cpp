#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "effects.h"


#define	CROWBAR_BODYHIT_VOLUME 128
#define	CROWBAR_WALLHIT_VOLUME 512


LINK_ENTITY_TO_CLASS(weapon_wrench, CWrench);


enum crowbar_e {
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



void CWrench::Spawn()
{
	Precache();
	m_iId = WEAPON_WRENCH;
	SET_MODEL(ENT(pev), "models/w_wrench.mdl");
	m_iClip = -1;

	m_iDefaultAmmo = 100;

	FallInit();// get ready to fall down.
}


void CWrench::Precache(void)
{
	PRECACHE_MODEL("models/v_wrench.mdl");
	PRECACHE_MODEL("models/w_wrench.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("items/gunpickup3.wav");
	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
	PRECACHE_SOUND("weapons/mine_deploy.wav");
	PRECACHE_SOUND("weapons/construct.wav");
	PRECACHE_MODEL("models/sentry.mdl");

	PRECACHE_SOUND("turret/tu_fire1.wav");
	PRECACHE_SOUND("turret/tu_ping.wav");
	PRECACHE_SOUND("turret/tu_active2.wav");
	PRECACHE_SOUND("turret/tu_die.wav");
	PRECACHE_SOUND("turret/tu_die2.wav");
	PRECACHE_SOUND("turret/tu_die3.wav");
	// PRECACHE_SOUND ("turret/tu_retract.wav"); // just use deploy sound to save memory
	PRECACHE_SOUND("turret/tu_deploy.wav");
	PRECACHE_SOUND("turret/tu_spinup.wav");
	PRECACHE_SOUND("turret/tu_spindown.wav");
	PRECACHE_SOUND("turret/tu_search.wav");
	PRECACHE_SOUND("turret/tu_alert.wav");

	m_usWrench = PRECACHE_EVENT(1, "events/crowbar.sc");
}

int CWrench::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = _9MM_MAX_CARRY;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iId = WEAPON_WRENCH;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}



BOOL CWrench::Deploy()
{
	return DefaultDeploy("models/v_wrench.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar");
}

void CWrench::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(CROWBAR_HOLSTER);
#ifndef CLIENT_DLL
	if (m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NEVER);
		m_pSpot = NULL;
	}
#endif
}

void FindHullIntersection2(const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity)
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

void CWrench::PrimaryAttack()
{
	if (!Swing(1))
	{
		SetThink(&CWrench::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CWrench::SecondaryAttack()
{
	BuildSentry2();
}

void CWrench::BuildSentry2()
{
	if (!m_pSpot)
		return;
	if ((m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= 100) && (m_pPlayer->pev->waterlevel != 3))
	{
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		TraceResult tr;
		Vector trace_origin;

		// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
		// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
		trace_origin = m_pPlayer->pev->origin;
		if (m_pPlayer->pev->flags & FL_DUCKING)
		{
			trace_origin = trace_origin - (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);
		}

		// find place to toss monster
		UTIL_TraceLine(trace_origin + gpGlobals->v_forward * 40, trace_origin + gpGlobals->v_forward * 64, dont_ignore_monsters, NULL, &tr);

		int flags;
#ifdef CLIENT_WEAPONS
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif
		float distance = (m_pSpot->pev->origin - pev->origin).Length2D();
		if (distance > 100)
			return;
		if (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.25)
		{
			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
			CBaseEntity *pSentry = CBaseEntity::Create("monster_builtsentry", m_pSpot->pev->origin, Vector(0, 90, 0));
			pSentry->pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;
			pSentry->pev->velocity = gpGlobals->v_forward * 128;
			pSentry->pev->speed = 128;
#endif
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/mine_deploy.wav", 1, ATTN_NORM, 0, 105);
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/construct.wav", 1, ATTN_NORM, 0, 105);

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 100;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		}
	}
}


void CWrench::Smack()
{
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}


void CWrench::SwingAgain(void)
{
	Swing(0);
}


int CWrench::Swing(int fFirst)
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
				FindHullIntersection2(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usWrench,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
		0.0, 0, 0.0);


	if (tr.flFraction >= 1.0)
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		}
	}
	else
	{
		switch (((m_iSwing++) % 2) + 1)
		{
		case 0:
			SendWeaponAnim(CROWBAR_ATTACK1HIT); break;
		case 1:
			SendWeaponAnim(CROWBAR_ATTACK2HIT); break;
		case 2:
			SendWeaponAnim(CROWBAR_ATTACK3HIT); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage();

		if (FClassnameIs(pEntity->pev, "monster_builtsentry"))
		{
			if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= 25 && pEntity->pev->health < gSkillData.sentryHealth)
			{
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_BODY, "items/gunpickup3.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				pEntity->pev->health = gSkillData.sentryHealth;
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 25;
			}
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit2.wav", 0.7, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
			return 1;
		}

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
			if ((pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE) && (!FClassnameIs(pEntity->pev, "monster_builtsentry")))
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

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25;

		SetThink(&CWrench::Smack);
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;


	}
	return fDidHit;
}


#ifndef CLIENT_DLL

LINK_ENTITY_TO_CLASS(WrenchSpot, CLaserSpot);

void CWrench::UpdateSpot(void)
{
	if (!m_pSpot)
	{
		m_pSpot = CLaserSpot::CreateSpotTurret();
	}

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->pev->origin;
	Vector vecAiming = gpGlobals->v_forward * 100;
	vecAiming.z = 0;

	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecSrc + vecAiming, ignore_monsters, ENT(m_pPlayer->pev), &tr);

	Vector EndPos = tr.vecEndPos;
	UTIL_TraceLine(EndPos, EndPos - gpGlobals->v_up * 500, ignore_monsters, ENT(m_pSpot->pev), &tr);
	UTIL_SetOrigin(m_pSpot->pev, tr.vecEndPos);

}
#endif

void CWrench::WeaponIdle(void)
{
#ifndef CLIENT_DLL
	UpdateSpot();
#endif
}