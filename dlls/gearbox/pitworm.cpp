/***
*
*   SPIRIT OF HALF-LIFE 1.9: OPPOSING-FORCE EDITION
*
*   Spirit of Half-Life and their logos are the property of their respective owners.
*   Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*   This product contains software technology licensed from Id
*   Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
*   All Rights Reserved.
*
*	Base Source-Code written by Marc-Antoine Lortie (https://github.com/malortie).
*   Modifications by Hammermaps.de DEV Team (support@hammermaps.de).
*
***/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"nodes.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"
#include	"weapons.h"
#include	"player.h"
#include	"decals.h"

#define PITWORM_ATTN 0.1
#define NUM_PITWORM_LEVELS		4

class CPitWorm : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	int  Classify(void) { return CLASS_ALIEN_MONSTER; }
	virtual int	ObjectCaps(void) { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void SetObjectCollisionBox()
	{
		pev->absmin = pev->origin + Vector( -400, -400, 0 );
		pev->absmax = pev->origin + Vector( 400, 400, 850 );
	}
	BOOL FVisible(CBaseEntity* pEntity);
	BOOL FVisible(const Vector& vecOrigin);

	void IdleSound(void);
	void AlertSound(void);
	void DeathSound(void);
	void PainSound(void);

	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void AngrySound(void);
	void SwipeSound(void);
	void BeamSound(void);

	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void EXPORT StartupThink(void);
	void EXPORT DyingThink(void);
	void EXPORT StartupUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT NullThink(void);
	void EXPORT CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT	HuntThink(void);
	void EXPORT HitTouch(CBaseEntity* pOther);

	void LockTopLevel();
	BOOL ClawAttack();
	void ShootBeam();
	void StrafeBeam();
	void ChangeLevel();
	void TrackEnemy();

	void NextActivity(void);

	void EyeLight(const Vector& vecEyePos);
	void BeamEffect(TraceResult& tr);

	int SizeForGrapple() { return GRAPPLE_LARGE; }

	//
	float m_flNextPainSound;

	Vector m_vecTarget;
	Vector m_posTarget;
	Vector m_vecDesired;
	Vector m_posDesired;

	float m_offsetBeam;
	Vector m_posBeam;
	Vector m_vecBeam;
	Vector m_angleBeam;
	float m_flBeamExpireTime;
	float m_flBeamDir;

	float m_flTorsoYaw;
	float m_flHeadYaw;
	float m_flHeadPitch;
	float m_flIdealTorsoYaw;
	float m_flIdealHeadYaw;
	float m_flIdealHeadPitch;

	float m_flLevels[NUM_PITWORM_LEVELS];
	float m_flTargetLevels[NUM_PITWORM_LEVELS];

	float m_flLastSeen;

	int m_iLevel;
	float m_flLevelSpeed;

	CBeam* m_pBeam;
	CSprite* m_pSprite;

	BOOL m_fAttacking;
	BOOL m_fLockHeight;
	BOOL m_fLockYaw;

	int m_iWasHit;
	float m_flTakeHitTime;

	float m_flHitTime;
	float m_flNextMeleeTime;
	float m_flNextRangeTime;
	float m_flDeathStartTime;

	BOOL m_fFirstSighting;
	BOOL m_fTopLevelLocked;

	float m_flLastBlinkTime;
	float m_flLastBlinkInterval;
	float m_flLastEventTime;

	static const char* pHitGroundSounds[];
	static const char* pAngrySounds[];
	static const char* pSwipeSounds[];
	static const char* pShootSounds[];

	static const char* pPainSounds[];
	static const char* pAlertSounds[];
	static const char* pIdleSounds[];
	static const char* pDeathSounds[];
	static const char* pAttackSounds[];
};

LINK_ENTITY_TO_CLASS(monster_pitworm, CPitWorm)
LINK_ENTITY_TO_CLASS(monster_pitworm_up, CPitWorm)

#define PITWORM_EYE_OFFSET				Vector(0, 0, 300)

#define PITWORM_CONTROLLER_EYE_YAW		0
#define PITWORM_CONTROLLER_EYE_PITCH	1
#define PITWORM_CONTROLLER_BODY_YAW		2

#define PITWORM_EYE_PITCH_MIN	-45
#define PITWORM_EYE_PITCH_MAX	 45
#define PITWORM_EYE_YAW_MIN		-45
#define PITWORM_EYE_YAW_MAX		 45

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define PITWORM_AE_SWIPE			( 1 )
#define PITWORM_AE_EYEBLAST_START	( 2 )
#define PITWORM_AE_EYEBLAST_END		( 4 )

