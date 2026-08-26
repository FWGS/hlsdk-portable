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
#include	"soundent.h"


//FIREBALL

class CFireWave : public CBaseEntity
{
	void Spawn(int type);
	void Precache(void);
	int  Classify(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	int bolttype;
	int m_iTrail;

public:
	static CFireWave *BoltCreate(int type);
};
LINK_ENTITY_TO_CLASS(firewave, CFireWave);

CFireWave *CFireWave::BoltCreate(int type)
{
	// Create a new entity with CCrossbowBolt private data
	CFireWave *pBolt = GetClassPtr((CFireWave *)NULL);
	pBolt->pev->classname = MAKE_STRING("firewave");
	pBolt->Spawn(type);
	pBolt->bolttype = type;
	return pBolt;
}

void CFireWave::Spawn(int type)
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	SET_MODEL(ENT(pev), "sprites/gwave1.spr");
	pev->rendermode = kRenderTransColor;
	pev->renderamt = 255;
	pev->gravity = 0.5;
	pev->frame = 0;
	pev->framerate = 8;
	pev->scale = 0.5;
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));
	// rocket trail
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);

	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex());	// entity
	WRITE_SHORT(m_iTrail);	// model
	WRITE_BYTE(15); // life
	WRITE_BYTE(10);  // width
	WRITE_BYTE(255);   // r, g, b
	WRITE_BYTE(0);   // r, g, b
	WRITE_BYTE(0);   // r, g, b
	WRITE_BYTE(255);	// brightness

	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = rand() % 255;
	pev->renderamt = 255;
	pev->renderfx = kRenderFxNoDissipation;
	SetTouch(&CFireWave::BoltTouch);
	pev->nextthink = gpGlobals->time + 0.2;
}

void CFireWave::Precache()
{
	PRECACHE_MODEL("sprites/gwave1.spr");
	m_iTrail = PRECACHE_MODEL("sprites/laserbeam.spr");
}


int	CFireWave::Classify(void)
{
	return	CLASS_NONE;
}

void CFireWave::BoltTouch(CBaseEntity *pOther)
{
	if (FClassnameIs(pOther->pev, "firewave"))
		return;

	SetTouch(NULL);
	SetThink(NULL);
	UTIL_Remove(this);

	
	if (pOther->edict() == pev->owner)
		return;

		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -32), ignore_monsters, ENT(pev), &tr);
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_EXPLOSION);		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD(pev->origin.x);	// Send to PAS because of the sound
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexFireball);
		WRITE_BYTE(15); // scale * 10
		WRITE_BYTE(30); // framerate
		WRITE_BYTE(TE_EXPLFLAG_NONE);
		MESSAGE_END();
		RadiusDamage(pev->origin, pev, pev, 40, 128, CLASS_NONE, DMG_ENERGYBEAM);

		// draw decal
		if (!(pev->spawnflags))
		{
			UTIL_DecalTrace(&tr, 11);
		}

}


//ICE BARRAGE

class CIceBarrage : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void RunAI(void);
	int  Classify(void);
	int ISoundMask(void);
	int dmgcounter = 0;
	void Killed(entvars_t *pevAttacker, int iGib);
	void EXPORT IceTouch(CBaseEntity *pOther);
	void Animate(float frames);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
};

LINK_ENTITY_TO_CLASS(monster_icebarrage, CIceBarrage);

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CIceBarrage::Classify(void)
{
	return	CLASS_NONE;
}

void CIceBarrage::Killed(entvars_t *pevAttacker, int iGib)
{
	UTIL_Remove(this);
	return;
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CIceBarrage::ISoundMask(void)
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CIceBarrage::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/w_icebarrage.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 100));

	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_FLY;
	pev->health = 30;
	pev->takedamage = DAMAGE_NO;
	pev->max_health = pev->health;
	m_MonsterState = MONSTERSTATE_NONE;
	pev->renderamt = 185;
	pev->rendermode = kRenderGlow;
	pev->nextthink = gpGlobals->time + 0.1;
	MonsterInit();
	SetTouch(&CIceBarrage::IceTouch);
	Vector lightorigin = pev->origin + gpGlobals->v_up * 1;

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(lightorigin.x);	// X
	WRITE_COORD(lightorigin.y);	// Y
	WRITE_COORD(lightorigin.z);	// Z
	WRITE_BYTE(10);		// radius * 0.1
	WRITE_BYTE(255);		// r
	WRITE_BYTE(255);		// g
	WRITE_BYTE(255);		// b
	WRITE_BYTE(30);		// time * 10
	WRITE_BYTE(0);		// decay * 0.1
	MESSAGE_END();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CIceBarrage::Precache()
{
	PRECACHE_MODEL("models/w_icebarrage.mdl");
}

