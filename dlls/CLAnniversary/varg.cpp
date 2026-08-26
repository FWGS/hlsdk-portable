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


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	VARG_WHIP		1
#define	VARG_SMACK		2
#define VARG_THROW		3
#define VARG_JUMP		4
#define	VARG_LAND		5

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs



class CVargWhip : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT BoltThink(void);
	CBeam *m_pBeam;
	bool caught;
	CBaseMonster *Victim;
	int m_iSpriteTexture;
	bool insky;

	int m_iTrail;
	int hangtimer;

public:
	static CVargWhip *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(varg_whip, CVargWhip);

CVargWhip *CVargWhip::BoltCreate(void)
{
	// Create a new entity with CVargWhip private data
	CVargWhip *pBolt = GetClassPtr((CVargWhip *)NULL);
	pBolt->pev->classname = MAKE_STRING("vargwhip");
	pBolt->caught = false;
	pBolt->Spawn();

	return pBolt;
}

void CVargWhip::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/w_claw.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-10, -10, -10), Vector(10, 10, 10));
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;

	SetTouch(&CVargWhip::BoltTouch);
	SetThink(&CVargWhip::BoltThink);
	hangtimer = 0;
	pev->nextthink = gpGlobals->time + 0.01;
}


void CVargWhip::Precache()
{
	PRECACHE_MODEL("models/w_claw.mdl");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/rope.spr");
}


int	CVargWhip::Classify(void)
{
	return	CLASS_NONE;
}

void CVargWhip::BoltThink()
{
	entvars_t *pevOwner = NULL;
	if (pev->owner)
		pevOwner = VARS(pev->owner);
	if (!caught)
	{
		m_pBeam = CBeam::BeamCreate("sprites/rope.spr", 15);
		if (!m_pBeam)
			return;

		m_pBeam->PointsInit(pev->origin, pevOwner->origin - gpGlobals->v_right * 28 + gpGlobals->v_forward * 12 + gpGlobals->v_up * 124);
		m_pBeam->SetFlags(0x20);
		m_pBeam->SetTexture(m_iSpriteTexture);
		m_pBeam->LiveForTime(0.02);
		m_pBeam->SetNoise(0);
	}
	else
	{
		m_pBeam = CBeam::BeamCreate("sprites/rope.spr", 15);
		if (!m_pBeam)
			return;

		m_pBeam->PointsInit(pev->origin, Victim->pev->origin + (gpGlobals->v_up * (Victim->pev->size.z * 0.25)));
		m_pBeam->SetFlags(0x20);
		m_pBeam->SetTexture(m_iSpriteTexture);
		m_pBeam->LiveForTime(0.02);
		m_pBeam->SetNoise(0);
		
		UTIL_MakeVectors(pevOwner->angles);
		Vector ownerorigin = pevOwner->origin + gpGlobals->v_forward * 64 + gpGlobals->v_up * 72;
		Victim->pev->velocity = Vector(ownerorigin.x - Victim->pev->origin.x, ownerorigin.y - Victim->pev->origin.y, ownerorigin.z - Victim->pev->origin.z) * 15;
		Victim->pev->health = Victim->pev->health - 0.05;
		hangtimer++;

		if (hangtimer > 65)
		{
			UTIL_Remove(this);
		}
	}
	pev->nextthink = gpGlobals->time + 0.01;
}

void CVargWhip::BoltTouch(CBaseEntity *pOther)
{
	SetTouch(NULL);

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t	*pevOwner;

		pevOwner = VARS(pev->owner);

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		pOther->TraceAttack(pevOwner, 2, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);

		pev->origin = pevOwner->origin + gpGlobals->v_right * 28 + gpGlobals->v_forward * 42 + gpGlobals->v_up * 124;
		pev->movetype = MOVETYPE_FLY;

		CBaseMonster *pVictim = pOther->MyMonsterPointer();
		Schedule_t	*pNewSchedule;

		Victim = pVictim;

		pev->gravity = 0;
		caught = true;

		ApplyMultiDamage(pev, pevOwner);

		pev->velocity = Vector(0, 0, 0);
		// play body "thwack" sound
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM); break;
		}
	}
	else
	{
		SetThink(NULL);
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/xbow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));

		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if (FClassnameIs(pOther->pev, "worldspawn"))
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin(pev, pev->origin - vecDir * 12);
			pev->angles = UTIL_VecToAngles(vecDir);
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector(0, 0, 0);
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0, 360);
			pev->nextthink = gpGlobals->time + 10.0;
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(pev->origin);
		}
	}
}



