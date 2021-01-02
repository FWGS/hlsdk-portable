/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"

/*
 * In Opposing Force pitdrone spawned via monstermaker did not have spikes
 * That's probably a bug, because number of spikes is set in level editor,
 * so spawned pitdrones always had 0 spikes.
 * Having no spikes after spawn also prevented spike reloading.
 * Those who want to keep original Opposing Force behavior can set this constant to zero.
 */
#define FEATURE_PITDRONE_SPAWN_WITH_SPIKES 1

// Disable this feature if you don't want to include spike_trail.spr in your mod
#define FEATURE_PITDRONE_SPIKE_TRAIL 1

#if FEATURE_PITDRONE_SPIKE_TRAIL
int		iSpikeTrail;
#endif
int		iPitdroneSpitSprite;
//=========================================================
// CPitDrone's spit projectile
//=========================================================
class CPitdroneSpike : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);
	void EXPORT SpikeTouch(CBaseEntity *pOther);
	void EXPORT StartTrail();
	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles);
};

LINK_ENTITY_TO_CLASS(pitdronespike, CPitdroneSpike)

void CPitdroneSpike::Spawn(void)
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("pitdronespike");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "models/pit_drone_spike.mdl");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));
}

void CPitdroneSpike::Precache(void)
{
	PRECACHE_MODEL("models/pit_drone_spike.mdl");// spit projectile
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
#if FEATURE_PITDRONE_SPIKE_TRAIL
	iSpikeTrail = PRECACHE_MODEL("sprites/spike_trail.spr");
#endif
}

void CPitdroneSpike::SpikeTouch(CBaseEntity *pOther)
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT(115, 125);

	SetTouch(NULL);
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time;

	if (!pOther->pev->takedamage)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/xbow_hit1.wav", 1, ATTN_NORM, 0, iPitch);
		// make a horn in the wall

		if (FClassnameIs(pOther->pev, "worldspawn"))
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin(pev, pev->origin - vecDir * 6.0f);
			pev->angles = UTIL_VecToAngles(vecDir);
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector(0, 0, 0);
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0, 360);
			pev->nextthink = gpGlobals->time + 10.0;
			SetThink(&CBaseEntity::SUB_FadeOut);
		}
	}
	else
	{
		entvars_t	*pevOwner = VARS(pev->owner);
		pOther->TakeDamage(pev, pevOwner, gSkillData.pitdroneDmgSpit, DMG_GENERIC | DMG_NEVERGIB);
		if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM, 0, iPitch);
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM, 0, iPitch);
	}
}

void CPitdroneSpike::StartTrail()
{
#if FEATURE_PITDRONE_SPIKE_TRAIL
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );
		WRITE_SHORT( iSpikeTrail );	// model
		WRITE_BYTE(2); // life
		WRITE_BYTE(1); // width
		WRITE_BYTE(197); // r
		WRITE_BYTE(194); // g
		WRITE_BYTE(11); // b
		WRITE_BYTE(192); //brigtness
	MESSAGE_END();
#endif
	SetTouch(&CPitdroneSpike::SpikeTouch);
}

void CPitdroneSpike::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles)
{
	CPitdroneSpike *pSpit = GetClassPtr( (CPitdroneSpike *)NULL );
	pSpit->Spawn();

	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->angles = vecAngles;
	pSpit->pev->owner = ENT( pevOwner );

	pSpit->SetThink(&CPitdroneSpike::StartTrail);
	pSpit->pev->nextthink = gpGlobals->time;
}