//=========================================================
// Save & Restore
//=========================================================
TYPEDESCRIPTION	CPitWorm::m_SaveData[] =
{
	DEFINE_FIELD(CPitWorm, m_flNextPainSound, FIELD_TIME),

	DEFINE_FIELD(CPitWorm, m_vecTarget, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_posTarget, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_vecDesired, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_posDesired, FIELD_POSITION_VECTOR),

	DEFINE_FIELD(CPitWorm, m_offsetBeam, FIELD_FLOAT),
	DEFINE_FIELD(CPitWorm, m_posBeam, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_vecBeam, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_angleBeam, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPitWorm, m_flBeamExpireTime, FIELD_TIME),
	DEFINE_FIELD(CPitWorm, m_flBeamDir, FIELD_FLOAT),

	DEFINE_ARRAY(CPitWorm, m_flLevels, FIELD_FLOAT, NUM_PITWORM_LEVELS),
	DEFINE_ARRAY(CPitWorm, m_flTargetLevels, FIELD_FLOAT, NUM_PITWORM_LEVELS),

	DEFINE_FIELD(CPitWorm, m_flLastSeen, FIELD_TIME),

	DEFINE_FIELD(CPitWorm, m_iLevel, FIELD_INTEGER),
	DEFINE_FIELD(CPitWorm, m_flLevelSpeed, FIELD_FLOAT),

	DEFINE_FIELD(CPitWorm, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CPitWorm, m_pSprite, FIELD_CLASSPTR),

	DEFINE_FIELD(CPitWorm, m_fAttacking, FIELD_BOOLEAN),
	DEFINE_FIELD(CPitWorm, m_fLockHeight, FIELD_BOOLEAN),
	DEFINE_FIELD(CPitWorm, m_fLockYaw, FIELD_BOOLEAN),

	DEFINE_FIELD(CPitWorm, m_iWasHit, FIELD_INTEGER),
	DEFINE_FIELD(CPitWorm, m_flTakeHitTime, FIELD_TIME),

	DEFINE_FIELD(CPitWorm, m_flHitTime, FIELD_TIME),
	DEFINE_FIELD(CPitWorm, m_flNextMeleeTime, FIELD_TIME),
	DEFINE_FIELD(CPitWorm, m_flNextRangeTime, FIELD_TIME),
	DEFINE_FIELD(CPitWorm, m_flDeathStartTime, FIELD_TIME),

	DEFINE_FIELD(CPitWorm, m_fFirstSighting, FIELD_BOOLEAN),
	DEFINE_FIELD(CPitWorm, m_fTopLevelLocked, FIELD_BOOLEAN),

	DEFINE_FIELD(CPitWorm, m_flLastBlinkTime, FIELD_TIME),
	DEFINE_FIELD(CPitWorm, m_flLastBlinkInterval, FIELD_FLOAT),
	DEFINE_FIELD(CPitWorm, m_flLastEventTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CPitWorm, CBaseMonster)

const char* CPitWorm::pHitGroundSounds[] =
{
	"tentacle/te_strike1.wav",
	"tentacle/te_strike2.wav",
};

const char* CPitWorm::pAngrySounds[] =
{
	"pitworm/pit_worm_angry1.wav",
	"pitworm/pit_worm_angry2.wav",
	"pitworm/pit_worm_angry3.wav",
};


const char* CPitWorm::pSwipeSounds[] =
{
	"pitworm/pit_worm_attack_swipe1.wav",
	"pitworm/pit_worm_attack_swipe2.wav",
	"pitworm/pit_worm_attack_swipe3.wav",
};

const char* CPitWorm::pShootSounds[] =
{
	"debris/beamstart3.wav",
	"debris/beamstart8.wav",
};

const char* CPitWorm::pPainSounds[] =
{
	"pitworm/pit_worm_flinch1.wav",
	"pitworm/pit_worm_flinch2.wav",
};

const char* CPitWorm::pAlertSounds[] =
{
	"pitworm/pit_worm_alert.wav",
};

const char* CPitWorm::pIdleSounds[] =
{
	"pitworm/pit_worm_idle1.wav",
	"pitworm/pit_worm_idle2.wav",
	"pitworm/pit_worm_idle3.wav",
};

const char* CPitWorm::pDeathSounds[] =
{
	"pitworm/pit_worm_death.wav",
};

const char *CPitWorm::pAttackSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav"
};

//=========================================================
// Spawn
//=========================================================
void CPitWorm::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/pit_worm_up.mdl");

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));
	UTIL_SetOrigin(pev, pev->origin);

	pev->flags |= FL_MONSTER|FL_FLY;
	pev->takedamage = DAMAGE_AIM;

	pev->health = gSkillData.pwormHealth;
	pev->max_health = pev->health;

	pev->view_ofs = PITWORM_EYE_OFFSET;
	m_bloodColor = BLOOD_COLOR_GREEN;
	m_flFieldOfView = 0.5;

	pev->sequence = 0;
	ResetSequenceInfo();

	m_flTorsoYaw = 0;
	m_flHeadYaw = 0;
	m_flHeadPitch = 0;
	m_flIdealTorsoYaw = 0;
	m_flIdealHeadYaw = 0;
	m_flIdealHeadPitch = 0;

	InitBoneControllers();

	SetThink(&CPitWorm::StartupThink);
	SetTouch(&CPitWorm::HitTouch);
	pev->nextthink = gpGlobals->time + 0.1;

	m_vecDesired = Vector(1,0,0);
	m_posDesired = pev->origin;

	m_fAttacking = FALSE;
	m_fLockHeight = FALSE;
	m_fFirstSighting = FALSE;
	m_flBeamExpireTime = gpGlobals->time;
	m_iLevel = 0;
	m_fLockYaw = FALSE;

	m_iWasHit = 0;
	m_flTakeHitTime = 0;
	m_flHitTime = 0;
	m_flLevelSpeed = 10;

	m_fTopLevelLocked = FALSE;
	m_flLastBlinkTime = gpGlobals->time;
	m_flLastBlinkInterval = gpGlobals->time;
	m_flLastEventTime = gpGlobals->time;
	m_flTargetLevels[3] = pev->origin.z;
	m_flLevels[3] = pev->origin.z - 350.0;
	m_flTargetLevels[2] = pev->origin.z;
	m_flLevels[2] = pev->origin.z - 300.0;
	m_flTargetLevels[1] = pev->origin.z;
	m_flLevels[1] = pev->origin.z - 300.0;
	m_flTargetLevels[0] = pev->origin.z;
	m_flLevels[0] = pev->origin.z - 300.0;;
	m_pBeam = 0;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPitWorm::Precache()
{
	PRECACHE_MODEL("models/pit_worm_up.mdl");

	PRECACHE_SOUND("pitworm/pit_worm_alert.wav");
	PRECACHE_SOUND("pitworm/pit_worm_attack_eyeblast.wav");
	PRECACHE_SOUND("pitworm/pit_worm_attack_eyeblast_impact.wav");
	PRECACHE_SOUND("pitworm/pit_worm_death.wav");

	PRECACHE_SOUND_ARRAY(pHitGroundSounds);
	PRECACHE_SOUND_ARRAY(pAngrySounds);
	PRECACHE_SOUND_ARRAY(pSwipeSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);

	PRECACHE_MODEL("sprites/laserbeam.spr");
	PRECACHE_MODEL("sprites/tele1.spr");
}

