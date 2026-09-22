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
#include	"animation.h"
//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is OTIS dying for scripted sequences?
#define		OTIS_AE_DRAW		( 2 )
#define		OTIS_AE_SHOOT		( 3 )
#define		OTIS_AE_HOLSTER	( 4 )
#define		OTIS_AE_RELOAD	( 5 )
#define		OTIS_AE_MELEE		( 6 )

#define	OTIS_BODY_GUNHOLSTERED	0
#define	OTIS_BODY_GUNDRAWN		1
#define OTIS_BODY_GUNGONE			2

enum
{
	SCHED_OTIS_COVER_AND_RELOAD = LAST_COMMON_SCHEDULE + 1,
	SCHED_OTIS_TAKECOVER_FAILED,
};


class COTIS : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void OTISFirePistol( void );
	void AlertSound( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	virtual int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

	void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );
	void SetActivity ( Activity NewActivity );

	void CheckAmmo ( void );
	void RunAI( void );

	void TalkInit( void );
	int IgnoreConditions ( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	int     m_ammo_not_full;
	int     m_ammo_next_reload;

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_otis, COTIS );

TYPEDESCRIPTION	COTIS::m_SaveData[] = 
{
	DEFINE_FIELD( COTIS, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( COTIS, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( COTIS, m_ammo_not_full, FIELD_INTEGER ),
	DEFINE_FIELD( COTIS, m_ammo_next_reload, FIELD_INTEGER ),
	DEFINE_FIELD( COTIS, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( COTIS, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( COTIS, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( COTIS, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlOtisFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)100		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slOtisFollow[] =
{
	{
		tlOtisFollow,
		ARRAYSIZE ( tlOtisFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};


Task_t	tl_OTIS_TakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_OTIS_TAKECOVER_FAILED},
	{ TASK_WAIT,					(float)0.1							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_FACE_ENEMY,				(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
};

Schedule_t	sl_OTIS_TakeCover[] =
{
	{ 
		tl_OTIS_TakeCover1,
		ARRAYSIZE ( tl_OTIS_TakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

Task_t	tl_OTIS_HideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t sl_OTIS_HideReload[] = 
{
	{
		tl_OTIS_HideReload,
		ARRAYSIZE ( tl_OTIS_HideReload ),
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_NEW_ENEMY,

		bits_SOUND_DANGER,
		"TakeCover"
	}
};

Task_t	tlOTISWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slOTISWaitInCover[] =
{
	{ 
		tlOTISWaitInCover,
		ARRAYSIZE ( tlOTISWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"OTISWaitInCover"
	},
};

//=========================================================
// OTISDraw- much better looking draw schedule for when
// OTIS knows who he's gonna attack.
//=========================================================
Task_t	tlOTISEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slOTISEnemyDraw[] = 
{
	{
		tlOTISEnemyDraw,
		ARRAYSIZE ( tlOTISEnemyDraw ),
		0,
		0,
		"OTIS Enemy Draw"
	}
};

Task_t	tlOtisFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slOtisFaceTarget[] =
{
	{
		tlOtisFaceTarget,
		ARRAYSIZE ( tlOtisFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleOtisStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleOtisStand[] =
{
	{ 
		tlIdleOtisStand,
		ARRAYSIZE ( tlIdleOtisStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
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

DEFINE_CUSTOM_SCHEDULES( COTIS )
{
	slOtisFollow,
	slOTISEnemyDraw,
	slOtisFaceTarget,
	slIdleOtisStand,
	slOTISWaitInCover,
	sl_OTIS_HideReload,
	sl_OTIS_TakeCover,
};


IMPLEMENT_CUSTOM_SCHEDULES( COTIS, CTalkMonster );

void COTIS :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );	
}

void COTIS :: RunTask( Task_t *pTask )
{
	CTalkMonster::RunTask( pTask );
}


//=========================================================
// RunAI
//=========================================================
void COTIS :: RunAI( void )
{
	CBaseMonster :: RunAI();

		if(m_killbyheadcrab >= 1){
				if ( GetBodygroup( 3 ) == 0 ){
				SetBodygroup( 3, 1 );
				}
				m_crabzombie_begain += 1;
				if(m_crabzombie_begain >= 60){
					if (IsMoving()){
					goto fail_check;
					}
					Vector trace_origin;
					trace_origin = pev->origin + Vector(0,0,38);
					TraceResult trace;
					UTIL_TraceHull(trace_origin, trace_origin, dont_ignore_monsters, human_hull, ENT(pev),&trace);
					if ( trace.fStartSolid )
					{
					goto fail_check;
					}
					SpawnBlood(Center(), BloodColor(), 250);
					CBaseEntity *pZombie = Create( "monster_zombie_barney", pev->origin, pev->angles, edict() );
					pZombie->pev->angles.x = 0;
					pZombie->pev->angles.z = 0;
					CBaseMonster *pMonster = pZombie->MyMonsterPointer( );
					pMonster->SetActivity( ACT_FALL );
					UTIL_Remove( this );
					return;
				}
		}
		if (pev->sequence == 4){
        m_flGroundSpeed = 80;
		}
		if (pev->sequence == 5){
        m_flGroundSpeed = 250;
		}
		
		if(pev->health == 114514){
		fail_check:
		m_crabzombie_begain = 40;
		}
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int COTIS :: ISoundMask ( void) 
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
int	COTIS :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// ALertSound - OTIS says "Freeze!"
//=========================================================
void COTIS :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
			PlaySentence( "BA_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		}
	}

}


int COTIS::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	iIgnore |= bits_COND_LIGHT_DAMAGE;	

	return iIgnore;
	
}

void COTIS :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}

	m_ammo_not_full = m_cClipSize - m_cAmmoLoaded;
}

void COTIS :: SetActivity ( Activity NewActivity )
{
	int	iSequence;

	iSequence = LookupActivity ( NewActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		NewActivity = ACT_IDLE;

	CTalkMonster::SetActivity( NewActivity );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COTIS :: SetYawSpeed ( void )
{
	pev->yaw_speed = 150;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL COTIS :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( m_cAmmoLoaded <= 0 ){
	return FALSE;
	}

	float dist = 768;
	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 256 )
		{
		dist += 256;
		}
	}

	if (flDist <= dist && flDot >= 0.5 )
	{
			TraceResult	tr;

			if (flDist <= 90 )
			{
				if ( m_hEnemy != NULL )
				{
					Vector org1 = BodyTarget_c(pev->origin);
					Vector org2 = m_hEnemy->BodyTarget_c(m_hEnemy->pev->origin);
					if ( fabs( org1.z - org2.z ) <= 12 ){
					return FALSE;
					}
				}
			}

			if ( gpGlobals->time > m_checkAttackTime )
			{
				Vector vecShootOrigin,vecdir;
				vecShootOrigin = pev->origin + Vector( 0, 0, 45 );
				TraceResult tr;
				
				Vector shootOrigin = vecShootOrigin;
				CBaseEntity *pEnemy = m_hEnemy;
				Vector shootTarget = ( (pEnemy->BodyTarget_c( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
				UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
				if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) ){
					m_checkAttackTime = gpGlobals->time + 0.5;
					m_lastAttackCheck = TRUE;
				}
				else{
					if(CBaseEntity::Instance(tr.pHit)->Classify() == CLASS_ALIEN_MONSTER
					|| CBaseEntity::Instance(tr.pHit)->Classify() == CLASS_ALIEN_MILITARY
					|| CBaseEntity::Instance(tr.pHit)->Classify() == CLASS_HUMAN_ASS
					|| CBaseEntity::Instance(tr.pHit)->Classify() == CLASS_HUMAN_MILITARY){
					m_checkAttackTime = gpGlobals->time + 0.5;
					m_lastAttackCheck = TRUE;
					}
					else{
					m_lastAttackCheck = FALSE;
					m_checkAttackTime = gpGlobals->time + 1.0;
					}
				}
			}
			return m_lastAttackCheck;

	}
	return FALSE;
}

BOOL COTIS :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( HasConditions ( bits_COND_SEE_ENEMY ) && flDist <= 70 && flDot >= 0.6 && m_hEnemy != NULL )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// OTISFirePistol - shoots one round from the pistol at
// the enemy OTIS is facing.
//=========================================================
void COTIS :: OTISFirePistol ( void )
{
	if ( m_cAmmoLoaded <= 0 ){
	return;
	}

	m_ammo_next_reload = 0;

	Vector vecShootOrigin,vecdir;

	UTIL_MakeVectors(pev->angles);

	GetAttachment( 0, vecShootOrigin,vecdir);

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(6, vecShootOrigin, vecShootDir, Vector(0.1,0.08,0.08), 1024, BULLET_12MM,1);
	
	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "barney/sbarrel1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

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
void COTIS :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case OTIS_AE_SHOOT:
		OTISFirePistol();
		break;

	case OTIS_AE_DRAW:
		m_fGunDrawn = TRUE;
		break;

	case OTIS_AE_HOLSTER:
		m_fGunDrawn = FALSE;
		break;

	case OTIS_AE_RELOAD:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barney/spas12_insert.wav", 1, ATTN_NORM );
		m_cAmmoLoaded++;
		if(m_cAmmoLoaded >= m_cClipSize){
		m_cAmmoLoaded = m_cClipSize;
		m_ammo_next_reload = 0;
		}
		if(m_ammo_next_reload > 0){
		m_ammo_next_reload--;
		}
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case OTIS_AE_MELEE:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 90;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, 30, gpGlobals->v_forward, &tr, DMG_SLASH ); 
				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
				pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * 300;
				}
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 90, 30, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
					pHurt->pev->velocity = (pHurt->pev->origin - pev->origin).Normalize() * 300;
					}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

		}
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void COTIS :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/otis.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 100;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->body			= 0; // gun in holster
	m_fGunDrawn			= FALSE;

	m_lovehate          = 100;
	m_cClipSize	    	= 8;
	m_cAmmoLoaded		= m_cClipSize;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_canheadcrab_mode  = 1;
	pev->armortype		= 1;
	m_canbarnacle_mode  = 1;

	m_candrownwater = 1;

	MonsterInit();
	SetUse( &COTIS::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COTIS :: Precache()
{
	UTIL_PrecacheOther( "monster_zombie_barney" );

	PRECACHE_MODEL("models/otis.mdl");

	PRECACHE_SOUND("barney/sbarrel1.wav" );
	PRECACHE_SOUND("barney/spas12_insert.wav" );

	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");
	
	// every new OTIS must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void COTIS :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"BA_ANSWER";
	m_szGrp[TLK_QUESTION] =	"BA_QUESTION";
	m_szGrp[TLK_IDLE] =		"BA_IDLE";
	m_szGrp[TLK_STARE] =		"BA_STARE";
	m_szGrp[TLK_USE] =		"BA_OK";
	m_szGrp[TLK_UNUSE] =	"BA_WAIT";
	m_szGrp[TLK_STOP] =		"BA_STOP";

	m_szGrp[TLK_NOSHOOT] =	"BA_SCARED";
	m_szGrp[TLK_HELLO] =	"BA_HELLO";

	m_szGrp[TLK_PLHURT1] =	"!BA_CUREA";
	m_szGrp[TLK_PLHURT2] =	"!BA_CUREB"; 
	m_szGrp[TLK_PLHURT3] =	"!BA_CUREC";

	m_szGrp[TLK_PHELLO] =	NULL;	//"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] =	NULL;	//"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "BA_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] =	"BA_SMELL";
	
	m_szGrp[TLK_WOUND] =	"BA_WOUND";
	m_szGrp[TLK_MORTAL] =	"BA_MORTAL";

	// get voice for head - just one OTIS voice for now
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


int COTIS :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

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
				PlaySentence( "BA_MAD", 4, VOL_NORM, ATTN_NORM );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
void COTIS :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void COTIS :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


void COTIS::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch( ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		flDamage *= 0.8;
		break;
	case 1:
		flDamage *= 0.75;
		break;
	case 10:
		flDamage *= 0.75;
					MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos);
					WRITE_BYTE(TE_STREAK_SPLASH);
					WRITE_COORD(ptr->vecEndPos.x);
					WRITE_COORD(ptr->vecEndPos.y);
					WRITE_COORD(ptr->vecEndPos.z);
					WRITE_COORD(ptr->vecPlaneNormal.x);
					WRITE_COORD(ptr->vecPlaneNormal.y);
					WRITE_COORD(ptr->vecPlaneNormal.z);
					WRITE_BYTE(5);
					WRITE_SHORT(22);
					WRITE_SHORT(25);
					WRITE_SHORT(65);
					MESSAGE_END();
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void COTIS::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( GetBodygroup( 2 ) == 0 )
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup( 2, 1 );

		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun = DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );

		if ( pGun )
		{
			pGun->pev->velocity = pev->velocity + Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}


	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* COTIS :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slOTISEnemyDraw;
		}
		break;

	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		return &sl_OTIS_TakeCover[ 0 ];
	}

	case SCHED_OTIS_TAKECOVER_FAILED:
	{
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		}
		return GetScheduleOfType ( SCHED_FAIL );
	}

	case SCHED_OTIS_COVER_AND_RELOAD:
	{
		return &sl_OTIS_HideReload[ 0 ];
	}

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that OTIS will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slOtisFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slOtisFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleOtisStand;
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
Schedule_t *COTIS :: GetSchedule ( void )
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
		PlaySentence( "BA_KILL", 4, VOL_NORM, ATTN_NORM );
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
				
			// wait for one schedule to draw gun
		//	if (!m_fGunDrawn )
			//	return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions ( bits_COND_NO_AMMO_LOADED ))
			{
				m_ammo_next_reload = 8;
				return GetScheduleOfType ( SCHED_OTIS_COVER_AND_RELOAD );
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
			}

			if(m_ammo_next_reload > 0){
				if (m_ammo_not_full > 0 && m_IdealActivity != ACT_RELOAD)
				{
				return GetScheduleOfType ( SCHED_RELOAD_DEEP );
				}
			}

		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if (m_ammo_not_full > 0)
		{
		return GetScheduleOfType ( SCHED_RELOAD_DEEP );
		}

		if ( HasConditions(bits_COND_HEAVY_DAMAGE))
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
				else{
				return GetScheduleOfType( SCHED_TARGET_FACE );
				}
			}
		}
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
				return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE COTIS :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}

void COTIS::DeclineFollowing( void )
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
class CDeadOtis : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadOtis::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

void CDeadOtis::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_otis_dead, CDeadOtis );

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadOtis :: Spawn( )
{
	PRECACHE_MODEL("models/otis.mdl");
	SET_MODEL(ENT(pev), "models/otis.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead barney with bad pose\n" );
	}
	// Corpses have less health
	pev->health			= 20;//gSkillData.barneyHealth;

	SetBodygroup( 2, 1 );//û��ǹ

	MonsterInitDead();
}