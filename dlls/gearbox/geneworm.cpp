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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"player.h"
#include	"weapons.h"

#define GENEWORM_LEVEL0					0
#define GENEWORM_LEVEL1					1

#define GENEWORM_LEVEL0_HEIGHT			244
#define GENEWORM_LEVEL1_HEIGHT			304


#define GENEWORM_SKIN_EYE_OPEN			0
#define GENEWORM_SKIN_EYE_LEFT			1
#define GENEWORM_SKIN_EYE_RIGHT			2
#define GENEWORM_SKIN_EYE_CLOSED		3


//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define GENEWORM_AE_BEAM			( 0 )		// Toxic beam attack (sprite trail)
#define GENEWORM_AE_PORTAL			( 2 )		// Create a portal that spawns an enemy.

#define GENEWORM_AE_MELEE_LEFT1		( 3 )		// Play hit sound
#define GENEWORM_AE_MELEE_LEFT2		( 4 )		// Activates the trigger_geneworm_hit

#define GENEWORM_AE_MELEE_RIGHT1	( 5 )		// Play hit sound
#define GENEWORM_AE_MELEE_RIGHT2	( 6 )		// Activates the trigger_geneworm_hit

#define GENEWORM_AE_MELEE_FORWARD1  ( 7 )		// Play hit sound
#define GENEWORM_AE_MELEE_FORWARD2  ( 8 )		// Activates the trigger_geneworm_hit

#define GENEWORM_AE_MAD				( 9 )		// Room starts shaking!

#define GENEWORM_AE_EYEPAIN			( 1012 )	// Still put here (In case we need to toggle eye pain status)

//=========================================================
// CGeneWorm
//=========================================================
class CGeneWorm : public CBaseMonster
{
public:

	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn(void);
	void Precache(void);
	int  Classify(void) { return CLASS_ALIEN_MONSTER; };
	int  BloodColor(void) { return BLOOD_COLOR_YELLOW; }
	void Killed(entvars_t *pevAttacker, int iGib);

	void SetObjectCollisionBox(void)
	{
		pev->absmin = pev->origin + Vector(-95, -95, 0);
		pev->absmax = pev->origin + Vector(95, 95, 0);
	}

	void HandleAnimEvent(MonsterEvent_t *pEvent);

	void EXPORT StartupThink(void);
	void EXPORT HuntThink(void);
	void EXPORT CrashTouch(CBaseEntity *pOther);
	void EXPORT DyingThink(void);
	void EXPORT StartupUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT NullThink(void);
	void EXPORT CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void FloatSequence(void);
	void NextActivity(void);

	void Flight(void);

	int  TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void PainSound(void);
	void DeathSound(void);
	void IdleSound(void);

	int		Level( float dz );
	int		MyEnemyLevel(void);
	float	MyEnemyHeight(void);

	Vector m_vecTarget;
	Vector m_posTarget;

	Vector m_vecDesired;
	Vector m_posDesired;

	float m_flLastSeen;
	float m_flPrevSeen;

	int m_iLevel;
	EHANDLE m_hHitVolumes[3];

	void PlaySequenceAttack( int side, BOOL bMelee );
	void PlaySequencePain( int side );

	//
	// SOUNDS
	//

	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pEntrySounds[];
	static const char *pPainSounds[];
	static const char *pIdleSounds[];
	static const char *pEyePainSounds[];


	//
	// ANIMATIONS
	//

	static const char *pDeathAnims[];
	static const char *pEntryAnims[];
	static const char *pIdleAnims[];

	static const char *pMadAnims[];
	static const char *pScreamAnims[];

	static const char *pBigPainAnims[];
	static const char *pEyePainAnims[];
	static const char *pIdlePainAnims[];

	static const char *pMeleeAttackAnims[];
	static const char *pBeamAttackAnims[];

	//
	// HIT VOLUMES
	//
	static const char* pHitVolumes[];
};

LINK_ENTITY_TO_CLASS(monster_geneworm, CGeneWorm);

TYPEDESCRIPTION CGeneWorm::m_SaveData[] =
{
	DEFINE_FIELD(CGeneWorm, m_vecTarget, FIELD_VECTOR),
	DEFINE_FIELD(CGeneWorm, m_posTarget, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CGeneWorm, m_vecDesired, FIELD_VECTOR),
	DEFINE_FIELD(CGeneWorm, m_posDesired, FIELD_POSITION_VECTOR),

	DEFINE_FIELD(CGeneWorm, m_flLastSeen, FIELD_TIME),
	DEFINE_FIELD(CGeneWorm, m_flPrevSeen, FIELD_TIME),

	DEFINE_FIELD(CGeneWorm, m_iLevel, FIELD_INTEGER),
	DEFINE_ARRAY(CGeneWorm, m_hHitVolumes, FIELD_EHANDLE, 3),
};
IMPLEMENT_SAVERESTORE(CGeneWorm, CBaseMonster);


