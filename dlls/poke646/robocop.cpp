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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"soundent.h"
#include	"animation.h"
#include	"effects.h"
#include	"explode.h"

#define ROBOCOP_EYE_SPRITE_NAME			"sprites/gargeye1.spr"
#define ROBOCOP_EYE_BEAM_NAME			"sprites/smoke.spr"
#define ROBOCOP_EYE_SPOT_NAME			"sprites/gargeye1.spr"

#define ROBOCOP_MAX_MORTAR_RADIUS		256

#define ROBOCOP_MORTAR_CHARGE_TIME		2.0f

#define ROBOCOP_MORTAR_ATTACK_DELAY		1.2f
#define ROBOCOP_SHOCKWAVE_IMPACT_DELAY	0.9f

#define ROBOCOP_MELEE_ATTACK_DIST		128
#define ROBOCOP_RANGE_ATTACK_DIST		512
#define ROBOCOP_MAX_CHASE_DIST			1024

#define ROBOCOP_DEATH_DURATION			2.1f
#define ROBOCOP_GIB_MODEL			"models/metalplategibs.mdl"

// Robocop is immune to any damage but this
#define ROBOCOP_DAMAGE			(DMG_ENERGYBEAM|DMG_CRUSH|DMG_MORTAR|DMG_BLAST)

void SpawnExplosion( Vector center, float randomRange, float time, int magnitude );

// AI Nodes for RoboCop
class CInfoRCNode : public CPointEntity
{
public:
	void Spawn(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CInfoRCNode* m_pNext;
};

LINK_ENTITY_TO_CLASS(info_rc_node, CInfoRCNode);

TYPEDESCRIPTION	CInfoRCNode::m_SaveData[] =
{
	DEFINE_FIELD(CInfoRCNode, m_pNext, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CInfoRCNode, CPointEntity);

void CInfoRCNode::Spawn(void)
{
}

// AI Sector for RoboCop
class CInfoRCSector : public CPointEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData* pkvd);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CInfoRCNode*	m_pNode;
	CInfoRCSector*	m_pNext;
	int				m_sector;
	int				m_nodecount;
};

LINK_ENTITY_TO_CLASS(info_rc_sector, CInfoRCSector);

