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
// Explosive Houndeye
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "animation.h"
#include "nodes.h"
#include "squadmonster.h"
#include "soundent.h"
#include "game.h"
#include "weapons.h"
#include "effects.h"

#define HOUND_EXP_MAX_ATTACK_RADIUS 384

// Marphy Fact Files Fix - Fix various instances of Houndeye not correctly blinking/closing eyes
#define HOUND_EXP_EYE_EYE_FRAMES 3 // how many different switchable maps for the eye

#define HOUND_EXP_EYE_SOUND_STARTLE_VOLUME 128 // how loud a sound has to be to badly scare a sleeping houndeye

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_HOUND_EXP_CLOSE_EYE = LAST_COMMON_TASK + 1,
	TASK_HOUND_EXP_OPEN_EYE,
	TASK_HOUND_EXP_THREAT_DISPLAY,
	TASK_HOUND_EXP_FALL_ASLEEP,
	TASK_HOUND_EXP_WAKE_UP,
	TASK_HOUND_EXP_HOP_BACK
};

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_HOUND_EXP_AGITATED = LAST_COMMON_SCHEDULE + 1,
	SCHED_HOUND_EXP_HOP_RETREAT,
	SCHED_HOUND_EXP_FAIL,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define HOUND_EXP_AE_WARN 1
#define HOUND_EXP_AE_STARTATTACK 2
#define HOUND_EXP_AE_THUMP 3
#define HOUND_EXP_AE_ANGERSOUND1 4
#define HOUND_EXP_AE_ANGERSOUND2 5
#define HOUND_EXP_AE_HOPBACK 6
#define HOUND_EXP_AE_CLOSE_EYE 7

#define HOUNDEYE_GLOW_SPRITE "sprites/glow03.spr"

class CHoundeyeExplosive : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	int Classify();
	int IRelationship(CBaseEntity* pTarget);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	void SetYawSpeed();
	void WarmUpSound();
	void AlertSound();
	void DeathSound();
	void WarnSound();
	void PainSound();
	void IdleSound();
	void StartTask(Task_t* pTask);
	void RunTask(Task_t* pTask);
	void RunAI();
	void ExplosiveAttack();
	void Killed(entvars_t* pevAttacker, int iGib);
	void DeathEffect();
	void PrescheduleThink();
	void SetActivity(Activity NewActivity);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL FValidateHintType(short sHint);
	Schedule_t* GetScheduleOfType(int Type);
	Schedule_t* GetSchedule();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	bool m_fDontBlink;		// don't try to open/close eye if this bit is set!
	Vector m_vecPackCenter; // the center of the pack. The leader maintains this by averaging the origins of all pack members.

private:
	CSprite* m_pGlow;
};
LINK_ENTITY_TO_CLASS(monster_houndeye_explosive, CHoundeyeExplosive);

