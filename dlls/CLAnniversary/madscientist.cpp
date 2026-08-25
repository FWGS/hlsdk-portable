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
// human scientist (passive lab worker)
//=========================================================

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


#define		NUM_SCIENTIST_HEADS		10 // four heads available for scientist model
enum { HEAD_GLASSES = 0, HEAD_EINSTEIN = 1, HEAD_LUTHER = 2, HEAD_SLICK = 3, HEAD_LUKA = 4, HEAD_VORT = 5, HEAD_GLASSES2 = 6, HEAD_EINSTEIN2 = 7, HEAD_LUTHER2 = 8, HEAD_SLICK2 = 9 };


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_HEAL		( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )

//=======================================================
// Scientist
//=======================================================

class CMadScientist : public CTalkMonster
{
public:
	void Spawn(void);
	void Precache(void);

	void SetYawSpeed(void);
	void GiveNeedle(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int	ObjectCaps(void) { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual int FriendNumber(int arrayNumber);
	void SetActivity(Activity newActivity);
	Activity GetStoppedActivity(void);
	int ISoundMask(void);
	int FIdleSpeak(void);
	int LastShitTalked;
	void EquipNeedle(void);

	BOOL	CanHeal(void);
	void	Heal(void);
	void	Scream(void);

	void DeathSound(void);
	void PainSound(void);

	void TalkInit(void);

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	void			Killed(entvars_t *pevAttacker, int iGib);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

private:
	float m_painTime;
	float m_healTime;
};

LINK_ENTITY_TO_CLASS(monster_madscientist, CMadScientist);

TYPEDESCRIPTION	CMadScientist::m_SaveData[] =
{
	DEFINE_FIELD(CMadScientist, m_painTime, FIELD_TIME),
	DEFINE_FIELD(CMadScientist, m_healTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CMadScientist, CTalkMonster);


void CMadScientist::Scream(void)
{
	if (FOkToSpeak())
	{
		Talk(10);
		m_hTalkTarget = m_hEnemy;
		PlaySentence("SC_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM);
	}
}

void CMadScientist::EquipNeedle(void)
{
	pev->sequence = LookupSequence("pull_needle");
	pev->frame = 0;
	pev->framerate = 1.5;
	ResetSequenceInfo();
	pev->nextthink = gpGlobals->time + 2;
}


Activity CMadScientist::GetStoppedActivity(void)
{
	if (m_hEnemy != NULL)
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMadScientist::Classify(void)
{
	return	CLASS_ALIEN_PREDATOR;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMadScientist::SetYawSpeed(void)
{
	int ys;

	ys = 180;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 120;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMadScientist::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case SCIENTIST_AE_HEAL:
		Heal();
		break;
	case SCIENTIST_AE_NEEDLEON:
	{
								  int oldBody = pev->body;
								  pev->body = (oldBody % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 1;
	}
		break;
	case SCIENTIST_AE_NEEDLEOFF:
	{
								   int oldBody = pev->body;
								   pev->body = (oldBody % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 0;
	}
		break;

	default:
		CTalkMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CMadScientist::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/scientist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	//	m_flDistTooFar		= 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	// White hands
	pev->skin = 0;

	if (pev->body == -1)
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS - 1);// pick a head, any head
	}

	// Luther is black, make his hands black
	if (pev->body == HEAD_LUTHER)
		pev->skin = 1;

	CTalkMonster::g_talkWaitTime = 5;
	m_painTime = gpGlobals->time + 5;
	MonsterInit();
	EquipNeedle();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMadScientist::Precache(void)
{
	PRECACHE_MODEL("models/scientist.mdl");
	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	PRECACHE_SOUND("zombie/claw_strike1.wav");
	PRECACHE_SOUND("zombie/claw_strike2.wav");
	PRECACHE_SOUND("zombie/claw_strike3.wav");
	PRECACHE_SOUND("zombie/claw_miss1.wav");
	PRECACHE_SOUND("zombie/claw_miss2.wav");


	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

const char *CMadScientist::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CMadScientist::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

// Init talk data
void CMadScientist::TalkInit()
{

	CTalkMonster::TalkInit();

	// scientist will try to talk to friends in this order:

	m_szFriends[0] = "monster_scientist";
	m_szFriends[1] = "monster_sitting_scientist";
	m_szFriends[2] = "monster_barney";

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "SC_ANSWER";
	m_szGrp[TLK_QUESTION] = "SC_QUESTION";
	m_szGrp[TLK_IDLE] = "SC_IDLE";
	m_szGrp[TLK_STARE] = "SC_STARE";
	m_szGrp[TLK_USE] = "SC_OK";
	m_szGrp[TLK_UNUSE] = "SC_WAIT";
	m_szGrp[TLK_STOP] = "SC_STOP";
	m_szGrp[TLK_NOSHOOT] = "SC_SCARED";
	m_szGrp[TLK_HELLO] = "SC_HELLO";

	m_szGrp[TLK_PLHURT1] = "!SC_CUREA";
	m_szGrp[TLK_PLHURT2] = "!SC_CUREB";
	m_szGrp[TLK_PLHURT3] = "!SC_CUREC";

	m_szGrp[TLK_PHELLO] = "SC_PHELLO";
	m_szGrp[TLK_PIDLE] = "SC_PIDLE";
	m_szGrp[TLK_PQUESTION] = "SC_PQUEST";
	m_szGrp[TLK_SMELL] = "SC_SMELL";

	m_szGrp[TLK_WOUND] = "SC_WOUND";
	m_szGrp[TLK_MORTAL] = "SC_MORTAL";

	// get voice for head
	switch (pev->body % 3)
	{
	default:
	case HEAD_GLASSES:	m_voicePitch = 105; break;	//glasses
	case HEAD_EINSTEIN: m_voicePitch = 100; break;	//einstein
	case HEAD_LUTHER:	m_voicePitch = 95;  break;	//luther
	case HEAD_SLICK:	m_voicePitch = 100;  break;//slick
	}
}

int CMadScientist::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{

	if (pevInflictor && pevInflictor->flags & FL_CLIENT)
	{
		Remember(bits_MEMORY_PROVOKED);
		StopFollowing(TRUE);
	}

	// make sure friends talk about it if player hurts scientist...
	return CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CMadScientist::ISoundMask(void)
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}

//=========================================================
// PainSound
//=========================================================
void CMadScientist::PainSound(void)
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 4))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CMadScientist::DeathSound(void)
{
	PainSound();
}


void CMadScientist::Killed(entvars_t *pevAttacker, int iGib)
{
	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
	if (FClassnameIs(pevAttacker, "monster_madscientist") && (m_bitsDamageType == DMG_SLASH))
	{
		int pitch = GetVoicePitch();
		UTIL_Remove(CBaseEntity::Instance(pevAttacker));
		CBaseEntity *pMad = CBaseEntity::Create("monster_scientist", pevAttacker->origin, pevAttacker->angles);
		pMad->pev->modelindex = pevAttacker->modelindex;
		pMad->pev->targetname = pevAttacker->targetname;
		pMad->pev->sequence = LookupSequence("return_needle");
		pMad->pev->frame = 0;
		pMad->pev->body = pevAttacker->body;
		int oldBody = pMad->pev->body;
		pMad->pev->body = (oldBody % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 0;
		if (pMad->pev->body == HEAD_LUTHER)
			pMad->pev->skin = 1;
	}
}


void CMadScientist::SetActivity(Activity newActivity)
{
	int	iSequence;

	iSequence = LookupActivity(newActivity);

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence == ACTIVITY_NOT_AVAILABLE)
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity(newActivity);
}


BOOL CMadScientist::CanHeal(void)
{
	return FALSE;
}

void CMadScientist::Heal(void)
{

	// do stuff for this event.
	CBaseEntity *pHurt = CheckTraceHullAttack(80,2, DMG_SLASH);
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

	if (RANDOM_LONG(0, 1) && (LastShitTalked < gpGlobals->time))
	{
		m_painTime = gpGlobals->time + 5;
		LastShitTalked = gpGlobals->time + 7;
		SENTENCEG_PlayRndSz(ENT(pev), "SC_FUCKIT", 1.0, ATTN_IDLE, 0, GetVoicePitch());
	}

	/*
	Vector target = m_hTargetEnt->pev->origin - pev->origin;
	if (target.Length() > 100)
		return;

	m_hTargetEnt->TakeHealth(-gSkillData.scientistHeal, DMG_GENERIC);
	// Don't heal again for 1 minute
	m_healTime = gpGlobals->time + 60;
	*/
}

int CMadScientist::FriendNumber(int arrayNumber)
{
	static int array[3] = { 1, 2, 0 };
	if (arrayNumber < 3)
		return array[arrayNumber];
	return arrayNumber;
}

void CMadScientist::GiveNeedle(void)
{
	int oldBody = pev->body;
	pev->body = (oldBody % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 1;
}