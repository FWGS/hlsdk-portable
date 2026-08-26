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
#include	"weapons.h"


#define		NUM_SCIENTIST_HEADS		4 // four heads available for scientist model
enum { HEAD_GLASSES = 0, HEAD_EINSTEIN = 1, HEAD_LUTHER = 2, HEAD_SLICK = 3 };


#define VENDOR_TYPE_PIWO	3
#define MAX_VENDOR_PIWO 2

//WEAPONS
const char* VendorPiwoItems[MAX_VENDOR_PIWO] = {
	"PIWKO (Beer) - Price: 1$",
	"PIWKO (Six Pack Beer) - Price: 5$",
};

char* VendorPiwoItemNames[MAX_VENDOR_PIWO] = {
	"weapon_piwo",
	"ammo_sixpack",
};

int VendorPiwoPrices[MAX_VENDOR_PIWO] =
{
	1,
	5,
};

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED,
};

enum
{
	TASK_SAY_HEAL = LAST_TALKMONSTER_TASK + 1,
	TASK_HEAL,
	TASK_SAY_FEAR,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_HEAL		( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )
#define		SMOOKER_SMOKE_CIGGY		( 53 )
#define		SMOOKER_THROW_CIGGY		( 54 )


#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY	500
#define BOLT_WATER_VELOCITY	250


class CCiggie : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT CiggieThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	int timer;
	int m_maxFrame;

	int m_iTrail;

public:
	static CCiggie *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(ciggie, CCiggie);

CCiggie *CCiggie::BoltCreate(void)
{
	// Create a new entity with CCrossbowBolt private data
	CCiggie *pBolt = GetClassPtr((CCiggie *)NULL);
	pBolt->pev->classname = MAKE_STRING("ciggie");
	pBolt->Spawn();

	return pBolt;
}

void CCiggie::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	timer = 0;
	m_maxFrame = 0;

	pev->gravity = 0.6;

	SET_MODEL(ENT(pev), "models/ciggie.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch(&CCiggie::BoltTouch);
	SetThink(&CCiggie::CiggieThink);
	pev->nextthink = gpGlobals->time + 0.01;
}


void CCiggie::Precache()
{
	PRECACHE_MODEL("models/ciggie.mdl");
	PRECACHE_MODEL("sprites/xffloor.spr");
	PRECACHE_SOUND("ambience/flameburst1.wav");
}


int	CCiggie::Classify(void)
{
	return	CLASS_NONE;
}

void CCiggie::BoltTouch(CBaseEntity *pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;
	}
}

void CCiggie::CiggieThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
	timer += 1;
	if (timer == 50)
	{
		SET_MODEL(ENT(pev), "sprites/xffloor.spr");
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "ambience/flameburst1.wav", 1, ATTN_NORM, 0, 100);
		pev->scale = 2;
		pev->rendermode = kRenderTransAdd;
		pev->renderamt = 255;
		pev->solid = SOLID_NOT;
		pev->gravity = 0;
		pev->origin = pev->origin + gpGlobals->v_up * 54;
		m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z - 20);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(20); // scale * 10
		WRITE_BYTE(10); // framerate
		MESSAGE_END();
	}
	if (timer > 50)
	{
		RadiusDamage(pev->origin, pev, pev, 2, 42, CLASS_NONE, DMG_BURN);

		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE(25);		// radius * 0.1
		WRITE_BYTE(255);		// r
		WRITE_BYTE(204);		// g
		WRITE_BYTE(0);		// b
		WRITE_BYTE(1);		// time * 10
		WRITE_BYTE(0);		// decay * 0.1
		MESSAGE_END();
		if (pev->frame++)
		{
			if (pev->frame > m_maxFrame)
			{
				pev->frame = 0;

			}
		}
	}
	if (timer >= 500)
	{
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z - 20);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(20); // scale * 10
		WRITE_BYTE(10); // framerate
		MESSAGE_END();
		SetThink(NULL);
		SetTouch(NULL);
		UTIL_Remove(this);
	}
}

#endif


//=======================================================
// Scientist
//=======================================================