TYPEDESCRIPTION CHoundeyeExplosive::m_SaveData[] =
{
	DEFINE_FIELD(CHoundeyeExplosive, m_fDontBlink, FIELD_BOOLEAN),
	DEFINE_FIELD(CHoundeyeExplosive, m_vecPackCenter, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CHoundeyeExplosive, m_pGlow, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CHoundeyeExplosive, CBaseMonster);

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CHoundeyeExplosive::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

int CHoundeyeExplosive::IRelationship(CBaseEntity* pTarget)
{
	// Don't hate on the controller exterminator, yet.
	if (FClassnameIs(pTarget->pev, "monster_controller_exterminator"))
	{
		return R_AL;
	}
	return CBaseMonster::IRelationship(pTarget);
}

//=========================================================
//  FValidateHintType
//=========================================================
BOOL CHoundeyeExplosive::FValidateHintType(short sHint)
{
	int i;

	static short sHoundHints[] =
	{
		HINT_WORLD_MACHINERY,
		HINT_WORLD_BLINKING_LIGHT,
		HINT_WORLD_HUMAN_BLOOD,
		HINT_WORLD_ALIEN_BLOOD,
	};

	for (i = 0; i < ARRAYSIZE(sHoundHints); i++)
	{
		if (sHoundHints[i] == sHint)
		{
			return TRUE;
		}
	}

	ALERT(at_aiconsole, "Couldn't validate hint type");
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for houndeyes so that they
// try to get within half of their max attack radius before
// attacking, so as to increase their chances of doing damage.
//=========================================================
BOOL CHoundeyeExplosive::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist <= (HOUND_EXP_MAX_ATTACK_RADIUS * 0.5) && flDot >= 0.3)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHoundeyeExplosive::SetYawSpeed()
{
	int ys;

	ys = 90;

	switch (m_Activity)
	{
	case ACT_CROUCHIDLE: //sleeping!
		ys = 0;
		break;
	case ACT_IDLE:
		ys = 60;
		break;
	case ACT_WALK:
		ys = 90;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// SetActivity
//=========================================================
void CHoundeyeExplosive::SetActivity(Activity NewActivity)
{
	int iSequence;

	if (NewActivity == m_Activity)
		return;

	if (m_MonsterState == MONSTERSTATE_COMBAT && NewActivity == ACT_IDLE && RANDOM_LONG(0, 1))
	{
		// play pissed idle.
		iSequence = LookupSequence("madidle");

		m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

		// In case someone calls this with something other than the ideal activity
		m_IdealActivity = m_Activity;

		// Set to the desired anim, or default anim if the desired is not present
		if (iSequence > ACTIVITY_NOT_AVAILABLE)
		{
			pev->sequence = iSequence; // Set to the reset anim (if it's there)
			pev->frame = 0;			   // FIX: frame counter shouldn't be reset when its the same activity as before
			ResetSequenceInfo();
			SetYawSpeed();
		}

	}
	else
	{
		CBaseMonster::SetActivity(NewActivity);
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHoundeyeExplosive::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case HOUND_EXP_AE_WARN:
		// do stuff for this event.
		WarnSound();
		break;

	case HOUND_EXP_AE_STARTATTACK:
		WarmUpSound();
		break;

	case HOUND_EXP_AE_HOPBACK:
	{
		float flGravity = g_psv_gravity->value;

		pev->flags &= ~FL_ONGROUND;

		pev->velocity = gpGlobals->v_forward * -200;
		pev->velocity.z += (0.6 * flGravity) * 0.5;

		break;
	}

	case HOUND_EXP_AE_THUMP:
		// emit the shockwaves
		ExplosiveAttack();
		break;

	case HOUND_EXP_AE_ANGERSOUND1:
		//EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "houndexp/he_pain3.wav", 1, ATTN_NORM);
		break;

	case HOUND_EXP_AE_ANGERSOUND2:
		//EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "houndexp/he_pain1.wav", 1, ATTN_NORM);
		break;

	case HOUND_EXP_AE_CLOSE_EYE:
		if (!m_fDontBlink)
		{
			pev->skin = HOUND_EXP_EYE_EYE_FRAMES - 1;
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
void CHoundeyeExplosive::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/houndeye_explosive.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 36));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = DONT_BLEED;
	pev->effects = 0;
	pev->health = gSkillData.houndeyeExpHealth;
	pev->yaw_speed = 5;	   //!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView = 0.5; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_fDontBlink = false;
	m_afCapability = 0;

	m_pGlow = CSprite::SpriteCreate(HOUNDEYE_GLOW_SPRITE, pev->origin + Vector(0, 0, (pev->mins.z + pev->maxs.z) * 0.5), false);
	m_pGlow->SetTransparency(kRenderGlow, 255, 109, 36, 255, kRenderFxNoDissipation);
	m_pGlow->SetAttachment(edict(), 1);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHoundeyeExplosive::Precache()
{
	PRECACHE_MODEL("models/houndeye_explosive.mdl");

	PRECACHE_MODEL(HOUNDEYE_GLOW_SPRITE);

	PRECACHE_SOUND("houndexp/he_alert1.wav");
	PRECACHE_SOUND("houndexp/he_alert2.wav");
	PRECACHE_SOUND("houndexp/he_alert3.wav");

	PRECACHE_SOUND("houndexp/he_idle1.wav");
	PRECACHE_SOUND("houndexp/he_idle2.wav");
	PRECACHE_SOUND("houndexp/he_idle3.wav");

	PRECACHE_SOUND("houndexp/he_hunt1.wav");
	PRECACHE_SOUND("houndexp/he_hunt2.wav");
	PRECACHE_SOUND("houndexp/he_hunt3.wav");

	PRECACHE_SOUND("houndexp/he_pain1.wav");
	PRECACHE_SOUND("houndexp/he_pain3.wav");
	PRECACHE_SOUND("houndexp/he_pain4.wav");
	PRECACHE_SOUND("houndexp/he_pain5.wav");

	PRECACHE_SOUND("houndexp/he_attack1.wav");
	PRECACHE_SOUND("houndexp/he_attack3.wav");
}

//=========================================================
// IdleSound
//=========================================================
void CHoundeyeExplosive::IdleSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "houndexp/he_idle1.wav", 1, ATTN_NORM, 0, PITCH_NORM);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "houndexp/he_idle2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "houndexp/he_idle3.wav", 1, ATTN_NORM, 0, PITCH_NORM);
		break;
	}
}

void CHoundeyeExplosive::RunAI()
{
	// first, do base class stuff
	CBaseMonster::RunAI();
 
	// this isn't cheap because it runs every frame, 
	// but this monster usually doesn't stay alive
	// for very long, might be removed later.
	Vector vecSrc = pev->origin;
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSrc);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(vecSrc.x);			 // X
	WRITE_COORD(vecSrc.y);			 // Y
	WRITE_COORD(vecSrc.z);			 // Z
	WRITE_BYTE(12);					 // radius * 0.1
	WRITE_BYTE(224);				 // r
	WRITE_BYTE(152);				 // g
	WRITE_BYTE(96);					 // b
	WRITE_BYTE(1.0); // time * 10
	WRITE_BYTE(0.1);					 // decay * 0.1
	MESSAGE_END();
}

