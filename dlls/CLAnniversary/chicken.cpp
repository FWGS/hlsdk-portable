/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//=========================================================
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	CHICKEN_SHOOT				1

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs

class CChicken : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);
	int LastSpoken;
	int m_flNextShootTime;
	void Shoot(void);

	float m_flNextFlinch;

	void AlertSound(void);

	int		m_iShotgunShell;

	static const char *pAlertSounds[];

	void Killed(entvars_t *pevAttacker, int iGib);

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_chicken, CChicken);

const char *CChicken::pAlertSounds[] =
{
	"chicken/chicken_alert1.wav",
	"chicken/chicken_alert2.wav",
	"chicken/chicken_alert3.wav",
	"chicken/chicken_alert4.wav",
	"chicken/chicken_alert5.wav",
	"chicken/chicken_alert6.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CChicken::Classify(void)
{
	return	CLASS_ALIEN_PREDATOR;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CChicken::SetYawSpeed(void)
{
	pev->yaw_speed = 300;
}

int CChicken::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	// Take 30% damage from bullets
	if (bitsDamageType == DMG_BULLET)
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce(flDamage);
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CChicken::Killed(entvars_t *pevAttacker, int iGib)
{
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM);
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "chicken/killChicken.wav", 1, ATTN_NORM);
	
	CGib::SpawnRandomGibs(pev, 6, 1);
	CBaseMonster::Killed(pevAttacker, iGib);
	UTIL_Remove(this);
}

void CChicken::AlertSound(void)
{
	if (LastSpoken < gpGlobals->time)
	{
		int pitch = 95 + RANDOM_LONG(0, 9);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
		LastSpoken = gpGlobals->time + 3;
	}
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CChicken::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case CHICKEN_SHOOT:
	{
		AlertSound();
		Shoot();
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CChicken::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/chicken.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.zombieHealth;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/scock1.wav", 1.0, ATTN_NORM, 0, 100);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CChicken::Precache()
{
	PRECACHE_MODEL("models/chicken.mdl");
	PRECACHE_SOUND("weapons/sbarrel1.wav");
	PRECACHE_SOUND("weapons/scock1.wav");

	PRECACHE_SOUND("chicken/chicken_alert1.wav");
	PRECACHE_SOUND("chicken/chicken_alert2.wav");
	PRECACHE_SOUND("chicken/chicken_alert3.wav");
	PRECACHE_SOUND("chicken/chicken_alert4.wav");
	PRECACHE_SOUND("chicken/chicken_alert5.wav");
	PRECACHE_SOUND("chicken/chicken_alert6.wav");
	PRECACHE_SOUND("chicken/killChicken.wav");

	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");

	PRECACHE_SOUND_ARRAY(pAlertSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CChicken::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (m_Activity != ACT_RANGE_ATTACK1)
	{
		pev->body = 0;
	}

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_RANGE_ATTACK1))
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
		else
#endif			
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;

}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CChicken::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist > 1600)
	{
		return FALSE;
	}

	if (flDist <= 1600 && flDot >= 0.5 && gpGlobals->time >= m_flNextShootTime)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 80)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}
		return TRUE;
	}

	return FALSE;
}

void CChicken::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
	FireBullets(4, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0); // shoot +-7.5 degrees
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 0.5, ATTN_NORM, 0, 100);

	pev->effects = EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	m_flNextShootTime = gpGlobals->time + 1;
}
