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
// monster template
//=========================================================
// UNDONE: Holster weapon?
// TEST SIEMKA321 FOR GIT EXTENSIONS
// WOOP WOOP

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"

#define	MELEE_ATTACK1		( 1 )
#define	RANGE_ATTACK1		( 2 )
#define	MELEE_ATTACK2		( 3 )
#define	MELEE_SLIDE			( 4 )


class CGeorgeDroid : public CTalkMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  ISoundMask(void);
	void BarneyFirePistol(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void Shoot(bool special);
	int iSkibidiFlash;
	int	m_iShell;
	int LastSpoken;
	bool slidehit = false;

	void RunAI(void);

	float m_flNextFlinch;
	float m_flSpecialAttack;
	float shootrightoffset;

	bool ChimpOut = false;

	void RunTask(Task_t *pTask);
	void StartTask(Task_t *pTask);
	virtual int	ObjectCaps(void) { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);

	float LastSpecialUsed;

	void DeclineFollowing(void);

	// Override these to set behavior
	Schedule_t *GetScheduleOfType(int Type);
	Schedule_t *GetSchedule(void);
	int IgnoreConditions(void);
	MONSTERSTATE GetIdealState(void);

	void TalkInit(void);

	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed(entvars_t *pevAttacker, int iGib);
	void Mutate(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pDeathSounds[];
	static const char *pPainSounds[];

	void PainSound(void);
	void DeathSound(void);

	Vector ShootAtEnemyDroid(const Vector &shootOrigin);

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS(monster_georgedroid, CGeorgeDroid);

TYPEDESCRIPTION	CGeorgeDroid::m_SaveData[] =
{
	DEFINE_FIELD(CGeorgeDroid, m_painTime, FIELD_TIME),
	DEFINE_FIELD(CGeorgeDroid, m_checkAttackTime, FIELD_TIME),
	DEFINE_FIELD(CGeorgeDroid, m_lastAttackCheck, FIELD_BOOLEAN),
	DEFINE_FIELD(CGeorgeDroid, m_flPlayerDamage, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CGeorgeDroid, CTalkMonster);

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlMLuFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, (float)128 },	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE },
};

