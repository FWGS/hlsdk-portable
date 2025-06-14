//=========================================================
// Opposing Forces Monster Gonome
//
// Made by Demiurge
//
//FGD monster_human_torch
//=========================================================
//=========================================================
// Human Torch ally
//=========================================================

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"decals.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"squadmonster.h"
#include	"talkmonster.h"
#include	"basemonster.h"

int g_fTorchQuestion;				// true if an idle grunt asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	TORCH_CLIP_SIZE					17 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define TORCH_VOL						0.35		// volume of grunt sounds
#define TORCH_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define HTORCH_LIMP_HEALTH				20
#define HTORCH_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a TORCH with a single headshot.
#define HTORCH_NUM_HEADS				1 // how many grunt heads are there? 
#define HTORCH_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	HTORCH_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

#define HTORCH_EAGLE			( 1 << 0)
#define HTORCH_NOGUN			( 1 << 3)

#define HEAD_TORCH				0
#define GUN_EAGLE				0
#define GUN_TORCH				1
#define GUN_NONE				2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HTORCH_AE_RELOAD		( 2 )
#define		HTORCH_AE_KICK			( 3 )
#define		HTORCH_AE_BURST1		( 4 )
#define		HTORCH_AE_BURST2		( 5 ) 
#define		HTORCH_AE_BURST3		( 6 ) 
#define		HTORCH_AE_GREN_TOSS		( 7 )
#define		HTORCH_AE_GREN_LAUNCH	( 8 )
#define		HTORCH_AE_GREN_DROP		( 9 )
#define		HTORCH_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		HTORCH_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.
#define		HTORCH_AE_SHOWGUN		( 17)
#define		HTORCH_AE_SHOWTORCH		( 18)
#define		HTORCH_AE_HIDETORCH		( 19)
#define		HTORCH_AE_ONGAS			( 20)
#define		HTORCH_AE_OFFGAS		( 21)

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_TORCH_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_TORCH_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_TORCH_COVER_AND_RELOAD,
	SCHED_TORCH_SWEEP,
	SCHED_TORCH_FOUND_ENEMY,
	SCHED_TORCH_REPEL,
	SCHED_TORCH_REPEL_ATTACK,
	SCHED_TORCH_REPEL_LAND,
	SCHED_TORCH_WAIT_FACE_ENEMY,
	SCHED_TORCH_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_TORCH_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_TORCH_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_TORCH_SPEAK_SENTENCE,
	TASK_TORCH_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_TORCH_NOFIRE	( bits_COND_SPECIAL1 )

class CHTorch : public CSquadMonster
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
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	virtual int	ObjectCaps( void ) { return CSquadMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	void DeathSound( void );
	void PainSound( void );
	void IdleSound ( void );
	Vector GetGunPosition( void );
	void Shoot ( void );
	void MakeGas( void );
	void UpdateGas( void );
	void KillGas( void );
	inline BOOL BeamIsOn( void ) { return m_pBeam != NULL; }

	void DeclineFollowing( void );

	void PrescheduleThink ( void );
	void GibMonster( void );
	void SpeakSentence( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	CBaseEntity	*Kick( void );
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );
//	void TalkInit( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void FollowingUse( void );
	void IsFollowing( void );

	int IRelationship ( CBaseEntity *pTarget );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	Vector m_vecDir;
	Vector m_vecEnd;

	CBeam *m_pBeam;
	float m_gasTime;
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;

	BOOL		m_fThrowGrenade;
	BOOL		m_fStanding;
	BOOL		m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int			m_cClipSize;
	int			m_voicePitch;
	int			m_iBrassShell;
	int			m_iSentence;
	float		m_flPlayerDamage;// how much pain has the player inflicted on me?

	static const char *pTorchSentences[];
};


LINK_ENTITY_TO_CLASS( monster_human_torch, CHTorch );