BOOL CPitWorm::FVisible(CBaseEntity *pEntity)
{
	if( FBitSet( pEntity->pev->flags, FL_NOTARGET ) )
		return FALSE;

	if( ( pev->waterlevel != 3 && pEntity->pev->waterlevel == 3 )
		|| ( pev->waterlevel == 3 && pEntity->pev->waterlevel == 0 ) )
		return FALSE;

	TraceResult tr;
	Vector vecLookerOrigin;
	Vector vecLookerAngle;

	GetAttachment(0, vecLookerOrigin, vecLookerAngle);
	UTIL_TraceLine( vecLookerOrigin, pEntity->EyePosition(), ignore_monsters, ignore_glass, ENT( pev ), &tr );

	return tr.flFraction == 1.0;
}

BOOL CPitWorm::FVisible(const Vector& vecOrigin)
{
	TraceResult tr;
	Vector vecLookerOrigin;
	Vector vecLookerAngle;

	GetAttachment(0, vecLookerOrigin, vecLookerAngle);
	UTIL_TraceLine( vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, ENT( pev ), &tr );
	return tr.flFraction == 1.0;
}

//=========================================================
// IdleSound
//=========================================================
void CPitWorm::IdleSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), VOL_NORM, PITWORM_ATTN);
}

//=========================================================
// AlertSound
//=========================================================
void CPitWorm::AlertSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), VOL_NORM, PITWORM_ATTN);
}

//=========================================================
// DeathSound
//=========================================================
void CPitWorm::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), VOL_NORM, PITWORM_ATTN);
}

void CPitWorm::PainSound(void)
{
	if (m_flNextPainSound <= gpGlobals->time)
	{
		m_flNextPainSound = gpGlobals->time + RANDOM_LONG(2, 5);
		EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), VOL_NORM, PITWORM_ATTN);
	}
}

//=========================================================
// AngrySound
//=========================================================
void CPitWorm::AngrySound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAngrySounds), VOL_NORM, PITWORM_ATTN);
}

//=========================================================
// SwipeSound
//=========================================================
void CPitWorm::SwipeSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pSwipeSounds), VOL_NORM, PITWORM_ATTN);
}

//=========================================================
// BeamSound
//=========================================================
void CPitWorm::BeamSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitworm/pit_worm_attack_eyeblast.wav", VOL_NORM, PITWORM_ATTN);
}

//=========================================================
// HandleAnimEvent
//=========================================================
void CPitWorm::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case PITWORM_AE_SWIPE:	// bang
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pHitGroundSounds), VOL_NORM, ATTN_NORM, 0, 100 + RANDOM_FLOAT(-5,5));

		if (pev->sequence == 2)
			UTIL_ScreenShake(pev->origin, 12.0, 100.0, 2.0, 100);
		else
			UTIL_ScreenShake(pev->origin, 4.0, 3.0, 1.0, 750.0);
	}
		break;
	case PITWORM_AE_EYEBLAST_START: // start killing swing
	{
		if ( gpGlobals->time - m_flLastEventTime >= 1.1 )
		{
			if (m_hEnemy)
			{
				m_posBeam = m_hEnemy->pev->origin;
				m_posBeam.z += 24;

				Vector vecEyePos, vecEyeAng;
				GetAttachment(0, vecEyePos, vecEyeAng);

				m_vecBeam = (m_posBeam - vecEyePos).Normalize();
				m_angleBeam = UTIL_VecToAngles(m_vecBeam);
				UTIL_MakeVectors(m_angleBeam);
				ShootBeam();
				m_fLockYaw = TRUE;
			}
		}
	}
		break;
	case PITWORM_AE_EYEBLAST_END: // end killing swing
	{
		m_fLockYaw = TRUE;
	}
		break;
	default:
		CBaseMonster::HandleAnimEvent(pEvent);

	}
}