TYPEDESCRIPTION	CInfoRCSector::m_SaveData[] =
{
	DEFINE_FIELD(CInfoRCSector, m_pNode, FIELD_CLASSPTR),
	DEFINE_FIELD(CInfoRCSector, m_pNext, FIELD_CLASSPTR),
	DEFINE_FIELD(CInfoRCSector, m_sector, FIELD_INTEGER),
	DEFINE_FIELD(CInfoRCSector, m_nodecount, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CInfoRCSector, CPointEntity);

void CInfoRCSector::Spawn(void)
{
}

void CInfoRCSector::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sector"))
	{
		m_sector = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

#define BEGIN_RC_SECTOR( sector, owner ) \
{\
	CInfoRCSector* pSector = (CInfoRCSector*)CBaseEntity::Create("info_rc_sector", g_vecZero, g_vecZero, NULL);\
	\
	if (pSector)\
	{\
		pSector->m_pNext = NULL;\
		pSector->m_pNode = NULL;\
		pSector->m_sector = sector;\
		pSector->m_nodecount = 0;\
		\
		if (owner)\
		{\
			if (owner->m_pSector == NULL)\
			{\
				owner->m_pSector = pSector;\
			}\
			else\
			{\
				pSector->m_pNext = owner->m_pSector;\
				owner->m_pSector = pSector;\
			}\
			owner->m_sectorcount++;\
		}


#define END_RC_SECTOR() \
	}\
}


#define ADD_RC_NODE( coordx, coordy, coordz ) \
{\
	CInfoRCNode* pNode = (CInfoRCNode*)CBaseEntity::Create("info_rc_node", Vector(coordx, coordy, coordz), g_vecZero, NULL); \
	if (pNode)\
	{\
		if (pSector->m_pNode == NULL)\
		{\
			pNode->m_pNext = NULL;\
			pSector->m_pNode = pNode;\
		}\
		else\
		{\
			pNode->m_pNext = pSector->m_pNode;\
			pSector->m_pNode = pNode;\
		}\
		pSector->m_nodecount++;\
	}\
}



//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ROBOCOP_AE_RIGHT_FOOT		0x03
#define ROBOCOP_AE_LEFT_FOOT		0x04
#define ROBOCOP_AE_FIST				0x05


class CRoboCop : public CBaseMonster
{
public:

	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	int  ISoundMask(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	BOOL ShouldFadeOnDeath(void) { return TRUE; }

	void MonsterThink(void);

	BOOL FCanCheckAttacks(void);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist) { return FALSE; }
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }

	void PrescheduleThink(void);
	void ScheduleChange(void);
	BOOL ShouldGibMonster(int iGib) { return FALSE; }
	void Killed(entvars_t *pevAttacker, int iGib);
	void UpdateOnRemove();

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	Schedule_t *GetSchedule(void);
	Schedule_t *GetScheduleOfType(int Type);
	void StartTask(Task_t *pTask);
	void RunTask(Task_t *pTask);

	int	Save(CSave &save);
	int Restore(CRestore &restore);
	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	void CreateEffects(void);
	void DestroyEffects(void);

	void CreateEyeGlow(void);
	void DestroyEyeGlow(void);

	void CreateBeam(void);
	void DestroyBeam(void);

	void CreateSpot(void);
	void DestroySpot(void);

	void EyeOff(void);
	void EyeOn(int level);
	void EyeUpdate(void);

	void BeamOff(void);
	void BeamOn(int level);
	void BeamUpdate(void);

	void SonicAttack(void);
	void MortarAttack(Vector vecSrc);

	void StartMortarAttack(void);
	void StopMortarAttack(void);

	BOOL PredictMeleeAttack(CBaseEntity* pEnemy);
	void PredictEnemyPosition(CBaseEntity* pEnemy, Vector& vecResult);

	BOOL IsEnemyReachable(CBaseEntity* pEnemy);
	void SetupNodes(float flRadius);

	int			m_iSpriteTexture;
	int			m_iRobocopGibModel;
	CSprite*	m_pEyeGlow;			// Glow around the eyes
	int			m_eyeBrightness;	// Brightness target

	CBeam*		m_pBeam;
	CSprite*	m_pBeamSpot;
	int			m_beamBrightness;

	float		m_flMortarAttackStart;
	BOOL		m_bInMortarAttack;
	BOOL		m_bAimLocked;
	Vector		m_vecMortarPos;
	float		m_flNextMortarAttack;
	BOOL		m_fMortarAttackEvent;

	CBeam*		m_pTemp;

	CInfoRCSector*	m_pSector;
	int				m_sectorcount;
	int				m_lastsector;

	float		m_flNextSparkTime;
};

void CreateRoboCopNodes(CRoboCop* pOwner);

LINK_ENTITY_TO_CLASS(monster_robocop, CRoboCop);

TYPEDESCRIPTION	CRoboCop::m_SaveData[] =
{
	DEFINE_FIELD(CRoboCop, m_iSpriteTexture, FIELD_INTEGER),
	DEFINE_FIELD(CRoboCop, m_pEyeGlow, FIELD_CLASSPTR),
	DEFINE_FIELD(CRoboCop, m_eyeBrightness, FIELD_INTEGER),
	DEFINE_FIELD(CRoboCop, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CRoboCop, m_pBeamSpot, FIELD_CLASSPTR),
	DEFINE_FIELD(CRoboCop, m_beamBrightness, FIELD_INTEGER),
	DEFINE_FIELD(CRoboCop, m_flMortarAttackStart, FIELD_TIME),
	DEFINE_FIELD(CRoboCop, m_bInMortarAttack, FIELD_BOOLEAN),
	DEFINE_FIELD(CRoboCop, m_flNextMortarAttack, FIELD_TIME),
	DEFINE_FIELD(CRoboCop, m_fMortarAttackEvent, FIELD_BOOLEAN),
	DEFINE_FIELD(CRoboCop, m_bAimLocked, FIELD_BOOLEAN),
	DEFINE_FIELD(CRoboCop, m_vecMortarPos, FIELD_POSITION_VECTOR),

	DEFINE_FIELD(CRoboCop, m_pSector, FIELD_CLASSPTR),
	DEFINE_FIELD(CRoboCop, m_sectorcount, FIELD_INTEGER),
	DEFINE_FIELD(CRoboCop, m_lastsector, FIELD_INTEGER),

	DEFINE_FIELD(CRoboCop, m_flNextSparkTime, FIELD_TIME),

};

IMPLEMENT_SAVERESTORE(CRoboCop, CBaseMonster);

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

enum
{
	SCHED_ROBOCOP_WALK = LAST_COMMON_SCHEDULE + 1,
	SCHED_ROBOCOP_RANGE_ATTACK1,
};

enum
{
	TASK_ROBOCOP_GET_PATH_TO_RANDOM_POSITION = LAST_COMMON_TASK + 1,
};

Task_t	tlRoboCopWalk[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_ROBOCOP_GET_PATH_TO_RANDOM_POSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t	slRoboCopWalk[] =
{
	{
		tlRoboCopWalk,
		ARRAYSIZE(tlRoboCopWalk),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"RoboCopWalk"
	},
};

// primary range attack
Task_t	tlRoboCopRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t	slRoboCopRangeAttack1[] =
{
	{
		tlRoboCopRangeAttack1,
		ARRAYSIZE(tlRoboCopRangeAttack1),
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"RoboCopRangeAttack1"
	},
};

DEFINE_CUSTOM_SCHEDULES(CRoboCop)
{
	slRoboCopWalk,
	slRoboCopRangeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES(CRoboCop, CBaseMonster);

//=========================================================
// ISoundMask
//=========================================================
int CRoboCop::ISoundMask(void)
{
	return 0;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CRoboCop::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRoboCop::SetYawSpeed(void)
{
	int ys;

	switch( m_Activity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CRoboCop::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case ROBOCOP_AE_RIGHT_FOOT:
	case ROBOCOP_AE_LEFT_FOOT:
		UTIL_ScreenShake( pev->origin, 4.0, 3.0, 1.0, 250.0 );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, RANDOM_LONG( 0, 1 ) ? "robocop/rc_step2.wav" : "robocop/rc_step1.wav", 1, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG( -10, 10 ) );
		break;

	case ROBOCOP_AE_FIST:
		SonicAttack();
		// ALERT(at_console, "%s Shockwave attack!\n", STRING(pev->classname));
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CRoboCop::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/robocop.mdl");
	UTIL_SetSize(pev, Vector(-64, -64, 0), Vector(64, 64, 180));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = DONT_BLEED;
	pev->health = gSkillData.robocopHealth;
	pev->view_ofs = Vector(0, 0, 172);// position of the eyes relative to monster's origin.
	m_flFieldOfView = -1.0f;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_HEAR | bits_CAP_DOORS_GROUP | bits_CAP_TURN_HEAD;

	CreateEffects();
	EyeOn(255);

	m_bInMortarAttack = FALSE;
	m_bAimLocked = FALSE;
	m_fMortarAttackEvent = FALSE;
	m_flNextMortarAttack = gpGlobals->time;

	m_pTemp = NULL;

	if (FStrEq(STRING(gpGlobals->mapname), "po_xen01") || FStrEq(STRING(gpGlobals->mapname), "pv_asl02"))
	{
		CreateRoboCopNodes( this );
	}

	MonsterInit();

	m_lastsector = -1;
	m_flNextSparkTime = 0;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRoboCop::Precache()
{
	PRECACHE_MODEL("models/robocop.mdl");

	PRECACHE_SOUND("robocop/rc_charge.wav");
	PRECACHE_SOUND("robocop/rc_fist.wav");
	PRECACHE_SOUND("robocop/rc_laser.wav");
	PRECACHE_SOUND("robocop/rc_step1.wav");
	PRECACHE_SOUND("robocop/rc_step2.wav");
	PRECACHE_SOUND("ambience/sparks.wav");

	PRECACHE_MODEL(ROBOCOP_EYE_SPRITE_NAME);
	PRECACHE_MODEL(ROBOCOP_EYE_BEAM_NAME);
	// PRECACHE_MODEL(ROBOCOP_EYE_SPOT_NAME);

	m_iSpriteTexture = PRECACHE_MODEL("sprites/xbeam3.spr");
	m_iRobocopGibModel = PRECACHE_MODEL( ROBOCOP_GIB_MODEL );

	UTIL_PrecacheOther( "monster_mortar" );
}

//=========================================================
// Monster Think - calls out to core AI functions and handles this
// monster's specific animation events
//=========================================================
void CRoboCop::MonsterThink(void)
{
	CBaseMonster::MonsterThink();

	// Override ground speed.
	if (m_Activity == ACT_WALK || m_Activity == ACT_RUN)
		m_flGroundSpeed = 200;
}

void CRoboCop::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	bitsDamageType &= ROBOCOP_DAMAGE;

	if( IsAlive() && !FBitSet( bitsDamageType, ROBOCOP_DAMAGE ) )
	{
		if( pev->dmgtime != gpGlobals->time || (RANDOM_LONG( 0, 100 ) < 20 ) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 0.5, 1.5 ) );
			pev->dmgtime = gpGlobals->time;
		}

		flDamage = 0;
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CRoboCop::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( IsAlive() )
	{
		if( !FBitSet( bitsDamageType, ROBOCOP_DAMAGE ) )
			flDamage *= 0.01;
		if( bitsDamageType & DMG_BLAST )
			SetConditions( bits_COND_LIGHT_DAMAGE );
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// FCanCheckAttacks - this is overridden for alien grunts
// because they can use their smart weapons against unseen
// enemies. Base class doesn't attack anyone it can't see.
//=========================================================
BOOL CRoboCop::FCanCheckAttacks(void)
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
// CheckMeleeAttack1 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CRoboCop::CheckMeleeAttack1(float flDot, float flDist)
{
	if( flDot >= 0.8f && flDist < gSkillData.robocopSWRadius )
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CRoboCop::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_flNextMortarAttack > gpGlobals->time)
		return FALSE;

	if (flDot >= 0.8f && flDist > gSkillData.robocopSWRadius && flDist < 4096.0f && !HasConditions(bits_COND_CAN_MELEE_ATTACK1))
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// ScheduleChange
//=========================================================
void CRoboCop::ScheduleChange(void)
{
	StopMortarAttack();

	CBaseMonster::ScheduleChange();
}

void CRoboCop::Killed(entvars_t *pevAttacker, int iGib)
{
	EyeOff();
	DestroyEffects();

	if (m_pTemp)
	{
		UTIL_Remove(m_pTemp);
		m_pTemp = NULL;
	}

	FCheckAITrigger();

	CBaseMonster::Killed(pevAttacker, GIB_NEVER);
}

void CRoboCop::UpdateOnRemove()
{
	CBaseEntity::UpdateOnRemove();

	EyeOff();
	DestroyEffects();

	if (m_pTemp)
	{
		UTIL_Remove(m_pTemp);
		m_pTemp = NULL;
	}
}
void CRoboCop::PrescheduleThink(void)
{
	CBaseMonster::PrescheduleThink();

	if (m_hEnemy)
	{
		if (PredictMeleeAttack(m_hEnemy))
		{
			SetConditions(bits_COND_CAN_MELEE_ATTACK1);
		}
		else if ((m_hEnemy->pev->origin - pev->origin).Length2D() > ROBOCOP_MAX_CHASE_DIST)
		{
			ClearConditions(bits_COND_NEW_ENEMY);
			ClearConditions(bits_COND_SEE_ENEMY);
			SetConditions(bits_COND_ENEMY_TOOFAR);
		}
	}

	EyeUpdate();

	BeamUpdate();
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t* CRoboCop::GetSchedule()
{
	// ALERT( at_console, "GetSchedule( )\n" );
	switch (m_MonsterState)
	{
	case MONSTERSTATE_SCRIPT:
		return CBaseMonster::GetSchedule();
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		// Stand still.
		return GetScheduleOfType( SCHED_IDLE_STAND );
	case MONSTERSTATE_COMBAT:

		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			return CBaseMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_SEE_ENEMY) && !HasConditions( bits_COND_ENEMY_TOOFAR ))
		{
			// shockwave
			if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
			{
				return GetScheduleOfType(SCHED_MELEE_ATTACK1);
			}

			// laser attack.
			if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
			{
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			}

			return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}

		// Wander or simply walk.
		return GetScheduleOfType(SCHED_STANDOFF);
	default:
		break;
	}

	return CBaseMonster::GetSchedule();
}

Schedule_t *CRoboCop::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return GetScheduleOfType(SCHED_ROBOCOP_RANGE_ATTACK1);

	case SCHED_ROBOCOP_RANGE_ATTACK1:
		return slRoboCopRangeAttack1;

	case SCHED_STANDOFF:
		return slRoboCopWalk;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}

void CRoboCop::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;

			StartMortarAttack();
		}
		break;

	case TASK_ROBOCOP_GET_PATH_TO_RANDOM_POSITION:
		{
			int sector, node;
			int attempts = 0;

			// Choose a random sector.
			do
			{
				sector = RANDOM_LONG(0, m_sectorcount - 1);
				if (sector == m_lastsector)
					continue;

			} while (attempts++ < m_sectorcount);

			ASSERT(sector >= 0 && sector < m_sectorcount);

			CInfoRCSector* pSector = m_pSector;

			int i = 0;
			while (i < sector)
			{
				pSector = pSector->m_pNext;
				i++;
			}

			ASSERT(pSector != NULL);


			// Choose a random node.

			node = RANDOM_LONG(0, pSector->m_nodecount - 1);

			ASSERT(node >= 0 && node < pSector->m_nodecount);

			CInfoRCNode* pNode = pSector->m_pNode;

			i = 0;

			while (i < node)
			{
				pNode = pNode->m_pNext;
				i++;
			}

			ASSERT(pNode != NULL);

			if (BuildRoute(pNode->pev->origin, bits_MF_TO_LOCATION, NULL))
			{

				MakeIdealYaw(pNode->pev->origin);
				ChangeYaw(pev->yaw_speed);

				m_lastsector = sector;

				// ALERT(at_console, "Robocop moving to sector %d\n", sector);

				TaskComplete();
			}
			else
			{
				// ALERT(at_console, "Robocop failed to find proper sector.\n");

				TaskFail();
			}
		}
		break;

	case TASK_DIE:
		m_flWaitFinished = gpGlobals->time + ROBOCOP_DEATH_DURATION;
		m_flNextSparkTime = gpGlobals->time + 0.3f;
		pev->renderamt = 19;
		pev->renderfx = kRenderFxGlowShell;
		pev->rendercolor = Vector(67, 85, 255);
		pev->health = 0;
	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}

void CRoboCop::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:

		if (!m_hEnemy)
		{
			TaskComplete();
			return;
		}

		MakeIdealYaw(m_vecEnemyLKP);
		ChangeYaw(pev->yaw_speed);

		if (!m_fSequenceFinished)
		{
			float elapsedTime = gpGlobals->time - m_flMortarAttackStart;

			if (elapsedTime > ROBOCOP_MORTAR_CHARGE_TIME)
			{
				if (!m_bAimLocked)
				{
					Vector vecPredictedPos;
					PredictEnemyPosition(m_hEnemy, vecPredictedPos);

					TraceResult tr;
					if (vecPredictedPos.z < pev->origin.z)
					{
						UTIL_TraceLine(vecPredictedPos, vecPredictedPos + Vector(0, 0, -256), dont_ignore_monsters, ENT(pev), &tr);
						int contents = UTIL_PointContents(tr.vecEndPos);

						// Enemy will hurt itself, stop task and return.
						if (contents == CONTENTS_SKY || contents == CONTENTS_LAVA || contents == CONTENTS_SLIME)
						{
							TaskComplete();
							return;
						}
					}

					Vector vecEyePos, vecAngles;
					GetAttachment(0, vecEyePos, vecAngles);
					if (m_hEnemy->pev->velocity.z <= 30 && (m_hEnemy->pev->origin.z < vecEyePos.z))
					{
						// Trace a line to our ground.
						vecPredictedPos.z = pev->origin.z;
						UTIL_TraceLine(vecEyePos, vecPredictedPos, dont_ignore_monsters, ENT(pev), &tr);
					}
					else
					{
						// Trace a line to enemy ground entity.
						UTIL_TraceLine(m_hEnemy->Center(), m_hEnemy->Center() + Vector(0, 0, -2048), ignore_monsters, ENT(pev), &tr);
						UTIL_TraceLine(vecEyePos, tr.vecEndPos, ignore_monsters, ENT(pev), &tr);
					}

					m_vecMortarPos = tr.vecEndPos;

					m_bAimLocked = TRUE;
				}
				else
				{
					if (!m_pBeam)
					{
						CreateBeam();
					}

					if (!m_pBeamSpot)
					{
						CreateSpot();
					}

					BeamOn(220);

					BeamUpdate();

					m_pBeam->SetEndAttachment(1);
					m_pBeam->SetStartPos( m_vecMortarPos );

					m_pBeamSpot->pev->origin = m_vecMortarPos;

					// Spawn the explosion.
					if (elapsedTime > (ROBOCOP_MORTAR_CHARGE_TIME + ROBOCOP_MORTAR_ATTACK_DELAY) && !m_fMortarAttackEvent)
					{
						MortarAttack( m_vecMortarPos );
						m_fMortarAttackEvent = TRUE;
					}
				}
			}
		}
		else
		{
			m_IdealActivity = ACT_IDLE;

			StopMortarAttack();

			m_flNextMortarAttack = gpGlobals->time + RANDOM_FLOAT(2.0f, 4.0f);

			TaskComplete();
		}
		break;

	case TASK_DIE:
		if (m_flWaitFinished > gpGlobals->time)
		{
			if( pev->frame >= 255.0f )
			{
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/null.wav", 1.0, ATTN_NORM, 0, 100);
				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
					WRITE_BYTE( TE_BREAKMODEL );

					// position
					WRITE_COORD( pev->origin.x );
					WRITE_COORD( pev->origin.y );
					WRITE_COORD( pev->origin.z );

					// size
					WRITE_COORD( 200 );
					WRITE_COORD( 200 );
					WRITE_COORD( 128 );

					// velocity
					WRITE_COORD( 0 );
					WRITE_COORD( 0 );
					WRITE_COORD( 0 );

					// randomization
					WRITE_BYTE( 200 );

					// Model
					WRITE_SHORT( m_iRobocopGibModel );   //model id#

					// # of shards
					WRITE_BYTE( 20 );

					// duration
					WRITE_BYTE( 20 );// 3.0 seconds

					// flags
					WRITE_BYTE( BREAK_FLESH );
				MESSAGE_END();

				SpawnExplosion( pev->origin, 70, 0, 150 );

				int trailCount = RANDOM_LONG( 2, 4 );
				for( int i = 0; i < trailCount; i++ )
					Create( "fire_trail", pev->origin, Vector( 0, 0, 1 ), NULL );

				SetBodygroup( 0, 1 );
				return;
			}
		}
		if (gpGlobals->time > m_flNextSparkTime)
		{
			Create( "spark_shower", pev->origin, Vector( 0, 0, 1 ), NULL );
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "ambience/sparks.wav", 1.0, ATTN_NORM, 0, 100);
			m_flNextSparkTime = gpGlobals->time + 0.3f;
			return;
		}
	default:
		CBaseMonster::RunTask(pTask);
		break;
	}
}


