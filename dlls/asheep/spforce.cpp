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
#include	"hgrunt.h"

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
#define SPFORCE_BERETTA				( 1 << 3)

#define HEAD_GROUP					1
#define HEAD_SPForce				0
#define HEAD_COMMANDER				1
#define HEAD_BERETTA				2
#define HEAD_M203					3
#define GUN_GROUP					2
#define GUN_M41A					0
#define GUN_BERETTA					1
#define GUN_NONE					2

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

class CSPForce : public CHGrunt
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void Shoot ( void );
	void Beretta ( void );
	void GibMonster( void );

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int IRelationship ( CBaseEntity *pTarget );
};

LINK_ENTITY_TO_CLASS( monster_human_spforce, CSPForce );

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
	else if( FClassnameIs( pTarget->pev, "monster_human_grunt" ) )
	{
		return R_HT;
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
//Alex994 begin - for beretta spawning
		if (FBitSet( pev->weapons, SPFORCE_BERETTA ))
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
// Shoot
//=========================================================
void CSPForce :: Shoot ( void )
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
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5); // shoot +-5 degrees		

	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shoot
//=========================================================
void CSPForce :: Beretta ( void )
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
	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_4DEGREES, 2048, BULLET_MONSTER_9MM, 0 ); // shoot +-7.5 degrees

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

			//Alex begin
			if (FBitSet( pev->weapons, SPFORCE_BERETTA ))
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
			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if ( FBitSet( pev->weapons, SPFORCE_BERETTA ) )
			{
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "spforce/spf_reload2.wav", 1, ATTN_NORM );
			}
			else
			{
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "spforce/spf_reload1.wav", 1, ATTN_NORM );
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
			CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity, gSkillData.plrDmgM41AGrenade );
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
			else
			{
				Beretta();

				// the first round of the three round burst plays the sound and puts a sound in the world sound list.
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/beretta_fire1.wav", 1, ATTN_NORM );
			
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
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "common/kick.wav", 1, ATTN_NORM );
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
			break;
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
	m_iSentence			= HGRUNT_SENT_NONE;

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

	if (FBitSet( pev->weapons, SPFORCE_BERETTA ))
	{
		SetBodygroup( GUN_GROUP, GUN_BERETTA );
		m_cClipSize = 15;
	}
	else
	{
		m_cClipSize = 45;
	}
	m_cAmmoLoaded		= m_cClipSize;

	if (FBitSet( pev->weapons, SPFORCE_BERETTA ))
	{
		SetBodygroup( HEAD_GROUP, HEAD_BERETTA);
	}
	else if (FBitSet( pev->weapons, SPFORCE_GRENADELAUNCHER ))
	{
		SetBodygroup( HEAD_GROUP, HEAD_M203 );
	}

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSPForce :: Precache()
{
	PRECACHE_MODEL( "models/spforce.mdl" );

	PRECACHE_SOUND( "weapons/m41ahks1.wav" );
	PRECACHE_SOUND( "weapons/m41ahks2.wav" );

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

	PRECACHE_SOUND( "weapons/beretta_fire1.wav" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" );// because we use the basemonster SWIPE animation event
	PRECACHE_SOUND( "common/kick.wav" );

	// get voice pitch
	m_voicePitch = 92 + RANDOM_LONG( 0, 6 );

	m_iBrassShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shell
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
	pBeam->pev->nextthink = gpGlobals->time + -4096.0f * tr.flFraction / pSPForce->pev->velocity.z + 0.5f;

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
	static const char *m_szPoses[3];
};

const char *CDeadSPForce::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

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
