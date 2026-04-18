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
// Empowered Alien Slave -- exterminators.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "effects.h"
#include "weapons.h"
#include "soundent.h"

#define ISLAVE_EMP_GLOW_SPRITE "sprites/glow03.spr"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ISLAVE_EMPOWERED_AE_CLAW (1)
#define ISLAVE_EMPOWERED_AE_CLAWRAKE (2)
#define ISLAVE_EMPOWERED_AE_ZAP_POWERUP (3)
#define ISLAVE_EMPOWERED_AE_ZAP_SHOOT (4)
#define ISLAVE_EMPOWERED_AE_ZAP_DONE (5)
#define ISLAVE_EMPOWERED_AE_SONIC_ATTACK (6)

#define ISLAVE_EMPOWERED_MAX_BEAMS 8

class CISlaveEmpowered : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int ISoundMask();
	int Classify();
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);

	void DeathSound();
	void PainSound();
	void AlertSound();
	void IdleSound();

	void SonicAttack();

	void Killed(entvars_t* pevAttacker, int iGib);

	void StartTask(Task_t* pTask);
	Schedule_t* GetSchedule();
	Schedule_t* GetScheduleOfType(int Type);
	CUSTOM_SCHEDULES;

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void ClearBeams();
	void ArmBeam(int side);
	void WackBeam(int side, CBaseEntity* pEntity);
	void ZapBeam(int side);
	void BeamGlow();

	CBeam* m_pBeam[ISLAVE_EMPOWERED_MAX_BEAMS];

	int m_iBeams;
	float m_flNextAttack;
	float m_flNextRevive;

	int m_iSpriteTexture;

	EHANDLE m_hDead;

	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	static const char* pPainSounds[];
	static const char* pDeathSounds[];

private:
	CSprite* m_pGlow;
};

LINK_ENTITY_TO_CLASS(monster_alien_slave_empowered, CISlaveEmpowered);


TYPEDESCRIPTION CISlaveEmpowered::m_SaveData[] =
{
	DEFINE_ARRAY(CISlaveEmpowered, m_pBeam, FIELD_CLASSPTR, ISLAVE_EMPOWERED_MAX_BEAMS),
	DEFINE_FIELD(CISlaveEmpowered, m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD(CISlaveEmpowered, m_flNextAttack, FIELD_TIME),
	DEFINE_FIELD(CISlaveEmpowered, m_flNextRevive, FIELD_TIME),
	DEFINE_FIELD(CISlaveEmpowered, m_hDead, FIELD_EHANDLE),
	DEFINE_FIELD(CISlaveEmpowered, m_iSpriteTexture, FIELD_INTEGER),
	DEFINE_FIELD(CISlaveEmpowered, m_pGlow, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CISlaveEmpowered, CBaseMonster);


const char* CISlaveEmpowered::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* CISlaveEmpowered::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* CISlaveEmpowered::pPainSounds[] =
{
	"aslavemp/slv_pain1.wav",
	"aslavemp/slv_pain2.wav",
};

const char* CISlaveEmpowered::pDeathSounds[] =
{
	"aslavemp/slv_die1.wav",
	"aslavemp/slv_die2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CISlaveEmpowered::Classify()
{
	return CLASS_ALIEN_SLAVE_EMPOWERED;
}

//=========================================================
// ALertSound - scream
//=========================================================
void CISlaveEmpowered::AlertSound()
{
	if (m_hEnemy != NULL)
	{
		SENTENCEG_PlayRndSz(ENT(pev), "SLVMP_ALERT", 0.85, ATTN_NORM, 0, PITCH_NORM);
	}
}

//=========================================================
// IdleSound
//=========================================================
void CISlaveEmpowered::IdleSound()
{
	if (RANDOM_LONG(0, 2) == 0)
	{
		SENTENCEG_PlayRndSz(ENT(pev), "SLVMP_IDLE", 0.85, ATTN_NORM, 0, PITCH_NORM);
	}

#if 0
	int side = RANDOM_LONG(0, 1) * 2 - 1;

	ClearBeams();
	ArmBeam(side);

	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_right * 2 * side;
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSrc);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(vecSrc.x);	// X
	WRITE_COORD(vecSrc.y);	// Y
	WRITE_COORD(vecSrc.z);	// Z
	WRITE_BYTE(8);		// radius * 0.1
	WRITE_BYTE(255);		// r
	WRITE_BYTE(180);		// g
	WRITE_BYTE(96);		// b
	WRITE_BYTE(10);		// time * 10
	WRITE_BYTE(0);		// decay * 0.1
	MESSAGE_END();

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/zap1.wav", 1, ATTN_NORM, 0, 100);
#endif
}

//=========================================================
// PainSound
//=========================================================
void CISlaveEmpowered::PainSound()
{
	if (RANDOM_LONG(0, 2) == 0)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, PITCH_NORM);
	}
}

