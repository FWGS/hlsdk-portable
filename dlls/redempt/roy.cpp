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

#ifdef MOBILE_HACKS
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
#include	"barney.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is roy dying for scripted sequences?
#define		ROY_AE_SHOOT		( 3 )

class CRoy : public CBarney
{
public:
	void Spawn( void );
	void Precache( void );
	void RoyFirePistol( void );
	void AlertSound( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetSchedule( void );

	void DeathSound( void );
	void PainSound( void );

	void TalkInit( void );
};

// LINK_ENTITY_TO_CLASS( monster_roy, CRoy )
extern "C" EXPORT void monster_roy( entvars_t *pev )
{
	if( g_iModType == MOD_REDEMPT )
		GetClassPtr( (CRoy *)pev );
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CRoy::AlertSound( void )
{
	if( m_hEnemy != 0 )
	{
		if( FOkToSpeak() )
		{
			PlaySentence( "RO_ATTACK", RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_IDLE );
		}
	}
}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CRoy::RoyFirePistol( void )
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
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "roy/ro_attack2.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CRoy::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ROY_AE_SHOOT:
		RoyFirePistol();
		break;
	default:
		CBarney::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CRoy::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/roy.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.royHealth;
	pev->view_ofs = Vector( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	pev->body = 0; // gun in holster
	m_fGunDrawn = FALSE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_cAmmoLoaded = GLOCK_MAX_CLIP;

	MonsterInit();
	SetUse( &CTalkMonster::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRoy::Precache()
{
	PRECACHE_MODEL( "models/roy.mdl" );

	PRECACHE_SOUND( "roy/ro_attack1.wav" );
	PRECACHE_SOUND( "roy/ro_attack2.wav" );

	PRECACHE_SOUND( "roy/ro_pain1.wav" );
	PRECACHE_SOUND( "roy/ro_pain2.wav" );
	PRECACHE_SOUND( "roy/ro_pain3.wav" );

	PRECACHE_SOUND( "roy/ro_die1.wav" );
	PRECACHE_SOUND( "roy/ro_die2.wav" );
	PRECACHE_SOUND( "roy/ro_die3.wav" );

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void CRoy::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)
	m_szGrp[TLK_ANSWER] = "RO_ANSWER";
	m_szGrp[TLK_QUESTION] =	"RO_QUESTION";
	m_szGrp[TLK_IDLE] = "RO_IDLE";
	m_szGrp[TLK_STARE] = "RO_STARE";
	m_szGrp[TLK_USE] = "RO_OK";
	m_szGrp[TLK_UNUSE] = "RO_WAIT";
	m_szGrp[TLK_STOP] = "RO_STOP";

	m_szGrp[TLK_NOSHOOT] = "RO_SCARED";
	m_szGrp[TLK_HELLO] = "RO_HELLO";

	m_szGrp[TLK_PLHURT1] = "!RO_CUREA";
	m_szGrp[TLK_PLHURT2] = "!RO_CUREB"; 
	m_szGrp[TLK_PLHURT3] = "!RO_CUREC";

	m_szGrp[TLK_PHELLO] = 0;		// UNDONE
	m_szGrp[TLK_PIDLE] = 0;			// UNDONE
	m_szGrp[TLK_PQUESTION] = "RO_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] = "RO_SMELL";

	m_szGrp[TLK_WOUND] = "RO_WOUND";
	m_szGrp[TLK_MORTAL] = "RO_MORTAL";

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}

static BOOL IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = reference - pevTest->origin;
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );

	// He's facing me, he meant it
	if( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}

int CRoy::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
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
				PlaySentence( "RO_MAD", 4, VOL_NORM, ATTN_NORM );

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "RO_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if( !( m_hEnemy->IsPlayer() ) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "RO_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

//=========================================================
// PainSound
//=========================================================
void CRoy::PainSound( void )
{
	const char *pszSound;

	if( gpGlobals->time < m_painTime )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );

	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		pszSound = "roy/ro_pain1.wav";
		break;
	case 1:
		pszSound = "roy/ro_pain2.wav";
		break;
	case 2:
		pszSound = "roy/ro_pain3.wav";
		break;
	}

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pszSound, 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// DeathSound 
//=========================================================
void CRoy::DeathSound( void )
{
	const char *pszSound;

	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		pszSound = "roy/ro_die1.wav";
		break;
	case 1:
		pszSound = "roy/ro_die2.wav";
		break;
	case 2:
		pszSound = "roy/ro_die3.wav";
		break;
	}

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pszSound, 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CRoy::GetSchedule( void )
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
		PlaySentence( "RO_KILL", 4, VOL_NORM, ATTN_NORM );
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
			if( !m_fGunDrawn )
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

void CRoy::DeclineFollowing( void )
{
	PlaySentence( "RO_POK", 2, VOL_NORM, ATTN_NORM );
}

//=========================================================
// DEAD ROY PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadRoy : public CBaseMonster
{
public:
	void Spawn( void );
	int Classify( void ) { return CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int m_iPose;// which sequence to display	-- temporary, don't need to save
	static const char *m_szPoses[3];
};

const char *CDeadRoy::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

void CDeadRoy::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

// LINK_ENTITY_TO_CLASS( monster_roy_dead, CDeadRoy )
extern "C" EXPORT void monster_roy_dead( entvars_t *pev )
{
	if( g_iModType == MOD_REDEMPT )
		GetClassPtr( (CDeadRoy *)pev );
}

//=========================================================
// ********** DeadRoy SPAWN **********
//=========================================================
void CDeadRoy::Spawn()
{
	PRECACHE_MODEL( "models/roy.mdl" );
	SET_MODEL( ENT( pev ), "models/roy.mdl" );

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if( pev->sequence == -1 )
	{
		ALERT( at_console, "Dead roy with bad pose\n" );
	}
	// Corpses have less health
	pev->health = 8;//gSkillData.royHealth;

	MonsterInitDead();
}
#endif // MOBILE_HACKS