//
// PitDrone, main part.
//
#define HORNGROUP			1
#define PITDRONE_HORNS0		0
#define PITDRONE_HORNS1		1
#define PITDRONE_HORNS2		2
#define PITDRONE_HORNS3		3
#define PITDRONE_HORNS4		4
#define PITDRONE_HORNS5		5
#define PITDRONE_HORNS6		6
#define	PITDRONE_SPRINT_DIST			255
#define PITDRONE_FLINCH_DELAY			2		// at most one flinch every n secs
#define PITDRONE_MAX_HORNS	6
#define PITDRONE_GIB_COUNT	7

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_PDRONE_HURTHOP = LAST_COMMON_SCHEDULE + 1,
	SCHED_PDRONE_SMELLFOOD,
	SCHED_PDRONE_EAT,
	SCHED_PDRONE_SNIFF_AND_EAT,
	SCHED_PDRONE_COVER_AND_RELOAD
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_PDRONE_HOPTURN = LAST_COMMON_TASK + 1
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define PIT_DRONE_AE_SPIT			( 1 )
// not sure what it is. It happens twice when pitdrone uses two claws at the same time
// once before 'throw' event and once after
#define PIT_DRONE_AE_ATTACK			( 2 )
#define PIT_DRONE_AE_SLASH			( 4 )
#define PIT_DRONE_AE_HOP			( 5 )
#define PIT_DRONE_AE_THROW			( 6 )
#define PIT_DRONE_AE_RELOAD			( 7 )

class CPitdrone : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void SetYawSpeed(void);
	int ISoundMask();
	void KeyValue(KeyValueData *pkvd);

	int Classify(void);

	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	void IdleSound(void);
	void PainSound(void);
	void AlertSound(void);
	void DeathSound(void);
	void BodyChange(float spikes);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	int IgnoreConditions(void);
	Schedule_t* GetSchedule(void);
	Schedule_t* GetScheduleOfType(int Type);
	void StartTask(Task_t *pTask);
	void RunTask(Task_t *pTask);
	void RunAI(void);
	void CheckAmmo();
	void GibMonster();
	CUSTOM_SCHEDULES

	virtual int SizeForGrapple() { return GRAPPLE_MEDIUM; }

	float	m_flLastHurtTime;
	float	m_flNextSpitTime;// last time the PitDrone used the spit attack.
	float	m_flNextFlinch;
	int m_iInitialAmmo;
	bool shouldAttackWithLeftClaw;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDieSounds[];
	static const char *pAttackMissSounds[];

	virtual int	Save(CSave &save);
	virtual int	Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS(monster_pitdrone, CPitdrone)

TYPEDESCRIPTION	CPitdrone::m_SaveData[] =
{
	DEFINE_FIELD(CPitdrone, m_iInitialAmmo, FIELD_INTEGER),
	DEFINE_FIELD(CPitdrone, m_flLastHurtTime, FIELD_TIME),
	DEFINE_FIELD(CPitdrone, m_flNextSpitTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CPitdrone, CBaseMonster)

void CPitdrone::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "initammo"))
	{
		m_iInitialAmmo = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

//=========================================================
// IgnoreConditions 
//=========================================================
int CPitdrone::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK2))
	{
			if (m_flNextFlinch >= gpGlobals->time)
				iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + PITDRONE_FLINCH_DELAY;
	}

	return iIgnore;

}

const char *CPitdrone::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CPitdrone::pIdleSounds[] =
{
	"pitdrone/pit_drone_idle1.wav",
	"pitdrone/pit_drone_idle2.wav",
	"pitdrone/pit_drone_idle3.wav",

};

const char *CPitdrone::pAlertSounds[] =
{
	"pitdrone/pit_drone_alert1.wav",
	"pitdrone/pit_drone_alert2.wav",
	"pitdrone/pit_drone_alert3.wav",
};

const char *CPitdrone::pPainSounds[] =
{
	"pitdrone/pit_drone_pain1.wav",
	"pitdrone/pit_drone_pain2.wav",
	"pitdrone/pit_drone_pain3.wav",
	"pitdrone/pit_drone_pain4.wav",
};

const char *CPitdrone::pDieSounds[] =
{
	"pitdrone/pit_drone_die1.wav",
	"pitdrone/pit_drone_die2.wav",
	"pitdrone/pit_drone_die3.wav",
};