//=========================================================
// DieSound
//=========================================================

void CISlaveEmpowered::DeathSound()
{
	m_pGlow->TurnOff();
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pDeathSounds), 1.0, ATTN_NORM, 0, PITCH_NORM);
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CISlaveEmpowered::ISoundMask()
{
	return bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}


void CISlaveEmpowered::Killed(entvars_t* pevAttacker, int iGib)
{
	m_pGlow->TurnOff();
	ClearBeams();
	CBaseMonster::Killed(pevAttacker, GIB_NEVER);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CISlaveEmpowered::SetYawSpeed()
{
	int ys;

	ys = 240;

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CISlaveEmpowered::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch (pEvent->event)
	{
	case ISLAVE_EMPOWERED_AE_ZAP_POWERUP:
	{
		// hard-coding speedup attack to prevent cheating
		pev->framerate = 2;
		
		if (g_iSkillLevel == SKILL_HARD)
			pev->framerate = 2.5;

		UTIL_MakeAimVectors(pev->angles);

		if (m_iBeams == 0)
		{
			Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;
			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSrc);
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD(vecSrc.x);			 // X
			WRITE_COORD(vecSrc.y);			 // Y
			WRITE_COORD(vecSrc.z);			 // Z
			WRITE_BYTE(12);					 // radius * 0.1
			WRITE_BYTE(96);				 // r
			WRITE_BYTE(176);				 // g
			WRITE_BYTE(255);					 // b
			WRITE_BYTE(20 / pev->framerate); // time * 10
			WRITE_BYTE(0);					 // decay * 0.1
			MESSAGE_END();
		}
		if (m_hDead != NULL)
		{
			WackBeam(-1, m_hDead);
			WackBeam(1, m_hDead);
		}
		else
		{
			ArmBeam(-1);
			ArmBeam(1);
			BeamGlow();
		}

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10);
		pev->skin = m_iBeams / 2;
	}
	break;

	case ISLAVE_EMPOWERED_AE_SONIC_ATTACK:
	{
		SonicAttack();

		// blast circles
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_BEAMCYLINDER);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 16);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(1500); // reach damage radius over .3 seconds
		WRITE_SHORT(m_iSpriteTexture);
		WRITE_BYTE(0);	// startframe
		WRITE_BYTE(0);	// framerate
		WRITE_BYTE(2);	// life
		WRITE_BYTE(64); // width
		WRITE_BYTE(0);	// noise
		WRITE_BYTE(96);
		WRITE_BYTE(168);
		WRITE_BYTE(255);
		WRITE_BYTE(255); //brightness
		WRITE_BYTE(0);	 // speed
		MESSAGE_END();

		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_BEAMCYLINDER);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 16);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(1000); // reach damage radius over .3 seconds
		WRITE_SHORT(m_iSpriteTexture);
		WRITE_BYTE(0);	// startframe
		WRITE_BYTE(0);	// framerate
		WRITE_BYTE(2);	// life
		WRITE_BYTE(32); // width
		WRITE_BYTE(0);	// noise
		WRITE_BYTE(96);
		WRITE_BYTE(168);
		WRITE_BYTE(255);
		WRITE_BYTE(255); //brightness
		WRITE_BYTE(0);	 // speed
		MESSAGE_END();
	}
	break;

	case ISLAVE_EMPOWERED_AE_ZAP_SHOOT:
	{
		ClearBeams();

		if (m_hDead != NULL)
		{
			Vector vecDest = m_hDead->pev->origin + Vector(0, 0, 38);
			TraceResult trace;
			UTIL_TraceHull(vecDest, vecDest, dont_ignore_monsters, human_hull, m_hDead->edict(), &trace);

			if (m_hDead->pev->deadflag == DEAD_DEAD && 0 == trace.fStartSolid)
			{
				CBaseEntity* pNew = Create("monster_alien_slave_empowered", m_hDead->pev->origin, m_hDead->pev->angles);
				CBaseMonster* pNewMonster = pNew->MyMonsterPointer();
				pNew->pev->spawnflags |= 1;
				WackBeam(-1, pNew);
				WackBeam(1, pNew);
				UTIL_Remove(m_hDead);
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG(130, 160));

				// Wait 10 seconds until you can revive their comrade.
				m_flNextRevive = gpGlobals->time + 5.0; // 1 second zap

				/*
					CBaseEntity *pEffect = Create( "test_effect", pNew->Center(), pev->angles );
					pEffect->Use( this, this, USE_ON, 1 );
					*/
				break;
			}
		}
		ClearMultiDamage();

		UTIL_MakeAimVectors(pev->angles);

		ZapBeam(-1);
		ZapBeam(1);

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG(130, 160));
		// STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
		ApplyMultiDamage(pev, pev);

		m_flNextAttack = gpGlobals->time + 1.0; // 1 second zap
	}
	break;

	case ISLAVE_EMPOWERED_AE_ZAP_DONE:
	{
		ClearBeams();
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// CheckRangeAttack1 - normal beam attack
//=========================================================
BOOL CISlaveEmpowered::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_flNextAttack > gpGlobals->time)
	{
		return FALSE;
	}
	if (flDist > 64 && flDist <= 1536 && flDot >= 0.5)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - check bravery and try to resurect dead comrades
