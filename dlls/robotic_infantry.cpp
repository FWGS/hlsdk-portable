//=========================================================
// Robotic Infantry by XF-Alien
//=========================================================

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
#include	"scripted.h" //LRC
#include	"player.h"
#include	"ach_counters.h"

int g_frinfantryQuestion;				// true if an idle rinfantry asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	R_INFANTRY_CLIP_SIZE					36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define R_INFANTRY_VOL						0.35		// volume of rinfantry sounds
#define R_INFANTRY_ATTN						ATTN_NORM	// attenutation of rinfantry sentences
#define R_INFANTRY_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a rinfantry with a single headshot.
#define R_INFANTRY_MINIMUM_HEADSHOT_DAMAGE	5 // must do at least this much damage in one shot to head to score a headshot kill
#define	R_INFANTRY_SENTENCE_VOLUME			(float)0.35 // volume of rinfantry sentences

#define R_INFANTRY_M4A1						( 1 << 0)
#define R_INFANTRY_HANDGRENADE				( 1 << 1)
#define R_INFANTRY_GRENADELAUNCHER			( 1 << 2)
#define R_INFANTRY_SHOTGUN					( 1 << 3)
#define R_INFANTRY_SMG						( 1 << 4)

#define GUN_GROUP					1
#define GUN_M4A1					0
#define GUN_SHOTGUN					1
#define GUN_SMG						2
#define GUN_NONE					3

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		R_INFANTRY_AE_RELOAD		( 2 )
#define		R_INFANTRY_AE_KICK			( 3 )
#define		R_INFANTRY_AE_BURST1		( 4 )
#define		R_INFANTRY_AE_BURST2		( 5 ) 
#define		R_INFANTRY_AE_BURST3		( 6 ) 
#define		R_INFANTRY_AE_GREN_TOSS		( 7 )
#define		R_INFANTRY_AE_GREN_LAUNCH	( 8 )
#define		R_INFANTRY_AE_GREN_DROP		( 9 )
#define		R_INFANTRY_AE_CAUGHT_ENEMY	( 10) // rinfantry established sight with an enemy (player only) that had previously eluded the squad.
#define		R_INFANTRY_AE_DROP_GUN		( 11) // rinfantry (probably dead) is dropping his mp5.

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_R_INFANTRY_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_R_INFANTRY_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_R_INFANTRY_COVER_AND_RELOAD,
	SCHED_R_INFANTRY_SWEEP,
	SCHED_R_INFANTRY_FOUND_ENEMY,
	SCHED_R_INFANTRY_WAIT_FACE_ENEMY,
	SCHED_R_INFANTRY_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_R_INFANTRY_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_R_INFANTRY_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_R_INFANTRY_SPEAK_SENTENCE,
	TASK_R_INFANTRY_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_R_INFANTRY_NOFIRE	( bits_COND_SPECIAL1 )

class Crinfantry : public CSquadMonster
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
	BOOL IsMachine() { return true; }
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void WorkSound( void );
	void DeathSound( void );
	void PainSound( void );
	Vector GetGunPosition( void );
	void Shoot ( void );
	void Shotgun ( void );
	void SMG ( void );
	void PrescheduleThink ( void );
	void GibMonster( void );
	void SpeakSentence( void );
	void Killed( entvars_t *pevAttacker, int iGib );

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
	float m_flWorkTime;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	BOOL	m_fWorkSound;
	int		m_cClipSize;

	int m_voicePitch;

	int		m_iBrassShell;
	int		m_iShotgunShell;
	int		m_iSMGShell;

	int		m_iSentence;

	static const char *prinfantrySentences[];
};

LINK_ENTITY_TO_CLASS( monster_robotic_infantry, Crinfantry );