//=========================================================
// TakeDamage - overridden for gonome so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CPitdrone::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	float flDist;
	Vector vecApex;

	// if the pitdrone is running, has an enemy, was hurt by the enemy, and isn't too close to the enemy,
	// it will swerve. (whew).
	if (m_hEnemy != 0 && IsMoving() && pevAttacker == m_hEnemy->pev)
	{
		flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();

		if (flDist > PITDRONE_SPRINT_DIST)
		{
			flDist = (pev->origin - m_Route[m_iRouteIndex].vecLocation).Length2D();// reusing flDist. 

			if (FTriangulate(pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5, m_hEnemy, &vecApex))
			{
				InsertWaypoint(vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY);
			}
		}
	}

	m_flLastHurtTime = gpGlobals->time;

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// CheckMeleeAttack1 - attack with both claws at the same time
//=========================================================
BOOL CPitdrone::CheckMeleeAttack1(float flDot, float flDist)
{
	// Give a better chance for MeleeAttack2
	if (RANDOM_LONG(0,2) == 0) {
		return CBaseMonster::CheckMeleeAttack1(flDot, flDist);
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - spike attack
//=========================================================
BOOL CPitdrone::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_cAmmoLoaded <= 0)
	{
		return FALSE;
	}
	if (IsMoving() && flDist >= 512)
	{
		// pitdone will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime)
	{

		if (IsMoving())
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 1;
		}

		return TRUE;
	}

	return FALSE;

}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPitdrone::SetYawSpeed(void)
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case	ACT_WALK:			ys = 120;	break;
	case	ACT_RUN:			ys = 120;	break;
	case	ACT_IDLE:			ys = 120;	break;
	case	ACT_RANGE_ATTACK1:	ys = 120;	break;
	default:
		ys = 120;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CPitdrone::ISoundMask( void )
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_GARBAGE |
		bits_SOUND_PLAYER;
}

