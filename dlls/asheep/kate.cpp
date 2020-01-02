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
#include	"barney.h"
#include        "animation.h"

#define	KATE_LIMP_HEALTH		40

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?

class CKate : public CBarney
{
public:
        void Spawn( void );
        void Precache( void );
        void BarneyFirePistol( void );
        void AlertSound( void );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
        void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IRelationship( CBaseEntity *pTarget );
	CBaseEntity* CheckTraceHullAttack( float flDist, int iDamage, int iDmgType );
        int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void SetActivity( Activity NewActivity );

        void DeclineFollowing( void );

        // Override these to set behavior
        Schedule_t *GetSchedule( void );

        void DeathSound( void );
        void PainSound( void );

        void TalkInit( void );

	int m_iCombatState;
};

LINK_ENTITY_TO_CLASS( monster_kate, CKate )

extern Schedule_t	slBaFollow[];
extern Schedule_t	slBarneyEnemyDraw[];
extern Schedule_t	slBaFaceTarget[];
extern Schedule_t	slIdleBaStand[];
extern Schedule_t	slGruntHideReload[];

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CKate::AlertSound( void )
{
	if( m_hEnemy != 0 )
	{
		if( FOkToSpeak() )
		{
			PlaySentence( "KA_ATTACK", RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_IDLE );
		}
	}
}

int CKate::IRelationship( CBaseEntity *pTarget )
{
        if( FClassnameIs( pTarget->pev, "monster_archer" ) || FClassnameIs( pTarget->pev, "monster_ichthyosaur" ) )
        {
                return R_NO;
        }
        
        return CTalkMonster::IRelationship( pTarget );
}

CBaseEntity* CKate::CheckTraceHullAttack( float flDist, int iDamage, int iDmgType )
{
        TraceResult tr;

	UTIL_MakeVectors( pev->angles );

        Vector vecStart = pev->origin;
        vecStart.z += pev->size.z * 0.5f;
        Vector vecEnd = vecStart + ( gpGlobals->v_forward * flDist );

        UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT( pev ), &tr );

        if( tr.pHit )
                return CBaseEntity::Instance( tr.pHit );

        return NULL;
}