class CGus : public CTalkMonster
{
public:
	void Spawn(void);
	void Precache(void);

	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void RunTask(Task_t *pTask);
	void StartTask(Task_t *pTask);
	int	ObjectCaps(void) { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void SwitchVendorItem(int ItemType);
	virtual int FriendNumber(int arrayNumber);
	void SetActivity(Activity newActivity);
	Activity GetStoppedActivity(void);
	int ISoundMask(void);
	void DeclineFollowing(void);

	int GetPaid(int amount);
	void KeyValue(KeyValueData *pkvd);
	int m_vendorType;

	float	CoverRadius(void) { return 1200; }		// Need more room for cover because scientists want to get far away!
	BOOL	DisregardEnemy(CBaseEntity *pEnemy) { return !pEnemy->IsAlive() || (gpGlobals->time - m_fearTime) > 15; }

	BOOL	CanHeal(void);
	void	Heal(void);
	void	Scream(void);

	// Override these to set behavior
	Schedule_t *GetScheduleOfType(int Type);
	Schedule_t *GetSchedule(void);
	MONSTERSTATE GetIdealState(void);

	void DeathSound(void);
	void PainSound(void);

	void TalkInit(void);

	void			Killed(entvars_t *pevAttacker, int iGib);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

private:
	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

LINK_ENTITY_TO_CLASS(monster_gus, CGus);

TYPEDESCRIPTION	CGus::m_SaveData[] =
{
	DEFINE_FIELD(CGus, m_painTime, FIELD_TIME),
	DEFINE_FIELD(CGus, m_healTime, FIELD_TIME),
	DEFINE_FIELD(CGus, m_fearTime, FIELD_TIME),
	DEFINE_FIELD(CGus, m_vendorType, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CGus, CTalkMonster);

void CGus::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "vendortype"))
	{
		m_vendorType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlFollowGus[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE, (float)128 },	// Move within 128 of target ent (client)
	//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slFollowGus[] =
{
	{
		tlFollowGus,
		ARRAYSIZE(tlFollowGus),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlFollowScaredGus[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },// If you fail, follow normally
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED, (float)128 },	// Move within 128 of target ent (client)
	//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE_SCARED },
};

Schedule_t	slFollowScaredGus[] =
{
	{
		tlFollowScaredGus,
		ARRAYSIZE(tlFollowScaredGus),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"FollowScared"
	},
};

Task_t	tlFaceTargetScaredGus[] =
{
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE_SCARED },
};

Schedule_t	slFaceTargetScaredGus[] =
{
	{
		tlFaceTargetScaredGus,
		ARRAYSIZE(tlFaceTargetScaredGus),
		bits_COND_HEAR_SOUND |
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"FaceTargetScared"
	},
};

Task_t	tlStopFollowingGus[] =
{
	{ TASK_CANT_FOLLOW, (float)0 },
};

Schedule_t	slStopFollowingGus[] =
{
	{
		tlStopFollowingGus,
		ARRAYSIZE(tlStopFollowingGus),
		0,
		0,
		"StopFollowing"
	},
};


Task_t	tlHealGus[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, (float)50 },	// Move within 60 of target ent (client)
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },	// If you fail, catch up with that guy! (change this to put syringe away and then chase)
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_SAY_HEAL, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, (float)ACT_ARM },			// Whip out the needle
	{ TASK_HEAL, (float)0 },	// Put it in the player
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, (float)ACT_DISARM },			// Put away the needle
};

Schedule_t	slHealGus[] =
{
	{
		tlHealGus,
		ARRAYSIZE(tlHealGus),
		0,	// Don't interrupt or he'll end up running around with a needle all the time
		0,
		"Heal"
	},
};


Task_t	tlFaceTargetGus[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};

Schedule_t	slFaceTargetGus[] =
{
	{
		tlFaceTargetGus,
		ARRAYSIZE(tlFaceTargetGus),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlSciPanicGus[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_SCREAM, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_EXCITED },	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slSciPanicGus[] =
{
	{
		tlSciPanicGus,
		ARRAYSIZE(tlSciPanicGus),
		0,
		0,
		"SciPanic"
	},
};


Task_t	tlIdleSciStandGus[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 }, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET, (float)0 }, // reset head position
};

Schedule_t	slIdleSciStandGus[] =
{
	{
		tlIdleSciStandGus,
		ARRAYSIZE(tlIdleSciStandGus),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_CLIENT_PUSH |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleSciStand"

	},
};


