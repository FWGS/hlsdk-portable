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
*	Monster was made by XF-Alien
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
#include	"plane.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		OTIS_AE_DRAW		( 2 )
#define		OTIS_AE_SHOOT		( 3 )
#define		OTIS_AE_HOLSTER		( 4 )

#define	OTIS_BODY_GUNHOLSTERED		0
#define	OTIS_BODY_GUNDRAWN			1
#define OTIS_BODY_GUNGONE			2

#define bits_COND_OTIS_NOFIRE	( bits_COND_SPECIAL1 )

enum
{
	TASK_OTIS_CHECK_FIRE = LAST_COMMON_TASK + 1,
};

class COtis : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void OtisFirePistol( void );
	void AlertSound( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	virtual int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL HeadGibbed;
	Vector HeadPos;
	int BuckshotCount;

	void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );
	void GibMonster( void );
	
	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );
	
	BOOL NoFriendlyFire();

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_otis, COtis );

TYPEDESCRIPTION	COtis::m_SaveData[] = 
{
	DEFINE_FIELD( COtis, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( COtis, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( COtis, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( COtis, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( COtis, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( COtis, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlOtFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slOtFollow[] =
{
	{
		tlOtFollow,
		ARRAYSIZE ( tlOtFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// OtisDraw- much better looking draw schedule for when
// otis knows who he's gonna attack.
//=========================================================
Task_t	tlOtisEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slOtisEnemyDraw[] = 
{
	{
		tlOtisEnemyDraw,
		ARRAYSIZE ( tlOtisEnemyDraw ),
		0,
		0,
		"Otis Enemy Draw"
	}
};

Task_t	tlOtFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slOtFaceTarget[] =
{
	{
		tlOtFaceTarget,
		ARRAYSIZE ( tlOtFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleOtStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleOtStand[] =
{
	{ 
		tlIdleOtStand,
		ARRAYSIZE ( tlIdleOtStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

// primary range attack
Task_t tlOtRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_OTIS_CHECK_FIRE, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t slOtRangeAttack1[] =
{
	{
		tlOtRangeAttack1,
		ARRAYSIZE( tlOtRangeAttack1 ),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED |
		bits_COND_OTIS_NOFIRE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES( COtis )
{
	slOtFollow,
	slOtisEnemyDraw,
	slOtFaceTarget,
	slIdleOtStand,
	slOtRangeAttack1,
};


IMPLEMENT_CUSTOM_SCHEDULES( COtis, CTalkMonster );

void COtis :: StartTask( Task_t *pTask )
{
	switch ( pTask->iTask ) {
	case TASK_OTIS_CHECK_FIRE:
		if( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_OTIS_NOFIRE );
		}
		TaskComplete();
		break;
	default:
		CTalkMonster::StartTask( pTask );
	}
}

void COtis :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
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




//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int COtis :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	COtis :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - otis says "Freeze!"
//=========================================================
void COtis :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak(SPEAK_DISREGARD_ENEMY) )
		{
			if (m_iszSpeakAs)
			{
				char szBuf[32];
				strcpy(szBuf,STRING(m_iszSpeakAs));
				strcat(szBuf,"_ATTACK");
				PlaySentence( szBuf, RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
			}
			else
			{
				PlaySentence( "OT_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
			}
		}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COtis :: SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 120;
		break;
	case ACT_WALK:
		ys = 120;
		break;
	case ACT_RUN:
		ys = 180;
		break;
	default:
		ys = 200;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL COtis :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;
			
			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}


//=========================================================
// OtisFirePistol - shoots one round from the pistol at
// the enemy otis is facing.
//=========================================================
void COtis :: OtisFirePistol ( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	if (pev->frags)
	{
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_OTIS_DEAGLE);
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/44_gun_fire.wav", 1, ATTN_NORM, 0, 100 );
	}
	else
	{
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_OTIS_DEAGLE );

		int pitchShift = RANDOM_LONG( 0, 20 );
	
		// Only shift about half the time
		if ( pitchShift > 10 )
			pitchShift = 0;
		else
			pitchShift -= 5;

		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/44_gun_fire.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
	}

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}
		
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void COtis :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case OTIS_AE_SHOOT:
		OtisFirePistol();
		break;

	case OTIS_AE_DRAW:
		SetBodygroup( 1, 1 );
		m_fGunDrawn = TRUE;
		break;

	case OTIS_AE_HOLSTER:
		SetBodygroup( 1, 0 );
		m_fGunDrawn = FALSE;
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void COtis :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/otis.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if (pev->health == 0) //LRC
		pev->health			= gSkillData.otisHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	SetBodygroup(1, 0); // gun in holster
	m_fGunDrawn			= FALSE;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse(&COtis :: FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COtis :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/otis.mdl");

	PRECACHE_SOUND("weapons/44_gun_fire.wav" );
	PRECACHE_SOUND("weapons/nosound.wav");
	
	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void COtis :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// otis speech group names (group names are in sentences.txt)

	if (!m_iszSpeakAs)
	{
		m_szGrp[TLK_IDLE]		=	"OT_IDLE";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER) //LRC
			m_szGrp[TLK_USE]	=	"OT_PFOLLOW";
		else
			m_szGrp[TLK_USE] =	"OT_OK";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_UNUSE] = "OT_PWAIT";
		else
			m_szGrp[TLK_UNUSE] = "OT_WAIT";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_DECLINE] =	"OT_POK";
		else
			m_szGrp[TLK_DECLINE] =	"OT_NOTOK";

		m_szGrp[TLK_NOSHOOT] =	"OT_SCARED";

		m_szGrp[TLK_PLHURT1] =	"!OT_CUREA";
		m_szGrp[TLK_PLHURT2] =	"!OT_CUREB"; 
		m_szGrp[TLK_PLHURT3] =	"!OT_CUREC";

		m_szGrp[TLK_PHELLO] =	NULL;	//"OT_PHELLO";		// UNDONE
		m_szGrp[TLK_PIDLE] =	NULL;	//"OT_PIDLE";			// UNDONE
	
		m_szGrp[TLK_WOUND] =	"OT_WOUND";
		m_szGrp[TLK_MORTAL] =	"OT_MORTAL";
	}

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}


static BOOL IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
	// He's facing me, he meant it
	if ( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


int COtis :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	// LRC - if my reaction to the player has been overridden, don't do this stuff
	if (m_iPlayerReact) return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf,STRING(m_iszSpeakAs));
					strcat(szBuf,"_MAD");
					PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
				}
				else
				{
					PlaySentence( "OT_MAD", 4, VOL_NORM, ATTN_NORM );
				}

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf,STRING(m_iszSpeakAs));
					strcat(szBuf,"_SHOT");
					PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
				}
				else
				{
					PlaySentence( "OT_SHOT", 4, VOL_NORM, ATTN_NORM );
				}
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			if (m_iszSpeakAs)
			{
				char szBuf[32];
				strcpy(szBuf,STRING(m_iszSpeakAs));
				strcat(szBuf,"_SHOT");
				PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
			}
			else
			{
				PlaySentence( "OT_SHOT", 4, VOL_NORM, ATTN_NORM );
			}
		}

	}

	if ( !HeadGibbed && (pev->health <= flDamage && BuckshotCount >= 5) ) // Hack to handle shotgun shells as each shell is a separate TraceAttack
	{
		SetBodygroup( 0, 1);

		GibHeadMonster( HeadPos, TRUE );
		HeadGibbed = TRUE;
	}

	BuckshotCount = 0;
	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
void COtis :: PainSound ( void )
{
}

//=========================================================
// DeathSound 
//=========================================================
void COtis :: DeathSound ( void )
{
}


void COtis::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch( ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			flDamage -= 20;
			if (flDamage <= 0 && !HeadGibbed)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
			else
			ptr->iHitgroup = HITGROUP_HEAD;
		}
		break;
	}

	if	( ptr->iHitgroup == 1 )
	{
		if ( (bitsDamageType & DMG_BULLET) && flDamage == gSkillData.plrDmgBuckshot )
			BuckshotCount++;

		Vector HeadPos = ptr->vecEndPos;

		if ( pev->health <= flDamage * gSkillData.monHead && flDamage >= 20 && !HeadGibbed )
		{
			SetBodygroup( 0, 1);

			GibHeadMonster( ptr->vecEndPos, TRUE );
			HeadGibbed = TRUE;
		}
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

void COtis :: GibMonster ( void )
{
	if ( !HeadGibbed )
		GibHeadMonster( Vector ( pev->origin.x, pev->origin.y, pev->origin.z + 32 ), TRUE );	
			 
	CTalkMonster :: GibMonster( );
}

void COtis::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( !(pev->spawnflags & SF_MONSTER_NO_WPN_DROP) && GetBodygroup( 1 ) != 2 )
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup( 1, 2 );
		GetAttachment( 0, vecGunPos, vecGunAngles );

		CBaseEntity *pGun;
		pGun = DropItem( "weapon_44desert_eagle", vecGunPos, vecGunAngles );
	}

	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* COtis :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slOtisEnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slOtFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slOtFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleOtStand;
		}
		else
			return psched;	
	case SCHED_RANGE_ATTACK1:
		return slOtRangeAttack1;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *COtis :: GetSchedule ( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		// Hey, be careful with that
		if (m_iszSpeakAs)
		{
			char szBuf[32];
			strcpy(szBuf,STRING(m_iszSpeakAs));
			strcat(szBuf,"_KILL");
			PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
		}
		else
		{
			PlaySentence( "OT_KILL", 4, VOL_NORM, ATTN_NORM );
		}
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if ( m_hEnemy == NULL && IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE COtis :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}



void COtis::DeclineFollowing( void )
{
	PlaySentence( m_szGrp[TLK_DECLINE], 2, VOL_NORM, ATTN_NORM ); //LRC
}

BOOL COtis::NoFriendlyFire()
{
	if( m_hEnemy != 0 )
	{
		UTIL_MakeVectors( UTIL_VecToAngles( m_hEnemy->Center() - pev->origin ) );
	}
	else
	{
		// if there's no enemy, pretend there's a friendly in the way, so the npc won't shoot.
		return FALSE;
	}

	CPlane backPlane;
	CPlane leftPlane;
	CPlane rightPlane;

	Vector vecLeftSide;
	Vector vecRightSide;
	Vector v_left;
	Vector v_dir;

	v_dir = gpGlobals->v_right * ( pev->size.x * 1.5f );
	vecLeftSide = pev->origin - v_dir;
	vecRightSide = pev->origin + v_dir;

	v_left = gpGlobals->v_right * -1.0f;

	leftPlane.InitializePlane( gpGlobals->v_right, vecLeftSide );
	rightPlane.InitializePlane( v_left, vecRightSide );
	backPlane.InitializePlane( gpGlobals->v_forward, pev->origin );

	for( int k = 1; k <= gpGlobals->maxClients; k++ )
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(k);
		if (pPlayer && pPlayer->IsPlayer() && IRelationship(pPlayer) == R_AL && pPlayer->IsAlive())
		{
			if( backPlane.PointInFront( pPlayer->pev->origin ) &&
				leftPlane.PointInFront( pPlayer->pev->origin ) &&
				rightPlane.PointInFront( pPlayer->pev->origin ) )
			{
				//ALERT(at_aiconsole, "%s: Ally player at fire plane!\n", STRING(pev->classname));
				// player is in the check volume! Don't shoot!
				if ((m_hEnemy->pev->origin - pev->origin).Length2D() > (pPlayer->pev->origin - pev->origin).Length2D())
				{
					//ALERT(at_aiconsole, "%s: Ally player is between enemy (%s) and myself. Don't shoot!\n", STRING(pev->classname), STRING(m_hEnemy->pev->classname));
					return FALSE;
				}
				else if (m_hEnemy->pev->deadflag == DEAD_DYING)
				{
					//ALERT(at_aiconsole, "%s: Enemy (%s) is dying and player is behind it. Stop shooting!\n", STRING(pev->classname), STRING(m_hEnemy->pev->classname));
					return FALSE;
				}
			}
		}
	}

	return TRUE;
}