//=========================================================
// SonicAttack
//=========================================================
void CRoboCop::SonicAttack(void)
{
	float		flAdjustedDamage;
	float		flDist;
	Vector		vecDist;
	int		i, r, g, b;

	UTIL_MakeVectors( pev->angles );
	Vector vecSrc = pev->origin + 12 * gpGlobals->v_right + 95 * gpGlobals->v_forward;

	for( i = 0; i < 3; i++ )
	{
		switch( i )
		{
		case 0:
			r = 101, g = 133, b = 221;
			break;
		case 1:
			r = 67, g = 85, b = 255;
			break;
		case 2:
			r = 62, g = 33, b = 211;
			break;
		}
 
		// blast circles
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_COORD( vecSrc.x );
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z + 16 );
			WRITE_COORD( vecSrc.x );
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z + gSkillData.robocopSWRadius / ( ( i + 1 ) * .2 ) ); // reach damage radius over .3 seconds
			WRITE_SHORT( m_iSpriteTexture );
			WRITE_BYTE( 0 ); // startframe
			WRITE_BYTE( 10 ); // framerate
			WRITE_BYTE( i + 2 ); // life
			WRITE_BYTE( 32 );  // width
			WRITE_BYTE( 0 );   // noise
			WRITE_BYTE( r );  // r
			WRITE_BYTE( g );  // g
			WRITE_BYTE( b ); // b
			WRITE_BYTE( 255 ); //brightness
			WRITE_BYTE( 0 );          // speed
		MESSAGE_END();
	}

	// Shake the screen.
	UTIL_ScreenShake(pev->origin, 12.0, 100.0, 2.0, 1000);

	CBaseEntity *pEntity = NULL;
	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, gSkillData.robocopSWRadius)) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// Robocop does not take damage from it's own attacks.
			if (pEntity != this)
			{
				// houndeyes do FULL damage if the ent in question is visible. Half damage otherwise.
				// This means that you must get out of the houndeye's attack range entirely to avoid damage.
				// Calculate full damage first

				vecDist = pEntity->Center() - vecSrc;
				flDist = Q_max( 0, gSkillData.robocopSWRadius - vecDist.Length() );

				flDist = flDist / gSkillData.robocopSWRadius;

				if (!FVisible(pEntity))
				{
					if (pEntity->IsPlayer())
					{
						// if this entity is a client, and is not in full view, inflict half damage. We do this so that players still 
						// take the residual damage if they don't totally leave the houndeye's effective radius. We restrict it to clients
						// so that monsters in other parts of the level don't take the damage and get pissed.
						flDist *= 0.5;
					}
					else if (!FClassnameIs(pEntity->pev, "func_breakable") && !FClassnameIs(pEntity->pev, "func_pushable"))
					{
						// do not hurt nonclients through walls, but allow damage to be done to breakables
						flDist = 0;
					}
				}

				flAdjustedDamage = gSkillData.robocopDmgFist * flDist;
				//ALERT ( at_aiconsole, "Damage: %f\n", flAdjustedDamage );

				if (flAdjustedDamage > 0)
				{
					pEntity->TakeDamage(pev, pev, flAdjustedDamage, DMG_SONIC);
				}
				if( pEntity->IsPlayer() )
				{
					vecDist = vecDist.Normalize();
					vecDist.x = vecDist.x * flDist * 600.0f;
					vecDist.y = vecDist.y * flDist * 600.0f;
					vecDist.z = flDist * 450.0f;
					pEntity->pev->velocity = vecDist + pEntity->pev->velocity;
					pEntity->pev->punchangle.x = 5;
				}
			}
		}
	}
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "robocop/rc_fist.wav", 1.0, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG( -10, 10 ) );
}