void CPitdrone::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case PIT_DRONE_AE_ATTACK:
		break;
	case PIT_DRONE_AE_THROW:
	{
		// SOUND HERE (in the pitdrone model)
		CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.pitdroneDmgWhip, DMG_SLASH );

		if( pHurt )
		{
			// croonchy bite sound
			const int iPitch = RANDOM_FLOAT( 110, 120 );
			switch( RANDOM_LONG( 0, 1 ) )
			{
			case 0:
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_bite2.wav", 0.7, ATTN_NORM, 0, iPitch );
				break;
			case 1:
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_bite3.wav", 0.7, ATTN_NORM, 0, iPitch );
				break;
			}

			// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
			UTIL_ScreenShake( pHurt->pev->origin, 15.0, 1.5, 0.7, 2 );

			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 200;
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
	break;

	case PIT_DRONE_AE_SLASH:
	{
		/* The same event is reused for both right and left claw attacks.
		 * Pitdrone always starts the attack with the right claw so we use shouldAttackWithLeftClaw to check which claw is used now.
		 */
		// SOUND HERE (in the pitdrone model)
		CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.pitdroneDmgBite, DMG_SLASH);
		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->punchangle.z = shouldAttackWithLeftClaw ? 18 : -18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * ( shouldAttackWithLeftClaw ? 100 : -100 );
			}
		}
		else // Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		shouldAttackWithLeftClaw = !shouldAttackWithLeftClaw;
	}
	break;

	case PIT_DRONE_AE_RELOAD:
	{
		if (m_iInitialAmmo >= 0)
			m_cAmmoLoaded = PITDRONE_MAX_HORNS;
		else
			m_cAmmoLoaded = 0;
		BodyChange(m_cAmmoLoaded);
		ClearConditions(bits_COND_NO_AMMO_LOADED);
	}
	break;

	case PIT_DRONE_AE_HOP:
	{
		float flGravity = g_psv_gravity->value;

		// throw the squid up into the air on this frame.
		if( FBitSet( pev->flags, FL_ONGROUND ) )
		{
			pev->flags -= FL_ONGROUND;
		}

		// jump into air for 0.8 (24/30) seconds
		pev->velocity.z += ( 0.625 * flGravity ) * 0.5;
	}
	break;

	case PIT_DRONE_AE_SPIT:
	{
		m_cAmmoLoaded--;
		BodyChange(m_cAmmoLoaded);

		Vector	vecSpitOffset;
		Vector	vecSpitDir;

		UTIL_MakeAimVectors(pev->angles);

		// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
		// we should be able to read the position of bones at runtime for this info.
		vecSpitOffset = (gpGlobals->v_forward * 15 + gpGlobals->v_up * 36);
		vecSpitOffset = (pev->origin + vecSpitOffset);
		//vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();
		Vector vecEnemyPosition;
		if (m_hEnemy != 0)
			vecEnemyPosition = m_hEnemy->BodyTarget(pev->origin);
		else
			vecEnemyPosition = m_vecEnemyLKP;
		vecSpitDir = (vecEnemyPosition - vecSpitOffset).Normalize();

		vecSpitDir.x += RANDOM_FLOAT(-0.01, 0.01);
		vecSpitDir.y += RANDOM_FLOAT(-0.01, 0.01);
		vecSpitDir.z += RANDOM_FLOAT(-0.01, 0);

		// SOUND HERE! (in the pitdrone model)

		CPitdroneSpike::Shoot(pev, vecSpitOffset, vecSpitDir * 900, UTIL_VecToAngles(vecSpitDir));

		// spew the spittle temporary ents.
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpitOffset );
			WRITE_BYTE( TE_SPRITE_SPRAY );
			WRITE_COORD( vecSpitOffset.x );	// pos
			WRITE_COORD( vecSpitOffset.y );
			WRITE_COORD( vecSpitOffset.z );
			WRITE_COORD( vecSpitDir.x );	// dir
			WRITE_COORD( vecSpitDir.y );
			WRITE_COORD( vecSpitDir.z );
			WRITE_SHORT( iPitdroneSpitSprite );	// model
			WRITE_BYTE( 15 );			// count
			WRITE_BYTE( 210 );			// speed
			WRITE_BYTE( 25 );			// noise ( client will divide by 100 )
		MESSAGE_END();
	}
	break;


	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

int	CPitdrone::Classify(void)
{
	return	CLASS_RACEX_PREDATOR;
}

void CPitdrone::BodyChange(float horns)
{
	if (horns <= 0)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS0);
	//		pev->body = PITDRONE_HORNS0;

	if (horns == 1)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS6);
	//		pev->body = PITDRONE_HORNS6;

	if (horns == 2)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS5);
	//		pev->body = PITDRONE_HORNS5;

	if (horns == 3)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS4);
	//		pev->body = PITDRONE_HORNS4;

	if (horns == 4)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS3);
	//		pev->body = PITDRONE_HORNS3;

	if (horns == 5)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS2);
	//		pev->body = PITDRONE_HORNS2;

	if (horns >= 6)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS1);
	//		pev->body = PITDRONE_HORNS1;

	return;
}
//=========================================================
// Spawn
//=========================================================
void CPitdrone::Spawn()
{
	Precache();

	SET_MODEL( ENT(pev), "models/pit_drone.mdl" );
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 48));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->effects = 0;
	pev->health = gSkillData.pitdroneHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_flNextSpitTime = gpGlobals->time;

	if (m_iInitialAmmo >= 0)
	{
		m_cAmmoLoaded = Q_min(m_iInitialAmmo, PITDRONE_MAX_HORNS);
#if FEATURE_PITDRONE_SPAWN_WITH_SPIKES
		if (!m_cAmmoLoaded) {
			m_cAmmoLoaded = PITDRONE_MAX_HORNS;
		}
#endif
	}
	BodyChange(m_cAmmoLoaded);
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPitdrone::Precache()
{
	PRECACHE_MODEL("models/pit_drone.mdl");
	PRECACHE_MODEL("models/pit_drone_gibs.mdl");
	iPitdroneSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pDieSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);

	PRECACHE_SOUND("bullchicken/bc_bite2.wav");
	PRECACHE_SOUND("bullchicken/bc_bite3.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_melee_attack1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_melee_attack2.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_attack_spike1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_attack_spike2.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_communicate1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate3.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate4.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_eat.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_hunt1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_hunt2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_hunt3.wav");

	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");

	UTIL_PrecacheOther("pitdronespike");
}