//=========================================================
// IdleSound
//=========================================================
void CHoundeyeExplosive::WarmUpSound()
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "houndexp/he_attack1.wav", 0.7, ATTN_NORM, 0, PITCH_NORM);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "houndexp/he_attack3.wav", 0.7, ATTN_NORM, 0, PITCH_NORM);
		break;
	}

	Vector vecSrc = pev->origin;
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSrc);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(vecSrc.x);			 // X
	WRITE_COORD(vecSrc.y);			 // Y
	WRITE_COORD(vecSrc.z);			 // Z
	WRITE_BYTE(12);					 // radius * 0.1
	WRITE_BYTE(224);				 // r
	WRITE_BYTE(152);				 // g
	WRITE_BYTE(96);					 // b
	WRITE_BYTE(20 / pev->framerate); // time * 10
	WRITE_BYTE(0);					 // decay * 0.1
	MESSAGE_END();
}

//=========================================================
// WarnSound
//=========================================================
void CHoundeyeExplosive::WarnSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "houndexp/he_hunt1.wav", 1, ATTN_NORM, 0, PITCH_NORM);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "houndexp/he_hunt2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "houndexp/he_hunt3.wav", 1, ATTN_NORM, 0, PITCH_NORM);
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CHoundeyeExplosive::AlertSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "houndexp/he_alert1.wav", 1, ATTN_NONE, 0, PITCH_NORM);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "houndexp/he_alert2.wav", 1, ATTN_NONE, 0, PITCH_NORM);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "houndexp/he_alert3.wav", 1, ATTN_NONE, 0, PITCH_NORM);
		break;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CHoundeyeExplosive::DeathSound()
{
	return;
}

//=========================================================
// PainSound
//=========================================================
void CHoundeyeExplosive::PainSound()
{
	return;
}

//=========================================================
// DeathEffect
//=========================================================
void CHoundeyeExplosive::DeathEffect()
{
	// A slightly smaller explosion when killed.
	int iContents = UTIL_PointContents(pev->origin);
	int iScale;

	iScale = 50;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	if (iContents != CONTENTS_WATER)
	{
		WRITE_SHORT(g_sModelIndexFireball);
	}
	else
	{
		WRITE_SHORT(g_sModelIndexWExplosion);
	}
	WRITE_BYTE(iScale); // scale * 10
	WRITE_BYTE(15);		// framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	if (m_pGlow)
		m_pGlow->pev->effects |= EF_NODRAW;

	entvars_t* pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage(pev->origin, pev, pevOwner, gSkillData.houndeyeExpDmgBlast,
		gSkillData.houndeyeExpDmgRadius, CLASS_NONE, DMG_BLAST | DMG_NEVERGIB);
}