//=========================================================
// SonicAttack
//=========================================================
void CRoboCop::MortarAttack(Vector vecSrc)
{
	CBaseEntity *pMortar = Create( "monster_mortar", vecSrc, g_vecZero, 0 );
	pMortar->pev->nextthink = gpGlobals->time + 0.1;
	pMortar->pev->dmg = gSkillData.robocopDmgMortar;
}

void CRoboCop::StartMortarAttack(void)
{
	m_bInMortarAttack = TRUE;

	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "robocop/rc_charge.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

	m_flMortarAttackStart = gpGlobals->time;
}

void CRoboCop::StopMortarAttack(void)
{
	BeamOff();

	STOP_SOUND( ENT(pev), CHAN_BODY, "robocop/rc_charge.wav" );

	m_bInMortarAttack = FALSE;
	m_bAimLocked = FALSE;
	m_fMortarAttackEvent = FALSE;

	m_flMortarAttackStart = 0;
}

BOOL CRoboCop::PredictMeleeAttack(CBaseEntity* pEnemy)
{
	ASSERT(pEnemy != NULL);

	Vector vecSrc, vecVelocity;
	Vector vecPredictedPos;

	vecSrc = pEnemy->pev->origin;
	vecVelocity = pEnemy->pev->velocity;

	float speed = vecVelocity.Length();
	float distance = speed * ROBOCOP_SHOCKWAVE_IMPACT_DELAY;

	vecPredictedPos = vecSrc + vecVelocity.Normalize() * distance;

	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecSrc + vecVelocity.Normalize() * distance, dont_ignore_monsters, ENT(pev), &tr);

	if ((tr.vecEndPos - pev->origin).Length() < ROBOCOP_MELEE_ATTACK_DIST)
		return TRUE;

	return FALSE;
}

