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
//=========================================================
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"barney.h"

#define		NUM_OTIS_HEADS		2 // heads available for otis model

#define		GUN_GROUP			1
#define		HEAD_GROUP			2

#define		HEAD_HAIR			0
#define		HEAD_BALD			1

#define		GUN_NONE			0
#define		GUN_EAGLE			1
#define		GUN_DONUT			2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is Otis dying for scripted sequences?
#define		OTIS_AE_DRAW		( 2 )
#define		OTIS_AE_SHOOT		( 3 )
#define		OTIS_AE_HOLSTER		( 4 )

#define OTIS_BODY_GUNHOLSTERED	0
#define OTIS_BODY_GUNDRAWN		1
#define OTIS_BODY_DONUT			2

class COtis : public CBarney
{
public:
#if 1
	void KeyValue(KeyValueData *pkvd);
#endif

	void Spawn(void);
	void Precache(void);
	void BarneyFirePistol(void);
	void AlertSound(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);

	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void DeclineFollowing(void);

	// Override these to set behavior
	Schedule_t *GetSchedule(void);

	void TalkInit(void);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed(entvars_t *pevAttacker, int iGib);

	int		head;
	int		bodystate;
};

LINK_ENTITY_TO_CLASS(monster_otis, COtis);

//=========================================================
// ALertSound - otis says "Freeze!"
//=========================================================
void COtis::AlertSound(void)
{
	if (m_hEnemy != NULL)
	{
		if (FOkToSpeak())
		{
			PlaySentence("OT_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
		}
	}
}



//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy otis is facing.
//=========================================================
void COtis::BarneyFirePistol(void)
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_357);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/desert_eagle_fire.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void COtis::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case OTIS_AE_DRAW:
		// otis' bodygroup switches here so he can pull gun from holster
		// pev->body = OTIS_BODY_GUNDRAWN;
		SetBodygroup( GUN_GROUP, GUN_EAGLE );
		m_fGunDrawn = TRUE;
		break;

	case OTIS_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		// pev->body = OTIS_BODY_GUNHOLSTERED;
		SetBodygroup( GUN_GROUP, GUN_NONE );
		m_fGunDrawn = FALSE;
		break;

	default:
		CBarney::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void COtis::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/otis.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.otisHealth;
	pev->view_ofs		= Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->body = 0; // gun in holster
	m_fGunDrawn = FALSE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	// Make sure hands are white.
	pev->skin = 0;

	// Select a random head.
	if (head == -1)
	{
		SetBodygroup(HEAD_GROUP, RANDOM_LONG(0, NUM_OTIS_HEADS - 1));
	}
	else
	{
		SetBodygroup(HEAD_GROUP, head);
	}

	if (bodystate == -1)
	{
		SetBodygroup(GUN_GROUP, RANDOM_LONG(OTIS_BODY_GUNHOLSTERED, OTIS_BODY_GUNDRAWN)); // don't random donut
	}
	else
	{
		SetBodygroup(GUN_GROUP, bodystate);
	}

	MonsterInit();
	SetUse(&COtis::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COtis::Precache()
{
	PRECACHE_MODEL("models/otis.mdl");

	PRECACHE_SOUND("barney/desert_eagle_fire.wav");

	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");

	// every new otis must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

// Init talk data
void COtis::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]		= "OT_ANSWER";
	m_szGrp[TLK_QUESTION]	= "OT_QUESTION";
	m_szGrp[TLK_IDLE]		= "OT_IDLE";
	m_szGrp[TLK_STARE]		= "OT_STARE";
	m_szGrp[TLK_USE]		= "OT_OK";
	m_szGrp[TLK_UNUSE]		= "OT_WAIT";
	m_szGrp[TLK_STOP]		= "OT_STOP";

	m_szGrp[TLK_NOSHOOT]	= "OT_SCARED";
	m_szGrp[TLK_HELLO]		= "OT_HELLO";

	m_szGrp[TLK_PLHURT1]	= "!OT_CUREA";
	m_szGrp[TLK_PLHURT2]	= "!OT_CUREB";
	m_szGrp[TLK_PLHURT3]	= "!OT_CUREC";

	m_szGrp[TLK_PHELLO]		= NULL;
	m_szGrp[TLK_PIDLE]		= NULL;	
	m_szGrp[TLK_PQUESTION]	= NULL;

	m_szGrp[TLK_SMELL]		= "OT_SMELL";

	m_szGrp[TLK_WOUND]		= "OT_WOUND";
	m_szGrp[TLK_MORTAL]		= "OT_MORTAL";

	// get voice for head - just one otis voice for now
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


int COtis::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
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
				PlaySentence("OT_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(TRUE);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("OT_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
		else if (!(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO)
		{
			PlaySentence("OT_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}

void COtis::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10: // Otis wears no helmet, so do not prevent taking headshot damage.
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	default:
		break;
	}
	CTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


void COtis::Killed(entvars_t *pevAttacker, int iGib)
{
	if (GetBodygroup(GUN_GROUP) != OTIS_BODY_GUNHOLSTERED)
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup(GUN_GROUP, OTIS_BODY_GUNHOLSTERED);

		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity *pGun = DropItem("weapon_eagle", vecGunPos, vecGunAngles);
	}

	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *COtis::GetSchedule(void)
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}

	if (HasConditions(bits_COND_ENEMY_DEAD) && FOkToSpeak())
	{
		PlaySentence("OT_KILL", 4, VOL_NORM, ATTN_NORM);
	}

	return CBarney::GetSchedule();
}


void COtis::DeclineFollowing(void)
{
	PlaySentence("OT_POK", 2, VOL_NORM, ATTN_NORM);
}

void COtis::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		head = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBarney::KeyValue(pkvd);
}




//=========================================================
// DEAD OTIS PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadOtis : public CBaseMonster
{
public:
	void Spawn(void);
	int	Classify(void) { return	CLASS_PLAYER_ALLY; }

	void KeyValue(KeyValueData *pkvd);

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[5];
};

char *CDeadOtis::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach", "stuffed_in_vent", "dead_sitting" };

void CDeadOtis::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_otis_dead, CDeadOtis);

//=========================================================
// ********** DeadOtis SPAWN **********
//=========================================================
void CDeadOtis::Spawn()
{
	PRECACHE_MODEL("models/otis.mdl");
	SET_MODEL(ENT(pev), "models/otis.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead otis with bad pose\n");
	}
	// Corpses have less health
	pev->health = 8;//gSkillData.otisHealth;

	MonsterInitDead();
}
