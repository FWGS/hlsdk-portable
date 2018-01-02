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
// SPForce
//=========================================================

//=========================================================
// Hit groups!	
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/


#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"

int g_fSPForceQuestion;				// true if an idle SPForce asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	SPFORCE_CLIP_SIZE					36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define SPFORCE_VOL						0.35		// volume of SPForce sounds
#define SPFORCE_ATTN						ATTN_NORM	// attenutation of SPForce sentences
#define SPFORCE_LIMP_HEALTH				20
#define SPFORCE_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a SPForce with a single headshot.
#define SPFORCE_NUM_HEADS				2 // how many SPForce heads are there? 
#define SPFORCE_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	SPFORCE_SENTENCE_VOLUME			(float)0.35 // volume of SPForce sentences

#define SPFORCE_M41A				( 1 << 0)
#define SPFORCE_HANDGRENADE			( 1 << 1)
#define SPFORCE_GRENADELAUNCHER		( 1 << 2)
#define SPFORCE_SHOTGUN				( 1 << 3)
#define SPFORCE_BERETTA				( 1 << 4)

#define HEAD_GROUP					1
#define HEAD_SPForce				0
#define HEAD_COMMANDER				1
#define HEAD_SHOTGUN				2
#define HEAD_M203					3
#define GUN_GROUP					3
#define GUN_M41A					0
#define GUN_SHOTGUN					1
#define GUN_BERETTA					2
#define GUN_NONE					3

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SPFORCE_AE_RELOAD		( 2 )
#define		SPFORCE_AE_KICK			( 3 )
#define		SPFORCE_AE_BURST1		( 4 )
#define		SPFORCE_AE_BURST2		( 5 ) 
#define		SPFORCE_AE_BURST3		( 6 ) 
#define		SPFORCE_AE_GREN_TOSS		( 7 )
#define		SPFORCE_AE_GREN_LAUNCH	( 8 )
#define		SPFORCE_AE_GREN_DROP		( 9 )
#define		SPFORCE_AE_CAUGHT_ENEMY	( 10) // SPForce established sight with an enemy (player only) that had previously eluded the squad.
#define		SPFORCE_AE_DROP_GUN		( 11) // SPForce (probably dead) is dropping his mp5.

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SPFORCE_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_SPFORCE_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_SPFORCE_COVER_AND_RELOAD,
	SCHED_SPFORCE_SWEEP,
	SCHED_SPFORCE_FOUND_ENEMY,
	SCHED_SPFORCE_REPEL,
	SCHED_SPFORCE_REPEL_ATTACK,
	SCHED_SPFORCE_REPEL_LAND,
	SCHED_SPFORCE_WAIT_FACE_ENEMY,
	SCHED_SPFORCE_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_SPFORCE_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_SPFORCE_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_SPFORCE_SPEAK_SENTENCE,
	TASK_SPFORCE_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_SPFORCE_NOFIRE	( bits_COND_SPECIAL1 )