Schedule_t	slMLuFollow[] =
{
	{
		tlMLuFollow,
		ARRAYSIZE(tlMLuFollow),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================

Task_t	tlMLuFaceTarget[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};

Schedule_t	slMLuFaceTarget[] =
{
	{
		tlMLuFaceTarget,
		ARRAYSIZE(tlMLuFaceTarget),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleMLuStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 }, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET, (float)0 }, // reset head position
};

Schedule_t	slIdleMLuStand[] =
{
	{
		tlIdleMLuStand,
		ARRAYSIZE(tlIdleMLuStand),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|

		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES(CGeorgeDroid)
{
	slMLuFollow,
		slMLuFaceTarget,
		slIdleMLuStand,
};


IMPLEMENT_CUSTOM_SCHEDULES(CGeorgeDroid, CTalkMonster);

void CGeorgeDroid::StartTask(Task_t *pTask)
{
	CTalkMonster::StartTask(pTask);
}

void CGeorgeDroid::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask(pTask);
		break;
	default:
		CTalkMonster::RunTask(pTask);
		break;
	}
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CGeorgeDroid::ISoundMask(void)
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_GARBAGE |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGeorgeDroid::Classify(void)
{
	return	CLASS_PLAYER_ALLY;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGeorgeDroid::SetYawSpeed(void)
{
	pev->yaw_speed = 900;
}


//=========================================================
// Spawn
//=========================================================
void CGeorgeDroid::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/georgedroid.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 1000;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.1;
	m_MonsterState = MONSTERSTATE_NONE;
	LastSpecialUsed = gpGlobals->time;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	ChimpOut = false;

	MonsterInit();
	SetUse(&CTalkMonster::FollowerUse);
}

const char *CGeorgeDroid::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CGeorgeDroid::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CGeorgeDroid::pDeathSounds[] =
{
	"droid/death1.wav",
	"droid/death2.wav",
	"droid/death3.wav"
};

const char *CGeorgeDroid::pPainSounds[] =
{
	"droid/pain1.wav",
	"droid/pain2.wav",
	"droid/pain3.wav",
	"droid/pain4.wav",
};

void CGeorgeDroid::PainSound(void)
{
	if (LastSpoken < gpGlobals->time)
	{
		int pitch = 95 + RANDOM_LONG(0, 9);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
		LastSpoken = gpGlobals->time + 1;
	}
}

void CGeorgeDroid::DeathSound(void)
{
	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pDeathSounds[RANDOM_LONG(0, ARRAYSIZE(pDeathSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGeorgeDroid::Precache()
{
	PRECACHE_MODEL("models/georgedroid.mdl");
	PRECACHE_SOUND("squeek/sqk_blast1.wav");
	PRECACHE_SOUND("droid/footstep1.wav");
	PRECACHE_SOUND("droid/footstep2.wav");
	PRECACHE_SOUND("common/fatroll.wav");
	PRECACHE_SOUND("droid/taunt.wav");
	PRECACHE_SOUND("droid/jump1.wav");
	PRECACHE_SOUND("turret/tu_fire1.wav");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");
	iSkibidiFlash = PRECACHE_MODEL("sprites/muz8.spr");


	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}


// Init talk data
void CGeorgeDroid::TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_USE] = "DROID_OK";
	m_szGrp[TLK_UNUSE] = "DROID_WAIT";
	m_szGrp[TLK_STOP] = "DROID_WAIT";

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CGeorgeDroid::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case MELEE_ATTACK1:
	{
					// do stuff for this event.
					//		ALERT( at_console, "Slash right!\n" );
					CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgOneSlash * 5, DMG_SLASH);
					if (pHurt)
					{
						if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
						{
							pHurt->pev->punchangle.z = -18;
							pHurt->pev->punchangle.x = 5;
							pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100;
						}
						// Play a random attack hit sound
						EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
					}
					else // Play a random attack miss sound
						EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

	}
		break;

	case RANGE_ATTACK1:
	{
		Shoot(false);
	}
		break;
	case MELEE_ATTACK2:
	{
						  ChimpOut = true;
						  CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgOneSlash * 5, DMG_SLASH);
						  if (pHurt)
						  {
							  if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
							  {
								  UTIL_MakeVectors(pev->angles);
								  pHurt->pev->punchangle.z = -18;
								  pHurt->pev->punchangle.x = 5;
								  pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100;
							  }
							  // Play a random attack hit sound
							  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
						  }
						  else // Play a random attack miss sound
							  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
		break;
	case MELEE_SLIDE:
	{
						if (slidehit)
							break;

						CBaseEntity *pHurt = CheckTraceHullAttack(100, gSkillData.zombieDmgOneSlash * 2, DMG_SLASH);
						if (pHurt)
						{
							if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
							{
								UTIL_MakeVectors(pev->angles);
								pHurt->pev->punchangle.z = -18;
								pHurt->pev->punchangle.x = 5;
								pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 400 + gpGlobals->v_up * 128;
							}
							// Play a random attack hit sound
							EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
							slidehit = true;
						}
	}
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}


static BOOL IsFacing(entvars_t *pevTest, const Vector &reference)
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate(angle, forward, NULL, NULL);
	// He's facing me, he meant it
	if (DotProduct(forward, vecDir) > 0.96)	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


int CGeorgeDroid::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT))
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy == NULL)
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing(pevAttacker, pev->origin))
			{
				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(TRUE);
			}
		}
	}

	return ret;
}


void CGeorgeDroid::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	UTIL_Ricochet(ptr->vecEndPos, 1.0);
	CTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


void CGeorgeDroid::Killed(entvars_t *pevAttacker, int iGib)
{
	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CGeorgeDroid::GetScheduleOfType(int Type)
{
	Schedule_t *psched;

	switch (Type)
	{

		// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slMLuFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slMLuFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleMLuStand;
		}
		else
			return psched;
	}

	return CTalkMonster::GetScheduleOfType(Type);
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CGeorgeDroid::GetSchedule(void)
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
								// dead enemy
								if (HasConditions(bits_COND_ENEMY_DEAD))
								{
									// call base class, all code to handle dead enemies is centralized there.
									//Do a quick chimpout check to see if there's enemies nearby
									CBaseEntity *pEntity = NULL;
									while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 256)) != NULL)
									{
										if (pEntity)
										{
											if ((pEntity != NULL) && (pEntity->Classify() == CLASS_HUMAN_MILITARY))
											{
												m_hTargetEnt = pEntity;
												m_hEnemy = pEntity;
												//m_MonsterState = MONSTERSTATE_COMBAT;
												SetConditions(bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY);
												MoveToTarget(ACT_RUN, 0.0);
												ALERT(at_console, "ADDITIONAL DROYD CHECK: %d\n", pev);
												ChimpOut = false;
												break;
											}
										}
									}
									return CBaseMonster::GetSchedule();
								}

	}
		break;

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		if (m_hEnemy == NULL && IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(FALSE);
				break;
			}
			else
			{
				if (HasConditions(bits_COND_CLIENT_PUSH))
				{
					return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);
			}
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))
		{
			return GetScheduleOfType(SCHED_MOVE_AWAY);
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CGeorgeDroid::GetIdealState(void)
{
	return CTalkMonster::GetIdealState();
}



void CGeorgeDroid::DeclineFollowing(void)
{
	PlaySentence("DROID_WAIT", 2, VOL_NORM, ATTN_NORM);
}

BOOL CGeorgeDroid::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 80 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CGeorgeDroid::CheckMeleeAttack2(float flDot, float flDist)
{
	if (flDist <= 180 && flDist > 100 && flDot >= 0.7 && gpGlobals->time > LastSpecialUsed)
	{
		slidehit = false;
		pev->velocity = pev->velocity + gpGlobals->v_forward * 600 + gpGlobals->v_up * 128;
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "common/fatroll.wav", 0.8, ATTN_NORM, 0, 100);
		LastSpecialUsed = gpGlobals->time + 4;
		return TRUE;
	}
	return FALSE;
}


