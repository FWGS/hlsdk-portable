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
// Alien slave monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"decals.h"

extern DLL_GLOBAL int		g_iSkillLevel;
int beamleftover;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ISLAVE_AE_CLAW		( 1 )
#define		ISLAVE_AE_CLAWRAKE	( 2 )
#define		ISLAVE_AE_ZAP_POWERUP	( 3 )
#define		ISLAVE_AE_ZAP_SHOOT		( 4 )
#define		ISLAVE_AE_ZAP_DONE		( 5 )
#define		ISLAVE_AE_PROJECTILE	( 6 )

#define		ISLAVE_MAX_BEAMS	8



class CElectroBall : public CBaseEntity
{
	void Spawn(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	void EXPORT ElectroBallTouch(CBaseEntity *pOther);
	CBeam *m_pBeam;
	int m_maxFrame;

	int m_iTrail;

public:
	static CElectroBall *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(electroball, CElectroBall);

CElectroBall *CElectroBall::BoltCreate(void)
{
	// Create a new entity with CCrossbowBolt private data
	CElectroBall *pBolt = GetClassPtr((CElectroBall *)NULL);
	pBolt->pev->classname = MAKE_STRING("electroball");
	pBolt->Spawn();

	return pBolt;
}

void CElectroBall::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "sprites/e-tele1.spr");
	pev->renderamt = 255;
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 196;
	pev->rendercolor.z = 0;
	pev->scale = 0.3;
	pev->gravity = 0.1;
	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;


	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	SetTouch(&CElectroBall::ElectroBallTouch);
	SetThink(&CElectroBall::BubbleThink);

	pev->nextthink = gpGlobals->time + 0.01;
}


int	CElectroBall::Classify(void)
{
	return	CLASS_NONE;
}

void CElectroBall::ElectroBallTouch(CBaseEntity *pOther)
{
	if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		UTIL_Sparks(pev->origin);
	if (pOther->pev->takedamage)
	{
		pOther->TakeDamage(pev, pev, 5, DMG_ENERGYBEAM);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/zap4.wav", 1, ATTN_NORM, 0, RANDOM_FLOAT(90, 110));
	}
	UTIL_Remove(this);
}


void CElectroBall::BubbleThink(void)
{
	CBaseEntity *pEntity = NULL;
	pev->velocity = pev->velocity + gpGlobals->v_up * RANDOM_LONG(-8, 8) + gpGlobals->v_right * RANDOM_LONG(-8, 8);
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 150)) != NULL)
	{
		if (FClassnameIs(pEntity->pev, "electroball"))
		{
			m_pBeam = CBeam::BeamCreate("sprites/lgtning.spr", 30);
			if (!m_pBeam)
				return;

			m_pBeam->PointsInit(pev->origin, pEntity->pev->origin);
			m_pBeam->SetColor(255, 196, 0);
			m_pBeam->SetBrightness(255);
			m_pBeam->LiveForTime(0.1);
			m_pBeam->SetWidth(45);
			m_pBeam->SetNoise(85);

			TraceResult tr;
			UTIL_TraceLine(pev->origin, pEntity->pev->origin, dont_ignore_monsters, ENT(pev->owner), &tr);
			if (tr.pHit)
			{
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
				if (pEntity != NULL && pEntity->pev->takedamage)
				{
					pEntity->TakeDamage(pev, pev, 4, DMG_ENERGYBEAM);
				}
			}
			break;
		}
	}
	if (pev->frame++)
	{
		if (pev->frame > m_maxFrame)
		{
			pev->frame = 0;
		}
	}
	pev->nextthink = gpGlobals->time + 0.01;
}


class CRoboSlave : public CSquadMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int	 ISoundMask(void);
	int  Classify(void);
	int  IRelationship(CBaseEntity *pTarget);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	void CallForHelp(char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void Explode(void);
	void DeathSound(void);
	void PainSound(void);
	void AlertSound(void);
	void IdleSound(void);
	int Projectiles;

	void Killed(entvars_t *pevAttacker, int iGib);

	void StartTask(Task_t *pTask);
	Schedule_t *GetSchedule(void);
	Schedule_t *GetScheduleOfType(int Type);
	CUSTOM_SCHEDULES;

	int	Save(CSave &save);
	int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	void ClearBeams();
	void ArmBeam(int side);
	void WackBeam(int side, CBaseEntity *pEntity);
	void ZapBeam(int side);
	void BeamGlow(void);

	int m_iBravery;

	CBeam *m_pBeam[ISLAVE_MAX_BEAMS];

	int m_iBeams;
	float m_flNextAttack;

	int	m_voicePitch;

	EHANDLE m_hDead;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
};
LINK_ENTITY_TO_CLASS(monster_robowizard, CRoboSlave);


