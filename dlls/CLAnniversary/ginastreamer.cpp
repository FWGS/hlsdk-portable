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
#include	"nodes.h"


class CGinaDart : public CBaseEntity
{
public:
	void Spawn(void);

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecDir, int pevdmg);
	void Touch(CBaseEntity *pOther);
	int damage;

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS(ginadart, CGinaDart);

TYPEDESCRIPTION	CGinaDart::m_SaveData[] =
{
	DEFINE_FIELD(CGinaDart, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CGinaDart, CBaseEntity);

void CGinaDart::Spawn(void)
{
	pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;
	pev->gravity = 0.5;
	damage = 15;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
}

void CGinaDart::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecDir, int pevdmg)
{
	CGinaDart *pSpit = GetClassPtr((CGinaDart *)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles(vecVelocity);
	pSpit->damage = pevdmg;
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CGinaDart::Touch(CBaseEntity *pOther)
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT(90, 110);

	if (!pOther->pev->takedamage)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/xbow_hit1.wav", 1, ATTN_NORM, 0, iPitch);
	}
	else
	{
		if (!FClassnameIs(pOther->pev, "monster_ginastreamer"))
		{			
			pOther->TakeDamage(pev, pev, 15, DMG_NEVERGIB);
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM, 0, iPitch);
		}
	}

	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define GINA_SLASH					1
#define GINA_TELEPORT				2
#define	GINA_SHOOT					3
#define	GINA_BURSTSHOOT				4

#define ZOMBIE_FLINCH_DELAY			8		// at most one flinch every n secs

#define GINA_PWR_NONE				0
#define GINA_PWR_QUAD				1
#define GINA_PWR_HASTE				2
#define GINA_PWR_INVISIBILITY		3

class CGinaStreamer : public CBaseMonster
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
	void RunAI(void);
	void Shoot(void);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	float LastTeleported;
	float LastPowerupUsed;
	float PowerupCooldown;
	int powerup;
	BOOL CheckTeleport(void);
	BOOL CheckRetreatTeleport(void);

	int IRelationship(CBaseEntity *pTarget);

	float m_flNextFlinch;

	void AttackSound(void);
	void DeathSound(void);
	void AlertSound(void);

	int		m_iShotgunShell;

	static const char *pTauntSounds[];
	static const char *pDeathSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_ginastreamer, CGinaStreamer);

const char *CGinaStreamer::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CGinaStreamer::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CGinaStreamer::pTauntSounds[] =
{
	"ginastreamer/ginataunt1.wav",
	"ginastreamer/ginataunt2.wav",
	"ginastreamer/ginataunt3.wav",
	"ginastreamer/ginataunt4.wav",
	"ginastreamer/ginataunt5.wav",
	"ginastreamer/ginataunt6.wav",
	"ginastreamer/ginataunt7.wav",
	"ginastreamer/ginataunt8.wav",
	"ginastreamer/ginataunt9.wav",
};

const char *CGinaStreamer::pDeathSounds[] =
{
	"rocketgina/Chkdeth1.wav",
	"rocketgina/Chkdeth2.wav"
};