TYPEDESCRIPTION	CHTorch::m_SaveData[] = 
{
	DEFINE_FIELD( CHTorch, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CHTorch, m_gasTime, FIELD_TIME ),
	DEFINE_FIELD( CHTorch, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHTorch, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( CHTorch, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CHTorch, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHTorch, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHTorch, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHTorch, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHTorch, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CHTorch, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CHTorch, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHTorch, CSquadMonster );

const char *CHTorch::pTorchSentences[] = 
{
	"HG_GREN", // grenade scared grunt
//	"HG_ALERT", // sees player
	"HG_MONSTER", // sees monster
	"HG_COVER", // running to cover
	"HG_THROW", // about to throw grenade
	"HG_CHARGE",  // running out to get the enemy
	"HG_TAUNT", // say rude things
};

enum
{
	HTORCH_SENT_NONE = -1,
	HTORCH_SENT_GREN = 0,
	HTORCH_SENT_ALERT,
	HTORCH_SENT_MONSTER,
	HTORCH_SENT_COVER,
	HTORCH_SENT_THROW,
	HTORCH_SENT_CHARGE,
	HTORCH_SENT_TAUNT,
} HTORCH_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
void CHTorch :: SpeakSentence( void )
{
	if ( m_iSentence == HTORCH_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pTorchSentences[ m_iSentence ], HTORCH_SENTENCE_VOLUME, TORCH_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CHTorch::IRelationship ( CBaseEntity *pTarget )
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
void CHTorch :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

//	if ( GetBodygroup( 2 ) != 2 )
		pev->skin = 0;
	{// throw a gun if the grunt has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun = NULL;
		if (FBitSet( pev->weapons, HTORCH_EAGLE ))
		{
			pGun = DropItem( "weapon_eagle", vecGunPos, vecGunAngles );
		}
		
		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	
	
	}
	SetUse( NULL );	
	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CHTorch :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CHTorch :: FOkToSpeak( void )
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
void CHTorch :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = HTORCH_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CHTorch :: PrescheduleThink ( void )
{

	if (m_pBeam)
	{
		UpdateGas();
	}

	if ( InSquad() && m_hEnemy != 0 )
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
// FCanCheckAttacks - this is overridden for human grunts
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
BOOL CHTorch :: FCanCheckAttacks ( void )
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
BOOL CHTorch :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if( m_hEnemy != 0 )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if( !pEnemy )
		{
			return FALSE;
		}
	
		if( flDist <= 64 && flDot >= 0.7f && 
			pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
			pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
		{
			return TRUE;
		}
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for HTorch, cause 
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HTorch can attack when the enemy is
// occluded (throw grenade over wall, etc). We must 
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CHTorch :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5f && NoFriendlyFire() )
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

		if ( tr.flFraction == 1.0f )
		{
			return TRUE;
		}
	}

	return FALSE;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CHTorch :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// make sure we're wearing one
		if (GetBodygroup( 1 ) == HEAD_TORCH && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
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
	if ( DotProduct( forward, vecDir ) > 0.96f )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// TakeDamage - overridden for the torch because the torch
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CHTorch :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// make sure friends team about it if player hurts talkmonsters...
	int ret = CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == 0 )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
	//			PlaySentence( "BA_MAD", 4, VOL_NORM, ATTN_NORM );

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
	//			PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
/*		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
		}*/
	}

//	return ret;

	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHTorch :: SetYawSpeed ( void )
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
	case ACT_MELEE_ATTACK1:	
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

void CHTorch :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fTorchQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fTorchQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CHECK", HTORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fTorchQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "HG_QUEST", HTORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fTorchQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "HG_IDLE", HTORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fTorchQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CLEAR", HTORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "HG_ANSWER", HTORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fTorchQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CHTorch :: CheckAmmo ( void )
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
int	CHTorch :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
//=========================================================
CBaseEntity *CHTorch :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5f;
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

Vector CHTorch :: GetGunPosition( )				//ZZS ????? ???????-??????????????? ??????? ???????
{
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 55 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 43 );
	}
}

//=========================================================
// Shoot
//=========================================================
void CHTorch :: Shoot ( void )
{
	if (m_hEnemy == 0)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL); 
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_357 ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// AUTOGENE 
//=========================================================
void CHTorch::UpdateGas( void )
{
	TraceResult tr;
	Vector			posGun, angleGun;
	Vector			vecEndPos;

		if ( m_pBeam )
		{
//		ALERT ( at_console, "Updated gas\n");
			UTIL_MakeVectors( pev->angles );

			GetAttachment( 2, posGun, angleGun );
			
			Vector vecEnd = (gpGlobals->v_forward * 5) + posGun;
			UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &tr );

			if ( tr.flFraction != 1.0f )
			{
				m_pBeam->DoSparks( tr.vecEndPos, posGun );
				UTIL_DecalTrace(&tr, DECAL_BIGSHOT1 + RANDOM_LONG(0,4));

					MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
						WRITE_BYTE( TE_STREAK_SPLASH );
						WRITE_COORD( tr.vecEndPos.x );		// origin
						WRITE_COORD( tr.vecEndPos.y );
						WRITE_COORD( tr.vecEndPos.z );
						WRITE_COORD( tr.vecPlaneNormal.x );	// direction
						WRITE_COORD( tr.vecPlaneNormal.y );
						WRITE_COORD( tr.vecPlaneNormal.z );
						WRITE_BYTE( 10 );	// Streak color 6
						WRITE_SHORT( 40 );	// count
						WRITE_SHORT( 25 );
						WRITE_SHORT( 50 );	// Random velocity modifier
					MESSAGE_END();
			}
//ZZS: ??????????? ????? ? ???????? ???????...
//Aperance: ammazzaamaz

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_DLIGHT );
				WRITE_COORD( posGun.x );		// origin
				WRITE_COORD( posGun.y );
				WRITE_COORD( posGun.z );
				WRITE_BYTE( RANDOM_LONG(4, 16) );	// radius
				WRITE_BYTE( 251 );	// R
				WRITE_BYTE( 68 );	// G
				WRITE_BYTE( 36 );	// B
				WRITE_BYTE( 1 );	// life * 10
				WRITE_BYTE( 0 ); // decay
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex( ) + 0x1000 * 3 );		// entity, attachment
				WRITE_COORD( posGun.x );		// origin
				WRITE_COORD( posGun.y );
				WRITE_COORD( posGun.z );
				WRITE_COORD( RANDOM_LONG(8, 12) );	// radius
				WRITE_BYTE( 251 );	// R
				WRITE_BYTE( 68 );	// G
				WRITE_BYTE( 36 );	// B
				WRITE_BYTE( 1 );	// life * 10
				WRITE_COORD( 0 ); // decay 
			MESSAGE_END();
			


	pev->nextthink = gpGlobals->time + 0.1f;
		}
}