//=========================================================
// ExplosiveAttack
//=========================================================
void CHoundeyeExplosive::ExplosiveAttack()
{
	// A more bigger explosion if you let him attack.
	int iContents = UTIL_PointContents(pev->origin);
	int iScale;

	iScale = 50;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	if (iContents != CONTENTS_WATER)
	{
		WRITE_SHORT(g_sModelIndexFireball);
	}
	else
	{
		WRITE_SHORT(g_sModelIndexWExplosion);
	}
	WRITE_BYTE(iScale); // scale * 10
	WRITE_BYTE(15);		// framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	if (m_pGlow)
		m_pGlow->pev->effects |= EF_NODRAW;

	entvars_t* pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage(pev->origin, pev, pevOwner, gSkillData.houndeyeExpDmgBlast * 2, 
		gSkillData.houndeyeExpDmgRadius * 2, CLASS_NONE, DMG_BLAST | DMG_NEVERGIB);

	UTIL_Remove(this);
}

void CHoundeyeExplosive::Killed(entvars_t* pevAttacker, int iGib)
{
	// When killed, make him non solid so the player doesn't step
	// in a invisible box.
	pev->solid = SOLID_NOT;
	pev->renderamt = 0;
	pev->rendermode = kRenderTransTexture;

	CBaseMonster::Killed(pevAttacker, GIB_NEVER);
}


//=========================================================
// start task
//=========================================================
void CHoundeyeExplosive::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_HOUND_EXP_OPEN_EYE:
	{
		m_fDontBlink = false; // turn blinking back on and that code will automatically open the eye
		m_iTaskStatus = TASKSTATUS_COMPLETE;
		break;
	}
	case TASK_HOUND_EXP_CLOSE_EYE:
	{
		pev->skin = 0;
		m_fDontBlink = true; // tell blink code to leave the eye alone.
		break;
	}
	case TASK_HOUND_EXP_THREAT_DISPLAY:
	{
		m_IdealActivity = ACT_IDLE_ANGRY;
		break;
	}
	case TASK_RANGE_ATTACK1:
	{
		m_IdealActivity = ACT_RANGE_ATTACK1;

		/*
			if ( InSquad() )
			{
				// see if there is a battery to connect to.
				CBaseMonster *pSquad = m_pSquadLeader;

				while ( pSquad )
				{
					if ( pSquad->m_iMySlot == bits_SLOT_HOUND_EXP_BATTERY )
					{
						// draw a beam.
						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
							WRITE_BYTE( TE_BEAMENTS );
							WRITE_SHORT( ENTINDEX( this->edict() ) );
							WRITE_SHORT( ENTINDEX( pSquad->edict() ) );
							WRITE_SHORT( m_iSpriteTexture );
							WRITE_BYTE( 0 ); // framestart
							WRITE_BYTE( 0 ); // framerate
							WRITE_BYTE( 10 ); // life
							WRITE_BYTE( 40 );  // width
							WRITE_BYTE( 10 );   // noise
							WRITE_BYTE( 0  );   // r, g, b
							WRITE_BYTE( 50 );   // r, g, b
							WRITE_BYTE( 250);   // r, g, b
							WRITE_BYTE( 255 );	// brightness
							WRITE_BYTE( 30 );		// speed
						MESSAGE_END();
						break;
					}

					pSquad = pSquad->m_pSquadNext;
				}
			}
*/

		break;
	}
	case TASK_SPECIAL_ATTACK1:
	{
		m_IdealActivity = ACT_SPECIAL_ATTACK1;
		break;
	}
	case TASK_GUARD:
	{
		m_IdealActivity = ACT_GUARD;
		break;
	}
	case TASK_DIE:
		m_flWaitFinished = gpGlobals->time + 1.6;
		DeathEffect();
		[[fallthrough]];
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
void CHoundeyeExplosive::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_HOUND_EXP_THREAT_DISPLAY:
	{
		MakeIdealYaw(m_vecEnemyLKP);
		ChangeYaw(pev->yaw_speed);

		if (m_fSequenceFinished)
		{
			TaskComplete();
		}

		break;
	}
	case TASK_HOUND_EXP_CLOSE_EYE:
	{
		if (pev->skin < HOUND_EXP_EYE_EYE_FRAMES - 1)
		{
			pev->skin++;
		}
		break;
	}
	case TASK_HOUND_EXP_HOP_BACK:
	{
		if (m_fSequenceFinished)
		{
			TaskComplete();
		}
		break;
	}
	case TASK_SPECIAL_ATTACK1:
	{
		pev->skin = RANDOM_LONG(0, HOUND_EXP_EYE_EYE_FRAMES - 1);

		MakeIdealYaw(m_vecEnemyLKP);
		ChangeYaw(pev->yaw_speed);

		float life;
		life = ((255 - pev->frame) / (pev->framerate * m_flFrameRate));
		if (life < 0.1)
			life = 0.1;

		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_IMPLOSION);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 16);
		WRITE_BYTE(50 * life + 100);   // radius
		WRITE_BYTE(pev->frame / 25.0); // count
		WRITE_BYTE(life * 10);		   // life
		MESSAGE_END();

		if (m_fSequenceFinished)
		{
			ExplosiveAttack();
			TaskComplete();
		}

		break;
	}
	case TASK_DIE:
		if (gpGlobals->time > m_flWaitFinished)
		{
			SetThink(&CHoundeyeExplosive::SUB_Remove);
			pev->nextthink = gpGlobals->time + 0.15;
		}
		break;
	default:
	{
		CBaseMonster::RunTask(pTask);
		break;
	}
	}
}