void CPitWorm::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if ( ptr->iHitgroup == HITGROUP_HEAD )
	{
		if (gpGlobals->time > m_flTakeHitTime )
		{
			pev->health -= flDamage;
			if (pev->health <= 0)
			{
				pev->health = pev->max_health;
				m_iWasHit = TRUE;
				m_flTakeHitTime = m_flTakeHitTime + RANDOM_LONG(2,4);
			}
		}

		UTIL_BloodDrips(ptr->vecEndPos, vecDir, m_bloodColor, flDamage * 10);
		UTIL_BloodDecalTrace(ptr, m_bloodColor);

		if (m_hEnemy == 0)
		{
			m_hEnemy = Instance(pevAttacker);
		}
		if (pev->skin == 0)
		{
			pev->skin = 1;
			m_flLastBlinkInterval = gpGlobals->time;
			m_flLastBlinkTime = gpGlobals->time;
		}
	}
	else
	{
		if (pev->dmgtime != gpGlobals->time || RANDOM_LONG(0, 10) >= 0 )
		{
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_LONG(0.5, 1.5));
			pev->dmgtime = gpGlobals->time;
		}
	}
}

void CPitWorm::CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	switch (useType)
	{
	case USE_ON:
		CSoundEnt::InsertSound(bits_SOUND_WORLD, pActivator->pev->origin, 1024, 1.0);
		//ALERT(at_console, "USE_ON\n");
		break;
	case USE_OFF:
	case USE_TOGGLE:
	{
		pev->takedamage = DAMAGE_NO;
		pev->health = 0;

		SetThink(&CPitWorm::DyingThink);
		pev->nextthink = gpGlobals->time;
	}
		break;
	default:
		break;
	}
}


//=========================================================
//
//=========================================================
int CPitWorm::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	PainSound();
	return 0;
}


//=========================================================
// StartupThink
//=========================================================
void CPitWorm::StartupThink(void)
{
	CBaseEntity *pEntity = NULL;

	pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tleveldead");
	if (pEntity)
	{
		ALERT(at_console, "level dead node set\n");
		m_flTargetLevels[0] = pEntity->pev->origin.z;
		m_flLevels[0] = m_flTargetLevels[0] - 300;
	}

	pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tlevel1");
	if (pEntity)
	{
		ALERT(at_console, "level 1 node set\n");
		m_flTargetLevels[1] = pEntity->pev->origin.z;
		m_flLevels[1] = m_flTargetLevels[1] - 300;
	}

	pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tlevel2");
	if (pEntity)
	{
		ALERT(at_console, "level 2 node set\n");
		m_flTargetLevels[2] = pEntity->pev->origin.z;
		m_flLevels[2] = m_flTargetLevels[2] - 300;
	}

	pEntity = UTIL_FindEntityByTargetname(NULL, "pw_tlevel3");
	if (pEntity)
	{
		ALERT(at_console, "level 3 node set\n");
		m_flTargetLevels[3] = pEntity->pev->origin.z;
		m_flLevels[3] = m_flTargetLevels[3] - 350;
	}

	m_iLevel = 2;
	if (!FStringNull(pev->target))
	{
		if (FStrEq(STRING(pev->target), "pw_level1"))
			m_iLevel = 1;
		else if (FStrEq(STRING(pev->target), "pw_level2"))
			m_iLevel = 2;
		else if (FStrEq(STRING(pev->target), "pw_level3"))
			m_iLevel = 3;
		else if (FStrEq(STRING(pev->target), "pw_leveldead"))
			m_iLevel = 0;
	}

	m_posDesired.z = m_flLevels[m_iLevel];

	Vector vecEyePos, vecEyeAng;
	GetAttachment(0, vecEyePos, vecEyeAng);
	pev->view_ofs = vecEyePos - pev->origin;

	SetThink(&CPitWorm::HuntThink);
	SetUse(&CPitWorm::CommandUse);
	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
//
//=========================================================
void CPitWorm::StartupUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SetThink(&CPitWorm::HuntThink);
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse(&CPitWorm::CommandUse);
}

//=========================================================
// NullThink
//=========================================================
void CPitWorm::NullThink(void)
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.5;
}

//=========================================================
// DyingThink
//=========================================================
void CPitWorm::DyingThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
	GlowShellUpdate();
	DispatchAnimEvents();
	StudioFrameAdvance();

	if (pev->deadflag == DEAD_DYING)
	{
		if ( gpGlobals->time - m_flDeathStartTime > 3.0 )
		{
			ChangeLevel();
		}
		if ( fabs(pev->origin.z - this->m_flLevels[0]) < 16.0 )
		{
			pev->velocity = Vector(0, 0, 0);
			pev->deadflag = DEAD_DEAD;
			SetThink(&CBaseEntity::SUB_Remove);
			pev->nextthink = gpGlobals->time + 0.1;
		}
	}
	else
	{
		pev->deadflag = DEAD_DYING;
		int deathAnim = LookupSequence("death");
		int iDir = 1;
		m_posDesired.z = m_flLevels[0];
		pev->sequence = FindTransition(pev->sequence, deathAnim, &iDir);
		if (iDir <= 0)
		{
			pev->frame = 255;
		}
		else
		{
			pev->frame = 0;
		}
		m_flLevelSpeed = 5;
		ResetSequenceInfo();
		DeathSound();

		if (m_pBeam)
		{
			UTIL_Remove(m_pBeam);
			m_pBeam = NULL;
		}
		if (m_pSprite)
		{
			UTIL_Remove(m_pSprite);
			m_pSprite = NULL;
		}

		SetUse(NULL);
		SetTouch(NULL);
	}
}