class CSPForce : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	void IdleSound ( void );
	Vector GetGunPosition( void );
	void Shoot ( void );
	void Shotgun ( void );
	void PrescheduleThink ( void );
	void GibMonster( void );
	void SpeakSentence( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	CBaseEntity	*Kick( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int IRelationship ( CBaseEntity *pTarget );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;

	int m_voicePitch;

	int		m_iBrassShell;
	int		m_iShotgunShell;

	int		m_iSentence;

	static const char *pSPForceSentences[];
};

LINK_ENTITY_TO_CLASS( monster_human_spforce, CSPForce );

TYPEDESCRIPTION	CSPForce::m_SaveData[] = 
{
	DEFINE_FIELD( CSPForce, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CSPForce, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( CSPForce, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CSPForce, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CSPForce, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSPForce, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSPForce, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSPForce, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CSPForce, m_voicePitch, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CSPForce, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSPForce, CSquadMonster );

const char *CSPForce::pSPForceSentences[] = 
{
	"HG_GREN", // grenade scared SPForce
	"HG_ALERT", // sees player
	"HG_MONSTER", // sees monster
	"HG_COVER", // running to cover
	"HG_THROW", // about to throw grenade
	"HG_CHARGE",  // running out to get the enemy
	"HG_TAUNT", // say rude things
};

enum
{
	SPFORCE_SENT_NONE = -1,
	SPFORCE_SENT_GREN = 0,
	SPFORCE_SENT_ALERT,
	SPFORCE_SENT_MONSTER,
	SPFORCE_SENT_COVER,
	SPFORCE_SENT_THROW,
	SPFORCE_SENT_CHARGE,
	SPFORCE_SENT_TAUNT,
} SPFORCE_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some SPForce sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a SPForce says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the SPForce has 
// started moving.
//=========================================================
void CSPForce :: SpeakSentence( void )
{
	if ( m_iSentence == SPFORCE_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pSPForceSentences[ m_iSentence ], SPFORCE_SENTENCE_VOLUME, SPFORCE_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human grunt's nemesis.
//=========================================================
int CSPForce::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_alien_grunt" ) || ( FClassnameIs( pTarget->pev,  "monster_gargantua" ) ) )
	{
		return R_NM;
	}

	return CSquadMonster::IRelationship( pTarget );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CSPForce :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( GetBodygroup( 2 ) != 2 )
	{// throw a gun if the SPForce has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun;
		if (FBitSet( pev->weapons, SPFORCE_SHOTGUN ))
		{
			pGun = DropItem( "weapon_barneyshotgun", vecGunPos, vecGunAngles );
		}
//Alex994 begin - for beretta spawning
		else if (FBitSet( pev->weapons, SPFORCE_BERETTA ))
		{
			pGun = DropItem( "weapon_beretta", vecGunPos, vecGunAngles );
		}
//Alex994 end
		else
		{
			pGun = DropItem( "weapon_9mmm41a", vecGunPos, vecGunAngles );
		}
		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	
		if (FBitSet( pev->weapons, SPFORCE_GRENADELAUNCHER ))
		{
			pGun = DropItem( "ammo_ARgrenades", vecGunPos, vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
	}

	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human SPForces because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CSPForce :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CSPForce :: FOkToSpeak( void )
{
// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return FALSE;
		}
	}

	// if player is not in pvs, don't speak
//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
//		return FALSE;
	
	return TRUE;
}

//=========================================================
//=========================================================
void CSPForce :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = SPFORCE_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CSPForce :: PrescheduleThink ( void )
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human SPForces
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds. 
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CSPForce :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CSPForce :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&& 
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for SPForce, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the SPForce can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CSPForce :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients, but don't shoot at them.
			return FALSE;
		}

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the SPForce's grenade
// attack. 
//=========================================================
BOOL CSPForce :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! FBitSet(pev->weapons, (SPFORCE_HANDGRENADE | SPFORCE_GRENADELAUNCHER)))
	{
		return FALSE;
	}
	
	// if the SPForce isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	
	Vector vecTarget;

	if (FBitSet( pev->weapons, SPFORCE_HANDGRENADE))
	{
		// find feet
		if (RANDOM_LONG(0,1))
		{
			// magically know where they are
			vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions( bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my squad members near the intended grenade impact area?
	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}
	
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

		
	if (FBitSet( pev->weapons, SPFORCE_HANDGRENADE))
	{
		Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.hgruntGrenadeSpeed, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}

	

	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CSPForce :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// make sure we're wearing one
		if (GetBodygroup( 1 ) == HEAD_SPForce && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		{
			// absorb damage
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the SPForce because the SPForce
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CSPForce :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSPForce :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:	
		ys = 150;		
		break;
	case ACT_RUN:	
		ys = 150;	
		break;
	case ACT_WALK:	
		ys = 180;		
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_RANGE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:	
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void CSPForce :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fSPForceQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fSPForceQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CHECK", SPFORCE_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fSPForceQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "HG_QUEST", SPFORCE_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fSPForceQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "HG_IDLE", SPFORCE_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fSPForceQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CLEAR", SPFORCE_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "HG_ANSWER", SPFORCE_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fSPForceQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the SPForce because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CSPForce :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSPForce :: Classify ( void )
{
	if(FClassnameIs(pev,"monster_human_spforce"))
		return CLASS_HUMAN_MILITARY;
	else 
		return CLASS_ALIEN_MILITARY;
}

//=========================================================
//=========================================================
CBaseEntity *CSPForce :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CSPForce :: GetGunPosition( )
{
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 48 );
	}
}

//=========================================================
// Shoot
//=========================================================
void CSPForce :: Shoot ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 
	if (FBitSet( pev->weapons, SPFORCE_BERETTA ))
	{
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BERETTA ); // shoot +-5 degrees
	}
	else
	{
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_M41A ); // shoot +-5 degrees		
	}

	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shoot
//=========================================================
void CSPForce :: Shotgun ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL); 
	FireBullets(gSkillData.hgruntShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 ); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSPForce :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case SPFORCE_AE_DROP_GUN:
			{
			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			SetBodygroup( GUN_GROUP, GUN_NONE );

			// now spawn a gun.
			if (FBitSet( pev->weapons, SPFORCE_SHOTGUN ))
			{
				 DropItem( "weapon_barneyshotgun", vecGunPos, vecGunAngles );
			}
			//Alex begin
			else if (FBitSet( pev->weapons, SPFORCE_BERETTA ))
			{
				 DropItem( "weapon_beretta", vecGunPos, vecGunAngles );
			}
			//Alex end
			else
			{
				 DropItem( "weapon_9mmm41a", vecGunPos, vecGunAngles );
			}
			if (FBitSet( pev->weapons, SPFORCE_GRENADELAUNCHER ))
			{
				DropItem( "ammo_ARgrenades", BodyTarget( pev->origin ), vecGunAngles );
			}

			}
			break;

		case SPFORCE_AE_RELOAD:
			//EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/spf_reload1.wav", 1, ATTN_NORM );
			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
				if ( RANDOM_LONG(0,1) )
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "spforce/spf_reload1.wav", 1, ATTN_NORM );
				}
				else
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "spforce/spf_reload2.wav", 1, ATTN_NORM );
				}
				
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case SPFORCE_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
			CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;

		case SPFORCE_AE_GREN_LAUNCH:
		{
			//EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/m41aglauncher.wav", 0.8, ATTN_NORM);
				if ( RANDOM_LONG(0,1) )
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/m41aglauncher.wav", 0.8, ATTN_NORM );
				}
				else
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/m41aglauncher2.wav", 0.8, ATTN_NORM );
				}
			CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
			m_fThrowGrenade = FALSE;
			if (g_iSkillLevel == SKILL_HARD)
				m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );// wait a random amount of time before shooting again
			else
				m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		}
		break;

		case SPFORCE_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
		}
		break;

		case SPFORCE_AE_BURST1:
		{
			if ( FBitSet( pev->weapons, SPFORCE_M41A ))
			{
				Shoot();

				// the first round of the three round burst plays the sound and puts a sound in the world sound list.
				if ( RANDOM_LONG(0,1) )
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/m41ahks1.wav", 1, ATTN_NORM );
				}
				else
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/m41ahks2.wav", 1, ATTN_NORM );
				}
			}
			//Alex begin
			else if ( FBitSet( pev->weapons, SPFORCE_BERETTA ))
			{
				Shoot();
				// the first round of the three round burst plays the sound and puts a sound in the world sound list.
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/beretta_fire1.wav", 1, ATTN_NORM );
			
			}
			//Alex end
			else
			{
				Shotgun( );

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM );
			}
		
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case SPFORCE_AE_BURST2:
		case SPFORCE_AE_BURST3:
			Shoot();
			break;

		case SPFORCE_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				// SOUND HERE!
				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );
			}
		}
		break;

		case SPFORCE_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "HG_ALERT", SPFORCE_SENTENCE_VOLUME, SPFORCE_ATTN, 0, m_voicePitch);
				 JustSpoke();
			}

		}

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CSPForce :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/spforce.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.hgruntHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= SPFORCE_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the SPForce spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = SPFORCE_M41A | SPFORCE_HANDGRENADE;
		// pev->weapons = SPFORCE_SHOTGUN;
		// pev->weapons = SPFORCE_9MMAR | SPFORCE_GRENADELAUNCHER;
	}

	if (FBitSet( pev->weapons, SPFORCE_SHOTGUN ))
	{
		SetBodygroup( GUN_GROUP, GUN_SHOTGUN );
		m_cClipSize		= 8;
	}
	else
	{
		m_cClipSize		= SPFORCE_CLIP_SIZE;
	}
	m_cAmmoLoaded		= m_cClipSize;

	if (RANDOM_LONG( 0, 99 ) < 80)
		pev->skin = 0;	// light skin
	else
		pev->skin = 1;	// dark skin

	if (FBitSet( pev->weapons, SPFORCE_SHOTGUN ))
	{
		SetBodygroup( HEAD_GROUP, HEAD_SHOTGUN);
	}
	else if (FBitSet( pev->weapons, SPFORCE_GRENADELAUNCHER ))
	{
		SetBodygroup( HEAD_GROUP, HEAD_M203 );
		pev->skin = 1; // alway dark skin
	}

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSPForce :: Precache()
{
	PRECACHE_MODEL("models/spforce.mdl");
	
	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );

	PRECACHE_SOUND( "hgrunt/gr_pain1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain4.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain5.wav" );

	PRECACHE_SOUND( "spforce/spf_reload1.wav" );
	PRECACHE_SOUND( "spforce/spf_reload2.wav" );
	
	PRECACHE_SOUND( "weapons/m41aglauncher.wav" );
	PRECACHE_SOUND( "weapons/m41aglauncher2.wav" );

	PRECACHE_SOUND( "weapons/m41ahks1.wav" );
	PRECACHE_SOUND( "weapons/m41ahks2.wav" );

	PRECACHE_SOUND( "weapons/sbarrel1.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");
}	

