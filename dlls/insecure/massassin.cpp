/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// hgrunt
//=========================================================

#include "extdll.h"
#include "plane.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "animation.h"
#include "squadmonster.h"
#include "weapons.h"
#include "talkmonster.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define MASSASSIN_CLIP_SIZE 36	 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!

#define SF_MALE_ASSASSIN_IS_DEAF		8

#define MASSASSIN_9MMAR (1 << 0)
#define MASSASSIN_HANDGRENADE (1 << 1)
#define MASSASSIN_GRENADELAUNCHER (1 << 2)
#define MASSASSIN_SNIPERRIFLE (1 << 3)

#define MASSASSIN_HEAD_GROUP 1
#define MASSASSIN_HEAD_WHITE 0
#define MASSASSIN_HEAD_BLACK 1
#define MASSASSIN_HEAD_GOGGLES 2

#define MASSASSIN_GUN_GROUP 2
#define MASSASSIN_GUN_MP5 0
#define MASSASSIN_GUN_SNIPER 1
#define MASSASSIN_GUN_NONE 2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define MASSASSIN_AE_RELOAD (2)
#define MASSASSIN_AE_KICK (3)
#define MASSASSIN_AE_BURST1 (4)
#define MASSASSIN_AE_BURST2 (5)
#define MASSASSIN_AE_BURST3 (6)
#define MASSASSIN_AE_GREN_TOSS (7)
#define MASSASSIN_AE_GREN_LAUNCH (8)
#define MASSASSIN_AE_GREN_DROP (9)
#define MASSASSIN_AE_CAUGHT_ENEMY (10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define MASSASSIN_AE_DROP_GUN (11)		// grunt (probably dead) is dropping his mp5.

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_MASSN_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_MASSN_ESTABLISH_LINE_OF_FIRE, // move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_MASSN_COVER_AND_RELOAD,
	SCHED_MASSN_SWEEP,
	SCHED_MASSN_FOUND_ENEMY,
	SCHED_MASSN_REPEL,
	SCHED_MASSN_REPEL_ATTACK,
	SCHED_MASSN_REPEL_LAND,
	SCHED_MASSN_WAIT_FACE_ENEMY,
	SCHED_MASSN_TAKECOVER_FAILED, // special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_MASSN_ELOF_FAIL,
	SCHED_MASSN_INVESTIGATE_SOUND,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_MASSN_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_MASSN_SPEAK_SENTENCE,
	TASK_MASSN_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_MASSN_NOFIRE (bits_COND_SPECIAL1)

class CMAssassin : public CSquadMonster
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int Classify();
	int ISoundMask();
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	BOOL FCanCheckAttacks();
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	void CheckAmmo();
	void SetActivity(Activity NewActivity);
	void StartTask(Task_t* pTask);
	void RunTask(Task_t* pTask);
	void DeathSound();
	void PainSound();
	void IdleSound();
	Vector GetGunPosition();
	void Shoot();
	void Sniper();
	void PrescheduleThink();
	void GibMonster();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	CBaseEntity* Kick();
	Schedule_t* GetSchedule();
	Schedule_t* GetScheduleOfType(int Type);
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	int IRelationship(CBaseEntity* pTarget);

	CUSTOM_SCHEDULES;

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector m_vecTossVelocity;

	bool m_fThrowGrenade;
	bool m_fStanding;
	bool m_fFirstEncounter; // only put on the handsign show in the squad's first encounter.
	int m_cClipSize;

	int m_iTargetRanderamt;

	int m_iBrassShell;
};

LINK_ENTITY_TO_CLASS(monster_male_assassin, CMAssassin);