Task_t	tlScientistCoverGus[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH_SCARED, (float)0 },
	{ TASK_TURN_LEFT, (float)179 },
	{ TASK_SET_SCHEDULE, (float)SCHED_HIDE },
};

Schedule_t	slScientistCoverGus[] =
{
	{
		tlScientistCoverGus,
		ARRAYSIZE(tlScientistCoverGus),
		bits_COND_NEW_ENEMY,
		0,
		"ScientistCover"
	},
};



Task_t	tlScientistHideGus[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_CROUCH },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },	// FIXME: This looks lame
	{ TASK_WAIT_RANDOM, (float)10.0 },
};

Schedule_t	slScientistHideGus[] =
{
	{
		tlScientistHideGus,
		ARRAYSIZE(tlScientistHideGus),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER,
		"ScientistHide"
	},
};


Task_t	tlScientistStartleGus[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_RANDOM_SCREAM, (float)0.3 },				// Scream 30% of the time
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH },
	{ TASK_RANDOM_SCREAM, (float)0.1 },				// Scream again 10% of the time
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCHIDLE },
	{ TASK_WAIT_RANDOM, (float)1.0 },
};

Schedule_t	slScientistStartleGus[] =
{
	{
		tlScientistStartleGus,
		ARRAYSIZE(tlScientistStartleGus),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		0,
		"ScientistStartle"
	},
};



Task_t	tlFearGus[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_SAY_FEAR, (float)0 },
	//	{ TASK_PLAY_SEQUENCE,			(float)ACT_FEAR_DISPLAY		},
};

Schedule_t	slFearGus[] =
{
	{
		tlFearGus,
		ARRAYSIZE(tlFearGus),
		bits_COND_NEW_ENEMY,
		0,
		"Fear"
	},
};


DEFINE_CUSTOM_SCHEDULES(CGus)
{
	slFollowGus,
		slFaceTargetGus,
		slIdleSciStandGus,
		slFearGus,
		slScientistCoverGus,
		slScientistHideGus,
		slScientistStartleGus,
		slHealGus,
		slStopFollowingGus,
		slSciPanicGus,
		slFollowScaredGus,
		slFaceTargetScaredGus,
};


IMPLEMENT_CUSTOM_SCHEDULES(CGus, CTalkMonster);

int CGus::GetPaid(int amount)
{
	if (!FBitSet(pev->spawnflags, SF_MONSTER_VENDOR))
		return 0;

	switch (m_vendorType)
	{
	case VENDOR_TYPE_PIWO:
	{
							 if (amount < VendorPiwoPrices[SelectedItem])
							{
								UTIL_CenterPrintAll("Not enough money!");
								PlaySentence("SC_VENDDECLINE", 2, VOL_NORM, ATTN_NORM);
								return 0;
							}
							else
							{
								UTIL_CenterPrintAll("Item purchased!");
								PlaySentence("SC_VENDBUY", 2, VOL_NORM, ATTN_NORM);

								UTIL_MakeVectors(pev->angles);

								CBaseEntity *pGun = DropItem(VendorPiwoItemNames[SelectedItem], pev->origin + gpGlobals->v_forward * 32 + gpGlobals->v_up * 42, pev->angles);
								pGun->pev->velocity = pGun->pev->velocity + gpGlobals->v_forward * 32;

								return VendorPiwoPrices[SelectedItem];
							}

							break;
	}
	default:
		return 0; break;
	}
}

void CGus::DeclineFollowing(void)
{
	if (FBitSet(pev->spawnflags, SF_MONSTER_VENDOR))
	{
		SwitchVendorItem(m_vendorType);
		return;
	}

	Talk(10);
	m_hTalkTarget = m_hEnemy;
	PlaySentence("SC_POK", 2, VOL_NORM, ATTN_NORM);
}

void CGus::SwitchVendorItem(int ItemType)
{
	switch (ItemType)
	{
	case VENDOR_TYPE_PIWO:
	{
							if (SelectedItem >= MAX_VENDOR_PIWO - 1)
								SelectedItem = 0;
							else
								SelectedItem += 1;
							UTIL_CenterPrintAll(VendorPiwoItems[SelectedItem]);
							break;
	}
	default:
		break;
	}

	PlaySentence("SC_VENDP", 2, VOL_NORM, ATTN_NORM);
}