//=========================================================
// IdleSound
//=========================================================
#define PITDRONE_ATTN_IDLE	(float)1.5
void CPitdrone::IdleSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, pIdleSounds[RANDOM_LONG(0, ARRAYSIZE(pIdleSounds)-1)], 1, PITDRONE_ATTN_IDLE);
}

//=========================================================
// PainSound
//=========================================================
void CPitdrone::PainSound(void)
{
	int iPitch = RANDOM_LONG(85, 120);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds)-1)], 1, ATTN_NORM, 0, iPitch);
}

//=========================================================
// AlertSound
//=========================================================
void CPitdrone::AlertSound(void)
{
	int iPitch = RANDOM_LONG(140, 160);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds)-1)], 1, ATTN_NORM, 0, iPitch);
}
//=========================================================
// DeathSound
//=========================================================
void CPitdrone::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, pDieSounds[RANDOM_LONG(0, ARRAYSIZE(pDieSounds)-1)], 1, ATTN_NORM);
}

void CPitdrone::RunAI(void)
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	if (m_hEnemy != 0 && m_Activity == ACT_RUN)
	{
		// chasing enemy. Sprint for last bit
		if ((pev->origin - m_hEnemy->pev->origin).Length2D() < PITDRONE_SPRINT_DIST)
		{
			pev->framerate = 1.25;
		}
	}
}

void CPitdrone::CheckAmmo( void )
{
	if( m_cAmmoLoaded <= 0 && m_iInitialAmmo >= 0 )
	{
		SetConditions( bits_COND_NO_AMMO_LOADED );
	}
}