//=========================================================
// HuntThink
//=========================================================
void CPitWorm::HuntThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
	GlowShellUpdate();
	DispatchAnimEvents();
	StudioFrameAdvance();

	if (m_pBeam)
	{
		if (m_hEnemy != 0 && m_flBeamExpireTime > gpGlobals->time)
		{
			StrafeBeam();
		}
		else
		{
			UTIL_Remove(m_pBeam);
			m_pBeam = NULL;
			UTIL_Remove(m_pSprite);
			m_pSprite = NULL;
		}
	}

	if (pev->health <= 0)
	{
		SetThink(&CPitWorm::DyingThink);
		m_fSequenceFinished = TRUE;
	}
	else
	{
		if ( (gpGlobals->time - m_flLastBlinkTime) >= 6.0 && !m_pBeam )
		{
			pev->skin = 1;
			m_flLastBlinkInterval = gpGlobals->time;
			m_flLastBlinkTime = gpGlobals->time;
		}

		if (pev->skin)
		{
			if ( gpGlobals->time - m_flLastBlinkInterval >= 0.0 )
			{
				if ( pev->skin == 5 )
					pev->skin = 0;
				else
					pev->skin++;
				m_flLastBlinkInterval = gpGlobals->time;
			}
		}
	}

	if (m_iWasHit)
	{
		int iDir = 1;
		const char* flinchAnim = RANDOM_LONG(0,1) ? "flinch1" : "flinch2";
		pev->sequence = FindTransition(pev->sequence, LookupSequence(flinchAnim), &iDir);
		if (iDir > 0)
		{
			pev->frame = 0;
		}
		else
		{
			pev->frame = 255;
		}
		ResetSequenceInfo();
		m_iWasHit = FALSE;
		PainSound();
	}
	else if (m_fSequenceFinished)
	{
		int oldSeq = pev->sequence;
		if ( m_fAttacking )
		{
			m_fLockHeight = FALSE;
			m_fLockYaw = FALSE;
			m_fAttacking = FALSE;
			m_flNextMeleeTime = gpGlobals->time + 0.25;
		}
		NextActivity();
		if (pev->sequence != oldSeq || !m_fSequenceLoops)
		{
			pev->frame = 0;
			ResetSequenceInfo();
		}
	}

	if (m_hEnemy != 0)
	{
		if (FVisible(m_hEnemy))
		{
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->pev->origin;
			m_posTarget.z += 24;
			Vector vecEyePos, vecEyeAng;
			GetAttachment(0, vecEyePos, vecEyeAng);
			m_vecTarget = (m_posTarget - vecEyePos).Normalize();
			m_vecDesired = m_vecTarget;
		}
	}

	if (m_posDesired.z > m_flLevels[3])
	{
		m_posDesired.z = m_flLevels[3];
	}
	else if (m_posDesired.z < m_flLevels[0])
	{
		m_posDesired.z = m_flLevels[0];
	}
	ChangeLevel();
	if (m_hEnemy != 0 && !m_pBeam)
	{
		TrackEnemy();
	}
}

//=========================================================
//
//=========================================================
void CPitWorm::HitTouch(CBaseEntity* pOther)
{
	TraceResult tr = UTIL_GetGlobalTrace();

	if (pOther->pev->modelindex == pev->modelindex)
		return;

	if (m_flHitTime > gpGlobals->time)
		return;

	if( tr.pHit == NULL || tr.pHit->v.modelindex != pev->modelindex )
		return;

	if (pOther->pev->takedamage)
	{
		pOther->TakeDamage(pev, pev, gSkillData.pwormDmgSwipe, DMG_CRUSH|DMG_SLASH);
		pOther->pev->punchangle.z = 15;
		pOther->pev->velocity.z += 200;
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackSounds), VOL_NORM, ATTN_NORM, 0, 100 + RANDOM_FLOAT(-5,5));
		m_flHitTime = gpGlobals->time + 1.0;
	}
}