void CGus::Scream(void)
{
	if (FOkToSpeak())
	{
		Talk(10);
		m_hTalkTarget = m_hEnemy;
		PlaySentence("SC_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM);
	}
}


Activity CGus::GetStoppedActivity(void)
{
	if (m_hEnemy != NULL)
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}


void CGus::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_SAY_HEAL:
		//		if ( FOkToSpeak() )
		Talk(2);
		m_hTalkTarget = m_hTargetEnt;
		PlaySentence("SC_HEAL", 2, VOL_NORM, ATTN_IDLE);

		TaskComplete();
		break;

	case TASK_SCREAM:
		Scream();
		TaskComplete();
		break;

	case TASK_RANDOM_SCREAM:
		if (RANDOM_FLOAT(0, 1) < pTask->flData)
			Scream();
		TaskComplete();
		break;

	case TASK_SAY_FEAR:
		if (FOkToSpeak())
		{
			Talk(2);
			m_hTalkTarget = m_hEnemy;
			if (m_hEnemy->IsPlayer())
				PlaySentence("SC_PLFEAR", 5, VOL_NORM, ATTN_NORM);
			else
				PlaySentence("SC_FEAR", 5, VOL_NORM, ATTN_NORM);
		}
		TaskComplete();
		break;

	case TASK_HEAL:
		m_IdealActivity = ACT_MELEE_ATTACK1;
		break;

	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
	{
											 if ((m_hTargetEnt->pev->origin - pev->origin).Length() < 1)
												 TaskComplete();
											 else
											 {
												 m_vecMoveGoal = m_hTargetEnt->pev->origin;
												 if (!MoveToTarget(ACT_WALK_SCARED, 0.5))
													 TaskFail();
											 }
	}
		break;

	default:
		CTalkMonster::StartTask(pTask);
		break;
	}
}

