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
#define	GRUNT_SHOOT		( 2 )

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs

class CMopedGrunt : public CBaseMonster
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
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed(entvars_t *pevAttacker, int iGib);
	int pGibName;

	float m_flNextFlinch;

	int		m_iShotgunShell;

	// No melee attacks
	BOOL CheckMeleeAttack1(float flDot, float flDist) { return FALSE; }
	BOOL CheckMeleeAttack2(float flDot, float flDist) { return FALSE; }
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_mopedgrunt, CMopedGrunt);


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMopedGrunt::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMopedGrunt::SetYawSpeed(void)
{
	pev->yaw_speed = 300;
}

int CMopedGrunt::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMopedGrunt::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case GRUNT_SHOOT:
		{
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
void CMopedGrunt::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/mopedgrunt.mdl");
	UTIL_SetSize(pev, Vector(-42, -16, 0), Vector(42, 16, 72));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.hgruntHealth * 2;
	pev->view_ofs = Vector(0,0,92);// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMopedGrunt::Precache()
{
	PRECACHE_MODEL("models/mopedgrunt.mdl");
	PRECACHE_SOUND("generic/moped.wav");
	PRECACHE_SOUND("weapons/hks1.wav");
	PRECACHE_SOUND("weapons/hks2.wav");
	PRECACHE_SOUND("weapons/hks3.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");
	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");
	pGibName = PRECACHE_MODEL("models/computergibs.mdl");
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CMopedGrunt::IgnoreConditions(void)
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
BOOL CMopedGrunt::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist >= 1200)
	{
		return FALSE;
	}
	if (flDist < 100)
	{
		Killed(pev, 1);
		UTIL_MakeVectors(pev->angles);
		CGrenade::ShootContact(pev, GetGunPosition(), gpGlobals->v_up * -1);
		return TRUE;
	}

	if (flDist >= 100 && flDist <= 1200 && flDot >= 0.2)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 80)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}
		UTIL_MakeVectors(pev->angles);
		pev->velocity = pev->velocity + gpGlobals->v_forward * 200;
		return TRUE;
	}

	return FALSE;
}

void CMopedGrunt::Killed(entvars_t *pevAttacker, int iGib)
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal1.wav", 1.0, ATTN_NORM, 0, 100);
		break;
	case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal2.wav", 1.0, ATTN_NORM, 0, 100);
		break;
	}
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_BREAKMODEL);

	// position
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);

	// size
	WRITE_COORD(pev->size.x * 2);
	WRITE_COORD(pev->size.y * 2);
	WRITE_COORD(pev->size.z * 2);

	// velocity
	WRITE_COORD(pev->velocity.x);
	WRITE_COORD(pev->velocity.y);
	WRITE_COORD(pev->velocity.z);

	// randomization
	WRITE_BYTE(25);

	// Model
	WRITE_SHORT(pGibName);	//model id#

	// # of shards
	WRITE_BYTE(0);	// let client decide

	// duration
	WRITE_BYTE(25);// 2.5 seconds

	// flags
	WRITE_BYTE(BREAK_METAL);
	MESSAGE_END();

	CGib::SpawnRandomGibs(pev, 6, 1);
	CBaseMonster::Killed(pevAttacker, iGib);
	UTIL_Remove(this);
}

void CMopedGrunt::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition() + gpGlobals->v_up * 32;
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5, 0); // shoot +-7.5 degrees
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM); break;
	case 1: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM); break;
	case 2: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks3.wav", 1, ATTN_NORM); break;
	}

	pev->effects = EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	m_flNextShootTime = gpGlobals->time + 10;
}

void CMopedGrunt::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}