//=========================================================
// PrescheduleThink
//=========================================================
void CHoundeyeExplosive::PrescheduleThink()
{
	// if the hound is mad and is running, make hunt noises.
	if (m_MonsterState == MONSTERSTATE_COMBAT && m_Activity == ACT_RUN && RANDOM_FLOAT(0, 1) < 0.2)
	{
		WarnSound();
	}

	// at random, initiate a blink if not already blinking or sleeping
	if (!m_fDontBlink)
	{
		if ((pev->skin == 0) && RANDOM_LONG(0, 0x7F) == 0)
		{ // start blinking!
			pev->skin = HOUND_EXP_EYE_EYE_FRAMES - 1;
		}
		else if (pev->skin != 0)
		{ // already blinking
			pev->skin--;
		}
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
// Marphy Fact Files Fix - Fix freeze stutter after leaderlook sequence
Task_t tlhoundeyeGuardPack[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_PLAY_SEQUENCE, (float)ACT_GUARD},
};

Schedule_t slhoundeyeGuardPack[] =
{
	{tlhoundeyeGuardPack,
		ARRAYSIZE(tlhoundeyeGuardPack),
		bits_COND_SEE_HATE |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_PROVOKED |
			bits_COND_HEAR_SOUND,

		bits_SOUND_COMBAT | // sound flags
			bits_SOUND_WORLD |
			bits_SOUND_MEAT |
			bits_SOUND_PLAYER,
		"GuardPack"},
};

// primary range attack
Task_t tlhoundeyeYell1[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_FACE_IDEAL, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
	{TASK_SET_SCHEDULE, (float)SCHED_HOUND_EXP_AGITATED},
};

Task_t tlhoundeyeYell2[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_FACE_IDEAL, (float)0},
	{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slhoundeyeRangeAttack[] =
{
	{tlhoundeyeYell1,
		ARRAYSIZE(tlhoundeyeYell1),
		bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE,
		0,
		"HoundRangeAttack1"},
	{tlhoundeyeYell2,
		ARRAYSIZE(tlhoundeyeYell2),
		bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE,
		0,
		"HoundRangeAttack2"},
};


Task_t tlhoundeyeSpecialAttack1[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_FACE_IDEAL, (float)0},
	{TASK_SPECIAL_ATTACK1, (float)0},
	{TASK_PLAY_SEQUENCE, (float)ACT_IDLE_ANGRY},
};

Schedule_t slhoundeyeSpecialAttack1[] =
{
	{tlhoundeyeSpecialAttack1,
		ARRAYSIZE(tlhoundeyeSpecialAttack1),
		bits_COND_NEW_ENEMY |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE |
			bits_COND_ENEMY_OCCLUDED,

		0,
		"Hound Special Attack1"},
};

Task_t tlhoundeyeAgitated[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_HOUND_EXP_THREAT_DISPLAY, 0},
};

