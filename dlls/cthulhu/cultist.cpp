/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// cultist
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
#include	"util.h"
#include	"plane.h"
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

int g_fCultistQuestion;				// true if an idle cultist asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	CULTIST_CLIP_SIZE				6 // how many bullets in a clip?
#define CULTIST_VOL						0.70		// volume of cultist sounds
#define CULTIST_ATTN					ATTN_NORM	// attenutation of cultist sentences
#define CULTIST_LIMP_HEALTH				20
#define CULTIST_DMG_HEADSHOT			( DMG_BULLET | DMG_CLUB )	// damage types that can kill a cultist with a single headshot.
#define CULTIST_NUM_HEADS				3 // how many cultist heads are there? 
#define CULTIST_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	CULTIST_SENTENCE_VOLUME			(float)0.70 // volume of cultist sentences

#define CULTIST_KNIFE				( 1 << 0)
#define CULTIST_REVOLVER			( 1 << 1)
#define CULTIST_SHOTGUN				( 1 << 2)

#define HEAD_GROUP					1
#define HEAD_GRIM					0
#define HEAD_BEARDY					1
#define HEAD_UGLY					2

#define GUN_GROUP					2
#define GUN_KNIFE					0
#define GUN_REVOLVER				1
#define GUN_SHOTGUN					2
#define GUN_NONE					3

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		CULTIST_AE_RELOAD		( 2 )
#define		CULTIST_AE_KICK			( 3 )
#define		CULTIST_AE_BURST1		( 4 )
#define		CULTIST_AE_CAUGHT_ENEMY	( 5 ) // cultist established sight with an enemy (player only) that had previously eluded the squad.
#define		CULTIST_AE_DROP_GUN		( 6 ) // cultist (probably dead) is dropping his mp5.
#define		CULTIST_AE_KNIFE		( 7 )

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_CULTIST_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_CULTIST_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_CULTIST_COVER_AND_RELOAD,
	SCHED_CULTIST_SWEEP,
	SCHED_CULTIST_FOUND_ENEMY,
	SCHED_CULTIST_WAIT_FACE_ENEMY,
	SCHED_CULTIST_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_CULTIST_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_CULTIST_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_CULTIST_SPEAK_SENTENCE,
	TASK_CULTIST_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_CULTIST_NOFIRE	( bits_COND_SPECIAL1 )


#include "cultist.h"


LINK_ENTITY_TO_CLASS( monster_cultist, CCultist );