TYPEDESCRIPTION CMAssassin::m_SaveData[] =
{
	DEFINE_FIELD(CMAssassin, m_flNextGrenadeCheck, FIELD_TIME),
	DEFINE_FIELD(CMAssassin, m_flNextPainTime, FIELD_TIME),
	//	DEFINE_FIELD( CMAssassin, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD(CMAssassin, m_vecTossVelocity, FIELD_VECTOR),
	DEFINE_FIELD(CMAssassin, m_fThrowGrenade, FIELD_BOOLEAN),
	DEFINE_FIELD(CMAssassin, m_fStanding, FIELD_BOOLEAN),
	DEFINE_FIELD(CMAssassin, m_fFirstEncounter, FIELD_BOOLEAN),
	DEFINE_FIELD(CMAssassin, m_cClipSize, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CMAssassin, CSquadMonster);

//=========================================================
// IRelationship - overridden because Alien Grunts are
// Human Grunt's nemesis.
//=========================================================
int CMAssassin::IRelationship(CBaseEntity* pTarget)
{
	if (FClassnameIs(pTarget->pev, "monster_human_grunt") || (FClassnameIs(pTarget->pev, "monster_saw_gunner")))
	{
		return R_NM;
	}

	return CSquadMonster::IRelationship(pTarget);
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CMAssassin::GibMonster()
{
	Vector vecGunPos;
	Vector vecGunAngles;

	if (GetBodygroup(MASSASSIN_GUN_GROUP) != MASSASSIN_GUN_NONE)
	{ // throw a gun if the grunt has one
		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity* pGun;
		if (FBitSet(pev->weapons, MASSASSIN_SNIPERRIFLE))
		{
			pGun = DropItem("weapon_sniperrifle", vecGunPos, vecGunAngles);
		}
		else
		{
			pGun = DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
		}
		if (pGun)
		{
			pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
			pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
		}

		if (FBitSet(pev->weapons, MASSASSIN_GRENADELAUNCHER))
		{
			pGun = DropItem("ammo_ARgrenades", vecGunPos, vecGunAngles);
			if (pGun)
			{
				pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
				pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
			}
		}
	}

	CBaseMonster::GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CMAssassin::ISoundMask()
{
	return bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_PLAYER |
		bits_SOUND_DANGER;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CMAssassin::PrescheduleThink()
{
	if (InSquad() && m_hEnemy != NULL)
	{
		if (HasConditions(bits_COND_SEE_ENEMY))
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if (gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5)
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = true;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds.
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CMAssassin::FCanCheckAttacks()
{
	if (!HasConditions(bits_COND_ENEMY_TOOFAR))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CMAssassin::CheckMeleeAttack1(float flDot, float flDist)
{
	CBaseMonster* pEnemy = nullptr;

	if (m_hEnemy != NULL)
	{
		pEnemy = m_hEnemy->MyMonsterPointer();
	}

	if (!pEnemy)
	{
		return FALSE;
	}

	if (flDist <= 64 && flDot >= 0.7 &&
		pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for MASSASSIN, cause
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the MASSASSIN can attack when the enemy is
// occluded (throw grenade over wall, etc). We must
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CMAssassin::CheckRangeAttack1(float flDot, float flDist)
{
	if (!HasConditions(bits_COND_ENEMY_OCCLUDED) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire())
	{
		TraceResult tr;

		if (!m_hEnemy->IsPlayer() && flDist <= 64)
		{
			// kick nonclients, but don't shoot at them.
			return FALSE;
		}

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine(vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if (tr.flFraction == 1.0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack.
//=========================================================
BOOL CMAssassin::CheckRangeAttack2(float flDot, float flDist)
{
	if (!FBitSet(pev->weapons, (MASSASSIN_HANDGRENADE | MASSASSIN_GRENADELAUNCHER)))
	{
		return FALSE;
	}

	// if the grunt isn't moving, it's ok to check.
	if (m_flGroundSpeed != 0)
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck)
	{
		return m_fThrowGrenade;
	}

	if (!FBitSet(m_hEnemy->pev->flags, FL_ONGROUND) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z)
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	if (FBitSet(pev->weapons, MASSASSIN_HANDGRENADE))
	{
		// find feet
		if (RANDOM_LONG(0, 1))
		{
			// magically know where they are
			vecTarget = Vector(m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z);
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget(pev->origin) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions(bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.massassinGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my squad members near the intended grenade impact area?
	if (InSquad())
	{
		if (SquadMemberInRange(vecTarget, 256))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}

	if ((vecTarget - pev->origin).Length2D() <= 256)
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}


	if (FBitSet(pev->weapons, MASSASSIN_HANDGRENADE))
	{
		Vector vecToss = VecCheckToss(pev, GetGunPosition(), vecTarget, 0.5);

		if (vecToss != g_vecZero)
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow(pev, GetGunPosition(), vecTarget, gSkillData.massassinGrenadeSpeed, 0.5);

		if (vecToss != g_vecZero)
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}



	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CMAssassin::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CMAssassin::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	Forget(bits_MEMORY_INCOVER);

	return CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMAssassin::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
	case ACT_RANGE_ATTACK2:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK1:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK2:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void CMAssassin::IdleSound()
{

}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CMAssassin::CheckAmmo()
{
	if (m_cAmmoLoaded <= 0)
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CMAssassin::Classify()
{
	return CLASS_HUMAN_ASSASSIN;
}

//=========================================================
//=========================================================
CBaseEntity* CMAssassin::Kick()
{
	TraceResult tr;

	UTIL_MakeVectors(pev->angles);
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		return pEntity;
	}

	return NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CMAssassin::GetGunPosition()
{
	if (m_fStanding)
	{
		return pev->origin + Vector(0, 0, 60);
	}
	else
	{
		return pev->origin + Vector(0, 0, 48);
	}
}

//=========================================================
// Shoot
//=========================================================
void CMAssassin::Shoot()
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_9DEGREES, 2048, BULLET_MONSTER_MP5); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--; // take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// Shoot
//=========================================================
void CMAssassin::Sniper()
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 8192, BULLET_MONSTER_762); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--; // take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMAssassin::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	Vector vecShootDir;
	Vector vecShootOrigin;

	switch (pEvent->event)
	{
	case MASSASSIN_AE_DROP_GUN:
	{
		if (GetBodygroup(MASSASSIN_GUN_GROUP) != MASSASSIN_GUN_NONE)
		{
			Vector vecGunPos;
			Vector vecGunAngles;

			GetAttachment(0, vecGunPos, vecGunAngles);

			// switch to body group with no gun.
			SetBodygroup(MASSASSIN_GUN_GROUP, MASSASSIN_GUN_NONE);

			// now spawn a gun.
			if (FBitSet(pev->weapons, MASSASSIN_SNIPERRIFLE))
			{
				DropItem("weapon_sniperrifle", vecGunPos, vecGunAngles);
			}
			else
			{
				DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
			}
			if (FBitSet(pev->weapons, MASSASSIN_GRENADELAUNCHER))
			{
				DropItem("ammo_ARgrenades", BodyTarget(pev->origin), vecGunAngles);
			}
		}
	}
	break;

	case MASSASSIN_AE_RELOAD:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM);
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case MASSASSIN_AE_GREN_TOSS:
	{
		UTIL_MakeVectors(pev->angles);
		// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
		CGrenade::ShootTimed(pev, GetGunPosition(), m_vecTossVelocity, 3.5);

		m_fThrowGrenade = false;
		m_flNextGrenadeCheck = gpGlobals->time + 6; // wait six seconds before even looking again to see if a grenade can be thrown.
		// !!!LATER - when in a group, only try to throw grenade if ordered.
	}
	break;

	case MASSASSIN_AE_GREN_LAUNCH:
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
		CGrenade::ShootContact(pev, GetGunPosition(), m_vecTossVelocity);
		m_fThrowGrenade = false;
		if (g_iSkillLevel == SKILL_HARD)
			m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT(2, 5); // wait a random amount of time before shooting again
		else
			m_flNextGrenadeCheck = gpGlobals->time + 6; // wait six seconds before even looking again to see if a grenade can be thrown.
	}
	break;

	case MASSASSIN_AE_GREN_DROP:
	{
		UTIL_MakeVectors(pev->angles);
		CGrenade::ShootTimed(pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3);
	}
	break;

	case MASSASSIN_AE_BURST1:
	{
		if (FBitSet(pev->weapons, MASSASSIN_9MMAR))
		{
			Shoot();

			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if (RANDOM_LONG(0, 1))
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM);
			}
			else
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM);
			}
		}
		else
		{
			Sniper();

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sniper_fire.wav", 1, ATTN_NORM);
		}

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	break;

	case MASSASSIN_AE_BURST2:
	case MASSASSIN_AE_BURST3:
		Shoot();
		break;

	case MASSASSIN_AE_KICK:
	{
		CBaseEntity* pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage(pev, pev, gSkillData.massassinDmgKick, DMG_CLUB);
		}
	}
	break;

	case MASSASSIN_AE_CAUGHT_ENEMY:
	{

	}
	break;

	default:
		CSquadMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CMAssassin::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/massn.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.massassinHealth;
	m_flFieldOfView = 0.2; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded = false;
	m_fFirstEncounter = true; // this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	SetBodygroup(MASSASSIN_HEAD_GROUP, RANDOM_LONG(0, 2)); // select the mask or the helmet

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = MASSASSIN_9MMAR | MASSASSIN_HANDGRENADE;
		// pev->weapons = MASSASSIN_SHOTGUN;
		// pev->weapons = MASSASSIN_9MMAR | MASSASSIN_GRENADELAUNCHER;
	}

	if (FBitSet(pev->weapons, MASSASSIN_SNIPERRIFLE))
	{
		SetBodygroup(MASSASSIN_GUN_GROUP, MASSASSIN_GUN_SNIPER);
		m_cClipSize = 1;
	}
	else
	{
		m_cClipSize = MASSASSIN_CLIP_SIZE;
	}
	m_cAmmoLoaded = m_cClipSize;

	if (RANDOM_LONG(0, 99) < 80)
		pev->skin = 0; // light skin
	else
		pev->skin = 1; // dark skin

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMAssassin::Precache()
{
	PRECACHE_MODEL("models/massn.mdl");

	PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");

	PRECACHE_SOUND("weapons/sniper_fire.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav"); // because we use the basemonster SWIPE animation event

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell
}

//=========================================================
// start task
//=========================================================
void CMAssassin::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MASSN_CHECK_FIRE:
		if (!NoFriendlyFire())
		{
			SetConditions(bits_COND_MASSN_NOFIRE);
		}
		TaskComplete();
		break;

	case TASK_MASSN_SPEAK_SENTENCE:
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget(bits_MEMORY_INCOVER);
		CSquadMonster::StartTask(pTask);
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_MASSN_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster::StartTask(pTask);
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default:
		CSquadMonster::StartTask(pTask);
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CMAssassin::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_MASSN_FACE_TOSS_DIR:
	{
		// project a point along the toss vector and turn to face that point.
		MakeIdealYaw(pev->origin + m_vecTossVelocity * 64);
		ChangeYaw(pev->yaw_speed);

		if (FacingIdeal())
		{
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		}
		break;
	}
	default:
	{
		CSquadMonster::RunTask(pTask);
		break;
	}
	}
}

//=========================================================
// PainSound
//=========================================================
void CMAssassin::PainSound()
{

}

//=========================================================
// DeathSound
//=========================================================
void CMAssassin::DeathSound()
{
	if ((pev->spawnflags & SF_MALE_ASSASSIN_IS_DEAF) == 0)
	{
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 1024, 3.0);
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t tlMassnFail[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
	{TASK_WAIT, (float)2},
	{TASK_WAIT_PVS, (float)0},
};

Schedule_t slMassnFail[] =
{
	{tlMassnFail,
		ARRAYSIZE(tlMassnFail),
		bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2 |
			bits_COND_CAN_MELEE_ATTACK1 |
			bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Fail"},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t tlMassnCombatFail[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
	{TASK_WAIT_FACE_ENEMY, (float)2},
	{TASK_WAIT_PVS, (float)0},
};

Schedule_t slMassnCombatFail[] =
{
	{tlMassnCombatFail,
		ARRAYSIZE(tlMassnCombatFail),
		bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Grunt Combat Fail"},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t tlMassnVictoryDance[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_WAIT, (float)1.5},
	{TASK_GET_PATH_TO_ENEMY_CORPSE, (float)0},
	{TASK_WALK_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
};

Schedule_t slMassnVictoryDance[] =
{
	{tlMassnVictoryDance,
		ARRAYSIZE(tlMassnVictoryDance),
		bits_COND_NEW_ENEMY |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE,
		0,
		"GruntVictoryDance"},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlMassnEstablishLineOfFire[] =
{
	{TASK_SET_FAIL_SCHEDULE, (float)SCHED_MASSN_ELOF_FAIL},
	{TASK_GET_PATH_TO_ENEMY, (float)0},
	{TASK_MASSN_SPEAK_SENTENCE, (float)0},
	{TASK_RUN_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
};

Schedule_t slMassnEstablishLineOfFire[] =
{
	{tlMassnEstablishLineOfFire,
		ARRAYSIZE(tlMassnEstablishLineOfFire),
		bits_COND_NEW_ENEMY |
			bits_COND_ENEMY_DEAD |
			bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_MELEE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2 |
			bits_COND_CAN_MELEE_ATTACK2 |
			bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire"},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t tlMassnFoundEnemy[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL1},
};

Schedule_t slMassnFoundEnemy[] =
{
	{tlMassnFoundEnemy,
		ARRAYSIZE(tlMassnFoundEnemy),
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntFoundEnemy"},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t tlMassnCombatFace1[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_WAIT, (float)1.5},
	{TASK_SET_SCHEDULE, (float)SCHED_MASSN_SWEEP},
};

Schedule_t slMassnCombatFace[] =
{
	{tlMassnCombatFace1,
		ARRAYSIZE(tlMassnCombatFace1),
		bits_COND_NEW_ENEMY |
			bits_COND_ENEMY_DEAD |
			bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t tlMassnSignalSuppress[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_FACE_IDEAL, (float)0},
	{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL2},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMassnSignalSuppress[] =
{
	{tlMassnSignalSuppress,
		ARRAYSIZE(tlMassnSignalSuppress),
		bits_COND_ENEMY_DEAD |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_HEAR_SOUND |
			bits_COND_MASSN_NOFIRE |
			bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"},
};

Task_t tlMassnSuppress[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMassnSuppress[] =
{
	{tlMassnSuppress,
		ARRAYSIZE(tlMassnSuppress),
		bits_COND_ENEMY_DEAD |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_HEAR_SOUND |
			bits_COND_MASSN_NOFIRE |
			bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t tlMassnWaitInCover[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
	{TASK_WAIT_FACE_ENEMY, (float)1},
};

Schedule_t slMassnWaitInCover[] =
{
	{tlMassnWaitInCover,
		ARRAYSIZE(tlMassnWaitInCover),
		bits_COND_NEW_ENEMY |
			bits_COND_HEAR_SOUND |
			bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2 |
			bits_COND_CAN_MELEE_ATTACK1 |
			bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"GruntWaitInCover"},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t tlMassnTakeCover1[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_SET_FAIL_SCHEDULE, (float)SCHED_MASSN_TAKECOVER_FAILED},
	{TASK_WAIT, (float)0.2},
	{TASK_FIND_COVER_FROM_ENEMY, (float)0},
	{TASK_MASSN_SPEAK_SENTENCE, (float)0},
	{TASK_RUN_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
	{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
	{TASK_SET_SCHEDULE, (float)SCHED_MASSN_WAIT_FACE_ENEMY},
};

Schedule_t slMassnTakeCover[] =
{
	{tlMassnTakeCover1,
		ARRAYSIZE(tlMassnTakeCover1),
		0,
		0,
		"TakeCover"},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlMassnGrenadeCover1[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_FIND_COVER_FROM_ENEMY, (float)99},
	{TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, (float)384},
	{TASK_PLAY_SEQUENCE, (float)ACT_SPECIAL_ATTACK1},
	{TASK_CLEAR_MOVE_WAIT, (float)0},
	{TASK_RUN_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
	{TASK_SET_SCHEDULE, (float)SCHED_MASSN_WAIT_FACE_ENEMY},
};

Schedule_t slMassnGrenadeCover[] =
{
	{tlMassnGrenadeCover1,
		ARRAYSIZE(tlMassnGrenadeCover1),
		0,
		0,
		"GrenadeCover"},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlMassnTossGrenadeCover1[] =
{
	{TASK_FACE_ENEMY, (float)0},
	{TASK_RANGE_ATTACK2, (float)0},
	{TASK_SET_SCHEDULE, (float)SCHED_TAKE_COVER_FROM_ENEMY},
};

Schedule_t slMassnTossGrenadeCover[] =
{
	{tlMassnTossGrenadeCover1,
		ARRAYSIZE(tlMassnTossGrenadeCover1),
		0,
		0,
		"TossGrenadeCover"},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t tlMassnTakeCoverFromBestSound[] =
{
	{TASK_SET_FAIL_SCHEDULE, (float)SCHED_COWER}, // duck and cover if cannot move from explosion
	{TASK_STOP_MOVING, (float)0},
	{TASK_FIND_COVER_FROM_BEST_SOUND, (float)0},
	{TASK_RUN_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
	{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
	{TASK_TURN_LEFT, (float)179},
};

Schedule_t slMassnTakeCoverFromBestSound[] =
{
	{tlMassnTakeCoverFromBestSound,
		ARRAYSIZE(tlMassnTakeCoverFromBestSound),
		0,
		0,
		"GruntTakeCoverFromBestSound"},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t tlMassnHideReload[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_SET_FAIL_SCHEDULE, (float)SCHED_RELOAD},
	{TASK_FIND_COVER_FROM_ENEMY, (float)0},
	{TASK_RUN_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
	{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_PLAY_SEQUENCE, (float)ACT_RELOAD},
};

Schedule_t slMassnHideReload[] =
{
	{tlMassnHideReload,
		ARRAYSIZE(tlMassnHideReload),
		bits_COND_HEAVY_DAMAGE |
			bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntHideReload"} };

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t tlMassnSweep[] =
{
	{TASK_TURN_LEFT, (float)179},
	{TASK_WAIT, (float)1},
	{TASK_TURN_LEFT, (float)179},
	{TASK_WAIT, (float)1},
};

Schedule_t slMassnSweep[] =
{
	{tlMassnSweep,
		ARRAYSIZE(tlMassnSweep),

		bits_COND_NEW_ENEMY |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_CAN_RANGE_ATTACK1 |
			bits_COND_CAN_RANGE_ATTACK2 |
			bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD | // sound flags
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER,

		"Grunt Sweep"},
};

//=========================================================
// primary range attack.n because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlMassnRangeAttack1A[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMassnRangeAttack1A[] =
{
	{tlMassnRangeAttack1A,
		ARRAYSIZE(tlMassnRangeAttack1A),
		bits_COND_NEW_ENEMY |
			bits_COND_ENEMY_DEAD |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_ENEMY_OCCLUDED |
			bits_COND_HEAR_SOUND |
			bits_COND_MASSN_NOFIRE |
			bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Range Attack1A"},
};


//=========================================================
// primary range attack.n because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlMassnRangeAttack1B[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_IDLE_ANGRY},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MASSN_CHECK_FIRE, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMassnRangeAttack1B[] =
{
	{tlMassnRangeAttack1B,
		ARRAYSIZE(tlMassnRangeAttack1B),
		bits_COND_NEW_ENEMY |
			bits_COND_ENEMY_DEAD |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_ENEMY_OCCLUDED |
			bits_COND_NO_AMMO_LOADED |
			bits_COND_MASSN_NOFIRE |
			bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"Range Attack1B"},
};

//=========================================================
// secondary range attack.n because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlMassnRangeAttack2[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_MASSN_FACE_TOSS_DIR, (float)0},
	{TASK_PLAY_SEQUENCE, (float)ACT_RANGE_ATTACK2},
	{TASK_SET_SCHEDULE, (float)SCHED_MASSN_WAIT_FACE_ENEMY}, // don't run immediately after throwing grenade.
};

Schedule_t slMassnRangeAttack2[] =
{
	{tlMassnRangeAttack2,
		ARRAYSIZE(tlMassnRangeAttack2),
		0,
		0,
		"RangeAttack2"},
};

Task_t tlMassnInvestigateSound[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_STORE_LASTPOSITION, (float)0},
	{TASK_GET_PATH_TO_BESTSOUND, (float)0},
	{TASK_FACE_IDEAL, (float)0},
	{TASK_RUN_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
	{TASK_PLAY_SEQUENCE, (float)ACT_IDLE},
	{TASK_WAIT, (float)5},
	{TASK_GET_PATH_TO_LASTPOSITION, (float)0},
	{TASK_WALK_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
	{TASK_CLEAR_LASTPOSITION, (float)0},
};

Schedule_t slMassnInvestigateSound[] =
{
	{tlMassnInvestigateSound,
		ARRAYSIZE(tlMassnInvestigateSound),
		bits_COND_NEW_ENEMY |
			bits_COND_SEE_FEAR |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntInvestigateSound"},
};


//=========================================================
// repel
//=========================================================
Task_t tlMassnRepel[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_FACE_IDEAL, (float)0},
	{TASK_PLAY_SEQUENCE, (float)ACT_GLIDE},
};

Schedule_t slMassnRepel[] =
{
	{tlMassnRepel,
		ARRAYSIZE(tlMassnRepel),
		bits_COND_SEE_ENEMY |
			bits_COND_NEW_ENEMY |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER |
			bits_SOUND_COMBAT |
			bits_SOUND_PLAYER,
		"Repel"},
};


//=========================================================
// repel
//=========================================================
Task_t tlMassnRepelAttack[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_PLAY_SEQUENCE, (float)ACT_FLY},
};

Schedule_t slMassnRepelAttack[] =
{
	{tlMassnRepelAttack,
		ARRAYSIZE(tlMassnRepelAttack),
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"},
};

//=========================================================
// repel land
//=========================================================
Task_t tlMassnRepelLand[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_PLAY_SEQUENCE, (float)ACT_LAND},
	{TASK_GET_PATH_TO_LASTPOSITION, (float)0},
	{TASK_RUN_PATH, (float)0},
	{TASK_WAIT_FOR_MOVEMENT, (float)0},
	{TASK_CLEAR_LASTPOSITION, (float)0},
};

Schedule_t slMassnRepelLand[] =
{
	{tlMassnRepelLand,
		ARRAYSIZE(tlMassnRepelLand),
		bits_COND_SEE_ENEMY |
			bits_COND_NEW_ENEMY |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER |
			bits_SOUND_COMBAT |
			bits_SOUND_PLAYER,
		"Repel Land"},
};


DEFINE_CUSTOM_SCHEDULES(CMAssassin) {
	slMassnFail,
		slMassnCombatFail,
		slMassnVictoryDance,
		slMassnEstablishLineOfFire,
		slMassnFoundEnemy,
		slMassnCombatFace,
		slMassnSignalSuppress,
		slMassnSuppress,
		slMassnWaitInCover,
		slMassnTakeCover,
		slMassnGrenadeCover,
		slMassnTossGrenadeCover,
		slMassnTakeCoverFromBestSound,
		slMassnHideReload,
		slMassnSweep,
		slMassnRangeAttack1A,
		slMassnRangeAttack1B,
		slMassnRangeAttack2,
		slMassnRepel,
		slMassnRepelAttack,
		slMassnRepelLand,
};

IMPLEMENT_CUSTOM_SCHEDULES(CMAssassin, CSquadMonster);

//=========================================================
// SetActivity
//=========================================================
void CMAssassin::SetActivity(Activity NewActivity)
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if (FBitSet(pev->weapons, MASSASSIN_9MMAR))
		{
			if (m_fStanding)
			{
				// get aimable sequence
				iSequence = LookupSequence("standing_mp5");
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence("crouching_mp5");
			}
		}
		else
		{
			if (m_fStanding)
			{
				// get aimable sequence
				iSequence = LookupSequence("standing_m40a1");
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence("crouching_m40a1");
			}
		}
		break;
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown
		// grenade or fired grenade, we must determine which and pick proper sequence
		if ((pev->weapons & MASSASSIN_HANDGRENADE) != 0)
		{
			// get toss anim
			iSequence = LookupSequence("throwgrenade");
		}
		else
		{
			// get launch anim
			iSequence = LookupSequence("launchgrenade");
		}
		break;
	case ACT_RUN:
		if (pev->health <= 20)
		{
			// limp!
			iSequence = LookupActivity(ACT_RUN_HURT);
		}
		else
		{
			iSequence = LookupActivity(NewActivity);
		}
		break;
	case ACT_WALK:
		if (pev->health <= 20)
		{
			// limp!
			iSequence = LookupActivity(ACT_WALK_HURT);
		}
		else
		{
			iSequence = LookupActivity(NewActivity);
		}
		break;
	case ACT_IDLE:
		if (m_MonsterState == MONSTERSTATE_COMBAT)
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity(NewActivity);
		break;
	default:
		iSequence = LookupActivity(NewActivity);
		break;
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence; // Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0; // Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t* CMAssassin::GetSchedule()
{
	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling.
	if (pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE)
	{
		if ((pev->flags & FL_ONGROUND) != 0)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType(SCHED_MASSN_REPEL_LAND);
		}
		else
		{
			// repel down a rope,
			if (m_MonsterState == MONSTERSTATE_COMBAT)
				return GetScheduleOfType(SCHED_MASSN_REPEL_ATTACK);
			else
				return GetScheduleOfType(SCHED_MASSN_REPEL);
		}
	}

	// grunts place HIGH priority on running away from danger sounds.
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound)
		{
			if ((pSound->m_iType & bits_SOUND_DANGER) != 0)
			{
				// dangerous sound nearby!

				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause
				// this may only affect a single individual in a squad.
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
	{
		// Similar to the ambush flag in the old Doom games --
		// it will only attack when he sees you.
		if (HasConditions(bits_COND_HEAR_SOUND) && (m_hEnemy == NULL)
			&& (pev->spawnflags & SF_MALE_ASSASSIN_IS_DEAF) == 0)
		{
			CSound* pSound;
			pSound = PBestSound();

			ASSERT(pSound != NULL);

			if (pSound && (pSound->m_iType & bits_SOUND_COMBAT) != 0)
			{
				return GetScheduleOfType(SCHED_MASSN_INVESTIGATE_SOUND);
			}
		}
	}
	break;
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		// new enemy
		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			if (InSquad())
			{
				MySquadLeader()->m_fEnemyEluded = false;

				if (!IsLeader())
				{
					return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
				}
				else
				{
					//!!!KELLY - the leader of a squad of grunts has just seen the player or a
					// monster and has made it the squad's enemy. You
					// can check pev->flags for FL_CLIENT to determine whether this is the player
					// or a monster. He's going to immediately start
					// firing, though. If you'd like, we can make an alternate "first sight"
					// schedule where the leader plays a handsign anim
					// that gives us enough time to hear a short sentence or spoken command
					// before he starts pluggin away.
					if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
					{
						return GetScheduleOfType(SCHED_MASSN_SUPPRESS);
					}
					else
					{
						return GetScheduleOfType(SCHED_MASSN_ESTABLISH_LINE_OF_FIRE);
					}
				}
			}
		}
		// no ammo
		else if (HasConditions(bits_COND_NO_AMMO_LOADED))
		{
			//!!!KELLY - this individual just realized he's out of bullet ammo.
			// He's going to try to find cover to run to and reload, but rarely, if
			// none is available, he'll drop and reload in the open here.
			return GetScheduleOfType(SCHED_MASSN_COVER_AND_RELOAD);
		}

		// damaged just a little
		else if (HasConditions(bits_COND_LIGHT_DAMAGE))
		{
			// if hurt:
			// 90% chance of taking cover
			// 10% chance of flinch.
			int iPercent = RANDOM_LONG(0, 99);

			if (iPercent <= 90 && m_hEnemy != NULL)
			{
				// only try to take cover if we actually have an enemy!
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
			}
			else
			{
				return GetScheduleOfType(SCHED_SMALL_FLINCH);
			}
		}
		// can kick
		else if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}
		// can grenade launch

		else if (FBitSet(pev->weapons, MASSASSIN_GRENADELAUNCHER) && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
		{
			// shoot a grenade if you can
			return GetScheduleOfType(SCHED_RANGE_ATTACK2);
		}
		// can shoot
		else if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			if (InSquad())
			{
				// if the enemy has eluded the squad and a squad member has just located the enemy
				// and the enemy does not see the squad member, issue a call to the squad to waste a
				// little time and give the player a chance to turn.
				if (MySquadLeader()->m_fEnemyEluded && !HasConditions(bits_COND_ENEMY_FACING_ME))
				{
					MySquadLeader()->m_fEnemyEluded = false;
					return GetScheduleOfType(SCHED_MASSN_FOUND_ENEMY);
				}
			}

			if (OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				// try to take an available ENGAGE slot
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			}
			else if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				// throw a grenade if can and no engage slots are available
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else
			{
				// hide!
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
			}
		}
		// can't see enemy
		else if (HasConditions(bits_COND_ENEMY_OCCLUDED))
		{
			if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else if (OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				return GetScheduleOfType(SCHED_MASSN_ESTABLISH_LINE_OF_FIRE);
			}
			else
			{
				//!!!KELLY - grunt is going to stay put for a couple seconds to see if
				// the enemy wanders back out into the open, or approaches the
				// grunt's covered position. Good place for a taunt, I guess?
				return GetScheduleOfType(SCHED_STANDOFF);
			}
		}

		if (HasConditions(bits_COND_SEE_ENEMY) && !HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MASSN_ESTABLISH_LINE_OF_FIRE);
		}
	}
	}

	// no special cases here, call the base class
	return CSquadMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CMAssassin::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		if (InSquad())
		{
			if (g_iSkillLevel == SKILL_HARD && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				return slMassnTossGrenadeCover;
			}
			else
			{
				return &slMassnTakeCover[0];
			}
		}
		else
		{
			if (RANDOM_LONG(0, 1))
			{
				return &slMassnTakeCover[0];
			}
			else
			{
				return &slMassnGrenadeCover[0];
			}
		}
	}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
	{
		return &slMassnTakeCoverFromBestSound[0];
	}
	case SCHED_MASSN_TAKECOVER_FAILED:
	{
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1) && OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_FAIL);
	}
	break;
	case SCHED_MASSN_ELOF_FAIL:
	{
		// human grunt is unable to move to a position that allows him to attack the enemy.
		return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	break;
	case SCHED_MASSN_ESTABLISH_LINE_OF_FIRE:
	{
		return &slMassnEstablishLineOfFire[0];
	}
	break;
	case SCHED_RANGE_ATTACK1:
	{
		// randomly stand or crouch
		if (RANDOM_LONG(0, 9) == 0)
			m_fStanding = RANDOM_LONG(0, 1);

		if (m_fStanding)
			return &slMassnRangeAttack1B[0];
		else
			return &slMassnRangeAttack1A[0];
	}
	case SCHED_RANGE_ATTACK2:
	{
		return &slMassnRangeAttack2[0];
	}
	case SCHED_COMBAT_FACE:
	{
		return &slMassnCombatFace[0];
	}
	case SCHED_MASSN_WAIT_FACE_ENEMY:
	{
		return &slMassnWaitInCover[0];
	}
	case SCHED_MASSN_SWEEP:
	{
		return &slMassnSweep[0];
	}
	case SCHED_MASSN_COVER_AND_RELOAD:
	{
		return &slMassnHideReload[0];
	}
	case SCHED_MASSN_FOUND_ENEMY:
	{
		return &slMassnFoundEnemy[0];
	}
	case SCHED_VICTORY_DANCE:
	{
		if (InSquad())
		{
			if (!IsLeader())
			{
				return &slMassnFail[0];
			}
		}

		return &slMassnVictoryDance[0];
	}
	case SCHED_MASSN_SUPPRESS:
	{
		if (m_hEnemy->IsPlayer() && m_fFirstEncounter)
		{
			m_fFirstEncounter = false; // after first encounter, leader won't issue handsigns anymore when he has a new enemy
			return &slMassnSignalSuppress[0];
		}
		else
		{
			return &slMassnSuppress[0];
		}
	}
	case SCHED_FAIL:
	{
		if (m_hEnemy != NULL)
		{
			// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
			return &slMassnCombatFail[0];
		}

		return &slMassnFail[0];
	}
	case SCHED_MASSN_INVESTIGATE_SOUND:
	{
		return &slMassnInvestigateSound[0];
	}
	case SCHED_MASSN_REPEL:
	{
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return &slMassnRepel[0];
	}
	case SCHED_MASSN_REPEL_ATTACK:
	{
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return &slMassnRepelAttack[0];
	}
	case SCHED_MASSN_REPEL_LAND:
	{
		return &slMassnRepelLand[0];
	}
	default:
	{
		return CSquadMonster::GetScheduleOfType(Type);
	}
	}
}


//=========================================================
// CMAssassinRepel - when triggered, spawns a monster_male_assassin
// repelling down a line.
//=========================================================

class CMAssassinRepel : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void EXPORT RepelUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	int m_iSpriteTexture; // Don't save, precache
};

LINK_ENTITY_TO_CLASS(monster_assassin_repel, CMAssassinRepel);

void CMAssassinRepel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;

	SetUse(&CMAssassinRepel::RepelUse);
}

void CMAssassinRepel::Precache()
{
	UTIL_PrecacheOther("monster_male_assassin");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/rope.spr");
}

void CMAssassinRepel::RepelUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP)
		return NULL;
	*/

	CBaseEntity* pEntity = Create("monster_male_assassin", pev->origin, pev->angles);
	CBaseMonster* pGrunt = pEntity->MyMonsterPointer();
	pGrunt->pev->movetype = MOVETYPE_FLY;
	pGrunt->pev->velocity = Vector(0, 0, RANDOM_FLOAT(-196, -128));
	pGrunt->SetActivity(ACT_GLIDE);
	// UNDONE: position?
	pGrunt->m_vecLastPosition = tr.vecEndPos;

	CBeam* pBeam = CBeam::BeamCreate("sprites/rope.spr", 10);
	pBeam->PointEntInit(pev->origin + Vector(0, 0, 112), pGrunt->entindex());
	pBeam->SetFlags(BEAM_FSOLID);
	pBeam->SetColor(255, 255, 255);
	pBeam->SetThink(&CBeam::SUB_Remove);
	pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;

	UTIL_Remove(this);
}