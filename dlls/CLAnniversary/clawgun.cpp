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
#include "soundent.h"
#include "gamerules.h"

#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY 1800


class CClaw : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT BoltThink(void);
	CBeam *m_pBeam;
	bool caught;
	CBaseMonster *Victim;
	int m_iSpriteTexture;
	bool insky;

	int m_iTrail;
	int hangtimer;

public:
	static CClaw *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(clawgun_claw, CClaw);

CClaw *CClaw::BoltCreate(void)
{
	// Create a new entity with CClaw private data
	CClaw *pBolt = GetClassPtr((CClaw *)NULL);
	pBolt->pev->classname = MAKE_STRING("claw");
	pBolt->caught = false;
	pBolt->Spawn();

	return pBolt;
}

void CClaw::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/w_claw.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	SetTouch(&CClaw::BoltTouch);
	SetThink(&CClaw::BoltThink);
	hangtimer = 0;
	pev->nextthink = gpGlobals->time + 0.01;
}


void CClaw::Precache()
{
	PRECACHE_MODEL("models/w_claw.mdl");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/rope.spr");
}


int	CClaw::Classify(void)
{
	return	CLASS_NONE;
}

void CClaw::BoltThink()
{
	if (!caught)
	{
		m_pBeam = CBeam::BeamCreate("sprites/rope.spr", 15);
		if (!m_pBeam)
			return;

		entvars_t *pevOwner = NULL;
		if (pev->owner)
			pevOwner = VARS(pev->owner);

		m_pBeam->PointsInit(pev->origin, pevOwner->origin + gpGlobals->v_right * 9 + gpGlobals->v_forward * 26 + gpGlobals->v_up * 12);
		m_pBeam->SetFlags(0x20);
		m_pBeam->SetTexture(m_iSpriteTexture);
		m_pBeam->LiveForTime(0.02);
		m_pBeam->SetNoise(0);
	}
	else
	{
		m_pBeam = CBeam::BeamCreate("sprites/rope.spr", 15);
		if (!m_pBeam)
			return;

		m_pBeam->PointsInit(pev->origin, Victim->pev->origin + (gpGlobals->v_up * (Victim->pev->size.z * 0.55)));
		m_pBeam->SetFlags(0x20);
		m_pBeam->SetTexture(m_iSpriteTexture);
		m_pBeam->LiveForTime(0.02);
		m_pBeam->SetNoise(0);

		Victim->pev->gravity = 0;
		Victim->pev->movetype = MOVETYPE_FLY;
		if (insky)
		{
			Victim->pev->velocity = gpGlobals->v_up * 550 + gpGlobals->v_forward * RANDOM_LONG(-350,350) + gpGlobals->v_right * RANDOM_LONG(-350,350);
			Victim->pev->health = Victim->pev->health - 1;
		}
		else
		{
			Victim->pev->velocity = gpGlobals->v_up * 50;
			Victim->pev->health = Victim->pev->health - 0.05;
			hangtimer++;
		}
		
		
		if ((Victim->pev->health <= 0) || (Victim->IsAlive() == false) || (hangtimer > 100))
		{
			Victim->Killed(pev, 1);
			UTIL_Remove(this);
		}
	}
	pev->nextthink = gpGlobals->time + 0.01;
}

void CClaw::BoltTouch(CBaseEntity *pOther)
{
	SetTouch(NULL);

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t	*pevOwner;

		pevOwner = VARS(pev->owner);

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		if (pOther->IsPlayer())
		{
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowClient, pev->velocity.Normalize(), &tr, DMG_NEVERGIB);
			SetThink(NULL);
		}
		else
		{
			if ((pOther->IsBSPModel() == false) && (pOther->pev->health > 0))
			{
				pOther->TraceAttack(pevOwner, 2, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);

				TraceResult		tr;

				Vector vecSrc = pev->origin + gpGlobals->v_up * (pev->size.z * 0.80);
				Vector vecDir = gpGlobals->v_up;
				vecDir = vecDir.Normalize();
				UTIL_TraceLine(vecSrc, vecSrc + vecDir * 4024, ignore_monsters, ENT(pev), &tr);

				int iContents = UTIL_PointContents(tr.vecEndPos);
				if ((pOther->Classify() == CLASS_MACHINE) || (pOther->Classify() == CLASS_NONE) || pOther->pev->size.z >= 80 || 
					FClassnameIs(pOther->pev, "monster_tentacle") || FClassnameIs(pOther->pev, "monster_gargantua") || FClassnameIs(pOther->pev, "monster_ginastreamer") || FClassnameIs(pOther->pev, "monster_barnacle") ||
					FClassnameIs(pOther->pev, "monster_nihilanth") || FClassnameIs(pOther->pev, "monster_bigmomma") || FClassnameIs(pOther->pev, "monster_oetker") || FClassnameIs(pOther->pev, "monster_georgedroid"))
				{
					SetThink(NULL);
					SetTouch(NULL);
					UTIL_Remove(this);
					return;
				}

				if (iContents == CONTENTS_SKY)
				{
					insky = true;
				}		
				else
				{
					insky = false;
				}
					
				pev->origin = tr.vecEndPos - gpGlobals->v_up * 8;
				caught = true;

				CBaseMonster *pVictim = pOther->MyMonsterPointer();
				Schedule_t	*pNewSchedule;

				pNewSchedule = pVictim->GetScheduleOfType(SCHED_BARNACLE_VICTIM_GRAB);
				if (pNewSchedule)
					pVictim->ChangeSchedule(pNewSchedule);

				//pVictim->ClearSchedule();
				pVictim->ChangeSchedule(pNewSchedule);
				Victim = pVictim;

				pev->movetype = MOVETYPE_FLY;
				pev->gravity = 0;
			
			}
			else
			{
				pOther->TraceAttack(pevOwner, 20, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
				SetThink(NULL);
				SetTouch(NULL);
				UTIL_Remove(this);
			}
		}

		ApplyMultiDamage(pev, pevOwner);
		if (Victim)
			Victim->GiveRandomDrop(Victim->pev->origin, Victim->pev->angles, true);

		pev->velocity = Vector(0, 0, 0);
		// play body "thwack" sound
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM); break;
		}

		if (insky)
		{

			if (FClassnameIs(pOther->pev, "monster_scientist") || FClassnameIs(pOther->pev, "monster_cleansuit") | FClassnameIs(pOther->pev, "monster_fedorasci") || FClassnameIs(pOther->pev, "monster_camerasci") || FClassnameIs(pOther->pev, "monster_madscientist"))
			{
				switch (RANDOM_LONG(0, 3))
				{
				case 0: EMIT_SOUND_DYN(ENT(pOther->pev), CHAN_VOICE, "scientist/scream1.wav", 1, ATTN_NORM, 0, 100); break;
				case 1: EMIT_SOUND_DYN(ENT(pOther->pev), CHAN_VOICE, "scientist/scream2.wav", 1, ATTN_NORM, 0, 100); break;
				case 2: EMIT_SOUND_DYN(ENT(pOther->pev), CHAN_VOICE, "scientist/scream3.wav", 1, ATTN_NORM, 0, 100); break;
				case 3: EMIT_SOUND_DYN(ENT(pOther->pev), CHAN_VOICE, "scientist/scream4.wav", 1, ATTN_NORM, 0, 100); break;
				}
			}
		}

	}
	else
	{
		SetThink(NULL);
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/xbow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));

		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if (FClassnameIs(pOther->pev, "worldspawn"))
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin(pev, pev->origin - vecDir * 12);
			pev->angles = UTIL_VecToAngles(vecDir);
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector(0, 0, 0);
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0, 360);
			pev->nextthink = gpGlobals->time + 10.0;
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(pev->origin);
		}
	}
}
#endif