void CHTorch::MakeGas( void )
{
	Vector		posGun, angleGun;
	TraceResult tr;
	Vector vecEndPos;
//		ALERT ( at_console, "Maked gas\n");

	UTIL_MakeVectors( pev->angles );
	
			m_pBeam = CBeam::BeamCreate( g_pModelNameLaser, 7 );

	if ( m_pBeam )
		{
			GetAttachment( 4, posGun, angleGun );
			GetAttachment( 3, posGun, angleGun );

			Vector vecEnd = (gpGlobals->v_forward * 5) + posGun;
			UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &tr );

//			UTIL_Sparks( tr.vecEndPos );
//			UTIL_DecalTrace(&tr, DECAL_BIGSHOT1 + RANDOM_LONG(0,4));

			m_pBeam->EntsInit( entindex(), entindex() );
			m_pBeam->SetColor( 24, 121, 239 );
			m_pBeam->SetBrightness( 190 );
			m_pBeam->SetScrollRate( 20 );
			m_pBeam->SetStartAttachment( 4 );
			m_pBeam->SetEndAttachment( 3 );
			m_pBeam->DoSparks( tr.vecEndPos, posGun );
			m_pBeam->SetFlags( BEAM_FSHADEIN );
			m_pBeam->pev->spawnflags = SF_BEAM_SPARKSTART;

		}
	return;
}