void CIceBarrage::IceTouch(CBaseEntity *pOther)
{
	if (pOther->edict() == pev->owner)
		return;

	if (pOther->IsBSPModel())
		return;

	if (pOther)
	{
		pOther->pev->origin.x = pev->origin.x;
		pOther->pev->origin.y = pev->origin.y;

		if (dmgcounter >= 3)
		{
			pOther->TakeDamage(pev, pev, 1, DMG_FREEZE);
			dmgcounter = 0;
		}
		dmgcounter++;
		if (pOther->IsPlayer() == true)
		{
			UTIL_ScreenShake(pOther->pev->origin, 8, 100, 3, 64);
			UTIL_ScreenFade(pOther, Vector(0,0,255), 0.25, 0.25, 125, 0x0002);
		}
	}
}

void CIceBarrage::RunAI(void)
{
	CBaseMonster::RunAI();
	pev->health--;

	if (pev->health < 15)
	{
		pev->renderamt = pev->health * 10;
	}
	if (pev->health <= 0)
	{
		UTIL_Remove(this);
	}
}

void CIceBarrage::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define OETKER_SLASH				1
#define OETKER_MAGIC				2
#define OETKER_TELEPORT				3
#define OETKER_SHOWSCIMMY			4
#define OETKER_ICEBARRAGE			7
#define OETKER_ICEBARRAGEHIT		6
#define OETKER_SHOOTMAGIC			8

#define ZOMBIE_FLINCH_DELAY			8		// at most one flinch every n secs

#define OETKER_PWR_NONE				0
#define OETKER_PWR_QUAD				1
#define OETKER_PWR_HASTE			2
#define OETKER_PWR_INVISIBILITY		3

#define OETKER_WEP_NONE				0
#define OETKER_WEP_SCIMMY			1
#define OETKER_WEP_STAFF			2

class COetker : public CBaseMonster
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
	int m_flNextSpecialShootTime;
	void RunAI(void);
	void Shoot(void);
	void SwitchWeapon(int NewWep);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	float LastTeleported;
	float LastPowerupUsed;
	float LastWepSwitched;
	float PowerupCooldown;
	int powerup;
	BOOL CheckTeleport(void);
	BOOL CheckRetreatTeleport(void);

	int IRelationship(CBaseEntity *pTarget);

	float m_flNextFlinch;

	void AttackSound(void);
	void PainSound(void);
	void AlertSound(void);

	int		m_iShotgunShell;

	static TYPEDESCRIPTION m_SaveData[];
	int		m_CurrentWep;

	static const char *pTauntSounds[];
	static const char *pPainSounds[];
	static const char *pAttackMissSounds[];

	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_oetker, COetker);

TYPEDESCRIPTION	COetker::m_SaveData[] =
{
	DEFINE_FIELD(COetker, m_CurrentWep, FIELD_INTEGER),
};

const char *COetker::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *COetker::pTauntSounds[] =
{
	"oetker/taunt1.wav",
	"oetker/taunt2.wav",
	"oetker/taunt3.wav",
	"oetker/taunt4.wav",
	"oetker/taunt5.wav",
	"oetker/taunt6.wav",
};