void CRoboCop::PredictEnemyPosition(CBaseEntity* pEnemy, Vector& vecResult)
{
	Vector vecSrc, vecVelocity;
	Vector vecPredictedPos;

	vecSrc = pEnemy->pev->origin;
	vecVelocity = pEnemy->pev->velocity;

	float speed = vecVelocity.Length();
	float distance = speed * ROBOCOP_MORTAR_ATTACK_DELAY;

	vecPredictedPos = vecSrc + vecVelocity.Normalize() * distance;

	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecPredictedPos, dont_ignore_monsters, ENT(pev), &tr);

	vecResult = tr.vecEndPos;
}

BOOL CRoboCop::IsEnemyReachable(CBaseEntity* pEnemy)
{
	ASSERT( pEnemy != NULL );

	Vector vecGroundEnemyPos = pEnemy->pev->origin;
	vecGroundEnemyPos.z = pev->origin.z;

	if (BuildRoute(vecGroundEnemyPos, bits_MF_TO_ENEMY, pEnemy))
	{
		return TRUE;
	}

	return FALSE;
}


void CRoboCop::CreateEyeGlow(void)
{
	m_pEyeGlow = CSprite::SpriteCreate(ROBOCOP_EYE_SPRITE_NAME, pev->origin, FALSE);
	m_pEyeGlow->SetTransparency(kRenderTransAdd, 255, 255, 255, 0, kRenderFxNoDissipation);
	m_pEyeGlow->SetAttachment(edict(), 1);
	m_pEyeGlow->SetScale(0.5f);
}