void CPitdrone::GibMonster()
{
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM );

	if( CVAR_GET_FLOAT( "violence_agibs" ) != 0 )	// Should never get here, but someone might call it directly
	{
		CGib::SpawnRandomGibs( pev, 5, "models/pit_drone_gibs.mdl", PITDRONE_GIB_COUNT );	// Throw alien gibs
	}
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlPDroneRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slPDroneRangeAttack1[] =
{
	{
		tlPDroneRangeAttack1,
		ARRAYSIZE(tlPDroneRangeAttack1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"PDrone Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlPDroneChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },// !!!OEM - this will stop nasty PitDrone oscillation.
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slPDroneChaseEnemy[] =
{
	{
		tlPDroneChaseEnemy1,
		ARRAYSIZE(tlPDroneChaseEnemy1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_SMELL_FOOD |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_TASK_FAILED |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER |
		bits_SOUND_MEAT,
		"PDrone Chase Enemy"
	},
};

Task_t tlPDroneHurtHop[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SOUND_WAKE, (float)0 },
	{ TASK_PDRONE_HOPTURN, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },// in case squid didn't turn all the way in the air.
};

Schedule_t slPDroneHurtHop[] =
{
	{
		tlPDroneHurtHop,
		ARRAYSIZE( tlPDroneHurtHop ),
		0,
		0,
		"PDroneHurtHop"
	}
};


// PitDrone walks to something tasty and eats it.
Task_t tlPDroneEat[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the PitDrone can't get to the food
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_EAT, (float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slPDroneEat[] =
{
	{
		tlPDroneEat,
		ARRAYSIZE(tlPDroneEat),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"PDroneEat"
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the PitDrone. This schedule plays a sniff animation before going to the source of food.
Task_t tlPDroneSniffAndEat[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the PitDrone can't get to the food
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_EAT, (float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slPDroneSniffAndEat[] =
{
	{
		tlPDroneSniffAndEat,
		ARRAYSIZE(tlPDroneSniffAndEat),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"PDroneSniffAndEat"
	}
};

Task_t	tlPDroneHideReload[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RELOAD },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_RELOAD },
};

Schedule_t slPDroneHideReload[] =
{
	{
		tlPDroneHideReload,
		ARRAYSIZE( tlPDroneHideReload ),
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"PDroneHideReload"
	}
};

DEFINE_CUSTOM_SCHEDULES(CPitdrone)
{
	slPDroneRangeAttack1,
	slPDroneChaseEnemy,
	slPDroneHurtHop,
	slPDroneEat,
	slPDroneSniffAndEat,
	slPDroneHideReload
};

IMPLEMENT_CUSTOM_SCHEDULES(CPitdrone, CBaseMonster)

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CPitdrone::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	{
		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			return GetScheduleOfType( SCHED_PDRONE_HURTHOP );
		}

		if (HasConditions(bits_COND_SMELL_FOOD))
		{
			CSound		*pSound;

			pSound = PBestScent();

			if (pSound && (!FInViewCone(&pSound->m_vecOrigin) || !FVisible(pSound->m_vecOrigin)))
			{
				// scent is behind or occluded
				return GetScheduleOfType(SCHED_PDRONE_SNIFF_AND_EAT);
			}

			// food is right out in the open. Just go get it.
			return GetScheduleOfType(SCHED_PDRONE_EAT);
		}

		break;
	}
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		if (HasConditions(bits_COND_SMELL_FOOD))
		{
			CSound		*pSound;

			pSound = PBestScent();

			if (pSound && (!FInViewCone(&pSound->m_vecOrigin) || !FVisible(pSound->m_vecOrigin)))
			{
				// scent is behind or occluded
				return GetScheduleOfType(SCHED_PDRONE_SNIFF_AND_EAT);
			}

			// food is right out in the open. Just go get it.
			return GetScheduleOfType(SCHED_PDRONE_EAT);
		}

		if( HasConditions( bits_COND_NO_AMMO_LOADED ) && (m_iInitialAmmo >= 0) )
		{
			return GetScheduleOfType( SCHED_PDRONE_COVER_AND_RELOAD );
		}

		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK2))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK2);
		}

		return GetScheduleOfType(SCHED_CHASE_ENEMY);

		break;
	}
	}

	return CBaseMonster::GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CPitdrone::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slPDroneRangeAttack1[0];
		break;
	case SCHED_PDRONE_HURTHOP:
		return &slPDroneHurtHop[0];
		break;
	case SCHED_PDRONE_EAT:
		return &slPDroneEat[0];
		break;
	case SCHED_PDRONE_SNIFF_AND_EAT:
		return &slPDroneSniffAndEat[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slPDroneChaseEnemy[0];
		break;
	case SCHED_PDRONE_COVER_AND_RELOAD:
		return &slPDroneHideReload[0];
		break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for PitDrone because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CPitdrone::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_PDRONE_HOPTURN:
	{
		SetActivity( ACT_HOP );
		MakeIdealYaw( m_vecEnemyLKP );
		break;
	}
	case TASK_GET_PATH_TO_ENEMY:
	{
		if (BuildRoute(m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy))
		{
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		}
		else
		{
			ALERT(at_aiconsole, "GetPathToEnemy failed!!\n");
			TaskFail();
		}
		break;
	}
	default:
	{
		CBaseMonster::StartTask(pTask);
		break;
	}
	}
}

//=========================================================
// RunTask
//=========================================================
void CPitdrone::RunTask(Task_t *pTask)
{
	switch( pTask->iTask )
	{
	case TASK_PDRONE_HOPTURN:
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );

			if( m_fSequenceFinished )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CBaseMonster::RunTask( pTask );
			break;
		}
	}
}
