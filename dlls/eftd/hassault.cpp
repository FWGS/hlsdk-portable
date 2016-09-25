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
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"effects.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HASSAULT_AE_BURST1		( 1 )
#define		HASSAULT_AE_BURST2		( 2 )
#define		HASSAULT_AE_BURST3		( 3 )
#define		HASSAULT_AE_BURST4		( 4 )
#define		HASSAULT_AE_BURST5		( 5 )
#define		HASSAULT_AE_BURST6		( 6 )
#define		HASSAULT_AE_BURST7		( 7 )
#define		HASSAULT_AE_BURST8		( 8 )
#define		HASSAULT_AE_BURST9		( 9 )
#define		HASSAULT_AE_PAIN1		( 10 )
#define		HASSAULT_AE_PAIN2		( 11 )

class CHassault : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int Classify(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);

	void DeathSound(void);

	BOOL CheckMeleeAttack1(float flDot, float flDist) { return FALSE; }
	BOOL CheckMeleeAttack2(float flDot, float flDist) { return FALSE; }
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }

	void Shoot(void);

	static const char *pDeathSounds[];
	static const char *pPainSounds[];
	static const char *pBurstSounds[];

private:
	int		m_iShell;
};

LINK_ENTITY_TO_CLASS(monster_hassault, CHassault);

const char *CHassault::pPainSounds[] =
{
	"hgrunt/gr_pain1.wav",
	"hgrunt/gr_pain2.wav",
	"hgrunt/gr_pain3.wav",
	"hgrunt/gr_pain4.wav",
	"hgrunt/gr_pain5.wav",
};

const char *CHassault::pDeathSounds[] =
{
	"hgrunt/gr_die1.wav",
	"hgrunt/gr_die2.wav",
	"hgrunt/gr_die3.wav",
};

const char *CHassault::pBurstSounds[] =
{
	"hassault/hw_shoot1.wav",
	"hassault/hw_shoot2.wav",
	"hassault/hw_shoot3.wav",
};

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHassault::SetYawSpeed(void)
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
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
void CHassault::HandleAnimEvent(MonsterEvent_t *pEvent)
{	
	switch (pEvent->event)
	{
	case HASSAULT_AE_BURST1:
	case HASSAULT_AE_BURST2:
	case HASSAULT_AE_BURST3:
	case HASSAULT_AE_BURST4:
	case HASSAULT_AE_BURST5:
	case HASSAULT_AE_BURST6:
	case HASSAULT_AE_BURST7:
	case HASSAULT_AE_BURST8:
	case HASSAULT_AE_BURST9:

		Shoot();
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBurstSounds), 1, ATTN_NORM);
	
		break;

	case HASSAULT_AE_PAIN1:
	case HASSAULT_AE_PAIN2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM);
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}


//=========================================================
// Spawn
//=========================================================
void CHassault::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/hassault.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.hassaultHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_HackedGunPos = Vector(0, 0, 55);

	m_cAmmoLoaded = 1;
	ClearConditions(bits_COND_NO_AMMO_LOADED);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHassault::Precache()
{
	PRECACHE_MODEL("models/hassault.mdl");

	PRECACHE_SOUND("hassault/hw_gun4.wav");

	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pBurstSounds);

	PRECACHE_SOUND("hassault/hw_spin.wav");
	PRECACHE_SOUND("hassault/hw_spindown.wav");
	PRECACHE_SOUND("hassault/hw_spinup.wav");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CHassault::Classify(void)
{
	return CLASS_HUMAN_MILITARY;
}

//=========================================================
// Shoot
//=========================================================
void CHassault::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_MONSTER_HVMG); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	// m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}


//=========================================================
// DeathSound 
//=========================================================
void CHassault::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1, ATTN_IDLE);
}