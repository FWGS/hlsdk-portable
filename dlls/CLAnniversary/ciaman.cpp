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
#include	"decals.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	CIAMAN_KICK				1
#define	CIAMAN_SHOOT			2

#define ZOMBIE_FLINCH_DELAY			10		// at most one flinch every n secs

class CCIAMan : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);
	int m_iSpriteTexture;

	float m_flNextFlinch;
	float lastspecialdone;

	void DeathSound(void);
	void AlertSound(void);
	void AttackSound(void);
	
	void RunAI(void);

	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pAlertSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist) { return FALSE; }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_ciaman, CCIAMan);

const char *CCIAMan::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CCIAMan::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CCIAMan::pAttackSounds[] =
{
	"ciaman/cia_attack1.wav",
	"ciaman/cia_attack2.wav",
	"ciaman/cia_attack3.wav",
};

const char *CCIAMan::pAlertSounds[] =
{
	"ciaman/cia_alert1.wav",
	"ciaman/cia_alert2.wav",
	"ciaman/cia_alert3.wav",
};

const char *CCIAMan::pDeathSounds[] =
{
	"generic/death.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCIAMan::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCIAMan::SetYawSpeed(void)
{
	pev->yaw_speed = 360;
}

int CCIAMan::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

	// HACK HACK -- until we fix this.
	if (IsAlive())
		PainSound();
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CCIAMan::DeathSound(void)
{
	int pitch = 100 + RANDOM_LONG(-8, 8);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pDeathSounds[RANDOM_LONG(0, ARRAYSIZE(pDeathSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CCIAMan::AlertSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}


void CCIAMan::AttackSound(void)
{
	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}




class CLaserBeam : public CBaseEntity
{
	void Spawn(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	int LaserLife;
	Vector EnemyPos;
	Vector BeginPos;
	CBeam *m_pBeam[1];
	entvars_t *Enemy;
	

	int m_iTrail;

public:
	static CLaserBeam *BoltCreate(Vector BeginPos, Vector EnemyPos, entvars_t *pevEnemy);
	entvars_t *Master;
};
LINK_ENTITY_TO_CLASS(laserbeam, CLaserBeam);

CLaserBeam *CLaserBeam::BoltCreate(Vector lBeginPos, Vector lEnemyPos, entvars_t *pevEnemy)
{
	// Create a new entity with CCrossbowBolt private data
	CLaserBeam *pBolt = GetClassPtr((CLaserBeam *)NULL);
	pBolt->pev->classname = MAKE_STRING("laserbeam");
	pBolt->Spawn();
	pBolt->LaserLife = 15;
	pBolt->EnemyPos = lEnemyPos;
	pBolt->BeginPos = lBeginPos;
	pBolt->Enemy = pevEnemy;
	

	return pBolt;
}

void CLaserBeam::Spawn()
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

	SetThink(&CLaserBeam::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.01;
}


int	CLaserBeam::Classify(void)
{
	return	CLASS_NONE;
}


void CLaserBeam::BubbleThink(void)
{
	LaserLife--;
	if (LaserLife <= 0 || Master->health <= 0)
	{
		UTIL_Remove(this);
		return;
	}


	m_pBeam[0] = CBeam::BeamCreate("sprites/laserbeam.spr", 8);
	if (!m_pBeam[0])
		return;

	UTIL_MakeVectors(Master->angles);
	pev->origin = Master->origin + gpGlobals->v_up * 38 + gpGlobals->v_forward * 38;

	TraceResult tr;
	UTIL_TraceLine(pev->origin, EnemyPos + gpGlobals->v_forward * 16, dont_ignore_monsters, ENT(pev->owner), &tr);

	m_pBeam[0]->PointsInit(tr.vecEndPos, pev->origin);
	m_pBeam[0]->SetColor(255, 196, 0);
	m_pBeam[0]->SetBrightness(255);
	m_pBeam[0]->LiveForTime(0.05);
	m_pBeam[0]->SetScrollRate(155);
	m_pBeam[0]->SetWidth(45);
	UTIL_Sparks(tr.vecEndPos);
	m_pBeam[0]->SetNoise(0);

	EnemyPos = Enemy->origin;

	UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH2);

	if (tr.pHit)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity != NULL && pEntity->pev->takedamage)
		{
			pEntity->TakeDamage(pev, pev, 0.25, DMG_ENERGYBEAM);
			pEntity->pev->velocity = pEntity->pev->velocity * 0.1;
			//pEntity->TraceAttack( pev, 8, vecAim, &tr, DMG_ENERGYBEAM );
		}
	}

	pev->nextthink = gpGlobals->time + 0.05;
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCIAMan::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case CIAMAN_SHOOT:
	{

		if (m_hEnemy)
		{
			Vector vecSrc, vecAim;
			TraceResult tr;

			vecSrc = pev->origin + gpGlobals->v_up * 38 + gpGlobals->v_forward * 16;
			vecAim = ShootAtEnemy(vecSrc);
			UTIL_TraceLine(vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT(pev), &tr);


			CLaserBeam *pBolt = CLaserBeam::BoltCreate(vecSrc, tr.vecEndPos, m_hEnemy->pev);
			pBolt->pev->owner = edict();
			pBolt->pev->origin = vecSrc + gpGlobals->v_forward * 24 + gpGlobals->v_up * 8;
			pBolt->Master = pev;


			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/beamstart3.wav", 1, ATTN_NORM, 0, 100);

			UTIL_EmitAmbientSound(ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG(140, 160));

			if (RANDOM_LONG(0, 1))
				AttackSound();

		}
	}
		break;
	case CIAMAN_KICK:
	{
							// do stuff for this event.
							CBaseEntity *pHurt = CheckTraceHullAttack(95, gSkillData.zombieDmgBothSlash, DMG_SLASH);
							if (pHurt)
							{
								if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
								{
									pHurt->pev->punchangle.x = 5;
									pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 50 + gpGlobals->v_up * 50;
								}
							}

							EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 50);

							if (RANDOM_LONG(0, 3) == 0)
								AttackSound();
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
void CCIAMan::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/ciaman.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 35;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	lastspecialdone = gpGlobals->time;

	MonsterInit();
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCIAMan::Precache()
{
	PRECACHE_MODEL("models/ciaman.mdl");
	PRECACHE_SOUND("debris/beamstart3.wav");
	PRECACHE_MODEL("sprites/laserbeam.spr");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_MODEL("sprites/glow02.spr");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/shockwave.spr");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);

}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


void CCIAMan::RunAI(void)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(pev->origin.x);	// X
	WRITE_COORD(pev->origin.y);	// Y
	WRITE_COORD(pev->origin.z);	// Z
	WRITE_BYTE(7);		// radius * 0.1
	WRITE_BYTE(0);		// r
	WRITE_BYTE(225);		// g
	WRITE_BYTE(0);		// b
	WRITE_BYTE(2);		// time * 10
	WRITE_BYTE(0);		// decay * 0.1
	MESSAGE_END();
	// first, do base class stuff
	CBaseMonster::RunAI();

	if (m_hEnemy == NULL && m_Activity == ACT_IDLE)
	{
		if (FStrEq(STRING(gpGlobals->mapname), "survival"))
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex(1);
			if (pPlayer)
			{
				m_hTargetEnt = pPlayer;
				m_hEnemy = pPlayer;
				m_MonsterState = MONSTERSTATE_COMBAT;
				MoveToLocation(ACT_RUN, 0.0, pPlayer->pev->origin);
			}
		}
	}
}


int CCIAMan::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
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


BOOL CCIAMan::CheckMeleeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= 95 && flDot >= 0.6 && m_hEnemy != NULL)
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CCIAMan::CheckRangeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist > 95 && flDist <= 450 && flDot >= 0.6 && m_hEnemy != NULL)
	{
		return TRUE;
	}
	return FALSE;
}