//=========================================================
// NextActivity
//=========================================================
void CPitWorm::NextActivity()
{
	UTIL_MakeAimVectors(pev->angles);

	if (m_hEnemy)
	{
		if (!m_hEnemy->IsAlive())
		{
			m_hEnemy = 0;
			m_flIdealHeadYaw = 0;
		}
	}

	if (gpGlobals->time > m_flLastSeen + 15)
	{
		if (m_hEnemy != 0)
		{
			if ((pev->origin - m_hEnemy->pev->origin).Length() > 700)
				m_hEnemy = NULL;
		}
	}

	if (m_hEnemy == 0)
	{
		Look(4096);
		m_hEnemy = BestVisibleEnemy();
	}

	if (m_hEnemy != 0 || m_fFirstSighting)
	{
		if (m_iWasHit)
		{
			const char* flinchAnim = RANDOM_LONG(0,1) ? "flinch1" : "flinch2";
			pev->sequence = LookupSequence(flinchAnim);
			m_iWasHit = FALSE;
			PainSound();
			m_fLockHeight = 0;
			m_fLockYaw = 0;
			m_fAttacking = 0;
		}
		else if (pev->origin.z == m_posDesired.z)
		{
			if ( abs((int)floor(m_flIdealTorsoYaw - m_flTorsoYaw)) > 10 || !ClawAttack() )
			{
				if ( RANDOM_LONG(0, 2) == 0 )
				{
					IdleSound();
				}

				pev->sequence = LookupSequence("idle2");
				m_fLockHeight = FALSE;
				m_fLockYaw = FALSE;
				m_fAttacking = FALSE;
			}
		}
		else
		{
			if ( RANDOM_LONG(0, 2) == 0 )
			{
				IdleSound();
			}
			pev->sequence = LookupSequence("idle");
			m_fLockHeight = FALSE;
			m_fLockYaw = FALSE;
			m_fAttacking = FALSE;
		}
	}
	if (m_hEnemy != 0 && !m_fFirstSighting)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitworm/pit_worm_alert.wav", VOL_NORM, 0.1, 0, 100);
		m_fFirstSighting = TRUE;
		pev->sequence = LookupSequence("scream");
	}
}

void CPitWorm::EyeLight(const Vector &vecEyePos)
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( entindex() );		// entity, attachment
		WRITE_COORD( vecEyePos.x );		// origin
		WRITE_COORD( vecEyePos.y );
		WRITE_COORD( vecEyePos.z );
		WRITE_COORD( 128 );	// radius
		WRITE_BYTE( 128 );	// R
		WRITE_BYTE( 255 );	// G
		WRITE_BYTE( 128 );	// B
		WRITE_BYTE( 1 );	// life * 10
		WRITE_COORD( 2 ); // decay
	MESSAGE_END();
}

void CPitWorm::BeamEffect(TraceResult &tr)
{
	CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
	if( pEntity != NULL && pEntity->pev->takedamage )
	{
		ClearMultiDamage();
		pEntity->TraceAttack(pev, gSkillData.pwormDmgBeam, m_vecBeam, &tr, DMG_ENERGYBEAM);
		ApplyMultiDamage(pev, pev);
	}
	else if ( tr.flFraction != 1.0f )
	{
		UTIL_DecalTrace(&tr, DECAL_GUNSHOT1 + RANDOM_LONG(0,4));
		m_pBeam->DoSparks(tr.vecEndPos, tr.vecEndPos);
	}
}

void CPitWorm::LockTopLevel()
{
	if (m_iLevel == 3)
	{
		pev->health = pev->max_health;
		m_iWasHit = 1;
		m_iLevel = 2;
		m_fTopLevelLocked = TRUE;
		m_flTakeHitTime = gpGlobals->time + RANDOM_LONG(0,2);
		m_posDesired.z = m_flLevels[2];
	}
	else
	{
		m_fTopLevelLocked = TRUE;
	}
}

BOOL CPitWorm::ClawAttack()
{
	if (m_hEnemy == 0)
		return FALSE;
	if (pev->origin.z != m_posDesired.z)
		return FALSE;
	if ( m_flNextMeleeTime > gpGlobals->time )
		return FALSE;

	float flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();
	Vector targetAngle = UTIL_VecToAngles((m_posTarget - pev->origin).Normalize());
	float angleDiff = UTIL_AngleDiff(targetAngle.y, pev->angles.y);

	if (!FVisible(m_posTarget))
	{
		if (flDist < 600)
		{
			return 0;
		}

		BOOL shouldClaw = FALSE;
		if (m_iLevel == 2)
		{
			if (angleDiff >= 30)
			{
				pev->sequence = LookupSequence("doorclaw1");
				m_flIdealHeadYaw = 0;
				shouldClaw = TRUE;
			}
		}
		else if (m_iLevel == 1)
		{
			if ( angleDiff <= -30.0 )
			{
				pev->sequence = LookupSequence("doorclaw2");
				m_flIdealHeadYaw = 0;
				shouldClaw = TRUE;
			}
			if ( angleDiff >= 30.0 )
			{
				pev->sequence = LookupSequence("doorclaw3");
				m_flIdealHeadYaw = 0;
				shouldClaw = TRUE;
			}
		}

		if (shouldClaw)
		{
			SwipeSound();
			m_fLockHeight = TRUE;
			m_fLockYaw = TRUE;
			m_fAttacking = TRUE;
			return TRUE;
		}

		return FALSE;
	}

	m_fLockYaw = FALSE;
	if (m_iLevel == 2)
	{
		if (angleDiff < 30)
		{
			if (flDist > 425 || angleDiff >= -50)
			{
				pev->sequence = LookupSequence("eyeblast");
				m_posBeam = m_posTarget;
				m_vecBeam = m_vecTarget;
				m_angleBeam = UTIL_VecToAngles(m_vecBeam);
			}
			else
			{
				pev->sequence = LookupSequence("attack");
			}
		}
		else
		{
			pev->sequence = LookupSequence("doorclaw1");
			m_flIdealHeadYaw = 0;
			m_fLockYaw = TRUE;
		}
	}
	else if (m_iLevel == 3)
	{
		if (flDist <= 425)
		{
			pev->sequence = RANDOM_LONG(0,1) ? LookupSequence("platclaw1") : LookupSequence("platclaw2");
		}
		else
		{
			pev->sequence = LookupSequence("eyeblast");
			m_posBeam = m_posTarget;
			m_vecBeam = m_vecTarget;
			m_angleBeam = UTIL_VecToAngles(m_vecBeam);
		}
	}
	else if (m_iLevel == 1)
	{
		if (angleDiff < 50)
		{
			if (angleDiff > -30)
			{
				if (flDist > 425)
				{
					pev->sequence = LookupSequence("eyeblast");
					m_posBeam = m_posTarget;
					m_vecBeam = m_vecTarget;
					m_angleBeam = UTIL_VecToAngles(m_vecBeam);
				}
				else
				{
					pev->sequence = LookupSequence("attacklow");
				}
			}
			else
			{
				pev->sequence = LookupSequence("doorclaw2");
			}
		}
		else
		{
			pev->sequence = LookupSequence("doorclaw3");
			m_flIdealHeadYaw = 0;
			m_fLockYaw = TRUE;
		}
	}
	if (pev->sequence == LookupSequence("eyeblast"))
	{
		BeamSound();
	}
	else
	{
		SwipeSound();
	}
	m_fAttacking = TRUE;
	m_fLockHeight = TRUE;
	return 1;
}