void CRoboCop::DestroyEyeGlow(void)
{
	UTIL_Remove(m_pEyeGlow);
	m_pEyeGlow = NULL;
}

void CRoboCop::CreateBeam(void)
{
	m_pBeam = CBeam::BeamCreate(ROBOCOP_EYE_BEAM_NAME, 30);
	m_pBeam->PointEntInit(pev->origin, entindex());
	m_pBeam->SetEndAttachment(1);
	m_pBeam->SetBrightness(0);
	m_pBeam->SetColor(255, 0, 0);
}

void CRoboCop::DestroyBeam(void)
{
	UTIL_Remove(m_pBeam);
	m_pBeam = NULL;
}

void CRoboCop::CreateSpot(void)
{
	m_pBeamSpot = CSprite::SpriteCreate(ROBOCOP_EYE_SPOT_NAME, pev->origin, FALSE);
	m_pBeamSpot->SetTransparency(kRenderTransAdd, 255, 255, 255, 0, kRenderFxNoDissipation);
	m_pBeamSpot->SetScale(0.30f);
}

void CRoboCop::DestroySpot(void)
{
	UTIL_Remove(m_pBeamSpot);
	m_pBeamSpot = NULL;
}

void CRoboCop::CreateEffects(void)
{
	if (!m_pEyeGlow)
	{
		CreateEyeGlow();
	}

	if (!m_pBeam)
	{
		CreateBeam();
	}

	if (!m_pBeamSpot)
	{
		CreateSpot();
	}
}