TYPEDESCRIPTION	Crinfantry::m_SaveData[] = 
{
	DEFINE_FIELD( Crinfantry, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( Crinfantry, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( Crinfantry, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( Crinfantry, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( Crinfantry, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( Crinfantry, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( Crinfantry, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( Crinfantry, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( Crinfantry, m_voicePitch, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( Crinfantry, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( Crinfantry, CSquadMonster );

const char *Crinfantry::prinfantrySentences[] = 
{
	"HG_GREN", // grenade scared rinfantry
	"HG_ALERT", // sees player
	"HG_MONSTER", // sees monster
	"HG_COVER", // running to cover
	"HG_THROW", // about to throw grenade
	"HG_CHARGE",  // running out to get the enemy
	"HG_TAUNT", // say rude things
};

enum
{
	R_INFANTRY_SENT_NONE = -1,
	R_INFANTRY_SENT_GREN = 0,
	R_INFANTRY_SENT_ALERT,
	R_INFANTRY_SENT_MONSTER,
	R_INFANTRY_SENT_COVER,
	R_INFANTRY_SENT_THROW,
	R_INFANTRY_SENT_CHARGE,
	R_INFANTRY_SENT_TAUNT,
} R_INFANTRY_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some rinfantry sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a rinfantry says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the rinfantry has 
// started moving.
//=========================================================
void Crinfantry :: SpeakSentence( void )
{
	if ( m_iSentence == R_INFANTRY_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), prinfantrySentences[ m_iSentence ], R_INFANTRY_SENTENCE_VOLUME, R_INFANTRY_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// IRelationship - overridden because Alien rinfantrys are 
// Human rinfantry's nemesis.
//=========================================================
int Crinfantry::IRelationship ( CBaseEntity *pTarget )
{
	//LRC- only hate alien rinfantrys if my behaviour hasn't been overridden
	if (!m_iClass && FClassnameIs( pTarget->pev, "monster_alien_rinfantry" ) || ( FClassnameIs( pTarget->pev,  "monster_gargantua" ) ) )
	{
		return R_NM;
	}

	return CSquadMonster::IRelationship( pTarget );
}

//=========================================================
// GibMonster
//=========================================================
void Crinfantry :: GibMonster ( void )
{
}

//=========================================================
// ISoundMask - Overidden for human rinfantrys because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int Crinfantry :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL Crinfantry :: FOkToSpeak( void )
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
void Crinfantry :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = R_INFANTRY_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void Crinfantry :: PrescheduleThink ( void )
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
// FCanCheckAttacks - this is overridden for human rinfantrys
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
BOOL Crinfantry :: FCanCheckAttacks ( void )
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
BOOL Crinfantry :: CheckMeleeAttack1 ( float flDot, float flDist )
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
// CheckRangeAttack1 - overridden for rinfantry, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the rinfantry can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL Crinfantry :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients who are close enough, but don't shoot at them.
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
// CheckRangeAttack2 - this checks the rinfantry's grenade
// attack. 
//=========================================================
BOOL Crinfantry :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! FBitSet(pev->weapons, (R_INFANTRY_HANDGRENADE | R_INFANTRY_GRENADELAUNCHER)))
	{
		return FALSE;
	}
	
	// if the rinfantry isn't moving, it's ok to check.
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

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && (m_hEnemy->pev->waterlevel == 0 || m_hEnemy->pev->watertype==CONTENT_FOG) && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	
	Vector vecTarget;

	if (FBitSet( pev->weapons, R_INFANTRY_HANDGRENADE))
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

		
	if (FBitSet( pev->weapons, R_INFANTRY_HANDGRENADE))
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

void Crinfantry::Killed( entvars_t *pevAttacker, int iGib )
{
	pev->movetype = MOVETYPE_TOSS;
	STOP_SOUND( ENT(pev), CHAN_STATIC, "robotic_infantry/ri_work.wav" );
	CSquadMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// TraceAttack - make sure we're not taking it in the armor
//=========================================================
void Crinfantry :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if  (ptr->iHitgroup != 1 && bitsDamageType == DMG_CLUB) // Can't deal damage with crowbar!
		{
			flDamage = 0.01;
			UTIL_Ricochet( ptr->vecEndPos, 1.0 );
			return;
		}

	if (ptr->iHitgroup == 11)
	{
		if (flDamage >= 10)
		{
			UTIL_Sparks( ptr->vecEndPos );
			flDamage = flDamage / 1.5;	// absorb damage
		
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.x, pev->absmax.x ) );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.y, pev->absmax.y ) );
			WRITE_COORD( pev->origin.z + 32);
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 5 ); 
			WRITE_BYTE( 10 ); 
		MESSAGE_END(); 
		}
		else
		flDamage = flDamage / 2.25;
	
		UTIL_Ricochet( ptr->vecEndPos, 1.0 );
	}
	else
	{
		ptr->iHitgroup = HITGROUP_HEAD;
		UTIL_Sparks( ptr->vecEndPos );
		
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.x, pev->absmax.x ) );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.y, pev->absmax.y ) );
			WRITE_COORD( pev->origin.z + 32);
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 5 );
			WRITE_BYTE( 10 ); 
		MESSAGE_END();
	}

	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the rinfantry because the rinfantry
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int Crinfantry :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );
	const bool wasAlive = !HasMemory(bits_MEMORY_KILLED);
	const int result = CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	const bool isDeadNow = HasMemory(bits_MEMORY_KILLED);
	if (wasAlive && isDeadNow && (bitsDamageType & DMG_CLUB) != 0)
	{
		CBasePlayer* pPlayer = CBasePlayer::PlayerInstance(pevAttacker);
		if (pPlayer)
		{
			pPlayer->m_robotsKilledByMelee++;
			if (pPlayer->m_robotsKilledByMelee == ACH_MIGHT_MAKES_RIGHT_COUNT)
			{
				pPlayer->SetAchievement("ACH_MIGHT_MAKES_RIGHT");
			}
		}
	}
	return result;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void Crinfantry :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:	
		ys = 180;		
		break;
	case ACT_RUN:	
		ys = 180;	
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
		ys = 130;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// CheckAmmo - overridden for the rinfantry because he actually
// uses ammo! (base class doesn't)
//=========================================================
void Crinfantry :: CheckAmmo ( void )
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
int	Crinfantry :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_HUMAN_MILITARY;
}