//=========================================================
BOOL CISlaveEmpowered::CheckRangeAttack2(float flDot, float flDist)
{
	if (m_flNextRevive > gpGlobals->time)
	{
		return FALSE;
	}

	CBaseEntity* pList[2];
	int count = UTIL_EntitiesInBox(pList, 2, pev->absmin - 32, pev->absmax - 32, FL_MONSTER);

	if (0 != count)
	{
		return FALSE;
	}

	// Don't revive teammates while the gargantua is around.
	if (FClassnameIs(m_hEnemy->pev, "monster_gargantua"))
		return FALSE;

	m_hDead = NULL;

	CBaseEntity* pEntity = NULL;
	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "monster_alien_slave_empowered")) != NULL)
	{
		TraceResult tr;

		UTIL_TraceLine(EyePosition(), pEntity->EyePosition(), ignore_monsters, ENT(pev), &tr);
		if (tr.flFraction == 1.0 || tr.pHit == pEntity->edict())
		{
			if (pEntity->pev->deadflag == DEAD_DEAD)
			{
				float d = (pev->origin - pEntity->pev->origin).Length();
				if (d < flDist)
				{
					m_hDead = pEntity;
					flDist = d;
				}
			}
		}
	}
	if (m_hDead != NULL)
		return TRUE;
	else
		return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - sonic wave attack
//=========================================================
BOOL CISlaveEmpowered::CheckMeleeAttack1(float flDot, float flDist)
{
	if (!HasConditions(bits_COND_CAN_RANGE_ATTACK1))
	{
		if (flDist <= 192 && flDot >= 0.7)
		{
			return TRUE;
		}
	}
	return false;
}

//=========================================================
// StartTask
//=========================================================
void CISlaveEmpowered::StartTask(Task_t* pTask)
{
	ClearBeams();

	CBaseMonster::StartTask(pTask);
}


