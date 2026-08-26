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

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )
#define		BARNEY_BAT_ATTACK	( 5 )
#define		BARNEY_BAT_HOLSTER	( 11 )
#define		BARNEY_BEER_CRACK	( 12 )
#define		BARNEY_BEER_THROW	( 13 )
#define		BARNEY_BEER_GET		( 8 )

#define	BARNEY_BODY_GUNHOLSTERED	0
#define	BARNEY_BODY_GUNDRAWN		1
#define BARNEY_BODY_BAT				2
#define BARNEY_BODY_PIWO			3
#define BARNEY_BODY_GUNGONE			4

#define BOLT_AIR_VELOCITY	1800
#define BOLT_WATER_VELOCITY	1200

#ifndef CLIENT_DLL
class CBarnpiwo : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT ExplodeThink(void);
	int touchcounter = 0;

	int m_iTrail;

public:
	static CBarnpiwo *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(barnpiwo, CBarnpiwo);

CBarnpiwo *CBarnpiwo::BoltCreate(void)
{
	// Create a new entity with CCrossbowBolt private data
	CBarnpiwo *pBolt = GetClassPtr((CBarnpiwo *)NULL);
	pBolt->pev->classname = MAKE_STRING("barnpiwo");
	pBolt->Spawn();
	pBolt->touchcounter = 0;

	return pBolt;
}

void CBarnpiwo::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.8;

	SET_MODEL(ENT(pev), "models/w_piwo.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));

	SetTouch(&CBarnpiwo::BoltTouch);
	SetThink(&CBarnpiwo::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.2;
}


void CBarnpiwo::Precache()
{
	PRECACHE_MODEL("models/w_piwo.mdl");
	PRECACHE_SOUND("debris/can.wav");
}


int	CBarnpiwo::Classify(void)
{
	return	CLASS_NONE;
}

void CBarnpiwo::BoltTouch(CBaseEntity *pOther)
{
	//pev->angles.x = 0;
	//pev->angles.z = 0;
	touchcounter++;

	if (touchcounter == 1)
	{
		SetThink(&CBarnpiwo::ExplodeThink);
		pev->nextthink = gpGlobals->time + 3;
	}


	if (touchcounter > 100)
		UTIL_Remove(this);

	if (pev->velocity.z == 0)
	{
		UTIL_Remove(this);
	}

	if (pOther->edict() == pev->owner)
		return;

	if (FClassnameIs(pOther->pev, "player"))
		return;

	// pev->avelocity = Vector (300, 300, 300);

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.15;

		if (pev->velocity <= Vector(1, 1, 1))
		{
			UTIL_Remove(this);
		}
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "debris/can.wav", 0.35, ATTN_IDLE, 0, 98 + RANDOM_LONG(0, 7));
	}
}

void CBarnpiwo::BubbleThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}

void CBarnpiwo::ExplodeThink(void)
{
	UTIL_Remove(this);
}
#endif

class CBarney : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int ISoundMask( void );
	void BarneyFirePistol( void );
	void AlertSound( void );
	int Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void GetBat(void);
	void BarneyBatAttack(void);
	void GetGun(void);

	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	virtual int ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckMeleeAttack1(float flDot, float flDist);

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType( int Type );
	Schedule_t *GetSchedule( void );
	MONSTERSTATE GetIdealState( void );

	void DeathSound( void );
	void PainSound( void );

	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fGunDrawn;
	BOOL m_fBatDrawn;
	float m_painTime;
	float m_checkAttackTime;
	BOOL m_lastAttackCheck;

	// UNDONE: What is this for?  It isn't used?
	float m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES
};

LINK_ENTITY_TO_CLASS( monster_barney, CBarney )

TYPEDESCRIPTION	CBarney::m_SaveData[] =
{
	DEFINE_FIELD( CBarney, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney, m_fBatDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBarney, CTalkMonster )

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t tlBaFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, (float)128 },	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE },
};