//=========================================================
//=========================================================
CBaseEntity *Crinfantry :: Kick( void )
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

Vector Crinfantry :: GetGunPosition( )
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
void Crinfantry :: Shoot ( void )
{
	if (m_hEnemy == NULL && m_pCine == NULL) //LRC - scripts may fire when you have no enemy
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if (m_cAmmoLoaded > 0)
	{
		UTIL_MakeVectors ( pev->angles );

		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5 ); // shoot +-5 degrees

		pev->effects |= EF_MUZZLEFLASH;
	
		m_cAmmoLoaded--;// take away a bullet!
	}

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shotgun
//=========================================================
void Crinfantry :: Shotgun ( void )
{
	if (m_hEnemy == NULL && m_pCine == NULL)
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
// SMG
//=========================================================
void Crinfantry :: SMG ( void )
{
	if (m_hEnemy == NULL && m_pCine == NULL) //LRC - scripts may fire when you have no enemy
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if (m_cAmmoLoaded > 0)
	{
		UTIL_MakeVectors ( pev->angles );

		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iSMGShell, TE_BOUNCE_SHELL); 
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_45ACP ); // shoot +-5 degrees

		pev->effects |= EF_MUZZLEFLASH;
	
		m_cAmmoLoaded--;// take away a bullet!
	}

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void Crinfantry :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case R_INFANTRY_AE_DROP_GUN:
			{
			if (pev->spawnflags & SF_MONSTER_NO_WPN_DROP) break; //LRC

			if ( GetBodygroup( GUN_GROUP ) != GUN_NONE )
			{
				Vector	vecGunPos;
				Vector	vecGunAngles;

				GetAttachment( 0, vecGunPos, vecGunAngles );

				// switch to body group with no gun.
				SetBodygroup( GUN_GROUP, GUN_NONE );
				DropItem( "item_battery", BodyTarget( pev->origin ), vecGunAngles );

				// now spawn a gun.
				if (FBitSet( pev->weapons, R_INFANTRY_SHOTGUN ))
				{
					DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
				}
				else if (FBitSet( pev->weapons, R_INFANTRY_SMG ))
				{
					DropItem( "weapon_smg", vecGunPos, vecGunAngles );
				}
				else
				{
					DropItem( "weapon_m4a1", vecGunPos, vecGunAngles );
				}
				if (FBitSet( pev->weapons, R_INFANTRY_GRENADELAUNCHER ))
				{
					DropItem( "ammo_ARgrenades", BodyTarget( pev->origin ), vecGunAngles );
				}
			}
			}
			break;

		case R_INFANTRY_AE_RELOAD:
			if (FBitSet( pev->weapons, R_INFANTRY_M4A1 | R_INFANTRY_SMG ))
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
			else
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload2.wav", 1, ATTN_NORM );

			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case R_INFANTRY_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
			//LRC - a bit of a hack. Ideally the rinfantrys would work out in advance whether it's ok to throw.
			if (m_pCine)
			{
				Vector vecToss = g_vecZero;
				if (m_hTargetEnt != NULL && m_pCine->PreciseAttack())
				{
					vecToss = VecCheckToss( pev, GetGunPosition(), m_hTargetEnt->pev->origin, 0.5 );
				}
				if (vecToss == g_vecZero)
				{
					vecToss = (gpGlobals->v_forward*0.5+gpGlobals->v_up*0.5).Normalize()*gSkillData.hgruntGrenadeSpeed;
				}
				CGrenade::ShootTimed( pev, GetGunPosition(), vecToss, 3.5 );
			}
			else
				CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;

		case R_INFANTRY_AE_GREN_LAUNCH:
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
			//LRC: firing due to a script?
			if (m_pCine)
			{
				Vector vecToss;
				if (m_hTargetEnt != NULL && m_pCine->PreciseAttack())
					vecToss = VecCheckThrow( pev, GetGunPosition(), m_hTargetEnt->pev->origin, gSkillData.hgruntGrenadeSpeed, 0.5 );
				else
				{
					// just shoot diagonally up+forwards
					UTIL_MakeVectors(pev->angles);
					vecToss = (gpGlobals->v_forward*0.5 + gpGlobals->v_up*0.5).Normalize() * gSkillData.hgruntGrenadeSpeed;
				}
				CGrenade::ShootContact( pev, GetGunPosition(), vecToss );
			}
			else
			CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
			m_fThrowGrenade = FALSE;
			if (g_iSkillLevel == SKILL_HARD)
				m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );// wait a random amount of time before shooting again
			else
				m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		}
		break;

		case R_INFANTRY_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
		}
		break;

		case R_INFANTRY_AE_BURST1:
		{
			if ( FBitSet( pev->weapons, R_INFANTRY_M4A1 ))
			{
				// the first round of the three round burst plays the sound and puts a sound in the world sound list.
				if (m_cAmmoLoaded > 0)
				{
					if ( RANDOM_LONG(0,1) )
					{
						EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM );
					}
					else
					{
						EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM );
					}
				}
				else
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/dryfire1.wav", 1, ATTN_NORM );
				}

				Shoot();
			}
			else if ( FBitSet( pev->weapons, R_INFANTRY_SMG ))
			{
				SMG( );

				if ( RANDOM_LONG(0,1) )
					{
						EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_smgburst1.wav", 1, ATTN_NORM );
					}
					else
					{
						EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_smgburst2.wav", 1, ATTN_NORM );
					}
			}
			else
			{
				Shotgun( );

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM );
			}
		
		
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case R_INFANTRY_AE_BURST2:
		case R_INFANTRY_AE_BURST3:
			if ( FBitSet( pev->weapons, R_INFANTRY_M4A1 ))
			Shoot();
			else if ( FBitSet( pev->weapons, R_INFANTRY_SHOTGUN ))
			Shotgun();
			else if ( FBitSet( pev->weapons, R_INFANTRY_SMG ))
			SMG();
			break;

		case R_INFANTRY_AE_KICK:
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

		case R_INFANTRY_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "HG_ALERT", R_INFANTRY_SENTENCE_VOLUME, R_INFANTRY_ATTN, 0, m_voicePitch);
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
void Crinfantry :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/robotic_infantry.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health		= gSkillData.rinfantryHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= R_INFANTRY_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the rinfantry spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = R_INFANTRY_M4A1;
	}

	if (FBitSet( pev->weapons, R_INFANTRY_M4A1 ))
	{
		SetBodygroup( GUN_GROUP, GUN_M4A1 );
		m_cClipSize		= 30;
	}

	if (FBitSet( pev->weapons, R_INFANTRY_SHOTGUN ))
	{
		SetBodygroup( GUN_GROUP, GUN_SHOTGUN );
		m_cClipSize		= 8;
	}
	else if (FBitSet( pev->weapons, R_INFANTRY_SMG ))
	{
		SetBodygroup( GUN_GROUP, GUN_SMG );
		m_cClipSize		= 60;
	}
	else
	{
		m_cClipSize		= R_INFANTRY_CLIP_SIZE;
	}

	m_cAmmoLoaded		= m_cClipSize;

	CTalkMonster::g_talkWaitTime = 0;
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void Crinfantry :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/robotic_infantry.mdl");

	PRECACHE_SOUND( "weapons/dryfire1.wav" ); //LRC

	PRECACHE_SOUND( "hgrunt/gr_mgun1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_mgun2.wav" );

	PRECACHE_SOUND( "hgrunt/gr_smgburst1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_smgburst2.wav" );

	PRECACHE_SOUND( "turret/tu_die.wav" );
	PRECACHE_SOUND( "turret/tu_die2.wav" );
	PRECACHE_SOUND( "turret/tu_die3.wav" );

	PRECACHE_SOUND( "robotic_infantry/ri_bodydrop1.wav" );
	PRECACHE_SOUND( "robotic_infantry/ri_bodydrop2.wav" );

	PRECACHE_SOUND( "robotic_infantry/ri_movement1.wav" );
	PRECACHE_SOUND( "robotic_infantry/ri_movement2.wav" );
	PRECACHE_SOUND( "robotic_infantry/ri_movement3.wav" );
	
	PRECACHE_SOUND( "robotic_infantry/ri_work.wav" );

	PRECACHE_SOUND( "player/pl_pain2.wav" );

	PRECACHE_SOUND( "hgrunt/gr_reload1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_reload2.wav" );

	PRECACHE_SOUND( "robotic_infantry/ri_step1.wav" );
	PRECACHE_SOUND( "robotic_infantry/ri_step2.wav" );
	PRECACHE_SOUND( "robotic_infantry/ri_step3.wav" );
	PRECACHE_SOUND( "robotic_infantry/ri_step4.wav" );


	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );
	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	m_iBrassShell = PRECACHE_MODEL ("models/rifleshell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");
	m_iSMGShell = PRECACHE_MODEL ("models/45acp_shell.mdl");
}	

