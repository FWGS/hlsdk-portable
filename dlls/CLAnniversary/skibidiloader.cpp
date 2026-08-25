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
#include	"weapons.h"
#include	"schedule.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define SKIBIDI_SHOOT	3
#define SKIBIDI_DIE	4

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs

class CSkibidiLoader : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Shoot(void);

	float m_flNextFlinch;

	int		m_iShell;
	int iSkibidiFlash;
	int pGibName;

	void PainSound(void);
	void AlertSound(void);
	void IdleSound(void);
	void AttackSound(void);
	void DeathSound(void);

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_skibidiloader, CSkibidiLoader);

const char *CSkibidiLoader::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CSkibidiLoader::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CSkibidiLoader::pAttackSounds[] =
{
	"skibidiloader/sl_alert1.wav",
	"skibidiloader/sl_alert2.wav",
};

const char *CSkibidiLoader::pIdleSounds[] =
{
	"skibidiloader/sl_chatter1.wav",
	"skibidiloader/sl_chatter2.wav",
	"skibidiloader/sl_chatter3.wav",
	"skibidiloader/sl_chatter4.wav",
	"skibidiloader/sl_chatter5.wav",
};

const char *CSkibidiLoader::pAlertSounds[] =
{
	"skibidiloader/sl_alert1.wav",
	"skibidiloader/sl_alert2.wav",
};

const char *CSkibidiLoader::pPainSounds[] =
{
	"skibidiloader/sl_die1.wav",
	"skibidiloader/sl_die2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSkibidiLoader::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSkibidiLoader::SetYawSpeed(void)
{
	int ys;

	ys = 60;

#if 0
	switch (m_Activity)
	{
	}
#endif

	pev->yaw_speed = ys;
}

int CSkibidiLoader::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

void CSkibidiLoader::PainSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CSkibidiLoader::AlertSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CSkibidiLoader::IdleSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	// Play a random idle sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pIdleSounds[RANDOM_LONG(0, ARRAYSIZE(pIdleSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}

void CSkibidiLoader::AttackSound(void)
{
	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}

void CSkibidiLoader::DeathSound(void)
{
	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}



//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSkibidiLoader::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case ZOMBIE_AE_ATTACK_RIGHT:
	{
								   // do stuff for this event.
								   //		ALERT( at_console, "Slash right!\n" );
								   CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgOneSlash, DMG_SLASH);
								   if (pHurt)
								   {
									   if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
									   {
										   pHurt->pev->punchangle.z = -18;
										   pHurt->pev->punchangle.x = 5;
										   pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
									   }
									   // Play a random attack hit sound
									   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
								   }
								   else // Play a random attack miss sound
									   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

								   if (RANDOM_LONG(0, 1))
									   AttackSound();
	}
		break;

	case ZOMBIE_AE_ATTACK_LEFT:
	{
								  // do stuff for this event.
								  //		ALERT( at_console, "Slash left!\n" );
								  CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgOneSlash, DMG_SLASH);
								  if (pHurt)
								  {
									  if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
									  {
										  pHurt->pev->punchangle.z = 18;
										  pHurt->pev->punchangle.x = 5;
										  pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
									  }
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
								  }
								  else
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

								  if (RANDOM_LONG(0, 1))
									  AttackSound();
	}
		break;

	case SKIBIDI_SHOOT:
	{
		Shoot();
	}
		break;
	case SKIBIDI_DIE:
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal1.wav", 1.0, ATTN_NORM, 0, 100);
			break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal2.wav", 1.0, ATTN_NORM, 0, 100);
			break;
		}
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_BREAKMODEL);

		// position
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);

		// size
		WRITE_COORD(pev->size.x * 2);
		WRITE_COORD(pev->size.y * 2);
		WRITE_COORD(pev->size.z * 2);

		// velocity
		WRITE_COORD(pev->velocity.x);
		WRITE_COORD(pev->velocity.y);
		WRITE_COORD(pev->velocity.z);

		// randomization
		WRITE_BYTE(25);

		// Model
		WRITE_SHORT(pGibName);	//model id#

		// # of shards
		WRITE_BYTE(0);	// let client decide

		// duration
		WRITE_BYTE(25);// 2.5 seconds

		// flags
		WRITE_BYTE(BREAK_METAL);
		MESSAGE_END();

		UTIL_MakeVectors(pev->angles);



		CGrenade::ShootContact(pev, GetGunPosition(), gpGlobals->v_up * -1);


		CGib::SpawnRandomGibs(pev, 6, 1);
		UTIL_Remove(this);
	}

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CSkibidiLoader::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/skibidiloader.mdl");
	UTIL_SetSize(pev, Vector(-80, -80, 0), Vector(80, 80, 172));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->health = gSkillData.zombieHealth * 10;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.1;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSkibidiLoader::Precache()
{
	PRECACHE_MODEL("models/skibidiloader.mdl");

	PRECACHE_SOUND("ambience/loader_hydra1.wav");
	PRECACHE_SOUND("ambience/loader_step1.wav");
	PRECACHE_SOUND("turret/tu_fire1.wav");
	PRECACHE_SOUND("debris/bustmetal1.wav");
	PRECACHE_SOUND("debris/bustmetal2.wav");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");
	iSkibidiFlash = PRECACHE_MODEL("sprites/muz8.spr");
	pGibName = PRECACHE_MODEL("models/computergibs.mdl");
	
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CSkibidiLoader::IgnoreConditions(void)
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

void CSkibidiLoader::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{

	if (!IsAlive())
	{
		CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
		return;
	}

	if (pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0, 100) < 20))
	{
		UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(0.5, 1.5));
		pev->dmgtime = gpGlobals->time;
		//			if ( RANDOM_LONG(0,100) < 25 )
		//				EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, pRicSounds[ RANDOM_LONG(0,ARRAYSIZE(pRicSounds)-1) ], 1.0, ATTN_NORM, 0, PITCH_NORM );
	}

	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);

}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CSkibidiLoader::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDot >= 0.5) //&& gpGlobals->time >= m_flNextShootTime
	{
		return TRUE;
	}

	return FALSE;
}

void CSkibidiLoader::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = pev->origin + gpGlobals->v_up * 30 + gpGlobals->v_forward * 52;
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_9MM, 1); // shoot +-7.5 degrees
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1.0, ATTN_NORM, 0, 100);

	pev->effects = pev->effects | EF_MUZZLEFLASH;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecShootOrigin);
	WRITE_BYTE(TE_SPRITE);
	WRITE_COORD(vecShootOrigin.x);	// pos
	WRITE_COORD(vecShootOrigin.y);
	WRITE_COORD(vecShootOrigin.z);
	WRITE_SHORT(iSkibidiFlash);		// model
	WRITE_BYTE(6);				// size * 10
	WRITE_BYTE(128);			// brightness
	MESSAGE_END();

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHOTSHELL);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	//m_flNextShootTime = gpGlobals->time + 1;
}