void CRoboCop::DestroyEffects(void)
{
	if (m_pEyeGlow)
	{
		DestroyEyeGlow();
	}

	if (m_pBeam)
	{
		DestroyBeam();
	}

	if (m_pBeamSpot)
	{
		DestroySpot();
	}
}

void CRoboCop::EyeOn(int level)
{
	m_eyeBrightness = level;
}


void CRoboCop::EyeOff(void)
{
	m_eyeBrightness = 0;
}


void CRoboCop::EyeUpdate(void)
{
	if (!m_pEyeGlow)
	{
		CreateEyeGlow();
	}

	if (m_pEyeGlow)
	{
		m_pEyeGlow->pev->renderamt = UTIL_Approach(m_eyeBrightness, m_pEyeGlow->pev->renderamt, 30);
		if (m_pEyeGlow->pev->renderamt == 0)
			m_pEyeGlow->pev->effects |= EF_NODRAW;
		else
			m_pEyeGlow->pev->effects &= ~EF_NODRAW;
		UTIL_SetOrigin(m_pEyeGlow->pev, pev->origin);
	}
}


void CRoboCop::BeamOn(int level)
{
	m_beamBrightness = level;
}


void CRoboCop::BeamOff(void)
{
	m_beamBrightness = 0;
}


void CRoboCop::BeamUpdate(void)
{
	if (!m_pBeam)
	{
		CreateBeam();
	}

	if (!m_pBeamSpot)
	{
		CreateSpot();
	}

	if (m_pBeam)
	{
		m_pBeam->pev->renderamt = UTIL_Approach(m_beamBrightness, m_pBeam->pev->renderamt, 60);
		if (m_pBeam->pev->renderamt == 0)
			m_pBeam->pev->effects |= EF_NODRAW;
		else
			m_pBeam->pev->effects &= ~EF_NODRAW;
	}

	if (m_pBeamSpot)
	{
		m_pBeamSpot->pev->renderamt = UTIL_Approach(Q_min(m_beamBrightness + 25, 255), m_pBeamSpot->pev->renderamt, 60);
		if (m_pBeamSpot->pev->renderamt == 0)
			m_pBeamSpot->pev->effects |= EF_NODRAW;
		else
			m_pBeamSpot->pev->effects &= ~EF_NODRAW;
	}
}


