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



#define	ANNIE_BODY_GUNHOLSTERED		0
#define	ANNIE_BODY_GUNDRAWN			1
#define ANNIE_BODY_GUNGONE			2

//=======================================================
// Annie
//=======================================================

class CAnnie : public CBarney
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

LINK_ENTITY_TO_CLASS(monster_annie, CAnnie);

//=========================================================
// ALertSound - annie says "Freeze!"
//=========================================================
void CAnnie::AlertSound(void)
{
	if (m_hEnemy != NULL)
	{
		if (FOkToSpeak())
		{
			PlaySentence("AN_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
		}
	}

}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy annie is facing.
//=========================================================
void CAnnie::BarneyFirePistol(void)
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
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "annie/an_attack2.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}



//=========================================================
// Spawn
//=========================================================
void CAnnie::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/annie.mdl");
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
	SetUse(&CAnnie::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CAnnie::Precache()
{
	PRECACHE_MODEL("models/annie.mdl");

	PRECACHE_SOUND("annie/an_attack1.wav");
	PRECACHE_SOUND("annie/an_attack2.wav");

	PRECACHE_SOUND("annie/an_pain1.wav");
	PRECACHE_SOUND("annie/an_pain2.wav");
	PRECACHE_SOUND("annie/an_pain3.wav");

	PRECACHE_SOUND("annie/an_die1.wav");
	PRECACHE_SOUND("annie/an_die2.wav");
	PRECACHE_SOUND("annie/an_die3.wav");

	// every new annie must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

// Init talk data
void CAnnie::TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]		= "AN_ANSWER";
	m_szGrp[TLK_QUESTION]	= "AN_QUESTION";
	m_szGrp[TLK_IDLE]		= "AN_IDLE";
	m_szGrp[TLK_STARE]		= "AN_STARE";
	m_szGrp[TLK_USE]		= "AN_OK";
	m_szGrp[TLK_UNUSE]		= "AN_WAIT";
	m_szGrp[TLK_STOP]		= "AN_STOP";

	m_szGrp[TLK_NOSHOOT]	= "AN_SCARED";
	m_szGrp[TLK_HELLO]		= "AN_HELLO";

	m_szGrp[TLK_PLHURT1]	= "!AN_CUREA";
	m_szGrp[TLK_PLHURT2]	= "!AN_CUREB";
	m_szGrp[TLK_PLHURT3]	= "!AN_CUREC";

	m_szGrp[TLK_PHELLO]		= NULL;	//"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE]		= NULL;	//"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION]	= "AN_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL]		= "AN_SMELL";

	m_szGrp[TLK_WOUND]		= "AN_WOUND";
	m_szGrp[TLK_MORTAL]		= "AN_MORTAL";

	// get voice for head - just one annie voice for now
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


int CAnnie::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
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
				PlaySentence("AN_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(TRUE);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("AN_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
		else if (!(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO)
		{
			PlaySentence("AN_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}


//=========================================================
// PainSound
//=========================================================
void CAnnie::PainSound(void)
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "annie/an_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "annie/an_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "annie/an_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CAnnie::DeathSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "annie/an_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "annie/an_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "annie/an_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

void CAnnie::Killed(entvars_t *pevAttacker, int iGib)
{
	if (pev->body < ANNIE_BODY_GUNGONE)
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		pev->body = ANNIE_BODY_GUNGONE;

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
Schedule_t *CAnnie::GetSchedule(void)
{
	if (HasConditions(bits_COND_ENEMY_DEAD) && FOkToSpeak())
	{
		PlaySentence("AN_KILL", 4, VOL_NORM, ATTN_NORM);
	}

	return CBarney::GetSchedule();
}


void CAnnie::DeclineFollowing(void)
{
	PlaySentence("AN_POK", 2, VOL_NORM, ATTN_NORM);
}



//=========================================================
// Dead Annie PROP
//=========================================================
class CDeadAnnie : public CDeadBarney
{
public:
	void Spawn(void);
	static char *m_szPoses[3];
};

char *CDeadAnnie::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

LINK_ENTITY_TO_CLASS(monster_annie_dead, CDeadAnnie);

//=========================================================
// ********** DeadAnnie SPAWN **********
//=========================================================
void CDeadAnnie::Spawn()
{
	PRECACHE_MODEL("models/annie.mdl");
	SET_MODEL(ENT(pev), "models/annie.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead annie with bad pose\n");
	}
	// Corpses have less health
	pev->health = 8;//gSkillData.barneyHealth;

	MonsterInitDead();
}