Schedule_t slhoundeyeAgitated[] =
{
	{tlhoundeyeAgitated,
		ARRAYSIZE(tlhoundeyeAgitated),
		bits_COND_NEW_ENEMY |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE,
		0,
		"Hound Agitated"},
};

// hound fails in combat with client in the PVS
Task_t tlhoundeyeCombatFailPVS[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_HOUND_EXP_THREAT_DISPLAY, 0},
	{TASK_WAIT_FACE_ENEMY, (float)1},
};

Schedule_t slhoundeyeCombatFailPVS[] =
{
	{tlhoundeyeCombatFailPVS,
		ARRAYSIZE(tlhoundeyeCombatFailPVS),
		bits_COND_NEW_ENEMY |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE,
		0,
		"HoundCombatFailPVS"},
};

// hound fails in combat with no client in the PVS. Don't keep peeping!
Task_t tlhoundeyeCombatFailNoPVS[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_HOUND_EXP_THREAT_DISPLAY, 0},
	{TASK_WAIT_FACE_ENEMY, (float)2},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
	{TASK_WAIT_PVS, 0},
};

Schedule_t slhoundeyeCombatFailNoPVS[] =
{
	{tlhoundeyeCombatFailNoPVS,
		ARRAYSIZE(tlhoundeyeCombatFailNoPVS),
		bits_COND_NEW_ENEMY |
			bits_COND_LIGHT_DAMAGE |
			bits_COND_HEAVY_DAMAGE,
		0,
		"HoundCombatFailNoPVS"},
};

DEFINE_CUSTOM_SCHEDULES(CHoundeyeExplosive) {
	slhoundeyeGuardPack,
		slhoundeyeRangeAttack,
		& slhoundeyeRangeAttack[1],
		slhoundeyeSpecialAttack1,
		slhoundeyeAgitated,
		slhoundeyeCombatFailPVS,
		slhoundeyeCombatFailNoPVS,
};

IMPLEMENT_CUSTOM_SCHEDULES(CHoundeyeExplosive, CBaseMonster);

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CHoundeyeExplosive::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
	{
		return &slhoundeyeRangeAttack[0];
		/*
			if ( InSquad() )
			{
				return &slhoundeyeRangeAttack[ RANDOM_LONG( 0, 1 ) ];
			}

			return &slhoundeyeRangeAttack[ 1 ];
*/
	}
	case SCHED_SPECIAL_ATTACK1:
	{
		return &slhoundeyeSpecialAttack1[0];
	}
	case SCHED_GUARD:
	{
		return &slhoundeyeGuardPack[0];
	}
	case SCHED_HOUND_EXP_AGITATED:
	{
		return &slhoundeyeAgitated[0];
	}
	case SCHED_FAIL:
	{
		if (m_MonsterState == MONSTERSTATE_COMBAT)
		{
			if (!FNullEnt(FIND_CLIENT_IN_PVS(edict())))
			{
				// client in PVS
				return &slhoundeyeCombatFailPVS[0];
			}
			else
			{
				// client has taken off!
				return &slhoundeyeCombatFailNoPVS[0];
			}
		}
		else
		{
			return CBaseMonster::GetScheduleOfType(Type);
		}
	}
	default:
	{
		return CBaseMonster::GetScheduleOfType(Type);
	}
	}
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t* CHoundeyeExplosive::GetSchedule()
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}
		break;
	}
	}

	return CBaseMonster::GetSchedule();
}