TYPEDESCRIPTION	CRoboSlave::m_SaveData[] =
{
	DEFINE_FIELD(CRoboSlave, m_iBravery, FIELD_INTEGER),

	DEFINE_ARRAY(CRoboSlave, m_pBeam, FIELD_CLASSPTR, ISLAVE_MAX_BEAMS),
	DEFINE_FIELD(CRoboSlave, m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD(CRoboSlave, m_flNextAttack, FIELD_TIME),

	DEFINE_FIELD(CRoboSlave, m_voicePitch, FIELD_INTEGER),

	DEFINE_FIELD(CRoboSlave, m_hDead, FIELD_EHANDLE),

};

IMPLEMENT_SAVERESTORE(CRoboSlave, CSquadMonster);


const char *CRoboSlave::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CRoboSlave::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CRoboSlave::pPainSounds[] =
{
	"robowizard/slv_pain1.wav",
	"robowizard/slv_pain2.wav",
};

const char *CRoboSlave::pDeathSounds[] =
{
	"robowizard/slv_die1.wav",
	"robowizard/slv_die2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CRoboSlave::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}


int CRoboSlave::IRelationship(CBaseEntity *pTarget)
{
	if ((pTarget->IsPlayer()))
	if ((pev->spawnflags & SF_MONSTER_WAIT_UNTIL_PROVOKED) && !(m_afMemory & bits_MEMORY_PROVOKED))
		return R_NO;
	return CBaseMonster::IRelationship(pTarget);
}


void CRoboSlave::CallForHelp(char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation)
{
	// ALERT( at_aiconsole, "help " );

	// skip ones not on my netname
	if (FStringNull(pev->netname))
		return;

	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByString(pEntity, "netname", STRING(pev->netname))) != NULL)
	{
		float d = (pev->origin - pEntity->pev->origin).Length();
		if (d < flDist)
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if (pMonster)
			{
				pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
				pMonster->PushEnemy(hEnemy, vecLocation);
			}
		}
	}
}


//=========================================================
// ALertSound - scream
//=========================================================
void CRoboSlave::AlertSound(void)
{
	if (m_hEnemy != NULL)
	{
		SENTENCEG_PlayRndSz(ENT(pev), "RBW_ALERT", 0.85, ATTN_NORM, 0, m_voicePitch);

		CallForHelp("monster_robowizard", 512, m_hEnemy, m_vecEnemyLKP);
	}
}

//=========================================================
// IdleSound
//=========================================================
void CRoboSlave::IdleSound(void)
{
	if (RANDOM_LONG(0, 6) == 0)
	{
		SENTENCEG_PlayRndSz(ENT(pev), "RBW_IDLE", 0.85, ATTN_NORM, 0, m_voicePitch);
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
void CRoboSlave::PainSound(void)
{
	//SPECIAL HAPPENING
	if (RANDOM_LONG(0, 8) == 0) //SPECIAL HAPPENING
	{
		switch (RANDOM_LONG(0, 10))
		{
		case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain1.wav", 1, ATTN_NORM, 0, 100); break;
		case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain2.wav", 1, ATTN_NORM, 0, 100); break;
		case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain3.wav", 1, ATTN_NORM, 0, 100); break;
		case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain4.wav", 1, ATTN_NORM, 0, 100); break;
		case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain5.wav", 1, ATTN_NORM, 0, 100); break;
		case 5: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain6.wav", 1, ATTN_NORM, 0, 100); break;
		case 6: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain7.wav", 1, ATTN_NORM, 0, 100); break;
		case 7: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain8.wav", 1, ATTN_NORM, 0, 100); break;
		case 8: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain9.wav", 1, ATTN_NORM, 0, 100); break;
		case 9: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain10.wav", 1, ATTN_NORM, 0, 100); break;
		case 10: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain11.wav", 1, ATTN_NORM, 0, 100); break;
		}
		return;
	}
	//SPECIAL HAPPENING
	if (RANDOM_LONG(0, 8) == 0) //SPECIAL HAPPENING
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, m_voicePitch);
	}
}

//=========================================================
// DieSound
//=========================================================

void CRoboSlave::DeathSound(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pDeathSounds[RANDOM_LONG(0, ARRAYSIZE(pDeathSounds) - 1)], 1.0, ATTN_NORM, 0, m_voicePitch);
	SetThink(&CRoboSlave::Explode);
	pev->nextthink = gpGlobals->time + 2;
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CRoboSlave::ISoundMask(void)
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}