Schedule_t slBaFollow[] =
{
	{
		tlBaFollow,
		ARRAYSIZE( tlBaFollow ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// BarneyDraw - much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t tlBarneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float) ACT_ARM },
};

Schedule_t slBarneyEnemyDraw[] =
{
	{
		tlBarneyEnemyDraw,
		ARRAYSIZE( tlBarneyEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t tlBaFaceTarget[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};

Schedule_t slBaFaceTarget[] =
{
	{
		tlBaFaceTarget,
		ARRAYSIZE( tlBaFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};

Task_t tlIdleBaStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 }, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET, (float)0 }, // reset head position
};

Schedule_t slIdleBaStand[] =
{
	{
		tlIdleBaStand,
		ARRAYSIZE( tlIdleBaStand ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_PROVOKED,
		bits_SOUND_COMBAT |// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER |
		//bits_SOUND_WORLD |
		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES( CBarney )
{
	slBaFollow,
	slBarneyEnemyDraw,
	slBaFaceTarget,
	slIdleBaStand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBarney, CTalkMonster )

void CBarney::StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );	
}

void CBarney::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if( m_hEnemy != 0 && ( m_hEnemy->IsPlayer() ) )
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

const char *CBarney::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CBarney::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CBarney::ISoundMask( void) 
{
	return bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CBarney::Classify( void )
{
	return CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CBarney::AlertSound( void )
{
	if( m_hEnemy != 0 )
	{
		if( FOkToSpeak() )
		{
			PlaySentence( "BA_ATTACK", RANDOM_FLOAT( 2.8f, 3.2f ), VOL_NORM, ATTN_IDLE );
		}
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBarney::SetYawSpeed( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 180;
		break;
	case ACT_WALK:
		ys = 270;
		break;
	case ACT_RUN:
		ys = 270;
		break;
	default:
		ys = 270;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBarney::CheckRangeAttack1( float flDot, float flDist )
{
	if( flDist <= 1024.0f && flDot >= 0.5f && flDist >= 200.0f )
	{
		if (m_fBatDrawn)
		{
			GetBat();
			return FALSE;
		}
		if( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector( 0.0f, 0.0f, 55.0f );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( ( pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin ) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT( pev ), &tr );
			m_checkAttackTime = gpGlobals->time + 1.0f;
			if( tr.flFraction == 1.0f || ( tr.pHit != NULL && CBaseEntity::Instance( tr.pHit ) == pEnemy ) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5f;
		}
		return m_lastAttackCheck;
	}
	else if (flDist < 200.0f)
	{
		if (!m_fBatDrawn)
			GetBat();
		else
			return FALSE;
	}
	return FALSE;
}

BOOL CBarney::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 70.0f && flDot >= 0.7f && m_fBatDrawn)
		return TRUE;
	return FALSE;
}


void CBarney::GetBat(void)
{
	pev->sequence = LookupSequence("getbat");
	pev->frame = 0;
	pev->framerate = 1;
	ResetSequenceInfo();
	if (!m_fBatDrawn)
	{
		m_fBatDrawn = TRUE;
		m_fGunDrawn = FALSE;
		pev->body = BARNEY_BODY_BAT;
	}
	else
	{
		m_fBatDrawn = FALSE;
		m_fGunDrawn = FALSE;
		pev->body = BARNEY_BODY_GUNHOLSTERED;
	}
	//pev->nextthink = gpGlobals->time + 0.6;
}


void CBarney::BarneyBatAttack(void)
{
	// do stuff for this event.
	CBaseEntity *pHurt = CheckTraceHullAttack(70.0f, 15.0f, DMG_CRUSH);
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
}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CBarney::BarneyFirePistol( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors( pev->angles );
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM );

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "barney/ba_attack2.wav", 1.0f, ATTN_NORM, 0, 100 + pitchShift );

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3f );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBarney::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		BarneyFirePistol();
		break;
	case BARNEY_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		m_fBatDrawn = false;
		pev->body = BARNEY_BODY_GUNDRAWN;
		m_fGunDrawn = TRUE;
		break;
	case BARNEY_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		pev->body = BARNEY_BODY_GUNHOLSTERED;
		m_fGunDrawn = FALSE;
		break;
	case BARNEY_BAT_HOLSTER:
	{

	}
		break;

	case BARNEY_BEER_GET:
	{
		pev->body = BARNEY_BODY_PIWO;
		break;
	}
	case BARNEY_BEER_CRACK:
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/piwo.wav", 0.5, ATTN_IDLE, 0, 100);
		break;
	}
	case BARNEY_BEER_THROW:
	{
		if (m_fBatDrawn)
			pev->body = BARNEY_BODY_BAT;
		else if (m_fGunDrawn)
			pev->body = BARNEY_BODY_GUNDRAWN;
		else
			pev->body = BARNEY_BODY_GUNHOLSTERED;

		//THROW CAN HERE
#ifndef CLIENT_DLL
		UTIL_MakeVectors(pev->angles);
		CBarnpiwo *pBolt = CBarnpiwo::BoltCreate();
		pBolt->pev->origin = pev->origin + gpGlobals->v_up * 32 - gpGlobals->v_right * 16 + gpGlobals->v_forward * 8;

		pBolt->pev->movetype = MOVETYPE_BOUNCE;
		pBolt->pev->gravity = 0.5;
		pBolt->pev->friction = 0.8;
		pBolt->pev->angles = pev->angles;
		pBolt->pev->owner = edict();

		pBolt->pev->velocity = pev->velocity + gpGlobals->v_forward * 128 + gpGlobals->v_up * 24 + gpGlobals->v_right * 16;
		pBolt->pev->speed = 12;

		pBolt->pev->avelocity.x = -600;
		pBolt->pev->avelocity.y = RANDOM_LONG(-300, 500);
		pBolt->pev->avelocity.z = RANDOM_LONG(-100, 200);
#endif
		switch (RANDOM_LONG(0, 2))
		{
			case 0: 
				EMIT_SOUND(ENT(pev), CHAN_VOICE, "generic/burp.wav", 0.5, ATTN_IDLE); 
				break;
			case 1: 
				EMIT_SOUND(ENT(pev), CHAN_VOICE, "generic/burp2.wav", 0.5, ATTN_IDLE); 
				break;
			case 2: 
				EMIT_SOUND(ENT(pev), CHAN_VOICE, "generic/burp3.wav", 0.5, ATTN_IDLE); 
				break;
		}
		break;
	}
	case BARNEY_BAT_ATTACK:
		BarneyBatAttack();
		break;
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarney::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/barney.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.barneyHealth;
	pev->view_ofs = Vector( 0.0f, 0.0f, 50.0f );// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	pev->body = 0; // gun in holster
	m_fGunDrawn = FALSE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse( &CTalkMonster::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarney::Precache()
{
	PRECACHE_MODEL( "models/barney.mdl" );

	PRECACHE_SOUND( "barney/ba_attack1.wav" );
	PRECACHE_SOUND( "barney/ba_attack2.wav" );

	PRECACHE_SOUND( "barney/ba_pain1.wav" );
	PRECACHE_SOUND( "barney/ba_pain2.wav" );
	PRECACHE_SOUND( "barney/ba_pain3.wav" );

	PRECACHE_SOUND( "barney/ba_die1.wav" );
	PRECACHE_SOUND( "barney/ba_die2.wav" );
	PRECACHE_SOUND( "barney/ba_die3.wav" );

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void CBarney::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)
	m_szGrp[TLK_ANSWER] = "BA_ANSWER";
	m_szGrp[TLK_QUESTION] =	"BA_QUESTION";
	m_szGrp[TLK_IDLE] = "BA_IDLE";
	m_szGrp[TLK_STARE] = "BA_STARE";
	m_szGrp[TLK_USE] = "BA_OK";
	m_szGrp[TLK_UNUSE] = "BA_WAIT";
	m_szGrp[TLK_STOP] = "BA_STOP";

	m_szGrp[TLK_NOSHOOT] = "BA_SCARED";
	m_szGrp[TLK_HELLO] = "BA_HELLO";

	m_szGrp[TLK_PLHURT1] = "!BA_CUREA";
	m_szGrp[TLK_PLHURT2] = "!BA_CUREB"; 
	m_szGrp[TLK_PLHURT3] = "!BA_CUREC";

	m_szGrp[TLK_PHELLO] = NULL;	//"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] = NULL;	//"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "BA_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] = "BA_SMELL";
	
	m_szGrp[TLK_WOUND] = "BA_WOUND";
	m_szGrp[TLK_MORTAL] = "BA_MORTAL";

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}

static BOOL IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = reference - pevTest->origin;
	vecDir.z = 0.0f;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0.0f;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );

	// He's facing me, he meant it
	if( DotProduct( forward, vecDir ) > 0.96f )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}

int CBarney::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	if( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if( m_MonsterState != MONSTERSTATE_PRONE && ( pevAttacker->flags & FL_CLIENT ) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if( m_hEnemy == 0 )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if( ( m_afMemory & bits_MEMORY_SUSPICIOUS ) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				PlaySentence( "BA_MAD", 4, VOL_NORM, ATTN_NORM );

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if( !( m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

//=========================================================
// PainSound
//=========================================================
void CBarney::PainSound( void )
{
	if( gpGlobals->time < m_painTime )
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

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5f, 0.75f );

	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain1.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain2.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain3.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CBarney::DeathSound( void )
{
	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die1.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die2.wav", 1.0f, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

void CBarney::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	switch( ptr->iHitgroup )
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_BLAST ) )
		{
			flDamage = flDamage * 0.5f;
		}
		break;
	case 10:
		if( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB ) )
		{
			flDamage -= 20.0f;
			if( flDamage <= 0.0f )
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0f );
				flDamage = 0.01f;
			}
		}

		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

void CBarney::Killed( entvars_t *pevAttacker, int iGib )
{
	if( pev->body < BARNEY_BODY_GUNGONE )
	{
		// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		pev->body = BARNEY_BODY_GUNGONE;

		GetAttachment( 0, vecGunPos, vecGunAngles );

		DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
	}

	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Schedule_t *CBarney::GetScheduleOfType( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if( m_hEnemy != 0 )
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType( Type );

		if( psched == slIdleStand )
			return slBaFaceTarget;	// override this for different target face behavior
		else
			return psched;
	case SCHED_TARGET_CHASE:
		return slBaFollow;
	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType( Type );

		if( psched == slIdleStand )
		{
			// just look straight ahead.
			return slIdleBaStand;
		}
		else
			return psched;	
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CBarney::GetSchedule( void )
{
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		PlaySentence( "BA_KILL", 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster::GetSchedule();
			}

			// always act surprized with a new enemy
			if( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE ) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );

			// wait for one schedule to draw gun
			if( !m_fGunDrawn && !m_fBatDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if( m_hEnemy == 0 && IsFollowing() )
		{
			if( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	default:
		break;
	}

	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CBarney::GetIdealState( void )
{
	return CTalkMonster::GetIdealState();
}

void CBarney::DeclineFollowing( void )
{
	PlaySentence( "BA_POK", 2, VOL_NORM, ATTN_NORM );
}

//=========================================================
// DEAD BARNEY PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadBarney : public CBaseMonster
{
public:
	void Spawn( void );
	int Classify( void ) { return CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int m_iPose;// which sequence to display	-- temporary, don't need to save
	static const char *m_szPoses[3];
};

const char *CDeadBarney::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

void CDeadBarney::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_barney_dead, CDeadBarney )

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadBarney::Spawn()
{
	PRECACHE_MODEL( "models/barney.mdl" );
	SET_MODEL( ENT( pev ), "models/barney.mdl" );

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if( pev->sequence == -1 )
	{
		ALERT( at_console, "Dead barney with bad pose\n" );
	}
	// Corpses have less health
	pev->health = 8;//gSkillData.barneyHealth;

	MonsterInitDead();
}