//=========================================================
// start task
//=========================================================
void CSPForce :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_SPFORCE_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_SPFORCE_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_SPFORCE_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// SPForce no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_SPFORCE_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default: 
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CSPForce :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SPFORCE_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// PainSound
//=========================================================
void CSPForce :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
#if 0
		if ( RANDOM_LONG(0,99) < 5 )
		{
			// pain sentences are rare
			if (FOkToSpeak())
			{
				SENTENCEG_PlayRndSz(ENT(pev), "HG_PAIN", SPFORCE_SENTENCE_VOLUME, ATTN_NORM, 0, PITCH_NORM);
				JustSpoke();
				return;
			}
		}
#endif 
		switch ( RANDOM_LONG(0,6) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain3.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain4.wav", 1, ATTN_NORM );	
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain5.wav", 1, ATTN_NORM );	
			break;
		case 3:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain1.wav", 1, ATTN_NORM );	
			break;
		case 4:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain2.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CSPForce :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE );	
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// SPForceFail
//=========================================================
Task_t	tlSPForceFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slSPForceFail[] =
{
	{
		tlSPForceFail,
		ARRAYSIZE ( tlSPForceFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"SPForce Fail"
	},
};

//=========================================================
// SPForce Combat Fail
//=========================================================
Task_t	tlSPForceCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slSPForceCombatFail[] =
{
	{
		tlSPForceCombatFail,
		ARRAYSIZE ( tlSPForceCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"SPForce Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlSPForceVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slSPForceVictoryDance[] =
{
	{ 
		tlSPForceVictoryDance,
		ARRAYSIZE ( tlSPForceVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"SPForceVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the SPForce to attack.
//=========================================================
Task_t tlSPForceEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_SPFORCE_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_SPFORCE_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slSPForceEstablishLineOfFire[] =
{
	{ 
		tlSPForceEstablishLineOfFire,
		ARRAYSIZE ( tlSPForceEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"SPForceEstablishLineOfFire"
	},
};

//=========================================================
// SPForceFoundEnemy - SPForce established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlSPForceFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slSPForceFoundEnemy[] =
{
	{ 
		tlSPForceFoundEnemy,
		ARRAYSIZE ( tlSPForceFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"SPForceFoundEnemy"
	},
};

//=========================================================
// SPForceCombatFace Schedule
//=========================================================
Task_t	tlSPForceCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SPFORCE_SWEEP	},
};

Schedule_t	slSPForceCombatFace[] =
{
	{ 
		tlSPForceCombatFace1,
		ARRAYSIZE ( tlSPForceCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or SPForce gets hurt.
//=========================================================
Task_t	tlSPForceSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_SPFORCE_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_SPFORCE_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_SPFORCE_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_SPFORCE_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_SPFORCE_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slSPForceSignalSuppress[] =
{
	{ 
		tlSPForceSignalSuppress,
		ARRAYSIZE ( tlSPForceSignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_SPFORCE_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlSPForceSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slSPForceSuppress[] =
{
	{ 
		tlSPForceSuppress,
		ARRAYSIZE ( tlSPForceSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_SPFORCE_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// SPForce wait in cover - we don't allow danger or the ability
// to attack to break a SPForce's run to cover schedule, but
// when a SPForce is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlSPForceWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slSPForceWaitInCover[] =
{
	{ 
		tlSPForceWaitInCover,
		ARRAYSIZE ( tlSPForceWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"SPForceWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlSPForceTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_SPFORCE_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_SPFORCE_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SPFORCE_WAIT_FACE_ENEMY	},
};

Schedule_t	slSPForceTakeCover[] =
{
	{ 
		tlSPForceTakeCover1,
		ARRAYSIZE ( tlSPForceTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlSPForceGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_SPFORCE_WAIT_FACE_ENEMY	},
};

Schedule_t	slSPForceGrenadeCover[] =
{
	{ 
		tlSPForceGrenadeCover1,
		ARRAYSIZE ( tlSPForceGrenadeCover1 ), 
		0,
		0,
		"GrenadeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlSPForceTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slSPForceTossGrenadeCover[] =
{
	{ 
		tlSPForceTossGrenadeCover1,
		ARRAYSIZE ( tlSPForceTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlSPForceTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slSPForceTakeCoverFromBestSound[] =
{
	{ 
		tlSPForceTakeCoverFromBestSound,
		ARRAYSIZE ( tlSPForceTakeCoverFromBestSound ), 
		0,
		0,
		"SPForceTakeCoverFromBestSound"
	},
};

//=========================================================
// SPForce reload schedule
//=========================================================
Task_t	tlSPForceHideReload[] =
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

Schedule_t slSPForceHideReload[] = 
{
	{
		tlSPForceHideReload,
		ARRAYSIZE ( tlSPForceHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"SPForceHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlSPForceSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slSPForceSweep[] =
{
	{ 
		tlSPForceSweep,
		ARRAYSIZE ( tlSPForceSweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"SPForce Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// SPForce's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlSPForceRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH },
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slSPForceRangeAttack1A[] =
{
	{ 
		tlSPForceRangeAttack1A,
		ARRAYSIZE ( tlSPForceRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_SPFORCE_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// SPForce's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlSPForceRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SPFORCE_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slSPForceRangeAttack1B[] =
{
	{ 
		tlSPForceRangeAttack1B,
		ARRAYSIZE ( tlSPForceRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_SPFORCE_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// SPForce's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlSPForceRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SPFORCE_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SPFORCE_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slSPForceRangeAttack2[] =
{
	{ 
		tlSPForceRangeAttack2,
		ARRAYSIZE ( tlSPForceRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlSPForceRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slSPForceRepel[] =
{
	{ 
		tlSPForceRepel,
		ARRAYSIZE ( tlSPForceRepel ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlSPForceRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slSPForceRepelAttack[] =
{
	{ 
		tlSPForceRepelAttack,
		ARRAYSIZE ( tlSPForceRepelAttack ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlSPForceRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slSPForceRepelLand[] =
{
	{ 
		tlSPForceRepelLand,
		ARRAYSIZE ( tlSPForceRepelLand ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel Land"
	},
};


DEFINE_CUSTOM_SCHEDULES( CSPForce )
{
	slSPForceFail,
	slSPForceCombatFail,
	slSPForceVictoryDance,
	slSPForceEstablishLineOfFire,
	slSPForceFoundEnemy,
	slSPForceCombatFace,
	slSPForceSignalSuppress,
	slSPForceSuppress,
	slSPForceWaitInCover,
	slSPForceTakeCover,
	slSPForceGrenadeCover,
	slSPForceTossGrenadeCover,
	slSPForceTakeCoverFromBestSound,
	slSPForceHideReload,
	slSPForceSweep,
	slSPForceRangeAttack1A,
	slSPForceRangeAttack1B,
	slSPForceRangeAttack2,
	slSPForceRepel,
	slSPForceRepelAttack,
	slSPForceRepelLand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSPForce, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CSPForce :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// SPForce is either shooting standing or shooting crouched
		if (FBitSet( pev->weapons, SPFORCE_M41A))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_mp5" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_mp5" );
			}
		}
		//Alex begin
		else if (FBitSet( pev->weapons, SPFORCE_BERETTA))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_beretta" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_beretta" );
			}
		}
		//Alex end
		else
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_shotgun" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_shotgun" );
			}
		}
		break;
	case ACT_RANGE_ATTACK2:
		// SPForce is going to a secondary long range attack. This may be a thrown 
		// grenade or fired grenade, we must determine which and pick proper sequence
		if ( pev->weapons & SPFORCE_HANDGRENADE )
		{
			// get toss anim
			iSequence = LookupSequence( "throwgrenade" );
		}
		else
		{
			// get launch anim
			iSequence = LookupSequence( "launchgrenade" );
		}
		break;
	case ACT_RUN:
		if ( pev->health <= SPFORCE_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= SPFORCE_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CSPForce :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = SPFORCE_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_SPFORCE_REPEL_LAND );
		}
		else
		{
			// repel down a rope, 
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_SPFORCE_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_SPFORCE_REPEL );
		}
	}

	// SPForces place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				
				//!!!KELLY - currently, this is the SPForce's signal that a grenade has landed nearby,
				// and the SPForce should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "HG_GREN", SPFORCE_SENTENCE_VOLUME, SPFORCE_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
					{
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}
					else 
					{
						//!!!KELLY - the leader of a squad of SPForces has just seen the player or a 
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight" 
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
								// player
								SENTENCEG_PlayRndSz( ENT(pev), "HG_ALERT", SPFORCE_SENTENCE_VOLUME, SPFORCE_ATTN, 0, m_voicePitch);
							else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) && 
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) && 
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "HG_MONST", SPFORCE_SENTENCE_VOLUME, SPFORCE_ATTN, 0, m_voicePitch);

							JustSpoke();
						}
						
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_SPFORCE_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_SPFORCE_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return GetScheduleOfType ( SCHED_SPFORCE_COVER_AND_RELOAD );
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != NULL )
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this SPForce was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", SPFORCE_SENTENCE_VOLUME, SPFORCE_ATTN, 0, m_voicePitch);
						m_iSentence = SPFORCE_SENT_COVER;
						//JustSpoke();
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can grenade launch

			else if ( FBitSet( pev->weapons, SPFORCE_GRENADELAUNCHER) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a 
					// little time and give the player a chance to turn.
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_SPFORCE_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					// throw a grenade if can and no engage slots are available
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else
				{
					// hide!
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					//!!!KELLY - this SPForce is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "HG_THROW", SPFORCE_SENTENCE_VOLUME, SPFORCE_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					//!!!KELLY - SPForce cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", SPFORCE_SENTENCE_VOLUME, SPFORCE_ATTN, 0, m_voicePitch);
						m_iSentence = SPFORCE_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_SPFORCE_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - SPForce is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// SPForce's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "HG_TAUNT", SPFORCE_SENTENCE_VOLUME, SPFORCE_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_SPFORCE_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CSPForce :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "HG_THROW", SPFORCE_SENTENCE_VOLUME, SPFORCE_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return slSPForceTossGrenadeCover;
				}
				else
				{
					return &slSPForceTakeCover[ 0 ];
				}
			}
			else
			{
				if ( RANDOM_LONG(0,1) )
				{
					return &slSPForceTakeCover[ 0 ];
				}
				else
				{
					return &slSPForceGrenadeCover[ 0 ];
				}
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slSPForceTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_SPFORCE_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_SPFORCE_ELOF_FAIL:
		{
			// human SPForce is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_SPFORCE_ESTABLISH_LINE_OF_FIRE:
		{
			return &slSPForceEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// randomly stand or crouch
			if (RANDOM_LONG(0,9) == 0)
				m_fStanding = RANDOM_LONG(0,1);
		 
			if (m_fStanding)
				return &slSPForceRangeAttack1B[ 0 ];
			else
				return &slSPForceRangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slSPForceRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slSPForceCombatFace[ 0 ];
		}
	case SCHED_SPFORCE_WAIT_FACE_ENEMY:
		{
			return &slSPForceWaitInCover[ 0 ];
		}
	case SCHED_SPFORCE_SWEEP:
		{
			return &slSPForceSweep[ 0 ];
		}
	case SCHED_SPFORCE_COVER_AND_RELOAD:
		{
			return &slSPForceHideReload[ 0 ];
		}
	case SCHED_SPFORCE_FOUND_ENEMY:
		{
			return &slSPForceFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slSPForceFail[ 0 ];
				}
			}

			return &slSPForceVictoryDance[ 0 ];
		}
	case SCHED_SPFORCE_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slSPForceSignalSuppress[ 0 ];
			}
			else
			{
				return &slSPForceSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// SPForce has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slSPForceCombatFail[ 0 ];
			}

			return &slSPForceFail[ 0 ];
		}
	case SCHED_SPFORCE_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slSPForceRepel[ 0 ];
		}
	case SCHED_SPFORCE_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slSPForceRepelAttack[ 0 ];
		}
	case SCHED_SPFORCE_REPEL_LAND:
		{
			return &slSPForceRepelLand[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}


//=========================================================
// CSPForceRepel - when triggered, spawns a monster_human_spforce
// repelling down a line.
//=========================================================

class CSPForceRepel : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;	// Don't save, precache
};

LINK_ENTITY_TO_CLASS( monster_spforce_repel, CSPForceRepel );

void CSPForceRepel::Spawn( void )
{
	Precache( );
	pev->solid = SOLID_NOT;

	SetUse( &CSPForceRepel::RepelUse );
}

void CSPForceRepel::Precache( void )
{
	UTIL_PrecacheOther( "monster_human_spforce" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CSPForceRepel::RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP) 
		return NULL;
	*/

	CBaseEntity *pEntity = Create( "monster_human_spforce", pev->origin, pev->angles );
	CBaseMonster *pSPForce = pEntity->MyMonsterPointer( );
	pSPForce->pev->movetype = MOVETYPE_FLY;
	pSPForce->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	pSPForce->SetActivity( ACT_GLIDE );
	// UNDONE: position?
	pSPForce->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector(0,0,112), pSPForce->entindex() );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink( &CBeam::SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pSPForce->pev->velocity.z + 0.5;

	UTIL_Remove( this );
}



//=========================================================
// DEAD SPFORCE PROP
//=========================================================
class CDeadSPForce : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadSPForce::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadSPForce::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_spforce_dead, CDeadSPForce );

//=========================================================
// ********** DeadSPForce SPAWN **********
//=========================================================
void CDeadSPForce :: Spawn( void )
{
	PRECACHE_MODEL("models/spforce.mdl");
	SET_MODEL(ENT(pev), "models/spforce.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead SPForce with bad pose\n" );
	}

	// Corpses have less health
	pev->health			= 8;

	// map old bodies onto new bodies
	switch( pev->body )
	{
	case 0: // SPForce with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_SPForce );
		SetBodygroup( GUN_GROUP, GUN_M41A );
		break;
	case 1: // Commander with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_COMMANDER );
		SetBodygroup( GUN_GROUP, GUN_M41A );
		break;
	case 2: // SPForce no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_SPForce );
		SetBodygroup( GUN_GROUP, GUN_NONE );
		break;
	case 3: // Commander no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_COMMANDER );
		SetBodygroup( GUN_GROUP, GUN_NONE );
		break;
	}

	MonsterInitDead();
}