//=========================================================
// Spawn
//=========================================================
void CISlaveEmpowered::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/islave_emp.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->effects = 0;
	pev->health = gSkillData.slaveEmpHealth;
	pev->view_ofs = Vector(0, 0, 64);  // position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP;

	// Don't instantly revive their comrade after revived.
	m_flNextRevive = gpGlobals->time + 5.0;

	// glow if they're aggresive
	if ((pev->spawnflags & SF_MONSTER_PRISONER) == 0)
	{
		m_pGlow = CSprite::SpriteCreate(ISLAVE_EMP_GLOW_SPRITE, pev->origin + Vector(0, 0, (pev->mins.z + pev->maxs.z) * 0.5), false);
		m_pGlow->SetTransparency(kRenderGlow, 48, 250, 4, 200, kRenderFxNoDissipation);
		m_pGlow->SetScale(0.5);
		m_pGlow->SetAttachment(edict(), 3);
	}

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CISlaveEmpowered::Precache()
{
	PRECACHE_MODEL("models/islave_emp.mdl");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	PRECACHE_SOUND("zombie/zo_pain2.wav");
	PRECACHE_SOUND("headcrab/hc_headbite.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
	PRECACHE_SOUND("debris/beamstart1.wav");
	PRECACHE_SOUND("debris/beamstart15.wav");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);

	PRECACHE_SOUND("houndeye/he_blast1.wav");
	PRECACHE_SOUND("houndeye/he_blast2.wav");
	PRECACHE_SOUND("houndeye/he_blast3.wav");

	UTIL_PrecacheOther("test_effect");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/shockwave.spr");
}


//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

void CISlaveEmpowered::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if ((bitsDamageType & DMG_SHOCK ) != 0)
		return;

	if (bitsDamageType == DMG_BURN)
	{
		flDamage = flDamage / 10;
	}

	// Don't take any acid damage -- BigMomma's mortar is acid
	if ((bitsDamageType & DMG_ACID) != 0)
		return;

	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================



// primary range attack
Task_t tlSlaveEmpAttack1[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_FACE_IDEAL, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slSlaveEmpAttack1[] =
{
	{tlSlaveEmpAttack1,
		ARRAYSIZE(tlSlaveEmpAttack1),
		bits_COND_CAN_MELEE_ATTACK1 |
			bits_COND_HEAR_SOUND |
			bits_COND_HEAVY_DAMAGE,

		bits_SOUND_DANGER,
		"Slave Range Attack1"},
};


DEFINE_CUSTOM_SCHEDULES(CISlaveEmpowered) {
	slSlaveEmpAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES(CISlaveEmpowered, CBaseMonster);


//=========================================================
//=========================================================
Schedule_t* CISlaveEmpowered::GetSchedule()
{
	ClearBeams();

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}
		break;
	}
	return CBaseMonster::GetSchedule();
}


Schedule_t* CISlaveEmpowered::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_FAIL:
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK2))
		{
			return CBaseMonster::GetScheduleOfType(SCHED_RANGE_ATTACK2);
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slSlaveEmpAttack1;
	case SCHED_RANGE_ATTACK2:
		return slSlaveEmpAttack1;
	}
	return CBaseMonster::GetScheduleOfType(Type);
}


//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void CISlaveEmpowered::ArmBeam(int side)
{
	TraceResult tr;
	float flDist = 1.0;

	if (m_iBeams >= ISLAVE_EMPOWERED_MAX_BEAMS)
		return;

	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT(0, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1);
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT(pev), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if (flDist == 1.0)
		return;

	//DecalGunshot(&tr, BULLET_PLAYER_CROWBAR);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
	// m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetColor(64, 160, 255);
	m_pBeam[m_iBeams]->SetBrightness(64);
	m_pBeam[m_iBeams]->SetNoise(80);
	m_iBeams++;
}


//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CISlaveEmpowered::BeamGlow()
{
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i]->GetBrightness() != 255)
		{
			m_pBeam[i]->SetBrightness(b);
		}
	}
}