void CPitWorm::ShootBeam()
{
	if ( m_hEnemy != 0 )
	{
		if ( m_flHeadYaw > 0.0 )
		{
			m_offsetBeam = 80;
			m_flBeamDir = -1;
		}
		else
		{
			m_offsetBeam = -80;
			m_flBeamDir = 1;
		}
		Vector vecEyePos, vecEyeAng;
		GetAttachment(0, vecEyePos, vecEyeAng);

		m_vecBeam = (m_posBeam - vecEyePos).Normalize();
		m_angleBeam = UTIL_VecToAngles(m_vecBeam);
		UTIL_MakeVectors(m_angleBeam);
		m_vecBeam = gpGlobals->v_forward;
		m_vecBeam.z = -m_vecBeam.z;
		Vector vecEnd = m_vecBeam * m_offsetBeam + m_vecBeam * 1280 + vecEyePos;

		TraceResult tr;
		UTIL_TraceLine(vecEyePos, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

		m_pBeam = CBeam::BeamCreate("sprites/laserbeam.spr", 80);
		if ( m_pBeam )
		{
			m_pBeam->PointEntInit(tr.vecEndPos, entindex());
			m_pBeam->SetEndAttachment(1);
			m_pBeam->SetColor(0,255,32);
			m_pBeam->SetBrightness(128);
			m_pBeam->SetWidth(32);
			m_pBeam->pev->spawnflags |= SF_BEAM_SPARKSTART;

			BeamEffect(tr);

			m_pBeam->DoSparks(vecEyePos, vecEyePos);
			m_flBeamExpireTime = gpGlobals->time + 0.9;
			float beamYaw = m_flHeadYaw - m_flBeamDir * 25.0;
			if ( beamYaw < -45.0 || beamYaw > 45.0 )
			{
				m_flIdealHeadYaw += m_flBeamDir * 50;
			}

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pShootSounds), VOL_NORM, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5));

			EyeLight(vecEyePos);

			m_pSprite = CSprite::SpriteCreate("sprites/tele1.spr", vecEyePos, TRUE);
			if ( m_pSprite )
			{
				m_pSprite->SetTransparency(kRenderGlow, 0, 255, 0, 0, kRenderFxNoDissipation);
				m_pSprite->SetAttachment(edict(), 1);
				m_pSprite->SetScale(0.75);
				m_pSprite->pev->framerate = 10;
				m_pSprite->TurnOn();
			}
		}
	}
}