class CVarg : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	float m_flNextFlinch;
	Vector vecToss;

	int firebombcount;
	float firebombcooldown;

	float jumpcooldown;
	int m_iSpriteTexture;

	void PainSound(void);
	void AlertSound(void);
	void AttackSound(void);

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);

	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_varg, CVarg);

const char *CVarg::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CVarg::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CVarg::pAttackSounds[] =
{
	"junglesci/chargescream.wav",
	"junglesci/chargescream2.wav",
};

const char *CVarg::pAlertSounds[] =
{
	"gonrider/grider_speak5.wav",
	"gonrider/grider_speak6.wav",
	"gonrider/grider_speak2.wav",
};

const char *CVarg::pPainSounds[] =
{
	"scientist/sci_pain1.wav",
	"scientist/sci_pain2.wav",
	"scientist/sci_pain3.wav",
	"scientist/sci_pain4.wav",
	"scientist/sci_pain5.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CVarg::Classify(void)
{
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CVarg::SetYawSpeed(void)
{
	int ys;

	ys = 120;

#if 0
	switch (m_Activity)
	{
	}
#endif

	pev->yaw_speed = ys;
}

int CVarg::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

void CVarg::PainSound(void)
{
	int pitch = 75 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CVarg::AlertSound(void)
{
	int pitch = 75 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CVarg::AttackSound(void)
{
	int pitch = 75 + RANDOM_LONG(0, 9);
	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CVarg::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case VARG_SMACK:
	{
								  CBaseEntity *pHurt = CheckTraceHullAttack(120, 40, DMG_SLASH);
								  if (pHurt)
								  {
									  if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
									  {
										  pHurt->pev->punchangle.x = 5;
										  pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 500 + gpGlobals->v_up * 350;
									  }
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
								  }
								  else
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

								  if (RANDOM_LONG(0, 1))
									  AttackSound();
	}
		break;
	case VARG_WHIP:
	{
					  Vector anglesAim = pev->v_angle + pev->punchangle;
					  UTIL_MakeVectors(anglesAim);
					  anglesAim.x = -anglesAim.x;
					  UTIL_MakeVectors(pev->angles);

					  Vector vecSrc = pev->origin + gpGlobals->v_up * 72;
					  Vector vecDir = gpGlobals->v_forward;

					  CVargWhip *pBolt = CVargWhip::BoltCreate();
					  pBolt->pev->origin = vecSrc;
					  pBolt->pev->angles = anglesAim;
					  pBolt->pev->owner = edict();
					  pBolt->pev->velocity = vecDir * 2000;
					  pBolt->pev->speed = 2000;
					  pBolt->pev->avelocity.z = 10;
					  pBolt->pev->gravity = 0.6;

	}
		break;
	case VARG_THROW:
	{
					   UTIL_MakeVectors(pev->angles);
					   vecToss = vecToss + gpGlobals->v_up * 12;
					   vecToss = vecToss - gpGlobals->v_forward * 30;
					   CGrenade::ShootFirebomb(pev, pev->origin + gpGlobals->v_forward * 64 + Vector(0, 0, 128), vecToss);
					   firebombcount--;
	}
		break;
	case VARG_JUMP:
	{
					  if (gpGlobals->time >= jumpcooldown)
					  {
						  UTIL_MakeVectors(pev->angles);
						  pev->velocity = pev->velocity + gpGlobals->v_up * 150 + gpGlobals->v_forward * 1600;
						  jumpcooldown = gpGlobals->time + 5;
					  }
	}
		break;
	case VARG_LAND:
	{
					  CBaseEntity *pEntity = NULL;
					  while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 150)) != NULL)
					  {
						  if (pEntity->pev->takedamage != DAMAGE_NO)
						  {
							  if (!FClassnameIs(pEntity->pev, "monster_varg"))
							  {
								  pEntity->pev->velocity = pEntity->pev->velocity + gpGlobals->v_up * 350;
								  pEntity->TakeDamage(pev, pev, 30, DMG_SONIC | DMG_ALWAYSGIB);
							  }
						  }
					  }
					  // blast circles
					  MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
					  WRITE_BYTE(TE_BEAMCYLINDER);
					  WRITE_COORD(pev->origin.x);
					  WRITE_COORD(pev->origin.y);
					  WRITE_COORD(pev->origin.z + 32);
					  WRITE_COORD(pev->origin.x);
					  WRITE_COORD(pev->origin.y);
					  WRITE_COORD(pev->origin.z + 32 + 150 / .2); // reach damage radius over .3 seconds
					  WRITE_SHORT(m_iSpriteTexture);
					  WRITE_BYTE(0); // startframe
					  WRITE_BYTE(0); // framerate
					  WRITE_BYTE(5); // life
					  WRITE_BYTE(40);  // width
					  WRITE_BYTE(50);   // noise

					  WRITE_BYTE(255);
					  WRITE_BYTE(100);
					  WRITE_BYTE(100);

					  WRITE_BYTE(225); //brightness
					  WRITE_BYTE(0);		// speed
					  MESSAGE_END();
					  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "houndeye/he_blast1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
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
void CVarg::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/varg.mdl");
	UTIL_SetSize(pev, Vector(-32,-32,0), Vector(32,32,144));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 4000;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.1;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	firebombcount = RANDOM_LONG(1, 4);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CVarg::Precache()
{
	PRECACHE_MODEL("models/varg.mdl");

	PRECACHE_SOUND("gonarch/gon_step1.wav");
	PRECACHE_SOUND("gonarch/gon_step2.wav");
	PRECACHE_SOUND("gonarch/gon_step3.wav");

	PRECACHE_SOUND("weapons/firebomb_break.wav");
	PRECACHE_SOUND("weapons/firebomb_set.wav");
	PRECACHE_SOUND("weapons/firebomb_flame.wav");
	PRECACHE_MODEL("models/firebomb.mdl");
	PRECACHE_MODEL("sprites/firebombflame.spr");

	PRECACHE_SOUND("houndeye/he_blast1.wav");
	PRECACHE_SOUND("houndeye/he_blast2.wav");
	PRECACHE_SOUND("houndeye/he_blast3.wav");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/shockwave.spr");

	UTIL_PrecacheOther("vargwhip");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CVarg::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);

	return iIgnore;
}

void CVarg::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case 10:
	{
			   UTIL_Ricochet(ptr->vecEndPos, 1.0);
			   flDamage = 0.1;
	}
	}
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CVarg::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 400 && flDist > 100 && flDot >= 0.5)
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