//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CISlaveEmpowered::WackBeam(int side, CBaseEntity* pEntity)
{
	Vector vecDest;
	float flDist = 1.0;

	if (m_iBeams >= ISLAVE_EMPOWERED_MAX_BEAMS)
		return;

	if (pEntity == NULL)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(pEntity->Center(), entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
	m_pBeam[m_iBeams]->SetColor(96, 168, 255);
	m_pBeam[m_iBeams]->SetBrightness(255);
	m_pBeam[m_iBeams]->SetNoise(80);
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CISlaveEmpowered::ZapBeam(int side)
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity* pEntity;

	if (m_iBeams >= ISLAVE_EMPOWERED_MAX_BEAMS)
		return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	vecAim = ShootAtEnemy(vecSrc);
	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT(0, deflection) + gpGlobals->v_up * RANDOM_FLOAT(-deflection, deflection);
	UTIL_TraceLine(vecSrc, vecSrc + vecAim * 1536, dont_ignore_monsters, ENT(pev), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 50);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
	m_pBeam[m_iBeams]->SetColor(96, 168, 255);
	m_pBeam[m_iBeams]->SetBrightness(255);
	m_pBeam[m_iBeams]->SetNoise(20);
	m_iBeams++;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && 0 != pEntity->pev->takedamage)
	{
		// deal extra damage to monsters, so they don't get stuck on a long fight.
		if (!pEntity->IsPlayer())
		{
			pEntity->TraceAttack(pev, gSkillData.slaveEmpDmgZap * 6, vecAim, &tr, DMG_SHOCK | DMG_NEVERGIB);
		}
		else
		{
			pEntity->TraceAttack(pev, gSkillData.slaveEmpDmgZap, vecAim, &tr, DMG_SHOCK);
		}
	}
	UTIL_EmitAmbientSound(ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG(140, 160));
}


//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CISlaveEmpowered::ClearBeams()
{
	for (int i = 0; i < ISLAVE_EMPOWERED_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;

	STOP_SOUND(ENT(pev), CHAN_WEAPON, "debris/zap4.wav");
}

//=========================================================
// SonicAttack
//=========================================================
void CISlaveEmpowered::SonicAttack()
{
	float flAdjustedDamage;
	float flDist;

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "houndeye/he_blast1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "houndeye/he_blast2.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "houndeye/he_blast3.wav", 1, ATTN_NORM);
		break;
	}

	UTIL_ScreenShake(pev->origin, 12.0, 100.0, 2.0, 1000);

	CBaseEntity* pEntity = NULL;
	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 384)) != NULL)
	{
		if (pEntity->pev->takedamage != DAMAGE_NO)
		{
			if (!FClassnameIs(pEntity->pev, "monster_alien_slave_empowered") 
				&& !FClassnameIs(pEntity->pev, "monster_controller_exterminator")
				&& !FClassnameIs(pEntity->pev, "monster_alien_slave"))
			{
				flAdjustedDamage = gSkillData.slaveEmpDmgSonic;

				flDist = (pEntity->Center() - pev->origin).Length();

				if (!pEntity->IsPlayer())
				{
					flAdjustedDamage *= 4;
				}

				flAdjustedDamage -= (flDist / 384) * flAdjustedDamage;

				if (!FVisible(pEntity))
				{
					if (pEntity->IsPlayer())
					{
						// if this entity is a client, and is not in full view, inflict half damage. We do this so that players still
						// take the residual damage if they don't totally leave the houndeye's effective radius. We restrict it to clients
						// so that monsters in other parts of the level don't take the damage and get pissed.
						flAdjustedDamage *= 0.5;
					}
					else if (!FClassnameIs(pEntity->pev, "func_breakable") && !FClassnameIs(pEntity->pev, "func_pushable"))
					{
						// do not hurt nonclients through walls, but allow damage to be done to breakables
						flAdjustedDamage = 0;
					}
				}

				//ALERT ( at_aiconsole, "Damage: %f\n", flAdjustedDamage );

				if (flAdjustedDamage > 0)
				{
					pEntity->TakeDamage(pev, pev, flAdjustedDamage, DMG_SONIC | DMG_ALWAYSGIB);
				}
			}
		}
	}
}