void CPitWorm::StrafeBeam()
{
	m_offsetBeam += m_flBeamDir * 20;

	Vector vecEyePos, vecEyeAng;
	GetAttachment(0, vecEyePos, vecEyeAng);

	m_vecBeam = (m_posBeam - vecEyePos).Normalize();
	m_angleBeam = UTIL_VecToAngles(m_vecBeam);
	UTIL_MakeVectors(m_angleBeam);
	m_vecBeam = gpGlobals->v_forward;
	m_vecBeam.z = -m_vecBeam.z;

	Vector vecEnd = m_vecBeam * m_offsetBeam + m_vecBeam * 1280 + vecEyePos;

	TraceResult tr;
	UTIL_TraceLine(vecEyePos, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

	m_pBeam->SetStartPos(tr.vecEndPos);
	BeamEffect(tr);

	EyeLight(vecEyePos);
}

void CPitWorm::ChangeLevel()
{
	float currentZ = pev->origin.z;
	float desiredZ = m_posDesired.z;
	if (currentZ > desiredZ)
	{
		pev->origin.z -= m_flLevelSpeed;

		if (pev->origin.z < desiredZ)
			pev->origin.z = desiredZ;
	}
	else if (currentZ < desiredZ)
	{
		pev->origin.z += m_flLevelSpeed;

		if (pev->origin.z > desiredZ)
			pev->origin.z = desiredZ;
	}

	if (m_flIdealTorsoYaw != m_flTorsoYaw)
	{
		if (m_flIdealTorsoYaw < m_flTorsoYaw)
		{
			m_flTorsoYaw -= 5.0;
			if (m_flTorsoYaw < m_flIdealTorsoYaw)
				m_flTorsoYaw = m_flIdealTorsoYaw;
		}
		else if (m_flIdealTorsoYaw > m_flTorsoYaw)
		{
			m_flTorsoYaw += 5;
			if (m_flTorsoYaw > m_flIdealTorsoYaw)
				m_flTorsoYaw = m_flIdealTorsoYaw;
		}
		SetBoneController(PITWORM_CONTROLLER_BODY_YAW, m_flTorsoYaw);
	}

	if (m_flIdealHeadYaw != m_flHeadYaw)
	{
		if (m_flIdealHeadYaw < m_flHeadYaw)
		{
			m_flHeadYaw -= 5.0;
			if (m_flHeadYaw < m_flIdealHeadYaw)
				m_flHeadYaw = m_flIdealHeadYaw;
		}
		else if (m_flIdealHeadYaw > m_flHeadYaw)
		{
			m_flHeadYaw += 5;
			if (m_flHeadYaw > m_flIdealHeadYaw)
				m_flHeadYaw = m_flIdealHeadYaw;
		}
		SetBoneController(PITWORM_CONTROLLER_EYE_YAW, m_flHeadYaw);
	}

	if (m_flIdealHeadPitch != m_flHeadPitch)
	{
		if (m_flIdealHeadPitch < m_flHeadPitch)
		{
			m_flHeadPitch -= 5.0;
			if (m_flHeadPitch < m_flIdealHeadPitch)
				m_flHeadPitch = m_flIdealHeadPitch;
		}
		else if (m_flIdealHeadPitch > m_flHeadPitch)
		{
			m_flHeadPitch += 5;
			if (m_flHeadPitch > m_flIdealHeadPitch)
				m_flHeadPitch = m_flIdealHeadPitch;
		}
		SetBoneController(PITWORM_CONTROLLER_EYE_PITCH, m_flHeadPitch);
	}
}

void CPitWorm::TrackEnemy()
{
	Vector vecEyePos, vecEyeAng;
	GetAttachment(0, vecEyePos, vecEyeAng);

	Vector vec = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - Vector(pev->origin.x, pev->origin.y, vecEyePos.z);
	Vector vecDir = UTIL_VecToAngles(vec);
	float angleDiff = UTIL_AngleDiff(vecDir.x, pev->angles.x);
	if (angleDiff < PITWORM_EYE_PITCH_MIN)
	{
		angleDiff = PITWORM_EYE_PITCH_MIN;
	}
	else if (angleDiff > PITWORM_EYE_PITCH_MAX)
	{
		angleDiff = PITWORM_EYE_PITCH_MAX;
	}
	m_flIdealHeadPitch = angleDiff;

	float targetYaw = UTIL_VecToYaw(m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - vecEyePos) - pev->angles.y;
	if (targetYaw > 180)
	{
		targetYaw -= 360;
	}
	else if (targetYaw < -180)
	{
		targetYaw += 360;
	}
	if (!m_fLockYaw)
	{
		if (targetYaw < 0)
		{
			const float minYaw = (m_iLevel == 1) ? -30 : -50;
			if (targetYaw <= minYaw)
				targetYaw = minYaw;
		}
		else
		{
			const float maxYaw = (m_iLevel == 2) ? 30 :50;
			if (targetYaw > maxYaw)
				targetYaw = maxYaw;
		}
		m_flIdealTorsoYaw = targetYaw;
	}
	float torsoYawDiff = m_flTorsoYaw - targetYaw;
	if (torsoYawDiff > 0)
	{
		if (torsoYawDiff > PITWORM_EYE_YAW_MAX)
		{
			torsoYawDiff = PITWORM_EYE_YAW_MAX;
		}
		else if (torsoYawDiff < PITWORM_EYE_YAW_MIN)
		{
			torsoYawDiff = PITWORM_EYE_YAW_MIN;
		}
	}
	if (!m_fAttacking || m_pBeam != 0)
	{
		m_flIdealHeadYaw = torsoYawDiff;
	}

	if (!m_fLockHeight)
	{
		if (m_hEnemy->pev->origin.z <= m_flTargetLevels[1])
		{
			m_iLevel = 0;
		}
		else if (m_hEnemy->pev->origin.z <= m_flTargetLevels[2])
		{
			m_iLevel = 1;
		}
		else if (m_fTopLevelLocked || m_hEnemy->pev->origin.z <= m_flTargetLevels[3])
		{
			m_iLevel = 2;
		}
		else
		{
			m_iLevel = 3;
		}
		m_posDesired.z = m_flLevels[m_iLevel];
	}
}

class CPitWormSteamTrigger : public CBaseEntity
{
public:
	void Spawn();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS(info_pitworm_steam_lock, CPitWormSteamTrigger)

void CPitWormSteamTrigger::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	UTIL_SetOrigin(pev, pev->origin);
}

void CPitWormSteamTrigger::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CPitWorm* pWorm = (CPitWorm*)UTIL_FindEntityByClassname(0, "monster_pitworm_up");
	if (pWorm)
	{
		pWorm->LockTopLevel();
	}
}