void CRoboSlave::Killed(entvars_t *pevAttacker, int iGib)
{
	ClearBeams();
	CSquadMonster::Killed(pevAttacker, iGib);
}

void CRoboSlave::Explode(void)
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -32), ignore_monsters, ENT(pev), &tr);
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);		// This makes a dynamic light and the explosion sprites/sound
	WRITE_COORD(pev->origin.x);	// Send to PAS because of the sound
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_SHORT(g_sModelIndexFireball);
	WRITE_BYTE(30); // scale * 10
	WRITE_BYTE(15); // framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();
	RadiusDamage(pev, pev, 100, CLASS_NONE, DMG_BLAST);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRoboSlave::SetYawSpeed(void)
{
	int ys;

	switch (m_Activity)
	{
	case ACT_WALK:
		ys = 850;
		break;
	case ACT_RUN:
		ys = 900;
		break;
	case ACT_IDLE:
		ys = 820;
		break;
	default:
		ys = 820;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CRoboSlave::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch (pEvent->event)
	{
	case ISLAVE_AE_CLAW:
	{
						   // SOUND HERE!
						   CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.slaveDmgClaw, DMG_SLASH);
						   if (pHurt)
						   {
							   if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
							   {
								   pHurt->pev->punchangle.z = -18;
								   pHurt->pev->punchangle.x = 5;
							   }
							   // Play a random attack hit sound
							   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, m_voicePitch);
						   }
						   else
						   {
							   // Play a random attack miss sound
							   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, m_voicePitch);
						   }
	}
		break;

	case ISLAVE_AE_CLAWRAKE:
	{
							   CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.slaveDmgClawrake, DMG_SLASH);
							   if (pHurt)
							   {
								   if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
								   {
									   pHurt->pev->punchangle.z = -18;
									   pHurt->pev->punchangle.x = 5;
								   }
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, m_voicePitch);
							   }
							   else
							   {
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, m_voicePitch);
							   }
	}
		break;

	case ISLAVE_AE_ZAP_POWERUP:
	{
								  ArmBeam(5);
	}
		break;

	case ISLAVE_AE_PROJECTILE:
	{
								 if (m_hEnemy)
								 {

									 Vector	vecSpitOffset;
									 Vector	vecSpitDir;
									 Vector anglesAim = pev->v_angle + pev->punchangle;
									 UTIL_MakeVectors(anglesAim);
									 anglesAim.x = -anglesAim.x;
									 UTIL_MakeVectors(pev->angles);

									 // !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
									 // we should be able to read the position of bones at runtime for this info.
									 vecSpitOffset = (gpGlobals->v_right * 8 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 55);
									 vecSpitOffset = (pev->origin + vecSpitOffset);
									 vecSpitDir = ((m_hEnemy->pev->origin + gpGlobals->v_up * 15) - vecSpitOffset).Normalize();

									 vecSpitOffset = pev->origin + gpGlobals->v_forward * 40 + gpGlobals->v_right * RANDOM_LONG(-24, 24) + gpGlobals->v_up * RANDOM_LONG(42, 82);
									 UTIL_MakeAimVectors(pev->angles);
									 CElectroBall *pBolt = CElectroBall::BoltCreate();
									 pBolt->pev->origin = vecSpitOffset;
									 pBolt->pev->velocity = vecSpitDir * RANDOM_LONG(1300, 1500);
									 pBolt->pev->speed = RANDOM_LONG(1300, 1500);
								 }

	}
		break;
	case ISLAVE_AE_ZAP_SHOOT:
	{
								//LAZER
								UTIL_MakeAimVectors(pev->angles);
								ZapBeam(1);
								m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(0.5, 4.0);
	}
		break;

	case ISLAVE_AE_ZAP_DONE:
	{
							   ClearBeams();
	}
		break;

	default:
		CSquadMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// CheckRangeAttack1 - normal beam attack 
//=========================================================
BOOL CRoboSlave::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_flNextAttack > gpGlobals->time)
	{
		return FALSE;
	}

	return CSquadMonster::CheckRangeAttack1(flDot, flDist);
}

//=========================================================
// CheckRangeAttack2 - check bravery and try to resurect dead comrades
//=========================================================
BOOL CRoboSlave::CheckRangeAttack2(float flDot, float flDist)
{
	return FALSE;

	if (m_flNextAttack > gpGlobals->time)
	{
		return FALSE;
	}

	m_hDead = NULL;
	m_iBravery = 0;

	CBaseEntity *pEntity = NULL;
	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "monster_robowizard")) != NULL)
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
				m_iBravery--;
			}
			else
			{
				m_iBravery++;
			}
		}
	}
	if (m_hDead != NULL)
		return TRUE;
	else
		return FALSE;
}


