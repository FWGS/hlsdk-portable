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

#define	PIPEWRENCH_BODYHIT_VOLUME 128
#define	PIPEWRENCH_WALLHIT_VOLUME 512

#define PIPEWRENCH_ATTACK2_MIN_DAMAGE				45
#define PIPEWRENCH_ATTACK2_MAX_DAMAGE				200
#define PIPEWRENCH_ATTACK2_MAX_DAMAGE_HOLD_TIME		4

LINK_ENTITY_TO_CLASS(weapon_pipewrench, CPipeWrench);

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
	m_iClip = -1;

	m_iFirestate = FIRESTATE_NONE;

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
	m_iFirestate = FIRESTATE_NONE;

	return DefaultDeploy("models/v_pipe_wrench.mdl", "models/p_pipe_wrench.mdl", PIPEWRENCH_DRAW, "pipewrench");
}

void CPipeWrench::Holster(int skiplocal /* = 0 */)
{
	m_iFirestate = FIRESTATE_NONE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(PIPEWRENCH_HOLSTER);
}

void CPipeWrench::PrimaryAttack()
{
	if (!Swing(1, TRUE))
	{
		SetThink(&CPipeWrench::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CPipeWrench::SecondaryAttack(void)
{
	if (m_iFirestate != FIRESTATE_NONE)
		return;

	m_iFirestate			= FIRESTATE_WINDUP;

	SendWeaponAnim(PIPEWRENCH_ATTACKBIGWIND);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(1.0f);
	pev->nextthink = UTIL_WeaponTimeBase() + 1.0f;

	ALERT( at_console, "CPipeWrench::SecondaryAttack\n" );
}


void CPipeWrench::ItemPostFrame(void)
{
	
	if (!(m_pPlayer->pev->button & IN_ATTACK))
	{
		m_flLastFireTime = 0.0f;
	}

	if (m_iFirestate != FIRESTATE_NONE)
	{
		// Set primary attack flag off, in case the player is thinking of doing a primary
		// attack while the secondary attack sequence is incomplete.
		m_pPlayer->pev->button &= ~IN_ATTACK;

		if (CanAttack(m_flNextSecondaryAttack, gpGlobals->time, UseDecrement()))
		{
			switch (m_iFirestate)
			{
			case FIRESTATE_WINDUP:
			{
				ALERT(at_console, "CPipeWrench::FIRESTATE_WINDUP\n");

				m_iFirestate = FIRESTATE_WINDLOOP;

				SendWeaponAnim(PIPEWRENCH_ATTACKBIGLOOP);

				m_flHoldStartTime = gpGlobals->time;

				m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.01f);
				pev->nextthink = UTIL_WeaponTimeBase() + 0.01f;
			}
			break;


			case FIRESTATE_WINDLOOP:
			{
				ALERT(at_console, "CPipeWrench::FIRESTATE_WINDLOOP\n");
				if (!(m_pPlayer->pev->button & IN_ATTACK2))
				{
					ALERT(at_console, "Releasing CPipeWrench\n");
					m_iFirestate = FIRESTATE_BIGHIT;
				}
				else
				{
					ALERT(at_console, "Holding CPipeWrench\n");
					SendWeaponAnim(PIPEWRENCH_ATTACKBIGLOOP);
				}
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.01f);
			}
			break;

			case FIRESTATE_BIGHIT:
			{
				ALERT(at_console, "CPipeWrench::FIRESTATE_BIGHIT\n");

				Swing(1, FALSE);

				m_iFirestate = FIRESTATE_NONE;

				m_flHoldStartTime = 0.0f;

				m_pPlayer->pev->button &= ~IN_ATTACK2;
			}
			break;
			}
		}
		return;
	}

	if ((m_pPlayer->pev->button & IN_ATTACK2) && CanAttack(m_flNextSecondaryAttack, gpGlobals->time, UseDecrement()))
	{
#ifndef CLIENT_DLL
		m_pPlayer->TabulateAmmo();
#endif
		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) && CanAttack(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()))
	{

#ifndef CLIENT_DLL
		m_pPlayer->TabulateAmmo();
#endif
		PrimaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK;
	}
	else if (!(m_pPlayer->pev->button & (IN_ATTACK | IN_ATTACK2)))
	{
		// no fire buttons down
		WeaponIdle();
		return;
	}

	// catch all
	if (ShouldWeaponIdle())
	{
		WeaponIdle();
	}
}

void CPipeWrench::Smack()
{
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}


void CPipeWrench::SwingAgain(void)
{
	Swing(0, TRUE);
}

#define clamp( val, min, max ) ( ((val) > (max)) ? (max) : ( ((val) < (min)) ? (min) : (val) ) )