void CHTorch :: KillGas( void )
{

	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	return;
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHTorch :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case HTORCH_AE_SHOWTORCH:
			pev->body = GUN_NONE;
			pev->body = GUN_TORCH;
			break;

		case HTORCH_AE_SHOWGUN:
			pev->body = GUN_NONE;
			pev->body = GUN_EAGLE;
			break;

		case HTORCH_AE_HIDETORCH:
			pev->body = GUN_NONE;
			break;

		case HTORCH_AE_ONGAS:
			MakeGas ();
			UpdateGas ();
			break;

		case HTORCH_AE_OFFGAS:
			KillGas ();
			break;

		case HTORCH_AE_DROP_GUN:
			{
			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );
			
			// switch to body group with no gun.
			SetBodygroup( GUN_NONE,GUN_NONE );

			// now spawn a gun.
			if (FBitSet( pev->weapons, HTORCH_EAGLE ))
			{
				 DropItem( "weapon_eagle", vecGunPos, vecGunAngles );
			}
			else
			{
			}
			}
			break;

		case HTORCH_AE_RELOAD:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case HTORCH_AE_BURST1:
		{
			if ( FBitSet( pev->weapons, HTORCH_EAGLE ))
			{
				Shoot();

				// the first round of the three round burst plays the sound and puts a sound in the world sound list.
				if ( RANDOM_LONG(0,1) )
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/bullet_hit1.wav", 1, ATTN_NORM );
				}
				else
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/bullet_hit2.wav", 1, ATTN_NORM );
				}
			}
		
			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case HTORCH_AE_BURST2:
		case HTORCH_AE_BURST3:
			Shoot();
			break;

		case HTORCH_AE_KICK:
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

		case HTORCH_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "HG_ALERT", HTORCH_SENTENCE_VOLUME, TORCH_ATTN, 0, m_voicePitch);
				 JustSpoke();
			}

		}
			break;
		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHTorch :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/hgrunt_torch.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 1;
	pev->health			= gSkillData.hgruntHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_IDLE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= HTORCH_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = HTORCH_EAGLE;
	}

		SetBodygroup( GUN_EAGLE, GUN_EAGLE );
		m_cClipSize		= TORCH_CLIP_SIZE;
	m_cAmmoLoaded		= m_cClipSize;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
//	SetUse( &CHTorch :: FollowingUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHTorch :: Precache()
{
	PRECACHE_MODEL("models/hgrunt_torch.mdl");

	PRECACHE_SOUND( "weapons/bullet_hit1.wav" );
	PRECACHE_SOUND( "weapons/bullet_hit2.wav" );
	
	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );

	PRECACHE_SOUND( "hgrunt/gr_pain1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain4.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain5.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event
//	TalkInit ();
	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

}
	
/*void CHTorch :: TalkInit()
{
	
	CSquadMonster::TalkInit();
}*/

//=========================================================
// start task
//=========================================================
void CHTorch :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_TORCH_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_TORCH_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_TORCH_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_TORCH_FACE_TOSS_DIR:
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
void CHTorch :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_TORCH_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
		}
			break;

		default:
			CSquadMonster :: RunTask( pTask );
			break;
	}
}