//=========================================================
// StartTask
//=========================================================
void CRoboSlave::StartTask(Task_t *pTask)
{
	ClearBeams();

	CSquadMonster::StartTask(pTask);
}


//=========================================================
// Spawn
//=========================================================
void CRoboSlave::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/robowizard.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.slaveHealth;
	pev->view_ofs = Vector(0, 0, 64);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP;

	m_voicePitch = RANDOM_LONG(85, 110);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRoboSlave::Precache()
{
	beamleftover = PRECACHE_MODEL("sprites/glow02.spr");
	PRECACHE_MODEL("models/robowizard.mdl");
	PRECACHE_MODEL("sprites/laserbeam.spr");
	PRECACHE_MODEL("sprites/e-tele1.spr");
	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("debris/beamstart3.wav");

	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);

	UTIL_PrecacheOther("test_effect");
}


//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CRoboSlave::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// don't slash one of your own
	if ((bitsDamageType & DMG_SLASH) && pevAttacker && IRelationship(Instance(pevAttacker)) < R_DL)
		return 0;

	m_afMemory |= bits_MEMORY_PROVOKED;

	return CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


void CRoboSlave::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (bitsDamageType & DMG_SHOCK)
		return;

	switch (ptr->iHitgroup)
	{
	case 10:
	{
			   UTIL_Ricochet(ptr->vecEndPos, 1.0);
			   UTIL_Sparks(ptr->vecEndPos);
			   flDamage = flDamage / 4;
	}
	}

	CSquadMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================



// primary range attack
Task_t	tlRoboWizardAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t	slRoboWizardAttack1[] =
{
	{
		tlRoboWizardAttack1,
		ARRAYSIZE(tlRoboWizardAttack1),
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE,

		bits_SOUND_DANGER,
		"Slave Range Attack1"
	},
};


DEFINE_CUSTOM_SCHEDULES(CRoboSlave)
{
	slRoboWizardAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES(CRoboSlave, CSquadMonster);


//=========================================================
//=========================================================
Schedule_t *CRoboSlave::GetSchedule(void)
{
	ClearBeams();

	/*
	if (pev->spawnflags)
	{
	pev->spawnflags = 0;
	return GetScheduleOfType( SCHED_RELOAD );
	}
	*/

	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);

		if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
		if (pSound->m_iType & bits_SOUND_COMBAT)
			m_afMemory |= bits_MEMORY_PROVOKED;
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}
		/*
		if (pev->health < 20 || m_iBravery < 0)
		{
		if (!HasConditions( bits_COND_CAN_MELEE_ATTACK1 ))
		{
		m_failSchedule = SCHED_CHASE_ENEMY;
		if (HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		if ( HasConditions ( bits_COND_SEE_ENEMY ) && HasConditions ( bits_COND_ENEMY_FACING_ME ) )
		{
		// ALERT( at_console, "exposed\n");
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		}
		}*/
		break;
	}
	return CSquadMonster::GetSchedule();
}


Schedule_t *CRoboSlave::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_FAIL:
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return CSquadMonster::GetScheduleOfType(SCHED_MELEE_ATTACK1);;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slRoboWizardAttack1;
	case SCHED_RANGE_ATTACK2:
		return slRoboWizardAttack1;
	}
	return CSquadMonster::GetScheduleOfType(Type);
}