void CGus::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RUN_PATH_SCARED:
		if (MovementIsComplete())
			TaskComplete();
		if (RANDOM_LONG(0, 31) < 8)
			Scream();
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
	{
											 if (RANDOM_LONG(0, 63)< 8)
												 Scream();

											 if (m_hEnemy == NULL)
											 {
												 TaskFail();
											 }
											 else
											 {
												 float distance;

												 distance = (m_vecMoveGoal - pev->origin).Length2D();
												 // Re-evaluate when you think your finished, or the target has moved too far
												 if ((distance < pTask->flData) || (m_vecMoveGoal - m_hTargetEnt->pev->origin).Length() > pTask->flData * 0.5)
												 {
													 m_vecMoveGoal = m_hTargetEnt->pev->origin;
													 distance = (m_vecMoveGoal - pev->origin).Length2D();
													 FRefreshRoute();
												 }

												 // Set the appropriate activity based on an overlapping range
												 // overlap the range to prevent oscillation
												 if (distance < pTask->flData)
												 {
													 TaskComplete();
													 RouteClear();		// Stop moving
												 }
												 else if (distance < 190 && m_movementActivity != ACT_WALK_SCARED)
													 m_movementActivity = ACT_WALK_SCARED;
												 else if (distance >= 270 && m_movementActivity != ACT_RUN_SCARED)
													 m_movementActivity = ACT_RUN_SCARED;
											 }
	}
		break;

	case TASK_HEAL:
		if (m_fSequenceFinished)
		{
			TaskComplete();
		}
		else
		{
			if (TargetDistance() > 90)
				TaskComplete();
			pev->ideal_yaw = UTIL_VecToYaw(m_hTargetEnt->pev->origin - pev->origin);
			ChangeYaw(pev->yaw_speed);
		}
		break;
	default:
		CTalkMonster::RunTask(pTask);
		break;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGus::Classify(void)
{
	return	CLASS_HUMAN_PASSIVE;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGus::SetYawSpeed(void)
{
	int ys;

	ys = 90;

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
void CGus::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case SCIENTIST_AE_HEAL:		// Heal my target (if within range)
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
	case SMOOKER_SMOKE_CIGGY:
	{
		Vector smook = pev->origin + gpGlobals->v_up * 44 + gpGlobals->v_forward * 12;
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(smook.x);
		WRITE_COORD(smook.y);
		WRITE_COORD(smook.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(10); // scale * 10
		WRITE_BYTE(10); // framerate
		MESSAGE_END();
	}
		break;
	case SMOOKER_THROW_CIGGY:
	{
		if (CVAR_GET_FLOAT("preventrandom") == 0)
		{					
		#ifndef CLIENT_DLL
			UTIL_MakeVectors(pev->angles);
			CCiggie *pBolt = CCiggie::BoltCreate();
			pBolt->pev->origin = pev->origin - gpGlobals->v_right * 14 + gpGlobals->v_up * 52;
			pBolt->pev->movetype = MOVETYPE_BOUNCE;
			pBolt->pev->gravity = 0.6;
			pBolt->pev->friction = 0.8;
			pBolt->pev->velocity = -gpGlobals->v_right * RANDOM_LONG(90,200);
			pBolt->pev->owner = edict();

			//pBolt->pev->avelocity.x = -600;
			pBolt->pev->avelocity.y = RANDOM_LONG(-300, 500);
			//pBolt->pev->avelocity.z = RANDOM_LONG(-100, 200);
		#endif
		}
	}
		break;

	default:
		CTalkMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CGus::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/gus.mdl");
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

	MonsterInit();
	SetUse(&CTalkMonster::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGus::Precache(void)
{
	PRECACHE_MODEL("models/gus.mdl");
	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");
	UTIL_PrecacheOther("ciggie");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

// Init talk data
void CGus::TalkInit()
{

	CTalkMonster::TalkInit();

	// scientist will try to talk to friends in this order:

	m_szFriends[0] = "monster_scientist";
	m_szFriends[1] = "monster_sitting_scientist";
	m_szFriends[2] = "monster_barney";

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "SC_ANSWER";
	m_szGrp[TLK_QUESTION] = "SC_PQUEST";
	m_szGrp[TLK_IDLE] = "SC_PIDLE";
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

	m_voicePitch = 85;
}

int CGus::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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
int CGus::ISoundMask(void)
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}

//=========================================================
// PainSound
//=========================================================
void CGus::PainSound(void)
{
	if (gpGlobals->time < m_painTime)
		return;

	//SPECIAL HAPPENING
	if (RANDOM_LONG(0, 8) == 0) //SPECIAL HAPPENING
	{
		switch (RANDOM_LONG(0, 10))
		{
		case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain1.wav", 1, ATTN_NORM, 0, 100); break;
		case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain2.wav", 1, ATTN_NORM, 0, 100); break;
		case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain3.wav", 1, ATTN_NORM, 0, 100); break;
		case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain4.wav", 1, ATTN_NORM, 0, 100); break;
		case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain5.wav", 1, ATTN_NORM, 0, 100); break;
		case 5: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain6.wav", 1, ATTN_NORM, 0, 100); break;
		case 6: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain7.wav", 1, ATTN_NORM, 0, 100); break;
		case 7: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain8.wav", 1, ATTN_NORM, 0, 100); break;
		case 8: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain9.wav", 1, ATTN_NORM, 0, 100); break;
		case 9: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain10.wav", 1, ATTN_NORM, 0, 100); break;
		case 10: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain11.wav", 1, ATTN_NORM, 0, 100); break;
		}
		return;
	}
	//SPECIAL HAPPENING

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
void CGus::DeathSound(void)
{
	PainSound();
}


void CGus::Killed(entvars_t *pevAttacker, int iGib)
{
	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}


void CGus::SetActivity(Activity newActivity)
{
	int	iSequence;

	iSequence = LookupActivity(newActivity);

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence == ACTIVITY_NOT_AVAILABLE)
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity(newActivity);
}


Schedule_t* CGus::GetScheduleOfType(int Type)
{
	Schedule_t *psched;

	switch (Type)
	{
		// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that scientist will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slFaceTargetGus;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slFollowGus;

	case SCHED_CANT_FOLLOW:
		return slStopFollowingGus;

	case SCHED_PANIC:
		return slSciPanicGus;

	case SCHED_TARGET_CHASE_SCARED:
		return slFollowScaredGus;

	case SCHED_TARGET_FACE_SCARED:
		return slFaceTargetScaredGus;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slIdleSciStandGus;
		else
			return psched;

	case SCHED_HIDE:
		return slScientistHideGus;

	case SCHED_STARTLE:
		return slScientistStartleGus;

	case SCHED_FEAR:
		return slFearGus;
	}

	return CTalkMonster::GetScheduleOfType(Type);
}

Schedule_t *CGus::GetSchedule(void)
{
	// so we don't keep calling through the EHANDLE stuff
	CBaseEntity *pEnemy = m_hEnemy;

	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (pEnemy)
		{
			if (HasConditions(bits_COND_SEE_ENEMY))
				m_fearTime = gpGlobals->time;
			else if (DisregardEnemy(pEnemy))		// After 15 seconds of being hidden, return to alert
			{
				m_hEnemy = NULL;
				pEnemy = NULL;
			}
		}

		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		// Cower when you hear something scary
		if (HasConditions(bits_COND_HEAR_SOUND))
		{
			CSound *pSound;
			pSound = PBestSound();

			ASSERT(pSound != NULL);
			if (pSound)
			{
				if (pSound->m_iType & (bits_SOUND_DANGER | bits_SOUND_COMBAT))
				{
					if (gpGlobals->time - m_fearTime > 3)	// Only cower every 3 seconds or so
					{
						m_fearTime = gpGlobals->time;		// Update last fear
						return GetScheduleOfType(SCHED_STARTLE);	// This will just duck for a second
					}
				}
			}
		}

		// Behavior for following the player
		if (IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(FALSE);
				break;
			}

			int relationship = R_NO;

			// Nothing scary, just me and the player
			if (pEnemy != NULL)
				relationship = IRelationship(pEnemy);

			// UNDONE: Model fear properly, fix R_FR and add multiple levels of fear
			if (relationship != R_DL && relationship != R_HT)
			{
				// If I'm already close enough to my target
				if (TargetDistance() <= 128)
				{
					if (CanHeal())	// Heal opportunistically
						return slHealGus;
					if (HasConditions(bits_COND_CLIENT_PUSH))	// Player wants me to move
						return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);	// Just face and follow.
			}
			else	// UNDONE: When afraid, scientist won't move out of your way.  Keep This?  If not, write move away scared
			{
				if (HasConditions(bits_COND_NEW_ENEMY)) // I just saw something new and scary, react
					return GetScheduleOfType(SCHED_FEAR);					// React to something scary
				return GetScheduleOfType(SCHED_TARGET_FACE_SCARED);	// face and follow, but I'm scared!
			}
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))	// Player wants me to move
			return GetScheduleOfType(SCHED_MOVE_AWAY);

		// try to say something about smells
		TrySmellTalk();
		break;
	case MONSTERSTATE_COMBAT:
		if (HasConditions(bits_COND_NEW_ENEMY))
			return slFearGus;					// Point and scream!
		if (HasConditions(bits_COND_SEE_ENEMY))
			return slScientistCoverGus;		// Take Cover

		if (HasConditions(bits_COND_HEAR_SOUND))
			return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

		return slScientistCoverGus;			// Run & Cower
		break;
	}

	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CGus::GetIdealState(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			if (IsFollowing())
			{
				int relationship = IRelationship(m_hEnemy);
				if (relationship != R_FR || relationship != R_HT && !HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
				{
					// Don't go to combat if you're following the player
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}
				StopFollowing(TRUE);
			}
		}
		else if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// Stop following if you take damage
			if (IsFollowing())
				StopFollowing(TRUE);
		}
		break;

	case MONSTERSTATE_COMBAT:
	{
								CBaseEntity *pEnemy = m_hEnemy;
								if (pEnemy != NULL)
								{
									if (DisregardEnemy(pEnemy))		// After 15 seconds of being hidden, return to alert
									{
										// Strip enemy when going to alert
										m_IdealMonsterState = MONSTERSTATE_ALERT;
										m_hEnemy = NULL;
										return m_IdealMonsterState;
									}
									// Follow if only scared a little
									if (m_hTargetEnt != NULL)
									{
										m_IdealMonsterState = MONSTERSTATE_ALERT;
										return m_IdealMonsterState;
									}

									if (HasConditions(bits_COND_SEE_ENEMY))
									{
										m_fearTime = gpGlobals->time;
										m_IdealMonsterState = MONSTERSTATE_COMBAT;
										return m_IdealMonsterState;
									}

								}
	}
		break;
	}

	return CTalkMonster::GetIdealState();
}


