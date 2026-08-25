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
#include	"soundent.h"

extern DLL_GLOBAL int  g_iSkillLevel;

enum
{
	SCHED_COWBOY_COVER_AND_RELOAD,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_AE_ATTACK_BOTH		0x03
#define	ZOMBIE_AE_SHOOT				4
#define	HGRUNT_AE_RELOAD			2
#define	EUGENE_STRAFE_LEFT			8
#define	EUGENE_STRAFE_RIGHT			9
#define	EUGENE_EQUIP				10

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs

#define HEAD_GROUP					1
#define HEAD_GRUNT					0
#define HEAD_COMMANDER				1
#define HEAD_SHOTGUN				2
#define HEAD_M203					3
#define GUN_GROUP					2
#define GUN_357						0
#define GUN_NONE					1

class CEugene : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);
	int LastSpoken;
	int DodgeTimer;
	int DodgePoints = 15;
	int m_flNextShootTime;
	void Shoot(void);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void EXPORT Dodged(void);
	void SetSkin(void);
	int IRelationship(CBaseEntity *pTarget);

	//For strafing
	bool justfired;
	float RollCooldown;

	void StartTask(Task_t *pTask);
	Schedule_t	*GetSchedule(void);
	Schedule_t  *GetScheduleOfType(int Type);
	void CheckAmmo(void);
	CUSTOM_SCHEDULES;

	float m_flNextFlinch;

	int		m_iShotgunShell;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_eugene, CEugene);

const char *CEugene::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CEugene::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CEugene::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CEugene::SetYawSpeed(void)
{
	pev->yaw_speed = 300;
}

int CEugene::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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
// Grunt reload schedule
//=========================================================
Task_t	tlEugeneHideReload[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RELOAD },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_RELOAD },
};

Schedule_t slEugeneHideReload[] =
{
	{
		tlEugeneHideReload,
		ARRAYSIZE(tlEugeneHideReload),
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"EugeneHideReload"
	}
};

DEFINE_CUSTOM_SCHEDULES(CEugene)
{
	slEugeneHideReload,
};

IMPLEMENT_CUSTOM_SCHEDULES(CEugene, CBaseMonster);

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CEugene::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
								// dead enemy
								if (HasConditions(bits_COND_ENEMY_DEAD))
								{
									// call base class, all code to handle dead enemies is centralized there.
									return CBaseMonster::GetSchedule();
								}


								// no ammo
								else if (HasConditions(bits_COND_NO_AMMO_LOADED))
								{
									//!!!KELLY - this individual just realized he's out of bullet ammo. 
									// He's going to try to find cover to run to and reload, but rarely, if 
									// none is available, he'll drop and reload in the open here. 
									return GetScheduleOfType(SCHED_COWBOY_COVER_AND_RELOAD);
								}
	}
	}
	// no special cases here, call the base class
	return CBaseMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CEugene::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_COWBOY_COVER_AND_RELOAD:
	{
										  return &slEugeneHideReload[0];
	}
	default:
	{
			   return CBaseMonster::GetScheduleOfType(Type);
	}
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CEugene::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case ZOMBIE_AE_ATTACK_BOTH:
	{
								  // do stuff for this event.
								  CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgBothSlash / 3, DMG_SLASH);
								  if (pHurt)
								  {
									  if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
									  {
										  pHurt->pev->punchangle.x = 5;
										  pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 150 + gpGlobals->v_up * 50;
									  }
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
								  }
								  else
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

	}
		break;
	case HGRUNT_AE_RELOAD:
	{
							 Vector vecShootOrigin = pev->origin + gpGlobals->v_up * 42 + gpGlobals->v_right * 8 + gpGlobals->v_forward * 12;
							 Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
							 UTIL_MakeVectors(pev->angles);
							 Vector	vecShellVelocity;
							 EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM);
							 m_cAmmoLoaded = 17;
							 ClearConditions(bits_COND_NO_AMMO_LOADED);
	}
		break;
	case ZOMBIE_AE_SHOOT:
	{
							Shoot();
	}
		break;
	case EUGENE_EQUIP:
	{
						 pev->body = 0;
	}
		break;
	case EUGENE_STRAFE_LEFT:
	{
							   RollCooldown = gpGlobals->time + 1;
							   justfired = false;
							   //UTIL_MoveToOrigin(ENT(pev), pev->origin - gpGlobals->v_right * 64, 32, MOVE_STRAFE);
							   pev->velocity = pev->velocity + gpGlobals->v_up * 50 - gpGlobals->v_right * 150;
	}
		break;
	case EUGENE_STRAFE_RIGHT:
	{
								RollCooldown = gpGlobals->time + 1;
								justfired = false;
								pev->velocity = pev->velocity + gpGlobals->v_up * 50 + gpGlobals->v_right * 150;
								//UTIL_MoveToOrigin(ENT(pev), pev->origin + gpGlobals->v_right * 64, 32, MOVE_STRAFE);
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
void CEugene::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/eugene.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.hgruntHealth * 8;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	DodgePoints = RANDOM_LONG(0, 15);
	RollCooldown = gpGlobals->time;
	justfired = false;

	m_cAmmoLoaded = 17;
	MonsterInit();
	SetSkin();
}

void CEugene::SetSkin()
{
	pev->skin = 0;
	pev->body = 1;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEugene::Precache()
{
	int i;

	PRECACHE_MODEL("models/eugene.mdl");
	PRECACHE_SOUND("weapons/pl_gun3.wav");
	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	m_iShotgunShell = PRECACHE_MODEL("models/shell.mdl");
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CEugene::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK2) || (m_Activity == ACT_RANGE_ATTACK1))
	{
		if (justfired && gpGlobals->time > RollCooldown)
		{
			if (RANDOM_LONG(0, 1))
				m_IdealActivity = ACT_STRAFE_LEFT;
			else
				m_IdealActivity = ACT_STRAFE_RIGHT;
		}

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

int CEugene::IRelationship(CBaseEntity *pTarget)
{
	return CBaseMonster::IRelationship(pTarget);
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CEugene::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist >= 512)
	{
		return FALSE;
	}

	if (flDist > 100 && flDist <= 1200 && flDot >= 0.5) //&& gpGlobals->time >= m_flNextShootTime
	{
		/*if (m_hEnemy != NULL)
		{
		if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 80)
		{
		// don't try to spit at someone up really high or down really low.
		return FALSE;
		}
		}*/
		return TRUE;
	}

	return FALSE;
}

void CEugene::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	justfired = true;

	Vector vecShootOrigin = pev->origin + gpGlobals->v_up * 42 + gpGlobals->v_right * 8 + gpGlobals->v_forward * 12;
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 2048, BULLET_MONSTER_9MM, 0); // shoot +-7.5 degrees
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1.0, ATTN_NORM, 0, 100);

	pev->effects = EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	m_flNextShootTime = gpGlobals->time + 1;
}

void CEugene::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

void CEugene::Dodged(void)
{
	ResetSequenceInfo();
	m_IdealMonsterState = MONSTERSTATE_IDLE;
}

//=========================================================
// start task
//=========================================================
void CEugene::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CEugene::CheckAmmo(void)
{
	if (m_cAmmoLoaded <= 0)
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}