TYPEDESCRIPTION	CCultist::m_SaveData[] = 
{
	DEFINE_FIELD( CCultist, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( CCultist, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CCultist, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CCultist, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CCultist, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CCultist, m_voicePitch, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CCultist, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CCultist, CSquadMonster );

const char *CCultist::pCultistSentences[] = 
{
	"CULT_GREN", // grenade scared cultist
	"CULT_ALERT", // sees player
	"CULT_COVER", // running to cover
	"CULT_CHARGE",  // running out to get the enemy
	"CULT_TAUNT", // say rude things
};

enum
{
	CULTIST_SENT_NONE = -1,
	CULTIST_SENT_GREN = 0,
	CULTIST_SENT_ALERT,
	CULTIST_SENT_COVER,
	CULTIST_SENT_CHARGE,
	CULTIST_SENT_TAUNT,
} CULTIST_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some cultist sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a cultist says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the cultist has 
// started moving.
//=========================================================
void CCultist :: SpeakSentence( void )
{
	if ( m_iSentence == CULTIST_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pCultistSentences[ m_iSentence ], CULTIST_SENTENCE_VOLUME, CULTIST_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CCultist :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if (( GetBodygroup( GUN_GROUP ) != GUN_NONE ) && !(pev->spawnflags & SF_MONSTER_NO_WPN_DROP))
	{// throw a gun if the cultist has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun;
		if (FBitSet( pev->weapons, CULTIST_SHOTGUN ))
		{
			pGun = DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
		}
		else if (FBitSet( pev->weapons, CULTIST_REVOLVER ))
		{
			pGun = DropItem( "weapon_revolver", vecGunPos, vecGunAngles );
		}
		else
		{
			pGun = DropItem( "weapon_knife", vecGunPos, vecGunAngles );
		}
		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}

	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human cultists because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CCultist :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CCultist :: FOkToSpeak( void )
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
void CCultist :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = CULTIST_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CCultist :: PrescheduleThink ( void )
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
// FCanCheckAttacks - this is overridden.
//=========================================================
BOOL CCultist :: FCanCheckAttacks ( void )
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
BOOL CCultist :: CheckMeleeAttack1 ( float flDot, float flDist )
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

	if (!FBitSet( pev->weapons, CULTIST_KNIFE ))
	{
		return FALSE;
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
// CheckRangeAttack1 - overridden for cultists.
//=========================================================
BOOL CCultist :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (FBitSet( pev->weapons, CULTIST_KNIFE ))
	{
		return FALSE;
	}

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
// CheckRangeAttack2 - this checks the cultists's (non-existant) second range
// attack. 
//=========================================================
BOOL CCultist :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CCultist :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the cultist because the cultist
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CCultist :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCultist :: SetYawSpeed ( void )
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

void CCultist :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fCultistQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fCultistQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "CULT_CHECK", CULTIST_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fCultistQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "CULT_QUEST", CULTIST_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fCultistQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "CULT_IDLE", CULTIST_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fCultistQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "CULT_CLEAR", CULTIST_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "CULT_ANSWER", CULTIST_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fCultistQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the cultist because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CCultist :: CheckAmmo ( void )
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
int	CCultist :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_HUMAN_CULTIST;
}

//=========================================================
//=========================================================
CBaseEntity *CCultist :: Kick( void )
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

Vector CCultist :: GetGunPosition( )
{
	// because origin is at the centre of the box, not at the bottom
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 24 );
//		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 12 );
//		return pev->origin + Vector( 0, 0, 48 );
	}
}

//=========================================================
// Shoot
//=========================================================
void CCultist :: Shoot ( void )
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
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_9MM ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shoot
//=========================================================
void CCultist :: Shotgun ( void )
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
	FireBullets(gSkillData.cultistShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_SHOTGUN, 0 ); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;
	
	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCultist :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case CULTIST_AE_DROP_GUN:
			{
			if (pev->spawnflags & SF_MONSTER_NO_WPN_DROP) break;

			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			SetBodygroup( GUN_GROUP, GUN_NONE );

			// now spawn a gun.
			if (FBitSet( pev->weapons, CULTIST_SHOTGUN ))
			{
				 DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
			}
			else if (FBitSet( pev->weapons, CULTIST_REVOLVER ))
			{
				 DropItem( "weapon_revolver", vecGunPos, vecGunAngles );
			}
			else
			{
				 DropItem( "weapon_knife", vecGunPos, vecGunAngles );
			}

			}
			break;

		case CULTIST_AE_RELOAD:
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case CULTIST_AE_BURST1:
		{
			if ( FBitSet( pev->weapons, CULTIST_KNIFE ))
			{
				// TO DO: need to enter knife logic
			}
			else if ( FBitSet( pev->weapons, CULTIST_REVOLVER ))
			{
				Shoot();

				// the first round of the three round burst plays the sound and puts a sound in the world sound list.
				if ( RANDOM_LONG(0,1) )
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/revolver_shot1.wav", 1, ATTN_NORM );
				}
				else
				{
					EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/revolver_shot2.wav", 1, ATTN_NORM );
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

		case CULTIST_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				// SOUND HERE!
				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.cultistDmgKick, DMG_CLUB );
			}
		}
		break;

		case CULTIST_AE_KNIFE:
		{
			Vector oldorig = pev->origin;
			CBaseEntity *pHurt = NULL;
			// check down below in stages for snakes...
			for (int dz = 0; dz >=-3; dz--)
			{
				pev->origin = oldorig;
				pev->origin.z += dz * 12;
				pHurt = CheckTraceHullAttack( 70, gSkillData.priestDmgKnife, DMG_SLASH );
				if (pHurt) 
				{
					break;
				}
			}
			pev->origin = oldorig;
			if ( pHurt )
			{
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "zombie/claw_miss1.wav", 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
		break;

		case CULTIST_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "CULT_ALERT", CULTIST_SENTENCE_VOLUME, CULTIST_ATTN, 0, m_voicePitch);
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
void CCultist :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/cultist.mdl");
	//UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	UTIL_SetSize(pev, Vector(-16,-16,-36), Vector(16,16,36));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= gSkillData.cultistHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= CULTIST_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the cultist spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 19 );
	//m_HackedGunPos = Vector ( 0, 0, 55 );

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = CULTIST_KNIFE;
		// pev->weapons = CULTIST_REVOLVER;
		// pev->weapons = CULTIST_SHOTGUN;
	}

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, 2);// pick a head, any head
	}

	if (FBitSet( pev->weapons, CULTIST_SHOTGUN ))
	{
		SetBodygroup( GUN_GROUP, GUN_SHOTGUN );
		m_cClipSize		= 2;
	}
	else if (FBitSet( pev->weapons, CULTIST_REVOLVER ))
	{
		SetBodygroup( GUN_GROUP, GUN_REVOLVER );
		m_cClipSize		= CULTIST_CLIP_SIZE;
	}
	else // knife
	{
		SetBodygroup( GUN_GROUP, GUN_KNIFE );
		m_cClipSize		= 1;
	}
	m_cAmmoLoaded		= m_cClipSize;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCultist :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/cultist.mdl");

	PRECACHE_SOUND( "hgrunt/gr_mgun1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_mgun2.wav" );
	
	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );

	PRECACHE_SOUND( "hgrunt/gr_pain3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain4.wav" );

	PRECACHE_SOUND( "hgrunt/gr_reload1.wav" );

	PRECACHE_SOUND( "weapons/glauncher.wav" );

	PRECACHE_SOUND( "weapons/sbarrel1.wav" );
	PRECACHE_SOUND( "weapons/revolver_shot1.wav" );

	PRECACHE_SOUND("zombie/claw_strike1.wav");// because we use the basemonster SWIPE animation event
	PRECACHE_SOUND("zombie/claw_miss1.wav");// because we use the basemonster SWIPE animation event

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
void CCultist :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_CULTIST_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_CULTIST_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_CULTIST_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// cultist no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_CULTIST_FACE_TOSS_DIR:
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
void CCultist :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CULTIST_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin );
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
void CCultist :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain3.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain4.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CCultist :: DeathSound ( void )
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
// CultistFail
//=========================================================
Task_t	tlCultistFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slCultistFail[] =
{
	{
		tlCultistFail,
		ARRAYSIZE ( tlCultistFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Cultist Fail"
	},
};

//=========================================================
// Cultist Combat Fail
//=========================================================
Task_t	tlCultistCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slCultistCombatFail[] =
{
	{
		tlCultistCombatFail,
		ARRAYSIZE ( tlCultistCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Cultist Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlCultistVictoryDance[] =
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

Schedule_t	slCultistVictoryDance[] =
{
	{ 
		tlCultistVictoryDance,
		ARRAYSIZE ( tlCultistVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"CultistVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the cultist to attack.
//=========================================================
Task_t tlCultistEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CULTIST_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_CULTIST_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slCultistEstablishLineOfFire[] =
{
	{ 
		tlCultistEstablishLineOfFire,
		ARRAYSIZE ( tlCultistEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"CultistEstablishLineOfFire"
	},
};

//=========================================================
// CultistFoundEnemy - cultist established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlCultistFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slCultistFoundEnemy[] =
{
	{ 
		tlCultistFoundEnemy,
		ARRAYSIZE ( tlCultistFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"CultistFoundEnemy"
	},
};

//=========================================================
// CultistCombatFace Schedule
//=========================================================
Task_t	tlCultistCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_CULTIST_SWEEP	},
};

Schedule_t	slCultistCombatFace[] =
{
	{ 
		tlCultistCombatFace1,
		ARRAYSIZE ( tlCultistCombatFace1 ), 
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
// empty or cultist gets hurt.
//=========================================================
Task_t	tlCultistSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_CULTIST_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_CULTIST_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_CULTIST_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_CULTIST_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_CULTIST_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slCultistSignalSuppress[] =
{
	{ 
		tlCultistSignalSuppress,
		ARRAYSIZE ( tlCultistSignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_CULTIST_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlCultistSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CULTIST_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CULTIST_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CULTIST_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CULTIST_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_CULTIST_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slCultistSuppress[] =
{
	{ 
		tlCultistSuppress,
		ARRAYSIZE ( tlCultistSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_CULTIST_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// cultist wait in cover - we don't allow danger or the ability
// to attack to break a cultist's run to cover schedule, but
// when a cultist is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlCultistWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slCultistWaitInCover[] =
{
	{ 
		tlCultistWaitInCover,
		ARRAYSIZE ( tlCultistWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"CultistWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlCultistTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_CULTIST_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_CULTIST_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_CULTIST_WAIT_FACE_ENEMY	},
};

Schedule_t	slCultistTakeCover[] =
{
	{ 
		tlCultistTakeCover1,
		ARRAYSIZE ( tlCultistTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlCultistTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slCultistTakeCoverFromBestSound[] =
{
	{ 
		tlCultistTakeCoverFromBestSound,
		ARRAYSIZE ( tlCultistTakeCoverFromBestSound ), 
		0,
		0,
		"CultistTakeCoverFromBestSound"
	},
};

//=========================================================
// Cultist reload schedule
//=========================================================
Task_t	tlCultistHideReload[] =
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

Schedule_t slCultistHideReload[] = 
{
	{
		tlCultistHideReload,
		ARRAYSIZE ( tlCultistHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"CultistHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlCultistSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slCultistSweep[] =
{
	{ 
		tlCultistSweep,
		ARRAYSIZE ( tlCultistSweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Cultist Sweep"
	},
};

//=========================================================
// primary range attack. Overriden.
//=========================================================
Task_t	tlCultistRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
//	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH },
	{ TASK_CULTIST_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CULTIST_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CULTIST_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CULTIST_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slCultistRangeAttack1A[] =
{
	{ 
		tlCultistRangeAttack1A,
		ARRAYSIZE ( tlCultistRangeAttack1A ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_CULTIST_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,
		
		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden.
//=========================================================
Task_t	tlCultistRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
//	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE  },
	{ TASK_CULTIST_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CULTIST_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CULTIST_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_CULTIST_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slCultistRangeAttack1B[] =
{
	{ 
		tlCultistRangeAttack1B,
		ARRAYSIZE ( tlCultistRangeAttack1B ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_CULTIST_NOFIRE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

DEFINE_CUSTOM_SCHEDULES( CCultist )
{
	slCultistFail,
	slCultistCombatFail,
	slCultistVictoryDance,
	slCultistEstablishLineOfFire,
	slCultistFoundEnemy,
	slCultistCombatFace,
	slCultistSignalSuppress,
	slCultistSuppress,
	slCultistWaitInCover,
	slCultistTakeCover,
	slCultistTakeCoverFromBestSound,
	slCultistHideReload,
	slCultistSweep,
	slCultistRangeAttack1A,
	slCultistRangeAttack1B,
};

IMPLEMENT_CUSTOM_SCHEDULES( CCultist, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CCultist :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_MELEE_ATTACK1:

		// randomly stand or crouch
		if (RANDOM_LONG(0,99) == 0)
			m_fStanding = 0;
		else
			m_fStanding = 1;
		 
		// a short enemy...probably a snake...
		if ((m_hEnemy != NULL) && (m_hEnemy->pev->maxs.z < 36) && FBitSet( pev->weapons, CULTIST_KNIFE))
		{
			m_fStanding = 0;
		}

		if (FBitSet( pev->weapons, CULTIST_KNIFE))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "ref_shoot_knife" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouch_shoot_knife" );
			}
		}
		break;
	case ACT_RANGE_ATTACK1:

		// randomly stand or crouch
		if (RANDOM_LONG(0,99) == 0)
			m_fStanding = 0;
		else
			m_fStanding = 1;

		// cultist is either shooting standing or shooting crouched
		if (FBitSet( pev->weapons, CULTIST_REVOLVER))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "ref_shoot_onehanded" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouch_shoot_onehanded" );
			}
		}
		else if (FBitSet( pev->weapons, CULTIST_SHOTGUN))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "ref_shoot_shotgun" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouch_shoot_shotgun" );
			}
		}
		break;
	case ACT_RUN:
		if ( pev->health <= CULTIST_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
			if ( iSequence == ACTIVITY_NOT_AVAILABLE )
			{
				iSequence = LookupActivity ( ACT_RUN );
			}
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= CULTIST_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
			if ( iSequence == ACTIVITY_NOT_AVAILABLE )
			{
				iSequence = LookupActivity ( ACT_WALK );
			}
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
//			NewActivity = ACT_IDLE_ANGRY;
			NewActivity = ACT_IDLE;
		}
		iSequence = LookupActivity ( NewActivity );
		if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		{
			iSequence = LookupActivity ( ACT_IDLE );
		}
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
Schedule_t *CCultist :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = CULTIST_SENT_NONE;

	// cultists place HIGH priority on running away from danger sounds.
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
				
				//!!!KELLY - currently, this is the cultist's signal that a grenade has landed nearby,
				// and the cultist should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "CULT_GREN", CULTIST_SENTENCE_VOLUME, CULTIST_ATTN, 0, m_voicePitch);
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
						//!!!KELLY - the leader of a squad of cultists has just seen the player or a 
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
								SENTENCEG_PlayRndSz( ENT(pev), "CULT_ALERT", CULTIST_SENTENCE_VOLUME, CULTIST_ATTN, 0, m_voicePitch);
							/*
								else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) && 
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) && 
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "CULT_MONST", CULTIST_SENTENCE_VOLUME, CULTIST_ATTN, 0, m_voicePitch);
								*/

							JustSpoke();
						}
						
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_CULTIST_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_CULTIST_ESTABLISH_LINE_OF_FIRE );
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
				return GetScheduleOfType ( SCHED_CULTIST_COVER_AND_RELOAD );
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

					//!!!KELLY - this cultist was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "CULT_COVER", CULTIST_SENTENCE_VOLUME, CULTIST_ATTN, 0, m_voicePitch);
						m_iSentence = CULTIST_SENT_COVER;
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
						return GetScheduleOfType ( SCHED_CULTIST_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_CULTIST_ENGAGE ) )
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
				if ( OccupySlot( bits_SLOTS_CULTIST_ENGAGE ) )
				{
					//!!!KELLY - cultist cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "CULT_CHARGE", CULTIST_SENTENCE_VOLUME, CULTIST_ATTN, 0, m_voicePitch);
						m_iSentence = CULTIST_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_CULTIST_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - cultist is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// cultist's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "CULT_TAUNT", CULTIST_SENTENCE_VOLUME, CULTIST_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_CULTIST_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CCultist :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				return &slCultistTakeCover[ 0 ];
			}
			else
			{
				return &slCultistTakeCover[ 0 ];
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slCultistTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_CULTIST_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_CULTIST_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_CULTIST_ELOF_FAIL:
		{
			// human cultist is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_CULTIST_ESTABLISH_LINE_OF_FIRE:
		{
			return &slCultistEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			if (m_fStanding)
				return &slCultistRangeAttack1B[ 0 ];
			else
				return &slCultistRangeAttack1A[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slCultistCombatFace[ 0 ];
		}
	case SCHED_CULTIST_WAIT_FACE_ENEMY:
		{
			return &slCultistWaitInCover[ 0 ];
		}
	case SCHED_CULTIST_SWEEP:
		{
			return &slCultistSweep[ 0 ];
		}
	case SCHED_CULTIST_COVER_AND_RELOAD:
		{
			return &slCultistHideReload[ 0 ];
		}
	case SCHED_CULTIST_FOUND_ENEMY:
		{
			return &slCultistFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slCultistFail[ 0 ];
				}
			}

			return &slCultistVictoryDance[ 0 ];
		}
	case SCHED_CULTIST_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slCultistSignalSuppress[ 0 ];
			}
			else
			{
				return &slCultistSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// cultist has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slCultistCombatFail[ 0 ];
			}

			return &slCultistFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}


//=========================================================
// DEAD CULTIST PROP
//=========================================================

char *CDeadCultist::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadCultist::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_cultist_dead, CDeadCultist );

//=========================================================
// ********** DeadCultist SPAWN **********
//=========================================================
void CDeadCultist :: Spawn( void )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/cultist.mdl");
	
	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/cultist.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead cultist with bad pose\n" );
	}

	// Corpses have less health
	pev->health			= 8;

	// map old bodies onto new bodies
	switch( pev->body )
	{
	case 0: // Cultist with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_GRIM );
		SetBodygroup( GUN_GROUP, GUN_REVOLVER );
		break;
	case 1: // Commander with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_BEARDY );
		SetBodygroup( GUN_GROUP, GUN_REVOLVER );
		break;
	case 2: // Cultist no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_UGLY );
		SetBodygroup( GUN_GROUP, GUN_NONE );
		break;
	case 3: // Commander no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, HEAD_GRIM );
		SetBodygroup( GUN_GROUP, GUN_NONE );
		break;
	}

	MonsterInitDead();
}