BOOL CGus::CanHeal(void)
{
	if ((m_healTime > gpGlobals->time) || (m_hTargetEnt == NULL) || (m_hTargetEnt->pev->health > (m_hTargetEnt->pev->max_health * 0.5)))
		return FALSE;

	return TRUE;
}

void CGus::Heal(void)
{
	if (!CanHeal())
		return;

	Vector target = m_hTargetEnt->pev->origin - pev->origin;
	if (target.Length() > 100)
		return;

	m_hTargetEnt->TakeHealth(gSkillData.scientistHeal, DMG_GENERIC);
	// Don't heal again for 1 minute
	m_healTime = gpGlobals->time + 60;
}

int CGus::FriendNumber(int arrayNumber)
{
	static int array[3] = { 1, 2, 0 };
	if (arrayNumber < 3)
		return array[arrayNumber];
	return arrayNumber;
}


//=========================================================
// Dead Scientist PROP
//=========================================================
class CDeadGus : public CBaseMonster
{
public:
	void Spawn(void);
	int	Classify(void) { return	CLASS_HUMAN_PASSIVE; }

	void KeyValue(KeyValueData *pkvd);
	int	m_iPose;// which sequence to display
	static char *m_szPoses[7];
};
char *CDeadGus::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

void CDeadGus::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}
LINK_ENTITY_TO_CLASS(monster_gus_dead, CDeadGus);