void CreateRoboCopNodes(CRoboCop* pOwner)
{
	BEGIN_RC_SECTOR(0, pOwner)
		ADD_RC_NODE(-256, 2272, 256);
		ADD_RC_NODE(-768, 2272, 256);
		ADD_RC_NODE(-416, 1856, 256);
		ADD_RC_NODE(-256, 1728, 256);
		ADD_RC_NODE(-380, 1472, 256);
	END_RC_SECTOR()

	BEGIN_RC_SECTOR(1, pOwner)
		ADD_RC_NODE(-1792, 2272, 256);
		ADD_RC_NODE(-1280, 2272, 256);
		ADD_RC_NODE(-1536, 2016, 256);
		ADD_RC_NODE(-1104, 1856, 256);
		ADD_RC_NODE(-1792, 1728, 256);
		ADD_RC_NODE(-1280, 1728, 256);
		ADD_RC_NODE(-1536, 1472, 256);
		ADD_RC_NODE(-1232, 1472, 256);
	END_RC_SECTOR()

	BEGIN_RC_SECTOR(2, pOwner)
		ADD_RC_NODE(-1792, 672, 256);
		ADD_RC_NODE(-1280, 672, 256);
		ADD_RC_NODE(-1536, 928, 256);
		ADD_RC_NODE(-1024, 896, 256);
		ADD_RC_NODE(-1792, 1184, 256);
		ADD_RC_NODE(-1424, 1200, 256);
		ADD_RC_NODE(-1120, 1120, 256);
	END_RC_SECTOR()

	BEGIN_RC_SECTOR(3, pOwner)
		ADD_RC_NODE(-256, 672, 256);
		ADD_RC_NODE(-768, 672, 256);
		ADD_RC_NODE(-512, 896, 256);
		ADD_RC_NODE(-416, 1120, 256);
		ADD_RC_NODE(-256, 1184, 256);
	END_RC_SECTOR()
}