//=========================================================
//=========================================================

const char *CGeneWorm::pAttackSounds[] =
{
	"geneworm/geneworm_attack_mounted_gun.wav",
	"geneworm/geneworm_attack_mounted_rocket.wav",
	"geneworm/geneworm_beam_attack.wav",
	"geneworm/geneworm_big_attack_forward.wav",
};

const char *CGeneWorm::pDeathSounds[] =
{
	"geneworm/geneworm_death.wav",
};

const char *CGeneWorm::pEntrySounds[] =
{
	"geneworm/geneworm_entry.wav",
};

const char *CGeneWorm::pPainSounds[] =
{
	"geneworm/geneworm_final_pain1.wav",
	"geneworm/geneworm_final_pain2.wav",
	"geneworm/geneworm_final_pain3.wav",
	"geneworm/geneworm_final_pain4.wav",
};

const char *CGeneWorm::pIdleSounds[] =
{
	"geneworm/geneworm_idle1.wav",
	"geneworm/geneworm_idle2.wav",
	"geneworm/geneworm_idle3.wav",
	"geneworm/geneworm_idle4.wav",
};

const char *CGeneWorm::pEyePainSounds[] =
{
	"geneworm/geneworm_shot_in_eye.wav",
};

const char* CGeneWorm::pHitVolumes[]
{
	"GeneWormRightSlash",
	"GeneWormCenterSlash",
	"GeneWormLeftSlash",
};


//=========================================================
//=========================================================

const char *CGeneWorm::pDeathAnims[] =
{
	"death",
};

const char *CGeneWorm::pEntryAnims[] =
{
	"entry",
}; 

const char *CGeneWorm::pIdleAnims[] =
{
	"idle",
};

const char *CGeneWorm::pMadAnims[] =
{
	"mad",
};

const char *CGeneWorm::pScreamAnims[] =
{
	"scream1",
	"scream2",
};

const char *CGeneWorm::pBigPainAnims[] =
{
	"bigpain1",
	"bigpain2",
	"bigpain3",
	"bigpain4",
};

const char *CGeneWorm::pEyePainAnims[] =
{
	"eyepain1",
	"eyepain2",
};

const char *CGeneWorm::pIdlePainAnims[] =
{

	"idlepain",
	"idlepain2",
	"idlepain3",
};

const char *CGeneWorm::pMeleeAttackAnims[] =
{
	"melee1",
	"melee2",
	"melee3",
};

const char *CGeneWorm::pBeamAttackAnims[] =
{
	"dattack1",
	"dattack2",
	"dattack3",
};

//=========================================================
// Spawn
//=========================================================
void CGeneWorm::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;


	SET_MODEL(ENT(pev), "models/geneworm.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));
	UTIL_SetOrigin(pev, pev->origin);

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->health = 999;
	pev->view_ofs = Vector(0, 0, 192);

	pev->sequence = 0;
	ResetSequenceInfo();

	InitBoneControllers();

	SetThink(&CGeneWorm::StartupThink);
	pev->nextthink = gpGlobals->time + 0.1;

	m_vecDesired = Vector(1, 0, 0);
	m_posDesired = Vector(pev->origin.x, pev->origin.y, 512);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGeneWorm::Precache()
{
	PRECACHE_MODEL("models/geneworm.mdl");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pEntrySounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pEyePainSounds);
}

//=========================================================
// NullThink
//=========================================================
void CGeneWorm::NullThink(void)
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.5;
}

//=========================================================
// StartupUse
//=========================================================
void CGeneWorm::StartupUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SetThink(&CGeneWorm::HuntThink);
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse(&CGeneWorm::CommandUse);
}

//=========================================================
//
//=========================================================
void CGeneWorm::StartupThink(void)
{
	CBaseEntity *pEntity = NULL;

	int i;
	for (i = 0; i < 3; i++)
	{
		pEntity = UTIL_FindEntityByTargetname(NULL, pHitVolumes[i]);
		if (pEntity)
			m_hHitVolumes[i] = pEntity;
	}

	SetThink(&CGeneWorm::HuntThink);
	SetUse(&CGeneWorm::CommandUse);
	pev->nextthink = gpGlobals->time + 0.1;
}


void CGeneWorm::Killed(entvars_t *pevAttacker, int iGib)
{
	CBaseMonster::Killed(pevAttacker, iGib);
}

