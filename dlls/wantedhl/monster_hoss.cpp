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
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"
#include	"weapons.h"
#include	"barney.h"



#define	HOSS_BODY_GUNHOLSTERED		0
#define	HOSS_BODY_GUNDRAWN			1
#define HOSS_BODY_GUNGONE			2

//=======================================================
// Hoss
//=======================================================

class CHoss : public CBarney
{
public:
	void Spawn(void);
	void Precache(void);

	void BarneyFirePistol(void);
	void AlertSound(void);

	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void DeclineFollowing(void);

	Schedule_t *GetSchedule(void);

	void DeathSound(void);
	void PainSound(void);

	void TalkInit(void);
	void Killed(entvars_t *pevAttacker, int iGib);
};

LINK_ENTITY_TO_CLASS(monster_hoss, CHoss);

//=========================================================
// ALertSound - hoss says "Freeze!"
//=========================================================
void CHoss::AlertSound(void)
{
	if (m_hEnemy != NULL)
	{
		if (FOkToSpeak())
		{
			PlaySentence("HO_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
		}
	}

}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy hoss is facing.
//=========================================================
void CHoss::BarneyFirePistol(void)
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hoss/ho_attack2.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}



//=========================================================
// Spawn
//=========================================================
void CHoss::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/hoss.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.barneyHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	pev->body = 0; // gun in holster
	m_fGunDrawn = FALSE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse(&CHoss::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHoss::Precache()
{
	PRECACHE_MODEL("models/hoss.mdl");

	PRECACHE_SOUND("hoss/ho_attack1.wav");
	PRECACHE_SOUND("hoss/ho_attack2.wav");

	PRECACHE_SOUND("hoss/ho_pain1.wav");
	PRECACHE_SOUND("hoss/ho_pain2.wav");
	PRECACHE_SOUND("hoss/ho_pain3.wav");

	PRECACHE_SOUND("hoss/ho_die1.wav");
	PRECACHE_SOUND("hoss/ho_die2.wav");
	PRECACHE_SOUND("hoss/ho_die3.wav");

	// every new hoss must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

// Init talk data
void CHoss::TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]		= "HO_ANSWER";
	m_szGrp[TLK_QUESTION]	= "HO_QUESTION";
	m_szGrp[TLK_IDLE]		= "HO_IDLE";
	m_szGrp[TLK_STARE]		= "HO_STARE";
	m_szGrp[TLK_USE]		= "HO_OK";
	m_szGrp[TLK_UNUSE]		= "HO_WAIT";
	m_szGrp[TLK_STOP]		= "HO_STOP";

	m_szGrp[TLK_NOSHOOT]	= "HO_SCARED";
	m_szGrp[TLK_HELLO]		= "HO_HELLO";

	m_szGrp[TLK_PLHURT1]	= "!HO_CUREA";
	m_szGrp[TLK_PLHURT2]	= "!HO_CUREB";
	m_szGrp[TLK_PLHURT3]	= "!HO_CUREC";

	m_szGrp[TLK_PHELLO]		= NULL;	//"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE]		= NULL;	//"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION]	= "HO_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL]		= "HO_SMELL";

	m_szGrp[TLK_WOUND]		= "HO_WOUND";
	m_szGrp[TLK_MORTAL]		= "HO_MORTAL";

	// get voice for head - just one hoss voice for now
	m_voicePitch = 100;
}


static BOOL IsFacing(entvars_t *pevTest, const Vector &reference)
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate(angle, forward, NULL, NULL);
	// He's facing me, he meant it
	if (DotProduct(forward, vecDir) > 0.96)	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


int CHoss::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT))
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy == NULL)
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing(pevAttacker, pev->origin))
			{
				// Alright, now I'm pissed!
				PlaySentence("HO_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(TRUE);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("HO_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
		else if (!(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO)
		{
			PlaySentence("HO_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}


//=========================================================
// PainSound
//=========================================================
void CHoss::PainSound(void)
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hoss/ho_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hoss/ho_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hoss/ho_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CHoss::DeathSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hoss/ho_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hoss/ho_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "hoss/ho_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

void CHoss::Killed(entvars_t *pevAttacker, int iGib)
{
	if (pev->body < HOSS_BODY_GUNGONE)
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		pev->body = HOSS_BODY_GUNGONE;

		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity *pGun = DropItem("weapon_pistol", vecGunPos, vecGunAngles);
	}

	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CHoss::GetSchedule(void)
{
	if (HasConditions(bits_COND_ENEMY_DEAD) && FOkToSpeak())
	{
		PlaySentence("HO_KILL", 4, VOL_NORM, ATTN_NORM);
	}

	return CBarney::GetSchedule();
}


void CHoss::DeclineFollowing(void)
{
	PlaySentence("HO_POK", 2, VOL_NORM, ATTN_NORM);
}



//=========================================================
// Dead Hoss PROP
//=========================================================
class CDeadHoss : public CDeadBarney
{
public:
	void Spawn(void);
	static char *m_szPoses[3];
};

char *CDeadHoss::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

LINK_ENTITY_TO_CLASS(monster_hoss_dead, CDeadHoss);

//=========================================================
// ********** DeadHoss SPAWN **********
//=========================================================
void CDeadHoss::Spawn()
{
	PRECACHE_MODEL("models/hoss.mdl");
	SET_MODEL(ENT(pev), "models/hoss.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead hoss with bad pose\n");
	}
	// Corpses have less health
	pev->health = 8;//gSkillData.barneyHealth;

	MonsterInitDead();
}