//
// ********** DeadScientist SPAWN **********
//
void CDeadGus::Spawn()
{
	PRECACHE_MODEL("models/gus.mdl");
	SET_MODEL(ENT(pev), "models/gus.mdl");

	pev->effects = 0;
	pev->sequence = 0;
	// Corpses have less health
	pev->health = 8;//gSkillData.scientistHealth;

	m_bloodColor = BLOOD_COLOR_RED;

	if (pev->body == -1)
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS - 1);// pick a head, any head
	}
	// Luther is black, make his hands black
	if (pev->body == HEAD_LUTHER)
		pev->skin = 1;
	else
		pev->skin = 0;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead scientist with bad pose\n");
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}


//=========================================================
// Sitting Scientist PROP
//=========================================================

class CSittingGus : public CGus // kdb: changed from public CBaseMonster so he can speak
{
public:
	void Spawn(void);
	void  Precache(void);

	void EXPORT SittingThink(void);
	int	Classify(void);
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	virtual void SetAnswerQuestion(CTalkMonster *pSpeaker);
	int FriendNumber(int arrayNumber);

	int FIdleSpeak(void);
	int		m_baseSequence;
	int		m_headTurn;
	float	m_flResponseDelay;
};

LINK_ENTITY_TO_CLASS(monster_sitting_gus, CSittingGus);
TYPEDESCRIPTION	CSittingGus::m_SaveData[] =
{
	// Don't need to save/restore m_baseSequence (recalced)
	DEFINE_FIELD(CSittingGus, m_headTurn, FIELD_INTEGER),
	DEFINE_FIELD(CSittingGus, m_flResponseDelay, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CSittingGus, CGus);

// animation sequence aliases 
typedef enum
{
	SITTING_ANIM_sitlookleft,
	SITTING_ANIM_sitlookright,
	SITTING_ANIM_sitscared,
	SITTING_ANIM_sitting2,
	SITTING_ANIM_sitting3
} SITTING_ANIM;


//
// ********** Scientist SPAWN **********
//
void CSittingGus::Spawn()
{
	PRECACHE_MODEL("models/gus.mdl");
	SET_MODEL(ENT(pev), "models/gus.mdl");
	Precache();
	InitBoneControllers();

	UTIL_SetSize(pev, Vector(-14, -14, 0), Vector(14, 14, 36));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->effects = 0;
	pev->health = 50;

	m_bloodColor = BLOOD_COLOR_RED;
	m_flFieldOfView = VIEW_FIELD_WIDE; // indicates the width of this monster's forward view cone ( as a dotproduct result )

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD;

	SetBits(pev->spawnflags, SF_MONSTER_PREDISASTER); // predisaster only!

	if (pev->body == -1)
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS - 1);// pick a head, any head
	}
	// Luther is black, make his hands black
	if (pev->body == HEAD_LUTHER)
		pev->skin = 1;

	m_baseSequence = LookupSequence("sitlookleft");
	pev->sequence = m_baseSequence + RANDOM_LONG(0, 4);
	ResetSequenceInfo();

	SetThink(&CSittingGus::SittingThink);
	pev->nextthink = gpGlobals->time + 0.1;

	DROP_TO_FLOOR(ENT(pev));
}