void CGeneWorm::DyingThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
	DispatchAnimEvents();
	StudioFrameAdvance();

	if (pev->deadflag == DEAD_NO)
	{
		FireTargets(STRING(pev->target), this, this, USE_ON, 1.0);
		pev->velocity = Vector(0, 0, 0);

		DeathSound();
		pev->deadflag = DEAD_DYING;
	}

	if (pev->deadflag == DEAD_DYING)
	{
		Flight();
		pev->deadflag = DEAD_DEAD;
	}

	if (m_fSequenceFinished)
	{
		pev->sequence = LookupSequence("death");
		pev->framerate = 0.5f;
	}

	if (m_hHitVolumes)
	{
		int i;
		for (i = 0; i < 3; i++)
		{
			UTIL_Remove(m_hHitVolumes[i]);
			m_hHitVolumes[i] = NULL;
		}
	}

	return;
}

//=========================================================
//=========================================================
void CGeneWorm::FloatSequence(void)
{
	pev->sequence = LookupSequence("idle");
}

//=========================================================
//=========================================================
void CGeneWorm::NextActivity(void)
{
	UTIL_MakeAimVectors(pev->angles);

	float flDist = (m_posDesired - pev->origin).Length();
	float flDot = DotProduct(m_vecDesired, gpGlobals->v_forward);

	if (m_hEnemy != NULL && !m_hEnemy->IsAlive())
	{
		m_hEnemy = NULL;
	}

	if (m_flLastSeen + 15 < gpGlobals->time)
	{
		m_hEnemy = NULL;
	}

	if (m_hEnemy == NULL)
	{
		Look(4096);
		m_hEnemy = BestVisibleEnemy();

		if (!m_hEnemy)
		{
			m_hEnemy = UTIL_PlayerByIndex( 1 );
			ASSERT( m_hEnemy );
		}
	}

	if (m_hEnemy != NULL)
	{
		if (m_flLastSeen + 5 > gpGlobals->time)
		{
			return;
		}
	}


	if (m_iLevel != -1)
	{
		ASSERT( m_hEnemy );
	}

	FloatSequence();
}

void CGeneWorm::HuntThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
	DispatchAnimEvents();
	StudioFrameAdvance();

	// if dead, force cancelation of current animation
	if (pev->health <= 0)
	{
		SetThink(&CGeneWorm::DyingThink);
		m_fSequenceFinished = TRUE;
		return;
	}

	// get new sequence
	if (m_fSequenceFinished)
	{
		pev->frame = 0;
		NextActivity();
		ResetSequenceInfo();
	}

	// look for current enemy	
	if (m_hEnemy != NULL)
	{
		// Update level.
		m_iLevel = Level( m_hEnemy->pev->origin.z );

		if (FVisible(m_hEnemy))
		{
			if (m_flLastSeen < gpGlobals->time - 5)
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->pev->origin;
			m_vecTarget = (m_posTarget - pev->origin).Normalize();
			m_vecDesired = m_vecTarget;
			m_posDesired = Vector(pev->origin.x, pev->origin.y, pev->origin.z);
		}
		else
		{
			//m_flAdj = min(m_flAdj + 10, 1000);
		}
	}
	else
	{
		m_iLevel = -1;
	}

#if 0
	// don't go too high
	if (m_posDesired.z > m_flMaxZ)
		m_posDesired.z = m_flMaxZ;

	// don't go too low
	if (m_posDesired.z < m_flMinZ)
		m_posDesired.z = m_flMinZ;
#endif

	Flight();
}



//=========================================================
//=========================================================
void CGeneWorm::Flight(void)
{

}

void CGeneWorm::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case 1:	// shoot 
		break;
	default:
		break;
	}
}

void CGeneWorm::CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	switch (useType)
	{
	case USE_OFF:
		break;
	case USE_ON:
		break;
	case USE_SET:
		break;
	case USE_TOGGLE:
		break;
	}
}

int CGeneWorm::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (pevInflictor->owner == edict())
		return 0;

	if (flDamage >= pev->health)
	{
		pev->health = 1;
	}

	PainSound();

	pev->health -= flDamage;
	return 0;
}

void CGeneWorm::PainSound(void)
{

}

void CGeneWorm::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 2, ATTN_NORM);
}

void CGeneWorm::IdleSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), VOL_NORM, ATTN_NORM);
}

int CGeneWorm::Level(float dz)
{
	if (dz < GENEWORM_LEVEL1_HEIGHT)
		return GENEWORM_LEVEL0;

	return GENEWORM_LEVEL1;
}
int CGeneWorm::MyEnemyLevel(void)
{
	if (!m_hEnemy)
		return -1;

	return Level(m_hEnemy->pev->origin.z);
}

float CGeneWorm::MyEnemyHeight(void)
{
	switch (m_iLevel)
	{
	case GENEWORM_LEVEL0:
		return GENEWORM_LEVEL0_HEIGHT;
	case GENEWORM_LEVEL1:
		return GENEWORM_LEVEL1_HEIGHT;
	}

	return GENEWORM_LEVEL1_HEIGHT;
}