BOOL CVarg::CheckMeleeAttack2(float flDot, float flDist)
{
	if (flDist <= 100 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CVarg::CheckRangeAttack1(float flDot, float flDist)
{
	//if (!FBitSet(m_hEnemy->pev->flags, FL_ONGROUND))
	//{
	//	return FALSE;
	//}
	if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 512)
	{
		// don't try to spit at someone up really high or down really low.
		return FALSE;
	}

	if (firebombcount <= 0)
	{
		firebombcount = RANDOM_LONG(1, 4);
		firebombcooldown = gpGlobals->time + 8;
		return FALSE;
	}
	if (gpGlobals->time < firebombcooldown)
	{
		return FALSE;
	}

	if (HasConditions(bits_COND_SEE_ENEMY) && FBitSet(pev->flags, FL_ONGROUND) && flDist > 400 && flDist <= 1800 && flDot >= 0.8)
	{
		vecToss = VecCheckThrow(pev, GetGunPosition() + gpGlobals->v_up * 4 + gpGlobals->v_forward * 34, m_hEnemy->pev->origin, flDist, 0.3); // use dist as speed to get there in 1 secondx
		return TRUE;
	}
	return FALSE;
}

BOOL CVarg::CheckRangeAttack2(float flDot, float flDist)
{
	if (gpGlobals->time < jumpcooldown)
	{
		return FALSE;
	}
	if (flDist <= 1000 && flDist > 400 && flDot >= 0.5)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 80)
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	return FALSE;
}