int CGinaStreamer::IRelationship(CBaseEntity *pTarget)
{
	if (FClassnameIs(pTarget->pev, "monster_oetker"))
		return R_HT;

	return CBaseMonster::IRelationship(pTarget);
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGinaStreamer::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGinaStreamer::SetYawSpeed(void)
{
	pev->yaw_speed = 600;
}

int CGinaStreamer::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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


void CGinaStreamer::AlertSound(void)
{
	if (LastSpoken < gpGlobals->time)
	{
		int pitch = 95 + RANDOM_LONG(0, 9);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pTauntSounds[RANDOM_LONG(0, ARRAYSIZE(pTauntSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
		LastSpoken = gpGlobals->time + 10;
	}
}


void CGinaStreamer::AttackSound(void)
{
	if (LastSpoken < gpGlobals->time)
	{
		// Play a random attack sound
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pTauntSounds[RANDOM_LONG(0, ARRAYSIZE(pTauntSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		LastSpoken = gpGlobals->time + 10;
	}
}

void CGinaStreamer::DeathSound(void)
{
	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pDeathSounds[RANDOM_LONG(0, ARRAYSIZE(pDeathSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGinaStreamer::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	if (powerup == GINA_PWR_HASTE)
		pev->framerate = 2.0;

	switch (pEvent->event)
	{
	case GINA_SLASH:
	{
		// do stuff for this event.
		 CBaseEntity *pHurt;
		 if (powerup == GINA_PWR_QUAD)
		 {
			 EMIT_SOUND(ENT(pev), CHAN_ITEM, "generic/quad.wav", 1, ATTN_NORM);
			 pHurt = CheckTraceHullAttack(130, gSkillData.zombieDmgOneSlash * 4, DMG_SLASH);
		 }	
		else
			pHurt = CheckTraceHullAttack(130, gSkillData.zombieDmgOneSlash, DMG_SLASH);

		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
			
		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case GINA_SHOOT:
	{
		Shoot();
		if (RANDOM_LONG(0, 4))
			AttackSound();
	}
	break;

	case GINA_TELEPORT:
	{
		if (m_hEnemy)
		{
			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_SMOKE);
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.x, pev->absmax.x));
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.y, pev->absmax.y));
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT(g_sModelIndexSmoke);
			WRITE_BYTE(25); // scale * 10
			WRITE_BYTE(10); // framerate
			MESSAGE_END();
			UTIL_MakeVectors(m_hEnemy->pev->angles);
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/beamstart8.wav", 0.8, ATTN_NORM, 0, 100);
			if (RANDOM_LONG(0, 1) && CheckTeleport())
			{
				pev->origin = m_hEnemy->pev->origin - gpGlobals->v_forward * 100;
				pev->velocity = pev->velocity + gpGlobals->v_forward * 50;
			}
			else
			{	
				int iNode = WorldGraph.FindNearestNode(m_hEnemy->pev->origin, bits_NODE_LAND | bits_NODE_WATER | bits_NODE_AIR);
				if (iNode != NO_NODE)
				{
					CNode &node = WorldGraph.Node(iNode + 1);
					TraceResult tr;
					UTIL_TraceHull(node.m_vecOrigin + Vector(0, 0, 32), node.m_vecOrigin + Vector(0, 0, 32), dont_ignore_monsters, large_hull, NULL, &tr);
					if (tr.fStartSolid == 0)
					{
						pev->origin = node.m_vecOrigin;
					}		
				}
			}
			m_flNextShootTime = gpGlobals->time;
			pev->angles.y = m_hEnemy->pev->angles.y;
			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_SMOKE);
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.x, pev->absmax.x));
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.y, pev->absmax.y));
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT(g_sModelIndexSmoke);
			WRITE_BYTE(25); // scale * 10
			WRITE_BYTE(10); // framerate
			MESSAGE_END();
		}
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
void CGinaStreamer::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/ginastreamer.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 3500;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	LastTeleported = gpGlobals->time;
	LastPowerupUsed = gpGlobals->time + 10;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGinaStreamer::Precache()
{
	PRECACHE_MODEL("models/ginastreamer.mdl");
	PRECACHE_SOUND("weapons/sshotgun_shoot.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fire1.wav");

	PRECACHE_SOUND("generic/pwr_quad.wav");
	PRECACHE_SOUND("generic/quad.wav");
	PRECACHE_SOUND("generic/pwr_haste.wav");
	PRECACHE_SOUND("generic/pwr_invisibility.wav");

	PRECACHE_SOUND("debris/beamstart8.wav");

	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");

	PRECACHE_SOUND_ARRAY(pTauntSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CGinaStreamer::IgnoreConditions(void)
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
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;

}


void CGinaStreamer::RunAI(void)
{
	CBaseMonster::RunAI();
	if ((powerup != GINA_PWR_NONE) && (gpGlobals->time >= PowerupCooldown + 25))
	{
		powerup = GINA_PWR_NONE;
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
		ClearShockEffect();
	}
	if (powerup == GINA_PWR_INVISIBILITY)
	{
		if (m_Activity == ACT_RUN || m_Activity == ACT_WALK || m_Activity == ACT_IDLE)
		{
			if (pev->renderamt != 5)
			{
				pev->renderamt = 5;
				pev->rendermode = kRenderTransTexture;
			}
		}
		else
		{
			if (pev->renderamt != 35)
			{
				pev->renderamt = 35;
				pev->rendermode = kRenderTransTexture;
			}
		}
	}
}

BOOL CGinaStreamer::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 120 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CGinaStreamer::CheckMeleeAttack2(float flDot, float flDist)
{
	if (CheckTeleport() && gpGlobals->time > LastTeleported)
	{
		LastTeleported = gpGlobals->time + 3;
		return TRUE;
	}
	return FALSE;
}

BOOL CGinaStreamer::CheckTeleport()
{
	TraceResult	tr;
	UTIL_MakeVectors(m_hEnemy->pev->angles);
	UTIL_TraceHull(m_hEnemy->pev->origin, m_hEnemy->pev->origin - gpGlobals->v_forward * 100, dont_ignore_monsters, head_hull, edict(), &tr);
	return (tr.flFraction == 1.0);
}

BOOL CGinaStreamer::CheckRetreatTeleport()
{
	return TRUE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CGinaStreamer::CheckRangeAttack1(float flDot, float flDist)
{
	if (gpGlobals->time <= LastTeleported)
		return FALSE;
	if (flDist > 60 && flDist <= 2100 && flDot >= 0.5 && gpGlobals->time >= m_flNextShootTime)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 80)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}
		return TRUE;
	}

	return FALSE;
}


//=========================================================
// CheckRangeAttack2
//=========================================================
BOOL CGinaStreamer::CheckRangeAttack2(float flDot, float flDist)
{
	if (flDist > 80 && flDist <= 2100 && flDot >= 0.5 && gpGlobals->time >= m_flNextShootTime)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 80)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}
		return TRUE;
	}

	return FALSE;
}