void CSittingGus::Precache(void)
{
	m_baseSequence = LookupSequence("sitlookleft");
	TalkInit();
}

//=========================================================
// ID as a passive human
//=========================================================
int	CSittingGus::Classify(void)
{
	return	CLASS_HUMAN_PASSIVE;
}


int CSittingGus::FriendNumber(int arrayNumber)
{
	static int array[3] = { 2, 1, 0 };
	if (arrayNumber < 3)
		return array[arrayNumber];
	return arrayNumber;
}



//=========================================================
// sit, do stuff
//=========================================================
void CSittingGus::SittingThink(void)
{
	CBaseEntity *pent;

	StudioFrameAdvance();

	// try to greet player
	if (FIdleHello())
	{
		pent = FindNearestFriend(TRUE);
		if (pent)
		{
			float yaw = VecToYaw(pent->pev->origin - pev->origin) - pev->angles.y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;

			if (yaw > 0)
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
			else
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;

			ResetSequenceInfo();
			pev->frame = 0;
			SetBoneController(0, 0);
		}
	}
	else if (m_fSequenceFinished)
	{
		int i = RANDOM_LONG(0, 99);
		m_headTurn = 0;

		if (m_flResponseDelay && gpGlobals->time > m_flResponseDelay)
		{
			// respond to question
			IdleRespond();
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
			m_flResponseDelay = 0;
		}
		else if (i < 30)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;

			// turn towards player or nearest friend and speak

			if (!FBitSet(m_bitsSaid, bit_saidHelloPlayer))
				pent = FindNearestFriend(TRUE);
			else
				pent = FindNearestFriend(FALSE);

			if (!FIdleSpeak() || !pent)
			{
				m_headTurn = RANDOM_LONG(0, 8) * 10 - 40;
				pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			}
			else
			{
				// only turn head if we spoke
				float yaw = VecToYaw(pent->pev->origin - pev->origin) - pev->angles.y;

				if (yaw > 180) yaw -= 360;
				if (yaw < -180) yaw += 360;

				if (yaw > 0)
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
				else
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;

				//ALERT(at_console, "sitting speak\n");
			}
		}
		else if (i < 60)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			m_headTurn = RANDOM_LONG(0, 8) * 10 - 40;
			if (RANDOM_LONG(0, 99) < 5)
			{
				//ALERT(at_console, "sitting speak2\n");
				FIdleSpeak();
			}
		}
		else if (i < 80)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting2;
		}
		else if (i < 100)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
		}

		ResetSequenceInfo();
		pev->frame = 0;
		SetBoneController(0, m_headTurn);
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

// prepare sitting scientist to answer a question
void CSittingGus::SetAnswerQuestion(CTalkMonster *pSpeaker)
{
	m_flResponseDelay = gpGlobals->time + RANDOM_FLOAT(3, 4);
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
}


//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
int CSittingGus::FIdleSpeak(void)
{
	// try to start a conversation, or make statement
	int pitch;

	if (!FOkToSpeak())
		return FALSE;

	// set global min delay for next conversation
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);

	pitch = GetVoicePitch();

	// if there is a friend nearby to speak to, play sentence, set friend's response time, return

	// try to talk to any standing or sitting scientists nearby
	CBaseEntity *pentFriend = FindNearestFriend(FALSE);

	if (pentFriend && RANDOM_LONG(0, 1))
	{
		CTalkMonster *pTalkMonster = GetClassPtr((CTalkMonster *)pentFriend->pev);
		pTalkMonster->SetAnswerQuestion(this);

		IdleHeadTurn(pentFriend->pev->origin);
		SENTENCEG_PlayRndSz(ENT(pev), m_szGrp[TLK_PQUESTION], 1.0, ATTN_IDLE, 0, pitch);
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}

	// otherwise, play an idle statement
	if (RANDOM_LONG(0, 1))
	{
		SENTENCEG_PlayRndSz(ENT(pev), m_szGrp[TLK_PIDLE], 1.0, ATTN_IDLE, 0, pitch);
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}

	// never spoke
	CTalkMonster::g_talkWaitTime = 0;
	return FALSE;
}