enum clawgun_e
{
	CLAWGUN_IDLE1,
	CLAWGUN_IDLE2,
	CLAWGUN_IDLE1_CLAWLESS,
	CLAWGUN_IDLE2_CLAWLESS,
	CLAWGUN_FIRE1,
	CLAWGUN_RELOAD,
	CLAWGUN_DEPLOY,
	CLAWGUN_DEPLOY_CLAWLESS,
};



LINK_ENTITY_TO_CLASS(weapon_clawgun, CClawgun);


//=========================================================
//=========================================================

void CClawgun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_clawgun"); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/w_clawgun.mdl");
	m_iId = WEAPON_CLAWGUN;

	m_iDefaultAmmo = CLAWGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CClawgun::Precache(void)
{
	PRECACHE_MODEL("models/v_clawgun.mdl");
	PRECACHE_MODEL("models/w_clawgun.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	PRECACHE_MODEL("models/grenade.mdl");	// grenade

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("weapons/gren_cock1.wav");

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");

	PRECACHE_SOUND("scientist/scream1.wav");
	PRECACHE_SOUND("scientist/scream2.wav");
	PRECACHE_SOUND("scientist/scream3.wav");
	PRECACHE_SOUND("scientist/scream4.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	PRECACHE_MODEL("sprites/rope.spr");

	m_usClawgun = PRECACHE_EVENT(1, "events/clawgun.sc");
}

int CClawgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "claw";
	p->iMaxAmmo1 = CLAWGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = NULL;
	p->iMaxClip = CLAWGUN_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_CLAWGUN;
	p->iWeight = CLAWGUN_WEIGHT;

	return 1;
}

int CClawgun::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CClawgun::Deploy()
{
	if (m_iClip)
		return DefaultDeploy("models/v_clawgun.mdl", "models/p_9mmAR.mdl", CLAWGUN_DEPLOY, "clawgun");
	return DefaultDeploy("models/v_clawgun.mdl", "models/p_9mmAR.mdl", CLAWGUN_DEPLOY_CLAWLESS, "clawgun");
}


void CClawgun::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.05;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.05;
		return;
	}

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if (RANDOM_LONG(0, 1))
		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/glauncher.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/glauncher2.wav", 1, ATTN_NORM);

	m_iClip--;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	SendWeaponAnim(CLAWGUN_FIRE1);

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);

	anglesAim.x = -anglesAim.x;
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	CClaw *pBolt = CClaw::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();
	pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
	pBolt->pev->speed = BOLT_AIR_VELOCITY;
	pBolt->pev->avelocity.z = 10;
	pBolt->pev->gravity = 0.6;
#endif

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;

}


void CClawgun::Reload(void)
{
	if (m_pPlayer->ammo_claw <= 0)
		return;

	DefaultReload(CLAWGUN_MAX_CLIP, CLAWGUN_RELOAD, 0.5);
}


void CClawgun::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	if (m_iClip)
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			iAnim = CLAWGUN_IDLE1;
			break;

		default:
		case 1:
			iAnim = CLAWGUN_IDLE2;
			break;
		}
	}
	else
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			iAnim = CLAWGUN_IDLE1_CLAWLESS;
			break;

		default:
		case 1:
			iAnim = CLAWGUN_IDLE2_CLAWLESS;
			break;
		}
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}



class CClawgunAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_claw.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_claw.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(AMMO_CLAW_GIVE, "claw", CLAWGUN_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_claw, CClawgunAmmo);


