void CGinaStreamer::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector	vecSpitOffset;
	Vector	vecSpitDir;
	Vector anglesAim = pev->v_angle + pev->punchangle;
	UTIL_MakeVectors(anglesAim);
	anglesAim.x = -anglesAim.x;
	UTIL_MakeVectors(pev->angles);

	// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
	// we should be able to read the position of bones at runtime for this info.
	vecSpitOffset = (gpGlobals->v_forward * 37 + gpGlobals->v_up * 64);
	vecSpitOffset = (pev->origin + vecSpitOffset);
	vecSpitDir = ((m_hEnemy->pev->origin + gpGlobals->v_up * 18) - vecSpitOffset).Normalize();

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM);
	if (powerup == GINA_PWR_QUAD)
	{
		CGinaDart::Shoot(pev, vecSpitOffset, vecSpitDir * 1800, anglesAim,60);
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "generic/quad.wav", 1, ATTN_NORM);
	}
	else
		CGinaDart::Shoot(pev, vecSpitOffset, vecSpitDir * 1800, anglesAim,15);

	
	m_flNextShootTime = gpGlobals->time + 5;
}

void CGinaStreamer::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (gpGlobals->time >= LastPowerupUsed)
	{
		LastPowerupUsed = gpGlobals->time + 45;
		PowerupCooldown = gpGlobals->time;
		
		switch (RANDOM_LONG(0, 2))
		{
		case 0: //Quad
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "generic/pwr_quad.wav", 1, ATTN_NONE);
			AddShockEffect(0, 255, 255, 16, 25);
			powerup = GINA_PWR_QUAD;
			break;
		}
		case 1: //Haste
		{
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "generic/pwr_haste.wav", 1, ATTN_NONE);
			AddShockEffect(255, 201, 14, 16, 25);
			powerup = GINA_PWR_HASTE;
			break;
		}
		case 2: //Invisible
		{
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "generic/pwr_invisibility.wav", 1, ATTN_NONE);
			pev->renderamt = 5;
			pev->rendermode = kRenderTransTexture;
			powerup = GINA_PWR_INVISIBILITY;
			break;
		}
		}
	}
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}