//=========================================================
// start task
//=========================================================
void Crinfantry :: StartTask ( Task_t *pTask )
{
	WorkSound( );

	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_R_INFANTRY_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_R_INFANTRY_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_R_INFANTRY_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// rinfantry no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_R_INFANTRY_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	case TASK_DIE:
		STOP_SOUND( ENT(pev), CHAN_STATIC, "robotic_infantry/ri_work.wav" );


	default: 
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void Crinfantry :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_R_INFANTRY_FACE_TOSS_DIR:
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
// WorkSound
//=========================================================
void Crinfantry::WorkSound( void )
{
	if ((pev->health > 0) && (m_fWorkSound == FALSE))
	{
		m_flWorkTime = gpGlobals->time + 0.95;
		EMIT_SOUND( ENT(pev), CHAN_STATIC,  "robotic_infantry/ri_work.wav", 0.5, ATTN_NORM );
		m_fWorkSound = TRUE;
	}
}

//=========================================================
// PainSound
//=========================================================
void Crinfantry :: PainSound ( void )
{ 
	if ( gpGlobals->time > m_flNextPainTime )
	{
#if 0
		if ( RANDOM_LONG(0,99) < 5 )
		{
			// pain sentences are rare
			if (FOkToSpeak())
			{
				SENTENCEG_PlayRndSz(ENT(pev), "HG_PAIN", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, PITCH_NORM);
				JustSpoke();
				return;
			}
		}
#endif 
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/pl_pain2.wav", 1, ATTN_NORM );	
		m_flNextPainTime = gpGlobals->time + 1;
	} 
}

//=========================================================
// DeathSound 
//=========================================================
void Crinfantry :: DeathSound ( void )
{
	if (pev->deadflag != DEAD_DEAD)
	{
	// lots of smoke
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.x, pev->absmax.x ) );
			WRITE_COORD( RANDOM_FLOAT( pev->absmin.y, pev->absmax.y ) );
			WRITE_COORD( pev->origin.z + 32);
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 25 ); // scale * 10
			WRITE_BYTE( 10 ); // framerate
		MESSAGE_END(); 

		pev->solid = SOLID_NOT;
	}

	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "turret/tu_die.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "turret/tu_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "turret/tu_die3.wav", 1, ATTN_IDLE );	
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// rinfantryFail
//=========================================================
Task_t	tlrinfantryFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slrinfantryFail[] =
{
	{
		tlrinfantryFail,
		ARRAYSIZE ( tlrinfantryFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"rinfantry Fail"
	},
};

//=========================================================
// rinfantry Combat Fail
//=========================================================
Task_t	tlrinfantryCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slrinfantryCombatFail[] =
{
	{
		tlrinfantryCombatFail,
		ARRAYSIZE ( tlrinfantryCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"rinfantry Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlrinfantryVictoryDance[] =
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

Schedule_t	slrinfantryVictoryDance[] =
{
	{ 
		tlrinfantryVictoryDance,
		ARRAYSIZE ( tlrinfantryVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"rinfantryVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the rinfantry to attack.
//=========================================================
Task_t tlrinfantryEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_R_INFANTRY_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_R_INFANTRY_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slrinfantryEstablishLineOfFire[] =
{
	{ 
		tlrinfantryEstablishLineOfFire,
		ARRAYSIZE ( tlrinfantryEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"rinfantryEstablishLineOfFire"
	},
};

//=========================================================
// rinfantryFoundEnemy - rinfantry established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlrinfantryFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slrinfantryFoundEnemy[] =
{
	{ 
		tlrinfantryFoundEnemy,
		ARRAYSIZE ( tlrinfantryFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"rinfantryFoundEnemy"
	},
};

//=========================================================
// rinfantryCombatFace Schedule
//=========================================================
Task_t	tlrinfantryCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_R_INFANTRY_SWEEP	},
};

Schedule_t	slrinfantryCombatFace[] =
{
	{ 
		tlrinfantryCombatFace1,
		ARRAYSIZE ( tlrinfantryCombatFace1 ), 
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
// empty or rinfantry gets hurt.
//=========================================================
Task_t	tlrinfantrySignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_R_INFANTRY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_R_INFANTRY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_R_INFANTRY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_R_INFANTRY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_R_INFANTRY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slrinfantrySignalSuppress[] =
{
	{ 
		tlrinfantrySignalSuppress,
		ARRAYSIZE ( tlrinfantrySignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_R_INFANTRY_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlrinfantrySuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slrinfantrySuppress[] =
{
	{ 
		tlrinfantrySuppress,
		ARRAYSIZE ( tlrinfantrySuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_R_INFANTRY_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// rinfantry wait in cover - we don't allow danger or the ability
// to attack to break a rinfantry's run to cover schedule, but
// when a rinfantry is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlrinfantryWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slrinfantryWaitInCover[] =
{
	{ 
		tlrinfantryWaitInCover,
		ARRAYSIZE ( tlrinfantryWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"rinfantryWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlrinfantryTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_R_INFANTRY_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_SET_SCHEDULE,			(float)SCHED_R_INFANTRY_WAIT_FACE_ENEMY	},
};

Schedule_t	slrinfantryTakeCover[] =
{
	{ 
		tlrinfantryTakeCover1,
		ARRAYSIZE ( tlrinfantryTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlrinfantryGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_R_INFANTRY_WAIT_FACE_ENEMY	},
};

Schedule_t	slrinfantryGrenadeCover[] =
{
	{ 
		tlrinfantryGrenadeCover1,
		ARRAYSIZE ( tlrinfantryGrenadeCover1 ), 
		0,
		0,
		"GrenadeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlrinfantryTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slrinfantryTossGrenadeCover[] =
{
	{ 
		tlrinfantryTossGrenadeCover1,
		ARRAYSIZE ( tlrinfantryTossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlrinfantryTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slrinfantryTakeCoverFromBestSound[] =
{
	{ 
		tlrinfantryTakeCoverFromBestSound,
		ARRAYSIZE ( tlrinfantryTakeCoverFromBestSound ), 
		0,
		0,
		"rinfantryTakeCoverFromBestSound"
	},
};

//=========================================================
// rinfantry reload schedule
//=========================================================
Task_t	tlrinfantryHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slrinfantryHideReload[] = 
{
	{
		tlrinfantryHideReload,
		ARRAYSIZE ( tlrinfantryHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"rinfantryHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlrinfantrySweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slrinfantrySweep[] =
{
	{ 
		tlrinfantrySweep,
		ARRAYSIZE ( tlrinfantrySweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"rinfantry Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// rinfantry's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlrinfantryRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH },
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slrinfantryRangeAttack1A[] =
{
	{ 
		tlrinfantryRangeAttack1A,
		ARRAYSIZE ( tlrinfantryRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_R_INFANTRY_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// rinfantry's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlrinfantryRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_R_INFANTRY_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slrinfantryRangeAttack1B[] =
{
	{ 
		tlrinfantryRangeAttack1B,
		ARRAYSIZE ( tlrinfantryRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_R_INFANTRY_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// rinfantry's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlrinfantryRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_R_INFANTRY_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_R_INFANTRY_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slrinfantryRangeAttack2[] =
{
	{ 
		tlrinfantryRangeAttack2,
		ARRAYSIZE ( tlrinfantryRangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};

DEFINE_CUSTOM_SCHEDULES( Crinfantry )
{
	slrinfantryFail,
	slrinfantryCombatFail,
	slrinfantryVictoryDance,
	slrinfantryEstablishLineOfFire,
	slrinfantryFoundEnemy,
	slrinfantryCombatFace,
	slrinfantrySignalSuppress,
	slrinfantrySuppress,
	slrinfantryWaitInCover,
	slrinfantryTakeCover,
	slrinfantryGrenadeCover,
	slrinfantryTossGrenadeCover,
	slrinfantryTakeCoverFromBestSound,
	slrinfantryHideReload,
	slrinfantrySweep,
	slrinfantryRangeAttack1A,
	slrinfantryRangeAttack1B,
	slrinfantryRangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES( Crinfantry, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void Crinfantry :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// rinfantry is either shooting standing or shooting crouched
		if (FBitSet( pev->weapons, R_INFANTRY_M4A1))
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
		else if (FBitSet( pev->weapons, R_INFANTRY_SMG))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_smg" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_smg" );
			}
		}
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
		// rinfantry is going to a secondary long range attack. This may be a thrown 
		// grenade or fired grenade, we must determine which and pick proper sequence
		if ( pev->weapons & R_INFANTRY_HANDGRENADE )
		{
			// get toss anim
			iSequence = LookupSequence( "throwgrenade" );
		}
		// LRC: added a test to stop a marine without a launcher from firing.
		else if ( pev->weapons & R_INFANTRY_GRENADELAUNCHER )
		{
			// get launch anim
			iSequence = LookupSequence( "launchgrenade" );
		}
		else
		{
			ALERT( at_console, "No grenades available. "); // flow into the error message we get at the end...
		}
		break;
	case ACT_RUN:
			iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_WALK:
			iSequence = LookupActivity ( NewActivity );
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
Schedule_t *Crinfantry :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = R_INFANTRY_SENT_NONE;

	// rinfantrys place HIGH priority on running away from danger sounds.
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
				
				//!!!KELLY - currently, this is the rinfantry's signal that a grenade has landed nearby,
				// and the rinfantry should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "HG_GREN", R_INFANTRY_SENTENCE_VOLUME, R_INFANTRY_ATTN, 0, m_voicePitch);
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
						ALERT(at_aiconsole,"leader spotted player!\n");
						//!!!KELLY - the leader of a squad of rinfantrys has just seen the player or a 
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
								SENTENCEG_PlayRndSz( ENT(pev), "HG_ALERT", R_INFANTRY_SENTENCE_VOLUME, R_INFANTRY_ATTN, 0, m_voicePitch);
							else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) && 
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) && 
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "HG_MONST", R_INFANTRY_SENTENCE_VOLUME, R_INFANTRY_ATTN, 0, m_voicePitch);

							JustSpoke();
						}
						
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_R_INFANTRY_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_R_INFANTRY_ESTABLISH_LINE_OF_FIRE );
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
				return GetScheduleOfType ( SCHED_R_INFANTRY_COVER_AND_RELOAD );
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

					//!!!KELLY - this rinfantry was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", R_INFANTRY_SENTENCE_VOLUME, R_INFANTRY_ATTN, 0, m_voicePitch);
						m_iSentence = R_INFANTRY_SENT_COVER;
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

			else if ( FBitSet( pev->weapons, R_INFANTRY_GRENADELAUNCHER) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_R_INFANTRY_GRENADE ) )
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
						return GetScheduleOfType ( SCHED_R_INFANTRY_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_R_INFANTRY_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_R_INFANTRY_GRENADE ) )
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
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_R_INFANTRY_GRENADE ) )
				{
					//!!!KELLY - this rinfantry is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "HG_THROW", R_INFANTRY_SENTENCE_VOLUME, R_INFANTRY_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_R_INFANTRY_ENGAGE ) )
				{
					//!!!KELLY - rinfantry cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", R_INFANTRY_SENTENCE_VOLUME, R_INFANTRY_ATTN, 0, m_voicePitch);
						m_iSentence = R_INFANTRY_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_R_INFANTRY_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - rinfantry is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// rinfantry's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "HG_TAUNT", R_INFANTRY_SENTENCE_VOLUME, R_INFANTRY_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_R_INFANTRY_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* Crinfantry :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_R_INFANTRY_GRENADE ) )
				{
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "HG_THROW", R_INFANTRY_SENTENCE_VOLUME, R_INFANTRY_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return slrinfantryTossGrenadeCover;
				}
				else
				{
					return &slrinfantryTakeCover[ 0 ];
				}
			}
			else
			{
				if (FBitSet(pev->weapons, R_INFANTRY_HANDGRENADE))
				{
					if ( OccupySlot( bits_SLOTS_R_INFANTRY_GRENADE ) && RANDOM_LONG(0,1) )
					{
						return &slrinfantryGrenadeCover[ 0 ];
					}
					else
					{
						return &slrinfantryTakeCover[ 0 ];
					}
				}
				else
				{
					return &slrinfantryTakeCover[ 0 ];
				}
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slrinfantryTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_R_INFANTRY_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_R_INFANTRY_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_R_INFANTRY_ELOF_FAIL:
		{
			// human rinfantry is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_R_INFANTRY_ESTABLISH_LINE_OF_FIRE:
		{
			return &slrinfantryEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// randomly stand or crouch
			if (RANDOM_LONG(0,9) == 0)
				m_fStanding = RANDOM_LONG(0,1);
		 
			if (m_fStanding)
				return &slrinfantryRangeAttack1B[ 0 ];
			else
				return &slrinfantryRangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slrinfantryRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slrinfantryCombatFace[ 0 ];
		}
	case SCHED_R_INFANTRY_WAIT_FACE_ENEMY:
		{
			return &slrinfantryWaitInCover[ 0 ];
		}
	case SCHED_R_INFANTRY_SWEEP:
		{
			return &slrinfantrySweep[ 0 ];
		}
	case SCHED_R_INFANTRY_COVER_AND_RELOAD:
		{
			return &slrinfantryHideReload[ 0 ];
		}
	case SCHED_R_INFANTRY_FOUND_ENEMY:
		{
			return &slrinfantryFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slrinfantryFail[ 0 ];
				}
			}

			return &slrinfantryVictoryDance[ 0 ];
		}
	case SCHED_R_INFANTRY_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slrinfantrySignalSuppress[ 0 ];
			}
			else
			{
				return &slrinfantrySuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// rinfantry has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slrinfantryCombatFail[ 0 ];
			}

			return &slrinfantryFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}