//=========================================================
// PainSound
//=========================================================
void CHTorch :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
#if 0
		if ( RANDOM_LONG(0,99) < 5 )
		{
			// pain sentences are rare
			if (FOkToSpeak())
			{
				SENTENCEG_PlayRndSz(ENT(pev), "HG_PAIN", HTORCH_SENTENCE_VOLUME, ATTN_NORM, 0, PITCH_NORM);
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
void CHTorch :: DeathSound ( void )
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
Task_t	tlTorchFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slTorchFollow[] =
{
	{
		tlTorchFollow,
		ARRAYSIZE ( tlTorchFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlTorchFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slTorchFaceTarget[] =
{
	{
		tlTorchFaceTarget,
		ARRAYSIZE ( tlTorchFaceTarget ),
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

Task_t	tlIdleTorchStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
//	{ TASK_TALK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleTorchStand[] =
{
	{ 
		tlIdleTorchStand,
		ARRAYSIZE ( tlIdleTorchStand ), 
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

//=========================================================
// TorchFail
//=========================================================
Task_t	tlTorchFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slTorchFail[] =
{
	{
		tlTorchFail,
		ARRAYSIZE ( tlTorchFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1, 
		0,
		"Torch Fail"
	},
};

//=========================================================
// Torch Combat Fail
//=========================================================
Task_t	tlTorchCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slTorchCombatFail[] =
{
	{
		tlTorchCombatFail,
		ARRAYSIZE ( tlTorchCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"Torch Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlTorchVictoryDance[] =
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

Schedule_t	slTorchVictoryDance[] =
{
	{ 
		tlTorchVictoryDance,
		ARRAYSIZE ( tlTorchVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"GruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlTorchEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TORCH_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_TORCH_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slTorchEstablishLineOfFire[] =
{
	{ 
		tlTorchEstablishLineOfFire,
		ARRAYSIZE ( tlTorchEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"TorchEstablishLineOfFire"
	},
};

//=========================================================
// TorchFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlTorchFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slTorchFoundEnemy[] =
{
	{ 
		tlTorchFoundEnemy,
		ARRAYSIZE ( tlTorchFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"TorchFoundEnemy"
	},
};

//=========================================================
// TorchCombatFace Schedule
//=========================================================
Task_t	tlTorchCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_TORCH_SWEEP	},
};

Schedule_t	slTorchCombatFace[] =
{
	{ 
		tlTorchCombatFace1,
		ARRAYSIZE ( tlTorchCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlTorchSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TORCH_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TORCH_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TORCH_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TORCH_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TORCH_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slTorchSignalSuppress[] =
{
	{ 
		tlTorchSignalSuppress,
		ARRAYSIZE ( tlTorchSignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_TORCH_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlTorchSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TORCH_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TORCH_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TORCH_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TORCH_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TORCH_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slTorchSuppress[] =
{
	{ 
		tlTorchSuppress,
		ARRAYSIZE ( tlTorchSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_TORCH_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlTorchWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slTorchWaitInCover[] =
{
	{ 
		tlTorchWaitInCover,
		ARRAYSIZE ( tlTorchWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1,

		bits_SOUND_DANGER,
		"TorchWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlTorchTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_TORCH_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_TORCH_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_TORCH_WAIT_FACE_ENEMY	},
};

Schedule_t	slTorchTakeCover[] =
{
	{ 
		tlTorchTakeCover1,
		ARRAYSIZE ( tlTorchTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};



//=========================================================
// Torch reload schedule
//=========================================================
Task_t	tlTorchHideReload[] =
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

Schedule_t slTorchHideReload[] = 
{
	{
		tlTorchHideReload,
		ARRAYSIZE ( tlTorchHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"TorchHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlTorchSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slTorchSweep[] =
{
	{ 
		tlTorchSweep,
		ARRAYSIZE ( tlTorchSweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Torch Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlTorchRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH },
	{ TASK_TORCH_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TORCH_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TORCH_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TORCH_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slTorchRangeAttack1A[] =
{
	{ 
		tlTorchRangeAttack1A,
		ARRAYSIZE ( tlTorchRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_TORCH_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlTorchRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_TORCH_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TORCH_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TORCH_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TORCH_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slTorchRangeAttack1B[] =
{
	{ 
		tlTorchRangeAttack1B,
		ARRAYSIZE ( tlTorchRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_TORCH_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// repel 
//=========================================================
Task_t	tlTorchRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slTorchRepel[] =
{
	{ 
		tlTorchRepel,
		ARRAYSIZE ( tlTorchRepel ), 
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
Task_t	tlTorchRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slTorchRepelAttack[] =
{
	{ 
		tlTorchRepelAttack,
		ARRAYSIZE ( tlTorchRepelAttack ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlTorchRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slTorchRepelLand[] =
{
	{ 
		tlTorchRepelLand,
		ARRAYSIZE ( tlTorchRepelLand ), 
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


DEFINE_CUSTOM_SCHEDULES( CHTorch )
{
	slTorchFail,
	slTorchCombatFail,
	slTorchVictoryDance,
	slTorchEstablishLineOfFire,
	slTorchFoundEnemy,
	slTorchCombatFace,
	slTorchSignalSuppress,
	slTorchSuppress,
	slTorchWaitInCover,
	slTorchTakeCover,
	slTorchHideReload,
	slTorchSweep,
	slTorchRangeAttack1A,
	slTorchRangeAttack1B,
	slTorchRepel,
	slTorchRepelAttack,
	slTorchRepelLand,
	slTorchFollow,
	slTorchFaceTarget,
	slIdleTorchStand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHTorch, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CHTorch :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if (FBitSet( pev->weapons, HTORCH_EAGLE))
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
		break;
	
	case ACT_RUN:
		if ( pev->health <= HTORCH_LIMP_HEALTH )
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
		if ( pev->health <= HTORCH_LIMP_HEALTH )
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
Schedule_t *CHTorch :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = HTORCH_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_TORCH_REPEL_LAND );
		}
		else
		{
			// repel down a rope, 
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_TORCH_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_TORCH_REPEL );
		}
	}
		// grunts place HIGH priority on running away from danger sounds.
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
				
				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "HG_GREN", HTORCH_SENTENCE_VOLUME, TORCH_ATTN, 0, m_voicePitch);
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
						//!!!KELLY - the leader of a squad of grunts has just seen the player or a 
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight" 
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != 0) && m_hEnemy->IsPlayer())
								// player
								SENTENCEG_PlayRndSz( ENT(pev), "HG_ALERT", HTORCH_SENTENCE_VOLUME, TORCH_ATTN, 0, m_voicePitch);
							else if ((m_hEnemy != 0) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) && 
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) && 
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "HG_MONST", HTORCH_SENTENCE_VOLUME, TORCH_ATTN, 0, m_voicePitch);

							JustSpoke();
						}
						
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_TORCH_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_TORCH_ESTABLISH_LINE_OF_FIRE );
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
				return GetScheduleOfType ( SCHED_TORCH_COVER_AND_RELOAD );
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != 0 )
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = HTORCH_SENT_COVER;
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
						return GetScheduleOfType ( SCHED_TORCH_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
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
				if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					//!!!KELLY - grunt cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = HTORCH_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_TORCH_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - grunt is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// grunt's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "HG_TAUNT", HTORCH_SENTENCE_VOLUME, TORCH_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_TORCH_ESTABLISH_LINE_OF_FIRE );
			}
			break;
	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

//		if ( m_hEnemy == 0 && IsFollowing() )
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


//		TrySmellTalk();
//		break;

		}
	default:
		break;
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

MONSTERSTATE CHTorch :: GetIdealState ( void )
{
	return CSquadMonster::GetIdealState();
}

void CHTorch::DeclineFollowing( void )
{
//	PlaySentence( "BA_POK", 2, VOL_NORM, ATTN_NORM );
}


//=========================================================
//=========================================================
Schedule_t* CHTorch :: GetScheduleOfType ( int Type ) 
{
	Schedule_t *psched;
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
		
			{
				if ( RANDOM_LONG(0,1) )
				{
					return &slTorchTakeCover[ 0 ];
				}
				
			}
		}

	case SCHED_TORCH_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_TORCH_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_TORCH_ESTABLISH_LINE_OF_FIRE:
		{
			return &slTorchEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// randomly stand or crouch
			if (RANDOM_LONG(0,9) == 0)
				m_fStanding = RANDOM_LONG(0,1);
		 
			if (m_fStanding)
				return &slTorchRangeAttack1B[ 0 ];
			else
				return &slTorchRangeAttack1A[ 0 ];
		}
	
	case SCHED_COMBAT_FACE:
		{
			return &slTorchCombatFace[ 0 ];
		}
	case SCHED_TORCH_WAIT_FACE_ENEMY:
		{
			return &slTorchWaitInCover[ 0 ];
		}
	case SCHED_TORCH_SWEEP:
		{
			return &slTorchSweep[ 0 ];
		}
	case SCHED_TORCH_COVER_AND_RELOAD:
		{
			return &slTorchHideReload[ 0 ];
		}
	case SCHED_TORCH_FOUND_ENEMY:
		{
			return &slTorchFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slTorchFail[ 0 ];
				}
			}

			return &slTorchVictoryDance[ 0 ];
		}
	case SCHED_TORCH_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slTorchSignalSuppress[ 0 ];
			}
			else
			{
				return &slTorchSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != 0 )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slTorchCombatFail[ 0 ];
			}

			return &slTorchFail[ 0 ];
		}
	case SCHED_TORCH_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slTorchRepel[ 0 ];
		}
	case SCHED_TORCH_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slTorchRepelAttack[ 0 ];
		}
	case SCHED_TORCH_REPEL_LAND:
		{
			return &slTorchRepelLand[ 0 ];
		}
	case SCHED_TARGET_FACE:
		{
		// call base class default so that barney will talk
		// when 'used' 
		psched = CSquadMonster::GetScheduleOfType(Type);
		if (psched == slIdleStand)
			return slTorchFaceTarget;	// override this for different target face behavior
		else
			return psched;

		}
	case SCHED_TARGET_CHASE:
		{
		return slTorchFollow;
		}
	case SCHED_IDLE_STAND:
		{
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CSquadMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleTorchStand;
		}
		else
			return psched;	
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
}
}

//=========================================================
// CHTorchRepel - when triggered, spawns a monster_human_grunt
// repelling down a line.
//=========================================================

class CHTorchRepel : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;	// Don't save, precache
};

LINK_ENTITY_TO_CLASS( monster_torch_repel, CHTorchRepel );

void CHTorchRepel::Spawn( void )
{
	Precache( );
	pev->solid = SOLID_NOT;

	SetUse( &CHTorchRepel::RepelUse );
}

void CHTorchRepel::Precache( void )
{
	UTIL_PrecacheOther( "monster_human_torch" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CHTorchRepel::RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP) 
		return NULL;
	*/

	CBaseEntity *pEntity = Create( "monster_human_torch", pev->origin, pev->angles );
	CBaseMonster *pTorch = pEntity->MyMonsterPointer( );
	pTorch->pev->movetype = MOVETYPE_FLY;
	pTorch->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	pTorch->SetActivity( ACT_GLIDE );
	// UNDONE: position?
	pTorch->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector(0,0,112), pTorch->entindex() );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink( &CBaseEntity::SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + -4096.0f * tr.flFraction / pTorch->pev->velocity.z + 0.5f;

	UTIL_Remove( this );
}



//=========================================================
// DEAD HGRUNT PROP
//=========================================================
class CDeadHTorch : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static const char *m_szPoses[3];
};

const char *CDeadHTorch::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadHTorch::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_htorch_dead, CDeadHTorch );

//=========================================================
// ********** DeadHTorch SPAWN **********
//=========================================================
void CDeadHTorch :: Spawn( void )
{
	PRECACHE_MODEL("models/hgrunt_torch.mdl");
	SET_MODEL(ENT(pev), "models/hgrunt_torch.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead htorch with bad pose\n" );
	}

	// Corpses have less health
	pev->health			= 8;

	// map old bodies onto new bodies
	switch( pev->body )
	{
	case 0: // Grunt with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_TORCH, HEAD_TORCH );
		SetBodygroup( GUN_NONE, GUN_EAGLE );
		break;
	case 1: // Grunt no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_TORCH, HEAD_TORCH );
		SetBodygroup( GUN_NONE, GUN_NONE );
		break;
	}

	MonsterInitDead();
}