const char *COetker::pPainSounds[] =
{
	"oetker/pain1.wav",
	"oetker/pain2.wav",
	"oetker/pain3.wav",
	"oetker/pain4.wav"
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	COetker::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COetker::SetYawSpeed(void)
{
	pev->yaw_speed = 900;
}

int COetker::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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


void COetker::AlertSound(void)
{
	if (LastSpoken < gpGlobals->time)
	{
		int pitch = 95 + RANDOM_LONG(0, 9);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pTauntSounds[RANDOM_LONG(0, ARRAYSIZE(pTauntSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
		LastSpoken = gpGlobals->time + 10;
	}
}


void COetker::AttackSound(void)
{
	if (LastSpoken < gpGlobals->time)
	{
		// Play a random attack sound
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pTauntSounds[RANDOM_LONG(0, ARRAYSIZE(pTauntSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		LastSpoken = gpGlobals->time + 10;
	}
}

void COetker::PainSound(void)
{
	if (LastSpoken < gpGlobals->time)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		LastSpoken = gpGlobals->time + 5;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void COetker::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	if (powerup == OETKER_PWR_HASTE)
		pev->framerate = 2.0;

	switch (pEvent->event)
	{
	case OETKER_SLASH:
	{
					   // do stuff for this event.
					   CBaseEntity *pHurt;
					   if (powerup == OETKER_PWR_QUAD)
					   {
						   EMIT_SOUND(ENT(pev), CHAN_ITEM, "generic/quad.wav", 1, ATTN_NORM);
						   pHurt = CheckTraceHullAttack(130, gSkillData.zombieDmgOneSlash * 8, DMG_SLASH);
					   }
					   else
						   pHurt = CheckTraceHullAttack(130, gSkillData.zombieDmgOneSlash * 2, DMG_SLASH);

					   if (pHurt)
					   {
						   if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
						   {
							   pHurt->pev->punchangle.x = 5;
							   pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
						   }
						   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/scimitar_hitbod.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
					   }
					   else
						   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

					   if (RANDOM_LONG(0, 1))
						   AttackSound();
	}
		break;

	case OETKER_MAGIC:
	{
					   Shoot();
					   if (RANDOM_LONG(0, 1))
						   AttackSound();
	}
		break;

	case OETKER_TELEPORT:
	{
						  if (m_hEnemy)
						  {
							  UTIL_MakeVectors(m_hEnemy->pev->angles);
							  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "oetker/teleport.wav", 0.8, ATTN_NORM, 0, 100);
							  if (RANDOM_LONG(0, 1) && CheckTeleport())
							  {
								  //pev->origin = m_hEnemy->pev->origin - gpGlobals->v_forward * 100 + gpGlobals->v_up * 32;
								  pev->origin = m_hEnemy->pev->origin - gpGlobals->v_forward * 100;
								  pev->velocity = pev->velocity + gpGlobals->v_forward * 50;
							  }
							  else
							  {
								  int iNode = WorldGraph.FindNearestNode(m_hEnemy->pev->origin, bits_NODE_LAND );
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
						  }
	}
		break;
	case OETKER_SHOWSCIMMY:
	{
							  SwitchWeapon(OETKER_WEP_SCIMMY);
	}
		break;
	case OETKER_ICEBARRAGE:
	{
							  m_flNextSpecialShootTime = gpGlobals->time + 10;
							  EMIT_SOUND(ENT(pev), CHAN_ITEM, "oetker/icebarrage.wav", 1, ATTN_NONE);
	}
		break;
	case OETKER_ICEBARRAGEHIT:
	{
		if (m_hEnemy)
		{
			EMIT_SOUND(ENT(m_hEnemy->pev), CHAN_ITEM, "oetker/icebarragehit.wav", 1, ATTN_NONE);
			CBaseEntity *pIce = CBaseEntity::Create("monster_icebarrage", m_hEnemy->pev->origin - gpGlobals->v_up * 20, Vector(0, 90, 0), edict());
			
		}					  
	}
		break;
	case OETKER_SHOOTMAGIC:
	{
	  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "oetker/firewave.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void COetker::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/oetker.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 6000;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	LastTeleported = gpGlobals->time;
	LastPowerupUsed = gpGlobals->time + 10;
	pev->body = m_CurrentWep;

	MonsterInit();
}

int COetker::IRelationship(CBaseEntity *pTarget)
{
	if (FClassnameIs(pTarget->pev, "monster_ginastreamer"))
		return R_HT;

	return CBaseMonster::IRelationship(pTarget);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COetker::Precache()
{
	PRECACHE_MODEL("models/oetker.mdl");
	PRECACHE_SOUND("weapons/sshotgun_shoot.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fire1.wav");
	PRECACHE_SOUND("weapons/scimitar_hitbod.wav");

	PRECACHE_SOUND("oetker/icebarrage.wav");
	PRECACHE_SOUND("oetker/icebarragehit.wav");
	PRECACHE_SOUND("oetker/firewave.wav");

	PRECACHE_SOUND("generic/pwr_quad.wav");
	PRECACHE_SOUND("generic/quad.wav");
	PRECACHE_SOUND("generic/pwr_haste.wav");
	PRECACHE_SOUND("generic/pwr_invisibility.wav");

	PRECACHE_SOUND("oetker/teleport.wav");

	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");

	UTIL_PrecacheOther("monster_icebarrage");
	UTIL_PrecacheOther("firewave");

	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pTauntSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


int COetker::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);

	return iIgnore;

}


void COetker::SwitchWeapon(int NewWep)
{
	if (NewWep > OETKER_WEP_STAFF)
		NewWep = OETKER_WEP_SCIMMY;
	LastWepSwitched = gpGlobals->time + RANDOM_LONG(10,30);
	m_CurrentWep = NewWep;
	pev->body = NewWep;
}

void COetker::RunAI(void)
{
	CBaseMonster::RunAI();
	if ((powerup != OETKER_PWR_NONE) && (gpGlobals->time >= PowerupCooldown + 25))
	{
		powerup = OETKER_PWR_NONE;
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
		ClearShockEffect();
	}
	if (powerup == OETKER_PWR_INVISIBILITY)
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

BOOL COetker::CheckMeleeAttack1(float flDot, float flDist)
{
	if (m_CurrentWep != OETKER_WEP_SCIMMY)
	{
		if (m_CurrentWep == OETKER_WEP_NONE)
			SwitchWeapon(OETKER_WEP_STAFF); //SCIMMY FIRST, CHANGE LATER
		return FALSE;
	}

	if (flDist <= 120 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL COetker::CheckMeleeAttack2(float flDot, float flDist)
{
	//THIS IS TELEPORT
	if (CheckTeleport() && gpGlobals->time > LastTeleported)
	{
		//First, check if you wanna switch weapons instead of teleporting
		if (gpGlobals->time > LastWepSwitched)
		{
			SwitchWeapon(m_CurrentWep + 1);
		}
		LastTeleported = gpGlobals->time + 3;
		return TRUE;
	}
	return FALSE;
}

BOOL COetker::CheckTeleport()
{
	CBaseEntity *pEntity = NULL;

	if(UTIL_FindEntityByClassname(pEntity, "monster_icebarrage") != NULL)
		return false;

	TraceResult	tr;
	UTIL_MakeVectors(m_hEnemy->pev->angles);
	UTIL_TraceHull(m_hEnemy->pev->origin, m_hEnemy->pev->origin - gpGlobals->v_forward * 100, dont_ignore_monsters, head_hull, edict(), &tr);
	return (tr.flFraction == 1.0);
}

BOOL COetker::CheckRetreatTeleport()
{
	return TRUE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL COetker::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_CurrentWep != OETKER_WEP_STAFF)
		return FALSE;

	//use special instead
	if (gpGlobals->time >= m_flNextSpecialShootTime)
		return FALSE;
	if (flDist <= 512 && flDot >= 0.5 && gpGlobals->time >= m_flNextShootTime)
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
BOOL COetker::CheckRangeAttack2(float flDot, float flDist)
{
	if (m_CurrentWep != OETKER_WEP_STAFF)
		return FALSE;
	if (flDist <= 2100 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpecialShootTime)
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

void COetker::Shoot(void)
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
	vecSpitOffset = (gpGlobals->v_right * 8 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 55);
	vecSpitOffset = (pev->origin + vecSpitOffset);
	if (m_hEnemy)
		vecSpitDir = (m_hEnemy->pev->origin - vecSpitOffset).Normalize();
	else
		vecSpitDir = ((pev->origin + gpGlobals->v_forward * 32) - vecSpitOffset).Normalize();

	CFireWave *pBolt = CFireWave::BoltCreate(0);
	pBolt->pev->origin = vecSpitOffset;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = edict();
	pBolt->pev->velocity = vecSpitDir * 1600;
	pBolt->pev->speed = 800;

	m_flNextShootTime = gpGlobals->time + 4;
}

void COetker::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	/*if (gpGlobals->time >= LastPowerupUsed)
	{
		LastPowerupUsed = gpGlobals->time + 45;
		PowerupCooldown = gpGlobals->time;

		switch (RANDOM_LONG(0, 2))
		{
		case 0: //Quad
		{
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "generic/pwr_quad.wav", 1, ATTN_NONE);
					AddShockEffect(0, 255, 255, 16, 25);
					powerup = OETKER_PWR_QUAD;
					break;
		}
		case 1: //Haste
		{
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "generic/pwr_haste.wav", 1, ATTN_NONE);
					AddShockEffect(255, 201, 14, 16, 25);
					powerup = OETKER_PWR_HASTE;
					break;
		}
		case 2: //Invisible
		{
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "generic/pwr_invisibility.wav", 1, ATTN_NONE);
					pev->renderamt = 5;
					pev->rendermode = kRenderTransTexture;
					powerup = OETKER_PWR_INVISIBILITY;
					break;
		}
		}
	}*/
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}