int CPipeWrench::Swing(int fFirst, BOOL fIsPrimary)
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

	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usPWrench,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, fIsPrimary,
		0.0, 0, 0.0);


	if (tr.flFraction >= 1.0)
	{
		// miss
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = (fIsPrimary)
			? GetNextAttackDelay(0.35f)
			: GetNextAttackDelay(1.0f);

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	}
	else
	{
		if (fIsPrimary)
		{
			switch (((m_iSwing++) % 2) + 1)
			{
			case 0:
				SendWeaponAnim(PIPEWRENCH_ATTACK1HIT); break;
			case 1:
				SendWeaponAnim(PIPEWRENCH_ATTACK2HIT); break;
			case 2:
				SendWeaponAnim(PIPEWRENCH_ATTACK3HIT); break;
			}
		}
		else
		{
			SendWeaponAnim(PIPEWRENCH_ATTACKBIGHIT);

			m_pPlayer->pev->punchangle.x = -5;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage();

		float flDamage;

		if (!fIsPrimary)
		{
			float flRealDamage, flTotalDamageSpan;
			float flRealHoldTimeDelta, flNormHoldTimeDelta;
			float flNormHoldTimeProp;

			// Get the total damge to be dealt with, excluding the starting minimum.
			flTotalDamageSpan = PIPEWRENCH_ATTACK2_MAX_DAMAGE - PIPEWRENCH_ATTACK2_MIN_DAMAGE;

			// Get the time delta since we hold the secondary attack button.
			flRealHoldTimeDelta = clamp(gpGlobals->time - m_flHoldStartTime, 0, PIPEWRENCH_ATTACK2_MAX_DAMAGE_HOLD_TIME);

			// Normalize the value, between 0. and 1.0
			flNormHoldTimeProp = clamp(flRealHoldTimeDelta / PIPEWRENCH_ATTACK2_MAX_DAMAGE_HOLD_TIME, 0.0f, 1.0f);

			// Establish a proportion between normalized value and total hold time
			// for maximum damage.
			flNormHoldTimeDelta = flNormHoldTimeProp * PIPEWRENCH_ATTACK2_MAX_DAMAGE_HOLD_TIME;

			// Convert the computed proportion relative to maximum hold time, to damage.
			flRealDamage = (flTotalDamageSpan * flNormHoldTimeDelta) / PIPEWRENCH_ATTACK2_MAX_DAMAGE_HOLD_TIME;

			// Add the desired extra damage to the minimum.
			flDamage = PIPEWRENCH_ATTACK2_MIN_DAMAGE + flRealDamage;

			// Clamp the new desired damage value between min and max.
			flDamage = clamp(flDamage, PIPEWRENCH_ATTACK2_MIN_DAMAGE, PIPEWRENCH_ATTACK2_MAX_DAMAGE);
		}
		else
		{
			if ((m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase()) || g_pGameRules->IsMultiplayer())
			{
				// first swing does full damage
				flDamage = gSkillData.plrDmgPWrench;
			}
			else
			{
				// subsequent swings do half
				flDamage = gSkillData.plrDmgPWrench / 2;
			}
		}

		ALERT(at_console, "PipeWrench damage: %f\n", flDamage);

		// Send trace attack to player.
		pEntity->TraceAttack(m_pPlayer->pev, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB);

		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// play thwack or smack sound
				if (fIsPrimary)
				{
					// Primary attack body hit sound.
					switch (RANDOM_LONG(0, 2))
					{
					case 0: EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hitbod1.wav", 1, ATTN_NORM); break;
					case 1: EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hitbod2.wav", 1, ATTN_NORM); break;
					case 2: EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hitbod3.wav", 1, ATTN_NORM); break;
					}
				}
				else
				{
					// Secondary attack body hit sound.
					switch (RANDOM_LONG(0, 1))
					{
					case 0: EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_big_hitbod1.wav", 1, ATTN_NORM); break;
					case 1: EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_big_hitbod2.wav", 1, ATTN_NORM); break;
					}
				}


				m_pPlayer->m_iWeaponVolume = PIPEWRENCH_BODYHIT_VOLUME;
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
			//
			// Shared between both primary and secondary attack.
			//
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * PIPEWRENCH_WALLHIT_VOLUME;
#endif
	
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = (fIsPrimary) 
			? GetNextAttackDelay(0.5f) 
			: GetNextAttackDelay(1.0f);
	
		SetThink(&CPipeWrench::Smack);

		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;


	}
	return fDidHit;
}

void CPipeWrench::WindUp(void)
{
	SendWeaponAnim(PIPEWRENCH_ATTACKBIGWIND);

	SetThink(&CPipeWrench::WindLoop);

	m_flNextSecondaryAttack = GetNextAttackDelay(1.0f);
	pev->nextthink = UTIL_WeaponTimeBase() + 1.0f;
}

void CPipeWrench::WindLoop(void)
{
	if (m_flNextSecondaryAttack < UTIL_WeaponTimeBase() && !(m_pPlayer->pev->button & IN_ATTACK2))
	{
		if (!Swing(1, FALSE))
		{
			SetThink(&CPipeWrench::SwingAgain2);
			pev->nextthink = UTIL_WeaponTimeBase() + 0.1f;
		}
		return;
	}

	SendWeaponAnim( PIPEWRENCH_ATTACKBIGLOOP );
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1f;
}

void CPipeWrench::SwingAgain2(void)
{
	Swing(0, FALSE);
}


//=========================================================
// Purpose:
//=========================================================
BOOL CPipeWrench::CanAttack(float attack_time, float curtime, BOOL isPredicted)
{
#if defined( CLIENT_WEAPONS )
	if (!isPredicted)
#else
	if (1)
#endif
	{
		return (attack_time <= curtime) ? TRUE : FALSE;
	}
	else
	{
		return (attack_time <= 0.0) ? TRUE : FALSE;
	}
}