BOOL CKate::CheckMeleeAttack1( float flDot, float flDist )
{
	CBaseMonster *pEnemy = 0;

        if( m_hEnemy != 0 )
        {
                pEnemy = m_hEnemy->MyMonsterPointer();
                                
                if( !pEnemy )
                {
                        return FALSE;
                }
        }

        if( flDist <= 64 && flDot >= 0.7f &&
                 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
                 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
        {
                return TRUE;
        }
        return FALSE;
}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CKate::BarneyFirePistol( void )
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
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "kate/ka_attack2.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

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
void CKate::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	CBaseEntity *pHurt;

	switch( pEvent->event )
	{
	case BARNEY_AE_KICK:
		pHurt = CheckTraceHullAttack( 70, 0, DMG_GENERIC );
		if( pHurt )
		{
			const char *pszSound;
			if( m_iCombatState == -1 )
			{
				pszSound = "common/kick.wav";
			}
			else
			{
				++m_iCombatState;
				if( m_iCombatState == 3 )
					pszSound = "common/kick.wav";
				else
					pszSound = "common/punch.wav";
			}
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pszSound, 1, ATTN_NORM, 0, PITCH_NORM );
			UTIL_MakeVectors( pev->angles );

			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );
		}
		break;
	default:
		CBarney::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CKate::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/kate.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.kateHealth;
	pev->view_ofs = Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	pev->body = 0; // gun in holster
	m_fGunDrawn = FALSE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_cAmmoLoaded = GLOCK_MAX_CLIP;

	m_iCombatState = -1;

	MonsterInit();
	SetUse( &CTalkMonster::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CKate::Precache()
{
	PRECACHE_MODEL( "models/kate.mdl" );

	PRECACHE_SOUND( "kate/ka_attack1.wav" );
	PRECACHE_SOUND( "kate/ka_attack2.wav" );

	PRECACHE_SOUND( "kate/ka_pain1.wav" );
	PRECACHE_SOUND( "kate/ka_pain2.wav" );
	//PRECACHE_SOUND( "kate/ka_pain3.wav" );

	PRECACHE_SOUND( "kate/ka_die1.wav" );
	//PRECACHE_SOUND( "kate/ka_die2.wav" );
	//PRECACHE_SOUND( "kate/ka_die3.wav" );

	PRECACHE_SOUND( "weapons/reload3.wav" );

	PRECACHE_SOUND( "zombie/claw_miss1.wav" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" );
	PRECACHE_SOUND( "common/kick.wav" );
	PRECACHE_SOUND( "common/punch.wav" );

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void CKate::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)
	m_szGrp[TLK_ANSWER] = "KA_ANSWER";
	m_szGrp[TLK_QUESTION] =	"KA_QUESTION";
	m_szGrp[TLK_IDLE] = "KA_IDLE";
	m_szGrp[TLK_STARE] = "KA_STARE";
	m_szGrp[TLK_USE] = "KA_OK";
	m_szGrp[TLK_UNUSE] = "KA_WAIT";
	m_szGrp[TLK_STOP] = "KA_STOP";

	m_szGrp[TLK_NOSHOOT] = "KA_SCARED";
	m_szGrp[TLK_HELLO] = "KA_HELLO";

	m_szGrp[TLK_PLHURT1] = "!KA_CUREA";
	m_szGrp[TLK_PLHURT2] = "!KA_CUREB"; 
	m_szGrp[TLK_PLHURT3] = "!KA_CUREC";

	m_szGrp[TLK_PHELLO] = NULL;	//"KA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] = NULL;	//"KA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "KA_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] = "KA_SMELL";

	m_szGrp[TLK_WOUND] = "KA_WOUND";
	m_szGrp[TLK_MORTAL] = "KA_MORTAL";

	// get voice for head - just one barney voice for now
	m_voicePitch = 98;
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
	if( DotProduct( forward, vecDir ) > 0.96f )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}

int CKate::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
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
				PlaySentence( "KA_MAD", 4, VOL_NORM, ATTN_NORM );

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "KA_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if( !( m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "KA_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

//=========================================================
// PainSound
//=========================================================
void CKate::PainSound( void )
{
	if( gpGlobals->time < m_painTime )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_LONG( 0, 1 ) ? "kate/ka_pain1.wav" : "kate/ka_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// DeathSound 
//=========================================================
void CKate::DeathSound( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "kate/ka_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
}

//=========================================================
// SetActivity 
//=========================================================
void CKate::SetActivity( Activity NewActivity )
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;
	//void *pmodel = GET_MODEL_PTR( ENT( pev ) );

	switch( NewActivity )
	{
	case ACT_RUN:
		if( pev->health <= KATE_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity( ACT_RUN_HURT );
		}
		else
		{
			iSequence = LookupActivity( NewActivity );
		}
		break;
	case ACT_WALK:
		if( pev->health <= KATE_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity( ACT_WALK_HURT );
		}
		else
		{
			iSequence = LookupActivity( NewActivity );
		}
		break;
	case ACT_MELEE_ATTACK1:
		if( RANDOM_LONG( 0, 2 ) )
		{
			m_iCombatState = -1;
			iSequence = LookupSequence( "karate_hit" );
		}
		else
		{
			m_iCombatState = 0;
			iSequence = LookupSequence( "karate_bighit" );
		}
		break;
	default:
		iSequence = LookupActivity( NewActivity );
		break;
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT( at_console, "%s has no sequence for act:%d\n", STRING( pev->classname ), NewActivity );
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CKate::GetSchedule( void )
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
		PlaySentence( "KA_KILL", 4, VOL_NORM, ATTN_NORM );
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

			if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );

			if( HasConditions( bits_COND_NO_AMMO_LOADED ) )
				return GetScheduleOfType( SCHED_BARNEY_COVER_AND_RELOAD );
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

void CKate::DeclineFollowing( void )
{
	PlaySentence( "KA_POK", 2, VOL_NORM, ATTN_NORM );
}