//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void CRoboSlave::ArmBeam(int side)
{
	TraceResult tr;
	float flDist = 1.0;

	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT(0, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1);
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT(pev), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if (flDist == 1.0)
		return;

	DecalGunshot(&tr, BULLET_PLAYER_CROWBAR);

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/laserbeam.spr", 30);
	if (!m_pBeam[m_iBeams])
		return;

	UTIL_EmitAmbientSound(ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG(140, 160));

	m_pBeam[m_iBeams]->PointEntInit(tr.vecEndPos, entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
	// m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetColor(255, 196, 0);
	m_pBeam[m_iBeams]->SetBrightness(225);
	m_pBeam[m_iBeams]->LiveForTime(2);
	m_pBeam[m_iBeams]->SetNoise(80);
	m_iBeams++;
}


//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CRoboSlave::BeamGlow()
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
void CRoboSlave::WackBeam(int side, CBaseEntity *pEntity)
{
	Vector vecDest;
	float flDist = 1.0;

	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	if (pEntity == NULL)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/laserbeam.spr", 30);
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit(pEntity->Center(), entindex());
	m_pBeam[m_iBeams]->SetEndAttachment(side < 0 ? 2 : 1);
	m_pBeam[m_iBeams]->SetColor(180, 255, 96);
	m_pBeam[m_iBeams]->SetBrightness(255);
	m_pBeam[m_iBeams]->SetNoise(80);
	m_iBeams++;
}


class CWizardBeam : public CBaseEntity
{
	void Spawn(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	int LaserLife;
	Vector EnemyPos;
	Vector BeginPos;
	CBeam *m_pBeam[1];

	int m_iTrail;

public:
	static CWizardBeam *BoltCreate(Vector BeginPos, Vector EnemyPos);
};
LINK_ENTITY_TO_CLASS(wizardbeam, CWizardBeam);

CWizardBeam *CWizardBeam::BoltCreate(Vector lBeginPos, Vector lEnemyPos)
{
	// Create a new entity with CCrossbowBolt private data
	CWizardBeam *pBolt = GetClassPtr((CWizardBeam *)NULL);
	pBolt->pev->classname = MAKE_STRING("wizardbeam");
	pBolt->Spawn();
	pBolt->LaserLife = 15;
	pBolt->EnemyPos = lEnemyPos;
	pBolt->BeginPos = lBeginPos;

	return pBolt;
}

void CWizardBeam::Spawn()
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;

	SET_MODEL(ENT(pev), "sprites/glow02.spr");
	pev->renderamt = 225;
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 196;
	pev->rendercolor.z = 0;
	pev->scale = 0.6;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(1, 1, 1), Vector(1, 1, 1));

	SetThink(&CWizardBeam::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.01;
}


int	CWizardBeam::Classify(void)
{
	return	CLASS_NONE;
}


void CWizardBeam::BubbleThink(void)
{
	LaserLife--;
	if (LaserLife <= 0)
	{
		UTIL_Remove(this);
		return;
	}


	m_pBeam[0] = CBeam::BeamCreate("sprites/laserbeam.spr", 8);
	if (!m_pBeam[0])
		return;



	TraceResult tr;
	UTIL_TraceLine(pev->origin, EnemyPos, dont_ignore_monsters, ENT(pev->owner), &tr);

	m_pBeam[0]->PointsInit(tr.vecEndPos, pev->origin);
	m_pBeam[0]->SetColor(255, 196, 0);
	m_pBeam[0]->SetBrightness(255);
	m_pBeam[0]->LiveForTime(0.1);
	m_pBeam[0]->SetScrollRate(155);
	m_pBeam[0]->SetWidth(45);
	UTIL_Sparks(tr.vecEndPos);
	m_pBeam[0]->SetNoise(0);

	EnemyPos = EnemyPos + gpGlobals->v_up * 35;

	UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH2);

	if (tr.pHit)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity != NULL && pEntity->pev->takedamage)
		{
			pEntity->TakeDamage(pev, pev, 8, DMG_ENERGYBEAM);
			//pEntity->TraceAttack( pev, 8, vecAim, &tr, DMG_ENERGYBEAM );
		}
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CRoboSlave::ZapBeam(int side)
{
	Vector vecSrc, vecAim;
	TraceResult tr;

	vecSrc = pev->origin + gpGlobals->v_up * 72;
	vecAim = ShootAtEnemy(vecSrc);
	//UTIL_TraceLine(vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT(pev), &tr);

	UTIL_TraceLine(vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT(pev), &tr);
	CWizardBeam *pBolt = CWizardBeam::BoltCreate(vecSrc, (vecSrc + vecAim * 1024) + gpGlobals->v_up * -250);
	pBolt->pev->owner = edict();
	pBolt->pev->origin = vecSrc + gpGlobals->v_forward * 16 + gpGlobals->v_up * 16 + gpGlobals->v_right * 4;

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/beamstart3.wav", 1, ATTN_NORM, 0, 100);

	UTIL_EmitAmbientSound(ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG(140, 160));

	pev->nextthink = gpGlobals->time + 1.0;
	//UTIL_TraceLine(vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT(pev), &tr)


	/*
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity *pEntity;

	if (m_iBeams >= ISLAVE_MAX_BEAMS)
	return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	vecAim = ShootAtEnemy( vecSrc );
	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if (!m_pBeam[m_iBeams])
	return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 20 );
	m_iBeams++;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && pEntity->pev->takedamage)
	{
	pEntity->TraceAttack( pev, gSkillData.slaveDmgZap, vecAim, &tr, DMG_SHOCK );
	}
	UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
	*/
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CRoboSlave::ClearBeams()
{
	for (int i = 0; i < ISLAVE_MAX_BEAMS; i++)
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