BOOL CGeorgeDroid::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist <= 256 && flDist > 100 && flDot >= 0.7 && gpGlobals->time <= LastSpecialUsed)
	{
		if (m_flNextAttack <= gpGlobals->time)
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CGeorgeDroid::CheckRangeAttack2(float flDot, float flDist)
{
	return FALSE;
	if (flDist > 100 && flDot >= 0.7)
	{
		if (m_flNextAttack <= gpGlobals->time)
		{
			shootrightoffset = 3;
			m_flNextAttack = gpGlobals->time + 3;
			return TRUE;
		}	
	}

	return FALSE;
}


int CGeorgeDroid::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (m_Activity != ACT_RANGE_ATTACK1)
	{
		pev->body = 0;
	}

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK2) || (m_Activity == ACT_RANGE_ATTACK1))
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
			m_flNextFlinch = gpGlobals->time + 8;
	}

	return iIgnore;
}

void CGeorgeDroid::Shoot(bool special)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = pev->origin + gpGlobals->v_up * 50 + gpGlobals->v_forward * 38 + gpGlobals->v_right * 8;
	Vector vecShootDir = ShootAtEnemyDroid(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	if (!special)
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_9MM, 1);
	else
	{
		vecShootDir = gpGlobals->v_forward * 15 + gpGlobals->v_right * shootrightoffset;
		shootrightoffset -= 0.5;
		FireBullets(3, vecShootOrigin, vecShootDir, VECTOR_CONE_20DEGREES, 2048, BULLET_MONSTER_9MM, 1);
	}
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1.0, ATTN_NORM, 0, 100);

	pev->effects = pev->effects | EF_MUZZLEFLASH;

	/*MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecShootOrigin);
	WRITE_BYTE(TE_SPRITE);
	WRITE_COORD(vecShootOrigin.x);	// pos
	WRITE_COORD(vecShootOrigin.y);
	WRITE_COORD(vecShootOrigin.z);
	WRITE_SHORT(iSkibidiFlash);		// model
	WRITE_BYTE(2);				// size * 10
	WRITE_BYTE(128);			// brightness
	MESSAGE_END();*/

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHOTSHELL);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	//m_flNextShootTime = gpGlobals->time + 1;
}

Vector CGeorgeDroid::ShootAtEnemyDroid(const Vector &shootOrigin)
{
	CBaseEntity *pEnemy = m_hEnemy;

	if (pEnemy)
	{
		return ((pEnemy->Center() - pEnemy->pev->origin) + m_vecEnemyLKP - shootOrigin).Normalize();
	}
	else
		return gpGlobals->v_forward;
}

void CGeorgeDroid::RunAI(void)
{
	CBaseMonster::RunAI();
	if (ChimpOut)
	{
		if (m_hEnemy == NULL && m_Activity == ACT_IDLE)
		{
			CBaseEntity *pEntity = NULL;
			while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 512)) != NULL)
			{
				if (pEntity)
				{
					if ((pEntity != NULL) && (pEntity->Classify() == CLASS_HUMAN_MILITARY))
					{
						m_hTargetEnt = pEntity;
						m_hEnemy = pEntity;
						//m_MonsterState = MONSTERSTATE_COMBAT;
						SetConditions(bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY);
						MoveToTarget(ACT_RUN, 0.0);
						ALERT(at_console, "GEORGE DROYD RUN: %d\n", pev);
						ChimpOut = false;
						break;
					}
				}
			}
		}
	}
}